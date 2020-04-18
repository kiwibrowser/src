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

#include "esextcGeometryShaderInput.hpp"

#include "gluContextInfo.hpp"
#include "glwEnums.hpp"
#include "glwFunctions.hpp"
#include "tcuTestLog.hpp"
#include <cstring>

namespace glcts
{
/* Vertex shader for GeometryShader_gl_in_ArrayContents */
const char* const GeometryShader_gl_in_ArrayContentsTest::m_vertex_shader_code =
	"${VERSION}\n"
	"\n"
	"     out  vec2 vs_gs_a;\n"
	"flat out ivec4 vs_gs_b;\n"
	"\n"
	"void main()\n"
	"{\n"
	"    vs_gs_a = vec2 (gl_VertexID, 0);\n"
	"    vs_gs_b = ivec4(0,           gl_VertexID, 0, 1);\n"
	"}\n";

/* Geometry shader for GeometryShader_gl_in_ArrayContents */
const char* const GeometryShader_gl_in_ArrayContentsTest::m_geometry_shader_preamble_code =
	"${VERSION}\n"
	"\n"
	"${GEOMETRY_SHADER_REQUIRE}\n"
	"\n"
	"layout(triangles)                      in;\n"
	"layout(triangle_strip, max_vertices=3) out;\n"
	"\n";

const char* const GeometryShader_gl_in_ArrayContentsTest::m_geometry_shader_code =
	"#ifdef USE_UNSIZED_ARRAYS\n"
	"         in  vec2 vs_gs_a[];\n"
	"    flat in ivec4 vs_gs_b[];\n"
	"#else\n"
	"         in  vec2 vs_gs_a[3];\n"
	"    flat in ivec4 vs_gs_b[3];\n"
	"#endif\n"
	"\n"
	"     out  vec2 gs_fs_a;\n"
	"flat out ivec4 gs_fs_b;\n"
	"\n"
	"void main()\n"
	"{\n"
	"    gl_Position = vec4(-1, -1, 0, 1);\n"
	"    gs_fs_a     = vs_gs_a[0];\n"
	"    gs_fs_b     = vs_gs_b[0];\n"
	"    EmitVertex();\n"
	"    \n"
	"    gl_Position = vec4(-1, 1, 0, 1);\n"
	"    gs_fs_a     = vs_gs_a[1];\n"
	"    gs_fs_b     = vs_gs_b[1];\n"
	"    EmitVertex();\n"
	"    \n"
	"    gl_Position = vec4(1, 1, 0, 1);\n"
	"    gs_fs_a     = vs_gs_a[2];\n"
	"    gs_fs_b     = vs_gs_b[2];\n"
	"    EmitVertex();\n"
	"    \n"
	"    EndPrimitive();\n"
	"}\n";

/* Fragment shader for GeometryShader_gl_in_ArrayContents */
const char* const GeometryShader_gl_in_ArrayContentsTest::m_fragment_shader_code =
	"${VERSION}\n"
	"\n"
	"precision highp float;\n"
	"\n"
	"layout(location = 0) out vec4 fs_out_color;\n"
	"\n"
	"void main()\n"
	"{\n"
	"    fs_out_color = vec4(1, 1, 1, 1);\n"
	"}\n";

/* Vertex Shader for GeometryShader_gl_in_ArrayLengthTest*/
const char* const GeometryShader_gl_in_ArrayLengthTest::m_vertex_shader_code =
	"${VERSION}\n"
	"\n"
	"void main()\n"
	"{\n"
	"    gl_Position = vec4(gl_VertexID, 0, 0, 1);\n"
	"}\n";

/* Geometry shader body parts for GeometryShader_gl_in_ArrayLengthTest */
const char* const GeometryShader_gl_in_ArrayLengthTest::m_geometry_shader_code_preamble = "${VERSION}\n"
																						  "\n"
																						  "${GEOMETRY_SHADER_REQUIRE}\n"
																						  "\n";

const char* const GeometryShader_gl_in_ArrayLengthTest::m_geometry_shader_code_input_points =
	"layout(points)                     in;\n";

const char* const GeometryShader_gl_in_ArrayLengthTest::m_geometry_shader_code_input_lines =
	"layout(lines)                      in;\n";

const char* const GeometryShader_gl_in_ArrayLengthTest::m_geometry_shader_code_input_lines_with_adjacency =
	"layout(lines_adjacency)            in;\n";

const char* const GeometryShader_gl_in_ArrayLengthTest::m_geometry_shader_code_input_triangles =
	"layout(triangles)                  in;\n";

const char* const GeometryShader_gl_in_ArrayLengthTest::m_geometry_shader_code_input_triangles_with_adjacency =
	"layout(triangles_adjacency)        in;\n";

const char* const GeometryShader_gl_in_ArrayLengthTest::m_geometry_shader_code_output_points =
	"layout(points, max_vertices=1)     out;\n"
	"\n"
	"#define N_OUT_VERTICES (1)\n";

const char* const GeometryShader_gl_in_ArrayLengthTest::m_geometry_shader_code_output_line_strip =
	"layout(line_strip, max_vertices=2) out;\n"
	"\n"
	"#define N_OUT_VERTICES (2)\n";

const char* const GeometryShader_gl_in_ArrayLengthTest::m_geometry_shader_code_output_triangle_strip =
	"layout(triangle_strip, max_vertices=3) out;\n"
	"\n"
	"#define N_OUT_VERTICES (3)\n";

const char* const GeometryShader_gl_in_ArrayLengthTest::m_geometry_shader_code_main =
	"\n"
	"flat out int in_array_size;\n"
	"\n"
	"void main()\n"
	"{\n"
	"    for (int n = 0; n < N_OUT_VERTICES; n++)\n"
	"    {\n"
	"        in_array_size = gl_in.length();\n"
	"        EmitVertex();\n"
	"    }\n"
	"\n"
	"    EndPrimitive();\n"
	"}\n";

/* Fragment Shader for GeometryShader_gl_in_ArrayLengthTest */
const char* const GeometryShader_gl_in_ArrayLengthTest::m_fragment_shader_code = "${VERSION}\n"
																				 "\n"
																				 "precision highp float;\n"
																				 "\n"
																				 "void main()\n"
																				 "{\n"
																				 "}\n";

/* Vertex Shader for GeometryShader_gl_PointSize_ValueTest */
const char* const GeometryShader_gl_PointSize_ValueTest::m_vertex_shader_code =
	"${VERSION}\n"
	"\n"
	"void main()\n"
	"{\n"
	"    // See test description for explanation of magic numbers\n"
	"    switch (gl_VertexID)\n"
	"    {\n"
	"        case 0:\n"
	"        {\n"
	"            gl_Position = vec4(-7.0/8.0, 0, 0, 1);\n"
	"\n"
	"            break;\n"
	"        }\n"
	"\n"
	"        case 1:\n"
	"        {\n"
	"            gl_Position = vec4(6.0/8.0, 0, 0, 1);\n"
	"\n"
	"            break;\n"
	"        }\n"
	"    }\n"
	"\n"
	"    gl_PointSize = float(2 * (gl_VertexID + 1));\n"
	"}\n";

/* Geometry Shader for GeometryShader_gl_PointSize_ValueTest */
const char* const GeometryShader_gl_PointSize_ValueTest::m_geometry_shader_code =
	"${VERSION}\n"
	"\n"
	"${GEOMETRY_SHADER_REQUIRE}\n"
	"${GEOMETRY_POINT_SIZE_REQUIRE}\n"
	"\n"
	"layout(points)                 in;\n"
	"layout(points, max_vertices=1) out;\n"
	"\n"
	"void main()\n"
	"{\n"
	"    gl_Position  = gl_in[0].gl_Position;\n"
	"    gl_PointSize = gl_in[0].gl_PointSize * 2.0;\n"
	"    EmitVertex();\n"
	"    \n"
	"    EndPrimitive();\n"
	"}\n";

/* Fragment Shader for GeometryShader_gl_PointSize_ValueTest */
const char* const GeometryShader_gl_PointSize_ValueTest::m_fragment_shader_code =
	"${VERSION}\n"
	"\n"
	"precision highp float;\n"
	"\n"
	"layout(location = 0) out vec4 fs_out_color;\n"
	"\n"
	"void main()\n"
	"{\n"
	"    fs_out_color = vec4(1, 1, 1, 1);\n"
	"}\n";

/* Vertex Shader for GeometryShader_gl_Position_ValueTest */
const char* const GeometryShader_gl_Position_ValueTest::m_vertex_shader_code =
	"${VERSION}\n"
	"\n"
	"void main()\n"
	"{\n"
	"    gl_Position = vec4(gl_VertexID, gl_VertexID, 0, 1);\n"
	"\n"
	"}\n";

/* Geometry Shader for GeometryShader_gl_Position_ValueTest */
const char* const GeometryShader_gl_Position_ValueTest::m_geometry_shader_code =
	"${VERSION}\n"
	"\n"
	"${GEOMETRY_SHADER_REQUIRE}\n"
	"${GEOMETRY_POINT_SIZE_REQUIRE}\n"
	"\n"
	"layout(points)                 in;\n"
	"layout(points, max_vertices=1) out;\n"
	"\n"
	"void main()\n"
	"{\n"
	"    // See test description for discussion on the magic numbers\n"
	"    gl_Position  = vec4(-1.0 + 4.0/32.0 + gl_in[0].gl_Position.x / 4.0, 0, 0, 1);\n"
	"    gl_PointSize = 8.0;\n"
	"    EmitVertex();\n"
	"\n"
	"    EndPrimitive();\n"
	"}\n";

/* Fragment Shader for GeometryShader_gl_Position_ValueTest */
const char* const GeometryShader_gl_Position_ValueTest::m_fragment_shader_code =
	"${VERSION}\n"
	"\n"
	"precision highp float;\n"
	"\n"
	"layout(location = 0) out vec4 fs_out_color;\n"
	"\n"
	"void main()\n"
	"{\n"
	"    fs_out_color = vec4(1, 1, 1, 1);\n"
	"}\n";

/* Constants for GeometryShader_gl_in_ArrayContentsTest */
const unsigned int GeometryShader_gl_in_ArrayContentsTest::m_n_bytes_emitted_per_vertex =
	2 * sizeof(glw::GLfloat) + 4 * sizeof(glw::GLint);
const unsigned int GeometryShader_gl_in_ArrayContentsTest::m_n_emitted_primitives			  = 1;
const unsigned int GeometryShader_gl_in_ArrayContentsTest::m_n_vertices_emitted_per_primitive = 3;

const unsigned int GeometryShader_gl_in_ArrayContentsTest::m_buffer_size =
	GeometryShader_gl_in_ArrayContentsTest::m_n_bytes_emitted_per_vertex *
	GeometryShader_gl_in_ArrayContentsTest::m_n_vertices_emitted_per_primitive *
	GeometryShader_gl_in_ArrayContentsTest::m_n_emitted_primitives;

/* Constants for GeometryShader_gl_in_ArrayLengthTest */
const glw::GLuint GeometryShader_gl_in_ArrayLengthTest::m_max_primitive_emitted = 6;
const glw::GLuint GeometryShader_gl_in_ArrayLengthTest::m_buffer_size = sizeof(glw::GLint) * m_max_primitive_emitted;

/* Constants for GeometryShader_gl_PointSize_ValueTest */
const glw::GLuint GeometryShader_gl_PointSize_ValueTest::m_texture_height	 = 16;
const glw::GLuint GeometryShader_gl_PointSize_ValueTest::m_texture_pixel_size = 4;
const glw::GLuint GeometryShader_gl_PointSize_ValueTest::m_texture_width	  = 16;

/* Constants for GeometryShader_gl_Position_ValueTest */
const glw::GLuint GeometryShader_gl_Position_ValueTest::m_texture_height	 = 64;
const glw::GLuint GeometryShader_gl_Position_ValueTest::m_texture_pixel_size = 4;
const glw::GLuint GeometryShader_gl_Position_ValueTest::m_texture_width		 = 64;

/** Constructor
 *
 * @param context       Test context
 * @param name          Test case's name
 * @param description   Test case's desricption
 **/
GeometryShader_gl_in_ArrayContentsTest::GeometryShader_gl_in_ArrayContentsTest(Context&				context,
																			   const ExtParameters& extParams,
																			   const char*			name,
																			   const char*			description)
	: TestCaseBase(context, extParams, name, description)
	, m_fragment_shader_id(0)
	, m_geometry_shader_sized_arrays_id(0)
	, m_geometry_shader_unsized_arrays_id(0)
	, m_program_object_sized_arrays_id(0)
	, m_program_object_unsized_arrays_id(0)
	, m_vertex_shader_id(0)
	, m_buffer_object_id(0)
	, m_vertex_array_object_id(0)
{
	/* Nothing to be done here */
}

/** Initializes GLES objects used during the test.
 *
 **/
void GeometryShader_gl_in_ArrayContentsTest::initTest()
{
	/* Varing names */
	const glw::GLchar* const captured_varyings[] = {
		"gs_fs_a", "gs_fs_b",
	};

	/* Number of varings */
	const glw::GLuint n_captured_varyings_size = sizeof(captured_varyings) / sizeof(captured_varyings[0]);

	/* GL */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Create program and shaders */
	m_program_object_sized_arrays_id   = gl.createProgram();
	m_program_object_unsized_arrays_id = gl.createProgram();

	m_fragment_shader_id				= gl.createShader(GL_FRAGMENT_SHADER);
	m_geometry_shader_unsized_arrays_id = gl.createShader(m_glExtTokens.GEOMETRY_SHADER);
	m_geometry_shader_sized_arrays_id   = gl.createShader(m_glExtTokens.GEOMETRY_SHADER);
	m_vertex_shader_id					= gl.createShader(GL_VERTEX_SHADER);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Failed to create program");

	/* Set up transform feedback */
	gl.transformFeedbackVaryings(m_program_object_sized_arrays_id, n_captured_varyings_size, captured_varyings,
								 GL_INTERLEAVED_ATTRIBS);
	gl.transformFeedbackVaryings(m_program_object_unsized_arrays_id, n_captured_varyings_size, captured_varyings,
								 GL_INTERLEAVED_ATTRIBS);

	/* Build programs */
	const char* geometry_shader_unsized_arrays_code[] = { m_geometry_shader_preamble_code,
														  "#define USE_UNSIZED_ARRAYS\n", m_geometry_shader_code };
	const char* geometry_shader_sized_arrays_code[] = { m_geometry_shader_preamble_code, m_geometry_shader_code };

	if (false ==
		buildProgram(m_program_object_unsized_arrays_id, m_fragment_shader_id, 1 /* number of fragment shader parts */,
					 &m_fragment_shader_code, m_geometry_shader_unsized_arrays_id,
					 DE_LENGTH_OF_ARRAY(geometry_shader_unsized_arrays_code), geometry_shader_unsized_arrays_code,
					 m_vertex_shader_id, 1 /* number of vertex shader parts */, &m_vertex_shader_code))
	{
		TCU_FAIL("Could not create a program from valid vertex/geometry (unsized arrays version)/fragment shaders");
	}

	if (false == buildProgram(m_program_object_sized_arrays_id, m_fragment_shader_id,
							  1 /* number of fragment shader parts */, &m_fragment_shader_code,
							  m_geometry_shader_sized_arrays_id, DE_LENGTH_OF_ARRAY(geometry_shader_sized_arrays_code),
							  geometry_shader_sized_arrays_code, m_vertex_shader_id,
							  1 /* number of vertex shader parts */, &m_vertex_shader_code))
	{
		TCU_FAIL("Could not create a program from valid vertex/geometry (sized arrays version)/fragment shaders");
	}

	/* Generate, bind and allocate buffer */
	gl.genBuffers(1, &m_buffer_object_id);
	gl.bindBuffer(GL_ARRAY_BUFFER, m_buffer_object_id);
	gl.bufferData(GL_ARRAY_BUFFER, m_buffer_size, 0 /* no start data */, GL_STATIC_DRAW);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not create buffer object");

	/* Generate and bind VAO */
	gl.genVertexArrays(1, &m_vertex_array_object_id);
	gl.bindVertexArray(m_vertex_array_object_id);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not create vertex array object");
}

/** Executes the test.
 *  Sets the test result to QP_TEST_RESULT_FAIL if the test failed, QP_TEST_RESULT_PASS otherwise.
 *  @return STOP if the test has finished, CONTINUE to indicate iterate should be called once again.
 *  Note the function throws exception should an error occur!
 **/
tcu::TestCase::IterateResult GeometryShader_gl_in_ArrayContentsTest::iterate()
{
	/* This test should only run if EXT_geometry_shader is supported */
	if (true != m_is_geometry_shader_extension_supported)
	{
		throw tcu::NotSupportedError(GEOMETRY_SHADER_EXTENSION_NOT_SUPPORTED, "", __FILE__, __LINE__);
	}

	initTest();

	const glw::Functions& gl							= m_context.getRenderContext().getFunctions();
	unsigned char		  reference_data[m_buffer_size] = { 0 };
	bool				  result						= true;

	/* Prepare reference data */
	{
		glw::GLint*   ivec4_data_ptr;
		glw::GLfloat* vec2_data_ptr;

		const unsigned int ivec4_offset_from_vertex = 2 * sizeof(glw::GLfloat);
		const unsigned int vec2_offset_from_vertex  = 0;

		/* Expected data for vertex:
		 * vec2  = {VertexID, 0.0f}
		 * ivec4 = {0,        VertexID, 0, 1}
		 */
		for (unsigned int vertex_id = 0; vertex_id < m_n_vertices_emitted_per_primitive * m_n_emitted_primitives;
			 ++vertex_id)
		{
			const unsigned int vertex_offset = vertex_id * m_n_bytes_emitted_per_vertex;
			const unsigned int ivec4_offset  = vertex_offset + ivec4_offset_from_vertex;
			const unsigned int vec2_offset   = vertex_offset + vec2_offset_from_vertex;

			ivec4_data_ptr = (glw::GLint*)(reference_data + ivec4_offset);
			vec2_data_ptr  = (glw::GLfloat*)(reference_data + vec2_offset);

			ivec4_data_ptr[0] = 0;
			ivec4_data_ptr[1] = vertex_id;
			ivec4_data_ptr[2] = 0;
			ivec4_data_ptr[3] = 1;

			vec2_data_ptr[0] = (float)vertex_id;
			vec2_data_ptr[1] = 0.0f;
		}
	}

	/* Setup transform feedback */
	gl.enable(GL_RASTERIZER_DISCARD);
	gl.bindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, m_buffer_object_id);

