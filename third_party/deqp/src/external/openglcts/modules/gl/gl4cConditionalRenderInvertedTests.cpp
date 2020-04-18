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
#include "gl4cConditionalRenderInvertedTests.hpp"
#include "gluContextInfo.hpp"
#include "gluDefs.hpp"
#include "gluRenderContext.hpp"
#include "gluStrUtil.hpp"
#include "tcuTestLog.hpp"

/******************************** Test Group Implementation       ********************************/

/** @brief Context Flush Control tests group constructor.
 *
 *  @param [in] context     OpenGL context.
 */
gl4cts::ConditionalRenderInverted::Tests::Tests(deqp::Context& context)
	: TestCaseGroup(context, "conditional_render_inverted", "Conditional Render Inverted Test Suite")
{
	/* Intentionally left blank */
}

/** @brief Context Flush Control tests initializer. */
void gl4cts::ConditionalRenderInverted::Tests::init()
{
	addChild(new gl4cts::ConditionalRenderInverted::CoverageTest(m_context));
	addChild(new gl4cts::ConditionalRenderInverted::FunctionalTest(m_context));
}

/******************************** Coverage Tests Implementation   ********************************/

/** @brief API coverage tests constructor.
 *
 *  @param [in] context     OpenGL context.
 */
gl4cts::ConditionalRenderInverted::CoverageTest::CoverageTest(deqp::Context& context)
	: deqp::TestCase(context, "coverage", "Conditional Render Inverted Coverage Test"), m_qo_id(0)
{
	/* Intentionally left blank. */
}

/** @brief Iterate API coverage tests.
 *
 *  @return Iteration result.
 */
tcu::TestNode::IterateResult gl4cts::ConditionalRenderInverted::CoverageTest::iterate()
{
	/* OpenGL support query. */
	bool is_at_least_gl_45 = (glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::core(4, 5)));
	bool is_arb_conditional_render_inverted =
		m_context.getContextInfo().isExtensionSupported("GL_ARB_conditional_render_inverted");

	/* Running tests. */
	bool is_ok	= true;
	bool is_error = false;

	/* This test should only be executed if we're running a GL4.5 context or related extension is available */
	try
	{
		if (is_at_least_gl_45 || is_arb_conditional_render_inverted)
		{
			/* Prepare common objects. */
			createQueryObject();

			/* Test cases. */
			static const glw::GLenum modes[] = { GL_QUERY_WAIT_INVERTED, GL_QUERY_NO_WAIT_INVERTED,
												 GL_QUERY_BY_REGION_WAIT_INVERTED,
												 GL_QUERY_BY_REGION_NO_WAIT_INVERTED };

			static const glw::GLuint modes_count = sizeof(modes) / sizeof(modes[0]);

			/* Iterate over the test cases. */
			for (glw::GLuint i = 0; i < modes_count; ++i)
			{
				is_ok &= test(modes[i]);
			}
		}
	}
	catch (...)
	{
		is_ok	= false;
		is_error = true;
	}

	/* Cleanup. */
	clean();

	/* Result's setup. */
	if (is_ok)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		if (is_error)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message
				<< "Internal error has occured during Conditional Render Inverted Coverage Test."
				<< tcu::TestLog::EndMessage;

			m_testCtx.setTestResult(QP_TEST_RESULT_INTERNAL_ERROR, "Test error.");
		}
		else
		{
			m_context.getTestContext().getLog() << tcu::TestLog::Message
												<< "The Conditional Render Inverted Coverage Test has failed."
												<< tcu::TestLog::EndMessage;

			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
		}
	}

	return STOP;
}

/** @brief Create query object.
 */
void gl4cts::ConditionalRenderInverted::CoverageTest::createQueryObject()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Create valid query object. */
	gl.genQueries(1, &m_qo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenQueries() call failed.");

	gl.beginQuery(GL_SAMPLES_PASSED, m_qo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBeginQuery() call failed.");

	gl.endQuery(GL_SAMPLES_PASSED);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glEndQuery() call failed.");
}

/** @brief Clean query object and error values.
 */
void gl4cts::ConditionalRenderInverted::CoverageTest::clean()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Clean query object. */
	gl.deleteQueries(1, &m_qo_id);

	m_qo_id = 0;

	/* Make sure no errors are left. */
	while (gl.getError())
		;
}

/** @brief Test that glBeginConditionalRender accept mode.
 *
 *  @param [in] mode    Render condition mode.
 *
 *  @return True if glBeginConditionalRender did not generate an error, false otherwise.
 */
