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
 * \file esextcGeometryShaderLimits.cpp
 * \brief Geometry Shader Limits (Test Group 16)
 */ /*-------------------------------------------------------------------*/

#include "esextcGeometryShaderLimits.hpp"

#include "gluContextInfo.hpp"
#include "glwEnums.hpp"
#include "glwFunctions.hpp"
#include "tcuTestLog.hpp"

#include <cstring>
#include <sstream>
#include <string>

namespace glcts
{
/* Vertex Shader for GeometryShaderMaxUniformComponentsTest */
const glw::GLchar* const GeometryShaderMaxUniformComponentsTest::m_vertex_shader_code =
	"${VERSION}\n"
	"\n"
	"${GEOMETRY_SHADER_REQUIRE}\n"
	"\n"
	"void main()\n"
	"{\n"
	"    gl_Position = vec4(gl_VertexID, 0, 0, 1);\n"
	"}\n";

/* Geometry Shader parts for GeometryShaderMaxUniformComponentsTest */
const glw::GLchar* const GeometryShaderMaxUniformComponentsTest::m_geometry_shader_code_preamble =
	"${VERSION}\n"
	"\n"
	"${GEOMETRY_SHADER_REQUIRE}\n"
	"\n"
	"layout(points)                 in;\n"
	"layout(points, max_vertices=1) out;\n"
	"\n"
	"// definition of NUMBER_OF_UNIFORMS goes here\n";

const glw::GLchar* const GeometryShaderMaxUniformComponentsTest::m_geometry_shader_code_number_of_uniforms =
	"#define NUMBER_OF_UNIFORMS ";

const glw::GLchar* const GeometryShaderMaxUniformComponentsTest::m_geometry_shader_code_body =
	"u\n"
	"\n"
	"uniform ivec4 uni_array[NUMBER_OF_UNIFORMS];\n"
	"\n"
	"flat out uint gs_out_sum;\n"
	"\n"
	"void main()\n"
	"{\n"
	"    gs_out_sum = 0u;\n"
	"\n"
	"    for (uint i = 0u; i < NUMBER_OF_UNIFORMS; ++i)\n"
	"    {\n"
	"        gs_out_sum += uint(uni_array[i].x);\n"
	"        gs_out_sum += uint(uni_array[i].y);\n"
	"        gs_out_sum += uint(uni_array[i].z);\n"
	"        gs_out_sum += uint(uni_array[i].w);\n"
	"    }\n"
	"    EmitVertex();\n"
	"    \n"
	"    EndPrimitive();\n"
	"}\n";

/* Fragment Shader for GeometryShaderMaxUniformComponentsTest */
const glw::GLchar* const GeometryShaderMaxUniformComponentsTest::m_fragment_shader_code =
	"${VERSION}\n"
	"${GEOMETRY_SHADER_REQUIRE}\n"
	"precision mediump float;\n"
	"out vec4 fs_out_color;\n"
	"void main()\n"
	"{\n"
	"    fs_out_color = vec4(1, 1, 1, 1);\n"
	"}\n";

/** ***************************************************************************************************** **/
/* Vertex Shader for GeometryShaderMaxUniformBlocksTest */
const glw::GLchar* const GeometryShaderMaxUniformBlocksTest::m_vertex_shader_code =
	"${VERSION}\n"
	"\n"
	"${GEOMETRY_SHADER_REQUIRE}\n"
	"\n"
	"void main()\n"
	"{\n"
	"    gl_Position = vec4(gl_VertexID, 0, 0, 1);\n"
	"}\n";

/* Geometry Shader Parts for GeometryShaderMaxUniformBlocksTest */
const glw::GLchar* const GeometryShaderMaxUniformBlocksTest::m_geometry_shader_code_preamble =
	"${VERSION}\n"
	"\n"
	"${GEOMETRY_SHADER_REQUIRE}\n"
	"\n"
	"layout(points)                 in;\n"
	"layout(points, max_vertices=1) out;\n"
	"\n"
	"// definition of NUMBER_OF_UNIFORMS goes here\n";

const glw::GLchar* const GeometryShaderMaxUniformBlocksTest::m_geometry_shader_code_number_of_uniforms =
	"#define NUMBER_OF_UNIFORM_BLOCKS ";

const glw::GLchar* const GeometryShaderMaxUniformBlocksTest::m_geometry_shader_code_body_str =
	"u\n"
	"\n"
	"layout(binding = 0) uniform UniformBlock\n"
	"{\n"
	"    int entry;\n"
	"} uni_block_array[NUMBER_OF_UNIFORM_BLOCKS];\n"
	"\n"
	"flat out int gs_out_sum;\n"
	"\n"
	"void main()\n"
	"{\n"
	"    gs_out_sum = 0;\n"
	"\n";

const glw::GLchar* const GeometryShaderMaxUniformBlocksTest::m_geometry_shader_code_body_end = "\n"
																							   "    EmitVertex();\n"
																							   "\n"
																							   "    EndPrimitive();\n"
																							   "}\n";

/* Fragment Shader for GeometryShaderMaxUniformBlocksTest */
const glw::GLchar* const GeometryShaderMaxUniformBlocksTest::m_fragment_shader_code =
	"${VERSION}\n"
	"${GEOMETRY_SHADER_REQUIRE}\n"
	"precision mediump float;\n"
	"out vec4 fs_out_color;\n"
	"void main()\n"
	"{\n"
	"    fs_out_color = vec4(1, 1, 1, 1);\n"
	"}\n";

/** ****************************************************************************************** **/
/* Vertex Shader for GeometryShaderMaxInputComponentsTest */
const glw::GLchar* const GeometryShaderMaxInputComponentsTest::m_vertex_shader_code_preamble =
	"${VERSION}\n"
	"\n"
	"${GEOMETRY_SHADER_REQUIRE}\n"
	"\n"
	"// definition of NUMBER_OF_GEOMETRY_INPUT_VECTORS\n";

const glw::GLchar* const GeometryShaderMaxInputComponentsTest::m_vertex_shader_code_number_of_uniforms =
	"#define NUMBER_OF_GEOMETRY_INPUT_VECTORS ";

const glw::GLchar* const GeometryShaderMaxInputComponentsTest::m_vertex_shader_code_body =
	"u\n"
	"\n"
	"out Vertex\n"
	"{"
	"   flat out ivec4 vs_gs_out[NUMBER_OF_GEOMETRY_INPUT_VECTORS];\n"
	"};\n"
	"\n"
	"void main()\n"
	"{\n"
	"   int index = 1;\n"
	"\n"
	"   for (uint i = 0u; i < NUMBER_OF_GEOMETRY_INPUT_VECTORS; ++i)\n"
	"   {\n"
	"       vs_gs_out[i] = ivec4(index, index + 1, index + 2, index + 3);\n"
	"       index       += 4;\n"
	"   }\n"
	"}\n";

/* Geometry Shader Parts for GeometryShaderMaxInputComponentsTest */
const glw::GLchar* const GeometryShaderMaxInputComponentsTest::m_geometry_shader_code_preamble =
	"${VERSION}\n"
	"\n"
	"${GEOMETRY_SHADER_REQUIRE}\n"
	"\n"
	"layout(points)                 in;\n"
	"layout(points, max_vertices=1) out;\n"
	"\n"
	"// definition of NUMBER_OF_GEOMETRY_INPUT_VECTORS goes here\n";

const glw::GLchar* const GeometryShaderMaxInputComponentsTest::m_geometry_shader_code_number_of_uniforms =
	"#define NUMBER_OF_GEOMETRY_INPUT_VECTORS ";

const glw::GLchar* const GeometryShaderMaxInputComponentsTest::m_geometry_shader_code_body =
	"u\n"
	"\n"
	"in Vertex\n"
	"{\n"
	"    flat in ivec4 vs_gs_out[NUMBER_OF_GEOMETRY_INPUT_VECTORS];\n"
	"} vertex[1];\n"
	"\n"
	"flat out int gs_out_sum;\n"
	"\n"
	"void main()\n"
	"{\n"
	"    gs_out_sum = 0;\n"
	"\n"
	"    for (uint i = 0u; i < NUMBER_OF_GEOMETRY_INPUT_VECTORS; ++i)\n"
	"    {\n"
	"        gs_out_sum += vertex[0].vs_gs_out[i].x;\n"
	"        gs_out_sum += vertex[0].vs_gs_out[i].y;\n"
	"        gs_out_sum += vertex[0].vs_gs_out[i].z;\n"
	"        gs_out_sum += vertex[0].vs_gs_out[i].w;\n"
	"    }\n"
	"    EmitVertex();\n"
	"    \n"
	"    EndPrimitive();\n"
	"}\n";

/* Fragment Shader for GeometryShaderMaxInputComponentsTest */
const glw::GLchar* const GeometryShaderMaxInputComponentsTest::m_fragment_shader_code =
	"${VERSION}\n"
	"${GEOMETRY_SHADER_REQUIRE}\n"
	"precision mediump float;\n"
	"out vec4 fs_out_color;\n"
	"void main()\n"
	"{\n"
	"    fs_out_color = vec4(1, 1, 1, 1);\n"
	"}\n";

/** **************************************************************************************************/
/* Common shader parts for GeometryShaderMaxOutputComponentsTest */
const glw::GLchar* const GeometryShaderMaxOutputComponentsTest::m_common_shader_code_gs_fs_out = "gs_fs_out_";

const glw::GLchar* const GeometryShaderMaxOutputComponentsTest::m_common_shader_code_number_of_points =
	"#define NUMBER_OF_POINTS ";

const glw::GLchar* const GeometryShaderMaxOutputComponentsTest::m_common_shader_code_gs_fs_out_definitions =
	"// definitions of gs_fs_out_ varyings go here\n";

/* Vertex Shader for GeometryShaderMaxOutputComponentsTest */
const glw::GLchar* const GeometryShaderMaxOutputComponentsTest::m_vertex_shader_code =
	"${VERSION}\n"
	"\n"
	"${GEOMETRY_SHADER_REQUIRE}\n"
	"\n"
	"void main()\n"
	"{\n"
	"    gl_Position = vec4(gl_VertexID, 0, 0, 1);\n"
	"}\n";

/* Geometry Shader Parts for GeometryShaderMaxOutputComponentsTest */
const glw::GLchar* const GeometryShaderMaxOutputComponentsTest::m_geometry_shader_code_preamble =
	"${VERSION}\n"
	"\n"
	"${GEOMETRY_SHADER_REQUIRE}\n"
	"${GEOMETRY_POINT_SIZE_ENABLE}\n"
	"\n"
	"// definition of NUMBER_OF_POINTS goes here\n";

const glw::GLchar* const GeometryShaderMaxOutputComponentsTest::m_geometry_shader_code_layout =
	"u\n"
	"\n"
	"layout(points)                                in;\n"
	"layout(points, max_vertices=NUMBER_OF_POINTS) out;\n"
	"\n";

const glw::GLchar* const GeometryShaderMaxOutputComponentsTest::m_geometry_shader_code_flat_out_ivec4 =
	"flat out ivec4";

const glw::GLchar* const GeometryShaderMaxOutputComponentsTest::m_geometry_shader_code_assignment =
	" = ivec4(index++, index++, index++, index++);\n";

const glw::GLchar* const GeometryShaderMaxOutputComponentsTest::m_geometry_shader_code_body_begin =
	"\n"
	"void main()\n"
	"{\n"
	"    int index = 1;\n"
	"\n"
	"    for (uint point = 0u; point < NUMBER_OF_POINTS; ++point)\n"
	"    {\n"
	"        // gs_fs_out assignments go here\n";

const glw::GLchar* const GeometryShaderMaxOutputComponentsTest::m_geometry_shader_code_body_end =
	"\n"
	"        gl_PointSize = 2.0;\n"
	"        gl_Position = vec4(-1.0 + ((2.0 / float(NUMBER_OF_POINTS)) * float(point)) + (1.0 / "
	"float(NUMBER_OF_POINTS)), 0.0, 0.0, 1.0);\n"
	"\n"
	"        EmitVertex();\n"
	"        EndPrimitive();\n"
	"    }\n"
	"}\n";

/* Fragment Shader for GeometryShaderMaxOutputComponentsTest */
const glw::GLchar* const GeometryShaderMaxOutputComponentsTest::m_fragment_shader_code_preamble =
	"${VERSION}\n"
	"\n"
	"${GEOMETRY_SHADER_REQUIRE}\n"
	"precision highp int;\n"
	"\n";

const glw::GLchar* const GeometryShaderMaxOutputComponentsTest::m_fragment_shader_code_flat_in_ivec4 = "flat in ivec4";

const glw::GLchar* const GeometryShaderMaxOutputComponentsTest::m_fragment_shader_code_sum = "sum += ";

const glw::GLchar* const GeometryShaderMaxOutputComponentsTest::m_fragment_shader_code_body_begin =
	"\n"
	"layout(location = 0) out int fs_out;\n"
	"\n"
	"void main()\n"
	"{\n"
	"    int sum = 0;\n"
	"\n"
	"    // sum calculation go here\n";

const glw::GLchar* const GeometryShaderMaxOutputComponentsTest::m_fragment_shader_code_body_end = "\n"
																								  "    fs_out = sum;\n"
																								  "}\n";

/** ******************************************************************************************* **/
/* Vertex Shader for GeometryShaderMaxOutputVerticesTest */
const glw::GLchar* const GeometryShaderMaxOutputVerticesTest::m_vertex_shader_code =
	"${VERSION}\n"
	"\n"
	"${GEOMETRY_SHADER_REQUIRE}\n"
	"\n"
	"void main()\n"
	"{\n"
	"    gl_Position = vec4(gl_VertexID, 0, 0, 1);\n"
	"}\n";

/* Geometry Shader for GeometryShaderMaxOutputVerticesTest */
const glw::GLchar* const GeometryShaderMaxOutputVerticesTest::m_geometry_shader_code_preamble =
	"${VERSION}\n"
	"\n"
	"${GEOMETRY_SHADER_REQUIRE}\n"
	"\n"
	"// definition of NUMBER_OF_POINTS goes here\n"
	"#define NUMBER_OF_POINTS ";

const glw::GLchar* const GeometryShaderMaxOutputVerticesTest::m_geometry_shader_code_body =
	"u\n"
	"\n"
	"layout(points)                                in;\n"
	"layout(points, max_vertices=NUMBER_OF_POINTS) out;\n"
	"\n"
	"void main()\n"
	"{\n"
	"    int index = 0;\n"
	"\n"
	"    for (uint point = 0u; point < NUMBER_OF_POINTS; ++point)\n"
	"    {\n"
	"        EmitVertex();\n"
	"        EndPrimitive();\n"
	"    }\n"
	"\n"
	"}\n";

/* Fragment Shader for GeometryShaderMaxOutputVerticesTest */
const glw::GLchar* const GeometryShaderMaxOutputVerticesTest::m_fragment_shader_code =
	"${VERSION}\n"
	"\n"
	"${GEOMETRY_SHADER_REQUIRE}\n"
	"\n"
	"precision highp float;\n"
	"\n"
	"layout(location = 0) out vec4 fs_out;\n"
	"\n"
	"void main()\n"
	"{\n"
	"    fs_out = vec4(1, 1, 1, 1);\n"
	"}\n";

/** ***************************************************************************************************************** **/
/* Common shader parts for GeometryShaderMaxOutputComponentsSinglePointTest */
const glw::GLchar* const GeometryShaderMaxOutputComponentsSinglePointTest::m_common_shader_code_gs_fs_out =
	"gs_fs_out_";

const glw::GLchar* const GeometryShaderMaxOutputComponentsSinglePointTest::m_common_shader_code_gs_fs_out_definitions =
	"// definitions of gs_fs_out_ varyings go here\n";

/* Vertex Shader for GeometryShaderMaxOutputComponentsSinglePointTest */
const glw::GLchar* const GeometryShaderMaxOutputComponentsSinglePointTest::m_vertex_shader_code =
	"${VERSION}\n"
	"\n"
	"${GEOMETRY_SHADER_REQUIRE}\n"
	"\n"
	"void main()\n"
	"{\n"
	"    gl_Position = vec4(gl_VertexID, 0, 0, 1);\n"
	"}\n";

/* Geometry Shader Parts for GeometryShaderMaxOutputComponentsSinglePointTest */
const glw::GLchar* const GeometryShaderMaxOutputComponentsSinglePointTest::m_geometry_shader_code_preamble =
	"${VERSION}\n"
	"\n"
	"${GEOMETRY_SHADER_REQUIRE}\n"
	"${GEOMETRY_POINT_SIZE_ENABLE}\n"
	"\n"
	"layout(points)                 in;\n"
	"layout(points, max_vertices=1) out;\n"
	"\n";

const glw::GLchar* const GeometryShaderMaxOutputComponentsSinglePointTest::m_geometry_shader_code_flat_out_ivec4 =
	"flat out ivec4";

const glw::GLchar* const GeometryShaderMaxOutputComponentsSinglePointTest::m_geometry_shader_code_assignment =
	" = ivec4(index++, index++, index++, index++);\n";

const glw::GLchar* const GeometryShaderMaxOutputComponentsSinglePointTest::m_geometry_shader_code_body_begin =
	"\n"
	"void main()\n"
	"{\n"
	"    int index = 1;\n"
	"\n"
	"    // gs_fs_out assignments go here\n";

const glw::GLchar* const GeometryShaderMaxOutputComponentsSinglePointTest::m_geometry_shader_code_body_end =
	"\n"
	"    gl_PointSize = 2.0;\n"
	"    gl_Position = vec4(0.0, 0.0, 0.0, 1.0);\n"
	"\n"
	"    EmitVertex();\n"
	"    EndPrimitive();\n"
	"}\n";

/* Fragment Shader for GeometryShaderMaxOutputComponentsSinglePointTest */
const glw::GLchar* const GeometryShaderMaxOutputComponentsSinglePointTest::m_fragment_shader_code_preamble =
	"${VERSION}\n"
	"\n"
	"${GEOMETRY_SHADER_REQUIRE}\n"
	"\n";

const glw::GLchar* const GeometryShaderMaxOutputComponentsSinglePointTest::m_fragment_shader_code_flat_in_ivec4 =
	"flat in ivec4";

const glw::GLchar* const GeometryShaderMaxOutputComponentsSinglePointTest::m_fragment_shader_code_sum = "sum += ";

const glw::GLchar* const GeometryShaderMaxOutputComponentsSinglePointTest::m_fragment_shader_code_body_begin =
	"\n"
	"layout(location = 0) out int fs_out;\n"
	"\n"
	"void main()\n"
	"{\n"
	"    int sum = 0;\n"
	"\n"
	"    // sum calculation go here\n";

const glw::GLchar* const GeometryShaderMaxOutputComponentsSinglePointTest::m_fragment_shader_code_body_end =
	"\n"
	"    fs_out = sum;\n"
	"}\n";

/** ******************************************************************************************************************** **/
/* Vertex Shader for GeometryShaderMaxTextureUnitsTest */
const glw::GLchar* const GeometryShaderMaxTextureUnitsTest::m_vertex_shader_code_preamble =
	"${VERSION}\n"
	"\n"
	"${GEOMETRY_SHADER_REQUIRE}\n"
	"\n"
	"precision highp float;\n"
	"\n"
	"#define NUMBER_OF_POINTS ";

const glw::GLchar* const GeometryShaderMaxTextureUnitsTest::m_vertex_shader_code_body =
	"u\n"
	"\n"
	"flat out int vs_gs_vertex_id;\n"
	"\n"
	"void main()\n"
	"{\n"
	"    gl_Position     = vec4(-1.0 + ((2.0 / float(NUMBER_OF_POINTS)) * float(gl_VertexID)) + (1.0 / "
	"float(NUMBER_OF_POINTS)), 0, 0, 1.0);\n"
	"    vs_gs_vertex_id = gl_VertexID;\n"
	"}\n";

/* Geometry Shader for GeometryShaderMaxTextureUnitsTest */
const glw::GLchar* const GeometryShaderMaxTextureUnitsTest::m_geometry_shader_code_preamble =
	"${VERSION}\n"
	"\n"
	"${GEOMETRY_SHADER_REQUIRE}\n"
	"${GEOMETRY_POINT_SIZE_ENABLE}\n"
	"${GPU_SHADER5_REQUIRE}\n"
	"\n"
	"precision highp float;\n"
	"\n"
	"layout(points)                           in;\n"
	"layout(triangle_strip, max_vertices = 4) out;\n"
	"\n"
	"#define NUMBER_OF_POINTS ";

const glw::GLchar* const GeometryShaderMaxTextureUnitsTest::m_geometry_shader_code_body =
	"u\n"
	"\n"
	"// NUMBER_OF_POINTS == NUMBER_OF_SAMPLERS\n"
	"     uniform lowp isampler2D gs_texture[NUMBER_OF_POINTS];\n"
	"flat in      int        vs_gs_vertex_id[1];\n"
	"flat out     int        gs_fs_color;\n"
	"\n"
	"void main()\n"
	"{\n"
	"    float half_of_edge = (1.0 / float(NUMBER_OF_POINTS));\n"
	"    int   color        = 0;\n"
	"\n"
	"    for (uint i = 0u; i <= uint(vs_gs_vertex_id[0]); ++i)\n"
	"    {\n"
	"        color += texture(gs_texture[i], vec2(0.0, 0.0)).r;\n"
	"    }\n"
	"\n"
	"    gl_Position = gl_in[0].gl_Position + vec4(-half_of_edge,  1.0, 0, 0);\n"
	"    gs_fs_color = color;\n"
	"    EmitVertex();\n"
	"\n"
	"    gl_Position = gl_in[0].gl_Position + vec4( half_of_edge,  1.0, 0, 0);\n"
	"    gs_fs_color = color;\n"
	"    EmitVertex();\n"
	"\n"
	"    gl_Position = gl_in[0].gl_Position + vec4(-half_of_edge, -1.0, 0, 0);\n"
	"    gs_fs_color = color;\n"
	"    EmitVertex();\n"
	"\n"
	"    gl_Position = gl_in[0].gl_Position + vec4( half_of_edge, -1.0, 0, 0);\n"
	"    gs_fs_color = color;\n"
	"    EmitVertex();\n"
	"    EndPrimitive();\n"
	"}\n";

/* Fragment Shader for GeometryShaderMaxTextureUnitsTest */
const glw::GLchar* const GeometryShaderMaxTextureUnitsTest::m_fragment_shader_code =
	"${VERSION}\n"
	"\n"
	"${GEOMETRY_SHADER_REQUIRE}\n"
	"\n"
	"precision highp float;\n"
	"\n"
	"flat                 in  int gs_fs_color;\n"
	"layout(location = 0) out int fs_out_color;\n"
	"\n"
	"void main()\n"
	"{\n"
	"    fs_out_color = gs_fs_color;\n"
	"}\n";

/** *******************************************************************************************************/
/* Vertex Shader for GeometryShaderMaxInvocationsTest */
const glw::GLchar* const GeometryShaderMaxInvocationsTest::m_vertex_shader_code =
	"${VERSION}\n"
	"\n"
	"${GEOMETRY_SHADER_REQUIRE}\n"
	"\n"
	"precision highp float;\n"
	"\n"
	"void main()\n"
	"{\n"
	"    gl_Position = vec4(gl_VertexID, 0, 0, 1);\n"
	"}\n";

/* Geometry Shader for GeometryShaderMaxInvocationsTest */
const glw::GLchar* const GeometryShaderMaxInvocationsTest::m_geometry_shader_code_preamble =
	"${VERSION}\n"
	"\n"
	"${GEOMETRY_SHADER_REQUIRE}\n"
	"\n"
	"precision highp float;\n"
	"\n"
	"#define GEOMETRY_SHADER_INVOCATIONS ";

const glw::GLchar* const GeometryShaderMaxInvocationsTest::m_geometry_shader_code_layout =
	"u\n"
	"\n"
	"layout(points) in;\n"
	"layout(triangle_strip, max_vertices = 3) out;\n";

const glw::GLchar* const GeometryShaderMaxInvocationsTest::m_geometry_shader_code_layout_invocations =
	"layout(invocations = GEOMETRY_SHADER_INVOCATIONS) in;\n";

const glw::GLchar* const GeometryShaderMaxInvocationsTest::m_geometry_shader_code_body =
	"\n"
	"void main()\n"
	"{\n"
	"    float dx = (2.0 / float(GEOMETRY_SHADER_INVOCATIONS));\n"
	"    \n"
	"    gl_Position = vec4(-1.0 + (dx * float(gl_InvocationID)),      -1.001, 0.0, 1.0);\n"
	"    EmitVertex();\n"
	"    \n"
	"    gl_Position = vec4(-1.0 + (dx * float(gl_InvocationID)),       1.001, 0.0, 1.0);\n"
	"    EmitVertex();\n"
	"    \n"
	"    gl_Position = vec4(-1.0 + (dx * float(gl_InvocationID + 1)),   1.001, 0.0, 1.0);\n"
	"    EmitVertex();\n"
	"}\n";

/* Fragment Shader for GeometryShaderMaxInvocationsTest */
const glw::GLchar* const GeometryShaderMaxInvocationsTest::m_fragment_shader_code =
	"${VERSION}\n"
	"\n"
	"${GEOMETRY_SHADER_REQUIRE}\n"
	"\n"
	"precision highp float;\n"
	"\n"
	"layout(location = 0) out vec4 fs_out_color;\n"
	"\n"
	"void main()\n"
	"{\n"
	"    fs_out_color = vec4(0.0, 1.0, 0.0, 0.0);\n"
	"}\n";

/** ***************************************************************************************************** **/
/* Vertex Shader for GeometryShaderMaxCombinedTextureUnitsTest */
const glw::GLchar* const GeometryShaderMaxCombinedTextureUnitsTest::m_vertex_shader_code_preamble =
	"${VERSION}\n"
	"\n"
	"${GEOMETRY_SHADER_REQUIRE}\n"
	"${GPU_SHADER5_ENABLE}\n"
	"\n"
	"precision highp float;\n"
	"\n"
	"#define NUMBER_OF_POINTS ";

const glw::GLchar* const GeometryShaderMaxCombinedTextureUnitsTest::m_vertex_shader_code_body =
	"u\n"
	"\n"
	"// NUMBER_OF_POINTS == NUMBER_OF_SAMPLERS\n"
	"     uniform highp usampler2D sampler[NUMBER_OF_POINTS];"
	"flat out     int        vs_gs_vertex_id;\n"
	"flat out     uint       vs_gs_sum;\n"
	"\n"
	"void main()\n"
	"{\n"
	"    uint sum = 0u;\n"
	"\n"
	"    for (uint i = 0u; i < uint(gl_VertexID); ++i)\n"
	"    {\n"
	"        sum += texture(sampler[i], vec2(0.0, 0.0)).r;\n"
	"    }\n"
	"\n"
	"    gl_Position     = vec4(-1.0 + ((2.0 / float(NUMBER_OF_POINTS)) * float(gl_VertexID)) + (1.0 / "
	"float(NUMBER_OF_POINTS)), 0, 0, 1.0);\n"
	"    vs_gs_vertex_id = gl_VertexID;\n"
	"    vs_gs_sum       = sum;\n"
	"}\n";

/* Geometry Shader for GeometryShaderMaxCombinedTextureUnitsTest */
const glw::GLchar* const GeometryShaderMaxCombinedTextureUnitsTest::m_geometry_shader_code_preamble =
	"${VERSION}\n"
	"\n"
	"${GEOMETRY_SHADER_REQUIRE}\n"
	"${GPU_SHADER5_ENABLE}\n"
	"\n"
	"precision highp float;\n"
	"\n"
	"layout(points)                   in;\n"
	"layout(points, max_vertices = 1) out;\n"
	"\n"
	"#define NUMBER_OF_POINTS ";

const glw::GLchar* const GeometryShaderMaxCombinedTextureUnitsTest::m_geometry_shader_code_body =
	"u\n"
	"\n"
	"// NUMBER_OF_POINTS == NUMBER_OF_SAMPLERS\n"
	"     uniform highp usampler2D sampler[NUMBER_OF_POINTS];\n"
	"flat in      int        vs_gs_vertex_id[1];\n"
	"flat in      uint       vs_gs_sum      [1];\n"
	"flat out     int        gs_fs_vertex_id;\n"
	"flat out     uint       gs_fs_vertex_sum;\n"
	"flat out     uint       gs_fs_geometry_sum;\n"
	"\n"
	"void main()\n"
	"{\n"
	"    uint sum = 0u;\n"
	"\n"
	"    for (uint i = 0u; i < uint(vs_gs_vertex_id[0]); ++i)\n"
	"    {\n"
	"        sum += texture(sampler[i], vec2(0.0, 0.0)).r;\n"
	"    }\n"
	"\n"
	"    gl_Position        = gl_in[0].gl_Position;\n"
	"    gs_fs_vertex_id    = vs_gs_vertex_id[0];\n"
	"    gs_fs_vertex_sum   = vs_gs_sum      [0];\n"
	"    gs_fs_geometry_sum = sum;\n"
	"    EmitVertex();\n"
	"    EndPrimitive();\n"
	"}\n";

/* Fragment Shader for GeometryShaderMaxCombinedTextureUnitsTest */
const glw::GLchar* const GeometryShaderMaxCombinedTextureUnitsTest::m_fragment_shader_code_preamble =
	"${VERSION}\n"
	"\n"
	"${GEOMETRY_SHADER_REQUIRE}\n"
	"${GPU_SHADER5_ENABLE}\n"
	"\n"
	"precision highp float;\n"
	"\n"
	"// NUMBER_OF_POINTS == NUMBER_OF_SAMPLERS\n"
	"#define NUMBER_OF_POINTS ";

const glw::GLchar* const GeometryShaderMaxCombinedTextureUnitsTest::m_fragment_shader_code_body =
	"u\n"
	"\n"
	"                     uniform highp usampler2D sampler[NUMBER_OF_POINTS];\n"
	"flat                 in      int        gs_fs_vertex_id;\n"
	"flat                 in      uint       gs_fs_vertex_sum;\n"
	"flat                 in      uint       gs_fs_geometry_sum;\n"
	"layout(location = 0) out     uint       fs_out_color;\n"
	"\n"
	"void main()\n"
	"{\n"
	"    uint sum = 0u;\n"
	"\n"
	"    for (uint i = 0u; i < uint(gs_fs_vertex_id); ++i)\n"
	"    {\n"
	"        sum += texture(sampler[i], vec2(0.0, 0.0)).r;\n"
	"    }\n"
	"    fs_out_color = sum + gs_fs_vertex_sum + gs_fs_geometry_sum;\n"
	"}\n";

/** ***************************************************************************************************************** **/

/* Constants for GeometryShaderMaxUniformComponentsTest */
const unsigned int		 GeometryShaderMaxUniformComponentsTest::m_buffer_size			   = sizeof(glw::GLint);
const glw::GLchar* const GeometryShaderMaxUniformComponentsTest::m_captured_varyings_names = "gs_out_sum";

/* Constants for GeometryShaderMaxUniformBlocksTest */
const unsigned int		 GeometryShaderMaxUniformBlocksTest::m_buffer_size			   = sizeof(glw::GLint);
const glw::GLchar* const GeometryShaderMaxUniformBlocksTest::m_captured_varyings_names = "gs_out_sum";

/* Constants for GeometryShaderMaxInputComponentsTest */
const unsigned int		 GeometryShaderMaxInputComponentsTest::m_buffer_size			 = sizeof(glw::GLint);
const glw::GLchar* const GeometryShaderMaxInputComponentsTest::m_captured_varyings_names = "gs_out_sum";

/* Constants for GeometryShaderMaxOutputComponentsTest */
const unsigned int GeometryShaderMaxOutputComponentsTest::m_point_size		   = 2;
const unsigned int GeometryShaderMaxOutputComponentsTest::m_texture_height	 = m_point_size;
const unsigned int GeometryShaderMaxOutputComponentsTest::m_texture_pixel_size = 4 * sizeof(glw::GLint);

/* Constants for GeometryShaderMaxOutputComponentsSinglePointTest */
const unsigned int GeometryShaderMaxOutputComponentsSinglePointTest::m_point_size		  = 2;
const unsigned int GeometryShaderMaxOutputComponentsSinglePointTest::m_texture_height	 = m_point_size;
const unsigned int GeometryShaderMaxOutputComponentsSinglePointTest::m_texture_pixel_size = 4 * sizeof(glw::GLint);
const unsigned int GeometryShaderMaxOutputComponentsSinglePointTest::m_texture_width	  = m_point_size;

/* Constants for GeometryShaderMaxTextureUnitsTest */
const unsigned int GeometryShaderMaxTextureUnitsTest::m_point_size		   = 2;
const unsigned int GeometryShaderMaxTextureUnitsTest::m_texture_height	 = m_point_size;
const unsigned int GeometryShaderMaxTextureUnitsTest::m_texture_pixel_size = 4 * sizeof(glw::GLint);

/* Constants for GeometryShaderMaxInvocationsTest */
const unsigned int GeometryShaderMaxInvocationsTest::m_triangle_edge_length = 9;
const unsigned int GeometryShaderMaxInvocationsTest::m_texture_height		= m_triangle_edge_length;
const unsigned int GeometryShaderMaxInvocationsTest::m_texture_pixel_size   = 4 * sizeof(glw::GLubyte);

/* Constants for GeometryShaderMaxCombinedTextureUnitsTest */
const unsigned int GeometryShaderMaxCombinedTextureUnitsTest::m_point_size		   = 1;
const unsigned int GeometryShaderMaxCombinedTextureUnitsTest::m_texture_height	 = m_point_size;
const unsigned int GeometryShaderMaxCombinedTextureUnitsTest::m_texture_pixel_size = 4 * sizeof(glw::GLint);

/** Constructor
 *
 *  @param context     Test context
 *  @param name        Test case's name
 *  @param description Test case's description
 **/
GeometryShaderLimitsTransformFeedbackBase::GeometryShaderLimitsTransformFeedbackBase(Context&			  context,
																					 const ExtParameters& extParams,
																					 const char*		  name,
																					 const char*		  description)
	: TestCaseBase(context, extParams, name, description)
	, m_fragment_shader_id(0)
	, m_geometry_shader_id(0)
	, m_program_object_id(0)
	, m_vertex_shader_id(0)
	, m_buffer_object_id(0)
	, m_vertex_array_object_id(0)
	, m_fragment_shader_parts(0)
	, m_geometry_shader_parts(0)
	, m_vertex_shader_parts(0)
	, m_n_fragment_shader_parts(0)
	, m_n_geometry_shader_parts(0)
	, m_n_vertex_shader_parts(0)
	, m_captured_varyings_names(0)
	, m_n_captured_varyings(0)
	, m_buffer_size(0)
{
	/* Nothing to be done here */
}

/** Initializes GLES objects used during the test.
 *
 */
void GeometryShaderLimitsTransformFeedbackBase::initTest()
{
	/* This test should only run if EXT_geometry_shader is supported */
	if (!m_is_geometry_shader_extension_supported)
	{
		throw tcu::NotSupportedError(GEOMETRY_SHADER_EXTENSION_NOT_SUPPORTED, "", __FILE__, __LINE__);
	}

	/* Retrieve ES entrypoints */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Get shaders code from child class */
	getShaderParts(m_fragment_shader_parts, m_n_fragment_shader_parts, m_geometry_shader_parts,
				   m_n_geometry_shader_parts, m_vertex_shader_parts, m_n_vertex_shader_parts);

	/* Get captured varyings from inheriting class */
	getCapturedVaryings(m_captured_varyings_names, m_n_captured_varyings);

	/* Create program and shaders */
	m_program_object_id = gl.createProgram();

	m_fragment_shader_id = gl.createShader(GL_FRAGMENT_SHADER);
	m_geometry_shader_id = gl.createShader(m_glExtTokens.GEOMETRY_SHADER);
	m_vertex_shader_id   = gl.createShader(GL_VERTEX_SHADER);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Failed to create program or shader object(s)");

	/* Set up transform feedback */
	gl.transformFeedbackVaryings(m_program_object_id, m_n_captured_varyings, m_captured_varyings_names,
								 GL_INTERLEAVED_ATTRIBS);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Failed to set transform feedback varyings");

	/* Build program */
	if (false == buildProgram(m_program_object_id, m_fragment_shader_id, m_n_fragment_shader_parts,
							  m_fragment_shader_parts, m_geometry_shader_id, m_n_geometry_shader_parts,
							  m_geometry_shader_parts, m_vertex_shader_id, m_n_vertex_shader_parts,
							  m_vertex_shader_parts))
	{
		TCU_FAIL("Could not create program from valid vertex/geometry/fragment shader");
	}

	/* Generate and bind VAO */
	gl.genVertexArrays(1, &m_vertex_array_object_id);
	gl.bindVertexArray(m_vertex_array_object_id);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not create vertex array object");

	/* Get size of buffer used by transform feedback from child class */
	getTransformFeedbackBufferSize(m_buffer_size);

	/* Generate, bind and allocate buffer */
	gl.genBuffers(1, &m_buffer_object_id);
	gl.bindBuffer(GL_ARRAY_BUFFER, m_buffer_object_id);
	gl.bufferData(GL_ARRAY_BUFFER, m_buffer_size, 0 /* no start data */, GL_STATIC_COPY);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not create buffer object");
}

/** Executes the test.
 *  Sets the test result to QP_TEST_RESULT_FAIL if the test failed, QP_TEST_RESULT_PASS otherwise.
 *
 *  Note the function throws exception should an error occur!
 *
 *  @return STOP if the test has finished, CONTINUE to indicate iterate should be called once again.
 *
 **/
tcu::TestCase::IterateResult GeometryShaderLimitsTransformFeedbackBase::iterate()
{
	initTest();

	/* Retrieve ES entrypoints */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Verification result */
	bool result = false;

	/* Setup transform feedback */
	gl.enable(GL_RASTERIZER_DISCARD);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glEnable(GL_RASTERIZER_DISCARD) call failed");

	gl.bindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, m_buffer_object_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBufferBase() call failed");

	/* Setup draw call */
	gl.useProgram(m_program_object_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not use program");

	gl.beginTransformFeedback(GL_POINTS);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBeginTransformFeedback() call failed");
	{
		/* Let child class prepare input data */
		prepareProgramInput();

		/* Draw */
		gl.drawArrays(GL_POINTS, 0 /* first */, 1 /* one point */);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawArrays() call failed");
	}
	/* Stop transform feedback */
	gl.endTransformFeedback();
	GLU_EXPECT_NO_ERROR(gl.getError(), "glEndTransformFeedback() call failed");

	/* Map transfrom feedback results */
	const void* transform_feedback_data =
		gl.mapBufferRange(GL_TRANSFORM_FEEDBACK_BUFFER, 0 /* offset */, m_buffer_size, GL_MAP_READ_BIT);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not map the buffer object into process space");

	/* Verify data extracted from transfrom feedback */
	result = verifyResult(transform_feedback_data);

	/* Unmap transform feedback buffer */
	gl.unmapBuffer(GL_TRANSFORM_FEEDBACK_BUFFER);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error unmapping the buffer object");

	/* Let child class clean itself */
	clean();

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

/** Deinitializes GLES objects created during the test.
 *
 */
void GeometryShaderLimitsTransformFeedbackBase::deinit()
{
	/* Retrieve ES entrypoints */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Bind default values */
	gl.useProgram(0);
	gl.bindVertexArray(0);
	gl.bindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0 /* offset */, 0 /* id */);
	gl.bindBuffer(GL_ARRAY_BUFFER, 0);

	/* Delete program object and shaders */
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

	if (0 != m_geometry_shader_id)
	{
		gl.deleteShader(m_geometry_shader_id);

		m_geometry_shader_id = 0;
	}

	if (0 != m_vertex_shader_id)
	{
		gl.deleteShader(m_vertex_shader_id);

		m_vertex_shader_id = 0;
	}

	/* Delete buffer objects */
	if (0 != m_buffer_object_id)
	{
		gl.deleteBuffers(1, &m_buffer_object_id);

		m_buffer_object_id = 0;
	}

	if (0 != m_vertex_array_object_id)
	{
		gl.deleteVertexArrays(1, &m_vertex_array_object_id);

		m_vertex_array_object_id = 0;
	}

	/* Deinitialize base class */
	TestCaseBase::deinit();
}

/** Constructor
 *
 * @param context       Test context
 * @param name          Test case's name
 * @param description   Test case's description
 **/
GeometryShaderLimitsRenderingBase::GeometryShaderLimitsRenderingBase(Context& context, const ExtParameters& extParams,
																	 const char* name, const char* description)
	: TestCaseBase(context, extParams, name, description)
	, m_fragment_shader_id(0)
	, m_geometry_shader_id(0)
	, m_program_object_id(0)
	, m_vertex_shader_id(0)
	, m_framebuffer_object_id(0)
	, m_color_texture_id(0)
	, m_vertex_array_object_id(0)
	, m_fragment_shader_parts(0)
	, m_geometry_shader_parts(0)
	, m_vertex_shader_parts(0)
	, m_n_fragment_shader_parts(0)
	, m_n_geometry_shader_parts(0)
	, m_n_vertex_shader_parts(0)
	, m_texture_format(GL_RGBA8)
	, m_texture_height(0)
	, m_texture_pixel_size(0)
	, m_texture_read_format(GL_RGBA)
	, m_texture_read_type(GL_UNSIGNED_BYTE)
	, m_texture_width(0)
{
	/* Nothing to be done here */
}

/** Initializes GLES objects used during the test.
 *
 */
void GeometryShaderLimitsRenderingBase::initTest()
{
	/* This test should only run if EXT_geometry_shader is supported */
	if (!m_is_geometry_shader_extension_supported)
	{
		throw tcu::NotSupportedError(GEOMETRY_SHADER_EXTENSION_NOT_SUPPORTED, "", __FILE__, __LINE__);
	}

	/* Query on support of EXT_gpu_shader5 */
	if (!m_is_gpu_shader5_supported)
	{
		throw tcu::NotSupportedError(GPU_SHADER5_EXTENSION_NOT_SUPPORTED, "", __FILE__, __LINE__);
	}

	/* Retrieve ES entry-points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Verify that point size range is supported */
	glw::GLfloat point_size_range[2] = { 0 };
	glw::GLfloat required_point_size = 0.0f;

	if (!glu::isContextTypeES(m_context.getRenderContext().getType()))
	{
		gl.getFloatv(GL_POINT_SIZE_RANGE, point_size_range);
	}
	else
	{
		gl.getFloatv(GL_ALIASED_POINT_SIZE_RANGE, point_size_range);
	}

	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetFloatv() call failed");

	getRequiredPointSize(required_point_size);

	if (required_point_size > point_size_range[1])
	{
		m_testCtx.getLog() << tcu::TestLog::Message
						   << "Test requires a minimum maximum point size of: " << required_point_size
						   << ", implementation reports a maximum of : " << point_size_range[1]
						   << tcu::TestLog::EndMessage;

		throw tcu::NotSupportedError("Required point size is not supported", "", __FILE__, __LINE__);
	}
	if (!glu::isContextTypeES(m_context.getRenderContext().getType()))
	{
		gl.enable(GL_PROGRAM_POINT_SIZE);
	}

	/* Get shaders code from child class */
	getShaderParts(m_fragment_shader_parts, m_n_fragment_shader_parts, m_geometry_shader_parts,
				   m_n_geometry_shader_parts, m_vertex_shader_parts, m_n_vertex_shader_parts);

	/* Create program and shaders */
	m_program_object_id = gl.createProgram();

	m_fragment_shader_id = gl.createShader(GL_FRAGMENT_SHADER);
	m_geometry_shader_id = gl.createShader(m_glExtTokens.GEOMETRY_SHADER);
	m_vertex_shader_id   = gl.createShader(GL_VERTEX_SHADER);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Failed to create program or shader object(s)");

	/* Build program */
	if (false == buildProgram(m_program_object_id, m_fragment_shader_id, m_n_fragment_shader_parts,
							  m_fragment_shader_parts, m_geometry_shader_id, m_n_geometry_shader_parts,
							  m_geometry_shader_parts, m_vertex_shader_id, m_n_vertex_shader_parts,
							  m_vertex_shader_parts))
	{
		TCU_FAIL("Could not create program from valid vertex/geometry/fragment shader");
	}

	/* Set up a vertex array object */
	gl.genVertexArrays(1, &m_vertex_array_object_id);
	gl.bindVertexArray(m_vertex_array_object_id);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not create vertex array object");

	/* Get framebuffer details */
	getFramebufferDetails(m_texture_format, m_texture_read_format, m_texture_read_type, m_texture_width,
						  m_texture_height, m_texture_pixel_size);

	/* Set up texture object and a FBO */
	gl.genTextures(1, &m_color_texture_id);
	gl.genFramebuffers(1, &m_framebuffer_object_id);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not create framebuffer");

	if (false == setupFramebufferWithTextureAsAttachment(m_framebuffer_object_id, m_color_texture_id, m_texture_format,
														 m_texture_width, m_texture_height))
	{
		TCU_FAIL("Failed to setup framebuffer");
	}
}

/** Executes the test.
 *  Sets the test result to QP_TEST_RESULT_FAIL if the test failed, QP_TEST_RESULT_PASS otherwise.
 *
 *  Note the function throws exception should an error occur!
 *
 *  @return STOP if the test has finished, CONTINUE to indicate iterate should be called once again.
 *
 **/
tcu::TestCase::IterateResult GeometryShaderLimitsRenderingBase::iterate()
{
	initTest();

	/* Retrieve ES entry-points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Variables used for image verification purposes */
	std::vector<unsigned char> result_image(m_texture_width * m_texture_height * m_texture_pixel_size);

	/* Render */
	gl.useProgram(m_program_object_id);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not use program");

	/* Let child class prepare input for program */
	prepareProgramInput();

	gl.clearColor(0 /* red */, 0 /* green */, 0 /* blue */, 0 /* alpha */);
	gl.clear(GL_COLOR_BUFFER_BIT);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not clear the color buffer");

	/* Get draw call details from child class */
	glw::GLenum primitive_type = GL_POINTS;
	glw::GLuint n_vertices	 = 1;

	getDrawCallDetails(primitive_type, n_vertices);

	gl.drawArrays(primitive_type, 0 /* first */, n_vertices);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawArrays() call failed");

	/* Extract image from FBO */
	gl.readPixels(0 /* x */, 0 /* y */, m_texture_width, m_texture_height, m_texture_read_format, m_texture_read_type,
				  &result_image[0]);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not read back pixels from color buffer");

	/* Run verification */
	if (true == verifyResult(&result_image[0]))
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
	}

	/* Let child class clean itself */
	clean();

	return STOP;
}

/** Deinitializes GLES objects created during the test.
 *
 */
void GeometryShaderLimitsRenderingBase::deinit()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Reset OpenGL ES state */
	gl.useProgram(0);
	gl.bindVertexArray(0);
	gl.framebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0 /* texture */, 0 /* level */);
	gl.bindTexture(GL_TEXTURE_2D, 0);
	gl.bindFramebuffer(GL_FRAMEBUFFER, 0);
	if (!glu::isContextTypeES(m_context.getRenderContext().getType()))
	{
		gl.disable(GL_PROGRAM_POINT_SIZE);
	}

	/* Delete program object and shaders */
	if (m_program_object_id != 0)
	{
		gl.deleteProgram(m_program_object_id);

		m_program_object_id = 0;
	}

	if (m_fragment_shader_id != 0)
	{
		gl.deleteShader(m_fragment_shader_id);

		m_fragment_shader_id = 0;
	}

	if (m_geometry_shader_id != 0)
	{
		gl.deleteShader(m_geometry_shader_id);

		m_geometry_shader_id = 0;
	}

	if (m_vertex_shader_id != 0)
	{
		gl.deleteShader(m_vertex_shader_id);

		m_vertex_shader_id = 0;
	}

	/* Delete frambuffer and textures */
	if (m_framebuffer_object_id != 0)
	{
		gl.deleteFramebuffers(1, &m_framebuffer_object_id);

		m_framebuffer_object_id = 0;
	}

	if (m_color_texture_id != 0)
	{
		gl.deleteTextures(1, &m_color_texture_id);

		m_color_texture_id = 0;
	}

	if (m_vertex_array_object_id != 0)
	{
		gl.deleteVertexArrays(1, &m_vertex_array_object_id);

		m_vertex_array_object_id = 0;
	}

	/* Deinitialize base class */
	TestCaseBase::deinit();
}

