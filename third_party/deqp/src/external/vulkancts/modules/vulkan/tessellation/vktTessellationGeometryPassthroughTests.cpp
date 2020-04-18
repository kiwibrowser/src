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
* \brief Tessellation Geometry Interaction - Passthrough
*//*--------------------------------------------------------------------*/

#include "vktTessellationGeometryPassthroughTests.hpp"
#include "vktTestCaseUtil.hpp"
#include "vktTessellationUtil.hpp"

#include "tcuTestLog.hpp"
#include "tcuImageCompare.hpp"

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

void addVertexAndFragmentShaders (vk::SourceCollections&  programCollection)
{
	// Vertex shader
	{
		std::ostringstream src;
		src << glu::getGLSLVersionDeclaration(glu::GLSL_VERSION_310_ES) << "\n"
			<< "\n"
			<< "layout(location = 0) in  highp vec4 a_position;\n"
			<< "layout(location = 0) out highp vec4 v_vertex_color;\n"
			<< "\n"
			<< "void main (void)\n"
			<< "{\n"
			<< "    gl_Position = a_position;\n"
			<< "    v_vertex_color = vec4(a_position.x * 0.5 + 0.5, a_position.y * 0.5 + 0.5, 1.0, 0.4);\n"
			<< "}\n";

		programCollection.glslSources.add("vert") << glu::VertexSource(src.str());
	}

	// Fragment shader
	{
		std::ostringstream src;
		src << glu::getGLSLVersionDeclaration(glu::GLSL_VERSION_310_ES) << "\n"
			<< "\n"
			<< "layout(location = 0) in  highp   vec4 v_fragment_color;\n"
			<< "layout(location = 0) out mediump vec4 fragColor;\n"
			<< "void main (void)\n"
			<< "{\n"
			<< "	fragColor = v_fragment_color;\n"
			<< "}\n";

		programCollection.glslSources.add("frag") << glu::FragmentSource(src.str());
	}
}

//! Tessellation evaluation shader used in passthrough geometry shader case.
std::string generateTessellationEvaluationShader (const TessPrimitiveType primitiveType, const std::string& colorOutputName)
{
	std::ostringstream	src;
	src <<	glu::getGLSLVersionDeclaration(glu::GLSL_VERSION_310_ES) << "\n"
		<< "#extension GL_EXT_tessellation_shader : require\n"
		<< "layout(" << getTessPrimitiveTypeShaderName(primitiveType) << ") in;\n"
		<< "\n"
		<< "layout(location = 0) in  highp vec4 v_patch_color[];\n"
		<< "layout(location = 0) out highp vec4 " << colorOutputName << ";\n"
		<< "\n"
		<< "// note: No need to use precise gl_Position since we do not require gapless geometry\n"
		<< "void main (void)\n"
		<< "{\n";

	if (primitiveType == TESSPRIMITIVETYPE_TRIANGLES)
		src << "    vec3 weights = vec3(pow(gl_TessCoord.x, 1.3), pow(gl_TessCoord.y, 1.3), pow(gl_TessCoord.z, 1.3));\n"
			<< "    vec3 cweights = gl_TessCoord;\n"
			<< "    gl_Position = vec4(weights.x * gl_in[0].gl_Position.xyz + weights.y * gl_in[1].gl_Position.xyz + weights.z * gl_in[2].gl_Position.xyz, 1.0);\n"
			<< "    " << colorOutputName << " = cweights.x * v_patch_color[0] + cweights.y * v_patch_color[1] + cweights.z * v_patch_color[2];\n";
	else if (primitiveType == TESSPRIMITIVETYPE_QUADS || primitiveType == TESSPRIMITIVETYPE_ISOLINES)
		src << "    vec2 normalizedCoord = (gl_TessCoord.xy * 2.0 - vec2(1.0));\n"
			<< "    vec2 normalizedWeights = normalizedCoord * (vec2(1.0) - 0.3 * cos(normalizedCoord.yx * 1.57));\n"
			<< "    vec2 weights = normalizedWeights * 0.5 + vec2(0.5);\n"
			<< "    vec2 cweights = gl_TessCoord.xy;\n"
			<< "    gl_Position = mix(mix(gl_in[0].gl_Position, gl_in[1].gl_Position, weights.y), mix(gl_in[2].gl_Position, gl_in[3].gl_Position, weights.y), weights.x);\n"
			<< "    " << colorOutputName << " = mix(mix(v_patch_color[0], v_patch_color[1], cweights.y), mix(v_patch_color[2], v_patch_color[3], cweights.y), cweights.x);\n";
	else
		DE_ASSERT(false);

	src <<	"}\n";

	return src.str();
}

