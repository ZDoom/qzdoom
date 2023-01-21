//-----------------------------------------------------------------------------
//
// Copyright 1993-1996 id Software
// Copyright 1999-2016 Randy Heit
// Copyright 2016 Magnus Norddahl
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/
//
//-----------------------------------------------------------------------------
//

#include <stdlib.h>
#include <float.h>


#include "filesystem.h"
#include "doomdef.h"
#include "doomstat.h"
#include "r_sky.h"
#include "stats.h"
#include "v_video.h"
#include "a_sharedglobal.h"
#include "c_console.h"
#include "cmdlib.h"
#include "d_net.h"
#include "g_level.h"
#include "texturemanager.h"
#include "swrenderer/scene/r_opaque_pass.h"
#include "r_skyplane.h"
#include "swrenderer/scene/r_3dfloors.h"
#include "v_palette.h"
#include "r_data/colormaps.h"
#include "swrenderer/drawers/r_draw_rgba.h"
#include "a_dynlight.h"
#include "swrenderer/segments/r_clipsegment.h"
#include "swrenderer/segments/r_drawsegment.h"
#include "swrenderer/line/r_wallsetup.h"
#include "swrenderer/line/r_walldraw.h"
#include "swrenderer/scene/r_portal.h"
#include "swrenderer/scene/r_scene.h"
#include "swrenderer/scene/r_light.h"
#include "swrenderer/viewport/r_viewport.h"
#include "r_memory.h"
#include "swrenderer/r_renderthread.h"
#include "g_levellocals.h"

CVAR(Bool, r_linearsky, false, CVAR_ARCHIVE | CVAR_GLOBALCONFIG);
EXTERN_CVAR(Int, r_skymode)
EXTERN_CVAR(Bool, cl_oldfreelooklimit)

namespace swrenderer
{
	static FSoftwareTexture *GetSWTex(FTextureID texid, bool allownull = true)
	{
		return GetPalettedSWTexture(texid, true, false, true);
	}

	RenderSkyPlane::RenderSkyPlane(RenderThread *thread)
	{
		Thread = thread;
		auto Level = Thread->Viewport->Level();

		auto sskytex1 = GetPalettedSWTexture(Level->skytexture1, true, false, true);
		auto sskytex2 = GetPalettedSWTexture(Level->skytexture2, true, false, true);

		if (sskytex1 == nullptr || sskytex2 == nullptr)
			return;

		skytexturemid = 0;
		int skyheight = sskytex1->GetScaledHeight();
		skyoffset = cl_oldfreelooklimit? 0 : skyheight == 256? 166 : skyheight >= 240? 150 : skyheight >= 200? 110 : 138;
		if (skyheight >= 128 && skyheight < 200)
		{
			skytexturemid = -28;
		}
		else if (skyheight >= 200)
		{
			skytexturemid = (200 - skyheight) * sskytex1->GetScale().Y + ((r_skymode == 2 && !(Level->flags & LEVEL_FORCETILEDSKY)) ? sskytex1->GetSkyOffset() : 0);
		}

		if (viewwidth != 0 && viewheight != 0)
		{
			skyiscale = float(r_Yaspect / freelookviewheight);
			skyscale = freelookviewheight / r_Yaspect;

			skyiscale *= float(thread->Viewport->viewpoint.FieldOfView.Degrees() / 90.);
			skyscale *= float(90. / thread->Viewport->viewpoint.FieldOfView.Degrees());
		}

		if (Level->skystretch)
		{
			skyscale *= (double)(SKYSTRETCH_HEIGHT + skyoffset) / skyheight;
			skyiscale *= skyheight / (float)(SKYSTRETCH_HEIGHT + skyoffset);
			skytexturemid *= skyheight / (double)(SKYSTRETCH_HEIGHT + skyoffset);
		}

		// The standard Doom sky texture is 256 pixels wide, repeated 4 times over 360 degrees,
		// giving a total sky width of 1024 pixels. So if the sky texture is no wider than 1024,
		// we map it to a cylinder with circumfrence 1024. For larger ones, we use the width of
		// the texture as the cylinder's circumfrence.
		sky1cyl = max(sskytex1->GetWidth(), fixed_t(sskytex1->GetScale().X * 1024));
		sky2cyl = max(sskytex2->GetWidth(), fixed_t(sskytex2->GetScale().Y * 1024));
	}

