/*
** p_effect.h
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

#pragma once

#include "vectors.h"
#include "doomdef.h"
#include "renderstyle.h"

enum
{
	FX_ROCKET			= 0x00000001,
	FX_GRENADE			= 0x00000002,
	FX_RESPAWNINVUL		= 0x00000020,
	FX_VISIBILITYPULSE	= 0x00000040
};

struct subsector_t;
struct FLevelLocals;

// [RH] Particle details

enum EParticleFlags
{
    PT_NOTIMEFREEZE = 1,
	PT_DOROLL = 1 << 1,
	PT_NOXYBILLBOARD = 1 << 2,
};

struct particle_t
{
    DVector3 Pos;
    DVector3 Vel;
    DVector3 Acc;
    double    size, sizestep;
    float    fadestep, alpha;
    subsector_t* subsector;
    int32_t    ttl;
    int        color;
    FTextureID texture;
    ERenderStyle style;
    double Roll, RollVel, RollAcc;
    uint16_t    tnext, snext, tprev;
    uint8_t    bright;
	uint8_t flags;
};

const uint16_t NO_PARTICLE = 0xffff;

void P_InitParticles(FLevelLocals *);
void P_ClearParticles (FLevelLocals *Level);
void P_FindParticleSubsectors (FLevelLocals *Level);


class AActor;

particle_t *JitterParticle (FLevelLocals *Level, int ttl);
particle_t *JitterParticle (FLevelLocals *Level, int ttl, double drift);

void P_ThinkParticles (FLevelLocals *Level);

struct FSpawnParticleParams
{
	int color;
	FTextureID texture;
	int style;
	int flags;
	int lifetime;

	double size;
	double sizestep;

	DVector3 pos;
	DVector3 vel;
	DVector3 accel;

	double startalpha;
	double fadestep;

	double startroll;
	double rollvel;
	double rollacc;
};

void P_SpawnParticle(FLevelLocals *Level, const DVector3 &pos, const DVector3 &vel, const DVector3 &accel, PalEntry color, double startalpha, int lifetime, double size, double fadestep, double sizestep, int flags = 0, FTextureID texture = FNullTextureID(), ERenderStyle style = STYLE_None, double startroll = 0, double rollvel = 0, double rollacc = 0);

void P_InitEffects (void);

void P_RunEffect (AActor *actor, int effects);

struct SPortalHit
{
	DVector3 HitPos;
	DVector3 ContPos;
	DVector3 OutDir;
};

void P_DrawRailTrail(AActor *source, TArray<SPortalHit> &portalhits, int color1, int color2, double maxdiff = 0, int flags = 0, PClassActor *spawnclass = NULL, DAngle angle = nullAngle, int duration = TICRATE, double sparsity = 1.0, double drift = 1.0, int SpiralOffset = 270, DAngle pitch = nullAngle);
void P_DrawSplash (FLevelLocals *Level, int count, const DVector3 &pos, DAngle angle, int kind);
void P_DrawSplash2 (FLevelLocals *Level, int count, const DVector3 &pos, DAngle angle, int updown, int kind);
void P_DisconnectEffect (AActor *actor);

