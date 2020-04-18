#ifndef _ESEXTCGEOMETRYSHADERLIMITS_HPP
#define _ESEXTCGEOMETRYSHADERLIMITS_HPP
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
 * \file esextcGeometryShaderLimits.hpp
 * \brief Geometry Shader Limits (Test Group 16)
 */ /*-------------------------------------------------------------------*/

#include "../esextcTestCaseBase.hpp"

#include <vector>

namespace glcts
{
/** Parent class for geometry shader Test Group 16 tests
 *  based on fetching result via transfrom feedback.
 **/
class GeometryShaderLimitsTransformFeedbackBase : public TestCaseBase
{
public:
	/* Public methods */
	virtual void		  deinit(void);
	virtual IterateResult iterate(void);

protected:
	/* Protected methods */
	GeometryShaderLimitsTransformFeedbackBase(Context& context, const ExtParameters& extParams, const char* name,
											  const char* description);

	virtual ~GeometryShaderLimitsTransformFeedbackBase()
	{
	}

	void initTest(void);

	/* Methods to be overriden by inheriting test cases */
	virtual void clean() = 0;

	virtual void getCapturedVaryings(const glw::GLchar* const*& out_captured_varyings_names,
									 glw::GLuint&				out_n_captured_varyings) = 0;

	virtual void getShaderParts(const glw::GLchar* const*& out_fragment_shader_parts,
								glw::GLuint&			   out_n_fragment_shader_parts,
								const glw::GLchar* const*& out_geometry_shader_parts,
								glw::GLuint&			   out_n_geometry_shader_parts,
								const glw::GLchar* const*& out_vertex_shader_parts,
								glw::GLuint&			   out_n_vertex_shader_parts) = 0;

	virtual void getTransformFeedbackBufferSize(glw::GLuint& out_buffer_size) = 0;
	virtual void prepareProgramInput()										  = 0;
	virtual bool verifyResult(const void* data)								  = 0;

	/* Protected fields */
	/* Program and shader ids */
	glw::GLuint m_fragment_shader_id;
	glw::GLuint m_geometry_shader_id;
	glw::GLuint m_program_object_id;
	glw::GLuint m_vertex_shader_id;

	/* Buffer object used in transform feedback */
	glw::GLuint m_buffer_object_id;

	/* Vertex array object */
	glw::GLuint m_vertex_array_object_id;

private:
	/* Private fields */
	/* Shaders' code */
	const glw::GLchar* const* m_fragment_shader_parts;
	const glw::GLchar* const* m_geometry_shader_parts;
	const glw::GLchar* const* m_vertex_shader_parts;

	glw::GLuint m_n_fragment_shader_parts;
	glw::GLuint m_n_geometry_shader_parts;
	glw::GLuint m_n_vertex_shader_parts;

	/* Names of varyings */
	const glw::GLchar* const* m_captured_varyings_names;
	glw::GLuint				  m_n_captured_varyings;

	/* Size of buffer used by transform feedback */
	glw::GLuint m_buffer_size;
};

/** Parent class for geometry shader Test Group 16 tests
 *  based on fetching result via rendering to texture.
 **/
class GeometryShaderLimitsRenderingBase : public TestCaseBase
{
public:
	/* Public methods */
	virtual void		  deinit(void);
	virtual IterateResult iterate(void);

protected:
	/* Protected methods */
	GeometryShaderLimitsRenderingBase(Context& context, const ExtParameters& extParams, const char* name,
									  const char* description);

	virtual ~GeometryShaderLimitsRenderingBase()
	{
	}

	void initTest(void);

	/* Methods to be overriden by child test cases */
	virtual void clean() = 0;

	virtual void getDrawCallDetails(glw::GLenum& out_primitive_type, glw::GLuint& out_n_vertices) = 0;

	virtual void getFramebufferDetails(glw::GLenum& out_texture_format, glw::GLenum& out_texture_read_format,
									   glw::GLenum& out_texture_read_type, glw::GLuint& out_texture_width,
									   glw::GLuint& out_texture_height, glw::GLuint& out_texture_pixel_size) = 0;

	virtual void getRequiredPointSize(glw::GLfloat& out_point_size) = 0;

	virtual void getShaderParts(const glw::GLchar* const*& out_fragment_shader_parts,
								glw::GLuint&			   out_n_fragment_shader_parts,
								const glw::GLchar* const*& out_geometry_shader_parts,
								glw::GLuint&			   out_n_geometry_shader_parts,
								const glw::GLchar* const*& out_vertex_shader_parts,
								glw::GLuint&			   out_n_vertex_shader_parts) = 0;

	virtual void prepareProgramInput()			= 0;
	virtual bool verifyResult(const void* data) = 0;

	/* Protected fields */
	/* Program and shader ids */
	glw::GLuint m_fragment_shader_id;
	glw::GLuint m_geometry_shader_id;
	glw::GLuint m_program_object_id;
	glw::GLuint m_vertex_shader_id;

	/* Framebuffer object id */
	glw::GLuint m_framebuffer_object_id;
	glw::GLuint m_color_texture_id;

	/* Vertex array object */
	glw::GLuint m_vertex_array_object_id;

private:
	/* Private fields */
	/* Shaders' code */
	const glw::GLchar* const* m_fragment_shader_parts;
	const glw::GLchar* const* m_geometry_shader_parts;
	const glw::GLchar* const* m_vertex_shader_parts;

	glw::GLuint m_n_fragment_shader_parts;
	glw::GLuint m_n_geometry_shader_parts;
	glw::GLuint m_n_vertex_shader_parts;

