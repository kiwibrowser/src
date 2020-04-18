#ifndef _GL4CGPUSHADERFP64TESTS_HPP
#define _GL4CGPUSHADERFP64TESTS_HPP
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

/**
 * \file  gl4cGPUShaderFP64Tests.hpp
 * \brief Declares test classes for "GPU Shader FP64" functionality.
 */ /*-------------------------------------------------------------------*/

#include "glcTestCase.hpp"
#include "glwDefs.hpp"
#include "glwEnums.hpp"
#include "tcuDefs.hpp"
#include "tcuVector.hpp"
#include <queue>

namespace gl4cts
{
class Utils
{
public:
	/* Public type definitions */

	/** Store information about program object
	 *
	 **/
	struct programInfo
	{
		programInfo(deqp::Context& context);
		~programInfo();

		void build(const glw::GLchar* compute_shader_code, const glw::GLchar* fragment_shader_code,
				   const glw::GLchar* geometry_shader_code, const glw::GLchar* tesselation_control_shader_code,
				   const glw::GLchar* tesselation_evaluation_shader_code, const glw::GLchar* vertex_shader_code,
				   const glw::GLchar* const* varying_names, glw::GLuint n_varying_names);

		void compile(glw::GLuint shader_id, const glw::GLchar* shader_code) const;

		void link() const;

		static const glw::GLenum ARB_COMPUTE_SHADER;

		deqp::Context& m_context;

		glw::GLuint m_compute_shader_id;
		glw::GLuint m_fragment_shader_id;
		glw::GLuint m_geometry_shader_id;
		glw::GLuint m_program_object_id;
		glw::GLuint m_tesselation_control_shader_id;
		glw::GLuint m_tesselation_evaluation_shader_id;
		glw::GLuint m_vertex_shader_id;
	};

	/* Defines GLSL variable type */
	enum _variable_type
	{
		VARIABLE_TYPE_BOOL,
		VARIABLE_TYPE_BVEC2,
		VARIABLE_TYPE_BVEC3,
		VARIABLE_TYPE_BVEC4,
		VARIABLE_TYPE_DOUBLE,
		VARIABLE_TYPE_DMAT2,
		VARIABLE_TYPE_DMAT2X3,
		VARIABLE_TYPE_DMAT2X4,
		VARIABLE_TYPE_DMAT3,
		VARIABLE_TYPE_DMAT3X2,
		VARIABLE_TYPE_DMAT3X4,
		VARIABLE_TYPE_DMAT4,
		VARIABLE_TYPE_DMAT4X2,
		VARIABLE_TYPE_DMAT4X3,
		VARIABLE_TYPE_DVEC2,
		VARIABLE_TYPE_DVEC3,
		VARIABLE_TYPE_DVEC4,
		VARIABLE_TYPE_FLOAT,
		VARIABLE_TYPE_INT,
		VARIABLE_TYPE_IVEC2,
		VARIABLE_TYPE_IVEC3,
		VARIABLE_TYPE_IVEC4,
		VARIABLE_TYPE_MAT2,
		VARIABLE_TYPE_MAT2X3,
		VARIABLE_TYPE_MAT2X4,
		VARIABLE_TYPE_MAT3,
		VARIABLE_TYPE_MAT3X2,
		VARIABLE_TYPE_MAT3X4,
		VARIABLE_TYPE_MAT4,
		VARIABLE_TYPE_MAT4X2,
		VARIABLE_TYPE_MAT4X3,
		VARIABLE_TYPE_UINT,
		VARIABLE_TYPE_UVEC2,
		VARIABLE_TYPE_UVEC3,
		VARIABLE_TYPE_UVEC4,
		VARIABLE_TYPE_VEC2,
		VARIABLE_TYPE_VEC3,
		VARIABLE_TYPE_VEC4,

		/* Always last */
		VARIABLE_TYPE_UNKNOWN
	};

	/* Public static methods */
	static _variable_type getBaseVariableType(_variable_type type);
	static unsigned int getBaseVariableTypeComponentSize(_variable_type type);
	static unsigned char getComponentAtIndex(unsigned int index);

	static _variable_type getDoubleVariableType(glw::GLuint n_columns, glw::GLuint n_rows);

	static std::string getFPVariableTypeStringForVariableType(_variable_type type);
	static glw::GLenum getGLDataTypeOfBaseVariableType(_variable_type type);
	static glw::GLenum getGLDataTypeOfVariableType(_variable_type type);

	static _variable_type getIntVariableType(glw::GLuint n_columns, glw::GLuint n_rows);

	static unsigned int getNumberOfColumnsForVariableType(_variable_type type);
	static unsigned int getNumberOfComponentsForVariableType(_variable_type type);
	static unsigned int getNumberOfLocationsUsedByDoublePrecisionVariableType(_variable_type type);
	static unsigned int getNumberOfRowsForVariableType(_variable_type type);

	static _variable_type getPostMatrixMultiplicationVariableType(_variable_type type_matrix_a,
																  _variable_type type_matrix_b);

	static std::string getStringForVariableTypeValue(_variable_type type, const unsigned char* data_ptr);

	static _variable_type getTransposedMatrixVariableType(_variable_type type);

	static _variable_type getUintVariableType(glw::GLuint n_columns, glw::GLuint n_rows);

	static std::string getVariableTypeString(_variable_type type);

	static bool isGLVersionAtLeast(const glw::Functions& gl, glw::GLint required_major, glw::GLint required_minor);

	static bool isMatrixVariableType(_variable_type type);
	static bool isScalarVariableType(_variable_type type);

	static void replaceToken(const glw::GLchar* token, size_t& search_position, const glw::GLchar* text,
							 std::string& string);
};

/** Make sure errors as per spec are generated for new entry-points.
 *
 * a) Make sure GL_INVALID_OPERATION is generated by glUniform*() and
 *    glUniformMatrix*() functions if there is no current program object.
 * b) Make sure GL_INVALID_OPERATION is generated by glUniform*() if
 *    the size of the uniform variable declared in the shader does not
 *    match the size indicated by the command.
 * c) Make sure GL_INVALID_OPERATION is generated if glUniform*() and
 *    glUniformMatrix*() are used to load a uniform variable of type
 *    bool, bvec2, bvec3, bvec4, float, int, ivec2, ivec3, ivec4,
 *    unsigned int, uvec2, uvec3, uvec4, vec2, vec3, vec4 or an array
 *    of these.
 * d) Make sure GL_INVALID_OPERATION is generated if glUniform*() and
 *    glUniformMatrix*() are used to load incompatible double-typed
 *    uniforms, as presented below:
 *
 *    I.    double-typed uniform configured by glUniform2d();
 *    II.   double-typed uniform configured by glUniform3d();
 *    III.  double-typed uniform configured by glUniform4d();
 *    IV.   double-typed uniform configured by glUniformMatrix*();
 *    V.    dvec2-typed  uniform configured by glUniform1d();
 *    VI.   dvec2-typed  uniform configured by glUniform3d();
 *    VII.  dvec2-typed  uniform configured by glUniform4d();
 *    VIII. dvec2-typed  uniform configured by glUniformMatrix*();
 *
 *                             (etc.)
 *
 * e) Make sure GL_INVALID_OPERATION is generated if <location> of
 *    glUniform*() and glUniformMatrix*() is an invalid uniform
 *    location for the current program object and location is not
 *    equal to -1.
 * f) Make sure GL_INVALID_VALUE is generated if <count> of
 *    glUniform*() (*dv() functions only) and glUniformMatrix*() is
 *    negative.
 * g) Make sure GL_INVALID_OPERATION is generated if <count> of
 *    glUniform*() (*dv() functions only) and glUniformMatrix*() is
 *    greater than 1 and the indicated uniform variable is not an
 *    array variable.
 * h) Make sure GL_INVALID_OPERATION is generated if a sampler is
 *    loaded by glUniform*() and glUniformMatrix*().
 * i) Make sure GL_INVALID_OPERATION is generated if glUniform*() and
 *    glUniformMatrix*() is used to load values for uniforms of
 *    boolean types.
 */
class GPUShaderFP64Test1 : public deqp::TestCase
{
public:
	/* Public methods */
	GPUShaderFP64Test1(deqp::Context& context);

