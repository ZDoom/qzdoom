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

#include <stddef.h>
#include "r_skyspheredrawer.h"
#include "swrenderer/r_renderthread.h"

namespace swrenderer
{
	void SkySphereDrawerArgs::DrawColumn(RenderThread *thread)
	{
		thread->Drawers(dc_viewport)->DrawSkySphereColumn(*this);
	}

	void SkySphereDrawerArgs::SetDest(RenderViewport *viewport, int x, int y)
	{
		dc_dest = viewport->GetDest(x, y);
		dc_dest_y = y;
		dc_viewport = viewport;
	}

	void SkySphereDrawerArgs::SetTextures(RenderThread *thread, FTexture **textures)
	{
		if (thread->Viewport->RenderTarget->IsBgra())
		{
			for (int i = 0; i < 6; i++)
				dc_textures[i] = (const uint8_t*)textures[i]->GetPixelsBgra();
		}
		else
		{
			for (int i = 0; i < 6; i++)
				dc_textures[i] = textures[i]->GetPixels();
		}
		dc_texture_width = textures[0]->GetWidth();
		dc_texture_height = textures[0]->GetHeight();
	}
}
