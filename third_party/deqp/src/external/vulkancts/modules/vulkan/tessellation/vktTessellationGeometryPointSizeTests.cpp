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
* \brief Tessellation Geometry Interaction - Point Size
*//*--------------------------------------------------------------------*/

#include "vktTessellationGeometryPassthroughTests.hpp"
#include "vktTestCaseUtil.hpp"
#include "vktTessellationUtil.hpp"

#include "tcuTestLog.hpp"

#include "vkDefs.hpp"
#include "vkQueryUtil.hpp"
#include "vkBuilderUtil.hpp"
#include "vkTypeUtil.hpp"
#include "vkImageUtil.hpp"

#include "deUniquePtr.hpp"

#include <string>
#include <vector>

namespace vkt
{
namespace tessellation
{

using namespace vk;

namespace
{

enum Constants
{
	RENDER_SIZE = 32,
};

enum FlagBits
{
	FLAG_VERTEX_SET						= 1u << 0,		// !< set gl_PointSize in vertex shader
	FLAG_TESSELLATION_EVALUATION_SET	= 1u << 1,		// !< set gl_PointSize in tessellation evaluation shader
	FLAG_TESSELLATION_ADD				= 1u << 2,		// !< read and add to gl_PointSize in tessellation shader pair
	FLAG_GEOMETRY_SET					= 1u << 3,		// !< set gl_PointSize in geometry shader
	FLAG_GEOMETRY_ADD					= 1u << 4,		// !< read and add to gl_PointSize in geometry shader
};
typedef deUint32 Flags;

void checkPointSizeRequirements (const InstanceInterface& vki, const VkPhysicalDevice physDevice, const int maxPointSize)
{
	const VkPhysicalDeviceProperties properties = getPhysicalDeviceProperties(vki, physDevice);
	if (maxPointSize > static_cast<int>(properties.limits.pointSizeRange[1]))
		throw tcu::NotSupportedError("Test requires point size " + de::toString(maxPointSize));
	// Point size granularity must be 1.0 at most, so no need to check it for this test.
}

int getExpectedPointSize (const Flags flags)
{
	int addition = 0;

	// geometry
	if (flags & FLAG_GEOMETRY_SET)
		return 6;
	else if (flags & FLAG_GEOMETRY_ADD)
		addition += 2;

	// tessellation
	if (flags & FLAG_TESSELLATION_EVALUATION_SET)
		return 4 + addition;
	else if (flags & FLAG_TESSELLATION_ADD)
		addition += 2;

	// vertex
	if (flags & FLAG_VERTEX_SET)
		return 2 + addition;

	// undefined
	DE_ASSERT(false);
	return -1;
}

inline bool isTessellationStage (const Flags flags)
{
	return (flags & (FLAG_TESSELLATION_EVALUATION_SET | FLAG_TESSELLATION_ADD)) != 0;
}

inline bool isGeometryStage (const Flags flags)
{
	return (flags & (FLAG_GEOMETRY_SET | FLAG_GEOMETRY_ADD)) != 0;
}

bool verifyImage (tcu::TestLog& log, const tcu::ConstPixelBufferAccess image, const int expectedSize)
{
	log << tcu::TestLog::Message << "Verifying rendered point size. Expecting " << expectedSize << " pixels." << tcu::TestLog::EndMessage;

	bool			resultAreaFound	= false;
	tcu::IVec4		resultArea;
	const tcu::Vec4	black(0.0, 0.0, 0.0, 1.0);

	// Find rasterization output area

	for (int y = 0; y < image.getHeight(); ++y)
	for (int x = 0; x < image.getWidth();  ++x)
		if (image.getPixel(x, y) != black)
		{
			if (!resultAreaFound)
			{
				// first fragment
				resultArea = tcu::IVec4(x, y, x + 1, y + 1);
				resultAreaFound = true;
			}
			else
			{
				// union area
				resultArea.x() = de::min(resultArea.x(), x);
				resultArea.y() = de::min(resultArea.y(), y);
				resultArea.z() = de::max(resultArea.z(), x+1);
				resultArea.w() = de::max(resultArea.w(), y+1);
			}
		}

	if (!resultAreaFound)
	{
		log << tcu::TestLog::Message << "Verification failed, could not find any point fragments." << tcu::TestLog::EndMessage;
		return false;
	}

	const tcu::IVec2 pointSize = resultArea.swizzle(2,3) - resultArea.swizzle(0, 1);

	if (pointSize.x() != pointSize.y())
	{
		log << tcu::TestLog::Message << "ERROR! Rasterized point is not a square. Point size was " << pointSize << tcu::TestLog::EndMessage;
		return false;
	}

	if (pointSize.x() != expectedSize)
	{
		log << tcu::TestLog::Message << "ERROR! Point size invalid, expected " << expectedSize << ", got " << pointSize.x() << tcu::TestLog::EndMessage;
		return false;
	}

	return true;
}

void initPrograms (vk::SourceCollections& programCollection, const Flags flags)
{
	// Vertex shader
	{
		std::ostringstream src;
		src << glu::getGLSLVersionDeclaration(glu::GLSL_VERSION_310_ES) << "\n"
			<< "\n"
			<< "void main (void)\n"
			<< "{\n"
			<< "    gl_Position = vec4(0.0, 0.0, 0.0, 1.0);\n";

		if (flags & FLAG_VERTEX_SET)
			src << "    gl_PointSize = 2.0;\n";

		src << "}\n";

		programCollection.glslSources.add("vert") << glu::VertexSource(src.str());
	}

	// Fragment shader
	{
		std::ostringstream src;
		src << glu::getGLSLVersionDeclaration(glu::GLSL_VERSION_310_ES) << "\n"
			<< "layout(location = 0) out mediump vec4 fragColor;\n"
			<< "\n"
			<< "void main (void)\n"
			<< "{\n"
			<< "    fragColor = vec4(1.0);\n"
			<< "}\n";

		programCollection.glslSources.add("frag") << glu::FragmentSource(src.str());
	}

	if (isTessellationStage(flags))
	{
		// Tessellation control shader
		{
			std::ostringstream src;
			src << glu::getGLSLVersionDeclaration(glu::GLSL_VERSION_310_ES) << "\n"
				<< "#extension GL_EXT_tessellation_shader : require\n"
				<< "#extension GL_EXT_tessellation_point_size : require\n"
				<< "layout(vertices = 1) out;\n"
				<< "\n"
				<< "void main (void)\n"
				<< "{\n"
				<< "    gl_TessLevelOuter[0] = 3.0;\n"
				<< "    gl_TessLevelOuter[1] = 3.0;\n"
				<< "    gl_TessLevelOuter[2] = 3.0;\n"
				<< "    gl_TessLevelInner[0] = 3.0;\n"
				<< "\n"
				<< "    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;\n";

			if (flags & FLAG_TESSELLATION_ADD)
				src << "    // pass as is to eval\n"
					<< "    gl_out[gl_InvocationID].gl_PointSize = gl_in[gl_InvocationID].gl_PointSize;\n";

			src << "}\n";

			programCollection.glslSources.add("tesc") << glu::TessellationControlSource(src.str());
		}

		// Tessellation evaluation shader
		{
			std::ostringstream src;
			src << glu::getGLSLVersionDeclaration(glu::GLSL_VERSION_310_ES) << "\n"
				<< "#extension GL_EXT_tessellation_shader : require\n"
				<< "#extension GL_EXT_tessellation_point_size : require\n"
				<< "layout(triangles, point_mode) in;\n"
				<< "\n"
				<< "void main (void)\n"
				<< "{\n"
				<< "    // hide all but one vertex\n"
				<< "    if (gl_TessCoord.x < 0.99)\n"
				<< "        gl_Position = vec4(-2.0, 0.0, 0.0, 1.0);\n"
				<< "    else\n"
				<< "        gl_Position = gl_in[0].gl_Position;\n";

			if (flags & FLAG_TESSELLATION_ADD)
				src << "\n"
					<< "    // add to point size\n"
					<< "    gl_PointSize = gl_in[0].gl_PointSize + 2.0;\n";
			else if (flags & FLAG_TESSELLATION_EVALUATION_SET)
				src << "\n"
					<< "    // set point size\n"
					<< "    gl_PointSize = 4.0;\n";

			src << "}\n";

			programCollection.glslSources.add("tese") << glu::TessellationEvaluationSource(src.str());
		}
	}

	if (isGeometryStage(flags))
	{
		// Geometry shader
		std::ostringstream src;
		src << glu::getGLSLVersionDeclaration(glu::GLSL_VERSION_310_ES) << "\n"
			<< "#extension GL_EXT_geometry_shader : require\n"
			<< "#extension GL_EXT_geometry_point_size : require\n"
			<< "layout(points) in;\n"
			<< "layout(points, max_vertices = 1) out;\n"
			<< "\n"
			<< "void main (void)\n"
			<< "{\n"
			<< "    gl_Position  = gl_in[0].gl_Position;\n";

		if (flags & FLAG_GEOMETRY_SET)
			src << "    gl_PointSize = 6.0;\n";
		else if (flags & FLAG_GEOMETRY_ADD)
			src << "    gl_PointSize = gl_in[0].gl_PointSize + 2.0;\n";

		src << "\n"
			<< "    EmitVertex();\n"
			<< "}\n";

		programCollection.glslSources.add("geom") << glu::GeometrySource(src.str());
	}
}

tcu::TestStatus test (Context& context, const Flags flags)
{
	const int expectedPointSize = getExpectedPointSize(flags);
	{
		const InstanceInterface& vki        = context.getInstanceInterface();
		const VkPhysicalDevice   physDevice = context.getPhysicalDevice();

		requireFeatures           (vki, physDevice, FEATURE_TESSELLATION_SHADER | FEATURE_GEOMETRY_SHADER | FEATURE_SHADER_TESSELLATION_AND_GEOMETRY_POINT_SIZE);
		checkPointSizeRequirements(vki, physDevice, expectedPointSize);
	}
	{
		tcu::TestLog& log = context.getTestContext().getLog();

		if (flags & FLAG_VERTEX_SET)
			log << tcu::TestLog::Message << "Setting point size in vertex shader to 2.0." << tcu::TestLog::EndMessage;
		if (flags & FLAG_TESSELLATION_EVALUATION_SET)
			log << tcu::TestLog::Message << "Setting point size in tessellation evaluation shader to 4.0." << tcu::TestLog::EndMessage;
		if (flags & FLAG_TESSELLATION_ADD)
			log << tcu::TestLog::Message << "Reading point size in tessellation control shader and adding 2.0 to it in evaluation." << tcu::TestLog::EndMessage;
		if (flags & FLAG_GEOMETRY_SET)
			log << tcu::TestLog::Message << "Setting point size in geometry shader to 6.0." << tcu::TestLog::EndMessage;
		if (flags & FLAG_GEOMETRY_ADD)
			log << tcu::TestLog::Message << "Reading point size in geometry shader and adding 2.0." << tcu::TestLog::EndMessage;
	}

	const DeviceInterface&	vk					= context.getDeviceInterface();
	const VkDevice			device				= context.getDevice();
	const VkQueue			queue				= context.getUniversalQueue();
	const deUint32			queueFamilyIndex	= context.getUniversalQueueFamilyIndex();
	Allocator&				allocator			= context.getDefaultAllocator();

	// Color attachment

	const tcu::IVec2			  renderSize				 = tcu::IVec2(RENDER_SIZE, RENDER_SIZE);
	const VkFormat				  colorFormat				 = VK_FORMAT_R8G8B8A8_UNORM;
	const VkImageSubresourceRange colorImageSubresourceRange = makeImageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT, 0u, 1u, 0u, 1u);
	const Image					  colorAttachmentImage		 (vk, device, allocator,
															 makeImageCreateInfo(renderSize, colorFormat, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, 1u),
															 MemoryRequirement::Any);