	void RenderSkyPlane::Render(VisiblePlane *pl)
	{
		FTextureID sky1tex, sky2tex;
		double frontdpos = 0, backdpos = 0;
		auto Level = Thread->Viewport->Level();

		if ((Level->flags & LEVEL_SWAPSKIES) && !(Level->flags & LEVEL_DOUBLESKY))
		{
			sky1tex = Level->skytexture2;
		}
		else
		{
			sky1tex = Level->skytexture1;
		}
		sky2tex = Level->skytexture2;
		skymid = skytexturemid;
		skyangle = Thread->Viewport->viewpoint.Angles.Yaw.BAMs();

		if (pl->picnum == skyflatnum)
		{
			if (!(pl->sky & PL_SKYFLAT))
			{	// use sky1
			sky1:
				frontskytex = GetSWTex(sky1tex);
				if (Level->flags & LEVEL_DOUBLESKY)
					backskytex = GetSWTex(sky2tex);
				else
					backskytex = NULL;
				skyflip = 0;
				frontdpos = Level->sky1pos;
				backdpos = Level->sky2pos;
				frontcyl = sky1cyl;
				backcyl = sky2cyl;
			}
			else if (pl->sky == PL_SKYFLAT)
			{	// use sky2
				frontskytex = GetSWTex(sky2tex);
				backskytex = NULL;
				frontcyl = sky2cyl;
				skyflip = 0;
				frontdpos = Level->sky2pos;
			}
			else
			{	// MBF's linedef-controlled skies
				// Sky Linedef
				const line_t *l = &Level->lines[(pl->sky & ~PL_SKYFLAT) - 1];

				// Sky transferred from first sidedef
				const side_t *s = l->sidedef[0];
				int pos;

				// Texture comes from upper texture of reference sidedef
				// [RH] If swapping skies, then use the lower sidedef
				if (Level->flags & LEVEL_SWAPSKIES && s->GetTexture(side_t::bottom).isValid())
				{
					pos = side_t::bottom;
				}
				else
				{
					pos = side_t::top;
				}

				frontskytex = GetSWTex(s->GetTexture(pos));
				if (frontskytex == nullptr)
				{ // [RH] The blank texture: Use normal sky instead.
					goto sky1;
				}
				backskytex = NULL;

				// Horizontal offset is turned into an angle offset,
				// to allow sky rotation as well as careful positioning.
				// However, the offset is scaled very small, so that it
				// allows a long-period of sky rotation.
				skyangle += FLOAT2FIXED(s->GetTextureXOffset(pos));

				// Vertical offset allows careful sky positioning.
				skymid = s->GetTextureYOffset(pos) - 28.0;

				// We sometimes flip the picture horizontally.
				//
				// Doom always flipped the picture, so we make it optional,
				// to make it easier to use the new feature, while to still
				// allow old sky textures to be used.
				skyflip = l->args[2] ? 0u : ~0u;

				int frontxscale = int(frontskytex->GetScale().X * 1024);
				frontcyl = max(frontskytex->GetWidth(), frontxscale);
				if (Level->skystretch)
				{
					skymid = skymid * frontskytex->GetScaledHeight() / (SKYSTRETCH_HEIGHT + skyoffset);
				}
			}
		}
		frontpos = int(fmod(frontdpos, sky1cyl * 65536.0));
		if (backskytex != NULL)
		{
			backpos = int(fmod(backdpos, sky2cyl * 65536.0));
		}

		drawerargs.SetStyle();

		DrawSky(pl);
	}