	/* Draw the geometry */
	for (int n_case = 0; n_case < 2 /* unsized/sized array cases */; ++n_case)
	{
		glw::GLuint po_id = (n_case == 0) ? m_program_object_unsized_arrays_id : m_program_object_sized_arrays_id;

		gl.useProgram(po_id);
		gl.beginTransformFeedback(GL_TRIANGLES);
		{
			gl.drawArrays(GL_TRIANGLES, 0 /* first */, 3 /* one triangle */);
		}
		gl.endTransformFeedback();

		GLU_EXPECT_NO_ERROR(gl.getError(), "Error doing a draw call");

		/* Map buffer object storage holding XFB result into process space. */
		glw::GLchar* transform_feedback_data = (glw::GLchar*)gl.mapBufferRange(
			GL_TRANSFORM_FEEDBACK_BUFFER, 0 /* offset */, m_buffer_size, GL_MAP_READ_BIT);

		GLU_EXPECT_NO_ERROR(gl.getError(), "Could not map the buffer object into process space");

		/* Verify data extracted from transform feedback */
		if (0 != memcmp(transform_feedback_data, reference_data, m_buffer_size))
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "Data extracted from transform feedback is invalid."
							   << tcu::TestLog::EndMessage;

			result = false;
		}

		/* Unmap the buffer object. */
		gl.unmapBuffer(GL_TRANSFORM_FEEDBACK_BUFFER);

