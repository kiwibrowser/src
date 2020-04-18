/*-------------------------------------------------------------------------
 * Vulkan Conformance Tests
 * ------------------------
 *
 * Copyright (c) 2016 The Khronos Group Inc.
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
 * \brief Texture filtering anisotropy tests
 *//*--------------------------------------------------------------------*/

#include "vktTextureFilteringAnisotropyTests.hpp"

#include "vktTextureTestUtil.hpp"
#include "vkImageUtil.hpp"
#include "vkQueryUtil.hpp"
#include "tcuImageCompare.hpp"
#include <vector>

using namespace vk;

namespace vkt
{
namespace texture
{

using std::string;
using std::vector;
using std::max;
using std::min;
using tcu::Vec2;
using tcu::Vec4;
using tcu::Sampler;
using tcu::Surface;
using tcu::TextureFormat;
using namespace texture::util;
using namespace glu::TextureTestUtil;

namespace
{
static const deUint32 ANISOTROPY_TEST_RESOLUTION = 128u;

struct AnisotropyParams : public ReferenceParams
{
	AnisotropyParams	(const TextureType			texType_,
						 const float				maxAnisotropy_,
						 const Sampler::FilterMode	minFilter_,
						 const Sampler::FilterMode	magFilter_,
						 const bool					mipMap_ = false)
		: ReferenceParams	(texType_)
		, maxAnisotropy		(maxAnisotropy_)
		, minFilter			(minFilter_)
		, magFilter			(magFilter_)
		, mipMap			(mipMap_)
	{
	}

	float				maxAnisotropy;
	Sampler::FilterMode	minFilter;
	Sampler::FilterMode	magFilter;
	bool				mipMap;
};

class FilteringAnisotropyInstance : public vkt::TestInstance
{
public:
	FilteringAnisotropyInstance	(Context& context, const AnisotropyParams& refParams)
		: vkt::TestInstance	(context)
		, m_refParams		(refParams)
	{
		// Sampling parameters.
		m_refParams.sampler			= util::createSampler(Sampler::CLAMP_TO_EDGE, Sampler::CLAMP_TO_EDGE, m_refParams.minFilter, m_refParams.magFilter);
		m_refParams.samplerType		= getSamplerType(vk::mapVkFormat(VK_FORMAT_R8G8B8A8_UNORM));
		m_refParams.flags			= 0u;
		m_refParams.lodMode			= LODMODE_EXACT;
		m_refParams.maxAnisotropy	= min(getPhysicalDeviceProperties(m_context.getInstanceInterface(), m_context.getPhysicalDevice()).limits.maxSamplerAnisotropy, m_refParams.maxAnisotropy);

		if (m_refParams.mipMap)
		{
			m_refParams.maxLevel	= deLog2Floor32(ANISOTROPY_TEST_RESOLUTION);
			m_refParams.minLod		= 0.0f;
			m_refParams.maxLod		= static_cast<float>(m_refParams.maxLevel);
		}
		else
			m_refParams.maxLevel	= 0;
	}

