
struct _ native	// These are the global variables, the struct is only here to avoid extending the parser for this.
{
	native readonly Array<class> AllClasses;
	native readonly Array<class<Actor> > AllActorClasses;
	native readonly Array<@PlayerClass> PlayerClasses;
	native readonly Array<@PlayerSkin> PlayerSkins;
	native readonly Array<@Team> Teams;
	native int validcount;
	native readonly bool multiplayer;
	native @KeyBindings Bindings;
	native @KeyBindings AutomapBindings;
	native play @DehInfo deh;
	native readonly @GameInfoStruct gameinfo;
	native readonly ui bool netgame;

	native readonly bool automapactive;
	native readonly uint gameaction;
	native readonly int gamestate;
	native readonly TextureID skyflatnum;
	native readonly Font smallfont;
	native readonly Font smallfont2;
	native readonly Font bigfont;
	native readonly Font confont;
	native readonly Font NewConsoleFont;
	native readonly Font NewSmallFont;
	native readonly Font AlternativeSmallFont;
	native readonly Font OriginalSmallFont;
	native readonly Font OriginalBigFont;
	native readonly Font intermissionfont;
	native readonly int CleanXFac;
	native readonly int CleanYFac;
	native readonly int CleanWidth;
	native readonly int CleanHeight;
	native readonly int CleanXFac_1;
	native readonly int CleanYFac_1;
	native readonly int CleanWidth_1;
	native readonly int CleanHeight_1;
	native ui int menuactive;
	native readonly @FOptionMenuSettings OptionMenuSettings;
	native readonly int gametic;
	native readonly bool demoplayback;
	native ui int BackbuttonTime;
	native ui float BackbuttonAlpha;
	native readonly int Net_Arbitrator;
	native ui BaseStatusBar StatusBar;
	native readonly Weapon WP_NOCHANGE;
	deprecated("3.8", "Use Actor.isFrozen() or Level.isFrozen() instead") native readonly bool globalfreeze;
	native int LocalViewPitch;
	native readonly @MusPlayingInfo musplaying;
	native readonly bool generic_ui;

// sandbox state in multi-level setups:

	native play @PlayerInfo players[MAXPLAYERS];
	native readonly bool playeringame[MAXPLAYERS];
	native readonly int consoleplayer;
	native play LevelLocals Level;

}

struct MusPlayingInfo native
{
	native String name;
	native int baseorder;
	native bool loop;
};

struct TexMan
{
	enum EUseTypes
	{
		Type_Any,
		Type_Wall,
		Type_Flat,
		Type_Sprite,
		Type_WallPatch,
		Type_Build,
		Type_SkinSprite,
		Type_Decal,
		Type_MiscPatch,
		Type_FontChar,
		Type_Override,	// For patches between TX_START/TX_END
		Type_Autopage,	// Automap background - used to enable the use of FAutomapTexture
		Type_SkinGraphic,
		Type_Null,
		Type_FirstDefined,
	};

	enum EFlags
	{
		TryAny = 1,
		Overridable = 2,
		ReturnFirst = 4,
		AllowSkins = 8,
		ShortNameOnly = 16,
		DontCreate = 32,
		Localize = 64
	};
	
	enum ETexReplaceFlags
	{
		NOT_BOTTOM			= 1,
		NOT_MIDDLE			= 2,
		NOT_TOP				= 4,
		NOT_FLOOR			= 8,
		NOT_CEILING			= 16,
		NOT_WALL			= 7,
		NOT_FLAT			= 24
	};

	native static TextureID CheckForTexture(String name, int usetype, int flags = TryAny);
	deprecated("3.8", "Use Level.ReplaceTextures() instead") static void ReplaceTextures(String from, String to, int flags)
	{
		level.ReplaceTextures(from, to, flags);
	}
	native static String GetName(TextureID tex);
	native static int, int GetSize(TextureID tex);
	native static Vector2 GetScaledSize(TextureID tex);
	native static Vector2 GetScaledOffset(TextureID tex);
	native static int CheckRealHeight(TextureID tex);
	native static bool OkForLocalization(TextureID patch, String textSubstitute);

	native static void SetCameraToTexture(Actor viewpoint, String texture, double fov);
}

