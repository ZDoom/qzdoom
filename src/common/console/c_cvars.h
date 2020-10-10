/*
** c_cvars.h
**
**---------------------------------------------------------------------------
** Copyright 1998-2006 Randy Heit
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

#ifndef __C_CVARS_H__
#define __C_CVARS_H__
#include "zstring.h"
#include "tarray.h"

class FSerializer; // this needs to go away.
/*
==========================================================

CVARS (console variables)

==========================================================
*/

enum
{
	CVAR_ARCHIVE       = 1,       // set to cause it to be saved to config.
	CVAR_USERINFO      = 1 <<  1, // added to userinfo  when changed.
	CVAR_SERVERINFO    = 1 <<  2, // added to serverinfo when changed.
	CVAR_NOSET         = 1 <<  3, // don't allow change from console at all,
	                              // but can be set from the command line.
	CVAR_LATCH         = 1 <<  4, // save changes until server restart.
	CVAR_UNSETTABLE    = 1 <<  5, // can unset this var from console.
	CVAR_DEMOSAVE      = 1 <<  6, // save the value of this cvar in a demo.
	CVAR_ISDEFAULT     = 1 <<  7, // is cvar unchanged since creation?
	CVAR_AUTO          = 1 <<  8, // allocated; needs to be freed when destroyed.
	CVAR_NOINITCALL    = 1 <<  9, // don't call callback at game start.
	CVAR_GLOBALCONFIG  = 1 << 10, // cvar is saved to global config section.
	CVAR_VIDEOCONFIG   = 1 << 11, // cvar is saved to video config section (not implemented).
	CVAR_NOSAVE        = 1 << 12, // when used with CVAR_SERVERINFO, do not save var to savegame
	                              // and config.
	CVAR_MOD           = 1 << 13, // cvar was defined by a mod.
	CVAR_IGNORE        = 1 << 14, // do not send cvar across the network/inaccesible from ACS
	                              // (dummy mod cvar).
	CVAR_CHEAT         = 1 << 15, // can be set only when sv_cheats is enabled.
	CVAR_UNSAFECONTEXT = 1 << 16, // cvar value came from unsafe context.
	CVAR_VIRTUAL       = 1 << 17, // do not invoke the callback recursively so it can be used to
	                              // mirror an external variable.
	CVAR_CONFIG_ONLY   = 1 << 18, // do not save var to savegame and do not send it across network.
};

union UCVarValue
{
	bool Bool;
	int Int;
	float Float;
	const char* String;

	UCVarValue() = default;
	UCVarValue(bool v) { Bool = v; }
	UCVarValue(int v) { Int = v; }
	UCVarValue(float v) { Float = v; }
	UCVarValue(const char * v) { String = v; }
};

enum ECVarType
{
	CVAR_Bool,
	CVAR_Int,
	CVAR_Float,
	CVAR_String,
	CVAR_Color,		// stored as CVAR_Int
	CVAR_DummyBool,		// just redirects to another cvar
	CVAR_DummyInt,		// just redirects to another cvar
	CVAR_Dummy			// Unknown
};

class FConfigFile;

class FxCVar;

class FBaseCVar;

// These are calls into the game code. Having these hard coded in the CVAR implementation has always been the biggest blocker
// for reusing the CVAR module outside of ZDoom. So now they get called through this struct for easier reusability.
struct ConsoleCallbacks
{
	void (*UserInfoChanged)(FBaseCVar*);
	bool (*SendServerInfoChange)(FBaseCVar* cvar, UCVarValue value, ECVarType type);
	bool (*SendServerFlagChange)(FBaseCVar* cvar, int bitnum, bool set, bool silent);
	FBaseCVar* (*GetUserCVar)(int playernum, const char* cvarname);
	bool (*MustLatch)();

};

class FBaseCVar
{
public:
	FBaseCVar (const char *name, uint32_t flags, void (*callback)(FBaseCVar &), const char *descr);
	virtual ~FBaseCVar ();