		GLU_EXPECT_NO_ERROR(gl.getError(), "Error unmapping the buffer object");

		/* Verify results */
		if (true != result)
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
			return STOP;
		}
	}

	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	return STOP;
}

/** Deinitializes GLES objects created during the test.
 *
 */
void GeometryShader_gl_in_ArrayContentsTest::deinit()
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
	}

	if (0 != m_buffer_object_id)
	{
		gl.deleteBuffers(1, &m_buffer_object_id);
	}

	if (0 != m_program_object_sized_arrays_id)
	{
		gl.deleteProgram(m_program_object_sized_arrays_id);
	}

	if (0 != m_program_object_unsized_arrays_id)
	{
		gl.deleteProgram(m_program_object_unsized_arrays_id);
	}

	if (0 != m_fragment_shader_id)
	{
		gl.deleteShader(m_fragment_shader_id);
	}

	if (0 != m_geometry_shader_sized_arrays_id)
	{
		gl.deleteShader(m_geometry_shader_sized_arrays_id);
	}

	if (0 != m_geometry_shader_unsized_arrays_id)
	{
		gl.deleteShader(m_geometry_shader_unsized_arrays_id);
	}

	if (0 != m_vertex_shader_id)
	{
		gl.deleteShader(m_vertex_shader_id);
	}

	/* Deinitialize Base */
	TestCaseBase::deinit();
}

