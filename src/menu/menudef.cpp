/*
** menudef.cpp
** MENUDEF parser amd menu generation code
**
**---------------------------------------------------------------------------
** Copyright 2010 Christoph Oelckers
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
*/
#include <float.h>

#include "menu/menu.h"
#include "filesystem.h"
#include "c_bind.h"
#include "i_music.h"
#include "gi.h"
#include "i_sound.h"
#include "cmdlib.h"
#include "vm.h"
#include "types.h"
#include "gameconfigfile.h"
#include "m_argv.h"
#include "i_soundfont.h"
#include "i_system.h"
#include "v_video.h"
#include "gstrings.h"
#include "teaminfo.h"
#include "r_data/sprites.h"
#include <zmusic.h>
#include "texturemanager.h"


void ClearSaveGames();

MenuDescriptorList MenuDescriptors;
static DListMenuDescriptor *DefaultListMenuSettings;	// contains common settings for all list menus
static DOptionMenuDescriptor *DefaultOptionMenuSettings;	// contains common settings for all Option menus
FOptionMenuSettings OptionSettings;
FOptionMap OptionValues;
bool mustPrintErrors;
PClass *DefaultListMenuClass;
PClass *DefaultOptionMenuClass;

void I_BuildALDeviceList(FOptionValues *opt);
void I_BuildALResamplersList(FOptionValues *opt);


//==========================================================================
//
// Defines how graphics substitution is handled.
// 0: Never replace a text-containing graphic with a font-based text.
// 1: Always replace, regardless of any missing information. Useful for testing the substitution without providing full data.
// 2: Only replace for non-default texts, i.e. if some language redefines the string's content, use it instead of the graphic. Never replace a localized graphic.
// 3: Only replace if the string is not the default and the graphic comes from the IWAD. Never replace a localized graphic.
// 4: Like 1, but lets localized graphics pass.
//
// The default is 3, which only replaces known content with non-default texts.
//
//==========================================================================

CUSTOM_CVAR(Int, cl_gfxlocalization, 3, CVAR_ARCHIVE)
{
	if (self < 0 || self > 4) self = 0;
}

bool OkForLocalization(FTextureID texnum, const char* substitute)
{
	if (!texnum.isValid()) return false;

	// First the unconditional settings, 0='never' and 1='always'.
	if (cl_gfxlocalization == 1 || gameinfo.forcetextinmenus) return false;
	if (cl_gfxlocalization == 0 || gameinfo.forcenogfxsubstitution) return true;
	return TexMan.OkForLocalization(texnum, substitute, cl_gfxlocalization);
}



DEFINE_GLOBAL_NAMED(OptionSettings, OptionMenuSettings)

DEFINE_ACTION_FUNCTION(FOptionValues, GetCount)
{
	PARAM_PROLOGUE;
	PARAM_NAME(grp);
	int cnt = 0;
	FOptionValues **pGrp = OptionValues.CheckKey(grp);
	if (pGrp != nullptr)
	{
		cnt = (*pGrp)->mValues.Size();
	}
	ACTION_RETURN_INT(cnt);
}

DEFINE_ACTION_FUNCTION(FOptionValues, GetValue)
{
	PARAM_PROLOGUE;
	PARAM_NAME(grp);
	PARAM_UINT(index);
	double val = 0;
	FOptionValues **pGrp = OptionValues.CheckKey(grp);
	if (pGrp != nullptr)
	{
		if (index < (*pGrp)->mValues.Size())
		{
			val = (*pGrp)->mValues[index].Value;
		}
	}
	ACTION_RETURN_FLOAT(val);
}

DEFINE_ACTION_FUNCTION(FOptionValues, GetTextValue)
{
	PARAM_PROLOGUE;
	PARAM_NAME(grp);
	PARAM_UINT(index);
	FString val;
	FOptionValues **pGrp = OptionValues.CheckKey(grp);
	if (pGrp != nullptr)
	{
		if (index < (*pGrp)->mValues.Size())
		{
			val = (*pGrp)->mValues[index].TextValue;
		}
	}
	ACTION_RETURN_STRING(val);
}

DEFINE_ACTION_FUNCTION(FOptionValues, GetText)
{
	PARAM_PROLOGUE;
	PARAM_NAME(grp);
	PARAM_UINT(index);
	FString val;
	FOptionValues **pGrp = OptionValues.CheckKey(grp);
	if (pGrp != nullptr)
	{
		if (index < (*pGrp)->mValues.Size())
		{
			val = (*pGrp)->mValues[index].Text;
		}
	}
	ACTION_RETURN_STRING(val);
}


void DeinitMenus()
{
	M_ClearMenus();
	{
		FOptionMap::Iterator it(OptionValues);

		FOptionMap::Pair *pair;

		while (it.NextPair(pair))
		{
			delete pair->Value;
			pair->Value = nullptr;
		}
	}
	MenuDescriptors.Clear();
	OptionValues.Clear();
	savegameManager.ClearSaveGames();
}

static FTextureID GetMenuTexture(const char* const name)
{
	const FTextureID texture = TexMan.CheckForTexture(name, ETextureType::MiscPatch);

	if (!texture.Exists() && mustPrintErrors)
	{
		Printf("Missing menu texture: \"%s\"\n", name);
	}

	return texture;
}

//=============================================================================
//
//
//
//=============================================================================

static void SkipSubBlock(FScanner &sc)
{
	sc.MustGetStringName("{");
	int depth = 1;
	while (depth > 0)
	{
		sc.MustGetString();
		if (sc.Compare("{")) depth++;
		if (sc.Compare("}")) depth--;
	}
}

//=============================================================================
//
//
//
//=============================================================================

static bool CheckSkipGameBlock(FScanner &sc)
{
	bool filter = false;
	sc.MustGetStringName("(");
	do
	{
		sc.MustGetString();
		filter |= CheckGame(sc.String, false);
	}
	while (sc.CheckString(","));
	sc.MustGetStringName(")");
	if (!filter)
	{
		SkipSubBlock(sc);
		return !sc.CheckString("else");
	}
	return false;
}

//=============================================================================
//
//
//
//=============================================================================

static bool CheckSkipOptionBlock(FScanner &sc)
{
	bool filter = false;
	sc.MustGetStringName("(");
	do
	{
		sc.MustGetString();
		if (sc.Compare("ReadThis")) filter |= gameinfo.drawreadthis;
		else if (sc.Compare("Swapmenu")) filter |= gameinfo.swapmenu;
		else if (sc.Compare("Windows"))
		{
			#ifdef _WIN32
				filter = true;
			#endif
		}
		else if (sc.Compare("unix"))
		{
			#ifdef __unix__
				filter = true;
			#endif
		}
		else if (sc.Compare("Mac"))
		{
			#ifdef __APPLE__
				filter = true;
			#endif
		}
		else if (sc.Compare("OpenAL"))
		{
			filter |= IsOpenALPresent();
		}
		else if (sc.Compare("MMX"))
		{
			#ifdef HAVE_MMX
				filter = true;
			#endif
		}
	}
	while (sc.CheckString(","));
	sc.MustGetStringName(")");
	if (!filter)
	{
		SkipSubBlock(sc);
		return !sc.CheckString("else");
	}
	return false;
}

//=============================================================================
//
//
//
//=============================================================================

