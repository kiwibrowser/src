#ifndef _DEINT32_H
#define _DEINT32_H
/*-------------------------------------------------------------------------
 * drawElements Base Portability Library
 * -------------------------------------
 *
 * Copyright 2014 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 *//*!
 * \file
 * \brief 32-bit integer math.
 *//*--------------------------------------------------------------------*/

#include "deDefs.h"

#if (DE_COMPILER == DE_COMPILER_MSC)
#	include <intrin.h>
#endif

DE_BEGIN_EXTERN_C

enum
{
	DE_RCP_FRAC_BITS	= 30		/*!< Number of fractional bits in deRcp32() result. */
};

void	deRcp32				(deUint32 a, deUint32* rcp, int* exp);
void	deInt32_computeLUTs	(void);
void	deInt32_selfTest	(void);

/*--------------------------------------------------------------------*//*!
 * \brief Compute the absolute of an int.
 * \param a	Input value.
 * \return Absolute of the input value.
 *
 * \note The input 0x80000000u (for which the abs value cannot be
 * represented), is asserted and returns the value itself.
 *//*--------------------------------------------------------------------*/
DE_INLINE int deAbs32 (int a)
{
	DE_ASSERT((unsigned int) a != 0x80000000u);
	return (a < 0) ? -a : a;
}

/*--------------------------------------------------------------------*//*!
 * \brief Compute the signed minimum of two values.
 * \param a	First input value.
 * \param b Second input value.
 * \return The smallest of the two input values.
 *//*--------------------------------------------------------------------*/
DE_INLINE int deMin32 (int a, int b)
{
	return (a <= b) ? a : b;
}

/*--------------------------------------------------------------------*//*!
 * \brief Compute the signed maximum of two values.
 * \param a	First input value.
 * \param b Second input value.
 * \return The largest of the two input values.
 *//*--------------------------------------------------------------------*/
DE_INLINE int deMax32 (int a, int b)
{
	return (a >= b) ? a : b;
}

/*--------------------------------------------------------------------*//*!
 * \brief Compute the unsigned minimum of two values.
 * \param a	First input value.
 * \param b Second input value.
 * \return The smallest of the two input values.
 *//*--------------------------------------------------------------------*/
DE_INLINE deUint32 deMinu32 (deUint32 a, deUint32 b)
{
	return (a <= b) ? a : b;
}

/*--------------------------------------------------------------------*//*!
 * \brief Compute the unsigned maximum of two values.
 * \param a	First input value.
 * \param b Second input value.
 * \return The largest of the two input values.
 *//*--------------------------------------------------------------------*/
DE_INLINE deUint32 deMaxu32 (deUint32 a, deUint32 b)
{
	return (a >= b) ? a : b;
}

/*--------------------------------------------------------------------*//*!
 * \brief Check if a value is in the <b>inclusive<b> range [mn, mx].
 * \param a		Value to check for range.
 * \param mn	Range minimum value.
 * \param mx	Range maximum value.
 * \return True if (a >= mn) and (a <= mx), false otherwise.
 *
 * \see deInBounds32()
 *//*--------------------------------------------------------------------*/
DE_INLINE deBool deInRange32 (int a, int mn, int mx)
{
	return (a >= mn) && (a <= mx);
}

/*--------------------------------------------------------------------*//*!
 * \brief Check if a value is in the half-inclusive bounds [mn, mx[.
 * \param a		Value to check for range.
 * \param mn	Range minimum value.
 * \param mx	Range maximum value.
 * \return True if (a >= mn) and (a < mx), false otherwise.
 *
 * \see deInRange32()
 *//*--------------------------------------------------------------------*/
DE_INLINE deBool deInBounds32 (int a, int mn, int mx)
{
	return (a >= mn) && (a < mx);
}

/*--------------------------------------------------------------------*//*!
 * \brief Clamp a value into the range [mn, mx].
 * \param a		Value to clamp.
 * \param mn	Minimum value.
 * \param mx	Maximum value.
 * \return The clamped value in [mn, mx] range.
 *//*--------------------------------------------------------------------*/
DE_INLINE int deClamp32 (int a, int mn, int mx)
{
	DE_ASSERT(mn <= mx);
	if (a < mn) return mn;
	if (a > mx) return mx;
	return a;
}

