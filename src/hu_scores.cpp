/*
** hu_scores.cpp
** Routines for drawing the scoreboards.
**
**---------------------------------------------------------------------------
** Copyright 1998-2008 Randy Heit
** Copyright 2007-2008 Christopher Westley
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

// HEADER FILES ------------------------------------------------------------

#include "c_console.h"
#include "teaminfo.h"

#include "v_video.h"
#include "doomstat.h"
#include "g_level.h"
#include "d_netinf.h"
#include "v_font.h"
#include "d_player.h"
#include "hu_stuff.h"
#include "gstrings.h"
#include "d_net.h"
#include "c_dispatch.h"
#include "g_levellocals.h"
#include "g_game.h"
#include "sbar.h"
#include "texturemanager.h"
#include "v_draw.h"

// MACROS ------------------------------------------------------------------

// TYPES -------------------------------------------------------------------

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

static void HU_DoDrawScores (player_t *, player_t *[MAXPLAYERS]);
static void HU_DrawTimeRemaining (int y);
static void HU_DrawPlayer(player_t *, bool, int, int, int, int, int, int, int, int, int);

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

EXTERN_CVAR (Float, timelimit)

// PUBLIC DATA DEFINITIONS -------------------------------------------------

CVAR (Bool,	sb_cooperative_enable,				true,		CVAR_ARCHIVE)
CVAR (Int,	sb_cooperative_headingcolor,		CR_RED,		CVAR_ARCHIVE)
CVAR (Int,	sb_cooperative_yourplayercolor,		CR_GREEN,	CVAR_ARCHIVE)
CVAR (Int,	sb_cooperative_otherplayercolor,	CR_GREY,	CVAR_ARCHIVE)

CVAR (Bool,	sb_deathmatch_enable,				true,		CVAR_ARCHIVE)
CVAR (Int,	sb_deathmatch_headingcolor,			CR_RED,		CVAR_ARCHIVE)
CVAR (Int,	sb_deathmatch_yourplayercolor,		CR_GREEN,	CVAR_ARCHIVE)
CVAR (Int,	sb_deathmatch_otherplayercolor,		CR_GREY,	CVAR_ARCHIVE)

CVAR (Bool,	sb_teamdeathmatch_enable,			true,		CVAR_ARCHIVE)
CVAR (Int,	sb_teamdeathmatch_headingcolor,		CR_RED,		CVAR_ARCHIVE)

int comparepoints (const void *arg1, const void *arg2)
{
	// Compare first be frags/kills, then by name.
	player_t *p1 = *(player_t **)arg1;
	player_t *p2 = *(player_t **)arg2;
	int diff;

	diff = deathmatch ? p2->fragcount - p1->fragcount : p2->killcount - p1->killcount;
	if (diff == 0)
	{
		diff = stricmp(p1->userinfo.GetName(), p2->userinfo.GetName());
	}
	return diff;
}

int compareteams (const void *arg1, const void *arg2)
{
	// Compare first by teams, then by frags, then by name.
	player_t *p1 = *(player_t **)arg1;
	player_t *p2 = *(player_t **)arg2;
	int diff;

	diff = p1->userinfo.GetTeam() - p2->userinfo.GetTeam();
	if (diff == 0)
	{
		diff = p2->fragcount - p1->fragcount;
		if (diff == 0)
		{
			diff = stricmp (p1->userinfo.GetName(), p2->userinfo.GetName());
		}
	}
	return diff;
}

/*
void HU_SortPlayers
{
	if (teamplay)
	qsort(sortedplayers, MAXPLAYERS, sizeof(player_t *), compareteams);
	else
		qsort(sortedplayers, MAXPLAYERS, sizeof(player_t *), comparepoints);
}
*/

bool SB_ForceActive = false;
static FFont *displayFont;
static int FontScale;


// PRIVATE DATA DEFINITIONS ------------------------------------------------

// CODE --------------------------------------------------------------------

//==========================================================================
//
// HU_DrawScores
//
//==========================================================================