static void ParseListMenuBody(FScanner &sc, DListMenuDescriptor *desc)
{
	sc.MustGetStringName("{");
	while (!sc.CheckString("}"))
	{
		sc.MustGetString();
		if (sc.Compare("else"))
		{
			SkipSubBlock(sc);
		}
		else if (sc.Compare("ifgame"))
		{
			if (!CheckSkipGameBlock(sc))
			{
				// recursively parse sub-block
				ParseListMenuBody(sc, desc);
			}
		}
		else if (sc.Compare("ifoption"))
		{
			if (!CheckSkipOptionBlock(sc))
			{
				// recursively parse sub-block
				ParseListMenuBody(sc, desc);
			}
		}
		else if (sc.Compare("Class"))
		{
			sc.MustGetString();
			PClass *cls = PClass::FindClass(sc.String);
			if (cls == nullptr || !cls->IsDescendantOf("ListMenu"))
			{
				sc.ScriptError("Unknown menu class '%s'", sc.String);
			}
			desc->mClass = cls;
		}
		else if (sc.Compare("Selector"))
		{
			sc.MustGetString();
			desc->mSelector = GetMenuTexture(sc.String);
			sc.MustGetStringName(",");
			sc.MustGetFloat();
			desc->mSelectOfsX = sc.Float;
			sc.MustGetStringName(",");
			sc.MustGetFloat();
			desc->mSelectOfsY = sc.Float;
		}
		else if (sc.Compare("Linespacing"))
		{
			sc.MustGetNumber();
			desc->mLinespacing = sc.Number;
		}
		else if (sc.Compare("Position"))
		{
			sc.MustGetFloat();
			desc->mXpos = sc.Float;
			sc.MustGetStringName(",");
			sc.MustGetFloat();
			desc->mYpos = sc.Float;
		}
		else if (sc.Compare("Centermenu"))
		{
			desc->mCenter = true;
		}
		else if (sc.Compare("MouseWindow"))
		{
			sc.MustGetNumber();
			desc->mWLeft = sc.Number;
			sc.MustGetStringName(",");
			sc.MustGetNumber();
			desc->mWRight = sc.Number;
		}
		else if (sc.Compare("Font"))
		{
			sc.MustGetString();
			FFont *newfont = V_GetFont(sc.String);
			if (newfont != nullptr) desc->mFont = newfont;
			if (sc.CheckString(","))
			{
				sc.MustGetString();
				desc->mFontColor2 = desc->mFontColor = V_FindFontColor((FName)sc.String);
				if (sc.CheckString(","))
				{
					sc.MustGetString();
					desc->mFontColor2 = V_FindFontColor((FName)sc.String);
				}
			}
			else
			{
				desc->mFontColor = OptionSettings.mFontColor;
				desc->mFontColor2 = OptionSettings.mFontColorValue;
			}
		}
		else if (sc.Compare("NetgameMessage"))
		{
			sc.MustGetString();
			desc->mNetgameMessage = sc.String;
		}
		else
		{
			bool success = false;
			FStringf buildname("ListMenuItem%s", sc.String);
			PClass *cls = PClass::FindClass(buildname);
			if (cls != nullptr && cls->IsDescendantOf("ListMenuItem"))
			{
				auto func = dyn_cast<PFunction>(cls->FindSymbol("Init", true));
				if (func != nullptr && !(func->Variants[0].Flags & (VARF_Protected | VARF_Private)))	// skip internal classes which have a protected init method.
				{
					auto &args = func->Variants[0].Proto->ArgumentTypes;
					TArray<VMValue> params;
					int start = 1;

					params.Push(0);
					if (args.Size() > 1 && args[1] == NewPointer(PClass::FindClass("ListMenuDescriptor")))
					{
						params.Push(desc);
						start = 2;
					}
					auto TypeCVar = NewPointer(NewStruct("CVar", nullptr, true));

					// Note that this array may not be reallocated so its initial size must be the maximum possible elements.
					TArray<FString> strings(args.Size());
					for (unsigned i = start; i < args.Size(); i++)
					{
						sc.MustGetString();
						if (args[i] == TypeString)
						{
							strings.Push(sc.String);
							params.Push(&strings.Last());
						}
						else if (args[i] == TypeName)
						{
							params.Push(FName(sc.String).GetIndex());
						}
						else if (args[i] == TypeColor)
						{
							params.Push(V_GetColor(nullptr, sc));
						}
						else if (args[i] == TypeFont)
						{
							auto f = V_GetFont(sc.String);
							if (f == nullptr)
							{
								sc.ScriptError("Unknown font %s", sc.String);
							}
							params.Push(f);
						}
						else if (args[i] == TypeTextureID)
						{
							auto f = TexMan.CheckForTexture(sc.String, ETextureType::MiscPatch);
							if (!f.Exists())
							{
								sc.ScriptMessage("Unknown texture %s", sc.String);
							}
							params.Push(f.GetIndex());
						}
						else if (args[i]->isIntCompatible())
						{
							char *endp;
							int v = (int)strtoll(sc.String, &endp, 0);
							if (*endp != 0)
							{
								// special check for font color ranges.
								v = V_FindFontColor(sc.String);
								if (v == CR_UNTRANSLATED && !sc.Compare("untranslated"))
								{
									// todo: check other data types that may get used.
									sc.ScriptError("Integer expected, got %s", sc.String);
								}
							}
							if (args[i] == TypeBool) v = !!v;
							params.Push(v);
						}
						else if (args[i]->isFloat())
						{
							char *endp;
							double v = strtod(sc.String, &endp);
							if (*endp != 0)
							{
								sc.ScriptError("Float expected, got %s", sc.String);
							}
							params.Push(v);
						}
						else if (args[i] == TypeCVar)
						{
							auto cv = FindCVar(sc.String, nullptr);
							if (cv == nullptr && *sc.String)
							{
								sc.ScriptError("Unknown CVar %s", sc.String);
							}
							params.Push(cv);
						}
						else
						{
							sc.ScriptError("Invalid parameter type %s for menu item", args[i]->DescriptiveName());
						}
						if (sc.CheckString(","))
						{
							if (i == args.Size() - 1)
							{
								sc.ScriptError("Too many parameters for %s", cls->TypeName.GetChars());
							}
						}
						else
						{
							if (i < args.Size() - 1 && !(func->Variants[0].ArgFlags[i + 1] & VARF_Optional))
							{
								sc.ScriptError("Insufficient parameters for %s", cls->TypeName.GetChars());
							}
							break;
						}
					}
					DMenuItemBase *item = (DMenuItemBase*)cls->CreateNew();
					params[0] = item;
					VMCallWithDefaults(func->Variants[0].Implementation, params, nullptr, 0);
					desc->mItems.Push((DMenuItemBase*)item);

					if (cls->IsDescendantOf("ListMenuItemSelectable"))
					{
						desc->mYpos += desc->mLinespacing;
						if (desc->mSelectedItem == -1) desc->mSelectedItem = desc->mItems.Size() - 1;
					}
					success = true;
				}
			}
			if (!success)
			{
				sc.ScriptError("Unknown keyword '%s'", sc.String);
			}
		}
	}
	for (auto &p : desc->mItems)
	{
		GC::WriteBarrier(p);
	}
}

//=============================================================================
//
//
//
//=============================================================================

static bool CheckCompatible(DMenuDescriptor *newd, DMenuDescriptor *oldd)
{
	if (oldd->mClass == nullptr) return true;
	return newd->mClass->IsDescendantOf(oldd->mClass);
}

static int GetGroup(DMenuItemBase *desc)
{
	if (desc->IsKindOf(NAME_OptionMenuItemCommand)) return 2;
	if (desc->IsKindOf(NAME_OptionMenuItemSubmenu)) return 1;
	if (desc->IsKindOf(NAME_OptionMenuItemControlBase)) return 3;
	if (desc->IsKindOf(NAME_OptionMenuItemOptionBase)) return 4;
	if (desc->IsKindOf(NAME_OptionMenuSliderBase)) return 4;
	if (desc->IsKindOf(NAME_OptionMenuFieldBase)) return 4;
	if (desc->IsKindOf(NAME_OptionMenuItemColorPicker)) return 4;
	if (desc->IsKindOf(NAME_OptionMenuItemStaticText)) return 5;
	if (desc->IsKindOf(NAME_OptionMenuItemStaticTextSwitchable)) return 5;
	return 0;
}

