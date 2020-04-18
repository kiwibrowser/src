/*-------------------------------------------------------------------------
 * Vulkan Conformance Tests
 * ------------------------
 *
 * Copyright 2014 The Android Open Source Project
 * Copyright (c) 2016 The Khronos Group Inc.
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
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
 * \brief Shadow texture lookup tests.
 *//*--------------------------------------------------------------------*/

#include "vktTextureShadowTests.hpp"

#include "deMath.h"
#include "deString.h"
#include "deStringUtil.hpp"
#include "gluPixelTransfer.hpp"
#include "gluTextureTestUtil.hpp"
#include "glwEnums.hpp"
#include "glwFunctions.hpp"
#include "tcuImageIO.hpp"
#include "tcuRenderTarget.hpp"
#include "tcuTexCompareVerifier.hpp"
#include "tcuTexVerifierUtil.hpp"
#include "tcuTexture.hpp"
#include "tcuTextureUtil.hpp"
#include "vkImageUtil.hpp"
#include "vkTypeUtil.hpp"
#include "vktTestGroupUtil.hpp"
#include "vktTextureTestUtil.hpp"

using namespace vk;

namespace vkt
{
namespace texture
{
namespace
{

using std::vector;
using std::string;
using tcu::TestLog;
using tcu::Sampler;
using namespace texture::util;
using namespace glu::TextureTestUtil;

enum
{
	TEXCUBE_VIEWPORT_SIZE	= 28,
	TEX2D_VIEWPORT_WIDTH	= 64,
	TEX2D_VIEWPORT_HEIGHT	= 64
};

struct TextureShadowCommonTestCaseParameters
{
										TextureShadowCommonTestCaseParameters	(void);
	Sampler::CompareMode				compareOp;
	TextureBinding::ImageBackingMode	backingMode;
};

TextureShadowCommonTestCaseParameters::TextureShadowCommonTestCaseParameters (void)
	: compareOp							(Sampler::COMPAREMODE_EQUAL)
	, backingMode						(TextureBinding::IMAGE_BACKING_MODE_REGULAR)
{
}

struct Texture2DShadowTestCaseParameters : public Texture2DTestCaseParameters, public TextureShadowCommonTestCaseParameters
{
};

bool isFloatingPointDepthFormat (const tcu::TextureFormat& format)
{
	// Only two depth and depth-stencil formats are floating point
	return	(format.order == tcu::TextureFormat::D && format.type == tcu::TextureFormat::FLOAT) ||
			(format.order == tcu::TextureFormat::DS && format.type == tcu::TextureFormat::FLOAT_UNSIGNED_INT_24_8_REV);
}

void clampFloatingPointTexture (const tcu::PixelBufferAccess& access)
{
	DE_ASSERT(isFloatingPointDepthFormat(access.getFormat()));

	for (int z = 0; z < access.getDepth(); ++z)
	for (int y = 0; y < access.getHeight(); ++y)
	for (int x = 0; x < access.getWidth(); ++x)
		access.setPixDepth(de::clamp(access.getPixDepth(x, y, z), 0.0f, 1.0f), x, y, z);
}

void clampFloatingPointTexture (tcu::Texture2D& target)
{
	for (int level = 0; level < target.getNumLevels(); ++level)
		if (!target.isLevelEmpty(level))
			clampFloatingPointTexture(target.getLevel(level));
}

static void clampFloatingPointTexture (tcu::Texture2DArray& target)
{
	for (int level = 0; level < target.getNumLevels(); ++level)
		if (!target.isLevelEmpty(level))
			clampFloatingPointTexture(target.getLevel(level));
}

void clampFloatingPointTexture (tcu::TextureCube& target)
{
	for (int level = 0; level < target.getNumLevels(); ++level)
		for (int face = tcu::CUBEFACE_NEGATIVE_X; face < tcu::CUBEFACE_LAST; ++face)
			clampFloatingPointTexture(target.getLevelFace(level, (tcu::CubeFace)face));
}

tcu::PixelFormat getPixelFormat(tcu::TextureFormat texFormat)
{
	const tcu::IVec4			formatBitDepth		= tcu::getTextureFormatBitDepth(tcu::getEffectiveDepthStencilTextureFormat(texFormat, Sampler::MODE_DEPTH));
	return tcu::PixelFormat(formatBitDepth[0], formatBitDepth[1], formatBitDepth[2], formatBitDepth[3]);
}

template<typename TextureType>
bool verifyTexCompareResult (tcu::TestContext&						testCtx,
							 const tcu::ConstPixelBufferAccess&		result,
							 const TextureType&						src,
							 const float*							texCoord,
							 const ReferenceParams&					sampleParams,
							 const tcu::TexComparePrecision&		comparePrec,
							 const tcu::LodPrecision&				lodPrec,
							 const tcu::PixelFormat&				pixelFormat)
{
	tcu::TestLog&	log					= testCtx.getLog();
	tcu::Surface	reference			(result.getWidth(), result.getHeight());
	tcu::Surface	errorMask			(result.getWidth(), result.getHeight());
	const tcu::Vec3	nonShadowThreshold	= tcu::computeFixedPointThreshold(getBitsVec(pixelFormat)-1).swizzle(1,2,3);
	int				numFailedPixels;

	// sampleTexture() expects source image to be the same state as it would be in a GL implementation, that is
	// the floating point depth values should be in [0, 1] range as data is clamped during texture upload. Since
	// we don't have a separate "uploading" phase and just reuse the buffer we used for GL-upload, do the clamping
	// here if necessary.

	if (isFloatingPointDepthFormat(src.getFormat()))
	{
		TextureType clampedSource(src);

		clampFloatingPointTexture(clampedSource);

		// sample clamped values

		sampleTexture(tcu::SurfaceAccess(reference, pixelFormat), clampedSource, texCoord, sampleParams);
		numFailedPixels = computeTextureCompareDiff(result, reference.getAccess(), errorMask.getAccess(), clampedSource, texCoord, sampleParams, comparePrec, lodPrec, nonShadowThreshold);
	}
	else
	{
		// sample raw values (they are guaranteed to be in [0, 1] range as the format cannot represent any other values)

		sampleTexture(tcu::SurfaceAccess(reference, pixelFormat), src, texCoord, sampleParams);
		numFailedPixels = computeTextureCompareDiff(result, reference.getAccess(), errorMask.getAccess(), src, texCoord, sampleParams, comparePrec, lodPrec, nonShadowThreshold);
	}

	if (numFailedPixels > 0)
		log << TestLog::Message << "ERROR: Result verification failed, got " << numFailedPixels << " invalid pixels!" << TestLog::EndMessage;

	log << TestLog::ImageSet("VerifyResult", "Verification result")
		<< TestLog::Image("Rendered", "Rendered image", result);

	if (numFailedPixels > 0)
	{
		log << TestLog::Image("Reference", "Ideal reference image", reference)
			<< TestLog::Image("ErrorMask", "Error mask", errorMask);
	}

	log << TestLog::EndImageSet;

	return numFailedPixels == 0;
}

class Texture2DShadowTestInstance : public TestInstance
{
public:
	typedef Texture2DShadowTestCaseParameters	ParameterType;
												Texture2DShadowTestInstance		(Context& context, const ParameterType& testParameters);
												~Texture2DShadowTestInstance	(void);

