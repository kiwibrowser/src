/*-------------------------------------------------------------------------
 * OpenGL Conformance Test Suite
 * -----------------------------
 *
 * Copyright (c) 2015-2016 The Khronos Group Inc.
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

/* Includes. */
#include "gl3cClipDistance.hpp"
#include "gluContextInfo.hpp"
#include "gluDefs.hpp"
#include "gluRenderContext.hpp"
#include "gluStrUtil.hpp"
#include "tcuTestLog.hpp"

#include <cmath>
#include <sstream>

/* Stringify macro. */
#define _STR(s) STR(s)
#define STR(s) #s

/* In OpenGL 3.0 specification GL_CLIP_DISTANCEi is named GL_CLIP_PLANEi */
#ifndef GL_CLIP_DISTANCE0
#define GL_CLIP_DISTANCE0 GL_CLIP_PLANE0
#endif

/* In OpenGL 3.0 specification GL_MAX_CLIP_DISTANCES is named GL_MAX_CLIP_PLANES */
#ifndef GL_MAX_CLIP_DISTANCES
#define GL_MAX_CLIP_DISTANCES GL_MAX_CLIP_PLANES
#endif

/******************************** Test Group Implementation       ********************************/

/** @brief Clip distances tests group constructor.
 *
 *  @param [in] context     OpenGL context.
 */
gl3cts::ClipDistance::Tests::Tests(deqp::Context& context)
	: TestCaseGroup(context, "clip_distance", "Clip Distance Test Suite")
{
	/* Intentionally left blank */
}

/** @brief Clip distances tests initializer. */
void gl3cts::ClipDistance::Tests::init()
{
	addChild(new gl3cts::ClipDistance::CoverageTest(m_context));
	addChild(new gl3cts::ClipDistance::FunctionalTest(m_context));
	addChild(new gl3cts::ClipDistance::NegativeTest(m_context));
}

/******************************** Coverage Tests Implementation   ********************************/

/** @brief API coverage tests constructor.
 *
 *  @param [in] context     OpenGL context.
 */
gl3cts::ClipDistance::CoverageTest::CoverageTest(deqp::Context& context)
	: deqp::TestCase(context, "coverage", "Clip Distance API Coverage Test"), m_gl_max_clip_distances_value(0)
{
	/* Intentionally left blank. */
}

/** @brief Iterate API coverage tests.
 *
 *  @return Iteration result.
 */
tcu::TestNode::IterateResult gl3cts::ClipDistance::CoverageTest::iterate()
{
	/* Shortcut for GL functionality */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* This test should only be executed if we're running a GL3.0 context */
	if (!glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType(3, 0, glu::PROFILE_CORE)))
	{
		throw tcu::NotSupportedError("GL_ARB_clip_distance is not supported");
	}

	/* Running tests. */
	bool is_ok = true;

	is_ok = is_ok && MaxClipDistancesValueTest(gl);
	is_ok = is_ok && EnableDisableTest(gl);
	is_ok = is_ok && MaxClipDistancesValueInVertexShaderTest(gl);
	is_ok = is_ok && MaxClipDistancesValueInFragmentShaderTest(gl);
	is_ok = is_ok && ClipDistancesValuePassing(gl);

	/* Result's setup. */
	if (is_ok)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
	}

	return STOP;
}

/* @brief glGet GL_MAX_CLIP_DISTANCES limit coverage test.
 *
 *  @param [in] gl                  OpenGL functions' access.
 *
 *  @return True if passed, false otherwise.
 */
bool gl3cts::ClipDistance::CoverageTest::MaxClipDistancesValueTest(const glw::Functions& gl)
{
	/*  Check that calling GetIntegerv with GL_MAX_CLIP_DISTANCES doesn't
	 generate any errors and returns a value at least 6 in OpenGL 3.0
	 or 8 in OpenGL 3.1 and higher (see issues). */

	glw::GLint error_code = GL_NO_ERROR;

	gl.getIntegerv(GL_MAX_CLIP_DISTANCES, &m_gl_max_clip_distances_value);

	error_code = gl.getError();

	if (error_code != GL_NO_ERROR)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "glGetIntegerv(" << STR(GL_MAX_CLIP_DISTANCES)
						   << ") returned error code " << glu::getErrorStr(error_code) << " instead of GL_NO_ERROR."
						   << tcu::TestLog::EndMessage;

		return false;
	}
	else
	{
		glw::GLint gl_max_clip_distances_minimum_value = 6; /* OpenGL 3.0 Specification minimum value */

		if (glu::contextSupports(
				m_context.getRenderContext().getType(),
				glu::ApiType(3, 1, glu::PROFILE_CORE))) /* OpenGL 3.1 Specification minimum value, see bug #4803 */
		{
			gl_max_clip_distances_minimum_value = 8;
		}

		if (m_gl_max_clip_distances_value < gl_max_clip_distances_minimum_value)
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "Value of " << STR(GL_MAX_CLIP_DISTANCES) << "is equal to "
							   << m_gl_max_clip_distances_value << " which is less than minimum required ("
							   << gl_max_clip_distances_minimum_value << ")." << tcu::TestLog::EndMessage;

			return false;
		}
	}

	return true;
}

/* @brief glEnable / glDisable of GL_CLIP_DISTANCEi coverage test.
 *
 *  @param [in] gl                  OpenGL functions' access.
 *
 *  @return True if passed, false otherwise.
 */
bool gl3cts::ClipDistance::CoverageTest::EnableDisableTest(const glw::Functions& gl)
{
	/*  Check that calling Enable and Disable with GL_CLIP_DISTANCEi for all
	 available clip distances does not generate errors.
	 glw::GLint error_code = GL_NO_ERROR; */

	glw::GLint error_code = GL_NO_ERROR;

	/* Test glEnable */
	for (glw::GLint i = 0; i < m_gl_max_clip_distances_value; ++i)
	{
		gl.enable(GL_CLIP_DISTANCE0 + i);

		error_code = gl.getError();

		if (error_code != GL_NO_ERROR)
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "glEnable(GL_CLIP_DISTANCE" << i << ") returned error code "
							   << glu::getErrorStr(error_code) << " instead of GL_NO_ERROR."
							   << tcu::TestLog::EndMessage;

			return false;
		}
	}

	/* Test glDisable */
	for (glw::GLint i = 0; i < m_gl_max_clip_distances_value; ++i)
	{
		gl.disable(GL_CLIP_DISTANCE0 + i);

		error_code = gl.getError();

		if (error_code != GL_NO_ERROR)
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "glDisable(GL_CLIP_DISTANCE" << i << ") returned error code "
							   << glu::getErrorStr(error_code) << " instead of GL_NO_ERROR."
							   << tcu::TestLog::EndMessage;

			return false;
		}
	}

	return true;
}

/* @brief gl_MaxClipDistances value test in the vertex shader coverage test.
 *
 *  @param [in] gl                  OpenGL functions' access.
 *
 *  @return True if passed, false otherwise.
 */