static bool FindMatchingItem(DMenuItemBase *desc)
{
	int grp = GetGroup(desc);
	if (grp == 0) return false;	// no idea what this is.
	if (grp == 5) return true;	// static texts always match

	FName name = desc->mAction;

	if (grp == 1)
	{
		// Check for presence of menu
		auto menu = MenuDescriptors.CheckKey(name);
		if (menu == nullptr) return false;
	}
	else if (grp == 4)
	{
		static const FName CVarBlacklist[] = {
			NAME_snd_waterlp, NAME_snd_output, NAME_snd_output_format, NAME_snd_speakermode, NAME_snd_resampler, NAME_AlwaysRun };

		// Check for presence of CVAR and blacklist
		auto cv = FindCVar(name.GetChars(), nullptr);
		if (cv == nullptr) return true;

		for (auto bname : CVarBlacklist)
		{
			if (name == bname) return true;
		}
	}

	MenuDescriptorList::Iterator it(MenuDescriptors);
	MenuDescriptorList::Pair *pair;
	while (it.NextPair(pair))
	{
		for (auto it : pair->Value->mItems)
		{
			if (it->mAction == name && GetGroup(it) == grp) return true;
		}
	}
	return false;
}

static bool ReplaceMenu(FScanner &sc, DMenuDescriptor *desc)
{
	DMenuDescriptor **pOld = MenuDescriptors.CheckKey(desc->mMenuName);
	if (pOld != nullptr && *pOld != nullptr) 
	{
		if ((*pOld)->mProtected)
		{
			// If this tries to replace an option menu with an option menu, let's append all new entries to the old menu.
			// Otherwise bail out because for list menus it's not that simple.
			if (desc->IsKindOf(RUNTIME_CLASS(DListMenuDescriptor)) || (*pOld)->IsKindOf(RUNTIME_CLASS(DListMenuDescriptor)))
			{
				sc.ScriptMessage("Cannot replace protected menu %s.", desc->mMenuName.GetChars());
				return true;
			}
			for (int i = desc->mItems.Size()-1; i >= 0; i--)
			{
				if (FindMatchingItem(desc->mItems[i]))
				{
					desc->mItems.Delete(i);
				}
			}
			if (desc->mItems.Size() > 0)
			{
				auto sep = CreateOptionMenuItemStaticText(" ");
				(*pOld)->mItems.Push(sep);
				sep = CreateOptionMenuItemStaticText("---------------", 1);
				(*pOld)->mItems.Push(sep);
				for (auto it : desc->mItems)
				{
					(*pOld)->mItems.Push(it);
				}
				desc->mItems.Clear();
				//sc.ScriptMessage("Merged %d items into %s", desc->mItems.Size(), desc->mMenuName.GetChars());
			}
			return true;
		}

		if (!CheckCompatible(desc, *pOld))
		{
			sc.ScriptMessage("Tried to replace menu '%s' with a menu of different type", desc->mMenuName.GetChars());
			return true;
		}
	}
	MenuDescriptors[desc->mMenuName] = desc;
	GC::WriteBarrier(desc);
	return false;
}

//=============================================================================
//
//
//
//=============================================================================

static void ParseListMenu(FScanner &sc)
{
	sc.MustGetString();

	DListMenuDescriptor *desc = Create<DListMenuDescriptor>();
	desc->mMenuName = sc.String;
	desc->mSelectedItem = -1;
	desc->mAutoselect = -1;
	desc->mSelectOfsX = DefaultListMenuSettings->mSelectOfsX;
	desc->mSelectOfsY = DefaultListMenuSettings->mSelectOfsY;
	desc->mSelector = DefaultListMenuSettings->mSelector;
	desc->mDisplayTop = DefaultListMenuSettings->mDisplayTop;
	desc->mXpos = DefaultListMenuSettings->mXpos;
	desc->mYpos = DefaultListMenuSettings->mYpos;
	desc->mLinespacing = DefaultListMenuSettings->mLinespacing;
	desc->mNetgameMessage = DefaultListMenuSettings->mNetgameMessage;
	desc->mFont = DefaultListMenuSettings->mFont;
	desc->mFontColor = DefaultListMenuSettings->mFontColor;
	desc->mFontColor2 = DefaultListMenuSettings->mFontColor2;
	desc->mClass = nullptr;
	desc->mWLeft = 0;
	desc->mWRight = 0;
	desc->mCenter = false;
	desc->mFromEngine = fileSystem.GetFileContainer(sc.LumpNum) == 0;	// flags menu if the definition is from the IWAD.

	ParseListMenuBody(sc, desc);
	ReplaceMenu(sc, desc);
}

//=============================================================================
//
//
//
//=============================================================================

static void ParseOptionValue(FScanner &sc)
{
	FOptionValues *val = new FOptionValues;
	sc.MustGetString();
	FName optname = sc.String;
	sc.MustGetStringName("{");
	while (!sc.CheckString("}"))
	{
		FOptionValues::Pair &pair = val->mValues[val->mValues.Reserve(1)];
		sc.MustGetFloat();
		pair.Value = sc.Float;
		sc.MustGetStringName(",");
		sc.MustGetString();
		pair.Text = strbin1(sc.String);
	}
	FOptionValues **pOld = OptionValues.CheckKey(optname);
	if (pOld != nullptr && *pOld != nullptr) 
	{
		delete *pOld;
	}
	OptionValues[optname] = val;
}


//=============================================================================
//
//
//
//=============================================================================

static void ParseOptionString(FScanner &sc)
{
	FOptionValues *val = new FOptionValues;
	sc.MustGetString();
	FName optname = sc.String;
	sc.MustGetStringName("{");
	while (!sc.CheckString("}"))
	{
		FOptionValues::Pair &pair = val->mValues[val->mValues.Reserve(1)];
		sc.MustGetString();
		pair.Value = DBL_MAX;
		pair.TextValue = sc.String;
		sc.MustGetStringName(",");
		sc.MustGetString();
		pair.Text = strbin1(sc.String);
	}
	FOptionValues **pOld = OptionValues.CheckKey(optname);
	if (pOld != nullptr && *pOld != nullptr) 
	{
		delete *pOld;
	}
	OptionValues[optname] = val;
}


//=============================================================================
//
//
//
//=============================================================================

static void ParseOptionSettings(FScanner &sc)
{
	sc.MustGetStringName("{");
	while (!sc.CheckString("}"))
	{
		sc.MustGetString();
		if (sc.Compare("else"))
		{
			SkipSubBlock(sc);
		}
		else if (sc.Compare("ifgame"))
		{
			if (!CheckSkipGameBlock(sc))
			{
				// recursively parse sub-block
				ParseOptionSettings(sc);
			}
		}
		else if (sc.Compare("Linespacing"))
		{
			sc.MustGetNumber();
			OptionSettings.mLinespacing = sc.Number;
		}
		else if (sc.Compare("LabelOffset"))
		{
			sc.MustGetNumber();
			// ignored
		}
		else
		{
			sc.ScriptError("Unknown keyword '%s'", sc.String);
		}
	}
}

//=============================================================================
//
//
//
//=============================================================================

