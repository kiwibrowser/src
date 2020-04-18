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
 * \brief Tessellation Miscellaneous Draw Tests
 *//*--------------------------------------------------------------------*/

#include "vktTessellationMiscDrawTests.hpp"
#include "vktTestCaseUtil.hpp"
#include "vktTessellationUtil.hpp"

#include "tcuTestLog.hpp"
#include "tcuImageIO.hpp"
#include "tcuTexture.hpp"
#include "tcuImageCompare.hpp"

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

struct CaseDefinition
{
	TessPrimitiveType	primitiveType;
	SpacingMode			spacingMode;
	std::string			referenceImagePathPrefix;	//!< without case suffix and extension (e.g. "_1.png")
};

inline CaseDefinition makeCaseDefinition (const TessPrimitiveType	primitiveType,
										  const SpacingMode			spacingMode,
										  const std::string&		referenceImagePathPrefix)
{
	CaseDefinition caseDef;
	caseDef.primitiveType = primitiveType;
	caseDef.spacingMode = spacingMode;
	caseDef.referenceImagePathPrefix = referenceImagePathPrefix;
	return caseDef;
}

std::vector<TessLevels> genTessLevelCases (const SpacingMode spacingMode)
{
	static const TessLevels tessLevelCases[] =
	{
		{ { 9.0f,	9.0f	},	{ 9.0f,		9.0f,	9.0f,	9.0f	} },
		{ { 8.0f,	11.0f	},	{ 13.0f,	15.0f,	18.0f,	21.0f	} },
		{ { 17.0f,	14.0f	},	{ 3.0f,		6.0f,	9.0f,	12.0f	} },
	};

	std::vector<TessLevels> resultTessLevels(DE_LENGTH_OF_ARRAY(tessLevelCases));

	for (int tessLevelCaseNdx = 0; tessLevelCaseNdx < DE_LENGTH_OF_ARRAY(tessLevelCases); ++tessLevelCaseNdx)
	{
		TessLevels& tessLevels = resultTessLevels[tessLevelCaseNdx];

		for (int i = 0; i < 2; ++i)
			tessLevels.inner[i] = static_cast<float>(getClampedRoundedTessLevel(spacingMode, tessLevelCases[tessLevelCaseNdx].inner[i]));

		for (int i = 0; i < 4; ++i)
			tessLevels.outer[i] = static_cast<float>(getClampedRoundedTessLevel(spacingMode, tessLevelCases[tessLevelCaseNdx].outer[i]));
	}

	return resultTessLevels;
}

std::vector<tcu::Vec2> genVertexPositions (const TessPrimitiveType primitiveType)
{
	std::vector<tcu::Vec2> positions;
	positions.reserve(4);

	if (primitiveType == TESSPRIMITIVETYPE_TRIANGLES)
	{
		positions.push_back(tcu::Vec2( 0.8f,    0.6f));
		positions.push_back(tcu::Vec2( 0.0f, -0.786f));
		positions.push_back(tcu::Vec2(-0.8f,    0.6f));
	}
	else if (primitiveType == TESSPRIMITIVETYPE_QUADS || primitiveType == TESSPRIMITIVETYPE_ISOLINES)
	{
		positions.push_back(tcu::Vec2(-0.8f, -0.8f));
		positions.push_back(tcu::Vec2( 0.8f, -0.8f));
		positions.push_back(tcu::Vec2(-0.8f,  0.8f));
		positions.push_back(tcu::Vec2( 0.8f,  0.8f));
	}
	else
		DE_ASSERT(false);

	return positions;
}

