/*------------------------------------------------------------------------
 * Vulkan Conformance Tests
 * ------------------------
 *
 * Copyright (c) 2016 The Khronos Group Inc.
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
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
 * \brief Instanced Draw Tests
 *//*--------------------------------------------------------------------*/

#include "vktDrawInstancedTests.hpp"

#include "deSharedPtr.hpp"
#include "rrRenderer.hpp"
#include "tcuImageCompare.hpp"
#include "tcuRGBA.hpp"
#include "tcuTextureUtil.hpp"
#include "vkImageUtil.hpp"
#include "vkPrograms.hpp"
#include "vktDrawBufferObjectUtil.hpp"
#include "vktDrawCreateInfoUtil.hpp"
#include "vktDrawImageObjectUtil.hpp"
#include "vktDrawTestCaseUtil.hpp"

namespace vkt
{
namespace Draw
{
namespace
{

static const int	QUAD_GRID_SIZE	= 8;
static const int	WIDTH			= 128;
static const int	HEIGHT			= 128;

struct TestParams
{
	enum DrawFunction
	{
		FUNCTION_DRAW = 0,
		FUNCTION_DRAW_INDEXED,
		FUNCTION_DRAW_INDIRECT,
		FUNCTION_DRAW_INDEXED_INDIRECT,

		FUNTION_LAST
	};

	DrawFunction			function;
	vk::VkPrimitiveTopology	topology;
};

struct VertexPositionAndColor
{
				VertexPositionAndColor (tcu::Vec4 position_, tcu::Vec4 color_)
					: position	(position_)
					, color		(color_)
				{
				}

	tcu::Vec4	position;
	tcu::Vec4	color;
};

std::ostream & operator<<(std::ostream & str, TestParams const & v)
{
	std::ostringstream string;
	switch (v.function)
	{
		case TestParams::FUNCTION_DRAW:
			string << "draw";
			break;
		case TestParams::FUNCTION_DRAW_INDEXED:
			string << "draw_indexed";
			break;
		case TestParams::FUNCTION_DRAW_INDIRECT:
			string << "draw_indirect";
			break;
		case TestParams::FUNCTION_DRAW_INDEXED_INDIRECT:
			string << "draw_indexed_indirect";
			break;
		default:
			DE_ASSERT(false);
	}

	string << "_" << de::toString(v.topology);
	return str << string.str();
}

rr::PrimitiveType mapVkPrimitiveTopology (vk::VkPrimitiveTopology primitiveTopology)
{
	switch (primitiveTopology)
	{
		case vk::VK_PRIMITIVE_TOPOLOGY_POINT_LIST:						return rr::PRIMITIVETYPE_POINTS;
		case vk::VK_PRIMITIVE_TOPOLOGY_LINE_LIST:						return rr::PRIMITIVETYPE_LINES;
		case vk::VK_PRIMITIVE_TOPOLOGY_LINE_STRIP:						return rr::PRIMITIVETYPE_LINE_STRIP;
		case vk::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST:					return rr::PRIMITIVETYPE_TRIANGLES;
		case vk::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN:					return rr::PRIMITIVETYPE_TRIANGLE_FAN;
		case vk::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP:					return rr::PRIMITIVETYPE_TRIANGLE_STRIP;
		case vk::VK_PRIMITIVE_TOPOLOGY_LINE_LIST_WITH_ADJACENCY:		return rr::PRIMITIVETYPE_LINES_ADJACENCY;
		case vk::VK_PRIMITIVE_TOPOLOGY_LINE_STRIP_WITH_ADJACENCY:		return rr::PRIMITIVETYPE_LINE_STRIP_ADJACENCY;
		case vk::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY:	return rr::PRIMITIVETYPE_TRIANGLES_ADJACENCY;
		case vk::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP_WITH_ADJACENCY:	return rr::PRIMITIVETYPE_TRIANGLE_STRIP_ADJACENCY;
		default:
			DE_ASSERT(false);
	}
	return rr::PRIMITIVETYPE_LAST;
}

template<typename T>
de::SharedPtr<Buffer> createAndUploadBuffer(const std::vector<T> data, const vk::DeviceInterface& vk, const Context& context, vk::VkBufferUsageFlags usage)
{
	const vk::VkDeviceSize dataSize = data.size() * sizeof(T);
	de::SharedPtr<Buffer> buffer = Buffer::createAndAlloc(vk, context.getDevice(),
														  BufferCreateInfo(dataSize, usage),
														  context.getDefaultAllocator(),
														  vk::MemoryRequirement::HostVisible);

	deUint8* ptr = reinterpret_cast<deUint8*>(buffer->getBoundMemory().getHostPtr());

	deMemcpy(ptr, &data[0], static_cast<size_t>(dataSize));

	vk::flushMappedMemoryRange(vk, context.getDevice(),
							   buffer->getBoundMemory().getMemory(),
							   buffer->getBoundMemory().getOffset(),
							   VK_WHOLE_SIZE);
	return buffer;
}

class TestVertShader : public rr::VertexShader
{
public:
	TestVertShader (int numInstances, int firstInstance)
		: rr::VertexShader	(3, 1)
		, m_numInstances	(numInstances)
		, m_firstInstance	(firstInstance)
	{
		m_inputs[0].type	= rr::GENERICVECTYPE_FLOAT;
		m_inputs[1].type	= rr::GENERICVECTYPE_FLOAT;
		m_inputs[2].type	= rr::GENERICVECTYPE_FLOAT;
		m_outputs[0].type	= rr::GENERICVECTYPE_FLOAT;
	}