/** Constructor
 *
 * @param context       Test context
 * @param name          Test case's name
 * @param description   Test case's desricption
 **/
GeometryShader_gl_in_ArrayLengthTest::GeometryShader_gl_in_ArrayLengthTest(Context&				context,
																		   const ExtParameters& extParams,
																		   const char* name, const char* description)
	: TestCaseBase(context, extParams, name, description), m_buffer_object_id(0), m_vertex_array_object_id(0)
{
	/* Nothing to be done here */
}

/** Initialize test case
 *
 **/
void GeometryShader_gl_in_ArrayLengthTest::init()
{
	/* Initialize Base */
	TestCaseBase::init();

	/* Captured variables */
	const char* captured_varyings[] = { "in_array_size" };

	/* This test should only run if EXT_geometry_shader is supported */
	if (true != m_is_geometry_shader_extension_supported)
	{
		throw tcu::NotSupportedError(GEOMETRY_SHADER_EXTENSION_NOT_SUPPORTED, "", __FILE__, __LINE__);
	}

	/* GL */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Set up test descriptors */
	initCase(m_test_lines, GL_LINES, 2, /* number of vertices */
			 2,							/* as per spec */
			 GL_POINTS, m_geometry_shader_code_input_lines, m_geometry_shader_code_output_points);

	m_tests.push_back(&m_test_lines);

	initCase(m_test_lines_adjacency, m_glExtTokens.LINES_ADJACENCY, 4, /* number of vertices */
			 4,														   /* as per spec */
			 GL_POINTS, m_geometry_shader_code_input_lines_with_adjacency, m_geometry_shader_code_output_points);

	m_tests.push_back(&m_test_lines_adjacency);

	initCase(m_test_points, GL_POINTS, 1, /* number of vertices */
			 1,							  /* as per spec */
			 GL_POINTS, m_geometry_shader_code_input_points, m_geometry_shader_code_output_points);

	m_tests.push_back(&m_test_points);

	initCase(m_test_triangles, GL_TRIANGLES, 3, /* number of vertices */
			 3,									/* as per spec */
			 GL_POINTS, m_geometry_shader_code_input_triangles, m_geometry_shader_code_output_points);

	m_tests.push_back(&m_test_triangles);

	initCase(m_test_triangles_adjacency, m_glExtTokens.TRIANGLE_STRIP_ADJACENCY, 6, /* number of vertices */
			 6,																		/* as per spec */
			 GL_POINTS, m_geometry_shader_code_input_triangles_with_adjacency, m_geometry_shader_code_output_points);

	m_tests.push_back(&m_test_triangles_adjacency);

	initCase(m_test_lines_adjacency_to_line_strip, m_glExtTokens.LINES_ADJACENCY, 4 /* number of vertices */,
			 4 /* expected array length */, GL_LINES, m_geometry_shader_code_input_lines_with_adjacency,
			 m_geometry_shader_code_output_line_strip);

	m_tests.push_back(&m_test_lines_adjacency_to_line_strip);

	initCase(m_test_triangles_adjacency_to_triangle_strip, m_glExtTokens.TRIANGLE_STRIP_ADJACENCY,
			 6 /* number of vertices */, 6 /* expected array length */, GL_TRIANGLES,
			 m_geometry_shader_code_input_triangles_with_adjacency, m_geometry_shader_code_output_triangle_strip);

	m_tests.push_back(&m_test_triangles_adjacency_to_triangle_strip);

	/* Initialize program objects */
	for (testContainer::iterator it = m_tests.begin(); it != m_tests.end(); ++it)
	{
		/* Case instance */
		Case* test = *it;

		/* Init program */
		initCaseProgram(*test, captured_varyings, sizeof(captured_varyings) / sizeof(captured_varyings[0]));
	}

	/* Generate, bind and allocate buffer */
	gl.genBuffers(1, &m_buffer_object_id);
	gl.bindBuffer(GL_ARRAY_BUFFER, m_buffer_object_id);
	gl.bufferData(GL_ARRAY_BUFFER, m_buffer_size, 0 /* no start data */, GL_STATIC_DRAW);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not create buffer object");

	/* Generate and bind VAO */
	gl.genVertexArrays(1, &m_vertex_array_object_id);
	gl.bindVertexArray(m_vertex_array_object_id);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not create vertex array object");
}