/*--------------------------------------------------------------------*//*!
 * \brief Get the sign of an integer.
 * \param a	Input value.
 * \return +1 if a>0, 0 if a==0, -1 if a<0.
 *//*--------------------------------------------------------------------*/
DE_INLINE int deSign32 (int a)
{
	if (a > 0) return +1;
	if (a < 0) return -1;
	return 0;
}

/*--------------------------------------------------------------------*//*!
 * \brief Extract the sign bit of a.
 * \param a	Input value.
 * \return 0x80000000 if a<0, 0 otherwise.
 *//*--------------------------------------------------------------------*/
DE_INLINE deInt32 deSignBit32 (deInt32 a)
{
	return (deInt32)((deUint32)a & 0x80000000u);
}

/*--------------------------------------------------------------------*//*!
 * \brief Integer rotate right.
 * \param val	Value to rotate.
 * \param r		Number of bits to rotate (in range [0, 32]).
 * \return The rotated value.
 *//*--------------------------------------------------------------------*/
DE_INLINE int deRor32 (int val, int r)
{
	DE_ASSERT(r >= 0 && r <= 32);
	if (r == 0 || r == 32)
		return val;
	else
		return (int)(((deUint32)val >> r) | ((deUint32)val << (32-r)));
}

/*--------------------------------------------------------------------*//*!
 * \brief Integer rotate left.
 * \param val	Value to rotate.
 * \param r		Number of bits to rotate (in range [0, 32]).
 * \return The rotated value.
 *//*--------------------------------------------------------------------*/
DE_INLINE int deRol32 (int val, int r)
{
	DE_ASSERT(r >= 0 && r <= 32);
	if (r == 0 || r == 32)
		return val;
	else
		return (int)(((deUint32)val << r) | ((deUint32)val >> (32-r)));
}

/*--------------------------------------------------------------------*//*!
 * \brief Check if a value is a power-of-two.
 * \param a Input value.
 * \return True if input is a power-of-two value, false otherwise.
 *
 * \note Also returns true for zero.
 *//*--------------------------------------------------------------------*/
DE_INLINE deBool deIsPowerOfTwo32 (int a)
{
	return ((a & (a - 1)) == 0);
}

/*--------------------------------------------------------------------*//*!
 * \brief Check if a value is a power-of-two.
 * \param a Input value.
 * \return True if input is a power-of-two value, false otherwise.
 *
 * \note Also returns true for zero.
 *//*--------------------------------------------------------------------*/
DE_INLINE deBool deIsPowerOfTwo64 (deUint64 a)
{
	return ((a & (a - 1ull)) == 0);
}

/*--------------------------------------------------------------------*//*!
 * \brief Check if a value is a power-of-two.
 * \param a Input value.
 * \return True if input is a power-of-two value, false otherwise.
 *
 * \note Also returns true for zero.
 *//*--------------------------------------------------------------------*/
DE_INLINE deBool deIsPowerOfTwoSize (size_t a)
{
#if (DE_PTR_SIZE == 4)
	return deIsPowerOfTwo32(a);
#elif (DE_PTR_SIZE == 8)
	return deIsPowerOfTwo64(a);
#else
#	error "Invalid DE_PTR_SIZE"
#endif
}

/*--------------------------------------------------------------------*//*!
 * \brief Roud a value up to a power-of-two.
 * \param a Input value.
 * \return Smallest power-of-two value that is greater or equal to an input value.
 *//*--------------------------------------------------------------------*/
DE_INLINE deUint32 deSmallestGreaterOrEquallPowerOfTwoU32 (deUint32 a)
{
	--a;
	a |= a >> 1u;
	a |= a >> 2u;
	a |= a >> 4u;
	a |= a >> 8u;
	a |= a >> 16u;
	return ++a;
}

/*--------------------------------------------------------------------*//*!
 * \brief Roud a value up to a power-of-two.
 * \param a Input value.
 * \return Smallest power-of-two value that is greater or equal to an input value.
 *//*--------------------------------------------------------------------*/
DE_INLINE deUint64 deSmallestGreaterOrEquallPowerOfTwoU64 (deUint64 a)
{
	--a;
	a |= a >> 1u;
	a |= a >> 2u;
	a |= a >> 4u;
	a |= a >> 8u;
	a |= a >> 16u;
	a |= a >> 32u;
	return ++a;
}

