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
#include "gl4cContextFlushControlTests.hpp"
#include "deClock.h"
#include "gluContextInfo.hpp"
#include "gluDefs.hpp"
#include "gluPlatform.hpp"
#include "gluRenderContext.hpp"
#include "gluStrUtil.hpp"
#include "tcuTestLog.hpp"

#ifndef GL_CONTEXT_RELEASE_BEHAVIOR
#define GL_CONTEXT_RELEASE_BEHAVIOR 0x82FB
#endif

#ifndef GL_CONTEXT_RELEASE_BEHAVIOR_FLUSH
#define GL_CONTEXT_RELEASE_BEHAVIOR_FLUSH 0x82FC
#endif

#define CONTEXT_FLUSH_CONTROL_FUNCTIONAL_TEST_DRAW_COUNT 1024

/******************************** Test Group Implementation       ********************************/

/** @brief Context Flush Control tests group constructor.
 *
 *  @param [in] context     OpenGL context.
 */
gl4cts::ContextFlushControl::Tests::Tests(deqp::Context& context)
	: TestCaseGroup(context, "context_flush_control", "Context Flush Control Test Suite")
{
	/* Intentionally left blank */
}

/** @brief Context Flush Control tests initializer. */
void gl4cts::ContextFlushControl::Tests::init()
{
	addChild(new gl4cts::ContextFlushControl::CoverageTest(m_context));
	addChild(new gl4cts::ContextFlushControl::FunctionalTest(m_context));
}

/******************************** Coverage Tests Implementation   ********************************/

/** @brief API coverage tests constructor.
 *
 *  @param [in] context     OpenGL context.
 */
gl4cts::ContextFlushControl::CoverageTest::CoverageTest(deqp::Context& context)
	: deqp::TestCase(context, "coverage", "Context Flush Control API Coverage Test")
{
	/* Intentionally left blank. */
}

/** @brief Iterate API coverage tests.
 *
 *  @return Iteration result.
 */
tcu::TestNode::IterateResult gl4cts::ContextFlushControl::CoverageTest::iterate()
{
	/* OpenGL support query. */
	bool is_at_least_gl_44 = (glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::core(4, 4)));
	bool is_khr_context_flush_control = m_context.getContextInfo().isExtensionSupported("GL_KHR_context_flush_control");

	/* Running tests. */
	bool is_ok = true;

	/* This test should only be executed if we're running a GL4.4 context or related extension is available */
	if (is_at_least_gl_44 || is_khr_context_flush_control)
	{
		/* Test deafult context which shall use implicit flush when swapped. */
		is_ok = is_ok && testQuery(m_context.getRenderContext(), GL_CONTEXT_RELEASE_BEHAVIOR_FLUSH);

		/* Create context which shall swap without flush. */
		glu::RenderContext* no_flush_context = createNoFlushContext();

		/* Proceed only if such context has been created. */
		if (DE_NULL != no_flush_context)
		{
			/* Test no-flush context. */
			no_flush_context->makeCurrent();

			is_ok = is_ok && testQuery(*no_flush_context, GL_NONE);

			/* Release no-flush context. */
			m_context.getRenderContext().makeCurrent();

			delete no_flush_context;
		}
	}

	/* Result's setup. */
	if (is_ok)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		m_context.getTestContext().getLog() << tcu::TestLog::Message
											<< "The Context Flush Control Coverage test have failed."
											<< tcu::TestLog::EndMessage;

		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
	}

	return STOP;
}

/** @brief Test getter coverage for the given GL context.
 *
 *  This function tests following GL query functions:
 *      glGetIntegerv,
 *      glGetFloatv,
 *      glGetBooleanv,
 *      glGetDoublev,
 *      glGetInteger64v.
 *  Expected value is clamped to <0, 1> range flor glGetBooleanv.
 *  For reference see KHR_context_flush_control extension.
 *
 *  @param [in] context         Render context to be used with the test.
 *  @param [in] expected_value  Expected value to be returned by glGet*v function.
 *
 *  @return True if all tested functions returned expected value, false otherwise.
 */
