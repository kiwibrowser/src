/*-------------------------------------------------------------------------
 * drawElements Quality Program OpenGL ES 3.1 Module
 * -------------------------------------------------
 *
 * Copyright 2015 The Android Open Source Project
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
 * \brief Texture border clamp tests.
 *//*--------------------------------------------------------------------*/

#include "es31fTextureBorderClampTests.hpp"

#include "glsTextureTestUtil.hpp"

#include "tcuTextureUtil.hpp"
#include "tcuTexLookupVerifier.hpp"
#include "tcuTexCompareVerifier.hpp"
#include "tcuCompressedTexture.hpp"
#include "tcuResultCollector.hpp"
#include "tcuSurface.hpp"
#include "tcuSeedBuilder.hpp"
#include "tcuVectorUtil.hpp"

#include "rrGenericVector.hpp"

#include "gluContextInfo.hpp"
#include "gluTexture.hpp"
#include "gluTextureUtil.hpp"
#include "gluPixelTransfer.hpp"
#include "gluStrUtil.hpp"
#include "gluObjectWrapper.hpp"
#include "gluShaderProgram.hpp"
#include "gluDrawUtil.hpp"

#include "glwEnums.hpp"
#include "glwFunctions.hpp"

#include "deStringUtil.hpp"
#include "deUniquePtr.hpp"
#include "deRandom.hpp"

#include <limits>


namespace deqp
{
namespace gles31
{
namespace Functional
{
namespace
{

enum SizeType
{
	SIZE_POT = 0,
	SIZE_NPOT
};

bool filterRequiresFilterability (deUint32 filter)
{
	switch (filter)
	{
		case GL_NEAREST:
		case GL_NEAREST_MIPMAP_NEAREST:
			return false;

		case GL_LINEAR:
		case GL_LINEAR_MIPMAP_NEAREST:
		case GL_NEAREST_MIPMAP_LINEAR:
		case GL_LINEAR_MIPMAP_LINEAR:
			return true;

		default:
			DE_ASSERT(false);
			return false;
	}
}

bool isDepthFormat (deUint32 format, tcu::Sampler::DepthStencilMode mode)
{
	if (format == GL_LUMINANCE || format == GL_LUMINANCE_ALPHA || format == GL_ALPHA || format == GL_BGRA)
	{
		// Unsized formats are a special case
		return false;
	}
	else if (glu::isCompressedFormat(format))
	{
		// no known compressed depth formats
		return false;
	}
	else
	{
		const tcu::TextureFormat fmt = glu::mapGLInternalFormat(format);

		if (fmt.order == tcu::TextureFormat::D)
		{
			DE_ASSERT(mode == tcu::Sampler::MODE_DEPTH);
			return true;
		}
		else if (fmt.order == tcu::TextureFormat::DS && mode == tcu::Sampler::MODE_DEPTH)
			return true;
		else
			return false;
	}
}

bool isStencilFormat (deUint32 format, tcu::Sampler::DepthStencilMode mode)
{
	if (format == GL_LUMINANCE || format == GL_LUMINANCE_ALPHA || format == GL_ALPHA || format == GL_BGRA)
	{
		// Unsized formats are a special case
		return false;
	}
	else if (glu::isCompressedFormat(format))
	{
		// no known compressed stencil formats
		return false;
	}
	else
	{
		const tcu::TextureFormat fmt = glu::mapGLInternalFormat(format);

		if (fmt.order == tcu::TextureFormat::S)
		{
			DE_ASSERT(mode == tcu::Sampler::MODE_STENCIL);
			return true;
		}
		else if (fmt.order == tcu::TextureFormat::DS && mode == tcu::Sampler::MODE_STENCIL)
			return true;
		else
			return false;
	}
}

tcu::TextureChannelClass getFormatChannelClass (deUint32 format, tcu::Sampler::DepthStencilMode mode)
{
	if (format == GL_LUMINANCE || format == GL_LUMINANCE_ALPHA || format == GL_ALPHA || format == GL_BGRA)
	{
		// Unsized formats are a special c, use UNORM8
		return tcu::TEXTURECHANNELCLASS_UNSIGNED_FIXED_POINT;
	}
	else if (glu::isCompressedFormat(format))
	{
		const tcu::CompressedTexFormat	compressedFmt	= glu::mapGLCompressedTexFormat(format);
		const tcu::TextureFormat		uncompressedFmt	= tcu::getUncompressedFormat(compressedFmt);
		return tcu::getTextureChannelClass(uncompressedFmt.type);
	}
	else
	{
		const tcu::TextureFormat fmt			= glu::mapGLInternalFormat(format);
		const tcu::TextureFormat effectiveFmt	= tcu::getEffectiveDepthStencilTextureFormat(fmt, mode);

		return tcu::getTextureChannelClass(effectiveFmt.type);
	}
}

int getDimensionNumBlocks (int dimensionSize, int blockSize)
{
	// ceil( a / b )
	return (dimensionSize + blockSize - 1) / blockSize;
}

void generateDummyCompressedData (tcu::CompressedTexture& dst, const tcu::CompressedTexFormat& format)
{
	const int			blockByteSize	= tcu::getBlockSize(format);
	const tcu::IVec3	blockPixelSize	= tcu::getBlockPixelSize(format);
	const tcu::IVec3	numBlocks		(getDimensionNumBlocks(dst.getWidth(),    blockPixelSize.x()),
										 getDimensionNumBlocks(dst.getHeight(),   blockPixelSize.y()),
										 getDimensionNumBlocks(dst.getDepth(),    blockPixelSize.z()));
	const int			numTotalBlocks	= numBlocks.x() * numBlocks.y() * numBlocks.z();
	const int			dataSize		= numTotalBlocks * blockByteSize;

	DE_ASSERT(dst.getDataSize() == dataSize);

	if (tcu::isAstcFormat(format))
	{
		// generate data that is valid in LDR mode
		const int		BLOCK_SIZE			= 16;
		const deUint8	block[BLOCK_SIZE]	= { 252, 253, 255, 255, 255, 255, 255, 255, 223, 251, 28, 206, 54, 251, 160, 174 };

		DE_ASSERT(blockByteSize == BLOCK_SIZE);
		for (int ndx = 0; ndx < numTotalBlocks; ++ndx)
			deMemcpy((deUint8*)dst.getData() + ndx * BLOCK_SIZE, block, BLOCK_SIZE);
	}
	else
	{
		// any data is ok
		de::Random rnd(0xabc);

		for (int ndx = 0; ndx < dataSize; ++ndx)
			((deUint8*)dst.getData())[ndx] = rnd.getUint8();
	}
}

template <typename T>
struct TextureTraits
{
};

template <>
struct TextureTraits<glu::Texture2D>
{
	typedef tcu::IVec2 SizeType;

	static de::MovePtr<glu::Texture2D> createTextureFromInternalFormat (glu::RenderContext& renderCtx, deUint32 texFormat, const tcu::IVec2& size)
	{
		return de::MovePtr<glu::Texture2D>(new glu::Texture2D(renderCtx, texFormat, size.x(), size.y()));
	}

	static de::MovePtr<glu::Texture2D> createTextureFromFormatAndType (glu::RenderContext& renderCtx, deUint32 texFormat, deUint32 type, const tcu::IVec2& size)
	{
		return de::MovePtr<glu::Texture2D>(new glu::Texture2D(renderCtx, texFormat, type, size.x(), size.y()));
	}

	static de::MovePtr<glu::Texture2D> createTextureFromCompressedData (glu::RenderContext&					renderCtx,
																		const glu::ContextInfo&				ctxInfo,
																		const tcu::CompressedTexture&		compressedLevel,
																		const tcu::TexDecompressionParams&	decompressionParams)
	{
		return de::MovePtr<glu::Texture2D>(new glu::Texture2D(renderCtx,
															  ctxInfo,
															  1,
															  &compressedLevel,
															  decompressionParams));
	}

	static int getTextureNumLayers (const tcu::IVec2& size)
	{
		// 2D textures have one layer
		DE_UNREF(size);
		return 1;
	}
};

template <>
struct TextureTraits<glu::Texture3D>
{
	typedef tcu::IVec3 SizeType;

	static de::MovePtr<glu::Texture3D> createTextureFromInternalFormat (glu::RenderContext& renderCtx, deUint32 texFormat, const tcu::IVec3& size)
	{
		return de::MovePtr<glu::Texture3D>(new glu::Texture3D(renderCtx, texFormat, size.x(), size.y(), size.z()));
	}

	static de::MovePtr<glu::Texture3D> createTextureFromFormatAndType (glu::RenderContext& renderCtx, deUint32 texFormat, deUint32 type, const tcu::IVec3& size)
	{
		return de::MovePtr<glu::Texture3D>(new glu::Texture3D(renderCtx, texFormat, type, size.x(), size.y(), size.z()));
	}

	static de::MovePtr<glu::Texture3D> createTextureFromCompressedData (glu::RenderContext&					renderCtx,
																		const glu::ContextInfo&				ctxInfo,
																		const tcu::CompressedTexture&		compressedLevel,
																		const tcu::TexDecompressionParams&	decompressionParams)
	{
		return de::MovePtr<glu::Texture3D>(new glu::Texture3D(renderCtx,
															  ctxInfo,
															  1,
															  &compressedLevel,
															  decompressionParams));
	}

	static int getTextureNumLayers (const tcu::IVec3& size)
	{
		// 3D textures have Z layers
		return size.z();
	}
};

template <typename T>
de::MovePtr<T> genDummyTexture (glu::RenderContext& renderCtx, const glu::ContextInfo& ctxInfo, deUint32 texFormat, const typename TextureTraits<T>::SizeType& size)
{
	de::MovePtr<T> texture;

	if (isDepthFormat(texFormat, tcu::Sampler::MODE_DEPTH) || isStencilFormat(texFormat, tcu::Sampler::MODE_STENCIL))
	{
		// fill different channels with different gradients
		texture = TextureTraits<T>::createTextureFromInternalFormat(renderCtx, texFormat, size);
		texture->getRefTexture().allocLevel(0);

		if (isDepthFormat(texFormat, tcu::Sampler::MODE_DEPTH))
		{
			// fill depth with 0 -> 1
			const tcu::PixelBufferAccess depthAccess = tcu::getEffectiveDepthStencilAccess(texture->getRefTexture().getLevel(0), tcu::Sampler::MODE_DEPTH);
			tcu::fillWithComponentGradients(depthAccess, tcu::Vec4(0.0f), tcu::Vec4(1.0f));
		}

		if (isStencilFormat(texFormat, tcu::Sampler::MODE_STENCIL))
		{
			// fill stencil with 0 -> max
			const tcu::PixelBufferAccess	stencilAccess	= tcu::getEffectiveDepthStencilAccess(texture->getRefTexture().getLevel(0), tcu::Sampler::MODE_STENCIL);
			const tcu::TextureFormatInfo	texFormatInfo	= tcu::getTextureFormatInfo(stencilAccess.getFormat());

			// Flip y to make stencil and depth cases not look identical
			tcu::fillWithComponentGradients(tcu::flipYAccess(stencilAccess), texFormatInfo.valueMax, texFormatInfo.valueMin);
		}

		texture->upload();
	}
	else if (!glu::isCompressedFormat(texFormat))
	{
		if (texFormat == GL_LUMINANCE || texFormat == GL_LUMINANCE_ALPHA || texFormat == GL_ALPHA || texFormat == GL_BGRA)
			texture = TextureTraits<T>::createTextureFromFormatAndType(renderCtx, texFormat, GL_UNSIGNED_BYTE, size);
		else
			texture = TextureTraits<T>::createTextureFromInternalFormat(renderCtx, texFormat, size);

		// Fill level 0.
		texture->getRefTexture().allocLevel(0);

		// fill with gradient min -> max
		{
			const tcu::TextureFormatInfo	texFormatInfo	= tcu::getTextureFormatInfo(texture->getRefTexture().getFormat());
			const tcu::Vec4					rampLow			= texFormatInfo.valueMin;
			const tcu::Vec4					rampHigh		= texFormatInfo.valueMax;
			tcu::fillWithComponentGradients(texture->getRefTexture().getLevel(0), rampLow, rampHigh);
		}

		texture->upload();
	}
	else
	{
		const tcu::CompressedTexFormat	compressedFormat	= glu::mapGLCompressedTexFormat(texFormat);
		const int						numLayers			= TextureTraits<T>::getTextureNumLayers(size);
		tcu::CompressedTexture			compressedLevel		(compressedFormat, size.x(), size.y(), numLayers);
		const bool						isAstcFormat		= tcu::isAstcFormat(compressedFormat);
		tcu::TexDecompressionParams		decompressionParams	((isAstcFormat) ? (tcu::TexDecompressionParams::ASTCMODE_LDR) : (tcu::TexDecompressionParams::ASTCMODE_LAST));

		generateDummyCompressedData(compressedLevel, compressedFormat);

		texture = TextureTraits<T>::createTextureFromCompressedData(renderCtx,
																	ctxInfo,
																	compressedLevel,
																	decompressionParams);
	}

	return texture;
}

int getNBitIntegerMaxValue (bool isSigned, int numBits)
{
	DE_ASSERT(numBits < 32);

	if (numBits == 0)
		return 0;
	else if (isSigned)
		return deIntMaxValue32(numBits);
	else
		return deUintMaxValue32(numBits);
}

int getNBitIntegerMinValue (bool isSigned, int numBits)
{
	DE_ASSERT(numBits < 32);

	if (numBits == 0)
		return 0;
	else if (isSigned)
		return deIntMinValue32(numBits);
	else
		return 0;
}

tcu::IVec4 getNBitIntegerVec4MaxValue (bool isSigned, const tcu::IVec4& numBits)
{
	return tcu::IVec4(getNBitIntegerMaxValue(isSigned, numBits[0]),
					  getNBitIntegerMaxValue(isSigned, numBits[1]),
					  getNBitIntegerMaxValue(isSigned, numBits[2]),
					  getNBitIntegerMaxValue(isSigned, numBits[3]));
}

tcu::IVec4 getNBitIntegerVec4MinValue (bool isSigned, const tcu::IVec4& numBits)
{
	return tcu::IVec4(getNBitIntegerMinValue(isSigned, numBits[0]),
					  getNBitIntegerMinValue(isSigned, numBits[1]),
					  getNBitIntegerMinValue(isSigned, numBits[2]),
					  getNBitIntegerMinValue(isSigned, numBits[3]));
}

rr::GenericVec4 mapToFormatColorUnits (const tcu::TextureFormat& texFormat, const tcu::Vec4& normalizedRange)
{
	const tcu::TextureFormatInfo texFormatInfo = tcu::getTextureFormatInfo(texFormat);

	switch (tcu::getTextureChannelClass(texFormat.type))
	{
		case tcu::TEXTURECHANNELCLASS_UNSIGNED_FIXED_POINT:		return rr::GenericVec4(normalizedRange);
		case tcu::TEXTURECHANNELCLASS_SIGNED_FIXED_POINT:		return rr::GenericVec4(normalizedRange * 2.0f - 1.0f);
		case tcu::TEXTURECHANNELCLASS_FLOATING_POINT:			return rr::GenericVec4(texFormatInfo.valueMin + normalizedRange * texFormatInfo.valueMax);
		case tcu::TEXTURECHANNELCLASS_SIGNED_INTEGER:			return rr::GenericVec4(tcu::mix(texFormatInfo.valueMin, texFormatInfo.valueMax, normalizedRange).cast<deInt32>());
		case tcu::TEXTURECHANNELCLASS_UNSIGNED_INTEGER:			return rr::GenericVec4(tcu::mix(texFormatInfo.valueMin, texFormatInfo.valueMax, normalizedRange).cast<deUint32>());

		default:
			DE_ASSERT(false);
			return rr::GenericVec4();
	}
}

rr::GenericVec4 mapToFormatColorRepresentable (const tcu::TextureFormat& texFormat, const tcu::Vec4& normalizedRange)
{
	// make sure value is representable in the target format and clear channels
	// not present in the target format.

	const rr::GenericVec4		inFormatUnits	= mapToFormatColorUnits(texFormat, normalizedRange);
	const tcu::BVec4			channelMask		= tcu::getTextureFormatChannelMask(texFormat);
	de::ArrayBuffer<deUint8, 4>	buffer			(texFormat.getPixelSize());
	tcu::PixelBufferAccess		access			(texFormat, tcu::IVec3(1, 1, 1), buffer.getPtr());

	if (tcu::isSRGB(texFormat))
	{
		DE_ASSERT(texFormat.type == tcu::TextureFormat::UNORM_INT8);

		// make sure border color (in linear space) can be converted to 8-bit sRGB space without
		// significant loss.
		const tcu::Vec4		sRGB		= tcu::linearToSRGB(normalizedRange);
		const tcu::IVec4	sRGB8		= tcu::IVec4(tcu::floatToU8(sRGB[0]),
													 tcu::floatToU8(sRGB[1]),
													 tcu::floatToU8(sRGB[2]),
													 tcu::floatToU8(sRGB[3]));
		const tcu::Vec4		linearized	= tcu::sRGBToLinear(tcu::Vec4((float)sRGB8[0] / 255.0f,
																	  (float)sRGB8[1] / 255.0f,
																	  (float)sRGB8[2] / 255.0f,
																	  (float)sRGB8[3] / 255.0f));

		return rr::GenericVec4(tcu::select(linearized, tcu::Vec4(0.0f), channelMask));
	}

	switch (tcu::getTextureChannelClass(texFormat.type))
	{
		case tcu::TEXTURECHANNELCLASS_SIGNED_FIXED_POINT:
		case tcu::TEXTURECHANNELCLASS_UNSIGNED_FIXED_POINT:
		case tcu::TEXTURECHANNELCLASS_FLOATING_POINT:
		{
			access.setPixel(inFormatUnits.get<float>(), 0, 0);
			return rr::GenericVec4(tcu::select(access.getPixel(0, 0), tcu::Vec4(0.0f), channelMask));
		}
		case tcu::TEXTURECHANNELCLASS_SIGNED_INTEGER:
		{
			access.setPixel(inFormatUnits.get<deInt32>(), 0, 0);
			return rr::GenericVec4(tcu::select(access.getPixelInt(0, 0), tcu::IVec4(0), channelMask));
		}
		case tcu::TEXTURECHANNELCLASS_UNSIGNED_INTEGER:
		{
			access.setPixel(inFormatUnits.get<deUint32>(), 0, 0);
			return rr::GenericVec4(tcu::select(access.getPixelUint(0, 0), tcu::UVec4(0u), channelMask));
		}
		default:
		{
			DE_ASSERT(false);
			return rr::GenericVec4();
		}
	}
}

bool isCoreFilterableFormat (deUint32 format, tcu::Sampler::DepthStencilMode mode)
{
	const bool	isLuminanceOrAlpha		= (format == GL_LUMINANCE || format == GL_ALPHA || format == GL_LUMINANCE_ALPHA); // special case for luminance/alpha
	const bool	isUnsizedColorFormat	= (format == GL_BGRA);
	const bool	isCompressed			= glu::isCompressedFormat(format);
	const bool	isDepth					= isDepthFormat(format, mode);
	const bool	isStencil				= isStencilFormat(format, mode);

	// special cases
	if (isLuminanceOrAlpha || isUnsizedColorFormat || isCompressed)
		return true;
	if (isStencil || isDepth)
		return false;

	// color case
	return glu::isGLInternalColorFormatFilterable(format);
}

class TextureBorderClampTest : public TestCase
{
public:
	enum StateType
	{
		STATE_SAMPLER_PARAM = 0,
		STATE_TEXTURE_PARAM,