/** Constructor
 *
 *  @param context       Test context
 *  @param name          Test case's name
 *  @param description   Test case's description
 **/
GeometryShaderMaxUniformComponentsTest::GeometryShaderMaxUniformComponentsTest(Context&				context,
																			   const ExtParameters& extParams,
																			   const char*			name,
																			   const char*			description)
	: GeometryShaderLimitsTransformFeedbackBase(context, extParams, name, description)
	, m_max_uniform_components(0)
	, m_max_uniform_vectors(0)
	, m_uniform_location(0)
{
	/* Nothing to be done here */
}

/** Clears data after draw call and result verification
 *
 **/
void GeometryShaderMaxUniformComponentsTest::clean()
{
	m_uniform_data.clear();
}

/** Get names and number of varyings to be captured by transform feedback
 *
 *  @param out_captured_varyings_names Array of varying names
 *  @param out_n_captured_varyings     Number of varying names
 **/
void GeometryShaderMaxUniformComponentsTest::getCapturedVaryings(const glw::GLchar* const*& out_captured_varyings_names,
																 glw::GLuint&				out_n_captured_varyings)
{
	/* Varying names */
	out_captured_varyings_names = &m_captured_varyings_names;

	/* Number of varyings */
	out_n_captured_varyings = 1;
}