bool gl4cts::ContextFlushControl::CoverageTest::testQuery(glu::RenderContext& context, glw::GLenum expected_value)
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = context.getFunctions();

	/* Variables for query. */
	glw::GLint	 value_i   = -1;
	glw::GLint64   value_i64 = -1;
	glw::GLfloat   value_f   = -1;
	glw::GLdouble  value_d   = -1;
	glw::GLboolean value_b   = -1;

	glw::GLboolean expected_bool_value = (glw::GLboolean)de::min((glw::GLint)expected_value, (glw::GLint)1);

	/* Test. */
	try
	{
		/* Fetch data. */
		gl.getIntegerv(GL_CONTEXT_RELEASE_BEHAVIOR, &value_i);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv call failed.");

		gl.getInteger64v(GL_CONTEXT_RELEASE_BEHAVIOR, &value_i64);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGetInteger64v call failed.");

		gl.getFloatv(GL_CONTEXT_RELEASE_BEHAVIOR, &value_f);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGetFloatv call failed.");

		gl.getDoublev(GL_CONTEXT_RELEASE_BEHAVIOR, &value_d);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGetDoublev call failed.");

		gl.getBooleanv(GL_CONTEXT_RELEASE_BEHAVIOR, &value_b);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGetBooleanv call failed.");

		/* Check result. */
		if ((expected_value == value_i) && (expected_value == value_i64) && (expected_value == value_f) &&
			(expected_value == value_d) && (expected_bool_value == value_b))
		{
			return true;
		}
	}
	catch (...)
	{
		return false;
	}

	return false;
}

/** @brief Create render context with CONTEXT_RELEASE_BEHAVIOR set as NONE.
 *
 *  @return Render context pointer if creation succeeded, DE_NULL otherwise.
 */
glu::RenderContext* gl4cts::ContextFlushControl::CoverageTest::createNoFlushContext()
{
	/* Get current platform.*/
	glu::Platform& platform = dynamic_cast<glu::Platform&>(m_context.getTestContext().getPlatform());

	/* Context to be returned (NULL if failed). */
	glu::RenderContext* context = DE_NULL;

	/* Get context related attributes needed to create no-flush context. */
	const int* attributes = platform.getContextFlushControlContextAttributes();

	/* Proceed only if it is possible to make no-flush context. */
	if (DE_NULL != attributes)
	{
		glu::ContextType renderContextType = m_context.getRenderContext().getType();

		/* Create no-flush context. */
		context = platform.createRenderContext(renderContextType, m_context.getTestContext().getCommandLine(),
											   0 /* shared_context */, attributes);
	}

	return context;
}

/******************************** Functional Test Implementation   ********************************/

/** @brief Functional test constructor.
 *
 *  @param [in] context     OpenGL context.
 */
gl4cts::ContextFlushControl::FunctionalTest::FunctionalTest(deqp::Context& context)
	: deqp::TestCase(context, "functional", "Context Flush Control Functional Test")
{
	/* Intentionally left blank. */
}

/** @brief Iterate Functional test cases.
 *
 *  @return Iteration result.
 */
tcu::TestNode::IterateResult gl4cts::ContextFlushControl::FunctionalTest::iterate()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Get current platform.*/
	glu::Platform& platform = dynamic_cast<glu::Platform&>(m_context.getTestContext().getPlatform());

	/* OpenGL support query. */
	bool is_at_least_gl_44 = (glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::core(4, 4)));
	bool is_khr_context_flush_control = m_context.getContextInfo().isExtensionSupported("GL_KHR_context_flush_control");

	/* Running tests. */
	bool is_ok	= true;
	bool is_error = false;

	/* This test should only be executed if we're running a GL4.4 context or related extension is available */
	try
	{
		if ((is_at_least_gl_44 || is_khr_context_flush_control) &&
			(DE_NULL != platform.getContextFlushControlContextAttributes()))
		{
			glw::GLfloat test_time_no_flush = testTime(false);
			glw::GLfloat test_time_flush	= testTime(true);

			is_ok = (test_time_no_flush < test_time_flush);
		}
		else
		{
			is_error = true;
		}
	}
	catch (...)
	{
		is_ok	= false;
		is_error = true;
	}

	/* Result's setup. */
	if (is_ok)
	{
		if (is_error)
		{
			m_context.getTestContext().getLog() << tcu::TestLog::Message
												<< "The context does not support No-Flush behavior."
												<< tcu::TestLog::EndMessage;

			m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "Not supported.");
		}
		else
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
		}
	}
	else
	{
		if (is_error)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "Internal error has occured during Context Flush Control Functional test."
				<< tcu::TestLog::EndMessage;

			m_testCtx.setTestResult(QP_TEST_RESULT_INTERNAL_ERROR, "Test error.");
		}
		else
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "The running time of no-flush context switches has been slower than flush "
											"behavior context switching case. "
				<< "This is not expected from quality implementation." << tcu::TestLog::EndMessage;

			m_testCtx.setTestResult(QP_TEST_RESULT_QUALITY_WARNING, "Quality warning.");
		}
	}

	return STOP;
}