	void shadeVertices (const rr::VertexAttrib* inputs,
						rr::VertexPacket* const* packets,
						const int numPackets) const
	{
		for (int packetNdx = 0; packetNdx < numPackets; ++packetNdx)
		{
			const int		instanceNdx		= packets[packetNdx]->instanceNdx + m_firstInstance;
			const tcu::Vec4	position		= rr::readVertexAttribFloat(inputs[0], instanceNdx,	packets[packetNdx]->vertexNdx);
			const tcu::Vec4	color			= rr::readVertexAttribFloat(inputs[1], instanceNdx,	packets[packetNdx]->vertexNdx);
			const tcu::Vec4	color2			= rr::readVertexAttribFloat(inputs[2], instanceNdx, packets[packetNdx]->vertexNdx);
			packets[packetNdx]->position	= position + tcu::Vec4((float)(packets[packetNdx]->instanceNdx * 2.0 / m_numInstances), 0.0, 0.0, 0.0);
			packets[packetNdx]->outputs[0]	= color + tcu::Vec4((float)instanceNdx / (float)m_numInstances, 0.0, 0.0, 1.0) + color2;
		}
	}

private:
	const int m_numInstances;
	const int m_firstInstance;
};

class TestFragShader : public rr::FragmentShader
{
public:
	TestFragShader (void)
		: rr::FragmentShader(1, 1)
	{
		m_inputs[0].type	= rr::GENERICVECTYPE_FLOAT;
		m_outputs[0].type	= rr::GENERICVECTYPE_FLOAT;
	}

	void shadeFragments (rr::FragmentPacket* packets,
						 const int numPackets,
						 const rr::FragmentShadingContext& context) const
	{
		for (int packetNdx = 0; packetNdx < numPackets; ++packetNdx)
		{
			rr::FragmentPacket& packet = packets[packetNdx];
			for (int fragNdx = 0; fragNdx < rr::NUM_FRAGMENTS_PER_PACKET; ++fragNdx)
			{
				const tcu::Vec4 color = rr::readVarying<float>(packet, context, 0, fragNdx);
				rr::writeFragmentOutput(context, packetNdx, fragNdx, 0, color);
			}
		}
	}
};

class InstancedDrawInstance : public TestInstance
{
public:
												InstancedDrawInstance	(Context& context, TestParams params);
	virtual	tcu::TestStatus						iterate					(void);

private:
	void										prepareVertexData		(int instanceCount, int firstInstance);

	const TestParams							m_params;
	const vk::DeviceInterface&					m_vk;

	vk::VkFormat								m_colorAttachmentFormat;

	vk::Move<vk::VkPipeline>					m_pipeline;
	vk::Move<vk::VkPipelineLayout>				m_pipelineLayout;

	de::SharedPtr<Image>						m_colorTargetImage;
	vk::Move<vk::VkImageView>					m_colorTargetView;

	PipelineCreateInfo::VertexInputState		m_vertexInputState;

	vk::Move<vk::VkCommandPool>					m_cmdPool;
	vk::Move<vk::VkCommandBuffer>				m_cmdBuffer;

	vk::Move<vk::VkFramebuffer>					m_framebuffer;
	vk::Move<vk::VkRenderPass>					m_renderPass;