class IdentityGeometryShaderTestCase : public TestCase
{
public:
	void			initPrograms	(vk::SourceCollections& programCollection) const;
	TestInstance*	createInstance	(Context& context) const;

	IdentityGeometryShaderTestCase (tcu::TestContext& testCtx, const std::string& name, const std::string& description, const TessPrimitiveType primitiveType)
		: TestCase			(testCtx, name, description)
		, m_primitiveType	(primitiveType)
	{
	}

private:
	const TessPrimitiveType m_primitiveType;
};

void IdentityGeometryShaderTestCase::initPrograms (vk::SourceCollections& programCollection) const
{
	addVertexAndFragmentShaders(programCollection);

	// Tessellation control
	{
		std::ostringstream src;
		src <<	glu::getGLSLVersionDeclaration(glu::GLSL_VERSION_310_ES) << "\n"
			<< "#extension GL_EXT_tessellation_shader : require\n"
			<< "layout(vertices = 4) out;\n"
			<< "\n"
			<< "layout(set = 0, binding = 0, std430) readonly restrict buffer TessLevels {\n"
			<< "    float inner0;\n"
			<< "    float inner1;\n"
			<< "    float outer0;\n"
			<< "    float outer1;\n"
			<< "    float outer2;\n"
			<< "    float outer3;\n"
			<< "} sb_levels;\n"
			<< "\n"
			<< "layout(location = 0) in  highp vec4 v_vertex_color[];\n"
			<< "layout(location = 0) out highp vec4 v_patch_color[];\n"
			<< "\n"
			<< "void main (void)\n"
			<< "{\n"
			<< "    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;\n"
			<< "    v_patch_color[gl_InvocationID] = v_vertex_color[gl_InvocationID];\n"
			<< "\n"
			<< "    gl_TessLevelInner[0] = sb_levels.inner0;\n"
			<< "    gl_TessLevelInner[1] = sb_levels.inner1;\n"
			<< "    gl_TessLevelOuter[0] = sb_levels.outer0;\n"
			<< "    gl_TessLevelOuter[1] = sb_levels.outer1;\n"
			<< "    gl_TessLevelOuter[2] = sb_levels.outer2;\n"
			<< "    gl_TessLevelOuter[3] = sb_levels.outer3;\n"
			<<	"}\n";

		programCollection.glslSources.add("tesc") << glu::TessellationControlSource(src.str());
	}

	// Tessellation evaluation shader
	{
		programCollection.glslSources.add("tese_to_frag")
			<< glu::TessellationEvaluationSource(generateTessellationEvaluationShader(m_primitiveType, "v_fragment_color"));
		programCollection.glslSources.add("tese_to_geom")
			<< glu::TessellationEvaluationSource(generateTessellationEvaluationShader(m_primitiveType, "v_evaluated_color"));
	}

	// Geometry shader
	{
		std::ostringstream	src;
		src << glu::getGLSLVersionDeclaration(glu::GLSL_VERSION_310_ES) << "\n"
			<< "#extension GL_EXT_geometry_shader : require\n"
			<< "layout(" << getGeometryShaderInputPrimitiveTypeShaderName(m_primitiveType, false) << ") in;\n"
			<< "layout(" << getGeometryShaderOutputPrimitiveTypeShaderName(m_primitiveType, false)
						 << ", max_vertices=" << numVerticesPerPrimitive(m_primitiveType, false) << ") out;\n"
			<< "\n"
			<< "layout(location = 0) in  highp vec4 v_evaluated_color[];\n"
			<< "layout(location = 0) out highp vec4 v_fragment_color;\n"
			<< "\n"
			<< "void main (void)\n"
			<< "{\n"
			<< "    for (int ndx = 0; ndx < gl_in.length(); ++ndx)\n"
			<< "    {\n"
			<< "        gl_Position = gl_in[ndx].gl_Position;\n"
			<< "        v_fragment_color = v_evaluated_color[ndx];\n"
			<< "        EmitVertex();\n"
			<< "    }\n"
			<< "}\n";

		programCollection.glslSources.add("geom") << glu::GeometrySource(src.str());
	}
}