bool gl3cts::ClipDistance::CoverageTest::MaxClipDistancesValueInVertexShaderTest(const glw::Functions& gl)
{
	/*  Make a program that consist of vertex and fragment shader stages. A
	 vertex shader shall assign the value of gl_MaxClipDistances to transform
	 feedback output variable. Setup gl_Position with passed in attribute.
	 Use blank fragment shader. Check that the shaders compiles and links
	 successfully. Draw a single GL_POINT with screen centered position
	 attribute, a configured transform feedback and GL_RASTERIZER_DISCARD.
	 Query transform feedback value and compare it against
	 GL_MAX_CLIP_DISTANCES. Expect that both values are equal. */

	/* Building program. */
	const std::string vertex_shader					  = m_vertex_shader_code_case_0;
	const std::string fragment_shader				  = m_fragment_shader_code_case_0;
	std::string		  transform_feedback_varying_name = "max_value";

	std::vector<std::string> transform_feedback_varyings(1, transform_feedback_varying_name);

	gl3cts::ClipDistance::Utility::Program program(gl, vertex_shader, fragment_shader, transform_feedback_varyings);

	if (program.ProgramStatus().program_id == 0)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Program failed to build.\n Vertex Shader:\n"
						   << m_vertex_shader_code_case_0 << "\nVertex Shader compilation log:\n"
						   << program.VertexShaderStatus().shader_log << "\nWith Fragment Shader:\n"
						   << m_fragment_shader_code_case_0 << "\nWith Fragment Shader compilation log:\n"
						   << program.FragmentShaderStatus().shader_log << "\nWith Program linkage log:\n"
						   << program.FragmentShaderStatus().shader_log << "\n"
						   << tcu::TestLog::EndMessage;
		return false;
	}

	program.UseProgram();

	/* Creating and binding empty VAO. */
	gl3cts::ClipDistance::Utility::VertexArrayObject vertex_array_object(gl, GL_POINTS);

	/* Creating and binding output VBO */
	gl3cts::ClipDistance::Utility::VertexBufferObject<glw::GLint> vertex_buffer_object(gl, GL_TRANSFORM_FEEDBACK_BUFFER,
																					   std::vector<glw::GLint>(1, 0));

	/* Draw test. */
	vertex_array_object.drawWithTransformFeedback(0, 1, true);

	/* Check results. */
	std::vector<glw::GLint> results = vertex_buffer_object.readBuffer();

	if (results.size() < 1)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Results reading error." << tcu::TestLog::EndMessage;
		return false;
	}

	if (results[0] != m_gl_max_clip_distances_value)
	{
		m_testCtx.getLog() << tcu::TestLog::Message
						   << "Vertex shader's gl_MaxClipDistances constant has improper value equal to " << results[0]
						   << "but " << m_gl_max_clip_distances_value << "is expected. Test failed."
						   << tcu::TestLog::EndMessage;
		return false;
	}

	/* Test passed. */
	return true;
}

/* @brief gl_MaxClipDistances value test in the fragment shader coverage test.
 *
 *  @param [in] gl                  OpenGL functions' access.
 *
 *  @return True if passed, false otherwise.
 */
bool gl3cts::ClipDistance::CoverageTest::MaxClipDistancesValueInFragmentShaderTest(const glw::Functions& gl)
{
	/*  Make a program that consist of vertex and fragment shader stages. In
	 vertex shader setup gl_Position with passed in attribute. Check in
	 fragment shader using "if" statement that gl_MaxClipDistances is equal
	 to GL_MAX_CLIP_DISTANCES passed by uniform. If compared values are not
	 equal, discard the fragment. Output distinguishable color otherwise.
	 Check that the shader program compiles and links successfully. Draw a
	 single GL_POINT with screen centered position attribute and with a
	 configured 1 x 1 pixel size framebuffer. Using glReadPixels function,
	 check that point's fragments were not discarded. */

	/* Creating red-color-only frambuffer. */
	gl3cts::ClipDistance::Utility::Framebuffer framebuffer(gl, 1, 1);

	if (!framebuffer.isValid())
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Unable to create framebuffer with size [1,1].\n"
						   << tcu::TestLog::EndMessage;
		return false;
	}

	framebuffer.bind();
	framebuffer.clear();

	/* Building program. */
	const std::string vertex_shader   = m_vertex_shader_code_case_1;
	const std::string fragment_shader = m_fragment_shader_code_case_1;

	gl3cts::ClipDistance::Utility::Program program(gl, vertex_shader, fragment_shader);

	if (program.ProgramStatus().program_id == 0)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Program failed to build.\n Vertex Shader:\n"
						   << m_vertex_shader_code_case_1 << "\nVertex Shader compilation log:\n"
						   << program.VertexShaderStatus().shader_log << "\nWith Fragment Shader:\n"
						   << m_fragment_shader_code_case_1 << "\nWith Fragment Shader compilation log:\n"
						   << program.FragmentShaderStatus().shader_log << "\nWith Program linkage log:\n"
						   << program.FragmentShaderStatus().shader_log << "\n"
						   << tcu::TestLog::EndMessage;
		return false;
	}

	program.UseProgram();

	/* Creating empty VAO. */
	gl3cts::ClipDistance::Utility::VertexArrayObject vertex_array_object(gl, GL_POINTS);

	/* Draw test. */
	vertex_array_object.draw(0, 1);

	/* Fetch results. */
	std::vector<glw::GLfloat> pixels = framebuffer.readPixels();

	if (pixels.size() < 1)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "ReadPixels error.\n" << tcu::TestLog::EndMessage;
		return false;
	}

	/* Check results. */
	glw::GLuint gl_max_clip_distances_value_in_fragment_shader = glw::GLuint(pixels.front());

	if (gl_max_clip_distances_value_in_fragment_shader != glw::GLuint(m_gl_max_clip_distances_value))
	{
		m_testCtx.getLog() << tcu::TestLog::Message
						   << "Fragment shader's gl_MaxClipDistances constant has improper value equal to "
						   << gl_max_clip_distances_value_in_fragment_shader << "but " << m_gl_max_clip_distances_value
						   << "is expected. Test failed." << tcu::TestLog::EndMessage;
		return false;
	}

	/* Test passed. */
	return true;
}

/* @brief Vertex shader to fragment shader passing coverage test.
 *
 *  @param [in] gl                  OpenGL functions' access.
 *
 *  @return True if passed, false otherwise.
 */
bool gl3cts::ClipDistance::CoverageTest::ClipDistancesValuePassing(const glw::Functions& gl)
{
	/*  Make a program that consist of vertex and fragment shader stages.
	 Redeclare gl_ClipDistance with size equal to GL_MAX_CLIP_DISTANCES in
	 vertex and fragment shader. In vertex shader, assign values to
	 gl_ClipDistance array using function of clip distance index i:

	 f(i) = float(i + 1) / float(gl_MaxClipDistances).

	 Setup gl_Position with passed in attribute. Read gl_ClipDistance in the
	 fragment shader and compare them with the same function. Take into
	 account low precision errors. If compared values are not equal, discard
	 the fragment. Output distinguishable color otherwise. Check that the
	 shaders compiles and the program links successfully. Enable all
	 GL_CLIP_DISTANCEs. Draw a single GL_POINT with screen centered position
	 attribute and with a configured 1 x 1 pixel size framebuffer. Using
	 glReadPixels function, check that point's fragments were not discarded. */

	/* Creating red-color-only frambuffer. */
	gl3cts::ClipDistance::Utility::Framebuffer framebuffer(gl, 1, 1);

	if (!framebuffer.isValid())
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Unable to create framebuffer with size [1,1].\n"
						   << tcu::TestLog::EndMessage;
		return false;
	}

	framebuffer.bind();
	framebuffer.clear();

	/* Building program. */
	const std::string vertex_shader   = m_vertex_shader_code_case_2;
	const std::string fragment_shader = m_fragment_shader_code_case_2;

	gl3cts::ClipDistance::Utility::Program program(gl, vertex_shader, fragment_shader);

	if (program.ProgramStatus().program_id == 0)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Program failed to build.\n Vertex Shader:\n"
						   << m_vertex_shader_code_case_2 << "\nVertex Shader compilation log:\n"
						   << program.VertexShaderStatus().shader_log << "\nWith Fragment Shader:\n"
						   << m_fragment_shader_code_case_2 << "\nWith Fragment Shader compilation log:\n"
						   << program.FragmentShaderStatus().shader_log << "\nWith Program linkage log:\n"
						   << program.FragmentShaderStatus().shader_log << "\n"
						   << tcu::TestLog::EndMessage;
		return false;
	}

	program.UseProgram();

	/* Creating empty VAO. */
	gl3cts::ClipDistance::Utility::VertexArrayObject vertex_array_object(gl, GL_POINTS);

	/* Draw test. */
	vertex_array_object.draw(0, 1);

	/* Fetch results. */
	std::vector<glw::GLfloat> pixels = framebuffer.readPixels();

	if (pixels.size() < 1)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "ReadPixels error.\n" << tcu::TestLog::EndMessage;
		return false;
	}

	/* Check results. */
	glw::GLfloat results = pixels.front();

	if (fabs(results - 1.f) > 0.0125)
	{
		m_testCtx.getLog() << tcu::TestLog::Message
						   << "Fragment shader values of gl_Clip_distance does not match vertex shader's output value."
						   << tcu::TestLog::EndMessage;
		return false;
	}

	/* Test passed. */
	return true;
}

