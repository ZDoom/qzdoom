/*
** texture.cpp
** The base texture class
**
**---------------------------------------------------------------------------
** Copyright 2004-2007 Randy Heit
** Copyright 2006-2018 Christoph Oelckers
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
**
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 3. The name of the author may not be used to endorse or promote products
**    derived from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
** OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
** IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
** INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
** NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
** THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**---------------------------------------------------------------------------
**
**
*/

#include "r_swtexture.h"
#include "bitmap.h"
#include "m_alloc.h"
#include "imagehelpers.h"
#include "texturemanager.h"
#include <mutex>

inline EUpscaleFlags scaleFlagFromUseType(ETextureType useType)
{
	switch (useType)
	{
	case ETextureType::Sprite:
	case ETextureType::SkinSprite:
		return UF_Sprite;

	case ETextureType::FontChar:
		return UF_Font;

	default:
		return UF_Texture;
	}
}
//==========================================================================
//
//
//
//==========================================================================

FSoftwareTexture::FSoftwareTexture(FGameTexture *tex)
{
	mTexture = tex;
	mSource = tex->GetTexture();

	mBufferFlags = CTF_ProcessData;

	if (shouldUpscale(tex, scaleFlagFromUseType(tex->GetUseType()))) mBufferFlags |= CTF_Upscale;
	// calculate the real size after running the scaler.
	auto info = mSource->CreateTexBuffer(0, CTF_CheckOnly| mBufferFlags);
	mPhysicalWidth = info.mWidth;
	mPhysicalHeight = info.mHeight; 
	mPhysicalScale = tex->GetTexelWidth() > 0 ? mPhysicalWidth / tex->GetTexelWidth() : mPhysicalWidth;
	Scale.X = (double)tex->GetTexelWidth() / tex->GetDisplayWidth();
	Scale.Y = (double)tex->GetTexelHeight() / tex->GetDisplayHeight();
	CalcBitSize();
}

//==========================================================================
//
//
//
//==========================================================================

void FSoftwareTexture::CalcBitSize ()
{
	// WidthBits is rounded down, and HeightBits is rounded up
	int i;
	
	for (i = 0; (1 << i) < GetPhysicalWidth(); ++i)
	{ }
	
	WidthBits = i;
	
	// Having WidthBits that would allow for columns past the end of the
	// texture is not allowed, even if it means the entire texture is
	// not drawn.
	if (GetPhysicalWidth() < (1 << WidthBits))
	{
		WidthBits--;
	}
	WidthMask = (1 << WidthBits) - 1;
	
	// <hr>The minimum height is 2, because we cannot shift right 32 bits.</hr>
	// Scratch that. Somebody actually made a 1x1 texture, so now we have to handle it.
	for (i = 0; (1 << i) < GetPhysicalHeight(); ++i)
	{ }
	
	HeightBits = i;
}

//==========================================================================
//
// 
//
//==========================================================================

const uint8_t *FSoftwareTexture::GetPixelsLocked(int style)
{
	if (Pixels.Size() == 0 || CheckModified(style))
	{
		if (mPhysicalScale == 1)
		{
			Pixels = mSource->Get8BitPixels(style);
		}
		else
		{
			auto f = mBufferFlags;
			auto tempbuffer = mSource->CreateTexBuffer(0, f);
			Pixels.Resize(GetPhysicalWidth()*GetPhysicalHeight());
			PalEntry *pe = (PalEntry*)tempbuffer.mBuffer;
			if (!style)
			{
				for (int y = 0; y < GetPhysicalHeight(); y++)
				{
					for (int x = 0; x < GetPhysicalWidth(); x++)
					{
						Pixels[y + x * GetPhysicalHeight()] = ImageHelpers::RGBToPalette(false, pe[x + y * GetPhysicalWidth()], true);
					}
				}
			}
			else
			{
				for (int y = 0; y < GetPhysicalHeight(); y++)
				{
					for (int x = 0; x < GetPhysicalWidth(); x++)
					{
						Pixels[y + x * GetPhysicalHeight()] = pe[x + y * GetPhysicalWidth()].Luminance();
					}
				}
			}
		}
	}
	return Pixels.Data();
}