bool gl4cts::ConditionalRenderInverted::CoverageTest::test(glw::GLenum mode)
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Default return value;*/
	bool is_no_error = true;

	/* Test. */
	gl.beginConditionalRender(m_qo_id, mode);

	while (GL_NO_ERROR != gl.getError())
	{
		is_no_error = false;
	}

	/* Clean up. */
	if (is_no_error)
	{
		gl.endConditionalRender();
		GLU_EXPECT_NO_ERROR(gl.getError(), "glEndConditionalRender() call failed.");
	}

	/* Logging. */
	if (!is_no_error)
	{
		m_context.getTestContext().getLog() << tcu::TestLog::Message
											<< "glBeginConditionalRender failed when used with mode "
											<< Utilities::modeToChars(mode) << "." << tcu::TestLog::EndMessage;
	}

	/* Return test result. */
	return is_no_error;
}

/******************************** Functional Test Implementation   ********************************/

/** @brief Functional test constructor.
 *
 *  @param [in] context     OpenGL context.
 */
gl4cts::ConditionalRenderInverted::FunctionalTest::FunctionalTest(deqp::Context& context)
	: deqp::TestCase(context, "functional", "Conditional Render Inverted Functional Test")
	, m_fbo_id(0)
	, m_rbo_id(0)
	, m_vao_id(0)
	, m_po_id(0)
	, m_qo_id(0)
{
	/* Intentionally left blank. */
}

/** @brief Iterate Functional test cases.
 *
 *  @return Iteration result.
 */
tcu::TestNode::IterateResult gl4cts::ConditionalRenderInverted::FunctionalTest::iterate()
{
	/* OpenGL support query. */
	bool is_at_least_gl_45 = (glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::core(4, 5)));
	bool is_arb_conditional_render_inverted =
		m_context.getContextInfo().isExtensionSupported("GL_ARB_conditional_render_inverted");

	/* Running tests. */
	bool is_ok	= true;
	bool is_error = false;

	/* This test should only be executed if we're running a GL4.5 context or related extension is available */
	try
	{
		if (is_at_least_gl_45 || is_arb_conditional_render_inverted)
		{
			/* Test cases. */
			static const bool render_cases[] = { false, true };

			static const glw::GLuint render_cases_count = sizeof(render_cases) / sizeof(render_cases[0]);

			static const glw::GLenum query_cases[] = { GL_SAMPLES_PASSED, GL_ANY_SAMPLES_PASSED };

			static const glw::GLuint query_cases_count = sizeof(query_cases) / sizeof(query_cases[0]);

			static const glw::GLenum modes[] = { GL_QUERY_WAIT_INVERTED, GL_QUERY_NO_WAIT_INVERTED,
												 GL_QUERY_BY_REGION_WAIT_INVERTED,
												 GL_QUERY_BY_REGION_NO_WAIT_INVERTED };

			static const glw::GLuint modes_count = sizeof(modes) / sizeof(modes[0]);

			/* Creating common objects. */
			createProgram();
			createView();
			createVertexArrayObject();

			/* Iterating over test cases. */
			for (glw::GLuint i = 0; i < render_cases_count; ++i)
			{
				for (glw::GLuint j = 0; j < query_cases_count; ++j)
				{
					for (glw::GLuint k = 0; k < modes_count; ++k)
					{
						createQueryObject();

						setupColor(1.f);
						setupPassSwitch(render_cases[i]);
						clearView();
						draw(false, query_cases[j]);

						if (render_cases[i] == fragmentsPassed())
						{
							setupColor(0.f);
							setupPassSwitch(true);
							draw(true, modes[k]);

							glw::GLfloat expected_value = (render_cases[i]) ? 1.f : 0.f;
							glw::GLfloat resulted_value = readPixel();

							if (de::abs(expected_value - resulted_value) > 0.0078125f /* Precission (1/128) */)
							{
								m_context.getTestContext().getLog()
									<< tcu::TestLog::Message << "The functional test's expected value ("
									<< expected_value << ") is different than resulted value (" << resulted_value
									<< "). The tested mode was " << Utilities::modeToChars(modes[k])
									<< ". Query was done for target " << Utilities::queryTargetToChars(query_cases[j])
									<< ", and the test was prepared to " << ((render_cases[i]) ? "pass" : "discard")
									<< " all fragments." << tcu::TestLog::EndMessage;

								is_ok = false;
							}
						}
						else
						{
							is_ok = false;
						}

						cleanQueryObject();
					}
				}
			}
		}
	}
	catch (...)
	{
		is_ok	= false;
		is_error = true;

		cleanQueryObject();
	}

	/* Clean-up. */
	cleanProgramViewAndVAO();

	/* Result's setup. */
	if (is_ok)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		if (is_error)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message
				<< "Internal error has occured during Conditional Render Inverted Functional Test."
				<< tcu::TestLog::EndMessage;

			m_testCtx.setTestResult(QP_TEST_RESULT_INTERNAL_ERROR, "Test error.");
		}
		else
		{
			m_context.getTestContext().getLog() << tcu::TestLog::Message
												<< "The Conditional Render Inverted Functional Test has failed."
												<< tcu::TestLog::EndMessage;

			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail.");
		}
	}

	return STOP;
}

