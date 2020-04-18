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
 * \brief GLSL textureGather[Offset[s]] tests.
 *//*--------------------------------------------------------------------*/

#include "es31fTextureGatherTests.hpp"
#include "glsTextureTestUtil.hpp"
#include "gluShaderProgram.hpp"
#include "gluTexture.hpp"
#include "gluDrawUtil.hpp"
#include "gluPixelTransfer.hpp"
#include "gluTextureUtil.hpp"
#include "gluStrUtil.hpp"
#include "gluObjectWrapper.hpp"
#include "tcuTextureUtil.hpp"
#include "tcuStringTemplate.hpp"
#include "tcuSurface.hpp"
#include "tcuTestLog.hpp"
#include "tcuVectorUtil.hpp"
#include "tcuTexLookupVerifier.hpp"
#include "tcuTexCompareVerifier.hpp"
#include "tcuCommandLine.hpp"
#include "deUniquePtr.hpp"
#include "deStringUtil.hpp"
#include "deRandom.hpp"
#include "deString.h"

#include "glwEnums.hpp"
#include "glwFunctions.hpp"

using glu::ShaderProgram;
using tcu::ConstPixelBufferAccess;
using tcu::PixelBufferAccess;
using tcu::TestLog;
using tcu::IVec2;
using tcu::IVec3;
using tcu::IVec4;
using tcu::UVec4;
using tcu::Vec2;
using tcu::Vec3;
using tcu::Vec4;
using de::MovePtr;

using std::string;
using std::vector;

namespace deqp
{

using glu::TextureTestUtil::TextureType;
using glu::TextureTestUtil::TEXTURETYPE_2D;
using glu::TextureTestUtil::TEXTURETYPE_2D_ARRAY;
using glu::TextureTestUtil::TEXTURETYPE_CUBE;

namespace gles31
{
namespace Functional
{

namespace
{

static std::string specializeShader(Context& context, const char* code)
{
	const glu::GLSLVersion				glslVersion			= glu::getContextTypeGLSLVersion(context.getRenderContext().getType());
	std::map<std::string, std::string>	specializationMap;

	specializationMap["GLSL_VERSION_DECL"] = glu::getGLSLVersionDeclaration(glslVersion);

	if (glu::contextSupports(context.getRenderContext().getType(), glu::ApiType::es(3, 2)))
		specializationMap["GPU_SHADER5_REQUIRE"] = "";
	else
		specializationMap["GPU_SHADER5_REQUIRE"] = "#extension GL_EXT_gpu_shader5 : require";

	return tcu::StringTemplate(code).specialize(specializationMap);
}

// Round-to-zero int division, because pre-c++11 it's somewhat implementation-defined for negative values.
static inline int divRoundToZero (int a, int b)
{
	return de::abs(a) / de::abs(b) * deSign32(a) * deSign32(b);
}

static void fillWithRandomColorTiles (const PixelBufferAccess& dst, const Vec4& minVal, const Vec4& maxVal, deUint32 seed)
{
	const int	numCols		= dst.getWidth()  >= 7 ? 7 : dst.getWidth();
	const int	numRows		= dst.getHeight() >= 5 ? 5 : dst.getHeight();
	de::Random	rnd			(seed);

	for (int slice = 0; slice < dst.getDepth(); slice++)
	for (int row = 0; row < numRows; row++)
	for (int col = 0; col < numCols; col++)
	{
		const int	yBegin	= (row+0)*dst.getHeight()/numRows;
		const int	yEnd	= (row+1)*dst.getHeight()/numRows;
		const int	xBegin	= (col+0)*dst.getWidth()/numCols;
		const int	xEnd	= (col+1)*dst.getWidth()/numCols;
		Vec4		color;
		for (int i = 0; i < 4; i++)
			color[i] = rnd.getFloat(minVal[i], maxVal[i]);
		tcu::clear(tcu::getSubregion(dst, xBegin, yBegin, slice, xEnd-xBegin, yEnd-yBegin, 1), color);
	}
}

static inline bool isDepthFormat (const tcu::TextureFormat& fmt)
{
	return fmt.order == tcu::TextureFormat::D || fmt.order == tcu::TextureFormat::DS;
}

static inline bool isUnormFormatType (tcu::TextureFormat::ChannelType type)
{
	return type == tcu::TextureFormat::UNORM_INT8	||
		   type == tcu::TextureFormat::UNORM_INT16	||
		   type == tcu::TextureFormat::UNORM_INT32;
}

static inline bool isSIntFormatType (tcu::TextureFormat::ChannelType type)
{
	return type == tcu::TextureFormat::SIGNED_INT8	||
		   type == tcu::TextureFormat::SIGNED_INT16	||
		   type == tcu::TextureFormat::SIGNED_INT32;
}

static inline bool isUIntFormatType (tcu::TextureFormat::ChannelType type)
{
	return type == tcu::TextureFormat::UNSIGNED_INT8	||
		   type == tcu::TextureFormat::UNSIGNED_INT16	||
		   type == tcu::TextureFormat::UNSIGNED_INT32;
}

static tcu::TextureLevel getPixels (const glu::RenderContext& renderCtx, const IVec2& size, const tcu::TextureFormat& colorBufferFormat)
{
	tcu::TextureLevel result(colorBufferFormat, size.x(), size.y());

	// only a few pixel formats are guaranteed to be valid targets for readPixels, convert the rest
	if (colorBufferFormat.order == tcu::TextureFormat::RGBA &&
		(colorBufferFormat.type == tcu::TextureFormat::UNORM_INT8	||
		 colorBufferFormat.type == tcu::TextureFormat::SIGNED_INT32	||
		 colorBufferFormat.type == tcu::TextureFormat::UNSIGNED_INT32))
	{
		// valid as is
		glu::readPixels(renderCtx, 0, 0, result.getAccess());
	}
	else if (colorBufferFormat.order == tcu::TextureFormat::RGBA &&
			 (isSIntFormatType(colorBufferFormat.type) ||
			  isUIntFormatType(colorBufferFormat.type)))
	{
		// signed and unsigned integers must be read using 32-bit values
		const bool			isSigned	= isSIntFormatType(colorBufferFormat.type);
		tcu::TextureLevel	readResult	(tcu::TextureFormat(tcu::TextureFormat::RGBA,
														    (isSigned) ? (tcu::TextureFormat::SIGNED_INT32) : (tcu::TextureFormat::UNSIGNED_INT32)),
										 size.x(),
										 size.y());

		glu::readPixels(renderCtx, 0, 0, readResult.getAccess());
		tcu::copy(result.getAccess(), readResult.getAccess());
	}
	else
	{
		// unreadable format
		DE_ASSERT(false);
	}

	return result;
}

enum TextureSwizzleComponent
{
	TEXTURESWIZZLECOMPONENT_R = 0,
	TEXTURESWIZZLECOMPONENT_G,
	TEXTURESWIZZLECOMPONENT_B,
	TEXTURESWIZZLECOMPONENT_A,
	TEXTURESWIZZLECOMPONENT_ZERO,
	TEXTURESWIZZLECOMPONENT_ONE,

	TEXTURESWIZZLECOMPONENT_LAST
};

static std::ostream& operator<< (std::ostream& stream, TextureSwizzleComponent comp)
{
	switch (comp)
	{
		case TEXTURESWIZZLECOMPONENT_R:		return stream << "RED";
		case TEXTURESWIZZLECOMPONENT_G:		return stream << "GREEN";
		case TEXTURESWIZZLECOMPONENT_B:		return stream << "BLUE";
		case TEXTURESWIZZLECOMPONENT_A:		return stream << "ALPHA";
		case TEXTURESWIZZLECOMPONENT_ZERO:	return stream << "ZERO";
		case TEXTURESWIZZLECOMPONENT_ONE:	return stream << "ONE";
		default: DE_ASSERT(false); return stream;
	}
}

struct MaybeTextureSwizzle
{
public:
	static MaybeTextureSwizzle						createNoneTextureSwizzle	(void);
	static MaybeTextureSwizzle						createSomeTextureSwizzle	(void);

	bool											isSome						(void) const;
	bool											isNone						(void) const;
	bool											isIdentitySwizzle			(void) const;

	tcu::Vector<TextureSwizzleComponent, 4>&		getSwizzle					(void);
	const tcu::Vector<TextureSwizzleComponent, 4>&	getSwizzle					(void) const;

private:
													MaybeTextureSwizzle			(void);