	inline void Callback () 
	{ 
		if (m_Callback && !inCallback)
		{
			inCallback = !!(Flags & CVAR_VIRTUAL);	// Virtual CVARs never invoke the callback recursively, giving it a chance to manipulate the value without side effects.
			m_Callback(*this);
			inCallback = false;
		}
	}

	inline const char *GetName () const { return VarName.GetChars(); }
	inline uint32_t GetFlags () const { return Flags; }
	inline FBaseCVar *GetNext() const { return m_Next; }

	void CmdSet (const char *newval);
	void ForceSet (UCVarValue value, ECVarType type, bool nouserinfosend=false);
	void SetGenericRep (UCVarValue value, ECVarType type);
	void ResetToDefault ();
	void SetArchiveBit () { Flags |= CVAR_ARCHIVE; }
	void MarkUnsafe();
	void MarkSafe() { Flags &= ~CVAR_UNSAFECONTEXT; }
	void AddDescription(const FString& label)
	{
		if (Description.IsEmpty()) Description = label;
	}

	int ToInt()
	{
		ECVarType vt;
		auto val = GetFavoriteRep(&vt);
		return ToInt(val, vt);
	}

	virtual ECVarType GetRealType () const = 0;

	virtual const char *GetHumanString(int precision=-1) const;
	virtual UCVarValue GetGenericRep (ECVarType type) const = 0;
	virtual UCVarValue GetFavoriteRep (ECVarType *type) const = 0;

	virtual const char *GetHumanStringDefault(int precision = -1) const;
	virtual UCVarValue GetGenericRepDefault (ECVarType type) const = 0;
	virtual UCVarValue GetFavoriteRepDefault (ECVarType *type) const = 0;
	virtual void SetGenericRepDefault (UCVarValue value, ECVarType type) = 0;

	FBaseCVar &operator= (const FBaseCVar &var)
		{ UCVarValue val; ECVarType type; val = var.GetFavoriteRep (&type); SetGenericRep (val, type); return *this; }

	static void EnableNoSet ();		// enable the honoring of CVAR_NOSET
	static void EnableCallbacks ();
	static void DisableCallbacks ();
	static void ResetColors ();		// recalc color cvars' indices after screen change

	static void ListVars (const char *filter, bool plain);
	
	const FString &GetDescription() const { return Description; };
	const FString& GetToggleMessage(int which) { return ToggleMessages[which]; }
	void SetToggleMessages(const char* on, const char* off)
	{
		ToggleMessages[0] = off;
		ToggleMessages[1] = on;
	}


protected:
	virtual void DoSet (UCVarValue value, ECVarType type) = 0;

	static bool ToBool (UCVarValue value, ECVarType type);
	static int ToInt (UCVarValue value, ECVarType type);
	static float ToFloat (UCVarValue value, ECVarType type);
	static const char *ToString (UCVarValue value, ECVarType type);
	static UCVarValue FromBool (bool value, ECVarType type);
	static UCVarValue FromInt (int value, ECVarType type);
	static UCVarValue FromFloat (float value, ECVarType type);
	static UCVarValue FromString (const char *value, ECVarType type);

	FString VarName;
	FString SafeValue;
	FString Description;
	FString ToggleMessages[2];
	uint32_t Flags;
	bool inCallback = false;

private:
	FBaseCVar (const FBaseCVar &var) = delete;
	FBaseCVar (const char *name, uint32_t flags);
	void (*m_Callback)(FBaseCVar &);
	FBaseCVar *m_Next;

	static bool m_UseCallback;
	static bool m_DoNoSet;

