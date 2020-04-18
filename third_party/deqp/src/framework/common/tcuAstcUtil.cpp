/*-------------------------------------------------------------------------
 * drawElements Quality Program Tester Core
 * ----------------------------------------
 *
 * Copyright 2016 The Android Open Source Project
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
 * \brief ASTC Utilities.
 *//*--------------------------------------------------------------------*/

#include "tcuAstcUtil.hpp"
#include "deFloat16.h"
#include "deRandom.hpp"
#include "deMeta.hpp"

#include <algorithm>

namespace tcu
{
namespace astc
{

using std::vector;

namespace
{

// Common utilities

enum
{
	MAX_BLOCK_WIDTH		= 12,
	MAX_BLOCK_HEIGHT	= 12
};

inline deUint32 getBit (deUint32 src, int ndx)
{
	DE_ASSERT(de::inBounds(ndx, 0, 32));
	return (src >> ndx) & 1;
}

inline deUint32 getBits (deUint32 src, int low, int high)
{
	const int numBits = (high-low) + 1;

	DE_ASSERT(de::inRange(numBits, 1, 32));

	if (numBits < 32)
		return (deUint32)((src >> low) & ((1u<<numBits)-1));
	else
		return (deUint32)((src >> low) & 0xFFFFFFFFu);
}

inline bool isBitSet (deUint32 src, int ndx)
{
	return getBit(src, ndx) != 0;
}

inline deUint32 reverseBits (deUint32 src, int numBits)
{
	DE_ASSERT(de::inRange(numBits, 0, 32));
	deUint32 result = 0;
	for (int i = 0; i < numBits; i++)
		result |= ((src >> i) & 1) << (numBits-1-i);
	return result;
}

inline deUint32 bitReplicationScale (deUint32 src, int numSrcBits, int numDstBits)
{
	DE_ASSERT(numSrcBits <= numDstBits);
	DE_ASSERT((src & ((1<<numSrcBits)-1)) == src);
	deUint32 dst = 0;
	for (int shift = numDstBits-numSrcBits; shift > -numSrcBits; shift -= numSrcBits)
		dst |= shift >= 0 ? src << shift : src >> -shift;
	return dst;
}

inline deInt32 signExtend (deInt32 src, int numSrcBits)
{
	DE_ASSERT(de::inRange(numSrcBits, 2, 31));
	const bool negative = (src & (1 << (numSrcBits-1))) != 0;
	return src | (negative ? ~((1 << numSrcBits) - 1) : 0);
}

inline bool isFloat16InfOrNan (deFloat16 v)
{
	return getBits(v, 10, 14) == 31;
}

enum ISEMode
{
	ISEMODE_TRIT = 0,
	ISEMODE_QUINT,
	ISEMODE_PLAIN_BIT,

	ISEMODE_LAST
};

struct ISEParams
{
	ISEMode		mode;
	int			numBits;

	ISEParams (ISEMode mode_, int numBits_) : mode(mode_), numBits(numBits_) {}
};

inline int computeNumRequiredBits (const ISEParams& iseParams, int numValues)
{
	switch (iseParams.mode)
	{
		case ISEMODE_TRIT:			return deDivRoundUp32(numValues*8, 5) + numValues*iseParams.numBits;
		case ISEMODE_QUINT:			return deDivRoundUp32(numValues*7, 3) + numValues*iseParams.numBits;
		case ISEMODE_PLAIN_BIT:		return numValues*iseParams.numBits;
		default:
			DE_ASSERT(false);
			return -1;
	}
}

ISEParams computeMaximumRangeISEParams (int numAvailableBits, int numValuesInSequence)
{
	int curBitsForTritMode		= 6;
	int curBitsForQuintMode		= 5;
	int curBitsForPlainBitMode	= 8;

	while (true)
	{
		DE_ASSERT(curBitsForTritMode > 0 || curBitsForQuintMode > 0 || curBitsForPlainBitMode > 0);

		const int tritRange			= curBitsForTritMode > 0		? (3 << curBitsForTritMode) - 1			: -1;
		const int quintRange		= curBitsForQuintMode > 0		? (5 << curBitsForQuintMode) - 1		: -1;
		const int plainBitRange		= curBitsForPlainBitMode > 0	? (1 << curBitsForPlainBitMode) - 1		: -1;
		const int maxRange			= de::max(de::max(tritRange, quintRange), plainBitRange);

		if (maxRange == tritRange)
		{
			const ISEParams params(ISEMODE_TRIT, curBitsForTritMode);
			if (computeNumRequiredBits(params, numValuesInSequence) <= numAvailableBits)
				return ISEParams(ISEMODE_TRIT, curBitsForTritMode);
			curBitsForTritMode--;
		}
		else if (maxRange == quintRange)
		{
			const ISEParams params(ISEMODE_QUINT, curBitsForQuintMode);
			if (computeNumRequiredBits(params, numValuesInSequence) <= numAvailableBits)
				return ISEParams(ISEMODE_QUINT, curBitsForQuintMode);
			curBitsForQuintMode--;
		}
		else
		{
			const ISEParams params(ISEMODE_PLAIN_BIT, curBitsForPlainBitMode);
			DE_ASSERT(maxRange == plainBitRange);
			if (computeNumRequiredBits(params, numValuesInSequence) <= numAvailableBits)
				return ISEParams(ISEMODE_PLAIN_BIT, curBitsForPlainBitMode);
			curBitsForPlainBitMode--;
		}
	}
}

inline int computeNumColorEndpointValues (deUint32 endpointMode)
{
	DE_ASSERT(endpointMode < 16);
	return (endpointMode/4 + 1) * 2;
}

// Decompression utilities

enum DecompressResult
{
	DECOMPRESS_RESULT_VALID_BLOCK	= 0,	//!< Decompressed valid block
	DECOMPRESS_RESULT_ERROR,				//!< Encountered error while decompressing, error color written

	DECOMPRESS_RESULT_LAST
};

// A helper for getting bits from a 128-bit block.
class Block128
{
private:
	typedef deUint64 Word;

	enum
	{
		WORD_BYTES	= sizeof(Word),
		WORD_BITS	= 8*WORD_BYTES,
		NUM_WORDS	= 128 / WORD_BITS
	};

	DE_STATIC_ASSERT(128 % WORD_BITS == 0);

public:
	Block128 (const deUint8* src)
	{
		for (int wordNdx = 0; wordNdx < NUM_WORDS; wordNdx++)
		{
			m_words[wordNdx] = 0;
			for (int byteNdx = 0; byteNdx < WORD_BYTES; byteNdx++)
				m_words[wordNdx] |= (Word)src[wordNdx*WORD_BYTES + byteNdx] << (8*byteNdx);
		}
	}

	deUint32 getBit (int ndx) const
	{
		DE_ASSERT(de::inBounds(ndx, 0, 128));
		return (m_words[ndx / WORD_BITS] >> (ndx % WORD_BITS)) & 1;
	}

	deUint32 getBits (int low, int high) const
	{
		DE_ASSERT(de::inBounds(low, 0, 128));
		DE_ASSERT(de::inBounds(high, 0, 128));
		DE_ASSERT(de::inRange(high-low+1, 0, 32));

		if (high-low+1 == 0)
			return 0;

		const int word0Ndx = low / WORD_BITS;
		const int word1Ndx = high / WORD_BITS;

		// \note "foo << bar << 1" done instead of "foo << (bar+1)" to avoid overflow, i.e. shift amount being too big.

		if (word0Ndx == word1Ndx)
			return (deUint32)((m_words[word0Ndx] & ((((Word)1 << high%WORD_BITS << 1) - 1))) >> ((Word)low % WORD_BITS));
		else
		{
			DE_ASSERT(word1Ndx == word0Ndx + 1);

			return (deUint32)(m_words[word0Ndx] >> (low%WORD_BITS)) |
				   (deUint32)((m_words[word1Ndx] & (((Word)1 << high%WORD_BITS << 1) - 1)) << (high-low - high%WORD_BITS));
		}
	}

	bool isBitSet (int ndx) const
	{
		DE_ASSERT(de::inBounds(ndx, 0, 128));
		return getBit(ndx) != 0;
	}

private:
	Word m_words[NUM_WORDS];
};

// A helper for sequential access into a Block128.
class BitAccessStream
{
public:
	BitAccessStream (const Block128& src, int startNdxInSrc, int length, bool forward)
		: m_src				(src)
		, m_startNdxInSrc	(startNdxInSrc)
		, m_length			(length)
		, m_forward			(forward)
		, m_ndx				(0)
	{
	}

	// Get the next num bits. Bits at positions greater than or equal to m_length are zeros.
	deUint32 getNext (int num)
	{
		if (num == 0 || m_ndx >= m_length)
			return 0;

		const int end				= m_ndx + num;
		const int numBitsFromSrc	= de::max(0, de::min(m_length, end) - m_ndx);
		const int low				= m_ndx;
		const int high				= m_ndx + numBitsFromSrc - 1;

		m_ndx += num;

		return m_forward ?			   m_src.getBits(m_startNdxInSrc + low,  m_startNdxInSrc + high)
						 : reverseBits(m_src.getBits(m_startNdxInSrc - high, m_startNdxInSrc - low), numBitsFromSrc);
	}

private:
	const Block128&		m_src;
	const int			m_startNdxInSrc;
	const int			m_length;
	const bool			m_forward;

	int					m_ndx;
};

struct ISEDecodedResult
{
	deUint32 m;
	deUint32 tq; //!< Trit or quint value, depending on ISE mode.
	deUint32 v;
};

// Data from an ASTC block's "block mode" part (i.e. bits [0,10]).
struct ASTCBlockMode
{
	bool		isError;
	// \note Following fields only relevant if !isError.
	bool		isVoidExtent;
	// \note Following fields only relevant if !isVoidExtent.
	bool		isDualPlane;
	int			weightGridWidth;
	int			weightGridHeight;
	ISEParams	weightISEParams;

