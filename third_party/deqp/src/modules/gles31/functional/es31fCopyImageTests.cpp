/*-------------------------------------------------------------------------
 * drawElements Quality Program OpenGL ES 3.1 Module
 * -------------------------------------------------
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
 * \brief Copy image tests for GL_EXT_copy_image.
 *//*--------------------------------------------------------------------*/

#include "es31fCopyImageTests.hpp"

#include "tes31TestCase.hpp"

#include "glsTextureTestUtil.hpp"

#include "gluContextInfo.hpp"
#include "gluObjectWrapper.hpp"
#include "gluRenderContext.hpp"
#include "gluStrUtil.hpp"
#include "gluTextureUtil.hpp"
#include "gluPixelTransfer.hpp"

#include "glwEnums.hpp"
#include "glwFunctions.hpp"

#include "tcuCompressedTexture.hpp"
#include "tcuFloat.hpp"
#include "tcuImageCompare.hpp"
#include "tcuTestLog.hpp"
#include "tcuTexture.hpp"
#include "tcuTextureUtil.hpp"
#include "tcuVector.hpp"
#include "tcuVectorUtil.hpp"
#include "tcuSeedBuilder.hpp"
#include "tcuResultCollector.hpp"

#include "deArrayBuffer.hpp"
#include "deFloat16.h"
#include "deRandom.hpp"
#include "deStringUtil.hpp"
#include "deUniquePtr.hpp"
#include "deArrayUtil.hpp"

#include <map>
#include <string>
#include <vector>

using namespace deqp::gls::TextureTestUtil;
using namespace glu::TextureTestUtil;

using tcu::Float;
using tcu::IVec2;
using tcu::IVec3;
using tcu::IVec4;
using tcu::Sampler;
using tcu::ScopedLogSection;
using tcu::TestLog;
using tcu::Vec4;
using tcu::SeedBuilder;

using de::ArrayBuffer;

using std::map;
using std::string;
using std::vector;
using std::pair;

namespace deqp
{
namespace gles31
{
namespace Functional
{
namespace
{

enum ViewClass
{
	VIEWCLASS_128_BITS = 0,
	VIEWCLASS_96_BITS,
	VIEWCLASS_64_BITS,
	VIEWCLASS_48_BITS,
	VIEWCLASS_32_BITS,
	VIEWCLASS_24_BITS,
	VIEWCLASS_16_BITS,
	VIEWCLASS_8_BITS,

	VIEWCLASS_EAC_R11,
	VIEWCLASS_EAC_RG11,
	VIEWCLASS_ETC2_RGB,
	VIEWCLASS_ETC2_RGBA,
	VIEWCLASS_ETC2_EAC_RGBA,
	VIEWCLASS_ASTC_4x4_RGBA,
	VIEWCLASS_ASTC_5x4_RGBA,
	VIEWCLASS_ASTC_5x5_RGBA,
	VIEWCLASS_ASTC_6x5_RGBA,
	VIEWCLASS_ASTC_6x6_RGBA,
	VIEWCLASS_ASTC_8x5_RGBA,
	VIEWCLASS_ASTC_8x6_RGBA,
	VIEWCLASS_ASTC_8x8_RGBA,
	VIEWCLASS_ASTC_10x5_RGBA,
	VIEWCLASS_ASTC_10x6_RGBA,
	VIEWCLASS_ASTC_10x8_RGBA,
	VIEWCLASS_ASTC_10x10_RGBA,
	VIEWCLASS_ASTC_12x10_RGBA,
	VIEWCLASS_ASTC_12x12_RGBA
};

enum Verify
{
	VERIFY_NONE = 0,
	VERIFY_COMPARE_REFERENCE
};

const char* viewClassToName (ViewClass viewClass)
{
	switch (viewClass)
	{
		case VIEWCLASS_128_BITS:			return "viewclass_128_bits";
		case VIEWCLASS_96_BITS:				return "viewclass_96_bits";
		case VIEWCLASS_64_BITS:				return "viewclass_64_bits";
		case VIEWCLASS_48_BITS:				return "viewclass_48_bits";
		case VIEWCLASS_32_BITS:				return "viewclass_32_bits";
		case VIEWCLASS_24_BITS:				return "viewclass_24_bits";
		case VIEWCLASS_16_BITS:				return "viewclass_16_bits";
		case VIEWCLASS_8_BITS:				return "viewclass_8_bits";
		case VIEWCLASS_EAC_R11:				return "viewclass_eac_r11";
		case VIEWCLASS_EAC_RG11:			return "viewclass_eac_rg11";
		case VIEWCLASS_ETC2_RGB:			return "viewclass_etc2_rgb";
		case VIEWCLASS_ETC2_RGBA:			return "viewclass_etc2_rgba";
		case VIEWCLASS_ETC2_EAC_RGBA:		return "viewclass_etc2_eac_rgba";
		case VIEWCLASS_ASTC_4x4_RGBA:		return "viewclass_astc_4x4_rgba";
		case VIEWCLASS_ASTC_5x4_RGBA:		return "viewclass_astc_5x4_rgba";
		case VIEWCLASS_ASTC_5x5_RGBA:		return "viewclass_astc_5x5_rgba";
		case VIEWCLASS_ASTC_6x5_RGBA:		return "viewclass_astc_6x5_rgba";
		case VIEWCLASS_ASTC_6x6_RGBA:		return "viewclass_astc_6x6_rgba";
		case VIEWCLASS_ASTC_8x5_RGBA:		return "viewclass_astc_8x5_rgba";
		case VIEWCLASS_ASTC_8x6_RGBA:		return "viewclass_astc_8x6_rgba";
		case VIEWCLASS_ASTC_8x8_RGBA:		return "viewclass_astc_8x8_rgba";
		case VIEWCLASS_ASTC_10x5_RGBA:		return "viewclass_astc_10x5_rgba";
		case VIEWCLASS_ASTC_10x6_RGBA:		return "viewclass_astc_10x6_rgba";
		case VIEWCLASS_ASTC_10x8_RGBA:		return "viewclass_astc_10x8_rgba";
		case VIEWCLASS_ASTC_10x10_RGBA:		return "viewclass_astc_10x10_rgba";
		case VIEWCLASS_ASTC_12x10_RGBA:		return "viewclass_astc_12x10_rgba";
		case VIEWCLASS_ASTC_12x12_RGBA:		return "viewclass_astc_12x12_rgba";

		default:
			DE_ASSERT(false);
			return NULL;
	}
}

const char* targetToName (deUint32 target)
{
	switch (target)
	{
		case GL_RENDERBUFFER:		return "renderbuffer";
		case GL_TEXTURE_2D:			return "texture2d";
		case GL_TEXTURE_3D:			return "texture3d";
		case GL_TEXTURE_2D_ARRAY:	return "texture2d_array";
		case GL_TEXTURE_CUBE_MAP:	return "cubemap";

		default:
			DE_ASSERT(false);
			return NULL;
	}
}

string formatToName (deUint32 format)
{
	string enumName;

	if (glu::isCompressedFormat(format))
		enumName = glu::getCompressedTextureFormatStr(format).toString().substr(14); // Strip GL_COMPRESSED_
	else
		enumName = glu::getUncompressedTextureFormatStr(format).toString().substr(3); // Strip GL_

	return de::toLower(enumName);
}

bool isFloatFormat (deUint32 format)
{
	if (glu::isCompressedFormat(format))
		return false;
	else
		return tcu::getTextureChannelClass(glu::mapGLInternalFormat(format).type) == tcu::TEXTURECHANNELCLASS_FLOATING_POINT;
}

bool isUintFormat (deUint32 format)
{
	if (glu::isCompressedFormat(format))
		return false;
	else
		return tcu::getTextureChannelClass(glu::mapGLInternalFormat(format).type) == tcu::TEXTURECHANNELCLASS_UNSIGNED_INTEGER;
}

bool isIntFormat (deUint32 format)
{
	if (glu::isCompressedFormat(format))
		return false;
	else
		return tcu::getTextureChannelClass(glu::mapGLInternalFormat(format).type) == tcu::TEXTURECHANNELCLASS_SIGNED_INTEGER;
}

bool isFixedPointFormat (deUint32 format)
{
	if (glu::isCompressedFormat(format))
		return false;
	else
	{
		const tcu::TextureChannelClass channelClass = tcu::getTextureChannelClass(glu::mapGLInternalFormat(format).type);

		return channelClass == tcu::TEXTURECHANNELCLASS_SIGNED_FIXED_POINT || channelClass == tcu::TEXTURECHANNELCLASS_UNSIGNED_FIXED_POINT;
	}
}

bool isTextureTarget (deUint32 target)
{
	return target != GL_RENDERBUFFER;
}

int getTargetTexDims (deUint32 target)
{
	DE_ASSERT(isTextureTarget(target));

	switch (target)
	{
		case GL_TEXTURE_1D:
			return 1;

		case GL_TEXTURE_1D_ARRAY:
		case GL_TEXTURE_2D:
		case GL_TEXTURE_CUBE_MAP:
			return 2;

		case GL_TEXTURE_2D_ARRAY:
		case GL_TEXTURE_3D:
			return 3;

		default:
			DE_ASSERT(false);
			return -1;
	}
}

class RandomizedRenderGrid
{
public:
					RandomizedRenderGrid	(const IVec2& targetSize, const IVec2& cellSize, int maxCellCount, deUint32 seed);
	bool			nextCell				(void);
	IVec2			getOrigin				(void) const;

	const IVec2&	getCellSize				(void) const { return m_cellSize; };
	IVec4			getUsedAreaBoundingBox	(void) const;
	int				getCellCount			(void) const { return m_cellCount; };

private:
	static IVec2	getRandomOffset			(deUint32 seed, IVec2 targetSize, IVec2 cellSize, IVec2 grid, int cellCount);

	const IVec2		m_targetSize;
	const IVec2		m_cellSize;
	const IVec2		m_grid;
	int				m_currentCell;
	const int		m_cellCount;
	const IVec2		m_baseRandomOffset;
};

RandomizedRenderGrid::RandomizedRenderGrid (const IVec2& targetSize, const IVec2& cellSize, int maxCellCount, deUint32 seed)
	: m_targetSize			(targetSize)
	, m_cellSize			(cellSize)
	, m_grid				(targetSize / cellSize)
	, m_currentCell			(0)
	// If the grid exactly fits height, take one row for randomization.
	, m_cellCount			(deMin32(maxCellCount, ((targetSize.y() % cellSize.y()) == 0) && m_grid.y() > 1 ? m_grid.x() * (m_grid.y() - 1) :  m_grid.x() * m_grid.y()))
	, m_baseRandomOffset	(getRandomOffset(seed, targetSize, cellSize, m_grid, m_cellCount))
{
}

IVec2 RandomizedRenderGrid::getRandomOffset (deUint32 seed, IVec2 targetSize, IVec2 cellSize, IVec2 grid, int cellCount)
{
	de::Random	rng			(seed);
	IVec2		result;
	IVec2		extraSpace = targetSize - (cellSize * grid);

	// If there'll be unused rows, donate them into extra space.
	// (Round the required rows to full cell row to find out how many rows are unused, multiply by size)
	DE_ASSERT(deDivRoundUp32(cellCount, grid.x()) <= grid.y());
	extraSpace.y() += (grid.y() - deDivRoundUp32(cellCount, grid.x())) * cellSize.y();

	DE_ASSERT(targetSize.x() > cellSize.x() && targetSize.y() > cellSize.y());
	// If grid fits perfectly just one row of cells, just give up on randomizing.
	DE_ASSERT(extraSpace.x() > 0 || extraSpace.y() > 0 || grid.y() == 1);
	DE_ASSERT(extraSpace.x() + grid.x() * cellSize.x() == targetSize.x());

	// \note Putting these as ctor params would make evaluation order undefined, I think <sigh>. Hence,
	// no direct return.
	result.x() = rng.getInt(0, extraSpace.x());
	result.y() = rng.getInt(0, extraSpace.y());
	return result;
}

bool RandomizedRenderGrid::nextCell (void)
{
	if (m_currentCell >= getCellCount())
		return false;

	m_currentCell++;
	return true;
}

IVec2 RandomizedRenderGrid::getOrigin (void) const
{
	const int	gridX		  = (m_currentCell - 1) % m_grid.x();
	const int	gridY		  = (m_currentCell - 1) / m_grid.x();
	const IVec2 currentOrigin = (IVec2(gridX, gridY) * m_cellSize) + m_baseRandomOffset;

	DE_ASSERT(currentOrigin.x() >= 0 && (currentOrigin.x() + m_cellSize.x()) <= m_targetSize.x());
	DE_ASSERT(currentOrigin.y() >= 0 && (currentOrigin.y() + m_cellSize.y()) <= m_targetSize.y());

	return currentOrigin;
}

IVec4 RandomizedRenderGrid::getUsedAreaBoundingBox (void) const
{
	const IVec2 lastCell	(de::min(m_currentCell + 1, m_grid.x()), ((m_currentCell + m_grid.x() - 1) / m_grid.x()));
	const IVec2 size		= lastCell * m_cellSize;

	return IVec4(m_baseRandomOffset.x(), m_baseRandomOffset.y(), size.x(), size.y());
}

class ImageInfo
{
public:
					ImageInfo		(deUint32 format, deUint32 target, const IVec3& size);

	deUint32		getFormat		(void) const { return m_format; }
	deUint32		getTarget		(void) const { return m_target; }
	const IVec3&	getSize			(void) const { return m_size; }

private:
	deUint32		m_format;
	deUint32		m_target;
	IVec3			m_size;
};

ImageInfo::ImageInfo (deUint32 format, deUint32 target, const IVec3& size)
	: m_format		(format)
	, m_target		(target)
	, m_size		(size)
{
	DE_ASSERT(m_target == GL_TEXTURE_2D_ARRAY || m_target == GL_TEXTURE_3D || m_size.z() == 1);
	DE_ASSERT(isTextureTarget(m_target) || !glu::isCompressedFormat(m_target));
}


SeedBuilder& operator<< (SeedBuilder& builder, const ImageInfo& info)
{
	builder << info.getFormat() << info.getTarget() << info.getSize();
	return builder;
}

const glu::ObjectTraits& getObjectTraits (const ImageInfo& info)
{
	if (isTextureTarget(info.getTarget()))
		return glu::objectTraits(glu::OBJECTTYPE_TEXTURE);
	else
		return glu::objectTraits(glu::OBJECTTYPE_RENDERBUFFER);
}

int getLevelCount (const ImageInfo& info)
{
	const deUint32	target	= info.getTarget();
	const IVec3		size	= info.getSize();

	if (target == GL_RENDERBUFFER)
		return 1;
	else if (target == GL_TEXTURE_2D_ARRAY)
	{
		const int maxSize = de::max(size.x(), size.y());

		return deLog2Ceil32(maxSize);
	}
	else
	{
		const int maxSize = de::max(size.x(), de::max(size.y(), size.z()));

		return deLog2Ceil32(maxSize);
	}
}

IVec3 getLevelSize (deUint32 target, const IVec3& baseSize, int level)
{
	IVec3 size;

	if (target != GL_TEXTURE_2D_ARRAY)
	{
		for (int i = 0; i < 3; i++)
			size[i] = de::max(baseSize[i] >> level, 1);
	}
	else
	{
		for (int i = 0; i < 2; i++)
			size[i] = de::max(baseSize[i] >> level, 1);

		size[2] = baseSize[2];
	}

	return size;
}

deUint32 mapFaceNdxToFace (int ndx)
{
	const deUint32 cubeFaces[] =
	{
		GL_TEXTURE_CUBE_MAP_POSITIVE_X,
		GL_TEXTURE_CUBE_MAP_NEGATIVE_X,

		GL_TEXTURE_CUBE_MAP_POSITIVE_Y,
		GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,

		GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
		GL_TEXTURE_CUBE_MAP_NEGATIVE_Z
	};

	return de::getSizedArrayElement<6>(cubeFaces, ndx);
}

// Class for iterating over mip levels and faces/slices/... of a texture.
class TextureImageIterator
{
public:
						TextureImageIterator	(const ImageInfo info, int levelCount);
						~TextureImageIterator	(void)							{}