enum DrawTextureTags
{
	TAG_USER = (1<<30),
	DTA_Base = TAG_USER + 5000,
	DTA_DestWidth,		// width of area to draw to
	DTA_DestHeight,		// height of area to draw to
	DTA_Alpha,			// alpha value for translucency
	DTA_FillColor,		// color to stencil onto the destination (RGB is the color for truecolor drawers, A is the palette index for paletted drawers)
	DTA_TranslationIndex, // translation table to recolor the source
	DTA_AlphaChannel,	// bool: the source is an alpha channel; used with DTA_FillColor
	DTA_Clean,			// bool: scale texture size and position by CleanXfac and CleanYfac
	DTA_320x200,		// bool: scale texture size and position to fit on a virtual 320x200 screen
	DTA_Bottom320x200,	// bool: same as DTA_320x200 but centers virtual screen on bottom for 1280x1024 targets
	DTA_CleanNoMove,	// bool: like DTA_Clean but does not reposition output position
	DTA_CleanNoMove_1,	// bool: like DTA_CleanNoMove, but uses Clean[XY]fac_1 instead
	DTA_FlipX,			// bool: flip image horizontally	//FIXME: Does not work with DTA_Window(Left|Right)
	DTA_ShadowColor,	// color of shadow
	DTA_ShadowAlpha,	// alpha of shadow
	DTA_Shadow,			// set shadow color and alphas to defaults
	DTA_VirtualWidth,	// pretend the canvas is this wide
	DTA_VirtualHeight,	// pretend the canvas is this tall
	DTA_TopOffset,		// override texture's top offset
	DTA_LeftOffset,		// override texture's left offset
	DTA_CenterOffset,	// bool: override texture's left and top offsets and set them for the texture's middle
	DTA_CenterBottomOffset,// bool: override texture's left and top offsets and set them for the texture's bottom middle
	DTA_WindowLeft,		// don't draw anything left of this column (on source, not dest)
	DTA_WindowRight,	// don't draw anything at or to the right of this column (on source, not dest)
	DTA_ClipTop,		// don't draw anything above this row (on dest, not source)
	DTA_ClipBottom,		// don't draw anything at or below this row (on dest, not source)
	DTA_ClipLeft,		// don't draw anything to the left of this column (on dest, not source)
	DTA_ClipRight,		// don't draw anything at or to the right of this column (on dest, not source)
	DTA_Masked,			// true(default)=use masks from texture, false=ignore masks
	DTA_HUDRules,		// use fullscreen HUD rules to position and size textures
	DTA_HUDRulesC,		// only used internally for marking HUD_HorizCenter
	DTA_KeepRatio,		// doesn't adjust screen size for DTA_Virtual* if the aspect ratio is not 4:3
	DTA_RenderStyle,	// same as render style for actors
	DTA_ColorOverlay,	// DWORD: ARGB to overlay on top of image; limited to black for software
	DTA_Internal1,
	DTA_Internal2,
	DTA_Desaturate,		// explicit desaturation factor (does not do anything in Legacy OpenGL)
	DTA_Fullscreen,		// Draw image fullscreen (same as DTA_VirtualWidth/Height with graphics size.)

	// floating point duplicates of some of the above:
	DTA_DestWidthF,
	DTA_DestHeightF,
	DTA_TopOffsetF,
	DTA_LeftOffsetF,
	DTA_VirtualWidthF,
	DTA_VirtualHeightF,
	DTA_WindowLeftF,
	DTA_WindowRightF,

	// For DrawText calls only:
	DTA_TextLen,		// stop after this many characters, even if \0 not hit
	DTA_CellX,			// horizontal size of character cell
	DTA_CellY,			// vertical size of character cell

	DTA_Color,
	DTA_FlipY,			// bool: flip image vertically
	DTA_SrcX,			// specify a source rectangle (this supersedes the poorly implemented DTA_WindowLeft/Right
	DTA_SrcY,
	DTA_SrcWidth,
	DTA_SrcHeight,
	DTA_LegacyRenderStyle,	// takes an old-style STYLE_* constant instead of an FRenderStyle
	DTA_Internal3,
	DTA_Spacing,			// Strings only: Additional spacing between characters
	DTA_Monospace,			// Strings only: Use a fixed distance between characters.

	DTA_FullscreenEx,		// advanced fullscreen control.
};

class Shape2DTransform : Object native
{
	native void Clear();
	native void Rotate(double angle);
	native void Scale(Vector2 scaleVec);
	native void Translate(Vector2 translateVec);
}

class Shape2D : Object native
{
	enum EClearWhich
	{
		C_Verts = 1,
		C_Coords = 2,
		C_Indices = 4,
	};

	native void SetTransform(Shape2DTransform transform);

	native void Clear( int which = C_Verts|C_Coords|C_Indices );
	native void PushVertex( Vector2 v );
	native void PushCoord( Vector2 c );
	native void PushTriangle( int a, int b, int c );
}

struct Screen native
{
	native static Color PaletteColor(int index);
	native static int GetWidth();
	native static int GetHeight();
	native static void Clear(int left, int top, int right, int bottom, Color color, int palcolor = -1);
	native static void Dim(Color col, double amount, int x, int y, int w, int h);

	native static vararg void DrawTexture(TextureID tex, bool animate, double x, double y, ...);
	native static vararg void DrawShape(TextureID tex, bool animate, Shape2D s, ...);
	native static vararg void DrawChar(Font font, int normalcolor, double x, double y, int character, ...);
	native static vararg void DrawText(Font font, int normalcolor, double x, double y, String text, ...);
	native static void DrawLine(int x0, int y0, int x1, int y1, Color color, int alpha = 255);
	native static void DrawThickLine(int x0, int y0, int x1, int y1, double thickness, Color color, int alpha = 255);
	native static void DrawFrame(int x, int y, int w, int h);
	native static Vector2, Vector2 VirtualToRealCoords(Vector2 pos, Vector2 size, Vector2 vsize, bool vbottom=false, bool handleaspect=true);
	native static double GetAspectRatio();
	native static void SetClipRect(int x, int y, int w, int h);
	native static void ClearClipRect();
	native static int, int, int, int GetClipRect();
	native static int, int, int, int GetViewWindow();
	
	
	// This is a leftover of the abandoned Inventory.DrawPowerup method.
	deprecated("2.5", "Use StatusBar.DrawTexture() instead") static ui void DrawHUDTexture(TextureID tex, double x, double y)
	{
		statusBar.DrawTexture(tex, (x, y), BaseStatusBar.DI_SCREEN_RIGHT_TOP, 1., (32, 32));
	}
}

