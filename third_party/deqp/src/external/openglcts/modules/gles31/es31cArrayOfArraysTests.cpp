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

#include "es31cArrayOfArraysTests.hpp"
#include "gluContextInfo.hpp"
#include "gluDefs.hpp"
#include "gluRenderContext.hpp"
#include "glwFunctions.hpp"
#include "tcuTestLog.hpp"

#include <algorithm>
#include <cassert>
#include <cstdio>
#include <string>
using std::string;

/* Selects if debug output is enabled */
#define IS_DEBUG 0
#define IS_DEBUG_DUMP_ALL_SHADERS 0

/* Selects if workaround in ExpressionsInvalid2 test is enabled */
#define WRKARD_EXPRESSIONSINVALID2 0

#if IS_DEBUG
#include "tcuTestLog.hpp"
#endif

namespace glcts
{
namespace ArraysOfArrays
{
namespace Interface
{
/* Sets limits on number of dimensions. Value 8 comes from ogirinal "ES" implementation.
 * Explanation was as follows:
 *
 *     "The current specifations allow up to 8 array dimensions."
 */
const size_t ES::MAX_ARRAY_DIMENSIONS = 8;
const size_t GL::MAX_ARRAY_DIMENSIONS = 8;

/* API specific shader parts */
const char* ES::shader_version_gpu5 =
	"#version 310 es\n#extension GL_EXT_gpu_shader5 : require\nprecision mediump float;\n\n";
const char* ES::shader_version = "#version 310 es\nprecision mediump float;\n\n";

const char* GL::shader_version_gpu5 = "#version 430 core\n\n";
const char* GL::shader_version		= "#version 430 core\n\n";
} /* namespace Interface */

/* Dummy fragment shader source code.
 * Used when testing the vertex shader. */
const std::string default_fragment_shader_source = "//default fragment shader\n"
												   "out vec4 color;\n"
												   "void main()\n"
												   "{\n"
												   "    color = vec4(1.0);\n"
												   "}\n";

/* Dummy vertex shader source code.
 * Used when testing the fragment shader. */
const std::string default_vertex_shader_source = "//default vertex shader\n"
												 "\n"
												 "void main()\n"
												 "{\n"
												 "    gl_Position = vec4(0.0,0.0,0.0,1.0);\n"
												 "}\n";

/* Dummy geometry shader source code.
 * Used when testing the other shaders. */
const std::string default_geometry_shader_source = "//default geometry\n"
												   "\n"
												   "void main()\n"
												   "{\n"
												   "    gl_Position  = vec4(-1, -1, 0, 1);\n"
												   "    EmitVertex();\n"
												   "    gl_Position  = vec4(-1, 1, 0, 1);\n"
												   "    EmitVertex();\n"
												   "    gl_Position  = vec4(1, -1, 0, 1);\n"
												   "    EmitVertex();\n"
												   "    gl_Position  = vec4(1, 1, 0, 1);\n"
												   "    EmitVertex();\n"
												   "}\n";

/* Dummy tesselation control shader source code.
 * Used when testing the other shaders. */
const std::string default_tc_shader_source = "//default tcs\n"
											 "\n"
											 "void main()\n"
											 "{\n"
											 "    gl_TessLevelOuter[0] = 1.0;\n"
											 "    gl_TessLevelOuter[1] = 1.0;\n"
											 "    gl_TessLevelOuter[2] = 1.0;\n"
											 "    gl_TessLevelOuter[3] = 1.0;\n"
											 "    gl_TessLevelInner[0] = 1.0;\n"
											 "    gl_TessLevelInner[1] = 1.0;\n"
											 "}\n";

/* Dummy tesselation evaluation shader source code.
 * Used when testing the other shaders. */
const std::string default_te_shader_source = "//default tes\n"
											 "\n"
											 "void main()\n"
											 "{\n"
											 "}\n";

/* Pass-through shaders source code. Used when testing other stage. */
const std::string pass_fragment_shader_source = "//pass fragment shader\n"
												"in float fs_result;\n"
												"out vec4 color;\n"
												"\n"
												"void main()\n"
												"{\n"
												"    color = vec4(fs_result);\n"
												"}\n";

const std::string pass_geometry_shader_source = "//pass geometry\n"
												"in  float gs_result[];\n"
												"out float fs_result;\n"
												"\n"
												"void main()\n"
												"{\n"
												"    gl_Position  = vec4(-1, -1, 0, 1);\n"
												"    fs_result = gs_result[0];\n"
												"    EmitVertex();\n"
												"    gl_Position  = vec4(-1, 1, 0, 1);\n"
												"    fs_result = gs_result[0];\n"
												"    EmitVertex();\n"
												"    gl_Position  = vec4(1, -1, 0, 1);\n"
												"    fs_result = gs_result[0];\n"
												"    EmitVertex();\n"
												"    gl_Position  = vec4(1, 1, 0, 1);\n"
												"    fs_result = gs_result[0];\n"
												"    EmitVertex();\n"
												"}\n";

const std::string pass_te_shader_source = "//pass tes\n"
										  "in  float tcs_result[];\n"
										  "out float fs_result;\n"
										  "\n"
										  "void main()\n"
										  "{\n"
										  "    fs_result = tcs_result[0];\n"
										  "}\n";

/* Empty string */
static const std::string empty_string = "";

/* Beginning of a shader source code. */
const std::string shader_start = "void main()\n"
								 "{\n";

/* End of a shader source code. */
const std::string shader_end = "}\n";

/* Emit full screen quad from GS */
const std::string emit_quad = "    gl_Position  = vec4(-1, -1, 0, 1);\n"
							  "    EmitVertex();\n"
							  "    gl_Position  = vec4(-1, 1, 0, 1);\n"
							  "    EmitVertex();\n"
							  "    gl_Position  = vec4(1, -1, 0, 1);\n"
							  "    EmitVertex();\n"
							  "    gl_Position  = vec4(1, 1, 0, 1);\n"
							  "    EmitVertex();\n";

/* Set tesselation levels */
const std::string set_tesseation = "    gl_TessLevelOuter[0] = 1.0;\n"
								   "    gl_TessLevelOuter[1] = 1.0;\n"
								   "    gl_TessLevelOuter[2] = 1.0;\n"
								   "    gl_TessLevelOuter[3] = 1.0;\n"
								   "    gl_TessLevelInner[0] = 1.0;\n"
								   "    gl_TessLevelInner[1] = 1.0;\n";

/* Input and output data type modifiers. */
const std::string in_out_type_modifiers[] = { "in", "out", "uniform" };

/* Types and appropriate initialisers, used throughout these tests */
const var_descriptor var_descriptors[] = {
	{ "bool", "", "true", "false", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A" },
	{ "int", "", "1", "0", "0", "int", "", "iterator", "1", "N/A", "N/A" },
	{ "uint", "", "1u", "0u", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A" },
	{ "float", "", "1.0", "0.0", "0.0", "float", "", "iterator", "1.0", "N/A", "N/A" },
	{ "vec2", "", "vec2(1.0)", "vec2(0.0)", "0.0", "float", "[0]", "vec2(iterator)", "vec2(1.0)", "N/A", "N/A" },
	{ "vec3", "", "vec3(1.0)", "vec3(0.0)", "0.0", "float", "[0]", "vec3(iterator)", "vec3(1.0)", "N/A", "N/A" },
	{ "vec4", "", "vec4(1.0)", "vec4(0.0)", "0.0", "float", "[0]", "vec4(iterator)", "vec4(1.0)", "N/A", "N/A" },
	{ "bvec2", "", "bvec2(1)", "bvec2(0)", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A" },
	{ "bvec3", "", "bvec3(1)", "bvec3(0)", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A" },
	{ "bvec4", "", "bvec4(1)", "bvec4(0)", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A" },
	{ "ivec2", "", "ivec2(1)", "ivec2(0)", "0", "int", "[0]", "ivec2(iterator)", "ivec2(1)", "N/A", "N/A" },
	{ "ivec3", "", "ivec3(1)", "ivec3(0)", "0", "int", "[0]", "ivec3(iterator)", "ivec3(1)", "N/A", "N/A" },
	{ "ivec4", "", "ivec4(1)", "ivec4(0)", "0", "int", "[0]", "ivec4(iterator)", "ivec4(1)", "N/A", "N/A" },
	{ "uvec2", "", "uvec2(1u)", "uvec2(0u)", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A" },
	{ "uvec3", "", "uvec3(1u)", "uvec3(0u)", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A" },
	{ "uvec4", "", "uvec4(1u)", "uvec4(0u)", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A" },
	{ "mat2", "", "mat2(1.0)", "mat2(0.0)", "0.0", "float", "[0][0]", "mat2(iterator)", "mat2(1.0)", "N/A", "N/A" },
	{ "mat3", "", "mat3(1.0)", "mat3(0.0)", "0.0", "float", "[0][0]", "mat3(iterator)", "mat3(1.0)", "N/A", "N/A" },
	{ "mat4", "", "mat4(1.0)", "mat4(0.0)", "0.0", "float", "[0][0]", "mat4(iterator)", "mat4(1.0)", "N/A", "N/A" },
	{ "mat2x2", "", "mat2x2(1.0)", "mat2x2(0.0)", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A" },
	{ "mat2x3", "", "mat2x3(1.0)", "mat2x3(0.0)", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A" },
	{ "mat2x4", "", "mat2x4(1.0)", "mat2x4(0.0)", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A" },
	{ "mat3x2", "", "mat3x2(1.0)", "mat3x2(0.0)", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A" },
	{ "mat3x3", "", "mat3x3(1.0)", "mat3x3(0.0)", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A" },
	{ "mat3x4", "", "mat3x4(1.0)", "mat3x4(0.0)", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A" },
	{ "mat4x2", "", "mat4x2(1.0)", "mat4x2(0.0)", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A" },
	{ "mat4x3", "", "mat4x3(1.0)", "mat4x3(0.0)", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A" },
	{ "mat4x4", "", "mat4x4(1.0)", "mat4x4(0.0)", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A" },
	{ "imageBuffer", "", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A" },
	{ "iimageBuffer", "", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A" },
	{ "uimageBuffer", "", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A" },
	{ "samplerBuffer", "lowp", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A" },
	{ "isamplerBuffer", "lowp", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A" },
	{ "usamplerBuffer", "lowp", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A" },
	{ "sampler2D", "", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "vec2(0.0)", "vec4" },
	{ "sampler3D", "lowp", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "vec3(0.0)", "vec4" },
	{ "samplerCube", "", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "vec3(0.0)", "vec4" },
	{
		"samplerCubeShadow", "lowp", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "vec4(0.0)", "float",
	},
	{ "sampler2DShadow", "lowp", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "vec3(0.0)", "float" },
	{ "sampler2DArray", "lowp", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "vec3(0.0)", "vec4" },
	{ "sampler2DArrayShadow", "lowp", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "vec4(0.0)", "float" },
	{ "isampler2D", "lowp", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "vec2(0.0)", "ivec4" },
	{ "isampler3D", "lowp", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "vec3(0.0)", "ivec4" },
	{ "isamplerCube", "lowp", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "vec3(0.0)", "ivec4" },
	{ "isampler2DArray", "lowp", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "vec3(0.0)", "ivec4" },
	{ "usampler2D", "lowp", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "vec2(0.0)", "uvec4" },
	{ "usampler3D", "lowp", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "vec3(0.0)", "uvec4" },
	{ "usamplerCube", "lowp", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "vec3(0.0)", "uvec4" },
	{ "usampler2DArray", "lowp", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "vec3(0.0)", "uvec4" },
};

const var_descriptor var_double_descriptors[] = {
	{ "double", "", "1.0", "0.0", "0.0", "double", "", "iterator", "1.0", "N/A", "N/A" },
	{ "dmat2", "", "dmat2(1.0)", "dmat2(0.0)", "0.0", "double", "[0][0]", "dmat2(iterator)", "dmat2(1.0)", "N/A",
	  "N/A" },
	{ "dmat3", "", "dmat3(1.0)", "dmat3(0.0)", "0.0", "double", "[0][0]", "dmat3(iterator)", "dmat3(1.0)", "N/A",
	  "N/A" },
	{ "dmat4", "", "dmat4(1.0)", "dmat4(0.0)", "0.0", "double", "[0][0]", "dmat4(iterator)", "dmat4(1.0)", "N/A",
	  "N/A" },
	{ "dmat2x2", "", "dmat2x2(1.0)", "dmat2x2(0.0)", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A" },
	{ "dmat2x3", "", "dmat2x3(1.0)", "dmat2x3(0.0)", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A" },
	{ "dmat2x4", "", "dmat2x4(1.0)", "dmat2x4(0.0)", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A" },
	{ "dmat3x2", "", "dmat3x2(1.0)", "dmat3x2(0.0)", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A" },
	{ "dmat3x3", "", "dmat3x3(1.0)", "dmat3x3(0.0)", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A" },
	{ "dmat3x4", "", "dmat3x4(1.0)", "dmat3x4(0.0)", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A" },
	{ "dmat4x2", "", "dmat4x2(1.0)", "dmat4x2(0.0)", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A" },
	{ "dmat4x3", "", "dmat4x3(1.0)", "dmat4x3(0.0)", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A" },
	{ "dmat4x4", "", "dmat4x4(1.0)", "dmat4x4(0.0)", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A" },
};

_supported_variable_types_map supported_variable_types_map;

/** List of all supported variable types for es. */
const test_var_type var_types_es[] = {
	VAR_TYPE_BOOL,   VAR_TYPE_INT,	VAR_TYPE_UINT,   VAR_TYPE_FLOAT,  VAR_TYPE_VEC2,   VAR_TYPE_VEC3,
	VAR_TYPE_VEC4,   VAR_TYPE_BVEC2,  VAR_TYPE_BVEC3,  VAR_TYPE_BVEC4,  VAR_TYPE_IVEC2,  VAR_TYPE_IVEC3,
	VAR_TYPE_IVEC4,  VAR_TYPE_UVEC2,  VAR_TYPE_UVEC3,  VAR_TYPE_UVEC4,  VAR_TYPE_MAT2,   VAR_TYPE_MAT3,
	VAR_TYPE_MAT4,   VAR_TYPE_MAT2X2, VAR_TYPE_MAT2X3, VAR_TYPE_MAT2X4, VAR_TYPE_MAT3X2, VAR_TYPE_MAT3X3,
	VAR_TYPE_MAT3X4, VAR_TYPE_MAT4X2, VAR_TYPE_MAT4X3, VAR_TYPE_MAT4X4,
};

const test_var_type* Interface::ES::var_types   = var_types_es;
const size_t		 Interface::ES::n_var_types = sizeof(var_types_es) / sizeof(var_types_es[0]);

/** List of all supported variable types for gl. */
static const glcts::test_var_type var_types_gl[] = {
	VAR_TYPE_BOOL,	VAR_TYPE_INT,		VAR_TYPE_UINT,	VAR_TYPE_FLOAT,   VAR_TYPE_VEC2,	VAR_TYPE_VEC3,
	VAR_TYPE_VEC4,	VAR_TYPE_BVEC2,   VAR_TYPE_BVEC3,   VAR_TYPE_BVEC4,   VAR_TYPE_IVEC2,   VAR_TYPE_IVEC3,
	VAR_TYPE_IVEC4,   VAR_TYPE_UVEC2,   VAR_TYPE_UVEC3,   VAR_TYPE_UVEC4,   VAR_TYPE_MAT2,	VAR_TYPE_MAT3,
	VAR_TYPE_MAT4,	VAR_TYPE_MAT2X2,  VAR_TYPE_MAT2X3,  VAR_TYPE_MAT2X4,  VAR_TYPE_MAT3X2,  VAR_TYPE_MAT3X3,
	VAR_TYPE_MAT3X4,  VAR_TYPE_MAT4X2,  VAR_TYPE_MAT4X3,  VAR_TYPE_MAT4X4,  VAR_TYPE_DOUBLE,  VAR_TYPE_DMAT2,
	VAR_TYPE_DMAT3,   VAR_TYPE_DMAT4,   VAR_TYPE_DMAT2X2, VAR_TYPE_DMAT2X3, VAR_TYPE_DMAT2X4, VAR_TYPE_DMAT3X2,
	VAR_TYPE_DMAT3X3, VAR_TYPE_DMAT3X4, VAR_TYPE_DMAT4X2, VAR_TYPE_DMAT4X3, VAR_TYPE_DMAT4X4,
};

const test_var_type* Interface::GL::var_types   = var_types_gl;
const size_t		 Interface::GL::n_var_types = sizeof(var_types_gl) / sizeof(var_types_gl[0]);

/** List of all supported opaque types. */
const glcts::test_var_type opaque_var_types[] = {
	//Floating Point Sampler Types (opaque)
	VAR_TYPE_SAMPLER2D, VAR_TYPE_SAMPLER3D, VAR_TYPE_SAMPLERCUBE, VAR_TYPE_SAMPLERCUBESHADOW, VAR_TYPE_SAMPLER2DSHADOW,
	VAR_TYPE_SAMPLER2DARRAY, VAR_TYPE_SAMPLER2DARRAYSHADOW,
	//Signed Integer Sampler Types (opaque)
	VAR_TYPE_ISAMPLER2D, VAR_TYPE_ISAMPLER3D, VAR_TYPE_ISAMPLERCUBE, VAR_TYPE_ISAMPLER2DARRAY,
	//Unsigned Integer Sampler Types (opaque)
	VAR_TYPE_USAMPLER2D, VAR_TYPE_USAMPLER3D, VAR_TYPE_USAMPLERCUBE, VAR_TYPE_USAMPLER2DARRAY,
};

/** Sets up the type map that will be used to look up the type names, initialisation
 *  values, etc., associated with each of the types used within the array tests
 *
 **/
template <class API>
void initializeMap()
{
	int temp_index = 0;

	// Set up the map
	supported_variable_types_map[VAR_TYPE_BOOL]					= var_descriptors[temp_index++];
	supported_variable_types_map[VAR_TYPE_INT]					= var_descriptors[temp_index++];
	supported_variable_types_map[VAR_TYPE_UINT]					= var_descriptors[temp_index++];
	supported_variable_types_map[VAR_TYPE_FLOAT]				= var_descriptors[temp_index++];
	supported_variable_types_map[VAR_TYPE_VEC2]					= var_descriptors[temp_index++];
	supported_variable_types_map[VAR_TYPE_VEC3]					= var_descriptors[temp_index++];
	supported_variable_types_map[VAR_TYPE_VEC4]					= var_descriptors[temp_index++];
	supported_variable_types_map[VAR_TYPE_BVEC2]				= var_descriptors[temp_index++];
	supported_variable_types_map[VAR_TYPE_BVEC3]				= var_descriptors[temp_index++];
	supported_variable_types_map[VAR_TYPE_BVEC4]				= var_descriptors[temp_index++];
	supported_variable_types_map[VAR_TYPE_IVEC2]				= var_descriptors[temp_index++];
	supported_variable_types_map[VAR_TYPE_IVEC3]				= var_descriptors[temp_index++];
	supported_variable_types_map[VAR_TYPE_IVEC4]				= var_descriptors[temp_index++];
	supported_variable_types_map[VAR_TYPE_UVEC2]				= var_descriptors[temp_index++];
	supported_variable_types_map[VAR_TYPE_UVEC3]				= var_descriptors[temp_index++];
	supported_variable_types_map[VAR_TYPE_UVEC4]				= var_descriptors[temp_index++];
	supported_variable_types_map[VAR_TYPE_MAT2]					= var_descriptors[temp_index++];
	supported_variable_types_map[VAR_TYPE_MAT3]					= var_descriptors[temp_index++];
	supported_variable_types_map[VAR_TYPE_MAT4]					= var_descriptors[temp_index++];
	supported_variable_types_map[VAR_TYPE_MAT2X2]				= var_descriptors[temp_index++];
	supported_variable_types_map[VAR_TYPE_MAT2X3]				= var_descriptors[temp_index++];
	supported_variable_types_map[VAR_TYPE_MAT2X4]				= var_descriptors[temp_index++];
	supported_variable_types_map[VAR_TYPE_MAT3X2]				= var_descriptors[temp_index++];
	supported_variable_types_map[VAR_TYPE_MAT3X3]				= var_descriptors[temp_index++];
	supported_variable_types_map[VAR_TYPE_MAT3X4]				= var_descriptors[temp_index++];
	supported_variable_types_map[VAR_TYPE_MAT4X2]				= var_descriptors[temp_index++];
	supported_variable_types_map[VAR_TYPE_MAT4X3]				= var_descriptors[temp_index++];
	supported_variable_types_map[VAR_TYPE_MAT4X4]				= var_descriptors[temp_index++];
	supported_variable_types_map[VAR_TYPE_IMAGEBUFFER]			= var_descriptors[temp_index++];
	supported_variable_types_map[VAR_TYPE_IIMAGEBUFFER]			= var_descriptors[temp_index++];
	supported_variable_types_map[VAR_TYPE_UIMAGEBUFFER]			= var_descriptors[temp_index++];
	supported_variable_types_map[VAR_TYPE_SAMPLERBUFFER]		= var_descriptors[temp_index++];
	supported_variable_types_map[VAR_TYPE_ISAMPLERBUFFER]		= var_descriptors[temp_index++];
	supported_variable_types_map[VAR_TYPE_USAMPLERBUFFER]		= var_descriptors[temp_index++];
	supported_variable_types_map[VAR_TYPE_SAMPLER2D]			= var_descriptors[temp_index++];
	supported_variable_types_map[VAR_TYPE_SAMPLER3D]			= var_descriptors[temp_index++];
	supported_variable_types_map[VAR_TYPE_SAMPLERCUBE]			= var_descriptors[temp_index++];
	supported_variable_types_map[VAR_TYPE_SAMPLERCUBESHADOW]	= var_descriptors[temp_index++];
	supported_variable_types_map[VAR_TYPE_SAMPLER2DSHADOW]		= var_descriptors[temp_index++];
	supported_variable_types_map[VAR_TYPE_SAMPLER2DARRAY]		= var_descriptors[temp_index++];
	supported_variable_types_map[VAR_TYPE_SAMPLER2DARRAYSHADOW] = var_descriptors[temp_index++];
	supported_variable_types_map[VAR_TYPE_ISAMPLER2D]			= var_descriptors[temp_index++];
	supported_variable_types_map[VAR_TYPE_ISAMPLER3D]			= var_descriptors[temp_index++];
	supported_variable_types_map[VAR_TYPE_ISAMPLERCUBE]			= var_descriptors[temp_index++];
	supported_variable_types_map[VAR_TYPE_ISAMPLER2DARRAY]		= var_descriptors[temp_index++];
	supported_variable_types_map[VAR_TYPE_USAMPLER2D]			= var_descriptors[temp_index++];
	supported_variable_types_map[VAR_TYPE_USAMPLER3D]			= var_descriptors[temp_index++];
	supported_variable_types_map[VAR_TYPE_USAMPLERCUBE]			= var_descriptors[temp_index++];
	supported_variable_types_map[VAR_TYPE_USAMPLER2DARRAY]		= var_descriptors[temp_index++];

	if (API::USE_DOUBLE)
	{
		temp_index = 0;

		supported_variable_types_map[VAR_TYPE_DOUBLE]  = var_double_descriptors[temp_index++];
		supported_variable_types_map[VAR_TYPE_DMAT2]   = var_double_descriptors[temp_index++];
		supported_variable_types_map[VAR_TYPE_DMAT3]   = var_double_descriptors[temp_index++];
		supported_variable_types_map[VAR_TYPE_DMAT4]   = var_double_descriptors[temp_index++];
		supported_variable_types_map[VAR_TYPE_DMAT2X2] = var_double_descriptors[temp_index++];
		supported_variable_types_map[VAR_TYPE_DMAT2X3] = var_double_descriptors[temp_index++];
		supported_variable_types_map[VAR_TYPE_DMAT2X4] = var_double_descriptors[temp_index++];
		supported_variable_types_map[VAR_TYPE_DMAT3X2] = var_double_descriptors[temp_index++];
		supported_variable_types_map[VAR_TYPE_DMAT3X3] = var_double_descriptors[temp_index++];
		supported_variable_types_map[VAR_TYPE_DMAT3X4] = var_double_descriptors[temp_index++];
		supported_variable_types_map[VAR_TYPE_DMAT4X2] = var_double_descriptors[temp_index++];
		supported_variable_types_map[VAR_TYPE_DMAT4X3] = var_double_descriptors[temp_index++];
		supported_variable_types_map[VAR_TYPE_DMAT4X4] = var_double_descriptors[temp_index++];
	}
}

/** Macro appends default ending of main function to source string
 *
 * @param SOURCE Tested shader source
 **/
#define DEFAULT_MAIN_ENDING(TYPE, SOURCE)                           \
	{                                                               \
		/* Apply stage specific stuff */                            \
		switch (TYPE)                                               \
		{                                                           \
		case TestCaseBase<API>::VERTEX_SHADER_TYPE:                 \
			SOURCE += "\n	gl_Position = vec4(0.0);\n";            \
			break;                                                  \
		case TestCaseBase<API>::FRAGMENT_SHADER_TYPE:               \
			break;                                                  \
		case TestCaseBase<API>::COMPUTE_SHADER_TYPE:                \
			break;                                                  \
		case TestCaseBase<API>::GEOMETRY_SHADER_TYPE:               \
			SOURCE += emit_quad;                                    \
			break;                                                  \
		case TestCaseBase<API>::TESSELATION_CONTROL_SHADER_TYPE:    \
			SOURCE += set_tesseation;                               \
			break;                                                  \
		case TestCaseBase<API>::TESSELATION_EVALUATION_SHADER_TYPE: \
			break;                                                  \
		default:                                                    \
			TCU_FAIL("Unrecognized shader type.");                  \
			break;                                                  \
		}                                                           \
                                                                    \
		/* End main function */                                     \
		SOURCE += shader_end;                                       \
	}

/** Macro executes positive test selected on USE_ALL_SHADER_STAGES
 *
 * @param TYPE	Tested shader stage
 * @param SOURCE Tested shader source
 * @param DELETE Selects if program should be deleted afterwards
 **/
#define EXECUTE_POSITIVE_TEST(TYPE, SOURCE, DELETE, GPU5)                              \
	{                                                                                  \
		const std::string* cs  = &empty_string;                                        \
		const std::string* vs  = &default_vertex_shader_source;                        \
		const std::string* tcs = &default_tc_shader_source;                            \
		const std::string* tes = &default_te_shader_source;                            \
		const std::string* gs  = &default_geometry_shader_source;                      \
		const std::string* fs  = &default_fragment_shader_source;                      \
                                                                                       \
		switch (TYPE)                                                                  \
		{                                                                              \
		case TestCaseBase<API>::COMPUTE_SHADER_TYPE:                                   \
			cs  = &SOURCE;                                                             \
			vs  = &empty_string;                                                       \
			tcs = &empty_string;                                                       \
			tes = &empty_string;                                                       \
			gs  = &empty_string;                                                       \
			fs  = &empty_string;                                                       \
			break;                                                                     \
		case TestCaseBase<API>::FRAGMENT_SHADER_TYPE:                                  \
			fs = &SOURCE;                                                              \
			break;                                                                     \
		case TestCaseBase<API>::GEOMETRY_SHADER_TYPE:                                  \
			gs = &SOURCE;                                                              \
			break;                                                                     \
		case TestCaseBase<API>::TESSELATION_CONTROL_SHADER_TYPE:                       \
			tcs = &SOURCE;                                                             \
			break;                                                                     \
		case TestCaseBase<API>::TESSELATION_EVALUATION_SHADER_TYPE:                    \
			tes = &SOURCE;                                                             \
			break;                                                                     \
		case TestCaseBase<API>::VERTEX_SHADER_TYPE:                                    \
			vs = &SOURCE;                                                              \
			break;                                                                     \
		default:                                                                       \
			TCU_FAIL("Invalid enum");                                                  \
			break;                                                                     \
		};                                                                             \
                                                                                       \
		if (API::USE_ALL_SHADER_STAGES)                                                \
		{                                                                              \
			this->execute_positive_test(*vs, *tcs, *tes, *gs, *fs, *cs, DELETE, GPU5); \
		}                                                                              \
		else                                                                           \
		{                                                                              \
			this->execute_positive_test(*vs, *fs, DELETE, GPU5);                       \
		}                                                                              \
	}

/** Macro executes either positive or negative test
 *
 * @param S		Selects negative test when 0, positive test otherwise
 * @param TYPE	Tested shader stage
 * @param SOURCE Tested shader source
 **/
#define EXECUTE_SHADER_TEST(S, TYPE, SOURCE)              \
	if (S)                                                \
	{                                                     \
		EXECUTE_POSITIVE_TEST(TYPE, SOURCE, true, false); \
	}                                                     \
	else                                                  \
	{                                                     \
		this->execute_negative_test(TYPE, SOURCE);        \
	}

/** Test case constructor.
 *
 * @tparam API        Tested API descriptor
 *
 * @param context     EGL context ID.
 * @param name        Name of a test case.
 * @param description Test case description.
 **/
template <class API>
TestCaseBase<API>::TestCaseBase(Context& context, const char* name, const char* description)
	: tcu::TestCase(context.getTestContext(), name, description)
	, context_id(context)
	, program_object_id(0)
	, compute_shader_object_id(0)
	, fragment_shader_object_id(0)
	, geometry_shader_object_id(0)
	, tess_ctrl_shader_object_id(0)
	, tess_eval_shader_object_id(0)
	, vertex_shader_object_id(0)
{
	/* Left blank on purpose */
}

/** Clears up the shaders and program that were created during the tests
 *
 * @tparam API Tested API descriptor
 */
template <class API>
void TestCaseBase<API>::delete_objects(void)
{
	const glw::Functions& gl = context_id.getRenderContext().getFunctions();

	/* Release all ES objects that may have been created by iterate() */
	if (program_object_id != 0)
	{
		gl.deleteProgram(program_object_id);
		program_object_id = 0;
	}

	/* Use default program object to be sure the objects were released. */
	gl.useProgram(0);
}

/** Releases all OpenGL ES objects that were created for test case purposes.
 *
 * @tparam API Tested API descriptor
 */
template <class API>
void TestCaseBase<API>::deinit(void)
{
	this->delete_objects();
}

/** Runs the actual test for each shader type.
 *
 * @tparam API               Tested API descriptor
 *
 *  @return QP_TEST_RESULT_FAIL - test has failed;
 *          QP_TEST_RESULT_PASS - test has succeeded;
 **/
template <class API>
tcu::TestNode::IterateResult TestCaseBase<API>::iterate(void)
{
	test_shader_compilation(TestCaseBase<API>::VERTEX_SHADER_TYPE);
	test_shader_compilation(TestCaseBase<API>::FRAGMENT_SHADER_TYPE);

	if (API::USE_ALL_SHADER_STAGES)
	{
		test_shader_compilation(TestCaseBase<API>::COMPUTE_SHADER_TYPE);
		test_shader_compilation(TestCaseBase<API>::GEOMETRY_SHADER_TYPE);
		test_shader_compilation(TestCaseBase<API>::TESSELATION_CONTROL_SHADER_TYPE);
		test_shader_compilation(TestCaseBase<API>::TESSELATION_EVALUATION_SHADER_TYPE);
	}

	return STOP;
}

/** Generates a shader object of the specified type,
 *  attaches the specified shader source,
 *  compiles it, and returns the compilation result.
 *
 * @tparam API               Tested API descriptor
 *
 * @param shader_source      The source for the shader object.
 * @param tested_shader_type The type of shader being compiled (vertex or fragment).
 *
 * @return Compilation result (GL_TRUE if the shader compilation succeeded, GL_FALSE otherwise).
 **/
template <class API>
glw::GLint TestCaseBase<API>::compile_shader_and_get_compilation_result(const std::string& tested_snippet,
																		TestShaderType	 tested_shader_type,
																		bool			   require_gpu_shader5)
{
	static const char* preamble_cs = "\n"
									 "layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;\n"
									 "\n";

	static const char* preamble_gs = "\n"
									 "layout(points)                           in;\n"
									 "layout(triangle_strip, max_vertices = 4) out;\n"
									 "\n";

	static const char* preamble_tcs = "\n"
									  "layout(vertices = 1) out;\n"
									  "\n";

	static const char* preamble_tes = "\n"
									  "layout(isolines, point_mode) in;\n"
									  "\n";

	glw::GLint			  compile_status   = GL_TRUE;
	const glw::Functions& gl			   = context_id.getRenderContext().getFunctions();
	glw::GLint			  shader_object_id = 0;

	std::string shader_source;

	if (true == tested_snippet.empty())
	{
		return compile_status;
	}

	if (require_gpu_shader5)
	{
		// Add the version number here, rather than in each individual test
		shader_source = API::shader_version_gpu5;
	}
	else
	{
		// Add the version number here, rather than in each individual test
		shader_source = API::shader_version;
	}

	/* Apply stage specific stuff */
	switch (tested_shader_type)
	{
	case TestCaseBase<API>::VERTEX_SHADER_TYPE:
		break;
	case TestCaseBase<API>::FRAGMENT_SHADER_TYPE:
		break;
	case TestCaseBase<API>::COMPUTE_SHADER_TYPE:
		shader_source += preamble_cs;
		break;
	case TestCaseBase<API>::GEOMETRY_SHADER_TYPE:
		shader_source += preamble_gs;
		break;
	case TestCaseBase<API>::TESSELATION_CONTROL_SHADER_TYPE:
		shader_source += preamble_tcs;
		break;
	case TestCaseBase<API>::TESSELATION_EVALUATION_SHADER_TYPE:
		shader_source += preamble_tes;
		break;
	default:
		TCU_FAIL("Unrecognized shader type.");
		break;
	}

	shader_source += tested_snippet;

	/* Prepare shader object */
	switch (tested_shader_type)
	{
	case TestCaseBase<API>::VERTEX_SHADER_TYPE:
	{
		shader_object_id = gl.createShader(GL_VERTEX_SHADER);
		assert(0 == vertex_shader_object_id);
		vertex_shader_object_id = shader_object_id;

		GLU_EXPECT_NO_ERROR(gl.getError(), "Could not create a vertex shader.");

		break;
	} /* case TestCaseBase<API>::VERTEX_SHADER_TYPE: */
	case TestCaseBase<API>::FRAGMENT_SHADER_TYPE:
	{
		shader_object_id = gl.createShader(GL_FRAGMENT_SHADER);
		assert(0 == fragment_shader_object_id);
		fragment_shader_object_id = shader_object_id;

		GLU_EXPECT_NO_ERROR(gl.getError(), "Could not create a fragment shader.");

		break;
	} /* case TestCaseBase<API>::FRAGMENT_SHADER_TYPE: */
	case TestCaseBase<API>::COMPUTE_SHADER_TYPE:
	{
		shader_object_id = gl.createShader(GL_COMPUTE_SHADER);
		assert(0 == compute_shader_object_id);
		compute_shader_object_id = shader_object_id;

		GLU_EXPECT_NO_ERROR(gl.getError(), "Could not create a compute shader.");

		break;
	} /* case TestCaseBase<API>::COMPUTE_SHADER_TYPE: */
	case TestCaseBase<API>::GEOMETRY_SHADER_TYPE:
	{
		shader_object_id = gl.createShader(GL_GEOMETRY_SHADER);
		assert(0 == geometry_shader_object_id);
		geometry_shader_object_id = shader_object_id;

		GLU_EXPECT_NO_ERROR(gl.getError(), "Could not create a geometry shader.");

		break;
	} /* case TestCaseBase<API>::GEOMETRY_SHADER_TYPE: */
	case TestCaseBase<API>::TESSELATION_CONTROL_SHADER_TYPE:
	{
		shader_object_id = gl.createShader(GL_TESS_CONTROL_SHADER);
		assert(0 == tess_ctrl_shader_object_id);
		tess_ctrl_shader_object_id = shader_object_id;

		GLU_EXPECT_NO_ERROR(gl.getError(), "Could not create a tesselation control shader.");

		break;
	} /* case TestCaseBase<API>::TESSELATION_CONTROL_SHADER_TYPE: */
	case TestCaseBase<API>::TESSELATION_EVALUATION_SHADER_TYPE:
	{
		shader_object_id = gl.createShader(GL_TESS_EVALUATION_SHADER);
		assert(0 == tess_eval_shader_object_id);
		tess_eval_shader_object_id = shader_object_id;

		GLU_EXPECT_NO_ERROR(gl.getError(), "Could not create a tesselation evaluation shader.");

		break;
	} /* case TestCaseBase<API>::TESSELATION_EVALUATION_SHADER_TYPE: */
	default:
	{
		TCU_FAIL("Unrecognized shader type.");

		break;
	} /* default: */
	} /* switch (tested_shader_type) */

	/* Assign source code to the objects */
	const char* code_ptr = shader_source.c_str();

#if IS_DEBUG_DUMP_ALL_SHADERS
	context_id.getTestContext().getLog() << tcu::TestLog::Message << "Compiling: " << tcu::TestLog::EndMessage;
	context_id.getTestContext().getLog() << tcu::TestLog::KernelSource(code_ptr);
#endif /* IS_DEBUG_DUMP_ALL_SHADERS */

	gl.shaderSource(shader_object_id, 1 /* count */, &code_ptr, NULL);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glShaderSource() failed");

	/* Compile the shader */
	gl.compileShader(shader_object_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCompileShader() failed");

	/* Get the compilation result. */
	gl.getShaderiv(shader_object_id, GL_COMPILE_STATUS, &compile_status);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetShaderiv() failed");

#if IS_DEBUG
	if (GL_TRUE != compile_status)
	{
		glw::GLint  length = 0;
		std::string message;

		/* Error log length */
		gl.getShaderiv(shader_object_id, GL_INFO_LOG_LENGTH, &length);
		GLU_EXPECT_NO_ERROR(gl.getError(), "GetShaderiv");

		/* Prepare storage */
		message.resize(length, 0);

		/* Get error log */
		gl.getShaderInfoLog(shader_object_id, length, 0, &message[0]);
		GLU_EXPECT_NO_ERROR(gl.getError(), "GetShaderInfoLog");

		context_id.getTestContext().getLog() << tcu::TestLog::Message << "Error message: " << &message[0]
											 << tcu::TestLog::EndMessage;

#if IS_DEBUG_DUMP_ALL_SHADERS
#else  /* IS_DEBUG_DUMP_ALL_SHADERS */
		context_id.getTestContext().getLog() << tcu::TestLog::KernelSource(code_ptr);
#endif /* IS_DEBUG_DUMP_ALL_SHADERS */
	}
#endif /* IS_DEBUG */

	return compile_status;
}

/** Runs the negative test.
 *  The shader sources are considered as invalid,
 *  and the compilation of a shader object with the specified
 *  shader source is expected to fail.
 *
 * @tparam API               Tested API descriptor
 *
 * @param tested_shader_type The type of shader object (can be fragment or vertex).
 * @param shader_source      The source for the shader object to be used for this test.
 *
 *  @return QP_TEST_RESULT_FAIL - test has failed;
 *          QP_TEST_RESULT_PASS - test has succeeded;
 **/
template <class API>
tcu::TestNode::IterateResult TestCaseBase<API>::execute_negative_test(
	typename TestCaseBase<API>::TestShaderType tested_shader_type, const std::string& shader_source)
{
	glw::GLint			  compile_status = GL_FALSE;
	const char*			  error_message  = 0;
	const glw::Functions& gl			 = context_id.getRenderContext().getFunctions();
	bool				  test_result	= true;

	/* Try to generate and compile the shader object. */
	switch (tested_shader_type)
	{
	case TestCaseBase<API>::FRAGMENT_SHADER_TYPE:
		error_message =
			"The fragment shader was expected to fail to compile, but the compilation process was successful.";
		break;

	case TestCaseBase<API>::VERTEX_SHADER_TYPE:
		error_message =
			"The vertex shader was expected to fail to compile, but the compilation process was successful.";

		break;

	case TestCaseBase<API>::COMPUTE_SHADER_TYPE:
		error_message =
			"The compute shader was expected to fail to compile, but the compilation process was successful.";
		break;

	case TestCaseBase<API>::GEOMETRY_SHADER_TYPE:
		error_message =
			"The geometry shader was expected to fail to compile, but the compilation process was successful.";
		break;

	case TestCaseBase<API>::TESSELATION_CONTROL_SHADER_TYPE:
		error_message = "The tesselation control shader was expected to fail to compile, but the compilation process "
						"was successful.";
		break;

	case TestCaseBase<API>::TESSELATION_EVALUATION_SHADER_TYPE:
		error_message = "The tesselation evaluation shader was expected to fail to compile, but the compilation "
						"process was successful.";
		break;

	default:
		TCU_FAIL("Unrecognized shader type.");
		test_result = false;

		break;
	} /* switch (shader_type) */

	compile_status = compile_shader_and_get_compilation_result(shader_source, tested_shader_type);

	if (compile_status == GL_TRUE)
	{
		TCU_FAIL(error_message);

		test_result = false;
	}

	/* Deallocate any resources used. */
	this->delete_objects();
	if (0 != compute_shader_object_id)
	{
		gl.deleteShader(compute_shader_object_id);
		compute_shader_object_id = 0;
	}
	if (0 != fragment_shader_object_id)
	{
		gl.deleteShader(fragment_shader_object_id);
		fragment_shader_object_id = 0;
	}
	if (0 != geometry_shader_object_id)
	{
		gl.deleteShader(geometry_shader_object_id);
		geometry_shader_object_id = 0;
	}
	if (0 != tess_ctrl_shader_object_id)
	{
		gl.deleteShader(tess_ctrl_shader_object_id);
		tess_ctrl_shader_object_id = 0;
	}
	if (0 != tess_eval_shader_object_id)
	{
		gl.deleteShader(tess_eval_shader_object_id);
		tess_eval_shader_object_id = 0;
	}
	if (0 != vertex_shader_object_id)
	{
		gl.deleteShader(vertex_shader_object_id);
		vertex_shader_object_id = 0;
	}

	/* Return test pass if true. */
	if (true == test_result)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}

	return CONTINUE;
}

/** Runs the positive test.
 *  The shader sources are considered as valid,
 *  and the compilation and program linking are expected to succeed.
 *
 * @tparam API                     Tested API descriptor
 *
 * @param vertex_shader_source     The source for the vertex shader to be used for this test.
 * @param fragment_shader_source   The source for the fragment shader to be used for this test.
 * @param delete_generated_objects If true, the compiled shader objects will be deleted before returning.
 *
 *  @return QP_TEST_RESULT_FAIL - test has failed;
 *          QP_TEST_RESULT_PASS - test has succeeded;
 **/
template <class API>
tcu::TestNode::IterateResult TestCaseBase<API>::execute_positive_test(const std::string& vertex_shader_source,
																	  const std::string& fragment_shader_source,
																	  bool				 delete_generated_objects,
																	  bool				 require_gpu_shader5)
{
	glw::GLint			  compile_status = GL_TRUE;
	const glw::Functions& gl			 = context_id.getRenderContext().getFunctions();
	glw::GLint			  link_status	= GL_TRUE;
	bool				  test_result	= true;

	/* Compile, and check the compilation result for the fragment shader object. */
	compile_status = compile_shader_and_get_compilation_result(
		fragment_shader_source, TestCaseBase<API>::FRAGMENT_SHADER_TYPE, require_gpu_shader5);

	if (compile_status == GL_FALSE)
	{
		TCU_FAIL("The fragment shader was expected to compile successfully, but failed to compile.");

		test_result = false;
	}

	/* Compile, and check the compilation result for the vertex shader object. */
	compile_status = compile_shader_and_get_compilation_result(
		vertex_shader_source, TestCaseBase<API>::VERTEX_SHADER_TYPE, require_gpu_shader5);

	if (compile_status == GL_FALSE)
	{
		TCU_FAIL("The vertex shader was expected to compile successfully, but failed to compile.");

		test_result = false;
	}

	if (true == test_result)
	{
		/* Create program object. */
		assert(0 == program_object_id);
		program_object_id = gl.createProgram();
		GLU_EXPECT_NO_ERROR(gl.getError(), "Could not create a program object.");

		/* Configure the program object */
		gl.attachShader(program_object_id, fragment_shader_object_id);
		gl.attachShader(program_object_id, vertex_shader_object_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glAttachShader() failed.");

		gl.deleteShader(fragment_shader_object_id);
		gl.deleteShader(vertex_shader_object_id);
		fragment_shader_object_id = 0;
		vertex_shader_object_id   = 0;

		/* Link the program object */
		gl.linkProgram(program_object_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glLinkProgram() failed.");

		/* Make sure the linking operation succeeded. */
		gl.getProgramiv(program_object_id, GL_LINK_STATUS, &link_status);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGetProgramiv() failed.");

		if (link_status != GL_TRUE)
		{
#if IS_DEBUG
			glw::GLint  length = 0;
			std::string message;

			/* Get error log length */
			gl.getProgramiv(program_object_id, GL_INFO_LOG_LENGTH, &length);
			GLU_EXPECT_NO_ERROR(gl.getError(), "GetProgramiv");

			message.resize(length, 0);

			/* Get error log */
			gl.getProgramInfoLog(program_object_id, length, 0, &message[0]);
			GLU_EXPECT_NO_ERROR(gl.getError(), "GetProgramInfoLog");

			context_id.getTestContext().getLog() << tcu::TestLog::Message << "Error message: " << &message[0]
												 << tcu::TestLog::EndMessage;

#if IS_DEBUG_DUMP_ALL_SHADERS
#else  /* IS_DEBUG_DUMP_ALL_SHADERS */
			context_id.getTestContext().getLog() << tcu::TestLog::KernelSource(vertex_shader_source);
			context_id.getTestContext().getLog() << tcu::TestLog::KernelSource(fragment_shader_source);
#endif /* IS_DEBUG_DUMP_ALL_SHADERS */
#endif /* IS_DEBUG */

			TCU_FAIL("Linking was expected to succeed, but the process was unsuccessful.");

			test_result = false;
		}
	}

	if (delete_generated_objects)
	{
		/* Deallocate any resources used. */
		this->delete_objects();
	}

	/* Return test pass if true. */
	if (true == test_result)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}

	return CONTINUE;
}

/** Runs the positive test.
 *  The shader sources are considered as valid,
 *  and the compilation and program linking are expected to succeed.
 *
 * @tparam API                     Tested API descriptor
 *
 * @param vertex_shader_source     The source for the vertex shader to be used for this test.
 * @param tess_ctrl_shader_source  The source for the vertex shader to be used for this test.
 * @param tess_eval_shader_source  The source for the vertex shader to be used for this test.
 * @param geometry_shader_source   The source for the vertex shader to be used for this test.
 * @param fragment_shader_source   The source for the vertex shader to be used for this test.
 * @param compute_shader_source    The source for the fragment shader to be used for this test.
 * @param delete_generated_objects If true, the compiled shader objects will be deleted before returning.
 *
 *  @return QP_TEST_RESULT_FAIL - test has failed;
 *          QP_TEST_RESULT_PASS - test has succeeded;
 **/
template <class API>
tcu::TestNode::IterateResult TestCaseBase<API>::execute_positive_test(
	const std::string& vertex_shader_source, const std::string& tess_ctrl_shader_source,
	const std::string& tess_eval_shader_source, const std::string& geometry_shader_source,
	const std::string& fragment_shader_source, const std::string& compute_shader_source, bool delete_generated_objects,
	bool require_gpu_shader5)
{
	glw::GLint			  compile_status = GL_TRUE;
	const glw::Functions& gl			 = context_id.getRenderContext().getFunctions();
	glw::GLint			  link_status	= GL_TRUE;
	bool				  test_compute   = !compute_shader_source.empty();
	bool				  test_result	= true;

	if (false == test_compute)
	{
		/* Compile, and check the compilation result for the fragment shader object. */
		compile_status = compile_shader_and_get_compilation_result(
			fragment_shader_source, TestCaseBase<API>::FRAGMENT_SHADER_TYPE, require_gpu_shader5);

		if (compile_status == GL_FALSE)
		{
			TCU_FAIL("The fragment shader was expected to compile successfully, but failed to compile.");

			test_result = false;
		}

		/* Compile, and check the compilation result for the geometry shader object. */
		compile_status = compile_shader_and_get_compilation_result(
			geometry_shader_source, TestCaseBase<API>::GEOMETRY_SHADER_TYPE, require_gpu_shader5);

		if (compile_status == GL_FALSE)
		{
			TCU_FAIL("The geometry shader was expected to compile successfully, but failed to compile.");

			test_result = false;
		}

		/* Compile, and check the compilation result for the te shader object. */
		compile_status = compile_shader_and_get_compilation_result(
			tess_eval_shader_source, TestCaseBase<API>::TESSELATION_EVALUATION_SHADER_TYPE, require_gpu_shader5);

		if (compile_status == GL_FALSE)
		{
			TCU_FAIL("The tesselation evaluation shader was expected to compile successfully, but failed to compile.");

			test_result = false;
		}

		/* Compile, and check the compilation result for the tc shader object. */
		compile_status = compile_shader_and_get_compilation_result(
			tess_ctrl_shader_source, TestCaseBase<API>::TESSELATION_CONTROL_SHADER_TYPE, require_gpu_shader5);

		if (compile_status == GL_FALSE)
		{
			TCU_FAIL("The tesselation control shader was expected to compile successfully, but failed to compile.");

			test_result = false;
		}

		/* Compile, and check the compilation result for the vertex shader object. */
		compile_status = compile_shader_and_get_compilation_result(
			vertex_shader_source, TestCaseBase<API>::VERTEX_SHADER_TYPE, require_gpu_shader5);

		if (compile_status == GL_FALSE)
		{
			TCU_FAIL("The vertex shader was expected to compile successfully, but failed to compile.");

			test_result = false;
		}
	}
	else
	{
		/* Compile, and check the compilation result for the compute shader object. */
		compile_status = compile_shader_and_get_compilation_result(
			compute_shader_source, TestCaseBase<API>::COMPUTE_SHADER_TYPE, require_gpu_shader5);

		if (compile_status == GL_FALSE)
		{
			TCU_FAIL("The compute shader was expected to compile successfully, but failed to compile.");

			test_result = false;
		}
	}

	if (true == test_result)
	{
		/* Create program object. */
		program_object_id = gl.createProgram();
		GLU_EXPECT_NO_ERROR(gl.getError(), "Could not create a program object.");

		/* Configure the program object */
		if (false == test_compute)
		{
			gl.attachShader(program_object_id, fragment_shader_object_id);

			if (geometry_shader_object_id)
			{
				gl.attachShader(program_object_id, geometry_shader_object_id);
			}

			if (tess_ctrl_shader_object_id)
			{
				gl.attachShader(program_object_id, tess_ctrl_shader_object_id);
			}

			if (tess_eval_shader_object_id)
			{
				gl.attachShader(program_object_id, tess_eval_shader_object_id);
			}

			gl.attachShader(program_object_id, vertex_shader_object_id);
		}
		else
		{
			gl.attachShader(program_object_id, compute_shader_object_id);
		}
		GLU_EXPECT_NO_ERROR(gl.getError(), "glAttachShader() failed.");

		if (false == test_compute)
		{
			gl.deleteShader(fragment_shader_object_id);

			if (geometry_shader_object_id)
			{
				gl.deleteShader(geometry_shader_object_id);
			}

			if (tess_ctrl_shader_object_id)
			{
				gl.deleteShader(tess_ctrl_shader_object_id);
			}

			if (tess_eval_shader_object_id)
			{
				gl.deleteShader(tess_eval_shader_object_id);
			}

			gl.deleteShader(vertex_shader_object_id);
		}
		else
		{
			gl.deleteShader(compute_shader_object_id);
		}

		fragment_shader_object_id  = 0;
		vertex_shader_object_id	= 0;
		geometry_shader_object_id  = 0;
		tess_ctrl_shader_object_id = 0;
		tess_eval_shader_object_id = 0;
		compute_shader_object_id   = 0;

		/* Link the program object */
		gl.linkProgram(program_object_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glLinkProgram() failed.");

		/* Make sure the linking operation succeeded. */
		gl.getProgramiv(program_object_id, GL_LINK_STATUS, &link_status);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGetProgramiv() failed.");

		if (link_status != GL_TRUE)
		{
#if IS_DEBUG
			glw::GLint  length = 0;
			std::string message;

			/* Get error log length */
			gl.getProgramiv(program_object_id, GL_INFO_LOG_LENGTH, &length);
			GLU_EXPECT_NO_ERROR(gl.getError(), "GetProgramiv");

			message.resize(length, 0);

			/* Get error log */
			gl.getProgramInfoLog(program_object_id, length, 0, &message[0]);
			GLU_EXPECT_NO_ERROR(gl.getError(), "GetProgramInfoLog");

			context_id.getTestContext().getLog() << tcu::TestLog::Message << "Error message: " << &message[0]
												 << tcu::TestLog::EndMessage;

#if IS_DEBUG_DUMP_ALL_SHADERS
			if (false == test_compute)
			{
				context_id.getTestContext().getLog() << tcu::TestLog::KernelSource(vertex_shader_source);
				context_id.getTestContext().getLog() << tcu::TestLog::KernelSource(tess_ctrl_shader_source);
				context_id.getTestContext().getLog() << tcu::TestLog::KernelSource(tess_eval_shader_source);
				context_id.getTestContext().getLog() << tcu::TestLog::KernelSource(geometry_shader_source);
				context_id.getTestContext().getLog() << tcu::TestLog::KernelSource(fragment_shader_source);
			}
			else
			{
				context_id.getTestContext().getLog() << tcu::TestLog::KernelSource(compute_shader_source);
			}
#endif /* IS_DEBUG_DUMP_ALL_SHADERS */
#endif /* IS_DEBUG */

			TCU_FAIL("Linking was expected to succeed, but the process was unsuccessful.");

			test_result = false;
		}
	}

	if (delete_generated_objects)
	{
		/* Deallocate any resources used. */
		this->delete_objects();
	}

	/* Return test pass if true. */
	if (true == test_result)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}

	return CONTINUE;
}

/** Adds the specified @param sub_script onto the base_string @param number_of_elements times.
 *  E.g. extend_string("a", [2], 3) would give a[2][2][2].
 *
 * @tparam API               Tested API descriptor
 *
 * @param base_string        The base string that is to be added to.
 * @param sub_string         The string to be repeatedly added
 * @param number_of_elements The number of repetitions.
 *
 *  @return The extended string.
 **/
template <class API>
std::string TestCaseBase<API>::extend_string(std::string base_string, std::string sub_string, size_t number_of_elements)
{
	std::string temp_string = base_string;

	for (size_t sub_script_index = 0; sub_script_index < number_of_elements; sub_script_index++)
	{
		temp_string += sub_string;
	}

	return temp_string;
}

/* Generates the shader source code for the SizedDeclarationsPrimitive
 * array tests, and attempts to compile each test shader, for both
 * vertex and fragment shaders.
 *
 * @tparam API               Tested API descriptor
 *
 * @param tested_shader_type The type of shader that is being tested
 *                           (either TestCaseBase<API>::VERTEX_SHADER_TYPE or TestCaseBase<API>::FRAGMENT_SHADER_TYPE).
 *
 **/
template <class API>
void SizedDeclarationsPrimitive<API>::test_shader_compilation(
	typename TestCaseBase<API>::TestShaderType tested_shader_type)
{
	for (size_t var_type_index = 0; var_type_index < API::n_var_types; var_type_index++)
	{
		_supported_variable_types_map_const_iterator var_iterator =
			supported_variable_types_map.find(API::var_types[var_type_index]);

		if (var_iterator != supported_variable_types_map.end())
		{
			/* Loop round for each var_types ("int", "uint", "float", etc.)
			 * We are testing a[][] to a [][][][][][][][], so start counter at 2. */
			for (size_t max_dimension_limit = 2; max_dimension_limit <= API::MAX_ARRAY_DIMENSIONS;
				 max_dimension_limit++)
			{
				// Record the base varTypeModifier + varType
				std::string base_var_type		 = var_iterator->second.type;
				std::string base_variable_string = base_var_type;

				for (size_t base_sub_script_index = 0; base_sub_script_index <= max_dimension_limit;
					 base_sub_script_index++)
				{
					std::string shader_source = "";

					// Add the shader body start, and the base varTypeModifier + varType + variable name.
					shader_source += shader_start + "    " + base_variable_string + " a";

					for (size_t remaining_sub_script_index = base_sub_script_index;
						 remaining_sub_script_index < max_dimension_limit; remaining_sub_script_index++)
					{
						/* Add as many array sub_scripts as we can, up to the current dimension limit. */
						shader_source += "[2]";
					}

					/* End line */
					shader_source += ";\n";

					/* End main */
					DEFAULT_MAIN_ENDING(tested_shader_type, shader_source);

					/* Execute test */
					EXECUTE_POSITIVE_TEST(tested_shader_type, shader_source, true, false);

					/* From now on, we'll have an extra sub_script each time. */
					base_variable_string += "[2]";
				} /* for (int base_sub_script_index = 0; ...) */
			}	 /* for (int max_dimension_limit = 2; ...) */
		}		  /* if var_type iterator found */
		else
		{
			TCU_FAIL("Type not found.");
		}
	} /* for (int var_type_index = 0; ...) */
}

/* Generates the shader source code for the SizedDeclarationsStructTypes1
 * array tests, and attempts to compile each test shader, for both
 * vertex and fragment shaders.
 *
 * @tparam API               Tested API descriptor
 *
 * @param tested_shader_type The type of shader that is being tested
 *                           (either TestCaseBase<API>::VERTEX_SHADER_TYPE or TestCaseBase<API>::FRAGMENT_SHADER_TYPE).
 */
template <class API>
void SizedDeclarationsStructTypes1<API>::test_shader_compilation(
	typename TestCaseBase<API>::TestShaderType tested_shader_type)
{
	std::string example_struct("struct light {\n"
							   "    float intensity;\n"
							   "    int   position;\n"
							   "};\n\n");
	std::string shader_source;

	for (size_t max_dimension_index = 1; max_dimension_index < API::MAX_ARRAY_DIMENSIONS; max_dimension_index++)
	{
		shader_source = example_struct;
		shader_source += shader_start;
		shader_source += "    light[2]";

		for (size_t temp_dimension_index = 0; temp_dimension_index < max_dimension_index; temp_dimension_index++)
		{
			shader_source += "[2]";
		}

		shader_source += " x;\n";

		/* End main */
		DEFAULT_MAIN_ENDING(tested_shader_type, shader_source);

		/* Execute test */
		EXECUTE_POSITIVE_TEST(tested_shader_type, shader_source, true, false);

	} /* for (int max_dimension_index = 1; ...) */
}

/* Generates the shader source code for the SizedDeclarationsStructTypes2
 * array tests, and attempts to compile each test shader, for both
 * vertex and fragment shaders.
 *
 * @tparam API               Tested API descriptor
 *
 * @param tested_shader_type The type of shader that is being tested
 *                           (either TestCaseBase<API>::VERTEX_SHADER_TYPE or TestCaseBase<API>::FRAGMENT_SHADER_TYPE).
 */
template <class API>
void SizedDeclarationsStructTypes2<API>::test_shader_compilation(
	typename TestCaseBase<API>::TestShaderType tested_shader_type)
{
	std::string structure_declaration = "struct MyStructure {\n"
										"   float a[2], "
										"b[2][2], "
										"c[2][2][2], "
										"d[2][2][2][2], "
										"e[2][2][2][2][2], "
										"f[2][2][2][2][2][2], "
										"g[2][2][2][2][2][2][2], "
										"h[2][2][2][2][2][2][2][2];\n"
										"} myStructureObject;\n\n";
	std::string shader_source = structure_declaration;

	shader_source += shader_start;

	/* End main */
	DEFAULT_MAIN_ENDING(tested_shader_type, shader_source);

	/* Execute test */
	EXECUTE_POSITIVE_TEST(tested_shader_type, shader_source, true, false);
}

/* Generates the shader source code for the SizedDeclarationsStructTypes3
 * array tests, and attempts to compile each test shader, for both
 * vertex and fragment shaders.
 *
 * @tparam API               Tested API descriptor
 *
 * @param tested_shader_type The type of shader that is being tested
 *                           (either TestCaseBase<API>::VERTEX_SHADER_TYPE or TestCaseBase<API>::FRAGMENT_SHADER_TYPE).
 */
template <class API>
void SizedDeclarationsStructTypes3<API>::test_shader_compilation(
	typename TestCaseBase<API>::TestShaderType tested_shader_type)
{
	std::string example_struct("struct light {\n"
							   "    float[2] intensity;\n"
							   "    int[2] position;\n"
							   "};\n");
	std::string shader_source;

	for (size_t max_dimension_index = 1; max_dimension_index <= API::MAX_ARRAY_DIMENSIONS; max_dimension_index++)
	{
		shader_source = example_struct;
		shader_source += shader_start;
		shader_source += this->extend_string("    light my_light_object", "[2]", max_dimension_index);
		shader_source += ";\n";

		/* End main */
		DEFAULT_MAIN_ENDING(tested_shader_type, shader_source);

		/* Execute test */
		EXECUTE_POSITIVE_TEST(tested_shader_type, shader_source, true, false);
	} /* for (int max_dimension_index = 1; ...) */
}

/* Generates the shader source code for the SizedDeclarationsStructTypes4
 * array tests, and attempts to compile each test shader, for both
 * vertex and fragment shaders.
 *
 * @tparam API               Tested API descriptor
 *
 * @param tested_shader_type The type of shader that is being tested
 *                           (either TestCaseBase<API>::VERTEX_SHADER_TYPE or TestCaseBase<API>::FRAGMENT_SHADER_TYPE).
 */
template <class API>
void SizedDeclarationsStructTypes4<API>::test_shader_compilation(
	typename TestCaseBase<API>::TestShaderType tested_shader_type)
{
	std::string example_struct("struct light {\n"
							   "    float[2] intensity;\n"
							   "    int[2] position;\n"
							   "} lightVar[2]");
	std::string shader_source;

	for (size_t max_dimension_index = 1; max_dimension_index < API::MAX_ARRAY_DIMENSIONS; max_dimension_index++)
	{
		shader_source = example_struct;

		for (size_t temp_dimension_index = 0; temp_dimension_index < max_dimension_index; temp_dimension_index++)
		{
			shader_source += "[2]";
		}
		shader_source += ";\n\n";
		shader_source += shader_start;

		/* End main */
		DEFAULT_MAIN_ENDING(tested_shader_type, shader_source);

		/* Execute test */
		EXECUTE_POSITIVE_TEST(tested_shader_type, shader_source, true, false);
	} /* for (int max_dimension_index = 1; ...) */
}

/* Generates the shader source code for the SizedDeclarationsTypenameStyle1
 * array tests, and attempts to compile each test shader, for both
 * vertex and fragment shaders.
 *
 * @tparam API               Tested API descriptor
 *
 * @param tested_shader_type The type of shader that is being tested
 *                           (either TestCaseBase<API>::VERTEX_SHADER_TYPE or TestCaseBase<API>::FRAGMENT_SHADER_TYPE).
 */
template <class API>
void SizedDeclarationsTypenameStyle1<API>::test_shader_compilation(
	typename TestCaseBase<API>::TestShaderType tested_shader_type)
{
	for (size_t max_dimension_index = 1; max_dimension_index <= API::MAX_ARRAY_DIMENSIONS; max_dimension_index++)
	{
		std::string shader_source = shader_start;

		shader_source += this->extend_string("    float", "[2]", max_dimension_index);
		shader_source += this->extend_string(" x", "[2]", API::MAX_ARRAY_DIMENSIONS - max_dimension_index);
		shader_source += ";\n";

		/* End main */
		DEFAULT_MAIN_ENDING(tested_shader_type, shader_source);

		/* Execute test */
		EXECUTE_POSITIVE_TEST(tested_shader_type, shader_source, true, false);
	} /* for (int max_dimension_index = 1; ...) */
}

/* Generates the shader source code for the SizedDeclarationsTypenameStyle2
 * array tests, and attempts to compile each test shader, for both
 * vertex and fragment shaders.
 *
 * @tparam API               Tested API descriptor
 *
 * @param tested_shader_type The type of shader that is being tested
 *                           (either TestCaseBase<API>::VERTEX_SHADER_TYPE or TestCaseBase<API>::FRAGMENT_SHADER_TYPE).
 */
template <class API>
void SizedDeclarationsTypenameStyle2<API>::test_shader_compilation(
	typename TestCaseBase<API>::TestShaderType tested_shader_type)
{
	std::string shader_source = shader_start;

	shader_source += this->extend_string("    float", "[2]", 2);
	shader_source += this->extend_string(" a", "[2]", 0);
	shader_source += ", ";
	shader_source += this->extend_string("b", "[2]", 1);
	shader_source += ", ";
	shader_source += this->extend_string("c", "[2]", 2);
	shader_source += ", ";
	shader_source += this->extend_string("d", "[2]", 3);
	shader_source += ", ";
	shader_source += this->extend_string("e", "[2]", 4);
	shader_source += ", ";
	shader_source += this->extend_string("f", "[2]", 5);
	shader_source += ", ";
	shader_source += this->extend_string("g", "[2]", 6);
	shader_source += ";\n";

	/* End main */
	DEFAULT_MAIN_ENDING(tested_shader_type, shader_source);

	/* Execute test */
	EXECUTE_POSITIVE_TEST(tested_shader_type, shader_source, true, false);
}

/* Generates the shader source code for the SizedDeclarationsTypenameStyle3
 * array tests, and attempts to compile each test shader, for both
 * vertex and fragment shaders.
 *
 * @tparam API               Tested API descriptor
 *
 * @param tested_shader_type The type of shader that is being tested
 *                           (either TestCaseBase<API>::VERTEX_SHADER_TYPE or TestCaseBase<API>::FRAGMENT_SHADER_TYPE).
 */
template <class API>
void SizedDeclarationsTypenameStyle3<API>::test_shader_compilation(
	typename TestCaseBase<API>::TestShaderType tested_shader_type)
{
	std::string shader_source = "struct{\n" + this->extend_string("    float", "[2]", 2);

	shader_source += this->extend_string(" a", "[2]", API::MAX_ARRAY_DIMENSIONS - API::MAX_ARRAY_DIMENSIONS);
	shader_source += ",";
	shader_source += this->extend_string("    b", "[2]", 1);
	shader_source += ",";
	shader_source += this->extend_string("    c", "[2]", 2);
	shader_source += ",";
	shader_source += this->extend_string("    d", "[2]", 3);
	shader_source += ",";
	shader_source += this->extend_string("    e", "[2]", 4);
	shader_source += ",";
	shader_source += this->extend_string("    f", "[2]", 5);
	shader_source += ",";
	shader_source += this->extend_string("    g", "[2]", 6);
	shader_source += ";\n} x;\n\n";
	shader_source += shader_start;

	/* End main */
	DEFAULT_MAIN_ENDING(tested_shader_type, shader_source);

	/* Execute test */
	EXECUTE_POSITIVE_TEST(tested_shader_type, shader_source, true, false);
}

/* Generates the shader source code for the SizedDeclarationsTypenameStyle4
 * array tests, and attempts to compile each test shader, for both
 * vertex and fragment shaders.
 *
 * @tparam API               Tested API descriptor
 *
 * @param tested_shader_type The type of shader that is being tested
 *                           (either TestCaseBase<API>::VERTEX_SHADER_TYPE or TestCaseBase<API>::FRAGMENT_SHADER_TYPE).
 */
template <class API>
void SizedDeclarationsTypenameStyle4<API>::test_shader_compilation(
	typename TestCaseBase<API>::TestShaderType tested_shader_type)
{
	std::string example_struct_begin("struct light {\n");
	std::string example_struct_end("};\n\n");

	for (size_t max_dimension_index = 1; max_dimension_index <= API::MAX_ARRAY_DIMENSIONS; max_dimension_index++)
	{
		std::string shader_source = example_struct_begin;

		shader_source += this->extend_string("    float", "[2]", max_dimension_index);
		shader_source += this->extend_string(" x", "[2]", API::MAX_ARRAY_DIMENSIONS - max_dimension_index);
		shader_source += ";\n";
		shader_source += example_struct_end;
		shader_source += shader_start;

		/* End main */
		DEFAULT_MAIN_ENDING(tested_shader_type, shader_source);

		/* Execute test */
		EXECUTE_POSITIVE_TEST(tested_shader_type, shader_source, true, false);
	} /* for (int max_dimension_index = 1; ...) */
}

/* Generates the shader source code for the SizedDeclarationsTypenameStyle5
 * array tests, and attempts to compile each test shader, for both
 * vertex and fragment shaders.
 *
 * @tparam API               Tested API descriptor
 *
 * @param tested_shader_type The type of shader that is being tested
 *                           (either TestCaseBase<API>::VERTEX_SHADER_TYPE or TestCaseBase<API>::FRAGMENT_SHADER_TYPE).
 */
template <class API>
void SizedDeclarationsTypenameStyle5<API>::test_shader_compilation(
	typename TestCaseBase<API>::TestShaderType tested_shader_type)
{
	std::string example_struct_begin("struct light {\n");
	std::string example_struct_end("};\n\n");

	std::string shader_source = example_struct_begin;

	shader_source += this->extend_string("    float", "[2]", 2);
	shader_source += this->extend_string(" a", "[2]", API::MAX_ARRAY_DIMENSIONS - API::MAX_ARRAY_DIMENSIONS);
	shader_source += ", ";
	shader_source += this->extend_string("b", "[2]", 2);
	shader_source += ", ";
	shader_source += this->extend_string("c", "[2]", 3);
	shader_source += ", ";
	shader_source += this->extend_string("d", "[2]", 4);
	shader_source += ", ";
	shader_source += this->extend_string("e", "[2]", 5);
	shader_source += ", ";
	shader_source += this->extend_string("f", "[2]", 6);
	shader_source += ";\n";
	shader_source += example_struct_end;
	shader_source += shader_start;

	/* End main */
	DEFAULT_MAIN_ENDING(tested_shader_type, shader_source);

	/* Execute test */
	EXECUTE_POSITIVE_TEST(tested_shader_type, shader_source, true, false);
}

/* Generates the shader source code for the SizedDeclarationsFunctionParams
 * array tests, and attempts to compile each test shader, for both
 * vertex and fragment shaders.
 *
 * @tparam API               Tested API descriptor
 *
 * @param tested_shader_type The type of shader that is being tested
 *                           (either TestCaseBase<API>::VERTEX_SHADER_TYPE or TestCaseBase<API>::FRAGMENT_SHADER_TYPE).
 */
template <class API>
void SizedDeclarationsFunctionParams<API>::test_shader_compilation(
	typename TestCaseBase<API>::TestShaderType tested_shader_type)
{
	size_t		dimension_index = 0;
	std::string example_struct1("\nvoid my_function(");
	std::string example_struct2(")\n"
								"{\n"
								"}\n\n");
	std::string base_variable_string;
	std::string variable_basenames[API::MAX_ARRAY_DIMENSIONS] = { "v1", "v2", "v3", "v4", "v5", "v6", "v7", "v8" };
	std::string full_variable_names[API::MAX_ARRAY_DIMENSIONS];

	for (size_t max_dimension_index = 0; max_dimension_index < API::MAX_ARRAY_DIMENSIONS; max_dimension_index++)
	{
		full_variable_names[max_dimension_index] =
			this->extend_string(variable_basenames[max_dimension_index], "[2]", max_dimension_index + 1);
	}

	for (size_t max_dimension_index = 0; max_dimension_index < API::MAX_ARRAY_DIMENSIONS; max_dimension_index++)
	{
		base_variable_string += "float ";
		base_variable_string += full_variable_names[max_dimension_index];
		base_variable_string += ";\n";
	}

	base_variable_string += example_struct1;
	base_variable_string += this->extend_string("float a", "[2]", 1);
	base_variable_string += ", ";
	base_variable_string += this->extend_string("float b", "[2]", 2);
	base_variable_string += ", ";
	base_variable_string += this->extend_string("float c", "[2]", 3);
	base_variable_string += ", ";
	base_variable_string += this->extend_string("float d", "[2]", 4);
	base_variable_string += ", ";
	base_variable_string += this->extend_string("float e", "[2]", 5);
	base_variable_string += ", ";
	base_variable_string += this->extend_string("float f", "[2]", 6);
	base_variable_string += ", ";
	base_variable_string += this->extend_string("float g", "[2]", 7);
	base_variable_string += ", ";
	base_variable_string += this->extend_string("float h", "[2]", 8);
	base_variable_string += example_struct2;

	std::string shader_source = base_variable_string;

	shader_source += shader_start;
	shader_source += "    my_function(";

	for (dimension_index = 0; dimension_index < API::MAX_ARRAY_DIMENSIONS - 1; dimension_index++)
	{
		shader_source += variable_basenames[dimension_index];
		shader_source += ", ";
	}

	shader_source += variable_basenames[dimension_index];
	shader_source += ");\n";

	/* End main */
	DEFAULT_MAIN_ENDING(tested_shader_type, shader_source);

	/* Execute test */
	EXECUTE_POSITIVE_TEST(tested_shader_type, shader_source, true, false);

	/* Only the previous case should succeed, so start from index 1 rather than 0.
	 * The other cases should fail, so only compile them, rather than trying to also link them.
	 * We'll swap items 2/3, then 2/4, then 2/5, then 2/6, ...
	 * Then we'll swap items 3/4, then 3/5, ...
	 * Repeat, starting for 4/5-8, 5/6-8, 6/7-8...
	 * Finally, we'll swap items 7/8
	 */
	for (size_t swap_item = 1; swap_item < API::MAX_ARRAY_DIMENSIONS; swap_item++)
	{
		for (size_t max_dimension_index = swap_item + 1; max_dimension_index < API::MAX_ARRAY_DIMENSIONS;
			 max_dimension_index++)
		{
			std::string temp = variable_basenames[swap_item];

			shader_source							= base_variable_string;
			variable_basenames[swap_item]			= variable_basenames[max_dimension_index];
			variable_basenames[max_dimension_index] = temp;

			shader_source += shader_start;
			shader_source += "    my_function(";

			for (dimension_index = 0; dimension_index < API::MAX_ARRAY_DIMENSIONS - 1; dimension_index++)
			{
				shader_source += variable_basenames[dimension_index];
				shader_source += ", ";
			}

			shader_source += variable_basenames[dimension_index];
			shader_source += ");\n";

			temp									= variable_basenames[swap_item];
			variable_basenames[swap_item]			= variable_basenames[max_dimension_index];
			variable_basenames[max_dimension_index] = temp;

			/* End main */
			DEFAULT_MAIN_ENDING(tested_shader_type, shader_source);

			/* Execute test */
			this->execute_negative_test(tested_shader_type, shader_source);
		} /* for (int max_dimension_index = swap_item + 1; ...) */
	}	 /* for (int swap_item = 1; ...) */
}

/* Generates the shader source code for the sized_declarations_invalid_sizes1
 * array tests, and attempts to compile each test shader, for both
 * vertex and fragment shaders.
 *
 * @tparam API               Tested API descriptor
 *
 * @param tested_shader_type The type of shader that is being tested
 *                           (either TestCaseBase<API>::VERTEX_SHADER_TYPE or TestCaseBase<API>::FRAGMENT_SHADER_TYPE).
 */
template <class API>
void sized_declarations_invalid_sizes1<API>::test_shader_compilation(
	typename TestCaseBase<API>::TestShaderType tested_shader_type)
{
	std::string invalid_declarations[] = {
		"float x[2][2][2][0];\n", "float x[2][2][0][2];\n", "float x[2][0][2][2];\n", "float x[0][2][2][2];\n",
		"float x[2][2][0][0];\n", "float x[2][0][2][0];\n", "float x[0][2][2][0];\n", "float x[2][0][0][2];\n",
		"float x[0][2][0][2];\n", "float x[0][0][2][2];\n", "float x[2][0][0][0];\n", "float x[0][2][0][0];\n",
		"float x[0][0][2][0];\n", "float x[0][0][0][2];\n", "float x[0][0][0][0];\n"
	};

	for (size_t invalid_declarations_index = 0;
		 invalid_declarations_index < sizeof(invalid_declarations) / sizeof(invalid_declarations);
		 invalid_declarations_index++)
	{
		std::string shader_source;

		shader_source = shader_start;
		shader_source += invalid_declarations[invalid_declarations_index];

		/* End main */
		DEFAULT_MAIN_ENDING(tested_shader_type, shader_source);

		/* Execute test */
		this->execute_negative_test(tested_shader_type, shader_source);
	} /* for (int invalid_declarations_index = 0; ...) */
}

/* Generates the shader source code for the sized_declarations_invalid_sizes2
 * array tests, and attempts to compile each test shader, for both
 * vertex and fragment shaders.
 *
 * @tparam API               Tested API descriptor
 *
 * @param tested_shader_type The type of shader that is being tested
 *                           (either TestCaseBase<API>::VERTEX_SHADER_TYPE or TestCaseBase<API>::FRAGMENT_SHADER_TYPE).
 */
template <class API>
void sized_declarations_invalid_sizes2<API>::test_shader_compilation(
	typename TestCaseBase<API>::TestShaderType tested_shader_type)
{
	std::string invalid_declarations[] = {
		"    float x[2][2][2][-1];\n",   "    float x[2][2][-1][2];\n",   "    float x[2][-1][2][2];\n",
		"    float x[-1][2][2][2];\n",   "    float x[2][2][-1][-1];\n",  "    float x[2][-1][2][-1];\n",
		"    float x[-1][2][2][-1];\n",  "    float x[2][-1][-1][2];\n",  "    float x[-1][2][-1][2];\n",
		"    float x[-1][-1][2][2];\n",  "    float x[2][-1][-1][-1];\n", "    float x[-1][2][-1][-1];\n",
		"    float x[-1][-1][2][-1];\n", "    float x[-1][-1][-1][2];\n", "    float x[-1][-1][-1][-1];\n"
	};

	for (size_t invalid_declarations_index = 0;
		 invalid_declarations_index < sizeof(invalid_declarations) / sizeof(invalid_declarations);
		 invalid_declarations_index++)
	{
		std::string shader_source;

		shader_source = shader_start;
		shader_source += invalid_declarations[invalid_declarations_index];

		/* End main */
		DEFAULT_MAIN_ENDING(tested_shader_type, shader_source);

		/* Execute test */
		this->execute_negative_test(tested_shader_type, shader_source);
	} /* for (int invalid_declarations_index = 0; ...) */
}

/* Generates the shader source code for the sized_declarations_invalid_sizes3
 * array tests, and attempts to compile each test shader, for both
 * vertex and fragment shaders.
 *
 * @tparam API               Tested API descriptor
 *
 * @param tested_shader_type The type of shader that is being tested
 *                           (either TestCaseBase<API>::VERTEX_SHADER_TYPE or TestCaseBase<API>::FRAGMENT_SHADER_TYPE).
 */
template <class API>
void sized_declarations_invalid_sizes3<API>::test_shader_compilation(
	typename TestCaseBase<API>::TestShaderType tested_shader_type)
{
	std::string invalid_declarations[] = {
		"    float x[2][2][2][a];\n", "    float x[2][2][a][2];\n", "    float x[2][a][2][2];\n",
		"    float x[a][2][2][2];\n", "    float x[2][2][a][a];\n", "    float x[2][a][2][a];\n",
		"    float x[a][2][2][a];\n", "    float x[2][a][a][2];\n", "    float x[a][2][a][2];\n",
		"    float x[a][a][2][2];\n", "    float x[2][a][a][a];\n", "    float x[a][2][a][a];\n",
		"    float x[a][a][2][a];\n", "    float x[a][a][a][2];\n", "    float x[a][a][a][a];\n"
	};
	std::string non_constant_variable_declaration = "    uint a = 2u;\n";

	for (size_t invalid_declarations_index = 0;
		 invalid_declarations_index < sizeof(invalid_declarations) / sizeof(invalid_declarations);
		 invalid_declarations_index++)
	{
		std::string shader_source;

		shader_source = shader_start;
		shader_source += non_constant_variable_declaration;
		shader_source += invalid_declarations[invalid_declarations_index];

		/* End main */
		DEFAULT_MAIN_ENDING(tested_shader_type, shader_source);

		/* Execute test */
		this->execute_negative_test(tested_shader_type, shader_source);
	} /* for (int invalid_declarations_index = 0; ...) */
}

/* Generates the shader source code for the sized_declarations_invalid_sizes4
 * array tests, and attempts to compile each test shader, for both
 * vertex and fragment shaders.
 *
 * @tparam API               Tested API descriptor
 *
 * @param tested_shader_type The type of shader that is being tested
 *                           (either TestCaseBase<API>::VERTEX_SHADER_TYPE or TestCaseBase<API>::FRAGMENT_SHADER_TYPE).
 */
template <class API>
void sized_declarations_invalid_sizes4<API>::test_shader_compilation(
	typename TestCaseBase<API>::TestShaderType tested_shader_type)
{
	std::string input[] = { "    float x[2,2][2][2];\n", "    float x[2][2,2][2];\n", "    float x[2][2][2,2];\n",
							"    float x[2,2,2][2];\n",  "    float x[2][2,2,2];\n",  "    float x[2,2,2,2];\n" };

	for (size_t string_index = 0; string_index < sizeof(input) / sizeof(input[0]); string_index++)
	{
		std::string shader_source;

		shader_source += shader_start;
		shader_source += input[string_index];

		/* End main */
		DEFAULT_MAIN_ENDING(tested_shader_type, shader_source);

		/* Execute test */
		this->execute_negative_test(tested_shader_type, shader_source);
	} /* for (int string_index = 0; ...) */
}

/* Constructs a suitable constructor for the specified number of dimensions.
 *
 * @tparam API            Tested API descriptor
 *
 * @param var_type        The type of the variable
 * @param dimension_index The current recursion level (counts down)
 * @param init_string     The initialisation string
 */
template <class API>
std::string ConstructorsAndUnsizedDeclConstructors1<API>::recursively_initialise(std::string var_type,
																				 size_t		 dimension_index,
																				 std::string init_string)
{
	std::string temp_string;

	if (dimension_index == 0)
	{
		temp_string = init_string;
	}
	else
	{
		std::string prefix = "\n";

		for (size_t indent_index = dimension_index; indent_index < (API::MAX_ARRAY_DIMENSIONS + 1); indent_index++)
		{
			prefix += "    ";
		}

		prefix += this->extend_string(var_type, "[]", dimension_index);
		prefix += "(";
		for (int sub_script_index = 0; sub_script_index < 2; sub_script_index++)
		{
			temp_string += prefix;
			temp_string += recursively_initialise(var_type, dimension_index - 1, init_string);
			prefix = ", ";
			if (sub_script_index == 1)
			{
				break;
			}
		}
		temp_string += ")";
	}

	return temp_string;
}

/* Generates the shader source code for the ConstructorsAndUnsizedDeclConstructors1
 * array tests, and attempts to compile each test shader, for both
 * vertex and fragment shaders.
 *
 * @tparam API               Tested API descriptor
 *
 * @param tested_shader_type The type of shader that is being tested
 *                           (either TestCaseBase<API>::VERTEX_SHADER_TYPE or TestCaseBase<API>::FRAGMENT_SHADER_TYPE).
 */
template <class API>
void ConstructorsAndUnsizedDeclConstructors1<API>::test_shader_compilation(
	typename TestCaseBase<API>::TestShaderType tested_shader_type)
{
	//vec4 color = vec4(0.0, 1.0, 0.0, 1.0);
	int num_var_types = API::n_var_types;

	for (int var_type_index = 0; var_type_index < num_var_types; var_type_index++)
	{
		_supported_variable_types_map_const_iterator var_iterator =
			supported_variable_types_map.find(API::var_types[var_type_index]);

		if (var_iterator != supported_variable_types_map.end())
		{
			for (size_t max_dimension_index = 2; max_dimension_index < API::MAX_ARRAY_DIMENSIONS; max_dimension_index++)
			{
				std::string base_variable_string =
					this->extend_string("    " + var_iterator->second.type + " a", "[]", max_dimension_index);

				base_variable_string += " = ";
				base_variable_string += recursively_initialise(var_iterator->second.type, max_dimension_index,
															   var_iterator->second.initializer_with_ones);
				base_variable_string += ";\n\n";

				std::string shader_source = shader_start + base_variable_string;

				/* End main */
				DEFAULT_MAIN_ENDING(tested_shader_type, shader_source);

				/* Execute test */
				EXECUTE_POSITIVE_TEST(tested_shader_type, shader_source, true, false);
			} /* for (int max_dimension_index = 1; ...) */
		}	 /* if var_type iterator found */
		else
		{
			TCU_FAIL("Type not found.");
		}
	} /* for (int var_type_index = 0; ...) */

	for (int var_type_index = 0; var_type_index < num_var_types; var_type_index++)
	{
		_supported_variable_types_map_const_iterator var_iterator =
			supported_variable_types_map.find(API::var_types[var_type_index]);

		if (var_iterator != supported_variable_types_map.end())
		{
			std::string base_structure = "struct my_structure\n";

			base_structure += "{\n";
			base_structure += "    " + var_iterator->second.type + " b;\n";
			base_structure += "    " + var_iterator->second.type + " c;\n";
			base_structure += "};\n\n";

			for (size_t max_dimension_index = 1; max_dimension_index < API::MAX_ARRAY_DIMENSIONS; max_dimension_index++)
			{
				std::string outer_separator = "(";
				std::string base_variable_string;

				base_variable_string +=
					this->extend_string("    " + var_iterator->second.type + " a", "[2]", max_dimension_index);
				base_variable_string += " = ";
				base_variable_string += recursively_initialise(var_iterator->second.type, max_dimension_index,
															   var_iterator->second.initializer_with_ones);
				base_variable_string += ";\n\n";

				std::string shader_source = base_structure + shader_start + base_variable_string;

				/* End main */
				DEFAULT_MAIN_ENDING(tested_shader_type, shader_source);

				/* Execute test */
				EXECUTE_POSITIVE_TEST(tested_shader_type, shader_source, true, false);
			} /* for (int max_dimension_index = 1; ...) */
		}	 /* if var_type iterator found */
		else
		{
			TCU_FAIL("Type not found.");
		}
	} /* for (int var_type_index = 0; ...) */
}

/* Generates the shader source code for the ConstructorsAndUnsizedDeclConstructors2
 * array tests, and attempts to compile each test shader, for both
 * vertex and fragment shaders.
 *
 * @tparam API               Tested API descriptor
 *
 * @param tested_shader_type The type of shader that is being tested
 *                           (either TestCaseBase<API>::VERTEX_SHADER_TYPE or TestCaseBase<API>::FRAGMENT_SHADER_TYPE).
 */
template <class API>
void ConstructorsAndUnsizedDeclConstructors2<API>::test_shader_compilation(
	typename TestCaseBase<API>::TestShaderType tested_shader_type)
{
	std::string base_variable_string = "    float[2][2] x = float[2][2](float[4](1.0, 2.0, 3.0, 4.0));\n";
	std::string shader_source		 = shader_start + base_variable_string;

	/* End main */
	DEFAULT_MAIN_ENDING(tested_shader_type, shader_source);

	/* Execute test */
	this->execute_negative_test(tested_shader_type, shader_source);

	base_variable_string = "float[2][2] x = float[2][2](float[1][4](float[4](1.0, 2.0, 3.0, 4.0)));\n\n";
	shader_source		 = base_variable_string + shader_start;

	/* End main */
	DEFAULT_MAIN_ENDING(tested_shader_type, shader_source);

	/* Execute test */
	this->execute_negative_test(tested_shader_type, shader_source);
}

/* Generates the shader source code for the ConstructorsAndUnsizedDeclUnsizedConstructors
 * array tests, and attempts to compile each test shader, for both
 * vertex and fragment shaders.
 *
 * @tparam API               Tested API descriptor
 *
 * @param tested_shader_type The type of shader that is being tested
 *                           (either TestCaseBase<API>::VERTEX_SHADER_TYPE or TestCaseBase<API>::FRAGMENT_SHADER_TYPE).
 */
template <class API>
void ConstructorsAndUnsizedDeclUnsizedConstructors<API>::test_shader_compilation(
	typename TestCaseBase<API>::TestShaderType tested_shader_type)
{
	std::string shader_variable_declarations = "= float[2][1][2][1](\n"
											   "        float[1][2][1](\n"
											   "            float[2][1]( \n"
											   "                float[1](12.3), float[1](54.2) \n"
											   "            )\n"
											   "        ),\n"
											   "        float[1][2][1](\n"
											   "            float[2][1]( \n"
											   "                float[1]( 3.2), float[1]( 7.4) \n"
											   "            )\n"
											   "        )\n"
											   "    );\n\n";

	std::string input[] = { "float a[2][1][2][]", "float a[2][1][][1]", "float a[2][1][][]", "float a[2][][2][1]",
							"float a[2][][2][]",  "float a[2][][][1]",  "float a[2][][][]",  "float a[][1][2][1]",
							"float a[][1][2][]",  "float a[][1][][1]",  "float a[][1][][]",  "float a[][][2][1]",
							"float a[][][2][]",   "float a[][][][1]",   "float a[][][][]" };

	for (size_t string_index = 0; string_index < sizeof(input) / sizeof(input[0]); string_index++)
	{
		std::string shader_source = shader_start + "    " + input[string_index] + shader_variable_declarations;

		/* End main */
		DEFAULT_MAIN_ENDING(tested_shader_type, shader_source);

		/* Execute test */
		EXECUTE_POSITIVE_TEST(tested_shader_type, shader_source, true, false);
	} /* for (int string_index = 0; ...) */
}

/* Generates the shader source code for the ConstructorsAndUnsizedDeclConst
 * array tests, and attempts to compile each test shader, for both
 * vertex and fragment shaders.
 *
 * @tparam API               Tested API descriptor
 *
 * @param tested_shader_type The type of shader that is being tested
 *                           (either TestCaseBase<API>::VERTEX_SHADER_TYPE or TestCaseBase<API>::FRAGMENT_SHADER_TYPE).
 */
template <class API>
void ConstructorsAndUnsizedDeclConst<API>::test_shader_compilation(
	typename TestCaseBase<API>::TestShaderType tested_shader_type)
{
	std::string shader_source = "const float[2][2] x = float[2][2](float[2](1.0, 2.0), float[2](3.0, 4.0));\n\n";
	shader_source += shader_start;

	/* End main */
	DEFAULT_MAIN_ENDING(tested_shader_type, shader_source);

	/* Execute test */
	EXECUTE_POSITIVE_TEST(tested_shader_type, shader_source, true, false);
}

/* Generates the shader source code for the ConstructorsAndUnsizedDeclInvalidConstructors1
 * array tests, and attempts to compile each test shader, for both
 * vertex and fragment shaders.
 *
 * @tparam API               Tested API descriptor
 *
 * @param tested_shader_type The type of shader that is being tested
 *                           (either TestCaseBase<API>::VERTEX_SHADER_TYPE or TestCaseBase<API>::FRAGMENT_SHADER_TYPE).
 */
template <class API>
void ConstructorsAndUnsizedDeclInvalidConstructors1<API>::test_shader_compilation(
	typename TestCaseBase<API>::TestShaderType tested_shader_type)
{
	int num_var_types = sizeof(opaque_var_types) / sizeof(opaque_var_types[0]);

	for (int var_type_index = 0; var_type_index < num_var_types; var_type_index++)
	{
		_supported_variable_types_map_const_iterator var_iterator =
			supported_variable_types_map.find(opaque_var_types[var_type_index]);

		if (var_iterator != supported_variable_types_map.end())
		{
			std::string base_variable_string =
				"uniform " + var_iterator->second.precision + " " + var_iterator->second.type + " my_sampler1;\n" +
				"uniform " + var_iterator->second.precision + " " + var_iterator->second.type + " my_sampler2;\n" +
				"uniform " + var_iterator->second.precision + " " + var_iterator->second.type + " my_sampler3;\n" +
				"uniform " + var_iterator->second.precision + " " + var_iterator->second.type + " my_sampler4;\n\n";

			std::string shader_source = base_variable_string + shader_start;
			shader_source += "    const " + var_iterator->second.type + "[2][2] x = " + var_iterator->second.type +
							 "[2][2](" + var_iterator->second.type + "[2](my_sampler1, my_sampler2), " +
							 var_iterator->second.type + "[2](my_sampler3, my_sampler4));\n\n";

			/* End main */
			DEFAULT_MAIN_ENDING(tested_shader_type, shader_source);

			/* Execute test */
			this->execute_negative_test(tested_shader_type, shader_source);
		} /* if var_type iterator found */
		else
		{
			TCU_FAIL("Type not found.");
		}
	} /* for (int var_type_index = 0; ...) */
}

/* Generates the shader source code for the ConstructorsAndUnsizedDeclInvalidConstructors2
 * array tests, and attempts to compile each test shader, for both
 * vertex and fragment shaders.
 *
 * @tparam API               Tested API descriptor
 *
 * @param tested_shader_type The type of shader that is being tested
 *                           (either TestCaseBase<API>::VERTEX_SHADER_TYPE or TestCaseBase<API>::FRAGMENT_SHADER_TYPE).
 */
template <class API>
void ConstructorsAndUnsizedDeclInvalidConstructors2<API>::test_shader_compilation(
	typename TestCaseBase<API>::TestShaderType tested_shader_type)
{
	std::string invalid_initializers[] = { "    int x[2][2][0]; \n", "    int x[2][0][2]; \n", "    int x[0][2][2]; \n",
										   "    int x[2][0][0]; \n", "    int x[0][2][0]; \n", "    int x[0][0][2]; \n",
										   "    int x[0][0][0]; \n" };

	for (size_t invalid_initializers_index = 0;
		 invalid_initializers_index < sizeof(invalid_initializers) / sizeof(invalid_initializers[0]);
		 invalid_initializers_index++)
	{
		std::string shader_source;

		shader_source = shader_start;
		shader_source += invalid_initializers[invalid_initializers_index];

		/* End main */
		DEFAULT_MAIN_ENDING(tested_shader_type, shader_source);

		/* Execute test */
		this->execute_negative_test(tested_shader_type, shader_source);
	} /* for (int invalid_initializers_index = 0; ...) */
}

/* Generates the shader source code for the ConstructorsAndUnsizedDeclInvalidConstructors3
 * array tests, and attempts to compile each test shader, for both
 * vertex and fragment shaders.
 *
 * @tparam API               Tested API descriptor
 *
 * @param tested_shader_type The type of shader that is being tested
 *                           (either TestCaseBase<API>::VERTEX_SHADER_TYPE or TestCaseBase<API>::FRAGMENT_SHADER_TYPE).
 */
template <class API>
void ConstructorsAndUnsizedDeclInvalidConstructors3<API>::test_shader_compilation(
	typename TestCaseBase<API>::TestShaderType tested_shader_type)
{
	std::string invalid_initializers[] = { "    int x[2][2][-1]; \n",  "    int x[2][-1][2]; \n",
										   "    int x[-1][2][2]; \n",  "    int x[2][-1][-1]; \n",
										   "    int x[-1][2][-1]; \n", "    int x[-1][-1][2]; \n",
										   "    int x[-1][-1][-1]; \n" };

	for (size_t invalid_initializers_index = 0;
		 invalid_initializers_index < sizeof(invalid_initializers) / sizeof(invalid_initializers[0]);
		 invalid_initializers_index++)
	{
		std::string shader_source;

		shader_source = shader_start;
		shader_source += invalid_initializers[invalid_initializers_index];

		/* End main */
		DEFAULT_MAIN_ENDING(tested_shader_type, shader_source);

		/* Execute test */
		this->execute_negative_test(tested_shader_type, shader_source);
	} /* for (int invalid_initializers_index = 0; ...) */
}

/* Generates the shader source code for the ConstructorsAndUnsizedDeclInvalidConstructors4
 * array tests, and attempts to compile each test shader, for both
 * vertex and fragment shaders.
 *
 * @tparam API               Tested API descriptor
 *
 * @param tested_shader_type The type of shader that is being tested
 *                           (either TestCaseBase<API>::VERTEX_SHADER_TYPE or TestCaseBase<API>::FRAGMENT_SHADER_TYPE).
 */
template <class API>
void ConstructorsAndUnsizedDeclInvalidConstructors4<API>::test_shader_compilation(
	typename TestCaseBase<API>::TestShaderType tested_shader_type)
{
	std::string invalid_initializers[] = { "    int x[2][2][a]; \n", "    int x[2][a][2]; \n", "    int x[a][2][2]; \n",
										   "    int x[2][a][a]; \n", "    int x[a][2][a]; \n", "    int x[a][a][2]; \n",
										   "    int x[a][a][a]; \n" };
	std::string non_constant_variable_init = "    uint a = 2u;\n";

	for (size_t invalid_initializers_index = 0;
		 invalid_initializers_index < sizeof(invalid_initializers) / sizeof(invalid_initializers[0]);
		 invalid_initializers_index++)
	{
		std::string shader_source;

		shader_source = shader_start;
		shader_source += non_constant_variable_init;
		shader_source += invalid_initializers[invalid_initializers_index];

		/* End main */
		DEFAULT_MAIN_ENDING(tested_shader_type, shader_source);

		/* Execute test */
		this->execute_negative_test(tested_shader_type, shader_source);
	} /* for (int invalid_initializers_index = 0; ...) */
}

/* Generates the shader source code for the ConstructorsAndUnsizedDeclConstructorSizing1
 * array tests, and attempts to compile each test shader, for both
 * vertex and fragment shaders.
 *
 * @tparam API               Tested API descriptor
 *
 * @param tested_shader_type The type of shader that is being tested
 *                           (either TestCaseBase<API>::VERTEX_SHADER_TYPE or TestCaseBase<API>::FRAGMENT_SHADER_TYPE).
 */
template <class API>
void ConstructorsAndUnsizedDeclConstructorSizing1<API>::test_shader_compilation(
	typename TestCaseBase<API>::TestShaderType tested_shader_type)
{
	std::string valid_size_initializers[] = { "[1][1][1][]", "[1][1][][1]", "[1][][1][1]", "[][1][1][1]", "[1][1][][]",
											  "[1][][1][]",  "[][1][1][]",  "[1][][][1]",  "[][1][][1]",  "[][][1][1]",
											  "[1][][][]",   "[][1][][]",   "[][][1][]",   "[][][][1]",   "[][][][]" };

	for (size_t var_type_index = 0; var_type_index < API::n_var_types; var_type_index++)
	{
		_supported_variable_types_map_const_iterator var_iterator =
			supported_variable_types_map.find(API::var_types[var_type_index]);

		if (var_iterator != supported_variable_types_map.end())
		{
			for (size_t valid_size_initializers_index = 0;
				 valid_size_initializers_index < sizeof(valid_size_initializers) / sizeof(valid_size_initializers[0]);
				 valid_size_initializers_index++)
			{
				std::string shader_source;
				std::string variable_constructor =
					"    " + var_iterator->second.type + " x" + valid_size_initializers[valid_size_initializers_index] +
					" = " + var_iterator->second.type + "[1][1][1][1](" + var_iterator->second.type + "[1][1][1](" +
					var_iterator->second.type + "[1][1](" + var_iterator->second.type + "[1](" +
					var_iterator->second.initializer_with_zeroes + "))));\n";

				shader_source = shader_start;
				shader_source += variable_constructor;

				/* End main */
				DEFAULT_MAIN_ENDING(tested_shader_type, shader_source);

				/* Execute test */
				EXECUTE_POSITIVE_TEST(tested_shader_type, shader_source, true, false);
			} /* for (int valid_size_initializers_index = 0; ...) */
		}	 /* if var_type iterator found */
		else
		{
			TCU_FAIL("Type not found.");
		}
	} /* for (int var_type_index = 0; ...) */
}

/* Generates the shader source code for the ConstructorsAndUnsizedDeclConstructorSizing2
 * array tests, and attempts to compile each test shader, for both
 * vertex and fragment shaders.
 *
 * @tparam API               Tested API descriptor
 *
 * @param tested_shader_type The type of shader that is being tested
 *                           (either TestCaseBase<API>::VERTEX_SHADER_TYPE or TestCaseBase<API>::FRAGMENT_SHADER_TYPE).
 */
template <class API>
void ConstructorsAndUnsizedDeclConstructorSizing2<API>::test_shader_compilation(
	typename TestCaseBase<API>::TestShaderType tested_shader_type)
{
	std::string shader_source = shader_start;

	shader_source += "    float[] a ="
					 "            float[](1.0, 2.0),"
					 "        b[] ="
					 "         float[][]("
					 "             float[](1.0, 2.0),"
					 "             float[](3.0, 4.0)"
					 "         ),"
					 "        c[][] ="
					 "         float[][][]("
					 "             float[][]("
					 "                 float[](1.0),"
					 "                 float[](2.0)"
					 "             )"
					 "         ),"
					 "        d[][][] ="
					 "         float[][][][]("
					 "             float[][][]("
					 "                 float[][]("
					 "                     float[](1.0, 2.0),"
					 "                     float[](3.0, 4.0),"
					 "                     float[](5.0, 6.0)"
					 "                 )"
					 "             ),"
					 "             float[][][]("
					 "                 float[][]("
					 "                     float[](1.0, 2.0),"
					 "                     float[](3.0, 4.0),"
					 "                     float[](5.0, 6.0)"
					 "                 )"
					 "             )"
					 "         ),"
					 "        e[][][][]="
					 "         float[][][][][]("
					 "             float[][][][]("
					 "                 float[][][]("
					 "                     float[][]("
					 "                         float[](1.0),"
					 "                         float[](2.0)"
					 "                     ),"
					 "                     float[][]("
					 "                         float[](1.0),"
					 "                         float[](2.0)"
					 "                     ),"
					 "                     float[][]("
					 "                         float[](1.0),"
					 "                         float[](2.0)"
					 "                     )"
					 "                 ),"
					 "                 float[][][]("
					 "                     float[][]("
					 "                         float[](1.0),"
					 "                         float[](2.0)"
					 "                     ),"
					 "                     float[][]("
					 "                         float[](1.0),"
					 "                         float[](2.0)"
					 "                     ),"
					 "                     float[][]("
					 "                         float[](1.0),"
					 "                         float[](2.0)"
					 "                     )"
					 "                 )"
					 "             )"
					 "         ),"
					 "        f[][][][][]="
					 "         float[][][][][][]("
					 "             float[][][][][]("
					 "                 float[][][][]("
					 "                     float[][][]("
					 "                         float[][]("
					 "                             float[](1.0)"
					 "                         )"
					 "                     )"
					 "                 )"
					 "             )"
					 "         ),"
					 "        g[][][][][][]="
					 "         float[][][][][][][]("
					 "             float[][][][][][]("
					 "                 float[][][][][]("
					 "                     float[][][][]("
					 "                         float[][][]("
					 "                             float[][]("
					 "                                 float[](1.0)"
					 "                             )"
					 "                         )"
					 "                     )"
					 "                 )"
					 "             )"
					 "         ),"
					 "        h[][][][][][][]="
					 "         float[][][][][][][][]("
					 "             float[][][][][][][]("
					 "                 float[][][][][][]("
					 "                     float[][][][][]("
					 "                         float[][][][]("
					 "                             float[][][]("
					 "                                 float[][]("
					 "                                     float[](1.0)"
					 "                                 )"
					 "                             )"
					 "                         )"
					 "                     )"
					 "                 )"
					 "             )"
					 "         );\n";

	/* End main */
	DEFAULT_MAIN_ENDING(tested_shader_type, shader_source);

	/* Execute test */
	EXECUTE_POSITIVE_TEST(tested_shader_type, shader_source, true, false);
}

/* Constructs a suitable constructor for the specified number of dimensions.
 *
 * @tparam API            Tested API descriptor
 *
 * @param var_type        The type of the variable
 * @param dimension_index The current recursion level (counts down)
 * @param init_string     The initialisation string
 */
template <class API>
std::string ConstructorsAndUnsizedDeclStructConstructors<API>::recursively_initialise(std::string var_type,
																					  size_t	  dimension_index,
																					  std::string init_string)
{
	std::string temp_string;

	if (dimension_index == 0)
	{
		temp_string = var_type + "(" + init_string + ")";
	}
	else
	{
		std::string prefix = "\n";

		for (size_t indent_index = dimension_index; indent_index < (API::MAX_ARRAY_DIMENSIONS + 1); indent_index++)
		{
			prefix += "    ";
		}

		prefix += this->extend_string(var_type, "[]", dimension_index);
		prefix += "(";

		for (int sub_script_index = 0; sub_script_index < 2; sub_script_index++)
		{
			temp_string += prefix;
			temp_string += recursively_initialise(var_type, dimension_index - 1, init_string);
			prefix = ", ";

			if (dimension_index == 1)
			{
				break;
			}
		}
		temp_string += ")";
	}

	return temp_string;
}

/* Generates the shader source code for the ConstructorsAndUnsizedDeclStructConstructors
 * array tests, and attempts to compile each test shader, for both
 * vertex and fragment shaders.
 *
 * @tparam API               Tested API descriptor
 *
 * @param tested_shader_type The type of shader that is being tested
 *                           (either TestCaseBase<API>::VERTEX_SHADER_TYPE or TestCaseBase<API>::FRAGMENT_SHADER_TYPE).
 */
template <class API>
void ConstructorsAndUnsizedDeclStructConstructors<API>::test_shader_compilation(
	typename TestCaseBase<API>::TestShaderType tested_shader_type)
{
	std::string example_structure_definition("struct light {\n"
											 "    float intensity;\n"
											 "    int position;\n"
											 "};\n");
	std::string example_structure_object("    light my_light_variable");

	for (size_t max_dimension_index = 2; max_dimension_index <= API::MAX_ARRAY_DIMENSIONS; max_dimension_index++)
	{
		std::string base_variable_string = this->extend_string(example_structure_object, "[]", max_dimension_index);
		base_variable_string += " = ";
		base_variable_string += recursively_initialise("light", max_dimension_index, "1.0, 2");
		base_variable_string += ";\n\n";

		std::string shader_source = example_structure_definition + shader_start + base_variable_string;

		/* End main */
		DEFAULT_MAIN_ENDING(tested_shader_type, shader_source);

		/* Execute test */
		EXECUTE_POSITIVE_TEST(tested_shader_type, shader_source, true, false);
	} /* for (int max_dimension_index = 2; ...) */
}

/* Generates the shader source code for the ConstructorsAndUnsizedDeclUnsizedArrays1
 * array tests, and attempts to compile each test shader, for both
 * vertex and fragment shaders.
 *
 * @tparam API               Tested API descriptor
 *
 * @param tested_shader_type The type of shader that is being tested
 *                           (either TestCaseBase<API>::VERTEX_SHADER_TYPE or TestCaseBase<API>::FRAGMENT_SHADER_TYPE).
 */
template <class API>
void ConstructorsAndUnsizedDeclUnsizedArrays1<API>::test_shader_compilation(
	typename TestCaseBase<API>::TestShaderType tested_shader_type)
{
	std::string base_variable_string;

	for (size_t max_dimension_index = 2; max_dimension_index <= API::MAX_ARRAY_DIMENSIONS; max_dimension_index++)
	{
		base_variable_string = this->extend_string("    int x", "[]", max_dimension_index);
		base_variable_string += ";\n\n";

		std::string shader_source = shader_start + base_variable_string;

		/* End main */
		DEFAULT_MAIN_ENDING(tested_shader_type, shader_source);

		/* Execute test */
		this->execute_negative_test(tested_shader_type, shader_source);
	} /* for (int max_dimension_index = 2; ...) */
}

/* Generates the shader source code for the ConstructorsAndUnsizedDeclUnsizedArrays2
 * array tests, and attempts to compile each test shader, for both
 * vertex and fragment shaders.
 *
 * @tparam API               Tested API descriptor
 *
 * @param tested_shader_type The type of shader that is being tested
 *                           (either TestCaseBase<API>::VERTEX_SHADER_TYPE or TestCaseBase<API>::FRAGMENT_SHADER_TYPE).
 */
template <class API>
void ConstructorsAndUnsizedDeclUnsizedArrays2<API>::test_shader_compilation(
	typename TestCaseBase<API>::TestShaderType tested_shader_type)
{
	std::string input[] = { "    float [] x = float[](1), y;\n\n" };

	for (size_t string_index = 0; string_index < sizeof(input) / sizeof(input[0]); string_index++)
	{
		std::string shader_source;

		shader_source += shader_start;
		shader_source += input[string_index];

		/* End main */
		DEFAULT_MAIN_ENDING(tested_shader_type, shader_source);

		/* Execute test */
		EXECUTE_SHADER_TEST(API::ALLOW_UNSIZED_DECLARATION, tested_shader_type, shader_source);
	} /* for (int string_index = 0; ...) */
}

/* Generates the shader source code for the ConstructorsAndUnsizedDeclUnsizedArrays3
 * array tests, and attempts to compile each test shader, for both
 * vertex and fragment shaders.
 *
 * @tparam API               Tested API descriptor
 *
 * @param tested_shader_type The type of shader that is being tested
 *                           (either TestCaseBase<API>::VERTEX_SHADER_TYPE or TestCaseBase<API>::FRAGMENT_SHADER_TYPE).
 */
template <class API>
void ConstructorsAndUnsizedDeclUnsizedArrays3<API>::test_shader_compilation(
	typename TestCaseBase<API>::TestShaderType tested_shader_type)
{
	std::string base_variable_string("    float[][] x = mat4(0);\n\n");

	std::string shader_source = shader_start + base_variable_string;

	/* End main */
	DEFAULT_MAIN_ENDING(tested_shader_type, shader_source);

	/* Execute test */
	this->execute_negative_test(tested_shader_type, shader_source);
}

/* Generates the shader source code for the ConstructorsAndUnsizedDeclUnsizedArrays4
 * array tests, and attempts to compile each test shader, for both
 * vertex and fragment shaders.
 *
 * @tparam API               Tested API descriptor
 *
 * @param tested_shader_type The type of shader that is being tested
 *                           (either TestCaseBase<API>::VERTEX_SHADER_TYPE or TestCaseBase<API>::FRAGMENT_SHADER_TYPE).
 */
template <class API>
void ConstructorsAndUnsizedDeclUnsizedArrays4<API>::test_shader_compilation(
	typename TestCaseBase<API>::TestShaderType tested_shader_type)
{
	std::string example_struct("struct light {\n"
							   "    float[][] intensity;\n"
							   "    int       position;\n"
							   "} myLight;\n\n");

	std::string shader_source = example_struct + shader_start;

	/* End main */
	DEFAULT_MAIN_ENDING(tested_shader_type, shader_source);

	/* Execute test */
	this->execute_negative_test(tested_shader_type, shader_source);
}

/* Constructs a suitable constructor for the specified number of dimensions.
 *
 * @tparam API            Tested API descriptor
 *
 * @param dimension_index The current recursion level (counts down)
 * @param init_string     The initialisation string
 */
template <class API>
std::string ExpressionsAssignment1<API>::recursively_initialise(std::string var_type, size_t dimension_index,
																std::string init_string)
{
	std::string temp_string;

	if (dimension_index == 0)
	{
		temp_string = init_string;
	}
	else
	{
		std::string prefix = "\n";

		for (size_t indent_index = dimension_index; indent_index < (API::MAX_ARRAY_DIMENSIONS + 1); indent_index++)
		{
			prefix += "    ";
		}

		prefix += this->extend_string(var_type, "[]", dimension_index);
		prefix += "(";
		for (int sub_script_index = 0; sub_script_index < 2; sub_script_index++)
		{
			temp_string += prefix;
			temp_string += recursively_initialise(var_type, dimension_index - 1, init_string);
			prefix = ", ";
			if (dimension_index == 1)
			{
				break;
			}
		}
		temp_string += ")";
	}

	return temp_string;
}

/* Generates the shader source code for the ExpressionsAssignment1
 * array tests, and attempts to compile each test shader, for both
 * vertex and fragment shaders.
 *
 * @tparam API               Tested API descriptor
 *
 * @param tested_shader_type The type of shader that is being tested
 *                           (either TestCaseBase<API>::VERTEX_SHADER_TYPE or TestCaseBase<API>::FRAGMENT_SHADER_TYPE).
 */
template <class API>
void ExpressionsAssignment1<API>::test_shader_compilation(typename TestCaseBase<API>::TestShaderType tested_shader_type)
{
	for (size_t max_dimension_index = 2; max_dimension_index <= API::MAX_ARRAY_DIMENSIONS; max_dimension_index++)
	{
		std::string prefix = "(";
		std::string base_variable_string;

		base_variable_string += this->extend_string("    float x", "[2]", max_dimension_index);
		base_variable_string += " = ";
		base_variable_string += recursively_initialise("float", max_dimension_index, "4.0, 6.0");
		base_variable_string += ";\n";
		base_variable_string += this->extend_string("    float y", "[2]", max_dimension_index);
		base_variable_string += " = ";
		base_variable_string += recursively_initialise("float", max_dimension_index, "1.0, 2.0");
		base_variable_string += ";\n\n";

		std::string shader_source = shader_start + base_variable_string;

		shader_source += "    x = y;\n";

		/* End main */
		DEFAULT_MAIN_ENDING(tested_shader_type, shader_source);

		/* Execute test */
		EXECUTE_POSITIVE_TEST(tested_shader_type, shader_source, true, false);
	} /* for (int max_dimension_index = 2; ...) */
}

/* Generates the shader source code for the ExpressionsAssignment2
 * array tests, and attempts to compile each test shader, for both
 * vertex and fragment shaders.
 *
 * @tparam API               Tested API descriptor
 *
 * @param tested_shader_type The type of shader that is being tested
 *                           (either TestCaseBase<API>::VERTEX_SHADER_TYPE or TestCaseBase<API>::FRAGMENT_SHADER_TYPE).
 */
template <class API>
void ExpressionsAssignment2<API>::test_shader_compilation(typename TestCaseBase<API>::TestShaderType tested_shader_type)
{
	std::string shader_body("    float a[2] = float[](1.0, 2.0);\n"
							"    float b[2][2] = float[][](float[](1.0, 2.0), float[](1.0, 2.0));\n"
							"    float c[2][2][2] = float[][][]("
							"float[][](float[](1.0, 2.0), float[](1.0, 2.0)),"
							"float[][](float[](1.0, 2.0), float[](1.0, 2.0)));\n"
							"    float d[2][2][2][2] = float[][][][]("
							"float[][][]("
							"float[][](float[](1.0, 2.0), float[](1.0, 2.0)), "
							"float[][](float[](1.0, 2.0), float[](1.0, 2.0))),"
							"float[][][]("
							"float[][](float[](1.0, 2.0), float[](1.0, 2.0)), "
							"float[][](float[](1.0, 2.0), float[](1.0, 2.0))));\n\n");

	std::string variable_basenames[] = { "a", "b", "c", "d" };
	int			number_of_elements   = sizeof(variable_basenames) / sizeof(variable_basenames[0]);

	for (int variable_index = 0; variable_index < number_of_elements; variable_index++)
	{
		for (int value_index = variable_index; value_index < number_of_elements; value_index++)
		{
			std::string shader_source = shader_start + shader_body;

			/* Avoid the situation when a variable is assign to itself. */
			if (variable_index != value_index)
			{
				shader_source += "    " + variable_basenames[variable_index] + " = " + variable_basenames[value_index];
				shader_source += ";\n";

				/* End main */
				DEFAULT_MAIN_ENDING(tested_shader_type, shader_source);

				/* Execute test */
				this->execute_negative_test(tested_shader_type, shader_source);
			} /* if(variable_index != value_index) */
		}	 /* for (int value_index = variable_index; ...) */
	}		  /* for (int variable_index = 0; ...) */
}

/* Generates the shader source code for the ExpressionsAssignment3
 * array tests, and attempts to compile each test shader, for both
 * vertex and fragment shaders.
 *
 * @tparam API               Tested API descriptor
 *
 * @param tested_shader_type The type of shader that is being tested
 *                           (either TestCaseBase<API>::VERTEX_SHADER_TYPE or TestCaseBase<API>::FRAGMENT_SHADER_TYPE).
 */
template <class API>
void ExpressionsAssignment3<API>::test_shader_compilation(typename TestCaseBase<API>::TestShaderType tested_shader_type)
{
	std::string prefix, base_variable_string;

	const int test_array_dimensions = 4;

	prefix = this->extend_string("    float a", "[1]", 4);
	prefix += " = float[][][][](\n"
			  "       float[][][](\n"
			  "           float[][](\n"
			  "               float[](1.0))));\n";

	prefix += "    float b";

	for (int permutation = 0; permutation < (1 << test_array_dimensions); permutation++)
	{
		base_variable_string = prefix;

		for (int sub_script_index = test_array_dimensions - 1; sub_script_index >= 0; sub_script_index--)
		{
			if (permutation & (1 << sub_script_index))
			{
				base_variable_string += "[1]";
			}
			else
			{
				base_variable_string += "[2]";
			}
		}

		base_variable_string += ";\n\n";

		if (permutation != (1 << test_array_dimensions) - 1)
		{
			std::string shader_source = shader_start + base_variable_string;

			shader_source += "    b = a;\n";

			/* End main */
			DEFAULT_MAIN_ENDING(tested_shader_type, shader_source);

			/* Execute test */
			this->execute_negative_test(tested_shader_type, shader_source);
		} /* if (permutation != (1 << test_array_dimensions) - 1) */
	}	 /* for (int permutation = 0; ...) */
}

/* Generates the shader source code for the ExpressionsTypeRestrictions1
 * array tests, and attempts to compile each test shader, for both
 * vertex and fragment shaders.
 *
 * @tparam API               Tested API descriptor
 *
 * @param tested_shader_type The type of shader that is being tested
 *                           (either TestCaseBase<API>::VERTEX_SHADER_TYPE or TestCaseBase<API>::FRAGMENT_SHADER_TYPE).
 */
template <class API>
void ExpressionsTypeRestrictions1<API>::test_shader_compilation(
	typename TestCaseBase<API>::TestShaderType tested_shader_type)
{
	int num_var_types = sizeof(opaque_var_types) / sizeof(opaque_var_types[0]);

	for (int var_type_index = 0; var_type_index < num_var_types; var_type_index++)
	{
		_supported_variable_types_map_const_iterator var_iterator =
			supported_variable_types_map.find(opaque_var_types[var_type_index]);

		if (var_iterator != supported_variable_types_map.end())
		{
			std::string shader_source =
				"uniform " + var_iterator->second.precision + " " + var_iterator->second.type + " var1[2][2];\n"
																								"uniform " +
				var_iterator->second.precision + " " + var_iterator->second.type + " var2[2][2];\n\n";
			shader_source += shader_start;

			shader_source += "    var1 = var2;\n";

			/* End main */
			DEFAULT_MAIN_ENDING(tested_shader_type, shader_source);

			/* Execute test */
			this->execute_negative_test(tested_shader_type, shader_source);
		} /* if var_type iterator found */
		else
		{
			TCU_FAIL("Type not found.");
		}
	} /* for (int var_type_index = 0; ...) */
}

/* Generates the shader source code for the ExpressionsTypeRestrictions2
 * array tests, and attempts to compile each test shader, for both
 * vertex and fragment shaders.
 *
 * @tparam API               Tested API descriptor
 *
 * @param tested_shader_type The type of shader that is being tested
 *                           (either TestCaseBase<API>::VERTEX_SHADER_TYPE or TestCaseBase<API>::FRAGMENT_SHADER_TYPE).
 */
template <class API>
void ExpressionsTypeRestrictions2<API>::test_shader_compilation(
	typename TestCaseBase<API>::TestShaderType tested_shader_type)
{
	int num_var_types = sizeof(opaque_var_types) / sizeof(opaque_var_types[0]);

	for (int var_type_index = 0; var_type_index < num_var_types; var_type_index++)
	{
		_supported_variable_types_map_const_iterator var_iterator =
			supported_variable_types_map.find(opaque_var_types[var_type_index]);

		if (var_iterator != supported_variable_types_map.end())
		{
			std::string shader_source =
				"uniform " + var_iterator->second.precision + " " + var_iterator->second.type + " sampler1;\n"
																								"uniform " +
				var_iterator->second.precision + " " + var_iterator->second.type + " sampler2;\n"
																				   "uniform " +
				var_iterator->second.precision + " " + var_iterator->second.type + " sampler3;\n"
																				   "uniform " +
				var_iterator->second.precision + " " + var_iterator->second.type + " sampler4;\n"
																				   "struct light1 {\n"
																				   "    " +
				var_iterator->second.type + " var1[2][2];\n"
											"};\n\n";
			shader_source += shader_start;

			shader_source +=
				("    light1 x = light1(" + var_iterator->second.type + "[][](" + var_iterator->second.type +
				 "[](sampler1, sampler2), " + var_iterator->second.type + "[](sampler3, sampler4)));\n");
			shader_source += "    light1 y = x;\n\n";

			/* End main */
			DEFAULT_MAIN_ENDING(tested_shader_type, shader_source);

			/* Execute test */
			this->execute_negative_test(tested_shader_type, shader_source);
		} /* if var_type iterator found */
		else
		{
			TCU_FAIL("Type not found.");
		}
	} /* for (int var_type_index = 0; ...) */
}

/* Generates the shader source code for the ExpressionsIndexingScalar1
 * array tests, and attempts to compile each test shader, for both
 * vertex and fragment shaders.
 *
 * @tparam API               Tested API descriptor
 *
 * @param tested_shader_type The type of shader that is being tested
 *                           (either TestCaseBase<API>::VERTEX_SHADER_TYPE or TestCaseBase<API>::FRAGMENT_SHADER_TYPE).
 */
template <class API>
void ExpressionsIndexingScalar1<API>::test_shader_compilation(
	typename TestCaseBase<API>::TestShaderType tested_shader_type)
{
	for (size_t var_type_index = 0; var_type_index < API::n_var_types; var_type_index++)
	{
		_supported_variable_types_map_const_iterator var_iterator =
			supported_variable_types_map.find(API::var_types[var_type_index]);

		if (var_iterator != supported_variable_types_map.end())
		{
			std::string shader_source = shader_start + "    " + var_iterator->second.type + " x[1][2][3][4];\n\n";

			shader_source += "    for (uint i = 0u; i < 2u; i++) {\n";
			shader_source += "        for (uint j = 0u; j < 3u; j++) {\n";
			shader_source += "            for (uint k = 0u; k < 4u; k++) {\n";
			shader_source += "                x[0][i][j][k] = " + var_iterator->second.initializer_with_ones + ";\n";
			shader_source += "            }\n";
			shader_source += "        }\n";
			shader_source += "    }\n";

			/* End main */
			DEFAULT_MAIN_ENDING(tested_shader_type, shader_source);

			/* Execute test */
			EXECUTE_POSITIVE_TEST(tested_shader_type, shader_source, true, false);
		} /* if var_type iterator found */
		else
		{
			TCU_FAIL("Type not found.");
		}
	}
}

/* Generates the shader source code for the ExpressionsIndexingScalar2
 * array tests, and attempts to compile each test shader, for both
 * vertex and fragment shaders.
 *
 * @tparam API               Tested API descriptor
 *
 * @param tested_shader_type The type of shader that is being tested
 *                           (either TestCaseBase<API>::VERTEX_SHADER_TYPE or TestCaseBase<API>::FRAGMENT_SHADER_TYPE).
 */
template <class API>
void ExpressionsIndexingScalar2<API>::test_shader_compilation(
	typename TestCaseBase<API>::TestShaderType tested_shader_type)
{
	std::string base_shader_string, shader_source;

	// This test tests arrays with 4 dimensions, e.g. x[1][1][1][1]
	const int test_array_dimensions = 4;

	base_shader_string = "float a[1][2][3][4];\n";
	base_shader_string += "float b = 2.0;\n\n";
	base_shader_string += shader_start;

	// There are 16 permutations, so loop 4x4 times.
	for (int permutation = 0; permutation < (1 << test_array_dimensions); permutation++)
	{
		shader_source = base_shader_string + "    a"; // a var called 'a'

		for (int sub_script_index = test_array_dimensions - 1; sub_script_index >= 0; sub_script_index--)
		{
			/* If any bit is set for a particular number then add
			 * a valid array sub_script at that place, otherwise
			 * add an invalid array sub_script. */
			if (permutation & (1 << sub_script_index))
			{
				shader_source += "[0]";
			}
			else
			{
				shader_source += "[-1]";
			}
		}

		shader_source += " = b;\n";

		if (permutation != (1 << test_array_dimensions) - 1)
		{
			/* End main */
			DEFAULT_MAIN_ENDING(tested_shader_type, shader_source);

			/* Execute test */
			this->execute_negative_test(tested_shader_type, shader_source);
		} /* if (permutation != (1 << test_array_dimensions) - 1) */
	}	 /* for (int permutation = 0; ...) */
}

/* Generates the shader source code for the ExpressionsIndexingScalar3
 * array tests, and attempts to compile each test shader, for both
 * vertex and fragment shaders.
 *
 * @tparam API               Tested API descriptor
 *
 * @param tested_shader_type The type of shader that is being tested
 *                           (either TestCaseBase<API>::VERTEX_SHADER_TYPE or TestCaseBase<API>::FRAGMENT_SHADER_TYPE).
 */
template <class API>
void ExpressionsIndexingScalar3<API>::test_shader_compilation(
	typename TestCaseBase<API>::TestShaderType tested_shader_type)
{
	std::string base_shader_string;
	std::string shader_source;
	const int   test_array_dimensions = 4;

	base_shader_string = "float a[1][2][3][4];\n";
	base_shader_string += "float b = 2.0;\n\n";
	base_shader_string += shader_start;

	for (int permutation = 0; permutation < (1 << test_array_dimensions); permutation++)
	{
		shader_source = base_shader_string + "    a";

		for (int sub_script_index = test_array_dimensions - 1; sub_script_index >= 0; sub_script_index--)
		{
			if (permutation & (1 << sub_script_index))
			{
				shader_source += "[0]";
			}
			else
			{
				shader_source += "[4]";
			}
		}

		shader_source += " = b;\n";

		if (permutation != (1 << test_array_dimensions) - 1)
		{
			/* End main */
			DEFAULT_MAIN_ENDING(tested_shader_type, shader_source);

			/* Execute test */
			this->execute_negative_test(tested_shader_type, shader_source);
		} /* if (permutation != (1 << test_array_dimensions) - 1) */
	}	 /* for (int permutation = 0; ...) */
}

/* Generates the shader source code for the ExpressionsIndexingScalar4
 * array tests, and attempts to compile each test shader, for both
 * vertex and fragment shaders.
 *
 * @tparam API               Tested API descriptor
 *
 * @param tested_shader_type The type of shader that is being tested
 *                           (either TestCaseBase<API>::VERTEX_SHADER_TYPE or TestCaseBase<API>::FRAGMENT_SHADER_TYPE).
 */
template <class API>
void ExpressionsIndexingScalar4<API>::test_shader_compilation(
	typename TestCaseBase<API>::TestShaderType tested_shader_type)
{
	std::string base_shader_string;
	std::string shader_source;

	const int test_array_dimensions = 4;

	base_shader_string = "float a[1][2][3][4];\n";
	base_shader_string += "float b = 2.0;\n\n";
	base_shader_string += shader_start;

	for (int permutation = 0; permutation < (1 << test_array_dimensions); permutation++)
	{
		shader_source = base_shader_string + "    a";

		for (int sub_script_index = test_array_dimensions - 1; sub_script_index >= 0; sub_script_index--)
		{
			if (permutation & (1 << sub_script_index))
			{
				shader_source += "[0]";
			}
			else
			{
				shader_source += "[]";
			}
		}

		shader_source += " = b;\n";

		if (permutation != (1 << test_array_dimensions) - 1)
		{
			/* End main */
			DEFAULT_MAIN_ENDING(tested_shader_type, shader_source);

			/* Execute test */
			this->execute_negative_test(tested_shader_type, shader_source);
		} /* if (permutation != (1 << test_array_dimensions) - 1) */
	}	 /* for (int permutation = 0; ...) */
}

/* Generates the shader source code for the ExpressionsIndexingArray1
 * array tests, and attempts to compile each test shader, for both
 * vertex and fragment shaders.
 *
 * @tparam API               Tested API descriptor
 *
 * @param tested_shader_type The type of shader that is being tested
 *                           (either TestCaseBase<API>::VERTEX_SHADER_TYPE or TestCaseBase<API>::FRAGMENT_SHADER_TYPE).
 */
template <class API>
void ExpressionsIndexingArray1<API>::test_shader_compilation(
	typename TestCaseBase<API>::TestShaderType tested_shader_type)
{
	std::string shader_source;
	std::string variable_declaration = "float x[1][1][1][1][1][1][1][1];\n\n";

	std::string variable_initializations[] = {
		"x[0]                      = float[1][1][1][1][1][1][1]("
		"float[1][1][1][1][1][1](float[1][1][1][1][1](float[1][1][1][1](float[1][1][1](float[1][1](float[1](1.0)))))));"
		"\n",
		"x[0][0]                   = "
		"float[1][1][1][1][1][1](float[1][1][1][1][1](float[1][1][1][1](float[1][1][1](float[1][1](float[1](1.0))))));"
		"\n",
		"x[0][0][0]                = "
		"float[1][1][1][1][1](float[1][1][1][1](float[1][1][1](float[1][1](float[1](1.0)))));\n",
		"x[0][0][0][0]             = float[1][1][1][1](float[1][1][1](float[1][1](float[1](1.0))));\n",
		"x[0][0][0][0][0]          = float[1][1][1](float[1][1](float[1](1.0)));\n",
		"x[0][0][0][0][0][0]       = float[1][1](float[1](1.0));\n", "x[0][0][0][0][0][0][0]    = float[1](1.0);\n",
		"x[0][0][0][0][0][0][0][0] = 1.0;\n"
	};

	for (size_t string_index = 0; string_index < sizeof(variable_initializations) / sizeof(variable_initializations[0]);
		 string_index++)
	{
		shader_source = variable_declaration + shader_start;
		shader_source += "    " + variable_initializations[string_index];

		/* End main */
		DEFAULT_MAIN_ENDING(tested_shader_type, shader_source);

		/* Execute test */
		EXECUTE_POSITIVE_TEST(tested_shader_type, shader_source, true, false);
	} /* for (int string_index = 0; ...) */
}

/* Constructs a suitable constructor for the specified number of dimensions.
 *
 * @tparam API            Tested API descriptor
 *
 * @param dimension_index The current recursion level (counts down)
 * @param init_string     The initialisation string
 */
template <class API>
std::string ExpressionsIndexingArray2<API>::recursively_initialise(std::string var_type, size_t dimension_index,
																   std::string init_string)
{
	std::string temp_string;

	if (dimension_index == 0)
	{
		temp_string = init_string;
	}
	else
	{
		std::string prefix = "\n";

		for (size_t indent_index = dimension_index; indent_index < (API::MAX_ARRAY_DIMENSIONS + 1); indent_index++)
		{
			prefix += "    ";
		}

		prefix += this->extend_string(var_type, "[]", dimension_index);
		prefix += "(";
		for (int sub_script_index = 0; sub_script_index < 2; sub_script_index++)
		{
			temp_string += prefix;
			temp_string += recursively_initialise(var_type, dimension_index - 1, init_string);
			prefix = ", ";
			if (dimension_index == 1)
			{
				break;
			}
		}
		temp_string += ")";
	}

	return temp_string;
}

/* Generates the shader source code for the ExpressionsIndexingArray2
 * array tests, and attempts to compile each test shader, for both
 * vertex and fragment shaders.
 *
 * @tparam API               Tested API descriptor
 *
 * @param tested_shader_type The type of shader that is being tested
 *                           (either TestCaseBase<API>::VERTEX_SHADER_TYPE or TestCaseBase<API>::FRAGMENT_SHADER_TYPE).
 */
template <class API>
void ExpressionsIndexingArray2<API>::test_shader_compilation(
	typename TestCaseBase<API>::TestShaderType tested_shader_type)
{
	std::string variable_initialiser = "    float[](float[](float(float(float(float(float(float(1.0))))))));\n";

	std::string x_variable_initializaton =
		"    float x[2][2][2][2][2][2][2][1] = " + recursively_initialise("float", API::MAX_ARRAY_DIMENSIONS, "1.0") +
		";\n";
	std::string y_variable_initializaton =
		"    float y[2][2][2][2][2][2][2][1] = " + recursively_initialise("float", API::MAX_ARRAY_DIMENSIONS, "1.0") +
		";\n";

	std::string shader_code_common_part = shader_start + x_variable_initializaton + y_variable_initializaton;

	for (size_t max_dimension_index = 1; max_dimension_index <= API::MAX_ARRAY_DIMENSIONS; max_dimension_index++)
	{
		std::string iteration_specific_shader_code_part;

		iteration_specific_shader_code_part += this->extend_string("    x", "[0]", max_dimension_index);
		iteration_specific_shader_code_part += " = ";
		iteration_specific_shader_code_part += this->extend_string("y", "[0]", max_dimension_index);
		iteration_specific_shader_code_part += ";\n";

		std::string shader_source = shader_code_common_part + iteration_specific_shader_code_part;

		/* End main */
		DEFAULT_MAIN_ENDING(tested_shader_type, shader_source);

		/* Execute test */
		EXECUTE_POSITIVE_TEST(tested_shader_type, shader_source, true, false);
	}
}

/* Generates the shader source code for the ExpressionsIndexingArray3
 * array tests, and attempts to compile each test shader, for both
 * vertex and fragment shaders.
 *
 * @tparam API               Tested API descriptor
 *
 * @param tested_shader_type The type of shader that is being tested
 *                           (either TestCaseBase<API>::VERTEX_SHADER_TYPE or TestCaseBase<API>::FRAGMENT_SHADER_TYPE).
 */
template <class API>
void ExpressionsIndexingArray3<API>::test_shader_compilation(
	typename TestCaseBase<API>::TestShaderType tested_shader_type)
{
	std::string input[] = { "    x[ivec2(0)] = 1.0;\n\n", "    x[ivec3(0)] = 1.0;\n\n", "    x[ivec4(0)] = 1.0;\n\n" };

	for (size_t string_index = 0; string_index < sizeof(input) / sizeof(input[0]); string_index++)
	{
		std::string shader_source = shader_start + this->extend_string("    float x", "[2]", (int)string_index + 2) +
									";\n\n" + input[string_index];

		/* End main */
		DEFAULT_MAIN_ENDING(tested_shader_type, shader_source);

		/* Execute test */
		this->execute_negative_test(tested_shader_type, shader_source);
	} /* for (int string_index = 0; ...) */
}

/* Generates the shader source code for the ExpressionsDynamicIndexing1
 * array tests, and attempts to compile each test shader, for both
 * vertex and fragment shaders.
 *
 * @tparam API               Tested API descriptor
 *
 * @param tested_shader_type The type of shader that is being tested
 *                           (either TestCaseBase<API>::VERTEX_SHADER_TYPE or TestCaseBase<API>::FRAGMENT_SHADER_TYPE).
 */
template <class API>
void ExpressionsDynamicIndexing1<API>::test_shader_compilation(
	typename TestCaseBase<API>::TestShaderType tested_shader_type)
{
	std::string expression_type_declarations = "uniform int a;\n"
											   "const int b = 0;\n"
											   "int c = 0;\n"
											   "float x[2][2];\n";

	std::string expressions[] = { "a", "b", "c", "0 + 1" };
	std::string shader_source;

	for (size_t write_index = 0; write_index < sizeof(expressions) / sizeof(expressions[0]); write_index++)
	{
		for (size_t read_index = 0; read_index < sizeof(expressions) / sizeof(expressions[0]); read_index++)
		{
			shader_source = expression_type_declarations;
			shader_source += shader_start;
			shader_source += "    x[";
			shader_source += expressions[write_index];
			shader_source += "][";
			shader_source += expressions[read_index];
			shader_source += "] = 1.0;\n\n";

			/* End main */
			DEFAULT_MAIN_ENDING(tested_shader_type, shader_source);

			/* Execute test */
			EXECUTE_POSITIVE_TEST(tested_shader_type, shader_source, true, false);
		} /* for (int read_index = 0; ...) */
	}	 /* for (int write_index = 0; ...) */
}

/* Generates the shader source code for the ExpressionsDynamicIndexing2
 * array tests, and attempts to compile each test shader, for both
 * vertex and fragment shaders.
 *
 * @tparam API               Tested API descriptor
 *
 * @param tested_shader_type The type of shader that is being tested
 *                           (either TestCaseBase<API>::VERTEX_SHADER_TYPE or TestCaseBase<API>::FRAGMENT_SHADER_TYPE).
 */
template <class API>
void ExpressionsDynamicIndexing2<API>::test_shader_compilation(
	typename TestCaseBase<API>::TestShaderType tested_shader_type)
{
	int				  num_var_types				  = sizeof(opaque_var_types) / sizeof(opaque_var_types[0]);
	const std::string invalid_size_declarations[] = { "[0][0][0][y]", "[0][0][y][0]", "[0][y][0][0]", "[y][0][0][0]",
													  "[0][0][y][y]", "[0][y][0][y]", "[y][0][0][y]", "[0][y][y][0]",
													  "[y][0][y][0]", "[y][y][0][0]", "[0][y][y][y]", "[y][0][y][y]",
													  "[y][y][0][y]", "[y][y][y][0]", "[y][y][y][y]" };

	bool dynamic_indexing_supported = false;
	if (glu::contextSupports(this->context_id.getRenderContext().getType(), glu::ApiType::es(3, 2)) ||
		glu::contextSupports(this->context_id.getRenderContext().getType(), glu::ApiType::core(4, 0)) ||
		this->context_id.getContextInfo().isExtensionSupported("GL_EXT_gpu_shader5"))
	{
		dynamic_indexing_supported = true;
	}

	for (int var_type_index = 0; var_type_index < num_var_types; var_type_index++)
	{
		_supported_variable_types_map_const_iterator var_iterator =
			supported_variable_types_map.find(opaque_var_types[var_type_index]);

		if (var_iterator != supported_variable_types_map.end())
		{
			int num_invalid_size_declarations =
				sizeof(invalid_size_declarations) / sizeof(invalid_size_declarations[0]);

			for (int invalid_size_index = 0; invalid_size_index < num_invalid_size_declarations; invalid_size_index++)
			{
				std::string shader_source = "int y = 1;\n";

				shader_source += "uniform " + var_iterator->second.precision + " " + var_iterator->second.type +
								 " x[2][2][2][2];\n\n";
				shader_source += "void main()\n";
				shader_source += "{\n";
				shader_source += ("    " + var_iterator->second.type_of_result_of_texture_function +
								  " color = texture(x" + invalid_size_declarations[invalid_size_index] + ", " +
								  var_iterator->second.coord_param_for_texture_function + ");\n");

				/* End main */
				DEFAULT_MAIN_ENDING(tested_shader_type, shader_source);

				if (dynamic_indexing_supported)
				{
					EXECUTE_POSITIVE_TEST(tested_shader_type, shader_source, true, true);
				}
				else
				{
					this->execute_negative_test(tested_shader_type, shader_source);
				}
			} /* for (int invalid_size_index = 0; ...) */
		}	 /* if var_type iterator found */
		else
		{
			TCU_FAIL("Type not found.");
		}
	} /* for (int var_type_index = 0; ...) */
}

/* Generates the shader source code for the ExpressionsEquality1
 * array tests, and attempts to compile each test shader, for both
 * vertex and fragment shaders.
 *
 * @tparam API               Tested API descriptor
 *
 * @param tested_shader_type The type of shader that is being tested
 *                           (either TestCaseBase<API>::VERTEX_SHADER_TYPE or TestCaseBase<API>::FRAGMENT_SHADER_TYPE).
 */
template <class API>
void ExpressionsEquality1<API>::test_shader_compilation(typename TestCaseBase<API>::TestShaderType tested_shader_type)
{
	int num_var_types = API::n_var_types;

	for (int var_type_index = 0; var_type_index < num_var_types; var_type_index++)
	{
		_supported_variable_types_map_const_iterator var_iterator =
			supported_variable_types_map.find(API::var_types[var_type_index]);

		if (var_iterator != supported_variable_types_map.end())
		{
			std::string shader_source = shader_start;

			shader_source += "    ";
			shader_source += var_iterator->second.type;
			shader_source += "[][] x = ";
			shader_source += var_iterator->second.type;
			shader_source += "[][](";
			shader_source += var_iterator->second.type;
			shader_source += "[](";
			shader_source += var_iterator->second.initializer_with_zeroes;
			shader_source += ",";
			shader_source += var_iterator->second.initializer_with_zeroes;
			shader_source += "),";
			shader_source += var_iterator->second.type;
			shader_source += "[](";
			shader_source += var_iterator->second.initializer_with_zeroes;
			shader_source += ",";
			shader_source += var_iterator->second.initializer_with_zeroes;
			shader_source += "));\n";
			shader_source += "    ";
			shader_source += var_iterator->second.type;
			shader_source += "[][] y = ";
			shader_source += var_iterator->second.type;
			shader_source += "[][](";
			shader_source += var_iterator->second.type;
			shader_source += "[](";
			shader_source += var_iterator->second.initializer_with_zeroes;
			shader_source += ",";
			shader_source += var_iterator->second.initializer_with_zeroes;
			shader_source += "),";
			shader_source += var_iterator->second.type;
			shader_source += "[](";
			shader_source += var_iterator->second.initializer_with_zeroes;
			shader_source += ",";
			shader_source += var_iterator->second.initializer_with_zeroes;
			shader_source += "));\n\n";
			shader_source += "    float result = 0.0;\n\n";
			shader_source += "    if (x == y)\n";
			shader_source += "    {\n";
			shader_source += "        result = 1.0;\n";
			shader_source += "    }\n";
			shader_source += "    if (y != x)\n";
			shader_source += "    {\n";
			shader_source += "        result = 2.0;\n";
			shader_source += "    }\n";

			/* End main */
			DEFAULT_MAIN_ENDING(tested_shader_type, shader_source);

			/* Execute test */
			EXECUTE_POSITIVE_TEST(tested_shader_type, shader_source, true, false);
		} /* if var_type iterator found */
		else
		{
			TCU_FAIL("Type not found.");
		}
	}
}

/* Generates the shader source code for the ExpressionsEquality2
 * array tests, and attempts to compile each test shader, for both
 * vertex and fragment shaders.
 *
 * @tparam API               Tested API descriptor
 *
 * @param tested_shader_type The type of shader that is being tested
 *                           (either TestCaseBase<API>::VERTEX_SHADER_TYPE or TestCaseBase<API>::FRAGMENT_SHADER_TYPE).
 */
template <class API>
void ExpressionsEquality2<API>::test_shader_compilation(typename TestCaseBase<API>::TestShaderType tested_shader_type)
{
	int num_var_types = API::n_var_types;

	for (int var_type_index = 0; var_type_index < num_var_types; var_type_index++)
	{
		_supported_variable_types_map_const_iterator var_iterator =
			supported_variable_types_map.find(API::var_types[var_type_index]);

		if (var_iterator != supported_variable_types_map.end())
		{
			std::string shader_source = "struct light {\n    float intensity;\n    int position;\n};\n\n";

			shader_source += shader_start;
			shader_source += "    light[][] x =";
			shader_source += "light";
			shader_source += "[][](";
			shader_source += "light";
			shader_source += "[](light(1.0, 1)),";
			shader_source += "light";
			shader_source += "[](light(2.0, 2)));\n\n";
			shader_source += "    light[][] y =";
			shader_source += "light";
			shader_source += "[][](";
			shader_source += "light";
			shader_source += "[](light(3.0, 3)),";
			shader_source += "light";
			shader_source += "[](light(4.0, 4)));\n\n";
			shader_source += "    float result = 0.0;\n\n";
			shader_source += "    if (x == y)\n";
			shader_source += "    {\n";
			shader_source += "        result = 1.0;\n";
			shader_source += "    }\n";
			shader_source += "    if (y != x)\n";
			shader_source += "    {\n";
			shader_source += "        result = 2.0;\n";
			shader_source += "    }\n";

			/* Apply stage specific stuff */
			switch (tested_shader_type)
			{
			case TestCaseBase<API>::VERTEX_SHADER_TYPE:
				shader_source += "\n    gl_Position = vec4(0.0,0.0,0.0,1.0);\n";
				break;
			case TestCaseBase<API>::FRAGMENT_SHADER_TYPE:
				shader_source += "\n    gl_FragDepth = result;\n";
				break;
			case TestCaseBase<API>::COMPUTE_SHADER_TYPE:
				break;
			case TestCaseBase<API>::GEOMETRY_SHADER_TYPE:
				shader_source += emit_quad;
				break;
			case TestCaseBase<API>::TESSELATION_CONTROL_SHADER_TYPE:
				shader_source += set_tesseation;
				break;
			case TestCaseBase<API>::TESSELATION_EVALUATION_SHADER_TYPE:
				break;
			default:
				TCU_FAIL("Unrecognized shader type.");
				break;
			}

			/* End main function */
			shader_source += shader_end;

			/* Execute test */
			EXECUTE_POSITIVE_TEST(tested_shader_type, shader_source, true, false);
		} /* if var_type iterator found */
		else
		{
			TCU_FAIL("Type not found.");
		}
	}
}

/* Generates the shader source code for the ExpressionsLength1
 * array tests, and attempts to compile each test shader, for both
 * vertex and fragment shaders.
 *
 * @tparam API               Tested API descriptor
 *
 * @param tested_shader_type The type of shader that is being tested
 *                           (either TestCaseBase<API>::VERTEX_SHADER_TYPE or TestCaseBase<API>::FRAGMENT_SHADER_TYPE).
 */
template <class API>
void ExpressionsLength1<API>::test_shader_compilation(typename TestCaseBase<API>::TestShaderType tested_shader_type)
{
	std::string array_declaration	  = "    int x[4][3][2][1];\n\n";
	std::string case_specific_string[] = { "    if (x.length() != 4) {\n"
										   "        result = 0.0f;\n    }\n",
										   "    if (x[0].length() != 3) {\n"
										   "        result = 0.0f;\n    }\n",
										   "    if (x[0][0].length() != 2) {\n"
										   "        result = 0.0f;\n    }\n",
										   "    if (x[0][0][0].length() != 1) {\n"
										   "        result = 0.0f;\n    }\n" };
	const bool test_compute = (TestCaseBase<API>::COMPUTE_SHADER_TYPE == tested_shader_type);

	for (size_t case_specific_string_index = 0;
		 case_specific_string_index < sizeof(case_specific_string) / sizeof(case_specific_string[0]);
		 case_specific_string_index++)
	{
		const std::string& test_snippet = case_specific_string[case_specific_string_index];

		if (false == test_compute)
		{
			execute_draw_test(tested_shader_type, array_declaration, test_snippet);
		}
		else
		{
			execute_dispatch_test(tested_shader_type, array_declaration, test_snippet);
		}

		/* Deallocate any resources used. */
		this->delete_objects();
	} /* for (int case_specific_string_index = 0; ...) */
}

/** Executes test for compute program
 *
 * @tparam API               Tested API descriptor
 *
 * @param tested_shader_type The type of shader that is being tested
 * @param tested_declaration Declaration used to prepare shader
 * @param tested_snippet     Snippet used to prepare shader
 **/
template <class API>
void ExpressionsLength1<API>::execute_dispatch_test(typename TestCaseBase<API>::TestShaderType tested_shader_type,
													const std::string&						   tested_declaration,
													const std::string&						   tested_snippet)
{
	const std::string& compute_shader_source =
		prepare_compute_shader(tested_shader_type, tested_declaration, tested_snippet);
	const glw::Functions& gl = this->context_id.getRenderContext().getFunctions();

	this->execute_positive_test(empty_string, empty_string, empty_string, empty_string, empty_string,
								compute_shader_source, false, false);

	/* We are now ready to verify whether the returned size is correct. */
	unsigned char buffer[4]				= { 0 };
	glw::GLuint   framebuffer_object_id = 0;
	glw::GLint	location				= -1;
	glw::GLuint   texture_object_id		= 0;
	glw::GLuint   vao_id				= 0;

	gl.useProgram(this->program_object_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram() failed.");

	gl.genTextures(1, &texture_object_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenTextures() failed.");

	gl.bindTexture(GL_TEXTURE_2D, texture_object_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture() failed.");

	gl.texStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, 1, 1);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTexStorage2D() failed.");

	gl.bindImageTexture(0 /* image unit */, texture_object_id, 0 /* level */, GL_FALSE /* layered */, 0 /* layer */,
						GL_WRITE_ONLY, GL_RGBA8);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindImageTexture() failed.");

	location = gl.getUniformLocation(this->program_object_id, "uni_image");
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetUniformLocation() failed.");

	if (-1 == location)
	{
		TCU_FAIL("Uniform is inactive");
	}

	gl.uniform1i(location, 0 /* image unit */);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUniform1i() failed.");

	gl.genVertexArrays(1, &vao_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenVertexArrays() failed.");

	gl.bindVertexArray(vao_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindVertexArray() failed.");

	gl.dispatchCompute(1, 1, 1);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawArrays() failed.");

	gl.genFramebuffers(1, &framebuffer_object_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenFramebuffers() failed.");

	gl.bindFramebuffer(GL_FRAMEBUFFER, framebuffer_object_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffer() failed.");

	gl.framebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture_object_id, 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glFramebufferTexture2D() failed.");

	gl.viewport(0, 0, 1, 1);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glViewport() failed.");

	gl.readBuffer(GL_COLOR_ATTACHMENT0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glReadBuffer() failed.");

	gl.readPixels(0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, buffer);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glReadPixels() failed.");

	if (buffer[0] != 255)
	{
		TCU_FAIL("Invalid array size was returned.");
	}

	/* Delete generated objects. */
	gl.deleteTextures(1, &texture_object_id);
	gl.deleteFramebuffers(1, &framebuffer_object_id);
	gl.deleteVertexArrays(1, &vao_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "An error ocurred while deleting generated objects.");
}

/** Executes test for draw program
 *
 * @tparam API               Tested API descriptor
 *
 * @param tested_shader_type The type of shader that is being tested
 * @param tested_declaration Declaration used to prepare shader
 * @param tested_snippet     Snippet used to prepare shader
 **/
template <class API>
void ExpressionsLength1<API>::execute_draw_test(typename TestCaseBase<API>::TestShaderType tested_shader_type,
												const std::string&						   tested_declaration,
												const std::string&						   tested_snippet)
{
	const glw::Functions& gl = this->context_id.getRenderContext().getFunctions();

	if (API::USE_ALL_SHADER_STAGES)
	{
		const std::string& compute_shader_source = empty_string;
		const std::string& fragment_shader_source =
			this->prepare_fragment_shader(tested_shader_type, tested_declaration, tested_snippet);
		const std::string& geometry_shader_source =
			this->prepare_geometry_shader(tested_shader_type, tested_declaration, tested_snippet);
		const std::string& tess_ctrl_shader_source =
			this->prepare_tess_ctrl_shader(tested_shader_type, tested_declaration, tested_snippet);
		const std::string& tess_eval_shader_source =
			this->prepare_tess_eval_shader(tested_shader_type, tested_declaration, tested_snippet);
		const std::string& vertex_shader_source =
			this->prepare_vertex_shader(tested_shader_type, tested_declaration, tested_snippet);

		switch (tested_shader_type)
		{
		case TestCaseBase<API>::VERTEX_SHADER_TYPE: /* Fall through */
		case TestCaseBase<API>::FRAGMENT_SHADER_TYPE:
			this->execute_positive_test(vertex_shader_source, fragment_shader_source, false, false);
			break;

		case TestCaseBase<API>::COMPUTE_SHADER_TYPE:
		case TestCaseBase<API>::GEOMETRY_SHADER_TYPE:
		case TestCaseBase<API>::TESSELATION_CONTROL_SHADER_TYPE: /* Fall through */
		case TestCaseBase<API>::TESSELATION_EVALUATION_SHADER_TYPE:
			this->execute_positive_test(vertex_shader_source, tess_ctrl_shader_source, tess_eval_shader_source,
										geometry_shader_source, fragment_shader_source, compute_shader_source, false,
										false);
			break;

		default:
			TCU_FAIL("Invalid enum");
			break;
		}
	}
	else
	{
		const std::string& fragment_shader_source =
			this->prepare_fragment_shader(tested_shader_type, tested_declaration, tested_snippet);
		const std::string& vertex_shader_source =
			this->prepare_vertex_shader(tested_shader_type, tested_declaration, tested_snippet);

		this->execute_positive_test(vertex_shader_source, fragment_shader_source, false, false);
	}

	/* We are now ready to verify whether the returned size is correct. */
	unsigned char buffer[4]				= { 0 };
	glw::GLuint   framebuffer_object_id = 0;
	glw::GLuint   texture_object_id		= 0;
	glw::GLuint   vao_id				= 0;

	gl.useProgram(this->program_object_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram() failed.");

	gl.genTextures(1, &texture_object_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenTextures() failed.");

	gl.bindTexture(GL_TEXTURE_2D, texture_object_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture() failed.");

	gl.texStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, 1, 1);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTexStorage2D() failed.");

	gl.genFramebuffers(1, &framebuffer_object_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenFramebuffers() failed.");

	gl.bindFramebuffer(GL_FRAMEBUFFER, framebuffer_object_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffer() failed.");

	gl.framebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture_object_id, 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glFramebufferTexture2D() failed.");

	gl.viewport(0, 0, 1, 1);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glViewport() failed.");

	gl.genVertexArrays(1, &vao_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenVertexArrays() failed.");

	gl.bindVertexArray(vao_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindVertexArray() failed.");

	switch (tested_shader_type)
	{
	case TestCaseBase<API>::FRAGMENT_SHADER_TYPE: /* Fall through */
	case TestCaseBase<API>::VERTEX_SHADER_TYPE:
		gl.drawArrays(GL_TRIANGLE_FAN, 0, 4);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawArrays() failed.");
		break;

	case TestCaseBase<API>::TESSELATION_CONTROL_SHADER_TYPE: /* Fall through */
	case TestCaseBase<API>::TESSELATION_EVALUATION_SHADER_TYPE:
		/* Tesselation patch set up */
		gl.patchParameteri(GL_PATCH_VERTICES, 1);
		GLU_EXPECT_NO_ERROR(gl.getError(), "PatchParameteri");

		gl.drawArrays(GL_PATCHES, 0, 1);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawArrays() failed.");
		break;

	case TestCaseBase<API>::GEOMETRY_SHADER_TYPE:
		gl.drawArrays(GL_POINTS, 0, 1);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawArrays() failed.");
		break;

	default:
		TCU_FAIL("Invalid enum");
		break;
	}

	gl.readBuffer(GL_COLOR_ATTACHMENT0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glReadBuffer() failed.");

	gl.readPixels(0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, buffer);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glReadPixels() failed.");

	if (buffer[0] != 255)
	{
		TCU_FAIL("Invalid array size was returned.");
	}

	/* Delete generated objects. */
	gl.deleteTextures(1, &texture_object_id);
	gl.deleteFramebuffers(1, &framebuffer_object_id);
	gl.deleteVertexArrays(1, &vao_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "An error ocurred while deleting generated objects.");
}

/** Prepare shader
 *
 * @tparam API               Tested API descriptor
 *
 * @param tested_shader_type The type of shader that is being tested
 * @param tested_declaration Declaration used to prepare shader
 * @param tested_snippet     Snippet used to prepare shader
 **/
template <class API>
std::string ExpressionsLength1<API>::prepare_compute_shader(
	typename TestCaseBase<API>::TestShaderType tested_shader_type, const std::string& tested_declaration,
	const std::string& tested_snippet)
{
	std::string compute_shader_source;

	if (TestCaseBase<API>::COMPUTE_SHADER_TYPE == tested_shader_type)
	{
		compute_shader_source = "writeonly uniform image2D uni_image;\n"
								"\n"
								"void main()\n"
								"{\n"
								"    float result = 1u;\n"
								"\n";
		compute_shader_source += tested_declaration;
		compute_shader_source += tested_snippet;
		compute_shader_source += "\n"
								 "    imageStore(uni_image, ivec2(gl_GlobalInvocationID.xy), vec4(result, 0, 0, 0));\n"
								 "}\n"
								 "\n";
	}

	return compute_shader_source;
}

/** Prepare shader
 *
 * @tparam API               Tested API descriptor
 *
 * @param tested_shader_type The type of shader that is being tested
 * @param tested_declaration Declaration used to prepare shader
 * @param tested_snippet     Snippet used to prepare shader
 **/
template <class API>
std::string ExpressionsLength1<API>::prepare_fragment_shader(
	typename TestCaseBase<API>::TestShaderType tested_shader_type, const std::string& tested_declaration,
	const std::string& tested_snippet)
{
	std::string fragment_shader_source;

	switch (tested_shader_type)
	{
	case TestCaseBase<API>::COMPUTE_SHADER_TYPE:
		break;

	case TestCaseBase<API>::FRAGMENT_SHADER_TYPE:
		fragment_shader_source = "out vec4 colour;\n"
								 "\n"
								 "void main()\n"
								 "{\n";
		fragment_shader_source += tested_declaration;
		fragment_shader_source += "    float result = 1.0f;\n";
		fragment_shader_source += tested_snippet;
		fragment_shader_source += "    colour = vec4(result);\n"
								  "}\n\n";
		break;

	case TestCaseBase<API>::GEOMETRY_SHADER_TYPE:
	case TestCaseBase<API>::TESSELATION_CONTROL_SHADER_TYPE:
	case TestCaseBase<API>::TESSELATION_EVALUATION_SHADER_TYPE:
	case TestCaseBase<API>::VERTEX_SHADER_TYPE:
		fragment_shader_source = "in float fs_result;\n\n"
								 "out vec4 colour;\n\n"
								 "void main()\n"
								 "{\n"
								 "    colour =  vec4(fs_result);\n"
								 "}\n"
								 "\n";
		break;

	default:
		TCU_FAIL("Unrecognized shader object type.");
		break;
	}

	return fragment_shader_source;
}

/** Prepare shader
 *
 * @tparam API               Tested API descriptor
 *
 * @param tested_shader_type The type of shader that is being tested
 * @param tested_declaration Declaration used to prepare shader
 * @param tested_snippet     Snippet used to prepare shader
 **/
template <class API>
std::string ExpressionsLength1<API>::prepare_geometry_shader(
	typename TestCaseBase<API>::TestShaderType tested_shader_type, const std::string& tested_declaration,
	const std::string& tested_snippet)
{
	std::string geometry_shader_source;

	switch (tested_shader_type)
	{
	case TestCaseBase<API>::COMPUTE_SHADER_TYPE:
	case TestCaseBase<API>::FRAGMENT_SHADER_TYPE:
	case TestCaseBase<API>::VERTEX_SHADER_TYPE:
		break;

	case TestCaseBase<API>::TESSELATION_CONTROL_SHADER_TYPE:
	case TestCaseBase<API>::TESSELATION_EVALUATION_SHADER_TYPE:
		geometry_shader_source = "layout(points)                           in;\n"
								 "layout(triangle_strip, max_vertices = 4) out;\n"
								 "\n"
								 "in  float tes_result[];\n"
								 "out float fs_result;\n"
								 "\n"
								 "void main()\n"
								 "{\n"
								 "    gl_Position  = vec4(-1, -1, 0, 1);\n"
								 "    fs_result    = tes_result[0];\n"
								 "    EmitVertex();\n"
								 "    gl_Position  = vec4(-1, 1, 0, 1);\n"
								 "    fs_result    = tes_result[0];\n"
								 "    EmitVertex();\n"
								 "    gl_Position  = vec4(1, -1, 0, 1);\n"
								 "    fs_result    = tes_result[0];\n"
								 "    EmitVertex();\n"
								 "    gl_Position  = vec4(1, 1, 0, 1);\n"
								 "    fs_result    = tes_result[0];\n"
								 "    EmitVertex();\n"
								 "}\n";
		break;

	case TestCaseBase<API>::GEOMETRY_SHADER_TYPE:
		geometry_shader_source = "layout(points)                           in;\n"
								 "layout(triangle_strip, max_vertices = 4) out;\n"
								 "\n"
								 "out float fs_result;\n"
								 "\n"
								 "void main()\n"
								 "{\n";
		geometry_shader_source += tested_declaration;
		geometry_shader_source += "    float result = 1.0;\n\n";
		geometry_shader_source += tested_snippet;
		geometry_shader_source += "\n    gl_Position  = vec4(-1, -1, 0, 1);\n"
								  "    fs_result    = result;\n"
								  "    EmitVertex();\n"
								  "    gl_Position  = vec4(-1, 1, 0, 1);\n"
								  "    fs_result    = result;\n"
								  "    EmitVertex();\n"
								  "    gl_Position  = vec4(1, -1, 0, 1);\n"
								  "    fs_result    = result;\n"
								  "    EmitVertex();\n"
								  "    gl_Position  = vec4(1, 1, 0, 1);\n"
								  "    fs_result    = result;\n"
								  "    EmitVertex();\n"
								  "}\n";
		break;

	default:
		TCU_FAIL("Unrecognized shader object type.");
		break;
	}

	return geometry_shader_source;
}

/** Prepare shader
 *
 * @tparam API               Tested API descriptor
 *
 * @param tested_shader_type The type of shader that is being tested
 * @param tested_declaration Declaration used to prepare shader
 * @param tested_snippet     Snippet used to prepare shader
 **/
template <class API>
std::string ExpressionsLength1<API>::prepare_tess_ctrl_shader(
	typename TestCaseBase<API>::TestShaderType tested_shader_type, const std::string& tested_declaration,
	const std::string& tested_snippet)
{
	std::string tess_ctrl_shader_source;

	switch (tested_shader_type)
	{
	case TestCaseBase<API>::COMPUTE_SHADER_TYPE:
	case TestCaseBase<API>::FRAGMENT_SHADER_TYPE:
	case TestCaseBase<API>::GEOMETRY_SHADER_TYPE:
	case TestCaseBase<API>::VERTEX_SHADER_TYPE:
		break;

	case TestCaseBase<API>::TESSELATION_CONTROL_SHADER_TYPE:
		tess_ctrl_shader_source = "layout(vertices = 1) out;\n"
								  "\n"
								  "out float tcs_result[];\n"
								  "\n"
								  "void main()\n"
								  "{\n";
		tess_ctrl_shader_source += tested_declaration;
		tess_ctrl_shader_source += "    float result = 1.0;\n\n";
		tess_ctrl_shader_source += tested_snippet;
		tess_ctrl_shader_source += "    tcs_result[gl_InvocationID] = result;\n"
								   "\n"
								   "    gl_TessLevelOuter[0] = 1.0;\n"
								   "    gl_TessLevelOuter[1] = 1.0;\n"
								   "    gl_TessLevelOuter[2] = 1.0;\n"
								   "    gl_TessLevelOuter[3] = 1.0;\n"
								   "    gl_TessLevelInner[0] = 1.0;\n"
								   "    gl_TessLevelInner[1] = 1.0;\n"
								   "}\n";
		break;

	case TestCaseBase<API>::TESSELATION_EVALUATION_SHADER_TYPE:
		tess_ctrl_shader_source = default_tc_shader_source;
		break;

	default:
		TCU_FAIL("Unrecognized shader object type.");
		break;
	}

	return tess_ctrl_shader_source;
}

/** Prepare shader
 *
 * @tparam API               Tested API descriptor
 *
 * @param tested_shader_type The type of shader that is being tested
 * @param tested_declaration Declaration used to prepare shader
 * @param tested_snippet     Snippet used to prepare shader
 **/
template <class API>
std::string ExpressionsLength1<API>::prepare_tess_eval_shader(
	typename TestCaseBase<API>::TestShaderType tested_shader_type, const std::string& tested_declaration,
	const std::string& tested_snippet)
{
	std::string tess_eval_shader_source;

	switch (tested_shader_type)
	{
	case TestCaseBase<API>::COMPUTE_SHADER_TYPE:
	case TestCaseBase<API>::FRAGMENT_SHADER_TYPE:
	case TestCaseBase<API>::GEOMETRY_SHADER_TYPE:
	case TestCaseBase<API>::VERTEX_SHADER_TYPE:
		break;

	case TestCaseBase<API>::TESSELATION_CONTROL_SHADER_TYPE:
		tess_eval_shader_source = "layout(isolines, point_mode) in;\n"
								  "\n"
								  "in  float tcs_result[];\n"
								  "out float tes_result;\n"
								  "\n"
								  "void main()\n"
								  "{\n"
								  "    tes_result = tcs_result[0];\n"
								  "}\n";
		break;

	case TestCaseBase<API>::TESSELATION_EVALUATION_SHADER_TYPE:
		tess_eval_shader_source = "layout(isolines, point_mode) in;\n"
								  "\n"
								  "out float tes_result;\n"
								  "\n"
								  "void main()\n"
								  "{\n";
		tess_eval_shader_source += tested_declaration;
		tess_eval_shader_source += "    float result = 1.0;\n\n";
		tess_eval_shader_source += tested_snippet;
		tess_eval_shader_source += "    tes_result = result;\n"
								   "}\n";
		break;

	default:
		TCU_FAIL("Unrecognized shader object type.");
		break;
	}

	return tess_eval_shader_source;
}

/** Prepare shader
 *
 * @tparam API               Tested API descriptor
 *
 * @param tested_shader_type The type of shader that is being tested
 * @param tested_declaration Declaration used to prepare shader
 * @param tested_snippet     Snippet used to prepare shader
 **/
template <class API>
std::string ExpressionsLength1<API>::prepare_vertex_shader(
	typename TestCaseBase<API>::TestShaderType tested_shader_type, const std::string& tested_declaration,
	const std::string& tested_snippet)
{
	std::string vertex_shader_source;

	switch (tested_shader_type)
	{
	case TestCaseBase<API>::COMPUTE_SHADER_TYPE:
		break;

	case TestCaseBase<API>::FRAGMENT_SHADER_TYPE:
		vertex_shader_source = "/** GL_TRIANGLE_FAN-type quad vertex data. */\n"
							   "const vec4 vertex_positions[4] = vec4[4](vec4( 1.0, -1.0, 0.0, 1.0),\n"
							   "                                         vec4(-1.0, -1.0, 0.0, 1.0),\n"
							   "                                         vec4(-1.0,  1.0, 0.0, 1.0),\n"
							   "                                         vec4( 1.0,  1.0, 0.0, 1.0) );\n"
							   "\n"
							   "void main()\n"
							   "{\n"
							   "    gl_Position = vertex_positions[gl_VertexID];"
							   "}\n\n";
		break;

	case TestCaseBase<API>::GEOMETRY_SHADER_TYPE:
	case TestCaseBase<API>::TESSELATION_CONTROL_SHADER_TYPE:
	case TestCaseBase<API>::TESSELATION_EVALUATION_SHADER_TYPE:
		vertex_shader_source = default_vertex_shader_source;
		break;

	case TestCaseBase<API>::VERTEX_SHADER_TYPE:
		vertex_shader_source = "out float fs_result;\n"
							   "\n"
							   "/** GL_TRIANGLE_FAN-type quad vertex data. */\n"
							   "const vec4 vertex_positions[4] = vec4[4](vec4( 1.0, -1.0, 0.0, 1.0),\n"
							   "                                         vec4(-1.0, -1.0, 0.0, 1.0),\n"
							   "                                         vec4(-1.0,  1.0, 0.0, 1.0),\n"
							   "                                         vec4( 1.0,  1.0, 0.0, 1.0) );\n"
							   "\n"
							   "void main()\n"
							   "{\n";
		vertex_shader_source += tested_declaration;
		vertex_shader_source += "    float result = 1.0;\n\n";
		vertex_shader_source += tested_snippet;
		vertex_shader_source += "    gl_Position = vertex_positions[gl_VertexID];\n"
								"    fs_result = result;\n";
		vertex_shader_source += shader_end;
		break;

	default:
		TCU_FAIL("Unrecognized shader object type.");
		break;
	}

	return vertex_shader_source;
}

/* Generates the shader source code for the ExpressionsLength2
 * array tests, and attempts to compile each test shader, for both
 * vertex and fragment shaders.
 *
 * @tparam API               Tested API descriptor
 *
 * @param tested_shader_type The type of shader that is being tested
 *                           (either TestCaseBase<API>::VERTEX_SHADER_TYPE or TestCaseBase<API>::FRAGMENT_SHADER_TYPE).
 */
template <class API>
void ExpressionsLength2<API>::test_shader_compilation(typename TestCaseBase<API>::TestShaderType tested_shader_type)
{
	std::string array_declaration	  = "    int x[1][2][3][4];\n\n";
	std::string case_specific_string[] = { "    if (x.length() != 1) {\n"
										   "        result = 0.0f;\n    }\n",
										   "    if (x[0].length() != 2) {\n"
										   "        result = 0.0f;\n    }\n",
										   "    if (x[0][0].length() != 3) {\n"
										   "        result = 0.0f;\n    }\n",
										   "    if (x[0][0][0].length() != 4) {\n"
										   "        result = 0.0f;\n    }\n" };
	const bool test_compute = (TestCaseBase<API>::COMPUTE_SHADER_TYPE == tested_shader_type);

	for (size_t case_specific_string_index = 0;
		 case_specific_string_index < sizeof(case_specific_string) / sizeof(case_specific_string[0]);
		 case_specific_string_index++)
	{
		const std::string& test_snippet = case_specific_string[case_specific_string_index];

		if (false == test_compute)
		{
			this->execute_draw_test(tested_shader_type, array_declaration, test_snippet);
		}
		else
		{
			this->execute_dispatch_test(tested_shader_type, array_declaration, test_snippet);
		}

		/* Deallocate any resources used. */
		this->delete_objects();
	} /* for (int case_specific_string_index = 0; ...) */
}

/* Generates the shader source code for the ExpressionsLength3
 * array tests, and attempts to compile each test shader, for both
 * vertex and fragment shaders.
 *
 * @tparam API               Tested API descriptor
 *
 * @param tested_shader_type The type of shader that is being tested
 *                           (either TestCaseBase<API>::VERTEX_SHADER_TYPE or TestCaseBase<API>::FRAGMENT_SHADER_TYPE).
 */
template <class API>
void ExpressionsLength3<API>::test_shader_compilation(typename TestCaseBase<API>::TestShaderType tested_shader_type)
{
	std::string array_declaration = "    int x[1][1][1][1];\n\n";
	std::string input[]			  = { "    if (x[].length() != 2) {\n"
							"        result = 0.0f;\n    }\n",
							"    if (x[][].length() != 2)  {\n"
							"        result = 0.0f;\n    }\n",
							"    if (x[][][].length() != 2)  {\n"
							"        result = 0.0f;\n    }\n" };

	for (size_t string_index = 0; string_index < sizeof(input) / sizeof(input[0]); string_index++)
	{
		std::string		   shader_source;
		const std::string& test_snippet = input[string_index];

		switch (tested_shader_type)
		{
		case TestCaseBase<API>::VERTEX_SHADER_TYPE:
			shader_source = this->prepare_vertex_shader(tested_shader_type, array_declaration, test_snippet);
			break;

		case TestCaseBase<API>::FRAGMENT_SHADER_TYPE:
			shader_source = this->prepare_fragment_shader(tested_shader_type, array_declaration, test_snippet);
			break;

		case TestCaseBase<API>::COMPUTE_SHADER_TYPE:
			shader_source = this->prepare_compute_shader(tested_shader_type, array_declaration, test_snippet);
			break;

		case TestCaseBase<API>::GEOMETRY_SHADER_TYPE:
			shader_source = this->prepare_geometry_shader(tested_shader_type, array_declaration, test_snippet);
			break;

		case TestCaseBase<API>::TESSELATION_CONTROL_SHADER_TYPE:
			shader_source = this->prepare_tess_ctrl_shader(tested_shader_type, array_declaration, test_snippet);
			break;

		case TestCaseBase<API>::TESSELATION_EVALUATION_SHADER_TYPE:
			shader_source = this->prepare_tess_eval_shader(tested_shader_type, array_declaration, test_snippet);
			break;

		default:
			TCU_FAIL("Unrecognized shader type.");
			break;
		} /* switch (tested_shader_type) */

		this->execute_negative_test(tested_shader_type, shader_source);
	} /* for (int string_index = 0; ...) */
}

/* Generates the shader source code for the ExpressionsInvalid1
 * array tests, and attempts to compile each test shader, for both
 * vertex and fragment shaders.
 *
 * @tparam API               Tested API descriptor
 *
 * @param tested_shader_type The type of shader that is being tested
 *                           (either TestCaseBase<API>::VERTEX_SHADER_TYPE or TestCaseBase<API>::FRAGMENT_SHADER_TYPE).
 */
template <class API>
void ExpressionsInvalid1<API>::test_shader_compilation(typename TestCaseBase<API>::TestShaderType tested_shader_type)
{
	std::string shader_variable_declarations =
		"    mat2  y       = mat2(0.0);\n"
		"    float x[2][2] = float[2][2](float[2](4.0, 5.0), float[2](6.0, 7.0));\n\n";

	std::string shader_source = shader_start + shader_variable_declarations;

	shader_source += "    y = x;\n";

	DEFAULT_MAIN_ENDING(tested_shader_type, shader_source);

	this->execute_negative_test(tested_shader_type, shader_source);
}

/* Generates the shader source code for the ExpressionsInvalid2
 * array tests, and attempts to compile each test shader, for both
 * vertex and fragment shaders.
 *
 * @tparam API               Tested API descriptor
 *
 * @param tested_shader_type The type of shader that is being tested
 *                           (either TestCaseBase<API>::VERTEX_SHADER_TYPE or TestCaseBase<API>::FRAGMENT_SHADER_TYPE).
 */
template <class API>
void ExpressionsInvalid2<API>::test_shader_compilation(typename TestCaseBase<API>::TestShaderType tested_shader_type)
{

	std::string shader_variable_declarations[] = { " x", " y" };
	std::string variable_relation_opeartors[]  = {
		"    float result = 0.0;\n\n    if(x < y)\n    {\n        result = 1.0;\n    }\n\n\n",
		"    float result = 0.0;\n\n    if(x <= y)\n    {\n        result = 1.0;\n    }\n\n\n",
		"    float result = 0.0;\n\n    if(x > y)\n    {\n        result = 1.0;\n    }\n\n\n",
		"    float result = 0.0;\n\n    if(x >= y)\n    {\n        result = 1.0;\n    }\n\n\n"
	};
	std::string valid_relation_opeartors =
		"    float result = 0.0;\n\n    if(x == y)\n    {\n        result = 1.0;\n    }\n\n\n";

	for (size_t var_type_index = 0; var_type_index < API::n_var_types; var_type_index++)
	{
		_supported_variable_types_map_const_iterator var_iterator =
			supported_variable_types_map.find(API::var_types[var_type_index]);

		if (var_iterator != supported_variable_types_map.end())
		{
			std::string base_variable_string;

			for (size_t variable_declaration_index = 0;
				 variable_declaration_index <
				 sizeof(shader_variable_declarations) / sizeof(shader_variable_declarations[0]);
				 variable_declaration_index++)
			{
				base_variable_string += var_iterator->second.type;
				base_variable_string += shader_variable_declarations[variable_declaration_index];

				base_variable_string += "[1][1][1][1][1][1][1][1] = ";

				for (size_t sub_script_index = 0; sub_script_index < API::MAX_ARRAY_DIMENSIONS; sub_script_index++)
				{
					base_variable_string += this->extend_string(var_iterator->second.type, "[1]",
																API::MAX_ARRAY_DIMENSIONS - sub_script_index);
					base_variable_string += "(";
				}

				base_variable_string += var_iterator->second.initializer_with_ones;

				for (size_t sub_script_index = 0; sub_script_index < API::MAX_ARRAY_DIMENSIONS; sub_script_index++)
				{
					base_variable_string += ")";
				}

				base_variable_string += ";\n";
			} /* for (int variable_declaration_index = 0; ...) */

			/* Run positive case */
			{
				std::string shader_source;

				shader_source = base_variable_string + "\n";
				shader_source += shader_start + valid_relation_opeartors;

				DEFAULT_MAIN_ENDING(tested_shader_type, shader_source);

				EXECUTE_POSITIVE_TEST(tested_shader_type, shader_source, true, false);
			}

			/* Run negative cases */
			for (size_t string_index = 0;
				 string_index < sizeof(variable_relation_opeartors) / sizeof(variable_relation_opeartors[0]);
				 string_index++)
			{
				std::string shader_source;

				shader_source = base_variable_string + "\n";
				shader_source += shader_start + variable_relation_opeartors[string_index];

				DEFAULT_MAIN_ENDING(tested_shader_type, shader_source);

				this->execute_negative_test(tested_shader_type, shader_source);
			} /* for (int string_index = 0; ...) */
		}	 /* if var_type iterator found */
		else
		{
			TCU_FAIL("Type not found.");
		}
	} /* for (int var_type_index = 0; ...) */
}

/* Generates the shader source code for the InteractionFunctionCalls1
 * array tests, and attempts to compile each test shader, for both
 * vertex and fragment shaders.
 *
 * @tparam API               Tested API descriptor
 *
 * @param tested_shader_type The type of shader that is being tested
 *                           (either TestCaseBase<API>::VERTEX_SHADER_TYPE or TestCaseBase<API>::FRAGMENT_SHADER_TYPE).
 */
template <class API>
void InteractionFunctionCalls1<API>::test_shader_compilation(
	typename TestCaseBase<API>::TestShaderType tested_shader_type)
{
	static const glcts::test_var_type var_types_set_es[] = { VAR_TYPE_INT,   VAR_TYPE_FLOAT, VAR_TYPE_IVEC2,
															 VAR_TYPE_IVEC3, VAR_TYPE_IVEC4, VAR_TYPE_VEC2,
															 VAR_TYPE_VEC3,  VAR_TYPE_VEC4,  VAR_TYPE_MAT2,
															 VAR_TYPE_MAT3,  VAR_TYPE_MAT4 };
	static const size_t num_var_types_es = sizeof(var_types_set_es) / sizeof(var_types_set_es[0]);

	static const glcts::test_var_type var_types_set_gl[] = { VAR_TYPE_INT,   VAR_TYPE_FLOAT, VAR_TYPE_IVEC2,
															 VAR_TYPE_IVEC3, VAR_TYPE_IVEC4, VAR_TYPE_VEC2,
															 VAR_TYPE_VEC3,  VAR_TYPE_VEC4,  VAR_TYPE_MAT2,
															 VAR_TYPE_MAT3,  VAR_TYPE_MAT4,  VAR_TYPE_DOUBLE,
															 VAR_TYPE_DMAT2, VAR_TYPE_DMAT3, VAR_TYPE_DMAT4 };
	static const size_t num_var_types_gl = sizeof(var_types_set_gl) / sizeof(var_types_set_gl[0]);

	const std::string iteration_loop_end = "                }\n"
										   "            }\n"
										   "        }\n"
										   "    }\n";
	const std::string iteration_loop_start = "    for (uint a = 0u; a < 2u; a++)\n"
											 "    {\n"
											 "        for (uint b = 0u; b < 2u; b++)\n"
											 "        {\n"
											 "            for (uint c = 0u; c < 2u; c++)\n"
											 "            {\n"
											 "                for (uint d = 0u; d < 2u; d++)\n"
											 "                {\n";
	const glcts::test_var_type* var_types_set = var_types_set_es;
	size_t						num_var_types = num_var_types_es;
	const bool					test_compute  = (TestCaseBase<API>::COMPUTE_SHADER_TYPE == tested_shader_type);

	if (API::USE_DOUBLE)
	{
		var_types_set = var_types_set_gl;
		num_var_types = num_var_types_gl;
	}

	for (size_t var_type_index = 0; var_type_index < num_var_types; var_type_index++)
	{
		_supported_variable_types_map_const_iterator var_iterator =
			supported_variable_types_map.find(var_types_set[var_type_index]);

		if (var_iterator != supported_variable_types_map.end())
		{
			std::string iterator_declaration = "    " + var_iterator->second.iterator_type +
											   " iterator = " + var_iterator->second.iterator_initialization + ";\n";

			std::string function_definition;
			std::string function_use;
			std::string verification;

			function_definition = "void my_function(out ";
			function_definition += var_iterator->second.type;
			function_definition += " output_array[2][2][2][2]) {\n";
			function_definition += iterator_declaration;
			function_definition += iteration_loop_start;
			function_definition += "                                   output_array[a][b][c][d] = " +
								   var_iterator->second.variable_type_initializer1 + ";\n";
			function_definition +=
				"                                   iterator += " + var_iterator->second.iterator_type + "(1);\n";
			function_definition += iteration_loop_end;
			function_definition += "}";

			function_use = "    " + var_iterator->second.type + " my_array[2][2][2][2];\n";
			function_use += "    my_function(my_array);";

			verification = iterator_declaration;
			verification += "    float result = 1.0;\n";
			verification += iteration_loop_start;
			verification += "                                   if (my_array[a][b][c][d] " +
							var_iterator->second.specific_element +
							" != iterator)\n"
							"                                   {\n"
							"                                       result = 0.0;\n"
							"                                   }\n"
							"                                   iterator += " +
							var_iterator->second.iterator_type + "(1);\n";
			verification += iteration_loop_end;

			if (false == test_compute)
			{
				execute_draw_test(tested_shader_type, function_definition, function_use, verification);
			}
			else
			{
				execute_dispatch_test(tested_shader_type, function_definition, function_use, verification);
			}

			/* Deallocate any resources used. */
			this->delete_objects();
		} /* if var_type iterator found */
		else
		{
			TCU_FAIL("Type not found.");
		}
	} /* for (int var_type_index = 0; ...) */
}

/** Executes test for compute program
 *
 * @tparam API                Tested API descriptor
 *
 * @param tested_shader_type  The type of shader that is being tested
 * @param function_definition Definition used to prepare shader
 * @param function_use        Snippet that makes use of defined function
 * @param verification        Snippet that verifies results
 **/
template <class API>
void InteractionFunctionCalls1<API>::execute_dispatch_test(
	typename TestCaseBase<API>::TestShaderType tested_shader_type, const std::string& function_definition,
	const std::string& function_use, const std::string& verification)
{
	const std::string& compute_shader_source =
		prepare_compute_shader(tested_shader_type, function_definition, function_use, verification);
	const glw::Functions& gl = this->context_id.getRenderContext().getFunctions();

	this->execute_positive_test(empty_string, empty_string, empty_string, empty_string, empty_string,
								compute_shader_source, false, false);

	/* We are now ready to verify whether the returned size is correct. */
	unsigned char buffer[4]				= { 0 };
	glw::GLuint   framebuffer_object_id = 0;
	glw::GLint	location				= -1;
	glw::GLuint   texture_object_id		= 0;
	glw::GLuint   vao_id				= 0;

	gl.useProgram(this->program_object_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram() failed.");

	gl.genTextures(1, &texture_object_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenTextures() failed.");

	gl.bindTexture(GL_TEXTURE_2D, texture_object_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture() failed.");

	gl.texStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, 1, 1);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTexStorage2D() failed.");

	gl.bindImageTexture(0 /* image unit */, texture_object_id, 0 /* level */, GL_FALSE /* layered */, 0 /* layer */,
						GL_WRITE_ONLY, GL_RGBA8);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindImageTexture() failed.");

	location = gl.getUniformLocation(this->program_object_id, "uni_image");
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetUniformLocation() failed.");

	if (-1 == location)
	{
		TCU_FAIL("Uniform is inactive");
	}

	gl.uniform1i(location, 0 /* image unit */);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUniform1i() failed.");

	gl.genVertexArrays(1, &vao_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenVertexArrays() failed.");

	gl.bindVertexArray(vao_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindVertexArray() failed.");

	gl.dispatchCompute(1, 1, 1);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawArrays() failed.");

	gl.genFramebuffers(1, &framebuffer_object_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenFramebuffers() failed.");

	gl.bindFramebuffer(GL_FRAMEBUFFER, framebuffer_object_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffer() failed.");

	gl.framebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture_object_id, 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glFramebufferTexture2D() failed.");

	gl.viewport(0, 0, 1, 1);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glViewport() failed.");

	gl.readBuffer(GL_COLOR_ATTACHMENT0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glReadBuffer() failed.");

	gl.readPixels(0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, buffer);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glReadPixels() failed.");

	if (buffer[0] != 255)
	{
		TCU_FAIL("Invalid array size was returned.");
	}

	/* Delete generated objects. */
	gl.deleteTextures(1, &texture_object_id);
	gl.deleteFramebuffers(1, &framebuffer_object_id);
	gl.deleteVertexArrays(1, &vao_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "An error ocurred while deleting generated objects.");
}

/** Executes test for draw program
 *
 * @tparam API                Tested API descriptor
 *
 * @param tested_shader_type  The type of shader that is being tested
 * @param function_definition Definition used to prepare shader
 * @param function_use        Snippet that makes use of defined function
 * @param verification        Snippet that verifies results
 **/
template <class API>
void InteractionFunctionCalls1<API>::execute_draw_test(typename TestCaseBase<API>::TestShaderType tested_shader_type,
													   const std::string&						  function_definition,
													   const std::string& function_use, const std::string& verification)
{
	const glw::Functions& gl = this->context_id.getRenderContext().getFunctions();

	if (API::USE_ALL_SHADER_STAGES)
	{
		const std::string& compute_shader_source = empty_string;
		const std::string& fragment_shader_source =
			this->prepare_fragment_shader(tested_shader_type, function_definition, function_use, verification);
		const std::string& geometry_shader_source =
			this->prepare_geometry_shader(tested_shader_type, function_definition, function_use, verification);
		const std::string& tess_ctrl_shader_source =
			this->prepare_tess_ctrl_shader(tested_shader_type, function_definition, function_use, verification);
		const std::string& tess_eval_shader_source =
			this->prepare_tess_eval_shader(tested_shader_type, function_definition, function_use, verification);
		const std::string& vertex_shader_source =
			this->prepare_vertex_shader(tested_shader_type, function_definition, function_use, verification);

		switch (tested_shader_type)
		{
		case TestCaseBase<API>::VERTEX_SHADER_TYPE: /* Fall through */
		case TestCaseBase<API>::FRAGMENT_SHADER_TYPE:
			this->execute_positive_test(vertex_shader_source, fragment_shader_source, false, false);
			break;

		case TestCaseBase<API>::COMPUTE_SHADER_TYPE:
		case TestCaseBase<API>::GEOMETRY_SHADER_TYPE:
		case TestCaseBase<API>::TESSELATION_CONTROL_SHADER_TYPE: /* Fall through */
		case TestCaseBase<API>::TESSELATION_EVALUATION_SHADER_TYPE:
			this->execute_positive_test(vertex_shader_source, tess_ctrl_shader_source, tess_eval_shader_source,
										geometry_shader_source, fragment_shader_source, compute_shader_source, false,
										false);
			break;

		default:
			TCU_FAIL("Invalid enum");
			break;
		}
	}
	else
	{
		const std::string& fragment_shader_source =
			this->prepare_fragment_shader(tested_shader_type, function_definition, function_use, verification);
		const std::string& vertex_shader_source =
			this->prepare_vertex_shader(tested_shader_type, function_definition, function_use, verification);

		this->execute_positive_test(vertex_shader_source, fragment_shader_source, false, false);
	}

	/* We are now ready to verify whether the returned size is correct. */
	unsigned char buffer[4]				= { 0 };
	glw::GLuint   framebuffer_object_id = 0;
	glw::GLuint   texture_object_id		= 0;
	glw::GLuint   vao_id				= 0;

	gl.useProgram(this->program_object_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram() failed.");

	gl.genTextures(1, &texture_object_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenTextures() failed.");

	gl.bindTexture(GL_TEXTURE_2D, texture_object_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture() failed.");

	gl.texStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, 1, 1);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTexStorage2D() failed.");

	gl.genFramebuffers(1, &framebuffer_object_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenFramebuffers() failed.");

	gl.bindFramebuffer(GL_FRAMEBUFFER, framebuffer_object_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffer() failed.");

	gl.framebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture_object_id, 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glFramebufferTexture2D() failed.");

	gl.viewport(0, 0, 1, 1);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glViewport() failed.");

	gl.genVertexArrays(1, &vao_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenVertexArrays() failed.");

	gl.bindVertexArray(vao_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindVertexArray() failed.");

	switch (tested_shader_type)
	{
	case TestCaseBase<API>::FRAGMENT_SHADER_TYPE: /* Fall through */
	case TestCaseBase<API>::VERTEX_SHADER_TYPE:
		gl.drawArrays(GL_TRIANGLE_FAN, 0, 4);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawArrays() failed.");
		break;

	case TestCaseBase<API>::TESSELATION_CONTROL_SHADER_TYPE: /* Fall through */
	case TestCaseBase<API>::TESSELATION_EVALUATION_SHADER_TYPE:
		/* Tesselation patch set up */
		gl.patchParameteri(GL_PATCH_VERTICES, 1);
		GLU_EXPECT_NO_ERROR(gl.getError(), "PatchParameteri");

		gl.drawArrays(GL_PATCHES, 0, 1);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawArrays() failed.");
		break;

	case TestCaseBase<API>::GEOMETRY_SHADER_TYPE:
		gl.drawArrays(GL_POINTS, 0, 1);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawArrays() failed.");
		break;

	default:
		TCU_FAIL("Invalid enum");
		break;
	}

	gl.readBuffer(GL_COLOR_ATTACHMENT0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glReadBuffer() failed.");

	gl.readPixels(0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, buffer);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glReadPixels() failed.");

	if (buffer[0] != 255)
	{
		TCU_FAIL("Invalid array size was returned.");
	}

	/* Delete generated objects. */
	gl.bindTexture(GL_TEXTURE_2D, 0);
	gl.bindFramebuffer(GL_FRAMEBUFFER, 0);
	gl.bindVertexArray(0);
	gl.deleteTextures(1, &texture_object_id);
	gl.deleteFramebuffers(1, &framebuffer_object_id);
	gl.deleteVertexArrays(1, &vao_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "An error ocurred while deleting generated objects.");
}

/** Prepare shader
 *
 * @tparam API                Tested API descriptor
 *
 * @param tested_shader_type  The type of shader that is being tested
 * @param function_definition Definition used to prepare shader
 * @param function_use        Snippet that makes use of defined function
 * @param verification        Snippet that verifies results
 **/
template <class API>
std::string InteractionFunctionCalls1<API>::prepare_compute_shader(
	typename TestCaseBase<API>::TestShaderType tested_shader_type, const std::string& function_definition,
	const std::string& function_use, const std::string& verification)
{
	std::string compute_shader_source;

	if (TestCaseBase<API>::COMPUTE_SHADER_TYPE == tested_shader_type)
	{
		compute_shader_source = "writeonly uniform image2D uni_image;\n"
								"\n";

		/* User-defined function definition. */
		compute_shader_source += function_definition;
		compute_shader_source += "\n\n";

		/* Main function definition. */
		compute_shader_source += shader_start;
		compute_shader_source += function_use;
		compute_shader_source += "\n\n";
		compute_shader_source += verification;
		compute_shader_source += "\n\n";
		compute_shader_source += "\n"
								 "    imageStore(uni_image, ivec2(gl_GlobalInvocationID.xy), vec4(result, 0, 0, 0));\n"
								 "}\n"
								 "\n";
	}

	return compute_shader_source;
}

/** Prepare shader
 *
 * @tparam API                Tested API descriptor
 *
 * @param tested_shader_type  The type of shader that is being tested
 * @param function_definition Definition used to prepare shader
 * @param function_use        Snippet that makes use of defined function
 * @param verification        Snippet that verifies results
 **/
template <class API>
std::string InteractionFunctionCalls1<API>::prepare_fragment_shader(
	typename TestCaseBase<API>::TestShaderType tested_shader_type, const std::string& function_definition,
	const std::string& function_use, const std::string& verification)
{
	std::string fragment_shader_source;

	switch (tested_shader_type)
	{
	case TestCaseBase<API>::COMPUTE_SHADER_TYPE:
		break;

	case TestCaseBase<API>::FRAGMENT_SHADER_TYPE:
		fragment_shader_source = "out vec4 colour;\n\n";

		/* User-defined function definition. */
		fragment_shader_source += function_definition;
		fragment_shader_source += "\n\n";

		/* Main function definition. */
		fragment_shader_source += shader_start;
		fragment_shader_source += function_use;
		fragment_shader_source += "\n\n";
		fragment_shader_source += verification;
		fragment_shader_source += "\n\n";
		fragment_shader_source += "    colour = vec4(result);\n";
		fragment_shader_source += shader_end;
		break;

	case TestCaseBase<API>::GEOMETRY_SHADER_TYPE:
	case TestCaseBase<API>::TESSELATION_CONTROL_SHADER_TYPE:
	case TestCaseBase<API>::TESSELATION_EVALUATION_SHADER_TYPE:
	case TestCaseBase<API>::VERTEX_SHADER_TYPE:
		fragment_shader_source = "in float fs_result;\n\n"
								 "out vec4 colour;\n\n"
								 "void main()\n"
								 "{\n"
								 "    colour =  vec4(fs_result);\n"
								 "}\n"
								 "\n";
		break;

	default:
		TCU_FAIL("Unrecognized shader object type.");
		break;
	}

	return fragment_shader_source;
}

/** Prepare shader
 *
 * @tparam API                Tested API descriptor
 *
 * @param tested_shader_type  The type of shader that is being tested
 * @param function_definition Definition used to prepare shader
 * @param function_use        Snippet that makes use of defined function
 * @param verification        Snippet that verifies results
 **/
template <class API>
std::string InteractionFunctionCalls1<API>::prepare_geometry_shader(
	typename TestCaseBase<API>::TestShaderType tested_shader_type, const std::string& function_definition,
	const std::string& function_use, const std::string& verification)
{
	std::string geometry_shader_source;

	switch (tested_shader_type)
	{
	case TestCaseBase<API>::COMPUTE_SHADER_TYPE:
	case TestCaseBase<API>::FRAGMENT_SHADER_TYPE:
	case TestCaseBase<API>::VERTEX_SHADER_TYPE:
		break;

	case TestCaseBase<API>::TESSELATION_CONTROL_SHADER_TYPE:
	case TestCaseBase<API>::TESSELATION_EVALUATION_SHADER_TYPE:
		geometry_shader_source = "layout(points)                           in;\n"
								 "layout(triangle_strip, max_vertices = 4) out;\n"
								 "\n"
								 "in  float tes_result[];\n"
								 "out float fs_result;\n"
								 "\n"
								 "void main()\n"
								 "{\n"
								 "    gl_Position  = vec4(-1, -1, 0, 1);\n"
								 "    fs_result    = tes_result[0];\n"
								 "    EmitVertex();\n"
								 "    gl_Position  = vec4(-1, 1, 0, 1);\n"
								 "    fs_result    = tes_result[0];\n"
								 "    EmitVertex();\n"
								 "    gl_Position  = vec4(1, -1, 0, 1);\n"
								 "    fs_result    = tes_result[0];\n"
								 "    EmitVertex();\n"
								 "    gl_Position  = vec4(1, 1, 0, 1);\n"
								 "    fs_result    = tes_result[0];\n"
								 "    EmitVertex();\n"
								 "}\n";
		break;

	case TestCaseBase<API>::GEOMETRY_SHADER_TYPE:
		geometry_shader_source = "layout(points)                           in;\n"
								 "layout(triangle_strip, max_vertices = 4) out;\n"
								 "\n"
								 "out float fs_result;\n"
								 "\n";

		/* User-defined function definition. */
		geometry_shader_source += function_definition;
		geometry_shader_source += "\n\n";

		/* Main function definition. */
		geometry_shader_source += shader_start;
		geometry_shader_source += function_use;
		geometry_shader_source += "\n\n";
		geometry_shader_source += verification;
		geometry_shader_source += "\n\n";
		geometry_shader_source += "\n    gl_Position  = vec4(-1, -1, 0, 1);\n"
								  "    fs_result    = result;\n"
								  "    EmitVertex();\n"
								  "    gl_Position  = vec4(-1, 1, 0, 1);\n"
								  "    fs_result    = result;\n"
								  "    EmitVertex();\n"
								  "    gl_Position  = vec4(1, -1, 0, 1);\n"
								  "    fs_result    = result;\n"
								  "    EmitVertex();\n"
								  "    gl_Position  = vec4(1, 1, 0, 1);\n"
								  "    fs_result    = result;\n"
								  "    EmitVertex();\n"
								  "}\n";
		break;

	default:
		TCU_FAIL("Unrecognized shader object type.");
		break;
	}

	return geometry_shader_source;
}

/** Prepare shader
 *
 * @tparam API                Tested API descriptor
 *
 * @param tested_shader_type  The type of shader that is being tested
 * @param function_definition Definition used to prepare shader
 * @param function_use        Snippet that makes use of defined function
 * @param verification        Snippet that verifies results
 **/
template <class API>
std::string InteractionFunctionCalls1<API>::prepare_tess_ctrl_shader(
	typename TestCaseBase<API>::TestShaderType tested_shader_type, const std::string& function_definition,
	const std::string& function_use, const std::string& verification)
{
	std::string tess_ctrl_shader_source;

	switch (tested_shader_type)
	{
	case TestCaseBase<API>::COMPUTE_SHADER_TYPE:
	case TestCaseBase<API>::FRAGMENT_SHADER_TYPE:
	case TestCaseBase<API>::GEOMETRY_SHADER_TYPE:
	case TestCaseBase<API>::VERTEX_SHADER_TYPE:
		break;

	case TestCaseBase<API>::TESSELATION_CONTROL_SHADER_TYPE:
		tess_ctrl_shader_source = "layout(vertices = 1) out;\n"
								  "\n"
								  "out float tcs_result[];\n"
								  "\n";

		/* User-defined function definition. */
		tess_ctrl_shader_source += function_definition;
		tess_ctrl_shader_source += "\n\n";

		/* Main function definition. */
		tess_ctrl_shader_source += shader_start;
		tess_ctrl_shader_source += function_use;
		tess_ctrl_shader_source += "\n\n";
		tess_ctrl_shader_source += verification;
		tess_ctrl_shader_source += "\n\n";
		tess_ctrl_shader_source += "    tcs_result[gl_InvocationID] = result;\n"
								   "\n"
								   "    gl_TessLevelOuter[0] = 1.0;\n"
								   "    gl_TessLevelOuter[1] = 1.0;\n"
								   "    gl_TessLevelOuter[2] = 1.0;\n"
								   "    gl_TessLevelOuter[3] = 1.0;\n"
								   "    gl_TessLevelInner[0] = 1.0;\n"
								   "    gl_TessLevelInner[1] = 1.0;\n"
								   "}\n";
		break;

	case TestCaseBase<API>::TESSELATION_EVALUATION_SHADER_TYPE:
		tess_ctrl_shader_source = default_tc_shader_source;
		break;

	default:
		TCU_FAIL("Unrecognized shader object type.");
		break;
	}

	return tess_ctrl_shader_source;
}

/** Prepare shader
 *
 * @tparam API                Tested API descriptor
 *
 * @param tested_shader_type  The type of shader that is being tested
 * @param function_definition Definition used to prepare shader
 * @param function_use        Snippet that makes use of defined function
 * @param verification        Snippet that verifies results
 **/
template <class API>
std::string InteractionFunctionCalls1<API>::prepare_tess_eval_shader(
	typename TestCaseBase<API>::TestShaderType tested_shader_type, const std::string& function_definition,
	const std::string& function_use, const std::string& verification)
{
	std::string tess_eval_shader_source;

	switch (tested_shader_type)
	{
	case TestCaseBase<API>::COMPUTE_SHADER_TYPE:
	case TestCaseBase<API>::FRAGMENT_SHADER_TYPE:
	case TestCaseBase<API>::GEOMETRY_SHADER_TYPE:
	case TestCaseBase<API>::VERTEX_SHADER_TYPE:
		break;

	case TestCaseBase<API>::TESSELATION_CONTROL_SHADER_TYPE:
		tess_eval_shader_source = "layout(isolines, point_mode) in;\n"
								  "\n"
								  "in  float tcs_result[];\n"
								  "out float tes_result;\n"
								  "\n"
								  "void main()\n"
								  "{\n"
								  "    tes_result = tcs_result[0];\n"
								  "}\n";
		break;

	case TestCaseBase<API>::TESSELATION_EVALUATION_SHADER_TYPE:
		tess_eval_shader_source = "layout(isolines, point_mode) in;\n"
								  "\n"
								  "out float tes_result;\n"
								  "\n";

		/* User-defined function definition. */
		tess_eval_shader_source += function_definition;
		tess_eval_shader_source += "\n\n";

		/* Main function definition. */
		tess_eval_shader_source += shader_start;
		tess_eval_shader_source += function_use;
		tess_eval_shader_source += "\n\n";
		tess_eval_shader_source += verification;
		tess_eval_shader_source += "\n\n";
		tess_eval_shader_source += "    tes_result = result;\n"
								   "}\n";
		break;

	default:
		TCU_FAIL("Unrecognized shader object type.");
		break;
	}

	return tess_eval_shader_source;
}

/** Prepare shader
 *
 * @tparam API                Tested API descriptor
 *
 * @param tested_shader_type  The type of shader that is being tested
 * @param function_definition Definition used to prepare shader
 * @param function_use        Snippet that makes use of defined function
 * @param verification        Snippet that verifies results
 **/
template <class API>
std::string InteractionFunctionCalls1<API>::prepare_vertex_shader(
	typename TestCaseBase<API>::TestShaderType tested_shader_type, const std::string& function_definition,
	const std::string& function_use, const std::string& verification)
{
	std::string vertex_shader_source;

	switch (tested_shader_type)
	{
	case TestCaseBase<API>::COMPUTE_SHADER_TYPE:
		break;

	case TestCaseBase<API>::FRAGMENT_SHADER_TYPE:
		vertex_shader_source = "/** GL_TRIANGLE_FAN-type quad vertex data. */\n"
							   "const vec4 vertex_positions[4] = vec4[4](vec4( 1.0, -1.0, 0.0, 1.0),\n"
							   "                                         vec4(-1.0, -1.0, 0.0, 1.0),\n"
							   "                                         vec4(-1.0,  1.0, 0.0, 1.0),\n"
							   "                                         vec4( 1.0,  1.0, 0.0, 1.0) );\n"
							   "\n"
							   "void main()\n"
							   "{\n"
							   "    gl_Position = vertex_positions[gl_VertexID];"
							   "}\n\n";
		break;

	case TestCaseBase<API>::GEOMETRY_SHADER_TYPE:
	case TestCaseBase<API>::TESSELATION_CONTROL_SHADER_TYPE:
	case TestCaseBase<API>::TESSELATION_EVALUATION_SHADER_TYPE:
		vertex_shader_source = default_vertex_shader_source;
		break;

	case TestCaseBase<API>::VERTEX_SHADER_TYPE:
		/* Vertex shader source. */
		vertex_shader_source = "out float fs_result;\n\n";
		vertex_shader_source += "/** GL_TRIANGLE_FAN-type quad vertex data. */\n"
								"const vec4 vertex_positions[4] = vec4[4](vec4( 1.0, -1.0, 0.0, 1.0),\n"
								"                                         vec4(-1.0, -1.0, 0.0, 1.0),\n"
								"                                         vec4(-1.0,  1.0, 0.0, 1.0),\n"
								"                                         vec4( 1.0,  1.0, 0.0, 1.0) );\n\n";

		/* User-defined function definition. */
		vertex_shader_source += function_definition;
		vertex_shader_source += "\n\n";

		/* Main function definition. */
		vertex_shader_source += shader_start;
		vertex_shader_source += function_use;
		vertex_shader_source += "\n\n";
		vertex_shader_source += verification;
		vertex_shader_source += "\n\n";
		vertex_shader_source += "    fs_result   = result;\n"
								"    gl_Position = vertex_positions[gl_VertexID];\n";
		vertex_shader_source += shader_end;
		break;

	default:
		TCU_FAIL("Unrecognized shader object type.");
		break;
	}

	return vertex_shader_source;
}

/* Generates the shader source code for the InteractionFunctionCalls2
 * array tests, and attempts to compile each test shader, for both
 * vertex and fragment shaders.
 *
 * @tparam API               Tested API descriptor
 *
 * @param tested_shader_type The type of shader that is being tested
 *                           (either TestCaseBase<API>::VERTEX_SHADER_TYPE or TestCaseBase<API>::FRAGMENT_SHADER_TYPE).
 */
template <class API>
void InteractionFunctionCalls2<API>::test_shader_compilation(
	typename TestCaseBase<API>::TestShaderType tested_shader_type)
{
	static const glcts::test_var_type var_types_set_es[] = { VAR_TYPE_INT,   VAR_TYPE_FLOAT, VAR_TYPE_IVEC2,
															 VAR_TYPE_IVEC3, VAR_TYPE_IVEC4, VAR_TYPE_VEC2,
															 VAR_TYPE_VEC3,  VAR_TYPE_VEC4,  VAR_TYPE_MAT2,
															 VAR_TYPE_MAT3,  VAR_TYPE_MAT4 };
	static const size_t num_var_types_es = sizeof(var_types_set_es) / sizeof(var_types_set_es[0]);

	static const glcts::test_var_type var_types_set_gl[] = { VAR_TYPE_INT,   VAR_TYPE_FLOAT, VAR_TYPE_IVEC2,
															 VAR_TYPE_IVEC3, VAR_TYPE_IVEC4, VAR_TYPE_VEC2,
															 VAR_TYPE_VEC3,  VAR_TYPE_VEC4,  VAR_TYPE_MAT2,
															 VAR_TYPE_MAT3,  VAR_TYPE_MAT4,  VAR_TYPE_DOUBLE,
															 VAR_TYPE_DMAT2, VAR_TYPE_DMAT3, VAR_TYPE_DMAT4 };
	static const size_t num_var_types_gl = sizeof(var_types_set_gl) / sizeof(var_types_set_gl[0]);

	const std::string iteration_loop_end = "                }\n"
										   "            }\n"
										   "        }\n"
										   "    }\n";
	const std::string iteration_loop_start = "    for (uint a = 0u; a < 2u; a++)\n"
											 "    {\n"
											 "        for (uint b = 0u; b < 2u; b++)\n"
											 "        {\n"
											 "            for (uint c = 0u; c < 2u; c++)\n"
											 "            {\n"
											 "                for (uint d = 0u; d < 2u; d++)\n"
											 "                {\n";
	const std::string multiplier_array = "const int[] multiplier_array = int[]( 1,  2,  3,  4,  5,  6,  7,  8,\n"
										 "                                     11, 12, 13, 14, 15, 16, 17, 18,\n"
										 "                                     21, 22, 23, 24, 25, 26, 27, 28,\n"
										 "                                     31, 32, 33, 34, 35, 36, 37, 38,\n"
										 "                                     41, 42, 43, 44, 45, 46, 47, 48,\n"
										 "                                     51, 52, 53, 54, 55, 56, 57, 58,\n"
										 "                                     61, 62, 63, 64, 65, 66, 67, 68,\n"
										 "                                     71, 72, 73, 74, 75, 76, 77, 78,\n"
										 "                                     81, 82, 83, 84, 85, 86, 87, 88);\n";
	const glcts::test_var_type* var_types_set = var_types_set_es;
	size_t						num_var_types = num_var_types_es;
	const bool					test_compute  = (TestCaseBase<API>::COMPUTE_SHADER_TYPE == tested_shader_type);

	if (API::USE_DOUBLE)
	{
		var_types_set = var_types_set_gl;
		num_var_types = num_var_types_gl;
	}

	for (size_t var_type_index = 0; var_type_index < num_var_types; var_type_index++)
	{
		_supported_variable_types_map_const_iterator var_iterator =
			supported_variable_types_map.find(var_types_set[var_type_index]);

		if (var_iterator != supported_variable_types_map.end())
		{
			std::string function_definition;
			std::string function_use;
			std::string verification;

			function_definition += multiplier_array;
			function_definition += "void my_function(inout ";
			function_definition += var_iterator->second.type;
			function_definition += " inout_array[2][2][2][2]) {\n"
								   "    uint i = 0u;\n";
			function_definition += iteration_loop_start;
			function_definition += "                                   inout_array[a][b][c][d] *= " +
								   var_iterator->second.iterator_type + "(multiplier_array[i % 64u]);\n";
			function_definition += "                                   i+= 1u;\n";
			function_definition += iteration_loop_end;
			function_definition += "}";

			function_use += "    float result = 1.0;\n";
			function_use += "    uint iterator = 0u;\n";
			function_use += "    " + var_iterator->second.type + " my_array[2][2][2][2];\n";
			function_use += iteration_loop_start;
			function_use += "                                   my_array[a][b][c][d] = " +
							var_iterator->second.variable_type_initializer2 + ";\n";
			function_use += iteration_loop_end;
			function_use += "    my_function(my_array);";

			verification += iteration_loop_start;
			verification += "                                   if (my_array[a][b][c][d] " +
							var_iterator->second.specific_element + "!= " + var_iterator->second.iterator_type +
							"(multiplier_array[iterator % 64u]))\n"
							"                                   {\n"
							"                                       result = 0.0;\n"
							"                                   }\n"
							"                                   iterator += 1u;\n";
			verification += iteration_loop_end;

			if (false == test_compute)
			{
				this->execute_draw_test(tested_shader_type, function_definition, function_use, verification);
			}
			else
			{
				this->execute_dispatch_test(tested_shader_type, function_definition, function_use, verification);
			}

			/* Deallocate any resources used. */
			this->delete_objects();
		} /* if var_type iterator found */
		else
		{
			TCU_FAIL("Type not found.");
		}
	} /* for (int var_type_index = 0; ...) */
}

/* Generates the shader source code for the InteractionArgumentAliasing1
 * array tests, and attempts to compile each test shader, for both
 * vertex and fragment shaders.
 *
 * @tparam API               Tested API descriptor
 *
 * @param tested_shader_type The type of shader that is being tested
 *                           (either TestCaseBase<API>::VERTEX_SHADER_TYPE or TestCaseBase<API>::FRAGMENT_SHADER_TYPE).
 */
template <class API>
void InteractionArgumentAliasing1<API>::test_shader_compilation(
	typename TestCaseBase<API>::TestShaderType tested_shader_type)
{
	static const glcts::test_var_type var_types_set_es[] = { VAR_TYPE_INT, VAR_TYPE_FLOAT, VAR_TYPE_MAT4 };
	static const size_t				  num_var_types_es   = sizeof(var_types_set_es) / sizeof(var_types_set_es[0]);

	static const glcts::test_var_type var_types_set_gl[] = { VAR_TYPE_INT, VAR_TYPE_FLOAT, VAR_TYPE_MAT4,
															 VAR_TYPE_DOUBLE, VAR_TYPE_DMAT4 };
	static const size_t num_var_types_gl = sizeof(var_types_set_gl) / sizeof(var_types_set_gl[0]);

	const std::string iteration_loop_end = "                }\n"
										   "            }\n"
										   "        }\n"
										   "    }\n";
	const std::string iteration_loop_start = "    for (uint a = 0u; a < 2u; a++)\n"
											 "    {\n"
											 "        for (uint b = 0u; b < 2u; b++)\n"
											 "        {\n"
											 "            for (uint c = 0u; c < 2u; c++)\n"
											 "            {\n"
											 "                for (uint d = 0u; d < 2u; d++)\n"
											 "                {\n";
	const glcts::test_var_type* var_types_set = var_types_set_es;
	size_t						num_var_types = num_var_types_es;
	const bool					test_compute  = (TestCaseBase<API>::COMPUTE_SHADER_TYPE == tested_shader_type);

	if (API::USE_DOUBLE)
	{
		var_types_set = var_types_set_gl;
		num_var_types = num_var_types_gl;
	}

	for (size_t var_type_index = 0; var_type_index < num_var_types; var_type_index++)
	{
		_supported_variable_types_map_const_iterator var_iterator =
			supported_variable_types_map.find(var_types_set[var_type_index]);

		if (var_iterator != supported_variable_types_map.end())
		{
			std::string array_declaration = var_iterator->second.type + " z[2][2][2][2];\n\n";

			std::string function_definition;
			std::string function_use;
			std::string verification;

			function_definition += "bool gfunc(" + var_iterator->second.type + " x[2][2][2][2], ";
			function_definition += var_iterator->second.type + " y[2][2][2][2])\n";
			function_definition += "{\n";
			function_definition += "    " + iteration_loop_start;
			function_definition +=
				"                               x[a][b][c][d] = " + var_iterator->second.type + "(123);\n";
			function_definition += "    " + iteration_loop_end;
			function_definition += "\n";
			function_definition += "    " + iteration_loop_start;
			function_definition += "                                   if(y[a][b][c][d]";
			if (var_iterator->second.type == "mat4") // mat4 comparison
			{
				function_definition += "[0][0]";
				function_definition += " != float";
			}
			else if (var_iterator->second.type == "dmat4") // dmat4 comparison
			{
				function_definition += "[0][0]";
				function_definition += " != double";
			}
			else
			{
				function_definition += " != ";
				function_definition += var_iterator->second.type;
			}
			function_definition += "(((a*8u)+(b*4u)+(c*2u)+d))) {return false;}\n";
			function_definition += "    " + iteration_loop_end;
			function_definition += "  return true;\n";
			function_definition += "}";

			function_use += "    " + array_declaration;
			function_use += "    " + iteration_loop_start;
			function_use += "                                   z[a][b][c][d] = ";
			function_use += var_iterator->second.type;
			function_use += "(((a*8u)+(b*4u)+(c*2u)+d));\n";
			function_use += "    " + iteration_loop_end;

			verification += "    float result = 0.0;\n";
			verification += "    if(gfunc(z, z) == true)\n";
			verification += "    {\n";
			verification += "        result = 1.0;\n\n";
			verification += "    }\n";
			verification += "    else\n";
			verification += "    {\n";
			verification += "        result = 0.0;\n\n";
			verification += "    }\n";

			if (false == test_compute)
			{
				this->execute_draw_test(tested_shader_type, function_definition, function_use, verification);
			}
			else
			{
				this->execute_dispatch_test(tested_shader_type, function_definition, function_use, verification);
			}

			/* Deallocate any resources used. */
			this->delete_objects();
		} /* if var_type iterator found */
		else
		{
			TCU_FAIL("Type not found.");
		}
	}
}

/* Generates the shader source code for the InteractionArgumentAliasing2
 * array tests, and attempts to compile each test shader, for both
 * vertex and fragment shaders.
 *
 * @tparam API               Tested API descriptor
 *
 * @param tested_shader_type The type of shader that is being tested
 *                           (either TestCaseBase<API>::VERTEX_SHADER_TYPE or TestCaseBase<API>::FRAGMENT_SHADER_TYPE).
 */
template <class API>
void InteractionArgumentAliasing2<API>::test_shader_compilation(
	typename TestCaseBase<API>::TestShaderType tested_shader_type)
{
	static const glcts::test_var_type var_types_set_es[] = { VAR_TYPE_INT, VAR_TYPE_FLOAT, VAR_TYPE_MAT4 };
	static const size_t				  num_var_types_es   = sizeof(var_types_set_es) / sizeof(var_types_set_es[0]);

	static const glcts::test_var_type var_types_set_gl[] = { VAR_TYPE_INT, VAR_TYPE_FLOAT, VAR_TYPE_MAT4,
															 VAR_TYPE_DOUBLE, VAR_TYPE_DMAT4 };
	static const size_t num_var_types_gl = sizeof(var_types_set_gl) / sizeof(var_types_set_gl[0]);

	const std::string iteration_loop_end = "                }\n"
										   "            }\n"
										   "        }\n"
										   "    }\n";
	const std::string iteration_loop_start = "    for (uint a = 0u; a < 2u; a++)\n"
											 "    {\n"
											 "        for (uint b = 0u; b < 2u; b++)\n"
											 "        {\n"
											 "            for (uint c = 0u; c < 2u; c++)\n"
											 "            {\n"
											 "                for (uint d = 0u; d < 2u; d++)\n"
											 "                {\n";
	const glcts::test_var_type* var_types_set = var_types_set_es;
	size_t						num_var_types = num_var_types_es;
	const bool					test_compute  = (TestCaseBase<API>::COMPUTE_SHADER_TYPE == tested_shader_type);

	if (API::USE_DOUBLE)
	{
		var_types_set = var_types_set_gl;
		num_var_types = num_var_types_gl;
	}

	for (size_t var_type_index = 0; var_type_index < num_var_types; var_type_index++)
	{
		_supported_variable_types_map_const_iterator var_iterator =
			supported_variable_types_map.find(var_types_set[var_type_index]);

		if (var_iterator != supported_variable_types_map.end())
		{
			std::string array_declaration = var_iterator->second.type + " z[2][2][2][2];\n\n";

			std::string function_definition;
			std::string function_use;
			std::string verification;

			function_definition += "bool gfunc(" + var_iterator->second.type + " x[2][2][2][2], ";
			function_definition += var_iterator->second.type + " y[2][2][2][2])\n";
			function_definition += "{\n";
			function_definition += "    " + iteration_loop_start;
			function_definition +=
				"                                   y[a][b][c][d] = " + var_iterator->second.type +
				"(123);\n";
			function_definition += "    " + iteration_loop_end;
			function_definition += "\n";
			function_definition += "    " + iteration_loop_start;
			function_definition += "                                   if(x[a][b][c][d]";
			if (var_iterator->second.type == "mat4") // mat4 comparison
			{
				function_definition += "[0][0]";
				function_definition += " != float";
			}
			else if (var_iterator->second.type == "dmat4") // dmat4 comparison
			{
				function_definition += "[0][0]";
				function_definition += " != double";
			}
			else
			{
				function_definition += " != ";
				function_definition += var_iterator->second.type;
			}
			function_definition += "(((a*8u)+(b*4u)+(c*2u)+d))) {return false;}\n";
			function_definition += "    " + iteration_loop_end;
			function_definition += "  return true;\n";
			function_definition += "}";

			function_use += "    " + array_declaration;
			function_use += "    " + iteration_loop_start;
			function_use += "                                   z[a][b][c][d] = ";
			function_use += var_iterator->second.type;
			function_use += "(((a*8u)+(b*4u)+(c*2u)+d));\n";
			function_use += "    " + iteration_loop_end;

			verification += "    float result = 0.0;\n";
			verification += "    if(gfunc(z, z) == true)\n";
			verification += "    {\n";
			verification += "        result = 1.0;\n\n";
			verification += "    }\n";
			verification += "    else\n";
			verification += "    {\n";
			verification += "        result = 0.0;\n\n";
			verification += "    }\n";

			if (false == test_compute)
			{
				this->execute_draw_test(tested_shader_type, function_definition, function_use, verification);
			}
			else
			{
				this->execute_dispatch_test(tested_shader_type, function_definition, function_use, verification);
			}

			/* Deallocate any resources used. */
			this->delete_objects();
		} /* if var_type iterator found */
		else
		{
			TCU_FAIL("Type not found.");
		}
	}
}

/* Generates the shader source code for the InteractionArgumentAliasing3
 * array tests, and attempts to compile each test shader, for both
 * vertex and fragment shaders.
 *
 * @tparam API               Tested API descriptor
 *
 * @param tested_shader_type The type of shader that is being tested
 *                           (either TestCaseBase<API>::VERTEX_SHADER_TYPE or TestCaseBase<API>::FRAGMENT_SHADER_TYPE).
 */
template <class API>
void InteractionArgumentAliasing3<API>::test_shader_compilation(
	typename TestCaseBase<API>::TestShaderType tested_shader_type)
{
	static const glcts::test_var_type var_types_set_es[] = { VAR_TYPE_INT, VAR_TYPE_FLOAT, VAR_TYPE_MAT4 };
	static const size_t				  num_var_types_es   = sizeof(var_types_set_es) / sizeof(var_types_set_es[0]);

	static const glcts::test_var_type var_types_set_gl[] = { VAR_TYPE_INT, VAR_TYPE_FLOAT, VAR_TYPE_MAT4,
															 VAR_TYPE_DOUBLE, VAR_TYPE_DMAT4 };
	static const size_t num_var_types_gl = sizeof(var_types_set_gl) / sizeof(var_types_set_gl[0]);

	const std::string iteration_loop_end = "                }\n"
										   "            }\n"
										   "        }\n"
										   "    }\n";
	const std::string iteration_loop_start = "    for (uint a = 0u; a < 2u; a++)\n"
											 "    {\n"
											 "        for (uint b = 0u; b < 2u; b++)\n"
											 "        {\n"
											 "            for (uint c = 0u; c < 2u; c++)\n"
											 "            {\n"
											 "                for (uint d = 0u; d < 2u; d++)\n"
											 "                {\n";
	const glcts::test_var_type* var_types_set = var_types_set_es;
	size_t						num_var_types = num_var_types_es;
	const bool					test_compute  = (TestCaseBase<API>::COMPUTE_SHADER_TYPE == tested_shader_type);

	if (API::USE_DOUBLE)
	{
		var_types_set = var_types_set_gl;
		num_var_types = num_var_types_gl;
	}

	for (size_t var_type_index = 0; var_type_index < num_var_types; var_type_index++)
	{
		_supported_variable_types_map_const_iterator var_iterator =
			supported_variable_types_map.find(var_types_set[var_type_index]);

		if (var_iterator != supported_variable_types_map.end())
		{
			std::string array_declaration = var_iterator->second.type + " z[2][2][2][2];\n\n";

			std::string function_definition;
			std::string function_use;
			std::string verification;

			function_definition += "bool gfunc(out " + var_iterator->second.type + " x[2][2][2][2], ";
			function_definition += var_iterator->second.type + " y[2][2][2][2])\n";
			function_definition += "{\n";
			function_definition += "    " + iteration_loop_start;
			function_definition +=
				"                                   x[a][b][c][d] = " + var_iterator->second.type +
				"(123);\n";
			function_definition += "    " + iteration_loop_end;
			function_definition += "\n";
			function_definition += "    " + iteration_loop_start;
			function_definition += "                                   if(y[a][b][c][d]";
			if (var_iterator->second.type == "mat4") // mat4 comparison
			{
				function_definition += "[0][0]";
				function_definition += " != float";
			}
			else if (var_iterator->second.type == "dmat4") // dmat4 comparison
			{
				function_definition += "[0][0]";
				function_definition += " != double";
			}
			else
			{
				function_definition += " != ";
				function_definition += var_iterator->second.type;
			}
			function_definition += "(((a*8u)+(b*4u)+(c*2u)+d))) {return false;}\n";
			function_definition += "    " + iteration_loop_end;
			function_definition += "  return true;\n";
			function_definition += "}\n\n";

			function_use += "    " + array_declaration;
			function_use += "    " + iteration_loop_start;
			function_use += "                                   z[a][b][c][d] = ";
			function_use += var_iterator->second.type;
			function_use += "(((a*8u)+(b*4u)+(c*2u)+d));\n";
			function_use += "    " + iteration_loop_end;

			verification += "    float result = 0.0;\n";
			verification += "    if(gfunc(z, z) == true)\n";
			verification += "    {\n";
			verification += "        result = 1.0;\n\n";
			verification += "    }\n";
			verification += "    else\n";
			verification += "    {\n";
			verification += "        result = 0.0;\n\n";
			verification += "    }\n";

			if (false == test_compute)
			{
				this->execute_draw_test(tested_shader_type, function_definition, function_use, verification);
			}
			else
			{
				this->execute_dispatch_test(tested_shader_type, function_definition, function_use, verification);
			}

			/* Deallocate any resources used. */
			this->delete_objects();
		} /* if var_type iterator found */
		else
		{
			TCU_FAIL("Type not found.");
		}
	}
}

/* Generates the shader source code for the InteractionArgumentAliasing4
 * array tests, and attempts to compile each test shader, for both
 * vertex and fragment shaders.
 *
 * @tparam API               Tested API descriptor
 *
 * @param tested_shader_type The type of shader that is being tested
 *                           (either TestCaseBase<API>::VERTEX_SHADER_TYPE or TestCaseBase<API>::FRAGMENT_SHADER_TYPE).
 */
template <class API>
void InteractionArgumentAliasing4<API>::test_shader_compilation(
	typename TestCaseBase<API>::TestShaderType tested_shader_type)
{
	static const glcts::test_var_type var_types_set_es[] = { VAR_TYPE_INT, VAR_TYPE_FLOAT, VAR_TYPE_MAT4 };
	static const size_t				  num_var_types_es   = sizeof(var_types_set_es) / sizeof(var_types_set_es[0]);

	static const glcts::test_var_type var_types_set_gl[] = { VAR_TYPE_INT, VAR_TYPE_FLOAT, VAR_TYPE_MAT4,
															 VAR_TYPE_DOUBLE, VAR_TYPE_DMAT4 };
	static const size_t num_var_types_gl = sizeof(var_types_set_gl) / sizeof(var_types_set_gl[0]);

	const std::string iteration_loop_end = "                }\n"
										   "            }\n"
										   "        }\n"
										   "    }\n";
	const std::string iteration_loop_start = "    for (uint a = 0u; a < 2u; a++)\n"
											 "    {\n"
											 "        for (uint b = 0u; b < 2u; b++)\n"
											 "        {\n"
											 "            for (uint c = 0u; c < 2u; c++)\n"
											 "            {\n"
											 "                for (uint d = 0u; d < 2u; d++)\n"
											 "                {\n";
	const glcts::test_var_type* var_types_set = var_types_set_es;
	size_t						num_var_types = num_var_types_es;
	const bool					test_compute  = (TestCaseBase<API>::COMPUTE_SHADER_TYPE == tested_shader_type);

	if (API::USE_DOUBLE)
	{
		var_types_set = var_types_set_gl;
		num_var_types = num_var_types_gl;
	}

	for (size_t var_type_index = 0; var_type_index < num_var_types; var_type_index++)
	{
		_supported_variable_types_map_const_iterator var_iterator =
			supported_variable_types_map.find(var_types_set[var_type_index]);

		if (var_iterator != supported_variable_types_map.end())
		{
			std::string array_declaration = var_iterator->second.type + "[2][2][2][2] z;\n\n";

			std::string function_definition;
			std::string function_use;
			std::string verification;

			function_definition += "bool gfunc(" + var_iterator->second.type + " x[2][2][2][2], ";
			function_definition += "out " + var_iterator->second.type + " y[2][2][2][2])\n";
			function_definition += "{\n";
			function_definition += "    " + iteration_loop_start;
			function_definition +=
				"                                   y[a][b][c][d] = " + var_iterator->second.type +
				"(123);\n";
			function_definition += "    " + iteration_loop_end;
			function_definition += "\n";
			function_definition += "    " + iteration_loop_start;
			function_definition += "                                   if(x[a][b][c][d]";
			if (var_iterator->second.type == "mat4") // mat4 comparison
			{
				function_definition += "[0][0]";
				function_definition += " != float";
			}
			else if (var_iterator->second.type == "dmat4") // dmat4 comparison
			{
				function_definition += "[0][0]";
				function_definition += " != double";
			}
			else
			{
				function_definition += " != ";
				function_definition += var_iterator->second.type;
			}
			function_definition += "(((a*8u)+(b*4u)+(c*2u)+d))) {return false;}\n";
			function_definition += "    " + iteration_loop_end;
			function_definition += "  return true;\n";
			function_definition += "}\n\n";

			function_use += "    " + array_declaration;
			function_use += "    " + iteration_loop_start;
			function_use += "                                   z[a][b][c][d] = ";
			function_use += var_iterator->second.type;
			function_use += "(((a*8u)+(b*4u)+(c*2u)+d));\n";
			function_use += "    " + iteration_loop_end;

			verification += "    float result = 0.0;\n";
			verification += "    if(gfunc(z, z) == true)\n";
			verification += "    {\n";
			verification += "        result = 1.0;\n\n";
			verification += "    }\n";
			verification += "    else\n";
			verification += "    {\n";
			verification += "        result = 0.0;\n\n";
			verification += "    }\n";

			if (false == test_compute)
			{
				this->execute_draw_test(tested_shader_type, function_definition, function_use, verification);
			}
			else
			{
				this->execute_dispatch_test(tested_shader_type, function_definition, function_use, verification);
			}

			/* Deallocate any resources used. */
			this->delete_objects();
		} /* if var_type iterator found */
		else
		{
			TCU_FAIL("Type not found.");
		}
	}
}

/* Generates the shader source code for the InteractionArgumentAliasing3
 * array tests, and attempts to compile each test shader, for both
 * vertex and fragment shaders.
 *
 * @tparam API               Tested API descriptor
 *
 * @param tested_shader_type The type of shader that is being tested
 *                           (either TestCaseBase<API>::VERTEX_SHADER_TYPE or TestCaseBase<API>::FRAGMENT_SHADER_TYPE).
 */
template <class API>
void InteractionArgumentAliasing5<API>::test_shader_compilation(
	typename TestCaseBase<API>::TestShaderType tested_shader_type)
{
	static const glcts::test_var_type var_types_set_es[] = { VAR_TYPE_INT, VAR_TYPE_FLOAT, VAR_TYPE_MAT4 };
	static const size_t				  num_var_types_es   = sizeof(var_types_set_es) / sizeof(var_types_set_es[0]);

	static const glcts::test_var_type var_types_set_gl[] = { VAR_TYPE_INT, VAR_TYPE_FLOAT, VAR_TYPE_MAT4,
															 VAR_TYPE_DOUBLE, VAR_TYPE_DMAT4 };
	static const size_t num_var_types_gl = sizeof(var_types_set_gl) / sizeof(var_types_set_gl[0]);

	const std::string iteration_loop_end = "                }\n"
										   "            }\n"
										   "        }\n"
										   "    }\n";
	const std::string iteration_loop_start = "    for (uint a = 0u; a < 2u; a++)\n"
											 "    {\n"
											 "        for (uint b = 0u; b < 2u; b++)\n"
											 "        {\n"
											 "            for (uint c = 0u; c < 2u; c++)\n"
											 "            {\n"
											 "                for (uint d = 0u; d < 2u; d++)\n"
											 "                {\n";
	const glcts::test_var_type* var_types_set = var_types_set_es;
	size_t						num_var_types = num_var_types_es;
	const bool					test_compute  = (TestCaseBase<API>::COMPUTE_SHADER_TYPE == tested_shader_type);

	if (API::USE_DOUBLE)
	{
		var_types_set = var_types_set_gl;
		num_var_types = num_var_types_gl;
	}

	for (size_t var_type_index = 0; var_type_index < num_var_types; var_type_index++)
	{
		_supported_variable_types_map_const_iterator var_iterator =
			supported_variable_types_map.find(var_types_set[var_type_index]);

		if (var_iterator != supported_variable_types_map.end())
		{
			std::string array_declaration = var_iterator->second.type + "[2][2][2][2] z;\n\n";

			std::string function_definition;
			std::string function_use;
			std::string verification;

			function_definition += "bool gfunc(inout " + var_iterator->second.type + " x[2][2][2][2], ";
			function_definition += var_iterator->second.type + " y[2][2][2][2])\n";
			function_definition += "{\n";
			function_definition += "    " + iteration_loop_start;
			function_definition +=
				"                                   x[a][b][c][d] = " + var_iterator->second.type +
				"(123);\n";
			function_definition += "    " + iteration_loop_end;
			function_definition += "\n";
			function_definition += "    " + iteration_loop_start;
			function_definition += "                                   if(y[a][b][c][d]";
			if (var_iterator->second.type == "mat4") // mat4 comparison
			{
				function_definition += "[0][0]";
				function_definition += " != float";
			}
			else if (var_iterator->second.type == "dmat4") // dmat4 comparison
			{
				function_definition += "[0][0]";
				function_definition += " != double";
			}
			else
			{
				function_definition += " != ";
				function_definition += var_iterator->second.type;
			}
			function_definition += "(((a*8u)+(b*4u)+(c*2u)+d))) {return false;}\n";
			function_definition += "    " + iteration_loop_end;
			function_definition += "  return true;\n";
			function_definition += "}\n\n";

			function_use += "    " + array_declaration;
			function_use += "    " + iteration_loop_start;
			function_use += "                                   z[a][b][c][d] = ";
			function_use += var_iterator->second.type;
			function_use += "(((a*8u)+(b*4u)+(c*2u)+d));\n";
			function_use += "    " + iteration_loop_end;

			verification += "    float result = 0.0;\n";
			verification += "    if(gfunc(z, z) == true)\n";
			verification += "    {\n";
			verification += "        result = 1.0;\n\n";
			verification += "    }\n";
			verification += "    else\n";
			verification += "    {\n";
			verification += "        result = 0.0;\n\n";
			verification += "    }\n";

			if (false == test_compute)
			{
				this->execute_draw_test(tested_shader_type, function_definition, function_use, verification);
			}
			else
			{
				this->execute_dispatch_test(tested_shader_type, function_definition, function_use, verification);
			}

			/* Deallocate any resources used. */
			this->delete_objects();
		} /* if var_type iterator found */
		else
		{
			TCU_FAIL("Type not found.");
		}
	}
}

/* Generates the shader source code for the InteractionArgumentAliasing4
 * array tests, and attempts to compile each test shader, for both
 * vertex and fragment shaders.
 *
 * @tparam API               Tested API descriptor
 *
 * @param tested_shader_type The type of shader that is being tested
 *                           (either TestCaseBase<API>::VERTEX_SHADER_TYPE or TestCaseBase<API>::FRAGMENT_SHADER_TYPE).
 */
template <class API>
void InteractionArgumentAliasing6<API>::test_shader_compilation(
	typename TestCaseBase<API>::TestShaderType tested_shader_type)
{
	static const glcts::test_var_type var_types_set_es[] = { VAR_TYPE_INT, VAR_TYPE_FLOAT, VAR_TYPE_MAT4 };
	static const size_t				  num_var_types_es   = sizeof(var_types_set_es) / sizeof(var_types_set_es[0]);

	static const glcts::test_var_type var_types_set_gl[] = { VAR_TYPE_INT, VAR_TYPE_FLOAT, VAR_TYPE_MAT4,
															 VAR_TYPE_DOUBLE, VAR_TYPE_DMAT4 };
	static const size_t num_var_types_gl = sizeof(var_types_set_gl) / sizeof(var_types_set_gl[0]);

	const std::string iteration_loop_end = "                }\n"
										   "            }\n"
										   "        }\n"
										   "    }\n";
	const std::string iteration_loop_start = "    for (uint a = 0u; a < 2u; a++)\n"
											 "    {\n"
											 "        for (uint b = 0u; b < 2u; b++)\n"
											 "        {\n"
											 "            for (uint c = 0u; c < 2u; c++)\n"
											 "            {\n"
											 "                for (uint d = 0u; d < 2u; d++)\n"
											 "                {\n";
	const glcts::test_var_type* var_types_set = var_types_set_es;
	size_t						num_var_types = num_var_types_es;
	const bool					test_compute  = (TestCaseBase<API>::COMPUTE_SHADER_TYPE == tested_shader_type);

	if (API::USE_DOUBLE)
	{
		var_types_set = var_types_set_gl;
		num_var_types = num_var_types_gl;
	}

	for (size_t var_type_index = 0; var_type_index < num_var_types; var_type_index++)
	{
		_supported_variable_types_map_const_iterator var_iterator =
			supported_variable_types_map.find(var_types_set[var_type_index]);

		if (var_iterator != supported_variable_types_map.end())
		{
			std::string array_declaration = var_iterator->second.type + "[2][2][2][2] z;\n\n";

			std::string function_definition;
			std::string function_use;
			std::string verification;

			function_definition += "bool gfunc(" + var_iterator->second.type + " x[2][2][2][2], ";
			function_definition += "inout " + var_iterator->second.type + " y[2][2][2][2])\n";
			function_definition += "{\n";
			function_definition += "    " + iteration_loop_start;
			function_definition +=
				"                                   y[a][b][c][d] = " + var_iterator->second.type +
				"(123);\n";
			function_definition += "    " + iteration_loop_end;
			function_definition += "\n";
			function_definition += "    " + iteration_loop_start;
			function_definition += "                                   if(x[a][b][c][d]";
			if (var_iterator->second.type == "mat4") // mat4 comparison
			{
				function_definition += "[0][0]";
				function_definition += " != float";
			}
			else if (var_iterator->second.type == "dmat4") // dmat4 comparison
			{
				function_definition += "[0][0]";
				function_definition += " != double";
			}
			else
			{
				function_definition += " != ";
				function_definition += var_iterator->second.type;
			}
			function_definition += "(((a*8u)+(b*4u)+(c*2u)+d))) {return false;}\n";
			function_definition += "    " + iteration_loop_end;
			function_definition += "  return true;\n";
			function_definition += "}\n\n";

			function_use += "    " + array_declaration;
			function_use += "    " + iteration_loop_start;
			function_use += "                                   z[a][b][c][d] = ";
			function_use += var_iterator->second.type;
			function_use += "(((a*8u)+(b*4u)+(c*2u)+d));\n";
			function_use += "    " + iteration_loop_end;

			verification += "    float result = 0.0;\n";
			verification += "    if(gfunc(z, z) == true)\n";
			verification += "    {\n";
			verification += "        result = 1.0;\n\n";
			verification += "    }\n";
			verification += "    else\n";
			verification += "    {\n";
			verification += "        result = 0.0;\n\n";
			verification += "    }\n";

			if (false == test_compute)
			{
				this->execute_draw_test(tested_shader_type, function_definition, function_use, verification);
			}
			else
			{
				this->execute_dispatch_test(tested_shader_type, function_definition, function_use, verification);
			}

			/* Deallocate any resources used. */
			this->delete_objects();
		} /* if var_type iterator found */
		else
		{
			TCU_FAIL("Type not found.");
		}
	}
}

/* Generates the shader source code for the InteractionUniforms1
 * array tests, and attempts to compile each test shader, for both
 * vertex and fragment shaders.
 *
 * @tparam API               Tested API descriptor
 *
 * @param tested_shader_type The type of shader that is being tested
 *                           (either TestCaseBase<API>::VERTEX_SHADER_TYPE or TestCaseBase<API>::FRAGMENT_SHADER_TYPE).
 */
template <class API>
void InteractionUniforms1<API>::test_shader_compilation(typename TestCaseBase<API>::TestShaderType tested_shader_type)
{
	static const glcts::test_var_type var_types_set_es[] = { VAR_TYPE_FLOAT, VAR_TYPE_INT, VAR_TYPE_UINT };
	static const size_t				  num_var_types_es   = sizeof(var_types_set_es) / sizeof(var_types_set_es[0]);

	static const glcts::test_var_type var_types_set_gl[] = { VAR_TYPE_FLOAT, VAR_TYPE_INT, VAR_TYPE_UINT,
															 VAR_TYPE_DOUBLE };
	static const size_t num_var_types_gl = sizeof(var_types_set_gl) / sizeof(var_types_set_gl[0]);

	const glw::Functions&		gl			  = this->context_id.getRenderContext().getFunctions();
	const glcts::test_var_type* var_types_set = var_types_set_es;
	size_t						num_var_types = num_var_types_es;

	if (API::USE_DOUBLE)
	{
		var_types_set = var_types_set_gl;
		num_var_types = num_var_types_gl;
	}

	for (size_t var_type_index = 0; var_type_index < num_var_types; var_type_index++)
	{
		_supported_variable_types_map_const_iterator var_iterator =
			supported_variable_types_map.find(var_types_set[var_type_index]);

		if (var_iterator != supported_variable_types_map.end())
		{
			std::string uniform_definition;
			std::string uniform_use;

			uniform_definition += "uniform ";
			uniform_definition += var_iterator->second.precision;
			uniform_definition += " ";
			uniform_definition += var_iterator->second.type;
			uniform_definition += " my_uniform_1[1][1][1][1];\n\n";

			uniform_use = "    float result = float(my_uniform_1[0][0][0][0]);\n";

			if (API::USE_ALL_SHADER_STAGES)
			{
				const std::string& compute_shader_source =
					this->prepare_compute_shader(tested_shader_type, uniform_definition, uniform_use);
				const std::string& fragment_shader_source =
					this->prepare_fragment_shader(tested_shader_type, uniform_definition, uniform_use);
				const std::string& geometry_shader_source =
					this->prepare_geometry_shader(tested_shader_type, uniform_definition, uniform_use);
				const std::string& tess_ctrl_shader_source =
					this->prepare_tess_ctrl_shader(tested_shader_type, uniform_definition, uniform_use);
				const std::string& tess_eval_shader_source =
					this->prepare_tess_eval_shader(tested_shader_type, uniform_definition, uniform_use);
				const std::string& vertex_shader_source =
					this->prepare_vertex_shader(tested_shader_type, uniform_definition, uniform_use);

				switch (tested_shader_type)
				{
				case TestCaseBase<API>::VERTEX_SHADER_TYPE: /* Fall through */
				case TestCaseBase<API>::FRAGMENT_SHADER_TYPE:
					this->execute_positive_test(vertex_shader_source, fragment_shader_source, false, false);
					break;

				case TestCaseBase<API>::COMPUTE_SHADER_TYPE:
				case TestCaseBase<API>::GEOMETRY_SHADER_TYPE:
				case TestCaseBase<API>::TESSELATION_CONTROL_SHADER_TYPE: /* Fall through */
				case TestCaseBase<API>::TESSELATION_EVALUATION_SHADER_TYPE:
					this->execute_positive_test(vertex_shader_source, tess_ctrl_shader_source, tess_eval_shader_source,
												geometry_shader_source, fragment_shader_source, compute_shader_source,
												false, false);
					break;

				default:
					TCU_FAIL("Invalid enum");
					break;
				}
			}
			else
			{
				const std::string& fragment_shader_source =
					this->prepare_fragment_shader(tested_shader_type, uniform_definition, uniform_use);
				const std::string& vertex_shader_source =
					this->prepare_vertex_shader(tested_shader_type, uniform_definition, uniform_use);

				this->execute_positive_test(vertex_shader_source, fragment_shader_source, false, false);
			}

			glw::GLint uniform_location = -1;

			/* Make program object active. */
			gl.useProgram(this->program_object_id);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram() failed.");

			/* Get uniform location. */
			uniform_location = gl.getUniformLocation(this->program_object_id, "my_uniform_1[0][0][0][0]");
			GLU_EXPECT_NO_ERROR(gl.getError(), "glGetUniformLocation() failed.");

			if (uniform_location == -1)
			{
				TCU_FAIL("Uniform is not found or is considered as not active.");
			}

			switch (var_type_index)
			{
			case 0: //float type of uniform is considered
			{
				glw::GLfloat uniform_value = 1.0f;

				gl.uniform1f(uniform_location, uniform_value);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glUniform1f() failed.");

				break;
			}
			case 1: //int type of uniform is considered
			{
				glw::GLint uniform_value = 1;

				gl.uniform1i(uniform_location, uniform_value);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glUniform1i() failed.");

				break;
			}
			case 2: //uint type of uniform is considered
			{
				glw::GLuint uniform_value = 1;

				gl.uniform1ui(uniform_location, uniform_value);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glUniform1ui() failed.");

				break;
			}
			case 3: //double type of uniform is considered
			{
				glw::GLdouble uniform_value = 1.0;

				gl.uniform1d(uniform_location, uniform_value);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glUniform1d() failed.");

				break;
			}
			default:
			{
				TCU_FAIL("Invalid variable-type index.");

				break;
			}
			} /* switch (var_type_index) */

			/* Deallocate any resources used. */
			this->delete_objects();
		} /* if var_type iterator found */
		else
		{
			TCU_FAIL("Type not found.");
		}
	} /* for (int var_type_index = 0; ...) */
}

/** Prepare shader
 *
 * @tparam API               Tested API descriptor
 *
 * @param tested_shader_type The type of shader that is being tested
 * @param uniform_definition Definition used to prepare shader
 * @param uniform_use        Snippet that use defined uniform
 **/
template <class API>
std::string InteractionUniforms1<API>::prepare_compute_shader(
	typename TestCaseBase<API>::TestShaderType tested_shader_type, const std::string& uniform_definition,
	const std::string& uniform_use)
{
	std::string compute_shader_source;

	if (TestCaseBase<API>::COMPUTE_SHADER_TYPE == tested_shader_type)
	{
		compute_shader_source = "writeonly uniform image2D uni_image;\n"
								"\n";

		/* User-defined function definition. */
		compute_shader_source += uniform_definition;
		compute_shader_source += "\n\n";

		/* Main function definition. */
		compute_shader_source += shader_start;
		compute_shader_source += uniform_use;
		compute_shader_source += "\n\n";
		compute_shader_source += "\n"
								 "    imageStore(uni_image, ivec2(gl_GlobalInvocationID.xy), vec4(result, 0, 0, 0));\n"
								 "}\n"
								 "\n";
	}

	return compute_shader_source;
}

/** Prepare shader
 *
 * @tparam API               Tested API descriptor
 *
 * @param tested_shader_type The type of shader that is being tested
 * @param uniform_definition Definition used to prepare shader
 * @param uniform_use        Snippet that use defined uniform
 **/
template <class API>
std::string InteractionUniforms1<API>::prepare_fragment_shader(
	typename TestCaseBase<API>::TestShaderType tested_shader_type, const std::string& uniform_definition,
	const std::string& uniform_use)
{
	std::string fragment_shader_source;

	switch (tested_shader_type)
	{
	case TestCaseBase<API>::COMPUTE_SHADER_TYPE:
		break;

	case TestCaseBase<API>::FRAGMENT_SHADER_TYPE:
		fragment_shader_source = "out vec4 colour;\n\n";

		/* User-defined function definition. */
		fragment_shader_source += uniform_definition;
		fragment_shader_source += "\n\n";

		/* Main function definition. */
		fragment_shader_source += shader_start;
		fragment_shader_source += uniform_use;
		fragment_shader_source += "\n\n";
		fragment_shader_source += "    colour = vec4(result);\n";
		fragment_shader_source += shader_end;
		break;

	case TestCaseBase<API>::GEOMETRY_SHADER_TYPE:
	case TestCaseBase<API>::TESSELATION_CONTROL_SHADER_TYPE:
	case TestCaseBase<API>::TESSELATION_EVALUATION_SHADER_TYPE:
		fragment_shader_source = "in float fs_result;\n\n"
								 "out vec4 colour;\n\n"
								 "void main()\n"
								 "{\n"
								 "    colour =  vec4(fs_result);\n"
								 "}\n"
								 "\n";
		break;

	case TestCaseBase<API>::VERTEX_SHADER_TYPE:
		fragment_shader_source = default_fragment_shader_source;
		break;

	default:
		TCU_FAIL("Unrecognized shader object type.");
		break;
	}

	return fragment_shader_source;
}

/** Prepare shader
 *
 * @tparam API               Tested API descriptor
 *
 * @param tested_shader_type The type of shader that is being tested
 * @param uniform_definition Definition used to prepare shader
 * @param uniform_use        Snippet that use defined uniform
 **/
template <class API>
std::string InteractionUniforms1<API>::prepare_geometry_shader(
	typename TestCaseBase<API>::TestShaderType tested_shader_type, const std::string& uniform_definition,
	const std::string& uniform_use)
{
	std::string geometry_shader_source;

	switch (tested_shader_type)
	{
	case TestCaseBase<API>::COMPUTE_SHADER_TYPE:
	case TestCaseBase<API>::FRAGMENT_SHADER_TYPE:
	case TestCaseBase<API>::VERTEX_SHADER_TYPE:
		break;

	case TestCaseBase<API>::TESSELATION_CONTROL_SHADER_TYPE:
	case TestCaseBase<API>::TESSELATION_EVALUATION_SHADER_TYPE:
		geometry_shader_source = "layout(points)                           in;\n"
								 "layout(triangle_strip, max_vertices = 4) out;\n"
								 "\n"
								 "in  float tes_result[];\n"
								 "out float fs_result;\n"
								 "\n"
								 "void main()\n"
								 "{\n"
								 "    gl_Position  = vec4(-1, -1, 0, 1);\n"
								 "    fs_result    = tes_result[0];\n"
								 "    EmitVertex();\n"
								 "    gl_Position  = vec4(-1, 1, 0, 1);\n"
								 "    fs_result    = tes_result[0];\n"
								 "    EmitVertex();\n"
								 "    gl_Position  = vec4(1, -1, 0, 1);\n"
								 "    fs_result    = tes_result[0];\n"
								 "    EmitVertex();\n"
								 "    gl_Position  = vec4(1, 1, 0, 1);\n"
								 "    fs_result    = tes_result[0];\n"
								 "    EmitVertex();\n"
								 "}\n";
		break;

	case TestCaseBase<API>::GEOMETRY_SHADER_TYPE:
		geometry_shader_source = "layout(points)                           in;\n"
								 "layout(triangle_strip, max_vertices = 4) out;\n"
								 "\n"
								 "out float fs_result;\n"
								 "\n";

		/* User-defined function definition. */
		geometry_shader_source += uniform_definition;
		geometry_shader_source += "\n\n";

		/* Main function definition. */
		geometry_shader_source += shader_start;
		geometry_shader_source += uniform_use;
		geometry_shader_source += "\n\n";
		geometry_shader_source += "\n    gl_Position  = vec4(-1, -1, 0, 1);\n"
								  "    fs_result    = result;\n"
								  "    EmitVertex();\n"
								  "    gl_Position  = vec4(-1, 1, 0, 1);\n"
								  "    fs_result    = result;\n"
								  "    EmitVertex();\n"
								  "    gl_Position  = vec4(1, -1, 0, 1);\n"
								  "    fs_result    = result;\n"
								  "    EmitVertex();\n"
								  "    gl_Position  = vec4(1, 1, 0, 1);\n"
								  "    fs_result    = result;\n"
								  "    EmitVertex();\n"
								  "}\n";
		break;

	default:
		TCU_FAIL("Unrecognized shader object type.");
		break;
	}

	return geometry_shader_source;
}

/** Prepare shader
 *
 * @tparam API               Tested API descriptor
 *
 * @param tested_shader_type The type of shader that is being tested
 * @param uniform_definition Definition used to prepare shader
 * @param uniform_use        Snippet that use defined uniform
 **/
template <class API>
std::string InteractionUniforms1<API>::prepare_tess_ctrl_shader(
	typename TestCaseBase<API>::TestShaderType tested_shader_type, const std::string& uniform_definition,
	const std::string& uniform_use)
{
	std::string tess_ctrl_shader_source;

	switch (tested_shader_type)
	{
	case TestCaseBase<API>::COMPUTE_SHADER_TYPE:
	case TestCaseBase<API>::FRAGMENT_SHADER_TYPE:
	case TestCaseBase<API>::GEOMETRY_SHADER_TYPE:
	case TestCaseBase<API>::VERTEX_SHADER_TYPE:
		break;

	case TestCaseBase<API>::TESSELATION_CONTROL_SHADER_TYPE:
		tess_ctrl_shader_source = "layout(vertices = 1) out;\n"
								  "\n"
								  "out float tcs_result[];\n"
								  "\n";

		/* User-defined function definition. */
		tess_ctrl_shader_source += uniform_definition;
		tess_ctrl_shader_source += "\n\n";

		/* Main function definition. */
		tess_ctrl_shader_source += shader_start;
		tess_ctrl_shader_source += uniform_use;
		tess_ctrl_shader_source += "\n\n";
		tess_ctrl_shader_source += "    tcs_result[gl_InvocationID] = result;\n"
								   "\n"
								   "    gl_TessLevelOuter[0] = 1.0;\n"
								   "    gl_TessLevelOuter[1] = 1.0;\n"
								   "    gl_TessLevelOuter[2] = 1.0;\n"
								   "    gl_TessLevelOuter[3] = 1.0;\n"
								   "    gl_TessLevelInner[0] = 1.0;\n"
								   "    gl_TessLevelInner[1] = 1.0;\n"
								   "}\n";
		break;

	case TestCaseBase<API>::TESSELATION_EVALUATION_SHADER_TYPE:
		tess_ctrl_shader_source = default_tc_shader_source;
		break;

	default:
		TCU_FAIL("Unrecognized shader object type.");
		break;
	}

	return tess_ctrl_shader_source;
}

/** Prepare shader
 *
 * @tparam API               Tested API descriptor
 *
 * @param tested_shader_type The type of shader that is being tested
 * @param uniform_definition Definition used to prepare shader
 * @param uniform_use        Snippet that use defined uniform
 **/
template <class API>
std::string InteractionUniforms1<API>::prepare_tess_eval_shader(
	typename TestCaseBase<API>::TestShaderType tested_shader_type, const std::string& uniform_definition,
	const std::string& uniform_use)
{
	std::string tess_eval_shader_source;

	switch (tested_shader_type)
	{
	case TestCaseBase<API>::COMPUTE_SHADER_TYPE:
	case TestCaseBase<API>::FRAGMENT_SHADER_TYPE:
	case TestCaseBase<API>::GEOMETRY_SHADER_TYPE:
	case TestCaseBase<API>::VERTEX_SHADER_TYPE:
		break;

	case TestCaseBase<API>::TESSELATION_CONTROL_SHADER_TYPE:
		tess_eval_shader_source = "layout(isolines, point_mode) in;\n"
								  "\n"
								  "in  float tcs_result[];\n"
								  "out float tes_result;\n"
								  "\n"
								  "void main()\n"
								  "{\n"
								  "    tes_result = tcs_result[0];\n"
								  "}\n";
		break;

	case TestCaseBase<API>::TESSELATION_EVALUATION_SHADER_TYPE:
		tess_eval_shader_source = "layout(isolines, point_mode) in;\n"
								  "\n"
								  "out float tes_result;\n"
								  "\n";

		/* User-defined function definition. */
		tess_eval_shader_source += uniform_definition;
		tess_eval_shader_source += "\n\n";

		/* Main function definition. */
		tess_eval_shader_source += shader_start;
		tess_eval_shader_source += uniform_use;
		tess_eval_shader_source += "\n\n";
		tess_eval_shader_source += "    tes_result = result;\n"
								   "}\n";
		break;

	default:
		TCU_FAIL("Unrecognized shader object type.");
		break;
	}

	return tess_eval_shader_source;
}

/** Prepare shader
 *
 * @tparam API               Tested API descriptor
 *
 * @param tested_shader_type The type of shader that is being tested
 * @param uniform_definition Definition used to prepare shader
 * @param uniform_use        Snippet that use defined uniform
 **/
template <class API>
std::string InteractionUniforms1<API>::prepare_vertex_shader(
	typename TestCaseBase<API>::TestShaderType tested_shader_type, const std::string& uniform_definition,
	const std::string& uniform_use)
{
	std::string vertex_shader_source;

	switch (tested_shader_type)
	{
	case TestCaseBase<API>::COMPUTE_SHADER_TYPE:
		break;

	case TestCaseBase<API>::FRAGMENT_SHADER_TYPE:
	case TestCaseBase<API>::GEOMETRY_SHADER_TYPE:
	case TestCaseBase<API>::TESSELATION_CONTROL_SHADER_TYPE:
	case TestCaseBase<API>::TESSELATION_EVALUATION_SHADER_TYPE:
		vertex_shader_source = default_vertex_shader_source;
		break;

	case TestCaseBase<API>::VERTEX_SHADER_TYPE:
		/* User-defined function definition. */
		vertex_shader_source += uniform_definition;

		/* Main function definition. */
		vertex_shader_source += shader_start;
		vertex_shader_source += uniform_use;
		vertex_shader_source += "    gl_Position = vec4(result);\n";
		vertex_shader_source += shader_end;
		break;

	default:
		TCU_FAIL("Unrecognized shader object type.");
		break;
	}

	return vertex_shader_source;
}

/* Generates the shader source code for the InteractionUniforms2
 * array tests, and attempts to compile each test shader, for both
 * vertex and fragment shaders.
 *
 * @tparam API               Tested API descriptor
 *
 * @param tested_shader_type The type of shader that is being tested
 *                           (either TestCaseBase<API>::VERTEX_SHADER_TYPE or TestCaseBase<API>::FRAGMENT_SHADER_TYPE).
 */
template <class API>
void InteractionUniforms2<API>::test_shader_compilation(typename TestCaseBase<API>::TestShaderType tested_shader_type)
{
	static const glcts::test_var_type var_types_set_es[] = { VAR_TYPE_INT, VAR_TYPE_FLOAT, VAR_TYPE_MAT4 };
	static const size_t				  num_var_types_es   = sizeof(var_types_set_es) / sizeof(var_types_set_es[0]);

	static const glcts::test_var_type var_types_set_gl[] = { VAR_TYPE_INT, VAR_TYPE_FLOAT, VAR_TYPE_MAT4,
															 VAR_TYPE_DOUBLE, VAR_TYPE_DMAT4 };
	static const size_t num_var_types_gl = sizeof(var_types_set_gl) / sizeof(var_types_set_gl[0]);

	const std::string array_initializers[] = { "int[2][2][2][2](\n"
											   "    int[2][2][2](\n"
											   "        int[2][2](\n"
											   "            int[2]( 1,  2),\n"
											   "            int[2]( 3,  4)\n"
											   "        ),\n"
											   "        int[2][2](\n"
											   "            int[2]( 5,  6),\n"
											   "            int[2]( 7,  8)\n"
											   "        )\n"
											   "    ),\n"
											   "    int[2][2][2](\n"
											   "        int[2][2](\n"
											   "            int[2](11, 12),\n"
											   "            int[2](13, 14)\n"
											   "        ),\n"
											   "        int[2][2](\n"
											   "            int[2](15, 16),\n"
											   "            int[2](17, 18)\n"
											   "        )\n"
											   "    )\n"
											   ")",

											   "float[2][2][2][2](\n"
											   "    float[2][2][2](\n"
											   "        float[2][2](\n"
											   "            float[2](1.0, 2.0),\n"
											   "            float[2](3.0, 4.0)),\n"
											   "        float[2][2](\n"
											   "            float[2](5.0, 6.0),\n"
											   "            float[2](7.0, 8.0))),\n"
											   "    float[2][2][2](\n"
											   "        float[2][2](\n"
											   "            float[2](1.1, 2.1),\n"
											   "            float[2](3.1, 4.1)\n"
											   "        ),\n"
											   "        float[2][2](\n"
											   "            float[2](5.1, 6.1),\n"
											   "            float[2](7.1, 8.1)\n"
											   "        )\n"
											   "    )\n"
											   ")",

											   "mat4[2][2][2][2](\n"
											   "    mat4[2][2][2](\n"
											   "        mat4[2][2](\n"
											   "            mat4[2]( mat4(1),  mat4(2)),\n"
											   "            mat4[2]( mat4(3),  mat4(4))\n"
											   "        ),\n"
											   "        mat4[2][2](\n"
											   "            mat4[2](mat4(5),  mat4(6)),\n"
											   "            mat4[2](mat4(7),  mat4(8))\n"
											   "        )\n"
											   "    ),\n"
											   "    mat4[2][2][2](\n"
											   "        mat4[2][2](\n"
											   "            mat4[2](mat4(9),  mat4(10)),\n"
											   "            mat4[2](mat4(11),  mat4(12))\n"
											   "        ),\n"
											   "        mat4[2][2](\n"
											   "            mat4[2](mat4(13),  mat4(14)),\n"
											   "            mat4[2](mat4(15),  mat4(16))\n"
											   "        )\n"
											   "    )\n"
											   ")",

											   "double[2][2][2][2](\n"
											   "    double[2][2][2](\n"
											   "        double[2][2](\n"
											   "            double[2](1.0, 2.0),\n"
											   "            double[2](3.0, 4.0)),\n"
											   "        double[2][2](\n"
											   "            double[2](5.0, 6.0),\n"
											   "            double[2](7.0, 8.0))),\n"
											   "    double[2][2][2](\n"
											   "        double[2][2](\n"
											   "            double[2](1.1, 2.1),\n"
											   "            double[2](3.1, 4.1)\n"
											   "        ),\n"
											   "        double[2][2](\n"
											   "            double[2](5.1, 6.1),\n"
											   "            double[2](7.1, 8.1)\n"
											   "        )\n"
											   "    )\n"
											   ")",

											   "dmat4[2][2][2][2](\n"
											   "    dmat4[2][2][2](\n"
											   "        dmat4[2][2](\n"
											   "            dmat4[2]( dmat4(1),  dmat4(2)),\n"
											   "            dmat4[2]( dmat4(3),  dmat4(4))\n"
											   "        ),\n"
											   "        dmat4[2][2](\n"
											   "            dmat4[2](dmat4(5),  dmat4(6)),\n"
											   "            dmat4[2](dmat4(7),  dmat4(8))\n"
											   "        )\n"
											   "    ),\n"
											   "    dmat4[2][2][2](\n"
											   "        dmat4[2][2](\n"
											   "            dmat4[2](dmat4(9),   dmat4(10)),\n"
											   "            dmat4[2](dmat4(11),  dmat4(12))\n"
											   "        ),\n"
											   "        dmat4[2][2](\n"
											   "            dmat4[2](dmat4(13),  dmat4(14)),\n"
											   "            dmat4[2](dmat4(15),  dmat4(16))\n"
											   "        )\n"
											   "    )\n"
											   ")" };

	const glcts::test_var_type* var_types_set = var_types_set_es;
	size_t						num_var_types = num_var_types_es;

	if (API::USE_DOUBLE)
	{
		var_types_set = var_types_set_gl;
		num_var_types = num_var_types_gl;
	}

	for (size_t var_type_index = 0; var_type_index < num_var_types; var_type_index++)
	{
		_supported_variable_types_map_const_iterator var_iterator =
			supported_variable_types_map.find(var_types_set[var_type_index]);

		if (var_iterator != supported_variable_types_map.end())
		{
			std::string base_variable_string;

			for (int initialiser_selector = 1; initialiser_selector >= 0; initialiser_selector--)
			{
				// We normally do all 16 possible permutations of [4][4][4][4] items (15..0).
				// However, in this case we will skip the case that will work,
				// so we'll merely process permutations 14..0
				for (int permutation_index = 14; permutation_index >= 0; permutation_index--)
				{
					base_variable_string =
						"uniform " + var_iterator->second.precision + " " + var_iterator->second.type + " x";

					// for all 4 possible sub_script entries
					for (int sub_script_entry_index = 3; sub_script_entry_index >= 0; sub_script_entry_index--)
					{
						if (permutation_index & (1 << sub_script_entry_index))
						{
							// In this case, we'll use a valid sub_script
							base_variable_string += "[2]";
						}
						else
						{
							// In this case, we'll use an invalid sub_script
							base_variable_string += "[]";
						}
					}

					if (initialiser_selector == 0)
					{
						// We'll use an initialiser
						base_variable_string += " = " + array_initializers[var_type_index];
					}

					base_variable_string += ";\n\n";

					std::string shader_source = base_variable_string + shader_start;

					/* End main */
					DEFAULT_MAIN_ENDING(tested_shader_type, shader_source);

					/* Execute test:
					 *
					 * This will succeed in case of allowed unsized
					 * declarations and when at least one of these is
					 * true:
					 *   1. There is an initialiser.
					 *   2. Only the outermost dimension is unsized,
					 *      as in [][2][2][2].
					 */
					EXECUTE_SHADER_TEST(API::ALLOW_UNSIZED_DECLARATION &&
											(initialiser_selector == 0 || permutation_index == 7),
										tested_shader_type, shader_source);
				} /* for (int permutation_index = 14; ...) */
			}	 /* for (int initialiser_selector  = 1; ...) */
		}		  /* if var_type iterator found */
		else
		{
			TCU_FAIL("Type not found.");
		}
	} /* for (int var_type_index = 0; ...) */
}

/* Generates the shader source code for the InteractionUniformBuffers1
 * array tests, and attempts to compile each test shader, for both
 * vertex and fragment shaders.
 *
 * @tparam API               Tested API descriptor
 *
 * @param tested_shader_type The type of shader that is being tested
 *                           (either TestCaseBase<API>::VERTEX_SHADER_TYPE or TestCaseBase<API>::FRAGMENT_SHADER_TYPE).
 */
template <class API>
void InteractionUniformBuffers1<API>::test_shader_compilation(
	typename TestCaseBase<API>::TestShaderType tested_shader_type)
{
	static const glcts::test_var_type var_types_set_es[] = { VAR_TYPE_FLOAT, VAR_TYPE_INT, VAR_TYPE_UINT };
	static const size_t				  num_var_types_es   = sizeof(var_types_set_es) / sizeof(var_types_set_es[0]);

	static const glcts::test_var_type var_types_set_gl[] = { VAR_TYPE_FLOAT, VAR_TYPE_INT, VAR_TYPE_UINT,
															 VAR_TYPE_DOUBLE };
	static const size_t num_var_types_gl = sizeof(var_types_set_gl) / sizeof(var_types_set_gl[0]);

	const glcts::test_var_type* var_types_set = var_types_set_es;
	size_t						num_var_types = num_var_types_es;

	if (API::USE_DOUBLE)
	{
		var_types_set = var_types_set_gl;
		num_var_types = num_var_types_gl;
	}

	for (size_t var_type_index = 0; var_type_index < num_var_types; var_type_index++)
	{
		_supported_variable_types_map_const_iterator var_iterator =
			supported_variable_types_map.find(var_types_set[var_type_index]);

		if (var_iterator != supported_variable_types_map.end())
		{
			std::string shader_source;

			shader_source += "uniform uBlocka {\n";
			shader_source += "    " + var_iterator->second.type + " x[1][1][1][1][1][1];\n";
			shader_source += "};\n\n";
			shader_source += shader_start;

			/* End main */
			DEFAULT_MAIN_ENDING(tested_shader_type, shader_source);

			/* Execute test */
			EXECUTE_POSITIVE_TEST(tested_shader_type, shader_source, true, false);
		} /* if var_type iterator found */
		else
		{
			TCU_FAIL("Type not found.");
		}
	}
}

/* Generates the shader source code for the InteractionUniformBuffers2
 * array tests, and attempts to compile each test shader, for both
 * vertex and fragment shaders.
 *
 * @tparam API               Tested API descriptor
 *
 * @param tested_shader_type The type of shader that is being tested
 *                           (either TestCaseBase<API>::VERTEX_SHADER_TYPE or TestCaseBase<API>::FRAGMENT_SHADER_TYPE).
 */
template <class API>
void InteractionUniformBuffers2<API>::test_shader_compilation(
	typename TestCaseBase<API>::TestShaderType tested_shader_type)
{
	static const glcts::test_var_type var_types_set_es[] = { VAR_TYPE_FLOAT, VAR_TYPE_INT, VAR_TYPE_UINT };
	static const size_t				  num_var_types_es   = sizeof(var_types_set_es) / sizeof(var_types_set_es[0]);

	static const glcts::test_var_type var_types_set_gl[] = { VAR_TYPE_FLOAT, VAR_TYPE_INT, VAR_TYPE_UINT,
															 VAR_TYPE_DOUBLE };
	static const size_t num_var_types_gl = sizeof(var_types_set_gl) / sizeof(var_types_set_gl[0]);

	const glw::Functions&		gl			  = this->context_id.getRenderContext().getFunctions();
	const glcts::test_var_type* var_types_set = var_types_set_es;
	size_t						num_var_types = num_var_types_es;

	if (API::USE_DOUBLE)
	{
		var_types_set = var_types_set_gl;
		num_var_types = num_var_types_gl;
	}

	/* Iterate through float / int / uint values. */
	for (size_t var_type_index = 0; var_type_index < num_var_types; var_type_index++)
	{
		_supported_variable_types_map_const_iterator var_iterator =
			supported_variable_types_map.find(var_types_set[var_type_index]);

		if (var_iterator != supported_variable_types_map.end())
		{
			std::string uniform_definition;
			std::string uniform_use;

			uniform_definition += "layout (std140) uniform uniform_block_name\n"
								  "{\n";
			uniform_definition += "    ";
			uniform_definition += var_iterator->second.type;
			uniform_definition += " my_uniform_1[1][1][1][1];\n"
								  "};\n";

			uniform_use = "    float result = float(my_uniform_1[0][0][0][0]);\n";

			if (API::USE_ALL_SHADER_STAGES)
			{
				const std::string& compute_shader_source =
					this->prepare_compute_shader(tested_shader_type, uniform_definition, uniform_use);
				const std::string& fragment_shader_source =
					this->prepare_fragment_shader(tested_shader_type, uniform_definition, uniform_use);
				const std::string& geometry_shader_source =
					this->prepare_geometry_shader(tested_shader_type, uniform_definition, uniform_use);
				const std::string& tess_ctrl_shader_source =
					this->prepare_tess_ctrl_shader(tested_shader_type, uniform_definition, uniform_use);
				const std::string& tess_eval_shader_source =
					this->prepare_tess_eval_shader(tested_shader_type, uniform_definition, uniform_use);
				const std::string& vertex_shader_source =
					this->prepare_vertex_shader(tested_shader_type, uniform_definition, uniform_use);

				switch (tested_shader_type)
				{
				case TestCaseBase<API>::VERTEX_SHADER_TYPE: /* Fall through */
				case TestCaseBase<API>::FRAGMENT_SHADER_TYPE:
					this->execute_positive_test(vertex_shader_source, fragment_shader_source, false, false);
					break;

				case TestCaseBase<API>::COMPUTE_SHADER_TYPE:
				case TestCaseBase<API>::GEOMETRY_SHADER_TYPE:
				case TestCaseBase<API>::TESSELATION_CONTROL_SHADER_TYPE: /* Fall through */
				case TestCaseBase<API>::TESSELATION_EVALUATION_SHADER_TYPE:
					this->execute_positive_test(vertex_shader_source, tess_ctrl_shader_source, tess_eval_shader_source,
												geometry_shader_source, fragment_shader_source, compute_shader_source,
												false, false);
					break;

				default:
					TCU_FAIL("Invalid enum");
					break;
				}
			}
			else
			{
				const std::string& fragment_shader_source =
					this->prepare_fragment_shader(tested_shader_type, uniform_definition, uniform_use);
				const std::string& vertex_shader_source =
					this->prepare_vertex_shader(tested_shader_type, uniform_definition, uniform_use);

				this->execute_positive_test(vertex_shader_source, fragment_shader_source, false, false);
			}

			glw::GLuint buffer_object_id	   = 0;
			glw::GLint  my_uniform_block_index = GL_INVALID_INDEX;

			gl.useProgram(this->program_object_id);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram() failed.");

			my_uniform_block_index = gl.getUniformBlockIndex(this->program_object_id, "uniform_block_name");
			GLU_EXPECT_NO_ERROR(gl.getError(), "glGetUniformBlockIndex() failed.");

			if ((unsigned)my_uniform_block_index == GL_INVALID_INDEX)
			{
				TCU_FAIL("Uniform block not found or is considered as not active.");
			}

			gl.genBuffers(1, &buffer_object_id);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glGenBuffers() failed.");

			gl.bindBuffer(GL_UNIFORM_BUFFER, buffer_object_id);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer() failed.");

			switch (var_type_index)
			{
			case 0: //float type of uniform is considered
			{
				glw::GLfloat buffer_data[] = { 0.0f, 1.0f, 2.0f,  3.0f,  4.0f,  5.0f,  6.0f,  7.0f,
											   8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f };

				gl.bufferData(GL_UNIFORM_BUFFER, sizeof(buffer_data), buffer_data, GL_STATIC_DRAW);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferData() failed.");

				break;
			}		/* float case */
			case 1: //int type of uniform is considered
			{

				glw::GLint buffer_data[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };

				gl.bufferData(GL_UNIFORM_BUFFER, sizeof(buffer_data), buffer_data, GL_STATIC_DRAW);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferData() failed.");

				break;
			}		/* int case */
			case 2: //uint type of uniform is considered
			{
				glw::GLuint buffer_data[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };

				gl.bufferData(GL_UNIFORM_BUFFER, sizeof(buffer_data), buffer_data, GL_STATIC_DRAW);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferData() failed.");

				break;
			}		/* uint case */
			case 3: //double type of uniform is considered
			{
				glw::GLdouble buffer_data[] = { 0.0, 1.0, 2.0,  3.0,  4.0,  5.0,  6.0,  7.0,
												8.0, 9.0, 10.0, 11.0, 12.0, 13.0, 14.0, 15.0 };

				gl.bufferData(GL_UNIFORM_BUFFER, sizeof(buffer_data), buffer_data, GL_STATIC_DRAW);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferData() failed.");

				break;
			} /* double case */
			default:
			{
				TCU_FAIL("Invalid variable-type index.");

				break;
			}
			} /* switch (var_type_index) */

			gl.uniformBlockBinding(this->program_object_id, my_uniform_block_index, 0);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glUniformBlockBinding() failed.");

			gl.bindBufferBase(GL_UNIFORM_BUFFER, 0, buffer_object_id);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBufferBase() failed.");

			if (TestCaseBase<API>::COMPUTE_SHADER_TYPE != tested_shader_type)
			{
				execute_draw_test(tested_shader_type);
			}
			else
			{
				execute_dispatch_test();
			}

			/* Deallocate any resources used. */
			gl.deleteBuffers(1, &buffer_object_id);
			this->delete_objects();
		} /* if var_type iterator found */
		else
		{
			TCU_FAIL("Type not found.");
		}
	} /* for (int var_type_index = 0; ...) */
}

/** Executes test for compute program
 *
 * @tparam API Tested API descriptor
 **/
template <class API>
void InteractionUniformBuffers2<API>::execute_dispatch_test()
{
	const glw::Functions& gl = this->context_id.getRenderContext().getFunctions();

	gl.dispatchCompute(1, 1, 1);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawArrays() failed.");
}

/** Executes test for draw program
 *
 * @tparam API               Tested API descriptor
 *
 * @param tested_shader_type The type of shader that is being tested
 **/
template <class API>
void InteractionUniformBuffers2<API>::execute_draw_test(typename TestCaseBase<API>::TestShaderType tested_shader_type)
{
	const glw::Functions& gl = this->context_id.getRenderContext().getFunctions();

	glw::GLuint vao_id = 0;

	gl.genVertexArrays(1, &vao_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenVertexArrays() failed.");

	gl.bindVertexArray(vao_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindVertexArray() failed.");

	switch (tested_shader_type)
	{
	case TestCaseBase<API>::FRAGMENT_SHADER_TYPE:
	case TestCaseBase<API>::VERTEX_SHADER_TYPE: /* Fall through */
	case TestCaseBase<API>::GEOMETRY_SHADER_TYPE:
		gl.drawArrays(GL_POINTS, 0, 1);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawArrays() failed.");
		break;

	case TestCaseBase<API>::TESSELATION_CONTROL_SHADER_TYPE: /* Fall through */
	case TestCaseBase<API>::TESSELATION_EVALUATION_SHADER_TYPE:
		/* Tesselation patch set up */
		gl.patchParameteri(GL_PATCH_VERTICES, 1);
		GLU_EXPECT_NO_ERROR(gl.getError(), "PatchParameteri");

		gl.drawArrays(GL_PATCHES, 0, 1);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawArrays() failed.");
		break;

	default:
		TCU_FAIL("Invalid enum");
		break;
	}

	gl.deleteVertexArrays(1, &vao_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glDeleteVertexArrays() failed.");
}

/* Generates the shader source code for the InteractionUniformBuffers3
 * array tests, and attempts to compile each test shader, for both
 * vertex and fragment shaders.
 *
 * @tparam API               Tested API descriptor
 *
 * @param tested_shader_type The type of shader that is being tested
 *                           (either TestCaseBase<API>::VERTEX_SHADER_TYPE or TestCaseBase<API>::FRAGMENT_SHADER_TYPE).
 */
template <class API>
void InteractionUniformBuffers3<API>::test_shader_compilation(
	typename TestCaseBase<API>::TestShaderType tested_shader_type)
{
	static const glcts::test_var_type var_types_set_es[] = { VAR_TYPE_FLOAT, VAR_TYPE_INT, VAR_TYPE_UINT };
	static const size_t				  num_var_types_es   = sizeof(var_types_set_es) / sizeof(var_types_set_es[0]);

	static const glcts::test_var_type var_types_set_gl[] = { VAR_TYPE_FLOAT, VAR_TYPE_INT, VAR_TYPE_UINT,
															 VAR_TYPE_DOUBLE };
	static const size_t num_var_types_gl = sizeof(var_types_set_gl) / sizeof(var_types_set_gl[0]);

	const std::string invalid_size_declarations[] = { "[2][2][2][]", "[2][2][][2]", "[2][][2][2]", "[][2][2][2]",
													  "[2][2][][]",  "[2][][2][]",  "[][2][2][]",  "[2][][][2]",
													  "[][2][][2]",  "[][][2][2]",  "[2][][][]",   "[][2][][]",
													  "[][][2][]",   "[][][][2]",   "[][][][]" };

	const std::string array_initializers[] = { "float[2][2][2][2](float[2][2][2](float[2][2](float[2](1.0, 2.0),"
											   "float[2](3.0, 4.0)),"
											   "float[2][2](float[2](5.0, 6.0),"
											   "float[2](7.0, 8.0))),"
											   "float[2][2][2](float[2][2](float[2](1.1, 2.1),"
											   "float[2](3.1, 4.1)),"
											   "float[2][2](float[2](5.1, 6.1),"
											   "float[2](7.1, 8.1))));\n",

											   "int[2][2][2][2](int[2][2][2](int[2][2](int[2]( 1,  2),"
											   "int[2]( 3,  4)),"
											   "int[2][2](int[2]( 5,  6),"
											   "int[2]( 7,  8))),"
											   "int[2][2][2](int[2][2](int[2](11, 12),"
											   "int[2](13, 14)),"
											   "int[2][2](int[2](15, 16),"
											   "int[2](17, 18))));\n",

											   "uint[2][2][2][2](uint[2][2][2](uint[2][2](uint[2]( 1u,  2u),"
											   "uint[2]( 3u,  4u)),"
											   "uint[2][2](uint[2]( 5u,  6u),"
											   "uint[2]( 7u,  8u))),"
											   "uint[2][2][2](uint[2][2](uint[2](11u, 12u),"
											   "uint[2](13u, 14u)),"
											   "uint[2][2](uint[2](15u, 16u),"
											   "uint[2](17u, 18u))));\n",

											   "double[2][2][2][2](double[2][2][2](double[2][2](double[2](1.0, 2.0),"
											   "double[2](3.0, 4.0)),"
											   "double[2][2](double[2](5.0, 6.0),"
											   "double[2](7.0, 8.0))),"
											   "double[2][2][2](double[2][2](double[2](1.1, 2.1),"
											   "double[2](3.1, 4.1)),"
											   "double[2][2](double[2](5.1, 6.1),"
											   "double[2](7.1, 8.1))));\n" };
	const glcts::test_var_type* var_types_set = var_types_set_es;
	size_t						num_var_types = num_var_types_es;

	if (API::USE_DOUBLE)
	{
		var_types_set = var_types_set_gl;
		num_var_types = num_var_types_gl;
	}

	/* Iterate through float/ int/ uint types.
	 * Case: without initializer.
	 */
	for (size_t var_type_index = 0; var_type_index < num_var_types; var_type_index++)
	{
		_supported_variable_types_map_const_iterator var_iterator =
			supported_variable_types_map.find(var_types_set[var_type_index]);

		if (var_iterator != supported_variable_types_map.end())
		{
			for (size_t invalid_size_declarations_index = 0;
				 invalid_size_declarations_index <
				 sizeof(invalid_size_declarations) / sizeof(invalid_size_declarations[0]);
				 invalid_size_declarations_index++)
			{
				std::string shader_source;

				shader_source = "layout (std140) uniform MyUniform {\n";
				shader_source += "    " + var_iterator->second.type +
								 invalid_size_declarations[invalid_size_declarations_index] + " my_variable;\n";
				shader_source += "};\n\n";
				shader_source += shader_start;

				/* End main */
				DEFAULT_MAIN_ENDING(tested_shader_type, shader_source);

				/* Execute test */
				EXECUTE_SHADER_TEST(API::ALLOW_UNSIZED_DECLARATION && invalid_size_declarations_index == 3,
									tested_shader_type, shader_source);
			} /* for (int invalid_size_declarations_index = 0; ...) */
		}
		else
		{
			TCU_FAIL("Type not found.");
		}
	} /* for (int var_type_index = 0; ...) */

	/* Iterate through float/ int/ uint types.
	 * Case: with initializer.
	 */
	for (size_t var_type_index = 0; var_type_index < num_var_types; var_type_index++)
	{
		_supported_variable_types_map_const_iterator var_iterator =
			supported_variable_types_map.find(var_types_set[var_type_index]);

		if (var_iterator != supported_variable_types_map.end())
		{
			for (size_t invalid_size_declarations_index = 0;
				 invalid_size_declarations_index <
				 sizeof(invalid_size_declarations) / sizeof(invalid_size_declarations[0]);
				 invalid_size_declarations_index++)
			{
				std::string shader_source;

				shader_source = "layout (std140) uniform MyUniform {\n";
				shader_source += "    " + var_iterator->second.type +
								 invalid_size_declarations[invalid_size_declarations_index] +
								 " my_variable = " + array_initializers[var_type_index];

				var_iterator->second.type + invalid_size_declarations[invalid_size_declarations_index] +
					" my_variable = " + array_initializers[var_type_index];
				shader_source += "};\n\n";
				shader_source += shader_start;

				/* End main */
				DEFAULT_MAIN_ENDING(tested_shader_type, shader_source);

				/* Execute test */
				this->execute_negative_test(tested_shader_type, shader_source);
			} /* for (int invalid_size_declarations_index = 0; ...) */
		}	 /* if var_type iterator found */
		else
		{
			TCU_FAIL("Type not found.");
		}
	} /* for (int var_type_index = 0; ...) */
}

/* Generates the shader source code for the InteractionStorageBuffers1
 * array tests, and attempts to compile each test shader, for both
 * vertex and fragment shaders.
 *
 * @tparam API               Tested API descriptor
 *
 * @param tested_shader_type The type of shader that is being tested
 *                           (either TestCaseBase<API>::VERTEX_SHADER_TYPE or TestCaseBase<API>::FRAGMENT_SHADER_TYPE).
 */
template <class API>
void InteractionStorageBuffers1<API>::test_shader_compilation(
	typename TestCaseBase<API>::TestShaderType tested_shader_type)
{
	static const glcts::test_var_type var_types_set_es[] = { VAR_TYPE_FLOAT, VAR_TYPE_INT, VAR_TYPE_UINT };
	static const size_t				  num_var_types_es   = sizeof(var_types_set_es) / sizeof(var_types_set_es[0]);

	static const glcts::test_var_type var_types_set_gl[] = { VAR_TYPE_FLOAT, VAR_TYPE_INT, VAR_TYPE_UINT,
															 VAR_TYPE_DOUBLE };
	static const size_t num_var_types_gl = sizeof(var_types_set_gl) / sizeof(var_types_set_gl[0]);

	const glcts::test_var_type* var_types_set = var_types_set_es;
	size_t						num_var_types = num_var_types_es;

	if (API::USE_DOUBLE)
	{
		var_types_set = var_types_set_gl;
		num_var_types = num_var_types_gl;
	}

	for (size_t var_type_index = 0; var_type_index < num_var_types; var_type_index++)
	{
		_supported_variable_types_map_const_iterator var_iterator =
			supported_variable_types_map.find(var_types_set[var_type_index]);

		if (var_iterator != supported_variable_types_map.end())
		{
			std::string shader_source;

			shader_source += "buffer uBlocka {\n";
			shader_source += "    " + var_iterator->second.type + " x[1][1][1][1][1][1];\n";
			shader_source += "};\n\n";
			shader_source += shader_start;

			/* End main */
			DEFAULT_MAIN_ENDING(tested_shader_type, shader_source);

			/* Execute test */
			EXECUTE_POSITIVE_TEST(tested_shader_type, shader_source, true, false);
		} /* if var_type iterator found */
		else
		{
			TCU_FAIL("Type not found.");
		}
	}
}

/* Generates the shader source code for the InteractionUniformBuffers2
 * array tests, and attempts to compile each test shader, for both
 * vertex and fragment shaders.
 *
 * @tparam API               Tested API descriptor
 *
 * @param tested_shader_type The type of shader that is being tested
 *                           (either TestCaseBase<API>::VERTEX_SHADER_TYPE or TestCaseBase<API>::FRAGMENT_SHADER_TYPE).
 */
template <class API>
void InteractionStorageBuffers2<API>::test_shader_compilation(
	typename TestCaseBase<API>::TestShaderType tested_shader_type)
{
	static const glcts::test_var_type var_types_set_es[] = { VAR_TYPE_FLOAT, VAR_TYPE_INT, VAR_TYPE_UINT };
	static const size_t				  num_var_types_es   = sizeof(var_types_set_es) / sizeof(var_types_set_es[0]);

	static const glcts::test_var_type var_types_set_gl[] = { VAR_TYPE_FLOAT, VAR_TYPE_INT, VAR_TYPE_UINT,
															 VAR_TYPE_DOUBLE };
	static const size_t num_var_types_gl = sizeof(var_types_set_gl) / sizeof(var_types_set_gl[0]);

	const glw::Functions&		gl			  = this->context_id.getRenderContext().getFunctions();
	const glcts::test_var_type* var_types_set = var_types_set_es;
	size_t						num_var_types = num_var_types_es;

	if (API::USE_DOUBLE)
	{
		var_types_set = var_types_set_gl;
		num_var_types = num_var_types_gl;
	}

	/* Iterate through float / int / uint values. */
	for (size_t var_type_index = 0; var_type_index < num_var_types; var_type_index++)
	{
		_supported_variable_types_map_const_iterator var_iterator =
			supported_variable_types_map.find(var_types_set[var_type_index]);

		if (var_iterator != supported_variable_types_map.end())
		{
			std::string uniform_definition;
			std::string uniform_use;

			uniform_definition += "layout (std140) buffer storage_block_name\n"
								  "{\n";
			uniform_definition += "    ";
			uniform_definition += var_iterator->second.type;
			uniform_definition += " my_storage_1[1][1][1][1];\n"
								  "};\n";

			uniform_use = "    float result = float(my_storage_1[0][0][0][0]);\n";

			if (API::USE_ALL_SHADER_STAGES)
			{
				const std::string& compute_shader_source =
					this->prepare_compute_shader(tested_shader_type, uniform_definition, uniform_use);
				const std::string& fragment_shader_source =
					this->prepare_fragment_shader(tested_shader_type, uniform_definition, uniform_use);
				const std::string& geometry_shader_source =
					this->prepare_geometry_shader(tested_shader_type, uniform_definition, uniform_use);
				const std::string& tess_ctrl_shader_source =
					this->prepare_tess_ctrl_shader(tested_shader_type, uniform_definition, uniform_use);
				const std::string& tess_eval_shader_source =
					this->prepare_tess_eval_shader(tested_shader_type, uniform_definition, uniform_use);
				const std::string& vertex_shader_source =
					this->prepare_vertex_shader(tested_shader_type, uniform_definition, uniform_use);

				switch (tested_shader_type)
				{
				case TestCaseBase<API>::VERTEX_SHADER_TYPE: /* Fall through */
				case TestCaseBase<API>::FRAGMENT_SHADER_TYPE:
					this->execute_positive_test(vertex_shader_source, fragment_shader_source, false, false);
					break;

				case TestCaseBase<API>::COMPUTE_SHADER_TYPE:
				case TestCaseBase<API>::GEOMETRY_SHADER_TYPE:
				case TestCaseBase<API>::TESSELATION_CONTROL_SHADER_TYPE: /* Fall through */
				case TestCaseBase<API>::TESSELATION_EVALUATION_SHADER_TYPE:
					this->execute_positive_test(vertex_shader_source, tess_ctrl_shader_source, tess_eval_shader_source,
												geometry_shader_source, fragment_shader_source, compute_shader_source,
												false, false);
					break;

				default:
					TCU_FAIL("Invalid enum");
					break;
				}
			}
			else
			{
				const std::string& fragment_shader_source =
					this->prepare_fragment_shader(tested_shader_type, uniform_definition, uniform_use);
				const std::string& vertex_shader_source =
					this->prepare_vertex_shader(tested_shader_type, uniform_definition, uniform_use);

				this->execute_positive_test(vertex_shader_source, fragment_shader_source, false, false);
			}

			glw::GLuint buffer_object_id	   = 0;
			glw::GLint  my_storage_block_index = GL_INVALID_INDEX;

			gl.useProgram(this->program_object_id);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram() failed.");

			my_storage_block_index =
				gl.getProgramResourceIndex(this->program_object_id, GL_SHADER_STORAGE_BLOCK, "storage_block_name");
			GLU_EXPECT_NO_ERROR(gl.getError(), "glGetProgramResourceIndex() failed.");

			if ((unsigned)my_storage_block_index == GL_INVALID_INDEX)
			{
				TCU_FAIL("Uniform block not found or is considered as not active.");
			}

			gl.genBuffers(1, &buffer_object_id);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glGenBuffers() failed.");

			gl.bindBuffer(GL_SHADER_STORAGE_BUFFER, buffer_object_id);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer() failed.");

			switch (var_type_index)
			{
			case 0: //float type of uniform is considered
			{
				glw::GLfloat buffer_data[] = { 0.0f, 1.0f, 2.0f,  3.0f,  4.0f,  5.0f,  6.0f,  7.0f,
											   8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f };

				gl.bufferData(GL_SHADER_STORAGE_BUFFER, sizeof(buffer_data), buffer_data, GL_STATIC_DRAW);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferData() failed.");

				break;
			}		/* float case */
			case 1: //int type of uniform is considered
			{

				glw::GLint buffer_data[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };

				gl.bufferData(GL_SHADER_STORAGE_BUFFER, sizeof(buffer_data), buffer_data, GL_STATIC_DRAW);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferData() failed.");

				break;
			}		/* int case */
			case 2: //uint type of uniform is considered
			{
				glw::GLuint buffer_data[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };

				gl.bufferData(GL_SHADER_STORAGE_BUFFER, sizeof(buffer_data), buffer_data, GL_STATIC_DRAW);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferData() failed.");

				break;
			}		/* uint case */
			case 3: //double type of uniform is considered
			{
				glw::GLdouble buffer_data[] = { 0.0, 1.0, 2.0,  3.0,  4.0,  5.0,  6.0,  7.0,
												8.0, 9.0, 10.0, 11.0, 12.0, 13.0, 14.0, 15.0 };

				gl.bufferData(GL_SHADER_STORAGE_BUFFER, sizeof(buffer_data), buffer_data, GL_STATIC_DRAW);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferData() failed.");

				break;
			} /* double case */
			default:
			{
				TCU_FAIL("Invalid variable-type index.");

				break;
			}
			} /* switch (var_type_index) */

			gl.shaderStorageBlockBinding(this->program_object_id, my_storage_block_index, 0);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glUniformBlockBinding() failed.");

			gl.bindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, buffer_object_id);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBufferBase() failed.");

			if (TestCaseBase<API>::COMPUTE_SHADER_TYPE != tested_shader_type)
			{
				execute_draw_test(tested_shader_type);
			}
			else
			{
				execute_dispatch_test();
			}

			/* Deallocate any resources used. */
			gl.deleteBuffers(1, &buffer_object_id);
			this->delete_objects();
		} /* if var_type iterator found */
		else
		{
			TCU_FAIL("Type not found.");
		}
	} /* for (int var_type_index = 0; ...) */
}

/** Executes test for compute program
 *
 * @tparam API               Tested API descriptor
 **/
template <class API>
void InteractionStorageBuffers2<API>::execute_dispatch_test()
{
	const glw::Functions& gl = this->context_id.getRenderContext().getFunctions();

	gl.dispatchCompute(1, 1, 1);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawArrays() failed.");
}

/** Executes test for draw program
 *
 * @tparam API               Tested API descriptor
 *
 * @param tested_shader_type The type of shader that is being tested
 **/
template <class API>
void InteractionStorageBuffers2<API>::execute_draw_test(typename TestCaseBase<API>::TestShaderType tested_shader_type)
{
	const glw::Functions& gl = this->context_id.getRenderContext().getFunctions();

	glw::GLuint vao_id = 0;

	gl.genVertexArrays(1, &vao_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenVertexArrays() failed.");

	gl.bindVertexArray(vao_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindVertexArray() failed.");

	switch (tested_shader_type)
	{
	case TestCaseBase<API>::FRAGMENT_SHADER_TYPE:
	case TestCaseBase<API>::VERTEX_SHADER_TYPE: /* Fall through */
	case TestCaseBase<API>::GEOMETRY_SHADER_TYPE:
		gl.drawArrays(GL_POINTS, 0, 1);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawArrays() failed.");
		break;

	case TestCaseBase<API>::TESSELATION_CONTROL_SHADER_TYPE: /* Fall through */
	case TestCaseBase<API>::TESSELATION_EVALUATION_SHADER_TYPE:
		/* Tesselation patch set up */
		gl.patchParameteri(GL_PATCH_VERTICES, 1);
		GLU_EXPECT_NO_ERROR(gl.getError(), "PatchParameteri");

		gl.drawArrays(GL_PATCHES, 0, 1);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawArrays() failed.");
		break;

	default:
		TCU_FAIL("Invalid enum");
		break;
	}

	gl.deleteVertexArrays(1, &vao_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glDeleteVertexArrays() failed.");
}

/* Generates the shader source code for the InteractionUniformBuffers3
 * array tests, and attempts to compile each test shader, for both
 * vertex and fragment shaders.
 *
 * @tparam API               Tested API descriptor
 *
 * @param tested_shader_type The type of shader that is being tested
 *                           (either TestCaseBase<API>::VERTEX_SHADER_TYPE or TestCaseBase<API>::FRAGMENT_SHADER_TYPE).
 */
template <class API>
void InteractionStorageBuffers3<API>::test_shader_compilation(
	typename TestCaseBase<API>::TestShaderType tested_shader_type)
{
	static const glcts::test_var_type var_types_set_es[] = { VAR_TYPE_FLOAT, VAR_TYPE_INT, VAR_TYPE_UINT };
	static const size_t				  num_var_types_es   = sizeof(var_types_set_es) / sizeof(var_types_set_es[0]);

	static const glcts::test_var_type var_types_set_gl[] = { VAR_TYPE_FLOAT, VAR_TYPE_INT, VAR_TYPE_UINT,
															 VAR_TYPE_DOUBLE };
	static const size_t num_var_types_gl = sizeof(var_types_set_gl) / sizeof(var_types_set_gl[0]);

	const std::string invalid_size_declarations[] = { "[2][2][2][]", "[2][2][][2]", "[2][][2][2]", "[][2][2][2]",
													  "[2][2][][]",  "[2][][2][]",  "[][2][2][]",  "[2][][][2]",
													  "[][2][][2]",  "[][][2][2]",  "[2][][][]",   "[][2][][]",
													  "[][][2][]",   "[][][][2]",   "[][][][]" };
	const std::string array_initializers[] = { "float[2][2][2][2](float[2][2][2](float[2][2](float[2](1.0, 2.0),"
											   "float[2](3.0, 4.0)),"
											   "float[2][2](float[2](5.0, 6.0),"
											   "float[2](7.0, 8.0))),"
											   "float[2][2][2](float[2][2](float[2](1.1, 2.1),"
											   "float[2](3.1, 4.1)),"
											   "float[2][2](float[2](5.1, 6.1),"
											   "float[2](7.1, 8.1))));\n",

											   "int[2][2][2][2](int[2][2][2](int[2][2](int[2]( 1,  2),"
											   "int[2]( 3,  4)),"
											   "int[2][2](int[2]( 5,  6),"
											   "int[2]( 7,  8))),"
											   "int[2][2][2](int[2][2](int[2](11, 12),"
											   "int[2](13, 14)),"
											   "int[2][2](int[2](15, 16),"
											   "int[2](17, 18))));\n",

											   "uint[2][2][2][2](uint[2][2][2](uint[2][2](uint[2]( 1u,  2u),"
											   "uint[2]( 3u,  4u)),"
											   "uint[2][2](uint[2]( 5u,  6u),"
											   "uint[2]( 7u,  8u))),"
											   "uint[2][2][2](uint[2][2](uint[2](11u, 12u),"
											   "uint[2](13u, 14u)),"
											   "uint[2][2](uint[2](15u, 16u),"
											   "uint[2](17u, 18u))));\n",

											   "double[2][2][2][2](double[2][2][2](double[2][2](double[2](1.0, 2.0),"
											   "double[2](3.0, 4.0)),"
											   "double[2][2](double[2](5.0, 6.0),"
											   "double[2](7.0, 8.0))),"
											   "double[2][2][2](double[2][2](double[2](1.1, 2.1),"
											   "double[2](3.1, 4.1)),"
											   "double[2][2](double[2](5.1, 6.1),"
											   "double[2](7.1, 8.1))));\n" };
	const glcts::test_var_type* var_types_set = var_types_set_es;
	size_t						num_var_types = num_var_types_es;

	if (API::USE_DOUBLE)
	{
		var_types_set = var_types_set_gl;
		num_var_types = num_var_types_gl;
	}

	/* Iterate through float/ int/ uint types.
	 * Case: without initializer.
	 */
	for (size_t var_type_index = 0; var_type_index < num_var_types; var_type_index++)
	{
		_supported_variable_types_map_const_iterator var_iterator =
			supported_variable_types_map.find(var_types_set[var_type_index]);

		if (var_iterator != supported_variable_types_map.end())
		{
			for (size_t invalid_size_declarations_index = 0;
				 invalid_size_declarations_index <
				 sizeof(invalid_size_declarations) / sizeof(invalid_size_declarations[0]);
				 invalid_size_declarations_index++)
			{
				std::string shader_source;

				shader_source = "layout (std140) buffer MyStorage {\n";
				shader_source += "    " + var_iterator->second.type +
								 invalid_size_declarations[invalid_size_declarations_index] + " my_variable;\n";
				shader_source += "};\n\n";
				shader_source += shader_start;

				/* End main */
				DEFAULT_MAIN_ENDING(tested_shader_type, shader_source);

				/* Execute test */
				EXECUTE_SHADER_TEST(API::ALLOW_UNSIZED_DECLARATION && invalid_size_declarations_index == 3,
									tested_shader_type, shader_source);
			} /* for (int invalid_size_declarations_index = 0; ...) */
		}
		else
		{
			TCU_FAIL("Type not found.");
		}
	} /* for (int var_type_index = 0; ...) */

	/* Iterate through float/ int/ uint types.
	 * Case: with initializer.
	 */
	for (size_t var_type_index = 0; var_type_index < num_var_types; var_type_index++)
	{
		_supported_variable_types_map_const_iterator var_iterator =
			supported_variable_types_map.find(var_types_set[var_type_index]);

		if (var_iterator != supported_variable_types_map.end())
		{
			for (size_t invalid_size_declarations_index = 0;
				 invalid_size_declarations_index <
				 sizeof(invalid_size_declarations) / sizeof(invalid_size_declarations[0]);
				 invalid_size_declarations_index++)
			{
				std::string shader_source;

				shader_source = "layout (std140) buffer MyStorage {\n";
				shader_source += "    " + var_iterator->second.type +
								 invalid_size_declarations[invalid_size_declarations_index] +
								 " my_variable = " + array_initializers[var_type_index];

				var_iterator->second.type + invalid_size_declarations[invalid_size_declarations_index] +
					" my_variable = " + array_initializers[var_type_index];
				shader_source += "};\n\n";
				shader_source += shader_start;

				/* End main */
				DEFAULT_MAIN_ENDING(tested_shader_type, shader_source);

				/* Execute test */
				this->execute_negative_test(tested_shader_type, shader_source);
			} /* for (int invalid_size_declarations_index = 0; ...) */
		}	 /* if var_type iterator found */
		else
		{
			TCU_FAIL("Type not found.");
		}
	} /* for (int var_type_index = 0; ...) */
}

/* Generates the shader source code for the InteractionInterfaceArrays1
 * array test, and attempts to compile the test shader.
 *
 * @tparam API               Tested API descriptor
 *
 * @param tested_shader_type The type of shader that is being tested.
 */
template <class API>
void InteractionInterfaceArrays1<API>::test_shader_compilation(
	typename TestCaseBase<API>::TestShaderType tested_shader_type)
{
	/* Shader source with invalid buffer (buffer cannot be of arrays of arrays type). */
	const std::string invalid_buffer_shader_source = "layout(std140) buffer MyBuffer\n"
													 "{\n"
													 "    float f;\n"
													 "    int   i;\n"
													 "    uint  ui;\n"
													 "} myBuffers[2][2];\n\n"
													 "void main()\n"
													 "{\n";

	/* Verify that buffer arrays of arrays type is rejected. */
	{
		std::string source = invalid_buffer_shader_source;

		DEFAULT_MAIN_ENDING(tested_shader_type, source);

		EXECUTE_SHADER_TEST(API::ALLOW_A_OF_A_ON_INTERFACE_BLOCKS, tested_shader_type, source);
	}
}

/* Generates the shader source code for the InteractionInterfaceArrays2
 * array test, and attempts to compile the test shader.
 *
 * @tparam API              Tested API descriptor
 *
 * @param input_shader_type The type of shader that is being tested.
 */
template <class API>
void InteractionInterfaceArrays2<API>::test_shader_compilation(
	typename TestCaseBase<API>::TestShaderType input_shader_type)
{
	/* Shader source with invalid input (input cannot be of arrays of arrays type). */
	const std::string input_variable_shader_source[] = { "in  float inout_variable", "[2][2];\n"
																					 "out float result",
														 ";\n\n"
														 "void main()\n"
														 "{\n"
														 "    result",
														 " = inout_variable", "[0][0];\n" };
	/* Shader source with invalid output (output cannot be of arrays of arrays type). */
	const std::string output_variable_shader_source[] = { "out float inout_variable",
														  "[2][2];\n\n"
														  "void main()\n"
														  "{\n"
														  "    inout_variable",
														  "[0][0] = 0.0;\n"
														  "    inout_variable",
														  "[0][1] = 1.0;\n"
														  "    inout_variable",
														  "[1][0] = 2.0;\n"
														  "    inout_variable",
														  "[1][1] = 3.0;\n" };

	const typename TestCaseBase<API>::TestShaderType& output_shader_type =
		this->get_output_shader_type(input_shader_type);
	std::string input_source;
	std::string output_source;

	this->prepare_sources(input_shader_type, output_shader_type, input_variable_shader_source,
						  output_variable_shader_source, input_source, output_source);

	/* Verify that INPUTs and OUTPUTs arrays of arrays type is rejected. */
	if (TestCaseBase<API>::COMPUTE_SHADER_TYPE != input_shader_type)
	{
		if (API::ALLOW_A_OF_A_ON_INTERFACE_BLOCKS)
		{

			if (API::USE_ALL_SHADER_STAGES)
			{
				const std::string& compute_shader_source = empty_string;
				const std::string& fragment_shader_source =
					this->prepare_fragment_shader(input_shader_type, input_source, output_source);
				const std::string& geometry_shader_source =
					this->prepare_geometry_shader(input_shader_type, input_source, output_source);
				const std::string& tess_ctrl_shader_source =
					this->prepare_tess_ctrl_shader_source(input_shader_type, input_source, output_source);
				const std::string& tess_eval_shader_source =
					this->prepare_tess_eval_shader_source(input_shader_type, input_source, output_source);
				const std::string& vertex_shader_source =
					this->prepare_vertex_shader(input_shader_type, input_source, output_source);

				this->execute_positive_test(vertex_shader_source, tess_ctrl_shader_source, tess_eval_shader_source,
											geometry_shader_source, fragment_shader_source, compute_shader_source, true,
											false);
			}
			else
			{
				const std::string& fragment_shader_source =
					this->prepare_fragment_shader(input_shader_type, input_source, output_source);
				const std::string& vertex_shader_source =
					this->prepare_vertex_shader(input_shader_type, input_source, output_source);

				this->execute_positive_test(vertex_shader_source, fragment_shader_source, true, false);
			}
		}
		else
		{
			this->execute_negative_test(input_shader_type, input_source);
			this->execute_negative_test(output_shader_type, output_source);
		}
	}
}

/** Gets the shader type to test for the outputs
 *
 * @tparam API              Tested API descriptor
 *
 * @param input_shader_type The type of input shader that is being tested
 **/
template <class API>
const typename TestCaseBase<API>::TestShaderType InteractionInterfaceArrays2<API>::get_output_shader_type(
	const typename TestCaseBase<API>::TestShaderType& input_shader_type)
{
	switch (input_shader_type)
	{
	case TestCaseBase<API>::VERTEX_SHADER_TYPE:
		return TestCaseBase<API>::FRAGMENT_SHADER_TYPE;

	case TestCaseBase<API>::FRAGMENT_SHADER_TYPE:
		if (API::USE_ALL_SHADER_STAGES)
		{
			return TestCaseBase<API>::GEOMETRY_SHADER_TYPE;
		}
		else
		{
			return TestCaseBase<API>::VERTEX_SHADER_TYPE;
		}

	case TestCaseBase<API>::COMPUTE_SHADER_TYPE:
		break;

	case TestCaseBase<API>::GEOMETRY_SHADER_TYPE:
		return TestCaseBase<API>::TESSELATION_EVALUATION_SHADER_TYPE;

	case TestCaseBase<API>::TESSELATION_CONTROL_SHADER_TYPE:
		return TestCaseBase<API>::VERTEX_SHADER_TYPE;

	case TestCaseBase<API>::TESSELATION_EVALUATION_SHADER_TYPE:
		return TestCaseBase<API>::TESSELATION_CONTROL_SHADER_TYPE;

	default:
		TCU_FAIL("Unrecognized shader type.");
		break;
	}

	return input_shader_type;
}

/** Prepare fragment shader
 *
 * @tparam API              Tested API descriptor
 *
 * @param input_shader_type The type of input shader that is being tested
 * @param input_source      Shader in case we want to test inputs for this shader
 * @param output_source     Shader in case we want to test outputs for this shader
 **/
template <class API>
const std::string InteractionInterfaceArrays2<API>::prepare_fragment_shader(
	const typename TestCaseBase<API>::TestShaderType& input_shader_type, const std::string& input_source,
	const std::string& output_source)
{
	switch (input_shader_type)
	{
	case TestCaseBase<API>::VERTEX_SHADER_TYPE:
		return output_source;

	case TestCaseBase<API>::FRAGMENT_SHADER_TYPE:
		return input_source;

	case TestCaseBase<API>::COMPUTE_SHADER_TYPE:
	case TestCaseBase<API>::GEOMETRY_SHADER_TYPE:
	case TestCaseBase<API>::TESSELATION_CONTROL_SHADER_TYPE:
	case TestCaseBase<API>::TESSELATION_EVALUATION_SHADER_TYPE:
		break;

	default:
		TCU_FAIL("Unrecognized shader type.");
		break;
	}

	return default_fragment_shader_source;
}

/** Prepare geometry shader
 *
 * @tparam API              Tested API descriptor
 *
 * @param input_shader_type The type of input shader that is being tested
 * @param input_source      Shader in case we want to test inputs for this shader
 * @param output_source     Shader in case we want to test outputs for this shader
 **/
template <class API>
const std::string InteractionInterfaceArrays2<API>::prepare_geometry_shader(
	const typename TestCaseBase<API>::TestShaderType& input_shader_type, const std::string& input_source,
	const std::string& output_source)
{
	switch (input_shader_type)
	{
	case TestCaseBase<API>::FRAGMENT_SHADER_TYPE:
		if (API::USE_ALL_SHADER_STAGES)
		{
			return output_source;
		}
		break;

	case TestCaseBase<API>::GEOMETRY_SHADER_TYPE:
		return input_source;

	case TestCaseBase<API>::COMPUTE_SHADER_TYPE:
	case TestCaseBase<API>::VERTEX_SHADER_TYPE:
	case TestCaseBase<API>::TESSELATION_CONTROL_SHADER_TYPE:
	case TestCaseBase<API>::TESSELATION_EVALUATION_SHADER_TYPE:
		break;

	default:
		TCU_FAIL("Unrecognized shader type.");
		break;
	}

	return default_geometry_shader_source;
}

/** Prepare tessellation control shader
 *
 * @tparam API              Tested API descriptor
 *
 * @param input_shader_type The type of input shader that is being tested
 * @param input_source      Shader in case we want to test inputs for this shader
 * @param output_source     Shader in case we want to test outputs for this shader
 **/
template <class API>
const std::string InteractionInterfaceArrays2<API>::prepare_tess_ctrl_shader_source(
	const typename TestCaseBase<API>::TestShaderType& input_shader_type, const std::string& input_source,
	const std::string& output_source)
{
	switch (input_shader_type)
	{
	case TestCaseBase<API>::VERTEX_SHADER_TYPE:
	case TestCaseBase<API>::FRAGMENT_SHADER_TYPE:
	case TestCaseBase<API>::COMPUTE_SHADER_TYPE:
	case TestCaseBase<API>::GEOMETRY_SHADER_TYPE:
		break;

	case TestCaseBase<API>::TESSELATION_CONTROL_SHADER_TYPE:
		return input_source;

	case TestCaseBase<API>::TESSELATION_EVALUATION_SHADER_TYPE:
		return output_source;

	default:
		TCU_FAIL("Unrecognized shader type.");
		break;
	}

	return default_tc_shader_source;
}

/** Prepare tessellation evaluation shader
 *
 * @tparam API              Tested API descriptor
 *
 * @param input_shader_type The type of input shader that is being tested
 * @param input_source      Shader in case we want to test inputs for this shader
 * @param output_source     Shader in case we want to test outputs for this shader
 **/
template <class API>
const std::string InteractionInterfaceArrays2<API>::prepare_tess_eval_shader_source(
	const typename TestCaseBase<API>::TestShaderType& input_shader_type, const std::string& input_source,
	const std::string& output_source)
{
	switch (input_shader_type)
	{
	case TestCaseBase<API>::VERTEX_SHADER_TYPE:
	case TestCaseBase<API>::FRAGMENT_SHADER_TYPE:
	case TestCaseBase<API>::COMPUTE_SHADER_TYPE:
	case TestCaseBase<API>::TESSELATION_CONTROL_SHADER_TYPE:
		break;

	case TestCaseBase<API>::GEOMETRY_SHADER_TYPE:
		return output_source;

	case TestCaseBase<API>::TESSELATION_EVALUATION_SHADER_TYPE:
		return input_source;

	default:
		TCU_FAIL("Unrecognized shader type.");
		break;
	}

	return default_te_shader_source;
}

/** Prepare vertex shader
 *
 * @tparam API              Tested API descriptor
 *
 * @param input_shader_type The type of input shader that is being tested
 * @param input_source      Shader in case we want to test inputs for this shader
 * @param output_source     Shader in case we want to test outputs for this shader
 **/
template <class API>
const std::string InteractionInterfaceArrays2<API>::prepare_vertex_shader(
	const typename TestCaseBase<API>::TestShaderType& input_shader_type, const std::string& input_source,
	const std::string& output_source)
{
	switch (input_shader_type)
	{
	case TestCaseBase<API>::VERTEX_SHADER_TYPE:
		return input_source;

	case TestCaseBase<API>::FRAGMENT_SHADER_TYPE:
		if (!API::USE_ALL_SHADER_STAGES)
		{
			return output_source;
		}
		break;

	case TestCaseBase<API>::TESSELATION_CONTROL_SHADER_TYPE:
		return output_source;

	case TestCaseBase<API>::COMPUTE_SHADER_TYPE:
	case TestCaseBase<API>::GEOMETRY_SHADER_TYPE:
	case TestCaseBase<API>::TESSELATION_EVALUATION_SHADER_TYPE:
		break;

	default:
		TCU_FAIL("Unrecognized shader type.");
		break;
	}

	return default_vertex_shader_source;
}

/** Prepare the inputs and outputs shaders
 *
 * @tparam API                 Tested API descriptor
 *
 * @param input_shader_type    The type of input shader that is being tested
 * @param output_shader_type   The type of output shader that is being tested
 * @param input_shader_source  Snippet used to prepare the input shader
 * @param output_shader_source Snippet used to prepare the output shader
 * @param input_source         Resulting input shader
 * @param output_source        Resulting output shader
 **/
template <class API>
void InteractionInterfaceArrays2<API>::prepare_sources(
	const typename TestCaseBase<API>::TestShaderType& input_shader_type,
	const typename TestCaseBase<API>::TestShaderType& output_shader_type, const std::string* input_shader_source,
	const std::string* output_shader_source, std::string& input_source, std::string& output_source)
{
	if (TestCaseBase<API>::COMPUTE_SHADER_TYPE != input_shader_type)
	{
		input_source += input_shader_source[0];
		output_source += output_shader_source[0];

		if ((TestCaseBase<API>::GEOMETRY_SHADER_TYPE == input_shader_type) ||
			(TestCaseBase<API>::TESSELATION_CONTROL_SHADER_TYPE == input_shader_type) ||
			(TestCaseBase<API>::TESSELATION_EVALUATION_SHADER_TYPE == input_shader_type))
		{
			input_source += "[]";
		}

		if (TestCaseBase<API>::TESSELATION_CONTROL_SHADER_TYPE == output_shader_type)
		{
			output_source += "[]";
		}

		input_source += input_shader_source[1];
		output_source += output_shader_source[1];

		if (TestCaseBase<API>::TESSELATION_CONTROL_SHADER_TYPE == input_shader_type)
		{
			input_source += "[]";
		}

		if (TestCaseBase<API>::TESSELATION_CONTROL_SHADER_TYPE == output_shader_type)
		{
			output_source += "[gl_InvocationID]";
		}

		input_source += input_shader_source[2];
		output_source += output_shader_source[2];

		if (TestCaseBase<API>::TESSELATION_CONTROL_SHADER_TYPE == input_shader_type)
		{
			input_source += "[gl_InvocationID]";
		}

		if (TestCaseBase<API>::TESSELATION_CONTROL_SHADER_TYPE == output_shader_type)
		{
			output_source += "[gl_InvocationID]";
		}

		input_source += input_shader_source[3];
		output_source += output_shader_source[3];

		if ((TestCaseBase<API>::GEOMETRY_SHADER_TYPE == input_shader_type) ||
			(TestCaseBase<API>::TESSELATION_EVALUATION_SHADER_TYPE == input_shader_type))
		{
			input_source += "[0]";
		}

		if (TestCaseBase<API>::TESSELATION_CONTROL_SHADER_TYPE == input_shader_type)
		{
			input_source += "[gl_InvocationID]";
		}

		if (TestCaseBase<API>::TESSELATION_CONTROL_SHADER_TYPE == output_shader_type)
		{
			output_source += "[gl_InvocationID]";
		}

		input_source += input_shader_source[4];
		output_source += output_shader_source[4];

		if (TestCaseBase<API>::TESSELATION_CONTROL_SHADER_TYPE == output_shader_type)
		{
			output_source += "[gl_InvocationID]";
		}

		output_source += output_shader_source[5];

		DEFAULT_MAIN_ENDING(input_shader_type, input_source);
		DEFAULT_MAIN_ENDING(output_shader_type, output_source);
	}
}

/* Generates the shader source code for the InteractionInterfaceArrays3
 * array test, and attempts to compile the test shader.
 *
 * @tparam API               Tested API descriptor
 *
 * @param tested_shader_type The type of shader that is being tested.
 */
template <class API>
void InteractionInterfaceArrays3<API>::test_shader_compilation(
	typename TestCaseBase<API>::TestShaderType tested_shader_type)
{
	/* Shader source with invalid uniform block (uniform block cannot be of arrays of arrays type). */
	const std::string invalid_uniform_block_shader_source = "layout(std140) uniform MyUniformBlock\n"
															"{\n"
															"    float f;\n"
															"    int   i;\n"
															"    uint  ui;\n"
															"} myUniformBlocks[2][2];\n\n"
															"void main()\n"
															"{\n";

	/* Verify that uniform block arrays of arrays type is rejected. */
	{
		std::string source = invalid_uniform_block_shader_source;

		DEFAULT_MAIN_ENDING(tested_shader_type, source);

		EXECUTE_SHADER_TEST(API::ALLOW_A_OF_A_ON_INTERFACE_BLOCKS, tested_shader_type, source);
	}
}

/* Generates the shader source code for the InteractionInterfaceArrays4
 * array test, and attempts to compile the test shader.
 *
 * @tparam API              Tested API descriptor
 *
 * @param input_shader_type The type of shader that is being tested.
 */
template <class API>
void InteractionInterfaceArrays4<API>::test_shader_compilation(
	typename TestCaseBase<API>::TestShaderType input_shader_type)
{
	/* Shader source with invalid input (input cannot be of arrays of arrays type). */
	const std::string input_block_shader_source[] = { "in  InOutBlock {\n"
													  "    float inout_variable;\n"
													  "} inout_block",
													  "[2][2];\n"
													  "out float result",
													  ";\n\n"
													  "void main()\n"
													  "{\n"
													  "    result",
													  " = inout_block", "[0][0].inout_variable;\n" };
	/* Shader source with invalid output (output cannot be of arrays of arrays type). */
	const std::string output_block_shader_source[] = { "out InOutBlock {\n"
													   "    float inout_variable;\n"
													   "} inout_block",
													   "[2][2];\n"
													   "\n"
													   "void main()\n"
													   "{\n"
													   "    inout_block",
													   "[0][0].inout_variable = 0.0;\n"
													   "    inout_block",
													   "[0][1].inout_variable = 1.0;\n"
													   "    inout_block",
													   "[1][0].inout_variable = 2.0;\n"
													   "    inout_block",
													   "[1][1].inout_variable = 3.0;\n" };

	const typename TestCaseBase<API>::TestShaderType& output_shader_type =
		this->get_output_shader_type(input_shader_type);
	std::string input_source;
	std::string output_source;

	this->prepare_sources(input_shader_type, output_shader_type, input_block_shader_source, output_block_shader_source,
						  input_source, output_source);

	/* Verify that INPUTs and OUTPUTs arrays of arrays type is rejected. */
	if ((TestCaseBase<API>::VERTEX_SHADER_TYPE != input_shader_type) &&
		(TestCaseBase<API>::COMPUTE_SHADER_TYPE != input_shader_type))
	{
		if (API::ALLOW_A_OF_A_ON_INTERFACE_BLOCKS && API::ALLOW_IN_OUT_INTERFACE_BLOCKS)
		{

			if (API::USE_ALL_SHADER_STAGES)
			{
				const std::string& compute_shader_source = empty_string;
				const std::string& fragment_shader_source =
					this->prepare_fragment_shader(input_shader_type, input_source, output_source);
				const std::string& geometry_shader_source =
					this->prepare_geometry_shader(input_shader_type, input_source, output_source);
				const std::string& tess_ctrl_shader_source =
					this->prepare_tess_ctrl_shader_source(input_shader_type, input_source, output_source);
				const std::string& tess_eval_shader_source =
					this->prepare_tess_eval_shader_source(input_shader_type, input_source, output_source);
				const std::string& vertex_shader_source =
					this->prepare_vertex_shader(input_shader_type, input_source, output_source);

				this->execute_positive_test(vertex_shader_source, tess_ctrl_shader_source, tess_eval_shader_source,
											geometry_shader_source, fragment_shader_source, compute_shader_source, true,
											false);
			}
			else
			{
				const std::string& fragment_shader_source =
					this->prepare_fragment_shader(input_shader_type, input_source, output_source);
				const std::string& vertex_shader_source =
					this->prepare_vertex_shader(input_shader_type, input_source, output_source);

				this->execute_positive_test(vertex_shader_source, fragment_shader_source, true, false);
			}
		}
		else
		{
			this->execute_negative_test(input_shader_type, input_source);
			this->execute_negative_test(output_shader_type, output_source);
		}
	}
}

/** Calulate smallest denominator for values over 1
 *
 * @param value Value in question
 *
 * @return Smallest denominator
 **/
size_t findSmallestDenominator(const size_t value)
{
	/* Skip 0 and 1 */
	for (size_t i = 2; i < value; ++i)
	{
		if (0 == value % i)
		{
			return i;
		}
	}

	return value;
}

/** Check if left is bigger than right
 *
 * @tparam T Type of values

 * @param l  Left value
 * @param r  Right value
 *
 * @return true if l > r, false otherwise
 **/
template <class T>
bool more(const T& l, const T& r)
{
	return l > r;
}

/** Prepare dimensions of array with given number of entries
 *
 * @tparam API       Tested API descriptor
 *
 * @param n_entries  Number of entries
 * @param dimensions Storage for dimesnions
 **/
template <class API>
void prepareDimensions(size_t n_entries, std::vector<size_t>& dimensions)
{
	if (dimensions.empty())
		return;

	const size_t last = dimensions.size() - 1;

	/* Calculate */
	for (size_t i = 0; i < last; ++i)
	{
		const size_t denom = findSmallestDenominator(n_entries);

		n_entries /= denom;

		dimensions[i] = denom;
	}

	dimensions[last] = n_entries;

	/* Sort */
	std::sort(dimensions.begin(), dimensions.end(), more<size_t>);
}

/* Generates the shader source code for the AtomicDeclarationTest
 * and attempts to compile each shader
 *
 * @tparam API               Tested API descriptor
 *
 * @param tested_shader_type The type of shader that is being tested
 */
template <class API>
void AtomicDeclarationTest<API>::test_shader_compilation(typename TestCaseBase<API>::TestShaderType tested_shader_type)
{
	static const char* indent_step		   = "    ";
	static const char* uniform_atomic_uint = "layout(binding = 0) uniform atomic_uint";

	const glw::Functions& gl = this->context_id.getRenderContext().getFunctions();

	std::string			comment;
	std::vector<size_t> dimensions;
	std::string			indent;
	std::string			indexing;
	std::string			invalid_definition = uniform_atomic_uint;
	std::string			invalid_iteration;
	std::string			invalid_shader_source;
	std::string			loop_end;
	glw::GLint			max_atomics = 0;
	glw::GLenum			pname		= 0;
	std::string			valid_shader_source;
	std::string			valid_definition = uniform_atomic_uint;
	std::string			valid_iteration;

	/* Select pname of max for stage */
	switch (tested_shader_type)
	{
	case TestCaseBase<API>::COMPUTE_SHADER_TYPE:
		pname = GL_MAX_COMPUTE_ATOMIC_COUNTERS;
		break;
	case TestCaseBase<API>::FRAGMENT_SHADER_TYPE:
		pname = GL_MAX_FRAGMENT_ATOMIC_COUNTERS;
		break;
	case TestCaseBase<API>::GEOMETRY_SHADER_TYPE:
		pname = GL_MAX_GEOMETRY_ATOMIC_COUNTERS;
		break;
	case TestCaseBase<API>::TESSELATION_CONTROL_SHADER_TYPE:
		pname = GL_MAX_TESS_CONTROL_ATOMIC_COUNTERS;
		break;
	case TestCaseBase<API>::TESSELATION_EVALUATION_SHADER_TYPE:
		pname = GL_MAX_TESS_EVALUATION_ATOMIC_COUNTERS;
		break;
	case TestCaseBase<API>::VERTEX_SHADER_TYPE:
		pname = GL_MAX_VERTEX_ATOMIC_COUNTERS;
		break;
	default:
		TCU_FAIL("Invalid enum");
		break;
	}

	/* Get maximum */
	gl.getIntegerv(pname, &max_atomics);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GetIntegerv");

	if (0 == max_atomics)
	{
		/* Not supported - skip */
		return;
	}
	else
	{
		dimensions.resize(API::MAX_ARRAY_DIMENSIONS);
		prepareDimensions<API>(max_atomics, dimensions);
	}

	/* Prepare parts of shader */
	for (size_t i = API::MAX_ARRAY_DIMENSIONS; i != 0; --i)
	{
		char it[16];
		char max[16];

		indent += indent_step;

		loop_end.insert(0, "}\n");
		loop_end.insert(0, indent);

		sprintf(it, "i%u", (unsigned int)(API::MAX_ARRAY_DIMENSIONS - i));

		indexing += "[";
		indexing += it;
		indexing += "]";

		sprintf(max, "%u", (unsigned int)(dimensions[i - 1]));

		valid_definition += "[";
		valid_definition += max;
		valid_definition += "]";

		valid_iteration += indent;
		valid_iteration += "for (uint ";
		valid_iteration += it;
		valid_iteration += " = 0; ";
		valid_iteration += it;
		valid_iteration += " < ";
		valid_iteration += max;
		valid_iteration += "; ++";
		valid_iteration += it;
		valid_iteration += ")\n";
		valid_iteration += indent;
		valid_iteration += "{\n";

		if (1 == i)
		{
			sprintf(max, "%u", (unsigned int)(dimensions[i - 1] + 1));
		}
		invalid_definition += "[";
		invalid_definition += max;
		invalid_definition += "]";

		invalid_iteration += indent;
		invalid_iteration += "for (uint ";
		invalid_iteration += it;
		invalid_iteration += " = 0; ";
		invalid_iteration += it;
		invalid_iteration += " < ";
		invalid_iteration += max;
		invalid_iteration += "; ++";
		invalid_iteration += it;
		invalid_iteration += ")\n";
		invalid_iteration += indent;
		invalid_iteration += "{\n";
	}

	{
		char max[16];

		sprintf(max, "%u", (unsigned int)(max_atomics));
		comment += "/* MAX_*_ATOMIC_COUNTERS = ";
		comment += max;
		comment += " */\n";
	}

	/* Prepare invalid source */
	invalid_shader_source += comment;
	invalid_shader_source += invalid_definition;
	invalid_shader_source += " a;\n\nvoid main()\n{\n";
	invalid_shader_source += invalid_iteration;
	invalid_shader_source += indent;
	invalid_shader_source += indent_step;
	invalid_shader_source += "atomicCounterIncrement( a";
	invalid_shader_source += indexing;
	invalid_shader_source += " );\n";
	invalid_shader_source += loop_end;

	/* Prepare valid source */
	valid_shader_source += comment;
	valid_shader_source += valid_definition;
	valid_shader_source += " a;\n\nvoid main()\n{\n";
	valid_shader_source += valid_iteration;
	valid_shader_source += indent;
	valid_shader_source += indent_step;
	valid_shader_source += "atomicCounterIncrement( a";
	valid_shader_source += indexing;
	valid_shader_source += " );\n";
	valid_shader_source += loop_end;

	/* End main */
	DEFAULT_MAIN_ENDING(tested_shader_type, invalid_shader_source);
	DEFAULT_MAIN_ENDING(tested_shader_type, valid_shader_source);

	/* Execute test */
	EXECUTE_POSITIVE_TEST(tested_shader_type, valid_shader_source, true, false);

	/* Expect build failure for invalid shader source */
	{
		bool negative_build_test_result = false;

		try
		{
			EXECUTE_POSITIVE_TEST(tested_shader_type, invalid_shader_source, true, false);
		}
		catch (...)
		{
			negative_build_test_result = true;
		}

		if (false == negative_build_test_result)
		{
			TCU_FAIL("It was expected that build process will fail");
		}
	}
}

/* Generates the shader source code for the AtomicUsageTest
 * and attempts to compile each shader
 *
 * @tparam API               Tested API descriptor
 *
 * @param tested_shader_type The type of shader that is being tested
 */
template <class API>
void AtomicUsageTest<API>::test_shader_compilation(typename TestCaseBase<API>::TestShaderType tested_shader_type)
{
	const glw::Functions& gl = this->context_id.getRenderContext().getFunctions();

	glw::GLint  max_atomics  = 0;
	glw::GLint  max_bindings = 0;
	glw::GLint  max_size	 = 0;
	glw::GLenum pname		 = 0;

	/* Select pname of max for stage */
	switch (tested_shader_type)
	{
	case TestCaseBase<API>::COMPUTE_SHADER_TYPE:
		pname = GL_MAX_COMPUTE_ATOMIC_COUNTERS;
		break;
	case TestCaseBase<API>::FRAGMENT_SHADER_TYPE:
		pname = GL_MAX_FRAGMENT_ATOMIC_COUNTERS;
		break;
	case TestCaseBase<API>::GEOMETRY_SHADER_TYPE:
		pname = GL_MAX_GEOMETRY_ATOMIC_COUNTERS;
		break;
	case TestCaseBase<API>::TESSELATION_CONTROL_SHADER_TYPE:
		pname = GL_MAX_TESS_CONTROL_ATOMIC_COUNTERS;
		break;
	case TestCaseBase<API>::TESSELATION_EVALUATION_SHADER_TYPE:
		pname = GL_MAX_TESS_EVALUATION_ATOMIC_COUNTERS;
		break;
	case TestCaseBase<API>::VERTEX_SHADER_TYPE:
		pname = GL_MAX_VERTEX_ATOMIC_COUNTERS;
		break;
	default:
		TCU_FAIL("Invalid enum");
		break;
	}

	/* Get limits */
	gl.getIntegerv(pname, &max_atomics);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GetIntegerv");

	gl.getIntegerv(GL_MAX_ATOMIC_COUNTER_BUFFER_BINDINGS, &max_bindings);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GetIntegerv");

	gl.getIntegerv(GL_MAX_ATOMIC_COUNTER_BUFFER_SIZE, &max_size);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GetIntegerv");

	if (0 == max_atomics)
	{
		/* Not supported - skip */
		return;
	}

	const glw::GLuint last_binding = (glw::GLuint)max_bindings - 1;
	const glw::GLuint offset	   = (glw::GLuint)max_size / 2;
	glw::GLuint		  n_entries =
		std::min((glw::GLuint)(max_size - offset) / (glw::GLuint)sizeof(glw::GLuint), (glw::GLuint)max_atomics);

	if (TestCaseBase<API>::COMPUTE_SHADER_TYPE == tested_shader_type)
	{
		glw::GLint max_uniform_locations = 0;

		gl.getIntegerv(GL_MAX_UNIFORM_LOCATIONS, &max_uniform_locations);
		GLU_EXPECT_NO_ERROR(gl.getError(), "GetIntegerv");

		max_atomics = std::min(max_atomics, (max_uniform_locations - 1));
		n_entries   = (glw::GLuint)std::min((glw::GLint)n_entries, (max_uniform_locations - 1));
	}

	execute(tested_shader_type, last_binding, 0 /* offset */, max_atomics);
	execute(tested_shader_type, last_binding, offset, n_entries);
}

/* Generates the shader source code for the AtomicUsageTest
 * and attempts to compile each shader
 *
 * @tparam API               Tested API descriptor
 *
 * @param tested_shader_type The type of shader that is being tested
 * @param binding            Binding index
 * @param offset             Offset of data
 * @param n_entries          Number of entries in array
 */
template <class API>
void AtomicUsageTest<API>::execute(typename TestCaseBase<API>::TestShaderType tested_shader_type, glw::GLuint binding,
								   glw::GLuint offset, glw::GLuint n_entries)
{
	static const char* indent_step		   = "    ";
	static const char* layout_binding	  = "layout(binding = ";
	static const char* layout_offset	   = ", offset = ";
	static const char* uniform_atomic_uint = ") uniform atomic_uint";

	const glw::Functions& gl = this->context_id.getRenderContext().getFunctions();

	std::string			comment;
	std::vector<size_t> dimensions;
	std::string			indent;
	std::string			indexing;
	std::string			loop_end;
	std::string			result;
	std::string			valid_shader_source;
	std::string			valid_definition = layout_binding;
	std::string			valid_iteration;
	std::string			varying_definition;

	dimensions.resize(API::MAX_ARRAY_DIMENSIONS);
	prepareDimensions<API>(n_entries, dimensions);

	/* Prepare parts of shader */

	/* Append binding */
	{
		char buffer[16];
		sprintf(buffer, "%u", static_cast<unsigned int>(binding));
		valid_definition += buffer;
		valid_definition += layout_offset;
		sprintf(buffer, "%u", static_cast<unsigned int>(offset));
		valid_definition += buffer;
		valid_definition += uniform_atomic_uint;
	}

	for (size_t i = API::MAX_ARRAY_DIMENSIONS; i != 0; --i)
	{
		char it[16];
		char max[16];

		indent += indent_step;

		loop_end.insert(0, "}\n");
		loop_end.insert(0, indent);

		sprintf(it, "i%u", (unsigned int)(API::MAX_ARRAY_DIMENSIONS - i));

		indexing += "[";
		indexing += it;
		indexing += "]";

		sprintf(max, "%u", (unsigned int)(dimensions[i - 1]));
		valid_definition += "[";
		valid_definition += max;
		valid_definition += "]";

		valid_iteration += indent;
		valid_iteration += "for (uint ";
		valid_iteration += it;
		valid_iteration += " = 0; ";
		valid_iteration += it;
		valid_iteration += " < ";
		valid_iteration += max;
		valid_iteration += "; ++";
		valid_iteration += it;
		valid_iteration += ")\n";
		valid_iteration += indent;
		valid_iteration += "{\n";
	}

	{
		char max[16];

		sprintf(max, "%u", (unsigned int)(n_entries));
		comment += "/* Number of atomic counters = ";
		comment += max;
		comment += " */\n";
	}

	/* Select varyings and result */
	switch (tested_shader_type)
	{
	case TestCaseBase<API>::COMPUTE_SHADER_TYPE:
		result			   = "    imageStore(uni_image, ivec2(gl_GlobalInvocationID.xy), vec4(result, 0, 0, 0));\n";
		varying_definition = "writeonly uniform image2D uni_image;\n"
							 "\n";
		break;

	case TestCaseBase<API>::FRAGMENT_SHADER_TYPE:
		result			   = "    color = vec4(result);\n";
		varying_definition = "out vec4 color;\n"
							 "\n";
		break;

	case TestCaseBase<API>::GEOMETRY_SHADER_TYPE:
		result = "    gl_Position  = vec4(-1, -1, 0, 1);\n"
				 "    fs_result = result;\n"
				 "    EmitVertex();\n"
				 "    gl_Position  = vec4(-1, 1, 0, 1);\n"
				 "    fs_result = result;\n"
				 "    EmitVertex();\n"
				 "    gl_Position  = vec4(1, -1, 0, 1);\n"
				 "    fs_result = result;\n"
				 "    EmitVertex();\n"
				 "    gl_Position  = vec4(1, 1, 0, 1);\n"
				 "    fs_result = result;\n"
				 "    EmitVertex();\n";
		varying_definition = "out float fs_result;\n"
							 "\n";
		break;

	case TestCaseBase<API>::TESSELATION_CONTROL_SHADER_TYPE:
		result = "    tcs_result[gl_InvocationID] = result;\n"
				 "\n"
				 "    gl_TessLevelOuter[0] = 1.0;\n"
				 "    gl_TessLevelOuter[1] = 1.0;\n"
				 "    gl_TessLevelOuter[2] = 1.0;\n"
				 "    gl_TessLevelOuter[3] = 1.0;\n"
				 "    gl_TessLevelInner[0] = 1.0;\n"
				 "    gl_TessLevelInner[1] = 1.0;\n";
		varying_definition = "out float tcs_result[];\n"
							 "\n";
		break;

	case TestCaseBase<API>::TESSELATION_EVALUATION_SHADER_TYPE:
		result			   = "    fs_result = result;\n";
		varying_definition = "out float fs_result;\n"
							 "\n";
		break;

	case TestCaseBase<API>::VERTEX_SHADER_TYPE:
		result			   = "    fs_result = result;\n";
		varying_definition = "out float fs_result;\n"
							 "\n";
		break;

	default:
		TCU_FAIL("Invalid enum");
		break;
	};

	/* Prepare valid source */
	valid_shader_source += varying_definition;
	valid_shader_source += comment;
	valid_shader_source += valid_definition;
	valid_shader_source += " a;\n\nvoid main()\n{\n    uint sum = 0u;\n";
	valid_shader_source += valid_iteration;
	valid_shader_source += indent;
	valid_shader_source += indent_step;
	valid_shader_source += "sum += atomicCounterIncrement( a";
	valid_shader_source += indexing;
	valid_shader_source += " );\n";
	valid_shader_source += loop_end;
	valid_shader_source += "\n"
						   "    float result = 0.0;\n"
						   "\n"
						   "    if (16u < sum)\n"
						   "    {\n"
						   "         result = 1.0;\n"
						   "    }\n";
	valid_shader_source += result;
	valid_shader_source += shader_end;

	/* Build program */
	{
		const std::string* cs  = &empty_string;
		const std::string* vs  = &default_vertex_shader_source;
		const std::string* tcs = &empty_string;
		const std::string* tes = &empty_string;
		const std::string* gs  = &empty_string;
		const std::string* fs  = &pass_fragment_shader_source;

		switch (tested_shader_type)
		{
		case TestCaseBase<API>::COMPUTE_SHADER_TYPE:
			cs = &valid_shader_source;
			vs = &empty_string;
			fs = &empty_string;
			break;

		case TestCaseBase<API>::FRAGMENT_SHADER_TYPE:
			fs = &valid_shader_source;
			break;

		case TestCaseBase<API>::GEOMETRY_SHADER_TYPE:
			gs = &valid_shader_source;
			break;

		case TestCaseBase<API>::TESSELATION_CONTROL_SHADER_TYPE:
			tcs = &valid_shader_source;
			tes = &pass_te_shader_source;
			break;

		case TestCaseBase<API>::TESSELATION_EVALUATION_SHADER_TYPE:
			tcs = &default_tc_shader_source;
			tes = &valid_shader_source;
			break;

		case TestCaseBase<API>::VERTEX_SHADER_TYPE:
			vs = &valid_shader_source;
			break;

		default:
			TCU_FAIL("Invalid enum");
			break;
		};

		if (API::USE_ALL_SHADER_STAGES)
		{
			this->execute_positive_test(*vs, *tcs, *tes, *gs, *fs, *cs, false, false);
		}
		else
		{
			this->execute_positive_test(*vs, *fs, false, false);
		}
	}

	gl.useProgram(this->program_object_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram() failed.");

	/* Prepare buffer */
	glw::GLuint				 buffer_object_id = 0;
	std::vector<glw::GLuint> buffer_data;
	const size_t			 start_pos		  = offset / 4;
	const size_t			 last_pos		  = start_pos + n_entries;
	const size_t			 buffer_data_size = last_pos * sizeof(glw::GLuint);

	gl.genBuffers(1, &buffer_object_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenBuffers() failed.");

	gl.bindBuffer(GL_ATOMIC_COUNTER_BUFFER, buffer_object_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer() failed.");

	buffer_data.resize(start_pos + n_entries);
	for (size_t i = 0; i < n_entries; ++i)
	{
		buffer_data[start_pos + i] = (glw::GLuint)i;
	}

	gl.bufferData(GL_ATOMIC_COUNTER_BUFFER, buffer_data_size, &buffer_data[0], GL_STATIC_DRAW);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferData() failed.");

	gl.bindBufferBase(GL_ATOMIC_COUNTER_BUFFER, binding, buffer_object_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBufferBase() failed.");

	/* Run program */
	if (TestCaseBase<API>::COMPUTE_SHADER_TYPE != tested_shader_type)
	{
		glw::GLuint framebuffer_object_id = 0;
		glw::GLuint texture_object_id	 = 0;
		glw::GLuint vao_id				  = 0;

		gl.genTextures(1, &texture_object_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGenTextures() failed.");

		gl.bindTexture(GL_TEXTURE_2D, texture_object_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture() failed.");

		gl.texStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, 1, 1);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glTexStorage2D() failed.");

		gl.genFramebuffers(1, &framebuffer_object_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGenFramebuffers() failed.");

		gl.bindFramebuffer(GL_FRAMEBUFFER, framebuffer_object_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffer() failed.");

		gl.framebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture_object_id, 0);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glFramebufferTexture2D() failed.");

		gl.viewport(0, 0, 1, 1);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glViewport() failed.");

		gl.genVertexArrays(1, &vao_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGenVertexArrays() failed.");

		gl.bindVertexArray(vao_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindVertexArray() failed.");

		switch (tested_shader_type)
		{
		case TestCaseBase<API>::FRAGMENT_SHADER_TYPE:
		case TestCaseBase<API>::VERTEX_SHADER_TYPE: /* Fall through */
		case TestCaseBase<API>::GEOMETRY_SHADER_TYPE:
			gl.drawArrays(GL_POINTS, 0, 1);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawArrays() failed.");
			break;

		case TestCaseBase<API>::TESSELATION_CONTROL_SHADER_TYPE: /* Fall through */
		case TestCaseBase<API>::TESSELATION_EVALUATION_SHADER_TYPE:
			/* Tesselation patch set up */
			gl.patchParameteri(GL_PATCH_VERTICES, 1);
			GLU_EXPECT_NO_ERROR(gl.getError(), "PatchParameteri");

			gl.drawArrays(GL_PATCHES, 0, 1);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawArrays() failed.");
			break;

		default:
			TCU_FAIL("Invalid enum");
			break;
		}

		gl.memoryBarrier(GL_ALL_BARRIER_BITS);
		GLU_EXPECT_NO_ERROR(gl.getError(), "MemoryBarrier.");

		gl.bindTexture(GL_TEXTURE_2D, 0);
		gl.bindFramebuffer(GL_FRAMEBUFFER, 0);
		gl.bindVertexArray(0);
		gl.deleteTextures(1, &texture_object_id);
		gl.deleteFramebuffers(1, &framebuffer_object_id);
		gl.deleteVertexArrays(1, &vao_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "An error ocurred while deleting generated objects.");
	}
	else
	{
		gl.dispatchCompute(1, 1, 1);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawArrays() failed.");

		gl.memoryBarrier(GL_ALL_BARRIER_BITS);
		GLU_EXPECT_NO_ERROR(gl.getError(), "MemoryBarrier.");
	}

	/* Verify results */
	bool test_result = true;

	const glw::GLuint* results =
		(glw::GLuint*)gl.mapBufferRange(GL_ATOMIC_COUNTER_BUFFER, 0 /* offset */, buffer_data_size, GL_MAP_READ_BIT);
	GLU_EXPECT_NO_ERROR(gl.getError(), "MapBufferRange");

	/* Anything before start position should be 0 */
	for (size_t i = 0; i < start_pos; ++i)
	{
		if (0 != results[i])
		{
			test_result = false;
			break;
		}
	}

	/* Anything from start_pos should be incremented by 1 */
	int diff = 0;
	for (size_t i = 0; i < n_entries; ++i)
	{
		/* Tesselation evaluation can be called several times
		 In here, check the increment is consistent over all results.
		 */
		if (tested_shader_type == TestCaseBase<API>::TESSELATION_EVALUATION_SHADER_TYPE)
		{
			if (i == 0)
			{
				diff = static_cast<int>(results[i + start_pos]) - static_cast<int>(i);
				if (diff <= 0)
				{
					test_result = false;
					break;
				}
			}
			else if ((static_cast<int>(results[i + start_pos]) - static_cast<int>(i)) != diff)
			{
				test_result = false;
				break;
			}
		}
		else
		{
			if (i + 1 != results[i + start_pos])
			{
				test_result = false;
				break;
			}
		}
	}

	gl.unmapBuffer(GL_ATOMIC_COUNTER_BUFFER);
	GLU_EXPECT_NO_ERROR(gl.getError(), "UnmapBuffer");

	/* Deallocate any resources used. */
	gl.deleteBuffers(1, &buffer_object_id);
	this->delete_objects();

	if (false == test_result)
	{
		TCU_FAIL("Invalid results.");
	}
}

/* Generates the shader source code for the SubroutineFunctionCalls1
 * array tests, attempts to build and execute test program
 *
 * @tparam API               Tested API descriptor
 *
 * @param tested_shader_type The type of shader that is being tested
 */
template <class API>
void SubroutineFunctionCalls1<API>::test_shader_compilation(
	typename TestCaseBase<API>::TestShaderType tested_shader_type)
{
	static const glcts::test_var_type var_types_set_es[] = { VAR_TYPE_INT,   VAR_TYPE_FLOAT, VAR_TYPE_IVEC2,
															 VAR_TYPE_IVEC3, VAR_TYPE_IVEC4, VAR_TYPE_VEC2,
															 VAR_TYPE_VEC3,  VAR_TYPE_VEC4,  VAR_TYPE_MAT2,
															 VAR_TYPE_MAT3,  VAR_TYPE_MAT4 };
	static const size_t num_var_types_es = sizeof(var_types_set_es) / sizeof(var_types_set_es[0]);

	static const glcts::test_var_type var_types_set_gl[] = { VAR_TYPE_INT,   VAR_TYPE_FLOAT, VAR_TYPE_IVEC2,
															 VAR_TYPE_IVEC3, VAR_TYPE_IVEC4, VAR_TYPE_VEC2,
															 VAR_TYPE_VEC3,  VAR_TYPE_VEC4,  VAR_TYPE_MAT2,
															 VAR_TYPE_MAT3,  VAR_TYPE_MAT4,  VAR_TYPE_DOUBLE,
															 VAR_TYPE_DMAT2, VAR_TYPE_DMAT3, VAR_TYPE_DMAT4 };
	static const size_t num_var_types_gl = sizeof(var_types_set_gl) / sizeof(var_types_set_gl[0]);

	const std::string iteration_loop_end = "                }\n"
										   "            }\n"
										   "        }\n"
										   "    }\n";
	const std::string iteration_loop_start = "    for (uint a = 0u; a < 2u; a++)\n"
											 "    {\n"
											 "        for (uint b = 0u; b < 2u; b++)\n"
											 "        {\n"
											 "            for (uint c = 0u; c < 2u; c++)\n"
											 "            {\n"
											 "                for (uint d = 0u; d < 2u; d++)\n"
											 "                {\n";
	const glcts::test_var_type* var_types_set = var_types_set_es;
	size_t						num_var_types = num_var_types_es;
	const bool					test_compute  = (TestCaseBase<API>::COMPUTE_SHADER_TYPE == tested_shader_type);

	if (API::USE_DOUBLE)
	{
		var_types_set = var_types_set_gl;
		num_var_types = num_var_types_gl;
	}

	for (size_t var_type_index = 0; var_type_index < num_var_types; var_type_index++)
	{
		_supported_variable_types_map_const_iterator var_iterator =
			supported_variable_types_map.find(var_types_set[var_type_index]);

		if (var_iterator != supported_variable_types_map.end())
		{
			std::string iterator_declaration = "    " + var_iterator->second.iterator_type +
											   " iterator = " + var_iterator->second.iterator_initialization + ";\n";

			std::string function_definition;
			std::string function_use;
			std::string verification;

			function_definition += "// Subroutine types\n"
								   "subroutine void out_routine_type(out ";
			function_definition += var_iterator->second.type;
			function_definition += " output_array[2][2][2][2]);\n\n"
								   "// Subroutine definitions\n"
								   "subroutine(out_routine_type) void original_routine(out ";
			function_definition += var_iterator->second.type;
			function_definition += " output_array[2][2][2][2]) {\n";
			function_definition += iterator_declaration;
			function_definition += iteration_loop_start;
			function_definition += "                                   output_array[a][b][c][d] = " +
								   var_iterator->second.variable_type_initializer1 + ";\n";
			function_definition +=
				"                                   iterator += " + var_iterator->second.iterator_type + "(1);\n";
			function_definition += iteration_loop_end;
			function_definition += "}\n\n";
			function_definition += "subroutine(out_routine_type) void new_routine(out ";
			function_definition += var_iterator->second.type;
			function_definition += " output_array[2][2][2][2]) {\n";
			function_definition += iterator_declaration;
			function_definition += iteration_loop_start;
			function_definition += "                                   output_array[a][b][c][d] = " +
								   var_iterator->second.variable_type_initializer1 + ";\n";
			function_definition +=
				"                                   iterator -= " + var_iterator->second.iterator_type + "(1);\n";
			function_definition += iteration_loop_end;
			function_definition += "}\n\n"
								   "// Subroutine uniform\n"
								   "subroutine uniform out_routine_type routine;\n";

			function_use = "    " + var_iterator->second.type + " my_array[2][2][2][2];\n";
			function_use += "    routine(my_array);";

			verification = iterator_declaration;
			verification += "    float result = 1.0;\n";
			verification += iteration_loop_start;
			verification += "                                   if (my_array[a][b][c][d] " +
							var_iterator->second.specific_element +
							" != iterator)\n"
							"                                   {\n"
							"                                       result = 0.0;\n"
							"                                   }\n"
							"                                   iterator += " +
							var_iterator->second.iterator_type + "(1);\n";
			verification += iteration_loop_end;

			if (false == test_compute)
			{
				execute_draw_test(tested_shader_type, function_definition, function_use, verification, false, true);
				execute_draw_test(tested_shader_type, function_definition, function_use, verification, true, false);
			}
			else
			{
				execute_dispatch_test(tested_shader_type, function_definition, function_use, verification, false, true);
				execute_dispatch_test(tested_shader_type, function_definition, function_use, verification, true, false);
			}

			/* Deallocate any resources used. */
			this->delete_objects();
		} /* if var_type iterator found */
		else
		{
			TCU_FAIL("Type not found.");
		}
	} /* for (int var_type_index = 0; ...) */
}

/** Executes test for compute program
 *
 * @tparam API                  Tested API descriptor
 *
 * @param tested_shader_type    The type of shader that is being tested
 * @param function_definition   Definition used to prepare shader
 * @param function_use          Use of definition
 * @param verification          Result verification
 * @param use_original          Selects if "original_routine" - true or "new_routine" is choosen
 * @param expect_invalid_result Does test expects invalid results
 **/
template <class API>
void SubroutineFunctionCalls1<API>::execute_dispatch_test(typename TestCaseBase<API>::TestShaderType tested_shader_type,
														  const std::string& function_definition,
														  const std::string& function_use,
														  const std::string& verification, bool use_original,
														  bool expect_invalid_result)
{
	const std::string& compute_shader_source =
		prepare_compute_shader(tested_shader_type, function_definition, function_use, verification);
	const glw::Functions& gl = this->context_id.getRenderContext().getFunctions();

	this->execute_positive_test(empty_string, empty_string, empty_string, empty_string, empty_string,
								compute_shader_source, false, false);

	/* We are now ready to verify whether the returned size is correct. */
	unsigned char	  buffer[4]			 = { 0 };
	glw::GLuint		   framebuffer_object_id = 0;
	glw::GLint		   location				 = -1;
	glw::GLuint		   routine_index		 = -1;
	glw::GLuint		   routine_location		 = -1;
	const glw::GLchar* routine_name			 = "original_routine";
	const glw::GLenum  shader_type			 = GL_COMPUTE_SHADER;
	glw::GLuint		   texture_object_id	 = 0;

	if (false == use_original)
	{
		routine_name = "new_routine";
	}

	gl.useProgram(this->program_object_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram() failed.");

	/* Select subroutine */
	routine_index = gl.getSubroutineIndex(this->program_object_id, shader_type, routine_name);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetSubroutineIndex() failed.");

	routine_location = gl.getSubroutineUniformLocation(this->program_object_id, shader_type, "routine");
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetSubroutineUniformLocation() failed.");

	if (0 != routine_location)
	{
		TCU_FAIL("Subroutine location is invalid");
	}

	gl.uniformSubroutinesuiv(shader_type, 1, &routine_index);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUniformSubroutinesuiv() failed.");

	/* Prepare texture */
	gl.genTextures(1, &texture_object_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenTextures() failed.");

	gl.bindTexture(GL_TEXTURE_2D, texture_object_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture() failed.");

	gl.texStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, 1, 1);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTexStorage2D() failed.");

	gl.bindImageTexture(0 /* image unit */, texture_object_id, 0 /* level */, GL_FALSE /* layered */, 0 /* layer */,
						GL_WRITE_ONLY, GL_RGBA8);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindImageTexture() failed.");

	location = gl.getUniformLocation(this->program_object_id, "uni_image");
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetUniformLocation() failed.");

	if (-1 == location)
	{
		TCU_FAIL("Uniform is inactive");
	}

	gl.uniform1i(location, 0 /* image unit */);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUniform1i() failed.");

	/* Execute */
	gl.dispatchCompute(1, 1, 1);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawArrays() failed.");

	/* Verify */
	gl.genFramebuffers(1, &framebuffer_object_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenFramebuffers() failed.");

	gl.bindFramebuffer(GL_FRAMEBUFFER, framebuffer_object_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffer() failed.");

	gl.framebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture_object_id, 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glFramebufferTexture2D() failed.");

	gl.viewport(0, 0, 1, 1);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glViewport() failed.");

	gl.readBuffer(GL_COLOR_ATTACHMENT0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glReadBuffer() failed.");

	gl.readPixels(0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, buffer);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glReadPixels() failed.");

	if ((buffer[0] != 255) != expect_invalid_result)
	{
		TCU_FAIL("Invalid result was returned.");
	}

	/* Delete generated objects. */
	gl.deleteTextures(1, &texture_object_id);
	gl.deleteFramebuffers(1, &framebuffer_object_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "An error ocurred while deleting generated objects.");
}

/** Executes test for draw program
 *
 * @tparam API                  Tested API descriptor
 *
 * @param tested_shader_type    The type of shader that is being tested
 * @param function_definition   Definition used to prepare shader
 * @param function_use          Use of definition
 * @param verification          Result verification
 * @param use_original          Selects if "original_routine" - true or "new_routine" is choosen
 * @param expect_invalid_result Does test expects invalid results
 **/
template <class API>
void SubroutineFunctionCalls1<API>::execute_draw_test(typename TestCaseBase<API>::TestShaderType tested_shader_type,
													  const std::string&						 function_definition,
													  const std::string& function_use, const std::string& verification,
													  bool use_original, bool expect_invalid_result)
{
	const glw::Functions& gl = this->context_id.getRenderContext().getFunctions();

	if (API::USE_ALL_SHADER_STAGES)
	{
		const std::string& compute_shader_source = empty_string;
		const std::string& fragment_shader_source =
			this->prepare_fragment_shader(tested_shader_type, function_definition, function_use, verification);
		const std::string& geometry_shader_source =
			this->prepare_geometry_shader(tested_shader_type, function_definition, function_use, verification);
		const std::string& tess_ctrl_shader_source =
			this->prepare_tess_ctrl_shader(tested_shader_type, function_definition, function_use, verification);
		const std::string& tess_eval_shader_source =
			this->prepare_tess_eval_shader(tested_shader_type, function_definition, function_use, verification);
		const std::string& vertex_shader_source =
			this->prepare_vertex_shader(tested_shader_type, function_definition, function_use, verification);

		switch (tested_shader_type)
		{
		case TestCaseBase<API>::VERTEX_SHADER_TYPE: /* Fall through */
		case TestCaseBase<API>::FRAGMENT_SHADER_TYPE:
			this->execute_positive_test(vertex_shader_source, fragment_shader_source, false, false);
			break;

		case TestCaseBase<API>::COMPUTE_SHADER_TYPE:
		case TestCaseBase<API>::GEOMETRY_SHADER_TYPE:
		case TestCaseBase<API>::TESSELATION_CONTROL_SHADER_TYPE: /* Fall through */
		case TestCaseBase<API>::TESSELATION_EVALUATION_SHADER_TYPE:
			this->execute_positive_test(vertex_shader_source, tess_ctrl_shader_source, tess_eval_shader_source,
										geometry_shader_source, fragment_shader_source, compute_shader_source, false,
										false);
			break;

		default:
			TCU_FAIL("Invalid enum");
			break;
		}
	}
	else
	{
		const std::string& fragment_shader_source =
			this->prepare_fragment_shader(tested_shader_type, function_definition, function_use, verification);
		const std::string& vertex_shader_source =
			this->prepare_vertex_shader(tested_shader_type, function_definition, function_use, verification);

		this->execute_positive_test(vertex_shader_source, fragment_shader_source, false, false);
	}

	/* We are now ready to verify whether the returned size is correct. */
	unsigned char	  buffer[4]			 = { 0 };
	glw::GLuint		   framebuffer_object_id = 0;
	glw::GLuint		   routine_index		 = -1;
	glw::GLuint		   routine_location		 = -1;
	const glw::GLchar* routine_name			 = "original_routine";
	glw::GLenum		   shader_type			 = 0;
	glw::GLuint		   texture_object_id	 = 0;
	glw::GLuint		   vao_id				 = 0;

	switch (tested_shader_type)
	{
	case TestCaseBase<API>::FRAGMENT_SHADER_TYPE:
		shader_type = GL_FRAGMENT_SHADER;
		break;
	case TestCaseBase<API>::VERTEX_SHADER_TYPE:
		shader_type = GL_VERTEX_SHADER;
		break;
	case TestCaseBase<API>::COMPUTE_SHADER_TYPE:
		shader_type = GL_COMPUTE_SHADER;
		break;
	case TestCaseBase<API>::GEOMETRY_SHADER_TYPE:
		shader_type = GL_GEOMETRY_SHADER;
		break;
	case TestCaseBase<API>::TESSELATION_CONTROL_SHADER_TYPE:
		shader_type = GL_TESS_CONTROL_SHADER;
		break;
	case TestCaseBase<API>::TESSELATION_EVALUATION_SHADER_TYPE:
		shader_type = GL_TESS_EVALUATION_SHADER;
		break;
	default:
		TCU_FAIL("Invalid shader type");
		break;
	}

	if (false == use_original)
	{
		routine_name = "new_routine";
	}

	gl.useProgram(this->program_object_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram() failed.");

	/* Select subroutine */
	routine_index = gl.getSubroutineIndex(this->program_object_id, shader_type, routine_name);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetSubroutineIndex() failed.");

	routine_location = gl.getSubroutineUniformLocation(this->program_object_id, shader_type, "routine");
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetSubroutineUniformLocation() failed.");

	if (0 != routine_location)
	{
		TCU_FAIL("Subroutine location is invalid");
	}

	gl.uniformSubroutinesuiv(shader_type, 1, &routine_index);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUniformSubroutinesuiv() failed.");

	/* Prepre texture */
	assert(0 == texture_object_id);
	gl.genTextures(1, &texture_object_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenTextures() failed.");

	gl.bindTexture(GL_TEXTURE_2D, texture_object_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture() failed.");

	gl.texStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, 1, 1);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTexStorage2D() failed.");

	/* Prepare framebuffer */
	assert(0 == framebuffer_object_id);
	gl.genFramebuffers(1, &framebuffer_object_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenFramebuffers() failed.");

	gl.bindFramebuffer(GL_FRAMEBUFFER, framebuffer_object_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffer() failed.");

	gl.framebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture_object_id, 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glFramebufferTexture2D() failed.");

	gl.viewport(0, 0, 1, 1);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glViewport() failed.");

	/* Set VAO */
	assert(0 == vao_id);
	gl.genVertexArrays(1, &vao_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenVertexArrays() failed.");

	gl.bindVertexArray(vao_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindVertexArray() failed.");

	switch (tested_shader_type)
	{
	case TestCaseBase<API>::FRAGMENT_SHADER_TYPE: /* Fall through */
	case TestCaseBase<API>::VERTEX_SHADER_TYPE:
		gl.drawArrays(GL_TRIANGLE_FAN, 0, 4);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawArrays() failed.");
		break;

	case TestCaseBase<API>::TESSELATION_CONTROL_SHADER_TYPE: /* Fall through */
	case TestCaseBase<API>::TESSELATION_EVALUATION_SHADER_TYPE:
		/* Tesselation patch set up */
		gl.patchParameteri(GL_PATCH_VERTICES, 1);
		GLU_EXPECT_NO_ERROR(gl.getError(), "PatchParameteri");

		gl.drawArrays(GL_PATCHES, 0, 1);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawArrays() failed.");
		break;

	case TestCaseBase<API>::GEOMETRY_SHADER_TYPE:
		gl.drawArrays(GL_POINTS, 0, 1);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawArrays() failed.");
		break;

	default:
		TCU_FAIL("Invalid enum");
		break;
	}

	/* Verify */
	gl.readBuffer(GL_COLOR_ATTACHMENT0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glReadBuffer() failed.");

	gl.readPixels(0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, buffer);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glReadPixels() failed.");

	const bool result = ((buffer[0] != 255) == expect_invalid_result);

	/* Delete generated objects. */
	gl.useProgram(0);
	gl.bindTexture(GL_TEXTURE_2D, 0);
	gl.bindFramebuffer(GL_FRAMEBUFFER, 0);
	gl.bindVertexArray(0);

	gl.deleteProgram(this->program_object_id);
	this->program_object_id = 0;

	gl.deleteTextures(1, &texture_object_id);
	texture_object_id = 0;

	gl.deleteFramebuffers(1, &framebuffer_object_id);
	framebuffer_object_id = 0;

	gl.deleteVertexArrays(1, &vao_id);
	vao_id = 0;

	GLU_EXPECT_NO_ERROR(gl.getError(), "An error ocurred while deleting generated objects.");

	if (!result)
	{
		TCU_FAIL("Invalid result was returned.");
	}
}

/** Prepare shader
 *
 * @tparam API               Tested API descriptor
 *
 * @param tested_shader_type    The type of shader that is being tested
 * @param function_definition   Definition used to prepare shader
 * @param function_use          Use of definition
 * @param verification          Result verification
 **/
template <class API>
std::string SubroutineFunctionCalls1<API>::prepare_compute_shader(
	typename TestCaseBase<API>::TestShaderType tested_shader_type, const std::string& function_definition,
	const std::string& function_use, const std::string& verification)
{
	std::string compute_shader_source;

	if (TestCaseBase<API>::COMPUTE_SHADER_TYPE == tested_shader_type)
	{
		compute_shader_source = "writeonly uniform image2D uni_image;\n"
								"\n";

		/* User-defined function definition. */
		compute_shader_source += function_definition;
		compute_shader_source += "\n\n";

		/* Main function definition. */
		compute_shader_source += shader_start;
		compute_shader_source += function_use;
		compute_shader_source += "\n\n";
		compute_shader_source += verification;
		compute_shader_source += "\n\n";
		compute_shader_source += "\n"
								 "    imageStore(uni_image, ivec2(gl_GlobalInvocationID.xy), vec4(result, 0, 0, 0));\n"
								 "}\n"
								 "\n";
	}

	return compute_shader_source;
}

/** Prepare shader
 *
 * @tparam API               Tested API descriptor
 *
 * @param tested_shader_type    The type of shader that is being tested
 * @param function_definition   Definition used to prepare shader
 * @param function_use          Use of definition
 * @param verification          Result verification
 **/
template <class API>
std::string SubroutineFunctionCalls1<API>::prepare_fragment_shader(
	typename TestCaseBase<API>::TestShaderType tested_shader_type, const std::string& function_definition,
	const std::string& function_use, const std::string& verification)
{
	std::string fragment_shader_source;

	switch (tested_shader_type)
	{
	case TestCaseBase<API>::COMPUTE_SHADER_TYPE:
		break;

	case TestCaseBase<API>::FRAGMENT_SHADER_TYPE:
		fragment_shader_source = "out vec4 colour;\n\n";

		/* User-defined function definition. */
		fragment_shader_source += function_definition;
		fragment_shader_source += "\n\n";

		/* Main function definition. */
		fragment_shader_source += shader_start;
		fragment_shader_source += function_use;
		fragment_shader_source += "\n\n";
		fragment_shader_source += verification;
		fragment_shader_source += "\n\n";
		fragment_shader_source += "    colour = vec4(result);\n";
		fragment_shader_source += shader_end;
		break;

	case TestCaseBase<API>::GEOMETRY_SHADER_TYPE:
	case TestCaseBase<API>::TESSELATION_CONTROL_SHADER_TYPE:
	case TestCaseBase<API>::TESSELATION_EVALUATION_SHADER_TYPE: /* Fall through */
	case TestCaseBase<API>::VERTEX_SHADER_TYPE:
		fragment_shader_source = "in float fs_result;\n\n"
								 "out vec4 colour;\n\n"
								 "void main()\n"
								 "{\n"
								 "    colour =  vec4(fs_result);\n"
								 "}\n"
								 "\n";
		break;

	default:
		TCU_FAIL("Unrecognized shader object type.");
		break;
	}

	return fragment_shader_source;
}

/** Prepare shader
 *
 * @tparam API               Tested API descriptor
 *
 * @param tested_shader_type    The type of shader that is being tested
 * @param function_definition   Definition used to prepare shader
 * @param function_use          Use of definition
 * @param verification          Result verification
 **/
template <class API>
std::string SubroutineFunctionCalls1<API>::prepare_geometry_shader(
	typename TestCaseBase<API>::TestShaderType tested_shader_type, const std::string& function_definition,
	const std::string& function_use, const std::string& verification)
{
	std::string geometry_shader_source;

	switch (tested_shader_type)
	{
	case TestCaseBase<API>::COMPUTE_SHADER_TYPE:
	case TestCaseBase<API>::FRAGMENT_SHADER_TYPE: /* Fall through */
	case TestCaseBase<API>::VERTEX_SHADER_TYPE:
		break;

	case TestCaseBase<API>::TESSELATION_CONTROL_SHADER_TYPE: /* Fall through */
	case TestCaseBase<API>::TESSELATION_EVALUATION_SHADER_TYPE:
		geometry_shader_source = "layout(points)                           in;\n"
								 "layout(triangle_strip, max_vertices = 4) out;\n"
								 "\n"
								 "in  float tes_result[];\n"
								 "out float fs_result;\n"
								 "\n"
								 "void main()\n"
								 "{\n"
								 "    gl_Position  = vec4(-1, -1, 0, 1);\n"
								 "    fs_result    = tes_result[0];\n"
								 "    EmitVertex();\n"
								 "    gl_Position  = vec4(-1, 1, 0, 1);\n"
								 "    fs_result    = tes_result[0];\n"
								 "    EmitVertex();\n"
								 "    gl_Position  = vec4(1, -1, 0, 1);\n"
								 "    fs_result    = tes_result[0];\n"
								 "    EmitVertex();\n"
								 "    gl_Position  = vec4(1, 1, 0, 1);\n"
								 "    fs_result    = tes_result[0];\n"
								 "    EmitVertex();\n"
								 "}\n";
		break;

	case TestCaseBase<API>::GEOMETRY_SHADER_TYPE:
		geometry_shader_source = "layout(points)                           in;\n"
								 "layout(triangle_strip, max_vertices = 4) out;\n"
								 "\n"
								 "out float fs_result;\n"
								 "\n";

		/* User-defined function definition. */
		geometry_shader_source += function_definition;
		geometry_shader_source += "\n\n";

		/* Main function definition. */
		geometry_shader_source += shader_start;
		geometry_shader_source += function_use;
		geometry_shader_source += "\n\n";
		geometry_shader_source += verification;
		geometry_shader_source += "\n\n";
		geometry_shader_source += "\n    gl_Position  = vec4(-1, -1, 0, 1);\n"
								  "    fs_result    = result;\n"
								  "    EmitVertex();\n"
								  "    gl_Position  = vec4(-1, 1, 0, 1);\n"
								  "    fs_result    = result;\n"
								  "    EmitVertex();\n"
								  "    gl_Position  = vec4(1, -1, 0, 1);\n"
								  "    fs_result    = result;\n"
								  "    EmitVertex();\n"
								  "    gl_Position  = vec4(1, 1, 0, 1);\n"
								  "    fs_result    = result;\n"
								  "    EmitVertex();\n"
								  "}\n";
		break;

	default:
		TCU_FAIL("Unrecognized shader object type.");
		break;
	}

	return geometry_shader_source;
}

/** Prepare shader
 *
 * @tparam API               Tested API descriptor
 *
 * @param tested_shader_type    The type of shader that is being tested
 * @param function_definition   Definition used to prepare shader
 * @param function_use          Use of definition
 * @param verification          Result verification
 **/
template <class API>
std::string SubroutineFunctionCalls1<API>::prepare_tess_ctrl_shader(
	typename TestCaseBase<API>::TestShaderType tested_shader_type, const std::string& function_definition,
	const std::string& function_use, const std::string& verification)
{
	std::string tess_ctrl_shader_source;

	switch (tested_shader_type)
	{
	case TestCaseBase<API>::COMPUTE_SHADER_TYPE:
	case TestCaseBase<API>::FRAGMENT_SHADER_TYPE:
	case TestCaseBase<API>::GEOMETRY_SHADER_TYPE:
	case TestCaseBase<API>::VERTEX_SHADER_TYPE:
		break;

	case TestCaseBase<API>::TESSELATION_CONTROL_SHADER_TYPE:
		tess_ctrl_shader_source = "layout(vertices = 1) out;\n"
								  "\n"
								  "out float tcs_result[];\n"
								  "\n";

		/* User-defined function definition. */
		tess_ctrl_shader_source += function_definition;
		tess_ctrl_shader_source += "\n\n";

		/* Main function definition. */
		tess_ctrl_shader_source += shader_start;
		tess_ctrl_shader_source += function_use;
		tess_ctrl_shader_source += "\n\n";
		tess_ctrl_shader_source += verification;
		tess_ctrl_shader_source += "\n\n";
		tess_ctrl_shader_source += "    tcs_result[gl_InvocationID] = result;\n"
								   "\n"
								   "    gl_TessLevelOuter[0] = 1.0;\n"
								   "    gl_TessLevelOuter[1] = 1.0;\n"
								   "    gl_TessLevelOuter[2] = 1.0;\n"
								   "    gl_TessLevelOuter[3] = 1.0;\n"
								   "    gl_TessLevelInner[0] = 1.0;\n"
								   "    gl_TessLevelInner[1] = 1.0;\n"
								   "}\n";
		break;

	case TestCaseBase<API>::TESSELATION_EVALUATION_SHADER_TYPE:
		tess_ctrl_shader_source = default_tc_shader_source;
		break;

	default:
		TCU_FAIL("Unrecognized shader object type.");
		break;
	}

	return tess_ctrl_shader_source;
}

/** Prepare shader
 *
 * @tparam API               Tested API descriptor
 *
 * @param tested_shader_type    The type of shader that is being tested
 * @param function_definition   Definition used to prepare shader
 * @param function_use          Use of definition
 * @param verification          Result verification
 **/
template <class API>
std::string SubroutineFunctionCalls1<API>::prepare_tess_eval_shader(
	typename TestCaseBase<API>::TestShaderType tested_shader_type, const std::string& function_definition,
	const std::string& function_use, const std::string& verification)
{
	std::string tess_eval_shader_source;

	switch (tested_shader_type)
	{
	case TestCaseBase<API>::COMPUTE_SHADER_TYPE:
	case TestCaseBase<API>::FRAGMENT_SHADER_TYPE:
	case TestCaseBase<API>::GEOMETRY_SHADER_TYPE:
	case TestCaseBase<API>::VERTEX_SHADER_TYPE:
		break;

	case TestCaseBase<API>::TESSELATION_CONTROL_SHADER_TYPE:
		tess_eval_shader_source = "layout(isolines, point_mode) in;\n"
								  "\n"
								  "in  float tcs_result[];\n"
								  "out float tes_result;\n"
								  "\n"
								  "void main()\n"
								  "{\n"
								  "    tes_result = tcs_result[0];\n"
								  "}\n";
		break;

	case TestCaseBase<API>::TESSELATION_EVALUATION_SHADER_TYPE:
		tess_eval_shader_source = "layout(isolines, point_mode) in;\n"
								  "\n"
								  "out float tes_result;\n"
								  "\n";

		/* User-defined function definition. */
		tess_eval_shader_source += function_definition;
		tess_eval_shader_source += "\n\n";

		/* Main function definition. */
		tess_eval_shader_source += shader_start;
		tess_eval_shader_source += function_use;
		tess_eval_shader_source += "\n\n";
		tess_eval_shader_source += verification;
		tess_eval_shader_source += "\n\n";
		tess_eval_shader_source += "    tes_result = result;\n"
								   "}\n";
		break;

	default:
		TCU_FAIL("Unrecognized shader object type.");
		break;
	}

	return tess_eval_shader_source;
}

/** Prepare shader
 *
 * @tparam API               Tested API descriptor
 *
 * @param tested_shader_type    The type of shader that is being tested
 * @param function_definition   Definition used to prepare shader
 * @param function_use          Use of definition
 * @param verification          Result verification
 **/
template <class API>
std::string SubroutineFunctionCalls1<API>::prepare_vertex_shader(
	typename TestCaseBase<API>::TestShaderType tested_shader_type, const std::string& function_definition,
	const std::string& function_use, const std::string& verification)
{
	std::string vertex_shader_source;

	switch (tested_shader_type)
	{
	case TestCaseBase<API>::COMPUTE_SHADER_TYPE:
		break;

	case TestCaseBase<API>::FRAGMENT_SHADER_TYPE:
		vertex_shader_source = "/** GL_TRIANGLE_FAN-type quad vertex data. */\n"
							   "const vec4 vertex_positions[4] = vec4[4](vec4( 1.0, -1.0, 0.0, 1.0),\n"
							   "                                         vec4(-1.0, -1.0, 0.0, 1.0),\n"
							   "                                         vec4(-1.0,  1.0, 0.0, 1.0),\n"
							   "                                         vec4( 1.0,  1.0, 0.0, 1.0) );\n"
							   "\n"
							   "void main()\n"
							   "{\n"
							   "    gl_Position = vertex_positions[gl_VertexID];"
							   "}\n\n";
		break;

	case TestCaseBase<API>::GEOMETRY_SHADER_TYPE:
	case TestCaseBase<API>::TESSELATION_CONTROL_SHADER_TYPE:
	case TestCaseBase<API>::TESSELATION_EVALUATION_SHADER_TYPE:
		vertex_shader_source = default_vertex_shader_source;
		break;

	case TestCaseBase<API>::VERTEX_SHADER_TYPE:
		/* Vertex shader source. */
		vertex_shader_source = "out float fs_result;\n\n";
		vertex_shader_source += "/** GL_TRIANGLE_FAN-type quad vertex data. */\n"
								"const vec4 vertex_positions[4] = vec4[4](vec4( 1.0, -1.0, 0.0, 1.0),\n"
								"                                         vec4(-1.0, -1.0, 0.0, 1.0),\n"
								"                                         vec4(-1.0,  1.0, 0.0, 1.0),\n"
								"                                         vec4( 1.0,  1.0, 0.0, 1.0) );\n\n";

		/* User-defined function definition. */
		vertex_shader_source += function_definition;
		vertex_shader_source += "\n\n";

		/* Main function definition. */
		vertex_shader_source += shader_start;
		vertex_shader_source += function_use;
		vertex_shader_source += "\n\n";
		vertex_shader_source += verification;
		vertex_shader_source += "\n\n";
		vertex_shader_source += "    fs_result   = result;\n"
								"    gl_Position = vertex_positions[gl_VertexID];\n";
		vertex_shader_source += shader_end;
		break;

	default:
		TCU_FAIL("Unrecognized shader object type.");
		break;
	}

	return vertex_shader_source;
}

/* Generates the shader source code for the InteractionFunctionCalls2
 * array tests, and attempts to build and execute test program.
 *
 * @tparam API               Tested API descriptor
 *
 * @param tested_shader_type The type of shader that is being tested
 */
template <class API>
void SubroutineFunctionCalls2<API>::test_shader_compilation(
	typename TestCaseBase<API>::TestShaderType tested_shader_type)
{
	static const glcts::test_var_type var_types_set_es[] = { VAR_TYPE_INT,   VAR_TYPE_FLOAT, VAR_TYPE_IVEC2,
															 VAR_TYPE_IVEC3, VAR_TYPE_IVEC4, VAR_TYPE_VEC2,
															 VAR_TYPE_VEC3,  VAR_TYPE_VEC4,  VAR_TYPE_MAT2,
															 VAR_TYPE_MAT3,  VAR_TYPE_MAT4 };
	static const size_t num_var_types_es = sizeof(var_types_set_es) / sizeof(var_types_set_es[0]);

	static const glcts::test_var_type var_types_set_gl[] = { VAR_TYPE_INT,   VAR_TYPE_FLOAT, VAR_TYPE_IVEC2,
															 VAR_TYPE_IVEC3, VAR_TYPE_IVEC4, VAR_TYPE_VEC2,
															 VAR_TYPE_VEC3,  VAR_TYPE_VEC4,  VAR_TYPE_MAT2,
															 VAR_TYPE_MAT3,  VAR_TYPE_MAT4,  VAR_TYPE_DOUBLE,
															 VAR_TYPE_DMAT2, VAR_TYPE_DMAT3, VAR_TYPE_DMAT4 };
	static const size_t num_var_types_gl = sizeof(var_types_set_gl) / sizeof(var_types_set_gl[0]);

	const std::string iteration_loop_end = "                }\n"
										   "            }\n"
										   "        }\n"
										   "    }\n";
	const std::string iteration_loop_start = "    for (uint a = 0u; a < 2u; a++)\n"
											 "    {\n"
											 "        for (uint b = 0u; b < 2u; b++)\n"
											 "        {\n"
											 "            for (uint c = 0u; c < 2u; c++)\n"
											 "            {\n"
											 "                for (uint d = 0u; d < 2u; d++)\n"
											 "                {\n";
	const std::string multiplier_array = "const int[] multiplier_array = int[]( 1,  2,  3,  4,  5,  6,  7,  8,\n"
										 "                                     11, 12, 13, 14, 15, 16, 17, 18,\n"
										 "                                     21, 22, 23, 24, 25, 26, 27, 28,\n"
										 "                                     31, 32, 33, 34, 35, 36, 37, 38,\n"
										 "                                     41, 42, 43, 44, 45, 46, 47, 48,\n"
										 "                                     51, 52, 53, 54, 55, 56, 57, 58,\n"
										 "                                     61, 62, 63, 64, 65, 66, 67, 68,\n"
										 "                                     71, 72, 73, 74, 75, 76, 77, 78,\n"
										 "                                     81, 82, 83, 84, 85, 86, 87, 88);\n";
	const glcts::test_var_type* var_types_set = var_types_set_es;
	size_t						num_var_types = num_var_types_es;
	const bool					test_compute  = (TestCaseBase<API>::COMPUTE_SHADER_TYPE == tested_shader_type);

	if (API::USE_DOUBLE)
	{
		var_types_set = var_types_set_gl;
		num_var_types = num_var_types_gl;
	}

	for (size_t var_type_index = 0; var_type_index < num_var_types; var_type_index++)
	{
		_supported_variable_types_map_const_iterator var_iterator =
			supported_variable_types_map.find(var_types_set[var_type_index]);

		if (var_iterator != supported_variable_types_map.end())
		{
			std::string function_definition;
			std::string function_use;
			std::string verification;

			function_definition += multiplier_array;

			function_definition += "// Subroutine types\n"
								   "subroutine void inout_routine_type(inout ";
			function_definition += var_iterator->second.type;
			function_definition += " inout_array[2][2][2][2]);\n\n"
								   "// Subroutine definitions\n"
								   "subroutine(inout_routine_type) void original_routine(inout ";
			function_definition += var_iterator->second.type;
			function_definition += " inout_array[2][2][2][2]) {\n"
								   "    uint i = 0u;\n";
			function_definition += iteration_loop_start;
			function_definition += "                                   inout_array[a][b][c][d] *= " +
								   var_iterator->second.iterator_type + "(multiplier_array[i % 64u]);\n";
			function_definition += "                                   i+= 1u;\n";
			function_definition += iteration_loop_end;
			function_definition += "}\n\n"
								   "subroutine(inout_routine_type) void new_routine(inout ";
			function_definition += var_iterator->second.type;
			function_definition += " inout_array[2][2][2][2]) {\n"
								   "    uint i = 0u;\n";
			function_definition += iteration_loop_start;
			function_definition += "                                   inout_array[a][b][c][d] /= " +
								   var_iterator->second.iterator_type + "(multiplier_array[i % 64u]);\n";
			function_definition += "                                   i+= 1u;\n";
			function_definition += iteration_loop_end;
			function_definition += "}\n\n"
								   "// Subroutine uniform\n"
								   "subroutine uniform inout_routine_type routine;\n";

			function_use += "    float result = 1.0;\n";
			function_use += "    uint iterator = 0u;\n";
			function_use += "    " + var_iterator->second.type + " my_array[2][2][2][2];\n";
			function_use += iteration_loop_start;
			function_use += "                                   my_array[a][b][c][d] = " +
							var_iterator->second.variable_type_initializer2 + ";\n";
			function_use += iteration_loop_end;
			function_use += "    routine(my_array);";

			verification += iteration_loop_start;
			verification += "                                   if (my_array[a][b][c][d] " +
							var_iterator->second.specific_element + "!= " + var_iterator->second.iterator_type +
							"(multiplier_array[iterator % 64u]))\n"
							"                                   {\n"
							"                                       result = 0.0;\n"
							"                                   }\n"
							"                                   iterator += 1u;\n";
			verification += iteration_loop_end;

			if (false == test_compute)
			{
				this->execute_draw_test(tested_shader_type, function_definition, function_use, verification, false,
										true);
				this->execute_draw_test(tested_shader_type, function_definition, function_use, verification, true,
										false);
			}
			else
			{
				this->execute_dispatch_test(tested_shader_type, function_definition, function_use, verification, false,
											true);
				this->execute_dispatch_test(tested_shader_type, function_definition, function_use, verification, true,
											false);
			}

			/* Deallocate any resources used. */
			this->delete_objects();
		} /* if var_type iterator found */
		else
		{
			TCU_FAIL("Type not found.");
		}
	} /* for (int var_type_index = 0; ...) */
}

/* Generates the shader source code for the SubroutineArgumentAliasing1
 * array tests, attempts to build and execute test program
 *
 * @tparam API               Tested API descriptor
 *
 * @param tested_shader_type The type of shader that is being tested
 */
template <class API>
void SubroutineArgumentAliasing1<API>::test_shader_compilation(
	typename TestCaseBase<API>::TestShaderType tested_shader_type)
{
	static const glcts::test_var_type var_types_set_es[] = { VAR_TYPE_INT,   VAR_TYPE_FLOAT, VAR_TYPE_IVEC2,
															 VAR_TYPE_IVEC3, VAR_TYPE_IVEC4, VAR_TYPE_VEC2,
															 VAR_TYPE_VEC3,  VAR_TYPE_VEC4,  VAR_TYPE_MAT2,
															 VAR_TYPE_MAT3,  VAR_TYPE_MAT4 };
	static const size_t num_var_types_es = sizeof(var_types_set_es) / sizeof(var_types_set_es[0]);

	static const glcts::test_var_type var_types_set_gl[] = { VAR_TYPE_INT,   VAR_TYPE_FLOAT, VAR_TYPE_IVEC2,
															 VAR_TYPE_IVEC3, VAR_TYPE_IVEC4, VAR_TYPE_VEC2,
															 VAR_TYPE_VEC3,  VAR_TYPE_VEC4,  VAR_TYPE_MAT2,
															 VAR_TYPE_MAT3,  VAR_TYPE_MAT4,  VAR_TYPE_DOUBLE,
															 VAR_TYPE_DMAT2, VAR_TYPE_DMAT3, VAR_TYPE_DMAT4 };
	static const size_t num_var_types_gl = sizeof(var_types_set_gl) / sizeof(var_types_set_gl[0]);

	const std::string iteration_loop_end = "                }\n"
										   "            }\n"
										   "        }\n"
										   "    }\n";
	const std::string iteration_loop_start = "    for (uint a = 0u; a < 2u; a++)\n"
											 "    {\n"
											 "        for (uint b = 0u; b < 2u; b++)\n"
											 "        {\n"
											 "            for (uint c = 0u; c < 2u; c++)\n"
											 "            {\n"
											 "                for (uint d = 0u; d < 2u; d++)\n"
											 "                {\n";
	const glcts::test_var_type* var_types_set = var_types_set_es;
	size_t						num_var_types = num_var_types_es;
	const bool					test_compute  = (TestCaseBase<API>::COMPUTE_SHADER_TYPE == tested_shader_type);

	if (API::USE_DOUBLE)
	{
		var_types_set = var_types_set_gl;
		num_var_types = num_var_types_gl;
	}

	for (size_t var_type_index = 0; var_type_index < num_var_types; var_type_index++)
	{
		_supported_variable_types_map_const_iterator var_iterator =
			supported_variable_types_map.find(var_types_set[var_type_index]);

		if (var_iterator != supported_variable_types_map.end())
		{
			std::string function_definition;
			std::string function_use;
			std::string verification;

			function_definition += "// Subroutine types\n"
								   "subroutine bool in_routine_type(";
			function_definition += var_iterator->second.type;
			function_definition += " x[2][2][2][2], ";
			function_definition += var_iterator->second.type;
			function_definition += " y[2][2][2][2]);\n\n"
								   "// Subroutine definitions\n"
								   "subroutine(in_routine_type) bool original_routine(";
			function_definition += var_iterator->second.type;
			function_definition += " x[2][2][2][2], ";
			function_definition += var_iterator->second.type;
			function_definition += " y[2][2][2][2])\n{\n";
			function_definition += iteration_loop_start;
			function_definition +=
				"                                   x[a][b][c][d] = " + var_iterator->second.type + "(123);\n";
			function_definition += iteration_loop_end;
			function_definition += "\n";
			function_definition += iteration_loop_start;
			function_definition += "                                   if(y[a][b][c][d]";
			if (var_iterator->second.type == "mat4") // mat4 comparison
			{
				function_definition += "[0][0]";
				function_definition += " != float";
			}
			else if (var_iterator->second.type == "dmat4") // dmat4 comparison
			{
				function_definition += "[0][0]";
				function_definition += " != double";
			}
			else
			{
				function_definition += " != ";
				function_definition += var_iterator->second.type;
			}
			function_definition += "(((a*8u)+(b*4u)+(c*2u)+d))) {return false;}\n";
			function_definition += iteration_loop_end;
			function_definition += "\n    return true;\n";
			function_definition += "}\n\n"
								   "subroutine(in_routine_type) bool new_routine(";
			function_definition += var_iterator->second.type;
			function_definition += " x[2][2][2][2], ";
			function_definition += var_iterator->second.type;
			function_definition += " y[2][2][2][2])\n{\n";
			function_definition += iteration_loop_start;
			function_definition +=
				"                                   y[a][b][c][d] = " + var_iterator->second.type + "(123);\n";
			function_definition += iteration_loop_end;
			function_definition += "\n";
			function_definition += iteration_loop_start;
			function_definition += "                                   if(x[a][b][c][d]";
			if (var_iterator->second.type == "mat4") // mat4 comparison
			{
				function_definition += "[0][0]";
				function_definition += " != float";
			}
			else if (var_iterator->second.type == "dmat4") // dmat4 comparison
			{
				function_definition += "[0][0]";
				function_definition += " != double";
			}
			else
			{
				function_definition += " != ";
				function_definition += var_iterator->second.type;
			}
			function_definition += "(((a*8u)+(b*4u)+(c*2u)+d))) {return false;}\n";
			function_definition += iteration_loop_end;
			function_definition += "\n    return true;\n";
			function_definition += "}\n\n"
								   "// Subroutine uniform\n"
								   "subroutine uniform in_routine_type routine;\n";

			function_use += "    " + var_iterator->second.type + " z[2][2][2][2];\n";
			function_use += iteration_loop_start;
			function_use += "                                   z[a][b][c][d] = ";
			function_use += var_iterator->second.type;
			function_use += "(((a*8u)+(b*4u)+(c*2u)+d));\n";
			function_use += iteration_loop_end;

			verification = "    float result = 0.0;\n"
						   "    if(routine(z, z) == true)\n"
						   "    {\n"
						   "        result = 1.0;\n"
						   "    }\n"
						   "    else\n"
						   "    {\n"
						   "        result = 0.5;\n"
						   "    }\n";

			if (false == test_compute)
			{
				this->execute_draw_test(tested_shader_type, function_definition, function_use, verification, false,
										false);
				this->execute_draw_test(tested_shader_type, function_definition, function_use, verification, true,
										false);
			}
			else
			{
				this->execute_dispatch_test(tested_shader_type, function_definition, function_use, verification, false,
											false);
				this->execute_dispatch_test(tested_shader_type, function_definition, function_use, verification, true,
											false);
			}

			/* Deallocate any resources used. */
			this->delete_objects();
		} /* if var_type iterator found */
		else
		{
			TCU_FAIL("Type not found.");
		}
	} /* for (int var_type_index = 0; ...) */
}

/* Generates the shader source code for the SubroutineArgumentAliasing1
 * array tests, attempts to build and execute test program
 *
 * @tparam API               Tested API descriptor
 *
 * @param tested_shader_type The type of shader that is being tested
 */
template <class API>
void SubroutineArgumentAliasing2<API>::test_shader_compilation(
	typename TestCaseBase<API>::TestShaderType tested_shader_type)
{
	static const glcts::test_var_type var_types_set_es[] = { VAR_TYPE_INT,   VAR_TYPE_FLOAT, VAR_TYPE_IVEC2,
															 VAR_TYPE_IVEC3, VAR_TYPE_IVEC4, VAR_TYPE_VEC2,
															 VAR_TYPE_VEC3,  VAR_TYPE_VEC4,  VAR_TYPE_MAT2,
															 VAR_TYPE_MAT3,  VAR_TYPE_MAT4 };
	static const size_t num_var_types_es = sizeof(var_types_set_es) / sizeof(var_types_set_es[0]);

	static const glcts::test_var_type var_types_set_gl[] = { VAR_TYPE_INT,   VAR_TYPE_FLOAT, VAR_TYPE_IVEC2,
															 VAR_TYPE_IVEC3, VAR_TYPE_IVEC4, VAR_TYPE_VEC2,
															 VAR_TYPE_VEC3,  VAR_TYPE_VEC4,  VAR_TYPE_MAT2,
															 VAR_TYPE_MAT3,  VAR_TYPE_MAT4,  VAR_TYPE_DOUBLE,
															 VAR_TYPE_DMAT2, VAR_TYPE_DMAT3, VAR_TYPE_DMAT4 };
	static const size_t num_var_types_gl = sizeof(var_types_set_gl) / sizeof(var_types_set_gl[0]);

	const std::string iteration_loop_end = "                }\n"
										   "            }\n"
										   "        }\n"
										   "    }\n";
	const std::string iteration_loop_start = "    for (uint a = 0u; a < 2u; a++)\n"
											 "    {\n"
											 "        for (uint b = 0u; b < 2u; b++)\n"
											 "        {\n"
											 "            for (uint c = 0u; c < 2u; c++)\n"
											 "            {\n"
											 "                for (uint d = 0u; d < 2u; d++)\n"
											 "                {\n";
	const glcts::test_var_type* var_types_set = var_types_set_es;
	size_t						num_var_types = num_var_types_es;
	const bool					test_compute  = (TestCaseBase<API>::COMPUTE_SHADER_TYPE == tested_shader_type);

	if (API::USE_DOUBLE)
	{
		var_types_set = var_types_set_gl;
		num_var_types = num_var_types_gl;
	}

	for (size_t var_type_index = 0; var_type_index < num_var_types; var_type_index++)
	{
		_supported_variable_types_map_const_iterator var_iterator =
			supported_variable_types_map.find(var_types_set[var_type_index]);

		if (var_iterator != supported_variable_types_map.end())
		{
			std::string function_definition;
			std::string function_use;
			std::string verification;

			function_definition += "// Subroutine types\n"
								   "subroutine bool inout_routine_type(inout ";
			function_definition += var_iterator->second.type;
			function_definition += " x[2][2][2][2], inout ";
			function_definition += var_iterator->second.type;
			function_definition += " y[2][2][2][2]);\n\n"
								   "// Subroutine definitions\n"
								   "subroutine(inout_routine_type) bool original_routine(inout ";
			function_definition += var_iterator->second.type;
			function_definition += " x[2][2][2][2], inout ";
			function_definition += var_iterator->second.type;
			function_definition += " y[2][2][2][2])\n{\n";
			function_definition += iteration_loop_start;
			function_definition +=
				"                                   x[a][b][c][d] = " + var_iterator->second.type + "(123);\n";
			function_definition += iteration_loop_end;
			function_definition += "\n";
			function_definition += iteration_loop_start;
			function_definition += "                                   if(y[a][b][c][d]";
			if (var_iterator->second.type == "mat4") // mat4 comparison
			{
				function_definition += "[0][0]";
				function_definition += " != float";
			}
			else if (var_iterator->second.type == "dmat4") // dmat4 comparison
			{
				function_definition += "[0][0]";
				function_definition += " != double";
			}
			else
			{
				function_definition += " != ";
				function_definition += var_iterator->second.type;
			}
			function_definition += "(((a*8u)+(b*4u)+(c*2u)+d))) {return false;}\n";
			function_definition += iteration_loop_end;
			function_definition += "\n    return true;\n";
			function_definition += "}\n\n"
								   "subroutine(inout_routine_type) bool new_routine(inout ";
			function_definition += var_iterator->second.type;
			function_definition += " x[2][2][2][2], inout ";
			function_definition += var_iterator->second.type;
			function_definition += " y[2][2][2][2])\n{\n";
			function_definition += iteration_loop_start;
			function_definition +=
				"                                   y[a][b][c][d] = " + var_iterator->second.type + "(123);\n";
			function_definition += iteration_loop_end;
			function_definition += "\n";
			function_definition += iteration_loop_start;
			function_definition += "                                   if(x[a][b][c][d]";
			if (var_iterator->second.type == "mat4") // mat4 comparison
			{
				function_definition += "[0][0]";
				function_definition += " != float";
			}
			else if (var_iterator->second.type == "dmat4") // dmat4 comparison
			{
				function_definition += "[0][0]";
				function_definition += " != double";
			}
			else
			{
				function_definition += " != ";
				function_definition += var_iterator->second.type;
			}
			function_definition += "(((a*8u)+(b*4u)+(c*2u)+d))) {return false;}\n";
			function_definition += iteration_loop_end;
			function_definition += "\n    return true;\n";
			function_definition += "}\n\n"
								   "// Subroutine uniform\n"
								   "subroutine uniform inout_routine_type routine;\n";

			function_use += "    " + var_iterator->second.type + " z[2][2][2][2];\n";
			function_use += iteration_loop_start;
			function_use += "                                   z[a][b][c][d] = ";
			function_use += var_iterator->second.type;
			function_use += "(((a*8u)+(b*4u)+(c*2u)+d));\n";
			function_use += iteration_loop_end;

			verification = "    float result = 0.0;\n"
						   "    if(routine(z, z) == true)\n"
						   "    {\n"
						   "        result = 1.0;\n"
						   "    }\n"
						   "    else\n"
						   "    {\n"
						   "        result = 0.5;\n"
						   "    }\n";

			if (false == test_compute)
			{
				this->execute_draw_test(tested_shader_type, function_definition, function_use, verification, false,
										false);
				this->execute_draw_test(tested_shader_type, function_definition, function_use, verification, true,
										false);
			}
			else
			{
				this->execute_dispatch_test(tested_shader_type, function_definition, function_use, verification, false,
											false);
				this->execute_dispatch_test(tested_shader_type, function_definition, function_use, verification, true,
											false);
			}

			/* Deallocate any resources used. */
			this->delete_objects();
		} /* if var_type iterator found */
		else
		{
			TCU_FAIL("Type not found.");
		}
	} /* for (int var_type_index = 0; ...) */
}

/* Generates the shader source code for the SubroutineArgumentAliasing1
 * array tests, attempts to build and execute test program
 *
 * @tparam API               Tested API descriptor
 *
 * @param tested_shader_type The type of shader that is being tested
 */
template <class API>
void SubroutineArgumentAliasing3<API>::test_shader_compilation(
	typename TestCaseBase<API>::TestShaderType tested_shader_type)
{
	static const glcts::test_var_type var_types_set_es[] = { VAR_TYPE_INT,   VAR_TYPE_FLOAT, VAR_TYPE_IVEC2,
															 VAR_TYPE_IVEC3, VAR_TYPE_IVEC4, VAR_TYPE_VEC2,
															 VAR_TYPE_VEC3,  VAR_TYPE_VEC4,  VAR_TYPE_MAT2,
															 VAR_TYPE_MAT3,  VAR_TYPE_MAT4 };
	static const size_t num_var_types_es = sizeof(var_types_set_es) / sizeof(var_types_set_es[0]);

	static const glcts::test_var_type var_types_set_gl[] = { VAR_TYPE_INT,   VAR_TYPE_FLOAT, VAR_TYPE_IVEC2,
															 VAR_TYPE_IVEC3, VAR_TYPE_IVEC4, VAR_TYPE_VEC2,
															 VAR_TYPE_VEC3,  VAR_TYPE_VEC4,  VAR_TYPE_MAT2,
															 VAR_TYPE_MAT3,  VAR_TYPE_MAT4,  VAR_TYPE_DOUBLE,
															 VAR_TYPE_DMAT2, VAR_TYPE_DMAT3, VAR_TYPE_DMAT4 };
	static const size_t num_var_types_gl = sizeof(var_types_set_gl) / sizeof(var_types_set_gl[0]);

	const std::string iteration_loop_end = "                }\n"
										   "            }\n"
										   "        }\n"
										   "    }\n";
	const std::string iteration_loop_start = "    for (uint a = 0u; a < 2u; a++)\n"
											 "    {\n"
											 "        for (uint b = 0u; b < 2u; b++)\n"
											 "        {\n"
											 "            for (uint c = 0u; c < 2u; c++)\n"
											 "            {\n"
											 "                for (uint d = 0u; d < 2u; d++)\n"
											 "                {\n";
	const glcts::test_var_type* var_types_set = var_types_set_es;
	size_t						num_var_types = num_var_types_es;
	const bool					test_compute  = (TestCaseBase<API>::COMPUTE_SHADER_TYPE == tested_shader_type);

	if (API::USE_DOUBLE)
	{
		var_types_set = var_types_set_gl;
		num_var_types = num_var_types_gl;
	}

	for (size_t var_type_index = 0; var_type_index < num_var_types; var_type_index++)
	{
		_supported_variable_types_map_const_iterator var_iterator =
			supported_variable_types_map.find(var_types_set[var_type_index]);

		if (var_iterator != supported_variable_types_map.end())
		{
			std::string function_definition;
			std::string function_use;
			std::string verification;

			function_definition += "// Subroutine types\n"
								   "subroutine bool out_routine_type(out ";
			function_definition += var_iterator->second.type;
			function_definition += " x[2][2][2][2], ";
			function_definition += var_iterator->second.type;
			function_definition += " y[2][2][2][2]);\n\n"
								   "// Subroutine definitions\n"
								   "subroutine(out_routine_type) bool original_routine(out ";
			function_definition += var_iterator->second.type;
			function_definition += " x[2][2][2][2], ";
			function_definition += var_iterator->second.type;
			function_definition += " y[2][2][2][2])\n{\n";
			function_definition += iteration_loop_start;
			function_definition +=
				"                                   x[a][b][c][d] = " + var_iterator->second.type + "(123);\n";
			function_definition += iteration_loop_end;
			function_definition += "\n";
			function_definition += iteration_loop_start;
			function_definition += "                                   if(y[a][b][c][d]";
			if (var_iterator->second.type == "mat4") // mat4 comparison
			{
				function_definition += "[0][0]";
				function_definition += " != float";
			}
			else if (var_iterator->second.type == "dmat4") // dmat4 comparison
			{
				function_definition += "[0][0]";
				function_definition += " != double";
			}
			else
			{
				function_definition += " != ";
				function_definition += var_iterator->second.type;
			}
			function_definition += "(((a*8u)+(b*4u)+(c*2u)+d))) {return false;}\n";
			function_definition += iteration_loop_end;
			function_definition += "\n    return true;\n";
			function_definition += "}\n\n"
								   "subroutine(out_routine_type) bool new_routine(out ";
			function_definition += var_iterator->second.type;
			function_definition += " x[2][2][2][2], ";
			function_definition += var_iterator->second.type;
			function_definition += " y[2][2][2][2])\n{\n";
			function_definition += iteration_loop_start;
			function_definition +=
				"                                   x[a][b][c][d] = " + var_iterator->second.type + "(321);\n";
			function_definition += iteration_loop_end;
			function_definition += "\n";
			function_definition += iteration_loop_start;
			function_definition += "                                   if(y[a][b][c][d]";
			if (var_iterator->second.type == "mat4") // mat4 comparison
			{
				function_definition += "[0][0]";
				function_definition += " != float";
			}
			else if (var_iterator->second.type == "dmat4") // dmat4 comparison
			{
				function_definition += "[0][0]";
				function_definition += " != double";
			}
			else
			{
				function_definition += " != ";
				function_definition += var_iterator->second.type;
			}
			function_definition += "(((a*8u)+(b*4u)+(c*2u)+d))) {return false;}\n";
			function_definition += iteration_loop_end;
			function_definition += "\n    return true;\n";
			function_definition += "}\n\n"
								   "// Subroutine uniform\n"
								   "subroutine uniform out_routine_type routine;\n";

			function_use += "    " + var_iterator->second.type + " z[2][2][2][2];\n";
			function_use += iteration_loop_start;
			function_use += "                                   z[a][b][c][d] = ";
			function_use += var_iterator->second.type;
			function_use += "(((a*8u)+(b*4u)+(c*2u)+d));\n";
			function_use += iteration_loop_end;

			verification = "    float result = 0.0;\n"
						   "    if(routine(z, z) == true)\n"
						   "    {\n"
						   "        result = 1.0;\n"
						   "    }\n"
						   "    else\n"
						   "    {\n"
						   "        result = 0.5;\n"
						   "    }\n";

			if (false == test_compute)
			{
				this->execute_draw_test(tested_shader_type, function_definition, function_use, verification, false,
										false);
				this->execute_draw_test(tested_shader_type, function_definition, function_use, verification, true,
										false);
			}
			else
			{
				this->execute_dispatch_test(tested_shader_type, function_definition, function_use, verification, false,
											false);
				this->execute_dispatch_test(tested_shader_type, function_definition, function_use, verification, true,
											false);
			}

			/* Deallocate any resources used. */
			this->delete_objects();
		} /* if var_type iterator found */
		else
		{
			TCU_FAIL("Type not found.");
		}
	} /* for (int var_type_index = 0; ...) */
}

/* Generates the shader source code for the SubroutineArgumentAliasing1
 * array tests, attempts to build and execute test program
 *
 * @tparam API               Tested API descriptor
 *
 * @param tested_shader_type The type of shader that is being tested
 */
template <class API>
void SubroutineArgumentAliasing4<API>::test_shader_compilation(
	typename TestCaseBase<API>::TestShaderType tested_shader_type)
{
	static const glcts::test_var_type var_types_set_es[] = { VAR_TYPE_INT,   VAR_TYPE_FLOAT, VAR_TYPE_IVEC2,
															 VAR_TYPE_IVEC3, VAR_TYPE_IVEC4, VAR_TYPE_VEC2,
															 VAR_TYPE_VEC3,  VAR_TYPE_VEC4,  VAR_TYPE_MAT2,
															 VAR_TYPE_MAT3,  VAR_TYPE_MAT4 };
	static const size_t num_var_types_es = sizeof(var_types_set_es) / sizeof(var_types_set_es[0]);

	static const glcts::test_var_type var_types_set_gl[] = { VAR_TYPE_INT,   VAR_TYPE_FLOAT, VAR_TYPE_IVEC2,
															 VAR_TYPE_IVEC3, VAR_TYPE_IVEC4, VAR_TYPE_VEC2,
															 VAR_TYPE_VEC3,  VAR_TYPE_VEC4,  VAR_TYPE_MAT2,
															 VAR_TYPE_MAT3,  VAR_TYPE_MAT4,  VAR_TYPE_DOUBLE,
															 VAR_TYPE_DMAT2, VAR_TYPE_DMAT3, VAR_TYPE_DMAT4 };
	static const size_t num_var_types_gl = sizeof(var_types_set_gl) / sizeof(var_types_set_gl[0]);

	const std::string iteration_loop_end = "                }\n"
										   "            }\n"
										   "        }\n"
										   "    }\n";
	const std::string iteration_loop_start = "    for (uint a = 0u; a < 2u; a++)\n"
											 "    {\n"
											 "        for (uint b = 0u; b < 2u; b++)\n"
											 "        {\n"
											 "            for (uint c = 0u; c < 2u; c++)\n"
											 "            {\n"
											 "                for (uint d = 0u; d < 2u; d++)\n"
											 "                {\n";
	const glcts::test_var_type* var_types_set = var_types_set_es;
	size_t						num_var_types = num_var_types_es;
	const bool					test_compute  = (TestCaseBase<API>::COMPUTE_SHADER_TYPE == tested_shader_type);

	if (API::USE_DOUBLE)
	{
		var_types_set = var_types_set_gl;
		num_var_types = num_var_types_gl;
	}

	for (size_t var_type_index = 0; var_type_index < num_var_types; var_type_index++)
	{
		_supported_variable_types_map_const_iterator var_iterator =
			supported_variable_types_map.find(var_types_set[var_type_index]);

		if (var_iterator != supported_variable_types_map.end())
		{
			std::string function_definition;
			std::string function_use;
			std::string verification;

			function_definition += "// Subroutine types\n"
								   "subroutine bool out_routine_type(";
			function_definition += var_iterator->second.type;
			function_definition += " x[2][2][2][2], out ";
			function_definition += var_iterator->second.type;
			function_definition += " y[2][2][2][2]);\n\n"
								   "// Subroutine definitions\n"
								   "subroutine(out_routine_type) bool original_routine(";
			function_definition += var_iterator->second.type;
			function_definition += " x[2][2][2][2], out ";
			function_definition += var_iterator->second.type;
			function_definition += " y[2][2][2][2])\n{\n";
			function_definition += iteration_loop_start;
			function_definition +=
				"                                   y[a][b][c][d] = " + var_iterator->second.type + "(123);\n";
			function_definition += iteration_loop_end;
			function_definition += "\n";
			function_definition += iteration_loop_start;
			function_definition += "                                   if(x[a][b][c][d]";
			if (var_iterator->second.type == "mat4") // mat4 comparison
			{
				function_definition += "[0][0]";
				function_definition += " != float";
			}
			else if (var_iterator->second.type == "dmat4") // dmat4 comparison
			{
				function_definition += "[0][0]";
				function_definition += " != double";
			}
			else
			{
				function_definition += " != ";
				function_definition += var_iterator->second.type;
			}
			function_definition += "(((a*8u)+(b*4u)+(c*2u)+d))) {return false;}\n";
			function_definition += iteration_loop_end;
			function_definition += "\n    return true;\n";
			function_definition += "}\n\n"
								   "subroutine(out_routine_type) bool new_routine(";
			function_definition += var_iterator->second.type;
			function_definition += " x[2][2][2][2], out ";
			function_definition += var_iterator->second.type;
			function_definition += " y[2][2][2][2])\n{\n";
			function_definition += iteration_loop_start;
			function_definition +=
				"                                   y[a][b][c][d] = " + var_iterator->second.type + "(321);\n";
			function_definition += iteration_loop_end;
			function_definition += "\n";
			function_definition += iteration_loop_start;
			function_definition += "                                   if(x[a][b][c][d]";
			if (var_iterator->second.type == "mat4") // mat4 comparison
			{
				function_definition += "[0][0]";
				function_definition += " != float";
			}
			else if (var_iterator->second.type == "dmat4") // dmat4 comparison
			{
				function_definition += "[0][0]";
				function_definition += " != double";
			}
			else
			{
				function_definition += " != ";
				function_definition += var_iterator->second.type;
			}
			function_definition += "(((a*8u)+(b*4u)+(c*2u)+d))) {return false;}\n";
			function_definition += iteration_loop_end;
			function_definition += "\n    return true;\n";
			function_definition += "}\n\n"
								   "// Subroutine uniform\n"
								   "subroutine uniform out_routine_type routine;\n";

			function_use += "    " + var_iterator->second.type + " z[2][2][2][2];\n";
			function_use += iteration_loop_start;
			function_use += "                                   z[a][b][c][d] = ";
			function_use += var_iterator->second.type;
			function_use += "(((a*8u)+(b*4u)+(c*2u)+d));\n";
			function_use += iteration_loop_end;

			verification = "    float result = 0.0;\n"
						   "    if(routine(z, z) == true)\n"
						   "    {\n"
						   "        result = 1.0;\n"
						   "    }\n"
						   "    else\n"
						   "    {\n"
						   "        result = 0.5;\n"
						   "    }\n";

			if (false == test_compute)
			{
				this->execute_draw_test(tested_shader_type, function_definition, function_use, verification, false,
										false);
				this->execute_draw_test(tested_shader_type, function_definition, function_use, verification, true,
										false);
			}
			else
			{
				this->execute_dispatch_test(tested_shader_type, function_definition, function_use, verification, false,
											false);
				this->execute_dispatch_test(tested_shader_type, function_definition, function_use, verification, true,
											false);
			}

			/* Deallocate any resources used. */
			this->delete_objects();
		} /* if var_type iterator found */
		else
		{
			TCU_FAIL("Type not found.");
		}
	} /* for (int var_type_index = 0; ...) */
}

/** Instantiates all tests and adds them as children to the node
 *
 * @tparam API    Tested API descriptor
 *
 * @param context CTS context
 **/
template <class API>
void initTests(TestCaseGroup& group, glcts::Context& context)
{
	// Set up the map
	ArraysOfArrays::initializeMap<API>();

	group.addChild(new glcts::ArraysOfArrays::SizedDeclarationsPrimitive<API>(context));
	group.addChild(new glcts::ArraysOfArrays::SizedDeclarationsStructTypes1<API>(context));
	group.addChild(new glcts::ArraysOfArrays::SizedDeclarationsStructTypes2<API>(context));
	group.addChild(new glcts::ArraysOfArrays::SizedDeclarationsStructTypes3<API>(context));
	group.addChild(new glcts::ArraysOfArrays::SizedDeclarationsStructTypes4<API>(context));
	group.addChild(new glcts::ArraysOfArrays::SizedDeclarationsTypenameStyle1<API>(context));
	group.addChild(new glcts::ArraysOfArrays::SizedDeclarationsTypenameStyle2<API>(context));
	group.addChild(new glcts::ArraysOfArrays::SizedDeclarationsTypenameStyle3<API>(context));
	group.addChild(new glcts::ArraysOfArrays::SizedDeclarationsTypenameStyle4<API>(context));
	group.addChild(new glcts::ArraysOfArrays::SizedDeclarationsTypenameStyle5<API>(context));
	group.addChild(new glcts::ArraysOfArrays::SizedDeclarationsFunctionParams<API>(context));
	group.addChild(new glcts::ArraysOfArrays::sized_declarations_invalid_sizes1<API>(context));
	group.addChild(new glcts::ArraysOfArrays::sized_declarations_invalid_sizes2<API>(context));
	group.addChild(new glcts::ArraysOfArrays::sized_declarations_invalid_sizes3<API>(context));
	group.addChild(new glcts::ArraysOfArrays::sized_declarations_invalid_sizes4<API>(context));
	group.addChild(new glcts::ArraysOfArrays::ConstructorsAndUnsizedDeclConstructors1<API>(context));
	group.addChild(new glcts::ArraysOfArrays::ConstructorsAndUnsizedDeclConstructors2<API>(context));
	group.addChild(new glcts::ArraysOfArrays::ConstructorsAndUnsizedDeclUnsizedConstructors<API>(context));
	group.addChild(new glcts::ArraysOfArrays::ConstructorsAndUnsizedDeclConst<API>(context));

	group.addChild(new glcts::ArraysOfArrays::ConstructorsAndUnsizedDeclInvalidConstructors1<API>(context));
	group.addChild(new glcts::ArraysOfArrays::ConstructorsAndUnsizedDeclInvalidConstructors2<API>(context));
	group.addChild(new glcts::ArraysOfArrays::ConstructorsAndUnsizedDeclInvalidConstructors3<API>(context));
	group.addChild(new glcts::ArraysOfArrays::ConstructorsAndUnsizedDeclInvalidConstructors4<API>(context));
	group.addChild(new glcts::ArraysOfArrays::ConstructorsAndUnsizedDeclConstructorSizing1<API>(context));
	group.addChild(new glcts::ArraysOfArrays::ConstructorsAndUnsizedDeclConstructorSizing2<API>(context));
	group.addChild(new glcts::ArraysOfArrays::ConstructorsAndUnsizedDeclStructConstructors<API>(context));
	group.addChild(new glcts::ArraysOfArrays::ConstructorsAndUnsizedDeclUnsizedArrays1<API>(context));
	group.addChild(new glcts::ArraysOfArrays::ConstructorsAndUnsizedDeclUnsizedArrays2<API>(context));
	group.addChild(new glcts::ArraysOfArrays::ConstructorsAndUnsizedDeclUnsizedArrays3<API>(context));
	group.addChild(new glcts::ArraysOfArrays::ConstructorsAndUnsizedDeclUnsizedArrays4<API>(context));
	group.addChild(new glcts::ArraysOfArrays::ExpressionsAssignment1<API>(context));
	group.addChild(new glcts::ArraysOfArrays::ExpressionsAssignment2<API>(context));
	group.addChild(new glcts::ArraysOfArrays::ExpressionsAssignment3<API>(context));
	group.addChild(new glcts::ArraysOfArrays::ExpressionsTypeRestrictions1<API>(context));
	group.addChild(new glcts::ArraysOfArrays::ExpressionsTypeRestrictions2<API>(context));
	group.addChild(new glcts::ArraysOfArrays::ExpressionsIndexingScalar1<API>(context));
	group.addChild(new glcts::ArraysOfArrays::ExpressionsIndexingScalar2<API>(context));
	group.addChild(new glcts::ArraysOfArrays::ExpressionsIndexingScalar3<API>(context));
	group.addChild(new glcts::ArraysOfArrays::ExpressionsIndexingScalar4<API>(context));
	group.addChild(new glcts::ArraysOfArrays::ExpressionsIndexingArray1<API>(context));
	group.addChild(new glcts::ArraysOfArrays::ExpressionsIndexingArray2<API>(context));
	group.addChild(new glcts::ArraysOfArrays::ExpressionsIndexingArray3<API>(context));
	group.addChild(new glcts::ArraysOfArrays::ExpressionsDynamicIndexing1<API>(context));
	group.addChild(new glcts::ArraysOfArrays::ExpressionsDynamicIndexing2<API>(context));
	group.addChild(new glcts::ArraysOfArrays::ExpressionsEquality1<API>(context));
	group.addChild(new glcts::ArraysOfArrays::ExpressionsEquality2<API>(context));
	group.addChild(new glcts::ArraysOfArrays::ExpressionsLength1<API>(context));
	group.addChild(new glcts::ArraysOfArrays::ExpressionsLength2<API>(context));
	group.addChild(new glcts::ArraysOfArrays::ExpressionsLength3<API>(context));
	group.addChild(new glcts::ArraysOfArrays::ExpressionsInvalid1<API>(context));
	group.addChild(new glcts::ArraysOfArrays::ExpressionsInvalid2<API>(context));

	group.addChild(new glcts::ArraysOfArrays::InteractionFunctionCalls1<API>(context));
	group.addChild(new glcts::ArraysOfArrays::InteractionFunctionCalls2<API>(context));
	group.addChild(new glcts::ArraysOfArrays::InteractionArgumentAliasing1<API>(context));
	group.addChild(new glcts::ArraysOfArrays::InteractionArgumentAliasing2<API>(context));
	group.addChild(new glcts::ArraysOfArrays::InteractionArgumentAliasing3<API>(context));
	group.addChild(new glcts::ArraysOfArrays::InteractionArgumentAliasing4<API>(context));
	group.addChild(new glcts::ArraysOfArrays::InteractionArgumentAliasing5<API>(context));
	group.addChild(new glcts::ArraysOfArrays::InteractionArgumentAliasing6<API>(context));

	group.addChild(new glcts::ArraysOfArrays::InteractionUniforms1<API>(context));
	group.addChild(new glcts::ArraysOfArrays::InteractionUniforms2<API>(context));
	group.addChild(new glcts::ArraysOfArrays::InteractionUniformBuffers1<API>(context));
	group.addChild(new glcts::ArraysOfArrays::InteractionUniformBuffers2<API>(context));
	group.addChild(new glcts::ArraysOfArrays::InteractionUniformBuffers3<API>(context));
	group.addChild(new glcts::ArraysOfArrays::InteractionInterfaceArrays1<API>(context));
	group.addChild(new glcts::ArraysOfArrays::InteractionInterfaceArrays2<API>(context));
	group.addChild(new glcts::ArraysOfArrays::InteractionInterfaceArrays3<API>(context));
	group.addChild(new glcts::ArraysOfArrays::InteractionInterfaceArrays4<API>(context));

	if (API::USE_STORAGE_BLOCK)
	{
		group.addChild(new glcts::ArraysOfArrays::InteractionStorageBuffers1<API>(context));
		group.addChild(new glcts::ArraysOfArrays::InteractionStorageBuffers2<API>(context));
		group.addChild(new glcts::ArraysOfArrays::InteractionStorageBuffers3<API>(context));
	}

	if (API::USE_ATOMIC)
	{
		group.addChild(new glcts::ArraysOfArrays::AtomicDeclarationTest<API>(context));
		group.addChild(new glcts::ArraysOfArrays::AtomicUsageTest<API>(context));
	}

	if (API::USE_SUBROUTINE)
	{
		group.addChild(new glcts::ArraysOfArrays::SubroutineFunctionCalls1<API>(context));
		group.addChild(new glcts::ArraysOfArrays::SubroutineFunctionCalls2<API>(context));
		group.addChild(new glcts::ArraysOfArrays::SubroutineArgumentAliasing1<API>(context));
		group.addChild(new glcts::ArraysOfArrays::SubroutineArgumentAliasing2<API>(context));
		group.addChild(new glcts::ArraysOfArrays::SubroutineArgumentAliasing3<API>(context));
		group.addChild(new glcts::ArraysOfArrays::SubroutineArgumentAliasing4<API>(context));
	}
}
} /* namespace ArraysOfArrays */

/** Constructor
 *
 * @param context CTS context
 **/
ArrayOfArraysTestGroup::ArrayOfArraysTestGroup(Context& context)
	: TestCaseGroup(context, "arrays_of_arrays", "Groups all tests that verify 'arrays of arrays' functionality.")
{
	/* Left blank on purpose */
}

/* Instantiates all tests and adds them as children to the node */
void ArrayOfArraysTestGroup::init(void)
{
	ArraysOfArrays::initTests<ArraysOfArrays::Interface::ES>(*this, m_context);
}

/** Constructor
 *
 * @param context CTS context
 **/
ArrayOfArraysTestGroupGL::ArrayOfArraysTestGroupGL(Context& context)
	: TestCaseGroup(context, "arrays_of_arrays_gl", "Groups all tests that verify 'arrays of arrays' functionality.")
{
	/* Left blank on purpose */
}

/* Instantiates all tests and adds them as children to the node */
void ArrayOfArraysTestGroupGL::init(void)
{
	ArraysOfArrays::initTests<ArraysOfArrays::Interface::GL>(*this, m_context);
}
} /* namespace glcts */
