/*------------------------------------------------------------------------
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
 * \brief Negative viewport height (part of VK_KHR_maintenance1)
 *//*--------------------------------------------------------------------*/

#include "vktDrawNegativeViewportHeightTests.hpp"
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

enum Constants
{
	WIDTH	= 256,
	HEIGHT	= WIDTH/2,
};

struct TestParams
{
	VkFrontFace				frontFace;
	VkCullModeFlagBits		cullMode;
};

class NegativeViewportHeightTestInstance : public TestInstance
{
public:
									NegativeViewportHeightTestInstance	(Context& context, const TestParams& params);
	tcu::TestStatus					iterate								(void);
	tcu::ConstPixelBufferAccess		draw								(const VkViewport viewport);
	MovePtr<tcu::TextureLevel>		generateReferenceImage				(void) const;
	bool							isCulled							(const VkFrontFace triangleFace) const;

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

NegativeViewportHeightTestInstance::NegativeViewportHeightTestInstance (Context& context, const TestParams& params)
	: TestInstance				(context)
	, m_params					(params)
	, m_colorAttachmentFormat	(VK_FORMAT_R8G8B8A8_UNORM)
{
	const DeviceInterface&	vk		= m_context.getDeviceInterface();
	const VkDevice			device	= m_context.getDevice();

	// Vertex data
	{
		std::vector<Vec4> vertexData;

		// CCW triangle
		vertexData.push_back(Vec4(-0.8f, -0.6f, 0.0f, 1.0f));	//  0-----2
		vertexData.push_back(Vec4(-0.8f,  0.6f, 0.0f, 1.0f));	//   |  /
		vertexData.push_back(Vec4(-0.2f, -0.6f, 0.0f, 1.0f));	//  1|/

		// CW triangle
		vertexData.push_back(Vec4( 0.2f, -0.6f, 0.0f, 1.0f));	//  0-----1
		vertexData.push_back(Vec4( 0.8f, -0.6f, 0.0f, 1.0f));	//    \  |
		vertexData.push_back(Vec4( 0.8f,  0.6f, 0.0f, 1.0f));	//      \|2

		const VkDeviceSize dataSize = vertexData.size() * sizeof(Vec4);
		m_vertexBuffer = Buffer::createAndAlloc(vk, device, BufferCreateInfo(dataSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT),
												m_context.getDefaultAllocator(), MemoryRequirement::HostVisible);

		deMemcpy(m_vertexBuffer->getBoundMemory().getHostPtr(), &vertexData[0], static_cast<std::size_t>(dataSize));
		flushMappedMemoryRange(vk, device, m_vertexBuffer->getBoundMemory().getMemory(), m_vertexBuffer->getBoundMemory().getOffset(), VK_WHOLE_SIZE);
	}

	// Render pass
	{
		const VkExtent3D		targetImageExtent		= { WIDTH, HEIGHT, 1 };
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

		const FramebufferCreateInfo	framebufferCreateInfo(*m_renderPass, colorAttachments, WIDTH, HEIGHT, 1);
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
		{ 0,		0		},		// x, y
		{ WIDTH,	HEIGHT	},		// width, height
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
		VK_FALSE,					// depthClampEnable
		VK_FALSE,					// rasterizerDiscardEnable
		VK_POLYGON_MODE_FILL,		// polygonMode
		m_params.cullMode,			// cullMode
		m_params.frontFace,			// frontFace
		VK_FALSE,					// depthBiasEnable
		0.0f,						// depthBiasConstantFactor
		0.0f,						// depthBiasClamp
		0.0f,						// depthBiasSlopeFactor
		1.0f));						// lineWidth
	pipelineCreateInfo.addState (PipelineCreateInfo::MultiSampleState	());
	pipelineCreateInfo.addState (PipelineCreateInfo::DynamicState		(dynamicStates));

	m_pipeline = createGraphicsPipeline(vk, device, DE_NULL, &pipelineCreateInfo);
}

