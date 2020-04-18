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
 * \brief Simple Draw Tests
 *//*--------------------------------------------------------------------*/

#include "vktBasicDrawTests.hpp"

#include "vktDrawBaseClass.hpp"
#include "vkQueryUtil.hpp"
#include "vktTestGroupUtil.hpp"

#include "deDefs.h"
#include "deRandom.hpp"
#include "deString.h"

#include "tcuTestCase.hpp"
#include "tcuRGBA.hpp"
#include "tcuTextureUtil.hpp"
#include "tcuImageCompare.hpp"

#include "rrRenderer.hpp"

#include <string>
#include <sstream>

namespace vkt
{
namespace Draw
{
namespace
{
static const deUint32 SEED			= 0xc2a39fu;
static const deUint32 INDEX_LIMIT	= 10000;
// To avoid too big and mostly empty structures
static const deUint32 OFFSET_LIMIT	= 1000;
// Number of primitives to draw
static const deUint32 PRIMITIVE_COUNT[] = {1, 3, 17, 45};

enum DrawCommandType
{
	DRAW_COMMAND_TYPE_DRAW,
	DRAW_COMMAND_TYPE_DRAW_INDEXED,
	DRAW_COMMAND_TYPE_DRAW_INDIRECT,
	DRAW_COMMAND_TYPE_DRAW_INDEXED_INDIRECT,

	DRAW_COMMAND_TYPE_DRAW_LAST
};

const char* getDrawCommandTypeName (DrawCommandType command)
{
	switch (command)
	{
		case DRAW_COMMAND_TYPE_DRAW:					return "draw";
		case DRAW_COMMAND_TYPE_DRAW_INDEXED:			return "draw_indexed";
		case DRAW_COMMAND_TYPE_DRAW_INDIRECT:			return "draw_indirect";
		case DRAW_COMMAND_TYPE_DRAW_INDEXED_INDIRECT:	return "draw_indexed_indirect";
		default:					DE_ASSERT(false);
	}
	return "";
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

struct DrawParamsBase
{
	std::vector<PositionColorVertex>	vertices;
	vk::VkPrimitiveTopology				topology;

	DrawParamsBase ()
	{}

	DrawParamsBase (const vk::VkPrimitiveTopology top)
		: topology	(top)
	{}
};

struct IndexedParamsBase
{
	std::vector<deUint32>	indexes;
	const vk::VkIndexType	indexType;

	IndexedParamsBase (const vk::VkIndexType indexT)
		: indexType	(indexT)
	{}
};

// Structs to store draw parameters
struct DrawParams : DrawParamsBase
{
	// vkCmdDraw parameters is like a single VkDrawIndirectCommand
	vk::VkDrawIndirectCommand	params;

	DrawParams (const vk::VkPrimitiveTopology top, const deUint32 vertexC, const deUint32 instanceC, const deUint32 firstV, const deUint32 firstI)
		: DrawParamsBase	(top)
	{
		params.vertexCount		= vertexC;
		params.instanceCount	= instanceC;
		params.firstVertex		= firstV;
		params.firstInstance	= firstI;
	}
};

struct DrawIndexedParams : DrawParamsBase, IndexedParamsBase
{
	// vkCmdDrawIndexed parameters is like a single VkDrawIndexedIndirectCommand
	vk::VkDrawIndexedIndirectCommand	params;

	DrawIndexedParams (const vk::VkPrimitiveTopology top, const vk::VkIndexType indexT, const deUint32 indexC, const deUint32 instanceC, const deUint32 firstIdx, const deInt32 vertexO, const deUint32 firstIns)
		: DrawParamsBase	(top)
		, IndexedParamsBase	(indexT)
	{
		params.indexCount		= indexC;
		params.instanceCount	= instanceC;
		params.firstIndex		= firstIdx;
		params.vertexOffset		= vertexO;
		params.firstInstance	= firstIns;
	}
};

struct DrawIndirectParams : DrawParamsBase
{
	std::vector<vk::VkDrawIndirectCommand>	commands;

	DrawIndirectParams (const vk::VkPrimitiveTopology top)
		: DrawParamsBase	(top)
	{}

	void addCommand (const deUint32 vertexC, const deUint32 instanceC, const deUint32 firstV, const deUint32 firstI)
	{
		vk::VkDrawIndirectCommand	cmd;
		cmd.vertexCount				= vertexC;
		cmd.instanceCount			= instanceC;
		cmd.firstVertex				= firstV;
		cmd.firstInstance			= firstI;

		commands.push_back(cmd);
	}
};

struct DrawIndexedIndirectParams : DrawParamsBase, IndexedParamsBase
{
	std::vector<vk::VkDrawIndexedIndirectCommand>	commands;

	DrawIndexedIndirectParams (const vk::VkPrimitiveTopology top, const vk::VkIndexType indexT)
		: DrawParamsBase	(top)
		, IndexedParamsBase	(indexT)
	{}

	void addCommand (const deUint32 indexC, const deUint32 instanceC, const deUint32 firstIdx, const deInt32 vertexO, const deUint32 firstIns)
	{
		vk::VkDrawIndexedIndirectCommand	cmd;
		cmd.indexCount						= indexC;
		cmd.instanceCount					= instanceC;
		cmd.firstIndex						= firstIdx;
		cmd.vertexOffset					= vertexO;
		cmd.firstInstance					= firstIns;

		commands.push_back(cmd);
	}
};

// Reference renderer shaders
class PassthruVertShader : public rr::VertexShader
{
public:
	PassthruVertShader (void)
	: rr::VertexShader (2, 1)
	{
		m_inputs[0].type	= rr::GENERICVECTYPE_FLOAT;
		m_inputs[1].type	= rr::GENERICVECTYPE_FLOAT;
		m_outputs[0].type	= rr::GENERICVECTYPE_FLOAT;
	}

	void shadeVertices (const rr::VertexAttrib* inputs, rr::VertexPacket* const* packets, const int numPackets) const
	{
		for (int packetNdx = 0; packetNdx < numPackets; ++packetNdx)
		{
			packets[packetNdx]->position = rr::readVertexAttribFloat(inputs[0],
																	 packets[packetNdx]->instanceNdx,
																	 packets[packetNdx]->vertexNdx);

			tcu::Vec4 color = rr::readVertexAttribFloat(inputs[1],
														packets[packetNdx]->instanceNdx,
														packets[packetNdx]->vertexNdx);

			packets[packetNdx]->outputs[0] = color;
		}
	}
};

class PassthruFragShader : public rr::FragmentShader
{
public:
	PassthruFragShader (void)
		: rr::FragmentShader(1, 1)
	{
		m_inputs[0].type	= rr::GENERICVECTYPE_FLOAT;
		m_outputs[0].type	= rr::GENERICVECTYPE_FLOAT;
	}

	void shadeFragments (rr::FragmentPacket* packets, const int numPackets, const rr::FragmentShadingContext& context) const
	{
		for (int packetNdx = 0; packetNdx < numPackets; ++packetNdx)
		{
			rr::FragmentPacket& packet = packets[packetNdx];
			for (deUint32 fragNdx = 0; fragNdx < rr::NUM_FRAGMENTS_PER_PACKET; ++fragNdx)
			{
				tcu::Vec4 color = rr::readVarying<float>(packet, context, 0, fragNdx);
				rr::writeFragmentOutput(context, packetNdx, fragNdx, 0, color);
			}
		}
	}
};

inline bool imageCompare (tcu::TestLog& log, const tcu::ConstPixelBufferAccess& reference, const tcu::ConstPixelBufferAccess& result, const vk::VkPrimitiveTopology topology)
{
	if (topology == vk::VK_PRIMITIVE_TOPOLOGY_POINT_LIST)
	{
		return tcu::intThresholdPositionDeviationCompare(
			log, "Result", "Image comparison result", reference, result,
			tcu::UVec4(4u),					// color threshold
			tcu::IVec3(1, 1, 0),			// position deviation tolerance
			true,							// don't check the pixels at the boundary
			tcu::COMPARE_LOG_RESULT);
	}
	else
		return tcu::fuzzyCompare(log, "Result", "Image comparison result", reference, result, 0.053f, tcu::COMPARE_LOG_RESULT);
}

class DrawTestInstanceBase : public TestInstance
{
public:
									DrawTestInstanceBase	(Context& context);
	virtual							~DrawTestInstanceBase	(void) = 0;
	void							initialize				(const DrawParamsBase& data);
	void							initPipeline			(const vk::VkDevice device);
	void							beginRenderPass			(void);

	// Specialize this function for each type
	virtual tcu::TestStatus			iterate					(void) = 0;
protected:
	// Specialize this function for each type
	virtual void					generateDrawData		(void) = 0;
	void							generateRefImage		(const tcu::PixelBufferAccess& access, const std::vector<tcu::Vec4>& vertices, const std::vector<tcu::Vec4>& colors) const;

	DrawParamsBase											m_data;
	const vk::DeviceInterface&								m_vk;
	vk::Move<vk::VkPipeline>								m_pipeline;
	vk::Move<vk::VkPipelineLayout>							m_pipelineLayout;
	vk::VkFormat											m_colorAttachmentFormat;
	de::SharedPtr<Image>									m_colorTargetImage;
	vk::Move<vk::VkImageView>								m_colorTargetView;
	vk::Move<vk::VkRenderPass>								m_renderPass;
	vk::Move<vk::VkFramebuffer>								m_framebuffer;
	PipelineCreateInfo::VertexInputState					m_vertexInputState;
	de::SharedPtr<Buffer>									m_vertexBuffer;
	vk::Move<vk::VkCommandPool>								m_cmdPool;
	vk::Move<vk::VkCommandBuffer>							m_cmdBuffer;

	enum
	{
		WIDTH = 256,
		HEIGHT = 256
	};
};

DrawTestInstanceBase::DrawTestInstanceBase (Context& context)
	: vkt::TestInstance			(context)
	, m_vk						(context.getDeviceInterface())
	, m_colorAttachmentFormat	(vk::VK_FORMAT_R8G8B8A8_UNORM)
{
}

DrawTestInstanceBase::~DrawTestInstanceBase (void)
{
}

void DrawTestInstanceBase::initialize (const DrawParamsBase& data)
{
	m_data	= data;

	const vk::VkDevice	device				= m_context.getDevice();
	const deUint32		queueFamilyIndex	= m_context.getUniversalQueueFamilyIndex();

	const vk::VkPhysicalDeviceFeatures features = m_context.getDeviceFeatures();

	if (features.geometryShader == VK_FALSE &&
		(m_data.topology == vk::VK_PRIMITIVE_TOPOLOGY_LINE_LIST_WITH_ADJACENCY ||
		 m_data.topology == vk::VK_PRIMITIVE_TOPOLOGY_LINE_STRIP_WITH_ADJACENCY ||
		 m_data.topology == vk::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY ||
		 m_data.topology == vk::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP_WITH_ADJACENCY)
		)
	{
		TCU_THROW(NotSupportedError, "Geometry Not Supported");
	}

	const PipelineLayoutCreateInfo pipelineLayoutCreateInfo;
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

	const vk::VkVertexInputBindingDescription vertexInputBindingDescription =
	{
		0,
		(deUint32)sizeof(tcu::Vec4) * 2,
		vk::VK_VERTEX_INPUT_RATE_VERTEX,
	};

	const vk::VkVertexInputAttributeDescription vertexInputAttributeDescriptions[2] =
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
			(deUint32)(sizeof(float)* 4),
		}
	};