	// Vertex data
	std::vector<VertexPositionAndColor>			m_data;
	std::vector<deUint32>						m_indexes;
	std::vector<tcu::Vec4>						m_instancedColor;
};

class InstancedDrawCase : public TestCase
{
public:
	InstancedDrawCase (tcu::TestContext&	testCtx,
					   const std::string&	name,
					   const std::string&	desc,
					   TestParams			params)
		: TestCase	(testCtx, name, desc)
		, m_params	(params)
	{
		m_vertexShader = "#version 430\n"
				"layout(location = 0) in vec4 in_position;\n"
				"layout(location = 1) in vec4 in_color;\n"
				"layout(location = 2) in vec4 in_color_2;\n"
				"layout(push_constant) uniform TestParams {\n"
				"	float firstInstance;\n"
				"	float instanceCount;\n"
				"} params;\n"
				"layout(location = 0) out vec4 out_color;\n"
				"out gl_PerVertex {\n"
				"    vec4  gl_Position;\n"
				"    float gl_PointSize;\n"
				"};\n"
				"void main() {\n"
				"    gl_PointSize = 1.0;\n"
				"    gl_Position  = in_position + vec4(float(gl_InstanceIndex - params.firstInstance) * 2.0 / params.instanceCount, 0.0, 0.0, 0.0);\n"
				"    out_color    = in_color + vec4(float(gl_InstanceIndex) / params.instanceCount, 0.0, 0.0, 1.0) + in_color_2;\n"
				"}\n";

		m_fragmentShader = "#version 430\n"
				"layout(location = 0) in vec4 in_color;\n"
				"layout(location = 0) out vec4 out_color;\n"
				"void main()\n"
				"{\n"
				"    out_color = in_color;\n"
				"}\n";
	}

	TestInstance* createInstance (Context& context) const
	{
		return new InstancedDrawInstance(context, m_params);
	}