struct Font native
{
	enum EColorRange
	{
		CR_UNDEFINED = -1,
		CR_BRICK,
		CR_TAN,
		CR_GRAY,
		CR_GREY = CR_GRAY,
		CR_GREEN,
		CR_BROWN,
		CR_GOLD,
		CR_RED,
		CR_BLUE,
		CR_ORANGE,
		CR_WHITE,
		CR_YELLOW,
		CR_UNTRANSLATED,
		CR_BLACK,
		CR_LIGHTBLUE,
		CR_CREAM,
		CR_OLIVE,
		CR_DARKGREEN,
		CR_DARKRED,
		CR_DARKBROWN,
		CR_PURPLE,
		CR_DARKGRAY,
		CR_CYAN,
		CR_ICE,
		CR_FIRE,
		CR_SAPPHIRE,
		CR_TEAL,
		NUM_TEXT_COLORS
	};
	
	const TEXTCOLOR_BRICK			= "\034A";
	const TEXTCOLOR_TAN				= "\034B";
	const TEXTCOLOR_GRAY			= "\034C";
	const TEXTCOLOR_GREY			= "\034C";
	const TEXTCOLOR_GREEN			= "\034D";
	const TEXTCOLOR_BROWN			= "\034E";
	const TEXTCOLOR_GOLD			= "\034F";
	const TEXTCOLOR_RED				= "\034G";
	const TEXTCOLOR_BLUE			= "\034H";
	const TEXTCOLOR_ORANGE			= "\034I";
	const TEXTCOLOR_WHITE			= "\034J";
	const TEXTCOLOR_YELLOW			= "\034K";
	const TEXTCOLOR_UNTRANSLATED	= "\034L";
	const TEXTCOLOR_BLACK			= "\034M";
	const TEXTCOLOR_LIGHTBLUE		= "\034N";
	const TEXTCOLOR_CREAM			= "\034O";
	const TEXTCOLOR_OLIVE			= "\034P";
	const TEXTCOLOR_DARKGREEN		= "\034Q";
	const TEXTCOLOR_DARKRED			= "\034R";
	const TEXTCOLOR_DARKBROWN		= "\034S";
	const TEXTCOLOR_PURPLE			= "\034T";
	const TEXTCOLOR_DARKGRAY		= "\034U";
	const TEXTCOLOR_CYAN			= "\034V";
	const TEXTCOLOR_ICE				= "\034W";
	const TEXTCOLOR_FIRE			= "\034X";
	const TEXTCOLOR_SAPPHIRE		= "\034Y";
	const TEXTCOLOR_TEAL			= "\034Z";

	const TEXTCOLOR_NORMAL			= "\034-";
	const TEXTCOLOR_BOLD			= "\034+";

	const TEXTCOLOR_CHAT			= "\034*";
	const TEXTCOLOR_TEAMCHAT		= "\034!";
	

	native int GetCharWidth(int code);
	native int StringWidth(String code);
	native int GetMaxAscender(String code);
	native bool CanPrint(String code);
	native int GetHeight();
	native int GetDisplacement();
	native String GetCursor();

	native static int FindFontColor(Name color);
	native double GetBottomAlignOffset(int code);
	native static Font FindFont(Name fontname);
	native static Font GetFont(Name fontname);
	native BrokenLines BreakLines(String text, int maxlen);
}

struct Translation version("2.4")
{
	Color colors[256];
	
	native int AddTranslation();
	native static bool SetPlayerTranslation(int group, int num, int plrnum, PlayerClass pclass);
	native static int GetID(Name transname);
	static int MakeID(int group, int num)
	{
		return (group << 16) + num;
	}
}

struct Console native
{
	native static void HideConsole();
	native static void MidPrint(Font fontname, string textlabel, bool bold = false);
	native static vararg void Printf(string fmt, ...);
}

struct DamageTypeDefinition native
{
	native static bool IgnoreArmor(Name type);
}

struct CVar native
{
	enum ECVarType
	{
		CVAR_Bool,
		CVAR_Int,
		CVAR_Float,
		CVAR_String,
		CVAR_Color,
	};

	native static CVar FindCVar(Name name);
	native static CVar GetCVar(Name name, PlayerInfo player = null);
	bool GetBool() { return GetInt(); }
	native int GetInt();
	native double GetFloat();
	native String GetString();
	void SetBool(bool b) { SetInt(b); }
	native void SetInt(int v);
	native void SetFloat(double v);
	native void SetString(String s);
	native int GetRealType();
	native int ResetToDefault();
}