/** @brief Compile and link test's GLSL program.
 */
void gl4cts::ConditionalRenderInverted::FunctionalTest::createProgram()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	struct Shader
	{
		glw::GLchar const* const source;
		glw::GLenum const		 type;
		glw::GLuint				 id;
	} shader[] = { { s_vertex_shader, GL_VERTEX_SHADER, 0 }, { s_fragment_shader, GL_FRAGMENT_SHADER, 0 } };

	glw::GLuint const shader_count = sizeof(shader) / sizeof(shader[0]);

	try
	{
		/* Create program. */
		m_po_id = gl.createProgram();
		GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateProgram call failed.");

		/* Shader compilation. */

		for (glw::GLuint i = 0; i < shader_count; ++i)
		{
			if (DE_NULL != shader[i].source)
			{
				shader[i].id = gl.createShader(shader[i].type);

				GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateShader call failed.");

				gl.attachShader(m_po_id, shader[i].id);

				GLU_EXPECT_NO_ERROR(gl.getError(), "glAttachShader call failed.");

				gl.shaderSource(shader[i].id, 1, &(shader[i].source), NULL);

				GLU_EXPECT_NO_ERROR(gl.getError(), "glShaderSource call failed.");

				gl.compileShader(shader[i].id);

				GLU_EXPECT_NO_ERROR(gl.getError(), "glCompileShader call failed.");

				glw::GLint status = GL_FALSE;

				gl.getShaderiv(shader[i].id, GL_COMPILE_STATUS, &status);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glGetShaderiv call failed.");

				if (GL_FALSE == status)
				{
					throw 0;
				}
			}
		}

		/* Link. */
		gl.linkProgram(m_po_id);

		GLU_EXPECT_NO_ERROR(gl.getError(), "glTransformFeedbackVaryings call failed.");

		glw::GLint status = GL_FALSE;

		gl.getProgramiv(m_po_id, GL_LINK_STATUS, &status);

		if (GL_TRUE == status)
		{
			for (glw::GLuint i = 0; i < shader_count; ++i)
			{
				if (shader[i].id)
				{
					gl.detachShader(m_po_id, shader[i].id);

					GLU_EXPECT_NO_ERROR(gl.getError(), "glDetachShader call failed.");
				}
			}
		}
		else
		{
			throw 0;
		}
	}
	catch (...)
	{
		if (m_po_id)
		{
			gl.deleteProgram(m_po_id);

			m_po_id = 0;
		}
	}

	for (glw::GLuint i = 0; i < shader_count; ++i)
	{
		if (0 != shader[i].id)
		{
			gl.deleteShader(shader[i].id);

			shader[i].id = 0;
		}
	}

	if (m_po_id)
	{
		gl.useProgram(m_po_id);

		GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram call failed.");
	}
}

/** @brief Create and bind framebuffer with renderbuffer color attachment.
 */