/*--------------------------------------------------------------------*//*!
 * \brief Roud a value up to a power-of-two.
 * \param a Input value.
 * \return Smallest power-of-two value that is greater or equal to an input value.
 *//*--------------------------------------------------------------------*/
DE_INLINE size_t deSmallestGreaterOrEquallPowerOfTwoSize (size_t a)
{
#if (DE_PTR_SIZE == 4)
	return deSmallestGreaterOrEquallPowerOfTwoU32(a);
#elif (DE_PTR_SIZE == 8)
	return deSmallestGreaterOrEquallPowerOfTwoU64(a);
#else
#	error "Invalid DE_PTR_SIZE"
#endif
}

/*--------------------------------------------------------------------*//*!
 * \brief Check if an integer is aligned to given power-of-two size.
 * \param a		Input value.
 * \param align	Alignment to check for.
 * \return True if input is aligned, false otherwise.
 *//*--------------------------------------------------------------------*/
DE_INLINE deBool deIsAligned32 (int a, int align)
{
	DE_ASSERT(deIsPowerOfTwo32(align));
	return ((a & (align-1)) == 0);
}

/*--------------------------------------------------------------------*//*!
 * \brief Check if an integer is aligned to given power-of-two size.
 * \param a		Input value.
 * \param align	Alignment to check for.
 * \return True if input is aligned, false otherwise.
 *//*--------------------------------------------------------------------*/
DE_INLINE deBool deIsAligned64 (deInt64 a, deInt64 align)
{
	DE_ASSERT(deIsPowerOfTwo64(align));
	return ((a & (align-1)) == 0);
}

/*--------------------------------------------------------------------*//*!
 * \brief Check if a pointer is aligned to given power-of-two size.
 * \param ptr	Input pointer.
 * \param align	Alignment to check for (power-of-two).
 * \return True if input is aligned, false otherwise.
 *//*--------------------------------------------------------------------*/
DE_INLINE deBool deIsAlignedPtr (const void* ptr, deUintptr align)
{
	DE_ASSERT((align & (align-1)) == 0); /* power of two */
	return (((deUintptr)ptr & (align-1)) == 0);
}

/*--------------------------------------------------------------------*//*!
 * \brief Align an integer to given power-of-two size.
 * \param val	Input to align.
 * \param align	Alignment to check for (power-of-two).
 * \return The aligned value (larger or equal to input).
 *//*--------------------------------------------------------------------*/
DE_INLINE deInt32 deAlign32 (deInt32 val, deInt32 align)
{
	DE_ASSERT(deIsPowerOfTwo32(align));
	return (val + align - 1) & ~(align - 1);
}

/*--------------------------------------------------------------------*//*!
 * \brief Align an integer to given power-of-two size.
 * \param val	Input to align.
 * \param align	Alignment to check for (power-of-two).
 * \return The aligned value (larger or equal to input).
 *//*--------------------------------------------------------------------*/
DE_INLINE deInt64 deAlign64 (deInt64 val, deInt64 align)
{
	DE_ASSERT(deIsPowerOfTwo64(align));
	return (val + align - 1) & ~(align - 1);
}

/*--------------------------------------------------------------------*//*!
 * \brief Align a pointer to given power-of-two size.
 * \param ptr	Input pointer to align.
 * \param align	Alignment to check for (power-of-two).
 * \return The aligned pointer (larger or equal to input).
 *//*--------------------------------------------------------------------*/
DE_INLINE void* deAlignPtr (void* ptr, deUintptr align)
{
	deUintptr val = (deUintptr)ptr;
	DE_ASSERT((align & (align-1)) == 0); /* power of two */
	return (void*)((val + align - 1) & ~(align - 1));
}

/*--------------------------------------------------------------------*//*!
 * \brief Align a size_t value to given power-of-two size.
 * \param ptr	Input value to align.
 * \param align	Alignment to check for (power-of-two).
 * \return The aligned size (larger or equal to input).
 *//*--------------------------------------------------------------------*/
DE_INLINE size_t deAlignSize (size_t val, size_t align)
{
	DE_ASSERT(deIsPowerOfTwoSize(align));
	return (val + align - 1) & ~(align - 1);
}

extern const deInt8 g_clzLUT[256];

/*--------------------------------------------------------------------*//*!
 * \brief Compute number of leading zeros in an integer.
 * \param a	Input value.
 * \return The number of leading zero bits in the input.
 *//*--------------------------------------------------------------------*/
