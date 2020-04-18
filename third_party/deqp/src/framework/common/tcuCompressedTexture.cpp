/*-------------------------------------------------------------------------
 * drawElements Quality Program Tester Core
 * ----------------------------------------
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
 * \brief Compressed Texture Utilities.
 *//*--------------------------------------------------------------------*/

#include "tcuCompressedTexture.hpp"
#include "tcuTextureUtil.hpp"
#include "tcuAstcUtil.hpp"

#include "deStringUtil.hpp"
#include "deFloat16.h"

#include <algorithm>

namespace tcu
{

int getBlockSize (CompressedTexFormat format)
{
	if (isAstcFormat(format))
	{
		return astc::BLOCK_SIZE_BYTES;
	}
	else if (isEtcFormat(format))
	{
		switch (format)
		{
			case COMPRESSEDTEXFORMAT_ETC1_RGB8:							return 8;
			case COMPRESSEDTEXFORMAT_EAC_R11:							return 8;
			case COMPRESSEDTEXFORMAT_EAC_SIGNED_R11:					return 8;
			case COMPRESSEDTEXFORMAT_EAC_RG11:							return 16;
			case COMPRESSEDTEXFORMAT_EAC_SIGNED_RG11:					return 16;
			case COMPRESSEDTEXFORMAT_ETC2_RGB8:							return 8;
			case COMPRESSEDTEXFORMAT_ETC2_SRGB8:						return 8;
			case COMPRESSEDTEXFORMAT_ETC2_RGB8_PUNCHTHROUGH_ALPHA1:		return 8;
			case COMPRESSEDTEXFORMAT_ETC2_SRGB8_PUNCHTHROUGH_ALPHA1:	return 8;
			case COMPRESSEDTEXFORMAT_ETC2_EAC_RGBA8:					return 16;
			case COMPRESSEDTEXFORMAT_ETC2_EAC_SRGB8_ALPHA8:				return 16;

			default:
				DE_ASSERT(false);
				return -1;
		}
	}
	else
	{
		DE_ASSERT(false);
		return -1;
	}
}

IVec3 getBlockPixelSize (CompressedTexFormat format)
{
	if (isEtcFormat(format))
	{
		return IVec3(4, 4, 1);
	}
	else if (isAstcFormat(format))
	{
		switch (format)
		{
			case COMPRESSEDTEXFORMAT_ASTC_4x4_RGBA:				return IVec3(4,  4,  1);
			case COMPRESSEDTEXFORMAT_ASTC_5x4_RGBA:				return IVec3(5,  4,  1);
			case COMPRESSEDTEXFORMAT_ASTC_5x5_RGBA:				return IVec3(5,  5,  1);
			case COMPRESSEDTEXFORMAT_ASTC_6x5_RGBA:				return IVec3(6,  5,  1);
			case COMPRESSEDTEXFORMAT_ASTC_6x6_RGBA:				return IVec3(6,  6,  1);
			case COMPRESSEDTEXFORMAT_ASTC_8x5_RGBA:				return IVec3(8,  5,  1);
			case COMPRESSEDTEXFORMAT_ASTC_8x6_RGBA:				return IVec3(8,  6,  1);
			case COMPRESSEDTEXFORMAT_ASTC_8x8_RGBA:				return IVec3(8,  8,  1);
			case COMPRESSEDTEXFORMAT_ASTC_10x5_RGBA:			return IVec3(10, 5,  1);
			case COMPRESSEDTEXFORMAT_ASTC_10x6_RGBA:			return IVec3(10, 6,  1);
			case COMPRESSEDTEXFORMAT_ASTC_10x8_RGBA:			return IVec3(10, 8,  1);
			case COMPRESSEDTEXFORMAT_ASTC_10x10_RGBA:			return IVec3(10, 10, 1);
			case COMPRESSEDTEXFORMAT_ASTC_12x10_RGBA:			return IVec3(12, 10, 1);
			case COMPRESSEDTEXFORMAT_ASTC_12x12_RGBA:			return IVec3(12, 12, 1);
			case COMPRESSEDTEXFORMAT_ASTC_4x4_SRGB8_ALPHA8:		return IVec3(4,  4,  1);
			case COMPRESSEDTEXFORMAT_ASTC_5x4_SRGB8_ALPHA8:		return IVec3(5,  4,  1);
			case COMPRESSEDTEXFORMAT_ASTC_5x5_SRGB8_ALPHA8:		return IVec3(5,  5,  1);
			case COMPRESSEDTEXFORMAT_ASTC_6x5_SRGB8_ALPHA8:		return IVec3(6,  5,  1);
			case COMPRESSEDTEXFORMAT_ASTC_6x6_SRGB8_ALPHA8:		return IVec3(6,  6,  1);
			case COMPRESSEDTEXFORMAT_ASTC_8x5_SRGB8_ALPHA8:		return IVec3(8,  5,  1);
			case COMPRESSEDTEXFORMAT_ASTC_8x6_SRGB8_ALPHA8:		return IVec3(8,  6,  1);
			case COMPRESSEDTEXFORMAT_ASTC_8x8_SRGB8_ALPHA8:		return IVec3(8,  8,  1);
			case COMPRESSEDTEXFORMAT_ASTC_10x5_SRGB8_ALPHA8:	return IVec3(10, 5,  1);
			case COMPRESSEDTEXFORMAT_ASTC_10x6_SRGB8_ALPHA8:	return IVec3(10, 6,  1);
			case COMPRESSEDTEXFORMAT_ASTC_10x8_SRGB8_ALPHA8:	return IVec3(10, 8,  1);
			case COMPRESSEDTEXFORMAT_ASTC_10x10_SRGB8_ALPHA8:	return IVec3(10, 10, 1);
			case COMPRESSEDTEXFORMAT_ASTC_12x10_SRGB8_ALPHA8:	return IVec3(12, 10, 1);
			case COMPRESSEDTEXFORMAT_ASTC_12x12_SRGB8_ALPHA8:	return IVec3(12, 12, 1);

			default:
				DE_ASSERT(false);
				return IVec3();
		}
	}
	else
	{
		DE_ASSERT(false);
		return IVec3(-1);
	}
}

bool isEtcFormat (CompressedTexFormat format)
{
	switch (format)
	{
		case COMPRESSEDTEXFORMAT_ETC1_RGB8:
		case COMPRESSEDTEXFORMAT_EAC_R11:
		case COMPRESSEDTEXFORMAT_EAC_SIGNED_R11:
		case COMPRESSEDTEXFORMAT_EAC_RG11:
		case COMPRESSEDTEXFORMAT_EAC_SIGNED_RG11:
		case COMPRESSEDTEXFORMAT_ETC2_RGB8:
		case COMPRESSEDTEXFORMAT_ETC2_SRGB8:
		case COMPRESSEDTEXFORMAT_ETC2_RGB8_PUNCHTHROUGH_ALPHA1:
		case COMPRESSEDTEXFORMAT_ETC2_SRGB8_PUNCHTHROUGH_ALPHA1:
		case COMPRESSEDTEXFORMAT_ETC2_EAC_RGBA8:
		case COMPRESSEDTEXFORMAT_ETC2_EAC_SRGB8_ALPHA8:
			return true;

		default:
			return false;
	}
}

bool isAstcFormat (CompressedTexFormat format)
{
	switch (format)
	{
		case COMPRESSEDTEXFORMAT_ASTC_4x4_RGBA:
		case COMPRESSEDTEXFORMAT_ASTC_5x4_RGBA:
		case COMPRESSEDTEXFORMAT_ASTC_5x5_RGBA:
		case COMPRESSEDTEXFORMAT_ASTC_6x5_RGBA:
		case COMPRESSEDTEXFORMAT_ASTC_6x6_RGBA:
		case COMPRESSEDTEXFORMAT_ASTC_8x5_RGBA:
		case COMPRESSEDTEXFORMAT_ASTC_8x6_RGBA:
		case COMPRESSEDTEXFORMAT_ASTC_8x8_RGBA:
		case COMPRESSEDTEXFORMAT_ASTC_10x5_RGBA:
		case COMPRESSEDTEXFORMAT_ASTC_10x6_RGBA:
		case COMPRESSEDTEXFORMAT_ASTC_10x8_RGBA:
		case COMPRESSEDTEXFORMAT_ASTC_10x10_RGBA:
		case COMPRESSEDTEXFORMAT_ASTC_12x10_RGBA:
		case COMPRESSEDTEXFORMAT_ASTC_12x12_RGBA:
		case COMPRESSEDTEXFORMAT_ASTC_4x4_SRGB8_ALPHA8:
		case COMPRESSEDTEXFORMAT_ASTC_5x4_SRGB8_ALPHA8:
		case COMPRESSEDTEXFORMAT_ASTC_5x5_SRGB8_ALPHA8:
		case COMPRESSEDTEXFORMAT_ASTC_6x5_SRGB8_ALPHA8:
		case COMPRESSEDTEXFORMAT_ASTC_6x6_SRGB8_ALPHA8:
		case COMPRESSEDTEXFORMAT_ASTC_8x5_SRGB8_ALPHA8:
		case COMPRESSEDTEXFORMAT_ASTC_8x6_SRGB8_ALPHA8:
		case COMPRESSEDTEXFORMAT_ASTC_8x8_SRGB8_ALPHA8:
		case COMPRESSEDTEXFORMAT_ASTC_10x5_SRGB8_ALPHA8:
		case COMPRESSEDTEXFORMAT_ASTC_10x6_SRGB8_ALPHA8:
		case COMPRESSEDTEXFORMAT_ASTC_10x8_SRGB8_ALPHA8:
		case COMPRESSEDTEXFORMAT_ASTC_10x10_SRGB8_ALPHA8:
		case COMPRESSEDTEXFORMAT_ASTC_12x10_SRGB8_ALPHA8:
		case COMPRESSEDTEXFORMAT_ASTC_12x12_SRGB8_ALPHA8:
			return true;

		default:
			return false;
	}
}

bool isAstcSRGBFormat (CompressedTexFormat format)
{
	switch (format)
	{
		case COMPRESSEDTEXFORMAT_ASTC_4x4_SRGB8_ALPHA8:
		case COMPRESSEDTEXFORMAT_ASTC_5x4_SRGB8_ALPHA8:
		case COMPRESSEDTEXFORMAT_ASTC_5x5_SRGB8_ALPHA8:
		case COMPRESSEDTEXFORMAT_ASTC_6x5_SRGB8_ALPHA8:
		case COMPRESSEDTEXFORMAT_ASTC_6x6_SRGB8_ALPHA8:
		case COMPRESSEDTEXFORMAT_ASTC_8x5_SRGB8_ALPHA8:
		case COMPRESSEDTEXFORMAT_ASTC_8x6_SRGB8_ALPHA8:
		case COMPRESSEDTEXFORMAT_ASTC_8x8_SRGB8_ALPHA8:
		case COMPRESSEDTEXFORMAT_ASTC_10x5_SRGB8_ALPHA8:
		case COMPRESSEDTEXFORMAT_ASTC_10x6_SRGB8_ALPHA8:
		case COMPRESSEDTEXFORMAT_ASTC_10x8_SRGB8_ALPHA8:
		case COMPRESSEDTEXFORMAT_ASTC_10x10_SRGB8_ALPHA8:
		case COMPRESSEDTEXFORMAT_ASTC_12x10_SRGB8_ALPHA8:
		case COMPRESSEDTEXFORMAT_ASTC_12x12_SRGB8_ALPHA8:
			return true;

		default:
			return false;
	}
}

TextureFormat getUncompressedFormat (CompressedTexFormat format)
{
	if (isEtcFormat(format))
	{
		switch (format)
		{
			case COMPRESSEDTEXFORMAT_ETC1_RGB8:							return TextureFormat(TextureFormat::RGB,	TextureFormat::UNORM_INT8);
			case COMPRESSEDTEXFORMAT_EAC_R11:							return TextureFormat(TextureFormat::R,		TextureFormat::UNORM_INT16);
			case COMPRESSEDTEXFORMAT_EAC_SIGNED_R11:					return TextureFormat(TextureFormat::R,		TextureFormat::SNORM_INT16);
			case COMPRESSEDTEXFORMAT_EAC_RG11:							return TextureFormat(TextureFormat::RG,		TextureFormat::UNORM_INT16);
			case COMPRESSEDTEXFORMAT_EAC_SIGNED_RG11:					return TextureFormat(TextureFormat::RG,		TextureFormat::SNORM_INT16);
			case COMPRESSEDTEXFORMAT_ETC2_RGB8:							return TextureFormat(TextureFormat::RGB,	TextureFormat::UNORM_INT8);
			case COMPRESSEDTEXFORMAT_ETC2_SRGB8:						return TextureFormat(TextureFormat::sRGB,	TextureFormat::UNORM_INT8);
			case COMPRESSEDTEXFORMAT_ETC2_RGB8_PUNCHTHROUGH_ALPHA1:		return TextureFormat(TextureFormat::RGBA,	TextureFormat::UNORM_INT8);
			case COMPRESSEDTEXFORMAT_ETC2_SRGB8_PUNCHTHROUGH_ALPHA1:	return TextureFormat(TextureFormat::sRGBA,	TextureFormat::UNORM_INT8);
			case COMPRESSEDTEXFORMAT_ETC2_EAC_RGBA8:					return TextureFormat(TextureFormat::RGBA,	TextureFormat::UNORM_INT8);
			case COMPRESSEDTEXFORMAT_ETC2_EAC_SRGB8_ALPHA8:				return TextureFormat(TextureFormat::sRGBA,	TextureFormat::UNORM_INT8);

			default:
				DE_ASSERT(false);
				return TextureFormat();
		}
	}
	else if (isAstcFormat(format))
	{
		if (isAstcSRGBFormat(format))
			return TextureFormat(TextureFormat::sRGBA, TextureFormat::UNORM_INT8);
		else
			return TextureFormat(TextureFormat::RGBA, TextureFormat::HALF_FLOAT);
	}
	else
	{
		DE_ASSERT(false);
		return TextureFormat();
	}
}

CompressedTexFormat getAstcFormatByBlockSize (const IVec3& size, bool isSRGB)
{
	if (size.z() > 1)
		throw InternalError("3D ASTC textures not currently supported");

	for (int fmtI = 0; fmtI < COMPRESSEDTEXFORMAT_LAST; fmtI++)
	{
		const CompressedTexFormat fmt = (CompressedTexFormat)fmtI;

		if (isAstcFormat(fmt) && getBlockPixelSize(fmt) == size && isAstcSRGBFormat(fmt) == isSRGB)
			return fmt;
	}

	throw InternalError("Invalid ASTC block size " + de::toString(size.x()) + "x" + de::toString(size.y()) + "x" + de::toString(size.z()));
}

namespace
{

// \todo [2013-08-06 nuutti] ETC and ASTC decompression codes are rather unrelated, and are already in their own "private" namespaces - should this be split to multiple files?

namespace EtcDecompressInternal
{

enum
{
	ETC2_BLOCK_WIDTH					= 4,
	ETC2_BLOCK_HEIGHT					= 4,
	ETC2_UNCOMPRESSED_PIXEL_SIZE_A8		= 1,
	ETC2_UNCOMPRESSED_PIXEL_SIZE_R11	= 2,
	ETC2_UNCOMPRESSED_PIXEL_SIZE_RG11	= 4,
	ETC2_UNCOMPRESSED_PIXEL_SIZE_RGB8	= 3,
	ETC2_UNCOMPRESSED_PIXEL_SIZE_RGBA8	= 4,
	ETC2_UNCOMPRESSED_BLOCK_SIZE_A8		= ETC2_BLOCK_WIDTH*ETC2_BLOCK_HEIGHT*ETC2_UNCOMPRESSED_PIXEL_SIZE_A8,
	ETC2_UNCOMPRESSED_BLOCK_SIZE_R11	= ETC2_BLOCK_WIDTH*ETC2_BLOCK_HEIGHT*ETC2_UNCOMPRESSED_PIXEL_SIZE_R11,
	ETC2_UNCOMPRESSED_BLOCK_SIZE_RG11	= ETC2_BLOCK_WIDTH*ETC2_BLOCK_HEIGHT*ETC2_UNCOMPRESSED_PIXEL_SIZE_RG11,
	ETC2_UNCOMPRESSED_BLOCK_SIZE_RGB8	= ETC2_BLOCK_WIDTH*ETC2_BLOCK_HEIGHT*ETC2_UNCOMPRESSED_PIXEL_SIZE_RGB8,
	ETC2_UNCOMPRESSED_BLOCK_SIZE_RGBA8	= ETC2_BLOCK_WIDTH*ETC2_BLOCK_HEIGHT*ETC2_UNCOMPRESSED_PIXEL_SIZE_RGBA8
};

inline deUint64 get64BitBlock (const deUint8* src, int blockNdx)
{
	// Stored in big-endian form.
	deUint64 block = 0;

	for (int i = 0; i < 8; i++)
		block = (block << 8ull) | (deUint64)(src[blockNdx*8+i]);

	return block;
}

// Return the first 64 bits of a 128 bit block.
inline deUint64 get128BitBlockStart (const deUint8* src, int blockNdx)
{
	return get64BitBlock(src, 2*blockNdx);
}

// Return the last 64 bits of a 128 bit block.
inline deUint64 get128BitBlockEnd (const deUint8* src, int blockNdx)
{
	return get64BitBlock(src, 2*blockNdx + 1);
}

inline deUint32 getBit (deUint64 src, int bit)
{
	return (src >> bit) & 1;
}

inline deUint32 getBits (deUint64 src, int low, int high)
{
	const int numBits = (high-low) + 1;
	DE_ASSERT(de::inRange(numBits, 1, 32));
	if (numBits < 32)
		return (deUint32)((src >> low) & ((1u<<numBits)-1));
	else
		return (deUint32)((src >> low) & 0xFFFFFFFFu);
}

inline deUint8 extend4To8 (deUint8 src)
{
	DE_ASSERT((src & ~((1<<4)-1)) == 0);
	return (deUint8)((src << 4) | src);
}

inline deUint8 extend5To8 (deUint8 src)
{
	DE_ASSERT((src & ~((1<<5)-1)) == 0);
	return (deUint8)((src << 3) | (src >> 2));
}

inline deUint8 extend6To8 (deUint8 src)
{
	DE_ASSERT((src & ~((1<<6)-1)) == 0);
	return (deUint8)((src << 2) | (src >> 4));
}

inline deUint8 extend7To8 (deUint8 src)
{
	DE_ASSERT((src & ~((1<<7)-1)) == 0);
	return (deUint8)((src << 1) | (src >> 6));
}

inline deInt8 extendSigned3To8 (deUint8 src)
{
	const bool isNeg = (src & (1<<2)) != 0;
	return (deInt8)((isNeg ? ~((1<<3)-1) : 0) | src);
}

inline deUint8 extend5Delta3To8 (deUint8 base5, deUint8 delta3)
{
	const deUint8 t = (deUint8)((deInt8)base5 + extendSigned3To8(delta3));
	return extend5To8(t);
}

inline deUint16 extend11To16 (deUint16 src)
{
	DE_ASSERT((src & ~((1<<11)-1)) == 0);
	return (deUint16)((src << 5) | (src >> 6));
}

inline deInt16 extend11To16WithSign (deInt16 src)
{
	if (src < 0)
		return (deInt16)(-(deInt16)extend11To16((deUint16)(-src)));
	else
		return (deInt16)extend11To16(src);
}

void decompressETC1Block (deUint8 dst[ETC2_UNCOMPRESSED_BLOCK_SIZE_RGB8], deUint64 src)
{
	const int		diffBit		= (int)getBit(src, 33);
	const int		flipBit		= (int)getBit(src, 32);
	const deUint32	table[2]	= { getBits(src, 37, 39), getBits(src, 34, 36) };
	deUint8			baseR[2];
	deUint8			baseG[2];
	deUint8			baseB[2];

	if (diffBit == 0)
	{
		// Individual mode.
		baseR[0] = extend4To8((deUint8)getBits(src, 60, 63));
		baseR[1] = extend4To8((deUint8)getBits(src, 56, 59));
		baseG[0] = extend4To8((deUint8)getBits(src, 52, 55));
		baseG[1] = extend4To8((deUint8)getBits(src, 48, 51));
		baseB[0] = extend4To8((deUint8)getBits(src, 44, 47));
		baseB[1] = extend4To8((deUint8)getBits(src, 40, 43));
	}
	else
	{
		// Differential mode (diffBit == 1).
		deUint8 bR = (deUint8)getBits(src, 59, 63); // 5b
		deUint8 dR = (deUint8)getBits(src, 56, 58); // 3b
		deUint8 bG = (deUint8)getBits(src, 51, 55);
		deUint8 dG = (deUint8)getBits(src, 48, 50);
		deUint8 bB = (deUint8)getBits(src, 43, 47);
		deUint8 dB = (deUint8)getBits(src, 40, 42);

		baseR[0] = extend5To8(bR);
		baseG[0] = extend5To8(bG);
		baseB[0] = extend5To8(bB);

		baseR[1] = extend5Delta3To8(bR, dR);
		baseG[1] = extend5Delta3To8(bG, dG);
		baseB[1] = extend5Delta3To8(bB, dB);
	}

	static const int modifierTable[8][4] =
	{
	//	  00   01   10    11
		{  2,   8,  -2,   -8 },
		{  5,  17,  -5,  -17 },
		{  9,  29,  -9,  -29 },
		{ 13,  42, -13,  -42 },
		{ 18,  60, -18,  -60 },
		{ 24,  80, -24,  -80 },
		{ 33, 106, -33, -106 },
		{ 47, 183, -47, -183 }
	};

	// Write final pixels.
	for (int pixelNdx = 0; pixelNdx < ETC2_BLOCK_HEIGHT*ETC2_BLOCK_WIDTH; pixelNdx++)
	{
		const int		x				= pixelNdx / ETC2_BLOCK_HEIGHT;
		const int		y				= pixelNdx % ETC2_BLOCK_HEIGHT;
		const int		dstOffset		= (y*ETC2_BLOCK_WIDTH + x)*ETC2_UNCOMPRESSED_PIXEL_SIZE_RGB8;
		const int		subBlock		= ((flipBit ? y : x) >= 2) ? 1 : 0;
		const deUint32	tableNdx		= table[subBlock];
		const deUint32	modifierNdx		= (getBit(src, 16+pixelNdx) << 1) | getBit(src, pixelNdx);
		const int		modifier		= modifierTable[tableNdx][modifierNdx];

		dst[dstOffset+0] = (deUint8)deClamp32((int)baseR[subBlock] + modifier, 0, 255);
		dst[dstOffset+1] = (deUint8)deClamp32((int)baseG[subBlock] + modifier, 0, 255);
		dst[dstOffset+2] = (deUint8)deClamp32((int)baseB[subBlock] + modifier, 0, 255);
	}
}

// if alphaMode is true, do PUNCHTHROUGH and store alpha to alphaDst; otherwise do ordinary ETC2 RGB8.
void decompressETC2Block (deUint8 dst[ETC2_UNCOMPRESSED_BLOCK_SIZE_RGB8], deUint64 src, deUint8 alphaDst[ETC2_UNCOMPRESSED_BLOCK_SIZE_A8], bool alphaMode)
{
	enum Etc2Mode
	{
		MODE_INDIVIDUAL = 0,
		MODE_DIFFERENTIAL,
		MODE_T,
		MODE_H,
		MODE_PLANAR,

		MODE_LAST
	};

	const int		diffOpaqueBit	= (int)getBit(src, 33);
	const deInt8	selBR			= (deInt8)getBits(src, 59, 63);	// 5 bits.
	const deInt8	selBG			= (deInt8)getBits(src, 51, 55);
	const deInt8	selBB			= (deInt8)getBits(src, 43, 47);
	const deInt8	selDR			= extendSigned3To8((deUint8)getBits(src, 56, 58)); // 3 bits.
	const deInt8	selDG			= extendSigned3To8((deUint8)getBits(src, 48, 50));
	const deInt8	selDB			= extendSigned3To8((deUint8)getBits(src, 40, 42));
	Etc2Mode		mode;

	if (!alphaMode && diffOpaqueBit == 0)
		mode = MODE_INDIVIDUAL;
	else if (!de::inRange(selBR + selDR, 0, 31))
		mode = MODE_T;
	else if (!de::inRange(selBG + selDG, 0, 31))
		mode = MODE_H;
	else if (!de::inRange(selBB + selDB, 0, 31))
		mode = MODE_PLANAR;
	else
		mode = MODE_DIFFERENTIAL;

	if (mode == MODE_INDIVIDUAL || mode == MODE_DIFFERENTIAL)
	{
		// Individual and differential modes have some steps in common, handle them here.
		static const int modifierTable[8][4] =
		{
		//	  00   01   10    11
			{  2,   8,  -2,   -8 },
			{  5,  17,  -5,  -17 },
			{  9,  29,  -9,  -29 },
			{ 13,  42, -13,  -42 },
			{ 18,  60, -18,  -60 },
			{ 24,  80, -24,  -80 },
			{ 33, 106, -33, -106 },
			{ 47, 183, -47, -183 }
		};

		const int		flipBit		= (int)getBit(src, 32);
		const deUint32	table[2]	= { getBits(src, 37, 39), getBits(src, 34, 36) };
		deUint8			baseR[2];
		deUint8			baseG[2];
		deUint8			baseB[2];

		if (mode == MODE_INDIVIDUAL)
		{
			// Individual mode, initial values.
			baseR[0] = extend4To8((deUint8)getBits(src, 60, 63));
			baseR[1] = extend4To8((deUint8)getBits(src, 56, 59));
			baseG[0] = extend4To8((deUint8)getBits(src, 52, 55));
			baseG[1] = extend4To8((deUint8)getBits(src, 48, 51));
			baseB[0] = extend4To8((deUint8)getBits(src, 44, 47));
			baseB[1] = extend4To8((deUint8)getBits(src, 40, 43));
		}
		else
		{
			// Differential mode, initial values.
			baseR[0] = extend5To8(selBR);
			baseG[0] = extend5To8(selBG);
			baseB[0] = extend5To8(selBB);

			baseR[1] = extend5To8((deUint8)(selBR + selDR));
			baseG[1] = extend5To8((deUint8)(selBG + selDG));
			baseB[1] = extend5To8((deUint8)(selBB + selDB));
		}

		// Write final pixels for individual or differential mode.
		for (int pixelNdx = 0; pixelNdx < ETC2_BLOCK_HEIGHT*ETC2_BLOCK_WIDTH; pixelNdx++)
		{
			const int		x				= pixelNdx / ETC2_BLOCK_HEIGHT;
			const int		y				= pixelNdx % ETC2_BLOCK_HEIGHT;
			const int		dstOffset		= (y*ETC2_BLOCK_WIDTH + x)*ETC2_UNCOMPRESSED_PIXEL_SIZE_RGB8;
			const int		subBlock		= ((flipBit ? y : x) >= 2) ? 1 : 0;
			const deUint32	tableNdx		= table[subBlock];
			const deUint32	modifierNdx		= (getBit(src, 16+pixelNdx) << 1) | getBit(src, pixelNdx);
			const int		alphaDstOffset	= (y*ETC2_BLOCK_WIDTH + x)*ETC2_UNCOMPRESSED_PIXEL_SIZE_A8; // Only needed for PUNCHTHROUGH version.

			// If doing PUNCHTHROUGH version (alphaMode), opaque bit may affect colors.
			if (alphaMode && diffOpaqueBit == 0 && modifierNdx == 2)
			{
				dst[dstOffset+0]			= 0;
				dst[dstOffset+1]			= 0;
				dst[dstOffset+2]			= 0;
				alphaDst[alphaDstOffset]	= 0;
			}
			else
			{
				int modifier;

				// PUNCHTHROUGH version and opaque bit may also affect modifiers.
				if (alphaMode && diffOpaqueBit == 0 && (modifierNdx == 0 || modifierNdx == 2))
					modifier = 0;
				else
					modifier = modifierTable[tableNdx][modifierNdx];

				dst[dstOffset+0] = (deUint8)deClamp32((int)baseR[subBlock] + modifier, 0, 255);
				dst[dstOffset+1] = (deUint8)deClamp32((int)baseG[subBlock] + modifier, 0, 255);
				dst[dstOffset+2] = (deUint8)deClamp32((int)baseB[subBlock] + modifier, 0, 255);

				if (alphaMode)
					alphaDst[alphaDstOffset] = 255;
			}
		}
	}
	else if (mode == MODE_T || mode == MODE_H)
	{
		// T and H modes have some steps in common, handle them here.
		static const int distTable[8] = { 3, 6, 11, 16, 23, 32, 41, 64 };

		deUint8 paintR[4];
		deUint8 paintG[4];
		deUint8 paintB[4];

		if (mode == MODE_T)
		{
			// T mode, calculate paint values.
			const deUint8	R1a			= (deUint8)getBits(src, 59, 60);
			const deUint8	R1b			= (deUint8)getBits(src, 56, 57);
			const deUint8	G1			= (deUint8)getBits(src, 52, 55);
			const deUint8	B1			= (deUint8)getBits(src, 48, 51);
			const deUint8	R2			= (deUint8)getBits(src, 44, 47);
			const deUint8	G2			= (deUint8)getBits(src, 40, 43);
			const deUint8	B2			= (deUint8)getBits(src, 36, 39);
			const deUint32	distNdx		= (getBits(src, 34, 35) << 1) | getBit(src, 32);
			const int		dist		= distTable[distNdx];

			paintR[0] = extend4To8((deUint8)((R1a << 2) | R1b));
			paintG[0] = extend4To8(G1);
			paintB[0] = extend4To8(B1);
			paintR[2] = extend4To8(R2);
			paintG[2] = extend4To8(G2);
			paintB[2] = extend4To8(B2);
			paintR[1] = (deUint8)deClamp32((int)paintR[2] + dist, 0, 255);
			paintG[1] = (deUint8)deClamp32((int)paintG[2] + dist, 0, 255);
			paintB[1] = (deUint8)deClamp32((int)paintB[2] + dist, 0, 255);
			paintR[3] = (deUint8)deClamp32((int)paintR[2] - dist, 0, 255);
			paintG[3] = (deUint8)deClamp32((int)paintG[2] - dist, 0, 255);
			paintB[3] = (deUint8)deClamp32((int)paintB[2] - dist, 0, 255);
		}
		else
		{
			// H mode, calculate paint values.
			const deUint8	R1		= (deUint8)getBits(src, 59, 62);
			const deUint8	G1a		= (deUint8)getBits(src, 56, 58);
			const deUint8	G1b		= (deUint8)getBit(src, 52);
			const deUint8	B1a		= (deUint8)getBit(src, 51);
			const deUint8	B1b		= (deUint8)getBits(src, 47, 49);
			const deUint8	R2		= (deUint8)getBits(src, 43, 46);
			const deUint8	G2		= (deUint8)getBits(src, 39, 42);
			const deUint8	B2		= (deUint8)getBits(src, 35, 38);
			deUint8			baseR[2];
			deUint8			baseG[2];
			deUint8			baseB[2];
			deUint32		baseValue[2];
			deUint32		distNdx;
			int				dist;

			baseR[0]		= extend4To8(R1);
			baseG[0]		= extend4To8((deUint8)((G1a << 1) | G1b));
			baseB[0]		= extend4To8((deUint8)((B1a << 3) | B1b));
			baseR[1]		= extend4To8(R2);
			baseG[1]		= extend4To8(G2);
			baseB[1]		= extend4To8(B2);
			baseValue[0]	= (((deUint32)baseR[0]) << 16) | (((deUint32)baseG[0]) << 8) | baseB[0];
			baseValue[1]	= (((deUint32)baseR[1]) << 16) | (((deUint32)baseG[1]) << 8) | baseB[1];
			distNdx			= (getBit(src, 34) << 2) | (getBit(src, 32) << 1) | (deUint32)(baseValue[0] >= baseValue[1]);
			dist			= distTable[distNdx];

			paintR[0]		= (deUint8)deClamp32((int)baseR[0] + dist, 0, 255);
			paintG[0]		= (deUint8)deClamp32((int)baseG[0] + dist, 0, 255);
			paintB[0]		= (deUint8)deClamp32((int)baseB[0] + dist, 0, 255);
			paintR[1]		= (deUint8)deClamp32((int)baseR[0] - dist, 0, 255);
			paintG[1]		= (deUint8)deClamp32((int)baseG[0] - dist, 0, 255);
			paintB[1]		= (deUint8)deClamp32((int)baseB[0] - dist, 0, 255);
			paintR[2]		= (deUint8)deClamp32((int)baseR[1] + dist, 0, 255);
			paintG[2]		= (deUint8)deClamp32((int)baseG[1] + dist, 0, 255);
			paintB[2]		= (deUint8)deClamp32((int)baseB[1] + dist, 0, 255);
			paintR[3]		= (deUint8)deClamp32((int)baseR[1] - dist, 0, 255);
			paintG[3]		= (deUint8)deClamp32((int)baseG[1] - dist, 0, 255);
			paintB[3]		= (deUint8)deClamp32((int)baseB[1] - dist, 0, 255);
		}

		// Write final pixels for T or H mode.
		for (int pixelNdx = 0; pixelNdx < ETC2_BLOCK_HEIGHT*ETC2_BLOCK_WIDTH; pixelNdx++)
		{
			const int		x				= pixelNdx / ETC2_BLOCK_HEIGHT;
			const int		y				= pixelNdx % ETC2_BLOCK_HEIGHT;
			const int		dstOffset		= (y*ETC2_BLOCK_WIDTH + x)*ETC2_UNCOMPRESSED_PIXEL_SIZE_RGB8;
			const deUint32	paintNdx		= (getBit(src, 16+pixelNdx) << 1) | getBit(src, pixelNdx);
			const int		alphaDstOffset	= (y*ETC2_BLOCK_WIDTH + x)*ETC2_UNCOMPRESSED_PIXEL_SIZE_A8; // Only needed for PUNCHTHROUGH version.

			if (alphaMode && diffOpaqueBit == 0 && paintNdx == 2)
			{
				dst[dstOffset+0]			= 0;
				dst[dstOffset+1]			= 0;
				dst[dstOffset+2]			= 0;
				alphaDst[alphaDstOffset]	= 0;
			}
			else
			{
				dst[dstOffset+0] = (deUint8)deClamp32((int)paintR[paintNdx], 0, 255);
				dst[dstOffset+1] = (deUint8)deClamp32((int)paintG[paintNdx], 0, 255);
				dst[dstOffset+2] = (deUint8)deClamp32((int)paintB[paintNdx], 0, 255);

				if (alphaMode)
					alphaDst[alphaDstOffset] = 255;
			}
		}
	}
	else
	{
		// Planar mode.
		const deUint8 GO1	= (deUint8)getBit(src, 56);
		const deUint8 GO2	= (deUint8)getBits(src, 49, 54);
		const deUint8 BO1	= (deUint8)getBit(src, 48);
		const deUint8 BO2	= (deUint8)getBits(src, 43, 44);
		const deUint8 BO3	= (deUint8)getBits(src, 39, 41);
		const deUint8 RH1	= (deUint8)getBits(src, 34, 38);
		const deUint8 RH2	= (deUint8)getBit(src, 32);
		const deUint8 RO	= extend6To8((deUint8)getBits(src, 57, 62));
		const deUint8 GO	= extend7To8((deUint8)((GO1 << 6) | GO2));
		const deUint8 BO	= extend6To8((deUint8)((BO1 << 5) | (BO2 << 3) | BO3));
		const deUint8 RH	= extend6To8((deUint8)((RH1 << 1) | RH2));
		const deUint8 GH	= extend7To8((deUint8)getBits(src, 25, 31));
		const deUint8 BH	= extend6To8((deUint8)getBits(src, 19, 24));
		const deUint8 RV	= extend6To8((deUint8)getBits(src, 13, 18));
		const deUint8 GV	= extend7To8((deUint8)getBits(src, 6, 12));
		const deUint8 BV	= extend6To8((deUint8)getBits(src, 0, 5));

		// Write final pixels for planar mode.
		for (int y = 0; y < 4; y++)
		{
			for (int x = 0; x < 4; x++)
			{
				const int dstOffset			= (y*ETC2_BLOCK_WIDTH + x)*ETC2_UNCOMPRESSED_PIXEL_SIZE_RGB8;
				const int unclampedR		= (x * ((int)RH-(int)RO) + y * ((int)RV-(int)RO) + 4*(int)RO + 2) >> 2;
				const int unclampedG		= (x * ((int)GH-(int)GO) + y * ((int)GV-(int)GO) + 4*(int)GO + 2) >> 2;
				const int unclampedB		= (x * ((int)BH-(int)BO) + y * ((int)BV-(int)BO) + 4*(int)BO + 2) >> 2;
				const int alphaDstOffset	= (y*ETC2_BLOCK_WIDTH + x)*ETC2_UNCOMPRESSED_PIXEL_SIZE_A8; // Only needed for PUNCHTHROUGH version.

				dst[dstOffset+0] = (deUint8)deClamp32(unclampedR, 0, 255);
				dst[dstOffset+1] = (deUint8)deClamp32(unclampedG, 0, 255);
				dst[dstOffset+2] = (deUint8)deClamp32(unclampedB, 0, 255);

				if (alphaMode)
					alphaDst[alphaDstOffset] = 255;
			}
		}
	}
}

void decompressEAC8Block (deUint8 dst[ETC2_UNCOMPRESSED_BLOCK_SIZE_A8], deUint64 src)
{
	static const int modifierTable[16][8] =
	{
		{-3,  -6,  -9, -15,  2,  5,  8, 14},
		{-3,  -7, -10, -13,  2,  6,  9, 12},
		{-2,  -5,  -8, -13,  1,  4,  7, 12},
		{-2,  -4,  -6, -13,  1,  3,  5, 12},
		{-3,  -6,  -8, -12,  2,  5,  7, 11},
		{-3,  -7,  -9, -11,  2,  6,  8, 10},
		{-4,  -7,  -8, -11,  3,  6,  7, 10},
		{-3,  -5,  -8, -11,  2,  4,  7, 10},
		{-2,  -6,  -8, -10,  1,  5,  7,  9},
		{-2,  -5,  -8, -10,  1,  4,  7,  9},
		{-2,  -4,  -8, -10,  1,  3,  7,  9},
		{-2,  -5,  -7, -10,  1,  4,  6,  9},
		{-3,  -4,  -7, -10,  2,  3,  6,  9},
		{-1,  -2,  -3, -10,  0,  1,  2,  9},
		{-4,  -6,  -8,  -9,  3,  5,  7,  8},
		{-3,  -5,  -7,  -9,  2,  4,  6,  8}
	};

	const deUint8	baseCodeword	= (deUint8)getBits(src, 56, 63);
	const deUint8	multiplier		= (deUint8)getBits(src, 52, 55);
	const deUint32	tableNdx		= getBits(src, 48, 51);

	for (int pixelNdx = 0; pixelNdx < ETC2_BLOCK_HEIGHT*ETC2_BLOCK_WIDTH; pixelNdx++)
	{
		const int		x				= pixelNdx / ETC2_BLOCK_HEIGHT;
		const int		y				= pixelNdx % ETC2_BLOCK_HEIGHT;
		const int		dstOffset		= (y*ETC2_BLOCK_WIDTH + x)*ETC2_UNCOMPRESSED_PIXEL_SIZE_A8;
		const int		pixelBitNdx		= 45 - 3*pixelNdx;
		const deUint32	modifierNdx		= (getBit(src, pixelBitNdx + 2) << 2) | (getBit(src, pixelBitNdx + 1) << 1) | getBit(src, pixelBitNdx);
		const int		modifier		= modifierTable[tableNdx][modifierNdx];

		dst[dstOffset] = (deUint8)deClamp32((int)baseCodeword + (int)multiplier*modifier, 0, 255);
	}
}

void decompressEAC11Block (deUint8 dst[ETC2_UNCOMPRESSED_BLOCK_SIZE_R11], deUint64 src, bool signedMode)
{
	static const int modifierTable[16][8] =
	{
		{-3,  -6,  -9, -15,  2,  5,  8, 14},
		{-3,  -7, -10, -13,  2,  6,  9, 12},
		{-2,  -5,  -8, -13,  1,  4,  7, 12},
		{-2,  -4,  -6, -13,  1,  3,  5, 12},
		{-3,  -6,  -8, -12,  2,  5,  7, 11},
		{-3,  -7,  -9, -11,  2,  6,  8, 10},
		{-4,  -7,  -8, -11,  3,  6,  7, 10},
		{-3,  -5,  -8, -11,  2,  4,  7, 10},
		{-2,  -6,  -8, -10,  1,  5,  7,  9},
		{-2,  -5,  -8, -10,  1,  4,  7,  9},
		{-2,  -4,  -8, -10,  1,  3,  7,  9},
		{-2,  -5,  -7, -10,  1,  4,  6,  9},
		{-3,  -4,  -7, -10,  2,  3,  6,  9},
		{-1,  -2,  -3, -10,  0,  1,  2,  9},
		{-4,  -6,  -8,  -9,  3,  5,  7,  8},
		{-3,  -5,  -7,  -9,  2,  4,  6,  8}
	};

	const deInt32 multiplier	= (deInt32)getBits(src, 52, 55);
	const deInt32 tableNdx		= (deInt32)getBits(src, 48, 51);
	deInt32 baseCodeword		= (deInt32)getBits(src, 56, 63);

	if (signedMode)
	{
		if (baseCodeword > 127)
			baseCodeword -= 256;
		if (baseCodeword == -128)
			baseCodeword = -127;
	}

	for (int pixelNdx = 0; pixelNdx < ETC2_BLOCK_HEIGHT*ETC2_BLOCK_WIDTH; pixelNdx++)
	{
		const int		x				= pixelNdx / ETC2_BLOCK_HEIGHT;
		const int		y				= pixelNdx % ETC2_BLOCK_HEIGHT;
		const int		dstOffset		= (y*ETC2_BLOCK_WIDTH + x)*ETC2_UNCOMPRESSED_PIXEL_SIZE_R11;
		const int		pixelBitNdx		= 45 - 3*pixelNdx;
		const deUint32	modifierNdx		= (getBit(src, pixelBitNdx + 2) << 2) | (getBit(src, pixelBitNdx + 1) << 1) | getBit(src, pixelBitNdx);
		const int		modifier		= modifierTable[tableNdx][modifierNdx];

		if (signedMode)
		{
			deInt16 value;

			if (multiplier != 0)
				value = (deInt16)deClamp32(baseCodeword*8 + multiplier*modifier*8, -1023, 1023);
			else
				value = (deInt16)deClamp32(baseCodeword*8 + modifier, -1023, 1023);

			*((deInt16*)(dst + dstOffset)) = value;
		}
		else
		{
			deUint16 value;

			if (multiplier != 0)
				value = (deUint16)deClamp32(baseCodeword*8 + 4 + multiplier*modifier*8, 0, 2047);
			else
				value= (deUint16)deClamp32(baseCodeword*8 + 4 + modifier, 0, 2047);

			*((deUint16*)(dst + dstOffset)) = value;
		}
	}
}

} // EtcDecompressInternal

void decompressETC1 (const PixelBufferAccess& dst, const deUint8* src)
{
	using namespace EtcDecompressInternal;

	deUint8* const	dstPtr			= (deUint8*)dst.getDataPtr();
	const deUint64	compressedBlock = get64BitBlock(src, 0);

	decompressETC1Block(dstPtr, compressedBlock);
}

void decompressETC2 (const PixelBufferAccess& dst, const deUint8* src)
{
	using namespace EtcDecompressInternal;

	deUint8* const	dstPtr			= (deUint8*)dst.getDataPtr();
	const deUint64	compressedBlock = get64BitBlock(src, 0);

	decompressETC2Block(dstPtr, compressedBlock, NULL, false);
}

void decompressETC2_EAC_RGBA8 (const PixelBufferAccess& dst, const deUint8* src)
{
	using namespace EtcDecompressInternal;

	deUint8* const	dstPtr			= (deUint8*)dst.getDataPtr();
	const int		dstRowPitch		= dst.getRowPitch();
	const int		dstPixelSize	= ETC2_UNCOMPRESSED_PIXEL_SIZE_RGBA8;

	const deUint64	compressedBlockAlpha	= get128BitBlockStart(src, 0);
	const deUint64	compressedBlockRGB		= get128BitBlockEnd(src, 0);
	deUint8			uncompressedBlockAlpha[ETC2_UNCOMPRESSED_BLOCK_SIZE_A8];
	deUint8			uncompressedBlockRGB[ETC2_UNCOMPRESSED_BLOCK_SIZE_RGB8];

	// Decompress.
	decompressETC2Block(uncompressedBlockRGB, compressedBlockRGB, NULL, false);
	decompressEAC8Block(uncompressedBlockAlpha, compressedBlockAlpha);

	// Write to dst.
	for (int y = 0; y < (int)ETC2_BLOCK_HEIGHT; y++)
	{
		for (int x = 0; x < (int)ETC2_BLOCK_WIDTH; x++)
		{
			const deUint8* const	srcPixelRGB		= &uncompressedBlockRGB[(y*ETC2_BLOCK_WIDTH + x)*ETC2_UNCOMPRESSED_PIXEL_SIZE_RGB8];
			const deUint8* const	srcPixelAlpha	= &uncompressedBlockAlpha[(y*ETC2_BLOCK_WIDTH + x)*ETC2_UNCOMPRESSED_PIXEL_SIZE_A8];
			deUint8* const			dstPixel		= dstPtr + y*dstRowPitch + x*dstPixelSize;

			DE_STATIC_ASSERT(ETC2_UNCOMPRESSED_PIXEL_SIZE_RGBA8 == 4);
			dstPixel[0] = srcPixelRGB[0];
			dstPixel[1] = srcPixelRGB[1];
			dstPixel[2] = srcPixelRGB[2];
			dstPixel[3] = srcPixelAlpha[0];
		}
	}
}

void decompressETC2_RGB8_PUNCHTHROUGH_ALPHA1 (const PixelBufferAccess& dst, const deUint8* src)
{
	using namespace EtcDecompressInternal;

	deUint8* const	dstPtr			= (deUint8*)dst.getDataPtr();
	const int		dstRowPitch		= dst.getRowPitch();
	const int		dstPixelSize	= ETC2_UNCOMPRESSED_PIXEL_SIZE_RGBA8;

	const deUint64	compressedBlockRGBA	= get64BitBlock(src, 0);
	deUint8			uncompressedBlockRGB[ETC2_UNCOMPRESSED_BLOCK_SIZE_RGB8];
	deUint8			uncompressedBlockAlpha[ETC2_UNCOMPRESSED_BLOCK_SIZE_A8];

	// Decompress.
	decompressETC2Block(uncompressedBlockRGB, compressedBlockRGBA, uncompressedBlockAlpha, DE_TRUE);

	// Write to dst.
	for (int y = 0; y < (int)ETC2_BLOCK_HEIGHT; y++)
	{
		for (int x = 0; x < (int)ETC2_BLOCK_WIDTH; x++)
		{
			const deUint8* const	srcPixel		= &uncompressedBlockRGB[(y*ETC2_BLOCK_WIDTH + x)*ETC2_UNCOMPRESSED_PIXEL_SIZE_RGB8];
			const deUint8* const	srcPixelAlpha	= &uncompressedBlockAlpha[(y*ETC2_BLOCK_WIDTH + x)*ETC2_UNCOMPRESSED_PIXEL_SIZE_A8];
			deUint8* const			dstPixel		= dstPtr + y*dstRowPitch + x*dstPixelSize;

			DE_STATIC_ASSERT(ETC2_UNCOMPRESSED_PIXEL_SIZE_RGBA8 == 4);
			dstPixel[0] = srcPixel[0];
			dstPixel[1] = srcPixel[1];
			dstPixel[2] = srcPixel[2];
			dstPixel[3] = srcPixelAlpha[0];
		}
	}
}

void decompressEAC_R11 (const PixelBufferAccess& dst, const deUint8* src, bool signedMode)
{
	using namespace EtcDecompressInternal;

	deUint8* const	dstPtr			= (deUint8*)dst.getDataPtr();
	const int		dstRowPitch		= dst.getRowPitch();
	const int		dstPixelSize	= ETC2_UNCOMPRESSED_PIXEL_SIZE_R11;

	const deUint64	compressedBlock = get64BitBlock(src, 0);
	deUint8			uncompressedBlock[ETC2_UNCOMPRESSED_BLOCK_SIZE_R11];

	// Decompress.
	decompressEAC11Block(uncompressedBlock, compressedBlock, signedMode);

	// Write to dst.
	for (int y = 0; y < (int)ETC2_BLOCK_HEIGHT; y++)
	{
		for (int x = 0; x < (int)ETC2_BLOCK_WIDTH; x++)
		{
			DE_STATIC_ASSERT(ETC2_UNCOMPRESSED_PIXEL_SIZE_R11 == 2);

			if (signedMode)
			{
				const deInt16* const	srcPixel = (deInt16*)&uncompressedBlock[(y*ETC2_BLOCK_WIDTH + x)*ETC2_UNCOMPRESSED_PIXEL_SIZE_R11];
				deInt16* const			dstPixel = (deInt16*)(dstPtr + y*dstRowPitch + x*dstPixelSize);

				dstPixel[0] = extend11To16WithSign(srcPixel[0]);
			}
			else
			{
				const deUint16* const	srcPixel = (deUint16*)&uncompressedBlock[(y*ETC2_BLOCK_WIDTH + x)*ETC2_UNCOMPRESSED_PIXEL_SIZE_R11];
				deUint16* const			dstPixel = (deUint16*)(dstPtr + y*dstRowPitch + x*dstPixelSize);

				dstPixel[0] = extend11To16(srcPixel[0]);
			}
		}
	}
}

void decompressEAC_RG11 (const PixelBufferAccess& dst, const deUint8* src, bool signedMode)
{
	using namespace EtcDecompressInternal;

	deUint8* const	dstPtr			= (deUint8*)dst.getDataPtr();
	const int		dstRowPitch		= dst.getRowPitch();
	const int		dstPixelSize	= ETC2_UNCOMPRESSED_PIXEL_SIZE_RG11;

	const deUint64	compressedBlockR = get128BitBlockStart(src, 0);
	const deUint64	compressedBlockG = get128BitBlockEnd(src, 0);
	deUint8			uncompressedBlockR[ETC2_UNCOMPRESSED_BLOCK_SIZE_R11];
	deUint8			uncompressedBlockG[ETC2_UNCOMPRESSED_BLOCK_SIZE_R11];

	// Decompress.
	decompressEAC11Block(uncompressedBlockR, compressedBlockR, signedMode);
	decompressEAC11Block(uncompressedBlockG, compressedBlockG, signedMode);

	// Write to dst.
	for (int y = 0; y < (int)ETC2_BLOCK_HEIGHT; y++)
	{
		for (int x = 0; x < (int)ETC2_BLOCK_WIDTH; x++)
		{
			DE_STATIC_ASSERT(ETC2_UNCOMPRESSED_PIXEL_SIZE_RG11 == 4);

			if (signedMode)
			{
				const deInt16* const	srcPixelR	= (deInt16*)&uncompressedBlockR[(y*ETC2_BLOCK_WIDTH + x)*ETC2_UNCOMPRESSED_PIXEL_SIZE_R11];
				const deInt16* const	srcPixelG	= (deInt16*)&uncompressedBlockG[(y*ETC2_BLOCK_WIDTH + x)*ETC2_UNCOMPRESSED_PIXEL_SIZE_R11];
				deInt16* const			dstPixel	= (deInt16*)(dstPtr + y*dstRowPitch + x*dstPixelSize);

				dstPixel[0] = extend11To16WithSign(srcPixelR[0]);
				dstPixel[1] = extend11To16WithSign(srcPixelG[0]);
			}
			else
			{
				const deUint16* const	srcPixelR	= (deUint16*)&uncompressedBlockR[(y*ETC2_BLOCK_WIDTH + x)*ETC2_UNCOMPRESSED_PIXEL_SIZE_R11];
				const deUint16* const	srcPixelG	= (deUint16*)&uncompressedBlockG[(y*ETC2_BLOCK_WIDTH + x)*ETC2_UNCOMPRESSED_PIXEL_SIZE_R11];
				deUint16* const			dstPixel	= (deUint16*)(dstPtr + y*dstRowPitch + x*dstPixelSize);

				dstPixel[0] = extend11To16(srcPixelR[0]);
				dstPixel[1] = extend11To16(srcPixelG[0]);
			}
		}
	}
}

void decompressBlock (CompressedTexFormat format, const PixelBufferAccess& dst, const deUint8* src, const TexDecompressionParams& params)
{
	// No 3D blocks supported right now
	DE_ASSERT(dst.getDepth() == 1);

	switch (format)
	{
		case COMPRESSEDTEXFORMAT_ETC1_RGB8:							decompressETC1							(dst, src);			break;
		case COMPRESSEDTEXFORMAT_EAC_R11:							decompressEAC_R11						(dst, src, false);	break;
		case COMPRESSEDTEXFORMAT_EAC_SIGNED_R11:					decompressEAC_R11						(dst, src, true);	break;
		case COMPRESSEDTEXFORMAT_EAC_RG11:							decompressEAC_RG11						(dst, src, false);	break;
		case COMPRESSEDTEXFORMAT_EAC_SIGNED_RG11:					decompressEAC_RG11						(dst, src, true);	break;
		case COMPRESSEDTEXFORMAT_ETC2_RGB8:							decompressETC2							(dst, src);			break;
		case COMPRESSEDTEXFORMAT_ETC2_SRGB8:						decompressETC2							(dst, src);			break;
		case COMPRESSEDTEXFORMAT_ETC2_RGB8_PUNCHTHROUGH_ALPHA1:		decompressETC2_RGB8_PUNCHTHROUGH_ALPHA1	(dst, src);			break;
		case COMPRESSEDTEXFORMAT_ETC2_SRGB8_PUNCHTHROUGH_ALPHA1:	decompressETC2_RGB8_PUNCHTHROUGH_ALPHA1	(dst, src);			break;
		case COMPRESSEDTEXFORMAT_ETC2_EAC_RGBA8:					decompressETC2_EAC_RGBA8				(dst, src);			break;
		case COMPRESSEDTEXFORMAT_ETC2_EAC_SRGB8_ALPHA8:				decompressETC2_EAC_RGBA8				(dst, src);			break;

		case COMPRESSEDTEXFORMAT_ASTC_4x4_RGBA:
		case COMPRESSEDTEXFORMAT_ASTC_5x4_RGBA:
		case COMPRESSEDTEXFORMAT_ASTC_5x5_RGBA:
		case COMPRESSEDTEXFORMAT_ASTC_6x5_RGBA:
		case COMPRESSEDTEXFORMAT_ASTC_6x6_RGBA:
		case COMPRESSEDTEXFORMAT_ASTC_8x5_RGBA:
		case COMPRESSEDTEXFORMAT_ASTC_8x6_RGBA:
		case COMPRESSEDTEXFORMAT_ASTC_8x8_RGBA:
		case COMPRESSEDTEXFORMAT_ASTC_10x5_RGBA:
		case COMPRESSEDTEXFORMAT_ASTC_10x6_RGBA:
		case COMPRESSEDTEXFORMAT_ASTC_10x8_RGBA:
		case COMPRESSEDTEXFORMAT_ASTC_10x10_RGBA:
		case COMPRESSEDTEXFORMAT_ASTC_12x10_RGBA:
		case COMPRESSEDTEXFORMAT_ASTC_12x12_RGBA:
		case COMPRESSEDTEXFORMAT_ASTC_4x4_SRGB8_ALPHA8:
		case COMPRESSEDTEXFORMAT_ASTC_5x4_SRGB8_ALPHA8:
		case COMPRESSEDTEXFORMAT_ASTC_5x5_SRGB8_ALPHA8:
		case COMPRESSEDTEXFORMAT_ASTC_6x5_SRGB8_ALPHA8:
		case COMPRESSEDTEXFORMAT_ASTC_6x6_SRGB8_ALPHA8:
		case COMPRESSEDTEXFORMAT_ASTC_8x5_SRGB8_ALPHA8:
		case COMPRESSEDTEXFORMAT_ASTC_8x6_SRGB8_ALPHA8:
		case COMPRESSEDTEXFORMAT_ASTC_8x8_SRGB8_ALPHA8:
		case COMPRESSEDTEXFORMAT_ASTC_10x5_SRGB8_ALPHA8:
		case COMPRESSEDTEXFORMAT_ASTC_10x6_SRGB8_ALPHA8:
		case COMPRESSEDTEXFORMAT_ASTC_10x8_SRGB8_ALPHA8:
		case COMPRESSEDTEXFORMAT_ASTC_10x10_SRGB8_ALPHA8:
		case COMPRESSEDTEXFORMAT_ASTC_12x10_SRGB8_ALPHA8:
		case COMPRESSEDTEXFORMAT_ASTC_12x12_SRGB8_ALPHA8:
			astc::decompress(dst, src, format, params.astcMode);
			break;

		default:
			DE_ASSERT(false);
			break;
	}
}

int componentSum (const IVec3& vec)
{
	return vec.x() + vec.y() + vec.z();
}

} // anonymous

void decompress (const PixelBufferAccess& dst, CompressedTexFormat fmt, const deUint8* src, const TexDecompressionParams& params)
{
	const int				blockSize			= getBlockSize(fmt);
	const IVec3				blockPixelSize		(getBlockPixelSize(fmt));
	const IVec3				blockCount			(deDivRoundUp32(dst.getWidth(),		blockPixelSize.x()),
												 deDivRoundUp32(dst.getHeight(),	blockPixelSize.y()),
												 deDivRoundUp32(dst.getDepth(),		blockPixelSize.z()));
	const IVec3				blockPitches		(blockSize, blockSize * blockCount.x(), blockSize * blockCount.x() * blockCount.y());

	std::vector<deUint8>	uncompressedBlock	(dst.getFormat().getPixelSize() * blockPixelSize.x() * blockPixelSize.y() * blockPixelSize.z());
	const PixelBufferAccess	blockAccess			(getUncompressedFormat(fmt), blockPixelSize.x(), blockPixelSize.y(), blockPixelSize.z(), &uncompressedBlock[0]);

	DE_ASSERT(dst.getFormat() == getUncompressedFormat(fmt));

	for (int blockZ = 0; blockZ < blockCount.z(); blockZ++)
	for (int blockY = 0; blockY < blockCount.y(); blockY++)
	for (int blockX = 0; blockX < blockCount.x(); blockX++)
	{
		const IVec3				blockPos	(blockX, blockY, blockZ);
		const deUint8* const	blockPtr	= src + componentSum(blockPos * blockPitches);
		const IVec3				copySize	(de::min(blockPixelSize.x(), dst.getWidth()		- blockPos.x() * blockPixelSize.x()),
											 de::min(blockPixelSize.y(), dst.getHeight()	- blockPos.y() * blockPixelSize.y()),
											 de::min(blockPixelSize.z(), dst.getDepth()		- blockPos.z() * blockPixelSize.z()));
		const IVec3				dstPixelPos	= blockPos * blockPixelSize;

		decompressBlock(fmt, blockAccess, blockPtr, params);

		copy(getSubregion(dst, dstPixelPos.x(), dstPixelPos.y(), dstPixelPos.z(), copySize.x(), copySize.y(), copySize.z()), getSubregion(blockAccess, 0, 0, 0, copySize.x(), copySize.y(), copySize.z()));
	}
}

CompressedTexture::CompressedTexture (void)
	: m_format	(COMPRESSEDTEXFORMAT_LAST)
	, m_width	(0)
	, m_height	(0)
	, m_depth	(0)
{
}

CompressedTexture::CompressedTexture (CompressedTexFormat format, int width, int height, int depth)
	: m_format	(COMPRESSEDTEXFORMAT_LAST)
	, m_width	(0)
	, m_height	(0)
	, m_depth	(0)
{
	setStorage(format, width, height, depth);
}

CompressedTexture::~CompressedTexture (void)
{
}

void CompressedTexture::setStorage (CompressedTexFormat format, int width, int height, int depth)
{
	m_format	= format;
	m_width		= width;
	m_height	= height;
	m_depth		= depth;

	if (m_format != COMPRESSEDTEXFORMAT_LAST)
	{
		const IVec3	blockPixelSize	= getBlockPixelSize(m_format);
		const int	blockSize		= getBlockSize(m_format);

		m_data.resize(deDivRoundUp32(m_width, blockPixelSize.x()) * deDivRoundUp32(m_height, blockPixelSize.y()) * deDivRoundUp32(m_depth, blockPixelSize.z()) * blockSize);
	}
	else
	{
		DE_ASSERT(m_format == COMPRESSEDTEXFORMAT_LAST);
		DE_ASSERT(m_width == 0 && m_height == 0 && m_depth == 0);
		m_data.resize(0);
	}
}

/*--------------------------------------------------------------------*//*!
 * \brief Decode to uncompressed pixel data
 * \param dst Destination buffer
 *//*--------------------------------------------------------------------*/
void CompressedTexture::decompress (const PixelBufferAccess& dst, const TexDecompressionParams& params) const
{
	DE_ASSERT(dst.getWidth() == m_width && dst.getHeight() == m_height && dst.getDepth() == m_depth);
	DE_ASSERT(dst.getFormat() == getUncompressedFormat(m_format));

	tcu::decompress(dst, m_format, &m_data[0], params);
}

} // tcu