	virtual tcu::TestStatus						iterate							(void);

private:
												Texture2DShadowTestInstance		(const Texture2DShadowTestInstance& other);
	Texture2DShadowTestInstance&				operator=						(const Texture2DShadowTestInstance& other);

	struct FilterCase
	{
		int			textureIndex;

		tcu::Vec2	minCoord;
		tcu::Vec2	maxCoord;
		float		ref;

		FilterCase	(void)
			: textureIndex(-1)
			, ref		(0.0f)
		{
		}

		FilterCase	(int tex_, const float ref_, const tcu::Vec2& minCoord_, const tcu::Vec2& maxCoord_)
			: textureIndex	(tex_)
			, minCoord		(minCoord_)
			, maxCoord		(maxCoord_)
			, ref			(ref_)
		{
		}
	};

	const ParameterType&			m_testParameters;
	std::vector<TestTexture2DSp>	m_textures;
	std::vector<FilterCase>			m_cases;

	TextureRenderer					m_renderer;

	int								m_caseNdx;
};

Texture2DShadowTestInstance::Texture2DShadowTestInstance (Context& context, const ParameterType& testParameters)
	: TestInstance			(context)
	, m_testParameters		(testParameters)
	, m_renderer			(context, testParameters.sampleCount, TEX2D_VIEWPORT_WIDTH, TEX2D_VIEWPORT_HEIGHT)
	, m_caseNdx				(0)
{
	// Create 2 textures.
	m_textures.reserve(2);
	for (int ndx = 0; ndx < 2; ndx++)
	{
		m_textures.push_back(TestTexture2DSp(new pipeline::TestTexture2D(vk::mapVkFormat(m_testParameters.format), m_testParameters.width, m_testParameters.height)));
	}

	const int	numLevels	= m_textures[0]->getNumLevels();

	// Fill first gradient texture.
	for (int levelNdx = 0; levelNdx < numLevels; ++levelNdx)
	{
		tcu::fillWithComponentGradients(m_textures[0]->getLevel(levelNdx, 0), tcu::Vec4(-0.5f, -0.5f, -0.5f, 2.0f), tcu::Vec4(1.0f, 1.0f, 1.0f, 0.0f));
	}

	// Fill second with grid texture.
	for (int levelNdx = 0; levelNdx < numLevels; levelNdx++)
	{
		const deUint32	step	= 0x00ffffff / numLevels;
		const deUint32	rgb		= step*levelNdx;
		const deUint32	colorA	= 0xff000000 | rgb;
		const deUint32	colorB	= 0xff000000 | ~rgb;

		tcu::fillWithGrid(m_textures[1]->getLevel(levelNdx, 0), 4, tcu::RGBA(colorA).toVec(), tcu::RGBA(colorB).toVec());
	}

	// Upload.
	for (std::vector<TestTexture2DSp>::iterator i = m_textures.begin(); i != m_textures.end(); ++i)
	{
		m_renderer.add2DTexture(*i, m_testParameters.backingMode);
	}

	// Compute cases.
	{
		const float refInRangeUpper		= (m_testParameters.compareOp == Sampler::COMPAREMODE_EQUAL || m_testParameters.compareOp == Sampler::COMPAREMODE_NOT_EQUAL) ? 1.0f : 0.5f;
		const float refInRangeLower		= (m_testParameters.compareOp == Sampler::COMPAREMODE_EQUAL || m_testParameters.compareOp == Sampler::COMPAREMODE_NOT_EQUAL) ? 0.0f : 0.5f;
		const float refOutOfBoundsUpper	= 1.1f;		// !< lookup function should clamp values to [0, 1] range
		const float refOutOfBoundsLower	= -0.1f;

		const struct
		{
			const int	texNdx;
			const float	ref;
			const float	lodX;
			const float	lodY;
			const float	oX;
			const float	oY;
		} cases[] =
		{
			{ 0,	refInRangeUpper,		1.6f,	2.9f,	-1.0f,	-2.7f	},
			{ 0,	refInRangeLower,		-2.0f,	-1.35f,	-0.2f,	0.7f	},
			{ 1,	refInRangeUpper,		0.14f,	0.275f,	-1.5f,	-1.1f	},
			{ 1,	refInRangeLower,		-0.92f,	-2.64f,	0.4f,	-0.1f	},
			{ 1,	refOutOfBoundsUpper,	-0.39f,	-0.52f,	0.65f,	0.87f	},
			{ 1,	refOutOfBoundsLower,	-1.55f,	0.65f,	0.35f,	0.91f	},
		};

		for (int caseNdx = 0; caseNdx < DE_LENGTH_OF_ARRAY(cases); caseNdx++)
		{
			const int	texNdx	= de::clamp(cases[caseNdx].texNdx, 0, (int)m_textures.size()-1);
			const float ref		= cases[caseNdx].ref;
			const float	lodX	= cases[caseNdx].lodX;
			const float	lodY	= cases[caseNdx].lodY;
			const float	oX		= cases[caseNdx].oX;
			const float	oY		= cases[caseNdx].oY;
			const float	sX		= deFloatExp2(lodX) * float(m_renderer.getRenderWidth()) / float(m_textures[texNdx]->getTexture().getWidth());
			const float	sY		= deFloatExp2(lodY) * float(m_renderer.getRenderHeight()) / float(m_textures[texNdx]->getTexture().getHeight());

			m_cases.push_back(FilterCase(texNdx, ref, tcu::Vec2(oX, oY), tcu::Vec2(oX+sX, oY+sY)));
		}
	}

	m_caseNdx = 0;
}

Texture2DShadowTestInstance::~Texture2DShadowTestInstance (void)
{
	m_textures.clear();
	m_cases.clear();
}

tcu::TestStatus Texture2DShadowTestInstance::iterate (void)
{
	tcu::TestLog&					log				= m_context.getTestContext().getLog();
	const pipeline::TestTexture2D&	texture			= m_renderer.get2DTexture(m_cases[m_caseNdx].textureIndex);
	const tcu::TextureFormat		texFmt			= texture.getTextureFormat();
	const tcu::TextureFormatInfo	fmtInfo			= tcu::getTextureFormatInfo(texFmt);
	const tcu::ScopedLogSection		section			(log, string("Test") + de::toString(m_caseNdx), string("Test ") + de::toString(m_caseNdx));

	const FilterCase&				curCase			= m_cases[m_caseNdx];
	ReferenceParams					sampleParams	(TEXTURETYPE_2D);
	tcu::Surface					rendered		(m_renderer.getRenderWidth(), m_renderer.getRenderHeight());
	vector<float>					texCoord;

	// Setup params for reference.
	sampleParams.sampler			= util::createSampler(m_testParameters.wrapS, m_testParameters.wrapT, m_testParameters.minFilter, m_testParameters.magFilter);
	sampleParams.sampler.compare	= m_testParameters.compareOp;
	sampleParams.samplerType		= SAMPLERTYPE_SHADOW;
	sampleParams.lodMode			= LODMODE_EXACT;
	sampleParams.colorBias			= fmtInfo.lookupBias;
	sampleParams.colorScale			= fmtInfo.lookupScale;
	sampleParams.ref				= curCase.ref;

	log << TestLog::Message << "Compare reference value = " << sampleParams.ref << TestLog::EndMessage;

	// Compute texture coordinates.
	log << TestLog::Message << "Texture coordinates: " << curCase.minCoord << " -> " << curCase.maxCoord << TestLog::EndMessage;
	computeQuadTexCoord2D(texCoord, curCase.minCoord, curCase.maxCoord);

	m_renderer.renderQuad(rendered, curCase.textureIndex, &texCoord[0], sampleParams);

	{
		const tcu::PixelFormat		pixelFormat			= getPixelFormat(vk::mapVkFormat(VK_FORMAT_R8G8B8A8_UNORM));
		tcu::LodPrecision			lodPrecision;
		tcu::TexComparePrecision	texComparePrecision;

		lodPrecision.derivateBits			= 18;
		lodPrecision.lodBits				= 6;
		texComparePrecision.coordBits		= tcu::IVec3(20,20,0);
		texComparePrecision.uvwBits			= tcu::IVec3(7,7,0);
		texComparePrecision.pcfBits			= 5;
		texComparePrecision.referenceBits	= 16;
		texComparePrecision.resultBits		= pixelFormat.redBits-1;

		const bool isHighQuality = verifyTexCompareResult(m_context.getTestContext(), rendered.getAccess(), texture.getTexture(),
														  &texCoord[0], sampleParams, texComparePrecision, lodPrecision, pixelFormat);

		if (!isHighQuality)
		{
			m_context.getTestContext().getLog() << TestLog::Message << "Warning: Verification assuming high-quality PCF filtering failed." << TestLog::EndMessage;

			lodPrecision.lodBits			= 4;
			texComparePrecision.uvwBits		= tcu::IVec3(4,4,0);
			texComparePrecision.pcfBits		= 0;

			const bool isOk = verifyTexCompareResult(m_context.getTestContext(), rendered.getAccess(), texture.getTexture(),
													 &texCoord[0], sampleParams, texComparePrecision, lodPrecision, pixelFormat);

			if (!isOk)
			{
				m_context.getTestContext().getLog() << TestLog::Message << "ERROR: Verification against low precision requirements failed, failing test case." << TestLog::EndMessage;
				return tcu::TestStatus::fail("Image verification failed");
			}
		}
	}

	m_caseNdx += 1;
	return m_caseNdx < (int)m_cases.size() ? tcu::TestStatus::incomplete() : tcu::TestStatus::pass("Pass");
}

struct TextureCubeShadowTestCaseParameters : public TextureShadowCommonTestCaseParameters, public TextureCubeTestCaseParameters
{
};

class TextureCubeShadowTestInstance : public TestInstance
{
public:
	typedef TextureCubeShadowTestCaseParameters ParameterType;
												TextureCubeShadowTestInstance		(Context& context, const ParameterType& testParameters);
												~TextureCubeShadowTestInstance		(void);