	// Need to call next image once, newly constructed not readable, except for getSize
	bool				nextImage				(void);
	bool				hasNextImage			(void) const					{ return (m_currentLevel < (m_levelCount - 1)) || m_currentImage < (m_levelImageCount - 1); }

	int					getMipLevel				(void) const					{ return m_currentLevel; }
	int					getMipLevelCount		(void) const					{ return m_levelCount; }
	int					getCurrentImage			(void) const					{ return m_currentImage;}
	int					getLevelImageCount		(void) const					{ return m_levelImageCount; }
	IVec2				getSize					(void) const					{ return m_levelSize.toWidth<2>(); }	// Assume that image sizes never grow over iteration
	deUint32			getTarget				(void) const					{ return m_info.getTarget(); }

private:
	int					m_levelImageCount;		// Need to be defined in CTOR for the hasNextImage to work!
	const ImageInfo		m_info;
	int					m_currentLevel;
	IVec3				m_levelSize;
	int					m_currentImage;
	const int			m_levelCount;
};

TextureImageIterator::TextureImageIterator (const ImageInfo info, int levelCount)
	: m_levelImageCount	(info.getTarget() == GL_TEXTURE_CUBE_MAP ? 6 : getLevelSize(info.getTarget(), info.getSize(), 0).z())
	, m_info			(info)
	, m_currentLevel	(0)
	, m_levelSize		(getLevelSize(info.getTarget(), info.getSize(), 0))
	, m_currentImage	(-1)
	, m_levelCount		(levelCount)
{
	DE_ASSERT(m_levelCount <= getLevelCount(info));
}

bool TextureImageIterator::nextImage (void)
{
	if (!hasNextImage())
		return false;

	m_currentImage++;
	if (m_currentImage == m_levelImageCount)
	{
		m_currentLevel++;
		m_currentImage		= 0;

		m_levelSize			= getLevelSize(m_info.getTarget(), m_info.getSize(), m_currentLevel);

		if (getTarget() == GL_TEXTURE_CUBE_MAP)
			m_levelImageCount = 6;
		else
			m_levelImageCount = m_levelSize.z();
	}
	DE_ASSERT(m_currentLevel < m_levelCount);
	DE_ASSERT(m_currentImage < m_levelImageCount);
	return true;
}

// Get name
string getTextureImageName (int textureTarget, int mipLevel, int imageIndex)
{
	std::ostringstream result;
	result << "Level";
	result << mipLevel;
	switch (textureTarget)
	{
		case GL_TEXTURE_2D:			break;
		case GL_TEXTURE_3D:			result << "Slice" << imageIndex; break;
		case GL_TEXTURE_CUBE_MAP:	result << "Face" << imageIndex; break;
		case GL_TEXTURE_2D_ARRAY:	result << "Layer" << imageIndex; break;
		default:
			DE_FATAL("Unsupported texture target");
			break;
	}
	return result.str();
}

// Get description
string getTextureImageDescription (int textureTarget, int mipLevel, int imageIndex)
{
	std::ostringstream result;
	result << "level ";
	result << mipLevel;

	switch (textureTarget)
	{
		case GL_TEXTURE_2D:			break;
		case GL_TEXTURE_3D:			result << " and Slice " << imageIndex; break;
		case GL_TEXTURE_CUBE_MAP:	result << " and Face " << imageIndex; break;
		case GL_TEXTURE_2D_ARRAY:	result << " and Layer " << imageIndex; break;
		default:
			DE_FATAL("Unsupported texture target");
			break;
	}
	return result.str();
}

// Compute texture coordinates
void computeQuadTexCoords(vector<float>& texCoord, const TextureImageIterator& iteration)
{
	const int currentImage = iteration.getCurrentImage();
	switch (iteration.getTarget())
	{
		case GL_TEXTURE_2D:
			computeQuadTexCoord2D(texCoord, tcu::Vec2(0.0f, 0.0f), tcu::Vec2(1.0f, 1.0f));
			break;

		case GL_TEXTURE_3D:
		{
			const float r = (float(currentImage) + 0.5f) / (float)iteration.getLevelImageCount();
			computeQuadTexCoord3D(texCoord, tcu::Vec3(0.0f, 0.0f, r), tcu::Vec3(1.0f, 1.0f, r), tcu::IVec3(0, 1, 2));
			break;
		}

		case GL_TEXTURE_CUBE_MAP:
			computeQuadTexCoordCube(texCoord, glu::getCubeFaceFromGL(mapFaceNdxToFace(currentImage)));
			break;

		case GL_TEXTURE_2D_ARRAY:
			computeQuadTexCoord2DArray(texCoord, currentImage, tcu::Vec2(0.0f, 0.0f), tcu::Vec2(1.0f, 1.0f));
			break;

		default:
			DE_FATAL("Unsupported texture target");
	}
}

// Struct for storing each reference image with necessary metadata.
struct CellContents
{
	IVec2			origin;
	tcu::Surface	reference;
	std::string		name;
	std::string		description;
};

// Return format that has more restrictions on texel data.
deUint32 getMoreRestrictiveFormat (deUint32 formatA, deUint32 formatB)
{
	if (formatA == formatB)
		return formatA;
	else if (glu::isCompressedFormat(formatA) && isAstcFormat(glu::mapGLCompressedTexFormat(formatA)))
		return formatA;
	else if (glu::isCompressedFormat(formatB) && isAstcFormat(glu::mapGLCompressedTexFormat(formatB)))
		return formatB;
	else if (isFloatFormat(formatA))
	{
		DE_ASSERT(!isFloatFormat(formatB));

		return formatA;
	}
	else if (isFloatFormat(formatB))
	{
		DE_ASSERT(!isFloatFormat(formatA));

		return formatB;
	}
	else if (glu::isCompressedFormat(formatA))
	{
		return formatA;
	}
	else if (glu::isCompressedFormat(formatB))
	{
		return formatB;
	}
	else
		return formatA;
}

int getTexelBlockSize (deUint32 format)
{
	if (glu::isCompressedFormat(format))
		return tcu::getBlockSize(glu::mapGLCompressedTexFormat(format));
	else
		return glu::mapGLInternalFormat(format).getPixelSize();
}

IVec3 getTexelBlockPixelSize (deUint32 format)
{
	if (glu::isCompressedFormat(format))
		return tcu::getBlockPixelSize(glu::mapGLCompressedTexFormat(format));
	else
		return IVec3(1, 1, 1);
}

bool isColorRenderable (deUint32 format)
{
	switch (format)
	{
		case GL_R8:
		case GL_RG8:
		case GL_RGB8:
		case GL_RGB565:
		case GL_RGB4:
		case GL_RGB5_A1:
		case GL_RGBA8:
		case GL_RGB10_A2:
		case GL_RGB10_A2UI:
		case GL_SRGB8_ALPHA8:
		case GL_R8I:
		case GL_R8UI:
		case GL_R16I:
		case GL_R16UI:
		case GL_R32I:
		case GL_R32UI:
		case GL_RG8I:
		case GL_RG8UI:
		case GL_RG16I:
		case GL_RG16UI:
		case GL_RG32I:
		case GL_RG32UI:
		case GL_RGBA8I:
		case GL_RGBA8UI:
		case GL_RGBA16I:
		case GL_RGBA16UI:
		case GL_RGBA32I:
		case GL_RGBA32UI:
			return true;

		default:
			return false;
	}
}

deUint32 getTypeForInternalFormat (deUint32 format)
{
	return glu::getTransferFormat(glu::mapGLInternalFormat(format)).dataType;
}

void genTexel (de::Random& rng, deUint32 glFormat, int texelBlockSize, const int texelCount, deUint8* buffer)
{
	if (isFloatFormat(glFormat))
	{
		const tcu::TextureFormat		format	= glu::mapGLInternalFormat(glFormat);
		const tcu::PixelBufferAccess	access	(format, texelCount, 1, 1, buffer);
		const tcu::TextureFormatInfo	info	= tcu::getTextureFormatInfo(format);

		for (int texelNdx = 0; texelNdx < texelCount; texelNdx++)
		{
			const float	red		= rng.getFloat(info.valueMin.x(), info.valueMax.x());
			const float green	= rng.getFloat(info.valueMin.y(), info.valueMax.y());
			const float blue	= rng.getFloat(info.valueMin.z(), info.valueMax.z());
			const float alpha	= rng.getFloat(info.valueMin.w(), info.valueMax.w());

			const Vec4	color	(red, green, blue, alpha);

			access.setPixel(color, texelNdx, 0, 0);
		}
	}
	else if (glu::isCompressedFormat(glFormat))
	{
		const tcu::CompressedTexFormat compressedFormat = glu::mapGLCompressedTexFormat(glFormat);

		if (tcu::isAstcFormat(compressedFormat))
		{
			const int		BLOCK_SIZE				= 16;
			const deUint8	blocks[][BLOCK_SIZE]	=
			{
				// \note All of the following blocks are valid in LDR mode.
				{ 252,	253,	255,	255,	255,	255,	255,	255,	8,		71,		90,		78,		22,		17,		26,		66,		},
				{ 252,	253,	255,	255,	255,	255,	255,	255,	220,	74,		139,	235,	249,	6,		145,	125		},
				{ 252,	253,	255,	255,	255,	255,	255,	255,	223,	251,	28,		206,	54,		251,	160,	174		},
				{ 252,	253,	255,	255,	255,	255,	255,	255,	39,		4,		153,	219,	180,	61,		51,		37		},
				{ 67,	2,		0,		254,	1,		0,		64,		215,	83,		211,	159,	105,	41,		140,	50,		2		},
				{ 67,	130,	0,		170,	84,		255,	65,		215,	83,		211,	159,	105,	41,		140,	50,		2		},
				{ 67,	2,		129,	38,		51,		229,	95,		215,	83,		211,	159,	105,	41,		140,	50,		2		},
				{ 67,	130,	193,	56,		213,	144,	95,		215,	83,		211,	159,	105,	41,		140,	50,		2		}
			};

			DE_ASSERT(texelBlockSize == BLOCK_SIZE);

			for (int i = 0; i < texelCount; i++)
			{
				const int blockNdx = rng.getInt(0, DE_LENGTH_OF_ARRAY(blocks)-1);

				deMemcpy(buffer + i * BLOCK_SIZE,  blocks[blockNdx], BLOCK_SIZE);
			}
		}
		else
		{
			for (int i = 0; i < texelBlockSize * texelCount; i++)
			{
				const deUint8 val = rng.getUint8();

				buffer[i] = val;
			}
		}
	}
	else
	{
		for (int i = 0; i < texelBlockSize * texelCount; i++)
		{
			const deUint8 val = rng.getUint8();

			buffer[i] = val;
		}
	}
}

IVec3 divRoundUp (const IVec3& a, const IVec3& b)
{
	IVec3 res;

	for (int i =0; i < 3; i++)
		res[i] = a[i] / b[i] + ((a[i] % b[i]) ? 1 : 0);

	return res;
}

deUint32 getFormatForInternalFormat (deUint32 format)
{
	return glu::getTransferFormat(glu::mapGLInternalFormat(format)).format;
}

void genericTexImage (const glw::Functions&	gl,
					  deUint32				target,
					  int					faceNdx,
					  int					level,
					  const IVec3&			size,
					  deUint32				format,
					  size_t				dataSize,
					  const void*			data)
{
	const deUint32 glTarget = (target == GL_TEXTURE_CUBE_MAP ? mapFaceNdxToFace(faceNdx) : target);

	DE_ASSERT(target == GL_TEXTURE_CUBE_MAP || faceNdx == 0);

	if (glu::isCompressedFormat(format))
	{
		switch (getTargetTexDims(target))
		{
			case 2:
				DE_ASSERT(size.z() == 1);
				gl.compressedTexImage2D(glTarget, level, format, (glw::GLsizei)size.x(), (glw::GLsizei)size.y(), 0, (glw::GLsizei)dataSize, data);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glCompressedTexImage2D failed.");
				break;

			case 3:
				gl.compressedTexImage3D(glTarget, level, format, (glw::GLsizei)size.x(), (glw::GLsizei)size.y(), (glw::GLsizei)size.z(), 0, (glw::GLsizei)dataSize, data);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glCompressedTexImage3D failed.");
				break;

			default:
				DE_ASSERT(false);
		}
	}
	else
	{
		const deUint32	glFormat	= getFormatForInternalFormat(format);
		const deUint32	glType		= getTypeForInternalFormat(format);

		switch (getTargetTexDims(target))
		{
			case 2:
				DE_ASSERT(size.z() == 1);
				gl.texImage2D(glTarget, level, format, (glw::GLsizei)size.x(), (glw::GLsizei)size.y(), 0, glFormat, glType, data);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glTexImage2D failed.");
				break;

			case 3:
				gl.texImage3D(glTarget, level, format, (glw::GLsizei)size.x(), (glw::GLsizei)size.y(), (glw::GLsizei)size.z(), 0, glFormat, glType, data);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glTexImage3D failed.");
				break;

			default:
				DE_ASSERT(false);
		}
	}
}

void genTextureImage (const glw::Functions&				gl,
					  de::Random&						rng,
					  deUint32							name,
					  vector<ArrayBuffer<deUint8> >&	levels,
					  const ImageInfo&					info,
					  deUint32							moreRestrictiveFormat)
{
	const int		texelBlockSize			= getTexelBlockSize(info.getFormat());
	const IVec3		texelBlockPixelSize		= getTexelBlockPixelSize(info.getFormat());

	levels.resize(getLevelCount(info));

	gl.pixelStorei(GL_UNPACK_ALIGNMENT, 1);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Setting pixel store aligment failed.");

	gl.bindTexture(info.getTarget(), name);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Binding texture failed.");

	for (int levelNdx = 0; levelNdx < getLevelCount(info); levelNdx++)
	{
		ArrayBuffer<deUint8>&	level					= levels[levelNdx];

		const int				faceCount				= (info.getTarget() == GL_TEXTURE_CUBE_MAP ? 6 : 1);

		const IVec3				levelPixelSize			= getLevelSize(info.getTarget(), info.getSize(), levelNdx);
		const IVec3				levelTexelBlockSize		= divRoundUp(levelPixelSize, texelBlockPixelSize);
		const int				levelTexelBlockCount	= levelTexelBlockSize.x() * levelTexelBlockSize.y() * levelTexelBlockSize.z();
		const int				levelSize				= levelTexelBlockCount * texelBlockSize;

		level.setStorage(levelSize * faceCount);

		for (int faceNdx = 0; faceNdx < faceCount; faceNdx++)
		{
			genTexel(rng, moreRestrictiveFormat, texelBlockSize, levelTexelBlockCount, level.getElementPtr(faceNdx * levelSize));

			genericTexImage(gl, info.getTarget(), faceNdx, levelNdx, levelPixelSize, info.getFormat(), levelSize, level.getElementPtr(faceNdx * levelSize));
		}
	}

	gl.texParameteri(info.getTarget(), GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	gl.texParameteri(info.getTarget(), GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	if (info.getTarget() == GL_TEXTURE_3D)
		gl.texParameteri(info.getTarget(), GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	gl.texParameteri(info.getTarget(), GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	gl.texParameteri(info.getTarget(), GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Setting texture parameters failed");

	gl.bindTexture(info.getTarget(), 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Unbinding texture failed.");
}

void genRenderbufferImage (const glw::Functions&			gl,
						   de::Random&						rng,
						   deUint32							name,
						   vector<ArrayBuffer<deUint8> >&	levels,
						   const ImageInfo&					info,
						   deUint32							moreRestrictiveFormat)
{
	const IVec3					size	= info.getSize();
	const tcu::TextureFormat	format	= glu::mapGLInternalFormat(info.getFormat());

	DE_ASSERT(info.getTarget() == GL_RENDERBUFFER);
	DE_ASSERT(info.getSize().z() == 1);
	DE_ASSERT(getLevelCount(info) == 1);
	DE_ASSERT(!glu::isCompressedFormat(info.getFormat()));

	glu::Framebuffer framebuffer(gl);

	levels.resize(1);
	levels[0].setStorage(format.getPixelSize() * size.x() * size.y());
	tcu::PixelBufferAccess refAccess(format, size.x(), size.y(), 1, levels[0].getPtr());

	gl.bindRenderbuffer(GL_RENDERBUFFER, name);
	gl.renderbufferStorage(GL_RENDERBUFFER, info.getFormat(), info.getSize().x(), info.getSize().y());
	GLU_EXPECT_NO_ERROR(gl.getError(), "Binding and setting storage for renderbuffer failed.");

	gl.bindFramebuffer(GL_FRAMEBUFFER, *framebuffer);
	gl.framebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, name);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Binding framebuffer and attaching renderbuffer failed.");

	{
		vector<deUint8> texelBlock(format.getPixelSize());

		if (isFixedPointFormat(info.getFormat()))
		{
			// All zeroes is only bit pattern that fixed point values can be
			// cleared to and that is valid floating point value.
			if (isFloatFormat(moreRestrictiveFormat))
				deMemset(&texelBlock[0], 0x0, texelBlock.size());
			else
			{
				// Fixed point values can be only cleared to all 0 or 1.
				const deInt32 fill = rng.getBool() ? 0xFF : 0x0;
				deMemset(&texelBlock[0], fill, texelBlock.size());
			}
		}
		else
			genTexel(rng, moreRestrictiveFormat, format.getPixelSize(), 1, &(texelBlock[0]));

		{
			const tcu::ConstPixelBufferAccess texelAccess (format, 1, 1, 1, &(texelBlock[0]));

			if (isIntFormat(info.getFormat()))
			{
				const tcu::IVec4 color = texelAccess.getPixelInt(0, 0, 0);

				gl.clearBufferiv(GL_COLOR, 0, (const deInt32*)&color);
				GLU_EXPECT_NO_ERROR(gl.getError(), "Failed to clear renderbuffer.");

				DE_ASSERT(!tcu::isSRGB(format));
				tcu::clear(refAccess, color);
			}
			else if (isUintFormat(info.getFormat()))
			{
				const tcu::IVec4 color = texelAccess.getPixelInt(0, 0, 0);

				gl.clearBufferuiv(GL_COLOR, 0, (const deUint32*)&color);
				GLU_EXPECT_NO_ERROR(gl.getError(), "Failed to clear renderbuffer.");

				DE_ASSERT(!tcu::isSRGB(format));
				tcu::clear(refAccess, color);
			}
			else
			{
				const tcu::Vec4 rawColor	= texelAccess.getPixel(0, 0, 0);
				const tcu::Vec4 linearColor	= (tcu::isSRGB(format) ? tcu::sRGBToLinear(rawColor) : rawColor);

				// rawColor bit pattern has been chosen to be "safe" in the destination format. For sRGB
				// formats, the clear color is in linear space. Since we want the resulting bit pattern
				// to be safe after implementation linear->sRGB transform, we must apply the inverting
				// transform to the clear color.

				if (isFloatFormat(info.getFormat()))
				{
					gl.clearBufferfv(GL_COLOR, 0, (const float*)&linearColor);
				}
				else
				{
					// fixed-point
					gl.clearColor(linearColor.x(), linearColor.y(), linearColor.z(), linearColor.w());
					gl.clear(GL_COLOR_BUFFER_BIT);
				}
				GLU_EXPECT_NO_ERROR(gl.getError(), "Failed to clear renderbuffer.");

				tcu::clear(refAccess, rawColor);
			}
		}
	}

	gl.bindRenderbuffer(GL_RENDERBUFFER, 0);
	gl.bindFramebuffer(GL_FRAMEBUFFER, 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Failed to unbind renderbufer and framebuffer.");
}

void genImage (const glw::Functions&			gl,
			   de::Random&						rng,
			   deUint32							name,
			   vector<ArrayBuffer<deUint8> >&	levels,
			   const ImageInfo&					info,
			   deUint32							moreRestrictiveFormat)
{
	if (isTextureTarget(info.getTarget()))
		genTextureImage(gl, rng, name, levels, info, moreRestrictiveFormat);
	else
		genRenderbufferImage(gl, rng, name, levels, info, moreRestrictiveFormat);
}

IVec3 getTexelBlockStride (const ImageInfo& info, int level)
{
	const IVec3	size					= getLevelSize(info.getTarget(), info.getSize(), level);
	const int	texelBlockSize			= getTexelBlockSize(info.getFormat());
	const IVec3 texelBlockPixelSize		= getTexelBlockPixelSize(info.getFormat());
	const IVec3 textureTexelBlockSize	= divRoundUp(size, texelBlockPixelSize);

	return IVec3(texelBlockSize, textureTexelBlockSize.x() * texelBlockSize, textureTexelBlockSize.x() * textureTexelBlockSize.y() * texelBlockSize);
}

int sumComponents (const IVec3& v)
{
	int s = 0;

	for (int i = 0; i < 3; i++)
		s += v[i];

	return s;
}

void copyImageData (vector<ArrayBuffer<deUint8> >&			dstImageData,
					const ImageInfo&						dstImageInfo,
					int										dstLevel,
					const IVec3&							dstPos,

					const vector<ArrayBuffer<deUint8> >&	srcImageData,
					const ImageInfo&						srcImageInfo,
					int										srcLevel,
					const IVec3&							srcPos,

					const IVec3&							copySize)
{
	const ArrayBuffer<deUint8>&	srcLevelData			= srcImageData[srcLevel];
	ArrayBuffer<deUint8>&		dstLevelData			= dstImageData[dstLevel];

	const IVec3					srcTexelBlockPixelSize	= getTexelBlockPixelSize(srcImageInfo.getFormat());
	const int					srcTexelBlockSize		= getTexelBlockSize(srcImageInfo.getFormat());
	const IVec3					srcTexelPos				= srcPos / srcTexelBlockPixelSize;
	const IVec3					srcTexelBlockStride		= getTexelBlockStride(srcImageInfo, srcLevel);

	const IVec3					dstTexelBlockPixelSize	= getTexelBlockPixelSize(dstImageInfo.getFormat());
	const int					dstTexelBlockSize		= getTexelBlockSize(dstImageInfo.getFormat());
	const IVec3					dstTexelPos				= dstPos / dstTexelBlockPixelSize;
	const IVec3					dstTexelBlockStride		= getTexelBlockStride(dstImageInfo, dstLevel);

	const IVec3					copyTexelBlockCount		= copySize / srcTexelBlockPixelSize;
	const int					texelBlockSize			= srcTexelBlockSize;

	DE_ASSERT(srcTexelBlockSize == dstTexelBlockSize);
	DE_UNREF(dstTexelBlockSize);

	DE_ASSERT((copySize.x() % srcTexelBlockPixelSize.x()) == 0);
	DE_ASSERT((copySize.y() % srcTexelBlockPixelSize.y()) == 0);
	DE_ASSERT((copySize.z() % srcTexelBlockPixelSize.z()) == 0);

	DE_ASSERT((srcPos.x() % srcTexelBlockPixelSize.x()) == 0);
	DE_ASSERT((srcPos.y() % srcTexelBlockPixelSize.y()) == 0);
	DE_ASSERT((srcPos.z() % srcTexelBlockPixelSize.z()) == 0);

	for (int z = 0; z < copyTexelBlockCount.z(); z++)
	for (int y = 0; y < copyTexelBlockCount.y(); y++)
	{
		const IVec3				blockPos		(0, y, z);
		const deUint8* const	srcPtr			= srcLevelData.getElementPtr(sumComponents((srcTexelPos + blockPos) * srcTexelBlockStride));
		deUint8* const			dstPtr			= dstLevelData.getElementPtr(sumComponents((dstTexelPos + blockPos) * dstTexelBlockStride));
		const int				copyLineSize	= copyTexelBlockCount.x() * texelBlockSize;

		deMemcpy(dstPtr, srcPtr, copyLineSize);
	}
}

vector<tcu::ConstPixelBufferAccess> getLevelAccesses (const vector<ArrayBuffer<deUint8> >& data, const ImageInfo& info)
{
	const tcu::TextureFormat			format	= glu::mapGLInternalFormat(info.getFormat());
	const IVec3							size	= info.getSize();

	vector<tcu::ConstPixelBufferAccess>	result;

	DE_ASSERT((int)data.size() == getLevelCount(info));

	for (int level = 0; level < (int)data.size(); level++)
	{
		const IVec3 levelSize = getLevelSize(info.getTarget(), size, level);

		result.push_back(tcu::ConstPixelBufferAccess(format, levelSize.x(), levelSize.y(), levelSize.z(), data[level].getPtr()));
	}

	return result;
}

vector<tcu::ConstPixelBufferAccess> getCubeLevelAccesses (const vector<ArrayBuffer<deUint8> >&	data,
														  const ImageInfo&						info,
														  int									faceNdx)
{
	const tcu::TextureFormat			format				= glu::mapGLInternalFormat(info.getFormat());
	const IVec3							size				= info.getSize();
	const int							texelBlockSize		= getTexelBlockSize(info.getFormat());
	const IVec3							texelBlockPixelSize = getTexelBlockPixelSize(info.getFormat());
	vector<tcu::ConstPixelBufferAccess>	result;

	DE_ASSERT(info.getTarget() == GL_TEXTURE_CUBE_MAP);
	DE_ASSERT((int)data.size() == getLevelCount(info));

	for (int level = 0; level < (int)data.size(); level++)
	{
		const IVec3 levelPixelSize			= getLevelSize(info.getTarget(), size, level);
		const IVec3	levelTexelBlockSize		= divRoundUp(levelPixelSize, texelBlockPixelSize);
		const int	levelTexelBlockCount	= levelTexelBlockSize.x() * levelTexelBlockSize.y() * levelTexelBlockSize.z();
		const int	levelSize				= levelTexelBlockCount * texelBlockSize;

		result.push_back(tcu::ConstPixelBufferAccess(format, levelPixelSize.x(), levelPixelSize.y(), levelPixelSize.z(), data[level].getElementPtr(levelSize * faceNdx)));
	}

	return result;
}

void copyImage (const glw::Functions&					gl,

				deUint32								dstName,
				vector<ArrayBuffer<deUint8> >&			dstImageData,
				const ImageInfo&						dstImageInfo,
				int										dstLevel,
				const IVec3&							dstPos,

				deUint32								srcName,
				const vector<ArrayBuffer<deUint8> >&	srcImageData,
				const ImageInfo&						srcImageInfo,
				int										srcLevel,
				const IVec3&							srcPos,

				const IVec3&							copySize)
{
	gl.copyImageSubData(srcName, srcImageInfo.getTarget(), srcLevel, srcPos.x(), srcPos.y(), srcPos.z(),
						dstName, dstImageInfo.getTarget(), dstLevel, dstPos.x(), dstPos.y(), dstPos.z(),
						copySize.x(), copySize.y(), copySize.z());

	GLU_EXPECT_NO_ERROR(gl.getError(), "glCopyImageSubData failed.");

	copyImageData(dstImageData, dstImageInfo, dstLevel, dstPos,
				  srcImageData, srcImageInfo, srcLevel, srcPos, copySize);
}

template<class TextureView>
void renderTexture (glu::RenderContext&		renderContext,
					TextureRenderer&		renderer,
					ReferenceParams&		renderParams,
					tcu::ResultCollector&	results,
					de::Random&				rng,
					const TextureView&		refTexture,
					const Verify			verify,
					TextureImageIterator&	imageIterator,
					tcu::TestLog&			log)
{
	const tcu::RenderTarget&	renderTarget		= renderContext.getRenderTarget();
	const tcu::RGBA				threshold			= renderTarget.getPixelFormat().getColorThreshold() + tcu::RGBA(1,1,1,1);
	const glw::Functions&		gl					= renderContext.getFunctions();
	const IVec2					renderTargetSize	= IVec2(renderTarget.getWidth(), renderTarget.getHeight());

	while (imageIterator.hasNextImage())
	{
		// \note: Reserve space upfront to avoid assigning tcu::Surface, which incurs buffer mem copy. Using a
		// conservative estimate for simplicity
		const int				imagesOnLevel	= imageIterator.getLevelImageCount();
		const int				imageEstimate	= (imageIterator.getMipLevelCount() - imageIterator.getMipLevel()) * imagesOnLevel;
		RandomizedRenderGrid	renderGrid		(renderTargetSize, imageIterator.getSize(), imageEstimate, rng.getUint32());
		vector<CellContents>	cellContents	(renderGrid.getCellCount());
		int						cellsUsed		= 0;

		// \note: Ordering of conditions is significant. If put the other way around, the code would skip one of the
		// images if the grid runs out of cells before the texture runs out of images. Advancing one grid cell over the
		// needed number has no negative impact.
		while (renderGrid.nextCell() && imageIterator.nextImage())
		{
			const int		level	  = imageIterator.getMipLevel();
			const IVec2		levelSize = imageIterator.getSize();
			const IVec2		origin	  = renderGrid.getOrigin();
			vector<float>	texCoord;

			DE_ASSERT(imageIterator.getTarget() != GL_TEXTURE_CUBE_MAP || levelSize.x() >= 4 || levelSize.y() >= 4);

			renderParams.baseLevel	= level;
			renderParams.maxLevel	= level;

			gl.texParameteri(imageIterator.getTarget(), GL_TEXTURE_BASE_LEVEL, level);
			gl.texParameteri(imageIterator.getTarget(), GL_TEXTURE_MAX_LEVEL, level);

			computeQuadTexCoords(texCoord, imageIterator);

			// Setup base viewport.
			gl.viewport(origin.x(), origin.y(), levelSize.x(), levelSize.y());

			// Draw.
			renderer.renderQuad(0, &texCoord[0], renderParams);
			GLU_EXPECT_NO_ERROR(gl.getError(), "Failed to render.");

			if (verify == VERIFY_COMPARE_REFERENCE)
			{
				const int	target					= imageIterator.getTarget();
				const int	imageIndex				= imageIterator.getCurrentImage();

				cellContents[cellsUsed].origin		= origin;
				cellContents[cellsUsed].name		= getTextureImageName(target, level, imageIndex);
				cellContents[cellsUsed].description	= getTextureImageDescription(target, level, imageIndex);

				cellContents[cellsUsed].reference.setSize(levelSize.x(), levelSize.y());

				// Compute reference.
				sampleTexture(tcu::SurfaceAccess(cellContents[cellsUsed].reference, renderContext.getRenderTarget().getPixelFormat()), refTexture, &texCoord[0], renderParams);
				cellsUsed++;
			}
		}

		if (cellsUsed > 0)
		{
			const IVec4		boundingBox		= renderGrid.getUsedAreaBoundingBox();
			tcu::Surface	renderedFrame	(boundingBox[2], boundingBox[3]);

			glu::readPixels(renderContext, boundingBox.x(), boundingBox.y(), renderedFrame.getAccess());
			GLU_EXPECT_NO_ERROR(gl.getError(), "Failed to read pixels.");

			for (int idx = 0; idx < cellsUsed; idx++)
			{
				const CellContents&					cell		 (cellContents[idx]);
				const IVec2							cellOrigin	 = cell.origin - boundingBox.toWidth<2>();
				const tcu::ConstPixelBufferAccess	resultAccess = getSubregion(renderedFrame.getAccess(), cellOrigin.x(), cellOrigin.y(), cell.reference.getWidth(), cell.reference.getHeight());

				if (!intThresholdCompare(log, cell.name.c_str(), cell.description.c_str(), cell.reference.getAccess(), resultAccess, threshold.toIVec().cast<deUint32>(), tcu::COMPARE_LOG_ON_ERROR))
					results.fail("Image comparison of " + cell.description + " failed.");
				else
					log << TestLog::Message << "Image comparison of " << cell.description << " passed." << TestLog::EndMessage;;
			}
		}
	}

	gl.texParameteri(imageIterator.getTarget(), GL_TEXTURE_BASE_LEVEL, 0);
	gl.texParameteri(imageIterator.getTarget(), GL_TEXTURE_MAX_LEVEL, 1000);
}

void renderTexture2DView (tcu::TestContext&			testContext,
						  glu::RenderContext&		renderContext,
						  TextureRenderer&			renderer,
						  tcu::ResultCollector&		results,
						  de::Random&				rng,
						  deUint32					name,
						  const ImageInfo&			info,
						  const tcu::Texture2DView&	refTexture,
						  Verify					verify)
{
	tcu::TestLog&					log				= testContext.getLog();
	const glw::Functions&			gl				= renderContext.getFunctions();
	const tcu::TextureFormat		format			= refTexture.getLevel(0).getFormat();
	const tcu::TextureFormatInfo	spec			= tcu::getTextureFormatInfo(format);

	ReferenceParams					renderParams	(TEXTURETYPE_2D);
	TextureImageIterator			imageIterator	(info, getLevelCount(info));

	renderParams.samplerType	= getSamplerType(format);
	renderParams.sampler		= Sampler(Sampler::CLAMP_TO_EDGE, Sampler::CLAMP_TO_EDGE, Sampler::CLAMP_TO_EDGE, Sampler::NEAREST_MIPMAP_NEAREST, Sampler::NEAREST);
	renderParams.colorScale		= spec.lookupScale;
	renderParams.colorBias		= spec.lookupBias;

	gl.activeTexture(GL_TEXTURE0);
	gl.bindTexture(GL_TEXTURE_2D, name);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Failed to bind texture.");

	gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
	gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Failed to setup texture filtering state.");

	renderTexture<tcu::Texture2DView>(renderContext, renderer, renderParams, results, rng, refTexture, verify, imageIterator, log);

	gl.bindTexture(GL_TEXTURE_2D, 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Failed to unbind texture.");
}

void decompressTextureLevel (const tcu::TexDecompressionParams&		params,
							 ArrayBuffer<deUint8>&					levelData,
							 tcu::PixelBufferAccess&				levelAccess,
							 const tcu::CompressedTexFormat&		compressedFormat,
							 const tcu::TextureFormat&				decompressedFormat,
							 const IVec3&							levelPixelSize,
							 const void*							data)
{
	levelData.setStorage(levelPixelSize.x() * levelPixelSize.y() * levelPixelSize.z() * decompressedFormat.getPixelSize());
	levelAccess = tcu::PixelBufferAccess(decompressedFormat, levelPixelSize.x(), levelPixelSize.y(), levelPixelSize.z(), levelData.getPtr());

	tcu::decompress(levelAccess, compressedFormat, (const deUint8*)data, params);
}

void decompressTexture (vector<ArrayBuffer<deUint8> >&			levelDatas,
						vector<tcu::PixelBufferAccess>&			levelAccesses,
						glu::RenderContext&						renderContext,
						const ImageInfo&						info,
						const vector<ArrayBuffer<deUint8> >&	data)
{
	const tcu::CompressedTexFormat	compressedFormat	= glu::mapGLCompressedTexFormat(info.getFormat());
	const tcu::TextureFormat		decompressedFormat	= tcu::getUncompressedFormat(compressedFormat);
	const IVec3						size				= info.getSize();
	const bool						isES32				= glu::contextSupports(renderContext.getType(), glu::ApiType::es(3, 2));

	de::UniquePtr<glu::ContextInfo>	ctxInfo				(glu::ContextInfo::create(renderContext));
	tcu::TexDecompressionParams		decompressParams;

	if (tcu::isAstcFormat(compressedFormat))
	{
		if (ctxInfo->isExtensionSupported("GL_KHR_texture_compression_astc_hdr") && !tcu::isAstcSRGBFormat(compressedFormat))
			decompressParams = tcu::TexDecompressionParams(tcu::TexDecompressionParams::ASTCMODE_HDR);
		else if (isES32 || ctxInfo->isExtensionSupported("GL_KHR_texture_compression_astc_ldr"))
			decompressParams = tcu::TexDecompressionParams(tcu::TexDecompressionParams::ASTCMODE_LDR);
		else
			DE_ASSERT(false);
	}

	levelDatas.resize(getLevelCount(info));
	levelAccesses.resize(getLevelCount(info));

	for (int level = 0; level < getLevelCount(info); level++)
	{
		const IVec3					levelPixelSize	= getLevelSize(info.getTarget(), size, level);
		de::ArrayBuffer<deUint8>&	levelData		= levelDatas[level];
		tcu::PixelBufferAccess&		levelAccess		= levelAccesses[level];

		decompressTextureLevel(decompressParams, levelData, levelAccess, compressedFormat, decompressedFormat, levelPixelSize, data[level].getPtr());
	}
}

void renderTexture2D (tcu::TestContext&						testContext,
					  glu::RenderContext&					renderContext,
					  TextureRenderer&						textureRenderer,
					  tcu::ResultCollector&					results,
					  de::Random&							rng,
					  deUint32								name,
					  const vector<ArrayBuffer<deUint8> >&	data,
					  const ImageInfo&						info,
					  Verify								verify)
{
	if (glu::isCompressedFormat(info.getFormat()))
	{
		vector<de::ArrayBuffer<deUint8> >	levelDatas;
		vector<tcu::PixelBufferAccess>		levelAccesses;

		decompressTexture(levelDatas, levelAccesses, renderContext, info, data);

		{
			const tcu::Texture2DView refTexture((int)levelAccesses.size(), &(levelAccesses[0]));

			renderTexture2DView(testContext, renderContext, textureRenderer, results, rng, name, info, refTexture, verify);
		}
	}
	else
	{
		const vector<tcu::ConstPixelBufferAccess>	levelAccesses	= getLevelAccesses(data, info);
		const tcu::Texture2DView					refTexture		((int)levelAccesses.size(), &(levelAccesses[0]));

		renderTexture2DView(testContext, renderContext, textureRenderer, results, rng, name, info, refTexture, verify);
	}
}

void renderTexture3DView (tcu::TestContext&			testContext,
						  glu::RenderContext&		renderContext,
						  TextureRenderer&			renderer,
						  tcu::ResultCollector&		results,
						  de::Random&				rng,
						  deUint32					name,
						  const ImageInfo&			info,
						  const tcu::Texture3DView&	refTexture,
						  Verify					verify)
{
	tcu::TestLog&					log				= testContext.getLog();
	const glw::Functions&			gl				= renderContext.getFunctions();
	const tcu::TextureFormat		format			= refTexture.getLevel(0).getFormat();
	const tcu::TextureFormatInfo	spec			= tcu::getTextureFormatInfo(format);

	ReferenceParams					renderParams	(TEXTURETYPE_3D);
	TextureImageIterator			imageIterator	(info, getLevelCount(info));

	renderParams.samplerType	= getSamplerType(format);
	renderParams.sampler		= Sampler(Sampler::CLAMP_TO_EDGE, Sampler::CLAMP_TO_EDGE, Sampler::CLAMP_TO_EDGE, Sampler::NEAREST_MIPMAP_NEAREST, Sampler::NEAREST);
	renderParams.colorScale		= spec.lookupScale;
	renderParams.colorBias		= spec.lookupBias;

	gl.activeTexture(GL_TEXTURE0);
	gl.bindTexture(GL_TEXTURE_3D, name);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Failed to bind texture.");

	gl.texParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	gl.texParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	gl.texParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	gl.texParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
	gl.texParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Failed to setup texture filtering state.");

	renderTexture<tcu::Texture3DView>(renderContext, renderer, renderParams, results, rng, refTexture, verify, imageIterator, log);

	gl.bindTexture(GL_TEXTURE_3D, 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Failed to unbind texture.");
}

void renderTexture3D (tcu::TestContext&						testContext,
					  glu::RenderContext&					renderContext,
					  TextureRenderer&						textureRenderer,
					  tcu::ResultCollector&					results,
					  de::Random&							rng,
					  deUint32								name,
					  const vector<ArrayBuffer<deUint8> >&	data,
					  const ImageInfo&						info,
				      Verify								verify)
{
	if (glu::isCompressedFormat(info.getFormat()))
	{
		vector<de::ArrayBuffer<deUint8> >	levelDatas;
		vector<tcu::PixelBufferAccess>		levelAccesses;

		decompressTexture(levelDatas, levelAccesses, renderContext, info, data);

		{
			const tcu::Texture3DView refTexture((int)levelAccesses.size(), &(levelAccesses[0]));

			renderTexture3DView(testContext, renderContext, textureRenderer, results, rng, name, info, refTexture, verify);
		}
	}
	else
	{
		const vector<tcu::ConstPixelBufferAccess>	levelAccesses	= getLevelAccesses(data, info);
		const tcu::Texture3DView					refTexture		((int)levelAccesses.size(), &(levelAccesses[0]));

		renderTexture3DView(testContext, renderContext, textureRenderer, results, rng, name, info, refTexture, verify);
	}
}

void renderTextureCubemapView (tcu::TestContext&			testContext,
							   glu::RenderContext&			renderContext,
							   TextureRenderer&				renderer,
							   tcu::ResultCollector&		results,
							   de::Random&					rng,
							   deUint32						name,
							   const ImageInfo&				info,
							   const tcu::TextureCubeView&	refTexture,
							   Verify						verify)
{
	tcu::TestLog&					log				= testContext.getLog();
	const glw::Functions&			gl				= renderContext.getFunctions();
	const tcu::TextureFormat		format			= refTexture.getLevelFace(0, tcu::CUBEFACE_POSITIVE_X).getFormat();
	const tcu::TextureFormatInfo	spec			= tcu::getTextureFormatInfo(format);

	ReferenceParams					renderParams	(TEXTURETYPE_CUBE);
    // \note It seems we can't reliably sample two smallest texture levels with cubemaps
	TextureImageIterator			imageIterator	(info, getLevelCount(info) - 2);

	renderParams.samplerType	= getSamplerType(format);
	renderParams.sampler		= Sampler(Sampler::CLAMP_TO_EDGE, Sampler::CLAMP_TO_EDGE, Sampler::CLAMP_TO_EDGE, Sampler::NEAREST_MIPMAP_NEAREST, Sampler::NEAREST);
	renderParams.colorScale		= spec.lookupScale;
	renderParams.colorBias		= spec.lookupBias;

	gl.activeTexture(GL_TEXTURE0);
	gl.bindTexture(GL_TEXTURE_CUBE_MAP, name);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Failed to bind texture.");

	gl.texParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	gl.texParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	gl.texParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
	gl.texParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Failed to setup texture filtering state.");

	renderTexture<tcu::TextureCubeView>(renderContext, renderer, renderParams, results, rng, refTexture, verify, imageIterator, log);

	gl.bindTexture(GL_TEXTURE_CUBE_MAP, 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Failed to unbind texture.");
}

void renderTextureCubemap (tcu::TestContext&					testContext,
						   glu::RenderContext&					renderContext,
						   TextureRenderer&						textureRenderer,
						   tcu::ResultCollector&				results,
						   de::Random&							rng,
						   deUint32								name,
						   const vector<ArrayBuffer<deUint8> >&	data,
						   const ImageInfo&						info,
						   Verify								verify)
{
	if (glu::isCompressedFormat(info.getFormat()))
	{
		const tcu::CompressedTexFormat&	compressedFormat	= glu::mapGLCompressedTexFormat(info.getFormat());
		const tcu::TextureFormat&		decompressedFormat	= tcu::getUncompressedFormat(compressedFormat);

		const int						texelBlockSize		= getTexelBlockSize(info.getFormat());
		const IVec3						texelBlockPixelSize = getTexelBlockPixelSize(info.getFormat());

		const bool						isES32				= glu::contextSupports(renderContext.getType(), glu::ApiType::es(3, 2));

		vector<tcu::PixelBufferAccess>	levelAccesses[6];
		vector<ArrayBuffer<deUint8> >	levelDatas[6];
		de::UniquePtr<glu::ContextInfo>	ctxInfo				(glu::ContextInfo::create(renderContext));
		tcu::TexDecompressionParams		decompressParams;

		if (tcu::isAstcFormat(compressedFormat))
		{
			if (ctxInfo->isExtensionSupported("GL_KHR_texture_compression_astc_hdr") && !tcu::isAstcSRGBFormat(compressedFormat))
				decompressParams = tcu::TexDecompressionParams(tcu::TexDecompressionParams::ASTCMODE_HDR);
			else if (isES32 || ctxInfo->isExtensionSupported("GL_KHR_texture_compression_astc_ldr"))
				decompressParams = tcu::TexDecompressionParams(tcu::TexDecompressionParams::ASTCMODE_LDR);
			else
				DE_ASSERT(false);
		}

		for (int faceNdx = 0; faceNdx < 6; faceNdx++)
		{
			levelAccesses[faceNdx].resize(getLevelCount(info));
			levelDatas[faceNdx].resize(getLevelCount(info));
		}

		for (int level = 0; level < getLevelCount(info); level++)
		{
			for (int faceNdx = 0; faceNdx < 6; faceNdx++)
			{
				const IVec3				levelPixelSize			= getLevelSize(info.getTarget(), info.getSize(), level);
				const IVec3				levelTexelBlockSize		= divRoundUp(levelPixelSize, texelBlockPixelSize);
				const int				levelTexelBlockCount	= levelTexelBlockSize.x() * levelTexelBlockSize.y() * levelTexelBlockSize.z();
				const int				levelSize				= levelTexelBlockCount * texelBlockSize;

				const deUint8*			dataPtr					= data[level].getElementPtr(faceNdx * levelSize);
				tcu::PixelBufferAccess& levelAccess				= levelAccesses[faceNdx][level];
				ArrayBuffer<deUint8>&	levelData				= levelDatas[faceNdx][level];

				decompressTextureLevel(decompressParams, levelData, levelAccess, compressedFormat, decompressedFormat, levelPixelSize, dataPtr);
			}
		}

		const tcu::ConstPixelBufferAccess* levels[6];

		for (int faceNdx = 0; faceNdx < 6; faceNdx++)
			levels[glu::getCubeFaceFromGL(mapFaceNdxToFace(faceNdx))] = &(levelAccesses[faceNdx][0]);

		{
			const tcu::TextureCubeView refTexture(getLevelCount(info), levels);

			renderTextureCubemapView(testContext, renderContext, textureRenderer, results, rng, name, info, refTexture, verify);
		}
	}
	else
	{
		const vector<tcu::ConstPixelBufferAccess> levelAccesses[6] =
		{
			getCubeLevelAccesses(data, info, 0),
			getCubeLevelAccesses(data, info, 1),
			getCubeLevelAccesses(data, info, 2),
			getCubeLevelAccesses(data, info, 3),
			getCubeLevelAccesses(data, info, 4),
			getCubeLevelAccesses(data, info, 5),
		};

		const tcu::ConstPixelBufferAccess* levels[6];

		for (int faceNdx = 0; faceNdx < 6; faceNdx++)
			levels[glu::getCubeFaceFromGL(mapFaceNdxToFace(faceNdx))] = &(levelAccesses[faceNdx][0]);

		{
			const tcu::TextureCubeView refTexture(getLevelCount(info), levels);

			renderTextureCubemapView(testContext, renderContext, textureRenderer, results, rng, name, info, refTexture, verify);
		}
	}
}

void renderTexture2DArrayView (tcu::TestContext&				testContext,
							   glu::RenderContext&				renderContext,
							   TextureRenderer&					renderer,
							   tcu::ResultCollector&			results,
							   de::Random&						rng,
							   deUint32							name,
							   const ImageInfo&					info,
							   const tcu::Texture2DArrayView&	refTexture,
							   Verify							verify)
{
	tcu::TestLog&					log				= testContext.getLog();
	const glw::Functions&			gl				= renderContext.getFunctions();
	const tcu::TextureFormat		format			= refTexture.getLevel(0).getFormat();
	const tcu::TextureFormatInfo	spec			= tcu::getTextureFormatInfo(format);

	ReferenceParams					renderParams	(TEXTURETYPE_2D_ARRAY);
	TextureImageIterator			imageIterator	(info, getLevelCount(info));

	renderParams.samplerType	= getSamplerType(format);
	renderParams.sampler		= Sampler(Sampler::CLAMP_TO_EDGE, Sampler::CLAMP_TO_EDGE, Sampler::CLAMP_TO_EDGE, Sampler::NEAREST_MIPMAP_NEAREST, Sampler::NEAREST);
	renderParams.colorScale		= spec.lookupScale;
	renderParams.colorBias		= spec.lookupBias;

	gl.activeTexture(GL_TEXTURE0);
	gl.bindTexture(GL_TEXTURE_2D_ARRAY, name);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Failed to bind texture.");

	gl.texParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	gl.texParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	gl.texParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
	gl.texParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Failed to setup texture filtering state.");

	renderTexture<tcu::Texture2DArrayView>(renderContext, renderer, renderParams, results, rng, refTexture, verify, imageIterator, log);

	gl.bindTexture(GL_TEXTURE_2D_ARRAY, 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Failed to unbind texture.");
}

void renderTexture2DArray (tcu::TestContext&					testContext,
						   glu::RenderContext&					renderContext,
						   TextureRenderer&						textureRenderer,
						   tcu::ResultCollector&				results,
						   de::Random&							rng,
						   deUint32								name,
						   const vector<ArrayBuffer<deUint8> >&	data,
						   const ImageInfo&						info,
						   Verify								verify)
{
	if (glu::isCompressedFormat(info.getFormat()))
	{
		vector<de::ArrayBuffer<deUint8> >	levelDatas;
		vector<tcu::PixelBufferAccess>		levelAccesses;

		decompressTexture(levelDatas, levelAccesses, renderContext, info, data);

		{
			const tcu::Texture2DArrayView refTexture((int)levelAccesses.size(), &(levelAccesses[0]));

			renderTexture2DArrayView(testContext, renderContext, textureRenderer, results, rng, name, info, refTexture, verify);
		}
	}
	else
	{
		const vector<tcu::ConstPixelBufferAccess>	levelAccesses	= getLevelAccesses(data, info);
		const tcu::Texture2DArrayView				refTexture		((int)levelAccesses.size(), &(levelAccesses[0]));

		renderTexture2DArrayView(testContext, renderContext, textureRenderer, results, rng, name, info, refTexture, verify);
	}
}

tcu::TextureFormat getReadPixelFormat (const tcu::TextureFormat& format)
{
	switch (tcu::getTextureChannelClass(format.type))
	{
		case tcu::TEXTURECHANNELCLASS_FLOATING_POINT:
		case tcu::TEXTURECHANNELCLASS_SIGNED_FIXED_POINT:
		case tcu::TEXTURECHANNELCLASS_UNSIGNED_FIXED_POINT:
			return tcu::TextureFormat(tcu::TextureFormat::RGBA, tcu::TextureFormat::UNORM_INT8);

		case tcu::TEXTURECHANNELCLASS_SIGNED_INTEGER:
			return tcu::TextureFormat(tcu::TextureFormat::RGBA, tcu::TextureFormat::SIGNED_INT32);

		case tcu::TEXTURECHANNELCLASS_UNSIGNED_INTEGER:
			return tcu::TextureFormat(tcu::TextureFormat::RGBA, tcu::TextureFormat::UNSIGNED_INT32);

		default:
			DE_ASSERT(false);
			return tcu::TextureFormat();
	}
}

Vec4 calculateThreshold (const tcu::TextureFormat& sourceFormat, const tcu::TextureFormat& readPixelsFormat)
{
	DE_ASSERT(tcu::getTextureChannelClass(sourceFormat.type) != tcu::TEXTURECHANNELCLASS_FLOATING_POINT);
	DE_ASSERT(tcu::getTextureChannelClass(readPixelsFormat.type) != tcu::TEXTURECHANNELCLASS_FLOATING_POINT);

	DE_ASSERT(tcu::getTextureChannelClass(sourceFormat.type) != tcu::TEXTURECHANNELCLASS_SIGNED_INTEGER);
	DE_ASSERT(tcu::getTextureChannelClass(readPixelsFormat.type) != tcu::TEXTURECHANNELCLASS_SIGNED_INTEGER);

	DE_ASSERT(tcu::getTextureChannelClass(sourceFormat.type) != tcu::TEXTURECHANNELCLASS_UNSIGNED_INTEGER);
	DE_ASSERT(tcu::getTextureChannelClass(readPixelsFormat.type) != tcu::TEXTURECHANNELCLASS_UNSIGNED_INTEGER);

	{
		const tcu::IVec4	srcBits		= tcu::getTextureFormatBitDepth(sourceFormat);
		const tcu::IVec4	readBits	= tcu::getTextureFormatBitDepth(readPixelsFormat);

		return Vec4(1.0f) / ((tcu::IVec4(1) << (tcu::min(srcBits, readBits))) - tcu::IVec4(1)).cast<float>();
	}
}

void renderRenderbuffer (tcu::TestContext&						testContext,
						 glu::RenderContext&					renderContext,
						 tcu::ResultCollector&					results,
						 deUint32								name,
						 const vector<ArrayBuffer<deUint8> >&	data,
						 const ImageInfo&						info,
						 Verify									verify)
{
	const glw::Functions&				gl					= renderContext.getFunctions();
	TestLog&							log					= testContext.getLog();

	const tcu::TextureFormat			format				= glu::mapGLInternalFormat(info.getFormat());
	const IVec3							size				= info.getSize();
	const tcu::ConstPixelBufferAccess	refRenderbuffer		(format, size.x(), size.y(), 1, data[0].getPtr());
	const tcu::TextureFormat			readPixelsFormat	= getReadPixelFormat(format);
	tcu::TextureLevel					renderbuffer		(readPixelsFormat, size.x(), size.y());

	DE_ASSERT(size.z() == 1);
	DE_ASSERT(data.size() == 1);

	{
		glu::Framebuffer framebuffer(gl);

		gl.bindFramebuffer(GL_FRAMEBUFFER, *framebuffer);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Failed to create and bind framebuffer.");

		gl.bindRenderbuffer(GL_RENDERBUFFER, name);
		gl.framebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, name);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Failed to bind and attach renderbuffer to framebuffer.");

		if (verify)
			glu::readPixels(renderContext, 0, 0, renderbuffer.getAccess());

		gl.bindRenderbuffer(GL_RENDERBUFFER, 0);
		gl.bindFramebuffer(GL_FRAMEBUFFER, 0);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Failed to unbind renderbuffer and framebuffer.");
	}

	if (verify == VERIFY_COMPARE_REFERENCE)
	{
		if (isFloatFormat(info.getFormat()))
		{
			const tcu::UVec4 threshold (2, 2, 2, 2);

			if (!(tcu::floatUlpThresholdCompare(log, "Image comparison", "Image comparison", refRenderbuffer, renderbuffer.getAccess(), threshold, tcu::COMPARE_LOG_ON_ERROR)))
				results.fail("Image comparison failed.");
			else
				log << TestLog::Message << "Image comarison passed." << TestLog::EndMessage;
		}
		else if (isIntFormat(info.getFormat()) || isUintFormat(info.getFormat()))
		{
			const tcu::UVec4 threshold (1, 1, 1, 1);

			if (!(tcu::intThresholdCompare(log, "Image comparison", "Image comparison", refRenderbuffer, renderbuffer.getAccess(), threshold, tcu::COMPARE_LOG_ON_ERROR)))
				results.fail("Image comparison failed.");
			else
				log << TestLog::Message << "Image comarison passed." << TestLog::EndMessage;
		}
		else
		{
			const Vec4 threshold = calculateThreshold(format, readPixelsFormat);

			if (!(tcu::floatThresholdCompare(log, "Image comparison", "Image comparison", refRenderbuffer, renderbuffer.getAccess(), threshold, tcu::COMPARE_LOG_ON_ERROR)))
				results.fail("Image comparison failed.");
			else
				log << TestLog::Message << "Image comarison passed." << TestLog::EndMessage;
		}
	}
}

void render (tcu::TestContext&						testContext,
			 glu::RenderContext&					renderContext,
			 TextureRenderer&						textureRenderer,
			 tcu::ResultCollector&					results,
			 de::Random&							rng,
			 deUint32								name,
			 const vector<ArrayBuffer<deUint8> >&	data,
			 const ImageInfo&						info,
			 Verify									verify)
{
	switch (info.getTarget())
	{
		case GL_TEXTURE_2D:
			renderTexture2D(testContext, renderContext, textureRenderer, results, rng, name, data, info, verify);
			break;

		case GL_TEXTURE_3D:
			renderTexture3D(testContext, renderContext, textureRenderer, results, rng, name, data, info, verify);
			break;

		case GL_TEXTURE_CUBE_MAP:
			renderTextureCubemap(testContext, renderContext, textureRenderer, results, rng, name, data, info, verify);
			break;

		case GL_TEXTURE_2D_ARRAY:
			renderTexture2DArray(testContext, renderContext, textureRenderer, results, rng, name, data, info, verify);
			break;

		case GL_RENDERBUFFER:
			renderRenderbuffer(testContext, renderContext, results, name, data, info, verify);
			break;

		default:
			DE_ASSERT(false);
	}
}

void logTestImageInfo (TestLog&			log,
					   const ImageInfo&	imageInfo)
{
	log << TestLog::Message << "Target: " << targetToName(imageInfo.getTarget()) << TestLog::EndMessage;
	log << TestLog::Message << "Size: " << imageInfo.getSize() << TestLog::EndMessage;
	log << TestLog::Message << "Levels: " << getLevelCount(imageInfo) << TestLog::EndMessage;
	log << TestLog::Message << "Format: " << formatToName(imageInfo.getFormat()) << TestLog::EndMessage;
}

void logTestInfo (TestLog&			log,
				  const ImageInfo&	srcImageInfo,
				  const ImageInfo&	dstImageInfo)
{
	tcu::ScopedLogSection section(log, "TestCaseInfo", "Test case info");

	log << TestLog::Message << "Testing copying from " << targetToName(srcImageInfo.getTarget()) << " to " << targetToName(dstImageInfo.getTarget()) << "." << TestLog::EndMessage;

	{
		tcu::ScopedLogSection srcSection(log, "Source image info.", "Source image info.");
		logTestImageInfo(log, srcImageInfo);
	}

	{
		tcu::ScopedLogSection dstSection(log, "Destination image info.", "Destination image info.");
		logTestImageInfo(log, dstImageInfo);
	}
}

class CopyImageTest : public TestCase
{
public:
							CopyImageTest			(Context&			context,
													 const ImageInfo&	srcImage,
													 const ImageInfo&	dstImage,
													 const char*		name,
													 const char*		description);

							~CopyImageTest			(void);

	void					init					(void);
	void					deinit					(void);

	TestCase::IterateResult	iterate					(void);

private:

	void					logTestInfoIter			(void);
	void					createImagesIter		(void);
	void					destroyImagesIter		(void);
	void					verifySourceIter		(void);
	void					verifyDestinationIter	(void);
	void					renderSourceIter		(void);
	void					renderDestinationIter	(void);
	void					copyImageIter			(void);

	typedef void (CopyImageTest::*IterationFunc)(void);

	struct Iteration
	{
		Iteration (int methodCount_, const IterationFunc* methods_)
			: methodCount	(methodCount_)
			, methods		(methods_)
		{
		}

		int						methodCount;
		const IterationFunc*	methods;
	};

	struct State
	{
		State (int					seed,
			   tcu::TestLog&		log,
			   glu::RenderContext&	renderContext)
			: rng				(seed)
			, results			(log)
			, srcImage			(NULL)
			, dstImage			(NULL)
			, textureRenderer	(renderContext, log, glu::getContextTypeGLSLVersion(renderContext.getType()), glu::PRECISION_HIGHP)
		{
		}

		~State (void)
		{
			delete srcImage;
			delete dstImage;
		}

		de::Random						rng;
		tcu::ResultCollector			results;
		glu::ObjectWrapper*				srcImage;
		glu::ObjectWrapper*				dstImage;
		TextureRenderer					textureRenderer;

		vector<ArrayBuffer<deUint8> >	srcImageLevels;
		vector<ArrayBuffer<deUint8> >	dstImageLevels;
	};

	const ImageInfo	m_srcImageInfo;
	const ImageInfo	m_dstImageInfo;

	int				m_iteration;
	State*			m_state;
};

CopyImageTest::CopyImageTest (Context&			context,
							  const ImageInfo&	srcImage,
							  const ImageInfo&	dstImage,
							  const char*		name,
							  const char*		description)
	: TestCase			(context, name, description)
	, m_srcImageInfo	(srcImage)
	, m_dstImageInfo	(dstImage)

	, m_iteration		(0)
	, m_state			(NULL)
{
}

CopyImageTest::~CopyImageTest (void)
{
	deinit();
}

void checkFormatSupport (glu::ContextInfo& info, deUint32 format, deUint32 target, glu::RenderContext& ctx)
{
	const bool isES32 = glu::contextSupports(ctx.getType(), glu::ApiType::es(3, 2));

	if (glu::isCompressedFormat(format))
	{
		if (isAstcFormat(glu::mapGLCompressedTexFormat(format)))
		{
			DE_ASSERT(target != GL_RENDERBUFFER);
			if (!info.isExtensionSupported("GL_KHR_texture_compression_astc_hdr") &&
				!info.isExtensionSupported("GL_OES_texture_compression_astc"))
			{
				if (target == GL_TEXTURE_3D)
					TCU_THROW(NotSupportedError, "TEXTURE_3D target requires HDR astc support.");
				if (!isES32 && !info.isExtensionSupported("GL_KHR_texture_compression_astc_ldr"))
					TCU_THROW(NotSupportedError, "Compressed astc texture not supported.");
			}
		}
		else
		{
			if (!info.isCompressedTextureFormatSupported(format))
				TCU_THROW(NotSupportedError, "Compressed texture not supported.");
		}
	}
}

void CopyImageTest::init (void)
{
	de::UniquePtr<glu::ContextInfo> ctxInfo(glu::ContextInfo::create(m_context.getRenderContext()));
	const bool						isES32 = glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::es(3, 2));

	if (!isES32 && !ctxInfo->isExtensionSupported("GL_EXT_copy_image"))
		throw tcu::NotSupportedError("Extension GL_EXT_copy_image not supported.", "", __FILE__, __LINE__);

	checkFormatSupport(*ctxInfo, m_srcImageInfo.getFormat(), m_srcImageInfo.getTarget(), m_context.getRenderContext());
	checkFormatSupport(*ctxInfo, m_dstImageInfo.getFormat(), m_dstImageInfo.getTarget(), m_context.getRenderContext());

	{
		SeedBuilder builder;

		builder << 903980
				<< m_srcImageInfo
				<< m_dstImageInfo;

		m_state = new State(builder.get(), m_testCtx.getLog(), m_context.getRenderContext());
	}
}

void CopyImageTest::deinit (void)
{
	delete m_state;
	m_state = NULL;
}

void CopyImageTest::logTestInfoIter (void)
{
	TestLog& log = m_testCtx.getLog();

	logTestInfo(log, m_srcImageInfo, m_dstImageInfo);
}

void CopyImageTest::createImagesIter (void)
{
	TestLog&				log						= m_testCtx.getLog();
	glu::RenderContext&		renderContext			= m_context.getRenderContext();
	const glw::Functions&	gl						= renderContext.getFunctions();
	const deUint32			moreRestrictiveFormat	= getMoreRestrictiveFormat(m_srcImageInfo.getFormat(), m_dstImageInfo.getFormat());
	de::Random&				rng						= m_state->rng;

	DE_ASSERT(!m_state->srcImage);
	DE_ASSERT(!m_state->dstImage);

	m_state->srcImage = new glu::ObjectWrapper(gl, getObjectTraits(m_srcImageInfo));
	m_state->dstImage = new glu::ObjectWrapper(gl, getObjectTraits(m_dstImageInfo));

	{
		glu::ObjectWrapper&				srcImage				= *m_state->srcImage;
		glu::ObjectWrapper&				dstImage				= *m_state->dstImage;

		vector<ArrayBuffer<deUint8> >&	srcImageLevels			= m_state->srcImageLevels;
		vector<ArrayBuffer<deUint8> >&	dstImageLevels			= m_state->dstImageLevels;

		log << TestLog::Message << "Creating source image." << TestLog::EndMessage;
		genImage(gl, rng, *srcImage, srcImageLevels, m_srcImageInfo, moreRestrictiveFormat);

		log << TestLog::Message << "Creating destination image." << TestLog::EndMessage;
		genImage(gl, rng, *dstImage, dstImageLevels, m_dstImageInfo, moreRestrictiveFormat);
	}
}

void CopyImageTest::destroyImagesIter (void)
{
	TestLog& log = m_testCtx.getLog();

	log << TestLog::Message << "Deleting source image. " << TestLog::EndMessage;

	delete m_state->srcImage;
	m_state->srcImage = NULL;
	m_state->srcImageLevels.clear();

	log << TestLog::Message << "Deleting destination image. " << TestLog::EndMessage;

	delete m_state->dstImage;
	m_state->dstImage = NULL;
	m_state->dstImageLevels.clear();
}

void CopyImageTest::verifySourceIter (void)
{
	TestLog&						log					= m_testCtx.getLog();
	const tcu::ScopedLogSection		sourceSection		(log, "Source image verify.", "Source image verify.");

	de::Random&						rng					= m_state->rng;
	tcu::ResultCollector&			results				= m_state->results;
	glu::ObjectWrapper&				srcImage			= *m_state->srcImage;
	vector<ArrayBuffer<deUint8> >&	srcImageLevels		= m_state->srcImageLevels;

	log << TestLog::Message << "Verifying source image." << TestLog::EndMessage;

	render(m_testCtx, m_context.getRenderContext(), m_state->textureRenderer, results, rng, *srcImage, srcImageLevels, m_srcImageInfo, VERIFY_COMPARE_REFERENCE);
}

void CopyImageTest::verifyDestinationIter (void)
{
	TestLog&						log					= m_testCtx.getLog();
	const tcu::ScopedLogSection		destinationSection	(log, "Destination image verify.", "Destination image verify.");

	de::Random&						rng					= m_state->rng;
	tcu::ResultCollector&			results				= m_state->results;
	glu::ObjectWrapper&				dstImage			= *m_state->dstImage;
	vector<ArrayBuffer<deUint8> >&	dstImageLevels		= m_state->dstImageLevels;

	log << TestLog::Message << "Verifying destination image." << TestLog::EndMessage;

	render(m_testCtx, m_context.getRenderContext(), m_state->textureRenderer, results, rng, *dstImage, dstImageLevels, m_dstImageInfo, VERIFY_COMPARE_REFERENCE);
}

void CopyImageTest::renderSourceIter (void)
{
	TestLog&						log					= m_testCtx.getLog();
	const tcu::ScopedLogSection		sourceSection		(log, "Source image verify.", "Source image verify.");

	de::Random&						rng					= m_state->rng;
	tcu::ResultCollector&			results				= m_state->results;
	glu::ObjectWrapper&				srcImage			= *m_state->srcImage;
	vector<ArrayBuffer<deUint8> >&	srcImageLevels		= m_state->srcImageLevels;

	log << TestLog::Message << "Verifying source image." << TestLog::EndMessage;

	render(m_testCtx, m_context.getRenderContext(), m_state->textureRenderer, results, rng, *srcImage, srcImageLevels, m_srcImageInfo, VERIFY_NONE);
}

void CopyImageTest::renderDestinationIter (void)
{
	TestLog&						log					= m_testCtx.getLog();
	const tcu::ScopedLogSection		destinationSection	(log, "Destination image verify.", "Destination image verify.");

	de::Random&						rng					= m_state->rng;
	tcu::ResultCollector&			results				= m_state->results;
	glu::ObjectWrapper&				dstImage			= *m_state->dstImage;
	vector<ArrayBuffer<deUint8> >&	dstImageLevels		= m_state->dstImageLevels;

	log << TestLog::Message << "Verifying destination image." << TestLog::EndMessage;

	render(m_testCtx, m_context.getRenderContext(), m_state->textureRenderer, results, rng, *dstImage, dstImageLevels, m_dstImageInfo, VERIFY_NONE);
}

struct Copy
{
	Copy (const IVec3&	srcPos_,
		  int			srcLevel_,

		  const IVec3&	dstPos_,
		  int			dstLevel_,

		  const IVec3&	size_,
		  const IVec3&	dstSize_)
		: srcPos	(srcPos_)
		, srcLevel	(srcLevel_)

		, dstPos	(dstPos_)
		, dstLevel	(dstLevel_)

		, size		(size_)
		, dstSize	(dstSize_)
	{
	}

	IVec3	srcPos;
	int		srcLevel;
	IVec3	dstPos;
	int		dstLevel;
	IVec3	size;
	IVec3	dstSize;	//!< used only for logging
};

int getLastFullLevel (const ImageInfo& info)
{
	const int	levelCount		= getLevelCount(info);
	const IVec3	blockPixelSize	= getTexelBlockPixelSize(info.getFormat());

	for (int level = 0; level < levelCount; level++)
	{
		const IVec3 levelSize = getLevelSize(info.getTarget(), info.getSize(), level);

		if (levelSize.x() < blockPixelSize.x() || levelSize.y() < blockPixelSize.y() || levelSize.z() < blockPixelSize.z())
			return level - 1;
	}

	return levelCount -1;
}

void generateCopies (vector<Copy>& copies, const ImageInfo& srcInfo, const ImageInfo& dstInfo)
{
	const deUint32	srcTarget		= srcInfo.getTarget();
	const deUint32	dstTarget		= dstInfo.getTarget();

	const bool		srcIsTexture	= isTextureTarget(srcInfo.getTarget());
	const bool		dstIsTexture	= isTextureTarget(dstInfo.getTarget());

	const bool		srcIsCube		= srcTarget == GL_TEXTURE_CUBE_MAP;
	const bool		dstIsCube		= dstTarget == GL_TEXTURE_CUBE_MAP;

	const IVec3		srcBlockPixelSize		= getTexelBlockPixelSize(srcInfo.getFormat());
	const IVec3		dstBlockPixelSize		= getTexelBlockPixelSize(dstInfo.getFormat());

	const int levels[] =
	{
		0, 1, -1
	};

	for (int levelNdx = 0; levelNdx < (srcIsTexture || dstIsTexture ? DE_LENGTH_OF_ARRAY(levels) : 1); levelNdx++)
	{
		const int	srcLevel				= (srcIsTexture ? (levels[levelNdx] >= 0 ? levels[levelNdx] : getLastFullLevel(srcInfo)) : 0);
		const int	dstLevel				= (dstIsTexture ? (levels[levelNdx] >= 0 ? levels[levelNdx] : getLastFullLevel(dstInfo)) : 0);

		const IVec3	srcSize					= getLevelSize(srcInfo.getTarget(), srcInfo.getSize(), srcLevel);
		const IVec3	dstSize					= getLevelSize(dstInfo.getTarget(), dstInfo.getSize(), dstLevel);

		// \note These are rounded down
		const IVec3	srcCompleteBlockSize	= IVec3(srcSize.x() / srcBlockPixelSize.x(), srcSize.y() / srcBlockPixelSize.y(), (srcIsCube ? 6 : srcSize.z() / srcBlockPixelSize.z()));
		const IVec3	dstCompleteBlockSize	= IVec3(dstSize.x() / dstBlockPixelSize.x(), dstSize.y() / dstBlockPixelSize.y(), (dstIsCube ? 6 : dstSize.z() / dstBlockPixelSize.z()));

		const IVec3	maxCopyBlockSize		= tcu::min(srcCompleteBlockSize, dstCompleteBlockSize);

		// \note These are rounded down
		const int	copyBlockWidth			= de::max((2 * (maxCopyBlockSize.x() / 4)) - 1, 1);
		const int	copyBlockHeight			= de::max((2 * (maxCopyBlockSize.y() / 4)) - 1, 1);
		const int	copyBlockDepth			= de::max((2 * (maxCopyBlockSize.z() / 4)) - 1, 1);

		// Copy NPOT block to (0,0,0) from other corner on src
		{
			const IVec3	copyBlockSize	(copyBlockWidth, copyBlockHeight, copyBlockDepth);
			const IVec3	srcBlockPos		(srcCompleteBlockSize - copyBlockSize);
			const IVec3	dstBlockPos		(0, 0, 0);

			const IVec3	srcPos			(srcBlockPos * srcBlockPixelSize);
			const IVec3	dstPos			(dstBlockPos * dstBlockPixelSize);
			const IVec3 srcCopySize		(copyBlockSize * srcBlockPixelSize);
			const IVec3 dstCopySize		(copyBlockSize * dstBlockPixelSize);

			copies.push_back(Copy(srcPos, srcLevel, dstPos, dstLevel, srcCopySize, dstCopySize));
		}

		// Copy NPOT block from (0,0,0) to other corner on dst
		{
			const IVec3	copyBlockSize	(copyBlockWidth, copyBlockHeight, copyBlockDepth);
			const IVec3	srcBlockPos		(0, 0, 0);
			const IVec3	dstBlockPos		(dstCompleteBlockSize - copyBlockSize);

			const IVec3	srcPos			(srcBlockPos * srcBlockPixelSize);
			const IVec3	dstPos			(dstBlockPos * dstBlockPixelSize);
			const IVec3 srcCopySize		(copyBlockSize * srcBlockPixelSize);
			const IVec3 dstCopySize		(copyBlockSize * dstBlockPixelSize);

			copies.push_back(Copy(srcPos, srcLevel, dstPos, dstLevel, srcCopySize, dstCopySize));
		}

		// Copy NPOT block near the corner with high coordinates
		{
			const IVec3	copyBlockSize	(copyBlockWidth, copyBlockHeight, copyBlockDepth);
			const IVec3	srcBlockPos		(tcu::max((srcCompleteBlockSize / 4) * 4 - copyBlockSize, IVec3(0)));
			const IVec3	dstBlockPos		(tcu::max((dstCompleteBlockSize / 4) * 4 - copyBlockSize, IVec3(0)));

			const IVec3	srcPos			(srcBlockPos * srcBlockPixelSize);
			const IVec3	dstPos			(dstBlockPos * dstBlockPixelSize);
			const IVec3 srcCopySize		(copyBlockSize * srcBlockPixelSize);
			const IVec3 dstCopySize		(copyBlockSize * dstBlockPixelSize);

			copies.push_back(Copy(srcPos, srcLevel, dstPos, dstLevel, srcCopySize, dstCopySize));
		}
	}
}

void CopyImageTest::copyImageIter (void)
{
	TestLog&						log				= m_testCtx.getLog();
	const glw::Functions&			gl				= m_context.getRenderContext().getFunctions();
	glu::ObjectWrapper&				srcImage		= *m_state->srcImage;
	glu::ObjectWrapper&				dstImage		= *m_state->dstImage;

	vector<ArrayBuffer<deUint8> >&	srcImageLevels	= m_state->srcImageLevels;
	vector<ArrayBuffer<deUint8> >&	dstImageLevels	= m_state->dstImageLevels;
	vector<Copy>					copies;

	generateCopies(copies, m_srcImageInfo, m_dstImageInfo);

	for (int copyNdx = 0; copyNdx < (int)copies.size(); copyNdx++)
	{
		const Copy& copy = copies[copyNdx];

		log	<< TestLog::Message
			<< "Copying area with size " << copy.size
			<< " from source image position " << copy.srcPos << " and mipmap level " << copy.srcLevel
			<< " to destination image position " << copy.dstPos << " and mipmap level " << copy.dstLevel << ". "
			<< "Size in destination format is " << copy.dstSize
			<< TestLog::EndMessage;

		copyImage(gl, *dstImage, dstImageLevels, m_dstImageInfo, copy.dstLevel, copy.dstPos,
					  *srcImage, srcImageLevels, m_srcImageInfo, copy.srcLevel, copy.srcPos, copy.size);
	}
}

TestCase::IterateResult CopyImageTest::iterate (void)
{
	// Note: Returning from iterate() has two side-effects: it touches
	// watchdog and calls eglSwapBuffers. For the first it's important
	// to keep work per iteration reasonable to avoid
	// timeouts. Because of the latter, it's prudent to do more than
	// trivial amount of work. Otherwise we'll end up waiting for a
	// new buffer in swap, it seems.

	// The split below tries to combine trivial work with actually
	// expensive rendering iterations without having too much
	// rendering in one iteration to avoid timeouts.
	const IterationFunc iteration1[] =
	{
		&CopyImageTest::logTestInfoIter,
		&CopyImageTest::createImagesIter,
		&CopyImageTest::renderSourceIter
	};
	const IterationFunc iteration2[] =
	{
		&CopyImageTest::renderDestinationIter
	};
	const IterationFunc iteration3[] =
	{
		&CopyImageTest::copyImageIter,
		&CopyImageTest::verifySourceIter
	};
	const IterationFunc iteration4[] =
	{
		&CopyImageTest::verifyDestinationIter,
		&CopyImageTest::destroyImagesIter
	};
	const IterationFunc iteration5[] =
	{
		&CopyImageTest::createImagesIter,
		&CopyImageTest::copyImageIter,
		&CopyImageTest::verifySourceIter
	};
	const IterationFunc iteration6[] =
	{
		&CopyImageTest::verifyDestinationIter,
		&CopyImageTest::destroyImagesIter
	};
	const Iteration iterations[] =
	{
		Iteration(DE_LENGTH_OF_ARRAY(iteration1), iteration1),
		Iteration(DE_LENGTH_OF_ARRAY(iteration2), iteration2),
		Iteration(DE_LENGTH_OF_ARRAY(iteration3), iteration3),
		Iteration(DE_LENGTH_OF_ARRAY(iteration4), iteration4),
		Iteration(DE_LENGTH_OF_ARRAY(iteration5), iteration5),
		Iteration(DE_LENGTH_OF_ARRAY(iteration6), iteration6)
	};

	DE_ASSERT(m_iteration < DE_LENGTH_OF_ARRAY(iterations));
	for (int method = 0; method < iterations[m_iteration].methodCount; method++)
		(this->*iterations[m_iteration].methods[method])();

	m_iteration++;

	if (m_iteration < DE_LENGTH_OF_ARRAY(iterations))
	{
		return CONTINUE;
	}
	else
	{
		m_state->results.setTestContextResult(m_testCtx);
		return STOP;
	}
}

class CopyImageTests : public TestCaseGroup
{
public:
						CopyImageTests			(Context& context);
						~CopyImageTests			(void);

	void				init					(void);

private:
						CopyImageTests			(const CopyImageTests& other);
	CopyImageTests&		operator=				(const CopyImageTests& other);
};

CopyImageTests::CopyImageTests (Context& context)
	: TestCaseGroup	(context, "copy_image", "Copy image tests for GL_EXT_copy_image.")
{
}

CopyImageTests::~CopyImageTests (void)
{
}

int smallestCommonMultiple (int a_, int b_)
{
	int	a		= (a_ > b_ ? a_ : b_);
	int	b		= (a_ > b_ ? b_ : a_);
	int	result  = 1;

	for (int i = b/2; i > 1; i--)
	{
		while ((a % i) == 0 && (b % i) == 0)
		{
			result *= i;
			a /= i;
			b /= i;
		}
	}

	return result * a * b;
}

IVec3 getTestedSize (deUint32 target, deUint32 format, const IVec3& targetSize)
{
	const IVec3 texelBlockPixelSize = getTexelBlockPixelSize(format);
	const bool	isCube				= target == GL_TEXTURE_CUBE_MAP;
	const bool	is3D				= target == GL_TEXTURE_3D || target == GL_TEXTURE_2D_ARRAY;

	if (isCube)
	{
		const int	multiplier	= smallestCommonMultiple(texelBlockPixelSize.x(), texelBlockPixelSize.y());
		const int	size		= (1 + (targetSize.x() / multiplier)) * multiplier;

		return IVec3(size, size, 1);
	}
	else if (is3D)
	{
		return (1 + (targetSize / texelBlockPixelSize)) * texelBlockPixelSize;
	}
	else
	{
		const int width = (1 + targetSize.x() / texelBlockPixelSize.x()) * texelBlockPixelSize.x();
		const int height = ((targetSize.y() / texelBlockPixelSize.y()) - 1) * texelBlockPixelSize.y();

		return IVec3(width, height, 1);
	}
}

void addCopyTests (TestCaseGroup* root, deUint32 srcFormat, deUint32 dstFormat)
{
	const string			groupName	= string(formatToName(srcFormat)) + "_" + formatToName(dstFormat);
	TestCaseGroup* const	group		= new TestCaseGroup(root->getContext(), groupName.c_str(), groupName.c_str());

	const deUint32 targets[] =
	{
		GL_TEXTURE_2D,
		GL_TEXTURE_3D,
		GL_TEXTURE_CUBE_MAP,
		GL_TEXTURE_2D_ARRAY,
		GL_RENDERBUFFER
	};

	root->addChild(group);

	for (int srcTargetNdx = 0; srcTargetNdx < DE_LENGTH_OF_ARRAY(targets); srcTargetNdx++)
	{
		const deUint32	srcTarget				= targets[srcTargetNdx];
		const bool		srcIs3D					= srcTarget == GL_TEXTURE_2D_ARRAY || srcTarget == GL_TEXTURE_3D;

		if (glu::isCompressedFormat(srcFormat) && srcTarget == GL_RENDERBUFFER)
			continue;

		if (srcTarget == GL_RENDERBUFFER && !isColorRenderable(srcFormat))
			continue;

		if (glu::isCompressedFormat(srcFormat) && !tcu::isAstcFormat(glu::mapGLCompressedTexFormat(srcFormat)) && srcIs3D)
			continue;

		for (int dstTargetNdx = 0; dstTargetNdx < DE_LENGTH_OF_ARRAY(targets); dstTargetNdx++)
		{
			const deUint32	dstTarget				= targets[dstTargetNdx];
			const bool		dstIs3D					= dstTarget == GL_TEXTURE_2D_ARRAY || dstTarget == GL_TEXTURE_3D;

			if (glu::isCompressedFormat(dstFormat) && dstTarget == GL_RENDERBUFFER)
				continue;

			if (dstTarget == GL_RENDERBUFFER && !isColorRenderable(dstFormat))
				continue;

			if (glu::isCompressedFormat(dstFormat) && !tcu::isAstcFormat(glu::mapGLCompressedTexFormat(dstFormat)) && dstIs3D)
				continue;

			const string	targetTestName	= string(targetToName(srcTarget)) + "_to_" + targetToName(dstTarget);

			// Compressed formats require more space to fit all block size combinations.
			const bool		isCompressedCase	= glu::isCompressedFormat(srcFormat) || glu::isCompressedFormat(dstFormat);
			const IVec3		targetSize			= isCompressedCase ? IVec3(128, 128, 16) : IVec3(64, 64, 8);
			const IVec3		srcSize				= getTestedSize(srcTarget, srcFormat, targetSize);
			const IVec3		dstSize				= getTestedSize(dstTarget, dstFormat, targetSize);

			group->addChild(new CopyImageTest(root->getContext(),
											ImageInfo(srcFormat, srcTarget, srcSize),
											ImageInfo(dstFormat, dstTarget, dstSize),
											targetTestName.c_str(), targetTestName.c_str()));
		}
	}
}

void CopyImageTests::init (void)
{
	TestCaseGroup* const	nonCompressedGroup	= new TestCaseGroup(m_context, "non_compressed", "Test copying between textures.");
	TestCaseGroup* const	compressedGroup		= new TestCaseGroup(m_context, "compressed", "Test copying between compressed textures.");
	TestCaseGroup* const	mixedGroup			= new TestCaseGroup(m_context, "mixed", "Test copying between compressed and non-compressed textures.");

	addChild(nonCompressedGroup);
	addChild(compressedGroup);
	addChild(mixedGroup);

	map<ViewClass, vector<deUint32> >							textureFormatViewClasses;
	map<ViewClass, vector<deUint32> >							compressedTextureFormatViewClasses;
	map<ViewClass, pair<vector<deUint32>, vector<deUint32> > >	mixedViewClasses;

	// Texture view classes
	textureFormatViewClasses[VIEWCLASS_128_BITS]		= vector<deUint32>();
	textureFormatViewClasses[VIEWCLASS_96_BITS]			= vector<deUint32>();
	textureFormatViewClasses[VIEWCLASS_64_BITS]			= vector<deUint32>();
	textureFormatViewClasses[VIEWCLASS_48_BITS]			= vector<deUint32>();
	textureFormatViewClasses[VIEWCLASS_32_BITS]			= vector<deUint32>();
	textureFormatViewClasses[VIEWCLASS_24_BITS]			= vector<deUint32>();
	textureFormatViewClasses[VIEWCLASS_16_BITS]			= vector<deUint32>();
	textureFormatViewClasses[VIEWCLASS_8_BITS]			= vector<deUint32>();

	// 128bit / VIEWCLASS_128_BITS
	textureFormatViewClasses[VIEWCLASS_128_BITS].push_back(GL_RGBA32F);
	textureFormatViewClasses[VIEWCLASS_128_BITS].push_back(GL_RGBA32I);
	textureFormatViewClasses[VIEWCLASS_128_BITS].push_back(GL_RGBA32UI);

	// 96bit / VIEWCLASS_96_BITS
	textureFormatViewClasses[VIEWCLASS_96_BITS].push_back(GL_RGB32F);
	textureFormatViewClasses[VIEWCLASS_96_BITS].push_back(GL_RGB32I);
	textureFormatViewClasses[VIEWCLASS_96_BITS].push_back(GL_RGB32UI);

	// 64bit / VIEWCLASS_64_BITS
	textureFormatViewClasses[VIEWCLASS_64_BITS].push_back(GL_RG32F);
	textureFormatViewClasses[VIEWCLASS_64_BITS].push_back(GL_RG32I);
	textureFormatViewClasses[VIEWCLASS_64_BITS].push_back(GL_RG32UI);

	textureFormatViewClasses[VIEWCLASS_64_BITS].push_back(GL_RGBA16F);
	textureFormatViewClasses[VIEWCLASS_64_BITS].push_back(GL_RGBA16I);
	textureFormatViewClasses[VIEWCLASS_64_BITS].push_back(GL_RGBA16UI);

	// 48bit / VIEWCLASS_48_BITS
	textureFormatViewClasses[VIEWCLASS_48_BITS].push_back(GL_RGB16F);
	textureFormatViewClasses[VIEWCLASS_48_BITS].push_back(GL_RGB16I);
	textureFormatViewClasses[VIEWCLASS_48_BITS].push_back(GL_RGB16UI);

	// 32bit / VIEWCLASS_32_BITS
	textureFormatViewClasses[VIEWCLASS_32_BITS].push_back(GL_R32F);
	textureFormatViewClasses[VIEWCLASS_32_BITS].push_back(GL_R32I);
	textureFormatViewClasses[VIEWCLASS_32_BITS].push_back(GL_R32UI);

	textureFormatViewClasses[VIEWCLASS_32_BITS].push_back(GL_RG16F);
	textureFormatViewClasses[VIEWCLASS_32_BITS].push_back(GL_RG16I);
	textureFormatViewClasses[VIEWCLASS_32_BITS].push_back(GL_RG16UI);

	textureFormatViewClasses[VIEWCLASS_32_BITS].push_back(GL_RGBA8);
	textureFormatViewClasses[VIEWCLASS_32_BITS].push_back(GL_RGBA8I);
	textureFormatViewClasses[VIEWCLASS_32_BITS].push_back(GL_RGBA8UI);

	textureFormatViewClasses[VIEWCLASS_32_BITS].push_back(GL_R11F_G11F_B10F);
	textureFormatViewClasses[VIEWCLASS_32_BITS].push_back(GL_RGB10_A2UI);
	textureFormatViewClasses[VIEWCLASS_32_BITS].push_back(GL_RGB10_A2);
	textureFormatViewClasses[VIEWCLASS_32_BITS].push_back(GL_RGBA8_SNORM);
	textureFormatViewClasses[VIEWCLASS_32_BITS].push_back(GL_SRGB8_ALPHA8);
	textureFormatViewClasses[VIEWCLASS_32_BITS].push_back(GL_RGB9_E5);

	// 24bit / VIEWCLASS_24_BITS
	textureFormatViewClasses[VIEWCLASS_24_BITS].push_back(GL_RGB8);
	textureFormatViewClasses[VIEWCLASS_24_BITS].push_back(GL_RGB8I);
	textureFormatViewClasses[VIEWCLASS_24_BITS].push_back(GL_RGB8UI);
	textureFormatViewClasses[VIEWCLASS_24_BITS].push_back(GL_RGB8_SNORM);
	textureFormatViewClasses[VIEWCLASS_24_BITS].push_back(GL_SRGB8);

	// 16bit / VIEWCLASS_16_BITS
	textureFormatViewClasses[VIEWCLASS_16_BITS].push_back(GL_R16F);
	textureFormatViewClasses[VIEWCLASS_16_BITS].push_back(GL_R16I);
	textureFormatViewClasses[VIEWCLASS_16_BITS].push_back(GL_R16UI);

	textureFormatViewClasses[VIEWCLASS_16_BITS].push_back(GL_RG8);
	textureFormatViewClasses[VIEWCLASS_16_BITS].push_back(GL_RG8I);
	textureFormatViewClasses[VIEWCLASS_16_BITS].push_back(GL_RG8UI);
	textureFormatViewClasses[VIEWCLASS_16_BITS].push_back(GL_RG8_SNORM);

	// 8bit / VIEWCLASS_8_BITS
	textureFormatViewClasses[VIEWCLASS_8_BITS].push_back(GL_R8);
	textureFormatViewClasses[VIEWCLASS_8_BITS].push_back(GL_R8I);
	textureFormatViewClasses[VIEWCLASS_8_BITS].push_back(GL_R8UI);
	textureFormatViewClasses[VIEWCLASS_8_BITS].push_back(GL_R8_SNORM);

	// Compressed texture view classes
	compressedTextureFormatViewClasses[VIEWCLASS_EAC_R11]			= vector<deUint32>();
	compressedTextureFormatViewClasses[VIEWCLASS_EAC_RG11]			= vector<deUint32>();
	compressedTextureFormatViewClasses[VIEWCLASS_ETC2_RGB]			= vector<deUint32>();
	compressedTextureFormatViewClasses[VIEWCLASS_ETC2_RGBA]			= vector<deUint32>();
	compressedTextureFormatViewClasses[VIEWCLASS_ETC2_EAC_RGBA]		= vector<deUint32>();
	compressedTextureFormatViewClasses[VIEWCLASS_ASTC_4x4_RGBA]		= vector<deUint32>();
	compressedTextureFormatViewClasses[VIEWCLASS_ASTC_5x4_RGBA]		= vector<deUint32>();
	compressedTextureFormatViewClasses[VIEWCLASS_ASTC_5x5_RGBA]		= vector<deUint32>();
	compressedTextureFormatViewClasses[VIEWCLASS_ASTC_6x5_RGBA]		= vector<deUint32>();
	compressedTextureFormatViewClasses[VIEWCLASS_ASTC_6x6_RGBA]		= vector<deUint32>();
	compressedTextureFormatViewClasses[VIEWCLASS_ASTC_8x5_RGBA]		= vector<deUint32>();
	compressedTextureFormatViewClasses[VIEWCLASS_ASTC_8x6_RGBA]		= vector<deUint32>();
	compressedTextureFormatViewClasses[VIEWCLASS_ASTC_8x8_RGBA]		= vector<deUint32>();
	compressedTextureFormatViewClasses[VIEWCLASS_ASTC_10x5_RGBA]	= vector<deUint32>();
	compressedTextureFormatViewClasses[VIEWCLASS_ASTC_10x6_RGBA]	= vector<deUint32>();
	compressedTextureFormatViewClasses[VIEWCLASS_ASTC_10x8_RGBA]	= vector<deUint32>();
	compressedTextureFormatViewClasses[VIEWCLASS_ASTC_10x10_RGBA]	= vector<deUint32>();
	compressedTextureFormatViewClasses[VIEWCLASS_ASTC_12x10_RGBA]	= vector<deUint32>();
	compressedTextureFormatViewClasses[VIEWCLASS_ASTC_12x12_RGBA]	= vector<deUint32>();

	// VIEWCLASS_EAC_R11
	compressedTextureFormatViewClasses[VIEWCLASS_EAC_R11].push_back(GL_COMPRESSED_R11_EAC);
	compressedTextureFormatViewClasses[VIEWCLASS_EAC_R11].push_back(GL_COMPRESSED_SIGNED_R11_EAC);

	// VIEWCLASS_EAC_RG11
	compressedTextureFormatViewClasses[VIEWCLASS_EAC_RG11].push_back(GL_COMPRESSED_RG11_EAC);
	compressedTextureFormatViewClasses[VIEWCLASS_EAC_RG11].push_back(GL_COMPRESSED_SIGNED_RG11_EAC);

	// VIEWCLASS_ETC2_RGB
	compressedTextureFormatViewClasses[VIEWCLASS_ETC2_RGB].push_back(GL_COMPRESSED_RGB8_ETC2);
	compressedTextureFormatViewClasses[VIEWCLASS_ETC2_RGB].push_back(GL_COMPRESSED_SRGB8_ETC2);

	// VIEWCLASS_ETC2_RGBA
	compressedTextureFormatViewClasses[VIEWCLASS_ETC2_RGBA].push_back(GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2);
	compressedTextureFormatViewClasses[VIEWCLASS_ETC2_RGBA].push_back(GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2);

	// VIEWCLASS_ETC2_EAC_RGBA
	compressedTextureFormatViewClasses[VIEWCLASS_ETC2_EAC_RGBA].push_back(GL_COMPRESSED_RGBA8_ETC2_EAC);
	compressedTextureFormatViewClasses[VIEWCLASS_ETC2_EAC_RGBA].push_back(GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC);

	// VIEWCLASS_ASTC_4x4_RGBA
	compressedTextureFormatViewClasses[VIEWCLASS_ASTC_4x4_RGBA].push_back(GL_COMPRESSED_RGBA_ASTC_4x4);
	compressedTextureFormatViewClasses[VIEWCLASS_ASTC_4x4_RGBA].push_back(GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4);

	// VIEWCLASS_ASTC_5x4_RGBA
	compressedTextureFormatViewClasses[VIEWCLASS_ASTC_5x4_RGBA].push_back(GL_COMPRESSED_RGBA_ASTC_5x4);
	compressedTextureFormatViewClasses[VIEWCLASS_ASTC_5x4_RGBA].push_back(GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x4);

	// VIEWCLASS_ASTC_5x5_RGBA
	compressedTextureFormatViewClasses[VIEWCLASS_ASTC_5x5_RGBA].push_back(GL_COMPRESSED_RGBA_ASTC_5x5);
	compressedTextureFormatViewClasses[VIEWCLASS_ASTC_5x5_RGBA].push_back(GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x5);

	// VIEWCLASS_ASTC_6x5_RGBA
	compressedTextureFormatViewClasses[VIEWCLASS_ASTC_6x5_RGBA].push_back(GL_COMPRESSED_RGBA_ASTC_6x5);
	compressedTextureFormatViewClasses[VIEWCLASS_ASTC_6x5_RGBA].push_back(GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x5);

	// VIEWCLASS_ASTC_6x6_RGBA
	compressedTextureFormatViewClasses[VIEWCLASS_ASTC_6x6_RGBA].push_back(GL_COMPRESSED_RGBA_ASTC_6x6);
	compressedTextureFormatViewClasses[VIEWCLASS_ASTC_6x6_RGBA].push_back(GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x6);

	// VIEWCLASS_ASTC_8x5_RGBA
	compressedTextureFormatViewClasses[VIEWCLASS_ASTC_8x5_RGBA].push_back(GL_COMPRESSED_RGBA_ASTC_8x5);
	compressedTextureFormatViewClasses[VIEWCLASS_ASTC_8x5_RGBA].push_back(GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x5);

	// VIEWCLASS_ASTC_8x6_RGBA
	compressedTextureFormatViewClasses[VIEWCLASS_ASTC_8x6_RGBA].push_back(GL_COMPRESSED_RGBA_ASTC_8x6);
	compressedTextureFormatViewClasses[VIEWCLASS_ASTC_8x6_RGBA].push_back(GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x6);

	// VIEWCLASS_ASTC_8x8_RGBA
	compressedTextureFormatViewClasses[VIEWCLASS_ASTC_8x8_RGBA].push_back(GL_COMPRESSED_RGBA_ASTC_8x8);
	compressedTextureFormatViewClasses[VIEWCLASS_ASTC_8x8_RGBA].push_back(GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x8);

	// VIEWCLASS_ASTC_10x5_RGBA
	compressedTextureFormatViewClasses[VIEWCLASS_ASTC_10x5_RGBA].push_back(GL_COMPRESSED_RGBA_ASTC_10x5);
	compressedTextureFormatViewClasses[VIEWCLASS_ASTC_10x5_RGBA].push_back(GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x5);

	// VIEWCLASS_ASTC_10x6_RGBA
	compressedTextureFormatViewClasses[VIEWCLASS_ASTC_10x6_RGBA].push_back(GL_COMPRESSED_RGBA_ASTC_10x6);
	compressedTextureFormatViewClasses[VIEWCLASS_ASTC_10x6_RGBA].push_back(GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x6);

	// VIEWCLASS_ASTC_10x8_RGBA
	compressedTextureFormatViewClasses[VIEWCLASS_ASTC_10x8_RGBA].push_back(GL_COMPRESSED_RGBA_ASTC_10x8);
	compressedTextureFormatViewClasses[VIEWCLASS_ASTC_10x8_RGBA].push_back(GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x8);

	// VIEWCLASS_ASTC_10x10_RGBA
	compressedTextureFormatViewClasses[VIEWCLASS_ASTC_10x10_RGBA].push_back(GL_COMPRESSED_RGBA_ASTC_10x10);
	compressedTextureFormatViewClasses[VIEWCLASS_ASTC_10x10_RGBA].push_back(GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x10);

	// VIEWCLASS_ASTC_12x10_RGBA
	compressedTextureFormatViewClasses[VIEWCLASS_ASTC_12x10_RGBA].push_back(GL_COMPRESSED_RGBA_ASTC_12x10);
	compressedTextureFormatViewClasses[VIEWCLASS_ASTC_12x10_RGBA].push_back(GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x10);

	// VIEWCLASS_ASTC_12x12_RGBA
	compressedTextureFormatViewClasses[VIEWCLASS_ASTC_12x12_RGBA].push_back(GL_COMPRESSED_RGBA_ASTC_12x12);
	compressedTextureFormatViewClasses[VIEWCLASS_ASTC_12x12_RGBA].push_back(GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x12);

	// Mixed view classes
	mixedViewClasses[VIEWCLASS_128_BITS] = pair<vector<deUint32>, vector<deUint32> >();
	mixedViewClasses[VIEWCLASS_64_BITS] = pair<vector<deUint32>, vector<deUint32> >();

	// 128 bits

	// Non compressed
	mixedViewClasses[VIEWCLASS_128_BITS].first.push_back(GL_RGBA32F);
	mixedViewClasses[VIEWCLASS_128_BITS].first.push_back(GL_RGBA32UI);
	mixedViewClasses[VIEWCLASS_128_BITS].first.push_back(GL_RGBA32I);

	// Compressed
	mixedViewClasses[VIEWCLASS_128_BITS].second.push_back(GL_COMPRESSED_RGBA8_ETC2_EAC);
	mixedViewClasses[VIEWCLASS_128_BITS].second.push_back(GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC);
	mixedViewClasses[VIEWCLASS_128_BITS].second.push_back(GL_COMPRESSED_RG11_EAC);
	mixedViewClasses[VIEWCLASS_128_BITS].second.push_back(GL_COMPRESSED_SIGNED_RG11_EAC);
	mixedViewClasses[VIEWCLASS_128_BITS].second.push_back(GL_COMPRESSED_RGBA_ASTC_4x4);
	mixedViewClasses[VIEWCLASS_128_BITS].second.push_back(GL_COMPRESSED_RGBA_ASTC_5x4);
	mixedViewClasses[VIEWCLASS_128_BITS].second.push_back(GL_COMPRESSED_RGBA_ASTC_5x5);
	mixedViewClasses[VIEWCLASS_128_BITS].second.push_back(GL_COMPRESSED_RGBA_ASTC_6x5);
	mixedViewClasses[VIEWCLASS_128_BITS].second.push_back(GL_COMPRESSED_RGBA_ASTC_6x6);
	mixedViewClasses[VIEWCLASS_128_BITS].second.push_back(GL_COMPRESSED_RGBA_ASTC_8x5);
	mixedViewClasses[VIEWCLASS_128_BITS].second.push_back(GL_COMPRESSED_RGBA_ASTC_8x6);
	mixedViewClasses[VIEWCLASS_128_BITS].second.push_back(GL_COMPRESSED_RGBA_ASTC_8x8);
	mixedViewClasses[VIEWCLASS_128_BITS].second.push_back(GL_COMPRESSED_RGBA_ASTC_10x5);
	mixedViewClasses[VIEWCLASS_128_BITS].second.push_back(GL_COMPRESSED_RGBA_ASTC_10x6);
	mixedViewClasses[VIEWCLASS_128_BITS].second.push_back(GL_COMPRESSED_RGBA_ASTC_10x8);
	mixedViewClasses[VIEWCLASS_128_BITS].second.push_back(GL_COMPRESSED_RGBA_ASTC_10x10);
	mixedViewClasses[VIEWCLASS_128_BITS].second.push_back(GL_COMPRESSED_RGBA_ASTC_12x10);
	mixedViewClasses[VIEWCLASS_128_BITS].second.push_back(GL_COMPRESSED_RGBA_ASTC_12x12);
	mixedViewClasses[VIEWCLASS_128_BITS].second.push_back(GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4);
	mixedViewClasses[VIEWCLASS_128_BITS].second.push_back(GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x4);
	mixedViewClasses[VIEWCLASS_128_BITS].second.push_back(GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x5);
	mixedViewClasses[VIEWCLASS_128_BITS].second.push_back(GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x5);
	mixedViewClasses[VIEWCLASS_128_BITS].second.push_back(GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x6);
	mixedViewClasses[VIEWCLASS_128_BITS].second.push_back(GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x5);
	mixedViewClasses[VIEWCLASS_128_BITS].second.push_back(GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x6);
	mixedViewClasses[VIEWCLASS_128_BITS].second.push_back(GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x8);
	mixedViewClasses[VIEWCLASS_128_BITS].second.push_back(GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x5);
	mixedViewClasses[VIEWCLASS_128_BITS].second.push_back(GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x6);
	mixedViewClasses[VIEWCLASS_128_BITS].second.push_back(GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x8);
	mixedViewClasses[VIEWCLASS_128_BITS].second.push_back(GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x10);
	mixedViewClasses[VIEWCLASS_128_BITS].second.push_back(GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x10);
	mixedViewClasses[VIEWCLASS_128_BITS].second.push_back(GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x12);

	// 64 bits

	// Non compressed
	mixedViewClasses[VIEWCLASS_64_BITS].first.push_back(GL_RGBA16F);
	mixedViewClasses[VIEWCLASS_64_BITS].first.push_back(GL_RGBA16UI);
	mixedViewClasses[VIEWCLASS_64_BITS].first.push_back(GL_RGBA16I);

	mixedViewClasses[VIEWCLASS_64_BITS].first.push_back(GL_RG32F);
	mixedViewClasses[VIEWCLASS_64_BITS].first.push_back(GL_RG32UI);
	mixedViewClasses[VIEWCLASS_64_BITS].first.push_back(GL_RG32I);

	// Compressed
	mixedViewClasses[VIEWCLASS_64_BITS].second.push_back(GL_COMPRESSED_R11_EAC);
	mixedViewClasses[VIEWCLASS_64_BITS].second.push_back(GL_COMPRESSED_SIGNED_R11_EAC);

	for (map<ViewClass, vector<deUint32> >::const_iterator viewClassIter = textureFormatViewClasses.begin(); viewClassIter != textureFormatViewClasses.end(); ++viewClassIter)
	{
		const vector<deUint32>&	formats		= viewClassIter->second;
		const ViewClass			viewClass	= viewClassIter->first;
		TestCaseGroup* const	viewGroup	= new TestCaseGroup(m_context, viewClassToName(viewClass), viewClassToName(viewClass));

		nonCompressedGroup->addChild(viewGroup);

		for (int srcFormatNdx = 0; srcFormatNdx < (int)formats.size(); srcFormatNdx++)
		for (int dstFormatNdx = 0; dstFormatNdx < (int)formats.size(); dstFormatNdx++)
		{
			const deUint32 srcFormat = formats[srcFormatNdx];
			const deUint32 dstFormat = formats[dstFormatNdx];

			if (srcFormat != dstFormat && isFloatFormat(srcFormat) && isFloatFormat(dstFormat))
				continue;

			addCopyTests(viewGroup, srcFormat, dstFormat);
		}
	}

	for (map<ViewClass, vector<deUint32> >::const_iterator viewClassIter = compressedTextureFormatViewClasses.begin(); viewClassIter != compressedTextureFormatViewClasses.end(); ++viewClassIter)
	{
		const vector<deUint32>&	formats		= viewClassIter->second;
		const ViewClass			viewClass	= viewClassIter->first;
		TestCaseGroup* const	viewGroup	= new TestCaseGroup(m_context, viewClassToName(viewClass), viewClassToName(viewClass));

		compressedGroup->addChild(viewGroup);

		for (int srcFormatNdx = 0; srcFormatNdx < (int)formats.size(); srcFormatNdx++)
		for (int dstFormatNdx = 0; dstFormatNdx < (int)formats.size(); dstFormatNdx++)
		{
			const deUint32 srcFormat = formats[srcFormatNdx];
			const deUint32 dstFormat = formats[dstFormatNdx];

			if (srcFormat != dstFormat && isFloatFormat(srcFormat) && isFloatFormat(dstFormat))
				continue;

			addCopyTests(viewGroup, srcFormat, dstFormat);
		}
	}

	for (map<ViewClass, pair<vector<deUint32>, vector<deUint32> > >::const_iterator iter = mixedViewClasses.begin(); iter != mixedViewClasses.end(); ++iter)
	{
		const ViewClass			viewClass				= iter->first;
		const string			viewClassName			= string(viewClassToName(viewClass)) + "_mixed";
		TestCaseGroup* const	viewGroup				= new TestCaseGroup(m_context, viewClassName.c_str(), viewClassName.c_str());

		const vector<deUint32>	nonCompressedFormats	= iter->second.first;
		const vector<deUint32>	compressedFormats		= iter->second.second;

		mixedGroup->addChild(viewGroup);

		for (int srcFormatNdx = 0; srcFormatNdx < (int)nonCompressedFormats.size(); srcFormatNdx++)
		for (int dstFormatNdx = 0; dstFormatNdx < (int)compressedFormats.size(); dstFormatNdx++)
		{
			const deUint32 srcFormat = nonCompressedFormats[srcFormatNdx];
			const deUint32 dstFormat = compressedFormats[dstFormatNdx];

			if (srcFormat != dstFormat && isFloatFormat(srcFormat) && isFloatFormat(dstFormat))
				continue;

			addCopyTests(viewGroup, srcFormat, dstFormat);
			addCopyTests(viewGroup, dstFormat, srcFormat);
		}
	}
}

} // anonymous

TestCaseGroup* createCopyImageTests (Context& context)
{
	return new CopyImageTests(context);
}

} // Functional
} // gles31
} // deqp
