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
 * \brief Simple Draw Tests
 *//*--------------------------------------------------------------------*/

#include "vktDrawSimpleTest.hpp"

#include "vktTestCaseUtil.hpp"
#include "vktDrawTestCaseUtil.hpp"

#include "vktDrawBaseClass.hpp"

#include "tcuTestLog.hpp"
#include "tcuResource.hpp"
#include "tcuImageCompare.hpp"
#include "tcuTextureUtil.hpp"
#include "tcuRGBA.hpp"

#include "vkDefs.hpp"

namespace vkt
{
namespace Draw
{
namespace
{
class SimpleDraw : public DrawTestsBaseClass
{
public:
	typedef TestSpecBase	TestSpec;
							SimpleDraw				(Context &context, TestSpec testSpec);
	virtual tcu::TestStatus iterate					(void);
};

class SimpleDrawInstanced : public SimpleDraw
{
public:
	typedef TestSpec		TestSpec;
							SimpleDrawInstanced		(Context &context, TestSpec testSpec);
	tcu::TestStatus			iterate					(void);
};

SimpleDraw::SimpleDraw (Context &context, TestSpec testSpec)
	: DrawTestsBaseClass(context, testSpec.shaders[glu::SHADERTYPE_VERTEX], testSpec.shaders[glu::SHADERTYPE_FRAGMENT], testSpec.topology)
{
	m_data.push_back(VertexElementData(tcu::Vec4(1.0f, -1.0f, 1.0f, 1.0f), tcu::RGBA::blue().toVec(), -1));
	m_data.push_back(VertexElementData(tcu::Vec4(-1.0f, 1.0f, 1.0f, 1.0f), tcu::RGBA::blue().toVec(), -1));

	int refVertexIndex = 2;

	switch (m_topology)
	{
		case vk::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST:
			m_data.push_back(VertexElementData(tcu::Vec4(-0.3f,	-0.3f, 1.0f, 1.0f), tcu::RGBA::blue().toVec(), refVertexIndex++));
			m_data.push_back(VertexElementData(tcu::Vec4(-0.3f,	 0.3f, 1.0f, 1.0f), tcu::RGBA::blue().toVec(), refVertexIndex++));
			m_data.push_back(VertexElementData(tcu::Vec4( 0.3f,	-0.3f, 1.0f, 1.0f), tcu::RGBA::blue().toVec(), refVertexIndex++));
			m_data.push_back(VertexElementData(tcu::Vec4( 0.3f,	-0.3f, 1.0f, 1.0f), tcu::RGBA::blue().toVec(), refVertexIndex++));
			m_data.push_back(VertexElementData(tcu::Vec4( 0.3f,	 0.3f, 1.0f, 1.0f), tcu::RGBA::blue().toVec(), refVertexIndex++));
			m_data.push_back(VertexElementData(tcu::Vec4(-0.3f,	 0.3f, 1.0f, 1.0f), tcu::RGBA::blue().toVec(), refVertexIndex++));
			break;
		case vk::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP:
			m_data.push_back(VertexElementData(tcu::Vec4(-0.3f,	-0.3f, 1.0f, 1.0f), tcu::RGBA::blue().toVec(), refVertexIndex++));
			m_data.push_back(VertexElementData(tcu::Vec4(-0.3f,	 0.3f, 1.0f, 1.0f), tcu::RGBA::blue().toVec(), refVertexIndex++));
			m_data.push_back(VertexElementData(tcu::Vec4( 0.3f,	-0.3f, 1.0f, 1.0f), tcu::RGBA::blue().toVec(), refVertexIndex++));
			m_data.push_back(VertexElementData(tcu::Vec4( 0.3f,	 0.3f, 1.0f, 1.0f), tcu::RGBA::blue().toVec(), refVertexIndex++));
			m_data.push_back(VertexElementData(tcu::Vec4(-0.3f,	 0.3f, 1.0f, 1.0f), tcu::RGBA::blue().toVec(), refVertexIndex++));
			break;
		case vk::VK_PRIMITIVE_TOPOLOGY_POINT_LIST:
		case vk::VK_PRIMITIVE_TOPOLOGY_LINE_LIST:
		case vk::VK_PRIMITIVE_TOPOLOGY_LINE_STRIP:
		case vk::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN:
		case vk::VK_PRIMITIVE_TOPOLOGY_LINE_LIST_WITH_ADJACENCY:
		case vk::VK_PRIMITIVE_TOPOLOGY_LINE_STRIP_WITH_ADJACENCY:
		case vk::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY:
		case vk::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP_WITH_ADJACENCY:
		case vk::VK_PRIMITIVE_TOPOLOGY_PATCH_LIST:
		case vk::VK_PRIMITIVE_TOPOLOGY_LAST:
			DE_FATAL("Topology not implemented");
			break;
		default:
			DE_FATAL("Unknown topology");
			break;
	}

	m_data.push_back(VertexElementData(tcu::Vec4(-1.0f, 1.0f, 1.0f, 1.0f), tcu::RGBA::blue().toVec(), -1));

	initialize();
}

tcu::TestStatus SimpleDraw::iterate (void)
{
	tcu::TestLog &log							= m_context.getTestContext().getLog();
	const vk::VkQueue queue						= m_context.getUniversalQueue();

	beginRenderPass();

	const vk::VkDeviceSize vertexBufferOffset	= 0;
	const vk::VkBuffer vertexBuffer				= m_vertexBuffer->object();

	m_vk.cmdBindVertexBuffers(*m_cmdBuffer, 0, 1, &vertexBuffer, &vertexBufferOffset);
	m_vk.cmdBindPipeline(*m_cmdBuffer, vk::VK_PIPELINE_BIND_POINT_GRAPHICS, *m_pipeline);

	switch (m_topology)
	{
		case vk::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST:
			m_vk.cmdDraw(*m_cmdBuffer, 6, 1, 2, 0);
			break;
		case vk::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP:
			m_vk.cmdDraw(*m_cmdBuffer, 4, 1, 2, 0);
			break;
		case vk::VK_PRIMITIVE_TOPOLOGY_POINT_LIST:
		case vk::VK_PRIMITIVE_TOPOLOGY_LINE_LIST:
		case vk::VK_PRIMITIVE_TOPOLOGY_LINE_STRIP:
		case vk::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN:
		case vk::VK_PRIMITIVE_TOPOLOGY_LINE_LIST_WITH_ADJACENCY:
		case vk::VK_PRIMITIVE_TOPOLOGY_LINE_STRIP_WITH_ADJACENCY:
		case vk::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY:
		case vk::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP_WITH_ADJACENCY:
		case vk::VK_PRIMITIVE_TOPOLOGY_PATCH_LIST:
		case vk::VK_PRIMITIVE_TOPOLOGY_LAST:
			DE_FATAL("Topology not implemented");
			break;
		default:
			DE_FATAL("Unknown topology");
			break;
	}

	m_vk.cmdEndRenderPass(*m_cmdBuffer);
	m_vk.endCommandBuffer(*m_cmdBuffer);

	vk::VkSubmitInfo submitInfo =
	{
		vk::VK_STRUCTURE_TYPE_SUBMIT_INFO,	// VkStructureType			sType;
		DE_NULL,							// const void*				pNext;
		0,										// deUint32					waitSemaphoreCount;
		DE_NULL,								// const VkSemaphore*		pWaitSemaphores;
		(const vk::VkPipelineStageFlags*)DE_NULL,
		1,										// deUint32					commandBufferCount;
		&m_cmdBuffer.get(),					// const VkCommandBuffer*	pCommandBuffers;
		0,										// deUint32					signalSemaphoreCount;
		DE_NULL								// const VkSemaphore*		pSignalSemaphores;
	};
	VK_CHECK(m_vk.queueSubmit(queue, 1, &submitInfo, DE_NULL));

	VK_CHECK(m_vk.queueWaitIdle(queue));

	// Validation
	tcu::Texture2D referenceFrame(vk::mapVkFormat(m_colorAttachmentFormat), (int)(0.5 + WIDTH), (int)(0.5 + HEIGHT));

	referenceFrame.allocLevel(0);

	const deInt32 frameWidth	= referenceFrame.getWidth();
	const deInt32 frameHeight	= referenceFrame.getHeight();

	tcu::clear(referenceFrame.getLevel(0), tcu::Vec4(0.0f, 0.0f, 0.0f, 1.0f));

	ReferenceImageCoordinates refCoords;

	for (int y = 0; y < frameHeight; y++)
	{
		const float yCoord = (float)(y / (0.5*frameHeight)) - 1.0f;

		for (int x = 0; x < frameWidth; x++)
		{
			const float xCoord = (float)(x / (0.5*frameWidth)) - 1.0f;

			if ((yCoord >= refCoords.bottom	&&
				 yCoord <= refCoords.top	&&
				 xCoord >= refCoords.left	&&
				 xCoord <= refCoords.right))
				referenceFrame.getLevel(0).setPixel(tcu::Vec4(0.0f, 0.0f, 1.0f, 1.0f), x, y);
		}
	}

	const vk::VkOffset3D zeroOffset = { 0, 0, 0 };
	const tcu::ConstPixelBufferAccess renderedFrame = m_colorTargetImage->readSurface(queue, m_context.getDefaultAllocator(),
		vk::VK_IMAGE_LAYOUT_GENERAL, zeroOffset, WIDTH, HEIGHT, vk::VK_IMAGE_ASPECT_COLOR_BIT);

	qpTestResult res = QP_TEST_RESULT_PASS;

	if (!tcu::fuzzyCompare(log, "Result", "Image comparison result",
		referenceFrame.getLevel(0), renderedFrame, 0.05f,
		tcu::COMPARE_LOG_RESULT)) {
		res = QP_TEST_RESULT_FAIL;
	}

	return tcu::TestStatus(res, qpGetTestResultName(res));

}

SimpleDrawInstanced::SimpleDrawInstanced (Context &context, TestSpec testSpec)
	: SimpleDraw	(context, testSpec) {}

tcu::TestStatus SimpleDrawInstanced::iterate (void)
{
	tcu::TestLog &log		= m_context.getTestContext().getLog();

	const vk::VkQueue queue = m_context.getUniversalQueue();

	beginRenderPass();

	const vk::VkDeviceSize vertexBufferOffset = 0;
	const vk::VkBuffer vertexBuffer = m_vertexBuffer->object();

	m_vk.cmdBindVertexBuffers(*m_cmdBuffer, 0, 1, &vertexBuffer, &vertexBufferOffset);

	m_vk.cmdBindPipeline(*m_cmdBuffer, vk::VK_PIPELINE_BIND_POINT_GRAPHICS, *m_pipeline);

	switch (m_topology)
	{
		case vk::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST:
			m_vk.cmdDraw(*m_cmdBuffer, 6, 4, 2, 2);
			break;
		case vk::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP:
			m_vk.cmdDraw(*m_cmdBuffer, 4, 4, 2, 2);
			break;
		case vk::VK_PRIMITIVE_TOPOLOGY_POINT_LIST:
		case vk::VK_PRIMITIVE_TOPOLOGY_LINE_LIST:
		case vk::VK_PRIMITIVE_TOPOLOGY_LINE_STRIP:
		case vk::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN:
		case vk::VK_PRIMITIVE_TOPOLOGY_LINE_LIST_WITH_ADJACENCY:
		case vk::VK_PRIMITIVE_TOPOLOGY_LINE_STRIP_WITH_ADJACENCY:
		case vk::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY:
		case vk::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP_WITH_ADJACENCY:
		case vk::VK_PRIMITIVE_TOPOLOGY_PATCH_LIST:
		case vk::VK_PRIMITIVE_TOPOLOGY_LAST:
			DE_FATAL("Topology not implemented");
			break;
		default:
			DE_FATAL("Unknown topology");
			break;
	}

	m_vk.cmdEndRenderPass(*m_cmdBuffer);
	m_vk.endCommandBuffer(*m_cmdBuffer);

	vk::VkSubmitInfo submitInfo =
	{
		vk::VK_STRUCTURE_TYPE_SUBMIT_INFO,	// VkStructureType			sType;
		DE_NULL,							// const void*				pNext;
		0,										// deUint32					waitSemaphoreCount;
		DE_NULL,								// const VkSemaphore*		pWaitSemaphores;
		(const vk::VkPipelineStageFlags*)DE_NULL,
		1,										// deUint32					commandBufferCount;
		&m_cmdBuffer.get(),					// const VkCommandBuffer*	pCommandBuffers;
		0,										// deUint32					signalSemaphoreCount;
		DE_NULL								// const VkSemaphore*		pSignalSemaphores;
	};
	VK_CHECK(m_vk.queueSubmit(queue, 1, &submitInfo, DE_NULL));

	VK_CHECK(m_vk.queueWaitIdle(queue));

	// Validation
	VK_CHECK(m_vk.queueWaitIdle(queue));

	tcu::Texture2D referenceFrame(vk::mapVkFormat(m_colorAttachmentFormat), (int)(0.5 + WIDTH), (int)(0.5 + HEIGHT));

	referenceFrame.allocLevel(0);

	const deInt32 frameWidth	= referenceFrame.getWidth();
	const deInt32 frameHeight	= referenceFrame.getHeight();

	tcu::clear(referenceFrame.getLevel(0), tcu::Vec4(0.0f, 0.0f, 0.0f, 1.0f));

	ReferenceImageInstancedCoordinates refInstancedCoords;

	for (int y = 0; y < frameHeight; y++)
	{
		const float yCoord = (float)(y / (0.5*frameHeight)) - 1.0f;

		for (int x = 0; x < frameWidth; x++)
		{
			const float xCoord = (float)(x / (0.5*frameWidth)) - 1.0f;

			if ((yCoord >= refInstancedCoords.bottom	&&
				 yCoord <= refInstancedCoords.top		&&
				 xCoord >= refInstancedCoords.left		&&
				 xCoord <= refInstancedCoords.right))
				referenceFrame.getLevel(0).setPixel(tcu::Vec4(0.0f, 0.0f, 1.0f, 1.0f), x, y);
		}
	}

	const vk::VkOffset3D zeroOffset = { 0, 0, 0 };
	const tcu::ConstPixelBufferAccess renderedFrame = m_colorTargetImage->readSurface(queue, m_context.getDefaultAllocator(),
		vk::VK_IMAGE_LAYOUT_GENERAL, zeroOffset, WIDTH, HEIGHT, vk::VK_IMAGE_ASPECT_COLOR_BIT);

	qpTestResult res = QP_TEST_RESULT_PASS;

	if (!tcu::fuzzyCompare(log, "Result", "Image comparison result",
		referenceFrame.getLevel(0), renderedFrame, 0.05f,
		tcu::COMPARE_LOG_RESULT)) {
		res = QP_TEST_RESULT_FAIL;
	}

	return tcu::TestStatus(res, qpGetTestResultName(res));
}

}	// anonymous

SimpleDrawTests::SimpleDrawTests (tcu::TestContext &testCtx)
: TestCaseGroup	(testCtx, "simple_draw", "drawing simple geometry")
{
	/* Left blank on purpose */
}

SimpleDrawTests::~SimpleDrawTests (void) {}


void SimpleDrawTests::init (void)
{
	{
		SimpleDraw::TestSpec testSpec;
		testSpec.shaders[glu::SHADERTYPE_VERTEX] = "vulkan/draw/VertexFetch.vert";
		testSpec.shaders[glu::SHADERTYPE_FRAGMENT] = "vulkan/draw/VertexFetch.frag";

		testSpec.topology = vk::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		addChild(new InstanceFactory<SimpleDraw>(m_testCtx, "simple_draw_triangle_list", "Draws triangle list", testSpec));
		testSpec.topology = vk::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
		addChild(new InstanceFactory<SimpleDraw>(m_testCtx, "simple_draw_triangle_strip", "Draws triangle strip", testSpec));
	}
	{
		SimpleDrawInstanced::TestSpec testSpec;
		testSpec.shaders[glu::SHADERTYPE_VERTEX] = "vulkan/draw/VertexFetchInstancedFirstInstance.vert";
		testSpec.shaders[glu::SHADERTYPE_FRAGMENT] = "vulkan/draw/VertexFetch.frag";

		testSpec.topology = vk::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		addChild(new InstanceFactory<SimpleDrawInstanced>(m_testCtx, "simple_draw_instanced_triangle_list", "Draws an instanced triangle list", testSpec));
		testSpec.topology = vk::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
		addChild(new InstanceFactory<SimpleDrawInstanced>(m_testCtx, "simple_draw_instanced_triangle_strip", "Draws an instanced triangle strip", testSpec));
	}
}

}	// DrawTests
}	// vkt
