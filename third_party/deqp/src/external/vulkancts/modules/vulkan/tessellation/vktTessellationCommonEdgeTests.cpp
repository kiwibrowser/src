/*------------------------------------------------------------------------
 * Vulkan Conformance Tests
 * ------------------------
 *
 * Copyright (c) 2014 The Android Open Source Project
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
 * \brief Tessellation Common Edge Tests
 *//*--------------------------------------------------------------------*/

#include "vktTessellationCommonEdgeTests.hpp"
#include "vktTestCaseUtil.hpp"
#include "vktTessellationUtil.hpp"

#include "tcuTestLog.hpp"
#include "tcuTexture.hpp"

#include "vkDefs.hpp"
#include "vkQueryUtil.hpp"
#include "vkBuilderUtil.hpp"
#include "vkImageUtil.hpp"
#include "vkTypeUtil.hpp"
#include "vkStrUtil.hpp"

#include "deUniquePtr.hpp"
#include "deStringUtil.hpp"

#include <string>
#include <vector>

namespace vkt
{
namespace tessellation
{

using namespace vk;

namespace
{

enum CaseType
{
	CASETYPE_BASIC = 0,		//!< Order patch vertices such that when two patches share a vertex, it's at the same index for both.
	CASETYPE_PRECISE,		//!< Vertex indices don't match like for CASETYPE_BASIC, but other measures are taken, using the 'precise' qualifier.