	ASTCBlockMode (void)
		: isError			(true)
		, isVoidExtent		(true)
		, isDualPlane		(true)
		, weightGridWidth	(-1)
		, weightGridHeight	(-1)
		, weightISEParams	(ISEMODE_LAST, -1)
	{
	}
};

inline int computeNumWeights (const ASTCBlockMode& mode)
{
	return mode.weightGridWidth * mode.weightGridHeight * (mode.isDualPlane ? 2 : 1);
}

struct ColorEndpointPair
{
	UVec4 e0;
	UVec4 e1;
};

struct TexelWeightPair
{
	deUint32 w[2];
};

ASTCBlockMode getASTCBlockMode (deUint32 blockModeData)
{
	ASTCBlockMode blockMode;
	blockMode.isError = true; // \note Set to false later, if not error.

	blockMode.isVoidExtent = getBits(blockModeData, 0, 8) == 0x1fc;

	if (!blockMode.isVoidExtent)
	{
		if ((getBits(blockModeData, 0, 1) == 0 && getBits(blockModeData, 6, 8) == 7) || getBits(blockModeData, 0, 3) == 0)
			return blockMode; // Invalid ("reserved").

		deUint32 r = (deUint32)-1; // \note Set in the following branches.

		if (getBits(blockModeData, 0, 1) == 0)
		{
			const deUint32 r0	= getBit(blockModeData, 4);
			const deUint32 r1	= getBit(blockModeData, 2);
			const deUint32 r2	= getBit(blockModeData, 3);
			const deUint32 i78	= getBits(blockModeData, 7, 8);

			r = (r2 << 2) | (r1 << 1) | (r0 << 0);

			if (i78 == 3)
			{
				const bool i5 = isBitSet(blockModeData, 5);
				blockMode.weightGridWidth	= i5 ? 10 : 6;
				blockMode.weightGridHeight	= i5 ? 6  : 10;
			}
			else
			{
				const deUint32 a = getBits(blockModeData, 5, 6);
				switch (i78)
				{
					case 0:		blockMode.weightGridWidth = 12;		blockMode.weightGridHeight = a + 2;									break;
					case 1:		blockMode.weightGridWidth = a + 2;	blockMode.weightGridHeight = 12;									break;
					case 2:		blockMode.weightGridWidth = a + 6;	blockMode.weightGridHeight = getBits(blockModeData, 9, 10) + 6;		break;
					default: DE_ASSERT(false);
				}
			}
		}
		else
		{
			const deUint32 r0	= getBit(blockModeData, 4);
			const deUint32 r1	= getBit(blockModeData, 0);
			const deUint32 r2	= getBit(blockModeData, 1);
			const deUint32 i23	= getBits(blockModeData, 2, 3);
			const deUint32 a	= getBits(blockModeData, 5, 6);

			r = (r2 << 2) | (r1 << 1) | (r0 << 0);

			if (i23 == 3)
			{
				const deUint32	b	= getBit(blockModeData, 7);
				const bool		i8	= isBitSet(blockModeData, 8);
				blockMode.weightGridWidth	= i8 ? b+2 : a+2;
				blockMode.weightGridHeight	= i8 ? a+2 : b+6;
			}
			else
			{
				const deUint32 b = getBits(blockModeData, 7, 8);

				switch (i23)
				{
					case 0:		blockMode.weightGridWidth = b + 4;	blockMode.weightGridHeight = a + 2;	break;
					case 1:		blockMode.weightGridWidth = b + 8;	blockMode.weightGridHeight = a + 2;	break;
					case 2:		blockMode.weightGridWidth = a + 2;	blockMode.weightGridHeight = b + 8;	break;
					default: DE_ASSERT(false);
				}
			}
		}

		const bool	zeroDH		= getBits(blockModeData, 0, 1) == 0 && getBits(blockModeData, 7, 8) == 2;
		const bool	h			= zeroDH ? 0 : isBitSet(blockModeData, 9);
		blockMode.isDualPlane	= zeroDH ? 0 : isBitSet(blockModeData, 10);

		{
			ISEMode&	m	= blockMode.weightISEParams.mode;
			int&		b	= blockMode.weightISEParams.numBits;
			m = ISEMODE_PLAIN_BIT;
			b = 0;

			if (h)
			{
				switch (r)
				{
					case 2:							m = ISEMODE_QUINT;	b = 1;	break;
					case 3:		m = ISEMODE_TRIT;						b = 2;	break;
					case 4:												b = 4;	break;
					case 5:							m = ISEMODE_QUINT;	b = 2;	break;
					case 6:		m = ISEMODE_TRIT;						b = 3;	break;
					case 7:												b = 5;	break;
					default:	DE_ASSERT(false);
				}
			}
			else
			{
				switch (r)
				{
					case 2:												b = 1;	break;
					case 3:		m = ISEMODE_TRIT;								break;
					case 4:												b = 2;	break;
					case 5:							m = ISEMODE_QUINT;			break;
					case 6:		m = ISEMODE_TRIT;						b = 1;	break;
					case 7:												b = 3;	break;
					default:	DE_ASSERT(false);
				}
			}
		}
	}

	blockMode.isError = false;
	return blockMode;
}

inline void setASTCErrorColorBlock (void* dst, int blockWidth, int blockHeight, bool isSRGB)
{
	if (isSRGB)
	{
		deUint8* const dstU = (deUint8*)dst;

		for (int i = 0; i < blockWidth*blockHeight; i++)
		{
			dstU[4*i + 0] = 0xff;
			dstU[4*i + 1] = 0;
			dstU[4*i + 2] = 0xff;
			dstU[4*i + 3] = 0xff;
		}
	}
	else
	{
		float* const dstF = (float*)dst;

		for (int i = 0; i < blockWidth*blockHeight; i++)
		{
			dstF[4*i + 0] = 1.0f;
			dstF[4*i + 1] = 0.0f;
			dstF[4*i + 2] = 1.0f;
			dstF[4*i + 3] = 1.0f;
		}
	}
}

DecompressResult decodeVoidExtentBlock (void* dst, const Block128& blockData, int blockWidth, int blockHeight, bool isSRGB, bool isLDRMode)
{
	const deUint32	minSExtent			= blockData.getBits(12, 24);
	const deUint32	maxSExtent			= blockData.getBits(25, 37);
	const deUint32	minTExtent			= blockData.getBits(38, 50);
	const deUint32	maxTExtent			= blockData.getBits(51, 63);
	const bool		allExtentsAllOnes	= minSExtent == 0x1fff && maxSExtent == 0x1fff && minTExtent == 0x1fff && maxTExtent == 0x1fff;
	const bool		isHDRBlock			= blockData.isBitSet(9);

	if ((isLDRMode && isHDRBlock) || (!allExtentsAllOnes && (minSExtent >= maxSExtent || minTExtent >= maxTExtent)))
	{
		setASTCErrorColorBlock(dst, blockWidth, blockHeight, isSRGB);
		return DECOMPRESS_RESULT_ERROR;
	}

	const deUint32 rgba[4] =
	{
		blockData.getBits(64,  79),
		blockData.getBits(80,  95),
		blockData.getBits(96,  111),
		blockData.getBits(112, 127)
	};

	if (isSRGB)
	{
		deUint8* const dstU = (deUint8*)dst;
		for (int i = 0; i < blockWidth*blockHeight; i++)
		for (int c = 0; c < 4; c++)
			dstU[i*4 + c] = (deUint8)((rgba[c] & 0xff00) >> 8);
	}
	else
	{
		float* const dstF = (float*)dst;

		if (isHDRBlock)
		{
			for (int c = 0; c < 4; c++)
			{
				if (isFloat16InfOrNan((deFloat16)rgba[c]))
					throw InternalError("Infinity or NaN color component in HDR void extent block in ASTC texture (behavior undefined by ASTC specification)");
			}

			for (int i = 0; i < blockWidth*blockHeight; i++)
			for (int c = 0; c < 4; c++)
				dstF[i*4 + c] = deFloat16To32((deFloat16)rgba[c]);
		}
		else
		{
			for (int i = 0; i < blockWidth*blockHeight; i++)
			for (int c = 0; c < 4; c++)
				dstF[i*4 + c] = rgba[c] == 65535 ? 1.0f : (float)rgba[c] / 65536.0f;
		}
	}

	return DECOMPRESS_RESULT_VALID_BLOCK;
}

void decodeColorEndpointModes (deUint32* endpointModesDst, const Block128& blockData, int numPartitions, int extraCemBitsStart)
{
	if (numPartitions == 1)
		endpointModesDst[0] = blockData.getBits(13, 16);
	else
	{
		const deUint32 highLevelSelector = blockData.getBits(23, 24);

		if (highLevelSelector == 0)
		{
			const deUint32 mode = blockData.getBits(25, 28);
			for (int i = 0; i < numPartitions; i++)
				endpointModesDst[i] = mode;
		}
		else
		{
			for (int partNdx = 0; partNdx < numPartitions; partNdx++)
			{
				const deUint32 cemClass		= highLevelSelector - (blockData.isBitSet(25 + partNdx) ? 0 : 1);
				const deUint32 lowBit0Ndx	= numPartitions + 2*partNdx;
				const deUint32 lowBit1Ndx	= numPartitions + 2*partNdx + 1;
				const deUint32 lowBit0		= blockData.getBit(lowBit0Ndx < 4 ? 25+lowBit0Ndx : extraCemBitsStart+lowBit0Ndx-4);
				const deUint32 lowBit1		= blockData.getBit(lowBit1Ndx < 4 ? 25+lowBit1Ndx : extraCemBitsStart+lowBit1Ndx-4);

				endpointModesDst[partNdx] = (cemClass << 2) | (lowBit1 << 1) | lowBit0;
			}
		}
	}
}

int computeNumColorEndpointValues (const deUint32* endpointModes, int numPartitions)
{
	int result = 0;
	for (int i = 0; i < numPartitions; i++)
		result += computeNumColorEndpointValues(endpointModes[i]);
	return result;
}

void decodeISETritBlock (ISEDecodedResult* dst, int numValues, BitAccessStream& data, int numBits)
{
	DE_ASSERT(de::inRange(numValues, 1, 5));

	deUint32 m[5];

	m[0]			= data.getNext(numBits);
	deUint32 T01	= data.getNext(2);
	m[1]			= data.getNext(numBits);
	deUint32 T23	= data.getNext(2);
	m[2]			= data.getNext(numBits);
	deUint32 T4		= data.getNext(1);
	m[3]			= data.getNext(numBits);
	deUint32 T56	= data.getNext(2);
	m[4]			= data.getNext(numBits);
	deUint32 T7		= data.getNext(1);

	switch (numValues)
	{
		// \note Fall-throughs.
		case 1: T23		= 0;
		case 2: T4		= 0;
		case 3: T56		= 0;
		case 4: T7		= 0;
		case 5: break;
		default:
			DE_ASSERT(false);
	}

	const deUint32 T = (T7 << 7) | (T56 << 5) | (T4 << 4) | (T23 << 2) | (T01 << 0);

	static const deUint32 tritsFromT[256][5] =
	{
		{ 0,0,0,0,0 }, { 1,0,0,0,0 }, { 2,0,0,0,0 }, { 0,0,2,0,0 }, { 0,1,0,0,0 }, { 1,1,0,0,0 }, { 2,1,0,0,0 }, { 1,0,2,0,0 }, { 0,2,0,0,0 }, { 1,2,0,0,0 }, { 2,2,0,0,0 }, { 2,0,2,0,0 }, { 0,2,2,0,0 }, { 1,2,2,0,0 }, { 2,2,2,0,0 }, { 2,0,2,0,0 },
		{ 0,0,1,0,0 }, { 1,0,1,0,0 }, { 2,0,1,0,0 }, { 0,1,2,0,0 }, { 0,1,1,0,0 }, { 1,1,1,0,0 }, { 2,1,1,0,0 }, { 1,1,2,0,0 }, { 0,2,1,0,0 }, { 1,2,1,0,0 }, { 2,2,1,0,0 }, { 2,1,2,0,0 }, { 0,0,0,2,2 }, { 1,0,0,2,2 }, { 2,0,0,2,2 }, { 0,0,2,2,2 },
		{ 0,0,0,1,0 }, { 1,0,0,1,0 }, { 2,0,0,1,0 }, { 0,0,2,1,0 }, { 0,1,0,1,0 }, { 1,1,0,1,0 }, { 2,1,0,1,0 }, { 1,0,2,1,0 }, { 0,2,0,1,0 }, { 1,2,0,1,0 }, { 2,2,0,1,0 }, { 2,0,2,1,0 }, { 0,2,2,1,0 }, { 1,2,2,1,0 }, { 2,2,2,1,0 }, { 2,0,2,1,0 },
		{ 0,0,1,1,0 }, { 1,0,1,1,0 }, { 2,0,1,1,0 }, { 0,1,2,1,0 }, { 0,1,1,1,0 }, { 1,1,1,1,0 }, { 2,1,1,1,0 }, { 1,1,2,1,0 }, { 0,2,1,1,0 }, { 1,2,1,1,0 }, { 2,2,1,1,0 }, { 2,1,2,1,0 }, { 0,1,0,2,2 }, { 1,1,0,2,2 }, { 2,1,0,2,2 }, { 1,0,2,2,2 },
		{ 0,0,0,2,0 }, { 1,0,0,2,0 }, { 2,0,0,2,0 }, { 0,0,2,2,0 }, { 0,1,0,2,0 }, { 1,1,0,2,0 }, { 2,1,0,2,0 }, { 1,0,2,2,0 }, { 0,2,0,2,0 }, { 1,2,0,2,0 }, { 2,2,0,2,0 }, { 2,0,2,2,0 }, { 0,2,2,2,0 }, { 1,2,2,2,0 }, { 2,2,2,2,0 }, { 2,0,2,2,0 },
		{ 0,0,1,2,0 }, { 1,0,1,2,0 }, { 2,0,1,2,0 }, { 0,1,2,2,0 }, { 0,1,1,2,0 }, { 1,1,1,2,0 }, { 2,1,1,2,0 }, { 1,1,2,2,0 }, { 0,2,1,2,0 }, { 1,2,1,2,0 }, { 2,2,1,2,0 }, { 2,1,2,2,0 }, { 0,2,0,2,2 }, { 1,2,0,2,2 }, { 2,2,0,2,2 }, { 2,0,2,2,2 },
		{ 0,0,0,0,2 }, { 1,0,0,0,2 }, { 2,0,0,0,2 }, { 0,0,2,0,2 }, { 0,1,0,0,2 }, { 1,1,0,0,2 }, { 2,1,0,0,2 }, { 1,0,2,0,2 }, { 0,2,0,0,2 }, { 1,2,0,0,2 }, { 2,2,0,0,2 }, { 2,0,2,0,2 }, { 0,2,2,0,2 }, { 1,2,2,0,2 }, { 2,2,2,0,2 }, { 2,0,2,0,2 },
		{ 0,0,1,0,2 }, { 1,0,1,0,2 }, { 2,0,1,0,2 }, { 0,1,2,0,2 }, { 0,1,1,0,2 }, { 1,1,1,0,2 }, { 2,1,1,0,2 }, { 1,1,2,0,2 }, { 0,2,1,0,2 }, { 1,2,1,0,2 }, { 2,2,1,0,2 }, { 2,1,2,0,2 }, { 0,2,2,2,2 }, { 1,2,2,2,2 }, { 2,2,2,2,2 }, { 2,0,2,2,2 },
		{ 0,0,0,0,1 }, { 1,0,0,0,1 }, { 2,0,0,0,1 }, { 0,0,2,0,1 }, { 0,1,0,0,1 }, { 1,1,0,0,1 }, { 2,1,0,0,1 }, { 1,0,2,0,1 }, { 0,2,0,0,1 }, { 1,2,0,0,1 }, { 2,2,0,0,1 }, { 2,0,2,0,1 }, { 0,2,2,0,1 }, { 1,2,2,0,1 }, { 2,2,2,0,1 }, { 2,0,2,0,1 },
		{ 0,0,1,0,1 }, { 1,0,1,0,1 }, { 2,0,1,0,1 }, { 0,1,2,0,1 }, { 0,1,1,0,1 }, { 1,1,1,0,1 }, { 2,1,1,0,1 }, { 1,1,2,0,1 }, { 0,2,1,0,1 }, { 1,2,1,0,1 }, { 2,2,1,0,1 }, { 2,1,2,0,1 }, { 0,0,1,2,2 }, { 1,0,1,2,2 }, { 2,0,1,2,2 }, { 0,1,2,2,2 },
		{ 0,0,0,1,1 }, { 1,0,0,1,1 }, { 2,0,0,1,1 }, { 0,0,2,1,1 }, { 0,1,0,1,1 }, { 1,1,0,1,1 }, { 2,1,0,1,1 }, { 1,0,2,1,1 }, { 0,2,0,1,1 }, { 1,2,0,1,1 }, { 2,2,0,1,1 }, { 2,0,2,1,1 }, { 0,2,2,1,1 }, { 1,2,2,1,1 }, { 2,2,2,1,1 }, { 2,0,2,1,1 },
		{ 0,0,1,1,1 }, { 1,0,1,1,1 }, { 2,0,1,1,1 }, { 0,1,2,1,1 }, { 0,1,1,1,1 }, { 1,1,1,1,1 }, { 2,1,1,1,1 }, { 1,1,2,1,1 }, { 0,2,1,1,1 }, { 1,2,1,1,1 }, { 2,2,1,1,1 }, { 2,1,2,1,1 }, { 0,1,1,2,2 }, { 1,1,1,2,2 }, { 2,1,1,2,2 }, { 1,1,2,2,2 },
		{ 0,0,0,2,1 }, { 1,0,0,2,1 }, { 2,0,0,2,1 }, { 0,0,2,2,1 }, { 0,1,0,2,1 }, { 1,1,0,2,1 }, { 2,1,0,2,1 }, { 1,0,2,2,1 }, { 0,2,0,2,1 }, { 1,2,0,2,1 }, { 2,2,0,2,1 }, { 2,0,2,2,1 }, { 0,2,2,2,1 }, { 1,2,2,2,1 }, { 2,2,2,2,1 }, { 2,0,2,2,1 },
		{ 0,0,1,2,1 }, { 1,0,1,2,1 }, { 2,0,1,2,1 }, { 0,1,2,2,1 }, { 0,1,1,2,1 }, { 1,1,1,2,1 }, { 2,1,1,2,1 }, { 1,1,2,2,1 }, { 0,2,1,2,1 }, { 1,2,1,2,1 }, { 2,2,1,2,1 }, { 2,1,2,2,1 }, { 0,2,1,2,2 }, { 1,2,1,2,2 }, { 2,2,1,2,2 }, { 2,1,2,2,2 },
		{ 0,0,0,1,2 }, { 1,0,0,1,2 }, { 2,0,0,1,2 }, { 0,0,2,1,2 }, { 0,1,0,1,2 }, { 1,1,0,1,2 }, { 2,1,0,1,2 }, { 1,0,2,1,2 }, { 0,2,0,1,2 }, { 1,2,0,1,2 }, { 2,2,0,1,2 }, { 2,0,2,1,2 }, { 0,2,2,1,2 }, { 1,2,2,1,2 }, { 2,2,2,1,2 }, { 2,0,2,1,2 },
		{ 0,0,1,1,2 }, { 1,0,1,1,2 }, { 2,0,1,1,2 }, { 0,1,2,1,2 }, { 0,1,1,1,2 }, { 1,1,1,1,2 }, { 2,1,1,1,2 }, { 1,1,2,1,2 }, { 0,2,1,1,2 }, { 1,2,1,1,2 }, { 2,2,1,1,2 }, { 2,1,2,1,2 }, { 0,2,2,2,2 }, { 1,2,2,2,2 }, { 2,2,2,2,2 }, { 2,1,2,2,2 }
	};

	const deUint32 (& trits)[5] = tritsFromT[T];

	for (int i = 0; i < numValues; i++)
	{
		dst[i].m	= m[i];
		dst[i].tq	= trits[i];
		dst[i].v	= (trits[i] << numBits) + m[i];
	}
}

void decodeISEQuintBlock (ISEDecodedResult* dst, int numValues, BitAccessStream& data, int numBits)
{
	DE_ASSERT(de::inRange(numValues, 1, 3));

	deUint32 m[3];

	m[0]			= data.getNext(numBits);
	deUint32 Q012	= data.getNext(3);
	m[1]			= data.getNext(numBits);
	deUint32 Q34	= data.getNext(2);
	m[2]			= data.getNext(numBits);
	deUint32 Q56	= data.getNext(2);

	switch (numValues)
	{
		// \note Fall-throughs.
		case 1: Q34		= 0;
		case 2: Q56		= 0;
		case 3: break;
		default:
			DE_ASSERT(false);
	}

	const deUint32 Q = (Q56 << 5) | (Q34 << 3) | (Q012 << 0);

	static const deUint32 quintsFromQ[256][3] =
	{
		{ 0,0,0 }, { 1,0,0 }, { 2,0,0 }, { 3,0,0 }, { 4,0,0 }, { 0,4,0 }, { 4,4,0 }, { 4,4,4 }, { 0,1,0 }, { 1,1,0 }, { 2,1,0 }, { 3,1,0 }, { 4,1,0 }, { 1,4,0 }, { 4,4,1 }, { 4,4,4 },
		{ 0,2,0 }, { 1,2,0 }, { 2,2,0 }, { 3,2,0 }, { 4,2,0 }, { 2,4,0 }, { 4,4,2 }, { 4,4,4 }, { 0,3,0 }, { 1,3,0 }, { 2,3,0 }, { 3,3,0 }, { 4,3,0 }, { 3,4,0 }, { 4,4,3 }, { 4,4,4 },
		{ 0,0,1 }, { 1,0,1 }, { 2,0,1 }, { 3,0,1 }, { 4,0,1 }, { 0,4,1 }, { 4,0,4 }, { 0,4,4 }, { 0,1,1 }, { 1,1,1 }, { 2,1,1 }, { 3,1,1 }, { 4,1,1 }, { 1,4,1 }, { 4,1,4 }, { 1,4,4 },
		{ 0,2,1 }, { 1,2,1 }, { 2,2,1 }, { 3,2,1 }, { 4,2,1 }, { 2,4,1 }, { 4,2,4 }, { 2,4,4 }, { 0,3,1 }, { 1,3,1 }, { 2,3,1 }, { 3,3,1 }, { 4,3,1 }, { 3,4,1 }, { 4,3,4 }, { 3,4,4 },
		{ 0,0,2 }, { 1,0,2 }, { 2,0,2 }, { 3,0,2 }, { 4,0,2 }, { 0,4,2 }, { 2,0,4 }, { 3,0,4 }, { 0,1,2 }, { 1,1,2 }, { 2,1,2 }, { 3,1,2 }, { 4,1,2 }, { 1,4,2 }, { 2,1,4 }, { 3,1,4 },
		{ 0,2,2 }, { 1,2,2 }, { 2,2,2 }, { 3,2,2 }, { 4,2,2 }, { 2,4,2 }, { 2,2,4 }, { 3,2,4 }, { 0,3,2 }, { 1,3,2 }, { 2,3,2 }, { 3,3,2 }, { 4,3,2 }, { 3,4,2 }, { 2,3,4 }, { 3,3,4 },
		{ 0,0,3 }, { 1,0,3 }, { 2,0,3 }, { 3,0,3 }, { 4,0,3 }, { 0,4,3 }, { 0,0,4 }, { 1,0,4 }, { 0,1,3 }, { 1,1,3 }, { 2,1,3 }, { 3,1,3 }, { 4,1,3 }, { 1,4,3 }, { 0,1,4 }, { 1,1,4 },
		{ 0,2,3 }, { 1,2,3 }, { 2,2,3 }, { 3,2,3 }, { 4,2,3 }, { 2,4,3 }, { 0,2,4 }, { 1,2,4 }, { 0,3,3 }, { 1,3,3 }, { 2,3,3 }, { 3,3,3 }, { 4,3,3 }, { 3,4,3 }, { 0,3,4 }, { 1,3,4 }
	};

	const deUint32 (& quints)[3] = quintsFromQ[Q];

	for (int i = 0; i < numValues; i++)
	{
		dst[i].m	= m[i];
		dst[i].tq	= quints[i];
		dst[i].v	= (quints[i] << numBits) + m[i];
	}
}

inline void decodeISEBitBlock (ISEDecodedResult* dst, BitAccessStream& data, int numBits)
{
	dst[0].m = data.getNext(numBits);
	dst[0].v = dst[0].m;
}

void decodeISE (ISEDecodedResult* dst, int numValues, BitAccessStream& data, const ISEParams& params)
{
	if (params.mode == ISEMODE_TRIT)
	{
		const int numBlocks = deDivRoundUp32(numValues, 5);
		for (int blockNdx = 0; blockNdx < numBlocks; blockNdx++)
		{
			const int numValuesInBlock = blockNdx == numBlocks-1 ? numValues - 5*(numBlocks-1) : 5;
			decodeISETritBlock(&dst[5*blockNdx], numValuesInBlock, data, params.numBits);
		}
	}
	else if (params.mode == ISEMODE_QUINT)
	{
		const int numBlocks = deDivRoundUp32(numValues, 3);
		for (int blockNdx = 0; blockNdx < numBlocks; blockNdx++)
		{
			const int numValuesInBlock = blockNdx == numBlocks-1 ? numValues - 3*(numBlocks-1) : 3;
			decodeISEQuintBlock(&dst[3*blockNdx], numValuesInBlock, data, params.numBits);
		}
	}
	else
	{
		DE_ASSERT(params.mode == ISEMODE_PLAIN_BIT);
		for (int i = 0; i < numValues; i++)
			decodeISEBitBlock(&dst[i], data, params.numBits);
	}
}

void unquantizeColorEndpoints (deUint32* dst, const ISEDecodedResult* iseResults, int numEndpoints, const ISEParams& iseParams)
{
	if (iseParams.mode == ISEMODE_TRIT || iseParams.mode == ISEMODE_QUINT)
	{
		const int rangeCase				= iseParams.numBits*2 - (iseParams.mode == ISEMODE_TRIT ? 2 : 1);
		DE_ASSERT(de::inRange(rangeCase, 0, 10));
		static const deUint32	Ca[11]	= { 204, 113, 93, 54, 44, 26, 22, 13, 11, 6, 5 };
		const deUint32			C		= Ca[rangeCase];

		for (int endpointNdx = 0; endpointNdx < numEndpoints; endpointNdx++)
		{
			const deUint32 a = getBit(iseResults[endpointNdx].m, 0);
			const deUint32 b = getBit(iseResults[endpointNdx].m, 1);
			const deUint32 c = getBit(iseResults[endpointNdx].m, 2);
			const deUint32 d = getBit(iseResults[endpointNdx].m, 3);
			const deUint32 e = getBit(iseResults[endpointNdx].m, 4);
			const deUint32 f = getBit(iseResults[endpointNdx].m, 5);

			const deUint32 A = a == 0 ? 0 : (1<<9)-1;
			const deUint32 B = rangeCase == 0	? 0
							 : rangeCase == 1	? 0
							 : rangeCase == 2	? (b << 8) |									(b << 4) |				(b << 2) |	(b << 1)
							 : rangeCase == 3	? (b << 8) |												(b << 3) |	(b << 2)
							 : rangeCase == 4	? (c << 8) | (b << 7) |										(c << 3) |	(b << 2) |	(c << 1) |	(b << 0)
							 : rangeCase == 5	? (c << 8) | (b << 7) |													(c << 2) |	(b << 1) |	(c << 0)
							 : rangeCase == 6	? (d << 8) | (c << 7) | (b << 6) |										(d << 2) |	(c << 1) |	(b << 0)
							 : rangeCase == 7	? (d << 8) | (c << 7) | (b << 6) |													(d << 1) |	(c << 0)
							 : rangeCase == 8	? (e << 8) | (d << 7) | (c << 6) | (b << 5) |										(e << 1) |	(d << 0)
							 : rangeCase == 9	? (e << 8) | (d << 7) | (c << 6) | (b << 5) |													(e << 0)
							 : rangeCase == 10	? (f << 8) | (e << 7) | (d << 6) | (c << 5) |	(b << 4) |										(f << 0)
							 : (deUint32)-1;
			DE_ASSERT(B != (deUint32)-1);

			dst[endpointNdx] = (((iseResults[endpointNdx].tq*C + B) ^ A) >> 2) | (A & 0x80);
		}
	}
	else
	{
		DE_ASSERT(iseParams.mode == ISEMODE_PLAIN_BIT);

		for (int endpointNdx = 0; endpointNdx < numEndpoints; endpointNdx++)
			dst[endpointNdx] = bitReplicationScale(iseResults[endpointNdx].v, iseParams.numBits, 8);
	}
}

inline void bitTransferSigned (deInt32& a, deInt32& b)
{
	b >>= 1;
	b |= a & 0x80;
	a >>= 1;
	a &= 0x3f;
	if (isBitSet(a, 5))
		a -= 0x40;
}

inline UVec4 clampedRGBA (const IVec4& rgba)
{
	return UVec4(de::clamp(rgba.x(), 0, 0xff),
				 de::clamp(rgba.y(), 0, 0xff),
				 de::clamp(rgba.z(), 0, 0xff),
				 de::clamp(rgba.w(), 0, 0xff));
}

inline IVec4 blueContract (int r, int g, int b, int a)
{
	return IVec4((r+b)>>1, (g+b)>>1, b, a);
}

inline bool isColorEndpointModeHDR (deUint32 mode)
{
	return mode == 2	||
		   mode == 3	||
		   mode == 7	||
		   mode == 11	||
		   mode == 14	||
		   mode == 15;
}

void decodeHDREndpointMode7 (UVec4& e0, UVec4& e1, deUint32 v0, deUint32 v1, deUint32 v2, deUint32 v3)
{
	const deUint32 m10		= getBit(v1, 7) | (getBit(v2, 7) << 1);
	const deUint32 m23		= getBits(v0, 6, 7);
	const deUint32 majComp	= m10 != 3	? m10
							: m23 != 3	? m23
							:			  0;
	const deUint32 mode		= m10 != 3	? m23
							: m23 != 3	? 4
							:			  5;

	deInt32			red		= (deInt32)getBits(v0, 0, 5);
	deInt32			green	= (deInt32)getBits(v1, 0, 4);
	deInt32			blue	= (deInt32)getBits(v2, 0, 4);
	deInt32			scale	= (deInt32)getBits(v3, 0, 4);

	{
#define SHOR(DST_VAR, SHIFT, BIT_VAR) (DST_VAR) |= (BIT_VAR) << (SHIFT)
#define ASSIGN_X_BITS(V0,S0, V1,S1, V2,S2, V3,S3, V4,S4, V5,S5, V6,S6) do { SHOR(V0,S0,x0); SHOR(V1,S1,x1); SHOR(V2,S2,x2); SHOR(V3,S3,x3); SHOR(V4,S4,x4); SHOR(V5,S5,x5); SHOR(V6,S6,x6); } while (false)

		const deUint32	x0	= getBit(v1, 6);
		const deUint32	x1	= getBit(v1, 5);
		const deUint32	x2	= getBit(v2, 6);
		const deUint32	x3	= getBit(v2, 5);
		const deUint32	x4	= getBit(v3, 7);
		const deUint32	x5	= getBit(v3, 6);
		const deUint32	x6	= getBit(v3, 5);

		deInt32&		R	= red;
		deInt32&		G	= green;
		deInt32&		B	= blue;
		deInt32&		S	= scale;

		switch (mode)
		{
			case 0: ASSIGN_X_BITS(R,9,  R,8,  R,7,  R,10,  R,6,  S,6,   S,5); break;
			case 1: ASSIGN_X_BITS(R,8,  G,5,  R,7,  B,5,   R,6,  R,10,  R,9); break;
			case 2: ASSIGN_X_BITS(R,9,  R,8,  R,7,  R,6,   S,7,  S,6,   S,5); break;
			case 3: ASSIGN_X_BITS(R,8,  G,5,  R,7,  B,5,   R,6,  S,6,   S,5); break;
			case 4: ASSIGN_X_BITS(G,6,  G,5,  B,6,  B,5,   R,6,  R,7,   S,5); break;
			case 5: ASSIGN_X_BITS(G,6,  G,5,  B,6,  B,5,   R,6,  S,6,   S,5); break;
			default:
				DE_ASSERT(false);
		}

#undef ASSIGN_X_BITS
#undef SHOR
	}

	static const int shiftAmounts[] = { 1, 1, 2, 3, 4, 5 };
	DE_ASSERT(mode < DE_LENGTH_OF_ARRAY(shiftAmounts));

	red		<<= shiftAmounts[mode];
	green	<<= shiftAmounts[mode];
	blue	<<= shiftAmounts[mode];
	scale	<<= shiftAmounts[mode];

	if (mode != 5)
	{
		green	= red - green;
		blue	= red - blue;
	}

	if (majComp == 1)
		std::swap(red, green);
	else if (majComp == 2)
		std::swap(red, blue);

	e0 = UVec4(de::clamp(red	- scale,	0, 0xfff),
			   de::clamp(green	- scale,	0, 0xfff),
			   de::clamp(blue	- scale,	0, 0xfff),
			   0x780);

	e1 = UVec4(de::clamp(red,				0, 0xfff),
			   de::clamp(green,				0, 0xfff),
			   de::clamp(blue,				0, 0xfff),
			   0x780);
}

void decodeHDREndpointMode11 (UVec4& e0, UVec4& e1, deUint32 v0, deUint32 v1, deUint32 v2, deUint32 v3, deUint32 v4, deUint32 v5)
{
	const deUint32 major = (getBit(v5, 7) << 1) | getBit(v4, 7);

	if (major == 3)
	{
		e0 = UVec4(v0<<4, v2<<4, getBits(v4,0,6)<<5, 0x780);
		e1 = UVec4(v1<<4, v3<<4, getBits(v5,0,6)<<5, 0x780);
	}
	else
	{
		const deUint32 mode = (getBit(v3, 7) << 2) | (getBit(v2, 7) << 1) | getBit(v1, 7);

		deInt32 a	= (deInt32)((getBit(v1, 6) << 8) | v0);
		deInt32 c	= (deInt32)(getBits(v1, 0, 5));
		deInt32 b0	= (deInt32)(getBits(v2, 0, 5));
		deInt32 b1	= (deInt32)(getBits(v3, 0, 5));
		deInt32 d0	= (deInt32)(getBits(v4, 0, 4));
		deInt32 d1	= (deInt32)(getBits(v5, 0, 4));

		{
#define SHOR(DST_VAR, SHIFT, BIT_VAR) (DST_VAR) |= (BIT_VAR) << (SHIFT)
#define ASSIGN_X_BITS(V0,S0, V1,S1, V2,S2, V3,S3, V4,S4, V5,S5) do { SHOR(V0,S0,x0); SHOR(V1,S1,x1); SHOR(V2,S2,x2); SHOR(V3,S3,x3); SHOR(V4,S4,x4); SHOR(V5,S5,x5); } while (false)

			const deUint32 x0 = getBit(v2, 6);
			const deUint32 x1 = getBit(v3, 6);
			const deUint32 x2 = getBit(v4, 6);
			const deUint32 x3 = getBit(v5, 6);
			const deUint32 x4 = getBit(v4, 5);
			const deUint32 x5 = getBit(v5, 5);

			switch (mode)
			{
				case 0: ASSIGN_X_BITS(b0,6,  b1,6,   d0,6,  d1,6,  d0,5,  d1,5); break;
				case 1: ASSIGN_X_BITS(b0,6,  b1,6,   b0,7,  b1,7,  d0,5,  d1,5); break;
				case 2: ASSIGN_X_BITS(a,9,   c,6,    d0,6,  d1,6,  d0,5,  d1,5); break;
				case 3: ASSIGN_X_BITS(b0,6,  b1,6,   a,9,   c,6,   d0,5,  d1,5); break;
				case 4: ASSIGN_X_BITS(b0,6,  b1,6,   b0,7,  b1,7,  a,9,   a,10); break;
				case 5: ASSIGN_X_BITS(a,9,   a,10,   c,7,   c,6,   d0,5,  d1,5); break;
				case 6: ASSIGN_X_BITS(b0,6,  b1,6,   a,11,  c,6,   a,9,   a,10); break;
				case 7: ASSIGN_X_BITS(a,9,   a,10,   a,11,  c,6,   d0,5,  d1,5); break;
				default:
					DE_ASSERT(false);
			}

#undef ASSIGN_X_BITS
#undef SHOR
		}

		static const int numDBits[] = { 7, 6, 7, 6, 5, 6, 5, 6 };
		DE_ASSERT(mode < DE_LENGTH_OF_ARRAY(numDBits));

		d0 = signExtend(d0, numDBits[mode]);
		d1 = signExtend(d1, numDBits[mode]);

		const int shiftAmount = (mode >> 1) ^ 3;
		a	<<= shiftAmount;
		c	<<= shiftAmount;
		b0	<<= shiftAmount;
		b1	<<= shiftAmount;
		d0	<<= shiftAmount;
		d1	<<= shiftAmount;

		e0 = UVec4(de::clamp(a-c,			0, 0xfff),
				   de::clamp(a-b0-c-d0,		0, 0xfff),
				   de::clamp(a-b1-c-d1,		0, 0xfff),
				   0x780);

		e1 = UVec4(de::clamp(a,				0, 0xfff),
				   de::clamp(a-b0,			0, 0xfff),
				   de::clamp(a-b1,			0, 0xfff),
				   0x780);

		if (major == 1)
		{
			std::swap(e0.x(), e0.y());
			std::swap(e1.x(), e1.y());
		}
		else if (major == 2)
		{
			std::swap(e0.x(), e0.z());
			std::swap(e1.x(), e1.z());
		}
	}
}

void decodeHDREndpointMode15(UVec4& e0, UVec4& e1, deUint32 v0, deUint32 v1, deUint32 v2, deUint32 v3, deUint32 v4, deUint32 v5, deUint32 v6In, deUint32 v7In)
{
	decodeHDREndpointMode11(e0, e1, v0, v1, v2, v3, v4, v5);

	const deUint32	mode	= (getBit(v7In, 7) << 1) | getBit(v6In, 7);
	deInt32			v6		= (deInt32)getBits(v6In, 0, 6);
	deInt32			v7		= (deInt32)getBits(v7In, 0, 6);

	if (mode == 3)
	{
		e0.w() = v6 << 5;
		e1.w() = v7 << 5;
	}
	else
	{
		v6 |= (v7 << (mode+1)) & 0x780;
		v7 &= (0x3f >> mode);
		v7 ^= 0x20 >> mode;
		v7 -= 0x20 >> mode;
		v6 <<= 4-mode;
		v7 <<= 4-mode;

		v7 += v6;
		v7 = de::clamp(v7, 0, 0xfff);
		e0.w() = v6;
		e1.w() = v7;
	}
}

void decodeColorEndpoints (ColorEndpointPair* dst, const deUint32* unquantizedEndpoints, const deUint32* endpointModes, int numPartitions)
{
	int unquantizedNdx = 0;

	for (int partitionNdx = 0; partitionNdx < numPartitions; partitionNdx++)
	{
		const deUint32		endpointMode	= endpointModes[partitionNdx];
		const deUint32*		v				= &unquantizedEndpoints[unquantizedNdx];
		UVec4&				e0				= dst[partitionNdx].e0;
		UVec4&				e1				= dst[partitionNdx].e1;

		unquantizedNdx += computeNumColorEndpointValues(endpointMode);

		switch (endpointMode)
		{
			case 0:
				e0 = UVec4(v[0], v[0], v[0], 0xff);
				e1 = UVec4(v[1], v[1], v[1], 0xff);
				break;

			case 1:
			{
				const deUint32 L0 = (v[0] >> 2) | (getBits(v[1], 6, 7) << 6);
				const deUint32 L1 = de::min(0xffu, L0 + getBits(v[1], 0, 5));
				e0 = UVec4(L0, L0, L0, 0xff);
				e1 = UVec4(L1, L1, L1, 0xff);
				break;
			}

			case 2:
			{
				const deUint32 v1Gr		= v[1] >= v[0];
				const deUint32 y0		= v1Gr ? v[0]<<4 : (v[1]<<4) + 8;
				const deUint32 y1		= v1Gr ? v[1]<<4 : (v[0]<<4) - 8;

				e0 = UVec4(y0, y0, y0, 0x780);
				e1 = UVec4(y1, y1, y1, 0x780);
				break;
			}

			case 3:
			{
				const bool		m	= isBitSet(v[0], 7);
				const deUint32	y0	= m ? (getBits(v[1], 5, 7) << 9) | (getBits(v[0], 0, 6) << 2)
										: (getBits(v[1], 4, 7) << 8) | (getBits(v[0], 0, 6) << 1);
				const deUint32	d	= m ? getBits(v[1], 0, 4) << 2
										: getBits(v[1], 0, 3) << 1;
				const deUint32	y1	= de::min(0xfffu, y0+d);

				e0 = UVec4(y0, y0, y0, 0x780);
				e1 = UVec4(y1, y1, y1, 0x780);
				break;
			}

			case 4:
				e0 = UVec4(v[0], v[0], v[0], v[2]);
				e1 = UVec4(v[1], v[1], v[1], v[3]);
				break;

			case 5:
			{
				deInt32 v0 = (deInt32)v[0];
				deInt32 v1 = (deInt32)v[1];
				deInt32 v2 = (deInt32)v[2];
				deInt32 v3 = (deInt32)v[3];
				bitTransferSigned(v1, v0);
				bitTransferSigned(v3, v2);

				e0 = clampedRGBA(IVec4(v0,		v0,		v0,		v2));
				e1 = clampedRGBA(IVec4(v0+v1,	v0+v1,	v0+v1,	v2+v3));
				break;
			}

			case 6:
				e0 = UVec4((v[0]*v[3]) >> 8,	(v[1]*v[3]) >> 8,	(v[2]*v[3]) >> 8,	0xff);
				e1 = UVec4(v[0],				v[1],				v[2],				0xff);
				break;

			case 7:
				decodeHDREndpointMode7(e0, e1, v[0], v[1], v[2], v[3]);
				break;

			case 8:
				if (v[1]+v[3]+v[5] >= v[0]+v[2]+v[4])
				{
					e0 = UVec4(v[0], v[2], v[4], 0xff);
					e1 = UVec4(v[1], v[3], v[5], 0xff);
				}
				else
				{
					e0 = blueContract(v[1], v[3], v[5], 0xff).asUint();
					e1 = blueContract(v[0], v[2], v[4], 0xff).asUint();
				}
				break;

			case 9:
			{
				deInt32 v0 = (deInt32)v[0];
				deInt32 v1 = (deInt32)v[1];
				deInt32 v2 = (deInt32)v[2];
				deInt32 v3 = (deInt32)v[3];
				deInt32 v4 = (deInt32)v[4];
				deInt32 v5 = (deInt32)v[5];
				bitTransferSigned(v1, v0);
				bitTransferSigned(v3, v2);
				bitTransferSigned(v5, v4);

				if (v1+v3+v5 >= 0)
				{
					e0 = clampedRGBA(IVec4(v0,		v2,		v4,		0xff));
					e1 = clampedRGBA(IVec4(v0+v1,	v2+v3,	v4+v5,	0xff));
				}
				else
				{
					e0 = clampedRGBA(blueContract(v0+v1,	v2+v3,	v4+v5,	0xff));
					e1 = clampedRGBA(blueContract(v0,		v2,		v4,		0xff));
				}
				break;
			}

			case 10:
				e0 = UVec4((v[0]*v[3]) >> 8,	(v[1]*v[3]) >> 8,	(v[2]*v[3]) >> 8,	v[4]);
				e1 = UVec4(v[0],				v[1],				v[2],				v[5]);
				break;

			case 11:
				decodeHDREndpointMode11(e0, e1, v[0], v[1], v[2], v[3], v[4], v[5]);
				break;

			case 12:
				if (v[1]+v[3]+v[5] >= v[0]+v[2]+v[4])
				{
					e0 = UVec4(v[0], v[2], v[4], v[6]);
					e1 = UVec4(v[1], v[3], v[5], v[7]);
				}
				else
				{
					e0 = clampedRGBA(blueContract(v[1], v[3], v[5], v[7]));
					e1 = clampedRGBA(blueContract(v[0], v[2], v[4], v[6]));
				}
				break;

			case 13:
			{
				deInt32 v0 = (deInt32)v[0];
				deInt32 v1 = (deInt32)v[1];
				deInt32 v2 = (deInt32)v[2];
				deInt32 v3 = (deInt32)v[3];
				deInt32 v4 = (deInt32)v[4];
				deInt32 v5 = (deInt32)v[5];
				deInt32 v6 = (deInt32)v[6];
				deInt32 v7 = (deInt32)v[7];
				bitTransferSigned(v1, v0);
				bitTransferSigned(v3, v2);
				bitTransferSigned(v5, v4);
				bitTransferSigned(v7, v6);

				if (v1+v3+v5 >= 0)
				{
					e0 = clampedRGBA(IVec4(v0,		v2,		v4,		v6));
					e1 = clampedRGBA(IVec4(v0+v1,	v2+v3,	v4+v5,	v6+v7));
				}
				else
				{
					e0 = clampedRGBA(blueContract(v0+v1,	v2+v3,	v4+v5,	v6+v7));
					e1 = clampedRGBA(blueContract(v0,		v2,		v4,		v6));
				}

				break;
			}

			case 14:
				decodeHDREndpointMode11(e0, e1, v[0], v[1], v[2], v[3], v[4], v[5]);
				e0.w() = v[6];
				e1.w() = v[7];
				break;

			case 15:
				decodeHDREndpointMode15(e0, e1, v[0], v[1], v[2], v[3], v[4], v[5], v[6], v[7]);
				break;

			default:
				DE_ASSERT(false);
		}
	}
}

void computeColorEndpoints (ColorEndpointPair* dst, const Block128& blockData, const deUint32* endpointModes, int numPartitions, int numColorEndpointValues, const ISEParams& iseParams, int numBitsAvailable)
{
	const int			colorEndpointDataStart = numPartitions == 1 ? 17 : 29;
	ISEDecodedResult	colorEndpointData[18];

	{
		BitAccessStream dataStream(blockData, colorEndpointDataStart, numBitsAvailable, true);
		decodeISE(&colorEndpointData[0], numColorEndpointValues, dataStream, iseParams);
	}

	{
		deUint32 unquantizedEndpoints[18];
		unquantizeColorEndpoints(&unquantizedEndpoints[0], &colorEndpointData[0], numColorEndpointValues, iseParams);
		decodeColorEndpoints(dst, &unquantizedEndpoints[0], &endpointModes[0], numPartitions);
	}
}

void unquantizeWeights (deUint32 dst[64], const ISEDecodedResult* weightGrid, const ASTCBlockMode& blockMode)
{
	const int			numWeights	= computeNumWeights(blockMode);
	const ISEParams&	iseParams	= blockMode.weightISEParams;

	if (iseParams.mode == ISEMODE_TRIT || iseParams.mode == ISEMODE_QUINT)
	{
		const int rangeCase = iseParams.numBits*2 + (iseParams.mode == ISEMODE_QUINT ? 1 : 0);

		if (rangeCase == 0 || rangeCase == 1)
		{
			static const deUint32 map0[3]	= { 0, 32, 63 };
			static const deUint32 map1[5]	= { 0, 16, 32, 47, 63 };
			const deUint32* const map		= rangeCase == 0 ? &map0[0] : &map1[0];
			for (int i = 0; i < numWeights; i++)
			{
				DE_ASSERT(weightGrid[i].v < (rangeCase == 0 ? 3u : 5u));
				dst[i] = map[weightGrid[i].v];
			}
		}
		else
		{
			DE_ASSERT(rangeCase <= 6);
			static const deUint32	Ca[5]	= { 50, 28, 23, 13, 11 };
			const deUint32			C		= Ca[rangeCase-2];

			for (int weightNdx = 0; weightNdx < numWeights; weightNdx++)
			{
				const deUint32 a = getBit(weightGrid[weightNdx].m, 0);
				const deUint32 b = getBit(weightGrid[weightNdx].m, 1);
				const deUint32 c = getBit(weightGrid[weightNdx].m, 2);

				const deUint32 A = a == 0 ? 0 : (1<<7)-1;
				const deUint32 B = rangeCase == 2 ? 0
								 : rangeCase == 3 ? 0
								 : rangeCase == 4 ? (b << 6) |					(b << 2) |				(b << 0)
								 : rangeCase == 5 ? (b << 6) |								(b << 1)
								 : rangeCase == 6 ? (c << 6) | (b << 5) |					(c << 1) |	(b << 0)
								 : (deUint32)-1;

				dst[weightNdx] = (((weightGrid[weightNdx].tq*C + B) ^ A) >> 2) | (A & 0x20);
			}
		}
	}
	else
	{
		DE_ASSERT(iseParams.mode == ISEMODE_PLAIN_BIT);

		for (int weightNdx = 0; weightNdx < numWeights; weightNdx++)
			dst[weightNdx] = bitReplicationScale(weightGrid[weightNdx].v, iseParams.numBits, 6);
	}

	for (int weightNdx = 0; weightNdx < numWeights; weightNdx++)
		dst[weightNdx] += dst[weightNdx] > 32 ? 1 : 0;

	// Initialize nonexistent weights to poison values
	for (int weightNdx = numWeights; weightNdx < 64; weightNdx++)
		dst[weightNdx] = ~0u;

}

void interpolateWeights (TexelWeightPair* dst, const deUint32 (&unquantizedWeights) [64], int blockWidth, int blockHeight, const ASTCBlockMode& blockMode)
{
	const int		numWeightsPerTexel	= blockMode.isDualPlane ? 2 : 1;
	const deUint32	scaleX				= (1024 + blockWidth/2) / (blockWidth-1);
	const deUint32	scaleY				= (1024 + blockHeight/2) / (blockHeight-1);

	DE_ASSERT(blockMode.weightGridWidth*blockMode.weightGridHeight*numWeightsPerTexel <= DE_LENGTH_OF_ARRAY(unquantizedWeights));

	for (int texelY = 0; texelY < blockHeight; texelY++)
	{
		for (int texelX = 0; texelX < blockWidth; texelX++)
		{
			const deUint32 gX	= (scaleX*texelX*(blockMode.weightGridWidth-1) + 32) >> 6;
			const deUint32 gY	= (scaleY*texelY*(blockMode.weightGridHeight-1) + 32) >> 6;
			const deUint32 jX	= gX >> 4;
			const deUint32 jY	= gY >> 4;
			const deUint32 fX	= gX & 0xf;
			const deUint32 fY	= gY & 0xf;

			const deUint32 w11	= (fX*fY + 8) >> 4;
			const deUint32 w10	= fY - w11;
			const deUint32 w01	= fX - w11;
			const deUint32 w00	= 16 - fX - fY + w11;

			const deUint32 i00	= jY*blockMode.weightGridWidth + jX;
			const deUint32 i01	= i00 + 1;
			const deUint32 i10	= i00 + blockMode.weightGridWidth;
			const deUint32 i11	= i00 + blockMode.weightGridWidth + 1;

			// These addresses can be out of bounds, but respective weights will be 0 then.
			DE_ASSERT(deInBounds32(i00, 0, blockMode.weightGridWidth*blockMode.weightGridHeight) || w00 == 0);
			DE_ASSERT(deInBounds32(i01, 0, blockMode.weightGridWidth*blockMode.weightGridHeight) || w01 == 0);
			DE_ASSERT(deInBounds32(i10, 0, blockMode.weightGridWidth*blockMode.weightGridHeight) || w10 == 0);
			DE_ASSERT(deInBounds32(i11, 0, blockMode.weightGridWidth*blockMode.weightGridHeight) || w11 == 0);

			for (int texelWeightNdx = 0; texelWeightNdx < numWeightsPerTexel; texelWeightNdx++)
			{
				// & 0x3f clamps address to bounds of unquantizedWeights
				const deUint32 p00	= unquantizedWeights[(i00 * numWeightsPerTexel + texelWeightNdx) & 0x3f];
				const deUint32 p01	= unquantizedWeights[(i01 * numWeightsPerTexel + texelWeightNdx) & 0x3f];
				const deUint32 p10	= unquantizedWeights[(i10 * numWeightsPerTexel + texelWeightNdx) & 0x3f];
				const deUint32 p11	= unquantizedWeights[(i11 * numWeightsPerTexel + texelWeightNdx) & 0x3f];

				dst[texelY*blockWidth + texelX].w[texelWeightNdx] = (p00*w00 + p01*w01 + p10*w10 + p11*w11 + 8) >> 4;
			}
		}
	}
}

void computeTexelWeights (TexelWeightPair* dst, const Block128& blockData, int blockWidth, int blockHeight, const ASTCBlockMode& blockMode)
{
	ISEDecodedResult weightGrid[64];

	{
		BitAccessStream dataStream(blockData, 127, computeNumRequiredBits(blockMode.weightISEParams, computeNumWeights(blockMode)), false);
		decodeISE(&weightGrid[0], computeNumWeights(blockMode), dataStream, blockMode.weightISEParams);
	}

	{
		deUint32 unquantizedWeights[64];
		unquantizeWeights(&unquantizedWeights[0], &weightGrid[0], blockMode);
		interpolateWeights(dst, unquantizedWeights, blockWidth, blockHeight, blockMode);
	}
}

inline deUint32 hash52 (deUint32 v)
{
	deUint32 p = v;
	p ^= p >> 15;	p -= p << 17;	p += p << 7;	p += p << 4;
	p ^= p >>  5;	p += p << 16;	p ^= p >> 7;	p ^= p >> 3;
	p ^= p <<  6;	p ^= p >> 17;
	return p;
}

int computeTexelPartition (deUint32 seedIn, deUint32 xIn, deUint32 yIn, deUint32 zIn, int numPartitions, bool smallBlock)
{
	DE_ASSERT(zIn == 0);
	const deUint32	x		= smallBlock ? xIn << 1 : xIn;
	const deUint32	y		= smallBlock ? yIn << 1 : yIn;
	const deUint32	z		= smallBlock ? zIn << 1 : zIn;
	const deUint32	seed	= seedIn + 1024*(numPartitions-1);
	const deUint32	rnum	= hash52(seed);
	deUint8			seed1	= (deUint8)( rnum							& 0xf);
	deUint8			seed2	= (deUint8)((rnum >>  4)					& 0xf);
	deUint8			seed3	= (deUint8)((rnum >>  8)					& 0xf);
	deUint8			seed4	= (deUint8)((rnum >> 12)					& 0xf);
	deUint8			seed5	= (deUint8)((rnum >> 16)					& 0xf);
	deUint8			seed6	= (deUint8)((rnum >> 20)					& 0xf);
	deUint8			seed7	= (deUint8)((rnum >> 24)					& 0xf);
	deUint8			seed8	= (deUint8)((rnum >> 28)					& 0xf);
	deUint8			seed9	= (deUint8)((rnum >> 18)					& 0xf);
	deUint8			seed10	= (deUint8)((rnum >> 22)					& 0xf);
	deUint8			seed11	= (deUint8)((rnum >> 26)					& 0xf);
	deUint8			seed12	= (deUint8)(((rnum >> 30) | (rnum << 2))	& 0xf);

	seed1  = (deUint8)(seed1  * seed1 );
	seed2  = (deUint8)(seed2  * seed2 );
	seed3  = (deUint8)(seed3  * seed3 );
	seed4  = (deUint8)(seed4  * seed4 );
	seed5  = (deUint8)(seed5  * seed5 );
	seed6  = (deUint8)(seed6  * seed6 );
	seed7  = (deUint8)(seed7  * seed7 );
	seed8  = (deUint8)(seed8  * seed8 );
	seed9  = (deUint8)(seed9  * seed9 );
	seed10 = (deUint8)(seed10 * seed10);
	seed11 = (deUint8)(seed11 * seed11);
	seed12 = (deUint8)(seed12 * seed12);

	const int shA = (seed & 2) != 0		? 4		: 5;
	const int shB = numPartitions == 3	? 6		: 5;
	const int sh1 = (seed & 1) != 0		? shA	: shB;
	const int sh2 = (seed & 1) != 0		? shB	: shA;
	const int sh3 = (seed & 0x10) != 0	? sh1	: sh2;

	seed1  = (deUint8)(seed1  >> sh1);
	seed2  = (deUint8)(seed2  >> sh2);
	seed3  = (deUint8)(seed3  >> sh1);
	seed4  = (deUint8)(seed4  >> sh2);
	seed5  = (deUint8)(seed5  >> sh1);
	seed6  = (deUint8)(seed6  >> sh2);
	seed7  = (deUint8)(seed7  >> sh1);
	seed8  = (deUint8)(seed8  >> sh2);
	seed9  = (deUint8)(seed9  >> sh3);
	seed10 = (deUint8)(seed10 >> sh3);
	seed11 = (deUint8)(seed11 >> sh3);
	seed12 = (deUint8)(seed12 >> sh3);

	const int a =						0x3f & (seed1*x + seed2*y + seed11*z + (rnum >> 14));
	const int b =						0x3f & (seed3*x + seed4*y + seed12*z + (rnum >> 10));
	const int c = numPartitions >= 3 ?	0x3f & (seed5*x + seed6*y + seed9*z  + (rnum >>  6))	: 0;
	const int d = numPartitions >= 4 ?	0x3f & (seed7*x + seed8*y + seed10*z + (rnum >>  2))	: 0;

	return a >= b && a >= c && a >= d	? 0
		 : b >= c && b >= d				? 1
		 : c >= d						? 2
		 :								  3;
}

DecompressResult setTexelColors (void* dst, ColorEndpointPair* colorEndpoints, TexelWeightPair* texelWeights, int ccs, deUint32 partitionIndexSeed,
								 int numPartitions, int blockWidth, int blockHeight, bool isSRGB, bool isLDRMode, const deUint32* colorEndpointModes)
{
	const bool			smallBlock	= blockWidth*blockHeight < 31;
	DecompressResult	result		= DECOMPRESS_RESULT_VALID_BLOCK;
	bool				isHDREndpoint[4];

	for (int i = 0; i < numPartitions; i++)
		isHDREndpoint[i] = isColorEndpointModeHDR(colorEndpointModes[i]);

	for (int texelY = 0; texelY < blockHeight; texelY++)
	for (int texelX = 0; texelX < blockWidth; texelX++)
	{
		const int				texelNdx			= texelY*blockWidth + texelX;
		const int				colorEndpointNdx	= numPartitions == 1 ? 0 : computeTexelPartition(partitionIndexSeed, texelX, texelY, 0, numPartitions, smallBlock);
		DE_ASSERT(colorEndpointNdx < numPartitions);
		const UVec4&			e0					= colorEndpoints[colorEndpointNdx].e0;
		const UVec4&			e1					= colorEndpoints[colorEndpointNdx].e1;
		const TexelWeightPair&	weight				= texelWeights[texelNdx];

		if (isLDRMode && isHDREndpoint[colorEndpointNdx])
		{
			if (isSRGB)
			{
				((deUint8*)dst)[texelNdx*4 + 0] = 0xff;
				((deUint8*)dst)[texelNdx*4 + 1] = 0;
				((deUint8*)dst)[texelNdx*4 + 2] = 0xff;
				((deUint8*)dst)[texelNdx*4 + 3] = 0xff;
			}
			else
			{
				((float*)dst)[texelNdx*4 + 0] = 1.0f;
				((float*)dst)[texelNdx*4 + 1] = 0;
				((float*)dst)[texelNdx*4 + 2] = 1.0f;
				((float*)dst)[texelNdx*4 + 3] = 1.0f;
			}

			result = DECOMPRESS_RESULT_ERROR;
		}
		else
		{
			for (int channelNdx = 0; channelNdx < 4; channelNdx++)
			{
				if (!isHDREndpoint[colorEndpointNdx] || (channelNdx == 3 && colorEndpointModes[colorEndpointNdx] == 14)) // \note Alpha for mode 14 is treated the same as LDR.
				{
					const deUint32 c0	= (e0[channelNdx] << 8) | (isSRGB ? 0x80 : e0[channelNdx]);
					const deUint32 c1	= (e1[channelNdx] << 8) | (isSRGB ? 0x80 : e1[channelNdx]);
					const deUint32 w	= weight.w[ccs == channelNdx ? 1 : 0];
					const deUint32 c	= (c0*(64-w) + c1*w + 32) / 64;

					if (isSRGB)
						((deUint8*)dst)[texelNdx*4 + channelNdx] = (deUint8)((c & 0xff00) >> 8);
					else
						((float*)dst)[texelNdx*4 + channelNdx] = c == 65535 ? 1.0f : (float)c / 65536.0f;
				}
				else
				{
					DE_STATIC_ASSERT((de::meta::TypesSame<deFloat16, deUint16>::Value));
					const deUint32		c0	= e0[channelNdx] << 4;
					const deUint32		c1	= e1[channelNdx] << 4;
					const deUint32		w	= weight.w[ccs == channelNdx ? 1 : 0];
					const deUint32		c	= (c0*(64-w) + c1*w + 32) / 64;
					const deUint32		e	= getBits(c, 11, 15);
					const deUint32		m	= getBits(c, 0, 10);
					const deUint32		mt	= m < 512		? 3*m
											: m >= 1536		? 5*m - 2048
											:				  4*m - 512;
					const deFloat16		cf	= (deFloat16)((e << 10) + (mt >> 3));

					((float*)dst)[texelNdx*4 + channelNdx] = deFloat16To32(isFloat16InfOrNan(cf) ? 0x7bff : cf);
				}
			}
		}
	}

	return result;
}

DecompressResult decompressBlock (void* dst, const Block128& blockData, int blockWidth, int blockHeight, bool isSRGB, bool isLDR)
{
	DE_ASSERT(isLDR || !isSRGB);

	// Decode block mode.

	const ASTCBlockMode blockMode = getASTCBlockMode(blockData.getBits(0, 10));

	// Check for block mode errors.

	if (blockMode.isError)
	{
		setASTCErrorColorBlock(dst, blockWidth, blockHeight, isSRGB);
		return DECOMPRESS_RESULT_ERROR;
	}

	// Separate path for void-extent.

	if (blockMode.isVoidExtent)
		return decodeVoidExtentBlock(dst, blockData, blockWidth, blockHeight, isSRGB, isLDR);

	// Compute weight grid values.

	const int numWeights			= computeNumWeights(blockMode);
	const int numWeightDataBits		= computeNumRequiredBits(blockMode.weightISEParams, numWeights);
	const int numPartitions			= (int)blockData.getBits(11, 12) + 1;

	// Check for errors in weight grid, partition and dual-plane parameters.

	if (numWeights > 64								||
		numWeightDataBits > 96						||
		numWeightDataBits < 24						||
		blockMode.weightGridWidth > blockWidth		||
		blockMode.weightGridHeight > blockHeight	||
		(numPartitions == 4 && blockMode.isDualPlane))
	{
		setASTCErrorColorBlock(dst, blockWidth, blockHeight, isSRGB);
		return DECOMPRESS_RESULT_ERROR;
	}

	// Compute number of bits available for color endpoint data.

	const bool	isSingleUniqueCem			= numPartitions == 1 || blockData.getBits(23, 24) == 0;
	const int	numConfigDataBits			= (numPartitions == 1 ? 17 : isSingleUniqueCem ? 29 : 25 + 3*numPartitions) +
											  (blockMode.isDualPlane ? 2 : 0);
	const int	numBitsForColorEndpoints	= 128 - numWeightDataBits - numConfigDataBits;
	const int	extraCemBitsStart			= 127 - numWeightDataBits - (isSingleUniqueCem		? -1
																		: numPartitions == 4	? 7
																		: numPartitions == 3	? 4
																		: numPartitions == 2	? 1
																		: 0);
	// Decode color endpoint modes.

	deUint32 colorEndpointModes[4];
	decodeColorEndpointModes(&colorEndpointModes[0], blockData, numPartitions, extraCemBitsStart);

	const int numColorEndpointValues = computeNumColorEndpointValues(colorEndpointModes, numPartitions);

	// Check for errors in color endpoint value count.

	if (numColorEndpointValues > 18 || numBitsForColorEndpoints < deDivRoundUp32(13*numColorEndpointValues, 5))
	{
		setASTCErrorColorBlock(dst, blockWidth, blockHeight, isSRGB);
		return DECOMPRESS_RESULT_ERROR;
	}

	// Compute color endpoints.

	ColorEndpointPair colorEndpoints[4];
	computeColorEndpoints(&colorEndpoints[0], blockData, &colorEndpointModes[0], numPartitions, numColorEndpointValues,
						  computeMaximumRangeISEParams(numBitsForColorEndpoints, numColorEndpointValues), numBitsForColorEndpoints);

	// Compute texel weights.

	TexelWeightPair texelWeights[MAX_BLOCK_WIDTH*MAX_BLOCK_HEIGHT];
	computeTexelWeights(&texelWeights[0], blockData, blockWidth, blockHeight, blockMode);

	// Set texel colors.

	const int		ccs						= blockMode.isDualPlane ? (int)blockData.getBits(extraCemBitsStart-2, extraCemBitsStart-1) : -1;
	const deUint32	partitionIndexSeed		= numPartitions > 1 ? blockData.getBits(13, 22) : (deUint32)-1;

	return setTexelColors(dst, &colorEndpoints[0], &texelWeights[0], ccs, partitionIndexSeed, numPartitions, blockWidth, blockHeight, isSRGB, isLDR, &colorEndpointModes[0]);
}

void decompress (const PixelBufferAccess& dst, const deUint8* data, bool isSRGB, bool isLDR)
{
	DE_ASSERT(isLDR || !isSRGB);

	const int blockWidth = dst.getWidth();
	const int blockHeight = dst.getHeight();

	union
	{
		deUint8		sRGB[MAX_BLOCK_WIDTH*MAX_BLOCK_HEIGHT*4];
		float		linear[MAX_BLOCK_WIDTH*MAX_BLOCK_HEIGHT*4];
	} decompressedBuffer;

	const Block128 blockData(data);
	decompressBlock(isSRGB ? (void*)&decompressedBuffer.sRGB[0] : (void*)&decompressedBuffer.linear[0],
					blockData, dst.getWidth(), dst.getHeight(), isSRGB, isLDR);

	if (isSRGB)
	{
		for (int i = 0; i < blockHeight; i++)
		for (int j = 0; j < blockWidth; j++)
		{
			dst.setPixel(IVec4(decompressedBuffer.sRGB[(i*blockWidth + j) * 4 + 0],
							   decompressedBuffer.sRGB[(i*blockWidth + j) * 4 + 1],
							   decompressedBuffer.sRGB[(i*blockWidth + j) * 4 + 2],
							   decompressedBuffer.sRGB[(i*blockWidth + j) * 4 + 3]), j, i);
		}
	}
	else
	{
		for (int i = 0; i < blockHeight; i++)
		for (int j = 0; j < blockWidth; j++)
		{
			dst.setPixel(Vec4(decompressedBuffer.linear[(i*blockWidth + j) * 4 + 0],
							  decompressedBuffer.linear[(i*blockWidth + j) * 4 + 1],
							  decompressedBuffer.linear[(i*blockWidth + j) * 4 + 2],
							  decompressedBuffer.linear[(i*blockWidth + j) * 4 + 3]), j, i);
		}
	}
}

// Helper class for setting bits in a 128-bit block.
class AssignBlock128
{
private:
	typedef deUint64 Word;

