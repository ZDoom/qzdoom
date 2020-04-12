/*
** automaptexture.cpp
** Texture class for Raven's automap parchment
**
**---------------------------------------------------------------------------
** Copyright 2004-2006 Randy Heit
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
** This texture type is only used as a last resort when everything else has failed for creating 
** the AUTOPAGE texture. That's because Raven used a raw lump of non-standard proportions to define it.
**
*/

#include "files.h"
#include "filesystem.h"
#include "imagehelpers.h"
#include "image.h"

//==========================================================================
//
// A raw 320x? graphic used by Heretic and Hexen for the automap parchment
//
//==========================================================================

class FAutomapTexture : public FImageSource
{
public:
	FAutomapTexture(int lumpnum);
	TArray<uint8_t> CreatePalettedPixels(int conversion) override;
};



//==========================================================================
//
// This texture type will only be used for the AUTOPAGE lump if no other
// format matches.
//
//==========================================================================

FImageSource *AutomapImage_TryCreate(FileReader &data, int lumpnum)
{
	if (data.GetLength() < 320) return nullptr;
	if (!fileSystem.CheckFileName(lumpnum, "AUTOPAGE")) return nullptr;
	return new FAutomapTexture(lumpnum);
}

//==========================================================================
//
//
//
//==========================================================================

FAutomapTexture::FAutomapTexture (int lumpnum)
: FImageSource(lumpnum)
{
	Width = 320;
	Height = uint16_t(fileSystem.FileLength(lumpnum) / 320);
	bUseGamePalette = true;
}

//==========================================================================
//
//
//
//==========================================================================

TArray<uint8_t> FAutomapTexture::CreatePalettedPixels(int conversion)
{
	int x, y;
	FileData data = fileSystem.ReadFile (SourceLump);
	const uint8_t *indata = (const uint8_t *)data.GetMem();

	TArray<uint8_t> Pixels(Width * Height, true);

	const uint8_t *remap = ImageHelpers::GetRemap(conversion == luminance);
	for (x = 0; x < Width; ++x)
	{
		for (y = 0; y < Height; ++y)
		{
			auto p = indata[x + 320 * y];
			Pixels[x*Height + y] = remap[p];
		}
	}
	return Pixels;
}

