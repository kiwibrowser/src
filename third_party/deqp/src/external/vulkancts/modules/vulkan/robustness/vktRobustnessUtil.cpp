/*------------------------------------------------------------------------
 * Vulkan Conformance Tests
 * ------------------------
 *
 * Copyright (c) 2016 The Khronos Group Inc.
 * Copyright (c) 2016 Imagination Technologies Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *	  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 *//*!
 * \file
 * \brief Robustness Utilities
 *//*--------------------------------------------------------------------*/

#include "vktRobustnessUtil.hpp"
#include "vkDefs.hpp"
#include "vkImageUtil.hpp"
#include "vkPrograms.hpp"
#include "vkQueryUtil.hpp"
#include "vkRefUtil.hpp"
#include "deMath.h"
#include <iomanip>
#include <limits>
#include <sstream>

namespace vkt
{
namespace robustness
{

using namespace vk;

Move<VkDevice> createRobustBufferAccessDevice (Context& context)
{
	const float queuePriority = 1.0f;

	// Create a universal queue that supports graphics and compute
	const VkDeviceQueueCreateInfo queueParams =
	{
		VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,	// VkStructureType				sType;
		DE_NULL,									// const void*					pNext;
		0u,											// VkDeviceQueueCreateFlags		flags;
		context.getUniversalQueueFamilyIndex(),		// deUint32						queueFamilyIndex;
		1u,											// deUint32						queueCount;
		&queuePriority								// const float*					pQueuePriorities;
	};

	VkPhysicalDeviceFeatures enabledFeatures = context.getDeviceFeatures();
	enabledFeatures.robustBufferAccess = true;

	const VkDeviceCreateInfo deviceParams =
	{
		VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,	// VkStructureType					sType;
		DE_NULL,								// const void*						pNext;
		0u,										// VkDeviceCreateFlags				flags;
		1u,										// deUint32							queueCreateInfoCount;
		&queueParams,							// const VkDeviceQueueCreateInfo*	pQueueCreateInfos;
		0u,										// deUint32							enabledLayerCount;
		DE_NULL,								// const char* const*				ppEnabledLayerNames;
		0u,										// deUint32							enabledExtensionCount;
		DE_NULL,								// const char* const*				ppEnabledExtensionNames;
		&enabledFeatures						// const VkPhysicalDeviceFeatures*	pEnabledFeatures;
	};

	return createDevice(context.getInstanceInterface(), context.getPhysicalDevice(), &deviceParams);
}

bool areEqual (float a, float b)
{
	return deFloatAbs(a - b) <= 0.001f;
}

bool isValueZero (const void* valuePtr, size_t valueSizeInBytes)
{
	const deUint8* bytePtr = reinterpret_cast<const deUint8*>(valuePtr);

	for (size_t i = 0; i < valueSizeInBytes; i++)
	{
		if (bytePtr[i] != 0)
			return false;
	}

	return true;
}

bool isValueWithinBuffer (const void* buffer, VkDeviceSize bufferSize, const void* valuePtr, size_t valueSizeInBytes)
{
	const deUint8* byteBuffer = reinterpret_cast<const deUint8*>(buffer);

	if (bufferSize < ((VkDeviceSize)valueSizeInBytes))
		return false;

	for (VkDeviceSize i = 0; i <= (bufferSize - valueSizeInBytes); i++)
	{
		if (!deMemCmp(&byteBuffer[i], valuePtr, valueSizeInBytes))
			return true;
	}

	return false;
}

bool isValueWithinBufferOrZero (const void* buffer, VkDeviceSize bufferSize, const void* valuePtr, size_t valueSizeInBytes)
{
	return isValueWithinBuffer(buffer, bufferSize, valuePtr, valueSizeInBytes) || isValueZero(valuePtr, valueSizeInBytes);
}

bool verifyOutOfBoundsVec4 (const void* vecPtr, VkFormat bufferFormat)
{
	if (isUintFormat(bufferFormat))
	{
		const deUint32* data = (deUint32*)vecPtr;

		return data[0] == 0u
			&& data[1] == 0u
			&& data[2] == 0u
			&& (data[3] == 0u || data[3] == 1u || data[3] == std::numeric_limits<deUint32>::max());
	}
	else if (isIntFormat(bufferFormat))
	{
		const deInt32* data = (deInt32*)vecPtr;

		return data[0] == 0
			&& data[1] == 0
			&& data[2] == 0
			&& (data[3] == 0 || data[3] == 1 || data[3] == std::numeric_limits<deInt32>::max());
	}
	else if (isFloatFormat(bufferFormat))
	{
		const float* data = (float*)vecPtr;

		return areEqual(data[0], 0.0f)
			&& areEqual(data[1], 0.0f)
			&& areEqual(data[2], 0.0f)
			&& (areEqual(data[3], 0.0f) || areEqual(data[3], 1.0f));
	}
	else if (bufferFormat == VK_FORMAT_A2B10G10R10_UNORM_PACK32)
	{
		return *((deUint32*)vecPtr) == 0xc0000000u;
	}

	DE_ASSERT(false);
	return false;
}

void populateBufferWithTestValues (void* buffer, VkDeviceSize size, VkFormat format)
{
	// Assign a sequence of 32-bit values
	for (VkDeviceSize scalarNdx = 0; scalarNdx < size / 4; scalarNdx++)
	{
		const deUint32 valueIndex = (deUint32)(2 + scalarNdx); // Do not use 0 or 1

		if (isUintFormat(format))
		{
			reinterpret_cast<deUint32*>(buffer)[scalarNdx] = valueIndex;
		}
		else if (isIntFormat(format))
		{
			reinterpret_cast<deInt32*>(buffer)[scalarNdx] = -deInt32(valueIndex);
		}
		else if (isFloatFormat(format))
		{
			reinterpret_cast<float*>(buffer)[scalarNdx] = float(valueIndex);
		}
		else if (format == VK_FORMAT_A2B10G10R10_UNORM_PACK32)
		{
			const deUint32	r	= ((valueIndex + 0) & ((2u << 10) - 1u));
			const deUint32	g	= ((valueIndex + 1) & ((2u << 10) - 1u));
			const deUint32	b	= ((valueIndex + 2) & ((2u << 10) - 1u));
			const deUint32	a	= ((valueIndex + 0) & ((2u << 2) - 1u));

			reinterpret_cast<deUint32*>(buffer)[scalarNdx] = (a << 30) | (b << 20) | (g << 10) | r;
		}
		else
		{
			DE_ASSERT(false);
		}
	}
}

void logValue (std::ostringstream& logMsg, const void* valuePtr, VkFormat valueFormat, size_t valueSize)
{
	if (isUintFormat(valueFormat))
	{
		logMsg << *reinterpret_cast<const deUint32*>(valuePtr);
	}
	else if (isIntFormat(valueFormat))
	{
		logMsg << *reinterpret_cast<const deInt32*>(valuePtr);
	}
	else if (isFloatFormat(valueFormat))
	{
		logMsg << *reinterpret_cast<const float*>(valuePtr);
	}
	else
	{
		const deUint8*				bytePtr		= reinterpret_cast<const deUint8*>(valuePtr);
		const std::ios::fmtflags	streamFlags	= logMsg.flags();

		logMsg << std::hex;
		for (size_t i = 0; i < valueSize; i++)
		{
			logMsg << " " << (deUint32)bytePtr[i];
		}
		logMsg.flags(streamFlags);
	}
}

// TestEnvironment

TestEnvironment::TestEnvironment (Context&				context,
								  VkDevice				device,
								  VkDescriptorSetLayout	descriptorSetLayout,
								  VkDescriptorSet		descriptorSet)
	: m_context				(context)
	, m_device				(device)
	, m_descriptorSetLayout	(descriptorSetLayout)
	, m_descriptorSet		(descriptorSet)
{
	const DeviceInterface& vk = context.getDeviceInterface();

	// Create command pool
	{
		const VkCommandPoolCreateInfo commandPoolParams =
		{
			VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,		// VkStructureType			sType;
			DE_NULL,										// const void*				pNext;
			VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,			// VkCommandPoolCreateFlags	flags;
			context.getUniversalQueueFamilyIndex()			// deUint32					queueFamilyIndex;
		};

		m_commandPool = createCommandPool(vk, m_device, &commandPoolParams);
	}

	// Create command buffer
	{
		const VkCommandBufferAllocateInfo commandBufferAllocateInfo =
		{
			VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,	// VkStructureType			sType;
			DE_NULL,										// const void*				pNext;
			*m_commandPool,										// VkCommandPool			commandPool;
			VK_COMMAND_BUFFER_LEVEL_PRIMARY,				// VkCommandBufferLevel		level;
			1u,												// deUint32					bufferCount;
		};

		m_commandBuffer = allocateCommandBuffer(vk, m_device, &commandBufferAllocateInfo);
	}
}

VkCommandBuffer TestEnvironment::getCommandBuffer (void)
{
	return *m_commandBuffer;
}

// GraphicsEnvironment

GraphicsEnvironment::GraphicsEnvironment (Context&					context,
										  VkDevice					device,
										  VkDescriptorSetLayout		descriptorSetLayout,
										  VkDescriptorSet			descriptorSet,
										  const VertexBindings&		vertexBindings,
										  const VertexAttributes&	vertexAttributes,
										  const DrawConfig&			drawConfig)

