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

/**
 */ /*!
 * \file  gl4cDirectStateAccessProgramPipelinesTests.cpp
 * \brief Conformance tests for the Direct State Access feature functionality (Program Pipelines part).
 */ /*-----------------------------------------------------------------------------------------------------------*/

/* Includes. */
#include "gl4cDirectStateAccessTests.hpp"

#include "deSharedPtr.hpp"

#include "gluContextInfo.hpp"
#include "gluDefs.hpp"
#include "gluPixelTransfer.hpp"
#include "gluStrUtil.hpp"

#include "tcuFuzzyImageCompare.hpp"
#include "tcuImageCompare.hpp"
#include "tcuRenderTarget.hpp"
#include "tcuSurface.hpp"
#include "tcuTestLog.hpp"

#include "glw.h"
#include "glwFunctions.hpp"

namespace gl4cts
{
namespace DirectStateAccess
{
namespace ProgramPipelines
{
/******************************** Creation Test Implementation   ********************************/

/** @brief Creation Test constructor.
 *
 *  @param [in] context     OpenGL context.
 */
CreationTest::CreationTest(deqp::Context& context)
	: deqp::TestCase(context, "program_pipelines_creation", "Program Pipeline Objects Creation Test")
{
	/* Intentionally left blank. */
}

/** @brief Iterate Creation Test cases.
 *
 *  @return Iteration result.
 */
tcu::TestNode::IterateResult CreationTest::iterate()
{
	/* Shortcut for GL functionality */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Get context setup. */
	bool is_at_least_gl_45 = (glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::core(4, 5)));
	bool is_arb_direct_state_access = m_context.getContextInfo().isExtensionSupported("GL_ARB_direct_state_access");

	if ((!is_at_least_gl_45) && (!is_arb_direct_state_access))
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "Not Supported");

		return STOP;
	}

	/* Running tests. */
	bool is_ok	= true;
	bool is_error = false;

	/* Program pipeline objects */
	static const glw::GLuint program_pipelines_count = 2;

	glw::GLuint program_pipelines_legacy[program_pipelines_count] = {};
	glw::GLuint program_pipelines_dsa[program_pipelines_count]	= {};

	try
	{
		/* Check legacy state creation. */
		gl.genProgramPipelines(program_pipelines_count, program_pipelines_legacy);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGenProgramPipelines have failed");

		for (glw::GLuint i = 0; i < program_pipelines_count; ++i)
		{
			if (gl.isProgramPipeline(program_pipelines_legacy[i]))
			{
				is_ok = false;

				/* Log. */
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message
					<< "GenProgramPipelines has created default objects, but it should create only a names."
					<< tcu::TestLog::EndMessage;
			}
		}

		/* Check direct state creation. */
		gl.createProgramPipelines(program_pipelines_count, program_pipelines_dsa);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateProgramPipelines have failed");

		for (glw::GLuint i = 0; i < program_pipelines_count; ++i)
		{
			if (!gl.isProgramPipeline(program_pipelines_dsa[i]))
			{
				is_ok = false;

				/* Log. */
				m_context.getTestContext().getLog() << tcu::TestLog::Message
													<< "CreateProgramPipelines has not created default objects."
													<< tcu::TestLog::EndMessage;
			}
		}
	}
	catch (...)
	{
		is_ok	= false;
		is_error = true;
	}

	/* Cleanup. */
	for (glw::GLuint i = 0; i < program_pipelines_count; ++i)
	{
		if (program_pipelines_legacy[i])
		{
			gl.deleteProgramPipelines(1, &program_pipelines_legacy[i]);

			program_pipelines_legacy[i] = 0;
		}

		if (program_pipelines_dsa[i])
		{
			gl.deleteProgramPipelines(1, &program_pipelines_dsa[i]);

			program_pipelines_dsa[i] = 0;
		}
	}

	/* Result's setup. */
	if (is_ok)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		if (is_error)
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_INTERNAL_ERROR, "Error");
		}
		else
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
		}
	}

	return STOP;
}

/******************************** Defaults Test Implementation   ********************************/

/** @brief Defaults Test constructor.
 *
 *  @param [in] context     OpenGL context.
 */
DefaultsTest::DefaultsTest(deqp::Context& context)
	: deqp::TestCase(context, "program_pipelines_defaults", "Program Pipelines Defaults Test")
	, m_program_pipeline_dsa(0)
{
	/* Intentionally left blank. */
}

