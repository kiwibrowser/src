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
 * \brief Tessellation Primitive Discard Tests
 *//*--------------------------------------------------------------------*/

#include "vktTessellationPrimitiveDiscardTests.hpp"
#include "vktTestCaseUtil.hpp"
#include "vktTessellationUtil.hpp"

#include "tcuTestLog.hpp"

#include "vkDefs.hpp"
#include "vkQueryUtil.hpp"
#include "vkBuilderUtil.hpp"
#include "vkImageUtil.hpp"
#include "vkTypeUtil.hpp"

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
	Winding				winding;
	bool				usePointMode;
	bool				useLessThanOneInnerLevels;
};

bool lessThanOneInnerLevelsDefined (const CaseDefinition& caseDef)
{
	// From Vulkan API specification:
	// >> When tessellating triangles or quads in point mode with fractional odd spacing, the tessellator
	// >> ***may*** produce interior vertices that are positioned on the edge of the patch if an inner
	// >> tessellation level is less than or equal to one.
	return !((caseDef.primitiveType == vkt::tessellation::TESSPRIMITIVETYPE_QUADS      ||
			  caseDef.primitiveType == vkt::tessellation::TESSPRIMITIVETYPE_TRIANGLES) &&
			 caseDef.usePointMode                                                     &&
			 caseDef.spacingMode == vkt::tessellation::SPACINGMODE_FRACTIONAL_ODD);
}

int intPow (int base, int exp)
{
	DE_ASSERT(exp >= 0);
	if (exp == 0)
		return 1;
	else
	{
		const int sub = intPow(base, exp/2);
		if (exp % 2 == 0)
			return sub*sub;
		else
			return sub*sub*base;
	}
}

std::vector<float> genAttributes (bool useLessThanOneInnerLevels)
{
	// Generate input attributes (tessellation levels, and position scale and
	// offset) for a number of primitives. Each primitive has a different
	// combination of tessellatio levels; each level is either a valid
	// value or an "invalid" value (negative or zero, chosen from
	// invalidTessLevelChoices).

	// \note The attributes are generated in such an order that all of the
	//		 valid attribute tuples come before the first invalid one both
	//		 in the result vector, and when scanning the resulting 2d grid
	//		 of primitives is scanned in y-major order. This makes
	//		 verification somewhat simpler.

	static const float	baseTessLevels[6]			= { 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f };
	static const float	invalidTessLevelChoices[]	= { -0.42f, 0.0f };
	const int			numChoices					= 1 + DE_LENGTH_OF_ARRAY(invalidTessLevelChoices);
	float				choices[6][numChoices];
	std::vector<float>	result;

	for (int levelNdx = 0; levelNdx < 6; levelNdx++)
		for (int choiceNdx = 0; choiceNdx < numChoices; choiceNdx++)
			choices[levelNdx][choiceNdx] = (choiceNdx == 0 || !useLessThanOneInnerLevels) ? baseTessLevels[levelNdx] : invalidTessLevelChoices[choiceNdx-1];

	{
		const int	numCols	= intPow(numChoices, 6/2); // sqrt(numChoices**6) == sqrt(number of primitives)
		const int	numRows	= numCols;
		int			index	= 0;
		int			i[6];
		// We could do this with some generic combination-generation function, but meh, it's not that bad.
		for (i[2] = 0; i[2] < numChoices; i[2]++) // First  outer
		for (i[3] = 0; i[3] < numChoices; i[3]++) // Second outer
		for (i[4] = 0; i[4] < numChoices; i[4]++) // Third  outer
		for (i[5] = 0; i[5] < numChoices; i[5]++) // Fourth outer
		for (i[0] = 0; i[0] < numChoices; i[0]++) // First  inner
		for (i[1] = 0; i[1] < numChoices; i[1]++) // Second inner
		{
			for (int j = 0; j < 6; j++)
				result.push_back(choices[j][i[j]]);

			{
				const int col = index % numCols;
				const int row = index / numCols;
				// Position scale.
				result.push_back((float)2.0f / (float)numCols);
				result.push_back((float)2.0f / (float)numRows);
				// Position offset.
				result.push_back((float)col / (float)numCols * 2.0f - 1.0f);
				result.push_back((float)row / (float)numRows * 2.0f - 1.0f);
			}

			index++;
		}
	}

	return result;
}