		STATE_LAST
	};

	enum SamplingFunction
	{
		SAMPLE_FILTER = 0,
		SAMPLE_GATHER,

		SAMPLE_LAST
	};

	enum Flag
	{
		FLAG_USE_SHADOW_SAMPLER = (1u << 0),
	};

	struct IterationConfig
	{
		tcu::Vec2		p0;
		tcu::Vec2		p1;
		rr::GenericVec4	borderColor;
		tcu::Vec4		lookupScale;
		tcu::Vec4		lookupBias;
		deUint32		minFilter;
		deUint32		magFilter;
		std::string		description;
		deUint32		sWrapMode;
		deUint32		tWrapMode;
		deUint32		compareMode;
		float			compareRef;
	};

														TextureBorderClampTest		(Context&						context,
																					 const char*					name,
																					 const char*					description,
																					 deUint32						texFormat,
																					 tcu::Sampler::DepthStencilMode	mode,
																					 StateType						stateType,
																					 int							texWidth,
																					 int							texHeight,
																					 SamplingFunction				samplingFunction,
																					 deUint32						flags				= 0);
														~TextureBorderClampTest		(void);

protected:
	void												init						(void);
	void												deinit						(void);

private:
	IterateResult										iterate						(void);

	void												logParams					(const IterationConfig&							config,
																					 const glu::TextureTestUtil::ReferenceParams&	samplerParams);

	void												renderTo					(tcu::Surface&									surface,
																					 const IterationConfig&							config,
																					 const glu::TextureTestUtil::ReferenceParams&	samplerParams);
	void												renderQuad					(const float*									texCoord,
																					 const glu::TextureTestUtil::ReferenceParams&	samplerParams);

	void												verifyImage					(const tcu::Surface&							image,
																					 const IterationConfig&							config,
																					 const glu::TextureTestUtil::ReferenceParams&	samplerParams);

	bool												verifyTextureSampleResult	(const tcu::ConstPixelBufferAccess&				renderedFrame,
																					 const float*									texCoord,
																					 const glu::TextureTestUtil::ReferenceParams&	samplerParams,
																					 const tcu::LodPrecision&						lodPrecision,
																					 const tcu::LookupPrecision&					lookupPrecision);

	bool												verifyTextureCompareResult	(const tcu::ConstPixelBufferAccess&				renderedFrame,
																					 const float*									texCoord,
																					 const glu::TextureTestUtil::ReferenceParams&	samplerParams,
																					 const tcu::TexComparePrecision&				texComparePrecision,
																					 const tcu::TexComparePrecision&				lowQualityTexComparePrecision,
																					 const tcu::LodPrecision&						lodPrecision,
																					 const tcu::LodPrecision&						lowQualityLodPrecision);

	bool												verifyTextureGatherResult	(const tcu::ConstPixelBufferAccess&				renderedFrame,
																					 const float*									texCoord,
																					 const glu::TextureTestUtil::ReferenceParams&	samplerParams,
																					 const tcu::LookupPrecision&					lookupPrecision);

	bool												verifyTextureGatherCmpResult(const tcu::ConstPixelBufferAccess&				renderedFrame,
																					 const float*									texCoord,
																					 const glu::TextureTestUtil::ReferenceParams&	samplerParams,
																					 const tcu::TexComparePrecision&				texComparePrecision,
																					 const tcu::TexComparePrecision&				lowQualityTexComparePrecision);

	deUint32											getIterationSeed			(const IterationConfig& config) const;
	glu::TextureTestUtil::ReferenceParams				genSamplerParams			(const IterationConfig& config) const;
	glu::ShaderProgram*									genGatherProgram			(void) const;

	virtual int											getNumIterations			(void) const = 0;
	virtual IterationConfig								getIteration				(int ndx) const = 0;

protected:
	const glu::Texture2D*								getTexture						(void) const;

	const deUint32										m_texFormat;
	const tcu::Sampler::DepthStencilMode				m_sampleMode;
	const tcu::TextureChannelClass						m_channelClass;
	const StateType										m_stateType;

	const int											m_texHeight;
	const int											m_texWidth;

	const SamplingFunction								m_samplingFunction;
	const bool											m_useShadowSampler;
private:
	enum
	{
		VIEWPORT_WIDTH		= 128,
		VIEWPORT_HEIGHT		= 128,
	};

	de::MovePtr<glu::Texture2D>							m_texture;
	de::MovePtr<gls::TextureTestUtil::TextureRenderer>	m_renderer;
	de::MovePtr<glu::ShaderProgram>						m_gatherProgram;

	int													m_iterationNdx;
	tcu::ResultCollector								m_result;
};

TextureBorderClampTest::TextureBorderClampTest (Context&						context,
												const char*						name,
												const char*						description,
												deUint32						texFormat,
												tcu::Sampler::DepthStencilMode	mode,
												StateType						stateType,
												int								texWidth,
												int								texHeight,
												SamplingFunction				samplingFunction,
												deUint32						flags)
	: TestCase				(context, name, description)
	, m_texFormat			(texFormat)
	, m_sampleMode			(mode)
	, m_channelClass		(getFormatChannelClass(texFormat, mode))
	, m_stateType			(stateType)
	, m_texHeight			(texHeight)
	, m_texWidth			(texWidth)
	, m_samplingFunction	(samplingFunction)
	, m_useShadowSampler	((flags & FLAG_USE_SHADOW_SAMPLER) != 0)
	, m_iterationNdx		(0)
	, m_result				(context.getTestContext().getLog())
{
	DE_ASSERT(stateType < STATE_LAST);
	DE_ASSERT(samplingFunction < SAMPLE_LAST);
	// mode must be set for combined depth-stencil formats
	DE_ASSERT(m_channelClass != tcu::TEXTURECHANNELCLASS_LAST || mode != tcu::Sampler::MODE_LAST);
}

TextureBorderClampTest::~TextureBorderClampTest (void)
{
	deinit();
}

void TextureBorderClampTest::init (void)
{
	// requirements
	const bool supportsES32 = glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::es(3, 2));

	if (!supportsES32 && !m_context.getContextInfo().isExtensionSupported("GL_EXT_texture_border_clamp"))
		throw tcu::NotSupportedError("Test requires GL_EXT_texture_border_clamp extension");

	if (glu::isCompressedFormat(m_texFormat)													&&
		!supportsES32																			&&
		tcu::isAstcFormat(glu::mapGLCompressedTexFormat(m_texFormat))							&&
		!m_context.getContextInfo().isExtensionSupported("GL_KHR_texture_compression_astc_ldr"))
	{
		throw tcu::NotSupportedError("Test requires GL_KHR_texture_compression_astc_ldr extension");
	}

	if (m_texFormat == GL_BGRA && !m_context.getContextInfo().isExtensionSupported("GL_EXT_texture_format_BGRA8888"))
		throw tcu::NotSupportedError("Test requires GL_EXT_texture_format_BGRA8888 extension");

	if (m_context.getRenderTarget().getWidth() < VIEWPORT_WIDTH ||
		m_context.getRenderTarget().getHeight() < VIEWPORT_HEIGHT)
	{
		throw tcu::NotSupportedError("Test requires " + de::toString<int>(VIEWPORT_WIDTH) + "x" + de::toString<int>(VIEWPORT_HEIGHT) + " viewport");
	}

	// resources

	m_texture = genDummyTexture<glu::Texture2D>(m_context.getRenderContext(), m_context.getContextInfo(), m_texFormat, tcu::IVec2(m_texWidth, m_texHeight));

	m_testCtx.getLog()	<< tcu::TestLog::Message
						<< "Created texture with format " << glu::getTextureFormatName(m_texFormat)
						<< ", size (" << m_texture->getRefTexture().getWidth() << ", " << m_texture->getRefTexture().getHeight() << ")\n"
						<< "Setting sampling state using " << ((m_stateType == STATE_TEXTURE_PARAM) ? ("texture state") : ("sampler state"))
						<< tcu::TestLog::EndMessage;

	if (m_samplingFunction == SAMPLE_FILTER)
	{
		const glu::GLSLVersion glslVersion = glu::getContextTypeGLSLVersion(m_context.getRenderContext().getType());

		m_renderer = de::MovePtr<gls::TextureTestUtil::TextureRenderer>(new gls::TextureTestUtil::TextureRenderer(m_context.getRenderContext(), m_testCtx.getLog(), glslVersion, glu::PRECISION_HIGHP));
	}
	else
	{
		m_gatherProgram = de::MovePtr<glu::ShaderProgram>(genGatherProgram());

		m_testCtx.getLog()	<< tcu::TestLog::Message
							<< "Using texture gather to sample texture"
							<< tcu::TestLog::EndMessage
							<< *m_gatherProgram;

		if (!m_gatherProgram->isOk())
			throw tcu::TestError("failed to build program");
	}
}

void TextureBorderClampTest::deinit (void)
{
	m_texture.clear();
	m_renderer.clear();
	m_gatherProgram.clear();
}

TextureBorderClampTest::IterateResult TextureBorderClampTest::iterate (void)
{
	const IterationConfig						iterationConfig		= getIteration(m_iterationNdx);
	const std::string							iterationDesc		= "Iteration " + de::toString(m_iterationNdx+1) + (iterationConfig.description.empty() ? ("") : (" - " + iterationConfig.description));
	const tcu::ScopedLogSection					section				(m_testCtx.getLog(), "Iteration", iterationDesc);
	tcu::Surface								renderedFrame		(VIEWPORT_WIDTH, VIEWPORT_HEIGHT);
	const glu::TextureTestUtil::ReferenceParams	samplerParams		= genSamplerParams(iterationConfig);

	logParams(iterationConfig, samplerParams);
	renderTo(renderedFrame, iterationConfig, samplerParams);
	verifyImage(renderedFrame, iterationConfig, samplerParams);

	if (++m_iterationNdx == getNumIterations())
	{
		m_result.setTestContextResult(m_testCtx);
		return STOP;
	}
	return CONTINUE;
}

void TextureBorderClampTest::logParams (const IterationConfig& config, const glu::TextureTestUtil::ReferenceParams& samplerParams)
{
	const std::string				borderColorString	= (m_channelClass == tcu::TEXTURECHANNELCLASS_SIGNED_INTEGER)   ? (de::toString(config.borderColor.get<deInt32>()))
														: (m_channelClass == tcu::TEXTURECHANNELCLASS_UNSIGNED_INTEGER) ? (de::toString(config.borderColor.get<deUint32>()))
														:																  (de::toString(config.borderColor.get<float>()));

	m_testCtx.getLog()	<< tcu::TestLog::Message
						<< "Rendering full screen quad, tex coords bottom-left: " << config.p0 << ", top-right " << config.p1 << "\n"
						<< "Border color is " << borderColorString << "\n"
						<< "Texture lookup bias: " << samplerParams.colorBias << "\n"
						<< "Texture lookup scale: " << samplerParams.colorScale << "\n"
						<< "Filters: min = " << glu::getTextureFilterName(glu::getGLFilterMode(samplerParams.sampler.minFilter))
							<< ", mag = " << glu::getTextureFilterName(glu::getGLFilterMode(samplerParams.sampler.magFilter)) << "\n"
						<< "Wrap mode: s = " << glu::getRepeatModeStr(config.sWrapMode)
							<< ", t = " << glu::getRepeatModeStr(config.tWrapMode) << "\n"
						<< tcu::TestLog::EndMessage;

	if (m_sampleMode == tcu::Sampler::MODE_DEPTH)
		m_testCtx.getLog() << tcu::TestLog::Message << "Depth stencil texture mode is DEPTH_COMPONENT" << tcu::TestLog::EndMessage;
	else if (m_sampleMode == tcu::Sampler::MODE_STENCIL)
		m_testCtx.getLog() << tcu::TestLog::Message << "Depth stencil texture mode is STENCIL_INDEX" << tcu::TestLog::EndMessage;

	if (config.compareMode != GL_NONE)
	{
		m_testCtx.getLog()	<< tcu::TestLog::Message
							<< "Texture mode is COMPARE_REF_TO_TEXTURE, mode = " << glu::getCompareFuncStr(config.compareMode) << "\n"
							<< "Compare reference value = " << config.compareRef << "\n"
							<< tcu::TestLog::EndMessage;
	}
}

void TextureBorderClampTest::renderTo (tcu::Surface&								surface,
									   const IterationConfig&						config,
									   const glu::TextureTestUtil::ReferenceParams&	samplerParams)
{
	const glw::Functions&						gl			= m_context.getRenderContext().getFunctions();
	const gls::TextureTestUtil::RandomViewport	viewport	(m_context.getRenderTarget(), VIEWPORT_WIDTH, VIEWPORT_HEIGHT, getIterationSeed(config));
	std::vector<float>							texCoord;
	de::MovePtr<glu::Sampler>					sampler;

	glu::TextureTestUtil::computeQuadTexCoord2D(texCoord, config.p0, config.p1);

	// Bind to unit 0.
	gl.activeTexture(GL_TEXTURE0);
	gl.bindTexture(GL_TEXTURE_2D, m_texture->getGLTexture());

	if (m_sampleMode == tcu::Sampler::MODE_DEPTH)
		gl.texParameteri(GL_TEXTURE_2D, GL_DEPTH_STENCIL_TEXTURE_MODE, GL_DEPTH_COMPONENT);
	else if (m_sampleMode == tcu::Sampler::MODE_STENCIL)
		gl.texParameteri(GL_TEXTURE_2D, GL_DEPTH_STENCIL_TEXTURE_MODE, GL_STENCIL_INDEX);

	if (config.compareMode == GL_NONE)
	{
		gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);
		gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_ALWAYS);
	}
	else
	{
		gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
		gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, config.compareMode);
	}

