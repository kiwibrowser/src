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
 * \brief Tests for render pass multisample resolve
 *//*--------------------------------------------------------------------*/

#include "vktRenderPassMultisampleResolveTests.hpp"

#include "vktTestCaseUtil.hpp"
#include "vktTestGroupUtil.hpp"

#include "vkDefs.hpp"
#include "vkDeviceUtil.hpp"
#include "vkImageUtil.hpp"
#include "vkMemUtil.hpp"
#include "vkPlatform.hpp"
#include "vkPrograms.hpp"
#include "vkQueryUtil.hpp"
#include "vkRef.hpp"
#include "vkRefUtil.hpp"
#include "vkTypeUtil.hpp"

#include "tcuFloat.hpp"
#include "tcuImageCompare.hpp"
#include "tcuFormatUtil.hpp"
#include "tcuMaybe.hpp"
#include "tcuResultCollector.hpp"
#include "tcuTestLog.hpp"
#include "tcuTextureUtil.hpp"
#include "tcuVectorUtil.hpp"

#include "deUniquePtr.hpp"
#include "deSharedPtr.hpp"

using namespace vk;

using tcu::BVec4;
using tcu::IVec2;
using tcu::IVec4;
using tcu::UVec2;
using tcu::UVec4;
using tcu::Vec2;
using tcu::Vec4;

using tcu::Maybe;
using tcu::just;
using tcu::nothing;

using tcu::ConstPixelBufferAccess;
using tcu::PixelBufferAccess;

using tcu::TestLog;

using std::pair;
using std::string;
using std::vector;

typedef de::SharedPtr<vk::Unique<VkImage> >		VkImageSp;
typedef de::SharedPtr<vk::Unique<VkImageView> >	VkImageViewSp;
typedef de::SharedPtr<vk::Unique<VkBuffer> >	VkBufferSp;
typedef de::SharedPtr<vk::Unique<VkPipeline> >	VkPipelineSp;