//! Check that white pixels are found around every non-discarded patch,
//! and that only black pixels are found after the last non-discarded patch.
//! Returns true on successful comparison.
bool verifyResultImage (tcu::TestLog&						log,
						const int							numPrimitives,
						const int							numAttribsPerPrimitive,
						const TessPrimitiveType				primitiveType,
						const std::vector<float>&			attributes,
						const tcu::ConstPixelBufferAccess	pixels)
{
	const tcu::Vec4 black(0.0f, 0.0f, 0.0f, 1.0f);
	const tcu::Vec4 white(1.0f, 1.0f, 1.0f, 1.0f);

	int lastWhitePixelRow								= 0;
	int secondToLastWhitePixelRow						= 0;
	int	lastWhitePixelColumnOnSecondToLastWhitePixelRow	= 0;

	for (int patchNdx = 0; patchNdx < numPrimitives; ++patchNdx)
	{
		const float* const	attr			= &attributes[numAttribsPerPrimitive*patchNdx];
		const bool			validLevels		= !isPatchDiscarded(primitiveType, &attr[2]);

		if (validLevels)
		{
			// Not a discarded patch; check that at least one white pixel is found in its area.

			const float* const	scale		= &attr[6];
			const float* const	offset		= &attr[8];
			const int			x0			= (int)((			offset[0] + 1.0f) * 0.5f * (float)pixels.getWidth()) - 1;
			const int			x1			= (int)((scale[0] + offset[0] + 1.0f) * 0.5f * (float)pixels.getWidth()) + 1;
			const int			y0			= (int)((			offset[1] + 1.0f) * 0.5f * (float)pixels.getHeight()) - 1;
			const int			y1			= (int)((scale[1] + offset[1] + 1.0f) * 0.5f * (float)pixels.getHeight()) + 1;
			bool				pixelOk		= false;

			if (y1 > lastWhitePixelRow)
			{
				secondToLastWhitePixelRow	= lastWhitePixelRow;
				lastWhitePixelRow			= y1;
			}
			lastWhitePixelColumnOnSecondToLastWhitePixelRow = x1;

			for (int y = y0; y <= y1 && !pixelOk; y++)
			for (int x = x0; x <= x1 && !pixelOk; x++)
			{
				if (!de::inBounds(x, 0, pixels.getWidth()) || !de::inBounds(y, 0, pixels.getHeight()))
					continue;

				if (pixels.getPixel(x, y) == white)
					pixelOk = true;
			}

			if (!pixelOk)
			{
				log << tcu::TestLog::Message
					<< "Failure: expected at least one white pixel in the rectangle "
					<< "[x0=" << x0 << ", y0=" << y0 << ", x1=" << x1 << ", y1=" << y1 << "]"
					<< tcu::TestLog::EndMessage
					<< tcu::TestLog::Message
					<< "Note: the rectangle approximately corresponds to the patch with these tessellation levels: "
					<< getTessellationLevelsString(&attr[0], &attr[1])
					<< tcu::TestLog::EndMessage;

				return false;
			}
		}
		else
		{
			// First discarded primitive patch; the remaining are guaranteed to be discarded ones as well.

			for (int y = 0; y < pixels.getHeight(); y++)
			for (int x = 0; x < pixels.getWidth(); x++)
			{
				if (y > lastWhitePixelRow || (y > secondToLastWhitePixelRow && x > lastWhitePixelColumnOnSecondToLastWhitePixelRow))
				{
					if (pixels.getPixel(x, y) != black)
					{
						log << tcu::TestLog::Message
							<< "Failure: expected all pixels to be black in the area "
							<< (lastWhitePixelColumnOnSecondToLastWhitePixelRow < pixels.getWidth()-1
								? std::string() + "y > " + de::toString(lastWhitePixelRow) + " || (y > " + de::toString(secondToLastWhitePixelRow)
												+ " && x > " + de::toString(lastWhitePixelColumnOnSecondToLastWhitePixelRow) + ")"
								: std::string() + "y > " + de::toString(lastWhitePixelRow))
							<< " (they all correspond to patches that should be discarded)"
							<< tcu::TestLog::EndMessage
							<< tcu::TestLog::Message << "Note: pixel " << tcu::IVec2(x, y) << " isn't black" << tcu::TestLog::EndMessage;

						return false;
					}
				}
			}
			break;
		}
	}
	return true;
}