/** @brief Vertex shader source code to test gl_MaxClipDistances limit value in vertex shader (API Coverage Test). */
const glw::GLchar* gl3cts::ClipDistance::CoverageTest::m_vertex_shader_code_case_0 =
	"#version 130\n"
	"\n"
	"out int max_value;\n"
	"\n"
	"void main()\n"
	"{\n"
	"    max_value   = gl_MaxClipDistances;\n"
	"    gl_Position = vec4(0.0, 0.0, 0.0, 1.0);\n"
	"}\n";

/** @brief Fragment shader source code to test gl_MaxClipDistances limit value in vertex shader (API Coverage Test). */
const glw::GLchar* gl3cts::ClipDistance::CoverageTest::m_fragment_shader_code_case_0 =
	"#version 130\n"
	"\n"
	"out vec4 color;\n"
	"\n"
	"void main()\n"
	"{\n"
	"    color = vec4(0.0, 0.0, 0.0, 1.0);\n"
	"}\n";

/** @brief Vertex shader source code to test gl_MaxClipDistances limit value in fragment shader (API Coverage Test). */
const glw::GLchar* gl3cts::ClipDistance::CoverageTest::m_vertex_shader_code_case_1 =
	"#version 130\n"
	"\n"
	"void main()\n"
	"{\n"
	"    gl_Position  = vec4(0.0, 0.0, 0.0, 1.0);\n"
	"}\n";

/** @brief Fragment shader source code to test gl_MaxClipDistances limit value in fragment shader (API Coverage Test). */
const glw::GLchar* gl3cts::ClipDistance::CoverageTest::m_fragment_shader_code_case_1 =
	"#version 130\n"
	"\n"
	"out highp vec4 color;\n"
	"\n"
	"void main()\n"
	"{\n"
	"    color = vec4(float(gl_MaxClipDistances), 0.0, 0.0, 1.0);\n"
	"}\n";

/** @brief Vertex shader source code to test if the gl_ClipDistance[] are passed properly to the fragment shader from vertex shader (API Coverage Test). */
const glw::GLchar* gl3cts::ClipDistance::CoverageTest::m_vertex_shader_code_case_2 =
	"#version 130\n"
	"\n"
	"out float gl_ClipDistance[gl_MaxClipDistances];\n"
	"\n"
	"void main()\n"
	"{\n"
	"    for(int i = 0; i < gl_MaxClipDistances; i++)\n"
	"    {\n"
	"        gl_ClipDistance[i] = float(i + 1) / float(gl_MaxClipDistances);\n"
	"    }\n"
	"\n"
	"    gl_Position  = vec4(0.0, 0.0, 0.0, 1.0);\n"
	"}\n";

/** @brief Fragment shader source code to test if the gl_ClipDistance[] are passed properly to the fragment shader from vertex shader (API Coverage Test). */
const glw::GLchar* gl3cts::ClipDistance::CoverageTest::m_fragment_shader_code_case_2 =
	"#version 130\n"
	"\n"
	"in float gl_ClipDistance[gl_MaxClipDistances];\n"
	"\n"
	"out highp vec4 color;\n"
	"\n"
	"void main()\n"
	"{\n"
	"    for(int i = 0; i < gl_MaxClipDistances; i++)\n"
	"    {\n"
	"        if(abs(gl_ClipDistance[i] - float(i + 1) / float(gl_MaxClipDistances)) > 0.0125)\n"
	"        {\n"
	"            discard;\n"
	"        }\n"
	"    }\n"
	"\n"
	"    color = vec4(1.0, 0.0, 0.0, 1.0);\n"
	"}\n";

/******************************** Functional Tests Implementation ********************************/

/** @brief Functional test constructor.
 *
 *  @param [in] context     OpenGL context.
 */
gl3cts::ClipDistance::FunctionalTest::FunctionalTest(deqp::Context& context)
	: deqp::TestCase(context, "functional", "Clip Distance Functional Test")
	, m_gl_max_clip_distances_value(8) /* Specification minimum required */
{
	/* Intentionally left blank */
}

/** @brief Initialize functional test. */
void gl3cts::ClipDistance::FunctionalTest::init()
{
	/* Shortcut for GL functionality */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.getIntegerv(GL_MAX_CLIP_DISTANCES, &m_gl_max_clip_distances_value);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv call failed.");
}

/** @brief Iterate functional test cases.
 *
 *  @return Iteration result.
 */
