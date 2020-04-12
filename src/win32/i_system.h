//-----------------------------------------------------------------------------
//
// Copyright 1993-1996 id Software
// Copyright 1999-2016 Randy Heit
// Copyright 2002-2016 Christoph Oelckers
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/
//
//-----------------------------------------------------------------------------
//
// DESCRIPTION:
//		System specific interface stuff.
//
//-----------------------------------------------------------------------------


#ifndef __I_SYSTEM__
#define __I_SYSTEM__

#include "doomtype.h"
#include <thread>

struct ticcmd_t;
struct WadStuff;

// [RH] Detects the OS the game is running under.
void I_DetectOS (void);

// Called by DoomMain.
void CalculateCPUSpeed (void);

// Return a seed value for the RNG.
unsigned int I_MakeRNGSeed();


//
// Called by D_DoomLoop,
// called before processing any tics in a frame
// (just after displaying a frame).
// Time consuming syncronous operations
// are performed here (joystick reading).
// Can call D_PostEvent.
//
void I_StartFrame (void);


//
// Called by D_DoomLoop,
// called before processing each tic in a frame.
// Quick syncronous operations are performed here.
// Can call D_PostEvent.
void I_StartTic (void);

// Asynchronous interrupt functions should maintain private queues
// that are read by the synchronous functions
// to be converted into events.

// Either returns a null ticcmd,
// or calls a loadable driver to build it.
// This ticcmd will then be modified by the gameloop
// for normal input.
ticcmd_t *I_BaseTiccmd (void);


// Called by M_Responder when quit is selected.
// Clean exit, displays sell blurb.
void I_Quit (void);


// Set the mouse cursor. The texture must be 32x32.
class FTexture;
bool I_SetCursor(FTexture *cursor);

// Repaint the pre-game console
void I_PaintConsole (void);

// Print a console string
void I_PrintStr (const char *cp);

// Set the title string of the startup window
void I_SetIWADInfo ();

// Pick from multiple IWADs to use
int I_PickIWad (WadStuff *wads, int numwads, bool queryiwad, int defaultiwad);

// The ini could not be saved at exit
bool I_WriteIniFailed ();

// [RH] Used by the display code to set the normal window procedure
void I_SetWndProc();

// [RH] Checks the registry for Steam's install path, so we can scan its
// directories for IWADs if the user purchased any through Steam.
TArray<FString> I_GetSteamPath();

// [GZ] Same deal for GOG paths
TArray<FString> I_GetGogPaths();

// Damn Microsoft for doing Get/SetWindowLongPtr half-assed. Instead of
// giving them proper prototypes under Win32, they are just macros for
// Get/SetWindowLong, meaning they take LONGs and not LONG_PTRs.
#ifdef _WIN64
typedef long long WLONG_PTR;
#elif _MSC_VER
typedef _W64 long WLONG_PTR;
#else
typedef long WLONG_PTR;
#endif

// Wrapper for GetLongPathName
FString I_GetLongPathName(const FString &shortpath);

// Mirror WIN32_FIND_DATAA in <winbase.h>
#ifndef MAX_PATH
#define MAX_PATH 260
#endif
#ifndef PATH_MAX
#define PATH_MAX 260
#endif

int I_GetNumaNodeCount();
int I_GetNumaNodeThreadCount(int numaNode);
void I_SetThreadNumaNode(std::thread &thread, int numaNode);

#endif