tcu::ConstPixelBufferAccess NegativeViewportHeightTestInstance::draw (const VkViewport viewport)
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

		initialTransitionColor2DImage(vk, *cmdBuffer, m_colorTargetImage->object(), VK_IMAGE_LAYOUT_GENERAL,
									  VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
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
		const VkRect2D				renderArea		= { { 0, 0 }, { WIDTH, HEIGHT } };
		const RenderPassBeginInfo	renderPassBegin	(*m_renderPass, *m_framebuffer, renderArea);

		vk.cmdBeginRenderPass(*cmdBuffer, &renderPassBegin, VK_SUBPASS_CONTENTS_INLINE);
	}
	{
		const VkDeviceSize	offset	= 0;
		const VkBuffer		buffer	= m_vertexBuffer->object();

		vk.cmdBindVertexBuffers(*cmdBuffer, 0, 1, &buffer, &offset);
	}

	vk.cmdBindPipeline(*cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *m_pipeline);
	vk.cmdDraw(*cmdBuffer, 6, 1, 0, 0);
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
		return m_colorTargetImage->readSurface(queue, m_context.getDefaultAllocator(), VK_IMAGE_LAYOUT_GENERAL, zeroOffset, WIDTH, HEIGHT, VK_IMAGE_ASPECT_COLOR_BIT);
	}
}

//! Determine if a triangle with triangleFace orientation will be culled or not
bool NegativeViewportHeightTestInstance::isCulled (const VkFrontFace triangleFace) const
{
	const bool isFrontFacing = (triangleFace == m_params.frontFace);

	if (m_params.cullMode == VK_CULL_MODE_FRONT_BIT && isFrontFacing)
		return true;
	if (m_params.cullMode == VK_CULL_MODE_BACK_BIT  && !isFrontFacing)
		return true;

	return m_params.cullMode == VK_CULL_MODE_FRONT_AND_BACK;
}

MovePtr<tcu::TextureLevel> NegativeViewportHeightTestInstance::generateReferenceImage (void) const
{
	DE_ASSERT(HEIGHT == WIDTH/2);

	MovePtr<tcu::TextureLevel>		image	(new tcu::TextureLevel(mapVkFormat(m_colorAttachmentFormat), WIDTH, HEIGHT));
	const tcu::PixelBufferAccess	access	(image->getAccess());
	const Vec4						black	(0.0f, 0.0f, 0.0f, 1.0f);
	const Vec4						white	(1.0f);
	const Vec4						gray	(0.5f, 0.5f, 0.5f, 1.0f);

	tcu::clear(access, black);

	const int p1 =      static_cast<int>(static_cast<float>(HEIGHT) * (1.0f - 0.6f) / 2.0f);
	const int p2 = p1 + static_cast<int>(static_cast<float>(HEIGHT) * (2.0f * 0.6f) / 2.0f);

	// left triangle (CCW -> CW after y-flip)
	if (!isCulled(VK_FRONT_FACE_CLOCKWISE))
	{
		const Vec4& color = (m_params.frontFace == VK_FRONT_FACE_CLOCKWISE ? white : gray);

		for (int y = p1; y <= p2; ++y)
		for (int x = p1; x <  y;  ++x)
			access.setPixel(color, x, y);
	}

	// right triangle (CW -> CCW after y-flip)
	if (!isCulled(VK_FRONT_FACE_COUNTER_CLOCKWISE))
	{
		const Vec4& color = (m_params.frontFace == VK_FRONT_FACE_COUNTER_CLOCKWISE ? white : gray);

		for (int y = p1;        y <= p2;          ++y)
		for (int x = WIDTH - y; x <  p2 + HEIGHT; ++x)
			access.setPixel(color, x, y);
	}

	return image;
}

std::string getCullModeStr (const VkCullModeFlagBits cullMode)
{
	// Cull mode flags are a bit special, because there's a meaning to 0 and or'ed flags.
	// The function getCullModeFlagsStr() doesn't work too well in this case.

	switch (cullMode)
	{
		case VK_CULL_MODE_NONE:				return "VK_CULL_MODE_NONE";
		case VK_CULL_MODE_FRONT_BIT:		return "VK_CULL_MODE_FRONT_BIT";
		case VK_CULL_MODE_BACK_BIT:			return "VK_CULL_MODE_BACK_BIT";
		case VK_CULL_MODE_FRONT_AND_BACK:	return "VK_CULL_MODE_FRONT_AND_BACK";

		default:
			DE_ASSERT(0);
			return std::string();
	}
}