int expectedVertexCount (const int					numPrimitives,
						 const int					numAttribsPerPrimitive,
						 const TessPrimitiveType	primitiveType,
						 const SpacingMode			spacingMode,
						 const std::vector<float>&	attributes)
{
	int count = 0;
	for (int patchNdx = 0; patchNdx < numPrimitives; ++patchNdx)
		count += referenceVertexCount(primitiveType, spacingMode, true, &attributes[numAttribsPerPrimitive*patchNdx+0], &attributes[numAttribsPerPrimitive*patchNdx+2]);
	return count;
}

void initPrograms (vk::SourceCollections& programCollection, const CaseDefinition caseDef)
{
	// Vertex shader
	{
		std::ostringstream src;
		src << glu::getGLSLVersionDeclaration(glu::GLSL_VERSION_310_ES) << "\n"
			<< "\n"
			<< "layout(location = 0) in  highp float in_v_attr;\n"
			<< "layout(location = 0) out highp float in_tc_attr;\n"
			<< "\n"
			<< "void main (void)\n"
			<< "{\n"
			<< "    in_tc_attr = in_v_attr;\n"
			<< "}\n";

		programCollection.glslSources.add("vert") << glu::VertexSource(src.str());
	}

	// Tessellation control shader
	{
		std::ostringstream src;
		src << glu::getGLSLVersionDeclaration(glu::GLSL_VERSION_310_ES) << "\n"
			<< "#extension GL_EXT_tessellation_shader : require\n"
			<< "\n"
			<< "layout(vertices = 1) out;\n"
			<< "\n"
			<< "layout(location = 0) in highp float in_tc_attr[];\n"
			<< "\n"
			<< "layout(location = 0) patch out highp vec2 in_te_positionScale;\n"
			<< "layout(location = 1) patch out highp vec2 in_te_positionOffset;\n"
			<< "\n"
			<< "void main (void)\n"
			<< "{\n"
			<< "    in_te_positionScale  = vec2(in_tc_attr[6], in_tc_attr[7]);\n"
			<< "    in_te_positionOffset = vec2(in_tc_attr[8], in_tc_attr[9]);\n"
			<< "\n"
			<< "    gl_TessLevelInner[0] = in_tc_attr[0];\n"
			<< "    gl_TessLevelInner[1] = in_tc_attr[1];\n"
			<< "\n"
			<< "    gl_TessLevelOuter[0] = in_tc_attr[2];\n"
			<< "    gl_TessLevelOuter[1] = in_tc_attr[3];\n"
			<< "    gl_TessLevelOuter[2] = in_tc_attr[4];\n"
			<< "    gl_TessLevelOuter[3] = in_tc_attr[5];\n"
			<< "}\n";

		programCollection.glslSources.add("tesc") << glu::TessellationControlSource(src.str());
	}

	// Tessellation evaluation shader
	// When using point mode we need two variants of the shader, one for the case where
	// shaderTessellationAndGeometryPointSize is enabled (in which the tessellation evaluation
	// shader needs to write to gl_PointSize for it to be defined) and one for the case where
	// it is disabled, in which we can't write to gl_PointSize but it has a default value
	// of 1.0
	{
		const deUint32	numVariants			= caseDef.usePointMode ? 2 : 1;
		for (deUint32 variant = 0; variant < numVariants; variant++)
		{
			const bool	needPointSizeWrite	= caseDef.usePointMode && variant == 1;

			std::ostringstream src;
			src << glu::getGLSLVersionDeclaration(glu::GLSL_VERSION_310_ES) << "\n"
				<< "#extension GL_EXT_tessellation_shader : require\n";
			if (needPointSizeWrite)
			{
				src << "#extension GL_EXT_tessellation_point_size : require\n";
			}
			src << "\n"
				<< "layout(" << getTessPrimitiveTypeShaderName(caseDef.primitiveType) << ", "
							 << getSpacingModeShaderName(caseDef.spacingMode) << ", "
							 << getWindingShaderName(caseDef.winding)
							 << (caseDef.usePointMode ? ", point_mode" : "") << ") in;\n"
				<< "\n"
				<< "layout(location = 0) patch in highp vec2 in_te_positionScale;\n"
				<< "layout(location = 1) patch in highp vec2 in_te_positionOffset;\n"
				<< "\n"
				<< "layout(set = 0, binding = 0, std430) coherent restrict buffer Output {\n"
				<< "    int  numInvocations;\n"
				<< "} sb_out;\n"
				<< "\n"
				<< "void main (void)\n"
				<< "{\n"
				<< "    atomicAdd(sb_out.numInvocations, 1);\n"
				<< "\n"
				<< "    gl_Position = vec4(gl_TessCoord.xy*in_te_positionScale + in_te_positionOffset, 0.0, 1.0);\n";
			if (needPointSizeWrite)
			{
				src << "    gl_PointSize = 1.0;\n";
			}
			src << "}\n";

			programCollection.glslSources.add(needPointSizeWrite ? "tese_psw" : "tese") << glu::TessellationEvaluationSource(src.str());
		}
	}

	// Fragment shader
	{
		std::ostringstream src;
		src << glu::getGLSLVersionDeclaration(glu::GLSL_VERSION_310_ES) << "\n"
			<< "\n"
			<< "layout(location = 0) out mediump vec4 o_color;\n"
			<< "\n"
			<< "void main (void)\n"
			<< "{\n"
			<< "    o_color = vec4(1.0);\n"
			<< "}\n";

		programCollection.glslSources.add("frag") << glu::FragmentSource(src.str());
	}
}