	tcu::Vector<TextureSwizzleComponent, 4>			m_swizzle;
	bool											m_isSome;
};

static std::ostream& operator<< (std::ostream& stream, const MaybeTextureSwizzle& comp)
{
	if (comp.isNone())
		stream << "[default swizzle state]";
	else
		stream << "(" << comp.getSwizzle()[0]
			   << ", " << comp.getSwizzle()[1]
			   << ", " << comp.getSwizzle()[2]
			   << ", " << comp.getSwizzle()[3]
			   << ")";

	return stream;
}

MaybeTextureSwizzle MaybeTextureSwizzle::createNoneTextureSwizzle (void)
{
	MaybeTextureSwizzle swizzle;

	swizzle.m_swizzle[0] = TEXTURESWIZZLECOMPONENT_LAST;
	swizzle.m_swizzle[1] = TEXTURESWIZZLECOMPONENT_LAST;
	swizzle.m_swizzle[2] = TEXTURESWIZZLECOMPONENT_LAST;
	swizzle.m_swizzle[3] = TEXTURESWIZZLECOMPONENT_LAST;
	swizzle.m_isSome = false;

	return swizzle;
}

MaybeTextureSwizzle MaybeTextureSwizzle::createSomeTextureSwizzle (void)
{
	MaybeTextureSwizzle swizzle;

	swizzle.m_swizzle[0] = TEXTURESWIZZLECOMPONENT_R;
	swizzle.m_swizzle[1] = TEXTURESWIZZLECOMPONENT_G;
	swizzle.m_swizzle[2] = TEXTURESWIZZLECOMPONENT_B;
	swizzle.m_swizzle[3] = TEXTURESWIZZLECOMPONENT_A;
	swizzle.m_isSome = true;

	return swizzle;
}

bool MaybeTextureSwizzle::isSome (void) const
{
	return m_isSome;
}

bool MaybeTextureSwizzle::isNone (void) const
{
	return !m_isSome;
}

bool MaybeTextureSwizzle::isIdentitySwizzle (void) const
{
	return	m_isSome									&&
			m_swizzle[0] == TEXTURESWIZZLECOMPONENT_R	&&
			m_swizzle[1] == TEXTURESWIZZLECOMPONENT_G	&&
			m_swizzle[2] == TEXTURESWIZZLECOMPONENT_B	&&
			m_swizzle[3] == TEXTURESWIZZLECOMPONENT_A;
}

tcu::Vector<TextureSwizzleComponent, 4>& MaybeTextureSwizzle::getSwizzle (void)
{
	return m_swizzle;
}

const tcu::Vector<TextureSwizzleComponent, 4>& MaybeTextureSwizzle::getSwizzle (void) const
{
	return m_swizzle;
}

MaybeTextureSwizzle::MaybeTextureSwizzle (void)
	: m_swizzle	(TEXTURESWIZZLECOMPONENT_LAST, TEXTURESWIZZLECOMPONENT_LAST, TEXTURESWIZZLECOMPONENT_LAST, TEXTURESWIZZLECOMPONENT_LAST)
	, m_isSome	(false)
{
}

static deUint32 getGLTextureSwizzleComponent (TextureSwizzleComponent c)
{
	switch (c)
	{
		case TEXTURESWIZZLECOMPONENT_R:		return GL_RED;
		case TEXTURESWIZZLECOMPONENT_G:		return GL_GREEN;
		case TEXTURESWIZZLECOMPONENT_B:		return GL_BLUE;
		case TEXTURESWIZZLECOMPONENT_A:		return GL_ALPHA;
		case TEXTURESWIZZLECOMPONENT_ZERO:	return GL_ZERO;
		case TEXTURESWIZZLECOMPONENT_ONE:	return GL_ONE;
		default: DE_ASSERT(false); return (deUint32)-1;
	}
}

template <typename T>
static inline T swizzleColorChannel (const tcu::Vector<T, 4>& src, TextureSwizzleComponent swizzle)
{
	switch (swizzle)
	{
		case TEXTURESWIZZLECOMPONENT_R:		return src[0];
		case TEXTURESWIZZLECOMPONENT_G:		return src[1];
		case TEXTURESWIZZLECOMPONENT_B:		return src[2];
		case TEXTURESWIZZLECOMPONENT_A:		return src[3];
		case TEXTURESWIZZLECOMPONENT_ZERO:	return (T)0;
		case TEXTURESWIZZLECOMPONENT_ONE:	return (T)1;
		default: DE_ASSERT(false); return (T)-1;
	}
}

template <typename T>
static inline tcu::Vector<T, 4> swizzleColor (const tcu::Vector<T, 4>& src, const MaybeTextureSwizzle& swizzle)
{
	DE_ASSERT(swizzle.isSome());

	tcu::Vector<T, 4> result;
	for (int i = 0; i < 4; i++)
		result[i] = swizzleColorChannel(src, swizzle.getSwizzle()[i]);
	return result;
}

template <typename T>
static void swizzlePixels (const PixelBufferAccess& dst, const ConstPixelBufferAccess& src, const MaybeTextureSwizzle& swizzle)
{
	DE_ASSERT(dst.getWidth()  == src.getWidth()  &&
			  dst.getHeight() == src.getHeight() &&
			  dst.getDepth()  == src.getDepth());
	for (int z = 0; z < src.getDepth(); z++)
	for (int y = 0; y < src.getHeight(); y++)
	for (int x = 0; x < src.getWidth(); x++)
		dst.setPixel(swizzleColor(src.getPixelT<T>(x, y, z), swizzle), x, y, z);
}

static void swizzlePixels (const PixelBufferAccess& dst, const ConstPixelBufferAccess& src, const MaybeTextureSwizzle& swizzle)
{
	if (isDepthFormat(dst.getFormat()))
		DE_ASSERT(swizzle.isNone() || swizzle.isIdentitySwizzle());

	if (swizzle.isNone() || swizzle.isIdentitySwizzle())
		tcu::copy(dst, src);
	else if (isUnormFormatType(dst.getFormat().type))
		swizzlePixels<float>(dst, src, swizzle);
	else if (isUIntFormatType(dst.getFormat().type))
		swizzlePixels<deUint32>(dst, src, swizzle);
	else if (isSIntFormatType(dst.getFormat().type))
		swizzlePixels<deInt32>(dst, src, swizzle);
	else
		DE_ASSERT(false);
}

static void swizzleTexture (tcu::Texture2D& dst, const tcu::Texture2D& src, const MaybeTextureSwizzle& swizzle)
{
	dst = tcu::Texture2D(src.getFormat(), src.getWidth(), src.getHeight());
	for (int levelNdx = 0; levelNdx < src.getNumLevels(); levelNdx++)
	{
		if (src.isLevelEmpty(levelNdx))
			continue;
		dst.allocLevel(levelNdx);
		swizzlePixels(dst.getLevel(levelNdx), src.getLevel(levelNdx), swizzle);
	}
}

static void swizzleTexture (tcu::Texture2DArray& dst, const tcu::Texture2DArray& src, const MaybeTextureSwizzle& swizzle)
{
	dst = tcu::Texture2DArray(src.getFormat(), src.getWidth(), src.getHeight(), src.getNumLayers());
	for (int levelNdx = 0; levelNdx < src.getNumLevels(); levelNdx++)
	{
		if (src.isLevelEmpty(levelNdx))
			continue;
		dst.allocLevel(levelNdx);
		swizzlePixels(dst.getLevel(levelNdx), src.getLevel(levelNdx), swizzle);
	}
}

static void swizzleTexture (tcu::TextureCube& dst, const tcu::TextureCube& src, const MaybeTextureSwizzle& swizzle)
{
	dst = tcu::TextureCube(src.getFormat(), src.getSize());
	for (int faceI = 0; faceI < tcu::CUBEFACE_LAST; faceI++)
	{
		const tcu::CubeFace face = (tcu::CubeFace)faceI;
		for (int levelNdx = 0; levelNdx < src.getNumLevels(); levelNdx++)
		{
			if (src.isLevelEmpty(face, levelNdx))
				continue;
			dst.allocLevel(face, levelNdx);
			swizzlePixels(dst.getLevelFace(levelNdx, face), src.getLevelFace(levelNdx, face), swizzle);
		}
	}
}

static tcu::Texture2DView getOneLevelSubView (const tcu::Texture2DView& view, int level)
{
	return tcu::Texture2DView(1, view.getLevels() + level);
}

static tcu::Texture2DArrayView getOneLevelSubView (const tcu::Texture2DArrayView& view, int level)
{
	return tcu::Texture2DArrayView(1, view.getLevels() + level);
}

static tcu::TextureCubeView getOneLevelSubView (const tcu::TextureCubeView& view, int level)
{
	const tcu::ConstPixelBufferAccess* levels[tcu::CUBEFACE_LAST];

	for (int face = 0; face < tcu::CUBEFACE_LAST; face++)
		levels[face] = view.getFaceLevels((tcu::CubeFace)face) + level;

	return tcu::TextureCubeView(1, levels);
}

class PixelOffsets
{
public:
	virtual void operator() (const IVec2& pixCoord, IVec2 (&dst)[4]) const = 0;
	virtual ~PixelOffsets (void) {}
};

class MultiplePixelOffsets : public PixelOffsets
{
public:
	MultiplePixelOffsets (const IVec2& a,
						  const IVec2& b,
						  const IVec2& c,
						  const IVec2& d)
	{
		m_offsets[0] = a;
		m_offsets[1] = b;
		m_offsets[2] = c;
		m_offsets[3] = d;
	}

	void operator() (const IVec2& /* pixCoord */, IVec2 (&dst)[4]) const
	{
		for (int i = 0; i < DE_LENGTH_OF_ARRAY(dst); i++)
			dst[i] = m_offsets[i];
	}

private:
	IVec2 m_offsets[4];
};

class SinglePixelOffsets : public MultiplePixelOffsets
{
public:
	SinglePixelOffsets (const IVec2& offset)
		: MultiplePixelOffsets(offset + IVec2(0, 1),
							   offset + IVec2(1, 1),
							   offset + IVec2(1, 0),
							   offset + IVec2(0, 0))
	{
	}
};

class DynamicSinglePixelOffsets : public PixelOffsets
{
public:
	DynamicSinglePixelOffsets (const IVec2& offsetRange) : m_offsetRange(offsetRange) {}

	void operator() (const IVec2& pixCoord, IVec2 (&dst)[4]) const
	{
		const int offsetRangeSize = m_offsetRange.y() - m_offsetRange.x() + 1;
		SinglePixelOffsets(tcu::mod(pixCoord.swizzle(1,0), IVec2(offsetRangeSize)) + m_offsetRange.x())(IVec2(), dst);
	}

private:
	IVec2 m_offsetRange;
};

template <typename T>
static inline T triQuadInterpolate (const T (&values)[4], float xFactor, float yFactor)
{
	if (xFactor + yFactor < 1.0f)
		return values[0] + (values[2]-values[0])*xFactor		+ (values[1]-values[0])*yFactor;
	else
		return values[3] + (values[1]-values[3])*(1.0f-xFactor)	+ (values[2]-values[3])*(1.0f-yFactor);
}

template <int N>
static inline void computeTexCoordVecs (const vector<float>& texCoords, tcu::Vector<float, N> (&dst)[4])
{
	DE_ASSERT((int)texCoords.size() == 4*N);
	for (int i = 0; i < 4; i++)
	for (int j = 0; j < N; j++)
		dst[i][j] = texCoords[i*N + j];
}

#if defined(DE_DEBUG)
// Whether offsets correspond to the sample offsets used with plain textureGather().
static inline bool isZeroOffsetOffsets (const IVec2 (&offsets)[4])
{
	IVec2 ref[4];
	SinglePixelOffsets(IVec2(0))(IVec2(), ref);
	return std::equal(DE_ARRAY_BEGIN(offsets),
					  DE_ARRAY_END(offsets),
					  DE_ARRAY_BEGIN(ref));
}
#endif

template <typename ColorScalarType>
static tcu::Vector<ColorScalarType, 4> gatherOffsets (const tcu::Texture2DView& texture, const tcu::Sampler& sampler, const Vec2& coord, int componentNdx, const IVec2 (&offsets)[4])
{
	return texture.gatherOffsets(sampler, coord.x(), coord.y(), componentNdx, offsets).cast<ColorScalarType>();
}

template <typename ColorScalarType>
static tcu::Vector<ColorScalarType, 4> gatherOffsets (const tcu::Texture2DArrayView& texture, const tcu::Sampler& sampler, const Vec3& coord, int componentNdx, const IVec2 (&offsets)[4])
{
	return texture.gatherOffsets(sampler, coord.x(), coord.y(), coord.z(), componentNdx, offsets).cast<ColorScalarType>();
}

template <typename ColorScalarType>
static tcu::Vector<ColorScalarType, 4> gatherOffsets (const tcu::TextureCubeView& texture, const tcu::Sampler& sampler, const Vec3& coord, int componentNdx, const IVec2 (&offsets)[4])
{
	DE_ASSERT(isZeroOffsetOffsets(offsets));
	DE_UNREF(offsets);
	return texture.gather(sampler, coord.x(), coord.y(), coord.z(), componentNdx).cast<ColorScalarType>();
}

static Vec4 gatherOffsetsCompare (const tcu::Texture2DView& texture, const tcu::Sampler& sampler, float refZ, const Vec2& coord, const IVec2 (&offsets)[4])
{
	return texture.gatherOffsetsCompare(sampler, refZ, coord.x(), coord.y(), offsets);
}

static Vec4 gatherOffsetsCompare (const tcu::Texture2DArrayView& texture, const tcu::Sampler& sampler, float refZ, const Vec3& coord, const IVec2 (&offsets)[4])
{
	return texture.gatherOffsetsCompare(sampler, refZ, coord.x(), coord.y(), coord.z(), offsets);
}

static Vec4 gatherOffsetsCompare (const tcu::TextureCubeView& texture, const tcu::Sampler& sampler, float refZ, const Vec3& coord, const IVec2 (&offsets)[4])
{
	DE_ASSERT(isZeroOffsetOffsets(offsets));
	DE_UNREF(offsets);
	return texture.gatherCompare(sampler, refZ, coord.x(), coord.y(), coord.z());
}

template <typename PrecType, typename ColorScalarT>
static bool isGatherOffsetsResultValid (const tcu::TextureCubeView&				texture,
										const tcu::Sampler&						sampler,
										const PrecType&							prec,
										const Vec3&								coord,
										int										componentNdx,
										const IVec2								(&offsets)[4],
										const tcu::Vector<ColorScalarT, 4>&		result)
{
	DE_ASSERT(isZeroOffsetOffsets(offsets));
	DE_UNREF(offsets);
	return tcu::isGatherResultValid(texture, sampler, prec, coord, componentNdx, result);
}

static bool isGatherOffsetsCompareResultValid (const tcu::TextureCubeView&		texture,
											   const tcu::Sampler&				sampler,
											   const tcu::TexComparePrecision&	prec,
											   const Vec3&						coord,
											   const IVec2						(&offsets)[4],
											   float							cmpReference,
											   const Vec4&						result)
{
	DE_ASSERT(isZeroOffsetOffsets(offsets));
	DE_UNREF(offsets);
	return tcu::isGatherCompareResultValid(texture, sampler, prec, coord, cmpReference, result);
}

template <typename ColorScalarType, typename PrecType, typename TexViewT, typename TexCoordT>
static bool verifyGatherOffsets (TestLog&						log,
								 const ConstPixelBufferAccess&	result,
								 const TexViewT&				texture,
								 const TexCoordT				(&texCoords)[4],
								 const tcu::Sampler&			sampler,
								 const PrecType&				lookupPrec,
								 int							componentNdx,
								 const PixelOffsets&			getPixelOffsets)
{
	typedef tcu::Vector<ColorScalarType, 4> ColorVec;

	const int					width			= result.getWidth();
	const int					height			= result.getWidth();
	tcu::TextureLevel			ideal			(result.getFormat(), width, height);
	const PixelBufferAccess		idealAccess		= ideal.getAccess();
	tcu::Surface				errorMask		(width, height);
	bool						success			= true;

	tcu::clear(errorMask.getAccess(), tcu::RGBA::green().toVec());

	for (int py = 0; py < height; py++)
	for (int px = 0; px < width; px++)
	{
		IVec2		offsets[4];
		getPixelOffsets(IVec2(px, py), offsets);

		const Vec2			viewportCoord	= (Vec2((float)px, (float)py) + 0.5f) / Vec2((float)width, (float)height);
		const TexCoordT		texCoord		= triQuadInterpolate(texCoords, viewportCoord.x(), viewportCoord.y());
		const ColorVec		resultPix		= result.getPixelT<ColorScalarType>(px, py);
		const ColorVec		idealPix		= gatherOffsets<ColorScalarType>(texture, sampler, texCoord, componentNdx, offsets);

		idealAccess.setPixel(idealPix, px, py);

		if (tcu::boolAny(tcu::logicalAnd(lookupPrec.colorMask,
										 tcu::greaterThan(tcu::absDiff(resultPix, idealPix),
														  lookupPrec.colorThreshold.template cast<ColorScalarType>()))))
		{
			if (!isGatherOffsetsResultValid(texture, sampler, lookupPrec, texCoord, componentNdx, offsets, resultPix))
			{
				errorMask.setPixel(px, py, tcu::RGBA::red());
				success = false;
			}
		}
	}

	log << TestLog::ImageSet("VerifyResult", "Verification result")
		<< TestLog::Image("Rendered", "Rendered image", result);

	if (!success)
	{
		log << TestLog::Image("Reference", "Ideal reference image", ideal)
			<< TestLog::Image("ErrorMask", "Error mask", errorMask);
	}

	log << TestLog::EndImageSet;

	return success;
}

class PixelCompareRefZ
{
public:
	virtual float operator() (const IVec2& pixCoord) const = 0;
};

class PixelCompareRefZDefault : public PixelCompareRefZ
{
public:
	PixelCompareRefZDefault (const IVec2& renderSize) : m_renderSize(renderSize) {}