	void								 deinit();
	virtual tcu::TestNode::IterateResult iterate();

	/* Private type definitions */
private:
	typedef enum {
		UNIFORM_FUNCTION_FIRST,

		UNIFORM_FUNCTION_1D = UNIFORM_FUNCTION_FIRST,
		UNIFORM_FUNCTION_1DV,
		UNIFORM_FUNCTION_2D,
		UNIFORM_FUNCTION_2DV,
		UNIFORM_FUNCTION_3D,
		UNIFORM_FUNCTION_3DV,
		UNIFORM_FUNCTION_4D,
		UNIFORM_FUNCTION_4DV,

		UNIFORM_FUNCTION_MATRIX2DV,
		UNIFORM_FUNCTION_MATRIX2X3DV,
		UNIFORM_FUNCTION_MATRIX2X4DV,
		UNIFORM_FUNCTION_MATRIX3DV,
		UNIFORM_FUNCTION_MATRIX3X2DV,
		UNIFORM_FUNCTION_MATRIX3X4DV,
		UNIFORM_FUNCTION_MATRIX4DV,
		UNIFORM_FUNCTION_MATRIX4X2DV,
		UNIFORM_FUNCTION_MATRIX4X3DV,

		/* Always last */
		UNIFORM_FUNCTION_COUNT
	} _uniform_function;

	/* Private methods */
	const char* getUniformFunctionString(_uniform_function func);
	const char* getUniformNameForLocation(glw::GLint location);
	void initTest();
	bool isMatrixUniform(glw::GLint uniform_location);
	bool isMatrixUniformFunction(_uniform_function func);
	bool verifyErrorGenerationWhenCallingDoubleUniformFunctionsForBooleans();
	bool verifyErrorGenerationWhenCallingDoubleUniformFunctionsForSamplers();
	bool verifyErrorGenerationWhenCallingDoubleUniformFunctionsWithInvalidCount();
	bool verifyErrorGenerationWhenCallingDoubleUniformFunctionsWithInvalidLocation();
	bool verifyErrorGenerationWhenCallingDoubleUniformFunctionsWithNegativeCount();
	bool verifyErrorGenerationWhenCallingMismatchedDoubleUniformFunctions();
	bool verifyErrorGenerationWhenCallingSizeMismatchedUniformFunctions();
	bool verifyErrorGenerationWhenCallingTypeMismatchedUniformFunctions();
	bool verifyErrorGenerationWhenUniformFunctionsCalledWithoutActivePO();