	m_vertexInputState = PipelineCreateInfo::VertexInputState(1,
															  &vertexInputBindingDescription,
															  2,
															  vertexInputAttributeDescriptions);

	const vk::VkDeviceSize dataSize = m_data.vertices.size() * sizeof(PositionColorVertex);
	m_vertexBuffer = Buffer::createAndAlloc(m_vk, device, BufferCreateInfo(dataSize,
		vk::VK_BUFFER_USAGE_VERTEX_BUFFER_BIT), m_context.getDefaultAllocator(), vk::MemoryRequirement::HostVisible);

	deUint8* ptr = reinterpret_cast<deUint8*>(m_vertexBuffer->getBoundMemory().getHostPtr());
	deMemcpy(ptr, &(m_data.vertices[0]), static_cast<size_t>(dataSize));

	vk::flushMappedMemoryRange(m_vk,
							   device,
							   m_vertexBuffer->getBoundMemory().getMemory(),
							   m_vertexBuffer->getBoundMemory().getOffset(),
							   VK_WHOLE_SIZE);

	const CmdPoolCreateInfo cmdPoolCreateInfo(queueFamilyIndex);
	m_cmdPool	= vk::createCommandPool(m_vk, device, &cmdPoolCreateInfo);
	m_cmdBuffer	= vk::allocateCommandBuffer(m_vk, device, *m_cmdPool, vk::VK_COMMAND_BUFFER_LEVEL_PRIMARY);