	/* Framebuffer dimensions */
	glw::GLenum m_texture_format;
	glw::GLuint m_texture_height;
	glw::GLuint m_texture_pixel_size;
	glw::GLenum m_texture_read_format;
	glw::GLenum m_texture_read_type;
	glw::GLuint m_texture_width;
};

/** Implementation of test case 16.1. Test description follows:
 *
 *  Make sure it is possible to use as many uniform components as defined
 *  by GL_MAX_GEOMETRY_UNIFORM_COMPONENTS_EXT
 *
 *  Category: API;
 *            Functional Test.
 *
 *  1. Create a fragment, geometry and vertex shader objects:
 *
 *  - Vertex shader code can be boilerplate;
 *  - Geometry shader code should define
 *    floor(GL_MAX_GEOMETRY_UNIFORM_COMPONENTS_EXT / 4) uniform ivec4
 *    variables. It should take points as input and output, a maximum of
 *    1 vertex will be written by the shader. In main(), the shader should
 *    set output int variable named result to a sum of all the vectors'
 *    components and emit a vertex.
 *  - Fragment shader code can be boilerplate.
 *
 *  2. The program object consisting of these shader objects is expected to
 *  link successfully.
 *
 *  3. Configure the uniforms to use subsequently increasing values, starting
 *  from 1 for R component of first vector, 2 for G component of that vector,
 *  5 for first component of second vector, and so on.
 *
 *  4. Configure transform feedback object to capture output from result.
 *  Draw a single point. The test succeeds if first component of the result
 *  vector contains a valid value (bearing potentially minor precision issues
 *  in mind)
 **/
class GeometryShaderMaxUniformComponentsTest : public GeometryShaderLimitsTransformFeedbackBase
{
public:
	/* Public methods */
	GeometryShaderMaxUniformComponentsTest(Context& context, const ExtParameters& extParams, const char* name,
										   const char* description);

	virtual ~GeometryShaderMaxUniformComponentsTest()
	{
	}

protected:
	/* Overriden from GeometryShaderLimitsTransformFeedbackBase */
	virtual void clean();

	virtual void getCapturedVaryings(const glw::GLchar* const*& out_captured_varyings_names,
									 glw::GLuint&				out_n_captured_varyings);

	virtual void getShaderParts(const glw::GLchar* const*& out_fragment_shader_parts,
								glw::GLuint&			   out_n_fragment_shader_parts,
								const glw::GLchar* const*& out_geometry_shader_parts,
								glw::GLuint&			   out_n_geometry_shader_parts,
								const glw::GLchar* const*& out_vertex_shader_parts,
								glw::GLuint&			   out_n_vertex_shader_parts);

	virtual void getTransformFeedbackBufferSize(glw::GLuint& out_buffer_size);
	virtual void prepareProgramInput();
	virtual bool verifyResult(const void* data);

private:
	/* Private fields */
	/* Shaders' code */
	static const glw::GLchar* const m_fragment_shader_code;
	static const glw::GLchar* const m_geometry_shader_code_preamble;
	static const glw::GLchar* const m_geometry_shader_code_number_of_uniforms;
	static const glw::GLchar* const m_geometry_shader_code_body;
	static const glw::GLchar* const m_vertex_shader_code;

	const glw::GLchar* m_geometry_shader_parts[4];

	/* String used to store number of uniform vectors */
	std::string m_max_uniform_vectors_string;

	/* Varying names */
	static const glw::GLchar* const m_captured_varyings_names;

	/* Buffer size */
	static const glw::GLuint m_buffer_size;

	/* Max uniform components and vectors */
	glw::GLint m_max_uniform_components;
	glw::GLint m_max_uniform_vectors;

	/* Uniform location */
	glw::GLint m_uniform_location;

	/* Uniform data */
	std::vector<glw::GLint> m_uniform_data;
};

/** Implementation of test case 16.2. Test description follows:
 *
 *  Make sure it is possible to use as many uniform blocks as defined by
 *  GL_MAX_GEOMETRY_UNIFORM_BLOCKS_EXT
 *
 *  Category: API;
 *            Functional Test.
 *
 *  Slightly modify test case 16.1 to use a similar idea to test if the
 *  value reported for the property by the implementation is reliable:
 *
 *  - Test case 16.1's ivec4s take form of as many uniform blocks as needed,
 *    each hosting a single int.
 *  - The result value to be calculated in the geometry shader is a sum of
 *    all ints, stored in output int result variable.
 **/
class GeometryShaderMaxUniformBlocksTest : public GeometryShaderLimitsTransformFeedbackBase
{
public:
	/* Public methods */
	GeometryShaderMaxUniformBlocksTest(Context& context, const ExtParameters& extParams, const char* name,
									   const char* description);

	virtual ~GeometryShaderMaxUniformBlocksTest()
	{
	}

protected:
	/* Overriden from GeometryShaderLimitsTransformFeedbackBase */
	virtual void clean();

	virtual void getCapturedVaryings(const glw::GLchar* const*& out_captured_varyings_names,
									 glw::GLuint&				out_n_captured_varyings);

	virtual void getShaderParts(const glw::GLchar* const*& out_fragment_shader_parts,
								glw::GLuint&			   out_n_fragment_shader_parts,
								const glw::GLchar* const*& out_geometry_shader_parts,
								glw::GLuint&			   out_n_geometry_shader_parts,
								const glw::GLchar* const*& out_vertex_shader_parts,
								glw::GLuint&			   out_n_vertex_shader_parts);

	virtual void getTransformFeedbackBufferSize(glw::GLuint& out_buffer_size);
	virtual void prepareProgramInput();
	virtual bool verifyResult(const void* data);

private:
	/* Private type */
	struct _uniform_block
	{
		glw::GLuint buffer_object_id;
		glw::GLint  data;
	};