struct GIFont version("2.4")
{
	Name fontname;
	Name color;
};

struct GameInfoStruct native
{
	// will be extended as needed.
	native Name backpacktype;
	native double Armor2Percent;
	native String ArmorIcon1;
	native String ArmorIcon2;
	native int gametype;
	native bool norandomplayerclass;
	native Array<Name> infoPages;
	native String mBackButton;
	native GIFont mStatscreenMapNameFont;
	native GIFont mStatscreenEnteringFont;
	native GIFont mStatscreenFinishedFont;
	native GIFont mStatscreenContentFont;
	native GIFont mStatscreenAuthorFont;
	native double gibfactor;
	native bool intermissioncounter;
	native Name mSliderColor;
	native Color defaultbloodcolor;
	native double telefogheight;
	native int defKickback;
	native int defaultdropstyle;
	native TextureID healthpic;
	native TextureID berserkpic;
	native double normforwardmove[2];
	native double normsidemove[2];
}

class Object native
{
	const TICRATE = 35;
	native bool bDestroyed;

	// These must be defined in some class, so that the compiler can find them. Object is just fine, as long as they are private to external code.
	private native static Object BuiltinNew(Class<Object> cls, int outerclass, int compatibility);
	private native static Object BuiltinNewDoom(Class<Object> cls, int outerclass, int compatibility);
	private native static int BuiltinRandom(voidptr rng, int min, int max);
	private native static double BuiltinFRandom(voidptr rng, double min, double max);
	private native static int BuiltinRandom2(voidptr rng, int mask);
	private native static void BuiltinRandomSeed(voidptr rng, int seed);
	private native static int BuiltinCallLineSpecial(int special, Actor activator, int arg1, int arg2, int arg3, int arg4, int arg5);
	private native static Class<Object> BuiltinNameToClass(Name nm, Class<Object> filter);
	private native static Object BuiltinClassCast(Object inptr, Class<Object> test);
	
	// These really should be global functions...
	native static String G_SkillName();
	native static int G_SkillPropertyInt(int p);
	native static double G_SkillPropertyFloat(int p);
	deprecated("3.8", "Use Level.PickDeathMatchStart() instead") static vector3, int G_PickDeathmatchStart()
	{
		return level.PickDeathmatchStart();
	}
	deprecated("3.8", "Use Level.PickPlayerStart() instead") static vector3, int G_PickPlayerStart(int pnum, int flags = 0)
	{
		return level.PickPlayerStart(pnum, flags);
	}
	deprecated("4.3", "Use S_StartSound() instead") native static void S_Sound (Sound sound_id, int channel, float volume = 1, float attenuation = ATTN_NORM, float pitch = 0.0);
	native static void S_StartSound (Sound sound_id, int channel, int flags = 0, float volume = 1, float attenuation = ATTN_NORM, float pitch = 0.0, float startTime = 0.0);
	native static void S_PauseSound (bool notmusic, bool notsfx);
	native static void S_ResumeSound (bool notsfx);
	native static bool S_ChangeMusic(String music_name, int order = 0, bool looping = true, bool force = false);
	native static float S_GetLength(Sound sound_id);
	native static void MarkSound(Sound snd);
	native static uint BAM(double angle);
	native static void SetMusicVolume(float vol);
	native static uint MSTime();
	native vararg static void ThrowAbortException(String fmt, ...);

	native virtualscope void Destroy();

	// This does not call into the native method of the same name to avoid problems with objects that get garbage collected late on shutdown.
	virtual virtualscope void OnDestroy() {}
}

class BrokenLines : Object native version("2.4")
{
	native int Count();
	native int StringWidth(int line);
	native String StringAt(int line);
}

class Thinker : Object native play
{
	enum EStatnums
	{
 		// Thinkers that don't actually think
		STAT_INFO,								// An info queue
		STAT_DECAL,								// A decal
		STAT_AUTODECAL,							// A decal that can be automatically deleted
		STAT_CORPSEPOINTER,						// An entry in Hexen's corpse queue
		STAT_TRAVELLING,						// An actor temporarily travelling to a new map
		STAT_STATIC,

		// Thinkers that do think
		STAT_FIRST_THINKING=32,
		STAT_SCROLLER=STAT_FIRST_THINKING,		// A DScroller thinker
		STAT_PLAYER,							// A player actor
		STAT_BOSSTARGET,						// A boss brain target
		STAT_LIGHTNING,							// The lightning thinker
		STAT_DECALTHINKER,						// An object that thinks for a decal
		STAT_INVENTORY,							// An inventory item
		STAT_LIGHT,								// A sector light effect
		STAT_LIGHTTRANSFER,						// A sector light transfer. These must be ticked after the light effects.
		STAT_EARTHQUAKE,						// Earthquake actors
		STAT_MAPMARKER,							// Map marker actors
		STAT_DLIGHT,							// Dynamic lights

		STAT_USER = 70,
		STAT_USER_MAX = 90,

