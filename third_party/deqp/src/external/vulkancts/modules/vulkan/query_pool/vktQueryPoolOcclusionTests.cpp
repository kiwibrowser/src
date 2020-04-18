/*------------------------------------------------------------------------
 * Vulkan Conformance Tests
 * ------------------------
 *
 * Copyright (c) 2015 The Khronos Group Inc.
 * Copyright (c) 2015 Intel Corporation
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
 * \brief Vulkan Occlusion Query Tests
 *//*--------------------------------------------------------------------*/

#include "vktQueryPoolOcclusionTests.hpp"

#include "vktTestCase.hpp"

#include "vktDrawImageObjectUtil.hpp"
#include "vktDrawBufferObjectUtil.hpp"
#include "vktDrawCreateInfoUtil.hpp"
#include "vkBuilderUtil.hpp"
#include "vkRefUtil.hpp"
#include "vkPrograms.hpp"

#include "tcuTestLog.hpp"
#include "tcuResource.hpp"
#include "tcuImageCompare.hpp"
#include "tcuCommandLine.hpp"

namespace vkt
{

namespace QueryPool
{

using namespace Draw;

namespace
{

struct StateObjects
{
			StateObjects	(const vk::DeviceInterface&vk, vkt::Context &context, const int numVertices, vk::VkPrimitiveTopology primitive);
	void	setVertices		(const vk::DeviceInterface&vk, std::vector<tcu::Vec4> vertices);

	enum
	{
		WIDTH	= 128,
		HEIGHT	= 128
	};

	vkt::Context &m_context;

	vk::Move<vk::VkPipeline>		m_pipeline;
	vk::Move<vk::VkPipelineLayout>	m_pipelineLayout;

	de::SharedPtr<Image>			m_colorAttachmentImage, m_DepthImage;
	vk::Move<vk::VkImageView>		m_attachmentView;
	vk::Move<vk::VkImageView>		m_depthiew;

	vk::Move<vk::VkRenderPass>		m_renderPass;
	vk::Move<vk::VkFramebuffer>		m_framebuffer;

	de::SharedPtr<Buffer>			m_vertexBuffer;

	vk::VkFormat					m_colorAttachmentFormat;
};

StateObjects::StateObjects (const vk::DeviceInterface&vk, vkt::Context &context, const int numVertices, vk::VkPrimitiveTopology primitive)
	: m_context(context)
	, m_colorAttachmentFormat(vk::VK_FORMAT_R8G8B8A8_UNORM)

{
	vk::VkFormat		depthFormat = vk::VK_FORMAT_D16_UNORM;
	const vk::VkDevice	device		= m_context.getDevice();

	//attachment images and views
	{
		vk::VkExtent3D imageExtent =
		{
			WIDTH,	// width;
			HEIGHT,	// height;
			1		// depth;
		};

		const ImageCreateInfo colorImageCreateInfo(vk::VK_IMAGE_TYPE_2D, m_colorAttachmentFormat, imageExtent, 1, 1, vk::VK_SAMPLE_COUNT_1_BIT, vk::VK_IMAGE_TILING_OPTIMAL,
												   vk::VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | vk::VK_IMAGE_USAGE_TRANSFER_SRC_BIT);

		m_colorAttachmentImage	= Image::createAndAlloc(vk, device, colorImageCreateInfo, m_context.getDefaultAllocator(), m_context.getUniversalQueueFamilyIndex());

		const ImageViewCreateInfo attachmentViewInfo(m_colorAttachmentImage->object(), vk::VK_IMAGE_VIEW_TYPE_2D, m_colorAttachmentFormat);
		m_attachmentView		= vk::createImageView(vk, device, &attachmentViewInfo);

		ImageCreateInfo depthImageCreateInfo(vk::VK_IMAGE_TYPE_2D, depthFormat, imageExtent, 1, 1, vk::VK_SAMPLE_COUNT_1_BIT, vk::VK_IMAGE_TILING_OPTIMAL,
			vk::VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);

		m_DepthImage			= Image::createAndAlloc(vk, device, depthImageCreateInfo, m_context.getDefaultAllocator(), m_context.getUniversalQueueFamilyIndex());

		// Construct a depth  view from depth image
		const ImageViewCreateInfo depthViewInfo(m_DepthImage->object(), vk::VK_IMAGE_VIEW_TYPE_2D, depthFormat);
		m_depthiew				= vk::createImageView(vk, device, &depthViewInfo);
	}

	{
		// Renderpass and Framebuffer

		RenderPassCreateInfo renderPassCreateInfo;
		renderPassCreateInfo.addAttachment(AttachmentDescription(m_colorAttachmentFormat,									// format
																	vk::VK_SAMPLE_COUNT_1_BIT,								// samples
																	vk::VK_ATTACHMENT_LOAD_OP_CLEAR,						// loadOp
																	vk::VK_ATTACHMENT_STORE_OP_DONT_CARE,					// storeOp
																	vk::VK_ATTACHMENT_LOAD_OP_DONT_CARE,					// stencilLoadOp
																	vk::VK_ATTACHMENT_STORE_OP_DONT_CARE,					// stencilLoadOp
																	vk::VK_IMAGE_LAYOUT_GENERAL,							// initialLauout
																	vk::VK_IMAGE_LAYOUT_GENERAL));							// finalLayout

		renderPassCreateInfo.addAttachment(AttachmentDescription(depthFormat,												// format
																 vk::VK_SAMPLE_COUNT_1_BIT,									// samples
																 vk::VK_ATTACHMENT_LOAD_OP_CLEAR,							// loadOp
																 vk::VK_ATTACHMENT_STORE_OP_DONT_CARE,						// storeOp
																 vk::VK_ATTACHMENT_LOAD_OP_DONT_CARE,						// stencilLoadOp
																 vk::VK_ATTACHMENT_STORE_OP_DONT_CARE,						// stencilLoadOp
																 vk::VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,		// initialLauout
																 vk::VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL));	// finalLayout

		const vk::VkAttachmentReference colorAttachmentReference =
		{
			0,															// attachment
			vk::VK_IMAGE_LAYOUT_GENERAL									// layout
		};

		const vk::VkAttachmentReference depthAttachmentReference =
		{
			1,															// attachment
			vk::VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL		// layout
		};

		renderPassCreateInfo.addSubpass(SubpassDescription(vk::VK_PIPELINE_BIND_POINT_GRAPHICS,					// pipelineBindPoint
														   0,													// flags
														   0,													// inputCount
														   DE_NULL,												// pInputAttachments
														   1,													// colorCount
														   &colorAttachmentReference,							// pColorAttachments
														   DE_NULL,												// pResolveAttachments
														   depthAttachmentReference,							// depthStencilAttachment
														   0,													// preserveCount
														   DE_NULL));											// preserveAttachments

		m_renderPass = vk::createRenderPass(vk, device, &renderPassCreateInfo);

		std::vector<vk::VkImageView> attachments(2);
		attachments[0] = *m_attachmentView;
		attachments[1] = *m_depthiew;

		FramebufferCreateInfo framebufferCreateInfo(*m_renderPass, attachments, WIDTH, HEIGHT, 1);
		m_framebuffer = vk::createFramebuffer(vk, device, &framebufferCreateInfo);
	}

	{
		// Pipeline

		vk::Unique<vk::VkShaderModule> vs(vk::createShaderModule(vk, device, m_context.getBinaryCollection().get("vert"), 0));
		vk::Unique<vk::VkShaderModule> fs(vk::createShaderModule(vk, device, m_context.getBinaryCollection().get("frag"), 0));

		const PipelineCreateInfo::ColorBlendState::Attachment attachmentState;

		const PipelineLayoutCreateInfo pipelineLayoutCreateInfo;
		m_pipelineLayout = vk::createPipelineLayout(vk, device, &pipelineLayoutCreateInfo);

		const vk::VkVertexInputBindingDescription vf_binding_desc		=
		{
			0,																// binding;
			4 * (deUint32)sizeof(float),									// stride;
			vk::VK_VERTEX_INPUT_RATE_VERTEX									// inputRate
		};

		const vk::VkVertexInputAttributeDescription vf_attribute_desc	=
		{
			0,																// location;
			0,																// binding;
			vk::VK_FORMAT_R32G32B32A32_SFLOAT,								// format;
			0																// offset;
		};

		const vk::VkPipelineVertexInputStateCreateInfo vf_info			=
		{																	// sType;
			vk::VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,	// pNext;
			NULL,															// flags;
			0u,																// vertexBindingDescriptionCount;
			1,																// pVertexBindingDescriptions;
			&vf_binding_desc,												// vertexAttributeDescriptionCount;
			1,																// pVertexAttributeDescriptions;
			&vf_attribute_desc
		};

		PipelineCreateInfo pipelineCreateInfo(*m_pipelineLayout, *m_renderPass, 0, 0);
		pipelineCreateInfo.addShader(PipelineCreateInfo::PipelineShaderStage(*vs, "main", vk::VK_SHADER_STAGE_VERTEX_BIT));
		pipelineCreateInfo.addShader(PipelineCreateInfo::PipelineShaderStage(*fs, "main", vk::VK_SHADER_STAGE_FRAGMENT_BIT));
		pipelineCreateInfo.addState(PipelineCreateInfo::InputAssemblerState(primitive));
		pipelineCreateInfo.addState(PipelineCreateInfo::ColorBlendState(1, &attachmentState));
		const vk::VkViewport viewport	=
		{
			0,		// float x;
			0,		// float y;
			WIDTH,	// float width;
			HEIGHT,	// float height;
			0.0f,	// float minDepth;
			1.0f	// float maxDepth;
		};

		const vk::VkRect2D scissor		=
		{
			{
				0,		// deInt32 x
				0,		// deInt32 y
			},		// VkOffset2D	offset;
			{
				WIDTH,	// deInt32 width;
				HEIGHT,	// deInt32 height
			},		// VkExtent2D	extent;
		};
		pipelineCreateInfo.addState(PipelineCreateInfo::ViewportState(1, std::vector<vk::VkViewport>(1, viewport), std::vector<vk::VkRect2D>(1, scissor)));
		pipelineCreateInfo.addState(PipelineCreateInfo::DepthStencilState(true, true, vk::VK_COMPARE_OP_GREATER_OR_EQUAL));
		pipelineCreateInfo.addState(PipelineCreateInfo::RasterizerState());
		pipelineCreateInfo.addState(PipelineCreateInfo::MultiSampleState());
		pipelineCreateInfo.addState(vf_info);
		m_pipeline = vk::createGraphicsPipeline(vk, device, DE_NULL, &pipelineCreateInfo);
	}

	{
		// Vertex buffer
		const size_t kBufferSize = numVertices * sizeof(tcu::Vec4);
		m_vertexBuffer = Buffer::createAndAlloc(vk, device, BufferCreateInfo(kBufferSize, vk::VK_BUFFER_USAGE_STORAGE_BUFFER_BIT), m_context.getDefaultAllocator(), vk::MemoryRequirement::HostVisible);
	}
}

void StateObjects::setVertices (const vk::DeviceInterface&vk, std::vector<tcu::Vec4> vertices)
{
	const vk::VkDevice device			= m_context.getDevice();

	tcu::Vec4 *ptr = reinterpret_cast<tcu::Vec4*>(m_vertexBuffer->getBoundMemory().getHostPtr());
	std::copy(vertices.begin(), vertices.end(), ptr);

	vk::flushMappedMemoryRange(vk, device,	m_vertexBuffer->getBoundMemory().getMemory(), m_vertexBuffer->getBoundMemory().getOffset(),	vertices.size() * sizeof(vertices[0]));
}

enum OcclusionQueryResultSize
{
	RESULT_SIZE_64_BIT,
	RESULT_SIZE_32_BIT,
};

enum OcclusionQueryWait
{
	WAIT_QUEUE,
	WAIT_QUERY,
	WAIT_NONE
};

enum OcclusionQueryResultsMode
{
	RESULTS_MODE_GET,
	RESULTS_MODE_COPY
};

struct OcclusionQueryTestVector
{
	vk::VkQueryControlFlags		queryControlFlags;
	OcclusionQueryResultSize	queryResultSize;
	OcclusionQueryWait			queryWait;
	OcclusionQueryResultsMode	queryResultsMode;
	vk::VkDeviceSize			queryResultsStride;
	bool						queryResultsAvailability;
	vk::VkPrimitiveTopology		primitiveTopology;
	bool						discardHalf;
};

class BasicOcclusionQueryTestInstance : public vkt::TestInstance
{
public:
					BasicOcclusionQueryTestInstance		(vkt::Context &context, const OcclusionQueryTestVector&  testVector);
					~BasicOcclusionQueryTestInstance	(void);
private:
	tcu::TestStatus	iterate								(void);