class IdentityTessellationShaderTestCase : public TestCase
{
public:
	void			initPrograms	(vk::SourceCollections& programCollection) const;
	TestInstance*	createInstance	(Context& context) const;

	IdentityTessellationShaderTestCase (tcu::TestContext& testCtx, const std::string& name, const std::string& description, const TessPrimitiveType primitiveType)
		: TestCase			(testCtx, name, description)
		, m_primitiveType	(primitiveType)
	{
	}

private:
	const TessPrimitiveType m_primitiveType;
};

//! Geometry shader used in passthrough tessellation shader case.
std::string generateGeometryShader (const TessPrimitiveType primitiveType, const std::string& colorSourceName)
{
	const int numEmitVertices = (primitiveType == TESSPRIMITIVETYPE_ISOLINES ? 11 : 8);

	std::ostringstream src;
	src << glu::getGLSLVersionDeclaration(glu::GLSL_VERSION_310_ES) << "\n"
		<< "#extension GL_EXT_geometry_shader : require\n"
		<< "layout(" << getGeometryShaderInputPrimitiveTypeShaderName(primitiveType, false) << ") in;\n"
		<< "layout(" << getGeometryShaderOutputPrimitiveTypeShaderName(primitiveType, false)
					  << ", max_vertices=" << numEmitVertices << ") out;\n"
		<< "\n"
		<< "layout(location = 0) in  highp vec4 " << colorSourceName << "[];\n"
		<< "layout(location = 0) out highp vec4 v_fragment_color;\n"
		<< "\n"
		<< "void main (void)\n"
		<< "{\n";

	if (primitiveType == TESSPRIMITIVETYPE_TRIANGLES)
	{
		src << "	vec4 centerPos = (gl_in[0].gl_Position + gl_in[1].gl_Position + gl_in[2].gl_Position) / 3.0f;\n"
			<< "\n"
			<< "	for (int ndx = 0; ndx < 4; ++ndx)\n"
			<< "	{\n"
			<< "		gl_Position = centerPos + (centerPos - gl_in[ndx % 3].gl_Position);\n"
			<< "		v_fragment_color = " << colorSourceName << "[ndx % 3];\n"
			<< "		EmitVertex();\n"
			<< "\n"
			<< "		gl_Position = centerPos + 0.7 * (centerPos - gl_in[ndx % 3].gl_Position);\n"
			<< "		v_fragment_color = " << colorSourceName << "[ndx % 3];\n"
			<< "		EmitVertex();\n"
			<< "	}\n";
	}
	else if (primitiveType == TESSPRIMITIVETYPE_ISOLINES)
	{
		src << "	vec4 mdir = vec4(gl_in[0].gl_Position.y - gl_in[1].gl_Position.y, gl_in[1].gl_Position.x - gl_in[0].gl_Position.x, 0.0, 0.0);\n"
			<< "	for (int i = 0; i <= 10; ++i)\n"
			<< "	{\n"
			<< "		float xweight = cos(float(i) / 10.0 * 6.28) * 0.5 + 0.5;\n"
			<< "		float mweight = sin(float(i) / 10.0 * 6.28) * 0.1 + 0.1;\n"
			<< "		gl_Position = mix(gl_in[0].gl_Position, gl_in[1].gl_Position, xweight) + mweight * mdir;\n"
			<< "		v_fragment_color = mix(" << colorSourceName << "[0], " << colorSourceName << "[1], xweight);\n"
			<< "		EmitVertex();\n"
			<< "	}\n";
	}
	else
		DE_ASSERT(false);

	src << "}\n";

	return src.str();
}