	// These need to go away!
	friend FString C_GetMassCVarString (uint32_t filter, bool compact);
	friend void C_SerializeCVars(FSerializer& arc, const char* label, uint32_t filter);
	friend void C_ReadCVars (uint8_t **demo_p);
	friend void C_BackupCVars (void);
	friend FBaseCVar *FindCVar (const char *var_name, FBaseCVar **prev);
	friend FBaseCVar *FindCVarSub (const char *var_name, int namelen);
	friend void UnlatchCVars (void);
	friend void DestroyCVarsFlagged (uint32_t flags);
	friend void C_ArchiveCVars (FConfigFile *f, uint32_t filter);
	friend void C_SetCVarsToDefaults (void);
	friend void FilterCompactCVars (TArray<FBaseCVar *> &cvars, uint32_t filter);
	friend void C_DeinitConsole();
	friend void C_ListCVarsWithoutDescription();
};

// Returns a string with all cvars whose flags match filter. In compact mode,
// the cvar names are omitted to save space.
FString C_GetMassCVarString (uint32_t filter, bool compact=false);

// Writes all cvars that could effect demo sync to *demo_p. These are
// cvars that have either CVAR_SERVERINFO or CVAR_DEMOSAVE set.
void C_WriteCVars (uint8_t **demo_p, uint32_t filter, bool compact=false);

// Read all cvars from *demo_p and set them appropriately.
void C_ReadCVars (uint8_t **demo_p);

void C_InstallHandlers(ConsoleCallbacks* cb);

// Backup demo cvars. Called before a demo starts playing to save all
// cvars the demo might change.
void C_BackupCVars (void);

// Finds a named cvar
FBaseCVar *FindCVar (const char *var_name, FBaseCVar **prev);
FBaseCVar *FindCVarSub (const char *var_name, int namelen);

// Used for ACS and DECORATE.
FBaseCVar *GetCVar(int playernum, const char *cvarname);

// Create a new cvar with the specified name and type
FBaseCVar *C_CreateCVar(const char *var_name, ECVarType var_type, uint32_t flags);

// Called from G_InitNew()
void UnlatchCVars (void);

// Destroy CVars with the matching flags; called from CCMD(restart)
void DestroyCVarsFlagged (uint32_t flags);

// archive cvars to FILE f
void C_ArchiveCVars (FConfigFile *f, uint32_t filter);

// initialize cvars to default values after they are created
void C_SetCVarsToDefaults (void);

void FilterCompactCVars (TArray<FBaseCVar *> &cvars, uint32_t filter);

void C_DeinitConsole();

class FBoolCVar : public FBaseCVar
{
	friend class FxCVar;
public:
	FBoolCVar (const char *name, bool def, uint32_t flags, void (*callback)(FBoolCVar &)=NULL, const char* descr = nullptr);

	virtual ECVarType GetRealType () const;

	virtual UCVarValue GetGenericRep (ECVarType type) const;
	virtual UCVarValue GetFavoriteRep (ECVarType *type) const;
	virtual UCVarValue GetGenericRepDefault (ECVarType type) const;
	virtual UCVarValue GetFavoriteRepDefault (ECVarType *type) const;
	virtual void SetGenericRepDefault (UCVarValue value, ECVarType type);

	inline bool operator= (bool boolval)
		{ UCVarValue val; val.Bool = boolval; SetGenericRep (val, CVAR_Bool); return boolval; }
	inline operator bool () const { return Value; }
	inline bool operator *() const { return Value; }

protected:
	virtual void DoSet (UCVarValue value, ECVarType type);

	bool Value;
	bool DefaultValue;
};

class FIntCVar : public FBaseCVar
{
	friend class FxCVar;
public:
	FIntCVar (const char *name, int def, uint32_t flags, void (*callback)(FIntCVar &)=NULL, const char* descr = nullptr);

	virtual ECVarType GetRealType () const;

	virtual UCVarValue GetGenericRep (ECVarType type) const;
	virtual UCVarValue GetFavoriteRep (ECVarType *type) const;
	virtual UCVarValue GetGenericRepDefault (ECVarType type) const;
	virtual UCVarValue GetFavoriteRepDefault (ECVarType *type) const;
	virtual void SetGenericRepDefault (UCVarValue value, ECVarType type);