	/* Private fields */
	bool		m_has_test_passed;
	glw::GLint  m_po_bool_arr_uniform_location;
	glw::GLint  m_po_bool_uniform_location;
	glw::GLint  m_po_bvec2_arr_uniform_location;
	glw::GLint  m_po_bvec2_uniform_location;
	glw::GLint  m_po_bvec3_arr_uniform_location;
	glw::GLint  m_po_bvec3_uniform_location;
	glw::GLint  m_po_bvec4_arr_uniform_location;
	glw::GLint  m_po_bvec4_uniform_location;
	glw::GLint  m_po_dmat2_arr_uniform_location;
	glw::GLint  m_po_dmat2_uniform_location;
	glw::GLint  m_po_dmat2x3_arr_uniform_location;
	glw::GLint  m_po_dmat2x3_uniform_location;
	glw::GLint  m_po_dmat2x4_arr_uniform_location;
	glw::GLint  m_po_dmat2x4_uniform_location;
	glw::GLint  m_po_dmat3_arr_uniform_location;
	glw::GLint  m_po_dmat3_uniform_location;
	glw::GLint  m_po_dmat3x2_arr_uniform_location;
	glw::GLint  m_po_dmat3x2_uniform_location;
	glw::GLint  m_po_dmat3x4_arr_uniform_location;
	glw::GLint  m_po_dmat3x4_uniform_location;
	glw::GLint  m_po_dmat4_arr_uniform_location;
	glw::GLint  m_po_dmat4_uniform_location;
	glw::GLint  m_po_dmat4x2_arr_uniform_location;
	glw::GLint  m_po_dmat4x2_uniform_location;
	glw::GLint  m_po_dmat4x3_arr_uniform_location;
	glw::GLint  m_po_dmat4x3_uniform_location;
	glw::GLint  m_po_double_arr_uniform_location;
	glw::GLint  m_po_double_uniform_location;
	glw::GLint  m_po_dvec2_arr_uniform_location;
	glw::GLint  m_po_dvec2_uniform_location;
	glw::GLint  m_po_dvec3_arr_uniform_location;
	glw::GLint  m_po_dvec3_uniform_location;
	glw::GLint  m_po_dvec4_arr_uniform_location;
	glw::GLint  m_po_dvec4_uniform_location;
	glw::GLint  m_po_float_arr_uniform_location;
	glw::GLint  m_po_float_uniform_location;
	glw::GLint  m_po_int_arr_uniform_location;
	glw::GLint  m_po_int_uniform_location;
	glw::GLint  m_po_ivec2_arr_uniform_location;
	glw::GLint  m_po_ivec2_uniform_location;
	glw::GLint  m_po_ivec3_arr_uniform_location;
	glw::GLint  m_po_ivec3_uniform_location;
	glw::GLint  m_po_ivec4_arr_uniform_location;
	glw::GLint  m_po_ivec4_uniform_location;
	glw::GLint  m_po_sampler_uniform_location;
	glw::GLint  m_po_uint_arr_uniform_location;
	glw::GLint  m_po_uint_uniform_location;
	glw::GLint  m_po_uvec2_arr_uniform_location;
	glw::GLint  m_po_uvec2_uniform_location;
	glw::GLint  m_po_uvec3_arr_uniform_location;
	glw::GLint  m_po_uvec3_uniform_location;
	glw::GLint  m_po_uvec4_arr_uniform_location;
	glw::GLint  m_po_uvec4_uniform_location;
	glw::GLint  m_po_vec2_arr_uniform_location;
	glw::GLint  m_po_vec2_uniform_location;
	glw::GLint  m_po_vec3_arr_uniform_location;
	glw::GLint  m_po_vec3_uniform_location;
	glw::GLint  m_po_vec4_arr_uniform_location;
	glw::GLint  m_po_vec4_uniform_location;
	glw::GLuint m_po_id;
	glw::GLuint m_vs_id;
};

/** Implements "Subtest 2" from CTS_ARB_gpu_shader_fp64, description follows:
 *  Make sure that it is possible to use:
 *
 *  a) up to (GL_MAX_VERTEX_UNIFORM_COMPONENTS / 2) double uniforms
 *    in a default uniform block of a vertex shader;
 *  b) up to (GL_MAX_VERTEX_UNIFORM_COMPONENTS / 4) dvec2 uniforms
 *    in a default uniform block of a vertex shader;
 *  c) up to (GL_MAX_VERTEX_UNIFORM_COMPONENTS / 6) dvec3 uniforms
 *    in a default uniform block of a vertex shader;
 *  d) up to (GL_MAX_VERTEX_UNIFORM_COMPONENTS / 8) dvec4 uniforms
 *    in a default uniform block of a vertex shader;
 *  e) up to (GL_MAX_VERTEX_UNIFORM_COMPONENTS / 16) dmat2, dmat2x3,
 *    dmat2x4, dmat3x2, dmat4x2 uniforms in a default uniform block
 *    of a vertex shader; (each type tested in a separate shader)
 *  f) up to (GL_MAX_VERTEX_UNIFORM_COMPONENTS / 24) dmat3, dmat3x4,
 *    dmat4x3 uniforms in a default uniform block of a vertex shader;
 *    (each type tested in a separate shader)
 *  g) up to (GL_MAX_VERTEX_UNIFORM_COMPONENTS / 32) dmat4 uniforms
 *    in a default uniform block of a vertex shader;
 *  h) arrayed cases of a)-g), where the array size is 3 and the
 *    amount of uniforms that can be supported should be divided
 *    by three as well.
 *  i) cases a)-h), where "vertex shader" is replaced by "fragment
 *    shader" and GL_MAX_VERTEX_UNIFORM_COMPONENTS is changed to
 *    GL_MAX_FRAGMENT_UNIFORM_COMPONENTS;
 *  j) cases a)-h) where "vertex shader" is replaced by "geometry
 *    shader" and GL_MAX_VERTEX_UNIFORM_COMPONENTS is changed to
 *    GL_MAX_GEOMETRY_UNIFORM_COMPONENTS;
 *  k) cases a)-h) where "vertex shader" is replaced by "compute
 *    shader" and GL_MAX_VERTEX_UNIFORM_COMPONENTS is changed to
 *    GL_MAX_COMPUTE_UNIFORM_COMPONENTS;
 *  l) cases a)-h) where "vertex shader" is replaced by "tessellation
 *    control shader" and GL_MAX_VERTEX_UNIFORM_COMPONENTS is changed
 *    to GL_MAX_TESS_CONTROL_UNIFORM_COMPONENTS;
 *  m) cases a)-h) where "vertex shader" is replaced by "tessellation
 *    evaluation shader" and GL_MAX_VERTEX_UNIFORM_COMPONENTS is
 *    changed to GL_MAX_TESS_EVALUATION_UNIFORM_COMPONENTS;
 *
 *  For each case considered, the test should only define uniforms
 *  for the stage the test is dealing with.
 *
 *  All uniform components considered as a whole should be assigned
 *  unique values, where the very first uniform component is set to
 *  a value of 1, the one following set to 2, the one afterward set
 *  to 3, and so on.
 *
 *  For "vertex shader" cases, the test should work by creating
 *  a program object, to which a vertex shader (as per test case)
 *  would be attached. The shader would then read all the uniforms,
 *  verify their values are correct and then set an output bool
 *  variable to true if the validation passed, false otherwise.
 *  The test should draw 1024 points, XFB the contents of the output
 *  variable and verify the result.
 *
 *  For "geometry shader" cases, the test should create a program object
 *  and attach vertex and geometry shaders to it. The vertex shader
 *  should set gl_Position to (1, 0, 0, 1). The geometry shader
 *  should accept points on input and emit 1 point, read all the
 *  uniforms, verify their values are correct and then set an output
 *  bool variable to true if the validation passed, false otherwise.
 *  The test should draw 1024 points, XFB the contents of the output
 *  variable and verify the result.
 *
 *  For "tessellation control shader" cases, the test should create
 *  a program object and attach vertex and tessellation control shaders
 *  to the program object. The vertex shader should set gl_Position
 *  to (1, 0, 0, 1). The tessellation control shader should output
 *  a single vertex per patch and set all inner/outer tessellation.
 *  levels to 1. The shader should read all the uniforms, verify
 *  their values are correct and then set an output per-vertex bool
 *  variable to true if the validation passed, false otherwise. The
 *  test should draw 1024 patches, XFB the contents of the output
 *  variable and verify the result.
 *
 *  For "tessellation evaluation shader" cases, the test should create
 *  a program object and attach vertex, tessellation control and
 *  tessellation evaluation shaders to the program object. The vertex
 *  shader should set gl_Position to (1, 0, 0, 1). The tessellation
 *  control shader should behave as in "tessellation control shader"
 *  case. The tessellation evaluation shader should accept isolines
 *  and work in point mode. The shader should read all the uniforms,
 *  verify their values are correct and then set an output
 *  bool variable to true if the validation passed, false otherwise.
 *  The test should draw 1024 patches, XFB the contents of the output
 *  variable and verify the result.
 *
 *  For "fragment shader" cases, the test should create a program object
 *  and attach vertex and fragment shaders to the program object. The
 *  vertex shader should set gl_Position to 4 different values
 *  (depending on gl_VertexID value), defining screen-space corners
 *  for a GL_TRIANGLE_FAN-based draw call. The fragment shader should
 *  read all the uniforms, verify their values are correct and then
 *  set the output color to vec4(1) if the validation passed, vec4(0)
 *  otherwise. The test should draw 4 vertices making up a triangle fan,
 *  read the rasterized image and confirm the result.
 *
 *  For all cases apart from "fragment shader", the test should also
 *  verify that glGetTransformFeedbackVarying() reports correct
 *  properties for the uniforms.
 *
 *  The test should also verify that in all cases glGetActiveUniform()
 *  reports correct properties for all the defined uniforms.
 **/
class GPUShaderFP64Test2 : public deqp::TestCase
{
public:
	/* Public methods */
	GPUShaderFP64Test2(deqp::Context& context);

	virtual void						 deinit();
	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private types */
	typedef glw::GLint captured_varying_type;
	typedef void(GLW_APIENTRY* arbDispatchComputeFunc)(glw::GLuint num_groups_x, glw::GLuint num_groups_y,
													   glw::GLuint num_groups_z);

	/** Shader stage enumeration
	 *
	 */
	enum shaderStage
	{
		COMPUTE_SHADER,
		FRAGMENT_SHADER,
		GEOMETRY_SHADER,
		TESS_CTRL_SHADER,
		TESS_EVAL_SHADER,
		VERTEX_SHADER,
	};

	/** Store details about uniform type
	 *
	 **/
	struct uniformTypeDetails
	{
		uniformTypeDetails(glw::GLuint n_columns, glw::GLuint n_rows);

		glw::GLuint m_n_columns;
		glw::GLuint m_n_rows;
		glw::GLenum m_type;
		std::string m_type_name;
	};

	/** Store details about uniform
	 *
	 **/
	struct uniformDetails
	{
		glw::GLuint m_array_stride;
		glw::GLuint m_matrix_stride;
		glw::GLuint m_offset;
	};