	// Color output buffer

	const VkDeviceSize	colorBufferSizeBytes = renderSize.x()*renderSize.y() * tcu::getPixelSize(mapVkFormat(colorFormat));
	const Buffer		colorBuffer          (vk, device, allocator, makeBufferCreateInfo(colorBufferSizeBytes, VK_BUFFER_USAGE_TRANSFER_DST_BIT), MemoryRequirement::HostVisible);

	// Pipeline

	const Unique<VkImageView>		colorAttachmentView(makeImageView						(vk, device, *colorAttachmentImage, VK_IMAGE_VIEW_TYPE_2D, colorFormat, colorImageSubresourceRange));
	const Unique<VkRenderPass>		renderPass		   (makeRenderPass						(vk, device, colorFormat));
	const Unique<VkFramebuffer>		framebuffer		   (makeFramebuffer						(vk, device, *renderPass, *colorAttachmentView, renderSize.x(), renderSize.y(), 1u));
	const Unique<VkPipelineLayout>	pipelineLayout	   (makePipelineLayoutWithoutDescriptors(vk, device));
	const Unique<VkCommandPool>		cmdPool			   (makeCommandPool						(vk, device, queueFamilyIndex));
	const Unique<VkCommandBuffer>	cmdBuffer		   (allocateCommandBuffer				(vk, device, *cmdPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY));