namespace vkt
{
namespace
{
enum
{
	MAX_COLOR_ATTACHMENT_COUNT = 4u
};

template<typename T>
de::SharedPtr<T> safeSharedPtr (T* ptr)
{
	try
	{
		return de::SharedPtr<T>(ptr);
	}
	catch (...)
	{
		delete ptr;
		throw;
	}
}

void bindBufferMemory (const DeviceInterface& vk, VkDevice device, VkBuffer buffer, VkDeviceMemory mem, VkDeviceSize memOffset)
{
	VK_CHECK(vk.bindBufferMemory(device, buffer, mem, memOffset));
}

void bindImageMemory (const DeviceInterface& vk, VkDevice device, VkImage image, VkDeviceMemory mem, VkDeviceSize memOffset)
{
	VK_CHECK(vk.bindImageMemory(device, image, mem, memOffset));
}

de::MovePtr<Allocation> createBufferMemory (const DeviceInterface&	vk,
											VkDevice				device,
											Allocator&				allocator,
											VkBuffer				buffer)
{
	de::MovePtr<Allocation> allocation (allocator.allocate(getBufferMemoryRequirements(vk, device, buffer), MemoryRequirement::HostVisible));
	bindBufferMemory(vk, device, buffer, allocation->getMemory(), allocation->getOffset());
	return allocation;
}

de::MovePtr<Allocation> createImageMemory (const DeviceInterface&	vk,
										   VkDevice					device,
										   Allocator&				allocator,
										   VkImage					image)
{
	de::MovePtr<Allocation> allocation (allocator.allocate(getImageMemoryRequirements(vk, device, image), MemoryRequirement::Any));
	bindImageMemory(vk, device, image, allocation->getMemory(), allocation->getOffset());
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
	const VkImageCreateInfo pCreateInfo =
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
	return createImage(vk, device, &pCreateInfo);
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
		const tcu::TextureFormat		format					(mapVkFormat(vkFormat));
		const VkImageType				imageType				(VK_IMAGE_TYPE_2D);
		const VkImageTiling				imageTiling				(VK_IMAGE_TILING_OPTIMAL);
		const VkFormatProperties		formatProperties		(getPhysicalDeviceFormatProperties(vki, physicalDevice, vkFormat));
		const VkImageFormatProperties	imageFormatProperties	(getPhysicalDeviceImageFormatProperties(vki, physicalDevice, vkFormat, imageType, imageTiling, usage, 0u));
		const VkExtent3D				imageExtent				=
		{
			width,
			height,
			1u
		};

		if ((tcu::hasDepthComponent(format.order) || tcu::hasStencilComponent(format.order))
			&& (formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) == 0)
			TCU_THROW(NotSupportedError, "Format can't be used as depth stencil attachment");

		if (!(tcu::hasDepthComponent(format.order) || tcu::hasStencilComponent(format.order))
			&& (formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT) == 0)
			TCU_THROW(NotSupportedError, "Format can't be used as color attachment");

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
			return (VkSampleCountFlagBits)0x0;
	}
}

std::vector<VkImageSp> createMultisampleImages (const InstanceInterface&	vki,
												VkPhysicalDevice			physicalDevice,
												const DeviceInterface&		vkd,
												VkDevice					device,
												VkFormat					format,
												deUint32					sampleCount,
												deUint32					width,
												deUint32					height)
{
	std::vector<VkImageSp> images (MAX_COLOR_ATTACHMENT_COUNT);

	for (size_t imageNdx = 0; imageNdx < images.size(); imageNdx++)
		images[imageNdx] = safeSharedPtr(new Unique<VkImage>(createImage(vki, physicalDevice, vkd, device, format, sampleCountBitFromSampleCount(sampleCount), VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, width, height)));

	return images;
}

std::vector<VkImageSp> createSingleSampleImages (const InstanceInterface&	vki,
												 VkPhysicalDevice			physicalDevice,
												 const DeviceInterface&		vkd,
												 VkDevice					device,
												 VkFormat					format,
												 deUint32					width,
												 deUint32					height)
{
	std::vector<VkImageSp> images (MAX_COLOR_ATTACHMENT_COUNT);

	for (size_t imageNdx = 0; imageNdx < images.size(); imageNdx++)
		images[imageNdx] = safeSharedPtr(new Unique<VkImage>(createImage(vki, physicalDevice, vkd, device, format, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, width, height)));

	return images;
}

std::vector<de::SharedPtr<Allocation> > createImageMemory (const DeviceInterface&		vkd,
														   VkDevice						device,
														   Allocator&					allocator,
														   const std::vector<VkImageSp>	images)
{
	std::vector<de::SharedPtr<Allocation> > memory (images.size());

	for (size_t memoryNdx = 0; memoryNdx < memory.size(); memoryNdx++)
		memory[memoryNdx] = safeSharedPtr(createImageMemory(vkd, device, allocator, **images[memoryNdx]).release());

	return memory;
}

std::vector<VkImageViewSp> createImageViews (const DeviceInterface&			vkd,
											 VkDevice						device,
											 const std::vector<VkImageSp>&	images,
											 VkFormat						format,
											 VkImageAspectFlagBits			aspect)
{
	std::vector<VkImageViewSp> views (images.size());

	for (size_t imageNdx = 0; imageNdx < images.size(); imageNdx++)
		views[imageNdx] = safeSharedPtr(new Unique<VkImageView>(createImageView(vkd, device, **images[imageNdx], format, aspect)));

	return views;
}

std::vector<VkBufferSp> createBuffers (const DeviceInterface&	vkd,
									   VkDevice					device,
									   VkFormat					format,
									   deUint32					width,
									   deUint32					height)
{
	std::vector<VkBufferSp> buffers (MAX_COLOR_ATTACHMENT_COUNT);

	for (size_t bufferNdx = 0; bufferNdx < buffers.size(); bufferNdx++)
		buffers[bufferNdx] = safeSharedPtr(new Unique<VkBuffer>(createBuffer(vkd, device, format, width, height)));

	return buffers;
}

std::vector<de::SharedPtr<Allocation> > createBufferMemory (const DeviceInterface&			vkd,
															VkDevice						device,
															Allocator&						allocator,
															const std::vector<VkBufferSp>	buffers)
{
	std::vector<de::SharedPtr<Allocation> > memory (buffers.size());

	for (size_t memoryNdx = 0; memoryNdx < memory.size(); memoryNdx++)
		memory[memoryNdx] = safeSharedPtr(createBufferMemory(vkd, device, allocator, **buffers[memoryNdx]).release());

	return memory;
}

Move<VkRenderPass> createRenderPass (const DeviceInterface&	vkd,
									 VkDevice				device,
									 VkFormat				format,
									 deUint32				sampleCount)
{
	const VkSampleCountFlagBits				samples						(sampleCountBitFromSampleCount(sampleCount));
	std::vector<VkAttachmentDescription>	attachments;
	std::vector<VkAttachmentReference>		colorAttachmentRefs;
	std::vector<VkAttachmentReference>		resolveAttachmentRefs;

	for (size_t attachmentNdx = 0; attachmentNdx < 4; attachmentNdx++)
	{
		{
			const VkAttachmentDescription multisampleAttachment =
			{
				0u,

				format,
				samples,

				VK_ATTACHMENT_LOAD_OP_DONT_CARE,
				VK_ATTACHMENT_STORE_OP_DONT_CARE,

				VK_ATTACHMENT_LOAD_OP_DONT_CARE,
				VK_ATTACHMENT_STORE_OP_DONT_CARE,

				VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
			};
			const VkAttachmentReference attachmentRef =
			{
				(deUint32)attachments.size(),
				VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
			};
			colorAttachmentRefs.push_back(attachmentRef);
			attachments.push_back(multisampleAttachment);
		}
		{
			const VkAttachmentDescription singlesampleAttachment =
			{
				0u,

				format,
				VK_SAMPLE_COUNT_1_BIT,

				VK_ATTACHMENT_LOAD_OP_DONT_CARE,
				VK_ATTACHMENT_STORE_OP_STORE,

				VK_ATTACHMENT_LOAD_OP_DONT_CARE,
				VK_ATTACHMENT_STORE_OP_DONT_CARE,

				VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
			};
			const VkAttachmentReference attachmentRef =
			{
				(deUint32)attachments.size(),
				VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
			};
			resolveAttachmentRefs.push_back(attachmentRef);
			attachments.push_back(singlesampleAttachment);
		}
	}

	DE_ASSERT(colorAttachmentRefs.size() == resolveAttachmentRefs.size());
	DE_ASSERT(attachments.size() == colorAttachmentRefs.size() + resolveAttachmentRefs.size());

	{
		const VkSubpassDescription	subpass =
		{
			(VkSubpassDescriptionFlags)0,
			VK_PIPELINE_BIND_POINT_GRAPHICS,

			0u,
			DE_NULL,

			(deUint32)colorAttachmentRefs.size(),
			&colorAttachmentRefs[0],
			&resolveAttachmentRefs[0],

			DE_NULL,
			0u,
			DE_NULL
		};
		const VkRenderPassCreateInfo	createInfo	=
		{
			VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
			DE_NULL,
			(VkRenderPassCreateFlags)0u,

			(deUint32)attachments.size(),
			&attachments[0],

			1u,
			&subpass,

			0u,
			DE_NULL
		};

		return createRenderPass(vkd, device, &createInfo);
	}
}

Move<VkFramebuffer> createFramebuffer (const DeviceInterface&			vkd,
									   VkDevice							device,
									   VkRenderPass						renderPass,
									   const std::vector<VkImageViewSp>&	multisampleImageViews,
									   const std::vector<VkImageViewSp>&	singlesampleImageViews,
									   deUint32							width,
									   deUint32							height)
{
	std::vector<VkImageView> attachments;

	attachments.reserve(multisampleImageViews.size() + singlesampleImageViews.size());

	DE_ASSERT(multisampleImageViews.size() == singlesampleImageViews.size());

	for (size_t ndx = 0; ndx < multisampleImageViews.size(); ndx++)
	{
		attachments.push_back(**multisampleImageViews[ndx]);
		attachments.push_back(**singlesampleImageViews[ndx]);
	}

	const VkFramebufferCreateInfo createInfo =
	{
		VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
		DE_NULL,
		0u,

		renderPass,
		(deUint32)attachments.size(),
		&attachments[0],

		width,
		height,
		1u
	};

	return createFramebuffer(vkd, device, &createInfo);
}

Move<VkPipelineLayout> createRenderPipelineLayout (const DeviceInterface&	vkd,
												   VkDevice					device)
{
	const VkPushConstantRange			pushConstant			=
	{
		VK_SHADER_STAGE_FRAGMENT_BIT,
		0u,
		4u
	};
	const VkPipelineLayoutCreateInfo	createInfo	=
	{
		VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		DE_NULL,
		(vk::VkPipelineLayoutCreateFlags)0,

		0u,
		DE_NULL,

		1u,
		&pushConstant
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
	const Unique<VkShaderModule>	vertexShaderModule			(createShaderModule(vkd, device, binaryCollection.get("quad-vert"), 0u));
	const Unique<VkShaderModule>	fragmentShaderModule		(createShaderModule(vkd, device, binaryCollection.get("quad-frag"), 0u));
	const VkSpecializationInfo		emptyShaderSpecializations	=
	{
		0u,
		DE_NULL,

		0u,
		DE_NULL
	};
	// Disable blending
	const VkPipelineColorBlendAttachmentState attachmentBlendState =
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
	const VkPipelineColorBlendAttachmentState attachmentBlendStates[] =
	{
		attachmentBlendState,
		attachmentBlendState,
		attachmentBlendState,
		attachmentBlendState,
	};
	const VkPipelineShaderStageCreateInfo shaderStages[2] =
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
	const VkPipelineVertexInputStateCreateInfo vertexInputState =
	{
		VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		DE_NULL,
		(VkPipelineVertexInputStateCreateFlags)0u,

		0u,
		DE_NULL,

		0u,
		DE_NULL
	};
	const VkPipelineInputAssemblyStateCreateInfo inputAssemblyState =
	{
		VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
		DE_NULL,

		(VkPipelineInputAssemblyStateCreateFlags)0u,
		VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
		VK_FALSE
	};
	const VkViewport viewport =
	{
		0.0f,  0.0f,
		(float)width, (float)height,

		0.0f, 1.0f
	};
	const VkRect2D scissor =
	{
		{ 0u, 0u },
		{ width, height }
	};
	const VkPipelineViewportStateCreateInfo viewportState =
	{
		VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
		DE_NULL,
		(VkPipelineViewportStateCreateFlags)0u,

		1u,
		&viewport,

		1u,
		&scissor
	};
	const VkPipelineRasterizationStateCreateInfo rasterState =
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
	const VkPipelineMultisampleStateCreateInfo multisampleState =
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
	const VkPipelineDepthStencilStateCreateInfo depthStencilState =
	{
		VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
		DE_NULL,
		(VkPipelineDepthStencilStateCreateFlags)0u,

		VK_FALSE,
		VK_TRUE,
		VK_COMPARE_OP_ALWAYS,
		VK_FALSE,
		VK_TRUE,
		{
			VK_STENCIL_OP_KEEP,
			VK_STENCIL_OP_INCREMENT_AND_WRAP,
			VK_STENCIL_OP_KEEP,
			VK_COMPARE_OP_ALWAYS,
			~0u,
			~0u,
			0xFFu / (sampleCount + 1)
		},
		{
			VK_STENCIL_OP_KEEP,
			VK_STENCIL_OP_INCREMENT_AND_WRAP,
			VK_STENCIL_OP_KEEP,
			VK_COMPARE_OP_ALWAYS,
			~0u,
			~0u,
			0xFFu / (sampleCount + 1)
		},

		0.0f,
		1.0f
	};
	const VkPipelineColorBlendStateCreateInfo blendState =
	{
		VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
		DE_NULL,
		(VkPipelineColorBlendStateCreateFlags)0u,

		VK_FALSE,
		VK_LOGIC_OP_COPY,
		DE_LENGTH_OF_ARRAY(attachmentBlendStates),
		attachmentBlendStates,
		{ 0.0f, 0.0f, 0.0f, 0.0f }
	};
	const VkGraphicsPipelineCreateInfo createInfo =
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
		&depthStencilState,
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

struct TestConfig
{
				TestConfig		(VkFormat	format_,
								 deUint32	sampleCount_)
		: format		(format_)
		, sampleCount	(sampleCount_)
	{
	}

	VkFormat	format;
	deUint32	sampleCount;
};

class MultisampleRenderPassTestInstance : public TestInstance
{
public:
													MultisampleRenderPassTestInstance	(Context& context, TestConfig config);
													~MultisampleRenderPassTestInstance	(void);

	tcu::TestStatus									iterate								(void);

private:
	void											submit								(void);
	void											verify								(void);

	const VkFormat									m_format;
	const deUint32									m_sampleCount;
	const deUint32									m_width;
	const deUint32									m_height;

	const std::vector<VkImageSp>					m_multisampleImages;
	const std::vector<de::SharedPtr<Allocation> >	m_multisampleImageMemory;
	const std::vector<VkImageViewSp>				m_multisampleImageViews;

	const std::vector<VkImageSp>					m_singlesampleImages;
	const std::vector<de::SharedPtr<Allocation> >	m_singlesampleImageMemory;
	const std::vector<VkImageViewSp>				m_singlesampleImageViews;

	const Unique<VkRenderPass>						m_renderPass;
	const Unique<VkFramebuffer>						m_framebuffer;

	const Unique<VkPipelineLayout>					m_renderPipelineLayout;
	const Unique<VkPipeline>						m_renderPipeline;

	const std::vector<VkBufferSp>					m_buffers;
	const std::vector<de::SharedPtr<Allocation> >	m_bufferMemory;

	const Unique<VkCommandPool>						m_commandPool;
	tcu::TextureLevel								m_sum;
	deUint32										m_sampleMask;
	tcu::ResultCollector							m_resultCollector;
};

MultisampleRenderPassTestInstance::MultisampleRenderPassTestInstance (Context& context, TestConfig config)
	: TestInstance				(context)
	, m_format					(config.format)
	, m_sampleCount				(config.sampleCount)
	, m_width					(32u)
	, m_height					(32u)

	, m_multisampleImages		(createMultisampleImages(context.getInstanceInterface(), context.getPhysicalDevice(), context.getDeviceInterface(), context.getDevice(), m_format, m_sampleCount, m_width, m_height))
	, m_multisampleImageMemory	(createImageMemory(context.getDeviceInterface(), context.getDevice(), context.getDefaultAllocator(), m_multisampleImages))
	, m_multisampleImageViews	(createImageViews(context.getDeviceInterface(), context.getDevice(), m_multisampleImages, m_format, VK_IMAGE_ASPECT_COLOR_BIT))

	, m_singlesampleImages		(createSingleSampleImages(context.getInstanceInterface(), context.getPhysicalDevice(), context.getDeviceInterface(), context.getDevice(), m_format, m_width, m_height))
	, m_singlesampleImageMemory	(createImageMemory(context.getDeviceInterface(), context.getDevice(), context.getDefaultAllocator(), m_singlesampleImages))
	, m_singlesampleImageViews	(createImageViews(context.getDeviceInterface(), context.getDevice(), m_singlesampleImages, m_format, VK_IMAGE_ASPECT_COLOR_BIT))

	, m_renderPass				(createRenderPass(context.getDeviceInterface(), context.getDevice(), m_format, m_sampleCount))
	, m_framebuffer				(createFramebuffer(context.getDeviceInterface(), context.getDevice(), *m_renderPass, m_multisampleImageViews, m_singlesampleImageViews, m_width, m_height))

	, m_renderPipelineLayout	(createRenderPipelineLayout(context.getDeviceInterface(), context.getDevice()))
	, m_renderPipeline			(createRenderPipeline(context.getDeviceInterface(), context.getDevice(), *m_renderPass, *m_renderPipelineLayout, context.getBinaryCollection(), m_width, m_height, m_sampleCount))

	, m_buffers					(createBuffers(context.getDeviceInterface(), context.getDevice(), m_format, m_width, m_height))
	, m_bufferMemory			(createBufferMemory(context.getDeviceInterface(), context.getDevice(), context.getDefaultAllocator(), m_buffers))

	, m_commandPool				(createCommandPool(context.getDeviceInterface(), context.getDevice(), VK_COMMAND_POOL_CREATE_TRANSIENT_BIT, context.getUniversalQueueFamilyIndex()))
	, m_sum						(tcu::TextureFormat(tcu::TextureFormat::RGBA, tcu::TextureFormat::FLOAT), m_width, m_height)
	, m_sampleMask				(0x0u)
{
	tcu::clear(m_sum.getAccess(), Vec4(0.0f, 0.0f, 0.0f, 0.0f));
}

MultisampleRenderPassTestInstance::~MultisampleRenderPassTestInstance (void)
{
}

void MultisampleRenderPassTestInstance::submit (void)
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

	// Memory barriers between previous copies and rendering
	{
		std::vector<VkImageMemoryBarrier> barriers;

		for (size_t dstNdx = 0; dstNdx < m_singlesampleImages.size(); dstNdx++)
		{
			const VkImageMemoryBarrier barrier =
			{
				VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
				DE_NULL,

				VK_ACCESS_TRANSFER_READ_BIT,
				VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,

				VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,

				VK_QUEUE_FAMILY_IGNORED,
				VK_QUEUE_FAMILY_IGNORED,

				**m_singlesampleImages[dstNdx],
				{
					VK_IMAGE_ASPECT_COLOR_BIT,
					0u,
					1u,
					0u,
					1u
				}
			};

			barriers.push_back(barrier);
		}

		vkd.cmdPipelineBarrier(*commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0u, 0u, DE_NULL, 0u, DE_NULL, (deUint32)barriers.size(), &barriers[0]);
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

	// Clear everything to black
	{
		const tcu::TextureFormat			format			(mapVkFormat(m_format));
		const tcu::TextureChannelClass		channelClass	(tcu::getTextureChannelClass(format.type));
		VkClearValue						value;

		switch (channelClass)
		{
			case tcu::TEXTURECHANNELCLASS_FLOATING_POINT:
				value = makeClearValueColorF32(-1.0f, -1.0f, -1.0f, -1.0f);
				break;

			case tcu::TEXTURECHANNELCLASS_UNSIGNED_FIXED_POINT:
				value = makeClearValueColorF32(0.0f, 0.0f, 0.0f, 0.0f);
				break;

			case tcu::TEXTURECHANNELCLASS_SIGNED_FIXED_POINT:
				value = makeClearValueColorF32(-1.0f, -1.0f, -1.0f, -1.0f);
				break;

			case tcu::TEXTURECHANNELCLASS_SIGNED_INTEGER:
				value = makeClearValueColorI32(-128, -128, -128, -128);
				break;

			case tcu::TEXTURECHANNELCLASS_UNSIGNED_INTEGER:
				value = makeClearValueColorU32(0u, 0u, 0u, 0u);
				break;

			default:
				DE_FATAL("Unknown channel class");
		}
		const VkClearAttachment				colors[]		=
		{
			{
				VK_IMAGE_ASPECT_COLOR_BIT,
				0u,
				value
			},
			{
				VK_IMAGE_ASPECT_COLOR_BIT,
				1u,
				value
			},
			{
				VK_IMAGE_ASPECT_COLOR_BIT,
				2u,
				value
			},
			{
				VK_IMAGE_ASPECT_COLOR_BIT,
				3u,
				value
			}
		};
		const VkClearRect rect =
		{
			{
				{ 0u, 0u },
				{ m_width, m_height }
			},
			0u,
			1u,
		};
		vkd.cmdClearAttachments(*commandBuffer, DE_LENGTH_OF_ARRAY(colors), colors, 1u, &rect);
	}

	// Render black samples
	{
		vkd.cmdBindPipeline(*commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *m_renderPipeline);
		vkd.cmdPushConstants(*commandBuffer, *m_renderPipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0u, sizeof(m_sampleMask), &m_sampleMask);
		vkd.cmdDraw(*commandBuffer, 6u, 1u, 0u, 0u);
	}

	vkd.cmdEndRenderPass(*commandBuffer);

	// Memory barriers between rendering and copies
	{
		std::vector<VkImageMemoryBarrier> barriers;

		for (size_t dstNdx = 0; dstNdx < m_singlesampleImages.size(); dstNdx++)
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

				**m_singlesampleImages[dstNdx],
				{
					VK_IMAGE_ASPECT_COLOR_BIT,
					0u,
					1u,
					0u,
					1u
				}
			};

			barriers.push_back(barrier);
		}

		vkd.cmdPipelineBarrier(*commandBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0u, 0u, DE_NULL, 0u, DE_NULL, (deUint32)barriers.size(), &barriers[0]);
	}

	// Copy image memory to buffers
	for (size_t dstNdx = 0; dstNdx < m_singlesampleImages.size(); dstNdx++)
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

		vkd.cmdCopyImageToBuffer(*commandBuffer, **m_singlesampleImages[dstNdx], VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, **m_buffers[dstNdx], 1u, &region);
	}

