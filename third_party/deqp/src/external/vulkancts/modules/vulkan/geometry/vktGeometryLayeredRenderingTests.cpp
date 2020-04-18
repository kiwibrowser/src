/*------------------------------------------------------------------------
 * Vulkan Conformance Tests
 * ------------------------
 *
 * Copyright (c) 2016 The Khronos Group Inc.
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
 * \brief Geometry shader layered rendering tests
 *//*--------------------------------------------------------------------*/

#include "vktGeometryLayeredRenderingTests.hpp"
#include "vktTestCase.hpp"
#include "vktTestCaseUtil.hpp"
#include "vktGeometryTestsUtil.hpp"

#include "vkPrograms.hpp"
#include "vkStrUtil.hpp"
#include "vkQueryUtil.hpp"
#include "vkMemUtil.hpp"
#include "vkRefUtil.hpp"
#include "vkTypeUtil.hpp"
#include "vkImageUtil.hpp"

#include "deStringUtil.hpp"
#include "deUniquePtr.hpp"

#include "tcuTextureUtil.hpp"
#include "tcuVectorUtil.hpp"
#include "tcuTestLog.hpp"

namespace vkt
{
namespace geometry
{
namespace
{
using namespace vk;
using de::MovePtr;
using de::UniquePtr;
using tcu::Vec4;
using tcu::IVec3;

enum TestType
{
	TEST_TYPE_DEFAULT_LAYER,					// !< draw to default layer
	TEST_TYPE_SINGLE_LAYER,						// !< draw to single layer
	TEST_TYPE_ALL_LAYERS,						// !< draw all layers
	TEST_TYPE_DIFFERENT_CONTENT,				// !< draw different content to different layers
	TEST_TYPE_LAYER_ID,							// !< draw to all layers, verify gl_Layer fragment input
	TEST_TYPE_INVOCATION_PER_LAYER,				// !< draw to all layers, one invocation per layer
	TEST_TYPE_MULTIPLE_LAYERS_PER_INVOCATION,	// !< draw to all layers, multiple invocations write to multiple layers
};

struct ImageParams
{
	VkImageViewType		viewType;
	VkExtent3D			size;
	deUint32			numLayers;
};

struct TestParams
{
	TestType			testType;
	ImageParams			image;
};

static const float s_colors[][4] =
{
	{ 1.0f, 1.0f, 1.0f, 1.0f },		// white
	{ 1.0f, 0.0f, 0.0f, 1.0f },		// red
	{ 0.0f, 1.0f, 0.0f, 1.0f },		// green
	{ 0.0f, 0.0f, 1.0f, 1.0f },		// blue
	{ 1.0f, 1.0f, 0.0f, 1.0f },		// yellow
	{ 1.0f, 0.0f, 1.0f, 1.0f },		// magenta
};

deUint32 getTargetLayer (const ImageParams& imageParams)
{
	if (imageParams.viewType == VK_IMAGE_VIEW_TYPE_3D)
		return imageParams.size.depth / 2;
	else
		return imageParams.numLayers / 2;
}

std::string getShortImageViewTypeName (const VkImageViewType imageViewType)
{
	std::string s(getImageViewTypeName(imageViewType));
	return de::toLower(s.substr(19));
}

VkImageType getImageType (const VkImageViewType viewType)
{
	switch (viewType)
	{
		case VK_IMAGE_VIEW_TYPE_1D:
		case VK_IMAGE_VIEW_TYPE_1D_ARRAY:
			return VK_IMAGE_TYPE_1D;

		case VK_IMAGE_VIEW_TYPE_2D:
		case VK_IMAGE_VIEW_TYPE_2D_ARRAY:
		case VK_IMAGE_VIEW_TYPE_CUBE:
		case VK_IMAGE_VIEW_TYPE_CUBE_ARRAY:
			return VK_IMAGE_TYPE_2D;

		case VK_IMAGE_VIEW_TYPE_3D:
			return VK_IMAGE_TYPE_3D;

		default:
			DE_ASSERT(0);
			return VK_IMAGE_TYPE_LAST;
	}
}

inline bool isCubeImageViewType (const VkImageViewType viewType)
{
	return viewType == VK_IMAGE_VIEW_TYPE_CUBE || viewType == VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;
}

VkImageCreateInfo makeImageCreateInfo (const VkImageCreateFlags flags, const VkImageType type, const VkFormat format, const VkExtent3D size, const deUint32 numLayers, const VkImageUsageFlags usage)
{
	const VkImageCreateInfo imageParams =
	{
		VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,			// VkStructureType			sType;
		DE_NULL,										// const void*				pNext;
		flags,											// VkImageCreateFlags		flags;
		type,											// VkImageType				imageType;
		format,											// VkFormat					format;
		size,											// VkExtent3D				extent;
		1u,												// deUint32					mipLevels;
		numLayers,										// deUint32					arrayLayers;
		VK_SAMPLE_COUNT_1_BIT,							// VkSampleCountFlagBits	samples;
		VK_IMAGE_TILING_OPTIMAL,						// VkImageTiling			tiling;
		usage,											// VkImageUsageFlags		usage;
		VK_SHARING_MODE_EXCLUSIVE,						// VkSharingMode			sharingMode;
		0u,												// deUint32					queueFamilyIndexCount;
		DE_NULL,										// const deUint32*			pQueueFamilyIndices;
		VK_IMAGE_LAYOUT_UNDEFINED,						// VkImageLayout			initialLayout;
	};
	return imageParams;
}

Move<VkRenderPass> makeRenderPass (const DeviceInterface&	vk,
								   const VkDevice			device,
								   const VkFormat			colorFormat)
{
	const VkAttachmentDescription colorAttachmentDescription =
	{
		(VkAttachmentDescriptionFlags)0,					// VkAttachmentDescriptionFlags		flags;
		colorFormat,										// VkFormat							format;
		VK_SAMPLE_COUNT_1_BIT,								// VkSampleCountFlagBits			samples;
		VK_ATTACHMENT_LOAD_OP_CLEAR,						// VkAttachmentLoadOp				loadOp;
		VK_ATTACHMENT_STORE_OP_STORE,						// VkAttachmentStoreOp				storeOp;
		VK_ATTACHMENT_LOAD_OP_DONT_CARE,					// VkAttachmentLoadOp				stencilLoadOp;
		VK_ATTACHMENT_STORE_OP_DONT_CARE,					// VkAttachmentStoreOp				stencilStoreOp;
		VK_IMAGE_LAYOUT_UNDEFINED,							// VkImageLayout					initialLayout;
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,			// VkImageLayout					finalLayout;
	};

	const VkAttachmentReference colorAttachmentRef =
	{
		0u,													// deUint32			attachment;
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL			// VkImageLayout	layout;
	};

	const VkSubpassDescription subpassDescription =
	{
		(VkSubpassDescriptionFlags)0,						// VkSubpassDescriptionFlags		flags;
		VK_PIPELINE_BIND_POINT_GRAPHICS,					// VkPipelineBindPoint				pipelineBindPoint;
		0u,													// deUint32							inputAttachmentCount;
		DE_NULL,											// const VkAttachmentReference*		pInputAttachments;
		1u,													// deUint32							colorAttachmentCount;
		&colorAttachmentRef,								// const VkAttachmentReference*		pColorAttachments;
		DE_NULL,											// const VkAttachmentReference*		pResolveAttachments;
		DE_NULL,											// const VkAttachmentReference*		pDepthStencilAttachment;
		0u,													// deUint32							preserveAttachmentCount;
		DE_NULL												// const deUint32*					pPreserveAttachments;
	};

	const VkRenderPassCreateInfo renderPassInfo =
	{
		VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,			// VkStructureType					sType;
		DE_NULL,											// const void*						pNext;
		(VkRenderPassCreateFlags)0,							// VkRenderPassCreateFlags			flags;
		1u,													// deUint32							attachmentCount;
		&colorAttachmentDescription,						// const VkAttachmentDescription*	pAttachments;
		1u,													// deUint32							subpassCount;
		&subpassDescription,								// const VkSubpassDescription*		pSubpasses;
		0u,													// deUint32							dependencyCount;
		DE_NULL												// const VkSubpassDependency*		pDependencies;
	};

	return createRenderPass(vk, device, &renderPassInfo);
}

Move<VkPipeline> makeGraphicsPipeline (const DeviceInterface&		vk,
									   const VkDevice				device,
									   const VkPipelineLayout		pipelineLayout,
									   const VkRenderPass			renderPass,
									   const VkShaderModule			vertexModule,
									   const VkShaderModule			geometryModule,
									   const VkShaderModule			fragmentModule,
									   const VkExtent2D				renderSize)
{
	const VkPipelineVertexInputStateCreateInfo vertexInputStateInfo =
	{
		VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,		// VkStructureType                             sType;
		DE_NULL,														// const void*                                 pNext;
		(VkPipelineVertexInputStateCreateFlags)0,						// VkPipelineVertexInputStateCreateFlags       flags;
		0u,																// uint32_t                                    vertexBindingDescriptionCount;
		DE_NULL,														// const VkVertexInputBindingDescription*      pVertexBindingDescriptions;
		0u,																// uint32_t                                    vertexAttributeDescriptionCount;
		DE_NULL,														// const VkVertexInputAttributeDescription*    pVertexAttributeDescriptions;
	};

	const VkPipelineInputAssemblyStateCreateInfo pipelineInputAssemblyStateInfo =
	{
		VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,	// VkStructureType                             sType;
		DE_NULL,														// const void*                                 pNext;
		(VkPipelineInputAssemblyStateCreateFlags)0,						// VkPipelineInputAssemblyStateCreateFlags     flags;
		VK_PRIMITIVE_TOPOLOGY_POINT_LIST,								// VkPrimitiveTopology                         topology;
		VK_FALSE,														// VkBool32                                    primitiveRestartEnable;
	};

	const VkViewport viewport = makeViewport(
		0.0f, 0.0f,
		static_cast<float>(renderSize.width), static_cast<float>(renderSize.height),
		0.0f, 1.0f);
	const VkRect2D scissor =
	{
		makeOffset2D(0, 0),
		makeExtent2D(renderSize.width, renderSize.height),
	};

	const VkPipelineViewportStateCreateInfo pipelineViewportStateInfo =
	{
		VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,			// VkStructureType                             sType;
		DE_NULL,														// const void*                                 pNext;
		(VkPipelineViewportStateCreateFlags)0,							// VkPipelineViewportStateCreateFlags          flags;
		1u,																// uint32_t                                    viewportCount;
		&viewport,														// const VkViewport*                           pViewports;
		1u,																// uint32_t                                    scissorCount;
		&scissor,														// const VkRect2D*                             pScissors;
	};

	const VkPipelineRasterizationStateCreateInfo pipelineRasterizationStateInfo =
	{
		VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,		// VkStructureType                          sType;
		DE_NULL,														// const void*                              pNext;
		(VkPipelineRasterizationStateCreateFlags)0,						// VkPipelineRasterizationStateCreateFlags  flags;
		VK_FALSE,														// VkBool32                                 depthClampEnable;
		VK_FALSE,														// VkBool32                                 rasterizerDiscardEnable;
		VK_POLYGON_MODE_FILL,											// VkPolygonMode							polygonMode;
		VK_CULL_MODE_NONE,												// VkCullModeFlags							cullMode;
		VK_FRONT_FACE_COUNTER_CLOCKWISE,								// VkFrontFace								frontFace;
		VK_FALSE,														// VkBool32									depthBiasEnable;
		0.0f,															// float									depthBiasConstantFactor;
		0.0f,															// float									depthBiasClamp;
		0.0f,															// float									depthBiasSlopeFactor;
		1.0f,															// float									lineWidth;
	};

	const VkPipelineMultisampleStateCreateInfo pipelineMultisampleStateInfo =
	{
		VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,		// VkStructureType							sType;
		DE_NULL,														// const void*								pNext;
		(VkPipelineMultisampleStateCreateFlags)0,						// VkPipelineMultisampleStateCreateFlags	flags;
		VK_SAMPLE_COUNT_1_BIT,											// VkSampleCountFlagBits					rasterizationSamples;
		VK_FALSE,														// VkBool32									sampleShadingEnable;
		0.0f,															// float									minSampleShading;
		DE_NULL,														// const VkSampleMask*						pSampleMask;
		VK_FALSE,														// VkBool32									alphaToCoverageEnable;
		VK_FALSE														// VkBool32									alphaToOneEnable;
	};

	const VkStencilOpState stencilOpState = makeStencilOpState(
		VK_STENCIL_OP_KEEP,				// stencil fail
		VK_STENCIL_OP_KEEP,				// depth & stencil pass
		VK_STENCIL_OP_KEEP,				// depth only fail
		VK_COMPARE_OP_ALWAYS,			// compare op
		0u,								// compare mask
		0u,								// write mask
		0u);							// reference

	VkPipelineDepthStencilStateCreateInfo pipelineDepthStencilStateInfo =
	{
		VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,		// VkStructureType							sType;
		DE_NULL,														// const void*								pNext;
		(VkPipelineDepthStencilStateCreateFlags)0,						// VkPipelineDepthStencilStateCreateFlags	flags;
		VK_FALSE,														// VkBool32									depthTestEnable;
		VK_FALSE,														// VkBool32									depthWriteEnable;
		VK_COMPARE_OP_LESS,												// VkCompareOp								depthCompareOp;
		VK_FALSE,														// VkBool32									depthBoundsTestEnable;
		VK_FALSE,														// VkBool32									stencilTestEnable;
		stencilOpState,													// VkStencilOpState							front;
		stencilOpState,													// VkStencilOpState							back;
		0.0f,															// float									minDepthBounds;
		1.0f,															// float									maxDepthBounds;
	};

	const VkColorComponentFlags					colorComponentsAll					= VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	const VkPipelineColorBlendAttachmentState	pipelineColorBlendAttachmentState	=
	{
		VK_FALSE,						// VkBool32					blendEnable;
		VK_BLEND_FACTOR_ONE,			// VkBlendFactor			srcColorBlendFactor;
		VK_BLEND_FACTOR_ZERO,			// VkBlendFactor			dstColorBlendFactor;
		VK_BLEND_OP_ADD,				// VkBlendOp				colorBlendOp;
		VK_BLEND_FACTOR_ONE,			// VkBlendFactor			srcAlphaBlendFactor;
		VK_BLEND_FACTOR_ZERO,			// VkBlendFactor			dstAlphaBlendFactor;
		VK_BLEND_OP_ADD,				// VkBlendOp				alphaBlendOp;
		colorComponentsAll,				// VkColorComponentFlags	colorWriteMask;
	};

	const VkPipelineColorBlendStateCreateInfo pipelineColorBlendStateInfo =
	{
		VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,		// VkStructureType								sType;
		DE_NULL,														// const void*									pNext;
		(VkPipelineColorBlendStateCreateFlags)0,						// VkPipelineColorBlendStateCreateFlags			flags;
		VK_FALSE,														// VkBool32										logicOpEnable;
		VK_LOGIC_OP_COPY,												// VkLogicOp									logicOp;
		1u,																// deUint32										attachmentCount;
		&pipelineColorBlendAttachmentState,								// const VkPipelineColorBlendAttachmentState*	pAttachments;
		{ 0.0f, 0.0f, 0.0f, 0.0f },										// float										blendConstants[4];
	};

	const VkPipelineShaderStageCreateInfo pShaderStages[] =
	{
		{
			VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,		// VkStructureType						sType;
			DE_NULL,													// const void*							pNext;
			(VkPipelineShaderStageCreateFlags)0,						// VkPipelineShaderStageCreateFlags		flags;
			VK_SHADER_STAGE_VERTEX_BIT,									// VkShaderStageFlagBits				stage;
			vertexModule,												// VkShaderModule						module;
			"main",														// const char*							pName;
			DE_NULL,													// const VkSpecializationInfo*			pSpecializationInfo;
		},
		{
			VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,		// VkStructureType						sType;
			DE_NULL,													// const void*							pNext;
			(VkPipelineShaderStageCreateFlags)0,						// VkPipelineShaderStageCreateFlags		flags;
			VK_SHADER_STAGE_GEOMETRY_BIT,								// VkShaderStageFlagBits				stage;
			geometryModule,												// VkShaderModule						module;
			"main",														// const char*							pName;
			DE_NULL,													// const VkSpecializationInfo*			pSpecializationInfo;
		},
		{
			VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,		// VkStructureType						sType;
			DE_NULL,													// const void*							pNext;
			(VkPipelineShaderStageCreateFlags)0,						// VkPipelineShaderStageCreateFlags		flags;
			VK_SHADER_STAGE_FRAGMENT_BIT,								// VkShaderStageFlagBits				stage;
			fragmentModule,												// VkShaderModule						module;
			"main",														// const char*							pName;
			DE_NULL,													// const VkSpecializationInfo*			pSpecializationInfo;
		},
	};

	const VkGraphicsPipelineCreateInfo graphicsPipelineInfo =
	{
		VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,	// VkStructureType									sType;
		DE_NULL,											// const void*										pNext;
		(VkPipelineCreateFlags)0,							// VkPipelineCreateFlags							flags;
		DE_LENGTH_OF_ARRAY(pShaderStages),					// deUint32											stageCount;
		pShaderStages,										// const VkPipelineShaderStageCreateInfo*			pStages;
		&vertexInputStateInfo,								// const VkPipelineVertexInputStateCreateInfo*		pVertexInputState;
		&pipelineInputAssemblyStateInfo,					// const VkPipelineInputAssemblyStateCreateInfo*	pInputAssemblyState;
		DE_NULL,											// const VkPipelineTessellationStateCreateInfo*		pTessellationState;
		&pipelineViewportStateInfo,							// const VkPipelineViewportStateCreateInfo*			pViewportState;
		&pipelineRasterizationStateInfo,					// const VkPipelineRasterizationStateCreateInfo*	pRasterizationState;
		&pipelineMultisampleStateInfo,						// const VkPipelineMultisampleStateCreateInfo*		pMultisampleState;
		&pipelineDepthStencilStateInfo,						// const VkPipelineDepthStencilStateCreateInfo*		pDepthStencilState;
		&pipelineColorBlendStateInfo,						// const VkPipelineColorBlendStateCreateInfo*		pColorBlendState;
		DE_NULL,											// const VkPipelineDynamicStateCreateInfo*			pDynamicState;
		pipelineLayout,										// VkPipelineLayout									layout;
		renderPass,											// VkRenderPass										renderPass;
		0u,													// deUint32											subpass;
		DE_NULL,											// VkPipeline										basePipelineHandle;
		0,													// deInt32											basePipelineIndex;
	};

	return createGraphicsPipeline(vk, device, DE_NULL, &graphicsPipelineInfo);
}

//! Convenience wrapper to access 1D, 2D, and 3D image layers/slices in a uniform way.
class LayeredImageAccess
{
public:
	static LayeredImageAccess create (const VkImageType type, const VkFormat format, const VkExtent3D size, const deUint32 numLayers, const void* pData)
	{
		if (type == VK_IMAGE_TYPE_1D)
			return LayeredImageAccess(format, size.width, numLayers, pData);
		else
			return LayeredImageAccess(type, format, size, numLayers, pData);
	}