void HU_DrawScores (player_t *player)
{
	displayFont = NewSmallFont;
	FontScale = max(screen->GetHeight() / 400, 1);

	if (deathmatch)
	{
		if (teamplay)
		{
			if (!sb_teamdeathmatch_enable)
				return;
		}
		else
		{
			if (!sb_deathmatch_enable)
				return;
		}
	}
	else
	{
		if (!sb_cooperative_enable || !multiplayer)
			return;
	}

	int i, j;
	player_t *sortedplayers[MAXPLAYERS];

	if (player->camera && player->camera->player)
		player = player->camera->player;

	sortedplayers[MAXPLAYERS-1] = player;
	for (i = 0, j = 0; j < MAXPLAYERS - 1; i++, j++)
	{
		if (&players[i] == player)
			i++;
		sortedplayers[j] = &players[i];
	}

	if (teamplay && deathmatch)
		qsort (sortedplayers, MAXPLAYERS, sizeof(player_t *), compareteams);
	else
		qsort (sortedplayers, MAXPLAYERS, sizeof(player_t *), comparepoints);

	HU_DoDrawScores (player, sortedplayers);

}

//==========================================================================
//
// HU_GetPlayerWidths
//
// Returns the widest player name and class icon.
//
//==========================================================================

void HU_GetPlayerWidths(int &maxnamewidth, int &maxscorewidth, int &maxiconheight)
{
	displayFont = NewSmallFont;
	maxnamewidth = displayFont->StringWidth("Name");
	maxscorewidth = 0;
	maxiconheight = 0;

	for (int i = 0; i < MAXPLAYERS; i++)
	{
		if (playeringame[i])
		{
			int width = displayFont->StringWidth(players[i].userinfo.GetName());
			if (width > maxnamewidth)
			{
				maxnamewidth = width;
			}
			auto icon = FSetTextureID(players[i].mo->IntVar(NAME_ScoreIcon));
			if (icon.isValid())
			{
				auto pic = TexMan.GetGameTexture(icon);
				width = int(pic->GetDisplayWidth() - pic->GetDisplayLeftOffset() + 2.5);
				if (width > maxscorewidth)
				{
					maxscorewidth = width;
				}
				// The icon's top offset does not count toward its height, because
				// zdoom.pk3's standard Hexen class icons are designed that way.
				int height = int(pic->GetDisplayHeight() - pic->GetDisplayTopOffset() + 0.5);
				if (height > maxiconheight)
				{
					maxiconheight = height;
				}
			}
		}
	}
}

//==========================================================================
//
// HU_DoDrawScores
//
//==========================================================================

static void HU_DrawFontScaled(double x, double y, int color, const char *text)
{
	DrawText(twod, displayFont, color, x / FontScale, y / FontScale, text, DTA_VirtualWidth, twod->GetWidth() / FontScale, DTA_VirtualHeight, twod->GetHeight() / FontScale, TAG_END);
}