	int operator= (int intval)
		{ UCVarValue val; val.Int = intval; SetGenericRep (val, CVAR_Int); return intval; }
	inline operator int () const { return Value; }
	inline int operator *() const { return Value; }

protected:
	virtual void DoSet (UCVarValue value, ECVarType type);

	int Value;
	int DefaultValue;

	friend class FFlagCVar;
};

class FFloatCVar : public FBaseCVar
{
	friend class FxCVar;
public:
	FFloatCVar (const char *name, float def, uint32_t flags, void (*callback)(FFloatCVar &)=NULL, const char* descr = nullptr);

	virtual ECVarType GetRealType () const override;

	virtual UCVarValue GetGenericRep (ECVarType type) const override ;
	virtual UCVarValue GetFavoriteRep (ECVarType *type) const override;
	virtual UCVarValue GetGenericRepDefault (ECVarType type) const override;
	virtual UCVarValue GetFavoriteRepDefault (ECVarType *type) const override;
	virtual void SetGenericRepDefault (UCVarValue value, ECVarType type) override;
	const char *GetHumanString(int precision) const override;
	const char *GetHumanStringDefault(int precision) const override;

	float operator= (float floatval)
		{ UCVarValue val; val.Float = floatval; SetGenericRep (val, CVAR_Float); return floatval; }
	inline operator float () const { return Value; }
	inline float operator *() const { return Value; }

protected:
	virtual void DoSet (UCVarValue value, ECVarType type) override;

	float Value;
	float DefaultValue;
};

class FStringCVar : public FBaseCVar
{
	friend class FxCVar;
public:
	FStringCVar (const char *name, const char *def, uint32_t flags, void (*callback)(FStringCVar &)=NULL, const char* descr = nullptr);
	~FStringCVar ();

	virtual ECVarType GetRealType () const;

	virtual UCVarValue GetGenericRep (ECVarType type) const;
	virtual UCVarValue GetFavoriteRep (ECVarType *type) const;
	virtual UCVarValue GetGenericRepDefault (ECVarType type) const;
	virtual UCVarValue GetFavoriteRepDefault (ECVarType *type) const;
	virtual void SetGenericRepDefault (UCVarValue value, ECVarType type);

	const char *operator= (const char *stringrep)
		{ UCVarValue val; val.String = const_cast<char *>(stringrep); SetGenericRep (val, CVAR_String); return stringrep; }
	inline operator const char * () const { return mValue; }
	inline const char *operator *() const { return mValue; }

protected:
	virtual void DoSet (UCVarValue value, ECVarType type);

	FString mValue;
	FString mDefaultValue;
};

class FColorCVar : public FIntCVar
{
	friend class FxCVar;
public:
	FColorCVar (const char *name, int def, uint32_t flags, void (*callback)(FColorCVar &)=NULL, const char* descr = nullptr);

	virtual ECVarType GetRealType () const;

	virtual UCVarValue GetGenericRep (ECVarType type) const;
	virtual UCVarValue GetGenericRepDefault (ECVarType type) const;
	virtual void SetGenericRepDefault (UCVarValue value, ECVarType type);

	inline operator uint32_t () const { return Value; }
	inline uint32_t operator *() const { return Value; }

protected:
	virtual void DoSet (UCVarValue value, ECVarType type);
	
	static UCVarValue FromInt2 (int value, ECVarType type);
	static int ToInt2 (UCVarValue value, ECVarType type);
};

class FFlagCVar : public FBaseCVar
{
	friend class FxCVar;
public:
	FFlagCVar (const char *name, FIntCVar &realvar, uint32_t bitval, const char* descr = nullptr);

	virtual ECVarType GetRealType () const;