	inline tcu::ConstPixelBufferAccess getLayer (const int layer) const
	{
		return tcu::getSubregion(m_wholeImage, 0, (m_1dModifier * layer), ((~m_1dModifier & 1) * layer), m_width, m_height, 1);
	}

	inline int getNumLayersOrSlices (void) const
	{
		return m_layers;
	}

private:
	// Specialized for 1D images.
	LayeredImageAccess (const VkFormat format, const deUint32 width, const deUint32 numLayers, const void* pData)
		: m_width		(static_cast<int>(width))
		, m_height		(1)
		, m_1dModifier	(1)
		, m_layers		(numLayers)
		, m_wholeImage	(tcu::ConstPixelBufferAccess(mapVkFormat(format), m_width, m_layers, 1, pData))
	{
	}

	LayeredImageAccess (const VkImageType type, const VkFormat format, const VkExtent3D size, const deUint32 numLayers, const void* pData)
		: m_width		(static_cast<int>(size.width))
		, m_height		(static_cast<int>(size.height))
		, m_1dModifier	(0)
		, m_layers		(static_cast<int>(type == VK_IMAGE_TYPE_3D ? size.depth : numLayers))
		, m_wholeImage	(tcu::ConstPixelBufferAccess(mapVkFormat(format), m_width, m_height, m_layers, pData))
	{
	}