	GraphicsPipelineBuilder			pipelineBuilder;

	pipelineBuilder
		.setPrimitiveTopology		  (VK_PRIMITIVE_TOPOLOGY_POINT_LIST)
		.setRenderSize				  (renderSize)
		.setPatchControlPoints		  (1)
		.setShader					  (vk, device, VK_SHADER_STAGE_VERTEX_BIT,					context.getBinaryCollection().get("vert"), DE_NULL)
		.setShader					  (vk, device, VK_SHADER_STAGE_FRAGMENT_BIT,				context.getBinaryCollection().get("frag"), DE_NULL);

	if (isTessellationStage(flags))
		pipelineBuilder
			.setShader				  (vk, device, VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT,	context.getBinaryCollection().get("tesc"), DE_NULL)
			.setShader				  (vk, device, VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT, context.getBinaryCollection().get("tese"), DE_NULL);

	if (isGeometryStage(flags))
		pipelineBuilder
			.setShader				  (vk, device, VK_SHADER_STAGE_GEOMETRY_BIT,				context.getBinaryCollection().get("geom"), DE_NULL);

	const Unique<VkPipeline> pipeline(pipelineBuilder.build(vk, device, *pipelineLayout, *renderPass));

	// Draw commands

	beginCommandBuffer(vk, *cmdBuffer);

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