static void ParseOptionMenuBody(FScanner &sc, DOptionMenuDescriptor *desc)
{
	sc.MustGetStringName("{");
	while (!sc.CheckString("}"))
	{
		sc.MustGetString();
		if (sc.Compare("else"))
		{
			SkipSubBlock(sc);
		}
		else if (sc.Compare("ifgame"))
		{
			if (!CheckSkipGameBlock(sc))
			{
				// recursively parse sub-block
				ParseOptionMenuBody(sc, desc);
			}
		}
		else if (sc.Compare("ifoption"))
		{
			if (!CheckSkipOptionBlock(sc))
			{
				// recursively parse sub-block
				ParseOptionMenuBody(sc, desc);
			}
		}
		else if (sc.Compare("Class"))
		{
			sc.MustGetString();
			PClass *cls = PClass::FindClass(sc.String);
			if (cls == nullptr || !cls->IsDescendantOf("OptionMenu"))
			{
				sc.ScriptError("Unknown menu class '%s'", sc.String);
			}
			desc->mClass = cls;
		}
		else if (sc.Compare("Title"))
		{
			sc.MustGetString();
			desc->mTitle = sc.String;
		}
		else if (sc.Compare("Position"))
		{
			sc.MustGetNumber();
			desc->mPosition = sc.Number;
		}
		else if (sc.Compare("DefaultSelection"))
		{
			sc.MustGetNumber();
			desc->mSelectedItem = sc.Number;
		}
		else if (sc.Compare("ScrollTop"))
		{
			sc.MustGetNumber();
			desc->mScrollTop = sc.Number;
		}
		else if (sc.Compare("Indent"))
		{
			sc.MustGetNumber();
			desc->mIndent = sc.Number;
		}
		else
		{
			bool success = false;
			FStringf buildname("OptionMenuItem%s", sc.String);
			// Handle one special case: MapControl maps to Control with one parameter different
			PClass *cls = PClass::FindClass(buildname);
			if (cls != nullptr && cls->IsDescendantOf("OptionMenuItem"))
			{
				auto func = dyn_cast<PFunction>(cls->FindSymbol("Init", true));
				if (func != nullptr && !(func->Variants[0].Flags & (VARF_Protected | VARF_Private)))	// skip internal classes which have a protexted init method.
				{
					auto &args = func->Variants[0].Proto->ArgumentTypes;
					TArray<VMValue> params;

					params.Push(0);
					auto TypeCVar = NewPointer(NewStruct("CVar", nullptr, true));

					// Note that this array may not be reallocated so its initial size must be the maximum possible elements.
					TArray<FString> strings(args.Size());
					for (unsigned i = 1; i < args.Size(); i++)
					{
						sc.MustGetString();
						if (args[i] == TypeString)
						{
							strings.Push(sc.String);
							params.Push(&strings.Last());
						}
						else if (args[i] == TypeName)
						{
							params.Push(FName(sc.String).GetIndex());
						}
						else if (args[i] == TypeColor)
						{
							params.Push(V_GetColor(nullptr, sc));
						}
						else if (args[i]->isIntCompatible())
						{
							char *endp;
							int v = (int)strtoll(sc.String, &endp, 0);
							if (*endp != 0)
							{
								// special check for font color ranges.
								v = V_FindFontColor(sc.String);
								if (v == CR_UNTRANSLATED && !sc.Compare("untranslated"))
								{
									// todo: check other data types that may get used.
									sc.ScriptError("Integer expected, got %s", sc.String);
								}
								// Color ranges need to be marked for option menu items to support an older feature where a boolean number could be passed instead.
								v |= 0x12340000;
							}
							if (args[i] == TypeBool) v = !!v;
							params.Push(v);
						}
						else if (args[i]->isFloat())
						{
							char *endp;
							double v = strtod(sc.String, &endp);
							if (*endp != 0)
							{
								sc.ScriptError("Float expected, got %s", sc.String);
							}
							params.Push(v);
						}
						else if (args[i] == TypeCVar)
						{
							auto cv = FindCVar(sc.String, nullptr);
							if (cv == nullptr && *sc.String)
							{
									if (func->Variants[0].ArgFlags[i] & VARF_Optional)
										sc.ScriptMessage("Unknown CVar %s", sc.String);
									else
										sc.ScriptError("Unknown CVar %s", sc.String);
							}
							params.Push(cv);
						}
						else
						{
							sc.ScriptError("Invalid parameter type %s for menu item", args[i]->DescriptiveName());
						}
						if (sc.CheckString(","))
						{
							if (i == args.Size() - 1)
							{
								sc.ScriptError("Too many parameters for %s", cls->TypeName.GetChars());
							}
						}
						else
						{
							if (i < args.Size() - 1 && !(func->Variants[0].ArgFlags[i + 1] & VARF_Optional))
							{
								sc.ScriptError("Insufficient parameters for %s", cls->TypeName.GetChars());
							}
							break;
						}
					}

					DMenuItemBase *item = (DMenuItemBase*)cls->CreateNew();
					params[0] = item;
					VMCallWithDefaults(func->Variants[0].Implementation, params, nullptr, 0);
					desc->mItems.Push((DMenuItemBase*)item);

					success = true;
				}
			}
			if (!success)
			{
				sc.ScriptError("Unknown keyword '%s'", sc.String);
			}
		}
	}
	for (auto &p : desc->mItems)
	{
		GC::WriteBarrier(p);
	}
}

//=============================================================================
//
//
//
//=============================================================================

static void ParseOptionMenu(FScanner &sc)
{
	sc.MustGetString();

	DOptionMenuDescriptor *desc = Create<DOptionMenuDescriptor>();
	desc->mFont = gameinfo.gametype == GAME_Doom ? BigUpper : BigFont;
	desc->mMenuName = sc.String;
	desc->mSelectedItem = -1;
	desc->mScrollPos = 0;
	desc->mClass = nullptr;
	desc->mPosition = DefaultOptionMenuSettings->mPosition;
	desc->mScrollTop = DefaultOptionMenuSettings->mScrollTop;
	desc->mIndent =  DefaultOptionMenuSettings->mIndent;
	desc->mDontDim =  DefaultOptionMenuSettings->mDontDim;
	desc->mProtected = sc.CheckString("protected");

	ParseOptionMenuBody(sc, desc);
	ReplaceMenu(sc, desc);
}


//=============================================================================
//
//
//
//=============================================================================

static void ParseAddOptionMenu(FScanner &sc)
{
	sc.MustGetString();

	DMenuDescriptor **pOld = MenuDescriptors.CheckKey(sc.String);
	if (pOld == nullptr || *pOld == nullptr || !(*pOld)->IsKindOf(RUNTIME_CLASS(DOptionMenuDescriptor)))
	{
		sc.ScriptError("%s is not an option menu that can be extended", sc.String);
	}
	ParseOptionMenuBody(sc, (DOptionMenuDescriptor*)(*pOld));
}


//=============================================================================
//
//
//
//=============================================================================

