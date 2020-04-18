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
 * \brief Tests sparse render target.
 *//*--------------------------------------------------------------------*/

#include "vktRenderPassSparseRenderTargetTests.hpp"

#include "vktTestCaseUtil.hpp"
#include "vktTestGroupUtil.hpp"
#include "pipeline/vktPipelineImageUtil.hpp"

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
#include "tcuTextureUtil.hpp"

#include "deUniquePtr.hpp"
#include "deSharedPtr.hpp"

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

Move<VkImage> createSparseImageAndMemory (const DeviceInterface&				vk,
										  VkDevice								device,
										  const VkPhysicalDevice				physicalDevice,
										  const InstanceInterface&				instance,
										  Allocator&							allocator,
										  vector<de::SharedPtr<Allocation> >&	allocations,
										  deUint32								universalQueueFamilyIndex,
										  VkQueue								sparseQueue,
										  deUint32								sparseQueueFamilyIndex,
										  const VkSemaphore&					bindSemaphore,
										  VkFormat								format,
										  deUint32								width,
										  deUint32								height)
{
	deUint32				queueFamilyIndices[]	= {universalQueueFamilyIndex, sparseQueueFamilyIndex};
	const VkSharingMode		sharingMode             = universalQueueFamilyIndex != sparseQueueFamilyIndex ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE;

	const VkExtent3D		imageExtent				=
	{
		width,
		height,
		1u
	};

	const VkImageCreateInfo	imageCreateInfo			=
	{
		VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		DE_NULL,
		VK_IMAGE_CREATE_SPARSE_BINDING_BIT | VK_IMAGE_CREATE_SPARSE_RESIDENCY_BIT,
		VK_IMAGE_TYPE_2D,
		format,
		imageExtent,
		1u,
		1u,
		VK_SAMPLE_COUNT_1_BIT,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
		sharingMode,
		sharingMode == VK_SHARING_MODE_CONCURRENT ? 2u : 1u,
		queueFamilyIndices,
		VK_IMAGE_LAYOUT_UNDEFINED
	};

	Move<VkImage>			destImage			= createImage(vk, device, &imageCreateInfo);

	vkt::pipeline::allocateAndBindSparseImage(vk, device, physicalDevice, instance, imageCreateInfo, bindSemaphore, sparseQueue, allocator, allocations, mapVkFormat(format), *destImage);

	return destImage;
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

Move<VkImageView> createImageView (const DeviceInterface&	vkd,
								   VkDevice					device,
								   VkImage					image,
								   VkFormat					format,
								   VkImageAspectFlags		aspect)
{
	const VkImageSubresourceRange range =
	{
		aspect,
		0u,
		1u,
		0u,
		1u
	};

	return createImageView(vkd, device, 0u, image, VK_IMAGE_VIEW_TYPE_2D, format, makeComponentMappingRGBA(), range);
}

Move<VkBuffer> createBuffer (const DeviceInterface&		vkd,
							 VkDevice					device,
							 VkFormat					format,
							 deUint32					width,
							 deUint32					height)
{
	const VkBufferUsageFlags	bufferUsage			(VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
	const VkDeviceSize			pixelSize			= mapVkFormat(format).getPixelSize();
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

Move<VkRenderPass> createRenderPass (const DeviceInterface&	vkd,
									 VkDevice				device,
									 VkFormat				dstFormat)
{
	const VkAttachmentReference		dstAttachmentRef	=
	{
		0u,
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
	};
	const VkAttachmentDescription	dstAttachment		=
	{
		0u,

		dstFormat,
		VK_SAMPLE_COUNT_1_BIT,

		VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		VK_ATTACHMENT_STORE_OP_STORE,

		VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		VK_ATTACHMENT_STORE_OP_DONT_CARE,

		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
	};
	const VkSubpassDescription		subpasses[]			=
	{
		{
			(VkSubpassDescriptionFlags)0,
			VK_PIPELINE_BIND_POINT_GRAPHICS,

			0u,
			DE_NULL,

			1u,
			&dstAttachmentRef,
			DE_NULL,

			DE_NULL,
			0u,
			DE_NULL
		}
	};
	const VkRenderPassCreateInfo	createInfo			=
	{
		VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
		DE_NULL,
		(VkRenderPassCreateFlags)0u,

		1u,
		&dstAttachment,

		1u,
		subpasses,

		0u,
		DE_NULL
	};

	return createRenderPass(vkd, device, &createInfo);
}

Move<VkFramebuffer> createFramebuffer (const DeviceInterface&	vkd,
									   VkDevice					device,
									   VkRenderPass				renderPass,
									   VkImageView				dstImageView,
									   deUint32					width,
									   deUint32					height)
{
	const VkFramebufferCreateInfo createInfo =
	{
		VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
		DE_NULL,
		0u,

		renderPass,
		1u,
		&dstImageView,

		width,
		height,
		1u
	};

	return createFramebuffer(vkd, device, &createInfo);
}

Move<VkPipelineLayout> createRenderPipelineLayout (const DeviceInterface&	vkd,
												   VkDevice					device)
{
	const VkPipelineLayoutCreateInfo createInfo =
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
									   deUint32											height)
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

		VK_SAMPLE_COUNT_1_BIT,
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

		2u,
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

class SparseRenderTargetTestInstance : public TestInstance
{
public:
											SparseRenderTargetTestInstance	(Context& context, VkFormat format);
											~SparseRenderTargetTestInstance	(void);

											tcu::TestStatus	iterate	(void);

private:
	const deUint32							m_width;
	const deUint32							m_height;
	const VkFormat							m_format;

	vector<de::SharedPtr<Allocation> >		m_allocations;

	const Unique<VkSemaphore>				m_bindSemaphore;

	const Unique<VkImage>					m_dstImage;
	const Unique<VkImageView>				m_dstImageView;

	const Unique<VkBuffer>					m_dstBuffer;
	const de::UniquePtr<Allocation>			m_dstBufferMemory;

	const Unique<VkRenderPass>				m_renderPass;
	const Unique<VkFramebuffer>				m_framebuffer;

	const Unique<VkPipelineLayout>			m_renderPipelineLayout;
	const Unique<VkPipeline>				m_renderPipeline;

	const Unique<VkCommandPool>				m_commandPool;
	tcu::ResultCollector					m_resultCollector;
};

SparseRenderTargetTestInstance::SparseRenderTargetTestInstance (Context& context, VkFormat format)
	: TestInstance				(context)
	, m_width					(32u)
	, m_height					(32u)
	, m_format					(format)
	, m_bindSemaphore			(createSemaphore(context.getDeviceInterface(), context.getDevice()))
	, m_dstImage				(createSparseImageAndMemory(context.getDeviceInterface(), context.getDevice(), context.getPhysicalDevice(), context.getInstanceInterface(), context.getDefaultAllocator(), m_allocations, context.getUniversalQueueFamilyIndex(), context.getSparseQueue(), context.getSparseQueueFamilyIndex(), *m_bindSemaphore, m_format, m_width, m_height))
	, m_dstImageView			(createImageView(context.getDeviceInterface(), context.getDevice(), *m_dstImage, m_format, VK_IMAGE_ASPECT_COLOR_BIT))
	, m_dstBuffer				(createBuffer(context.getDeviceInterface(), context.getDevice(), m_format, m_width, m_height))
	, m_dstBufferMemory			(createBufferMemory(context.getDeviceInterface(), context.getDevice(), context.getDefaultAllocator(), *m_dstBuffer))
	, m_renderPass				(createRenderPass(context.getDeviceInterface(), context.getDevice(), m_format))
	, m_framebuffer				(createFramebuffer(context.getDeviceInterface(), context.getDevice(), *m_renderPass, *m_dstImageView, m_width, m_height))
	, m_renderPipelineLayout	(createRenderPipelineLayout(context.getDeviceInterface(), context.getDevice()))
	, m_renderPipeline			(createRenderPipeline(context.getDeviceInterface(), context.getDevice(), *m_renderPass, *m_renderPipelineLayout, context.getBinaryCollection(), m_width, m_height))
	, m_commandPool				(createCommandPool(context.getDeviceInterface(), context.getDevice(), VK_COMMAND_POOL_CREATE_TRANSIENT_BIT, context.getUniversalQueueFamilyIndex()))
{
}

SparseRenderTargetTestInstance::~SparseRenderTargetTestInstance (void)
{
}

tcu::TestStatus SparseRenderTargetTestInstance::iterate (void)
{
	const DeviceInterface&			vkd				(m_context.getDeviceInterface());
	const Unique<VkCommandBuffer>	commandBuffer	(allocateCommandBuffer(vkd, m_context.getDevice(), *m_commandPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY));

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
	vkd.cmdEndRenderPass(*commandBuffer);

	// Memory barrier between rendering and copy
	{
		const VkImageMemoryBarrier barrier =
		{
			VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
			DE_NULL,

			VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			VK_ACCESS_TRANSFER_READ_BIT,

			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,

			VK_QUEUE_FAMILY_IGNORED,
			VK_QUEUE_FAMILY_IGNORED,

			*m_dstImage,
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

		vkd.cmdCopyImageToBuffer(*commandBuffer, *m_dstImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, *m_dstBuffer, 1u, &region);
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
		const tcu::TextureFormat			format			(mapVkFormat(m_format));
		const void* const					ptr				(m_dstBufferMemory->getHostPtr());
		const tcu::ConstPixelBufferAccess	access			(format, m_width, m_height, 1, ptr);
		tcu::TextureLevel					reference		(format, m_width, m_height);
		const tcu::TextureChannelClass		channelClass	(tcu::getTextureChannelClass(format.type));

		switch (channelClass)
		{
			case tcu::TEXTURECHANNELCLASS_UNSIGNED_INTEGER:
			{
				const UVec4	bits	(tcu::getTextureFormatBitDepth(format).cast<deUint32>());
				const UVec4	color	(1u << (bits.x()-1), 1u << (bits.y()-2), 1u << (bits.z()-3), 0xffffffff);

				for (deUint32 y = 0; y < m_height; y++)
				for (deUint32 x = 0; x < m_width; x++)
				{
					reference.getAccess().setPixel(color, x, y);
				}

				if (!tcu::intThresholdCompare(m_context.getTestContext().getLog(), "", "", reference.getAccess(), access, UVec4(0u), tcu::COMPARE_LOG_ON_ERROR))
					m_resultCollector.fail("Compare failed.");
			}
			break;

			case tcu::TEXTURECHANNELCLASS_SIGNED_INTEGER:
			{
				const UVec4	bits	(tcu::getTextureFormatBitDepth(format).cast<deUint32>());
				const UVec4	color	(1u << (bits.x()-2), 1u << (bits.y()-3), 1u << (bits.z()-4), 0xffffffff);

				for (deUint32 y = 0; y < m_height; y++)
				for (deUint32 x = 0; x < m_width; x++)
				{
					reference.getAccess().setPixel(color, x, y);
				}

				if (!tcu::intThresholdCompare(m_context.getTestContext().getLog(), "", "", reference.getAccess(), access, UVec4(0u), tcu::COMPARE_LOG_ON_ERROR))
					m_resultCollector.fail("Compare failed.");
			}
			break;

			case tcu::TEXTURECHANNELCLASS_UNSIGNED_FIXED_POINT:
			case tcu::TEXTURECHANNELCLASS_SIGNED_FIXED_POINT:
			{
				const tcu::TextureFormatInfo	info		(tcu::getTextureFormatInfo(format));
				const Vec4						maxValue	(info.valueMax);
				const Vec4						color		(maxValue.x() / 2.0f, maxValue.y() / 4.0f, maxValue.z() / 8.0f, maxValue.w());

				for (deUint32 y = 0; y < m_height; y++)
				for (deUint32 x = 0; x < m_width; x++)
				{
					if (tcu::isSRGB(format))
						reference.getAccess().setPixel(tcu::linearToSRGB(color), x, y);
					else
						reference.getAccess().setPixel(color, x, y);
				}

				{
					// Allow error of 4 times the minimum presentable difference
					const Vec4 threshold (4.0f * 1.0f / ((UVec4(1u) << tcu::getTextureFormatMantissaBitDepth(format).cast<deUint32>()) - 1u).cast<float>());

					if (!tcu::floatThresholdCompare(m_context.getTestContext().getLog(), "", "", reference.getAccess(), access, threshold, tcu::COMPARE_LOG_ON_ERROR))
						m_resultCollector.fail("Compare failed.");
				}
			}
			break;

			case tcu::TEXTURECHANNELCLASS_FLOATING_POINT:
			{
				const Vec4 color(0.5f, 0.25f, 0.125f, 1.0f);

				for (deUint32 y = 0; y < m_height; y++)
				for (deUint32 x = 0; x < m_width; x++)
				{
					if (tcu::isSRGB(format))
						reference.getAccess().setPixel(tcu::linearToSRGB(color), x, y);
					else
						reference.getAccess().setPixel(color, x, y);
				}

				{
					// Convert target format ulps to float ulps and allow 64ulp differences
					const UVec4 threshold (64u * (UVec4(1u) << (UVec4(23) - tcu::getTextureFormatMantissaBitDepth(format).cast<deUint32>())));

					if (!tcu::floatUlpThresholdCompare(m_context.getTestContext().getLog(), "", "", reference.getAccess(), access, threshold, tcu::COMPARE_LOG_ON_ERROR))
						m_resultCollector.fail("Compare failed.");
				}
			}
			break;

			default:
				DE_FATAL("Unknown channel class");
		}
	}

	return tcu::TestStatus(m_resultCollector.getResult(), m_resultCollector.getMessage());
}

struct Programs
{
	void init (vk::SourceCollections& dst, VkFormat format) const
	{
		std::ostringstream				fragmentShader;
		const tcu::TextureFormat		texFormat		(mapVkFormat(format));
		const UVec4						bits			(tcu::getTextureFormatBitDepth(texFormat).cast<deUint32>());
		const tcu::TextureChannelClass	channelClass	(tcu::getTextureChannelClass(texFormat.type));

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

		switch (channelClass)
		{
			case tcu::TEXTURECHANNELCLASS_UNSIGNED_INTEGER:
			{
				fragmentShader <<
					"#version 450\n"
					"layout(location = 0) out highp uvec4 o_color;\n"
					"void main (void)\n"
					"{\n"
					"    o_color = uvec4(" << de::toString(1u << (bits.x()-1)) << ", " << de::toString(1u << (bits.y()-2)) << ", " << de::toString(1u << (bits.z()-3)) << ", 0xffffffff);"
					"}\n";
			}
			break;

			case tcu::TEXTURECHANNELCLASS_SIGNED_INTEGER:
			{
				fragmentShader <<
					"#version 450\n"
					"layout(location = 0) out highp ivec4 o_color;\n"
					"void main (void)\n"
					"{\n"
					"    o_color = ivec4(" << de::toString(1u << (bits.x()-2)) << ", " << de::toString(1u << (bits.y()-3)) << ", " << de::toString(1u << (bits.z()-4)) << ", 0xffffffff);"
					"}\n";
			}
			break;

			default:
			{
				fragmentShader <<
					"#version 450\n"
					"layout(location = 0) out highp vec4 o_color;\n"
					"void main (void)\n"
					"{\n"
					"    o_color = vec4(0.5, 0.25, 0.125, 1.0);\n"
					"}\n";
			}
			break;
		};

		dst.glslSources.add("quad-frag") << glu::FragmentSource(fragmentShader.str());
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
		VK_FORMAT_R32G32B32A32_SFLOAT
	};

	tcu::TestContext&		testCtx		(group->getTestContext());

	for (size_t formatNdx = 0; formatNdx < DE_LENGTH_OF_ARRAY(formats); formatNdx++)
	{
		const VkFormat	format		(formats[formatNdx]);
		string			testName	(formatToName(format));

		group->addChild(new InstanceFactory1<SparseRenderTargetTestInstance, VkFormat, Programs>(testCtx, tcu::NODETYPE_SELF_VALIDATE, testName.c_str(), testName.c_str(), format));
	}
}

} // anonymous

tcu::TestCaseGroup* createRenderPassSparseRenderTargetTests (tcu::TestContext& testCtx)
{
	return createTestGroup(testCtx, "sparserendertarget", "Sparse render target tests", initTests);
}

} // vkt