	/* Private fields */
	/* Shaders' code */
	static const glw::GLchar* const m_fragment_shader_code;
	static const glw::GLchar* const m_geometry_shader_code_preamble;
	static const glw::GLchar* const m_geometry_shader_code_number_of_uniforms;
	static const glw::GLchar* const m_geometry_shader_code_body_str;
	static const glw::GLchar* const m_geometry_shader_code_body_end;
	static const glw::GLchar* const m_vertex_shader_code;

	const glw::GLchar* m_geometry_shader_parts[6];

	/* String used to store uniform blocks accesses */
	std::string m_uniform_block_access_string;

	/* String used to store number of uniform blocks */
	std::string m_max_uniform_blocks_string;

	/* Varying names */
	static const glw::GLchar* const m_captured_varyings_names;

	/* Buffer size */
	static const glw::GLuint m_buffer_size;

	/* Max uniform blocks */
	glw::GLint m_max_uniform_blocks;

	/* Uniform blocks data */
	std::vector<_uniform_block> m_uniform_blocks;
};

/** Implementation of test case 16.3. Test description follows:
 *
 *  Make sure it is possible to use as many input components as defined by
 *  GL_MAX_GEOMETRY_INPUT_COMPONENTS_EXT
 *
 *  Category: API.
 *
 *  Create a program object, attach a fragment, geometry and a vertex shader to it:
 *
 *  - Fragment shader can be boilerplate;
 *  - Vertex shader should define exactly
 *    (GL_MAX_GEOMETRY_INPUT_COMPONENTS_EXT / 4) ivec4 output variables.
 *    Each of the variables should be assigned a vector value of
 *    (n, n+1, n+2, n+3) where n corresponds to "index" of the variable,
 *    assuming the very first output variable has an "index" of 1.
 *  - Geometry shader should define exactly
 *    (GL_MAX_GEOMETRY_INPUT_COMPONENTS_EXT / 4) vec4 input variables. It
 *    should accept input point geometry and output a maximum of 1 point.
 *    It should sum up all components read from input variables and store them
 *    in output int variable called result..
 *
 *  The test should then configure the program object to capture values of
 *  "result" variable using transform feedback and link the program object.
 *
 *  The test should now generate and bind a vertex array object, and then
 *  draw a single point. Test succeeds if the value stored in a buffer object
 *  configured for transform feedback storage is valid.
 **/
class GeometryShaderMaxInputComponentsTest : public GeometryShaderLimitsTransformFeedbackBase
{
public:
	/* Public methods */
	GeometryShaderMaxInputComponentsTest(Context& context, const ExtParameters& extParams, const char* name,
										 const char* description);

	virtual ~GeometryShaderMaxInputComponentsTest()
	{
	}

protected:
	/* Overriden from GeometryShaderLimitsTransformFeedbackBase */
	virtual void clean();

	virtual void getCapturedVaryings(const glw::GLchar* const*& out_captured_varyings_names,
									 glw::GLuint&				out_n_captured_varyings);

	virtual void getShaderParts(const glw::GLchar* const*& out_fragment_shader_parts,
								glw::GLuint&			   out_n_fragment_shader_parts,
								const glw::GLchar* const*& out_geometry_shader_parts,
								glw::GLuint&			   out_n_geometry_shader_parts,
								const glw::GLchar* const*& out_vertex_shader_parts,
								glw::GLuint&			   out_n_vertex_shader_parts);

	virtual void getTransformFeedbackBufferSize(glw::GLuint& out_buffer_size);
	virtual void prepareProgramInput();
	virtual bool verifyResult(const void* data);

private:
	/* Private fields */
	/* Shaders' code */
	static const glw::GLchar* const m_fragment_shader_code;
	static const glw::GLchar* const m_geometry_shader_code_preamble;
	static const glw::GLchar* const m_geometry_shader_code_number_of_uniforms;
	static const glw::GLchar* const m_geometry_shader_code_body;
	static const glw::GLchar* const m_vertex_shader_code_preamble;
	static const glw::GLchar* const m_vertex_shader_code_number_of_uniforms;
	static const glw::GLchar* const m_vertex_shader_code_body;

	const glw::GLchar* m_geometry_shader_parts[4];
	const glw::GLchar* m_vertex_shader_parts[4];

	/* Max input components and vectors */
	glw::GLint m_max_geometry_input_components;
	glw::GLint m_max_geometry_input_vectors;

	/* String used to store number of geometry input vectors */
	std::string m_max_geometry_input_vectors_string;

	/* Varying names */
	static const glw::GLchar* const m_captured_varyings_names;

	/* Buffer size */
	static const glw::GLuint m_buffer_size;
};

/** Implementation of test case 16.4. Test description follows:
 *
 *  Make sure it is possible to use as many total output components as
 *  defined by GL_MAX_GEOMETRY_TOTAL_OUTPUT_COMPONENTS_EXT.
 *
 *  Category: API.
 *
 *  Let n_points be equal to:
 *
 *  (GL_MAX_GEOMETRY_TOTAL_OUTPUT_COMPONENTS_EXT / GL_MAX_GEOMETRY_OUTPUT_COMPONENTS_EXT)
 *
 *  Create a fragment, geometry and vertex shader objects:
 *
 *  - Vertex shader code can be boilerplate;
 *  - Geometry shader code should define:
 *
 *             floor(GL_MAX_GEOMETRY_OUTPUT_COMPONENTS_EXT / 4)
 *
 *   output ivec4 variables. It should take points as input and output,
 *   a maximum of n_points vertices will be written. In main(), for each
 *   vertex, the shader should set gl_Position to:
 *
 *         (-1 + (2 * vertex id + 1) / (2 * max vertices) * 2, 0, 0, 1)
 *
 *   where (vertex id) corresponds to index of about-to-be-emitted vertices,
 *   assuming the indexing starts from 0.
 *   Each vertex should store subsequently increasing, unique values to
 *   components of the output variables.
 *   Geometry shader should emit as many vertices as specified. For each
 *   output point, point size should be set to 2.
 *  - Fragment shader code should take all aforementioned varyings as input
 *   variables, read them, and store result int fragment as sum of all
 *   components for all vectors passed from the geometry shader.
 *
 *  For rendering, the test should use a framebuffer object, to which
 *  a GL_R32I-based 2D texture object of resolution:
 *
 *                              (2*n_points, 2)
 *
 *  has been attached to color attachment 0.
 *
 *  The test should link the program (no linking error should be reported)
 *  and then activate it. Having bound a vertex array object, it should then
 *  draw n_points points.
 *
 *  The test passes, if the texture attached to color attachment 0 consists
 *  of 2x2 quads filled with the same value, that can be considered valid in
 *  light of the description above.
 *  The program object consisting of these shader objects is expected to link
 *  successfully.
 **/
class GeometryShaderMaxOutputComponentsTest : public GeometryShaderLimitsRenderingBase
{
public:
	/* Public methods */
	GeometryShaderMaxOutputComponentsTest(Context& context, const ExtParameters& extParams, const char* name,
										  const char* description);