	initPipeline(device);
}

void DrawTestInstanceBase::initPipeline (const vk::VkDevice device)
{
	const vk::Unique<vk::VkShaderModule>	vs(createShaderModule(m_vk, device, m_context.getBinaryCollection().get("vert"), 0));
	const vk::Unique<vk::VkShaderModule>	fs(createShaderModule(m_vk, device, m_context.getBinaryCollection().get("frag"), 0));

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
	pipelineCreateInfo.addState(PipelineCreateInfo::InputAssemblerState(m_data.topology));
	pipelineCreateInfo.addState(PipelineCreateInfo::ColorBlendState(1, &vkCbAttachmentState));
	pipelineCreateInfo.addState(PipelineCreateInfo::ViewportState(1, std::vector<vk::VkViewport>(1, viewport), std::vector<vk::VkRect2D>(1, scissor)));
	pipelineCreateInfo.addState(PipelineCreateInfo::DepthStencilState());
	pipelineCreateInfo.addState(PipelineCreateInfo::RasterizerState());
	pipelineCreateInfo.addState(PipelineCreateInfo::MultiSampleState());

	m_pipeline = vk::createGraphicsPipeline(m_vk, device, DE_NULL, &pipelineCreateInfo);
}

void DrawTestInstanceBase::beginRenderPass (void)
{
	const vk::VkClearColorValue clearColor = { { 0.0f, 0.0f, 0.0f, 1.0f } };
	const CmdBufferBeginInfo beginInfo;

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
}

void DrawTestInstanceBase::generateRefImage (const tcu::PixelBufferAccess& access, const std::vector<tcu::Vec4>& vertices, const std::vector<tcu::Vec4>& colors) const
{
	const PassthruVertShader				vertShader;
	const PassthruFragShader				fragShader;
	const rr::Program						program			(&vertShader, &fragShader);
	const rr::MultisamplePixelBufferAccess	colorBuffer		= rr::MultisamplePixelBufferAccess::fromSinglesampleAccess(access);
	const rr::RenderTarget					renderTarget	(colorBuffer);
	const rr::RenderState					renderState		((rr::ViewportState(colorBuffer)));
	const rr::Renderer						renderer;

	const rr::VertexAttrib	vertexAttribs[] =
	{
		rr::VertexAttrib(rr::VERTEXATTRIBTYPE_FLOAT, 4, sizeof(tcu::Vec4), 0, &vertices[0]),
		rr::VertexAttrib(rr::VERTEXATTRIBTYPE_FLOAT, 4, sizeof(tcu::Vec4), 0, &colors[0])
	};

	renderer.draw(rr::DrawCommand(renderState,
								  renderTarget,
								  program,
								  DE_LENGTH_OF_ARRAY(vertexAttribs),
								  &vertexAttribs[0],
								  rr::PrimitiveList(mapVkPrimitiveTopology(m_data.topology), (deUint32)vertices.size(), 0)));
}

template<typename T>
class DrawTestInstance : public DrawTestInstanceBase
{
public:
							DrawTestInstance		(Context& context, const T& data);
	virtual					~DrawTestInstance		(void);
	virtual void			generateDrawData		(void);
	virtual tcu::TestStatus	iterate					(void);
private:
	T						m_data;
};

template<typename T>
DrawTestInstance<T>::DrawTestInstance (Context& context, const T& data)
	: DrawTestInstanceBase	(context)
	, m_data				(data)
{
	generateDrawData();
	initialize(m_data);
}

template<typename T>
DrawTestInstance<T>::~DrawTestInstance (void)
{
}

template<typename T>
void DrawTestInstance<T>::generateDrawData (void)
{
	DE_FATAL("Using the general case of this function is forbidden!");
}

template<typename T>
tcu::TestStatus DrawTestInstance<T>::iterate (void)
{
	DE_FATAL("Using the general case of this function is forbidden!");
	return tcu::TestStatus::fail("");
}

template<typename T>
class DrawTestCase : public TestCase
{
	public:
									DrawTestCase		(tcu::TestContext& context, const char* name, const char* desc, const T data);
									~DrawTestCase		(void);
	virtual	void					initPrograms		(vk::SourceCollections& programCollection) const;
	virtual void					initShaderSources	(void);
	virtual TestInstance*			createInstance		(Context& context) const;

private:
	T													m_data;
	std::string											m_vertShaderSource;
	std::string											m_fragShaderSource;
};

template<typename T>
DrawTestCase<T>::DrawTestCase (tcu::TestContext& context, const char* name, const char* desc, const T data)
	: vkt::TestCase	(context, name, desc)
	, m_data		(data)
{
	initShaderSources();
}

template<typename T>
DrawTestCase<T>::~DrawTestCase	(void)
{
}

template<typename T>
void DrawTestCase<T>::initPrograms (vk::SourceCollections& programCollection) const
{
	programCollection.glslSources.add("vert") << glu::VertexSource(m_vertShaderSource);
	programCollection.glslSources.add("frag") << glu::FragmentSource(m_fragShaderSource);
}

template<typename T>
void DrawTestCase<T>::initShaderSources (void)
{
	std::stringstream vertShader;
	vertShader	<< "#version 430\n"
				<< "layout(location = 0) in vec4 in_position;\n"
				<< "layout(location = 1) in vec4 in_color;\n"
				<< "layout(location = 0) out vec4 out_color;\n"

				<< "out gl_PerVertex {\n"
				<< "    vec4  gl_Position;\n"
				<< "    float gl_PointSize;\n"
				<< "};\n"
				<< "void main() {\n"
				<< "    gl_PointSize = 1.0;\n"
				<< "    gl_Position  = in_position;\n"
				<< "    out_color    = in_color;\n"
				<< "}\n";

	m_vertShaderSource = vertShader.str();

	std::stringstream fragShader;
	fragShader	<< "#version 430\n"
				<< "layout(location = 0) in vec4 in_color;\n"
				<< "layout(location = 0) out vec4 out_color;\n"
				<< "void main()\n"
				<< "{\n"
				<< "    out_color = in_color;\n"
				<< "}\n";

	m_fragShaderSource = fragShader.str();
}

template<typename T>
TestInstance* DrawTestCase<T>::createInstance (Context& context) const
{
	return new DrawTestInstance<T>(context, m_data);
}

// Specialized cases
template<>
void DrawTestInstance<DrawParams>::generateDrawData (void)
{
	de::Random		rnd			(SEED ^ m_data.params.firstVertex ^ m_data.params.vertexCount);

	const deUint32	vectorSize	= m_data.params.firstVertex + m_data.params.vertexCount;

	// Initialize the vector
	m_data.vertices = std::vector<PositionColorVertex>(vectorSize, PositionColorVertex(tcu::Vec4(0.0, 0.0, 0.0, 0.0), tcu::Vec4(0.0, 0.0, 0.0, 0.0)));

	// Fill only the used indexes
	for (deUint32 vertexIdx = m_data.params.firstVertex; vertexIdx < vectorSize; ++vertexIdx)
	{
		m_data.vertices[vertexIdx] = PositionColorVertex(
			tcu::Vec4(rnd.getFloat(-1.0, 1.0), rnd.getFloat(-1.0, 1.0), 1.0, 1.0),										// Coord
			tcu::Vec4(rnd.getFloat(0.0, 1.0), rnd.getFloat(0.0, 1.0), rnd.getFloat(0.0, 1.0), rnd.getFloat(0.0, 1.0)));	// Color
	}
}

template<>
tcu::TestStatus DrawTestInstance<DrawParams>::iterate (void)
{
	tcu::TestLog			&log				= m_context.getTestContext().getLog();
	const vk::VkQueue		queue				= m_context.getUniversalQueue();

	beginRenderPass();

	const vk::VkDeviceSize	vertexBufferOffset	= 0;
	const vk::VkBuffer		vertexBuffer		= m_vertexBuffer->object();

	m_vk.cmdBindVertexBuffers(*m_cmdBuffer, 0, 1, &vertexBuffer, &vertexBufferOffset);
	m_vk.cmdBindPipeline(*m_cmdBuffer, vk::VK_PIPELINE_BIND_POINT_GRAPHICS, *m_pipeline);
	m_vk.cmdDraw(*m_cmdBuffer, m_data.params.vertexCount, m_data.params.instanceCount, m_data.params.firstVertex, m_data.params.firstInstance);
	m_vk.cmdEndRenderPass(*m_cmdBuffer);
	m_vk.endCommandBuffer(*m_cmdBuffer);

	vk::VkSubmitInfo	submitInfo =
	{
		vk::VK_STRUCTURE_TYPE_SUBMIT_INFO,			// VkStructureType			sType;
		DE_NULL,									// const void*				pNext;
		0,											// deUint32					waitSemaphoreCount;
		DE_NULL,									// const VkSemaphore*		pWaitSemaphores;
		(const vk::VkPipelineStageFlags*)DE_NULL,
		1,											// deUint32					commandBufferCount;
		&m_cmdBuffer.get(),							// const VkCommandBuffer*	pCommandBuffers;
		0,											// deUint32					signalSemaphoreCount;
		DE_NULL										// const VkSemaphore*		pSignalSemaphores;
	};
	VK_CHECK(m_vk.queueSubmit(queue, 1, &submitInfo, DE_NULL));

	// Validation
	tcu::TextureLevel refImage (vk::mapVkFormat(m_colorAttachmentFormat), (int)(0.5 + WIDTH), (int)(0.5 + HEIGHT));
	tcu::clear(refImage.getAccess(), tcu::Vec4(0.0f, 0.0f, 0.0f, 1.0f));

	std::vector<tcu::Vec4>	vertices;
	std::vector<tcu::Vec4>	colors;

	for (std::vector<PositionColorVertex>::const_iterator vertex = m_data.vertices.begin() + m_data.params.firstVertex; vertex != m_data.vertices.end(); ++vertex)
	{
		vertices.push_back(vertex->position);
		colors.push_back(vertex->color);
	}
	generateRefImage(refImage.getAccess(), vertices, colors);

	VK_CHECK(m_vk.queueWaitIdle(queue));

	const vk::VkOffset3D zeroOffset = { 0, 0, 0 };
	const tcu::ConstPixelBufferAccess renderedFrame = m_colorTargetImage->readSurface(queue, m_context.getDefaultAllocator(),
		vk::VK_IMAGE_LAYOUT_GENERAL, zeroOffset, WIDTH, HEIGHT, vk::VK_IMAGE_ASPECT_COLOR_BIT);

	qpTestResult res = QP_TEST_RESULT_PASS;

	if (!imageCompare(log, refImage.getAccess(), renderedFrame, m_data.topology))
		res = QP_TEST_RESULT_FAIL;

	return tcu::TestStatus(res, qpGetTestResultName(res));
}

template<>
void DrawTestInstance<DrawIndexedParams>::generateDrawData (void)
{
	de::Random		rnd			(SEED ^ m_data.params.firstIndex ^ m_data.params.indexCount);
	const deUint32	indexSize	= m_data.params.firstIndex + m_data.params.indexCount;

	// Initialize the vector with zeros
	m_data.indexes = std::vector<deUint32>(indexSize, 0);

	deUint32		highestIndex	= 0;	// Store to highest index to calculate the vertices size
	// Fill the indexes from firstIndex
	for (deUint32 idx = 0; idx < m_data.params.indexCount; ++idx)
	{
		deUint32	vertexIdx	= rnd.getInt(m_data.params.vertexOffset, INDEX_LIMIT);
		highestIndex = (vertexIdx > highestIndex) ? vertexIdx : highestIndex;

		m_data.indexes[m_data.params.firstIndex + idx]	= vertexIdx;
	}

	// Fill up the vertex coordinates with zeros until the highestIndex including the vertexOffset
	m_data.vertices = std::vector<PositionColorVertex>(m_data.params.vertexOffset + highestIndex + 1, PositionColorVertex(tcu::Vec4(0.0, 0.0, 0.0, 0.0), tcu::Vec4(0.0, 0.0, 0.0, 0.0)));

	// Generate random vertex only where you have index pointing at
	for (std::vector<deUint32>::const_iterator indexIt = m_data.indexes.begin() + m_data.params.firstIndex; indexIt != m_data.indexes.end(); ++indexIt)
	{
		// Get iterator to the vertex position  with the vertexOffset
		std::vector<PositionColorVertex>::iterator vertexIt = m_data.vertices.begin() + m_data.params.vertexOffset + *indexIt;

		tcu::VecAccess<float, 4, 4>	positionAccess = vertexIt->position.xyzw();
		positionAccess = tcu::Vec4(rnd.getFloat(-1.0, 1.0), rnd.getFloat(-1.0, 1.0), 1.0, 1.0);

		tcu::VecAccess<float, 4, 4>	colorAccess = vertexIt->color.xyzw();
		colorAccess = tcu::Vec4(rnd.getFloat(0.0, 1.0), rnd.getFloat(0.0, 1.0), rnd.getFloat(0.0, 1.0), rnd.getFloat(0.0, 1.0));
	}
}

template<>
tcu::TestStatus DrawTestInstance<DrawIndexedParams>::iterate (void)
{
	tcu::TestLog				&log				= m_context.getTestContext().getLog();
	const vk::DeviceInterface&	vk					= m_context.getDeviceInterface();
	const vk::VkDevice			vkDevice			= m_context.getDevice();
	const deUint32				queueFamilyIndex	= m_context.getUniversalQueueFamilyIndex();
	const vk::VkQueue			queue				= m_context.getUniversalQueue();
	vk::Allocator&				allocator			= m_context.getDefaultAllocator();

	beginRenderPass();

	const vk::VkDeviceSize	vertexBufferOffset = 0;
	const vk::VkBuffer	vertexBuffer = m_vertexBuffer->object();

	m_vk.cmdBindPipeline(*m_cmdBuffer, vk::VK_PIPELINE_BIND_POINT_GRAPHICS, *m_pipeline);
	m_vk.cmdBindVertexBuffers(*m_cmdBuffer, 0, 1, &vertexBuffer, &vertexBufferOffset);

	const deUint32	bufferSize	= (deUint32)(m_data.indexes.size() * sizeof(deUint32));

	vk::Move<vk::VkBuffer>	indexBuffer;

	const vk::VkBufferCreateInfo	bufferCreateInfo =
	{
		vk::VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,	// VkStructureType		sType;
		DE_NULL,									// const void*			pNext;
		0u,											// VkBufferCreateFlags	flags;
		bufferSize,									// VkDeviceSize			size;
		vk::VK_BUFFER_USAGE_INDEX_BUFFER_BIT,		// VkBufferUsageFlags	usage;
		vk::VK_SHARING_MODE_EXCLUSIVE,				// VkSharingMode		sharingMode;
		1u,											// deUint32				queueFamilyIndexCount;
		&queueFamilyIndex,							// const deUint32*		pQueueFamilyIndices;
	};

	indexBuffer = createBuffer(vk, vkDevice, &bufferCreateInfo);

	de::MovePtr<vk::Allocation>	indexAlloc;

	indexAlloc = allocator.allocate(getBufferMemoryRequirements(vk, vkDevice, *indexBuffer), vk::MemoryRequirement::HostVisible);
	VK_CHECK(vk.bindBufferMemory(vkDevice, *indexBuffer, indexAlloc->getMemory(), indexAlloc->getOffset()));

	deMemcpy(indexAlloc->getHostPtr(), &(m_data.indexes[0]), bufferSize);

	vk::flushMappedMemoryRange(m_vk, vkDevice, indexAlloc->getMemory(), indexAlloc->getOffset(), bufferSize);

	m_vk.cmdBindIndexBuffer(*m_cmdBuffer, *indexBuffer, 0u, m_data.indexType);
	m_vk.cmdDrawIndexed(*m_cmdBuffer, m_data.params.indexCount, m_data.params.instanceCount, m_data.params.firstIndex, m_data.params.vertexOffset, m_data.params.firstInstance);
	m_vk.cmdEndRenderPass(*m_cmdBuffer);
	m_vk.endCommandBuffer(*m_cmdBuffer);

	vk::VkSubmitInfo	submitInfo =
	{
		vk::VK_STRUCTURE_TYPE_SUBMIT_INFO,			// VkStructureType			sType;
		DE_NULL,									// const void*				pNext;
		0,											// deUint32					waitSemaphoreCount;
		DE_NULL,									// const VkSemaphore*		pWaitSemaphores;
		(const vk::VkPipelineStageFlags*)DE_NULL,
		1,											// deUint32					commandBufferCount;
		&m_cmdBuffer.get(),							// const VkCommandBuffer*	pCommandBuffers;
		0,											// deUint32					signalSemaphoreCount;
		DE_NULL										// const VkSemaphore*		pSignalSemaphores;
	};
	VK_CHECK(m_vk.queueSubmit(queue, 1, &submitInfo, DE_NULL));

	// Validation
	tcu::TextureLevel	refImage	(vk::mapVkFormat(m_colorAttachmentFormat), (int)(0.5 + WIDTH), (int)(0.5 + HEIGHT));
	tcu::clear(refImage.getAccess(), tcu::Vec4(0.0f, 0.0f, 0.0f, 1.0f));

	std::vector<tcu::Vec4>	vertices;
	std::vector<tcu::Vec4>	colors;

	for (std::vector<deUint32>::const_iterator it = m_data.indexes.begin() + m_data.params.firstIndex; it != m_data.indexes.end(); ++it)
	{
		deUint32 idx = m_data.params.vertexOffset + *it;
		vertices.push_back(m_data.vertices[idx].position);
		colors.push_back(m_data.vertices[idx].color);
	}
	generateRefImage(refImage.getAccess(), vertices, colors);

	VK_CHECK(m_vk.queueWaitIdle(queue));

	const vk::VkOffset3D zeroOffset = { 0, 0, 0 };
	const tcu::ConstPixelBufferAccess renderedFrame = m_colorTargetImage->readSurface(queue, m_context.getDefaultAllocator(),
		vk::VK_IMAGE_LAYOUT_GENERAL, zeroOffset, WIDTH, HEIGHT, vk::VK_IMAGE_ASPECT_COLOR_BIT);

	qpTestResult res = QP_TEST_RESULT_PASS;

	if (!imageCompare(log, refImage.getAccess(), renderedFrame, m_data.topology))
		res = QP_TEST_RESULT_FAIL;

	return tcu::TestStatus(res, qpGetTestResultName(res));
}

template<>
void DrawTestInstance<DrawIndirectParams>::generateDrawData (void)
{
	de::Random	rnd(SEED ^ m_data.commands[0].vertexCount ^ m_data.commands[0].firstVertex);

	deUint32 lastIndex	= 0;

	// Find the interval which will be used
	for (std::vector<vk::VkDrawIndirectCommand>::const_iterator it = m_data.commands.begin(); it != m_data.commands.end(); ++it)
	{
		const deUint32	index = it->firstVertex + it->vertexCount;
		lastIndex	= (index > lastIndex) ? index : lastIndex;
	}

	// Initialize with zeros
	m_data.vertices = std::vector<PositionColorVertex>(lastIndex, PositionColorVertex(tcu::Vec4(0.0, 0.0, 0.0, 0.0), tcu::Vec4(0.0, 0.0, 0.0, 0.0)));

	// Generate random vertices only where necessary
	for (std::vector<vk::VkDrawIndirectCommand>::const_iterator it = m_data.commands.begin(); it != m_data.commands.end(); ++it)
	{
		std::vector<PositionColorVertex>::iterator vertexStart = m_data.vertices.begin() + it->firstVertex;

		for (deUint32 idx = 0; idx < it->vertexCount; ++idx)
		{
			std::vector<PositionColorVertex>::iterator vertexIt = vertexStart + idx;

			tcu::VecAccess<float, 4, 4> positionAccess = vertexIt->position.xyzw();
			positionAccess = tcu::Vec4(rnd.getFloat(-1.0, 1.0), rnd.getFloat(-1.0, 1.0), 1.0, 1.0);

			tcu::VecAccess<float, 4, 4> colorAccess = vertexIt->color.xyzw();
			colorAccess = tcu::Vec4(rnd.getFloat(0.0, 1.0), rnd.getFloat(0.0, 1.0), rnd.getFloat(0.0, 1.0), rnd.getFloat(0.0, 1.0));
		}
	}
}

template<>
tcu::TestStatus DrawTestInstance<DrawIndirectParams>::iterate (void)
{
	tcu::TestLog						&log				= m_context.getTestContext().getLog();
	const vk::DeviceInterface&			vk					= m_context.getDeviceInterface();
	const vk::VkDevice					vkDevice			= m_context.getDevice();
	vk::Allocator&						allocator			= m_context.getDefaultAllocator();
	const vk::VkQueue					queue				= m_context.getUniversalQueue();
	const deUint32						queueFamilyIndex	= m_context.getUniversalQueueFamilyIndex();
	const vk::VkPhysicalDeviceFeatures	features			= m_context.getDeviceFeatures();

	beginRenderPass();

	const vk::VkDeviceSize	vertexBufferOffset	= 0;
	const vk::VkBuffer		vertexBuffer		= m_vertexBuffer->object();

	m_vk.cmdBindVertexBuffers(*m_cmdBuffer, 0, 1, &vertexBuffer, &vertexBufferOffset);
	m_vk.cmdBindPipeline(*m_cmdBuffer, vk::VK_PIPELINE_BIND_POINT_GRAPHICS, *m_pipeline);

	vk::Move<vk::VkBuffer>		indirectBuffer;
	de::MovePtr<vk::Allocation>	indirectAlloc;

	{
		const vk::VkDeviceSize	indirectInfoSize	= m_data.commands.size() * sizeof(vk::VkDrawIndirectCommand);

		const vk::VkBufferCreateInfo	indirectCreateInfo =
		{
			vk::VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,	// VkStructureType		sType;
			DE_NULL,									// const void*			pNext;
			0u,											// VkBufferCreateFlags	flags;
			indirectInfoSize,							// VkDeviceSize			size;
			vk::VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT,	// VkBufferUsageFlags	usage;
			vk::VK_SHARING_MODE_EXCLUSIVE,				// VkSharingMode		sharingMode;
			1u,											// deUint32				queueFamilyIndexCount;
			&queueFamilyIndex,							// const deUint32*		pQueueFamilyIndices;
		};

		indirectBuffer	= createBuffer(vk, vkDevice, &indirectCreateInfo);
		indirectAlloc	= allocator.allocate(getBufferMemoryRequirements(vk, vkDevice, *indirectBuffer), vk::MemoryRequirement::HostVisible);
		VK_CHECK(vk.bindBufferMemory(vkDevice, *indirectBuffer, indirectAlloc->getMemory(), indirectAlloc->getOffset()));

		deMemcpy(indirectAlloc->getHostPtr(), &(m_data.commands[0]), (size_t)indirectInfoSize);

		vk::flushMappedMemoryRange(m_vk, vkDevice, indirectAlloc->getMemory(), indirectAlloc->getOffset(), indirectInfoSize);
	}

	// If multiDrawIndirect not supported execute single calls
	if (m_data.commands.size() > 1 && !(features.multiDrawIndirect))
	{
		for (deUint32 cmdIdx = 0; cmdIdx < m_data.commands.size(); ++cmdIdx)
		{
			const deUint32	offset	= (deUint32)(indirectAlloc->getOffset() + cmdIdx * sizeof(vk::VkDrawIndirectCommand));
			m_vk.cmdDrawIndirect(*m_cmdBuffer, *indirectBuffer, offset, 1, sizeof(vk::VkDrawIndirectCommand));
		}
	}
	else
	{
		m_vk.cmdDrawIndirect(*m_cmdBuffer, *indirectBuffer, indirectAlloc->getOffset(), (deUint32)m_data.commands.size(), sizeof(vk::VkDrawIndirectCommand));
	}

	m_vk.cmdEndRenderPass(*m_cmdBuffer);
	m_vk.endCommandBuffer(*m_cmdBuffer);

	vk::VkSubmitInfo	submitInfo =
	{
		vk::VK_STRUCTURE_TYPE_SUBMIT_INFO,			// VkStructureType			sType;
		DE_NULL,									// const void*				pNext;
		0,											// deUint32					waitSemaphoreCount;
		DE_NULL,									// const VkSemaphore*		pWaitSemaphores;
		(const vk::VkPipelineStageFlags*)DE_NULL,
		1,											// deUint32					commandBufferCount;
		&m_cmdBuffer.get(),							// const VkCommandBuffer*	pCommandBuffers;
		0,											// deUint32					signalSemaphoreCount;
		DE_NULL										// const VkSemaphore*		pSignalSemaphores;
	};
	VK_CHECK(m_vk.queueSubmit(queue, 1, &submitInfo, DE_NULL));

	// Validation
	tcu::TextureLevel refImage (vk::mapVkFormat(m_colorAttachmentFormat), (int)(0.5 + WIDTH), (int)(0.5 + HEIGHT));
	tcu::clear(refImage.getAccess(), tcu::Vec4(0.0f, 0.0f, 0.0f, 1.0f));

	for (std::vector<vk::VkDrawIndirectCommand>::const_iterator it = m_data.commands.begin(); it != m_data.commands.end(); ++it)
	{
		std::vector<tcu::Vec4>	vertices;
		std::vector<tcu::Vec4>	colors;

		std::vector<PositionColorVertex>::const_iterator	firstIt	= m_data.vertices.begin() + it->firstVertex;
		std::vector<PositionColorVertex>::const_iterator	lastIt	= firstIt + it->vertexCount;

		for (std::vector<PositionColorVertex>::const_iterator vertex = firstIt; vertex != lastIt; ++vertex)
		{
			vertices.push_back(vertex->position);
			colors.push_back(vertex->color);
		}
		generateRefImage(refImage.getAccess(), vertices, colors);
	}

	VK_CHECK(m_vk.queueWaitIdle(queue));

	const vk::VkOffset3D zeroOffset = { 0, 0, 0 };
	const tcu::ConstPixelBufferAccess renderedFrame = m_colorTargetImage->readSurface(queue, m_context.getDefaultAllocator(),
		vk::VK_IMAGE_LAYOUT_GENERAL, zeroOffset, WIDTH, HEIGHT, vk::VK_IMAGE_ASPECT_COLOR_BIT);

	qpTestResult res = QP_TEST_RESULT_PASS;

	if (!imageCompare(log, refImage.getAccess(), renderedFrame, m_data.topology))
		res = QP_TEST_RESULT_FAIL;

	return tcu::TestStatus(res, qpGetTestResultName(res));
}

template<>
void DrawTestInstance<DrawIndexedIndirectParams>::generateDrawData (void)
{
	de::Random		rnd			(SEED ^ m_data.commands[0].firstIndex ^ m_data.commands[0].indexCount);

	deUint32		lastIndex	= 0;

	// Get the maximum range of indexes
	for (std::vector<vk::VkDrawIndexedIndirectCommand>::const_iterator it = m_data.commands.begin(); it != m_data.commands.end(); ++it)
	{
		const deUint32	index		= it->firstIndex + it->indexCount;
						lastIndex	= (index > lastIndex) ? index : lastIndex;
	}

	// Initialize the vector with zeros
	m_data.indexes = std::vector<deUint32>(lastIndex, 0);

	deUint32	highestIndex	= 0;

	// Generate random indexes for the ranges
	for (std::vector<vk::VkDrawIndexedIndirectCommand>::const_iterator it = m_data.commands.begin(); it != m_data.commands.end(); ++it)
	{
		for (deUint32 idx = 0; idx < it->indexCount; ++idx)
		{
			const deUint32	vertexIdx	= rnd.getInt(it->vertexOffset, INDEX_LIMIT);
			const deUint32	maxIndex	= vertexIdx + it->vertexOffset;

			highestIndex = (maxIndex > highestIndex) ? maxIndex : highestIndex;
			m_data.indexes[it->firstIndex + idx] = vertexIdx;
		}
	}

	// Initialize the vertex vector
	m_data.vertices = std::vector<PositionColorVertex>(highestIndex + 1, PositionColorVertex(tcu::Vec4(0.0, 0.0, 0.0, 0.0), tcu::Vec4(0.0, 0.0, 0.0, 0.0)));

	// Generate random vertices in the used locations
	for (std::vector<vk::VkDrawIndexedIndirectCommand>::const_iterator cmdIt = m_data.commands.begin(); cmdIt != m_data.commands.end(); ++cmdIt)
	{
		deUint32	firstIdx	= cmdIt->firstIndex;
		deUint32	lastIdx		= firstIdx + cmdIt->indexCount;

		for (deUint32 idx = firstIdx; idx < lastIdx; ++idx)
		{
			std::vector<PositionColorVertex>::iterator	vertexIt = m_data.vertices.begin() + cmdIt->vertexOffset + m_data.indexes[idx];

			tcu::VecAccess<float, 4, 4> positionAccess = vertexIt->position.xyzw();
			positionAccess = tcu::Vec4(rnd.getFloat(-1.0, 1.0), rnd.getFloat(-1.0, 1.0), 1.0, 1.0);

			tcu::VecAccess<float, 4, 4> colorAccess = vertexIt->color.xyzw();
			colorAccess = tcu::Vec4(rnd.getFloat(0.0, 1.0), rnd.getFloat(0.0, 1.0), rnd.getFloat(0.0, 1.0), rnd.getFloat(0.0, 1.0));
		}
	}
}

template<>
tcu::TestStatus DrawTestInstance<DrawIndexedIndirectParams>::iterate (void)
{
	tcu::TestLog						&log				= m_context.getTestContext().getLog();
	const vk::DeviceInterface&			vk					= m_context.getDeviceInterface();
	const vk::VkDevice					vkDevice			= m_context.getDevice();
	const deUint32						queueFamilyIndex	= m_context.getUniversalQueueFamilyIndex();
	const vk::VkQueue					queue				= m_context.getUniversalQueue();
	vk::Allocator&						allocator			= m_context.getDefaultAllocator();
	const vk::VkPhysicalDeviceFeatures	features			= m_context.getDeviceFeatures();

	beginRenderPass();

	const vk::VkDeviceSize	vertexBufferOffset	= 0;
	const vk::VkBuffer		vertexBuffer		= m_vertexBuffer->object();

	m_vk.cmdBindPipeline(*m_cmdBuffer, vk::VK_PIPELINE_BIND_POINT_GRAPHICS, *m_pipeline);
	m_vk.cmdBindVertexBuffers(*m_cmdBuffer, 0, 1, &vertexBuffer, &vertexBufferOffset);

	vk::Move<vk::VkBuffer>		indirectBuffer;
	de::MovePtr<vk::Allocation>	indirectAlloc;

	{
		const vk::VkDeviceSize	indirectInfoSize	= m_data.commands.size() * sizeof(vk::VkDrawIndexedIndirectCommand);

		const vk::VkBufferCreateInfo	indirectCreateInfo =
		{
			vk::VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,	// VkStructureType		sType;
			DE_NULL,									// const void*			pNext;
			0u,											// VkBufferCreateFlags	flags;
			indirectInfoSize,							// VkDeviceSize			size;
			vk::VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT,	// VkBufferUsageFlags	usage;
			vk::VK_SHARING_MODE_EXCLUSIVE,				// VkSharingMode		sharingMode;
			1u,											// deUint32				queueFamilyIndexCount;
			&queueFamilyIndex,							// const deUint32*		pQueueFamilyIndices;
		};

		indirectBuffer	= createBuffer(vk, vkDevice, &indirectCreateInfo);
		indirectAlloc	= allocator.allocate(getBufferMemoryRequirements(vk, vkDevice, *indirectBuffer), vk::MemoryRequirement::HostVisible);
		VK_CHECK(vk.bindBufferMemory(vkDevice, *indirectBuffer, indirectAlloc->getMemory(), indirectAlloc->getOffset()));

		deMemcpy(indirectAlloc->getHostPtr(), &(m_data.commands[0]), (size_t)indirectInfoSize);

		vk::flushMappedMemoryRange(m_vk, vkDevice, indirectAlloc->getMemory(), indirectAlloc->getOffset(), indirectInfoSize);
	}

	const deUint32	bufferSize = (deUint32)(m_data.indexes.size() * sizeof(deUint32));

	vk::Move<vk::VkBuffer>			indexBuffer;

	const vk::VkBufferCreateInfo	bufferCreateInfo =
	{
		vk::VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,	// VkStructureType		sType;
		DE_NULL,									// const void*			pNext;
		0u,											// VkBufferCreateFlags	flags;
		bufferSize,									// VkDeviceSize			size;
		vk::VK_BUFFER_USAGE_INDEX_BUFFER_BIT,		// VkBufferUsageFlags	usage;
		vk::VK_SHARING_MODE_EXCLUSIVE,				// VkSharingMode		sharingMode;
		1u,											// deUint32				queueFamilyIndexCount;
		&queueFamilyIndex,							// const deUint32*		pQueueFamilyIndices;
	};

	indexBuffer = createBuffer(vk, vkDevice, &bufferCreateInfo);

	de::MovePtr<vk::Allocation>	indexAlloc;

	indexAlloc = allocator.allocate(getBufferMemoryRequirements(vk, vkDevice, *indexBuffer), vk::MemoryRequirement::HostVisible);
	VK_CHECK(vk.bindBufferMemory(vkDevice, *indexBuffer, indexAlloc->getMemory(), indexAlloc->getOffset()));

	deMemcpy(indexAlloc->getHostPtr(), &(m_data.indexes[0]), bufferSize);

	vk::flushMappedMemoryRange(m_vk, vkDevice, indexAlloc->getMemory(), indexAlloc->getOffset(), bufferSize);

	m_vk.cmdBindIndexBuffer(*m_cmdBuffer, *indexBuffer, 0u, m_data.indexType);

	// If multiDrawIndirect not supported execute single calls
	if (m_data.commands.size() > 1 && !(features.multiDrawIndirect))
	{
		for (deUint32 cmdIdx = 0; cmdIdx < m_data.commands.size(); ++cmdIdx)
		{
			const deUint32	offset	= (deUint32)(indirectAlloc->getOffset() + cmdIdx * sizeof(vk::VkDrawIndexedIndirectCommand));
			m_vk.cmdDrawIndexedIndirect(*m_cmdBuffer, *indirectBuffer, offset, 1, sizeof(vk::VkDrawIndexedIndirectCommand));
		}
	}
	else
	{
		m_vk.cmdDrawIndexedIndirect(*m_cmdBuffer, *indirectBuffer, indirectAlloc->getOffset(), (deUint32)m_data.commands.size(), sizeof(vk::VkDrawIndexedIndirectCommand));
	}

	m_vk.cmdEndRenderPass(*m_cmdBuffer);
	m_vk.endCommandBuffer(*m_cmdBuffer);

	vk::VkSubmitInfo	submitInfo =
	{
		vk::VK_STRUCTURE_TYPE_SUBMIT_INFO,			// VkStructureType			sType;
		DE_NULL,									// const void*				pNext;
		0,											// deUint32					waitSemaphoreCount;
		DE_NULL,									// const VkSemaphore*		pWaitSemaphores;
		(const vk::VkPipelineStageFlags*)DE_NULL,
		1,											// deUint32					commandBufferCount;
		&m_cmdBuffer.get(),							// const VkCommandBuffer*	pCommandBuffers;
		0,											// deUint32					signalSemaphoreCount;
		DE_NULL										// const VkSemaphore*		pSignalSemaphores;
	};
	VK_CHECK(m_vk.queueSubmit(queue, 1, &submitInfo, DE_NULL));

	// Validation
	tcu::TextureLevel refImage (vk::mapVkFormat(m_colorAttachmentFormat), (int)(0.5 + WIDTH), (int)(0.5 + HEIGHT));
	tcu::clear(refImage.getAccess(), tcu::Vec4(0.0f, 0.0f, 0.0f, 1.0f));

	for (std::vector<vk::VkDrawIndexedIndirectCommand>::const_iterator cmd = m_data.commands.begin(); cmd != m_data.commands.end(); ++cmd)
	{
		std::vector<tcu::Vec4>	vertices;
		std::vector<tcu::Vec4>	colors;

		for (deUint32 idx = 0; idx < cmd->indexCount; ++idx)
		{
			const deUint32 vertexIndex = cmd->vertexOffset + m_data.indexes[cmd->firstIndex + idx];
			vertices.push_back(m_data.vertices[vertexIndex].position);
			colors.push_back(m_data.vertices[vertexIndex].color);
		}
		generateRefImage(refImage.getAccess(), vertices, colors);
	}

	VK_CHECK(m_vk.queueWaitIdle(queue));

	const vk::VkOffset3D zeroOffset = { 0, 0, 0 };
	const tcu::ConstPixelBufferAccess renderedFrame = m_colorTargetImage->readSurface(queue, m_context.getDefaultAllocator(),
		vk::VK_IMAGE_LAYOUT_GENERAL, zeroOffset, WIDTH, HEIGHT, vk::VK_IMAGE_ASPECT_COLOR_BIT);

	qpTestResult res = QP_TEST_RESULT_PASS;

	if (!imageCompare(log, refImage.getAccess(), renderedFrame, m_data.topology))
		res = QP_TEST_RESULT_FAIL;

	return tcu::TestStatus(res, qpGetTestResultName(res));
}

typedef DrawTestCase<DrawParams>				DrawCase;
typedef DrawTestCase<DrawIndexedParams>			IndexedCase;
typedef DrawTestCase<DrawIndirectParams>		IndirectCase;
typedef DrawTestCase<DrawIndexedIndirectParams>	IndexedIndirectCase;

struct TestCaseParams
{
	const DrawCommandType			command;
	const vk::VkPrimitiveTopology	topology;