/** Executes the test.
 *  Sets the test result to QP_TEST_RESULT_FAIL if the test failed, QP_TEST_RESULT_PASS otherwise.
 *  @return STOP if the test has finished, CONTINUE to indicate iterate should be called once again.
 *  Note the function throws exception should an error occur!
 **/
tcu::TestCase::IterateResult GeometryShader_gl_in_ArrayLengthTest::iterate()
{
	/* GL */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* This test should only run if EXT_geometry_shader is supported */
	if (true != m_is_geometry_shader_extension_supported)
	{
		throw tcu::NotSupportedError(GEOMETRY_SHADER_EXTENSION_NOT_SUPPORTED, "", __FILE__, __LINE__);
	}

	/* Setup transform feedback */
	gl.enable(GL_RASTERIZER_DISCARD);
	gl.bindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, m_buffer_object_id);

	/* Execute tests */
	for (testContainer::iterator it = m_tests.begin(); it != m_tests.end(); ++it)
	{
		glw::GLint result_value = 0;
		Case*	  test			= *it;

		/* Execute */
		gl.useProgram(test->po_id);

		gl.beginTransformFeedback(test->tf_mode);
		{
			gl.drawArrays(test->draw_call_mode, 0, /* first */
						  test->draw_call_n_vertices);
		}
		gl.endTransformFeedback();

		GLU_EXPECT_NO_ERROR(gl.getError(), "Error doing a draw call");

		/* Map transform feedback results */
		glw::GLint* result = (glw::GLint*)gl.mapBufferRange(GL_TRANSFORM_FEEDBACK_BUFFER, 0 /* offset */,
															sizeof(glw::GLint), GL_MAP_READ_BIT);

		GLU_EXPECT_NO_ERROR(gl.getError(), "Could not map the buffer object into process space");

		/* Extract value from transform feedback */
		result_value = *result;

		/* Unmap transform feedback buffer */
		gl.unmapBuffer(GL_TRANSFORM_FEEDBACK_BUFFER);

		GLU_EXPECT_NO_ERROR(gl.getError(), "Error unmapping the buffer object");

		/* Verify results */
		if (result_value != test->expected_array_length)
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "Expected array length: " << test->expected_array_length
							   << " but found: " << result_value << tcu::TestLog::EndMessage;

			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
			return STOP;
		}
	}

	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	return STOP;
}

/** Deinitializes test case
 *
 **/
void GeometryShader_gl_in_ArrayLengthTest::deinit()
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
	}

	if (0 != m_buffer_object_id)
	{
		gl.deleteBuffers(1, &m_buffer_object_id);
	}

	/* Deinit test cases */
	for (testContainer::iterator it = m_tests.begin(); it != m_tests.end(); ++it)
	{
		Case* test = *it;

		deinitCase(*test);
	}

	m_tests.clear();

	/* Deinitialize Base */
	TestCaseBase::deinit();
}

/** Deinitialize test case instance
 *
 * @param info Case instance
 **/
void GeometryShader_gl_in_ArrayLengthTest::deinitCase(Case& info)
{
	/* GL */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Delete everything */
	if (0 != info.po_id)
	{
		gl.deleteProgram(info.po_id);
	}

	if (0 != info.vs_id)
	{
		gl.deleteShader(info.vs_id);
	}

	if (0 != info.gs_id)
	{
		gl.deleteShader(info.gs_id);
	}

	if (0 != info.fs_id)
	{
		gl.deleteShader(info.fs_id);
	}

	/* Clear case */
	resetCase(info);
}

/** Initialize test case instance with provided data.
 *
 * @param info                  Case instance;
 * @param draw_call_mode        Primitive type used by a draw call;
 * @param draw_call_n_vertices  Number of vertices used by a draw call;
 * @param expected_array_length Expected size of gl_in array;
 * @param tf_mode               Primitive type used by transform feedback;
 * @param input_body_part       Part of geometry shader which specifies input layout;
 * @param output_body_part      Part of geometry shader which specifies output layout;
 **/
void GeometryShader_gl_in_ArrayLengthTest::initCase(Case& info, glw::GLenum draw_call_mode,
													glw::GLint draw_call_n_vertices, glw::GLint expected_array_length,
													glw::GLenum tf_mode, const glw::GLchar* input_body_part,
													const glw::GLchar* output_body_part)
{
	/* Reset case descriptor */
	resetCase(info);

	/* Set fields */
	info.draw_call_mode		   = draw_call_mode;
	info.draw_call_n_vertices  = draw_call_n_vertices;
	info.expected_array_length = expected_array_length;
	info.input_body_part	   = input_body_part;
	info.output_body_part	  = output_body_part;
	info.tf_mode			   = tf_mode;
}

/** Creates and build program for given Case
 *
 * @param info                     Case instance
 * @param captured_varyings        Name of varyings captured by transform feedback
 * @param n_captured_varyings_size Number of varyings captured by transform feedback
 **/