	if (m_stateType == STATE_TEXTURE_PARAM)
	{
		// Setup filtering and wrap modes.
		gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,		glu::getGLWrapMode(samplerParams.sampler.wrapS));
		gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,		glu::getGLWrapMode(samplerParams.sampler.wrapT));
		gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,	glu::getGLFilterMode(samplerParams.sampler.minFilter));
		gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,	glu::getGLFilterMode(samplerParams.sampler.magFilter));

		switch (m_channelClass)
		{
			case tcu::TEXTURECHANNELCLASS_SIGNED_FIXED_POINT:
			case tcu::TEXTURECHANNELCLASS_UNSIGNED_FIXED_POINT:
			case tcu::TEXTURECHANNELCLASS_FLOATING_POINT:
				gl.texParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, config.borderColor.getAccess<float>());
				break;

			case tcu::TEXTURECHANNELCLASS_SIGNED_INTEGER:
				gl.texParameterIiv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, config.borderColor.getAccess<deInt32>());
				break;

			case tcu::TEXTURECHANNELCLASS_UNSIGNED_INTEGER:
				gl.texParameterIuiv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, config.borderColor.getAccess<deUint32>());
				break;

			default:
				DE_ASSERT(false);
		}
	}
	else if (m_stateType == STATE_SAMPLER_PARAM)
	{
		const tcu::Vec4 blue(0.0f, 0.0f, 1.0f, 1.0f);

		// Setup filtering and wrap modes to bad values
		gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,		GL_REPEAT);
		gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,		GL_REPEAT);
		gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,	GL_NEAREST);
		gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,	GL_NEAREST);
		gl.texParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, blue.getPtr()); // just set some unlikely color

		// setup sampler to correct values
		sampler = de::MovePtr<glu::Sampler>(new glu::Sampler(m_context.getRenderContext()));

		gl.samplerParameteri(**sampler, GL_TEXTURE_WRAP_S,		glu::getGLWrapMode(samplerParams.sampler.wrapS));
		gl.samplerParameteri(**sampler, GL_TEXTURE_WRAP_T,		glu::getGLWrapMode(samplerParams.sampler.wrapT));
		gl.samplerParameteri(**sampler, GL_TEXTURE_MIN_FILTER,	glu::getGLFilterMode(samplerParams.sampler.minFilter));
		gl.samplerParameteri(**sampler, GL_TEXTURE_MAG_FILTER,	glu::getGLFilterMode(samplerParams.sampler.magFilter));

		switch (m_channelClass)
		{
			case tcu::TEXTURECHANNELCLASS_SIGNED_FIXED_POINT:
			case tcu::TEXTURECHANNELCLASS_UNSIGNED_FIXED_POINT:
			case tcu::TEXTURECHANNELCLASS_FLOATING_POINT:
				gl.samplerParameterfv(**sampler, GL_TEXTURE_BORDER_COLOR, config.borderColor.getAccess<float>());
				break;

			case tcu::TEXTURECHANNELCLASS_SIGNED_INTEGER:
				gl.samplerParameterIiv(**sampler, GL_TEXTURE_BORDER_COLOR, config.borderColor.getAccess<deInt32>());
				break;

			case tcu::TEXTURECHANNELCLASS_UNSIGNED_INTEGER:
				gl.samplerParameterIuiv(**sampler, GL_TEXTURE_BORDER_COLOR, config.borderColor.getAccess<deUint32>());
				break;

			default:
				DE_ASSERT(false);
		}

		gl.bindSampler(0, **sampler);
	}

	GLU_EXPECT_NO_ERROR(gl.getError(), "Set texturing state");

	gl.viewport(viewport.x, viewport.y, viewport.width, viewport.height);
	renderQuad(&texCoord[0], samplerParams);
	glu::readPixels(m_context.getRenderContext(), viewport.x, viewport.y, surface.getAccess());
}

void TextureBorderClampTest::renderQuad (const float* texCoord, const glu::TextureTestUtil::ReferenceParams& samplerParams)
{
	// use TextureRenderer for basic rendering, use custom for gather
	if (m_samplingFunction == SAMPLE_FILTER)
		m_renderer->renderQuad(0, texCoord, samplerParams);
	else
	{
		static const float position[] =
		{
			-1.0f, -1.0f, 0.0f, 1.0f,
			-1.0f, +1.0f, 0.0f, 1.0f,
			+1.0f, -1.0f, 0.0f, 1.0f,
			+1.0f, +1.0f, 0.0f, 1.0f
		};
		static const deUint16 indices[] =
		{
			0, 1, 2, 2, 1, 3
		};
		const glu::VertexArrayBinding vertexArrays[] =
		{
			glu::va::Float("a_position",	4,	4, 0, &position[0]),
			glu::va::Float("a_texcoord",	2,	4, 0, texCoord)
		};

		const glw::Functions&	gl		= m_context.getRenderContext().getFunctions();
		const deUint32			progId	= m_gatherProgram->getProgram();

		gl.useProgram(progId);
		gl.uniform1i(gl.getUniformLocation(progId, "u_sampler"), 0);
		if (m_useShadowSampler)
			gl.uniform1f(gl.getUniformLocation(progId, "u_ref"), samplerParams.ref);
		gl.uniform4fv(gl.getUniformLocation(progId, "u_colorScale"), 1, samplerParams.colorScale.getPtr());
		gl.uniform4fv(gl.getUniformLocation(progId, "u_colorBias"), 1, samplerParams.colorBias.getPtr());

		glu::draw(m_context.getRenderContext(), progId, DE_LENGTH_OF_ARRAY(vertexArrays), &vertexArrays[0],
					glu::pr::Triangles(DE_LENGTH_OF_ARRAY(indices), &indices[0]));
	}
}

void TextureBorderClampTest::verifyImage (const tcu::Surface&							renderedFrame,
										  const IterationConfig&						config,
										  const glu::TextureTestUtil::ReferenceParams&	samplerParams)
{
	const tcu::PixelFormat	pixelFormat		= m_context.getRenderTarget().getPixelFormat();

	tcu::LodPrecision		lodPrecision;
	std::vector<float>		texCoord;
	bool					verificationOk;

	glu::TextureTestUtil::computeQuadTexCoord2D(texCoord, config.p0, config.p1);

	lodPrecision.derivateBits		= 18;
	lodPrecision.lodBits			= 5;

	if (samplerParams.sampler.compare == tcu::Sampler::COMPAREMODE_NONE)
	{
		const tcu::TextureFormat		texFormat			= tcu::getEffectiveDepthStencilTextureFormat(m_texture->getRefTexture().getFormat(), m_sampleMode);
		const bool						isNearestMinFilter	= samplerParams.sampler.minFilter == tcu::Sampler::NEAREST || samplerParams.sampler.minFilter == tcu::Sampler::NEAREST_MIPMAP_NEAREST;
		const bool						isNearestMagFilter	= samplerParams.sampler.magFilter == tcu::Sampler::NEAREST;
		const bool						isNearestOnly		= isNearestMinFilter && isNearestMagFilter;
		const bool						isSRGB				= texFormat.order == tcu::TextureFormat::sRGB || texFormat.order == tcu::TextureFormat::sRGBA;
		const int						colorErrorBits		= (isNearestOnly && !isSRGB) ? (1) : (2);
		const tcu::IVec4				colorBits			= tcu::max(glu::TextureTestUtil::getBitsVec(pixelFormat) - tcu::IVec4(colorErrorBits), tcu::IVec4(0));
		tcu::LookupPrecision			lookupPrecision;

		lookupPrecision.colorThreshold	= tcu::computeFixedPointThreshold(colorBits) / samplerParams.colorScale;
		lookupPrecision.coordBits		= tcu::IVec3(20,20,0);
		lookupPrecision.uvwBits			= tcu::IVec3(5,5,0);
		lookupPrecision.colorMask		= glu::TextureTestUtil::getCompareMask(pixelFormat);

		if (m_samplingFunction == SAMPLE_FILTER)
		{
			verificationOk = verifyTextureSampleResult(renderedFrame.getAccess(),
													   &texCoord[0],
													   samplerParams,
													   lodPrecision,
													   lookupPrecision);
		}
		else if (m_samplingFunction == SAMPLE_GATHER)
		{
			verificationOk = verifyTextureGatherResult(renderedFrame.getAccess(),
													   &texCoord[0],
													   samplerParams,
													   lookupPrecision);
		}
		else
		{
			DE_ASSERT(false);
			verificationOk = false;
		}
	}
	else
	{
		tcu::TexComparePrecision	texComparePrecision;
		tcu::TexComparePrecision	lowQualityTexComparePrecision;
		tcu::LodPrecision			lowQualityLodPrecision			= lodPrecision;

		texComparePrecision.coordBits					= tcu::IVec3(20,20,0);
		texComparePrecision.uvwBits						= tcu::IVec3(7,7,0);
		texComparePrecision.pcfBits						= 5;
		texComparePrecision.referenceBits				= 16;
		texComparePrecision.resultBits					= de::max(0, pixelFormat.redBits - 1);

		lowQualityTexComparePrecision.coordBits			= tcu::IVec3(20,20,0);
		lowQualityTexComparePrecision.uvwBits			= tcu::IVec3(4,4,0);
		lowQualityTexComparePrecision.pcfBits			= 0;
		lowQualityTexComparePrecision.referenceBits		= 16;
		lowQualityTexComparePrecision.resultBits		= de::max(0, pixelFormat.redBits - 1);

		lowQualityLodPrecision.lodBits					= 4;

		if (m_samplingFunction == SAMPLE_FILTER)
		{
			verificationOk = verifyTextureCompareResult(renderedFrame.getAccess(),
														&texCoord[0],
														samplerParams,
														texComparePrecision,
														lowQualityTexComparePrecision,
														lodPrecision,
														lowQualityLodPrecision);
		}
		else if (m_samplingFunction == SAMPLE_GATHER)
		{
			verificationOk = verifyTextureGatherCmpResult(renderedFrame.getAccess(),
														  &texCoord[0],
														  samplerParams,
														  texComparePrecision,
														  lowQualityTexComparePrecision);
		}
		else
		{
			DE_ASSERT(false);
			verificationOk = false;
		}
	}

	if (!verificationOk)
		m_result.fail("Image verification failed");
}

bool TextureBorderClampTest::verifyTextureSampleResult (const tcu::ConstPixelBufferAccess&				renderedFrame,
														const float*									texCoord,
														const glu::TextureTestUtil::ReferenceParams&	samplerParams,
														const tcu::LodPrecision&						lodPrecision,
													    const tcu::LookupPrecision&						lookupPrecision)
{
	const tcu::PixelFormat			pixelFormat			= m_context.getRenderTarget().getPixelFormat();
	tcu::Surface					reference			(renderedFrame.getWidth(), renderedFrame.getHeight());
	tcu::Surface					errorMask			(renderedFrame.getWidth(), renderedFrame.getHeight());
	int								numFailedPixels;

	glu::TextureTestUtil::sampleTexture(tcu::SurfaceAccess(reference, pixelFormat), m_texture->getRefTexture(), texCoord, samplerParams);

	numFailedPixels = glu::TextureTestUtil::computeTextureLookupDiff(renderedFrame, reference.getAccess(), errorMask.getAccess(), m_texture->getRefTexture(),
																	 texCoord, samplerParams, lookupPrecision, lodPrecision, m_testCtx.getWatchDog());

	if (numFailedPixels > 0)
		m_testCtx.getLog() << tcu::TestLog::Message << "ERROR: Result verification failed, got " << numFailedPixels << " invalid pixels!" << tcu::TestLog::EndMessage;
	m_testCtx.getLog()	<< tcu::TestLog::ImageSet("VerifyResult", "Verification result")
						<< tcu::TestLog::Image("Rendered", "Rendered image", renderedFrame);
	if (numFailedPixels > 0)
	{
		m_testCtx.getLog()	<< tcu::TestLog::Image("Reference", "Ideal reference image", reference)
							<< tcu::TestLog::Image("ErrorMask", "Error mask", errorMask);
	}
	m_testCtx.getLog() << tcu::TestLog::EndImageSet;

	return (numFailedPixels == 0);
}

bool TextureBorderClampTest::verifyTextureCompareResult (const tcu::ConstPixelBufferAccess&				renderedFrame,
														 const float*									texCoord,
														 const glu::TextureTestUtil::ReferenceParams&	samplerParams,
													     const tcu::TexComparePrecision&				texComparePrecision,
													     const tcu::TexComparePrecision&				lowQualityTexComparePrecision,
														 const tcu::LodPrecision&						lodPrecision,
														 const tcu::LodPrecision&						lowQualityLodPrecision)
{
	const tcu::PixelFormat						pixelFormat				= m_context.getRenderTarget().getPixelFormat();
	const int									colorErrorBits			= 1;
	const tcu::IVec4							nonShadowBits			= tcu::max(glu::TextureTestUtil::getBitsVec(pixelFormat) - tcu::IVec4(colorErrorBits), tcu::IVec4(0));
	const tcu::Vec3								nonShadowThreshold		= tcu::computeFixedPointThreshold(nonShadowBits).swizzle(1,2,3);
	std::vector<tcu::ConstPixelBufferAccess>	srcLevelStorage;
	const tcu::Texture2DView					effectiveView			= tcu::getEffectiveTextureView(m_texture->getRefTexture(), srcLevelStorage, samplerParams.sampler);
	tcu::Surface								reference				(renderedFrame.getWidth(), renderedFrame.getHeight());
	tcu::Surface								errorMask				(renderedFrame.getWidth(), renderedFrame.getHeight());
	int											numFailedPixels;

	glu::TextureTestUtil::sampleTexture(tcu::SurfaceAccess(reference, pixelFormat), effectiveView, texCoord, samplerParams);

	numFailedPixels = glu::TextureTestUtil::computeTextureCompareDiff(renderedFrame, reference.getAccess(), errorMask.getAccess(), effectiveView,
																	  texCoord, samplerParams, texComparePrecision, lodPrecision, nonShadowThreshold);

	if (numFailedPixels > 0)
	{
		m_testCtx.getLog()	<< tcu::TestLog::Message
							<< "Warning: Verification assuming high-quality PCF filtering failed."
							<< tcu::TestLog::EndMessage;

		numFailedPixels = glu::TextureTestUtil::computeTextureCompareDiff(renderedFrame, reference.getAccess(), errorMask.getAccess(), effectiveView,
																		  texCoord, samplerParams, lowQualityTexComparePrecision, lowQualityLodPrecision, nonShadowThreshold);

		if (numFailedPixels > 0)
			m_testCtx.getLog() << tcu::TestLog::Message << "ERROR: Verification against low precision requirements failed, failing test case." << tcu::TestLog::EndMessage;
		else if (m_testCtx.getTestResult() == QP_TEST_RESULT_PASS)
			m_result.addResult(QP_TEST_RESULT_QUALITY_WARNING, "Low-quality result");
	}

	if (numFailedPixels > 0)
		m_testCtx.getLog() << tcu::TestLog::Message << "ERROR: Result verification failed, got " << numFailedPixels << " invalid pixels!" << tcu::TestLog::EndMessage;
	m_testCtx.getLog()	<< tcu::TestLog::ImageSet("VerifyResult", "Verification result")
						<< tcu::TestLog::Image("Rendered", "Rendered image", renderedFrame);
	if (numFailedPixels > 0)
	{
		m_testCtx.getLog()	<< tcu::TestLog::Image("Reference", "Ideal reference image", reference)
							<< tcu::TestLog::Image("ErrorMask", "Error mask", errorMask);
	}
	m_testCtx.getLog() << tcu::TestLog::EndImageSet;

	return (numFailedPixels == 0);
}

template <typename T>
static inline T triQuadInterpolate (const T (&values)[4], float xFactor, float yFactor)
{
	if (xFactor + yFactor < 1.0f)
		return values[0] + (values[2]-values[0])*xFactor		+ (values[1]-values[0])*yFactor;
	else
		return values[3] + (values[1]-values[3])*(1.0f-xFactor)	+ (values[2]-values[3])*(1.0f-yFactor);
}