tcu::TestNode::IterateResult gl3cts::ClipDistance::FunctionalTest::iterate()
{
	/* Shortcut for GL functionality */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* This test should only be executed if we're running a GL>=3.0 context */
	if (!glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::core(3, 0)))
	{
		throw tcu::NotSupportedError("GL_ARB_clip_distance is not supported");
	}

	/* Functional test */

	/* For all primitive modes. */
	for (glw::GLuint i_primitive_type = 0; i_primitive_type < m_primitive_types_count; ++i_primitive_type)
	{
		glw::GLenum primitive_type			= m_primitive_types[i_primitive_type];
		glw::GLenum primitive_indices_count = m_primitive_indices[i_primitive_type];

		/* Framebuffer setup. */
		glw::GLuint framebuffer_size = (primitive_type == GL_POINTS) ? 1 : 32;

		gl3cts::ClipDistance::Utility::Framebuffer framebuffer(gl, framebuffer_size,
															   framebuffer_size); /* Framebuffer shall be square */

		framebuffer.bind();

		/* For all clip combinations. */
		for (glw::GLuint i_clip_function = 0;
			 i_clip_function <
			 m_clip_function_count -
				 int(i_primitive_type == GL_POINTS); /* Do not use last clip function with GL_POINTS. */
			 ++i_clip_function)
		{
			/* For both redeclaration types (implicit/explicit). */
			for (glw::GLuint i_redeclaration = 0; i_redeclaration < 2; ++i_redeclaration)
			{
				bool redeclaration = (i_redeclaration == 1);

				/* For different clip array sizes. */
				for (glw::GLuint i_clip_count = 1; i_clip_count <= glw::GLuint(m_gl_max_clip_distances_value);
					 ++i_clip_count)
				{
					/* Create and build program. */
					std::string vertex_shader_code = gl3cts::ClipDistance::FunctionalTest::prepareVertexShaderCode(
						redeclaration, redeclaration, i_clip_count, i_clip_function, primitive_type);

					gl3cts::ClipDistance::Utility::Program program(gl, vertex_shader_code, m_fragment_shader_code);

					if (program.ProgramStatus().program_id == GL_NONE)
					{
						/* Result's setup. */
						m_testCtx.getLog()
							<< tcu::TestLog::Message
							<< "Functional test have failed when building program.\nVertex shader code:\n"
							<< vertex_shader_code << "\nFragment shader code:\n"
							<< m_fragment_shader_code << "\n"
							<< tcu::TestLog::EndMessage;

						m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");

						return STOP;
					}

					program.UseProgram();

					/* Framebuffer clear */
					framebuffer.clear();

					/* Clip setup */
					gl.enable(GL_CLIP_DISTANCE0 + i_clip_count - 1);

					/* Geometry Setup */
					gl3cts::ClipDistance::Utility::VertexArrayObject vertex_array_object(gl, primitive_type);

					gl3cts::ClipDistance::Utility::VertexBufferObject<glw::GLfloat>* vertex_buffer_object =
						prepareGeometry(gl, primitive_type);

					if (!vertex_buffer_object->useAsShaderInput(program, "position", 4))
					{
						/* Result's setup. */
						m_testCtx.getLog() << tcu::TestLog::Message
										   << "Functional test have failed when enabling vertex attribute array.\n"
										   << tcu::TestLog::EndMessage;

						m_testCtx.setTestResult(QP_TEST_RESULT_INTERNAL_ERROR, "Error");

						delete vertex_buffer_object;
						return STOP;
					}

					/* Draw geometry to the framebuffer */
					vertex_array_object.draw(0, primitive_indices_count);

					/* Check results */
					std::vector<glw::GLfloat> results = framebuffer.readPixels();

					if (!checkResults(primitive_type, i_clip_function, results))
					{
						/* Result's setup. */
						m_testCtx.getLog() << tcu::TestLog::Message << "Functional test have failed when drawing "
										   << glu::getPrimitiveTypeStr(primitive_type)
										   << ((redeclaration) ? " with " : " without ")
										   << "dynamic redeclaration, when " << i_clip_count
										   << " GL_CLIP_DISTANCES where enabled and set up using function:\n"
										   << m_clip_function[i_clip_function] << tcu::TestLog::EndMessage;

						m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");

						delete vertex_buffer_object;
						return STOP;
					}

					delete vertex_buffer_object;
				}

				/* Clip clean */
				for (glw::GLuint i_clip_count = 0; i_clip_count < glw::GLuint(m_gl_max_clip_distances_value);
					 ++i_clip_count)
				{
					gl.disable(GL_CLIP_DISTANCE0 + i_clip_count);
				}
			}
		}
	}

	/* Result's setup. */

	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

	return STOP;
}

/** @brief Prepare vertex shader code for functional test.
 *
 *  @param [in] explicit_redeclaration      Use explicit redeclaration with size.
 *  @param [in] dynamic_setter              Use dynamic array setter.
 *  @param [in] clip_count                  Set all first # of gl_ClipDistance-s.
 *  @param [in] clip_function               Use #th clip function for gl_ClipDistance[] setup (see m_clip_function[]).
 *  @param [in] primitive_type              Primitive mode.
 *
 *  @return Compilation ready vertex shader source code.
 */
std::string gl3cts::ClipDistance::FunctionalTest::prepareVertexShaderCode(bool explicit_redeclaration,
																		  bool dynamic_setter, glw::GLuint clip_count,
																		  glw::GLuint clip_function,
																		  glw::GLenum primitive_type)
{
	std::string vertex_shader = m_vertex_shader_code;

	if (explicit_redeclaration)
	{
		vertex_shader = gl3cts::ClipDistance::Utility::preprocessCode(vertex_shader, "CLIP_DISTANCE_REDECLARATION",
																	  m_explicit_redeclaration);
	}
	else
	{
		vertex_shader = gl3cts::ClipDistance::Utility::preprocessCode(vertex_shader, "CLIP_DISTANCE_REDECLARATION", "");
	}

	if (dynamic_setter)
	{
		vertex_shader =
			gl3cts::ClipDistance::Utility::preprocessCode(vertex_shader, "CLIP_DISTANCE_SETUP", m_dynamic_array_setter);
	}
	else
	{
		std::string static_setters = "";

		for (glw::GLuint i = 0; i < clip_count; ++i)
		{
			std::string i_setter = m_static_array_setter;

			i_setter = gl3cts::ClipDistance::Utility::preprocessCode(i_setter, "CLIP_INDEX",
																	 gl3cts::ClipDistance::Utility::itoa(i));

			static_setters.append(i_setter);
		}

		vertex_shader =
			gl3cts::ClipDistance::Utility::preprocessCode(vertex_shader, "CLIP_DISTANCE_SETUP", static_setters);
	}

	vertex_shader =
		gl3cts::ClipDistance::Utility::preprocessCode(vertex_shader, "CLIP_FUNCTION", m_clip_function[clip_function]);

	vertex_shader = gl3cts::ClipDistance::Utility::preprocessCode(vertex_shader, "CLIP_COUNT",
																  gl3cts::ClipDistance::Utility::itoa(clip_count));

	switch (primitive_type)
	{
	case GL_POINTS:
		vertex_shader = gl3cts::ClipDistance::Utility::preprocessCode(vertex_shader, "VERTEX_COUNT", "1");
		break;
	case GL_LINES:
		vertex_shader = gl3cts::ClipDistance::Utility::preprocessCode(vertex_shader, "VERTEX_COUNT", "2");
		break;
	case GL_TRIANGLES:
		vertex_shader = gl3cts::ClipDistance::Utility::preprocessCode(vertex_shader, "VERTEX_COUNT", "3");
		break;
	}

	return vertex_shader;
}

/** @brief Prepare geometry for functional test.
 *
 *  @param [in] gl                  OpenGL functions' access.
 *  @param [in] primitive_type      Primitive mode.
 *
 *  @return Vertex Buffer Object pointer.
 */
gl3cts::ClipDistance::Utility::VertexBufferObject<glw::GLfloat>* gl3cts::ClipDistance::FunctionalTest::prepareGeometry(
	const glw::Functions& gl, const glw::GLenum primitive_type)
{
	std::vector<glw::GLfloat> data;

	switch (primitive_type)
	{
	case GL_POINTS:
		data.push_back(0.0);
		data.push_back(0.0);
		data.push_back(0.0);
		data.push_back(1.0);
		break;
	case GL_LINES:
		data.push_back(1.0);
		data.push_back(1.0);
		data.push_back(0.0);
		data.push_back(1.0);
		data.push_back(-1.0);
		data.push_back(-1.0);
		data.push_back(0.0);
		data.push_back(1.0);
		break;
	case GL_TRIANGLES:
		data.push_back(-1.0);
		data.push_back(-1.0);
		data.push_back(0.0);
		data.push_back(1.0);
		data.push_back(0.0);
		data.push_back(1.0);
		data.push_back(0.0);
		data.push_back(1.0);
		data.push_back(1.0);
		data.push_back(-1.0);
		data.push_back(0.0);
		data.push_back(1.0);
		break;
	default:
		return NULL;
	}

	return new gl3cts::ClipDistance::Utility::VertexBufferObject<glw::GLfloat>(gl, GL_ARRAY_BUFFER, data);
}

/** @brief Check results fetched from framebuffer of functional test.
 *
 *  @param [in] primitive_type      Primitive mode.
 *  @param [in] clip_function       Use #th clip function for gl_ClipDistance[] setup (see m_clip_function[]).
 *  @param [in] results             Array with framebuffer content.
 *
 *  @return True if proper result, false otherwise.
 */