	virtual ~GeometryShaderMaxOutputComponentsTest()
	{
	}

protected:
	/* Methods overriden from GeometryShaderLimitsRenderingBase */
	virtual void clean();

	virtual void getDrawCallDetails(glw::GLenum& out_primitive_type, glw::GLuint& out_n_vertices);

	virtual void getFramebufferDetails(glw::GLenum& out_texture_format, glw::GLenum& out_texture_read_format,
									   glw::GLenum& out_texture_read_type, glw::GLuint& out_texture_width,
									   glw::GLuint& out_texture_height, glw::GLuint& out_texture_pixel_size);

	virtual void getRequiredPointSize(glw::GLfloat& out_point_size);

	virtual void getShaderParts(const glw::GLchar* const*& out_fragment_shader_parts,
								glw::GLuint&			   out_n_fragment_shader_parts,
								const glw::GLchar* const*& out_geometry_shader_parts,
								glw::GLuint&			   out_n_geometry_shader_parts,
								const glw::GLchar* const*& out_vertex_shader_parts,
								glw::GLuint&			   out_n_vertex_shader_parts);

	virtual void prepareProgramInput();
	virtual bool verifyResult(const void* data);

private:
	/* Private methods */
	void prepareFragmentShader(std::string& out_shader_code) const;
	void prepareGeometryShader(std::string& out_shader_code) const;

	/* Private fields */
	/* Shaders' code */
	static const glw::GLchar* const m_common_shader_code_gs_fs_out;
	static const glw::GLchar* const m_common_shader_code_number_of_points;
	static const glw::GLchar* const m_common_shader_code_gs_fs_out_definitions;
	static const glw::GLchar* const m_fragment_shader_code_preamble;
	static const glw::GLchar* const m_fragment_shader_code_flat_in_ivec4;
	static const glw::GLchar* const m_fragment_shader_code_sum;
	static const glw::GLchar* const m_fragment_shader_code_body_begin;
	static const glw::GLchar* const m_fragment_shader_code_body_end;
	static const glw::GLchar* const m_geometry_shader_code_preamble;
	static const glw::GLchar* const m_geometry_shader_code_layout;
	static const glw::GLchar* const m_geometry_shader_code_flat_out_ivec4;
	static const glw::GLchar* const m_geometry_shader_code_assignment;
	static const glw::GLchar* const m_geometry_shader_code_body_begin;
	static const glw::GLchar* const m_geometry_shader_code_body_end;
	static const glw::GLchar* const m_vertex_shader_code;

	/* Storage for prepared fragment and geometry shader */
	std::string		   m_fragment_shader_code;
	const glw::GLchar* m_fragment_shader_code_c_str;
	std::string		   m_geometry_shader_code;
	const glw::GLchar* m_geometry_shader_code_c_str;

	/* Framebuffer dimensions */
	glw::GLuint				 m_texture_width;
	static const glw::GLuint m_texture_height;
	static const glw::GLuint m_texture_pixel_size;
	static const glw::GLuint m_point_size;

	/* Max number of output components */
	glw::GLint m_max_output_components;
	glw::GLint m_max_output_vectors;
	glw::GLint m_max_total_output_components;
	glw::GLint m_n_available_vectors;
	glw::GLint m_n_output_points;
};

/** Implementation of test case 16.5. Test description follows:
 *
 *  Make sure it possible to request as many output vertices as report for
 *  GL_MAX_GEOMETRY_OUTPUT_VERTICES_EXT. Requesting support for larger amount
 *  of output vertices should cause the linking process to fail.
 *
 *  Category: API;
 *           Negative Test.
 *
 *  Create two program objects and one boilerplate fragment & one boilerplate
 *  vertex shader objects.
 *  Also create two boilerplate geometry shader objects where:
 *
 *  a) The first geometry shader object can output up to
 *    GL_MAX_GEOMETRY_OUTPUT_VERTICES_EXT vertices.
 *  b) The other geometry shader object can output up to
 *    (GL_MAX_GEOMETRY_OUTPUT_VERTICES_EXT+1) vertices.
 *
 *  1) Program object A should be attached fragment and vertex shader
 *    objects, as well as geometry shader A. This program object should
 *    link successfully.
 *  2) Program object B should be attached fragment and vertex shader objects,
 *    as well as geometry shader B. This program object should fail to link.
 **/
class GeometryShaderMaxOutputVerticesTest : public TestCaseBase
{
public:
	/* Public methods */
	GeometryShaderMaxOutputVerticesTest(Context& context, const ExtParameters& extParams, const char* name,
										const char* description);

	virtual ~GeometryShaderMaxOutputVerticesTest()
	{
	}