bool TextureBorderClampTest::verifyTextureGatherResult (const tcu::ConstPixelBufferAccess&				renderedFrame,
														const float*									texCoordArray,
														const glu::TextureTestUtil::ReferenceParams&	samplerParams,
														const tcu::LookupPrecision&						lookupPrecision)
{
	const tcu::Vec2 texCoords[4] =
	{
		tcu::Vec2(texCoordArray[0], texCoordArray[1]),
		tcu::Vec2(texCoordArray[2], texCoordArray[3]),
		tcu::Vec2(texCoordArray[4], texCoordArray[5]),
		tcu::Vec2(texCoordArray[6], texCoordArray[7]),
	};

	const tcu::PixelFormat						pixelFormat			= m_context.getRenderTarget().getPixelFormat();
	const deUint8								fbColormask			= tcu::getColorMask(pixelFormat);

	std::vector<tcu::ConstPixelBufferAccess>	srcLevelStorage;
	const tcu::Texture2DView					effectiveView		= tcu::getEffectiveTextureView(m_texture->getRefTexture(), srcLevelStorage, samplerParams.sampler);

	tcu::Surface								reference			(renderedFrame.getWidth(), renderedFrame.getHeight());
	tcu::Surface								errorMask			(renderedFrame.getWidth(), renderedFrame.getHeight());
	int											numFailedPixels		= 0;

	tcu::clear(errorMask.getAccess(), tcu::RGBA::green().toVec());

	for (int py = 0; py < reference.getHeight(); ++py)
	for (int px = 0; px < reference.getWidth(); ++px)
	{
		const tcu::Vec2			viewportCoord	= (tcu::Vec2((float)px, (float)py) + tcu::Vec2(0.5f)) / tcu::Vec2((float)reference.getWidth(), (float)reference.getHeight());
		const tcu::Vec2			texCoord		= triQuadInterpolate(texCoords, viewportCoord.x(), viewportCoord.y());
		const tcu::Vec4			referenceValue	= effectiveView.gatherOffsets(samplerParams.sampler, texCoord.x(), texCoord.y(), 0, glu::getDefaultGatherOffsets());
		const tcu::Vec4			referencePixel	= referenceValue * samplerParams.colorScale + samplerParams.colorBias;
		const tcu::Vec4			resultPixel		= renderedFrame.getPixel(px, py);
		const tcu::Vec4			resultValue		= (resultPixel - samplerParams.colorBias) / samplerParams.colorScale;

		reference.setPixel(px, py, tcu::toRGBAMasked(referenceValue, fbColormask));

		if (tcu::boolAny(tcu::logicalAnd(lookupPrecision.colorMask,
										 tcu::greaterThan(tcu::absDiff(resultPixel, referencePixel),
														  lookupPrecision.colorThreshold))))
		{
			if (!tcu::isGatherOffsetsResultValid(effectiveView, samplerParams.sampler, lookupPrecision, texCoord, 0, glu::getDefaultGatherOffsets(), resultValue))
			{
				errorMask.setPixel(px, py, tcu::RGBA::red());
				++numFailedPixels;
			}
		}
	}

	if (numFailedPixels > 0)
		m_testCtx.getLog() << tcu::TestLog::Message << "ERROR: Result verification failed, got " << numFailedPixels << " invalid pixels!" << tcu::TestLog::EndMessage;
	m_testCtx.getLog()	<< tcu::TestLog::ImageSet("VerifyResult", "Verification result")
						<< tcu::TestLog::Image("Rendered", "Rendered image", renderedFrame);
	if (numFailedPixels > 0)
	{
		m_testCtx.getLog()	<< tcu::TestLog::Image("Reference", "Ideal reference image", reference)
							<< tcu::TestLog::Image("ErrorMask", "Error mask", errorMask);
	}
	m_testCtx.getLog() << tcu::TestLog::EndImageSet;

	return (numFailedPixels == 0);
}

bool TextureBorderClampTest::verifyTextureGatherCmpResult (const tcu::ConstPixelBufferAccess&			renderedFrame,
														   const float*									texCoordArray,
														   const glu::TextureTestUtil::ReferenceParams&	samplerParams,
														   const tcu::TexComparePrecision&				texComparePrecision,
														   const tcu::TexComparePrecision&				lowQualityTexComparePrecision)
{
	const tcu::Vec2 texCoords[4] =
	{
		tcu::Vec2(texCoordArray[0], texCoordArray[1]),
		tcu::Vec2(texCoordArray[2], texCoordArray[3]),
		tcu::Vec2(texCoordArray[4], texCoordArray[5]),
		tcu::Vec2(texCoordArray[6], texCoordArray[7]),
	};

	std::vector<tcu::ConstPixelBufferAccess>	srcLevelStorage;
	const tcu::Texture2DView					effectiveView		= tcu::getEffectiveTextureView(m_texture->getRefTexture(), srcLevelStorage, samplerParams.sampler);

	const tcu::PixelFormat						pixelFormat			= m_context.getRenderTarget().getPixelFormat();
	const tcu::BVec4							colorMask			= glu::TextureTestUtil::getCompareMask(pixelFormat);
	const deUint8								fbColormask			= tcu::getColorMask(pixelFormat);
	tcu::Surface								reference			(renderedFrame.getWidth(), renderedFrame.getHeight());
	tcu::Surface								errorMask			(renderedFrame.getWidth(), renderedFrame.getHeight());
	int											numFailedPixels		= 0;
	bool										lowQuality			= false;

	tcu::clear(errorMask.getAccess(), tcu::RGBA::green().toVec());

	for (int py = 0; py < reference.getHeight(); ++py)
	for (int px = 0; px < reference.getWidth(); ++px)
	{
		const tcu::Vec2			viewportCoord	= (tcu::Vec2((float)px, (float)py) + tcu::Vec2(0.5f)) / tcu::Vec2((float)reference.getWidth(), (float)reference.getHeight());
		const tcu::Vec2			texCoord		= triQuadInterpolate(texCoords, viewportCoord.x(), viewportCoord.y());
		const float				refZ			= samplerParams.ref;
		const tcu::Vec4			referenceValue	= effectiveView.gatherOffsetsCompare(samplerParams.sampler, refZ, texCoord.x(), texCoord.y(), glu::getDefaultGatherOffsets());
		const tcu::Vec4			resultValue		= renderedFrame.getPixel(px, py);

		reference.setPixel(px, py, tcu::toRGBAMasked(referenceValue, fbColormask));

		if (tcu::boolAny(tcu::logicalAnd(colorMask, tcu::notEqual(referenceValue, resultValue))))
		{
			if (!tcu::isGatherOffsetsCompareResultValid(effectiveView, samplerParams.sampler, texComparePrecision, texCoord, glu::getDefaultGatherOffsets(), refZ, resultValue))
			{
				lowQuality = true;

				// fall back to low quality verification
				if (!tcu::isGatherOffsetsCompareResultValid(effectiveView, samplerParams.sampler, lowQualityTexComparePrecision, texCoord, glu::getDefaultGatherOffsets(), refZ, resultValue))
				{
					errorMask.setPixel(px, py, tcu::RGBA::red());
					++numFailedPixels;
				}
			}
		}
	}

	if (numFailedPixels > 0)
		m_testCtx.getLog() << tcu::TestLog::Message << "ERROR: Result verification failed, got " << numFailedPixels << " invalid pixels!" << tcu::TestLog::EndMessage;
	else if (lowQuality)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Warning: Verification assuming high-quality PCF filtering failed." << tcu::TestLog::EndMessage;
		m_result.addResult(QP_TEST_RESULT_QUALITY_WARNING, "Low-quality result");
	}

	m_testCtx.getLog()	<< tcu::TestLog::ImageSet("VerifyResult", "Verification result")
						<< tcu::TestLog::Image("Rendered", "Rendered image", renderedFrame);
	if (numFailedPixels > 0)
	{
		m_testCtx.getLog()	<< tcu::TestLog::Image("Reference", "Ideal reference image", reference)
							<< tcu::TestLog::Image("ErrorMask", "Error mask", errorMask);
	}
	m_testCtx.getLog() << tcu::TestLog::EndImageSet;

	return (numFailedPixels == 0);
}

const glu::Texture2D* TextureBorderClampTest::getTexture (void) const
{
	return m_texture.get();
}

deUint32 TextureBorderClampTest::getIterationSeed (const IterationConfig& config) const
{
	tcu::SeedBuilder builder;
	builder	<< std::string(getName())
			<< m_iterationNdx
			<< m_texFormat
			<< config.minFilter << config.magFilter
			<< m_texture->getRefTexture().getWidth() << m_texture->getRefTexture().getHeight();
	return builder.get();
}

glu::TextureTestUtil::ReferenceParams TextureBorderClampTest::genSamplerParams (const IterationConfig& config) const
{
	const tcu::TextureFormat				texFormat		= tcu::getEffectiveDepthStencilTextureFormat(m_texture->getRefTexture().getFormat(), m_sampleMode);
	glu::TextureTestUtil::ReferenceParams	refParams		(glu::TextureTestUtil::TEXTURETYPE_2D);

	refParams.sampler					= glu::mapGLSampler(config.sWrapMode, config.tWrapMode, config.minFilter, config.magFilter);
	refParams.sampler.borderColor		= config.borderColor;
	refParams.sampler.compare			= (!m_useShadowSampler) ? (tcu::Sampler::COMPAREMODE_NONE) : (glu::mapGLCompareFunc(config.compareMode));
	refParams.sampler.depthStencilMode	= m_sampleMode;
	refParams.lodMode					= glu::TextureTestUtil::LODMODE_EXACT;
	refParams.samplerType				= (!m_useShadowSampler) ? (glu::TextureTestUtil::getSamplerType(texFormat)) : (glu::TextureTestUtil::SAMPLERTYPE_SHADOW);
	refParams.colorScale				= config.lookupScale;
	refParams.colorBias					= config.lookupBias;
	refParams.ref						= config.compareRef;

	// compare can only be used with depth textures
	if (!isDepthFormat(m_texFormat, m_sampleMode))
		DE_ASSERT(refParams.sampler.compare == tcu::Sampler::COMPAREMODE_NONE);

	// sampler type must match compare mode
	DE_ASSERT(m_useShadowSampler == (config.compareMode != GL_NONE));

	// in gather, weird mapping is most likely an error
	if (m_samplingFunction == SAMPLE_GATHER)
	{
		DE_ASSERT(refParams.colorScale == tcu::Vec4(refParams.colorScale.x()));
		DE_ASSERT(refParams.colorBias == tcu::Vec4(refParams.colorBias.x()));
	}

	return refParams;
}

glu::ShaderProgram* TextureBorderClampTest::genGatherProgram (void) const
{
	const std::string	glslVersionDecl	= glu::getGLSLVersionDeclaration(glu::getContextTypeGLSLVersion(m_context.getRenderContext().getType()));
	const std::string	vtxSource		= glslVersionDecl + "\n"
										"in highp vec4 a_position;\n"
										"in highp vec2 a_texcoord;\n"
										"out highp vec2 v_texcoord;\n"
										"void main()\n"
										"{\n"
										"	gl_Position = a_position;\n"
										"	v_texcoord = a_texcoord;\n"
										"}\n";
	const char*			samplerType;
	const char*			lookup;
	std::ostringstream	fragSource;

	if (m_useShadowSampler)
	{
		samplerType	= "sampler2DShadow";
		lookup		= "textureGather(u_sampler, v_texcoord, u_ref)";
	}
	else
	{
		switch (m_channelClass)
		{
			case tcu::TEXTURECHANNELCLASS_UNSIGNED_FIXED_POINT:
			case tcu::TEXTURECHANNELCLASS_SIGNED_FIXED_POINT:
			case tcu::TEXTURECHANNELCLASS_FLOATING_POINT:
				samplerType	= "sampler2D";
				lookup		= "textureGather(u_sampler, v_texcoord)";
				break;

			case tcu::TEXTURECHANNELCLASS_SIGNED_INTEGER:
				samplerType	= "isampler2D";
				lookup		= "vec4(textureGather(u_sampler, v_texcoord))";
				break;

			case tcu::TEXTURECHANNELCLASS_UNSIGNED_INTEGER:
				samplerType	= "usampler2D";
				lookup		= "vec4(textureGather(u_sampler, v_texcoord))";
				break;

			default:
				samplerType	= "";
				lookup		= "";
				DE_ASSERT(false);
		}
	}

	fragSource	<<	glslVersionDecl + "\n"
					"uniform highp " << samplerType << " u_sampler;\n"
					"uniform highp vec4 u_colorScale;\n"
					"uniform highp vec4 u_colorBias;\n"
				<<	((m_useShadowSampler) ? ("uniform highp float u_ref;\n") : (""))
				<<	"in highp vec2 v_texcoord;\n"
					"layout(location=0) out highp vec4 o_color;\n"
					"void main()\n"
					"{\n"
					"	o_color = " << lookup << " * u_colorScale + u_colorBias;\n"
					"}\n";

	return new glu::ShaderProgram(m_context.getRenderContext(), glu::ProgramSources() << glu::VertexSource(vtxSource) << glu::FragmentSource(fragSource.str()));
}

class TextureBorderClampFormatCase : public TextureBorderClampTest
{
public:
									TextureBorderClampFormatCase	(Context&						context,
																	 const char*					name,
																	 const char*					description,
																	 deUint32						texFormat,
																	 tcu::Sampler::DepthStencilMode	mode,
																	 StateType						stateType,
																	 SizeType						sizeType,
																	 deUint32						filter,
																	 SamplingFunction				samplingFunction);

private:
	void							init							(void);

	int								getNumIterations				(void) const;
	IterationConfig					getIteration					(int ndx) const;

	const SizeType					m_sizeType;
	const deUint32					m_filter;

	std::vector<IterationConfig>	m_iterations;
};


TextureBorderClampFormatCase::TextureBorderClampFormatCase	(Context&						context,
															 const char*					name,
															 const char*					description,
															 deUint32						texFormat,
															 tcu::Sampler::DepthStencilMode	mode,
															 StateType						stateType,
															 SizeType						sizeType,
															 deUint32						filter,
															 SamplingFunction				samplingFunction)
	: TextureBorderClampTest(context,
							 name,
							 description,
							 texFormat,
							 mode,
							 stateType,
							 (sizeType == SIZE_POT) ? (32) : (17),
							 (sizeType == SIZE_POT) ? (16) : (31),
							 samplingFunction)
	, m_sizeType			(sizeType)
	, m_filter				(filter)
{
	if (m_sizeType == SIZE_POT)
		DE_ASSERT(deIsPowerOfTwo32(m_texWidth) && deIsPowerOfTwo32(m_texHeight));
	else
		DE_ASSERT(!deIsPowerOfTwo32(m_texWidth) && !deIsPowerOfTwo32(m_texHeight));

	if (glu::isCompressedFormat(texFormat))
	{
		const tcu::CompressedTexFormat	compressedFormat	= glu::mapGLCompressedTexFormat(texFormat);
		const tcu::IVec3				blockPixelSize		= tcu::getBlockPixelSize(compressedFormat);

		// is (not) multiple of a block size
		if (m_sizeType == SIZE_POT)
			DE_ASSERT((m_texWidth % blockPixelSize.x()) == 0 && (m_texHeight % blockPixelSize.y()) == 0);
		else
			DE_ASSERT((m_texWidth % blockPixelSize.x()) != 0 && (m_texHeight % blockPixelSize.y()) != 0);

		DE_UNREF(blockPixelSize);
	}
}

