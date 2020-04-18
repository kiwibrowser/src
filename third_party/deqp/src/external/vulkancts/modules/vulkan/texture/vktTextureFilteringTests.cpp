/*------------------------------------------------------------------------
 * Vulkan Conformance Tests
 * ------------------------
 *
 * Copyright (c) 2016 The Khronos Group Inc.
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 * Copyright (c) 2014 The Android Open Source Project
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
 * \brief Texture filtering tests.
 *//*--------------------------------------------------------------------*/

#include "tcuVectorUtil.hpp"
#include "tcuTexVerifierUtil.hpp"
#include "vkImageUtil.hpp"
#include "vkMemUtil.hpp"
#include "vkPrograms.hpp"
#include "vkQueryUtil.hpp"
#include "vkRefUtil.hpp"
#include "vktTestCase.hpp"
#include "vktTestCaseUtil.hpp"
#include "vktTestGroupUtil.hpp"
#include "vktTextureFilteringTests.hpp"
#include "vktTextureTestUtil.hpp"
#include <string>
#include <vector>

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
	TEXCUBE_VIEWPORT_SIZE		= 28,

	TEX2D_VIEWPORT_WIDTH		= 64,
	TEX2D_VIEWPORT_HEIGHT		= 64,

	TEX3D_VIEWPORT_WIDTH		= 64,
	TEX3D_VIEWPORT_HEIGHT		= 64,
};

class Texture2DFilteringTestInstance : public TestInstance
{
public:
	typedef Texture2DTestCaseParameters	ParameterType;

										Texture2DFilteringTestInstance		(Context& context, const ParameterType& testParameters);
										~Texture2DFilteringTestInstance		(void);

	virtual tcu::TestStatus				iterate								(void);
private:
										Texture2DFilteringTestInstance		(const Texture2DFilteringTestInstance& other);
	Texture2DFilteringTestInstance&		operator=							(const Texture2DFilteringTestInstance& other);

	struct FilterCase
	{
		int						textureIndex;

		tcu::Vec2				minCoord;
		tcu::Vec2				maxCoord;

		FilterCase (void)
			: textureIndex(-1)
		{
		}

		FilterCase (int tex_, const tcu::Vec2& minCoord_, const tcu::Vec2& maxCoord_)
			: textureIndex	(tex_)
			, minCoord		(minCoord_)
			, maxCoord		(maxCoord_)
		{
		}
	};

	const ParameterType			m_testParameters;
	vector<TestTexture2DSp>		m_textures;
	vector<FilterCase>			m_cases;
	TextureRenderer				m_renderer;
	int							m_caseNdx;
};

Texture2DFilteringTestInstance::Texture2DFilteringTestInstance (Context& context, const ParameterType& testParameters)
	: TestInstance			(context)
	, m_testParameters		(testParameters)
	, m_renderer			(context, testParameters.sampleCount, TEX2D_VIEWPORT_WIDTH, TEX2D_VIEWPORT_HEIGHT)
	, m_caseNdx				(0)
{
	const bool						mipmaps		= true;
	const int						numLevels	= mipmaps ? deLog2Floor32(de::max(m_testParameters.width, m_testParameters.height))+1 : 1;
	const tcu::TextureFormatInfo	fmtInfo		= tcu::getTextureFormatInfo(vk::mapVkFormat(m_testParameters.format));
	const tcu::Vec4					cBias		= fmtInfo.valueMin;
	const tcu::Vec4					cScale		= fmtInfo.valueMax-fmtInfo.valueMin;

	if ((testParameters.wrapS == Sampler::MIRRORED_ONCE ||
		testParameters.wrapT == Sampler::MIRRORED_ONCE) &&
		!de::contains(context.getDeviceExtensions().begin(), context.getDeviceExtensions().end(), "VK_KHR_sampler_mirror_clamp_to_edge"))
		TCU_THROW(NotSupportedError, "VK_KHR_sampler_mirror_clamp_to_edge not supported");

	// Create 2 textures.
	m_textures.reserve(2);
	for (int ndx = 0; ndx < 2; ndx++)
		m_textures.push_back(TestTexture2DSp(new pipeline::TestTexture2D(vk::mapVkFormat(m_testParameters.format), m_testParameters.width, m_testParameters.height)));

	// Fill first gradient texture.
	for (int levelNdx = 0; levelNdx < numLevels; levelNdx++)
	{
		const tcu::Vec4 gMin = tcu::Vec4(0.0f, 0.0f, 0.0f, 1.0f)*cScale + cBias;
		const tcu::Vec4 gMax = tcu::Vec4(1.0f, 1.0f, 1.0f, 0.0f)*cScale + cBias;

		tcu::fillWithComponentGradients(m_textures[0]->getLevel(levelNdx, 0), gMin, gMax);
	}

	// Fill second with grid texture.
	for (int levelNdx = 0; levelNdx < numLevels; levelNdx++)
	{
		const deUint32	step	= 0x00ffffff / numLevels;
		const deUint32	rgb		= step*levelNdx;
		const deUint32	colorA	= 0xff000000 | rgb;
		const deUint32	colorB	= 0xff000000 | ~rgb;

		tcu::fillWithGrid(m_textures[1]->getLevel(levelNdx, 0), 4, tcu::RGBA(colorA).toVec()*cScale + cBias, tcu::RGBA(colorB).toVec()*cScale + cBias);
	}

	// Upload.
	for (vector<TestTexture2DSp>::iterator i = m_textures.begin(); i != m_textures.end(); i++)
	{
		m_renderer.add2DTexture(*i);
	}

	// Compute cases.
	{
		const struct
		{
			const int		texNdx;
			const float		lodX;
			const float		lodY;
			const float		oX;
			const float		oY;
		} cases[] =
		{
			{ 0,	1.6f,	2.9f,	-1.0f,	-2.7f	},
			{ 0,	-2.0f,	-1.35f,	-0.2f,	0.7f	},
			{ 1,	0.14f,	0.275f,	-1.5f,	-1.1f	},
			{ 1,	-0.92f,	-2.64f,	0.4f,	-0.1f	},
		};

		for (int caseNdx = 0; caseNdx < DE_LENGTH_OF_ARRAY(cases); caseNdx++)
		{
			const int	texNdx	= de::clamp(cases[caseNdx].texNdx, 0, (int)m_textures.size()-1);
			const float	lodX	= cases[caseNdx].lodX;
			const float	lodY	= cases[caseNdx].lodY;
			const float	oX		= cases[caseNdx].oX;
			const float	oY		= cases[caseNdx].oY;
			const float	sX		= deFloatExp2(lodX) * float(m_renderer.getRenderWidth()) / float(m_textures[texNdx]->getTexture().getWidth());
			const float	sY		= deFloatExp2(lodY) * float(m_renderer.getRenderHeight()) / float(m_textures[texNdx]->getTexture().getHeight());

			m_cases.push_back(FilterCase(texNdx, tcu::Vec2(oX, oY), tcu::Vec2(oX+sX, oY+sY)));
		}
	}
}

Texture2DFilteringTestInstance::~Texture2DFilteringTestInstance (void)
{
}