void IdentityTessellationShaderTestCase::initPrograms (vk::SourceCollections& programCollection) const
{
	addVertexAndFragmentShaders(programCollection);

	// Tessellation control
	{
		std::ostringstream src;
		src << glu::getGLSLVersionDeclaration(glu::GLSL_VERSION_310_ES) << "\n"
			<< "#extension GL_EXT_tessellation_shader : require\n"
			<< "layout(vertices = " << numVerticesPerPrimitive(m_primitiveType, false) << ") out;\n"
			<< "\n"
			<< "layout(location = 0) in  highp vec4 v_vertex_color[];\n"
			<< "layout(location = 0) out highp vec4 v_control_color[];\n"
			<< "\n"
			<< "void main (void)\n"
			<< "{\n"
			<< "    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;\n"
			<< "    v_control_color[gl_InvocationID] = v_vertex_color[gl_InvocationID];\n"
			<< "\n"
			<< "    gl_TessLevelInner[0] = 1.0;\n"
			<< "    gl_TessLevelInner[1] = 1.0;\n"
			<< "    gl_TessLevelOuter[0] = 1.0;\n"
			<< "    gl_TessLevelOuter[1] = 1.0;\n"
			<< "    gl_TessLevelOuter[2] = 1.0;\n"
			<< "    gl_TessLevelOuter[3] = 1.0;\n"
			<<	"}\n";

		programCollection.glslSources.add("tesc") << glu::TessellationControlSource(src.str());
	}

	// Tessellation evaluation shader
	{
		std::ostringstream src;
		src << glu::getGLSLVersionDeclaration(glu::GLSL_VERSION_310_ES) << "\n"
			<< "#extension GL_EXT_tessellation_shader : require\n"
			<< "layout(" << getTessPrimitiveTypeShaderName(m_primitiveType) << ") in;\n"
			<< "\n"
			<< "layout(location = 0) in  highp vec4 v_control_color[];\n"
			<< "layout(location = 0) out highp vec4 v_evaluated_color;\n"
			<< "\n"
			<< "// note: No need to use precise gl_Position since we do not require gapless geometry\n"
			<< "void main (void)\n"
			<< "{\n";

		if (m_primitiveType == TESSPRIMITIVETYPE_TRIANGLES)
			src << "    gl_Position = gl_TessCoord.x * gl_in[0].gl_Position + gl_TessCoord.y * gl_in[1].gl_Position + gl_TessCoord.z * gl_in[2].gl_Position;\n"
				<< "    v_evaluated_color = gl_TessCoord.x * v_control_color[0] + gl_TessCoord.y * v_control_color[1] + gl_TessCoord.z * v_control_color[2];\n";
		else if (m_primitiveType == TESSPRIMITIVETYPE_ISOLINES)
			src << "    gl_Position = mix(gl_in[0].gl_Position, gl_in[1].gl_Position, gl_TessCoord.x);\n"
				<< "    v_evaluated_color = mix(v_control_color[0], v_control_color[1], gl_TessCoord.x);\n";
		else
			DE_ASSERT(false);

		src << "}\n";

		programCollection.glslSources.add("tese") << glu::TessellationEvaluationSource(src.str());
	}

	// Geometry shader
	{
		programCollection.glslSources.add("geom_from_tese") << glu::GeometrySource(
			generateGeometryShader(m_primitiveType, "v_evaluated_color"));
		programCollection.glslSources.add("geom_from_vert") << glu::GeometrySource(
			generateGeometryShader(m_primitiveType, "v_vertex_color"));
	}
}

inline tcu::ConstPixelBufferAccess getPixelBufferAccess (const DeviceInterface& vk,
														 const VkDevice			device,
														 const Buffer&			colorBuffer,
														 const VkFormat			colorFormat,
														 const VkDeviceSize		colorBufferSizeBytes,
														 const tcu::IVec2&		renderSize)
{
	const Allocation& alloc = colorBuffer.getAllocation();
	invalidateMappedMemoryRange(vk, device, alloc.getMemory(), alloc.getOffset(), colorBufferSizeBytes);
	return tcu::ConstPixelBufferAccess(mapVkFormat(colorFormat), renderSize.x(), renderSize.y(), 1, alloc.getHostPtr());
}

//! When a test case disables tessellation stage and we need to derive a primitive type.
VkPrimitiveTopology getPrimitiveTopology (const TessPrimitiveType primitiveType)
{
	switch (primitiveType)
	{
		case TESSPRIMITIVETYPE_TRIANGLES:
		case TESSPRIMITIVETYPE_QUADS:
			return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

		case TESSPRIMITIVETYPE_ISOLINES:
			return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;

		default:
			DE_ASSERT(false);
			return VK_PRIMITIVE_TOPOLOGY_LAST;
	}
}

enum Constants
{
	PIPELINE_CASES	= 2,
	RENDER_SIZE		= 256,
};