	virtual IterateResult iterate(void);

private:
	/* Private fields */
	/* Shaders' code */
	static const glw::GLchar* const m_fragment_shader_code;
	static const glw::GLchar* const m_geometry_shader_code_preamble;
	static const glw::GLchar* const m_geometry_shader_code_body;
	static const glw::GLchar* const m_vertex_shader_code;
};

/** Implementation of test case 16.6. Test description follows:
 *
 *  Make sure it is possible to use as many output components as defined by
 *  GL_MAX_GEOMETRY_OUTPUT_COMPONENTS_EXT
 *
 *  Category: API.
 *
 *  Modify test case 16.4, so that:
 *
 *  * n_points is always 1;
 **/
class GeometryShaderMaxOutputComponentsSinglePointTest : public GeometryShaderLimitsRenderingBase
{
public:
	/* Public methods */
	GeometryShaderMaxOutputComponentsSinglePointTest(Context& context, const ExtParameters& extParams, const char* name,
													 const char* description);

	virtual ~GeometryShaderMaxOutputComponentsSinglePointTest()
	{
	}

protected:
	/* Methods overriden from GeometryShaderLimitsRenderingBase */
	virtual void clean();

	virtual void getDrawCallDetails(glw::GLenum& out_primitive_type, glw::GLuint& out_n_vertices);

	virtual void getFramebufferDetails(glw::GLenum& out_texture_format, glw::GLenum& out_texture_read_format,
									   glw::GLenum& out_texture_read_type, glw::GLuint& out_texture_width,
									   glw::GLuint& out_texture_height, glw::GLuint& out_texture_pixel_size);

	virtual void getRequiredPointSize(glw::GLfloat& out_point_size);

	virtual void getShaderParts(const glw::GLchar* const*& out_fragment_shader_parts,
								glw::GLuint&			   out_n_fragment_shader_parts,
								const glw::GLchar* const*& out_geometry_shader_parts,
								glw::GLuint&			   out_n_geometry_shader_parts,
								const glw::GLchar* const*& out_vertex_shader_parts,
								glw::GLuint&			   out_n_vertex_shader_parts);

	virtual void prepareProgramInput();
	virtual bool verifyResult(const void* data);

private:
	/* Private methods */
	void prepareFragmentShader(std::string& out_shader_code) const;
	void prepareGeometryShader(std::string& out_shader_code) const;

	/* Private fields */
	/* Shaders' code */
	static const glw::GLchar* const m_common_shader_code_gs_fs_out;
	static const glw::GLchar* const m_common_shader_code_gs_fs_out_definitions;
	static const glw::GLchar* const m_fragment_shader_code_preamble;
	static const glw::GLchar* const m_fragment_shader_code_flat_in_ivec4;
	static const glw::GLchar* const m_fragment_shader_code_sum;
	static const glw::GLchar* const m_fragment_shader_code_body_begin;
	static const glw::GLchar* const m_fragment_shader_code_body_end;
	static const glw::GLchar* const m_geometry_shader_code_preamble;
	static const glw::GLchar* const m_geometry_shader_code_flat_out_ivec4;
	static const glw::GLchar* const m_geometry_shader_code_assignment;
	static const glw::GLchar* const m_geometry_shader_code_body_begin;
	static const glw::GLchar* const m_geometry_shader_code_body_end;
	static const glw::GLchar* const m_vertex_shader_code;

	/* Storage for prepared fragment and geometry shader */
	std::string m_fragment_shader_code;
	std::string m_geometry_shader_code;

	const glw::GLchar* m_fragment_shader_code_c_str;
	const glw::GLchar* m_geometry_shader_code_c_str;

	/* Framebuffer dimensions */
	static const glw::GLuint m_texture_width;
	static const glw::GLuint m_texture_height;
	static const glw::GLuint m_texture_pixel_size;
	static const glw::GLuint m_point_size;

	/* Max number of output components */
	glw::GLint m_max_output_components;
	glw::GLint m_max_output_vectors;
	glw::GLint m_n_available_vectors;
};

/** Implementation of test case 16.7. Test description follows:
 *
 *  Make sure that it is possible to access as many texture image units from
 *  a geometry shader as reported by GL_MAX_GEOMETRY_TEXTURE_IMAGE_UNITS_EXT
 *
 *  Category: API;
 *            Functional Test.
 *
 *  Create as many texture objects as reported by
 *  GL_MAX_GEOMETRY_TEXTURE_IMAGE_UNITS_EXT. Each texture should be made an
 *  immutable GL_R32I 2D texture, have 1x1 resolution and be filled with
 *  subsequently increasing intensities (starting from 1, delta: 2). Let's
 *  name these textures "source textures" for the purpose of this test case.
 *
 *  Create a program object and a fragment/geometry/vertex shader object.
 *  Behavior of the shaders should be as follows:
 *
 *  1) Vertex shader should take gl_VertexID and calculate an unique 2D
 *  location, that will later be used to render a 2x2 quad. The calculations
 *  should take quad size into account, note the quads must not overlap. The
 *  result location should be passed to geometry shader by storing it in an
 *  output variable. The shader should also store the vertex id in a
 *  flat-interpolated int output variable called vertex_id.
 *
 *  2) Geometry shader should accept points as input types and should emit
 *  triangle strips with a maximum of 4 output vertices. For each output
 *  geometry's vertex, two values should be written:
 *
 *  * gl_Position obviously;
 *  * color (stored as flat-interpolated integer);
 *
 *  The shader should define as many 2D samplers as reported for the tested
 *  property. In geometry shader's entry-point, the aforementioned 2D
 *  location should be used to calculate vertices of a quad the shader will
 *  emit (built using a triangle strip). Geometry shader should also write
 *  a result of the following computation to an output color variable:
 *
 *  sum(i=0..n_samplers)( (vertex_id == i) * (result of sampling 2D texture
 *  at (0,0) using a sampler bound to texture unit i) );
 *
 *  3) Fragment shader should take the color as passed by geometry shader
 *  and write it to output result variable.
 *
 *  These shader objects should then be compiled, attached to the program
 *  object. The program object should be linked. Each sampler uniform should
 *  be assigned a consecutive texture unit index, starting from 0.
 *
 *  A framebuffer object should then be created, as well as a 2D GL_R32I
 *  texture of resolution:
 *
 *            (GL_MAX_GEOMETRY_TEXTURE_IMAGE_UNITS_EXT*2, 2)
 *
 *  The texture should be attached to the FBO's color attachment.
 *
 *  A vertex array object should be created and bound.
 *
 *  "Source textures" are next bound to corresponding texture units, and the
 *  FBO should be made a draw framebuffer. The program object should be made
 *  current and exactly GL_MAX_GEOMETRY_TEXTURE_IMAGE_UNITS_EXT points should
 *  be drawn.
 *
 *  Next, bind the FBO to GL_READ_FRAMEBUFFER target, read the data, and make
 *  sure that consequent 2x2 quads are of expected intensities (epsilon to
 *  consider: +-1).
 **/
class GeometryShaderMaxTextureUnitsTest : public GeometryShaderLimitsRenderingBase
{
public:
	/* Public methods */
	GeometryShaderMaxTextureUnitsTest(Context& context, const ExtParameters& extParams, const char* name,
									  const char* description);