	// Memory barriers between copies and host access
	{
		std::vector<VkBufferMemoryBarrier> barriers;

		for (size_t dstNdx = 0; dstNdx < m_buffers.size(); dstNdx++)
		{
			const VkBufferMemoryBarrier barrier =
			{
				VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
				DE_NULL,

				VK_ACCESS_TRANSFER_WRITE_BIT,
				VK_ACCESS_HOST_READ_BIT,

				VK_QUEUE_FAMILY_IGNORED,
				VK_QUEUE_FAMILY_IGNORED,

				**m_buffers[dstNdx],
				0u,
				VK_WHOLE_SIZE
			};

			barriers.push_back(barrier);
		}

		vkd.cmdPipelineBarrier(*commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_HOST_BIT, 0u, 0u, DE_NULL, (deUint32)barriers.size(), &barriers[0], 0u, DE_NULL);
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
}

void MultisampleRenderPassTestInstance::verify (void)
{
	const Vec4							errorColor		(1.0f, 0.0f, 0.0f, 1.0f);
	const Vec4							okColor			(0.0f, 0.0f, 0.0f, 1.0f);
	const tcu::TextureFormat			format			(mapVkFormat(m_format));
	const tcu::TextureChannelClass		channelClass	(tcu::getTextureChannelClass(format.type));
	const void* const					ptrs[]			=
	{
		m_bufferMemory[0]->getHostPtr(),
		m_bufferMemory[1]->getHostPtr(),
		m_bufferMemory[2]->getHostPtr(),
		m_bufferMemory[3]->getHostPtr()
	};
	const tcu::ConstPixelBufferAccess	accesses[]		=
	{
		tcu::ConstPixelBufferAccess(format, m_width, m_height, 1, ptrs[0]),
		tcu::ConstPixelBufferAccess(format, m_width, m_height, 1, ptrs[1]),
		tcu::ConstPixelBufferAccess(format, m_width, m_height, 1, ptrs[2]),
		tcu::ConstPixelBufferAccess(format, m_width, m_height, 1, ptrs[3])
	};
	tcu::TextureLevel					errorMask		(tcu::TextureFormat(tcu::TextureFormat::RGB, tcu::TextureFormat::UNORM_INT8), m_width, m_height);
	tcu::TestLog&						log				(m_context.getTestContext().getLog());

	switch (channelClass)
	{
		case tcu::TEXTURECHANNELCLASS_FLOATING_POINT:
		case tcu::TEXTURECHANNELCLASS_UNSIGNED_FIXED_POINT:
		case tcu::TEXTURECHANNELCLASS_SIGNED_FIXED_POINT:
		{
			const int	componentCount	(tcu::getNumUsedChannels(format.order));
			bool		isOk			= true;
			float		clearValue;
			float		renderValue;

			switch (channelClass)
			{
				case tcu::TEXTURECHANNELCLASS_FLOATING_POINT:
				case tcu::TEXTURECHANNELCLASS_SIGNED_FIXED_POINT:
					clearValue  = -1.0f;
					renderValue = 1.0f;
					break;

				case tcu::TEXTURECHANNELCLASS_UNSIGNED_FIXED_POINT:
					clearValue  = 0.0f;
					renderValue = 1.0f;
					break;

				default:
					clearValue  = 0.0f;
					renderValue = 0.0f;
					DE_FATAL("Unknown channel class");
			}

			for (deUint32 y = 0; y < m_height; y++)
			for (deUint32 x = 0; x < m_width; x++)
			{
				// Color has to be black if no samples were covered, white if all samples were covered or same in every attachment
				const Vec4	firstColor	(accesses[0].getPixel(x, y));
				const Vec4	refColor	(m_sampleMask == 0x0u
										? Vec4(clearValue,
												componentCount > 1 ? clearValue : 0.0f,
												componentCount > 2 ? clearValue : 0.0f,
												componentCount > 3 ? clearValue : 1.0f)
										: m_sampleMask == ((0x1u << m_sampleCount) - 1u)
										? Vec4(renderValue,
												componentCount > 1 ? renderValue : 0.0f,
												componentCount > 2 ? renderValue : 0.0f,
												componentCount > 3 ? renderValue : 1.0f)
										: firstColor);

				errorMask.getAccess().setPixel(okColor, x, y);

				for (size_t attachmentNdx = 0; attachmentNdx < MAX_COLOR_ATTACHMENT_COUNT; attachmentNdx++)
				{
					const Vec4 color (accesses[attachmentNdx].getPixel(x, y));

					if (refColor != color)
					{
						isOk = false;
						errorMask.getAccess().setPixel(errorColor, x, y);
						break;
					}
				}

				{
					const Vec4 old = m_sum.getAccess().getPixel(x, y);

					m_sum.getAccess().setPixel(old + firstColor, x, y);
				}
			}

			if (!isOk)
			{
				const std::string			sectionName	("ResolveVerifyWithMask" + de::toString(m_sampleMask));
				const tcu::ScopedLogSection	section		(log, sectionName, sectionName);

				for (size_t attachmentNdx = 0; attachmentNdx < MAX_COLOR_ATTACHMENT_COUNT; attachmentNdx++)
				{
					const std::string	name	("Attachment" + de::toString(attachmentNdx));
					m_context.getTestContext().getLog() << tcu::LogImage(name.c_str(), name.c_str(), accesses[attachmentNdx]);
				}

				m_context.getTestContext().getLog() << tcu::LogImage("ErrorMask", "ErrorMask", errorMask.getAccess());

				if (m_sampleMask == 0x0u)
				{
					m_context.getTestContext().getLog() << tcu::TestLog::Message << "Empty sample mask didn't produce all " << clearValue << " pixels" << tcu::TestLog::EndMessage;
					m_resultCollector.fail("Empty sample mask didn't produce correct pixel values");
				}
				else if (m_sampleMask == ((0x1u << m_sampleCount) - 1u))
				{
					m_context.getTestContext().getLog() << tcu::TestLog::Message << "Full sample mask didn't produce all " << renderValue << " pixels" << tcu::TestLog::EndMessage;
					m_resultCollector.fail("Full sample mask didn't produce correct pixel values");
				}
				else
				{
					m_context.getTestContext().getLog() << tcu::TestLog::Message << "Resolve is inconsistent between attachments" << tcu::TestLog::EndMessage;
					m_resultCollector.fail("Resolve is inconsistent between attachments");
				}
			}
			break;
		}

		case tcu::TEXTURECHANNELCLASS_UNSIGNED_INTEGER:
		{
			const int		componentCount			(tcu::getNumUsedChannels(format.order));
			const UVec4		bitDepth				(tcu::getTextureFormatBitDepth(format).cast<deUint32>());
			const UVec4		renderValue				(tcu::select((UVec4(1u) << tcu::min(UVec4(8u), bitDepth)) - UVec4(1u),
																  UVec4(0u, 0u, 0u, 1u),
																  tcu::lessThan(IVec4(0, 1, 2, 3), IVec4(componentCount))));
			const UVec4		clearValue				(tcu::select(UVec4(0u),
																 UVec4(0u, 0u, 0u, 1u),
																 tcu::lessThan(IVec4(0, 1, 2, 3), IVec4(componentCount))));
			bool			unexpectedValues		= false;
			bool			inconsistentComponents	= false;
			bool			inconsistentAttachments	= false;

			for (deUint32 y = 0; y < m_height; y++)
			for (deUint32 x = 0; x < m_width; x++)
			{
				// Color has to be all zeros if no samples were covered, all 255 if all samples were covered or consistent across all attachments
				const UVec4 refColor	(m_sampleMask == 0x0u
										? clearValue
										: m_sampleMask == ((0x1u << m_sampleCount) - 1u)
										? renderValue
										: accesses[0].getPixelUint(x, y));

				errorMask.getAccess().setPixel(okColor, x, y);

				// If reference value was taken from first attachment, check that it is valid value i.e. clear or render value
				if (m_sampleMask != 0x0u && m_sampleMask != ((0x1u << m_sampleCount) - 1u))
				{
					// Each component must be resolved same way
					const BVec4		isRenderValue	(refColor == renderValue);
					const BVec4		isClearValue	(refColor == clearValue);

					unexpectedValues		= tcu::anyNotEqual(tcu::logicalOr(isRenderValue, isClearValue), BVec4(true));
					inconsistentComponents	= !(tcu::allEqual(isRenderValue, BVec4(true)) || tcu::allEqual(isClearValue, BVec4(true)));

					if (unexpectedValues || inconsistentComponents)
						errorMask.getAccess().setPixel(errorColor, x, y);
				}

				for (size_t attachmentNdx = 0; attachmentNdx < MAX_COLOR_ATTACHMENT_COUNT; attachmentNdx++)
				{
					const UVec4 color (accesses[attachmentNdx].getPixelUint(x, y));

					if (refColor != color)
					{
						inconsistentAttachments = true;
						errorMask.getAccess().setPixel(errorColor, x, y);
						break;
					}
				}
			}

			if (unexpectedValues || inconsistentComponents || inconsistentAttachments)
			{
				const std::string			sectionName	("ResolveVerifyWithMask" + de::toString(m_sampleMask));
				const tcu::ScopedLogSection	section		(log, sectionName, sectionName);

				for (size_t attachmentNdx = 0; attachmentNdx < MAX_COLOR_ATTACHMENT_COUNT; attachmentNdx++)
				{
					const std::string	name	("Attachment" + de::toString(attachmentNdx));
					m_context.getTestContext().getLog() << tcu::LogImage(name.c_str(), name.c_str(), accesses[attachmentNdx]);
				}

				m_context.getTestContext().getLog() << tcu::LogImage("ErrorMask", "ErrorMask", errorMask.getAccess());

				if (m_sampleMask == 0x0u)
				{
					m_context.getTestContext().getLog() << tcu::TestLog::Message << "Empty sample mask didn't produce all " << clearValue << " pixels" << tcu::TestLog::EndMessage;
					m_resultCollector.fail("Empty sample mask didn't produce correct pixels");
				}
				else if (m_sampleMask == ((0x1u << m_sampleCount) - 1u))
				{
					m_context.getTestContext().getLog() << tcu::TestLog::Message << "Full sample mask didn't produce all " << renderValue << " pixels" << tcu::TestLog::EndMessage;
					m_resultCollector.fail("Full sample mask didn't produce correct pixels");
				}
				else
				{
					if (unexpectedValues)
					{
						m_context.getTestContext().getLog() << tcu::TestLog::Message << "Resolve produced unexpected values i.e. not " << clearValue << " or " << renderValue << tcu::TestLog::EndMessage;
						m_resultCollector.fail("Resolve produced unexpected values");
					}

					if (inconsistentComponents)
					{
						m_context.getTestContext().getLog() << tcu::TestLog::Message << "Different components of attachment were resolved to different values." << tcu::TestLog::EndMessage;
						m_resultCollector.fail("Different components of attachment were resolved to different values.");
					}

					if (inconsistentAttachments)
					{
						m_context.getTestContext().getLog() << tcu::TestLog::Message << "Different attachments were resolved to different values." << tcu::TestLog::EndMessage;
						m_resultCollector.fail("Different attachments were resolved to different values.");
					}
				}
			}
			break;
		}

		case tcu::TEXTURECHANNELCLASS_SIGNED_INTEGER:
		{
			const int		componentCount			(tcu::getNumUsedChannels(format.order));
			const IVec4		bitDepth				(tcu::getTextureFormatBitDepth(format));
			const IVec4		renderValue				(tcu::select((IVec4(1) << (tcu::min(IVec4(8), bitDepth) - IVec4(1))) - IVec4(1),
																  IVec4(0, 0, 0, 1),
																  tcu::lessThan(IVec4(0, 1, 2, 3), IVec4(componentCount))));
			const IVec4		clearValue				(tcu::select(-(IVec4(1) << (tcu::min(IVec4(8), bitDepth) - IVec4(1))),
																 IVec4(0, 0, 0, 1),
																 tcu::lessThan(IVec4(0, 1, 2, 3), IVec4(componentCount))));
			bool			unexpectedValues		= false;
			bool			inconsistentComponents	= false;
			bool			inconsistentAttachments	= false;

			for (deUint32 y = 0; y < m_height; y++)
			for (deUint32 x = 0; x < m_width; x++)
			{
				// Color has to be all zeros if no samples were covered, all 255 if all samples were covered or consistent across all attachments
				const IVec4 refColor	(m_sampleMask == 0x0u
										? clearValue
										: m_sampleMask == ((0x1u << m_sampleCount) - 1u)
										? renderValue
										: accesses[0].getPixelInt(x, y));

				errorMask.getAccess().setPixel(okColor, x, y);

				// If reference value was taken from first attachment, check that it is valid value i.e. clear or render value
				if (m_sampleMask != 0x0u && m_sampleMask != ((0x1u << m_sampleCount) - 1u))
				{
					// Each component must be resolved same way
					const BVec4		isRenderValue	(refColor == renderValue);
					const BVec4		isClearValue	(refColor == clearValue);

					unexpectedValues		= tcu::anyNotEqual(tcu::logicalOr(isRenderValue, isClearValue), BVec4(true));
					inconsistentComponents	= !(tcu::allEqual(isRenderValue, BVec4(true)) || tcu::allEqual(isClearValue, BVec4(true)));

					if (unexpectedValues || inconsistentComponents)
						errorMask.getAccess().setPixel(errorColor, x, y);
				}
			}

			if (unexpectedValues || inconsistentComponents || inconsistentAttachments)
			{
				const std::string			sectionName	("ResolveVerifyWithMask" + de::toString(m_sampleMask));
				const tcu::ScopedLogSection	section		(log, sectionName, sectionName);

				for (size_t attachmentNdx = 0; attachmentNdx < MAX_COLOR_ATTACHMENT_COUNT; attachmentNdx++)
				{
					const std::string	name	("Attachment" + de::toString(attachmentNdx));
					m_context.getTestContext().getLog() << tcu::LogImage(name.c_str(), name.c_str(), accesses[attachmentNdx]);
				}

				m_context.getTestContext().getLog() << tcu::LogImage("ErrorMask", "ErrorMask", errorMask.getAccess());

				if (m_sampleMask == 0x0u)
				{
					m_context.getTestContext().getLog() << tcu::TestLog::Message << "Empty sample mask didn't produce all " << clearValue << " pixels" << tcu::TestLog::EndMessage;
					m_resultCollector.fail("Empty sample mask didn't produce correct pixels");
				}
				else if (m_sampleMask == ((0x1u << m_sampleCount) - 1u))
				{
					m_context.getTestContext().getLog() << tcu::TestLog::Message << "Full sample mask didn't produce all " << renderValue << " pixels" << tcu::TestLog::EndMessage;
					m_resultCollector.fail("Full sample mask didn't produce correct pixels");
				}
				else
				{
					if (unexpectedValues)
					{
						m_context.getTestContext().getLog() << tcu::TestLog::Message << "Resolve produced unexpected values i.e. not " << clearValue << " or " << renderValue << tcu::TestLog::EndMessage;
						m_resultCollector.fail("Resolve produced unexpected values");
					}

					if (inconsistentComponents)
					{
						m_context.getTestContext().getLog() << tcu::TestLog::Message << "Different components of attachment were resolved to different values." << tcu::TestLog::EndMessage;
						m_resultCollector.fail("Different components of attachment were resolved to different values.");
					}

					if (inconsistentAttachments)
					{
						m_context.getTestContext().getLog() << tcu::TestLog::Message << "Different attachments were resolved to different values." << tcu::TestLog::EndMessage;
						m_resultCollector.fail("Different attachments were resolved to different values.");
					}
				}
			}
			break;
		}

		default:
			DE_FATAL("Unknown channel class");
	}
}

tcu::TestStatus MultisampleRenderPassTestInstance::iterate (void)
{
	if (m_sampleMask == 0u)
	{
		const tcu::TextureFormat		format			(mapVkFormat(m_format));
		const tcu::TextureChannelClass	channelClass	(tcu::getTextureChannelClass(format.type));
		tcu::TestLog&					log				(m_context.getTestContext().getLog());

		switch (channelClass)
		{
			case tcu::TEXTURECHANNELCLASS_UNSIGNED_INTEGER:
				log << TestLog::Message << "Clearing target to zero and rendering 255 pixels with every possible sample mask" << TestLog::EndMessage;
				break;

			case tcu::TEXTURECHANNELCLASS_SIGNED_INTEGER:
				log << TestLog::Message << "Clearing target to -128 and rendering 127 pixels with every possible sample mask" << TestLog::EndMessage;
				break;

			case tcu::TEXTURECHANNELCLASS_UNSIGNED_FIXED_POINT:
			case tcu::TEXTURECHANNELCLASS_SIGNED_FIXED_POINT:
			case tcu::TEXTURECHANNELCLASS_FLOATING_POINT:
				log << TestLog::Message << "Clearing target to black and rendering white pixels with every possible sample mask" << TestLog::EndMessage;
				break;

			default:
				DE_FATAL("Unknown channel class");
		}
	}

	submit();
	verify();

	if (m_sampleMask == ((0x1u << m_sampleCount) - 1u))
	{
		const tcu::TextureFormat		format			(mapVkFormat(m_format));
		const tcu::TextureChannelClass	channelClass	(tcu::getTextureChannelClass(format.type));
		tcu::TestLog&					log				(m_context.getTestContext().getLog());

		if (channelClass == tcu::TEXTURECHANNELCLASS_UNSIGNED_FIXED_POINT
				|| channelClass == tcu::TEXTURECHANNELCLASS_SIGNED_FIXED_POINT
				|| channelClass == tcu::TEXTURECHANNELCLASS_FLOATING_POINT)
		{
			const float			threshold		= 0.05f;
			const int			componentCount	(tcu::getNumUsedChannels(format.order));
			const Vec4			errorColor		(1.0f, 0.0f, 0.0f, 1.0f);
			const Vec4			okColor			(0.0f, 0.0f, 0.0f, 1.0f);
			tcu::TextureLevel	errorMask		(tcu::TextureFormat(tcu::TextureFormat::RGB, tcu::TextureFormat::UNORM_INT8), m_width, m_height);
			bool				isOk			= true;
			Vec4				maxDiff			(0.0f);
			Vec4				expectedAverage;

			switch (channelClass)
			{
				case tcu::TEXTURECHANNELCLASS_UNSIGNED_FIXED_POINT:
				{
					expectedAverage = Vec4(0.5f, componentCount > 1 ? 0.5f : 0.0f, componentCount > 2 ? 0.5f : 0.0f, componentCount > 3 ? 0.5f : 1.0f);
					break;
				}

				case tcu::TEXTURECHANNELCLASS_SIGNED_FIXED_POINT:
				case tcu::TEXTURECHANNELCLASS_FLOATING_POINT:
				{
					expectedAverage = Vec4(0.0f, 0.0f, 0.0f, componentCount > 3 ? 0.0f : 1.0f);
					break;
				}

				default:
					DE_FATAL("Unknown channel class");
			}

			for (deUint32 y = 0; y < m_height; y++)
			for (deUint32 x = 0; x < m_width; x++)
			{
				const Vec4	sum		(m_sum.getAccess().getPixel(x, y));
				const Vec4	average	(sum / Vec4((float)(0x1u << m_sampleCount)));
				const Vec4	diff	(tcu::abs(average - expectedAverage));

				m_sum.getAccess().setPixel(average, x, y);
				errorMask.getAccess().setPixel(okColor, x, y);

				if (diff[0] > threshold
						|| diff[1] > threshold
						|| diff[2] > threshold
						|| diff[3] > threshold)
				{
					isOk	= false;
					maxDiff	= tcu::max(maxDiff, diff);
					errorMask.getAccess().setPixel(errorColor, x, y);
				}
			}

			log << TestLog::Image("Average resolved values in attachment 0", "Average resolved values in attachment 0", m_sum);

			if (!isOk)
			{
				m_context.getTestContext().getLog() << tcu::LogImage("ErrorMask", "ErrorMask", errorMask.getAccess());

				log << TestLog::Message << "Average resolved values differ from expected average values by more than " << threshold << " max per component diff " << maxDiff << TestLog::EndMessage;
			}
		}

		return tcu::TestStatus(m_resultCollector.getResult(), m_resultCollector.getMessage());
	}
	else
	{
		m_sampleMask++;
		return tcu::TestStatus::incomplete();
	}
}

struct Programs
{
	void init (vk::SourceCollections& dst, TestConfig config) const
	{
		const tcu::TextureFormat		format			(mapVkFormat(config.format));
		const tcu::TextureChannelClass	channelClass	(tcu::getTextureChannelClass(format.type));

		dst.glslSources.add("quad-vert") << glu::VertexSource(
			"#version 450\n"
			"out gl_PerVertex {\n"
			"\tvec4 gl_Position;\n"
			"};\n"
			"highp float;\n"
			"void main (void) {\n"
			"\tgl_Position = vec4(((gl_VertexIndex + 2) / 3) % 2 == 0 ? -1.0 : 1.0,\n"
			"\t                   ((gl_VertexIndex + 1) / 3) % 2 == 0 ? -1.0 : 1.0, 0.0, 1.0);\n"
			"}\n");

		switch (channelClass)
		{
			case tcu::TEXTURECHANNELCLASS_UNSIGNED_INTEGER:
				dst.glslSources.add("quad-frag") << glu::FragmentSource(
					"#version 450\n"
					"layout(push_constant) uniform PushConstant {\n"
					"\thighp uint sampleMask;\n"
					"} pushConstants;\n"
					"layout(location = 0) out highp uvec4 o_color0;\n"
					"layout(location = 1) out highp uvec4 o_color1;\n"
					"layout(location = 2) out highp uvec4 o_color2;\n"
					"layout(location = 3) out highp uvec4 o_color3;\n"
					"void main (void)\n"
					"{\n"
					"\tgl_SampleMask[0] = int(pushConstants.sampleMask);\n"
					"\to_color0 = uvec4(255);\n"
					"\to_color1 = uvec4(255);\n"
					"\to_color2 = uvec4(255);\n"
					"\to_color3 = uvec4(255);\n"
					"}\n");
				break;

			case tcu::TEXTURECHANNELCLASS_SIGNED_INTEGER:
				dst.glslSources.add("quad-frag") << glu::FragmentSource(
					"#version 450\n"
					"layout(push_constant) uniform PushConstant {\n"
					"\thighp uint sampleMask;\n"
					"} pushConstants;\n"
					"layout(location = 0) out highp ivec4 o_color0;\n"
					"layout(location = 1) out highp ivec4 o_color1;\n"
					"layout(location = 2) out highp ivec4 o_color2;\n"
					"layout(location = 3) out highp ivec4 o_color3;\n"
					"void main (void)\n"
					"{\n"
					"\tgl_SampleMask[0] = int(pushConstants.sampleMask);\n"
					"\to_color0 = ivec4(127);\n"
					"\to_color1 = ivec4(127);\n"
					"\to_color2 = ivec4(127);\n"
					"\to_color3 = ivec4(127);\n"
					"}\n");
				break;

			case tcu::TEXTURECHANNELCLASS_FLOATING_POINT:
			case tcu::TEXTURECHANNELCLASS_UNSIGNED_FIXED_POINT:
			case tcu::TEXTURECHANNELCLASS_SIGNED_FIXED_POINT:
				dst.glslSources.add("quad-frag") << glu::FragmentSource(
					"#version 450\n"
					"layout(push_constant) uniform PushConstant {\n"
					"\thighp uint sampleMask;\n"
					"} pushConstants;\n"
					"layout(location = 0) out highp vec4 o_color0;\n"
					"layout(location = 1) out highp vec4 o_color1;\n"
					"layout(location = 2) out highp vec4 o_color2;\n"
					"layout(location = 3) out highp vec4 o_color3;\n"
					"void main (void)\n"
					"{\n"
					"\tgl_SampleMask[0] = int(pushConstants.sampleMask);\n"
					"\to_color0 = vec4(1.0);\n"
					"\to_color1 = vec4(1.0);\n"
					"\to_color2 = vec4(1.0);\n"
					"\to_color3 = vec4(1.0);\n"
					"}\n");
				break;

			default:
				DE_FATAL("Unknown channel class");
		}
	}
};

std::string formatToName (VkFormat format)
{
	const std::string	formatStr	= de::toString(format);
	const std::string	prefix		= "VK_FORMAT_";

	DE_ASSERT(formatStr.substr(0, prefix.length()) == prefix);

	return de::toLower(formatStr.substr(prefix.length()));
}

void initTests (tcu::TestCaseGroup* group)
{
	static const VkFormat	formats[]	=
	{
		VK_FORMAT_R5G6B5_UNORM_PACK16,
		VK_FORMAT_R8_UNORM,
		VK_FORMAT_R8_SNORM,
		VK_FORMAT_R8_UINT,
		VK_FORMAT_R8_SINT,
		VK_FORMAT_R8G8_UNORM,
		VK_FORMAT_R8G8_SNORM,
		VK_FORMAT_R8G8_UINT,
		VK_FORMAT_R8G8_SINT,
		VK_FORMAT_R8G8B8A8_UNORM,
		VK_FORMAT_R8G8B8A8_SNORM,
		VK_FORMAT_R8G8B8A8_UINT,
		VK_FORMAT_R8G8B8A8_SINT,
		VK_FORMAT_R8G8B8A8_SRGB,
		VK_FORMAT_A8B8G8R8_UNORM_PACK32,
		VK_FORMAT_A8B8G8R8_SNORM_PACK32,
		VK_FORMAT_A8B8G8R8_UINT_PACK32,
		VK_FORMAT_A8B8G8R8_SINT_PACK32,
		VK_FORMAT_A8B8G8R8_SRGB_PACK32,
		VK_FORMAT_B8G8R8A8_UNORM,
		VK_FORMAT_B8G8R8A8_SRGB,
		VK_FORMAT_A2R10G10B10_UNORM_PACK32,
		VK_FORMAT_A2B10G10R10_UNORM_PACK32,
		VK_FORMAT_A2B10G10R10_UINT_PACK32,
		VK_FORMAT_R16_UNORM,
		VK_FORMAT_R16_SNORM,
		VK_FORMAT_R16_UINT,
		VK_FORMAT_R16_SINT,
		VK_FORMAT_R16_SFLOAT,
		VK_FORMAT_R16G16_UNORM,
		VK_FORMAT_R16G16_SNORM,
		VK_FORMAT_R16G16_UINT,
		VK_FORMAT_R16G16_SINT,
		VK_FORMAT_R16G16_SFLOAT,
		VK_FORMAT_R16G16B16A16_UNORM,
		VK_FORMAT_R16G16B16A16_SNORM,
		VK_FORMAT_R16G16B16A16_UINT,
		VK_FORMAT_R16G16B16A16_SINT,
		VK_FORMAT_R16G16B16A16_SFLOAT,
		VK_FORMAT_R32_UINT,
		VK_FORMAT_R32_SINT,
		VK_FORMAT_R32_SFLOAT,
		VK_FORMAT_R32G32_UINT,
		VK_FORMAT_R32G32_SINT,
		VK_FORMAT_R32G32_SFLOAT,
		VK_FORMAT_R32G32B32A32_UINT,
		VK_FORMAT_R32G32B32A32_SINT,
		VK_FORMAT_R32G32B32A32_SFLOAT,
	};
	const deUint32			sampleCounts[] =
	{
		2u, 4u, 8u
	};
	tcu::TestContext&		testCtx		(group->getTestContext());

	for (size_t formatNdx = 0; formatNdx < DE_LENGTH_OF_ARRAY(formats); formatNdx++)
	{
		const VkFormat					format		(formats[formatNdx]);
		const std::string				formatName	(formatToName(format));
		de::MovePtr<tcu::TestCaseGroup>	formatGroup	(new tcu::TestCaseGroup(testCtx, formatName.c_str(), formatName.c_str()));

		for (size_t sampleCountNdx = 0; sampleCountNdx < DE_LENGTH_OF_ARRAY(sampleCounts); sampleCountNdx++)
		{
			const deUint32		sampleCount	(sampleCounts[sampleCountNdx]);
			const std::string	testName	("samples_" + de::toString(sampleCount));

			formatGroup->addChild(new InstanceFactory1<MultisampleRenderPassTestInstance, TestConfig, Programs>(testCtx, tcu::NODETYPE_SELF_VALIDATE, testName.c_str(), testName.c_str(), TestConfig(format, sampleCount)));
		}

		group->addChild(formatGroup.release());
	}
}

} // anonymous

tcu::TestCaseGroup* createRenderPassMultisampleResolveTests (tcu::TestContext& testCtx)
{
	return createTestGroup(testCtx, "multisample_resolve", "Multisample render pass resolve tests", initTests);
}

} // vkt
