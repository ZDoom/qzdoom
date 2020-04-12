#pragma once

#include <stdint.h>
#include "tarray.h"
#include "textureid.h"
#include "basics.h"
#include "texmanip.h"
#include "name.h"

class FxAddSub;
class FTexture;
struct BuildInfo;
int PalCheck(int tex);

// Texture manager
class FTextureManager
{
	void (*progressFunc)();
	friend class FxAddSub;	// needs access to do a bounds check on the texture ID.
public:
	FTextureManager ();
	~FTextureManager ();
	
private:
	int ResolveLocalizedTexture(int texnum);

	FTexture *InternalGetTexture(int texnum, bool animate, bool localize, bool palettesubst)
	{
		if ((unsigned)texnum >= Textures.Size()) return nullptr;
		if (animate) texnum = Translation[texnum];
		if (localize && Textures[texnum].HasLocalization) texnum = ResolveLocalizedTexture(texnum);
		if (palettesubst) texnum = PalCheck(texnum);
		return Textures[texnum].Texture;
	}
public:
	// This only gets used in UI code so we do not need PALVERS handling.
	FTexture *GetTextureByName(const char *name, bool animate = false)
	{
		FTextureID texnum = GetTextureID (name, ETextureType::MiscPatch);
		return InternalGetTexture(texnum.GetIndex(), animate, true, false);
	}
	
	FTexture *GetTexture(FTextureID texnum, bool animate = false)
	{
		return InternalGetTexture(texnum.GetIndex(), animate, true, false);
	}
	
	// This is the only access function that should be used inside the software renderer.
	FTexture *GetPalettedTexture(FTextureID texnum, bool animate)
	{
		return InternalGetTexture(texnum.GetIndex(), animate, true, true);
	}
	
	FTexture *ByIndex(int i, bool animate = false)
	{
		return InternalGetTexture(i, animate, true, false);
	}
	
	FTexture *FindTexture(const char *texname, ETextureType usetype = ETextureType::MiscPatch, BITFIELD flags = TEXMAN_TryAny);
	bool OkForLocalization(FTextureID texnum, const char *substitute, int locnum);

	void FlushAll();


	enum
	{
		TEXMAN_TryAny = 1,
		TEXMAN_Overridable = 2,
		TEXMAN_ReturnFirst = 4,
		TEXMAN_AllowSkins = 8,
		TEXMAN_ShortNameOnly = 16,
		TEXMAN_DontCreate = 32,
		TEXMAN_Localize = 64
	};

	enum
	{
		HIT_Wall = 1,
		HIT_Flat = 2,
		HIT_Sky = 4,
		HIT_Sprite = 8,

		HIT_Columnmode = HIT_Wall|HIT_Sky|HIT_Sprite
	};

	FTextureID CheckForTexture (const char *name, ETextureType usetype, BITFIELD flags=TEXMAN_TryAny);
	FTextureID GetTextureID (const char *name, ETextureType usetype, BITFIELD flags=0);
	int ListTextures (const char *name, TArray<FTextureID> &list, bool listall = false);

	void AddGroup(int wadnum, int ns, ETextureType usetype);
	void AddPatches (int lumpnum);
	void AddHiresTextures (int wadnum);
	void LoadTextureDefs(int wadnum, const char *lumpname, FMultipatchTextureBuilder &build);
	void ParseColorization(FScanner& sc);
	void ParseTextureDef(int remapLump, FMultipatchTextureBuilder &build);
	void SortTexturesByType(int start, int end);
	bool AreTexturesCompatible (FTextureID picnum1, FTextureID picnum2);
	void AddLocalizedVariants();

	FTextureID CreateTexture (int lumpnum, ETextureType usetype=ETextureType::Any);	// Also calls AddTexture
	FTextureID AddTexture (FTexture *texture);
	FTextureID GetDefaultTexture() const { return DefaultTexture; }

	void LoadTextureX(int wadnum, FMultipatchTextureBuilder &build);
	void AddTexturesForWad(int wadnum, FMultipatchTextureBuilder &build);
	void Init(void (*progressFunc_)(), void (*checkForHacks)(BuildInfo &));
	void DeleteAll();

	void ReplaceTexture (FTextureID picnum, FTexture *newtexture, bool free);

	int NumTextures () const { return (int)Textures.Size(); }

	int GuesstimateNumTextures ();

	TextureManipulation* GetTextureManipulation(FName name)
	{
		return tmanips.CheckKey(name);
	}
	void InsertTextureManipulation(FName cname, TextureManipulation tm)
	{
		tmanips.Insert(cname, tm);
	}
	void RemoveTextureManipulation(FName cname)
	{
		tmanips.Remove(cname);
	}

private:

	// texture counting
	int CountTexturesX ();
	int CountLumpTextures (int lumpnum);
	void AdjustSpriteOffsets();

	// Build tiles
	//int CountBuildTiles ();

public:

	TArray<uint8_t>& GetNewBuildTileData()
	{
		BuildTileData.Reserve(1);
		return BuildTileData.Last();
	}

	FTexture* Texture(FTextureID id) { return Textures[id.GetIndex()].Texture; }
	void SetTranslation(FTextureID fromtexnum, FTextureID totexnum);

private:

	void InitPalettedVersions();
	
	// Switches

	struct TextureHash
	{
		FTexture *Texture;
		int HashNext;
		bool HasLocalization;
	};
	enum { HASH_END = -1, HASH_SIZE = 1027 };
	TArray<TextureHash> Textures;
	TMap<uint64_t, int> LocalizedTextures;
	int HashFirst[HASH_SIZE];
	FTextureID DefaultTexture;
	TArray<int> FirstTextureForFile;
	TArray<TArray<uint8_t> > BuildTileData;
	TArray<int> Translation;

	TMap<FName, TextureManipulation> tmanips;

public:

	short sintable[2048];	// for texture warping
	enum
	{
		SINMASK = 2047
	};

	FTextureID glPart2;
	FTextureID glPart;
	FTextureID mirrorTexture;

};

extern FTextureManager TexMan;