	float operator() (const IVec2& pixCoord) const
	{
		return ((float)pixCoord.x() + 0.5f) / (float)m_renderSize.x();
	}

private:
	IVec2 m_renderSize;
};

template <typename TexViewT, typename TexCoordT>
static bool verifyGatherOffsetsCompare (TestLog&							log,
										const ConstPixelBufferAccess&		result,
										const TexViewT&						texture,
										const TexCoordT						(&texCoords)[4],
										const tcu::Sampler&					sampler,
										const tcu::TexComparePrecision&		compPrec,
										const PixelCompareRefZ&				getPixelRefZ,
										const PixelOffsets&					getPixelOffsets)
{
	const int					width			= result.getWidth();
	const int					height			= result.getWidth();
	tcu::Surface				ideal			(width, height);
	const PixelBufferAccess		idealAccess		= ideal.getAccess();
	tcu::Surface				errorMask		(width, height);
	bool						success			= true;

	tcu::clear(errorMask.getAccess(), tcu::RGBA::green().toVec());

	for (int py = 0; py < height; py++)
	for (int px = 0; px < width; px++)
	{
		IVec2		offsets[4];
		getPixelOffsets(IVec2(px, py), offsets);

		const Vec2			viewportCoord	= (Vec2((float)px, (float)py) + 0.5f) / Vec2((float)width, (float)height);
		const TexCoordT		texCoord		= triQuadInterpolate(texCoords, viewportCoord.x(), viewportCoord.y());
		const float			refZ			= getPixelRefZ(IVec2(px, py));
		const Vec4			resultPix		= result.getPixel(px, py);
		const Vec4			idealPix		= gatherOffsetsCompare(texture, sampler, refZ, texCoord, offsets);

		idealAccess.setPixel(idealPix, px, py);

		if (!tcu::boolAll(tcu::equal(resultPix, idealPix)))
		{
			if (!isGatherOffsetsCompareResultValid(texture, sampler, compPrec, texCoord, offsets, refZ, resultPix))
			{
				errorMask.setPixel(px, py, tcu::RGBA::red());
				success = false;
			}
		}
	}

	log << TestLog::ImageSet("VerifyResult", "Verification result")
		<< TestLog::Image("Rendered", "Rendered image", result);

	if (!success)
	{
		log << TestLog::Image("Reference", "Ideal reference image", ideal)
			<< TestLog::Image("ErrorMask", "Error mask", errorMask);
	}

	log << TestLog::EndImageSet;

	return success;
}

static bool verifySingleColored (TestLog& log, const ConstPixelBufferAccess& result, const Vec4& refColor)
{
	const int					width			= result.getWidth();
	const int					height			= result.getWidth();
	tcu::Surface				ideal			(width, height);
	const PixelBufferAccess		idealAccess		= ideal.getAccess();
	tcu::Surface				errorMask		(width, height);
	bool						success			= true;

	tcu::clear(errorMask.getAccess(), tcu::RGBA::green().toVec());
	tcu::clear(idealAccess, refColor);

	for (int py = 0; py < height; py++)
	for (int px = 0; px < width; px++)
	{
		if (result.getPixel(px, py) != refColor)
		{
			errorMask.setPixel(px, py, tcu::RGBA::red());
			success = false;
		}
	}

	log << TestLog::ImageSet("VerifyResult", "Verification result")
		<< TestLog::Image("Rendered", "Rendered image", result);

	if (!success)
	{
		log << TestLog::Image("Reference", "Ideal reference image", ideal)
			<< TestLog::Image("ErrorMask", "Error mask", errorMask);
	}

	log << TestLog::EndImageSet;

	return success;
}

enum GatherType
{
	GATHERTYPE_BASIC = 0,
	GATHERTYPE_OFFSET,
	GATHERTYPE_OFFSET_DYNAMIC,
	GATHERTYPE_OFFSETS,

	GATHERTYPE_LAST
};

enum GatherCaseFlags
{
	GATHERCASE_MIPMAP_INCOMPLETE		= (1<<0),	//!< Excercise special case of sampling mipmap-incomplete texture
	GATHERCASE_DONT_SAMPLE_CUBE_CORNERS	= (1<<1)	//!< For cube map cases: do not sample cube corners
};

static inline const char* gatherTypeName (GatherType type)
{
	switch (type)
	{
		case GATHERTYPE_BASIC:				return "basic";
		case GATHERTYPE_OFFSET:				return "offset";
		case GATHERTYPE_OFFSET_DYNAMIC:		return "offset_dynamic";
		case GATHERTYPE_OFFSETS:			return "offsets";
		default: DE_ASSERT(false); return DE_NULL;
	}
}

static inline const char* gatherTypeDescription (GatherType type)
{
	switch (type)
	{
		case GATHERTYPE_BASIC:				return "textureGather";
		case GATHERTYPE_OFFSET:				return "textureGatherOffset";
		case GATHERTYPE_OFFSET_DYNAMIC:		return "textureGatherOffset with dynamic offsets";
		case GATHERTYPE_OFFSETS:			return "textureGatherOffsets";
		default: DE_ASSERT(false); return DE_NULL;
	}
}

static inline bool requireGpuShader5 (GatherType gatherType)
{
	return gatherType == GATHERTYPE_OFFSET_DYNAMIC || gatherType == GATHERTYPE_OFFSETS;
}

struct GatherArgs
{
	int		componentNdx;	// If negative, implicit component index 0 is used (i.e. the parameter is not given).
	IVec2	offsets[4];		// \note Unless GATHERTYPE_OFFSETS is used, only offsets[0] is relevant; also, for GATHERTYPE_OFFSET_DYNAMIC, none are relevant.

	GatherArgs (void)
		: componentNdx(-1)
	{
		std::fill(DE_ARRAY_BEGIN(offsets), DE_ARRAY_END(offsets), IVec2());
	}

	GatherArgs (int comp,
				const IVec2& off0 = IVec2(),
				const IVec2& off1 = IVec2(),
				const IVec2& off2 = IVec2(),
				const IVec2& off3 = IVec2())
		: componentNdx(comp)
	{
		offsets[0] = off0;
		offsets[1] = off1;
		offsets[2] = off2;
		offsets[3] = off3;
	}
};

static MovePtr<PixelOffsets> makePixelOffsetsFunctor (GatherType gatherType, const GatherArgs& gatherArgs, const IVec2& offsetRange)
{
	if (gatherType == GATHERTYPE_BASIC || gatherType == GATHERTYPE_OFFSET)
	{
		const IVec2 offset = gatherType == GATHERTYPE_BASIC ? IVec2(0) : gatherArgs.offsets[0];
		return MovePtr<PixelOffsets>(new SinglePixelOffsets(offset));
	}
	else if (gatherType == GATHERTYPE_OFFSET_DYNAMIC)
	{
		return MovePtr<PixelOffsets>(new DynamicSinglePixelOffsets(offsetRange));
	}
	else if (gatherType == GATHERTYPE_OFFSETS)
		return MovePtr<PixelOffsets>(new MultiplePixelOffsets(gatherArgs.offsets[0],
															  gatherArgs.offsets[1],
															  gatherArgs.offsets[2],
															  gatherArgs.offsets[3]));
	else
	{
		DE_ASSERT(false);
		return MovePtr<PixelOffsets>(DE_NULL);
	}
}

static inline glu::DataType getSamplerType (TextureType textureType, const tcu::TextureFormat& format)
{
	if (isDepthFormat(format))
	{
		switch (textureType)
		{
			case TEXTURETYPE_2D:		return glu::TYPE_SAMPLER_2D_SHADOW;
			case TEXTURETYPE_2D_ARRAY:	return glu::TYPE_SAMPLER_2D_ARRAY_SHADOW;
			case TEXTURETYPE_CUBE:		return glu::TYPE_SAMPLER_CUBE_SHADOW;
			default: DE_ASSERT(false); return glu::TYPE_LAST;
		}
	}
	else
	{
		switch (textureType)
		{
			case TEXTURETYPE_2D:		return glu::getSampler2DType(format);
			case TEXTURETYPE_2D_ARRAY:	return glu::getSampler2DArrayType(format);
			case TEXTURETYPE_CUBE:		return glu::getSamplerCubeType(format);
			default: DE_ASSERT(false); return glu::TYPE_LAST;
		}
	}
}

static inline glu::DataType getSamplerGatherResultType (glu::DataType samplerType)
{
	switch (samplerType)
	{
		case glu::TYPE_SAMPLER_2D_SHADOW:
		case glu::TYPE_SAMPLER_2D_ARRAY_SHADOW:
		case glu::TYPE_SAMPLER_CUBE_SHADOW:
		case glu::TYPE_SAMPLER_2D:
		case glu::TYPE_SAMPLER_2D_ARRAY:
		case glu::TYPE_SAMPLER_CUBE:
			return glu::TYPE_FLOAT_VEC4;

		case glu::TYPE_INT_SAMPLER_2D:
		case glu::TYPE_INT_SAMPLER_2D_ARRAY:
		case glu::TYPE_INT_SAMPLER_CUBE:
			return glu::TYPE_INT_VEC4;

		case glu::TYPE_UINT_SAMPLER_2D:
		case glu::TYPE_UINT_SAMPLER_2D_ARRAY:
		case glu::TYPE_UINT_SAMPLER_CUBE:
			return glu::TYPE_UINT_VEC4;

		default:
			DE_ASSERT(false);
			return glu::TYPE_LAST;
	}
}

static inline int getNumTextureSamplingDimensions (TextureType type)
{
	switch (type)
	{
		case TEXTURETYPE_2D:		return 2;
		case TEXTURETYPE_2D_ARRAY:	return 3;
		case TEXTURETYPE_CUBE:		return 3;
		default: DE_ASSERT(false); return -1;
	}
}

static deUint32 getGLTextureType (TextureType type)
{
	switch (type)
	{
		case TEXTURETYPE_2D:		return GL_TEXTURE_2D;
		case TEXTURETYPE_2D_ARRAY:	return GL_TEXTURE_2D_ARRAY;
		case TEXTURETYPE_CUBE:		return GL_TEXTURE_CUBE_MAP;
		default: DE_ASSERT(false); return (deUint32)-1;
	}
}

enum OffsetSize
{
	OFFSETSIZE_NONE = 0,
	OFFSETSIZE_MINIMUM_REQUIRED,
	OFFSETSIZE_IMPLEMENTATION_MAXIMUM,

	OFFSETSIZE_LAST
};

static inline bool isMipmapFilter (tcu::Sampler::FilterMode filter)
{
	switch (filter)
	{
		case tcu::Sampler::NEAREST:
		case tcu::Sampler::LINEAR:
			return false;

		case tcu::Sampler::NEAREST_MIPMAP_NEAREST:
		case tcu::Sampler::NEAREST_MIPMAP_LINEAR:
		case tcu::Sampler::LINEAR_MIPMAP_NEAREST:
		case tcu::Sampler::LINEAR_MIPMAP_LINEAR:
			return true;

		default:
			DE_ASSERT(false);
			return false;
	}
}

class TextureGatherCase : public TestCase
{
public:
										TextureGatherCase		(Context&					context,
																 const char*				name,
																 const char*				description,
																 TextureType				textureType,
																 GatherType					gatherType,
																 OffsetSize					offsetSize,
																 tcu::TextureFormat			textureFormat,
																 tcu::Sampler::CompareMode	shadowCompareMode, //!< Should be COMPAREMODE_NONE iff textureFormat is a depth format.
																 tcu::Sampler::WrapMode		wrapS,
																 tcu::Sampler::WrapMode		wrapT,
																 const MaybeTextureSwizzle&	texSwizzle,
																 // \note Filter modes have no effect on gather (except when it comes to
																 //		  texture completeness); these are supposed to test just that.
																 tcu::Sampler::FilterMode	minFilter,
																 tcu::Sampler::FilterMode	magFilter,
																 int						baseLevel,
																 deUint32					flags);

	void								init					(void);
	void								deinit					(void);
	IterateResult						iterate					(void);

protected:
	IVec2								getOffsetRange			(void) const;

	template <typename TexViewT, typename TexCoordT>
	bool								verify					(const ConstPixelBufferAccess&	rendered,
																 const TexViewT&				texture,
																 const TexCoordT				(&bottomLeft)[4],
																 const GatherArgs&				gatherArgs) const;

	virtual void						generateIterations		(void) = 0;
	virtual void						createAndUploadTexture	(void) = 0;
	virtual int							getNumIterations		(void) const = 0;
	virtual GatherArgs					getGatherArgs			(int iterationNdx) const = 0;
	virtual vector<float>				computeQuadTexCoord		(int iterationNdx) const = 0;
	virtual bool						verify					(int iterationNdx, const ConstPixelBufferAccess& rendered) const = 0;