void M_ParseMenuDefs()
{
	int lump, lastlump = 0;

	OptionSettings.mTitleColor = V_FindFontColor(gameinfo.mTitleColor);
	OptionSettings.mFontColor = V_FindFontColor(gameinfo.mFontColor);
	OptionSettings.mFontColorValue = V_FindFontColor(gameinfo.mFontColorValue);
	OptionSettings.mFontColorMore = V_FindFontColor(gameinfo.mFontColorMore);
	OptionSettings.mFontColorHeader = V_FindFontColor(gameinfo.mFontColorHeader);
	OptionSettings.mFontColorHighlight = V_FindFontColor(gameinfo.mFontColorHighlight);
	OptionSettings.mFontColorSelection = V_FindFontColor(gameinfo.mFontColorSelection);
	// these are supposed to get GC'd after parsing is complete.
	DefaultListMenuSettings = Create<DListMenuDescriptor>();
	DefaultOptionMenuSettings = Create<DOptionMenuDescriptor>();
	DefaultListMenuSettings->Reset();
	DefaultOptionMenuSettings->Reset();

	int IWADMenu = fileSystem.CheckNumForName("MENUDEF", ns_global, fileSystem.GetIwadNum());

	while ((lump = fileSystem.FindLump ("MENUDEF", &lastlump)) != -1)
	{
		FScanner sc(lump);

		mustPrintErrors = lump >= IWADMenu;
		sc.SetCMode(true);
		while (sc.GetString())
		{
			if (sc.Compare("LISTMENU"))
			{
				ParseListMenu(sc);
			}
			else if (sc.Compare("DEFAULTLISTMENU"))
			{
				ParseListMenuBody(sc, DefaultListMenuSettings);
				if (DefaultListMenuSettings->mItems.Size() > 0)
				{
					I_FatalError("You cannot add menu items to the menu default settings.");
				}
			}
			else if (sc.Compare("OPTIONVALUE"))
			{
				ParseOptionValue(sc);
			}
			else if (sc.Compare("OPTIONSTRING"))
			{
				ParseOptionString(sc);
			}
			else if (sc.Compare("OPTIONMENUSETTINGS"))
			{
				ParseOptionSettings(sc);
			}
			else if (sc.Compare("OPTIONMENU"))
			{
				ParseOptionMenu(sc);
			}
			else if (sc.Compare("ADDOPTIONMENU"))
			{
				ParseAddOptionMenu(sc);
			}
			else if (sc.Compare("DEFAULTOPTIONMENU"))
			{
				ParseOptionMenuBody(sc, DefaultOptionMenuSettings);
				if (DefaultOptionMenuSettings->mItems.Size() > 0)
				{
					I_FatalError("You cannot add menu items to the menu default settings.");
				}
			}
			else
			{
				sc.ScriptError("Unknown keyword '%s'", sc.String);
			}
		}
		if (Args->CheckParm("-nocustommenu")) break;
	}
	DefaultListMenuClass = DefaultListMenuSettings->mClass;
	DefaultListMenuSettings = nullptr;
	DefaultOptionMenuClass = DefaultOptionMenuSettings->mClass;
	DefaultOptionMenuSettings = nullptr;
}


//=============================================================================
//
// Creates the episode menu
// Falls back on an option menu if there's not enough screen space to show all episodes
//
//=============================================================================

void M_StartupEpisodeMenu(FGameStartup *gs)
{
	// Build episode menu
	bool success = false;
	bool isOld = false;
	DMenuDescriptor **desc = MenuDescriptors.CheckKey(NAME_Episodemenu);
	if (desc != nullptr)
	{
		if ((*desc)->IsKindOf(RUNTIME_CLASS(DListMenuDescriptor)))
		{
			DListMenuDescriptor *ld = static_cast<DListMenuDescriptor*>(*desc);
			
			// Delete previous contents
			for(unsigned i=0; i<ld->mItems.Size(); i++)
			{
				FName n = ld->mItems[i]->mAction;
				if (n == NAME_Skillmenu)
				{
					isOld = true;
					ld->mItems.Resize(i);
					break;
				}
			}

			
			int posx = (int)ld->mXpos;
			int posy = (int)ld->mYpos;
			int topy = posy;

			// Get lowest y coordinate of any static item in the menu
			for(unsigned i = 0; i < ld->mItems.Size(); i++)
			{
				int y = (int)ld->mItems[i]->GetY();
				if (y < topy) topy = y;
			}

			// center the menu on the screen if the top space is larger than the bottom space
			int totalheight = posy + AllEpisodes.Size() * ld->mLinespacing - topy;

			if (totalheight < 190 || AllEpisodes.Size() == 1)
			{
				int newtop = (200 - totalheight + topy) / 2;
				int topdelta = newtop - topy;
				if (topdelta < 0)
				{
					for(unsigned i = 0; i < ld->mItems.Size(); i++)
					{
						ld->mItems[i]->OffsetPositionY(topdelta);
					}
					posy -= topdelta;
				}

				if (!isOld) ld->mSelectedItem = ld->mItems.Size();

				for (unsigned i = 0; i < AllEpisodes.Size(); i++)
				{
					DMenuItemBase *it = nullptr;
					if (AllEpisodes[i].mPicName.IsNotEmpty())
					{
						FTextureID tex = GetMenuTexture(AllEpisodes[i].mPicName);
						if (AllEpisodes[i].mEpisodeName.IsEmpty() || OkForLocalization(tex, AllEpisodes[i].mEpisodeName))
							continue;	// We do not measure patch based entries. They are assumed to fit
					}
					const char *c = AllEpisodes[i].mEpisodeName;
					if (*c == '$') c = GStrings(c + 1);
					int textwidth = ld->mFont->StringWidth(c);
					int textright = posx + textwidth;
					if (posx + textright > 320) posx = std::max(0, 320 - textright);
				}

				for(unsigned i = 0; i < AllEpisodes.Size(); i++)
				{
					DMenuItemBase *it = nullptr;
					if (AllEpisodes[i].mPicName.IsNotEmpty())
					{
						FTextureID tex = GetMenuTexture(AllEpisodes[i].mPicName);
						if (AllEpisodes[i].mEpisodeName.IsEmpty() || OkForLocalization(tex, AllEpisodes[i].mEpisodeName))
							it = CreateListMenuItemPatch(posx, posy, ld->mLinespacing, AllEpisodes[i].mShortcut, tex, NAME_Skillmenu, i);
					}
					if (it == nullptr)
					{
						it = CreateListMenuItemText(posx, posy, ld->mLinespacing, AllEpisodes[i].mShortcut, 
							AllEpisodes[i].mEpisodeName, ld->mFont, ld->mFontColor, ld->mFontColor2, NAME_Skillmenu, i);
					}
					ld->mItems.Push(it);
					posy += ld->mLinespacing;
				}
				if (AllEpisodes.Size() == 1)
				{
					ld->mAutoselect = ld->mSelectedItem;
				}
				success = true;
				for (auto &p : ld->mItems)
				{
					GC::WriteBarrier(*desc, p);
				}
			}
		}
		else return;	// do not recreate the option menu variant, because it is always text based.
	}
	if (!success)
	{
		// Couldn't create the episode menu, either because there's too many episodes or some error occured
		// Create an option menu for episode selection instead.
		DOptionMenuDescriptor *od = Create<DOptionMenuDescriptor>();
		MenuDescriptors[NAME_Episodemenu] = od;
		od->mMenuName = NAME_Episodemenu;
		od->mFont = gameinfo.gametype == GAME_Doom ? BigUpper : BigFont;
		od->mTitle = "$MNU_EPISODE";
		od->mSelectedItem = 0;
		od->mScrollPos = 0;
		od->mClass = nullptr;
		od->mPosition = -15;
		od->mScrollTop = 0;
		od->mIndent = 160;
		od->mDontDim = false;
		GC::WriteBarrier(od);
		for(unsigned i = 0; i < AllEpisodes.Size(); i++)
		{
			auto it = CreateOptionMenuItemSubmenu(AllEpisodes[i].mEpisodeName, "Skillmenu", i);
			od->mItems.Push(it);
			GC::WriteBarrier(od, it);
		}
	}
}

//=============================================================================
//
//
//
//=============================================================================