	virtual tcu::TestStatus						iterate								(void);

private:
												TextureCubeShadowTestInstance		(const TextureCubeShadowTestInstance& other);
	TextureCubeShadowTestInstance&				operator=							(const TextureCubeShadowTestInstance& other);

	struct FilterCase
	{
		int						textureIndex;
		tcu::Vec2				bottomLeft;
		tcu::Vec2				topRight;
		float					ref;

		FilterCase (void)
			: textureIndex	(-1)
			, ref			(0.0f)
		{
		}

		FilterCase (const int tex_, const float ref_, const tcu::Vec2& bottomLeft_, const tcu::Vec2& topRight_)
			: textureIndex	(tex_)
			, bottomLeft	(bottomLeft_)
			, topRight		(topRight_)
			, ref			(ref_)
		{
		}
	};

	const ParameterType&		m_testParameters;
	vector<TestTextureCubeSp>	m_textures;
	std::vector<FilterCase>		m_cases;

	TextureRenderer				m_renderer;
	int							m_caseNdx;
};

TextureCubeShadowTestInstance::TextureCubeShadowTestInstance (Context& context, const ParameterType& testParameters)
	: TestInstance			(context)
	, m_testParameters		(testParameters)
	, m_renderer			(context, testParameters.sampleCount, TEXCUBE_VIEWPORT_SIZE, TEXCUBE_VIEWPORT_SIZE)
	, m_caseNdx				(0)
{
	const int						numLevels	= deLog2Floor32(m_testParameters.size)+1;
	const tcu::TextureFormatInfo	fmtInfo		= tcu::getTextureFormatInfo(vk::mapVkFormat(m_testParameters.format));
	const tcu::Vec4					cBias		= fmtInfo.valueMin;
	const tcu::Vec4					cScale		= fmtInfo.valueMax-fmtInfo.valueMin;

	// Create textures.

	m_textures.reserve(2);
	for (int ndx = 0; ndx < 2; ndx++)
	{
		m_textures.push_back(TestTextureCubeSp(new pipeline::TestTextureCube(vk::mapVkFormat(m_testParameters.format), m_testParameters.size)));
	}

	// Fill first with gradient texture.
	static const tcu::Vec4 gradients[tcu::CUBEFACE_LAST][2] =
	{
		{ tcu::Vec4(-1.0f, -1.0f, -1.0f, 2.0f), tcu::Vec4(1.0f, 1.0f, 1.0f, 0.0f) }, // negative x
		{ tcu::Vec4( 0.0f, -1.0f, -1.0f, 2.0f), tcu::Vec4(1.0f, 1.0f, 1.0f, 0.0f) }, // positive x
		{ tcu::Vec4(-1.0f,  0.0f, -1.0f, 2.0f), tcu::Vec4(1.0f, 1.0f, 1.0f, 0.0f) }, // negative y
		{ tcu::Vec4(-1.0f, -1.0f,  0.0f, 2.0f), tcu::Vec4(1.0f, 1.0f, 1.0f, 0.0f) }, // positive y
		{ tcu::Vec4(-1.0f, -1.0f, -1.0f, 0.0f), tcu::Vec4(1.0f, 1.0f, 1.0f, 1.0f) }, // negative z
		{ tcu::Vec4( 0.0f,  0.0f,  0.0f, 2.0f), tcu::Vec4(1.0f, 1.0f, 1.0f, 0.0f) }  // positive z
	};
	for (int face = 0; face < tcu::CUBEFACE_LAST; face++)
	{
		for (int levelNdx = 0; levelNdx < numLevels; levelNdx++)
		{
			tcu::fillWithComponentGradients(m_textures[0]->getLevel(levelNdx, face), gradients[face][0]*cScale + cBias, gradients[face][1]*cScale + cBias);
		}
	}

	// Fill second with grid texture.
	for (int face = 0; face < tcu::CUBEFACE_LAST; face++)
	{
		for (int levelNdx = 0; levelNdx < numLevels; levelNdx++)
		{
			const deUint32	step	= 0x00ffffff / (numLevels*tcu::CUBEFACE_LAST);
			const deUint32	rgb		= step*levelNdx*face;
			const deUint32	colorA	= 0xff000000 | rgb;
			const deUint32	colorB	= 0xff000000 | ~rgb;

			tcu::fillWithGrid(m_textures[1]->getLevel(levelNdx, face), 4, tcu::RGBA(colorA).toVec()*cScale + cBias, tcu::RGBA(colorB).toVec()*cScale + cBias);
		}
	}

	// Upload.
	for (vector<TestTextureCubeSp>::iterator i = m_textures.begin(); i != m_textures.end(); i++)
	{
		m_renderer.addCubeTexture(*i, m_testParameters.backingMode);
	}

	// Compute cases
	{
		const float refInRangeUpper		= (m_testParameters.compareOp == Sampler::COMPAREMODE_EQUAL || m_testParameters.compareOp == Sampler::COMPAREMODE_NOT_EQUAL) ? 1.0f : 0.5f;
		const float refInRangeLower		= (m_testParameters.compareOp == Sampler::COMPAREMODE_EQUAL || m_testParameters.compareOp == Sampler::COMPAREMODE_NOT_EQUAL) ? 0.0f : 0.5f;
		const float refOutOfBoundsUpper	= 1.1f;
		const float refOutOfBoundsLower	= -0.1f;

		m_cases.push_back(FilterCase(0,	refInRangeUpper,		tcu::Vec2(-1.25f, -1.2f),	tcu::Vec2(1.2f, 1.25f)));	// minification
		m_cases.push_back(FilterCase(0,	refInRangeLower,		tcu::Vec2(0.8f, 0.8f),		tcu::Vec2(1.25f, 1.20f)));	// magnification
		m_cases.push_back(FilterCase(1,	refInRangeUpper,		tcu::Vec2(-1.19f, -1.3f),	tcu::Vec2(1.1f, 1.35f)));	// minification
		m_cases.push_back(FilterCase(1,	refInRangeLower,		tcu::Vec2(-1.2f, -1.1f),	tcu::Vec2(-0.8f, -0.8f)));	// magnification
		m_cases.push_back(FilterCase(1,	refOutOfBoundsUpper,	tcu::Vec2(-0.61f, -0.1f),	tcu::Vec2(0.9f, 1.18f)));	// reference value clamp, upper
		m_cases.push_back(FilterCase(1,	refOutOfBoundsLower,	tcu::Vec2(-0.75f, 1.0f),	tcu::Vec2(0.05f, 0.75f)));	// reference value clamp, lower
	}
}

TextureCubeShadowTestInstance::~TextureCubeShadowTestInstance	(void)
{
}

static const char* getFaceDesc (const tcu::CubeFace face)
{
	switch (face)
	{
		case tcu::CUBEFACE_NEGATIVE_X:	return "-X";
		case tcu::CUBEFACE_POSITIVE_X:	return "+X";
		case tcu::CUBEFACE_NEGATIVE_Y:	return "-Y";
		case tcu::CUBEFACE_POSITIVE_Y:	return "+Y";
		case tcu::CUBEFACE_NEGATIVE_Z:	return "-Z";
		case tcu::CUBEFACE_POSITIVE_Z:	return "+Z";
		default:
			DE_ASSERT(false);
			return DE_NULL;
	}
}

tcu::TestStatus TextureCubeShadowTestInstance::iterate (void)
{

	tcu::TestLog&						log				= m_context.getTestContext().getLog();
	const tcu::ScopedLogSection			iterSection		(log, string("Test") + de::toString(m_caseNdx), string("Test ") + de::toString(m_caseNdx));
	const FilterCase&					curCase			= m_cases[m_caseNdx];
	const pipeline::TestTextureCube&	texture			= m_renderer.getCubeTexture(curCase.textureIndex);

	ReferenceParams						sampleParams	(TEXTURETYPE_CUBE);

	// Params for reference computation.
	sampleParams.sampler					= util::createSampler(Sampler::CLAMP_TO_EDGE, Sampler::CLAMP_TO_EDGE, m_testParameters.minFilter, m_testParameters.magFilter);
	sampleParams.sampler.seamlessCubeMap	= true;
	sampleParams.sampler.compare			= m_testParameters.compareOp;
	sampleParams.samplerType				= SAMPLERTYPE_SHADOW;
	sampleParams.lodMode					= LODMODE_EXACT;
	sampleParams.ref						= curCase.ref;

	log	<< TestLog::Message
		<< "Compare reference value = " << sampleParams.ref << "\n"
		<< "Coordinates: " << curCase.bottomLeft << " -> " << curCase.topRight
		<< TestLog::EndMessage;

	for (int faceNdx = 0; faceNdx < tcu::CUBEFACE_LAST; faceNdx++)
	{
		const tcu::CubeFace		face		= tcu::CubeFace(faceNdx);
		tcu::Surface			result		(m_renderer.getRenderWidth(), m_renderer.getRenderHeight());
		vector<float>			texCoord;

		computeQuadTexCoordCube(texCoord, face, curCase.bottomLeft, curCase.topRight);

		log << TestLog::Message << "Face " << getFaceDesc(face) << TestLog::EndMessage;

		// \todo Log texture coordinates.

		m_renderer.renderQuad(result, curCase.textureIndex, &texCoord[0], sampleParams);

		{
			const tcu::PixelFormat		pixelFormat			= getPixelFormat(vk::mapVkFormat(VK_FORMAT_R8G8B8A8_UNORM));
			tcu::LodPrecision			lodPrecision;
			tcu::TexComparePrecision	texComparePrecision;

			lodPrecision.derivateBits			= 10;
			lodPrecision.lodBits				= 5;
			texComparePrecision.coordBits		= tcu::IVec3(10,10,10);
			texComparePrecision.uvwBits			= tcu::IVec3(6,6,0);
			texComparePrecision.pcfBits			= 5;
			texComparePrecision.referenceBits	= 16;
			texComparePrecision.resultBits		= pixelFormat.redBits-1;

			const bool isHighQuality = verifyTexCompareResult(m_context.getTestContext(), result.getAccess(), texture.getTexture(),
															  &texCoord[0], sampleParams, texComparePrecision, lodPrecision, pixelFormat);

			if (!isHighQuality)
			{
				log << TestLog::Message << "Warning: Verification assuming high-quality PCF filtering failed." << TestLog::EndMessage;

				lodPrecision.lodBits			= 4;
				texComparePrecision.uvwBits		= tcu::IVec3(4,4,0);
				texComparePrecision.pcfBits		= 0;

				const bool isOk = verifyTexCompareResult(m_context.getTestContext(), result.getAccess(), texture.getTexture(),
														 &texCoord[0], sampleParams, texComparePrecision, lodPrecision, pixelFormat);

				if (!isOk)
				{
					log << TestLog::Message << "ERROR: Verification against low precision requirements failed, failing test case." << TestLog::EndMessage;
					return tcu::TestStatus::fail("Image verification failed");
				}
			}
		}
	}

	m_caseNdx += 1;
	return m_caseNdx < (int)m_cases.size() ? tcu::TestStatus::incomplete() : tcu::TestStatus::pass("Pass");
}

struct Texture2DArrayShadowTestCaseParameters : public TextureShadowCommonTestCaseParameters, public Texture2DArrayTestCaseParameters
{
};

class Texture2DArrayShadowTestInstance : public TestInstance
{
public:
	typedef Texture2DArrayShadowTestCaseParameters	ParameterType;
													Texture2DArrayShadowTestInstance		(Context& context, const ParameterType& testParameters);
													~Texture2DArrayShadowTestInstance		(void);

