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
 * \brief Tests for render passses with multisample attachments
 *//*--------------------------------------------------------------------*/

#include "vktRenderPassMultisampleTests.hpp"

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

typedef de::SharedPtr<vk::Unique<VkImage> > VkImageSp;
typedef de::SharedPtr<vk::Unique<VkImageView> > VkImageViewSp;
typedef de::SharedPtr<vk::Unique<VkBuffer> > VkBufferSp;
typedef de::SharedPtr<vk::Unique<VkPipeline> > VkPipelineSp;

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

VkImageAspectFlags getImageAspectFlags (VkFormat vkFormat)
{
	const tcu::TextureFormat	format		(mapVkFormat(vkFormat));
	const bool					hasDepth	(tcu::hasDepthComponent(format.order));
	const bool					hasStencil	(tcu::hasStencilComponent(format.order));

	if (hasDepth || hasStencil)
	{
		return (hasDepth ? VK_IMAGE_ASPECT_DEPTH_BIT : (VkImageAspectFlagBits)0u)
				| (hasStencil ? VK_IMAGE_ASPECT_STENCIL_BIT : (VkImageAspectFlagBits)0u);
	}
	else
		return VK_IMAGE_ASPECT_COLOR_BIT;
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

Move<VkImageView> createImageAttachmentView (const DeviceInterface&	vkd,
											 VkDevice				device,
											 VkImage				image,
											 VkFormat				format,
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

Move<VkImageView> createSrcPrimaryInputImageView (const DeviceInterface&	vkd,
												  VkDevice					device,
												  VkImage					image,
												  VkFormat					format,
												  VkImageAspectFlags		aspect)
{
	const VkImageSubresourceRange	range =
	{
		aspect == (VK_IMAGE_ASPECT_STENCIL_BIT | VK_IMAGE_ASPECT_DEPTH_BIT)
			? (VkImageAspectFlags)VK_IMAGE_ASPECT_DEPTH_BIT
			: aspect,
		0u,
		1u,
		0u,
		1u
	};

	return createImageView(vkd, device, 0u, image, VK_IMAGE_VIEW_TYPE_2D, format, makeComponentMappingRGBA(), range);
}

Move<VkImageView> createSrcSecondaryInputImageView (const DeviceInterface&	vkd,
													VkDevice				device,
													VkImage					image,
													VkFormat				format,
													VkImageAspectFlags		aspect)
{
	if (aspect == (VK_IMAGE_ASPECT_STENCIL_BIT | VK_IMAGE_ASPECT_DEPTH_BIT))
	{
		const VkImageSubresourceRange	range =
		{
			VK_IMAGE_ASPECT_STENCIL_BIT,
			0u,
			1u,
			0u,
			1u
		};

		return createImageView(vkd, device, 0u, image, VK_IMAGE_VIEW_TYPE_2D, format, makeComponentMappingRGBA(), range);
	}
	else
		return Move<VkImageView>();
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

VkSampleCountFlagBits sampleCountBitFromomSampleCount (deUint32 count)
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

std::vector<VkImageSp> createMultisampleImages (const InstanceInterface&	vki,
												VkPhysicalDevice			physicalDevice,
												const DeviceInterface&		vkd,
												VkDevice					device,
												VkFormat					format,
												deUint32					sampleCount,
												deUint32					width,
												deUint32					height)
{
	std::vector<VkImageSp> images (sampleCount);

	for (size_t imageNdx = 0; imageNdx < images.size(); imageNdx++)
		images[imageNdx] = safeSharedPtr(new vk::Unique<VkImage>(createImage(vki, physicalDevice, vkd, device, format, sampleCountBitFromomSampleCount(sampleCount), VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, width, height)));

	return images;
}

std::vector<VkImageSp> createSingleSampleImages (const InstanceInterface&	vki,
												 VkPhysicalDevice			physicalDevice,
												 const DeviceInterface&		vkd,
												 VkDevice					device,
												 VkFormat					format,
												 deUint32					sampleCount,
												 deUint32					width,
												 deUint32					height)
{
	std::vector<VkImageSp> images (sampleCount);

	for (size_t imageNdx = 0; imageNdx < images.size(); imageNdx++)
		images[imageNdx] = safeSharedPtr(new vk::Unique<VkImage>(createImage(vki, physicalDevice, vkd, device, format, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, width, height)));

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

std::vector<VkImageViewSp> createImageAttachmentViews (const DeviceInterface&			vkd,
													   VkDevice							device,
													   const std::vector<VkImageSp>&	images,
													   VkFormat							format,
													   VkImageAspectFlagBits			aspect)
{
	std::vector<VkImageViewSp> views (images.size());

	for (size_t imageNdx = 0; imageNdx < images.size(); imageNdx++)
		views[imageNdx] = safeSharedPtr(new vk::Unique<VkImageView>(createImageAttachmentView(vkd, device, **images[imageNdx], format, aspect)));

	return views;
}

std::vector<VkBufferSp> createBuffers (const DeviceInterface&	vkd,
									   VkDevice					device,
									   VkFormat					format,
									   deUint32					sampleCount,
									   deUint32					width,
									   deUint32					height)
{
	std::vector<VkBufferSp> buffers (sampleCount);

	for (size_t bufferNdx = 0; bufferNdx < buffers.size(); bufferNdx++)
		buffers[bufferNdx] = safeSharedPtr(new vk::Unique<VkBuffer>(createBuffer(vkd, device, format, width, height)));

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
									 VkFormat				srcFormat,
									 VkFormat				dstFormat,
									 deUint32				sampleCount)
{
	const VkSampleCountFlagBits				samples						(sampleCountBitFromomSampleCount(sampleCount));
	const deUint32							splitSubpassCount			(deDivRoundUp32(sampleCount, MAX_COLOR_ATTACHMENT_COUNT));
	const tcu::TextureFormat				format						(mapVkFormat(srcFormat));
	const bool								isDepthStencilFormat		(tcu::hasDepthComponent(format.order) || tcu::hasStencilComponent(format.order));
	vector<VkSubpassDescription>			subpasses;
	vector<vector<VkAttachmentReference> >	dstAttachmentRefs			(splitSubpassCount);
	vector<vector<VkAttachmentReference> >	dstResolveAttachmentRefs	(splitSubpassCount);
	vector<VkAttachmentDescription>			attachments;
	vector<VkSubpassDependency>				dependencies;
	const VkAttachmentReference				srcAttachmentRef =
	{
		0u,
		isDepthStencilFormat
			? VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
			: VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
	};
	const VkAttachmentReference				srcAttachmentInputRef =
	{
		0u,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
	};

	{
		const VkAttachmentDescription srcAttachment =
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

		attachments.push_back(srcAttachment);
	}

	for (deUint32 splitSubpassIndex = 0; splitSubpassIndex < splitSubpassCount; splitSubpassIndex++)
	{
		for (deUint32 sampleNdx = 0; sampleNdx < de::min((deUint32)MAX_COLOR_ATTACHMENT_COUNT, sampleCount  - splitSubpassIndex * MAX_COLOR_ATTACHMENT_COUNT); sampleNdx++)
		{
			// Multisample color attachment
			{
				const VkAttachmentDescription dstAttachment =
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
				const VkAttachmentReference dstAttachmentRef =
				{
					(deUint32)attachments.size(),
					VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
				};

				attachments.push_back(dstAttachment);
				dstAttachmentRefs[splitSubpassIndex].push_back(dstAttachmentRef);
			}
			// Resolve attachment
			{
				const VkAttachmentDescription dstAttachment =
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
				const VkAttachmentReference dstAttachmentRef =
				{
					(deUint32)attachments.size(),
					VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
				};

				attachments.push_back(dstAttachment);
				dstResolveAttachmentRefs[splitSubpassIndex].push_back(dstAttachmentRef);
			}
		}
	}

	{
		{
			const VkSubpassDescription	subpass =
			{
				(VkSubpassDescriptionFlags)0,
				VK_PIPELINE_BIND_POINT_GRAPHICS,

				0u,
				DE_NULL,

				isDepthStencilFormat ? 0u : 1u,
				isDepthStencilFormat ? DE_NULL : &srcAttachmentRef,
				DE_NULL,

				isDepthStencilFormat ? &srcAttachmentRef : DE_NULL,
				0u,
				DE_NULL
			};

			subpasses.push_back(subpass);
		}

		for (deUint32 splitSubpassIndex = 0; splitSubpassIndex < splitSubpassCount; splitSubpassIndex++)
		{
			{
				const VkSubpassDescription	subpass =
				{
					(VkSubpassDescriptionFlags)0,
					VK_PIPELINE_BIND_POINT_GRAPHICS,

					1u,
					&srcAttachmentInputRef,

					(deUint32)dstAttachmentRefs[splitSubpassIndex].size(),
					&dstAttachmentRefs[splitSubpassIndex][0],
					&dstResolveAttachmentRefs[splitSubpassIndex][0],

					DE_NULL,
					0u,
					DE_NULL
				};
				subpasses.push_back(subpass);
			}
			{
				const VkSubpassDependency		dependency	=
				{
					0u, splitSubpassIndex + 1,
					VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
					VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,

					VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
					VK_ACCESS_INPUT_ATTACHMENT_READ_BIT,

					VK_DEPENDENCY_BY_REGION_BIT
				};

				dependencies.push_back(dependency);
			}
		};
		const VkRenderPassCreateInfo	createInfo	=
		{
			VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
			DE_NULL,
			(VkRenderPassCreateFlags)0u,

			(deUint32)attachments.size(),
			&attachments[0],

			(deUint32)subpasses.size(),
			&subpasses[0],

			(deUint32)dependencies.size(),
			&dependencies[0]
		};

		return createRenderPass(vkd, device, &createInfo);
	}
}

Move<VkFramebuffer> createFramebuffer (const DeviceInterface&				vkd,
									   VkDevice								device,
									   VkRenderPass							renderPass,
									   VkImageView							srcImageView,
									   const std::vector<VkImageViewSp>&	dstMultisampleImageViews,
									   const std::vector<VkImageViewSp>&	dstSinglesampleImageViews,
									   deUint32								width,
									   deUint32								height)
{
	std::vector<VkImageView> attachments;

	attachments.reserve(dstMultisampleImageViews.size() + dstSinglesampleImageViews.size() + 1u);

	attachments.push_back(srcImageView);

	DE_ASSERT(dstMultisampleImageViews.size() == dstSinglesampleImageViews.size());

	for (size_t ndx = 0; ndx < dstMultisampleImageViews.size(); ndx++)
	{
		attachments.push_back(**dstMultisampleImageViews[ndx]);
		attachments.push_back(**dstSinglesampleImageViews[ndx]);
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
									   VkFormat											srcFormat,
									   VkRenderPass										renderPass,
									   VkPipelineLayout									pipelineLayout,
									   const vk::ProgramCollection<vk::ProgramBinary>&	binaryCollection,
									   deUint32											width,
									   deUint32											height,
									   deUint32											sampleCount)
{
	const tcu::TextureFormat		format						(mapVkFormat(srcFormat));
	const bool						isDepthStencilFormat		(tcu::hasDepthComponent(format.order) || tcu::hasStencilComponent(format.order));

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

		sampleCountBitFromomSampleCount(sampleCount),
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

		VK_TRUE,
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
		(isDepthStencilFormat ? 0u : 1u),
		(isDepthStencilFormat ? DE_NULL : &attachmentBlendState),
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

Move<VkDescriptorSetLayout> createSplitDescriptorSetLayout (const DeviceInterface&	vkd,
															VkDevice				device,
															VkFormat				vkFormat)
{
	const tcu::TextureFormat				format		(mapVkFormat(vkFormat));
	const bool								hasDepth	(tcu::hasDepthComponent(format.order));
	const bool								hasStencil	(tcu::hasStencilComponent(format.order));
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

		hasDepth && hasStencil ? 2u : 1u,
		bindings
	};

	return createDescriptorSetLayout(vkd, device, &createInfo);
}

Move<VkPipelineLayout> createSplitPipelineLayout (const DeviceInterface&	vkd,
												  VkDevice					device,
												  VkDescriptorSetLayout		descriptorSetLayout)
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

		1u,
		&descriptorSetLayout,

		1u,
		&pushConstant
	};

	return createPipelineLayout(vkd, device, &createInfo);
}

Move<VkPipeline> createSplitPipeline (const DeviceInterface&							vkd,
									  VkDevice											device,
									  VkRenderPass										renderPass,
									  deUint32											subpassIndex,
									  VkPipelineLayout									pipelineLayout,
									  const vk::ProgramCollection<vk::ProgramBinary>&	binaryCollection,
									  deUint32											width,
									  deUint32											height,
									  deUint32											sampleCount)
{
	const Unique<VkShaderModule>	vertexShaderModule			(createShaderModule(vkd, device, binaryCollection.get("quad-vert"), 0u));
	const Unique<VkShaderModule>	fragmentShaderModule		(createShaderModule(vkd, device, binaryCollection.get("quad-split-frag"), 0u));
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
	const std::vector<VkPipelineColorBlendAttachmentState> attachmentBlendStates (de::min((deUint32)MAX_COLOR_ATTACHMENT_COUNT, sampleCount), attachmentBlendState);
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

		sampleCountBitFromomSampleCount(sampleCount),
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
		VK_FALSE,
		VK_COMPARE_OP_ALWAYS,
		VK_FALSE,
		VK_FALSE,
		{
			VK_STENCIL_OP_REPLACE,
			VK_STENCIL_OP_REPLACE,
			VK_STENCIL_OP_REPLACE,
			VK_COMPARE_OP_ALWAYS,
			~0u,
			~0u,
			0x0u
		},
		{
			VK_STENCIL_OP_REPLACE,
			VK_STENCIL_OP_REPLACE,
			VK_STENCIL_OP_REPLACE,
			VK_COMPARE_OP_ALWAYS,
			~0u,
			~0u,
			0x0u
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

		(deUint32)attachmentBlendStates.size(),
		&attachmentBlendStates[0],

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
		subpassIndex,
		DE_NULL,
		0u
	};

	return createGraphicsPipeline(vkd, device, DE_NULL, &createInfo);
}

vector<VkPipelineSp> createSplitPipelines (const DeviceInterface&							vkd,
										 VkDevice											device,
										 VkRenderPass										renderPass,
										 VkPipelineLayout									pipelineLayout,
										 const vk::ProgramCollection<vk::ProgramBinary>&	binaryCollection,
										 deUint32											width,
										 deUint32											height,
										 deUint32											sampleCount)
{
	std::vector<VkPipelineSp> pipelines (deDivRoundUp32(sampleCount, MAX_COLOR_ATTACHMENT_COUNT), (VkPipelineSp)0u);

	for (size_t ndx = 0; ndx < pipelines.size(); ndx++)
		pipelines[ndx] = safeSharedPtr(new Unique<VkPipeline>(createSplitPipeline(vkd, device, renderPass, (deUint32)(ndx + 1), pipelineLayout, binaryCollection, width, height, sampleCount)));

	return pipelines;
}

Move<VkDescriptorPool> createSplitDescriptorPool (const DeviceInterface&	vkd,
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

Move<VkDescriptorSet> createSplitDescriptorSet (const DeviceInterface&	vkd,
												VkDevice				device,
												VkDescriptorPool		pool,
												VkDescriptorSetLayout	layout,
												VkImageView				primaryImageView,
												VkImageView				secondaryImageView)
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
		const VkDescriptorImageInfo	imageInfos[]	=
		{
			{
				(VkSampler)0u,
				primaryImageView,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
			},
			{
				(VkSampler)0u,
				secondaryImageView,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
			}
		};
		const VkWriteDescriptorSet	writes[]	=
		{
			{
				VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				DE_NULL,

				*set,
				0u,
				0u,
				1u,
				VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,
				&imageInfos[0],
				DE_NULL,
				DE_NULL
			},
			{
				VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				DE_NULL,

				*set,
				1u,
				0u,
				1u,
				VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,
				&imageInfos[1],
				DE_NULL,
				DE_NULL
			}
		};
		const deUint32	count	= secondaryImageView != (VkImageView)0
								? 2u
								: 1u;

		vkd.updateDescriptorSets(device, count, writes, 0u, DE_NULL);
	}
	return set;
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

VkImageUsageFlags getSrcImageUsage (VkFormat vkFormat)
{
	const tcu::TextureFormat	format		(mapVkFormat(vkFormat));
	const bool					hasDepth	(tcu::hasDepthComponent(format.order));
	const bool					hasStencil	(tcu::hasStencilComponent(format.order));

	if (hasDepth || hasStencil)
		return VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
	else
		return VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
}

VkFormat getDstFormat (VkFormat vkFormat)
{
	const tcu::TextureFormat	format		(mapVkFormat(vkFormat));
	const bool					hasDepth	(tcu::hasDepthComponent(format.order));
	const bool					hasStencil	(tcu::hasStencilComponent(format.order));

	if (hasDepth && hasStencil)
		return VK_FORMAT_R32G32_SFLOAT;
	else if (hasDepth || hasStencil)
		return VK_FORMAT_R32_SFLOAT;
	else
		return vkFormat;
}


class MultisampleRenderPassTestInstance : public TestInstance
{
public:
					MultisampleRenderPassTestInstance	(Context& context, TestConfig config);
					~MultisampleRenderPassTestInstance	(void);

	tcu::TestStatus	iterate								(void);

private:
	const VkFormat									m_srcFormat;
	const VkFormat									m_dstFormat;
	const deUint32									m_sampleCount;
	const deUint32									m_width;
	const deUint32									m_height;

	const VkImageAspectFlags						m_srcImageAspect;
	const VkImageUsageFlags							m_srcImageUsage;
	const Unique<VkImage>							m_srcImage;
	const de::UniquePtr<Allocation>					m_srcImageMemory;
	const Unique<VkImageView>						m_srcImageView;
	const Unique<VkImageView>						m_srcPrimaryInputImageView;
	const Unique<VkImageView>						m_srcSecondaryInputImageView;

	const std::vector<VkImageSp>					m_dstMultisampleImages;
	const std::vector<de::SharedPtr<Allocation> >	m_dstMultisampleImageMemory;
	const std::vector<VkImageViewSp>				m_dstMultisampleImageViews;

	const std::vector<VkImageSp>					m_dstSinglesampleImages;
	const std::vector<de::SharedPtr<Allocation> >	m_dstSinglesampleImageMemory;
	const std::vector<VkImageViewSp>				m_dstSinglesampleImageViews;

	const std::vector<VkBufferSp>					m_dstBuffers;
	const std::vector<de::SharedPtr<Allocation> >	m_dstBufferMemory;

	const Unique<VkRenderPass>						m_renderPass;
	const Unique<VkFramebuffer>						m_framebuffer;

	const Unique<VkPipelineLayout>					m_renderPipelineLayout;
	const Unique<VkPipeline>						m_renderPipeline;

	const Unique<VkDescriptorSetLayout>				m_splitDescriptorSetLayout;
	const Unique<VkPipelineLayout>					m_splitPipelineLayout;
	const std::vector<VkPipelineSp>					m_splitPipelines;
	const Unique<VkDescriptorPool>					m_splitDescriptorPool;
	const Unique<VkDescriptorSet>					m_splitDescriptorSet;

	const Unique<VkCommandPool>						m_commandPool;
	tcu::ResultCollector							m_resultCollector;
};

MultisampleRenderPassTestInstance::MultisampleRenderPassTestInstance (Context& context, TestConfig config)
	: TestInstance					(context)
	, m_srcFormat					(config.format)
	, m_dstFormat					(getDstFormat(config.format))
	, m_sampleCount					(config.sampleCount)
	, m_width						(32u)
	, m_height						(32u)

	, m_srcImageAspect				(getImageAspectFlags(m_srcFormat))
	, m_srcImageUsage				(getSrcImageUsage(m_srcFormat))
	, m_srcImage					(createImage(context.getInstanceInterface(), context.getPhysicalDevice(), context.getDeviceInterface(), context.getDevice(), m_srcFormat, sampleCountBitFromomSampleCount(m_sampleCount), m_srcImageUsage, m_width, m_height))
	, m_srcImageMemory				(createImageMemory(context.getDeviceInterface(), context.getDevice(), context.getDefaultAllocator(), *m_srcImage))
	, m_srcImageView				(createImageAttachmentView(context.getDeviceInterface(), context.getDevice(), *m_srcImage, m_srcFormat, m_srcImageAspect))
	, m_srcPrimaryInputImageView	(createSrcPrimaryInputImageView(context.getDeviceInterface(), context.getDevice(), *m_srcImage, m_srcFormat, m_srcImageAspect))
	, m_srcSecondaryInputImageView	(createSrcSecondaryInputImageView(context.getDeviceInterface(), context.getDevice(), *m_srcImage, m_srcFormat, m_srcImageAspect))

	, m_dstMultisampleImages		(createMultisampleImages(context.getInstanceInterface(), context.getPhysicalDevice(), context.getDeviceInterface(), context.getDevice(), m_dstFormat, m_sampleCount, m_width, m_height))
	, m_dstMultisampleImageMemory	(createImageMemory(context.getDeviceInterface(), context.getDevice(), context.getDefaultAllocator(), m_dstMultisampleImages))
	, m_dstMultisampleImageViews	(createImageAttachmentViews(context.getDeviceInterface(), context.getDevice(), m_dstMultisampleImages, m_dstFormat, VK_IMAGE_ASPECT_COLOR_BIT))

	, m_dstSinglesampleImages		(createSingleSampleImages(context.getInstanceInterface(), context.getPhysicalDevice(), context.getDeviceInterface(), context.getDevice(), m_dstFormat, m_sampleCount, m_width, m_height))
	, m_dstSinglesampleImageMemory	(createImageMemory(context.getDeviceInterface(), context.getDevice(), context.getDefaultAllocator(), m_dstSinglesampleImages))
	, m_dstSinglesampleImageViews	(createImageAttachmentViews(context.getDeviceInterface(), context.getDevice(), m_dstSinglesampleImages, m_dstFormat, VK_IMAGE_ASPECT_COLOR_BIT))

	, m_dstBuffers					(createBuffers(context.getDeviceInterface(), context.getDevice(), m_dstFormat, m_sampleCount, m_width, m_height))
	, m_dstBufferMemory				(createBufferMemory(context.getDeviceInterface(), context.getDevice(), context.getDefaultAllocator(), m_dstBuffers))

	, m_renderPass					(createRenderPass(context.getDeviceInterface(), context.getDevice(), m_srcFormat, m_dstFormat, m_sampleCount))
	, m_framebuffer					(createFramebuffer(context.getDeviceInterface(), context.getDevice(), *m_renderPass, *m_srcImageView, m_dstMultisampleImageViews, m_dstSinglesampleImageViews, m_width, m_height))

	, m_renderPipelineLayout		(createRenderPipelineLayout(context.getDeviceInterface(), context.getDevice()))
	, m_renderPipeline				(createRenderPipeline(context.getDeviceInterface(), context.getDevice(), m_srcFormat, *m_renderPass, *m_renderPipelineLayout, context.getBinaryCollection(), m_width, m_height, m_sampleCount))

	, m_splitDescriptorSetLayout	(createSplitDescriptorSetLayout(context.getDeviceInterface(), context.getDevice(), m_srcFormat))
	, m_splitPipelineLayout			(createSplitPipelineLayout(context.getDeviceInterface(), context.getDevice(), *m_splitDescriptorSetLayout))
	, m_splitPipelines				(createSplitPipelines(context.getDeviceInterface(), context.getDevice(), *m_renderPass, *m_splitPipelineLayout, context.getBinaryCollection(), m_width, m_height, m_sampleCount))
	, m_splitDescriptorPool			(createSplitDescriptorPool(context.getDeviceInterface(), context.getDevice()))
	, m_splitDescriptorSet			(createSplitDescriptorSet(context.getDeviceInterface(), context.getDevice(), *m_splitDescriptorPool, *m_splitDescriptorSetLayout, *m_srcPrimaryInputImageView, *m_srcSecondaryInputImageView))
	, m_commandPool					(createCommandPool(context.getDeviceInterface(), context.getDevice(), VK_COMMAND_POOL_CREATE_TRANSIENT_BIT, context.getUniversalQueueFamilyIndex()))
{
}

MultisampleRenderPassTestInstance::~MultisampleRenderPassTestInstance (void)
{
}

tcu::TestStatus MultisampleRenderPassTestInstance::iterate (void)
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

		// Stencil needs to be cleared if it exists.
		if (tcu::hasStencilComponent(mapVkFormat(m_srcFormat).order))
		{
			const VkClearAttachment clearAttachment =
			{
				VK_IMAGE_ASPECT_STENCIL_BIT,						// VkImageAspectFlags	aspectMask;
				0,													// deUint32				colorAttachment;
				makeClearValueDepthStencil(0, 0)					// VkClearValue			clearValue;
			};

			const VkClearRect clearRect =
			{
				{
					{ 0u, 0u },
					{ m_width, m_height }
				},
				0,													// deUint32	baseArrayLayer;
				1													// deUint32	layerCount;
			};

			vkd.cmdClearAttachments(*commandBuffer, 1, &clearAttachment, 1, &clearRect);
		}
	}

	vkd.cmdBindPipeline(*commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *m_renderPipeline);

	for (deUint32 sampleNdx = 0; sampleNdx < m_sampleCount; sampleNdx++)
	{
		vkd.cmdPushConstants(*commandBuffer, *m_renderPipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0u, sizeof(sampleNdx), &sampleNdx);
		vkd.cmdDraw(*commandBuffer, 6u, 1u, 0u, 0u);
	}

	for (deUint32 splitPipelineNdx = 0; splitPipelineNdx < m_splitPipelines.size(); splitPipelineNdx++)
	{
		vkd.cmdNextSubpass(*commandBuffer, VK_SUBPASS_CONTENTS_INLINE);

		vkd.cmdBindPipeline(*commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, **m_splitPipelines[splitPipelineNdx]);
		vkd.cmdBindDescriptorSets(*commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *m_splitPipelineLayout, 0u, 1u,  &*m_splitDescriptorSet, 0u, DE_NULL);
		vkd.cmdPushConstants(*commandBuffer, *m_splitPipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0u, sizeof(splitPipelineNdx), &splitPipelineNdx);
		vkd.cmdDraw(*commandBuffer, 6u, 1u, 0u, 0u);
	}

	vkd.cmdEndRenderPass(*commandBuffer);

	// Memory barriers between rendering and copies
	{
		std::vector<VkImageMemoryBarrier> barriers;

		for (size_t dstNdx = 0; dstNdx < m_dstSinglesampleImages.size(); dstNdx++)
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

				**m_dstSinglesampleImages[dstNdx],
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
	for (size_t dstNdx = 0; dstNdx < m_dstSinglesampleImages.size(); dstNdx++)
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

		vkd.cmdCopyImageToBuffer(*commandBuffer, **m_dstSinglesampleImages[dstNdx], VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, **m_dstBuffers[dstNdx], 1u, &region);
	}

	// Memory barriers between copies and host access
	{
		std::vector<VkBufferMemoryBarrier> barriers;

		for (size_t dstNdx = 0; dstNdx < m_dstBuffers.size(); dstNdx++)
		{
			const VkBufferMemoryBarrier barrier =
			{
				VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
				DE_NULL,

				VK_ACCESS_TRANSFER_WRITE_BIT,
				VK_ACCESS_HOST_READ_BIT,

				VK_QUEUE_FAMILY_IGNORED,
				VK_QUEUE_FAMILY_IGNORED,

				**m_dstBuffers[dstNdx],
				0u,
				VK_WHOLE_SIZE
			};

			barriers.push_back(barrier);
		}

		vkd.cmdPipelineBarrier(*commandBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0u, 0u, DE_NULL, (deUint32)barriers.size(), &barriers[0], 0u, DE_NULL);
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
		const tcu::TextureFormat		format			(mapVkFormat(m_dstFormat));
		const tcu::TextureFormat		srcFormat		(mapVkFormat(m_srcFormat));
		const bool						hasDepth		(tcu::hasDepthComponent(srcFormat.order));
		const bool						hasStencil		(tcu::hasStencilComponent(srcFormat.order));

		for (deUint32 sampleNdx = 0; sampleNdx < m_sampleCount; sampleNdx++)
		{
			const std::string					name		("Sample" + de::toString(sampleNdx));
			const void* const					ptr			(m_dstBufferMemory[sampleNdx]->getHostPtr());
			const tcu::ConstPixelBufferAccess	access		(format, m_width, m_height, 1, ptr);
			tcu::TextureLevel					reference	(format, m_width, m_height);

			if (hasDepth || hasStencil)
			{
				if (hasDepth)
				{
					for (deUint32 y = 0; y < m_height; y++)
					for (deUint32 x = 0; x < m_width; x++)
					{
						const deUint32	x1				= x ^ sampleNdx;
						const deUint32	y1				= y ^ sampleNdx;
						const float		range			= 1.0f;
						float			depth			= 0.0f;
						deUint32		divider			= 2;

						// \note Limited to ten bits since the target is 32x32, so there are 10 input bits
						for (size_t bitNdx = 0; bitNdx < 10; bitNdx++)
						{
							depth += (range / (float)divider)
									* (((bitNdx % 2 == 0 ? x1 : y1) & (0x1u << (bitNdx / 2u))) == 0u ? 0u : 1u);
							divider *= 2;
						}

						reference.getAccess().setPixel(Vec4(depth, 0.0f, 0.0f, 0.0f), x, y);
					}
				}
				if (hasStencil)
				{
					for (deUint32 y = 0; y < m_height; y++)
					for (deUint32 x = 0; x < m_width; x++)
					{
						const deUint32	stencil	= sampleNdx + 1u;

						if (hasDepth)
						{
							const Vec4 src (reference.getAccess().getPixel(x, y));

							reference.getAccess().setPixel(Vec4(src.x(), (float)stencil, 0.0f, 0.0f), x, y);
						}
						else
							reference.getAccess().setPixel(Vec4((float)stencil, 0.0f, 0.0f, 0.0f), x, y);
					}
				}
				{
					const Vec4 threshold (hasDepth ? (1.0f / 1024.0f) : 0.0f, 0.0f, 0.0f, 0.0f);

					if (!tcu::floatThresholdCompare(m_context.getTestContext().getLog(), name.c_str(), name.c_str(), reference.getAccess(), access, threshold, tcu::COMPARE_LOG_ON_ERROR))
						m_resultCollector.fail("Compare failed for sample " + de::toString(sampleNdx));
				}
			}
			else
			{
				const tcu::TextureChannelClass	channelClass	(tcu::getTextureChannelClass(format.type));

				switch (channelClass)
				{
					case tcu::TEXTURECHANNELCLASS_UNSIGNED_INTEGER:
					{
						const UVec4		bits			(tcu::getTextureFormatBitDepth(format).cast<deUint32>());
						const UVec4		minValue		(0);
						const UVec4		range			(UVec4(1u) << tcu::min(bits, UVec4(31)));
						const int		componentCount	(tcu::getNumUsedChannels(format.order));
						const deUint32	bitSize			(bits[0] + bits[1] + bits[2] + bits[3]);

						for (deUint32 y = 0; y < m_height; y++)
						for (deUint32 x = 0; x < m_width; x++)
						{
							const deUint32	x1				= x ^ sampleNdx;
							const deUint32	y1				= y ^ sampleNdx;
							UVec4			color			(minValue);
							deUint32		dstBitsUsed[4]	= { 0u, 0u, 0u, 0u };
							deUint32		nextSrcBit		= 0;
							deUint32		divider			= 2;

							// \note Limited to ten bits since the target is 32x32, so there are 10 input bits
							while (nextSrcBit < de::min(bitSize, 10u))
							{
								for (int compNdx = 0; compNdx < componentCount; compNdx++)
								{
									if (dstBitsUsed[compNdx] > bits[compNdx])
										continue;

									color[compNdx] += (range[compNdx] / divider)
													* (((nextSrcBit % 2 == 0 ? x1 : y1) & (0x1u << (nextSrcBit / 2u))) == 0u ? 0u : 1u);

									nextSrcBit++;
									dstBitsUsed[compNdx]++;
								}

								divider *= 2;
							}

							reference.getAccess().setPixel(color, x, y);
						}

						if (!tcu::intThresholdCompare(m_context.getTestContext().getLog(), name.c_str(), name.c_str(), reference.getAccess(), access, UVec4(0u), tcu::COMPARE_LOG_ON_ERROR))
							m_resultCollector.fail("Compare failed for sample " + de::toString(sampleNdx));

						break;
					}

					case tcu::TEXTURECHANNELCLASS_SIGNED_INTEGER:
					{
						const UVec4		bits			(tcu::getTextureFormatBitDepth(format).cast<deUint32>());
						const IVec4		minValue		(0);
						const IVec4		range			((UVec4(1u) << tcu::min(bits, UVec4(30))).cast<deInt32>());
						const int		componentCount	(tcu::getNumUsedChannels(format.order));
						const deUint32	bitSize			(bits[0] + bits[1] + bits[2] + bits[3]);

						for (deUint32 y = 0; y < m_height; y++)
						for (deUint32 x = 0; x < m_width; x++)
						{
							const deUint32	x1				= x ^ sampleNdx;
							const deUint32	y1				= y ^ sampleNdx;
							IVec4			color			(minValue);
							deUint32		dstBitsUsed[4]	= { 0u, 0u, 0u, 0u };
							deUint32		nextSrcBit		= 0;
							deUint32		divider			= 2;

							// \note Limited to ten bits since the target is 32x32, so there are 10 input bits
							while (nextSrcBit < de::min(bitSize, 10u))
							{
								for (int compNdx = 0; compNdx < componentCount; compNdx++)
								{
									if (dstBitsUsed[compNdx] > bits[compNdx])
										continue;

									color[compNdx] += (range[compNdx] / divider)
													* (((nextSrcBit % 2 == 0 ? x1 : y1) & (0x1u << (nextSrcBit / 2u))) == 0u ? 0u : 1u);

									nextSrcBit++;
									dstBitsUsed[compNdx]++;
								}

								divider *= 2;
							}

							reference.getAccess().setPixel(color, x, y);
						}

						if (!tcu::intThresholdCompare(m_context.getTestContext().getLog(), name.c_str(), name.c_str(), reference.getAccess(), access, UVec4(0u), tcu::COMPARE_LOG_ON_ERROR))
							m_resultCollector.fail("Compare failed for sample " + de::toString(sampleNdx));

						break;
					}

					case tcu::TEXTURECHANNELCLASS_UNSIGNED_FIXED_POINT:
					case tcu::TEXTURECHANNELCLASS_SIGNED_FIXED_POINT:
					case tcu::TEXTURECHANNELCLASS_FLOATING_POINT:
					{
						const tcu::TextureFormatInfo	info			(tcu::getTextureFormatInfo(format));
						const UVec4						bits			(tcu::getTextureFormatBitDepth(format).cast<deUint32>());
						const Vec4						minLimit		(-65536.0);
						const Vec4						maxLimit		(65536.0);
						const Vec4						minValue		(tcu::max(info.valueMin, minLimit));
						const Vec4						range			(tcu::min(info.valueMax, maxLimit) - minValue);
						const int						componentCount	(tcu::getNumUsedChannels(format.order));
						const deUint32					bitSize			(bits[0] + bits[1] + bits[2] + bits[3]);

						for (deUint32 y = 0; y < m_height; y++)
						for (deUint32 x = 0; x < m_width; x++)
						{
							const deUint32	x1				= x ^ sampleNdx;
							const deUint32	y1				= y ^ sampleNdx;
							Vec4			color			(minValue);
							deUint32		dstBitsUsed[4]	= { 0u, 0u, 0u, 0u };
							deUint32		nextSrcBit		= 0;
							deUint32		divider			= 2;

							// \note Limited to ten bits since the target is 32x32, so there are 10 input bits
							while (nextSrcBit < de::min(bitSize, 10u))
							{
								for (int compNdx = 0; compNdx < componentCount; compNdx++)
								{
									if (dstBitsUsed[compNdx] > bits[compNdx])
										continue;

									color[compNdx] += (range[compNdx] / (float)divider)
													* (((nextSrcBit % 2 == 0 ? x1 : y1) & (0x1u << (nextSrcBit / 2u))) == 0u ? 0u : 1u);

									nextSrcBit++;
									dstBitsUsed[compNdx]++;
								}

								divider *= 2;
							}

							if (tcu::isSRGB(format))
								reference.getAccess().setPixel(tcu::linearToSRGB(color), x, y);
							else
								reference.getAccess().setPixel(color, x, y);
						}

						if (channelClass == tcu::TEXTURECHANNELCLASS_FLOATING_POINT)
						{
							// Convert target format ulps to float ulps and allow 64ulp differences
							const UVec4 threshold (64u * (UVec4(1u) << (UVec4(23) - tcu::getTextureFormatMantissaBitDepth(format).cast<deUint32>())));

							if (!tcu::floatUlpThresholdCompare(m_context.getTestContext().getLog(), name.c_str(), name.c_str(), reference.getAccess(), access, threshold, tcu::COMPARE_LOG_ON_ERROR))
								m_resultCollector.fail("Compare failed for sample " + de::toString(sampleNdx));
						}
						else
						{
							// Allow error of 4 times the minimum presentable difference
							const Vec4 threshold (4.0f * 1.0f / ((UVec4(1u) << tcu::getTextureFormatMantissaBitDepth(format).cast<deUint32>()) - 1u).cast<float>());

							if (!tcu::floatThresholdCompare(m_context.getTestContext().getLog(), name.c_str(), name.c_str(), reference.getAccess(), access, threshold, tcu::COMPARE_LOG_ON_ERROR))
								m_resultCollector.fail("Compare failed for sample " + de::toString(sampleNdx));
						}

						break;
					}

					default:
						DE_FATAL("Unknown channel class");
				}
			}
		}
	}

	return tcu::TestStatus(m_resultCollector.getResult(), m_resultCollector.getMessage());
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

		if (tcu::hasDepthComponent(format.order))
		{
			const Vec4			minValue		(0.0f);
			const Vec4			range			(1.0f);
			std::ostringstream	fragmentShader;

			fragmentShader <<
				"#version 450\n"
				"layout(push_constant) uniform PushConstant {\n"
				"\thighp uint sampleIndex;\n"
				"} pushConstants;\n"
				"void main (void)\n"
				"{\n"
				"\thighp uint sampleIndex = pushConstants.sampleIndex;\n"
				"\tgl_SampleMask[0] = int((~0x0u) << sampleIndex);\n"
				"\thighp float depth;\n"
				"\thighp uint x = sampleIndex ^ uint(gl_FragCoord.x);\n"
				"\thighp uint y = sampleIndex ^ uint(gl_FragCoord.y);\n";

			fragmentShader << "\tdepth = "  << minValue[0] << ";\n";

			{
				deUint32 divider = 2;

				// \note Limited to ten bits since the target is 32x32, so there are 10 input bits
				for (size_t bitNdx = 0; bitNdx < 10; bitNdx++)
				{
					fragmentShader <<
							"\tdepth += " << (range[0] / (float)divider)
							<< " * float(bitfieldExtract(" << (bitNdx % 2 == 0 ? "x" : "y") << ", " << (bitNdx / 2) << ", 1));\n";

					divider *= 2;
				}
			}

			fragmentShader <<
				"\tgl_FragDepth = depth;\n"
				"}\n";

			dst.glslSources.add("quad-frag") << glu::FragmentSource(fragmentShader.str());
		}
		else if (tcu::hasStencilComponent(format.order))
		{
			dst.glslSources.add("quad-frag") << glu::FragmentSource(
				"#version 450\n"
				"layout(push_constant) uniform PushConstant {\n"
				"\thighp uint sampleIndex;\n"
				"} pushConstants;\n"
				"void main (void)\n"
				"{\n"
				"\thighp uint sampleIndex = pushConstants.sampleIndex;\n"
				"\tgl_SampleMask[0] = int((~0x0u) << sampleIndex);\n"
				"}\n");
		}
		else
		{
			switch (channelClass)
			{
				case tcu::TEXTURECHANNELCLASS_UNSIGNED_INTEGER:
				{
					const UVec4	bits		(tcu::getTextureFormatBitDepth(format).cast<deUint32>());
					const UVec4 minValue	(0);
					const UVec4 range		(UVec4(1u) << tcu::min(bits, UVec4(31)));
					std::ostringstream		fragmentShader;

					fragmentShader <<
						"#version 450\n"
						"layout(location = 0) out highp uvec4 o_color;\n"
						"layout(push_constant) uniform PushConstant {\n"
						"\thighp uint sampleIndex;\n"
						"} pushConstants;\n"
						"void main (void)\n"
						"{\n"
						"\thighp uint sampleIndex = pushConstants.sampleIndex;\n"
						"\tgl_SampleMask[0] = int(0x1u << sampleIndex);\n"
						"\thighp uint color[4];\n"
						"\thighp uint x = sampleIndex ^ uint(gl_FragCoord.x);\n"
						"\thighp uint y = sampleIndex ^ uint(gl_FragCoord.y);\n";

					for (int ndx = 0; ndx < 4; ndx++)
						fragmentShader << "\tcolor[" << ndx << "] = "  << minValue[ndx] << ";\n";

					{
						const int		componentCount	= tcu::getNumUsedChannels(format.order);
						const deUint32	bitSize			(bits[0] + bits[1] + bits[2] + bits[3]);
						deUint32		dstBitsUsed[4]	= { 0u, 0u, 0u, 0u };
						deUint32		nextSrcBit		= 0;
						deUint32		divider			= 2;

						// \note Limited to ten bits since the target is 32x32, so there are 10 input bits
						while (nextSrcBit < de::min(bitSize, 10u))
						{
							for (int compNdx = 0; compNdx < componentCount; compNdx++)
							{
								if (dstBitsUsed[compNdx] > bits[compNdx])
									continue;

								fragmentShader <<
										"\tcolor[" << compNdx << "] += " << (range[compNdx] / divider)
										<< " * bitfieldExtract(" << (nextSrcBit % 2 == 0 ? "x" : "y") << ", " << (nextSrcBit / 2) << ", 1);\n";

								nextSrcBit++;
								dstBitsUsed[compNdx]++;
							}

							divider *= 2;
						}
					}

					fragmentShader <<
						"\to_color = uvec4(color[0], color[1], color[2], color[3]);\n"
						"}\n";

					dst.glslSources.add("quad-frag") << glu::FragmentSource(fragmentShader.str());
					break;
				}

				case tcu::TEXTURECHANNELCLASS_SIGNED_INTEGER:
				{
					const UVec4	bits		(tcu::getTextureFormatBitDepth(format).cast<deUint32>());
					const IVec4 minValue	(0);
					const IVec4 range		((UVec4(1u) << tcu::min(bits, UVec4(30))).cast<deInt32>());
					const IVec4 maxV		((UVec4(1u) << (bits - UVec4(1u))).cast<deInt32>());
					const IVec4 clampMax	(maxV - 1);
					const IVec4 clampMin	(-maxV);
					std::ostringstream		fragmentShader;

					fragmentShader <<
						"#version 450\n"
						"layout(location = 0) out highp ivec4 o_color;\n"
						"layout(push_constant) uniform PushConstant {\n"
						"\thighp uint sampleIndex;\n"
						"} pushConstants;\n"
						"void main (void)\n"
						"{\n"
						"\thighp uint sampleIndex = pushConstants.sampleIndex;\n"
						"\tgl_SampleMask[0] = int(0x1u << sampleIndex);\n"
						"\thighp int color[4];\n"
						"\thighp uint x = sampleIndex ^ uint(gl_FragCoord.x);\n"
						"\thighp uint y = sampleIndex ^ uint(gl_FragCoord.y);\n";

					for (int ndx = 0; ndx < 4; ndx++)
						fragmentShader << "\tcolor[" << ndx << "] = "  << minValue[ndx] << ";\n";

					{
						const int		componentCount	= tcu::getNumUsedChannels(format.order);
						const deUint32	bitSize			(bits[0] + bits[1] + bits[2] + bits[3]);
						deUint32		dstBitsUsed[4]	= { 0u, 0u, 0u, 0u };
						deUint32		nextSrcBit		= 0;
						deUint32		divider			= 2;

						// \note Limited to ten bits since the target is 32x32, so there are 10 input bits
						while (nextSrcBit < de::min(bitSize, 10u))
						{
							for (int compNdx = 0; compNdx < componentCount; compNdx++)
							{
								if (dstBitsUsed[compNdx] > bits[compNdx])
									continue;

								fragmentShader <<
										"\tcolor[" << compNdx << "] += " << (range[compNdx] / divider)
										<< " * int(bitfieldExtract(" << (nextSrcBit % 2 == 0 ? "x" : "y") << ", " << (nextSrcBit / 2) << ", 1));\n";

								nextSrcBit++;
								dstBitsUsed[compNdx]++;
							}

							divider *= 2;
						}
					}

					// The spec doesn't define whether signed-integers are clamped on output,
					// so we'll clamp them explicitly to have well-defined outputs.
					fragmentShader <<
						"\to_color = clamp(ivec4(color[0], color[1], color[2], color[3]), " <<
						"ivec4" << clampMin << ", ivec4" << clampMax << ");\n" <<
						"}\n";

					dst.glslSources.add("quad-frag") << glu::FragmentSource(fragmentShader.str());
					break;
				}

				case tcu::TEXTURECHANNELCLASS_UNSIGNED_FIXED_POINT:
				case tcu::TEXTURECHANNELCLASS_SIGNED_FIXED_POINT:
				case tcu::TEXTURECHANNELCLASS_FLOATING_POINT:
				{
					const tcu::TextureFormatInfo	info			(tcu::getTextureFormatInfo(format));
					const UVec4						bits			(tcu::getTextureFormatMantissaBitDepth(format).cast<deUint32>());
					const Vec4						minLimit		(-65536.0);
					const Vec4						maxLimit		(65536.0);
					const Vec4						minValue		(tcu::max(info.valueMin, minLimit));
					const Vec4						range			(tcu::min(info.valueMax, maxLimit) - minValue);
					std::ostringstream				fragmentShader;

					fragmentShader <<
						"#version 450\n"
						"layout(location = 0) out highp vec4 o_color;\n"
						"layout(push_constant) uniform PushConstant {\n"
						"\thighp uint sampleIndex;\n"
						"} pushConstants;\n"
						"void main (void)\n"
						"{\n"
						"\thighp uint sampleIndex = pushConstants.sampleIndex;\n"
						"\tgl_SampleMask[0] = int(0x1u << sampleIndex);\n"
						"\thighp float color[4];\n"
						"\thighp uint x = sampleIndex ^ uint(gl_FragCoord.x);\n"
						"\thighp uint y = sampleIndex ^ uint(gl_FragCoord.y);\n";

					for (int ndx = 0; ndx < 4; ndx++)
						fragmentShader << "\tcolor[" << ndx << "] = "  << minValue[ndx] << ";\n";

					{
						const int		componentCount	= tcu::getNumUsedChannels(format.order);
						const deUint32	bitSize			(bits[0] + bits[1] + bits[2] + bits[3]);
						deUint32		dstBitsUsed[4]	= { 0u, 0u, 0u, 0u };
						deUint32		nextSrcBit		= 0;
						deUint32		divider			= 2;

						// \note Limited to ten bits since the target is 32x32, so there are 10 input bits
						while (nextSrcBit < de::min(bitSize, 10u))
						{
							for (int compNdx = 0; compNdx < componentCount; compNdx++)
							{
								if (dstBitsUsed[compNdx] > bits[compNdx])
									continue;

								fragmentShader <<
										"\tcolor[" << compNdx << "] += " << (range[compNdx] / (float)divider)
										<< " * float(bitfieldExtract(" << (nextSrcBit % 2 == 0 ? "x" : "y") << ", " << (nextSrcBit / 2) << ", 1));\n";

								nextSrcBit++;
								dstBitsUsed[compNdx]++;
							}

							divider *= 2;
						}
					}

					fragmentShader <<
						"\to_color = vec4(color[0], color[1], color[2], color[3]);\n"
						"}\n";

					dst.glslSources.add("quad-frag") << glu::FragmentSource(fragmentShader.str());
					break;
				}

				default:
					DE_FATAL("Unknown channel class");
			}
		}

		if (tcu::hasDepthComponent(format.order) || tcu::hasStencilComponent(format.order))
		{
			std::ostringstream splitShader;

			splitShader <<
				"#version 450\n";

			if (tcu::hasDepthComponent(format.order) && tcu::hasStencilComponent(format.order))
			{
				splitShader << "layout(input_attachment_index = 0, set = 0, binding = 0) uniform highp subpassInputMS i_depth;\n"
							<< "layout(input_attachment_index = 0, set = 0, binding = 1) uniform highp usubpassInputMS i_stencil;\n";
			}
			else if (tcu::hasDepthComponent(format.order))
				splitShader << "layout(input_attachment_index = 0, set = 0, binding = 0) uniform highp subpassInputMS i_depth;\n";
			else if (tcu::hasStencilComponent(format.order))
				splitShader << "layout(input_attachment_index = 0, set = 0, binding = 0) uniform highp usubpassInputMS i_stencil;\n";

			splitShader <<
				"layout(push_constant) uniform PushConstant {\n"
				"\thighp uint splitSubpassIndex;\n"
				"} pushConstants;\n";

			for (deUint32 attachmentNdx = 0; attachmentNdx < de::min((deUint32)MAX_COLOR_ATTACHMENT_COUNT, config.sampleCount); attachmentNdx++)
			{
				if (tcu::hasDepthComponent(format.order) && tcu::hasStencilComponent(format.order))
					splitShader << "layout(location = " << attachmentNdx << ") out highp vec2 o_color" << attachmentNdx << ";\n";
				else
					splitShader << "layout(location = " << attachmentNdx << ") out highp float o_color" << attachmentNdx << ";\n";
			}

			splitShader <<
				"void main (void)\n"
				"{\n";

			for (deUint32 attachmentNdx = 0; attachmentNdx < de::min((deUint32)MAX_COLOR_ATTACHMENT_COUNT, config.sampleCount); attachmentNdx++)
			{
				if (tcu::hasDepthComponent(format.order))
					splitShader << "\thighp float depth" << attachmentNdx << " = subpassLoad(i_depth, int(" << MAX_COLOR_ATTACHMENT_COUNT << " * pushConstants.splitSubpassIndex + " << attachmentNdx << "u)).x;\n";

				if (tcu::hasStencilComponent(format.order))
					splitShader << "\thighp uint stencil" << attachmentNdx << " = subpassLoad(i_stencil, int(" << MAX_COLOR_ATTACHMENT_COUNT << " * pushConstants.splitSubpassIndex + " << attachmentNdx << "u)).x;\n";

				if (tcu::hasDepthComponent(format.order) && tcu::hasStencilComponent(format.order))
					splitShader << "\to_color" << attachmentNdx << " = vec2(depth" << attachmentNdx << ", float(stencil" << attachmentNdx << "));\n";
				else if (tcu::hasDepthComponent(format.order))
					splitShader << "\to_color" << attachmentNdx << " = float(depth" << attachmentNdx << ");\n";
				else if (tcu::hasStencilComponent(format.order))
					splitShader << "\to_color" << attachmentNdx << " = float(stencil" << attachmentNdx << ");\n";
			}

			splitShader <<
				"}\n";

			dst.glslSources.add("quad-split-frag") << glu::FragmentSource(splitShader.str());
		}
		else
		{
			std::string subpassType;
			std::string outputType;

			switch (channelClass)
			{
				case tcu::TEXTURECHANNELCLASS_UNSIGNED_INTEGER:
					subpassType	= "usubpassInputMS";
					outputType	= "uvec4";
					break;

				case tcu::TEXTURECHANNELCLASS_SIGNED_INTEGER:
					subpassType	= "isubpassInputMS";
					outputType	= "ivec4";
					break;

				case tcu::TEXTURECHANNELCLASS_UNSIGNED_FIXED_POINT:
				case tcu::TEXTURECHANNELCLASS_SIGNED_FIXED_POINT:
				case tcu::TEXTURECHANNELCLASS_FLOATING_POINT:
					subpassType	= "subpassInputMS";
					outputType	= "vec4";
					break;

				default:
					DE_FATAL("Unknown channel class");
			}

			std::ostringstream splitShader;
			splitShader <<
				"#version 450\n"
				"layout(input_attachment_index = 0, set = 0, binding = 0) uniform highp " << subpassType << " i_color;\n"
				"layout(push_constant) uniform PushConstant {\n"
				"\thighp uint splitSubpassIndex;\n"
				"} pushConstants;\n";

			for (deUint32 attachmentNdx = 0; attachmentNdx < de::min((deUint32)MAX_COLOR_ATTACHMENT_COUNT, config.sampleCount); attachmentNdx++)
				splitShader << "layout(location = " << attachmentNdx << ") out highp " << outputType << " o_color" << attachmentNdx << ";\n";

			splitShader <<
				"void main (void)\n"
				"{\n";

			for (deUint32 attachmentNdx = 0; attachmentNdx < de::min((deUint32)MAX_COLOR_ATTACHMENT_COUNT, config.sampleCount); attachmentNdx++)
				splitShader << "\to_color" << attachmentNdx << " = subpassLoad(i_color, int(" << MAX_COLOR_ATTACHMENT_COUNT << " * pushConstants.splitSubpassIndex + " << attachmentNdx << "u));\n";

			splitShader <<
				"}\n";

			dst.glslSources.add("quad-split-frag") << glu::FragmentSource(splitShader.str());
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

		VK_FORMAT_D16_UNORM,
		VK_FORMAT_X8_D24_UNORM_PACK32,
		VK_FORMAT_D32_SFLOAT,
		VK_FORMAT_S8_UINT,
		VK_FORMAT_D16_UNORM_S8_UINT,
		VK_FORMAT_D24_UNORM_S8_UINT,
		VK_FORMAT_D32_SFLOAT_S8_UINT
	};
	const deUint32			sampleCounts[] =
	{
		2u, 4u, 8u, 16u, 32u
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

tcu::TestCaseGroup* createRenderPassMultisampleTests (tcu::TestContext& testCtx)
{
	return createTestGroup(testCtx, "multisample", "Multisample render pass tests", initTests);
}

} // vkt