	CASETYPE_LAST
};

struct CaseDefinition
{
	TessPrimitiveType	primitiveType;
	SpacingMode			spacingMode;
	CaseType			caseType;
};

//! Check that a certain rectangle in the image contains no black pixels.
//! Returns true if an image successfully passess the verification.
bool verifyResult (tcu::TestLog& log, const tcu::ConstPixelBufferAccess image)
{
	const int startX = static_cast<int>(0.15f * (float)image.getWidth());
	const int endX	 = static_cast<int>(0.85f * (float)image.getWidth());
	const int startY = static_cast<int>(0.15f * (float)image.getHeight());
	const int endY	 = static_cast<int>(0.85f * (float)image.getHeight());

	for (int y = startY; y < endY; ++y)
	for (int x = startX; x < endX; ++x)
	{
		const tcu::Vec4 pixel = image.getPixel(x, y);

		if (pixel.x() == 0 && pixel.y() == 0 && pixel.z() == 0)
		{
			log << tcu::TestLog::Message << "Failure: there seem to be cracks in the rendered result" << tcu::TestLog::EndMessage
				<< tcu::TestLog::Message << "Note: pixel with zero r, g and b channels found at " << tcu::IVec2(x, y) << tcu::TestLog::EndMessage;

			return false;
		}
	}

	log << tcu::TestLog::Message << "Success: there seem to be no cracks in the rendered result" << tcu::TestLog::EndMessage;

	return true;
}

void initPrograms (vk::SourceCollections& programCollection, const CaseDefinition caseDef)
{
	DE_ASSERT(caseDef.primitiveType == TESSPRIMITIVETYPE_TRIANGLES || caseDef.primitiveType == TESSPRIMITIVETYPE_QUADS);

	// Vertex shader
	{
		std::ostringstream src;
		src << glu::getGLSLVersionDeclaration(glu::GLSL_VERSION_310_ES) << "\n"
			<< "\n"
			<< "layout(location = 0) in highp vec2  in_v_position;\n"
			<< "layout(location = 1) in highp float in_v_tessParam;\n"
			<< "\n"
			<< "layout(location = 0) out highp vec2  in_tc_position;\n"
			<< "layout(location = 1) out highp float in_tc_tessParam;\n"
			<< "\n"
			<< "void main (void)\n"
			<< "{\n"
			<< "    in_tc_position = in_v_position;\n"
			<< "    in_tc_tessParam = in_v_tessParam;\n"
			<< "}\n";

		programCollection.glslSources.add("vert") << glu::VertexSource(src.str());
	}

	// Tessellation control shader
	{
		const int numVertices = (caseDef.primitiveType == TESSPRIMITIVETYPE_TRIANGLES ? 3 : 4);

		std::ostringstream src;
		src << glu::getGLSLVersionDeclaration(glu::GLSL_VERSION_310_ES) << "\n"
			<< "#extension GL_EXT_tessellation_shader : require\n"
			<< (caseDef.caseType == CASETYPE_PRECISE ? "#extension GL_EXT_gpu_shader5 : require\n" : "")
			<< "\n"
			<< "layout(vertices = " << numVertices << ") out;\n"
			<< "\n"
			<< "layout(location = 0) in highp vec2  in_tc_position[];\n"
			<< "layout(location = 1) in highp float in_tc_tessParam[];\n"
			<< "\n"
			<< "layout(location = 0) out highp vec2 in_te_position[];\n"
			<< "\n"
			<< (caseDef.caseType == CASETYPE_PRECISE ? "precise gl_TessLevelOuter;\n\n" : "")
			<< "void main (void)\n"
			<< "{\n"
			<< "    in_te_position[gl_InvocationID] = in_tc_position[gl_InvocationID];\n"
			<< "\n"
			<< "    gl_TessLevelInner[0] = 5.0;\n"
			<< "    gl_TessLevelInner[1] = 5.0;\n"
			<< "\n"
			<< (caseDef.primitiveType == TESSPRIMITIVETYPE_TRIANGLES ?
				"    gl_TessLevelOuter[0] = 1.0 + 59.0 * 0.5 * (in_tc_tessParam[1] + in_tc_tessParam[2]);\n"
				"    gl_TessLevelOuter[1] = 1.0 + 59.0 * 0.5 * (in_tc_tessParam[2] + in_tc_tessParam[0]);\n"
				"    gl_TessLevelOuter[2] = 1.0 + 59.0 * 0.5 * (in_tc_tessParam[0] + in_tc_tessParam[1]);\n"
				: caseDef.primitiveType == TESSPRIMITIVETYPE_QUADS ?
				"    gl_TessLevelOuter[0] = 1.0 + 59.0 * 0.5 * (in_tc_tessParam[0] + in_tc_tessParam[2]);\n"
				"    gl_TessLevelOuter[1] = 1.0 + 59.0 * 0.5 * (in_tc_tessParam[1] + in_tc_tessParam[0]);\n"
				"    gl_TessLevelOuter[2] = 1.0 + 59.0 * 0.5 * (in_tc_tessParam[3] + in_tc_tessParam[1]);\n"
				"    gl_TessLevelOuter[3] = 1.0 + 59.0 * 0.5 * (in_tc_tessParam[2] + in_tc_tessParam[3]);\n"
				: "")
			<< "}\n";

		programCollection.glslSources.add("tesc") << glu::TessellationControlSource(src.str());
	}

	// Tessellation evaluation shader
	{
		std::ostringstream primitiveSpecificCode;
		if (caseDef.primitiveType == TESSPRIMITIVETYPE_TRIANGLES)
			primitiveSpecificCode
			<< "    highp vec2 pos = gl_TessCoord.x*in_te_position[0] + gl_TessCoord.y*in_te_position[1] + gl_TessCoord.z*in_te_position[2];\n"
			<< "\n"
			<< "    highp float f = sqrt(3.0 * min(gl_TessCoord.x, min(gl_TessCoord.y, gl_TessCoord.z))) * 0.5 + 0.5;\n"
			<< "    in_f_color = vec4(gl_TessCoord*f, 1.0);\n";
		else if (caseDef.primitiveType == TESSPRIMITIVETYPE_QUADS)
			primitiveSpecificCode
			<< (caseDef.caseType == CASETYPE_BASIC ?
				"    highp vec2 pos = (1.0-gl_TessCoord.x)*(1.0-gl_TessCoord.y)*in_te_position[0]\n"
				"                   + (    gl_TessCoord.x)*(1.0-gl_TessCoord.y)*in_te_position[1]\n"
				"                   + (1.0-gl_TessCoord.x)*(    gl_TessCoord.y)*in_te_position[2]\n"
				"                   + (    gl_TessCoord.x)*(    gl_TessCoord.y)*in_te_position[3];\n"
				: caseDef.caseType == CASETYPE_PRECISE ?
				"    highp vec2 a = (1.0-gl_TessCoord.x)*(1.0-gl_TessCoord.y)*in_te_position[0];\n"
				"    highp vec2 b = (    gl_TessCoord.x)*(1.0-gl_TessCoord.y)*in_te_position[1];\n"
				"    highp vec2 c = (1.0-gl_TessCoord.x)*(    gl_TessCoord.y)*in_te_position[2];\n"
				"    highp vec2 d = (    gl_TessCoord.x)*(    gl_TessCoord.y)*in_te_position[3];\n"
				"    highp vec2 pos = a+b+c+d;\n"
				: "")
			<< "\n"
			<< "    highp float f = sqrt(1.0 - 2.0 * max(abs(gl_TessCoord.x - 0.5), abs(gl_TessCoord.y - 0.5)))*0.5 + 0.5;\n"
			<< "    in_f_color = vec4(0.1, gl_TessCoord.xy*f, 1.0);\n";

		std::ostringstream src;
		src << glu::getGLSLVersionDeclaration(glu::GLSL_VERSION_310_ES) << "\n"
			<< "#extension GL_EXT_tessellation_shader : require\n"
			<< (caseDef.caseType == CASETYPE_PRECISE ? "#extension GL_EXT_gpu_shader5 : require\n" : "")
			<< "\n"
			<< "layout(" << getTessPrimitiveTypeShaderName(caseDef.primitiveType) << ", "
						 << getSpacingModeShaderName(caseDef.spacingMode) << ") in;\n"
			<< "\n"
			<< "layout(location = 0) in highp vec2 in_te_position[];\n"
			<< "\n"
			<< "layout(location = 0) out mediump vec4 in_f_color;\n"
			<< "\n"
			<< (caseDef.caseType == CASETYPE_PRECISE ? "precise gl_Position;\n\n" : "")
			<< "void main (void)\n"
			<< "{\n"
			<< primitiveSpecificCode.str()
			<< "\n"
			<< "    // Offset the position slightly, based on the parity of the bits in the float representation.\n"
			<< "    // This is done to detect possible small differences in edge vertex positions between patches.\n"
			<< "    uvec2 bits = floatBitsToUint(pos);\n"
			<< "    uint numBits = 0u;\n"
			<< "    for (uint i = 0u; i < 32u; i++)\n"
			<< "        numBits += ((bits[0] >> i) & 1u) + ((bits[1] >> i) & 1u);\n"
			<< "    pos += float(numBits&1u)*0.04;\n"
			<< "\n"
			<< "    gl_Position = vec4(pos, 0.0, 1.0);\n"
			<< "}\n";

		programCollection.glslSources.add("tese") << glu::TessellationEvaluationSource(src.str());
	}

	// Fragment shader
	{
		std::ostringstream src;
		src << glu::getGLSLVersionDeclaration(glu::GLSL_VERSION_310_ES) << "\n"
			<< "\n"
			<< "layout(location = 0) in mediump vec4 in_f_color;\n"
			<< "\n"
			<< "layout(location = 0) out mediump vec4 o_color;\n"
			<< "\n"
			<< "void main (void)\n"
			<< "{\n"
			<< "    o_color = in_f_color;\n"
			<< "}\n";

		programCollection.glslSources.add("frag") << glu::FragmentSource(src.str());
	}
}

//! Generic test code used by all test cases.
tcu::TestStatus test (Context& context, const CaseDefinition caseDef)
{
	DE_ASSERT(caseDef.primitiveType == TESSPRIMITIVETYPE_TRIANGLES || caseDef.primitiveType == TESSPRIMITIVETYPE_QUADS);
	DE_ASSERT(caseDef.caseType == CASETYPE_BASIC || caseDef.caseType == CASETYPE_PRECISE);

	requireFeatures(context.getInstanceInterface(), context.getPhysicalDevice(), FEATURE_TESSELLATION_SHADER);

	const DeviceInterface&	vk					= context.getDeviceInterface();
	const VkDevice			device				= context.getDevice();
	const VkQueue			queue				= context.getUniversalQueue();
	const deUint32			queueFamilyIndex	= context.getUniversalQueueFamilyIndex();
	Allocator&				allocator			= context.getDefaultAllocator();

	// Prepare test data

	std::vector<float>		gridPosComps;
	std::vector<float>		gridTessParams;
	std::vector<deUint16>	gridIndices;

	{
		const int gridWidth				= 4;
		const int gridHeight			= 4;
		const int numVertices			= (gridWidth+1)*(gridHeight+1);
		const int numIndices			= gridWidth*gridHeight * (caseDef.primitiveType == TESSPRIMITIVETYPE_TRIANGLES ? 3*2 : 4);
		const int numPosCompsPerVertex	= 2;
		const int totalNumPosComps		= numPosCompsPerVertex*numVertices;

		gridPosComps.reserve(totalNumPosComps);
		gridTessParams.reserve(numVertices);
		gridIndices.reserve(numIndices);

		{
			for (int i = 0; i < gridHeight+1; ++i)
			for (int j = 0; j < gridWidth+1; ++j)
			{
				gridPosComps.push_back(-1.0f + 2.0f * ((float)j + 0.5f) / (float)(gridWidth+1));
				gridPosComps.push_back(-1.0f + 2.0f * ((float)i + 0.5f) / (float)(gridHeight+1));
				gridTessParams.push_back((float)(i*(gridWidth+1) + j) / (float)(numVertices-1));
			}
		}

		// Generate patch vertex indices.
		// \note If CASETYPE_BASIC, the vertices are ordered such that when multiple
		//		 triangles/quads share a vertex, it's at the same index for everyone.

		if (caseDef.primitiveType == TESSPRIMITIVETYPE_TRIANGLES)
		{
			for (int i = 0; i < gridHeight; i++)
			for (int j = 0; j < gridWidth; j++)
			{
				const deUint16 corners[4] =
				{
					(deUint16)((i+0)*(gridWidth+1) + j+0),
					(deUint16)((i+0)*(gridWidth+1) + j+1),
					(deUint16)((i+1)*(gridWidth+1) + j+0),
					(deUint16)((i+1)*(gridWidth+1) + j+1)
				};

				const int secondTriangleVertexIndexOffset = caseDef.caseType == CASETYPE_BASIC   ? 0 : 1;

				for (int k = 0; k < 3; k++)
					gridIndices.push_back(corners[(k+0 + i + (2-j%3)) % 3]);
				for (int k = 0; k < 3; k++)
					gridIndices.push_back(corners[(k+2 + i + (2-j%3) + secondTriangleVertexIndexOffset) % 3 + 1]);
			}
		}
		else if (caseDef.primitiveType == TESSPRIMITIVETYPE_QUADS)
		{
			for (int i = 0; i < gridHeight; ++i)
			for (int j = 0; j < gridWidth; ++j)
			{
				for (int m = 0; m < 2; m++)
				for (int n = 0; n < 2; n++)
					gridIndices.push_back((deUint16)((i+(i+m)%2)*(gridWidth+1) + j+(j+n)%2));

				if (caseDef.caseType == CASETYPE_PRECISE && (i+j) % 2 == 0)
					std::reverse(gridIndices.begin() + (gridIndices.size() - 4),
								 gridIndices.begin() + gridIndices.size());
			}
		}
		else
			DE_ASSERT(false);

		DE_ASSERT(static_cast<int>(gridPosComps.size()) == totalNumPosComps);
		DE_ASSERT(static_cast<int>(gridTessParams.size()) == numVertices);
		DE_ASSERT(static_cast<int>(gridIndices.size()) == numIndices);
	}

	// Vertex input buffer: we put both attributes and indices in here.

	const VkDeviceSize vertexDataSizeBytes    = sizeInBytes(gridPosComps) + sizeInBytes(gridTessParams) + sizeInBytes(gridIndices);
	const std::size_t  vertexPositionsOffset  = 0;
	const std::size_t  vertexTessParamsOffset = sizeInBytes(gridPosComps);
	const std::size_t  vertexIndicesOffset    = vertexTessParamsOffset + sizeInBytes(gridTessParams);

	const Buffer vertexBuffer(vk, device, allocator,
		makeBufferCreateInfo(vertexDataSizeBytes, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT), MemoryRequirement::HostVisible);

	{
		const Allocation& alloc = vertexBuffer.getAllocation();
		deUint8* const pData = static_cast<deUint8*>(alloc.getHostPtr());

		deMemcpy(pData + vertexPositionsOffset,  &gridPosComps[0],   static_cast<std::size_t>(sizeInBytes(gridPosComps)));
		deMemcpy(pData + vertexTessParamsOffset, &gridTessParams[0], static_cast<std::size_t>(sizeInBytes(gridTessParams)));
		deMemcpy(pData + vertexIndicesOffset,    &gridIndices[0],    static_cast<std::size_t>(sizeInBytes(gridIndices)));

		flushMappedMemoryRange(vk, device, alloc.getMemory(), alloc.getOffset(), vertexDataSizeBytes);
		// No barrier needed, flushed memory is automatically visible
	}

	// Color attachment

	const tcu::IVec2			  renderSize				 = tcu::IVec2(256, 256);
	const VkFormat				  colorFormat				 = VK_FORMAT_R8G8B8A8_UNORM;
	const VkImageSubresourceRange colorImageSubresourceRange = makeImageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT, 0u, 1u, 0u, 1u);
	const Image					  colorAttachmentImage		 (vk, device, allocator,
															 makeImageCreateInfo(renderSize, colorFormat, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, 1u),
															 MemoryRequirement::Any);