	tcu::TestStatus	iterate	(void)
	{
		// Check device for anisotropic filtering support
		if (!m_context.getDeviceFeatures().samplerAnisotropy)
			TCU_THROW(NotSupportedError, "Skipping anisotropic tests since the device does not support anisotropic filtering.");

		TextureRenderer	renderer	(m_context, VK_SAMPLE_COUNT_1_BIT, ANISOTROPY_TEST_RESOLUTION, ANISOTROPY_TEST_RESOLUTION);
		TestTexture2DSp	texture		= TestTexture2DSp(new pipeline::TestTexture2D(vk::mapVkFormat(VK_FORMAT_R8G8B8A8_UNORM), ANISOTROPY_TEST_RESOLUTION, ANISOTROPY_TEST_RESOLUTION));

		for (int levelNdx = 0; levelNdx < m_refParams.maxLevel + 1; levelNdx++)
		{
			const int gridSize = max(texture->getLevel(levelNdx, 0).getHeight() / 8, 1);
			tcu::fillWithGrid(texture->getLevel(levelNdx, 0), gridSize, Vec4(0.0f, 0.0f, 0.0f, 1.0f), Vec4(1.0f));
		}

		renderer.setViewport(0.0f, 0.0f, static_cast<float>(ANISOTROPY_TEST_RESOLUTION), static_cast<float>(ANISOTROPY_TEST_RESOLUTION));
		renderer.add2DTexture(texture);

		{
			Surface			renderedFrame			(ANISOTROPY_TEST_RESOLUTION, ANISOTROPY_TEST_RESOLUTION);
			Surface			renderedAnisotropyFrame	(ANISOTROPY_TEST_RESOLUTION, ANISOTROPY_TEST_RESOLUTION);
			const float		position[]				=
			{
				-3.5f, -1.0f, 0.0f, 3.5f,
				-3.5f, +1.0f, 0.0f, 1.0f,
				+3.5f, -1.0f, 0.0f, 3.5f,
				+3.5f, +1.0f, 0.0f, 1.0f
			};
			vector<float>	texCoord;

			computeQuadTexCoord2D(texCoord, Vec2(0.0f), Vec2(1.0f));

			renderer.renderQuad(renderedFrame,				position, 0, &texCoord[0], m_refParams, 1.0f);
			renderer.renderQuad(renderedAnisotropyFrame,	position, 0, &texCoord[0], m_refParams, m_refParams.maxAnisotropy);

			if (!tcu::fuzzyCompare(m_context.getTestContext().getLog(), "Expecting comparison to pass", "Expecting comparison to pass",
					renderedFrame.getAccess(), renderedAnisotropyFrame.getAccess(), 0.05f, tcu::COMPARE_LOG_RESULT))
				return tcu::TestStatus::fail("Fail");

			// Anisotropic filtering is implementation dependent. Expecting differences with minification/magnification filter set to NEAREST is too strict.
			if (m_refParams.minFilter != tcu::Sampler::NEAREST && m_refParams.magFilter != tcu::Sampler::NEAREST)
			{
				if (floatThresholdCompare (m_context.getTestContext().getLog(), "Expecting comparison to fail", "Expecting comparison to fail",
							   renderedFrame.getAccess(), renderedAnisotropyFrame.getAccess(), Vec4(0.05f), tcu::COMPARE_LOG_RESULT))
					return tcu::TestStatus::fail("Fail");
			}
		}
		return tcu::TestStatus::pass("Pass");
	}

private:
	 AnisotropyParams m_refParams;
};

class FilteringAnisotropyTests : public vkt::TestCase
{
public:
					FilteringAnisotropyTests	(tcu::TestContext&			testCtx,
												 const string&				name,
												 const string&				description,
												 const AnisotropyParams&	refParams)
		: vkt::TestCase		(testCtx, name, description)
		, m_refParams		(refParams)
	{
	}

	void			initPrograms				(SourceCollections&	programCollection) const
	{
		std::vector<util::Program>	programs;
		programs.push_back(util::PROGRAM_2D_FLOAT);
		initializePrograms(programCollection,glu::PRECISION_HIGHP, programs);
	}