	const GatherType					m_gatherType;
	const OffsetSize					m_offsetSize;
	const tcu::TextureFormat			m_textureFormat;
	const tcu::Sampler::CompareMode		m_shadowCompareMode;
	const tcu::Sampler::WrapMode		m_wrapS;
	const tcu::Sampler::WrapMode		m_wrapT;
	const MaybeTextureSwizzle			m_textureSwizzle;
	const tcu::Sampler::FilterMode		m_minFilter;
	const tcu::Sampler::FilterMode		m_magFilter;
	const int							m_baseLevel;
	const deUint32						m_flags;

private:
	enum
	{
		SPEC_MAX_MIN_OFFSET = -8,
		SPEC_MIN_MAX_OFFSET = 7
	};

	static const IVec2					RENDER_SIZE;

	glu::VertexSource					genVertexShaderSource		(bool requireGpuShader5, int numTexCoordComponents, bool useNormalizedCoordInput);
	glu::FragmentSource					genFragmentShaderSource		(bool requireGpuShader5, int numTexCoordComponents, glu::DataType samplerType, const string& funcCall, bool useNormalizedCoordInput, bool usePixCoord);
	string								genGatherFuncCall			(GatherType, const tcu::TextureFormat&, const GatherArgs&, const string& refZExpr, const IVec2& offsetRange, int indentationDepth);
	glu::ProgramSources					genProgramSources			(GatherType, TextureType, const tcu::TextureFormat&, const GatherArgs&, const string& refZExpr, const IVec2& offsetRange);

	const TextureType					m_textureType;

	const tcu::TextureFormat			m_colorBufferFormat;
	MovePtr<glu::Renderbuffer>			m_colorBuffer;
	MovePtr<glu::Framebuffer>			m_fbo;

	int									m_currentIteration;
	MovePtr<ShaderProgram>				m_program;
};

const IVec2 TextureGatherCase::RENDER_SIZE = IVec2(64, 64);

TextureGatherCase::TextureGatherCase (Context&						context,
									  const char*					name,
									  const char*					description,
									  TextureType					textureType,
									  GatherType					gatherType,
									  OffsetSize					offsetSize,
									  tcu::TextureFormat			textureFormat,
									  tcu::Sampler::CompareMode		shadowCompareMode, //!< Should be COMPAREMODE_NONE iff textureType == TEXTURETYPE_NORMAL.
									  tcu::Sampler::WrapMode		wrapS,
									  tcu::Sampler::WrapMode		wrapT,
									  const MaybeTextureSwizzle&	textureSwizzle,
									  tcu::Sampler::FilterMode		minFilter,
									  tcu::Sampler::FilterMode		magFilter,
									  int							baseLevel,
									  deUint32						flags)
	: TestCase				(context, name, description)
	, m_gatherType			(gatherType)
	, m_offsetSize			(offsetSize)
	, m_textureFormat		(textureFormat)
	, m_shadowCompareMode	(shadowCompareMode)
	, m_wrapS				(wrapS)
	, m_wrapT				(wrapT)
	, m_textureSwizzle		(textureSwizzle)
	, m_minFilter			(minFilter)
	, m_magFilter			(magFilter)
	, m_baseLevel			(baseLevel)
	, m_flags				(flags)
	, m_textureType			(textureType)
	, m_colorBufferFormat	(tcu::TextureFormat(tcu::TextureFormat::RGBA,
												isDepthFormat(textureFormat) ? tcu::TextureFormat::UNORM_INT8 : textureFormat.type))
	, m_currentIteration	(0)
{
	DE_ASSERT((m_gatherType == GATHERTYPE_BASIC) == (m_offsetSize == OFFSETSIZE_NONE));
	DE_ASSERT((m_shadowCompareMode != tcu::Sampler::COMPAREMODE_NONE) == isDepthFormat(m_textureFormat));
	DE_ASSERT(isUnormFormatType(m_colorBufferFormat.type)						||
			  m_colorBufferFormat.type == tcu::TextureFormat::UNSIGNED_INT8		||
			  m_colorBufferFormat.type == tcu::TextureFormat::UNSIGNED_INT16	||
			  m_colorBufferFormat.type == tcu::TextureFormat::SIGNED_INT8		||
			  m_colorBufferFormat.type == tcu::TextureFormat::SIGNED_INT16);
	DE_ASSERT(glu::isGLInternalColorFormatFilterable(glu::getInternalFormat(m_colorBufferFormat)) ||
			  (m_magFilter == tcu::Sampler::NEAREST && (m_minFilter == tcu::Sampler::NEAREST || m_minFilter == tcu::Sampler::NEAREST_MIPMAP_NEAREST)));
	DE_ASSERT(isMipmapFilter(m_minFilter) || !(m_flags & GATHERCASE_MIPMAP_INCOMPLETE));
	DE_ASSERT(m_textureType == TEXTURETYPE_CUBE || !(m_flags & GATHERCASE_DONT_SAMPLE_CUBE_CORNERS));
	DE_ASSERT(!((m_flags & GATHERCASE_MIPMAP_INCOMPLETE) && isDepthFormat(m_textureFormat))); // It's not clear what shadow textures should return when incomplete.
}

IVec2 TextureGatherCase::getOffsetRange (void) const
{
	switch (m_offsetSize)
	{
		case OFFSETSIZE_NONE:
			return IVec2(0);
			break;

		case OFFSETSIZE_MINIMUM_REQUIRED:
			// \note Defined by spec.
			return IVec2(SPEC_MAX_MIN_OFFSET,
						 SPEC_MIN_MAX_OFFSET);
			break;

		case OFFSETSIZE_IMPLEMENTATION_MAXIMUM:
			return IVec2(m_context.getContextInfo().getInt(GL_MIN_PROGRAM_TEXTURE_GATHER_OFFSET),
						 m_context.getContextInfo().getInt(GL_MAX_PROGRAM_TEXTURE_GATHER_OFFSET));
			break;

		default:
			DE_ASSERT(false);
			return IVec2(-1);
	}
}

glu::VertexSource TextureGatherCase::genVertexShaderSource (bool requireGpuShader5, int numTexCoordComponents, bool useNormalizedCoordInput)
{
	DE_ASSERT(numTexCoordComponents == 2 || numTexCoordComponents == 3);
	const string texCoordType = "vec" + de::toString(numTexCoordComponents);
	std::string vertexSource = "${GLSL_VERSION_DECL}\n"
							   + string(requireGpuShader5 ? "${GPU_SHADER5_REQUIRE}\n" : "") +
							 "\n"
							 "in highp vec2 a_position;\n"
							 "in highp " + texCoordType + " a_texCoord;\n"
							 + (useNormalizedCoordInput ? "in highp vec2 a_normalizedCoord; // (0,0) to (1,1)\n" : "") +
							 "\n"
							 "out highp " + texCoordType + " v_texCoord;\n"
							 + (useNormalizedCoordInput ? "out highp vec2 v_normalizedCoord;\n" : "") +
							 "\n"
							 "void main (void)\n"
							 "{\n"
							 "	gl_Position = vec4(a_position.x, a_position.y, 0.0, 1.0);\n"
							 "	v_texCoord = a_texCoord;\n"
							 + (useNormalizedCoordInput ? "\tv_normalizedCoord = a_normalizedCoord;\n" : "") +
							   "}\n";
	return glu::VertexSource(specializeShader(m_context, vertexSource.c_str()));
}

glu::FragmentSource TextureGatherCase::genFragmentShaderSource (bool			requireGpuShader5,
																int				numTexCoordComponents,
																glu::DataType	samplerType,
																const string&	funcCall,
																bool			useNormalizedCoordInput,
																bool			usePixCoord)
{
	DE_ASSERT(glu::isDataTypeSampler(samplerType));
	DE_ASSERT(de::inRange(numTexCoordComponents, 2, 3));
	DE_ASSERT(!usePixCoord || useNormalizedCoordInput);

	const string texCoordType = "vec" + de::toString(numTexCoordComponents);

	std::string fragmentSource = "${GLSL_VERSION_DECL}\n"
								 + string(requireGpuShader5 ? "${GPU_SHADER5_REQUIRE}\n" : "") +
							   "\n"
							   "layout (location = 0) out mediump " + glu::getDataTypeName(getSamplerGatherResultType(samplerType)) + " o_color;\n"
							   "\n"
							   "in highp " + texCoordType + " v_texCoord;\n"
							   + (useNormalizedCoordInput ? "in highp vec2 v_normalizedCoord;\n" : "") +
							   "\n"
							   "uniform highp " + string(glu::getDataTypeName(samplerType)) + " u_sampler;\n"
							   + (useNormalizedCoordInput ? "uniform highp vec2 u_viewportSize;\n" : "") +
							   "\n"
							   "void main(void)\n"
							   "{\n"
							   + (usePixCoord ? "\tivec2 pixCoord = ivec2(v_normalizedCoord*u_viewportSize);\n" : "") +
							   "	o_color = " + funcCall + ";\n"
								 "}\n";

	return glu::FragmentSource(specializeShader(m_context, fragmentSource.c_str()));
}

string TextureGatherCase::genGatherFuncCall (GatherType gatherType, const tcu::TextureFormat& textureFormat, const GatherArgs& gatherArgs, const string& refZExpr, const IVec2& offsetRange, int indentationDepth)
{
	string result;

	switch (gatherType)
	{
		case GATHERTYPE_BASIC:
			result += "textureGather";
			break;
		case GATHERTYPE_OFFSET: // \note Fallthrough.
		case GATHERTYPE_OFFSET_DYNAMIC:
			result += "textureGatherOffset";
			break;
		case GATHERTYPE_OFFSETS:
			result += "textureGatherOffsets";
			break;
		default:
			DE_ASSERT(false);
	}

	result += "(u_sampler, v_texCoord";

	if (isDepthFormat(textureFormat))
	{
		DE_ASSERT(gatherArgs.componentNdx < 0);
		result += ", " + refZExpr;
	}

	if (gatherType == GATHERTYPE_OFFSET ||
		gatherType == GATHERTYPE_OFFSET_DYNAMIC ||
		gatherType == GATHERTYPE_OFFSETS)
	{
		result += ", ";
		switch (gatherType)
		{
			case GATHERTYPE_OFFSET:
				result += "ivec2" + de::toString(gatherArgs.offsets[0]);
				break;

			case GATHERTYPE_OFFSET_DYNAMIC:
				result += "pixCoord.yx % ivec2(" + de::toString(offsetRange.y() - offsetRange.x() + 1) + ") + " + de::toString(offsetRange.x());
				break;

			case GATHERTYPE_OFFSETS:
				result += "ivec2[4](\n"
						  + string(indentationDepth, '\t') + "\tivec2" + de::toString(gatherArgs.offsets[0]) + ",\n"
						  + string(indentationDepth, '\t') + "\tivec2" + de::toString(gatherArgs.offsets[1]) + ",\n"
						  + string(indentationDepth, '\t') + "\tivec2" + de::toString(gatherArgs.offsets[2]) + ",\n"
						  + string(indentationDepth, '\t') + "\tivec2" + de::toString(gatherArgs.offsets[3]) + ")\n"
						  + string(indentationDepth, '\t') + "\t";
				break;

			default:
				DE_ASSERT(false);
		}
	}

	if (gatherArgs.componentNdx >= 0)
	{
		DE_ASSERT(gatherArgs.componentNdx < 4);
		result += ", " + de::toString(gatherArgs.componentNdx);
	}

	result += ")";

	return result;
}

// \note If componentNdx for genProgramSources() is -1, component index is not specified.
glu::ProgramSources TextureGatherCase::genProgramSources (GatherType					gatherType,
														  TextureType					textureType,
														  const tcu::TextureFormat&		textureFormat,
														  const GatherArgs&				gatherArgs,
														  const string&					refZExpr,
														  const IVec2&					offsetRange)
{
	const bool				usePixCoord			= gatherType == GATHERTYPE_OFFSET_DYNAMIC;
	const bool				useNormalizedCoord	= usePixCoord || isDepthFormat(textureFormat);
	const bool				isDynamicOffset		= gatherType == GATHERTYPE_OFFSET_DYNAMIC;
	const bool				isShadow			= isDepthFormat(textureFormat);
	const glu::DataType		samplerType			= getSamplerType(textureType, textureFormat);
	const int				numDims				= getNumTextureSamplingDimensions(textureType);
	const string			funcCall			= genGatherFuncCall(gatherType, textureFormat, gatherArgs, refZExpr, offsetRange, 1);

	return glu::ProgramSources() << genVertexShaderSource(requireGpuShader5(gatherType), numDims, isDynamicOffset || isShadow)
								 << genFragmentShaderSource(requireGpuShader5(gatherType), numDims, samplerType, funcCall, useNormalizedCoord, usePixCoord);
}

void TextureGatherCase::init (void)
{
	TestLog&					log				= m_testCtx.getLog();
	const glu::RenderContext&	renderCtx		= m_context.getRenderContext();
	const glw::Functions&		gl				= renderCtx.getFunctions();
	const deUint32				texTypeGL		= getGLTextureType(m_textureType);
	const bool					supportsES32	= glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::es(3, 2));

