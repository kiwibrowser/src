#ifndef _ESEXTCGEOMETRYSHADERINPUT_HPP
#define _ESEXTCGEOMETRYSHADERINPUT_HPP
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

#include "../esextcTestCaseBase.hpp"
#include <deque>

namespace glcts
{
/** Implementation of test case 5.1. Test description follows:
 *
 *  Make sure that all output variables of a vertex shader can be accessed
 *  under geometry shader's input array variables.
 *
 *  Category: API;
 *            Functional Test.
 *
 *  Vertex shader should define:
 *
 *  - vec2 output variable named vs_gs_a;
 *  - flat ivec4 output variable named vs_gs_b;
 *
 *  and set them to meaningful vector values (based on gl_VertexID).
 *
 *  Geometry shader should take triangles on input, output triangles (3
 *  vertices will be emitted) and also define:
 *
 *  - vec2 output variable named gs_fs_a (and set it to vs_gs_a[i]);
 *  - flat ivec4 output variable named gs_fs_b (and set it to vs_gs_b[i]);
 *
 *  where i is index of vertex.
 *
 *  gl_Position should be set as follows:
 *
 *  1st vertex) (-1, -1, 0, 1)
 *  2nd vertex) (-1, 1, 0, 1)
 *  3rd vertex) (1, 1, 0, 1)
 *
 *  Vertex attribute arrays should be configured so that vertex shader passes
 *  meaningful values to the geometry shader.
 *
 *  Use Transform Feedback to check if geometry shader was passed the right
 *  values. Draw a single triangle. The test passes if the captured data
 *  matches the expected values.
 **/
class GeometryShader_gl_in_ArrayContentsTest : public TestCaseBase
{
public:
	/* Public methods */
	GeometryShader_gl_in_ArrayContentsTest(Context& context, const ExtParameters& extParams, const char* name,
										   const char* description);

	virtual ~GeometryShader_gl_in_ArrayContentsTest()
	{
	}

	virtual void		  deinit(void);
	virtual IterateResult iterate(void);

private:
	/* Private methods */
	void initTest(void);

	/* Private fields */
	/* Program and shader ids */
	glw::GLuint m_fragment_shader_id;
	glw::GLuint m_geometry_shader_sized_arrays_id;
	glw::GLuint m_geometry_shader_unsized_arrays_id;
	glw::GLuint m_program_object_sized_arrays_id;
	glw::GLuint m_program_object_unsized_arrays_id;
	glw::GLuint m_vertex_shader_id;

	/* Shaders' code */
	static const glw::GLchar* const m_fragment_shader_code;
	static const glw::GLchar* const m_geometry_shader_code;
	static const glw::GLchar* const m_geometry_shader_preamble_code;
	static const glw::GLchar* const m_vertex_shader_code;

	/* Buffer Object used to store output from transform feedback */
	glw::GLuint m_buffer_object_id;

	/* Constants used to calculate and store size of buffer object, that will be used as transform feedback output */
	static const unsigned int m_buffer_size;
	static const unsigned int m_n_bytes_emitted_per_vertex;
	static const unsigned int m_n_emitted_primitives;
	static const unsigned int m_n_vertices_emitted_per_primitive;

	/* Vertex array object ID */
	glw::GLuint m_vertex_array_object_id;
};

/** Implementation of test case 5.2. Test description follows:
 *
 *  Make sure that length of gl_in array is correct for all input primitive
 *  types accepted by a geometry shader.
 *
 *  Category: API;
 *            Functional Test.
 *
 *  Consider a set of geometry shaders, where each geometry shader takes an
 *  unique input primitive type, and the set - as a whole - covers all input
 *  primitive types accepted by geometry shaders. Each geometry shader should
 *  output a maximum of 1 point. They should define an output int variable
 *  called in_array_size and set it to gl_in.length().
 *
 *  Using transform feed-back, the test should check whether all geometry
 *  shaders are reported correct gl_in array size for all acceptable input
 *  primitive types.
 **/
class GeometryShader_gl_in_ArrayLengthTest : public TestCaseBase
{
public:
	/* Public methods */
	GeometryShader_gl_in_ArrayLengthTest(Context& context, const ExtParameters& extParams, const char* name,
										 const char* description);

	virtual ~GeometryShader_gl_in_ArrayLengthTest()
	{
	}

