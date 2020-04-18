/*------------------------------------------------------------------------
 * Vulkan Conformance Tests
 * ------------------------
 *
 * Copyright (c) 2016 The Khronos Group Inc.
 * Copyright (c) 2016 The Android Open Source Project
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
 * \brief Geometry Basic Class
 *//*--------------------------------------------------------------------*/

#include "vktGeometryBasicClass.hpp"

#include "vkDefs.hpp"
#include "vktTestCase.hpp"
#include "vktTestCaseUtil.hpp"
#include "vkImageUtil.hpp"
#include "vkPrograms.hpp"
#include "vkBuilderUtil.hpp"
#include "vkRefUtil.hpp"
#include "vkQueryUtil.hpp"
#include "vkMemUtil.hpp"

#include <string>

namespace vkt
{
namespace geometry
{
using namespace vk;
using tcu::IVec2;
using tcu::Vec4;
using tcu::TestStatus;
using tcu::TestContext;
using tcu::TestCaseGroup;
using de::MovePtr;
using std::string;
using std::vector;
using std::size_t;

static const int TEST_CANVAS_SIZE = 256;

GeometryExpanderRenderTestInstance::GeometryExpanderRenderTestInstance (Context&					context,
																		const VkPrimitiveTopology	primitiveType,
																		const char*					name)
	: TestInstance		(context)
	, m_primitiveType	(primitiveType)
	, m_name			(name)
{
	checkGeometryShaderSupport(context.getInstanceInterface(), context.getPhysicalDevice());
}

tcu::TestStatus GeometryExpanderRenderTestInstance::iterate (void)
{
	const DeviceInterface&			vk						= m_context.getDeviceInterface();
	const VkDevice					device					= m_context.getDevice();
	const deUint32					queueFamilyIndex		= m_context.getUniversalQueueFamilyIndex();
	const VkQueue					queue					= m_context.getUniversalQueue();
	Allocator&						memAlloc				= m_context.getDefaultAllocator();
	const IVec2						resolution				= IVec2(TEST_CANVAS_SIZE, TEST_CANVAS_SIZE);
	const VkFormat					colorFormat				= VK_FORMAT_R8G8B8A8_UNORM;
	const Image						colorAttachmentImage	(
																vk,
																device,
																memAlloc,
																makeImageCreateInfo(resolution, colorFormat, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT),
																MemoryRequirement::Any
															);
	const Unique<VkRenderPass>		renderPass				(makeRenderPass(vk, device, colorFormat));

	const Move<VkPipelineLayout>	pipelineLayout			(createPipelineLayout(vk, device));
	const VkImageSubresourceRange	colorSubRange			= makeImageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT, 0u, 1u, 0u, 1u);
	const Unique<VkImageView>		colorAttachmentView		(makeImageView(vk, device, *colorAttachmentImage, VK_IMAGE_VIEW_TYPE_2D, colorFormat, colorSubRange));
	const Unique<VkFramebuffer>		framebuffer				(makeFramebuffer(vk, device, *renderPass, *colorAttachmentView, resolution.x(), resolution.y(), 1u));
	const Unique<VkCommandPool>		cmdPool					(createCommandPool(vk, device, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, queueFamilyIndex));
	const Unique<VkCommandBuffer>	cmdBuffer				(allocateCommandBuffer(vk, device, *cmdPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY));

	const VkDeviceSize				vertexDataSizeBytes		= sizeInBytes(m_vertexPosData) + sizeInBytes(m_vertexAttrData);
	const deUint32					vertexPositionsOffset	= 0u;
	const deUint32					vertexAtrrOffset		= static_cast<deUint32>(sizeof(Vec4));
	const string					geometryShaderName		= (m_context.getBinaryCollection().contains("geometry_pointsize") && checkPointSize (m_context.getInstanceInterface(), m_context.getPhysicalDevice()))?
																"geometry_pointsize" : "geometry";

	const Unique<VkPipeline>		pipeline				(GraphicsPipelineBuilder()
																.setRenderSize			(resolution)
																.setShader				(vk, device, VK_SHADER_STAGE_VERTEX_BIT, m_context.getBinaryCollection().get("vertex"), DE_NULL)
																.setShader				(vk, device, VK_SHADER_STAGE_GEOMETRY_BIT, m_context.getBinaryCollection().get(geometryShaderName), DE_NULL)
																.setShader				(vk, device, VK_SHADER_STAGE_FRAGMENT_BIT, m_context.getBinaryCollection().get("fragment"), DE_NULL)
																.addVertexBinding		(makeVertexInputBindingDescription(0u, 2u*vertexAtrrOffset, VK_VERTEX_INPUT_RATE_VERTEX))
																.addVertexAttribute		(makeVertexInputAttributeDescription(0u, 0u, VK_FORMAT_R32G32B32A32_SFLOAT, vertexPositionsOffset))
																.addVertexAttribute		(makeVertexInputAttributeDescription(1u, 0u, VK_FORMAT_R32G32B32A32_SFLOAT, vertexAtrrOffset))
																.setPrimitiveTopology	(m_primitiveType)
																.build					(vk, device, *pipelineLayout, *renderPass));