static void BuildPlayerclassMenu()
{
	bool success = false;

	// Build player class menu
	DMenuDescriptor **desc = MenuDescriptors.CheckKey(NAME_Playerclassmenu);
	if (desc != nullptr)
	{
		if ((*desc)->IsKindOf(RUNTIME_CLASS(DListMenuDescriptor)))
		{
			DListMenuDescriptor *ld = static_cast<DListMenuDescriptor*>(*desc);
			// add player display

			ld->mSelectedItem = ld->mItems.Size();
			
			int posy = (int)ld->mYpos;
			int topy = posy;

			// Get lowest y coordinate of any static item in the menu
			for(unsigned i = 0; i < ld->mItems.Size(); i++)
			{
				int y = (int)ld->mItems[i]->GetY();
				if (y < topy) topy = y;
			}

			// Count the number of items this menu will show
			int numclassitems = 0;
			for (unsigned i = 0; i < PlayerClasses.Size (); i++)
			{
				if (!(PlayerClasses[i].Flags & PCF_NOMENU))
				{
					const char *pname = GetPrintableDisplayName(PlayerClasses[i].Type);
					if (pname != nullptr)
					{
						numclassitems++;
					}
				}
			}

			// center the menu on the screen if the top space is larger than the bottom space
			int totalheight = posy + (numclassitems+1) * ld->mLinespacing - topy;

			if (numclassitems <= 1)
			{
				// create a dummy item that auto-chooses the default class.
				auto it = CreateListMenuItemText(0, 0, 0, 'p', "player", 
					ld->mFont,ld->mFontColor, ld->mFontColor2, NAME_Episodemenu, -1000);
				ld->mAutoselect = ld->mItems.Push(it);
				success = true;
			}
			else if (totalheight <= 190)
			{
				int newtop = (200 - totalheight + topy) / 2;
				int topdelta = newtop - topy;
				if (topdelta < 0)
				{
					for(unsigned i = 0; i < ld->mItems.Size(); i++)
					{
						ld->mItems[i]->OffsetPositionY(topdelta);
					}
					posy -= topdelta;
				}

				int n = 0;
				for (unsigned i = 0; i < PlayerClasses.Size (); i++)
				{
					if (!(PlayerClasses[i].Flags & PCF_NOMENU))
					{
						const char *pname = GetPrintableDisplayName(PlayerClasses[i].Type);
						if (pname != nullptr)
						{
							auto it = CreateListMenuItemText(ld->mXpos, ld->mYpos, ld->mLinespacing, *pname,
								pname, ld->mFont,ld->mFontColor,ld->mFontColor2, NAME_Episodemenu, i);
							ld->mItems.Push(it);
							ld->mYpos += ld->mLinespacing;
							n++;
						}
					}
				}
				if (n > 1 && !gameinfo.norandomplayerclass)
				{
					auto it = CreateListMenuItemText(ld->mXpos, ld->mYpos, ld->mLinespacing, 'r',
						"$MNU_RANDOM", ld->mFont,ld->mFontColor,ld->mFontColor2, NAME_Episodemenu, -1);
					ld->mItems.Push(it);
				}
				if (n == 0)
				{
					const char *pname = GetPrintableDisplayName(PlayerClasses[0].Type);
					if (pname != nullptr)
					{
						auto it = CreateListMenuItemText(ld->mXpos, ld->mYpos, ld->mLinespacing, *pname,
							pname, ld->mFont,ld->mFontColor,ld->mFontColor2, NAME_Episodemenu, 0);
						ld->mItems.Push(it);
					}
				}
				success = true;
				for (auto &p : ld->mItems)
				{
					GC::WriteBarrier(ld, p);
				}
			}
		}
	}
	if (!success)
	{
		// Couldn't create the playerclass menu, either because there's too many episodes or some error occured
		// Create an option menu for class selection instead.
		DOptionMenuDescriptor *od = Create<DOptionMenuDescriptor>();
		MenuDescriptors[NAME_Playerclassmenu] = od;
		od->mMenuName = NAME_Playerclassmenu;
		od->mFont = gameinfo.gametype == GAME_Doom ? BigUpper : BigFont;
		od->mTitle = "$MNU_CHOOSECLASS";
		od->mSelectedItem = 0;
		od->mScrollPos = 0;
		od->mClass = nullptr;
		od->mPosition = -15;
		od->mScrollTop = 0;
		od->mIndent = 160;
		od->mDontDim = false;
		od->mNetgameMessage = "$NEWGAME";
		GC::WriteBarrier(od);
		for (unsigned i = 0; i < PlayerClasses.Size (); i++)
		{
			if (!(PlayerClasses[i].Flags & PCF_NOMENU))
			{
				const char *pname = GetPrintableDisplayName(PlayerClasses[i].Type);
				if (pname != nullptr)
				{
					auto it = CreateOptionMenuItemSubmenu(pname, "Episodemenu", i);
					od->mItems.Push(it);
					GC::WriteBarrier(od, it);
				}
			}
		}
		auto it = CreateOptionMenuItemSubmenu("Random", "Episodemenu", -1);
		od->mItems.Push(it);
		GC::WriteBarrier(od, it);
	}
}

//=============================================================================
//
// Reads any XHAIRS lumps for the names of crosshairs and
// adds them to the display options menu.
//
//=============================================================================

static void InitCrosshairsList()
{
	int lastlump, lump;

	lastlump = 0;

	FOptionValues **opt = OptionValues.CheckKey(NAME_Crosshairs);
	if (opt == nullptr) 
	{
		return;	// no crosshair value list present. No need to go on.
	}

	FOptionValues::Pair *pair = &(*opt)->mValues[(*opt)->mValues.Reserve(1)];
	pair->Value = 0;
	pair->Text = "None";

	while ((lump = fileSystem.FindLump("XHAIRS", &lastlump)) != -1)
	{
		FScanner sc(lump);
		while (sc.GetNumber())
		{
			FOptionValues::Pair value;
			value.Value = sc.Number;
			sc.MustGetString();
			value.Text = sc.String;
			if (value.Value != 0)
			{ // Check if it already exists. If not, add it.
				unsigned int i;

				for (i = 1; i < (*opt)->mValues.Size(); ++i)
				{
					if ((*opt)->mValues[i].Value == value.Value)
					{
						break;
					}
				}
				if (i < (*opt)->mValues.Size())
				{
					(*opt)->mValues[i].Text = value.Text;
				}
				else
				{
					(*opt)->mValues.Push(value);
				}
			}
		}
	}
}

//=============================================================================
//
// Initialize the music configuration submenus
//
//=============================================================================
extern "C"
{
	extern int adl_getBanksCount();
	extern const char *const *adl_getBankNames();
}

