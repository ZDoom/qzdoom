/*
** stats.cpp
** Performance-monitoring statistics
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

#include "stats.h"
#include "v_draw.h"
#include "v_text.h"
#include "v_font.h"
#include "c_console.h"
#include "c_dispatch.h"
#include "printf.h"

FStat *FStat::FirstStat;

FStat::FStat (const char *name)
{
	m_Name = name;
	m_Active = false;
	m_Next = FirstStat;
	FirstStat = this;
}

FStat::~FStat ()
{
	FStat **prev = &FirstStat;

	while (*prev && *prev != this)
		prev = &((*prev)->m_Next)->m_Next;

	if (*prev == this)
		*prev = m_Next;
}

FStat *FStat::FindStat (const char *name)
{
	FStat *stat = FirstStat;

	while (stat && stricmp (name, stat->m_Name))
		stat = stat->m_Next;

	return stat;
}

void FStat::ToggleStat (const char *name)
{
	FStat *stat = FindStat (name);
	if (stat)
		stat->ToggleStat ();
	else
		Printf ("Unknown stat: %s\n", name);
}

void FStat::EnableStat(const char* name, bool on)
{
	FStat* stat = FindStat(name);
	if (stat)
		stat->m_Active = on;
	else
		Printf("Unknown stat: %s\n", name);
}

void FStat::ToggleStat ()
{
	m_Active = !m_Active;
}

void FStat::PrintStat (F2DDrawer *drawer)
{
	int textScale = active_con_scale(drawer);

	int fontheight = NewConsoleFont->GetHeight() + 1;
	int y = drawer->GetHeight() / textScale;
	int count = 0;

	for (FStat *stat = FirstStat; stat != NULL; stat = stat->m_Next)
	{
		if (stat->m_Active)
		{
			FString stattext(stat->GetStats());

			if (stattext.Len() > 0)
			{
				y -= fontheight;	// there's at least one line of text
				for (unsigned i = 0; i < stattext.Len()-1; i++)
				{
					// Count number of linefeeds but ignore terminating ones.
					if (stattext[i] == '\n') y -= fontheight;
				}
				DrawText(drawer, NewConsoleFont, CR_GREEN, 5 / textScale, y, stattext,
					DTA_VirtualWidth, twod->GetWidth() / textScale,
					DTA_VirtualHeight, twod->GetHeight() / textScale,
					DTA_KeepRatio, true, TAG_DONE);
				count++;
			}
		}
	}
}

void FStat::DumpRegisteredStats ()
{
	FStat *stat = FirstStat;

	Printf ("Available stats:\n");
	while (stat)
	{
		Printf (" %c%s\n", stat->m_Active ? '*' : ' ', stat->m_Name);
		stat = stat->m_Next;
	}
}

CCMD (stat)
{
	if (argv.argc() != 2)
	{
		Printf ("Usage: stat <statistics>\n");
		FStat::DumpRegisteredStats ();
	}
	else
	{
		FStat::ToggleStat (argv[1]);
	}
}
