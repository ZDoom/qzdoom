
#pragma once

#include "r_drawerargs.h"

struct FSWColormap;
struct FLightNode;

namespace swrenderer
{
	class RenderThread;
	
	class SkySphereDrawerArgs : public DrawerArgs
	{
	public:
		void SetDest(RenderViewport *viewport, int x, int y);
		void SetCount(int count) { dc_count = count; }
		void SetTextures(RenderThread *thread, FTexture **textures);

		uint8_t *Dest() const { return dc_dest; }
		int DestY() const { return dc_dest_y; }
		int Count() const { return dc_count; }
		const uint8_t *TexturePixels(int side) const { return dc_textures[side]; }
		int TextureWidth() const { return dc_texture_width; }
		int TextureHeight() const { return dc_texture_height; }

		RenderViewport *Viewport() const { return dc_viewport; }

		void DrawColumn(RenderThread *thread);

	private:
		uint8_t *dc_dest = nullptr;
		int dc_dest_y = 0;
		int dc_count = 0;
		const uint8_t *dc_textures[6];
		int dc_texture_width = 0;
		int dc_texture_height = 0;
		RenderViewport *dc_viewport = nullptr;
	};
}