	// Check prerequisites.
	if (requireGpuShader5(m_gatherType) && !supportsES32 && !m_context.getContextInfo().isExtensionSupported("GL_EXT_gpu_shader5"))
		throw tcu::NotSupportedError("GL_EXT_gpu_shader5 required");

	// Log and check implementation offset limits, if appropriate.
	if (m_offsetSize == OFFSETSIZE_IMPLEMENTATION_MAXIMUM)
	{
		const IVec2 offsetRange = getOffsetRange();
		log << TestLog::Integer("ImplementationMinTextureGatherOffset", "Implementation's value for GL_MIN_PROGRAM_TEXTURE_GATHER_OFFSET", "", QP_KEY_TAG_NONE, offsetRange[0])
			<< TestLog::Integer("ImplementationMaxTextureGatherOffset", "Implementation's value for GL_MAX_PROGRAM_TEXTURE_GATHER_OFFSET", "", QP_KEY_TAG_NONE, offsetRange[1]);
		TCU_CHECK_MSG(offsetRange[0] <= SPEC_MAX_MIN_OFFSET, ("GL_MIN_PROGRAM_TEXTURE_GATHER_OFFSET must be at most " + de::toString((int)SPEC_MAX_MIN_OFFSET)).c_str());
		TCU_CHECK_MSG(offsetRange[1] >= SPEC_MIN_MAX_OFFSET, ("GL_MAX_PROGRAM_TEXTURE_GATHER_OFFSET must be at least " + de::toString((int)SPEC_MIN_MAX_OFFSET)).c_str());
	}

	// Create rbo and fbo.

	m_colorBuffer = MovePtr<glu::Renderbuffer>(new glu::Renderbuffer(renderCtx));
	gl.bindRenderbuffer(GL_RENDERBUFFER, **m_colorBuffer);
	gl.renderbufferStorage(GL_RENDERBUFFER, glu::getInternalFormat(m_colorBufferFormat), RENDER_SIZE.x(), RENDER_SIZE.y());
	GLU_EXPECT_NO_ERROR(gl.getError(), "Create and setup renderbuffer object");

	m_fbo = MovePtr<glu::Framebuffer>(new glu::Framebuffer(renderCtx));
	gl.bindFramebuffer(GL_FRAMEBUFFER, **m_fbo);
	gl.framebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, **m_colorBuffer);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Create and setup framebuffer object");

	log << TestLog::Message << "Using a framebuffer object with renderbuffer with format "
							<< glu::getTextureFormatName(glu::getInternalFormat(m_colorBufferFormat))
							<< " and size " << RENDER_SIZE << TestLog::EndMessage;

	// Generate subclass-specific iterations.

	generateIterations();
	m_currentIteration = 0;

	// Initialize texture.

	createAndUploadTexture();
	gl.texParameteri(texTypeGL, GL_TEXTURE_WRAP_S,		glu::getGLWrapMode(m_wrapS));
	gl.texParameteri(texTypeGL, GL_TEXTURE_WRAP_T,		glu::getGLWrapMode(m_wrapT));
	gl.texParameteri(texTypeGL, GL_TEXTURE_MIN_FILTER,	glu::getGLFilterMode(m_minFilter));
	gl.texParameteri(texTypeGL, GL_TEXTURE_MAG_FILTER,	glu::getGLFilterMode(m_magFilter));

	if (m_baseLevel != 0)
		gl.texParameteri(texTypeGL, GL_TEXTURE_BASE_LEVEL, m_baseLevel);

	if (isDepthFormat(m_textureFormat))
	{
		gl.texParameteri(texTypeGL, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
		gl.texParameteri(texTypeGL, GL_TEXTURE_COMPARE_FUNC, glu::getGLCompareFunc(m_shadowCompareMode));
	}

	if (m_textureSwizzle.isSome())
	{
		const deUint32 swizzleNamesGL[4] =
		{
			GL_TEXTURE_SWIZZLE_R,
			GL_TEXTURE_SWIZZLE_G,
			GL_TEXTURE_SWIZZLE_B,
			GL_TEXTURE_SWIZZLE_A
		};

		for (int i = 0; i < 4; i++)
		{
			const deUint32 curGLSwizzle = getGLTextureSwizzleComponent(m_textureSwizzle.getSwizzle()[i]);
			gl.texParameteri(texTypeGL, swizzleNamesGL[i], curGLSwizzle);
		}
	}

	GLU_EXPECT_NO_ERROR(gl.getError(), "Set texture parameters");

	log << TestLog::Message << "Texture base level is " << m_baseLevel << TestLog::EndMessage
		<< TestLog::Message << "s and t wrap modes are "
							<< glu::getTextureWrapModeName(glu::getGLWrapMode(m_wrapS)) << " and "
							<< glu::getTextureWrapModeName(glu::getGLWrapMode(m_wrapT)) << ", respectively" << TestLog::EndMessage
		<< TestLog::Message << "Minification and magnification filter modes are "
							<< glu::getTextureFilterName(glu::getGLFilterMode(m_minFilter)) << " and "
							<< glu::getTextureFilterName(glu::getGLFilterMode(m_magFilter)) << ", respectively "
							<< ((m_flags & GATHERCASE_MIPMAP_INCOMPLETE) ?
								"(note that they cause the texture to be incomplete)" :
								"(note that they should have no effect on gather result)")
							<< TestLog::EndMessage
		<< TestLog::Message << "Using texture swizzle " << m_textureSwizzle << TestLog::EndMessage;

	if (m_shadowCompareMode != tcu::Sampler::COMPAREMODE_NONE)
		log << TestLog::Message << "Using texture compare func " << glu::getCompareFuncName(glu::getGLCompareFunc(m_shadowCompareMode)) << TestLog::EndMessage;
}

void TextureGatherCase::deinit (void)
{
	m_program		= MovePtr<ShaderProgram>(DE_NULL);
	m_fbo			= MovePtr<glu::Framebuffer>(DE_NULL);
	m_colorBuffer	= MovePtr<glu::Renderbuffer>(DE_NULL);
}

TextureGatherCase::IterateResult TextureGatherCase::iterate (void)
{
	TestLog&						log								= m_testCtx.getLog();
	const tcu::ScopedLogSection		iterationSection				(log, "Iteration" + de::toString(m_currentIteration), "Iteration " + de::toString(m_currentIteration));
	const glu::RenderContext&		renderCtx						= m_context.getRenderContext();
	const glw::Functions&			gl								= renderCtx.getFunctions();
	const GatherArgs&				gatherArgs						= getGatherArgs(m_currentIteration);
	const string					refZExpr						= "v_normalizedCoord.x";
	const bool						needPixelCoordInShader			= m_gatherType == GATHERTYPE_OFFSET_DYNAMIC;
	const bool						needNormalizedCoordInShader		= needPixelCoordInShader || isDepthFormat(m_textureFormat);

	// Generate a program appropriate for this iteration.

	m_program = MovePtr<ShaderProgram>(new ShaderProgram(renderCtx, genProgramSources(m_gatherType, m_textureType, m_textureFormat, gatherArgs, refZExpr, getOffsetRange())));
	if (m_currentIteration == 0)
		m_testCtx.getLog() << *m_program;
	else
		m_testCtx.getLog() << TestLog::Message << "Using a program similar to the previous one, except with a gather function call as follows:\n"
											   << genGatherFuncCall(m_gatherType, m_textureFormat, gatherArgs, refZExpr, getOffsetRange(), 0)
											   << TestLog::EndMessage;
	if (!m_program->isOk())
	{
		if (m_currentIteration != 0)
			m_testCtx.getLog() << *m_program;
		TCU_FAIL("Failed to build program");
	}

	// Render.

	gl.viewport(0, 0, RENDER_SIZE.x(), RENDER_SIZE.y());
	gl.clearColor(0.0f, 0.0f, 0.0f, 1.0f);
	gl.clear(GL_COLOR_BUFFER_BIT);

	{
		const float position[4*2] =
		{
			-1.0f, -1.0f,
			-1.0f, +1.0f,
			+1.0f, -1.0f,
			+1.0f, +1.0f,
		};

		const float normalizedCoord[4*2] =
		{
			0.0f, 0.0f,
			0.0f, 1.0f,
			1.0f, 0.0f,
			1.0f, 1.0f,
		};

		const vector<float> texCoord = computeQuadTexCoord(m_currentIteration);

		vector<glu::VertexArrayBinding> attrBindings;
		attrBindings.push_back(glu::va::Float("a_position", 2, 4, 0, &position[0]));
		attrBindings.push_back(glu::va::Float("a_texCoord", (int)texCoord.size()/4, 4, 0, &texCoord[0]));
		if (needNormalizedCoordInShader)
			attrBindings.push_back(glu::va::Float("a_normalizedCoord", 2, 4, 0, &normalizedCoord[0]));

		const deUint16 indices[6] = { 0, 1, 2, 2, 1, 3 };

		gl.useProgram(m_program->getProgram());

		{
			const int samplerUniformLocation = gl.getUniformLocation(m_program->getProgram(), "u_sampler");
			TCU_CHECK(samplerUniformLocation >= 0);
			gl.uniform1i(samplerUniformLocation, 0);
		}

		if (needPixelCoordInShader)
		{
			const int viewportSizeUniformLocation = gl.getUniformLocation(m_program->getProgram(), "u_viewportSize");
			TCU_CHECK(viewportSizeUniformLocation >= 0);
			gl.uniform2f(viewportSizeUniformLocation, (float)RENDER_SIZE.x(), (float)RENDER_SIZE.y());
		}

		if (texCoord.size() == 2*4)
		{
			Vec2 texCoordVec[4];
			computeTexCoordVecs(texCoord, texCoordVec);
			log << TestLog::Message << "Texture coordinates run from " << texCoordVec[0] << " to " << texCoordVec[3] << TestLog::EndMessage;
		}
		else if (texCoord.size() == 3*4)
		{
			Vec3 texCoordVec[4];
			computeTexCoordVecs(texCoord, texCoordVec);
			log << TestLog::Message << "Texture coordinates run from " << texCoordVec[0] << " to " << texCoordVec[3] << TestLog::EndMessage;
		}
		else
			DE_ASSERT(false);

		glu::draw(renderCtx, m_program->getProgram(), (int)attrBindings.size(), &attrBindings[0],
			glu::pr::Triangles(DE_LENGTH_OF_ARRAY(indices), &indices[0]));
	}

	// Verify result.

	{
		const tcu::TextureLevel rendered = getPixels(renderCtx, RENDER_SIZE, m_colorBufferFormat);

		if (!verify(m_currentIteration, rendered.getAccess()))
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Result verification failed");
			return STOP;
		}
	}