	/* Provate methods */
	glw::GLenum getCapturedPrimitiveType(shaderStage shader_stage) const;
	glw::GLenum getDrawPrimitiveType(shaderStage shader_stage) const;
	glw::GLuint getMaxUniformComponents(shaderStage shader_stage) const;
	glw::GLuint getMaxUniformBlockSize() const;
	glw::GLuint getRequiredComponentsNumber(const uniformTypeDetails& uniform_type) const;
	glw::GLuint getUniformTypeMemberSize(const uniformTypeDetails& uniform_type) const;
	glw::GLuint getAmountUniforms(shaderStage shader_stage, const uniformTypeDetails& uniform_type) const;
	const glw::GLchar* getShaderStageName(shaderStage shader_stage) const;

	void inspectProgram(glw::GLuint program_id, glw::GLint n_uniforms, const uniformTypeDetails& uniform_type,
						glw::GLint& out_buffer_size, uniformDetails& out_offsets,
						glw::GLuint uniform_block_index) const;

	void prepareBoilerplateShader(const glw::GLchar* stage_specific_layout, const glw::GLchar* stage_specific_main_body,
								  std::string& out_source_code) const;

	void prepareProgram(shaderStage shader_stage, const uniformTypeDetails& uniform_type,
						Utils::programInfo& out_program_info) const;

	void prepareShaderStages();

	void prepareTestShader(const glw::GLchar* stage_specific_layout, const glw::GLchar* uniform_definitions,
						   const glw::GLchar* in_variable_definitions, const glw::GLchar* out_variable_definitions,
						   const glw::GLchar* uniform_verification, const glw::GLchar* stage_specific_main_body,
						   std::string& out_source_code) const;

	void prepareTestComputeShader(const glw::GLchar* uniform_definitions, const glw::GLchar* uniform_verification,
								  std::string& out_source_code) const;

	void prepareUniformDefinitions(shaderStage shader_stage, const uniformTypeDetails& uniform_type,
								   std::string& out_source_code) const;

	void prepareUniforms(shaderStage shader_stage, const uniformTypeDetails& uniform_type,
						 const Utils::programInfo& program_info) const;

	void prepareUniformTypes();

	void prepareUniformVerification(shaderStage shader_stage, const uniformTypeDetails& uniform_type,
									std::string& out_source_code) const;

	bool test(shaderStage shader_stage, const uniformTypeDetails& uniform_type) const;

	void testBegin(glw::GLuint program_id, shaderStage shader_stage) const;

	void testEnd(shaderStage shader_stage) const;
	void testInit();
	bool verifyResults(shaderStage shader_stage) const;

	/* Private constants */
	static const glw::GLuint m_n_captured_results;
	static const glw::GLint  m_result_failure;
	static const glw::GLint  m_result_success;
	static const glw::GLuint m_texture_width;
	static const glw::GLuint m_texture_height;
	static const glw::GLuint m_transform_feedback_buffer_size;

	static const glw::GLchar* m_uniform_block_name;

	static const glw::GLenum ARB_MAX_COMPUTE_UNIFORM_COMPONENTS;

	/* Private fields */
	arbDispatchComputeFunc m_pDispatchCompute;

	std::vector<shaderStage>		m_shader_stages;
	std::vector<uniformTypeDetails> m_uniform_types;

	glw::GLuint m_framebuffer_id;
	glw::GLuint m_texture_id;
	glw::GLuint m_transform_feedback_buffer_id;
	glw::GLuint m_uniform_buffer_id;
	glw::GLuint m_vertex_array_object_id;
};

/** Implements "Subtest 3" from CTS_ARB_gpu_shader_fp64, description follows:
 *
 *  Make sure it is possible to use new types for member declaration
 *  in a named uniform block.
 *  The following members should be defined in the block:
 *
 *  ivec3   dummy1[3];
 *  double  double_value;
 *  bool    dummy2;
 *  dvec2   dvec2_value;
 *  bvec3   dummy3;
 *  dvec3   dvec3_value;
 *  int     dummy4[3];
 *  dvec4   dvec4_value;
 *  bool    dummy5;
 *  bool    dummy6[2];
 *  dmat2   dmat2_value;
 *  dmat3   dmat3_value;
 *  bool    dummy7;
 *  dmat4   dmat4_value;
 *  dmat2x3 dmat2x3_value;
 *  uvec3   dummy8;
 *  dmat2x4 dmat2x4_value;
 *  dmat3x2 dmat3x2_value;
 *  bool    dummy9;
 *  dmat3x4 dmat3x4_value;
 *  int     dummy10;
 *  dmat4x2 dmat4x2_value;
 *  dmat4x3 dmat4x3_value;
 *
 *  std140, packed and shared layout qualifiers should be tested
 *  separately.
 *
 *  For the purpose of the test, a buffer object, storage of which
 *  is to be used for the uniform buffer, should be filled with
 *  predefined values at member-specific offsets. These values
 *  should then be read in each shader stage covered by the test
 *  and verified.
 *
 *  For std140 layout qualifier, the test should additionally verify
 *  the offsets assigned by GL implementation are as per specification.
 *
 *  The following shader stages should be defined for a program object
 *  to be used by the test:
 *
 *  1) Vertex shader stage;
 *  2) Geometry shader stage;
 *  3) Tessellation control shader stage;
 *  4) Tessellation evaluation shader stage;
 *  5) Fragment shader stage;
 *
 *  Vertex shader stage should set a stage-specific bool variable to
 *  true if all uniform buffer members are assigned valid values,
 *  false otherwise.
 *
 *  Geometry shader stage should take points on input and emit a single
 *  point. Similarly to vertex shader stage, it should set a stage-specific
 *  bool variable to true, if all uniform buffer members are determined
 *  to expose valid values, false otherwise.
 *  Geometry shader stage should pass the result value from the vertex
 *  shader stage down the rendering pipeline using a new output variable,
 *  set to the value of the variable exposed by vertex shader stage.
 *
 *  Tessellation control shader stage should set all inner/outer
 *  tessellation levels to 1 and output 1 vertex per patch.
 *  The verification should be carried out as in previous stages.
 *  TC shader stage should also define stage-specific output variables
 *  for vertex and geometry shader stages and set them to relevant
 *  values, similar to what's been described for geometry shader stage.
 *
 *  Tessellation evaluation shader stage should take quads on
 *  input. It should be constructed in a way that will make it
 *  generate a quad that occupies whole screen-space. This is necessary
 *  to perform validation of the input variables in fragment shader
 *  stage.
 *  The verification should be carried out as in previous stages.
 *  TE stage should pass down validation results from previous stages
 *  in a similar manner as was described for TC shader stage.
 *
 *  Fragment shader stage should verify all the uniform buffer members
 *  carry valid values. Upon success, it should output vec4(1) to the
 *  only output variable. Otherwise, it should set it to vec4(0).
 *
 *  XFB should be used to read all the output variables defined in TE
 *  stage that carry result information from all the rendering stages
 *  until tessellation evaluation shader stage. The test passes, if
 *  all result values are set to true and the draw buffer is filled
 *  with vec4(1).
 **/
class GPUShaderFP64Test3 : public deqp::TestCase
{
public:
	/* Public methods */
	GPUShaderFP64Test3(deqp::Context&);
	virtual ~GPUShaderFP64Test3()
	{
	}