/** Get parts of shaders
 *
 *  @param out_fragment_shader_parts   Array of fragment shader parts
 *  @param out_n_fragment_shader_parts Number of fragment shader parts
 *  @param out_geometry_shader_parts   Array of geometry shader parts
 *  @param out_n_geometry_shader_parts Number of geometry shader parts
 *  @param out_vertex_shader_parts     Array of vertex shader parts
 *  @param out_n_vertex_shader_parts   Number of vertex shader parts
 **/
void GeometryShaderMaxUniformComponentsTest::getShaderParts(const glw::GLchar* const*& out_fragment_shader_parts,
															unsigned int&			   out_n_fragment_shader_parts,
															const glw::GLchar* const*& out_geometry_shader_parts,
															unsigned int&			   out_n_geometry_shader_parts,
															const glw::GLchar* const*& out_vertex_shader_parts,
															unsigned int&			   out_n_vertex_shader_parts)
{
	/* Retrieve ES entry-points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Fragment Shader */
	out_fragment_shader_parts   = &m_fragment_shader_code;
	out_n_fragment_shader_parts = 1;

	/* Get maximum number of uniform */
	gl.getIntegerv(m_glExtTokens.MAX_GEOMETRY_UNIFORM_COMPONENTS, &m_max_uniform_components);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv() failed for GL_MAX_GEOMETRY_UNIFORM_COMPONENTS_EXT pname.");

	m_max_uniform_vectors = m_max_uniform_components / 4 /* 4 components per vector */;

	std::stringstream stream;
	stream << m_max_uniform_vectors;
	m_max_uniform_vectors_string = stream.str();

	/* Geometry Shader */
	m_geometry_shader_parts[0] = m_geometry_shader_code_preamble;
	m_geometry_shader_parts[1] = m_geometry_shader_code_number_of_uniforms;
	m_geometry_shader_parts[2] = m_max_uniform_vectors_string.c_str();
	m_geometry_shader_parts[3] = m_geometry_shader_code_body;

	out_geometry_shader_parts   = m_geometry_shader_parts;
	out_n_geometry_shader_parts = 4;

	/* Vertex Shader */
	out_vertex_shader_parts   = &m_vertex_shader_code;
	out_n_vertex_shader_parts = 1;
}