void TextureBorderClampFormatCase::init (void)
{
	TextureBorderClampTest::init();

	// \note TextureBorderClampTest::init() creates texture
	const tcu::TextureFormat		texFormat		= tcu::getEffectiveDepthStencilTextureFormat(getTexture()->getRefTexture().getFormat(), m_sampleMode);
	const tcu::TextureFormatInfo	texFormatInfo	= tcu::getTextureFormatInfo(texFormat);

	// iterations

	{
		IterationConfig iteration;
		iteration.p0			= tcu::Vec2(-1.5f, -3.0f);
		iteration.p1			= tcu::Vec2( 1.5f,  2.5f);
		iteration.borderColor	= mapToFormatColorRepresentable(texFormat, tcu::Vec4(0.3f, 0.7f, 0.2f, 0.5f));
		m_iterations.push_back(iteration);
	}
	{
		IterationConfig iteration;
		iteration.p0			= tcu::Vec2(-0.5f, 0.75f);
		iteration.p1			= tcu::Vec2(0.25f, 1.25f);
		iteration.borderColor	= mapToFormatColorRepresentable(texFormat, tcu::Vec4(0.9f, 0.2f, 0.4f, 0.6f));
		m_iterations.push_back(iteration);
	}

	// common parameters
	for (int ndx = 0; ndx < (int)m_iterations.size(); ++ndx)
	{
		IterationConfig& iteration = m_iterations[ndx];

		if (m_samplingFunction == SAMPLE_GATHER)
		{
			iteration.lookupScale	= tcu::Vec4(texFormatInfo.lookupScale.x());
			iteration.lookupBias	= tcu::Vec4(texFormatInfo.lookupBias.x());
		}
		else
		{
			iteration.lookupScale	= texFormatInfo.lookupScale;
			iteration.lookupBias	= texFormatInfo.lookupBias;
		}

		iteration.minFilter		= m_filter;
		iteration.magFilter		= m_filter;
		iteration.sWrapMode		= GL_CLAMP_TO_BORDER;
		iteration.tWrapMode		= GL_CLAMP_TO_BORDER;
		iteration.compareMode	= GL_NONE;
		iteration.compareRef	= 0.0f;
	}
}

int TextureBorderClampFormatCase::getNumIterations	(void) const
{
	return (int)m_iterations.size();
}

TextureBorderClampTest::IterationConfig TextureBorderClampFormatCase::getIteration (int ndx) const
{
	return m_iterations[ndx];
}

class TextureBorderClampRangeClampCase : public TextureBorderClampTest
{
public:
									TextureBorderClampRangeClampCase	(Context&						context,
																		 const char*					name,
																		 const char*					description,
																		 deUint32						texFormat,
																		 tcu::Sampler::DepthStencilMode	mode,
																		 deUint32						filter);

private:
	void							init								(void);

	int								getNumIterations					(void) const;
	IterationConfig					getIteration						(int ndx) const;

	const deUint32					m_filter;
	std::vector<IterationConfig>	m_iterations;
};

TextureBorderClampRangeClampCase::TextureBorderClampRangeClampCase	(Context&						context,
																	 const char*					name,
																	 const char*					description,
																	 deUint32						texFormat,
																	 tcu::Sampler::DepthStencilMode	mode,
																	 deUint32						filter)
	: TextureBorderClampTest(context, name, description, texFormat, mode, TextureBorderClampTest::STATE_TEXTURE_PARAM, 8, 32, SAMPLE_FILTER)
	, m_filter				(filter)
{
}

void TextureBorderClampRangeClampCase::init (void)
{
	TextureBorderClampTest::init();

	const tcu::TextureFormat	texFormat		= tcu::getEffectiveDepthStencilTextureFormat(getTexture()->getRefTexture().getFormat(), m_sampleMode);
	const bool					isDepth			= isDepthFormat(m_texFormat, m_sampleMode);
	const bool					isFloat			= m_channelClass == tcu::TEXTURECHANNELCLASS_FLOATING_POINT;
	const bool					isFixed			= m_channelClass == tcu::TEXTURECHANNELCLASS_SIGNED_FIXED_POINT || m_channelClass == tcu::TEXTURECHANNELCLASS_UNSIGNED_FIXED_POINT;
	const bool					isPureInteger	= m_channelClass == tcu::TEXTURECHANNELCLASS_SIGNED_INTEGER || m_channelClass == tcu::TEXTURECHANNELCLASS_UNSIGNED_INTEGER;

	if (isDepth || isFloat)
	{
		// infinities are commonly used values on depth/float borders
		{
			IterationConfig iteration;
			iteration.p0			= tcu::Vec2(-1.2f, -3.0f);
			iteration.p1			= tcu::Vec2( 1.2f,  2.5f);
			iteration.borderColor	= rr::GenericVec4(tcu::Vec4(std::numeric_limits<float>::infinity()));
			iteration.lookupScale	= tcu::Vec4(0.5f); // scale & bias to [0.25, 0.5] range to make out-of-range values visible
			iteration.lookupBias	= tcu::Vec4(0.25f);
			iteration.description	= "border value infinity";
			m_iterations.push_back(iteration);
		}
		{
			IterationConfig iteration;
			iteration.p0			= tcu::Vec2(-0.25f, -0.75f);
			iteration.p1			= tcu::Vec2( 2.25f,  1.25f);
			iteration.borderColor	= rr::GenericVec4(tcu::Vec4(-std::numeric_limits<float>::infinity()));
			iteration.lookupScale	= tcu::Vec4(0.5f);
			iteration.lookupBias	= tcu::Vec4(0.25f);
			iteration.description	= "border value negative infinity";
			m_iterations.push_back(iteration);
		}
	}
	else if (isPureInteger)
	{
		const tcu::IVec4			numBits		= tcu::getTextureFormatBitDepth(texFormat);
		const bool					isSigned	= m_channelClass == tcu::TEXTURECHANNELCLASS_SIGNED_INTEGER;

		// can't overflow 32bit integers with 32bit integers
		for (int ndx = 0; ndx < 4; ++ndx)
			DE_ASSERT(numBits[ndx] == 0 || numBits[ndx] == 8 || numBits[ndx] == 16);

		const tcu::IVec4	minValue		= getNBitIntegerVec4MinValue(isSigned, numBits);
		const tcu::IVec4	maxValue		= getNBitIntegerVec4MaxValue(isSigned, numBits);
		const tcu::IVec4	valueRange		= maxValue - minValue;
		const tcu::IVec4	divSafeRange	((valueRange[0]==0) ? (1) : (valueRange[0]),
											 (valueRange[1]==0) ? (1) : (valueRange[1]),
											 (valueRange[2]==0) ? (1) : (valueRange[2]),
											 (valueRange[3]==0) ? (1) : (valueRange[3]));

		// format max
		{
			const tcu::IVec4 value = maxValue + tcu::IVec4(1);

			IterationConfig iteration;
			iteration.p0			= tcu::Vec2(-1.2f, -3.0f);
			iteration.p1			= tcu::Vec2( 1.2f,  2.5f);
			iteration.borderColor	= (isSigned) ? (rr::GenericVec4(value)) : (rr::GenericVec4(value.cast<deUint32>()));
			iteration.lookupScale	= tcu::Vec4(0.5f) / divSafeRange.cast<float>();
			iteration.lookupBias	= (isSigned) ? (tcu::Vec4(0.5f)) : (tcu::Vec4(0.25f));
			iteration.description	= "border values one larger than maximum";
			m_iterations.push_back(iteration);
		}
		// format min
		if (isSigned)
		{
			const tcu::IVec4 value = minValue - tcu::IVec4(1);

			IterationConfig iteration;
			iteration.p0			= tcu::Vec2(-0.25f, -0.75f);
			iteration.p1			= tcu::Vec2( 2.25f,  1.25f);
			iteration.borderColor	= rr::GenericVec4(value);
			iteration.lookupScale	= tcu::Vec4(0.5f) / divSafeRange.cast<float>();
			iteration.lookupBias	= tcu::Vec4(0.5f);
			iteration.description	= "border values one less than minimum";
			m_iterations.push_back(iteration);
		}
		// (u)int32 max
		{
			const tcu::IVec4 value = (isSigned) ? (tcu::IVec4(std::numeric_limits<deInt32>::max())) : (tcu::IVec4(std::numeric_limits<deUint32>::max()));

			IterationConfig iteration;
			iteration.p0			= tcu::Vec2(-1.6f, -2.1f);
			iteration.p1			= tcu::Vec2( 1.2f,  3.5f);
			iteration.borderColor	= (isSigned) ? (rr::GenericVec4(value)) : (rr::GenericVec4(value.cast<deUint32>()));
			iteration.lookupScale	= tcu::Vec4(0.5f) / divSafeRange.cast<float>();
			iteration.lookupBias	= tcu::Vec4(0.25f);
			iteration.description	= "border values 32-bit maximum";
			m_iterations.push_back(iteration);
		}
		// int32 min
		if (isSigned)
		{
			const tcu::IVec4 value = tcu::IVec4(std::numeric_limits<deInt32>::min());

			IterationConfig iteration;
			iteration.p0			= tcu::Vec2(-2.6f, -4.0f);
			iteration.p1			= tcu::Vec2( 1.1f,  1.5f);
			iteration.borderColor	= rr::GenericVec4(value);
			iteration.lookupScale	= tcu::Vec4(0.5f) / divSafeRange.cast<float>();
			iteration.lookupBias	= tcu::Vec4(0.25f);
			iteration.description	= "border values 0";
			m_iterations.push_back(iteration);
		}
	}
	else if (isFixed)
	{
		const bool		isSigned	= m_channelClass == tcu::TEXTURECHANNELCLASS_SIGNED_FIXED_POINT;;
		const tcu::Vec4	lookupBias	= (isSigned) ? (tcu::Vec4(0.5f))    : (tcu::Vec4(0.25f)); // scale & bias to [0.25, 0.5] range to make out-of-range values visible
		const tcu::Vec4	lookupScale	= (isSigned) ? (tcu::Vec4(0.25f))   : (tcu::Vec4(0.5f));

		{
			IterationConfig iteration;
			iteration.p0			= tcu::Vec2(-1.2f, -3.0f);
			iteration.p1			= tcu::Vec2( 1.2f,  2.5f);
			iteration.borderColor	= mapToFormatColorUnits(texFormat, tcu::Vec4(1.1f, 1.3f, 2.2f, 1.3f));
			iteration.lookupScale	= lookupScale;
			iteration.lookupBias	= lookupBias;
			iteration.description	= "border values larger than maximum";
			m_iterations.push_back(iteration);
		}
		{
			IterationConfig iteration;
			iteration.p0			= tcu::Vec2(-0.25f, -0.75f);
			iteration.p1			= tcu::Vec2( 2.25f,  1.25f);
			iteration.borderColor	= mapToFormatColorUnits(texFormat, tcu::Vec4(-0.2f, -0.9f, -2.4f, -0.6f));
			iteration.lookupScale	= lookupScale;
			iteration.lookupBias	= lookupBias;
			iteration.description	= "border values less than minimum";
			m_iterations.push_back(iteration);
		}
	}
	else
		DE_ASSERT(false);

	// common parameters
	for (int ndx = 0; ndx < (int)m_iterations.size(); ++ndx)
	{
		IterationConfig& iteration = m_iterations[ndx];

		iteration.minFilter		= m_filter;
		iteration.magFilter		= m_filter;
		iteration.sWrapMode		= GL_CLAMP_TO_BORDER;
		iteration.tWrapMode		= GL_CLAMP_TO_BORDER;
		iteration.compareMode	= GL_NONE;
		iteration.compareRef	= 0.0f;
	}
}

int TextureBorderClampRangeClampCase::getNumIterations	(void) const
{
	return (int)m_iterations.size();
}

TextureBorderClampTest::IterationConfig TextureBorderClampRangeClampCase::getIteration (int ndx) const
{
	return m_iterations[ndx];
}

class TextureBorderClampPerAxisCase2D : public TextureBorderClampTest
{
public:
									TextureBorderClampPerAxisCase2D	(Context&						context,
																	 const char*					name,
																	 const char*					description,
																	 deUint32						texFormat,
																	 tcu::Sampler::DepthStencilMode	mode,
																	 SizeType						sizeType,
																	 deUint32						filter,
																	 deUint32						texSWrap,
																	 deUint32						texTWrap,
																	 SamplingFunction				samplingFunction);

private:
	void							init							(void);

	int								getNumIterations				(void) const;
	IterationConfig					getIteration					(int ndx) const;

	const deUint32					m_texSWrap;
	const deUint32					m_texTWrap;
	const deUint32					m_filter;

	std::vector<IterationConfig>	m_iterations;
};

TextureBorderClampPerAxisCase2D::TextureBorderClampPerAxisCase2D (Context&							context,
																  const char*						name,
																  const char*						description,
																  deUint32							texFormat,
																  tcu::Sampler::DepthStencilMode	mode,
																  SizeType							sizeType,
																  deUint32							filter,
																  deUint32							texSWrap,
																  deUint32							texTWrap,
																  SamplingFunction					samplingFunction)
	: TextureBorderClampTest(context,
							 name,
							 description,
							 texFormat,
							 mode,
							 TextureBorderClampTest::STATE_TEXTURE_PARAM,
							 (sizeType == SIZE_POT) ? (16) : (7),
							 (sizeType == SIZE_POT) ? (8) : (9),
							 samplingFunction)
	, m_texSWrap			(texSWrap)
	, m_texTWrap			(texTWrap)
	, m_filter				(filter)
{
}

void TextureBorderClampPerAxisCase2D::init (void)
{
	TextureBorderClampTest::init();

	// \note TextureBorderClampTest::init() creates texture
	const tcu::TextureFormat		texFormat		= tcu::getEffectiveDepthStencilTextureFormat(getTexture()->getRefTexture().getFormat(), m_sampleMode);
	const tcu::TextureFormatInfo	texFormatInfo	= tcu::getTextureFormatInfo(texFormat);

	IterationConfig iteration;
	iteration.p0			= tcu::Vec2(-0.25f, -0.75f);
	iteration.p1			= tcu::Vec2( 2.25f,  1.25f);
	iteration.borderColor	= mapToFormatColorRepresentable(texFormat, tcu::Vec4(0.4f, 0.9f, 0.1f, 0.2f));

	if (m_samplingFunction == SAMPLE_GATHER)
	{
		iteration.lookupScale	= tcu::Vec4(texFormatInfo.lookupScale.x());
		iteration.lookupBias	= tcu::Vec4(texFormatInfo.lookupBias.x());
	}
	else
	{
		iteration.lookupScale	= texFormatInfo.lookupScale;
		iteration.lookupBias	= texFormatInfo.lookupBias;
	}

	iteration.minFilter		= m_filter;
	iteration.magFilter		= m_filter;
	iteration.sWrapMode		= m_texSWrap;
	iteration.tWrapMode		= m_texTWrap;
	iteration.compareMode	= GL_NONE;
	iteration.compareRef	= 0.0f;

	m_iterations.push_back(iteration);
}

int TextureBorderClampPerAxisCase2D::getNumIterations	(void) const
{
	return (int)m_iterations.size();
}

TextureBorderClampTest::IterationConfig TextureBorderClampPerAxisCase2D::getIteration (int ndx) const
{
	return m_iterations[ndx];
}

class TextureBorderClampDepthCompareCase : public TextureBorderClampTest
{
public:
									TextureBorderClampDepthCompareCase	(Context&			context,
																		 const char*		name,
																		 const char*		description,
																		 deUint32			texFormat,
																		 SizeType			sizeType,
																		 deUint32			filter,
																		 SamplingFunction	samplingFunction);

private:
	void							init								(void);

	int								getNumIterations					(void) const;
	IterationConfig					getIteration						(int ndx) const;

	const deUint32					m_filter;
	std::vector<IterationConfig>	m_iterations;
};

TextureBorderClampDepthCompareCase::TextureBorderClampDepthCompareCase (Context&			context,
																		const char*			name,
																		const char*			description,
																		deUint32			texFormat,
																		SizeType			sizeType,
																		deUint32			filter,
																		SamplingFunction	samplingFunction)
	: TextureBorderClampTest(context,
							 name,
							 description,
							 texFormat,
							 tcu::Sampler::MODE_DEPTH,
							 TextureBorderClampTest::STATE_TEXTURE_PARAM,
							 (sizeType == SIZE_POT) ? (32) : (13),
							 (sizeType == SIZE_POT) ? (16) : (17),
							 samplingFunction,
							 FLAG_USE_SHADOW_SAMPLER)
	, m_filter				(filter)
{
}