	/* Public methods inherited from TestCase */
	virtual void						 deinit(void);
	virtual tcu::TestNode::IterateResult iterate(void);

private:
	/* Private types */

	/** Enumerate shader stages
	 *
	 **/
	enum shaderStage
	{
		FRAGMENT_SHADER,
		GEOMETRY_SHADER,
		TESS_CONTROL_SHADER,
		TESS_EVAL_SHADER,
		VERTEX_SHADER,
	};

	/** Enumerate buffer data layouts
	 *
	 **/
	enum uniformDataLayout
	{
		PACKED,
		SHARED,
		STD140,
	};

	/** Store details about "double precision" uniforms
	 *
	 **/
	struct uniformDetails
	{
		uniformDetails(glw::GLint expected_std140_offset, const glw::GLchar* name, glw::GLuint n_columns,
					   glw::GLuint n_elements, const glw::GLchar* type_name)
			: m_expected_std140_offset(expected_std140_offset)
			, m_name(name)
			, m_n_columns(n_columns)
			, m_n_elements(n_elements)
			, m_type_name(type_name)
		{
			/* Nothing to be done */
		}

		glw::GLint		   m_expected_std140_offset;
		const glw::GLchar* m_name;
		glw::GLuint		   m_n_columns;
		glw::GLuint		   m_n_elements;
		const glw::GLchar* m_type_name;
	};

	/** Store details about program object
	 *
	 **/
	struct programInfo
	{

		programInfo();

		void compile(deqp::Context& context, glw::GLuint shader_id, const glw::GLchar* shader_code) const;

		void deinit(deqp::Context& context);

		void init(deqp::Context& context, const std::vector<uniformDetails> m_uniform_details,
				  const glw::GLchar* fragment_shader_code, const glw::GLchar* geometry_shader_code,
				  const glw::GLchar* tesselation_control_shader_code,
				  const glw::GLchar* tesselation_evaluation_shader_code, const glw::GLchar* vertex_shader_code);

		void link(deqp::Context& context) const;

		static const glw::GLint m_invalid_uniform_offset;
		static const glw::GLint m_invalid_uniform_matrix_stride;
		static const glw::GLint m_non_matrix_uniform_matrix_stride;

		glw::GLuint m_fragment_shader_id;
		glw::GLuint m_geometry_shader_id;
		glw::GLuint m_program_object_id;
		glw::GLuint m_tesselation_control_shader_id;
		glw::GLuint m_tesselation_evaluation_shader_id;
		glw::GLuint m_vertex_shader_id;

		glw::GLint  m_buffer_size;
		glw::GLuint m_uniform_block_index;

		std::vector<glw::GLint> m_uniform_offsets;
		std::vector<glw::GLint> m_uniform_matrix_strides;
	};

	/* Private methods */
	glw::GLdouble getExpectedValue(glw::GLuint type_ordinal, glw::GLuint element) const;
	const programInfo& getProgramInfo(uniformDataLayout uniform_data_layout) const;
	const glw::GLchar* getUniformLayoutName(uniformDataLayout uniform_data_layout) const;

	void prepareProgram(programInfo& program_info, uniformDataLayout uniform_data_layout) const;

	bool prepareUniformBuffer(const programInfo& program_info, bool verify_offsets) const;

	void testInit();
	bool test(uniformDataLayout uniform_data_layout) const;
	bool verifyResults() const;

	void writeMainBody(std::ostream& stream, shaderStage shader_stage) const;

	void writePreamble(std::ostream& stream, shaderStage shader_stage) const;

	void writeUniformBlock(std::ostream& stream, uniformDataLayout uniform_data_layout) const;

	void writeVaryingDeclarations(std::ostream& stream, shaderStage shader_stage) const;

	void writeVaryingPassthrough(std::ostream& stream, shaderStage shader_stage) const;

	/* Private fields */
	/* Constants */
	static const glw::GLuint m_result_failure;
	static const glw::GLuint m_result_success;

	static const glw::GLchar* m_uniform_block_name;
	static const glw::GLchar* m_uniform_block_instance_name;

	static const glw::GLchar* m_varying_name_fs_out_fs_result;
	static const glw::GLchar* m_varying_name_gs_fs_gs_result;
	static const glw::GLchar* m_varying_name_gs_fs_tcs_result;
	static const glw::GLchar* m_varying_name_gs_fs_tes_result;
	static const glw::GLchar* m_varying_name_gs_fs_vs_result;
	static const glw::GLchar* m_varying_name_tcs_tes_tcs_result;
	static const glw::GLchar* m_varying_name_tcs_tes_vs_result;
	static const glw::GLchar* m_varying_name_tes_gs_tcs_result;
	static const glw::GLchar* m_varying_name_tes_gs_tes_result;
	static const glw::GLchar* m_varying_name_tes_gs_vs_result;
	static const glw::GLchar* m_varying_name_vs_tcs_vs_result;

	/* GL objects */
	glw::GLuint m_color_texture_id;
	glw::GLuint m_framebuffer_id;
	glw::GLuint m_transform_feedback_buffer_id;
	glw::GLuint m_uniform_buffer_id;
	glw::GLuint m_vertex_array_object_id;

	/* Random values used by getExpectedValue */
	glw::GLdouble m_base_element;
	glw::GLdouble m_base_type_ordinal;

	/* Program details, one per data layout */
	programInfo m_packed_program;
	programInfo m_shared_program;
	programInfo m_std140_program;

	/* Storage for uniforms' details */
	std::vector<uniformDetails> m_uniform_details;
};

/** Make sure glGetUniformdv() reports correct values, as assigned
 *  by corresponding glUniform*() functions, for the following
 *  uniform types:
 *
 *  a) double;
 *  b) dvec2;
 *  c) dvec3;
 *  d) dvec4;
 *  e) Arrays of a)-d);
 *  f) a)-d) defined in single-dimensional arrays built of the same
 *     structure.
 *
 *  These uniforms should be defined in the default uniform block,
 *  separately in each stage defined for the following program
 *  objects:
 *
 *  a) A program object consisting of a fragment, geometry, tessellation
 *     control, tessellation evaluation, vertex shader stages.
 *  b) A program object consisting of a compute shader stage.
 *
 *  If GL_ARB_program_interface_query is supported, the test should
 *  also verify that the following API functions work correctly with
 *  the described uniforms:
 *
 *  - glGetProgramResourceiv()    (query GL_TYPE and GL_ARRAY_SIZE
 *                                 properties of GL_UNIFORM interface
 *                                 for all uniforms);
 *  - glGetProgramResourceIndex() (use GL_UNIFORM interface)
 *  - glGetProgramResourceName()  (use GL_UNIFORM interface)
 *
 *  To verify the values returned by these functions, values returned
 *  by relevant glGetUniform*() API functions should be used as
 *  reference.
 */
class GPUShaderFP64Test4 : public deqp::TestCase
{
public:
	/* Public methods */
	GPUShaderFP64Test4(deqp::Context& context);

	virtual void						 deinit();
	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private type definitions */
	/* Defines a type used as a data container for one of the test cases */
	typedef std::pair<glw::GLint /* uniform location */, double* /* value(s) assigned */> uniform_value_pair;

	/* Holds uniform locations & associated values. used by one of the test cases */
	struct _data
	{
		_data();