//! Common test function used by all test cases.
tcu::TestStatus runTest (Context& context, const CaseDefinition caseDef)
{
	requireFeatures(context.getInstanceInterface(), context.getPhysicalDevice(), FEATURE_TESSELLATION_SHADER);

	const DeviceInterface&	vk					= context.getDeviceInterface();
	const VkDevice			device				= context.getDevice();
	const VkQueue			queue				= context.getUniversalQueue();
	const deUint32			queueFamilyIndex	= context.getUniversalQueueFamilyIndex();
	Allocator&				allocator			= context.getDefaultAllocator();

	const std::vector<TessLevels> tessLevelCases = genTessLevelCases(caseDef.spacingMode);
	const std::vector<tcu::Vec2>  vertexData	 = genVertexPositions(caseDef.primitiveType);
	const deUint32				  inPatchSize	 = (caseDef.primitiveType == TESSPRIMITIVETYPE_TRIANGLES ? 3 : 4);

	// Vertex input: positions

	const VkFormat	   vertexFormat		   = VK_FORMAT_R32G32_SFLOAT;
	const deUint32	   vertexStride		   = tcu::getPixelSize(mapVkFormat(vertexFormat));
	const VkDeviceSize vertexDataSizeBytes = sizeInBytes(vertexData);

	const Buffer vertexBuffer(vk, device, allocator,
		makeBufferCreateInfo(vertexDataSizeBytes, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT), MemoryRequirement::HostVisible);

	DE_ASSERT(inPatchSize == vertexData.size());
	DE_ASSERT(sizeof(vertexData[0]) == vertexStride);

	{
		const Allocation& alloc = vertexBuffer.getAllocation();
		deMemcpy(alloc.getHostPtr(), &vertexData[0], static_cast<std::size_t>(vertexDataSizeBytes));

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

	const VkDeviceSize colorBufferSizeBytes = renderSize.x()*renderSize.y() * tcu::getPixelSize(mapVkFormat(colorFormat));
	const Buffer	   colorBuffer			(vk, device, allocator, makeBufferCreateInfo(colorBufferSizeBytes, VK_BUFFER_USAGE_TRANSFER_DST_BIT), MemoryRequirement::HostVisible);

	// Input buffer: tessellation levels. Data is filled in later.

	const Buffer tessLevelsBuffer(vk, device, allocator,
		makeBufferCreateInfo(sizeof(TessLevels), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT), MemoryRequirement::HostVisible);

	// Descriptors

	const Unique<VkDescriptorSetLayout> descriptorSetLayout(DescriptorSetLayoutBuilder()
		.addSingleBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT | VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT)
		.build(vk, device));

	const Unique<VkDescriptorPool> descriptorPool(DescriptorPoolBuilder()
		.addType(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
		.build(vk, device, VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT, 1u));

	const Unique<VkDescriptorSet> descriptorSet(makeDescriptorSet(vk, device, *descriptorPool, *descriptorSetLayout));

	const VkDescriptorBufferInfo tessLevelsBufferInfo = makeDescriptorBufferInfo(tessLevelsBuffer.get(), 0ull, sizeof(TessLevels));

	DescriptorSetUpdateBuilder()
		.writeSingle(*descriptorSet, DescriptorSetUpdateBuilder::Location::binding(0u), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, &tessLevelsBufferInfo)
		.update(vk, device);

	// Pipeline

	const Unique<VkImageView>		colorAttachmentView	(makeImageView(vk, device, *colorAttachmentImage, VK_IMAGE_VIEW_TYPE_2D, colorFormat, colorImageSubresourceRange));
	const Unique<VkRenderPass>		renderPass			(makeRenderPass(vk, device, colorFormat));
	const Unique<VkFramebuffer>		framebuffer			(makeFramebuffer(vk, device, *renderPass, *colorAttachmentView, renderSize.x(), renderSize.y(), 1u));
	const Unique<VkPipelineLayout>	pipelineLayout		(makePipelineLayout(vk, device, *descriptorSetLayout));
	const Unique<VkCommandPool>		cmdPool				(makeCommandPool(vk, device, queueFamilyIndex));
	const Unique<VkCommandBuffer>	cmdBuffer			(allocateCommandBuffer(vk, device, *cmdPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY));

	const Unique<VkPipeline> pipeline(GraphicsPipelineBuilder()
		.setRenderSize				  (renderSize)
		.setVertexInputSingleAttribute(vertexFormat, vertexStride)
		.setPatchControlPoints		  (inPatchSize)
		.setShader					  (vk, device, VK_SHADER_STAGE_VERTEX_BIT,					context.getBinaryCollection().get("vert"), DE_NULL)
		.setShader					  (vk, device, VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT,	context.getBinaryCollection().get("tesc"), DE_NULL)
		.setShader					  (vk, device, VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT, context.getBinaryCollection().get("tese"), DE_NULL)
		.setShader					  (vk, device, VK_SHADER_STAGE_FRAGMENT_BIT,				context.getBinaryCollection().get("frag"), DE_NULL)
		.build						  (vk, device, *pipelineLayout, *renderPass));

	// Draw commands

	deUint32 numPassedCases = 0;

	for (deUint32 tessLevelCaseNdx = 0; tessLevelCaseNdx < tessLevelCases.size(); ++tessLevelCaseNdx)
	{
		context.getTestContext().getLog()
			<< tcu::TestLog::Message
			<< "Tessellation levels: " << getTessellationLevelsString(tessLevelCases[tessLevelCaseNdx], caseDef.primitiveType)
			<< tcu::TestLog::EndMessage;

		// Upload tessellation levels data to the input buffer
		{
			const Allocation& alloc = tessLevelsBuffer.getAllocation();
			TessLevels* const bufferTessLevels = static_cast<TessLevels*>(alloc.getHostPtr());
			*bufferTessLevels = tessLevelCases[tessLevelCaseNdx];
			flushMappedMemoryRange(vk, device, alloc.getMemory(), alloc.getOffset(), sizeof(TessLevels));
		}

		// Reset the command buffer and begin recording.
		beginCommandBuffer(vk, *cmdBuffer);

		// Change color attachment image layout
		{
			// State is slightly different on the first iteration.
			const VkImageLayout currentLayout = (tessLevelCaseNdx == 0 ? VK_IMAGE_LAYOUT_UNDEFINED : VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
			const VkAccessFlags srcFlags	  = (tessLevelCaseNdx == 0 ? (VkAccessFlags)0 : (VkAccessFlags)VK_ACCESS_TRANSFER_READ_BIT);

			const VkImageMemoryBarrier colorAttachmentLayoutBarrier = makeImageMemoryBarrier(
				srcFlags, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
				currentLayout, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
				*colorAttachmentImage, colorImageSubresourceRange);

			vk.cmdPipelineBarrier(*cmdBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0u,
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
		vk.cmdBindDescriptorSets(*cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *pipelineLayout, 0u, 1u, &descriptorSet.get(), 0u, DE_NULL);
		{
			const VkDeviceSize vertexBufferOffset = 0ull;
			vk.cmdBindVertexBuffers(*cmdBuffer, 0u, 1u, &vertexBuffer.get(), &vertexBufferOffset);
		}

		// Process enough vertices to make a patch.
		vk.cmdDraw(*cmdBuffer, inPatchSize, 1u, 0u, 0u);
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
			const Allocation& colorBufferAlloc = colorBuffer.getAllocation();
			invalidateMappedMemoryRange(vk, device, colorBufferAlloc.getMemory(), colorBufferAlloc.getOffset(), colorBufferSizeBytes);

			// Verify case result
			const tcu::ConstPixelBufferAccess resultImageAccess(mapVkFormat(colorFormat), renderSize.x(), renderSize.y(), 1, colorBufferAlloc.getHostPtr());

			// Load reference image
			const std::string referenceImagePath = caseDef.referenceImagePathPrefix + "_" + de::toString(tessLevelCaseNdx) + ".png";
			tcu::TextureLevel referenceImage;
			tcu::ImageIO::loadPNG(referenceImage, context.getTestContext().getArchive(), referenceImagePath.c_str());

			if (tcu::fuzzyCompare(context.getTestContext().getLog(), "ImageComparison", "Image Comparison",
								  referenceImage.getAccess(), resultImageAccess, 0.002f, tcu::COMPARE_LOG_RESULT))
				++numPassedCases;
		}
	} // tessLevelCaseNdx

	return (numPassedCases == tessLevelCases.size() ? tcu::TestStatus::pass("OK") : tcu::TestStatus::fail("Failure"));
}

inline const char* getTessLevelsSSBODeclaration (void)
{
	return	"layout(set = 0, binding = 0, std430) readonly restrict buffer TessLevels {\n"
			"    float inner0;\n"
			"    float inner1;\n"
			"    float outer0;\n"
			"    float outer1;\n"
			"    float outer2;\n"
			"    float outer3;\n"
			"} sb_levels;\n";
}

//! Add vertex, fragment, and tessellation control shaders.
void initCommonPrograms (vk::SourceCollections& programCollection, const CaseDefinition caseDef)
{
	DE_ASSERT(!programCollection.glslSources.contains("vert"));
	DE_ASSERT(!programCollection.glslSources.contains("tesc"));
	DE_ASSERT(!programCollection.glslSources.contains("frag"));

	// Vertex shader
	{
		std::ostringstream src;
		src << glu::getGLSLVersionDeclaration(glu::GLSL_VERSION_310_ES) << "\n"
			<< "\n"
			<< "layout(location = 0) in  highp vec2 in_v_position;\n"
			<< "layout(location = 0) out highp vec2 in_tc_position;\n"
			<< "\n"
			<< "void main (void)\n"
			<< "{\n"
			<< "    in_tc_position = in_v_position;\n"
			<< "}\n";

		programCollection.glslSources.add("vert") << glu::VertexSource(src.str());
	}

	// Tessellation control shader
	{
		const int numVertices = (caseDef.primitiveType == TESSPRIMITIVETYPE_TRIANGLES ? 3 : 4);

		std::ostringstream src;
		src << glu::getGLSLVersionDeclaration(glu::GLSL_VERSION_310_ES) << "\n"
			<< "#extension GL_EXT_tessellation_shader : require\n"
			<< "\n"
			<< "layout(vertices = " << numVertices << ") out;\n"
			<< "\n"
			<< getTessLevelsSSBODeclaration()
			<< "\n"
			<< "layout(location = 0) in  highp vec2 in_tc_position[];\n"
			<< "layout(location = 0) out highp vec2 in_te_position[];\n"
			<< "\n"
			<< "void main (void)\n"
			<< "{\n"
			<< "    in_te_position[gl_InvocationID] = in_tc_position[gl_InvocationID];\n"
			<< "\n"
			<< "    gl_TessLevelInner[0] = sb_levels.inner0;\n"
			<< "    gl_TessLevelInner[1] = sb_levels.inner1;\n"
			<< "\n"
			<< "    gl_TessLevelOuter[0] = sb_levels.outer0;\n"
			<< "    gl_TessLevelOuter[1] = sb_levels.outer1;\n"
			<< "    gl_TessLevelOuter[2] = sb_levels.outer2;\n"
			<< "    gl_TessLevelOuter[3] = sb_levels.outer3;\n"
			<< "}\n";

		programCollection.glslSources.add("tesc") << glu::TessellationControlSource(src.str());
	}

	// Fragment shader
	{
		std::ostringstream src;
		src << glu::getGLSLVersionDeclaration(glu::GLSL_VERSION_310_ES) << "\n"
			<< "\n"
			<< "layout(location = 0) in  highp   vec4 in_f_color;\n"
			<< "layout(location = 0) out mediump vec4 o_color;\n"
			<< "\n"
			<< "void main (void)\n"
			<< "{\n"
			<< "    o_color = in_f_color;\n"
			<< "}\n";

		programCollection.glslSources.add("frag") << glu::FragmentSource(src.str());
	}
}

void initProgramsFillCoverCase (vk::SourceCollections& programCollection, const CaseDefinition caseDef)
{
	DE_ASSERT(caseDef.primitiveType == TESSPRIMITIVETYPE_TRIANGLES || caseDef.primitiveType == TESSPRIMITIVETYPE_QUADS);

	initCommonPrograms(programCollection, caseDef);

	// Tessellation evaluation shader
	{
		std::ostringstream src;
		src << glu::getGLSLVersionDeclaration(glu::GLSL_VERSION_310_ES) << "\n"
			<< "#extension GL_EXT_tessellation_shader : require\n"
			<< "\n"
			<< "layout(" << getTessPrimitiveTypeShaderName(caseDef.primitiveType) << ", "
						 << getSpacingModeShaderName(caseDef.spacingMode) << ") in;\n"
			<< "\n"
			<< "layout(location = 0) in  highp vec2 in_te_position[];\n"
			<< "layout(location = 0) out highp vec4 in_f_color;\n"
			<< "\n"
			<< "void main (void)\n"
			<< "{\n"
			<<	(caseDef.primitiveType == TESSPRIMITIVETYPE_TRIANGLES ?
					"    highp float d = 3.0 * min(gl_TessCoord.x, min(gl_TessCoord.y, gl_TessCoord.z));\n"
					"    highp vec2 corner0 = in_te_position[0];\n"
					"    highp vec2 corner1 = in_te_position[1];\n"
					"    highp vec2 corner2 = in_te_position[2];\n"
					"    highp vec2 pos =  corner0*gl_TessCoord.x + corner1*gl_TessCoord.y + corner2*gl_TessCoord.z;\n"
					"    highp vec2 fromCenter = pos - (corner0 + corner1 + corner2) / 3.0;\n"
					"    highp float f = (1.0 - length(fromCenter)) * (1.5 - d);\n"
					"    pos += 0.75 * f * fromCenter / (length(fromCenter) + 0.3);\n"
					"    gl_Position = vec4(pos, 0.0, 1.0);\n"
				: caseDef.primitiveType == TESSPRIMITIVETYPE_QUADS ?
					"    highp vec2 corner0 = in_te_position[0];\n"
					"    highp vec2 corner1 = in_te_position[1];\n"
					"    highp vec2 corner2 = in_te_position[2];\n"
					"    highp vec2 corner3 = in_te_position[3];\n"
					"    highp vec2 pos = (1.0-gl_TessCoord.x)*(1.0-gl_TessCoord.y)*corner0\n"
					"                   + (    gl_TessCoord.x)*(1.0-gl_TessCoord.y)*corner1\n"
					"                   + (1.0-gl_TessCoord.x)*(    gl_TessCoord.y)*corner2\n"
					"                   + (    gl_TessCoord.x)*(    gl_TessCoord.y)*corner3;\n"
					"    highp float d = 2.0 * min(abs(gl_TessCoord.x-0.5), abs(gl_TessCoord.y-0.5));\n"
					"    highp vec2 fromCenter = pos - (corner0 + corner1 + corner2 + corner3) / 4.0;\n"
					"    highp float f = (1.0 - length(fromCenter)) * sqrt(1.7 - d);\n"
					"    pos += 0.75 * f * fromCenter / (length(fromCenter) + 0.3);\n"
					"    gl_Position = vec4(pos, 0.0, 1.0);\n"
				: "")
			<< "    in_f_color = vec4(1.0);\n"
			<< "}\n";

		programCollection.glslSources.add("tese") << glu::TessellationEvaluationSource(src.str());
	}
}

void initProgramsFillNonOverlapCase (vk::SourceCollections& programCollection, const CaseDefinition caseDef)
{
	DE_ASSERT(caseDef.primitiveType == TESSPRIMITIVETYPE_TRIANGLES || caseDef.primitiveType == TESSPRIMITIVETYPE_QUADS);

	initCommonPrograms(programCollection, caseDef);

	// Tessellation evaluation shader
	{
		std::ostringstream src;
		src << glu::getGLSLVersionDeclaration(glu::GLSL_VERSION_310_ES) << "\n"
			<< "#extension GL_EXT_tessellation_shader : require\n"
			<< "\n"
			<< "layout(" << getTessPrimitiveTypeShaderName(caseDef.primitiveType) << ", "
						 << getSpacingModeShaderName(caseDef.spacingMode) << ") in;\n"
			<< "\n"
			<< getTessLevelsSSBODeclaration()
			<< "\n"
			<< "layout(location = 0) in  highp vec2 in_te_position[];\n"
			<< "layout(location = 0) out highp vec4 in_f_color;\n"
			<< "\n"
			<< "void main (void)\n"
			<< "{\n"
			<<	(caseDef.primitiveType == TESSPRIMITIVETYPE_TRIANGLES ?
					"    highp vec2 corner0 = in_te_position[0];\n"
					"    highp vec2 corner1 = in_te_position[1];\n"
					"    highp vec2 corner2 = in_te_position[2];\n"
					"    highp vec2 pos =  corner0*gl_TessCoord.x + corner1*gl_TessCoord.y + corner2*gl_TessCoord.z;\n"
					"    gl_Position = vec4(pos, 0.0, 1.0);\n"
					"    highp int numConcentricTriangles = int(round(sb_levels.inner0)) / 2 + 1;\n"
					"    highp float d = 3.0 * min(gl_TessCoord.x, min(gl_TessCoord.y, gl_TessCoord.z));\n"
					"    highp int phase = int(d*float(numConcentricTriangles)) % 3;\n"
					"    in_f_color = phase == 0 ? vec4(1.0, 0.0, 0.0, 1.0)\n"
					"               : phase == 1 ? vec4(0.0, 1.0, 0.0, 1.0)\n"
					"               :              vec4(0.0, 0.0, 1.0, 1.0);\n"
				: caseDef.primitiveType == TESSPRIMITIVETYPE_QUADS ?
					"    highp vec2 corner0 = in_te_position[0];\n"
					"    highp vec2 corner1 = in_te_position[1];\n"
					"    highp vec2 corner2 = in_te_position[2];\n"
					"    highp vec2 corner3 = in_te_position[3];\n"
					"    highp vec2 pos = (1.0-gl_TessCoord.x)*(1.0-gl_TessCoord.y)*corner0\n"
					"                   + (    gl_TessCoord.x)*(1.0-gl_TessCoord.y)*corner1\n"
					"                   + (1.0-gl_TessCoord.x)*(    gl_TessCoord.y)*corner2\n"
					"                   + (    gl_TessCoord.x)*(    gl_TessCoord.y)*corner3;\n"
					"    gl_Position = vec4(pos, 0.0, 1.0);\n"
					"    highp int phaseX = int(round((0.5 - abs(gl_TessCoord.x-0.5)) * sb_levels.inner0));\n"
					"    highp int phaseY = int(round((0.5 - abs(gl_TessCoord.y-0.5)) * sb_levels.inner1));\n"
					"    highp int phase = min(phaseX, phaseY) % 3;\n"
					"    in_f_color = phase == 0 ? vec4(1.0, 0.0, 0.0, 1.0)\n"
					"               : phase == 1 ? vec4(0.0, 1.0, 0.0, 1.0)\n"
					"               :              vec4(0.0, 0.0, 1.0, 1.0);\n"
					: "")
			<< "}\n";

		programCollection.glslSources.add("tese") << glu::TessellationEvaluationSource(src.str());
	}
}

void initProgramsIsolinesCase (vk::SourceCollections& programCollection, const CaseDefinition caseDef)
{
	DE_ASSERT(caseDef.primitiveType == TESSPRIMITIVETYPE_ISOLINES);

	initCommonPrograms(programCollection, caseDef);

	// Tessellation evaluation shader
	{
		std::ostringstream src;
		src << glu::getGLSLVersionDeclaration(glu::GLSL_VERSION_310_ES) << "\n"
			<< "#extension GL_EXT_tessellation_shader : require\n"
			<< "\n"
			<< "layout(" << getTessPrimitiveTypeShaderName(caseDef.primitiveType) << ", "
						 << getSpacingModeShaderName(caseDef.spacingMode) << ") in;\n"
			<< "\n"
			<< getTessLevelsSSBODeclaration()
			<< "\n"
			<< "layout(location = 0) in  highp vec2 in_te_position[];\n"
			<< "layout(location = 0) out highp vec4 in_f_color;\n"
			<< "\n"
			<< "void main (void)\n"
			<< "{\n"
			<< "    highp vec2 corner0 = in_te_position[0];\n"
			<< "    highp vec2 corner1 = in_te_position[1];\n"
			<< "    highp vec2 corner2 = in_te_position[2];\n"
			<< "    highp vec2 corner3 = in_te_position[3];\n"
			<< "    highp vec2 pos = (1.0-gl_TessCoord.x)*(1.0-gl_TessCoord.y)*corner0\n"
			<< "                   + (    gl_TessCoord.x)*(1.0-gl_TessCoord.y)*corner1\n"
			<< "                   + (1.0-gl_TessCoord.x)*(    gl_TessCoord.y)*corner2\n"
			<< "                   + (    gl_TessCoord.x)*(    gl_TessCoord.y)*corner3;\n"
			<< "    pos.y += 0.15*sin(gl_TessCoord.x*10.0);\n"
			<< "    gl_Position = vec4(pos, 0.0, 1.0);\n"
			<< "    highp int phaseX = int(round(gl_TessCoord.x*sb_levels.outer1));\n"
			<< "    highp int phaseY = int(round(gl_TessCoord.y*sb_levels.outer0));\n"
			<< "    highp int phase = (phaseX + phaseY) % 3;\n"
			<< "    in_f_color = phase == 0 ? vec4(1.0, 0.0, 0.0, 1.0)\n"
			<< "               : phase == 1 ? vec4(0.0, 1.0, 0.0, 1.0)\n"
			<< "               :              vec4(0.0, 0.0, 1.0, 1.0);\n"
			<< "}\n";

		programCollection.glslSources.add("tese") << glu::TessellationEvaluationSource(src.str());
	}
}

inline std::string getReferenceImagePathPrefix (const std::string& caseName)
{
	return "vulkan/data/tessellation/" + caseName + "_ref";
}

} // anonymous

//! These tests correspond to dEQP-GLES31.functional.tessellation.misc_draw.*
tcu::TestCaseGroup* createMiscDrawTests (tcu::TestContext& testCtx)
{
	de::MovePtr<tcu::TestCaseGroup> group (new tcu::TestCaseGroup(testCtx, "misc_draw", "Miscellaneous draw-result-verifying cases"));

	static const TessPrimitiveType primitivesNoIsolines[] =
	{
		TESSPRIMITIVETYPE_TRIANGLES,
		TESSPRIMITIVETYPE_QUADS,
	};

	// Triangle fill case
	for (int primitiveTypeNdx = 0; primitiveTypeNdx < DE_LENGTH_OF_ARRAY(primitivesNoIsolines); ++primitiveTypeNdx)
	for (int spacingModeNdx = 0; spacingModeNdx < SPACINGMODE_LAST; ++spacingModeNdx)
	{
		const TessPrimitiveType primitiveType = primitivesNoIsolines[primitiveTypeNdx];
		const SpacingMode		spacingMode	  = static_cast<SpacingMode>(spacingModeNdx);
		const std::string		caseName	  = std::string() + "fill_cover_" + getTessPrimitiveTypeShaderName(primitiveType) + "_" + getSpacingModeShaderName(spacingMode);

		addFunctionCaseWithPrograms(group.get(), caseName, "Check that there are no obvious gaps in the triangle-filled area of a tessellated shape",
									initProgramsFillCoverCase, runTest, makeCaseDefinition(primitiveType, spacingMode, getReferenceImagePathPrefix(caseName)));
	}

	// Triangle non-overlap case
	for (int primitiveTypeNdx = 0; primitiveTypeNdx < DE_LENGTH_OF_ARRAY(primitivesNoIsolines); ++primitiveTypeNdx)
	for (int spacingModeNdx = 0; spacingModeNdx < SPACINGMODE_LAST; ++spacingModeNdx)
	{
		const TessPrimitiveType primitiveType = primitivesNoIsolines[primitiveTypeNdx];
		const SpacingMode		spacingMode	  = static_cast<SpacingMode>(spacingModeNdx);
		const std::string		caseName	  = std::string() + "fill_overlap_" + getTessPrimitiveTypeShaderName(primitiveType) + "_" + getSpacingModeShaderName(spacingMode);

		addFunctionCaseWithPrograms(group.get(), caseName, "Check that there are no obvious triangle overlaps in the triangle-filled area of a tessellated shape",
									initProgramsFillNonOverlapCase, runTest, makeCaseDefinition(primitiveType, spacingMode, getReferenceImagePathPrefix(caseName)));
	}

	// Isolines
	for (int spacingModeNdx = 0; spacingModeNdx < SPACINGMODE_LAST; ++spacingModeNdx)
	{
		const SpacingMode spacingMode = static_cast<SpacingMode>(spacingModeNdx);
		const std::string caseName    = std::string() + "isolines_" + getSpacingModeShaderName(spacingMode);

		addFunctionCaseWithPrograms(group.get(), caseName, "Basic isolines render test",
									initProgramsIsolinesCase, runTest, makeCaseDefinition(TESSPRIMITIVETYPE_ISOLINES, spacingMode, getReferenceImagePathPrefix(caseName)));
	}

	return group.release();
}

} // tessellation
} // vkt