DE_INLINE int deClz32 (deUint32 a)
{
#if (DE_COMPILER == DE_COMPILER_MSC)
	unsigned long i;
	if (_BitScanReverse(&i, (unsigned long)a) == 0)
		return 32;
	else
		return 31-i;
#elif (DE_COMPILER == DE_COMPILER_GCC) || (DE_COMPILER == DE_COMPILER_CLANG)
	if (a == 0)
		return 32;
	else
		return __builtin_clz((unsigned int)a);
#else
	if ((a & 0xFF000000u) != 0)
		return (int)g_clzLUT[a >> 24];
	if ((a & 0x00FF0000u) != 0)
		return 8 + (int)g_clzLUT[a >> 16];
	if ((a & 0x0000FF00u) != 0)
		return 16 + (int)g_clzLUT[a >> 8];
	return 24 + (int)g_clzLUT[a];
#endif
}

extern const deInt8 g_ctzLUT[256];

/*--------------------------------------------------------------------*//*!
 * \brief Compute number of trailing zeros in an integer.
 * \param a	Input value.
 * \return The number of trailing zero bits in the input.
 *//*--------------------------------------------------------------------*/
DE_INLINE int deCtz32 (deUint32 a)
{
#if (DE_COMPILER == DE_COMPILER_MSC)
	unsigned long i;
	if (_BitScanForward(&i, (unsigned long)a) == 0)
		return 32;
	else
		return i;
#elif (DE_COMPILER == DE_COMPILER_GCC) || (DE_COMPILER == DE_COMPILER_CLANG)
	if (a == 0)
		return 32;
	else
		return __builtin_ctz((unsigned int)a);
#else
	if ((a & 0x00FFFFFFu) == 0)
		return (int)g_ctzLUT[a >> 24] + 24;
	if ((a & 0x0000FFFFu) == 0)
		return (int)g_ctzLUT[(a >> 16) & 0xffu] + 16;
	if ((a & 0x000000FFu) == 0)
		return (int)g_ctzLUT[(a >> 8) & 0xffu] + 8;
	return (int)g_ctzLUT[a & 0xffu];
#endif
}

/*--------------------------------------------------------------------*//*!
 * \brief Compute integer 'floor' of 'log2' for a positive integer.
 * \param a	Input value.
 * \return floor(log2(a)).
 *//*--------------------------------------------------------------------*/
DE_INLINE int deLog2Floor32 (deInt32 a)
{
	DE_ASSERT(a > 0);
	return 31 - deClz32((deUint32)a);
}

/*--------------------------------------------------------------------*//*!
 * \brief Compute integer 'ceil' of 'log2' for a positive integer.
 * \param a	Input value.
 * \return ceil(log2(a)).
 *//*--------------------------------------------------------------------*/
DE_INLINE int deLog2Ceil32 (deInt32 a)
{
	int log2floor = deLog2Floor32(a);
	if (deIsPowerOfTwo32(a))
		return log2floor;
	else
		return log2floor+1;
}

/*--------------------------------------------------------------------*//*!
 * \brief Compute the bit population count of an integer.
 * \param a	Input value.
 * \return The number of one bits in the input.
 *//*--------------------------------------------------------------------*/
DE_INLINE int dePop32 (deUint32 a)
{
	deUint32 mask0 = 0x55555555; /* 1-bit values. */
	deUint32 mask1 = 0x33333333; /* 2-bit values. */
	deUint32 mask2 = 0x0f0f0f0f; /* 4-bit values. */
	deUint32 mask3 = 0x00ff00ff; /* 8-bit values. */
	deUint32 mask4 = 0x0000ffff; /* 16-bit values. */
	deUint32 t = (deUint32)a;
	t = (t & mask0) + ((t>>1) & mask0);
	t = (t & mask1) + ((t>>2) & mask1);
	t = (t & mask2) + ((t>>4) & mask2);
	t = (t & mask3) + ((t>>8) & mask3);
	t = (t & mask4) + (t>>16);
	return (int)t;
}

DE_INLINE int dePop64 (deUint64 a)
{
	return dePop32((deUint32)(a & 0xffffffffull)) + dePop32((deUint32)(a >> 32));
}