	virtual ~GeometryShaderMaxTextureUnitsTest()
	{
	}

protected:
	/* Methods overriden from GeometryShaderLimitsRenderingBase */
	virtual void clean();

	virtual void getDrawCallDetails(glw::GLenum& out_primitive_type, glw::GLuint& out_n_vertices);

	virtual void getFramebufferDetails(glw::GLenum& out_texture_format, glw::GLenum& out_texture_read_format,
									   glw::GLenum& out_texture_read_type, glw::GLuint& out_texture_width,
									   glw::GLuint& out_texture_height, glw::GLuint& out_texture_pixel_size);

	virtual void getRequiredPointSize(glw::GLfloat& out_point_size);

	virtual void getShaderParts(const glw::GLchar* const*& out_fragment_shader_parts,
								glw::GLuint&			   out_n_fragment_shader_parts,
								const glw::GLchar* const*& out_geometry_shader_parts,
								glw::GLuint&			   out_n_geometry_shader_parts,
								const glw::GLchar* const*& out_vertex_shader_parts,
								glw::GLuint&			   out_n_vertex_shader_parts);

	virtual void prepareProgramInput();
	virtual bool verifyResult(const void* data);

private:
	/* Private types */
	struct _texture_data
	{
		glw::GLuint texture_id;
		glw::GLint  data;
	};
	typedef std::vector<_texture_data> textureContainer;

	/* Private fields */
	/* Shaders' code */
	static const glw::GLchar* const m_fragment_shader_code;
	static const glw::GLchar* const m_geometry_shader_code_preamble;
	static const glw::GLchar* const m_geometry_shader_code_body;
	static const glw::GLchar* const m_vertex_shader_code_preamble;
	static const glw::GLchar* const m_vertex_shader_code_body;

	/* Storage for vertex and geometry shader parts */
	const glw::GLchar* m_geometry_shader_parts[3];
	const glw::GLchar* m_vertex_shader_parts[3];

	/* Framebuffer dimensions */
	glw::GLuint				 m_texture_width;
	static const glw::GLuint m_texture_height;
	static const glw::GLuint m_texture_pixel_size;
	static const glw::GLuint m_point_size;

	/* Max number of texture units */
	glw::GLint  m_max_texture_units;
	std::string m_max_texture_units_string;

	/* Texture units */
	textureContainer m_textures;
};

/** Implementation of test case 16.8. Test description follows:
 *
 *  Make sure it is possible to use as many geometry shader invocations as
 *  defined by GL_MAX_GEOMETRY_SHADER_INVOCATIONS_EXT. Verify invocation
 *  count defaults to 1 if no number of invocations is defined in the
 *  geometry shader.
 *
 *  Category: API.
 *
 *  Create a program object and:
 *
 *  - A boilerplate vertex shader object;
 *  - A geometry shader object that:
 *
 *  1) takes points on input;
 *  2) outputs a maximum of 3 vertices making up triangles;
 *  3) uses exactly GL_MAX_GEOMETRY_SHADER_INVOCATIONS_EXT invocations;
 *  4) let:
 *
 *              dx = 2.0 / GL_MAX_GEOMETRY_SHADER_INVOCATIONS_EXT.
 *
 *    Emit 3 vertices:
 *
 *    4a) (-1+dx*(gl_InvocationID),  -1.001, 0, 1)
 *    4b) (-1+dx*(gl_InvocationID),   1.001, 0, 1)
 *    4c) (-1+dx*(gl_InvocationID+1), 1.001, 0, 1)
 *
 *  - A fragment shader object that always sets green color for rasterized
 *   fragments.
 *
 *  Compile the shaders, attach them to the program object, link the program
 *  object.
 *
 *  Generate a texture object of type GL_RGBA8 type and of resolution:
 *
 *                     (GL_MAX_GEOMETRY_SHADER_INVOCATIONS_EXT*9)x9
 *
 *  Generate a FBO and attach the texture object to its color attachment.
 *
 *  Bind the FBO to GL_FRAMEBUFFER target and clear the attachments with red
 *  color.
 *
 *  Generate a vertex array object, bind it.
 *
 *  Use the program object and issue a draw call for a single point.
 *
 *  Read back texture object data. The test succeeds if correct amount of
 *  triangles was rendered at expected locations. To test this: :
 *
 *  * Let n = (GL_MAX_GEOMETRY_SHADER_INVOCATIONS_EXT - 1);
 *  * Let (x1, y1) = ((n)     * 9,     0);
 *  * Let (x2, y2) = ((n)     * 9,     0);
 *  * Let (x3, y3) = ((n + 1) * 9 - 1, 9 - 1);
 *  * Triangle rendered in last invocation is described by vertices at
 *   coordinates (x_i, y_i) where i e {1, 2, 3}.
 *  * Centroid of this triangle is defined by (x', y') where:
 *
 *                      x' = floor( (x1 + x2 + x3) / 3);
 *                      y' = floor( (y1 + y2 + y3) / 3);
 *
 *  * Pixel at (x',    y') should be set to (0, 255, 0, 0) (allowed epsilon: 0)
 *  * Pixel at (n*9-1, 9)  should be set to red color      (allowed epsilon: 0)
 *
 *  Repeat this test for a geometry shader with no number of invocations
 *  defined, in which case only one triangle should be rendered.
 **/
class GeometryShaderMaxInvocationsTest : public TestCaseBase
{
public:
	/* Public methods */
	GeometryShaderMaxInvocationsTest(Context& context, const ExtParameters& extParams, const char* name,
									 const char* description);