void GeometryShader_gl_in_ArrayLengthTest::initCaseProgram(Case& info, const glw::GLchar** captured_varyings,
														   glw::GLuint n_captured_varyings_size)
{
	/* GL */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Create program and shader objects */
	info.po_id = gl.createProgram();

	info.vs_id = gl.createShader(GL_VERTEX_SHADER);
	info.gs_id = gl.createShader(m_glExtTokens.GEOMETRY_SHADER);
	info.fs_id = gl.createShader(GL_FRAGMENT_SHADER);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not create program object");

	/* Prepare geometry shader parts */
	const char* const geometry_shader_parts[] = { m_geometry_shader_code_preamble, info.input_body_part,
												  info.output_body_part, m_geometry_shader_code_main };

	/* Set up transform feedback */
	gl.transformFeedbackVaryings(info.po_id, n_captured_varyings_size, captured_varyings, GL_SEPARATE_ATTRIBS);

	/* Build program */
	if (false == buildProgram(info.po_id, info.fs_id, 1 /* number of fragment shader code parts */,
							  &m_fragment_shader_code, info.gs_id, DE_LENGTH_OF_ARRAY(geometry_shader_parts),
							  geometry_shader_parts, info.vs_id, 1 /* number of vertex shader code parts */,
							  &m_vertex_shader_code))
	{
		TCU_FAIL("Could not create program from valid vertex/geometry/fragment shader");
	}
}

/** Reset Case instance descriptor's contents.
 *
 * @param info Case instance
 **/
void GeometryShader_gl_in_ArrayLengthTest::resetCase(Case& info)
{
	memset(&info, 0, sizeof(info));
}

/** Constructor
 *
 * @param context     Test context
 * @param name        Test case's name
 * @param description Test case's desricption
 **/
GeometryShader_gl_PointSize_ValueTest::GeometryShader_gl_PointSize_ValueTest(Context&			  context,
																			 const ExtParameters& extParams,
																			 const char* name, const char* description)
	: TestCaseBase(context, extParams, name, description)
	, m_fragment_shader_id(0)
	, m_geometry_shader_id(0)
	, m_program_object_id(0)
	, m_vertex_shader_id(0)
	, m_vertex_array_object_id(0)
	, m_color_texture_id(0)
	, m_framebuffer_object_id(0)
{
	/* Nothing to be done here */
}

/** Initialize test case
 *
 **/