bool gl3cts::ClipDistance::FunctionalTest::checkResults(glw::GLenum primitive_type, glw::GLuint clip_function,
														std::vector<glw::GLfloat>& results)
{
	/* Check for errors */
	if (results.size() == 0)
	{
		return false;
	}

	/* Calculate surface/line integral */
	glw::GLfloat integral = 0.f;

	glw::GLuint increment = (glw::GLuint)((primitive_type == GL_LINES) ?
											  glw::GLuint(sqrt(glw::GLfloat(results.size()))) + 1 /* line integral */ :
											  1 /* surface integral */);
	glw::GLuint base = (glw::GLuint)((primitive_type == GL_LINES) ?
										 glw::GLuint(sqrt(glw::GLfloat(results.size()))) /* line integral */ :
										 results.size() /* surface integral */);

	for (glw::GLuint i_pixels = 0; i_pixels < results.size(); i_pixels += increment)
	{
		integral += results[i_pixels];
	}

	integral /= static_cast<glw::GLfloat>(base);

	/* Check with results' lookup table */
	glw::GLuint i_primitive_type = (primitive_type == GL_POINTS) ? 0 : ((primitive_type == GL_LINES) ? 1 : 2);

	if (fabs(m_expected_integral[i_primitive_type * m_clip_function_count + clip_function] - integral) >
		0.01 /* Precision */)
	{
		return false;
	}

	return true;
}

/* @brief Vertex Shader template for functional tests. */
const glw::GLchar* gl3cts::ClipDistance::FunctionalTest::m_vertex_shader_code = "#version 130\n"
																				"\n"
																				"CLIP_DISTANCE_REDECLARATION"
																				"\n"
																				"CLIP_FUNCTION"
																				"\n"
																				"in vec4 position;\n"
																				"\n"
																				"void main()\n"
																				"{\n"
																				"CLIP_DISTANCE_SETUP"
																				"\n"
																				"    gl_Position  = position;\n"
																				"}\n";

/* @brief Explicit redeclaration key value to preprocess the Vertex Shader template for functional tests. */
const glw::GLchar* gl3cts::ClipDistance::FunctionalTest::m_explicit_redeclaration =
	"out float gl_ClipDistance[CLIP_COUNT];\n";

/* @brief Dynamic array setter key value to preprocess the Vertex Shader template for functional tests. */
const glw::GLchar* gl3cts::ClipDistance::FunctionalTest::m_dynamic_array_setter =
	"    for(int i = 0; i < CLIP_COUNT; i++)\n"
	"    {\n"
	"        gl_ClipDistance[i] = f(i);\n"
	"    }\n";

/* @brief Static array setter key value to preprocess the Vertex Shader template for functional tests. */
const glw::GLchar* gl3cts::ClipDistance::FunctionalTest::m_static_array_setter =
	"    gl_ClipDistance[CLIP_INDEX] = f(CLIP_INDEX);\n";

/* @brief Clip Distance functions to preprocess the Vertex Shader template for functional tests. */
const glw::GLchar* gl3cts::ClipDistance::FunctionalTest::m_clip_function[] = {
	"float f(int i)\n"
	"{\n"
	"    return 0.0;\n"
	"}\n",

	"float f(int i)\n"
	"{\n"
	"    return 0.25 + 0.75 * (float(i) + 1.0) * (float(gl_VertexID) + 1.0) / (float(CLIP_COUNT) * "
	"float(VERTEX_COUNT));\n"
	"}\n",

	"float f(int i)\n"
	"{\n"
	"    return - 0.25 - 0.75 * (float(i) + 1.0) * (float(gl_VertexID) + 1.0) / (float(CLIP_COUNT) * "
	"float(VERTEX_COUNT));\n"
	"}\n",

	/* This case must be last (it is not rendered for GL_POINTS). */
	"#define PI 3.1415926535897932384626433832795\n"
	"\n"
	"float f(int i)\n"
	"{\n"
	"    if(i == 0)\n"
	"    {\n"
	/* This function case generates such series of gl_VertexID:
	 1.0, -1.0              -  for VERTEX_COUNT == 2 aka GL_LINES
	 1.0,  0.0, -1.0        -  for VERTEX_COUNT == 3 aka GL_TRIANGLES
	 and if needed in future:
	 1.0,  0.0, -1.0,  0.0  -  for VERTEX_COUNT == 4 aka GL_QUADS */
	"        return cos( gl_VertexID * PI / ceil( float(VERTEX_COUNT)/2.0 ) );\n"
	"    }\n"
	"\n"
	"    return 0.25 + 0.75 * (float(i) + 1.0) * (float(gl_VertexID) + 1.0) / (float(CLIP_COUNT) * "
	"float(VERTEX_COUNT));\n"
	"}\n"
};

/* @brief Count of Clip Distance functions. */
const glw::GLuint gl3cts::ClipDistance::FunctionalTest::m_clip_function_count =
	static_cast<glw::GLuint>(sizeof(m_clip_function) / sizeof(m_clip_function[0]));

/* @brief Fragment shader source code for functional tests. */
const glw::GLchar* gl3cts::ClipDistance::FunctionalTest::m_fragment_shader_code =
	"#version 130\n"
	"\n"
	"out highp vec4 color;\n"
	"\n"
	"void main()\n"
	"{\n"
	"    color = vec4(1.0, 0.0, 0.0, 1.0);\n"
	"}\n";

/* @brief Primitive modes to be tested in functional test. */
const glw::GLenum gl3cts::ClipDistance::FunctionalTest::m_primitive_types[] = { GL_POINTS, GL_LINES, GL_TRIANGLES };

/* @brief Number of primitive indices for each primitive mode. */
const glw::GLenum gl3cts::ClipDistance::FunctionalTest::m_primitive_indices[] = { 1, 2, 3 };

/* @brief Primitive modes count. */
const glw::GLuint gl3cts::ClipDistance::FunctionalTest::m_primitive_types_count =
	static_cast<glw::GLuint>(sizeof(m_primitive_types) / sizeof(m_primitive_types[0]));

/* @brief Expected results of testing integral for functional test. */
const glw::GLfloat
	gl3cts::ClipDistance::FunctionalTest::m_expected_integral[m_primitive_types_count * m_clip_function_count] = {
		1.0, 1.0, 0.0, 0.0, /* for GL_POINTS    */
		1.0, 1.0, 0.0, 0.5, /* for GL_LINES     */
		0.5, 0.5, 0.0, 0.25 /* for GL_TRIANGLES */
	};

/******************************** Negative Tests Implementation   ********************************/

/** @brief Negative tests constructor.
 *
 *  @param [in] context     OpenGL context.
 */
gl3cts::ClipDistance::NegativeTest::NegativeTest(deqp::Context& context)
	: deqp::TestCase(context, "negative", "Clip Distance Negative Tests")
{
	/* Intentionally left blank */
}

/** @brief Iterate negative tests
 *
 *  @return Iteration result.
 */
tcu::TestNode::IterateResult gl3cts::ClipDistance::NegativeTest::iterate()
{
	/* Shortcut for GL functionality */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Iterate tests */
	bool is_ok	 = true;
	bool may_be_ok = true;

	is_ok	 = is_ok && testClipVertexBuildingErrors(gl);
	is_ok	 = is_ok && testMaxClipDistancesBuildingErrors(gl);
	may_be_ok = may_be_ok && testClipDistancesRedeclarationBuildingErrors(gl);

	/* Result's setup. */
	if (is_ok)
	{
		if (may_be_ok)
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
		}
		else
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_QUALITY_WARNING, "Pass with warning");
		}
	}
	else
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
	}

	return STOP;
}