	virtual ~GeometryShaderMaxInvocationsTest()
	{
	}

	virtual void		  deinit(void);
	virtual IterateResult iterate(void);

private:
	/* Private methods */
	void initTest(void);

	/* Verification of results */
	bool verifyResultOfMultipleInvocationsPass(unsigned char* result_image);
	bool verifyResultOfSingleInvocationPass(unsigned char* result_image);

	/* Private fields */
	/* Program and shader ids for multiple GS invocations */
	glw::GLuint m_fragment_shader_id_for_multiple_invocations_pass;
	glw::GLuint m_geometry_shader_id_for_multiple_invocations_pass;
	glw::GLuint m_program_object_id_for_multiple_invocations_pass;
	glw::GLuint m_vertex_shader_id_for_multiple_invocations_pass;

	/* Program and shader ids for single GS invocation */
	glw::GLuint m_fragment_shader_id_for_single_invocation_pass;
	glw::GLuint m_geometry_shader_id_for_single_invocation_pass;
	glw::GLuint m_program_object_id_for_single_invocation_pass;
	glw::GLuint m_vertex_shader_id_for_single_invocation_pass;

	/* Shaders' code */
	static const glw::GLchar* const m_fragment_shader_code;
	static const glw::GLchar* const m_geometry_shader_code_preamble;
	static const glw::GLchar* const m_geometry_shader_code_layout;
	static const glw::GLchar* const m_geometry_shader_code_layout_invocations;
	static const glw::GLchar* const m_geometry_shader_code_body;
	static const glw::GLchar* const m_vertex_shader_code;

	const glw::GLchar* m_geometry_shader_parts_for_multiple_invocations_pass[16];
	const glw::GLchar* m_geometry_shader_parts_for_single_invocation_pass[16];

	/* Max GS invocations */
	glw::GLint m_max_geometry_shader_invocations;

	/* String used to store maximum number of GS invocations */
	std::string m_max_geometry_shader_invocations_string;

	/* Framebuffer */
	glw::GLuint m_framebuffer_object_id;
	glw::GLuint m_color_texture_id;

	/* Framebuffer dimensions */
	glw::GLuint				 m_texture_width;
	static const glw::GLuint m_triangle_edge_length;
	static const glw::GLuint m_texture_height;
	static const glw::GLuint m_texture_pixel_size;