	enum
	{
		WORD_BYTES	= sizeof(Word),
		WORD_BITS	= 8*WORD_BYTES,
		NUM_WORDS	= 128 / WORD_BITS
	};

	DE_STATIC_ASSERT(128 % WORD_BITS == 0);

public:
	AssignBlock128 (void)
	{
		for (int wordNdx = 0; wordNdx < NUM_WORDS; wordNdx++)
			m_words[wordNdx] = 0;
	}

	void setBit (int ndx, deUint32 val)
	{
		DE_ASSERT(de::inBounds(ndx, 0, 128));
		DE_ASSERT((val & 1) == val);
		const int wordNdx	= ndx / WORD_BITS;
		const int bitNdx	= ndx % WORD_BITS;
		m_words[wordNdx] = (m_words[wordNdx] & ~((Word)1 << bitNdx)) | ((Word)val << bitNdx);
	}

	void setBits (int low, int high, deUint32 bits)
	{
		DE_ASSERT(de::inBounds(low, 0, 128));
		DE_ASSERT(de::inBounds(high, 0, 128));
		DE_ASSERT(de::inRange(high-low+1, 0, 32));
		DE_ASSERT((bits & (((Word)1 << (high-low+1)) - 1)) == bits);

		if (high-low+1 == 0)
			return;

		const int word0Ndx		= low / WORD_BITS;
		const int word1Ndx		= high / WORD_BITS;
		const int lowNdxInW0	= low % WORD_BITS;

		if (word0Ndx == word1Ndx)
			m_words[word0Ndx] = (m_words[word0Ndx] & ~((((Word)1 << (high-low+1)) - 1) << lowNdxInW0)) | ((Word)bits << lowNdxInW0);
		else
		{
			DE_ASSERT(word1Ndx == word0Ndx + 1);

			const int	highNdxInW1			= high % WORD_BITS;
			const int	numBitsToSetInW0	= WORD_BITS - lowNdxInW0;
			const Word	bitsLowMask			= ((Word)1 << numBitsToSetInW0) - 1;

			m_words[word0Ndx] = (m_words[word0Ndx] & (((Word)1 << lowNdxInW0) - 1))			| (((Word)bits & bitsLowMask) << lowNdxInW0);
			m_words[word1Ndx] = (m_words[word1Ndx] & ~(((Word)1 << (highNdxInW1+1)) - 1))	| (((Word)bits & ~bitsLowMask) >> numBitsToSetInW0);
		}
	}