	virtual tcu::TestStatus							iterate									(void);

private:
													Texture2DArrayShadowTestInstance		(const Texture2DArrayShadowTestInstance& other);
	Texture2DArrayShadowTestInstance&				operator=								(const Texture2DArrayShadowTestInstance& other);

	struct FilterCase
	{
		int							textureIndex;
		tcu::Vec3					minCoord;
		tcu::Vec3					maxCoord;
		float						ref;

		FilterCase (void)
			: textureIndex	(-1)
			, ref			(0.0f)
		{
		}

		FilterCase (const int tex_, float ref_, const tcu::Vec3& minCoord_, const tcu::Vec3& maxCoord_)
			: textureIndex	(tex_)
			, minCoord		(minCoord_)
			, maxCoord		(maxCoord_)
			, ref			(ref_)
		{
		}
	};

	const ParameterType&				m_testParameters;
	std::vector<TestTexture2DArraySp>	m_textures;
	std::vector<FilterCase>				m_cases;

	TextureRenderer						m_renderer;

	int									m_caseNdx;
};

Texture2DArrayShadowTestInstance::Texture2DArrayShadowTestInstance (Context& context, const ParameterType& testParameters)
	: TestInstance			(context)
	, m_testParameters		(testParameters)
	, m_renderer			(context, testParameters.sampleCount, TEX2D_VIEWPORT_WIDTH, TEX2D_VIEWPORT_HEIGHT)
	, m_caseNdx				(0)
{
	const int						numLevels	= deLog2Floor32(de::max(m_testParameters.width, m_testParameters.height))+1;
	const tcu::TextureFormatInfo	fmtInfo		= tcu::getTextureFormatInfo(vk::mapVkFormat(m_testParameters.format));
	const tcu::Vec4					cScale		= fmtInfo.valueMax-fmtInfo.valueMin;
	const tcu::Vec4					cBias		= fmtInfo.valueMin;

	// Create 2 textures.
	m_textures.reserve(2);
	for (int ndx = 0; ndx < 2; ndx++)
	{
		m_textures.push_back(TestTexture2DArraySp(new pipeline::TestTexture2DArray(vk::mapVkFormat(m_testParameters.format), m_testParameters.width, m_testParameters.height, m_testParameters.numLayers)));
	}

	// Fill first gradient texture.
	for (int levelNdx = 0; levelNdx < numLevels; levelNdx++)
	{
		const tcu::Vec4 gMin = tcu::Vec4(-0.5f, -0.5f, -0.5f, 2.0f)*cScale + cBias;
		const tcu::Vec4 gMax = tcu::Vec4( 1.0f,  1.0f,  1.0f, 0.0f)*cScale + cBias;

		tcu::fillWithComponentGradients(m_textures[0]->getTexture().getLevel(levelNdx), gMin, gMax);
	}

	// Fill second with grid texture.
	for (int levelNdx = 0; levelNdx < numLevels; levelNdx++)
	{
		const deUint32	step	= 0x00ffffff / numLevels;
		const deUint32	rgb		= step*levelNdx;
		const deUint32	colorA	= 0xff000000 | rgb;
		const deUint32	colorB	= 0xff000000 | ~rgb;

		tcu::fillWithGrid(m_textures[1]->getTexture().getLevel(levelNdx), 4, tcu::RGBA(colorA).toVec()*cScale + cBias, tcu::RGBA(colorB).toVec()*cScale + cBias);
	}

	// Upload.
	for (std::vector<TestTexture2DArraySp>::iterator i = m_textures.begin(); i != m_textures.end(); ++i)
	{
		m_renderer.add2DArrayTexture(*i, m_testParameters.backingMode);
	}

	// Compute cases.
	{
		const float refInRangeUpper		= (m_testParameters.compareOp == Sampler::COMPAREMODE_EQUAL || m_testParameters.compareOp == Sampler::COMPAREMODE_NOT_EQUAL) ? 1.0f : 0.5f;
		const float refInRangeLower		= (m_testParameters.compareOp == Sampler::COMPAREMODE_EQUAL || m_testParameters.compareOp == Sampler::COMPAREMODE_NOT_EQUAL) ? 0.0f : 0.5f;
		const float refOutOfBoundsUpper	= 1.1f;		// !< lookup function should clamp values to [0, 1] range
		const float refOutOfBoundsLower	= -0.1f;

		const struct
		{
			const int	texNdx;
			const float	ref;
			const float	lodX;
			const float	lodY;
			const float	oX;
			const float	oY;
		} cases[] =
		{
			{ 0,	refInRangeUpper,		1.6f,	2.9f,	-1.0f,	-2.7f	},
			{ 0,	refInRangeLower,		-2.0f,	-1.35f,	-0.2f,	0.7f	},
			{ 1,	refInRangeUpper,		0.14f,	0.275f,	-1.5f,	-1.1f	},
			{ 1,	refInRangeLower,		-0.92f,	-2.64f,	0.4f,	-0.1f	},
			{ 1,	refOutOfBoundsUpper,	-0.49f,	-0.22f,	0.45f,	0.97f	},
			{ 1,	refOutOfBoundsLower,	-0.85f,	0.75f,	0.25f,	0.61f	},
		};

		const float	minLayer	= -0.5f;
		const float	maxLayer	= (float)m_testParameters.numLayers;

		for (int caseNdx = 0; caseNdx < DE_LENGTH_OF_ARRAY(cases); caseNdx++)
		{
			const int	tex		= cases[caseNdx].texNdx > 0 ? 1 : 0;
			const float	ref		= cases[caseNdx].ref;
			const float	lodX	= cases[caseNdx].lodX;
			const float	lodY	= cases[caseNdx].lodY;
			const float	oX		= cases[caseNdx].oX;
			const float	oY		= cases[caseNdx].oY;
			const float	sX		= deFloatExp2(lodX) * float(m_renderer.getRenderWidth()) / float(m_textures[tex]->getTexture().getWidth());
			const float	sY		= deFloatExp2(lodY) * float(m_renderer.getRenderHeight()) / float(m_textures[tex]->getTexture().getHeight());

			m_cases.push_back(FilterCase(tex, ref, tcu::Vec3(oX, oY, minLayer), tcu::Vec3(oX+sX, oY+sY, maxLayer)));
		}
	}
}

Texture2DArrayShadowTestInstance::~Texture2DArrayShadowTestInstance (void)
{
}

tcu::TestStatus Texture2DArrayShadowTestInstance::iterate (void)
{
	tcu::TestLog&						log				= m_context.getTestContext().getLog();
	const FilterCase&					curCase			= m_cases[m_caseNdx];
	const pipeline::TestTexture2DArray&	texture			= m_renderer.get2DArrayTexture(curCase.textureIndex);

	ReferenceParams						sampleParams	(TEXTURETYPE_2D_ARRAY);
	tcu::Surface						rendered		(m_renderer.getRenderWidth(), m_renderer.getRenderHeight());
	const tcu::ScopedLogSection			section			(log, string("Test") + de::toString(m_caseNdx), string("Test ") + de::toString(m_caseNdx));

	const float							texCoord[]		=
	{
		curCase.minCoord.x(), curCase.minCoord.y(), curCase.minCoord.z(),
		curCase.minCoord.x(), curCase.maxCoord.y(), (curCase.minCoord.z() + curCase.maxCoord.z()) / 2.0f,
		curCase.maxCoord.x(), curCase.minCoord.y(), (curCase.minCoord.z() + curCase.maxCoord.z()) / 2.0f,
		curCase.maxCoord.x(), curCase.maxCoord.y(), curCase.maxCoord.z()
	};

	// Setup params for reference.
	sampleParams.sampler			= util::createSampler(m_testParameters.wrapS, m_testParameters.wrapT, m_testParameters.minFilter, m_testParameters.magFilter);
	sampleParams.sampler.compare	= m_testParameters.compareOp;
	sampleParams.samplerType		= SAMPLERTYPE_SHADOW;
	sampleParams.lodMode			= LODMODE_EXACT;
	sampleParams.ref				= curCase.ref;

	log	<< TestLog::Message
		<< "Compare reference value = " << sampleParams.ref << "\n"
		<< "Texture coordinates: " << curCase.minCoord << " -> " << curCase.maxCoord
		<< TestLog::EndMessage;

	m_renderer.renderQuad(rendered, curCase.textureIndex, &texCoord[0], sampleParams);

	{
		const tcu::PixelFormat		pixelFormat			= getPixelFormat(vk::mapVkFormat(VK_FORMAT_R8G8B8A8_UNORM));
		tcu::LodPrecision			lodPrecision;
		tcu::TexComparePrecision	texComparePrecision;

		lodPrecision.derivateBits			= 18;
		lodPrecision.lodBits				= 6;
		texComparePrecision.coordBits		= tcu::IVec3(20,20,20);
		texComparePrecision.uvwBits			= tcu::IVec3(7,7,7);
		texComparePrecision.pcfBits			= 5;
		texComparePrecision.referenceBits	= 16;
		texComparePrecision.resultBits		= pixelFormat.redBits-1;

		const bool isHighQuality = verifyTexCompareResult(m_context.getTestContext(), rendered.getAccess(), texture.getTexture(),
														  &texCoord[0], sampleParams, texComparePrecision, lodPrecision, pixelFormat);

		if (!isHighQuality)
		{
			log << TestLog::Message << "Warning: Verification assuming high-quality PCF filtering failed." << TestLog::EndMessage;

			lodPrecision.lodBits			= 4;
			texComparePrecision.uvwBits		= tcu::IVec3(4,4,4);
			texComparePrecision.pcfBits		= 0;

			const bool isOk = verifyTexCompareResult(m_context.getTestContext(), rendered.getAccess(), texture.getTexture(),
													 &texCoord[0], sampleParams, texComparePrecision, lodPrecision, pixelFormat);

			if (!isOk)
			{
				log << TestLog::Message << "ERROR: Verification against low precision requirements failed, failing test case." << TestLog::EndMessage;
				return tcu::TestStatus::fail("Image verification failed");
			}
		}
	}

	m_caseNdx += 1;
	return m_caseNdx < (int)m_cases.size() ? tcu::TestStatus::incomplete() : tcu::TestStatus::pass("Pass");
}

void populateTextureShadowTests (tcu::TestCaseGroup* textureShadowTests)
{
	tcu::TestContext&				testCtx				= textureShadowTests->getTestContext();

	static const struct
	{
		const char*								name;
		const TextureBinding::ImageBackingMode	backingMode;
	} backingModes[] =
	{
		{ "",			TextureBinding::IMAGE_BACKING_MODE_REGULAR	},
		{ "sparse_",	TextureBinding::IMAGE_BACKING_MODE_SPARSE	}
	};

	static const struct
	{
		const char*								name;
		const VkFormat							format;
	} formats[] =
	{
		{ "d16_unorm",				VK_FORMAT_D16_UNORM				},
		{ "x8_d24_unorm_pack32",	VK_FORMAT_X8_D24_UNORM_PACK32	},
		{ "d32_sfloat",				VK_FORMAT_D32_SFLOAT			},
		{ "d16_unorm_s8_uint",		VK_FORMAT_D16_UNORM_S8_UINT		},
		{ "d24_unorm_s8_uint",		VK_FORMAT_D24_UNORM_S8_UINT		},
		{ "d32_sfloat_s8_uint",		VK_FORMAT_D32_SFLOAT_S8_UINT	}
	};

	static const struct
	{
		const char*								name;
		const Sampler::FilterMode				minFilter;
		const Sampler::FilterMode				magFilter;
	} filters[] =
	{
		{ "nearest",				Sampler::NEAREST,					Sampler::NEAREST	},
		{ "linear",					Sampler::LINEAR,					Sampler::LINEAR		},
		{ "nearest_mipmap_nearest",	Sampler::NEAREST_MIPMAP_NEAREST,	Sampler::LINEAR		},
		{ "linear_mipmap_nearest",	Sampler::LINEAR_MIPMAP_NEAREST,		Sampler::LINEAR		},
		{ "nearest_mipmap_linear",	Sampler::NEAREST_MIPMAP_LINEAR,		Sampler::LINEAR		},
		{ "linear_mipmap_linear",	Sampler::LINEAR_MIPMAP_LINEAR,		Sampler::LINEAR		}
	};

	static const struct
	{
		const char*								name;
		const Sampler::CompareMode				op;
	} compareOp[] =
	{
		{ "less_or_equal",		Sampler::COMPAREMODE_LESS_OR_EQUAL		},
		{ "greater_or_equal",	Sampler::COMPAREMODE_GREATER_OR_EQUAL	},
		{ "less",				Sampler::COMPAREMODE_LESS				},
		{ "greater",			Sampler::COMPAREMODE_GREATER			},
		{ "equal",				Sampler::COMPAREMODE_EQUAL				},
		{ "not_equal",			Sampler::COMPAREMODE_NOT_EQUAL			},
		{ "always",				Sampler::COMPAREMODE_ALWAYS				},
		{ "never",				Sampler::COMPAREMODE_NEVER				}
	};

	// 2D cases.
	{
		de::MovePtr<tcu::TestCaseGroup>	group2D	(new tcu::TestCaseGroup(testCtx, "2d", "2D texture shadow lookup tests"));

		for (int filterNdx = 0; filterNdx < DE_LENGTH_OF_ARRAY(filters); filterNdx++)
		{
			de::MovePtr<tcu::TestCaseGroup>	filterGroup	(new tcu::TestCaseGroup(testCtx, filters[filterNdx].name, ""));

			for (int compareNdx = 0; compareNdx < DE_LENGTH_OF_ARRAY(compareOp); compareNdx++)
			{
				for (int formatNdx = 0; formatNdx < DE_LENGTH_OF_ARRAY(formats); formatNdx++)
				{
					for (int backingNdx = 0; backingNdx < DE_LENGTH_OF_ARRAY(backingModes); backingNdx++)
					{
						const string						name	= string(backingModes[backingNdx].name) + compareOp[compareNdx].name + "_" + formats[formatNdx].name;
						Texture2DShadowTestCaseParameters	testParameters;

						testParameters.minFilter	= filters[filterNdx].minFilter;
						testParameters.magFilter	= filters[filterNdx].magFilter;
						testParameters.format		= formats[formatNdx].format;
						testParameters.backingMode	= backingModes[backingNdx].backingMode;
						testParameters.compareOp	= compareOp[compareNdx].op;
						testParameters.wrapS		= Sampler::REPEAT_GL;
						testParameters.wrapT		= Sampler::REPEAT_GL;
						testParameters.width		= 32;
						testParameters.height		= 64;

						testParameters.programs.push_back(PROGRAM_2D_SHADOW);

						filterGroup->addChild(new TextureTestCase<Texture2DShadowTestInstance>(testCtx, name.c_str(), "", testParameters));
					}
				}
			}

			group2D->addChild(filterGroup.release());
		}

		textureShadowTests->addChild(group2D.release());
	}

	// Cubemap cases.
	{
		de::MovePtr<tcu::TestCaseGroup>	groupCube	(new tcu::TestCaseGroup(testCtx, "cube", "Cube map texture shadow lookup tests"));

		for (int filterNdx = 0; filterNdx < DE_LENGTH_OF_ARRAY(filters); filterNdx++)
		{
			de::MovePtr<tcu::TestCaseGroup>	filterGroup	(new tcu::TestCaseGroup(testCtx, filters[filterNdx].name, ""));

			for (int compareNdx = 0; compareNdx < DE_LENGTH_OF_ARRAY(compareOp); compareNdx++)
			{
				for (int formatNdx = 0; formatNdx < DE_LENGTH_OF_ARRAY(formats); formatNdx++)
				{
					for (int backingNdx = 0; backingNdx < DE_LENGTH_OF_ARRAY(backingModes); backingNdx++)
					{
						const string							name	= string(backingModes[backingNdx].name) + compareOp[compareNdx].name + "_" + formats[formatNdx].name;
						TextureCubeShadowTestCaseParameters		testParameters;

						testParameters.minFilter	= filters[filterNdx].minFilter;
						testParameters.magFilter	= filters[filterNdx].magFilter;
						testParameters.format		= formats[formatNdx].format;
						testParameters.backingMode	= backingModes[backingNdx].backingMode;
						testParameters.compareOp	= compareOp[compareNdx].op;
						testParameters.wrapS		= Sampler::REPEAT_GL;
						testParameters.wrapT		= Sampler::REPEAT_GL;
						testParameters.size			= 32;

						testParameters.programs.push_back(PROGRAM_CUBE_SHADOW);

						filterGroup->addChild(new TextureTestCase<TextureCubeShadowTestInstance>(testCtx, name.c_str(), "", testParameters));
					}
				}
			}

			groupCube->addChild(filterGroup.release());
		}

		textureShadowTests->addChild(groupCube.release());
	}

	// 2D array cases.
	{
		de::MovePtr<tcu::TestCaseGroup>	group2DArray	(new tcu::TestCaseGroup(testCtx, "2d_array", "2D texture array shadow lookup tests"));

		for (int filterNdx = 0; filterNdx < DE_LENGTH_OF_ARRAY(filters); filterNdx++)
		{
			de::MovePtr<tcu::TestCaseGroup>	filterGroup	(new tcu::TestCaseGroup(testCtx, filters[filterNdx].name, ""));

			for (int compareNdx = 0; compareNdx < DE_LENGTH_OF_ARRAY(compareOp); compareNdx++)
			{
				for (int formatNdx = 0; formatNdx < DE_LENGTH_OF_ARRAY(formats); formatNdx++)
				{
					for (int backingNdx = 0; backingNdx < DE_LENGTH_OF_ARRAY(backingModes); backingNdx++)
					{
						const string							name	= string(backingModes[backingNdx].name) + compareOp[compareNdx].name + "_" + formats[formatNdx].name;
						Texture2DArrayShadowTestCaseParameters	testParameters;

						testParameters.minFilter	= filters[filterNdx].minFilter;
						testParameters.magFilter	= filters[filterNdx].magFilter;
						testParameters.format		= formats[formatNdx].format;
						testParameters.backingMode	= backingModes[backingNdx].backingMode;
						testParameters.compareOp	= compareOp[compareNdx].op;
						testParameters.wrapS		= Sampler::REPEAT_GL;
						testParameters.wrapT		= Sampler::REPEAT_GL;
						testParameters.width		= 32;
						testParameters.height		= 64;
						testParameters.numLayers	= 8;

						testParameters.programs.push_back(PROGRAM_2D_ARRAY_SHADOW);

						filterGroup->addChild(new TextureTestCase<Texture2DArrayShadowTestInstance>(testCtx, name.c_str(), "", testParameters));
					}
				}
			}

			group2DArray->addChild(filterGroup.release());
		}

		textureShadowTests->addChild(group2DArray.release());
	}
}

} // anonymous

tcu::TestCaseGroup* createTextureShadowTests (tcu::TestContext& testCtx)
{
	return createTestGroup(testCtx, "shadow", "Texture shadow tests.", populateTextureShadowTests);
}

} // texture
} // vkt