	m_currentIteration++;
	if (m_currentIteration == (int)getNumIterations())
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
		return STOP;
	}
	else
		return CONTINUE;
}

template <typename TexViewT, typename TexCoordT>
bool TextureGatherCase::verify (const ConstPixelBufferAccess&	rendered,
								const TexViewT&					texture,
								const TexCoordT					(&texCoords)[4],
								const GatherArgs&				gatherArgs) const
{
	TestLog& log = m_testCtx.getLog();

	if (m_flags & GATHERCASE_MIPMAP_INCOMPLETE)
	{
		const int	componentNdx		= de::max(0, gatherArgs.componentNdx);
		const Vec4	incompleteColor		(0.0f, 0.0f, 0.0f, 1.0f);
		const Vec4	refColor			(incompleteColor[componentNdx]);
		const bool	isOk				= verifySingleColored(log, rendered, refColor);

		if (!isOk)
			log << TestLog::Message << "Note: expected color " << refColor << " for all pixels; "
									<< incompleteColor[componentNdx] << " is component at index " << componentNdx
									<< " in the color " << incompleteColor << ", which is used for incomplete textures" << TestLog::EndMessage;

		return isOk;
	}
	else
	{
		DE_ASSERT(m_colorBufferFormat.order == tcu::TextureFormat::RGBA);
		DE_ASSERT(m_colorBufferFormat.type == tcu::TextureFormat::UNORM_INT8		||
				  m_colorBufferFormat.type == tcu::TextureFormat::UNSIGNED_INT8		||
				  m_colorBufferFormat.type == tcu::TextureFormat::SIGNED_INT8);

		const MovePtr<PixelOffsets>		pixelOffsets	= makePixelOffsetsFunctor(m_gatherType, gatherArgs, getOffsetRange());
		const tcu::PixelFormat			pixelFormat		= tcu::PixelFormat(8,8,8,8);
		const IVec4						colorBits		= tcu::max(glu::TextureTestUtil::getBitsVec(pixelFormat) - 1, tcu::IVec4(0));
		const IVec3						coordBits		= m_textureType == TEXTURETYPE_2D			? IVec3(20,20,0)
														: m_textureType == TEXTURETYPE_CUBE			? IVec3(10,10,10)
														: m_textureType == TEXTURETYPE_2D_ARRAY		? IVec3(20,20,20)
														: IVec3(-1);
		const IVec3						uvwBits			= m_textureType == TEXTURETYPE_2D			? IVec3(7,7,0)
														: m_textureType == TEXTURETYPE_CUBE			? IVec3(6,6,0)
														: m_textureType == TEXTURETYPE_2D_ARRAY		? IVec3(7,7,7)
														: IVec3(-1);
		tcu::Sampler					sampler;
		sampler.wrapS		= m_wrapS;
		sampler.wrapT		= m_wrapT;
		sampler.compare		= m_shadowCompareMode;

		if (isDepthFormat(m_textureFormat))
		{
			tcu::TexComparePrecision comparePrec;
			comparePrec.coordBits		= coordBits;
			comparePrec.uvwBits			= uvwBits;
			comparePrec.referenceBits	= 16;
			comparePrec.resultBits		= pixelFormat.redBits-1;

			return verifyGatherOffsetsCompare(log, rendered, texture, texCoords, sampler, comparePrec, PixelCompareRefZDefault(RENDER_SIZE), *pixelOffsets);
		}
		else
		{
			const int componentNdx = de::max(0, gatherArgs.componentNdx);

			if (isUnormFormatType(m_textureFormat.type))
			{
				tcu::LookupPrecision lookupPrec;
				lookupPrec.colorThreshold	= tcu::computeFixedPointThreshold(colorBits);
				lookupPrec.coordBits		= coordBits;
				lookupPrec.uvwBits			= uvwBits;
				lookupPrec.colorMask		= glu::TextureTestUtil::getCompareMask(pixelFormat);
				return verifyGatherOffsets<float>(log, rendered, texture, texCoords, sampler, lookupPrec, componentNdx, *pixelOffsets);
			}
			else if (isUIntFormatType(m_textureFormat.type) || isSIntFormatType(m_textureFormat.type))
			{
				tcu::IntLookupPrecision		lookupPrec;
				lookupPrec.colorThreshold	= UVec4(0);
				lookupPrec.coordBits		= coordBits;
				lookupPrec.uvwBits			= uvwBits;
				lookupPrec.colorMask		= glu::TextureTestUtil::getCompareMask(pixelFormat);

				if (isUIntFormatType(m_textureFormat.type))
					return verifyGatherOffsets<deUint32>(log, rendered, texture, texCoords, sampler, lookupPrec, componentNdx, *pixelOffsets);
				else if (isSIntFormatType(m_textureFormat.type))
					return verifyGatherOffsets<deInt32>(log, rendered, texture, texCoords, sampler, lookupPrec, componentNdx, *pixelOffsets);
				else
				{
					DE_ASSERT(false);
					return false;
				}
			}
			else
			{
				DE_ASSERT(false);
				return false;
			}
		}
	}
}

vector<GatherArgs> generateBasic2DCaseIterations (GatherType gatherType, const tcu::TextureFormat& textureFormat, const IVec2& offsetRange)
{
	const int			numComponentCases	= isDepthFormat(textureFormat) ? 1 : 4+1; // \note For non-depth textures, test explicit components 0 to 3 and implicit component 0.
	vector<GatherArgs>	result;

	for (int componentCaseNdx = 0; componentCaseNdx < numComponentCases; componentCaseNdx++)
	{
		const int componentNdx = componentCaseNdx - 1;

		switch (gatherType)
		{
			case GATHERTYPE_BASIC:
				result.push_back(GatherArgs(componentNdx));
				break;

			case GATHERTYPE_OFFSET:
			{
				const int min	= offsetRange.x();
				const int max	= offsetRange.y();
				const int hmin	= divRoundToZero(min, 2);
				const int hmax	= divRoundToZero(max, 2);

				result.push_back(GatherArgs(componentNdx, IVec2(min, max)));

				if (componentCaseNdx == 0) // Don't test all offsets variants for all color components (they should be pretty orthogonal).
				{
					result.push_back(GatherArgs(componentNdx, IVec2(min,	min)));
					result.push_back(GatherArgs(componentNdx, IVec2(max,	min)));
					result.push_back(GatherArgs(componentNdx, IVec2(max,	max)));

					result.push_back(GatherArgs(componentNdx, IVec2(0,		hmax)));
					result.push_back(GatherArgs(componentNdx, IVec2(hmin,	0)));
					result.push_back(GatherArgs(componentNdx, IVec2(0,		0)));
				}

				break;
			}

			case GATHERTYPE_OFFSET_DYNAMIC:
				result.push_back(GatherArgs(componentNdx));
				break;

			case GATHERTYPE_OFFSETS:
			{
				const int min	= offsetRange.x();
				const int max	= offsetRange.y();
				const int hmin	= divRoundToZero(min, 2);
				const int hmax	= divRoundToZero(max, 2);

				result.push_back(GatherArgs(componentNdx,
											IVec2(min,	min),
											IVec2(min,	max),
											IVec2(max,	min),
											IVec2(max,	max)));

				if (componentCaseNdx == 0) // Don't test all offsets variants for all color components (they should be pretty orthogonal).
					result.push_back(GatherArgs(componentNdx,
												IVec2(min,	hmax),
												IVec2(hmin,	max),
												IVec2(0,	hmax),
												IVec2(hmax,	0)));
				break;
			}

			default:
				DE_ASSERT(false);
		}
	}

	return result;
}

class TextureGather2DCase : public TextureGatherCase
{
public:
	TextureGather2DCase (Context&					context,
						 const char*				name,
						 const char*				description,
						 GatherType					gatherType,
						 OffsetSize					offsetSize,
						 tcu::TextureFormat			textureFormat,
						 tcu::Sampler::CompareMode	shadowCompareMode,
						 tcu::Sampler::WrapMode		wrapS,
						 tcu::Sampler::WrapMode		wrapT,
						 const MaybeTextureSwizzle&	texSwizzle,
						 tcu::Sampler::FilterMode	minFilter,
						 tcu::Sampler::FilterMode	magFilter,
						 int						baseLevel,
						 deUint32					flags,
						 const IVec2&				textureSize)
		: TextureGatherCase		(context, name, description, TEXTURETYPE_2D, gatherType, offsetSize, textureFormat, shadowCompareMode, wrapS, wrapT, texSwizzle, minFilter, magFilter, baseLevel, flags)
		, m_textureSize			(textureSize)
		, m_swizzledTexture		(tcu::TextureFormat(), 1, 1)
	{
	}

protected:
	void						generateIterations		(void);
	void						createAndUploadTexture	(void);
	int							getNumIterations		(void) const { DE_ASSERT(!m_iterations.empty()); return (int)m_iterations.size(); }
	GatherArgs					getGatherArgs			(int iterationNdx) const { return m_iterations[iterationNdx]; }
	vector<float>				computeQuadTexCoord		(int iterationNdx) const;
	bool						verify					(int iterationNdx, const ConstPixelBufferAccess& rendered) const;

private:
	const IVec2					m_textureSize;

	MovePtr<glu::Texture2D>		m_texture;
	tcu::Texture2D				m_swizzledTexture;
	vector<GatherArgs>			m_iterations;
};

vector<float> TextureGather2DCase::computeQuadTexCoord (int /* iterationNdx */) const
{
	vector<float> res;
	glu::TextureTestUtil::computeQuadTexCoord2D(res, Vec2(-0.3f, -0.4f), Vec2(1.5f, 1.6f));
	return res;
}

void TextureGather2DCase::generateIterations (void)
{
	DE_ASSERT(m_iterations.empty());
	m_iterations = generateBasic2DCaseIterations(m_gatherType, m_textureFormat, getOffsetRange());
}

void TextureGather2DCase::createAndUploadTexture (void)
{
	const glu::RenderContext&		renderCtx	= m_context.getRenderContext();
	const glw::Functions&			gl			= renderCtx.getFunctions();
	const tcu::TextureFormatInfo	texFmtInfo	= tcu::getTextureFormatInfo(m_textureFormat);

	m_texture = MovePtr<glu::Texture2D>(new glu::Texture2D(renderCtx, glu::getInternalFormat(m_textureFormat), m_textureSize.x(), m_textureSize.y()));

	{
		tcu::Texture2D&		refTexture	= m_texture->getRefTexture();
		const int			levelBegin	= m_baseLevel;
		const int			levelEnd	= isMipmapFilter(m_minFilter) && !(m_flags & GATHERCASE_MIPMAP_INCOMPLETE) ? refTexture.getNumLevels() : m_baseLevel+1;
		DE_ASSERT(m_baseLevel < refTexture.getNumLevels());

		for (int levelNdx = levelBegin; levelNdx < levelEnd; levelNdx++)
		{
			refTexture.allocLevel(levelNdx);
			const PixelBufferAccess& level = refTexture.getLevel(levelNdx);
			fillWithRandomColorTiles(level, texFmtInfo.valueMin, texFmtInfo.valueMax, (deUint32)m_testCtx.getCommandLine().getBaseSeed());
			m_testCtx.getLog() << TestLog::Image("InputTextureLevel" + de::toString(levelNdx), "Input texture, level " + de::toString(levelNdx), level)
							   << TestLog::Message << "Note: texture level's size is " << IVec2(level.getWidth(), level.getHeight()) << TestLog::EndMessage;
		}

		swizzleTexture(m_swizzledTexture, refTexture, m_textureSwizzle);
	}

	gl.activeTexture(GL_TEXTURE0);
	m_texture->upload();
}

bool TextureGather2DCase::verify (int iterationNdx, const ConstPixelBufferAccess& rendered) const
{
	Vec2 texCoords[4];
	computeTexCoordVecs(computeQuadTexCoord(iterationNdx), texCoords);
	return TextureGatherCase::verify(rendered, getOneLevelSubView(tcu::Texture2DView(m_swizzledTexture), m_baseLevel), texCoords, m_iterations[iterationNdx]);
}

class TextureGather2DArrayCase : public TextureGatherCase
{
public:
	TextureGather2DArrayCase (Context&						context,
							  const char*					name,
							  const char*					description,
							  GatherType					gatherType,
							  OffsetSize					offsetSize,
							  tcu::TextureFormat			textureFormat,
							  tcu::Sampler::CompareMode		shadowCompareMode,
							  tcu::Sampler::WrapMode		wrapS,
							  tcu::Sampler::WrapMode		wrapT,
							  const MaybeTextureSwizzle&	texSwizzle,
							  tcu::Sampler::FilterMode		minFilter,
							  tcu::Sampler::FilterMode		magFilter,
							  int							baseLevel,
							  deUint32						flags,
							  const IVec3&					textureSize)
		: TextureGatherCase		(context, name, description, TEXTURETYPE_2D_ARRAY, gatherType, offsetSize, textureFormat, shadowCompareMode, wrapS, wrapT, texSwizzle, minFilter, magFilter, baseLevel, flags)
		, m_textureSize			(textureSize)
		, m_swizzledTexture		(tcu::TextureFormat(), 1, 1, 1)
	{
	}

protected:
	void							generateIterations		(void);
	void							createAndUploadTexture	(void);
	int								getNumIterations		(void) const { DE_ASSERT(!m_iterations.empty()); return (int)m_iterations.size(); }
	GatherArgs						getGatherArgs			(int iterationNdx) const { return m_iterations[iterationNdx].gatherArgs; }
	vector<float>					computeQuadTexCoord		(int iterationNdx) const;
	bool							verify					(int iterationNdx, const ConstPixelBufferAccess& rendered) const;

private:
	struct Iteration
	{
		GatherArgs	gatherArgs;
		int			layerNdx;
	};

	const IVec3						m_textureSize;

