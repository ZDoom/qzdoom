#ifndef __SC_MAN_H__
#define __SC_MAN_H__

#include "zstring.h"
#include "name.h"
#include "basics.h"

struct VersionInfo
{
	uint16_t major;
	uint16_t minor;
	uint32_t revision;

	bool operator <=(const VersionInfo& o) const
	{
		return o.major > this->major || (o.major == this->major && o.minor > this->minor) || (o.major == this->major && o.minor == this->minor && o.revision >= this->revision);
	}
	bool operator >=(const VersionInfo& o) const
	{
		return o.major < this->major || (o.major == this->major && o.minor < this->minor) || (o.major == this->major && o.minor == this->minor && o.revision <= this->revision);
	}
	bool operator > (const VersionInfo& o) const
	{
		return o.major < this->major || (o.major == this->major && o.minor < this->minor) || (o.major == this->major && o.minor == this->minor && o.revision < this->revision);
	}
	bool operator < (const VersionInfo& o) const
	{
		return o.major > this->major || (o.major == this->major && o.minor > this->minor) || (o.major == this->major && o.minor == this->minor && o.revision > this->revision);
	}
	void operator=(const char* string);
};

// Cannot be a constructor because Lemon would puke on it.
inline VersionInfo MakeVersion(unsigned int ma, unsigned int mi, unsigned int re = 0)
{
	return{ (uint16_t)ma, (uint16_t)mi, (uint32_t)re };
}


class FScanner
{
public:
	struct SavedPos
	{
		const char *SavedScriptPtr;
		int SavedScriptLine;
	};

	// Methods ------------------------------------------------------
	FScanner();
	FScanner(const FScanner &other);
	FScanner(int lumpnum);
	~FScanner();

	FScanner &operator=(const FScanner &other);

	void Open(const char *lumpname);
	bool OpenFile(const char *filename);
	void OpenMem(const char *name, const char *buffer, int size);
	void OpenMem(const char *name, const TArray<uint8_t> &buffer)
	{
		OpenMem(name, (const char*)buffer.Data(), buffer.Size());
	}
	void OpenString(const char *name, FString buffer);
	void OpenLumpNum(int lump);
	void Close();
	void SetParseVersion(VersionInfo ver)
	{
		ParseVersion = ver;
	}

	void SetCMode(bool cmode);
	void SetEscape(bool esc);
	void SetStateMode(bool stately);
	void DisableStateOptions();
	const SavedPos SavePos();
	void RestorePos(const SavedPos &pos);

	static FString TokenName(int token, const char *string=NULL);

	bool GetString();
	void MustGetString();
	void MustGetStringName(const char *name);
	bool CheckString(const char *name);

	bool GetToken();
	void MustGetAnyToken();
	void TokenMustBe(int token);
	void MustGetToken(int token);
	bool CheckToken(int token);
	bool CheckTokenId(ENamedName id);

	bool GetNumber();
	void MustGetNumber();
	bool CheckNumber();

	bool GetFloat();
	void MustGetFloat();
	bool CheckFloat();
	
	// Token based variant
	bool CheckValue(bool allowfloat);
	void MustGetValue(bool allowfloat);
	bool CheckBoolToken();
	void MustGetBoolToken();

	void UnGet();

	bool Compare(const char *text);
	int MatchString(const char * const *strings, size_t stride = sizeof(char*));
	int MustMatchString(const char * const *strings, size_t stride = sizeof(char*));
	int GetMessageLine();

	void ScriptError(const char *message, ...) GCCPRINTF(2,3);
	void ScriptMessage(const char *message, ...) GCCPRINTF(2,3);

	bool isText();

	// Members ------------------------------------------------------
	char *String;
	int StringLen;
	int TokenType;
	int Number;
	int64_t BigNumber;
	double Float;
	int Line;
	bool End;
	bool Crossed;
	int LumpNum;
	FString ScriptName;

protected:
	void PrepareScript();
	void CheckOpen();
	bool ScanString(bool tokens);

	// Strings longer than this minus one will be dynamically allocated.
	static const int MAX_STRING_SIZE = 128;

	bool ScriptOpen;
	FString ScriptBuffer;
	const char *ScriptPtr;
	const char *ScriptEndPtr;
	char StringBuffer[MAX_STRING_SIZE];
	FString BigStringBuffer;
	bool AlreadyGot;
	int AlreadyGotLine;
	bool LastGotToken;
	const char *LastGotPtr;
	int LastGotLine;
	bool CMode;
	uint8_t StateMode;
	bool StateOptions;
	bool Escape;
	VersionInfo ParseVersion = { 0, 0, 0 };	// no ZScript extensions by default


	bool ScanValue(bool allowfloat);
};

enum
{
	TK_SequenceStart = 256,
#define xx(sym,str) sym,
#include "sc_man_tokens.h"
	TK_LastToken
};


//==========================================================================
//
//
//
//==========================================================================

enum
{
	MSG_WARNING,
	MSG_FATAL,
	MSG_ERROR,
	MSG_OPTERROR,
	MSG_DEBUGERROR,
	MSG_DEBUGWARN,
	MSG_DEBUGMSG,
	MSG_LOG,
	MSG_DEBUGLOG,
	MSG_MESSAGE
};

//==========================================================================
//
// a class that remembers a parser position
//
//==========================================================================

struct FScriptPosition
{
	static int WarnCounter;
	static int ErrorCounter;
	static bool StrictErrors;
	static int Developer;
	static bool errorout;
	FName FileName;
	int ScriptLine;

	FScriptPosition()
	{
		FileName = NAME_None;
		ScriptLine=0;
	}
	FScriptPosition(const FScriptPosition &other) = default;
	FScriptPosition(FString fname, int line);
	FScriptPosition(FScanner &sc);
	FScriptPosition &operator=(const FScriptPosition &other) = default;
	FScriptPosition &operator=(FScanner &sc);
	void Message(int severity, const char *message,...) const GCCPRINTF(3,4);
	static void ResetErrorCounter()
	{
		WarnCounter = 0;
		ErrorCounter = 0;
	}
};

int ParseHex(const char* hex, FScriptPosition* sc);


#endif //__SC_MAN_H__