/** @brief Iterate Defaults Test cases.
 *
 *  @return Iteration result.
 */
tcu::TestNode::IterateResult DefaultsTest::iterate()
{
	/* Get context setup. */
	bool is_at_least_gl_45 = (glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::core(4, 5)));
	bool is_arb_direct_state_access = m_context.getContextInfo().isExtensionSupported("GL_ARB_direct_state_access");

	if ((!is_at_least_gl_45) && (!is_arb_direct_state_access))
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "Not Supported");

		return STOP;
	}

	/* Running tests. */
	bool is_ok	= true;
	bool is_error = false;

	try
	{
		prepare();

		is_ok &= testProgramPipelineParameter(GL_ACTIVE_PROGRAM, 0);
		is_ok &= testProgramPipelineParameter(GL_VERTEX_SHADER, 0);
		is_ok &= testProgramPipelineParameter(GL_GEOMETRY_SHADER, 0);
		is_ok &= testProgramPipelineParameter(GL_FRAGMENT_SHADER, 0);
		is_ok &= testProgramPipelineParameter(GL_COMPUTE_SHADER, 0);
		is_ok &= testProgramPipelineParameter(GL_TESS_CONTROL_SHADER, 0);
		is_ok &= testProgramPipelineParameter(GL_TESS_EVALUATION_SHADER, 0);
		is_ok &= testProgramPipelineParameter(GL_VALIDATE_STATUS, 0);
		is_ok &= testProgramPipelineParameter(GL_INFO_LOG_LENGTH, 0);

		is_ok &= testProgramPipelineInfoLog(DE_NULL);
	}
	catch (...)
	{
		is_ok	= false;
		is_error = true;
	}

	/* Clean up. */
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
			m_testCtx.setTestResult(QP_TEST_RESULT_INTERNAL_ERROR, "Error");
		}
		else
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
		}
	}

	return STOP;
}

/** @brief Create Program Pipeline Objects.
 *
 *  @note The function may throw if unexpected error has occured.
 *
 *  @return True if test succeeded, false otherwise.
 */
void DefaultsTest::prepare()
{
	/* Shortcut for GL functionality */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Program Pipeline object creation */
	gl.createProgramPipelines(1, &m_program_pipeline_dsa);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateProgramPipelines have failed");
}

/** @brief Test if Program Pipeline Parameter has value as expected.
 *
 *  @note The function may throw if unexpected error has occured.
 *
 *  @param [in] pname           Parameter name enumeration. Must be one of
 *                              GL_ACTIVE_PROGRAM, GL_VERTEX_SHADER, GL_TESS_CONTROL_SHADER,
 *                              GL_TESS_EVALUATION_SHADER, GL_GEOMETRY_SHADER,
 *                              GL_FRAGMENT_SHADER, GL_INFO_LOG_LENGTH.
 *  @param [in] expected_value  Reference value to be compared.
 *
 *  @return True if test succeeded, false otherwise.
 */
bool DefaultsTest::testProgramPipelineParameter(glw::GLenum pname, glw::GLint expected_value)
{
	/* Shortcut for GL functionality */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Get data. */
	glw::GLint value = -1;

	gl.getProgramPipelineiv(m_program_pipeline_dsa, pname, &value);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetProgramPipelineiv have failed");

	if (-1 == value)
	{
		m_context.getTestContext().getLog() << tcu::TestLog::Message << "glGetProgramPipelineiv with parameter "
											<< pname << " has not returned anything and error has not been generated."
											<< tcu::TestLog::EndMessage;

		return false;
	}
	else
	{
		if (expected_value != value)
		{
			m_context.getTestContext().getLog() << tcu::TestLog::Message << "glGetProgramPipelineiv with parameter "
												<< pname << " has returned " << value << ", however " << expected_value
												<< " was expected." << tcu::TestLog::EndMessage;

			return false;
		}
	}

	return true;
}

/** @brief Test if Program Pipeline Parameter has value as expected.
 *
 *  @note The function may throw if unexpected error has occured.
 *
 *  @param [in] expected_value  Reference value to be compared.
 *
 *  @return True if test succeeded, false otherwise.
 */