		STAT_DEFAULT = 100,						// Thinkers go here unless specified otherwise.
		STAT_SECTOREFFECT,						// All sector effects that cause floor and ceiling movement
		STAT_ACTORMOVER,						// actor movers
		STAT_SCRIPTS,							// The ACS thinker. This is to ensure that it can't tick before all actors called PostBeginPlay
		STAT_BOT,								// Bot thinker
		MAX_STATNUM = 127
	}


	native LevelLocals Level;
	
	virtual native void Tick();
	virtual native void PostBeginPlay();
	native void ChangeStatNum(int stat);
	
	static clearscope int Tics2Seconds(int tics)
	{
		return int(tics / TICRATE);
	}

}

class ThinkerIterator : Object native
{
	native static ThinkerIterator Create(class<Object> type = "Actor", int statnum=Thinker.MAX_STATNUM+1);
	native Thinker Next(bool exact = false);
	native void Reinit();
}

class ActorIterator : Object native
{
	deprecated("3.8", "Use Level.CreateActorIterator() instead") static ActorIterator Create(int tid, class<Actor> type = "Actor")
	{
		return level.CreateActorIterator(tid, type);
	}
	native Actor Next();
	native void Reinit();
}

class BlockThingsIterator : Object native
{
	native Actor thing;
	native Vector3 position;
	native int portalflags;
	
	native static BlockThingsIterator Create(Actor origin, double checkradius = -1, bool ignorerestricted = false);
	native static BlockThingsIterator CreateFromPos(double checkx, double checky, double checkz, double checkh, double checkradius, bool ignorerestricted);
	native bool Next();
}

class BlockLinesIterator : Object native
{
	native Line CurLine;
	native Vector3 position;
	native int portalflags;
	
	native static BlockLinesIterator Create(Actor origin, double checkradius = -1);
	native static BlockLinesIterator CreateFromPos(Vector3 pos, double checkh, double checkradius, Sector sec = null);
	native bool Next();
}

enum ETraceStatus
{
	TRACE_Stop,		// stop the trace, returning this hit
	TRACE_Continue,		// continue the trace, returning this hit if there are none further along
	TRACE_Skip,		// continue the trace; do not return this hit
	TRACE_Abort		// stop the trace, returning no hits
}

enum ETraceFlags
{
	TRACE_NoSky		= 0x0001,	// Hitting the sky returns TRACE_HitNone
	//TRACE_PCross		= 0x0002,	// Trigger SPAC_PCROSS lines
	//TRACE_Impact		= 0x0004,	// Trigger SPAC_IMPACT lines
	TRACE_PortalRestrict	= 0x0008,	// Cannot go through portals without a static link offset.
	TRACE_ReportPortals	= 0x0010,	// Report any portal crossing to the TraceCallback
	//TRACE_3DCallback	= 0x0020,	// [ZZ] use TraceCallback to determine whether we need to go through a line to do 3D floor check, or not. without this, only line flag mask is used
	TRACE_HitSky		= 0x0040	// Hitting the sky returns TRACE_HasHitSky
}


enum ETraceResult
{
	TRACE_HitNone,
	TRACE_HitFloor,
	TRACE_HitCeiling,
	TRACE_HitWall,
	TRACE_HitActor,
	TRACE_CrossingPortal,
	TRACE_HasHitSky
}

enum ELineTier
{
	TIER_Middle,
	TIER_Upper,
	TIER_Lower,
	TIER_FFloor
}

struct TraceResults native
{
	native Sector HitSector; // originally called "Sector". cannot be named like this in ZScript.
	native TextureID HitTexture;
	native vector3 HitPos;
	native vector3 HitVector;
	native vector3 SrcFromTarget;
	native double SrcAngleFromTarget;

	native double Distance;
	native double Fraction;

	native Actor HitActor;		// valid if hit an actor. // originally called "Actor".

	native Line HitLine;		// valid if hit a line // originally called "Line".
	native uint8 Side;
	native uint8 Tier;		// see Tracer.ELineTier
	native bool unlinked;		// passed through a portal without static offset.

	native ETraceResult HitType;
	native F3DFloor ffloor;

	native Sector CrossedWater;		// For Boom-style, Transfer_Heights-based deep water
	native vector3 CrossedWaterPos;	// remember the position so that we can use it for spawning the splash
	native Sector Crossed3DWater;		// For 3D floor-based deep water
	native vector3 Crossed3DWaterPos;
}

class LineTracer : Object native
{
	native @TraceResults Results;
	native bool Trace(vector3 start, Sector sec, vector3 direction, double maxDist, ETraceFlags traceFlags);

	virtual ETraceStatus TraceCallback()
	{
		// Normally you would examine Results.HitType (for ETraceResult), and determine either:
		//  - stop tracing and return the entity that was found (return TRACE_Stop)
		//  - ignore some object, like noclip, e.g. only count solid walls and floors, and ignore actors (return TRACE_Skip)
		//  - find last object of some type (return TRACE_Continue)
		//  - stop tracing entirely and assume we found nothing (return TRACE_Abort)
		// TRACE_Abort and TRACE_Continue are of limited use in scripting.

		return TRACE_Stop; // default callback returns first hit, whatever it is.
	}
}