	const int							m_width;
	const int							m_height;
	const int							m_1dModifier;
	const int							m_layers;
	const tcu::ConstPixelBufferAccess	m_wholeImage;
};

inline bool compareColors (const Vec4& colorA, const Vec4& colorB, const Vec4& threshold)
{
	return tcu::allEqual(
				tcu::lessThan(tcu::abs(colorA - colorB), threshold),
				tcu::BVec4(true, true, true, true));
}

bool verifyImageSingleColoredRow (tcu::TestLog& log, const tcu::ConstPixelBufferAccess image, const float rowWidthRatio, const tcu::Vec4& barColor)
{
	DE_ASSERT(rowWidthRatio > 0.0f);

	const Vec4				black				(0.0f, 0.0f, 0.0f, 1.0f);
	const Vec4				green				(0.0f, 1.0f, 0.0f, 1.0f);
	const Vec4				red					(1.0f, 0.0f, 0.0f, 1.0f);
	const Vec4				threshold			(0.02f);
	const int				barLength			= static_cast<int>(rowWidthRatio * static_cast<float>(image.getWidth()));
	const int				barLengthThreshold	= 1;
	tcu::TextureLevel		errorMask			(image.getFormat(), image.getWidth(), image.getHeight());
	tcu::PixelBufferAccess	errorMaskAccess		= errorMask.getAccess();

	tcu::clear(errorMask.getAccess(), green);

	log << tcu::TestLog::Message
		<< "Expecting all pixels with distance less or equal to (about) " << barLength
		<< " pixels from left border to be of color " << barColor.swizzle(0, 1, 2) << "."
		<< tcu::TestLog::EndMessage;

	bool allPixelsOk = true;

	for (int y = 0; y < image.getHeight(); ++y)
	for (int x = 0; x < image.getWidth();  ++x)
	{
		const Vec4	color		= image.getPixel(x, y);
		const bool	isBlack		= compareColors(color, black, threshold);
		const bool	isColor		= compareColors(color, barColor, threshold);

		bool isOk;

		if (x <= barLength - barLengthThreshold)
			isOk = isColor;
		else if (x >= barLength + barLengthThreshold)
			isOk = isBlack;
		else
			isOk = isColor || isBlack;

		allPixelsOk &= isOk;

		if (!isOk)
			errorMaskAccess.setPixel(red, x, y);
	}

	if (allPixelsOk)
	{
		log << tcu::TestLog::Message << "Image is valid." << tcu::TestLog::EndMessage
			<< tcu::TestLog::ImageSet("LayerContent", "Layer content")
			<< tcu::TestLog::Image("Layer", "Layer", image)
			<< tcu::TestLog::EndImageSet;
		return true;
	}
	else
	{
		log << tcu::TestLog::Message << "Image verification failed. Got unexpected pixels." << tcu::TestLog::EndMessage
			<< tcu::TestLog::ImageSet("LayerContent", "Layer content")
			<< tcu::TestLog::Image("Layer",		"Layer",	image)
			<< tcu::TestLog::Image("ErrorMask",	"Errors",	errorMask)
			<< tcu::TestLog::EndImageSet;
		return false;
	}

	log << tcu::TestLog::Image("LayerContent", "Layer content", image);

	return allPixelsOk;
}

bool verifyEmptyImage (tcu::TestLog& log, const tcu::ConstPixelBufferAccess image)
{
	log << tcu::TestLog::Message << "Expecting empty image" << tcu::TestLog::EndMessage;

	const Vec4	black		(0.0f, 0.0f, 0.0f, 1.0f);
	const Vec4	threshold	(0.02f);

	for (int y = 0; y < image.getHeight(); ++y)
	for (int x = 0; x < image.getWidth();  ++x)
	{
		const Vec4 color = image.getPixel(x, y);

		if (!compareColors(color, black, threshold))
		{
			log	<< tcu::TestLog::Message
				<< "Found (at least) one bad pixel at " << x << "," << y << ". Pixel color is not background color."
				<< tcu::TestLog::EndMessage
				<< tcu::TestLog::ImageSet("LayerContent", "Layer content")
				<< tcu::TestLog::Image("Layer", "Layer", image)
				<< tcu::TestLog::EndImageSet;
			return false;
		}
	}

	log << tcu::TestLog::Message << "Image is valid" << tcu::TestLog::EndMessage;

	return true;
}

bool verifyLayerContent (tcu::TestLog& log, const TestType testType, const tcu::ConstPixelBufferAccess image, const int layerNdx, const int numLayers)
{
	const Vec4	white				(1.0f, 1.0f, 1.0f, 1.0f);
	const int	targetLayer			= numLayers / 2;
	const float	variableBarRatio	= static_cast<float>(layerNdx) / static_cast<float>(numLayers);

	switch (testType)
	{
		case TEST_TYPE_DEFAULT_LAYER:
			if (layerNdx == 0)
				return verifyImageSingleColoredRow(log, image, 0.5f, white);
			else
				return verifyEmptyImage(log, image);

		case TEST_TYPE_SINGLE_LAYER:
			if (layerNdx == targetLayer)
				return verifyImageSingleColoredRow(log, image, 0.5f, white);
			else
				return verifyEmptyImage(log, image);

		case TEST_TYPE_ALL_LAYERS:
		case TEST_TYPE_INVOCATION_PER_LAYER:
			return verifyImageSingleColoredRow(log, image, 0.5f, s_colors[layerNdx % DE_LENGTH_OF_ARRAY(s_colors)]);

		case TEST_TYPE_DIFFERENT_CONTENT:
		case TEST_TYPE_MULTIPLE_LAYERS_PER_INVOCATION:
			if (layerNdx == 0)
				return verifyEmptyImage(log, image);
			else
				return verifyImageSingleColoredRow(log, image, variableBarRatio, white);

		case TEST_TYPE_LAYER_ID:
		{
			// This code must be in sync with the fragment shader.
			const tcu::Vec4 layerColor( (layerNdx    % 2) == 1 ? 1.0f : 0.5f,
									   ((layerNdx/2) % 2) == 1 ? 1.0f : 0.5f,
									     layerNdx         == 0 ? 1.0f : 0.0f,
																 1.0f);
			return verifyImageSingleColoredRow(log, image, 0.5f, layerColor);
		}

		default:
			DE_ASSERT(0);
			return false;
	};
}

std::string getLayerDescription (const VkImageViewType viewType, const int layer)
{
	std::ostringstream str;
	const int numCubeFaces = 6;

	if (isCubeImageViewType(viewType))
		str << "cube " << (layer / numCubeFaces) << ", face " << (layer % numCubeFaces);
	else if (viewType == VK_IMAGE_VIEW_TYPE_3D)
		str << "slice z = " << layer;
	else
		str << "layer " << layer;

	return str.str();
}

bool verifyResults (tcu::TestLog& log, const TestParams& params, const VkFormat colorFormat, const void* resultData)
{
	const LayeredImageAccess image = LayeredImageAccess::create(getImageType(params.image.viewType), colorFormat, params.image.size, params.image.numLayers, resultData);

	int numGoodLayers = 0;

	for (int layerNdx = 0; layerNdx < image.getNumLayersOrSlices(); ++layerNdx)
	{
		const tcu::ConstPixelBufferAccess layerImage = image.getLayer(layerNdx);

		log << tcu::TestLog::Message << "Verifying " << getLayerDescription(params.image.viewType, layerNdx) << tcu::TestLog::EndMessage;

		if (verifyLayerContent(log, params.testType, layerImage, layerNdx, image.getNumLayersOrSlices()))
			++numGoodLayers;
	}

	return numGoodLayers == image.getNumLayersOrSlices();
}

std::string toGlsl (const Vec4& v)
{
	std::ostringstream str;
	str << "vec4(";
	for (int i = 0; i < 4; ++i)
		str << (i != 0 ? ", " : "") << de::floatToString(v[i], 1);
	str << ")";
	return str.str();
}

void initPrograms (SourceCollections& programCollection, const TestParams params)
{
	const bool geomOutputColor = (params.testType == TEST_TYPE_ALL_LAYERS || params.testType == TEST_TYPE_INVOCATION_PER_LAYER);

	// Vertex shader
	{
		std::ostringstream src;
		src << glu::getGLSLVersionDeclaration(glu::GLSL_VERSION_450) << "\n"
			<< "\n"
			<< "void main(void)\n"
			<< "{\n"
			<< "}\n";

		programCollection.glslSources.add("vert") << glu::VertexSource(src.str());
	}

	// Geometry shader
	{
		const int numLayers		= static_cast<int>(params.image.viewType == VK_IMAGE_VIEW_TYPE_3D ? params.image.size.depth : params.image.numLayers);

		const int maxVertices	= (params.testType == TEST_TYPE_DIFFERENT_CONTENT)										? (numLayers + 1) * numLayers :
								  (params.testType == TEST_TYPE_ALL_LAYERS || params.testType == TEST_TYPE_LAYER_ID)	? numLayers * 4 :
								  (params.testType == TEST_TYPE_MULTIPLE_LAYERS_PER_INVOCATION)							? 6 : 4;

		std::ostringstream src;
		src << glu::getGLSLVersionDeclaration(glu::GLSL_VERSION_450) << "\n"
			<< "\n";

		if (params.testType == TEST_TYPE_INVOCATION_PER_LAYER || params.testType == TEST_TYPE_MULTIPLE_LAYERS_PER_INVOCATION)
			src << "layout(points, invocations = " << numLayers << ") in;\n";
		else
			src << "layout(points) in;\n";

		src << "layout(triangle_strip, max_vertices = " << maxVertices << ") out;\n"
			<< "\n"
			<< (geomOutputColor ? "layout(location = 0) out vec4 vert_color;\n\n" : "")
			<< "out gl_PerVertex {\n"
			<< "    vec4 gl_Position;\n"
			<< "};\n"
			<< "\n"
			<< "void main(void)\n"
			<< "{\n";

		std::ostringstream colorTable;
		{
			const int numColors = DE_LENGTH_OF_ARRAY(s_colors);

			colorTable << "    const vec4 colors[" << numColors << "] = vec4[" << numColors << "](";

			const std::string padding(colorTable.str().length(), ' ');

			for (int i = 0; i < numColors; ++i)
				colorTable << (i != 0 ? ",\n" + padding : "") << toGlsl(s_colors[i]);

			colorTable << ");\n";
		}

		if (params.testType == TEST_TYPE_DEFAULT_LAYER)
		{
			src << "    gl_Position = vec4(-1.0, -1.0, 0.0, 1.0);\n"
				<< "    EmitVertex();\n"
				<< "\n"
				<< "    gl_Position = vec4(-1.0,  1.0, 0.0, 1.0);\n"
				<< "    EmitVertex();\n"
				<< "\n"
				<< "    gl_Position = vec4( 0.0, -1.0, 0.0, 1.0);\n"
				<< "    EmitVertex();\n"
				<< "\n"
				<< "    gl_Position = vec4( 0.0,  1.0, 0.0, 1.0);\n"
				<< "    EmitVertex();\n";
		}
		else if (params.testType == TEST_TYPE_SINGLE_LAYER)
		{
			const deUint32 targetLayer = getTargetLayer(params.image);

			src << "    gl_Position = vec4(-1.0, -1.0, 0.0, 1.0);\n"
				<< "    gl_Layer    = " << targetLayer << ";\n"
				<< "    EmitVertex();\n"
				<< "\n"
				<< "    gl_Position = vec4(-1.0,  1.0, 0.0, 1.0);\n"
				<< "    gl_Layer    = " << targetLayer << ";\n"
				<< "    EmitVertex();\n"
				<< "\n"
				<< "    gl_Position = vec4( 0.0, -1.0, 0.0, 1.0);\n"
				<< "    gl_Layer    = " << targetLayer << ";\n"
				<< "    EmitVertex();\n"
				<< "\n"
				<< "    gl_Position = vec4( 0.0,  1.0, 0.0, 1.0);\n"
				<< "    gl_Layer    = " << targetLayer << ";\n"
				<< "    EmitVertex();\n";
		}
		else if (params.testType == TEST_TYPE_ALL_LAYERS)
		{
			src << colorTable.str()
				<< "\n"
				<< "    for (int layerNdx = 0; layerNdx < " << numLayers << "; ++layerNdx) {\n"
				<< "        const int colorNdx = layerNdx % " << DE_LENGTH_OF_ARRAY(s_colors) << ";\n"
				<< "\n"
				<< "        gl_Position = vec4(-1.0, -1.0, 0.0, 1.0);\n"
				<< "        gl_Layer    = layerNdx;\n"
				<< "        vert_color  = colors[colorNdx];\n"
				<< "        EmitVertex();\n"
				<< "\n"
				<< "        gl_Position = vec4(-1.0,  1.0, 0.0, 1.0);\n"
				<< "        gl_Layer    = layerNdx;\n"
				<< "        vert_color  = colors[colorNdx];\n"
				<< "        EmitVertex();\n"
				<< "\n"
				<< "        gl_Position = vec4( 0.0, -1.0, 0.0, 1.0);\n"
				<< "        gl_Layer    = layerNdx;\n"
				<< "        vert_color  = colors[colorNdx];\n"
				<< "        EmitVertex();\n"
				<< "\n"
				<< "        gl_Position = vec4( 0.0,  1.0, 0.0, 1.0);\n"
				<< "        gl_Layer    = layerNdx;\n"
				<< "        vert_color  = colors[colorNdx];\n"
				<< "        EmitVertex();\n"
				<< "        EndPrimitive();\n"
				<< "    };\n";
		}
		else if (params.testType == TEST_TYPE_LAYER_ID)
		{
			src << "    for (int layerNdx = 0; layerNdx < " << numLayers << "; ++layerNdx) {\n"
				<< "        gl_Position = vec4(-1.0, -1.0, 0.0, 1.0);\n"
				<< "        gl_Layer    = layerNdx;\n"
				<< "        EmitVertex();\n"
				<< "\n"
				<< "        gl_Position = vec4(-1.0,  1.0, 0.0, 1.0);\n"
				<< "        gl_Layer    = layerNdx;\n"
				<< "        EmitVertex();\n"
				<< "\n"
				<< "        gl_Position = vec4( 0.0, -1.0, 0.0, 1.0);\n"
				<< "        gl_Layer    = layerNdx;\n"
				<< "        EmitVertex();\n"
				<< "\n"
				<< "        gl_Position = vec4( 0.0,  1.0, 0.0, 1.0);\n"
				<< "        gl_Layer    = layerNdx;\n"
				<< "        EmitVertex();\n"
				<< "        EndPrimitive();\n"
				<< "    };\n";
		}
		else if (params.testType == TEST_TYPE_DIFFERENT_CONTENT)
		{
			src << "    for (int layerNdx = 0; layerNdx < " << numLayers << "; ++layerNdx) {\n"
				<< "        for (int colNdx = 0; colNdx <= layerNdx; ++colNdx) {\n"
				<< "            const float posX = float(colNdx) / float(" << numLayers << ") * 2.0 - 1.0;\n"
				<< "\n"
				<< "            gl_Position = vec4(posX,  1.0, 0.0, 1.0);\n"
				<< "            gl_Layer    = layerNdx;\n"
				<< "            EmitVertex();\n"
				<< "\n"
				<< "            gl_Position = vec4(posX, -1.0, 0.0, 1.0);\n"
				<< "            gl_Layer    = layerNdx;\n"
				<< "            EmitVertex();\n"
				<< "        }\n"
				<< "        EndPrimitive();\n"
				<< "    }\n";
		}
		else if (params.testType == TEST_TYPE_INVOCATION_PER_LAYER)
		{
			src << colorTable.str()
				<< "    const int colorNdx = gl_InvocationID % " << DE_LENGTH_OF_ARRAY(s_colors) << ";\n"
				<< "\n"
				<< "    gl_Position = vec4(-1.0, -1.0, 0.0, 1.0);\n"
				<< "    gl_Layer    = gl_InvocationID;\n"
				<< "    vert_color  = colors[colorNdx];\n"
				<< "    EmitVertex();\n"
				<< "\n"
				<< "    gl_Position = vec4(-1.0,  1.0, 0.0, 1.0);\n"
				<< "    gl_Layer    = gl_InvocationID;\n"
				<< "    vert_color  = colors[colorNdx];\n"
				<< "    EmitVertex();\n"
				<< "\n"
				<< "    gl_Position = vec4( 0.0, -1.0, 0.0, 1.0);\n"
				<< "    gl_Layer    = gl_InvocationID;\n"
				<< "    vert_color  = colors[colorNdx];\n"
				<< "    EmitVertex();\n"
				<< "\n"
				<< "    gl_Position = vec4( 0.0,  1.0, 0.0, 1.0);\n"
				<< "    gl_Layer    = gl_InvocationID;\n"
				<< "    vert_color  = colors[colorNdx];\n"
				<< "    EmitVertex();\n"
				<< "    EndPrimitive();\n";
		}
		else if (params.testType == TEST_TYPE_MULTIPLE_LAYERS_PER_INVOCATION)
		{
			src << "    const int   layerA = gl_InvocationID;\n"
				<< "    const int   layerB = (gl_InvocationID + 1) % " << numLayers << ";\n"
				<< "    const float aEnd   = float(layerA) / float(" << numLayers << ") * 2.0 - 1.0;\n"
				<< "    const float bEnd   = float(layerB) / float(" << numLayers << ") * 2.0 - 1.0;\n"
				<< "\n"
				<< "    gl_Position = vec4(-1.0, -1.0, 0.0, 1.0);\n"
				<< "    gl_Layer    = layerA;\n"
				<< "    EmitVertex();\n"
				<< "\n"
				<< "    gl_Position = vec4(-1.0,  1.0, 0.0, 1.0);\n"
				<< "    gl_Layer    = layerA;\n"
				<< "    EmitVertex();\n"
				<< "\n"
				<< "    gl_Position = vec4(aEnd, -1.0, 0.0, 1.0);\n"
				<< "    gl_Layer    = layerA;\n"
				<< "    EmitVertex();\n"
				<< "    EndPrimitive();\n"
				<< "\n"
				<< "    gl_Position = vec4(-1.0,  1.0, 0.0, 1.0);\n"
				<< "    gl_Layer    = layerB;\n"
				<< "    EmitVertex();\n"
				<< "\n"
				<< "    gl_Position = vec4(bEnd,  1.0, 0.0, 1.0);\n"
				<< "    gl_Layer    = layerB;\n"
				<< "    EmitVertex();\n"
				<< "\n"
				<< "    gl_Position = vec4(bEnd, -1.0, 0.0, 1.0);\n"
				<< "    gl_Layer    = layerB;\n"
				<< "    EmitVertex();\n"
				<< "    EndPrimitive();\n";
		}
		else
			DE_ASSERT(0);

		src <<	"}\n";	// end main

		programCollection.glslSources.add("geom") << glu::GeometrySource(src.str());
	}

	// Fragment shader
	{
		std::ostringstream src;
		src << glu::getGLSLVersionDeclaration(glu::GLSL_VERSION_450) << "\n"
			<< "\n"
			<< "layout(location = 0) out vec4 o_color;\n"
			<< (geomOutputColor ? "layout(location = 0) in  vec4 vert_color;\n" : "")
			<< "\n"
			<< "void main(void)\n"
			<< "{\n";

		if (params.testType == TEST_TYPE_LAYER_ID)
		{
			// This code must be in sync with verifyLayerContent()
			src << "    o_color = vec4( (gl_Layer    % 2) == 1 ? 1.0 : 0.5,\n"
				<< "                   ((gl_Layer/2) % 2) == 1 ? 1.0 : 0.5,\n"
				<< "                     gl_Layer         == 0 ? 1.0 : 0.0,\n"
				<< "                                             1.0);\n";
		}
		else if (geomOutputColor)
			src << "    o_color = vert_color;\n";
		else
			src << "    o_color = vec4(1.0);\n";

		src << "}\n";

		programCollection.glslSources.add("frag") << glu::FragmentSource(src.str());
	}
}

tcu::TestStatus test (Context& context, const TestParams params)
{
	if (VK_IMAGE_VIEW_TYPE_3D == params.image.viewType &&
		(!de::contains(context.getDeviceExtensions().begin(), context.getDeviceExtensions().end(), "VK_KHR_maintenance1")))
		TCU_THROW(NotSupportedError, "Extension VK_KHR_maintenance1 not supported");

	const DeviceInterface&			vk						= context.getDeviceInterface();
	const InstanceInterface&		vki						= context.getInstanceInterface();
	const VkDevice					device					= context.getDevice();
	const VkPhysicalDevice			physDevice				= context.getPhysicalDevice();
	const deUint32					queueFamilyIndex		= context.getUniversalQueueFamilyIndex();
	const VkQueue					queue					= context.getUniversalQueue();
	Allocator&						allocator				= context.getDefaultAllocator();

	checkGeometryShaderSupport(vki, physDevice);

	const VkFormat					colorFormat				= VK_FORMAT_R8G8B8A8_UNORM;
	const deUint32					numLayers				= (VK_IMAGE_VIEW_TYPE_3D == params.image.viewType ? params.image.size.depth : params.image.numLayers);
	const Vec4						clearColor				= Vec4(0.0f, 0.0f, 0.0f, 1.0f);
	const VkDeviceSize				colorBufferSize			= params.image.size.width * params.image.size.height * params.image.size.depth * params.image.numLayers * tcu::getPixelSize(mapVkFormat(colorFormat));
	const VkImageCreateFlags		imageCreateFlags		= (isCubeImageViewType(params.image.viewType) ? VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT : (VkImageCreateFlagBits)0) |
															  (VK_IMAGE_VIEW_TYPE_3D == params.image.viewType ? VK_IMAGE_CREATE_2D_ARRAY_COMPATIBLE_BIT_KHR : (VkImageCreateFlagBits)0);
	const VkImageViewType			viewType				= (VK_IMAGE_VIEW_TYPE_3D == params.image.viewType ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : params.image.viewType);

	const Unique<VkImage>			colorImage				(makeImage				(vk, device, makeImageCreateInfo(imageCreateFlags, getImageType(params.image.viewType), colorFormat, params.image.size,
																					 params.image.numLayers, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT)));
	const UniquePtr<Allocation>		colorImageAlloc			(bindImage				(vk, device, allocator, *colorImage, MemoryRequirement::Any));
	const Unique<VkImageView>		colorAttachment			(makeImageView			(vk, device, *colorImage, viewType, colorFormat, makeImageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT, 0u, 1u, 0u, numLayers)));