	// Color output buffer: image will be copied here for verification

	const VkDeviceSize	colorBufferSizeBytes = renderSize.x()*renderSize.y() * tcu::getPixelSize(mapVkFormat(colorFormat));
	const Buffer		colorBuffer			 (vk, device, allocator, makeBufferCreateInfo(colorBufferSizeBytes, VK_BUFFER_USAGE_TRANSFER_DST_BIT), MemoryRequirement::HostVisible);

	// Pipeline

	const Unique<VkImageView>	   colorAttachmentView	(makeImageView						 (vk, device, *colorAttachmentImage, VK_IMAGE_VIEW_TYPE_2D, colorFormat, colorImageSubresourceRange));
	const Unique<VkRenderPass>	   renderPass			(makeRenderPass						 (vk, device, colorFormat));
	const Unique<VkFramebuffer>	   framebuffer			(makeFramebuffer					 (vk, device, *renderPass, *colorAttachmentView, renderSize.x(), renderSize.y(), 1u));
	const Unique<VkCommandPool>	   cmdPool				(makeCommandPool					 (vk, device, queueFamilyIndex));
	const Unique<VkCommandBuffer>  cmdBuffer			(allocateCommandBuffer				 (vk, device, *cmdPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY));
	const Unique<VkPipelineLayout> pipelineLayout		(makePipelineLayoutWithoutDescriptors(vk, device));

