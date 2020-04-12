// 
//---------------------------------------------------------------------------
//
// Copyright(C) 2018 Christoph Oelckers
// All rights reserved.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/
//
//--------------------------------------------------------------------------
//
/*
** 2d drawer
** Renderer interface
**
*/

#include "doomstat.h"
#include "v_video.h"
#include "cmdlib.h"
#include "r_defs.h"
#include "hwrenderer/data/buffers.h"
#include "hwrenderer/data/flatvertices.h"
#include "hwrenderer/data/hw_viewpointbuffer.h"
#include "hwrenderer/utility/hw_clock.h"
#include "hwrenderer/utility/hw_cvars.h"
#include "hwrenderer/scene/hw_renderstate.h"
#include "r_videoscale.h"


//===========================================================================
// 
// Vertex buffer for 2D drawer
//
//===========================================================================

class F2DVertexBuffer
{
	IVertexBuffer *mVertexBuffer;
	IIndexBuffer *mIndexBuffer;


public:

	F2DVertexBuffer()
	{
		mVertexBuffer = screen->CreateVertexBuffer();
		mIndexBuffer = screen->CreateIndexBuffer();

		static const FVertexBufferAttribute format[] = {
			{ 0, VATTR_VERTEX, VFmt_Float3, (int)myoffsetof(F2DDrawer::TwoDVertex, x) },
			{ 0, VATTR_TEXCOORD, VFmt_Float2, (int)myoffsetof(F2DDrawer::TwoDVertex, u) },
			{ 0, VATTR_COLOR, VFmt_Byte4, (int)myoffsetof(F2DDrawer::TwoDVertex, color0) }
		};
		mVertexBuffer->SetFormat(1, 3, sizeof(F2DDrawer::TwoDVertex), format);
	}
	~F2DVertexBuffer()
	{
		delete mIndexBuffer;
		delete mVertexBuffer;
	}

	void UploadData(F2DDrawer::TwoDVertex *vertices, int vertcount, int *indices, int indexcount)
	{
		mVertexBuffer->SetData(vertcount * sizeof(*vertices), vertices, false);
		mIndexBuffer->SetData(indexcount * sizeof(unsigned int), indices, false);
	}

	std::pair<IVertexBuffer *, IIndexBuffer *> GetBufferObjects() const
	{
		return std::make_pair(mVertexBuffer, mIndexBuffer);
	}
};

//===========================================================================
// 
// Draws the 2D stuff. This is the version for OpenGL 3 and later.
//
//===========================================================================

CVAR(Bool, gl_aalines, false, CVAR_ARCHIVE) 

void Draw2D(F2DDrawer *drawer, FRenderState &state)
{
	twoD.Clock();

	const auto &mScreenViewport = screen->mScreenViewport;
	state.SetViewport(mScreenViewport.left, mScreenViewport.top, mScreenViewport.width, mScreenViewport.height);
	screen->mViewpoints->Set2D(state, twod->GetWidth(), twod->GetHeight());

	state.EnableDepthTest(false);
	state.EnableMultisampling(false);
	state.EnableLineSmooth(gl_aalines);

	auto &vertices = drawer->mVertices;
	auto &indices = drawer->mIndices;
	auto &commands = drawer->mData;

	if (commands.Size() == 0)
	{
		twoD.Unclock();
		return;
	}

	if (drawer->mIsFirstPass)
	{
		for (auto &v : vertices)
		{
			// Change from BGRA to RGBA
			std::swap(v.color0.r, v.color0.b);
		}
	}
	F2DVertexBuffer vb;
	vb.UploadData(&vertices[0], vertices.Size(), &indices[0], indices.Size());
	state.SetVertexBuffer(&vb);
	state.EnableFog(false);

	for(auto &cmd : commands)
	{

		int gltrans = -1;
		state.SetRenderStyle(cmd.mRenderStyle);
		state.EnableBrightmap(!(cmd.mRenderStyle.Flags & STYLEF_ColorIsFixed));
		state.EnableFog(2);	// Special 2D mode 'fog'.

		state.SetTextureMode(cmd.mDrawMode);

		int sciX, sciY, sciW, sciH;
		if (cmd.mFlags & F2DDrawer::DTF_Scissor)
		{
			// scissor test doesn't use the current viewport for the coordinates, so use real screen coordinates
			// Note that the origin here is the lower left corner!
			sciX = screen->ScreenToWindowX(cmd.mScissor[0]);
			sciY = screen->ScreenToWindowY(cmd.mScissor[3]);
			sciW = screen->ScreenToWindowX(cmd.mScissor[2]) - sciX;
			sciH = screen->ScreenToWindowY(cmd.mScissor[1]) - sciY;
		}
		else
		{
			sciX = sciY = sciW = sciH = -1;
		}
		state.SetScissor(sciX, sciY, sciW, sciH);

		if (cmd.mSpecialColormap[0].a != 0)
		{
			state.SetTextureMode(TM_FIXEDCOLORMAP);
			state.SetObjectColor(cmd.mSpecialColormap[0]);
			state.SetAddColor(cmd.mSpecialColormap[1]);
		}
		state.SetFog(cmd.mColor1, 0);
		state.SetColor(1, 1, 1, 1, cmd.mDesaturate); 

		state.AlphaFunc(Alpha_GEqual, 0.f);

		if (cmd.mTexture != nullptr)
		{
			auto mat = FMaterial::ValidateTexture(cmd.mTexture, false);
			if (mat == nullptr) continue;

			state.SetMaterial(mat, cmd.mFlags & F2DDrawer::DTF_Wrap ? CLAMP_NONE : CLAMP_XY_NOMIP, cmd.mTranslationId, -1);
			state.EnableTexture(true);

			// Canvas textures are stored upside down
			if (cmd.mTexture->isHardwareCanvas())
			{
				state.mTextureMatrix.loadIdentity();
				state.mTextureMatrix.scale(1.f, -1.f, 1.f);
				state.mTextureMatrix.translate(0.f, 1.f, 0.0f);
				state.EnableTextureMatrix(true);
			}
			if (cmd.mFlags & F2DDrawer::DTF_Burn)
			{
				state.SetEffect(EFF_BURN);
			}
		}
		else
		{
			state.EnableTexture(false);
		}

		switch (cmd.mType)
		{
		case F2DDrawer::DrawTypeTriangles:
			state.DrawIndexed(DT_Triangles, cmd.mIndexIndex, cmd.mIndexCount);
			break;

		case F2DDrawer::DrawTypeLines:
			state.Draw(DT_Lines, cmd.mVertIndex, cmd.mVertCount);
			break;

		case F2DDrawer::DrawTypePoints:
			state.Draw(DT_Points, cmd.mVertIndex, cmd.mVertCount);
			break;

		}
		state.SetObjectColor(0xffffffff);
		state.SetObjectColor2(0);
		state.SetAddColor(0);
		state.EnableTextureMatrix(false);
		state.SetEffect(EFF_NONE);

	}
	state.SetScissor(-1, -1, -1, -1);

	state.SetRenderStyle(STYLE_Translucent);
	state.SetVertexBuffer(screen->mVertexData);
	state.EnableTexture(true);
	state.EnableBrightmap(true);
	state.SetTextureMode(TM_NORMAL);
	state.EnableFog(false);
	state.ResetColor();
	drawer->mIsFirstPass = false;
	twoD.Unclock();
}