bool DefaultsTest::testProgramPipelineInfoLog(glw::GLchar* expected_value)
{
	/* Shortcut for GL functionality */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Comparison limit. */
	static const glw::GLsizei max_log_size = 4096;

	/* Storage for data. */
	glw::GLchar log[max_log_size] = { 0 };

	/* Storage fetched length. */
	glw::GLsizei log_size = 0;

	/* Fetch. */
	gl.getProgramPipelineInfoLog(m_program_pipeline_dsa, max_log_size, &log_size, log);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetProgramPipelineInfoLog have failed");

	/* Comparison. */
	if (DE_NULL == expected_value)
	{
		if (0 != log_size)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message
				<< "glGetProgramPipelineInfoLog returned unexpectedly non-empty string: " << log << "."
				<< tcu::TestLog::EndMessage;

			return false;
		}

		return true;
	}

	if (0 != strcmp(log, expected_value))
	{
		m_context.getTestContext().getLog()
			<< tcu::TestLog::Message << "glGetProgramPipelineInfoLog returned string: " << log
			<< ", but is not the same as expected: " << expected_value << "." << tcu::TestLog::EndMessage;

		return false;
	}

	return true;
}

/** @brief Release GL objects.
 */
void DefaultsTest::clean()
{
	/* Shortcut for GL functionality */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (m_program_pipeline_dsa)
	{
		gl.deleteProgramPipelines(1, &m_program_pipeline_dsa);

		m_program_pipeline_dsa = 0;
	}
}

/******************************** Errors Test Implementation   ********************************/

/** @brief Errors Test constructor.
 *
 *  @param [in] context     OpenGL context.
 */
ErrorsTest::ErrorsTest(deqp::Context& context)
	: deqp::TestCase(context, "program_pipelines_errors", "Program Pipeline Objects Errors Test")
{
	/* Intentionally left blank. */
}

/** @brief Iterate Errors Test cases.
 *
 *  @return Iteration result.
 */
tcu::TestNode::IterateResult ErrorsTest::iterate()
{
	/* Shortcut for GL functionality */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Get context setup. */
	bool is_at_least_gl_45 = (glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::core(4, 5)));
	bool is_arb_direct_state_access = m_context.getContextInfo().isExtensionSupported("GL_ARB_direct_state_access");

	if ((!is_at_least_gl_45) && (!is_arb_direct_state_access))
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "Not Supported");

		return STOP;
	}

	/* Running tests. */
	bool is_ok	= true;
	bool is_error = false;

	glw::GLuint program_pipeline_dsa = 0;

	try
	{
		/* Check direct state creation. */
		gl.createProgramPipelines(-1, &program_pipeline_dsa);

		glw::GLenum error = GL_NO_ERROR;

		if (GL_INVALID_VALUE != (error = gl.getError()))
		{
			is_ok = false;

			/* Log. */
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "CreateProgramPipelines has not generated INVALID_VALUE error when callded "
											"with negative number of objects to be created."
				<< "Instead, " << glu::getErrorStr(error) << " error value was generated." << tcu::TestLog::EndMessage;
		}
	}
	catch (...)
	{
		is_ok	= false;
		is_error = true;
	}

	/* Cleanup. */
	if (program_pipeline_dsa)
	{
		gl.deleteProgramPipelines(1, &program_pipeline_dsa);

		program_pipeline_dsa = 0;
	}

	/* Result's setup. */
	if (is_ok)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		if (is_error)
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_INTERNAL_ERROR, "Error");
		}
		else
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
		}
	}

	return STOP;
}

/******************************** Functional Test Implementation   ********************************/

/** @brief Functional Test constructor.
 *
 *  @param [in] context     OpenGL context.
 */
FunctionalTest::FunctionalTest(deqp::Context& context)
	: deqp::TestCase(context, "program_pipelines_functional", "Program Pipeline Objects Functional Test")
	, m_fbo(0)
	, m_rbo(0)
	, m_vao(0)
	, m_spo_v(0)
	, m_spo_f(0)
	, m_ppo(0)
{
	/* Intentionally left blank. */
}

/** @brief Iterate Functional Test cases.
 *
 *  @return Iteration result.
 */
tcu::TestNode::IterateResult FunctionalTest::iterate()
{
	/* Get context setup. */
	bool is_at_least_gl_45 = (glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::core(4, 5)));
	bool is_arb_direct_state_access = m_context.getContextInfo().isExtensionSupported("GL_ARB_direct_state_access");

	if ((!is_at_least_gl_45) && (!is_arb_direct_state_access))
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "Not Supported");

		return STOP;
	}

	/* Running tests. */
	bool is_ok	= true;
	bool is_error = false;

	try
	{
		prepareFramebuffer();
		prepareVertexArrayObject();
		prepareShaderPrograms();
		preparePipeline();
		draw();

		is_ok &= checkFramebufferContent();
	}
	catch (...)
	{
		is_ok	= false;
		is_error = true;
	}

	/* Clean-up. */
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
			m_testCtx.setTestResult(QP_TEST_RESULT_INTERNAL_ERROR, "Error");
		}
		else
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
		}
	}

	return STOP;
}