tcu::TestStatus Texture2DFilteringTestInstance::iterate (void)
{
	tcu::TestLog&					log			= m_context.getTestContext().getLog();

	const pipeline::TestTexture2D&	texture		= m_renderer.get2DTexture(m_cases[m_caseNdx].textureIndex);
	const tcu::TextureFormat		texFmt		= texture.getTextureFormat();
	const tcu::TextureFormatInfo	fmtInfo		= tcu::getTextureFormatInfo(texFmt);
	const FilterCase&				curCase		= m_cases[m_caseNdx];
	ReferenceParams					refParams	(TEXTURETYPE_2D);
	tcu::Surface					rendered	(m_renderer.getRenderWidth(), m_renderer.getRenderHeight());
	vector<float>					texCoord;

	// Setup params for reference.

	refParams.sampler		= util::createSampler(m_testParameters.wrapS, m_testParameters.wrapT, m_testParameters.minFilter, m_testParameters.magFilter);
	refParams.samplerType	= getSamplerType(texFmt);
	refParams.lodMode		= LODMODE_EXACT;
	refParams.colorBias		= fmtInfo.lookupBias;
	refParams.colorScale	= fmtInfo.lookupScale;

	// Compute texture coordinates.
	log << TestLog::Message << "Texture coordinates: " << curCase.minCoord << " -> " << curCase.maxCoord << TestLog::EndMessage;
	computeQuadTexCoord2D(texCoord, curCase.minCoord, curCase.maxCoord);

	m_renderer.renderQuad(rendered, curCase.textureIndex, &texCoord[0], refParams);

	{
		const bool				isNearestOnly	= m_testParameters.minFilter == Sampler::NEAREST && m_testParameters.magFilter == Sampler::NEAREST;
		const tcu::IVec4		formatBitDepth	= getTextureFormatBitDepth(vk::mapVkFormat(VK_FORMAT_R8G8B8A8_UNORM));
		const tcu::PixelFormat	pixelFormat		(formatBitDepth[0], formatBitDepth[1], formatBitDepth[2], formatBitDepth[3]);
		const tcu::IVec4		colorBits		= max(getBitsVec(pixelFormat) - (isNearestOnly ? 1 : 2), tcu::IVec4(0)); // 1 inaccurate bit if nearest only, 2 otherwise
		tcu::LodPrecision		lodPrecision;
		tcu::LookupPrecision	lookupPrecision;

		lodPrecision.derivateBits		= 18;
		lodPrecision.lodBits			= 6;
		lookupPrecision.colorThreshold	= tcu::computeFixedPointThreshold(colorBits) / refParams.colorScale;
		lookupPrecision.coordBits		= tcu::IVec3(20,20,0);
		lookupPrecision.uvwBits			= tcu::IVec3(7,7,0);
		lookupPrecision.colorMask		= getCompareMask(pixelFormat);

		const bool isHighQuality = verifyTextureResult(m_context.getTestContext(), rendered.getAccess(), (tcu::Texture2DView)texture.getTexture(),
													   &texCoord[0], refParams, lookupPrecision, lodPrecision, pixelFormat);

		if (!isHighQuality)
		{
			// Evaluate against lower precision requirements.
			lodPrecision.lodBits	= 4;
			lookupPrecision.uvwBits	= tcu::IVec3(4,4,0);

			log << TestLog::Message << "Warning: Verification against high precision requirements failed, trying with lower requirements." << TestLog::EndMessage;

			const bool isOk = verifyTextureResult(m_context.getTestContext(), rendered.getAccess(), (tcu::Texture2DView)texture.getTexture(),
												  &texCoord[0], refParams, lookupPrecision, lodPrecision, pixelFormat);

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

struct TextureCubeFilteringTestCaseParameters : public TextureCubeTestCaseParameters
{
	bool	onlySampleFaceInterior;
};

class TextureCubeFilteringTestInstance : public TestInstance
{
public:
	typedef TextureCubeFilteringTestCaseParameters	ParameterType;

													TextureCubeFilteringTestInstance	(Context& context, const ParameterType& testParameters);
													~TextureCubeFilteringTestInstance	(void);

	virtual tcu::TestStatus							iterate								(void);

private:
													TextureCubeFilteringTestInstance	(const TextureCubeFilteringTestInstance& other);
	TextureCubeFilteringTestInstance&				operator=							(const TextureCubeFilteringTestInstance& other);

	struct FilterCase
	{
		int						textureIndex;
		tcu::Vec2				bottomLeft;
		tcu::Vec2				topRight;

		FilterCase (void)
			: textureIndex(-1)
		{
		}

		FilterCase (int tex_, const tcu::Vec2& bottomLeft_, const tcu::Vec2& topRight_)
			: textureIndex	(tex_)
			, bottomLeft	(bottomLeft_)
			, topRight		(topRight_)
		{
		}
	};

	const ParameterType			m_testParameters;
	vector<TestTextureCubeSp>	m_textures;
	vector<FilterCase>			m_cases;
	TextureRenderer				m_renderer;
	int							m_caseNdx;
};

TextureCubeFilteringTestInstance::TextureCubeFilteringTestInstance (Context& context, const ParameterType& testParameters)
	: TestInstance			(context)
	, m_testParameters		(testParameters)
	, m_renderer			(context, testParameters.sampleCount, TEXCUBE_VIEWPORT_SIZE, TEXCUBE_VIEWPORT_SIZE)
	, m_caseNdx				(0)
{
	const int						numLevels	= deLog2Floor32(m_testParameters.size)+1;
	const tcu::TextureFormatInfo	fmtInfo		= tcu::getTextureFormatInfo(vk::mapVkFormat(m_testParameters.format));
	const tcu::Vec4					cBias		= fmtInfo.valueMin;
	const tcu::Vec4					cScale		= fmtInfo.valueMax-fmtInfo.valueMin;

	if ((testParameters.wrapS == Sampler::MIRRORED_ONCE ||
		testParameters.wrapT == Sampler::MIRRORED_ONCE) &&
		!de::contains(context.getDeviceExtensions().begin(), context.getDeviceExtensions().end(), "VK_KHR_sampler_mirror_clamp_to_edge"))
		TCU_THROW(NotSupportedError, "VK_KHR_sampler_mirror_clamp_to_edge not supported");

	m_textures.reserve(2);
	for (int ndx = 0; ndx < 2; ndx++)
		m_textures.push_back(TestTextureCubeSp(new pipeline::TestTextureCube(vk::mapVkFormat(m_testParameters.format), m_testParameters.size)));

	// Fill first with gradient texture.
	static const tcu::Vec4 gradients[tcu::CUBEFACE_LAST][2] =
	{
		{ tcu::Vec4(0.0f, 0.0f, 0.0f, 1.0f), tcu::Vec4(1.0f, 1.0f, 1.0f, 0.0f) }, // negative x
		{ tcu::Vec4(0.5f, 0.0f, 0.0f, 1.0f), tcu::Vec4(1.0f, 1.0f, 1.0f, 0.0f) }, // positive x
		{ tcu::Vec4(0.0f, 0.5f, 0.0f, 1.0f), tcu::Vec4(1.0f, 1.0f, 1.0f, 0.0f) }, // negative y
		{ tcu::Vec4(0.0f, 0.0f, 0.5f, 1.0f), tcu::Vec4(1.0f, 1.0f, 1.0f, 0.0f) }, // positive y
		{ tcu::Vec4(0.0f, 0.0f, 0.0f, 0.5f), tcu::Vec4(1.0f, 1.0f, 1.0f, 1.0f) }, // negative z
		{ tcu::Vec4(0.5f, 0.5f, 0.5f, 1.0f), tcu::Vec4(1.0f, 1.0f, 1.0f, 0.0f) }  // positive z
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
		m_renderer.addCubeTexture(*i);
	}

	// Compute cases
	{
		const int tex0	= 0;
		const int tex1	= m_textures.size() > 1 ? 1 : 0;

		if (m_testParameters.onlySampleFaceInterior)
		{
			m_cases.push_back(FilterCase(tex0, tcu::Vec2(-0.8f, -0.8f), tcu::Vec2(0.8f,  0.8f)));	// minification
			m_cases.push_back(FilterCase(tex0, tcu::Vec2(0.5f, 0.65f), tcu::Vec2(0.8f,  0.8f)));	// magnification
			m_cases.push_back(FilterCase(tex1, tcu::Vec2(-0.8f, -0.8f), tcu::Vec2(0.8f,  0.8f)));	// minification
			m_cases.push_back(FilterCase(tex1, tcu::Vec2(0.2f, 0.2f), tcu::Vec2(0.6f,  0.5f)));		// magnification
		}
		else
		{
			m_cases.push_back(FilterCase(tex0, tcu::Vec2(-1.25f, -1.2f), tcu::Vec2(1.2f, 1.25f)));	// minification

			m_cases.push_back(FilterCase(tex0, tcu::Vec2(0.8f, 0.8f), tcu::Vec2(1.25f, 1.20f)));	// magnification
			m_cases.push_back(FilterCase(tex1, tcu::Vec2(-1.19f, -1.3f), tcu::Vec2(1.1f, 1.35f)));	// minification
			m_cases.push_back(FilterCase(tex1, tcu::Vec2(-1.2f, -1.1f), tcu::Vec2(-0.8f, -0.8f)));	// magnification
		}
	}
}

TextureCubeFilteringTestInstance::~TextureCubeFilteringTestInstance (void)
{
}

const char* getFaceDesc (const tcu::CubeFace face)
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

tcu::TestStatus TextureCubeFilteringTestInstance::iterate (void)
{
	tcu::TestLog&						log			= m_context.getTestContext().getLog();

	const pipeline::TestTextureCube&	texture		= m_renderer.getCubeTexture(m_cases[m_caseNdx].textureIndex);
	const tcu::TextureFormat			texFmt		= texture.getTextureFormat();
	const tcu::TextureFormatInfo		fmtInfo		= tcu::getTextureFormatInfo(texFmt);
	const FilterCase&					curCase		= m_cases[m_caseNdx];
	ReferenceParams						refParams	(TEXTURETYPE_CUBE);

	// Params for reference computation.
	refParams.sampler					= util::createSampler(Sampler::CLAMP_TO_EDGE, Sampler::CLAMP_TO_EDGE, m_testParameters.minFilter, m_testParameters.magFilter);
	refParams.sampler.seamlessCubeMap	= true;
	refParams.samplerType				= getSamplerType(texFmt);
	refParams.lodMode					= LODMODE_EXACT;
	refParams.colorBias					= fmtInfo.lookupBias;
	refParams.colorScale				= fmtInfo.lookupScale;

	log << TestLog::Message << "Coordinates: " << curCase.bottomLeft << " -> " << curCase.topRight << TestLog::EndMessage;

	for (int faceNdx = 0; faceNdx < tcu::CUBEFACE_LAST; faceNdx++)
	{
		const tcu::CubeFace		face		= tcu::CubeFace(faceNdx);
		tcu::Surface			rendered	(m_renderer.getRenderWidth(), m_renderer.getRenderHeight());
		vector<float>			texCoord;

		computeQuadTexCoordCube(texCoord, face, curCase.bottomLeft, curCase.topRight);

		log << TestLog::Message << "Face " << getFaceDesc(face) << TestLog::EndMessage;

		// \todo Log texture coordinates.

		m_renderer.renderQuad(rendered, curCase.textureIndex, &texCoord[0], refParams);

		{
			const bool				isNearestOnly	= m_testParameters.minFilter == Sampler::NEAREST && m_testParameters.magFilter == Sampler::NEAREST;
			const tcu::IVec4		formatBitDepth	= getTextureFormatBitDepth(vk::mapVkFormat(VK_FORMAT_R8G8B8A8_UNORM));
			const tcu::PixelFormat	pixelFormat		(formatBitDepth[0], formatBitDepth[1], formatBitDepth[2], formatBitDepth[3]);
			const tcu::IVec4		colorBits		= max(getBitsVec(pixelFormat) - (isNearestOnly ? 1 : 2), tcu::IVec4(0)); // 1 inaccurate bit if nearest only, 2 otherwise
			tcu::LodPrecision		lodPrecision;
			tcu::LookupPrecision	lookupPrecision;

			lodPrecision.derivateBits		= 10;
			lodPrecision.lodBits			= 5;
			lookupPrecision.colorThreshold	= tcu::computeFixedPointThreshold(colorBits) / refParams.colorScale;
			lookupPrecision.coordBits		= tcu::IVec3(10,10,10);
			lookupPrecision.uvwBits			= tcu::IVec3(6,6,0);
			lookupPrecision.colorMask		= getCompareMask(pixelFormat);

			const bool isHighQuality = verifyTextureResult(m_context.getTestContext(), rendered.getAccess(), (tcu::TextureCubeView)texture.getTexture(),
														   &texCoord[0], refParams, lookupPrecision, lodPrecision, pixelFormat);

			if (!isHighQuality)
			{
				// Evaluate against lower precision requirements.
				lodPrecision.lodBits	= 4;
				lookupPrecision.uvwBits	= tcu::IVec3(4,4,0);

				log << TestLog::Message << "Warning: Verification against high precision requirements failed, trying with lower requirements." << TestLog::EndMessage;

				const bool isOk = verifyTextureResult(m_context.getTestContext(), rendered.getAccess(), (tcu::TextureCubeView)texture.getTexture(),
													  &texCoord[0], refParams, lookupPrecision, lodPrecision, pixelFormat);

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

// 2D array filtering

class Texture2DArrayFilteringTestInstance : public TestInstance
{
public:
	typedef Texture2DArrayTestCaseParameters	ParameterType;

												Texture2DArrayFilteringTestInstance		(Context& context, const ParameterType& testParameters);
												~Texture2DArrayFilteringTestInstance	(void);

	virtual tcu::TestStatus						iterate									(void);

private:
												Texture2DArrayFilteringTestInstance		(const Texture2DArrayFilteringTestInstance&);
	Texture2DArrayFilteringTestInstance&		operator=								(const Texture2DArrayFilteringTestInstance&);

	struct FilterCase
	{
		int							textureIndex;
		tcu::Vec2					lod;
		tcu::Vec2					offset;
		tcu::Vec2					layerRange;

		FilterCase (void)
			: textureIndex(-1)
		{
		}

		FilterCase (const int tex_, const tcu::Vec2& lod_, const tcu::Vec2& offset_, const tcu::Vec2& layerRange_)
			: textureIndex	(tex_)
			, lod			(lod_)
			, offset		(offset_)
			, layerRange	(layerRange_)
		{
		}
	};

	const ParameterType				m_testParameters;
	vector<TestTexture2DArraySp>	m_textures;
	vector<FilterCase>				m_cases;
	TextureRenderer					m_renderer;
	int								m_caseNdx;
};

Texture2DArrayFilteringTestInstance::Texture2DArrayFilteringTestInstance (Context& context, const ParameterType& testParameters)
	: TestInstance			(context)
	, m_testParameters		(testParameters)
	, m_renderer			(context, testParameters.sampleCount, TEX3D_VIEWPORT_WIDTH, TEX3D_VIEWPORT_HEIGHT)
	, m_caseNdx				(0)
{
	const tcu::TextureFormatInfo	fmtInfo		= tcu::getTextureFormatInfo(vk::mapVkFormat(m_testParameters.format));
	const tcu::Vec4					cScale		= fmtInfo.valueMax-fmtInfo.valueMin;
	const tcu::Vec4					cBias		= fmtInfo.valueMin;
	const int						numLevels	= deLog2Floor32(de::max(m_testParameters.width, m_testParameters.height)) + 1;

	if ((testParameters.wrapS == Sampler::MIRRORED_ONCE ||
		testParameters.wrapT == Sampler::MIRRORED_ONCE) &&
		!de::contains(context.getDeviceExtensions().begin(), context.getDeviceExtensions().end(), "VK_KHR_sampler_mirror_clamp_to_edge"))
		TCU_THROW(NotSupportedError, "VK_KHR_sampler_mirror_clamp_to_edge not supported");

	// Create textures.
	m_textures.reserve(2);
	for (int ndx = 0; ndx < 2; ndx++)
		m_textures.push_back(TestTexture2DArraySp(new pipeline::TestTexture2DArray(vk::mapVkFormat(m_testParameters.format), m_testParameters.width, m_testParameters.height, m_testParameters.numLayers)));

	const tcu::IVec4 levelSwz[] =
	{
		tcu::IVec4(0,1,2,3),
		tcu::IVec4(2,1,3,0),
		tcu::IVec4(3,0,1,2),
		tcu::IVec4(1,3,2,0),
	};

	// Fill first gradient texture (gradient direction varies between layers).
	for (int levelNdx = 0; levelNdx < numLevels; levelNdx++)
	{
		for (int layerNdx = 0; layerNdx < m_testParameters.numLayers; layerNdx++)
		{
			const tcu::PixelBufferAccess levelBuf = m_textures[0]->getLevel(levelNdx, layerNdx);

			const tcu::IVec4	swz		= levelSwz[layerNdx%DE_LENGTH_OF_ARRAY(levelSwz)];
			const tcu::Vec4		gMin	= tcu::Vec4(0.0f, 0.0f, 0.0f, 1.0f).swizzle(swz[0],swz[1],swz[2],swz[3])*cScale + cBias;
			const tcu::Vec4		gMax	= tcu::Vec4(1.0f, 1.0f, 1.0f, 0.0f).swizzle(swz[0],swz[1],swz[2],swz[3])*cScale + cBias;

			tcu::fillWithComponentGradients(levelBuf, gMin, gMax);
		}
	}

	// Fill second with grid texture (each layer has unique colors).
	for (int levelNdx = 0; levelNdx < numLevels; levelNdx++)
	{
		for (int layerNdx = 0; layerNdx < m_testParameters.numLayers; layerNdx++)
		{
			const tcu::PixelBufferAccess levelBuf = m_textures[1]->getLevel(levelNdx, layerNdx);

			const deUint32	step	= 0x00ffffff / (numLevels*m_testParameters.numLayers - 1);
			const deUint32	rgb		= step * (levelNdx + layerNdx*numLevels);
			const deUint32	colorA	= 0xff000000 | rgb;
			const deUint32	colorB	= 0xff000000 | ~rgb;

			tcu::fillWithGrid(levelBuf, 4, tcu::RGBA(colorA).toVec()*cScale + cBias, tcu::RGBA(colorB).toVec()*cScale + cBias);
		}
	}

	// Upload.
	for (vector<TestTexture2DArraySp>::const_iterator i = m_textures.begin(); i != m_textures.end(); i++)
	{
		m_renderer.add2DArrayTexture(*i);
	}

	// Test cases
	m_cases.push_back(FilterCase(0,	tcu::Vec2( 1.5f,  2.8f  ),	tcu::Vec2(-1.0f, -2.7f), tcu::Vec2(-0.5f, float(m_testParameters.numLayers)+0.5f)));
	m_cases.push_back(FilterCase(1,	tcu::Vec2( 0.2f,  0.175f),	tcu::Vec2(-2.0f, -3.7f), tcu::Vec2(-0.5f, float(m_testParameters.numLayers)+0.5f)));
	m_cases.push_back(FilterCase(1,	tcu::Vec2(-0.8f, -2.3f  ),	tcu::Vec2( 0.2f, -0.1f), tcu::Vec2(float(m_testParameters.numLayers)+0.5f, -0.5f)));
	m_cases.push_back(FilterCase(0,	tcu::Vec2(-2.0f, -1.5f  ),	tcu::Vec2(-0.1f,  0.9f), tcu::Vec2(1.50001f, 1.49999f)));
}

Texture2DArrayFilteringTestInstance::~Texture2DArrayFilteringTestInstance (void)
{
}

tcu::TestStatus Texture2DArrayFilteringTestInstance::iterate (void)
{
	tcu::TestLog&						log			= m_context.getTestContext().getLog();

	const FilterCase&					curCase		= m_cases[m_caseNdx];
	const pipeline::TestTexture2DArray&	texture		= m_renderer.get2DArrayTexture(curCase.textureIndex);
	const tcu::TextureFormat			texFmt		= texture.getTextureFormat();
	const tcu::TextureFormatInfo		fmtInfo		= tcu::getTextureFormatInfo(texFmt);
	ReferenceParams						refParams	(TEXTURETYPE_2D_ARRAY);
	tcu::Surface						rendered	(m_renderer.getRenderWidth(), m_renderer.getRenderHeight());
	tcu::Vec3							texCoord[4];
	const float* const					texCoordPtr	= (const float*)&texCoord[0];

	// Params for reference computation.

	refParams.sampler		= util::createSampler(m_testParameters.wrapS, m_testParameters.wrapT, m_testParameters.minFilter, m_testParameters.magFilter);
	refParams.samplerType	= getSamplerType(texFmt);
	refParams.lodMode		= LODMODE_EXACT;
	refParams.colorBias		= fmtInfo.lookupBias;
	refParams.colorScale	= fmtInfo.lookupScale;

	// Compute texture coordinates.
	log << TestLog::Message << "Approximate lod per axis = " << curCase.lod << ", offset = " << curCase.offset << TestLog::EndMessage;

	{
		const float	lodX	= curCase.lod.x();
		const float	lodY	= curCase.lod.y();
		const float	oX		= curCase.offset.x();
		const float	oY		= curCase.offset.y();
		const float	sX		= deFloatExp2(lodX) * float(m_renderer.getRenderWidth()) / float(m_textures[0]->getTexture().getWidth());
		const float	sY		= deFloatExp2(lodY) * float(m_renderer.getRenderHeight()) / float(m_textures[0]->getTexture().getHeight());
		const float	l0		= curCase.layerRange.x();
		const float	l1		= curCase.layerRange.y();

		texCoord[0] = tcu::Vec3(oX,		oY,		l0);
		texCoord[1] = tcu::Vec3(oX,		oY+sY,	l0*0.5f + l1*0.5f);
		texCoord[2] = tcu::Vec3(oX+sX,	oY,		l0*0.5f + l1*0.5f);
		texCoord[3] = tcu::Vec3(oX+sX,	oY+sY,	l1);
	}

	m_renderer.renderQuad(rendered, curCase.textureIndex, texCoordPtr, refParams);

	{

		const bool				isNearestOnly	= m_testParameters.minFilter == Sampler::NEAREST && m_testParameters.magFilter == Sampler::NEAREST;
		const tcu::IVec4		formatBitDepth	= getTextureFormatBitDepth(vk::mapVkFormat(VK_FORMAT_R8G8B8A8_UNORM));
		const tcu::PixelFormat	pixelFormat		(formatBitDepth[0], formatBitDepth[1], formatBitDepth[2], formatBitDepth[3]);
		const tcu::IVec4		colorBits		= max(getBitsVec(pixelFormat) - (isNearestOnly ? 1 : 2), tcu::IVec4(0)); // 1 inaccurate bit if nearest only, 2 otherwise
		tcu::LodPrecision		lodPrecision;
		tcu::LookupPrecision	lookupPrecision;

		lodPrecision.derivateBits		= 18;
		lodPrecision.lodBits			= 6;
		lookupPrecision.colorThreshold	= tcu::computeFixedPointThreshold(colorBits) / refParams.colorScale;
		lookupPrecision.coordBits		= tcu::IVec3(20,20,20);
		lookupPrecision.uvwBits			= tcu::IVec3(7,7,0);
		lookupPrecision.colorMask		= getCompareMask(pixelFormat);

		const bool isHighQuality = verifyTextureResult(m_context.getTestContext(), rendered.getAccess(), (tcu::Texture2DArrayView)texture.getTexture(),
													   texCoordPtr, refParams, lookupPrecision, lodPrecision, pixelFormat);

		if (!isHighQuality)
		{
			// Evaluate against lower precision requirements.
			lodPrecision.lodBits	= 4;
			lookupPrecision.uvwBits	= tcu::IVec3(4,4,0);

			log << TestLog::Message << "Warning: Verification against high precision requirements failed, trying with lower requirements." << TestLog::EndMessage;

			const bool isOk = verifyTextureResult(m_context.getTestContext(), rendered.getAccess(), (tcu::Texture2DArrayView)texture.getTexture(),
												  texCoordPtr, refParams, lookupPrecision, lodPrecision, pixelFormat);

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

// 3D filtering

class Texture3DFilteringTestInstance : public TestInstance
{
public:
	typedef Texture3DTestCaseParameters	ParameterType;

										Texture3DFilteringTestInstance		(Context& context, const ParameterType& testParameters);
										~Texture3DFilteringTestInstance		(void);

	virtual tcu::TestStatus				iterate								(void);

private:
										Texture3DFilteringTestInstance		(const Texture3DFilteringTestInstance& other);
	Texture3DFilteringTestInstance&		operator=							(const Texture3DFilteringTestInstance& other);

	struct FilterCase
	{
		int						textureIndex;
		tcu::Vec3				lod;
		tcu::Vec3				offset;

		FilterCase (void)
			: textureIndex(-1)
		{
		}

		FilterCase (const int tex_, const tcu::Vec3& lod_, const tcu::Vec3& offset_)
			: textureIndex	(tex_)
			, lod			(lod_)
			, offset		(offset_)
		{
		}
	};

	const ParameterType			m_testParameters;
	vector<TestTexture3DSp>		m_textures;
	vector<FilterCase>			m_cases;
	TextureRenderer				m_renderer;
	int							m_caseNdx;
};

Texture3DFilteringTestInstance::Texture3DFilteringTestInstance (Context& context, const ParameterType& testParameters)
	: TestInstance			(context)
	, m_testParameters		(testParameters)
	, m_renderer			(context, testParameters.sampleCount, TEX3D_VIEWPORT_WIDTH, TEX3D_VIEWPORT_HEIGHT)
	, m_caseNdx				(0)
{
	const tcu::TextureFormatInfo	fmtInfo		= tcu::getTextureFormatInfo(vk::mapVkFormat(m_testParameters.format));
	const tcu::Vec4					cScale		= fmtInfo.valueMax-fmtInfo.valueMin;
	const tcu::Vec4					cBias		= fmtInfo.valueMin;
	const int						numLevels	= deLog2Floor32(de::max(de::max(m_testParameters.width, m_testParameters.height), m_testParameters.depth)) + 1;

	if ((testParameters.wrapS == Sampler::MIRRORED_ONCE ||
		testParameters.wrapT == Sampler::MIRRORED_ONCE ||
		testParameters.wrapR == Sampler::MIRRORED_ONCE) &&
		!de::contains(context.getDeviceExtensions().begin(), context.getDeviceExtensions().end(), "VK_KHR_sampler_mirror_clamp_to_edge"))
		TCU_THROW(NotSupportedError, "VK_KHR_sampler_mirror_clamp_to_edge not supported");

	// Create textures.
	m_textures.reserve(2);
	for (int ndx = 0; ndx < 2; ndx++)
		m_textures.push_back(TestTexture3DSp(new pipeline::TestTexture3D(vk::mapVkFormat(m_testParameters.format), m_testParameters.width, m_testParameters.height, m_testParameters.depth)));

	// Fill first gradient texture.
	for (int levelNdx = 0; levelNdx < numLevels; levelNdx++)
	{
		const tcu::Vec4 gMin = tcu::Vec4(0.0f, 0.0f, 0.0f, 1.0f)*cScale + cBias;
		const tcu::Vec4 gMax = tcu::Vec4(1.0f, 1.0f, 1.0f, 0.0f)*cScale + cBias;

		tcu::fillWithComponentGradients(m_textures[0]->getLevel(levelNdx, 0), gMin, gMax);
	}

	// Fill second with grid texture.
	for (int levelNdx = 0; levelNdx < numLevels; levelNdx++)
	{
		const deUint32	step	= 0x00ffffff / numLevels;
		const deUint32	rgb		= step*levelNdx;
		const deUint32	colorA	= 0xff000000 | rgb;
		const deUint32	colorB	= 0xff000000 | ~rgb;

		tcu::fillWithGrid(m_textures[1]->getLevel(levelNdx, 0), 4, tcu::RGBA(colorA).toVec()*cScale + cBias, tcu::RGBA(colorB).toVec()*cScale + cBias);
	}

	// Upload.
	for (vector<TestTexture3DSp>::const_iterator i = m_textures.begin(); i != m_textures.end(); i++)
	{
		m_renderer.add3DTexture(*i);
	}

	// Test cases
	m_cases.push_back(FilterCase(0,	tcu::Vec3(1.5f, 2.8f, 1.0f),	tcu::Vec3(-1.0f, -2.7f, -2.275f)));
	m_cases.push_back(FilterCase(0,	tcu::Vec3(-2.0f, -1.5f, -1.8f),	tcu::Vec3(-0.1f, 0.9f, -0.25f)));
	m_cases.push_back(FilterCase(1,	tcu::Vec3(0.2f, 0.175f, 0.3f),	tcu::Vec3(-2.0f, -3.7f, -1.825f)));
	m_cases.push_back(FilterCase(1,	tcu::Vec3(-0.8f, -2.3f, -2.5f),	tcu::Vec3(0.2f, -0.1f, 1.325f)));
}

Texture3DFilteringTestInstance::~Texture3DFilteringTestInstance (void)
{
}

tcu::TestStatus Texture3DFilteringTestInstance::iterate (void)
{
	tcu::TestLog&						log			= m_context.getTestContext().getLog();

	const pipeline::TestTexture3D&	texture		= m_renderer.get3DTexture(m_cases[m_caseNdx].textureIndex);
	const tcu::TextureFormat		texFmt		= texture.getTextureFormat();
	const tcu::TextureFormatInfo	fmtInfo		= tcu::getTextureFormatInfo(texFmt);
	const FilterCase&				curCase		= m_cases[m_caseNdx];
	ReferenceParams					refParams	(TEXTURETYPE_3D);
	tcu::Surface					rendered	(m_renderer.getRenderWidth(), m_renderer.getRenderHeight());
	tcu::Vec3						texCoord[4];
	const float* const				texCoordPtr	= (const float*)&texCoord[0];

	// Params for reference computation.
	refParams.sampler		= util::createSampler(m_testParameters.wrapS, m_testParameters.wrapT, m_testParameters.wrapR, m_testParameters.minFilter, m_testParameters.magFilter);
	refParams.samplerType	= getSamplerType(texFmt);
	refParams.lodMode		= LODMODE_EXACT;
	refParams.colorBias		= fmtInfo.lookupBias;
	refParams.colorScale	= fmtInfo.lookupScale;

	// Compute texture coordinates.
	log << TestLog::Message << "Approximate lod per axis = " << curCase.lod << ", offset = " << curCase.offset << TestLog::EndMessage;

	{
		const float	lodX	= curCase.lod.x();
		const float	lodY	= curCase.lod.y();
		const float	lodZ	= curCase.lod.z();
		const float	oX		= curCase.offset.x();
		const float	oY		= curCase.offset.y();
		const float oZ		= curCase.offset.z();
		const float	sX		= deFloatExp2(lodX) * float(m_renderer.getRenderWidth())										/ float(m_textures[0]->getTexture().getWidth());
		const float	sY		= deFloatExp2(lodY) * float(m_renderer.getRenderHeight())										/ float(m_textures[0]->getTexture().getHeight());
		const float	sZ		= deFloatExp2(lodZ) * float(de::max(m_renderer.getRenderWidth(), m_renderer.getRenderHeight()))	/ float(m_textures[0]->getTexture().getDepth());

		texCoord[0] = tcu::Vec3(oX,		oY,		oZ);
		texCoord[1] = tcu::Vec3(oX,		oY+sY,	oZ + sZ*0.5f);
		texCoord[2] = tcu::Vec3(oX+sX,	oY,		oZ + sZ*0.5f);
		texCoord[3] = tcu::Vec3(oX+sX,	oY+sY,	oZ + sZ);
	}

	m_renderer.renderQuad(rendered, curCase.textureIndex, texCoordPtr, refParams);

	{
		const bool				isNearestOnly	= m_testParameters.minFilter == Sampler::NEAREST && m_testParameters.magFilter == Sampler::NEAREST;
		const tcu::IVec4		formatBitDepth	= getTextureFormatBitDepth(vk::mapVkFormat(VK_FORMAT_R8G8B8A8_UNORM));
		const tcu::PixelFormat	pixelFormat		(formatBitDepth[0], formatBitDepth[1], formatBitDepth[2], formatBitDepth[3]);
		const tcu::IVec4		colorBits		= max(getBitsVec(pixelFormat) - (isNearestOnly ? 1 : 2), tcu::IVec4(0)); // 1 inaccurate bit if nearest only, 2 otherwise
		tcu::LodPrecision		lodPrecision;
		tcu::LookupPrecision	lookupPrecision;

		lodPrecision.derivateBits		= 18;
		lodPrecision.lodBits			= 6;
		lookupPrecision.colorThreshold	= tcu::computeFixedPointThreshold(colorBits) / refParams.colorScale;
		lookupPrecision.coordBits		= tcu::IVec3(20,20,20);
		lookupPrecision.uvwBits			= tcu::IVec3(7,7,7);
		lookupPrecision.colorMask		= getCompareMask(pixelFormat);

		const bool isHighQuality = verifyTextureResult(m_context.getTestContext(), rendered.getAccess(), (tcu::Texture3DView)texture.getTexture(),
													   texCoordPtr, refParams, lookupPrecision, lodPrecision, pixelFormat);

		if (!isHighQuality)
		{
			// Evaluate against lower precision requirements.
			lodPrecision.lodBits	= 4;
			lookupPrecision.uvwBits	= tcu::IVec3(4,4,4);

			log << TestLog::Message << "Warning: Verification against high precision requirements failed, trying with lower requirements." << TestLog::EndMessage;

			const bool isOk = verifyTextureResult(m_context.getTestContext(), rendered.getAccess(), (tcu::Texture3DView)texture.getTexture(),
												  texCoordPtr, refParams, lookupPrecision, lodPrecision, pixelFormat);

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

bool verifierCanBeUsed (const VkFormat format, const Sampler::FilterMode minFilter, const Sampler::FilterMode magFilter)
{
	const tcu::TextureFormat				textureFormat		= mapVkFormat(format);
	const tcu::TextureChannelClass			textureChannelClass	= tcu::getTextureChannelClass(textureFormat.type);

	return !(!(textureChannelClass == tcu::TEXTURECHANNELCLASS_UNSIGNED_FIXED_POINT	||
			   textureChannelClass == tcu::TEXTURECHANNELCLASS_SIGNED_FIXED_POINT	||
			   textureChannelClass == tcu::TEXTURECHANNELCLASS_FLOATING_POINT) &&
			  (tcu::TexVerifierUtil::isLinearFilter(minFilter) || tcu::TexVerifierUtil::isLinearFilter(magFilter)));
}

void populateTextureFilteringTests (tcu::TestCaseGroup* textureFilteringTests)
{
	tcu::TestContext&	testCtx		= textureFilteringTests->getTestContext();

	static const struct
	{
		const char* const			name;
		const Sampler::WrapMode		mode;
	} wrapModes[] =
	{
		{ "repeat",					Sampler::REPEAT_GL			},
		{ "mirrored_repeat",		Sampler::MIRRORED_REPEAT_GL	},
		{ "clamp_to_edge",			Sampler::CLAMP_TO_EDGE		},
		{ "clamp_to_border",		Sampler::CLAMP_TO_BORDER	},
		{ "mirror_clamp_to_edge",	Sampler::MIRRORED_ONCE		}
	};

	static const struct
	{
		const char* const			name;
		const Sampler::FilterMode	mode;
	} minFilterModes[] =
	{
		{ "nearest",				Sampler::NEAREST					},
		{ "linear",					Sampler::LINEAR						},
		{ "nearest_mipmap_nearest",	Sampler::NEAREST_MIPMAP_NEAREST		},
		{ "linear_mipmap_nearest",	Sampler::LINEAR_MIPMAP_NEAREST		},
		{ "nearest_mipmap_linear",	Sampler::NEAREST_MIPMAP_LINEAR		},
		{ "linear_mipmap_linear",	Sampler::LINEAR_MIPMAP_LINEAR		}
	};

	static const struct
	{
		const char* const			name;
		const Sampler::FilterMode	mode;
	} magFilterModes[] =
	{
		{ "nearest",				Sampler::NEAREST },
		{ "linear",					Sampler::LINEAR	 }
	};

	static const struct
	{
		const int	width;
		const int	height;
	} sizes2D[] =
	{
		{   4,	  8 },
		{  32,	 64 },
		{ 128,	128	},
		{   3,	  7 },
		{  31,	 55 },
		{ 127,	 99 }
	};

	static const struct
	{
		const int	size;
	} sizesCube[] =
	{
		{   8 },
		{  64 },
		{ 128 },
		{   7 },
		{  63 }
	};

	static const struct
	{
		const int	width;
		const int	height;
		const int	numLayers;
	} sizes2DArray[] =
	{
		{   4,   8,   8 },
		{  32,  64,  16 },
		{ 128,  32,  64 },
		{   3,   7,   5 },
		{  63,  63,  63 }
	};

	static const struct
	{
		const int	width;
		const int	height;
		const int	depth;
	} sizes3D[] =
	{
		{   4,   8,   8 },
		{  32,  64,  16 },
		{ 128,  32,  64 },
		{   3,   7,   5 },
		{  63,  63,  63 }
	};

	static const struct
	{
		const char* const	name;
		const VkFormat		format;
	} filterableFormatsByType[] =
	{
		{ "r16g16b16a16_sfloat",	VK_FORMAT_R16G16B16A16_SFLOAT		},
		{ "b10g11r11_ufloat",		VK_FORMAT_B10G11R11_UFLOAT_PACK32	},
		{ "e5b9g9r9_ufloat",		VK_FORMAT_E5B9G9R9_UFLOAT_PACK32	},
		{ "r8g8b8a8_unorm",			VK_FORMAT_R8G8B8A8_UNORM			},
		{ "r8g8b8a8_snorm",			VK_FORMAT_R8G8B8A8_SNORM			},
		{ "r5g6b5_unorm",			VK_FORMAT_R5G6B5_UNORM_PACK16		},
		{ "r4g4b4a4_unorm",			VK_FORMAT_R4G4B4A4_UNORM_PACK16		},
		{ "r5g5b5a1_unorm",			VK_FORMAT_R5G5B5A1_UNORM_PACK16		},
		{ "a8b8g8r8_srgb",			VK_FORMAT_A8B8G8R8_SRGB_PACK32		},
		{ "a1r5g5b5_unorm",			VK_FORMAT_A1R5G5B5_UNORM_PACK16		}
	};

	// 2D texture filtering.
	{
		de::MovePtr<tcu::TestCaseGroup>	group2D				(new tcu::TestCaseGroup(testCtx, "2d", "2D Texture Filtering"));

		de::MovePtr<tcu::TestCaseGroup>	formatsGroup		(new tcu::TestCaseGroup(testCtx, "formats", "2D Texture Formats"));
		de::MovePtr<tcu::TestCaseGroup>	sizesGroup			(new tcu::TestCaseGroup(testCtx, "sizes", "Texture Sizes"));
		de::MovePtr<tcu::TestCaseGroup>	combinationsGroup	(new tcu::TestCaseGroup(testCtx, "combinations", "Filter and wrap mode combinations"));

		// Formats.
		for (int fmtNdx = 0; fmtNdx < DE_LENGTH_OF_ARRAY(filterableFormatsByType); fmtNdx++)
		{
			const string					filterGroupName	= filterableFormatsByType[fmtNdx].name;
			de::MovePtr<tcu::TestCaseGroup>	filterGroup		(new tcu::TestCaseGroup(testCtx, filterGroupName.c_str(), ""));

			for (int filterNdx = 0; filterNdx < DE_LENGTH_OF_ARRAY(minFilterModes); filterNdx++)
			{
				const Sampler::FilterMode	minFilter		= minFilterModes[filterNdx].mode;
				const bool					isMipmap		= minFilter != Sampler::NEAREST && minFilter != Sampler::LINEAR;
				const string				name			= minFilterModes[filterNdx].name;
				Texture2DTestCaseParameters	testParameters;

				testParameters.format		= filterableFormatsByType[fmtNdx].format;
				testParameters.minFilter	= minFilter;
				testParameters.magFilter	= isMipmap ? Sampler::LINEAR : minFilter;

				testParameters.wrapS		= Sampler::REPEAT_GL;
				testParameters.wrapT		= Sampler::REPEAT_GL;
				testParameters.width		= 64;
				testParameters.height		= 64;

				testParameters.programs.push_back(PROGRAM_2D_FLOAT);
				testParameters.programs.push_back(PROGRAM_2D_UINT);

				// Some combinations of the tests have to be skipped due to the restrictions of the verifiers.
				if (verifierCanBeUsed(testParameters.format, testParameters.minFilter, testParameters.magFilter))
				{
					filterGroup->addChild(new TextureTestCase<Texture2DFilteringTestInstance>(testCtx, name.c_str(), "", testParameters));
				}
			}
			formatsGroup->addChild(filterGroup.release());
		}

		// Sizes.
		for (int sizeNdx = 0; sizeNdx < DE_LENGTH_OF_ARRAY(sizes2D); sizeNdx++)
		{
			const string					filterGroupName = de::toString(sizes2D[sizeNdx].width) + "x" + de::toString(sizes2D[sizeNdx].height);
			de::MovePtr<tcu::TestCaseGroup>	filterGroup		(new tcu::TestCaseGroup(testCtx, filterGroupName.c_str(), ""));

			for (int filterNdx = 0; filterNdx < DE_LENGTH_OF_ARRAY(minFilterModes); filterNdx++)
			{
				const Sampler::FilterMode	minFilter		= minFilterModes[filterNdx].mode;
				const bool					isMipmap		= minFilter != Sampler::NEAREST && minFilter != Sampler::LINEAR;
				const string				name			= minFilterModes[filterNdx].name;
				Texture2DTestCaseParameters	testParameters;

				testParameters.format		= VK_FORMAT_R8G8B8A8_UNORM;
				testParameters.minFilter	= minFilter;
				testParameters.magFilter	= isMipmap ? Sampler::LINEAR : minFilter;
				testParameters.wrapS		= Sampler::REPEAT_GL;
				testParameters.wrapT		= Sampler::REPEAT_GL;
				testParameters.width		= sizes2D[sizeNdx].width;
				testParameters.height		= sizes2D[sizeNdx].height;

				testParameters.programs.push_back(PROGRAM_2D_FLOAT);

				filterGroup->addChild(new TextureTestCase<Texture2DFilteringTestInstance>(testCtx, name.c_str(), "", testParameters));
			}
			sizesGroup->addChild(filterGroup.release());
		}

		// Wrap modes.
		for (int minFilterNdx = 0; minFilterNdx < DE_LENGTH_OF_ARRAY(minFilterModes); minFilterNdx++)
		{
			de::MovePtr<tcu::TestCaseGroup>	minFilterGroup(new tcu::TestCaseGroup(testCtx, minFilterModes[minFilterNdx].name, ""));

			for (int magFilterNdx = 0; magFilterNdx < DE_LENGTH_OF_ARRAY(magFilterModes); magFilterNdx++)
			{
				de::MovePtr<tcu::TestCaseGroup>	magFilterGroup(new tcu::TestCaseGroup(testCtx, magFilterModes[magFilterNdx].name, ""));

				for (int wrapSNdx = 0; wrapSNdx < DE_LENGTH_OF_ARRAY(wrapModes); wrapSNdx++)
				{
					de::MovePtr<tcu::TestCaseGroup>	wrapSGroup(new tcu::TestCaseGroup(testCtx, wrapModes[wrapSNdx].name, ""));

					for (int wrapTNdx = 0; wrapTNdx < DE_LENGTH_OF_ARRAY(wrapModes); wrapTNdx++)
					{
						const string	name		= wrapModes[wrapTNdx].name;
						Texture2DTestCaseParameters	testParameters;

						testParameters.format		= VK_FORMAT_R8G8B8A8_UNORM;
						testParameters.minFilter	= minFilterModes[minFilterNdx].mode;
						testParameters.magFilter	= magFilterModes[magFilterNdx].mode;
						testParameters.wrapS		= wrapModes[wrapSNdx].mode;
						testParameters.wrapT		= wrapModes[wrapTNdx].mode;
						testParameters.width		= 63;
						testParameters.height		= 57;

						testParameters.programs.push_back(PROGRAM_2D_FLOAT);

						wrapSGroup->addChild(new TextureTestCase<Texture2DFilteringTestInstance>(testCtx, name.c_str(), "", testParameters));
					}
					magFilterGroup->addChild(wrapSGroup.release());
				}
				minFilterGroup->addChild(magFilterGroup.release());
			}
			combinationsGroup->addChild(minFilterGroup.release());
		}

		group2D->addChild(formatsGroup.release());
		group2D->addChild(sizesGroup.release());
		group2D->addChild(combinationsGroup.release());

		textureFilteringTests->addChild(group2D.release());
	}

	// Cube map texture filtering.
	{
		de::MovePtr<tcu::TestCaseGroup>	groupCube				(new tcu::TestCaseGroup(testCtx, "cube", "Cube Map Texture Filtering"));

		de::MovePtr<tcu::TestCaseGroup>	formatsGroup			(new tcu::TestCaseGroup(testCtx, "formats", "2D Texture Formats"));
		de::MovePtr<tcu::TestCaseGroup>	sizesGroup				(new tcu::TestCaseGroup(testCtx, "sizes", "Texture Sizes"));
		de::MovePtr<tcu::TestCaseGroup>	combinationsGroup		(new tcu::TestCaseGroup(testCtx, "combinations", "Filter and wrap mode combinations"));
		de::MovePtr<tcu::TestCaseGroup>	onlyFaceInteriorGroup	(new tcu::TestCaseGroup(testCtx, "no_edges_visible", "Don't sample anywhere near a face's edges"));

		// Formats.
		for (int fmtNdx = 0; fmtNdx < DE_LENGTH_OF_ARRAY(filterableFormatsByType); fmtNdx++)
		{
			const string					filterGroupName = filterableFormatsByType[fmtNdx].name;
			de::MovePtr<tcu::TestCaseGroup>	filterGroup		(new tcu::TestCaseGroup(testCtx, filterGroupName.c_str(), ""));

			for (int filterNdx = 0; filterNdx < DE_LENGTH_OF_ARRAY(minFilterModes); filterNdx++)
			{
				const Sampler::FilterMode				minFilter	= minFilterModes[filterNdx].mode;
				const bool								isMipmap	= minFilter != Sampler::NEAREST && minFilter != Sampler::LINEAR;
				const string							name		= minFilterModes[filterNdx].name;
				TextureCubeFilteringTestCaseParameters	testParameters;

				testParameters.format					= filterableFormatsByType[fmtNdx].format;
				testParameters.minFilter				= minFilter;
				testParameters.magFilter				= isMipmap ? Sampler::LINEAR : minFilter;

				testParameters.wrapS					= Sampler::REPEAT_GL;
				testParameters.wrapT					= Sampler::REPEAT_GL;
				testParameters.onlySampleFaceInterior	= false;
				testParameters.size						= 64;

				testParameters.programs.push_back(PROGRAM_CUBE_FLOAT);
				testParameters.programs.push_back(PROGRAM_CUBE_UINT);

				// Some tests have to be skipped due to the restrictions of the verifiers.
				if (verifierCanBeUsed(testParameters.format, testParameters.minFilter, testParameters.magFilter))
				{
					filterGroup->addChild(new TextureTestCase<TextureCubeFilteringTestInstance>(testCtx, name.c_str(), "", testParameters));
				}
			}
			formatsGroup->addChild(filterGroup.release());
		}

		// Sizes.
		for (int sizeNdx = 0; sizeNdx < DE_LENGTH_OF_ARRAY(sizesCube); sizeNdx++)
		{
			const string					filterGroupName = de::toString(sizesCube[sizeNdx].size) + "x" + de::toString(sizesCube[sizeNdx].size);
			de::MovePtr<tcu::TestCaseGroup>	filterGroup		(new tcu::TestCaseGroup(testCtx, filterGroupName.c_str(), ""));

			for (int filterNdx = 0; filterNdx < DE_LENGTH_OF_ARRAY(minFilterModes); filterNdx++)
			{
				const Sampler::FilterMode				minFilter		= minFilterModes[filterNdx].mode;
				const bool								isMipmap		= minFilter != Sampler::NEAREST && minFilter != Sampler::LINEAR;
				const string							name			= minFilterModes[filterNdx].name;
				TextureCubeFilteringTestCaseParameters	testParameters;

				testParameters.format					= VK_FORMAT_R8G8B8A8_UNORM;
				testParameters.minFilter				= minFilter;
				testParameters.magFilter				= isMipmap ? Sampler::LINEAR : minFilter;
				testParameters.wrapS					= Sampler::REPEAT_GL;
				testParameters.wrapT					= Sampler::REPEAT_GL;
				testParameters.onlySampleFaceInterior	= false;
				testParameters.size						= sizesCube[sizeNdx].size;

				testParameters.programs.push_back(PROGRAM_CUBE_FLOAT);

				filterGroup->addChild(new TextureTestCase<TextureCubeFilteringTestInstance>(testCtx, name.c_str(), "", testParameters));

			}
			sizesGroup->addChild(filterGroup.release());
		}

		// Filter/wrap mode combinations.
		for (int minFilterNdx = 0; minFilterNdx < DE_LENGTH_OF_ARRAY(minFilterModes); minFilterNdx++)
		{
			de::MovePtr<tcu::TestCaseGroup>	minFilterGroup(new tcu::TestCaseGroup(testCtx, minFilterModes[minFilterNdx].name, ""));

			for (int magFilterNdx = 0; magFilterNdx < DE_LENGTH_OF_ARRAY(magFilterModes); magFilterNdx++)
			{
				de::MovePtr<tcu::TestCaseGroup>	magFilterGroup(new tcu::TestCaseGroup(testCtx, magFilterModes[magFilterNdx].name, ""));

				for (int wrapSNdx = 0; wrapSNdx < DE_LENGTH_OF_ARRAY(wrapModes); wrapSNdx++)
				{
					de::MovePtr<tcu::TestCaseGroup>	wrapSGroup(new tcu::TestCaseGroup(testCtx, wrapModes[wrapSNdx].name, ""));

					for (int wrapTNdx = 0; wrapTNdx < DE_LENGTH_OF_ARRAY(wrapModes); wrapTNdx++)
					{
						const string							name			= wrapModes[wrapTNdx].name;
						TextureCubeFilteringTestCaseParameters	testParameters;

						testParameters.format					= VK_FORMAT_R8G8B8A8_UNORM;
						testParameters.minFilter				= minFilterModes[minFilterNdx].mode;
						testParameters.magFilter				= magFilterModes[magFilterNdx].mode;
						testParameters.wrapS					= wrapModes[wrapSNdx].mode;
						testParameters.wrapT					= wrapModes[wrapTNdx].mode;
						testParameters.onlySampleFaceInterior	= false;
						testParameters.size						= 63;

						testParameters.programs.push_back(PROGRAM_CUBE_FLOAT);

						wrapSGroup->addChild(new TextureTestCase<TextureCubeFilteringTestInstance>(testCtx, name.c_str(), "", testParameters));
					}
					magFilterGroup->addChild(wrapSGroup.release());
				}
				minFilterGroup->addChild(magFilterGroup.release());
			}
			combinationsGroup->addChild(minFilterGroup.release());
		}

		// Cases with no visible cube edges.
		for (int isLinearI = 0; isLinearI <= 1; isLinearI++)
		{
			const bool								isLinear		= isLinearI != 0;
			const string							name			= isLinear ? "linear" : "nearest";
			TextureCubeFilteringTestCaseParameters	testParameters;

			testParameters.format					= VK_FORMAT_R8G8B8A8_UNORM;
			testParameters.minFilter				= isLinear ? Sampler::LINEAR : Sampler::NEAREST;
			testParameters.magFilter				= isLinear ? Sampler::LINEAR : Sampler::NEAREST;
			testParameters.wrapS					= Sampler::REPEAT_GL;
			testParameters.wrapT					= Sampler::REPEAT_GL;
			testParameters.onlySampleFaceInterior	= true;
			testParameters.size						= 63;

			testParameters.programs.push_back(PROGRAM_CUBE_FLOAT);

			onlyFaceInteriorGroup->addChild(new TextureTestCase<TextureCubeFilteringTestInstance>(testCtx, name.c_str(), "", testParameters));
		}

		groupCube->addChild(formatsGroup.release());
		groupCube->addChild(sizesGroup.release());
		groupCube->addChild(combinationsGroup.release());
		groupCube->addChild(onlyFaceInteriorGroup.release());

		textureFilteringTests->addChild(groupCube.release());
	}

	// 2D array texture filtering.
	{
		de::MovePtr<tcu::TestCaseGroup>	group2DArray		(new tcu::TestCaseGroup(testCtx, "2d_array", "2D Array Texture Filtering"));

		de::MovePtr<tcu::TestCaseGroup>	formatsGroup		(new tcu::TestCaseGroup(testCtx, "formats", "2D Array Texture Formats"));
		de::MovePtr<tcu::TestCaseGroup>	sizesGroup			(new tcu::TestCaseGroup(testCtx, "sizes", "Texture Sizes"));
		de::MovePtr<tcu::TestCaseGroup>	combinationsGroup	(new tcu::TestCaseGroup(testCtx, "combinations", "Filter and wrap mode combinations"));

		// Formats.
		for (int fmtNdx = 0; fmtNdx < DE_LENGTH_OF_ARRAY(filterableFormatsByType); fmtNdx++)
		{
			const string					filterGroupName = filterableFormatsByType[fmtNdx].name;
			de::MovePtr<tcu::TestCaseGroup>	filterGroup		(new tcu::TestCaseGroup(testCtx, filterGroupName.c_str(), ""));

			for (int filterNdx = 0; filterNdx < DE_LENGTH_OF_ARRAY(minFilterModes); filterNdx++)
			{
				const Sampler::FilterMode			minFilter		= minFilterModes[filterNdx].mode;
				const char* const					filterName		= minFilterModes[filterNdx].name;
				const bool							isMipmap		= minFilter != Sampler::NEAREST && minFilter != Sampler::LINEAR;
				const char* const					formatName		= filterableFormatsByType[fmtNdx].name;
				const string						name			= string(formatName) + "_" + filterName;
				Texture2DArrayTestCaseParameters	testParameters;

				testParameters.format		= filterableFormatsByType[fmtNdx].format;
				testParameters.minFilter	= minFilter;
				testParameters.magFilter	= isMipmap ? Sampler::LINEAR : minFilter;

				testParameters.wrapS		= Sampler::REPEAT_GL;
				testParameters.wrapT		= Sampler::REPEAT_GL;
				testParameters.width		= 128;
				testParameters.height		= 128;
				testParameters.numLayers	= 8;

				testParameters.programs.push_back(PROGRAM_2D_ARRAY_FLOAT);
				testParameters.programs.push_back(PROGRAM_2D_ARRAY_UINT);

				// Some tests have to be skipped due to the restrictions of the verifiers.
				if (verifierCanBeUsed(testParameters.format, testParameters.minFilter, testParameters.magFilter))
				{
					filterGroup->addChild(new TextureTestCase<Texture2DArrayFilteringTestInstance>(testCtx, name.c_str(), "", testParameters));
				}
			}
			formatsGroup->addChild(filterGroup.release());
		}

		// Sizes.
		for (int sizeNdx = 0; sizeNdx < DE_LENGTH_OF_ARRAY(sizes2DArray); sizeNdx++)
		{
			const string					filterGroupName = de::toString(sizes2DArray[sizeNdx].width) + "x" + de::toString(sizes2DArray[sizeNdx].height) + "x" + de::toString(sizes2DArray[sizeNdx].numLayers);
			de::MovePtr<tcu::TestCaseGroup>	filterGroup		(new tcu::TestCaseGroup(testCtx, filterGroupName.c_str(), ""));

			for (int filterNdx = 0; filterNdx < DE_LENGTH_OF_ARRAY(minFilterModes); filterNdx++)
			{
				const Sampler::FilterMode			minFilter		= minFilterModes[filterNdx].mode;
				const char* const					filterName		= minFilterModes[filterNdx].name;
				const bool							isMipmap		= minFilter != Sampler::NEAREST && minFilter != Sampler::LINEAR;
				const string						name			= filterName;
				Texture2DArrayTestCaseParameters	testParameters;

				testParameters.format		= VK_FORMAT_R8G8B8A8_UNORM;
				testParameters.minFilter	= minFilter;
				testParameters.magFilter	= isMipmap ? Sampler::LINEAR : minFilter;
				testParameters.wrapS		= Sampler::REPEAT_GL;
				testParameters.wrapT		= Sampler::REPEAT_GL;
				testParameters.width		= sizes2DArray[sizeNdx].width;
				testParameters.height		= sizes2DArray[sizeNdx].height;
				testParameters.numLayers	= sizes2DArray[sizeNdx].numLayers;

				testParameters.programs.push_back(PROGRAM_2D_ARRAY_FLOAT);

				filterGroup->addChild(new TextureTestCase<Texture2DArrayFilteringTestInstance>(testCtx, name.c_str(), "", testParameters));
			}
			sizesGroup->addChild(filterGroup.release());
		}

		// Wrap modes.
		for (int minFilterNdx = 0; minFilterNdx < DE_LENGTH_OF_ARRAY(minFilterModes); minFilterNdx++)
		{
			de::MovePtr<tcu::TestCaseGroup>	minFilterGroup(new tcu::TestCaseGroup(testCtx, minFilterModes[minFilterNdx].name, ""));

			for (int magFilterNdx = 0; magFilterNdx < DE_LENGTH_OF_ARRAY(magFilterModes); magFilterNdx++)
			{
				de::MovePtr<tcu::TestCaseGroup>	magFilterGroup(new tcu::TestCaseGroup(testCtx, magFilterModes[magFilterNdx].name, ""));

				for (int wrapSNdx = 0; wrapSNdx < DE_LENGTH_OF_ARRAY(wrapModes); wrapSNdx++)
				{
					de::MovePtr<tcu::TestCaseGroup>	wrapSGroup(new tcu::TestCaseGroup(testCtx, wrapModes[wrapSNdx].name, ""));

					for (int wrapTNdx = 0; wrapTNdx < DE_LENGTH_OF_ARRAY(wrapModes); wrapTNdx++)
					{
						const string						name			= wrapModes[wrapTNdx].name;
						Texture2DArrayTestCaseParameters	testParameters;

						testParameters.format		= VK_FORMAT_R8G8B8A8_UNORM;
						testParameters.minFilter	= minFilterModes[minFilterNdx].mode;
						testParameters.magFilter	= magFilterModes[magFilterNdx].mode;
						testParameters.wrapS		= wrapModes[wrapSNdx].mode;
						testParameters.wrapT		= wrapModes[wrapTNdx].mode;
						testParameters.width		= 123;
						testParameters.height		= 107;
						testParameters.numLayers	= 7;

						testParameters.programs.push_back(PROGRAM_2D_ARRAY_FLOAT);

						wrapSGroup->addChild(new TextureTestCase<Texture2DArrayFilteringTestInstance>(testCtx, name.c_str(), "", testParameters));
					}
					magFilterGroup->addChild(wrapSGroup.release());
				}
				minFilterGroup->addChild(magFilterGroup.release());
			}
			combinationsGroup->addChild(minFilterGroup.release());
		}

		group2DArray->addChild(formatsGroup.release());
		group2DArray->addChild(sizesGroup.release());
		group2DArray->addChild(combinationsGroup.release());

		textureFilteringTests->addChild(group2DArray.release());
	}

	// 3D texture filtering.
	{
		de::MovePtr<tcu::TestCaseGroup>	group3D				(new tcu::TestCaseGroup(testCtx, "3d", "3D Texture Filtering"));

		de::MovePtr<tcu::TestCaseGroup>	formatsGroup		(new tcu::TestCaseGroup(testCtx, "formats", "3D Texture Formats"));
		de::MovePtr<tcu::TestCaseGroup>	sizesGroup			(new tcu::TestCaseGroup(testCtx, "sizes", "Texture Sizes"));
		de::MovePtr<tcu::TestCaseGroup>	combinationsGroup	(new tcu::TestCaseGroup(testCtx, "combinations", "Filter and wrap mode combinations"));

		// Formats.
		for (int fmtNdx = 0; fmtNdx < DE_LENGTH_OF_ARRAY(filterableFormatsByType); fmtNdx++)
		{
			const string					filterGroupName = filterableFormatsByType[fmtNdx].name;
			de::MovePtr<tcu::TestCaseGroup>	filterGroup		(new tcu::TestCaseGroup(testCtx, filterGroupName.c_str(), ""));

			for (int filterNdx = 0; filterNdx < DE_LENGTH_OF_ARRAY(minFilterModes); filterNdx++)
			{
				const Sampler::FilterMode	minFilter		= minFilterModes[filterNdx].mode;
				const char* const			filterName		= minFilterModes[filterNdx].name;
				const bool					isMipmap		= minFilter != Sampler::NEAREST && minFilter != Sampler::LINEAR;
				const char* const			formatName		= filterableFormatsByType[fmtNdx].name;
				const string				name			= string(formatName) + "_" + filterName;
				Texture3DTestCaseParameters	testParameters;

				testParameters.format		= filterableFormatsByType[fmtNdx].format;
				testParameters.minFilter	= minFilter;
				testParameters.magFilter	= isMipmap ? Sampler::LINEAR : minFilter;

				testParameters.wrapS		= Sampler::REPEAT_GL;
				testParameters.wrapT		= Sampler::REPEAT_GL;
				testParameters.wrapR		= Sampler::REPEAT_GL;
				testParameters.width		= 64;
				testParameters.height		= 64;
				testParameters.depth		= 64;

				testParameters.programs.push_back(PROGRAM_3D_FLOAT);
				testParameters.programs.push_back(PROGRAM_3D_UINT);

				// Some tests have to be skipped due to the restrictions of the verifiers.
				if (verifierCanBeUsed(testParameters.format, testParameters.minFilter, testParameters.magFilter))
				{
					filterGroup->addChild(new TextureTestCase<Texture3DFilteringTestInstance>(testCtx, name.c_str(), "", testParameters));
				}
			}
			formatsGroup->addChild(filterGroup.release());
		}

		// Sizes.
		for (int sizeNdx = 0; sizeNdx < DE_LENGTH_OF_ARRAY(sizes3D); sizeNdx++)
		{
			const string					filterGroupName = de::toString(sizes3D[sizeNdx].width) + "x" + de::toString(sizes3D[sizeNdx].height) + "x" + de::toString(sizes3D[sizeNdx].depth);
			de::MovePtr<tcu::TestCaseGroup>	filterGroup		(new tcu::TestCaseGroup(testCtx, filterGroupName.c_str(), ""));

			for (int filterNdx = 0; filterNdx < DE_LENGTH_OF_ARRAY(minFilterModes); filterNdx++)
			{
				const Sampler::FilterMode		minFilter		= minFilterModes[filterNdx].mode;
				const char* const				filterName		= minFilterModes[filterNdx].name;
				const bool						isMipmap		= minFilter != Sampler::NEAREST && minFilter != Sampler::LINEAR;
				const string					name			= filterName;
				Texture3DTestCaseParameters		testParameters;

				testParameters.format		= VK_FORMAT_R8G8B8A8_UNORM;
				testParameters.minFilter	= minFilter;
				testParameters.magFilter	= isMipmap ? Sampler::LINEAR : minFilter;
				testParameters.wrapS		= Sampler::REPEAT_GL;
				testParameters.wrapT		= Sampler::REPEAT_GL;
				testParameters.wrapR		= Sampler::REPEAT_GL;
				testParameters.width		= sizes3D[sizeNdx].width;
				testParameters.height		= sizes3D[sizeNdx].height;
				testParameters.depth		= sizes3D[sizeNdx].depth;

				testParameters.programs.push_back(PROGRAM_3D_FLOAT);

				filterGroup->addChild(new TextureTestCase<Texture3DFilteringTestInstance>(testCtx, name.c_str(), "", testParameters));
			}
			sizesGroup->addChild(filterGroup.release());
		}

		// Wrap modes.
		for (int minFilterNdx = 0; minFilterNdx < DE_LENGTH_OF_ARRAY(minFilterModes); minFilterNdx++)
		{
			de::MovePtr<tcu::TestCaseGroup>	minFilterGroup(new tcu::TestCaseGroup(testCtx, minFilterModes[minFilterNdx].name, ""));

			for (int magFilterNdx = 0; magFilterNdx < DE_LENGTH_OF_ARRAY(magFilterModes); magFilterNdx++)
			{
				de::MovePtr<tcu::TestCaseGroup>	magFilterGroup(new tcu::TestCaseGroup(testCtx, magFilterModes[magFilterNdx].name, ""));

				for (int wrapSNdx = 0; wrapSNdx < DE_LENGTH_OF_ARRAY(wrapModes); wrapSNdx++)
				{
					de::MovePtr<tcu::TestCaseGroup>	wrapSGroup(new tcu::TestCaseGroup(testCtx, wrapModes[wrapSNdx].name, ""));

					for (int wrapTNdx = 0; wrapTNdx < DE_LENGTH_OF_ARRAY(wrapModes); wrapTNdx++)
					{
						de::MovePtr<tcu::TestCaseGroup>	wrapTGroup(new tcu::TestCaseGroup(testCtx, wrapModes[wrapTNdx].name, ""));

						for (int wrapRNdx = 0; wrapRNdx < DE_LENGTH_OF_ARRAY(wrapModes); wrapRNdx++)
						{
							const string				name			= wrapModes[wrapRNdx].name;
							Texture3DTestCaseParameters	testParameters;

							testParameters.format		= VK_FORMAT_R8G8B8A8_UNORM;
							testParameters.minFilter	= minFilterModes[minFilterNdx].mode;
							testParameters.magFilter	= magFilterModes[magFilterNdx].mode;
							testParameters.wrapS		= wrapModes[wrapSNdx].mode;
							testParameters.wrapT		= wrapModes[wrapTNdx].mode;
							testParameters.wrapR		= wrapModes[wrapRNdx].mode;
							testParameters.width		= 63;
							testParameters.height		= 57;
							testParameters.depth		= 67;

							testParameters.programs.push_back(PROGRAM_3D_FLOAT);

							wrapTGroup->addChild(new TextureTestCase<Texture3DFilteringTestInstance>(testCtx, name.c_str(), "", testParameters));
						}
						wrapSGroup->addChild(wrapTGroup.release());
					}
					magFilterGroup->addChild(wrapSGroup.release());
				}
				minFilterGroup->addChild(magFilterGroup.release());
			}
			combinationsGroup->addChild(minFilterGroup.release());
		}

		group3D->addChild(formatsGroup.release());
		group3D->addChild(sizesGroup.release());
		group3D->addChild(combinationsGroup.release());

		textureFilteringTests->addChild(group3D.release());
	}
}

} // anonymous

tcu::TestCaseGroup*	createTextureFilteringTests	(tcu::TestContext& testCtx)
{
	return createTestGroup(testCtx, "filtering", "Texture filtering tests.", populateTextureFilteringTests);
}

} // texture
} // vkt