	: TestEnvironment		(context, device, descriptorSetLayout, descriptorSet)
	, m_renderSize			(16, 16)
	, m_colorFormat			(VK_FORMAT_R8G8B8A8_UNORM)
{
	const DeviceInterface&		vk						= context.getDeviceInterface();
	const deUint32				queueFamilyIndex		= context.getUniversalQueueFamilyIndex();
	const VkComponentMapping	componentMappingRGBA	= { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
	SimpleAllocator				memAlloc				(vk, m_device, getPhysicalDeviceMemoryProperties(m_context.getInstanceInterface(), m_context.getPhysicalDevice()));

	// Create color image and view
	{
		const VkImageCreateInfo colorImageParams =
		{
			VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,										// VkStructureType			sType;
			DE_NULL,																	// const void*				pNext;
			0u,																			// VkImageCreateFlags		flags;
			VK_IMAGE_TYPE_2D,															// VkImageType				imageType;
			m_colorFormat,																// VkFormat					format;
			{ (deUint32)m_renderSize.x(), (deUint32)m_renderSize.y(), 1u },				// VkExtent3D				extent;
			1u,																			// deUint32					mipLevels;
			1u,																			// deUint32					arrayLayers;
			VK_SAMPLE_COUNT_1_BIT,														// VkSampleCountFlagBits	samples;
			VK_IMAGE_TILING_OPTIMAL,													// VkImageTiling			tiling;
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,		// VkImageUsageFlags		usage;
			VK_SHARING_MODE_EXCLUSIVE,													// VkSharingMode			sharingMode;
			1u,																			// deUint32					queueFamilyIndexCount;
			&queueFamilyIndex,															// const deUint32*			pQueueFamilyIndices;
			VK_IMAGE_LAYOUT_UNDEFINED													// VkImageLayout			initialLayout;
		};

		m_colorImage			= createImage(vk, m_device, &colorImageParams);
		m_colorImageAlloc		= memAlloc.allocate(getImageMemoryRequirements(vk, m_device, *m_colorImage), MemoryRequirement::Any);
		VK_CHECK(vk.bindImageMemory(m_device, *m_colorImage, m_colorImageAlloc->getMemory(), m_colorImageAlloc->getOffset()));

		const VkImageViewCreateInfo colorAttachmentViewParams =
		{
			VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,			// VkStructureType			sType;
			DE_NULL,											// const void*				pNext;
			0u,													// VkImageViewCreateFlags	flags;
			*m_colorImage,										// VkImage					image;
			VK_IMAGE_VIEW_TYPE_2D,								// VkImageViewType			viewType;
			m_colorFormat,										// VkFormat					format;
			componentMappingRGBA,								// VkComponentMapping		components;
			{ VK_IMAGE_ASPECT_COLOR_BIT, 0u, 1u, 0u, 1u }		// VkImageSubresourceRange	subresourceRange;
		};

		m_colorAttachmentView = createImageView(vk, m_device, &colorAttachmentViewParams);
	}

	// Create render pass
	{
		const VkAttachmentDescription colorAttachmentDescription =
		{
			0u,													// VkAttachmentDescriptionFlags		flags;
			m_colorFormat,										// VkFormat							format;
			VK_SAMPLE_COUNT_1_BIT,								// VkSampleCountFlagBits			samples;
			VK_ATTACHMENT_LOAD_OP_CLEAR,						// VkAttachmentLoadOp				loadOp;
			VK_ATTACHMENT_STORE_OP_STORE,						// VkAttachmentStoreOp				storeOp;
			VK_ATTACHMENT_LOAD_OP_DONT_CARE,					// VkAttachmentLoadOp				stencilLoadOp;
			VK_ATTACHMENT_STORE_OP_DONT_CARE,					// VkAttachmentStoreOp				stencilStoreOp;
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,			// VkImageLayout					initialLayout;
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL			// VkImageLayout					finalLayout;
		};

		const VkAttachmentReference colorAttachmentReference =
		{
			0u,													// deUint32			attachment;
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL			// VkImageLayout	layout;
		};

		const VkSubpassDescription subpassDescription =
		{
			0u,													// VkSubpassDescriptionFlags		flags;
			VK_PIPELINE_BIND_POINT_GRAPHICS,					// VkPipelineBindPoint				pipelineBindPoint;
			0u,													// deUint32							inputAttachmentCount;
			DE_NULL,											// const VkAttachmentReference*		pInputAttachments;
			1u,													// deUint32							colorAttachmentCount;
			&colorAttachmentReference,							// const VkAttachmentReference*		pColorAttachments;
			DE_NULL,											// const VkAttachmentReference*		pResolveAttachments;
			DE_NULL,											// const VkAttachmentReference*		pDepthStencilAttachment;
			0u,													// deUint32							preserveAttachmentCount;
			DE_NULL												// const VkAttachmentReference*		pPreserveAttachments;
		};

		const VkRenderPassCreateInfo renderPassParams =
		{
			VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,			// VkStructureType					sType;
			DE_NULL,											// const void*						pNext;
			0u,													// VkRenderPassCreateFlags			flags;
			1u,													// deUint32							attachmentCount;
			&colorAttachmentDescription,						// const VkAttachmentDescription*	pAttachments;
			1u,													// deUint32							subpassCount;
			&subpassDescription,								// const VkSubpassDescription*		pSubpasses;
			0u,													// deUint32							dependencyCount;
			DE_NULL												// const VkSubpassDependency*		pDependencies;
		};

		m_renderPass = createRenderPass(vk, m_device, &renderPassParams);
	}

	// Create framebuffer
	{
		const VkFramebufferCreateInfo framebufferParams =
		{
			VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,			// VkStructureType			sType;
			DE_NULL,											// const void*				pNext;
			0u,													// VkFramebufferCreateFlags	flags;
			*m_renderPass,										// VkRenderPass				renderPass;
			1u,													// deUint32					attachmentCount;
			&m_colorAttachmentView.get(),						// const VkImageView*		pAttachments;
			(deUint32)m_renderSize.x(),							// deUint32					width;
			(deUint32)m_renderSize.y(),							// deUint32					height;
			1u													// deUint32					layers;
		};

		m_framebuffer = createFramebuffer(vk, m_device, &framebufferParams);
	}

	// Create pipeline layout
	{
		const VkPipelineLayoutCreateInfo pipelineLayoutParams =
		{
			VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,		// VkStructureType				sType;
			DE_NULL,											// const void*					pNext;
			0u,													// VkPipelineLayoutCreateFlags	flags;
			1u,													// deUint32						setLayoutCount;
			&m_descriptorSetLayout,								// const VkDescriptorSetLayout*	pSetLayouts;
			0u,													// deUint32						pushConstantRangeCount;
			DE_NULL												// const VkPushConstantRange*	pPushConstantRanges;
		};

		m_pipelineLayout = createPipelineLayout(vk, m_device, &pipelineLayoutParams);
	}

	m_vertexShaderModule	= createShaderModule(vk, m_device, m_context.getBinaryCollection().get("vertex"), 0);
	m_fragmentShaderModule	= createShaderModule(vk, m_device, m_context.getBinaryCollection().get("fragment"), 0);

	// Create pipeline
	{
		const VkPipelineShaderStageCreateInfo renderShaderStages[2] =
		{
			{
				VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,		// VkStructureType						sType;
				DE_NULL,													// const void*							pNext;
				0u,															// VkPipelineShaderStageCreateFlags		flags;
				VK_SHADER_STAGE_VERTEX_BIT,									// VkShaderStageFlagBits				stage;
				*m_vertexShaderModule,										// VkShaderModule						module;
				"main",														// const char*							pName;
				DE_NULL														// const VkSpecializationInfo*			pSpecializationInfo;
			},
			{
				VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,		// VkStructureType						sType;
				DE_NULL,													// const void*							pNext;
				0u,															// VkPipelineShaderStageCreateFlags		flags;
				VK_SHADER_STAGE_FRAGMENT_BIT,								// VkShaderStageFlagBits				stage;
				*m_fragmentShaderModule,									// VkShaderModule						module;
				"main",														// const char*							pName;
				DE_NULL														// const VkSpecializationInfo*			pSpecializationInfo;
			}
		};

		const VkPipelineVertexInputStateCreateInfo vertexInputStateParams =
		{
			VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,		// VkStructureType							sType;
			DE_NULL,														// const void*								pNext;
			0u,																// VkPipelineVertexInputStateCreateFlags	flags;
			(deUint32)vertexBindings.size(),								// deUint32									vertexBindingDescriptionCount;
			vertexBindings.data(),											// const VkVertexInputBindingDescription*	pVertexBindingDescriptions;
			(deUint32)vertexAttributes.size(),								// deUint32									vertexAttributeDescriptionCount;
			vertexAttributes.data()											// const VkVertexInputAttributeDescription*	pVertexAttributeDescriptions;
		};

		const VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateParams =
		{
			VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,	// VkStructureType							sType;
			DE_NULL,														// const void*								pNext;
			0u,																// VkPipelineInputAssemblyStateCreateFlags	flags;
			VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,							// VkPrimitiveTopology						topology;
			false															// VkBool32									primitiveRestartEnable;
		};

		const VkViewport viewport =
		{
			0.0f,						// float	x;
			0.0f,						// float	y;
			(float)m_renderSize.x(),	// float	width;
			(float)m_renderSize.y(),	// float	height;
			0.0f,						// float	minDepth;
			1.0f						// float	maxDepth;
		};

		const VkRect2D scissor = { { 0, 0 }, { (deUint32)m_renderSize.x(), (deUint32)m_renderSize.y() } };

		const VkPipelineViewportStateCreateInfo viewportStateParams =
		{
			VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,			// VkStructureType						sType;
			DE_NULL,														// const void*							pNext;
			0u,																// VkPipelineViewportStateCreateFlags	flags;
			1u,																// deUint32								viewportCount;
			&viewport,														// const VkViewport*					pViewports;
			1u,																// deUint32								scissorCount;
			&scissor														// const VkRect2D*						pScissors;
		};

		const VkPipelineRasterizationStateCreateInfo rasterStateParams =
		{
			VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,		// VkStructureType							sType;
			DE_NULL,														// const void*								pNext;
			0u,																// VkPipelineRasterizationStateCreateFlags	flags;
			false,															// VkBool32									depthClampEnable;
			false,															// VkBool32									rasterizerDiscardEnable;
			VK_POLYGON_MODE_FILL,											// VkPolygonMode							polygonMode;
			VK_CULL_MODE_NONE,												// VkCullModeFlags							cullMode;
			VK_FRONT_FACE_COUNTER_CLOCKWISE,								// VkFrontFace								frontFace;
			false,															// VkBool32									depthBiasEnable;
			0.0f,															// float									depthBiasConstantFactor;
			0.0f,															// float									depthBiasClamp;
			0.0f,															// float									depthBiasSlopeFactor;
			1.0f															// float									lineWidth;
		};

		const VkPipelineColorBlendAttachmentState colorBlendAttachmentState =
		{
			false,														// VkBool32					blendEnable;
			VK_BLEND_FACTOR_ONE,										// VkBlendFactor			srcColorBlendFactor;
			VK_BLEND_FACTOR_ZERO,										// VkBlendFactor			dstColorBlendFactor;
			VK_BLEND_OP_ADD,											// VkBlendOp				colorBlendOp;
			VK_BLEND_FACTOR_ONE,										// VkBlendFactor			srcAlphaBlendFactor;
			VK_BLEND_FACTOR_ZERO,										// VkBlendFactor			dstAlphaBlendFactor;
			VK_BLEND_OP_ADD,											// VkBlendOp				alphaBlendOp;
			VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |		// VkColorComponentFlags	colorWriteMask;
				VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
		};

		const VkPipelineColorBlendStateCreateInfo colorBlendStateParams =
		{
			VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,	// VkStructureType								sType;
			DE_NULL,													// const void*									pNext;
			0u,															// VkPipelineColorBlendStateCreateFlags			flags;
			false,														// VkBool32										logicOpEnable;
			VK_LOGIC_OP_COPY,											// VkLogicOp									logicOp;
			1u,															// deUint32										attachmentCount;
			&colorBlendAttachmentState,									// const VkPipelineColorBlendAttachmentState*	pAttachments;
			{ 0.0f, 0.0f, 0.0f, 0.0f }									// float										blendConstants[4];
		};

		const VkPipelineMultisampleStateCreateInfo multisampleStateParams =
		{
			VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,	// VkStructureType							sType;
			DE_NULL,													// const void*								pNext;
			0u,															// VkPipelineMultisampleStateCreateFlags	flags;
			VK_SAMPLE_COUNT_1_BIT,										// VkSampleCountFlagBits					rasterizationSamples;
			false,														// VkBool32									sampleShadingEnable;
			0.0f,														// float									minSampleShading;
			DE_NULL,													// const VkSampleMask*						pSampleMask;
			false,														// VkBool32									alphaToCoverageEnable;
			false														// VkBool32									alphaToOneEnable;
		};

		VkPipelineDepthStencilStateCreateInfo depthStencilStateParams =
		{
			VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,	// VkStructureType							sType;
			DE_NULL,													// const void*								pNext;
			0u,															// VkPipelineDepthStencilStateCreateFlags	flags;
			false,														// VkBool32									depthTestEnable;
			false,														// VkBool32									depthWriteEnable;
			VK_COMPARE_OP_LESS,											// VkCompareOp								depthCompareOp;
			false,														// VkBool32									depthBoundsTestEnable;
			false,														// VkBool32									stencilTestEnable;
			{															// VkStencilOpState							front;
				VK_STENCIL_OP_ZERO,		// VkStencilOp	failOp;
				VK_STENCIL_OP_ZERO,		// VkStencilOp	passOp;
				VK_STENCIL_OP_ZERO,		// VkStencilOp	depthFailOp;
				VK_COMPARE_OP_NEVER,	// VkCompareOp	compareOp;
				0u,						// deUint32		compareMask;
				0u,						// deUint32		writeMask;
				0u						// deUint32		reference;
			},
			{															// VkStencilOpState	back;
				VK_STENCIL_OP_ZERO,		// VkStencilOp	failOp;
				VK_STENCIL_OP_ZERO,		// VkStencilOp	passOp;
				VK_STENCIL_OP_ZERO,		// VkStencilOp	depthFailOp;
				VK_COMPARE_OP_NEVER,	// VkCompareOp	compareOp;
				0u,						// deUint32		compareMask;
				0u,						// deUint32		writeMask;
				0u						// deUint32		reference;
			},
			-1.0f,														// float			minDepthBounds;
			+1.0f														// float			maxDepthBounds;
		};

		const VkGraphicsPipelineCreateInfo graphicsPipelineParams =
		{
			VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,	// VkStructureType									sType;
			DE_NULL,											// const void*										pNext;
			0u,													// VkPipelineCreateFlags							flags;
			2u,													// deUint32											stageCount;
			renderShaderStages,									// const VkPipelineShaderStageCreateInfo*			pStages;
			&vertexInputStateParams,							// const VkPipelineVertexInputStateCreateInfo*		pVertexInputState;
			&inputAssemblyStateParams,							// const VkPipelineInputAssemblyStateCreateInfo*	pInputAssemblyState;
			DE_NULL,											// const VkPipelineTessellationStateCreateInfo*		pTessellationState;
			&viewportStateParams,								// const VkPipelineViewportStateCreateInfo*			pViewportState;
			&rasterStateParams,									// const VkPipelineRasterizationStateCreateInfo*	pRasterizationState;
			&multisampleStateParams,							// const VkPipelineMultisampleStateCreateInfo*		pMultisampleState;
			&depthStencilStateParams,							// const VkPipelineDepthStencilStateCreateInfo*		pDepthStencilState;
			&colorBlendStateParams,								// const VkPipelineColorBlendStateCreateInfo*		pColorBlendState;
			DE_NULL,											// const VkPipelineDynamicStateCreateInfo*			pDynamicState;
			*m_pipelineLayout,									// VkPipelineLayout									layout;
			*m_renderPass,										// VkRenderPass										renderPass;
			0u,													// deUint32											subpass;
			0u,													// VkPipeline										basePipelineHandle;
			0u													// deInt32											basePipelineIndex;
		};

		m_graphicsPipeline	= createGraphicsPipeline(vk, m_device, DE_NULL, &graphicsPipelineParams);
	}

	// Record commands
	{
		const VkCommandBufferBeginInfo commandBufferBeginInfo =
		{
			VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,	// VkStructureType						sType;
			DE_NULL,										// const void*							pNext;
			0u,												// VkCommandBufferUsageFlags			flags;
			(const VkCommandBufferInheritanceInfo*)DE_NULL,	// const VkCommandBufferInheritanceInfo	*pInheritanceInfo;
		};

		VkClearValue attachmentClearValue;
		attachmentClearValue.color.float32[0] = 0.0f;
		attachmentClearValue.color.float32[1] = 0.0f;
		attachmentClearValue.color.float32[2] = 0.0f;
		attachmentClearValue.color.float32[3] = 0.0f;

		const VkRenderPassBeginInfo renderPassBeginInfo =
		{
			VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,				// VkStructureType		sType;
			DE_NULL,												// const void*			pNext;
			*m_renderPass,											// VkRenderPass			renderPass;
			*m_framebuffer,											// VkFramebuffer		framebuffer;
			{
				{ 0, 0 },
				{ (deUint32)m_renderSize.x(), (deUint32)m_renderSize.y() }
			},														// VkRect2D				renderArea;
			1,														// deUint32				clearValueCount;
			&attachmentClearValue									// const VkClearValue*	pClearValues;
		};

		const VkImageMemoryBarrier imageLayoutBarrier =
		{
			VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,					// VkStructureType			sType;
			DE_NULL,												// const void*				pNext;
			(VkAccessFlags)0,										// VkAccessFlags			srcAccessMask;
			VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,					// VkAccessFlags			dstAccessMask;
			VK_IMAGE_LAYOUT_UNDEFINED,								// VkImageLayout			oldLayout;
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,				// VkImageLayout			newLayout;
			VK_QUEUE_FAMILY_IGNORED,								// uint32_t					srcQueueFamilyIndex;
			VK_QUEUE_FAMILY_IGNORED,								// uint32_t					dstQueueFamilyIndex;
			*m_colorImage,											// VkImage					image;
			{ VK_IMAGE_ASPECT_COLOR_BIT, 0u, 1u, 0u, 1u }			// VkImageSubresourceRange	subresourceRange;
		};

		VK_CHECK(vk.beginCommandBuffer(*m_commandBuffer, &commandBufferBeginInfo));
		{
			vk.cmdPipelineBarrier(*m_commandBuffer,
								  VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
								  VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
								  (VkDependencyFlags)0,
								  0u, DE_NULL,
								  0u, DE_NULL,
								  1u, &imageLayoutBarrier);

			vk.cmdBeginRenderPass(*m_commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
			{
				const std::vector<VkDeviceSize> vertexBufferOffsets(drawConfig.vertexBuffers.size(), 0ull);

				vk.cmdBindPipeline(*m_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *m_graphicsPipeline);
				vk.cmdBindDescriptorSets(*m_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *m_pipelineLayout, 0, 1, &m_descriptorSet, 0, DE_NULL);
				vk.cmdBindVertexBuffers(*m_commandBuffer, 0, (deUint32)drawConfig.vertexBuffers.size(), drawConfig.vertexBuffers.data(), vertexBufferOffsets.data());

				if (drawConfig.indexBuffer == DE_NULL || drawConfig.indexCount == 0)
				{
					vk.cmdDraw(*m_commandBuffer, drawConfig.vertexCount, drawConfig.instanceCount, 0, 0);
				}
				else
				{
					vk.cmdBindIndexBuffer(*m_commandBuffer, drawConfig.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
					vk.cmdDrawIndexed(*m_commandBuffer, drawConfig.indexCount, drawConfig.instanceCount, 0, 0, 0);
				}
			}
			vk.cmdEndRenderPass(*m_commandBuffer);
		}
		VK_CHECK(vk.endCommandBuffer(*m_commandBuffer));
	}
}

// ComputeEnvironment

ComputeEnvironment::ComputeEnvironment (Context&				context,
										VkDevice				device,
										VkDescriptorSetLayout	descriptorSetLayout,
										VkDescriptorSet			descriptorSet)

	: TestEnvironment	(context, device, descriptorSetLayout, descriptorSet)
{
	const DeviceInterface& vk = context.getDeviceInterface();

	// Create pipeline layout
	{
		const VkPipelineLayoutCreateInfo pipelineLayoutParams =
		{
			VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,			// VkStructureType					sType;
			DE_NULL,												// const void*						pNext;
			0u,														// VkPipelineLayoutCreateFlags		flags;
			1u,														// deUint32							setLayoutCount;
			&m_descriptorSetLayout,									// const VkDescriptorSetLayout*		pSetLayouts;
			0u,														// deUint32							pushConstantRangeCount;
			DE_NULL													// const VkPushConstantRange*		pPushConstantRanges;
		};

		m_pipelineLayout = createPipelineLayout(vk, m_device, &pipelineLayoutParams);
	}

	// Create compute pipeline
	{
		m_computeShaderModule = createShaderModule(vk, m_device, m_context.getBinaryCollection().get("compute"), 0);

		const VkPipelineShaderStageCreateInfo computeStageParams =
		{
			VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,	// VkStructureType						sType;
			DE_NULL,												// const void*							pNext;
			0u,														// VkPipelineShaderStageCreateFlags		flags;
			VK_SHADER_STAGE_COMPUTE_BIT,							// VkShaderStageFlagBits				stage;
			*m_computeShaderModule,									// VkShaderModule						module;
			"main",													// const char*							pName;
			DE_NULL,												// const VkSpecializationInfo*			pSpecializationInfo;
		};

		const VkComputePipelineCreateInfo computePipelineParams =
		{
			VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,			// VkStructureType						sType;
			DE_NULL,												// const void*							pNext;
			0u,														// VkPipelineCreateFlags				flags;
			computeStageParams,										// VkPipelineShaderStageCreateInfo		stage;
			*m_pipelineLayout,										// VkPipelineLayout						layout;
			DE_NULL,												// VkPipeline							basePipelineHandle;
			0u														// deInt32								basePipelineIndex;
		};

		m_computePipeline = createComputePipeline(vk, m_device, DE_NULL, &computePipelineParams);
	}

	// Record commands
	{
		const VkCommandBufferBeginInfo commandBufferBeginInfo =
		{
			VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,	// VkStructureType						sType;
			DE_NULL,										// const void*							pNext;
			0u,												// VkCommandBufferUsageFlags			flags;
			(const VkCommandBufferInheritanceInfo*)DE_NULL,	// const VkCommandBufferInheritanceInfo	*pInheritanceInfo;
		};

		VK_CHECK(vk.beginCommandBuffer(*m_commandBuffer, &commandBufferBeginInfo));
		vk.cmdBindPipeline(*m_commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, *m_computePipeline);
		vk.cmdBindDescriptorSets(*m_commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, *m_pipelineLayout, 0, 1, &m_descriptorSet, 0, DE_NULL);
		vk.cmdDispatch(*m_commandBuffer, 32, 32, 1);
		VK_CHECK(vk.endCommandBuffer(*m_commandBuffer));
	}
}

} // robustness
} // vkt