void TextureBorderClampDepthCompareCase::init (void)
{
	TextureBorderClampTest::init();

	// 0.5 <= 0.7
	{
		IterationConfig iteration;
		iteration.p0			= tcu::Vec2(-0.15f, -0.35f);
		iteration.p1			= tcu::Vec2( 1.25f,  1.1f);
		iteration.borderColor	= rr::GenericVec4(tcu::Vec4(0.7f, 0.0f, 0.0f, 0.0f));
		iteration.description	= "Border color in [0, 1] range";
		iteration.compareMode	= GL_LEQUAL;
		iteration.compareRef	= 0.5f;
		m_iterations.push_back(iteration);
	}

	// 1.5 <= 1.0
	{
		IterationConfig iteration;
		iteration.p0			= tcu::Vec2(-0.15f, -0.35f);
		iteration.p1			= tcu::Vec2( 1.25f,  1.1f);
		iteration.borderColor	= rr::GenericVec4(tcu::Vec4(1.5f, 0.0f, 0.0f, 0.0f));
		iteration.description	= "Border color > 1, should be clamped";
		iteration.compareMode	= GL_LEQUAL;
		iteration.compareRef	= 1.0f;
		m_iterations.push_back(iteration);
	}

	// -0.5 >= 0.0
	{
		IterationConfig iteration;
		iteration.p0			= tcu::Vec2(-0.15f, -0.35f);
		iteration.p1			= tcu::Vec2( 1.25f,  1.1f);
		iteration.borderColor	= rr::GenericVec4(tcu::Vec4(-0.5f, 0.0f, 0.0f, 0.0f));
		iteration.description	= "Border color < 0, should be clamped";
		iteration.compareMode	= GL_GEQUAL;
		iteration.compareRef	= 0.0f;
		m_iterations.push_back(iteration);
	}

	// inf < 1.25
	{
		IterationConfig iteration;
		iteration.p0			= tcu::Vec2(-0.15f, -0.35f);
		iteration.p1			= tcu::Vec2( 1.25f,  1.1f);
		iteration.borderColor	= rr::GenericVec4(tcu::Vec4(std::numeric_limits<float>::infinity(), 0.0f, 0.0f, 0.0f));
		iteration.description	= "Border color == inf, should be clamped; ref > 1";
		iteration.compareMode	= GL_LESS;
		iteration.compareRef	= 1.25f;
		m_iterations.push_back(iteration);
	}

	// -inf > -0.5
	{
		IterationConfig iteration;
		iteration.p0			= tcu::Vec2(-0.15f, -0.35f);
		iteration.p1			= tcu::Vec2( 1.25f,  1.1f);
		iteration.borderColor	= rr::GenericVec4(tcu::Vec4(-std::numeric_limits<float>::infinity(), 0.0f, 0.0f, 0.0f));
		iteration.description	= "Border color == inf, should be clamped; ref < 0";
		iteration.compareMode	= GL_GREATER;
		iteration.compareRef	= -0.5f;
		m_iterations.push_back(iteration);
	}

	// common parameters
	for (int ndx = 0; ndx < (int)m_iterations.size(); ++ndx)
	{
		IterationConfig& iteration = m_iterations[ndx];

		iteration.lookupScale	= tcu::Vec4(1.0);
		iteration.lookupBias	= tcu::Vec4(0.0);
		iteration.minFilter		= m_filter;
		iteration.magFilter		= m_filter;
		iteration.sWrapMode		= GL_CLAMP_TO_BORDER;
		iteration.tWrapMode		= GL_CLAMP_TO_BORDER;
	}
}

int TextureBorderClampDepthCompareCase::getNumIterations	(void) const
{
	return (int)m_iterations.size();
}

TextureBorderClampTest::IterationConfig TextureBorderClampDepthCompareCase::getIteration (int ndx) const
{
	return m_iterations[ndx];
}

class TextureBorderClampUnusedChannelCase : public TextureBorderClampTest
{
public:
									TextureBorderClampUnusedChannelCase	(Context&						context,
																		 const char*					name,
																		 const char*					description,
																		 deUint32						texFormat,
																		 tcu::Sampler::DepthStencilMode	depthStencilMode);

private:
	void							init								(void);

	int								getNumIterations					(void) const;
	IterationConfig					getIteration						(int ndx) const;

	std::vector<IterationConfig>	m_iterations;
};

TextureBorderClampUnusedChannelCase::TextureBorderClampUnusedChannelCase (Context&							context,
																		  const char*						name,
																		  const char*						description,
																		  deUint32							texFormat,
																		  tcu::Sampler::DepthStencilMode	depthStencilMode)
	: TextureBorderClampTest(context,
							 name,
							 description,
							 texFormat,
							 depthStencilMode,
							 TextureBorderClampTest::STATE_TEXTURE_PARAM,
							 8,
							 8,
							 SAMPLE_FILTER)
{
}

static rr::GenericVec4 selectComponents (const rr::GenericVec4& trueComponents, const rr::GenericVec4& falseComponents, const tcu::BVec4& m)
{
	return rr::GenericVec4(tcu::select(trueComponents.get<deUint32>(), falseComponents.get<deUint32>(), m));
}

void TextureBorderClampUnusedChannelCase::init (void)
{
	TextureBorderClampTest::init();

	// \note TextureBorderClampTest::init() creates texture
	const tcu::TextureFormat		texFormat			= tcu::getEffectiveDepthStencilTextureFormat(getTexture()->getRefTexture().getFormat(), m_sampleMode);
	const tcu::TextureFormatInfo	texFormatInfo		= tcu::getTextureFormatInfo(texFormat);
	const tcu::BVec4				channelMask			= tcu::getTextureFormatChannelMask(texFormat);
	const float						maxChannelValue		= (channelMask[0]) ? (texFormatInfo.valueMax[0])
														: (channelMask[1]) ? (texFormatInfo.valueMax[1])
														: (channelMask[2]) ? (texFormatInfo.valueMax[2])
														:                    (texFormatInfo.valueMax[3]);

	const rr::GenericVec4			effectiveColors		= mapToFormatColorRepresentable(texFormat, tcu::Vec4(0.6f));
	rr::GenericVec4					nonEffectiveColors;

	switch (m_channelClass)
	{
		case tcu::TEXTURECHANNELCLASS_UNSIGNED_FIXED_POINT:
		case tcu::TEXTURECHANNELCLASS_SIGNED_FIXED_POINT:
		case tcu::TEXTURECHANNELCLASS_FLOATING_POINT:
			nonEffectiveColors = rr::GenericVec4(tcu::Vec4(maxChannelValue * 0.8f));
			break;

		case tcu::TEXTURECHANNELCLASS_SIGNED_INTEGER:
			nonEffectiveColors = rr::GenericVec4(tcu::Vec4(maxChannelValue * 0.8f).cast<deInt32>());
			break;

		case tcu::TEXTURECHANNELCLASS_UNSIGNED_INTEGER:
			nonEffectiveColors = rr::GenericVec4(tcu::Vec4(maxChannelValue * 0.8f).cast<deUint32>());
			break;
		default:
			DE_ASSERT(false);
	}

	IterationConfig iteration;
	iteration.p0			= tcu::Vec2(-0.25f, -0.75f);
	iteration.p1			= tcu::Vec2( 2.25f,  1.25f);
	iteration.borderColor	= selectComponents(effectiveColors, nonEffectiveColors, channelMask);
	iteration.lookupScale	= texFormatInfo.lookupScale;
	iteration.lookupBias	= texFormatInfo.lookupBias;
	iteration.minFilter		= GL_NEAREST;
	iteration.magFilter		= GL_NEAREST;
	iteration.sWrapMode		= GL_CLAMP_TO_BORDER;
	iteration.tWrapMode		= GL_CLAMP_TO_BORDER;
	iteration.compareMode	= GL_NONE;
	iteration.compareRef	= 0.0f;
	iteration.description	= "Setting values to unused border color components";

	m_iterations.push_back(iteration);
}

int TextureBorderClampUnusedChannelCase::getNumIterations	(void) const
{
	return (int)m_iterations.size();
}

TextureBorderClampTest::IterationConfig TextureBorderClampUnusedChannelCase::getIteration (int ndx) const
{
	return m_iterations[ndx];
}

class TextureBorderClampPerAxisCase3D : public TestCase
{
public:
														TextureBorderClampPerAxisCase3D	(Context&		context,
																						 const char*	name,
																						 const char*	description,
																						 deUint32		texFormat,
																						 SizeType		size,
																						 deUint32		filter,
																						 deUint32		sWrap,
																						 deUint32		tWrap,
																						 deUint32		rWrap);

private:
	void												init							(void);
	void												deinit							(void);
	IterateResult										iterate							(void);

	void												renderTo						(tcu::Surface&									surface,
																						 const glu::TextureTestUtil::ReferenceParams&	samplerParams);

	void												logParams						(const glu::TextureTestUtil::ReferenceParams&	samplerParams);

	void												verifyImage						(const tcu::Surface&							image,
																						 const glu::TextureTestUtil::ReferenceParams&	samplerParams);

	glu::TextureTestUtil::ReferenceParams				getSamplerParams				(void) const;
	deUint32											getCaseSeed						(void) const;

	enum
	{
		VIEWPORT_WIDTH		= 128,
		VIEWPORT_HEIGHT		= 128,
	};

	const deUint32										m_texFormat;
	const tcu::TextureChannelClass						m_channelClass;
	const tcu::IVec3									m_size;
	const deUint32										m_filter;
	const deUint32										m_sWrap;
	const deUint32										m_tWrap;
	const deUint32										m_rWrap;

	de::MovePtr<glu::Texture3D>							m_texture;
	de::MovePtr<gls::TextureTestUtil::TextureRenderer>	m_renderer;

	rr::GenericVec4										m_borderColor;
	std::vector<float>									m_texCoords;
	tcu::Vec4											m_lookupScale;
	tcu::Vec4											m_lookupBias;
};

TextureBorderClampPerAxisCase3D::TextureBorderClampPerAxisCase3D (Context&		context,
																  const char*	name,
																  const char*	description,
																  deUint32		texFormat,
																  SizeType		size,
																  deUint32		filter,
																  deUint32		sWrap,
																  deUint32		tWrap,
																  deUint32		rWrap)
	: TestCase			(context, name, description)
	, m_texFormat		(texFormat)
	, m_channelClass	(getFormatChannelClass(texFormat, tcu::Sampler::MODE_LAST))
	, m_size			((size == SIZE_POT) ? (tcu::IVec3(8, 16, 4)) : (tcu::IVec3(13, 5, 7)))
	, m_filter			(filter)
	, m_sWrap			(sWrap)
	, m_tWrap			(tWrap)
	, m_rWrap			(rWrap)
{
}

void TextureBorderClampPerAxisCase3D::init (void)
{
	const bool				supportsES32	= glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::es(3, 2));
	const glu::GLSLVersion	glslVersion		= glu::getContextTypeGLSLVersion(m_context.getRenderContext().getType());

	if (!supportsES32 && !m_context.getContextInfo().isExtensionSupported("GL_EXT_texture_border_clamp"))
		throw tcu::NotSupportedError("Test requires GL_EXT_texture_border_clamp extension");

	if (glu::isCompressedFormat(m_texFormat)													&&
		!supportsES32																			&&
		tcu::isAstcFormat(glu::mapGLCompressedTexFormat(m_texFormat))							&&
		!m_context.getContextInfo().isExtensionSupported("GL_KHR_texture_compression_astc_ldr"))
	{
		throw tcu::NotSupportedError("Test requires GL_KHR_texture_compression_astc_ldr extension");
	}
	if (m_texFormat == GL_BGRA && !m_context.getContextInfo().isExtensionSupported("GL_EXT_texture_format_BGRA8888"))
		throw tcu::NotSupportedError("Test requires GL_EXT_texture_format_BGRA8888 extension");
	if (m_context.getRenderTarget().getWidth() < VIEWPORT_WIDTH ||
		m_context.getRenderTarget().getHeight() < VIEWPORT_HEIGHT)
	{
		throw tcu::NotSupportedError("Test requires " + de::toString<int>(VIEWPORT_WIDTH) + "x" + de::toString<int>(VIEWPORT_HEIGHT) + " viewport");
	}

	// resources
	m_texture = genDummyTexture<glu::Texture3D>(m_context.getRenderContext(), m_context.getContextInfo(), m_texFormat, m_size);
	m_renderer = de::MovePtr<gls::TextureTestUtil::TextureRenderer>(new gls::TextureTestUtil::TextureRenderer(m_context.getRenderContext(), m_testCtx.getLog(), glslVersion, glu::PRECISION_HIGHP));

	// texture info
	m_testCtx.getLog()	<< tcu::TestLog::Message
						<< "Created 3D texture with format " << glu::getTextureFormatName(m_texFormat)
						<< ", size (" << m_texture->getRefTexture().getWidth() << ", " << m_texture->getRefTexture().getHeight() << ", " << m_texture->getRefTexture().getDepth() << ")\n"
						<< tcu::TestLog::EndMessage;

	// tex coord
	{
		m_testCtx.getLog()	<< tcu::TestLog::Message
							<< "Setting tex coords bottom-left: (-1, -1, -1.5), top-right (2, 2, 2.5)\n"
							<< tcu::TestLog::EndMessage;

		m_texCoords.resize(4*3);

		m_texCoords[0] = -1.0f; m_texCoords[ 1] = -1.0f; m_texCoords[ 2] = -1.5f;
		m_texCoords[3] = -1.0f; m_texCoords[ 4] =  2.0f; m_texCoords[ 5] = 0.5f;
		m_texCoords[6] =  2.0f; m_texCoords[ 7] = -1.0f; m_texCoords[ 8] = 0.5f;
		m_texCoords[9] =  2.0f; m_texCoords[10] =  2.0f; m_texCoords[11] =  2.5f;
	}

	// set render params
	{
		const tcu::TextureFormat		texFormat		= m_texture->getRefTexture().getFormat();
		const tcu::TextureFormatInfo	texFormatInfo	= tcu::getTextureFormatInfo(texFormat);

		m_borderColor	= mapToFormatColorRepresentable(texFormat, tcu::Vec4(0.2f, 0.6f, 0.9f, 0.4f));

		m_lookupScale	= texFormatInfo.lookupScale;
		m_lookupBias	= texFormatInfo.lookupBias;
	}
}

void TextureBorderClampPerAxisCase3D::deinit (void)
{
	m_texture.clear();
	m_renderer.clear();
}

TextureBorderClampPerAxisCase3D::IterateResult TextureBorderClampPerAxisCase3D::iterate (void)
{
	tcu::Surface								renderedFrame		(VIEWPORT_WIDTH, VIEWPORT_HEIGHT);
	const glu::TextureTestUtil::ReferenceParams	samplerParams		= getSamplerParams();

	logParams(samplerParams);
	renderTo(renderedFrame, samplerParams);
	verifyImage(renderedFrame, samplerParams);

	return STOP;
}

void TextureBorderClampPerAxisCase3D::logParams (const glu::TextureTestUtil::ReferenceParams& samplerParams)
{
	const std::string	borderColorString	= (m_channelClass == tcu::TEXTURECHANNELCLASS_SIGNED_INTEGER)   ? (de::toString(m_borderColor.get<deInt32>()))
											: (m_channelClass == tcu::TEXTURECHANNELCLASS_UNSIGNED_INTEGER) ? (de::toString(m_borderColor.get<deUint32>()))
											:																  (de::toString(m_borderColor.get<float>()));

	m_testCtx.getLog()	<< tcu::TestLog::Message
						<< "Border color is " << borderColorString << "\n"
						<< "Texture lookup bias: " << samplerParams.colorBias << "\n"
						<< "Texture lookup scale: " << samplerParams.colorScale << "\n"
						<< "Filter: " << glu::getTextureFilterName(m_filter) << "\n"
						<< "Wrap mode: s = " << glu::getRepeatModeStr(m_sWrap)
							<< ", t = " << glu::getRepeatModeStr(m_tWrap)
							<< ", r = " << glu::getRepeatModeStr(m_rWrap) << "\n"
						<< tcu::TestLog::EndMessage;
}