/** @brief Clip Distance / Clip Vertex negative test sub-case.
 *
 *  @param [in] gl  OpenGL functions' access.
 *
 *  @return True if passed, false otherwise.
 */
bool gl3cts::ClipDistance::NegativeTest::testClipVertexBuildingErrors(const glw::Functions& gl)
{
	/* If OpenGL version < 3.1 is available, check that building shader program
	 fails when vertex shader statically writes to both gl_ClipVertex and
	 gl_ClipDistance[0]. Validate that the vertex shader which statically
	 writes to only the gl_ClipVertex or to the gl_ClipDistance[0] builds
	 without fail. */

	/* This test should only be executed if we're running a GL3.0 or less context */
	if (!glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType(3, 1, glu::PROFILE_CORE)))
	{
		return true;
	}

	gl3cts::ClipDistance::Utility::Program program(gl, m_vertex_shader_code_case_0, m_fragment_shader_code);

	if (program.ProgramStatus().program_id)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Functional test have failed. "
													   "Building shader which statically writes to both gl_ClipVertex "
													   "and gl_ClipDistances[] has unexpectedly succeeded."
						   << tcu::TestLog::EndMessage;

		return false;
	}

	return true;
}

/** @brief Explicit redeclaration negative test sub-case.
 *
 *  @param [in] gl  OpenGL functions' access.
 *
 *  @return True if passed, false otherwise.
 */
bool gl3cts::ClipDistance::NegativeTest::testMaxClipDistancesBuildingErrors(const glw::Functions& gl)
{
	/* Check that building shader program fails when gl_ClipDistance is
	 redeclared in the shader with size higher than GL_MAX_CLIP_DISTANCES. */

	gl3cts::ClipDistance::Utility::Program program(gl, m_vertex_shader_code_case_1, m_fragment_shader_code);

	if (program.ProgramStatus().program_id)
	{
		m_testCtx.getLog() << tcu::TestLog::Message
						   << "Functional test have failed. "
							  "Building shader with explicit redeclaration of gl_ClipDistance[] array with size "
							  "(gl_MaxClipDistances + 1) has unexpectedly succeeded."
						   << tcu::TestLog::EndMessage;

		return false;
	}

	return true;
}

/** @brief Implicit redeclaration negative test sub-case.
 *
 *  @param [in] gl  OpenGL functions' access.
 *
 *  @return True if passed, false when quality warning occured.
 */
bool gl3cts::ClipDistance::NegativeTest::testClipDistancesRedeclarationBuildingErrors(const glw::Functions& gl)
{
	/* Check that building shader program fails when gl_ClipDistance is not
	 redeclared with explicit size and dynamic indexing is used.*/

	gl3cts::ClipDistance::Utility::Program program(gl, m_vertex_shader_code_case_2, m_fragment_shader_code);

	if (program.ProgramStatus().program_id)
	{
		m_testCtx.getLog()
			<< tcu::TestLog::Message
			<< "Functional test have passed but with warning. "
			   "Building shader without explicit redeclaration and with variable indexing has unexpectedly succeeded. "
			   "This is within the bound of the specification (no error is being), but it may lead to errors."
			<< tcu::TestLog::EndMessage;

		return false;
	}

	return true;
}

/** @brief Vertex shader source code for gl_ClipVertex negative test. */
const glw::GLchar* gl3cts::ClipDistance::NegativeTest::m_vertex_shader_code_case_0 =
	"#version 130\n"
	"\n"
	"void main()\n"
	"{\n"
	"    gl_ClipDistance[0] = 0.0;\n"
	"    gl_ClipVertex       = vec4(0.0);\n"
	"    gl_Position         = vec4(1.0);\n"
	"}\n";

/** @brief Vertex shader source code for explicit redeclaration negative test. */
const glw::GLchar* gl3cts::ClipDistance::NegativeTest::m_vertex_shader_code_case_1 =
	"#version 130\n"
	"\n"
	"out float gl_ClipDistance[gl_MaxClipDistances + 1];\n"
	"\n"
	"void main()\n"
	"{\n"
	"    gl_ClipDistance[0] = 0.0;\n"
	"    gl_Position        = vec4(1.0);\n"
	"}\n";

/** @brief Vertex shader source code for impilicit redeclaration negative test. */
const glw::GLchar* gl3cts::ClipDistance::NegativeTest::m_vertex_shader_code_case_2 =
	"#version 130\n"
	"\n"
	"in int count;\n"
	"\n"
	"void main()\n"
	"{\n"
	"    for(int i = 0; i < count; i++)\n"
	"    {\n"
	"        gl_ClipDistance[i] = 0.0;\n"
	"    }\n"
	"\n"
	"    gl_Position = vec4(1.0);\n"
	"}\n";

/** @brief Simple passthrough fragment shader source code for negative tests. */
const glw::GLchar* gl3cts::ClipDistance::NegativeTest::m_fragment_shader_code =
	"#version 130\n"
	"\n"
	"out vec4 color;\n"
	"\n"
	"void main()\n"
	"{\n"
	"    color = vec4(0.0, 0.0, 0.0, 1.0);\n"
	"}\n";

/******************************** Utility Clases Implementations  ********************************/

/** @brief Program constructor.
 *
 *  @param [in] gl                              OpenGL functions' access.
 *  @param [in] vertex_shader_code              Vertex shader source code.
 *  @param [in] fragment_shader_code            Fragment shader source code.
 *  @param [in] transform_feedback_varyings     Transform feedback varying names.
 */
gl3cts::ClipDistance::Utility::Program::Program(const glw::Functions& gl, const std::string& vertex_shader_code,
												const std::string&		 fragment_shader_code,
												std::vector<std::string> transform_feedback_varyings)
	: m_gl(gl)
{
	/* Compilation */
	const glw::GLchar* vertex_shader_code_c   = (const glw::GLchar*)vertex_shader_code.c_str();
	const glw::GLchar* fragment_shader_code_c = (const glw::GLchar*)fragment_shader_code.c_str();

	m_vertex_shader_status   = compileShader(GL_VERTEX_SHADER, &vertex_shader_code_c);
	m_fragment_shader_status = compileShader(GL_FRAGMENT_SHADER, &fragment_shader_code_c);

	/* Linking */
	m_program_status.program_id = 0;
	if (m_vertex_shader_status.shader_compilation_status && m_fragment_shader_status.shader_compilation_status)
	{
		m_program_status = linkShaders(m_vertex_shader_status, m_fragment_shader_status, transform_feedback_varyings);
	}

	/* Cleaning */
	if (m_vertex_shader_status.shader_id)
	{
		m_gl.deleteShader(m_vertex_shader_status.shader_id);

		m_vertex_shader_status.shader_id = 0;
	}

	if (m_fragment_shader_status.shader_id)
	{
		m_gl.deleteShader(m_fragment_shader_status.shader_id);

		m_fragment_shader_status.shader_id = 0;
	}
}

/** @brief Program destructor. */
gl3cts::ClipDistance::Utility::Program::~Program()
{
	if (m_vertex_shader_status.shader_id)
	{
		m_gl.deleteShader(m_vertex_shader_status.shader_id);

		m_vertex_shader_status.shader_id = 0;
	}

	if (m_fragment_shader_status.shader_id)
	{
		m_gl.deleteShader(m_fragment_shader_status.shader_id);

		m_fragment_shader_status.shader_id = 0;
	}

	if (m_program_status.program_id)
	{
		m_gl.deleteProgram(m_program_status.program_id);

		m_program_status.program_id = 0;
	}
}