/*--------------------------------------------------------------------*//*!
 * \brief Reverse bytes in 32-bit integer (for example MSB -> LSB).
 * \param a	Input value.
 * \return The input with bytes reversed
 *//*--------------------------------------------------------------------*/
DE_INLINE deUint32 deReverseBytes32 (deUint32 v)
{
	deUint32 b0 = v << 24;
	deUint32 b1 = (v & 0x0000ff00) << 8;
	deUint32 b2 = (v & 0x00ff0000) >> 8;
	deUint32 b3 = v >> 24;
	return b0|b1|b2|b3;
}

/*--------------------------------------------------------------------*//*!
 * \brief Reverse bytes in 16-bit integer (for example MSB -> LSB).
 * \param a	Input value.
 * \return The input with bytes reversed
 *//*--------------------------------------------------------------------*/
DE_INLINE deUint16 deReverseBytes16 (deUint16 v)
{
	return (deUint16)((v << 8) | (v >> 8));
}

DE_INLINE deInt32 deSafeMul32 (deInt32 a, deInt32 b)
{
	deInt32 res = a * b;
	DE_ASSERT((deInt64)res == ((deInt64)a * (deInt64)b));
	return res;
}

DE_INLINE deInt32 deSafeAdd32 (deInt32 a, deInt32 b)
{
	DE_ASSERT((deInt64)a + (deInt64)b == (deInt64)(a + b));
	return (a + b);
}

DE_INLINE deInt32 deDivRoundUp32 (deInt32 a, deInt32 b)
{
	return a/b + ((a%b) ? 1 : 0);
}

/* \todo [petri] Move to deInt64.h? */

DE_INLINE deInt32 deMulAsr32 (deInt32 a, deInt32 b, int shift)
{
	return (deInt32)(((deInt64)a * (deInt64)b) >> shift);
}

DE_INLINE deInt32 deSafeMulAsr32 (deInt32 a, deInt32 b, int shift)
{
	deInt64 res = ((deInt64)a * (deInt64)b) >> shift;
	DE_ASSERT(res == (deInt64)(deInt32)res);
	return (deInt32)res;
}

DE_INLINE deUint32 deSafeMuluAsr32 (deUint32 a, deUint32 b, int shift)
{
	deUint64 res = ((deUint64)a * (deUint64)b) >> shift;
	DE_ASSERT(res == (deUint64)(deUint32)res);
	return (deUint32)res;
}

DE_INLINE deInt64 deMul32_32_64 (deInt32 a, deInt32 b)
{
	return ((deInt64)a * (deInt64)b);
}

DE_INLINE deInt64 deAbs64 (deInt64 a)
{
	DE_ASSERT((deUint64) a != 0x8000000000000000LL);
	return (a >= 0) ? a : -a;
}

DE_INLINE int deClz64 (deUint64 a)
{
	if ((a >> 32) != 0)
		return deClz32((deUint32)(a >> 32));
	return deClz32((deUint32)a) + 32;
}

/* Common hash & compare functions. */

DE_INLINE deUint32 deInt32Hash (deInt32 a)
{
	/* From: http://www.concentric.net/~Ttwang/tech/inthash.htm */
	deUint32 key = (deUint32)a;
	key = (key ^ 61) ^ (key >> 16);
	key = key + (key << 3);
	key = key ^ (key >> 4);
	key = key * 0x27d4eb2d; /* prime/odd constant */
	key = key ^ (key >> 15);
	return key;
}

DE_INLINE deUint32 deInt64Hash (deInt64 a)
{
	/* From: http://www.concentric.net/~Ttwang/tech/inthash.htm */
	deUint64 key = (deUint64)a;
	key = (~key) + (key << 21); /* key = (key << 21) - key - 1; */
	key = key ^ (key >> 24);
	key = (key + (key << 3)) + (key << 8); /* key * 265 */
	key = key ^ (key >> 14);
	key = (key + (key << 2)) + (key << 4); /* key * 21 */
	key = key ^ (key >> 28);
	key = key + (key << 31);
	return (deUint32)key;
}

DE_INLINE deUint32	deInt16Hash		(deInt16 v)					{ return deInt32Hash(v);			}
DE_INLINE deUint32	deUint16Hash	(deUint16 v)				{ return deInt32Hash((deInt32)v);	}
DE_INLINE deUint32	deUint32Hash	(deUint32 v)				{ return deInt32Hash((deInt32)v);	}
DE_INLINE deUint32	deUint64Hash	(deUint64 v)				{ return deInt64Hash((deInt64)v);	}