void gl4cts::ConditionalRenderInverted::FunctionalTest::createView()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Prepare framebuffer. */
	gl.clearColor(0.5f, 0.5f, 0.5f, 0.5f);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glClearColor call failed.");

	gl.genFramebuffers(1, &m_fbo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenFramebuffers call failed.");

	gl.genRenderbuffers(1, &m_rbo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenRenderbuffers call failed.");

	gl.bindFramebuffer(GL_FRAMEBUFFER, m_fbo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffer call failed.");

	gl.bindRenderbuffer(GL_RENDERBUFFER, m_rbo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindRenderbuffer call failed.");

	gl.renderbufferStorage(GL_RENDERBUFFER, GL_R8, s_view_size, s_view_size);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glRenderbufferStorage call failed.");

	gl.framebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, m_rbo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glFramebufferRenderbuffer call failed.");

	if (gl.checkFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		throw 0;
	}

	gl.viewport(0, 0, s_view_size, s_view_size);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glViewport call failed.");
}

/** @brief Create test's query object.
 */
void gl4cts::ConditionalRenderInverted::FunctionalTest::createQueryObject()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Create valid query object. */
	gl.genQueries(1, &m_qo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenQueries() call failed.");
}

/** @brief Setup color uniform of the test's program.
 */
void gl4cts::ConditionalRenderInverted::FunctionalTest::setupColor(const glw::GLfloat red)
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Fetch where to set. */
	glw::GLuint location = gl.getUniformLocation(m_po_id, s_color_uniform_name);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetUniformLocation() call failed.");

	/* Set. */
	gl.uniform1f(location, red);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUniform1f() call failed.");
}

/** @brief Setup pass or discard switch uniform of the test's program.
 */
void gl4cts::ConditionalRenderInverted::FunctionalTest::setupPassSwitch(const bool shall_pass)
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Fetch where to set. */
	glw::GLuint location = gl.getUniformLocation(m_po_id, s_pass_switch_uniform_name);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetUniformLocation() call failed.");

	/* Set. */
	gl.uniform1i(location, (int)shall_pass);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUniform1f() call failed.");
}

/** @brief Draw full screen within query or conditional block.
 *
 *  @param [in] conditional_or_query_draw       If true draw will be done in conditional rendering block, otherwise in query block.
 *  @param [in] condition_mode_or_query_target  The param needed by query or conditional block - target or mode.
 */
void gl4cts::ConditionalRenderInverted::FunctionalTest::draw(const bool		   conditional_or_query_draw,
															 const glw::GLenum condition_mode_or_query_target)
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (conditional_or_query_draw)
	{
		gl.beginConditionalRender(m_qo_id, condition_mode_or_query_target);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBeginConditionalRender() call failed.");
	}
	else
	{
		gl.beginQuery(condition_mode_or_query_target, m_qo_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBeginQuery() call failed.");
	}

	gl.drawArrays(GL_TRIANGLE_STRIP, 0, 4);

	if (conditional_or_query_draw)
	{
		gl.endConditionalRender();
		GLU_EXPECT_NO_ERROR(gl.getError(), "glEndConditionalRender() call failed.");
	}
	else
	{
		gl.endQuery(condition_mode_or_query_target);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glEndQuery() call failed.");
	}
}

/** @brief Check if any fragments have passed rendering.
 *
 *  @return True if any sample passed, false otherwise.
 */
bool gl4cts::ConditionalRenderInverted::FunctionalTest::fragmentsPassed()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Fetch result. */
	glw::GLint result = -1;

	gl.getQueryObjectiv(m_qo_id, GL_QUERY_RESULT, &result);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetQueryObjectiv() call failed.");

	/* Check for unusual errors. */
	if (-1 == result)
	{
		throw 0;
	}

	/* Return results. */
	return (result > 0);
}

/** @brief Read framebuffer's first pixel red component (left, bottom).
 *
 *  @return Red value of the pixel.
 */
glw::GLfloat gl4cts::ConditionalRenderInverted::FunctionalTest::readPixel()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	glw::GLfloat red = -1.f;

	gl.readPixels(0, 0, 1, 1, GL_RED, GL_FLOAT, &red);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glReadPixels() call failed.");

	return red;
}

/** @brief Destroy test's query object.
 */
void gl4cts::ConditionalRenderInverted::FunctionalTest::cleanQueryObject()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Clean query object. */
	if (m_qo_id)
	{
		gl.deleteQueries(1, &m_qo_id);

		m_qo_id = 0;
	}
}

/** @brief Create test's empty Vertex Array Object.
 */
void gl4cts::ConditionalRenderInverted::FunctionalTest::createVertexArrayObject()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Create and bind vertex array. */
	gl.genVertexArrays(1, &m_vao_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenVertexArrays() call failed.");

	gl.bindVertexArray(m_vao_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindVertexArray() call failed.");
}

/** @brief Destroy test's Vertex Array Object.
 */
void gl4cts::ConditionalRenderInverted::FunctionalTest::cleanProgramViewAndVAO()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Deleting view. */
	if (m_fbo_id)
	{
		gl.deleteFramebuffers(1, &m_fbo_id);

		m_fbo_id = 0;
	}

	if (m_rbo_id)
	{
		gl.deleteRenderbuffers(1, &m_rbo_id);

		m_rbo_id = 0;
	}

	if (m_vao_id)
	{
		gl.deleteVertexArrays(1, &m_vao_id);

		m_vao_id = 0;
	}
}

/** @brief Destroy test's framebuffer with related objects.
 */
