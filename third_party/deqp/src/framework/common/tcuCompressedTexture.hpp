#ifndef _TCUCOMPRESSEDTEXTURE_HPP
#define _TCUCOMPRESSEDTEXTURE_HPP
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

#include "tcuDefs.hpp"
#include "tcuTexture.hpp"

#include <vector>

namespace tcu
{

enum CompressedTexFormat
{
	COMPRESSEDTEXFORMAT_ETC1_RGB8 = 0,
	COMPRESSEDTEXFORMAT_EAC_R11,
	COMPRESSEDTEXFORMAT_EAC_SIGNED_R11,
	COMPRESSEDTEXFORMAT_EAC_RG11,
	COMPRESSEDTEXFORMAT_EAC_SIGNED_RG11,
	COMPRESSEDTEXFORMAT_ETC2_RGB8,
	COMPRESSEDTEXFORMAT_ETC2_SRGB8,
	COMPRESSEDTEXFORMAT_ETC2_RGB8_PUNCHTHROUGH_ALPHA1,
	COMPRESSEDTEXFORMAT_ETC2_SRGB8_PUNCHTHROUGH_ALPHA1,
	COMPRESSEDTEXFORMAT_ETC2_EAC_RGBA8,
	COMPRESSEDTEXFORMAT_ETC2_EAC_SRGB8_ALPHA8,

	COMPRESSEDTEXFORMAT_ASTC_4x4_RGBA,
	COMPRESSEDTEXFORMAT_ASTC_5x4_RGBA,
	COMPRESSEDTEXFORMAT_ASTC_5x5_RGBA,
	COMPRESSEDTEXFORMAT_ASTC_6x5_RGBA,
	COMPRESSEDTEXFORMAT_ASTC_6x6_RGBA,
	COMPRESSEDTEXFORMAT_ASTC_8x5_RGBA,
	COMPRESSEDTEXFORMAT_ASTC_8x6_RGBA,
	COMPRESSEDTEXFORMAT_ASTC_8x8_RGBA,
	COMPRESSEDTEXFORMAT_ASTC_10x5_RGBA,
	COMPRESSEDTEXFORMAT_ASTC_10x6_RGBA,
	COMPRESSEDTEXFORMAT_ASTC_10x8_RGBA,
	COMPRESSEDTEXFORMAT_ASTC_10x10_RGBA,
	COMPRESSEDTEXFORMAT_ASTC_12x10_RGBA,
	COMPRESSEDTEXFORMAT_ASTC_12x12_RGBA,
	COMPRESSEDTEXFORMAT_ASTC_4x4_SRGB8_ALPHA8,
	COMPRESSEDTEXFORMAT_ASTC_5x4_SRGB8_ALPHA8,
	COMPRESSEDTEXFORMAT_ASTC_5x5_SRGB8_ALPHA8,
	COMPRESSEDTEXFORMAT_ASTC_6x5_SRGB8_ALPHA8,
	COMPRESSEDTEXFORMAT_ASTC_6x6_SRGB8_ALPHA8,
	COMPRESSEDTEXFORMAT_ASTC_8x5_SRGB8_ALPHA8,
	COMPRESSEDTEXFORMAT_ASTC_8x6_SRGB8_ALPHA8,
	COMPRESSEDTEXFORMAT_ASTC_8x8_SRGB8_ALPHA8,
	COMPRESSEDTEXFORMAT_ASTC_10x5_SRGB8_ALPHA8,
	COMPRESSEDTEXFORMAT_ASTC_10x6_SRGB8_ALPHA8,
	COMPRESSEDTEXFORMAT_ASTC_10x8_SRGB8_ALPHA8,
	COMPRESSEDTEXFORMAT_ASTC_10x10_SRGB8_ALPHA8,
	COMPRESSEDTEXFORMAT_ASTC_12x10_SRGB8_ALPHA8,
	COMPRESSEDTEXFORMAT_ASTC_12x12_SRGB8_ALPHA8,

	COMPRESSEDTEXFORMAT_LAST
};

int					getBlockSize				(CompressedTexFormat format);
IVec3				getBlockPixelSize			(CompressedTexFormat format);

bool				isEtcFormat					(CompressedTexFormat format);
bool				isAstcFormat				(CompressedTexFormat format);
bool				isAstcSRGBFormat			(CompressedTexFormat format);

TextureFormat		getUncompressedFormat		(CompressedTexFormat format);
CompressedTexFormat getAstcFormatByBlockSize	(const IVec3& size, bool isSRGB);

struct TexDecompressionParams
{
	enum AstcMode
	{
		ASTCMODE_LDR = 0,
		ASTCMODE_HDR,
		ASTCMODE_LAST
	};

	TexDecompressionParams (AstcMode astcMode_ = ASTCMODE_LAST) : astcMode(astcMode_) {}

	AstcMode astcMode;
};

/*--------------------------------------------------------------------*//*!
 * \brief Compressed texture
 *
 * This class implements container for common compressed texture formats.
 * Reference decoding to uncompressed formats is supported.
 *//*--------------------------------------------------------------------*/
class CompressedTexture
{
public:

							CompressedTexture			(CompressedTexFormat format, int width, int height, int depth = 1);
							CompressedTexture			(void);
							~CompressedTexture			(void);

	void					setStorage					(CompressedTexFormat format, int width, int height, int depth = 1);

	int						getWidth					(void) const	{ return m_width;				}
	int						getHeight					(void) const	{ return m_height;				}
	int						getDepth					(void) const	{ return m_depth;				}
	CompressedTexFormat		getFormat					(void) const	{ return m_format;				}
	int						getDataSize					(void) const	{ return (int)m_data.size();	}
	const void*				getData						(void) const	{ return &m_data[0];			}
	void*					getData						(void)			{ return &m_data[0];			}

	void					decompress					(const PixelBufferAccess& dst, const TexDecompressionParams& params = TexDecompressionParams()) const;

private:
	CompressedTexFormat		m_format;
	int						m_width;
	int						m_height;
	int						m_depth;
	std::vector<deUint8>	m_data;
} DE_WARN_UNUSED_TYPE;

void decompress (const PixelBufferAccess& dst, CompressedTexFormat fmt, const deUint8* src, const TexDecompressionParams& params = TexDecompressionParams());

} // tcu

#endif // _TCUCOMPRESSEDTEXTURE_HPP
