/*-------------------------------------------------------------------------
 * OpenGL Conformance Test Suite
 * -----------------------------
 *
 * Copyright (c) 2014-2016 The Khronos Group Inc.
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
 */ /*!
 * \file
 * \brief
 */ /*-------------------------------------------------------------------*/

/*!
 * \file esextcGPUShader5TextureGatherOffset.cpp
 * \brief gpu_shader5 extension - texture gather offset tests (Test 9 and 10)
 */ /*-------------------------------------------------------------------*/

#include "esextcGPUShader5TextureGatherOffset.hpp"

#include "gluContextInfo.hpp"
#include "glwEnums.hpp"
#include "glwFunctions.hpp"
#include "tcuTestLog.hpp"

#include <cstdlib>
#include <cstring>
#include <sstream>

namespace glcts
{

/* Fragment Shader for GPUShader5TextureGatherOffsetTestBase */
const glw::GLchar* const GPUShader5TextureGatherOffsetTestBase::m_fragment_shader_code =
	"${VERSION}\n"
	"\n"
	"${GPU_SHADER5_REQUIRE}\n"
	"\n"
	"precision highp float;\n"
	"\n"
	"layout(location = 0) out vec4 fs_out_color;\n"
	"\n"
	"void main()\n"
	"{\n"
	"    fs_out_color = vec4(1, 1, 1, 1);\n"
	"}\n";

/* Vertex Shader for GPUShader5TextureGatherOffsetColor2DRepeatCaseTest */
const glw::GLchar* const GPUShader5TextureGatherOffsetColor2DRepeatCaseTest::m_vertex_shader_code =
	"${VERSION}\n"
	"\n"
	"${GPU_SHADER5_REQUIRE}\n"
	"\n"
	"precision highp float;\n"
	"precision highp isampler2D;\n"
	"\n"
	"uniform isampler2D sampler;\n"
	"\n"
	"in ivec2 offsets;\n"
	"in vec2  texCoords;\n"
	"\n"
	"flat out ivec4 without_offset_0;\n"
	"flat out ivec4 without_offset_1;\n"
	"flat out ivec4 without_offset_2;\n"
	"flat out ivec4 without_offset_3;\n"
	"\n"
	"flat out ivec4 with_offset_0;\n"
	"flat out ivec4 with_offset_1;\n"
	"flat out ivec4 with_offset_2;\n"
	"flat out ivec4 with_offset_3;\n"
	"\n"
	"void main()\n"
	"{\n"
	"    without_offset_0 = textureGather      (sampler, texCoords, 0);\n"
	"    without_offset_1 = textureGather      (sampler, texCoords, 1);\n"
	"    without_offset_2 = textureGather      (sampler, texCoords, 2);\n"
	"    without_offset_3 = textureGather      (sampler, texCoords, 3);\n"
	"\n"
	"    with_offset_0    = textureGatherOffset(sampler, texCoords, offsets, 0);\n"
	"    with_offset_1    = textureGatherOffset(sampler, texCoords, offsets, 1);\n"
	"    with_offset_2    = textureGatherOffset(sampler, texCoords, offsets, 2);\n"
	"    with_offset_3    = textureGatherOffset(sampler, texCoords, offsets, 3);\n"
	"}\n";

/* Vertex Shader for GPUShader5TextureGatherOffsetColor2DArrayCaseTest */
const glw::GLchar* const GPUShader5TextureGatherOffsetColor2DArrayCaseTest::m_vertex_shader_code =
	"${VERSION}\n"
	"\n"
	"${GPU_SHADER5_REQUIRE}\n"
	"\n"
	"precision highp float;\n"
	"precision highp isampler2DArray;\n"
	"\n"
	"uniform isampler2DArray sampler;\n"
	"\n"
	"in ivec2 offsets;\n"
	"in vec3  texCoords;\n"
	"\n"
	"flat out ivec4 without_offset_0;\n"
	"flat out ivec4 without_offset_1;\n"
	"flat out ivec4 without_offset_2;\n"
	"flat out ivec4 without_offset_3;\n"
	"\n"
	"flat out ivec4 with_offset_0;\n"
	"flat out ivec4 with_offset_1;\n"
	"flat out ivec4 with_offset_2;\n"
	"flat out ivec4 with_offset_3;\n"
	"\n"
	"void main()\n"
	"{\n"
	"    without_offset_0 = textureGather      (sampler, texCoords, 0);\n"
	"    without_offset_1 = textureGather      (sampler, texCoords, 1);\n"
	"    without_offset_2 = textureGather      (sampler, texCoords, 2);\n"
	"    without_offset_3 = textureGather      (sampler, texCoords, 3);\n"
	"\n"
	"    with_offset_0    = textureGatherOffset(sampler, texCoords, offsets, 0);\n"
	"    with_offset_1    = textureGatherOffset(sampler, texCoords, offsets, 1);\n"
	"    with_offset_2    = textureGatherOffset(sampler, texCoords, offsets, 2);\n"
	"    with_offset_3    = textureGatherOffset(sampler, texCoords, offsets, 3);\n"
	"}\n";

/* Vertex Shader for GPUShader5TextureGatherOffsetColor2DClampToBorderCaseTest */
const glw::GLchar* const GPUShader5TextureGatherOffsetColor2DClampToBorderCaseTest::m_vertex_shader_code =
	"${VERSION}\n"
	"\n"
	"${GPU_SHADER5_REQUIRE}\n"
	"\n"
	"precision highp float;\n"
	"precision highp isampler2D;\n"
	"\n"
	"uniform isampler2D sampler;\n"
	"\n"
	"in ivec2 offsets;\n"
	"in vec2  texCoords;\n"
	"\n"
	"flat out ivec4 without_offset_0;\n"
	"flat out ivec4 without_offset_1;\n"
	"flat out ivec4 without_offset_2;\n"
	"flat out ivec4 without_offset_3;\n"
	"\n"
	"flat out ivec4 with_offset_0;\n"
	"flat out ivec4 with_offset_1;\n"
	"flat out ivec4 with_offset_2;\n"
	"flat out ivec4 with_offset_3;\n"
	"\n"
	"void main()\n"
	"{\n"
	"    vec2 floorTexCoords = floor(texCoords);\n"
	"    vec2 fractTexCoords = texCoords - floorTexCoords;\n"
	"\n"
	"    without_offset_0 = textureGather      (sampler, fractTexCoords, 0);\n"
	"    without_offset_1 = textureGather      (sampler, fractTexCoords, 1);\n"
	"\n"
	"    without_offset_2 = ivec4(int(floorTexCoords.x));\n"
	"    without_offset_3 = ivec4(int(floorTexCoords.y));\n"
	"\n"
	"    with_offset_0    = textureGatherOffset(sampler, texCoords, offsets, 0);\n"
	"    with_offset_1    = textureGatherOffset(sampler, texCoords, offsets, 1);\n"
	"    with_offset_2    = textureGatherOffset(sampler, texCoords, offsets, 2);\n"
	"    with_offset_3    = textureGatherOffset(sampler, texCoords, offsets, 3);\n"
	"}\n";

/* Vertex Shader for GPUShader5TextureGatherOffsetColor2DClampToEdgeCaseTest */
const glw::GLchar* const GPUShader5TextureGatherOffsetColor2DClampToEdgeCaseTest::m_vertex_shader_code =
	"${VERSION}\n"
	"\n"
	"${GPU_SHADER5_REQUIRE}\n"
	"\n"
	"precision highp float;\n"
	"precision highp isampler2D;\n"
	"\n"
	"uniform isampler2D sampler;\n"
	"uniform isampler2D reference_sampler;\n"
	"\n"
	"in ivec2 offsets;\n"
	"in vec2  texCoords;\n"
	"\n"
	"flat out ivec4 without_offset_0;\n"
	"flat out ivec4 without_offset_1;\n"
	"flat out ivec4 without_offset_2;\n"
	"flat out ivec4 without_offset_3;\n"
	"\n"
	"flat out ivec4 with_offset_0;\n"
	"flat out ivec4 with_offset_1;\n"
	"flat out ivec4 with_offset_2;\n"
	"flat out ivec4 with_offset_3;\n"
	"\n"
	"void main()\n"
	"{\n"
	"    vec2 floorTexCoords = floor(texCoords);\n"
	"    vec2 fractTexCoords = texCoords - floorTexCoords;\n"
	"\n"
	"    without_offset_0 = textureGather      (reference_sampler, fractTexCoords, 0);\n"
	"    without_offset_1 = textureGather      (reference_sampler, fractTexCoords, 1);\n"
	"\n"
	"    without_offset_2 = ivec4(int(floorTexCoords.x));\n"
	"    without_offset_3 = ivec4(int(floorTexCoords.y));\n"
	"\n"
	"    with_offset_0    = textureGatherOffset(sampler, texCoords, offsets, 0);\n"
	"    with_offset_1    = textureGatherOffset(sampler, texCoords, offsets, 1);\n"
	"    with_offset_2    = textureGatherOffset(sampler, texCoords, offsets, 2);\n"
	"    with_offset_3    = textureGatherOffset(sampler, texCoords, offsets, 3);\n"
	"}\n";

/* Vertex Shader for GPUShader5TextureGatherOffsetColor2DOffsetsCaseTest */
const glw::GLchar* const GPUShader5TextureGatherOffsetColor2DOffsetsCaseTest::m_vertex_shader_code_preamble =
	"${VERSION}\n"
	"\n"
	"${GPU_SHADER5_REQUIRE}\n"
	"\n"
	"precision highp float;\n"
	"precision highp isampler2D;\n"
	"\n"
	"uniform isampler2D sampler;\n"
	"\n"
	"in vec2  texCoords;\n"
	"\n"
	"flat out ivec4 without_offset_0;\n"
	"flat out ivec4 without_offset_1;\n"
	"flat out ivec4 without_offset_2;\n"
	"flat out ivec4 without_offset_3;\n"
	"\n"
	"flat out ivec4 with_offset_0;\n"
	"flat out ivec4 with_offset_1;\n"
	"flat out ivec4 with_offset_2;\n"
	"flat out ivec4 with_offset_3;\n"
	"\n"
	"void main()\n"
	"{\n"
	"    const ivec2 offsets[4] = \n";

const glw::GLchar* const GPUShader5TextureGatherOffsetColor2DOffsetsCaseTest::m_vertex_shader_code_body =
	"\n"
	"    without_offset_0 = textureGather       (sampler, texCoords, 0);\n"
	"    without_offset_1 = textureGather       (sampler, texCoords, 1);\n"
	"    without_offset_2 = textureGather       (sampler, texCoords, 2);\n"
	"    without_offset_3 = textureGather       (sampler, texCoords, 3);\n"
	"\n"
	"    with_offset_0    = textureGatherOffsets(sampler, texCoords, offsets, 0);\n"
	"    with_offset_1    = textureGatherOffsets(sampler, texCoords, offsets, 1);\n"
	"    with_offset_2    = textureGatherOffsets(sampler, texCoords, offsets, 2);\n"
	"    with_offset_3    = textureGatherOffsets(sampler, texCoords, offsets, 3);\n"
	"}\n";

/* Vertex Shader for GPUShader5TextureGatherOffsetDepth2DRepeatCaseTest */
const glw::GLchar* const GPUShader5TextureGatherOffsetDepth2DRepeatCaseTest::m_vertex_shader_code =
	"${VERSION}\n"
	"\n"
	"${GPU_SHADER5_REQUIRE}\n"
	"\n"
	"precision highp float;\n"
	"precision highp sampler2DShadow;\n"
	"\n"
	"uniform sampler2DShadow sampler;\n"
	"\n"
	"in ivec2 offsets;\n"
	"in vec2  texCoords;\n"
	"\n"
	"flat out ivec4 with_offset;\n"
	"flat out ivec4 without_offset;\n"
	"\n"
	"void main()\n"
	"{\n"
	"    ivec2 texture_size = textureSize(sampler, 0 /* lod */);\n"
	"    float step         = 1.0f / float(texture_size.x);\n"
	"\n"
	"    without_offset = ivec4(0, 0, 0, 0);\n"
	"    with_offset    = ivec4(0, 0, 0, 0);\n"
	"\n"
	"    for (int x = 0; x < texture_size.x; ++x)\n"
	"    {\n"
	"        float refZ = float(x) * step;\n"
	"\n"
	"        without_offset += ivec4(textureGather      (sampler, texCoords, refZ));\n"
	"        with_offset    += ivec4(textureGatherOffset(sampler, texCoords, refZ, offsets));\n"
	"    }\n"
	"}\n";

/* Vertex Shader for GPUShader5TextureGatherOffsetDepth2DRepeatYCaseTest */
const glw::GLchar* const GPUShader5TextureGatherOffsetDepth2DRepeatYCaseTest::m_vertex_shader_code =
	"${VERSION}\n"
	"\n"
	"${GPU_SHADER5_REQUIRE}\n"
	"\n"
	"precision highp float;\n"
	"precision highp sampler2DShadow;\n"
	"\n"
	"uniform sampler2DShadow sampler;\n"
	"\n"
	"in ivec2 offsets;\n"
	"in vec2  texCoords;\n"
	"\n"
	"flat out ivec4 without_offset;\n"
	"flat out ivec4 with_offset;\n"
	"\n"
	"void main()\n"
	"{\n"
	"    ivec2 texture_size = textureSize(sampler, 0 /* lod */);\n"
	"    float step         = 1.0f / float(texture_size.y);\n"
	"\n"
	"    without_offset = ivec4(0, 0, 0, 0);\n"
	"    with_offset    = ivec4(0, 0, 0, 0);\n"
	"\n"
	"    for (int y = 0; y < texture_size.y; ++y)\n"
	"    {\n"
	"        float refZ = float(y) * step;\n"
	"\n"
	"        without_offset += ivec4(textureGather      (sampler, texCoords, refZ));\n"
	"        with_offset    += ivec4(textureGatherOffset(sampler, texCoords, refZ, offsets));\n"
	"    }\n"
	"}\n";

/* Vertex Shader for GPUShader5TextureGatherOffsetDepth2DArrayCaseTest */
const glw::GLchar* const GPUShader5TextureGatherOffsetDepth2DArrayCaseTest::m_vertex_shader_code =
	"${VERSION}\n"
	"\n"
	"${GPU_SHADER5_REQUIRE}\n"
	"\n"
	"precision highp float;\n"
	"precision highp sampler2DArrayShadow;\n"
	"\n"
	"uniform sampler2DArrayShadow sampler;\n"
	"\n"
	"in ivec2 offsets;\n"
	"in vec3  texCoords;\n"
	"\n"
	"flat out ivec4 without_offset;\n"
	"flat out ivec4 with_offset;\n"
	"\n"
	"void main()\n"
	"{\n"
	"    ivec3 texture_size = textureSize(sampler, 0 /* lod */);\n"
	"    float step         = 1.0f / float(texture_size.x);\n"
	"\n"
	"    without_offset = ivec4(0, 0, 0, 0);\n"
	"    with_offset    = ivec4(0, 0, 0, 0);\n"
	"\n"
	"    for (int x = 0; x < texture_size.x; ++x)\n"
	"    {\n"
	"        float refZ = float(x) * step;\n"
	"\n"
	"        without_offset += ivec4(textureGather      (sampler, texCoords, refZ));\n"
	"        with_offset    += ivec4(textureGatherOffset(sampler, texCoords, refZ, offsets));\n"
	"    }\n"
	"}\n";

/* Vertex Shader for GPUShader5TextureGatherOffsetDepth2DClampToBorderCaseTest */
const glw::GLchar* const GPUShader5TextureGatherOffsetDepth2DClampToBorderCaseTest::m_vertex_shader_code =
	"${VERSION}\n"
	"\n"
	"${GPU_SHADER5_REQUIRE}\n"
	"\n"
	"precision highp float;\n"
	"precision highp sampler2DShadow;\n"
	"\n"
	"uniform sampler2DShadow sampler;\n"
	"\n"
	"in ivec2 offsets;\n"
	"in vec2  texCoords;\n"
	"\n"
	"flat out ivec4 with_offset;\n"
	"flat out ivec4 without_offset;\n"
	"flat out ivec4 floor_tex_coord;\n"
	"\n"
	"void main()\n"
	"{\n"
	"    ivec2 texture_size  = textureSize(sampler, 0 /* lod */);\n"
	"    float step          = 1.0f / float(texture_size.x);\n"
	"\n"
	"    vec2 floorTexCoords = floor(texCoords);\n"
	"    vec2 fractTexCoords = texCoords - floorTexCoords;\n"
	"\n"
	"    floor_tex_coord     = ivec4(ivec2(floorTexCoords.xy), 0, 0);\n"
	"\n"
	"    without_offset      = ivec4(0, 0, 0, 0);\n"
	"    with_offset         = ivec4(0, 0, 0, 0);\n"
	"\n"
	"    for (int x = 0; x < texture_size.x; ++x)\n"
	"    {\n"
	"        float refZ = float(x) * step;\n"
	"\n"
	"        without_offset += ivec4(textureGather      (sampler, fractTexCoords, refZ));\n"
	"        with_offset    += ivec4(textureGatherOffset(sampler, texCoords,      refZ, offsets));\n"
	"    }\n"
	"}\n";

/* Vertex Shader for GPUShader5TextureGatherOffsetDepth2DClampToEdgeCaseTest */
const glw::GLchar* const GPUShader5TextureGatherOffsetDepth2DClampToEdgeCaseTest::m_vertex_shader_code =
	"${VERSION}\n"
	"\n"
	"${GPU_SHADER5_REQUIRE}\n"
	"\n"
	"precision highp float;\n"
	"precision highp sampler2DShadow;\n"
	"\n"
	"uniform sampler2DShadow sampler;\n"
	"uniform sampler2DShadow reference_sampler;\n"
	"\n"
	"in ivec2 offsets;\n"
	"in vec2  texCoords;\n"
	"\n"
	"flat out ivec4 with_offset;\n"
	"flat out ivec4 without_offset;\n"
	"flat out ivec4 floor_tex_coord;\n"
	"\n"
	"void main()\n"
	"{\n"
	"    ivec2 texture_size  = textureSize(sampler, 0 /* lod */);\n"
	"    float step          = 1.0f / float(texture_size.x);\n"
	"\n"
	"    vec2 floorTexCoords = floor(texCoords);\n"
	"    vec2 fractTexCoords = texCoords - floorTexCoords;\n"
	"\n"
	"    floor_tex_coord     = ivec4(ivec2(floorTexCoords.xy), 0, 0);\n"
	"\n"
	"    without_offset      = ivec4(0, 0, 0, 0);\n"
	"    with_offset         = ivec4(0, 0, 0, 0);\n"
	"\n"
	"    for (int x = 0; x < texture_size.x; ++x)\n"
	"    {\n"
	"        float refZ = float(x) * step;\n"
	"\n"
	"        without_offset += ivec4(textureGather      (reference_sampler, fractTexCoords, refZ));\n"
	"        with_offset    += ivec4(textureGatherOffset(sampler,           texCoords,      refZ, offsets));\n"
	"    }\n"
	"}\n";

/* Vertex Shader for GPUShader5TextureGatherOffsetDepth2DOffsetsCaseTest */
const glw::GLchar* const GPUShader5TextureGatherOffsetDepth2DOffsetsCaseTest::m_vertex_shader_code_preamble =
	"${VERSION}\n"
	"\n"
	"${GPU_SHADER5_REQUIRE}\n"
	"\n"
	"precision highp float;\n"
	"precision highp sampler2DShadow;\n"
	"\n"
	"uniform sampler2DShadow sampler;\n"
	"\n"
	"in vec2  texCoords;\n"
	"\n"
	"flat out ivec4 without_offset;\n"
	"flat out ivec4 with_offset;\n"
	"\n"
	"void main()\n"
	"{\n"
	"    const ivec2 offsets[4] =\n";

const glw::GLchar* const GPUShader5TextureGatherOffsetDepth2DOffsetsCaseTest::m_vertex_shader_code_body =
	"\n"
	"    ivec2 texture_size = textureSize(sampler, 0 /* lod */);\n"
	"    float step         = 1.0f / float(texture_size.x);\n"
	"\n"
	"    without_offset = ivec4(0, 0, 0, 0);\n"
	"    with_offset = ivec4(0, 0, 0, 0);\n"
	"\n"
	"    for (int x = 0; x < texture_size.x; ++x)\n"
	"    {\n"
	"        float refZ = float(x) * step;\n"
	"\n"
	"        without_offset += ivec4(textureGather       (sampler, texCoords, refZ));\n"
	"        with_offset    += ivec4(textureGatherOffsets(sampler, texCoords, refZ, offsets));\n"
	"    }\n"
	"}\n";

/* Constants for GPUShader5TextureGatherOffsetTestBase */
const glw::GLchar* const GPUShader5TextureGatherOffsetTestBase::m_coordinates_attribute_name	 = "texCoords";
const int				 GPUShader5TextureGatherOffsetTestBase::m_coordinate_resolution			 = 1024;
const int				 GPUShader5TextureGatherOffsetTestBase::m_max_coordinate_value			 = 8;
const int				 GPUShader5TextureGatherOffsetTestBase::m_min_coordinate_value			 = -8;
const unsigned int		 GPUShader5TextureGatherOffsetTestBase::m_n_components_per_varying		 = 4;
const unsigned int		 GPUShader5TextureGatherOffsetTestBase::m_n_texture_array_length		 = 4;
const unsigned int		 GPUShader5TextureGatherOffsetTestBase::m_n_vertices					 = 128;
const glw::GLchar* const GPUShader5TextureGatherOffsetTestBase::m_sampler_uniform_name			 = "sampler";
const glw::GLchar* const GPUShader5TextureGatherOffsetTestBase::m_reference_sampler_uniform_name = "reference_sampler";

/* Constants for GPUShader5TextureGatherOffsetColorTestBase */
const unsigned int GPUShader5TextureGatherOffsetColorTestBase::m_texture_size = 64;

/* Constants for GPUShader5TextureGatherOffsetDepthTestBase */
const unsigned int GPUShader5TextureGatherOffsetDepthTestBase::m_texture_size = 64;

/* Constants for GPUShader5TextureGatherOffsetColor2DRepeatCaseTest */
const glw::GLchar* const GPUShader5TextureGatherOffsetColor2DRepeatCaseTest::m_offsets_attribute_name = "offsets";
const unsigned int		 GPUShader5TextureGatherOffsetColor2DRepeatCaseTest::m_n_offsets_components   = 2;

/* Constants for GPUShader5TextureGatherOffsetDepth2DRepeatCaseTest */
const glw::GLchar* const GPUShader5TextureGatherOffsetDepth2DRepeatCaseTest::m_offsets_attribute_name = "offsets";
const unsigned int		 GPUShader5TextureGatherOffsetDepth2DRepeatCaseTest::m_n_offsets_components   = 2;

/** Constructor
 *
 *  @param context       Test context
 *  @param name          Test case's name
 *  @param description   Test case's description
 **/
GPUShader5TextureGatherOffsetTestBase::GPUShader5TextureGatherOffsetTestBase(Context&			  context,
																			 const ExtParameters& extParams,
																			 const char* name, const char* description)
	: TestCaseBase(context, extParams, name, description)
	, m_min_texture_gather_offset(0)
	, m_max_texture_gather_offset(0)
	, m_fragment_shader_id(0)
	, m_program_object_id(0)
	, m_vertex_shader_id(0)
	, m_vertex_array_object_id(0)
	, m_texture_object_id(0)
	, m_sampler_object_id(0)
	, m_is_texture_array(0)
	, m_texture_bytes_per_pixel(0)
	, m_texture_format(0)
	, m_texture_internal_format(0)
	, m_texture_type(0)
	, m_texture_size(0)
	, m_texture_wrap_mode(0)
	, m_transform_feedback_buffer_size(0)
	, m_transform_feedback_buffer_object_id(0)
	, m_n_coordinates_components(0)
{
	/* Nothing to be done here */
}

/** Deinitializes GLES objects created during the test.
 *
 */
void GPUShader5TextureGatherOffsetTestBase::deinit()
{
	/* GL */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Bind default values */
	gl.useProgram(0);
	gl.bindVertexArray(0);
	gl.bindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0 /* offset */, 0 /* id */);
	gl.bindBuffer(GL_ARRAY_BUFFER, 0);