	static uint32_t UMulScale16(uint32_t a, uint32_t b) { return (uint32_t)(((uint64_t)a * b) >> 16); }

	void RenderSkyPlane::DrawSkyColumnStripe(int start_x, int y1, int y2, double scale, double texturemid, double yrepeat)
	{
		RenderPortal *renderportal = Thread->Portal.get();
		auto viewport = Thread->Viewport.get();
		auto Level = viewport->Level();

		double uv_stepd = skyiscale * yrepeat;
		double v = (texturemid + uv_stepd * (y1 - viewport->CenterY + 0.5)) / frontskytex->GetHeight();
		double v_step = uv_stepd / frontskytex->GetHeight();

		uint32_t uv_pos = (uint32_t)(int32_t)(v * 0x01000000);
		uint32_t uv_step = (uint32_t)(int32_t)(v_step * 0x01000000);

		int x = start_x;
		if (renderportal->MirrorFlags & RF_XFLIP)
			x = (viewwidth - x);

		uint32_t ang, angle1, angle2;

		if (r_linearsky)
		{
			angle_t xangle = (angle_t)((0.5 - x / (double)viewwidth) * viewport->viewwindow.FocalTangent * ANGLE_90);
			ang = (skyangle + xangle) ^ skyflip;
		}
		else
		{
			ang = (skyangle + viewport->xtoviewangle[x]) ^ skyflip;
		}
		angle1 = UMulScale16(ang, frontcyl) + frontpos;
		angle2 = UMulScale16(ang, backcyl) + backpos;

		auto skycapcolors = Thread->GetSkyCapColor(frontskytex);

		drawerargs.SetFrontTexture(Thread, frontskytex, angle1);
		drawerargs.SetBackTexture(Thread, backskytex, angle2);
		drawerargs.SetTextureVStep(uv_step);
		drawerargs.SetTextureVPos(uv_pos);
		drawerargs.SetDest(viewport, start_x, y1);
		drawerargs.SetCount(y2 - y1);
		drawerargs.SetFadeSky(r_skymode == 2 && !(Level->flags & LEVEL_FORCETILEDSKY));
		drawerargs.SetSolidTop(skycapcolors.first);
		drawerargs.SetSolidBottom(skycapcolors.second);

		if (!backskytex)
			drawerargs.DrawSingleSkyColumn(Thread);
		else
			drawerargs.DrawDoubleSkyColumn(Thread);

	}

	void RenderSkyPlane::DrawSkyColumn(int start_x, int y1, int y2)
	{
		if (1 << frontskytex->GetHeightBits() >= frontskytex->GetPhysicalHeight())
		{
			double texturemid = skymid * frontskytex->GetScale().Y + frontskytex->GetHeight();
			DrawSkyColumnStripe(start_x, y1, y2, frontskytex->GetScale().Y, texturemid, frontskytex->GetScale().Y);
		}
		else
		{
			auto viewport = Thread->Viewport.get();
			double yrepeat = frontskytex->GetScale().Y;
			double scale = frontskytex->GetScale().Y * skyscale;
			double iscale = 1 / scale;
			short drawheight = short(frontskytex->GetHeight() * scale);
			double topfrac = fmod(skymid + iscale * (1 - viewport->CenterY), frontskytex->GetHeight());
			if (topfrac < 0) topfrac += frontskytex->GetHeight();
			double texturemid = topfrac - iscale * (1 - viewport->CenterY);
			DrawSkyColumnStripe(start_x, y1, y2, scale, texturemid, yrepeat);
		}
	}

	void RenderSkyPlane::DrawSky(VisiblePlane *pl)
	{
		int x1 = pl->left;
		int x2 = pl->right;
		short *uwal = (short *)pl->top;
		short *dwal = (short *)pl->bottom;

		for (int x = x1; x < x2; x++)
		{
			int y1 = uwal[x];
			int y2 = dwal[x];
			if (y2 <= y1)
				continue;

			DrawSkyColumn(x, y1, y2);
		}
	}
}