tcu::TestStatus NegativeViewportHeightTestInstance::iterate (void)
{
	// Check requirements

	if (!de::contains(m_context.getDeviceExtensions().begin(), m_context.getDeviceExtensions().end(), std::string("VK_KHR_maintenance1")))
		TCU_THROW(NotSupportedError, "Missing extension: VK_KHR_maintenance1");

	// Set up the viewport and draw

	const VkViewport viewport =
	{
		0.0f,							// float    x;
		static_cast<float>(HEIGHT),		// float    y;
		static_cast<float>(WIDTH),		// float    width;
		-static_cast<float>(HEIGHT),	// float    height;
		0.0f,							// float    minDepth;
		1.0f,							// float    maxDepth;
	};

	const tcu::ConstPixelBufferAccess	resultImage	= draw(viewport);

	// Verify the results

	tcu::TestLog&				log				= m_context.getTestContext().getLog();
	MovePtr<tcu::TextureLevel>	referenceImage	= generateReferenceImage();

	log << tcu::TestLog::Message
		<< "Drawing two triangles with negative viewport height, which will cause a y-flip. This changes the sign of the triangle's area."
		<< tcu::TestLog::EndMessage;
	log << tcu::TestLog::Message
		<< "After the flip, the triangle on the left is CW and the triangle on the right is CCW. Right angles of the both triangles should be at the bottom of the image."
		<< " Front face is white, back face is gray."
		<< tcu::TestLog::EndMessage;
	log << tcu::TestLog::Message
		<< "Front face: " << getFrontFaceName(m_params.frontFace) << "\n"
		<< "Cull mode: "  << getCullModeStr  (m_params.cullMode)  << "\n"
		<< tcu::TestLog::EndMessage;

	if (!tcu::fuzzyCompare(log, "Image compare", "Image compare", referenceImage->getAccess(), resultImage, 0.02f, tcu::COMPARE_LOG_RESULT))
		return tcu::TestStatus::fail("Rendered image is incorrect");
	else
		return tcu::TestStatus::pass("Pass");
}

class NegativeViewportHeightTest : public TestCase
{
public:
	NegativeViewportHeightTest (tcu::TestContext& testCtx, const std::string& name, const std::string& description, const TestParams& params)
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
				<< "    if (gl_FrontFacing)\n"
				<< "        out_color = vec4(1.0);\n"
				<< "    else\n"
				<< "        out_color = vec4(vec3(0.5), 1.0);\n"
				<< "}\n";

			programCollection.glslSources.add("frag") << glu::FragmentSource(src.str());
		}
	}

	virtual TestInstance* createInstance (Context& context) const
	{
		return new NegativeViewportHeightTestInstance(context, m_params);
	}

private:
	const TestParams	m_params;
};

void populateTestGroup (tcu::TestCaseGroup* testGroup)
{
	const struct
	{
		const char* const	name;
		VkFrontFace			frontFace;
	} frontFace[] =
	{
		{ "front_ccw",	VK_FRONT_FACE_COUNTER_CLOCKWISE	},
		{ "front_cw",	VK_FRONT_FACE_CLOCKWISE			},
	};

	const struct
	{
		const char* const	name;
		VkCullModeFlagBits	cullMode;
	} cullMode[] =
	{
		{ "cull_none",	VK_CULL_MODE_NONE			},
		{ "cull_front",	VK_CULL_MODE_FRONT_BIT		},
		{ "cull_back",	VK_CULL_MODE_BACK_BIT		},
		{ "cull_both",	VK_CULL_MODE_FRONT_AND_BACK	},
	};

	for (int ndxFrontFace = 0; ndxFrontFace < DE_LENGTH_OF_ARRAY(frontFace); ++ndxFrontFace)
	for (int ndxCullMode  = 0; ndxCullMode  < DE_LENGTH_OF_ARRAY(cullMode);  ++ndxCullMode)
	{
		const TestParams params =
		{
			frontFace[ndxFrontFace].frontFace,
			cullMode[ndxCullMode].cullMode,
		};
		std::ostringstream	name;
		name << frontFace[ndxFrontFace].name << "_" << cullMode[ndxCullMode].name;

		testGroup->addChild(new NegativeViewportHeightTest(testGroup->getTestContext(), name.str(), "", params));
	}
}

}	// anonymous

tcu::TestCaseGroup*	createNegativeViewportHeightTests (tcu::TestContext& testCtx)
{
	return createTestGroup(testCtx, "negative_viewport_height", "Negative viewport height (VK_KHR_maintenance1)", populateTestGroup);
}

}	// Draw
}	// vkt
