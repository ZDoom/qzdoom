/*
**  Drawer commands for spans
**  Copyright (c) 2016 Magnus Norddahl
**
**  This software is provided 'as-is', without any express or implied
**  warranty.  In no event will the authors be held liable for any damages
**  arising from the use of this software.
**
**  Permission is granted to anyone to use this software for any purpose,
**  including commercial applications, and to alter it and redistribute it
**  freely, subject to the following restrictions:
**
**  1. The origin of this software must not be misrepresented; you must not
**     claim that you wrote the original software. If you use this software
**     in a product, an acknowledgment in the product documentation would be
**     appreciated but is not required.
**  2. Altered source versions must be plainly marked as such, and must not be
**     misrepresented as being the original software.
**  3. This notice may not be removed or altered from any source distribution.
**
*/

#pragma once

#include "swrenderer/drawers/r_draw_rgba.h"
#include "swrenderer/viewport/r_skyspheredrawer.h"

namespace swrenderer
{
	class DrawSkySphere32Command : public DrawerCommand
	{
	public:
		DrawSkySphere32Command(const SkySphereDrawerArgs &args) : args(args)
		{
		}

		void Execute(DrawerThread *thread) override
		{
			uint32_t *dest = (uint32_t *)args.Dest();
			int count = args.Count();
			int pitch = args.Viewport()->RenderTarget->GetPitch();

			const uint32_t *textures[6] =
			{
				(const uint32_t*)args.TexturePixels(0),
				(const uint32_t*)args.TexturePixels(1),
				(const uint32_t*)args.TexturePixels(2),
				(const uint32_t*)args.TexturePixels(3),
				(const uint32_t*)args.TexturePixels(4),
				(const uint32_t*)args.TexturePixels(5),
			};
			int texWidth = args.TextureWidth();
			int texHeight = args.TextureHeight();

			count = thread->count_for_thread(args.DestY(), count);
			dest = thread->dest_for_thread(args.DestY(), pitch, dest);
			pitch *= thread->num_cores;

			for (int i = 0; i < count; i++)
			{
				*dest = 0xffffff00;
				dest += pitch;
			}
		}

		FString DebugInfo() override { return "DrawSkySphere32Command"; }

	protected:
		SkySphereDrawerArgs args;
	};
}
