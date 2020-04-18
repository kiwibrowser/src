/*------------------------------------------------------------------------
 * Vulkan Conformance Tests
 * ------------------------
 *
 * Copyright (c) 2016 The Khronos Group Inc.
 * Copyright (c) 2014 The Android Open Source Project
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
 * \brief Scissor tests
 *//*--------------------------------------------------------------------*/

#include "vktFragmentOperationsScissorTests.hpp"
#include "vktFragmentOperationsScissorMultiViewportTests.hpp"
#include "vktTestCaseUtil.hpp"
#include "vktTestGroupUtil.hpp"
#include "vktFragmentOperationsMakeUtil.hpp"

#include "vkDefs.hpp"
#include "vkRefUtil.hpp"
#include "vkTypeUtil.hpp"
#include "vkMemUtil.hpp"
#include "vkPrograms.hpp"
#include "vkImageUtil.hpp"

#include "tcuTestLog.hpp"
#include "tcuVector.hpp"
#include "tcuImageCompare.hpp"

#include "deUniquePtr.hpp"
#include "deRandom.hpp"

namespace vkt
{
namespace FragmentOperations
{
using namespace vk;
using de::UniquePtr;
using de::MovePtr;
using tcu::Vec4;
using tcu::Vec2;
using tcu::IVec2;
using tcu::IVec4;

namespace
{

//! What primitives will be drawn by the test case.
enum TestPrimitive
{
	TEST_PRIMITIVE_POINTS,			//!< Many points.
	TEST_PRIMITIVE_LINES,			//!< Many short lines.
	TEST_PRIMITIVE_TRIANGLES,		//!< Many small triangles.
	TEST_PRIMITIVE_BIG_LINE,		//!< One line crossing the whole render area.
	TEST_PRIMITIVE_BIG_TRIANGLE,	//!< One triangle covering the whole render area.
};

struct VertexData
{
	Vec4	position;
	Vec4	color;
};

//! Parameters used by the test case.
struct CaseDef
{
	Vec4			renderArea;		//!< (ox, oy, w, h), where origin (0,0) is the top-left corner of the viewport. Width and height are in range [0, 1].
	Vec4			scissorArea;	//!< scissored area (ox, oy, w, h)
	TestPrimitive	primitive;
};

template<typename T>
inline VkDeviceSize sizeInBytes(const std::vector<T>& vec)
{
	return vec.size() * sizeof(vec[0]);
}

VkImageCreateInfo makeImageCreateInfo (const VkFormat format, const IVec2& size, VkImageUsageFlags usage)
{
	const VkImageCreateInfo imageParams =
	{
		VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,			// VkStructureType			sType;
		DE_NULL,										// const void*				pNext;
		(VkImageCreateFlags)0,							// VkImageCreateFlags		flags;
		VK_IMAGE_TYPE_2D,								// VkImageType				imageType;
		format,											// VkFormat					format;
		makeExtent3D(size.x(), size.y(), 1),			// VkExtent3D				extent;
		1u,												// deUint32					mipLevels;
		1u,												// deUint32					arrayLayers;
		VK_SAMPLE_COUNT_1_BIT,							// VkSampleCountFlagBits	samples;
		VK_IMAGE_TILING_OPTIMAL,						// VkImageTiling			tiling;
		usage,											// VkImageUsageFlags		usage;
		VK_SHARING_MODE_EXCLUSIVE,						// VkSharingMode			sharingMode;
		0u,												// deUint32					queueFamilyIndexCount;
		DE_NULL,										// const deUint32*			pQueueFamilyIndices;
		VK_IMAGE_LAYOUT_UNDEFINED,						// VkImageLayout			initialLayout;
	};
	return imageParams;
}

//! A single-attachment, single-subpass render pass.
Move<VkRenderPass> makeRenderPass (const DeviceInterface&	vk,
								   const VkDevice			device,
								   const VkFormat			colorFormat)
{
	const VkAttachmentDescription colorAttachmentDescription =
	{
		(VkAttachmentDescriptionFlags)0,					// VkAttachmentDescriptionFlags		flags;
		colorFormat,										// VkFormat							format;
		VK_SAMPLE_COUNT_1_BIT,								// VkSampleCountFlagBits			samples;
		VK_ATTACHMENT_LOAD_OP_CLEAR,						// VkAttachmentLoadOp				loadOp;
		VK_ATTACHMENT_STORE_OP_STORE,						// VkAttachmentStoreOp				storeOp;
		VK_ATTACHMENT_LOAD_OP_DONT_CARE,					// VkAttachmentLoadOp				stencilLoadOp;
		VK_ATTACHMENT_STORE_OP_DONT_CARE,					// VkAttachmentStoreOp				stencilStoreOp;
		VK_IMAGE_LAYOUT_UNDEFINED,							// VkImageLayout					initialLayout;
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,			// VkImageLayout					finalLayout;
	};

	const VkAttachmentReference colorAttachmentRef =
	{
		0u,													// deUint32			attachment;
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL			// VkImageLayout	layout;
	};

	const VkSubpassDescription subpassDescription =
	{
		(VkSubpassDescriptionFlags)0,						// VkSubpassDescriptionFlags		flags;
		VK_PIPELINE_BIND_POINT_GRAPHICS,					// VkPipelineBindPoint				pipelineBindPoint;
		0u,													// deUint32							inputAttachmentCount;
		DE_NULL,											// const VkAttachmentReference*		pInputAttachments;
		1u,													// deUint32							colorAttachmentCount;
		&colorAttachmentRef,								// const VkAttachmentReference*		pColorAttachments;
		DE_NULL,											// const VkAttachmentReference*		pResolveAttachments;
		DE_NULL,											// const VkAttachmentReference*		pDepthStencilAttachment;
		0u,													// deUint32							preserveAttachmentCount;
		DE_NULL												// const deUint32*					pPreserveAttachments;
	};

	const VkRenderPassCreateInfo renderPassInfo =
	{
		VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,			// VkStructureType					sType;
		DE_NULL,											// const void*						pNext;
		(VkRenderPassCreateFlags)0,							// VkRenderPassCreateFlags			flags;
		1u,													// deUint32							attachmentCount;
		&colorAttachmentDescription,						// const VkAttachmentDescription*	pAttachments;
		1u,													// deUint32							subpassCount;
		&subpassDescription,								// const VkSubpassDescription*		pSubpasses;
		0u,													// deUint32							dependencyCount;
		DE_NULL												// const VkSubpassDependency*		pDependencies;
	};

	return createRenderPass(vk, device, &renderPassInfo);
}

Move<VkPipeline> makeGraphicsPipeline (const DeviceInterface&		vk,
									   const VkDevice				device,
									   const VkPipelineLayout		pipelineLayout,
									   const VkRenderPass			renderPass,
									   const VkShaderModule			vertexModule,
									   const VkShaderModule			fragmentModule,
									   const IVec2					renderSize,
									   const IVec4					scissorArea,	//!< (ox, oy, w, h)
									   const VkPrimitiveTopology	topology)
{
	const VkVertexInputBindingDescription vertexInputBindingDescription =
	{
		0u,								// uint32_t				binding;
		sizeof(VertexData),				// uint32_t				stride;
		VK_VERTEX_INPUT_RATE_VERTEX,	// VkVertexInputRate	inputRate;
	};

	const VkVertexInputAttributeDescription vertexInputAttributeDescriptions[] =
	{
		{
			0u,									// uint32_t			location;
			0u,									// uint32_t			binding;
			VK_FORMAT_R32G32B32A32_SFLOAT,		// VkFormat			format;
			0u,									// uint32_t			offset;
		},
		{
			1u,									// uint32_t			location;
			0u,									// uint32_t			binding;
			VK_FORMAT_R32G32B32A32_SFLOAT,		// VkFormat			format;
			sizeof(Vec4),						// uint32_t			offset;
		},
	};

	const VkPipelineVertexInputStateCreateInfo vertexInputStateInfo =
	{
		VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,		// VkStructureType                             sType;
		DE_NULL,														// const void*                                 pNext;
		(VkPipelineVertexInputStateCreateFlags)0,						// VkPipelineVertexInputStateCreateFlags       flags;
		1u,																// uint32_t                                    vertexBindingDescriptionCount;
		&vertexInputBindingDescription,									// const VkVertexInputBindingDescription*      pVertexBindingDescriptions;
		DE_LENGTH_OF_ARRAY(vertexInputAttributeDescriptions),			// uint32_t                                    vertexAttributeDescriptionCount;
		vertexInputAttributeDescriptions,								// const VkVertexInputAttributeDescription*    pVertexAttributeDescriptions;
	};

	const VkPipelineInputAssemblyStateCreateInfo pipelineInputAssemblyStateInfo =
	{
		VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,	// VkStructureType                             sType;
		DE_NULL,														// const void*                                 pNext;
		(VkPipelineInputAssemblyStateCreateFlags)0,						// VkPipelineInputAssemblyStateCreateFlags     flags;
		topology,														// VkPrimitiveTopology                         topology;
		VK_FALSE,														// VkBool32                                    primitiveRestartEnable;
	};

	const VkViewport viewport = makeViewport(
		0.0f, 0.0f,
		static_cast<float>(renderSize.x()), static_cast<float>(renderSize.y()),
		0.0f, 1.0f);

	const VkRect2D scissor = {
		makeOffset2D(scissorArea.x(), scissorArea.y()),
		makeExtent2D(static_cast<deUint32>(scissorArea.z()), static_cast<deUint32>(scissorArea.w())),
	};

	const VkPipelineViewportStateCreateInfo pipelineViewportStateInfo =
	{
		VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,			// VkStructureType                             sType;
		DE_NULL,														// const void*                                 pNext;
		(VkPipelineViewportStateCreateFlags)0,							// VkPipelineViewportStateCreateFlags          flags;
		1u,																// uint32_t                                    viewportCount;
		&viewport,														// const VkViewport*                           pViewports;
		1u,																// uint32_t                                    scissorCount;
		&scissor,														// const VkRect2D*                             pScissors;
	};

	const VkPipelineRasterizationStateCreateInfo pipelineRasterizationStateInfo =
	{
		VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,		// VkStructureType                          sType;
		DE_NULL,														// const void*                              pNext;
		(VkPipelineRasterizationStateCreateFlags)0,						// VkPipelineRasterizationStateCreateFlags  flags;
		VK_FALSE,														// VkBool32                                 depthClampEnable;
		VK_FALSE,														// VkBool32                                 rasterizerDiscardEnable;
		VK_POLYGON_MODE_FILL,											// VkPolygonMode							polygonMode;
		VK_CULL_MODE_NONE,												// VkCullModeFlags							cullMode;
		VK_FRONT_FACE_COUNTER_CLOCKWISE,								// VkFrontFace								frontFace;
		VK_FALSE,														// VkBool32									depthBiasEnable;
		0.0f,															// float									depthBiasConstantFactor;
		0.0f,															// float									depthBiasClamp;
		0.0f,															// float									depthBiasSlopeFactor;
		1.0f,															// float									lineWidth;
	};

	const VkPipelineMultisampleStateCreateInfo pipelineMultisampleStateInfo =
	{
		VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,		// VkStructureType							sType;
		DE_NULL,														// const void*								pNext;
		(VkPipelineMultisampleStateCreateFlags)0,						// VkPipelineMultisampleStateCreateFlags	flags;
		VK_SAMPLE_COUNT_1_BIT,											// VkSampleCountFlagBits					rasterizationSamples;
		VK_FALSE,														// VkBool32									sampleShadingEnable;
		0.0f,															// float									minSampleShading;
		DE_NULL,														// const VkSampleMask*						pSampleMask;
		VK_FALSE,														// VkBool32									alphaToCoverageEnable;
		VK_FALSE														// VkBool32									alphaToOneEnable;
	};

	const VkStencilOpState stencilOpState = makeStencilOpState(
		VK_STENCIL_OP_KEEP,				// stencil fail
		VK_STENCIL_OP_KEEP,				// depth & stencil pass
		VK_STENCIL_OP_KEEP,				// depth only fail
		VK_COMPARE_OP_ALWAYS,			// compare op
		0u,								// compare mask
		0u,								// write mask
		0u);							// reference

	VkPipelineDepthStencilStateCreateInfo pipelineDepthStencilStateInfo =
	{
		VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,		// VkStructureType							sType;
		DE_NULL,														// const void*								pNext;
		(VkPipelineDepthStencilStateCreateFlags)0,						// VkPipelineDepthStencilStateCreateFlags	flags;
		VK_FALSE,														// VkBool32									depthTestEnable;
		VK_FALSE,														// VkBool32									depthWriteEnable;
		VK_COMPARE_OP_LESS,												// VkCompareOp								depthCompareOp;
		VK_FALSE,														// VkBool32									depthBoundsTestEnable;
		VK_FALSE,														// VkBool32									stencilTestEnable;
		stencilOpState,													// VkStencilOpState							front;
		stencilOpState,													// VkStencilOpState							back;
		0.0f,															// float									minDepthBounds;
		1.0f,															// float									maxDepthBounds;
	};

	const VkColorComponentFlags					colorComponentsAll					= VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	const VkPipelineColorBlendAttachmentState	pipelineColorBlendAttachmentState	=
	{
		VK_FALSE,						// VkBool32					blendEnable;
		VK_BLEND_FACTOR_ONE,			// VkBlendFactor			srcColorBlendFactor;
		VK_BLEND_FACTOR_ZERO,			// VkBlendFactor			dstColorBlendFactor;
		VK_BLEND_OP_ADD,				// VkBlendOp				colorBlendOp;
		VK_BLEND_FACTOR_ONE,			// VkBlendFactor			srcAlphaBlendFactor;
		VK_BLEND_FACTOR_ZERO,			// VkBlendFactor			dstAlphaBlendFactor;
		VK_BLEND_OP_ADD,				// VkBlendOp				alphaBlendOp;
		colorComponentsAll,				// VkColorComponentFlags	colorWriteMask;
	};

	const VkPipelineColorBlendStateCreateInfo pipelineColorBlendStateInfo =
	{
		VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,		// VkStructureType								sType;
		DE_NULL,														// const void*									pNext;
		(VkPipelineColorBlendStateCreateFlags)0,						// VkPipelineColorBlendStateCreateFlags			flags;
		VK_FALSE,														// VkBool32										logicOpEnable;
		VK_LOGIC_OP_COPY,												// VkLogicOp									logicOp;
		1u,																// deUint32										attachmentCount;
		&pipelineColorBlendAttachmentState,								// const VkPipelineColorBlendAttachmentState*	pAttachments;
		{ 0.0f, 0.0f, 0.0f, 0.0f },										// float										blendConstants[4];
	};

	const VkPipelineShaderStageCreateInfo pShaderStages[] =
	{
		{
			VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,		// VkStructureType						sType;
			DE_NULL,													// const void*							pNext;
			(VkPipelineShaderStageCreateFlags)0,						// VkPipelineShaderStageCreateFlags		flags;
			VK_SHADER_STAGE_VERTEX_BIT,									// VkShaderStageFlagBits				stage;
			vertexModule,												// VkShaderModule						module;
			"main",														// const char*							pName;
			DE_NULL,													// const VkSpecializationInfo*			pSpecializationInfo;
		},
		{
			VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,		// VkStructureType						sType;
			DE_NULL,													// const void*							pNext;
			(VkPipelineShaderStageCreateFlags)0,						// VkPipelineShaderStageCreateFlags		flags;
			VK_SHADER_STAGE_FRAGMENT_BIT,								// VkShaderStageFlagBits				stage;
			fragmentModule,												// VkShaderModule						module;
			"main",														// const char*							pName;
			DE_NULL,													// const VkSpecializationInfo*			pSpecializationInfo;
		}
	};

	const VkGraphicsPipelineCreateInfo graphicsPipelineInfo =
	{
		VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,	// VkStructureType									sType;
		DE_NULL,											// const void*										pNext;
		(VkPipelineCreateFlags)0,							// VkPipelineCreateFlags							flags;
		DE_LENGTH_OF_ARRAY(pShaderStages),					// deUint32											stageCount;
		pShaderStages,										// const VkPipelineShaderStageCreateInfo*			pStages;
		&vertexInputStateInfo,								// const VkPipelineVertexInputStateCreateInfo*		pVertexInputState;
		&pipelineInputAssemblyStateInfo,					// const VkPipelineInputAssemblyStateCreateInfo*	pInputAssemblyState;
		DE_NULL,											// const VkPipelineTessellationStateCreateInfo*		pTessellationState;
		&pipelineViewportStateInfo,							// const VkPipelineViewportStateCreateInfo*			pViewportState;
		&pipelineRasterizationStateInfo,					// const VkPipelineRasterizationStateCreateInfo*	pRasterizationState;
		&pipelineMultisampleStateInfo,						// const VkPipelineMultisampleStateCreateInfo*		pMultisampleState;
		&pipelineDepthStencilStateInfo,						// const VkPipelineDepthStencilStateCreateInfo*		pDepthStencilState;
		&pipelineColorBlendStateInfo,						// const VkPipelineColorBlendStateCreateInfo*		pColorBlendState;
		DE_NULL,											// const VkPipelineDynamicStateCreateInfo*			pDynamicState;
		pipelineLayout,										// VkPipelineLayout									layout;
		renderPass,											// VkRenderPass										renderPass;
		0u,													// deUint32											subpass;
		DE_NULL,											// VkPipeline										basePipelineHandle;
		0,													// deInt32											basePipelineIndex;
	};

	return createGraphicsPipeline(vk, device, DE_NULL, &graphicsPipelineInfo);
}

inline VertexData makeVertex (const float x, const float y, const Vec4& color)
{
	const VertexData data = { Vec4(x, y, 0.0f, 1.0f), color };
	return data;
}

std::vector<VertexData> genVertices (const TestPrimitive primitive, const Vec4& renderArea, const Vec4& primitiveColor)
{
	std::vector<VertexData> vertices;
	de::Random				rng			(1234);

	const float	x0		= 2.0f * renderArea.x() - 1.0f;
	const float y0		= 2.0f * renderArea.y() - 1.0f;
	const float	rx		= 2.0f * renderArea.z();
	const float	ry		= 2.0f * renderArea.w();
	const float	size	= 0.2f;

	switch (primitive)
	{
		case TEST_PRIMITIVE_POINTS:
			for (int i = 0; i < 50; ++i)
			{
				const float x = x0 + rng.getFloat(0.0f, rx);
				const float y = y0 + rng.getFloat(0.0f, ry);
				vertices.push_back(makeVertex(x, y, primitiveColor));
			}
			break;

		case TEST_PRIMITIVE_LINES:
			for (int i = 0; i < 30; ++i)
			{
				const float x = x0 + rng.getFloat(0.0f, rx - size);
				const float y = y0 + rng.getFloat(0.0f, ry - size);
				vertices.push_back(makeVertex(x,        y,        primitiveColor));
				vertices.push_back(makeVertex(x + size, y + size, primitiveColor));
			}
			break;

		case TEST_PRIMITIVE_TRIANGLES:
			for (int i = 0; i < 20; ++i)
			{
				const float x = x0 + rng.getFloat(0.0f, rx - size);
				const float y = y0 + rng.getFloat(0.0f, ry - size);
				vertices.push_back(makeVertex(x,             y,        primitiveColor));
				vertices.push_back(makeVertex(x + size/2.0f, y + size, primitiveColor));
				vertices.push_back(makeVertex(x + size,      y,        primitiveColor));
			}
			break;

		case TEST_PRIMITIVE_BIG_LINE:
			vertices.push_back(makeVertex(x0,      y0,      primitiveColor));
			vertices.push_back(makeVertex(x0 + rx, y0 + ry, primitiveColor));
			break;

		case TEST_PRIMITIVE_BIG_TRIANGLE:
			vertices.push_back(makeVertex(x0,           y0,      primitiveColor));
			vertices.push_back(makeVertex(x0 + rx/2.0f, y0 + ry, primitiveColor));
			vertices.push_back(makeVertex(x0 + rx,      y0,      primitiveColor));
			break;
	}

	return vertices;
}

VkPrimitiveTopology	getTopology (const TestPrimitive primitive)
{
	switch (primitive)
	{
		case TEST_PRIMITIVE_POINTS:			return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;

		case TEST_PRIMITIVE_LINES:
		case TEST_PRIMITIVE_BIG_LINE:		return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;

		case TEST_PRIMITIVE_TRIANGLES:
		case TEST_PRIMITIVE_BIG_TRIANGLE:	return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

		default:
			DE_ASSERT(0);
			return VK_PRIMITIVE_TOPOLOGY_LAST;
	}
}

void zeroBuffer (const DeviceInterface& vk, const VkDevice device, const Allocation& alloc, const VkDeviceSize size)
{
	deMemset(alloc.getHostPtr(), 0, static_cast<std::size_t>(size));
	flushMappedMemoryRange(vk, device, alloc.getMemory(), alloc.getOffset(), size);
}

//! Transform from normalized coords to framebuffer space.
inline IVec4 getAreaRect (const Vec4& area, const int width, const int height)
{
	return IVec4(static_cast<deInt32>(static_cast<float>(width)  * area.x()),
				 static_cast<deInt32>(static_cast<float>(height) * area.y()),
				 static_cast<deInt32>(static_cast<float>(width)  * area.z()),
				 static_cast<deInt32>(static_cast<float>(height) * area.w()));
}

void applyScissor (tcu::PixelBufferAccess imageAccess, const Vec4& floatScissorArea, const Vec4& clearColor)
{
	const IVec4	scissorRect	(getAreaRect(floatScissorArea, imageAccess.getWidth(), imageAccess.getHeight()));
	const int	sx0			= scissorRect.x();
	const int	sx1			= scissorRect.x() + scissorRect.z();
	const int	sy0			= scissorRect.y();
	const int	sy1			= scissorRect.y() + scissorRect.w();

	for (int y = 0; y < imageAccess.getHeight(); ++y)
	for (int x = 0; x < imageAccess.getWidth(); ++x)
	{
		// Fragments outside fail the scissor test.
		if (x < sx0 || x >= sx1 || y < sy0 || y >= sy1)
			imageAccess.setPixel(clearColor, x, y);
	}
}

void initPrograms (SourceCollections& programCollection, const CaseDef caseDef)
{
	DE_UNREF(caseDef);

	// Vertex shader
	{
		const bool usePointSize = (caseDef.primitive == TEST_PRIMITIVE_POINTS);

		std::ostringstream src;
		src << glu::getGLSLVersionDeclaration(glu::GLSL_VERSION_450) << "\n"
			<< "\n"
			<< "layout(location = 0) in  vec4 in_position;\n"
			<< "layout(location = 1) in  vec4 in_color;\n"
			<< "layout(location = 0) out vec4 o_color;\n"
			<< "\n"
			<< "out gl_PerVertex {\n"
			<< "    vec4  gl_Position;\n"
			<< (usePointSize ? "    float gl_PointSize;\n" : "")
			<< "};\n"
			<< "\n"
			<< "void main(void)\n"
			<< "{\n"
			<< "    gl_Position  = in_position;\n"
			<< (usePointSize ? "    gl_PointSize = 1.0;\n" : "")
			<< "    o_color      = in_color;\n"
			<< "}\n";

		programCollection.glslSources.add("vert") << glu::VertexSource(src.str());
	}

	// Fragment shader
	{
		std::ostringstream src;
		src << glu::getGLSLVersionDeclaration(glu::GLSL_VERSION_450) << "\n"
			<< "\n"
			<< "layout(location = 0) in  vec4 in_color;\n"
			<< "layout(location = 0) out vec4 o_color;\n"
			<< "\n"
			<< "void main(void)\n"
			<< "{\n"
			<< "    o_color = in_color;\n"
			<< "}\n";

		programCollection.glslSources.add("frag") << glu::FragmentSource(src.str());
	}
}

class ScissorRenderer
{
public:
	ScissorRenderer (Context& context, const CaseDef caseDef, const IVec2& renderSize, const VkFormat colorFormat, const Vec4& primitiveColor, const Vec4& clearColor)
		: m_renderSize				(renderSize)
		, m_colorFormat				(colorFormat)
		, m_colorSubresourceRange	(makeImageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT, 0u, 1u, 0u, 1u))
		, m_primitiveColor			(primitiveColor)
		, m_clearColor				(clearColor)
		, m_vertices				(genVertices(caseDef.primitive, caseDef.renderArea, m_primitiveColor))
		, m_vertexBufferSize		(sizeInBytes(m_vertices))
		, m_topology				(getTopology(caseDef.primitive))
	{
		const DeviceInterface&		vk					= context.getDeviceInterface();
		const VkDevice				device				= context.getDevice();
		const deUint32				queueFamilyIndex	= context.getUniversalQueueFamilyIndex();
		Allocator&					allocator			= context.getDefaultAllocator();

		m_colorImage			= makeImage(vk, device, makeImageCreateInfo(m_colorFormat, m_renderSize, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT));
		m_colorImageAlloc		= bindImage(vk, device, allocator, *m_colorImage, MemoryRequirement::Any);
		m_colorAttachment		= makeImageView(vk, device, *m_colorImage, VK_IMAGE_VIEW_TYPE_2D, m_colorFormat, m_colorSubresourceRange);

		m_vertexBuffer			= makeBuffer(vk, device, makeBufferCreateInfo(m_vertexBufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT));
		m_vertexBufferAlloc		= bindBuffer(vk, device, allocator, *m_vertexBuffer, MemoryRequirement::HostVisible);

		{
			deMemcpy(m_vertexBufferAlloc->getHostPtr(), &m_vertices[0], static_cast<std::size_t>(m_vertexBufferSize));
			flushMappedMemoryRange(vk, device, m_vertexBufferAlloc->getMemory(), m_vertexBufferAlloc->getOffset(), m_vertexBufferSize);
		}

		m_vertexModule				= createShaderModule	(vk, device, context.getBinaryCollection().get("vert"), 0u);
		m_fragmentModule			= createShaderModule	(vk, device, context.getBinaryCollection().get("frag"), 0u);
		m_renderPass				= makeRenderPass		(vk, device, m_colorFormat);
		m_framebuffer				= makeFramebuffer		(vk, device, *m_renderPass, 1u, &m_colorAttachment.get(),
															 static_cast<deUint32>(m_renderSize.x()),  static_cast<deUint32>(m_renderSize.y()));
		m_pipelineLayout			= makePipelineLayout	(vk, device);
		m_cmdPool					= createCommandPool		(vk, device, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, queueFamilyIndex);
		m_cmdBuffer					= allocateCommandBuffer	(vk, device, *m_cmdPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY);

	}

