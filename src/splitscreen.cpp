// 
//---------------------------------------------------------------------------
//
// Copyright(C) 2017 Rachael Alexanderson
// All rights reserved.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/
//
//--------------------------------------------------------------------------
//
/*
** splitscreen.cpp
** Lots and lots of splitscreen stuff
**
**/


#include "gl/system/gl_system.h"
#include "c_dispatch.h"
#include "d_player.h"
#include "d_net.h"
#include "doomstat.h"
#include "gi.h"
#include "g_game.h"
#include "g_statusbar/sbar.h"
#include "g_statusbar/sbarinfo.h"
#include "gl/renderer/gl_renderbuffers.h"
#include "gl/renderer/gl_renderer.h"
#include "gl/renderer/gl_renderstate.h"
#include "gl/stereo3d/gl_stereo3d.h"
#include "gl/renderer/gl_2ddrawer.h"
#include "gl/renderer/gl_postprocessstate.h"
#include "teaminfo.h"


EXTERN_CVAR(Bool, splitscreen)
EXTERN_CVAR(Int, vr_mode)
EXTERN_CVAR(Int, ss_mode)


void D_Display();
void G_BuildTiccmd(ticcmd_t* cmd);
void G_BuildTiccmd_Split(ticcmd_t* cmd, ticcmd_t* cmd2);
void V_FixAspectSettings();


//-----------------------------------------------------------------------------
//
// Alternative to D_Display () for splitscreen. In fact, it calls it, but
// with some restrictions. ;)
//
//-----------------------------------------------------------------------------

void FGLRenderer::SplitDisplays()
{
	if (!this)
	{
		splitscreen = false;
		return D_Display();
	}

	//static gamestate_t lastgamestate = gamestate;
	int oldcp = consoleplayer;
	int oldvr = vr_mode;
	DBaseStatusBar *OldStatusBar = StatusBar;

	if (vr_mode == 0)
		vr_mode = ss_mode;

	const s3d::Stereo3DMode& stereo3dMode = s3d::Stereo3DMode::getCurrentMode();

	if (gamestate == GS_LEVEL || gamestate == GS_TITLELEVEL)
		Renderer->RenderView(&players[consoleplayer]);

	//vr_mode = 0;
	for (int player = 0;player<2;player++)
	{
		if (consoleplayer == -1)
			consoleplayer = oldcp;

		if (gamestate == GS_LEVEL || gamestate == GS_TITLELEVEL)
		{
			if (StatusBar && &players[consoleplayer] && &players[consoleplayer].camera && &players[consoleplayer].camera->player)
			{
				StatusBar->AttachToPlayer(players[consoleplayer].camera->player);
				StatusBar->Tick();
			}
		}

		D_Display ();

		mBuffers->BindEyeFB(player);
		
		glViewport(mScreenViewport.left, mScreenViewport.top, mScreenViewport.width, mScreenViewport.height);
		glScissor(mScreenViewport.left, mScreenViewport.top, mScreenViewport.width, mScreenViewport.height);
		m2DDrawer->Draw();
		m2DDrawer->Clear();

		consoleplayer = consoleplayer2;
		StatusBar = StatusBar2;
	}
	vr_mode = oldvr;
	consoleplayer = oldcp;
	StatusBar = OldStatusBar;

	FGLPostProcessState savedState;
	stereo3dMode.Present();

	screen->Update ();

	StatusBar->AttachToPlayer (&players[consoleplayer]); // just in case...
}


//==========================================================================
//
// G_HandleSplitscreen()
//
// Sets up another player if necessary, and then processes input for that
// player. Rendering the other screen will be handled by the VR code.
//
//==========================================================================

extern int playerfornode[];