	MovePtr<glu::Texture2DArray>	m_texture;
	tcu::Texture2DArray				m_swizzledTexture;
	vector<Iteration>				m_iterations;
};

vector<float> TextureGather2DArrayCase::computeQuadTexCoord (int iterationNdx) const
{
	vector<float> res;
	glu::TextureTestUtil::computeQuadTexCoord2DArray(res, m_iterations[iterationNdx].layerNdx, Vec2(-0.3f, -0.4f), Vec2(1.5f, 1.6f));
	return res;
}

void TextureGather2DArrayCase::generateIterations (void)
{
	DE_ASSERT(m_iterations.empty());

	const vector<GatherArgs> basicIterations = generateBasic2DCaseIterations(m_gatherType, m_textureFormat, getOffsetRange());

	// \note Out-of-bounds layer indices are tested too.
	for (int layerNdx = -1; layerNdx < m_textureSize.z()+1; layerNdx++)
	{
		// Don't duplicate all cases for all layers.
		if (layerNdx == 0)
		{
			for (int basicNdx = 0; basicNdx < (int)basicIterations.size(); basicNdx++)
			{
				m_iterations.push_back(Iteration());
				m_iterations.back().gatherArgs = basicIterations[basicNdx];
				m_iterations.back().layerNdx = layerNdx;
			}
		}
		else
		{
			// For other layers than 0, only test one component and one set of offsets per layer.
			for (int basicNdx = 0; basicNdx < (int)basicIterations.size(); basicNdx++)
			{
				if (isDepthFormat(m_textureFormat) || basicIterations[basicNdx].componentNdx == (layerNdx + 2) % 4)
				{
					m_iterations.push_back(Iteration());
					m_iterations.back().gatherArgs = basicIterations[basicNdx];
					m_iterations.back().layerNdx = layerNdx;
					break;
				}
			}
		}
	}
}

void TextureGather2DArrayCase::createAndUploadTexture (void)
{
	TestLog&						log			= m_testCtx.getLog();
	const glu::RenderContext&		renderCtx	= m_context.getRenderContext();
	const glw::Functions&			gl			= renderCtx.getFunctions();
	const tcu::TextureFormatInfo	texFmtInfo	= tcu::getTextureFormatInfo(m_textureFormat);

	m_texture = MovePtr<glu::Texture2DArray>(new glu::Texture2DArray(renderCtx, glu::getInternalFormat(m_textureFormat), m_textureSize.x(), m_textureSize.y(), m_textureSize.z()));

	{
		tcu::Texture2DArray&	refTexture	= m_texture->getRefTexture();
		const int				levelBegin	= m_baseLevel;
		const int				levelEnd	= isMipmapFilter(m_minFilter) && !(m_flags & GATHERCASE_MIPMAP_INCOMPLETE) ? refTexture.getNumLevels() : m_baseLevel+1;
		DE_ASSERT(m_baseLevel < refTexture.getNumLevels());

		for (int levelNdx = levelBegin; levelNdx < levelEnd; levelNdx++)
		{
			refTexture.allocLevel(levelNdx);
			const PixelBufferAccess& level = refTexture.getLevel(levelNdx);
			fillWithRandomColorTiles(level, texFmtInfo.valueMin, texFmtInfo.valueMax, (deUint32)m_testCtx.getCommandLine().getBaseSeed());

			log << TestLog::ImageSet("InputTextureLevel", "Input texture, level " + de::toString(levelNdx));
			for (int layerNdx = 0; layerNdx < m_textureSize.z(); layerNdx++)
				log << TestLog::Image("InputTextureLevel" + de::toString(layerNdx) + "Layer" + de::toString(layerNdx),
									  "Layer " + de::toString(layerNdx),
									  tcu::getSubregion(level, 0, 0, layerNdx, level.getWidth(), level.getHeight(), 1));
			log << TestLog::EndImageSet
				<< TestLog::Message << "Note: texture level's size is " << IVec3(level.getWidth(), level.getHeight(), level.getDepth()) << TestLog::EndMessage;
		}

		swizzleTexture(m_swizzledTexture, refTexture, m_textureSwizzle);
	}

	gl.activeTexture(GL_TEXTURE0);
	m_texture->upload();
}

bool TextureGather2DArrayCase::verify (int iterationNdx, const ConstPixelBufferAccess& rendered) const
{
	Vec3 texCoords[4];
	computeTexCoordVecs(computeQuadTexCoord(iterationNdx), texCoords);
	return TextureGatherCase::verify(rendered, getOneLevelSubView(tcu::Texture2DArrayView(m_swizzledTexture), m_baseLevel), texCoords, m_iterations[iterationNdx].gatherArgs);
}

// \note Cube case always uses just basic textureGather(); offset versions are not defined for cube maps.
class TextureGatherCubeCase : public TextureGatherCase
{
public:
	TextureGatherCubeCase (Context&						context,
						   const char*					name,
						   const char*					description,
						   tcu::TextureFormat			textureFormat,
						   tcu::Sampler::CompareMode	shadowCompareMode,
						   tcu::Sampler::WrapMode		wrapS,
						   tcu::Sampler::WrapMode		wrapT,
						   const MaybeTextureSwizzle&	texSwizzle,
						   tcu::Sampler::FilterMode		minFilter,
						   tcu::Sampler::FilterMode		magFilter,
						   int							baseLevel,
						   deUint32						flags,
						   int							textureSize)
		: TextureGatherCase		(context, name, description, TEXTURETYPE_CUBE, GATHERTYPE_BASIC, OFFSETSIZE_NONE, textureFormat, shadowCompareMode, wrapS, wrapT, texSwizzle, minFilter, magFilter, baseLevel, flags)
		, m_textureSize			(textureSize)
		, m_swizzledTexture		(tcu::TextureFormat(), 1)
	{
	}

protected:
	void						generateIterations		(void);
	void						createAndUploadTexture	(void);
	int							getNumIterations		(void) const { DE_ASSERT(!m_iterations.empty()); return (int)m_iterations.size(); }
	GatherArgs					getGatherArgs			(int iterationNdx) const { return m_iterations[iterationNdx].gatherArgs; }
	vector<float>				computeQuadTexCoord		(int iterationNdx) const;
	bool						verify					(int iterationNdx, const ConstPixelBufferAccess& rendered) const;

private:
	struct Iteration
	{
		GatherArgs		gatherArgs;
		tcu::CubeFace	face;
	};

	const int					m_textureSize;

	MovePtr<glu::TextureCube>	m_texture;
	tcu::TextureCube			m_swizzledTexture;
	vector<Iteration>			m_iterations;
};

vector<float> TextureGatherCubeCase::computeQuadTexCoord (int iterationNdx) const
{
	const bool		corners	= (m_flags & GATHERCASE_DONT_SAMPLE_CUBE_CORNERS) == 0;
	const Vec2		minC	= corners ? Vec2(-1.2f) : Vec2(-0.6f, -1.2f);
	const Vec2		maxC	= corners ? Vec2( 1.2f) : Vec2( 0.6f,  1.2f);
	vector<float>	res;
	glu::TextureTestUtil::computeQuadTexCoordCube(res, m_iterations[iterationNdx].face, minC, maxC);
	return res;
}

void TextureGatherCubeCase::generateIterations (void)
{
	DE_ASSERT(m_iterations.empty());

	const vector<GatherArgs> basicIterations = generateBasic2DCaseIterations(m_gatherType, m_textureFormat, getOffsetRange());

	for (int cubeFaceI = 0; cubeFaceI < tcu::CUBEFACE_LAST; cubeFaceI++)
	{
		const tcu::CubeFace cubeFace = (tcu::CubeFace)cubeFaceI;

		// Don't duplicate all cases for all faces.
		if (cubeFaceI == 0)
		{
			for (int basicNdx = 0; basicNdx < (int)basicIterations.size(); basicNdx++)
			{
				m_iterations.push_back(Iteration());
				m_iterations.back().gatherArgs = basicIterations[basicNdx];
				m_iterations.back().face = cubeFace;
			}
		}
		else
		{
			// For other faces than first, only test one component per face.
			for (int basicNdx = 0; basicNdx < (int)basicIterations.size(); basicNdx++)
			{
				if (isDepthFormat(m_textureFormat) || basicIterations[basicNdx].componentNdx == cubeFaceI % 4)
				{
					m_iterations.push_back(Iteration());
					m_iterations.back().gatherArgs = basicIterations[basicNdx];
					m_iterations.back().face = cubeFace;
					break;
				}
			}
		}
	}
}

void TextureGatherCubeCase::createAndUploadTexture (void)
{
	TestLog&						log			= m_testCtx.getLog();
	const glu::RenderContext&		renderCtx	= m_context.getRenderContext();
	const glw::Functions&			gl			= renderCtx.getFunctions();
	const tcu::TextureFormatInfo	texFmtInfo	= tcu::getTextureFormatInfo(m_textureFormat);

	m_texture = MovePtr<glu::TextureCube>(new glu::TextureCube(renderCtx, glu::getInternalFormat(m_textureFormat), m_textureSize));

	{
		tcu::TextureCube&	refTexture	= m_texture->getRefTexture();
		const int			levelBegin	= m_baseLevel;
		const int			levelEnd	= isMipmapFilter(m_minFilter) && !(m_flags & GATHERCASE_MIPMAP_INCOMPLETE) ? refTexture.getNumLevels() : m_baseLevel+1;
		DE_ASSERT(m_baseLevel < refTexture.getNumLevels());

		for (int levelNdx = levelBegin; levelNdx < levelEnd; levelNdx++)
		{
			log << TestLog::ImageSet("InputTextureLevel" + de::toString(levelNdx), "Input texture, level " + de::toString(levelNdx));

			for (int cubeFaceI = 0; cubeFaceI < tcu::CUBEFACE_LAST; cubeFaceI++)
			{
				const tcu::CubeFace			cubeFace	= (tcu::CubeFace)cubeFaceI;
				refTexture.allocLevel(cubeFace, levelNdx);
				const PixelBufferAccess&	levelFace	= refTexture.getLevelFace(levelNdx, cubeFace);
				fillWithRandomColorTiles(levelFace, texFmtInfo.valueMin, texFmtInfo.valueMax, (deUint32)m_testCtx.getCommandLine().getBaseSeed() ^ (deUint32)cubeFaceI);

				m_testCtx.getLog() << TestLog::Image("InputTextureLevel" + de::toString(levelNdx) + "Face" + de::toString((int)cubeFace),
													 de::toString(cubeFace),
													 levelFace);
			}

			log << TestLog::EndImageSet
				<< TestLog::Message << "Note: texture level's size is " << refTexture.getLevelFace(levelNdx, tcu::CUBEFACE_NEGATIVE_X).getWidth() << TestLog::EndMessage;
		}

		swizzleTexture(m_swizzledTexture, refTexture, m_textureSwizzle);
	}

	gl.activeTexture(GL_TEXTURE0);
	m_texture->upload();
}

bool TextureGatherCubeCase::verify (int iterationNdx, const ConstPixelBufferAccess& rendered) const
{
	Vec3 texCoords[4];
	computeTexCoordVecs(computeQuadTexCoord(iterationNdx), texCoords);
	return TextureGatherCase::verify(rendered, getOneLevelSubView(tcu::TextureCubeView(m_swizzledTexture), m_baseLevel), texCoords, m_iterations[iterationNdx].gatherArgs);
}

static inline TextureGatherCase* makeTextureGatherCase (TextureType					textureType,
														Context&					context,
														const char*					name,
														const char*					description,
														GatherType					gatherType,
														OffsetSize					offsetSize,
														tcu::TextureFormat			textureFormat,
														tcu::Sampler::CompareMode	shadowCompareMode,
														tcu::Sampler::WrapMode		wrapS,
														tcu::Sampler::WrapMode		wrapT,
														const MaybeTextureSwizzle&	texSwizzle,
														tcu::Sampler::FilterMode	minFilter,
														tcu::Sampler::FilterMode	magFilter,
														int							baseLevel,
														const IVec3&				textureSize,
														deUint32					flags = 0)
{
	switch (textureType)
	{
		case TEXTURETYPE_2D:
			return new TextureGather2DCase(context, name, description, gatherType, offsetSize, textureFormat, shadowCompareMode,
										   wrapS, wrapT, texSwizzle, minFilter, magFilter, baseLevel, flags, textureSize.swizzle(0, 1));

		case TEXTURETYPE_2D_ARRAY:
			return new TextureGather2DArrayCase(context, name, description, gatherType, offsetSize, textureFormat, shadowCompareMode,
												wrapS, wrapT, texSwizzle, minFilter, magFilter, baseLevel, flags, textureSize);

		case TEXTURETYPE_CUBE:
			DE_ASSERT(gatherType == GATHERTYPE_BASIC);
			DE_ASSERT(offsetSize == OFFSETSIZE_NONE);
			return new TextureGatherCubeCase(context, name, description, textureFormat, shadowCompareMode,
											 wrapS, wrapT, texSwizzle, minFilter, magFilter, baseLevel, flags, textureSize.x());

		default:
			DE_ASSERT(false);
			return DE_NULL;
	}
}

} // anonymous

TextureGatherTests::TextureGatherTests (Context& context)
	: TestCaseGroup(context, "gather", "textureGather* tests")
{
}

static inline const char* compareModeName (tcu::Sampler::CompareMode mode)
{
	switch (mode)
	{
		case tcu::Sampler::COMPAREMODE_LESS:				return "less";
		case tcu::Sampler::COMPAREMODE_LESS_OR_EQUAL:		return "less_or_equal";
		case tcu::Sampler::COMPAREMODE_GREATER:				return "greater";
		case tcu::Sampler::COMPAREMODE_GREATER_OR_EQUAL:	return "greater_or_equal";
		case tcu::Sampler::COMPAREMODE_EQUAL:				return "equal";
		case tcu::Sampler::COMPAREMODE_NOT_EQUAL:			return "not_equal";
		case tcu::Sampler::COMPAREMODE_ALWAYS:				return "always";
		case tcu::Sampler::COMPAREMODE_NEVER:				return "never";
		default: DE_ASSERT(false); return DE_NULL;
	}
}