	virtual UCVarValue GetGenericRep (ECVarType type) const;
	virtual UCVarValue GetFavoriteRep (ECVarType *type) const;
	virtual UCVarValue GetGenericRepDefault (ECVarType type) const;
	virtual UCVarValue GetFavoriteRepDefault (ECVarType *type) const;
	virtual void SetGenericRepDefault (UCVarValue value, ECVarType type);

	bool operator= (bool boolval)
		{ UCVarValue val; val.Bool = boolval; SetGenericRep (val, CVAR_Bool); return boolval; }
	bool operator= (FFlagCVar &flag)
		{ UCVarValue val; val.Bool = !!flag; SetGenericRep (val, CVAR_Bool); return val.Bool; }
	inline operator int () const { return (ValueVar & BitVal); }
	inline int operator *() const { return (ValueVar & BitVal); }

protected:
	virtual void DoSet (UCVarValue value, ECVarType type);

	FIntCVar &ValueVar;
	uint32_t BitVal;
	int BitNum;
};

class FMaskCVar : public FBaseCVar
{
	friend class FxCVar;
public:
	FMaskCVar (const char *name, FIntCVar &realvar, uint32_t bitval, const char* descr = nullptr);

	virtual ECVarType GetRealType () const;

	virtual UCVarValue GetGenericRep (ECVarType type) const;
	virtual UCVarValue GetFavoriteRep (ECVarType *type) const;
	virtual UCVarValue GetGenericRepDefault (ECVarType type) const;
	virtual UCVarValue GetFavoriteRepDefault (ECVarType *type) const;
	virtual void SetGenericRepDefault (UCVarValue value, ECVarType type);

	inline operator int () const { return (ValueVar & BitVal) >> BitNum; }
	inline int operator *() const { return (ValueVar & BitVal) >> BitNum; }

protected:
	virtual void DoSet (UCVarValue value, ECVarType type);

	FIntCVar &ValueVar;
	uint32_t BitVal;
	int BitNum;
};

extern int cvar_defflags;

FBaseCVar *cvar_set (const char *var_name, const char *value);
FBaseCVar *cvar_forceset (const char *var_name, const char *value);

inline FBaseCVar *cvar_set (const char *var_name, const uint8_t *value) { return cvar_set (var_name, (const char *)value); }
inline FBaseCVar *cvar_forceset (const char *var_name, const uint8_t *value) { return cvar_forceset (var_name, (const char *)value); }



// Restore demo cvars. Called after demo playback to restore all cvars
// that might possibly have been changed during the course of demo playback.
void C_RestoreCVars (void);

void C_ForgetCVars (void);


#define CUSTOM_CVAR(type,name,def,flags) \
	static void cvarfunc_##name(F##type##CVar &); \
	F##type##CVar name (#name, def, flags, cvarfunc_##name); \
	static void cvarfunc_##name(F##type##CVar &self)

#define CUSTOM_CVAR_NAMED(type,name,cname,def,flags) \
	static void cvarfunc_##name(F##type##CVar &); \
	F##type##CVar name (#cname, def, flags, cvarfunc_##name); \
	static void cvarfunc_##name(F##type##CVar &self)

#define CVAR(type,name,def,flags) \
	F##type##CVar name (#name, def, flags);

#define EXTERN_CVAR(type,name) extern F##type##CVar name;

#define CUSTOM_CVARD(type,name,def,flags,descr) \
	static void cvarfunc_##name(F##type##CVar &); \
	F##type##CVar name (#name, def, flags, cvarfunc_##name, descr); \
	static void cvarfunc_##name(F##type##CVar &self)

#define CVARD(type,name,def,flags, descr) \
	F##type##CVar name (#name, def, flags, nullptr, descr);

#define CVARD_NAMED(type,name,varname,def,flags, descr) \
F##type##CVar varname (#name, def, flags, nullptr, descr);

#define CVAR_UNAMED(type,varname) \
F##type##CVar varname (nullptr, 0, 0, nullptr, nullptr);

extern FBaseCVar *CVars;

#endif //__C_CVARS_H__
