/*------------------------------------------------------------------------
 * Vulkan Conformance Tests
 * ------------------------
 *
 * Copyright (c) 2016 The Khronos Group Inc.
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
 * \brief Inverted depth ranges tests.
 *//*--------------------------------------------------------------------*/

#include "vktDrawInvertedDepthRangesTests.hpp"
#include "vktDrawCreateInfoUtil.hpp"
#include "vktDrawImageObjectUtil.hpp"
#include "vktDrawBufferObjectUtil.hpp"
#include "vktTestGroupUtil.hpp"
#include "vktTestCaseUtil.hpp"

#include "vkPrograms.hpp"
#include "vkTypeUtil.hpp"
#include "vkImageUtil.hpp"

#include "tcuVector.hpp"
#include "tcuTextureUtil.hpp"
#include "tcuImageCompare.hpp"
#include "tcuTestLog.hpp"

#include "deSharedPtr.hpp"

namespace vkt
{
namespace Draw
{
namespace
{
using namespace vk;
using tcu::Vec4;
using de::SharedPtr;
using de::MovePtr;

struct TestParams
{
	VkBool32	depthClampEnable;
	float		minDepth;
	float		maxDepth;
};

class InvertedDepthRangesTestInstance : public TestInstance
{
public:
									InvertedDepthRangesTestInstance	(Context& context, const TestParams& params);
	tcu::TestStatus					iterate							(void);
	tcu::ConstPixelBufferAccess		draw							(const VkViewport viewport);
	MovePtr<tcu::TextureLevel>		generateReferenceImage			(void) const;

private:
	const TestParams				m_params;
	const VkFormat					m_colorAttachmentFormat;
	SharedPtr<Image>				m_colorTargetImage;
	Move<VkImageView>				m_colorTargetView;
	SharedPtr<Buffer>				m_vertexBuffer;
	Move<VkRenderPass>				m_renderPass;
	Move<VkFramebuffer>				m_framebuffer;
	Move<VkPipelineLayout>			m_pipelineLayout;
	Move<VkPipeline>				m_pipeline;
};

InvertedDepthRangesTestInstance::InvertedDepthRangesTestInstance (Context& context, const TestParams& params)
	: TestInstance				(context)
	, m_params					(params)
	, m_colorAttachmentFormat	(VK_FORMAT_R8G8B8A8_UNORM)
{
	const DeviceInterface&	vk		= m_context.getDeviceInterface();
	const VkDevice			device	= m_context.getDevice();

	if (m_params.depthClampEnable && !m_context.getDeviceFeatures().depthClamp)
		TCU_THROW(NotSupportedError, "DepthClamp device feature not supported.");

	if (params.minDepth > 1.0f	||
		params.minDepth < 0.0f	||
		params.maxDepth > 1.0f	||
		params.maxDepth < 0.0f)
	{
		if (!de::contains(context.getDeviceExtensions().begin(), context.getDeviceExtensions().end(), "VK_EXT_depth_range_unrestricted"))
			throw tcu::NotSupportedError("Test variant with minDepth/maxDepth outside 0..1 requires the VK_EXT_depth_range_unrestricted extension");
	}

	// Vertex data
	{
		std::vector<Vec4> vertexData;

		vertexData.push_back(Vec4(-0.8f, -0.8f, -0.2f, 1.0f));	//  0-----2
		vertexData.push_back(Vec4(-0.8f,  0.8f,  0.0f, 1.0f));	//   |  /
		vertexData.push_back(Vec4( 0.8f, -0.8f,  1.2f, 1.0f));	//  1|/

		const VkDeviceSize dataSize = vertexData.size() * sizeof(Vec4);
		m_vertexBuffer = Buffer::createAndAlloc(vk, device, BufferCreateInfo(dataSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT),
												m_context.getDefaultAllocator(), MemoryRequirement::HostVisible);

		deMemcpy(m_vertexBuffer->getBoundMemory().getHostPtr(), &vertexData[0], static_cast<std::size_t>(dataSize));
		flushMappedMemoryRange(vk, device, m_vertexBuffer->getBoundMemory().getMemory(), m_vertexBuffer->getBoundMemory().getOffset(), VK_WHOLE_SIZE);
	}

	// Render pass
	{
		const VkExtent3D		targetImageExtent		= { 256, 256, 1 };
		const VkImageUsageFlags	targetImageUsageFlags	= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

		const ImageCreateInfo	targetImageCreateInfo(
			VK_IMAGE_TYPE_2D,						// imageType,
			m_colorAttachmentFormat,				// format,
			targetImageExtent,						// extent,
			1u,										// mipLevels,
			1u,										// arrayLayers,
			VK_SAMPLE_COUNT_1_BIT,					// samples,
			VK_IMAGE_TILING_OPTIMAL,				// tiling,
			targetImageUsageFlags);					// usage,

		m_colorTargetImage = Image::createAndAlloc(vk, device, targetImageCreateInfo, m_context.getDefaultAllocator(), m_context.getUniversalQueueFamilyIndex());

		RenderPassCreateInfo	renderPassCreateInfo;
		renderPassCreateInfo.addAttachment(AttachmentDescription(
			m_colorAttachmentFormat,				// format
			VK_SAMPLE_COUNT_1_BIT,					// samples
			VK_ATTACHMENT_LOAD_OP_LOAD,				// loadOp
			VK_ATTACHMENT_STORE_OP_STORE,			// storeOp
			VK_ATTACHMENT_LOAD_OP_DONT_CARE,		// stencilLoadOp
			VK_ATTACHMENT_STORE_OP_DONT_CARE,		// stencilStoreOp
			VK_IMAGE_LAYOUT_GENERAL,				// initialLayout
			VK_IMAGE_LAYOUT_GENERAL));				// finalLayout

		const VkAttachmentReference colorAttachmentReference =
		{
			0u,
			VK_IMAGE_LAYOUT_GENERAL
		};

		renderPassCreateInfo.addSubpass(SubpassDescription(
			VK_PIPELINE_BIND_POINT_GRAPHICS,		// pipelineBindPoint
			(VkSubpassDescriptionFlags)0,			// flags
			0u,										// inputAttachmentCount
			DE_NULL,								// inputAttachments
			1u,										// colorAttachmentCount
			&colorAttachmentReference,				// colorAttachments
			DE_NULL,								// resolveAttachments
			AttachmentReference(),					// depthStencilAttachment
			0u,										// preserveAttachmentCount
			DE_NULL));								// preserveAttachments

		m_renderPass = createRenderPass(vk, device, &renderPassCreateInfo);
	}

	// Framebuffer
	{
		const ImageViewCreateInfo colorTargetViewInfo (m_colorTargetImage->object(), VK_IMAGE_VIEW_TYPE_2D, m_colorAttachmentFormat);
		m_colorTargetView = createImageView(vk, device, &colorTargetViewInfo);

		std::vector<VkImageView> colorAttachments(1);
		colorAttachments[0] = *m_colorTargetView;

		const FramebufferCreateInfo	framebufferCreateInfo(*m_renderPass, colorAttachments, 256, 256, 1);
		m_framebuffer = createFramebuffer(vk, device, &framebufferCreateInfo);
	}

	// Vertex input

	const VkVertexInputBindingDescription		vertexInputBindingDescription =
	{
		0u,										// uint32_t             binding;
		sizeof(Vec4),							// uint32_t             stride;
		VK_VERTEX_INPUT_RATE_VERTEX,			// VkVertexInputRate    inputRate;
	};

	const VkVertexInputAttributeDescription		vertexInputAttributeDescription =
	{
		0u,										// uint32_t    location;
		0u,										// uint32_t    binding;
		VK_FORMAT_R32G32B32A32_SFLOAT,			// VkFormat    format;
		0u										// uint32_t    offset;
	};

	const PipelineCreateInfo::VertexInputState	vertexInputState = PipelineCreateInfo::VertexInputState(1, &vertexInputBindingDescription,
																										1, &vertexInputAttributeDescription);

	// Graphics pipeline

	const VkRect2D scissor =
	{
		{ 0,	0	},	// x, y
		{ 256,	256	},	// width, height
	};

	std::vector<VkDynamicState>		dynamicStates;
	dynamicStates.push_back(VK_DYNAMIC_STATE_VIEWPORT);

	const Unique<VkShaderModule>	vertexModule	(createShaderModule(vk, device, m_context.getBinaryCollection().get("vert"), 0));
	const Unique<VkShaderModule>	fragmentModule	(createShaderModule(vk, device, m_context.getBinaryCollection().get("frag"), 0));

	const PipelineLayoutCreateInfo	pipelineLayoutCreateInfo;
	m_pipelineLayout = createPipelineLayout(vk, device, &pipelineLayoutCreateInfo);

	const PipelineCreateInfo::ColorBlendState::Attachment colorBlendAttachmentState;

	PipelineCreateInfo pipelineCreateInfo(*m_pipelineLayout, *m_renderPass, 0, (VkPipelineCreateFlags)0);
	pipelineCreateInfo.addShader(PipelineCreateInfo::PipelineShaderStage(*vertexModule,   "main", VK_SHADER_STAGE_VERTEX_BIT));
	pipelineCreateInfo.addShader(PipelineCreateInfo::PipelineShaderStage(*fragmentModule, "main", VK_SHADER_STAGE_FRAGMENT_BIT));
	pipelineCreateInfo.addState (PipelineCreateInfo::VertexInputState	(vertexInputState));
	pipelineCreateInfo.addState (PipelineCreateInfo::InputAssemblerState(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST));
	pipelineCreateInfo.addState (PipelineCreateInfo::ColorBlendState	(1, &colorBlendAttachmentState));
	pipelineCreateInfo.addState (PipelineCreateInfo::ViewportState		(1, std::vector<VkViewport>(), std::vector<VkRect2D>(1, scissor)));
	pipelineCreateInfo.addState (PipelineCreateInfo::DepthStencilState	());
	pipelineCreateInfo.addState (PipelineCreateInfo::RasterizerState	(
		m_params.depthClampEnable,	// depthClampEnable
		VK_FALSE,					// rasterizerDiscardEnable
		VK_POLYGON_MODE_FILL,		// polygonMode
		VK_CULL_MODE_NONE,			// cullMode
		VK_FRONT_FACE_CLOCKWISE,	// frontFace
		VK_FALSE,					// depthBiasEnable
		0.0f,						// depthBiasConstantFactor
		0.0f,						// depthBiasClamp
		0.0f,						// depthBiasSlopeFactor
		1.0f));						// lineWidth
	pipelineCreateInfo.addState (PipelineCreateInfo::MultiSampleState	());
	pipelineCreateInfo.addState (PipelineCreateInfo::DynamicState		(dynamicStates));

	m_pipeline = createGraphicsPipeline(vk, device, DE_NULL, &pipelineCreateInfo);
}

tcu::ConstPixelBufferAccess InvertedDepthRangesTestInstance::draw (const VkViewport viewport)
{
	const DeviceInterface&	vk					= m_context.getDeviceInterface();
	const VkDevice			device				= m_context.getDevice();
	const VkQueue			queue				= m_context.getUniversalQueue();
	const deUint32			queueFamilyIndex	= m_context.getUniversalQueueFamilyIndex();

	// Command buffer

	const CmdPoolCreateInfo			cmdPoolCreateInfo	(queueFamilyIndex);
	const Unique<VkCommandPool>		cmdPool				(createCommandPool(vk, device, &cmdPoolCreateInfo));
	const Unique<VkCommandBuffer>	cmdBuffer			(allocateCommandBuffer(vk, device, *cmdPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY));

	// Draw

	{
		const CmdBufferBeginInfo beginInfo;
		vk.beginCommandBuffer(*cmdBuffer, &beginInfo);
	}

	vk.cmdSetViewport(*cmdBuffer, 0u, 1u, &viewport);

	{
		const VkClearColorValue		clearColor			= makeClearValueColorF32(0.0f, 0.0f, 0.0f, 1.0f).color;
		const ImageSubresourceRange subresourceRange	(VK_IMAGE_ASPECT_COLOR_BIT);

		initialTransitionColor2DImage(vk, *cmdBuffer, m_colorTargetImage->object(), VK_IMAGE_LAYOUT_GENERAL, VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
		vk.cmdClearColorImage(*cmdBuffer, m_colorTargetImage->object(), VK_IMAGE_LAYOUT_GENERAL, &clearColor, 1, &subresourceRange);
	}
	{
		const VkMemoryBarrier memBarrier =
		{
			VK_STRUCTURE_TYPE_MEMORY_BARRIER,												// VkStructureType    sType;
			DE_NULL,																		// const void*        pNext;
			VK_ACCESS_TRANSFER_WRITE_BIT,													// VkAccessFlags      srcAccessMask;
			VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT		// VkAccessFlags      dstAccessMask;
		};

		vk.cmdPipelineBarrier(*cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 1, &memBarrier, 0, DE_NULL, 0, DE_NULL);
	}
	{
		const VkRect2D				renderArea		= { { 0, 0 }, { 256, 256 } };
		const RenderPassBeginInfo	renderPassBegin	(*m_renderPass, *m_framebuffer, renderArea);

		vk.cmdBeginRenderPass(*cmdBuffer, &renderPassBegin, VK_SUBPASS_CONTENTS_INLINE);
	}
	{
		const VkDeviceSize	offset	= 0;
		const VkBuffer		buffer	= m_vertexBuffer->object();

		vk.cmdBindVertexBuffers(*cmdBuffer, 0, 1, &buffer, &offset);
	}

	vk.cmdBindPipeline(*cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *m_pipeline);
	vk.cmdDraw(*cmdBuffer, 3, 1, 0, 0);
	vk.cmdEndRenderPass(*cmdBuffer);
	vk.endCommandBuffer(*cmdBuffer);

	// Submit
	{
		const Unique<VkFence>	fence		(createFence(vk, device));
		const VkSubmitInfo		submitInfo	=
		{
			VK_STRUCTURE_TYPE_SUBMIT_INFO,				// VkStructureType                sType;
			DE_NULL,									// const void*                    pNext;
			0,											// uint32_t                       waitSemaphoreCount;
			DE_NULL,									// const VkSemaphore*             pWaitSemaphores;
			(const VkPipelineStageFlags*)DE_NULL,		// const VkPipelineStageFlags*    pWaitDstStageMask;
			1,											// uint32_t                       commandBufferCount;
			&cmdBuffer.get(),							// const VkCommandBuffer*         pCommandBuffers;
			0,											// uint32_t                       signalSemaphoreCount;
			DE_NULL										// const VkSemaphore*             pSignalSemaphores;
		};

		VK_CHECK(vk.queueSubmit(queue, 1, &submitInfo, *fence));
		VK_CHECK(vk.waitForFences(device, 1u, &fence.get(), VK_TRUE, ~0ull));
	}

	// Get result
	{
		const VkOffset3D zeroOffset = { 0, 0, 0 };
		return m_colorTargetImage->readSurface(queue, m_context.getDefaultAllocator(), VK_IMAGE_LAYOUT_GENERAL, zeroOffset, 256, 256, VK_IMAGE_ASPECT_COLOR_BIT);
	}
}

MovePtr<tcu::TextureLevel> InvertedDepthRangesTestInstance::generateReferenceImage (void) const
{
	MovePtr<tcu::TextureLevel>		image			(new tcu::TextureLevel(mapVkFormat(m_colorAttachmentFormat), 256, 256));
	const tcu::PixelBufferAccess	access			(image->getAccess());
	const Vec4						black			(0.0f, 0.0f, 0.0f, 1.0f);
	const int						p1				= static_cast<int>(256.0f * 0.2f / 2.0f);
	const int						p2				= static_cast<int>(256.0f * 1.8f / 2.0f);
	const float						delta			= 256.0f * 1.6f / 2.0f;
	const float						depthValues[]	= { -0.2f, 0.0f, 1.2f };

	tcu::clear(access, black);

	for (int y = p1; y <= p2; ++y)
		for (int x = p1; x <  256 - y;  ++x)
		{
			const float	a = static_cast<float>(p2 - x + p1 - y) / delta;
			const float	b = static_cast<float>(y - p1) / delta;
			const float	c = 1.0f - a - b;
			const float	depth = a * depthValues[0] + b * depthValues[1] + c * depthValues[2];
			const float	depthClamped = de::clamp(depth, 0.0f, 1.0f);
			const float	depthFinal = depthClamped * m_params.maxDepth + (1.0f - depthClamped) * m_params.minDepth;

			if (m_params.depthClampEnable || (depth >= 0.0f && depth <= 1.0f))
				access.setPixel(Vec4(depthFinal, 0.5f, 0.5f, 1.0f), x, y);
		}

	return image;
}

tcu::TestStatus InvertedDepthRangesTestInstance::iterate (void)
{
	// Set up the viewport and draw

	const VkViewport viewport =
	{
		0.0f,				// float    x;
		0.0f,				// float    y;
		256.0f,				// float    width;
		256.0f,				// float    height;
		m_params.minDepth,	// float    minDepth;
		m_params.maxDepth,	// float    maxDepth;
	};

	const tcu::ConstPixelBufferAccess	resultImage	= draw(viewport);

	// Verify the results

	tcu::TestLog&				log				= m_context.getTestContext().getLog();
	MovePtr<tcu::TextureLevel>	referenceImage	= generateReferenceImage();

	if (!tcu::fuzzyCompare(log, "Image compare", "Image compare", referenceImage->getAccess(), resultImage, 0.02f, tcu::COMPARE_LOG_RESULT))
		return tcu::TestStatus::fail("Rendered image is incorrect");
	else
		return tcu::TestStatus::pass("Pass");
}

class InvertedDepthRangesTest : public TestCase
{
public:
	InvertedDepthRangesTest (tcu::TestContext& testCtx, const std::string& name, const std::string& description, const TestParams& params)
		: TestCase	(testCtx, name, description)
		, m_params	(params)
	{
	}

	void initPrograms (SourceCollections& programCollection) const
	{
		// Vertex shader
		{
			std::ostringstream src;
			src << glu::getGLSLVersionDeclaration(glu::GLSL_VERSION_450) << "\n"
				<< "\n"
				<< "layout(location = 0) in vec4 in_position;\n"
				<< "\n"
				<< "out gl_PerVertex {\n"
				<< "    vec4  gl_Position;\n"
				<< "};\n"
				<< "\n"
				<< "void main(void)\n"
				<< "{\n"
				<< "    gl_Position = in_position;\n"
				<< "}\n";

			programCollection.glslSources.add("vert") << glu::VertexSource(src.str());
		}

		// Fragment shader
		{
			std::ostringstream src;
			src << glu::getGLSLVersionDeclaration(glu::GLSL_VERSION_450) << "\n"
				<< "\n"
				<< "layout(location = 0) out vec4 out_color;\n"
				<< "\n"
				<< "void main(void)\n"
				<< "{\n"
				<< "    out_color = vec4(gl_FragCoord.z, 0.5, 0.5, 1.0);\n"
				<< "}\n";

			programCollection.glslSources.add("frag") << glu::FragmentSource(src.str());
		}
	}

	virtual TestInstance* createInstance (Context& context) const
	{
		return new InvertedDepthRangesTestInstance(context, m_params);
	}

private:
	const TestParams	m_params;
};

void populateTestGroup (tcu::TestCaseGroup* testGroup)
{
	const struct
	{
		const char* const	name;
		VkBool32			depthClamp;
	} depthClamp[] =
	{
		{ "depthclamp",		VK_TRUE		},
		{ "nodepthclamp",	VK_FALSE	},
	};

	const struct
	{
		const char* const	name;
		float				delta;
	} delta[] =
	{
		{ "deltazero",					0.0f	},
		{ "deltasmall",					0.3f	},
		{ "deltaone",					1.0f	},

		// Range > 1.0 requires VK_EXT_depth_range_unrestricted extension
		{ "depth_range_unrestricted",	2.7f	},
	};

	for (int ndxDepthClamp = 0; ndxDepthClamp < DE_LENGTH_OF_ARRAY(depthClamp); ++ndxDepthClamp)
	for (int ndxDelta = 0; ndxDelta < DE_LENGTH_OF_ARRAY(delta); ++ndxDelta)
	{
		const float minDepth = 0.5f + delta[ndxDelta].delta / 2.0f;
		const float maxDepth = minDepth - delta[ndxDelta].delta;
		DE_ASSERT(minDepth >= maxDepth);

		const TestParams params =
		{
			depthClamp[ndxDepthClamp].depthClamp,
			minDepth,
			maxDepth
		};
		std::ostringstream	name;
		name << depthClamp[ndxDepthClamp].name << "_" << delta[ndxDelta].name;

		testGroup->addChild(new InvertedDepthRangesTest(testGroup->getTestContext(), name.str(), "", params));
	}
}

}	// anonymous

tcu::TestCaseGroup*	createInvertedDepthRangesTests (tcu::TestContext& testCtx)
{
	return createTestGroup(testCtx, "inverted_depth_ranges", "Inverted depth ranges", populateTestGroup);
}

}	// Draw
}	// vkt