static void InitMusicMenus()
{
	DMenuDescriptor **advmenu = MenuDescriptors.CheckKey("AdvSoundOptions");
	auto soundfonts = sfmanager.GetList();
	std::tuple<const char *, int, const char *> sfmenus[] = { std::make_tuple("GusConfigMenu", SF_SF2 | SF_GUS, "midi_config"),
																std::make_tuple("WildMidiConfigMenu", SF_GUS, "wildmidi_config"),
																std::make_tuple("TimidityConfigMenu", SF_SF2 | SF_GUS, "timidity_config"),
																std::make_tuple("FluidPatchsetMenu", SF_SF2, "fluid_patchset"),
																std::make_tuple("ADLMIDICustomBanksMenu", SF_WOPL, "adl_custom_bank"),
																std::make_tuple("OPNMIDICustomBanksMenu", SF_WOPN, "opn_custom_bank")};

	for (auto &p : sfmenus)
	{
		DMenuDescriptor **menu = MenuDescriptors.CheckKey(std::get<0>(p));

		if (menu != nullptr)
		{
			if (soundfonts.Size() > 0)
			{
				for (auto &entry : soundfonts)
				{
					if (entry.type & std::get<1>(p))
					{
						FString display = entry.mName;
						display.ReplaceChars("_", ' ');
						auto it = CreateOptionMenuItemCommand(display, FStringf("%s \"%s\"", std::get<2>(p), entry.mName.GetChars()), true);
						static_cast<DOptionMenuDescriptor*>(*menu)->mItems.Push(it);
					}
				}
			}
			else if (advmenu != nullptr)
			{
				// Remove the item for this submenu
				auto d = static_cast<DOptionMenuDescriptor*>(*advmenu);
				auto it = d->GetItem(std::get<0>(p));
				if (it != nullptr) d->mItems.Delete(d->mItems.Find(it));
			}
		}
	}

	DMenuDescriptor **menu = MenuDescriptors.CheckKey("ADLBankMenu");

	if (menu != nullptr)
	{
		const char* const* adl_bank_names;
		int adl_banks_count = ZMusic_GetADLBanks(&adl_bank_names);
		for (int i=0; i < adl_banks_count; i++)
		{
			auto it = CreateOptionMenuItemCommand(adl_bank_names[i], FStringf("adl_bank %d", i), true);
			static_cast<DOptionMenuDescriptor*>(*menu)->mItems.Push(it);
		}
	}
}

//=============================================================================
//
// With the current workings of the menu system this cannot be done any longer
// from within the respective CCMDs.
//
//=============================================================================

static void InitKeySections()
{
	DMenuDescriptor **desc = MenuDescriptors.CheckKey(NAME_CustomizeControls);
	if (desc != nullptr)
	{
		if ((*desc)->IsKindOf(RUNTIME_CLASS(DOptionMenuDescriptor)))
		{
			DOptionMenuDescriptor *menu = static_cast<DOptionMenuDescriptor*>(*desc);

			for (unsigned i = 0; i < KeySections.Size(); i++)
			{
				FKeySection *sect = &KeySections[i];
				DMenuItemBase *item = CreateOptionMenuItemStaticText(" ");
				menu->mItems.Push(item);
				item = CreateOptionMenuItemStaticText(sect->mTitle, 1);
				menu->mItems.Push(item);
				for (unsigned j = 0; j < sect->mActions.Size(); j++)
				{
					FKeyAction *act = &sect->mActions[j];
					item = CreateOptionMenuItemControl(act->mTitle, act->mAction, &Bindings);
					menu->mItems.Push(item);
				}
			}
			for (auto &p : menu->mItems)
			{
				GC::WriteBarrier(*desc, p);
			}
		}
	}
}

//=============================================================================
//
// Special menus will be created once all engine data is loaded
//
//=============================================================================

void M_CreateMenus()
{
	BuildPlayerclassMenu();
	InitCrosshairsList();
	InitMusicMenus();
	InitKeySections();

	FOptionValues **opt = OptionValues.CheckKey(NAME_Mididevices);
	if (opt != nullptr) 
	{
		I_BuildMIDIMenuList(*opt);
	}
	opt = OptionValues.CheckKey(NAME_Aldevices);
	if (opt != nullptr) 
	{
		I_BuildALDeviceList(*opt);
	}
	opt = OptionValues.CheckKey(NAME_Alresamplers);
	if (opt != nullptr)
	{
		I_BuildALResamplersList(*opt);
	}
	opt = OptionValues.CheckKey(NAME_PlayerTeam);
	if (opt != nullptr)
	{
		auto op = *opt; 
		op->mValues.Resize(Teams.Size() + 1);
		op->mValues[0].Value = 0;
		op->mValues[0].Text = "$OPTVAL_NONE";
		for (unsigned i = 0; i < Teams.Size(); i++)
		{
			op->mValues[i+1].Value = i+1;
			op->mValues[i+1].Text = Teams[i].GetName();
		}
	}
	opt = OptionValues.CheckKey(NAME_PlayerClass);
	if (opt != nullptr)
	{
		auto op = *opt;
		int o = 0;
		if (!gameinfo.norandomplayerclass && PlayerClasses.Size() > 1)
		{
			op->mValues.Resize(PlayerClasses.Size()+1);
			op->mValues[0].Value = -1;
			op->mValues[0].Text = "$MNU_RANDOM";
			o = 1;
		}
		else op->mValues.Resize(PlayerClasses.Size());
		for (unsigned i = 0; i < PlayerClasses.Size(); i++)
		{
			op->mValues[i+o].Value = i;
			op->mValues[i+o].Text = GetPrintableDisplayName(PlayerClasses[i].Type);
		}
	}
}


DEFINE_ACTION_FUNCTION(DMenu, UpdateColorsets)
{
	PARAM_PROLOGUE;
	PARAM_POINTER(playerClass, FPlayerClass);

	TArray<int> PlayerColorSets;

	EnumColorSets(playerClass->Type, &PlayerColorSets);

	auto opt = OptionValues.CheckKey(NAME_PlayerColors);
	if (opt != nullptr)
	{
		auto op = *opt;
		op->mValues.Resize(PlayerColorSets.Size() + 1);
		op->mValues[0].Value = -1;
		op->mValues[0].Text = "$OPTVAL_CUSTOM";
		for (unsigned i = 0; i < PlayerColorSets.Size(); i++)
		{
			auto cset = GetColorSet(playerClass->Type, PlayerColorSets[i]);
			op->mValues[i + 1].Value = PlayerColorSets[i];
			op->mValues[i + 1].Text = cset? cset->Name.GetChars() : "?";	// The null case should never happen here.
		}
	}
	return 0;
}

DEFINE_ACTION_FUNCTION(DMenu, UpdateSkinOptions)
{
	PARAM_PROLOGUE;
	PARAM_POINTER(playerClass, FPlayerClass);

	auto opt = OptionValues.CheckKey(NAME_PlayerSkin);
	if (opt != nullptr)
	{
		auto op = *opt;

		if ((GetDefaultByType(playerClass->Type)->flags4 & MF4_NOSKIN) || players[consoleplayer].userinfo.GetPlayerClassNum() == -1)
		{
			op->mValues.Resize(1);
			op->mValues[0].Value = -1;
			op->mValues[0].Text = "$OPTVAL_DEFAULT";
		}
		else
		{
			op->mValues.Clear();
			for (unsigned i = 0; i < Skins.Size(); i++)
			{
				op->mValues.Reserve(1);
				op->mValues.Last().Value = i;
				op->mValues.Last().Text = Skins[i].Name;
			}
		}
	}
	return 0;
}

//=============================================================================
//
// The skill menu must be refeshed each time it starts up
//
//=============================================================================
extern int restart;