void TextureBorderClampPerAxisCase3D::renderTo (tcu::Surface&									surface,
												const glu::TextureTestUtil::ReferenceParams&	samplerParams)
{
	const glw::Functions&						gl			= m_context.getRenderContext().getFunctions();
	const gls::TextureTestUtil::RandomViewport	viewport	(m_context.getRenderTarget(), VIEWPORT_WIDTH, VIEWPORT_HEIGHT, getCaseSeed());

	// Bind to unit 0.
	gl.activeTexture(GL_TEXTURE0);
	gl.bindTexture(GL_TEXTURE_3D, m_texture->getGLTexture());

	// Setup filtering and wrap modes.
	gl.texParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S,		glu::getGLWrapMode(samplerParams.sampler.wrapS));
	gl.texParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T,		glu::getGLWrapMode(samplerParams.sampler.wrapT));
	gl.texParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R,		glu::getGLWrapMode(samplerParams.sampler.wrapR));
	gl.texParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER,	glu::getGLFilterMode(samplerParams.sampler.minFilter));
	gl.texParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER,	glu::getGLFilterMode(samplerParams.sampler.magFilter));

	switch (m_channelClass)
	{
		case tcu::TEXTURECHANNELCLASS_SIGNED_FIXED_POINT:
		case tcu::TEXTURECHANNELCLASS_UNSIGNED_FIXED_POINT:
		case tcu::TEXTURECHANNELCLASS_FLOATING_POINT:
			gl.texParameterfv(GL_TEXTURE_3D, GL_TEXTURE_BORDER_COLOR, m_borderColor.getAccess<float>());
			break;

		case tcu::TEXTURECHANNELCLASS_SIGNED_INTEGER:
			gl.texParameterIiv(GL_TEXTURE_3D, GL_TEXTURE_BORDER_COLOR, m_borderColor.getAccess<deInt32>());
			break;

		case tcu::TEXTURECHANNELCLASS_UNSIGNED_INTEGER:
			gl.texParameterIuiv(GL_TEXTURE_3D, GL_TEXTURE_BORDER_COLOR, m_borderColor.getAccess<deUint32>());
			break;

		default:
			DE_ASSERT(false);
	}

	GLU_EXPECT_NO_ERROR(gl.getError(), "Set texturing state");

	gl.viewport(viewport.x, viewport.y, viewport.width, viewport.height);
	m_renderer->renderQuad(0, &m_texCoords[0], samplerParams);
	glu::readPixels(m_context.getRenderContext(), viewport.x, viewport.y, surface.getAccess());
}

void TextureBorderClampPerAxisCase3D::verifyImage (const tcu::Surface&							renderedFrame,
												   const glu::TextureTestUtil::ReferenceParams&	samplerParams)
{
	const tcu::PixelFormat			pixelFormat			= m_context.getRenderTarget().getPixelFormat();
	const int						colorErrorBits		= 2;
	const tcu::IVec4				colorBits			= tcu::max(glu::TextureTestUtil::getBitsVec(pixelFormat) - tcu::IVec4(colorErrorBits), tcu::IVec4(0));
	tcu::Surface					reference			(renderedFrame.getWidth(), renderedFrame.getHeight());
	tcu::Surface					errorMask			(renderedFrame.getWidth(), renderedFrame.getHeight());
	tcu::LodPrecision				lodPrecision;
	tcu::LookupPrecision			lookupPrecision;
	int								numFailedPixels;

	lodPrecision.derivateBits		= 18;
	lodPrecision.lodBits			= 5;

	lookupPrecision.colorThreshold	= tcu::computeFixedPointThreshold(colorBits) / samplerParams.colorScale;
	lookupPrecision.coordBits		= tcu::IVec3(20,20,0);
	lookupPrecision.uvwBits			= tcu::IVec3(5,5,0);
	lookupPrecision.colorMask		= glu::TextureTestUtil::getCompareMask(pixelFormat);

	glu::TextureTestUtil::sampleTexture(tcu::SurfaceAccess(reference, pixelFormat), m_texture->getRefTexture(), &m_texCoords[0], samplerParams);

	numFailedPixels = glu::TextureTestUtil::computeTextureLookupDiff(renderedFrame.getAccess(), reference.getAccess(), errorMask.getAccess(), m_texture->getRefTexture(),
																	 &m_texCoords[0], samplerParams, lookupPrecision, lodPrecision, m_testCtx.getWatchDog());

	if (numFailedPixels > 0)
		m_testCtx.getLog() << tcu::TestLog::Message << "ERROR: Result verification failed, got " << numFailedPixels << " invalid pixels!" << tcu::TestLog::EndMessage;
	m_testCtx.getLog()	<< tcu::TestLog::ImageSet("VerifyResult", "Verification result")
						<< tcu::TestLog::Image("Rendered", "Rendered image", renderedFrame);
	if (numFailedPixels > 0)
	{
		m_testCtx.getLog()	<< tcu::TestLog::Image("Reference", "Ideal reference image", reference)
							<< tcu::TestLog::Image("ErrorMask", "Error mask", errorMask);
	}
	m_testCtx.getLog() << tcu::TestLog::EndImageSet;

	if (numFailedPixels == 0)
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	else
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Image verification failed");
}

glu::TextureTestUtil::ReferenceParams TextureBorderClampPerAxisCase3D::getSamplerParams (void) const
{
	const tcu::TextureFormat				texFormat		= m_texture->getRefTexture().getFormat();
	glu::TextureTestUtil::ReferenceParams	refParams		(glu::TextureTestUtil::TEXTURETYPE_3D);

	refParams.sampler					= glu::mapGLSampler(m_sWrap, m_tWrap, m_rWrap, m_filter, m_filter);
	refParams.sampler.borderColor		= m_borderColor;
	refParams.lodMode					= glu::TextureTestUtil::LODMODE_EXACT;
	refParams.samplerType				= glu::TextureTestUtil::getSamplerType(texFormat);
	refParams.colorScale				= m_lookupScale;
	refParams.colorBias					= m_lookupBias;

	return refParams;
}

deUint32 TextureBorderClampPerAxisCase3D::getCaseSeed (void) const
{
	tcu::SeedBuilder builder;
	builder	<< std::string(getName())
			<< m_texFormat
			<< m_filter
			<< m_sWrap
			<< m_tWrap
			<< m_rWrap
			<< m_texture->getRefTexture().getWidth()
			<< m_texture->getRefTexture().getHeight()
			<< m_texture->getRefTexture().getDepth();
	return builder.get();
}

} // anonymous

TextureBorderClampTests::TextureBorderClampTests (Context& context)
	: TestCaseGroup(context, "border_clamp", "EXT_texture_border_clamp tests")
{
}

TextureBorderClampTests::~TextureBorderClampTests (void)
{
}