DE_INLINE deBool	deInt16Equal	(deInt16 a, deInt16 b)		{ return (a == b);	}
DE_INLINE deBool	deUint16Equal	(deUint16 a, deUint16 b)	{ return (a == b);	}
DE_INLINE deBool	deInt32Equal	(deInt32 a, deInt32 b)		{ return (a == b);	}
DE_INLINE deBool	deUint32Equal	(deUint32 a, deUint32 b)	{ return (a == b);	}
DE_INLINE deBool	deInt64Equal	(deInt64 a, deInt64 b)		{ return (a == b);	}
DE_INLINE deBool	deUint64Equal	(deUint64 a, deUint64 b)	{ return (a == b);	}

DE_INLINE deUint32	dePointerHash (const void* ptr)
{
	deUintptr val = (deUintptr)ptr;
#if (DE_PTR_SIZE == 4)
	return deInt32Hash((int)val);
#elif (DE_PTR_SIZE == 8)
	return deInt64Hash((deInt64)val);
#else
#	error Unsupported pointer size.
#endif
}

DE_INLINE deBool dePointerEqual (const void* a, const void* b)
{
	return (a == b);
}

/**
 *	\brief	Modulo that generates the same sign as divisor and rounds toward
 *			negative infinity -- assuming c99 %-operator.
 */
DE_INLINE deInt32 deInt32ModF (deInt32 n, deInt32 d)
{
	deInt32 r = n%d;
	if ((r > 0 && d < 0) || (r < 0 && d > 0)) r = r+d;
	return r;
}

DE_INLINE deBool deInt64InInt32Range (deInt64 x)
{
	return ((x >= (((deInt64)((deInt32)(-0x7FFFFFFF - 1))))) && (x <= ((1ll<<31)-1)));
}


DE_INLINE deUint32 deBitMask32 (int leastSignificantBitNdx, int numBits)
{
	DE_ASSERT(deInRange32(leastSignificantBitNdx, 0, 32));
	DE_ASSERT(deInRange32(numBits, 0, 32));
	DE_ASSERT(deInRange32(leastSignificantBitNdx+numBits, 0, 32));

	if (numBits < 32 && leastSignificantBitNdx < 32)
		return ((1u<<numBits)-1u) << (deUint32)leastSignificantBitNdx;
	else if (numBits == 0 && leastSignificantBitNdx == 32)
		return 0u;
	else
	{
		DE_ASSERT(numBits == 32 && leastSignificantBitNdx == 0);
		return 0xFFFFFFFFu;
	}
}

DE_INLINE deUint32 deUintMaxValue32 (int numBits)
{
	DE_ASSERT(deInRange32(numBits, 1, 32));
	if (numBits < 32)
		return ((1u<<numBits)-1u);
	else
		return 0xFFFFFFFFu;
}

DE_INLINE deInt32 deIntMaxValue32 (int numBits)
{
	DE_ASSERT(deInRange32(numBits, 1, 32));
	if (numBits < 32)
		return ((deInt32)1 << (numBits - 1)) - 1;
	else
	{
		/* avoid undefined behavior of int overflow when shifting */
		return 0x7FFFFFFF;
	}
}

DE_INLINE deInt32 deIntMinValue32 (int numBits)
{
	DE_ASSERT(deInRange32(numBits, 1, 32));
	if (numBits < 32)
		return -((deInt32)1 << (numBits - 1));
	else
	{
		/* avoid undefined behavior of int overflow when shifting */
		return (deInt32)(-0x7FFFFFFF - 1);
	}
}

DE_INLINE deInt32 deSignExtendTo32 (deInt32 value, int numBits)
{
	DE_ASSERT(deInRange32(numBits, 1, 32));

	if (numBits < 32)
	{
		deBool		signSet		= ((deUint32)value & (1u<<(numBits-1))) != 0;
		deUint32	signMask	= deBitMask32(numBits, 32-numBits);

		DE_ASSERT(((deUint32)value & signMask) == 0u);

		return (deInt32)((deUint32)value | (signSet ? signMask : 0u));
	}
	else
		return value;
}

DE_END_EXTERN_C

#endif /* _DEINT32_H */