	TestCaseParams (const DrawCommandType cmd, const vk::VkPrimitiveTopology top)
		: command	(cmd)
		, topology	(top)
	{}
};

}	// anonymous

void populateSubGroup (tcu::TestCaseGroup* testGroup, const TestCaseParams caseParams)
{
	de::Random						rnd			(SEED ^ deStringHash(testGroup->getName()));
	tcu::TestContext&				testCtx		= testGroup->getTestContext();
	const DrawCommandType			command		= caseParams.command;
	const vk::VkPrimitiveTopology	topology	= caseParams.topology;

	for (deUint32 primitiveCountIdx = 0; primitiveCountIdx < DE_LENGTH_OF_ARRAY(PRIMITIVE_COUNT); ++primitiveCountIdx)
	{
		const deUint32 primitives = PRIMITIVE_COUNT[primitiveCountIdx];

		deUint32	multiplier	= 1;
		deUint32	offset		= 0;
		// Calculated by Vulkan 23.1
		switch (topology)
		{
			case vk::VK_PRIMITIVE_TOPOLOGY_POINT_LIST:													break;
			case vk::VK_PRIMITIVE_TOPOLOGY_LINE_LIST:						multiplier = 2;				break;
			case vk::VK_PRIMITIVE_TOPOLOGY_LINE_STRIP:													break;
			case vk::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST:					multiplier = 3;				break;
			case vk::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP:												break;
			case vk::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN:									offset = 1;	break;
			case vk::VK_PRIMITIVE_TOPOLOGY_LINE_LIST_WITH_ADJACENCY:		multiplier = 4;	offset = 1;	break;
			case vk::VK_PRIMITIVE_TOPOLOGY_LINE_STRIP_WITH_ADJACENCY:						offset = 1;	break;
			case vk::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY:	multiplier = 6;				break;
			case vk::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP_WITH_ADJACENCY:	multiplier = 2;				break;
			default:														DE_FATAL("Unsupported topology.");
		}

		const deUint32	vertexCount		= multiplier * primitives + offset;
		std::string		name			= de::toString(primitives);

		switch (command)
		{
			case DRAW_COMMAND_TYPE_DRAW:
			{
				deUint32	firstPrimitive	= rnd.getInt(0, primitives);
				deUint32	firstVertex		= multiplier * firstPrimitive;
				testGroup->addChild(new DrawCase(testCtx, name.c_str(), "vkCmdDraw testcase.",
					DrawParams(topology, vertexCount, 1, firstVertex, 0))
				);
				break;
			}
			case DRAW_COMMAND_TYPE_DRAW_INDEXED:
			{
				deUint32	firstIndex			= rnd.getInt(0, OFFSET_LIMIT);
				deUint32	vertexOffset		= rnd.getInt(0, OFFSET_LIMIT);
				testGroup->addChild(new IndexedCase(testCtx, name.c_str(), "vkCmdDrawIndexed testcase.",
					DrawIndexedParams(topology, vk::VK_INDEX_TYPE_UINT32, vertexCount, 1, firstIndex, vertexOffset, 0))
				);
				break;
			}
			case DRAW_COMMAND_TYPE_DRAW_INDIRECT:
			{
				deUint32	firstVertex		= rnd.getInt(0, OFFSET_LIMIT);

				DrawIndirectParams	params	= DrawIndirectParams(topology);

				params.addCommand(vertexCount, 1, 0, 0);
				testGroup->addChild(new IndirectCase(testCtx, (name + "_single_command").c_str(), "vkCmdDrawIndirect testcase.", params));

				params.addCommand(vertexCount, 1, firstVertex, 0);
				testGroup->addChild(new IndirectCase(testCtx, (name + "_multi_command").c_str(), "vkCmdDrawIndirect testcase.", params));
				break;
			}
			case DRAW_COMMAND_TYPE_DRAW_INDEXED_INDIRECT:
			{
				deUint32	firstIndex		= rnd.getInt(vertexCount, OFFSET_LIMIT);
				deUint32	vertexOffset	= rnd.getInt(vertexCount, OFFSET_LIMIT);

				DrawIndexedIndirectParams	params	= DrawIndexedIndirectParams(topology, vk::VK_INDEX_TYPE_UINT32);
				params.addCommand(vertexCount, 1, 0, 0, 0);
				testGroup->addChild(new IndexedIndirectCase(testCtx, (name + "_single_command").c_str(), "vkCmdDrawIndexedIndirect testcase.", params));

				params.addCommand(vertexCount, 1, firstIndex, vertexOffset, 0);
				testGroup->addChild(new IndexedIndirectCase(testCtx, (name + "_multi_command").c_str(), "vkCmdDrawIndexedIndirect testcase.", params));
				break;
			}
			default:
				DE_FATAL("Unsupported draw command.");
		}
	}
}

void createTopologyGroups (tcu::TestCaseGroup* testGroup, const DrawCommandType cmdType)
{
	for (deUint32 idx = 0; idx != vk::VK_PRIMITIVE_TOPOLOGY_PATCH_LIST; ++idx)
	{
		const vk::VkPrimitiveTopology	topology	= vk::VkPrimitiveTopology(idx);
		const std::string				groupName	= de::toLower(getPrimitiveTopologyName(topology)).substr(22);
		addTestGroup(testGroup, groupName, "Testcases with a specific topology.", populateSubGroup, TestCaseParams(cmdType, topology));
	}
}

void createDrawTests (tcu::TestCaseGroup* testGroup)
{
	for (deUint32 idx = 0; idx < DRAW_COMMAND_TYPE_DRAW_LAST; ++idx)
	{
		const DrawCommandType	command	= DrawCommandType(idx);
		addTestGroup(testGroup, getDrawCommandTypeName(command), "Group for testing a specific draw command.", createTopologyGroups, command);
	}
}

tcu::TestCaseGroup*	createBasicDrawTests (tcu::TestContext& testCtx)
{
	return createTestGroup(testCtx, "basic_draw", "Basic drawing tests", createDrawTests);
}

}	// DrawTests
}	// vkt