	const Unique<VkBuffer>			colorBuffer				(makeBuffer				(vk, device, makeBufferCreateInfo(colorBufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT)));
	const UniquePtr<Allocation>		colorBufferAlloc		(bindBuffer				(vk, device, allocator, *colorBuffer, MemoryRequirement::HostVisible));

	const Unique<VkShaderModule>	vertexModule			(createShaderModule		(vk, device, context.getBinaryCollection().get("vert"), 0u));
	const Unique<VkShaderModule>	geometryModule			(createShaderModule		(vk, device, context.getBinaryCollection().get("geom"), 0u));
	const Unique<VkShaderModule>	fragmentModule			(createShaderModule		(vk, device, context.getBinaryCollection().get("frag"), 0u));

	const Unique<VkRenderPass>		renderPass				(makeRenderPass			(vk, device, colorFormat));
	const Unique<VkFramebuffer>		framebuffer				(makeFramebuffer		(vk, device, *renderPass, *colorAttachment, params.image.size.width,  params.image.size.height, numLayers));
	const Unique<VkPipelineLayout>	pipelineLayout			(makePipelineLayout		(vk, device));
	const Unique<VkPipeline>		pipeline				(makeGraphicsPipeline	(vk, device, *pipelineLayout, *renderPass, *vertexModule, *geometryModule, *fragmentModule,
																					 makeExtent2D(params.image.size.width, params.image.size.height)));
	const Unique<VkCommandPool>		cmdPool					(createCommandPool		(vk, device, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, queueFamilyIndex));
	const Unique<VkCommandBuffer>	cmdBuffer				(allocateCommandBuffer	(vk, device, *cmdPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY));