/*--------------------------------------------------------------------*//*!
 * \brief Test that patch is discarded if relevant outer level <= 0.0
 *
 * Draws patches with different combinations of tessellation levels,
 * varying which levels are negative. Verifies by checking that white
 * pixels exist inside the area of valid primitives, and only black pixels
 * exist inside the area of discarded primitives. An additional sanity
 * test is done, checking that the number of primitives written by shader is
 * correct.
 *//*--------------------------------------------------------------------*/
tcu::TestStatus test (Context& context, const CaseDefinition caseDef)
{
	requireFeatures(context.getInstanceInterface(), context.getPhysicalDevice(), FEATURE_TESSELLATION_SHADER | FEATURE_VERTEX_PIPELINE_STORES_AND_ATOMICS);

	const DeviceInterface&	vk					= context.getDeviceInterface();
	const VkDevice			device				= context.getDevice();
	const VkQueue			queue				= context.getUniversalQueue();
	const deUint32			queueFamilyIndex	= context.getUniversalQueueFamilyIndex();
	Allocator&				allocator			= context.getDefaultAllocator();

	const std::vector<float>	attributes				= genAttributes(caseDef.useLessThanOneInnerLevels);
	const int					numAttribsPerPrimitive	= 6 + 2 + 2; // Tess levels, scale, offset.
	const int					numPrimitives			= static_cast<int>(attributes.size() / numAttribsPerPrimitive);
	const int					numExpectedVertices		= expectedVertexCount(numPrimitives, numAttribsPerPrimitive, caseDef.primitiveType, caseDef.spacingMode, attributes);

	// Check the convenience assertion that all discarded patches come after the last non-discarded patch.
	{
		bool discardedPatchEncountered = false;
		for (int patchNdx = 0; patchNdx < numPrimitives; ++patchNdx)
		{
			const bool discard = isPatchDiscarded(caseDef.primitiveType, &attributes[numAttribsPerPrimitive*patchNdx + 2]);
			DE_ASSERT(discard || !discardedPatchEncountered);
			discardedPatchEncountered = discard;
		}
		DE_UNREF(discardedPatchEncountered);
	}

	// Vertex input attributes buffer

	const VkFormat	   vertexFormat		   = VK_FORMAT_R32_SFLOAT;
	const deUint32	   vertexStride		   = tcu::getPixelSize(mapVkFormat(vertexFormat));
	const VkDeviceSize vertexDataSizeBytes = sizeInBytes(attributes);
	const Buffer	   vertexBuffer		   (vk, device, allocator, makeBufferCreateInfo(vertexDataSizeBytes, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT), MemoryRequirement::HostVisible);

	DE_ASSERT(static_cast<int>(attributes.size()) == numPrimitives * numAttribsPerPrimitive);
	DE_ASSERT(sizeof(attributes[0]) == vertexStride);

	{
		const Allocation& alloc = vertexBuffer.getAllocation();
		deMemcpy(alloc.getHostPtr(), &attributes[0], static_cast<std::size_t>(vertexDataSizeBytes));
		flushMappedMemoryRange(vk, device, alloc.getMemory(), alloc.getOffset(), vertexDataSizeBytes);
		// No barrier needed, flushed memory is automatically visible
	}

	// Output buffer: number of invocations

	const VkDeviceSize resultBufferSizeBytes = sizeof(deInt32);
	const Buffer	   resultBuffer			 (vk, device, allocator, makeBufferCreateInfo(resultBufferSizeBytes, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT), MemoryRequirement::HostVisible);

	{
		const Allocation& alloc = resultBuffer.getAllocation();
		deMemset(alloc.getHostPtr(), 0, static_cast<std::size_t>(resultBufferSizeBytes));
		flushMappedMemoryRange(vk, device, alloc.getMemory(), alloc.getOffset(), resultBufferSizeBytes);
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
	const Buffer colorBuffer(vk, device, allocator,
		makeBufferCreateInfo(colorBufferSizeBytes, VK_BUFFER_USAGE_TRANSFER_DST_BIT), MemoryRequirement::HostVisible);

	// Descriptors

	const Unique<VkDescriptorSetLayout> descriptorSetLayout(DescriptorSetLayoutBuilder()
		.addSingleBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT)
		.build(vk, device));

	const Unique<VkDescriptorPool> descriptorPool(DescriptorPoolBuilder()
		.addType(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
		.build(vk, device, VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT, 1u));

	const Unique<VkDescriptorSet> descriptorSet    (makeDescriptorSet(vk, device, *descriptorPool, *descriptorSetLayout));
	const VkDescriptorBufferInfo  resultBufferInfo = makeDescriptorBufferInfo(resultBuffer.get(), 0ull, resultBufferSizeBytes);

	DescriptorSetUpdateBuilder()
		.writeSingle(*descriptorSet, DescriptorSetUpdateBuilder::Location::binding(0u), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, &resultBufferInfo)
		.update(vk, device);

	// Pipeline

	const Unique<VkImageView>		colorAttachmentView	(makeImageView(vk, device, *colorAttachmentImage, VK_IMAGE_VIEW_TYPE_2D, colorFormat, colorImageSubresourceRange));
	const Unique<VkRenderPass>		renderPass			(makeRenderPass(vk, device, colorFormat));
	const Unique<VkFramebuffer>		framebuffer			(makeFramebuffer(vk, device, *renderPass, *colorAttachmentView, renderSize.x(), renderSize.y(), 1u));
	const Unique<VkPipelineLayout>	pipelineLayout		(makePipelineLayout(vk, device, *descriptorSetLayout));
	const Unique<VkCommandPool>		cmdPool				(makeCommandPool(vk, device, queueFamilyIndex));
	const Unique<VkCommandBuffer>	cmdBuffer			(allocateCommandBuffer(vk, device, *cmdPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY));
	const bool						needPointSizeWrite	= getPhysicalDeviceFeatures(context.getInstanceInterface(), context.getPhysicalDevice()).shaderTessellationAndGeometryPointSize && caseDef.usePointMode;

	const Unique<VkPipeline> pipeline(GraphicsPipelineBuilder()
		.setRenderSize				  (renderSize)
		.setPatchControlPoints		  (numAttribsPerPrimitive)
		.setVertexInputSingleAttribute(vertexFormat, vertexStride)
		.setShader					  (vk, device, VK_SHADER_STAGE_VERTEX_BIT,					context.getBinaryCollection().get("vert"), DE_NULL)
		.setShader					  (vk, device, VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT,	context.getBinaryCollection().get("tesc"), DE_NULL)
		.setShader					  (vk, device, VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT, context.getBinaryCollection().get(needPointSizeWrite ? "tese_psw" : "tese"), DE_NULL)
		.setShader					  (vk, device, VK_SHADER_STAGE_FRAGMENT_BIT,				context.getBinaryCollection().get("frag"), DE_NULL)
		.build						  (vk, device, *pipelineLayout, *renderPass));

	context.getTestContext().getLog()
		<< tcu::TestLog::Message
		<< "Note: rendering " << numPrimitives << " patches; first patches have valid relevant outer levels, "
		<< "but later patches have one or more invalid (i.e. less than or equal to 0.0) relevant outer levels"
		<< tcu::TestLog::EndMessage;

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
		const tcu::Vec4 clearColor = tcu::Vec4(0.0f, 0.0f, 0.0f, 1.0f);

		beginRenderPass(vk, *cmdBuffer, *renderPass, *framebuffer, renderArea, clearColor);
	}

	vk.cmdBindPipeline(*cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *pipeline);
	vk.cmdBindDescriptorSets(*cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *pipelineLayout, 0u, 1u, &descriptorSet.get(), 0u, DE_NULL);
	{
		const VkDeviceSize vertexBufferOffset = 0ull;
		vk.cmdBindVertexBuffers(*cmdBuffer, 0u, 1u, &vertexBuffer.get(), &vertexBufferOffset);
	}

	vk.cmdDraw(*cmdBuffer, static_cast<deUint32>(attributes.size()), 1u, 0u, 0u);
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
	{
		const VkBufferMemoryBarrier shaderWriteBarrier = makeBufferMemoryBarrier(
			VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_HOST_READ_BIT, *resultBuffer, 0ull, resultBufferSizeBytes);

		vk.cmdPipelineBarrier(*cmdBuffer, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, VK_PIPELINE_STAGE_HOST_BIT, 0u,
			0u, DE_NULL, 1u, &shaderWriteBarrier, 0u, DE_NULL);
	}

	endCommandBuffer(vk, *cmdBuffer);
	submitCommandsAndWait(vk, device, queue, *cmdBuffer);

	{
		// Log rendered image
		const Allocation& colorBufferAlloc = colorBuffer.getAllocation();
		invalidateMappedMemoryRange(vk, device, colorBufferAlloc.getMemory(), colorBufferAlloc.getOffset(), colorBufferSizeBytes);

		const tcu::ConstPixelBufferAccess imagePixelAccess(mapVkFormat(colorFormat), renderSize.x(), renderSize.y(), 1, colorBufferAlloc.getHostPtr());

		tcu::TestLog& log = context.getTestContext().getLog();
		log << tcu::TestLog::Image("color0", "Rendered image", imagePixelAccess);

		// Verify case result
		const Allocation& resultAlloc = resultBuffer.getAllocation();
		invalidateMappedMemoryRange(vk, device, resultAlloc.getMemory(), resultAlloc.getOffset(), resultBufferSizeBytes);

		const deInt32 numResultVertices = *static_cast<deInt32*>(resultAlloc.getHostPtr());

		if (!lessThanOneInnerLevelsDefined(caseDef) && caseDef.useLessThanOneInnerLevels)
		{
			// Since we cannot explicitly determine whether or not such interior vertices are going to be
			// generated, we will not verify the number of generated vertices for fractional odd + quads/triangles
			// tessellation configurations.
			log << tcu::TestLog::Message
				<< "Note: shader invocations generated " << numResultVertices << " vertices (not verified as number of vertices is implementation-dependent)"
				<< tcu::TestLog::EndMessage;
		}
		else if (numResultVertices < numExpectedVertices)
		{
			log << tcu::TestLog::Message
				<< "Failure: expected " << numExpectedVertices << " vertices from shader invocations, but got only " << numResultVertices
				<< tcu::TestLog::EndMessage;
			return tcu::TestStatus::fail("Wrong number of tessellation coordinates");
		}
		else if (numResultVertices == numExpectedVertices)
		{
			log << tcu::TestLog::Message
				<< "Note: shader invocations generated " << numResultVertices << " vertices"
				<< tcu::TestLog::EndMessage;
		}
		else
		{
			log << tcu::TestLog::Message
				<< "Note: shader invocations generated " << numResultVertices << " vertices (expected " << numExpectedVertices << ", got "
				<< (numResultVertices - numExpectedVertices) << " extra)"
				<< tcu::TestLog::EndMessage;
		}

		return (verifyResultImage(log, numPrimitives, numAttribsPerPrimitive, caseDef.primitiveType, attributes, imagePixelAccess)
				? tcu::TestStatus::pass("OK") : tcu::TestStatus::fail("Image verification failed"));
	}
}

} // anonymous