const uint32_t *FSoftwareTexture::GetPixelsBgraLocked()
{
	if (PixelsBgra.Size() == 0 || CheckModified(2))
	{
		if (mPhysicalScale == 1)
		{
			FBitmap bitmap = mSource->GetBgraBitmap(nullptr);
			GenerateBgraFromBitmap(bitmap);
		}
		else
		{
			auto tempbuffer = mSource->CreateTexBuffer(0, mBufferFlags);
			CreatePixelsBgraWithMipmaps();
			PalEntry *pe = (PalEntry*)tempbuffer.mBuffer;
			for (int y = 0; y < GetPhysicalHeight(); y++)
			{
				for (int x = 0; x < GetPhysicalWidth(); x++)
				{
					PixelsBgra[y + x * GetPhysicalHeight()] = pe[x + y * GetPhysicalWidth()];
				}
			}
			GenerateBgraMipmaps();
		}
	}
	return PixelsBgra.Data();
}

//==========================================================================
//
//
//
//==========================================================================

int FSoftwareTexture::CurrentUpdate = 0;
namespace swrenderer { extern std::mutex loadmutex; }

void FSoftwareTexture::UpdatePixels(int index)
{
	std::unique_lock<std::mutex> lock(swrenderer::loadmutex);
	if (Unlockeddata[index].LastUpdate != CurrentUpdate)
	{
		if (index != 2)
		{
			const uint8_t* Pixeldata = GetPixelsLocked(index);
			if (Spandata[index] == nullptr)
				Spandata[index] = CreateSpans(Pixeldata);
			Unlockeddata[index].Pixels = Pixeldata;
			Unlockeddata[index].LastUpdate = CurrentUpdate;
		}
		else
		{
			const uint32_t* Pixeldata = GetPixelsBgraLocked();
			if (Spandata[index] == nullptr)
				Spandata[index] = CreateSpans(Pixeldata);
			Unlockeddata[index].Pixels = Pixeldata;
			Unlockeddata[index].LastUpdate = CurrentUpdate;
		}
	}
}

//==========================================================================
//
// 
//
//==========================================================================

static bool isTranslucent(uint8_t val)
{
	return val == 0;
}

static bool isTranslucent(uint32_t val)
{
	return (val & 0xff000000) == 0;
}

template<class T>
FSoftwareTextureSpan **FSoftwareTexture::CreateSpans (const T *pixels)
{
	FSoftwareTextureSpan **spans, *span;

	if (!mTexture->isMasked())
	{ // Texture does not have holes, so it can use a simpler span structure
		spans = (FSoftwareTextureSpan **)M_Malloc (sizeof(FSoftwareTextureSpan*)*GetPhysicalWidth() + sizeof(FSoftwareTextureSpan)*2);
		span = (FSoftwareTextureSpan *)&spans[GetPhysicalWidth()];
		for (int x = 0; x < GetPhysicalWidth(); ++x)
		{
			spans[x] = span;
		}
		span[0].Length = GetPhysicalHeight();
		span[0].TopOffset = 0;
		span[1].Length = 0;
		span[1].TopOffset = 0;
	}
	else
	{ // Texture might have holes, so build a complete span structure
		int numcols = GetPhysicalWidth();
		int numrows = GetPhysicalHeight();
		int numspans = numcols;	// One span to terminate each column
		const T *data_p;
		bool newspan;
		int x, y;

		data_p = pixels;

		// Count the number of spans in this texture
		for (x = numcols; x > 0; --x)
		{
			newspan = true;
			for (y = numrows; y > 0; --y)
			{

				if (isTranslucent(*data_p++))
				{
					if (!newspan)
					{
						newspan = true;
					}
				}
				else if (newspan)
				{
					newspan = false;
					numspans++;
				}
			}
		}

		// Allocate space for the spans
		spans = (FSoftwareTextureSpan **)M_Malloc (sizeof(FSoftwareTextureSpan*)*numcols + sizeof(FSoftwareTextureSpan)*numspans);

		// Fill in the spans
		for (x = 0, span = (FSoftwareTextureSpan *)&spans[numcols], data_p = pixels; x < numcols; ++x)
		{
			newspan = true;
			spans[x] = span;
			for (y = 0; y < numrows; ++y)
			{
				if (isTranslucent(*data_p++))
				{
					if (!newspan)
					{
						newspan = true;
						span++;
					}
				}
				else
				{
					if (newspan)
					{
						newspan = false;
						span->TopOffset = y;
						span->Length = 1;
					}
					else
					{
						span->Length++;
					}
				}
			}
			if (!newspan)
			{
				span++;
			}
			span->TopOffset = 0;
			span->Length = 0;
			span++;
		}
	}
	return spans;
}

void FSoftwareTexture::FreeSpans (FSoftwareTextureSpan **spans)
{
	M_Free (spans);
}

//==========================================================================
//
// 
//
//==========================================================================