	zeroBuffer(vk, device, *colorBufferAlloc, colorBufferSize);

	beginCommandBuffer(vk, *cmdBuffer);

	const VkClearValue			clearValue	= makeClearValueColor(clearColor);
	const VkRect2D				renderArea	=
	{
		makeOffset2D(0, 0),
		makeExtent2D(params.image.size.width, params.image.size.height),
	};
	const VkRenderPassBeginInfo renderPassBeginInfo =
	{
		VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,		// VkStructureType         sType;
		DE_NULL,										// const void*             pNext;
		*renderPass,									// VkRenderPass            renderPass;
		*framebuffer,									// VkFramebuffer           framebuffer;
		renderArea,										// VkRect2D                renderArea;
		1u,												// uint32_t                clearValueCount;
		&clearValue,									// const VkClearValue*     pClearValues;
	};
	vk.cmdBeginRenderPass(*cmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

	vk.cmdBindPipeline(*cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *pipeline);
	vk.cmdDraw(*cmdBuffer, 1u, 1u, 0u, 0u);
	vk.cmdEndRenderPass(*cmdBuffer);

	// Prepare color image for copy
	{
		const VkImageSubresourceRange	colorSubresourceRange	= makeImageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT, 0u, 1u, 0u, params.image.numLayers);
		const VkImageMemoryBarrier		barriers[] =
		{
			{
				VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,			// VkStructureType			sType;
				DE_NULL,										// const void*				pNext;
				VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,			// VkAccessFlags			outputMask;
				VK_ACCESS_TRANSFER_READ_BIT,					// VkAccessFlags			inputMask;
				VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,		// VkImageLayout			oldLayout;
				VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,			// VkImageLayout			newLayout;
				VK_QUEUE_FAMILY_IGNORED,						// deUint32					srcQueueFamilyIndex;
				VK_QUEUE_FAMILY_IGNORED,						// deUint32					destQueueFamilyIndex;
				*colorImage,									// VkImage					image;
				colorSubresourceRange,							// VkImageSubresourceRange	subresourceRange;
			},
		};

		vk.cmdPipelineBarrier(*cmdBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0u,
			0u, DE_NULL, 0u, DE_NULL, DE_LENGTH_OF_ARRAY(barriers), barriers);
	}
	// Color image -> host buffer
	{
		const VkBufferImageCopy region =
		{
			0ull,																						// VkDeviceSize                bufferOffset;
			0u,																							// uint32_t                    bufferRowLength;
			0u,																							// uint32_t                    bufferImageHeight;
			makeImageSubresourceLayers(VK_IMAGE_ASPECT_COLOR_BIT, 0u, 0u, params.image.numLayers),		// VkImageSubresourceLayers    imageSubresource;
			makeOffset3D(0, 0, 0),																		// VkOffset3D                  imageOffset;
			params.image.size,																			// VkExtent3D                  imageExtent;
		};

		vk.cmdCopyImageToBuffer(*cmdBuffer, *colorImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, *colorBuffer, 1u, &region);
	}
	// Buffer write barrier
	{
		const VkBufferMemoryBarrier barriers[] =
		{
			{
				VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,		// VkStructureType    sType;
				DE_NULL,										// const void*        pNext;
				VK_ACCESS_TRANSFER_WRITE_BIT,					// VkAccessFlags      srcAccessMask;
				VK_ACCESS_HOST_READ_BIT,						// VkAccessFlags      dstAccessMask;
				VK_QUEUE_FAMILY_IGNORED,						// uint32_t           srcQueueFamilyIndex;
				VK_QUEUE_FAMILY_IGNORED,						// uint32_t           dstQueueFamilyIndex;
				*colorBuffer,									// VkBuffer           buffer;
				0ull,											// VkDeviceSize       offset;
				VK_WHOLE_SIZE,									// VkDeviceSize       size;
			},
		};

		vk.cmdPipelineBarrier(*cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_HOST_BIT, 0u,
			0u, DE_NULL, DE_LENGTH_OF_ARRAY(barriers), barriers, DE_NULL, 0u);
	}