	TestInstance*	createInstance				(Context&	context) const
	{
		return new FilteringAnisotropyInstance(context, m_refParams);
	}
private :
	const AnisotropyParams	m_refParams;
};

} // anonymous

tcu::TestCaseGroup* createFilteringAnisotropyTests (tcu::TestContext& testCtx)
{
	de::MovePtr<tcu::TestCaseGroup>	filteringAnisotropyTests	(new tcu::TestCaseGroup(testCtx,	"filtering_anisotropy",	"Filtering anisotropy tests"));
	de::MovePtr<tcu::TestCaseGroup>	basicTests					(new tcu::TestCaseGroup(testCtx, "basic", "Filtering anisotropy tests"));
	de::MovePtr<tcu::TestCaseGroup>	mipmapTests					(new tcu::TestCaseGroup(testCtx, "mipmap", "Filtering anisotropy tests"));
	const char*						valueName[]					=
	{
		"anisotropy_2",
		"anisotropy_4",
		"anisotropy_8",
		"anisotropy_max"
	};
	const float						maxAnisotropy[]				=
	{
		2.0f,
		4.0f,
		8.0f,
		10000.0f	//too huge will be flated to max value
	};
	const char*						magFilterName[]				=
	{
		"nearest",
		"linear"
	};
	const tcu::Sampler::FilterMode	magFilters[]				=
	{
		Sampler::NEAREST,
		Sampler::LINEAR
	};

	{
		const tcu::Sampler::FilterMode*	minFilters		= magFilters;
		const char**					minFilterName	= magFilterName;

		for (int anisotropyNdx = 0; anisotropyNdx < DE_LENGTH_OF_ARRAY(maxAnisotropy); anisotropyNdx++)
		{
			de::MovePtr<tcu::TestCaseGroup> levelAnisotropyGroups (new tcu::TestCaseGroup(testCtx, valueName[anisotropyNdx], "Filtering anisotropy tests"));

			for (int minFilterNdx = 0; minFilterNdx < DE_LENGTH_OF_ARRAY(magFilters); minFilterNdx++)
			for (int magFilterNdx = 0; magFilterNdx < DE_LENGTH_OF_ARRAY(magFilters); magFilterNdx++)
			{
				AnisotropyParams	refParams	(TEXTURETYPE_2D, maxAnisotropy[anisotropyNdx] ,minFilters[minFilterNdx], magFilters[magFilterNdx]);
				levelAnisotropyGroups->addChild(new FilteringAnisotropyTests(testCtx,
														"mag_" + string(magFilterName[magFilterNdx]) + "_min_" + string(minFilterName[minFilterNdx]),
														"Texture filtering anisotropy basic tests", refParams));
			}
			basicTests->addChild(levelAnisotropyGroups.release());
		}
		filteringAnisotropyTests->addChild(basicTests.release());
	}

	{
		const tcu::Sampler::FilterMode		minFilters[]	=
		{
			Sampler::NEAREST_MIPMAP_NEAREST,
			Sampler::NEAREST_MIPMAP_LINEAR,
			Sampler::LINEAR_MIPMAP_NEAREST,
			Sampler::LINEAR_MIPMAP_LINEAR
		};
		const char*							minFilterName[]	=
		{
			"nearest_mipmap_nearest",
			"nearest_mipmap_linear",
			"linear_mipmap_nearest",
			"linear_mipmap_linear"
		};

		for (int anisotropyNdx = 0; anisotropyNdx < DE_LENGTH_OF_ARRAY(maxAnisotropy); anisotropyNdx++)
		{
			de::MovePtr<tcu::TestCaseGroup> levelAnisotropyGroups (new tcu::TestCaseGroup(testCtx, valueName[anisotropyNdx], "Filtering anisotropy tests"));

			for (int minFilterNdx = 0; minFilterNdx < DE_LENGTH_OF_ARRAY(minFilters); minFilterNdx++)
			for (int magFilterNdx = 0; magFilterNdx < DE_LENGTH_OF_ARRAY(magFilters); magFilterNdx++)
			{
				AnisotropyParams	refParams	(TEXTURETYPE_2D, maxAnisotropy[anisotropyNdx] ,minFilters[minFilterNdx], magFilters[magFilterNdx], true);
				levelAnisotropyGroups->addChild(new FilteringAnisotropyTests(testCtx,
														"mag_" + string(magFilterName[magFilterNdx]) + "_min_" + string(minFilterName[minFilterNdx]),
														"Texture filtering anisotropy basic tests", refParams));
			}
			mipmapTests->addChild(levelAnisotropyGroups.release());
		}
		filteringAnisotropyTests->addChild(mipmapTests.release());
	}

	return filteringAnisotropyTests.release();
}

} // texture
} // vkt