	/* Delete everything */
	if (0 != m_vertex_array_object_id)
	{
		gl.deleteVertexArrays(1, &m_vertex_array_object_id);

		m_vertex_array_object_id = 0;
	}

	if (0 != m_transform_feedback_buffer_object_id)
	{
		gl.deleteBuffers(1, &m_transform_feedback_buffer_object_id);

		m_transform_feedback_buffer_object_id = 0;
	}

	if (false == m_vertex_buffer_ids.empty())
	{
		gl.deleteBuffers((glw::GLsizei)m_vertex_buffer_ids.size(), &m_vertex_buffer_ids[0]);
	}

	if (0 != m_program_object_id)
	{
		gl.deleteProgram(m_program_object_id);

		m_program_object_id = 0;
	}

	if (0 != m_fragment_shader_id)
	{
		gl.deleteShader(m_fragment_shader_id);

		m_fragment_shader_id = 0;
	}

	if (0 != m_vertex_shader_id)
	{
		gl.deleteShader(m_vertex_shader_id);

		m_vertex_shader_id = 0;
	}

	if (0 != m_texture_object_id)
	{
		gl.deleteTextures(1, &m_texture_object_id);

		m_texture_object_id = 0;
	}

	if (0 != m_sampler_object_id)
	{
		gl.deleteSamplers(1, &m_sampler_object_id);

		m_sampler_object_id = 0;
	}

	/* Deinitialize base */
	TestCaseBase::deinit();
}

/** Initializes GLES objects used during the test.
 *
 */
void GPUShader5TextureGatherOffsetTestBase::initTest()
{
	/* This test should only run if EXT_gpu_shader5 is supported */
	if (true != m_is_gpu_shader5_supported)
	{
		throw tcu::NotSupportedError(GPU_SHADER5_EXTENSION_NOT_SUPPORTED, "", __FILE__, __LINE__);
	}

	/* GL */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Get minimum and maximum texture gather offsets */
	gl.getIntegerv(GL_MIN_PROGRAM_TEXTURE_GATHER_OFFSET, &m_min_texture_gather_offset);
	gl.getIntegerv(GL_MAX_PROGRAM_TEXTURE_GATHER_OFFSET, &m_max_texture_gather_offset);

	GLU_EXPECT_NO_ERROR(
		gl.getError(),
		"Failed to get value of GL_MIN_PROGRAM_TEXTURE_GATHER_OFFSET or GL_MAX_PROGRAM_TEXTURE_GATHER_OFFSET");

	/* Get details for transform feedback from child class */
	getTransformFeedBackDetails(m_transform_feedback_buffer_size, m_captured_varying_names);

	/* Get shaders' code from children class */
	getShaderParts(m_vertex_shader_parts);

	/* Create program and shaders */
	m_program_object_id = gl.createProgram();

	m_fragment_shader_id = gl.createShader(GL_FRAGMENT_SHADER);
	m_vertex_shader_id   = gl.createShader(GL_VERTEX_SHADER);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Failed to create program or shader object(s)");

	/* Set up transform feedback */
	gl.transformFeedbackVaryings(m_program_object_id, (glw::GLsizei)m_captured_varying_names.size(),
								 &m_captured_varying_names[0], GL_INTERLEAVED_ATTRIBS);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glTransformFeedbackVaryings() call failed");

	/* Build program */
	if (false == buildProgram(m_program_object_id, m_fragment_shader_id, 1 /* number of FS parts */,
							  &m_fragment_shader_code, m_vertex_shader_id, (unsigned int)m_vertex_shader_parts.size(),
							  &m_vertex_shader_parts[0]))
	{
		TCU_FAIL("Could not create program from valid vertex/geometry/fragment shader");
	}

	/* Set program as active */
	gl.useProgram(m_program_object_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram() call failed");

	/* Generate and bind VAO */
	gl.genVertexArrays(1, &m_vertex_array_object_id);
	gl.bindVertexArray(m_vertex_array_object_id);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not create vertex array object");

	/* Generate, bind and allocate buffer */
	gl.genBuffers(1, &m_transform_feedback_buffer_object_id);
	gl.bindBuffer(GL_ARRAY_BUFFER, m_transform_feedback_buffer_object_id);
	gl.bufferData(GL_ARRAY_BUFFER, m_transform_feedback_buffer_size, 0 /* no start data */, GL_STATIC_DRAW);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not create buffer object");

	gl.bindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, m_transform_feedback_buffer_object_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBufferBase() call failed");

	/* Create and fill texture */
	prepareTexture();

	/* Let inheriting class prepare test specific input for program */
	prepareProgramInput();
}

/** Executes the test.
 *  Sets the test result to QP_TEST_RESULT_FAIL if the test failed, QP_TEST_RESULT_PASS otherwise.
 *
 *  @return STOP if the test has finished, CONTINUE to indicate iterate should be called once again.
 *
 *  Note the function throws exception should an error occur!
 **/
tcu::TestCase::IterateResult GPUShader5TextureGatherOffsetTestBase::iterate()
{
	initTest();

	/* Retrieve ES entrypoints */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Verification result */
	bool result = false;

	/* Setup transform feedback */
	gl.enable(GL_RASTERIZER_DISCARD);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glEnable(GL_RASTERIZER_DISCARD) call failed");

	gl.beginTransformFeedback(GL_POINTS);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBeginTransformFeedback() call failed");
	{
		/* Draw */
		gl.drawArrays(GL_POINTS, 0 /* first */, m_n_vertices);

		GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawArrays() call failed");
	}
	/* Stop transform feedback */
	gl.endTransformFeedback();
	GLU_EXPECT_NO_ERROR(gl.getError(), "glEndTransformFeedback() call failed");

	gl.disable(GL_RASTERIZER_DISCARD);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glDisable(GL_RASTERIZER_DISCARD) call failed");

	/* Map transfrom feedback results */
	const void* transform_feedback_data = gl.mapBufferRange(GL_TRANSFORM_FEEDBACK_BUFFER, 0 /* offset */,
															m_transform_feedback_buffer_size, GL_MAP_READ_BIT);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not map the buffer object into process space");

	/* Verify data extracted from transfrom feedback */
	result = verifyResult(transform_feedback_data);

	/* Unmap transform feedback buffer */
	gl.unmapBuffer(GL_TRANSFORM_FEEDBACK_BUFFER);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error unmapping the buffer object");

	/* Verify results */
	if (true != result)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
	}
	else
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}

	return STOP;
}