	const int inPatchSize = (caseDef.primitiveType == TESSPRIMITIVETYPE_TRIANGLES ? 3 : 4);
	const Unique<VkPipeline> pipeline(GraphicsPipelineBuilder()
		.setRenderSize		  (renderSize)
		.setPatchControlPoints(inPatchSize)
		.addVertexBinding	  (makeVertexInputBindingDescription(0u, sizeof(tcu::Vec2), VK_VERTEX_INPUT_RATE_VERTEX))
		.addVertexBinding	  (makeVertexInputBindingDescription(1u, sizeof(float),     VK_VERTEX_INPUT_RATE_VERTEX))
		.addVertexAttribute	  (makeVertexInputAttributeDescription(0u, 0u, VK_FORMAT_R32G32_SFLOAT, 0u))
		.addVertexAttribute	  (makeVertexInputAttributeDescription(1u, 1u, VK_FORMAT_R32_SFLOAT,    0u))
		.setShader			  (vk, device, VK_SHADER_STAGE_VERTEX_BIT,					context.getBinaryCollection().get("vert"), DE_NULL)
		.setShader			  (vk, device, VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT,	context.getBinaryCollection().get("tesc"), DE_NULL)
		.setShader			  (vk, device, VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT, context.getBinaryCollection().get("tese"), DE_NULL)
		.setShader			  (vk, device, VK_SHADER_STAGE_FRAGMENT_BIT,				context.getBinaryCollection().get("frag"), DE_NULL)
		.build				  (vk, device, *pipelineLayout, *renderPass));