		double uniform_double;
		double uniform_double_arr[2];
		double uniform_dvec2[2];
		double uniform_dvec2_arr[4];
		double uniform_dvec3[3];
		double uniform_dvec3_arr[6];
		double uniform_dvec4[4];
		double uniform_dvec4_arr[8];

		glw::GLint uniform_location_double;
		glw::GLint uniform_location_double_arr[2];
		glw::GLint uniform_location_dvec2;
		glw::GLint uniform_location_dvec2_arr[2];
		glw::GLint uniform_location_dvec3;
		glw::GLint uniform_location_dvec3_arr[2];
		glw::GLint uniform_location_dvec4;
		glw::GLint uniform_location_dvec4_arr[2];
	};

	/** Holds uniform location & properties information. Used by one of the test cases. */
	struct _program_interface_query_test_item
	{
		glw::GLint  expected_array_size;
		std::string name;
		glw::GLenum expected_type;
		glw::GLint  location;
	};

	/** Holds information on all uniforms defined for a single shader stage. */
	struct _stage_data
	{
		_data uniform_structure_arrays[2];
		_data uniforms;
	};

	/* Private methods */
	void generateUniformValues();
	void initProgramObjects();
	void initTest();
	void initUniformValues();
	bool verifyProgramInterfaceQuerySupport();
	bool verifyUniformValues();

	/* Private declarations */
	bool  m_has_test_passed;
	char* m_uniform_name_buffer;

	glw::GLuint m_cs_id;
	glw::GLuint m_fs_id;
	glw::GLuint m_gs_id;
	glw::GLuint m_po_cs_id;
	glw::GLuint m_po_noncs_id;
	glw::GLuint m_tc_id;
	glw::GLuint m_te_id;
	glw::GLuint m_vs_id;

	_stage_data m_data_cs;
	_stage_data m_data_fs;
	_stage_data m_data_gs;
	_stage_data m_data_tc;
	_stage_data m_data_te;
	_stage_data m_data_vs;
};

/** Make sure the following implicit conversion work correctly:
 *
 *   a) int    -> double;
 *   b) ivec2  -> dvec2;
 *   c) ivec3  -> dvec3;
 *   d) ivec4  -> dvec4;
 *   e) uint   -> double;
 *   f) uvec2  -> dvec2;
 *   g) uvec3  -> dvec3;
 *   h) uvec4  -> dvec4;
 *   i) float  -> double;
 *   j) vec2   -> dvec2;
 *   k) vec3   -> dvec3;
 *   l) vec4   -> dvec4;
 *   m) mat2   -> dmat2;
 *   n) mat3   -> dmat3;
 *   o) mat4   -> dmat4;
 *   p) mat2x3 -> dmat2x3;
 *   q) mat2x4 -> dmat2x4;
 *   r) mat3x2 -> dmat3x2;
 *   s) mat3x4 -> dmat3x4;
 *   t) max4x2 -> dmat4x2;
 *   u) mat4x3 -> dmat4x3;
 *
 *   The test should also verify the following explicit conversions
 *   are supported (that is: when the right-side value is used as
 *   an argument to a constructor of left-side type):
 *
 *   a) int    -> double;
 *   b) uint   -> double;
 *   c) float  -> double;
 *   d) double -> int;
 *   e) double -> uint;
 *   f) double -> float;
 *   g) double -> bool;
 *   h) bool   -> double;
 *
 *   For each conversion, the test should create a program object and
 *   attach a vertex shader to it.
 *   The shader should define an uniform named "base_value", with
 *   its type dependent on the type defined left-side for particular
 *   case, as defined below:
 *
 *   [base_value type: bool]
 *   bool
 *
 *   [base_value type: double]
 *   double
 *
 *   [base_value type: int]
 *   int, ivec2, ivec3, ivec4
 *
 *   [base_value type: uint]
 *   uint, uvec2, uvec3, uvec4
 *
 *   [base_value type: float]
 *   float,  vec2,   vec3,   vec4,
 *   mat2,   mat3,   mat4,   mat2x3,
 *   mat2x4, mat3x2, mat3x4, mat4x2,
 *   mat4x3
 *
 *   For each tested pair, it should build the "source" value/vector/matrix
 *   by taking the value specified in uniform of the same type as the one
 *   that is to be used as source, and use it for zeroth component. First
 *   component should be larger by one, second component should be bigger
 *   by two, and so on.
 *   Once the source value/vector/matrix is defined, the casting operation
 *   should be performed, giving a "destination" value/vector/matrix of
 *   type as defined for particular case.
 *   The resulting value should be XFBed out to the test implementation.
 *   The comparison should be performed on CPU to ensure validity of
 *   the resulting data.
 *
 *   A swizzling operator should be used where possible when setting
 *   the output variables to additionally check that swizzling works
 *   correctly for multi-component double-precision types.
 *
 *   The program object should be used for the following base values:
 *
 *   a) -25.12065
 *   b)   0.0
 *   c)   0.001
 *   d)   1.0
 *   e) 256.78901
 *
 *   An epsilon of 1e-5 should be used for floating-point comparisons.
 **/
class GPUShaderFP64Test5 : public deqp::TestCase
{
public:
	/* Public methods */
	GPUShaderFP64Test5(deqp::Context& context);

	virtual void						 deinit();
	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private type definitions */
	/* Defines swizzle operators used by shaders generated by the test */
	enum _swizzle_type
	{
		SWIZZLE_TYPE_NONE,

		SWIZZLE_TYPE_XWZY,
		SWIZZLE_TYPE_XZXY,
		SWIZZLE_TYPE_XZY,
		SWIZZLE_TYPE_XZYW,

		SWIZZLE_TYPE_Y,
		SWIZZLE_TYPE_YX,
		SWIZZLE_TYPE_YXX,
		SWIZZLE_TYPE_YXXY,

		SWIZZLE_TYPE_Z,
		SWIZZLE_TYPE_ZY,

		SWIZZLE_TYPE_W,
		SWIZZLE_TYPE_WX,
	};

	/* Defines cast type to be used for specific test case */
	enum _test_case_type
	{
		TEST_CASE_TYPE_EXPLICIT,
		TEST_CASE_TYPE_IMPLICIT,

		/* Always last */
		TEST_CASE_TYPE_UNKNOWN
	};

	/* Holds a complete description of a single test case */
	struct _test_case
	{
		_test_case_type type;

		Utils::_variable_type src_type;
		Utils::_variable_type dst_type;

		std::string shader_body;
	};

	/* Private methods */
	bool executeIteration(const _test_case& test_case);

	void getSwizzleTypeProperties(_swizzle_type type, std::string* out_swizzle_string, unsigned int* out_n_components,
								  unsigned int* out_component_order);

	std::string getVertexShaderBody(const _test_case& test_case);
	void initIteration(_test_case& test_case);
	void initTest();

	bool verifyXFBData(const unsigned char* data_ptr, const _test_case& test_case);

	void deinitInteration();

	/* Private declarations */
	unsigned char* m_base_value_bo_data;
	glw::GLuint	m_base_value_bo_id;
	bool		   m_has_test_passed;
	glw::GLint	 m_po_base_value_attribute_location;
	glw::GLint	 m_po_id;
	glw::GLuint	m_vao_id;
	glw::GLint	 m_vs_id;
	glw::GLuint	m_xfb_bo_id;
	unsigned int   m_xfb_bo_size;