	virtual void initPrograms (vk::SourceCollections& programCollection) const
	{
		programCollection.glslSources.add("InstancedDrawVert") << glu::VertexSource(m_vertexShader);
		programCollection.glslSources.add("InstancedDrawFrag") << glu::FragmentSource(m_fragmentShader);
	}

private:
	const TestParams	m_params;
	std::string			m_vertexShader;
	std::string			m_fragmentShader;
};

InstancedDrawInstance::InstancedDrawInstance(Context &context, TestParams params)
	: TestInstance				(context)
	, m_params					(params)
	, m_vk						(context.getDeviceInterface())
	, m_colorAttachmentFormat	(vk::VK_FORMAT_R8G8B8A8_UNORM)
{
	const vk::VkDevice device				= m_context.getDevice();
	const deUint32 queueFamilyIndex			= m_context.getUniversalQueueFamilyIndex();

	const vk::VkPushConstantRange pushConstantRange = {
		vk::VK_SHADER_STAGE_VERTEX_BIT,				// VkShaderStageFlags    stageFlags;
		0u,											// uint32_t              offset;
		(deUint32)sizeof(float) * 2,				// uint32_t              size;
	};

	const PipelineLayoutCreateInfo pipelineLayoutCreateInfo(0, DE_NULL, 1, &pushConstantRange);
	m_pipelineLayout						= vk::createPipelineLayout(m_vk, device, &pipelineLayoutCreateInfo);

	const vk::VkExtent3D targetImageExtent	= { WIDTH, HEIGHT, 1 };
	const ImageCreateInfo targetImageCreateInfo(vk::VK_IMAGE_TYPE_2D, m_colorAttachmentFormat, targetImageExtent, 1, 1, vk::VK_SAMPLE_COUNT_1_BIT,
		vk::VK_IMAGE_TILING_OPTIMAL, vk::VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | vk::VK_IMAGE_USAGE_TRANSFER_SRC_BIT | vk::VK_IMAGE_USAGE_TRANSFER_DST_BIT);

	m_colorTargetImage						= Image::createAndAlloc(m_vk, device, targetImageCreateInfo, m_context.getDefaultAllocator(), m_context.getUniversalQueueFamilyIndex());

	const ImageViewCreateInfo colorTargetViewInfo(m_colorTargetImage->object(), vk::VK_IMAGE_VIEW_TYPE_2D, m_colorAttachmentFormat);
	m_colorTargetView						= vk::createImageView(m_vk, device, &colorTargetViewInfo);

	RenderPassCreateInfo renderPassCreateInfo;
	renderPassCreateInfo.addAttachment(AttachmentDescription(m_colorAttachmentFormat,
															 vk::VK_SAMPLE_COUNT_1_BIT,
															 vk::VK_ATTACHMENT_LOAD_OP_LOAD,
															 vk::VK_ATTACHMENT_STORE_OP_STORE,
															 vk::VK_ATTACHMENT_LOAD_OP_DONT_CARE,
															 vk::VK_ATTACHMENT_STORE_OP_STORE,
															 vk::VK_IMAGE_LAYOUT_GENERAL,
															 vk::VK_IMAGE_LAYOUT_GENERAL));

	const vk::VkAttachmentReference colorAttachmentReference =
	{
		0,
		vk::VK_IMAGE_LAYOUT_GENERAL
	};

	renderPassCreateInfo.addSubpass(SubpassDescription(vk::VK_PIPELINE_BIND_POINT_GRAPHICS,
													   0,
													   0,
													   DE_NULL,
													   1,
													   &colorAttachmentReference,
													   DE_NULL,
													   AttachmentReference(),
													   0,
													   DE_NULL));

	m_renderPass		= vk::createRenderPass(m_vk, device, &renderPassCreateInfo);

	std::vector<vk::VkImageView> colorAttachments(1);
	colorAttachments[0] = *m_colorTargetView;

	const FramebufferCreateInfo framebufferCreateInfo(*m_renderPass, colorAttachments, WIDTH, HEIGHT, 1);

	m_framebuffer		= vk::createFramebuffer(m_vk, device, &framebufferCreateInfo);

	const vk::VkVertexInputBindingDescription vertexInputBindingDescription[2] =
	{
		{
			0u,
			(deUint32)sizeof(VertexPositionAndColor),
			vk::VK_VERTEX_INPUT_RATE_VERTEX,
		},
		{
			1u,
			(deUint32)sizeof(tcu::Vec4),
			vk::VK_VERTEX_INPUT_RATE_INSTANCE,
		},
	};

	const vk::VkVertexInputAttributeDescription vertexInputAttributeDescriptions[] =
	{
		{
			0u,
			0u,
			vk::VK_FORMAT_R32G32B32A32_SFLOAT,
			0u
		},
		{
			1u,
			0u,
			vk::VK_FORMAT_R32G32B32A32_SFLOAT,
			(deUint32)sizeof(tcu::Vec4),
		},
		{
			2u,
			1u,
			vk::VK_FORMAT_R32G32B32A32_SFLOAT,
			0,
		}
	};

	m_vertexInputState = PipelineCreateInfo::VertexInputState(2,
															  vertexInputBindingDescription,
															  DE_LENGTH_OF_ARRAY(vertexInputAttributeDescriptions),
															  vertexInputAttributeDescriptions);

	const CmdPoolCreateInfo cmdPoolCreateInfo(queueFamilyIndex);
	m_cmdPool = vk::createCommandPool(m_vk, device, &cmdPoolCreateInfo);

	m_cmdBuffer = vk::allocateCommandBuffer(m_vk, device, *m_cmdPool, vk::VK_COMMAND_BUFFER_LEVEL_PRIMARY);

	const vk::Unique<vk::VkShaderModule> vs(createShaderModule(m_vk, device, m_context.getBinaryCollection().get("InstancedDrawVert"), 0));
	const vk::Unique<vk::VkShaderModule> fs(createShaderModule(m_vk, device, m_context.getBinaryCollection().get("InstancedDrawFrag"), 0));

	const PipelineCreateInfo::ColorBlendState::Attachment vkCbAttachmentState;

	vk::VkViewport viewport;
	viewport.x				= 0;
	viewport.y				= 0;
	viewport.width			= static_cast<float>(WIDTH);
	viewport.height			= static_cast<float>(HEIGHT);
	viewport.minDepth		= 0.0f;
	viewport.maxDepth		= 1.0f;

	vk::VkRect2D scissor;
	scissor.offset.x		= 0;
	scissor.offset.y		= 0;
	scissor.extent.width	= WIDTH;
	scissor.extent.height	= HEIGHT;

	PipelineCreateInfo pipelineCreateInfo(*m_pipelineLayout, *m_renderPass, 0, 0);
	pipelineCreateInfo.addShader(PipelineCreateInfo::PipelineShaderStage(*vs, "main", vk::VK_SHADER_STAGE_VERTEX_BIT));
	pipelineCreateInfo.addShader(PipelineCreateInfo::PipelineShaderStage(*fs, "main", vk::VK_SHADER_STAGE_FRAGMENT_BIT));
	pipelineCreateInfo.addState(PipelineCreateInfo::VertexInputState(m_vertexInputState));
	pipelineCreateInfo.addState(PipelineCreateInfo::InputAssemblerState(m_params.topology));
	pipelineCreateInfo.addState(PipelineCreateInfo::ColorBlendState(1, &vkCbAttachmentState));
	pipelineCreateInfo.addState(PipelineCreateInfo::ViewportState(1, std::vector<vk::VkViewport>(1, viewport), std::vector<vk::VkRect2D>(1, scissor)));
	pipelineCreateInfo.addState(PipelineCreateInfo::DepthStencilState());
	pipelineCreateInfo.addState(PipelineCreateInfo::RasterizerState());
	pipelineCreateInfo.addState(PipelineCreateInfo::MultiSampleState());

	m_pipeline = vk::createGraphicsPipeline(m_vk, device, DE_NULL, &pipelineCreateInfo);
}

tcu::TestStatus InstancedDrawInstance::iterate()
{
	const vk::VkQueue		queue					= m_context.getUniversalQueue();
	static const deUint32	instanceCounts[]		= { 0, 1, 2, 4, 20 };
	static const deUint32	firstInstanceIndices[]	= { 0, 1, 3, 4, 20 };

	qpTestResult			res						= QP_TEST_RESULT_PASS;

	const vk::VkClearColorValue clearColor = { { 0.0f, 0.0f, 0.0f, 1.0f } };
	const CmdBufferBeginInfo beginInfo;
	int firstInstanceIndicesCount = 1;

	// Require 'drawIndirectFirstInstance' feature to run non-zero firstInstance indirect draw tests.
	if (m_context.getDeviceFeatures().drawIndirectFirstInstance)
		firstInstanceIndicesCount = DE_LENGTH_OF_ARRAY(firstInstanceIndices);

	for (int instanceCountNdx = 0; instanceCountNdx < DE_LENGTH_OF_ARRAY(instanceCounts); instanceCountNdx++)
	{
		const deUint32 instanceCount = instanceCounts[instanceCountNdx];
		for (int firstInstanceIndexNdx = 0; firstInstanceIndexNdx < firstInstanceIndicesCount; firstInstanceIndexNdx++)
		{
			// Prepare vertex data for at least one instance
			const deUint32				prepareCount			= de::max(instanceCount, 1u);
			const deUint32				firstInstance			= firstInstanceIndices[firstInstanceIndexNdx];

			prepareVertexData(prepareCount, firstInstance);
			const de::SharedPtr<Buffer>	vertexBuffer			= createAndUploadBuffer(m_data, m_vk, m_context, vk::VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
			const de::SharedPtr<Buffer>	instancedVertexBuffer	= createAndUploadBuffer(m_instancedColor, m_vk, m_context, vk::VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
			de::SharedPtr<Buffer>		indexBuffer;
			de::SharedPtr<Buffer>		indirectBuffer;
			m_vk.beginCommandBuffer(*m_cmdBuffer, &beginInfo);

			initialTransitionColor2DImage(m_vk, *m_cmdBuffer, m_colorTargetImage->object(), vk::VK_IMAGE_LAYOUT_GENERAL,
										  vk::VK_ACCESS_TRANSFER_WRITE_BIT, vk::VK_PIPELINE_STAGE_TRANSFER_BIT);

			const ImageSubresourceRange subresourceRange(vk::VK_IMAGE_ASPECT_COLOR_BIT);
			m_vk.cmdClearColorImage(*m_cmdBuffer, m_colorTargetImage->object(),
				vk::VK_IMAGE_LAYOUT_GENERAL, &clearColor, 1, &subresourceRange);

			const vk::VkMemoryBarrier memBarrier =
			{
				vk::VK_STRUCTURE_TYPE_MEMORY_BARRIER,
				DE_NULL,
				vk::VK_ACCESS_TRANSFER_WRITE_BIT,
				vk::VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | vk::VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT
			};

			m_vk.cmdPipelineBarrier(*m_cmdBuffer, vk::VK_PIPELINE_STAGE_TRANSFER_BIT,
				vk::VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
				0, 1, &memBarrier, 0, DE_NULL, 0, DE_NULL);

			const vk::VkRect2D renderArea = { { 0, 0 }, { WIDTH, HEIGHT } };
			const RenderPassBeginInfo renderPassBegin(*m_renderPass, *m_framebuffer, renderArea);

			m_vk.cmdBeginRenderPass(*m_cmdBuffer, &renderPassBegin, vk::VK_SUBPASS_CONTENTS_INLINE);

			if (m_params.function == TestParams::FUNCTION_DRAW_INDEXED || m_params.function == TestParams::FUNCTION_DRAW_INDEXED_INDIRECT)
			{
				indexBuffer = createAndUploadBuffer(m_indexes, m_vk, m_context, vk::VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
				m_vk.cmdBindIndexBuffer(*m_cmdBuffer, indexBuffer->object(), 0, vk::VK_INDEX_TYPE_UINT32);
			}

			const vk::VkBuffer vertexBuffers[] =
			{
				vertexBuffer->object(),
				instancedVertexBuffer->object(),
			};

			const vk::VkDeviceSize vertexBufferOffsets[] =
			{
				0,	// vertexBufferOffset
				0,	// instancedVertexBufferOffset
			};

			m_vk.cmdBindVertexBuffers(*m_cmdBuffer, 0, DE_LENGTH_OF_ARRAY(vertexBuffers), vertexBuffers, vertexBufferOffsets);

			const float pushConstants[] = { (float)firstInstance, (float)instanceCount };
			m_vk.cmdPushConstants(*m_cmdBuffer, *m_pipelineLayout, vk::VK_SHADER_STAGE_VERTEX_BIT, 0u, (deUint32)sizeof(pushConstants), pushConstants);

			m_vk.cmdBindPipeline(*m_cmdBuffer, vk::VK_PIPELINE_BIND_POINT_GRAPHICS, *m_pipeline);

			switch (m_params.function)
			{
				case TestParams::FUNCTION_DRAW:
					m_vk.cmdDraw(*m_cmdBuffer, (deUint32)m_data.size(), instanceCount, 0u, firstInstance);
					break;

				case TestParams::FUNCTION_DRAW_INDEXED:
					m_vk.cmdDrawIndexed(*m_cmdBuffer, (deUint32)m_indexes.size(), instanceCount, 0u, 0u, firstInstance);
					break;

				case TestParams::FUNCTION_DRAW_INDIRECT:
				{
					vk::VkDrawIndirectCommand drawCommand =
					{
						(deUint32)m_data.size(),	// uint32_t	vertexCount;
						instanceCount,				// uint32_t	instanceCount;
						0u,							// uint32_t	firstVertex;
						firstInstance,				// uint32_t	firstInstance;
					};
					std::vector<vk::VkDrawIndirectCommand> drawCommands;
					drawCommands.push_back(drawCommand);
					indirectBuffer = createAndUploadBuffer(drawCommands, m_vk, m_context, vk::VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT);

					m_vk.cmdDrawIndirect(*m_cmdBuffer, indirectBuffer->object(), 0, 1u, 0u);
					break;
				}
				case TestParams::FUNCTION_DRAW_INDEXED_INDIRECT:
				{
					vk::VkDrawIndexedIndirectCommand drawCommand =
					{
						(deUint32)m_indexes.size(),	// uint32_t	indexCount;
						instanceCount,				// uint32_t	instanceCount;
						0u,							// uint32_t	firstIndex;
						0,							// int32_t	vertexOffset;
						firstInstance,				// uint32_t	firstInstance;
					};
					std::vector<vk::VkDrawIndexedIndirectCommand> drawCommands;
					drawCommands.push_back(drawCommand);
					indirectBuffer = createAndUploadBuffer(drawCommands, m_vk, m_context, vk::VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT);

					m_vk.cmdDrawIndexedIndirect(*m_cmdBuffer, indirectBuffer->object(), 0, 1u, 0u);
					break;
				}
				default:
					DE_ASSERT(false);
			}

			m_vk.cmdEndRenderPass(*m_cmdBuffer);
			m_vk.endCommandBuffer(*m_cmdBuffer);

			vk::VkSubmitInfo submitInfo =
			{
				vk::VK_STRUCTURE_TYPE_SUBMIT_INFO,			// VkStructureType				sType;
				DE_NULL,									// const void*					pNext;
				0,											// deUint32						waitSemaphoreCount;
				DE_NULL,									// const VkSemaphore*			pWaitSemaphores;
				(const vk::VkPipelineStageFlags*)DE_NULL,	// const VkPipelineStageFlags*	pWaitDstStageMask;
				1,											// deUint32						commandBufferCount;
				&m_cmdBuffer.get(),							// const VkCommandBuffer*		pCommandBuffers;
				0,											// deUint32						signalSemaphoreCount;
				DE_NULL										// const VkSemaphore*			pSignalSemaphores;
			};
			VK_CHECK(m_vk.queueSubmit(queue, 1, &submitInfo, DE_NULL));

			VK_CHECK(m_vk.queueWaitIdle(queue));

			// Reference rendering
			std::vector<tcu::Vec4>	vetrices;
			std::vector<tcu::Vec4>	colors;

			for (std::vector<VertexPositionAndColor>::const_iterator it = m_data.begin(); it != m_data.end(); ++it)
			{
				vetrices.push_back(it->position);
				colors.push_back(it->color);
			}

			tcu::TextureLevel refImage (vk::mapVkFormat(m_colorAttachmentFormat), (int)(0.5 + WIDTH), (int)(0.5 + HEIGHT));

			tcu::clear(refImage.getAccess(), tcu::Vec4(0.0f, 0.0f, 0.0f, 1.0f));

			const TestVertShader					vertShader(instanceCount, firstInstance);
			const TestFragShader					fragShader;
			const rr::Program						program			(&vertShader, &fragShader);
			const rr::MultisamplePixelBufferAccess	colorBuffer		= rr::MultisamplePixelBufferAccess::fromSinglesampleAccess(refImage.getAccess());
			const rr::RenderTarget					renderTarget	(colorBuffer);
			const rr::RenderState					renderState		((rr::ViewportState(colorBuffer)));
			const rr::Renderer						renderer;

			const rr::VertexAttrib	vertexAttribs[] =
			{
				rr::VertexAttrib(rr::VERTEXATTRIBTYPE_FLOAT, 4, sizeof(tcu::Vec4), 0, &vetrices[0]),
				rr::VertexAttrib(rr::VERTEXATTRIBTYPE_FLOAT, 4, sizeof(tcu::Vec4), 0, &colors[0]),
				rr::VertexAttrib(rr::VERTEXATTRIBTYPE_FLOAT, 4, sizeof(tcu::Vec4), 1, &m_instancedColor[0])
			};

			if (m_params.function == TestParams::FUNCTION_DRAW || m_params.function == TestParams::FUNCTION_DRAW_INDIRECT)
			{
				const rr::PrimitiveList	primitives = rr::PrimitiveList(mapVkPrimitiveTopology(m_params.topology), (int)vetrices.size(), 0);
				const rr::DrawCommand	command(renderState, renderTarget, program, DE_LENGTH_OF_ARRAY(vertexAttribs), &vertexAttribs[0],
												primitives);
				renderer.drawInstanced(command, instanceCount);
			}
			else
			{
				const rr::DrawIndices indicies(m_indexes.data());

				const rr::PrimitiveList	primitives = rr::PrimitiveList(mapVkPrimitiveTopology(m_params.topology), (int)m_indexes.size(), indicies);
				const rr::DrawCommand	command(renderState, renderTarget, program, DE_LENGTH_OF_ARRAY(vertexAttribs), &vertexAttribs[0],
												primitives);
				renderer.drawInstanced(command, instanceCount);
			}

			const vk::VkOffset3D zeroOffset = { 0, 0, 0 };
			const tcu::ConstPixelBufferAccess renderedFrame = m_colorTargetImage->readSurface(queue, m_context.getDefaultAllocator(),
				vk::VK_IMAGE_LAYOUT_GENERAL, zeroOffset, WIDTH, HEIGHT, vk::VK_IMAGE_ASPECT_COLOR_BIT);

			tcu::TestLog &log		= m_context.getTestContext().getLog();

			std::ostringstream resultDesc;
			resultDesc << "Image comparison result. Instance count: " << instanceCount << " first instance index: " << firstInstance;

			if (m_params.topology == vk::VK_PRIMITIVE_TOPOLOGY_POINT_LIST)
			{
				const bool ok = tcu::intThresholdPositionDeviationCompare(
					log, "Result", resultDesc.str().c_str(), refImage.getAccess(), renderedFrame,
					tcu::UVec4(4u),					// color threshold
					tcu::IVec3(1, 1, 0),			// position deviation tolerance
					true,							// don't check the pixels at the boundary
					tcu::COMPARE_LOG_RESULT);

				if (!ok)
					res = QP_TEST_RESULT_FAIL;
			}
			else
			{
				if (!tcu::fuzzyCompare(log, "Result", resultDesc.str().c_str(), refImage.getAccess(), renderedFrame, 0.05f, tcu::COMPARE_LOG_RESULT))
					res = QP_TEST_RESULT_FAIL;
			}
		}
	}
	return tcu::TestStatus(res, qpGetTestResultName(res));
}

void InstancedDrawInstance::prepareVertexData(int instanceCount, int firstInstance)
{
	m_data.clear();
	m_indexes.clear();
	m_instancedColor.clear();

	if (m_params.function == TestParams::FUNCTION_DRAW || m_params.function == TestParams::FUNCTION_DRAW_INDIRECT)
	{
		for (int y = 0; y < QUAD_GRID_SIZE; y++)
		{
			for (int x = 0; x < QUAD_GRID_SIZE; x++)
			{
				const float fx0 = -1.0f + (float)(x+0) / (float)QUAD_GRID_SIZE * 2.0f / (float)instanceCount;
				const float fx1 = -1.0f + (float)(x+1) / (float)QUAD_GRID_SIZE * 2.0f / (float)instanceCount;
				const float fy0 = -1.0f + (float)(y+0) / (float)QUAD_GRID_SIZE * 2.0f;
				const float fy1 = -1.0f + (float)(y+1) / (float)QUAD_GRID_SIZE * 2.0f;

				// Vertices of a quad's lower-left triangle: (fx0, fy0), (fx1, fy0) and (fx0, fy1)
				m_data.push_back(VertexPositionAndColor(tcu::Vec4(fx0, fy0, 1.0f, 1.0f), tcu::RGBA::blue().toVec()));
				m_data.push_back(VertexPositionAndColor(tcu::Vec4(fx1, fy0, 1.0f, 1.0f), tcu::RGBA::blue().toVec()));
				m_data.push_back(VertexPositionAndColor(tcu::Vec4(fx0, fy1, 1.0f, 1.0f), tcu::RGBA::green().toVec()));

				// Vertices of a quad's upper-right triangle: (fx1, fy1), (fx0, fy1) and (fx1, fy0)
				m_data.push_back(VertexPositionAndColor(tcu::Vec4(fx1, fy1, 1.0f, 1.0f), tcu::RGBA::green().toVec()));
				m_data.push_back(VertexPositionAndColor(tcu::Vec4(fx0, fy1, 1.0f, 1.0f), tcu::RGBA::green().toVec()));
				m_data.push_back(VertexPositionAndColor(tcu::Vec4(fx1, fy0, 1.0f, 1.0f), tcu::RGBA::blue().toVec()));
			}
		}
	}
	else
	{
		for (int y = 0; y < QUAD_GRID_SIZE + 1; y++)
		{
			for (int x = 0; x < QUAD_GRID_SIZE + 1; x++)
			{
				const float fx = -1.0f + (float)x / (float)QUAD_GRID_SIZE * 2.0f / (float)instanceCount;
				const float fy = -1.0f + (float)y / (float)QUAD_GRID_SIZE * 2.0f;

				m_data.push_back(VertexPositionAndColor(tcu::Vec4(fx, fy, 1.0f, 1.0f),
														(y % 2 ? tcu::RGBA::blue().toVec() : tcu::RGBA::green().toVec())));
			}
		}

		for (int y = 0; y < QUAD_GRID_SIZE; y++)
		{
			for (int x = 0; x < QUAD_GRID_SIZE; x++)
			{
				const int ndx00 = y*(QUAD_GRID_SIZE + 1) + x;
				const int ndx10 = y*(QUAD_GRID_SIZE + 1) + x + 1;
				const int ndx01 = (y + 1)*(QUAD_GRID_SIZE + 1) + x;
				const int ndx11 = (y + 1)*(QUAD_GRID_SIZE + 1) + x + 1;

				// Lower-left triangle of a quad.
				m_indexes.push_back((deUint16)ndx00);
				m_indexes.push_back((deUint16)ndx10);
				m_indexes.push_back((deUint16)ndx01);

				// Upper-right triangle of a quad.
				m_indexes.push_back((deUint16)ndx11);
				m_indexes.push_back((deUint16)ndx01);
				m_indexes.push_back((deUint16)ndx10);
			}
		}
	}

	for (int i = 0; i < instanceCount + firstInstance; i++)
	{
		m_instancedColor.push_back(tcu::Vec4(0.0, (float)(1.0 - i * 1.0 / (instanceCount + firstInstance)) / 2, 0.0, 1.0));
	}
}

} // anonymus

InstancedTests::InstancedTests(tcu::TestContext& testCtx)
	: TestCaseGroup	(testCtx, "instanced", "Instanced drawing tests")
{
	static const vk::VkPrimitiveTopology	topologies[]			=
	{
		vk::VK_PRIMITIVE_TOPOLOGY_POINT_LIST,
		vk::VK_PRIMITIVE_TOPOLOGY_LINE_LIST,
		vk::VK_PRIMITIVE_TOPOLOGY_LINE_STRIP,
		vk::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
		vk::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,
		vk::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN,
	};
	static const TestParams::DrawFunction	functions[]				=
	{
		TestParams::FUNCTION_DRAW,
		TestParams::FUNCTION_DRAW_INDEXED,
		TestParams::FUNCTION_DRAW_INDIRECT,
		TestParams::FUNCTION_DRAW_INDEXED_INDIRECT,
	};

	for (int topologyNdx = 0; topologyNdx < DE_LENGTH_OF_ARRAY(topologies); topologyNdx++)
	{
		for (int functionNdx = 0; functionNdx < DE_LENGTH_OF_ARRAY(functions); functionNdx++)
		{
			TestParams param;
			param.function = functions[functionNdx];
			param.topology = topologies[topologyNdx];

			std::string testName = de::toString(param);

			addChild(new InstancedDrawCase(m_testCtx, de::toLower(testName), "Instanced drawing test", param));
		}
	}
}

InstancedTests::~InstancedTests() {}

} // DrawTests
} // vkt