void GeometryShader_gl_PointSize_ValueTest::init()
{
	/* Initialize Base */
	TestCaseBase::init();

	/* This test should only run if EXT_geometry_shader and EXT_geometry_point_size both are supported */
	if (true != m_is_geometry_shader_extension_supported)
	{
		throw tcu::NotSupportedError(GEOMETRY_SHADER_EXTENSION_NOT_SUPPORTED, "", __FILE__, __LINE__);
	}

	if (true != m_is_geometry_shader_point_size_supported)
	{
		throw tcu::NotSupportedError(GEOMETRY_SHADER_POINT_SIZE_NOT_SUPPORTED, "", __FILE__, __LINE__);
	}

	/* GL */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Verify that point size range is supported */
	glw::GLfloat point_size_range[2] = { 0 };

	if (!glu::isContextTypeES(m_context.getRenderContext().getType()))
	{
		gl.getFloatv(GL_POINT_SIZE_RANGE, point_size_range);
	}
	else
	{
		gl.getFloatv(GL_ALIASED_POINT_SIZE_RANGE, point_size_range);
	}

	if (8.0f > point_size_range[1])
	{
		m_testCtx.getLog() << tcu::TestLog::Message
						   << "Test requires a minimum maximum point size of 8, implementation reports a maximum of : "
						   << point_size_range[1] << tcu::TestLog::EndMessage;

		throw tcu::NotSupportedError("Not supported point size", "", __FILE__, __LINE__);
	}

	if (!glu::isContextTypeES(m_context.getRenderContext().getType()))
	{
		gl.enable(GL_PROGRAM_POINT_SIZE);
	}

	/* Create program and shaders */
	m_program_object_id = gl.createProgram();

	m_fragment_shader_id = gl.createShader(GL_FRAGMENT_SHADER);
	m_geometry_shader_id = gl.createShader(m_glExtTokens.GEOMETRY_SHADER);
	m_vertex_shader_id   = gl.createShader(GL_VERTEX_SHADER);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Failed to create program");

	/* Build program */
	if (false == buildProgram(m_program_object_id, m_fragment_shader_id, 1 /* fragment shader parts number */,
							  &m_fragment_shader_code, m_geometry_shader_id, 1 /* geometry shader parts number */,
							  &m_geometry_shader_code, m_vertex_shader_id, 1 /* vertex shader parts number */,
							  &m_vertex_shader_code))
	{
		TCU_FAIL("Could not create program from valid vertex/geometry/fragment shader");
	}

	/* Set up texture object and a FBO */
	gl.genTextures(1, &m_color_texture_id);
	gl.genFramebuffers(1, &m_framebuffer_object_id);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not create framebuffer");

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
 *  Sets the test result to QP_TEST_RESULT_FAIL if the test failed, QP_TEST_RESULT_PASS otherwise.
 *  @return STOP if the test has finished, CONTINUE to indicate iterate should be called once again.
 *  Note the function throws exception should an error occur!
 **/
tcu::TestCase::IterateResult GeometryShader_gl_PointSize_ValueTest::iterate()
{
	/* Buffer to store results of rendering */
	unsigned char result_image[m_texture_width * m_texture_height * m_texture_pixel_size];

	/* This test should only run if EXT_geometry_shader is supported */
	if (true != m_is_geometry_shader_extension_supported)
	{
		throw tcu::NotSupportedError(GEOMETRY_SHADER_EXTENSION_NOT_SUPPORTED, "", __FILE__, __LINE__);
	}

	if (true != m_is_geometry_shader_point_size_supported)
	{
		throw tcu::NotSupportedError(GEOMETRY_SHADER_POINT_SIZE_NOT_SUPPORTED, "", __FILE__, __LINE__);
	}

	/* GL  */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Render */
	gl.useProgram(m_program_object_id);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Failed to use program");

	gl.clearColor(0 /* red */, 0 /* green */, 0 /* blue */, 0 /* alpha */);
	gl.clear(GL_COLOR_BUFFER_BIT);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not clear the color buffer");

	gl.drawArrays(GL_POINTS, 0 /* first */, 2 /* count */);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Call drawArrays() failed");

	/* Check if the data was modified during the rendering process */
	gl.readPixels(0 /* x */, 0 /* y */, m_texture_width, m_texture_height, GL_RGBA, GL_UNSIGNED_BYTE, result_image);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not read back pixels from color buffer");

	/* 1) pixel at (2,  8) is (255, 255, 255, 255) */
	unsigned int referencePixelCoordinates[2] = { 2, 8 };

	if (false == comparePixel(result_image, referencePixelCoordinates[0] /* x */, referencePixelCoordinates[1] /* y */,
							  m_texture_width, m_texture_height, m_texture_pixel_size, 255 /* red */, 255 /* green */,
							  255 /* blue */, 255 /* alpha */))
	{
		const unsigned int texel_offset = referencePixelCoordinates[1] * m_texture_width * m_texture_pixel_size +
										  referencePixelCoordinates[0] * m_texture_pixel_size;

		m_testCtx.getLog() << tcu::TestLog::Message << "Rendered data [" << result_image[texel_offset + 0] << ", "
						   << result_image[texel_offset + 1] << ", " << result_image[texel_offset + 2] << ", "
						   << result_image[texel_offset + 3] << "]"
						   << " is different from reference data [255, 255, 255, 255]!" << tcu::TestLog::EndMessage;

		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
		return STOP;
	}

	/* 2) pixel at (14, 8) is (255, 255, 255, 255) */
	referencePixelCoordinates[0] = 14;
	referencePixelCoordinates[1] = 8;

	if (false == comparePixel(result_image, referencePixelCoordinates[0] /* x */, referencePixelCoordinates[1] /* y */,
							  m_texture_width, m_texture_height, m_texture_pixel_size, 255 /* red */, 255 /* green */,
							  255 /* blue */, 255 /* alpha */))
	{
		const unsigned int texel_offset = referencePixelCoordinates[1] * m_texture_width * m_texture_pixel_size +
										  referencePixelCoordinates[0] * m_texture_pixel_size;

		m_testCtx.getLog() << tcu::TestLog::Message << "Rendered data [" << result_image[texel_offset + 0] << ", "
						   << result_image[texel_offset + 1] << ", " << result_image[texel_offset + 2] << ", "
						   << result_image[texel_offset + 3] << "]"
						   << " is different from reference data [255, 255, 255, 255]!" << tcu::TestLog::EndMessage;

		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
		return STOP;
	}

	/* 3) pixel at (6,  8) is (0,   0,   0,   0) */
	referencePixelCoordinates[0] = 6;
	referencePixelCoordinates[1] = 8;

	if (false == comparePixel(result_image, referencePixelCoordinates[0] /* x */, referencePixelCoordinates[1] /* y */,
							  m_texture_width, m_texture_height, m_texture_pixel_size, 0 /* red */, 0 /* green */,
							  0 /* blue */, 0 /* alpha */))
	{
		const unsigned int texel_offset = referencePixelCoordinates[1] * m_texture_width * m_texture_pixel_size +
										  referencePixelCoordinates[0] * m_texture_pixel_size;

		m_testCtx.getLog() << tcu::TestLog::Message << "Rendered data [" << result_image[texel_offset + 0] << ", "
						   << result_image[texel_offset + 1] << ", " << result_image[texel_offset + 2] << ", "
						   << result_image[texel_offset + 3] << "]"
						   << "is different from reference data [0, 0, 0, 0]!" << tcu::TestLog::EndMessage;

		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
		return STOP;
	}

	/* Done */
	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	return STOP;
}

/** Deinitializes test case
 *
 **/
void GeometryShader_gl_PointSize_ValueTest::deinit()
{
	/* GL */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Bind defaults */
	gl.useProgram(0);
	gl.bindVertexArray(0);
	gl.framebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0 /* texture */, 0 /* level */);
	gl.bindTexture(GL_TEXTURE_2D, 0);
	gl.bindFramebuffer(GL_FRAMEBUFFER, 0);
	if (!glu::isContextTypeES(m_context.getRenderContext().getType()))
	{
		gl.disable(GL_PROGRAM_POINT_SIZE);
	}

	/* Delete everything */
	if (m_program_object_id != 0)
	{
		gl.deleteProgram(m_program_object_id);
	}

	if (m_fragment_shader_id != 0)
	{
		gl.deleteShader(m_fragment_shader_id);
	}

	if (m_geometry_shader_id != 0)
	{
		gl.deleteShader(m_geometry_shader_id);
	}

	if (m_vertex_shader_id != 0)
	{
		gl.deleteShader(m_vertex_shader_id);
	}

	if (m_vertex_array_object_id != 0)
	{
		gl.deleteVertexArrays(1, &m_vertex_array_object_id);
	}

	if (m_color_texture_id != 0)
	{
		gl.deleteTextures(1, &m_color_texture_id);
	}

	if (m_framebuffer_object_id != 0)
	{
		gl.deleteFramebuffers(1, &m_framebuffer_object_id);
	}

	/* Release base class */
	TestCaseBase::deinit();
}

/** Constructor
 *
 * @param context     Test context
 * @param name        Test case's name
 * @param description Test case's desricption
 **/
GeometryShader_gl_Position_ValueTest::GeometryShader_gl_Position_ValueTest(Context&				context,
																		   const ExtParameters& extParams,
																		   const char* name, const char* description)
	: TestCaseBase(context, extParams, name, description)
	, m_fragment_shader_id(0)
	, m_geometry_shader_id(0)
	, m_program_object_id(0)
	, m_vertex_shader_id(0)
	, m_vertex_array_object_id(0)
	, m_color_texture_id(0)
	, m_framebuffer_object_id(0)
{
	/* Nothing to be done here */
}

/** Initialize test case
 *
 **/
