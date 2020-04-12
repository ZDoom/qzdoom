#pragma once

#include <stddef.h>
#include <stdint.h>

//
// fixed point, 32bit as 16.16.
//
#define FRACBITS						16
#define FRACUNIT						(1<<FRACBITS)

typedef int32_t							fixed_t;

#define FIXED_MAX						(signed)(0x7fffffff)
#define FIXED_MIN						(signed)(0x80000000)

// the last remnants of tables.h
#define ANGLE_90		(0x40000000)
#define ANGLE_180		(0x80000000)
#define ANGLE_270		(0xc0000000)
#define ANGLE_MAX		(0xffffffff)

typedef uint32_t			angle_t;

#if defined(__GNUC__)
// With versions of GCC newer than 4.2, it appears it was determined that the
// cost of an unaligned pointer on PPC was high enough to add padding to the
// end of packed structs.  For whatever reason __packed__ and pragma pack are
// handled differently in this regard. Note that this only needs to be applied
// to types which are used in arrays or sizeof is needed. This also prevents
// code from taking references to the struct members.
#define FORCE_PACKED __attribute__((__packed__))
#else
#define FORCE_PACKED
#endif


#ifdef __GNUC__
#define GCCPRINTF(stri,firstargi)		__attribute__((format(printf,stri,firstargi)))
#define GCCFORMAT(stri)					__attribute__((format(printf,stri,0)))
#define GCCNOWARN						__attribute__((unused))
#else
#define GCCPRINTF(a,b)
#define GCCFORMAT(a)
#define GCCNOWARN
#endif

#ifndef MAKE_ID
#ifndef __BIG_ENDIAN__
#define MAKE_ID(a,b,c,d)	((uint32_t)((a)|((b)<<8)|((c)<<16)|((d)<<24)))
#else
#define MAKE_ID(a,b,c,d)	((uint32_t)((d)|((c)<<8)|((b)<<16)|((a)<<24)))
#endif
#endif

using INTBOOL = int;
using BITFIELD = uint32_t;


#if defined(_MSC_VER)
#define NOVTABLE __declspec(novtable)
#else
#define NOVTABLE
#endif

// always use our own definition for consistency.
#ifdef M_PI
#undef M_PI
#endif

const double M_PI = 3.14159265358979323846;	// matches value in gcc v2 math.h

inline float DEG2RAD(float deg)
{
	return deg * float(M_PI / 180.0);
}

inline double DEG2RAD(double deg)
{
	return deg * (M_PI / 180.0);
}

inline float RAD2DEG(float deg)
{
	return deg * float(180. / M_PI);
}


// Auto-registration sections for GCC.
// Apparently, you cannot do string concatenation inside section attributes.
#ifdef __MACH__
#define SECTION_AREG "__DATA,areg"
#define SECTION_CREG "__DATA,creg"
#define SECTION_FREG "__DATA,freg"
#define SECTION_GREG "__DATA,greg"
#define SECTION_MREG "__DATA,mreg"
#define SECTION_YREG "__DATA,yreg"
#else
#define SECTION_AREG "areg"
#define SECTION_CREG "creg"
#define SECTION_FREG "freg"
#define SECTION_GREG "greg"
#define SECTION_MREG "mreg"
#define SECTION_YREG "yreg"
#endif

// This is needed in common code, despite being Doom specific.
enum EStateUseFlags
{
	SUF_ACTOR = 1,
	SUF_OVERLAY = 2,
	SUF_WEAPON = 4,
	SUF_ITEM = 8,
};
