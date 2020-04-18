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
 * \file  gl4cDirectStateAccessXFBTests.cpp
 * \brief Conformance tests for the Direct State Access feature functionality (Transform Feedbeck access part).
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
namespace Samplers
{
/******************************** Creation Test Implementation   ********************************/

/** @brief Creation Test constructor.
 *
 *  @param [in] context     OpenGL context.
 */
CreationTest::CreationTest(deqp::Context& context)
	: deqp::TestCase(context, "samplers_creation", "Sampler Objects Creation Test")
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

	/* Samplers objects */
	static const glw::GLuint samplers_count = 2;

	glw::GLuint samplers_dsa[samplers_count] = {};

	try
	{
		/* Check direct state creation. */
		gl.createSamplers(samplers_count, samplers_dsa);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateSamplers have failed");

		for (glw::GLuint i = 0; i < samplers_count; ++i)
		{
			if (!gl.isSampler(samplers_dsa[i]))
			{
				is_ok = false;

				/* Log. */
				m_context.getTestContext().getLog() << tcu::TestLog::Message
													<< "CreateSamplers has not created defualt objects."
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
	for (glw::GLuint i = 0; i < samplers_count; ++i)
	{
		if (samplers_dsa[i])
		{
			gl.deleteSamplers(1, &samplers_dsa[i]);

			samplers_dsa[i] = 0;
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
	: deqp::TestCase(context, "samplers_defaults", "Samplers Defaults Test"), m_sampler_dsa(0)
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

		glw::GLfloat expected_vector[4] = { 0.0, 0.0, 0.0, 0.0 };

		is_ok &= testSamplerFloatVectorParameter(GL_TEXTURE_BORDER_COLOR, expected_vector);
		is_ok &= testSamplerIntegerParameter(GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
		is_ok &= testSamplerIntegerParameter(GL_TEXTURE_COMPARE_MODE, GL_NONE);
		is_ok &= testSamplerFloatParameter(GL_TEXTURE_LOD_BIAS, 0.0);
		is_ok &= testSamplerFloatParameter(GL_TEXTURE_MAX_LOD, 1000);
		is_ok &= testSamplerIntegerParameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		is_ok &= testSamplerIntegerParameter(GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
		is_ok &= testSamplerFloatParameter(GL_TEXTURE_MIN_LOD, -1000);
		is_ok &= testSamplerIntegerParameter(GL_TEXTURE_WRAP_S, GL_REPEAT);
		is_ok &= testSamplerIntegerParameter(GL_TEXTURE_WRAP_T, GL_REPEAT);
		is_ok &= testSamplerIntegerParameter(GL_TEXTURE_WRAP_R, GL_REPEAT);
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

/** @brief Create Sampler Objects.
 *
 *  @note The function may throw if unexpected error has occured.
 *
 *  @return True if test succeeded, false otherwise.
 */
void DefaultsTest::prepare()
{
	/* Shortcut for GL functionality */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Sampler object creation */
	gl.createSamplers(1, &m_sampler_dsa);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateTransformFeedbacks have failed");
}

/** @brief Test Sampler Integer Parameter.
 *
 *  @note The function may throw if unexpected error has occured.
 *
 *  @return True if test succeeded, false otherwise.
 */
bool DefaultsTest::testSamplerIntegerParameter(glw::GLenum pname, glw::GLint expected_value)
{
	/* Shortcut for GL functionality */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Get data. */
	glw::GLint value = -1;

	gl.getSamplerParameteriv(m_sampler_dsa, pname, &value);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetSamplerParameteriv have failed");

	if (-1 == value)
	{
		m_context.getTestContext().getLog()
			<< tcu::TestLog::Message << "glGetSamplerParameteriv with parameter " << glu::getTextureParameterName(pname)
			<< " has not returned anything and error has not been generated." << tcu::TestLog::EndMessage;

		return false;
	}
	else
	{
		if (expected_value != value)
		{
			m_context.getTestContext().getLog() << tcu::TestLog::Message << "glGetSamplerParameteriv with parameter "
												<< glu::getTextureParameterName(pname) << " has returned " << value
												<< ", however " << expected_value << " was expected."
												<< tcu::TestLog::EndMessage;

			return false;
		}
	}

	return true;
}

/** @brief Test Sampler Float Parameter.
 *
 *  @note The function may throw if unexpected error has occured.
 *
 *  @return True if test succeeded, false otherwise.
 */
bool DefaultsTest::testSamplerFloatParameter(glw::GLenum pname, glw::GLfloat expected_value)
{
	/* Shortcut for GL functionality */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Get data. */
	glw::GLfloat value = -1.0;

	gl.getSamplerParameterfv(m_sampler_dsa, pname, &value);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetSamplerParameteriv have failed");

	if (de::abs(expected_value - value) > 0.015625f /* Precision */)
	{
		m_context.getTestContext().getLog() << tcu::TestLog::Message << "glGetSamplerParameterfv with parameter "
											<< glu::getTextureParameterName(pname) << " has returned " << value
											<< ", however " << expected_value << " was expected."
											<< tcu::TestLog::EndMessage;

		return false;
	}

	return true;
}

/** @brief Test Sampler Float Parameter.
 *
 *  @note The function may throw if unexpected error has occured.
 *
 *  @return True if test succeeded, false otherwise.
 */
bool DefaultsTest::testSamplerFloatVectorParameter(glw::GLenum pname, glw::GLfloat expected_value[4])
{
	/* Shortcut for GL functionality */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Get data. */
	glw::GLfloat value[4] = { -1.0, -1.0, -1.0, -1.0 };

	gl.getSamplerParameterfv(m_sampler_dsa, pname, value);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetSamplerParameterfv have failed");

	if ((de::abs(expected_value[0] - value[0]) > 0.015625f /* Precision */) ||
		(de::abs(expected_value[1] - value[1]) > 0.015625f /* Precision */) ||
		(de::abs(expected_value[2] - value[2]) > 0.015625f /* Precision */) ||
		(de::abs(expected_value[3] - value[3]) > 0.015625f /* Precision */))
	{
		m_context.getTestContext().getLog() << tcu::TestLog::Message << "glGetSamplerParameterfv with parameter "
											<< glu::getTextureParameterName(pname) << " has returned [" << value[0]
											<< ", " << value[1] << ", " << value[2] << ", " << value[3] << "], however "
											<< expected_value << " was expected." << tcu::TestLog::EndMessage;

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

	if (m_sampler_dsa)
	{
		gl.deleteSamplers(1, &m_sampler_dsa);

		m_sampler_dsa = 0;
	}
}

/******************************** Errors Test Implementation   ********************************/

/** @brief Errors Test constructor.
 *
 *  @param [in] context     OpenGL context.
 */
ErrorsTest::ErrorsTest(deqp::Context& context) : deqp::TestCase(context, "samplers_errors", "Samplers Errors Test")
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

	glw::GLuint sampler_dsa = 0;

	try
	{
		/* Check direct state creation. */
		gl.createSamplers(-1, &sampler_dsa);

		glw::GLenum error = GL_NO_ERROR;

		if (GL_INVALID_VALUE != (error = gl.getError()))
		{
			is_ok = false;

			/* Log. */
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "CreateSamplers has not generated INVALID_VALUE error when callded with "
											"negative number of samplers to be created."
				<< "Instead, " << glu::getErrorStr(error) << " error value was generated." << tcu::TestLog::EndMessage;
		}
	}
	catch (...)
	{
		is_ok	= false;
		is_error = true;
	}

	/* Cleanup. */
	if (sampler_dsa)
	{
		gl.deleteSamplers(1, &sampler_dsa);

		sampler_dsa = 0;
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
	: deqp::TestCase(context, "samplers_functional", "Samplers Functional Test")
	, m_fbo(0)
	, m_rbo(0)
	, m_vao(0)
	, m_to(0)
	, m_so(0)
	, m_po(0)
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
		prepareProgram();
		prepareTexture();
		prepareSampler();
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

/** @brief Function builds test's GLSL program.
 *         If succeded, the program will be set to be used.
 *
 *  @note The function may throw if unexpected error has occured.
 */
void FunctionalTest::prepareProgram()
{
	/* Shortcut for GL functionality */
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
					glw::GLint log_size = 0;
					gl.getShaderiv(shader[i].id, GL_INFO_LOG_LENGTH, &log_size);
					GLU_EXPECT_NO_ERROR(gl.getError(), "glGetShaderiv call failed.");

					glw::GLchar* log_text = new glw::GLchar[log_size];

					gl.getShaderInfoLog(shader[i].id, log_size, NULL, &log_text[0]);

					m_context.getTestContext().getLog() << tcu::TestLog::Message << "Shader compilation has failed.\n"
														<< "Shader type: " << glu::getShaderTypeStr(shader[i].type)
														<< "\n"
														<< "Shader compilation error log:\n"
														<< log_text << "\n"
														<< "Shader source code:\n"
														<< shader[i].source << "\n"
														<< tcu::TestLog::EndMessage;

					delete[] log_text;

					GLU_EXPECT_NO_ERROR(gl.getError(), "glGetShaderInfoLog call failed.");

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
			glw::GLint log_size = 0;

			gl.getProgramiv(m_po, GL_INFO_LOG_LENGTH, &log_size);

			GLU_EXPECT_NO_ERROR(gl.getError(), "glGetProgramiv call failed.");

			glw::GLchar* log_text = new glw::GLchar[log_size];

			gl.getProgramInfoLog(m_po, log_size, NULL, &log_text[0]);

			m_context.getTestContext().getLog() << tcu::TestLog::Message << "Program linkage has failed due to:\n"
												<< log_text << "\n"
												<< tcu::TestLog::EndMessage;

			delete[] log_text;

			GLU_EXPECT_NO_ERROR(gl.getError(), "glGetProgramInfoLog call failed.");

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

	if (m_po)
	{
		gl.useProgram(m_po);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram call failed.");
	}

	if (0 == m_po)
	{
		throw 0;
	}
}

/** @brief Function prepares texture object with test's data.
 *
 *  @note The function may throw if unexpected error has occured.
 */
void FunctionalTest::prepareTexture()
{
	/* Shortcut for GL functionality */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.activeTexture(GL_TEXTURE0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glActiveTexture call failed.");

	/* Texture creation and binding. */
	gl.genTextures(1, &m_to);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenTextures call failed.");

	gl.bindTexture(GL_TEXTURE_2D, m_to);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture call failed.");

	/* Uploading texture. */

	gl.texImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 2, 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, s_texture_data);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTexImage2D call failed.");

	/* Setup fragment shader's sampler. */
	glw::GLint location = 0;

	gl.getUniformLocation(m_po, s_uniform_sampler);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetUniformLocation call failed.");

	gl.uniform1i(location, 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUniform1i call failed.");
}

/** @brief Function prepares sampler with test setup and binds it to unit 0.
 *
 *  @note The function may throw if unexpected error has occured.
 */
void FunctionalTest::prepareSampler()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Sampler creation and setup. */
	gl.createSamplers(1, &m_so);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateTransformFeedbacks have failed");

	gl.bindSampler(0, m_so);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindSampler have failed");

	gl.samplerParameteri(m_so, GL_TEXTURE_WRAP_S, GL_REPEAT);
	gl.samplerParameteri(m_so, GL_TEXTURE_WRAP_T, GL_REPEAT);
	gl.samplerParameteri(m_so, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	gl.samplerParameteri(m_so, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glSamplerParameteri have failed");
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
	if ((s_texture_data[0] != pixel[0]) || (s_texture_data[1] != pixel[1]) || (s_texture_data[2] != pixel[2]) ||
		(s_texture_data[3] != pixel[3]))
	{
		m_context.getTestContext().getLog()
			<< tcu::TestLog::Message << "Frameuffer content (" << (unsigned int)pixel[0] << ", "
			<< (unsigned int)pixel[1] << ", " << (unsigned int)pixel[2] << ", " << (unsigned int)pixel[3]
			<< ") is different than expected (" << (unsigned int)s_texture_data[0] << ", "
			<< (unsigned int)s_texture_data[1] << ", " << (unsigned int)s_texture_data[2] << ", "
			<< (unsigned int)s_texture_data[3] << ")." << tcu::TestLog::EndMessage;

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

	/* Release texture. */
	if (m_to)
	{
		gl.deleteTextures(1, &m_to);

		m_to = 0;
	}

	/* Release sampler. */
	if (m_so)
	{
		gl.deleteSamplers(1, &m_so);

		m_so = 0;
	}

	/* Release GLSL program. */
	if (m_po)
	{
		gl.useProgram(0);

		gl.deleteProgram(m_po);

		m_po = 0;
	}
}

/* Vertex shader source code. */
const glw::GLchar FunctionalTest::s_vertex_shader[] = "#version 330\n"
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
const glw::GLchar FunctionalTest::s_fragment_shader[] =
	"#version 330\n"
	"\n"
	"uniform sampler2D texture_sampler;\n"
	"\n"
	"out vec4 color;\n"
	"\n"
	"void main()\n"
	"{\n"
	"    color = texture(texture_sampler, vec2(0.33333, 0.33333));\n"
	"}\n";

/* Name of texture sampler uniform. */
const glw::GLchar FunctionalTest::s_uniform_sampler[] = "texture_sampler";

/* Test's texture data. */
const glw::GLubyte FunctionalTest::s_texture_data[] = {
	0xFF, 0x00, 0x00, 0xFF, /* RED  */
	0x00, 0xFF, 0x00, 0xFF, /* GREEN  */
	0x00, 0x00, 0xFF, 0xFF, /* BLUE */
	0xFF, 0xFF, 0x00, 0xFF  /* YELLOW */
};

} /* Samplers namespace. */
} /* DirectStateAccess namespace. */
} /* gl4cts namespace. */