void TextureGatherTests::init (void)
{
	const struct
	{
		const char* name;
		TextureType type;
	} textureTypes[] =
	{
		{ "2d",			TEXTURETYPE_2D			},
		{ "2d_array",	TEXTURETYPE_2D_ARRAY	},
		{ "cube",		TEXTURETYPE_CUBE		}
	};

	const struct
	{
		const char*			name;
		tcu::TextureFormat	format;
	} formats[] =
	{
		{ "rgba8",		tcu::TextureFormat(tcu::TextureFormat::RGBA,	tcu::TextureFormat::UNORM_INT8)		},
		{ "rgba8ui",	tcu::TextureFormat(tcu::TextureFormat::RGBA,	tcu::TextureFormat::UNSIGNED_INT8)	},
		{ "rgba8i",		tcu::TextureFormat(tcu::TextureFormat::RGBA,	tcu::TextureFormat::SIGNED_INT8)	},
		{ "depth32f",	tcu::TextureFormat(tcu::TextureFormat::D,		tcu::TextureFormat::FLOAT)			}
	};

	const struct
	{
		const char*		name;
		IVec3			size;
	} textureSizes[] =
	{
		{ "size_pot",	IVec3(64, 64, 3) },
		{ "size_npot",	IVec3(17, 23, 3) }
	};

	const struct
	{
		const char*				name;
		tcu::Sampler::WrapMode	mode;
	} wrapModes[] =
	{
		{ "clamp_to_edge",		tcu::Sampler::CLAMP_TO_EDGE			},
		{ "repeat",				tcu::Sampler::REPEAT_GL				},
		{ "mirrored_repeat",	tcu::Sampler::MIRRORED_REPEAT_GL	}
	};

	for (int gatherTypeI = 0; gatherTypeI < GATHERTYPE_LAST; gatherTypeI++)
	{
		const GatherType		gatherType			= (GatherType)gatherTypeI;
		TestCaseGroup* const	gatherTypeGroup		= new TestCaseGroup(m_context, gatherTypeName(gatherType), gatherTypeDescription(gatherType));
		addChild(gatherTypeGroup);

		for (int offsetSizeI = 0; offsetSizeI < OFFSETSIZE_LAST; offsetSizeI++)
		{
			const OffsetSize offsetSize = (OffsetSize)offsetSizeI;
			if ((gatherType == GATHERTYPE_BASIC) != (offsetSize == OFFSETSIZE_NONE))
				continue;

			TestCaseGroup* const offsetSizeGroup = offsetSize == OFFSETSIZE_NONE ?
													gatherTypeGroup :
													new TestCaseGroup(m_context,
																	  offsetSize == OFFSETSIZE_MINIMUM_REQUIRED				? "min_required_offset"
																	  : offsetSize == OFFSETSIZE_IMPLEMENTATION_MAXIMUM		? "implementation_offset"
																	  : DE_NULL,
																	  offsetSize == OFFSETSIZE_MINIMUM_REQUIRED				? "Use offsets within GL minimum required range"
																	  : offsetSize == OFFSETSIZE_IMPLEMENTATION_MAXIMUM		? "Use offsets within the implementation range"
																	  : DE_NULL);
			if (offsetSizeGroup != gatherTypeGroup)
				gatherTypeGroup->addChild(offsetSizeGroup);

			for (int textureTypeNdx = 0; textureTypeNdx < DE_LENGTH_OF_ARRAY(textureTypes); textureTypeNdx++)
			{
				const TextureType textureType = textureTypes[textureTypeNdx].type;

				if (textureType == TEXTURETYPE_CUBE && gatherType != GATHERTYPE_BASIC)
					continue;

				TestCaseGroup* const textureTypeGroup = new TestCaseGroup(m_context, textureTypes[textureTypeNdx].name, "");
				offsetSizeGroup->addChild(textureTypeGroup);

				for (int formatNdx = 0; formatNdx < DE_LENGTH_OF_ARRAY(formats); formatNdx++)
				{
					const tcu::TextureFormat&	format			= formats[formatNdx].format;
					TestCaseGroup* const		formatGroup		= new TestCaseGroup(m_context, formats[formatNdx].name, "");
					textureTypeGroup->addChild(formatGroup);

					for (int noCornersI = 0; noCornersI <= ((textureType == TEXTURETYPE_CUBE)?1:0); noCornersI++)
					{
						const bool				noCorners		= noCornersI!= 0;
						TestCaseGroup* const	cornersGroup	= noCorners
																? new TestCaseGroup(m_context, "no_corners", "Test case variants that don't sample around cube map corners")
																: formatGroup;

						if (formatGroup != cornersGroup)
							formatGroup->addChild(cornersGroup);

						for (int textureSizeNdx = 0; textureSizeNdx < DE_LENGTH_OF_ARRAY(textureSizes); textureSizeNdx++)
						{
							const IVec3&			textureSize			= textureSizes[textureSizeNdx].size;
							TestCaseGroup* const	textureSizeGroup	= new TestCaseGroup(m_context, textureSizes[textureSizeNdx].name, "");
							cornersGroup->addChild(textureSizeGroup);

							for (int compareModeI = 0; compareModeI < tcu::Sampler::COMPAREMODE_LAST; compareModeI++)
							{
								const tcu::Sampler::CompareMode compareMode = (tcu::Sampler::CompareMode)compareModeI;

								if ((compareMode != tcu::Sampler::COMPAREMODE_NONE) != isDepthFormat(format))
									continue;

								if (compareMode != tcu::Sampler::COMPAREMODE_NONE &&
									compareMode != tcu::Sampler::COMPAREMODE_LESS &&
									compareMode != tcu::Sampler::COMPAREMODE_GREATER)
									continue;

								TestCaseGroup* const compareModeGroup = compareMode == tcu::Sampler::COMPAREMODE_NONE ?
																			textureSizeGroup :
																			new TestCaseGroup(m_context,
																							  (string() + "compare_" + compareModeName(compareMode)).c_str(),
																							  "");
								if (compareModeGroup != textureSizeGroup)
									textureSizeGroup->addChild(compareModeGroup);

								for (int wrapCaseNdx = 0; wrapCaseNdx < DE_LENGTH_OF_ARRAY(wrapModes); wrapCaseNdx++)
								{
									const int						wrapSNdx	= wrapCaseNdx;
									const int						wrapTNdx	= (wrapCaseNdx + 1) % DE_LENGTH_OF_ARRAY(wrapModes);
									const tcu::Sampler::WrapMode	wrapS		= wrapModes[wrapSNdx].mode;
									const tcu::Sampler::WrapMode	wrapT		= wrapModes[wrapTNdx].mode;

									const string caseName = string() + wrapModes[wrapSNdx].name + "_" + wrapModes[wrapTNdx].name;

									compareModeGroup->addChild(makeTextureGatherCase(textureType, m_context, caseName.c_str(), "", gatherType, offsetSize, format, compareMode, wrapS, wrapT,
																					 MaybeTextureSwizzle::createNoneTextureSwizzle(), tcu::Sampler::NEAREST, tcu::Sampler::NEAREST, 0, textureSize,
																					 noCorners ? GATHERCASE_DONT_SAMPLE_CUBE_CORNERS : 0));
								}
							}
						}
					}

					if (offsetSize != OFFSETSIZE_MINIMUM_REQUIRED) // Don't test all features for both offset size types, as they should be rather orthogonal.
					{
						if (!isDepthFormat(format))
						{
							TestCaseGroup* const swizzleGroup = new TestCaseGroup(m_context, "texture_swizzle", "");
							formatGroup->addChild(swizzleGroup);

							DE_STATIC_ASSERT(TEXTURESWIZZLECOMPONENT_R == 0);
							for (int swizzleCaseNdx = 0; swizzleCaseNdx < TEXTURESWIZZLECOMPONENT_LAST; swizzleCaseNdx++)
							{
								MaybeTextureSwizzle	swizzle	= MaybeTextureSwizzle::createSomeTextureSwizzle();
								string				caseName;

								for (int i = 0; i < 4; i++)
								{
									swizzle.getSwizzle()[i] = (TextureSwizzleComponent)((swizzleCaseNdx + i) % (int)TEXTURESWIZZLECOMPONENT_LAST);
									caseName += (i > 0 ? "_" : "") + de::toLower(de::toString(swizzle.getSwizzle()[i]));
								}

								swizzleGroup->addChild(makeTextureGatherCase(textureType, m_context, caseName.c_str(), "", gatherType, offsetSize, format,
																			 tcu::Sampler::COMPAREMODE_NONE, tcu::Sampler::REPEAT_GL, tcu::Sampler::REPEAT_GL,
																			 swizzle, tcu::Sampler::NEAREST, tcu::Sampler::NEAREST, 0, IVec3(64, 64, 3)));
							}
						}

						{
							TestCaseGroup* const filterModeGroup = new TestCaseGroup(m_context, "filter_mode", "Test that filter modes have no effect");
							formatGroup->addChild(filterModeGroup);

							const struct
							{
								const char*					name;
								tcu::Sampler::FilterMode	filter;
							} magFilters[] =
							{
								{ "linear",		tcu::Sampler::LINEAR	},
								{ "nearest",	tcu::Sampler::NEAREST	}
							};

							const struct
							{
								const char*					name;
								tcu::Sampler::FilterMode	filter;
							} minFilters[] =
							{
								// \note Don't test NEAREST here, as it's covered by other cases.
								{ "linear",						tcu::Sampler::LINEAR					},
								{ "nearest_mipmap_nearest",		tcu::Sampler::NEAREST_MIPMAP_NEAREST	},
								{ "nearest_mipmap_linear",		tcu::Sampler::NEAREST_MIPMAP_LINEAR		},
								{ "linear_mipmap_nearest",		tcu::Sampler::LINEAR_MIPMAP_NEAREST		},
								{ "linear_mipmap_linear",		tcu::Sampler::LINEAR_MIPMAP_LINEAR		},
							};

							for (int minFilterNdx = 0; minFilterNdx < DE_LENGTH_OF_ARRAY(minFilters); minFilterNdx++)
							for (int magFilterNdx = 0; magFilterNdx < DE_LENGTH_OF_ARRAY(magFilters); magFilterNdx++)
							{
								const tcu::Sampler::FilterMode		minFilter		= minFilters[minFilterNdx].filter;
								const tcu::Sampler::FilterMode		magFilter		= magFilters[magFilterNdx].filter;
								const tcu::Sampler::CompareMode		compareMode		= isDepthFormat(format) ? tcu::Sampler::COMPAREMODE_LESS : tcu::Sampler::COMPAREMODE_NONE;

								if ((isUnormFormatType(format.type) || isDepthFormat(format)) && magFilter == tcu::Sampler::NEAREST)
									continue; // Covered by other cases.
								if ((isUIntFormatType(format.type) || isSIntFormatType(format.type)) &&
									(magFilter != tcu::Sampler::NEAREST || minFilter != tcu::Sampler::NEAREST_MIPMAP_NEAREST))
									continue;

								const string caseName = string() + "min_" + minFilters[minFilterNdx].name + "_mag_" + magFilters[magFilterNdx].name;

								filterModeGroup->addChild(makeTextureGatherCase(textureType, m_context, caseName.c_str(), "", gatherType, offsetSize, format, compareMode,
																				tcu::Sampler::REPEAT_GL, tcu::Sampler::REPEAT_GL, MaybeTextureSwizzle::createNoneTextureSwizzle(),
																				minFilter, magFilter, 0, IVec3(64, 64, 3)));
							}
						}

						{
							TestCaseGroup* const baseLevelGroup = new TestCaseGroup(m_context, "base_level", "");
							formatGroup->addChild(baseLevelGroup);

							for (int baseLevel = 1; baseLevel <= 2; baseLevel++)
							{
								const string						caseName		= "level_" + de::toString(baseLevel);
								const tcu::Sampler::CompareMode		compareMode		= isDepthFormat(format) ? tcu::Sampler::COMPAREMODE_LESS : tcu::Sampler::COMPAREMODE_NONE;
								baseLevelGroup->addChild(makeTextureGatherCase(textureType, m_context, caseName.c_str(), "", gatherType, offsetSize, format,
																			   compareMode, tcu::Sampler::REPEAT_GL, tcu::Sampler::REPEAT_GL,
																			   MaybeTextureSwizzle::createNoneTextureSwizzle(), tcu::Sampler::NEAREST, tcu::Sampler::NEAREST,
																			   baseLevel, IVec3(64, 64, 3)));
							}
						}

						// What shadow textures should return for incomplete textures is unclear.
						// Integer and unsigned integer lookups from incomplete textures return undefined values.
						if (!isDepthFormat(format) && !isSIntFormatType(format.type) && !isUIntFormatType(format.type))
						{
							TestCaseGroup* const incompleteGroup = new TestCaseGroup(m_context, "incomplete", "Test that textureGather* takes components from (0,0,0,1) for incomplete textures");
							formatGroup->addChild(incompleteGroup);

							const tcu::Sampler::CompareMode compareMode = isDepthFormat(format) ? tcu::Sampler::COMPAREMODE_LESS : tcu::Sampler::COMPAREMODE_NONE;
							incompleteGroup->addChild(makeTextureGatherCase(textureType, m_context, "mipmap_incomplete", "", gatherType, offsetSize, format,
																			compareMode, tcu::Sampler::REPEAT_GL, tcu::Sampler::REPEAT_GL,
																			MaybeTextureSwizzle::createNoneTextureSwizzle(), tcu::Sampler::NEAREST_MIPMAP_NEAREST, tcu::Sampler::NEAREST,
																			0, IVec3(64, 64, 3), GATHERCASE_MIPMAP_INCOMPLETE));
						}
					}
				}
			}
		}
	}
}

} // Functional
} // gles31
} // deqp