	vk.cmdDraw(*cmdBuffer, 1u, 1u, 0u, 0u);
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

	// Verify results
	{
		const Allocation& alloc = colorBuffer.getAllocation();
		invalidateMappedMemoryRange(vk, device, alloc.getMemory(), alloc.getOffset(), colorBufferSizeBytes);
		tcu::ConstPixelBufferAccess image(mapVkFormat(colorFormat), renderSize.x(), renderSize.y(), 1, alloc.getHostPtr());

		tcu::TestLog& log = context.getTestContext().getLog();
		log << tcu::LogImage("color0", "", image);

		if (verifyImage(log, image, expectedPointSize))
			return tcu::TestStatus::pass("OK");
		else
			return tcu::TestStatus::fail("Didn't render expected point");
	}
}

std::string getTestCaseName (const Flags flags)
{
	std::ostringstream buf;

	// join per-bit descriptions into a single string with '_' separator
	if (flags & FLAG_VERTEX_SET)					buf																		<< "vertex_set";
	if (flags & FLAG_TESSELLATION_EVALUATION_SET)	buf << ((flags & (FLAG_TESSELLATION_EVALUATION_SET-1))	? ("_") : (""))	<< "evaluation_set";
	if (flags & FLAG_TESSELLATION_ADD)				buf << ((flags & (FLAG_TESSELLATION_ADD-1))				? ("_") : (""))	<< "control_pass_eval_add";
	if (flags & FLAG_GEOMETRY_SET)					buf << ((flags & (FLAG_GEOMETRY_SET-1))					? ("_") : (""))	<< "geometry_set";
	if (flags & FLAG_GEOMETRY_ADD)					buf << ((flags & (FLAG_GEOMETRY_ADD-1))					? ("_") : (""))	<< "geometry_add";

	return buf.str();
}

std::string getTestCaseDescription (const Flags flags)
{
	std::ostringstream buf;

	// join per-bit descriptions into a single string with ", " separator
	if (flags & FLAG_VERTEX_SET)					buf																			<< "set point size in vertex shader";
	if (flags & FLAG_TESSELLATION_EVALUATION_SET)	buf << ((flags & (FLAG_TESSELLATION_EVALUATION_SET-1))	? (", ") : (""))	<< "set point size in tessellation evaluation shader";
	if (flags & FLAG_TESSELLATION_ADD)				buf << ((flags & (FLAG_TESSELLATION_ADD-1))				? (", ") : (""))	<< "add to point size in tessellation shader";
	if (flags & FLAG_GEOMETRY_SET)					buf << ((flags & (FLAG_GEOMETRY_SET-1))					? (", ") : (""))	<< "set point size in geometry shader";
	if (flags & FLAG_GEOMETRY_ADD)					buf << ((flags & (FLAG_GEOMETRY_ADD-1))					? (", ") : (""))	<< "add to point size in geometry shader";

	return buf.str();
}

} // anonymous

//! Ported from dEQP-GLES31.functional.tessellation_geometry_interaction.point_size.*
//! with the exception of the default 1.0 point size cases (not valid in Vulkan).
tcu::TestCaseGroup* createGeometryPointSizeTests (tcu::TestContext& testCtx)
{
	de::MovePtr<tcu::TestCaseGroup> group (new tcu::TestCaseGroup(testCtx, "point_size", "Test point size"));

	static const Flags caseFlags[] =
	{
		FLAG_VERTEX_SET,
							FLAG_TESSELLATION_EVALUATION_SET,
																	FLAG_GEOMETRY_SET,
		FLAG_VERTEX_SET	|	FLAG_TESSELLATION_EVALUATION_SET,
		FLAG_VERTEX_SET |											FLAG_GEOMETRY_SET,
		FLAG_VERTEX_SET	|	FLAG_TESSELLATION_EVALUATION_SET	|	FLAG_GEOMETRY_SET,
		FLAG_VERTEX_SET	|	FLAG_TESSELLATION_ADD				|	FLAG_GEOMETRY_ADD,
	};

	for (int ndx = 0; ndx < DE_LENGTH_OF_ARRAY(caseFlags); ++ndx)
	{
		const std::string name = getTestCaseName       (caseFlags[ndx]);
		const std::string desc = getTestCaseDescription(caseFlags[ndx]);

		addFunctionCaseWithPrograms(group.get(), name, desc, initPrograms, test, caseFlags[ndx]);
	}

	return group.release();
}

} // tessellation
} // vkt
