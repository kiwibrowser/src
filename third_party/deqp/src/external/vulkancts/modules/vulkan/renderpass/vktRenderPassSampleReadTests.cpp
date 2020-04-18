/*-------------------------------------------------------------------------
 * Vulkan Conformance Tests
 * ------------------------
 *
 * Copyright (c) 2017 Google Inc.
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
 * \brief Tests reading of samples from a previous subpass.
 *//*--------------------------------------------------------------------*/

#include "vktRenderPassSampleReadTests.hpp"

#include "vktTestCaseUtil.hpp"
#include "vktTestGroupUtil.hpp"

#include "vkDefs.hpp"
#include "vkImageUtil.hpp"
#include "vkMemUtil.hpp"
#include "vkPrograms.hpp"
#include "vkQueryUtil.hpp"
#include "vkRef.hpp"
#include "vkRefUtil.hpp"
#include "vkTypeUtil.hpp"

#include "tcuImageCompare.hpp"
#include "tcuResultCollector.hpp"

#include "deUniquePtr.hpp"

using namespace vk;

using tcu::UVec4;
using tcu::Vec4;

using tcu::ConstPixelBufferAccess;
using tcu::PixelBufferAccess;

using tcu::TestLog;

using std::string;
using std::vector;

namespace vkt
{
namespace
{

de::MovePtr<Allocation> createBufferMemory (const DeviceInterface&	vk,
											VkDevice				device,
											Allocator&				allocator,
											VkBuffer				buffer)
{
	de::MovePtr<Allocation> allocation (allocator.allocate(getBufferMemoryRequirements(vk, device, buffer), MemoryRequirement::HostVisible));
	VK_CHECK(vk.bindBufferMemory(device, buffer, allocation->getMemory(), allocation->getOffset()));
	return allocation;
}

de::MovePtr<Allocation> createImageMemory (const DeviceInterface&	vk,
										   VkDevice					device,
										   Allocator&				allocator,
										   VkImage					image)
{
	de::MovePtr<Allocation> allocation (allocator.allocate(getImageMemoryRequirements(vk, device, image), MemoryRequirement::Any));
	VK_CHECK(vk.bindImageMemory(device, image, allocation->getMemory(), allocation->getOffset()));
	return allocation;
}

Move<VkImage> createImage (const DeviceInterface&	vk,
						   VkDevice					device,
						   VkImageCreateFlags		flags,
						   VkImageType				imageType,
						   VkFormat					format,
						   VkExtent3D				extent,
						   deUint32					mipLevels,
						   deUint32					arrayLayers,
						   VkSampleCountFlagBits	samples,
						   VkImageTiling			tiling,
						   VkImageUsageFlags		usage,
						   VkSharingMode			sharingMode,
						   deUint32					queueFamilyCount,
						   const deUint32*			pQueueFamilyIndices,
						   VkImageLayout			initialLayout)
{
	const VkImageCreateInfo createInfo =
	{
		VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		DE_NULL,
		flags,
		imageType,
		format,
		extent,
		mipLevels,
		arrayLayers,
		samples,
		tiling,
		usage,
		sharingMode,
		queueFamilyCount,
		pQueueFamilyIndices,
		initialLayout
	};
	return createImage(vk, device, &createInfo);
}

Move<VkImageView> createImageView (const DeviceInterface&	vk,
								   VkDevice					device,
								   VkImageViewCreateFlags	flags,
								   VkImage					image,
								   VkImageViewType			viewType,
								   VkFormat					format,
								   VkComponentMapping		components,
								   VkImageSubresourceRange	subresourceRange)
{
	const VkImageViewCreateInfo pCreateInfo =
	{
		VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		DE_NULL,
		flags,
		image,
		viewType,
		format,
		components,
		subresourceRange,
	};
	return createImageView(vk, device, &pCreateInfo);
}

Move<VkImage> createImage (const InstanceInterface&	vki,
						   VkPhysicalDevice			physicalDevice,
						   const DeviceInterface&	vkd,
						   VkDevice					device,
						   VkFormat					vkFormat,
						   VkSampleCountFlagBits	sampleCountBit,
						   VkImageUsageFlags		usage,
						   deUint32					width,
						   deUint32					height)
{
	try
	{
		const VkImageType				imageType				(VK_IMAGE_TYPE_2D);
		const VkImageTiling				imageTiling				(VK_IMAGE_TILING_OPTIMAL);
		const VkImageFormatProperties	imageFormatProperties	(getPhysicalDeviceImageFormatProperties(vki, physicalDevice, vkFormat, imageType, imageTiling, usage, 0u));
		const VkExtent3D				imageExtent				=
		{
			width,
			height,
			1u
		};

		if (imageFormatProperties.maxExtent.width < imageExtent.width
			|| imageFormatProperties.maxExtent.height < imageExtent.height
			|| ((imageFormatProperties.sampleCounts & sampleCountBit) == 0))
		{
			TCU_THROW(NotSupportedError, "Image type not supported");
		}

		return createImage(vkd, device, 0u, imageType, vkFormat, imageExtent, 1u, 1u, sampleCountBit, imageTiling, usage, VK_SHARING_MODE_EXCLUSIVE, 0u, DE_NULL, VK_IMAGE_LAYOUT_UNDEFINED);
	}
	catch (const vk::Error& error)
	{
		if (error.getError() == VK_ERROR_FORMAT_NOT_SUPPORTED)
			TCU_THROW(NotSupportedError, "Image format not supported");

		throw;
	}
}

Move<VkImageView> createImageView (const DeviceInterface&	vkd,
								   VkDevice					device,
								   VkImage					image,
								   VkFormat					format,
								   VkImageAspectFlags		aspect)
{
	const VkImageSubresourceRange	range =
	{
		aspect,
		0u,
		1u,
		0u,
		1u
	};

	return createImageView(vkd, device, 0u, image, VK_IMAGE_VIEW_TYPE_2D, format, makeComponentMappingRGBA(), range);
}

VkDeviceSize getPixelSize (VkFormat vkFormat)
{
	const tcu::TextureFormat	format	(mapVkFormat(vkFormat));

	return format.getPixelSize();
}

Move<VkBuffer> createBuffer (const DeviceInterface&		vkd,
							 VkDevice					device,
							 VkFormat					format,
							 deUint32					width,
							 deUint32					height)
{
	const VkBufferUsageFlags	bufferUsage			(VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
	const VkDeviceSize			pixelSize			(getPixelSize(format));
	const VkBufferCreateInfo	createInfo			=
	{
		VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		DE_NULL,
		0u,

		width * height * pixelSize,
		bufferUsage,

		VK_SHARING_MODE_EXCLUSIVE,
		0u,
		DE_NULL
	};
	return createBuffer(vkd, device, &createInfo);
}

VkSampleCountFlagBits sampleCountBitFromSampleCount (deUint32 count)
{
	switch (count)
	{
		case 1:  return VK_SAMPLE_COUNT_1_BIT;
		case 2:  return VK_SAMPLE_COUNT_2_BIT;
		case 4:  return VK_SAMPLE_COUNT_4_BIT;
		case 8:  return VK_SAMPLE_COUNT_8_BIT;
		case 16: return VK_SAMPLE_COUNT_16_BIT;
		case 32: return VK_SAMPLE_COUNT_32_BIT;
		case 64: return VK_SAMPLE_COUNT_64_BIT;

		default:
			DE_FATAL("Invalid sample count");
			return (VkSampleCountFlagBits)(0x1u << count);
	}
}

Move<VkRenderPass> createRenderPass (const DeviceInterface&	vkd,
									 VkDevice				device,
									 VkFormat				srcFormat,
									 VkFormat				dstFormat,
									 deUint32				sampleCount)
{
	const VkSampleCountFlagBits			samples						(sampleCountBitFromSampleCount(sampleCount));
	const VkAttachmentReference			srcAttachmentRef			=
	{
		0u,
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
	};
	const VkAttachmentReference			srcAttachmentInputRef		=
	{
		0u,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
	};
	const VkAttachmentReference			dstAttachmentRef			=
	{
		1u,
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
	};
	const VkAttachmentReference			dstResolveAttachmentRef		=
	{
		2u,
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
	};
	const VkSubpassDependency			dependency					=
	{
		0u,
		1u,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,

		VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
		VK_ACCESS_INPUT_ATTACHMENT_READ_BIT,

		VK_DEPENDENCY_BY_REGION_BIT
	};
	const VkAttachmentDescription		srcAttachment				=
	{
		0u,

		srcFormat,
		samples,

		VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		VK_ATTACHMENT_STORE_OP_DONT_CARE,

		VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		VK_ATTACHMENT_STORE_OP_DONT_CARE,

		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_GENERAL
	};
	const VkAttachmentDescription		dstMultisampleAttachment	=
	{
		0u,

		dstFormat,
		samples,

		VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		VK_ATTACHMENT_STORE_OP_DONT_CARE,

		VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		VK_ATTACHMENT_STORE_OP_DONT_CARE,

		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
	};
	const VkAttachmentDescription		dstResolveAttachment		=
	{
		0u,

		dstFormat,
		VK_SAMPLE_COUNT_1_BIT,

		VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		VK_ATTACHMENT_STORE_OP_STORE,

		VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		VK_ATTACHMENT_STORE_OP_STORE,

		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
	};
	const VkAttachmentDescription		attachments[]				=
	{
		srcAttachment,
		dstMultisampleAttachment,
		dstResolveAttachment
	};
	const VkSubpassDescription			subpasses[]					=
	{
		{
			(VkSubpassDescriptionFlags)0,
			VK_PIPELINE_BIND_POINT_GRAPHICS,

			0u,
			DE_NULL,

			1u,
			&srcAttachmentRef,
			DE_NULL,

			DE_NULL,
			0u,
			DE_NULL
		},
		{
			(VkSubpassDescriptionFlags)0,
			VK_PIPELINE_BIND_POINT_GRAPHICS,

			1u,
			&srcAttachmentInputRef,

			1u,
			&dstAttachmentRef,
			&dstResolveAttachmentRef,

			DE_NULL,
			0u,
			DE_NULL
		}
	};
	const VkRenderPassCreateInfo	createInfo						=
	{
		VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
		DE_NULL,
		(VkRenderPassCreateFlags)0u,

		3u,
		attachments,

		2u,
		subpasses,

		1u,
		&dependency
	};

	return createRenderPass(vkd, device, &createInfo);
}

Move<VkFramebuffer> createFramebuffer (const DeviceInterface&	vkd,
									   VkDevice					device,
									   VkRenderPass				renderPass,
									   VkImageView				srcImageView,
									   VkImageView				dstMultisampleImageView,
									   VkImageView				dstSinglesampleImageView,
									   deUint32					width,
									   deUint32					height)
{
	VkImageView attachments[] =
	{
		srcImageView,
		dstMultisampleImageView,
		dstSinglesampleImageView
	};

	const VkFramebufferCreateInfo	createInfo	=
	{
		VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
		DE_NULL,
		0u,

		renderPass,
		3u,
		attachments,

		width,
		height,
		1u
	};

	return createFramebuffer(vkd, device, &createInfo);
}

Move<VkPipelineLayout> createRenderPipelineLayout (const DeviceInterface&	vkd,
												   VkDevice					device)
{
	const VkPipelineLayoutCreateInfo	createInfo	=
	{
		VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		DE_NULL,
		(vk::VkPipelineLayoutCreateFlags)0,

		0u,
		DE_NULL,

		0u,
		DE_NULL
	};

	return createPipelineLayout(vkd, device, &createInfo);
}

Move<VkPipeline> createRenderPipeline (const DeviceInterface&							vkd,
									   VkDevice											device,
									   VkRenderPass										renderPass,
									   VkPipelineLayout									pipelineLayout,
									   const vk::ProgramCollection<vk::ProgramBinary>&	binaryCollection,
									   deUint32											width,
									   deUint32											height,
									   deUint32											sampleCount)
{
	const Unique<VkShaderModule>					vertexShaderModule				(createShaderModule(vkd, device, binaryCollection.get("quad-vert"), 0u));
	const Unique<VkShaderModule>					fragmentShaderModule			(createShaderModule(vkd, device, binaryCollection.get("quad-frag"), 0u));
	const VkSpecializationInfo						emptyShaderSpecializations		=
	{
		0u,
		DE_NULL,

		0u,
		DE_NULL
	};
	// Disable blending
	const VkPipelineColorBlendAttachmentState		attachmentBlendState			=
	{
		VK_FALSE,
		VK_BLEND_FACTOR_SRC_ALPHA,
		VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
		VK_BLEND_OP_ADD,
		VK_BLEND_FACTOR_ONE,
		VK_BLEND_FACTOR_ONE,
		VK_BLEND_OP_ADD,
		VK_COLOR_COMPONENT_R_BIT|VK_COLOR_COMPONENT_G_BIT|VK_COLOR_COMPONENT_B_BIT|VK_COLOR_COMPONENT_A_BIT
	};
	const VkPipelineShaderStageCreateInfo			shaderStages[2]					=
	{
		{
			VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			DE_NULL,
			(VkPipelineShaderStageCreateFlags)0u,
			VK_SHADER_STAGE_VERTEX_BIT,
			*vertexShaderModule,
			"main",
			&emptyShaderSpecializations
		},
		{
			VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			DE_NULL,
			(VkPipelineShaderStageCreateFlags)0u,
			VK_SHADER_STAGE_FRAGMENT_BIT,
			*fragmentShaderModule,
			"main",
			&emptyShaderSpecializations
		}
	};
	const VkPipelineVertexInputStateCreateInfo		vertexInputState				=
	{
		VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		DE_NULL,
		(VkPipelineVertexInputStateCreateFlags)0u,

		0u,
		DE_NULL,

		0u,
		DE_NULL
	};
	const VkPipelineInputAssemblyStateCreateInfo	inputAssemblyState				=
	{
		VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
		DE_NULL,

		(VkPipelineInputAssemblyStateCreateFlags)0u,
		VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
		VK_FALSE
	};
	const VkViewport								viewport						=
	{
		0.0f,  0.0f,
		(float)width, (float)height,

		0.0f, 1.0f
	};
	const VkRect2D									scissor							=
	{
		{ 0u, 0u },
		{ width, height }
	};
	const VkPipelineViewportStateCreateInfo			viewportState					=
	{
		VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
		DE_NULL,
		(VkPipelineViewportStateCreateFlags)0u,

		1u,
		&viewport,

		1u,
		&scissor
	};
	const VkPipelineRasterizationStateCreateInfo	rasterState						=
	{
		VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
		DE_NULL,
		(VkPipelineRasterizationStateCreateFlags)0u,
		VK_TRUE,
		VK_FALSE,
		VK_POLYGON_MODE_FILL,
		VK_CULL_MODE_NONE,
		VK_FRONT_FACE_COUNTER_CLOCKWISE,
		VK_FALSE,
		0.0f,
		0.0f,
		0.0f,
		1.0f
	};
	const VkPipelineMultisampleStateCreateInfo		multisampleState				=
	{
		VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
		DE_NULL,
		(VkPipelineMultisampleStateCreateFlags)0u,

		sampleCountBitFromSampleCount(sampleCount),
		VK_FALSE,
		0.0f,
		DE_NULL,
		VK_FALSE,
		VK_FALSE,
	};
	const VkPipelineColorBlendStateCreateInfo		blendState						=
	{
		VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
		DE_NULL,
		(VkPipelineColorBlendStateCreateFlags)0u,

		VK_FALSE,
		VK_LOGIC_OP_COPY,
		1u,
		&attachmentBlendState,
		{ 0.0f, 0.0f, 0.0f, 0.0f }
	};
	const VkGraphicsPipelineCreateInfo				createInfo						=
	{
		VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		DE_NULL,
		(VkPipelineCreateFlags)0u,

		2,
		shaderStages,

		&vertexInputState,
		&inputAssemblyState,
		DE_NULL,
		&viewportState,
		&rasterState,
		&multisampleState,
		DE_NULL,
		&blendState,
		(const VkPipelineDynamicStateCreateInfo*)DE_NULL,
		pipelineLayout,

		renderPass,
		0u,
		DE_NULL,
		0u
	};

	return createGraphicsPipeline(vkd, device, DE_NULL, &createInfo);
}

Move<VkDescriptorSetLayout> createSubpassDescriptorSetLayout (const DeviceInterface&	vkd,
															  VkDevice					device)
{
	const VkDescriptorSetLayoutBinding		bindings[]	=
	{
		{
			0u,
			VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,
			1u,
			VK_SHADER_STAGE_FRAGMENT_BIT,
			DE_NULL
		},
		{
			1u,
			VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,
			1u,
			VK_SHADER_STAGE_FRAGMENT_BIT,
			DE_NULL
		}
	};
	const VkDescriptorSetLayoutCreateInfo	createInfo	=
	{
		VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		DE_NULL,
		0u,

		1u,
		bindings
	};

	return createDescriptorSetLayout(vkd, device, &createInfo);
}

Move<VkPipelineLayout> createSubpassPipelineLayout (const DeviceInterface&	vkd,
												  VkDevice					device,
												  VkDescriptorSetLayout		descriptorSetLayout)
{
	const VkPipelineLayoutCreateInfo	createInfo	=
	{
		VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		DE_NULL,
		(vk::VkPipelineLayoutCreateFlags)0,

		1u,
		&descriptorSetLayout,

		0u,
		DE_NULL
	};

	return createPipelineLayout(vkd, device, &createInfo);
}

Move<VkPipeline> createSubpassPipeline (const DeviceInterface&							vkd,
									  VkDevice											device,
									  VkRenderPass										renderPass,
									  VkPipelineLayout									pipelineLayout,
									  const vk::ProgramCollection<vk::ProgramBinary>&	binaryCollection,
									  deUint32											width,
									  deUint32											height,
									  deUint32											sampleCount)
{
	const Unique<VkShaderModule>					vertexShaderModule			(createShaderModule(vkd, device, binaryCollection.get("quad-vert"), 0u));
	const Unique<VkShaderModule>					fragmentShaderModule		(createShaderModule(vkd, device, binaryCollection.get("quad-subpass-frag"), 0u));
	const VkSpecializationInfo						emptyShaderSpecializations	=
	{
		0u,
		DE_NULL,

		0u,
		DE_NULL
	};
	// Disable blending
	const VkPipelineColorBlendAttachmentState		attachmentBlendState		=
	{
		VK_FALSE,
		VK_BLEND_FACTOR_SRC_ALPHA,
		VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
		VK_BLEND_OP_ADD,
		VK_BLEND_FACTOR_ONE,
		VK_BLEND_FACTOR_ONE,
		VK_BLEND_OP_ADD,
		VK_COLOR_COMPONENT_R_BIT|VK_COLOR_COMPONENT_G_BIT|VK_COLOR_COMPONENT_B_BIT|VK_COLOR_COMPONENT_A_BIT
	};
	const VkPipelineShaderStageCreateInfo			shaderStages[2]				=
	{
		{
			VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			DE_NULL,
			(VkPipelineShaderStageCreateFlags)0u,
			VK_SHADER_STAGE_VERTEX_BIT,
			*vertexShaderModule,
			"main",
			&emptyShaderSpecializations
		},
		{
			VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			DE_NULL,
			(VkPipelineShaderStageCreateFlags)0u,
			VK_SHADER_STAGE_FRAGMENT_BIT,
			*fragmentShaderModule,
			"main",
			&emptyShaderSpecializations
		}
	};
	const VkPipelineVertexInputStateCreateInfo		vertexInputState			=
	{
		VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		DE_NULL,
		(VkPipelineVertexInputStateCreateFlags)0u,

		0u,
		DE_NULL,

		0u,
		DE_NULL
	};
	const VkPipelineInputAssemblyStateCreateInfo	inputAssemblyState			=
	{
		VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
		DE_NULL,

		(VkPipelineInputAssemblyStateCreateFlags)0u,
		VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
		VK_FALSE
	};
	const VkViewport								viewport					=
	{
		0.0f,  0.0f,
		(float)width, (float)height,

		0.0f, 1.0f
	};
	const VkRect2D									scissor						=
	{
		{ 0u, 0u },
		{ width, height }
	};
	const VkPipelineViewportStateCreateInfo			viewportState				=
	{
		VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
		DE_NULL,
		(VkPipelineViewportStateCreateFlags)0u,

		1u,
		&viewport,

		1u,
		&scissor
	};
	const VkPipelineRasterizationStateCreateInfo	rasterState					=
	{
		VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
		DE_NULL,
		(VkPipelineRasterizationStateCreateFlags)0u,
		VK_TRUE,
		VK_FALSE,
		VK_POLYGON_MODE_FILL,
		VK_CULL_MODE_NONE,
		VK_FRONT_FACE_COUNTER_CLOCKWISE,
		VK_FALSE,
		0.0f,
		0.0f,
		0.0f,
		1.0f
	};
	const VkPipelineMultisampleStateCreateInfo		multisampleState			=
	{
		VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
		DE_NULL,
		(VkPipelineMultisampleStateCreateFlags)0u,

		sampleCountBitFromSampleCount(sampleCount),
		VK_FALSE,
		0.0f,
		DE_NULL,
		VK_FALSE,
		VK_FALSE,
	};
	const VkPipelineColorBlendStateCreateInfo		blendState					=
	{
		VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
		DE_NULL,
		(VkPipelineColorBlendStateCreateFlags)0u,

		VK_FALSE,
		VK_LOGIC_OP_COPY,

		1u,
		&attachmentBlendState,

		{ 0.0f, 0.0f, 0.0f, 0.0f }
	};
	const VkGraphicsPipelineCreateInfo				createInfo					=
	{
		VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		DE_NULL,
		(VkPipelineCreateFlags)0u,

		2,
		shaderStages,

		&vertexInputState,
		&inputAssemblyState,
		DE_NULL,
		&viewportState,
		&rasterState,
		&multisampleState,
		DE_NULL,
		&blendState,
		(const VkPipelineDynamicStateCreateInfo*)DE_NULL,
		pipelineLayout,

		renderPass,
		1u,
		DE_NULL,
		0u
	};

	return createGraphicsPipeline(vkd, device, DE_NULL, &createInfo);
}

Move<VkDescriptorPool> createSubpassDescriptorPool (const DeviceInterface&	vkd,
												  VkDevice					device)
{
	const VkDescriptorPoolSize			size		=
	{
		VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 2u
	};
	const VkDescriptorPoolCreateInfo	createInfo	=
	{
		VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		DE_NULL,
		VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,

		2u,
		1u,
		&size
	};

	return createDescriptorPool(vkd, device, &createInfo);
}

Move<VkDescriptorSet> createSubpassDescriptorSet (const DeviceInterface&	vkd,
												  VkDevice					device,
												  VkDescriptorPool			pool,
												  VkDescriptorSetLayout		layout,
												  VkImageView				imageView)
{
	const VkDescriptorSetAllocateInfo	allocateInfo	=
	{
		VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		DE_NULL,

		pool,
		1u,
		&layout
	};
	Move<VkDescriptorSet> set (allocateDescriptorSet(vkd, device, &allocateInfo));

	{
		const VkDescriptorImageInfo	imageInfo	=
		{
			(VkSampler)0u,
			imageView,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		};
		const VkWriteDescriptorSet	write		=
		{
			VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			DE_NULL,

			*set,
			0u,
			0u,
			1u,
			VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,
			&imageInfo,
			DE_NULL,
			DE_NULL
		};

		vkd.updateDescriptorSets(device, 1u, &write, 0u, DE_NULL);
	}
	return set;
}

enum TestMode
{
	TESTMODE_ADD = 0,
	TESTMODE_SELECT,

	TESTMODE_LAST
};

struct TestConfig
{
	TestConfig (deUint32 sampleCount_, TestMode testMode_, deUint32 selectedSample_)
	: sampleCount		(sampleCount_)
	, testMode			(testMode_)
	, selectedSample	(selectedSample_)
	{
	}

	deUint32	sampleCount;
	TestMode	testMode;
	deUint32	selectedSample;
};

class SampleReadTestInstance : public TestInstance
{
public:
											SampleReadTestInstance	(Context& context, TestConfig config);
											~SampleReadTestInstance	(void);

											tcu::TestStatus	iterate	(void);

private:
	const deUint32							m_sampleCount;
	const deUint32							m_width;
	const deUint32							m_height;
	const TestMode							m_testMode;
	const deUint32							m_selectedSample;

	const Unique<VkImage>					m_srcImage;
	const de::UniquePtr<Allocation>			m_srcImageMemory;
	const Unique<VkImageView>				m_srcImageView;
	const Unique<VkImageView>				m_srcInputImageView;

	const Unique<VkImage>					m_dstMultisampleImage;
	const de::UniquePtr<Allocation>			m_dstMultisampleImageMemory;
	const Unique<VkImageView>				m_dstMultisampleImageView;

	const Unique<VkImage>					m_dstSinglesampleImage;
	const de::UniquePtr<Allocation>			m_dstSinglesampleImageMemory;
	const Unique<VkImageView>				m_dstSinglesampleImageView;

	const Unique<VkBuffer>					m_dstBuffer;
	const de::UniquePtr<Allocation>			m_dstBufferMemory;

	const Unique<VkRenderPass>				m_renderPass;
	const Unique<VkFramebuffer>				m_framebuffer;

	const Unique<VkPipelineLayout>			m_renderPipelineLayout;
	const Unique<VkPipeline>				m_renderPipeline;

	const Unique<VkDescriptorSetLayout>		m_subpassDescriptorSetLayout;
	const Unique<VkPipelineLayout>			m_subpassPipelineLayout;
	const Unique<VkPipeline>				m_subpassPipeline;
	const Unique<VkDescriptorPool>			m_subpassDescriptorPool;
	const Unique<VkDescriptorSet>			m_subpassDescriptorSet;

	const Unique<VkCommandPool>				m_commandPool;
	tcu::ResultCollector					m_resultCollector;
};

SampleReadTestInstance::SampleReadTestInstance (Context& context, TestConfig config)
	: TestInstance					(context)
	, m_sampleCount					(config.sampleCount)
	, m_width						(32u)
	, m_height						(32u)
	, m_testMode					(config.testMode)
	, m_selectedSample				(config.selectedSample)
	, m_srcImage					(createImage(context.getInstanceInterface(), context.getPhysicalDevice(), context.getDeviceInterface(), context.getDevice(), VK_FORMAT_R32_UINT, sampleCountBitFromSampleCount(m_sampleCount), VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT, m_width, m_height))
	, m_srcImageMemory				(createImageMemory(context.getDeviceInterface(), context.getDevice(), context.getDefaultAllocator(), *m_srcImage))
	, m_srcImageView				(createImageView(context.getDeviceInterface(), context.getDevice(), *m_srcImage, VK_FORMAT_R32_UINT, VK_IMAGE_ASPECT_COLOR_BIT))
	, m_srcInputImageView			(createImageView(context.getDeviceInterface(), context.getDevice(), *m_srcImage, VK_FORMAT_R32_UINT, VK_IMAGE_ASPECT_COLOR_BIT))
	, m_dstMultisampleImage			(createImage(context.getInstanceInterface(), context.getPhysicalDevice(), context.getDeviceInterface(), context.getDevice(), VK_FORMAT_R32_UINT, sampleCountBitFromSampleCount(m_sampleCount), VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, m_width, m_height))
	, m_dstMultisampleImageMemory	(createImageMemory(context.getDeviceInterface(), context.getDevice(), context.getDefaultAllocator(), *m_dstMultisampleImage))
	, m_dstMultisampleImageView		(createImageView(context.getDeviceInterface(), context.getDevice(), *m_dstMultisampleImage, VK_FORMAT_R32_UINT, VK_IMAGE_ASPECT_COLOR_BIT))
	, m_dstSinglesampleImage		(createImage(context.getInstanceInterface(), context.getPhysicalDevice(), context.getDeviceInterface(), context.getDevice(), VK_FORMAT_R32_UINT, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, m_width, m_height))
	, m_dstSinglesampleImageMemory	(createImageMemory(context.getDeviceInterface(), context.getDevice(), context.getDefaultAllocator(), *m_dstSinglesampleImage))
	, m_dstSinglesampleImageView	(createImageView(context.getDeviceInterface(), context.getDevice(), *m_dstSinglesampleImage, VK_FORMAT_R32_UINT, VK_IMAGE_ASPECT_COLOR_BIT))
	, m_dstBuffer					(createBuffer(context.getDeviceInterface(), context.getDevice(), VK_FORMAT_R32_UINT, m_width, m_height))
	, m_dstBufferMemory				(createBufferMemory(context.getDeviceInterface(), context.getDevice(), context.getDefaultAllocator(), *m_dstBuffer))
	, m_renderPass					(createRenderPass(context.getDeviceInterface(), context.getDevice(), VK_FORMAT_R32_UINT, VK_FORMAT_R32_UINT, m_sampleCount))
	, m_framebuffer					(createFramebuffer(context.getDeviceInterface(), context.getDevice(), *m_renderPass, *m_srcImageView, *m_dstMultisampleImageView, *m_dstSinglesampleImageView, m_width, m_height))
	, m_renderPipelineLayout		(createRenderPipelineLayout(context.getDeviceInterface(), context.getDevice()))
	, m_renderPipeline				(createRenderPipeline(context.getDeviceInterface(), context.getDevice(), *m_renderPass, *m_renderPipelineLayout, context.getBinaryCollection(), m_width, m_height, m_sampleCount))
	, m_subpassDescriptorSetLayout	(createSubpassDescriptorSetLayout(context.getDeviceInterface(), context.getDevice()))
	, m_subpassPipelineLayout		(createSubpassPipelineLayout(context.getDeviceInterface(), context.getDevice(), *m_subpassDescriptorSetLayout))
	, m_subpassPipeline				(createSubpassPipeline(context.getDeviceInterface(), context.getDevice(), *m_renderPass, *m_subpassPipelineLayout, context.getBinaryCollection(), m_width, m_height, m_sampleCount))
	, m_subpassDescriptorPool		(createSubpassDescriptorPool(context.getDeviceInterface(), context.getDevice()))
	, m_subpassDescriptorSet		(createSubpassDescriptorSet(context.getDeviceInterface(), context.getDevice(), *m_subpassDescriptorPool, *m_subpassDescriptorSetLayout, *m_srcInputImageView))
	, m_commandPool					(createCommandPool(context.getDeviceInterface(), context.getDevice(), VK_COMMAND_POOL_CREATE_TRANSIENT_BIT, context.getUniversalQueueFamilyIndex()))
{
}

SampleReadTestInstance::~SampleReadTestInstance (void)
{
}

tcu::TestStatus SampleReadTestInstance::iterate (void)
{
	const DeviceInterface&			vkd				(m_context.getDeviceInterface());
	const VkDevice					device			(m_context.getDevice());
	const Unique<VkCommandBuffer>	commandBuffer	(allocateCommandBuffer(vkd, device, *m_commandPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY));

	{
		const VkCommandBufferBeginInfo beginInfo =
		{
			VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			DE_NULL,

			VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
			DE_NULL
		};

		VK_CHECK(vkd.beginCommandBuffer(*commandBuffer, &beginInfo));
	}

	{
		const VkRenderPassBeginInfo beginInfo =
		{
			VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
			DE_NULL,

			*m_renderPass,
			*m_framebuffer,

			{
				{ 0u, 0u },
				{ m_width, m_height }
			},

			0u,
			DE_NULL
		};
		vkd.cmdBeginRenderPass(*commandBuffer, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);
	}

	vkd.cmdBindPipeline(*commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *m_renderPipeline);

	vkd.cmdDraw(*commandBuffer, 6u, 1u, 0u, 0u);

	vkd.cmdNextSubpass(*commandBuffer, VK_SUBPASS_CONTENTS_INLINE);

	vkd.cmdBindPipeline(*commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *m_subpassPipeline);
	vkd.cmdBindDescriptorSets(*commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *m_subpassPipelineLayout, 0u, 1u,  &*m_subpassDescriptorSet, 0u, DE_NULL);
	vkd.cmdDraw(*commandBuffer, 6u, 1u, 0u, 0u);

	vkd.cmdEndRenderPass(*commandBuffer);

	// Memory barrier between rendering and copy
	{
		const VkImageMemoryBarrier barrier =
		{
			VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
			DE_NULL,

			VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			VK_ACCESS_TRANSFER_READ_BIT,

			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,

			VK_QUEUE_FAMILY_IGNORED,
			VK_QUEUE_FAMILY_IGNORED,

			*m_dstSinglesampleImage,
			{
				VK_IMAGE_ASPECT_COLOR_BIT,
				0u,
				1u,
				0u,
				1u
			}
		};

		vkd.cmdPipelineBarrier(*commandBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0u, 0u, DE_NULL, 0u, DE_NULL, 1u, &barrier);
	}

	// Copy image memory to buffer
	{
		const VkBufferImageCopy region =
		{
			0u,
			0u,
			0u,
			{
				VK_IMAGE_ASPECT_COLOR_BIT,
				0u,
				0u,
				1u,
			},
			{ 0u, 0u, 0u },
			{ m_width, m_height, 1u }
		};

		vkd.cmdCopyImageToBuffer(*commandBuffer, *m_dstSinglesampleImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, *m_dstBuffer, 1u, &region);
	}

	// Memory barrier between copy and host access
	{
		const VkBufferMemoryBarrier barrier =
		{
			VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
			DE_NULL,

			VK_ACCESS_TRANSFER_WRITE_BIT,
			VK_ACCESS_HOST_READ_BIT,

			VK_QUEUE_FAMILY_IGNORED,
			VK_QUEUE_FAMILY_IGNORED,

			*m_dstBuffer,
			0u,
			VK_WHOLE_SIZE
		};

		vkd.cmdPipelineBarrier(*commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_HOST_BIT, 0u, 0u, DE_NULL, 1u, &barrier, 0u, DE_NULL);
	}

	VK_CHECK(vkd.endCommandBuffer(*commandBuffer));

	{
		const VkSubmitInfo submitInfo =
		{
			VK_STRUCTURE_TYPE_SUBMIT_INFO,
			DE_NULL,

			0u,
			DE_NULL,
			DE_NULL,

			1u,
			&*commandBuffer,

			0u,
			DE_NULL
		};

		VK_CHECK(vkd.queueSubmit(m_context.getUniversalQueue(), 1u, &submitInfo, (VkFence)0u));

		VK_CHECK(vkd.queueWaitIdle(m_context.getUniversalQueue()));
	}

	{
		const tcu::TextureFormat			format		(mapVkFormat(VK_FORMAT_R32_UINT));
		const void* const					ptr			(m_dstBufferMemory->getHostPtr());
		const tcu::ConstPixelBufferAccess	access		(format, m_width, m_height, 1, ptr);
		tcu::TextureLevel					reference	(format, m_width, m_height);

		for (deUint32 y = 0; y < m_height; y++)
		for (deUint32 x = 0; x < m_width; x++)
		{
			deUint32		bits;

			if (m_testMode == TESTMODE_ADD)
				bits = m_sampleCount == 32 ? 0xffffffff : (1u << m_sampleCount) - 1;
			else
				bits = 1u << m_selectedSample;

			const UVec4		color	(bits, 0, 0, 0xffffffff);

			reference.getAccess().setPixel(color, x, y);
		}

		if (!tcu::intThresholdCompare(m_context.getTestContext().getLog(), "", "", reference.getAccess(), access, UVec4(0u), tcu::COMPARE_LOG_ON_ERROR))
			m_resultCollector.fail("Compare failed.");
	}

	return tcu::TestStatus(m_resultCollector.getResult(), m_resultCollector.getMessage());
}

struct Programs
{
	void init (vk::SourceCollections& dst, TestConfig config) const
	{
		std::ostringstream				fragmentShader;
		std::ostringstream				subpassShader;

		dst.glslSources.add("quad-vert") << glu::VertexSource(
			"#version 450\n"
			"out gl_PerVertex {\n"
			"\tvec4 gl_Position;\n"
			"};\n"
			"highp float;\n"
			"void main (void)\n"
			"{\n"
			"    gl_Position = vec4(((gl_VertexIndex + 2) / 3) % 2 == 0 ? -1.0 : 1.0,\n"
			"                       ((gl_VertexIndex + 1) / 3) % 2 == 0 ? -1.0 : 1.0, 0.0, 1.0);\n"
			"}\n");

		fragmentShader <<
			"#version 450\n"
			"layout(location = 0) out highp uvec4 o_color;\n"
			"void main (void)\n"
			"{\n"
			"    o_color = uvec4(1u << gl_SampleID, 0, 0, 0);\n"
			"}\n";

		dst.glslSources.add("quad-frag") << glu::FragmentSource(fragmentShader.str());

		subpassShader <<
			"#version 450\n"
			"layout(input_attachment_index = 0, set = 0, binding = 0) uniform highp usubpassInputMS i_color;\n"
			"layout(location = 0) out highp uvec4 o_color;\n"
			"void main (void)\n"
			"{\n"
			"    o_color = uvec4(0);\n";

		if (config.testMode == TESTMODE_ADD)
		{
			subpassShader <<
				"    for (int i = 0; i < " << config.sampleCount << "; i++)\n" <<
				"        o_color.r += subpassLoad(i_color, i).r;\n";
		}
		else
		{
			subpassShader <<
				"    o_color.r = subpassLoad(i_color, " << de::toString(config.selectedSample) << ").r;\n";
		}

		subpassShader << "}\n";

		dst.glslSources.add("quad-subpass-frag") << glu::FragmentSource(subpassShader.str());
	}
};

void initTests (tcu::TestCaseGroup* group)
{
	const deUint32			sampleCounts[]	= { 2u, 4u, 8u, 16u, 32u };
	tcu::TestContext&		testCtx			(group->getTestContext());

	for (deUint32 sampleCountNdx = 0; sampleCountNdx < DE_LENGTH_OF_ARRAY(sampleCounts); sampleCountNdx++)
	{
		const deUint32		sampleCount	(sampleCounts[sampleCountNdx]);
		{
			const std::string	testName	("numsamples_" + de::toString(sampleCount) + "_add");

			group->addChild(new InstanceFactory1<SampleReadTestInstance, TestConfig, Programs>(testCtx, tcu::NODETYPE_SELF_VALIDATE, testName.c_str(), testName.c_str(), TestConfig(sampleCount, TESTMODE_ADD, 0)));
		}

		for (deUint32 sample = 0; sample < sampleCount; sample++)
		{
			const std::string	testName	("numsamples_" + de::toString(sampleCount) + "_selected_sample_" + de::toString(sample));
			group->addChild(new InstanceFactory1<SampleReadTestInstance, TestConfig, Programs>(testCtx, tcu::NODETYPE_SELF_VALIDATE, testName.c_str(), testName.c_str(), TestConfig(sampleCount, TESTMODE_SELECT, sample)));
		}
	}
}

} // anonymous

tcu::TestCaseGroup* createRenderPassSampleReadTests (tcu::TestContext& testCtx)
{
	return createTestGroup(testCtx, "sampleread", "Sample reading tests", initTests);
}

} // vkt