	void assignToMemory (deUint8* dst) const
	{
		for (int wordNdx = 0; wordNdx < NUM_WORDS; wordNdx++)
		{
			for (int byteNdx = 0; byteNdx < WORD_BYTES; byteNdx++)
				dst[wordNdx*WORD_BYTES + byteNdx] = (deUint8)((m_words[wordNdx] >> (8*byteNdx)) & 0xff);
		}
	}

	void pushBytesToVector (vector<deUint8>& dst) const
	{
		const int assignStartIndex = (int)dst.size();
		dst.resize(dst.size() + BLOCK_SIZE_BYTES);
		assignToMemory(&dst[assignStartIndex]);
	}

private:
	Word m_words[NUM_WORDS];
};

// A helper for sequential access into a AssignBlock128.
class BitAssignAccessStream
{
public:
	BitAssignAccessStream (AssignBlock128& dst, int startNdxInSrc, int length, bool forward)
		: m_dst				(dst)
		, m_startNdxInSrc	(startNdxInSrc)
		, m_length			(length)
		, m_forward			(forward)
		, m_ndx				(0)
	{
	}

	// Set the next num bits. Bits at positions greater than or equal to m_length are not touched.
	void setNext (int num, deUint32 bits)
	{
		DE_ASSERT((bits & (((deUint64)1 << num) - 1)) == bits);

		if (num == 0 || m_ndx >= m_length)
			return;

		const int		end				= m_ndx + num;
		const int		numBitsToDst	= de::max(0, de::min(m_length, end) - m_ndx);
		const int		low				= m_ndx;
		const int		high			= m_ndx + numBitsToDst - 1;
		const deUint32	actualBits		= getBits(bits, 0, numBitsToDst-1);

		m_ndx += num;

		return m_forward ? m_dst.setBits(m_startNdxInSrc + low,  m_startNdxInSrc + high, actualBits)
						 : m_dst.setBits(m_startNdxInSrc - high, m_startNdxInSrc - low, reverseBits(actualBits, numBitsToDst));
	}

private:
	AssignBlock128&		m_dst;
	const int			m_startNdxInSrc;
	const int			m_length;
	const bool			m_forward;