	/* Vertex array object */
	glw::GLuint m_vertex_array_object_id;
};

/** Implementation of test case 16.9. Test description follows:
 *
 *  Make sure it is possible to use up to GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS
 *  texture image units in three different stages, with an assumption that
 *  each access to the same texture unit from a different stage counts as
 *  a separate texture unit access.
 *
 *  Category: API;
 *           Functional Test.
 *
 *  Create max(GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS,
 *  GL_MAX_GEOMETRY_TEXTURE_IMAGE_UNITS_EXT, GL_MAX_TEXTURE_IMAGE_UNITS)
 *  immutable texture objects. Each texture should use GL_R32UI internal
 *  format, have 1x1 resolution and contain an unique intensity equal to
 *  index of the texture, where first texture object created is considered
 *  to have index equal to 1.
 *
 *  We want each stage to use at least one texture unit. Use the following
 *  calculations to determine how many samplers should be defined for each
 *  stage:
 *
 *  1) Vertex stage: n_vertex_smpl = max(1,
 *    min(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS - 2,
 *        GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS) ).
 *  2) Fragment stage: n_frag_smpl = max(1,
 *    min(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS - n_vertex_smpl - 1,
 *        GL_MAX_TEXTURE_IMAGE_UNITS) )
 *  3) Geometry shader: n_geom_smpl = max(1,
 *    min(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS - n_vertex_smpl - n_frag_smpl,
 *        GL_MAX_GEOMETRY_TEXTURE_IMAGE_UNITS_EXT) )
 *
 *  Create a program object and fragment, geometry and vertex shader objects:
 *
 *  - Vertex shader object should define exactly n_vertex_smpl 2D texture
 *   samplers named samplerX where X stands for texture unit index that
 *   will be accessed. It should set gl_Position to:
 *
 *     (-1 + gl_VertexID / GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, -1, 0, 1)
 *
 *  Vertex shader should define int output variable out_vs_vertexid storing
 *  gl_VertexID value, and an output integer variable out_vs_vertex that
 *  should be written result of the following computation:
 *
 *  sum(i=0..n_vertex_smpl)( (gl_VertexID == i) * (value sampled from the
 *  texture sampler samplerX)) where: X = i;
 *
 *  - Geometry shader object should define exactly n_geom_smpl 2D texture
 *   samplers named samplerX where X stands for texture unit index that
 *   will be accessed. The geometry shader should define int input variables
 *   out_vs_vertexid, out_vs_vertex and int output variables:
 *
 *  * out_gs_vertexid - set to the value of out_vs_vertexid;
 *  * out_gs_vertex   - set to the value of out_vs_vertex;
 *  * out_geometry that should be written result of the following computation:
 *
 *  sum(i=0..n_geom_smpl)( (out_vs_vertexid == i) * (value sampled from the
 *  texture sampler samplerX)) where: X = i;
 *
 *  The geometry shader should emit exactly one point at position configured
 *  by vertex shader. The geometry shader should take points as input.
 *
 *  - Fragment shader object should define exactly n_frag_smpl 2D texture
 *   samplers named samplerX where X stands for texture unit index that
 *   will be accessed. The fragment shader should define int input variables
 *   out_gs_vertexid, out_gs_vertex and out_geometry. It should define
 *   a single int output variable result which should be written result of
 *   the following computation:
 *
 *  if (out_gs_vertex == out_geometry)
 *  {
 *     set to sum(i=0..n_frag_smpl)(out_gs_vertex_id == i) * (value sampled
 *     from the texture sampler samplerX)) where: X = i;
 *  }
 *  else
 *  {
 *     set to 0.
 *  }
 *
 *  The shaders should be attached to the program object and compiled. The
 *  program object should be linked.
 *
 *  Assume:
 *
 *  min_texture_image_units = min(
 *  GL_MAX_TEXTURE_IMAGE_UNITS, GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS,
 *  GL_MAX_GEOMETRY_TEXTURE_IMAGE_UNITS_EXT);
 *
 *  A framebuffer object should be created, along with a 2D texture object of
 *  min_texture_image_units x 1 resolution. The texture object should be
 *  attached to color attachment point of the FBO. Bind the framebuffer object
 *  to GL_DRAW_FRAMEBUFFER target.
 *
 *  A vertex array object should be created and bound.
 *
 *  Configure the program object's uniform samplers to use consecutive texture
 *  image units. Bind the texture objects we created at the beginning to these
 *  texture units. Draw exactly min_texture_image_units points.
 *
 *  Bind the FBO to GL_READ_FRAMEBUFFER. Read the rendered data and make sure
 *  the result values form a (1, 2, ... min_texture_image_units) set.
 **/
class GeometryShaderMaxCombinedTextureUnitsTest : public GeometryShaderLimitsRenderingBase
{
public:
	/* Public methods */
	GeometryShaderMaxCombinedTextureUnitsTest(Context& context, const ExtParameters& extParams, const char* name,
											  const char* description);

	virtual ~GeometryShaderMaxCombinedTextureUnitsTest()
	{
	}

protected:
	/* Methods overriden from GeometryShaderLimitsRenderingBase */
	virtual void clean();

	virtual void getDrawCallDetails(glw::GLenum& out_primitive_type, glw::GLuint& out_n_vertices);

	virtual void getFramebufferDetails(glw::GLenum& out_texture_format, glw::GLenum& out_texture_read_format,
									   glw::GLenum& out_texture_read_type, glw::GLuint& out_texture_width,
									   glw::GLuint& out_texture_height, glw::GLuint& out_texture_pixel_size);

	virtual void getRequiredPointSize(glw::GLfloat& out_point_size);

	virtual void getShaderParts(const glw::GLchar* const*& out_fragment_shader_parts,
								glw::GLuint&			   out_n_fragment_shader_parts,
								const glw::GLchar* const*& out_geometry_shader_parts,
								glw::GLuint&			   out_n_geometry_shader_parts,
								const glw::GLchar* const*& out_vertex_shader_parts,
								glw::GLuint&			   out_n_vertex_shader_parts);

	virtual void prepareProgramInput();
	virtual bool verifyResult(const void* data);

private:
	/* Private types */
	struct _texture_data
	{
		glw::GLuint texture_id;
		glw::GLuint data;
	};
	typedef std::vector<_texture_data> textureContainer;

	/* Private fields */
	/* Shaders' code */
	static const glw::GLchar* const m_fragment_shader_code_preamble;
	static const glw::GLchar* const m_fragment_shader_code_body;
	static const glw::GLchar* const m_geometry_shader_code_preamble;
	static const glw::GLchar* const m_geometry_shader_code_body;
	static const glw::GLchar* const m_vertex_shader_code_preamble;
	static const glw::GLchar* const m_vertex_shader_code_body;

	/* Storage for vertex and geometry shader parts */
	const glw::GLchar* m_fragment_shader_parts[3];
	const glw::GLchar* m_geometry_shader_parts[3];
	const glw::GLchar* m_vertex_shader_parts[3];

	/* Framebuffer dimensions */
	glw::GLuint				 m_texture_width;
	static const glw::GLuint m_texture_height;
	static const glw::GLuint m_texture_pixel_size;
	static const glw::GLuint m_point_size;

	/* Max number of texture units */
	glw::GLint  m_max_combined_texture_units;
	glw::GLint  m_max_fragment_texture_units;
	glw::GLint  m_max_geometry_texture_units;
	glw::GLint  m_max_vertex_texture_units;
	glw::GLint  m_min_texture_units;
	glw::GLint  m_n_fragment_texture_units;
	glw::GLint  m_n_geometry_texture_units;
	glw::GLint  m_n_texture_units;
	glw::GLint  m_n_vertex_texture_units;
	std::string m_n_fragment_texture_units_string;
	std::string m_n_geometry_texture_units_string;
	std::string m_n_vertex_texture_units_string;

	/* Texture units */
	textureContainer m_textures;
};

} /* glcts */

#endif // _ESEXTCGEOMETRYSHADERLIMITS_HPP
