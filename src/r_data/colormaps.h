#ifndef __RES_CMAP_H
#define __RES_CMAP_H

#include "doomtype.h"

struct lightlist_t;

void R_InitColormaps (bool allowCustomColormap = false);
void R_DeinitColormaps ();

void R_UpdateInvulnerabilityColormap ();

uint32_t R_ColormapNumForName(const char *name);	// killough 4/4/98
void R_SetDefaultColormap (const char *name);	// [RH] change normal fadetable
uint32_t R_BlendForColormap (uint32_t map);			// [RH] return calculated blend for a colormap

struct FakeCmap 
{
	char name[8];
	PalEntry blend;
	int lump;
};

extern TArray<FakeCmap> fakecmaps;

#include "fcolormap.h"

// For hardware-accelerated weapon sprites in colored sectors
struct FColormapStyle
{
	PalEntry Color;
	PalEntry Fade;
	int Desaturate;
	float FadeLevel;
};

// some utility functions to store special colormaps in powerup blends
#define SPECIALCOLORMAP_MASK 0x00b60000

inline uint32_t MakeSpecialColormap(int index)
{
	assert(index >= 0 && index < 65536);
	return index | SPECIALCOLORMAP_MASK;
}

enum EColorManipulation
{
	CM_PLAIN2D = -2,			// regular 2D drawing.
	CM_INVALID = -1,
	CM_DEFAULT = 0,					// untranslated
	CM_FIRSTSPECIALCOLORMAP,		// first special fixed colormap
	CM_FIRSTSPECIALCOLORMAPFORCED = 0x08000000,	// first special fixed colormap, application forced (for 2D overlays)
};

#define CM_MAXCOLORMAP int(CM_FIRSTSPECIALCOLORMAP + SpecialColormaps.Size())
#define CM_MAXCOLORMAPFORCED int(CM_FIRSTSPECIALCOLORMAPFORCED + SpecialColormaps.Size())


#endif
