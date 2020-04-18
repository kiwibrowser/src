/*-------------------------------------------------------------------------
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
 * \brief Compressed texture tests.
 *//*--------------------------------------------------------------------*/

#include "vktTextureCompressedFormatTests.hpp"

#include "deString.h"
#include "deStringUtil.hpp"
#include "tcuCompressedTexture.hpp"
#include "tcuTexture.hpp"
#include "tcuTextureUtil.hpp"
#include "vkImageUtil.hpp"
#include "vktTestGroupUtil.hpp"
#include "vktTextureTestUtil.hpp"
#include <string>
#include <vector>

namespace vkt
{
namespace texture
{
namespace
{

using namespace vk;
using namespace glu::TextureTestUtil;
using namespace texture::util;

using std::string;
using std::vector;
using tcu::Sampler;
using tcu::TestLog;

struct Compressed2DTestParameters : public Texture2DTestCaseParameters
{
};

class Compressed2DTestInstance : public TestInstance
{
public:
	typedef Compressed2DTestParameters	ParameterType;

										Compressed2DTestInstance	(Context&				context,
																	 const ParameterType&	testParameters);
	tcu::TestStatus						iterate						(void);

private:
										Compressed2DTestInstance	(const Compressed2DTestInstance& other);
	Compressed2DTestInstance&			operator=					(const Compressed2DTestInstance& other);

	const ParameterType&				m_testParameters;
	const tcu::CompressedTexFormat		m_compressedFormat;
	TestTexture2DSp						m_texture;
	TextureRenderer						m_renderer;
};

Compressed2DTestInstance::Compressed2DTestInstance (Context&				context,
													const ParameterType&	testParameters)
	: TestInstance			(context)
	, m_testParameters		(testParameters)
	, m_compressedFormat	(mapVkCompressedFormat(testParameters.format))
	, m_texture				(TestTexture2DSp(new pipeline::TestTexture2D(m_compressedFormat, testParameters.width, testParameters.height)))
	, m_renderer			(context, testParameters.sampleCount, testParameters.width, testParameters.height)
{
	m_renderer.add2DTexture(m_texture);
}

tcu::TestStatus Compressed2DTestInstance::iterate (void)
{
	tcu::TestLog&					log				= m_context.getTestContext().getLog();
	const pipeline::TestTexture2D&	texture			= m_renderer.get2DTexture(0);
	const tcu::TextureFormat		textureFormat	= texture.getTextureFormat();
	const tcu::TextureFormatInfo	formatInfo		= tcu::getTextureFormatInfo(textureFormat);

	ReferenceParams					sampleParams	(TEXTURETYPE_2D);
	tcu::Surface					rendered		(m_renderer.getRenderWidth(), m_renderer.getRenderHeight());
	vector<float>					texCoord;

	// Setup params for reference.
	sampleParams.sampler			= util::createSampler(m_testParameters.wrapS, m_testParameters.wrapT, m_testParameters.minFilter, m_testParameters.magFilter);
	sampleParams.samplerType		= SAMPLERTYPE_FLOAT;
	sampleParams.lodMode			= LODMODE_EXACT;
	sampleParams.colorBias			= formatInfo.lookupBias;
	sampleParams.colorScale			= formatInfo.lookupScale;

	log << TestLog::Message << "Compare reference value = " << sampleParams.ref << TestLog::EndMessage;

	// Compute texture coordinates.
	computeQuadTexCoord2D(texCoord, tcu::Vec2(0.0f, 0.0f), tcu::Vec2(1.0f, 1.0f));

	m_renderer.renderQuad(rendered, 0, &texCoord[0], sampleParams);

	// Compute reference.
	const tcu::IVec4		formatBitDepth	= getTextureFormatBitDepth(vk::mapVkFormat(VK_FORMAT_R8G8B8A8_UNORM));
	const tcu::PixelFormat	pixelFormat		(formatBitDepth[0], formatBitDepth[1], formatBitDepth[2], formatBitDepth[3]);
	tcu::Surface			referenceFrame	(m_renderer.getRenderWidth(), m_renderer.getRenderHeight());
	sampleTexture(tcu::SurfaceAccess(referenceFrame, pixelFormat), m_texture->getTexture(), &texCoord[0], sampleParams);

	// Compare and log.
	const bool isOk = compareImages(log, referenceFrame, rendered, pixelFormat.getColorThreshold() + tcu::RGBA(1, 1, 1, 1));

	return isOk ? tcu::TestStatus::pass("Pass") : tcu::TestStatus::fail("Image verification failed");
}

void populateTextureCompressedFormatTests (tcu::TestCaseGroup* compressedTextureTests)
{
	tcu::TestContext&	testCtx	= compressedTextureTests->getTestContext();

	// ETC2 and EAC compressed formats.
	const struct {
		const VkFormat	format;
	} etc2Formats[] =
	{
		{ VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK		},
		{ VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK		},
		{ VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK	},
		{ VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK	},
		{ VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK	},
		{ VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK	},

		{ VK_FORMAT_EAC_R11_UNORM_BLOCK			},
		{ VK_FORMAT_EAC_R11_SNORM_BLOCK			},
		{ VK_FORMAT_EAC_R11G11_UNORM_BLOCK		},
		{ VK_FORMAT_EAC_R11G11_SNORM_BLOCK		},
	};

	const struct {
		const int	width;
		const int	height;
		const char*	name;
	} sizes[] =
	{
		{ 128, 64, "pot"  },
		{ 51,  65, "npot" },
	};

	for (int sizeNdx = 0; sizeNdx < DE_LENGTH_OF_ARRAY(sizes); sizeNdx++)
	for (int formatNdx = 0; formatNdx < DE_LENGTH_OF_ARRAY(etc2Formats); formatNdx++)
	{
		const string	formatStr	= de::toString(getFormatStr(etc2Formats[formatNdx].format));
		const string	nameBase	= de::toLower(formatStr.substr(10));

		Compressed2DTestParameters	testParameters;
		testParameters.format		= etc2Formats[formatNdx].format;
		testParameters.width		= sizes[sizeNdx].width;
		testParameters.height		= sizes[sizeNdx].height;
		testParameters.minFilter	= tcu::Sampler::NEAREST;
		testParameters.magFilter	= tcu::Sampler::NEAREST;
		testParameters.programs.push_back(PROGRAM_2D_FLOAT);

		compressedTextureTests->addChild(new TextureTestCase<Compressed2DTestInstance>(testCtx, (nameBase + "_2d_" + sizes[sizeNdx].name).c_str(), (formatStr + ", TEXTURETYPE_2D").c_str(), testParameters));
	}
}

} // anonymous

tcu::TestCaseGroup* createTextureCompressedFormatTests (tcu::TestContext& testCtx)
{
	return createTestGroup(testCtx, "compressed", "Texture compressed format tests.", populateTextureCompressedFormatTests);
}

} // texture
} // vkt