struct DropItem native
{
	native readonly DropItem Next;
	native readonly name Name;
	native readonly int Probability;
	native readonly int Amount;
}

struct LevelLocals native
{
	enum EUDMF
	{
		UDMF_Line,
		UDMF_Side,
		UDMF_Sector,
		//UDMF_Thing // not implemented
	};
	
	const CLUSTER_HUB = 0x00000001;	// Cluster uses hub behavior


	native Array<@Sector> Sectors;
	native Array<@Line> Lines;
	native Array<@Side> Sides;
	native readonly Array<@Vertex> Vertexes;
	native internal Array<@SectorPortal> SectorPortals;
	
	native readonly int time;
	native readonly int maptime;
	native readonly int totaltime;
	native readonly int starttime;
	native readonly int partime;
	native readonly int sucktime;
	native readonly int cluster;
	native readonly int clusterflags;
	native readonly int levelnum;
	native readonly String LevelName;
	native readonly String MapName;
	native String NextMap;
	native String NextSecretMap;
	native readonly String F1Pic;
	native readonly int maptype;
	native readonly String AuthorName;
	native readonly String Music;
	native readonly int musicorder;
	native readonly TextureID skytexture1;
	native readonly TextureID skytexture2;
	native float skyspeed1;
	native float skyspeed2;
	native int total_secrets;
	native int found_secrets;
	native int total_items;
	native int found_items;
	native int total_monsters;
	native int killed_monsters;
	native play double gravity;
	native play double aircontrol;
	native play double airfriction;
	native play int airsupply;
	native readonly double teamdamage;
	native readonly bool noinventorybar;
	native readonly bool monsterstelefrag;
	native readonly bool actownspecial;
	native readonly bool sndseqtotalctrl;
	native bool allmap;
	native readonly bool missilesactivateimpact;
	native readonly bool monsterfallingdamage;
	native readonly bool checkswitchrange;
	native readonly bool polygrind;
	native readonly bool nomonsters;
	native readonly bool allowrespawn;
	deprecated("3.8", "Use Level.isFrozen() instead") native bool frozen;
	native readonly bool infinite_flight;
	native readonly bool no_dlg_freeze;
	native readonly bool keepfullinventory;
	native readonly bool removeitems;
	native readonly int fogdensity;
	native readonly int outsidefogdensity;
	native readonly int skyfog;
	native readonly float pixelstretch;
	native readonly float MusicVolume;
	native name deathsequence;
	native readonly int compatflags;
	native readonly int compatflags2;
// level_info_t *info cannot be done yet.

	native String GetUDMFString(int type, int index, Name key);
	native int GetUDMFInt(int type, int index, Name key);
	native double GetUDMFFloat(int type, int index, Name key);
	native play int ExecuteSpecial(int special, Actor activator, line linedef, bool lineside, int arg1 = 0, int arg2 = 0, int arg3 = 0, int arg4 = 0, int arg5 = 0);
	native void GiveSecret(Actor activator, bool printmsg = true, bool playsound = true);
	native void StartSlideshow(Name whichone = 'none');
	native static void MakeScreenShot();
	native static void MakeAutoSave();
	native void WorldDone();
	deprecated("3.8", "This function does nothing") static void RemoveAllBots(bool fromlist) { /* intentionally left as no-op. */ }
	native ui Vector2 GetAutomapPosition();
	native void SetInterMusic(String nextmap);
	native String FormatMapName(int mapnamecolor);
	native bool IsJumpingAllowed() const;
	native bool IsCrouchingAllowed() const;
	native bool IsFreelookAllowed() const;
	native void StartIntermission(Name type, int state) const;
	native play SpotState GetSpotState(bool create = true);
	native int FindUniqueTid(int start = 0, int limit = 0);
	native uint GetSkyboxPortal(Actor actor);
	native void ReplaceTextures(String from, String to, int flags);
    clearscope native HealthGroup FindHealthGroup(int id);
	native vector3, int PickDeathmatchStart();
	native vector3, int PickPlayerStart(int pnum, int flags = 0);
	native int isFrozen() const;
	native void setFrozen(bool on);

	native clearscope Sector PointInSector(Vector2 pt) const;

	native clearscope bool IsPointInLevel(vector3 p) const;
	deprecated("3.8", "Use Level.IsPointInLevel() instead") clearscope static bool IsPointInMap(vector3 p)
	{
		return level.IsPointInLevel(p);
	}

	native clearscope vector2 Vec2Diff(vector2 v1, vector2 v2) const;
	native clearscope vector3 Vec3Diff(vector3 v1, vector3 v2) const;
	native clearscope vector3 SphericalCoords(vector3 viewpoint, vector3 targetPos, vector2 viewAngles = (0, 0), bool absolute = false) const;
	
	native clearscope vector2 Vec2Offset(vector2 pos, vector2 dir, bool absolute = false) const;
	native clearscope vector3 Vec2OffsetZ(vector2 pos, vector2 dir, double atz, bool absolute = false) const;
	native clearscope vector3 Vec3Offset(vector3 pos, vector3 dir, bool absolute = false) const;

