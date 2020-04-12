/*
** autozend.cpp
** This file contains the tails of lists stored in special data segments
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
** See autostart.cpp for an explanation of why I do things like this.
*/

#include "autosegs.h"

#if defined(_MSC_VER)

#pragma section(".areg$z",read)
__declspec(allocate(".areg$z")) void *const ARegTail = 0;

#pragma section(".creg$z",read)
__declspec(allocate(".creg$z")) void *const CRegTail = 0;

#pragma section(".freg$z",read)
__declspec(allocate(".freg$z")) void *const FRegTail = 0;

#pragma section(".greg$z",read)
__declspec(allocate(".greg$z")) void *const GRegTail = 0;

#pragma section(".yreg$z",read)
__declspec(allocate(".yreg$z")) void *const YRegTail = 0;


#elif defined(__GNUC__)

#include "doomtype.h"

void *const ARegTail __attribute__((section(SECTION_AREG))) = 0;
void *const CRegTail __attribute__((section(SECTION_CREG))) = 0;
void *const FRegTail __attribute__((section(SECTION_FREG))) = 0;
void *const GRegTail __attribute__((section(SECTION_GREG))) = 0;
void *const YRegTail __attribute__((section(SECTION_YREG))) = 0;

#else

#error Please fix autozend.cpp for your compiler

#endif
