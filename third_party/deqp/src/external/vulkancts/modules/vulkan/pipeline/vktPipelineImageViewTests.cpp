/*------------------------------------------------------------------------
 * Vulkan Conformance Tests
 * ------------------------
 *
 * Copyright (c) 2015 The Khronos Group Inc.
 * Copyright (c) 2015 Imagination Technologies Ltd.
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
 * \brief Image View Tests
 *//*--------------------------------------------------------------------*/

#include "vktPipelineImageViewTests.hpp"
#include "vktPipelineImageSamplingInstance.hpp"
#include "vktPipelineImageUtil.hpp"
#include "vktPipelineVertexUtil.hpp"
#include "vktTestCase.hpp"
#include "vkImageUtil.hpp"
#include "vkPrograms.hpp"
#include "tcuPlatform.hpp"
#include "tcuTextureUtil.hpp"
#include "deStringUtil.hpp"
#include "deMemory.h"

#include <sstream>
#include <vector>

namespace vkt
{
namespace pipeline
{

using namespace vk;
using de::MovePtr;

namespace
{


class ImageViewTest : public vkt::TestCase
{
public:
							ImageViewTest			(tcu::TestContext&				testContext,
													 const char*					name,
													 const char*					description,
													 VkImageViewType				imageViewType,
													 VkFormat						imageFormat,
													 float							samplerLod,
													 const VkComponentMapping&		componentMapping,
													 const VkImageSubresourceRange&	subresourceRange);
	virtual					~ImageViewTest			(void) {}

	virtual void			initPrograms			(SourceCollections&				sourceCollections) const;
	virtual TestInstance*	createInstance			(Context&						context) const;
	static std::string		getGlslSamplerType		(const tcu::TextureFormat&		format,
													 VkImageViewType				type);
	static tcu::UVec2		getRenderSize			(VkImageViewType				viewType);
	static tcu::IVec3		getImageSize			(VkImageViewType				viewType);
	static int				getArraySize			(VkImageViewType				viewType);
	static int				getNumLevels			(VkImageViewType				viewType);
	static tcu::Vec4		swizzle					(tcu::Vec4						inputData,
													 VkComponentMapping				componentMapping);
private:
	VkImageViewType			m_imageViewType;
	VkFormat				m_imageFormat;
	float					m_samplerLod;
	VkComponentMapping		m_componentMapping;
	VkImageSubresourceRange	m_subresourceRange;
};

ImageViewTest::ImageViewTest (tcu::TestContext&					testContext,
							  const char*						name,
							  const char*						description,
							  VkImageViewType					imageViewType,
							  VkFormat							imageFormat,
							  float								samplerLod,
							  const VkComponentMapping&			componentMapping,
							  const VkImageSubresourceRange&	subresourceRange)

