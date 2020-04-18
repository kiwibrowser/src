/*------------------------------------------------------------------------
 * Vulkan Conformance Tests
 * ------------------------
 *
 * Copyright (c) 2017 The Khronos Group Inc.
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
 * \file vktPipelineFramebuferAttachmentTests.cpp
 * \brief Render to a framebuffer with attachments of different sizes and with
 *        no attachments at all
 *
 *//*--------------------------------------------------------------------*/

#include "vktPipelineFramebufferAttachmentTests.hpp"
#include "vktPipelineMakeUtil.hpp"
#include "vktTestCase.hpp"
#include "vktTestCaseUtil.hpp"
#include "vktPipelineVertexUtil.hpp"
#include "vktTestGroupUtil.hpp"

#include "vkMemUtil.hpp"
#include "vkQueryUtil.hpp"
#include "vkTypeUtil.hpp"
#include "vkRefUtil.hpp"
#include "vkBuilderUtil.hpp"
#include "vkPrograms.hpp"
#include "vkImageUtil.hpp"

#include "tcuTextureUtil.hpp"
#include "tcuImageCompare.hpp"

#include "deUniquePtr.hpp"
#include "deSharedPtr.hpp"

#include <string>
#include <vector>

namespace vkt
{
namespace pipeline
{
namespace
{
using namespace vk;
using de::UniquePtr;
using de::MovePtr;
using de::SharedPtr;
using tcu::IVec3;
using tcu::Vec4;
using tcu::UVec4;
using tcu::IVec4;
using std::vector;

static const VkFormat COLOR_FORMAT	=		VK_FORMAT_R8G8B8A8_UNORM;

typedef SharedPtr<Unique<VkImageView> >	SharedPtrVkImageView;
typedef SharedPtr<Unique<VkPipeline> >	SharedPtrVkPipeline;

struct CaseDef
{
	VkImageViewType	imageType;
	IVec3			renderSize;
	IVec3			attachmentSize;
	deUint32		numLayers;
	bool			multisample;
};

template<typename T>
inline SharedPtr<Unique<T> > makeSharedPtr(Move<T> move)
{
	return SharedPtr<Unique<T> >(new Unique<T>(move));
}

template<typename T>
inline VkDeviceSize sizeInBytes(const vector<T>& vec)
{
	return vec.size() * sizeof(vec[0]);
}

VkImageType getImageType(const VkImageViewType viewType)
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

//! Make a render pass with one subpass per color attachment and one attachment per image layer.
Move<VkRenderPass> makeRenderPass (const DeviceInterface& vk,
								   const VkDevice		  device,
								   const VkFormat		  colorFormat,
								   const deUint32		  numLayers,
								   const bool			  multisample)
{
	vector<VkAttachmentDescription>	attachmentDescriptions		(numLayers);
	deUint32						attachmentIndex				= 0;
	vector<VkAttachmentReference>	colorAttachmentReferences	(numLayers);
	vector<VkSubpassDescription>	subpasses;

	for (deUint32 i = 0; i < numLayers; i++)
	{
		VkAttachmentDescription colorAttachmentDescription =
		{
			(VkAttachmentDescriptionFlags)0,								// VkAttachmentDescriptionFla	flags;
			colorFormat,													// VkFormat						format;
			!multisample ? VK_SAMPLE_COUNT_1_BIT : VK_SAMPLE_COUNT_4_BIT,	// VkSampleCountFlagBits		samples;
			VK_ATTACHMENT_LOAD_OP_LOAD,										// VkAttachmentLoadOp			loadOp;
			VK_ATTACHMENT_STORE_OP_STORE,									// VkAttachmentStoreOp			storeOp;
			VK_ATTACHMENT_LOAD_OP_DONT_CARE,								// VkAttachmentLoadOp			stencilLoadOp;
			VK_ATTACHMENT_STORE_OP_DONT_CARE,								// VkAttachmentStoreOp			stencilStoreOp;
			VK_IMAGE_LAYOUT_GENERAL,										// VkImageLayout				initialLayout;
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,						// VkImageLayout				finalLayout;
		};
		attachmentDescriptions[attachmentIndex++] = colorAttachmentDescription;
	}

	// Create a subpass for each attachment (each attachment is a layer of an arrayed image).
	for (deUint32 i = 0; i < numLayers; ++i)
	{
		const VkAttachmentReference attachmentRef =
		{
			i,											// deUint32			attachment;
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL	// VkImageLayout	layout;
		};
		colorAttachmentReferences[i] = attachmentRef;

		const VkSubpassDescription subpassDescription =
		{
			(VkSubpassDescriptionFlags)0,		// VkSubpassDescriptionFlags		flags;
			VK_PIPELINE_BIND_POINT_GRAPHICS,	// VkPipelineBindPoint				pipelineBindPoint;
			0u,									// deUint32							inputAttachmentCount;
			DE_NULL,							// const VkAttachmentReference*		pInputAttachments;
			1u,									// deUint32							colorAttachmentCount;
			&colorAttachmentReferences[i],		// const VkAttachmentReference*		pColorAttachments;
			DE_NULL,							// const VkAttachmentReference*		pResolveAttachments;
			DE_NULL,							// const VkAttachmentReference*		pDepthStencilAttachment;
			0u,									// deUint32							preserveAttachmentCount;
			DE_NULL								// const deUint32*					pPreserveAttachments;
		};
		subpasses.push_back(subpassDescription);
	}

	const VkRenderPassCreateInfo renderPassInfo =
	{
		VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,	// VkStructureType					sType;
		DE_NULL,									// const void*						pNext;
		(VkRenderPassCreateFlags)0,					// VkRenderPassCreateFlags			flags;
		numLayers,									// deUint32							attachmentCount;
		&attachmentDescriptions[0],					// const VkAttachmentDescription*	pAttachments;
		static_cast<deUint32>(subpasses.size()),	// deUint32							subpassCount;
		&subpasses[0],								// const VkSubpassDescription*		pSubpasses;
		0u,											// deUint32							dependencyCount;
		DE_NULL										// const VkSubpassDependency*		pDependencies;
	};

	return createRenderPass(vk, device, &renderPassInfo);
}

Move<VkPipeline> makeGraphicsPipeline (const DeviceInterface&		vk,
									   const VkDevice				device,
									   const VkPipelineLayout		pipelineLayout,
									   const VkRenderPass			renderPass,
									   const VkShaderModule			vertexModule,
									   const VkShaderModule			fragmentModule,
									   const IVec3					renderSize,
									   const VkPrimitiveTopology	topology,
									   const deUint32				subpass,
									   const bool					hasAttachments,
									   const bool					multisample)
{
	const VkVertexInputBindingDescription vertexInputBindingDescription =
	{
		0u,								// uint32_t				binding;
		sizeof(tcu::Vec4),				// uint32_t				stride;
		VK_VERTEX_INPUT_RATE_VERTEX,	// VkVertexInputRate	inputRate;
	};

	const VkVertexInputAttributeDescription vertexInputAttributeDescriptions[] =
	{
		{
			0u,								// uint32_t		location;
			0u,								// uint32_t		binding;
			VK_FORMAT_R32G32B32A32_SFLOAT,	// VkFormat		format;
			0u,								// uint32_t		offset;
		},
	};

	const VkPipelineVertexInputStateCreateInfo vertexInputStateInfo =
	{
		VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,	// VkStructureType							sType;
		DE_NULL,													// const void*								pNext;
		(VkPipelineVertexInputStateCreateFlags)0,					// VkPipelineVertexInputStateCreateFlags	flags;
		1u,															// uint32_t									vertexBindingDescriptionCount;
		&vertexInputBindingDescription,								// const VkVertexInputBindingDescription*	pVertexBindingDescriptions;
		DE_LENGTH_OF_ARRAY(vertexInputAttributeDescriptions),		// uint32_t									vertexAttributeDescriptionCount;
		vertexInputAttributeDescriptions,							// const VkVertexInputAttributeDescription*	pVertexAttributeDescriptions;
	};

	const VkPipelineInputAssemblyStateCreateInfo pipelineInputAssemblyStateInfo =
	{
		VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,	// VkStructureType							sType;
		DE_NULL,														// const void*								pNext;
		(VkPipelineInputAssemblyStateCreateFlags)0,						// VkPipelineInputAssemblyStateCreateFlags	flags;
		topology,														// VkPrimitiveTopology						topology;
		VK_FALSE,														// VkBool32									primitiveRestartEnable;
	};

	const VkViewport viewport = makeViewport(
		0.0f, 0.0f,
		static_cast<float>(renderSize.x()), static_cast<float>(renderSize.y()),
		0.0f, 1.0f);

	// We must set the scissor rect to the renderSize, since the renderArea specified
	// during the begin render pass command does not ensure that rendering is strictly
	// limited to the rect provided there and the spec clearly states that the scissor
	// must be configured appropriately to ensure this
	const VkRect2D scissor =
	{
		makeOffset2D(0, 0),
		makeExtent2D(renderSize.x(), renderSize.y()),
	};

	const VkPipelineViewportStateCreateInfo pipelineViewportStateInfo =
	{
		VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,	// VkStructureType						sType;
		DE_NULL,												// const void*							pNext;
		(VkPipelineViewportStateCreateFlags)0,					// VkPipelineViewportStateCreateFlags	flags;
		1u,														// uint32_t								viewportCount;
		&viewport,												// const VkViewport*					pViewports;
		1u,														// uint32_t								scissorCount;
		&scissor,												// const VkRect2D*						pScissors;
	};

	const VkPipelineRasterizationStateCreateInfo pipelineRasterizationStateInfo =
	{
		VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,	// VkStructureType							sType;
		DE_NULL,													// const void*								pNext;
		(VkPipelineRasterizationStateCreateFlags)0,					// VkPipelineRasterizationStateCreateFlags	flags;
		VK_FALSE,													// VkBool32									depthClampEnable;
		VK_FALSE,													// VkBool32									rasterizerDiscardEnable;
		VK_POLYGON_MODE_FILL,										// VkPolygonMode							polygonMode;
		VK_CULL_MODE_NONE,											// VkCullModeFlags							cullMode;
		VK_FRONT_FACE_COUNTER_CLOCKWISE,							// VkFrontFace								frontFace;
		VK_FALSE,													// VkBool32									depthBiasEnable;
		0.0f,														// float									depthBiasConstantFactor;
		0.0f,														// float									depthBiasClamp;
		0.0f,														// float									depthBiasSlopeFactor;
		1.0f,														// float									lineWidth;
	};

	const VkPipelineMultisampleStateCreateInfo pipelineMultisampleStateInfo =
	{
		VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,		// VkStructureType							sType;
		DE_NULL,														// const void*								pNext;
		(VkPipelineMultisampleStateCreateFlags)0,						// VkPipelineMultisampleStateCreateFlags	flags;
		!multisample ? VK_SAMPLE_COUNT_1_BIT : VK_SAMPLE_COUNT_4_BIT,	// VkSampleCountFlagBits					rasterizationSamples;
		!multisample ? VK_FALSE : VK_TRUE,								// VkBool32									sampleShadingEnable;
		1.0f,															// float									minSampleShading;
		DE_NULL,														// const VkSampleMask*						pSampleMask;
		VK_FALSE,														// VkBool32									alphaToCoverageEnable;
		VK_FALSE														// VkBool32									alphaToOneEnable;
	};

	const VkStencilOpState stencilOpState = makeStencilOpState(
		VK_STENCIL_OP_KEEP,		// stencil fail
		VK_STENCIL_OP_KEEP,		// depth & stencil pass
		VK_STENCIL_OP_KEEP,		// depth only fail
		VK_COMPARE_OP_ALWAYS,	// compare op
		0u,						// compare mask
		0u,						// write mask
		0u);					// reference

	const VkPipelineDepthStencilStateCreateInfo pipelineDepthStencilStateInfo =
	{
		VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,	// VkStructureType							sType;
		DE_NULL,													// const void*								pNext;
		(VkPipelineDepthStencilStateCreateFlags)0,					// VkPipelineDepthStencilStateCreateFlags	flags;
		VK_FALSE,													// VkBool32									depthTestEnable;
		VK_FALSE,													// VkBool32									depthWriteEnable;
		VK_COMPARE_OP_LESS,											// VkCompareOp								depthCompareOp;
		VK_FALSE,													// VkBool32									depthBoundsTestEnable;
		VK_FALSE,													// VkBool32									stencilTestEnable;
		stencilOpState,												// VkStencilOpState							front;
		stencilOpState,												// VkStencilOpState							back;
		0.0f,														// float									minDepthBounds;
		1.0f,														// float									maxDepthBounds;
	};

	const VkColorComponentFlags colorComponentsAll = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	// Number of blend attachments must equal the number of color attachments during any subpass.
	const VkPipelineColorBlendAttachmentState pipelineColorBlendAttachmentState =
	{
		VK_FALSE,					// VkBool32					blendEnable;
		VK_BLEND_FACTOR_ONE,		// VkBlendFactor			srcColorBlendFactor;
		VK_BLEND_FACTOR_ZERO,		// VkBlendFactor			dstColorBlendFactor;
		VK_BLEND_OP_ADD,			// VkBlendOp				colorBlendOp;
		VK_BLEND_FACTOR_ONE,		// VkBlendFactor			srcAlphaBlendFactor;
		VK_BLEND_FACTOR_ZERO,		// VkBlendFactor			dstAlphaBlendFactor;
		VK_BLEND_OP_ADD,			// VkBlendOp				alphaBlendOp;
		colorComponentsAll,			// VkColorComponentFlags	colorWriteMask;
	};

	const VkPipelineColorBlendStateCreateInfo pipelineColorBlendStateInfo =
	{
		VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,	// VkStructureType								sType;
		DE_NULL,													// const void*									pNext;
		(VkPipelineColorBlendStateCreateFlags)0,					// VkPipelineColorBlendStateCreateFlags			flags;
		VK_FALSE,													// VkBool32										logicOpEnable;
		VK_LOGIC_OP_COPY,											// VkLogicOp									logicOp;
		hasAttachments ? 1u : 0u,									// deUint32										attachmentCount;
		&pipelineColorBlendAttachmentState,							// const VkPipelineColorBlendAttachmentState*	pAttachments;
		{ 0.0f, 0.0f, 0.0f, 0.0f },									// float										blendConstants[4];
	};

	const VkPipelineShaderStageCreateInfo pShaderStages[] =
	{
		{
			VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,	// VkStructureType						sType;
			DE_NULL,												// const void*							pNext;
			(VkPipelineShaderStageCreateFlags)0,					// VkPipelineShaderStageCreateFlags		flags;
			VK_SHADER_STAGE_VERTEX_BIT,								// VkShaderStageFlagBits				stage;
			vertexModule,											// VkShaderModule						module;
			"main",													// const char*							pName;
			DE_NULL,												// const VkSpecializationInfo*			pSpecializationInfo;
		},
		{
			VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,	// VkStructureType						sType;
			DE_NULL,												// const void*							pNext;
			(VkPipelineShaderStageCreateFlags)0,					// VkPipelineShaderStageCreateFlags		flags;
			VK_SHADER_STAGE_FRAGMENT_BIT,							// VkShaderStageFlagBits				stage;
			fragmentModule,											// VkShaderModule						module;
			"main",													// const char*							pName;
			DE_NULL,												// const VkSpecializationInfo*			pSpecializationInfo;
		}
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
		subpass,											// deUint32											subpass;
		DE_NULL,											// VkPipeline										basePipelineHandle;
		0,													// deInt32											basePipelineIndex;
	};

	return createGraphicsPipeline(vk, device, DE_NULL, &graphicsPipelineInfo);
}

Move<VkImage> makeImage (const DeviceInterface&		vk,
						 const VkDevice				device,
						 const VkImageCreateFlags	flags,
						 const VkImageType			imageType,
						 const VkFormat				format,
						 const IVec3&				size,
						 const deUint32				numLayers,
						 const VkImageUsageFlags	usage,
						 const bool					multisample)
{
	const VkImageCreateInfo imageParams =
	{
		VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,							// VkStructureType			sType;
		DE_NULL,														// const void*				pNext;
		flags,															// VkImageCreateFlags		flags;
		imageType,														// VkImageType				imageType;
		format,															// VkFormat					format;
		makeExtent3D(size),												// VkExtent3D				extent;
		1u,																// deUint32					mipLevels;
		numLayers,														// deUint32					arrayLayers;
		multisample ? VK_SAMPLE_COUNT_4_BIT : VK_SAMPLE_COUNT_1_BIT,	// VkSampleCountFlagBits	samples;
		VK_IMAGE_TILING_OPTIMAL,										// VkImageTiling			tiling;
		usage,															// VkImageUsageFlags		usage;
		VK_SHARING_MODE_EXCLUSIVE,										// VkSharingMode			sharingMode;
		0u,																// deUint32					queueFamilyIndexCount;
		DE_NULL,														// const deUint32*			pQueueFamilyIndices;
		VK_IMAGE_LAYOUT_UNDEFINED,										// VkImageLayout			initialLayout;
	};

	return createImage(vk, device, &imageParams);
}

vector<tcu::Vec4> genFullQuadVertices (const int subpassCount)
{
	vector<tcu::Vec4>	vectorData;
	for (int subpassNdx = 0; subpassNdx < subpassCount; ++subpassNdx)
	{
		vectorData.push_back(Vec4(-1.0f, -1.0f, 0.0f, 1.0f));
		vectorData.push_back(Vec4(-1.0f,  1.0f, 0.0f, 1.0f));
		vectorData.push_back(Vec4(1.0f, -1.0f, 0.0f, 1.0f));
		vectorData.push_back(Vec4(1.0f,  1.0f, 0.0f, 1.0f));
	}
	return vectorData;
}

void initColorPrograms (SourceCollections& programCollection, const CaseDef caseDef)
{
	(void)caseDef;

	// Vertex shader
	{
		std::ostringstream src;
		src << glu::getGLSLVersionDeclaration(glu::GLSL_VERSION_450) << "\n"
			<< "\n"
			<< "layout(location = 0) in vec4 in_position;\n"
			<< "\n"
			<< "out gl_PerVertex {\n"
			<< "	vec4 gl_Position;\n"
			<< "};\n"
			<< "\n"
			<< "void main(void)\n"
			<< "{\n"
			<< "	gl_Position	= in_position;\n"
			<< "}\n";

		programCollection.glslSources.add("vert") << glu::VertexSource(src.str());
	}

	// Fragment shader
	{
		std::ostringstream src;
		src << glu::getGLSLVersionDeclaration(glu::GLSL_VERSION_450) << "\n"
			<< "\n"
			<< "layout(location = 0) out vec4 o_color;\n"
			<< "\n"
			<< "void main(void)\n"
			<< "{\n"
			<< "    o_color = vec4(1.0, 0.5, 0.25, 1.0);\n"
			<< "}\n";

		programCollection.glslSources.add("frag") << glu::FragmentSource(src.str());
	}
}

tcu::PixelBufferAccess getExpectedData (tcu::TextureLevel& textureLevel, const CaseDef& caseDef)
{
	const tcu::PixelBufferAccess	expectedImage	(textureLevel);
	const int						renderDepth		= deMax32(caseDef.renderSize.z(), caseDef.numLayers);

	for (int z = 0; z < expectedImage.getDepth(); ++z)
	{
		for (int y = 0; y < expectedImage.getHeight(); ++y)
		{
			for (int x = 0; x < expectedImage.getWidth(); ++x)
			{
				if (x < caseDef.renderSize.x() && y < caseDef.renderSize.y() && z < renderDepth)
					expectedImage.setPixel(tcu::Vec4(1.0, 0.5, 0.25, 1.0), x, y, z);
				else
					expectedImage.setPixel(tcu::Vec4(0.0, 0.0, 0.0, 1.0), x, y, z);
			}
		}
	}
	return expectedImage;
}

inline Move<VkBuffer> makeBuffer (const DeviceInterface& vk, const VkDevice device, const VkDeviceSize bufferSize, const VkBufferUsageFlags usage)
{
	const VkBufferCreateInfo	bufferCreateInfo	= makeBufferCreateInfo(bufferSize, usage);
	return createBuffer(vk, device, &bufferCreateInfo);
}

inline VkImageSubresourceRange makeColorSubresourceRange (const deUint32 baseArrayLayer, const deUint32 layerCount)
{
	return makeImageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT, 0u, 1u, baseArrayLayer, layerCount);
}

// Tests rendering to a a framebuffer with color attachments larger than the
// framebuffer dimensions and verifies that rendering does not affect the areas
// of the attachment outside the framebuffer dimensions. Tests both single-sample
// and multi-sample configurations.
tcu::TestStatus test (Context& context, const CaseDef caseDef)
{
	const DeviceInterface&			vk					= context.getDeviceInterface();
	const VkDevice					device				= context.getDevice();
	const VkQueue					queue				= context.getUniversalQueue();
	const deUint32					queueFamilyIndex	= context.getUniversalQueueFamilyIndex();
	Allocator&						allocator			= context.getDefaultAllocator();

	// Color image for rendering in single-sample tests or resolve target for multi-sample tests
	Move<VkImage>					colorImage;
	MovePtr<Allocation>				colorImageAlloc;

	// For multisampled tests, this is the rendering target
	Move<VkImage>					msColorImage;
	MovePtr<Allocation>				msColorImageAlloc;

	// Host memory buffer where we will copy the rendered image for verification
	const deUint32					att_size_x			= caseDef.attachmentSize.x();
	const deUint32					att_size_y			= caseDef.attachmentSize.y();
	const deUint32					att_size_z			= caseDef.attachmentSize.z();
	const VkDeviceSize				colorBufferSize		= att_size_x * att_size_y * att_size_z * caseDef.numLayers * tcu::getPixelSize(mapVkFormat(COLOR_FORMAT));
	const Unique<VkBuffer>			colorBuffer			(makeBuffer(vk, device, colorBufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT));
	const UniquePtr<Allocation>		colorBufferAlloc	(bindBuffer(vk, device, allocator, *colorBuffer, MemoryRequirement::HostVisible));

	Move<VkBuffer>					vertexBuffer;
	MovePtr<Allocation>				vertexBufferAlloc;

	vector<SharedPtrVkImageView>	colorAttachments;
	vector<VkImageView>				attachmentHandles;

	const Unique<VkPipelineLayout>	pipelineLayout		(makePipelineLayout(vk, device));
	vector<SharedPtrVkPipeline>		pipeline;
	const Unique<VkRenderPass>		renderPass			(makeRenderPass(vk, device, COLOR_FORMAT, caseDef.numLayers, caseDef.multisample));
	Move<VkFramebuffer>				framebuffer;

	const Unique<VkShaderModule>	vertexModule		(createShaderModule(vk, device, context.getBinaryCollection().get("vert"), 0u));
	const Unique<VkShaderModule>	fragmentModule		(createShaderModule(vk, device, context.getBinaryCollection().get("frag"), 0u));

	const Unique<VkCommandPool>		cmdPool				(createCommandPool(vk, device, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, queueFamilyIndex));
	const Unique<VkCommandBuffer>	cmdBuffer			(makeCommandBuffer(vk, device, *cmdPool));

	const VkImageViewType			imageViewType		= caseDef.imageType == VK_IMAGE_VIEW_TYPE_CUBE || caseDef.imageType == VK_IMAGE_VIEW_TYPE_CUBE_ARRAY
		? VK_IMAGE_VIEW_TYPE_2D : caseDef.imageType;

	// create vertexBuffer
	{
		const vector<tcu::Vec4>	vertices			= genFullQuadVertices(caseDef.numLayers);
		const VkDeviceSize		vertexBufferSize	= sizeInBytes(vertices);

		vertexBuffer		= makeBuffer(vk, device, vertexBufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
		vertexBufferAlloc	= bindBuffer(vk, device, allocator, *vertexBuffer, MemoryRequirement::HostVisible);

		deMemcpy(vertexBufferAlloc->getHostPtr(), &vertices[0], static_cast<std::size_t>(vertexBufferSize));
		flushMappedMemoryRange(vk, device, vertexBufferAlloc->getMemory(), vertexBufferAlloc->getOffset(), vertexBufferSize);
	}

	// create colorImage (and msColorImage) using the configured attachmentsize
	{
		const VkImageUsageFlags	colorImageUsage	= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

		colorImage		= makeImage(vk, device, VkImageViewCreateFlags(0), getImageType(caseDef.imageType), COLOR_FORMAT,
			caseDef.attachmentSize, caseDef.numLayers, colorImageUsage, false);
		colorImageAlloc	= bindImage(vk, device, allocator, *colorImage, MemoryRequirement::Any);

		if (caseDef.multisample)
		{
			const VkImageUsageFlags	msImageUsage	= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

			msColorImage		= makeImage(vk, device, VkImageViewCreateFlags(0), getImageType(caseDef.imageType), COLOR_FORMAT,
				caseDef.attachmentSize, caseDef.numLayers, msImageUsage, true);
			msColorImageAlloc	= bindImage(vk, device, allocator, *msColorImage, MemoryRequirement::Any);
		}
	}

	// create attachmentHandles and pipelines (one for each layer). We use the renderSize for viewport and scissor
	for (deUint32 layerNdx = 0; layerNdx < caseDef.numLayers; ++layerNdx)
	{
		colorAttachments.push_back(makeSharedPtr(makeImageView(vk, device, ! caseDef.multisample ? *colorImage : *msColorImage,
			imageViewType, COLOR_FORMAT, makeColorSubresourceRange(layerNdx, 1))));
		attachmentHandles.push_back(**colorAttachments.back());

		pipeline.push_back(makeSharedPtr(makeGraphicsPipeline(vk, device, *pipelineLayout, *renderPass, *vertexModule, *fragmentModule,
			caseDef.renderSize, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP, layerNdx, true, caseDef.multisample)));
	}

	// create framebuffer
	framebuffer = makeFramebuffer(vk, device, *renderPass, caseDef.numLayers, &attachmentHandles[0],
		static_cast<deUint32>(caseDef.renderSize.x()), static_cast<deUint32>(caseDef.renderSize.y()));

	// record command buffer
	beginCommandBuffer(vk, *cmdBuffer);
	{
		// Clear the entire image attachment to black
		{
			const VkImageMemoryBarrier	imageLayoutBarriers[]	=
			{
				{
					VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,				// VkStructureType			sType;
					DE_NULL,											// const void*				pNext;
					0u,													// VkAccessFlags			srcAccessMask;
					VK_ACCESS_TRANSFER_WRITE_BIT,						// VkAccessFlags			dstAccessMask;
					VK_IMAGE_LAYOUT_UNDEFINED,							// VkImageLayout			oldLayout;
					VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,				// VkImageLayout			newLayout;
					VK_QUEUE_FAMILY_IGNORED,							// deUint32					srcQueueFamilyIndex;
					VK_QUEUE_FAMILY_IGNORED,							// deUint32					destQueueFamilyIndex;
					caseDef.multisample ? *msColorImage : *colorImage,	// VkImage					image;
					makeColorSubresourceRange(0, caseDef.numLayers)		// VkImageSubresourceRange	subresourceRange;
				},
			};

			vk.cmdPipelineBarrier(*cmdBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, 0u,
				0u, DE_NULL, 0u, DE_NULL, 1u, imageLayoutBarriers);

			const VkImageSubresourceRange	ranges		= makeColorSubresourceRange(0, caseDef.numLayers);
			const VkClearColorValue			clearColor	=
	        {
                {0.0f, 0.0f, 0.0f, 1.0f}
			};
			vk.cmdClearColorImage(*cmdBuffer, caseDef.multisample ? *msColorImage : *colorImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clearColor, 1u, &ranges);

			const VkImageMemoryBarrier	imageClearBarriers[]	=
			{
				{
					VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,				// VkStructureType			sType;
					DE_NULL,											// const void*				pNext;
					VK_ACCESS_TRANSFER_WRITE_BIT,						// VkAccessFlags			srcAccessMask;
					VK_ACCESS_COLOR_ATTACHMENT_READ_BIT,				// VkAccessFlags			dstAccessMask;
					VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,				// VkImageLayout			oldLayout;
					VK_IMAGE_LAYOUT_GENERAL,							// VkImageLayout			newLayout;
					VK_QUEUE_FAMILY_IGNORED,							// deUint32					srcQueueFamilyIndex;
					VK_QUEUE_FAMILY_IGNORED,							// deUint32					destQueueFamilyIndex;
					caseDef.multisample ? *msColorImage : *colorImage,	// VkImage					image;
					makeColorSubresourceRange(0, caseDef.numLayers)		// VkImageSubresourceRange	subresourceRange;
				},
			};

			vk.cmdPipelineBarrier(*cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0u,
				0u, DE_NULL, 0u, DE_NULL, 1u, imageClearBarriers);
		}

		// Render pass: this should render only to the area defined by renderSize (smaller than the size of the attachment)
		{
			const VkRect2D				renderArea			=
			{
				makeOffset2D(0, 0),
				makeExtent2D(caseDef.renderSize.x(), caseDef.renderSize.y()),
			};
			const VkRenderPassBeginInfo renderPassBeginInfo =
			{
				VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,	// VkStructureType         sType;
				DE_NULL,									// const void*             pNext;
				*renderPass,								// VkRenderPass            renderPass;
				*framebuffer,								// VkFramebuffer           framebuffer;
				renderArea,									// VkRect2D                renderArea;
				0,											// uint32_t                clearValueCount;
				DE_NULL,									// const VkClearValue*     pClearValues;
			};
			const VkDeviceSize			vertexBufferOffset	= 0ull;

			vk.cmdBeginRenderPass(*cmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
			{
				vk.cmdBindVertexBuffers(*cmdBuffer, 0u, 1u, &vertexBuffer.get(), &vertexBufferOffset);
				for (deUint32 layerNdx = 0; layerNdx < caseDef.numLayers; ++layerNdx)
				{
					if (layerNdx != 0)
						vk.cmdNextSubpass(*cmdBuffer, VK_SUBPASS_CONTENTS_INLINE);

					vk.cmdBindPipeline(*cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, **pipeline[layerNdx]);
					vk.cmdDraw(*cmdBuffer, 4u, 1u, layerNdx*4u, 0u);
				}
			}
			vk.cmdEndRenderPass(*cmdBuffer);
		}

		// If we are using a multi-sampled render target (msColorImage), resolve it now (to colorImage)
		if (caseDef.multisample)
		{
			// Transition msColorImage (from layout COLOR_ATTACHMENT_OPTIMAL) and colorImage (from layout UNDEFINED) to layout GENERAL before resolving
			const VkImageMemoryBarrier	imageBarriers[]	=
			{
				{
					VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,			// VkStructureType			sType;
					DE_NULL,										// const void*				pNext;
					VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,			// VkAccessFlags			srcAccessMask;
					VK_ACCESS_TRANSFER_READ_BIT,					// VkAccessFlags			dstAccessMask;
					VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,		// VkImageLayout			oldLayout;
					VK_IMAGE_LAYOUT_GENERAL,						// VkImageLayout			newLayout;
					VK_QUEUE_FAMILY_IGNORED,						// deUint32					srcQueueFamilyIndex;
					VK_QUEUE_FAMILY_IGNORED,						// deUint32					destQueueFamilyIndex;
					*msColorImage,									// VkImage					image;
					makeColorSubresourceRange(0, caseDef.numLayers)	// VkImageSubresourceRange	subresourceRange;
				},
				{
					VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,			// VkStructureType			sType;
					DE_NULL,										// const void*				pNext;
					(VkAccessFlags)0,								// VkAccessFlags			srcAccessMask;
					VK_ACCESS_TRANSFER_WRITE_BIT,					// VkAccessFlags			dstAccessMask;
					VK_IMAGE_LAYOUT_UNDEFINED,						// VkImageLayout			oldLayout;
					VK_IMAGE_LAYOUT_GENERAL,						// VkImageLayout			newLayout;
					VK_QUEUE_FAMILY_IGNORED,						// deUint32					srcQueueFamilyIndex;
					VK_QUEUE_FAMILY_IGNORED,						// deUint32					destQueueFamilyIndex;
					*colorImage,									// VkImage					image;
					makeColorSubresourceRange(0, caseDef.numLayers)	// VkImageSubresourceRange	subresourceRange;
				}
			};

			vk.cmdPipelineBarrier(*cmdBuffer, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0u,
				0u, DE_NULL, 0u, DE_NULL, 2u, imageBarriers);

			const VkImageResolve	region	=
			{
				makeImageSubresourceLayers(VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, caseDef.numLayers),	// VkImageSubresourceLayers    srcSubresource;
				makeOffset3D(0, 0, 0),															// VkOffset3D                  srcOffset;
				makeImageSubresourceLayers(VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, caseDef.numLayers),	// VkImageSubresourceLayers    dstSubresource;
				makeOffset3D(0, 0, 0),															// VkOffset3D                  dstOffset;
				makeExtent3D(caseDef.attachmentSize)											// VkExtent3D                  extent;
			};

			vk.cmdResolveImage(*cmdBuffer, *msColorImage, VK_IMAGE_LAYOUT_GENERAL, *colorImage, VK_IMAGE_LAYOUT_GENERAL, 1, &region);
		}

		// copy colorImage to host visible colorBuffer
		{
			const VkImageMemoryBarrier	imageBarriers[]		=
			{
				{
					VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,														// VkStructureType			sType;
					DE_NULL,																					// const void*				pNext;
					(vk::VkAccessFlags)(caseDef.multisample ? VK_ACCESS_TRANSFER_WRITE_BIT : VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT),
					VK_ACCESS_TRANSFER_READ_BIT,																// VkAccessFlags			dstAccessMask;
					caseDef.multisample ? VK_IMAGE_LAYOUT_GENERAL : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,	// VkImageLayout			oldLayout;
					VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,														// VkImageLayout			newLayout;
					VK_QUEUE_FAMILY_IGNORED,																	// deUint32					srcQueueFamilyIndex;
					VK_QUEUE_FAMILY_IGNORED,																	// deUint32					destQueueFamilyIndex;
					*colorImage,																				// VkImage					image;
					makeColorSubresourceRange(0, caseDef.numLayers)												// VkImageSubresourceRange	subresourceRange;
				}
			};

			vk.cmdPipelineBarrier(*cmdBuffer, caseDef.multisample ? VK_PIPELINE_STAGE_TRANSFER_BIT : VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0u,
				0u, DE_NULL, 0u, DE_NULL, 1u, imageBarriers);

			const VkBufferImageCopy		region				=
			{
				0ull,																				// VkDeviceSize                bufferOffset;
				0u,																					// uint32_t                    bufferRowLength;
				0u,																					// uint32_t                    bufferImageHeight;
				makeImageSubresourceLayers(VK_IMAGE_ASPECT_COLOR_BIT, 0u, 0u, caseDef.numLayers),	// VkImageSubresourceLayers    imageSubresource;
				makeOffset3D(0, 0, 0),																// VkOffset3D                  imageOffset;
				makeExtent3D(caseDef.attachmentSize),												// VkExtent3D                  imageExtent;
			};

			vk.cmdCopyImageToBuffer(*cmdBuffer, *colorImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, *colorBuffer, 1u, &region);

			const VkBufferMemoryBarrier	bufferBarriers[]	=
			{
				{
					VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,	// VkStructureType    sType;
					DE_NULL,									// const void*        pNext;
					VK_ACCESS_TRANSFER_WRITE_BIT,				// VkAccessFlags      srcAccessMask;
					VK_ACCESS_HOST_READ_BIT,					// VkAccessFlags      dstAccessMask;
					VK_QUEUE_FAMILY_IGNORED,					// uint32_t           srcQueueFamilyIndex;
					VK_QUEUE_FAMILY_IGNORED,					// uint32_t           dstQueueFamilyIndex;
					*colorBuffer,								// VkBuffer           buffer;
					0ull,										// VkDeviceSize       offset;
					VK_WHOLE_SIZE,								// VkDeviceSize       size;
				},
			};

			vk.cmdPipelineBarrier(*cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_HOST_BIT, 0u,
				0u, DE_NULL, DE_LENGTH_OF_ARRAY(bufferBarriers), bufferBarriers, 0u, DE_NULL);
		}
	} // beginCommandBuffer

	VK_CHECK(vk.endCommandBuffer(*cmdBuffer));
	submitCommandsAndWait(vk, device, queue, *cmdBuffer);

	// Verify results
	{
		invalidateMappedMemoryRange(vk, device, colorBufferAlloc->getMemory(), colorBufferAlloc->getOffset(), VK_WHOLE_SIZE);
		const tcu::TextureFormat			format			= mapVkFormat(COLOR_FORMAT);
		const int							depth			= deMax32(caseDef.attachmentSize.z(), caseDef.numLayers);
		tcu::TextureLevel					textureLevel	(format, caseDef.attachmentSize.x(), caseDef.attachmentSize.y(), depth);
		const tcu::PixelBufferAccess		expectedImage	= getExpectedData(textureLevel, caseDef);
		const tcu::ConstPixelBufferAccess	resultImage		(format, caseDef.attachmentSize.x(), caseDef.attachmentSize.y(), depth, colorBufferAlloc->getHostPtr());

		if (!tcu::intThresholdCompare(context.getTestContext().getLog(), "Image Comparison", "", expectedImage, resultImage, tcu::UVec4(1), tcu::COMPARE_LOG_RESULT))
			return tcu::TestStatus::fail("Fail");
	}

	return tcu::TestStatus::pass("Pass");
}

void initImagePrograms (SourceCollections& programCollection, const bool multisample)
{
	// Vertex shader
	{
		std::ostringstream src;
		src << glu::getGLSLVersionDeclaration(glu::GLSL_VERSION_450) << "\n"
			<< "\n"
			<< "layout(location = 0) in vec4 in_position;\n"
			<< "\n"
			<< "out gl_PerVertex {\n"
			<< "	vec4 gl_Position;\n"
			<< "};\n"
			<< "\n"
			<< "void main(void)\n"
			<< "{\n"
			<< "	gl_Position	= in_position;\n"
			<< "}\n";

		programCollection.glslSources.add("vert") << glu::VertexSource(src.str());
	}

	// Fragment shader
	{
		std::ostringstream src;
		src << glu::getGLSLVersionDeclaration(glu::GLSL_VERSION_450) << "\n"
			<< "\n"
			<< "layout(binding = 0, rgba8) writeonly uniform image2D image;\n"
			<< "\n"
			<< "void main(void)\n"
			<< "{\n";
			if (!multisample)
				src << "    imageStore(image, ivec2(gl_PrimitiveID % 4, 0), vec4(1.0, 0.5, 0.25, 1.0));\n";
			else
				src << "    imageStore(image, ivec2(gl_PrimitiveID % 4, gl_SampleID % 4), vec4(1.0, 0.5, 0.25, 1.0));\n";
			src << "}\n";

		programCollection.glslSources.add("frag") << glu::FragmentSource(src.str());
	}
}

//! Make a render pass with no attachments
Move<VkRenderPass> makeRenderPassNoAtt (const DeviceInterface& vk, const VkDevice device)
{
	// Create a single subpass with no attachment references
	vector<VkSubpassDescription>	subpasses;

	const VkSubpassDescription		subpassDescription	=
	{
		(VkSubpassDescriptionFlags)0,		// VkSubpassDescriptionFlags		flags;
		VK_PIPELINE_BIND_POINT_GRAPHICS,	// VkPipelineBindPoint				pipelineBindPoint;
		0u,									// deUint32							inputAttachmentCount;
		DE_NULL,							// const VkAttachmentReference*		pInputAttachments;
		0u,									// deUint32							colorAttachmentCount;
		DE_NULL,							// const VkAttachmentReference*		pColorAttachments;
		DE_NULL,							// const VkAttachmentReference*		pResolveAttachments;
		DE_NULL,							// const VkAttachmentReference*		pDepthStencilAttachment;
		0u,									// deUint32							preserveAttachmentCount;
		DE_NULL								// const deUint32*					pPreserveAttachments;
	};
	subpasses.push_back(subpassDescription);

	const VkRenderPassCreateInfo	renderPassInfo	=
	{
		VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,	// VkStructureType					sType;
		DE_NULL,									// const void*						pNext;
		(VkRenderPassCreateFlags)0,					// VkRenderPassCreateFlags			flags;
		0,											// deUint32							attachmentCount;
		DE_NULL,									// const VkAttachmentDescription*	pAttachments;
		1,											// deUint32							subpassCount;
		&subpasses[0],								// const VkSubpassDescription*		pSubpasses;
		0u,											// deUint32							dependencyCount;
		DE_NULL										// const VkSubpassDependency*		pDependencies;
	};

	return createRenderPass(vk, device, &renderPassInfo);
}

tcu::PixelBufferAccess getExpectedDataNoAtt (tcu::TextureLevel& textureLevel)
{
	const tcu::PixelBufferAccess	expectedImage	(textureLevel);
	for (int z = 0; z < expectedImage.getDepth(); ++z)
	{
		for (int y = 0; y < expectedImage.getHeight(); ++y)
		{
			for (int x = 0; x < expectedImage.getWidth(); ++x)
			{
				expectedImage.setPixel(tcu::Vec4(1.0, 0.5, 0.25, 1.0), x, y, z);
			}
		}
	}
	return expectedImage;
}

vector<tcu::Vec4> genPointVertices (void)
{
	vector<tcu::Vec4>	vectorData;
	vectorData.push_back(Vec4(-0.25f, -0.25f, 0, 1));
	vectorData.push_back(Vec4(-0.25f,  0.25f, 0, 1));
	vectorData.push_back(Vec4(0.25f, -0.25f, 0, 1));
	vectorData.push_back(Vec4(0.25f,  0.25f, 0, 1));
	return vectorData;
}

// Tests rendering to a framebuffer without color attachments, checking that
// the fragment shader is run even in the absence of color output. In this case
// we render 4 point primitives and we make the fragment shader write to a
// different pixel of an image via an imageStore command. For the single-sampled
// configuration we use a 4x1 image to record the output and for the
// multi-sampled case we use a 4x4 image to record all 16 samples produced by
// 4-sample multi-sampling
tcu::TestStatus testNoAtt (Context& context, const bool multisample)
{
	const VkPhysicalDeviceFeatures		features				= context.getDeviceFeatures();
	if (!features.fragmentStoresAndAtomics)
		throw tcu::NotSupportedError("fragmentStoresAndAtomics feature not supported");
	if (!features.geometryShader && !features.tessellationShader) // Shader uses gl_PrimitiveID
		throw tcu::NotSupportedError("geometryShader or tessellationShader feature not supported");
	if (multisample && !features.sampleRateShading) // MS shader uses gl_SampleID
		throw tcu::NotSupportedError("sampleRateShading feature not supported");

	const DeviceInterface&				vk						= context.getDeviceInterface();
	const VkDevice						device					= context.getDevice();
	const VkQueue						queue					= context.getUniversalQueue();
	const deUint32						queueFamilyIndex		= context.getUniversalQueueFamilyIndex();
	Allocator&							allocator				= context.getDefaultAllocator();
	const IVec3							renderSize				(32, 32, 1);

	Move<VkBuffer>						vertexBuffer;
	MovePtr<Allocation>					vertexBufferAlloc;

	const Unique<VkShaderModule>		vertexModule			(createShaderModule (vk, device, context.getBinaryCollection().get("vert"), 0u));
	const Unique<VkShaderModule>		fragmentModule			(createShaderModule (vk, device, context.getBinaryCollection().get("frag"), 0u));

	// Create image where we will record the writes. For single-sampled cases this is a 4x1 image
	// and for multi-sampled cases this is a 4x<num_samples> image.
	const deUint8						numSamples				= multisample ? 4 : 1;
	const deUint8						imageWidth				= 4;
	const deUint8						imageHeight				= numSamples;
	const deUint8						imageDepth				= 1;
	const deUint8						imageLayers				= 1;
	const IVec3							imageDim				= IVec3(imageWidth, imageHeight, imageDepth);
	const VkImageUsageFlags				imageUsage				= VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
	const Move<VkImage>					image					= makeImage(vk, device, VkImageViewCreateFlags(0), VK_IMAGE_TYPE_2D, COLOR_FORMAT, imageDim, imageLayers, imageUsage, false);
	const VkImageSubresourceRange		imageSubresourceRange	= makeColorSubresourceRange(0u, imageLayers);
	const MovePtr<Allocation>			imageAlloc				= bindImage(vk, device, allocator, *image, MemoryRequirement::Any);
	const Move<VkImageView>				imageView				= makeImageView(vk, device, *image, VK_IMAGE_VIEW_TYPE_2D, COLOR_FORMAT, imageSubresourceRange);

	// Create a buffer where we will copy the image for verification
	const VkDeviceSize					colorBufferSize		= imageWidth * imageHeight * imageDepth * numSamples * tcu::getPixelSize(mapVkFormat(COLOR_FORMAT));
	const Unique<VkBuffer>				colorBuffer			(makeBuffer(vk, device, colorBufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT));
	const UniquePtr<Allocation>			colorBufferAlloc	(bindBuffer(vk, device, allocator, *colorBuffer, MemoryRequirement::HostVisible));

	// Create pipeline descriptor set for the image
	const Move<VkDescriptorSetLayout>	descriptorSetLayout		= DescriptorSetLayoutBuilder()
		.addSingleBinding(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_FRAGMENT_BIT)
		.build(vk, device);

	const Move<VkDescriptorPool>		descriptorPool			= DescriptorPoolBuilder()
		.addType(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1)
		.build(vk, device, VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT, 1);

	const Move<VkDescriptorSet>			descriptorSet			= makeDescriptorSet(vk, device, *descriptorPool, *descriptorSetLayout);
	const VkDescriptorImageInfo			descriptorImageInfo		= makeDescriptorImageInfo(DE_NULL, *imageView, VK_IMAGE_LAYOUT_GENERAL);
	DescriptorSetUpdateBuilder()
		.writeSingle(*descriptorSet, DescriptorSetUpdateBuilder::Location::binding(0u), VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, &descriptorImageInfo)
		.update(vk, device);

	const Unique<VkPipelineLayout>		pipelineLayout			(makePipelineLayout (vk, device, *descriptorSetLayout));
	vector<SharedPtrVkPipeline>			pipeline;
	const Unique<VkRenderPass>			renderPass				(makeRenderPassNoAtt (vk, device));
	Move<VkFramebuffer>					framebuffer;

	const Unique<VkCommandPool>			cmdPool					(createCommandPool (vk, device, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, queueFamilyIndex));
	const Unique<VkCommandBuffer>		cmdBuffer				(makeCommandBuffer (vk, device, *cmdPool));

	// create vertexBuffer
	{
		const vector<tcu::Vec4>	vertices			= genPointVertices();
		const VkDeviceSize		vertexBufferSize	= sizeInBytes(vertices);

		vertexBuffer		= makeBuffer(vk, device, vertexBufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
		vertexBufferAlloc	= bindBuffer(vk, device, allocator, *vertexBuffer, MemoryRequirement::HostVisible);
		deMemcpy(vertexBufferAlloc->getHostPtr(), &vertices[0], static_cast<std::size_t>(vertexBufferSize));
		flushMappedMemoryRange(vk, device, vertexBufferAlloc->getMemory(), vertexBufferAlloc->getOffset(), vertexBufferSize);
	}

	// Create render pass and pipeline
	pipeline.push_back(makeSharedPtr(makeGraphicsPipeline(vk, device, *pipelineLayout, *renderPass, *vertexModule, *fragmentModule,
		renderSize, VK_PRIMITIVE_TOPOLOGY_POINT_LIST, 0, false, multisample)));
	framebuffer = makeFramebuffer(vk, device, *renderPass, 0, DE_NULL, renderSize.x(), renderSize.y());

	// Record command buffer
	beginCommandBuffer(vk, *cmdBuffer);
	{
		// shader image layout transition undefined -> general
		{
			const VkImageMemoryBarrier setImageLayoutBarrier = makeImageMemoryBarrier(
				0u, VK_ACCESS_SHADER_WRITE_BIT,
				VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL,
				*image, imageSubresourceRange);

			vk.cmdPipelineBarrier(*cmdBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
				0, DE_NULL, 0, DE_NULL, 1, &setImageLayoutBarrier);
		}

		// Render pass
		{
			const VkRect2D renderArea =
			{
				makeOffset2D(0, 0),
				makeExtent2D(renderSize.x(), renderSize.y()),
			};
			const VkRenderPassBeginInfo renderPassBeginInfo =
			{
				VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,	// VkStructureType         sType;
				DE_NULL,									// const void*             pNext;
				*renderPass,								// VkRenderPass            renderPass;
				*framebuffer,								// VkFramebuffer           framebuffer;
				renderArea,									// VkRect2D                renderArea;
				0,											// uint32_t                clearValueCount;
				DE_NULL,									// const VkClearValue*     pClearValues;
			};
			const VkDeviceSize vertexBufferOffset = 0ull;

			vk.cmdBeginRenderPass(*cmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

			vk.cmdBindPipeline(*cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, **pipeline[0]);
			vk.cmdBindVertexBuffers(*cmdBuffer, 0u, 1u, &vertexBuffer.get(), &vertexBufferOffset);
			vk.cmdBindDescriptorSets(*cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout.get(), 0u, 1u, &descriptorSet.get(), 0u, DE_NULL);
			vk.cmdDraw(*cmdBuffer, 4u, 1u, 0u, 0u);

			vk.cmdEndRenderPass(*cmdBuffer);
		}

		// copy image to host visible colorBuffer
		{
			const VkImageMemoryBarrier	imageBarriers[]		=
			{
				{
					VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,		// VkStructureType			sType;
					DE_NULL,									// const void*				pNext;
					VK_ACCESS_SHADER_WRITE_BIT,					// VkAccessFlags			srcAccessMask;
					VK_ACCESS_TRANSFER_READ_BIT,				// VkAccessFlags			dstAccessMask;
					VK_IMAGE_LAYOUT_GENERAL,					// VkImageLayout			oldLayout;
					VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,		// VkImageLayout			newLayout;
					VK_QUEUE_FAMILY_IGNORED,					// deUint32					srcQueueFamilyIndex;
					VK_QUEUE_FAMILY_IGNORED,					// deUint32					destQueueFamilyIndex;
					*image,										// VkImage					image;
					makeColorSubresourceRange(0, 1)				// VkImageSubresourceRange	subresourceRange;
				}
			};

			vk.cmdPipelineBarrier(*cmdBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0u,
				0u, DE_NULL, 0u, DE_NULL, 1u, imageBarriers);

			const VkBufferImageCopy		region				=
			{
				0ull,																// VkDeviceSize                bufferOffset;
				0u,																	// uint32_t                    bufferRowLength;
				0u,																	// uint32_t                    bufferImageHeight;
				makeImageSubresourceLayers(VK_IMAGE_ASPECT_COLOR_BIT, 0u, 0u, 1),	// VkImageSubresourceLayers    imageSubresource;
				makeOffset3D(0, 0, 0),												// VkOffset3D                  imageOffset;
				makeExtent3D(IVec3(imageWidth, imageHeight, imageDepth)),			// VkExtent3D                  imageExtent;
			};

			vk.cmdCopyImageToBuffer(*cmdBuffer, *image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, *colorBuffer, 1u, &region);

			const VkBufferMemoryBarrier	bufferBarriers[]	=
			{
				{
					VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,	// VkStructureType    sType;
					DE_NULL,									// const void*        pNext;
					VK_ACCESS_TRANSFER_WRITE_BIT,				// VkAccessFlags      srcAccessMask;
					VK_ACCESS_HOST_READ_BIT,					// VkAccessFlags      dstAccessMask;
					VK_QUEUE_FAMILY_IGNORED,					// uint32_t           srcQueueFamilyIndex;
					VK_QUEUE_FAMILY_IGNORED,					// uint32_t           dstQueueFamilyIndex;
					*colorBuffer,								// VkBuffer           buffer;
					0ull,										// VkDeviceSize       offset;
					VK_WHOLE_SIZE,								// VkDeviceSize       size;
				},
			};

			vk.cmdPipelineBarrier(*cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_HOST_BIT, 0u,
				0u, DE_NULL, DE_LENGTH_OF_ARRAY(bufferBarriers), bufferBarriers, 0u, DE_NULL);
		}
	} // beginCommandBuffer

	VK_CHECK(vk.endCommandBuffer(*cmdBuffer));
	submitCommandsAndWait(vk, device, queue, *cmdBuffer);

	// Verify results
	{
		invalidateMappedMemoryRange(vk, device, colorBufferAlloc->getMemory(), colorBufferAlloc->getOffset(), VK_WHOLE_SIZE);
		const tcu::TextureFormat			format			= mapVkFormat(COLOR_FORMAT);
		tcu::TextureLevel					textureLevel	(format, imageWidth, imageHeight, imageDepth);
		const tcu::PixelBufferAccess		expectedImage	= getExpectedDataNoAtt(textureLevel);
		const tcu::ConstPixelBufferAccess	resultImage		(format, imageWidth, imageHeight, imageDepth, colorBufferAlloc->getHostPtr());

		if (!tcu::intThresholdCompare(context.getTestContext().getLog(), "Image Comparison", "", expectedImage, resultImage, tcu::UVec4(1), tcu::COMPARE_LOG_RESULT))
			return tcu::TestStatus::fail("Fail");
	}

	return tcu::TestStatus::pass("Pass");
}

std::string getShortImageViewTypeName (const VkImageViewType imageViewType)
{
	std::string	s	(getImageViewTypeName(imageViewType));
	return de::toLower(s.substr(19));
}

std::string getSizeString (const CaseDef& caseDef)
{
	std::ostringstream	str;

										str << caseDef.renderSize.x();
	if (caseDef.renderSize.y() > 1)		str << "x" << caseDef.renderSize.y();
	if (caseDef.renderSize.z() > 1)		str << "x" << caseDef.renderSize.z();

										str << "_" << caseDef.attachmentSize.x();

	if (caseDef.attachmentSize.y() > 1)	str << "x" << caseDef.attachmentSize.y();
	if (caseDef.attachmentSize.z() > 1)	str << "x" << caseDef.attachmentSize.z();
	if (caseDef.numLayers > 1)			str << "_" << caseDef.numLayers;

	return str.str();
}

std::string getTestCaseString (const CaseDef& caseDef)
{
	std::ostringstream str;

	str << getShortImageViewTypeName (caseDef.imageType).c_str();
	str << "_";
	str << getSizeString(caseDef);

	if (caseDef.multisample)
		str << "_ms";

	return str.str();
}

void addAttachmentTestCasesWithFunctions (tcu::TestCaseGroup* group)
{

	// Add test cases for attachment strictly sizes larger than the framebuffer
	const CaseDef	caseDef[]	=
	{
		// Single-sample test cases
		{ VK_IMAGE_VIEW_TYPE_1D,			IVec3(32, 1, 1),	IVec3(64, 1, 1),	1,		false },
		{ VK_IMAGE_VIEW_TYPE_1D,			IVec3(32, 1, 1),	IVec3(48, 1, 1),	1,		false },
		{ VK_IMAGE_VIEW_TYPE_1D,			IVec3(32, 1, 1),	IVec3(39, 1, 1),	1,		false },
		{ VK_IMAGE_VIEW_TYPE_1D,			IVec3(19, 1, 1),	IVec3(32, 1, 1),	1,		false },

		{ VK_IMAGE_VIEW_TYPE_1D_ARRAY,		IVec3(32, 1, 1),	IVec3(64, 1, 1),	4,		false },
		{ VK_IMAGE_VIEW_TYPE_1D_ARRAY,		IVec3(32, 1, 1),	IVec3(48, 1, 1),	4,		false },
		{ VK_IMAGE_VIEW_TYPE_1D_ARRAY,		IVec3(32, 1, 1),	IVec3(39, 1, 1),	4,		false },
		{ VK_IMAGE_VIEW_TYPE_1D_ARRAY,		IVec3(19, 1, 1),	IVec3(32, 1, 1),	4,		false },

		{ VK_IMAGE_VIEW_TYPE_2D,			IVec3(32, 32, 1),	IVec3(64, 64, 1),	1,		false },
		{ VK_IMAGE_VIEW_TYPE_2D,			IVec3(32, 32, 1),	IVec3(48, 48, 1),	1,		false },
		{ VK_IMAGE_VIEW_TYPE_2D,			IVec3(32, 32, 1),	IVec3(39, 41, 1),	1,		false },
		{ VK_IMAGE_VIEW_TYPE_2D,			IVec3(19, 27, 1),	IVec3(32, 32, 1),	1,		false },

		{ VK_IMAGE_VIEW_TYPE_2D_ARRAY,		IVec3(32, 32, 1),	IVec3(64, 64, 1),	4,		false },
		{ VK_IMAGE_VIEW_TYPE_2D_ARRAY,		IVec3(32, 32, 1),	IVec3(48, 48, 1),	4,		false },
		{ VK_IMAGE_VIEW_TYPE_2D_ARRAY,		IVec3(32, 32, 1),	IVec3(39, 41, 1),	4,		false },
		{ VK_IMAGE_VIEW_TYPE_2D_ARRAY,		IVec3(19, 27, 1),	IVec3(32, 32, 1),	4,		false },

		{ VK_IMAGE_VIEW_TYPE_CUBE,			IVec3(32, 32, 1),	IVec3(64, 64, 1),	6,		false },
		{ VK_IMAGE_VIEW_TYPE_CUBE,			IVec3(32, 32, 1),	IVec3(48, 48, 1),	6,		false },
		{ VK_IMAGE_VIEW_TYPE_CUBE,			IVec3(32, 32, 1),	IVec3(39, 41, 1),	6,		false },
		{ VK_IMAGE_VIEW_TYPE_CUBE,			IVec3(19, 27, 1),	IVec3(32, 32, 1),	6,		false },

		{ VK_IMAGE_VIEW_TYPE_CUBE_ARRAY,	IVec3(32, 32, 1),	IVec3(64, 64, 1),	6*2,	false },
		{ VK_IMAGE_VIEW_TYPE_CUBE_ARRAY,	IVec3(32, 32, 1),	IVec3(48, 48, 1),	6*2,	false },
		{ VK_IMAGE_VIEW_TYPE_CUBE_ARRAY,	IVec3(32, 32, 1),	IVec3(39, 41, 1),	6*2,	false },
		{ VK_IMAGE_VIEW_TYPE_CUBE_ARRAY,	IVec3(19, 27, 1),	IVec3(32, 32, 1),	6*2,	false },

		// Multi-sample test cases
		{ VK_IMAGE_VIEW_TYPE_2D,			IVec3(32, 32, 1),	IVec3(64, 64, 1),	1,		true },
		{ VK_IMAGE_VIEW_TYPE_2D,			IVec3(32, 32, 1),	IVec3(48, 48, 1),	1,		true },
		{ VK_IMAGE_VIEW_TYPE_2D,			IVec3(32, 32, 1),	IVec3(39, 41, 1),	1,		true },
		{ VK_IMAGE_VIEW_TYPE_2D,			IVec3(19, 27, 1),	IVec3(32, 32, 1),	1,		true },

		{ VK_IMAGE_VIEW_TYPE_2D_ARRAY,		IVec3(32, 32, 1),	IVec3(64, 64, 1),	4,		true },
		{ VK_IMAGE_VIEW_TYPE_2D_ARRAY,		IVec3(32, 32, 1),	IVec3(48, 48, 1),	4,		true },
		{ VK_IMAGE_VIEW_TYPE_2D_ARRAY,		IVec3(32, 32, 1),	IVec3(39, 41, 1),	4,		true },
		{ VK_IMAGE_VIEW_TYPE_2D_ARRAY,		IVec3(19, 27, 1),	IVec3(32, 32, 1),	4,		true },
	};

	for (int sizeNdx = 0; sizeNdx < DE_LENGTH_OF_ARRAY(caseDef); ++sizeNdx)
		addFunctionCaseWithPrograms(group, getTestCaseString(caseDef[sizeNdx]).c_str(), "", initColorPrograms, test, caseDef[sizeNdx]);

	// Add tests for the case where there are no color attachments but the
	// fragment shader writes to an image via imageStore().
	addFunctionCaseWithPrograms(group, "no_attachments", "", initImagePrograms, testNoAtt, false);
	addFunctionCaseWithPrograms(group, "no_attachments_ms", "", initImagePrograms, testNoAtt, true);
}

} // anonymous ns

tcu::TestCaseGroup* createFramebufferAttachmentTests (tcu::TestContext& testCtx)
{
	return createTestGroup(testCtx, "framebuffer_attachment", "Framebuffer attachment tests", addAttachmentTestCasesWithFunctions);
}

} // pipeline
} // vkt