static void HU_DoDrawScores (player_t *player, player_t *sortedplayers[MAXPLAYERS])
{
	int color;
	int height, lineheight;
	unsigned int i;
	int maxnamewidth, maxscorewidth, maxiconheight;
	int numTeams = 0;
	int x, y, ypadding, bottom;
	int col2, col3, col4, col5;
	
	if (deathmatch)
	{
		if (teamplay)
			color = sb_teamdeathmatch_headingcolor;
		else
			color = sb_deathmatch_headingcolor;
	}
	else
	{
		color = sb_cooperative_headingcolor;
	}

	HU_GetPlayerWidths(maxnamewidth, maxscorewidth, maxiconheight);
	height = displayFont->GetHeight() * FontScale;
	lineheight = max(height, maxiconheight * CleanYfac);
	ypadding = (lineheight - height + 1) / 2;

	bottom = StatusBar->GetTopOfStatusbar();
	y = max(48*CleanYfac, (bottom - MAXPLAYERS * (height + CleanYfac + 1)) / 2);

	HU_DrawTimeRemaining (bottom - height);

	if (teamplay && deathmatch)
	{
		y -= (BigFont->GetHeight() + 8) * CleanYfac;

		for (i = 0; i < Teams.Size (); i++)
		{
			Teams[i].m_iPlayerCount = 0;
			Teams[i].m_iScore = 0;
		}

		for (i = 0; i < MAXPLAYERS; ++i)
		{
			if (playeringame[sortedplayers[i]-players] && TeamLibrary.IsValidTeam (sortedplayers[i]->userinfo.GetTeam()))
			{
				if (Teams[sortedplayers[i]->userinfo.GetTeam()].m_iPlayerCount++ == 0)
				{
					numTeams++;
				}

				Teams[sortedplayers[i]->userinfo.GetTeam()].m_iScore += sortedplayers[i]->fragcount;
			}
		}

		int scorexwidth = twod->GetWidth() / max(8, numTeams);
		int numscores = 0;
		int scorex;

		for (i = 0; i < Teams.Size(); ++i)
		{
			if (Teams[i].m_iPlayerCount)
			{
				numscores++;
			}
		}

		scorex = (twod->GetWidth() - scorexwidth * (numscores - 1)) / 2;

		for (i = 0; i < Teams.Size(); ++i)
		{
			if (Teams[i].m_iPlayerCount)
			{
				char score[80];
				mysnprintf (score, countof(score), "%d", Teams[i].m_iScore);

				DrawText(twod, BigFont, Teams[i].GetTextColor(),
					scorex - BigFont->StringWidth(score)*CleanXfac/2, y, score,
					DTA_CleanNoMove, true, TAG_DONE);

				scorex += scorexwidth;
			}
		}

		y += (BigFont->GetHeight() + 8) * CleanYfac;
	}

	const char *text_color = GStrings("SCORE_COLOR"),
		*text_frags = GStrings(deathmatch ? "SCORE_FRAGS" : "SCORE_KILLS"),
		*text_name = GStrings("SCORE_NAME"),
		*text_delay = GStrings("SCORE_DELAY");

	col2 = (displayFont->StringWidth(text_color) + 16) * FontScale;
	col3 = col2 + (displayFont->StringWidth(text_frags) + 16) * FontScale;
	col4 = col3 + maxscorewidth * FontScale;
	col5 = col4 + (maxnamewidth + 16) * FontScale;
	x = (twod->GetWidth() >> 1) - (((displayFont->StringWidth(text_delay) * FontScale) + col5) >> 1);

	//HU_DrawFontScaled(x, y, color, text_color);
	HU_DrawFontScaled(x + col2, y, color, text_frags);
	HU_DrawFontScaled(x + col4, y, color, text_name);
	HU_DrawFontScaled(x + col5, y, color, text_delay);

	y += height + 6 * CleanYfac;
	bottom -= height;

	for (i = 0; i < MAXPLAYERS && y <= bottom; i++)
	{
		if (playeringame[sortedplayers[i] - players])
		{
			HU_DrawPlayer(sortedplayers[i], player == sortedplayers[i], x, col2, col3, col4, col5, maxnamewidth, y, ypadding, lineheight);
			y += lineheight + CleanYfac;
		}
	}
}

//==========================================================================
//
// HU_DrawTimeRemaining
//
//==========================================================================

static void HU_DrawTimeRemaining (int y)
{
	if (deathmatch && timelimit && gamestate == GS_LEVEL)
	{
		char str[80];
		int timeleft = (int)(timelimit * TICRATE * 60) - primaryLevel->maptime;
		int hours, minutes, seconds;

		if (timeleft < 0)
			timeleft = 0;

		hours = timeleft / (TICRATE * 3600);
		timeleft -= hours * TICRATE * 3600;
		minutes = timeleft / (TICRATE * 60);
		timeleft -= minutes * TICRATE * 60;
		seconds = timeleft / TICRATE;

		if (hours)
			mysnprintf (str, countof(str), "Level ends in %d:%02d:%02d", hours, minutes, seconds);
		else
			mysnprintf (str, countof(str), "Level ends in %d:%02d", minutes, seconds);

		HU_DrawFontScaled(twod->GetWidth() / 2 - displayFont->StringWidth(str) / 2 * FontScale, y, CR_GRAY, str);
	}
}