	float		  m_base_values[5]; /* as per test spec */
	_swizzle_type m_swizzle_matrix[4 /* max number of dst components */][4 /* max number of src components */];
};

/** The test should verify it is a compilation error to perform the
 * following casts in the shader:
 *
 * a) int   [2] -> double;
 * b) ivec2 [2] -> dvec2;
 * c) ivec3 [2] -> dvec3;
 * d) ivec4 [2] -> dvec4;
 * e) uint  [2] -> double;
 * f) uvec2 [2] -> dvec2;
 * g) uvec3 [2] -> dvec3;
 * h) uvec4 [2] -> dvec4;
 * i) float [2] -> double;
 * j) vec2  [2] -> dvec2;
 * k) vec3  [2] -> dvec3;
 * l) vec4  [2] -> dvec4;
 * m) mat2  [2] -> dmat2;
 * n) mat3  [2] -> dmat3;
 * o) mat4  [2] -> dmat4;
 * p) mat2x3[2] -> dmat2x3;
 * q) mat2x4[2] -> dmat2x4;
 * r) mat3x2[2] -> dmat3x2;
 * s) mat3x4[2] -> dmat3x4;
 * t) mat4x2[2] -> dmat4x2;
 * u) mat4x3[2] -> dmat4x3;
 *
 * The test should also attempt to cast all types defined left-side
 * in a)-u) to structures, where the only variable embedded inside
 * the structure would be defined on the right-side of the test
 * case considered.
 *
 * The following shader stages should be considered for the purpose
 * of the test:
 *
 * 1) Fragment shader stage;
 * 2) Geometry shader stage;
 * 3) Tessellation control shader stage;
 * 4) Tessellation evaluation shader stage;
 * 5) Vertex shader stage;
 * 6) Compute shader stage;
 **/
class GPUShaderFP64Test6 : public deqp::TestCase
{
public:
	/* Public methods */
	GPUShaderFP64Test6(deqp::Context& context);

	virtual void						 deinit();
	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private type definitions */

	/* Holds a complete description of a single test case */
	struct _test_case
	{
		unsigned int		  src_array_size;
		Utils::_variable_type src_type;
		Utils::_variable_type dst_type;

		bool wrap_dst_type_in_structure;

		std::string cs_shader_body;
		std::string fs_shader_body;
		std::string gs_shader_body;
		std::string tc_shader_body;
		std::string te_shader_body;
		std::string vs_shader_body;
	};

	/* Private methods */
	bool executeIteration(const _test_case& test_case);
	std::string getComputeShaderBody(const _test_case& test_case);
	std::string getFragmentShaderBody(const _test_case& test_case);
	std::string getGeometryShaderBody(const _test_case& test_case);
	std::string getTessellationControlShaderBody(const _test_case& test_case);
	std::string getTessellationEvaluationShaderBody(const _test_case& test_case);
	std::string getVertexShaderBody(const _test_case& test_case);

	void initTest();
	void initIteration(_test_case& test_case);

	/* Private declarations */
	glw::GLuint m_cs_id;
	glw::GLuint m_fs_id;
	glw::GLuint m_gs_id;
	glw::GLuint m_tc_id;
	glw::GLuint m_te_id;
	glw::GLuint m_vs_id;

	bool m_has_test_passed;
};

/** Make sure that double-precision types (double, dvec2, dvec3,
 *  dvec4, dmat2, dmat3, dmat4, dmat2x3, dmat2x4, dmat3x2, dmat3x4,
 *  dmat4x2, dmat4x2) and arrays of those:
 *
 *  a) can be used as varyings (excl. vertex shader inputs *if*
 *     GL_ARB_vertex_attrib_64bit support is *not* reported, and
 *     fragment shader outputs; 'flat' layout qualifier should be
 *     used for transferring data to fragment shader stage);
 *  b) cannot be used as fragment shader output; (compilation error
 *     expected).
 *  c) cannot be used as fragment shader input if 'flat' layout
 *     qualifier is missing)
 *
 *  For case a), the following shader stages should be defined for
 *  a single program object:
 *
 *  1) Vertex shader stage;
 *  2) Geometry shader stage;
 *  3) Tessellation control shader stage;
 *  4) Tessellation evaluation shader stage;
 *  5) Fragment shader stage;
 *
 *  Vertex shader stage should define a single output variable for
 *  each type considered. Each component of these output variables
 *  should be set to predefined unique values.
 *  Geometry shader stage should take points on input and emit a single
 *  point. It should take all input variables from the previous stage,
 *  add a predefined value to each component and pass it down the
 *  rendering pipeline.
 *  Tessellation control shader stage should set all inner/outer
 *  tessellation levels to 1 and output 1 vertex per patch.
 *  It should take all input variables from the previous stage,
 *  add a predefined value to each component and pass it to
 *  tessellation evaluation stage by using per-vertex outputs.
 *  Tessellation evaluation shader stage should take quads on
 *  input. It should also define all relevant input variables, as
 *  defined by previous stage, add a predefined value to each
 *  component and pass it to geometry shader stage. Finally, it
 *  should be constructed in a way that will make it generate
 *  a quad that occupies whole screen-space. This is necessary
 *  to perform validation of the input variables in fragment shader
 *  stage.
 *  Fragment shader stage should take all inputs, as defined as
 *  outputs in tessellation evaluation shader stage. It should
 *  verify all the inputs carry valid values. Upon success, it
 *  should output vec4(1) to the only output variable. Otherwise,
 *  it should set it to vec4(0).
 **/
class GPUShaderFP64Test7 : public deqp::TestCase
{
public:
	/* Public methods */
	GPUShaderFP64Test7(deqp::Context& context);

	virtual void						 deinit();
	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private type definitions */
	struct _variable
	{
		glw::GLint			  attribute_location;
		unsigned int		  array_size;
		Utils::_variable_type type;
	};

	typedef std::vector<_variable>	 _variables;
	typedef _variables::const_iterator _variables_const_iterator;

	/* Private methods */
	bool buildTestProgram(_variables& variables);
	bool compileShader(glw::GLint shader_id, const std::string& body);
	void configureXFBBuffer(const _variables& variables);
	bool executeFunctionalTest(_variables& variables);
	void generateXFBVaryingNames(const _variables& variables);

	std::string getCodeOfFragmentShaderWithNonFlatDoublePrecisionInput(Utils::_variable_type input_variable_type,
																	   unsigned int			 array_size);

	std::string getCodeOfFragmentShaderWithDoublePrecisionOutput(Utils::_variable_type output_variable_type,
																 unsigned int		   array_size);

	std::string getFragmentShaderBody(const _variables& variables);
	std::string getGeometryShaderBody(const _variables& variables);
	std::string getTessellationControlShaderBody(const _variables& variables);
	std::string getTessellationEvaluationShaderBody(const _variables& variables);

	std::string getVariableDeclarations(const char* prefix, const _variables& variables,
										const char* layout_qualifier = "");

	std::string getVertexShaderBody(const _variables& variables);
	void initTest();
	void logVariableContents(const _variables& variables);
	void releaseXFBVaryingNames();
	void setInputAttributeValues(const _variables& variables);

