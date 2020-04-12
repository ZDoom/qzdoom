#pragma once

#include <algorithm>
#include <stdint.h>

// Beware of windows.h :(
#ifdef max
#undef min
#undef max
#endif

struct PalEntry
{
	PalEntry() = default;
	PalEntry (uint32_t argb) { d = argb; }
	operator uint32_t () const { return d; }
	void SetRGB(PalEntry other)
	{
		d = other.d & 0xffffff;
	}
	PalEntry Modulate(PalEntry other) const
	{
		if (isWhite())
		{
			return other;
		}
		else if (other.isWhite())
		{
			return *this;
		}
		else
		{
			other.r = (r * other.r) / 255;
			other.g = (g * other.g) / 255;
			other.b = (b * other.b) / 255;
			return other;
		}
	}
	int Luminance() const 
	{
		return (r * 77 + g * 143 + b * 37) >> 8;
	}

	int Amplitude() const
	{
		return std::max(r, std::max(g, b));
	}

	void Decolorize()	// this for 'nocoloredspritelighting' and not the same as desaturation. The normal formula results in a value that's too dark.
	{
		int v = (r + g + b);
		r = g = b = ((255*3) + v + v) / 9;
	}
	bool isBlack() const
	{
		return (d & 0xffffff) == 0;
	}
	bool isWhite() const
	{
		return (d & 0xffffff) == 0xffffff;
	}
	PalEntry &operator= (const PalEntry &other) = default;
	PalEntry &operator= (uint32_t other) { d = other; return *this; }
	PalEntry InverseColor() const { PalEntry nc; nc.a = a; nc.r = 255 - r; nc.g = 255 - g; nc.b = 255 - b; return nc; }
#ifdef __BIG_ENDIAN__
	PalEntry (uint8_t ir, uint8_t ig, uint8_t ib) : a(0), r(ir), g(ig), b(ib) {}
	PalEntry (uint8_t ia, uint8_t ir, uint8_t ig, uint8_t ib) : a(ia), r(ir), g(ig), b(ib) {}
	union
	{
		struct
		{
			uint8_t a,r,g,b;
		};
		uint32_t d;
	};
#else
	PalEntry (uint8_t ir, uint8_t ig, uint8_t ib) : b(ib), g(ig), r(ir), a(0) {}
	PalEntry (uint8_t ia, uint8_t ir, uint8_t ig, uint8_t ib) : b(ib), g(ig), r(ir), a(ia) {}
	union
	{
		struct
		{
			uint8_t b,g,r,a;
		};
		uint32_t d;
	};
#endif
};

inline int Luminance(int r, int g, int b)
{
	return (r * 77 + g * 143 + b * 37) >> 8;
}

#define APART(c)			(((c)>>24)&0xff)
#define RPART(c)			(((c)>>16)&0xff)
#define GPART(c)			(((c)>>8)&0xff)
#define BPART(c)			((c)&0xff) 
#define MAKERGB(r,g,b)		uint32_t(((r)<<16)|((g)<<8)|(b))
#define MAKEARGB(a,r,g,b)	uint32_t(((a)<<24)|((r)<<16)|((g)<<8)|(b)) 