/** @brief Vertex shader compilation status getter.
 *
 *  @return Vertex shader compilation status.
 */
const gl3cts::ClipDistance::Utility::Program::CompilationStatus& gl3cts::ClipDistance::Utility::Program::
	VertexShaderStatus() const
{
	return m_vertex_shader_status;
}

/** @brief Fragment shader compilation status getter.
 *
 *  @return Fragment shader compilation status.
 */
const gl3cts::ClipDistance::Utility::Program::CompilationStatus& gl3cts::ClipDistance::Utility::Program::
	FragmentShaderStatus() const
{
	return m_fragment_shader_status;
}

/** @brief Program building status getter.
 *
 *  @return Program linkage status.
 */
const gl3cts::ClipDistance::Utility::Program::LinkageStatus& gl3cts::ClipDistance::Utility::Program::ProgramStatus()
	const
{
	return m_program_status;
}

/** @brief Compile shader.
 *
 *  @param [in] shader_type     Shader type.
 *  @param [in] shader_code     Shader source code.
 *
 *  @return Compilation status.
 */
gl3cts::ClipDistance::Utility::Program::CompilationStatus gl3cts::ClipDistance::Utility::Program::compileShader(
	const glw::GLenum shader_type, const glw::GLchar* const* shader_code)
{
	CompilationStatus shader = { 0, GL_NONE, "" };

	if (shader_code != DE_NULL)
	{
		try
		{
			/* Creation */
			shader.shader_id = m_gl.createShader(shader_type);

			GLU_EXPECT_NO_ERROR(m_gl.getError(), "glCreateShader() call failed.");

			/* Compilation */
			m_gl.shaderSource(shader.shader_id, 1, shader_code, NULL);

			GLU_EXPECT_NO_ERROR(m_gl.getError(), "glShaderSource() call failed.");

			m_gl.compileShader(shader.shader_id);

			GLU_EXPECT_NO_ERROR(m_gl.getError(), "glCompileShader() call failed.");

			/* Status */
			m_gl.getShaderiv(shader.shader_id, GL_COMPILE_STATUS, &shader.shader_compilation_status);

			GLU_EXPECT_NO_ERROR(m_gl.getError(), "glGetShaderiv() call failed.");

			/* Logging */
			if (shader.shader_compilation_status == GL_FALSE)
			{
				glw::GLint log_size = 0;

				m_gl.getShaderiv(shader.shader_id, GL_INFO_LOG_LENGTH, &log_size);

				GLU_EXPECT_NO_ERROR(m_gl.getError(), "glGetShaderiv() call failed.");

				if (log_size)
				{
					glw::GLchar* log = new glw::GLchar[log_size];

					if (log)
					{
						memset(log, 0, log_size);

						m_gl.getShaderInfoLog(shader.shader_id, log_size, DE_NULL, log);

						shader.shader_log = log;

						delete[] log;

						GLU_EXPECT_NO_ERROR(m_gl.getError(), "glGetShaderInfoLog() call failed.");
					}
				}
			}
		}
		catch (...)
		{
			if (shader.shader_id)
			{
				m_gl.deleteShader(shader.shader_id);

				shader.shader_id = 0;
			}
		}
	}

	return shader;
}

/** @brief Link compiled shaders.
 *
 *  @param [in] vertex_shader                   Vertex shader compilation status.
 *  @param [in] fragment_shader                 Fragment shader compilation status.
 *  @param [in] transform_feedback_varyings     Transform feedback varying names array.
 *
 *  @return Linkage status.
 */
gl3cts::ClipDistance::Utility::Program::LinkageStatus gl3cts::ClipDistance::Utility::Program::linkShaders(
	const CompilationStatus& vertex_shader, const CompilationStatus& fragment_shader,
	std::vector<std::string>& transform_feedback_varyings)
{
	LinkageStatus program = { 0, GL_NONE, "" };

	if (vertex_shader.shader_id && fragment_shader.shader_id)
	{
		try
		{
			/* Creation */
			program.program_id = m_gl.createProgram();

			GLU_EXPECT_NO_ERROR(m_gl.getError(), "glCreateShader() call failed.");

			if (program.program_id)
			{
				/* Transform Feedback setup */
				for (std::vector<std::string>::iterator i = transform_feedback_varyings.begin();
					 i != transform_feedback_varyings.end(); ++i)
				{
					const glw::GLchar* varying = i->c_str();

					m_gl.transformFeedbackVaryings(program.program_id, 1, &varying, GL_INTERLEAVED_ATTRIBS);

					GLU_EXPECT_NO_ERROR(m_gl.getError(), "glTransformFeedbackVaryings() call failed.");
				}

				/* Linking */
				m_gl.attachShader(program.program_id, vertex_shader.shader_id);

				GLU_EXPECT_NO_ERROR(m_gl.getError(), "glAttachShader() call failed.");

				m_gl.attachShader(program.program_id, fragment_shader.shader_id);

				GLU_EXPECT_NO_ERROR(m_gl.getError(), "glAttachShader() call failed.");

				m_gl.linkProgram(program.program_id);

				GLU_EXPECT_NO_ERROR(m_gl.getError(), "glLinkProgram() call failed.");

				/* Status query */
				m_gl.getProgramiv(program.program_id, GL_LINK_STATUS, &program.program_linkage_status);

				GLU_EXPECT_NO_ERROR(m_gl.getError(), "glGetProgramiv() call failed.");

				/* Logging */
				if (program.program_linkage_status == GL_FALSE)
				{
					glw::GLint log_size = 0;

					m_gl.getProgramiv(program.program_id, GL_INFO_LOG_LENGTH, &log_size);

					GLU_EXPECT_NO_ERROR(m_gl.getError(), "glGetProgramiv() call failed.");

					if (log_size)
					{
						glw::GLchar* log = new glw::GLchar[log_size];

						if (log)
						{
							memset(log, 0, log_size);

							m_gl.getProgramInfoLog(program.program_id, log_size, DE_NULL, log);

							program.program_linkage_log = log;

							delete[] log;

							GLU_EXPECT_NO_ERROR(m_gl.getError(), "glGetProgramInfoLog() call failed.");
						}
					}
				}

				/* Cleanup */
				m_gl.detachShader(program.program_id, vertex_shader.shader_id);

				GLU_EXPECT_NO_ERROR(m_gl.getError(), "glDetachShader() call failed.");

				m_gl.detachShader(program.program_id, fragment_shader.shader_id);

				GLU_EXPECT_NO_ERROR(m_gl.getError(), "glDetachShader() call failed.");

				if (program.program_linkage_status == GL_FALSE)
				{
					m_gl.deleteProgram(program.program_id);

					program.program_id = 0;
				}
			}
		}
		catch (...)
		{
			if (program.program_id)
			{
				m_gl.deleteProgram(program.program_id);

				program.program_id = 0;
			}
		}
	}

	return program;
}

/** @brief Use program for drawing. */
void gl3cts::ClipDistance::Utility::Program::UseProgram() const
{
	m_gl.useProgram(ProgramStatus().program_id);
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "glUseProgram call failed.");
}

/** @brief Framebuffer (GL_32R only) constructor.
 *
 * @param [in] gl           OpenGL functions access.
 * @param [in] size_x       X size of framebuffer.
 * @param [in] size_y       Y size of framebuffer.
 */