/** @brief Function prepares framebuffer with RGBA8 color attachment.
 *         Viewport is set up. Content of the framebuffer is cleared.
 *
 *  @note The function may throw if unexpected error has occured.
 */
void FunctionalTest::prepareFramebuffer()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Prepare framebuffer. */
	gl.genFramebuffers(1, &m_fbo);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenFramebuffers call failed.");

	gl.genRenderbuffers(1, &m_rbo);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenRenderbuffers call failed.");

	gl.bindFramebuffer(GL_FRAMEBUFFER, m_fbo);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffer call failed.");

	gl.bindRenderbuffer(GL_RENDERBUFFER, m_rbo);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindRenderbuffer call failed.");

	gl.renderbufferStorage(GL_RENDERBUFFER, GL_RGBA8, 1 /* x size */, 1 /* y size */);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glRenderbufferStorage call failed.");

	gl.framebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, m_rbo);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glFramebufferRenderbuffer call failed.");

	if (gl.checkFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		throw 0;
	}

	gl.viewport(0, 0, 1, 1);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glViewport call failed.");

	/* Clear framebuffer's content. */
	gl.clearColor(0.0f, 0.0f, 0.0f, 0.0f);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glClearColor call failed.");

	gl.clear(GL_COLOR_BUFFER_BIT);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glClear call failed.");
}

/** @brief Function generate and bind empty vertex array object.
 *
 *  @note The function may throw if unexpected error has occured.
 */
void FunctionalTest::prepareVertexArrayObject()
{
	/* Shortcut for GL functionality */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.genVertexArrays(1, &m_vao);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenVertexArrays call failed.");

	gl.bindVertexArray(m_vao);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindVertexArray call failed.");
}

/** @brief Function builds test's GLSL shader program.
 *         If succeded, the program will be set to be used.
 *
 *  @note The function may throw if unexpected error has occured.
 */
void FunctionalTest::prepareShaderPrograms()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Log size limit. */
	static const glw::GLsizei max_log_size = 4096;

	/* Sanity check. */
	if (m_spo_v || m_spo_f)
	{
		throw 0;
	}

	/* Create Vertex Shader Program. */
	m_spo_v = gl.createShaderProgramv(GL_VERTEX_SHADER, 1, &s_vertex_shader);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateShaderProgramv call failed.");

	glw::GLint status = GL_TRUE;

	gl.validateProgram(m_spo_v);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glValidateProgram call failed.");

	gl.getProgramiv(m_spo_v, GL_VALIDATE_STATUS, &status);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetProgramiv call failed.");

	if (GL_FALSE == status)
	{
		/* Storage for data. */
		glw::GLchar log[max_log_size] = { 0 };

		/* Storage fetched length. */
		glw::GLsizei log_size = 0;

		gl.getProgramInfoLog(m_spo_v, max_log_size, &log_size, log);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGetProgramInfoLog call failed.");

		m_context.getTestContext().getLog() << tcu::TestLog::Message
											<< "Vertex shader program building failed with log: " << log
											<< tcu::TestLog::EndMessage;

		throw 0;
	}

	m_spo_f = gl.createShaderProgramv(GL_FRAGMENT_SHADER, 1, &s_fragment_shader);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateShaderProgramv call failed.");

	status = GL_TRUE;

	gl.validateProgram(m_spo_f);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glValidateProgram call failed.");

	gl.getProgramiv(m_spo_f, GL_VALIDATE_STATUS, &status);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetProgramiv call failed.");

	if (GL_FALSE == status)
	{
		/* Storage for data. */
		glw::GLchar log[max_log_size] = { 0 };

		/* Storage fetched length. */
		glw::GLsizei log_size = 0;

		gl.getProgramInfoLog(m_spo_f, max_log_size, &log_size, log);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGetProgramInfoLog call failed.");

		m_context.getTestContext().getLog() << tcu::TestLog::Message
											<< "Fragment shader program building failed with log: " << log
											<< tcu::TestLog::EndMessage;

		throw 0;
	}
}

/** @brief Function prepares program pipeline object.
 *
 *  @note The function may throw if unexpected error has occured.
 */