void FSoftwareTexture::GenerateBgraFromBitmap(const FBitmap &bitmap)
{
	CreatePixelsBgraWithMipmaps();

	// Transpose
	const uint32_t *src = (const uint32_t *)bitmap.GetPixels();
	uint32_t *dest = PixelsBgra.Data();
	for (int x = 0; x < GetPhysicalWidth(); x++)
	{
		for (int y = 0; y < GetPhysicalHeight(); y++)
		{
			dest[y + x * GetPhysicalHeight()] = src[x + y * GetPhysicalWidth()];
		}
	}

	GenerateBgraMipmaps();
}

void FSoftwareTexture::CreatePixelsBgraWithMipmaps()
{
	int levels = MipmapLevels();
	int buffersize = 0;
	for (int i = 0; i < levels; i++)
	{
		int w = max(GetPhysicalWidth() >> i, 1);
		int h = max(GetPhysicalHeight() >> i, 1);
		buffersize += w * h;
	}
	PixelsBgra.Resize(buffersize);
}

int FSoftwareTexture::MipmapLevels()
{
	int widthbits = 0;
	while ((GetPhysicalWidth() >> widthbits) != 0) widthbits++;

	int heightbits = 0;
	while ((GetPhysicalHeight() >> heightbits) != 0) heightbits++;

	return max(widthbits, heightbits);
}

//==========================================================================
//
// 
//
//==========================================================================

void FSoftwareTexture::GenerateBgraMipmaps()
{
	struct Color4f
	{
		float a, r, g, b;
		Color4f operator*(const Color4f &v) const { return Color4f{ a * v.a, r * v.r, g * v.g, b * v.b }; }
		Color4f operator/(const Color4f &v) const { return Color4f{ a / v.a, r / v.r, g / v.g, b / v.b }; }
		Color4f operator+(const Color4f &v) const { return Color4f{ a + v.a, r + v.r, g + v.g, b + v.b }; }
		Color4f operator-(const Color4f &v) const { return Color4f{ a - v.a, r - v.r, g - v.g, b - v.b }; }
		Color4f operator*(float s) const { return Color4f{ a * s, r * s, g * s, b * s }; }
		Color4f operator/(float s) const { return Color4f{ a / s, r / s, g / s, b / s }; }
		Color4f operator+(float s) const { return Color4f{ a + s, r + s, g + s, b + s }; }
		Color4f operator-(float s) const { return Color4f{ a - s, r - s, g - s, b - s }; }
	};

	int levels = MipmapLevels();
	std::vector<Color4f> image(PixelsBgra.Size());

	// Convert to normalized linear colorspace
	{
		for (int x = 0; x < GetPhysicalWidth(); x++)
		{
			for (int y = 0; y < GetPhysicalHeight(); y++)
			{
				uint32_t c8 = PixelsBgra[x * GetPhysicalHeight() + y];
				Color4f c;
				c.a = powf(APART(c8) * (1.0f / 255.0f), 2.2f);
				c.r = powf(RPART(c8) * (1.0f / 255.0f), 2.2f);
				c.g = powf(GPART(c8) * (1.0f / 255.0f), 2.2f);
				c.b = powf(BPART(c8) * (1.0f / 255.0f), 2.2f);
				image[x * GetPhysicalHeight() + y] = c;
			}
		}
	}

	// Generate mipmaps
	{
		std::vector<Color4f> smoothed(GetPhysicalWidth() * GetPhysicalHeight());
		Color4f *src = image.data();
		Color4f *dest = src + GetPhysicalWidth() * GetPhysicalHeight();
		for (int i = 1; i < levels; i++)
		{
			int srcw = max(GetPhysicalWidth() >> (i - 1), 1);
			int srch = max(GetPhysicalHeight() >> (i - 1), 1);
			int w = max(GetPhysicalWidth() >> i, 1);
			int h = max(GetPhysicalHeight() >> i, 1);

			// Downscale
			for (int x = 0; x < w; x++)
			{
				int sx0 = x * 2;
				int sx1 = min((x + 1) * 2, srcw - 1);
				for (int y = 0; y < h; y++)
				{
					int sy0 = y * 2;
					int sy1 = min((y + 1) * 2, srch - 1);

					Color4f src00 = src[sy0 + sx0 * srch];
					Color4f src01 = src[sy1 + sx0 * srch];
					Color4f src10 = src[sy0 + sx1 * srch];
					Color4f src11 = src[sy1 + sx1 * srch];
					Color4f c = (src00 + src01 + src10 + src11) * 0.25f;

					dest[y + x * h] = c;
				}
			}

			// Sharpen filter with a 3x3 kernel:
			for (int x = 0; x < w; x++)
			{
				for (int y = 0; y < h; y++)
				{
					Color4f c = { 0.0f, 0.0f, 0.0f, 0.0f };
					for (int kx = -1; kx < 2; kx++)
					{
						for (int ky = -1; ky < 2; ky++)
						{
							int a = y + ky;
							int b = x + kx;
							if (a < 0) a = h - 1;
							if (a == h) a = 0;
							if (b < 0) b = w - 1;
							if (b == w) b = 0;
							c = c + dest[a + b * h];
						}
					}
					c = c * (1.0f / 9.0f);
					smoothed[y + x * h] = c;
				}
			}
			float k = 0.08f;
			for (int j = 0; j < w * h; j++)
				dest[j] = dest[j] + (dest[j] - smoothed[j]) * k;

			src = dest;
			dest += w * h;
		}
	}

	// Convert to bgra8 sRGB colorspace
	{
		Color4f *src = image.data() + GetPhysicalWidth() * GetPhysicalHeight();
		uint32_t *dest = PixelsBgra.Data() + GetPhysicalWidth() * GetPhysicalHeight();
		for (int i = 1; i < levels; i++)
		{
			int w = max(GetPhysicalWidth() >> i, 1);
			int h = max(GetPhysicalHeight() >> i, 1);
			for (int j = 0; j < w * h; j++)
			{
				uint32_t a = (uint32_t)clamp(powf(max(src[j].a, 0.0f), 1.0f / 2.2f) * 255.0f + 0.5f, 0.0f, 255.0f);
				uint32_t r = (uint32_t)clamp(powf(max(src[j].r, 0.0f), 1.0f / 2.2f) * 255.0f + 0.5f, 0.0f, 255.0f);
				uint32_t g = (uint32_t)clamp(powf(max(src[j].g, 0.0f), 1.0f / 2.2f) * 255.0f + 0.5f, 0.0f, 255.0f);
				uint32_t b = (uint32_t)clamp(powf(max(src[j].b, 0.0f), 1.0f / 2.2f) * 255.0f + 0.5f, 0.0f, 255.0f);
				dest[j] = (a << 24) | (r << 16) | (g << 8) | b;
			}
			src += w * h;
			dest += w * h;
		}
	}
}

