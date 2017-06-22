//-----------------------------------------------------------------------------
//
// Copyright 2017 Rachael Alexanderson
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
#include "templates.h"
#include "i_system.h"
#include "w_wad.h"
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
#include "v_palette.h"
#include "r_data/colormaps.h"
#include "r_skysphereplane.h"
#include "swrenderer/r_memory.h"
#include "swrenderer/r_renderthread.h"
#include "g_levellocals.h"

namespace swrenderer
{
	RenderSkySpherePlane::RenderSkySpherePlane(RenderThread *thread)
	{
		Thread = thread;
	}

	void RenderSkySpherePlane::Render(VisiblePlane *pl)
	{
		FTexture *textures[6] =
		{
			TexMan(sky1texture),
			TexMan(sky1texture),
			TexMan(sky1texture),
			TexMan(sky1texture),
			TexMan(sky1texture),
			TexMan(sky1texture)
		};

		drawerargs.SetTextures(Thread, textures);

		int start = pl->left;
		for (int x = pl->left; x < pl->right; x++)
		{
			int y0 = pl->top[x];
			int y1 = pl->bottom[x];

			drawerargs.SetDest(Thread->Viewport.get(), x, y0);
			drawerargs.SetCount(y1 - y0);
			drawerargs.DrawColumn(Thread);
		}
	}
}