void GeometryShader_gl_Position_ValueTest::init()
{
	/* Initialize base */
	TestCaseBase::init();

	/* This test should only run if EXT_geometry_shader is supported */
	if (true != m_is_geometry_shader_extension_supported)
	{
		throw tcu::NotSupportedError(GEOMETRY_SHADER_EXTENSION_NOT_SUPPORTED, "", __FILE__, __LINE__);
	}

	if (true != m_is_geometry_shader_point_size_supported)
	{
		throw tcu::NotSupportedError(GEOMETRY_SHADER_POINT_SIZE_NOT_SUPPORTED, "", __FILE__, __LINE__);
	}

	/* GL */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Verify that point size range is supported */
	glw::GLfloat point_size_range[2] = { 0 };

	if (!glu::isContextTypeES(m_context.getRenderContext().getType()))
	{
		gl.getFloatv(GL_POINT_SIZE_RANGE, point_size_range);
	}
	else
	{
		gl.getFloatv(GL_ALIASED_POINT_SIZE_RANGE, point_size_range);
	}

	if (8.0f > point_size_range[1])
	{
		m_testCtx.getLog() << tcu::TestLog::Message
						   << "Test requires a minimum maximum point size of 8, implementation reports a maximum of : "
						   << point_size_range[1] << tcu::TestLog::EndMessage;

		throw tcu::NotSupportedError("Not supported point size", "", __FILE__, __LINE__);
	}

	if (!glu::isContextTypeES(m_context.getRenderContext().getType()))
	{
		gl.enable(GL_PROGRAM_POINT_SIZE);
	}

	/* Create program and shaders */
	m_program_object_id = gl.createProgram();

	m_fragment_shader_id = gl.createShader(GL_FRAGMENT_SHADER);
	m_geometry_shader_id = gl.createShader(m_glExtTokens.GEOMETRY_SHADER);
	m_vertex_shader_id   = gl.createShader(GL_VERTEX_SHADER);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Failed to create program");

	/* Build program */
	if (false == buildProgram(m_program_object_id, m_fragment_shader_id, 1 /* fragment shader parts number */,
							  &m_fragment_shader_code, m_geometry_shader_id, 1 /* geometry shader parts number */,
							  &m_geometry_shader_code, m_vertex_shader_id, 1 /* vertex shader parts number */,
							  &m_vertex_shader_code))
	{
		TCU_FAIL("Could not create program from valid vertex/geometry/fragment shader");
	}

	/* Set up a texture object and a FBO */
	gl.genTextures(1, &m_color_texture_id);
	gl.genFramebuffers(1, &m_framebuffer_object_id);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not create framebuffer");

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
 *  Sets the test result to QP_TEST_RESULT_FAIL if the test failed, QP_TEST_RESULT_PASS otherwise.
 *  @return STOP if the test has finished, CONTINUE to indicate iterate should be called once again.
 *  Note the function throws exception should an error occur!
 **/
tcu::TestCase::IterateResult GeometryShader_gl_Position_ValueTest::iterate()
{
	/* Variables used for image verification purposes */
	unsigned char result_image[m_texture_width * m_texture_height * m_texture_pixel_size];

	/* This test should only run if EXT_geometry_shader is supported */
	if (true != m_is_geometry_shader_extension_supported)
	{
		throw tcu::NotSupportedError(GEOMETRY_SHADER_EXTENSION_NOT_SUPPORTED, "", __FILE__, __LINE__);
	}

	/* GL  */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Render */
	gl.useProgram(m_program_object_id);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Failed to use program");

	gl.clearColor(0 /* red */, 0 /* green */, 0 /* blue */, 0 /* alpha */);
	gl.clear(GL_COLOR_BUFFER_BIT);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not clear the color buffer");

	gl.drawArrays(GL_POINTS, 0 /* first */, 8 /* count */);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Call drawArrays() failed");

	/* Check if the data was modified during the rendering process */
	gl.readPixels(0 /* x */, 0 /* y */, m_texture_width, m_texture_height, GL_RGBA, GL_UNSIGNED_BYTE, result_image);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not read back pixels from color buffer");

	/* The test passes if centers of the rendered points are lit at expected locations. */
	for (unsigned int x = 4; x < m_texture_width; x += 8)
	{
		if (false == comparePixel(result_image, x, 32 /* y */, m_texture_width, m_texture_height, m_texture_pixel_size,
								  255 /* red */, 255 /* green */, 255 /* blue */, 255 /* alpha */))
		{
			const unsigned int texel_offset = 32 * m_texture_width * m_texture_pixel_size + x * m_texture_pixel_size;

			m_testCtx.getLog() << tcu::TestLog::Message << "Rendered data [" << result_image[texel_offset + 0] << ", "
							   << result_image[texel_offset + 1] << ", " << result_image[texel_offset + 2] << ", "
							   << result_image[texel_offset + 3] << "]"
							   << "is different from reference data [255, 255, 255, 255] !" << tcu::TestLog::EndMessage;

			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
			return STOP;
		}
	}

	/* Done */
	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	return STOP;
}

/** Deinitializes test case
 *
 **/
void GeometryShader_gl_Position_ValueTest::deinit()
{
	/* GL */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Bind default values */
	gl.useProgram(0);
	gl.bindVertexArray(0);
	gl.framebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0 /* texture */, 0 /* level */);
	gl.bindTexture(GL_TEXTURE_2D, 0);
	gl.bindFramebuffer(GL_FRAMEBUFFER, 0);
	if (!glu::isContextTypeES(m_context.getRenderContext().getType()))
	{
		gl.disable(GL_PROGRAM_POINT_SIZE);
	}

	/* Delete everything */
	if (m_program_object_id != 0)
	{
		gl.deleteProgram(m_program_object_id);
	}

	if (m_fragment_shader_id != 0)
	{
		gl.deleteShader(m_fragment_shader_id);
	}

	if (m_geometry_shader_id != 0)
	{
		gl.deleteShader(m_geometry_shader_id);
	}

	if (m_vertex_shader_id != 0)
	{
		gl.deleteShader(m_vertex_shader_id);
	}

	if (m_vertex_array_object_id != 0)
	{
		gl.deleteVertexArrays(1, &m_vertex_array_object_id);
	}

	if (m_color_texture_id != 0)
	{
		gl.deleteTextures(1, &m_color_texture_id);
	}

	if (m_framebuffer_object_id != 0)
	{
		gl.deleteFramebuffers(1, &m_framebuffer_object_id);
	}

	/* Release base class */
	TestCaseBase::deinit();
}

} /* glcts */