/** Logs array of ints
 *  Format: description ( e1 e2 ... eN )
 *
 *  @param data        Array of ints
 *  @param length      Number of elements to log
 *  @param description Description string
 **/
void GPUShader5TextureGatherOffsetTestBase::logArray(const glw::GLint* data, unsigned int length,
													 const char* description)
{
	/* Message object */
	tcu::MessageBuilder message = m_testCtx.getLog() << tcu::TestLog::Message;

	/* Adds description */
	message << description;

	/* Begin array */
	message << "( ";

	/* For each varying */
	for (unsigned int i = 0; i < length; ++i)
	{
		message << data[i] << " ";
	}

	/* End array */
	message << ") ";

	message << tcu::TestLog::EndMessage;
}

/** Logs coordinates for vertex at given index
 *  Format: Texture coordinates (range [0:1]): [x, y, .. ]
 *
 *  @param index Index of vertex
 **/
void GPUShader5TextureGatherOffsetTestBase::logCoordinates(unsigned int index)
{
	/* Message object */
	tcu::MessageBuilder message = m_testCtx.getLog() << tcu::TestLog::Message;

	/* Begin array */
	message << "Texture coordinates (range [0:1]): [";

	/* Component index */
	unsigned int i = 0;

	while (1)
	{
		message << m_coordinates_buffer_data[index * m_n_coordinates_components + i];

		i += 1;

		if (i >= m_n_coordinates_components)
		{
			break;
		}
		else
		{
			message << ", ";
		}
	}

	/* End array */
	message << "]" << tcu::TestLog::EndMessage;
}

/** Prepare buffers for vertex attributes
 *
 **/
void GPUShader5TextureGatherOffsetTestBase::prepareProgramInput()
{
	/* GL */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Get vertex buffer infos from child class */
	prepareVertexBuffersData(m_vertex_buffer_infos);

	/* Prepare info for coordinates */
	prepareVertexBufferInfoForCoordinates();

	/* Constant for number of vertex buffers */
	const unsigned int n_vertex_buffers = (const unsigned int)m_vertex_buffer_infos.size();

	/* Prepare storage for vertex buffer ids */
	m_vertex_buffer_ids.resize(n_vertex_buffers);

	/* Generate buffers */
	gl.genBuffers(n_vertex_buffers, &m_vertex_buffer_ids[0]);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Failed to create vertex buffers");

	/* Setup each vertex buffer */
	for (unsigned int i = 0; i < n_vertex_buffers; ++i)
	{
		/* References for i-th vertex buffer */
		glw::GLuint		  vertex_buffer_id   = m_vertex_buffer_ids[i];
		VertexBufferInfo& vertex_buffer_info = m_vertex_buffer_infos[i];

		/* Get attribute location */
		glw::GLint attribute_location =
			gl.getAttribLocation(m_program_object_id, m_vertex_buffer_infos[i].attribute_name);

		if ((-1 == attribute_location) || (GL_NO_ERROR != gl.getError()))
		{
			TCU_FAIL("Failed to get location of attribute");
		}

		/* Setup buffer for offsets attribute */
		gl.bindBuffer(GL_ARRAY_BUFFER, vertex_buffer_id);
		gl.bufferData(GL_ARRAY_BUFFER, vertex_buffer_info.data_size, vertex_buffer_info.data, GL_STATIC_DRAW);

		GLU_EXPECT_NO_ERROR(gl.getError(), "Failed to fill vertex buffer with data");

		/* Setup attribute */
		gl.enableVertexAttribArray(attribute_location);

		if (GL_INT == vertex_buffer_info.type)
		{
			gl.vertexAttribIPointer(attribute_location, vertex_buffer_info.n_components, vertex_buffer_info.type,
									0 /* stride */, 0 /* offset */);
		}
		else
		{
			gl.vertexAttribPointer(attribute_location, vertex_buffer_info.n_components, vertex_buffer_info.type,
								   GL_FALSE /* normalization */, 0 /* stride */, 0 /* offset */);
		}

		GLU_EXPECT_NO_ERROR(gl.getError(), "Failed to setup vertex attribute arrays");
	}

	/* Get sampler location */
	glw::GLint sampler_location = gl.getUniformLocation(m_program_object_id, m_sampler_uniform_name);

	if ((-1 == sampler_location) || (GL_NO_ERROR != gl.getError()))
	{
		TCU_FAIL("Failed to get location of uniform \"sampler\"");
	}

	/* Set uniform at sampler location value to index of first texture unit */
	gl.uniform1i(sampler_location, 0 /* first texture unit */);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not set uniform value");

	if (GL_CLAMP_TO_EDGE == m_texture_wrap_mode)
	{
		/* Get reference_sampler location */
		glw::GLint reference_sampler_location =
			gl.getUniformLocation(m_program_object_id, m_reference_sampler_uniform_name);

		if ((-1 == reference_sampler_location) || (GL_NO_ERROR != gl.getError()))
		{
			TCU_FAIL("Failed to get location of uniform \"reference sampler\"");
		}

		/* Set uniform at reference_sampler location value to index of second texture unit */
		gl.uniform1i(reference_sampler_location, 1 /* second texture unit */);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Could not set uniform value");
	}
}

/** Prepare texture and bind it to sampler
 *
 **/