void G_CreateSplitStatusBar()
{
	if (StatusBar2 != NULL)
	{
		StatusBar2->Destroy();
		StatusBar2 = NULL;
	}
	auto cls = PClass::FindClass("DoomStatusBar");

	if (cls && gameinfo.gametype == GAME_Doom)
	{
		StatusBar2 = (DBaseStatusBar*)cls->CreateNew();
	}
	else if (SBarInfoScript[SCRIPT_CUSTOM] != NULL)
	{
		int cstype = SBarInfoScript[SCRIPT_CUSTOM]->GetGameType();

		//Did the user specify a "base"
		if(cstype == GAME_Strife)
		{
			StatusBar2 = CreateStrifeStatusBar();
		}
		else if(cstype == GAME_Any) //Use the default, empty or custom.
		{
			StatusBar2 = CreateCustomStatusBar(SCRIPT_CUSTOM);
		}
		else
		{
			StatusBar2 = CreateCustomStatusBar(SCRIPT_DEFAULT);
		}
	}
	if (StatusBar == NULL)
	{
		if (gameinfo.gametype & (GAME_DoomChex|GAME_Heretic|GAME_Hexen))
		{
			StatusBar2 = CreateCustomStatusBar (SCRIPT_DEFAULT);
		}
		else if (gameinfo.gametype == GAME_Strife)
		{
			StatusBar2 = CreateStrifeStatusBar ();
		}
		else
		{
			StatusBar2 = new DBaseStatusBar (0);
		}
	}
}

void G_HandleSplitscreen(ticcmd_t* cmd)
{
	if (netgame)
	{
		Printf("Splitscreen is not yet supported in netgames!\n");
		splitscreen = false;
		G_BuildTiccmd (cmd);
		return;
	}
	if (!GLRenderer)
	{
		Printf("Splitscreen is not yet supported in the software renderer!\n");
		splitscreen = false;
		G_BuildTiccmd (cmd);
		return;
	}
	if (demorecording)
	{
		Printf("Splitscreen is not yet supported in demos!\n");
		splitscreen = false;
		G_BuildTiccmd (cmd);
		return;
	}

	// splitscreen player not yet spawned, find a spot for them
	if (consoleplayer2 == -1)
	{
		int oldcp = consoleplayer;
		int bnum = 0;
		// Player not in game yet, let's tic like we're single player, we'll do split tics later.
		G_BuildTiccmd (cmd);

		// taken from the botcode
		for (bnum = 0; bnum < MAXPLAYERS; bnum++)
			if (!playeringame[bnum])
				break;

		if (bnum == MAXPLAYERS)
		{
			Printf ("The maximum of %d players/bots has been reached\n", MAXPLAYERS);
			splitscreen = false;
			return;
		}

		// [SP] Set up other "eye"
		//if (vr_mode == 0)
			//vr_mode = 3;

		multiplayer = true;
		playeringame[bnum] = true;
		consoleplayer2 = bnum;
		players[bnum].mo = nullptr;
		//players[bnum].userinfo.TransferFrom(players[consoleplayer].userinfo);
		players[bnum].playerstate = PST_ENTER;

		if (teamplay)
			Printf ("%s joined the %s team\n", players[bnum].userinfo.GetName(), Teams[players[bnum].userinfo.GetTeam()].GetName());
		else
			Printf ("%s joined the game\n", players[bnum].userinfo.GetName());

		G_DoReborn (bnum, true);
		if (StatusBar != NULL)
		{
			StatusBar->MultiplayerChanged ();
		}
		if (StatusBar2 == NULL)
			G_CreateSplitStatusBar();
		V_FixAspectSettings();
		return;
	}

	if (StatusBar2 == NULL)
		G_CreateSplitStatusBar();

	G_BuildTiccmd_Split (cmd, &netcmds[consoleplayer2][maketic%BACKUPTICS]);
}

//==========================================================================
//
// G_DestroySplitscreen()
//
// If we have a splitscreen player, remove them from the game!
//
//==========================================================================

void G_DestroySplitscreen()
{
	if (consoleplayer2 == -1)
		return;

	G_DoPlayerPop(consoleplayer2);

	if (StatusBar2 != NULL)
	{
		StatusBar2->Destroy();
		StatusBar2 = NULL;
	}

	V_FixAspectSettings();

	//vr_mode = 0;
	consoleplayer2 = -1;
}

CCMD (splitswap)
{
	int old_consoleplayer = consoleplayer;
	if (!splitscreen)
	{
		Printf("Splitscreen is not active!\n");
		return;
	}
	if (consoleplayer2 == -1)
	{
		Printf("Splitscreen player is not yet spawned!\n");
		return;
	}
	consoleplayer = playerfornode[0] = Net_Arbitrator = consoleplayer2;
	consoleplayer2 = old_consoleplayer;
}