//! These tests correspond to dEQP-GLES31.functional.tessellation.primitive_discard.*
//! \note Original test used transform feedback (TF) to capture the number of output vertices. The behavior of TF differs significantly from SSBO approach,
//!       especially for non-point_mode rendering. TF returned all coordinates, while SSBO computes the count based on the number of shader invocations
//!       which yields a much smaller number because invocations for duplicate coordinates are often eliminated.
//!       Because of this, the test was changed to:
//!       - always compute the number of expected coordinates as if point_mode was enabled
//!       - not fail if implementation returned more coordinates than expected
tcu::TestCaseGroup* createPrimitiveDiscardTests (tcu::TestContext& testCtx)
{
	de::MovePtr<tcu::TestCaseGroup> group (new tcu::TestCaseGroup(testCtx, "primitive_discard", "Test primitive discard with relevant outer tessellation level <= 0.0"));

	for (int primitiveTypeNdx = 0; primitiveTypeNdx < TESSPRIMITIVETYPE_LAST; primitiveTypeNdx++)
	for (int spacingModeNdx = 0; spacingModeNdx < SPACINGMODE_LAST; spacingModeNdx++)
	for (int windingNdx = 0; windingNdx < WINDING_LAST; windingNdx++)
	for (int usePointModeNdx = 0; usePointModeNdx <= 1; usePointModeNdx++)
	for (int lessThanOneInnerLevelsNdx = 0; lessThanOneInnerLevelsNdx <= 1; lessThanOneInnerLevelsNdx++)
	{
		const CaseDefinition caseDef =
		{
			(TessPrimitiveType)primitiveTypeNdx,
			(SpacingMode)spacingModeNdx,
			(Winding)windingNdx,
			(usePointModeNdx != 0),
			(lessThanOneInnerLevelsNdx != 0)
		};

		if (lessThanOneInnerLevelsDefined(caseDef) && !caseDef.useLessThanOneInnerLevels)
			continue; // No point generating a separate case as <= 1 inner level behavior is well-defined

		const std::string caseName = std::string() + getTessPrimitiveTypeShaderName(caseDef.primitiveType)
									 + "_" + getSpacingModeShaderName(caseDef.spacingMode)
									 + "_" + getWindingShaderName(caseDef.winding)
									 + (caseDef.usePointMode ? "_point_mode" : "")
									 + (caseDef.useLessThanOneInnerLevels ? "" : "_valid_levels");

		addFunctionCaseWithPrograms(group.get(), caseName, "", initPrograms, test, caseDef);
	}

	return group.release();
}

} // tessellation
} // vkt