	enum
	{
		NUM_QUERIES_IN_POOL				= 2,
		QUERY_INDEX_CAPTURE_EMPTY		= 0,
		QUERY_INDEX_CAPTURE_DRAWCALL	= 1,
		NUM_VERTICES_IN_DRAWCALL		= 3
	};

	OcclusionQueryTestVector	m_testVector;
	StateObjects*				m_stateObjects;
	vk::VkQueryPool				m_queryPool;
};

BasicOcclusionQueryTestInstance::BasicOcclusionQueryTestInstance (vkt::Context &context, const OcclusionQueryTestVector&  testVector)
	: TestInstance		(context)
	, m_testVector		(testVector)
{
	DE_ASSERT(testVector.queryResultSize			== RESULT_SIZE_64_BIT
			&& testVector.queryWait					== WAIT_QUEUE
			&& testVector.queryResultsMode			== RESULTS_MODE_GET
			&& testVector.queryResultsStride		== sizeof(deUint64)
			&& testVector.queryResultsAvailability	== false
			&& testVector.primitiveTopology			== vk::VK_PRIMITIVE_TOPOLOGY_POINT_LIST);

	if ((m_testVector.queryControlFlags & vk::VK_QUERY_CONTROL_PRECISE_BIT) && !m_context.getDeviceFeatures().occlusionQueryPrecise)
		throw tcu::NotSupportedError("Precise occlusion queries are not supported");

	m_stateObjects = new StateObjects(m_context.getDeviceInterface(), m_context, NUM_VERTICES_IN_DRAWCALL, m_testVector.primitiveTopology);

	const vk::VkDevice			device	= m_context.getDevice();
	const vk::DeviceInterface&	vk		= m_context.getDeviceInterface();

	const vk::VkQueryPoolCreateInfo queryPoolCreateInfo =
	{
		vk::VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO,
		DE_NULL,
		0u,
		vk::VK_QUERY_TYPE_OCCLUSION,
		NUM_QUERIES_IN_POOL,
		0
	};
	VK_CHECK(vk.createQueryPool(device, &queryPoolCreateInfo, /*pAllocator*/ DE_NULL, &m_queryPool));

	std::vector<tcu::Vec4> vertices(NUM_VERTICES_IN_DRAWCALL);
	vertices[0] = tcu::Vec4(0.5, 0.5, 0.0, 1.0);
	vertices[1] = tcu::Vec4(0.5, 0.0, 0.0, 1.0);
	vertices[2] = tcu::Vec4(0.0, 0.5, 0.0, 1.0);
	m_stateObjects->setVertices(vk, vertices);
}

BasicOcclusionQueryTestInstance::~BasicOcclusionQueryTestInstance (void)
{
	if (m_stateObjects)
		delete m_stateObjects;

	if (m_queryPool != DE_NULL)
	{
		const vk::VkDevice device		= m_context.getDevice();
		const vk::DeviceInterface& vk	= m_context.getDeviceInterface();

		vk.destroyQueryPool(device, m_queryPool, /*pAllocator*/ DE_NULL);
	}
}

tcu::TestStatus	BasicOcclusionQueryTestInstance::iterate (void)
{
	tcu::TestLog &log				= m_context.getTestContext().getLog();
	const vk::VkDevice device		= m_context.getDevice();
	const vk::VkQueue queue			= m_context.getUniversalQueue();
	const vk::DeviceInterface& vk	= m_context.getDeviceInterface();

	const CmdPoolCreateInfo			cmdPoolCreateInfo	(m_context.getUniversalQueueFamilyIndex());
	vk::Move<vk::VkCommandPool>		cmdPool				= vk::createCommandPool(vk, device, &cmdPoolCreateInfo);

	vk::Unique<vk::VkCommandBuffer> cmdBuffer			(vk::allocateCommandBuffer(vk, device, *cmdPool, vk::VK_COMMAND_BUFFER_LEVEL_PRIMARY));
	const CmdBufferBeginInfo		beginInfo			(0u);

	vk.beginCommandBuffer(*cmdBuffer, &beginInfo);

	initialTransitionColor2DImage(vk, *cmdBuffer, m_stateObjects->m_colorAttachmentImage->object(), vk::VK_IMAGE_LAYOUT_GENERAL,
								  vk::VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, vk::VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
	initialTransitionDepth2DImage(vk, *cmdBuffer, m_stateObjects->m_DepthImage->object(), vk::VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
								  vk::VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, vk::VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | vk::VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT);

	std::vector<vk::VkClearValue> renderPassClearValues(2);
	deMemset(&renderPassClearValues[0], 0, static_cast<int>(renderPassClearValues.size()) * sizeof(vk::VkClearValue));

	const vk::VkRect2D renderArea =
	{
		{ 0,					0 },
		{ StateObjects::WIDTH,	StateObjects::HEIGHT }
	};

	RenderPassBeginInfo renderPassBegin(*m_stateObjects->m_renderPass, *m_stateObjects->m_framebuffer, renderArea, renderPassClearValues);

	vk.cmdResetQueryPool(*cmdBuffer, m_queryPool, 0, NUM_QUERIES_IN_POOL);

	vk.cmdBeginRenderPass(*cmdBuffer, &renderPassBegin, vk::VK_SUBPASS_CONTENTS_INLINE);

	vk.cmdBindPipeline(*cmdBuffer, vk::VK_PIPELINE_BIND_POINT_GRAPHICS, *m_stateObjects->m_pipeline);

	vk::VkBuffer vertexBuffer = m_stateObjects->m_vertexBuffer->object();
	const vk::VkDeviceSize vertexBufferOffset = 0;
	vk.cmdBindVertexBuffers(*cmdBuffer, 0, 1, &vertexBuffer, &vertexBufferOffset);

	vk.cmdBeginQuery(*cmdBuffer, m_queryPool, QUERY_INDEX_CAPTURE_EMPTY, m_testVector.queryControlFlags);
	vk.cmdEndQuery(*cmdBuffer, m_queryPool,	QUERY_INDEX_CAPTURE_EMPTY);

	vk.cmdBeginQuery(*cmdBuffer, m_queryPool, QUERY_INDEX_CAPTURE_DRAWCALL, m_testVector.queryControlFlags);
	vk.cmdDraw(*cmdBuffer, NUM_VERTICES_IN_DRAWCALL, 1, 0, 0);
	vk.cmdEndQuery(*cmdBuffer, m_queryPool,	QUERY_INDEX_CAPTURE_DRAWCALL);

	vk.cmdEndRenderPass(*cmdBuffer);

	transition2DImage(vk, *cmdBuffer, m_stateObjects->m_colorAttachmentImage->object(), vk::VK_IMAGE_ASPECT_COLOR_BIT,
					  vk::VK_IMAGE_LAYOUT_GENERAL, vk::VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, vk::VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
					  vk::VK_ACCESS_TRANSFER_READ_BIT, vk::VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, vk::VK_PIPELINE_STAGE_TRANSFER_BIT);

	vk.endCommandBuffer(*cmdBuffer);

	// Submit command buffer
	const vk::VkSubmitInfo submitInfo =
	{
		vk::VK_STRUCTURE_TYPE_SUBMIT_INFO,	// VkStructureType			sType;
		DE_NULL,							// const void*				pNext;
		0,									// deUint32					waitSemaphoreCount;
		DE_NULL,							// const VkSemaphore*		pWaitSemaphores;
		(const vk::VkPipelineStageFlags*)DE_NULL,
		1,									// deUint32					commandBufferCount;
		&cmdBuffer.get(),					// const VkCommandBuffer*	pCommandBuffers;
		0,									// deUint32					signalSemaphoreCount;
		DE_NULL								// const VkSemaphore*		pSignalSemaphores;
	};
	vk.queueSubmit(queue, 1, &submitInfo, DE_NULL);

	VK_CHECK(vk.queueWaitIdle(queue));

	deUint64 queryResults[NUM_QUERIES_IN_POOL] = { 0 };
	size_t queryResultsSize		= sizeof(queryResults);

	vk::VkResult queryResult	= vk.getQueryPoolResults(device, m_queryPool, 0, NUM_QUERIES_IN_POOL, queryResultsSize, queryResults, sizeof(queryResults[0]), vk::VK_QUERY_RESULT_64_BIT);

	if (queryResult == vk::VK_NOT_READY)
	{
		TCU_FAIL("Query result not avaliable, but vkWaitIdle() was called.");
	}

	VK_CHECK(queryResult);

	log << tcu::TestLog::Section("OcclusionQueryResults",
		"Occlusion query results");
	for (int ndx = 0; ndx < DE_LENGTH_OF_ARRAY(queryResults); ++ndx)
	{
		log << tcu::TestLog::Message << "query[ slot == " << ndx
			<< "] result == " << queryResults[ndx] << tcu::TestLog::EndMessage;
	}

	bool passed = true;

	for (int queryNdx = 0; queryNdx < DE_LENGTH_OF_ARRAY(queryResults); ++queryNdx)
	{

		deUint64 expectedValue;

		switch (queryNdx)
		{
			case QUERY_INDEX_CAPTURE_EMPTY:
				expectedValue = 0;
				break;
			case QUERY_INDEX_CAPTURE_DRAWCALL:
				expectedValue = NUM_VERTICES_IN_DRAWCALL;
				break;
		}

		if ((m_testVector.queryControlFlags & vk::VK_QUERY_CONTROL_PRECISE_BIT) || expectedValue == 0)
		{
			// require precise value
			if (queryResults[queryNdx] != expectedValue)
			{
				log << tcu::TestLog::Message << "vkGetQueryPoolResults returned "
					"wrong value of query for index "
					<< queryNdx << ", expected " << expectedValue << ", got "
					<< queryResults[0] << "." << tcu::TestLog::EndMessage;
				passed = false;
			}
		}
		else
		{
			// require imprecize value > 0
			if (queryResults[queryNdx] == 0)
			{
				log << tcu::TestLog::Message << "vkGetQueryPoolResults returned "
					"wrong value of query for index "
					<< queryNdx << ", expected any non-zero value, got "
					<< queryResults[0] << "." << tcu::TestLog::EndMessage;
				passed = false;
			}
		}
	}
	log << tcu::TestLog::EndSection;

	if (passed)
	{
		return tcu::TestStatus(QP_TEST_RESULT_PASS, "Query result verification passed");
	}
	return tcu::TestStatus(QP_TEST_RESULT_FAIL, "Query result verification failed");
}

class OcclusionQueryTestInstance : public vkt::TestInstance
{
public:
	OcclusionQueryTestInstance		(vkt::Context &context, const OcclusionQueryTestVector& testVector);
	~OcclusionQueryTestInstance		(void);
private:
	tcu::TestStatus					iterate							(void);

	bool							hasSeparateResetCmdBuf			(void) const;
	bool							hasSeparateCopyCmdBuf			(void) const;

	vk::Move<vk::VkCommandBuffer>	recordQueryPoolReset			(vk::VkCommandPool commandPool);
	vk::Move<vk::VkCommandBuffer>	recordRender					(vk::VkCommandPool commandPool);
	vk::Move<vk::VkCommandBuffer>	recordCopyResults				(vk::VkCommandPool commandPool);

	void							captureResults					(deUint64*			retResults,	deUint64*		retAvailability,	bool	allowNotReady);
	void							logResults						(const deUint64*	results,	const deUint64* availability);
	bool							validateResults					(const deUint64*	results,	const deUint64* availability,		bool	allowUnavailable,	vk::VkPrimitiveTopology primitiveTopology);

	enum
	{
		NUM_QUERIES_IN_POOL							= 3,
		QUERY_INDEX_CAPTURE_ALL						= 0,
		QUERY_INDEX_CAPTURE_PARTIALLY_OCCLUDED		= 1,
		QUERY_INDEX_CAPTURE_OCCLUDED				= 2
	};
	enum
	{
		NUM_VERTICES_IN_DRAWCALL					= 3,
		NUM_VERTICES_IN_PARTIALLY_OCCLUDED_DRAWCALL	= 3,
		NUM_VERTICES_IN_OCCLUDER_DRAWCALL			= 3,
		NUM_VERTICES								= NUM_VERTICES_IN_DRAWCALL + NUM_VERTICES_IN_PARTIALLY_OCCLUDED_DRAWCALL + NUM_VERTICES_IN_OCCLUDER_DRAWCALL
	};
	enum
	{
		START_VERTEX								= 0,
		START_VERTEX_PARTIALLY_OCCLUDED				= START_VERTEX + NUM_VERTICES_IN_DRAWCALL,
		START_VERTEX_OCCLUDER						= START_VERTEX_PARTIALLY_OCCLUDED + NUM_VERTICES_IN_PARTIALLY_OCCLUDED_DRAWCALL
	};

	OcclusionQueryTestVector		m_testVector;

	const vk::VkQueryResultFlags	m_queryResultFlags;

	StateObjects*					m_stateObjects;
	vk::VkQueryPool					m_queryPool;
	de::SharedPtr<Buffer>			m_queryPoolResultsBuffer;

	vk::Move<vk::VkCommandPool>		m_commandPool;
	vk::Move<vk::VkCommandBuffer>	m_queryPoolResetCommandBuffer;
	vk::Move<vk::VkCommandBuffer>	m_renderCommandBuffer;
	vk::Move<vk::VkCommandBuffer>	m_copyResultsCommandBuffer;
};

OcclusionQueryTestInstance::OcclusionQueryTestInstance (vkt::Context &context, const OcclusionQueryTestVector& testVector)
	: vkt::TestInstance		(context)
	, m_testVector			(testVector)
	, m_queryResultFlags	((m_testVector.queryWait == WAIT_QUERY					? vk::VK_QUERY_RESULT_WAIT_BIT				: 0)
							| (m_testVector.queryResultSize == RESULT_SIZE_64_BIT	? vk::VK_QUERY_RESULT_64_BIT				: 0)
							| (m_testVector.queryResultsAvailability				? vk::VK_QUERY_RESULT_WITH_AVAILABILITY_BIT	: 0))
{
	const vk::VkDevice			device				= m_context.getDevice();
	const vk::DeviceInterface&	vk					= m_context.getDeviceInterface();

	if ((m_testVector.queryControlFlags & vk::VK_QUERY_CONTROL_PRECISE_BIT) && !m_context.getDeviceFeatures().occlusionQueryPrecise)
		throw tcu::NotSupportedError("Precise occlusion queries are not supported");

	m_stateObjects  = new StateObjects(m_context.getDeviceInterface(), m_context, NUM_VERTICES_IN_DRAWCALL + NUM_VERTICES_IN_PARTIALLY_OCCLUDED_DRAWCALL + NUM_VERTICES_IN_OCCLUDER_DRAWCALL, m_testVector.primitiveTopology);

	const vk::VkQueryPoolCreateInfo queryPoolCreateInfo	=
	{
		vk::VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO,
		DE_NULL,
		0u,
		vk::VK_QUERY_TYPE_OCCLUSION,
		NUM_QUERIES_IN_POOL,
		0
	};

	VK_CHECK(vk.createQueryPool(device, &queryPoolCreateInfo, /*pAllocator*/ DE_NULL, &m_queryPool));

	if (m_testVector.queryResultsMode == RESULTS_MODE_COPY)
	{
		const vk::VkDeviceSize	resultsBufferSize			= m_testVector.queryResultsStride * NUM_QUERIES_IN_POOL;
								m_queryPoolResultsBuffer	= Buffer::createAndAlloc(vk, device, BufferCreateInfo(resultsBufferSize, vk::VK_BUFFER_USAGE_TRANSFER_DST_BIT), m_context.getDefaultAllocator(), vk::MemoryRequirement::HostVisible);
	}

	const CmdPoolCreateInfo	cmdPoolCreateInfo		(m_context.getUniversalQueueFamilyIndex());
							m_commandPool			= vk::createCommandPool(vk, device, &cmdPoolCreateInfo);
							m_renderCommandBuffer	= recordRender(*m_commandPool);

	if (hasSeparateResetCmdBuf())
	{
		m_queryPoolResetCommandBuffer	= recordQueryPoolReset(*m_commandPool);
	}

	if (hasSeparateCopyCmdBuf())
	{
		m_copyResultsCommandBuffer = recordCopyResults(*m_commandPool);
	}
}

OcclusionQueryTestInstance::~OcclusionQueryTestInstance (void)
{
	const vk::VkDevice device = m_context.getDevice();

	if (m_stateObjects)
		delete m_stateObjects;

	if (m_queryPool != DE_NULL)
	{
		const vk::DeviceInterface& vk = m_context.getDeviceInterface();
		vk.destroyQueryPool(device, m_queryPool, /*pAllocator*/ DE_NULL);
	}
}

tcu::TestStatus OcclusionQueryTestInstance::iterate (void)
{
	const vk::VkQueue			queue		= m_context.getUniversalQueue();
	const vk::DeviceInterface&	vk			= m_context.getDeviceInterface();
	tcu::TestLog&				log			= m_context.getTestContext().getLog();
	std::vector<tcu::Vec4>		vertices	(NUM_VERTICES);

	// 1st triangle
	vertices[START_VERTEX + 0] = tcu::Vec4( 0.5,  0.5, 0.5, 1.0);
	vertices[START_VERTEX + 1] = tcu::Vec4( 0.5, -0.5, 0.5, 1.0);
	vertices[START_VERTEX + 2] = tcu::Vec4(-0.5,  0.5, 0.5, 1.0);
	// 2nd triangle - partially occluding the scene
	vertices[START_VERTEX_PARTIALLY_OCCLUDED + 0] = tcu::Vec4(-0.5, -0.5, 1.0, 1.0);
	vertices[START_VERTEX_PARTIALLY_OCCLUDED + 1] = tcu::Vec4( 0.5, -0.5, 1.0, 1.0);
	vertices[START_VERTEX_PARTIALLY_OCCLUDED + 2] = tcu::Vec4(-0.5,  0.5, 1.0, 1.0);
	// 3nd triangle - fully occluding the scene
	vertices[START_VERTEX_OCCLUDER + 0] = tcu::Vec4( 0.5,  0.5, 1.0, 1.0);
	vertices[START_VERTEX_OCCLUDER + 1] = tcu::Vec4( 0.5, -0.5, 1.0, 1.0);
	vertices[START_VERTEX_OCCLUDER + 2] = tcu::Vec4(-0.5,  0.5, 1.0, 1.0);

	m_stateObjects->setVertices(vk, vertices);

	if (hasSeparateResetCmdBuf())
	{
		const vk::VkSubmitInfo		submitInfoReset =
		{
			vk::VK_STRUCTURE_TYPE_SUBMIT_INFO,			// VkStructureType			sType;
			DE_NULL,									// const void*				pNext;
			0u,											// deUint32					waitSemaphoreCount;
			DE_NULL,									// const VkSemaphore*		pWaitSemaphores;
			(const vk::VkPipelineStageFlags*)DE_NULL,
			1u,											// deUint32					commandBufferCount;
			&m_queryPoolResetCommandBuffer.get(),		// const VkCommandBuffer*	pCommandBuffers;
			0u,											// deUint32					signalSemaphoreCount;
			DE_NULL										// const VkSemaphore*		pSignalSemaphores;
		};

		vk.queueSubmit(queue, 1, &submitInfoReset, DE_NULL);

		// Trivially wait for reset to complete. This is to ensure the query pool is in reset state before
		// host accesses, so as to not insert any synchronization before capturing the results needed for WAIT_NONE
		// variant of test.
		VK_CHECK(vk.queueWaitIdle(queue));
	}

	{
		const vk::VkSubmitInfo submitInfoRender =
		{
			vk::VK_STRUCTURE_TYPE_SUBMIT_INFO,	// VkStructureType			sType;
			DE_NULL,							// const void*				pNext;
			0,									// deUint32					waitSemaphoreCount;
			DE_NULL,							// const VkSemaphore*		pWaitSemaphores;
			(const vk::VkPipelineStageFlags*)DE_NULL,
			1,									// deUint32					commandBufferCount;
			&m_renderCommandBuffer.get(),		// const VkCommandBuffer*	pCommandBuffers;
			0,									// deUint32					signalSemaphoreCount;
			DE_NULL								// const VkSemaphore*		pSignalSemaphores;
		};
		vk.queueSubmit(queue, 1, &submitInfoRender, DE_NULL);
	}

	if (m_testVector.queryWait == WAIT_QUEUE)
	{
		VK_CHECK(vk.queueWaitIdle(queue));
	}

	if (hasSeparateCopyCmdBuf())
	{
		// In case of WAIT_QUEUE test variant, the previously submitted m_renderCommandBuffer did not
		// contain vkCmdCopyQueryResults, so additional cmd buffer is needed.

		// In the case of WAIT_NONE or WAIT_QUERY, vkCmdCopyQueryResults is stored in m_renderCommandBuffer.

		const vk::VkSubmitInfo submitInfo =
		{
			vk::VK_STRUCTURE_TYPE_SUBMIT_INFO,	// VkStructureType			sType;
			DE_NULL,							// const void*				pNext;
			0,									// deUint32					waitSemaphoreCount;
			DE_NULL,							// const VkSemaphore*		pWaitSemaphores;
			(const vk::VkPipelineStageFlags*)DE_NULL,
			1,									// deUint32					commandBufferCount;
			&m_copyResultsCommandBuffer.get(),	// const VkCommandBuffer*	pCommandBuffers;
			0,									// deUint32					signalSemaphoreCount;
			DE_NULL								// const VkSemaphore*		pSignalSemaphores;
		};
		vk.queueSubmit(queue, 1, &submitInfo, DE_NULL);
	}

	if (m_testVector.queryResultsMode == RESULTS_MODE_COPY)
	{
		// In case of vkCmdCopyQueryResults is used, test must always wait for it
		// to complete before we can read the result buffer.

		VK_CHECK(vk.queueWaitIdle(queue));
	}

	deUint64	queryResults		[NUM_QUERIES_IN_POOL];
	deUint64	queryAvailability	[NUM_QUERIES_IN_POOL];

	// Allow not ready results only if nobody waited before getting the query results
	const bool	allowNotReady		= (m_testVector.queryWait == WAIT_NONE);

	captureResults(queryResults, queryAvailability, allowNotReady);

	log << tcu::TestLog::Section("OcclusionQueryResults", "Occlusion query results");

	logResults(queryResults, queryAvailability);
	bool passed = validateResults(queryResults, queryAvailability, allowNotReady, m_testVector.primitiveTopology);

	log << tcu::TestLog::EndSection;

	if (m_testVector.queryResultsMode != RESULTS_MODE_COPY)
	{
		VK_CHECK(vk.queueWaitIdle(queue));
	}

		if (passed)
	{
		return tcu::TestStatus(QP_TEST_RESULT_PASS, "Query result verification passed");
	}
	return tcu::TestStatus(QP_TEST_RESULT_FAIL, "Query result verification failed");
}

bool OcclusionQueryTestInstance::hasSeparateResetCmdBuf (void) const
{
	// Determine if resetting query pool should be performed in separate command buffer
	// to avoid race condition between host query access and device query reset.

	if (m_testVector.queryResultsMode == RESULTS_MODE_COPY)
	{
		// We copy query results on device, so there is no race condition between
		// host and device
		return false;
	}
	if (m_testVector.queryWait == WAIT_QUEUE)
	{
		// We wait for queue to be complete before accessing query results
		return false;
	}

	// Separate command buffer with reset must be submitted & completed before
	// host accesses the query results
	return true;
}

bool OcclusionQueryTestInstance::hasSeparateCopyCmdBuf (void) const
{
	// Copy query results must go into separate command buffer, if we want to wait on queue before that
	return (m_testVector.queryResultsMode == RESULTS_MODE_COPY && m_testVector.queryWait == WAIT_QUEUE);
}

vk::Move<vk::VkCommandBuffer> OcclusionQueryTestInstance::recordQueryPoolReset (vk::VkCommandPool cmdPool)
{
	const vk::VkDevice				device		= m_context.getDevice();
	const vk::DeviceInterface&		vk			= m_context.getDeviceInterface();

	DE_ASSERT(hasSeparateResetCmdBuf());

	vk::Move<vk::VkCommandBuffer>	cmdBuffer	(vk::allocateCommandBuffer(vk, device, cmdPool, vk::VK_COMMAND_BUFFER_LEVEL_PRIMARY));
	CmdBufferBeginInfo				beginInfo	(0u);

	vk.beginCommandBuffer(*cmdBuffer, &beginInfo);
	vk.cmdResetQueryPool(*cmdBuffer, m_queryPool, 0, NUM_QUERIES_IN_POOL);
	vk.endCommandBuffer(*cmdBuffer);

	return cmdBuffer;
}

vk::Move<vk::VkCommandBuffer> OcclusionQueryTestInstance::recordRender (vk::VkCommandPool cmdPool)
{
	const vk::VkDevice				device		= m_context.getDevice();
	const vk::DeviceInterface&		vk			= m_context.getDeviceInterface();

	vk::Move<vk::VkCommandBuffer>	cmdBuffer	(vk::allocateCommandBuffer(vk, device, cmdPool, vk::VK_COMMAND_BUFFER_LEVEL_PRIMARY));
	CmdBufferBeginInfo				beginInfo	(0u);

	vk.beginCommandBuffer(*cmdBuffer, &beginInfo);

	initialTransitionColor2DImage(vk, *cmdBuffer, m_stateObjects->m_colorAttachmentImage->object(), vk::VK_IMAGE_LAYOUT_GENERAL,
								  vk::VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, vk::VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
	initialTransitionDepth2DImage(vk, *cmdBuffer, m_stateObjects->m_DepthImage->object(), vk::VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
								  vk::VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, vk::VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | vk::VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT);

	std::vector<vk::VkClearValue>	renderPassClearValues(2);
	deMemset(&renderPassClearValues[0], 0, static_cast<int>(renderPassClearValues.size()) * sizeof(vk::VkClearValue));

	const vk::VkRect2D renderArea =
	{
		{ 0,					0 },
		{ StateObjects::WIDTH,	StateObjects::HEIGHT }
	};

	RenderPassBeginInfo renderPassBegin(*m_stateObjects->m_renderPass, *m_stateObjects->m_framebuffer, renderArea, renderPassClearValues);

	if (!hasSeparateResetCmdBuf())
	{
		vk.cmdResetQueryPool(*cmdBuffer, m_queryPool, 0, NUM_QUERIES_IN_POOL);
	}

	vk.cmdBeginRenderPass(*cmdBuffer, &renderPassBegin, vk::VK_SUBPASS_CONTENTS_INLINE);

	vk.cmdBindPipeline(*cmdBuffer, vk::VK_PIPELINE_BIND_POINT_GRAPHICS,	*m_stateObjects->m_pipeline);

	vk::VkBuffer vertexBuffer = m_stateObjects->m_vertexBuffer->object();
	const vk::VkDeviceSize vertexBufferOffset = 0;
	vk.cmdBindVertexBuffers(*cmdBuffer, 0, 1, &vertexBuffer, &vertexBufferOffset);

	// Draw un-occluded geometry
	vk.cmdBeginQuery(*cmdBuffer, m_queryPool, QUERY_INDEX_CAPTURE_ALL, m_testVector.queryControlFlags);
	vk.cmdDraw(*cmdBuffer, NUM_VERTICES_IN_DRAWCALL, 1, START_VERTEX, 0);
	vk.cmdEndQuery(*cmdBuffer, m_queryPool,	QUERY_INDEX_CAPTURE_ALL);

	// Partially occlude geometry
	vk.cmdDraw(*cmdBuffer, NUM_VERTICES_IN_PARTIALLY_OCCLUDED_DRAWCALL, 1, START_VERTEX_PARTIALLY_OCCLUDED, 0);

	// Draw partially-occluded geometry
	vk.cmdBeginQuery(*cmdBuffer, m_queryPool, QUERY_INDEX_CAPTURE_PARTIALLY_OCCLUDED, m_testVector.queryControlFlags);
	vk.cmdDraw(*cmdBuffer, NUM_VERTICES_IN_DRAWCALL, 1, START_VERTEX, 0);
	vk.cmdEndQuery(*cmdBuffer, m_queryPool, QUERY_INDEX_CAPTURE_PARTIALLY_OCCLUDED);

	// Occlude geometry
	vk.cmdDraw(*cmdBuffer, NUM_VERTICES_IN_OCCLUDER_DRAWCALL, 1, START_VERTEX_OCCLUDER, 0);

	// Draw occluded geometry
	vk.cmdBeginQuery(*cmdBuffer, m_queryPool, QUERY_INDEX_CAPTURE_OCCLUDED, m_testVector.queryControlFlags);
	vk.cmdDraw(*cmdBuffer, NUM_VERTICES_IN_DRAWCALL, 1, START_VERTEX, 0);
	vk.cmdEndQuery(*cmdBuffer, m_queryPool,	QUERY_INDEX_CAPTURE_OCCLUDED);

	vk.cmdEndRenderPass(*cmdBuffer);

	if (m_testVector.queryResultsMode == RESULTS_MODE_COPY && !hasSeparateCopyCmdBuf())
	{
		vk.cmdCopyQueryPoolResults(*cmdBuffer, m_queryPool, 0, NUM_QUERIES_IN_POOL, m_queryPoolResultsBuffer->object(), /*dstOffset*/ 0, m_testVector.queryResultsStride, m_queryResultFlags);
	}

	transition2DImage(vk, *cmdBuffer, m_stateObjects->m_colorAttachmentImage->object(), vk::VK_IMAGE_ASPECT_COLOR_BIT, vk::VK_IMAGE_LAYOUT_GENERAL,
					  vk::VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, vk::VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, vk::VK_ACCESS_TRANSFER_READ_BIT,
					  vk::VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, vk::VK_PIPELINE_STAGE_TRANSFER_BIT);

	vk.endCommandBuffer(*cmdBuffer);

	return cmdBuffer;
}

vk::Move<vk::VkCommandBuffer> OcclusionQueryTestInstance::recordCopyResults (vk::VkCommandPool cmdPool)
{
	const vk::VkDevice				device		= m_context.getDevice();
	const vk::DeviceInterface&		vk			= m_context.getDeviceInterface();

	vk::Move<vk::VkCommandBuffer>	cmdBuffer	(vk::allocateCommandBuffer(vk, device, cmdPool, vk::VK_COMMAND_BUFFER_LEVEL_PRIMARY));
	const CmdBufferBeginInfo		beginInfo	(0u);

	vk.beginCommandBuffer(*cmdBuffer, &beginInfo);
	vk.cmdCopyQueryPoolResults(*cmdBuffer, m_queryPool, 0, NUM_QUERIES_IN_POOL, m_queryPoolResultsBuffer->object(), /*dstOffset*/ 0, m_testVector.queryResultsStride, m_queryResultFlags);
	vk.endCommandBuffer(*cmdBuffer);

	return cmdBuffer;
}

void OcclusionQueryTestInstance::captureResults (deUint64* retResults, deUint64* retAvailAbility, bool allowNotReady)
{

	const vk::VkDevice			device			= m_context.getDevice();
	const vk::DeviceInterface&	vk				= m_context.getDeviceInterface();
	std::vector<deUint8>		resultsBuffer	(static_cast<size_t>(m_testVector.queryResultsStride) * NUM_QUERIES_IN_POOL);

	if (m_testVector.queryResultsMode == RESULTS_MODE_GET)
	{
		const vk::VkResult queryResult = vk.getQueryPoolResults(device, m_queryPool, 0, NUM_QUERIES_IN_POOL, resultsBuffer.size(), &resultsBuffer[0], m_testVector.queryResultsStride, m_queryResultFlags);
		if (queryResult == vk::VK_NOT_READY && !allowNotReady)
		{
			TCU_FAIL("getQueryPoolResults returned VK_NOT_READY, but results should be already available.");
		}
		else
		{
			VK_CHECK(queryResult);
		}
	}
	else if (m_testVector.queryResultsMode == RESULTS_MODE_COPY)
	{
		const vk::Allocation& allocation = m_queryPoolResultsBuffer->getBoundMemory();
		const void* allocationData = allocation.getHostPtr();

		vk::invalidateMappedMemoryRange(vk, device, allocation.getMemory(), allocation.getOffset(), resultsBuffer.size());

		deMemcpy(&resultsBuffer[0], allocationData, resultsBuffer.size());
	}

	for (int queryNdx = 0; queryNdx < NUM_QUERIES_IN_POOL; queryNdx++)
	{
		const void* srcPtr = &resultsBuffer[queryNdx * static_cast<size_t>(m_testVector.queryResultsStride)];
		if (m_testVector.queryResultSize == RESULT_SIZE_32_BIT)
		{
			const deUint32* srcPtrTyped = static_cast<const deUint32*>(srcPtr);
			retResults[queryNdx]		= *srcPtrTyped;
			if (m_testVector.queryResultsAvailability)
			{
				retAvailAbility[queryNdx] = *(srcPtrTyped + 1);
			}
		}
		else if (m_testVector.queryResultSize == RESULT_SIZE_64_BIT)
		{
			const deUint64* srcPtrTyped = static_cast<const deUint64*>(srcPtr);
			retResults[queryNdx]		= *srcPtrTyped;

			if (m_testVector.queryResultsAvailability)
			{
				if (m_testVector.queryResultsAvailability)
				{
					retAvailAbility[queryNdx] = *(srcPtrTyped + 1);
				}
			}
		}
		else
		{
			TCU_FAIL("Wrong m_testVector.queryResultSize");
		}
	}
}

void OcclusionQueryTestInstance::logResults (const deUint64* results, const deUint64* availability)
{
	tcu::TestLog& log = m_context.getTestContext().getLog();

	for (int ndx = 0; ndx < NUM_QUERIES_IN_POOL; ++ndx)
	{
		if (!m_testVector.queryResultsAvailability)
		{
			log << tcu::TestLog::Message << "query[ slot == " << ndx << "] result == " << results[ndx] << tcu::TestLog::EndMessage;
		}
		else
		{
			log << tcu::TestLog::Message << "query[ slot == " << ndx << "] result == " << results[ndx] << ", availability	== " << availability[ndx] << tcu::TestLog::EndMessage;
		}
	}
}

bool OcclusionQueryTestInstance::validateResults (const deUint64* results , const deUint64* availability, bool allowUnavailable, vk::VkPrimitiveTopology primitiveTopology)
{
	bool passed			= true;
	tcu::TestLog& log	= m_context.getTestContext().getLog();

	for (int queryNdx = 0; queryNdx < NUM_QUERIES_IN_POOL; ++queryNdx)
	{
		deUint64 expectedValueMin = 0;
		deUint64 expectedValueMax = 0;

		if (m_testVector.queryResultsAvailability && availability[queryNdx] == 0)
		{
			// query result was not available
			if (!allowUnavailable)
			{
				log << tcu::TestLog::Message << "query results availability was 0 for index "
					<< queryNdx << ", expected any value greater than 0." << tcu::TestLog::EndMessage;
				passed = false;
				continue;
			}
		}
		else
		{
			// query is available, so expect proper result values
			if (primitiveTopology == vk::VK_PRIMITIVE_TOPOLOGY_POINT_LIST)
			{
				switch (queryNdx)
				{
					case QUERY_INDEX_CAPTURE_OCCLUDED:
						expectedValueMin = 0;
						expectedValueMax = 0;
						break;
					case QUERY_INDEX_CAPTURE_PARTIALLY_OCCLUDED:
						expectedValueMin = 1;
						expectedValueMax = 1;
						break;
					case QUERY_INDEX_CAPTURE_ALL:
						expectedValueMin = NUM_VERTICES_IN_DRAWCALL;
						expectedValueMax = NUM_VERTICES_IN_DRAWCALL;
						break;
				}
			}
			else if (primitiveTopology == vk::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
			{
				switch (queryNdx)
				{
					case QUERY_INDEX_CAPTURE_OCCLUDED:
						expectedValueMin = 0;
						expectedValueMax = 0;
						break;
					case QUERY_INDEX_CAPTURE_PARTIALLY_OCCLUDED:
					case QUERY_INDEX_CAPTURE_ALL:
						{
							const int primWidth		= StateObjects::WIDTH  / 2;
							const int primHeight	= StateObjects::HEIGHT / 2;
							const int primArea		= primWidth * primHeight / 2;

							if (m_testVector.discardHalf)
							{
								expectedValueMin	= (int)(0.95f * primArea * 0.5f);
								expectedValueMax	= (int)(1.05f * primArea * 0.5f);
							}
							else
							{
								expectedValueMin	= (int)(0.97f * primArea);
								expectedValueMax	= (int)(1.03f * primArea);
							}
						}
				}
			}
			else
			{
				TCU_FAIL("Unsupported primitive topology");
			}
		}

		if ((m_testVector.queryControlFlags & vk::VK_QUERY_CONTROL_PRECISE_BIT) || (expectedValueMin == 0 && expectedValueMax == 0))
		{
			// require precise value
			if (results[queryNdx] < expectedValueMin || results[queryNdx] > expectedValueMax)
			{
				log << tcu::TestLog::Message << "wrong value of query for index "
					<< queryNdx << ", expected the value minimum of " << expectedValueMin << ", maximum of " << expectedValueMax << " got "
					<< results[queryNdx] << "." << tcu::TestLog::EndMessage;
				passed = false;
			}
		}
		else
		{
			// require imprecise value greater than 0
			if (results[queryNdx] == 0)
			{
				log << tcu::TestLog::Message << "wrong value of query for index "
					<< queryNdx << ", expected any non-zero value, got "
					<< results[queryNdx] << "." << tcu::TestLog::EndMessage;
				passed = false;
			}
		}
	}
	return passed;
}

template<class Instance>
class QueryPoolOcclusionTest : public vkt::TestCase
{
public:
	QueryPoolOcclusionTest (tcu::TestContext &context, const char *name, const char *description, const OcclusionQueryTestVector& testVector)
		: TestCase			(context, name, description)
		, m_testVector		(testVector)
	{
	}
private:
	vkt::TestInstance* createInstance (vkt::Context& context) const
	{
		return new Instance(context, m_testVector);
	}

	void initPrograms(vk::SourceCollections& programCollection) const
	{
		const char* const discard =
			"	if ((int(gl_FragCoord.x) % 2) == (int(gl_FragCoord.y) % 2))\n"
			"		discard;\n";

		const std::string fragSrc = std::string(
			"#version 400\n"
			"layout(location = 0) out vec4 out_FragColor;\n"
			"void main()\n"
			"{\n"
			"	out_FragColor = vec4(0.07, 0.48, 0.75, 1.0);\n")
			+ std::string(m_testVector.discardHalf ? discard : "")
			+ "}\n";

		programCollection.glslSources.add("frag") << glu::FragmentSource(fragSrc.c_str());

		programCollection.glslSources.add("vert") << glu::VertexSource("#version 430\n"
																		 "layout(location = 0) in vec4 in_Position;\n"
																		 "out gl_PerVertex { vec4 gl_Position; float gl_PointSize; };\n"
																		 "void main() {\n"
																		 "	gl_Position  = in_Position;\n"
																		 "	gl_PointSize = 1.0;\n"
																		 "}\n");
	}

	OcclusionQueryTestVector m_testVector;
};

} //anonymous

QueryPoolOcclusionTests::QueryPoolOcclusionTests (tcu::TestContext &testCtx)
	: TestCaseGroup(testCtx, "occlusion_query", "Tests for occlusion queries")
{
	/* Left blank on purpose */
}

QueryPoolOcclusionTests::~QueryPoolOcclusionTests (void)
{
	/* Left blank on purpose */
}

void QueryPoolOcclusionTests::init (void)
{
	OcclusionQueryTestVector baseTestVector;
	baseTestVector.queryControlFlags		= 0;
	baseTestVector.queryResultSize			= RESULT_SIZE_64_BIT;
	baseTestVector.queryWait				= WAIT_QUEUE;
	baseTestVector.queryResultsMode			= RESULTS_MODE_GET;
	baseTestVector.queryResultsStride		= sizeof(deUint64);
	baseTestVector.queryResultsAvailability = false;
	baseTestVector.primitiveTopology		= vk::VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
	baseTestVector.discardHalf				= false;

	//Basic tests
	{
		OcclusionQueryTestVector testVector = baseTestVector;
		testVector.queryControlFlags = 0;
		addChild(new QueryPoolOcclusionTest<BasicOcclusionQueryTestInstance>(m_testCtx,	"basic_conservative",	"draw with conservative occlusion query",	testVector));
		testVector.queryControlFlags = vk::VK_QUERY_CONTROL_PRECISE_BIT;
		addChild(new QueryPoolOcclusionTest<BasicOcclusionQueryTestInstance>(m_testCtx,	"basic_precise",		"draw with precise occlusion query",		testVector));
	}

	// Functional test
	{
		const vk::VkQueryControlFlags	controlFlags[]		= { 0,					vk::VK_QUERY_CONTROL_PRECISE_BIT	};
		const char* const				controlFlagsStr[]	= { "conservative",		"precise"							};

		for (int controlFlagIdx = 0; controlFlagIdx < DE_LENGTH_OF_ARRAY(controlFlags); ++controlFlagIdx)
		{

			const vk::VkPrimitiveTopology	primitiveTopology[]		= { vk::VK_PRIMITIVE_TOPOLOGY_POINT_LIST, vk::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST };
			const char* const				primitiveTopologyStr[]	= { "points", "triangles" };
			for (int primitiveTopologyIdx = 0; primitiveTopologyIdx < DE_LENGTH_OF_ARRAY(primitiveTopology); ++primitiveTopologyIdx)
			{

				const OcclusionQueryResultSize	resultSize[]	= { RESULT_SIZE_32_BIT, RESULT_SIZE_64_BIT };
				const char* const				resultSizeStr[] = { "32",				"64" };

				for (int resultSizeIdx = 0; resultSizeIdx < DE_LENGTH_OF_ARRAY(resultSize); ++resultSizeIdx)
				{

					const OcclusionQueryWait	wait[]		= { WAIT_QUEUE, WAIT_QUERY };
					const char* const			waitStr[]	= { "queue",	"query" };

					for (int waitIdx = 0; waitIdx < DE_LENGTH_OF_ARRAY(wait); ++waitIdx)
					{
						const OcclusionQueryResultsMode	resultsMode[]		= { RESULTS_MODE_GET,	RESULTS_MODE_COPY };
						const char* const				resultsModeStr[]	= { "get",				"copy" };

						for (int resultsModeIdx = 0; resultsModeIdx < DE_LENGTH_OF_ARRAY(resultsMode); ++resultsModeIdx)
						{

							const bool			testAvailability[]		= { false, true };
							const char* const	testAvailabilityStr[]	= { "without", "with"};

							for (int testAvailabilityIdx = 0; testAvailabilityIdx < DE_LENGTH_OF_ARRAY(testAvailability); ++testAvailabilityIdx)
							{
								const bool			discardHalf[]		= { false, true };
								const char* const	discardHalfStr[]	= { "", "_discard" };

								for (int discardHalfIdx = 0; discardHalfIdx < DE_LENGTH_OF_ARRAY(discardHalf); ++discardHalfIdx)
								{
									OcclusionQueryTestVector testVector			= baseTestVector;
									testVector.queryControlFlags				= controlFlags[controlFlagIdx];
									testVector.queryResultSize					= resultSize[resultSizeIdx];
									testVector.queryWait						= wait[waitIdx];
									testVector.queryResultsMode					= resultsMode[resultsModeIdx];
									testVector.queryResultsStride				= (testVector.queryResultSize == RESULT_SIZE_32_BIT ? sizeof(deUint32) : sizeof(deUint64));
									testVector.queryResultsAvailability			= testAvailability[testAvailabilityIdx];
									testVector.primitiveTopology				= primitiveTopology[primitiveTopologyIdx];
									testVector.discardHalf						= discardHalf[discardHalfIdx];

									if (testVector.discardHalf && testVector.primitiveTopology == vk::VK_PRIMITIVE_TOPOLOGY_POINT_LIST)
										continue; // Discarding half of the pixels in fragment shader doesn't make sense with one-pixel-sized points.

									if (testVector.queryResultsAvailability)
									{
										testVector.queryResultsStride *= 2;
									}

									std::ostringstream testName;
									std::ostringstream testDescr;

									testName << resultsModeStr[resultsModeIdx] << "_results"
											 << "_" << controlFlagsStr[controlFlagIdx]
											 << "_size_" << resultSizeStr[resultSizeIdx]
											 << "_wait_" << waitStr[waitIdx]
											 << "_" << testAvailabilityStr[testAvailabilityIdx] << "_availability"
											 << "_draw_" <<  primitiveTopologyStr[primitiveTopologyIdx]
											 << discardHalfStr[discardHalfIdx];

									testDescr << "draw occluded " << primitiveTopologyStr[primitiveTopologyIdx]
											  << "with " << controlFlagsStr[controlFlagIdx] << ", "
											  << resultsModeStr[resultsModeIdx] << " results "
											  << testAvailabilityStr[testAvailabilityIdx] << " availability bit as "
											  << resultSizeStr[resultSizeIdx] << "bit variables,"
											  << (testVector.discardHalf ? " discarding half of the fragments," : "")
											  << "wait for results on" << waitStr[waitIdx];

									addChild(new QueryPoolOcclusionTest<OcclusionQueryTestInstance>(m_testCtx, testName.str().c_str(), testDescr.str().c_str(), testVector));
								}
							}
						}
					}
				}
			}
		}
	}
	// Test different strides
	{
		const OcclusionQueryResultsMode	resultsMode[]		= { RESULTS_MODE_GET,	RESULTS_MODE_COPY	};
		const char* const				resultsModeStr[]	= { "get",				"copy"				};

		for (int resultsModeIdx = 0; resultsModeIdx < DE_LENGTH_OF_ARRAY(resultsMode); ++resultsModeIdx)
		{
			const OcclusionQueryResultSize	resultSizes[]	= { RESULT_SIZE_32_BIT, RESULT_SIZE_64_BIT };
			const char* const				resultSizeStr[] = { "32", "64" };

			const bool			testAvailability[]		= { false,		true	};
			const char* const	testAvailabilityStr[]	= { "without",	"with"	};

			for (int testAvailabilityIdx = 0; testAvailabilityIdx < DE_LENGTH_OF_ARRAY(testAvailability); ++testAvailabilityIdx)
			{
				for (int resultSizeIdx = 0; resultSizeIdx < DE_LENGTH_OF_ARRAY(resultSizes); ++resultSizeIdx)
				{
					const vk::VkDeviceSize resultSize	= (resultSizes[resultSizeIdx] == RESULT_SIZE_32_BIT ? sizeof(deUint32) : sizeof(deUint64));

					// \todo [2015-12-18 scygan] Ensure only stride values aligned to resultSize are allowed. Otherwise test should be extended.
					const vk::VkDeviceSize strides[]	=
					{
						1 * resultSize,
						2 * resultSize,
						3 * resultSize,
						4 * resultSize,
						5 * resultSize,
						13 * resultSize,
						1024 * resultSize
					};

					for (int strideIdx = 0; strideIdx < DE_LENGTH_OF_ARRAY(strides); strideIdx++)
					{
						OcclusionQueryTestVector testVector		= baseTestVector;
						testVector.queryResultsMode				= resultsMode[resultsModeIdx];
						testVector.queryResultSize				= resultSizes[resultSizeIdx];
						testVector.queryResultsAvailability		= testAvailability[testAvailabilityIdx];
						testVector.queryResultsStride			= strides[strideIdx];

						const vk::VkDeviceSize elementSize		= (testVector.queryResultsAvailability ? resultSize * 2 : resultSize);

						if (elementSize > testVector.queryResultsStride)
						{
							continue;
						}

						std::ostringstream testName;
						std::ostringstream testDescr;

						testName << resultsModeStr[resultsModeIdx]
								 << "_results_size_" << resultSizeStr[resultSizeIdx]
								 << "_stride_" << strides[strideIdx]
								 << "_" << testAvailabilityStr[testAvailabilityIdx] << "_availability";

						testDescr << resultsModeStr[resultsModeIdx] << " results "
								  << testAvailabilityStr[testAvailabilityIdx] << " availability bit as "
								  << resultSizeStr[resultSizeIdx] << "bit variables, with stride" << strides[strideIdx];

						addChild(new QueryPoolOcclusionTest<OcclusionQueryTestInstance>(m_testCtx, testName.str().c_str(), testDescr.str().c_str(), testVector));
					}
				}
			}
		}

	}
}

} //QueryPool
} //vkt