//==========================================================================
//
//
//
//==========================================================================

void FSoftwareTexture::FreeAllSpans()
{
	for(int i = 0; i < 3; i++)
	{
		if (Spandata[i] != nullptr)
		{
			FreeSpans (Spandata[i]);
			Spandata[i] = nullptr;
		}
	}
}

// Note: this function needs to be thread safe
FSoftwareTexture* GetSoftwareTexture(FGameTexture* tex)
{
	FSoftwareTexture* SoftwareTexture = static_cast<FSoftwareTexture*>(tex->GetSoftwareTexture());
	if (!SoftwareTexture)
	{
		static std::mutex loadmutex;
		std::unique_lock<std::mutex> lock(loadmutex);

		SoftwareTexture = static_cast<FSoftwareTexture*>(tex->GetSoftwareTexture());
		if (!SoftwareTexture)
		{
			if (tex->isSoftwareCanvas()) SoftwareTexture = new FSWCanvasTexture(tex);
			else if (tex->isWarped()) SoftwareTexture = new FWarpTexture(tex, tex->isWarped());
			else SoftwareTexture = new FSoftwareTexture(tex);
			tex->SetSoftwareTexture(SoftwareTexture);
		}
	}
	return SoftwareTexture;
}


CUSTOM_CVAR(Bool, vid_nopalsubstitutions, false, CVAR_ARCHIVE | CVAR_NOINITCALL)
{
	// This is in case the sky texture has been substituted.
	R_InitSkyMap();
}

// Note: this function needs to be thread safe
FSoftwareTexture* GetPalettedSWTexture(FTextureID texid, bool animate, bool checkcompat, bool allownull)
{
	bool needpal = !vid_nopalsubstitutions && !V_IsTrueColor();
	auto tex = TexMan.GetPalettedTexture(texid, true, needpal);
	if (tex == nullptr || (!allownull && !tex->isValid())) return nullptr;
	if (checkcompat)
	{
		auto rawtexid = TexMan.GetRawTexture(tex->GetID());
		auto rawtex = TexMan.GetGameTexture(rawtexid);
		if (rawtex) tex = rawtex;
	}
	return GetSoftwareTexture(tex);
}