//==========================================================================
//
// HU_DrawPlayer
//
//==========================================================================

static void HU_DrawPlayer (player_t *player, bool highlight, int col1, int col2, int col3, int col4, int col5, int maxnamewidth, int y, int ypadding, int height)
{
	int color;
	char str[80];

	if (highlight)
	{
		// The teamplay mode uses colors to show teams, so we need some
		// other way to do highlighting. And it may as well be used for
		// all modes for the sake of consistancy.
		Dim(twod, MAKERGB(200,245,255), 0.125f, col1 - 12*FontScale, y - 1, col5 + (maxnamewidth + 24)*FontScale, height + 2);
	}

	col2 += col1;
	col3 += col1;
	col4 += col1;
	col5 += col1;

	color = HU_GetRowColor(player, highlight);
	HU_DrawColorBar(col1, y, height, (int)(player - players));
	mysnprintf (str, countof(str), "%d", deathmatch ? player->fragcount : player->killcount);

	HU_DrawFontScaled(col2, y + ypadding, color, player->playerstate == PST_DEAD && !deathmatch ? "DEAD" : str);

	auto icon = FSetTextureID(player->mo->IntVar(NAME_ScoreIcon));
	if (icon.isValid())
	{
		DrawTexture(twod, icon, false, col3, y,
			DTA_CleanNoMove, true,
			TAG_DONE);
	}

	HU_DrawFontScaled(col4, y + ypadding, color, player->userinfo.GetName());

	int avgdelay = 0;
	for (int i = 0; i < BACKUPTICS; i++)
	{
		avgdelay += netdelay[nodeforplayer[(int)(player - players)]][i];
	}
	avgdelay /= BACKUPTICS;

	mysnprintf(str, countof(str), "%d", (avgdelay * ticdup) * (1000 / TICRATE));

	HU_DrawFontScaled(col5, y + ypadding, color, str);

	if (teamplay && Teams[player->userinfo.GetTeam()].GetLogo().IsNotEmpty ())
	{
		auto pic = TexMan.GetGameTextureByName(Teams[player->userinfo.GetTeam()].GetLogo().GetChars ());
		DrawTexture(twod, pic, col1 - (pic->GetDisplayWidth() + 2) * CleanXfac, y,
			DTA_CleanNoMove, true, TAG_DONE);
	}
}

//==========================================================================
//
// HU_DrawColorBar
//
//==========================================================================

void HU_DrawColorBar(int x, int y, int height, int playernum)
{
	float h, s, v, r, g, b;

	D_GetPlayerColor (playernum, &h, &s, &v, NULL);
	HSVtoRGB (&r, &g, &b, h, s, v);

	ClearRect(twod, x, y, x + 24*FontScale, y + height, -1,
		MAKEARGB(255,clamp(int(r*255.f),0,255),
					 clamp(int(g*255.f),0,255),
					 clamp(int(b*255.f),0,255)));
}

//==========================================================================
//
// HU_GetRowColor
//
//==========================================================================

int HU_GetRowColor(player_t *player, bool highlight)
{
	if (teamplay && deathmatch)
	{
		if (TeamLibrary.IsValidTeam (player->userinfo.GetTeam()))
			return Teams[player->userinfo.GetTeam()].GetTextColor();
		else
			return CR_GREY;
	}
	else
	{
		if (!highlight)
		{
			if (demoplayback && player == &players[consoleplayer])
			{
				return CR_GOLD;
			}
			else
			{
				return deathmatch ? sb_deathmatch_otherplayercolor : sb_cooperative_otherplayercolor;
			}
		}
		else
		{
			return deathmatch ? sb_deathmatch_yourplayercolor : sb_cooperative_yourplayercolor;
		}
	}
}

CCMD (togglescoreboard)
{
	SB_ForceActive = !SB_ForceActive;
}