/** Get size of buffer used by transform feedback
 *
 *  @param out_buffer_size Size of buffer in bytes
 **/
void GeometryShaderMaxUniformComponentsTest::getTransformFeedbackBufferSize(unsigned int& out_buffer_size)
{
	out_buffer_size = m_buffer_size;
}

/** Prepare test specific program input for draw call
 *
 **/
void GeometryShaderMaxUniformComponentsTest::prepareProgramInput()
{
	/* Retrieve ES entry-points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Uniform location */
	m_uniform_location = gl.getUniformLocation(m_program_object_id, "uni_array");

	GLU_EXPECT_NO_ERROR(gl.getError(), "Failed to get uniform location");

	if (-1 == m_uniform_location)
	{
		TCU_FAIL("Invalid uniform location");
	}

	/* 3. Configure the uniforms to use subsequently increasing values, starting
	 *  from 1 for R component of first vector, 2 for G component of that vector,
	 *  5 for first component of second vector, and so on.
	 **/
	m_uniform_data.resize(m_max_uniform_components);

	for (glw::GLint i = 0; i < m_max_uniform_components; ++i)
	{
		m_uniform_data[i] = i + 1;
	}

	/* Set uniform data */
	gl.uniform4iv(m_uniform_location, m_max_uniform_vectors, &m_uniform_data[0]);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Failed to set uniform data");
}

/** Verification of results
 *
 *  @param result Pointer to data mapped from transform feedback buffer.
 (                Size of data is equal to buffer_size set by getTransformFeedbackBufferSize()
 *
 *  @return true  Result matches expected value
 *          false Result has wrong value
 **/
bool GeometryShaderMaxUniformComponentsTest::verifyResult(const void* data)
{
	/* Expected data, sum of elements in range <x;y> with length n = ((x + y) / 2) * n */
	const glw::GLint expected_data = ((1 + m_max_uniform_components) * m_max_uniform_components) / 2;

	/* Cast to const GLint */
	const glw::GLint* transform_feedback_data = (const glw::GLint*)data;

	/* Verify data extracted from transfrom feedback */
	if (0 != memcmp(transform_feedback_data, &expected_data, m_buffer_size))
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Wrong result! Expected: " << expected_data
						   << " Extracted: " << *transform_feedback_data << tcu::TestLog::EndMessage;

		return false;
	}
	else
	{
		return true;
	}
}

/** Constructor
 *
 *  @param context       Test context
 *  @param name          Test case's name
 *  @param description   Test case's description
 **/
GeometryShaderMaxUniformBlocksTest::GeometryShaderMaxUniformBlocksTest(Context& context, const ExtParameters& extParams,
																	   const char* name, const char* description)
	: GeometryShaderLimitsTransformFeedbackBase(context, extParams, name, description), m_max_uniform_blocks(0)
{
	/* Nothing to be done here */
}

/** Clears data after draw call and result verification
 *
 **/