	int					m_ndx;
};

struct VoidExtentParams
{
	DE_STATIC_ASSERT((de::meta::TypesSame<deFloat16, deUint16>::Value));
	bool		isHDR;
	deUint16	r;
	deUint16	g;
	deUint16	b;
	deUint16	a;
	// \note Currently extent coordinates are all set to all-ones.

	VoidExtentParams (bool isHDR_, deUint16 r_, deUint16 g_, deUint16 b_, deUint16 a_) : isHDR(isHDR_), r(r_), g(g_), b(b_), a(a_) {}
};

static AssignBlock128 generateVoidExtentBlock (const VoidExtentParams& params)
{
	AssignBlock128 block;

	block.setBits(0, 8, 0x1fc); // \note Marks void-extent block.
	block.setBit(9, params.isHDR);
	block.setBits(10, 11, 3); // \note Spec shows that these bits are both set, although they serve no purpose.

	// Extent coordinates - currently all-ones.
	block.setBits(12, 24, 0x1fff);
	block.setBits(25, 37, 0x1fff);
	block.setBits(38, 50, 0x1fff);
	block.setBits(51, 63, 0x1fff);

	DE_ASSERT(!params.isHDR || (!isFloat16InfOrNan(params.r) &&
								!isFloat16InfOrNan(params.g) &&
								!isFloat16InfOrNan(params.b) &&
								!isFloat16InfOrNan(params.a)));

	block.setBits(64,  79,  params.r);
	block.setBits(80,  95,  params.g);
	block.setBits(96,  111, params.b);
	block.setBits(112, 127, params.a);

	return block;
}

// An input array of ISE inputs for an entire ASTC block. Can be given as either single values in the
// range [0, maximumValueOfISERange] or as explicit block value specifications. The latter is needed
// so we can test all possible values of T and Q in a block, since multiple T or Q values may map
// to the same set of decoded values.
struct ISEInput
{
	struct Block
	{
		deUint32 tOrQValue; //!< The 8-bit T or 7-bit Q in a trit or quint ISE block.
		deUint32 bitValues[5];
	};

	bool isGivenInBlockForm;
	union
	{
		//!< \note 64 comes from the maximum number of weight values in an ASTC block.
		deUint32	plain[64];
		Block		block[64];
	} value;

	ISEInput (void)
		: isGivenInBlockForm (false)
	{
	}
};

static inline deUint32 computeISERangeMax (const ISEParams& iseParams)
{
	switch (iseParams.mode)
	{
		case ISEMODE_TRIT:			return (1u << iseParams.numBits) * 3 - 1;
		case ISEMODE_QUINT:			return (1u << iseParams.numBits) * 5 - 1;
		case ISEMODE_PLAIN_BIT:		return (1u << iseParams.numBits)     - 1;
		default:
			DE_ASSERT(false);
			return -1;
	}
}

struct NormalBlockParams
{
	int					weightGridWidth;
	int					weightGridHeight;
	ISEParams			weightISEParams;
	bool				isDualPlane;
	deUint32			ccs; //! \note Irrelevant if !isDualPlane.
	int					numPartitions;
	deUint32			colorEndpointModes[4];
	// \note Below members are irrelevant if numPartitions == 1.
	bool				isMultiPartSingleCemMode; //! \note If true, the single CEM is at colorEndpointModes[0].
	deUint32			partitionSeed;

	NormalBlockParams (void)
		: weightGridWidth			(-1)
		, weightGridHeight			(-1)
		, weightISEParams			(ISEMODE_LAST, -1)
		, isDualPlane				(true)
		, ccs						((deUint32)-1)
		, numPartitions				(-1)
		, isMultiPartSingleCemMode	(false)
		, partitionSeed				((deUint32)-1)
	{
		colorEndpointModes[0] = 0;
		colorEndpointModes[1] = 0;
		colorEndpointModes[2] = 0;
		colorEndpointModes[3] = 0;
	}
};

struct NormalBlockISEInputs
{
	ISEInput weight;
	ISEInput endpoint;

