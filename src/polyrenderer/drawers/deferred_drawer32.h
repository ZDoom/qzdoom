/*
**  Projected triangle drawer
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

#include "screen_triangle.h"

namespace TriScreenDrawerModes
{
	template<typename SamplerT, typename FilterModeT>
	FORCEINLINE unsigned int Sample32Deferred(int32_t u, int32_t v, const uint32_t *texPixels, int texWidth, int texHeight, uint32_t oneU, uint32_t oneV, uint32_t color, const uint32_t *translation)
	{
		uint32_t texel;
		if (SamplerT::Mode == (int)Samplers::Shaded || SamplerT::Mode == (int)Samplers::Stencil || SamplerT::Mode == (int)Samplers::Fill || SamplerT::Mode == (int)Samplers::Fuzz)
		{
			return color;
		}
		else if (SamplerT::Mode == (int)Samplers::Translated)
		{
			const uint8_t *texpal = (const uint8_t *)texPixels;
			uint32_t texelX = ((((uint32_t)u << 8) >> 16) * texWidth) >> 16;
			uint32_t texelY = ((((uint32_t)v << 8) >> 16) * texHeight) >> 16;
			return translation[texpal[texelX * texHeight + texelY]];
		}
		else if (FilterModeT::Mode == (int)FilterModes::Nearest)
		{
			uint32_t texelX = ((((uint32_t)u << 8) >> 16) * texWidth) >> 16;
			uint32_t texelY = ((((uint32_t)v << 8) >> 16) * texHeight) >> 16;
			texel = texPixels[texelX * texHeight + texelY];
		}
		else
		{
			u -= oneU >> 1;
			v -= oneV >> 1;

			unsigned int frac_x0 = (((uint32_t)u << 8) >> FRACBITS) * texWidth;
			unsigned int frac_x1 = ((((uint32_t)u << 8) + oneU) >> FRACBITS) * texWidth;
			unsigned int frac_y0 = (((uint32_t)v << 8) >> FRACBITS) * texHeight;
			unsigned int frac_y1 = ((((uint32_t)v << 8) + oneV) >> FRACBITS) * texHeight;
			unsigned int x0 = frac_x0 >> FRACBITS;
			unsigned int x1 = frac_x1 >> FRACBITS;
			unsigned int y0 = frac_y0 >> FRACBITS;
			unsigned int y1 = frac_y1 >> FRACBITS;

			unsigned int p00 = texPixels[x0 * texHeight + y0];
			unsigned int p01 = texPixels[x0 * texHeight + y1];
			unsigned int p10 = texPixels[x1 * texHeight + y0];
			unsigned int p11 = texPixels[x1 * texHeight + y1];

			unsigned int inv_a = (frac_x1 >> (FRACBITS - 4)) & 15;
			unsigned int inv_b = (frac_y1 >> (FRACBITS - 4)) & 15;
			unsigned int a = 16 - inv_a;
			unsigned int b = 16 - inv_b;

			unsigned int sred = (RPART(p00) * (a * b) + RPART(p01) * (inv_a * b) + RPART(p10) * (a * inv_b) + RPART(p11) * (inv_a * inv_b) + 127) >> 8;
			unsigned int sgreen = (GPART(p00) * (a * b) + GPART(p01) * (inv_a * b) + GPART(p10) * (a * inv_b) + GPART(p11) * (inv_a * inv_b) + 127) >> 8;
			unsigned int sblue = (BPART(p00) * (a * b) + BPART(p01) * (inv_a * b) + BPART(p10) * (a * inv_b) + BPART(p11) * (inv_a * inv_b) + 127) >> 8;
			unsigned int salpha = (APART(p00) * (a * b) + APART(p01) * (inv_a * b) + APART(p10) * (a * inv_b) + APART(p11) * (inv_a * inv_b) + 127) >> 8;

			texel = (salpha << 24) | (sred << 16) | (sgreen << 8) | sblue;
		}

		if (SamplerT::Mode == (int)Samplers::Skycap)
		{
			int start_fade = 2; // How fast it should fade out

			int alpha_top = clamp(v >> (16 - start_fade), 0, 256);
			int alpha_bottom = clamp(((2 << 24) - v) >> (16 - start_fade), 0, 256);
			int a = MIN(alpha_top, alpha_bottom);
			int inv_a = 256 - a;

			uint32_t r = RPART(texel);
			uint32_t g = GPART(texel);
			uint32_t b = BPART(texel);
			uint32_t fg_a = APART(texel);
			uint32_t bg_red = RPART(color);
			uint32_t bg_green = GPART(color);
			uint32_t bg_blue = BPART(color);
			r = (r * a + bg_red * inv_a + 127) >> 8;
			g = (g * a + bg_green * inv_a + 127) >> 8;
			b = (b * a + bg_blue * inv_a + 127) >> 8;
			return MAKEARGB(fg_a, r, g, b);
		}
		else
		{
			return texel;
		}
	}
}

template<typename SamplerT>
class TriScreenDrawer32Deferred
{
public:
	static void Execute(int x, int y, uint32_t mask0, uint32_t mask1, const TriDrawTriangleArgs *args)
	{
		using namespace TriScreenDrawerModes;

		if (SamplerT::Mode == (int)Samplers::Texture)
		{
			if (args->uniforms->NearestFilter())
				DrawBlock<NearestFilter>(x, y, mask0, mask1, args);
			else
				DrawBlock<LinearFilter>(x, y, mask0, mask1, args);
		}
		else
		{
			DrawBlock<NearestFilter>(x, y, mask0, mask1, args);
		}
	}

private:
	template<typename FilterModeT>
	FORCEINLINE static void DrawBlock(int destX, int destY, uint32_t mask0, uint32_t mask1, const TriDrawTriangleArgs *args)
	{
		using namespace TriScreenDrawerModes;

		// Calculate gradients
		const TriVertex &v1 = *args->v1;
		ScreenTriangleStepVariables gradientX = args->gradientX;
		ScreenTriangleStepVariables gradientY = args->gradientY;
		ScreenTriangleStepVariables blockPosY;
		blockPosY.W = v1.w + gradientX.W * (destX - v1.x) + gradientY.W * (destY - v1.y);
		blockPosY.U = v1.u * v1.w + gradientX.U * (destX - v1.x) + gradientY.U * (destY - v1.y);
		blockPosY.V = v1.v * v1.w + gradientX.V * (destX - v1.x) + gradientY.V * (destY - v1.y);
		gradientX.W *= 8.0f;
		gradientX.U *= 8.0f;
		gradientX.V *= 8.0f;
		float stepZ = gradientX.W * (1.0f / 8.0f);

		// Output
		uint32_t * RESTRICT destOrg = (uint32_t*)args->dest;
		int pitch = args->pitch;
		uint32_t *dest = destOrg + destX + destY * pitch;

		int block = (destX >> 3) + (destY >> 3) * args->stencilPitch;
		float *zdest = args->zbuffer + block * 64;

		// Light
		uint32_t is_fixed_light = args->uniforms->FixedLight();
		uint32_t light = args->uniforms->Light();
		light = ((light & 254) | is_fixed_light) << 24;

		// Sampling stuff
		uint32_t color = args->uniforms->Color();
		const uint32_t * RESTRICT translation = (const uint32_t *)args->uniforms->Translation();
		const uint32_t * RESTRICT texPixels = (const uint32_t *)args->uniforms->TexturePixels();
		uint32_t texWidth = args->uniforms->TextureWidth();
		uint32_t texHeight = args->uniforms->TextureHeight();
		uint32_t oneU, oneV;
		if (SamplerT::Mode != (int)Samplers::Fill)
		{
			oneU = ((0x800000 + texWidth - 1) / texWidth) * 2 + 1;
			oneV = ((0x800000 + texHeight - 1) / texHeight) * 2 + 1;
		}
		else
		{
			oneU = 0;
			oneV = 0;
		}

		if (mask0 == 0xffffffff && mask1 == 0xffffffff)
		{
			for (int y = 0; y < 8; y++)
			{
				float rcpW = 0x01000000 / blockPosY.W;
				int32_t posU = (int32_t)(blockPosY.U * rcpW);
				int32_t posV = (int32_t)(blockPosY.V * rcpW);
				float posZ = blockPosY.W;

				ScreenTriangleStepVariables blockPosX = blockPosY;
				blockPosX.W += gradientX.W;
				blockPosX.U += gradientX.U;
				blockPosX.V += gradientX.V;

				rcpW = 0x01000000 / blockPosX.W;
				int32_t nextU = (int32_t)(blockPosX.U * rcpW);
				int32_t nextV = (int32_t)(blockPosX.V * rcpW);
				int32_t stepU = (nextU - posU) / 8;
				int32_t stepV = (nextV - posV) / 8;

				for (int ix = 0; ix < 8; ix++)
				{
					// Sample fgcolor
					unsigned int ifgcolor = Sample32Deferred<SamplerT, FilterModeT>(posU, posV, texPixels, texWidth, texHeight, oneU, oneV, color, translation);
					float depth = posZ;
					posU += stepU;
					posV += stepV;
					posZ += stepZ;

					// Store lightlevel in alpha channel
					ifgcolor = light | (ifgcolor & 0x00ffffff);

					// Store result
					dest[ix] = ifgcolor;
					zdest[ix] = depth;
				}

				blockPosY.W += gradientY.W;
				blockPosY.U += gradientY.U;
				blockPosY.V += gradientY.V;

				dest += pitch;
				zdest += 8;
			}
		}
		else
		{
			// mask0 loop:
			for (int y = 0; y < 4; y++)
			{
				float rcpW = 0x01000000 / blockPosY.W;
				int32_t posU = (int32_t)(blockPosY.U * rcpW);
				int32_t posV = (int32_t)(blockPosY.V * rcpW);
				float posZ = blockPosY.W;

				ScreenTriangleStepVariables blockPosX = blockPosY;
				blockPosX.W += gradientX.W;
				blockPosX.U += gradientX.U;
				blockPosX.V += gradientX.V;

				rcpW = 0x01000000 / blockPosX.W;
				int32_t nextU = (int32_t)(blockPosX.U * rcpW);
				int32_t nextV = (int32_t)(blockPosX.V * rcpW);
				int32_t stepU = (nextU - posU) / 8;
				int32_t stepV = (nextV - posV) / 8;

				for (int x = 0; x < 8; x++)
				{
					// Sample fgcolor
					unsigned int ifgcolor = Sample32Deferred<SamplerT, FilterModeT>(posU, posV, texPixels, texWidth, texHeight, oneU, oneV, color, translation);
					float depth = posZ;
					posU += stepU;
					posV += stepV;
					posZ += stepZ;

					// Store lightlevel in alpha channel
					ifgcolor = light | (ifgcolor & 0x00ffffff);

					// Store result
					if (mask0 & (1 << 31))
					{
						dest[x] = ifgcolor;
						zdest[x] = depth;
					}

					mask0 <<= 1;
				}

				blockPosY.W += gradientY.W;
				blockPosY.U += gradientY.U;
				blockPosY.V += gradientY.V;

				dest += pitch;
				zdest += 8;
			}

			// mask1 loop:
			for (int y = 0; y < 4; y++)
			{
				float rcpW = 0x01000000 / blockPosY.W;
				int32_t posU = (int32_t)(blockPosY.U * rcpW);
				int32_t posV = (int32_t)(blockPosY.V * rcpW);
				float posZ = blockPosY.W;

				ScreenTriangleStepVariables blockPosX = blockPosY;
				blockPosX.W += gradientX.W;
				blockPosX.U += gradientX.U;
				blockPosX.V += gradientX.V;

				rcpW = 0x01000000 / blockPosX.W;
				int32_t nextU = (int32_t)(blockPosX.U * rcpW);
				int32_t nextV = (int32_t)(blockPosX.V * rcpW);
				int32_t stepU = (nextU - posU) / 8;
				int32_t stepV = (nextV - posV) / 8;

				for (int x = 0; x < 8; x++)
				{
					// Sample fgcolor
					unsigned int ifgcolor = Sample32Deferred<SamplerT, FilterModeT>(posU, posV, texPixels, texWidth, texHeight, oneU, oneV, color, translation);
					float depth = posZ;
					posU += stepU;
					posV += stepV;
					posZ += stepZ;

					// Store lightlevel in alpha channel
					ifgcolor = light | (ifgcolor & 0x00ffffff);

					// Store result
					if (mask1 & (1 << 31))
					{
						dest[x] = ifgcolor;
						zdest[x] = depth;
					}

					mask1 <<= 1;
				}

				blockPosY.W += gradientY.W;
				blockPosY.U += gradientY.U;
				blockPosY.V += gradientY.V;

				dest += pitch;
				zdest += 8;
			}
		}
	}
};