void GeometryShaderMaxUniformBlocksTest::clean()
{
	/* Retrieve ES entry-points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Bind default to uniform binding point */
	gl.bindBuffer(GL_UNIFORM_BUFFER, 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer() call failed");

	/* Release buffers */
	for (glw::GLint i = 0; i < m_max_uniform_blocks; ++i)
	{
		/* Bind default to uniform block */
		gl.bindBufferBase(GL_UNIFORM_BUFFER, i, 0);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBufferBase() call failed");

		/* Delete buffer */
		gl.deleteBuffers(1, &m_uniform_blocks[i].buffer_object_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glDeleteBuffers() call failed");
	}

	/* Free memory */
	m_uniform_blocks.clear();
}

/** Get names and number of varyings to be captured by transform feedback
 *
 *  @param out_captured_varyings_names Array of varying names
 *  @param out_n_captured_varyings     Number of varying names
 **/
void GeometryShaderMaxUniformBlocksTest::getCapturedVaryings(const glw::GLchar* const*& out_captured_varyings_names,
															 glw::GLuint&				out_n_captured_varyings)
{
	/* Varying names */
	out_captured_varyings_names = &m_captured_varyings_names;

	/* Number of varyings */
	out_n_captured_varyings = 1;
}

/** Get parts of shaders
 *
 *  @param out_fragment_shader_parts   Array of fragment shader parts
 *  @param out_n_fragment_shader_parts Number of fragment shader parts
 *  @param out_geometry_shader_parts   Array of geometry shader parts
 *  @param out_n_geometry_shader_parts Number of geometry shader parts
 *  @param out_vertex_shader_parts     Array of vertex shader parts
 *  @param out_n_vertex_shader_parts   Number of vertex shader parts
 **/
void GeometryShaderMaxUniformBlocksTest::getShaderParts(const glw::GLchar* const*& out_fragment_shader_parts,
														unsigned int&			   out_n_fragment_shader_parts,
														const glw::GLchar* const*& out_geometry_shader_parts,
														unsigned int&			   out_n_geometry_shader_parts,
														const glw::GLchar* const*& out_vertex_shader_parts,
														unsigned int&			   out_n_vertex_shader_parts)
{
	/* Retrieve ES entry-points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Fragment Shader */
	out_fragment_shader_parts   = &m_fragment_shader_code;
	out_n_fragment_shader_parts = 1;

	/* Get maximum number of uniform blocks */
	gl.getIntegerv(m_glExtTokens.MAX_GEOMETRY_UNIFORM_BLOCKS, &m_max_uniform_blocks);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv() call failed for GL_MAX_GEOMETRY_UNIFORM_BLOCKS_EXT");

	std::stringstream stream;
	stream << m_max_uniform_blocks;
	m_max_uniform_blocks_string = stream.str();

	/* Geometry Shader */
	m_geometry_shader_parts[0] = m_geometry_shader_code_preamble;
	m_geometry_shader_parts[1] = m_geometry_shader_code_number_of_uniforms;
	m_geometry_shader_parts[2] = m_max_uniform_blocks_string.c_str();
	m_geometry_shader_parts[3] = m_geometry_shader_code_body_str;

	stream.str(std::string());
	stream.clear();
	for (glw::GLint uniform_block_nr = 0; uniform_block_nr < m_max_uniform_blocks; ++uniform_block_nr)
	{
		stream << "    gs_out_sum += uni_block_array[" << uniform_block_nr << "].entry;\n";
	}
	m_uniform_block_access_string = stream.str();

	m_geometry_shader_parts[4] = m_uniform_block_access_string.c_str();
	m_geometry_shader_parts[5] = m_geometry_shader_code_body_end;

	out_geometry_shader_parts   = m_geometry_shader_parts;
	out_n_geometry_shader_parts = 6;

	/* Vertex Shader */
	out_vertex_shader_parts   = &m_vertex_shader_code;
	out_n_vertex_shader_parts = 1;
}

/** Get size of buffer used by transform feedback
 *
 *  @param out_buffer_size Size of buffer in bytes
 **/
void GeometryShaderMaxUniformBlocksTest::getTransformFeedbackBufferSize(unsigned int& out_buffer_size)
{
	out_buffer_size = m_buffer_size;
}

/** Prepare test specific program input for draw call
 *
 **/
void GeometryShaderMaxUniformBlocksTest::prepareProgramInput()
{
	/* Retrieve ES entry-points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Allocate memory */
	m_uniform_blocks.resize(m_max_uniform_blocks);

	/* Setup uniform blocks */
	for (glw::GLint i = 0; i < m_max_uniform_blocks; ++i)
	{
		/* Generate and bind */
		gl.genBuffers(1, &m_uniform_blocks[i].buffer_object_id);
		gl.bindBuffer(GL_UNIFORM_BUFFER, m_uniform_blocks[i].buffer_object_id);

		GLU_EXPECT_NO_ERROR(gl.getError(), "Failed create buffer object");

		/** Expected data is range <1;GL_MAX_GEOMETRY_UNIFORM_BLOCKS_EXT>
		 *  See test description for details
		 **/
		m_uniform_blocks[i].data = i + 1;

		gl.bufferData(GL_UNIFORM_BUFFER, sizeof(glw::GLint), &m_uniform_blocks[i].data, GL_STATIC_DRAW);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Failed to set buffer data");

		/* Bind buffer to uniform block */
		gl.bindBufferBase(GL_UNIFORM_BUFFER, i, m_uniform_blocks[i].buffer_object_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Failed to bind buffer to uniform block");
	}
}

/** Verification of results
 *
 *  @param result Pointer to data mapped from transform feedback buffer.
 *                Size of data is equal to buffer_size set by getTransformFeedbackBufferSize()
 *
 *  @return true  Result match expected value
 *          false Result has wrong value
 **/
bool GeometryShaderMaxUniformBlocksTest::verifyResult(const void* data)
{
	/* Expected data, sum of elements in range <x;y> with length n = ((x + y) / 2) * n */
	const glw::GLint expected_data = ((1 + m_max_uniform_blocks) * m_max_uniform_blocks) / 2;

	/* Cast to const GLint */
	const glw::GLint* transform_feedback_data = (const glw::GLint*)data;

	/* Verify data extracted from transfrom feedback */
	if (0 != memcmp(transform_feedback_data, &expected_data, m_buffer_size))
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Wrong result! Expected: " << expected_data
						   << " Extracted: " << *transform_feedback_data << tcu::TestLog::EndMessage;

		return false;
	}
	else
	{
		return true;
	}
}

/** Constructor
 *
 *  @param context       Test context
 *  @param name          Test case's name
 *  @param description   Test case's description
 **/
GeometryShaderMaxInputComponentsTest::GeometryShaderMaxInputComponentsTest(Context&				context,
																		   const ExtParameters& extParams,
																		   const char* name, const char* description)
	: GeometryShaderLimitsTransformFeedbackBase(context, extParams, name, description)
	, m_max_geometry_input_components(0)
	, m_max_geometry_input_vectors(0)
{
	/* Nothing to be done here */
}

/** Clears data after draw call and result verification
 *
 **/
void GeometryShaderMaxInputComponentsTest::clean()
{
	/* Nothing to be done here */
}

/** Get names and number of varyings to be captured by transform feedback
 *
 *  @param out_captured_varyings_names Array of varying names
 *  @param out_n_captured_varyings     Number of varying names
 **/
void GeometryShaderMaxInputComponentsTest::getCapturedVaryings(const glw::GLchar* const*& out_captured_varyings_names,
															   glw::GLuint&				  out_n_captured_varyings)
{
	/* Varying names */
	out_captured_varyings_names = &m_captured_varyings_names;

	/* Number of varyings */
	out_n_captured_varyings = 1;
}

/** Get parts of shaders
 *
 *  @param out_fragment_shader_parts   Array of fragment shader parts
 *  @param out_n_fragment_shader_parts Number of fragment shader parts
 *  @param out_geometry_shader_parts   Array of geometry shader parts
 *  @param out_n_geometry_shader_parts Number of geometry shader parts
 *  @param out_vertex_shader_parts     Array of vertex shader parts
 *  @param out_n_vertex_shader_parts   Number of vertex shader parts
 **/
void GeometryShaderMaxInputComponentsTest::getShaderParts(const glw::GLchar* const*& out_fragment_shader_parts,
														  unsigned int&				 out_n_fragment_shader_parts,
														  const glw::GLchar* const*& out_geometry_shader_parts,
														  unsigned int&				 out_n_geometry_shader_parts,
														  const glw::GLchar* const*& out_vertex_shader_parts,
														  unsigned int&				 out_n_vertex_shader_parts)
{
	/* Retrieve ES entry-points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Fragment Shader */
	out_fragment_shader_parts   = &m_fragment_shader_code;
	out_n_fragment_shader_parts = 1;

	/* Get maximum number of uniform */
	gl.getIntegerv(m_glExtTokens.MAX_GEOMETRY_INPUT_COMPONENTS, &m_max_geometry_input_components);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv() failed for GL_MAX_GEOMETRY_INPUT_COMPONENTS_EXT pname");

	m_max_geometry_input_vectors = m_max_geometry_input_components / 4 /* 4 components per vector */;

	std::stringstream stream;
	stream << m_max_geometry_input_vectors;
	m_max_geometry_input_vectors_string = stream.str();

	/* Geometry Shader */
	m_geometry_shader_parts[0] = m_geometry_shader_code_preamble;
	m_geometry_shader_parts[1] = m_geometry_shader_code_number_of_uniforms;
	m_geometry_shader_parts[2] = m_max_geometry_input_vectors_string.c_str();
	m_geometry_shader_parts[3] = m_geometry_shader_code_body;

	out_geometry_shader_parts   = m_geometry_shader_parts;
	out_n_geometry_shader_parts = 4;

	/* Vertex Shader */
	m_vertex_shader_parts[0] = m_vertex_shader_code_preamble;
	m_vertex_shader_parts[1] = m_vertex_shader_code_number_of_uniforms;
	m_vertex_shader_parts[2] = m_max_geometry_input_vectors_string.c_str();
	m_vertex_shader_parts[3] = m_vertex_shader_code_body;

	out_vertex_shader_parts   = m_vertex_shader_parts;
	out_n_vertex_shader_parts = 4;
}

/** Get size of buffer used by transform feedback
 *
 *  @param out_buffer_size  Size of buffer in bytes
 **/
void GeometryShaderMaxInputComponentsTest::getTransformFeedbackBufferSize(unsigned int& out_buffer_size)
{
	out_buffer_size = m_buffer_size;
}

/** Prepare test specific program input for draw call
 *
 **/
void GeometryShaderMaxInputComponentsTest::prepareProgramInput()
{
	/* Nothing to be done here */
}

/** Verification of results
 *
 *  @param result Pointer to data mapped from transform feedback buffer.
 *                Size of data is equal to buffer_size set by getTransformFeedbackBufferSize()
 *
 *  @return true  Result match expected value
 *          false Result has wrong value
 **/
bool GeometryShaderMaxInputComponentsTest::verifyResult(const void* data)
{
	/* Expected data, sum of elements in range <x;y> with length n = ((x + y) / 2) * n */
	const glw::GLint expected_data = ((1 + m_max_geometry_input_components) * m_max_geometry_input_components) / 2;

	/* Cast to const GLint */
	const glw::GLint* transform_feedback_data = (const glw::GLint*)data;

	/* Verify data extracted from transfrom feedback */
	if (0 != memcmp(transform_feedback_data, &expected_data, m_buffer_size))
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Wrong result! Expected: " << expected_data
						   << " Extracted: " << *transform_feedback_data << tcu::TestLog::EndMessage;

		return false;
	}
	else
	{
		return true;
	}
}

/** Constructor
 *
 * @param context       Test context
 * @param name          Test case's name
 * @param description   Test case's description
 **/
GeometryShaderMaxOutputComponentsTest::GeometryShaderMaxOutputComponentsTest(Context&			  context,
																			 const ExtParameters& extParams,
																			 const char* name, const char* description)
	: GeometryShaderLimitsRenderingBase(context, extParams, name, description)
	, m_texture_width(0)
	, m_max_output_components(0)
	, m_max_output_vectors(0)
	, m_max_total_output_components(0)
	, m_n_available_vectors(0)
	, m_n_output_points(0)
{
	/* Nothing to be done here */
}

/** Clears data after draw call and result verification
 *
 **/
void GeometryShaderMaxOutputComponentsTest::clean()
{
	/* Nothing to be done here */
}

/** Get details for draw call
 *
 *  @param out_primitive_type Type of primitive that will be used by next draw call
 *  @param out_n_vertices     Number of vertices that will used with next draw call
 **/
void GeometryShaderMaxOutputComponentsTest::getDrawCallDetails(glw::GLenum& out_primitive_type,
															   glw::GLuint& out_n_vertices)
{
	/* Draw one point */
	out_primitive_type = GL_POINTS;
	out_n_vertices	 = 1;
}

/** Get dimensions and format for texture bind to color attachment 0, get format and type for glReadPixels
 *
 *  @param out_texture_format      Format for texture used as color attachment 0
 *  @param out_texture_read_format Format of data used with glReadPixels
 *  @param out_texture_read_type   Type of data used with glReadPixels
 *  @param out_texture_width       Width of texture used as color attachment 0
 *  @param out_texture_height      Height of texture used as color attachment 0
 *  @param out_texture_pixel_size  Size of single pixel in bytes
 **/
void GeometryShaderMaxOutputComponentsTest::getFramebufferDetails(
	glw::GLenum& out_texture_format, glw::GLenum& out_texture_read_format, glw::GLenum& out_texture_read_type,
	glw::GLuint& out_texture_width, glw::GLuint& out_texture_height, unsigned int& out_texture_pixel_size)
{
	out_texture_format		= GL_R32I;
	out_texture_read_format = GL_RGBA_INTEGER;
	out_texture_read_type   = GL_INT;
	out_texture_width		= m_texture_width;
	out_texture_height		= m_texture_height;
	out_texture_pixel_size  = 4 * 4;
}

void GeometryShaderMaxOutputComponentsTest::getRequiredPointSize(glw::GLfloat& out_point_size)
{
	/* This test should only run if EXT_geometry_point_size is supported */
	if (!m_is_geometry_shader_point_size_supported)
	{
		throw tcu::NotSupportedError(GEOMETRY_SHADER_POINT_SIZE_NOT_SUPPORTED, "", __FILE__, __LINE__);
	}

	out_point_size = (float)m_point_size;
}

/** Get parts of shaders
 *
 *  @param out_fragment_shader_parts   Array of fragment shader parts
 *  @param out_n_fragment_shader_parts Number of fragment shader parts
 *  @param out_geometry_shader_parts   Array of geometry shader parts
 *  @param out_n_geometry_shader_parts Number of geometry shader parts
 *  @param out_vertex_shader_parts     Array of vertex shader parts
 *  @param out_n_vertex_shader_parts   Number of vertex shader parts
 **/
void GeometryShaderMaxOutputComponentsTest::getShaderParts(const glw::GLchar* const*& out_fragment_shader_parts,
														   unsigned int&			  out_n_fragment_shader_parts,
														   const glw::GLchar* const*& out_geometry_shader_parts,
														   unsigned int&			  out_n_geometry_shader_parts,
														   const glw::GLchar* const*& out_vertex_shader_parts,
														   unsigned int&			  out_n_vertex_shader_parts)
{
	/* GL functions */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Get maximum number of output components */
	gl.getIntegerv(m_glExtTokens.MAX_GEOMETRY_TOTAL_OUTPUT_COMPONENTS, &m_max_total_output_components);
	gl.getIntegerv(m_glExtTokens.MAX_GEOMETRY_OUTPUT_COMPONENTS, &m_max_output_components);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv() call(s) failed");

	m_n_output_points	 = m_max_total_output_components / m_max_output_components;
	m_max_output_vectors  = m_max_output_components / 4; /* 4 components per vector */
	m_n_available_vectors = m_max_output_vectors - 2;	/* 2 vectors are reserved for gl_Position and gl_PointSize */

	/* Framebuffer width */
	m_texture_width = m_point_size * m_n_output_points;

	/* Fragment shader parts */
	prepareFragmentShader(m_fragment_shader_code);

	m_fragment_shader_code_c_str = m_fragment_shader_code.c_str();
	out_fragment_shader_parts	= &m_fragment_shader_code_c_str;
	out_n_fragment_shader_parts  = 1;

	/* Geometry shader parts */
	prepareGeometryShader(m_geometry_shader_code);

	m_geometry_shader_code_c_str = m_geometry_shader_code.c_str();
	out_geometry_shader_parts	= &m_geometry_shader_code_c_str;
	out_n_geometry_shader_parts  = 1;

	/* Vertex shader */
	out_vertex_shader_parts   = &m_vertex_shader_code;
	out_n_vertex_shader_parts = 1;
}

/** Prepare test specific program input for draw call
 *
 **/
void GeometryShaderMaxOutputComponentsTest::prepareProgramInput()
{
	/* Nothing to be done here */
}

/** Verify rendered image
 *
 *  @param data Image to verify
 *
 *  @return true  Image pixels match expected values
 *          false Some pixels have wrong values
 **/
bool GeometryShaderMaxOutputComponentsTest::verifyResult(const void* data)
{
	const unsigned char* result_image			= (const unsigned char*)data;
	const unsigned int   line_size				= m_texture_width * m_texture_pixel_size;
	const glw::GLint	 n_components_per_point = m_n_available_vectors * 4; /* 4 components per vector */

	/* For each drawn point */
	for (glw::GLint point = 0; point < m_n_output_points; ++point)
	{
		const glw::GLint   first_value	= point * n_components_per_point + 1;
		const glw::GLint   last_value	 = (point + 1) * n_components_per_point;
		const glw::GLint   expected_value = ((first_value + last_value) * n_components_per_point) / 2;
		const unsigned int point_offset   = point * m_texture_pixel_size * m_point_size;

		/* Verify all pixels that belong to point, area m_point_size x m_point_size */
		for (unsigned int y = 0; y < m_point_size; ++y)
		{
			const unsigned int line_offset		  = y * line_size;
			const unsigned int first_texel_offset = line_offset + point_offset;

			for (unsigned int x = 0; x < m_point_size; ++x)
			{
				const unsigned int texel_offset = first_texel_offset + x * m_texture_pixel_size;

				if (0 != memcmp(result_image + texel_offset, &expected_value, sizeof(expected_value)))
				{
					glw::GLint* result_value = (glw::GLint*)(result_image + texel_offset);

					m_testCtx.getLog() << tcu::TestLog::Message << "Wrong result! Expected: " << expected_value
									   << " Extracted: " << *result_value << " Point: " << point << " X: " << x
									   << " Y: " << y << tcu::TestLog::EndMessage;

					return false;
				}
			}
		}
	}

	return true;
}

/** Prepare fragment shader code
 *
 *  @param out_shader_code String that will be used to store shaders code
 **/
void GeometryShaderMaxOutputComponentsTest::prepareFragmentShader(std::string& out_shader_code) const
{
	std::stringstream stream;

	stream << m_fragment_shader_code_preamble;
	stream << m_common_shader_code_gs_fs_out_definitions;

	for (int i = 0; i < m_n_available_vectors; ++i)
	{
		stream << m_fragment_shader_code_flat_in_ivec4 << " " << m_common_shader_code_gs_fs_out << i << ";\n";
	}

	stream << m_fragment_shader_code_body_begin;

	for (int i = 0; i < m_n_available_vectors; ++i)
	{
		stream << "    " << m_fragment_shader_code_sum << m_common_shader_code_gs_fs_out << i << ".x + "
			   << m_common_shader_code_gs_fs_out << i << ".y + " << m_common_shader_code_gs_fs_out << i << ".z + "
			   << m_common_shader_code_gs_fs_out << i << ".w;\n";
	}

	stream << m_fragment_shader_code_body_end;

	out_shader_code = stream.str();
}

/** Prepare geometry shader code
 *
 *  @param out_shader_code String that will be used to store shaders code
 **/
void GeometryShaderMaxOutputComponentsTest::prepareGeometryShader(std::string& out_shader_code) const
{
	std::stringstream stream;

	stream << m_geometry_shader_code_preamble;
	stream << m_common_shader_code_number_of_points;
	stream << m_n_output_points;
	stream << m_geometry_shader_code_layout;
	stream << m_common_shader_code_gs_fs_out_definitions;

	for (int i = 0; i < m_n_available_vectors; ++i)
	{
		stream << m_geometry_shader_code_flat_out_ivec4 << " " << m_common_shader_code_gs_fs_out << i << ";\n";
	}

	stream << m_geometry_shader_code_body_begin;

	for (int i = 0; i < m_n_available_vectors; ++i)
	{
		stream << "        " << m_common_shader_code_gs_fs_out << i << m_geometry_shader_code_assignment;
	}

	stream << m_geometry_shader_code_body_end;

	out_shader_code = stream.str();
}

/** Constructor
 *
 * @param context       Test context
 * @param name          Test case's name
 * @param description   Test case's description
 **/
GeometryShaderMaxOutputVerticesTest::GeometryShaderMaxOutputVerticesTest(Context&			  context,
																		 const ExtParameters& extParams,
																		 const char* name, const char* description)
	: TestCaseBase(context, extParams, name, description)
{
	/* Nothing to be done here */
}

/** Executes the test.
 *  Sets the test result to QP_TEST_RESULT_FAIL if the test failed, QP_TEST_RESULT_PASS otherwise.
 *
 *  Note the function throws exception should an error occur!
 *
 *  @return STOP if the test has finished, CONTINUE to indicate iterate should be called once again.
 *
 **/
tcu::TestCase::IterateResult GeometryShaderMaxOutputVerticesTest::iterate()
{
	/* This test should only run if EXT_geometry_shader is supported */
	if (!m_is_geometry_shader_extension_supported)
	{
		throw tcu::NotSupportedError(GEOMETRY_SHADER_EXTENSION_NOT_SUPPORTED, "", __FILE__, __LINE__);
	}

	/* GL */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Get maximum number of output vertices and prepare strings */
	glw::GLint  max_output_vertices;
	std::string valid_output_vertices_string;
	std::string invalid_output_vertices_string;

	gl.getIntegerv(m_glExtTokens.MAX_GEOMETRY_OUTPUT_VERTICES, &max_output_vertices);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv() call failed for GL_MAX_GEOMETRY_OUTPUT_VERTICES_EXT pname");

	std::stringstream stream_valid;
	stream_valid << max_output_vertices;
	valid_output_vertices_string = stream_valid.str();

	std::stringstream stream_invalid;
	stream_invalid << max_output_vertices + 1;
	invalid_output_vertices_string = stream_invalid.str();

	/* Geometry shader parts */
	const glw::GLchar* geometry_shader_valid_parts[] = { m_geometry_shader_code_preamble,
														 valid_output_vertices_string.c_str(),
														 m_geometry_shader_code_body };

	const glw::GLchar* geometry_shader_invalid_parts[] = { m_geometry_shader_code_preamble,
														   invalid_output_vertices_string.c_str(),
														   m_geometry_shader_code_body };

	/* Try to build programs */
	bool does_valid_build =
		doesProgramBuild(1, &m_fragment_shader_code, 3, geometry_shader_valid_parts, 1, &m_vertex_shader_code);

	bool does_invalid_build =
		doesProgramBuild(1, &m_fragment_shader_code, 3, geometry_shader_invalid_parts, 1, &m_vertex_shader_code);

	/* Verify results */
	if ((true == does_valid_build) && (false == does_invalid_build))
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		if (true != does_valid_build)
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "Failed to build valid program! GS::max_vertices "
														   "set to MAX_GEOMETRY_OUTPUT_VERTICES.\n"
							   << tcu::TestLog::EndMessage;
		}

		if (false != does_invalid_build)
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "Build of invalid program was successful! GS::max_vertices "
														   "set to MAX_GEOMETRY_OUTPUT_VERTICES + 1.\n"
							   << tcu::TestLog::EndMessage;
		}

		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
	}

	return STOP;
}