void M_StartupSkillMenu(FGameStartup *gs)
{
	static int done = -1;
	bool success = false;
	TArray<FSkillInfo*> MenuSkills;
	TArray<int> SkillIndices;
	if (MenuSkills.Size() == 0)
	{
		for (unsigned ind = 0; ind < AllSkills.Size(); ind++)
		{
			if (!AllSkills[ind].NoMenu)
			{
				MenuSkills.Push(&AllSkills[ind]);
				SkillIndices.Push(ind);
			}
		}
	}
	if (MenuSkills.Size() == 0) I_Error("No valid skills for menu found. At least one must be defined.");

	int defskill = DefaultSkill;
	if ((unsigned int)defskill >= MenuSkills.Size())
	{
		defskill = SkillIndices[(MenuSkills.Size() - 1) / 2];
	}
	if (AllSkills[defskill].NoMenu)
	{
		for (defskill = 0; defskill < (int)AllSkills.Size(); defskill++)
		{
			if (!AllSkills[defskill].NoMenu) break;
		}
	}
	int defindex = 0;
	for (unsigned i = 0; i < MenuSkills.Size(); i++)
	{
		if (MenuSkills[i] == &AllSkills[defskill])
		{
			defindex = i;
			break;
		}
	}

	DMenuDescriptor **desc = MenuDescriptors.CheckKey(NAME_Skillmenu);
	if (desc != nullptr)
	{
		if ((*desc)->IsKindOf(RUNTIME_CLASS(DListMenuDescriptor)))
		{
			DListMenuDescriptor *ld = static_cast<DListMenuDescriptor*>(*desc);
			int posx = (int)ld->mXpos;
			int y = (int)ld->mYpos;

			// Delete previous contents
			for(unsigned i=0; i<ld->mItems.Size(); i++)
			{
				FName n = ld->mItems[i]->mAction;
				if (n == NAME_Startgame || n == NAME_StartgameConfirm) 
				{
					ld->mItems.Resize(i);
					break;
				}
			}

			if (done != restart)
			{
				done = restart;
				ld->mSelectedItem = ld->mItems.Size() + defindex;

				int posy = y;
				int topy = posy;

				// Get lowest y coordinate of any static item in the menu
				for(unsigned i = 0; i < ld->mItems.Size(); i++)
				{
					int y = (int)ld->mItems[i]->GetY();
					if (y < topy) topy = y;
				}

				// center the menu on the screen if the top space is larger than the bottom space
				int totalheight = posy + MenuSkills.Size() * ld->mLinespacing - topy;

				if (totalheight < 190 || MenuSkills.Size() == 1)
				{
					int newtop = (200 - totalheight + topy) / 2;
					int topdelta = newtop - topy;
					if (topdelta < 0)
					{
						for(unsigned i = 0; i < ld->mItems.Size(); i++)
						{
							ld->mItems[i]->OffsetPositionY(topdelta);
						}
						ld->mYpos = y = posy - topdelta;
					}
				}
				else
				{
					// too large
					desc = nullptr;
					done = false;
					goto fail;
				}
			}

			for (unsigned int i = 0; i < MenuSkills.Size(); i++)
			{
				FSkillInfo &skill = *MenuSkills[i];
				DMenuItemBase *li = nullptr;

				FString *pItemText = nullptr;
				if (gs->PlayerClass != nullptr)
				{
					pItemText = skill.MenuNamesForPlayerClass.CheckKey(gs->PlayerClass);
				}

				if (skill.PicName.Len() != 0 && pItemText == nullptr)
				{
					FTextureID tex = GetMenuTexture(skill.PicName);
					if (skill.MenuName.IsEmpty() || OkForLocalization(tex, skill.MenuName))
						continue;
				}
				const char *c = pItemText ? pItemText->GetChars() : skill.MenuName.GetChars();
				if (*c == '$') c = GStrings(c + 1);
				int textwidth = ld->mFont->StringWidth(c);
				int textright = posx + textwidth;
				if (posx + textright > 320) posx = std::max(0, 320 - textright);
			}

			unsigned firstitem = ld->mItems.Size();
			for(unsigned int i = 0; i < MenuSkills.Size(); i++)
			{
				FSkillInfo &skill = *MenuSkills[i];
				DMenuItemBase *li = nullptr;
				// Using a different name for skills that must be confirmed makes handling this easier.
				FName action = (skill.MustConfirm && !AllEpisodes[gs->Episode].mNoSkill) ?
					NAME_StartgameConfirm : NAME_Startgame;
				FString *pItemText = nullptr;
				if (gs->PlayerClass != nullptr)
				{
					pItemText = skill.MenuNamesForPlayerClass.CheckKey(gs->PlayerClass);
				}

				EColorRange color = (EColorRange)skill.GetTextColor();
				if (color == CR_UNTRANSLATED) color = ld->mFontColor;
				if (skill.PicName.Len() != 0 && pItemText == nullptr)
				{
					FTextureID tex = GetMenuTexture(skill.PicName);
					if (skill.MenuName.IsEmpty() || OkForLocalization(tex, skill.MenuName))
						li = CreateListMenuItemPatch(posx, y, ld->mLinespacing, skill.Shortcut, tex, action, SkillIndices[i]);
				}
				if (li == nullptr)
				{
					li = CreateListMenuItemText(posx, y, ld->mLinespacing, skill.Shortcut,
									pItemText? *pItemText : skill.MenuName, ld->mFont, color,ld->mFontColor2, action, SkillIndices[i]);
				}
				ld->mItems.Push(li);
				GC::WriteBarrier(*desc, li);
				y += ld->mLinespacing;
			}
			if (AllEpisodes[gs->Episode].mNoSkill || MenuSkills.Size() == 1)
			{
				ld->mAutoselect = firstitem + defindex;
			}
			else
			{
				ld->mAutoselect = -1;
			}
			success = true;
		}
	}
	if (success) return;
fail:
	// Option menu fallback for overlong skill lists
	DOptionMenuDescriptor *od;
	if (desc == nullptr)
	{
		od = Create<DOptionMenuDescriptor>();
		MenuDescriptors[NAME_Skillmenu] = od;
		od->mMenuName = NAME_Skillmenu;
		od->mFont = gameinfo.gametype == GAME_Doom ? BigUpper : BigFont;
		od->mTitle = "$MNU_CHOOSESKILL";
		od->mSelectedItem = defindex;
		od->mScrollPos = 0;
		od->mClass = nullptr;
		od->mPosition = -15;
		od->mScrollTop = 0;
		od->mIndent = 160;
		od->mDontDim = false;
		GC::WriteBarrier(od);
	}
	else
	{
		od = static_cast<DOptionMenuDescriptor*>(*desc);
		od->mItems.Clear();
	}
	for(unsigned int i = 0; i < MenuSkills.Size(); i++)
	{
		FSkillInfo &skill = *MenuSkills[i];
		DMenuItemBase *li;
		// Using a different name for skills that must be confirmed makes handling this easier.
		const char *action = (skill.MustConfirm && !AllEpisodes[gs->Episode].mNoSkill) ?
			"StartgameConfirm" : "Startgame";

		FString *pItemText = nullptr;
		if (gs->PlayerClass != nullptr)
		{
			pItemText = skill.MenuNamesForPlayerClass.CheckKey(gs->PlayerClass);
		}
		li = CreateOptionMenuItemSubmenu(pItemText? *pItemText : skill.MenuName, action, SkillIndices[i]);
		od->mItems.Push(li);
		GC::WriteBarrier(od, li);
		if (!done)
		{
			done = true;
			od->mSelectedItem = defindex;
		}
	}
}


#ifdef _WIN32
EXTERN_CVAR(Bool, vr_enable_quadbuffered)
#endif

void UpdateVRModes(bool considerQuadBuffered)
{
	FOptionValues ** pVRModes = OptionValues.CheckKey("VRMode");
	if (pVRModes == nullptr) return;

	TArray<FOptionValues::Pair> & vals = (*pVRModes)->mValues;
	TArray<FOptionValues::Pair> filteredValues;
	int cnt = vals.Size();
	for (int i = 0; i < cnt; ++i) {
		auto const & mode = vals[i];
		if (mode.Value == 7) {  // Quad-buffered stereo
#ifdef _WIN32
			if (!vr_enable_quadbuffered) continue;
#else
			continue;  // Remove quad-buffered option on Mac and Linux
#endif
			if (!considerQuadBuffered) continue;  // Probably no compatible screen mode was found
		}
		filteredValues.Push(mode);
	}
	vals = filteredValues;
}