class PassthroughTestInstance : public TestInstance
{
public:
	struct PipelineDescription
	{
		bool		useTessellation;
		bool		useGeometry;
		std::string	tessEvalShaderName;
		std::string	geomShaderName;
		std::string description;

		PipelineDescription (void) : useTessellation(), useGeometry() {}
	};

	struct Params
	{
		bool					useTessLevels;
		TessLevels				tessLevels;
		TessPrimitiveType		primitiveType;
		int						inputPatchVertices;
		std::vector<tcu::Vec4>	vertices;
		PipelineDescription		pipelineCases[PIPELINE_CASES];	//!< Each test case renders with two pipelines and compares results
		std::string				message;

		Params (void) : useTessLevels(), tessLevels(), primitiveType(), inputPatchVertices() {}
	};

								PassthroughTestInstance	(Context& context, const Params& params) : TestInstance(context), m_params(params) {}
	tcu::TestStatus				iterate					(void);

private:
	const Params				m_params;
};

tcu::TestStatus PassthroughTestInstance::iterate (void)
{
	requireFeatures(m_context.getInstanceInterface(), m_context.getPhysicalDevice(), FEATURE_TESSELLATION_SHADER | FEATURE_GEOMETRY_SHADER);
	DE_STATIC_ASSERT(PIPELINE_CASES == 2);

	const DeviceInterface&	vk					= m_context.getDeviceInterface();
	const VkDevice			device				= m_context.getDevice();
	const VkQueue			queue				= m_context.getUniversalQueue();
	const deUint32			queueFamilyIndex	= m_context.getUniversalQueueFamilyIndex();
	Allocator&				allocator			= m_context.getDefaultAllocator();

	// Tessellation levels
	const Buffer tessLevelsBuffer (vk, device, allocator, makeBufferCreateInfo(sizeof(TessLevels), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT), MemoryRequirement::HostVisible);

	if (m_params.useTessLevels)
	{
		const Allocation& alloc = tessLevelsBuffer.getAllocation();
		TessLevels* const bufferTessLevels = static_cast<TessLevels*>(alloc.getHostPtr());
		*bufferTessLevels = m_params.tessLevels;
		flushMappedMemoryRange(vk, device, alloc.getMemory(), alloc.getOffset(), sizeof(TessLevels));
	}

	// Vertex attributes

	const VkDeviceSize	vertexDataSizeBytes = sizeInBytes(m_params.vertices);
	const VkFormat		vertexFormat		= VK_FORMAT_R32G32B32A32_SFLOAT;
	const Buffer		vertexBuffer		(vk, device, allocator, makeBufferCreateInfo(vertexDataSizeBytes, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT), MemoryRequirement::HostVisible);

	{
		const Allocation& alloc = vertexBuffer.getAllocation();
		deMemcpy(alloc.getHostPtr(), &m_params.vertices[0], static_cast<std::size_t>(vertexDataSizeBytes));
		flushMappedMemoryRange(vk, device, alloc.getMemory(), alloc.getOffset(), vertexDataSizeBytes);
	}

	// Descriptors - make descriptor for tessellation levels, even if we don't use them, to simplify code

	const Unique<VkDescriptorSetLayout> descriptorSetLayout(DescriptorSetLayoutBuilder()
		.addSingleBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT)
		.build(vk, device));

	const Unique<VkDescriptorPool> descriptorPool(DescriptorPoolBuilder()
		.addType(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
		.build(vk, device, VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT, 1u));

	const Unique<VkDescriptorSet> descriptorSet		   (makeDescriptorSet(vk, device, *descriptorPool, *descriptorSetLayout));
	const VkDescriptorBufferInfo  tessLevelsBufferInfo = makeDescriptorBufferInfo(*tessLevelsBuffer, 0ull, sizeof(TessLevels));

	DescriptorSetUpdateBuilder()
		.writeSingle(*descriptorSet, DescriptorSetUpdateBuilder::Location::binding(0u), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, &tessLevelsBufferInfo)
		.update(vk, device);

	// Color attachment

	const tcu::IVec2			  renderSize				 = tcu::IVec2(RENDER_SIZE, RENDER_SIZE);
	const VkFormat				  colorFormat				 = VK_FORMAT_R8G8B8A8_UNORM;
	const VkImageSubresourceRange colorImageSubresourceRange = makeImageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT, 0u, 1u, 0u, 1u);
	const Image					  colorAttachmentImage		 (vk, device, allocator,
															 makeImageCreateInfo(renderSize, colorFormat, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, 1u),
															 MemoryRequirement::Any);

	// Color output buffer: image will be copied here for verification.
	//                      We use two buffers, one for each case.

	const VkDeviceSize	colorBufferSizeBytes		= renderSize.x()*renderSize.y() * tcu::getPixelSize(mapVkFormat(colorFormat));
	const Buffer		colorBuffer1				(vk, device, allocator, makeBufferCreateInfo(colorBufferSizeBytes, VK_BUFFER_USAGE_TRANSFER_DST_BIT), MemoryRequirement::HostVisible);
	const Buffer		colorBuffer2				(vk, device, allocator, makeBufferCreateInfo(colorBufferSizeBytes, VK_BUFFER_USAGE_TRANSFER_DST_BIT), MemoryRequirement::HostVisible);
	const Buffer* const colorBuffer[PIPELINE_CASES] = { &colorBuffer1, &colorBuffer2 };

	// Pipeline

	const Unique<VkImageView>		colorAttachmentView(makeImageView(vk, device, *colorAttachmentImage, VK_IMAGE_VIEW_TYPE_2D, colorFormat, colorImageSubresourceRange));
	const Unique<VkRenderPass>		renderPass		   (makeRenderPass(vk, device, colorFormat));
	const Unique<VkFramebuffer>		framebuffer		   (makeFramebuffer(vk, device, *renderPass, *colorAttachmentView, renderSize.x(), renderSize.y(), 1u));
	const Unique<VkPipelineLayout>	pipelineLayout	   (makePipelineLayout(vk, device, *descriptorSetLayout));
	const Unique<VkCommandPool>		cmdPool			   (makeCommandPool(vk, device, queueFamilyIndex));
	const Unique<VkCommandBuffer>	cmdBuffer		   (allocateCommandBuffer(vk, device, *cmdPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY));

	// Message explaining the test
	{
		tcu::TestLog& log = m_context.getTestContext().getLog();
		log << tcu::TestLog::Message << m_params.message << tcu::TestLog::EndMessage;

		if (m_params.useTessLevels)
			log << tcu::TestLog::Message << "Tessellation levels: " << getTessellationLevelsString(m_params.tessLevels, m_params.primitiveType) << tcu::TestLog::EndMessage;
	}

	for (int pipelineNdx = 0; pipelineNdx < PIPELINE_CASES; ++pipelineNdx)
	{
		const PipelineDescription& pipelineDescription = m_params.pipelineCases[pipelineNdx];
		GraphicsPipelineBuilder	   pipelineBuilder;

		pipelineBuilder
			.setPrimitiveTopology		  (getPrimitiveTopology(m_params.primitiveType))
			.setRenderSize				  (renderSize)
			.setBlend					  (true)
			.setVertexInputSingleAttribute(vertexFormat, tcu::getPixelSize(mapVkFormat(vertexFormat)))
			.setPatchControlPoints		  (m_params.inputPatchVertices)
			.setShader					  (vk, device, VK_SHADER_STAGE_VERTEX_BIT,					m_context.getBinaryCollection().get("vert"), DE_NULL)
			.setShader					  (vk, device, VK_SHADER_STAGE_FRAGMENT_BIT,				m_context.getBinaryCollection().get("frag"), DE_NULL);

		if (pipelineDescription.useTessellation)
			pipelineBuilder
				.setShader				  (vk, device, VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT,	m_context.getBinaryCollection().get("tesc"), DE_NULL)
				.setShader				  (vk, device, VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT, m_context.getBinaryCollection().get(pipelineDescription.tessEvalShaderName), DE_NULL);

		if (pipelineDescription.useGeometry)
			pipelineBuilder
				.setShader				  (vk, device, VK_SHADER_STAGE_GEOMETRY_BIT,				m_context.getBinaryCollection().get(pipelineDescription.geomShaderName), DE_NULL);

		const Unique<VkPipeline> pipeline (pipelineBuilder.build(vk, device, *pipelineLayout, *renderPass));

		// Draw commands

		beginCommandBuffer(vk, *cmdBuffer);

		// Change color attachment image layout
		{
			// State is slightly different on the first iteration.
			const VkImageLayout currentLayout = (pipelineNdx == 0 ? VK_IMAGE_LAYOUT_UNDEFINED : VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
			const VkAccessFlags srcFlags	  = (pipelineNdx == 0 ? (VkAccessFlags)0          : (VkAccessFlags)VK_ACCESS_TRANSFER_READ_BIT);

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
		{
			const VkDeviceSize vertexBufferOffset = 0ull;
			vk.cmdBindVertexBuffers(*cmdBuffer, 0u, 1u, &vertexBuffer.get(), &vertexBufferOffset);
		}

		if (m_params.useTessLevels)
			vk.cmdBindDescriptorSets(*cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *pipelineLayout, 0u, 1u, &descriptorSet.get(), 0u, DE_NULL);

		vk.cmdDraw(*cmdBuffer, static_cast<deUint32>(m_params.vertices.size()), 1u, 0u, 0u);
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
			vk.cmdCopyImageToBuffer(*cmdBuffer, *colorAttachmentImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, colorBuffer[pipelineNdx]->get(), 1u, &copyRegion);
		}
		{
			const VkBufferMemoryBarrier postCopyBarrier = makeBufferMemoryBarrier(
				VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_HOST_READ_BIT, colorBuffer[pipelineNdx]->get(), 0ull, colorBufferSizeBytes);

			vk.cmdPipelineBarrier(*cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_HOST_BIT, 0u,
				0u, DE_NULL, 1u, &postCopyBarrier, 0u, DE_NULL);
		}

		endCommandBuffer(vk, *cmdBuffer);
		submitCommandsAndWait(vk, device, queue, *cmdBuffer);
	}

	// Verify results

	tcu::ConstPixelBufferAccess image0 = getPixelBufferAccess(vk, device, *colorBuffer[0], colorFormat, colorBufferSizeBytes, renderSize);
	tcu::ConstPixelBufferAccess image1 = getPixelBufferAccess(vk, device, *colorBuffer[1], colorFormat, colorBufferSizeBytes, renderSize);

	const tcu::UVec4 colorThreshold    (8, 8, 8, 255);
	const tcu::IVec3 positionDeviation (1, 1, 0);		// 3x3 search kernel
	const bool		 ignoreOutOfBounds = true;

	tcu::TestLog& log = m_context.getTestContext().getLog();
	log << tcu::TestLog::Message
		<< "In image comparison:\n"
		<< "  Reference - " << m_params.pipelineCases[0].description << "\n"
		<< "  Result    - " << m_params.pipelineCases[1].description << "\n"
		<< tcu::TestLog::EndMessage;

	const bool ok = tcu::intThresholdPositionDeviationCompare(
		log, "ImageCompare", "Image comparison", image0, image1, colorThreshold, positionDeviation, ignoreOutOfBounds, tcu::COMPARE_LOG_RESULT);

	return (ok ? tcu::TestStatus::pass("OK") : tcu::TestStatus::fail("Image comparison failed"));
}