	native String GetChecksum() const;

	native void ChangeSky(TextureID sky1, TextureID sky2 );

	native SectorTagIterator CreateSectorTagIterator(int tag, line defline = null);
	native LineIdIterator CreateLineIdIterator(int tag);
	native ActorIterator CreateActorIterator(int tid, class<Actor> type = "Actor");

	String TimeFormatted(bool totals = false)
	{
		int sec = Thinker.Tics2Seconds(totals? totaltime : time); 
		return String.Format("%02d:%02d:%02d", sec / 3600, (sec % 3600) / 60, sec % 60);
	}

	native play bool CreateCeiling(sector sec, int type, line ln, double speed, double speed2, double height = 0, int crush = -1, int silent = 0, int change = 0, int crushmode = 0 /*Floor.crushDoom*/);
	native play bool CreateFloor(sector sec, int floortype, line ln, double speed, double height = 0, int crush = -1, int change = 0, bool crushmode = false, bool hereticlower = false);

	native void ExitLevel(int position, bool keepFacing);
	native void SecretExitLevel(int position);
}

struct StringTable native
{
	native static String Localize(String val, bool prefixed = true);
}

// a few values of this need to be readable by the play code.
// Most are handled at load time and are omitted here.
struct DehInfo native
{
	native readonly int MaxSoulsphere;
	native readonly uint8 ExplosionStyle;
	native readonly double ExplosionAlpha;
	native readonly int NoAutofreeze;
	native readonly int BFGCells;
	native readonly int BlueAC;
	native readonly int MaxHealth;
}

struct State native
{
	native readonly State NextState;
	native readonly int sprite;
	native readonly int16 Tics;
	native readonly uint16 TicRange;
	native readonly uint8 Frame;		
	native readonly uint8 UseFlags;	
	native readonly int Misc1;
	native readonly int Misc2;
	native readonly uint16 bSlow;
	native readonly uint16 bFast;
	native readonly bool bFullbright;
	native readonly bool bNoDelay;
	native readonly bool bSameFrame;
	native readonly bool bCanRaise;
	native readonly bool bDehacked;
	
	native int DistanceTo(state other);
	native bool ValidateSpriteFrame();
	native TextureID, bool, Vector2 GetSpriteTexture(int rotation, int skin = 0, Vector2 scale = (0,0));
	native bool InStateSequence(State base);
}

struct Wads
{
	enum WadNamespace
	{
		ns_hidden = -1,

		ns_global = 0,
		ns_sprites,
		ns_flats,
		ns_colormaps,
		ns_acslibrary,
		ns_newtextures,
		ns_bloodraw,
		ns_bloodsfx,
		ns_bloodmisc,
		ns_strifevoices,
		ns_hires,
		ns_voxels,

		ns_specialzipdirectory,
		ns_sounds,
		ns_patches,
		ns_graphics,
		ns_music,

		ns_firstskin,
	}

	enum FindLumpNamespace
	{
		GlobalNamespace = 0,
		AnyNamespace = 1,
	}

	native static int CheckNumForName(string name, int ns, int wadnum = -1, bool exact = false);
	native static int CheckNumForFullName(string name);
	native static int FindLump(string name, int startlump = 0, FindLumpNamespace ns = GlobalNamespace);
	native static string ReadLump(int lump);

	native static int GetNumLumps();
	native static string GetLumpName(int lump);
	native static string GetLumpFullName(int lump);
	native static int GetLumpNamespace(int lump);
}

struct TerrainDef native
{
	native Name TerrainName;
	native int Splash;
	native int DamageAmount;
	native Name DamageMOD;
	native int DamageTimeMask;
	native double FootClip;
	native float StepVolume;
	native int WalkStepTics;
	native int RunStepTics;
	native Sound LeftStepSound;
	native Sound RightStepSound;
	native bool IsLiquid;
	native bool AllowProtection;
	native bool DamageOnLand;
	native double Friction;
	native double MoveFactor;
};

enum EPickStart
{
	PPS_FORCERANDOM			= 1,
	PPS_NOBLOCKINGCHECK		= 2,
}

enum EmptyTokenType
{
	TOK_SKIPEMPTY = 0,
	TOK_KEEPEMPTY = 1,
}

// Although String is a builtin type, this is a convenient way to attach methods to it.
struct StringStruct native
{
	native static vararg String Format(String fmt, ...);
	native vararg void AppendFormat(String fmt, ...);