/** Constructor
 *
 * @param context       Test context
 * @param name          Test case's name
 * @param description   Test case's decsription
 **/
GeometryShaderMaxOutputComponentsSinglePointTest::GeometryShaderMaxOutputComponentsSinglePointTest(
	Context& context, const ExtParameters& extParams, const char* name, const char* description)
	: GeometryShaderLimitsRenderingBase(context, extParams, name, description)
	, m_max_output_components(0)
	, m_max_output_vectors(0)
	, m_n_available_vectors(0)
{
	/* Nothing to be done here */
}

/** Clears data after draw call and result verification
 *
 **/
void GeometryShaderMaxOutputComponentsSinglePointTest::clean()
{
	/* Nothing to be done here */
}

/** Get details for draw call
 *
 *  @param out_primitive_type Type of primitive that will be used by next draw call
 *  @param out_n_vertices     Number of vertices that will used with next draw call
 **/
void GeometryShaderMaxOutputComponentsSinglePointTest::getDrawCallDetails(glw::GLenum& out_primitive_type,
																		  glw::GLuint& out_n_vertices)
{
	/* Draw one point */
	out_primitive_type = GL_POINTS;
	out_n_vertices	 = 1;
}

/** Get dimensions and format for texture bind to color attachment 0, get format and type for glReadPixels
 *
 *  @param out_texture_format      Format for texture used as color attachment 0
 *  @param out_texture_read_format Format of data used with glReadPixels
 *  @param out_texture_read_type   Type of data used with glReadPixels
 *  @param out_texture_width       Width of texture used as color attachment 0
 *  @param out_texture_height      Height of texture used as color attachment 0
 *  @param out_texture_pixel_size  Size of single pixel in bytes
 **/
void GeometryShaderMaxOutputComponentsSinglePointTest::getFramebufferDetails(
	glw::GLenum& out_texture_format, glw::GLenum& out_texture_read_format, glw::GLenum& out_texture_read_type,
	glw::GLuint& out_texture_width, glw::GLuint& out_texture_height, unsigned int& out_texture_pixel_size)
{
	out_texture_format		= GL_R32I;
	out_texture_read_format = GL_RGBA_INTEGER;
	out_texture_read_type   = GL_INT;
	out_texture_width		= m_texture_width;
	out_texture_height		= m_texture_height;
	out_texture_pixel_size  = 4 * 4;
}

void GeometryShaderMaxOutputComponentsSinglePointTest::getRequiredPointSize(glw::GLfloat& out_point_size)
{
	/* This test should only run if EXT_geometry_point_size is supported */
	if (!m_is_geometry_shader_point_size_supported)
	{
		throw tcu::NotSupportedError(GEOMETRY_SHADER_POINT_SIZE_NOT_SUPPORTED, "", __FILE__, __LINE__);
	}

	out_point_size = (float)m_point_size;
}

/** Get parts of shaders
 *
 *  @param out_fragment_shader_parts   Array of fragment shader parts
 *  @param out_n_fragment_shader_parts Number of fragment shader parts
 *  @param out_geometry_shader_parts   Array of geometry shader parts
 *  @param out_n_geometry_shader_parts Number of geometry shader parts
 *  @param out_vertex_shader_parts     Array of vertex shader parts
 *  @param out_n_vertex_shader_parts   Number of vertex shader parts
 **/
void GeometryShaderMaxOutputComponentsSinglePointTest::getShaderParts(
	const glw::GLchar* const*& out_fragment_shader_parts, unsigned int& out_n_fragment_shader_parts,
	const glw::GLchar* const*& out_geometry_shader_parts, unsigned int& out_n_geometry_shader_parts,
	const glw::GLchar* const*& out_vertex_shader_parts, unsigned int& out_n_vertex_shader_parts)
{
	/* Retrieve ES entry-points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Get maximum number of output components */
	gl.getIntegerv(m_glExtTokens.MAX_GEOMETRY_OUTPUT_COMPONENTS, &m_max_output_components);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv() call failed for GL_MAX_GEOMETRY_OUTPUT_COMPONENTS_EXT pname");

	m_max_output_vectors  = m_max_output_components / 4; /* 4 components per vector */
	m_n_available_vectors = m_max_output_vectors - 2;	/* 2 vectors are reserved for gl_Position and gl_PointSize */

	/* Fragment shader parts */
	prepareFragmentShader(m_fragment_shader_code);

	m_fragment_shader_code_c_str = m_fragment_shader_code.c_str();
	out_fragment_shader_parts	= &m_fragment_shader_code_c_str;
	out_n_fragment_shader_parts  = 1;

	/* Geometry shader parts */
	prepareGeometryShader(m_geometry_shader_code);

	m_geometry_shader_code_c_str = m_geometry_shader_code.c_str();
	out_geometry_shader_parts	= &m_geometry_shader_code_c_str;
	out_n_geometry_shader_parts  = 1;

	/* Vertex shader */
	out_vertex_shader_parts   = &m_vertex_shader_code;
	out_n_vertex_shader_parts = 1;
}

/** Prepare test specific program input for draw call
 *
 **/
void GeometryShaderMaxOutputComponentsSinglePointTest::prepareProgramInput()
{
	/* Nothing to be done here */
}

/** Verify rendered image
 *
 *  @param data Image to verify
 *
 *  @return true  Image pixels match expected values
 *          false Some pixels have wrong values
 **/
bool GeometryShaderMaxOutputComponentsSinglePointTest::verifyResult(const void* data)
{
	const unsigned char* result_image			= (const unsigned char*)data;
	const unsigned int   line_size				= m_texture_width * m_texture_pixel_size;
	const glw::GLint	 n_components_per_point = m_n_available_vectors * 4; /* 4 components per vector */

	const glw::GLint first_value	= 1;
	const glw::GLint last_value		= n_components_per_point;
	const glw::GLint expected_value = ((first_value + last_value) * n_components_per_point) / 2;

	/* Verify all pixels that belong to point, area m_point_size x m_point_size */
	for (unsigned int y = 0; y < m_point_size; ++y)
	{
		const unsigned int line_offset = y * line_size;

		for (unsigned int x = 0; x < m_point_size; ++x)
		{
			const unsigned int texel_offset = line_offset + x * m_texture_pixel_size;

			if (0 != memcmp(result_image + texel_offset, &expected_value, sizeof(expected_value)))
			{
				const glw::GLint* result_value = (const glw::GLint*)(result_image + texel_offset);

				m_testCtx.getLog() << tcu::TestLog::Message << "Wrong result! Expected: " << expected_value
								   << " Extracted: " << *result_value << "  X: " << x << " Y: " << y
								   << tcu::TestLog::EndMessage;

				return false;
			}
		}
	}

	return true;
}

/** Prepare fragment shader code
 *
 *  @param out_shader_code String that will be used to store shaders code
 **/
void GeometryShaderMaxOutputComponentsSinglePointTest::prepareFragmentShader(std::string& out_shader_code) const
{
	std::stringstream stream;

	stream << m_fragment_shader_code_preamble;
	stream << m_common_shader_code_gs_fs_out_definitions;

	for (int i = 0; i < m_n_available_vectors; ++i)
	{
		stream << m_fragment_shader_code_flat_in_ivec4 << " " << m_common_shader_code_gs_fs_out << i << ";\n";
	}

	stream << m_fragment_shader_code_body_begin;

	for (int i = 0; i < m_n_available_vectors; ++i)
	{
		stream << "    " << m_fragment_shader_code_sum << m_common_shader_code_gs_fs_out << i << ".x + "
			   << m_common_shader_code_gs_fs_out << i << ".y + " << m_common_shader_code_gs_fs_out << i << ".z + "
			   << m_common_shader_code_gs_fs_out << i << ".w;\n";
	}

	stream << m_fragment_shader_code_body_end;

	out_shader_code = stream.str();
}

/** Prepare geometry shader code
 *
 *  @param out_shader_code String that will be used to store shaders code
 **/
void GeometryShaderMaxOutputComponentsSinglePointTest::prepareGeometryShader(std::string& out_shader_code) const
{
	std::stringstream stream;

	stream << m_geometry_shader_code_preamble;
	stream << m_common_shader_code_gs_fs_out_definitions;

	for (int i = 0; i < m_n_available_vectors; ++i)
	{
		stream << m_geometry_shader_code_flat_out_ivec4 << " " << m_common_shader_code_gs_fs_out << i << ";\n";
	}

	stream << m_geometry_shader_code_body_begin;

	for (int i = 0; i < m_n_available_vectors; ++i)
	{
		stream << "    " << m_common_shader_code_gs_fs_out << i << m_geometry_shader_code_assignment;
	}

	stream << m_geometry_shader_code_body_end;

	out_shader_code = stream.str();
}

/** Constructor
 *
 * @param context     Test context
 * @param name        Test case's name
 * @param description Test case's description
 **/
GeometryShaderMaxTextureUnitsTest::GeometryShaderMaxTextureUnitsTest(Context& context, const ExtParameters& extParams,
																	 const char* name, const char* description)
	: GeometryShaderLimitsRenderingBase(context, extParams, name, description)
	, m_texture_width(0)
	, m_max_texture_units(0)
{
	/* Nothing to be done here */
}

/** Clears data after draw call and result verification
 *
 **/
void GeometryShaderMaxTextureUnitsTest::clean()
{
	/* GL functions */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Bind 0 to all texture units */
	for (int i = 0; i < m_max_texture_units; ++i)
	{
		gl.activeTexture(GL_TEXTURE0 + i);
		gl.bindTexture(GL_TEXTURE_2D, 0);
	}
	gl.activeTexture(GL_TEXTURE0);

	/* Delete textures */
	for (int i = 0; i < m_max_texture_units; ++i)
	{
		gl.deleteTextures(1, &m_textures[i].texture_id);
	}

	m_textures.clear();
}

/** Get details for draw call
 *
 *  @param out_primitive_type Type of primitive that will be used by next draw call
 *  @param out_n_vertices     Number of vertices that will used with next draw call
 **/
void GeometryShaderMaxTextureUnitsTest::getDrawCallDetails(glw::GLenum& out_primitive_type, glw::GLuint& out_n_vertices)
{
	/* Draw GL_MAX_GEOMETRY_TEXTURE_IMAGE_UNITS_EXT points */
	out_primitive_type = GL_POINTS;
	out_n_vertices	 = m_max_texture_units;
}

/** Get dimensions and format for texture bind to color attachment 0, get format and type for glReadPixels
 *
 *  @param out_texture_format      Format for texture used as color attachment 0
 *  @param out_texture_read_format Format of data used with glReadPixels
 *  @param out_texture_read_type   Type of data used with glReadPixels
 *  @param out_texture_width       Width of texture used as color attachment 0
 *  @param out_texture_height      Height of texture used as color attachment 0
 *  @param out_texture_pixel_size  Size of single pixel in bytes
 **/
void GeometryShaderMaxTextureUnitsTest::getFramebufferDetails(
	glw::GLenum& out_texture_format, glw::GLenum& out_texture_read_format, glw::GLenum& out_texture_read_type,
	glw::GLuint& out_texture_width, glw::GLuint& out_texture_height, unsigned int& out_texture_pixel_size)
{
	out_texture_format		= GL_R32I;
	out_texture_read_format = GL_RGBA_INTEGER;
	out_texture_read_type   = GL_INT;
	out_texture_width		= m_texture_width;
	out_texture_height		= m_texture_height;
	out_texture_pixel_size  = 4 * 4;
}

void GeometryShaderMaxTextureUnitsTest::getRequiredPointSize(glw::GLfloat& out_point_size)
{
	/* This test should only run if EXT_geometry_point_size is supported */
	if (!m_is_geometry_shader_point_size_supported)
	{
		throw tcu::NotSupportedError(GEOMETRY_SHADER_POINT_SIZE_NOT_SUPPORTED, "", __FILE__, __LINE__);
	}

	out_point_size = (float)m_point_size;
}

/** Get parts of shaders
 *
 *  @param out_fragment_shader_parts   Array of fragment shader parts
 *  @param out_n_fragment_shader_parts Number of fragment shader parts
 *  @param out_geometry_shader_parts   Array of geometry shader parts
 *  @param out_n_geometry_shader_parts Number of geometry shader parts
 *  @param out_vertex_shader_parts     Array of vertex shader parts
 *  @param out_n_vertex_shader_parts   Number of vertex shader parts
 **/
void GeometryShaderMaxTextureUnitsTest::getShaderParts(const glw::GLchar* const*& out_fragment_shader_parts,
													   unsigned int&			  out_n_fragment_shader_parts,
													   const glw::GLchar* const*& out_geometry_shader_parts,
													   unsigned int&			  out_n_geometry_shader_parts,
													   const glw::GLchar* const*& out_vertex_shader_parts,
													   unsigned int&			  out_n_vertex_shader_parts)
{
	/* Retrieve ES entry-points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Get maximum number of texture units */
	gl.getIntegerv(m_glExtTokens.MAX_GEOMETRY_TEXTURE_IMAGE_UNITS, &m_max_texture_units);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv() failed for GL_MAX_GEOMETRY_TEXTURE_IMAGE_UNITS_EXT pname");

	/* Number of drawn points is equal to GL_MAX_GEOMETRY_TEXTURE_IMAGE_UNITS_EXT */
	m_texture_width = m_max_texture_units * m_point_size;

	/* Prepare texture units string */
	std::stringstream stream;
	stream << m_max_texture_units;
	m_max_texture_units_string = stream.str();

	/* Fragment shader parts */
	out_fragment_shader_parts   = &m_fragment_shader_code;
	out_n_fragment_shader_parts = 1;

	/* Geometry shader parts */
	m_geometry_shader_parts[0] = m_geometry_shader_code_preamble;
	m_geometry_shader_parts[1] = m_max_texture_units_string.c_str();
	m_geometry_shader_parts[2] = m_geometry_shader_code_body;

	out_geometry_shader_parts   = m_geometry_shader_parts;
	out_n_geometry_shader_parts = 3;

	/* Vertex shader parts */
	m_vertex_shader_parts[0] = m_vertex_shader_code_preamble;
	m_vertex_shader_parts[1] = m_max_texture_units_string.c_str();
	m_vertex_shader_parts[2] = m_vertex_shader_code_body;

	out_vertex_shader_parts   = m_vertex_shader_parts;
	out_n_vertex_shader_parts = 3;
}

/** Prepare test specific program input for draw call
 *
 **/
void GeometryShaderMaxTextureUnitsTest::prepareProgramInput()
{
	/* Retrieve ES entry-points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	m_textures.resize(m_max_texture_units);

	/* Prepare texture storage and fill data */
	for (int i = 0; i < m_max_texture_units; ++i)
	{
		/* (starting from 1, delta: 2) */
		m_textures[i].data = i * 2 + 1;

		/* Generate and bind texture */
		gl.genTextures(1, &m_textures[i].texture_id);
		gl.bindTexture(GL_TEXTURE_2D, m_textures[i].texture_id);

		GLU_EXPECT_NO_ERROR(gl.getError(), "Failed to create texture");

		/* Allocate and upload texture data */
		gl.texImage2D(GL_TEXTURE_2D, 0 /* level */, GL_R32I, 1 /* width */, 1 /* height */, 0 /* border */,
					  GL_RED_INTEGER, GL_INT, &m_textures[i].data);

		gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		GLU_EXPECT_NO_ERROR(gl.getError(), "Failed to create storage and fill texture with data");
	}

	/* Prepare sampler uniforms */
	for (int i = 0; i < m_max_texture_units; ++i)
	{
		/* Prepare name of sampler */
		std::stringstream stream;

		stream << "gs_texture[" << i << "]";

		/* Get sampler location */
		glw::GLint gs_texture_location = gl.getUniformLocation(m_program_object_id, stream.str().c_str());

		if (-1 == gs_texture_location || (GL_NO_ERROR != gl.getError()))
		{
			TCU_FAIL("Failed to get uniform isampler2D location");
		}

		/* Set uniform at sampler location value to index of texture unit */
		gl.uniform1i(gs_texture_location, i);

		if (GL_NO_ERROR != gl.getError())
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "Failed to set uniform at location: " << gs_texture_location
							   << " to value: " << i << tcu::TestLog::EndMessage;

			TCU_FAIL("Failed to get uniform isampler2D location");
		}
	}

	/* Bind textures to texture units */
	for (int i = 0; i < m_max_texture_units; ++i)
	{
		gl.activeTexture(GL_TEXTURE0 + i);
		gl.bindTexture(GL_TEXTURE_2D, m_textures[i].texture_id);
	}

	GLU_EXPECT_NO_ERROR(gl.getError(), "Failed to set texture units up");
}