TestInstance* IdentityGeometryShaderTestCase::createInstance (Context& context) const
{
	PassthroughTestInstance::Params params;

	const float level		   = 14.0;
	params.useTessLevels	   = true;
	params.tessLevels.inner[0] = level;
	params.tessLevels.inner[1] = level;
	params.tessLevels.outer[0] = level;
	params.tessLevels.outer[1] = level;
	params.tessLevels.outer[2] = level;
	params.tessLevels.outer[3] = level;

	params.primitiveType	   = m_primitiveType;
	params.inputPatchVertices  = (m_primitiveType == TESSPRIMITIVETYPE_TRIANGLES ? 3 : 4);

	params.vertices.push_back(tcu::Vec4( -0.9f, -0.9f, 0.0f, 1.0f ));
	params.vertices.push_back(tcu::Vec4( -0.9f,  0.9f, 0.0f, 1.0f ));
	params.vertices.push_back(tcu::Vec4(  0.9f, -0.9f, 0.0f, 1.0f ));
	params.vertices.push_back(tcu::Vec4(  0.9f,  0.9f, 0.0f, 1.0f ));

	params.pipelineCases[0].useTessellation		= true;
	params.pipelineCases[0].useGeometry			= true;
	params.pipelineCases[0].tessEvalShaderName	= "tese_to_geom";
	params.pipelineCases[0].geomShaderName		= "geom";
	params.pipelineCases[0].description			= "passthrough geometry shader";

	params.pipelineCases[1].useTessellation		= true;
	params.pipelineCases[1].useGeometry			= false;
	params.pipelineCases[1].tessEvalShaderName	= "tese_to_frag";
	params.pipelineCases[1].geomShaderName		= "geom";
	params.pipelineCases[1].description			= "no geometry shader in the pipeline";

	params.message = "Testing tessellating shader program output does not change when a passthrough geometry shader is attached.\n"
					 "Rendering two images, first with and second without a geometry shader. Expecting similar results.\n"
					 "Using additive blending to detect overlap.\n";

	return new PassthroughTestInstance(context, params);
};