	: vkt::TestCase			(testContext, name, description)
	, m_imageViewType		(imageViewType)
	, m_imageFormat			(imageFormat)
	, m_samplerLod			(samplerLod)
	, m_componentMapping	(componentMapping)
	, m_subresourceRange	(subresourceRange)
{
}

tcu::Vec4 ImageViewTest::swizzle (tcu::Vec4 inputData, VkComponentMapping componentMapping)
{
	// array map with enum VkComponentSwizzle
	const float channelValues[] =
	{
		-1.0f,
		0.0f,
		1.0f,
		inputData.x(),
		inputData.y(),
		inputData.z(),
		inputData.w(),
		-1.0f
	};

	return tcu::Vec4(channelValues[componentMapping.r],
					 channelValues[componentMapping.g],
					 channelValues[componentMapping.b],
					 channelValues[componentMapping.a]);
}

void ImageViewTest::initPrograms (SourceCollections& sourceCollections) const
{
	std::ostringstream				vertexSrc;
	std::ostringstream				fragmentSrc;
	const char*						texCoordSwizzle	= DE_NULL;
	const tcu::TextureFormat		format			= (isCompressedFormat(m_imageFormat)) ? tcu::getUncompressedFormat(mapVkCompressedFormat(m_imageFormat))
																						  : mapVkFormat(m_imageFormat);

	tcu::Vec4						lookupScale;
	tcu::Vec4						lookupBias;

	getLookupScaleBias(m_imageFormat, lookupScale, lookupBias);

	tcu::Vec4						swizzledScale	= swizzle(lookupScale, m_componentMapping);
	tcu::Vec4						swizzledBias	= swizzle(lookupBias, m_componentMapping);

	switch (m_imageViewType)
	{
		case VK_IMAGE_VIEW_TYPE_1D:
			texCoordSwizzle = "x";
			break;
		case VK_IMAGE_VIEW_TYPE_1D_ARRAY:
		case VK_IMAGE_VIEW_TYPE_2D:
			texCoordSwizzle = "xy";
			break;
		case VK_IMAGE_VIEW_TYPE_2D_ARRAY:
		case VK_IMAGE_VIEW_TYPE_3D:
		case VK_IMAGE_VIEW_TYPE_CUBE:
			texCoordSwizzle = "xyz";
			break;
		case VK_IMAGE_VIEW_TYPE_CUBE_ARRAY:
			texCoordSwizzle = "xyzw";
			break;
		default:
			DE_ASSERT(false);
			break;
	}

	vertexSrc << "#version 440\n"
			  << "layout(location = 0) in vec4 position;\n"
			  << "layout(location = 1) in vec4 texCoords;\n"
			  << "layout(location = 0) out highp vec4 vtxTexCoords;\n"
			  << "out gl_PerVertex {\n"
			  << "	vec4 gl_Position;\n"
			  << "};\n"
			  << "void main (void)\n"
			  << "{\n"
			  << "	gl_Position = position;\n"
			  << "	vtxTexCoords = texCoords;\n"
			  << "}\n";

	fragmentSrc << "#version 440\n"
				<< "layout(set = 0, binding = 0) uniform highp " << getGlslSamplerType(format, m_imageViewType) << " texSampler;\n"
				<< "layout(location = 0) in highp vec4 vtxTexCoords;\n"
				<< "layout(location = 0) out highp vec4 fragColor;\n"
				<< "void main (void)\n"
				<< "{\n"
				<< "	fragColor = ";

	if (m_samplerLod > 0.0f)
		fragmentSrc << "textureLod(texSampler, vtxTexCoords." << texCoordSwizzle << ", " << std::fixed <<  m_samplerLod << ")";
	else
		fragmentSrc << "texture(texSampler, vtxTexCoords." << texCoordSwizzle << ")" << std::fixed;

	fragmentSrc << " * vec4" << std::scientific << swizzledScale << " + vec4" << swizzledBias << ";\n"
				<< "}\n";

	sourceCollections.glslSources.add("tex_vert") << glu::VertexSource(vertexSrc.str());
	sourceCollections.glslSources.add("tex_frag") << glu::FragmentSource(fragmentSrc.str());
}

TestInstance* ImageViewTest::createInstance (Context& context) const
{
	const tcu::UVec2				renderSize		= getRenderSize(m_imageViewType);
	const tcu::IVec3				imageSize		= getImageSize(m_imageViewType);
	const int						arraySize		= getArraySize(m_imageViewType);
	const std::vector<Vertex4Tex4>	vertices		= createTestQuadMosaic(m_imageViewType);

	const VkSamplerCreateInfo		samplerParams	=
	{
		VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,									// VkStructureType			sType;
		DE_NULL,																// const void*				pNext;
		0u,																		// VkSamplerCreateFlags		flags;
		VK_FILTER_NEAREST,														// VkFilter					magFilter;
		VK_FILTER_NEAREST,														// VkFilter					minFilter;
		VK_SAMPLER_MIPMAP_MODE_NEAREST,											// VkSamplerMipmapMode		mipmapMode;
		VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,									// VkSamplerAddressMode		addressModeU;
		VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,									// VkSamplerAddressMode		addressModeV;
		VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,									// VkSamplerAddressMode		addressModeW;
		0.0f,																	// float					mipLodBias;
		VK_FALSE,																// VkBool32					anisotropyEnable;
		1.0f,																	// float					maxAnisotropy;
		false,																	// VkBool32					compareEnable;
		VK_COMPARE_OP_NEVER,													// VkCompareOp				compareOp;
		0.0f,																	// float					minLod;
		(float)(m_subresourceRange.levelCount - 1),								// float					maxLod;
		getFormatBorderColor(BORDER_COLOR_TRANSPARENT_BLACK, m_imageFormat),	// VkBorderColor			borderColor;
		false																	// VkBool32					unnormalizedCoordinates;
	};

	return new ImageSamplingInstance(context, renderSize, m_imageViewType, m_imageFormat, imageSize, arraySize, m_componentMapping, m_subresourceRange, samplerParams, m_samplerLod, vertices);
}

std::string ImageViewTest::getGlslSamplerType (const tcu::TextureFormat& format, VkImageViewType type)
{
	std::ostringstream samplerType;

	if (tcu::getTextureChannelClass(format.type) == tcu::TEXTURECHANNELCLASS_UNSIGNED_INTEGER)
		samplerType << "u";
	else if (tcu::getTextureChannelClass(format.type) == tcu::TEXTURECHANNELCLASS_SIGNED_INTEGER)
		samplerType << "i";

	switch (type)
	{
		case VK_IMAGE_VIEW_TYPE_1D:
			samplerType << "sampler1D";
			break;

		case VK_IMAGE_VIEW_TYPE_1D_ARRAY:
			samplerType << "sampler1DArray";
			break;

		case VK_IMAGE_VIEW_TYPE_2D:
			samplerType << "sampler2D";
			break;

		case VK_IMAGE_VIEW_TYPE_2D_ARRAY:
			samplerType << "sampler2DArray";
			break;

		case VK_IMAGE_VIEW_TYPE_3D:
			samplerType << "sampler3D";
			break;

		case VK_IMAGE_VIEW_TYPE_CUBE:
			samplerType << "samplerCube";
			break;

		case VK_IMAGE_VIEW_TYPE_CUBE_ARRAY:
			samplerType << "samplerCubeArray";
			break;

		default:
			DE_FATAL("Unknown image view type");
			break;
	}

	return samplerType.str();
}

tcu::UVec2 ImageViewTest::getRenderSize (VkImageViewType viewType)
{
	if (viewType == VK_IMAGE_VIEW_TYPE_1D || viewType == VK_IMAGE_VIEW_TYPE_2D)
		return tcu::UVec2(16u, 16u);
	else
		return tcu::UVec2(16u * 3u, 16u * 2u);
}

tcu::IVec3 ImageViewTest::getImageSize (VkImageViewType viewType)
{
	switch (viewType)
	{
		case VK_IMAGE_VIEW_TYPE_1D:
		case VK_IMAGE_VIEW_TYPE_1D_ARRAY:
			return tcu::IVec3(16, 1, 1);

		case VK_IMAGE_VIEW_TYPE_3D:
			return tcu::IVec3(16);

		default:
			break;
	}

	return tcu::IVec3(16, 16, 1);
}

int ImageViewTest::getArraySize (VkImageViewType viewType)
{
	switch (viewType)
	{
		case VK_IMAGE_VIEW_TYPE_3D:
			return 1;

		case VK_IMAGE_VIEW_TYPE_CUBE:
		case VK_IMAGE_VIEW_TYPE_CUBE_ARRAY:
			return 18;

		default:
			break;
	}

	return 6;
}

int ImageViewTest::getNumLevels (VkImageViewType viewType)
{
	const tcu::IVec3 imageSize = getImageSize(viewType);

	return deLog2Floor32(deMax32(imageSize.x(), deMax32(imageSize.y(), imageSize.z()))) + 1;
}

static std::string getFormatCaseName (const VkFormat format)
{
	const std::string fullName = getFormatName(format);

	DE_ASSERT(de::beginsWith(fullName, "VK_FORMAT_"));

	return de::toLower(fullName.substr(10));
}

static de::MovePtr<tcu::TestCaseGroup> createSubresourceRangeTests(tcu::TestContext& testCtx, VkImageViewType viewType, VkFormat imageFormat)
{
	struct TestCaseConfig
	{
		const char*				name;
		float					samplerLod;
		VkImageSubresourceRange	subresourceRange;
	};

	const deUint32				numLevels				= ImageViewTest::getNumLevels(viewType);
	const deUint32				arraySize				= ImageViewTest::getArraySize(viewType);
	const VkImageAspectFlags	imageAspectFlags		= VK_IMAGE_ASPECT_COLOR_BIT;
	const VkComponentMapping	componentMapping		= { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };

	de::MovePtr<tcu::TestCaseGroup> rangeTests (new tcu::TestCaseGroup(testCtx, "subresource_range", ""));

#define ADD_SUBRESOURCE_RANGE_TESTS(TEST_CASES)															\
	do {																								\
		for (int configNdx = 0; configNdx < DE_LENGTH_OF_ARRAY(TEST_CASES); configNdx++)				\
		{																								\
			std::ostringstream		desc;																\
			const TestCaseConfig	config	= (TEST_CASES)[configNdx];									\
			desc << "Samples level " << config.samplerLod << " with :\n" << config.subresourceRange;	\
			rangeTests->addChild(new ImageViewTest(testCtx, config.name, desc.str().c_str(), viewType,	\
												   imageFormat, config.samplerLod, componentMapping,	\
												   config.subresourceRange));							\
		}																								\
	} while (deGetFalse())

	if (viewType == VK_IMAGE_VIEW_TYPE_1D_ARRAY || viewType == VK_IMAGE_VIEW_TYPE_2D_ARRAY)
	{
		const TestCaseConfig mipLevelRangeCases[] =
		{
			//	name					samplerLod	subresourceRange (aspectMask, baseMipLevel, mipLevels, baseArrayLayer, arraySize)
			{ "lod_base_mip_level",		0.0f,		{ imageAspectFlags, 2u, numLevels - 2u, 0u, arraySize } },
			{ "lod_mip_levels",			4.0f,		{ imageAspectFlags, 0u, 3u, 0u, arraySize } },
		};

		const TestCaseConfig arrayRangeCases[] =
		{
			//	name					samplerLod		subresourceRange (aspectMask, baseMipLevel, mipLevels, baseArrayLayer, arraySize)
			{ "base_array_layer",		0.0f,			{ imageAspectFlags, 0u, numLevels, 1u, arraySize - 1u } },
			{ "array_size",				0.0f,			{ imageAspectFlags, 0u, numLevels, 0u, 4u } },
			{ "array_base_and_size",	0.0f,			{ imageAspectFlags, 0u, numLevels, 2u, 3u } },
		};

		const TestCaseConfig mipLevelAndArrayRangeCases[] =
		{
			//	name										samplerLod		subresourceRange (aspectMask, baseMipLevel, mipLevels, baseArrayLayer, arraySize)
			{ "lod_base_mip_level_base_array_layer",		0.0f,			{ imageAspectFlags, 2u, numLevels - 2u, 1u, 5u } },
			{ "lod_mip_levels_base_array_layer",			4.0f,			{ imageAspectFlags, 0u, 3u, 1u, 5u } },

			{ "lod_base_mip_level_array_size",				0.0f,			{ imageAspectFlags, 2u, numLevels - 2u, 0u, 4u } },
			{ "lod_mip_levels_array_size",					4.0f,			{ imageAspectFlags, 0u, 3u, 0u, 4u } },

			{ "lod_base_mip_level_array_base_and_size",		0.0f,			{ imageAspectFlags, 2u, numLevels - 2u, 2u, 3u } },
			{ "lod_mip_levels_array_base_and_size",			4.0f,			{ imageAspectFlags, 0u, 3u, 2u, 3u } },
		};

		const TestCaseConfig mipLevelAndArrayRemainingRangeCases[] =
		{
			//	name																samplerLod	subresourceRange (aspectMask, baseMipLevel, mipLevels, baseArrayLayer, arraySize)
			{ "lod_base_mip_level_remaining_levels",								0.0f,		{ imageAspectFlags,	1u,	VK_REMAINING_MIP_LEVELS,	0u,	arraySize					} },
			{ "base_array_layer_remaining_layers",									0.0f,		{ imageAspectFlags,	0u,	numLevels,					1u,	VK_REMAINING_ARRAY_LAYERS	} },
			{ "lod_base_mip_level_base_array_layer_remaining_levels_and_layers",	0.0f,		{ imageAspectFlags,	2u,	VK_REMAINING_MIP_LEVELS,	2u,	VK_REMAINING_ARRAY_LAYERS	} },
		};

		ADD_SUBRESOURCE_RANGE_TESTS(mipLevelRangeCases);
		ADD_SUBRESOURCE_RANGE_TESTS(arrayRangeCases);
		ADD_SUBRESOURCE_RANGE_TESTS(mipLevelAndArrayRangeCases);
		ADD_SUBRESOURCE_RANGE_TESTS(mipLevelAndArrayRemainingRangeCases);
	}
	else if (viewType == VK_IMAGE_VIEW_TYPE_CUBE_ARRAY)
	{
		const TestCaseConfig mipLevelRangeCases[] =
		{
			//	name					samplerLod	subresourceRange (aspectMask, baseMipLevel, mipLevels, baseArrayLayer, arraySize)
			{ "lod_base_mip_level",		0.0f,		{ imageAspectFlags, 2u, numLevels - 2u, 0u, arraySize } },
			{ "lod_mip_levels",			4.0f,		{ imageAspectFlags, 0u, 3u, 0u, arraySize } },
		};

		const TestCaseConfig arrayRangeCases[] =
		{
			//	name					samplerLod		subresourceRange (aspectMask, baseMipLevel, mipLevels, baseArrayLayer, arraySize)
			{ "base_array_layer",		0.0f,			{ imageAspectFlags, 0u, numLevels, 6u, arraySize - 6u } },
			{ "array_size",				0.0f,			{ imageAspectFlags, 0u, numLevels, 0u, 6u } },
			{ "array_base_and_size",	0.0f,			{ imageAspectFlags, 0u, numLevels, 12u, 6u } },
		};

		const TestCaseConfig mipLevelAndArrayRangeCases[] =
		{
			//	name										samplerLod		subresourceRange (aspectMask, baseMipLevel, mipLevels, baseArrayLayer, arraySize)
			{ "lod_base_mip_level_base_array_layer",		0.0f,			{ imageAspectFlags, 2u, numLevels - 2u, 6u, arraySize - 6u } },
			{ "lod_mip_levels_base_array_layer",			4.0f,			{ imageAspectFlags, 0u, 3u, 6u, arraySize - 6u } },

			{ "lod_base_mip_level_array_size",				0.0f,			{ imageAspectFlags, 2u, numLevels - 2u, 0u, 6u } },
			{ "lod_mip_levels_array_size",					4.0f,			{ imageAspectFlags, 0u, 3u, 0u, 6u } },

			{ "lod_base_mip_level_array_base_and_size",		0.0f,			{ imageAspectFlags, 2u, numLevels - 2u, 12u, 6u } },
			{ "lod_mip_levels_array_base_and_size",			4.0f,			{ imageAspectFlags, 0u, 3u, 12u, 6u } },
		};

		const TestCaseConfig mipLevelAndArrayRemainingRangeCases[] =
		{
			//	name																samplerLod	subresourceRange (aspectMask, baseMipLevel, mipLevels, baseArrayLayer, arraySize)
			{ "lod_base_mip_level_remaining_levels",								0.0f,		{ imageAspectFlags,	1u,	VK_REMAINING_MIP_LEVELS,	0u,		arraySize					} },
			{ "base_array_layer_remaining_layers",									0.0f,		{ imageAspectFlags,	0u,	numLevels,					6u,		VK_REMAINING_ARRAY_LAYERS	} },
			{ "lod_base_mip_level_base_array_layer_remaining_levels_and_layers",	0.0f,		{ imageAspectFlags,	2u,	VK_REMAINING_MIP_LEVELS,	12u,	VK_REMAINING_ARRAY_LAYERS	} },
		};

		ADD_SUBRESOURCE_RANGE_TESTS(mipLevelRangeCases);
		ADD_SUBRESOURCE_RANGE_TESTS(arrayRangeCases);
		ADD_SUBRESOURCE_RANGE_TESTS(mipLevelAndArrayRangeCases);
		ADD_SUBRESOURCE_RANGE_TESTS(mipLevelAndArrayRemainingRangeCases);
	}
	else if (viewType == VK_IMAGE_VIEW_TYPE_1D || viewType == VK_IMAGE_VIEW_TYPE_2D)
	{
		const TestCaseConfig mipLevelRangeCases[] =
		{
			//	name					samplerLod	subresourceRange (aspectMask, baseMipLevel, mipLevels, baseArrayLayer, arraySize)
			{ "lod_base_mip_level",		0.0f,		{ imageAspectFlags, 2u, numLevels - 2u, 0u, 1u } },
			{ "lod_mip_levels",			4.0f,		{ imageAspectFlags, 0u, 3u, 0u, 1u } },
		};

		const TestCaseConfig arrayRangeCases[] =
		{
			//	name					samplerLod		subresourceRange (aspectMask, baseMipLevel, mipLevels, baseArrayLayer, arraySize)
			{ "array_layer_second",		0.0f,			{ imageAspectFlags, 0u, numLevels, 1u, 1u } },
			{ "array_layer_last",		0.0f,			{ imageAspectFlags, 0u, numLevels, arraySize - 1u, 1u } },
		};

		const TestCaseConfig mipLevelAndArrayRangeCases[] =
		{
			//	name									samplerLod	subresourceRange (aspectMask, baseMipLevel, mipLevels, baseArrayLayer, arraySize)
			{ "lod_base_mip_level_array_layer_second",	0.0f,		{ imageAspectFlags, 2u, numLevels - 2u, 1u, 1u } },
			{ "lod_mip_levels_array_layer_second",		4.0f,		{ imageAspectFlags, 0u, 3u, arraySize - 1u, 1u } },

			{ "lod_base_mip_level_array_layer_last",	0.0f,		{ imageAspectFlags, 2u, numLevels - 2u, 5u, 1u } },
			{ "lod_mip_levels_array_layer_last",		4.0f,		{ imageAspectFlags, 0u, 3u, arraySize - 1u, 1u } },
		};

		const TestCaseConfig mipLevelAndArrayRemainingRangeCases[] =
		{
			//	name																samplerLod	subresourceRange (aspectMask, baseMipLevel, mipLevels, baseArrayLayer, arraySize)
			{ "lod_base_mip_level_remaining_levels",								0.0f,		{ imageAspectFlags,	1u,	VK_REMAINING_MIP_LEVELS,	0u,				1u							} },
			{ "array_layer_last_remaining_layers",									0.0f,		{ imageAspectFlags,	0u,	numLevels,					arraySize - 1u,	VK_REMAINING_ARRAY_LAYERS	} },
			{ "lod_base_mip_level_array_layer_last_remaining_levels_and_layers",	0.0f,		{ imageAspectFlags,	2u,	VK_REMAINING_MIP_LEVELS,	arraySize - 1u,	VK_REMAINING_ARRAY_LAYERS	} },
		};

		ADD_SUBRESOURCE_RANGE_TESTS(mipLevelRangeCases);
		ADD_SUBRESOURCE_RANGE_TESTS(arrayRangeCases);
		ADD_SUBRESOURCE_RANGE_TESTS(mipLevelAndArrayRangeCases);
		ADD_SUBRESOURCE_RANGE_TESTS(mipLevelAndArrayRemainingRangeCases);
	}
	else if (viewType == VK_IMAGE_VIEW_TYPE_CUBE)
	{
		const TestCaseConfig mipLevelRangeCases[] =
		{
			//	name					samplerLod	subresourceRange (aspectMask, baseMipLevel, mipLevels, baseArrayLayer, arraySize)
			{ "lod_base_mip_level",		0.0f,		{ imageAspectFlags, 2u, numLevels - 2u, 0u, 6u } },
			{ "lod_mip_levels",			4.0f,		{ imageAspectFlags, 0u, 3u, 0u, 6u } },
		};

		const TestCaseConfig arrayRangeCases[] =
		{
			//	name					samplerLod		subresourceRange (aspectMask, baseMipLevel, mipLevels, baseArrayLayer, arraySize)
			{ "array_layer_second",		0.0f,			{ imageAspectFlags, 0u, numLevels, 6u, 6u } },
			{ "array_layer_last",		0.0f,			{ imageAspectFlags, 0u, numLevels, arraySize - 6u, 6u } },
		};

		const TestCaseConfig mipLevelAndArrayRangeCases[] =
		{
			//	name									samplerLod	subresourceRange (aspectMask, baseMipLevel, mipLevels, baseArrayLayer, arraySize)
			{ "lod_base_mip_level_array_layer_second",	0.0f,		{ imageAspectFlags, 2u, numLevels - 2u, 6u, 6u } },
			{ "lod_mip_levels_array_layer_second",		4.0f,		{ imageAspectFlags, 0u, 3u, 6u, 6u } },

			{ "lod_base_mip_level_array_layer_last",	0.0f,		{ imageAspectFlags, 2u, numLevels - 2u, arraySize - 6u, 6u } },
			{ "lod_mip_levels_array_layer_last",		4.0f,		{ imageAspectFlags, 0u, 3u, arraySize - 6u, 6u } },
		};

		const TestCaseConfig mipLevelAndArrayRemainingRangeCases[] =
		{
			//	name																samplerLod	subresourceRange (aspectMask, baseMipLevel, mipLevels, baseArrayLayer, arraySize)
			{ "lod_base_mip_level_remaining_levels",								0.0f,		{ imageAspectFlags,	1u,	VK_REMAINING_MIP_LEVELS,	0u,				6u							} },
			{ "array_layer_last_remaining_layers",									0.0f,		{ imageAspectFlags,	0u,	numLevels,					arraySize - 6u,	VK_REMAINING_ARRAY_LAYERS	} },
			{ "lod_base_mip_level_array_layer_last_remaining_levels_and_layers",	0.0f,		{ imageAspectFlags,	2u,	VK_REMAINING_MIP_LEVELS,	arraySize - 6u,	VK_REMAINING_ARRAY_LAYERS	} },
		};

		ADD_SUBRESOURCE_RANGE_TESTS(mipLevelRangeCases);
		ADD_SUBRESOURCE_RANGE_TESTS(arrayRangeCases);
		ADD_SUBRESOURCE_RANGE_TESTS(mipLevelAndArrayRangeCases);
		ADD_SUBRESOURCE_RANGE_TESTS(mipLevelAndArrayRemainingRangeCases);
	}
	else if (viewType == VK_IMAGE_VIEW_TYPE_3D)
	{
		const TestCaseConfig mipLevelRangeCases[] =
		{
			//	name					samplerLod	subresourceRange (aspectMask, baseMipLevel, mipLevels, baseArrayLayer, arraySize)
			{ "lod_base_mip_level",		0.0f,		{ imageAspectFlags, 2u, numLevels - 2u, 0u, arraySize } },
			{ "lod_mip_levels",			4.0f,		{ imageAspectFlags, 0u, 3u, 0u, arraySize } },
		};

		const TestCaseConfig mipLevelAndArrayRemainingRangeCases[] =
		{
			//	name																samplerLod	subresourceRange (aspectMask, baseMipLevel, mipLevels, baseArrayLayer, arraySize)
			{ "lod_base_mip_level_remaining_levels",								0.0f,		{ imageAspectFlags,	1u,	VK_REMAINING_MIP_LEVELS,	0u,	arraySize					} },
			{ "single_array_layer_remaining_layers",								0.0f,		{ imageAspectFlags,	0u,	numLevels,					0u,	VK_REMAINING_ARRAY_LAYERS	} },
			{ "lod_base_mip_level_single_array_layer_remaining_levels_and_layers",	0.0f,		{ imageAspectFlags,	2u,	VK_REMAINING_MIP_LEVELS,	0u,	VK_REMAINING_ARRAY_LAYERS	} },
		};

		ADD_SUBRESOURCE_RANGE_TESTS(mipLevelRangeCases);
		ADD_SUBRESOURCE_RANGE_TESTS(mipLevelAndArrayRemainingRangeCases);
	}

#undef ADD_SUBRESOURCE_RANGE_TESTS

	return rangeTests;
}

static std::vector<VkComponentMapping> getComponentMappingPermutations (const VkComponentMapping& componentMapping)
{
	std::vector<VkComponentMapping> mappings;

	const VkComponentSwizzle channelSwizzles[4] = { componentMapping.r, componentMapping.g, componentMapping.b, componentMapping.a };

	// Rearranges the channels by shifting their positions.
	for (int firstChannelNdx = 0; firstChannelNdx < 4; firstChannelNdx++)
	{
		VkComponentSwizzle currentChannel[4];

		for (int channelNdx = 0; channelNdx < 4; channelNdx++)
			currentChannel[channelNdx] = channelSwizzles[(firstChannelNdx + channelNdx) % 4];

		const VkComponentMapping mappingPermutation  =
		{
			currentChannel[0],
			currentChannel[1],
			currentChannel[2],
			currentChannel[3]
		};

		mappings.push_back(mappingPermutation);
	}

	return mappings;
}

static std::string getComponentSwizzleCaseName (VkComponentSwizzle componentSwizzle)
{
	const std::string fullName = getComponentSwizzleName(componentSwizzle);

	DE_ASSERT(de::beginsWith(fullName, "VK_COMPONENT_SWIZZLE_"));

	return de::toLower(fullName.substr(21));
}

static std::string getComponentMappingCaseName (const VkComponentMapping& componentMapping)
{
	std::ostringstream name;

	name << getComponentSwizzleCaseName(componentMapping.r) << "_"
		 << getComponentSwizzleCaseName(componentMapping.g) << "_"
		 << getComponentSwizzleCaseName(componentMapping.b) << "_"
		 << getComponentSwizzleCaseName(componentMapping.a);

	return name.str();
}

static de::MovePtr<tcu::TestCaseGroup> createComponentSwizzleTests (tcu::TestContext& testCtx, VkImageViewType viewType, VkFormat imageFormat)
{
	deUint32 arraySize = 0;

	switch (viewType)
	{
		case VK_IMAGE_VIEW_TYPE_1D:
		case VK_IMAGE_VIEW_TYPE_2D:
		case VK_IMAGE_VIEW_TYPE_3D:
			arraySize = 1;
			break;

		case VK_IMAGE_VIEW_TYPE_CUBE:
			arraySize = 6;
			break;

		case VK_IMAGE_VIEW_TYPE_1D_ARRAY:
		case VK_IMAGE_VIEW_TYPE_2D_ARRAY:
		case VK_IMAGE_VIEW_TYPE_CUBE_ARRAY:
			arraySize = ImageViewTest::getArraySize(viewType);
			break;

		default:
			break;
	}

	const VkImageSubresourceRange subresourceRange =
	{
		VK_IMAGE_ASPECT_COLOR_BIT,							// VkImageAspectFlags	aspectMask;
		0u,													// deUint32				baseMipLevel;
		(deUint32)ImageViewTest::getNumLevels(viewType),	// deUint32				mipLevels;
		0u,													// deUint32				baseArrayLayer;
		arraySize,											// deUint32				arraySize;
	};

	const VkComponentMapping				baseMapping			= { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
	const std::vector<VkComponentMapping>	componentMappings	= getComponentMappingPermutations(baseMapping);
	de::MovePtr<tcu::TestCaseGroup>			swizzleTests		(new tcu::TestCaseGroup(testCtx, "component_swizzle", ""));

	for (size_t mappingNdx = 0; mappingNdx < componentMappings.size(); mappingNdx++)
	{
		swizzleTests->addChild(new ImageViewTest(testCtx,
												 getComponentMappingCaseName(componentMappings[mappingNdx]).c_str(),
												 "",
												 viewType,
												 imageFormat,
												 0.0f,
												 componentMappings[mappingNdx],
												 subresourceRange));
	}

	return swizzleTests;
}

} // anonymous

tcu::TestCaseGroup* createImageViewTests (tcu::TestContext& testCtx)
{
	const struct
	{
		VkImageViewType		type;
		const char*			name;
	}
	imageViewTypes[] =
	{
		{ VK_IMAGE_VIEW_TYPE_1D,			"1d" },
		{ VK_IMAGE_VIEW_TYPE_1D_ARRAY,		"1d_array" },
		{ VK_IMAGE_VIEW_TYPE_2D,			"2d" },
		{ VK_IMAGE_VIEW_TYPE_2D_ARRAY,		"2d_array" },
		{ VK_IMAGE_VIEW_TYPE_3D,			"3d" },
		{ VK_IMAGE_VIEW_TYPE_CUBE,			"cube" },
		{ VK_IMAGE_VIEW_TYPE_CUBE_ARRAY,	"cube_array" }
	};

	const VkFormat formats[] =
	{
		VK_FORMAT_R4G4_UNORM_PACK8,
		VK_FORMAT_R4G4B4A4_UNORM_PACK16,
		VK_FORMAT_R5G6B5_UNORM_PACK16,
		VK_FORMAT_R5G5B5A1_UNORM_PACK16,
		VK_FORMAT_R8_UNORM,
		VK_FORMAT_R8_SNORM,
		VK_FORMAT_R8_USCALED,
		VK_FORMAT_R8_SSCALED,
		VK_FORMAT_R8_UINT,
		VK_FORMAT_R8_SINT,
		VK_FORMAT_R8_SRGB,
		VK_FORMAT_R8G8_UNORM,
		VK_FORMAT_R8G8_SNORM,
		VK_FORMAT_R8G8_USCALED,
		VK_FORMAT_R8G8_SSCALED,
		VK_FORMAT_R8G8_UINT,
		VK_FORMAT_R8G8_SINT,
		VK_FORMAT_R8G8_SRGB,
		VK_FORMAT_R8G8B8_UNORM,
		VK_FORMAT_R8G8B8_SNORM,
		VK_FORMAT_R8G8B8_USCALED,
		VK_FORMAT_R8G8B8_SSCALED,
		VK_FORMAT_R8G8B8_UINT,
		VK_FORMAT_R8G8B8_SINT,
		VK_FORMAT_R8G8B8_SRGB,
		VK_FORMAT_R8G8B8A8_UNORM,
		VK_FORMAT_R8G8B8A8_SNORM,
		VK_FORMAT_R8G8B8A8_USCALED,
		VK_FORMAT_R8G8B8A8_SSCALED,
		VK_FORMAT_R8G8B8A8_UINT,
		VK_FORMAT_R8G8B8A8_SINT,
		VK_FORMAT_R8G8B8A8_SRGB,
		VK_FORMAT_A2R10G10B10_UNORM_PACK32,
		VK_FORMAT_A2R10G10B10_UINT_PACK32,
		VK_FORMAT_A2B10G10R10_USCALED_PACK32,
		VK_FORMAT_R16_UNORM,
		VK_FORMAT_R16_SNORM,
		VK_FORMAT_R16_USCALED,
		VK_FORMAT_R16_SSCALED,
		VK_FORMAT_R16_UINT,
		VK_FORMAT_R16_SINT,
		VK_FORMAT_R16_SFLOAT,
		VK_FORMAT_R16G16_UNORM,
		VK_FORMAT_R16G16_SNORM,
		VK_FORMAT_R16G16_USCALED,
		VK_FORMAT_R16G16_SSCALED,
		VK_FORMAT_R16G16_UINT,
		VK_FORMAT_R16G16_SINT,
		VK_FORMAT_R16G16_SFLOAT,
		VK_FORMAT_R16G16B16_UNORM,
		VK_FORMAT_R16G16B16_SNORM,
		VK_FORMAT_R16G16B16_USCALED,
		VK_FORMAT_R16G16B16_SSCALED,
		VK_FORMAT_R16G16B16_UINT,
		VK_FORMAT_R16G16B16_SINT,
		VK_FORMAT_R16G16B16_SFLOAT,
		VK_FORMAT_R16G16B16A16_UNORM,
		VK_FORMAT_R16G16B16A16_SNORM,
		VK_FORMAT_R16G16B16A16_USCALED,
		VK_FORMAT_R16G16B16A16_SSCALED,
		VK_FORMAT_R16G16B16A16_UINT,
		VK_FORMAT_R16G16B16A16_SINT,
		VK_FORMAT_R16G16B16A16_SFLOAT,
		VK_FORMAT_R32_UINT,
		VK_FORMAT_R32_SINT,
		VK_FORMAT_R32_SFLOAT,
		VK_FORMAT_R32G32_UINT,
		VK_FORMAT_R32G32_SINT,
		VK_FORMAT_R32G32_SFLOAT,
		VK_FORMAT_R32G32B32_UINT,
		VK_FORMAT_R32G32B32_SINT,
		VK_FORMAT_R32G32B32_SFLOAT,
		VK_FORMAT_R32G32B32A32_UINT,
		VK_FORMAT_R32G32B32A32_SINT,
		VK_FORMAT_R32G32B32A32_SFLOAT,
		VK_FORMAT_B10G11R11_UFLOAT_PACK32,
		VK_FORMAT_E5B9G9R9_UFLOAT_PACK32,
		VK_FORMAT_B4G4R4A4_UNORM_PACK16,
		VK_FORMAT_B5G5R5A1_UNORM_PACK16,

		// Compressed formats
		VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK,
		VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK,
		VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK,
		VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK,
		VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK,
		VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK,
		VK_FORMAT_EAC_R11_UNORM_BLOCK,
		VK_FORMAT_EAC_R11_SNORM_BLOCK,
		VK_FORMAT_EAC_R11G11_UNORM_BLOCK,
		VK_FORMAT_EAC_R11G11_SNORM_BLOCK,
		VK_FORMAT_ASTC_4x4_UNORM_BLOCK,
		VK_FORMAT_ASTC_4x4_SRGB_BLOCK,
		VK_FORMAT_ASTC_5x4_UNORM_BLOCK,
		VK_FORMAT_ASTC_5x4_SRGB_BLOCK,
		VK_FORMAT_ASTC_5x5_UNORM_BLOCK,
		VK_FORMAT_ASTC_5x5_SRGB_BLOCK,
		VK_FORMAT_ASTC_6x5_UNORM_BLOCK,
		VK_FORMAT_ASTC_6x5_SRGB_BLOCK,
		VK_FORMAT_ASTC_6x6_UNORM_BLOCK,
		VK_FORMAT_ASTC_6x6_SRGB_BLOCK,
		VK_FORMAT_ASTC_8x5_UNORM_BLOCK,
		VK_FORMAT_ASTC_8x5_SRGB_BLOCK,
		VK_FORMAT_ASTC_8x6_UNORM_BLOCK,
		VK_FORMAT_ASTC_8x6_SRGB_BLOCK,
		VK_FORMAT_ASTC_8x8_UNORM_BLOCK,
		VK_FORMAT_ASTC_8x8_SRGB_BLOCK,
		VK_FORMAT_ASTC_10x5_UNORM_BLOCK,
		VK_FORMAT_ASTC_10x5_SRGB_BLOCK,
		VK_FORMAT_ASTC_10x6_UNORM_BLOCK,
		VK_FORMAT_ASTC_10x6_SRGB_BLOCK,
		VK_FORMAT_ASTC_10x8_UNORM_BLOCK,
		VK_FORMAT_ASTC_10x8_SRGB_BLOCK,
		VK_FORMAT_ASTC_10x10_UNORM_BLOCK,
		VK_FORMAT_ASTC_10x10_SRGB_BLOCK,
		VK_FORMAT_ASTC_12x10_UNORM_BLOCK,
		VK_FORMAT_ASTC_12x10_SRGB_BLOCK,
		VK_FORMAT_ASTC_12x12_UNORM_BLOCK,
		VK_FORMAT_ASTC_12x12_SRGB_BLOCK,
	};

	de::MovePtr<tcu::TestCaseGroup> imageTests			(new tcu::TestCaseGroup(testCtx, "image_view", "Image tests"));
	de::MovePtr<tcu::TestCaseGroup> viewTypeTests		(new tcu::TestCaseGroup(testCtx, "view_type", ""));

	for (int viewTypeNdx = 0; viewTypeNdx < DE_LENGTH_OF_ARRAY(imageViewTypes); viewTypeNdx++)
	{
		const VkImageViewType			viewType		= imageViewTypes[viewTypeNdx].type;
		de::MovePtr<tcu::TestCaseGroup>	viewTypeGroup	(new tcu::TestCaseGroup(testCtx, imageViewTypes[viewTypeNdx].name, (std::string("Uses a ") + imageViewTypes[viewTypeNdx].name + " view").c_str()));
		de::MovePtr<tcu::TestCaseGroup>	formatTests		(new tcu::TestCaseGroup(testCtx, "format", "Uses samplable formats"));

		for (size_t formatNdx = 0; formatNdx < DE_LENGTH_OF_ARRAY(formats); formatNdx++)
		{
			const VkFormat		format		= formats[formatNdx];

			if (isCompressedFormat(format))
			{
				// Do not use compressed formats with 1D and 1D array textures.
				if (viewType == VK_IMAGE_VIEW_TYPE_1D || viewType == VK_IMAGE_VIEW_TYPE_1D_ARRAY)
					break;
			}

			de::MovePtr<tcu::TestCaseGroup>	formatGroup	(new tcu::TestCaseGroup(testCtx,
																				getFormatCaseName(format).c_str(),
																				(std::string("Samples a texture of format ") + getFormatName(format)).c_str()));

			de::MovePtr<tcu::TestCaseGroup>	subresourceRangeTests	= createSubresourceRangeTests(testCtx, viewType, format);
			de::MovePtr<tcu::TestCaseGroup>	componentSwizzleTests	= createComponentSwizzleTests(testCtx, viewType, format);

			formatGroup->addChild(componentSwizzleTests.release());
			formatGroup->addChild(subresourceRangeTests.release());
			formatTests->addChild(formatGroup.release());
		}

		viewTypeGroup->addChild(formatTests.release());
		viewTypeTests->addChild(viewTypeGroup.release());
	}

	imageTests->addChild(viewTypeTests.release());

	return imageTests.release();
}

} // pipeline
} // vkt