/** @brief This function measures time of loop consisting of draw and switch context,
 *         which shall or shall not flush on switch.
 *
 *  The test is based on KHR_context_flush_control extension overview, that the main reason
 *  for no-flush context is to increase the performance of the implementation.
 *
 *  @param [in] shall_flush_on_release      Flag indicating that contexts shall flush when switched.
 *
 *  @return Run-time of the test loop.
 */
glw::GLfloat gl4cts::ContextFlushControl::FunctionalTest::testTime(bool shall_flush_on_release)
{
	/* Create two contexts to be switched during test. */
	DrawSetup draw_context_setup[2] = { DrawSetup(m_context, shall_flush_on_release),
										DrawSetup(m_context, shall_flush_on_release) };

	/* Check starting time. */
	deUint64 start_time = deGetMicroseconds();

	/* Loop over draw-switch context. */
	for (glw::GLuint i = 0; i < 1024; ++i)
	{
		draw_context_setup[i % 2].makeCurrent();
		draw_context_setup[i % 2].draw();
	}

	/* Check end time. */
	deUint64 end_time = deGetMicroseconds();

	/* Return resulting run-time. */
	return (glw::GLfloat)(end_time - start_time);
}

/** @brief Make context current.
 */
void gl4cts::ContextFlushControl::FunctionalTest::DrawSetup::makeCurrent()
{
	/* Switch context to this. */
	m_context->makeCurrent();
}

/** @brief Use program and draw full screen quad.
 */
void gl4cts::ContextFlushControl::FunctionalTest::DrawSetup::draw()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context->getFunctions();

	/* Use GLSL program. */
	gl.useProgram(m_po);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram call failed.");

	/* Clear. */
	gl.clear(GL_COLOR_BUFFER_BIT);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glClear call failed.");

	/* Draw. */
	gl.drawArrays(GL_TRIANGLE_STRIP, 0, 4 /* quad */);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawArrays call failed.");
}

/** @brief Draw Setup object constructor.
 *
 *  The constructor will throw on error.
 *
 *  @param [in] test_context                Test context for platform, logging and switching to default context on destruction.
 *  @param [in] shall_flush_on_release      Flag indicating that contexts shall flush when switched.
 */
gl4cts::ContextFlushControl::FunctionalTest::DrawSetup::DrawSetup(deqp::Context& test_context,
																  bool			 shall_flush_on_release)
	: m_test_context(test_context), m_fbo(0), m_rbo(0), m_vao(0), m_po(0)
{
	createContext(shall_flush_on_release);

	if (DE_NULL == m_context)
	{
		throw 0;
	}

	createGeometry();

	if (0 == m_vao)
	{
		throw 0;
	}

	createView();

	if ((0 == m_fbo) || (0 == m_rbo))
	{
		throw 0;
	}

	createProgram();

	if (0 == m_po)
	{
		throw 0;
	}
}

/** @brief Draw Setup object destructor.
 */
gl4cts::ContextFlushControl::FunctionalTest::DrawSetup::~DrawSetup()
{
	if (m_context)
	{
		/* Make sure context is current. */
		makeCurrent();

		/* Shortcut for GL functionality. */
		const glw::Functions& gl = m_context->getFunctions();

		/* Cleanup. */
		if (m_vao)
		{
			gl.deleteVertexArrays(1, &m_vao);

			m_vao = 0;
		}

		if (m_fbo)
		{
			gl.deleteFramebuffers(1, &m_fbo);

			m_fbo = 0;
		}

		if (m_rbo)
		{
			gl.deleteRenderbuffers(1, &m_rbo);

			m_rbo = 0;
		}

		if (m_po)
		{
			gl.deleteProgram(m_po);

			m_po = 0;
		}

		/* Make default context current. */
		m_test_context.getRenderContext().makeCurrent();

		/* Cleanup context. */
		delete m_context;
	}
}