TestInstance* IdentityTessellationShaderTestCase::createInstance (Context& context) const
{
	PassthroughTestInstance::Params params;

	params.useTessLevels	   = false;
	params.primitiveType	   = m_primitiveType;
	params.inputPatchVertices  = (m_primitiveType == TESSPRIMITIVETYPE_TRIANGLES ? 3 : 2);

	params.vertices.push_back(	  tcu::Vec4( -0.4f,  0.4f, 0.0f, 1.0f ));
	params.vertices.push_back(	  tcu::Vec4(  0.0f, -0.5f, 0.0f, 1.0f ));
	if (params.inputPatchVertices == 3)
		params.vertices.push_back(tcu::Vec4(  0.4f,  0.4f, 0.0f, 1.0f ));

	params.pipelineCases[0].useTessellation		= true;
	params.pipelineCases[0].useGeometry			= true;
	params.pipelineCases[0].tessEvalShaderName	= "tese";
	params.pipelineCases[0].geomShaderName		= "geom_from_tese";
	params.pipelineCases[0].description			= "passthrough tessellation shaders";

	params.pipelineCases[1].useTessellation		= false;
	params.pipelineCases[1].useGeometry			= true;
	params.pipelineCases[1].tessEvalShaderName	= "tese";
	params.pipelineCases[1].geomShaderName		= "geom_from_vert";
	params.pipelineCases[1].description			= "no tessellation shaders in the pipeline";

	params.message = "Testing geometry shading shader program output does not change when a passthrough tessellation shader is attached.\n"
					 "Rendering two images, first with and second without a tessellation shader. Expecting similar results.\n"
					 "Using additive blending to detect overlap.\n";

	return new PassthroughTestInstance(context, params);
};