/** Verify rendered image
 *
 *  @param data Image to verify
 *
 *  @return true  Image pixels match expected values
 *          false Some pixels have wrong values
 **/
bool GeometryShaderMaxTextureUnitsTest::verifyResult(const void* data)
{
	const unsigned char* result_image = (const unsigned char*)data;
	const unsigned int   line_size	= m_texture_width * m_texture_pixel_size;

	/* For each drawn point */
	for (glw::GLint point = 0; point < m_max_texture_units; ++point)
	{
		const glw::GLint   first_value	= m_textures[0].data;
		const glw::GLint   last_value	 = m_textures[point].data;
		const glw::GLint   expected_value = ((first_value + last_value) * (point + 1)) / 2;
		const unsigned int point_offset   = point * m_texture_pixel_size * m_point_size;

		/* Verify all pixels that belong to point, area m_point_size x m_point_size */
		for (unsigned int y = 0; y < m_point_size; ++y)
		{
			const unsigned int line_offset		  = y * line_size;
			const unsigned int first_texel_offset = line_offset + point_offset;

			for (unsigned int x = 0; x < m_point_size; ++x)
			{
				const unsigned int texel_offset = first_texel_offset + x * m_texture_pixel_size;

				if (0 != memcmp(result_image + texel_offset, &expected_value, sizeof(expected_value)))
				{
					glw::GLint* result_value = (glw::GLint*)(result_image + texel_offset);

					m_testCtx.getLog() << tcu::TestLog::Message << "Wrong result! "
																   "Expected: "
									   << expected_value << " Extracted: " << *result_value << " Point: " << point
									   << " X: " << x << " Y: " << y << tcu::TestLog::EndMessage;

					return false;
				}
			}
		}
	}

	return true;
}

/** Constructor
 *
 * @param context     Test context
 * @param name        Test case's name
 * @param description Test case's description
 **/
GeometryShaderMaxInvocationsTest::GeometryShaderMaxInvocationsTest(Context& context, const ExtParameters& extParams,
																   const char* name, const char* description)
	: TestCaseBase(context, extParams, name, description)
	, m_fragment_shader_id_for_multiple_invocations_pass(0)
	, m_geometry_shader_id_for_multiple_invocations_pass(0)
	, m_program_object_id_for_multiple_invocations_pass(0)
	, m_vertex_shader_id_for_multiple_invocations_pass(0)
	, m_fragment_shader_id_for_single_invocation_pass(0)
	, m_geometry_shader_id_for_single_invocation_pass(0)
	, m_program_object_id_for_single_invocation_pass(0)
	, m_vertex_shader_id_for_single_invocation_pass(0)
	, m_max_geometry_shader_invocations(0)
	, m_framebuffer_object_id(0)
	, m_color_texture_id(0)
	, m_texture_width(0)
	, m_vertex_array_object_id(0)
{
	/* Nothing to be done here */
}

/** Initializes GLES objects used during the test.
 *
 */
void GeometryShaderMaxInvocationsTest::initTest()
{
	/* This test should only run if EXT_geometry_shader is supported */
	if (!m_is_geometry_shader_extension_supported)
	{
		throw tcu::NotSupportedError(GEOMETRY_SHADER_EXTENSION_NOT_SUPPORTED, "", __FILE__, __LINE__);
	}

	/* Retrieve ES entry-points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Get GL_MAX_GEOMETRY_SHADER_INVOCATIONS_EXT */
	gl.getIntegerv(m_glExtTokens.MAX_GEOMETRY_SHADER_INVOCATIONS, &m_max_geometry_shader_invocations);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv() call failed for GL_MAX_GEOMETRY_SHADER_INVOCATIONS_EXT");

	/* Prepare string for GL_MAX_GEOMETRY_SHADER_INVOCATIONS_EXT */
	std::stringstream stream;
	stream << m_max_geometry_shader_invocations;
	m_max_geometry_shader_invocations_string = stream.str();

	/* Prepare gemetry shader parts for multiple invocations pass */
	const glw::GLuint n_geometry_shader_parts_for_multiple_invocations_pass = 5;

	m_geometry_shader_parts_for_multiple_invocations_pass[0] = m_geometry_shader_code_preamble;
	m_geometry_shader_parts_for_multiple_invocations_pass[1] = m_max_geometry_shader_invocations_string.c_str();
	m_geometry_shader_parts_for_multiple_invocations_pass[2] = m_geometry_shader_code_layout;
	m_geometry_shader_parts_for_multiple_invocations_pass[3] = m_geometry_shader_code_layout_invocations;
	m_geometry_shader_parts_for_multiple_invocations_pass[4] = m_geometry_shader_code_body;

	/* Prepare gemetry shader parts for single invocation pass */
	const glw::GLuint n_geometry_shader_parts_for_single_invocation_pass = 4;

	m_geometry_shader_parts_for_single_invocation_pass[0] = m_geometry_shader_code_preamble;
	m_geometry_shader_parts_for_single_invocation_pass[1] = m_max_geometry_shader_invocations_string.c_str();
	m_geometry_shader_parts_for_single_invocation_pass[2] = m_geometry_shader_code_layout;
	m_geometry_shader_parts_for_single_invocation_pass[3] = m_geometry_shader_code_body;

	/* Create program and shaders for multiple GS invocations */
	m_program_object_id_for_multiple_invocations_pass = gl.createProgram();

	m_fragment_shader_id_for_multiple_invocations_pass = gl.createShader(GL_FRAGMENT_SHADER);
	m_geometry_shader_id_for_multiple_invocations_pass = gl.createShader(m_glExtTokens.GEOMETRY_SHADER);
	m_vertex_shader_id_for_multiple_invocations_pass   = gl.createShader(GL_VERTEX_SHADER);

	/* Create program and shaders for single GS invocations */
	m_program_object_id_for_single_invocation_pass = gl.createProgram();

	m_fragment_shader_id_for_single_invocation_pass = gl.createShader(GL_FRAGMENT_SHADER);
	m_geometry_shader_id_for_single_invocation_pass = gl.createShader(m_glExtTokens.GEOMETRY_SHADER);
	m_vertex_shader_id_for_single_invocation_pass   = gl.createShader(GL_VERTEX_SHADER);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Failed to create program or shader objects");

	/* Build program for multiple GS invocations */
	if (false == buildProgram(m_program_object_id_for_multiple_invocations_pass,
							  m_fragment_shader_id_for_multiple_invocations_pass, 1, &m_fragment_shader_code,
							  m_geometry_shader_id_for_multiple_invocations_pass,
							  n_geometry_shader_parts_for_multiple_invocations_pass,
							  m_geometry_shader_parts_for_multiple_invocations_pass,
							  m_vertex_shader_id_for_multiple_invocations_pass, 1, &m_vertex_shader_code))
	{
		TCU_FAIL("Could not create program from valid vertex/geometry/fragment shader");
	}

	/* Build program for single GS invocations */
	if (false == buildProgram(m_program_object_id_for_single_invocation_pass,
							  m_fragment_shader_id_for_single_invocation_pass, 1, &m_fragment_shader_code,
							  m_geometry_shader_id_for_single_invocation_pass,
							  n_geometry_shader_parts_for_single_invocation_pass,
							  m_geometry_shader_parts_for_single_invocation_pass,
							  m_vertex_shader_id_for_single_invocation_pass, 1, &m_vertex_shader_code))
	{
		TCU_FAIL("Could not create program from valid vertex/geometry/fragment shader");
	}

	/* Set up texture object and a FBO */
	gl.genTextures(1, &m_color_texture_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not create texture object");

	gl.genFramebuffers(1, &m_framebuffer_object_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not create framebuffer object");

	m_texture_width = m_triangle_edge_length * m_max_geometry_shader_invocations;

	if (false == setupFramebufferWithTextureAsAttachment(m_framebuffer_object_id, m_color_texture_id, GL_RGBA8,
														 m_texture_width, m_texture_height))
	{
		TCU_FAIL("Failed to setup framebuffer");
	}

	/* Set up a vertex array object */
	gl.genVertexArrays(1, &m_vertex_array_object_id);
	gl.bindVertexArray(m_vertex_array_object_id);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not create vertex array object");
}

/** Executes the test.
 *
 *  Sets the test result to QP_TEST_RESULT_FAIL if the test failed, QP_TEST_RESULT_PASS otherwise.
 *
 *  Note the function throws exception should an error occur!
 *
 *  @return STOP if the test has finished, CONTINUE to indicate iterate should be called once again.
 **/
tcu::TestCase::IterateResult GeometryShaderMaxInvocationsTest::iterate()
{
	initTest();

	/* Variables used for image verification purposes */
	std::vector<unsigned char> result_image(m_texture_width * m_texture_height * m_texture_pixel_size);

	/* Retrieve ES entry-points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Render with multiple GS invocations */
	gl.useProgram(m_program_object_id_for_multiple_invocations_pass);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not use program");

	gl.clearColor(255 /* red */, 0 /* green */, 0 /* blue */, 0 /* alpha */);
	gl.clear(GL_COLOR_BUFFER_BIT);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not clear the color buffer");

	gl.drawArrays(GL_POINTS, 0 /* first */, 1 /* count */);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Call drawArrays() failed");

	/* Extract image from FBO */
	gl.readPixels(0 /* x */, 0 /* y */, m_texture_width, m_texture_height, GL_RGBA, GL_UNSIGNED_BYTE, &result_image[0]);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not read back pixels from color buffer");

	/* Run verification */
	bool result_of_multiple_invocations_pass = verifyResultOfMultipleInvocationsPass(&result_image[0]);

	/* Render with single GS invocations */
	gl.useProgram(m_program_object_id_for_single_invocation_pass);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not use program");

	gl.clearColor(255 /* red */, 0 /* green */, 0 /* blue */, 0 /* alpha */);
	gl.clear(GL_COLOR_BUFFER_BIT);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not clear the color buffer");

	gl.drawArrays(GL_POINTS, 0 /* first */, 1 /* count */);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Call drawArrays() failed");

	/* Extract image from FBO */
	gl.readPixels(0 /* x */, 0 /* y */, m_texture_width, m_texture_height, GL_RGBA, GL_UNSIGNED_BYTE, &result_image[0]);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not read back pixels from color buffer");

	/* Run verification */
	bool result_of_single_invocation_pass = verifyResultOfSingleInvocationPass(&result_image[0]);

	/* Set test result */
	if (result_of_multiple_invocations_pass && result_of_single_invocation_pass)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
	}

	return STOP;
}

/** Deinitializes GLES objects created during the test.
 *
 */
void GeometryShaderMaxInvocationsTest::deinit()
{
	/* Retrieve ES entry-points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Reset OpenGL ES state */
	gl.useProgram(0);
	gl.bindVertexArray(0);
	gl.framebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0 /* texture */, 0 /* level */);
	gl.bindTexture(GL_TEXTURE_2D, 0);
	gl.bindFramebuffer(GL_FRAMEBUFFER, 0);

	/* Delete everything */
	if (m_program_object_id_for_multiple_invocations_pass != 0)
	{
		gl.deleteProgram(m_program_object_id_for_multiple_invocations_pass);

		m_program_object_id_for_multiple_invocations_pass = 0;
	}

	if (m_fragment_shader_id_for_multiple_invocations_pass != 0)
	{
		gl.deleteShader(m_fragment_shader_id_for_multiple_invocations_pass);

		m_fragment_shader_id_for_multiple_invocations_pass = 0;
	}

	if (m_geometry_shader_id_for_multiple_invocations_pass != 0)
	{
		gl.deleteShader(m_geometry_shader_id_for_multiple_invocations_pass);

		m_geometry_shader_id_for_multiple_invocations_pass = 0;
	}

	if (m_vertex_shader_id_for_multiple_invocations_pass != 0)
	{
		gl.deleteShader(m_vertex_shader_id_for_multiple_invocations_pass);

		m_vertex_shader_id_for_multiple_invocations_pass = 0;
	}

	if (m_program_object_id_for_single_invocation_pass != 0)
	{
		gl.deleteProgram(m_program_object_id_for_single_invocation_pass);

		m_program_object_id_for_single_invocation_pass = 0;
	}

	if (m_fragment_shader_id_for_single_invocation_pass != 0)
	{
		gl.deleteShader(m_fragment_shader_id_for_single_invocation_pass);

		m_fragment_shader_id_for_single_invocation_pass = 0;
	}

	if (m_geometry_shader_id_for_single_invocation_pass != 0)
	{
		gl.deleteShader(m_geometry_shader_id_for_single_invocation_pass);

		m_geometry_shader_id_for_single_invocation_pass = 0;
	}

	if (m_vertex_shader_id_for_single_invocation_pass != 0)
	{
		gl.deleteShader(m_vertex_shader_id_for_single_invocation_pass);

		m_vertex_shader_id_for_single_invocation_pass = 0;
	}

	if (m_vertex_array_object_id != 0)
	{
		gl.deleteVertexArrays(1, &m_vertex_array_object_id);

		m_vertex_array_object_id = 0;
	}

	if (m_color_texture_id != 0)
	{
		gl.deleteTextures(1, &m_color_texture_id);

		m_color_texture_id = 0;
	}

	if (m_framebuffer_object_id != 0)
	{
		gl.deleteFramebuffers(1, &m_framebuffer_object_id);

		m_framebuffer_object_id = 0;
	}

	/* Deinitilize base class */
	TestCaseBase::deinit();
}

/** Verify image rendered during draw call for multiple invocations pass
 *
 *  @param result_image Image data
 *
 *  @return true  When image is as expected
 *          false When image is wrong
 **/
bool GeometryShaderMaxInvocationsTest::verifyResultOfMultipleInvocationsPass(unsigned char* result_image)
{
	for (unsigned int i = 0; i < (unsigned int)m_max_geometry_shader_invocations; ++i)
	{
		/* Verify that pixel at triangle's center was modified */
		const unsigned int x1 = m_triangle_edge_length * i;
		const unsigned int x2 = m_triangle_edge_length * i;
		const unsigned int x3 = m_triangle_edge_length * (i + 1) - 1;

		const unsigned int y1 = 0;
		const unsigned int y2 = m_triangle_edge_length - 1;
		const unsigned int y3 = m_triangle_edge_length - 1;

		const unsigned int center_x = (x1 + x2 + x3) / 3;
		const unsigned int center_y = (y1 + y2 + y3) / 3;

		bool is_pixel_valid = comparePixel(result_image, center_x, center_y, m_texture_width, m_texture_height,
										   m_texture_pixel_size, 0, 255, 0, 0);

		if (false == is_pixel_valid)
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "Invalid pixel at "
														   "["
							   << center_x << ";" << center_y << "]! "
																 "Triangle index: "
							   << i << " from range <0:" << m_max_geometry_shader_invocations << ")."
							   << tcu::TestLog::EndMessage;

			return false;
		}

		/* Verify that background's pixel was not modified */
		const unsigned int x4 = m_triangle_edge_length * (i + 1) - 1;
		const unsigned int y4 = m_triangle_edge_length - 1;

		is_pixel_valid =
			comparePixel(result_image, x4, y4, m_texture_width, m_texture_height, m_texture_pixel_size, 255, 0, 0, 0);

		if (false == is_pixel_valid)
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "Invalid pixel at [" << x4 << ";" << y4
							   << "]! "
								  "Background for index: "
							   << i << "from range <0:" << m_max_geometry_shader_invocations << ")."
							   << tcu::TestLog::EndMessage;

			return false;
		}
	}

	return true;
}