	/* Private declarations */
	bool		   m_are_double_inputs_supported;
	std::string	m_current_fs_body;
	std::string	m_current_gs_body;
	std::string	m_current_tc_body;
	std::string	m_current_te_body;
	std::string	m_current_vs_body;
	glw::GLuint	m_fbo_id;
	glw::GLint	 m_fs_id;
	glw::GLint	 m_gs_id;
	bool		   m_has_test_passed;
	glw::GLint	 m_n_max_components_per_stage;
	unsigned int   m_n_xfb_varyings;
	glw::GLint	 m_po_id;
	glw::GLint	 m_tc_id;
	glw::GLint	 m_te_id;
	glw::GLuint	m_to_id;
	unsigned char* m_to_data;
	unsigned int   m_to_height;
	unsigned int   m_to_width;
	glw::GLuint	m_xfb_bo_id;
	glw::GLchar**  m_xfb_varyings;
	glw::GLuint	m_vao_id;
	glw::GLint	 m_vs_id;
};

/** Make sure that all constructors valid for double-precision
 * vector/matrix types are accepted by the GLSL compiler for
 * all six shader stages.
 *
 * The test passes if all shaders compile successfully.
 **/
class GPUShaderFP64Test8 : public deqp::TestCase
{
public:
	/* Public methods */
	GPUShaderFP64Test8(deqp::Context& context);

	virtual void						 deinit();
	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private type definitions */
	typedef std::vector<Utils::_variable_type> _argument_list;
	typedef _argument_list::const_iterator	 _argument_list_const_iterator;
	typedef std::vector<_argument_list>		   _argument_lists;
	typedef _argument_lists::const_iterator	_argument_lists_const_iterator;

	/* Holds a complete description of a single test case */
	struct _argument_list_tree_node;

	typedef std::vector<_argument_list_tree_node*>	_argument_list_tree_nodes;
	typedef _argument_list_tree_nodes::const_iterator _argument_list_tree_nodes_const_iterator;
	typedef std::queue<_argument_list_tree_node*>	 _argument_list_tree_node_queue;

	struct _argument_list_tree_node
	{
		_argument_list_tree_nodes children;
		int						  n_components_used;
		_argument_list_tree_node* parent;
		Utils::_variable_type	 type;

		~_argument_list_tree_node()
		{
			while (children.size() > 0)
			{
				_argument_list_tree_node* node_ptr = children.back();

				children.pop_back();

				delete node_ptr;
				node_ptr = NULL;
			}
		}
	};

	struct _test_case
	{
		_argument_list		  argument_list;
		Utils::_variable_type type;

		std::string cs_shader_body;
		std::string fs_shader_body;
		std::string gs_shader_body;
		std::string tc_shader_body;
		std::string te_shader_body;
		std::string vs_shader_body;
	};

	/* Private methods */
	bool executeIteration(const _test_case& test_case);
	_argument_lists getArgumentListsForVariableType(const Utils::_variable_type& variable_type);
	std::string getComputeShaderBody(const _test_case& test_case);
	std::string getFragmentShaderBody(const _test_case& test_case);
	std::string getGeneralBody(const _test_case& test_case);
	std::string getGeometryShaderBody(const _test_case& test_case);
	std::string getTessellationControlShaderBody(const _test_case& test_case);
	std::string getTessellationEvaluationShaderBody(const _test_case& test_case);
	std::string getVertexShaderBody(const _test_case& test_case);

	void initTest();
	void initIteration(_test_case& test_case);

	/* Private declarations */
	glw::GLuint m_cs_id;
	glw::GLuint m_fs_id;
	glw::GLuint m_gs_id;
	glw::GLuint m_tc_id;
	glw::GLuint m_te_id;
	glw::GLuint m_vs_id;

	bool m_has_test_passed;
};

/** Make sure that the following operators work correctly for
 *  double-precision floating-point scalars, vectors and matrices:
 *
 *  a) +  (addition)
 *  b) -  (subtraction)
 *  c) *  (multiplication)
 *  d) /  (division)
 *  e) -  (negation)
 *  f) -- (pre-decrementation and post-decrementation)
 *  g) ++ (pre-incrementation and post-incrementation)
 *
 *  Furthermore, the following relational operators should also be
 *  tested for double-precision floating-point expressions:
 *
 *  a) <  (less than)
 *  b) <= (less than or equal)
 *  c) >  (greater than)
 *  d) >= (greater than or equal)
 *
 *  For each double-precision floating-point type, the test should
 *  create a program object, to which it should then attach
 *  a vertex shader, body of which was adjusted to handle case-specific
 *  type. The shader should use all the operators and operations
 *  described above. The result value should be XFBed out to the
 *  test for verification.
 *
 *  For relational operators, both cases described below should be
 *  tested:
 *
 *  a) fundamental type of the two operands should match without
 *     any implicit type conversion involved in the process;
 *  b) fundamental type of the two operands should match after an
 *     implicit type conversion (use some of the casts enlisted for
 *     test 6).
 *
 *  The test passes if the returned set of values was correct for
 *  all the types considered. Assume epsilon value of 1e-5.
 *
 **/
class GPUShaderFP64Test9 : public deqp::TestCase
{
public:
	/* Public methods */
	GPUShaderFP64Test9(deqp::Context& context);

	void								 deinit();
	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private type definitions */
	typedef enum {
		OPERATION_TYPE_ADDITION,
		OPERATION_TYPE_DIVISION,
		OPERATION_TYPE_MULTIPLICATION,
		OPERATION_TYPE_SUBTRACTION,
		OPERATION_TYPE_PRE_DECREMENTATION,
		OPERATION_TYPE_PRE_INCREMENTATION,
		OPERATION_TYPE_POST_DECREMENTATION,
		OPERATION_TYPE_POST_INCREMENTATION,

		/* Always last */
		OPERATION_TYPE_COUNT
	} _operation_type;

	struct _test_case
	{
		_operation_type		  operation_type;
		Utils::_variable_type result_variable_type;
		std::string			  vs_body;
		Utils::_variable_type variable_type;
	};

	/* Private methods */
	bool executeTestIteration(const _test_case& test_case);

	void getMatrixMultiplicationResult(const Utils::_variable_type& matrix_a_type,
									   const std::vector<double>&   matrix_a_data,
									   const Utils::_variable_type& matrix_b_type,
									   const std::vector<double>& matrix_b_data, double* out_result_ptr);

	const char* getOperatorForOperationType(const _operation_type& operation_type);
	std::string getOperationTypeString(const _operation_type& operation_type);
	std::string getVertexShaderBody(_test_case& test_case);
	void initTest();
	void initTestIteration(_test_case& test_case);

	bool verifyXFBData(const _test_case& test_case, const unsigned char* xfb_data);

	/* Private fields */
	bool		m_has_test_passed;
	glw::GLuint m_po_id;
	glw::GLuint m_xfb_bo_id;
	glw::GLuint m_vao_id;
	glw::GLuint m_vs_id;
};

/** Group class for GPU Shader FP64 conformance tests */
class GPUShaderFP64Tests : public deqp::TestCaseGroup
{
public:
	/* Public methods */
	GPUShaderFP64Tests(deqp::Context& context);
	virtual ~GPUShaderFP64Tests()
	{
	}

	virtual void init(void);

private:
	/* Private methods */
	GPUShaderFP64Tests(const GPUShaderFP64Tests&);
	GPUShaderFP64Tests& operator=(const GPUShaderFP64Tests&);
};

} /* gl4cts namespace */

#endif // _GL4CGPUSHADERFP64TESTS_HPP