inline TestCase* makeIdentityGeometryShaderCase (tcu::TestContext& testCtx, const TessPrimitiveType primitiveType)
{
	return new IdentityGeometryShaderTestCase(
		testCtx,
		"tessellate_" + de::toString(getTessPrimitiveTypeShaderName(primitiveType)) + "_passthrough_geometry_no_change",
		"Passthrough geometry shader has no effect",
		primitiveType);
}

inline TestCase* makeIdentityTessellationShaderCase (tcu::TestContext& testCtx, const TessPrimitiveType primitiveType)
{
	return new IdentityTessellationShaderTestCase(
		testCtx,
		"passthrough_tessellation_geometry_shade_" + de::toString(getTessPrimitiveTypeShaderName(primitiveType)) + "_no_change",
		"Passthrough tessellation shader has no effect",
		primitiveType);
}

} // anonymous


//! Ported from dEQP-GLES31.functional.tessellation_geometry_interaction.render.passthrough.*
tcu::TestCaseGroup* createGeometryPassthroughTests (tcu::TestContext& testCtx)
{
	de::MovePtr<tcu::TestCaseGroup> group (new tcu::TestCaseGroup(testCtx, "passthrough", "Render various types with either passthrough geometry or tessellation shader"));

	// Passthrough geometry shader
	group->addChild(makeIdentityGeometryShaderCase(testCtx, TESSPRIMITIVETYPE_TRIANGLES));
	group->addChild(makeIdentityGeometryShaderCase(testCtx, TESSPRIMITIVETYPE_QUADS));
	group->addChild(makeIdentityGeometryShaderCase(testCtx, TESSPRIMITIVETYPE_ISOLINES));

	// Passthrough tessellation shader
	group->addChild(makeIdentityTessellationShaderCase(testCtx, TESSPRIMITIVETYPE_TRIANGLES));
	group->addChild(makeIdentityTessellationShaderCase(testCtx, TESSPRIMITIVETYPE_ISOLINES));

	return group.release();
}

} // tessellation
} // vkt