	VK_CHECK(vk.endCommandBuffer(*cmdBuffer));
	submitCommandsAndWait(vk, device, queue, *cmdBuffer);

	invalidateMappedMemoryRange(vk, device, colorBufferAlloc->getMemory(), colorBufferAlloc->getOffset(), colorBufferSize);

	if (!verifyResults(context.getTestContext().getLog(), params, colorFormat, colorBufferAlloc->getHostPtr()))
		return tcu::TestStatus::fail("Rendered images are incorrect");
	else
		return tcu::TestStatus::pass("OK");
}

} // anonymous

tcu::TestCaseGroup* createLayeredRenderingTests (tcu::TestContext& testCtx)
{
	MovePtr<tcu::TestCaseGroup> group(new tcu::TestCaseGroup(testCtx, "layered", "Layered rendering tests."));

	const struct
	{
		TestType		test;
		const char*		name;
		const char*		description;
	} testTypes[] =
	{
		{ TEST_TYPE_DEFAULT_LAYER,					"render_to_default_layer",			"Render to the default layer"															},
		{ TEST_TYPE_SINGLE_LAYER,					"render_to_one",					"Render to one layer"																	},
		{ TEST_TYPE_ALL_LAYERS,						"render_to_all",					"Render to all layers"																	},
		{ TEST_TYPE_DIFFERENT_CONTENT,				"render_different_content",			"Render different data to different layers"												},
		{ TEST_TYPE_LAYER_ID,						"fragment_layer",					"Read gl_Layer in fragment shader"														},
		{ TEST_TYPE_INVOCATION_PER_LAYER,			"invocation_per_layer",				"Render to multiple layers with multiple invocations, one invocation per layer"			},
		{ TEST_TYPE_MULTIPLE_LAYERS_PER_INVOCATION,	"multiple_layers_per_invocation",	"Render to multiple layers with multiple invocations, multiple layers per invocation",	},
	};

	const ImageParams imageParams[] =
	{
		{ VK_IMAGE_VIEW_TYPE_1D_ARRAY,		{ 64,  1, 1 },	4	},
		{ VK_IMAGE_VIEW_TYPE_2D_ARRAY,		{ 64, 64, 1 },	4	},
		{ VK_IMAGE_VIEW_TYPE_CUBE,			{ 64, 64, 1 },	6	},
		{ VK_IMAGE_VIEW_TYPE_CUBE_ARRAY,	{ 64, 64, 1 },	2*6	},
		{ VK_IMAGE_VIEW_TYPE_3D,			{ 64, 64, 8 },	1	}
	};

	for (int imageParamNdx = 0; imageParamNdx < DE_LENGTH_OF_ARRAY(imageParams); ++imageParamNdx)
	{
		MovePtr<tcu::TestCaseGroup> viewTypeGroup(new tcu::TestCaseGroup(testCtx, getShortImageViewTypeName(imageParams[imageParamNdx].viewType).c_str(), ""));

		for (int testTypeNdx = 0; testTypeNdx < DE_LENGTH_OF_ARRAY(testTypes); ++testTypeNdx)
		{
			const TestParams params =
			{
				testTypes[testTypeNdx].test,
				imageParams[imageParamNdx],
			};
			addFunctionCaseWithPrograms(viewTypeGroup.get(), testTypes[testTypeNdx].name, testTypes[testTypeNdx].description, initPrograms, test, params);
		}

		group->addChild(viewTypeGroup.release());
	}

	return group.release();
}

} // geometry
} // vkt