gl3cts::ClipDistance::Utility::Framebuffer::Framebuffer(const glw::Functions& gl, const glw::GLsizei size_x,
														const glw::GLsizei size_y)
	: m_gl(gl), m_size_x(size_x), m_size_y(size_y), m_framebuffer_id(0), m_renderbuffer_id(0)
{
	m_gl.genFramebuffers(1, &m_framebuffer_id);
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "glGenFramebuffers call failed.");

	m_gl.genRenderbuffers(1, &m_renderbuffer_id);
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "glGenRenderbuffers call failed.");

	m_gl.bindFramebuffer(GL_FRAMEBUFFER, m_framebuffer_id);
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBindFramebuffer call failed.");

	m_gl.bindRenderbuffer(GL_RENDERBUFFER, m_renderbuffer_id);
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBindRenderbuffer call failed.");

	m_gl.renderbufferStorage(GL_RENDERBUFFER, GL_R32F, m_size_x, m_size_y);
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "glRenderbufferStorage call failed.");

	m_gl.framebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, m_renderbuffer_id);
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "glFramebufferRenderbuffer call failed.");

	if (gl.checkFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		m_gl.deleteFramebuffers(1, &m_framebuffer_id);
		m_framebuffer_id = 0;

		m_gl.deleteRenderbuffers(1, &m_renderbuffer_id);
		m_renderbuffer_id = 0;
		GLU_EXPECT_NO_ERROR(m_gl.getError(), "glDeleteRenderbuffers call failed.");
		GLU_EXPECT_NO_ERROR(m_gl.getError(), "glDeleteFramebuffers call failed.");
	}
}

/** @brief Framebuffer destructor */
gl3cts::ClipDistance::Utility::Framebuffer::~Framebuffer()
{
	if (m_framebuffer_id)
	{
		m_gl.deleteFramebuffers(1, &m_framebuffer_id);
		m_framebuffer_id = 0;
		GLU_EXPECT_NO_ERROR(m_gl.getError(), "glDeleteFramebuffers call failed.");
	}

	if (m_renderbuffer_id)
	{
		m_gl.deleteRenderbuffers(1, &m_renderbuffer_id);
		m_renderbuffer_id = 0;
		GLU_EXPECT_NO_ERROR(m_gl.getError(), "glDeleteRenderbuffers call failed.");
	}
}

/** @brief Check frambuffer completness.
 *
 *  @return True if valid, false otherwise.
 */
bool gl3cts::ClipDistance::Utility::Framebuffer::isValid()
{
	if (m_framebuffer_id)
	{
		return true;
	}

	return false;
}

/** @brief Bind framebuffer and setup viewport. */
void gl3cts::ClipDistance::Utility::Framebuffer::bind()
{
	m_gl.bindFramebuffer(GL_FRAMEBUFFER, m_framebuffer_id);
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBindFramebuffer call failed.");

	m_gl.viewport(0, 0, m_size_x, m_size_y);
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "glViewport call failed.");
}

/** @brief Read pixels from framebuffer.
 *
 *  @return Vector of read pixels.
 */
std::vector<glw::GLfloat> gl3cts::ClipDistance::Utility::Framebuffer::readPixels()
{
	std::vector<glw::GLfloat> pixels(m_size_x * m_size_y);

	if ((m_size_x > 0) && (m_size_y > 0))
	{
		m_gl.readPixels(0, 0, m_size_x, m_size_y, GL_RED, GL_FLOAT, pixels.data());
		GLU_EXPECT_NO_ERROR(m_gl.getError(), "glReadPixels call failed.");
	}

	return pixels;
}

/** @brief Clear framebuffer. */
void gl3cts::ClipDistance::Utility::Framebuffer::clear()
{
	if (isValid())
	{
		m_gl.clearColor(0.f, 0.f, 0.f, 1.f);
		GLU_EXPECT_NO_ERROR(m_gl.getError(), "glClearColor call failed.");

		m_gl.clear(GL_COLOR_BUFFER_BIT);
		GLU_EXPECT_NO_ERROR(m_gl.getError(), "glClear call failed.");
	}
}

/** @brief Vertex array object constructor.
 *
 *  @note It silently binds VAO to OpenGL.
 *
 *  @param [in] gl               OpenGL functions access.
 *  @param [in] primitive_type   Primitive mode.
 */
gl3cts::ClipDistance::Utility::VertexArrayObject::VertexArrayObject(const glw::Functions& gl,
																	const glw::GLenum	 primitive_type)
	: m_gl(gl), m_vertex_array_object_id(0), m_primitive_type(primitive_type)
{
	m_gl.genVertexArrays(1, &m_vertex_array_object_id);
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "glGenVertexArrays call failed.");

	m_gl.bindVertexArray(m_vertex_array_object_id);
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBindVertexArray call failed.");
}

/** @brief Vertex array object destructor. */
gl3cts::ClipDistance::Utility::VertexArrayObject::~VertexArrayObject()
{
	if (m_vertex_array_object_id)
	{
		m_gl.deleteVertexArrays(1, &m_vertex_array_object_id);
		GLU_EXPECT_NO_ERROR(m_gl.getError(), "glDeleteVertexArrays call failed.");
	}
}

/** @brief Bind vertex array object. */
void gl3cts::ClipDistance::Utility::VertexArrayObject::bind()
{
	if (m_vertex_array_object_id)
	{
		m_gl.bindVertexArray(m_vertex_array_object_id);
		GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBindVertexArray call failed.");
	}
}

/** @brief Draw array.
 *
 *  @param [in] first       First index to be drawn.
 *  @param [in] count       Count of indices to be drawn.
 */
void gl3cts::ClipDistance::Utility::VertexArrayObject::draw(glw::GLuint first, glw::GLuint count)
{
	m_gl.drawArrays(m_primitive_type, first, count);
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "glDrawArrays() call failed.");
}

/** @brief Draw array and fetch transform feedback varyings.
 *
 *  @param [in] first                   First index to be drawn.
 *  @param [in] count                   Count of indices to be drawn.
 *  @param [in] discard_rasterizer      Shall we discard rasterizer?
 */
void gl3cts::ClipDistance::Utility::VertexArrayObject::drawWithTransformFeedback(glw::GLuint first, glw::GLuint count,
																				 bool discard_rasterizer)
{
	if (discard_rasterizer)
	{
		m_gl.enable(GL_RASTERIZER_DISCARD);
		GLU_EXPECT_NO_ERROR(m_gl.getError(), "glEnable call failed.");
	}

	m_gl.beginTransformFeedback(GL_POINTS);
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "glBeginTransformFeedback call failed.");

	draw(first, count);

	m_gl.endTransformFeedback();
	GLU_EXPECT_NO_ERROR(m_gl.getError(), "glEndTransformFeedback call failed.");

	if (discard_rasterizer)
	{
		m_gl.disable(GL_RASTERIZER_DISCARD);
		GLU_EXPECT_NO_ERROR(m_gl.getError(), "glDisbale call failed.");
	}
}

/** @brief Substitute key with value within source code.
 *
 *  @param [in] source      Source code to be prerocessed.
 *  @param [in] key         Key to be substituted.
 *  @param [in] value       Value to be inserted.
 *
 *  @return Resulting string.
 */
std::string gl3cts::ClipDistance::Utility::preprocessCode(std::string source, std::string key, std::string value)
{
	std::string destination = source;

	while (true)
	{
		/* Find token in source code. */
		size_t position = destination.find(key, 0);

		/* No more occurences of this key. */
		if (position == std::string::npos)
		{
			break;
		}

		/* Replace token with sub_code. */
		destination.replace(position, key.size(), value);
	}

	return destination;
}

/** @brief Convert an integer to a string.
 *
 *  @param [in] i       Integer to be converted.
 *
 *  @return String representing integer.
 */
std::string gl3cts::ClipDistance::Utility::itoa(glw::GLint i)
{
	std::stringstream stream;

	stream << i;

	return stream.str();
}
