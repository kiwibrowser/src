/*------------------------------------------------------------------------
 * Vulkan Conformance Tests
 * ------------------------
 *
 * Copyright (c) 2015 The Khronos Group Inc.
 * Copyright (c) 2015 ARM Ltd.
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
 * \brief Timestamp Tests
 *//*--------------------------------------------------------------------*/

#include "vktPipelineTimestampTests.hpp"
#include "vktPipelineClearUtil.hpp"
#include "vktPipelineImageUtil.hpp"
#include "vktPipelineVertexUtil.hpp"
#include "vktPipelineReferenceRenderer.hpp"
#include "vktTestCase.hpp"
#include "vktTestCaseUtil.hpp"
#include "vkImageUtil.hpp"
#include "vkMemUtil.hpp"
#include "vkPrograms.hpp"
#include "vkBuilderUtil.hpp"
#include "vkQueryUtil.hpp"
#include "vkRef.hpp"
#include "vkRefUtil.hpp"
#include "tcuImageCompare.hpp"
#include "deUniquePtr.hpp"
#include "deStringUtil.hpp"
#include "deMemory.h"
#include "vkTypeUtil.hpp"

#include <sstream>
#include <vector>
#include <cctype>
#include <locale>

namespace vkt
{
namespace pipeline
{

using namespace vk;

namespace
{
typedef std::vector<VkPipelineStageFlagBits> StageFlagVector;

// helper functions
#define GEN_DESC_STRING(name,postfix)                                      \
		do {                                                               \
		   for (std::string::size_type ndx = 0; ndx<strlen(#name); ++ndx)  \
			 if(isDescription && #name[ndx] == '_')                        \
			   desc << " ";                                                \
			 else                                                          \
			   desc << std::tolower(#name[ndx],loc);                       \
		   if (isDescription)                                              \
			 desc << " " << #postfix;                                      \
		   else                                                            \
			 desc << "_" << #postfix;                                      \
		} while (deGetFalse())

std::string getPipelineStageFlagStr (const VkPipelineStageFlagBits stage,
									 bool                          isDescription)
{
	std::ostringstream desc;
	std::locale loc;
	switch(stage)
	{
#define STAGE_CASE(p)                              \
		case VK_PIPELINE_STAGE_##p##_BIT:          \
		{                                          \
			GEN_DESC_STRING(p, stage);             \
			break;                                 \
		}
		STAGE_CASE(TOP_OF_PIPE);
		STAGE_CASE(DRAW_INDIRECT);
		STAGE_CASE(VERTEX_INPUT);
		STAGE_CASE(VERTEX_SHADER);
		STAGE_CASE(TESSELLATION_CONTROL_SHADER);
		STAGE_CASE(TESSELLATION_EVALUATION_SHADER);
		STAGE_CASE(GEOMETRY_SHADER);
		STAGE_CASE(FRAGMENT_SHADER);
		STAGE_CASE(EARLY_FRAGMENT_TESTS);
		STAGE_CASE(LATE_FRAGMENT_TESTS);
		STAGE_CASE(COLOR_ATTACHMENT_OUTPUT);
		STAGE_CASE(COMPUTE_SHADER);
		STAGE_CASE(TRANSFER);
		STAGE_CASE(HOST);
		STAGE_CASE(ALL_GRAPHICS);
		STAGE_CASE(ALL_COMMANDS);
#undef STAGE_CASE
	  default:
		desc << "unknown stage!";
		DE_FATAL("Unknown Stage!");
		break;
	};

	return desc.str();
}

enum TransferMethod
{
	TRANSFER_METHOD_COPY_BUFFER = 0,
	TRANSFER_METHOD_COPY_IMAGE,
	TRANSFER_METHOD_BLIT_IMAGE,
	TRANSFER_METHOD_COPY_BUFFER_TO_IMAGE,
	TRANSFER_METHOD_COPY_IMAGE_TO_BUFFER,
	TRANSFER_METHOD_UPDATE_BUFFER,
	TRANSFER_METHOD_FILL_BUFFER,
	TRANSFER_METHOD_CLEAR_COLOR_IMAGE,
	TRANSFER_METHOD_CLEAR_DEPTH_STENCIL_IMAGE,
	TRANSFER_METHOD_RESOLVE_IMAGE,
	TRANSFER_METHOD_COPY_QUERY_POOL_RESULTS,
	TRANSFER_METHOD_LAST
};

std::string getTransferMethodStr(const TransferMethod method,
								 bool                 isDescription)
{
	std::ostringstream desc;
	std::locale loc;
	switch(method)
	{
#define METHOD_CASE(p)                             \
		case TRANSFER_METHOD_##p:                  \
		{                                          \
			GEN_DESC_STRING(p, method);            \
			break;                                 \
		}
	  METHOD_CASE(COPY_BUFFER)
	  METHOD_CASE(COPY_IMAGE)
	  METHOD_CASE(BLIT_IMAGE)
	  METHOD_CASE(COPY_BUFFER_TO_IMAGE)
	  METHOD_CASE(COPY_IMAGE_TO_BUFFER)
	  METHOD_CASE(UPDATE_BUFFER)
	  METHOD_CASE(FILL_BUFFER)
	  METHOD_CASE(CLEAR_COLOR_IMAGE)
	  METHOD_CASE(CLEAR_DEPTH_STENCIL_IMAGE)
	  METHOD_CASE(RESOLVE_IMAGE)
	  METHOD_CASE(COPY_QUERY_POOL_RESULTS)
#undef METHOD_CASE
	  default:
		desc << "unknown method!";
		DE_FATAL("Unknown method!");
		break;
	};

	return desc.str();
}

// helper classes
class TimestampTestParam
{
public:
							  TimestampTestParam      (const VkPipelineStageFlagBits* stages,
													   const deUint32                 stageCount,
													   const bool                     inRenderPass);
							  ~TimestampTestParam     (void);
	virtual const std::string generateTestName        (void) const;
	virtual const std::string generateTestDescription (void) const;
	StageFlagVector           getStageVector          (void) const { return m_stageVec; }
	bool                      getInRenderPass         (void) const { return m_inRenderPass; }
	void                      toggleInRenderPass      (void)       { m_inRenderPass = !m_inRenderPass; }
protected:
	StageFlagVector           m_stageVec;
	bool                      m_inRenderPass;
};

TimestampTestParam::TimestampTestParam(const VkPipelineStageFlagBits* stages,
									   const deUint32                 stageCount,
									   const bool                     inRenderPass)
	: m_inRenderPass(inRenderPass)
{
	for (deUint32 ndx = 0; ndx < stageCount; ndx++)
	{
		m_stageVec.push_back(stages[ndx]);
	}
}

TimestampTestParam::~TimestampTestParam(void)
{
}

const std::string TimestampTestParam::generateTestName(void) const
{
	std::string result("");

	for (StageFlagVector::const_iterator it = m_stageVec.begin(); it != m_stageVec.end(); it++)
	{
		if(*it != VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT)
		{
			result += getPipelineStageFlagStr(*it, false) + '_';
		}
	}
	if(m_inRenderPass)
		result += "in_render_pass";
	else
		result += "out_of_render_pass";

	return result;
}

const std::string TimestampTestParam::generateTestDescription(void) const
{
	std::string result("Record timestamp after ");

	for (StageFlagVector::const_iterator it = m_stageVec.begin(); it != m_stageVec.end(); it++)
	{
		if(*it != VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT)
		{
			result += getPipelineStageFlagStr(*it, true) + ' ';
		}
	}
	if(m_inRenderPass)
		result += " in the renderpass";
	else
		result += " out of the render pass";

	return result;
}

class TransferTimestampTestParam : public TimestampTestParam
{
public:
					  TransferTimestampTestParam  (const VkPipelineStageFlagBits* stages,
												   const deUint32                 stageCount,
												   const bool                     inRenderPass,
												   const deUint32                 methodNdx);
					  ~TransferTimestampTestParam (void)       { }
	const std::string generateTestName            (void) const;
	const std::string generateTestDescription     (void) const;
	TransferMethod    getMethod                   (void) const { return m_method; }
protected:
	TransferMethod    m_method;
};

TransferTimestampTestParam::TransferTimestampTestParam(const VkPipelineStageFlagBits* stages,
													   const deUint32                 stageCount,
													   const bool                     inRenderPass,
													   const deUint32                 methodNdx)
	: TimestampTestParam(stages, stageCount, inRenderPass)
{
	DE_ASSERT(methodNdx < (deUint32)TRANSFER_METHOD_LAST);

	m_method = (TransferMethod)methodNdx;
}

const std::string TransferTimestampTestParam::generateTestName(void) const
{
	std::string result("");

	for (StageFlagVector::const_iterator it = m_stageVec.begin(); it != m_stageVec.end(); it++)
	{
		if(*it != VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT)
		{
			result += getPipelineStageFlagStr(*it, false) + '_';
		}
	}

	result += "with_" + getTransferMethodStr(m_method, false);

	return result;

}

const std::string TransferTimestampTestParam::generateTestDescription(void) const
{
	std::string result("");

	for (StageFlagVector::const_iterator it = m_stageVec.begin(); it != m_stageVec.end(); it++)
	{
		if(*it != VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT)
		{
			result += getPipelineStageFlagStr(*it, true) + ' ';
		}
	}

	result += "with " + getTransferMethodStr(m_method, true);

	return result;

}

class SimpleGraphicsPipelineBuilder
{
public:
					 SimpleGraphicsPipelineBuilder  (Context&              context);
					 ~SimpleGraphicsPipelineBuilder (void) { }
	void             bindShaderStage                (VkShaderStageFlagBits stage,
													 const char*           source_name,
													 const char*           entry_name);
	void             enableTessellationStage        (deUint32              patchControlPoints);
	Move<VkPipeline> buildPipeline                  (tcu::UVec2            renderSize,
													 VkRenderPass          renderPass);
protected:
	enum
	{
		VK_MAX_SHADER_STAGES = 6,
	};

	Context&                            m_context;

	Move<VkShaderModule>                m_shaderModules[VK_MAX_SHADER_STAGES];
	deUint32                            m_shaderStageCount;
	VkPipelineShaderStageCreateInfo     m_shaderStageInfo[VK_MAX_SHADER_STAGES];

	deUint32                            m_patchControlPoints;

	Move<VkPipelineLayout>              m_pipelineLayout;
	Move<VkPipeline>                    m_graphicsPipelines;

};

SimpleGraphicsPipelineBuilder::SimpleGraphicsPipelineBuilder(Context& context)
	: m_context(context)
{
	m_patchControlPoints = 0;
	m_shaderStageCount   = 0;
}

void SimpleGraphicsPipelineBuilder::bindShaderStage(VkShaderStageFlagBits stage,
													const char*           source_name,
													const char*           entry_name)
{
	const DeviceInterface&  vk        = m_context.getDeviceInterface();
	const VkDevice          vkDevice  = m_context.getDevice();

	// Create shader module
	deUint32*               pCode     = (deUint32*)m_context.getBinaryCollection().get(source_name).getBinary();
	deUint32                codeSize  = (deUint32)m_context.getBinaryCollection().get(source_name).getSize();

	const VkShaderModuleCreateInfo moduleCreateInfo =
	{
		VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,                // VkStructureType             sType;
		DE_NULL,                                                    // const void*                 pNext;
		0u,                                                         // VkShaderModuleCreateFlags   flags;
		codeSize,                                                   // deUintptr                   codeSize;
		pCode,                                                      // const deUint32*             pCode;
	};

	m_shaderModules[m_shaderStageCount] = createShaderModule(vk, vkDevice, &moduleCreateInfo);

	// Prepare shader stage info
	m_shaderStageInfo[m_shaderStageCount].sType               = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	m_shaderStageInfo[m_shaderStageCount].pNext               = DE_NULL;
	m_shaderStageInfo[m_shaderStageCount].flags               = 0u;
	m_shaderStageInfo[m_shaderStageCount].stage               = stage;
	m_shaderStageInfo[m_shaderStageCount].module              = *m_shaderModules[m_shaderStageCount];
	m_shaderStageInfo[m_shaderStageCount].pName               = entry_name;
	m_shaderStageInfo[m_shaderStageCount].pSpecializationInfo = DE_NULL;

	m_shaderStageCount++;
}

Move<VkPipeline> SimpleGraphicsPipelineBuilder::buildPipeline(tcu::UVec2 renderSize, VkRenderPass renderPass)
{
	const DeviceInterface&      vk                  = m_context.getDeviceInterface();
	const VkDevice              vkDevice            = m_context.getDevice();

	// Create pipeline layout
	{
		const VkPipelineLayoutCreateInfo pipelineLayoutParams =
		{
			VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,      // VkStructureType                  sType;
			DE_NULL,                                            // const void*                      pNext;
			0u,                                                 // VkPipelineLayoutCreateFlags      flags;
			0u,                                                 // deUint32                         setLayoutCount;
			DE_NULL,                                            // const VkDescriptorSetLayout*     pSetLayouts;
			0u,                                                 // deUint32                         pushConstantRangeCount;
			DE_NULL                                             // const VkPushConstantRange*       pPushConstantRanges;
		};

		m_pipelineLayout = createPipelineLayout(vk, vkDevice, &pipelineLayoutParams);
	}

	// Create pipeline
	const VkVertexInputBindingDescription vertexInputBindingDescription =
	{
		0u,                                 // deUint32                 binding;
		sizeof(Vertex4RGBA),                // deUint32                 strideInBytes;
		VK_VERTEX_INPUT_RATE_VERTEX,        // VkVertexInputRate        inputRate;
	};

	const VkVertexInputAttributeDescription vertexInputAttributeDescriptions[2] =
	{
		{
			0u,                                 // deUint32 location;
			0u,                                 // deUint32 binding;
			VK_FORMAT_R32G32B32A32_SFLOAT,      // VkFormat format;
			0u                                  // deUint32 offsetInBytes;
		},
		{
			1u,                                 // deUint32 location;
			0u,                                 // deUint32 binding;
			VK_FORMAT_R32G32B32A32_SFLOAT,      // VkFormat format;
			DE_OFFSET_OF(Vertex4RGBA, color),   // deUint32 offsetInBytes;
		}
	};

	const VkPipelineVertexInputStateCreateInfo vertexInputStateParams =
	{
		VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,      // VkStructureType                          sType;
		DE_NULL,                                                        // const void*                              pNext;
		0u,                                                             // VkPipelineVertexInputStateCreateFlags    flags;
		1u,                                                             // deUint32                                 vertexBindingDescriptionCount;
		&vertexInputBindingDescription,                                 // const VkVertexInputBindingDescription*   pVertexBindingDescriptions;
		2u,                                                             // deUint32                                 vertexAttributeDescriptionCount;
		vertexInputAttributeDescriptions,                               // const VkVertexInputAttributeDescription* pVertexAttributeDescriptions;
	};

	VkPrimitiveTopology primitiveTopology = (m_patchControlPoints > 0) ? VK_PRIMITIVE_TOPOLOGY_PATCH_LIST : VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	const VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateParams =
	{
		VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,    // VkStructureType                          sType;
		DE_NULL,                                                        // const void*                              pNext;
		0u,                                                             // VkPipelineInputAssemblyStateCreateFlags  flags;
		primitiveTopology,                                              // VkPrimitiveTopology                      topology;
		VK_FALSE,                                                       // VkBool32                                 primitiveRestartEnable;
	};

	const VkViewport viewport =
	{
		0.0f,                       // float    originX;
		0.0f,                       // float    originY;
		(float)renderSize.x(),      // float    width;
		(float)renderSize.y(),      // float    height;
		0.0f,                       // float    minDepth;
		1.0f                        // float    maxDepth;
	};
	const VkRect2D scissor =
	{
		{ 0u, 0u },                                                     // VkOffset2D  offset;
		{ renderSize.x(), renderSize.y() }                              // VkExtent2D  extent;
	};
	const VkPipelineViewportStateCreateInfo viewportStateParams =
	{
		VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,          // VkStructureType                      sType;
		DE_NULL,                                                        // const void*                          pNext;
		0u,                                                             // VkPipelineViewportStateCreateFlags   flags;
		1u,                                                             // deUint32                             viewportCount;
		&viewport,                                                      // const VkViewport*                    pViewports;
		1u,                                                             // deUint32                             scissorCount;
		&scissor                                                        // const VkRect2D*                      pScissors;
	};

	const VkPipelineRasterizationStateCreateInfo rasterStateParams =
	{
		VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,     // VkStructureType                          sType;
		DE_NULL,                                                        // const void*                              pNext;
		0u,                                                             // VkPipelineRasterizationStateCreateFlags  flags;
		VK_FALSE,                                                       // VkBool32                                 depthClampEnable;
		VK_FALSE,                                                       // VkBool32                                 rasterizerDiscardEnable;
		VK_POLYGON_MODE_FILL,                                           // VkPolygonMode                            polygonMode;
		VK_CULL_MODE_NONE,                                              // VkCullModeFlags                          cullMode;
		VK_FRONT_FACE_COUNTER_CLOCKWISE,                                // VkFrontFace                              frontFace;
		VK_FALSE,                                                       // VkBool32                                 depthBiasEnable;
		0.0f,                                                           // float                                    depthBiasConstantFactor;
		0.0f,                                                           // float                                    depthBiasClamp;
		0.0f,                                                           // float                                    depthBiasSlopeFactor;
		1.0f,                                                           // float                                    lineWidth;
	};

	const VkPipelineColorBlendAttachmentState colorBlendAttachmentState =
	{
		VK_FALSE,                                                                   // VkBool32                 blendEnable;
		VK_BLEND_FACTOR_ONE,                                                        // VkBlendFactor            srcColorBlendFactor;
		VK_BLEND_FACTOR_ZERO,                                                       // VkBlendFactor            dstColorBlendFactor;
		VK_BLEND_OP_ADD,                                                            // VkBlendOp                colorBlendOp;
		VK_BLEND_FACTOR_ONE,                                                        // VkBlendFactor            srcAlphaBlendFactor;
		VK_BLEND_FACTOR_ZERO,                                                       // VkBlendFactor            dstAlphaBlendFactor;
		VK_BLEND_OP_ADD,                                                            // VkBlendOp                alphaBlendOp;
		VK_COLOR_COMPONENT_R_BIT |
		VK_COLOR_COMPONENT_G_BIT |
		VK_COLOR_COMPONENT_B_BIT |
		VK_COLOR_COMPONENT_A_BIT                                                    // VkColorComponentFlags    colorWriteMask;
	};

	const VkPipelineColorBlendStateCreateInfo colorBlendStateParams =
	{
		VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,   // VkStructureType                              sType;
		DE_NULL,                                                    // const void*                                  pNext;
		0u,                                                         // VkPipelineColorBlendStateCreateFlags         flags;
		VK_FALSE,                                                   // VkBool32                                     logicOpEnable;
		VK_LOGIC_OP_COPY,                                           // VkLogicOp                                    logicOp;
		1u,                                                         // deUint32                                     attachmentCount;
		&colorBlendAttachmentState,                                 // const VkPipelineColorBlendAttachmentState*   pAttachments;
		{ 0.0f, 0.0f, 0.0f, 0.0f },                                 // float                                        blendConst[4];
	};

	const VkPipelineMultisampleStateCreateInfo  multisampleStateParams  =
	{
		VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,   // VkStructureType                          sType;
		DE_NULL,                                                    // const void*                              pNext;
		0u,                                                         // VkPipelineMultisampleStateCreateFlags    flags;
		VK_SAMPLE_COUNT_1_BIT,                                      // VkSampleCountFlagBits                    rasterizationSamples;
		VK_FALSE,                                                   // VkBool32                                 sampleShadingEnable;
		0.0f,                                                       // float                                    minSampleShading;
		DE_NULL,                                                    // const VkSampleMask*                      pSampleMask;
		VK_FALSE,                                                   // VkBool32                                 alphaToCoverageEnable;
		VK_FALSE,                                                   // VkBool32                                 alphaToOneEnable;
	};

	VkPipelineDepthStencilStateCreateInfo depthStencilStateParams =
	{
		VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO, // VkStructureType                          sType;
		DE_NULL,                                                    // const void*                              pNext;
		0u,                                                         // VkPipelineDepthStencilStateCreateFlags   flags;
		VK_TRUE,                                                    // VkBool32                                 depthTestEnable;
		VK_TRUE,                                                    // VkBool32                                 depthWriteEnable;
		VK_COMPARE_OP_LESS_OR_EQUAL,                                // VkCompareOp                              depthCompareOp;
		VK_FALSE,                                                   // VkBool32                                 depthBoundsTestEnable;
		VK_FALSE,                                                   // VkBool32                                 stencilTestEnable;
		// VkStencilOpState front;
		{
			VK_STENCIL_OP_KEEP,     // VkStencilOp  failOp;
			VK_STENCIL_OP_KEEP,     // VkStencilOp  passOp;
			VK_STENCIL_OP_KEEP,     // VkStencilOp  depthFailOp;
			VK_COMPARE_OP_NEVER,    // VkCompareOp  compareOp;
			0u,                     // deUint32     compareMask;
			0u,                     // deUint32     writeMask;
			0u,                     // deUint32     reference;
		},
		// VkStencilOpState back;
		{
			VK_STENCIL_OP_KEEP,     // VkStencilOp  failOp;
			VK_STENCIL_OP_KEEP,     // VkStencilOp  passOp;
			VK_STENCIL_OP_KEEP,     // VkStencilOp  depthFailOp;
			VK_COMPARE_OP_NEVER,    // VkCompareOp  compareOp;
			0u,                     // deUint32     compareMask;
			0u,                     // deUint32     writeMask;
			0u,                     // deUint32     reference;
		},
		0.0f,                                                      // float                                    minDepthBounds;
		1.0f,                                                      // float                                    maxDepthBounds;
	};

	const VkPipelineTessellationStateCreateInfo*	pTessCreateInfo		= DE_NULL;
	const VkPipelineTessellationStateCreateInfo		tessStateCreateInfo	=
	{
			VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO,  // VkStructureType                          sType;
			DE_NULL,                                                    // const void*                              pNext;
			0u,                                                         // VkPipelineTessellationStateCreateFlags   flags;
			m_patchControlPoints,                                       // deUint32                                 patchControlPoints;
	};

	if (m_patchControlPoints > 0)
		pTessCreateInfo = &tessStateCreateInfo;

	const VkGraphicsPipelineCreateInfo graphicsPipelineParams =
	{
		VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,    // VkStructureType                                  sType;
		DE_NULL,                                            // const void*                                      pNext;
		0u,                                                 // VkPipelineCreateFlags                            flags;
		m_shaderStageCount,                                 // deUint32                                         stageCount;
		m_shaderStageInfo,                                  // const VkPipelineShaderStageCreateInfo*           pStages;
		&vertexInputStateParams,                            // const VkPipelineVertexInputStateCreateInfo*      pVertexInputState;
		&inputAssemblyStateParams,                          // const VkPipelineInputAssemblyStateCreateInfo*    pInputAssemblyState;
		pTessCreateInfo,                                    // const VkPipelineTessellationStateCreateInfo*     pTessellationState;
		&viewportStateParams,                               // const VkPipelineViewportStateCreateInfo*         pViewportState;
		&rasterStateParams,                                 // const VkPipelineRasterizationStateCreateInfo*    pRasterState;
		&multisampleStateParams,                            // const VkPipelineMultisampleStateCreateInfo*      pMultisampleState;
		&depthStencilStateParams,                           // const VkPipelineDepthStencilStateCreateInfo*     pDepthStencilState;
		&colorBlendStateParams,                             // const VkPipelineColorBlendStateCreateInfo*       pColorBlendState;
		(const VkPipelineDynamicStateCreateInfo*)DE_NULL,   // const VkPipelineDynamicStateCreateInfo*          pDynamicState;
		*m_pipelineLayout,                                  // VkPipelineLayout                                 layout;
		renderPass,                                         // VkRenderPass                                     renderPass;
		0u,                                                 // deUint32                                         subpass;
		0u,                                                 // VkPipeline                                       basePipelineHandle;
		0,                                                  // deInt32                                          basePipelineIndex;
	};

	return createGraphicsPipeline(vk, vkDevice, DE_NULL, &graphicsPipelineParams);
}

void SimpleGraphicsPipelineBuilder::enableTessellationStage(deUint32 patchControlPoints)
{
	m_patchControlPoints = patchControlPoints;
}

template <class Test>
vkt::TestCase* newTestCase(tcu::TestContext&     testContext,
						   TimestampTestParam*   testParam)
{
	return new Test(testContext,
					testParam->generateTestName().c_str(),
					testParam->generateTestDescription().c_str(),
					testParam);
}

// Test Classes
class TimestampTest : public vkt::TestCase
{
public:
	enum
	{
		ENTRY_COUNT = 8
	};

						  TimestampTest(tcu::TestContext&         testContext,
										const std::string&        name,
										const std::string&        description,
										const TimestampTestParam* param)
							  : vkt::TestCase  (testContext, name, description)
							  , m_stages       (param->getStageVector())
							  , m_inRenderPass (param->getInRenderPass())
							  { }
	virtual               ~TimestampTest (void) { }
	virtual void          initPrograms   (SourceCollections&      programCollection) const;
	virtual TestInstance* createInstance (Context&                context) const;
protected:
	const StageFlagVector m_stages;
	const bool            m_inRenderPass;
};

class TimestampTestInstance : public vkt::TestInstance
{
public:
							TimestampTestInstance      (Context&                 context,
														const StageFlagVector&   stages,
														const bool               inRenderPass);
	virtual                 ~TimestampTestInstance     (void);
	virtual tcu::TestStatus iterate                    (void);
protected:
	virtual tcu::TestStatus verifyTimestamp            (void);
	virtual void            configCommandBuffer        (void);
	Move<VkBuffer>          createBufferAndBindMemory  (VkDeviceSize             size,
														VkBufferUsageFlags       usage,
														de::MovePtr<Allocation>* pAlloc);
	Move<VkImage>           createImage2DAndBindMemory (VkFormat                 format,
														deUint32                 width,
														deUint32                 height,
														VkImageUsageFlags        usage,
														VkSampleCountFlagBits    sampleCount,
														de::MovePtr<Allocation>* pAlloc);
protected:
	const StageFlagVector   m_stages;
	bool                    m_inRenderPass;

	Move<VkCommandPool>     m_cmdPool;
	Move<VkCommandBuffer>   m_cmdBuffer;
	Move<VkFence>           m_fence;
	Move<VkQueryPool>       m_queryPool;
	deUint64*               m_timestampValues;
};

void TimestampTest::initPrograms(SourceCollections& programCollection) const
{
	vkt::TestCase::initPrograms(programCollection);
}

TestInstance* TimestampTest::createInstance(Context& context) const
{
	return new TimestampTestInstance(context,m_stages,m_inRenderPass);
}

TimestampTestInstance::TimestampTestInstance(Context&                context,
											 const StageFlagVector&  stages,
											 const bool              inRenderPass)
	: TestInstance  (context)
	, m_stages      (stages)
	, m_inRenderPass(inRenderPass)
{
	const DeviceInterface&      vk                  = context.getDeviceInterface();
	const VkDevice              vkDevice            = context.getDevice();
	const deUint32              queueFamilyIndex    = context.getUniversalQueueFamilyIndex();

	// Check support for timestamp queries
	{
		const std::vector<VkQueueFamilyProperties>   queueProperties = vk::getPhysicalDeviceQueueFamilyProperties(m_context.getInstanceInterface(), m_context.getPhysicalDevice());

		DE_ASSERT(queueFamilyIndex < (deUint32)queueProperties.size());

		if (!queueProperties[queueFamilyIndex].timestampValidBits)
			throw tcu::NotSupportedError("Universal queue does not support timestamps");
	}

	// Create Query Pool
	{
		const VkQueryPoolCreateInfo queryPoolParams =
		{
		   VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO,    // VkStructureType               sType;
		   DE_NULL,                                     // const void*                   pNext;
		   0u,                                          // VkQueryPoolCreateFlags        flags;
		   VK_QUERY_TYPE_TIMESTAMP,                     // VkQueryType                   queryType;
		   TimestampTest::ENTRY_COUNT,                  // deUint32                      entryCount;
		   0u,                                          // VkQueryPipelineStatisticFlags pipelineStatistics;
		};

		m_queryPool = createQueryPool(vk, vkDevice, &queryPoolParams);
	}

	// Create command pool
	m_cmdPool = createCommandPool(vk, vkDevice, VK_COMMAND_POOL_CREATE_TRANSIENT_BIT, queueFamilyIndex);

	// Create command buffer
	m_cmdBuffer = allocateCommandBuffer(vk, vkDevice, *m_cmdPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY);

	// Create fence
	{
		const VkFenceCreateInfo fenceParams =
		{
			VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,        // VkStructureType      sType;
			DE_NULL,                                    // const void*          pNext;
			0u,                                         // VkFenceCreateFlags   flags;
		};

		m_fence = createFence(vk, vkDevice, &fenceParams);
	}

	// alloc timestamp values
	m_timestampValues = new deUint64[m_stages.size()];
}

TimestampTestInstance::~TimestampTestInstance(void)
{
	if(m_timestampValues)
	{
		delete[] m_timestampValues;
		m_timestampValues = NULL;
	}
}

void TimestampTestInstance::configCommandBuffer(void)
{
	const DeviceInterface&      vk                  = m_context.getDeviceInterface();

	const VkCommandBufferBeginInfo cmdBufferBeginInfo =
	{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,    // VkStructureType                  sType;
		DE_NULL,                                        // const void*                      pNext;
		0u,                                             // VkCommandBufferUsageFlags        flags;
		(const VkCommandBufferInheritanceInfo*)DE_NULL,
	};

	VK_CHECK(vk.beginCommandBuffer(*m_cmdBuffer, &cmdBufferBeginInfo));

	vk.cmdResetQueryPool(*m_cmdBuffer, *m_queryPool, 0u, TimestampTest::ENTRY_COUNT);

	deUint32 timestampEntry = 0;
	for (StageFlagVector::const_iterator it = m_stages.begin(); it != m_stages.end(); it++)
	{
		vk.cmdWriteTimestamp(*m_cmdBuffer, *it, *m_queryPool, timestampEntry++);
	}

	VK_CHECK(vk.endCommandBuffer(*m_cmdBuffer));
}

tcu::TestStatus TimestampTestInstance::iterate(void)
{
	const DeviceInterface&      vk          = m_context.getDeviceInterface();
	const VkDevice              vkDevice    = m_context.getDevice();
	const VkQueue               queue       = m_context.getUniversalQueue();

	configCommandBuffer();

	VK_CHECK(vk.resetFences(vkDevice, 1u, &m_fence.get()));

	const VkSubmitInfo          submitInfo =
	{
		VK_STRUCTURE_TYPE_SUBMIT_INFO,                      // VkStructureType         sType;
		DE_NULL,                                            // const void*             pNext;
		0u,                                                 // deUint32                waitSemaphoreCount;
		DE_NULL,                                            // const VkSemaphore*      pWaitSemaphores;
		(const VkPipelineStageFlags*)DE_NULL,
		1u,                                                 // deUint32                commandBufferCount;
		&m_cmdBuffer.get(),                                 // const VkCommandBuffer*  pCommandBuffers;
		0u,                                                 // deUint32                signalSemaphoreCount;
		DE_NULL,                                            // const VkSemaphore*      pSignalSemaphores;
	};
	VK_CHECK(vk.queueSubmit(queue, 1u, &submitInfo, *m_fence));

	VK_CHECK(vk.waitForFences(vkDevice, 1u, &m_fence.get(), true, ~(0ull) /* infinity*/));

	// Generate the timestamp mask
	deUint64                    timestampMask;
	const std::vector<VkQueueFamilyProperties>   queueProperties = vk::getPhysicalDeviceQueueFamilyProperties(m_context.getInstanceInterface(), m_context.getPhysicalDevice());
	if(queueProperties[0].timestampValidBits == 0)
	{
		return tcu::TestStatus::fail("Device does not support timestamp!");
	}
	else if(queueProperties[0].timestampValidBits == 64)
	{
		timestampMask = 0xFFFFFFFFFFFFFFFF;
	}
	else
	{
		timestampMask = ((deUint64)1 << queueProperties[0].timestampValidBits) - 1;
	}

	// Get timestamp value from query pool
	deUint32                    stageSize = (deUint32)m_stages.size();

	vk.getQueryPoolResults(vkDevice, *m_queryPool, 0u, stageSize, sizeof(deUint64) * stageSize, (void*)m_timestampValues, sizeof(deUint64), VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT);

	for (deUint32 ndx = 0; ndx < stageSize; ndx++)
	{
		m_timestampValues[ndx] &= timestampMask;
	}

	return verifyTimestamp();
}

tcu::TestStatus TimestampTestInstance::verifyTimestamp(void)
{
	for (deUint32 first = 0; first < m_stages.size(); first++)
	{
		for (deUint32 second = 0; second < first; second++)
		{
			if(m_timestampValues[first] < m_timestampValues[second])
			{
				return tcu::TestStatus::fail("Latter stage timestamp is smaller than the former stage timestamp.");
			}
		}
	}

	return tcu::TestStatus::pass("Timestamp increases steadily.");
}

Move<VkBuffer> TimestampTestInstance::createBufferAndBindMemory(VkDeviceSize size, VkBufferUsageFlags usage, de::MovePtr<Allocation>* pAlloc)
{
	const DeviceInterface&      vk                  = m_context.getDeviceInterface();
	const VkDevice              vkDevice            = m_context.getDevice();
	const deUint32              queueFamilyIndex    = m_context.getUniversalQueueFamilyIndex();
	SimpleAllocator             memAlloc            (vk, vkDevice, getPhysicalDeviceMemoryProperties(m_context.getInstanceInterface(), m_context.getPhysicalDevice()));

	const VkBufferCreateInfo vertexBufferParams =
	{
		VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,       // VkStructureType      sType;
		DE_NULL,                                    // const void*          pNext;
		0u,                                         // VkBufferCreateFlags  flags;
		size,                                       // VkDeviceSize         size;
		usage,                                      // VkBufferUsageFlags   usage;
		VK_SHARING_MODE_EXCLUSIVE,                  // VkSharingMode        sharingMode;
		1u,                                         // deUint32             queueFamilyCount;
		&queueFamilyIndex                           // const deUint32*      pQueueFamilyIndices;
	};

	Move<VkBuffer> vertexBuffer = createBuffer(vk, vkDevice, &vertexBufferParams);
	de::MovePtr<Allocation> vertexBufferAlloc = memAlloc.allocate(getBufferMemoryRequirements(vk, vkDevice, *vertexBuffer), MemoryRequirement::HostVisible);

	VK_CHECK(vk.bindBufferMemory(vkDevice, *vertexBuffer, vertexBufferAlloc->getMemory(), vertexBufferAlloc->getOffset()));

	DE_ASSERT(pAlloc);
	*pAlloc = vertexBufferAlloc;

	return vertexBuffer;
}

Move<VkImage> TimestampTestInstance::createImage2DAndBindMemory(VkFormat                          format,
																deUint32                          width,
																deUint32                          height,
																VkImageUsageFlags                 usage,
																VkSampleCountFlagBits             sampleCount,
																de::details::MovePtr<Allocation>* pAlloc)
{
	const DeviceInterface&      vk                  = m_context.getDeviceInterface();
	const VkDevice              vkDevice            = m_context.getDevice();
	const deUint32              queueFamilyIndex    = m_context.getUniversalQueueFamilyIndex();
	SimpleAllocator             memAlloc            (vk, vkDevice, getPhysicalDeviceMemoryProperties(m_context.getInstanceInterface(), m_context.getPhysicalDevice()));

	// Optimal tiling feature check
	VkFormatProperties          formatProperty;
	m_context.getInstanceInterface().getPhysicalDeviceFormatProperties(m_context.getPhysicalDevice(), format, &formatProperty);
	if((usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) && !(formatProperty.optimalTilingFeatures & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT))
	{
		// Remove color attachment usage if the optimal tiling feature does not support it
		usage &= ~VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	}
	if((usage & VK_IMAGE_USAGE_STORAGE_BIT) && !(formatProperty.optimalTilingFeatures & VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT))
	{
		// Remove storage usage if the optimal tiling feature does not support it
		usage &= ~VK_IMAGE_USAGE_STORAGE_BIT;
	}

	const VkImageCreateInfo colorImageParams =
	{
		VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,                                        // VkStructureType      sType;
		DE_NULL,                                                                    // const void*          pNext;
		0u,                                                                         // VkImageCreateFlags   flags;
		VK_IMAGE_TYPE_2D,                                                           // VkImageType          imageType;
		format,                                                                     // VkFormat             format;
		{ width, height, 1u },                                                      // VkExtent3D           extent;
		1u,                                                                         // deUint32             mipLevels;
		1u,                                                                         // deUint32             arraySize;
		sampleCount,                                                                // deUint32             samples;
		VK_IMAGE_TILING_OPTIMAL,                                                    // VkImageTiling        tiling;
		usage,                                                                      // VkImageUsageFlags    usage;
		VK_SHARING_MODE_EXCLUSIVE,                                                  // VkSharingMode        sharingMode;
		1u,                                                                         // deUint32             queueFamilyCount;
		&queueFamilyIndex,                                                          // const deUint32*      pQueueFamilyIndices;
		VK_IMAGE_LAYOUT_UNDEFINED,                                                  // VkImageLayout        initialLayout;
	};

	Move<VkImage> image = createImage(vk, vkDevice, &colorImageParams);

	// Allocate and bind image memory
	de::MovePtr<Allocation> colorImageAlloc = memAlloc.allocate(getImageMemoryRequirements(vk, vkDevice, *image), MemoryRequirement::Any);
	VK_CHECK(vk.bindImageMemory(vkDevice, *image, colorImageAlloc->getMemory(), colorImageAlloc->getOffset()));

	DE_ASSERT(pAlloc);
	*pAlloc = colorImageAlloc;

	return image;
}

class BasicGraphicsTest : public TimestampTest
{
public:
						  BasicGraphicsTest(tcu::TestContext&         testContext,
											const std::string&        name,
											const std::string&        description,
											const TimestampTestParam* param)
							  : TimestampTest (testContext, name, description, param)
							  { }
	virtual               ~BasicGraphicsTest (void) { }
	virtual void          initPrograms       (SourceCollections&      programCollection) const;
	virtual TestInstance* createInstance     (Context&                context) const;
};

class BasicGraphicsTestInstance : public TimestampTestInstance
{
public:
	enum
	{
		VK_MAX_SHADER_STAGES = 6,
	};
				 BasicGraphicsTestInstance  (Context&              context,
											 const StageFlagVector stages,
											 const bool            inRenderPass);
	virtual      ~BasicGraphicsTestInstance (void);
protected:
	virtual void configCommandBuffer        (void);
	virtual void buildVertexBuffer          (void);
	virtual void buildRenderPass            (VkFormat colorFormat,
											 VkFormat depthFormat);
	virtual void buildFrameBuffer           (tcu::UVec2 renderSize,
											 VkFormat colorFormat,
											 VkFormat depthFormat);
protected:
	const tcu::UVec2                    m_renderSize;
	const VkFormat                      m_colorFormat;
	const VkFormat                      m_depthFormat;

	Move<VkImage>                       m_colorImage;
	de::MovePtr<Allocation>             m_colorImageAlloc;
	Move<VkImage>                       m_depthImage;
	de::MovePtr<Allocation>             m_depthImageAlloc;
	Move<VkImageView>                   m_colorAttachmentView;
	Move<VkImageView>                   m_depthAttachmentView;
	Move<VkRenderPass>                  m_renderPass;
	Move<VkFramebuffer>                 m_framebuffer;
	VkImageMemoryBarrier				m_imageLayoutBarriers[2];

	de::MovePtr<Allocation>             m_vertexBufferAlloc;
	Move<VkBuffer>                      m_vertexBuffer;
	std::vector<Vertex4RGBA>            m_vertices;

	SimpleGraphicsPipelineBuilder       m_pipelineBuilder;
	Move<VkPipeline>                    m_graphicsPipelines;
};

void BasicGraphicsTest::initPrograms (SourceCollections& programCollection) const
{
	programCollection.glslSources.add("color_vert") << glu::VertexSource(
		"#version 310 es\n"
		"layout(location = 0) in vec4 position;\n"
		"layout(location = 1) in vec4 color;\n"
		"layout(location = 0) out highp vec4 vtxColor;\n"
		"void main (void)\n"
		"{\n"
		"  gl_Position = position;\n"
		"  vtxColor = color;\n"
		"}\n");

	programCollection.glslSources.add("color_frag") << glu::FragmentSource(
		"#version 310 es\n"
		"layout(location = 0) in highp vec4 vtxColor;\n"
		"layout(location = 0) out highp vec4 fragColor;\n"
		"void main (void)\n"
		"{\n"
		"  fragColor = vtxColor;\n"
		"}\n");
}

TestInstance* BasicGraphicsTest::createInstance(Context& context) const
{
	return new BasicGraphicsTestInstance(context,m_stages,m_inRenderPass);
}

void BasicGraphicsTestInstance::buildVertexBuffer(void)
{
	const DeviceInterface&      vk       = m_context.getDeviceInterface();
	const VkDevice              vkDevice = m_context.getDevice();

	// Create vertex buffer
	{
		m_vertexBuffer = createBufferAndBindMemory(1024u, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, &m_vertexBufferAlloc);

		m_vertices          = createOverlappingQuads();
		// Load vertices into vertex buffer
		deMemcpy(m_vertexBufferAlloc->getHostPtr(), m_vertices.data(), m_vertices.size() * sizeof(Vertex4RGBA));
		flushMappedMemoryRange(vk, vkDevice, m_vertexBufferAlloc->getMemory(), m_vertexBufferAlloc->getOffset(), 1024u);
	}
}

void BasicGraphicsTestInstance::buildRenderPass(VkFormat colorFormat, VkFormat depthFormat)
{
	const DeviceInterface&      vk       = m_context.getDeviceInterface();
	const VkDevice              vkDevice = m_context.getDevice();

	// Create render pass
	{
		const VkAttachmentDescription colorAttachmentDescription =
		{
			0u,                                                 // VkAttachmentDescriptionFlags    flags;
			colorFormat,                                        // VkFormat                        format;
			VK_SAMPLE_COUNT_1_BIT,                              // VkSampleCountFlagBits           samples;
			VK_ATTACHMENT_LOAD_OP_CLEAR,                        // VkAttachmentLoadOp              loadOp;
			VK_ATTACHMENT_STORE_OP_STORE,                       // VkAttachmentStoreOp             storeOp;
			VK_ATTACHMENT_LOAD_OP_DONT_CARE,                    // VkAttachmentLoadOp              stencilLoadOp;
			VK_ATTACHMENT_STORE_OP_DONT_CARE,                   // VkAttachmentStoreOp             stencilStoreOp;
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,           // VkImageLayout                   initialLayout;
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,           // VkImageLayout                   finalLayout;
		};

		const VkAttachmentDescription depthAttachmentDescription =
		{
			0u,                                                 // VkAttachmentDescriptionFlags flags;
			depthFormat,                                        // VkFormat                     format;
			VK_SAMPLE_COUNT_1_BIT,                              // VkSampleCountFlagBits        samples;
			VK_ATTACHMENT_LOAD_OP_CLEAR,                        // VkAttachmentLoadOp           loadOp;
			VK_ATTACHMENT_STORE_OP_DONT_CARE,                   // VkAttachmentStoreOp          storeOp;
			VK_ATTACHMENT_LOAD_OP_DONT_CARE,                    // VkAttachmentLoadOp           stencilLoadOp;
			VK_ATTACHMENT_STORE_OP_DONT_CARE,                   // VkAttachmentStoreOp          stencilStoreOp;
			VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,   // VkImageLayout                initialLayout;
			VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,   // VkImageLayout                finalLayout;
		};

		const VkAttachmentDescription attachments[2] =
		{
			colorAttachmentDescription,
			depthAttachmentDescription
		};

		const VkAttachmentReference colorAttachmentReference =
		{
			0u,                                                 // deUint32         attachment;
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL            // VkImageLayout    layout;
		};

		const VkAttachmentReference depthAttachmentReference =
		{
			1u,                                                 // deUint32         attachment;
			VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL    // VkImageLayout    layout;
		};

		const VkSubpassDescription subpassDescription =
		{
			0u,                                                 // VkSubpassDescriptionFlags        flags;
			VK_PIPELINE_BIND_POINT_GRAPHICS,                    // VkPipelineBindPoint              pipelineBindPoint;
			0u,                                                 // deUint32                         inputAttachmentCount;
			DE_NULL,                                            // const VkAttachmentReference*     pInputAttachments;
			1u,                                                 // deUint32                         colorAttachmentCount;
			&colorAttachmentReference,                          // const VkAttachmentReference*     pColorAttachments;
			DE_NULL,                                            // const VkAttachmentReference*     pResolveAttachments;
			&depthAttachmentReference,                          // const VkAttachmentReference*     pDepthStencilAttachment;
			0u,                                                 // deUint32                         preserveAttachmentCount;
			DE_NULL                                             // const VkAttachmentReference*     pPreserveAttachments;
		};

		const VkRenderPassCreateInfo renderPassParams =
		{
			VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,          // VkStructureType                  sType;
			DE_NULL,                                            // const void*                      pNext;
			0u,                                                 // VkRenderPassCreateFlags          flags;
			2u,                                                 // deUint32                         attachmentCount;
			attachments,                                        // const VkAttachmentDescription*   pAttachments;
			1u,                                                 // deUint32                         subpassCount;
			&subpassDescription,                                // const VkSubpassDescription*      pSubpasses;
			0u,                                                 // deUint32                         dependencyCount;
			DE_NULL                                             // const VkSubpassDependency*       pDependencies;
		};

		m_renderPass = createRenderPass(vk, vkDevice, &renderPassParams);
	}

}

void BasicGraphicsTestInstance::buildFrameBuffer(tcu::UVec2 renderSize, VkFormat colorFormat, VkFormat depthFormat)
{
	const DeviceInterface&      vk                   = m_context.getDeviceInterface();
	const VkDevice              vkDevice             = m_context.getDevice();
	const VkComponentMapping    ComponentMappingRGBA = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A};

	// Create color image
	{
		m_colorImage = createImage2DAndBindMemory(colorFormat,
												  renderSize.x(),
												  renderSize.y(),
												  VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
												  VK_SAMPLE_COUNT_1_BIT,
												  &m_colorImageAlloc);
	}

	// Create depth image
	{
		m_depthImage = createImage2DAndBindMemory(depthFormat,
												  renderSize.x(),
												  renderSize.y(),
												  VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
												  VK_SAMPLE_COUNT_1_BIT,
												  &m_depthImageAlloc);
	}

	// Set up image layout transition barriers
	{
		const VkImageMemoryBarrier colorImageBarrier =
		{
			VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,				// VkStructureType			sType;
			DE_NULL,											// const void*				pNext;
			0u,													// VkAccessFlags			srcAccessMask;
			VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,				// VkAccessFlags			dstAccessMask;
			VK_IMAGE_LAYOUT_UNDEFINED,							// VkImageLayout			oldLayout;
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,			// VkImageLayout			newLayout;
			VK_QUEUE_FAMILY_IGNORED,							// deUint32					srcQueueFamilyIndex;
			VK_QUEUE_FAMILY_IGNORED,							// deUint32					dstQueueFamilyIndex;
			*m_colorImage,										// VkImage					image;
			{ VK_IMAGE_ASPECT_COLOR_BIT, 0u, 1u, 0u, 1u },		// VkImageSubresourceRange	subresourceRange;
		};
		const VkImageMemoryBarrier depthImageBarrier =
		{
			VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,				// VkStructureType			sType;
			DE_NULL,											// const void*				pNext;
			0u,													// VkAccessFlags			srcAccessMask;
			VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,		// VkAccessFlags			dstAccessMask;
			VK_IMAGE_LAYOUT_UNDEFINED,							// VkImageLayout			oldLayout;
			VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,	// VkImageLayout			newLayout;
			VK_QUEUE_FAMILY_IGNORED,							// deUint32					srcQueueFamilyIndex;
			VK_QUEUE_FAMILY_IGNORED,							// deUint32					dstQueueFamilyIndex;
			*m_depthImage,										// VkImage					image;
			{ VK_IMAGE_ASPECT_DEPTH_BIT, 0u, 1u, 0u, 1u },		// VkImageSubresourceRange	subresourceRange;
		};

		m_imageLayoutBarriers[0] = colorImageBarrier;
		m_imageLayoutBarriers[1] = depthImageBarrier;
	}

	// Create color attachment view
	{
		const VkImageViewCreateInfo colorAttachmentViewParams =
		{
			VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,       // VkStructureType          sType;
			DE_NULL,                                        // const void*              pNext;
			0u,                                             // VkImageViewCreateFlags   flags;
			*m_colorImage,                                  // VkImage                  image;
			VK_IMAGE_VIEW_TYPE_2D,                          // VkImageViewType          viewType;
			colorFormat,                                    // VkFormat                 format;
			ComponentMappingRGBA,                           // VkComponentMapping       components;
			{ VK_IMAGE_ASPECT_COLOR_BIT, 0u, 1u, 0u, 1u },  // VkImageSubresourceRange  subresourceRange;
		};

		m_colorAttachmentView = createImageView(vk, vkDevice, &colorAttachmentViewParams);
	}

	// Create depth attachment view
	{
		const VkImageViewCreateInfo depthAttachmentViewParams =
		{
			VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,       // VkStructureType          sType;
			DE_NULL,                                        // const void*              pNext;
			0u,                                             // VkImageViewCreateFlags   flags;
			*m_depthImage,                                  // VkImage                  image;
			VK_IMAGE_VIEW_TYPE_2D,                          // VkImageViewType          viewType;
			depthFormat,                                    // VkFormat                 format;
			ComponentMappingRGBA,                           // VkComponentMapping       components;
			{ VK_IMAGE_ASPECT_DEPTH_BIT, 0u, 1u, 0u, 1u },  // VkImageSubresourceRange  subresourceRange;
		};

		m_depthAttachmentView = createImageView(vk, vkDevice, &depthAttachmentViewParams);
	}

	// Create framebuffer
	{
		const VkImageView attachmentBindInfos[2] =
		{
			*m_colorAttachmentView,
			*m_depthAttachmentView,
		};

		const VkFramebufferCreateInfo framebufferParams =
		{
			VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,          // VkStructureType              sType;
			DE_NULL,                                            // const void*                  pNext;
			0u,                                                 // VkFramebufferCreateFlags     flags;
			*m_renderPass,                                      // VkRenderPass                 renderPass;
			2u,                                                 // deUint32                     attachmentCount;
			attachmentBindInfos,                                // const VkImageView*           pAttachments;
			(deUint32)renderSize.x(),                           // deUint32                     width;
			(deUint32)renderSize.y(),                           // deUint32                     height;
			1u,                                                 // deUint32                     layers;
		};

		m_framebuffer = createFramebuffer(vk, vkDevice, &framebufferParams);
	}

}

BasicGraphicsTestInstance::BasicGraphicsTestInstance(Context&              context,
													 const StageFlagVector stages,
													 const bool            inRenderPass)
													 : TimestampTestInstance (context,stages,inRenderPass)
													 , m_renderSize  (32, 32)
													 , m_colorFormat (VK_FORMAT_R8G8B8A8_UNORM)
													 , m_depthFormat (VK_FORMAT_D16_UNORM)
													 , m_pipelineBuilder (context)
{
	buildVertexBuffer();

	buildRenderPass(m_colorFormat, m_depthFormat);

	buildFrameBuffer(m_renderSize, m_colorFormat, m_depthFormat);

	m_pipelineBuilder.bindShaderStage(VK_SHADER_STAGE_VERTEX_BIT, "color_vert", "main");
	m_pipelineBuilder.bindShaderStage(VK_SHADER_STAGE_FRAGMENT_BIT, "color_frag", "main");

	m_graphicsPipelines = m_pipelineBuilder.buildPipeline(m_renderSize, *m_renderPass);

}

BasicGraphicsTestInstance::~BasicGraphicsTestInstance(void)
{
}

void BasicGraphicsTestInstance::configCommandBuffer(void)
{
	const DeviceInterface&      vk                  = m_context.getDeviceInterface();

	const VkCommandBufferBeginInfo cmdBufferBeginInfo =
	{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,    // VkStructureType                  sType;
		DE_NULL,                                        // const void*                      pNext;
		0u,                                             // VkCommandBufferUsageFlags        flags;
		(const VkCommandBufferInheritanceInfo*)DE_NULL,
	};

	const VkClearValue attachmentClearValues[2] =
	{
		defaultClearValue(m_colorFormat),
		defaultClearValue(m_depthFormat),
	};

	const VkRenderPassBeginInfo renderPassBeginInfo =
	{
		VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,               // VkStructureType      sType;
		DE_NULL,                                                // const void*          pNext;
		*m_renderPass,                                          // VkRenderPass         renderPass;
		*m_framebuffer,                                         // VkFramebuffer        framebuffer;
		{ { 0u, 0u }, { m_renderSize.x(), m_renderSize.y() } }, // VkRect2D             renderArea;
		2u,                                                     // deUint32             clearValueCount;
		attachmentClearValues                                   // const VkClearValue*  pClearValues;
	};

	VK_CHECK(vk.beginCommandBuffer(*m_cmdBuffer, &cmdBufferBeginInfo));

	vk.cmdPipelineBarrier(*m_cmdBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, (VkDependencyFlags)0,
		0u, DE_NULL, 0u, DE_NULL, DE_LENGTH_OF_ARRAY(m_imageLayoutBarriers), m_imageLayoutBarriers);

	vk.cmdResetQueryPool(*m_cmdBuffer, *m_queryPool, 0u, TimestampTest::ENTRY_COUNT);

	vk.cmdBeginRenderPass(*m_cmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

	vk.cmdBindPipeline(*m_cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *m_graphicsPipelines);
	VkDeviceSize offsets = 0u;
	vk.cmdBindVertexBuffers(*m_cmdBuffer, 0u, 1u, &m_vertexBuffer.get(), &offsets);
	vk.cmdDraw(*m_cmdBuffer, (deUint32)m_vertices.size(), 1u, 0u, 0u);

	if(m_inRenderPass)
	{
	  deUint32 timestampEntry = 0u;
	  for (StageFlagVector::const_iterator it = m_stages.begin(); it != m_stages.end(); it++)
	  {
		  vk.cmdWriteTimestamp(*m_cmdBuffer, *it, *m_queryPool, timestampEntry++);
	  }
	}

	vk.cmdEndRenderPass(*m_cmdBuffer);

	if(!m_inRenderPass)
	{
	  deUint32 timestampEntry = 0u;
	  for (StageFlagVector::const_iterator it = m_stages.begin(); it != m_stages.end(); it++)
	  {
		  vk.cmdWriteTimestamp(*m_cmdBuffer, *it, *m_queryPool, timestampEntry++);
	  }
	}

	VK_CHECK(vk.endCommandBuffer(*m_cmdBuffer));
}

class AdvGraphicsTest : public BasicGraphicsTest
{
public:
						  AdvGraphicsTest  (tcu::TestContext&         testContext,
											const std::string&        name,
											const std::string&        description,
											const TimestampTestParam* param)
							  : BasicGraphicsTest(testContext, name, description, param)
							  { }
	virtual               ~AdvGraphicsTest (void) { }
	virtual void          initPrograms     (SourceCollections&        programCollection) const;
	virtual TestInstance* createInstance   (Context&                  context) const;
};

class AdvGraphicsTestInstance : public BasicGraphicsTestInstance
{
public:
				 AdvGraphicsTestInstance  (Context&              context,
										   const StageFlagVector stages,
										   const bool            inRenderPass);
	virtual      ~AdvGraphicsTestInstance (void);
	virtual void configCommandBuffer      (void);
protected:
	virtual void featureSupportCheck      (void);
protected:
	VkPhysicalDeviceFeatures m_features;
	deUint32                 m_draw_count;
	de::MovePtr<Allocation>  m_indirectBufferAlloc;
	Move<VkBuffer>           m_indirectBuffer;
};

void AdvGraphicsTest::initPrograms(SourceCollections& programCollection) const
{
	BasicGraphicsTest::initPrograms(programCollection);

	programCollection.glslSources.add("dummy_geo") << glu::GeometrySource(
		"#version 310 es\n"
		"#extension GL_EXT_geometry_shader : enable\n"
		"layout(triangles) in;\n"
		"layout(triangle_strip, max_vertices = 3) out;\n"
		"layout(location = 0) in highp vec4 in_vtxColor[];\n"
		"layout(location = 0) out highp vec4 vtxColor;\n"
		"void main (void)\n"
		"{\n"
		"  for(int ndx=0; ndx<3; ndx++)\n"
		"  {\n"
		"    gl_Position = gl_in[ndx].gl_Position;\n"
		"    vtxColor    = in_vtxColor[ndx];\n"
		"    EmitVertex();\n"
		"  }\n"
		"  EndPrimitive();\n"
		"}\n");

	programCollection.glslSources.add("basic_tcs") << glu::TessellationControlSource(
		"#version 310 es\n"
		"#extension GL_EXT_tessellation_shader : enable\n"
		"layout(vertices = 3) out;\n"
		"layout(location = 0) in highp vec4 color[];\n"
		"layout(location = 0) out highp vec4 vtxColor[];\n"
		"void main()\n"
		"{\n"
		"  gl_TessLevelOuter[0] = 4.0;\n"
		"  gl_TessLevelOuter[1] = 4.0;\n"
		"  gl_TessLevelOuter[2] = 4.0;\n"
		"  gl_TessLevelInner[0] = 4.0;\n"
		"  gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;\n"
		"  vtxColor[gl_InvocationID] = color[gl_InvocationID];\n"
		"}\n");

	programCollection.glslSources.add("basic_tes") << glu::TessellationEvaluationSource(
		"#version 310 es\n"
		"#extension GL_EXT_tessellation_shader : enable\n"
		"layout(triangles, fractional_even_spacing, ccw) in;\n"
		"layout(location = 0) in highp vec4 colors[];\n"
		"layout(location = 0) out highp vec4 vtxColor;\n"
		"void main() \n"
		"{\n"
		"  float u = gl_TessCoord.x;\n"
		"  float v = gl_TessCoord.y;\n"
		"  float w = gl_TessCoord.z;\n"
		"  vec4 pos = vec4(0);\n"
		"  vec4 color = vec4(0);\n"
		"  pos.xyz += u * gl_in[0].gl_Position.xyz;\n"
		"  color.xyz += u * colors[0].xyz;\n"
		"  pos.xyz += v * gl_in[1].gl_Position.xyz;\n"
		"  color.xyz += v * colors[1].xyz;\n"
		"  pos.xyz += w * gl_in[2].gl_Position.xyz;\n"
		"  color.xyz += w * colors[2].xyz;\n"
		"  pos.w = 1.0;\n"
		"  color.w = 1.0;\n"
		"  gl_Position = pos;\n"
		"  vtxColor = color;\n"
		"}\n");
}

TestInstance* AdvGraphicsTest::createInstance(Context& context) const
{
	return new AdvGraphicsTestInstance(context,m_stages,m_inRenderPass);
}

void AdvGraphicsTestInstance::featureSupportCheck(void)
{
	for (StageFlagVector::const_iterator it = m_stages.begin(); it != m_stages.end(); it++)
	{
		switch(*it)
		{
			case VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT:
				if (m_features.geometryShader == VK_FALSE)
				{
					TCU_THROW(NotSupportedError, "Geometry Shader Not Supported");
				}
				break;
			case VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT:
			case VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT:
				if (m_features.tessellationShader == VK_FALSE)
				{
					TCU_THROW(NotSupportedError, "Tessellation Not Supported");
				}
				break;
			case VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT:
			default:
				break;
		};
	}
}

AdvGraphicsTestInstance::AdvGraphicsTestInstance(Context&              context,
												 const StageFlagVector stages,
												 const bool            inRenderPass)
	: BasicGraphicsTestInstance(context, stages, inRenderPass)
{
	m_features = m_context.getDeviceFeatures();

	// If necessary feature is not supported, throw error and fail current test
	featureSupportCheck();

	if(m_features.geometryShader == VK_TRUE)
	{
		m_pipelineBuilder.bindShaderStage(VK_SHADER_STAGE_GEOMETRY_BIT, "dummy_geo", "main");
	}

	if(m_features.tessellationShader == VK_TRUE)
	{
		m_pipelineBuilder.bindShaderStage(VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT, "basic_tcs", "main");
		m_pipelineBuilder.bindShaderStage(VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT, "basic_tes", "main");
		m_pipelineBuilder.enableTessellationStage(3);
	}

	m_graphicsPipelines = m_pipelineBuilder.buildPipeline(m_renderSize, *m_renderPass);

	// Prepare the indirect draw buffer
	const DeviceInterface&      vk                  = m_context.getDeviceInterface();
	const VkDevice              vkDevice            = m_context.getDevice();

	if(m_features.multiDrawIndirect == VK_TRUE)
	{
		m_draw_count = 2;
	}
	else
	{
		m_draw_count = 1;
	}
	m_indirectBuffer = createBufferAndBindMemory(32u, VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT, &m_indirectBufferAlloc);

	const VkDrawIndirectCommand indirectCmds[] =
	{
		{
			12u,                    // deUint32    vertexCount;
			1u,                     // deUint32    instanceCount;
			0u,                     // deUint32    firstVertex;
			0u,                     // deUint32    firstInstance;
		},
		{
			12u,                    // deUint32    vertexCount;
			1u,                     // deUint32    instanceCount;
			11u,                    // deUint32    firstVertex;
			0u,                     // deUint32    firstInstance;
		},
	};
	// Load data into indirect draw buffer
	deMemcpy(m_indirectBufferAlloc->getHostPtr(), indirectCmds, m_draw_count * sizeof(VkDrawIndirectCommand));
	flushMappedMemoryRange(vk, vkDevice, m_indirectBufferAlloc->getMemory(), m_indirectBufferAlloc->getOffset(), 32u);

}

AdvGraphicsTestInstance::~AdvGraphicsTestInstance(void)
{
}

void AdvGraphicsTestInstance::configCommandBuffer(void)
{
	const DeviceInterface&      vk                  = m_context.getDeviceInterface();

	const VkCommandBufferBeginInfo cmdBufferBeginInfo =
	{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,    // VkStructureType              sType;
		DE_NULL,                                        // const void*                  pNext;
		0u,                                             // VkCommandBufferUsageFlags    flags;
		(const VkCommandBufferInheritanceInfo*)DE_NULL,
	};

	const VkClearValue attachmentClearValues[2] =
	{
		defaultClearValue(m_colorFormat),
		defaultClearValue(m_depthFormat),
	};

	const VkRenderPassBeginInfo renderPassBeginInfo =
	{
		VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,               // VkStructureType      sType;
		DE_NULL,                                                // const void*          pNext;
		*m_renderPass,                                          // VkRenderPass         renderPass;
		*m_framebuffer,                                         // VkFramebuffer        framebuffer;
		{ { 0u, 0u }, { m_renderSize.x(), m_renderSize.y() } }, // VkRect2D             renderArea;
		2u,                                                     // deUint32             clearValueCount;
		attachmentClearValues                                   // const VkClearValue*  pClearValues;
	};

	VK_CHECK(vk.beginCommandBuffer(*m_cmdBuffer, &cmdBufferBeginInfo));

	vk.cmdPipelineBarrier(*m_cmdBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, (VkDependencyFlags)0,
		0u, DE_NULL, 0u, DE_NULL, DE_LENGTH_OF_ARRAY(m_imageLayoutBarriers), m_imageLayoutBarriers);

	vk.cmdResetQueryPool(*m_cmdBuffer, *m_queryPool, 0u, TimestampTest::ENTRY_COUNT);

	vk.cmdBeginRenderPass(*m_cmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

	vk.cmdBindPipeline(*m_cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *m_graphicsPipelines);

	VkDeviceSize offsets = 0u;
	vk.cmdBindVertexBuffers(*m_cmdBuffer, 0u, 1u, &m_vertexBuffer.get(), &offsets);

	vk.cmdDrawIndirect(*m_cmdBuffer, *m_indirectBuffer, 0u, m_draw_count, sizeof(VkDrawIndirectCommand));

	if(m_inRenderPass)
	{
	  deUint32 timestampEntry = 0u;
	  for (StageFlagVector::const_iterator it = m_stages.begin(); it != m_stages.end(); it++)
	  {
		  vk.cmdWriteTimestamp(*m_cmdBuffer, *it, *m_queryPool, timestampEntry++);
	  }
	}

	vk.cmdEndRenderPass(*m_cmdBuffer);

	if(!m_inRenderPass)
	{
	  deUint32 timestampEntry = 0u;
	  for (StageFlagVector::const_iterator it = m_stages.begin(); it != m_stages.end(); it++)
	  {
		  vk.cmdWriteTimestamp(*m_cmdBuffer, *it, *m_queryPool, timestampEntry++);
	  }
	}

	VK_CHECK(vk.endCommandBuffer(*m_cmdBuffer));

}

class BasicComputeTest : public TimestampTest
{
public:
						  BasicComputeTest  (tcu::TestContext&         testContext,
											 const std::string&        name,
											 const std::string&        description,
											 const TimestampTestParam* param)
							  : TimestampTest(testContext, name, description, param)
							  { }
	virtual               ~BasicComputeTest (void) { }
	virtual void          initPrograms      (SourceCollections&        programCollection) const;
	virtual TestInstance* createInstance    (Context&                  context) const;
};

class BasicComputeTestInstance : public TimestampTestInstance
{
public:
				 BasicComputeTestInstance  (Context&              context,
											const StageFlagVector stages,
											const bool            inRenderPass);
	virtual      ~BasicComputeTestInstance (void);
	virtual void configCommandBuffer       (void);
protected:
	de::MovePtr<Allocation>     m_inputBufAlloc;
	Move<VkBuffer>              m_inputBuf;
	de::MovePtr<Allocation>     m_outputBufAlloc;
	Move<VkBuffer>              m_outputBuf;

	Move<VkDescriptorPool>      m_descriptorPool;
	Move<VkDescriptorSet>       m_descriptorSet;
	Move<VkDescriptorSetLayout> m_descriptorSetLayout;

	Move<VkPipelineLayout>      m_pipelineLayout;
	Move<VkShaderModule>        m_computeShaderModule;
	Move<VkPipeline>            m_computePipelines;
};

void BasicComputeTest::initPrograms(SourceCollections& programCollection) const
{
	TimestampTest::initPrograms(programCollection);

	programCollection.glslSources.add("basic_compute") << glu::ComputeSource(
		"#version 310 es\n"
		"layout(local_size_x = 128) in;\n"
		"layout(std430) buffer;\n"
		"layout(binding = 0) readonly buffer Input0\n"
		"{\n"
		"  vec4 elements[];\n"
		"} input_data0;\n"
		"layout(binding = 1) writeonly buffer Output\n"
		"{\n"
		"  vec4 elements[];\n"
		"} output_data;\n"
		"void main()\n"
		"{\n"
		"  uint ident = gl_GlobalInvocationID.x;\n"
		"  output_data.elements[ident] = input_data0.elements[ident] * input_data0.elements[ident];\n"
		"}");
}

TestInstance* BasicComputeTest::createInstance(Context& context) const
{
	return new BasicComputeTestInstance(context,m_stages,m_inRenderPass);
}

BasicComputeTestInstance::BasicComputeTestInstance(Context&              context,
												   const StageFlagVector stages,
												   const bool            inRenderPass)
	: TimestampTestInstance(context, stages, inRenderPass)
{
	const DeviceInterface&      vk                  = context.getDeviceInterface();
	const VkDevice              vkDevice            = context.getDevice();

	// Create buffer object, allocate storage, and generate input data
	const VkDeviceSize          size                = sizeof(tcu::Vec4) * 128u * 128u;
	m_inputBuf = createBufferAndBindMemory(size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, &m_inputBufAlloc);
	// Load vertices into buffer
	tcu::Vec4* pVec = reinterpret_cast<tcu::Vec4*>(m_inputBufAlloc->getHostPtr());
	for (deUint32 ndx = 0u; ndx < (128u * 128u); ndx++)
	{
		for (deUint32 component = 0u; component < 4u; component++)
		{
			pVec[ndx][component]= (float)(ndx * (component + 1u));
		}
	}
	flushMappedMemoryRange(vk, vkDevice, m_inputBufAlloc->getMemory(), m_inputBufAlloc->getOffset(), size);

	m_outputBuf = createBufferAndBindMemory(size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, &m_outputBufAlloc);

	std::vector<VkDescriptorBufferInfo>        descriptorInfos;
	descriptorInfos.push_back(makeDescriptorBufferInfo(*m_inputBuf, 0u, size));
	descriptorInfos.push_back(makeDescriptorBufferInfo(*m_outputBuf, 0u, size));

	// Create descriptor set layout
	DescriptorSetLayoutBuilder descLayoutBuilder;

	for (deUint32 bindingNdx = 0u; bindingNdx < 2u; bindingNdx++)
	{
		descLayoutBuilder.addSingleBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT);
	}

	m_descriptorSetLayout = descLayoutBuilder.build(vk, vkDevice);

	// Create descriptor pool
	m_descriptorPool = DescriptorPoolBuilder().addType(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2).build(vk, vkDevice, VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT, 1u);

	// Create descriptor set
	const VkDescriptorSetAllocateInfo descriptorSetAllocInfo =
	{
		VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,     // VkStructureType                 sType;
		DE_NULL,                                            // const void*                     pNext;
		*m_descriptorPool,                                  // VkDescriptorPool                descriptorPool;
		1u,                                                 // deUint32                        setLayoutCount;
		&m_descriptorSetLayout.get(),                       // const VkDescriptorSetLayout*    pSetLayouts;
	};
	m_descriptorSet   = allocateDescriptorSet(vk, vkDevice, &descriptorSetAllocInfo);

	DescriptorSetUpdateBuilder  builder;
	for (deUint32 descriptorNdx = 0u; descriptorNdx < 2u; descriptorNdx++)
	{
		builder.writeSingle(*m_descriptorSet, DescriptorSetUpdateBuilder::Location::binding(descriptorNdx), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, &descriptorInfos[descriptorNdx]);
	}
	builder.update(vk, vkDevice);

	// Create compute pipeline layout
	const VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo =
	{
		VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,  // VkStructureType                 sType;
		DE_NULL,                                        // const void*                     pNext;
		0u,                                             // VkPipelineLayoutCreateFlags     flags;
		1u,                                             // deUint32                        setLayoutCount;
		&m_descriptorSetLayout.get(),                   // const VkDescriptorSetLayout*    pSetLayouts;
		0u,                                             // deUint32                        pushConstantRangeCount;
		DE_NULL,                                        // const VkPushConstantRange*      pPushConstantRanges;
	};

	m_pipelineLayout = createPipelineLayout(vk, vkDevice, &pipelineLayoutCreateInfo);

	// Create compute shader
	VkShaderModuleCreateInfo shaderModuleCreateInfo =
	{
		VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,                        // VkStructureType             sType;
		DE_NULL,                                                            // const void*                 pNext;
		0u,                                                                 // VkShaderModuleCreateFlags   flags;
		m_context.getBinaryCollection().get("basic_compute").getSize(),     // deUintptr                   codeSize;
		(deUint32*)m_context.getBinaryCollection().get("basic_compute").getBinary(),   // const deUint32*             pCode;

	};

	m_computeShaderModule = createShaderModule(vk, vkDevice, &shaderModuleCreateInfo);

	// Create compute pipeline
	const VkPipelineShaderStageCreateInfo stageCreateInfo =
	{
		VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, // VkStructureType                     sType;
		DE_NULL,                                             // const void*                         pNext;
		0u,                                                  // VkPipelineShaderStageCreateFlags    flags;
		VK_SHADER_STAGE_COMPUTE_BIT,                         // VkShaderStageFlagBits               stage;
		*m_computeShaderModule,                              // VkShaderModule                      module;
		"main",                                              // const char*                         pName;
		DE_NULL,                                             // const VkSpecializationInfo*         pSpecializationInfo;
	};

	const VkComputePipelineCreateInfo pipelineCreateInfo =
	{
		VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,      // VkStructureType                 sType;
		DE_NULL,                                             // const void*                     pNext;
		0u,                                                  // VkPipelineCreateFlags           flags;
		stageCreateInfo,                                     // VkPipelineShaderStageCreateInfo stage;
		*m_pipelineLayout,                                   // VkPipelineLayout                layout;
		(VkPipeline)0,                                       // VkPipeline                      basePipelineHandle;
		0u,                                                  // deInt32                         basePipelineIndex;
	};

	m_computePipelines = createComputePipeline(vk, vkDevice, (VkPipelineCache)0u, &pipelineCreateInfo);

}

BasicComputeTestInstance::~BasicComputeTestInstance(void)
{
}

void BasicComputeTestInstance::configCommandBuffer(void)
{
	const DeviceInterface&     vk                 = m_context.getDeviceInterface();

	const VkCommandBufferBeginInfo cmdBufferBeginInfo =
	{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,    // VkStructureType          sType;
		DE_NULL,                                        // const void*              pNext;
		0u,                                             // VkCmdBufferOptimizeFlags flags;
		(const VkCommandBufferInheritanceInfo*)DE_NULL,

	};

	VK_CHECK(vk.beginCommandBuffer(*m_cmdBuffer, &cmdBufferBeginInfo));

	vk.cmdResetQueryPool(*m_cmdBuffer, *m_queryPool, 0u, TimestampTest::ENTRY_COUNT);

	vk.cmdBindPipeline(*m_cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, *m_computePipelines);
	vk.cmdBindDescriptorSets(*m_cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, *m_pipelineLayout, 0u, 1u, &m_descriptorSet.get(), 0u, DE_NULL);
	vk.cmdDispatch(*m_cmdBuffer, 128u, 1u, 1u);

	deUint32 timestampEntry = 0u;
	for (StageFlagVector::const_iterator it = m_stages.begin(); it != m_stages.end(); it++)
	{
		vk.cmdWriteTimestamp(*m_cmdBuffer, *it, *m_queryPool, timestampEntry++);
	}

	VK_CHECK(vk.endCommandBuffer(*m_cmdBuffer));


}

class TransferTest : public TimestampTest
{
public:
						  TransferTest   (tcu::TestContext&          testContext,
										  const std::string&         name,
										  const std::string&         description,
										  const TimestampTestParam*  param);
	virtual               ~TransferTest  (void) { }
	virtual void          initPrograms   (SourceCollections&         programCollection) const;
	virtual TestInstance* createInstance (Context&                   context) const;
protected:
	TransferMethod        m_method;
};

class TransferTestInstance : public TimestampTestInstance
{
public:
					TransferTestInstance	(Context&					context,
											 const StageFlagVector		stages,
											 const bool					inRenderPass,
											 const TransferMethod		method);
	virtual         ~TransferTestInstance	(void);
	virtual void    configCommandBuffer		(void);
	virtual void	initialImageTransition	(VkCommandBuffer			cmdBuffer,
											 VkImage					image,
											 VkImageSubresourceRange	subRange,
											 VkImageLayout				layout);
protected:
	TransferMethod			m_method;

	VkDeviceSize			m_bufSize;
	Move<VkBuffer>			m_srcBuffer;
	Move<VkBuffer>			m_dstBuffer;
	de::MovePtr<Allocation> m_srcBufferAlloc;
	de::MovePtr<Allocation> m_dstBufferAlloc;

	VkFormat				m_imageFormat;
	deInt32					m_imageWidth;
	deInt32					m_imageHeight;
	VkDeviceSize			m_imageSize;
	Move<VkImage>			m_srcImage;
	Move<VkImage>			m_dstImage;
	Move<VkImage>			m_depthImage;
	Move<VkImage>			m_msImage;
	de::MovePtr<Allocation>	m_srcImageAlloc;
	de::MovePtr<Allocation>	m_dstImageAlloc;
	de::MovePtr<Allocation>	m_depthImageAlloc;
	de::MovePtr<Allocation>	m_msImageAlloc;
};

TransferTest::TransferTest(tcu::TestContext&                 testContext,
						   const std::string&                name,
						   const std::string&                description,
						   const TimestampTestParam*         param)
	: TimestampTest(testContext, name, description, param)
{
	const TransferTimestampTestParam* transferParam = dynamic_cast<const TransferTimestampTestParam*>(param);
	m_method = transferParam->getMethod();
}

void TransferTest::initPrograms(SourceCollections& programCollection) const
{
	TimestampTest::initPrograms(programCollection);
}

TestInstance* TransferTest::createInstance(Context& context) const
{
  return new TransferTestInstance(context, m_stages, m_inRenderPass, m_method);
}

TransferTestInstance::TransferTestInstance(Context&              context,
										   const StageFlagVector stages,
										   const bool            inRenderPass,
										   const TransferMethod  method)
	: TimestampTestInstance(context, stages, inRenderPass)
	, m_method(method)
	, m_bufSize(256u)
	, m_imageFormat(VK_FORMAT_R8G8B8A8_UNORM)
	, m_imageWidth(4u)
	, m_imageHeight(4u)
	, m_imageSize(256u)
{
	const DeviceInterface&      vk                  = context.getDeviceInterface();
	const VkDevice              vkDevice            = context.getDevice();

	// Create src buffer
	m_srcBuffer = createBufferAndBindMemory(m_bufSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT, &m_srcBufferAlloc);

	// Init the source buffer memory
	char* pBuf = reinterpret_cast<char*>(m_srcBufferAlloc->getHostPtr());
	memset(pBuf, 0xFF, sizeof(char)*(size_t)m_bufSize);
	flushMappedMemoryRange(vk, vkDevice, m_srcBufferAlloc->getMemory(), m_srcBufferAlloc->getOffset(), m_bufSize);

	// Create dst buffer
	m_dstBuffer = createBufferAndBindMemory(m_bufSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, &m_dstBufferAlloc);

	// Create src/dst/depth image
	m_srcImage   = createImage2DAndBindMemory(m_imageFormat, m_imageWidth, m_imageHeight,
											  VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
											  VK_SAMPLE_COUNT_1_BIT,
											  &m_srcImageAlloc);
	m_dstImage   = createImage2DAndBindMemory(m_imageFormat, m_imageWidth, m_imageHeight,
											  VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
											  VK_SAMPLE_COUNT_1_BIT,
											  &m_dstImageAlloc);
	m_depthImage = createImage2DAndBindMemory(VK_FORMAT_D16_UNORM, m_imageWidth, m_imageHeight,
											  VK_IMAGE_USAGE_TRANSFER_DST_BIT,
											  VK_SAMPLE_COUNT_1_BIT,
											  &m_depthImageAlloc);
	m_msImage    = createImage2DAndBindMemory(m_imageFormat, m_imageWidth, m_imageHeight,
											  VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
											  VK_SAMPLE_COUNT_4_BIT,
											  &m_msImageAlloc);
}

TransferTestInstance::~TransferTestInstance(void)
{
}

void TransferTestInstance::configCommandBuffer(void)
{
	const DeviceInterface&      vk                  = m_context.getDeviceInterface();

	const VkCommandBufferBeginInfo cmdBufferBeginInfo =
	{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,    // VkStructureType                  sType;
		DE_NULL,                                        // const void*                      pNext;
		0u,                                             // VkCmdBufferOptimizeFlags         flags;
		(const VkCommandBufferInheritanceInfo*)DE_NULL,
	};

	VK_CHECK(vk.beginCommandBuffer(*m_cmdBuffer, &cmdBufferBeginInfo));

	// Initialize buffer/image
	vk.cmdFillBuffer(*m_cmdBuffer, *m_dstBuffer, 0u, m_bufSize, 0x0);

	const VkClearColorValue srcClearValue =
	{
		{1.0f, 1.0f, 1.0f, 1.0f}
	};
	const VkClearColorValue dstClearValue =
	{
		{0.0f, 0.0f, 0.0f, 0.0f}
	};
	const struct VkImageSubresourceRange subRangeColor =
	{
		VK_IMAGE_ASPECT_COLOR_BIT,  // VkImageAspectFlags  aspectMask;
		0u,                         // deUint32            baseMipLevel;
		1u,                         // deUint32            mipLevels;
		0u,                         // deUint32            baseArrayLayer;
		1u,                         // deUint32            arraySize;
	};
	const struct VkImageSubresourceRange subRangeDepth =
	{
		VK_IMAGE_ASPECT_DEPTH_BIT,  // VkImageAspectFlags  aspectMask;
		0u,                  // deUint32            baseMipLevel;
		1u,                  // deUint32            mipLevels;
		0u,                  // deUint32            baseArrayLayer;
		1u,                  // deUint32            arraySize;
	};

	initialImageTransition(*m_cmdBuffer, *m_srcImage, subRangeColor, VK_IMAGE_LAYOUT_GENERAL);
	initialImageTransition(*m_cmdBuffer, *m_dstImage, subRangeColor, VK_IMAGE_LAYOUT_GENERAL);

	vk.cmdClearColorImage(*m_cmdBuffer, *m_srcImage, VK_IMAGE_LAYOUT_GENERAL, &srcClearValue, 1u, &subRangeColor);
	vk.cmdClearColorImage(*m_cmdBuffer, *m_dstImage, VK_IMAGE_LAYOUT_GENERAL, &dstClearValue, 1u, &subRangeColor);

	vk.cmdResetQueryPool(*m_cmdBuffer, *m_queryPool, 0u, TimestampTest::ENTRY_COUNT);

	// Copy Operations
	const VkImageSubresourceLayers imgSubResCopy =
	{
		VK_IMAGE_ASPECT_COLOR_BIT,              // VkImageAspectFlags  aspectMask;
		0u,                                     // deUint32            mipLevel;
		0u,                                     // deUint32            baseArrayLayer;
		1u,                                     // deUint32            layerCount;
	};

	const VkOffset3D nullOffset  = {0u, 0u, 0u};
	const VkExtent3D imageExtent = {(deUint32)m_imageWidth, (deUint32)m_imageHeight, 1u};
	const VkOffset3D imageOffset = {(int)m_imageWidth, (int)m_imageHeight, 1};
	switch(m_method)
	{
		case TRANSFER_METHOD_COPY_BUFFER:
			{
				const VkBufferCopy  copyBufRegion =
				{
					0u,      // VkDeviceSize    srcOffset;
					0u,      // VkDeviceSize    destOffset;
					512u,    // VkDeviceSize    copySize;
				};
				vk.cmdCopyBuffer(*m_cmdBuffer, *m_srcBuffer, *m_dstBuffer, 1u, &copyBufRegion);
				break;
			}
		case TRANSFER_METHOD_COPY_IMAGE:
			{
				const VkImageCopy copyImageRegion =
				{
					imgSubResCopy,                          // VkImageSubresourceCopy  srcSubresource;
					nullOffset,                             // VkOffset3D              srcOffset;
					imgSubResCopy,                          // VkImageSubresourceCopy  destSubresource;
					nullOffset,                             // VkOffset3D              destOffset;
					imageExtent,                            // VkExtent3D              extent;

				};
				vk.cmdCopyImage(*m_cmdBuffer, *m_srcImage, VK_IMAGE_LAYOUT_GENERAL, *m_dstImage, VK_IMAGE_LAYOUT_GENERAL, 1u, &copyImageRegion);
				break;
			}
		case TRANSFER_METHOD_COPY_BUFFER_TO_IMAGE:
			{
				const VkBufferImageCopy bufImageCopy =
				{
					0u,                                     // VkDeviceSize            bufferOffset;
					(deUint32)m_imageWidth,                 // deUint32                bufferRowLength;
					(deUint32)m_imageHeight,                // deUint32                bufferImageHeight;
					imgSubResCopy,                          // VkImageSubresourceCopy  imageSubresource;
					nullOffset,                             // VkOffset3D              imageOffset;
					imageExtent,                            // VkExtent3D              imageExtent;
				};
				vk.cmdCopyBufferToImage(*m_cmdBuffer, *m_srcBuffer, *m_dstImage, VK_IMAGE_LAYOUT_GENERAL, 1u, &bufImageCopy);
				break;
			}
		case TRANSFER_METHOD_COPY_IMAGE_TO_BUFFER:
			{
				const VkBufferImageCopy imgBufferCopy =
				{
					0u,                                     // VkDeviceSize            bufferOffset;
					(deUint32)m_imageWidth,                 // deUint32                bufferRowLength;
					(deUint32)m_imageHeight,                // deUint32                bufferImageHeight;
					imgSubResCopy,                          // VkImageSubresourceCopy  imageSubresource;
					nullOffset,                             // VkOffset3D              imageOffset;
					imageExtent,                            // VkExtent3D              imageExtent;
				};
				vk.cmdCopyImageToBuffer(*m_cmdBuffer, *m_srcImage, VK_IMAGE_LAYOUT_GENERAL, *m_dstBuffer, 1u, &imgBufferCopy);
				break;
			}
		case TRANSFER_METHOD_BLIT_IMAGE:
			{
				const VkImageBlit imageBlt =
				{
					imgSubResCopy,                          // VkImageSubresourceCopy  srcSubresource;
					{
						nullOffset,
						imageOffset,
					},
					imgSubResCopy,                          // VkImageSubresourceCopy  destSubresource;
					{
						nullOffset,
						imageOffset,
					}
				};
				vk.cmdBlitImage(*m_cmdBuffer, *m_srcImage, VK_IMAGE_LAYOUT_GENERAL, *m_dstImage, VK_IMAGE_LAYOUT_GENERAL, 1u, &imageBlt, VK_FILTER_NEAREST);
				break;
			}
		case TRANSFER_METHOD_CLEAR_COLOR_IMAGE:
			{
				vk.cmdClearColorImage(*m_cmdBuffer, *m_dstImage, VK_IMAGE_LAYOUT_GENERAL, &srcClearValue, 1u, &subRangeColor);
				break;
			}
		case TRANSFER_METHOD_CLEAR_DEPTH_STENCIL_IMAGE:
			{
				initialImageTransition(*m_cmdBuffer, *m_depthImage, subRangeDepth, VK_IMAGE_LAYOUT_GENERAL);
				const VkClearDepthStencilValue clearDSValue =
				{
					1.0f,                                   // float       depth;
					0u,                                     // deUint32    stencil;
				};
				vk.cmdClearDepthStencilImage(*m_cmdBuffer, *m_depthImage, VK_IMAGE_LAYOUT_GENERAL, &clearDSValue, 1u, &subRangeDepth);
				break;
			}
		case TRANSFER_METHOD_FILL_BUFFER:
			{
				vk.cmdFillBuffer(*m_cmdBuffer, *m_dstBuffer, 0u, m_bufSize, 0x0);
				break;
			}
		case TRANSFER_METHOD_UPDATE_BUFFER:
			{
				const deUint32 data[] =
				{
					0xdeadbeef, 0xabcdef00, 0x12345678
				};
				vk.cmdUpdateBuffer(*m_cmdBuffer, *m_dstBuffer, 0x10, sizeof(data), data);
				break;
			}
		case TRANSFER_METHOD_COPY_QUERY_POOL_RESULTS:
			{
				vk.cmdWriteTimestamp(*m_cmdBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, *m_queryPool, 0u);
				vk.cmdCopyQueryPoolResults(*m_cmdBuffer, *m_queryPool, 0u, 1u, *m_dstBuffer, 0u, 8u, VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT);
				vk.cmdResetQueryPool(*m_cmdBuffer, *m_queryPool, 0u, 1u);
				break;
			}
		case TRANSFER_METHOD_RESOLVE_IMAGE:
			{
				const VkImageResolve imageResolve =
				{
					imgSubResCopy,                              // VkImageSubresourceLayers  srcSubresource;
					nullOffset,                                 // VkOffset3D                srcOffset;
					imgSubResCopy,                              // VkImageSubresourceLayers  destSubresource;
					nullOffset,                                 // VkOffset3D                destOffset;
					imageExtent,                                // VkExtent3D                extent;
				};
				initialImageTransition(*m_cmdBuffer, *m_msImage, subRangeColor, VK_IMAGE_LAYOUT_GENERAL);
				vk.cmdClearColorImage(*m_cmdBuffer, *m_msImage, VK_IMAGE_LAYOUT_GENERAL, &srcClearValue, 1u, &subRangeColor);
				vk.cmdResolveImage(*m_cmdBuffer, *m_msImage, VK_IMAGE_LAYOUT_GENERAL, *m_dstImage, VK_IMAGE_LAYOUT_GENERAL, 1u, &imageResolve);
				break;
			}
		default:
			DE_FATAL("Unknown Transfer Method!");
			break;
	};

	deUint32 timestampEntry = 0u;
	for (StageFlagVector::const_iterator it = m_stages.begin(); it != m_stages.end(); it++)
	{
		vk.cmdWriteTimestamp(*m_cmdBuffer, *it, *m_queryPool, timestampEntry++);
	}

	VK_CHECK(vk.endCommandBuffer(*m_cmdBuffer));
}

void TransferTestInstance::initialImageTransition (VkCommandBuffer cmdBuffer, VkImage image, VkImageSubresourceRange subRange, VkImageLayout layout)
{
	const DeviceInterface&		vk				= m_context.getDeviceInterface();
	const VkImageMemoryBarrier	imageMemBarrier	=
	{
		VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER, // VkStructureType          sType;
		DE_NULL,                                // const void*              pNext;
		0u,                                     // VkAccessFlags            srcAccessMask;
		0u,                                     // VkAccessFlags            dstAccessMask;
		VK_IMAGE_LAYOUT_UNDEFINED,              // VkImageLayout            oldLayout;
		layout,                                 // VkImageLayout            newLayout;
		VK_QUEUE_FAMILY_IGNORED,                // uint32_t                 srcQueueFamilyIndex;
		VK_QUEUE_FAMILY_IGNORED,                // uint32_t                 dstQueueFamilyIndex;
		image,                                  // VkImage                  image;
		subRange                                // VkImageSubresourceRange  subresourceRange;
	};

	vk.cmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0, 0, DE_NULL, 0, DE_NULL, 1, &imageMemBarrier);
}

} // anonymous

tcu::TestCaseGroup* createTimestampTests (tcu::TestContext& testCtx)
{
	de::MovePtr<tcu::TestCaseGroup> timestampTests (new tcu::TestCaseGroup(testCtx, "timestamp", "timestamp tests"));

	// Basic Graphics Tests
	{
		de::MovePtr<tcu::TestCaseGroup> basicGraphicsTests (new tcu::TestCaseGroup(testCtx, "basic_graphics_tests", "Record timestamp in different pipeline stages of basic graphics tests"));

		const VkPipelineStageFlagBits basicGraphicsStages0[][2] =
		{
		  {VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,   VK_PIPELINE_STAGE_VERTEX_INPUT_BIT},
		  {VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,   VK_PIPELINE_STAGE_VERTEX_SHADER_BIT},
		  {VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,   VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT},
		  {VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,   VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT},
		  {VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,   VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT},
		  {VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,   VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT},
		  {VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,   VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT},
		  {VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,   VK_PIPELINE_STAGE_ALL_COMMANDS_BIT},
		};
		for (deUint32 stageNdx = 0u; stageNdx < DE_LENGTH_OF_ARRAY(basicGraphicsStages0); stageNdx++)
		{
			TimestampTestParam param(basicGraphicsStages0[stageNdx], 2u, true);
			basicGraphicsTests->addChild(newTestCase<BasicGraphicsTest>(testCtx, &param));
			param.toggleInRenderPass();
			basicGraphicsTests->addChild(newTestCase<BasicGraphicsTest>(testCtx, &param));
		}

		const VkPipelineStageFlagBits basicGraphicsStages1[][3] =
		{
		  {VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,      VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT},
		  {VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,  VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT},
		};
		for (deUint32 stageNdx = 0u; stageNdx < DE_LENGTH_OF_ARRAY(basicGraphicsStages1); stageNdx++)
		{
			TimestampTestParam param(basicGraphicsStages1[stageNdx], 3u, true);
			basicGraphicsTests->addChild(newTestCase<BasicGraphicsTest>(testCtx, &param));
			param.toggleInRenderPass();
			basicGraphicsTests->addChild(newTestCase<BasicGraphicsTest>(testCtx, &param));
		}

		timestampTests->addChild(basicGraphicsTests.release());
	}

	// Advanced Graphics Tests
	{
		de::MovePtr<tcu::TestCaseGroup> advGraphicsTests (new tcu::TestCaseGroup(testCtx, "advanced_graphics_tests", "Record timestamp in different pipeline stages of advanced graphics tests"));

		const VkPipelineStageFlagBits advGraphicsStages[][2] =
		{
			{VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT},
			{VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT},
			{VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT},
			{VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT},
		};
		for (deUint32 stageNdx = 0u; stageNdx < DE_LENGTH_OF_ARRAY(advGraphicsStages); stageNdx++)
		{
			TimestampTestParam param(advGraphicsStages[stageNdx], 2u, true);
			advGraphicsTests->addChild(newTestCase<AdvGraphicsTest>(testCtx, &param));
			param.toggleInRenderPass();
			advGraphicsTests->addChild(newTestCase<AdvGraphicsTest>(testCtx, &param));
		}

		timestampTests->addChild(advGraphicsTests.release());
	}

	// Basic Compute Tests
	{
		de::MovePtr<tcu::TestCaseGroup> basicComputeTests (new tcu::TestCaseGroup(testCtx, "basic_compute_tests", "Record timestamp for computer stages"));

		const VkPipelineStageFlagBits basicComputeStages[][2] =
		{
			{VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT},
			{VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT},
		};
		for (deUint32 stageNdx = 0u; stageNdx < DE_LENGTH_OF_ARRAY(basicComputeStages); stageNdx++)
		{
			TimestampTestParam param(basicComputeStages[stageNdx], 2u, false);
			basicComputeTests->addChild(newTestCase<BasicComputeTest>(testCtx, &param));
		}

		timestampTests->addChild(basicComputeTests.release());
	}

	// Transfer Tests
	{
		de::MovePtr<tcu::TestCaseGroup> transferTests (new tcu::TestCaseGroup(testCtx, "transfer_tests", "Record timestamp for transfer stages"));

		const VkPipelineStageFlagBits transferStages[][2] =
		{
			{VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT},
			{VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_HOST_BIT},
		};
		for (deUint32 stageNdx = 0u; stageNdx < DE_LENGTH_OF_ARRAY(transferStages); stageNdx++)
		{
			for (deUint32 method = 0u; method < TRANSFER_METHOD_LAST; method++)
			{
				TransferTimestampTestParam param(transferStages[stageNdx], 2u, false, method);
				transferTests->addChild(newTestCase<TransferTest>(testCtx, &param));
			}
		}

		timestampTests->addChild(transferTests.release());
	}

	// Misc Tests
	{
		de::MovePtr<tcu::TestCaseGroup> miscTests (new tcu::TestCaseGroup(testCtx, "misc_tests", "Misc tests that can not be categorized to other group."));

		const VkPipelineStageFlagBits miscStages[] = {VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT};
		TimestampTestParam param(miscStages, 1u, false);
		miscTests->addChild(new TimestampTest(testCtx,
											  "timestamp_only",
											  "Only write timestamp command in the commmand buffer",
											  &param));

		timestampTests->addChild(miscTests.release());
	}

	return timestampTests.release();
}

} // pipeline

} // vkt