	// Draw commands

	beginCommandBuffer(vk, *cmdBuffer);

	// Change color attachment image layout
	{
		const VkImageMemoryBarrier colorAttachmentLayoutBarrier = makeImageMemoryBarrier(
			(VkAccessFlags)0, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			*colorAttachmentImage, colorImageSubresourceRange);

		vk.cmdPipelineBarrier(*cmdBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0u,
			0u, DE_NULL, 0u, DE_NULL, 1u, &colorAttachmentLayoutBarrier);
	}

	// Begin render pass
	{
		const VkRect2D renderArea = {
			makeOffset2D(0, 0),
			makeExtent2D(renderSize.x(), renderSize.y()),
		};
		const tcu::Vec4 clearColor(0.0f, 0.0f, 0.0f, 1.0f);

		beginRenderPass(vk, *cmdBuffer, *renderPass, *framebuffer, renderArea, clearColor);
	}

	vk.cmdBindPipeline(*cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *pipeline);
	{
		const VkBuffer buffers[] = { *vertexBuffer, *vertexBuffer };
		const VkDeviceSize offsets[] = { vertexPositionsOffset, vertexTessParamsOffset, };
		vk.cmdBindVertexBuffers(*cmdBuffer, 0u, DE_LENGTH_OF_ARRAY(buffers), buffers, offsets);

		vk.cmdBindIndexBuffer(*cmdBuffer, *vertexBuffer, vertexIndicesOffset, VK_INDEX_TYPE_UINT16);
	}