	const VkDeviceSize				colorBufferSizeBytes	= resolution.x()*resolution.y() * tcu::getPixelSize(mapVkFormat(colorFormat));
	const Buffer					colorBuffer				(vk, device, memAlloc, makeBufferCreateInfo(colorBufferSizeBytes,
																VK_BUFFER_USAGE_TRANSFER_DST_BIT), MemoryRequirement::HostVisible);
	const Buffer					vertexBuffer			(vk, device, memAlloc, makeBufferCreateInfo(vertexDataSizeBytes,
																VK_BUFFER_USAGE_VERTEX_BUFFER_BIT ), MemoryRequirement::HostVisible);
	{
		const Allocation& alloc = vertexBuffer.getAllocation();
		struct DataVec4
		{
			Vec4 pos;
			Vec4 color;
		};

		DataVec4* const pData = static_cast<DataVec4*>(alloc.getHostPtr());
		for(int ndx = 0; ndx < m_numDrawVertices; ++ndx)
		{
			pData[ndx].pos = m_vertexPosData[ndx];
			pData[ndx].color = m_vertexAttrData[ndx];
		}
		flushMappedMemoryRange(vk, device, alloc.getMemory(), alloc.getOffset(), vertexDataSizeBytes);
		// No barrier needed, flushed memory is automatically visible
	}

	// Draw commands
	beginCommandBuffer(vk, *cmdBuffer);

	// Change color attachment image layout
	{
		const VkImageMemoryBarrier colorAttachmentLayoutBarrier = makeImageMemoryBarrier(
				(VkAccessFlags)0, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
				VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
				*colorAttachmentImage, colorSubRange);

		vk.cmdPipelineBarrier(*cmdBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0u,
						0u, DE_NULL, 0u, DE_NULL, 1u, &colorAttachmentLayoutBarrier);
	}

	// Begin render pass
	{
		const VkRect2D renderArea = {
				makeOffset2D(0, 0),
				makeExtent2D(resolution.x(), resolution.y()),
			};
			const tcu::Vec4 clearColor(0.0f, 0.0f, 0.0f, 1.0f);
			beginRenderPass(vk, *cmdBuffer, *renderPass, *framebuffer, renderArea, clearColor);
	}

	vk.cmdBindPipeline(*cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *pipeline);
	{
		const VkBuffer buffers[] = { vertexBuffer.get()};
		const VkDeviceSize offsets[] = { vertexPositionsOffset };
		vk.cmdBindVertexBuffers(*cmdBuffer, 0u, DE_LENGTH_OF_ARRAY(buffers), buffers, offsets);
	}

	bindDescriptorSets(vk, device, memAlloc, *cmdBuffer, *pipelineLayout);

	drawCommand (*cmdBuffer);
	endRenderPass(vk, *cmdBuffer);

	// Copy render result to a host-visible buffer
	{
		const VkImageMemoryBarrier colorAttachmentPreCopyBarrier = makeImageMemoryBarrier(
				VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT,
				VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				*colorAttachmentImage, colorSubRange);

		vk.cmdPipelineBarrier(*cmdBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0u,
				0u, DE_NULL, 0u, DE_NULL, 1u, &colorAttachmentPreCopyBarrier);
	}
	{
		const VkBufferImageCopy copyRegion = makeBufferImageCopy(makeExtent3D(resolution.x(), resolution.y(), 1), makeImageSubresourceLayers(VK_IMAGE_ASPECT_COLOR_BIT, 0u, 0u, 1u));
		vk.cmdCopyImageToBuffer(*cmdBuffer, *colorAttachmentImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, *colorBuffer, 1u, &copyRegion);
	}

	{
		const VkBufferMemoryBarrier postCopyBarrier = makeBufferMemoryBarrier(VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_HOST_READ_BIT, *colorBuffer, 0ull, colorBufferSizeBytes);

		vk.cmdPipelineBarrier(*cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_HOST_BIT, 0u,
			0u, DE_NULL, 1u, &postCopyBarrier, 0u, DE_NULL);
	}

	endCommandBuffer(vk, *cmdBuffer);
	submitCommandsAndWait(vk, device, queue, *cmdBuffer);

	{
		// Log the result image.
		const Allocation& colorBufferAlloc = colorBuffer.getAllocation();
		invalidateMappedMemoryRange(vk, device, colorBufferAlloc.getMemory(), colorBufferAlloc.getOffset(), colorBufferSizeBytes);
		const tcu::ConstPixelBufferAccess imagePixelAccess(mapVkFormat(colorFormat), resolution.x(), resolution.y(), 1, colorBufferAlloc.getHostPtr());

		if (!compareWithFileImage(m_context, imagePixelAccess, m_name))
			return TestStatus::fail("Fail");
	}

	return TestStatus::pass("Pass");
}

Move<VkPipelineLayout> GeometryExpanderRenderTestInstance::createPipelineLayout (const DeviceInterface& vk, const VkDevice device)
{
	return makePipelineLayout(vk, device);
}

void GeometryExpanderRenderTestInstance::drawCommand (const VkCommandBuffer& cmdBuffer)
{
	const DeviceInterface& vk = m_context.getDeviceInterface();
	vk.cmdDraw(cmdBuffer, static_cast<deUint32>(m_numDrawVertices), 1u, 0u, 0u);
}

} // geometry
} // vkt