void GPUShader5TextureGatherOffsetTestBase::prepareTexture()
{
	/* Retrieve ES entrypoints */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Get texture info from child class */
	getTextureInfo(m_texture_size, m_texture_internal_format, m_texture_format, m_texture_type,
				   m_texture_bytes_per_pixel);

	isTextureArray(m_is_texture_array);
	getTextureWrapMode(m_texture_wrap_mode);

	/* Verify that recieved texture parameters are valid */
	glw::GLint max_texture_size = 0;

	gl.getIntegerv(GL_MAX_TEXTURE_SIZE, &max_texture_size);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv() failed for GL_MAX_TEXTURE_SIZE pname");

	if (m_texture_size > (glw::GLuint)max_texture_size)
	{
		throw tcu::NotSupportedError("Required texture dimmensions exceeds GL_MAX_TEXTURE_SIZE limit", "", __FILE__,
									 __LINE__);
	}

	/* Activate first texture unit */
	gl.activeTexture(GL_TEXTURE0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glActiveTexture(GL_TEXTURE0) failed");

	/* Generate and allocate texture */
	gl.genTextures(1, &m_texture_object_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenTextures() call failed");

	if (true == m_is_texture_array)
	{
		gl.bindTexture(GL_TEXTURE_2D_ARRAY, m_texture_object_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture() call failed");

		gl.texStorage3D(GL_TEXTURE_2D_ARRAY, 1 /* levels */, m_texture_internal_format, m_texture_size, m_texture_size,
						m_n_texture_array_length);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glTexStorage3D() call failed");
	}
	else
	{
		gl.bindTexture(GL_TEXTURE_2D, m_texture_object_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture() call failed");

		gl.texStorage2D(GL_TEXTURE_2D, 1 /* levels */, m_texture_internal_format, m_texture_size, m_texture_size);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glTexStorage2D() call failed");
	}

	/* Storage for texture data */
	std::vector<glw::GLubyte> texture_data;

	texture_data.resize(m_texture_size * m_texture_size * m_texture_bytes_per_pixel);

	/* Let child class prepare data for texture */
	prepareTextureData(&texture_data[0]);

	/* Update texture data */
	if (true == m_is_texture_array)
	{
		for (unsigned int z = 0; z < m_n_texture_array_length; ++z)
		{
			gl.texSubImage3D(GL_TEXTURE_2D_ARRAY, 0 /* level */, 0 /* x */, 0 /* y */, z, m_texture_size,
							 m_texture_size, 1 /* depth */, m_texture_format, m_texture_type, &texture_data[0]);

			GLU_EXPECT_NO_ERROR(gl.getError(), "Failed to update texture data");
		}
	}
	else
	{
		gl.texSubImage2D(GL_TEXTURE_2D, 0 /* level */, 0 /* x */, 0 /* y */, m_texture_size, m_texture_size,
						 m_texture_format, m_texture_type, &texture_data[0]);

		GLU_EXPECT_NO_ERROR(gl.getError(), "Failed to update texture data");
	}

	/* Set shadow comparison, filtering and wrap modes */
	glw::GLenum texture_target;

	if (true == m_is_texture_array)
	{
		texture_target = GL_TEXTURE_2D_ARRAY;
	}
	else
	{
		texture_target = GL_TEXTURE_2D;
	}

	/* Storage for border color */
	glw::GLfloat color[4];

	/* Get border color */
	getBorderColor(color);

	gl.texParameteri(texture_target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	gl.texParameteri(texture_target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	gl.texParameteri(texture_target, GL_TEXTURE_WRAP_S, m_texture_wrap_mode);
	gl.texParameteri(texture_target, GL_TEXTURE_WRAP_T, m_texture_wrap_mode);
	gl.texParameteri(texture_target, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
	gl.texParameteri(texture_target, GL_TEXTURE_COMPARE_FUNC, GL_LESS);

	/* Set border color when "clamp to border" mode is selected */
	if (m_glExtTokens.CLAMP_TO_BORDER == m_texture_wrap_mode)
	{
		gl.texParameterfv(texture_target, m_glExtTokens.TEXTURE_BORDER_COLOR, color);
	}

	GLU_EXPECT_NO_ERROR(gl.getError(), "Failed to set texture parameters");

	/* Setup texture unit for reference_sampler */
	if (GL_CLAMP_TO_EDGE == m_texture_wrap_mode)
	{
		/* Activate second texture unit */
		gl.activeTexture(GL_TEXTURE1);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glActiveTexture(GL_TEXTURE1) failed");

		/* Bind texture */
		gl.bindTexture(texture_target, m_texture_object_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture failed");

		/* Generate sampler */
		gl.genSamplers(1, &m_sampler_object_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Failed to generate sampler");

		/* Bind sampler to second texture unit */
		gl.bindSampler(1, m_sampler_object_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Failed to bind sampler to unit 1");

		/* Set sampler parameters */
		gl.samplerParameteri(m_sampler_object_id, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		gl.samplerParameteri(m_sampler_object_id, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		gl.samplerParameteri(m_sampler_object_id, GL_TEXTURE_WRAP_S, m_glExtTokens.CLAMP_TO_BORDER);
		gl.samplerParameteri(m_sampler_object_id, GL_TEXTURE_WRAP_T, m_glExtTokens.CLAMP_TO_BORDER);
		gl.samplerParameteri(m_sampler_object_id, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
		gl.samplerParameteri(m_sampler_object_id, GL_TEXTURE_COMPARE_FUNC, GL_LESS);
		gl.samplerParameterfv(m_sampler_object_id, m_glExtTokens.TEXTURE_BORDER_COLOR, color);

		GLU_EXPECT_NO_ERROR(gl.getError(), "Failed to set sampler parameters");
	}
}

/** Prepare buffer for texture coordinates
 *
 **/
void GPUShader5TextureGatherOffsetTestBase::prepareVertexBufferInfoForCoordinates()
{
	/* Select proper number of components */
	if (true == m_is_texture_array)
	{
		m_n_coordinates_components = 3;
	}
	else
	{
		m_n_coordinates_components = 2;
	}

	/* Prepare storage for texture coordinates */
	m_coordinates_buffer_data.resize(m_n_vertices * m_n_coordinates_components);

	/* Variables for calculation of texture coordiantes' range and offsets */
	const float coordinate_denominator = m_coordinate_resolution;
	int			x_modulus;
	float		x_offset;
	float		x_range;
	int			y_modulus;
	float		y_offset;
	float		y_range;

	/* Calculation of texture coordiantes' range and offsets */
	x_range = m_max_coordinate_value - m_min_coordinate_value;
	y_range = m_max_coordinate_value - m_min_coordinate_value;

	x_offset = m_min_coordinate_value;
	y_offset = m_min_coordinate_value;

	x_range *= coordinate_denominator;
	y_range *= coordinate_denominator;

	x_modulus = (int)x_range;
	y_modulus = (int)y_range;

	/* Prepare texture coordinates */
	if (true == m_is_texture_array)
	{
		/* First four coordinates are corners, each comes from unique level */
		setCoordinatesData(0.0f, 0.0f, 0.0f, 0);
		setCoordinatesData(0.0f, 1.0f, 1.0f, 1);
		setCoordinatesData(1.0f, 0.0f, 2.0f, 2);
		setCoordinatesData(1.0f, 1.0f, 3.0f, 3);

		/* Generate rest of texture coordinates */
		for (unsigned int i = 4; i < m_n_vertices; ++i)
		{
			const glw::GLfloat coord_x = ((float)(rand() % x_modulus)) / coordinate_denominator + x_offset;
			const glw::GLfloat coord_y = ((float)(rand() % y_modulus)) / coordinate_denominator + y_offset;
			const glw::GLfloat coord_z =
				((float)(rand() % m_n_texture_array_length * m_coordinate_resolution)) / coordinate_denominator;

			setCoordinatesData(coord_x, coord_y, coord_z, i);
		}
	}
	else
	{
		/* First four coordinates are corners */
		setCoordinatesData(0.0f, 0.0f, 0);
		setCoordinatesData(0.0f, 1.0f, 1);
		setCoordinatesData(1.0f, 0.0f, 2);
		setCoordinatesData(1.0f, 1.0f, 3);

		/* Generate rest of texture coordinates */
		for (unsigned int i = 4; i < m_n_vertices; ++i)
		{
			const glw::GLfloat coord_x = ((float)(rand() % x_modulus)) / coordinate_denominator + x_offset;
			const glw::GLfloat coord_y = ((float)(rand() % y_modulus)) / coordinate_denominator + y_offset;

			setCoordinatesData(coord_x, coord_y, i);
		}
	}

	/* Setup coordinates VB info */
	VertexBufferInfo info;

	info.attribute_name = m_coordinates_attribute_name;
	info.n_components   = m_n_coordinates_components;
	info.type			= GL_FLOAT;
	info.data			= &m_coordinates_buffer_data[0];
	info.data_size		= (glw::GLuint)(m_coordinates_buffer_data.size() * sizeof(glw::GLfloat));

	/* Store results */
	m_vertex_buffer_infos.push_back(info);
}

/** Set 2D texture coordinate for vertex at given index
 *
 *  @param x     X coordinate
 *  @param y     Y coordinate
 *  @param index Index of vertex
 **/
void GPUShader5TextureGatherOffsetTestBase::setCoordinatesData(glw::GLfloat x, glw::GLfloat y, unsigned int index)
{
	const unsigned int vertex_offset = m_n_coordinates_components * index;

	m_coordinates_buffer_data[vertex_offset + 0] = x;
	m_coordinates_buffer_data[vertex_offset + 1] = y;
}

/** Set 3D texture coordinate for vertex at given index
 *
 *  @param x     X coordinate
 *  @param y     Y coordinate
 *  @param z     Z coordinate
 *  @param index Index of vertex
 **/
void GPUShader5TextureGatherOffsetTestBase::setCoordinatesData(glw::GLfloat x, glw::GLfloat y, glw::GLfloat z,
															   unsigned int index)
{
	const unsigned int vertex_offset = m_n_coordinates_components * index;

	m_coordinates_buffer_data[vertex_offset + 0] = x;
	m_coordinates_buffer_data[vertex_offset + 1] = y;
	m_coordinates_buffer_data[vertex_offset + 2] = z;
}

/** Constructor
 *
 * @param context       Test context
 * @param name          Test case's name
 * @param description   Test case's description
 **/
GPUShader5TextureGatherOffsetColorTestBase::GPUShader5TextureGatherOffsetColorTestBase(Context&				context,
																					   const ExtParameters& extParams,
																					   const char*			name,
																					   const char*			description)
	: GPUShader5TextureGatherOffsetTestBase(context, extParams, name, description), m_n_varyings_per_vertex(0)
{
	/* Nothing to be done here */
}

/** Provides color that will be used as border color for "clamp to border" mode
 *
 * @param out_color Color
 **/
void GPUShader5TextureGatherOffsetColorTestBase::getBorderColor(glw::GLfloat out_color[4])
{
	out_color[0] = -1.0f;
	out_color[1] = -1.0f;
	out_color[2] = -1.0f;
	out_color[3] = -1.0f;
}

/** Provides informations required to create texture
 *
 *  @param out_size                    Size of texture
 *  @param out_texture_internal_format Internal format of texture
 *  @param out_texture_format          Format of texture
 *  @param out_texture_type            Type of texture
 *  @param out_bytes_per_pixel         Bytes per pixel
 **/
void GPUShader5TextureGatherOffsetColorTestBase::getTextureInfo(glw::GLuint& out_size,
																glw::GLenum& out_texture_internal_format,
																glw::GLenum& out_texture_format,
																glw::GLenum& out_texture_type,
																glw::GLuint& out_bytes_per_pixel)
{
	out_size					= m_texture_size;
	out_texture_internal_format = GL_RGBA32I;
	out_texture_format			= GL_RGBA_INTEGER;
	out_texture_type			= GL_INT;
	out_bytes_per_pixel			= 4 * sizeof(glw::GLint); /* RGBA * int */
}

/** Get wrap mode for texture
 *
 *  @param out_wrap_mode Wrap mode
 **/
void GPUShader5TextureGatherOffsetColorTestBase::getTextureWrapMode(glw::GLenum& out_wrap_mode)
{
	out_wrap_mode = GL_REPEAT;
}

/** Get size of buffer used as output from transform feedback and names of varyings
 *
 *  @param out_buffer_size       Size of buffer
 *  @param out_captured_varyings Names of varyings
 **/
void GPUShader5TextureGatherOffsetColorTestBase::getTransformFeedBackDetails(
	glw::GLuint& out_buffer_size, std::vector<const glw::GLchar*>& out_captured_varyings)
{
	out_captured_varyings.reserve(8);
	out_captured_varyings.push_back("without_offset_0");
	out_captured_varyings.push_back("with_offset_0");
	out_captured_varyings.push_back("without_offset_1");
	out_captured_varyings.push_back("with_offset_1");
	out_captured_varyings.push_back("without_offset_2");
	out_captured_varyings.push_back("with_offset_2");
	out_captured_varyings.push_back("without_offset_3");
	out_captured_varyings.push_back("with_offset_3");

	m_n_varyings_per_vertex = (unsigned int)out_captured_varyings.size();

	out_buffer_size = static_cast<glw::GLuint>(m_n_vertices * m_n_varyings_per_vertex * m_n_components_per_varying *
											   sizeof(glw::GLint));
}

/** Check if texture array is required
 *
 *  @param out_is_texture_array Set to true if texture array is required, false otherwise
 **/
void GPUShader5TextureGatherOffsetColorTestBase::isTextureArray(bool& out_is_texture_array)
{
	out_is_texture_array = false;
}

/** Prepare texture data as expected by all cases in test 9.
 *  Each texel is assigned {x, y, x, y}, where x and y are coordinates of texel.
 *
 *  @param data Storage space for texture data
 **/
void GPUShader5TextureGatherOffsetColorTestBase::prepareTextureData(glw::GLubyte* data)
{
	/* Data pointer */
	glw::GLint* texture_data = (glw::GLint*)data;

	/* Constants */
	const unsigned int n_components_per_pixel = 4;
	const unsigned int line_size			  = n_components_per_pixel * m_texture_size;

	/* Prepare texture's data */
	for (unsigned int y = 0; y < m_texture_size; ++y)
	{
		for (unsigned int x = 0; x < m_texture_size; ++x)
		{
			const unsigned int pixel_offset = y * line_size + x * n_components_per_pixel;

			/* texel.r = x, texel.g = y */
			texture_data[pixel_offset + 0] = x;
			texture_data[pixel_offset + 1] = y;

			/* texel.b = x, texel.a = y */
			texture_data[pixel_offset + 2] = x;
			texture_data[pixel_offset + 3] = y;
		}
	}
}

/** Verification of results
 *
 *  @param result Pointer to data mapped from transform feedback buffer. Size of
 *                data is equal to buffer_size set by getTransformFeedbackBufferSize
 *
 *  @return true  Result match expected value
 *          false Result has wrong value
 **/
bool GPUShader5TextureGatherOffsetColorTestBase::verifyResult(const void* result_data)
{
	/* All data stored in data are ints */
	glw::GLint* results = (glw::GLint*)result_data;

	/* Verification result */
	bool result = true;

	/* Constants for data offsets calculation */
	const unsigned int vertex_size_in_results_buffer = m_n_varyings_per_vertex * m_n_components_per_varying;
	const unsigned int bytes_per_varying			 = m_n_components_per_varying * sizeof(glw::GLint);
	const unsigned int vertex_varying_0_offset		 = 0;
	const unsigned int vertex_varying_1_offset		 = vertex_varying_0_offset + 2 * m_n_components_per_varying;
	const unsigned int vertex_varying_2_offset		 = vertex_varying_1_offset + 2 * m_n_components_per_varying;
	const unsigned int vertex_varying_3_offset		 = vertex_varying_2_offset + 2 * m_n_components_per_varying;
	const unsigned int with_offset_offset			 = m_n_components_per_varying;
	const unsigned int without_offset_offset		 = 0;

	/* For each vertex */
	for (unsigned int vertex = 0; vertex < m_n_vertices; ++vertex)
	{
		CapturedVaryings captured_data;

		/* Pointers to all varyings for given vertex */
		const glw::GLint* without_offset_0 =
			results + vertex_varying_0_offset + (vertex * vertex_size_in_results_buffer) + without_offset_offset;
		const glw::GLint* with_offset_0 =
			results + vertex_varying_0_offset + (vertex * vertex_size_in_results_buffer) + with_offset_offset;

		const glw::GLint* without_offset_1 =
			results + vertex_varying_1_offset + (vertex * vertex_size_in_results_buffer) + without_offset_offset;
		const glw::GLint* with_offset_1 =
			results + vertex_varying_1_offset + (vertex * vertex_size_in_results_buffer) + with_offset_offset;

		const glw::GLint* without_offset_2 =
			results + vertex_varying_2_offset + (vertex * vertex_size_in_results_buffer) + without_offset_offset;
		const glw::GLint* with_offset_2 =
			results + vertex_varying_2_offset + (vertex * vertex_size_in_results_buffer) + with_offset_offset;

		const glw::GLint* without_offset_3 =
			results + vertex_varying_3_offset + (vertex * vertex_size_in_results_buffer) + without_offset_offset;
		const glw::GLint* with_offset_3 =
			results + vertex_varying_3_offset + (vertex * vertex_size_in_results_buffer) + with_offset_offset;

		/* Copy captured varyings to captured_data instance */
		memcpy(captured_data.without_offset_0, without_offset_0, bytes_per_varying);
		memcpy(captured_data.without_offset_1, without_offset_1, bytes_per_varying);
		memcpy(captured_data.without_offset_2, without_offset_2, bytes_per_varying);
		memcpy(captured_data.without_offset_3, without_offset_3, bytes_per_varying);

		memcpy(captured_data.with_offset_0, with_offset_0, bytes_per_varying);
		memcpy(captured_data.with_offset_1, with_offset_1, bytes_per_varying);
		memcpy(captured_data.with_offset_2, with_offset_2, bytes_per_varying);
		memcpy(captured_data.with_offset_3, with_offset_3, bytes_per_varying);

		/* Let child class check if result is ok */
		const bool is_result_ok = checkResult(captured_data, vertex, m_texture_size);

		/* Do not break on first error */
		if (false == is_result_ok)
		{
			result = false;
		}
	}

	/* Done */
	return result;
}

/** Logs data captured varyings
 *
 *  @param varyings Instance of capturedVaryings to be logged
 **/
void GPUShader5TextureGatherOffsetColorTestBase::logVaryings(const CapturedVaryings& varyings)
{
	logArray(varyings.without_offset_0, m_n_components_per_varying, "Without offset X: ");

	logArray(varyings.without_offset_1, m_n_components_per_varying, "Without offset Y: ");

	logArray(varyings.without_offset_2, m_n_components_per_varying, "Without offset Z: ");

	logArray(varyings.without_offset_3, m_n_components_per_varying, "Without offset W: ");

	logArray(varyings.with_offset_0, m_n_components_per_varying, "With offset X: ");

	logArray(varyings.with_offset_1, m_n_components_per_varying, "With offset Y: ");

	logArray(varyings.with_offset_2, m_n_components_per_varying, "With offset Z: ");

	logArray(varyings.with_offset_3, m_n_components_per_varying, "With offset W: ");
}

/** Constructor
 *
 * @param context       Test context
 * @param name          Test case's name
 * @param description   Test case's description
 **/
GPUShader5TextureGatherOffsetColor2DRepeatCaseTest::GPUShader5TextureGatherOffsetColor2DRepeatCaseTest(
	Context& context, const ExtParameters& extParams, const char* name, const char* description)
	: GPUShader5TextureGatherOffsetColorTestBase(context, extParams, name, description)
{
	/* Nothing to be done here */
}

/** Get parts of vertex shader
 *
 *  @param out_vertex_shader_parts Vector of vertex shader parts
 **/
void GPUShader5TextureGatherOffsetColor2DRepeatCaseTest::getShaderParts(
	std::vector<const glw::GLchar*>& out_vertex_shader_parts)
{
	out_vertex_shader_parts.push_back(m_vertex_shader_code);
}

/** Prepare test specific data, that will be used as vertex attributes
 *
 *  @param vertex_buffer_infos Vector of vertexBufferInfo instances
 **/
void GPUShader5TextureGatherOffsetColor2DRepeatCaseTest::prepareVertexBuffersData(
	std::vector<VertexBufferInfo>& vertex_buffer_infos)
{
	/* Constats for offsets and size of buffer */
	const unsigned int n_total_components = m_n_vertices * m_n_offsets_components;

	/* Limits used to generate offsets */
	const glw::GLint min_ptgo = m_min_texture_gather_offset;
	const glw::GLint max_ptgo = m_max_texture_gather_offset;

	/* Storage for offsets */
	m_offsets_buffer_data.resize(n_total_components);

	/* Set the seed for random number generator */
	randomSeed(1);

	/* Generate offsets */
	for (unsigned int i = 0; i < n_total_components; ++i)
	{
		m_offsets_buffer_data[i] =
			static_cast<glw::GLint>(randomFormula(static_cast<glw::GLuint>(max_ptgo - min_ptgo + 1))) + min_ptgo;
	}

	/* Setup offsets VB info */
	VertexBufferInfo info;

	info.attribute_name = m_offsets_attribute_name;
	info.n_components   = m_n_offsets_components;
	info.type			= GL_INT;
	info.data			= &m_offsets_buffer_data[0];
	info.data_size		= (glw::GLuint)(m_offsets_buffer_data.size() * sizeof(glw::GLint));

	/* Add VB info to vector */
	vertex_buffer_infos.push_back(info);
}

/** Check if data captured for vertex at index are as expected
 *
 *  @param captured_data Instance of capturedVaryings for vertex at index
 *  @param index         Index of vertex
 *  @param texture_size  Size of texture
 *
 *  @return True  Results match expected values
 *          False Results do not match expected values
 **/
bool GPUShader5TextureGatherOffsetColor2DRepeatCaseTest::checkResult(const CapturedVaryings& captured_data,
																	 unsigned int index, unsigned int tex_size)
{
	/* Signed int value has to be used with operator % in order to work as expected */
	const glw::GLint texture_size = (glw::GLint)tex_size;

	/* Get offsets for vertex */
	glw::GLint x_offset;
	glw::GLint y_offset;

	getOffsets(x_offset, y_offset, index);

	/* Roll x offsets around left edge when negative */
	if (0 > x_offset)
	{
		x_offset = x_offset % texture_size;
		x_offset += texture_size;
	}

	/* Roll y offsets around top edge when negative */
	if (0 > y_offset)
	{
		y_offset = y_offset % texture_size;
		y_offset += texture_size;
	}

	/* Storage for expected values of with_offset varyings */
	glw::GLint expected_values_0[m_n_components_per_varying];
	glw::GLint expected_values_1[m_n_components_per_varying];
	glw::GLint expected_values_2[m_n_components_per_varying];
	glw::GLint expected_values_3[m_n_components_per_varying];

	/* Calculate expected values of with_offset varyings */
	for (unsigned int i = 0; i < m_n_components_per_varying; ++i)
	{
		expected_values_0[i] = ((x_offset + captured_data.without_offset_0[i]) % texture_size);
		expected_values_1[i] = ((y_offset + captured_data.without_offset_1[i]) % texture_size);
		expected_values_2[i] = ((x_offset + captured_data.without_offset_2[i]) % texture_size);
		expected_values_3[i] = ((y_offset + captured_data.without_offset_3[i]) % texture_size);
	}

	/* Verify that expected_values match those in with_offset varyings */
	for (unsigned int i = 0; i < m_n_components_per_varying; ++i)
	{
		if ((captured_data.with_offset_0[i] != expected_values_0[i]) ||
			(captured_data.with_offset_1[i] != expected_values_1[i]) ||
			(captured_data.with_offset_2[i] != expected_values_2[i]) ||
			(captured_data.with_offset_3[i] != expected_values_3[i]))
		{
			/* Log invalid results */
			m_testCtx.getLog() << tcu::TestLog::Section("Invalid result", "");

			logCoordinates(index);

			m_testCtx.getLog() << tcu::TestLog::Message << "Texture size: " << texture_size << tcu::TestLog::EndMessage;

			m_testCtx.getLog() << tcu::TestLog::Message << "Offsets: " << x_offset << ", " << y_offset
							   << tcu::TestLog::EndMessage;

			logVaryings(captured_data);

			m_testCtx.getLog() << tcu::TestLog::EndSection;

			/* Done */
			return false;
		}
	}

	/* Done */
	return true;
}

/** Get offsets for vertex at index
 *
 *  @param out_x_offset X offset
 *  @param out_y_offset Y offset
 *  @param index        Index of vertex
 **/
void GPUShader5TextureGatherOffsetColor2DRepeatCaseTest::getOffsets(glw::GLint& out_x_offset, glw::GLint& out_y_offset,
																	unsigned int index)
{
	out_x_offset = m_offsets_buffer_data[index * m_n_offsets_components + 0];
	out_y_offset = m_offsets_buffer_data[index * m_n_offsets_components + 1];
}

/** Constructor
 *
 * @param context       Test context
 * @param name          Test case's name
 * @param description   Test case's description
 **/
GPUShader5TextureGatherOffsetColor2DArrayCaseTest::GPUShader5TextureGatherOffsetColor2DArrayCaseTest(
	Context& context, const ExtParameters& extParams, const char* name, const char* description)
	: GPUShader5TextureGatherOffsetColor2DRepeatCaseTest(context, extParams, name, description)
{
	/* Nothing to be done here */
}

/** Get parts of vertex shader
 *
 *  @param out_vertex_shader_parts Vector of vertex shader parts
 **/
void GPUShader5TextureGatherOffsetColor2DArrayCaseTest::getShaderParts(
	std::vector<const glw::GLchar*>& out_vertex_shader_parts)
{
	out_vertex_shader_parts.push_back(m_vertex_shader_code);
}

/** Check if texture array is required
 *
 *  @param out_is_texture_array Set to true if texture array is required, false otherwise
 **/
void GPUShader5TextureGatherOffsetColor2DArrayCaseTest::isTextureArray(bool& out_is_texture_array)
{
	out_is_texture_array = true;
}

/** Constructor
 *
 * @param context       Test context
 * @param name          Test case's name
 * @param description   Test case's description
 **/
GPUShader5TextureGatherOffsetColor2DClampToBorderCaseTest::GPUShader5TextureGatherOffsetColor2DClampToBorderCaseTest(
	Context& context, const ExtParameters& extParams, const char* name, const char* description)
	: GPUShader5TextureGatherOffsetColor2DRepeatCaseTest(context, extParams, name, description)
{
	/* Nothing to be done here */
}

/** Get wrap mode for texture
 *
 *  @param out_wrap_mode Wrap mode
 **/
void GPUShader5TextureGatherOffsetColor2DClampToBorderCaseTest::getTextureWrapMode(glw::GLenum& out_wrap_mode)
{
	out_wrap_mode = m_glExtTokens.CLAMP_TO_BORDER;
}

/** Get parts of vertex shader
 *
 *  @param out_vertex_shader_parts Vector of vertex shader parts
 **/
void GPUShader5TextureGatherOffsetColor2DClampToBorderCaseTest::getShaderParts(
	std::vector<const glw::GLchar*>& out_vertex_shader_parts)
{
	out_vertex_shader_parts.push_back(m_vertex_shader_code);
}

/** Check if data captured for vertex at index are as expected
 *
 *  @param captured_data Instance of capturedVaryings for vertex at index
 *  @param index         Index of vertex
 *  @param texture_size  Size of texture
 *
 *  @return True  Results match expected values
 *          False Results do not match expected values
 **/
bool GPUShader5TextureGatherOffsetColor2DClampToBorderCaseTest::checkResult(const CapturedVaryings& captured_data,
																			unsigned int			index,
																			unsigned int			texture_size)
{
	/* Get offsets for vertex */
	glw::GLint x_offset;
	glw::GLint y_offset;

	getOffsets(x_offset, y_offset, index);

	/* Storage for expected values of with_offset varyings */
	glw::GLint expected_values_0[m_n_components_per_varying];
	glw::GLint expected_values_1[m_n_components_per_varying];
	glw::GLint expected_values_2[m_n_components_per_varying];
	glw::GLint expected_values_3[m_n_components_per_varying];

	/* Storage for with_offset values */
	glw::GLint with_offset_0[m_n_components_per_varying];
	glw::GLint with_offset_1[m_n_components_per_varying];
	glw::GLint with_offset_2[m_n_components_per_varying];
	glw::GLint with_offset_3[m_n_components_per_varying];

	/* Index of first not clamped texel in without_offset set*/
	int not_clamped_texel_index = -1;

	/* X and Y offsets of texels in 2x2 set */
	static const glw::GLint gather_offsets_x[] = { 0, 1, 1, 0 };
	static const glw::GLint gather_offsets_y[] = { 1, 1, 0, 0 };

	/* Find first not clamped texel in without_offset set */
	for (unsigned int i = 0; i < m_n_components_per_varying; ++i)
	{
		if ((0 <= captured_data.without_offset_0[i]) && (0 <= captured_data.without_offset_1[i]))
		{
			not_clamped_texel_index = i;
			break;
		}
	}

	if (-1 == not_clamped_texel_index)
	{
		/* Log problem with sampler */
		m_testCtx.getLog() << tcu::TestLog::Section("Unexpected sampling results", "");

		logCoordinates(index);

		m_testCtx.getLog() << tcu::TestLog::Message << "Texture size: " << texture_size << tcu::TestLog::EndMessage;

		m_testCtx.getLog() << tcu::TestLog::Message << "Offsets: " << x_offset << ", " << y_offset
						   << tcu::TestLog::EndMessage;

		logVaryings(captured_data);

		m_testCtx.getLog() << tcu::TestLog::EndSection;

		TCU_FAIL("Unexpected sampling results");
	}

	/* Fraction part of position for lower left texel without_offset set */
	const glw::GLint fract_position_x =
		captured_data.without_offset_0[not_clamped_texel_index] - gather_offsets_x[not_clamped_texel_index];
	const glw::GLint fract_position_y =
		captured_data.without_offset_1[not_clamped_texel_index] - gather_offsets_y[not_clamped_texel_index];

	/* Absolute position of lower left corner in without_offset set */
	const glw::GLint absolute_position_x = fract_position_x + captured_data.without_offset_2[0] * (int)texture_size;
	const glw::GLint absolute_position_y = fract_position_y + captured_data.without_offset_3[0] * (int)texture_size;

	/* Calculate absolute position for all texels in without_offset set */
	for (unsigned int i = 0; i < m_n_components_per_varying; ++i)
	{
		expected_values_0[i] = absolute_position_x + gather_offsets_x[i];
		expected_values_1[i] = absolute_position_y + gather_offsets_y[i];
		expected_values_2[i] = absolute_position_x + gather_offsets_x[i];
		expected_values_3[i] = absolute_position_y + gather_offsets_y[i];
	}

	/* Modify "border color" to -1 in with_offset set */
	for (unsigned int i = 0; i < m_n_components_per_varying; ++i)
	{
		with_offset_0[i] = 0 <= captured_data.with_offset_0[i] ? captured_data.with_offset_0[i] : -1;
		with_offset_1[i] = 0 <= captured_data.with_offset_1[i] ? captured_data.with_offset_1[i] : -1;
		with_offset_2[i] = 0 <= captured_data.with_offset_2[i] ? captured_data.with_offset_2[i] : -1;
		with_offset_3[i] = 0 <= captured_data.with_offset_3[i] ? captured_data.with_offset_3[i] : -1;
	}

	/* Apply offsets to absolute positions of without_offset */
	for (unsigned int i = 0; i < m_n_components_per_varying; ++i)
	{
		expected_values_0[i] += x_offset;
		expected_values_1[i] += y_offset;
		expected_values_2[i] += x_offset;
		expected_values_3[i] += y_offset;

		/* Set to -1, when point defined by expected_values_01 is outside of texture */
		if ((0 > expected_values_0[i]) || (((glw::GLint)texture_size) <= expected_values_0[i]) ||
			(0 > expected_values_1[i]) || (((glw::GLint)texture_size) <= expected_values_1[i]))
		{
			expected_values_0[i] = -1;
			expected_values_1[i] = -1;
		}

		/* Set to -1, when point defined by expected_values_23 is outside of texture */
		if ((0 > expected_values_2[i]) || (((glw::GLint)texture_size) <= expected_values_2[i]) ||
			(0 > expected_values_3[i]) || (((glw::GLint)texture_size) <= expected_values_3[i]))
		{
			expected_values_2[i] = -1;
			expected_values_3[i] = -1;
		}
	}

	/* Verify that expected_values match those in with_offset varyings */
	for (unsigned int i = 0; i < m_n_components_per_varying; ++i)
	{
		if ((with_offset_0[i] != expected_values_0[i]) || (with_offset_1[i] != expected_values_1[i]) ||
			(with_offset_2[i] != expected_values_2[i]) || (with_offset_3[i] != expected_values_3[i]))
		{
			/* Log invalid results */
			m_testCtx.getLog() << tcu::TestLog::Section("Invalid result", "");

			logCoordinates(index);

			m_testCtx.getLog() << tcu::TestLog::Message << "Texture size: " << texture_size << tcu::TestLog::EndMessage;

			m_testCtx.getLog() << tcu::TestLog::Message << "Offsets: " << x_offset << ", " << y_offset
							   << tcu::TestLog::EndMessage;

			logVaryings(captured_data);

			m_testCtx.getLog() << tcu::TestLog::EndSection;

			/* Done */
			return false;
		}
	}

	/* Done */
	return true;
}

/** Initializes GLES objects used during the test.
 *
 */
void GPUShader5TextureGatherOffsetColor2DClampToBorderCaseTest::initTest(void)
{
	/* Check if texture_border_clamp extension is supported */
	if (!m_is_texture_border_clamp_supported)
	{
		throw tcu::NotSupportedError(TEXTURE_BORDER_CLAMP_NOT_SUPPORTED, "", __FILE__, __LINE__);
	}

	GPUShader5TextureGatherOffsetColor2DRepeatCaseTest::initTest();
}

/** Constructor
 *
 * @param context       Test context
 * @param name          Test case's name
 * @param description   Test case's description
 **/
GPUShader5TextureGatherOffsetColor2DClampToEdgeCaseTest::GPUShader5TextureGatherOffsetColor2DClampToEdgeCaseTest(
	Context& context, const ExtParameters& extParams, const char* name, const char* description)
	: GPUShader5TextureGatherOffsetColor2DRepeatCaseTest(context, extParams, name, description)
{
	/* Nothing to be done here */
}

/** Get wrap mode for texture
 *
 *  @param out_wrap_mode Wrap mode
 **/
void GPUShader5TextureGatherOffsetColor2DClampToEdgeCaseTest::getTextureWrapMode(glw::GLenum& out_wrap_mode)
{
	out_wrap_mode = GL_CLAMP_TO_EDGE;
}

/** Get parts of vertex shader
 *
 *  @param out_vertex_shader_parts Vector of vertex shader parts
 **/
void GPUShader5TextureGatherOffsetColor2DClampToEdgeCaseTest::getShaderParts(
	std::vector<const glw::GLchar*>& out_vertex_shader_parts)
{
	out_vertex_shader_parts.push_back(m_vertex_shader_code);
}

/** Clamps value to specified range
 *
 * @param value Value to be clamped
 * @param min   Minimum of range
 * @param max   Maximum of range
 **/
template <typename T>
void clamp(T& value, const T min, const T max)
{
	if (min > value)
	{
		value = min;
	}
	else if (max < value)
	{
		value = max;
	}
}

/** Check if data captured for vertex at index are as expected
 *
 *  @param captured_data Instance of capturedVaryings for vertex at index
 *  @param index         Index of vertex
 *  @param texture_size Size of texture
 *
 *  @return True  Results match expected values
 *          False Results do not match expected values
 **/
bool GPUShader5TextureGatherOffsetColor2DClampToEdgeCaseTest::checkResult(const CapturedVaryings& captured_data,
																		  unsigned int index, unsigned int texture_size)
{
	/* Get offsets for vertex */
	glw::GLint x_offset;
	glw::GLint y_offset;

	getOffsets(x_offset, y_offset, index);

	/* Storage for expected values of with_offset varyings */
	glw::GLint expected_values_0[m_n_components_per_varying];
	glw::GLint expected_values_1[m_n_components_per_varying];
	glw::GLint expected_values_2[m_n_components_per_varying];
	glw::GLint expected_values_3[m_n_components_per_varying];

	/* Index of first not clamped texel in without_offset set*/
	int not_clamped_texel_index = -1;

	/* X and Y offsets of texels in 2x2 set */
	static const glw::GLint gather_offsets_x[] = { 0, 1, 1, 0 };
	static const glw::GLint gather_offsets_y[] = { 1, 1, 0, 0 };

	/* Find first not clamped texel in without_offset set */
	for (unsigned int i = 0; i < m_n_components_per_varying; ++i)
	{
		if ((0 <= captured_data.without_offset_0[i]) && (0 <= captured_data.without_offset_1[i]))
		{
			not_clamped_texel_index = i;
			break;
		}
	}

	if (-1 == not_clamped_texel_index)
	{
		/* Log problem with sampler */
		m_testCtx.getLog() << tcu::TestLog::Section("Unexpected sampling results", "");

		logCoordinates(index);

		m_testCtx.getLog() << tcu::TestLog::Message << "Texture size: " << texture_size << tcu::TestLog::EndMessage;

		m_testCtx.getLog() << tcu::TestLog::Message << "Offsets: " << x_offset << ", " << y_offset
						   << tcu::TestLog::EndMessage;

		logVaryings(captured_data);

		m_testCtx.getLog() << tcu::TestLog::EndSection;

		TCU_FAIL("Unexpected sampling results");
	}

	/* Fraction part of position for lower left texel without_offset set */
	const glw::GLint fract_position_x =
		captured_data.without_offset_0[not_clamped_texel_index] - gather_offsets_x[not_clamped_texel_index];
	const glw::GLint fract_position_y =
		captured_data.without_offset_1[not_clamped_texel_index] - gather_offsets_y[not_clamped_texel_index];

	/* Absolute position of lower left corner in without_offset set */
	const glw::GLint absolute_position_x = fract_position_x + captured_data.without_offset_2[0] * (int)texture_size;
	const glw::GLint absolute_position_y = fract_position_y + captured_data.without_offset_3[0] * (int)texture_size;

	/* Calculate absolute position for all texels in without_offset set */
	for (unsigned int i = 0; i < m_n_components_per_varying; ++i)
	{
		expected_values_0[i] = absolute_position_x + gather_offsets_x[i];
		expected_values_1[i] = absolute_position_y + gather_offsets_y[i];
		expected_values_2[i] = absolute_position_x + gather_offsets_x[i];
		expected_values_3[i] = absolute_position_y + gather_offsets_y[i];
	}

	/* Apply offsets to absolute positions of without_offset */
	for (unsigned int i = 0; i < m_n_components_per_varying; ++i)
	{
		expected_values_0[i] += x_offset;
		expected_values_1[i] += y_offset;
		expected_values_2[i] += x_offset;
		expected_values_3[i] += y_offset;

		/* Clamp value when point is outside of texture */
		clamp(expected_values_0[i], 0, ((glw::GLint)texture_size) - 1);
		clamp(expected_values_1[i], 0, ((glw::GLint)texture_size) - 1);
		clamp(expected_values_2[i], 0, ((glw::GLint)texture_size) - 1);
		clamp(expected_values_3[i], 0, ((glw::GLint)texture_size) - 1);
	}

	/* Verify that expected_values match those in with_offset varyings */
	for (unsigned int i = 0; i < m_n_components_per_varying; ++i)
	{
		if ((captured_data.with_offset_0[i] != expected_values_0[i]) ||
			(captured_data.with_offset_1[i] != expected_values_1[i]) ||
			(captured_data.with_offset_2[i] != expected_values_2[i]) ||
			(captured_data.with_offset_3[i] != expected_values_3[i]))
		{
			/* Log invalid results */
			m_testCtx.getLog() << tcu::TestLog::Section("Invalid result", "");

			logCoordinates(index);

			m_testCtx.getLog() << tcu::TestLog::Message << "Texture size: " << texture_size << tcu::TestLog::EndMessage;

			m_testCtx.getLog() << tcu::TestLog::Message << "Offsets: " << x_offset << ", " << y_offset
							   << tcu::TestLog::EndMessage;

			logVaryings(captured_data);

			m_testCtx.getLog() << tcu::TestLog::EndSection;

			/* Done */
			return false;
		}
	}

	/* Done */
	return true;
}

/** Constructor
 *
 * @param context       Test context
 * @param name          Test case's name
 * @param description   Test case's description
 **/
GPUShader5TextureGatherOffsetColor2DOffsetsCaseTest::GPUShader5TextureGatherOffsetColor2DOffsetsCaseTest(
	Context& context, const ExtParameters& extParams, const char* name, const char* description)
	: GPUShader5TextureGatherOffsetColorTestBase(context, extParams, name, description)
{
	/* Nothing to be done here */
}

/** Get parts of vertex shader
 *
 *  @param out_vertex_shader_parts Vector of vertex shader parts
 **/
void GPUShader5TextureGatherOffsetColor2DOffsetsCaseTest::getShaderParts(
	std::vector<const glw::GLchar*>& out_vertex_shader_parts)
{
	std::stringstream stream;

	stream << "           ivec2[4]( ivec2( " << m_min_texture_gather_offset << ", " << m_min_texture_gather_offset
		   << "),\n";
	stream << "                     ivec2( " << m_min_texture_gather_offset << ", " << m_max_texture_gather_offset
		   << "),\n";
	stream << "                     ivec2( " << m_max_texture_gather_offset << ", " << m_min_texture_gather_offset
		   << "),\n";
	stream << "                     ivec2( " << m_max_texture_gather_offset << ", " << m_max_texture_gather_offset
		   << "));\n";

	m_vertex_shader_code_offsets = stream.str();

	out_vertex_shader_parts.push_back(m_vertex_shader_code_preamble);
	out_vertex_shader_parts.push_back(m_vertex_shader_code_offsets.c_str());
	out_vertex_shader_parts.push_back(m_vertex_shader_code_body);
}

/** Prepare test specific data, that will be used as vertex attributes
 *
 *  @param vertex_buffer_infos Vector of vertexBufferInfo instances
 **/
void GPUShader5TextureGatherOffsetColor2DOffsetsCaseTest::prepareVertexBuffersData(
	std::vector<VertexBufferInfo>& vertex_buffer_infos)
{
	DE_UNREF(vertex_buffer_infos);

	/* Nothing to be done here */
}

/** Check if data captured for vertex at index are as expected
 *
 *  @param captured_data Instance of capturedVaryings for vertex at index
 *  @param index         Index of vertex
 *  @param texture_size  Size of texture
 *
 *  @return True  Results match expected values
 *          False Results do not match expected values
 **/
bool GPUShader5TextureGatherOffsetColor2DOffsetsCaseTest::checkResult(const CapturedVaryings& captured_data,
																	  unsigned int index, unsigned int tex_size)
{
	/* Signed int value has to be used with operator % in order to work as expected */
	const glw::GLint texture_size = (glw::GLint)tex_size;

	/* X and Y offsets */
	glw::GLint x_offsets[m_n_components_per_varying] = {
		m_min_texture_gather_offset, m_min_texture_gather_offset, m_max_texture_gather_offset,
		m_max_texture_gather_offset,
	};

	glw::GLint y_offsets[m_n_components_per_varying] = {
		m_min_texture_gather_offset, m_max_texture_gather_offset, m_min_texture_gather_offset,
		m_max_texture_gather_offset,
	};

	for (unsigned int i = 0; i < m_n_components_per_varying; ++i)
	{
		/* Roll x offsets around left edge when negative */
		if (0 > x_offsets[i])
		{
			x_offsets[i] = x_offsets[i] % texture_size;
			x_offsets[i] += texture_size;
		}

		/* Roll y offsets around top edge when negative */
		if (0 > y_offsets[i])
		{
			y_offsets[i] = y_offsets[i] % texture_size;
			y_offsets[i] += texture_size;
		}
	}

	/* Storage for expected values of with_offset varyings */
	glw::GLint expected_values_0[m_n_components_per_varying];
	glw::GLint expected_values_1[m_n_components_per_varying];
	glw::GLint expected_values_2[m_n_components_per_varying];
	glw::GLint expected_values_3[m_n_components_per_varying];

	/* Calculate expected values of with_offset varyings */
	for (unsigned int i = 0; i < m_n_components_per_varying; ++i)
	{
		expected_values_0[i] = ((x_offsets[i] + captured_data.without_offset_0[3]) % texture_size);
		expected_values_1[i] = ((y_offsets[i] + captured_data.without_offset_1[3]) % texture_size);
		expected_values_2[i] = ((x_offsets[i] + captured_data.without_offset_2[3]) % texture_size);
		expected_values_3[i] = ((y_offsets[i] + captured_data.without_offset_3[3]) % texture_size);
	}

	/* Verify that expected_values match those in with_offset varyings */
	for (unsigned int i = 0; i < m_n_components_per_varying; ++i)
	{
		if ((captured_data.with_offset_0[i] != expected_values_0[i]) ||
			(captured_data.with_offset_1[i] != expected_values_1[i]) ||
			(captured_data.with_offset_2[i] != expected_values_2[i]) ||
			(captured_data.with_offset_3[i] != expected_values_3[i]))
		{
			/* Log invalid results */
			m_testCtx.getLog() << tcu::TestLog::Section("Invalid result", "");

			logCoordinates(index);

			m_testCtx.getLog() << tcu::TestLog::Message << "Texture size: " << texture_size << tcu::TestLog::EndMessage;

			m_testCtx.getLog() << tcu::TestLog::Message << "Offsets: "
							   << "(" << x_offsets[0] << ", " << y_offsets[0] << ") "
							   << "(" << x_offsets[1] << ", " << y_offsets[1] << ") "
							   << "(" << x_offsets[2] << ", " << y_offsets[2] << ") "
							   << "(" << x_offsets[3] << ", " << y_offsets[3] << ") " << tcu::TestLog::EndMessage;

			logVaryings(captured_data);

			m_testCtx.getLog() << tcu::TestLog::EndSection;

			/* Done */
			return false;
		}
	}

	/* Done */
	return true;
}

/** Initializes GLES objects used during the test.
 *
 */
void GPUShader5TextureGatherOffsetColor2DClampToEdgeCaseTest::initTest(void)
{
	/* Check if texture_border_clamp extension is supported */
	if (!m_is_texture_border_clamp_supported)
	{
		throw tcu::NotSupportedError(TEXTURE_BORDER_CLAMP_NOT_SUPPORTED, "", __FILE__, __LINE__);
	}

	GPUShader5TextureGatherOffsetColor2DRepeatCaseTest::initTest();
}

/** Constructor
 *
 * @param context       Test context
 * @param name          Test case's name
 * @param description   Test case's description
 **/
GPUShader5TextureGatherOffsetDepthTestBase::GPUShader5TextureGatherOffsetDepthTestBase(Context&				context,
																					   const ExtParameters& extParams,
																					   const char*			name,
																					   const char*			description)
	: GPUShader5TextureGatherOffsetTestBase(context, extParams, name, description), m_n_varyings_per_vertex(0)
{
	/* Nothing to be done here */
}

/** Provides color that will be used as border color for "clamp to border" mode
 *
 * @param out_color Color
 **/
void GPUShader5TextureGatherOffsetDepthTestBase::getBorderColor(glw::GLfloat out_color[4])
{
	out_color[0] = 1.0f;
	out_color[1] = 1.0f;
	out_color[2] = 1.0f;
	out_color[3] = 1.0f;
}

/** Provides informations required to create texture
 *
 *  @param out_width                   Size of texture
 *  @param out_texture_internal_format Internal format of texture
 *  @param out_texture_format          Format of texture
 *  @param out_texture_type            Type of texture
 *  @param out_bytes_per_pixel         Bytes per pixel
 **/
void GPUShader5TextureGatherOffsetDepthTestBase::getTextureInfo(glw::GLuint& out_width,
																glw::GLenum& out_texture_internal_format,
																glw::GLenum& out_texture_format,
																glw::GLenum& out_texture_type,
																glw::GLuint& out_bytes_per_pixel)
{
	out_width					= m_texture_size;
	out_texture_internal_format = GL_DEPTH_COMPONENT32F;
	out_texture_format			= GL_DEPTH_COMPONENT;
	out_texture_type			= GL_FLOAT;
	out_bytes_per_pixel			= 1 * sizeof(glw::GLfloat); /* Depth * float */
}

/** Get wrap mode for texture
 *
 *  @param out_wrap_mode Wrap mode
 **/
void GPUShader5TextureGatherOffsetDepthTestBase::getTextureWrapMode(glw::GLenum& out_wrap_mode)
{
	out_wrap_mode = GL_REPEAT;
}

/** Get size of buffer used as output from transform feedback and names of varyings
 *
 *  @param out_buffer_size       Size of buffer
 *  @param out_captured_varyings Names of varyings
 **/
void GPUShader5TextureGatherOffsetDepthTestBase::getTransformFeedBackDetails(
	glw::GLuint& out_buffer_size, std::vector<const glw::GLchar*>& out_captured_varyings)
{
	getVaryings(out_captured_varyings);

	m_n_varyings_per_vertex = (unsigned int)out_captured_varyings.size();

	out_buffer_size = static_cast<glw::GLuint>(m_n_vertices * m_n_varyings_per_vertex * m_n_components_per_varying *
											   sizeof(glw::GLint));
}

/** Check if texture array is required
 *
 *  @param out_is_texture_array Set to true if texture array is required, false otherwise
 **/
void GPUShader5TextureGatherOffsetDepthTestBase::isTextureArray(bool& out_is_texture_array)
{
	out_is_texture_array = false;
}

/** Prepare texture data as expected by all cases in test 10.
 *  Each texel is assigned value of { x / texture_size }
 *
 *  @param data Storage space for texture data
 **/
void GPUShader5TextureGatherOffsetDepthTestBase::prepareTextureData(glw::GLubyte* data)
{
	/* Data pointer */
	glw::GLfloat* texture_data = (glw::GLfloat*)data;

	/* Constants */
	const unsigned int n_components_per_pixel = 1;
	const unsigned int line_size			  = n_components_per_pixel * m_texture_size;
	const glw::GLfloat step					  = 1.0f / ((float)m_texture_size);

	/* Prepare texture's data */
	for (unsigned int y = 0; y < m_texture_size; ++y)
	{
		for (unsigned int x = 0; x < m_texture_size; ++x)
		{
			const glw::GLfloat depth		= ((float)x) * step;
			const unsigned int pixel_offset = y * line_size + x * n_components_per_pixel;

			/* texel.depth = depth */
			texture_data[pixel_offset] = depth;
		}
	}
}

/** Verification of results
 *
 *  @param result Pointer to data mapped from transform feedback buffer.
 *                Size of data is equal to buffer_size set by getTransformFeedbackBufferSize
 *
 *  @return true  Result match expected value
 *          false Result has wrong value
 **/
bool GPUShader5TextureGatherOffsetDepthTestBase::verifyResult(const void* result_data)
{
	/* All data stored in data are ints */
	glw::GLint* results = (glw::GLint*)result_data;

	/* Verification result */
	bool result = true;

	/* Constants for data offsets calculation */
	const unsigned int bytes_per_varying			 = m_n_components_per_varying * sizeof(glw::GLfloat);
	const unsigned int vertex_size_in_results_buffer = m_n_varyings_per_vertex * m_n_components_per_varying;
	const unsigned int with_offset_offset			 = m_n_components_per_varying;
	const unsigned int without_offset_offset		 = 0;
	const unsigned int floor_tex_coord_offset		 = with_offset_offset + m_n_components_per_varying;

	/* For each vertex */
	for (unsigned int vertex = 0; vertex < m_n_vertices; ++vertex)
	{
		CapturedVaryings captured_data;

		/* Pointers to all varyings for given vertex */
		const glw::GLint* without_offset  = results + (vertex * vertex_size_in_results_buffer) + without_offset_offset;
		const glw::GLint* with_offset	 = results + (vertex * vertex_size_in_results_buffer) + with_offset_offset;
		const glw::GLint* floor_tex_coord = results + (vertex * vertex_size_in_results_buffer) + floor_tex_coord_offset;

		/* Copy captured varyings to captured_data instance */
		memcpy(captured_data.without_offset, without_offset, bytes_per_varying);
		memcpy(captured_data.with_offset, with_offset, bytes_per_varying);
		if (3 <= m_n_varyings_per_vertex)
		{
			memcpy(captured_data.floor_tex_coord, floor_tex_coord, bytes_per_varying);
		}

		/* Let child class check if result is ok */
		const bool is_result_ok = checkResult(captured_data, vertex, m_texture_size);

		/* Do not break on first error */
		if (false == is_result_ok)
		{
			result = false;
		}
	}

	/* Done */
	return result;
}

/** Provides names of varyings to be captured by test
 *
 * @param out_captured_varyings Vector of varyings' names
 **/
void GPUShader5TextureGatherOffsetDepthTestBase::getVaryings(std::vector<const glw::GLchar*>& out_captured_varyings)
{
	out_captured_varyings.push_back("without_offset");
	out_captured_varyings.push_back("with_offset");
}

/** Logs data captured varyings
 *
 *  @param varyings Instance of capturedVaryings to be logged
 **/
void GPUShader5TextureGatherOffsetDepthTestBase::logVaryings(const CapturedVaryings& varyings)
{
	logArray(varyings.without_offset, m_n_components_per_varying, "Without offset: ");

	logArray(varyings.with_offset, m_n_components_per_varying, "With offset: ");
}

/** Constructor
 *
 * @param context       Test context
 * @param name          Test case's name
 * @param description   Test case's description
 **/
GPUShader5TextureGatherOffsetDepth2DRepeatCaseTest::GPUShader5TextureGatherOffsetDepth2DRepeatCaseTest(
	Context& context, const ExtParameters& extParams, const char* name, const char* description)
	: GPUShader5TextureGatherOffsetDepthTestBase(context, extParams, name, description)
{
	/* Nothing to be done here */
}

/** Get parts of vertex shader
 *
 *  @param out_vertex_shader_parts Vector of vertex shader parts
 **/
void GPUShader5TextureGatherOffsetDepth2DRepeatCaseTest::getShaderParts(
	std::vector<const glw::GLchar*>& out_vertex_shader_parts)
{
	out_vertex_shader_parts.push_back(m_vertex_shader_code);
}

/** Prepare test specific data, that will be used as vertex attributes
 *
 *  @param vertex_buffer_infos Vector of vertexBufferInfo instances
 **/
void GPUShader5TextureGatherOffsetDepth2DRepeatCaseTest::prepareVertexBuffersData(
	std::vector<VertexBufferInfo>& vertex_buffer_infos)
{
	/* Constants for offsets and size of buffer */
	const unsigned int n_total_components = m_n_vertices * m_n_offsets_components;

	/* Limits used to generate offsets */
	const glw::GLint min_ptgo = m_min_texture_gather_offset;
	const glw::GLint max_ptgo = m_max_texture_gather_offset;

	/* Storage for offsets */
	m_offsets_buffer_data.resize(n_total_components);

	/* Generate offsets */
	for (unsigned int i = 0; i < n_total_components; ++i)
	{
		m_offsets_buffer_data[i] = (rand() % (max_ptgo - min_ptgo + 1)) + min_ptgo;
	}

	/* Setup offsets VB info */
	VertexBufferInfo info;

	info.attribute_name = m_offsets_attribute_name;
	info.n_components   = m_n_offsets_components;
	info.type			= GL_INT;
	info.data			= &m_offsets_buffer_data[0];
	info.data_size		= (glw::GLuint)(m_offsets_buffer_data.size() * sizeof(glw::GLint));

	/* Add VB info to vector */
	vertex_buffer_infos.push_back(info);
}

/** Check if data captured for vertex at index are as expected
 *
 *  @param captured_data Instance of capturedVaryings for vertex at index
 *  @param index         Index of vertex
 *  @param texture_size  Size of texture
 *
 *  @return True  Results match expected values
 *          False Results do not match expected values
 **/
bool GPUShader5TextureGatherOffsetDepth2DRepeatCaseTest::checkResult(const CapturedVaryings& captured_data,
																	 unsigned int index, unsigned int tex_size)
{
	/* Signed int value has to be used with operator % in order to work as expected */
	const glw::GLint texture_size = (glw::GLint)tex_size;

	/* Get offsets for vertex */
	glw::GLint x_offset;
	glw::GLint y_offset;

	getOffsets(x_offset, y_offset, index);

	/* Roll x offsets around left edge when negative */
	if (0 > x_offset)
	{
		x_offset = x_offset % texture_size;
		x_offset += texture_size;
	}

	/* Storage for expected values of with_offset varyings */
	glw::GLint expected_values[m_n_components_per_varying];

	/* Calculate expected values of with_offset varyings */
	for (unsigned int i = 0; i < m_n_components_per_varying; ++i)
	{
		expected_values[i] = ((x_offset + captured_data.without_offset[i]) % texture_size);
	}

	/* Verify that expected_values match those in with_offset varyings */
	for (unsigned int i = 0; i < m_n_components_per_varying; ++i)
	{
		if (captured_data.with_offset[i] != expected_values[i])
		{
			/* Log invalid results */
			m_testCtx.getLog() << tcu::TestLog::Section("Invalid result", "");

			logCoordinates(index);

			m_testCtx.getLog() << tcu::TestLog::Message << "Texture size: " << texture_size << tcu::TestLog::EndMessage;

			m_testCtx.getLog() << tcu::TestLog::Message << "Offsets: " << x_offset << ", " << y_offset
							   << tcu::TestLog::EndMessage;

			logVaryings(captured_data);

			m_testCtx.getLog() << tcu::TestLog::EndSection;

			/* Done */
			return false;
		}
	}

	/* Done */
	return true;
}

/** Get offsets for vertex at index
 *
 *  @param out_x_offset X offset
 *  @param out_y_offset Y offset
 *  @param index        Index of vertex
 **/
void GPUShader5TextureGatherOffsetDepth2DRepeatCaseTest::getOffsets(glw::GLint& out_x_offset, glw::GLint& out_y_offset,
																	unsigned int index)
{
	out_x_offset = m_offsets_buffer_data[index * m_n_offsets_components + 0];
	out_y_offset = m_offsets_buffer_data[index * m_n_offsets_components + 1];
}

/** Constructor
 *
 * @param context       Test context
 * @param name          Test case's name
 * @param description   Test case's description
 **/
GPUShader5TextureGatherOffsetDepth2DRepeatYCaseTest::GPUShader5TextureGatherOffsetDepth2DRepeatYCaseTest(
	Context& context, const ExtParameters& extParams, const char* name, const char* description)
	: GPUShader5TextureGatherOffsetDepth2DRepeatCaseTest(context, extParams, name, description)
{
	/* Nothing to be done here */
}

/** Get parts of vertex shader
 *
 *  @param out_vertex_shader_parts Vector of vertex shader parts
 **/
void GPUShader5TextureGatherOffsetDepth2DRepeatYCaseTest::getShaderParts(
	std::vector<const glw::GLchar*>& out_vertex_shader_parts)
{
	out_vertex_shader_parts.push_back(m_vertex_shader_code);
}

/** Prepare texture data as expected by case "Repeat the same test for Y axis." of test 10.
 *  Each texel is assigned value of { y / texture_size }
 *
 *  @param data Storage space for texture data
 **/
void GPUShader5TextureGatherOffsetDepth2DRepeatYCaseTest::prepareTextureData(glw::GLubyte* data)
{
	/* Data pointer */
	glw::GLfloat* texture_data = (glw::GLfloat*)data;

	/* Get texture info from GPUShader5TextureGatherOffsetDepthTestBase */
	glw::GLuint texture_size;
	glw::GLenum ignored_texture_internal_format;
	glw::GLenum ignored_texture_format;
	glw::GLenum ignored_texture_type;
	glw::GLuint ignored_bytes_per_pixel;

	getTextureInfo(texture_size, ignored_texture_internal_format, ignored_texture_format, ignored_texture_type,
				   ignored_bytes_per_pixel);

	/* Constants */
	const unsigned int n_components_per_pixel = 1;
	const unsigned int line_size			  = n_components_per_pixel * texture_size;
	const glw::GLfloat step					  = 1.0f / ((float)texture_size);

	/* Prepare texture's data */
	for (unsigned int y = 0; y < texture_size; ++y)
	{
		const glw::GLfloat depth = ((float)y) * step;

		for (unsigned int x = 0; x < texture_size; ++x)
		{
			const unsigned int pixel_offset = y * line_size + x * n_components_per_pixel;

			/* texel.depth = depth */
			texture_data[pixel_offset] = depth;
		}
	}
}

/** Check if data captured for vertex at index are as expected
 *
 *  @param captured_data Instance of capturedVaryings for vertex at index
 *  @param index         Index of vertex
 *  @param texture_size  Size of texture
 *
 *  @return True  Results match expected values
 *          False Results do not match expected values
 **/
bool GPUShader5TextureGatherOffsetDepth2DRepeatYCaseTest::checkResult(const CapturedVaryings& captured_data,
																	  unsigned int index, unsigned int tex_size)
{
	/* Signed int value has to be used with operator % in order to work as expected */
	const glw::GLint texture_size = (glw::GLint)tex_size;

	/* Get offsets for vertex */
	glw::GLint x_offset;
	glw::GLint y_offset;

	getOffsets(x_offset, y_offset, index);

	/* Roll y offsets around left edge when negative */
	if (0 > y_offset)
	{
		y_offset = y_offset % texture_size;
		y_offset += texture_size;
	}

	/* Storage for expected values of with_offset varyings */
	glw::GLint expected_values[m_n_components_per_varying];

	/* Calculate expected values of with_offset varyings */
	for (unsigned int i = 0; i < m_n_components_per_varying; ++i)
	{
		expected_values[i] = ((y_offset + captured_data.without_offset[i]) % texture_size);
	}

	/* Verify that expected_values match those in with_offset varyings */
	for (unsigned int i = 0; i < m_n_components_per_varying; ++i)
	{
		if (captured_data.with_offset[i] != expected_values[i])
		{
			/* Log invalid results */
			m_testCtx.getLog() << tcu::TestLog::Section("Invalid result", "");

			logCoordinates(index);

			m_testCtx.getLog() << tcu::TestLog::Message << "Texture size: " << texture_size << tcu::TestLog::EndMessage;

			m_testCtx.getLog() << tcu::TestLog::Message << "Offsets: " << x_offset << ", " << y_offset
							   << tcu::TestLog::EndMessage;

			logVaryings(captured_data);

			m_testCtx.getLog() << tcu::TestLog::EndSection;

			/* Done */
			return false;
		}
	}

	/* Done */
	return true;
}

/** Constructor
 *
 * @param context       Test context
 * @param name          Test case's name
 * @param description   Test case's description
 **/
GPUShader5TextureGatherOffsetDepth2DArrayCaseTest::GPUShader5TextureGatherOffsetDepth2DArrayCaseTest(
	Context& context, const ExtParameters& extParams, const char* name, const char* description)
	: GPUShader5TextureGatherOffsetDepth2DRepeatCaseTest(context, extParams, name, description)
{
	/* Nothing to be done here */
}

/** Get parts of vertex shader
 *
 *  @param out_vertex_shader_parts Vector of vertex shader parts
 **/
void GPUShader5TextureGatherOffsetDepth2DArrayCaseTest::getShaderParts(
	std::vector<const glw::GLchar*>& out_vertex_shader_parts)
{
	out_vertex_shader_parts.push_back(m_vertex_shader_code);
}

/** Check if texture array is required
 *
 *  @param out_is_texture_array Set to true if texture array is required, false otherwise
 **/
void GPUShader5TextureGatherOffsetDepth2DArrayCaseTest::isTextureArray(bool& out_is_texture_array)
{
	out_is_texture_array = true;
}

/** Constructor
 *
 * @param context       Test context
 * @param name          Test case's name
 * @param description   Test case's description
 **/
GPUShader5TextureGatherOffsetDepth2DClampToBorderCaseTest::GPUShader5TextureGatherOffsetDepth2DClampToBorderCaseTest(
	Context& context, const ExtParameters& extParams, const char* name, const char* description)
	: GPUShader5TextureGatherOffsetDepth2DRepeatCaseTest(context, extParams, name, description)
{
	/* Nothing to be done here */
}

/** Get wrap mode for texture
 *
 *  @param out_wrap_mode Wrap mode
 **/
void GPUShader5TextureGatherOffsetDepth2DClampToBorderCaseTest::getTextureWrapMode(glw::GLenum& out_wrap_mode)
{
	out_wrap_mode = m_glExtTokens.CLAMP_TO_BORDER;
}

/** Provides names of varyings to be captured by test
 *
 * @param out_captured_varyings Vector of varyings' names
 **/
void GPUShader5TextureGatherOffsetDepth2DClampToBorderCaseTest::getVaryings(
	std::vector<const glw::GLchar*>& out_captured_varyings)
{
	out_captured_varyings.push_back("without_offset");
	out_captured_varyings.push_back("with_offset");
	out_captured_varyings.push_back("floor_tex_coord");
}

/** Get parts of vertex shader
 *
 *  @param out_vertex_shader_parts Vector of vertex shader parts
 **/
void GPUShader5TextureGatherOffsetDepth2DClampToBorderCaseTest::getShaderParts(
	std::vector<const glw::GLchar*>& out_vertex_shader_parts)
{
	out_vertex_shader_parts.push_back(m_vertex_shader_code);
}

/** Prepare test specific data, that will be used as vertex attributes
 *
 *  @param vertex_buffer_infos Vector of vertexBufferInfo instances
 **/
void GPUShader5TextureGatherOffsetDepth2DClampToBorderCaseTest::prepareVertexBuffersData(
	std::vector<VertexBufferInfo>& vertex_buffer_infos)
{
	/* Constats for offsets and size of buffer */
	const unsigned int n_total_components = m_n_vertices * m_n_offsets_components;

	/* Limits used to generate offsets */
	const glw::GLint min_ptgo = m_min_texture_gather_offset;
	const glw::GLint max_ptgo = m_max_texture_gather_offset;

	/* Storage for offsets */
	m_offsets_buffer_data.resize(n_total_components);

	/* Generate offsets */
	for (unsigned int i = 0; i < n_total_components; ++i)
	{
		if (0 == i % 2)
		{
			m_offsets_buffer_data[i] = (rand() % (max_ptgo - min_ptgo + 1)) + min_ptgo;
		}
		else
		{
			/* y offsets must be set to 0*/
			m_offsets_buffer_data[i] = 0;
		}
	}

	/* Setup offsets VB info */
	VertexBufferInfo info;

	info.attribute_name = m_offsets_attribute_name;
	info.n_components   = m_n_offsets_components;
	info.type			= GL_INT;
	info.data			= &m_offsets_buffer_data[0];
	info.data_size		= (glw::GLuint)(m_offsets_buffer_data.size() * sizeof(glw::GLint));

	/* Add VB info to vector */
	vertex_buffer_infos.push_back(info);
}

/** Check if data captured for vertex at index are as expected
 *
 *  @param captured_data Instance of capturedVaryings for vertex at index
 *  @param index         Index of vertex
 *  @param texture_size  Size of texture
 *
 *  @return True  Results match expected values
 *          False Results do not match expected values
 **/
bool GPUShader5TextureGatherOffsetDepth2DClampToBorderCaseTest::checkResult(const CapturedVaryings& captured_data,
																			unsigned int			index,
																			unsigned int			texture_size)
{
	/* Get offsets for vertex */
	glw::GLint x_offset;
	glw::GLint y_offset;

	getOffsets(x_offset, y_offset, index);

	/* Storage for expected values of with_offset varyings */
	glw::GLint expected_values[m_n_components_per_varying];

	/* Index of first not clamped texel in without_offset set*/
	int not_clamped_texel_index = -1;

	/* X offsets of texels in 2x2 set */
	static const glw::GLint gather_offsets_x[] = { 0, 1, 1, 0 };

	/* Find first not clamped texel in without_offset set */
	for (unsigned int i = 0; i < m_n_components_per_varying; ++i)
	{
		if ((int)texture_size != captured_data.without_offset[i])
		{
			not_clamped_texel_index = i;
			break;
		}
	}

	if (-1 == not_clamped_texel_index)
	{
		/* Log problem with sampler */
		m_testCtx.getLog() << tcu::TestLog::Section("Unexpected sampling results", "");

		logCoordinates(index);

		m_testCtx.getLog() << tcu::TestLog::Message << "Texture size: " << texture_size << tcu::TestLog::EndMessage;

		m_testCtx.getLog() << tcu::TestLog::Message << "Offsets: " << x_offset << ", " << y_offset
						   << tcu::TestLog::EndMessage;

		logVaryings(captured_data);

		m_testCtx.getLog() << tcu::TestLog::EndSection;

		TCU_FAIL("Unexpected sampling results");
	}

	/* Fraction part of position for lower left texel without_offset set */
	const glw::GLint fract_position_x =
		captured_data.without_offset[not_clamped_texel_index] - gather_offsets_x[not_clamped_texel_index];

	/* Absolute position of lower left corner in without_offset set */
	const glw::GLint absolute_position_x = fract_position_x + captured_data.floor_tex_coord[0] * (int)texture_size;

	/* Wraping matrix */
	glw::GLint wrap_matrix[4] = { 0 };

	/* Index of value in without_offset set, that should be check for being clamped */
	glw::GLint wrap_value_to_check_indices[4] = { 3, 2, 1, 0 };

	/* Get value from without_offset to check for clamping */
	glw::GLint wrap_value_to_check = captured_data.without_offset[wrap_value_to_check_indices[not_clamped_texel_index]];

	/* Setup wrap_matrix */
	switch (not_clamped_texel_index)
	{
	case 0:
	case 1:
		wrap_matrix[0] = 0;
		wrap_matrix[1] = 0;

		if ((glw::GLint)texture_size == wrap_value_to_check)
		{
			wrap_matrix[2] = -1;
			wrap_matrix[3] = -1;
		}
		break;
	case 2:
	case 3:
		wrap_matrix[2] = 0;
		wrap_matrix[3] = 0;

		if ((glw::GLint)texture_size == wrap_value_to_check)
		{
			wrap_matrix[0] = 1;
			wrap_matrix[1] = 1;
		}
		break;
	};

	/* Apply integral part of texture coordinates to wrap_matrix */
	for (unsigned int i = 0; i < m_n_components_per_varying; ++i)
	{
		wrap_matrix[i] += captured_data.floor_tex_coord[1];
	}

	/* For each texel in without_offset set calculate absolute position and apply offset */
	for (unsigned int i = 0; i < m_n_components_per_varying; ++i)
	{
		expected_values[i] = absolute_position_x + gather_offsets_x[i];
		expected_values[i] += x_offset;

		/* Set to texture_size when point is outside of texture */
		if ((0 > expected_values[i]) || (((glw::GLint)texture_size) < expected_values[i]) || (0 != wrap_matrix[i]))
		{
			expected_values[i] = texture_size;
		}
	}

	/* Verify that expected_values match those in with_offset varyings */
	for (unsigned int i = 0; i < m_n_components_per_varying; ++i)
	{
		if (captured_data.with_offset[i] != expected_values[i])
		{
			/* Log invalid results */
			m_testCtx.getLog() << tcu::TestLog::Section("Invalid result", "");

			logCoordinates(index);

			m_testCtx.getLog() << tcu::TestLog::Message << "Texture size: " << texture_size << tcu::TestLog::EndMessage;

			m_testCtx.getLog() << tcu::TestLog::Message << "Offsets: " << x_offset << ", " << y_offset
							   << tcu::TestLog::EndMessage;

			logVaryings(captured_data);

			m_testCtx.getLog() << tcu::TestLog::EndSection;

			/* Done */
			return false;
		}
	}

	/* Done */
	return true;
}

/** Initializes GLES objects used during the test.
 *
 */
void GPUShader5TextureGatherOffsetDepth2DClampToBorderCaseTest::initTest(void)
{
	/* Check if texture_border_clamp extension is supported */
	if (!m_is_texture_border_clamp_supported)
	{
		throw tcu::NotSupportedError(TEXTURE_BORDER_CLAMP_NOT_SUPPORTED, "", __FILE__, __LINE__);
	}

	GPUShader5TextureGatherOffsetDepth2DRepeatCaseTest::initTest();
}

/** Constructor
 *
 * @param context       Test context
 * @param name          Test case's name
 * @param description   Test case's description
 **/
GPUShader5TextureGatherOffsetDepth2DClampToEdgeCaseTest::GPUShader5TextureGatherOffsetDepth2DClampToEdgeCaseTest(
	Context& context, const ExtParameters& extParams, const char* name, const char* description)
	: GPUShader5TextureGatherOffsetDepth2DRepeatCaseTest(context, extParams, name, description)
{
	/* Nothing to be done here */
}

/** Get wrap mode for texture
 *
 *  @param out_wrap_mode Wrap mode
 **/
void GPUShader5TextureGatherOffsetDepth2DClampToEdgeCaseTest::getTextureWrapMode(glw::GLenum& out_wrap_mode)
{
	out_wrap_mode = GL_CLAMP_TO_EDGE;
}

/** Get parts of vertex shader
 *
 *  @param out_vertex_shader_parts Vector of vertex shader parts
 **/
void GPUShader5TextureGatherOffsetDepth2DClampToEdgeCaseTest::getShaderParts(
	std::vector<const glw::GLchar*>& out_vertex_shader_parts)
{
	out_vertex_shader_parts.push_back(m_vertex_shader_code);
}

/** Check if data captured for vertex at index are as expected
 *
 *  @param captured_data Instance of capturedVaryings for vertex at index
 *  @param index         Index of vertex
 *  @param texture_size  Size of texture
 *
 *  @return True  Results match expected values
 *          False Results do not match expected values
 **/
bool GPUShader5TextureGatherOffsetDepth2DClampToEdgeCaseTest::checkResult(const CapturedVaryings& captured_data,
																		  unsigned int index, unsigned int texture_size)
{
	/* Get offsets for vertex */
	glw::GLint x_offset;
	glw::GLint y_offset;

	getOffsets(x_offset, y_offset, index);

	/* Storage for expected values of with_offset varyings */
	glw::GLint expected_values[m_n_components_per_varying];

	/* Index of first not clamped texel in without_offset set*/
	int not_clamped_texel_index = -1;

	/* X offsets of texels in 2x2 set */
	static const glw::GLint gather_offsets_x[] = { 0, 1, 1, 0 };

	/* Find first not clamped texel in without_offset set */
	for (unsigned int i = 0; i < m_n_components_per_varying; ++i)
	{
		if ((int)texture_size != captured_data.without_offset[i])
		{
			not_clamped_texel_index = i;
			break;
		}
	}

	if (-1 == not_clamped_texel_index)
	{
		/* Log problem with sampler */
		m_testCtx.getLog() << tcu::TestLog::Section("Unexpected sampling results", "");

		logCoordinates(index);

		m_testCtx.getLog() << tcu::TestLog::Message << "Texture size: " << texture_size << tcu::TestLog::EndMessage;

		m_testCtx.getLog() << tcu::TestLog::Message << "Offsets: " << x_offset << ", " << y_offset
						   << tcu::TestLog::EndMessage;

		logVaryings(captured_data);

		m_testCtx.getLog() << tcu::TestLog::EndSection;

		TCU_FAIL("Unexpected sampling results");
	}

	/* Fraction part of position for lower left texel without_offset set */
	const glw::GLint fract_position_x =
		captured_data.without_offset[not_clamped_texel_index] - gather_offsets_x[not_clamped_texel_index];

	/* Absolute position of lower left corner in without_offset set */
	const glw::GLint absolute_position_x = fract_position_x + captured_data.floor_tex_coord[0] * (int)texture_size;

	/* Calculate expected values of with_offset varyings */
	for (unsigned int i = 0; i < m_n_components_per_varying; ++i)
	{
		expected_values[i] = absolute_position_x + gather_offsets_x[i];
		expected_values[i] += x_offset;

		/* Clamp when point is outside of texture */
		clamp(expected_values[i], 0, ((glw::GLint)texture_size) - 1);
	}

	/* Verify that expected_values match those in with_offset varyings */
	for (unsigned int i = 0; i < m_n_components_per_varying; ++i)
	{
		if (captured_data.with_offset[i] != expected_values[i])
		{
			/* Log invalid results */
			m_testCtx.getLog() << tcu::TestLog::Section("Invalid result", "");

			logCoordinates(index);

			m_testCtx.getLog() << tcu::TestLog::Message << "Texture size: " << texture_size << tcu::TestLog::EndMessage;

			m_testCtx.getLog() << tcu::TestLog::Message << "Offsets: " << x_offset << ", " << y_offset
							   << tcu::TestLog::EndMessage;

			logVaryings(captured_data);

			m_testCtx.getLog() << tcu::TestLog::EndSection;

			/* Done */
			return false;
		}
	}

	/* Done */
	return true;
}

/** Provides names of varyings to be captured by test
 *
 * @param out_captured_varyings Vector of varyings' names
 **/
void GPUShader5TextureGatherOffsetDepth2DClampToEdgeCaseTest::getVaryings(
	std::vector<const glw::GLchar*>& out_captured_varyings)
{
	out_captured_varyings.push_back("without_offset");
	out_captured_varyings.push_back("with_offset");
	out_captured_varyings.push_back("floor_tex_coord");
}

/** Initializes GLES objects used during the test.
 *
 */
void GPUShader5TextureGatherOffsetDepth2DClampToEdgeCaseTest::initTest(void)
{
	/* Check if texture_border_clamp extension is supported */
	if (!m_is_texture_border_clamp_supported)
	{
		throw tcu::NotSupportedError(TEXTURE_BORDER_CLAMP_NOT_SUPPORTED, "", __FILE__, __LINE__);
	}

	GPUShader5TextureGatherOffsetDepth2DRepeatCaseTest::initTest();
}

/** Constructor
 *
 * @param context       Test context
 * @param name          Test case's name
 * @param description   Test case's description
 **/
GPUShader5TextureGatherOffsetDepth2DOffsetsCaseTest::GPUShader5TextureGatherOffsetDepth2DOffsetsCaseTest(
	Context& context, const ExtParameters& extParams, const char* name, const char* description)
	: GPUShader5TextureGatherOffsetDepthTestBase(context, extParams, name, description)
{
	/* Nothing to be done here */
}

/** Get parts of vertex shader
 *
 *  @param out_vertex_shader_parts Vector of vertex shader parts
 **/
void GPUShader5TextureGatherOffsetDepth2DOffsetsCaseTest::getShaderParts(
	std::vector<const glw::GLchar*>& out_vertex_shader_parts)
{
	std::stringstream stream;

	stream << "           ivec2[4]( ivec2( " << m_min_texture_gather_offset << ", " << m_min_texture_gather_offset
		   << "),\n";
	stream << "                     ivec2( " << m_min_texture_gather_offset << ", " << m_max_texture_gather_offset
		   << "),\n";
	stream << "                     ivec2( " << m_max_texture_gather_offset << ", " << m_min_texture_gather_offset
		   << "),\n";
	stream << "                     ivec2( " << m_max_texture_gather_offset << ", " << m_max_texture_gather_offset
		   << "));\n";

	m_vertex_shader_code_offsets = stream.str();

	out_vertex_shader_parts.push_back(m_vertex_shader_code_preamble);
	out_vertex_shader_parts.push_back(m_vertex_shader_code_offsets.c_str());
	out_vertex_shader_parts.push_back(m_vertex_shader_code_body);
}

/** Prepare test specific data, that will be used as vertex attributes
 *
 *  @param vertex_buffer_infos Vector of vertexBufferInfo instances
 **/
void GPUShader5TextureGatherOffsetDepth2DOffsetsCaseTest::prepareVertexBuffersData(
	std::vector<VertexBufferInfo>& vertex_buffer_infos)
{
	DE_UNREF(vertex_buffer_infos);

	/* Nothing to be done here */
}

/** Check if data captured for vertex at index are as expected
 *
 *  @param captured_data Instance of capturedVaryings for vertex at index
 *  @param index         Index of vertex
 *  @param texture_size  Size of texture
 *
 *  @return True  Results match expected values
 *          False Results do not match expected values
 **/
bool GPUShader5TextureGatherOffsetDepth2DOffsetsCaseTest::checkResult(const CapturedVaryings& captured_data,
																	  unsigned int index, unsigned int tex_size)
{
	/* Signed int value has to be used with operator % in order to work as expected */
	const glw::GLint texture_size = (glw::GLint)tex_size;

	/* X and Y offsets */
	glw::GLint x_offsets[m_n_components_per_varying] = {
		m_min_texture_gather_offset, m_min_texture_gather_offset, m_max_texture_gather_offset,
		m_max_texture_gather_offset,
	};

	glw::GLint y_offsets[m_n_components_per_varying] = {
		m_min_texture_gather_offset, m_max_texture_gather_offset, m_min_texture_gather_offset,
		m_max_texture_gather_offset,
	};

	for (unsigned int i = 0; i < m_n_components_per_varying; ++i)
	{
		/* Roll x offsets around left edge when negative */
		if (0 > x_offsets[i])
		{
			x_offsets[i] = x_offsets[i] % texture_size;
			x_offsets[i] += texture_size;
		}
	}

	/* Storage for expected values of with_offset varyings */
	glw::GLint expected_values[m_n_components_per_varying];

	/* Calculate expected values of with_offset varyings */
	for (unsigned int i = 0; i < m_n_components_per_varying; ++i)
	{
		expected_values[i] = ((x_offsets[i] + captured_data.without_offset[3]) % texture_size);
	}

	/* Verify that expected_values match those in with_offset varyings */
	for (unsigned int i = 0; i < m_n_components_per_varying; ++i)
	{
		if (captured_data.with_offset[i] != expected_values[i])
		{
			/* Log invalid results */
			m_testCtx.getLog() << tcu::TestLog::Section("Invalid result", "");

			logCoordinates(index);

			m_testCtx.getLog() << tcu::TestLog::Message << "Texture size: " << texture_size << tcu::TestLog::EndMessage;

			m_testCtx.getLog() << tcu::TestLog::Message << "Offsets: "
							   << "(" << x_offsets[0] << ", " << y_offsets[0] << ") "
							   << "(" << x_offsets[1] << ", " << y_offsets[1] << ") "
							   << "(" << x_offsets[2] << ", " << y_offsets[2] << ") "
							   << "(" << x_offsets[3] << ", " << y_offsets[3] << ") " << tcu::TestLog::EndMessage;

			logVaryings(captured_data);

			m_testCtx.getLog() << tcu::TestLog::EndSection;

			/* Done */
			return false;
		}
	}

	/* Done */
	return true;
}

} /* glcts */