	vk.cmdDrawIndexed(*cmdBuffer, static_cast<deUint32>(gridIndices.size()), 1u, 0u, 0, 0u);
	endRenderPass(vk, *cmdBuffer);

	// Copy render result to a host-visible buffer
	{
		const VkImageMemoryBarrier colorAttachmentPreCopyBarrier = makeImageMemoryBarrier(
			VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			*colorAttachmentImage, colorImageSubresourceRange);

		vk.cmdPipelineBarrier(*cmdBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0u,
			0u, DE_NULL, 0u, DE_NULL, 1u, &colorAttachmentPreCopyBarrier);
	}
	{
		const VkBufferImageCopy copyRegion = makeBufferImageCopy(makeExtent3D(renderSize.x(), renderSize.y(), 1), makeImageSubresourceLayers(VK_IMAGE_ASPECT_COLOR_BIT, 0u, 0u, 1u));
		vk.cmdCopyImageToBuffer(*cmdBuffer, *colorAttachmentImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, *colorBuffer, 1u, &copyRegion);
	}
	{
		const VkBufferMemoryBarrier postCopyBarrier = makeBufferMemoryBarrier(
			VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_HOST_READ_BIT, *colorBuffer, 0ull, colorBufferSizeBytes);

		vk.cmdPipelineBarrier(*cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_HOST_BIT, 0u,
			0u, DE_NULL, 1u, &postCopyBarrier, 0u, DE_NULL);
	}