void FunctionalTest::preparePipeline()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Sanity check. */
	if (m_ppo)
	{
		throw 0;
	}

	/* Create, use and set up program pipeline. */
	gl.createProgramPipelines(1, &m_ppo);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateProgramPipelines call failed.");

	gl.bindProgramPipeline(m_ppo);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindProgramPipeline call failed.");

	gl.useProgramStages(m_ppo, GL_VERTEX_SHADER_BIT, m_spo_v);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgramStages call failed.");

	gl.useProgramStages(m_ppo, GL_FRAGMENT_SHADER_BIT, m_spo_f);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgramStages call failed.");
}

/** @brief Function draws a quad.
 *
 *  @note The function may throw if unexpected error has occured.
 */
void FunctionalTest::draw()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.drawArrays(GL_TRIANGLE_STRIP, 0, 4);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawArrays have failed");
}

/** @brief Check content of the framebuffer and compare it with expected data.
 *
 *  @note The function may throw if unexpected error has occured.
 *
 *  @return True if succeeded, false otherwise.
 */
bool FunctionalTest::checkFramebufferContent()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Fetch framebuffer data. */
	glw::GLubyte pixel[4] = { 0 };

	gl.readPixels(0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glReadPixels have failed");

	/* Comparison with expected values. */
	if ((255 != pixel[0]) || (0 != pixel[1]) || (0 != pixel[2]) || (255 != pixel[3]))
	{
		m_context.getTestContext().getLog()
			<< tcu::TestLog::Message << "Frameuffer content (" << (unsigned int)pixel[0] << ", "
			<< (unsigned int)pixel[1] << ", " << (unsigned int)pixel[2] << ", " << (unsigned int)pixel[3]
			<< ") is different than expected (255, 0, 0, 255)." << tcu::TestLog::EndMessage;

		return false;
	}

	return true;
}

/** @brief Release all GL objects.
 */
void FunctionalTest::clean()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Release framebuffer. */
	if (m_fbo)
	{
		gl.deleteFramebuffers(1, &m_fbo);

		m_fbo = 0;
	}

	/* Release renderbuffer. */
	if (m_rbo)
	{
		gl.deleteRenderbuffers(1, &m_rbo);

		m_rbo = 0;
	}

	/* Release vertex array object. */
	if (m_vao)
	{
		gl.deleteVertexArrays(1, &m_vao);

		m_vao = 0;
	}

	/* Release shader programs. */
	if (m_spo_v)
	{
		gl.deleteProgram(m_spo_v);

		m_spo_v = 0;
	}

	if (m_spo_f)
	{
		gl.deleteProgram(m_spo_f);

		m_spo_f = 0;
	}

	/* Release program pipelines. */
	if (m_ppo)
	{
		gl.bindProgramPipeline(0);

		gl.deleteProgramPipelines(1, &m_ppo);

		m_ppo = 0;
	}
}

/* Vertex shader source code. */
const glw::GLchar* FunctionalTest::s_vertex_shader = "#version 450\n"
													 "\n"
													 "out gl_PerVertex\n"
													 "{\n"
													 "    vec4  gl_Position;\n"
													 "    float gl_PointSize;\n"
													 "    float gl_ClipDistance[];\n"
													 "};\n"
													 "\n"
													 "void main()\n"
													 "{\n"
													 "    switch(gl_VertexID)\n"
													 "    {\n"
													 "        case 0:\n"
													 "            gl_Position = vec4(-1.0, 1.0, 0.0, 1.0);\n"
													 "            break;\n"
													 "        case 1:\n"
													 "            gl_Position = vec4( 1.0, 1.0, 0.0, 1.0);\n"
													 "            break;\n"
													 "        case 2:\n"
													 "            gl_Position = vec4(-1.0,-1.0, 0.0, 1.0);\n"
													 "            break;\n"
													 "        case 3:\n"
													 "            gl_Position = vec4( 1.0,-1.0, 0.0, 1.0);\n"
													 "            break;\n"
													 "    }\n"
													 "}\n";

/* Fragment shader source program. */
const glw::GLchar* FunctionalTest::s_fragment_shader = "#version 450\n"
													   "\n"
													   "out vec4 color;\n"
													   "\n"
													   "void main()\n"
													   "{\n"
													   "    color = vec4(1.0, 0.0, 0.0, 1.0);\n"
													   "}\n";

} /* ProgramPipelines namespace. */
} /* DirectStateAccess namespace. */
} /* gl4cts namespace. */