	NormalBlockISEInputs (void)
		: weight	()
		, endpoint	()
	{
	}
};

static inline int computeNumWeights (const NormalBlockParams& params)
{
	return params.weightGridWidth * params.weightGridHeight * (params.isDualPlane ? 2 : 1);
}

static inline int computeNumBitsForColorEndpoints (const NormalBlockParams& params)
{
	const int numWeightBits			= computeNumRequiredBits(params.weightISEParams, computeNumWeights(params));
	const int numConfigDataBits		= (params.numPartitions == 1 ? 17 : params.isMultiPartSingleCemMode ? 29 : 25 + 3*params.numPartitions) +
									  (params.isDualPlane ? 2 : 0);

	return 128 - numWeightBits - numConfigDataBits;
}

static inline int computeNumColorEndpointValues (const deUint32* endpointModes, int numPartitions, bool isMultiPartSingleCemMode)
{
	if (isMultiPartSingleCemMode)
		return numPartitions * computeNumColorEndpointValues(endpointModes[0]);
	else
	{
		int result = 0;
		for (int i = 0; i < numPartitions; i++)
			result += computeNumColorEndpointValues(endpointModes[i]);
		return result;
	}
}

static inline bool isValidBlockParams (const NormalBlockParams& params, int blockWidth, int blockHeight)
{
	const int numWeights				= computeNumWeights(params);
	const int numWeightBits				= computeNumRequiredBits(params.weightISEParams, numWeights);
	const int numColorEndpointValues	= computeNumColorEndpointValues(&params.colorEndpointModes[0], params.numPartitions, params.isMultiPartSingleCemMode);
	const int numBitsForColorEndpoints	= computeNumBitsForColorEndpoints(params);

	return numWeights <= 64										&&
		   de::inRange(numWeightBits, 24, 96)					&&
		   params.weightGridWidth <= blockWidth					&&
		   params.weightGridHeight <= blockHeight				&&
		   !(params.numPartitions == 4 && params.isDualPlane)	&&
		   numColorEndpointValues <= 18							&&
		   numBitsForColorEndpoints >= deDivRoundUp32(13*numColorEndpointValues, 5);
}

// Write bits 0 to 10 of an ASTC block.
static void writeBlockMode (AssignBlock128& dst, const NormalBlockParams& blockParams)
{
	const deUint32	d = blockParams.isDualPlane != 0;
	// r and h initialized in switch below.
	deUint32		r;
	deUint32		h;
	// a, b and blockModeLayoutNdx initialized in block mode layout index detecting loop below.
	deUint32		a = (deUint32)-1;
	deUint32		b = (deUint32)-1;
	int				blockModeLayoutNdx;

	// Find the values of r and h (ISE range).
	switch (computeISERangeMax(blockParams.weightISEParams))
	{
		case 1:		r = 2; h = 0;	break;
		case 2:		r = 3; h = 0;	break;
		case 3:		r = 4; h = 0;	break;
		case 4:		r = 5; h = 0;	break;
		case 5:		r = 6; h = 0;	break;
		case 7:		r = 7; h = 0;	break;

		case 9:		r = 2; h = 1;	break;
		case 11:	r = 3; h = 1;	break;
		case 15:	r = 4; h = 1;	break;
		case 19:	r = 5; h = 1;	break;
		case 23:	r = 6; h = 1;	break;
		case 31:	r = 7; h = 1;	break;

		default:
			DE_ASSERT(false);
			r = (deUint32)-1;
			h = (deUint32)-1;
	}

	// Find block mode layout index, i.e. appropriate row in the "2d block mode layout" table in ASTC spec.

	{
		enum BlockModeLayoutABVariable { Z=0, A=1, B=2 };

		static const struct BlockModeLayout
		{
			int							aNumBits;
			int							bNumBits;
			BlockModeLayoutABVariable	gridWidthVariableTerm;
			int							gridWidthConstantTerm;
			BlockModeLayoutABVariable	gridHeightVariableTerm;
			int							gridHeightConstantTerm;
		} blockModeLayouts[] =
		{
			{ 2, 2,   B,  4,   A,  2},
			{ 2, 2,   B,  8,   A,  2},
			{ 2, 2,   A,  2,   B,  8},
			{ 2, 1,   A,  2,   B,  6},
			{ 2, 1,   B,  2,   A,  2},
			{ 2, 0,   Z, 12,   A,  2},
			{ 2, 0,   A,  2,   Z, 12},
			{ 0, 0,   Z,  6,   Z, 10},
			{ 0, 0,   Z, 10,   Z,  6},
			{ 2, 2,   A,  6,   B,  6}
		};

		for (blockModeLayoutNdx = 0; blockModeLayoutNdx < DE_LENGTH_OF_ARRAY(blockModeLayouts); blockModeLayoutNdx++)
		{
			const BlockModeLayout&	layout					= blockModeLayouts[blockModeLayoutNdx];
			const int				aMax					= (1 << layout.aNumBits) - 1;
			const int				bMax					= (1 << layout.bNumBits) - 1;
			const int				variableOffsetsMax[3]	= { 0, aMax, bMax };
			const int				widthMin				= layout.gridWidthConstantTerm;
			const int				heightMin				= layout.gridHeightConstantTerm;
			const int				widthMax				= widthMin  + variableOffsetsMax[layout.gridWidthVariableTerm];
			const int				heightMax				= heightMin + variableOffsetsMax[layout.gridHeightVariableTerm];

			DE_ASSERT(layout.gridWidthVariableTerm != layout.gridHeightVariableTerm || layout.gridWidthVariableTerm == Z);

			if (de::inRange(blockParams.weightGridWidth, widthMin, widthMax) &&
				de::inRange(blockParams.weightGridHeight, heightMin, heightMax))
			{
				deUint32	dummy			= 0;
				deUint32&	widthVariable	= layout.gridWidthVariableTerm == A  ? a : layout.gridWidthVariableTerm == B  ? b : dummy;
				deUint32&	heightVariable	= layout.gridHeightVariableTerm == A ? a : layout.gridHeightVariableTerm == B ? b : dummy;

				widthVariable	= blockParams.weightGridWidth  - layout.gridWidthConstantTerm;
				heightVariable	= blockParams.weightGridHeight - layout.gridHeightConstantTerm;

				break;
			}
		}
	}

	// Set block mode bits.

	const deUint32 a0 = getBit(a, 0);
	const deUint32 a1 = getBit(a, 1);
	const deUint32 b0 = getBit(b, 0);
	const deUint32 b1 = getBit(b, 1);
	const deUint32 r0 = getBit(r, 0);
	const deUint32 r1 = getBit(r, 1);
	const deUint32 r2 = getBit(r, 2);

#define SB(NDX, VAL) dst.setBit((NDX), (VAL))
#define ASSIGN_BITS(B10, B9, B8, B7, B6, B5, B4, B3, B2, B1, B0) do { SB(10,(B10)); SB(9,(B9)); SB(8,(B8)); SB(7,(B7)); SB(6,(B6)); SB(5,(B5)); SB(4,(B4)); SB(3,(B3)); SB(2,(B2)); SB(1,(B1)); SB(0,(B0)); } while (false)

	switch (blockModeLayoutNdx)
	{
		case 0: ASSIGN_BITS(d,  h,  b1, b0, a1, a0, r0, 0,  0,  r2, r1);									break;
		case 1: ASSIGN_BITS(d,  h,  b1, b0, a1, a0, r0, 0,  1,  r2, r1);									break;
		case 2: ASSIGN_BITS(d,  h,  b1, b0, a1, a0, r0, 1,  0,  r2, r1);									break;
		case 3: ASSIGN_BITS(d,  h,   0,  b, a1, a0, r0, 1,  1,  r2, r1);									break;
		case 4: ASSIGN_BITS(d,  h,   1,  b, a1, a0, r0, 1,  1,  r2, r1);									break;
		case 5: ASSIGN_BITS(d,  h,   0,  0, a1, a0, r0, r2, r1,  0,  0);									break;
		case 6: ASSIGN_BITS(d,  h,   0,  1, a1, a0, r0, r2, r1,  0,  0);									break;
		case 7: ASSIGN_BITS(d,  h,   1,  1,  0,  0, r0, r2, r1,  0,  0);									break;
		case 8: ASSIGN_BITS(d,  h,   1,  1,  0,  1, r0, r2, r1,  0,  0);									break;
		case 9: ASSIGN_BITS(b1, b0,  1,  0, a1, a0, r0, r2, r1,  0,  0); DE_ASSERT(d == 0 && h == 0);		break;
		default:
			DE_ASSERT(false);
	}

#undef ASSIGN_BITS
#undef SB
}

// Write color endpoint mode data of an ASTC block.
static void writeColorEndpointModes (AssignBlock128& dst, const deUint32* colorEndpointModes, bool isMultiPartSingleCemMode, int numPartitions, int extraCemBitsStart)
{
	if (numPartitions == 1)
		dst.setBits(13, 16, colorEndpointModes[0]);
	else
	{
		if (isMultiPartSingleCemMode)
		{
			dst.setBits(23, 24, 0);
			dst.setBits(25, 28, colorEndpointModes[0]);
		}
		else
		{
			DE_ASSERT(numPartitions > 0);
			const deUint32 minCem				= *std::min_element(&colorEndpointModes[0], &colorEndpointModes[numPartitions]);
			const deUint32 maxCem				= *std::max_element(&colorEndpointModes[0], &colorEndpointModes[numPartitions]);
			const deUint32 minCemClass			= minCem/4;
			const deUint32 maxCemClass			= maxCem/4;
			DE_ASSERT(maxCemClass - minCemClass <= 1);
			DE_UNREF(minCemClass); // \note For non-debug builds.
			const deUint32 highLevelSelector	= de::max(1u, maxCemClass);

			dst.setBits(23, 24, highLevelSelector);

			for (int partNdx = 0; partNdx < numPartitions; partNdx++)
			{
				const deUint32 c			= colorEndpointModes[partNdx] / 4 == highLevelSelector ? 1 : 0;
				const deUint32 m			= colorEndpointModes[partNdx] % 4;
				const deUint32 lowMBit0Ndx	= numPartitions + 2*partNdx;
				const deUint32 lowMBit1Ndx	= numPartitions + 2*partNdx + 1;
				dst.setBit(25 + partNdx, c);
				dst.setBit(lowMBit0Ndx < 4 ? 25+lowMBit0Ndx : extraCemBitsStart+lowMBit0Ndx-4, getBit(m, 0));
				dst.setBit(lowMBit1Ndx < 4 ? 25+lowMBit1Ndx : extraCemBitsStart+lowMBit1Ndx-4, getBit(m, 1));
			}
		}
	}
}

static void encodeISETritBlock (BitAssignAccessStream& dst, int numBits, bool fromExplicitInputBlock, const ISEInput::Block& blockInput, const deUint32* nonBlockInput, int numValues)
{
	// tritBlockTValue[t0][t1][t2][t3][t4] is a value of T (not necessarily the only one) that will yield the given trits when decoded.
	static const deUint32 tritBlockTValue[3][3][3][3][3] =
	{
		{
			{{{0, 128, 96}, {32, 160, 224}, {64, 192, 28}}, {{16, 144, 112}, {48, 176, 240}, {80, 208, 156}}, {{3, 131, 99}, {35, 163, 227}, {67, 195, 31}}},
			{{{4, 132, 100}, {36, 164, 228}, {68, 196, 60}}, {{20, 148, 116}, {52, 180, 244}, {84, 212, 188}}, {{19, 147, 115}, {51, 179, 243}, {83, 211, 159}}},
			{{{8, 136, 104}, {40, 168, 232}, {72, 200, 92}}, {{24, 152, 120}, {56, 184, 248}, {88, 216, 220}}, {{12, 140, 108}, {44, 172, 236}, {76, 204, 124}}}
		},
		{
			{{{1, 129, 97}, {33, 161, 225}, {65, 193, 29}}, {{17, 145, 113}, {49, 177, 241}, {81, 209, 157}}, {{7, 135, 103}, {39, 167, 231}, {71, 199, 63}}},
			{{{5, 133, 101}, {37, 165, 229}, {69, 197, 61}}, {{21, 149, 117}, {53, 181, 245}, {85, 213, 189}}, {{23, 151, 119}, {55, 183, 247}, {87, 215, 191}}},
			{{{9, 137, 105}, {41, 169, 233}, {73, 201, 93}}, {{25, 153, 121}, {57, 185, 249}, {89, 217, 221}}, {{13, 141, 109}, {45, 173, 237}, {77, 205, 125}}}
		},
		{
			{{{2, 130, 98}, {34, 162, 226}, {66, 194, 30}}, {{18, 146, 114}, {50, 178, 242}, {82, 210, 158}}, {{11, 139, 107}, {43, 171, 235}, {75, 203, 95}}},
			{{{6, 134, 102}, {38, 166, 230}, {70, 198, 62}}, {{22, 150, 118}, {54, 182, 246}, {86, 214, 190}}, {{27, 155, 123}, {59, 187, 251}, {91, 219, 223}}},
			{{{10, 138, 106}, {42, 170, 234}, {74, 202, 94}}, {{26, 154, 122}, {58, 186, 250}, {90, 218, 222}}, {{14, 142, 110}, {46, 174, 238}, {78, 206, 126}}}
		}
	};

	DE_ASSERT(de::inRange(numValues, 1, 5));

	deUint32 tritParts[5];
	deUint32 bitParts[5];

	for (int i = 0; i < 5; i++)
	{
		if (i < numValues)
		{
			if (fromExplicitInputBlock)
			{
				bitParts[i]		= blockInput.bitValues[i];
				tritParts[i]	= -1; // \note Won't be used, but silences warning.
			}
			else
			{
				// \todo [2016-01-20 pyry] numBits = 0 doesn't make sense
				bitParts[i]		= numBits > 0 ? getBits(nonBlockInput[i], 0, numBits-1) : 0;
				tritParts[i]	= nonBlockInput[i] >> numBits;
			}
		}
		else
		{
			bitParts[i]		= 0;
			tritParts[i]	= 0;
		}
	}

	const deUint32 T = fromExplicitInputBlock ? blockInput.tOrQValue : tritBlockTValue[tritParts[0]]
																					  [tritParts[1]]
																					  [tritParts[2]]
																					  [tritParts[3]]
																					  [tritParts[4]];

	dst.setNext(numBits,	bitParts[0]);
	dst.setNext(2,			getBits(T, 0, 1));
	dst.setNext(numBits,	bitParts[1]);
	dst.setNext(2,			getBits(T, 2, 3));
	dst.setNext(numBits,	bitParts[2]);
	dst.setNext(1,			getBit(T, 4));
	dst.setNext(numBits,	bitParts[3]);
	dst.setNext(2,			getBits(T, 5, 6));
	dst.setNext(numBits,	bitParts[4]);
	dst.setNext(1,			getBit(T, 7));
}

static void encodeISEQuintBlock (BitAssignAccessStream& dst, int numBits, bool fromExplicitInputBlock, const ISEInput::Block& blockInput, const deUint32* nonBlockInput, int numValues)
{
	// quintBlockQValue[q0][q1][q2] is a value of Q (not necessarily the only one) that will yield the given quints when decoded.
	static const deUint32 quintBlockQValue[5][5][5] =
	{
		{{0, 32, 64, 96, 102}, {8, 40, 72, 104, 110}, {16, 48, 80, 112, 118}, {24, 56, 88, 120, 126}, {5, 37, 69, 101, 39}},
		{{1, 33, 65, 97, 103}, {9, 41, 73, 105, 111}, {17, 49, 81, 113, 119}, {25, 57, 89, 121, 127}, {13, 45, 77, 109, 47}},
		{{2, 34, 66, 98, 70}, {10, 42, 74, 106, 78}, {18, 50, 82, 114, 86}, {26, 58, 90, 122, 94}, {21, 53, 85, 117, 55}},
		{{3, 35, 67, 99, 71}, {11, 43, 75, 107, 79}, {19, 51, 83, 115, 87}, {27, 59, 91, 123, 95}, {29, 61, 93, 125, 63}},
		{{4, 36, 68, 100, 38}, {12, 44, 76, 108, 46}, {20, 52, 84, 116, 54}, {28, 60, 92, 124, 62}, {6, 14, 22, 30, 7}}
	};

	DE_ASSERT(de::inRange(numValues, 1, 3));

	deUint32 quintParts[3];
	deUint32 bitParts[3];

	for (int i = 0; i < 3; i++)
	{
		if (i < numValues)
		{
			if (fromExplicitInputBlock)
			{
				bitParts[i]		= blockInput.bitValues[i];
				quintParts[i]	= -1; // \note Won't be used, but silences warning.
			}
			else
			{
				// \todo [2016-01-20 pyry] numBits = 0 doesn't make sense
				bitParts[i]		= numBits > 0 ? getBits(nonBlockInput[i], 0, numBits-1) : 0;
				quintParts[i]	= nonBlockInput[i] >> numBits;
			}
		}
		else
		{
			bitParts[i]		= 0;
			quintParts[i]	= 0;
		}
	}

	const deUint32 Q = fromExplicitInputBlock ? blockInput.tOrQValue : quintBlockQValue[quintParts[0]]
																					   [quintParts[1]]
																					   [quintParts[2]];

	dst.setNext(numBits,	bitParts[0]);
	dst.setNext(3,			getBits(Q, 0, 2));
	dst.setNext(numBits,	bitParts[1]);
	dst.setNext(2,			getBits(Q, 3, 4));
	dst.setNext(numBits,	bitParts[2]);
	dst.setNext(2,			getBits(Q, 5, 6));
}

static void encodeISEBitBlock (BitAssignAccessStream& dst, int numBits, deUint32 value)
{
	DE_ASSERT(de::inRange(value, 0u, (1u<<numBits)-1));
	dst.setNext(numBits, value);
}

static void encodeISE (BitAssignAccessStream& dst, const ISEParams& params, const ISEInput& input, int numValues)
{
	if (params.mode == ISEMODE_TRIT)
	{
		const int numBlocks = deDivRoundUp32(numValues, 5);
		for (int blockNdx = 0; blockNdx < numBlocks; blockNdx++)
		{
			const int numValuesInBlock = blockNdx == numBlocks-1 ? numValues - 5*(numBlocks-1) : 5;
			encodeISETritBlock(dst, params.numBits, input.isGivenInBlockForm,
							   input.isGivenInBlockForm ? input.value.block[blockNdx]	: ISEInput::Block(),
							   input.isGivenInBlockForm ? DE_NULL						: &input.value.plain[5*blockNdx],
							   numValuesInBlock);
		}
	}
	else if (params.mode == ISEMODE_QUINT)
	{
		const int numBlocks = deDivRoundUp32(numValues, 3);
		for (int blockNdx = 0; blockNdx < numBlocks; blockNdx++)
		{
			const int numValuesInBlock = blockNdx == numBlocks-1 ? numValues - 3*(numBlocks-1) : 3;
			encodeISEQuintBlock(dst, params.numBits, input.isGivenInBlockForm,
								input.isGivenInBlockForm ? input.value.block[blockNdx]	: ISEInput::Block(),
								input.isGivenInBlockForm ? DE_NULL						: &input.value.plain[3*blockNdx],
								numValuesInBlock);
		}
	}
	else
	{
		DE_ASSERT(params.mode == ISEMODE_PLAIN_BIT);
		for (int i = 0; i < numValues; i++)
			encodeISEBitBlock(dst, params.numBits, input.isGivenInBlockForm ? input.value.block[i].bitValues[0] : input.value.plain[i]);
	}
}

static void writeWeightData (AssignBlock128& dst, const ISEParams& iseParams, const ISEInput& input, int numWeights)
{
	const int				numWeightBits	= computeNumRequiredBits(iseParams, numWeights);
	BitAssignAccessStream	access			(dst, 127, numWeightBits, false);
	encodeISE(access, iseParams, input, numWeights);
}

static void writeColorEndpointData (AssignBlock128& dst, const ISEParams& iseParams, const ISEInput& input, int numEndpoints, int numBitsForColorEndpoints, int colorEndpointDataStartNdx)
{
	BitAssignAccessStream access(dst, colorEndpointDataStartNdx, numBitsForColorEndpoints, true);
	encodeISE(access, iseParams, input, numEndpoints);
}

static AssignBlock128 generateNormalBlock (const NormalBlockParams& blockParams, int blockWidth, int blockHeight, const NormalBlockISEInputs& iseInputs)
{
	DE_ASSERT(isValidBlockParams(blockParams, blockWidth, blockHeight));
	DE_UNREF(blockWidth);	// \note For non-debug builds.
	DE_UNREF(blockHeight);	// \note For non-debug builds.

	AssignBlock128	block;
	const int		numWeights		= computeNumWeights(blockParams);
	const int		numWeightBits	= computeNumRequiredBits(blockParams.weightISEParams, numWeights);

	writeBlockMode(block, blockParams);

	block.setBits(11, 12, blockParams.numPartitions - 1);
	if (blockParams.numPartitions > 1)
		block.setBits(13, 22, blockParams.partitionSeed);

	{
		const int extraCemBitsStart = 127 - numWeightBits - (blockParams.numPartitions == 1 || blockParams.isMultiPartSingleCemMode		? -1
															: blockParams.numPartitions == 4											? 7
															: blockParams.numPartitions == 3											? 4
															: blockParams.numPartitions == 2											? 1
															: 0);

		writeColorEndpointModes(block, &blockParams.colorEndpointModes[0], blockParams.isMultiPartSingleCemMode, blockParams.numPartitions, extraCemBitsStart);

		if (blockParams.isDualPlane)
			block.setBits(extraCemBitsStart-2, extraCemBitsStart-1, blockParams.ccs);
	}

	writeWeightData(block, blockParams.weightISEParams, iseInputs.weight, numWeights);

	{
		const int			numColorEndpointValues		= computeNumColorEndpointValues(&blockParams.colorEndpointModes[0], blockParams.numPartitions, blockParams.isMultiPartSingleCemMode);
		const int			numBitsForColorEndpoints	= computeNumBitsForColorEndpoints(blockParams);
		const int			colorEndpointDataStartNdx	= blockParams.numPartitions == 1 ? 17 : 29;
		const ISEParams&	colorEndpointISEParams		= computeMaximumRangeISEParams(numBitsForColorEndpoints, numColorEndpointValues);

		writeColorEndpointData(block, colorEndpointISEParams, iseInputs.endpoint, numColorEndpointValues, numBitsForColorEndpoints, colorEndpointDataStartNdx);
	}

	return block;
}

// Generate default ISE inputs for weight and endpoint data - gradient-ish values.
static NormalBlockISEInputs generateDefaultISEInputs (const NormalBlockParams& blockParams)
{
	NormalBlockISEInputs result;

	{
		result.weight.isGivenInBlockForm = false;

		const int numWeights		= computeNumWeights(blockParams);
		const int weightRangeMax	= computeISERangeMax(blockParams.weightISEParams);

		if (blockParams.isDualPlane)
		{
			for (int i = 0; i < numWeights; i += 2)
				result.weight.value.plain[i] = (i*weightRangeMax + (numWeights-1)/2) / (numWeights-1);

			for (int i = 1; i < numWeights; i += 2)
				result.weight.value.plain[i] = weightRangeMax - (i*weightRangeMax + (numWeights-1)/2) / (numWeights-1);
		}
		else
		{
			for (int i = 0; i < numWeights; i++)
				result.weight.value.plain[i] = (i*weightRangeMax + (numWeights-1)/2) / (numWeights-1);
		}
	}

	{
		result.endpoint.isGivenInBlockForm = false;

		const int			numColorEndpointValues		= computeNumColorEndpointValues(&blockParams.colorEndpointModes[0], blockParams.numPartitions, blockParams.isMultiPartSingleCemMode);
		const int			numBitsForColorEndpoints	= computeNumBitsForColorEndpoints(blockParams);
		const ISEParams&	colorEndpointISEParams		= computeMaximumRangeISEParams(numBitsForColorEndpoints, numColorEndpointValues);
		const int			colorEndpointRangeMax		= computeISERangeMax(colorEndpointISEParams);

		for (int i = 0; i < numColorEndpointValues; i++)
			result.endpoint.value.plain[i] = (i*colorEndpointRangeMax + (numColorEndpointValues-1)/2) / (numColorEndpointValues-1);
	}

	return result;
}

static const ISEParams s_weightISEParamsCandidates[] =
{
	ISEParams(ISEMODE_PLAIN_BIT,	1),
	ISEParams(ISEMODE_TRIT,			0),
	ISEParams(ISEMODE_PLAIN_BIT,	2),
	ISEParams(ISEMODE_QUINT,		0),
	ISEParams(ISEMODE_TRIT,			1),
	ISEParams(ISEMODE_PLAIN_BIT,	3),
	ISEParams(ISEMODE_QUINT,		1),
	ISEParams(ISEMODE_TRIT,			2),
	ISEParams(ISEMODE_PLAIN_BIT,	4),
	ISEParams(ISEMODE_QUINT,		2),
	ISEParams(ISEMODE_TRIT,			3),
	ISEParams(ISEMODE_PLAIN_BIT,	5)
};

void generateRandomBlock (deUint8* dst, const IVec3& blockSize, de::Random& rnd)
{
	DE_ASSERT(blockSize.z() == 1);

	if (rnd.getFloat() < 0.1f)
	{
		// Void extent block.
		const bool		isVoidExtentHDR		= rnd.getBool();
		const deUint16	r					= isVoidExtentHDR ? deFloat32To16(rnd.getFloat(0.0f, 1.0f)) : (deUint16)rnd.getInt(0, 0xffff);
		const deUint16	g					= isVoidExtentHDR ? deFloat32To16(rnd.getFloat(0.0f, 1.0f)) : (deUint16)rnd.getInt(0, 0xffff);
		const deUint16	b					= isVoidExtentHDR ? deFloat32To16(rnd.getFloat(0.0f, 1.0f)) : (deUint16)rnd.getInt(0, 0xffff);
		const deUint16	a					= isVoidExtentHDR ? deFloat32To16(rnd.getFloat(0.0f, 1.0f)) : (deUint16)rnd.getInt(0, 0xffff);
		generateVoidExtentBlock(VoidExtentParams(isVoidExtentHDR, r, g, b, a)).assignToMemory(dst);
	}
	else
	{
		// Not void extent block.

		// Generate block params.

		NormalBlockParams blockParams;

		do
		{
			blockParams.weightGridWidth				= rnd.getInt(2, blockSize.x());
			blockParams.weightGridHeight			= rnd.getInt(2, blockSize.y());
			blockParams.weightISEParams				= s_weightISEParamsCandidates[rnd.getInt(0, DE_LENGTH_OF_ARRAY(s_weightISEParamsCandidates)-1)];
			blockParams.numPartitions				= rnd.getInt(1, 4);
			blockParams.isMultiPartSingleCemMode	= rnd.getFloat() < 0.25f;
			blockParams.isDualPlane					= blockParams.numPartitions != 4 && rnd.getBool();
			blockParams.ccs							= rnd.getInt(0, 3);
			blockParams.partitionSeed				= rnd.getInt(0, 1023);

			blockParams.colorEndpointModes[0] = rnd.getInt(0, 15);

			{
				const int cemDiff = blockParams.isMultiPartSingleCemMode		? 0
									: blockParams.colorEndpointModes[0] == 0	? 1
									: blockParams.colorEndpointModes[0] == 15	? -1
									: rnd.getBool()								? 1 : -1;

				for (int i = 1; i < blockParams.numPartitions; i++)
					blockParams.colorEndpointModes[i] = blockParams.colorEndpointModes[0] + (cemDiff == -1 ? rnd.getInt(-1, 0) : cemDiff == 1 ? rnd.getInt(0, 1) : 0);
			}
		} while (!isValidBlockParams(blockParams, blockSize.x(), blockSize.y()));

		// Generate ISE inputs for both weight and endpoint data.

		NormalBlockISEInputs iseInputs;

		for (int weightOrEndpoints = 0; weightOrEndpoints <= 1; weightOrEndpoints++)
		{
			const bool			setWeights	= weightOrEndpoints == 0;
			const int			numValues	= setWeights ? computeNumWeights(blockParams) :
												computeNumColorEndpointValues(&blockParams.colorEndpointModes[0], blockParams.numPartitions, blockParams.isMultiPartSingleCemMode);
			const ISEParams		iseParams	= setWeights ? blockParams.weightISEParams : computeMaximumRangeISEParams(computeNumBitsForColorEndpoints(blockParams), numValues);
			ISEInput&			iseInput	= setWeights ? iseInputs.weight : iseInputs.endpoint;

			iseInput.isGivenInBlockForm = rnd.getBool();

			if (iseInput.isGivenInBlockForm)
			{
				const int numValuesPerISEBlock	= iseParams.mode == ISEMODE_TRIT	? 5
												: iseParams.mode == ISEMODE_QUINT	? 3
												:									  1;
				const int iseBitMax				= (1 << iseParams.numBits) - 1;
				const int numISEBlocks			= deDivRoundUp32(numValues, numValuesPerISEBlock);

				for (int iseBlockNdx = 0; iseBlockNdx < numISEBlocks; iseBlockNdx++)
				{
					iseInput.value.block[iseBlockNdx].tOrQValue = rnd.getInt(0, 255);
					for (int i = 0; i < numValuesPerISEBlock; i++)
						iseInput.value.block[iseBlockNdx].bitValues[i] = rnd.getInt(0, iseBitMax);
				}
			}
			else
			{
				const int rangeMax = computeISERangeMax(iseParams);

				for (int valueNdx = 0; valueNdx < numValues; valueNdx++)
					iseInput.value.plain[valueNdx] = rnd.getInt(0, rangeMax);
			}
		}

		generateNormalBlock(blockParams, blockSize.x(), blockSize.y(), iseInputs).assignToMemory(dst);
	}
}

} // anonymous

// Generate block data for a given BlockTestType and format.
void generateBlockCaseTestData (vector<deUint8>& dst, CompressedTexFormat format, BlockTestType testType)
{
	DE_ASSERT(isAstcFormat(format));
	DE_ASSERT(!(isAstcSRGBFormat(format) && isBlockTestTypeHDROnly(testType)));

	const IVec3 blockSize = getBlockPixelSize(format);
	DE_ASSERT(blockSize.z() == 1);

	switch (testType)
	{
		case BLOCK_TEST_TYPE_VOID_EXTENT_LDR:
		// Generate a gradient-like set of LDR void-extent blocks.
		{
			const int			numBlocks	= 1<<13;
			const deUint32		numValues	= 1<<16;
			dst.reserve(numBlocks*BLOCK_SIZE_BYTES);

			for (int blockNdx = 0; blockNdx < numBlocks; blockNdx++)
			{
				const deUint32 baseValue	= blockNdx*(numValues-1) / (numBlocks-1);
				const deUint16 r			= (deUint16)((baseValue + numValues*0/4) % numValues);
				const deUint16 g			= (deUint16)((baseValue + numValues*1/4) % numValues);
				const deUint16 b			= (deUint16)((baseValue + numValues*2/4) % numValues);
				const deUint16 a			= (deUint16)((baseValue + numValues*3/4) % numValues);
				AssignBlock128 block;

				generateVoidExtentBlock(VoidExtentParams(false, r, g, b, a)).pushBytesToVector(dst);
			}

			break;
		}

		case BLOCK_TEST_TYPE_VOID_EXTENT_HDR:
		// Generate a gradient-like set of HDR void-extent blocks, with values ranging from the largest finite negative to largest finite positive of fp16.
		{
			const float		minValue	= -65504.0f;
			const float		maxValue	= +65504.0f;
			const int		numBlocks	= 1<<13;
			dst.reserve(numBlocks*BLOCK_SIZE_BYTES);

			for (int blockNdx = 0; blockNdx < numBlocks; blockNdx++)
			{
				const int			rNdx	= (blockNdx + numBlocks*0/4) % numBlocks;
				const int			gNdx	= (blockNdx + numBlocks*1/4) % numBlocks;
				const int			bNdx	= (blockNdx + numBlocks*2/4) % numBlocks;
				const int			aNdx	= (blockNdx + numBlocks*3/4) % numBlocks;
				const deFloat16		r		= deFloat32To16(minValue + (float)rNdx * (maxValue - minValue) / (float)(numBlocks-1));
				const deFloat16		g		= deFloat32To16(minValue + (float)gNdx * (maxValue - minValue) / (float)(numBlocks-1));
				const deFloat16		b		= deFloat32To16(minValue + (float)bNdx * (maxValue - minValue) / (float)(numBlocks-1));
				const deFloat16		a		= deFloat32To16(minValue + (float)aNdx * (maxValue - minValue) / (float)(numBlocks-1));

				generateVoidExtentBlock(VoidExtentParams(true, r, g, b, a)).pushBytesToVector(dst);
			}

			break;
		}

		case BLOCK_TEST_TYPE_WEIGHT_GRID:
		// Generate different combinations of plane count, weight ISE params, and grid size.
		{
			for (int isDualPlane = 0;		isDualPlane <= 1;												isDualPlane++)
			for (int iseParamsNdx = 0;		iseParamsNdx < DE_LENGTH_OF_ARRAY(s_weightISEParamsCandidates);	iseParamsNdx++)
			for (int weightGridWidth = 2;	weightGridWidth <= 12;											weightGridWidth++)
			for (int weightGridHeight = 2;	weightGridHeight <= 12;											weightGridHeight++)
			{
				NormalBlockParams		blockParams;
				NormalBlockISEInputs	iseInputs;

				blockParams.weightGridWidth			= weightGridWidth;
				blockParams.weightGridHeight		= weightGridHeight;
				blockParams.isDualPlane				= isDualPlane != 0;
				blockParams.weightISEParams			= s_weightISEParamsCandidates[iseParamsNdx];
				blockParams.ccs						= 0;
				blockParams.numPartitions			= 1;
				blockParams.colorEndpointModes[0]	= 0;

				if (isValidBlockParams(blockParams, blockSize.x(), blockSize.y()))
					generateNormalBlock(blockParams, blockSize.x(), blockSize.y(), generateDefaultISEInputs(blockParams)).pushBytesToVector(dst);
			}

			break;
		}

		case BLOCK_TEST_TYPE_WEIGHT_ISE:
		// For each weight ISE param set, generate blocks that cover:
		// - each single value of the ISE's range, at each position inside an ISE block
		// - for trit and quint ISEs, each single T or Q value of an ISE block
		{
			for (int iseParamsNdx = 0;	iseParamsNdx < DE_LENGTH_OF_ARRAY(s_weightISEParamsCandidates);	iseParamsNdx++)
			{
				const ISEParams&	iseParams = s_weightISEParamsCandidates[iseParamsNdx];
				NormalBlockParams	blockParams;

				blockParams.weightGridWidth			= 4;
				blockParams.weightGridHeight		= 4;
				blockParams.weightISEParams			= iseParams;
				blockParams.numPartitions			= 1;
				blockParams.isDualPlane				= blockParams.weightGridWidth * blockParams.weightGridHeight < 24 ? true : false;
				blockParams.ccs						= 0;
				blockParams.colorEndpointModes[0]	= 0;

				while (!isValidBlockParams(blockParams, blockSize.x(), blockSize.y()))
				{
					blockParams.weightGridWidth--;
					blockParams.weightGridHeight--;
				}

				const int numValuesInISEBlock	= iseParams.mode == ISEMODE_TRIT ? 5 : iseParams.mode == ISEMODE_QUINT ? 3 : 1;
				const int numWeights			= computeNumWeights(blockParams);

				{
					const int				numWeightValues		= (int)computeISERangeMax(iseParams) + 1;
					const int				numBlocks			= deDivRoundUp32(numWeightValues, numWeights);
					NormalBlockISEInputs	iseInputs			= generateDefaultISEInputs(blockParams);
					iseInputs.weight.isGivenInBlockForm = false;

					for (int offset = 0;	offset < numValuesInISEBlock;	offset++)
					for (int blockNdx = 0;	blockNdx < numBlocks;			blockNdx++)
					{
						for (int weightNdx = 0; weightNdx < numWeights; weightNdx++)
							iseInputs.weight.value.plain[weightNdx] = (blockNdx*numWeights + weightNdx + offset) % numWeightValues;

						generateNormalBlock(blockParams, blockSize.x(), blockSize.y(), iseInputs).pushBytesToVector(dst);
					}
				}

				if (iseParams.mode == ISEMODE_TRIT || iseParams.mode == ISEMODE_QUINT)
				{
					NormalBlockISEInputs iseInputs = generateDefaultISEInputs(blockParams);
					iseInputs.weight.isGivenInBlockForm = true;

					const int numTQValues			= 1 << (iseParams.mode == ISEMODE_TRIT ? 8 : 7);
					const int numISEBlocksPerBlock	= deDivRoundUp32(numWeights, numValuesInISEBlock);
					const int numBlocks				= deDivRoundUp32(numTQValues, numISEBlocksPerBlock);

					for (int offset = 0;	offset < numValuesInISEBlock;	offset++)
					for (int blockNdx = 0;	blockNdx < numBlocks;			blockNdx++)
					{
						for (int iseBlockNdx = 0; iseBlockNdx < numISEBlocksPerBlock; iseBlockNdx++)
						{
							for (int i = 0; i < numValuesInISEBlock; i++)
								iseInputs.weight.value.block[iseBlockNdx].bitValues[i] = 0;
							iseInputs.weight.value.block[iseBlockNdx].tOrQValue = (blockNdx*numISEBlocksPerBlock + iseBlockNdx + offset) % numTQValues;
						}

						generateNormalBlock(blockParams, blockSize.x(), blockSize.y(), iseInputs).pushBytesToVector(dst);
					}
				}
			}

			break;
		}

		case BLOCK_TEST_TYPE_CEMS:
		// For each plane count & partition count combination, generate all color endpoint mode combinations.
		{
			for (int isDualPlane = 0;		isDualPlane <= 1;								isDualPlane++)
			for (int numPartitions = 1;		numPartitions <= (isDualPlane != 0 ? 3 : 4);	numPartitions++)
			{
				// Multi-partition, single-CEM mode.
				if (numPartitions > 1)
				{
					for (deUint32 singleCem = 0; singleCem < 16; singleCem++)
					{
						NormalBlockParams blockParams;
						blockParams.weightGridWidth				= 4;
						blockParams.weightGridHeight			= 4;
						blockParams.isDualPlane					= isDualPlane != 0;
						blockParams.ccs							= 0;
						blockParams.numPartitions				= numPartitions;
						blockParams.isMultiPartSingleCemMode	= true;
						blockParams.colorEndpointModes[0]		= singleCem;
						blockParams.partitionSeed				= 634;

						for (int iseParamsNdx = 0; iseParamsNdx < DE_LENGTH_OF_ARRAY(s_weightISEParamsCandidates); iseParamsNdx++)
						{
							blockParams.weightISEParams = s_weightISEParamsCandidates[iseParamsNdx];
							if (isValidBlockParams(blockParams, blockSize.x(), blockSize.y()))
							{
								generateNormalBlock(blockParams, blockSize.x(), blockSize.y(), generateDefaultISEInputs(blockParams)).pushBytesToVector(dst);
								break;
							}
						}
					}
				}

				// Separate-CEM mode.
				for (deUint32 cem0 = 0; cem0 < 16; cem0++)
				for (deUint32 cem1 = 0; cem1 < (numPartitions >= 2 ? 16u : 1u); cem1++)
				for (deUint32 cem2 = 0; cem2 < (numPartitions >= 3 ? 16u : 1u); cem2++)
				for (deUint32 cem3 = 0; cem3 < (numPartitions >= 4 ? 16u : 1u); cem3++)
				{
					NormalBlockParams blockParams;
					blockParams.weightGridWidth				= 4;
					blockParams.weightGridHeight			= 4;
					blockParams.isDualPlane					= isDualPlane != 0;
					blockParams.ccs							= 0;
					blockParams.numPartitions				= numPartitions;
					blockParams.isMultiPartSingleCemMode	= false;
					blockParams.colorEndpointModes[0]		= cem0;
					blockParams.colorEndpointModes[1]		= cem1;
					blockParams.colorEndpointModes[2]		= cem2;
					blockParams.colorEndpointModes[3]		= cem3;
					blockParams.partitionSeed				= 634;

					{
						const deUint32 minCem		= *std::min_element(&blockParams.colorEndpointModes[0], &blockParams.colorEndpointModes[numPartitions]);
						const deUint32 maxCem		= *std::max_element(&blockParams.colorEndpointModes[0], &blockParams.colorEndpointModes[numPartitions]);
						const deUint32 minCemClass	= minCem/4;
						const deUint32 maxCemClass	= maxCem/4;

						if (maxCemClass - minCemClass > 1)
							continue;
					}

					for (int iseParamsNdx = 0; iseParamsNdx < DE_LENGTH_OF_ARRAY(s_weightISEParamsCandidates); iseParamsNdx++)
					{
						blockParams.weightISEParams = s_weightISEParamsCandidates[iseParamsNdx];
						if (isValidBlockParams(blockParams, blockSize.x(), blockSize.y()))
						{
							generateNormalBlock(blockParams, blockSize.x(), blockSize.y(), generateDefaultISEInputs(blockParams)).pushBytesToVector(dst);
							break;
						}
					}
				}
			}

			break;
		}

		case BLOCK_TEST_TYPE_PARTITION_SEED:
		// Test all partition seeds ("partition pattern indices").
		{
			for (int		numPartitions = 2;	numPartitions <= 4;		numPartitions++)
			for (deUint32	partitionSeed = 0;	partitionSeed < 1<<10;	partitionSeed++)
			{
				NormalBlockParams blockParams;
				blockParams.weightGridWidth				= 4;
				blockParams.weightGridHeight			= 4;
				blockParams.weightISEParams				= ISEParams(ISEMODE_PLAIN_BIT, 2);
				blockParams.isDualPlane					= false;
				blockParams.numPartitions				= numPartitions;
				blockParams.isMultiPartSingleCemMode	= true;
				blockParams.colorEndpointModes[0]		= 0;
				blockParams.partitionSeed				= partitionSeed;

				generateNormalBlock(blockParams, blockSize.x(), blockSize.y(), generateDefaultISEInputs(blockParams)).pushBytesToVector(dst);
			}

			break;
		}

		// \note Fall-through.
		case BLOCK_TEST_TYPE_ENDPOINT_VALUE_LDR:
		case BLOCK_TEST_TYPE_ENDPOINT_VALUE_HDR_NO_15:
		case BLOCK_TEST_TYPE_ENDPOINT_VALUE_HDR_15:
		// For each endpoint mode, for each pair of components in the endpoint value, test 10x10 combinations of values for that pair.
		// \note Separate modes for HDR and mode 15 due to different color scales and biases.
		{
			for (deUint32 cem = 0; cem < 16; cem++)
			{
				const bool isHDRCem = cem == 2		||
									  cem == 3		||
									  cem == 7		||
									  cem == 11		||
									  cem == 14		||
									  cem == 15;

				if ((testType == BLOCK_TEST_TYPE_ENDPOINT_VALUE_LDR			&& isHDRCem)					||
					(testType == BLOCK_TEST_TYPE_ENDPOINT_VALUE_HDR_NO_15		&& (!isHDRCem || cem == 15))	||
					(testType == BLOCK_TEST_TYPE_ENDPOINT_VALUE_HDR_15		&& cem != 15))
					continue;

				NormalBlockParams blockParams;
				blockParams.weightGridWidth			= 3;
				blockParams.weightGridHeight		= 4;
				blockParams.weightISEParams			= ISEParams(ISEMODE_PLAIN_BIT, 2);
				blockParams.isDualPlane				= false;
				blockParams.numPartitions			= 1;
				blockParams.colorEndpointModes[0]	= cem;

				{
					const int			numBitsForEndpoints		= computeNumBitsForColorEndpoints(blockParams);
					const int			numEndpointParts		= computeNumColorEndpointValues(cem);
					const ISEParams		endpointISE				= computeMaximumRangeISEParams(numBitsForEndpoints, numEndpointParts);
					const int			endpointISERangeMax		= computeISERangeMax(endpointISE);

					for (int endpointPartNdx0 = 0;						endpointPartNdx0 < numEndpointParts; endpointPartNdx0++)
					for (int endpointPartNdx1 = endpointPartNdx0+1;		endpointPartNdx1 < numEndpointParts; endpointPartNdx1++)
					{
						NormalBlockISEInputs	iseInputs			= generateDefaultISEInputs(blockParams);
						const int				numEndpointValues	= de::min(10, endpointISERangeMax+1);

						for (int endpointValueNdx0 = 0; endpointValueNdx0 < numEndpointValues; endpointValueNdx0++)
						for (int endpointValueNdx1 = 0; endpointValueNdx1 < numEndpointValues; endpointValueNdx1++)
						{
							const int endpointValue0 = endpointValueNdx0 * endpointISERangeMax / (numEndpointValues-1);
							const int endpointValue1 = endpointValueNdx1 * endpointISERangeMax / (numEndpointValues-1);

							iseInputs.endpoint.value.plain[endpointPartNdx0] = endpointValue0;
							iseInputs.endpoint.value.plain[endpointPartNdx1] = endpointValue1;

							generateNormalBlock(blockParams, blockSize.x(), blockSize.y(), iseInputs).pushBytesToVector(dst);
						}
					}
				}
			}

			break;
		}

		case BLOCK_TEST_TYPE_ENDPOINT_ISE:
		// Similar to BLOCK_TEST_TYPE_WEIGHT_ISE, see above.
		{
			static const deUint32 endpointRangeMaximums[] = { 5, 9, 11, 19, 23, 39, 47, 79, 95, 159, 191 };

			for (int endpointRangeNdx = 0; endpointRangeNdx < DE_LENGTH_OF_ARRAY(endpointRangeMaximums); endpointRangeNdx++)
			{
				bool validCaseGenerated = false;

				for (int numPartitions = 1;			!validCaseGenerated && numPartitions <= 4;														numPartitions++)
				for (int isDual = 0;				!validCaseGenerated && isDual <= 1;																isDual++)
				for (int weightISEParamsNdx = 0;	!validCaseGenerated && weightISEParamsNdx < DE_LENGTH_OF_ARRAY(s_weightISEParamsCandidates);	weightISEParamsNdx++)
				for (int weightGridWidth = 2;		!validCaseGenerated && weightGridWidth <= 12;													weightGridWidth++)
				for (int weightGridHeight = 2;		!validCaseGenerated && weightGridHeight <= 12;													weightGridHeight++)
				{
					NormalBlockParams blockParams;
					blockParams.weightGridWidth				= weightGridWidth;
					blockParams.weightGridHeight			= weightGridHeight;
					blockParams.weightISEParams				= s_weightISEParamsCandidates[weightISEParamsNdx];
					blockParams.isDualPlane					= isDual != 0;
					blockParams.ccs							= 0;
					blockParams.numPartitions				= numPartitions;
					blockParams.isMultiPartSingleCemMode	= true;
					blockParams.colorEndpointModes[0]		= 12;
					blockParams.partitionSeed				= 634;

					if (isValidBlockParams(blockParams, blockSize.x(), blockSize.y()))
					{
						const ISEParams endpointISEParams = computeMaximumRangeISEParams(computeNumBitsForColorEndpoints(blockParams),
																						 computeNumColorEndpointValues(&blockParams.colorEndpointModes[0], numPartitions, true));

						if (computeISERangeMax(endpointISEParams) == endpointRangeMaximums[endpointRangeNdx])
						{
							validCaseGenerated = true;

							const int numColorEndpoints		= computeNumColorEndpointValues(&blockParams.colorEndpointModes[0], numPartitions, blockParams.isMultiPartSingleCemMode);
							const int numValuesInISEBlock	= endpointISEParams.mode == ISEMODE_TRIT ? 5 : endpointISEParams.mode == ISEMODE_QUINT ? 3 : 1;

							{
								const int				numColorEndpointValues	= (int)computeISERangeMax(endpointISEParams) + 1;
								const int				numBlocks				= deDivRoundUp32(numColorEndpointValues, numColorEndpoints);
								NormalBlockISEInputs	iseInputs				= generateDefaultISEInputs(blockParams);
								iseInputs.endpoint.isGivenInBlockForm = false;

								for (int offset = 0;	offset < numValuesInISEBlock;	offset++)
								for (int blockNdx = 0;	blockNdx < numBlocks;			blockNdx++)
								{
									for (int endpointNdx = 0; endpointNdx < numColorEndpoints; endpointNdx++)
										iseInputs.endpoint.value.plain[endpointNdx] = (blockNdx*numColorEndpoints + endpointNdx + offset) % numColorEndpointValues;

									generateNormalBlock(blockParams, blockSize.x(), blockSize.y(), iseInputs).pushBytesToVector(dst);
								}
							}

							if (endpointISEParams.mode == ISEMODE_TRIT || endpointISEParams.mode == ISEMODE_QUINT)
							{
								NormalBlockISEInputs iseInputs = generateDefaultISEInputs(blockParams);
								iseInputs.endpoint.isGivenInBlockForm = true;

								const int numTQValues			= 1 << (endpointISEParams.mode == ISEMODE_TRIT ? 8 : 7);
								const int numISEBlocksPerBlock	= deDivRoundUp32(numColorEndpoints, numValuesInISEBlock);
								const int numBlocks				= deDivRoundUp32(numTQValues, numISEBlocksPerBlock);

								for (int offset = 0;	offset < numValuesInISEBlock;	offset++)
								for (int blockNdx = 0;	blockNdx < numBlocks;			blockNdx++)
								{
									for (int iseBlockNdx = 0; iseBlockNdx < numISEBlocksPerBlock; iseBlockNdx++)
									{
										for (int i = 0; i < numValuesInISEBlock; i++)
											iseInputs.endpoint.value.block[iseBlockNdx].bitValues[i] = 0;
										iseInputs.endpoint.value.block[iseBlockNdx].tOrQValue = (blockNdx*numISEBlocksPerBlock + iseBlockNdx + offset) % numTQValues;
									}

									generateNormalBlock(blockParams, blockSize.x(), blockSize.y(), iseInputs).pushBytesToVector(dst);
								}
							}
						}
					}
				}

				DE_ASSERT(validCaseGenerated);
			}

			break;
		}

		case BLOCK_TEST_TYPE_CCS:
		// For all partition counts, test all values of the CCS (color component selector).
		{
			for (int		numPartitions = 1;		numPartitions <= 3;		numPartitions++)
			for (deUint32	ccs = 0;				ccs < 4;				ccs++)
			{
				NormalBlockParams blockParams;
				blockParams.weightGridWidth				= 3;
				blockParams.weightGridHeight			= 3;
				blockParams.weightISEParams				= ISEParams(ISEMODE_PLAIN_BIT, 2);
				blockParams.isDualPlane					= true;
				blockParams.ccs							= ccs;
				blockParams.numPartitions				= numPartitions;
				blockParams.isMultiPartSingleCemMode	= true;
				blockParams.colorEndpointModes[0]		= 8;
				blockParams.partitionSeed				= 634;

				generateNormalBlock(blockParams, blockSize.x(), blockSize.y(), generateDefaultISEInputs(blockParams)).pushBytesToVector(dst);
			}

			break;
		}

		case BLOCK_TEST_TYPE_RANDOM:
		// Generate a number of random (including invalid) blocks.
		{
			const int		numBlocks	= 16384;
			const deUint32	seed		= 1;

			dst.resize(numBlocks*BLOCK_SIZE_BYTES);

			generateRandomBlocks(&dst[0], numBlocks, format, seed);

			break;
		}

		default:
			DE_ASSERT(false);
	}
}

void generateRandomBlocks (deUint8* dst, size_t numBlocks, CompressedTexFormat format, deUint32 seed)
{
	const IVec3		blockSize			= getBlockPixelSize(format);
	de::Random		rnd					(seed);
	size_t			numBlocksGenerated	= 0;

	DE_ASSERT(isAstcFormat(format));
	DE_ASSERT(blockSize.z() == 1);

	for (numBlocksGenerated = 0; numBlocksGenerated < numBlocks; numBlocksGenerated++)
	{
		deUint8* const	curBlockPtr		= dst + numBlocksGenerated*BLOCK_SIZE_BYTES;

		generateRandomBlock(curBlockPtr, blockSize, rnd);
	}
}

void generateRandomValidBlocks (deUint8* dst, size_t numBlocks, CompressedTexFormat format, TexDecompressionParams::AstcMode mode, deUint32 seed)
{
	const IVec3		blockSize			= getBlockPixelSize(format);
	de::Random		rnd					(seed);
	size_t			numBlocksGenerated	= 0;

	DE_ASSERT(isAstcFormat(format));
	DE_ASSERT(blockSize.z() == 1);

	for (numBlocksGenerated = 0; numBlocksGenerated < numBlocks; numBlocksGenerated++)
	{
		deUint8* const	curBlockPtr		= dst + numBlocksGenerated*BLOCK_SIZE_BYTES;

		do
		{
			generateRandomBlock(curBlockPtr, blockSize, rnd);
		} while (!isValidBlock(curBlockPtr, format, mode));
	}
}

// Generate a number of trivial dummy blocks to fill unneeded space in a texture.
void generateDummyVoidExtentBlocks (deUint8* dst, size_t numBlocks)
{
	AssignBlock128 block = generateVoidExtentBlock(VoidExtentParams(false, 0, 0, 0, 0));
	for (size_t ndx = 0; ndx < numBlocks; ndx++)
		block.assignToMemory(&dst[ndx * BLOCK_SIZE_BYTES]);
}

void generateDummyNormalBlocks (deUint8* dst, size_t numBlocks, int blockWidth, int blockHeight)
{
	NormalBlockParams blockParams;

	blockParams.weightGridWidth			= 3;
	blockParams.weightGridHeight		= 3;
	blockParams.weightISEParams			= ISEParams(ISEMODE_PLAIN_BIT, 5);
	blockParams.isDualPlane				= false;
	blockParams.numPartitions			= 1;
	blockParams.colorEndpointModes[0]	= 8;

	NormalBlockISEInputs iseInputs = generateDefaultISEInputs(blockParams);
	iseInputs.weight.isGivenInBlockForm = false;

	const int numWeights		= computeNumWeights(blockParams);
	const int weightRangeMax	= computeISERangeMax(blockParams.weightISEParams);

	for (size_t blockNdx = 0; blockNdx < numBlocks; blockNdx++)
	{
		for (int weightNdx = 0; weightNdx < numWeights; weightNdx++)
			iseInputs.weight.value.plain[weightNdx] = (deUint32)((blockNdx*numWeights + weightNdx) * weightRangeMax / (numBlocks*numWeights-1));

		generateNormalBlock(blockParams, blockWidth, blockHeight, iseInputs).assignToMemory(dst + blockNdx*BLOCK_SIZE_BYTES);
	}
}

bool isValidBlock (const deUint8* data, CompressedTexFormat format, TexDecompressionParams::AstcMode mode)
{
	const tcu::IVec3		blockPixelSize	= getBlockPixelSize(format);
	const bool				isSRGB			= isAstcSRGBFormat(format);
	const bool				isLDR			= isSRGB || mode == TexDecompressionParams::ASTCMODE_LDR;

	// sRGB is not supported in HDR mode
	DE_ASSERT(!(mode == TexDecompressionParams::ASTCMODE_HDR && isSRGB));

	union
	{
		deUint8		sRGB[MAX_BLOCK_WIDTH*MAX_BLOCK_HEIGHT*4];
		float		linear[MAX_BLOCK_WIDTH*MAX_BLOCK_HEIGHT*4];
	} tmpBuffer;
	const Block128			blockData		(data);
	const DecompressResult	result			= decompressBlock((isSRGB ? (void*)&tmpBuffer.sRGB[0] : (void*)&tmpBuffer.linear[0]),
															  blockData, blockPixelSize.x(), blockPixelSize.y(), isSRGB, isLDR);

	return result == DECOMPRESS_RESULT_VALID_BLOCK;
}

void decompress (const PixelBufferAccess& dst, const deUint8* data, CompressedTexFormat format, TexDecompressionParams::AstcMode mode)
{
	const bool			isSRGBFormat	= isAstcSRGBFormat(format);

#if defined(DE_DEBUG)
	const tcu::IVec3	blockPixelSize	= getBlockPixelSize(format);

	DE_ASSERT(dst.getWidth()	== blockPixelSize.x() &&
			  dst.getHeight()	== blockPixelSize.y() &&
			  dst.getDepth()	== blockPixelSize.z());
	DE_ASSERT(mode == TexDecompressionParams::ASTCMODE_LDR || mode == TexDecompressionParams::ASTCMODE_HDR);
#endif

	// sRGB is not supported in HDR mode
	DE_ASSERT(!(mode == TexDecompressionParams::ASTCMODE_HDR && isSRGBFormat));

	decompress(dst, data, isSRGBFormat, isSRGBFormat || mode == TexDecompressionParams::ASTCMODE_LDR);
}

const char* getBlockTestTypeName (BlockTestType testType)
{
	switch (testType)
	{
		case BLOCK_TEST_TYPE_VOID_EXTENT_LDR:			return "void_extent_ldr";
		case BLOCK_TEST_TYPE_VOID_EXTENT_HDR:			return "void_extent_hdr";
		case BLOCK_TEST_TYPE_WEIGHT_GRID:				return "weight_grid";
		case BLOCK_TEST_TYPE_WEIGHT_ISE:				return "weight_ise";
		case BLOCK_TEST_TYPE_CEMS:						return "color_endpoint_modes";
		case BLOCK_TEST_TYPE_PARTITION_SEED:			return "partition_pattern_index";
		case BLOCK_TEST_TYPE_ENDPOINT_VALUE_LDR:		return "endpoint_value_ldr";
		case BLOCK_TEST_TYPE_ENDPOINT_VALUE_HDR_NO_15:	return "endpoint_value_hdr_cem_not_15";
		case BLOCK_TEST_TYPE_ENDPOINT_VALUE_HDR_15:		return "endpoint_value_hdr_cem_15";
		case BLOCK_TEST_TYPE_ENDPOINT_ISE:				return "endpoint_ise";
		case BLOCK_TEST_TYPE_CCS:						return "color_component_selector";
		case BLOCK_TEST_TYPE_RANDOM:					return "random";
		default:
			DE_ASSERT(false);
			return DE_NULL;
	}
}

const char* getBlockTestTypeDescription (BlockTestType testType)
{
	switch (testType)
	{
		case BLOCK_TEST_TYPE_VOID_EXTENT_LDR:			return "Test void extent block, LDR mode";
		case BLOCK_TEST_TYPE_VOID_EXTENT_HDR:			return "Test void extent block, HDR mode";
		case BLOCK_TEST_TYPE_WEIGHT_GRID:				return "Test combinations of plane count, weight integer sequence encoding parameters, and weight grid size";
		case BLOCK_TEST_TYPE_WEIGHT_ISE:				return "Test different integer sequence encoding block values for weight grid";
		case BLOCK_TEST_TYPE_CEMS:						return "Test different color endpoint mode combinations, combined with different plane and partition counts";
		case BLOCK_TEST_TYPE_PARTITION_SEED:			return "Test different partition pattern indices";
		case BLOCK_TEST_TYPE_ENDPOINT_VALUE_LDR:		return "Test various combinations of each pair of color endpoint values, for each LDR color endpoint mode";
		case BLOCK_TEST_TYPE_ENDPOINT_VALUE_HDR_NO_15:	return "Test various combinations of each pair of color endpoint values, for each HDR color endpoint mode other than mode 15";
		case BLOCK_TEST_TYPE_ENDPOINT_VALUE_HDR_15:		return "Test various combinations of each pair of color endpoint values, HDR color endpoint mode 15";
		case BLOCK_TEST_TYPE_ENDPOINT_ISE:				return "Test different integer sequence encoding block values for color endpoints";
		case BLOCK_TEST_TYPE_CCS:						return "Test color component selector, for different partition counts";
		case BLOCK_TEST_TYPE_RANDOM:					return "Random block test";
		default:
			DE_ASSERT(false);
			return DE_NULL;
	}
}

bool isBlockTestTypeHDROnly (BlockTestType testType)
{
	return testType == BLOCK_TEST_TYPE_VOID_EXTENT_HDR			||
		   testType == BLOCK_TEST_TYPE_ENDPOINT_VALUE_HDR_NO_15	||
		   testType == BLOCK_TEST_TYPE_ENDPOINT_VALUE_HDR_15;
}

Vec4 getBlockTestTypeColorScale (BlockTestType testType)
{
	switch (testType)
	{
		case tcu::astc::BLOCK_TEST_TYPE_VOID_EXTENT_HDR:			return Vec4(0.5f/65504.0f);
		case tcu::astc::BLOCK_TEST_TYPE_ENDPOINT_VALUE_HDR_NO_15:	return Vec4(1.0f/65504.0f, 1.0f/65504.0f, 1.0f/65504.0f, 1.0f);
		case tcu::astc::BLOCK_TEST_TYPE_ENDPOINT_VALUE_HDR_15:		return Vec4(1.0f/65504.0f);
		default:													return Vec4(1.0f);
	}
}

Vec4 getBlockTestTypeColorBias (BlockTestType testType)
{
	switch (testType)
	{
		case tcu::astc::BLOCK_TEST_TYPE_VOID_EXTENT_HDR:	return Vec4(0.5f);
		default:											return Vec4(0.0f);
	}
}

} // astc
} // tcu