	endCommandBuffer(vk, *cmdBuffer);
	submitCommandsAndWait(vk, device, queue, *cmdBuffer);

	{
		// Log the result image.

		const Allocation& colorBufferAlloc = colorBuffer.getAllocation();
		invalidateMappedMemoryRange(vk, device, colorBufferAlloc.getMemory(), colorBufferAlloc.getOffset(), colorBufferSizeBytes);
		const tcu::ConstPixelBufferAccess imagePixelAccess(mapVkFormat(colorFormat), renderSize.x(), renderSize.y(), 1, colorBufferAlloc.getHostPtr());

		tcu::TestLog& log = context.getTestContext().getLog();
		log << tcu::TestLog::Image("color0", "Rendered image", imagePixelAccess)
			<< tcu::TestLog::Message
			<< "Note: coloring is done to clarify the positioning and orientation of the "
			<< (caseDef.primitiveType == TESSPRIMITIVETYPE_TRIANGLES ? "triangles" :
				caseDef.primitiveType == TESSPRIMITIVETYPE_QUADS	 ? "quads"	   : "")
			<< "; the color of a vertex corresponds to the index of that vertex in the patch"
			<< tcu::TestLog::EndMessage;

		if (caseDef.caseType == CASETYPE_BASIC)
			log << tcu::TestLog::Message << "Note: each shared vertex has the same index among the primitives it belongs to" << tcu::TestLog::EndMessage;
		else if (caseDef.caseType == CASETYPE_PRECISE)
			log << tcu::TestLog::Message << "Note: the 'precise' qualifier is used to avoid cracks between primitives" << tcu::TestLog::EndMessage;
		else
			DE_ASSERT(false);

		// Verify the result.

		const bool ok = verifyResult(log, imagePixelAccess);
		return (ok ? tcu::TestStatus::pass("OK") : tcu::TestStatus::fail("Failure"));
	}
}

std::string getCaseName (const TessPrimitiveType primitiveType, const SpacingMode spacingMode, const CaseType caseType)
{
	std::ostringstream str;
	str << getTessPrimitiveTypeShaderName(primitiveType) << "_" << getSpacingModeShaderName(spacingMode)
		<< (caseType == CASETYPE_PRECISE ? "_precise" : "");
	return str.str();
}

} // anonymous

//! These tests correspond to dEQP-GLES31.functional.tessellation.common_edge.*
tcu::TestCaseGroup* createCommonEdgeTests (tcu::TestContext& testCtx)
{
	de::MovePtr<tcu::TestCaseGroup> group (new tcu::TestCaseGroup(testCtx, "common_edge", "Draw multiple adjacent shapes and check that no cracks appear between them"));

	static const TessPrimitiveType primitiveTypes[] =
	{
		TESSPRIMITIVETYPE_TRIANGLES,
		TESSPRIMITIVETYPE_QUADS,
	};

	for (int primitiveTypeNdx = 0; primitiveTypeNdx < DE_LENGTH_OF_ARRAY(primitiveTypes); ++primitiveTypeNdx)
	for (int caseTypeNdx = 0; caseTypeNdx < CASETYPE_LAST; ++caseTypeNdx)
	for (int spacingModeNdx = 0; spacingModeNdx < SPACINGMODE_LAST; ++spacingModeNdx)
	{
		const TessPrimitiveType primitiveType = primitiveTypes[primitiveTypeNdx];
		const CaseType			caseType	  = static_cast<CaseType>(caseTypeNdx);
		const SpacingMode		spacingMode   = static_cast<SpacingMode>(spacingModeNdx);
		const CaseDefinition	caseDef		  = { primitiveType, spacingMode, caseType };

		addFunctionCaseWithPrograms(group.get(), getCaseName(primitiveType, spacingMode, caseType), "", initPrograms, test, caseDef);
	}

	return group.release();
}

} // tessellation
} // vkt