/** @brief Create render context with CONTEXT_RELEASE_BEHAVIOR set to NONE or CONTEXT_RELEASE_BEHAVIOR_FLUSH.
 *
 *  @return Render context pointer if creation succeeded, DE_NULL otherwise.
 */
void gl4cts::ContextFlushControl::FunctionalTest::DrawSetup::createContext(bool shall_flush_on_release)
{
	/* Get current platform.*/
	glu::Platform& platform = dynamic_cast<glu::Platform&>(m_test_context.getTestContext().getPlatform());

	/* Get context related attributes needed to create no-flush context. */
	const int* attributes = DE_NULL;

	if (!shall_flush_on_release)
	{
		attributes = platform.getContextFlushControlContextAttributes();
	}

	/* Proceed only if it is possible to make no-flush context. */
	glu::ContextType renderContextType = m_test_context.getRenderContext().getType();

	/* Create no-flush context. */
	m_context = platform.createRenderContext(renderContextType, m_test_context.getTestContext().getCommandLine(),
											 0 /* shared_context */, attributes);
}

/** @brief Create RGBA8 framebuffer with attached renderbuffer.
 */
void gl4cts::ContextFlushControl::FunctionalTest::DrawSetup::createView()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context->getFunctions();

	/* Prepare framebuffer. */
	gl.clearColor(0.f, 0.f, 0.f, 1.f);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glClearColor call failed.");

	gl.genFramebuffers(1, &m_fbo);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenFramebuffers call failed.");

	gl.genRenderbuffers(1, &m_rbo);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenRenderbuffers call failed.");

	gl.bindFramebuffer(GL_FRAMEBUFFER, m_fbo);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffer call failed.");

	gl.bindRenderbuffer(GL_RENDERBUFFER, m_rbo);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindRenderbuffer call failed.");

	gl.renderbufferStorage(GL_RENDERBUFFER, GL_RGBA8, s_view_size, s_view_size);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glRenderbufferStorage call failed.");

	gl.framebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, m_rbo);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glFramebufferRenderbuffer call failed.");

	if (gl.checkFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		throw 0;
	}

	gl.viewport(0, 0, s_view_size, s_view_size);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glViewport call failed.");
}

/** @brief Create and bind empty vertex array object.
 */
void gl4cts::ContextFlushControl::FunctionalTest::DrawSetup::createGeometry()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context->getFunctions();

	/* Create and bind vertex array. */
	gl.genVertexArrays(1, &m_vao);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenVertexArrays call failed.");

	gl.bindVertexArray(m_vao);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindVertexArray call failed.");
}

/** @brief Compile and link shader program.
 */
void gl4cts::ContextFlushControl::FunctionalTest::DrawSetup::createProgram()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context->getFunctions();

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
		m_po = gl.createProgram();
		GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateProgram call failed.");

		/* Shader compilation. */

		for (glw::GLuint i = 0; i < shader_count; ++i)
		{
			if (DE_NULL != shader[i].source)
			{
				shader[i].id = gl.createShader(shader[i].type);

				GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateShader call failed.");

				gl.attachShader(m_po, shader[i].id);

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
		gl.linkProgram(m_po);

		GLU_EXPECT_NO_ERROR(gl.getError(), "glTransformFeedbackVaryings call failed.");

		glw::GLint status = GL_FALSE;

		gl.getProgramiv(m_po, GL_LINK_STATUS, &status);

		if (GL_TRUE == status)
		{
			for (glw::GLuint i = 0; i < shader_count; ++i)
			{
				if (shader[i].id)
				{
					gl.detachShader(m_po, shader[i].id);

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
		if (m_po)
		{
			gl.deleteProgram(m_po);

			m_po = 0;
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
}

const glw::GLuint gl4cts::ContextFlushControl::FunctionalTest::DrawSetup::s_view_size = 256;

const glw::GLchar gl4cts::ContextFlushControl::FunctionalTest::DrawSetup::s_vertex_shader[] =
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

const glw::GLchar gl4cts::ContextFlushControl::FunctionalTest::DrawSetup::s_fragment_shader[] =
	"#version 130\n"
	"\n"
	"out vec4 pixel;\n"
	"\n"
	"void main()\n"
	"{\n"
	"    pixel = vec4(1.0);\n"
	"}\n";