void TextureBorderClampTests::init (void)
{
	static const struct
	{
		const char*									name;
		deUint32									filter;
		TextureBorderClampTest::SamplingFunction	sampling;
	} s_filters[] =
	{
		{ "nearest",	GL_NEAREST,	TextureBorderClampTest::SAMPLE_FILTER	},
		{ "linear",		GL_LINEAR,	TextureBorderClampTest::SAMPLE_FILTER	},
		{ "gather",		GL_NEAREST,	TextureBorderClampTest::SAMPLE_GATHER	},
	};

	// .formats
	{
		static const struct
		{
			const char*						name;
			deUint32						format;
			tcu::Sampler::DepthStencilMode	mode;
		} formats[] =
		{
			{ "luminance",									GL_LUMINANCE,									tcu::Sampler::MODE_LAST		},
			{ "alpha",										GL_ALPHA,										tcu::Sampler::MODE_LAST		},
			{ "luminance_alpha",							GL_LUMINANCE_ALPHA,								tcu::Sampler::MODE_LAST		},
			{ "bgra",										GL_BGRA,										tcu::Sampler::MODE_LAST		},
			{ "r8",											GL_R8,											tcu::Sampler::MODE_LAST		},
			{ "r8_snorm",									GL_R8_SNORM,									tcu::Sampler::MODE_LAST		},
			{ "rg8",										GL_RG8,											tcu::Sampler::MODE_LAST		},
			{ "rg8_snorm",									GL_RG8_SNORM,									tcu::Sampler::MODE_LAST		},
			{ "rgb8",										GL_RGB8,										tcu::Sampler::MODE_LAST		},
			{ "rgb8_snorm",									GL_RGB8_SNORM,									tcu::Sampler::MODE_LAST		},
			{ "rgb565",										GL_RGB565,										tcu::Sampler::MODE_LAST		},
			{ "rgba4",										GL_RGBA4,										tcu::Sampler::MODE_LAST		},
			{ "rgb5_a1",									GL_RGB5_A1,										tcu::Sampler::MODE_LAST		},
			{ "rgba8",										GL_RGBA8,										tcu::Sampler::MODE_LAST		},
			{ "rgba8_snorm",								GL_RGBA8_SNORM,									tcu::Sampler::MODE_LAST		},
			{ "rgb10_a2",									GL_RGB10_A2,									tcu::Sampler::MODE_LAST		},
			{ "rgb10_a2ui",									GL_RGB10_A2UI,									tcu::Sampler::MODE_LAST		},
			{ "srgb8",										GL_SRGB8,										tcu::Sampler::MODE_LAST		},
			{ "srgb8_alpha8",								GL_SRGB8_ALPHA8,								tcu::Sampler::MODE_LAST		},
			{ "r16f",										GL_R16F,										tcu::Sampler::MODE_LAST		},
			{ "rg16f",										GL_RG16F,										tcu::Sampler::MODE_LAST		},
			{ "rgb16f",										GL_RGB16F,										tcu::Sampler::MODE_LAST		},
			{ "rgba16f",									GL_RGBA16F,										tcu::Sampler::MODE_LAST		},
			{ "r32f",										GL_R32F,										tcu::Sampler::MODE_LAST		},
			{ "rg32f",										GL_RG32F,										tcu::Sampler::MODE_LAST		},
			{ "rgb32f",										GL_RGB32F,										tcu::Sampler::MODE_LAST		},
			{ "rgba32f",									GL_RGBA32F,										tcu::Sampler::MODE_LAST		},
			{ "r11f_g11f_b10f",								GL_R11F_G11F_B10F,								tcu::Sampler::MODE_LAST		},
			{ "rgb9_e5",									GL_RGB9_E5,										tcu::Sampler::MODE_LAST		},
			{ "r8i",										GL_R8I,											tcu::Sampler::MODE_LAST		},
			{ "r8ui",										GL_R8UI,										tcu::Sampler::MODE_LAST		},
			{ "r16i",										GL_R16I,										tcu::Sampler::MODE_LAST		},
			{ "r16ui",										GL_R16UI,										tcu::Sampler::MODE_LAST		},
			{ "r32i",										GL_R32I,										tcu::Sampler::MODE_LAST		},
			{ "r32ui",										GL_R32UI,										tcu::Sampler::MODE_LAST		},
			{ "rg8i",										GL_RG8I,										tcu::Sampler::MODE_LAST		},
			{ "rg8ui",										GL_RG8UI,										tcu::Sampler::MODE_LAST		},
			{ "rg16i",										GL_RG16I,										tcu::Sampler::MODE_LAST		},
			{ "rg16ui",										GL_RG16UI,										tcu::Sampler::MODE_LAST		},
			{ "rg32i",										GL_RG32I,										tcu::Sampler::MODE_LAST		},
			{ "rg32ui",										GL_RG32UI,										tcu::Sampler::MODE_LAST		},
			{ "rgb8i",										GL_RGB8I,										tcu::Sampler::MODE_LAST		},
			{ "rgb8ui",										GL_RGB8UI,										tcu::Sampler::MODE_LAST		},
			{ "rgb16i",										GL_RGB16I,										tcu::Sampler::MODE_LAST		},
			{ "rgb16ui",									GL_RGB16UI,										tcu::Sampler::MODE_LAST		},
			{ "rgb32i",										GL_RGB32I,										tcu::Sampler::MODE_LAST		},
			{ "rgb32ui",									GL_RGB32UI,										tcu::Sampler::MODE_LAST		},
			{ "rgba8i",										GL_RGBA8I,										tcu::Sampler::MODE_LAST		},
			{ "rgba8ui",									GL_RGBA8UI,										tcu::Sampler::MODE_LAST		},
			{ "rgba16i",									GL_RGBA16I,										tcu::Sampler::MODE_LAST		},
			{ "rgba16ui",									GL_RGBA16UI,									tcu::Sampler::MODE_LAST		},
			{ "rgba32i",									GL_RGBA32I,										tcu::Sampler::MODE_LAST		},
			{ "rgba32ui",									GL_RGBA32UI,									tcu::Sampler::MODE_LAST		},
			{ "depth_component16",							GL_DEPTH_COMPONENT16,							tcu::Sampler::MODE_DEPTH	},
			{ "depth_component24",							GL_DEPTH_COMPONENT24,							tcu::Sampler::MODE_DEPTH	},
			{ "depth_component32f",							GL_DEPTH_COMPONENT32F,							tcu::Sampler::MODE_DEPTH	},
			{ "stencil_index8",								GL_STENCIL_INDEX8,								tcu::Sampler::MODE_STENCIL	},
			{ "depth24_stencil8_sample_depth",				GL_DEPTH24_STENCIL8,							tcu::Sampler::MODE_DEPTH	},
			{ "depth32f_stencil8_sample_depth",				GL_DEPTH32F_STENCIL8,							tcu::Sampler::MODE_DEPTH	},
			{ "depth24_stencil8_sample_stencil",			GL_DEPTH24_STENCIL8,							tcu::Sampler::MODE_STENCIL	},
			{ "depth32f_stencil8_sample_stencil",			GL_DEPTH32F_STENCIL8,							tcu::Sampler::MODE_STENCIL	},
			{ "compressed_r11_eac",							GL_COMPRESSED_R11_EAC,							tcu::Sampler::MODE_LAST		},
			{ "compressed_signed_r11_eac",					GL_COMPRESSED_SIGNED_R11_EAC,					tcu::Sampler::MODE_LAST		},
			{ "compressed_rg11_eac",						GL_COMPRESSED_RG11_EAC,							tcu::Sampler::MODE_LAST		},
			{ "compressed_signed_rg11_eac",					GL_COMPRESSED_SIGNED_RG11_EAC,					tcu::Sampler::MODE_LAST		},
			{ "compressed_rgb8_etc2",						GL_COMPRESSED_RGB8_ETC2,						tcu::Sampler::MODE_LAST		},
			{ "compressed_srgb8_etc2",						GL_COMPRESSED_SRGB8_ETC2,						tcu::Sampler::MODE_LAST		},
			{ "compressed_rgb8_punchthrough_alpha1_etc2",	GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2,	tcu::Sampler::MODE_LAST		},
			{ "compressed_srgb8_punchthrough_alpha1_etc2",	GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2,	tcu::Sampler::MODE_LAST		},
			{ "compressed_rgba8_etc2_eac",					GL_COMPRESSED_RGBA8_ETC2_EAC,					tcu::Sampler::MODE_LAST		},
			{ "compressed_srgb8_alpha8_etc2_eac",			GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC,			tcu::Sampler::MODE_LAST		},
		};

		tcu::TestCaseGroup* const formatsGroup = new tcu::TestCaseGroup(m_testCtx, "formats", "Format tests");
		addChild(formatsGroup);

		// .format
		for (int formatNdx = 0; formatNdx < DE_LENGTH_OF_ARRAY(formats); ++formatNdx)
		{
			const deUint32							format			= formats[formatNdx].format;
			const tcu::Sampler::DepthStencilMode	sampleMode		= formats[formatNdx].mode;
			const bool								isCompressed	= glu::isCompressedFormat(format);
			const bool								coreFilterable	= isCoreFilterableFormat(format, sampleMode);
			tcu::TestCaseGroup* const				formatGroup		= new tcu::TestCaseGroup(m_testCtx, formats[formatNdx].name, "Format test");

			formatsGroup->addChild(formatGroup);

			// .nearest
			// .linear
			for (int filterNdx = 0; filterNdx < DE_LENGTH_OF_ARRAY(s_filters); ++filterNdx)
			{
				// [not-compressed]
				// .size_pot
				// .size_npot
				// [compressed]
				// .size_tile_multiple (also pot)
				// .size_not_tile_multiple (also npot)
				for (int sizeNdx = 0; sizeNdx < 2; ++sizeNdx)
				{
					const bool				isNpotCase		= (sizeNdx == 1);
					const char* const		sizePotName		= (!isCompressed) ? ("size_pot") : ("size_tile_multiple");
					const char* const		sizeNpotName	= (!isCompressed) ? ("size_npot") : ("size_not_tile_multiple");
					const char* const		sizeName		= (isNpotCase) ? (sizeNpotName) : (sizePotName);
					const SizeType			sizeType		= (isNpotCase) ? (SIZE_NPOT) : (SIZE_POT);
					const std::string		caseName		= std::string() + s_filters[filterNdx].name + "_" + sizeName;
					const deUint32			filter			= s_filters[filterNdx].filter;

					if (coreFilterable || !filterRequiresFilterability(filter))
						formatGroup->addChild(new TextureBorderClampFormatCase(m_context,
																			   caseName.c_str(),
																			   "",
																			   format,
																			   sampleMode,
																			   TextureBorderClampFormatCase::STATE_TEXTURE_PARAM,
																			   sizeType,
																			   filter,
																			   s_filters[filterNdx].sampling));
				}
			}
		}
	}

	// .range_clamp
	{
		static const struct
		{
			const char*						name;
			deUint32						format;
			tcu::Sampler::DepthStencilMode	mode;
		} formats[] =
		{
			{ "unorm_color",								GL_R8,					tcu::Sampler::MODE_LAST		},
			{ "snorm_color",								GL_R8_SNORM,			tcu::Sampler::MODE_LAST		},
			{ "float_color",								GL_RG32F,				tcu::Sampler::MODE_LAST		},
			{ "int_color",									GL_R8I,					tcu::Sampler::MODE_LAST		},
			{ "uint_color",									GL_R16UI,				tcu::Sampler::MODE_LAST		},
			{ "srgb_color",									GL_SRGB8_ALPHA8,		tcu::Sampler::MODE_LAST		},
			{ "unorm_depth",								GL_DEPTH_COMPONENT24,	tcu::Sampler::MODE_DEPTH	},
			{ "float_depth",								GL_DEPTH_COMPONENT32F,	tcu::Sampler::MODE_DEPTH	},
			{ "uint_stencil",								GL_STENCIL_INDEX8,		tcu::Sampler::MODE_STENCIL	},
			{ "float_depth_uint_stencil_sample_depth",		GL_DEPTH32F_STENCIL8,	tcu::Sampler::MODE_DEPTH	},
			{ "float_depth_uint_stencil_sample_stencil",	GL_DEPTH32F_STENCIL8,	tcu::Sampler::MODE_STENCIL	},
			{ "unorm_depth_uint_stencil_sample_depth",		GL_DEPTH24_STENCIL8,	tcu::Sampler::MODE_DEPTH	},
			{ "unorm_depth_uint_stencil_sample_stencil",	GL_DEPTH24_STENCIL8,	tcu::Sampler::MODE_STENCIL	},
			{ "compressed_color",							GL_COMPRESSED_RG11_EAC,	tcu::Sampler::MODE_LAST		},
		};

		tcu::TestCaseGroup* const rangeClampGroup = new tcu::TestCaseGroup(m_testCtx, "range_clamp", "Range clamp tests");
		addChild(rangeClampGroup);

		for (int formatNdx = 0; formatNdx < DE_LENGTH_OF_ARRAY(formats); ++formatNdx)
		for (int filterNdx = 0; filterNdx < DE_LENGTH_OF_ARRAY(s_filters); ++filterNdx)
		{
			const deUint32							format			= formats[formatNdx].format;
			const tcu::Sampler::DepthStencilMode	sampleMode		= formats[formatNdx].mode;
			const std::string						caseName		= std::string() + s_filters[filterNdx].name + "_" + formats[formatNdx].name;
			const deUint32							filter			= s_filters[filterNdx].filter;
			const bool								coreFilterable	= isCoreFilterableFormat(format, sampleMode);

			if (s_filters[filterNdx].sampling == TextureBorderClampTest::SAMPLE_GATHER)
				continue;

			if (coreFilterable || !filterRequiresFilterability(filter))
				rangeClampGroup->addChild(new TextureBorderClampRangeClampCase(m_context, caseName.c_str(), "", format, sampleMode, filter));
		}
	}

	// .sampler
	{
		static const struct
		{
			const char*						name;
			deUint32						format;
			tcu::Sampler::DepthStencilMode	mode;
		} formats[] =
		{
			{ "unorm_color",		GL_R8,					tcu::Sampler::MODE_LAST		},
			{ "snorm_color",		GL_R8_SNORM,			tcu::Sampler::MODE_LAST		},
			{ "float_color",		GL_RG32F,				tcu::Sampler::MODE_LAST		},
			{ "int_color",			GL_R8I,					tcu::Sampler::MODE_LAST		},
			{ "uint_color",			GL_R16UI,				tcu::Sampler::MODE_LAST		},
			{ "unorm_depth",		GL_DEPTH_COMPONENT24,	tcu::Sampler::MODE_DEPTH	},
			{ "float_depth",		GL_DEPTH_COMPONENT32F,	tcu::Sampler::MODE_DEPTH	},
			{ "uint_stencil",		GL_STENCIL_INDEX8,		tcu::Sampler::MODE_STENCIL	},
			{ "compressed_color",	GL_COMPRESSED_RG11_EAC,	tcu::Sampler::MODE_LAST		},
		};

		tcu::TestCaseGroup* const samplerGroup = new tcu::TestCaseGroup(m_testCtx, "sampler", "Sampler param tests");
		addChild(samplerGroup);

		for (int formatNdx = 0; formatNdx < DE_LENGTH_OF_ARRAY(formats); ++formatNdx)
		{
			const deUint32							format		= formats[formatNdx].format;
			const tcu::Sampler::DepthStencilMode	sampleMode	= formats[formatNdx].mode;
			const char*								caseName	= formats[formatNdx].name;

			samplerGroup->addChild(new TextureBorderClampFormatCase(m_context,
																	caseName,
																	"",
																	format,
																	sampleMode,
																	TextureBorderClampFormatCase::STATE_SAMPLER_PARAM,
																	SIZE_POT,
																	GL_NEAREST,
																	TextureBorderClampFormatCase::SAMPLE_FILTER));
		}
	}

	// .per_axis_wrap_mode
	{
		static const struct
		{
			const char*						name;
			bool							is3D;
		} targets[] =
		{
			{ "texture_2d", false	},
			{ "texture_3d", true	},
		};
		static const struct
		{
			const char*						name;
			deUint32						format;
			tcu::Sampler::DepthStencilMode	mode;
			bool							supports3D;
		} formats[] =
		{
			{ "unorm_color",		GL_RG8,						tcu::Sampler::MODE_LAST,	true	},
			{ "snorm_color",		GL_RG8_SNORM,				tcu::Sampler::MODE_LAST,	true	},
			{ "float_color",		GL_R32F,					tcu::Sampler::MODE_LAST,	true	},
			{ "int_color",			GL_RG16I,					tcu::Sampler::MODE_LAST,	true	},
			{ "uint_color",			GL_R8UI,					tcu::Sampler::MODE_LAST,	true	},
			{ "unorm_depth",		GL_DEPTH_COMPONENT16,		tcu::Sampler::MODE_DEPTH,	false	},
			{ "float_depth",		GL_DEPTH32F_STENCIL8,		tcu::Sampler::MODE_DEPTH,	false	},
			{ "uint_stencil",		GL_DEPTH32F_STENCIL8,		tcu::Sampler::MODE_STENCIL,	false	},
			{ "compressed_color",	GL_COMPRESSED_RGB8_ETC2,	tcu::Sampler::MODE_LAST,	false	},
		};
		static const struct
		{
			const char*	name;
			deUint32	sWrap;
			deUint32	tWrap;
			deUint32	rWrap;
			bool		is3D;
		} wrapConfigs[] =
		{
			// 2d configs
			{ "s_clamp_to_edge_t_clamp_to_border",						GL_CLAMP_TO_EDGE,	GL_CLAMP_TO_BORDER,	GL_NONE,			false	},
			{ "s_repeat_t_clamp_to_border",								GL_REPEAT,			GL_CLAMP_TO_BORDER,	GL_NONE,			false	},
			{ "s_mirrored_repeat_t_clamp_to_border",					GL_MIRRORED_REPEAT,	GL_CLAMP_TO_BORDER,	GL_NONE,			false	},

			// 3d configs
			{ "s_clamp_to_border_t_clamp_to_border_r_clamp_to_border",	GL_CLAMP_TO_BORDER,	GL_CLAMP_TO_BORDER,	GL_CLAMP_TO_BORDER,	true	},
			{ "s_clamp_to_border_t_clamp_to_border_r_repeat",			GL_CLAMP_TO_BORDER,	GL_CLAMP_TO_BORDER,	GL_REPEAT,			true	},
			{ "s_mirrored_repeat_t_clamp_to_border_r_repeat",			GL_MIRRORED_REPEAT,	GL_CLAMP_TO_BORDER,	GL_REPEAT,			true	},
			{ "s_repeat_t_mirrored_repeat_r_clamp_to_border",			GL_REPEAT,			GL_MIRRORED_REPEAT,	GL_CLAMP_TO_BORDER,	true	},
		};

		tcu::TestCaseGroup* const perAxisGroup = new tcu::TestCaseGroup(m_testCtx, "per_axis_wrap_mode", "Per-axis wrapping modes");
		addChild(perAxisGroup);

		// .texture_nd
		for (int targetNdx = 0; targetNdx < DE_LENGTH_OF_ARRAY(targets); ++targetNdx)
		{
			tcu::TestCaseGroup* const targetGroup = new tcu::TestCaseGroup(m_testCtx, targets[targetNdx].name, "Texture target test");
			perAxisGroup->addChild(targetGroup);

			// .format
			for (int formatNdx = 0; formatNdx < DE_LENGTH_OF_ARRAY(formats); ++formatNdx)
			{
				if (targets[targetNdx].is3D && !formats[formatNdx].supports3D)
					continue;
				else
				{
					const deUint32							format			= formats[formatNdx].format;
					const tcu::Sampler::DepthStencilMode	sampleMode		= formats[formatNdx].mode;
					const bool								coreFilterable	= isCoreFilterableFormat(format, sampleMode);
					tcu::TestCaseGroup* const				formatGroup		= new tcu::TestCaseGroup(m_testCtx, formats[formatNdx].name, "Format test");
					targetGroup->addChild(formatGroup);

					// .linear
					// .nearest
					// .gather
					for (int filterNdx = 0; filterNdx < DE_LENGTH_OF_ARRAY(s_filters); ++filterNdx)
					{
						const deUint32 filter = s_filters[filterNdx].filter;

						if (!coreFilterable && filterRequiresFilterability(filter))
						{
							// skip linear on pure integers
							continue;
						}
						else if (s_filters[filterNdx].sampling == TextureBorderClampTest::SAMPLE_GATHER && targets[targetNdx].is3D)
						{
							// skip gather on 3d
							continue;
						}
						else
						{
							tcu::TestCaseGroup* const filteringGroup = new tcu::TestCaseGroup(m_testCtx, s_filters[filterNdx].name, "Tests with specific filter");
							formatGroup->addChild(filteringGroup);

							// .s_XXX_t_XXX(_r_XXX)
							for (int wrapNdx = 0; wrapNdx < DE_LENGTH_OF_ARRAY(wrapConfigs); ++wrapNdx)
							{
								if (wrapConfigs[wrapNdx].is3D != targets[targetNdx].is3D)
									continue;
								else
								{
									for (int sizeNdx = 0; sizeNdx < 2; ++sizeNdx)
									{
										const char* const		wrapName			= wrapConfigs[wrapNdx].name;
										const bool				isNpotCase			= (sizeNdx == 1);
										const char* const		sizeNameExtension	= (isNpotCase) ? ("_npot") : ("_pot");
										const SizeType			size				= (isNpotCase) ? (SIZE_NPOT) : (SIZE_POT);

										if (!targets[targetNdx].is3D)
											filteringGroup->addChild(new TextureBorderClampPerAxisCase2D(m_context,
																										 (std::string() + wrapName + sizeNameExtension).c_str(),
																										 "",
																										 format,
																										 sampleMode,
																										 size,
																										 filter,
																										 wrapConfigs[wrapNdx].sWrap,
																										 wrapConfigs[wrapNdx].tWrap,
																										 s_filters[filterNdx].sampling));
										else
										{
											DE_ASSERT(sampleMode == tcu::Sampler::MODE_LAST);
											filteringGroup->addChild(new TextureBorderClampPerAxisCase3D(m_context,
																										 (std::string() + wrapName + sizeNameExtension).c_str(),
																										 "",
																										 format,
																										 size,
																										 filter,
																										 wrapConfigs[wrapNdx].sWrap,
																										 wrapConfigs[wrapNdx].tWrap,
																										 wrapConfigs[wrapNdx].rWrap));
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}

	// .depth_compare_mode
	{
		static const struct
		{
			const char*						name;
			deUint32						format;
		} formats[] =
		{
			{ "depth_component16",		GL_DEPTH_COMPONENT16	},
			{ "depth_component24",		GL_DEPTH_COMPONENT24	},
			{ "depth24_stencil8",		GL_DEPTH24_STENCIL8		},
			{ "depth32f_stencil8",		GL_DEPTH32F_STENCIL8	},
		};

		tcu::TestCaseGroup* const compareGroup = new tcu::TestCaseGroup(m_testCtx, "depth_compare_mode", "Tests depth compare mode");
		addChild(compareGroup);

		for (int formatNdx = 0; formatNdx < DE_LENGTH_OF_ARRAY(formats); ++formatNdx)
		{
			const deUint32							format			= formats[formatNdx].format;
			tcu::TestCaseGroup* const				formatGroup		= new tcu::TestCaseGroup(m_testCtx, formats[formatNdx].name, "Format test");

			compareGroup->addChild(formatGroup);

			// (format).(linear|nearest|gather)_(pot|npot)
			for (int filterNdx = 0; filterNdx < DE_LENGTH_OF_ARRAY(s_filters); ++filterNdx)
			for (int sizeNdx = 0; sizeNdx < 2; ++sizeNdx)
			{
					const bool				isNpotCase		= (sizeNdx == 1);
					const char* const		sizeName		= (isNpotCase) ? ("size_npot") : ("size_pot");
					const SizeType			sizeType		= (isNpotCase) ? (SIZE_NPOT) : (SIZE_POT);
					const std::string		caseName		= std::string() + s_filters[filterNdx].name + "_" + sizeName;
					const deUint32			filter			= s_filters[filterNdx].filter;

					formatGroup->addChild(new TextureBorderClampDepthCompareCase(m_context,
																				 caseName.c_str(),
																				 "",
																				 format,
																				 sizeType,
																				 filter,
																				 s_filters[filterNdx].sampling));
			}
		}
	}

	// unused channels (A in rgb, G in stencil etc.)
	{
		static const struct
		{
			const char*						name;
			deUint32						format;
			tcu::Sampler::DepthStencilMode	mode;
		} formats[] =
		{
			{ "r8",										GL_R8,						tcu::Sampler::MODE_LAST		},
			{ "rg8_snorm",								GL_RG8_SNORM,				tcu::Sampler::MODE_LAST		},
			{ "rgb8",									GL_RGB8,					tcu::Sampler::MODE_LAST		},
			{ "rg32f",									GL_RG32F,					tcu::Sampler::MODE_LAST		},
			{ "r16i",									GL_RG16I,					tcu::Sampler::MODE_LAST		},
			{ "luminance",								GL_LUMINANCE,				tcu::Sampler::MODE_LAST		},
			{ "alpha",									GL_ALPHA,					tcu::Sampler::MODE_LAST		},
			{ "luminance_alpha",						GL_LUMINANCE_ALPHA,			tcu::Sampler::MODE_LAST		},
			{ "depth_component16",						GL_DEPTH_COMPONENT16,		tcu::Sampler::MODE_DEPTH	},
			{ "depth_component32f",						GL_DEPTH_COMPONENT32F,		tcu::Sampler::MODE_DEPTH	},
			{ "stencil_index8",							GL_STENCIL_INDEX8,			tcu::Sampler::MODE_STENCIL	},
			{ "depth32f_stencil8_sample_depth",			GL_DEPTH32F_STENCIL8,		tcu::Sampler::MODE_DEPTH	},
			{ "depth32f_stencil8_sample_stencil",		GL_DEPTH32F_STENCIL8,		tcu::Sampler::MODE_STENCIL	},
			{ "depth24_stencil8_sample_depth",			GL_DEPTH24_STENCIL8,		tcu::Sampler::MODE_DEPTH	},
			{ "depth24_stencil8_sample_stencil",		GL_DEPTH24_STENCIL8,		tcu::Sampler::MODE_STENCIL	},
			{ "compressed_r11_eac",						GL_COMPRESSED_R11_EAC,		tcu::Sampler::MODE_LAST		},
		};

		tcu::TestCaseGroup* const unusedGroup = new tcu::TestCaseGroup(m_testCtx, "unused_channels", "Tests channels that are not present in the internal format");
		addChild(unusedGroup);

		for (int formatNdx = 0; formatNdx < DE_LENGTH_OF_ARRAY(formats); ++formatNdx)
		{
			unusedGroup->addChild(new TextureBorderClampUnusedChannelCase(m_context,
																		  formats[formatNdx].name,
																		  "",
																		  formats[formatNdx].format,
																		  formats[formatNdx].mode));
		}
	}
}

} // Functional
} // gles31
} // deqp