void gl4cts::ConditionalRenderInverted::FunctionalTest::clearView()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Clear screen. */
	gl.clear(GL_COLOR_BUFFER_BIT);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glClear() call failed.");
}

const glw::GLchar gl4cts::ConditionalRenderInverted::FunctionalTest::s_vertex_shader[] =
	"#version 130\n"
	"\n"
	"void main()\n"
	"{\n"
	"    switch(gl_VertexID % 4)\n"
	"    {\n"
	"    case 0:\n"
	"       gl_Position = vec4(-1.0, -1.0,  0.0,  1.0);\n"
	"       break;\n"
	"    case 1:\n"
	"       gl_Position = vec4( 1.0, -1.0,  0.0,  1.0);\n"
	"       break;\n"
	"    case 2:\n"
	"       gl_Position = vec4(-1.0,  1.0,  0.0,  1.0);\n"
	"       break;\n"
	"    case 3:\n"
	"       gl_Position = vec4( 1.0,  1.0,  0.0,  1.0);\n"
	"       break;\n"
	"    }\n"
	"}\n";

const glw::GLchar gl4cts::ConditionalRenderInverted::FunctionalTest::s_fragment_shader[] = "#version 130\n"
																						   "\n"
																						   "uniform float color;\n"
																						   "uniform int   shall_pass;\n"
																						   "\n"
																						   "out vec4 pixel;\n"
																						   "\n"
																						   "void main()\n"
																						   "{\n"
																						   "    if(0 == shall_pass)\n"
																						   "    {\n"
																						   "        discard;\n"
																						   "    }\n"
																						   "\n"
																						   "    pixel = vec4(color);\n"
																						   "}\n";

const glw::GLchar gl4cts::ConditionalRenderInverted::FunctionalTest::s_color_uniform_name[] = "color";

const glw::GLchar gl4cts::ConditionalRenderInverted::FunctionalTest::s_pass_switch_uniform_name[] = "shall_pass";

const glw::GLuint gl4cts::ConditionalRenderInverted::FunctionalTest::s_view_size = 1;

/******************************** Utilities Implementation   ********************************/

/** @brief Return string representation of condional rendering mode.
 *
 *  @param [in] mode    Render condition mode.
 *
 *  @return Constant C-String representation of mode.
 */
const glw::GLchar* gl4cts::ConditionalRenderInverted::Utilities::modeToChars(glw::GLenum mode)
{
	/* Const name values. */
	static const glw::GLchar* query_wait_inverted_mode_name				 = "GL_QUERY_WAIT_INVERTED";
	static const glw::GLchar* query_no_wait_inverted_mode_name			 = "GL_QUERY_NO_WAIT_INVERTED";
	static const glw::GLchar* query_by_region_wait_inverted_mode_name	= "GL_QUERY_BY_REGION_WAIT_INVERTED";
	static const glw::GLchar* query_by_region_no_wait_inverted_mode_name = "GL_QUERY_BY_REGION_NO_WAIT_INVERTED";
	static const glw::GLchar* invalid_mode_name							 = "unknow mode";

	/* Return proper value. */
	if (GL_QUERY_WAIT_INVERTED == mode)
	{
		return query_wait_inverted_mode_name;
	}

	if (GL_QUERY_NO_WAIT_INVERTED == mode)
	{
		return query_no_wait_inverted_mode_name;
	}

	if (GL_QUERY_BY_REGION_WAIT_INVERTED == mode)
	{
		return query_by_region_wait_inverted_mode_name;
	}

	if (GL_QUERY_BY_REGION_NO_WAIT_INVERTED == mode)
	{
		return query_by_region_no_wait_inverted_mode_name;
	}

	/* If not, return invalid name. */
	return invalid_mode_name;
}

/** @brief Return string representation of glBeginQuery's target.
 *
 *  @param [in] mode    Render condition mode.
 *
 *  @return Constant C-String representation of mode.
 */
const glw::GLchar* gl4cts::ConditionalRenderInverted::Utilities::queryTargetToChars(glw::GLenum mode)
{
	/* Const name values. */
	static const glw::GLchar* any_samples_name	= "GL_ANY_SAMPLES_PASSED";
	static const glw::GLchar* samples_name		  = "GL_SAMPLES_PASSED";
	static const glw::GLchar* invalid_target_name = "unknow mode";

	/* Return proper value. */
	if (GL_ANY_SAMPLES_PASSED == mode)
	{
		return any_samples_name;
	}

	if (GL_SAMPLES_PASSED == mode)
	{
		return samples_name;
	}

	/* If not, return invalid name. */
	return invalid_target_name;
}