/** Verify image rendered during draw call for single invocation pass
 *
 *  @param result_image Image data
 *
 *  @return true  When image is as expected
 *          false When image is wrong
 **/
bool GeometryShaderMaxInvocationsTest::verifyResultOfSingleInvocationPass(unsigned char* result_image)
{
	/* Only one triangle should be drawn, verify that pixel at its center was modified */
	{
		const unsigned int x1 = 0;
		const unsigned int x2 = 0;
		const unsigned int x3 = m_triangle_edge_length - 1;

		const unsigned int y1 = 0;
		const unsigned int y2 = m_triangle_edge_length - 1;
		const unsigned int y3 = m_triangle_edge_length - 1;

		const unsigned int center_x = (x1 + x2 + x3) / 3;
		const unsigned int center_y = (y1 + y2 + y3) / 3;

		bool is_pixel_valid = comparePixel(result_image, center_x, center_y, m_texture_width, m_texture_height,
										   m_texture_pixel_size, 0, 255, 0, 0);

		if (false == is_pixel_valid)
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "Invalid pixel at [" << center_x << ";" << center_y
							   << "]! "
								  "Triangle index: "
							   << 0 << " from range <0:" << m_max_geometry_shader_invocations << ")."
							   << tcu::TestLog::EndMessage;

			return false;
		}

		/* Verify that background's pixel was not modified */
		const unsigned int x4 = m_triangle_edge_length - 1;
		const unsigned int y4 = m_triangle_edge_length - 1;

		is_pixel_valid =
			comparePixel(result_image, x4, y4, m_texture_width, m_texture_height, m_texture_pixel_size, 255, 0, 0, 0);

		if (false == is_pixel_valid)
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "Invalid pixel at [" << x4 << ";" << y4
							   << "]! "
								  "Background for index: "
							   << 0 << " from range <0:" << m_max_geometry_shader_invocations << ")."
							   << tcu::TestLog::EndMessage;

			return false;
		}
	}

	for (unsigned int i = 1; i < (unsigned int)m_max_geometry_shader_invocations; ++i)
	{
		/* Verify that pixel at triangle's center was not modified */
		const unsigned int x1 = m_triangle_edge_length * i;
		const unsigned int x2 = m_triangle_edge_length * i;
		const unsigned int x3 = m_triangle_edge_length * (i + 1) - 1;

		const unsigned int y1 = 0;
		const unsigned int y2 = m_triangle_edge_length - 1;
		const unsigned int y3 = m_triangle_edge_length - 1;

		const unsigned int center_x = (x1 + x2 + x3) / 3;
		const unsigned int center_y = (y1 + y2 + y3) / 3;

		bool is_pixel_valid = comparePixel(result_image, center_x, center_y, m_texture_width, m_texture_height,
										   m_texture_pixel_size, 255, 0, 0, 0);

		if (false == is_pixel_valid)
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "Invalid pixel at [" << center_x << ";" << center_y
							   << "]! "
								  "Triangle index: "
							   << i << " from range <0:" << m_max_geometry_shader_invocations << ")."
							   << tcu::TestLog::EndMessage;

			return false;
		}

		/* Verify that background's pixel was not modified */
		const unsigned int x4 = m_triangle_edge_length * (i + 1) - 1;
		const unsigned int y4 = m_triangle_edge_length - 1;

		is_pixel_valid =
			comparePixel(result_image, x4, y4, m_texture_width, m_texture_height, m_texture_pixel_size, 255, 0, 0, 0);

		if (false == is_pixel_valid)
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "Invalid pixel at [" << x4 << ";" << y4
							   << "]! "
								  "Background for index: "
							   << i << " from range <0:" << m_max_geometry_shader_invocations << ")."
							   << tcu::TestLog::EndMessage;

			return false;
		}
	}

	return true;
}

/** Constructor
 *
 * @param context       Test context
 * @param name          Test case's name
 * @param description   Test case's description
 **/
GeometryShaderMaxCombinedTextureUnitsTest::GeometryShaderMaxCombinedTextureUnitsTest(Context&			  context,
																					 const ExtParameters& extParams,
																					 const char*		  name,
																					 const char*		  description)
	: GeometryShaderLimitsRenderingBase(context, extParams, name, description)
	, m_texture_width(0)
	, m_max_combined_texture_units(0)
	, m_max_fragment_texture_units(0)
	, m_max_geometry_texture_units(0)
	, m_max_vertex_texture_units(0)
	, m_min_texture_units(0)
	, m_n_fragment_texture_units(0)
	, m_n_geometry_texture_units(0)
	, m_n_texture_units(0)
	, m_n_vertex_texture_units(0)
{
	/* Nothing to be done here */
}

/** Clears data after draw call and result verification
 *
 **/
void GeometryShaderMaxCombinedTextureUnitsTest::clean()
{
	/* GL functions */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Bind 0 to all texture units */
	for (int i = 0; i < m_n_texture_units; ++i)
	{
		gl.activeTexture(GL_TEXTURE0 + i);
		gl.bindTexture(GL_TEXTURE_2D, 0);
	}

	/* Delete textures */
	for (int i = 0; i < m_n_texture_units; ++i)
	{
		gl.deleteTextures(1, &m_textures[i].texture_id);
	}

	m_textures.clear();
}

/** Get details for draw call
 *
 *  @param out_primitive_type Type of primitive that will be used by next draw call
 *  @param out_n_vertices     Number of vertices that will used with next draw call
 **/
void GeometryShaderMaxCombinedTextureUnitsTest::getDrawCallDetails(glw::GLenum& out_primitive_type,
																   glw::GLuint& out_n_vertices)
{
	/* Draw GL_MAX_GEOMETRY_TEXTURE_IMAGE_UNITS_EXT points */
	out_primitive_type = GL_POINTS;
	out_n_vertices	 = m_min_texture_units;
}

/** Get dimensions and format for texture bind to color attachment 0, get format and type for glReadPixels
 *
 *  @param out_texture_format      Format for texture used as color attachment 0
 *  @param out_texture_read_format Format of data used with glReadPixels
 *  @param out_texture_read_type   Type of data used with glReadPixels
 *  @param out_texture_width       Width of texture used as color attachment 0
 *  @param out_texture_height      Height of texture used as color attachment 0
 *  @param out_texture_pixel_size  Size of single pixel in bytes
 **/
void GeometryShaderMaxCombinedTextureUnitsTest::getFramebufferDetails(
	glw::GLenum& out_texture_format, glw::GLenum& out_texture_read_format, glw::GLenum& out_texture_read_type,
	glw::GLuint& out_texture_width, glw::GLuint& out_texture_height, unsigned int& out_texture_pixel_size)
{
	out_texture_format		= GL_R32UI;
	out_texture_read_format = GL_RGBA_INTEGER;
	out_texture_read_type   = GL_UNSIGNED_INT;
	out_texture_width		= m_texture_width;
	out_texture_height		= m_texture_height;
	out_texture_pixel_size  = 4 * 4;
}

void GeometryShaderMaxCombinedTextureUnitsTest::getRequiredPointSize(glw::GLfloat& out_point_size)
{
	out_point_size = (float)m_point_size;
}

/** Get parts of shaders
 *
 *  @param out_fragment_shader_parts   Array of fragment shader parts
 *  @param out_n_fragment_shader_parts Number of fragment shader parts
 *  @param out_geometry_shader_parts   Array of geometry shader parts
 *  @param out_n_geometry_shader_parts Number of geometry shader parts
 *  @param out_vertex_shader_parts     Array of vertex shader parts
 *  @param out_n_vertex_shader_parts   Number of vertex shader parts
 **/
void GeometryShaderMaxCombinedTextureUnitsTest::getShaderParts(const glw::GLchar* const*& out_fragment_shader_parts,
															   unsigned int&			  out_n_fragment_shader_parts,
															   const glw::GLchar* const*& out_geometry_shader_parts,
															   unsigned int&			  out_n_geometry_shader_parts,
															   const glw::GLchar* const*& out_vertex_shader_parts,
															   unsigned int&			  out_n_vertex_shader_parts)
{
	/* GL functions */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Get maximum number of texture units */
	gl.getIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &m_max_combined_texture_units);
	gl.getIntegerv(GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS, &m_max_vertex_texture_units);
	gl.getIntegerv(m_glExtTokens.MAX_GEOMETRY_TEXTURE_IMAGE_UNITS, &m_max_geometry_texture_units);
	gl.getIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &m_max_fragment_texture_units);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv() call(s) failed");

	m_n_texture_units =
		de::max(m_max_vertex_texture_units, de::max(m_max_geometry_texture_units, m_max_fragment_texture_units));
	m_n_vertex_texture_units = de::max(1, de::min(m_max_combined_texture_units - 2, m_max_vertex_texture_units));
	m_n_fragment_texture_units =
		de::max(1, de::min(m_max_combined_texture_units - m_n_vertex_texture_units - 1, m_max_fragment_texture_units));
	m_n_geometry_texture_units =
		de::max(1, de::min(m_max_combined_texture_units - m_n_vertex_texture_units - m_n_fragment_texture_units,
						   m_max_geometry_texture_units));
	m_min_texture_units =
		de::min(m_n_vertex_texture_units, de::min(m_n_fragment_texture_units, m_n_geometry_texture_units));

	/* Number of drawn points is equal to GL_MAX_GEOMETRY_TEXTURE_IMAGE_UNITS_EXT */
	m_texture_width = m_n_texture_units * m_point_size;

	/* Prepare texture units string */
	std::stringstream stream_fragment;
	stream_fragment << m_n_fragment_texture_units;
	m_n_fragment_texture_units_string = stream_fragment.str();

	std::stringstream stream_geometry;
	stream_geometry << m_n_geometry_texture_units;
	m_n_geometry_texture_units_string = stream_geometry.str();

	std::stringstream stream_vertex;
	stream_vertex << m_n_vertex_texture_units;
	m_n_vertex_texture_units_string = stream_vertex.str();

	/* Fragment shader parts */
	m_fragment_shader_parts[0] = m_fragment_shader_code_preamble;
	m_fragment_shader_parts[1] = m_n_fragment_texture_units_string.c_str();
	m_fragment_shader_parts[2] = m_fragment_shader_code_body;

	out_fragment_shader_parts   = m_fragment_shader_parts;
	out_n_fragment_shader_parts = 3;

	/* Geometry shader parts */
	m_geometry_shader_parts[0] = m_geometry_shader_code_preamble;
	m_geometry_shader_parts[1] = m_n_geometry_texture_units_string.c_str();
	m_geometry_shader_parts[2] = m_geometry_shader_code_body;

	out_geometry_shader_parts   = m_geometry_shader_parts;
	out_n_geometry_shader_parts = 3;

	/* Vertex shader parts */
	m_vertex_shader_parts[0] = m_vertex_shader_code_preamble;
	m_vertex_shader_parts[1] = m_n_vertex_texture_units_string.c_str();
	m_vertex_shader_parts[2] = m_vertex_shader_code_body;

	out_vertex_shader_parts   = m_vertex_shader_parts;
	out_n_vertex_shader_parts = 3;
}

/** Prepare test specific program input for draw call
 *
 **/
void GeometryShaderMaxCombinedTextureUnitsTest::prepareProgramInput()
{
	/* Retrieve ES entry-points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	m_textures.resize(m_n_texture_units);

	/* Prepare texture storage and fill data */
	for (int i = 0; i < m_n_texture_units; ++i)
	{
		/* unique intensity equal to index of the texture */
		m_textures[i].data = i;

		/* Generate and bind texture */
		gl.genTextures(1, &m_textures[i].texture_id);
		gl.bindTexture(GL_TEXTURE_2D, m_textures[i].texture_id);

		GLU_EXPECT_NO_ERROR(gl.getError(), "Failed to create texture");

		/* Allocate and upload texture data */
		gl.texImage2D(GL_TEXTURE_2D, 0 /* level */, GL_R32UI, 1 /* width*/, 1 /* height */, 0 /* border */,
					  GL_RED_INTEGER, GL_UNSIGNED_INT, &m_textures[i].data);

		gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		GLU_EXPECT_NO_ERROR(gl.getError(), "Failed to create storage and fill texture with data");
	}

	/* Prepare sampler uniforms */
	for (int i = 0; i < m_n_texture_units; ++i)
	{
		/* Prepare name of sampler */
		std::stringstream stream;

		stream << "sampler[" << i << "]";

		/* Get sampler location */
		glw::GLint sampler_location = gl.getUniformLocation(m_program_object_id, stream.str().c_str());

		if (-1 == sampler_location || GL_NO_ERROR != gl.getError())
		{
			TCU_FAIL("Failed to get uniform usampler2D location");
		}

		/* Set uniform at sampler location value to index of texture unit */
		gl.uniform1i(sampler_location, i);

		if (GL_NO_ERROR != gl.getError())
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "Failed to set uniform at location: " << sampler_location
							   << " to value: " << i << tcu::TestLog::EndMessage;

			TCU_FAIL("Failed to get uniform isampler2D location");
		}
	}

	/* Bind textures to texture units */
	for (int i = 0; i < m_n_texture_units; ++i)
	{
		gl.activeTexture(GL_TEXTURE0 + i);
		gl.bindTexture(GL_TEXTURE_2D, m_textures[i].texture_id);

		gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	}

	GLU_EXPECT_NO_ERROR(gl.getError(), "Failed to set texture units up");
}

/** Verify rendered image
 *
 *  @param data Image to verify
 *
 *  @return true  Image pixels match expected values
 *          false Some pixels have wrong values
 **/
bool GeometryShaderMaxCombinedTextureUnitsTest::verifyResult(const void* data)
{
	const unsigned char* result_image = (const unsigned char*)data;
	const unsigned int   line_size	= m_texture_width * m_texture_pixel_size;

	/* For each drawn point */
	for (glw::GLint point = 0; point < m_n_texture_units; ++point)
	{
		const unsigned int last_vertex_index = de::min(point, m_n_vertex_texture_units);

		glw::GLint expected_vertex_value   = 0;
		glw::GLint expected_geometry_value = 0;
		glw::GLint expected_fragment_value = 0;

		for (unsigned int i = 0; i < last_vertex_index; ++i)
		{
			expected_vertex_value += m_textures[i].data;
		}

		for (unsigned int i = 0; i < last_vertex_index; ++i)
		{
			expected_geometry_value += m_textures[i].data;
		}

		for (unsigned int i = 0; i < last_vertex_index; ++i)
		{
			expected_fragment_value += m_textures[i].data;
		}

		const glw::GLint   expected_value = expected_vertex_value + expected_geometry_value + expected_fragment_value;
		const unsigned int point_offset   = point * m_texture_pixel_size * m_point_size;

		/* Verify all pixels that belong to point, area m_point_size x m_point_size */
		for (unsigned int y = 0; y < m_point_size; ++y)
		{
			const unsigned int line_offset		  = y * line_size;
			const unsigned int first_texel_offset = line_offset + point_offset;

			for (unsigned int x = 0; x < m_point_size; ++x)
			{
				const unsigned int texel_offset = first_texel_offset + x * m_texture_pixel_size;

				if (0 != memcmp(result_image + texel_offset, &expected_value, sizeof(expected_value)))
				{
					glw::GLint* result_value = (glw::GLint*)(result_image + texel_offset);

					m_testCtx.getLog() << tcu::TestLog::Message << "Wrong result!"
																   " Expected: "
									   << expected_value << " Extracted: " << *result_value << " Point: " << point
									   << " X: " << x << " Y: " << y << tcu::TestLog::EndMessage;

					return false;
				}
			}
		}
	}

	return true;
}

} /* glcts */