	native void Replace(String pattern, String replacement);
	native String Left(int len) const;
	native String Mid(int pos = 0, int len = 2147483647) const;
	native void Truncate(int newlen);
	native void Remove(int index, int remlen);
	deprecated("4.1", "use Left() or Mid() instead") native String CharAt(int pos) const;
	deprecated("4.1", "use ByteAt() instead") native int CharCodeAt(int pos) const;
	native int ByteAt(int pos) const;
	native String Filter();
	native int IndexOf(String substr, int startIndex = 0) const;
	deprecated("3.5.1", "use RightIndexOf() instead") native int LastIndexOf(String substr, int endIndex = 2147483647) const;
	native int RightIndexOf(String substr, int endIndex = 2147483647) const;
	deprecated("4.1", "use MakeUpper() instead") native void ToUpper();
	deprecated("4.1", "use MakeLower() instead") native void ToLower();
	native String MakeUpper() const;
	native String MakeLower() const;
	native static int CharUpper(int ch);
	native static int CharLower(int ch);
	native int ToInt(int base = 0) const;
	native double ToDouble() const;
	native void Split(out Array<String> tokens, String delimiter, EmptyTokenType keepEmpty = TOK_KEEPEMPTY) const;
	native void AppendCharacter(int c);
	native void DeleteLastCharacter();
	native int CodePointCount() const;
	native int, int GetNextCodePoint(int position) const;
}

class SectorEffect : Thinker native
{
	native protected Sector m_Sector;

	native Sector GetSector();
}

class Mover : SectorEffect native
{}

class MovingFloor : Mover native
{}

class MovingCeiling : Mover native
{}

class Floor : MovingFloor native
{
	// only here so that some constants and functions can be added. Not directly usable yet.
	enum EFloor
	{
		floorLowerToLowest,
		floorLowerToNearest,
		floorLowerToHighest,
		floorLowerByValue,
		floorRaiseByValue,
		floorRaiseToHighest,
		floorRaiseToNearest,
		floorRaiseAndCrush,
		floorRaiseAndCrushDoom,
		floorCrushStop,
		floorLowerInstant,
		floorRaiseInstant,
		floorMoveToValue,
		floorRaiseToLowestCeiling,
		floorRaiseByTexture,

		floorLowerAndChange,
		floorRaiseAndChange,

		floorRaiseToLowest,
		floorRaiseToCeiling,
		floorLowerToLowestCeiling,
		floorLowerByTexture,
		floorLowerToCeiling,

		donutRaise,

		buildStair,
		waitStair,
		resetStair,

		// Not to be used as parameters to DoFloor()
		genFloorChg0,
		genFloorChgT,
		genFloorChg
	};

	deprecated("3.8", "Use Level.CreateFloor() instead") static bool CreateFloor(sector sec, int floortype, line ln, double speed, double height = 0, int crush = -1, int change = 0, bool crushmode = false, bool hereticlower = false)
	{
		return level.CreateFloor(sec, floortype, ln, speed, height, crush, change, crushmode, hereticlower);
	}
}

class Ceiling : MovingCeiling native
{
	enum ECeiling
	{
		ceilLowerByValue,
		ceilRaiseByValue,
		ceilMoveToValue,
		ceilLowerToHighestFloor,
		ceilLowerInstant,
		ceilRaiseInstant,
		ceilCrushAndRaise,
		ceilLowerAndCrush,
		ceil_placeholder,
		ceilCrushRaiseAndStay,
		ceilRaiseToNearest,
		ceilLowerToLowest,
		ceilLowerToFloor,

		// The following are only used by Generic_Ceiling
		ceilRaiseToHighest,
		ceilLowerToHighest,
		ceilRaiseToLowest,
		ceilLowerToNearest,
		ceilRaiseToHighestFloor,
		ceilRaiseToFloor,
		ceilRaiseByTexture,
		ceilLowerByTexture,

		genCeilingChg0,
		genCeilingChgT,
		genCeilingChg
	}

	enum ECrushMode
	{
		crushDoom = 0,
		crushHexen = 1,
		crushSlowdown = 2
	}
	
	deprecated("3.8", "Use Level.CreateCeiling() instead") static bool CreateCeiling(sector sec, int type, line ln, double speed, double speed2, double height = 0, int crush = -1, int silent = 0, int change = 0, int crushmode = crushDoom)
	{
		return level.CreateCeiling(sec, type, ln, speed, speed2, height, crush, silent, change, crushmode);
	}
	
}

struct LookExParams
{
	double Fov;
	double minDist;
	double maxDist;
	double maxHeardist;
	int flags;
	State seestate;
};

class Lighting : SectorEffect native
{
}

struct Shader native
{
	native clearscope static void SetEnabled(PlayerInfo player, string shaderName, bool enable);
	native clearscope static void SetUniform1f(PlayerInfo player, string shaderName, string uniformName, float value);
	native clearscope static void SetUniform2f(PlayerInfo player, string shaderName, string uniformName, vector2 value);
	native clearscope static void SetUniform3f(PlayerInfo player, string shaderName, string uniformName, vector3 value);
	native clearscope static void SetUniform1i(PlayerInfo player, string shaderName, string uniformName, int value);
}

struct FRailParams
{
	native int damage;
	native double offset_xy;
	native double offset_z;
	native int color1, color2;
	native double maxdiff;
	native int flags;
	native Class<Actor> puff;
	native double angleoffset;
	native double pitchoffset;
	native double distance;
	native int duration;
	native double sparsity;
	native double drift;
	native Class<Actor> spawnclass;
	native int SpiralOffset;
	native int limit;
};	// [RH] Shoot a railgun