	virtual void		  deinit(void);
	virtual void		  init(void);
	virtual IterateResult iterate(void);

private:
	/* Private types */
	struct Case
	{
		/* Mode to be used for glDrawArrays() call */
		glw::GLenum draw_call_mode;
		/* Amount of vertices to be requested for glDrawArrays() call */
		glw::GLint draw_call_n_vertices;
		/* Transform feed-back mode to use before doing glDrawArrays() call */
		glw::GLenum tf_mode;

		/* ID of a fragment shader to be used for the test case */
		glw::GLint fs_id;
		/* ID of a geometry shader to be used for the test case */
		glw::GLint gs_id;
		/* String defining input layout qualifier for the test case. */
		const glw::GLchar* input_body_part;
		/* String defining output layout qualifier for the test case. */
		const glw::GLchar* output_body_part;
		/** ID of a program object to be used for the test case */
		glw::GLint po_id;
		/** ID of a vertex shader to be used for the test case */
		glw::GLint vs_id;

		/** Expected gl_in.length() value for the test case */
		glw::GLint expected_array_length;
	};

	/* Type of container used to group all test cases */
	typedef std::deque<Case*> testContainer;

	/* Private methods */
	void deinitCase(Case& info);

	void initCase(Case& info, glw::GLenum draw_call_mode, glw::GLint draw_call_n_vertices,
				  glw::GLint expected_array_length, glw::GLenum tf_mode, const glw::GLchar* input_body_part,
				  const glw::GLchar* output_body_part);

	void initCaseProgram(Case& info, const glw::GLchar** captured_varyings, glw::GLuint n_captured_varyings_size);

	void resetCase(Case& info);

	/* Private fields */
	/* Shaders */
	static const glw::GLchar* const m_vertex_shader_code;

	static const glw::GLchar* const m_geometry_shader_code_input_points;
	static const glw::GLchar* const m_geometry_shader_code_input_lines;
	static const glw::GLchar* const m_geometry_shader_code_input_lines_with_adjacency;
	static const glw::GLchar* const m_geometry_shader_code_input_triangles;
	static const glw::GLchar* const m_geometry_shader_code_input_triangles_with_adjacency;
	static const glw::GLchar* const m_geometry_shader_code_main;
	static const glw::GLchar* const m_geometry_shader_code_output_line_strip;
	static const glw::GLchar* const m_geometry_shader_code_output_points;
	static const glw::GLchar* const m_geometry_shader_code_output_triangle_strip;
	static const glw::GLchar* const m_geometry_shader_code_preamble;

	static const glw::GLchar* const m_fragment_shader_code;

	/* Test cases */
	Case m_test_lines;
	Case m_test_lines_adjacency;
	Case m_test_lines_adjacency_to_line_strip;
	Case m_test_points;
	Case m_test_triangles;
	Case m_test_triangles_adjacency;
	Case m_test_triangles_adjacency_to_triangle_strip;

	/* Set of test cases */
	testContainer m_tests;

	/* Buffer Object used to store output from transform feedback */
	glw::GLuint m_buffer_object_id;

	/* Constants used to calculate and store size of buffer object, that will be used as transform feedback output */
	static const glw::GLuint m_buffer_size;
	static const glw::GLuint m_max_primitive_emitted;

	/* Vertex array object ID */
	glw::GLuint m_vertex_array_object_id;
};

/** Implementation of test case 5.3. Test description follows:
 *
 *  Make sure geometry shader is passed gl_PointSize value as set by vertex
 *  shader.
 *
 *  Category: API;
 *            Functional Test.
 *
 *  NOTE: This test should be skipped if tested GLES implementation does not
 *        support points of sufficiently large sizes.
 *
 *  Vertex shader should set gl_PointSize variable to 2*(gl_VertexID+1).
 *
 *  Geometry shader should take points on input and emit 1 point at most. It
 *  should set gl_PointSize to 2*gl_in[0].gl_PointSize. The test will draw
 *  two points, so to have these two points centered on the left and the right
 *  side of the screen-space (assuming result draw buffer is 16px wide), set
 *  gl_Position to:
 *
 *  1) (-1 + (4*(1/16))/2, 0, 0, 1) for the first point, given its point size
 *    equal to 4;
 *  2) ( 1 - (8*(1/16))/2, 0, 0, 1) for the second point, with its point size
 *    set to 8;
 *
 *  Fragment shader should set the output variable to (1, 1, 1, 1), the
 *  default clear color should be set to (0, 0, 0, 0).
 *
 *  Color attachment should have a resolution of 16px x 16px.
 *
 *  The test should clear the color buffer and draw two points. The test
 *  passes if:
 *
 *  1) pixel at (2,  8) is (255, 255, 255, 255)
 *  2) pixel at (14, 8) is (255, 255, 255, 255)
 *  3) pixel at (6,  8) is (0,   0,   0,   0)
 *
 *  Assumption: glReadPixels() is called for GL_RGBA format and
 *              GL_UNSIGNED_BYTE type.
 **/
class GeometryShader_gl_PointSize_ValueTest : public TestCaseBase
{
public:
	/* Public methods */
	GeometryShader_gl_PointSize_ValueTest(Context& context, const ExtParameters& extParams, const char* name,
										  const char* description);