	void draw (Context& context, const Vec4& scissorAreaFloat, const VkBuffer colorBuffer) const
	{
		const DeviceInterface&		vk			= context.getDeviceInterface();
		const VkDevice				device		= context.getDevice();
		const VkQueue				queue		= context.getUniversalQueue();

		// New pipeline, because we're modifying scissor (we don't use dynamic state).
		const Unique<VkPipeline>	pipeline	(makeGraphicsPipeline(vk, device, *m_pipelineLayout, *m_renderPass, *m_vertexModule, *m_fragmentModule,
												 m_renderSize, getAreaRect(scissorAreaFloat, m_renderSize.x(), m_renderSize.y()), m_topology));

		beginCommandBuffer(vk, *m_cmdBuffer);

		const VkClearValue			clearValue	= makeClearValueColor(m_clearColor);
		const VkRect2D				renderArea	=
		{
			makeOffset2D(0, 0),
			makeExtent2D(m_renderSize.x(), m_renderSize.y()),
		};
		const VkRenderPassBeginInfo renderPassBeginInfo =
		{
			VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,		// VkStructureType         sType;
			DE_NULL,										// const void*             pNext;
			*m_renderPass,									// VkRenderPass            renderPass;
			*m_framebuffer,									// VkFramebuffer           framebuffer;
			renderArea,										// VkRect2D                renderArea;
			1u,												// uint32_t                clearValueCount;
			&clearValue,									// const VkClearValue*     pClearValues;
		};
		vk.cmdBeginRenderPass(*m_cmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

		vk.cmdBindPipeline(*m_cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *pipeline);
		{
			const VkDeviceSize vertexBufferOffset = 0ull;
			vk.cmdBindVertexBuffers(*m_cmdBuffer, 0u, 1u, &m_vertexBuffer.get(), &vertexBufferOffset);
		}

		vk.cmdDraw(*m_cmdBuffer, static_cast<deUint32>(m_vertices.size()), 1u, 0u, 0u);
		vk.cmdEndRenderPass(*m_cmdBuffer);

		// Prepare color image for copy
		{
			const VkImageMemoryBarrier barriers[] =
			{
				{
					VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,						// VkStructureType			sType;
					DE_NULL,													// const void*				pNext;
					VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,						// VkAccessFlags			outputMask;
					VK_ACCESS_TRANSFER_READ_BIT,								// VkAccessFlags			inputMask;
					VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,					// VkImageLayout			oldLayout;
					VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,						// VkImageLayout			newLayout;
					VK_QUEUE_FAMILY_IGNORED,									// deUint32					srcQueueFamilyIndex;
					VK_QUEUE_FAMILY_IGNORED,									// deUint32					destQueueFamilyIndex;
					*m_colorImage,												// VkImage					image;
					m_colorSubresourceRange,									// VkImageSubresourceRange	subresourceRange;
				},
			};

			vk.cmdPipelineBarrier(*m_cmdBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0u,
				0u, DE_NULL, 0u, DE_NULL, DE_LENGTH_OF_ARRAY(barriers), barriers);
		}
		// Color image -> host buffer
		{
			const VkBufferImageCopy region =
			{
				0ull,																		// VkDeviceSize                bufferOffset;
				0u,																			// uint32_t                    bufferRowLength;
				0u,																			// uint32_t                    bufferImageHeight;
				makeImageSubresourceLayers(VK_IMAGE_ASPECT_COLOR_BIT, 0u, 0u, 1u),			// VkImageSubresourceLayers    imageSubresource;
				makeOffset3D(0, 0, 0),														// VkOffset3D                  imageOffset;
				makeExtent3D(m_renderSize.x(), m_renderSize.y(), 1u),						// VkExtent3D                  imageExtent;
			};

			vk.cmdCopyImageToBuffer(*m_cmdBuffer, *m_colorImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, colorBuffer, 1u, &region);
		}
		// Buffer write barrier
		{
			const VkBufferMemoryBarrier barriers[] =
			{
				{
					VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,		// VkStructureType    sType;
					DE_NULL,										// const void*        pNext;
					VK_ACCESS_TRANSFER_WRITE_BIT,					// VkAccessFlags      srcAccessMask;
					VK_ACCESS_HOST_READ_BIT,						// VkAccessFlags      dstAccessMask;
					VK_QUEUE_FAMILY_IGNORED,						// uint32_t           srcQueueFamilyIndex;
					VK_QUEUE_FAMILY_IGNORED,						// uint32_t           dstQueueFamilyIndex;
					colorBuffer,									// VkBuffer           buffer;
					0ull,											// VkDeviceSize       offset;
					VK_WHOLE_SIZE,									// VkDeviceSize       size;
				},
			};

			vk.cmdPipelineBarrier(*m_cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_HOST_BIT, 0u,
				0u, DE_NULL, DE_LENGTH_OF_ARRAY(barriers), barriers, DE_NULL, 0u);
		}

		VK_CHECK(vk.endCommandBuffer(*m_cmdBuffer));
		submitCommandsAndWait(vk, device, queue, *m_cmdBuffer);
	}

private:
	const IVec2						m_renderSize;
	const VkFormat					m_colorFormat;
	const VkImageSubresourceRange	m_colorSubresourceRange;
	const Vec4						m_primitiveColor;
	const Vec4						m_clearColor;
	const std::vector<VertexData>	m_vertices;
	const VkDeviceSize				m_vertexBufferSize;
	const VkPrimitiveTopology		m_topology;

	Move<VkImage>					m_colorImage;
	MovePtr<Allocation>				m_colorImageAlloc;
	Move<VkImageView>				m_colorAttachment;
	Move<VkBuffer>					m_vertexBuffer;
	MovePtr<Allocation>				m_vertexBufferAlloc;
	Move<VkShaderModule>			m_vertexModule;
	Move<VkShaderModule>			m_fragmentModule;
	Move<VkRenderPass>				m_renderPass;
	Move<VkFramebuffer>				m_framebuffer;
	Move<VkPipelineLayout>			m_pipelineLayout;
	Move<VkCommandPool>				m_cmdPool;
	Move<VkCommandBuffer>			m_cmdBuffer;

	// "deleted"
						ScissorRenderer	(const ScissorRenderer&);
	ScissorRenderer&	operator=		(const ScissorRenderer&);
};

tcu::TestStatus test (Context& context, const CaseDef caseDef)
{
	const DeviceInterface&			vk							= context.getDeviceInterface();
	const VkDevice					device						= context.getDevice();
	Allocator&						allocator					= context.getDefaultAllocator();

	const IVec2						renderSize					(128, 128);
	const VkFormat					colorFormat					= VK_FORMAT_R8G8B8A8_UNORM;
	const Vec4						scissorFullArea				(0.0f, 0.0f, 1.0f, 1.0f);
	const Vec4						primitiveColor				(1.0f, 1.0f, 1.0f, 1.0f);
	const Vec4						clearColor					(0.5f, 0.5f, 1.0f, 1.0f);

	const VkDeviceSize				colorBufferSize				= renderSize.x() * renderSize.y() * tcu::getPixelSize(mapVkFormat(colorFormat));
	const Unique<VkBuffer>			colorBufferFull				(makeBuffer(vk, device, makeBufferCreateInfo(colorBufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT)));
	const UniquePtr<Allocation>		colorBufferFullAlloc		(bindBuffer(vk, device, allocator, *colorBufferFull, MemoryRequirement::HostVisible));

	const Unique<VkBuffer>			colorBufferScissored		(makeBuffer(vk, device, makeBufferCreateInfo(colorBufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT)));
	const UniquePtr<Allocation>		colorBufferScissoredAlloc	(bindBuffer(vk, device, allocator, *colorBufferScissored, MemoryRequirement::HostVisible));

	zeroBuffer(vk, device, *colorBufferFullAlloc, colorBufferSize);
	zeroBuffer(vk, device, *colorBufferScissoredAlloc, colorBufferSize);

	// Draw
	{
		const ScissorRenderer renderer (context, caseDef, renderSize, colorFormat, primitiveColor, clearColor);

		renderer.draw(context, scissorFullArea, *colorBufferFull);
		renderer.draw(context, caseDef.scissorArea, *colorBufferScissored);
	}

	// Log image
	{
		invalidateMappedMemoryRange(vk, device, colorBufferFullAlloc->getMemory(), 0ull, colorBufferSize);
		invalidateMappedMemoryRange(vk, device, colorBufferScissoredAlloc->getMemory(), 0ull, colorBufferSize);

		const tcu::ConstPixelBufferAccess	resultImage		(mapVkFormat(colorFormat), renderSize.x(), renderSize.y(), 1u, colorBufferScissoredAlloc->getHostPtr());
		tcu::PixelBufferAccess				referenceImage	(mapVkFormat(colorFormat), renderSize.x(), renderSize.y(), 1u, colorBufferFullAlloc->getHostPtr());

		// Apply scissor to the full image, so we can compare it with the result image.
		applyScissor (referenceImage, caseDef.scissorArea, clearColor);

		// Images should now match.
		if (!tcu::floatThresholdCompare(context.getTestContext().getLog(), "color", "Image compare", referenceImage, resultImage, Vec4(0.02f), tcu::COMPARE_LOG_RESULT))
			return tcu::TestStatus::fail("Rendered image is not correct");
	}

	return tcu::TestStatus::pass("OK");
}

//! \note The ES 2.0 scissoring tests included color/depth/stencil clear cases, but these operations are not affected by scissor test in Vulkan.
//!       Scissor is part of the pipeline state and pipeline only affects the drawing commands.
void createTestsInGroup (tcu::TestCaseGroup* scissorGroup)
{
	tcu::TestContext& testCtx = scissorGroup->getTestContext();

	struct TestSpec
	{
		const char*		name;
		const char*		description;
		CaseDef			caseDef;
	};

	const Vec4	areaFull			(0.0f, 0.0f, 1.0f, 1.0f);
	const Vec4	areaCropped			(0.2f, 0.2f, 0.6f, 0.6f);
	const Vec4	areaCroppedMore		(0.4f, 0.4f, 0.2f, 0.2f);
	const Vec4	areaLeftHalf		(0.0f, 0.0f, 0.5f, 1.0f);
	const Vec4	areaRightHalf		(0.5f, 0.0f, 0.5f, 1.0f);

	// Points
	{
		MovePtr<tcu::TestCaseGroup> primitiveGroup (new tcu::TestCaseGroup(testCtx, "points", ""));

		const TestSpec	cases[] =
		{
			{ "inside",				"Points fully inside the scissor area",		{ areaFull,		areaFull,		TEST_PRIMITIVE_POINTS } },
			{ "partially_inside",	"Points partially inside the scissor area",	{ areaFull,		areaCropped,	TEST_PRIMITIVE_POINTS } },
			{ "outside",			"Points fully outside the scissor area",	{ areaLeftHalf,	areaRightHalf,	TEST_PRIMITIVE_POINTS } },
		};

		for (int i = 0; i < DE_LENGTH_OF_ARRAY(cases); ++i)
			addFunctionCaseWithPrograms(primitiveGroup.get(), cases[i].name, cases[i].description, initPrograms, test, cases[i].caseDef);

		scissorGroup->addChild(primitiveGroup.release());
	}

	// Lines
	{
		MovePtr<tcu::TestCaseGroup> primitiveGroup (new tcu::TestCaseGroup(testCtx, "lines", ""));

		const TestSpec	cases[] =
		{
			{ "inside",				"Lines fully inside the scissor area",		{ areaFull,		areaFull,			TEST_PRIMITIVE_LINES	} },
			{ "partially_inside",	"Lines partially inside the scissor area",	{ areaFull,		areaCropped,		TEST_PRIMITIVE_LINES	} },
			{ "outside",			"Lines fully outside the scissor area",		{ areaLeftHalf,	areaRightHalf,		TEST_PRIMITIVE_LINES	} },
			{ "crossing",			"A line crossing the scissor area",			{ areaFull,		areaCroppedMore,	TEST_PRIMITIVE_BIG_LINE	} },
		};

		for (int i = 0; i < DE_LENGTH_OF_ARRAY(cases); ++i)
			addFunctionCaseWithPrograms(primitiveGroup.get(), cases[i].name, cases[i].description, initPrograms, test, cases[i].caseDef);

		scissorGroup->addChild(primitiveGroup.release());
	}

	// Triangles
	{
		MovePtr<tcu::TestCaseGroup> primitiveGroup (new tcu::TestCaseGroup(testCtx, "triangles", ""));

		const TestSpec	cases[] =
		{
			{ "inside",				"Triangles fully inside the scissor area",		{ areaFull,		areaFull,			TEST_PRIMITIVE_TRIANGLES	} },
			{ "partially_inside",	"Triangles partially inside the scissor area",	{ areaFull,		areaCropped,		TEST_PRIMITIVE_TRIANGLES	} },
			{ "outside",			"Triangles fully outside the scissor area",		{ areaLeftHalf,	areaRightHalf,		TEST_PRIMITIVE_TRIANGLES	} },
			{ "crossing",			"A triangle crossing the scissor area",			{ areaFull,		areaCroppedMore,	TEST_PRIMITIVE_BIG_TRIANGLE	} },
		};

		for (int i = 0; i < DE_LENGTH_OF_ARRAY(cases); ++i)
			addFunctionCaseWithPrograms(primitiveGroup.get(), cases[i].name, cases[i].description, initPrograms, test, cases[i].caseDef);

		scissorGroup->addChild(primitiveGroup.release());
	}

	// Mulit-viewport scissor
	{
		scissorGroup->addChild(createScissorMultiViewportTests(testCtx));
	}
}

} // anonymous

tcu::TestCaseGroup* createScissorTests (tcu::TestContext& testCtx)
{
	return createTestGroup(testCtx, "scissor", "Scissor tests", createTestsInGroup);
}

} // FragmentOperations
} // vkt