	virtual ~GeometryShader_gl_PointSize_ValueTest()
	{
	}

	virtual void		  deinit(void);
	virtual void		  init(void);
	virtual IterateResult iterate(void);

private:
	/* Private fields */
	/* Program and shader ids */
	glw::GLuint m_fragment_shader_id;
	glw::GLuint m_geometry_shader_id;
	glw::GLuint m_program_object_id;
	glw::GLuint m_vertex_shader_id;

	/* Shaders' code */
	static const glw::GLchar* const m_fragment_shader_code;
	static const glw::GLchar* const m_geometry_shader_code;
	static const glw::GLchar* const m_vertex_shader_code;

	/* Vertex array object ID */
	glw::GLuint m_vertex_array_object_id;

	/* Texture object ID */
	glw::GLuint m_color_texture_id;

	/* Framebuffer object ID */
	glw::GLuint m_framebuffer_object_id;

	/* Constants used to store properties of framebuffer's color attachment */
	static const glw::GLuint m_texture_height;
	static const glw::GLuint m_texture_pixel_size;
	static const glw::GLuint m_texture_width;
};

/** Implementation of test case 5.4. Test description follows:
 *
 *  Make sure geometry shader is passed gl_Position value as set by vertex shader.
 *
 *  Category: API;
 *            Functional Test.
 *
 *  Vertex shader should set gl_Position variable to
 *  (gl_VertexID, gl_VertexID, 0, 1).
 *
 *  Geometry shader should take points on input and emit 1 point at most. It
 *  should set gl_PointSize to 8. The test will draw eight points. The
 *  geometry shader should set gl_Position to:
 *
 *           (-1 + 4/32 + gl_in[0].gl_Position / 4, 0, 0, 1)
 *
 *  Fragment shader should set the output variable to (1, 1, 1, 1), the
 *  default clear color should be set to (0, 0, 0, 0).
 *
 *  Color attachment should have a resolution of 64px x 64px.
 *
 *  The test should clear the color buffer and draw eight points. The test
 *  passes if centers of the rendered points at expected locations are lit.
 **/
class GeometryShader_gl_Position_ValueTest : public TestCaseBase
{
public:
	/* Public methods */
	GeometryShader_gl_Position_ValueTest(Context& context, const ExtParameters& extParams, const char* name,
										 const char* description);

	virtual ~GeometryShader_gl_Position_ValueTest()
	{
	}

	virtual void		  deinit(void);
	virtual void		  init(void);
	virtual IterateResult iterate(void);

private:
	/* Private fields */
	/* Program and shader ids */
	glw::GLuint m_fragment_shader_id;
	glw::GLuint m_geometry_shader_id;
	glw::GLuint m_program_object_id;
	glw::GLuint m_vertex_shader_id;

	/* Shaders' code */
	static const glw::GLchar* const m_fragment_shader_code;
	static const glw::GLchar* const m_geometry_shader_code;
	static const glw::GLchar* const m_vertex_shader_code;

	/* Vertex array object ID */
	glw::GLuint m_vertex_array_object_id;

	/* Texture object ID */
	glw::GLuint m_color_texture_id;

	/* Framebuffer object ID */
	glw::GLuint m_framebuffer_object_id;

	/* Constants used to store properties of framebuffer's color attachment */
	static const glw::GLuint m_texture_height;
	static const glw::GLuint m_texture_pixel_size;
	static const glw::GLuint m_texture_width;
};

} /* glcts */

#endif // _ESEXTCGEOMETRYSHADERINPUT_HPP
