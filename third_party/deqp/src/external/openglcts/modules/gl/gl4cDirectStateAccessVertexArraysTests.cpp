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
 * \file  gl4cDirectStateAccessVertexArraysTests.cpp
 * \brief Conformance tests for the Direct State Access feature functionality (Vertex Array Objects access part).
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

#include <algorithm>
#include <climits>
#include <cmath>
#include <set>
#include <sstream>
#include <stack>

namespace gl4cts
{
namespace DirectStateAccess
{
namespace VertexArrays
{
/******************************** Creation Test Implementation   ********************************/

/** @brief Creation Test constructor.
 *
 *  @param [in] context     OpenGL context.
 */
CreationTest::CreationTest(deqp::Context& context)
	: deqp::TestCase(context, "vertex_arrays_creation", "Vertex Array Objects Creation Test")
{
	/* Intentionally left blank. */
}

/** @brief Iterate Creation Test cases.
 *
 *  @return Iteration result.
 */
tcu::TestNode::IterateResult CreationTest::iterate()
{
	/* Shortcut for GL functionality. */
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

	/* VertexArrays' objects */
	static const glw::GLuint vertex_arrays_count = 2;

	glw::GLuint vertex_arrays_legacy[vertex_arrays_count] = {};
	glw::GLuint vertex_arrays_dsa[vertex_arrays_count]	= {};

	try
	{
		/* Check legacy state creation. */
		gl.genVertexArrays(vertex_arrays_count, vertex_arrays_legacy);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGenVertexArrays have failed");

		for (glw::GLuint i = 0; i < vertex_arrays_count; ++i)
		{
			if (gl.isVertexArray(vertex_arrays_legacy[i]))
			{
				is_ok = false;

				/* Log. */
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message
					<< "GenVertexArrays has created default objects, but it should create only a names."
					<< tcu::TestLog::EndMessage;
			}
		}

		/* Check direct state creation. */
		gl.createVertexArrays(vertex_arrays_count, vertex_arrays_dsa);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateVertexArrays have failed");

		for (glw::GLuint i = 0; i < vertex_arrays_count; ++i)
		{
			if (!gl.isVertexArray(vertex_arrays_dsa[i]))
			{
				is_ok = false;

				/* Log. */
				m_context.getTestContext().getLog() << tcu::TestLog::Message
													<< "CreateVertexArrays has not created default objects."
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
	for (glw::GLuint i = 0; i < vertex_arrays_count; ++i)
	{
		if (vertex_arrays_legacy[i])
		{
			gl.deleteVertexArrays(1, &vertex_arrays_legacy[i]);

			vertex_arrays_legacy[i] = 0;
		}

		if (vertex_arrays_dsa[i])
		{
			gl.deleteVertexArrays(1, &vertex_arrays_dsa[i]);

			vertex_arrays_dsa[i] = 0;
		}
	}

	/* Errors clean up. */
	while (gl.getError())
		;

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

/******************************** Vertex Array Object Enable Disable Attributes Test Implementation   ********************************/

/** @brief Vertex Array Object Enable Disable Attributes Test constructor.
 *
 *  @param [in] context     OpenGL context.
 */
EnableDisableAttributesTest::EnableDisableAttributesTest(deqp::Context& context)
	: deqp::TestCase(context, "vertex_arrays_enable_disable_attributes",
					 "Vertex Array Objects Enable Disable Attributes Test")
	, m_po_even(0)
	, m_po_odd(0)
	, m_vao(0)
	, m_bo(0)
	, m_bo_xfb(0)
	, m_max_attributes(16) /* Required Minimum: OpenGL 4.5 Table 23.57: Implementation Dependent Vertex Shader Limits */
{
	/* Intentionally left blank. */
}

/** @brief Iterate Vertex Array Object Enable Disable Attributes Test cases.
 *
 *  @return Iteration result.
 */
tcu::TestNode::IterateResult EnableDisableAttributesTest::iterate()
{
	/* Shortcut for GL functionality. */
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

	try
	{
		gl.getIntegerv(GL_MAX_VERTEX_ATTRIBS, &m_max_attributes);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, ...) have failed");

		m_po_even = PrepareProgram(false);
		m_po_odd  = PrepareProgram(true);

		PrepareVAO();
		PrepareXFB();

		is_ok &= TurnOnAttributes(true, false);
		is_ok &= DrawAndCheck(false);

		is_ok &= TurnOnAttributes(false, true);
		is_ok &= DrawAndCheck(true);
	}
	catch (...)
	{
		is_ok	= false;
		is_error = true;
	}

	/* Cleanup. */
	Clean();

	/* Errors clean up. */
	while (gl.getError())
		;

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

glw::GLuint EnableDisableAttributesTest::PrepareProgram(const bool bind_even_or_odd)
{
	/* Preprocess vertex shader sources. */
	std::string declarations = "";
	std::string copies		 = "    sum = 0;\n";

	for (glw::GLint i = (glw::GLint)(bind_even_or_odd); i < m_max_attributes; i += 2)
	{
		declarations.append((std::string("in int a_").append(Utilities::itoa(i))).append(";\n"));
		copies.append((std::string("    sum += a_").append(Utilities::itoa(i))).append(";\n"));
	}

	std::string vs_template(s_vertex_shader_template);

	std::string vs_source = Utilities::replace(vs_template, "DECLARATION_TEMPLATE", declarations);
	vs_source			  = Utilities::replace(vs_source, "COPY_TEMPLATE", copies);

	/* Build and compile. */
	return BuildProgram(vs_source.c_str(), bind_even_or_odd);
}

/** @brief Build test's GLSL program.
 *
 *  @note The function may throw if unexpected error has occured.
 */
glw::GLuint EnableDisableAttributesTest::BuildProgram(const char* vertex_shader, const bool bind_even_or_odd)
{
	/* Shortcut for GL functionality */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	struct Shader
	{
		glw::GLchar const* const source;
		glw::GLenum const		 type;
		glw::GLuint				 id;
	} shader[] = { { vertex_shader, GL_VERTEX_SHADER, 0 }, { s_fragment_shader, GL_FRAGMENT_SHADER, 0 } };

	glw::GLuint const shader_count = sizeof(shader) / sizeof(shader[0]);

	glw::GLuint po = 0;

	try
	{
		/* Create program. */
		po = gl.createProgram();
		GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateProgram call failed.");

		/* Shader compilation. */

		for (glw::GLuint i = 0; i < shader_count; ++i)
		{
			if (DE_NULL != shader[i].source)
			{
				shader[i].id = gl.createShader(shader[i].type);

				GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateShader call failed.");

				gl.attachShader(po, shader[i].id);

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

		/* Transform Feedback setup. */
		static const glw::GLchar* xfb_varying = "sum";

		gl.transformFeedbackVaryings(po, 1, &xfb_varying, GL_INTERLEAVED_ATTRIBS);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glTransformFeedbackVaryings call failed.");

		for (glw::GLint i = (glw::GLint)(bind_even_or_odd); i < m_max_attributes; i += 2)
		{
			std::string attribute = std::string("a_").append(Utilities::itoa(i));

			gl.bindAttribLocation(po, i, attribute.c_str());
			GLU_EXPECT_NO_ERROR(gl.getError(), "glBindAttribLocation call failed.");
		}

		/* Link. */
		gl.linkProgram(po);

		GLU_EXPECT_NO_ERROR(gl.getError(), "glTransformFeedbackVaryings call failed.");

		glw::GLint status = GL_FALSE;

		gl.getProgramiv(po, GL_LINK_STATUS, &status);

		if (GL_TRUE == status)
		{
			for (glw::GLuint i = 0; i < shader_count; ++i)
			{
				if (shader[i].id)
				{
					gl.detachShader(po, shader[i].id);

					GLU_EXPECT_NO_ERROR(gl.getError(), "glDetachShader call failed.");
				}
			}
		}
		else
		{
			glw::GLint log_size = 0;

			gl.getProgramiv(po, GL_INFO_LOG_LENGTH, &log_size);

			GLU_EXPECT_NO_ERROR(gl.getError(), "glGetProgramiv call failed.");

			glw::GLchar* log_text = new glw::GLchar[log_size];

			gl.getProgramInfoLog(po, log_size, NULL, &log_text[0]);

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
		if (po)
		{
			gl.deleteProgram(po);

			po = 0;
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

	if (0 == po)
	{
		throw 0;
	}

	return po;
}

/** @brief Prepare vertex array object for the test.
 */
void EnableDisableAttributesTest::PrepareVAO()
{
	/* Shortcut for GL functionality */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* VAO creation. */
	gl.genVertexArrays(1, &m_vao);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenVertexArrays call failed.");

	gl.bindVertexArray(m_vao);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindVertexArray call failed.");

	/* Buffer creation. */
	gl.genBuffers(1, &m_bo);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenBuffers call failed.");

	gl.bindBuffer(GL_ARRAY_BUFFER, m_bo);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer call failed.");

	glw::GLint* reference_data = new glw::GLint[m_max_attributes];

	if (DE_NULL == reference_data)
	{
		throw 0;
	}

	for (glw::GLint i = 0; i < m_max_attributes; ++i)
	{
		reference_data[i] = i;
	}

	gl.bufferData(GL_ARRAY_BUFFER, sizeof(glw::GLint) * m_max_attributes, reference_data, GL_STATIC_DRAW);

	delete[] reference_data;

	GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferData call failed.");

	/* VAO setup. */
	for (glw::GLint i = 0; i < m_max_attributes; ++i)
	{
		gl.vertexAttribIPointer(i, 1, GL_INT, static_cast<glw::GLsizei>(sizeof(glw::GLint) * m_max_attributes),
								(glw::GLvoid*)((glw::GLint*)NULL + i));

		GLU_EXPECT_NO_ERROR(gl.getError(), "glVertexAttribIPointer call failed.");
	}
}

/** @brief Prepare buffer object for test GLSL program transform feedback results.
 */
void EnableDisableAttributesTest::PrepareXFB()
{
	/* Shortcut for GL functionality */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Buffer creation. */
	gl.genBuffers(1, &m_bo_xfb);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenBuffers call failed.");

	gl.bindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, m_bo_xfb);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer call failed.");

	/* Preparing storage. */
	gl.bufferStorage(GL_TRANSFORM_FEEDBACK_BUFFER, sizeof(glw::GLint), NULL, GL_MAP_READ_BIT);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferStorage call failed.");

	gl.bindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, m_bo_xfb);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBufferBase call failed.");
}

/** @brief Draw test program, fetch transform feedback results and compare them with expected values.
 *
 *  @param [in] bind_even_or_odd         Even or odd attribute are enabled.
 *
 *  @return True if expected results are equal to returned by XFB, false otherwise.
 */
bool EnableDisableAttributesTest::DrawAndCheck(bool bind_even_or_odd)
{
	/* Shortcut for GL functionality */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Setup state. */
	gl.useProgram(bind_even_or_odd ? m_po_odd : m_po_even);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram call failed.");

	gl.bindVertexArray(m_vao);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindVertexArray call failed.");

	gl.beginTransformFeedback(GL_POINTS);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBeginTransformFeedback call failed.");

	/* Draw. */
	gl.drawArrays(GL_POINTS, 0, 1);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawArrays call failed.");

	/* State reset. */
	gl.endTransformFeedback();
	GLU_EXPECT_NO_ERROR(gl.getError(), "glEndTransformFeedback call failed.");

	/* Result query. */
	glw::GLint* result_ptr = (glw::GLint*)gl.mapBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, GL_READ_ONLY);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glMapBuffer call failed.");

	glw::GLint result = *result_ptr;

	gl.unmapBuffer(GL_TRANSFORM_FEEDBACK_BUFFER);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUnmapBuffer call failed.");

	/* Check result and return. */
	if (bind_even_or_odd)
	{
		glw::GLint reference_sum = 0;

		for (glw::GLint i = 1; i < m_max_attributes; i += 2)
		{
			reference_sum += i;
		}

		if (reference_sum == result)
		{
			return true;
		}
	}
	else
	{
		glw::GLint reference_sum = 0;

		for (glw::GLint i = 0; i < m_max_attributes; i += 2)
		{
			reference_sum += i;
		}

		if (reference_sum == result)
		{
			return true;
		}
	}

	return false;
}

/** @brief Turn on even or odd attributes (up to m_max_attributes) using EnableVertexArrayAttrib function.
 *
 *  @param [in] enable_even         Turn on even attribute indexes.
 *  @param [in] enable_odd          Turn on odd  attribute indexes.
 *
 *  @return True if EnableVertexArrayAttrib does not generate any error, false otherwise.
 */
bool EnableDisableAttributesTest::TurnOnAttributes(bool enable_even, bool enable_odd)
{
	/* Shortcut for GL functionality */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.bindVertexArray(0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindVertexArray call failed.");

	for (glw::GLint i = 0; i < m_max_attributes; ++i)
	{
		bool disable = true;

		if (i % 2) /* if odd */
		{
			if (enable_odd)
			{
				gl.enableVertexArrayAttrib(m_vao, i);

				if (glw::GLenum error = gl.getError())
				{
					m_context.getTestContext().getLog()
						<< tcu::TestLog::Message << "glEnableVertexArrayAttrib generated error "
						<< glu::getErrorStr(error) << " when called with VAO " << m_vao << " and index " << i << "."
						<< tcu::TestLog::EndMessage;

					return false;
				}

				disable = false;
			}
		}
		else
		{
			if (enable_even)
			{
				gl.enableVertexArrayAttrib(m_vao, i);

				if (glw::GLenum error = gl.getError())
				{
					m_context.getTestContext().getLog()
						<< tcu::TestLog::Message << "glEnableVertexArrayAttrib generated error "
						<< glu::getErrorStr(error) << " when called with VAO " << m_vao << " and index " << i << "."
						<< tcu::TestLog::EndMessage;

					return false;
				}

				disable = false;
			}
		}

		if (disable)
		{
			gl.disableVertexArrayAttrib(m_vao, i);

			if (glw::GLenum error = gl.getError())
			{
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message << "glDisableVertexArrayAttrib generated error " << glu::getErrorStr(error)
					<< " when called with VAO " << m_vao << " and index " << i << "." << tcu::TestLog::EndMessage;

				return false;
			}
		}
	}

	gl.bindVertexArray(m_vao);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindVertexArray call failed.");

	return true;
}

/** @brief Clean GL objects. */
void EnableDisableAttributesTest::Clean()
{
	/* Shortcut for GL functionality */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.useProgram(0);

	if (m_po_even)
	{
		gl.deleteProgram(m_po_even);

		m_po_even = 0;
	}

	if (m_po_odd)
	{
		gl.deleteProgram(m_po_odd);

		m_po_odd = 0;
	}

	if (m_vao)
	{
		gl.deleteVertexArrays(1, &m_vao);

		m_vao = 0;
	}

	if (m_bo)
	{
		gl.deleteBuffers(1, &m_bo);

		m_bo = 0;
	}

	if (m_bo_xfb)
	{
		gl.deleteBuffers(1, &m_bo_xfb);

		m_bo_xfb = 0;
	}

	if (m_max_attributes)
	{
		m_max_attributes =
			16; /* OpenGL 4.5 Required Minimum Table 23.57: Implementation Dependent Vertex Shader Limits */
	}

	while (gl.getError())
		;
}

std::string Utilities::itoa(glw::GLuint i)
{
	std::string s = "";

	std::stringstream ss;

	ss << i;

	s = ss.str();

	return s;
}

std::string Utilities::replace(const std::string& src, const std::string& key, const std::string& value)
{
	size_t		pos = 0;
	std::string dst = src;

	while (std::string::npos != (pos = dst.find(key, pos)))
	{
		dst.replace(pos, key.length(), value);
		pos += key.length();
	}

	return dst;
}

const glw::GLchar EnableDisableAttributesTest::s_vertex_shader_template[] = "#version 450\n"
																			"\n"
																			"DECLARATION_TEMPLATE"
																			"out int sum;\n"
																			"\n"
																			"void main()\n"
																			"{\n"
																			"COPY_TEMPLATE"
																			"}\n";

const glw::GLchar EnableDisableAttributesTest::s_fragment_shader[] = "#version 450\n"
																	 "\n"
																	 "out vec4 color;\n"
																	 "\n"
																	 "void main()\n"
																	 "{\n"
																	 "    color = vec4(1.0);"
																	 "}\n";

/******************************** Vertex Array Object Element Buffer Test Implementation   ********************************/

/** @brief Vertex Array Object Element Buffer Test constructor.
 *
 *  @param [in] context     OpenGL context.
 */
ElementBufferTest::ElementBufferTest(deqp::Context& context)
	: deqp::TestCase(context, "vertex_arrays_element_buffer", "Vertex Array Objects Element Buffer Test")
	, m_po(0)
	, m_vao(0)
	, m_bo_array(0)
	, m_bo_elements(0)
	, m_bo_xfb(0)
{
	/* Intentionally left blank. */
}

/** @brief Iterate Vertex Array Object Enable Disable Attributes Test cases.
 *
 *  @return Iteration result.
 */
tcu::TestNode::IterateResult ElementBufferTest::iterate()
{
	/* Shortcut for GL functionality. */
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

	try
	{
		PrepareProgram();
		is_ok &= PrepareVAO();
		PrepareXFB();
		is_ok &= DrawAndCheck();
	}
	catch (...)
	{
		is_ok	= false;
		is_error = true;
	}

	/* Cleanup. */
	Clean();

	/* Errors clean up. */
	while (gl.getError())
		;

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

/** @brief Build test's GLSL program.
 *
 *  @note The function may throw if unexpected error has occured.
 */
void ElementBufferTest::PrepareProgram()
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

		/* Transform Feedback setup. */
		static const glw::GLchar* xfb_varying = "result";

		gl.transformFeedbackVaryings(m_po, 1, &xfb_varying, GL_INTERLEAVED_ATTRIBS);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glTransformFeedbackVaryings call failed.");

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

	if (0 == m_po)
	{
		throw 0;
	}
}

/** @brief Prepare vertex array object for the test of VertexArrayElementBuffer function.
 *
 *  @return True if function VertexArrayElementBuffer* does not generate any error.
 */
bool ElementBufferTest::PrepareVAO()
{
	/* Shortcut for GL functionality */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* VAO creation. */
	gl.genVertexArrays(1, &m_vao);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenVertexArrays call failed.");

	gl.bindVertexArray(m_vao);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindVertexArray call failed.");

	/* Array buffer creation. */
	glw::GLint array_data[3] = { 2, 1, 0 };

	gl.genBuffers(1, &m_bo_array);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenBuffers call failed.");

	gl.bindBuffer(GL_ARRAY_BUFFER, m_bo_array);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer call failed.");

	gl.bufferData(GL_ARRAY_BUFFER, sizeof(array_data), array_data, GL_STATIC_DRAW);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferData call failed.");

	gl.vertexAttribIPointer(gl.getAttribLocation(m_po, "a"), 1, GL_INT, 0, NULL);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glVertexAttribIPointer call failed.");

	gl.enableVertexAttribArray(0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glEnableVertexAttribArray call failed.");

	gl.bindVertexArray(0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindVertexArray call failed.");

	/* Element buffer creation. */
	glw::GLuint elements_data[3] = { 2, 1, 0 };

	gl.genBuffers(1, &m_bo_elements);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenBuffers call failed.");

	gl.bindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_bo_elements);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer call failed.");

	gl.bufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(elements_data), elements_data, GL_STATIC_DRAW);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferData call failed.");

	gl.vertexArrayElementBuffer(m_vao, m_bo_elements);

	if (glw::GLenum error = gl.getError())
	{
		m_context.getTestContext().getLog() << tcu::TestLog::Message
											<< "VertexArrayElementBuffer has unexpectedly generated "
											<< glu::getErrorStr(error) << "error. Test fails.\n"
											<< tcu::TestLog::EndMessage;

		return false;
	}

	return true;
}

/** @brief Prepare buffer object for test GLSL program transform feedback results.
 */
void ElementBufferTest::PrepareXFB()
{
	/* Shortcut for GL functionality */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Buffer creation. */
	gl.genBuffers(1, &m_bo_xfb);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenBuffers call failed.");

	gl.bindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, m_bo_xfb);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer call failed.");

	/* Preparing storage. */
	gl.bufferStorage(GL_TRANSFORM_FEEDBACK_BUFFER, 3 * sizeof(glw::GLint), NULL, GL_MAP_READ_BIT);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferStorage call failed.");

	gl.bindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, m_bo_xfb);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBufferBase call failed.");
}

/** @brief Draw test program, fetch transform feedback results and compare them with expected values.
 *
 *  @return True if expected results are equal to returned by XFB, false otherwise.
 */
bool ElementBufferTest::DrawAndCheck()
{
	/* Shortcut for GL functionality */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Setup state. */
	gl.useProgram(m_po);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram call failed.");

	gl.bindVertexArray(m_vao);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindVertexArray call failed.");

	gl.beginTransformFeedback(GL_POINTS);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBeginTransformFeedback call failed.");

	/* Draw. */
	gl.drawElements(GL_POINTS, 3, GL_UNSIGNED_INT, NULL);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawArrays call failed.");

	/* State reset. */
	gl.endTransformFeedback();
	GLU_EXPECT_NO_ERROR(gl.getError(), "glEndTransformFeedback call failed.");

	/* Result query. */
	glw::GLint* result_ptr = (glw::GLint*)gl.mapBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, GL_READ_ONLY);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glMapBuffer call failed.");

	glw::GLint result[3] = { result_ptr[0], result_ptr[1], result_ptr[2] };

	gl.unmapBuffer(GL_TRANSFORM_FEEDBACK_BUFFER);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUnmapBuffer call failed.");

	/* Check result and return. */
	for (glw::GLint i = 0; i < 3; ++i)
	{
		if (i != result[i])
		{
			m_context.getTestContext().getLog() << tcu::TestLog::Message << "Result vector is equal to [" << result[0]
												<< ", " << result[1] << ", " << result[2]
												<< "], but [0, 1, 2] was expected." << tcu::TestLog::EndMessage;

			return false;
		}
	}

	return true;
}

/** @brief Clean GL objects. */
void ElementBufferTest::Clean()
{
	/* Shortcut for GL functionality */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.useProgram(0);

	if (m_po)
	{
		gl.deleteProgram(m_po);

		m_po = 0;
	}

	if (m_vao)
	{
		gl.deleteVertexArrays(1, &m_vao);

		m_vao = 0;
	}

	if (m_bo_array)
	{
		gl.deleteBuffers(1, &m_bo_array);

		m_bo_array = 0;
	}

	if (m_bo_elements)
	{
		gl.deleteBuffers(1, &m_bo_elements);

		m_bo_elements = 0;
	}

	if (m_bo_xfb)
	{
		gl.deleteBuffers(1, &m_bo_xfb);

		m_bo_xfb = 0;
	}

	while (gl.getError())
		;
}

const glw::GLchar ElementBufferTest::s_vertex_shader[] = "#version 450\n"
														 "\n"
														 "in int a;"
														 "out int result;\n"
														 "\n"
														 "void main()\n"
														 "{\n"
														 "    gl_Position = vec4(1.0);\n"
														 "    result = a;"
														 "}\n";

const glw::GLchar ElementBufferTest::s_fragment_shader[] = "#version 450\n"
														   "\n"
														   "out vec4 color;\n"
														   "\n"
														   "void main()\n"
														   "{\n"
														   "    color = vec4(1.0);"
														   "}\n";

/******************************** Vertex Array Object Vertex Buffer and Buffers Test Implementation   ********************************/

/** @brief Vertex Array Object Element Buffer Test constructor.
 *
 *  @param [in] context     OpenGL context.
 */
VertexBuffersTest::VertexBuffersTest(deqp::Context& context)
	: deqp::TestCase(context, "vertex_arrays_vertex_buffers", "Vertex Array Object Vertex Buffer and Buffers Test")
	, m_po(0)
	, m_vao(0)
	, m_bo_array_0(0)
	, m_bo_array_1(0)
	, m_bo_xfb(0)
{
	/* Intentionally left blank. */
}

/** @brief Iterate Vertex Array Object Enable Disable Attributes Test cases.
 *
 *  @return Iteration result.
 */
tcu::TestNode::IterateResult VertexBuffersTest::iterate()
{
	/* Shortcut for GL functionality. */
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

	try
	{
		PrepareProgram();
		is_ok &= PrepareVAO(false);
		PrepareXFB();
		is_ok &= DrawAndCheck();
		Clean();

		PrepareProgram();
		is_ok &= PrepareVAO(true);
		PrepareXFB();
		is_ok &= DrawAndCheck();
	}
	catch (...)
	{
		is_ok	= false;
		is_error = true;
	}

	/* Cleanup. */
	Clean();

	/* Errors clean up. */
	while (gl.getError())
		;

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

/** @brief Build test's GLSL program.
 *
 *  @note The function may throw if unexpected error has occured.
 */
void VertexBuffersTest::PrepareProgram()
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

		/* Transform Feedback setup. */
		static const glw::GLchar* xfb_varying = "result";

		gl.transformFeedbackVaryings(m_po, 1, &xfb_varying, GL_INTERLEAVED_ATTRIBS);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glTransformFeedbackVaryings call failed.");

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

	if (0 == m_po)
	{
		throw 0;
	}
}

/** @brief Prepare vertex array object for the test of gl.vertexArrayVertexBuffer* functions.
 *
 *  @param [in] use_multiple_buffers_function    Use gl.vertexArrayVertexBuffers instead of gl.vertexArrayVertexBuffer.
 *
 *  @return True if functions gl.vertexArrayVertexBuffer* do not generate any error.
 */
bool VertexBuffersTest::PrepareVAO(bool use_multiple_buffers_function)
{
	/* Shortcut for GL functionality */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* VAO creation. */
	gl.genVertexArrays(1, &m_vao);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenVertexArrays call failed.");

	gl.bindVertexArray(m_vao);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindVertexArray call failed.");

	/* Array buffer 0 creation. */
	glw::GLint array_data_0[4] = { 0, 2, 1, 3 };

	gl.genBuffers(1, &m_bo_array_0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenBuffers call failed.");

	gl.bindBuffer(GL_ARRAY_BUFFER, m_bo_array_0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer call failed.");

	gl.bufferData(GL_ARRAY_BUFFER, sizeof(array_data_0), array_data_0, GL_STATIC_DRAW);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferData call failed.");

	gl.vertexAttribBinding(gl.getAttribLocation(m_po, "a_0"), 0);

	gl.vertexAttribBinding(gl.getAttribLocation(m_po, "a_1"), 1);

	gl.vertexAttribIFormat(gl.getAttribLocation(m_po, "a_0"), 1, GL_INT, 0);

	gl.vertexAttribIFormat(gl.getAttribLocation(m_po, "a_1"), 1, GL_INT, 0);

	if (use_multiple_buffers_function)
	{
		const glw::GLuint		   buffers[2] = { m_bo_array_0, m_bo_array_0 };
		static const glw::GLintptr offsets[2] = { (glw::GLintptr)NULL, (glw::GLintptr)((glw::GLint*)NULL + 1) };
		static const glw::GLsizei  strides[2] = { sizeof(glw::GLint) * 2, sizeof(glw::GLint) * 2 };

		gl.vertexArrayVertexBuffers(m_vao, 0, 2, buffers, offsets, strides);

		if (glw::GLenum error = gl.getError())
		{
			m_context.getTestContext().getLog() << tcu::TestLog::Message
												<< "VertexArrayVertexBuffers has unexpectedly generated "
												<< glu::getErrorStr(error) << "error. Test fails.\n"
												<< tcu::TestLog::EndMessage;

			return false;
		}
	}
	else
	{
		gl.vertexArrayVertexBuffer(m_vao, 0, m_bo_array_0, (glw::GLintptr)NULL, sizeof(glw::GLint) * 2);

		if (glw::GLenum error = gl.getError())
		{
			m_context.getTestContext().getLog() << tcu::TestLog::Message
												<< "VertexArrayVertexBuffer has unexpectedly generated "
												<< glu::getErrorStr(error) << "error. Test fails.\n"
												<< tcu::TestLog::EndMessage;

			return false;
		}

		gl.vertexArrayVertexBuffer(m_vao, 1, m_bo_array_0, (glw::GLintptr)((glw::GLint*)NULL + 1),
								   sizeof(glw::GLint) * 2);

		if (glw::GLenum error = gl.getError())
		{
			m_context.getTestContext().getLog() << tcu::TestLog::Message
												<< "VertexArrayVertexBuffer has unexpectedly generated "
												<< glu::getErrorStr(error) << "error. Test fails.\n"
												<< tcu::TestLog::EndMessage;

			return false;
		}
	}

	gl.enableVertexAttribArray(0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glEnableVertexAttribArray call failed.");

	gl.enableVertexAttribArray(1);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glEnableVertexAttribArray call failed.");

	/* Array buffer 1 creation. */
	glw::GLint array_data_1[2] = { 4, 5 };

	gl.genBuffers(1, &m_bo_array_1);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenBuffers call failed.");

	gl.bindBuffer(GL_ARRAY_BUFFER, m_bo_array_1);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer call failed.");

	gl.bufferData(GL_ARRAY_BUFFER, sizeof(array_data_1), array_data_1, GL_STATIC_DRAW);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferData call failed.");

	gl.vertexAttribBinding(gl.getAttribLocation(m_po, "a_2"), 2);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glVertexAttribBinding call failed.");

	gl.vertexAttribIFormat(gl.getAttribLocation(m_po, "a_2"), 1, GL_INT, 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glVertexAttribIFormat call failed.");

	if (use_multiple_buffers_function)
	{
		glw::GLintptr offset = (glw::GLintptr)NULL;
		glw::GLsizei  stride = sizeof(glw::GLint);

		gl.vertexArrayVertexBuffers(m_vao, 2, 1, &m_bo_array_1, &offset, &stride);

		if (glw::GLenum error = gl.getError())
		{
			m_context.getTestContext().getLog() << tcu::TestLog::Message
												<< "VertexArrayVertexBuffers has unexpectedly generated "
												<< glu::getErrorStr(error) << "error. Test fails.\n"
												<< tcu::TestLog::EndMessage;

			return false;
		}
	}
	else
	{
		gl.vertexArrayVertexBuffer(m_vao, 2, m_bo_array_1, (glw::GLintptr)NULL, sizeof(glw::GLint));

		if (glw::GLenum error = gl.getError())
		{
			m_context.getTestContext().getLog() << tcu::TestLog::Message
												<< "VertexArrayVertexBuffer has unexpectedly generated "
												<< glu::getErrorStr(error) << "error. Test fails.\n"
												<< tcu::TestLog::EndMessage;

			return false;
		}
	}

	gl.enableVertexAttribArray(2);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glEnableVertexAttribArray call failed.");

	gl.bindVertexArray(0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindVertexArray call failed.");

	return true;
}

/** @brief Prepare buffer object for test GLSL program transform feedback results.
 */
void VertexBuffersTest::PrepareXFB()
{
	/* Shortcut for GL functionality */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Buffer creation. */
	gl.genBuffers(1, &m_bo_xfb);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenBuffers call failed.");

	gl.bindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, m_bo_xfb);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer call failed.");

	/* Preparing storage. */
	gl.bufferStorage(GL_TRANSFORM_FEEDBACK_BUFFER, 2 * sizeof(glw::GLint), NULL, GL_MAP_READ_BIT);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferStorage call failed.");

	gl.bindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, m_bo_xfb);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBufferBase call failed.");
}

/** @brief Draw test program, fetch transform feedback results and compare them with expected values.
 *
 *  @return True if expected results are equal to returned by XFB, false otherwise.
 */
bool VertexBuffersTest::DrawAndCheck()
{
	/* Shortcut for GL functionality */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Setup state. */
	gl.useProgram(m_po);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram call failed.");

	gl.bindVertexArray(m_vao);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindVertexArray call failed.");

	gl.beginTransformFeedback(GL_POINTS);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBeginTransformFeedback call failed.");

	/* Draw. */
	gl.drawArrays(GL_POINTS, 0, 2);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawArrays call failed.");

	/* State reset. */
	gl.endTransformFeedback();
	GLU_EXPECT_NO_ERROR(gl.getError(), "glEndTransformFeedback call failed.");

	/* Result query. */
	glw::GLint* result_ptr = (glw::GLint*)gl.mapBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, GL_READ_ONLY);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glMapBuffer call failed.");

	glw::GLint result[2] = { result_ptr[0], result_ptr[1] };

	gl.unmapBuffer(GL_TRANSFORM_FEEDBACK_BUFFER);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUnmapBuffer call failed.");

	static const glw::GLint reference[2] = { 0 + 2 + 4, 1 + 3 + 5 };

	/* Check result and return. */
	for (glw::GLint i = 0; i < 2; ++i)
	{
		if (reference[i] != result[i])
		{
			m_context.getTestContext().getLog() << tcu::TestLog::Message << "Result vector is equal to [" << result[0]
												<< ", " << result[1] << "], but [" << reference[0] << ", "
												<< reference[1] << "] was expected." << tcu::TestLog::EndMessage;

			return false;
		}
	}

	return true;
}

/** @brief Clean GL objects. */
void VertexBuffersTest::Clean()
{
	/* Shortcut for GL functionality */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.useProgram(0);

	if (m_po)
	{
		gl.deleteProgram(m_po);

		m_po = 0;
	}

	if (m_vao)
	{
		gl.deleteVertexArrays(1, &m_vao);

		m_vao = 0;
	}

	if (m_bo_array_0)
	{
		gl.deleteBuffers(1, &m_bo_array_0);

		m_bo_array_0 = 0;
	}

	if (m_bo_array_1)
	{
		gl.deleteBuffers(1, &m_bo_array_1);

		m_bo_array_1 = 0;
	}

	if (m_bo_xfb)
	{
		gl.deleteBuffers(1, &m_bo_xfb);

		m_bo_xfb = 0;
	}

	while (gl.getError())
		;
}

const glw::GLchar VertexBuffersTest::s_vertex_shader[] = "#version 450\n"
														 "\n"
														 "in  int a_0;"
														 "in  int a_1;"
														 "in  int a_2;"
														 "\n"
														 "out int result;\n"
														 "\n"
														 "void main()\n"
														 "{\n"
														 "    gl_Position = vec4(1.0);\n"
														 "    result = a_0 + a_1 + a_2;"
														 "}\n";

const glw::GLchar VertexBuffersTest::s_fragment_shader[] = "#version 450\n"
														   "\n"
														   "out vec4 color;\n"
														   "\n"
														   "void main()\n"
														   "{\n"
														   "    color = vec4(1.0);"
														   "}\n";

/******************************** Vertex Array Object Attribute Format Test Implementation   ********************************/

/** @brief Vertex Array Object Element Buffer Test constructor.
 *
 *  @param [in] context     OpenGL context.
 */
AttributeFormatTest::AttributeFormatTest(deqp::Context& context)
	: deqp::TestCase(context, "vertex_arrays_attribute_format", "Vertex Array Object Attribute Format Test")
	, m_po(0)
	, m_vao(0)
	, m_bo_array(0)
	, m_bo_xfb(0)
{
	/* Intentionally left blank. */
}

/** @brief Iterate Vertex Array Object Enable Disable Attributes Test cases.
 *
 *  @return Iteration result.
 */
tcu::TestNode::IterateResult AttributeFormatTest::iterate()
{
	/* Shortcut for GL functionality. */
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

	try
	{
		PrepareXFB();

		/* Test floating function. */
		for (glw::GLuint i = 1; i <= 4 /* max size */; ++i)
		{
			PrepareProgram(i, ATTRIBUTE_FORMAT_FUNCTION_FLOAT);

			is_ok &= PrepareVAO<glw::GLfloat>(i, GL_FLOAT, false, ATTRIBUTE_FORMAT_FUNCTION_FLOAT);
			is_ok &= DrawAndCheck<glw::GLfloat>(i, false);

			CleanVAO();

			is_ok &= PrepareVAO<glw::GLbyte>(i, GL_BYTE, false, ATTRIBUTE_FORMAT_FUNCTION_FLOAT);
			is_ok &= DrawAndCheck<glw::GLfloat>(i, false);

			CleanVAO();

			is_ok &= PrepareVAO<glw::GLbyte>(i, GL_BYTE, true, ATTRIBUTE_FORMAT_FUNCTION_FLOAT);
			is_ok &= DrawAndCheck<glw::GLfloat>(i, true);

			CleanVAO();

			is_ok &= PrepareVAO<glw::GLubyte>(i, GL_UNSIGNED_BYTE, false, ATTRIBUTE_FORMAT_FUNCTION_FLOAT);
			is_ok &= DrawAndCheck<glw::GLfloat>(i, false);

			CleanVAO();

			is_ok &= PrepareVAO<glw::GLshort>(i, GL_SHORT, false, ATTRIBUTE_FORMAT_FUNCTION_FLOAT);
			is_ok &= DrawAndCheck<glw::GLfloat>(i, false);

			CleanVAO();

			is_ok &= PrepareVAO<glw::GLushort>(i, GL_UNSIGNED_SHORT, false, ATTRIBUTE_FORMAT_FUNCTION_FLOAT);
			is_ok &= DrawAndCheck<glw::GLfloat>(i, false);

			CleanVAO();

			is_ok &= PrepareVAO<glw::GLint>(i, GL_INT, false, ATTRIBUTE_FORMAT_FUNCTION_FLOAT);
			is_ok &= DrawAndCheck<glw::GLfloat>(i, false);

			CleanVAO();

			is_ok &= PrepareVAO<glw::GLuint>(i, GL_UNSIGNED_INT, false, ATTRIBUTE_FORMAT_FUNCTION_FLOAT);
			is_ok &= DrawAndCheck<glw::GLfloat>(i, false);

			CleanVAO();

			CleanProgram();
		}

		for (glw::GLuint i = 1; i <= 2 /* max size */; ++i)
		{
			PrepareProgram(i, ATTRIBUTE_FORMAT_FUNCTION_DOUBLE);

			is_ok &= PrepareVAO<glw::GLdouble>(i, GL_DOUBLE, false, ATTRIBUTE_FORMAT_FUNCTION_DOUBLE);
			is_ok &= DrawAndCheck<glw::GLdouble>(i, false);

			CleanProgram();
			CleanVAO();
		}

		for (glw::GLuint i = 1; i <= 4 /* max size */; ++i)
		{
			PrepareProgram(i, ATTRIBUTE_FORMAT_FUNCTION_INTEGER);

			is_ok &= PrepareVAO<glw::GLbyte>(i, GL_BYTE, false, ATTRIBUTE_FORMAT_FUNCTION_INTEGER);
			is_ok &= DrawAndCheck<glw::GLint>(i, false);

			CleanVAO();

			is_ok &= PrepareVAO<glw::GLubyte>(i, GL_UNSIGNED_BYTE, false, ATTRIBUTE_FORMAT_FUNCTION_INTEGER);
			is_ok &= DrawAndCheck<glw::GLint>(i, false);

			CleanVAO();

			is_ok &= PrepareVAO<glw::GLshort>(i, GL_SHORT, false, ATTRIBUTE_FORMAT_FUNCTION_INTEGER);
			is_ok &= DrawAndCheck<glw::GLint>(i, false);

			CleanVAO();

			is_ok &= PrepareVAO<glw::GLushort>(i, GL_UNSIGNED_SHORT, false, ATTRIBUTE_FORMAT_FUNCTION_INTEGER);
			is_ok &= DrawAndCheck<glw::GLint>(i, false);

			CleanVAO();

			is_ok &= PrepareVAO<glw::GLint>(i, GL_INT, false, ATTRIBUTE_FORMAT_FUNCTION_INTEGER);
			is_ok &= DrawAndCheck<glw::GLint>(i, false);

			CleanVAO();

			is_ok &= PrepareVAO<glw::GLuint>(i, GL_UNSIGNED_INT, false, ATTRIBUTE_FORMAT_FUNCTION_INTEGER);
			is_ok &= DrawAndCheck<glw::GLint>(i, false);

			CleanVAO();

			CleanProgram();
		}
	}
	catch (...)
	{
		is_ok	= false;
		is_error = true;
	}

	/* Cleanup. */
	CleanProgram();
	CleanVAO();
	CleanXFB();

	/* Errors clean up. */
	while (gl.getError())
		;

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

/** @brief Build test's GLSL program.
 *
 *  @note The function may throw if unexpected error has occured.
 */
void AttributeFormatTest::PrepareProgram(glw::GLint size, AtributeFormatFunctionType function_selector)
{
	/* Shortcut for GL functionality */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	struct Shader
	{
		glw::GLchar const* source[3];
		glw::GLuint const  count;
		glw::GLenum const  type;
		glw::GLuint		   id;
	} shader[] = { { { s_vertex_shader_head, s_vertex_shader_declaration[function_selector][size - 1],
					   s_vertex_shader_body },
					 3,
					 GL_VERTEX_SHADER,
					 0 },
				   { { s_fragment_shader, DE_NULL, DE_NULL }, 1, GL_FRAGMENT_SHADER, 0 } };

	glw::GLuint const shader_count = sizeof(shader) / sizeof(shader[0]);

	try
	{
		/* Create program. */
		m_po = gl.createProgram();
		GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateProgram call failed.");

		/* Shader compilation. */

		for (glw::GLuint i = 0; i < shader_count; ++i)
		{
			{
				shader[i].id = gl.createShader(shader[i].type);

				GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateShader call failed.");

				gl.attachShader(m_po, shader[i].id);

				GLU_EXPECT_NO_ERROR(gl.getError(), "glAttachShader call failed.");

				gl.shaderSource(shader[i].id, shader[i].count, shader[i].source, NULL);

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

		/* Transform Feedback setup. */
		static const glw::GLchar* xfb_varying = "result";

		gl.transformFeedbackVaryings(m_po, 1, &xfb_varying, GL_INTERLEAVED_ATTRIBS);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glTransformFeedbackVaryings call failed.");

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

	if (0 == m_po)
	{
		throw 0;
	}
}

template <>
glw::GLdouble AttributeFormatTest::NormalizationScaleFactor<glw::GLuint>()
{
	return std::pow(2.0, (glw::GLdouble)(CHAR_BIT * sizeof(glw::GLuint) - 4 /* 1.0 / 16.0 */));
}

template <>
glw::GLdouble AttributeFormatTest::NormalizationScaleFactor<glw::GLushort>()
{
	return std::pow(2.0, (glw::GLdouble)(CHAR_BIT * sizeof(glw::GLushort) - 4 /* 1.0 / 16.0 */));
}

template <>
glw::GLdouble AttributeFormatTest::NormalizationScaleFactor<glw::GLubyte>()
{
	return std::pow(2.0, (glw::GLdouble)(CHAR_BIT * sizeof(glw::GLubyte) - 4 /* 1.0 / 16.0 */));
}

template <>
glw::GLdouble AttributeFormatTest::NormalizationScaleFactor<glw::GLint>()
{
	return std::pow(2.0, (glw::GLdouble)(CHAR_BIT * sizeof(glw::GLint) - 4 /* 1.0 / 16.0 */ - 1 /* sign bit */));
}

template <>
glw::GLdouble AttributeFormatTest::NormalizationScaleFactor<glw::GLshort>()
{
	return std::pow(2.0, (glw::GLdouble)(CHAR_BIT * sizeof(glw::GLshort) - 4 /* 1.0 / 16.0 */ - 1 /* sign bit */));
}

template <>
glw::GLdouble AttributeFormatTest::NormalizationScaleFactor<glw::GLbyte>()
{
	return std::pow(2.0, (glw::GLdouble)(CHAR_BIT * sizeof(glw::GLbyte) - 4 /* 1.0 / 16.0 */ - 1 /* sign bit */));
}

template <typename T>
glw::GLdouble AttributeFormatTest::NormalizationScaleFactor()
{
	return 1.0; /* Rest of the types cannot be normalized. */
}

/** @brief Prepare vertex array object for the test of VertexArrayAttrib*Format function.
 *
 *  @param [in] size                Size passed to VertexArrayAttrib*Format.
 *  @param [in] type_gl_name        Type passed to VertexArrayAttrib*Format.
 *  @param [in] function_selector   Selects one of VertexArrayAttrib*Format functions.
 *
 *  @return True if function VertexArrayAttrib*Format does not generate any error.
 */
template <typename T>
bool AttributeFormatTest::PrepareVAO(glw::GLint size, glw::GLenum type_gl_name, bool normalized,
									 AtributeFormatFunctionType function_selector)
{
	/* Shortcut for GL functionality */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* VAO creation. */
	gl.genVertexArrays(1, &m_vao);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenVertexArrays call failed.");

	gl.bindVertexArray(m_vao);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindVertexArray call failed.");

	/* Array buffer 0 creation. */

	const glw::GLdouble scale = normalized ? NormalizationScaleFactor<T>() : 1.0;

	const T array_data[16] = { (T)(0.0 * scale),  (T)(1.0 * scale),  (T)(2.0 * scale),  (T)(3.0 * scale),
							   (T)(4.0 * scale),  (T)(5.0 * scale),  (T)(6.0 * scale),  (T)(7.0 * scale),
							   (T)(8.0 * scale),  (T)(9.0 * scale),  (T)(10.0 * scale), (T)(11.0 * scale),
							   (T)(12.0 * scale), (T)(13.0 * scale), (T)(14.0 * scale), (T)(15.0 * scale) };

	gl.genBuffers(1, &m_bo_array);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenBuffers call failed.");

	gl.bindBuffer(GL_ARRAY_BUFFER, m_bo_array);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer call failed.");

	gl.bufferData(GL_ARRAY_BUFFER, sizeof(array_data), array_data, GL_STATIC_DRAW);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferData call failed.");

	/* Attribute setup. */
	gl.vertexAttribBinding(gl.getAttribLocation(m_po, "a_0"), 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glVertexAttribBinding call failed.");

	gl.vertexAttribBinding(gl.getAttribLocation(m_po, "a_1"), 1);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glVertexAttribBinding call failed.");

	/* Tested attribute format setup. */
	switch (function_selector)
	{
	case ATTRIBUTE_FORMAT_FUNCTION_FLOAT:
		gl.vertexArrayAttribFormat(m_vao, gl.getAttribLocation(m_po, "a_0"), size, type_gl_name, normalized, 0);
		gl.vertexArrayAttribFormat(m_vao, gl.getAttribLocation(m_po, "a_1"), size, type_gl_name, normalized, 0);
		break;

	case ATTRIBUTE_FORMAT_FUNCTION_DOUBLE:
		gl.vertexArrayAttribLFormat(m_vao, gl.getAttribLocation(m_po, "a_0"), size, type_gl_name, 0);
		gl.vertexArrayAttribLFormat(m_vao, gl.getAttribLocation(m_po, "a_1"), size, type_gl_name, 0);
		break;

	case ATTRIBUTE_FORMAT_FUNCTION_INTEGER:
		gl.vertexArrayAttribIFormat(m_vao, gl.getAttribLocation(m_po, "a_0"), size, type_gl_name, 0);
		gl.vertexArrayAttribIFormat(m_vao, gl.getAttribLocation(m_po, "a_1"), size, type_gl_name, 0);
		break;
	default:
		throw 0;
	}

	if (glw::GLenum error = gl.getError())
	{
		m_context.getTestContext().getLog()
			<< tcu::TestLog::Message
			<< ((ATTRIBUTE_FORMAT_FUNCTION_FLOAT == function_selector) ?
					"VertexArrayAttribFormat" :
					((ATTRIBUTE_FORMAT_FUNCTION_DOUBLE == function_selector) ?
						 "VertexArrayAttribLFormat" :
						 ((ATTRIBUTE_FORMAT_FUNCTION_INTEGER == function_selector) ? "VertexArrayAttribIFormat" :
																					 "VertexArrayAttrib?Format")))
			<< " has unexpectedly generated " << glu::getErrorStr(error) << "error for test with size = " << size
			<< ", type = " << glu::getTypeStr(type_gl_name)
			<< ((ATTRIBUTE_FORMAT_FUNCTION_FLOAT == function_selector) ?
					(normalized ? ", which was normalized." : ", which was not normalized.") :
					".")
			<< " Test fails.\n"
			<< tcu::TestLog::EndMessage;

		return false;
	}

	gl.bindVertexBuffer(0, m_bo_array, 0, static_cast<glw::GLsizei>(sizeof(T) * size * 2));
	gl.bindVertexBuffer(1, m_bo_array, (glw::GLintptr)((T*)NULL + size),
						static_cast<glw::GLsizei>(sizeof(T) * size * 2));

	GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferData call failed.");

	gl.enableVertexAttribArray(0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glEnableVertexAttribArray call failed.");

	gl.enableVertexAttribArray(1);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glEnableVertexAttribArray call failed.");

	gl.bindVertexArray(0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindVertexArray call failed.");

	return true;
}

/** @brief Prepare buffer object for test GLSL program transform feedback results.
 */
void AttributeFormatTest::PrepareXFB()
{
	/* Shortcut for GL functionality */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Buffer creation. */
	gl.genBuffers(1, &m_bo_xfb);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenBuffers call failed.");

	gl.bindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, m_bo_xfb);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer call failed.");

	/* Calculating maximum size. */
	glw::GLsizei size = static_cast<glw::GLsizei>(
		de::max(sizeof(glw::GLubyte),
				de::max(sizeof(glw::GLbyte),
						de::max(sizeof(glw::GLushort),
								de::max(sizeof(glw::GLshort),
										de::max(sizeof(glw::GLhalf),
												de::max(sizeof(glw::GLint),
														de::max(sizeof(glw::GLuint),
																de::max(sizeof(glw::GLfixed),
																		de::max(sizeof(glw::GLfloat),
																				sizeof(glw::GLdouble)))))))))) *
		4 /* maximum number of components */);

	/* Preparing storage. */
	gl.bufferStorage(GL_TRANSFORM_FEEDBACK_BUFFER, size, NULL, GL_MAP_READ_BIT);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferStorage call failed.");

	gl.bindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, m_bo_xfb);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBufferBase call failed.");
}

template <>
bool AttributeFormatTest::compare<glw::GLfloat>(glw::GLfloat a, glw::GLfloat b)
{
	if (de::abs(a - b) < 0.03125)
	{
		return true;
	}

	return false;
}

template <>
bool AttributeFormatTest::compare<glw::GLdouble>(glw::GLdouble a, glw::GLdouble b)
{
	if (de::abs(a - b) < 0.03125)
	{
		return true;
	}

	return false;
}

template <typename T>
bool AttributeFormatTest::compare(T a, T b)
{
	return (a == b);
}

/** @brief Draw test program, fetch transform feedback results and compare them with expected values.
 *
 *  @param [in] size         Count of elements of the XFB vector is expected.
 *  @param [in] normalized   Normalized values are expected.
 *
 *  @return True if expected results are equal to returned by XFB, false otherwise.
 */
template <typename T>
bool AttributeFormatTest::DrawAndCheck(glw::GLint size, bool normalized)
{
	/* Shortcut for GL functionality */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Setup state. */
	gl.useProgram(m_po);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram call failed.");

	gl.bindVertexArray(m_vao);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindVertexArray call failed.");

	/* Draw. */
	gl.beginTransformFeedback(GL_POINTS);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBeginTransformFeedback call failed.");

	gl.drawArrays(GL_POINTS, 0, 2);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawArrays call failed.");

	gl.endTransformFeedback();
	GLU_EXPECT_NO_ERROR(gl.getError(), "glEndTransformFeedback call failed.");

	/* Result query. */
	T* result_ptr = (T*)gl.mapBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, GL_READ_ONLY);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glMapBuffer call failed.");

	T result[8] = { 0 };

	for (glw::GLint i = 0; i < size * 2 /* two points */; ++i)
	{
		result[i] = result_ptr[i];
	}

	gl.unmapBuffer(GL_TRANSFORM_FEEDBACK_BUFFER);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUnmapBuffer call failed.");

	const glw::GLdouble scale = normalized ? (1.0 / 16.0) /* Floating point scalling factor. */ : 1.0;

	const T array_data[16] = { (T)(0.0 * scale),  (T)(1.0 * scale),  (T)(2.0 * scale),  (T)(3.0 * scale),
							   (T)(4.0 * scale),  (T)(5.0 * scale),  (T)(6.0 * scale),  (T)(7.0 * scale),
							   (T)(8.0 * scale),  (T)(9.0 * scale),  (T)(10.0 * scale), (T)(11.0 * scale),
							   (T)(12.0 * scale), (T)(13.0 * scale), (T)(14.0 * scale), (T)(15.0 * scale) };

	T reference[8] = { 0 };

	for (glw::GLint i = 0; i < 2 /* two points */; ++i)
	{
		for (glw::GLint j = 0; j < size /* size components */; ++j)
		{
			reference[i * size + j] = array_data[i * size * 2 + j] + array_data[i * size * 2 + j + size];
		}
	}

	/* Check result and return. */
	for (glw::GLint i = 0; i < size * 2 /* two points */; ++i)
	{
		if (!AttributeFormatTest::compare<T>(reference[i], result[i]))
		{
			std::string reference_str = "[ ";

			for (glw::GLint j = 0; j < size * 2 /* two points */; ++j)
			{
				std::stringstream ss;

				ss << reference[j];

				reference_str.append(ss.str());

				if (j < size * 2 - 1 /* if it is not the last value */)
				{
					reference_str.append(", ");
				}
				else
				{
					reference_str.append(" ]");
				}
			}

			std::string result_str = "[ ";

			for (glw::GLint j = 0; j < size * 2 /* two points */; ++j)
			{
				std::stringstream ss;

				ss << result[j];

				result_str.append(ss.str());

				if (j < size * 2 - 1 /* if it is not the last value */)
				{
					result_str.append(", ");
				}
				else
				{
					result_str.append(" ]");
				}
			}

			m_context.getTestContext().getLog() << tcu::TestLog::Message << "Result vector is equal to "
												<< result_str.c_str() << ", but " << reference_str.c_str()
												<< " was expected." << tcu::TestLog::EndMessage;

			return false;
		}
	}

	return true;
}

/** @brief Clean GLSL program object. */
void AttributeFormatTest::CleanProgram()
{
	/* Shortcut for GL functionality */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.useProgram(0);

	if (m_po)
	{
		gl.deleteProgram(m_po);

		m_po = 0;
	}
}

/** @brief Clean Vertex Array Object and related buffer. */
void AttributeFormatTest::CleanVAO()
{
	/* Shortcut for GL functionality */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (m_vao)
	{
		gl.deleteVertexArrays(1, &m_vao);

		m_vao = 0;
	}

	if (m_bo_array)
	{
		gl.deleteBuffers(1, &m_bo_array);

		m_bo_array = 0;
	}
}

/** @brief Clean GL objects related to transform feedback. */
void AttributeFormatTest::CleanXFB()
{
	/* Shortcut for GL functionality */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (m_bo_xfb)
	{
		gl.deleteBuffers(1, &m_bo_xfb);

		m_bo_xfb = 0;
	}

	while (gl.getError())
		;
}

const glw::GLchar* AttributeFormatTest::s_vertex_shader_head = "#version 450\n"
															   "\n";

const glw::GLchar* AttributeFormatTest::s_vertex_shader_body = "\n"
															   "void main()\n"
															   "{\n"
															   "    gl_Position = vec4(1.0);\n"
															   "    result = a_0 + a_1;"
															   "}\n";

const glw::GLchar* AttributeFormatTest::s_vertex_shader_declaration[ATTRIBUTE_FORMAT_FUNCTION_COUNT]
																   [4 /* sizes count */] = { {
																								 "in  float a_0;"
																								 "in  float a_1;"
																								 "out float result;\n",

																								 "in  vec2 a_0;"
																								 "in  vec2 a_1;"
																								 "out vec2 result;\n",

																								 "in  vec3 a_0;"
																								 "in  vec3 a_1;"
																								 "out vec3 result;\n",

																								 "in  vec4 a_0;"
																								 "in  vec4 a_1;"
																								 "out vec4 result;\n",
																							 },
																							 {
																								 "in  double a_0;"
																								 "in  double a_1;"
																								 "out double result;\n",

																								 "in  dvec2 a_0;"
																								 "in  dvec2 a_1;"
																								 "out dvec2 result;\n",

																								 "in  dvec3 a_0;"
																								 "in  dvec3 a_1;"
																								 "out dvec3 result;\n",

																								 "in  dvec4 a_0;"
																								 "in  dvec4 a_1;"
																								 "out dvec4 result;\n",
																							 },
																							 {
																								 "in  int a_0;"
																								 "in  int a_1;"
																								 "out int result;\n",

																								 "in  ivec2 a_0;"
																								 "in  ivec2 a_1;"
																								 "out ivec2 result;\n",

																								 "in  ivec3 a_0;"
																								 "in  ivec3 a_1;"
																								 "out ivec3 result;\n",

																								 "in  ivec4 a_0;"
																								 "in  ivec4 a_1;"
																								 "out ivec4 result;\n",
																							 } };

const glw::GLchar* AttributeFormatTest::s_fragment_shader = "#version 450\n"
															"\n"
															"out vec4 color;\n"
															"\n"
															"void main()\n"
															"{\n"
															"    color = vec4(1.0);"
															"}\n";

/******************************** Vertex Array Object Attribute Binding Test Implementation   ********************************/

/** @brief Attribute Binding Test constructor.
 *
 *  @param [in] context     OpenGL context.
 */
AttributeBindingTest::AttributeBindingTest(deqp::Context& context)
	: deqp::TestCase(context, "vertex_arrays_attribute_binding", "Vertex Array Objects Attribute Binding Test")
	, m_po(0)
	, m_vao(0)
	, m_bo_array(0)
	, m_bo_xfb(0)
{
	/* Intentionally left blank. */
}

/** @brief Iterate Attribute Binding Test cases.
 *
 *  @return Iteration result.
 */
tcu::TestNode::IterateResult AttributeBindingTest::iterate()
{
	/* Shortcut for GL functionality. */
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

	try
	{
		PrepareProgram();
		is_ok &= PrepareVAO();
		PrepareXFB();
		is_ok &= DrawAndCheck();
	}
	catch (...)
	{
		is_ok	= false;
		is_error = true;
	}

	/* Cleanup. */
	Clean();

	/* Errors clean up. */
	while (gl.getError())
		;

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

/** @brief Build test's GLSL program.
 *
 *  @note The function may throw if unexpected error has occured.
 */
void AttributeBindingTest::PrepareProgram()
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

		/* Binding attributes. */
		gl.bindAttribLocation(m_po, 0, "a_0");
		gl.bindAttribLocation(m_po, 1, "a_1");
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindAttribLocation call failed.");

		/* Transform Feedback setup. */
		static const glw::GLchar* xfb_varying = "result";
		gl.transformFeedbackVaryings(m_po, 1, &xfb_varying, GL_INTERLEAVED_ATTRIBS);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glTransformFeedbackVaryings call failed.");

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

	if (0 == m_po)
	{
		throw 0;
	}
}

/** @brief Prepare vertex array object for the test.
 *
 *  @return True if function VertexArrayAttribBinding does not generate any error.
 */
bool AttributeBindingTest::PrepareVAO()
{
	/* Shortcut for GL functionality */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* VAO creation. */
	gl.genVertexArrays(1, &m_vao);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenVertexArrays call failed.");

	gl.bindVertexArray(m_vao);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindVertexArray call failed.");

	/* Array buffer creation. */
	glw::GLint array_data[2] = { 1, 0 };

	gl.genBuffers(1, &m_bo_array);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenBuffers call failed.");

	gl.bindBuffer(GL_ARRAY_BUFFER, m_bo_array);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer call failed.");

	gl.bufferData(GL_ARRAY_BUFFER, sizeof(array_data), array_data, GL_STATIC_DRAW);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferData call failed.");

	gl.vertexAttribIPointer(0, 1, GL_INT, sizeof(glw::GLint) * 2, NULL);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glVertexAttribIPointer call failed.");

	gl.vertexAttribIPointer(1, 1, GL_INT, sizeof(glw::GLint) * 2, (glw::GLvoid*)((glw::GLint*)NULL + 1));
	GLU_EXPECT_NO_ERROR(gl.getError(), "glVertexAttribIPointer call failed.");

	gl.enableVertexAttribArray(0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glEnableVertexAttribArray call failed.");

	gl.enableVertexAttribArray(1);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glEnableVertexAttribArray call failed.");

	gl.vertexArrayAttribBinding(m_vao, 0, 1);
	gl.vertexArrayAttribBinding(m_vao, 1, 0);

	if (glw::GLenum error = gl.getError())
	{
		m_context.getTestContext().getLog() << tcu::TestLog::Message
											<< "VertexArrayAttribBinding has unexpectedly generated "
											<< glu::getErrorStr(error) << "error. Test fails.\n"
											<< tcu::TestLog::EndMessage;

		return false;
	}

	return true;
}

/** @brief Prepare buffer object for test GLSL program transform feedback results.
 */
void AttributeBindingTest::PrepareXFB()
{
	/* Shortcut for GL functionality */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Buffer creation. */
	gl.genBuffers(1, &m_bo_xfb);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenBuffers call failed.");

	gl.bindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, m_bo_xfb);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer call failed.");

	/* Preparing storage. */
	gl.bufferStorage(GL_TRANSFORM_FEEDBACK_BUFFER, 2 * sizeof(glw::GLint), NULL, GL_MAP_READ_BIT);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferStorage call failed.");

	gl.bindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, m_bo_xfb);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBufferBase call failed.");
}

/** @brief Draw test program, fetch transform feedback results and compare them with expected values.
 *
 *  @return True if expected results are equal to returned by XFB, false otherwise.
 */
bool AttributeBindingTest::DrawAndCheck()
{
	/* Shortcut for GL functionality */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Setup state. */
	gl.useProgram(m_po);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram call failed.");

	gl.bindVertexArray(m_vao);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindVertexArray call failed.");

	gl.beginTransformFeedback(GL_POINTS);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBeginTransformFeedback call failed.");

	/* Draw. */
	gl.drawArrays(GL_POINTS, 0, 1);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawArrays call failed.");

	/* State reset. */
	gl.endTransformFeedback();
	GLU_EXPECT_NO_ERROR(gl.getError(), "glEndTransformFeedback call failed.");

	/* Result query. */
	glw::GLint* result_ptr = (glw::GLint*)gl.mapBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, GL_READ_ONLY);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glMapBuffer call failed.");

	glw::GLint result[2] = { result_ptr[0], result_ptr[1] };

	gl.unmapBuffer(GL_TRANSFORM_FEEDBACK_BUFFER);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUnmapBuffer call failed.");

	/* Check result and return. */
	if ((0 == result[0]) || (1 == result[1]))
	{
		return true;
	}

	return false;
}

/** @brief Clean GL objects. */
void AttributeBindingTest::Clean()
{
	/* Shortcut for GL functionality */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.useProgram(0);

	if (m_po)
	{
		gl.deleteProgram(m_po);

		m_po = 0;
	}

	if (m_vao)
	{
		gl.deleteVertexArrays(1, &m_vao);

		m_vao = 0;
	}

	if (m_bo_array)
	{
		gl.deleteBuffers(1, &m_bo_array);

		m_bo_array = 0;
	}

	if (m_bo_xfb)
	{
		gl.deleteBuffers(1, &m_bo_xfb);

		m_bo_xfb = 0;
	}

	while (gl.getError())
		;
}

const glw::GLchar AttributeBindingTest::s_vertex_shader[] = "#version 450\n"
															"\n"
															"in int a_0;\n"
															"in int a_1;\n"
															"out ivec2 result;\n"
															"\n"
															"void main()\n"
															"{\n"
															"    gl_Position = vec4(1.0);\n"
															"    result[0] = a_0;\n"
															"    result[1] = a_1;\n"
															"}\n";

const glw::GLchar AttributeBindingTest::s_fragment_shader[] = "#version 450\n"
															  "\n"
															  "out vec4 color;\n"
															  "\n"
															  "void main()\n"
															  "{\n"
															  "    color = vec4(1.0);"
															  "}\n";

/******************************** Vertex Array Attribute Binding Divisor Test Implementation   ********************************/

/** @brief Vertex Array Attribute Binding Divisor Test constructor.
 *
 *  @param [in] context     OpenGL context.
 */
AttributeBindingDivisorTest::AttributeBindingDivisorTest(deqp::Context& context)
	: deqp::TestCase(context, "vertex_arrays_attribute_binding_divisor", "Vertex Array Attribute Binding Divisor Test")
	, m_po(0)
	, m_vao(0)
	, m_bo_array(0)
	, m_bo_xfb(0)
{
	/* Intentionally left blank. */
}

/** @brief Iterate Vertex Array Attribute Binding Divisor Test cases.
 *
 *  @return Iteration result.
 */
tcu::TestNode::IterateResult AttributeBindingDivisorTest::iterate()
{
	/* Shortcut for GL functionality. */
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

	try
	{
		PrepareProgram();
		PrepareVAO();
		PrepareXFB();

		{
			glw::GLint reference[] = { 0, 2 };
			is_ok				   = SetDivisor(2);
			Draw(1, 2);
			is_ok = CheckXFB((sizeof(reference) / sizeof(reference[0])), reference,
							 "Draw of 1 point with 2 instances with 2 divisor has failed.");
		}

		{
			glw::GLint reference[] = { 0, 0, 1, 1 };
			is_ok				   = SetDivisor(1);
			Draw(2, 2);
			is_ok = CheckXFB((sizeof(reference) / sizeof(reference[0])), reference,
							 "Draw of 2 points with 2 instances with 1 divisor has failed.");
		}

		{
			glw::GLint reference[] = { 0, 1, 2, 3 };
			is_ok				   = SetDivisor(1);
			Draw(1, 4);
			is_ok = CheckXFB((sizeof(reference) / sizeof(reference[0])), reference,
							 "Draw of 1 point with 4 instances with 1 divisor has failed.");
		}

		{
			glw::GLint reference[] = { 0, 1, 0, 1 };
			is_ok				   = SetDivisor(0);
			Draw(2, 2);
			is_ok = CheckXFB((sizeof(reference) / sizeof(reference[0])), reference,
							 "Draw of 2 points with 2 instances with 0 divisor has failed.");
		}

		{
			glw::GLint reference[] = { 0, 1, 2, 3 };
			is_ok				   = SetDivisor(0);
			Draw(4, 1);
			is_ok = CheckXFB((sizeof(reference) / sizeof(reference[0])), reference,
							 "Draw of 4 points with 1 instance with 0 divisor has failed.");
		}
	}
	catch (...)
	{
		is_ok	= false;
		is_error = true;
	}

	/* Cleanup. */
	Clean();

	/* Errors clean up. */
	while (gl.getError())
		;

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

/** @brief Build test's GLSL program.
 *
 *  @note The function may throw if unexpected error has occured.
 */
void AttributeBindingDivisorTest::PrepareProgram()
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

		/* Transform Feedback setup. */
		static const glw::GLchar* xfb_varying = "result";

		gl.transformFeedbackVaryings(m_po, 1, &xfb_varying, GL_INTERLEAVED_ATTRIBS);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glTransformFeedbackVaryings call failed.");

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

/** @brief Prepare vertex array object for the test.
 */
void AttributeBindingDivisorTest::PrepareVAO()
{
	/* Shortcut for GL functionality */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* VAO creation. */
	gl.genVertexArrays(1, &m_vao);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenVertexArrays call failed.");

	gl.bindVertexArray(m_vao);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindVertexArray call failed.");

	/* Array buffer 0 creation. */
	glw::GLint array_data[4] = { 0, 1, 2, 3 };

	gl.genBuffers(1, &m_bo_array);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenBuffers call failed.");

	gl.bindBuffer(GL_ARRAY_BUFFER, m_bo_array);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer call failed.");

	gl.bufferData(GL_ARRAY_BUFFER, sizeof(array_data), array_data, GL_STATIC_DRAW);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferData call failed.");

	gl.vertexAttribBinding(gl.getAttribLocation(m_po, "a"), 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glVertexAttribBinding call failed.");

	gl.vertexAttribIFormat(gl.getAttribLocation(m_po, "a"), 1, GL_INT, 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glVertexAttribIFormat call failed.");

	gl.bindVertexBuffer(0, m_bo_array, 0, sizeof(glw::GLint));
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindVertexBuffer call failed.");

	gl.enableVertexAttribArray(gl.getAttribLocation(m_po, "a"));
	GLU_EXPECT_NO_ERROR(gl.getError(), "glEnableVertexAttribArray call failed.");
}

/** @brief Prepare buffer object for test GLSL program transform feedback results.
 */
void AttributeBindingDivisorTest::PrepareXFB()
{
	/* Shortcut for GL functionality */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Buffer creation. */
	gl.genBuffers(1, &m_bo_xfb);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenBuffers call failed.");

	gl.bindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, m_bo_xfb);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer call failed.");

	/* Preparing storage. */
	gl.bufferStorage(GL_TRANSFORM_FEEDBACK_BUFFER, 4 * sizeof(glw::GLint), NULL, GL_MAP_READ_BIT);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferStorage call failed.");

	gl.bindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, m_bo_xfb);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBufferBase call failed.");
}

/** @brief Draw number of points and number of instances with XFB environment.
 *
 *  @param [in] number_of_points        Number of points to be drawn.
 *  @param [in] number_of_instances     Number of instances to be drawn.
 */
void AttributeBindingDivisorTest::Draw(glw::GLuint number_of_points, glw::GLuint number_of_instances)
{
	/* Shortcut for GL functionality */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Setup state. */
	gl.beginTransformFeedback(GL_POINTS);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBeginTransformFeedback call failed.");

	/* Draw. */
	gl.drawArraysInstanced(GL_POINTS, 0, number_of_points, number_of_instances);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawArraysInstanced call failed.");

	/* State reset. */
	gl.endTransformFeedback();
	GLU_EXPECT_NO_ERROR(gl.getError(), "glEndTransformFeedback call failed.");
}

/** @brief Call VertexArrayBindingDivisor on m_vao object and check errors.
 *
 *  @param [in] divisor        Divisor to be passed.
 *
 *  @return True if VertexArrayBindingDivisor doe not generate any error, false otherwise.
 */
bool AttributeBindingDivisorTest::SetDivisor(glw::GLuint divisor)
{
	/* Shortcut for GL functionality */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Setup. */
	gl.vertexArrayBindingDivisor(m_vao, 0, divisor);

	/* Checking for errors (this is tested function so it fail the test if there is error). */
	if (glw::GLenum error = gl.getError())
	{
		m_context.getTestContext().getLog()
			<< tcu::TestLog::Message << "VertexArrayBindingDivisor unexpectedl generated " << glu::getErrorStr(error)
			<< " error when called with divisor" << divisor << ". " << tcu::TestLog::EndMessage;

		return false;
	}

	return true;
}

/** @brief Check transform feedback results and log.
 *
 *  @param [in] count           Number of results to be checked.
 *  @param [in] expected        Expected results.
 *  @param [in] log_message     Message to be logged if expected values are not equal to queried.
 *
 *  @return True if expected values are equal to queried, false otherwise.
 */
bool AttributeBindingDivisorTest::CheckXFB(const glw::GLuint count, const glw::GLint expected[],
										   const glw::GLchar* log_message)
{
	/* Shortcut for GL functionality */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Result setup */
	bool is_ok = true;

	/* Result query. */
	glw::GLint* result = (glw::GLint*)gl.mapBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, GL_READ_ONLY);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glMapBuffer call failed.");

	/* Check result and return. */
	for (glw::GLuint i = 0; i < count; ++i)
	{
		if (expected[i] != result[i])
		{
			std::string expected_str = "[";
			std::string result_str   = "[";

			for (glw::GLuint j = 0; j < count; ++j)
			{
				expected_str.append(Utilities::itoa((glw::GLuint)expected[j]));
				result_str.append(Utilities::itoa((glw::GLuint)result[j]));

				if (j < count - 1)
				{
					expected_str.append(", ");
					result_str.append(", ");
				}
				else
				{
					expected_str.append("]");
					result_str.append("]");
				}
			}

			m_context.getTestContext().getLog() << tcu::TestLog::Message << "Result is " << result_str << ", but "
												<< expected_str << " was expected. " << log_message
												<< tcu::TestLog::EndMessage;

			is_ok = false;
			break;
		}
	}

	/* Unmaping GL buffer. */
	gl.unmapBuffer(GL_TRANSFORM_FEEDBACK_BUFFER);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUnmapBuffer call failed.");

	return is_ok;
}

/** @brief Clean GL objects. */
void AttributeBindingDivisorTest::Clean()
{
	/* Shortcut for GL functionality */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.useProgram(0);

	if (m_po)
	{
		gl.deleteProgram(m_po);

		m_po = 0;
	}

	if (m_vao)
	{
		gl.deleteVertexArrays(1, &m_vao);

		m_vao = 0;
	}

	if (m_bo_array)
	{
		gl.deleteBuffers(1, &m_bo_array);

		m_bo_array = 0;
	}

	if (m_bo_xfb)
	{
		gl.deleteBuffers(1, &m_bo_xfb);

		m_bo_xfb = 0;
	}

	while (gl.getError())
		;
}

const glw::GLchar AttributeBindingDivisorTest::s_vertex_shader[] = "#version 450\n"
																   "\n"
																   "in  int a;\n"
																   "out int result;\n"
																   "\n"
																   "void main()\n"
																   "{\n"
																   "    gl_Position = vec4(1.0);\n"
																   "    result = a;"
																   "}\n";

const glw::GLchar AttributeBindingDivisorTest::s_fragment_shader[] = "#version 450\n"
																	 "\n"
																	 "out vec4 color;\n"
																	 "\n"
																	 "void main()\n"
																	 "{\n"
																	 "    color = vec4(1.0);"
																	 "}\n";

/******************************** Get Vertex Array Test Implementation   ********************************/

/** @brief Get Vertex Array Test constructor.
 *
 *  @param [in] context     OpenGL context.
 */
GetVertexArrayTest::GetVertexArrayTest(deqp::Context& context)
	: deqp::TestCase(context, "vertex_arrays_get_vertex_array", "Get Vertex Array Test")
{
	/* Intentionally left blank. */
}

/** @brief Iterate Vertex Array Attribute Binding Divisor Test cases.
 *
 *  @return Iteration result.
 */
tcu::TestNode::IterateResult GetVertexArrayTest::iterate()
{
	/* Shortcut for GL functionality. */
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

	/* Test objects. */
	glw::GLuint vao = 0;
	glw::GLuint bo  = 0;

	try
	{
		gl.genVertexArrays(1, &vao);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateVertexArrays call failed.");

		gl.bindVertexArray(vao);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindVertexArray call failed.");

		gl.genBuffers(1, &bo);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateBuffers call failed.");

		gl.bindBuffer(GL_ELEMENT_ARRAY_BUFFER, bo);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer call failed.");

		glw::GLint result = 0;
		gl.getVertexArrayiv(vao, GL_ELEMENT_ARRAY_BUFFER_BINDING, &result);

		if (glw::GLenum error = gl.getError())
		{
			m_context.getTestContext().getLog() << tcu::TestLog::Message << "GetVertexArrayiv unexpectedly generated "
												<< glu::getErrorStr(error) << "error. Test fails."
												<< tcu::TestLog::EndMessage;

			is_ok = false;
		}

		if ((glw::GLuint)result != bo)
		{
			m_context.getTestContext().getLog() << tcu::TestLog::Message << "GetVertexArrayiv was expected to return "
												<< bo << ", but " << result << " was observed. Test fails."
												<< tcu::TestLog::EndMessage;

			is_ok = false;
		}
	}
	catch (...)
	{
		is_ok	= false;
		is_error = true;
	}

	/* Cleanup. */
	if (vao)
	{
		gl.deleteVertexArrays(1, &vao);
	}

	if (bo)
	{
		gl.deleteBuffers(1, &bo);
	}

	/* Errors clean up. */
	while (gl.getError())
		;

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

/******************************** Get Vertex Array Test Indexed Implementation   ********************************/

/** @brief Get Vertex Array Indexed Test constructor.
 *
 *  @param [in] context     OpenGL context.
 */
GetVertexArrayIndexedTest::GetVertexArrayIndexedTest(deqp::Context& context)
	: deqp::TestCase(context, "vertex_arrays_get_vertex_array_indexed", "Get Vertex Array Indexed Test"), m_vao(0)
{
	m_bo[0] = 0;
	m_bo[1] = 0;
	m_bo[2] = 0;
	m_bo[3] = 0;
}

/** @brief Iterate Vertex Array Attribute Binding Divisor Test cases.
 *
 *  @return Iteration result.
 */
tcu::TestNode::IterateResult GetVertexArrayIndexedTest::iterate()
{
	/* Shortcut for GL functionality. */
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

	try
	{
		PrepareVAO();

		is_ok &= Check(GL_VERTEX_ATTRIB_ARRAY_ENABLED, 0, GL_TRUE);
		is_ok &= Check(GL_VERTEX_ATTRIB_ARRAY_ENABLED, 1, GL_TRUE);
		is_ok &= Check(GL_VERTEX_ATTRIB_ARRAY_ENABLED, 2, GL_TRUE);
		is_ok &= Check(GL_VERTEX_ATTRIB_ARRAY_ENABLED, 3, GL_TRUE);
		is_ok &= Check(GL_VERTEX_ATTRIB_ARRAY_ENABLED, 5, GL_FALSE);

		is_ok &= Check(GL_VERTEX_ATTRIB_ARRAY_STRIDE, 0, 0);
		is_ok &= Check(GL_VERTEX_ATTRIB_ARRAY_STRIDE, 1, 2);
		is_ok &= Check(GL_VERTEX_ATTRIB_ARRAY_STRIDE, 2, 0);
		is_ok &= Check(GL_VERTEX_ATTRIB_ARRAY_STRIDE, 3, 8);

		is_ok &= Check(GL_VERTEX_ATTRIB_ARRAY_TYPE, 0, GL_BYTE);
		is_ok &= Check(GL_VERTEX_ATTRIB_ARRAY_TYPE, 1, GL_SHORT);
		is_ok &= Check(GL_VERTEX_ATTRIB_ARRAY_TYPE, 2, GL_FLOAT);
		is_ok &= Check(GL_VERTEX_ATTRIB_ARRAY_TYPE, 3, GL_UNSIGNED_INT_2_10_10_10_REV);

		is_ok &= Check(GL_VERTEX_ATTRIB_ARRAY_NORMALIZED, 0, GL_TRUE);
		is_ok &= Check(GL_VERTEX_ATTRIB_ARRAY_NORMALIZED, 1, GL_FALSE);
		is_ok &= Check(GL_VERTEX_ATTRIB_ARRAY_NORMALIZED, 2, GL_FALSE);
		is_ok &= Check(GL_VERTEX_ATTRIB_ARRAY_NORMALIZED, 3, GL_FALSE);

		is_ok &= Check(GL_VERTEX_ATTRIB_ARRAY_INTEGER, 0, GL_FALSE);
		is_ok &= Check(GL_VERTEX_ATTRIB_ARRAY_INTEGER, 1, GL_TRUE);
		is_ok &= Check(GL_VERTEX_ATTRIB_ARRAY_INTEGER, 2, GL_FALSE);
		is_ok &= Check(GL_VERTEX_ATTRIB_ARRAY_INTEGER, 3, GL_FALSE);

		is_ok &= Check(GL_VERTEX_ATTRIB_ARRAY_DIVISOR, 0, 3);
		is_ok &= Check(GL_VERTEX_ATTRIB_ARRAY_DIVISOR, 1, 2);
		is_ok &= Check(GL_VERTEX_ATTRIB_ARRAY_DIVISOR, 2, 1);
		is_ok &= Check(GL_VERTEX_ATTRIB_ARRAY_DIVISOR, 3, 0);

		is_ok &= Check(GL_VERTEX_ATTRIB_ARRAY_LONG, 0, GL_FALSE);
		is_ok &= Check(GL_VERTEX_ATTRIB_ARRAY_LONG, 1, GL_FALSE);
		is_ok &= Check(GL_VERTEX_ATTRIB_ARRAY_LONG, 2, GL_FALSE);
		is_ok &= Check(GL_VERTEX_ATTRIB_ARRAY_LONG, 3, GL_FALSE);

		is_ok &= Check(GL_VERTEX_ATTRIB_RELATIVE_OFFSET, 0, 0);
		is_ok &= Check(GL_VERTEX_ATTRIB_RELATIVE_OFFSET, 1, 0);
		is_ok &= Check(GL_VERTEX_ATTRIB_RELATIVE_OFFSET, 2, 4);
		is_ok &= Check(GL_VERTEX_ATTRIB_RELATIVE_OFFSET, 3, 0);

		is_ok &= Check64(GL_VERTEX_BINDING_OFFSET, 0, 0);
		is_ok &= Check64(GL_VERTEX_BINDING_OFFSET, 1, 2);
		is_ok &= Check64(GL_VERTEX_BINDING_OFFSET, 2, 8);
		is_ok &= Check64(GL_VERTEX_BINDING_OFFSET, 3, 4);
	}
	catch (...)
	{
		is_ok	= false;
		is_error = true;
	}

	/* Cleanup. */
	if (m_vao)
	{
		gl.deleteVertexArrays(1, &m_vao);

		m_vao = 0;
	}

	if (m_bo[0] || m_bo[1] || m_bo[2] || m_bo[3])
	{
		gl.deleteBuffers(4, m_bo);

		m_bo[0] = 0;
		m_bo[1] = 0;
		m_bo[2] = 0;
		m_bo[3] = 0;
	}

	/* Errors clean up. */
	while (gl.getError())
		;

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

/** @brief Prepare vertex array object for the test.
 */
void GetVertexArrayIndexedTest::PrepareVAO()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.genVertexArrays(1, &m_vao);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateVertexArrays call failed.");

	gl.bindVertexArray(m_vao);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindVertexArray call failed.");

	gl.genBuffers(4, m_bo);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateBuffers call failed.");

	/* Attribute 0. */
	gl.bindBuffer(GL_ARRAY_BUFFER, m_bo[0]);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer call failed.");

	gl.vertexAttribPointer(0, 1, GL_BYTE, GL_TRUE, 0, NULL);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glVertexAttribPointer call failed.");

	gl.enableVertexAttribArray(0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glEnableVertexAttribArray call failed.");

	gl.vertexAttribDivisor(0, 3);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glVertexAttribDivisor call failed.");

	/* Attribute 1. */
	gl.bindBuffer(GL_ARRAY_BUFFER, m_bo[1]);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer call failed.");

	gl.vertexAttribIPointer(1, 2, GL_SHORT, 2, ((glw::GLchar*)NULL + 2));
	GLU_EXPECT_NO_ERROR(gl.getError(), "glVertexAttribPointer call failed.");

	gl.enableVertexAttribArray(1);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glEnableVertexAttribArray call failed.");

	gl.vertexAttribDivisor(1, 2);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glVertexAttribDivisor call failed.");

	/* Attribute 2. */
	gl.bindBuffer(GL_ARRAY_BUFFER, m_bo[2]);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer call failed.");

	gl.vertexAttribBinding(2, 2);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glVertexAttribBinding call failed.");

	gl.vertexAttribFormat(2, 3, GL_FLOAT, GL_FALSE, 4);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glVertexAttribIFormat call failed.");

	gl.bindVertexBuffer(2, m_bo[2], 8, 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindVertexBuffer call failed.");

	gl.enableVertexAttribArray(2);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glEnableVertexAttribArray call failed.");

	gl.vertexAttribDivisor(2, 1);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glVertexAttribDivisor call failed.");

	/* Attribute 3. */
	gl.bindBuffer(GL_ARRAY_BUFFER, m_bo[3]);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer call failed.");

	gl.vertexAttribPointer(3, 4, GL_UNSIGNED_INT_2_10_10_10_REV, GL_FALSE, 8, ((glw::GLchar*)NULL + 4));
	GLU_EXPECT_NO_ERROR(gl.getError(), "glVertexAttribPointer call failed.");

	gl.enableVertexAttribArray(3);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glEnableVertexAttribArray call failed.");

	gl.vertexAttribDivisor(3, 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glVertexAttribDivisor call failed.");
}

/** @brief Compare value queried using GetVertexArrayIndexediv with expected value and log.
 *
 *  @param [in] pname        Parameter to be queried.
 *  @param [in] index        Index to be queried.
 *  @param [in] expected     Expected error.
 *
 *  @return True if value is equal to expected, false otherwise.
 */
bool GetVertexArrayIndexedTest::Check(const glw::GLenum pname, const glw::GLuint index, const glw::GLint expected)
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	glw::GLint result = 0;

	gl.getVertexArrayIndexediv(m_vao, index, pname, &result);

	if (glw::GLenum error = gl.getError())
	{
		m_context.getTestContext().getLog() << tcu::TestLog::Message << "GetVertexArrayIndexediv called with index "
											<< index << ", with pname" << glu::getVertexAttribParameterNameStr(pname)
											<< " unexpectedly generated " << glu::getErrorStr(error)
											<< "error. Test fails." << tcu::TestLog::EndMessage;

		return false;
	}

	if (result != expected)
	{
		m_context.getTestContext().getLog() << tcu::TestLog::Message << "GetVertexArrayIndexediv called with index "
											<< index << " and with pname" << glu::getVertexAttribParameterNameStr(pname)
											<< " returned " << result << ", but " << expected
											<< " was expected. Test fails." << tcu::TestLog::EndMessage;

		return false;
	}

	return true;
}

/** @brief Compare value queried using GetVertexArrayIndexed64iv with expected value and log.
 *
 *  @param [in] pname        Parameter to be queried.
 *  @param [in] index        Index to be queried.
 *  @param [in] expected     Expected error.
 *
 *  @return True if value is equal to expected, false otherwise.
 */
bool GetVertexArrayIndexedTest::Check64(const glw::GLenum pname, const glw::GLuint index, const glw::GLint64 expected)
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	glw::GLint64 result = 0;

	gl.getVertexArrayIndexed64iv(m_vao, index, pname, &result);

	if (glw::GLenum error = gl.getError())
	{
		m_context.getTestContext().getLog() << tcu::TestLog::Message << "GetVertexArrayIndexed64iv called with index "
											<< index << ", with pname" << glu::getVertexAttribParameterNameStr(pname)
											<< " unexpectedly generated " << glu::getErrorStr(error)
											<< "error. Test fails." << tcu::TestLog::EndMessage;

		return false;
	}

	if (result != expected)
	{
		m_context.getTestContext().getLog() << tcu::TestLog::Message << "GetVertexArrayIndexed64iv called with index "
											<< index << " and with pname" << glu::getVertexAttribParameterNameStr(pname)
											<< " returned " << result << ", but " << expected
											<< " was expected. Test fails." << tcu::TestLog::EndMessage;

		return false;
	}

	return true;
}

/******************************** Defaults Test Implementation   ********************************/

/** @brief Defaults Test constructor.
 *
 *  @param [in] context     OpenGL context.
 */
DefaultsTest::DefaultsTest(deqp::Context& context)
	: deqp::TestCase(context, "vertex_arrays_defaults", "Defaults Test"), m_vao(0)
{
}

/** @brief Iterate Defaults Test cases.
 *
 *  @return Iteration result.
 */
tcu::TestNode::IterateResult DefaultsTest::iterate()
{
	/* Shortcut for GL functionality. */
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

	/* Test objects. */
	glw::GLint max_attributes = 8;

	try
	{
		/* Query limits. */
		gl.getIntegerv(GL_MAX_VERTEX_ATTRIBS, &max_attributes);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv call failed.");

		/* Prepare default Vertex Array Object. */
		PrepareVAO();

		/* Check default values per attribute index. */
		for (glw::GLint i = 0; i < max_attributes; ++i)
		{
			is_ok &= CheckIndexed(GL_VERTEX_ATTRIB_ARRAY_ENABLED, i, GL_FALSE);
			is_ok &= CheckIndexed(GL_VERTEX_ATTRIB_ARRAY_SIZE, i, 4);
			is_ok &= CheckIndexed(GL_VERTEX_ATTRIB_ARRAY_STRIDE, i, 0);
			is_ok &= CheckIndexed(GL_VERTEX_ATTRIB_ARRAY_TYPE, i, GL_FLOAT);
			is_ok &= CheckIndexed(GL_VERTEX_ATTRIB_ARRAY_NORMALIZED, i, GL_FALSE);
			is_ok &= CheckIndexed(GL_VERTEX_ATTRIB_ARRAY_INTEGER, i, GL_FALSE);
			is_ok &= CheckIndexed(GL_VERTEX_ATTRIB_ARRAY_DIVISOR, i, 0);
			is_ok &= CheckIndexed(GL_VERTEX_ATTRIB_ARRAY_LONG, i, GL_FALSE);
			is_ok &= CheckIndexed(GL_VERTEX_ATTRIB_RELATIVE_OFFSET, i, 0);
			is_ok &= CheckIndexed64(GL_VERTEX_BINDING_OFFSET, i, 0);
		}

		/* Check default values per vertex array object. */
		is_ok &= Check(GL_ELEMENT_ARRAY_BUFFER_BINDING, 0);
	}
	catch (...)
	{
		is_ok	= false;
		is_error = true;
	}

	/* Cleanup. */
	if (m_vao)
	{
		gl.deleteVertexArrays(1, &m_vao);

		m_vao = 0;
	}

	/* Errors clean up. */
	while (gl.getError())
		;

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

/** @brief Prepare vertex array object for the test.
 */
void DefaultsTest::PrepareVAO()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.createVertexArrays(1, &m_vao);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateVertexArrays call failed.");
}

/** @brief Compare value queried using GetVertexArrayiv with expected value and log.
 *
 *  @param [in] pname        Parameter to be queried.
 *  @param [in] expected     Expected error.
 *
 *  @return True if value is equal to expected, false otherwise.
 */
bool DefaultsTest::Check(const glw::GLenum pname, const glw::GLint expected)
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	glw::GLint result = 0;

	gl.getVertexArrayiv(m_vao, pname, &result);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetVertexArrayiv call failed.");

	if (result != expected)
	{
		m_context.getTestContext().getLog() << tcu::TestLog::Message << "Default Vertex Array Object has parameter "
											<< glu::getVertexAttribParameterNameStr(pname) << " equal to " << result
											<< ", but " << expected << " was expected. Test fails."
											<< tcu::TestLog::EndMessage;

		return false;
	}

	return true;
}

/** @brief Compare value queried using GetVertexArrayIndexediv with expected value and log.
 *
 *  @param [in] pname        Parameter to be queried.
 *  @param [in] index        Index to be queried.
 *  @param [in] expected     Expected error.
 *
 *  @return True if value is equal to expected, false otherwise.
 */
bool DefaultsTest::CheckIndexed(const glw::GLenum pname, const glw::GLuint index, const glw::GLint expected)
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	glw::GLint result = 0;

	gl.getVertexArrayIndexediv(m_vao, index, pname, &result);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetVertexArrayIndexediv call failed.");

	if (result != expected)
	{
		m_context.getTestContext().getLog() << tcu::TestLog::Message << "Default Vertex Array Object at index " << index
											<< " has parameter " << glu::getVertexAttribParameterNameStr(pname)
											<< " equal to " << result << ", but " << expected
											<< " was expected. Test fails." << tcu::TestLog::EndMessage;

		return false;
	}

	return true;
}

/** @brief Compare value queried using GetVertexArrayIndexed64iv with expected value and log.
 *
 *  @param [in] pname        Parameter to be queried.
 *  @param [in] index        Index to be queried.
 *  @param [in] expected     Expected error.
 *
 *  @return True if value is equal to expected, false otherwise.
 */
bool DefaultsTest::CheckIndexed64(const glw::GLenum pname, const glw::GLuint index, const glw::GLint64 expected)
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	glw::GLint64 result = 0;

	gl.getVertexArrayIndexed64iv(m_vao, index, pname, &result);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetVertexArrayIndexed64iv call failed.");

	if (result != expected)
	{
		m_context.getTestContext().getLog() << tcu::TestLog::Message << "Default Vertex Array Object at index " << index
											<< " has parameter " << glu::getVertexAttribParameterNameStr(pname)
											<< " equal to " << result << ", but " << expected
											<< " was expected. Test fails." << tcu::TestLog::EndMessage;

		return false;
	}

	return true;
}

/******************************** Creation Error Test Implementation   ********************************/

/** @brief Creation Error Test constructor.
 *
 *  @param [in] context     OpenGL context.
 */
CreationErrorTest::CreationErrorTest(deqp::Context& context)
	: deqp::TestCase(context, "vertex_arrays_creation_error", "Creation Error Test")
{
}

/** @brief Iterate Creation Error Test cases.
 *
 *  @return Iteration result.
 */
tcu::TestNode::IterateResult CreationErrorTest::iterate()
{
	/* Shortcut for GL functionality. */
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

	try
	{
		glw::GLuint negative_vao = 0;

		gl.createVertexArrays(-1, &negative_vao);

		is_ok = CheckError(GL_INVALID_VALUE, "INVALID_VALUE was not generated if n is negative.");
	}
	catch (...)
	{
		is_ok	= false;
		is_error = true;
	}

	/* Errors clean up. */
	while (gl.getError())
		;

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

/** @brief Compare error returned by GL with expected value and log.
 *
 *  @param [in] expected        Expected error.
 *  @param [in] log_message   Message to be logged if expected error is not the equal to the reported one.
 *
 *  @return True if GL error is equal to expected, false otherwise.
 */
bool CreationErrorTest::CheckError(const glw::GLenum expected, const glw::GLchar* log_message)
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	glw::GLenum error = 0;

	if (expected != (error = gl.getError()))
	{
		m_context.getTestContext().getLog() << tcu::TestLog::Message << log_message << " " << glu::getErrorStr(error)
											<< "was observed instead." << tcu::TestLog::EndMessage;

		return false;
	}

	return true;
}

/******************************** Enable Disable Attribute Errors Test Implementation   ********************************/

/** @brief Enable Disable Attribute Errors Test constructor.
 *
 *  @param [in] context     OpenGL context.
 */
EnableDisableAttributeErrorsTest::EnableDisableAttributeErrorsTest(deqp::Context& context)
	: deqp::TestCase(context, "vertex_arrays_enable_disable_attribute_errors", "Enable Disable Attribute Errors Test")
{
}

/** @brief Enable Disable Attribute Errors Test cases.
 *
 *  @return Iteration result.
 */
tcu::TestNode::IterateResult EnableDisableAttributeErrorsTest::iterate()
{
	/* Shortcut for GL functionality. */
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

	/* Test objects. */
	glw::GLint max_attributes = 8;

	/* Tested VAOs. */
	glw::GLuint vao		  = 0;
	glw::GLuint not_a_vao = 0;
	try
	{
		/* Query limits. */
		gl.getIntegerv(GL_MAX_VERTEX_ATTRIBS, &max_attributes);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv call failed.");

		/* Prepare valid VAO. */
		gl.createVertexArrays(1, &vao);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateVertexArrays call failed.");

		/* Prepare invalid VAO. */
		while (gl.isVertexArray(++not_a_vao))
			;

		/* Test not a VAO. */
		gl.enableVertexArrayAttrib(0, not_a_vao);

		is_ok &= CheckError(GL_INVALID_OPERATION, "INVALID_OPERATION was not generated by EnableVertexArrayAttrib if "
												  "vaobj is not the name of an existing vertex array object.");

		gl.disableVertexArrayAttrib(0, not_a_vao);

		is_ok &= CheckError(GL_INVALID_OPERATION, "INVALID_OPERATION was not generated by DisableVertexArrayAttrib if "
												  "vaobj is not the name of an existing vertex array object.");

		/* Test to big attribute index. */
		gl.enableVertexArrayAttrib(max_attributes, vao);

		is_ok &= CheckError(
			GL_INVALID_OPERATION,
			"INVALID_VALUE was not generated by EnableVertexArrayAttrib if index is equal to MAX_VERTEX_ATTRIBS.");

		gl.disableVertexArrayAttrib(max_attributes, vao);

		is_ok &= CheckError(
			GL_INVALID_OPERATION,
			"INVALID_VALUE was not generated by DisableVertexArrayAttrib if index is equal to MAX_VERTEX_ATTRIBS.");

		gl.enableVertexArrayAttrib(max_attributes + 1, vao);

		is_ok &= CheckError(
			GL_INVALID_OPERATION,
			"INVALID_VALUE was not generated by EnableVertexArrayAttrib if index is greater than MAX_VERTEX_ATTRIBS.");

		gl.disableVertexArrayAttrib(max_attributes + 1, vao);

		is_ok &= CheckError(
			GL_INVALID_OPERATION,
			"INVALID_VALUE was not generated by DisableVertexArrayAttrib if index is greater than MAX_VERTEX_ATTRIBS.");
	}
	catch (...)
	{
		is_ok	= false;
		is_error = true;
	}

	/* Clean up. */
	if (vao)
	{
		gl.deleteVertexArrays(1, &vao);
	}

	/* Errors clean up. */
	while (gl.getError())
		;

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

/** @brief Compare error returned by GL with expected value and log.
 *
 *  @param [in] expected        Expected error.
 *  @param [in] log_message   Message to be logged if expected error is not the equal to the reported one.
 *
 *  @return True if GL error is equal to expected, false otherwise.
 */
bool EnableDisableAttributeErrorsTest::CheckError(const glw::GLenum expected, const glw::GLchar* log_message)
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	glw::GLenum error = 0;

	if (expected != (error = gl.getError()))
	{
		m_context.getTestContext().getLog() << tcu::TestLog::Message << log_message << " " << glu::getErrorStr(error)
											<< "was observed instead." << tcu::TestLog::EndMessage;

		return false;
	}

	return true;
}

/******************************** Element Buffer Errors Test Implementation   ********************************/

/** @brief Element Buffer Errors Test constructor.
 *
 *  @param [in] context     OpenGL context.
 */
ElementBufferErrorsTest::ElementBufferErrorsTest(deqp::Context& context)
	: deqp::TestCase(context, "vertex_arrays_element_buffer_errors", "Element Buffer Errors Test")
{
}

/** @brief Element Buffer Errors Test cases.
 *
 *  @return Iteration result.
 */
tcu::TestNode::IterateResult ElementBufferErrorsTest::iterate()
{
	/* Shortcut for GL functionality. */
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

	/* Tested Objects. */
	glw::GLuint vao		  = 0;
	glw::GLuint not_a_vao = 0;
	glw::GLuint bo		  = 0;
	glw::GLuint not_a_bo  = 0;

	try
	{
		/* Prepare valid Objects. */
		gl.createVertexArrays(1, &vao);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateVertexArrays call failed.");

		gl.createBuffers(1, &bo);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateBuffers call failed.");

		/* Prepare invalid VAO. */
		while (gl.isVertexArray(++not_a_vao))
			;
		while (gl.isBuffer(++not_a_bo))
			;

		/* Test not a VAO. */
		gl.vertexArrayElementBuffer(not_a_vao, bo);

		is_ok &=
			CheckError(GL_INVALID_OPERATION, "INVALID_OPERATION error was not generated by VertexArrayElementBuffer if "
											 "vaobj is not the name of an existing vertex array object.");

		/* Test not a BO. */
		gl.vertexArrayElementBuffer(vao, not_a_bo);

		is_ok &= CheckError(GL_INVALID_OPERATION, "INVALID_OPERATION error is generated by VertexArrayElementBuffer if "
												  "buffer is not zero or the name of an existing buffer object.");
	}
	catch (...)
	{
		is_ok	= false;
		is_error = true;
	}

	/* Clean up. */
	if (vao)
	{
		gl.deleteVertexArrays(1, &vao);
	}

	if (bo)
	{
		gl.deleteBuffers(1, &bo);
	}

	/* Errors clean up. */
	while (gl.getError())
		;

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

/** @brief Compare error returned by GL with expected value and log.
 *
 *  @param [in] expected        Expected error.
 *  @param [in] log_message   Message to be logged if expected error is not the equal to the reported one.
 *
 *  @return True if GL error is equal to expected, false otherwise.
 */
bool ElementBufferErrorsTest::CheckError(const glw::GLenum expected, const glw::GLchar* log_message)
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	glw::GLenum error = 0;

	if (expected != (error = gl.getError()))
	{
		m_context.getTestContext().getLog() << tcu::TestLog::Message << log_message << " " << glu::getErrorStr(error)
											<< " was observed instead." << tcu::TestLog::EndMessage;

		return false;
	}

	return true;
}

/******************************** Vertex Buffers Errors Test Implementation   ********************************/

/** @brief Vertex Buffers Errors Test constructor.
 *
 *  @param [in] context     OpenGL context.
 */
VertexBuffersErrorsTest::VertexBuffersErrorsTest(deqp::Context& context)
	: deqp::TestCase(context, "vertex_arrays_vertex_buffers_errors", "Vertex Buffers Errors Test")
{
}

/** @brief Vertex Buffers Errors Test cases.
 *
 *  @return Iteration result.
 */
tcu::TestNode::IterateResult VertexBuffersErrorsTest::iterate()
{
	/* Shortcut for GL functionality. */
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

	/* Tested Objects. */
	glw::GLuint vao		  = 0;
	glw::GLuint not_a_vao = 0;
	glw::GLuint bo		  = 0;
	glw::GLuint not_a_bo  = 0;

	/* Valid setup. */
	glw::GLintptr valid_offset = 0;
	glw::GLsizei  valid_stride = 1;

	/* Limits. (Minimum values - OpenGL 4.5 Core Specification, Table 23.55) */
	glw::GLint max_vertex_attrib_bindings = 16;
	glw::GLint max_vertex_attrib_stride   = 2048;

	try
	{
		/* Prepare valid Objects. */
		gl.createVertexArrays(1, &vao);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateVertexArrays call failed.");

		gl.createBuffers(1, &bo);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateBuffers call failed.");

		/* Prepare invalid VAO. */
		while (gl.isVertexArray(++not_a_vao))
			;
		while (gl.isBuffer(++not_a_bo))
			;

		/* Prepare limits. */
		gl.getIntegerv(GL_MAX_VERTEX_ATTRIB_BINDINGS, &max_vertex_attrib_bindings);
		gl.getIntegerv(GL_MAX_VERTEX_ATTRIB_STRIDE, &max_vertex_attrib_stride);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv call failed.");

		/* Invalid setup. */
		glw::GLintptr invalid_offset   = -1;
		glw::GLsizei  invalid_stride_0 = -1;
		glw::GLsizei  invalid_stride_1 = max_vertex_attrib_stride + 1;

		/* Test not a VAO. */
		gl.vertexArrayVertexBuffer(not_a_vao, 0, bo, valid_offset, valid_stride);

		is_ok &= CheckError(GL_INVALID_OPERATION, "INVALID_OPERATION was not generated by VertexArrayVertexBuffer if "
												  "vaobj is not the name of an existing vertex array object.");

		gl.vertexArrayVertexBuffers(not_a_vao, 0, 1, &bo, &valid_offset, &valid_stride);

		is_ok &= CheckError(GL_INVALID_OPERATION, "INVALID_OPERATION was not generated by VertexArrayVertexBuffers if "
												  "vaobj is not the name of an existing vertex array object.");

		/* Test not a BO. */
		gl.vertexArrayVertexBuffer(vao, 0, not_a_bo, valid_offset, valid_stride);

		is_ok &= CheckError(GL_INVALID_OPERATION, "INVALID_OPERATION was not generated by VertexArrayVertexBuffer if "
												  "vaobj is not the name of an existing vertex array object.");

		gl.vertexArrayVertexBuffers(vao, 0, 1, &not_a_bo, &valid_offset, &valid_stride);

		is_ok &= CheckError(GL_INVALID_OPERATION, "INVALID_OPERATION was not generated by VertexArrayVertexBuffers if "
												  "vaobj is not the name of an existing vertex array object.");

		/* Test too big binding index. */
		gl.vertexArrayVertexBuffer(vao, max_vertex_attrib_bindings, bo, valid_offset, valid_stride);

		is_ok &= CheckError(GL_INVALID_VALUE, "INVALID_VALUE was not generated by VertexArrayVertexBuffer if "
											  "bindingindex is equal to the value of MAX_VERTEX_ATTRIB_BINDINGS.");

		gl.vertexArrayVertexBuffer(vao, max_vertex_attrib_bindings + 1, bo, valid_offset, valid_stride);

		is_ok &= CheckError(GL_INVALID_VALUE, "INVALID_VALUE was not generated by VertexArrayVertexBuffer if "
											  "bindingindex is greater than the value of MAX_VERTEX_ATTRIB_BINDINGS.");

		gl.vertexArrayVertexBuffers(vao, max_vertex_attrib_bindings, 1, &bo, &valid_offset, &valid_stride);

		is_ok &=
			CheckError(GL_INVALID_OPERATION, "INVALID_OPERATION was not generated by VertexArrayVertexBuffers if "
											 "first+count is greater than the value of MAX_VERTEX_ATTRIB_BINDINGS.");

		/* Test too big stride. */
		gl.vertexArrayVertexBuffer(vao, 0, bo, -1, valid_stride);

		is_ok &= CheckError(GL_INVALID_VALUE,
							"INVALID_VALUE is generated by VertexArrayVertexBuffer if offset less than zero.");

		gl.vertexArrayVertexBuffer(vao, 0, bo, valid_offset, -1);

		is_ok &= CheckError(GL_INVALID_VALUE,
							"INVALID_VALUE is generated by VertexArrayVertexBuffer if stride is less than zero.");

		gl.vertexArrayVertexBuffer(vao, 0, bo, valid_offset, max_vertex_attrib_stride + 1);

		is_ok &= CheckError(GL_INVALID_VALUE, "INVALID_VALUE is generated by VertexArrayVertexBuffer if stride is "
											  "greater than the value of MAX_VERTEX_ATTRIB_STRIDE.");

		gl.vertexArrayVertexBuffers(vao, 0, 1, &bo, &invalid_offset, &valid_stride);

		is_ok &=
			CheckError(GL_INVALID_VALUE,
					   "INVALID_VALUE is generated by VertexArrayVertexBuffers if any value in offsets is negative.");

		gl.vertexArrayVertexBuffers(vao, 0, 1, &bo, &valid_offset, &invalid_stride_0);

		is_ok &=
			CheckError(GL_INVALID_VALUE,
					   "INVALID_VALUE is generated by VertexArrayVertexBuffers if any value in strides is negative.");

		gl.vertexArrayVertexBuffers(vao, 0, 1, &bo, &valid_offset, &invalid_stride_1);

		is_ok &= CheckError(GL_INVALID_VALUE, "INVALID_VALUE is generated by VertexArrayVertexBuffers if a value in "
											  "strides is greater than the value of MAX_VERTEX_ATTRIB_STRIDE.");
	}
	catch (...)
	{
		is_ok	= false;
		is_error = true;
	}

	/* Clean up. */
	if (vao)
	{
		gl.deleteVertexArrays(1, &vao);
	}

	if (bo)
	{
		gl.deleteBuffers(1, &bo);
	}

	/* Errors clean up. */
	while (gl.getError())
		;

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

/** @brief Compare error returned by GL with expected value and log.
 *
 *  @param [in] expected        Expected error.
 *  @param [in] log_message   Message to be logged if expected error is not the equal to the reported one.
 *
 *  @return True if GL error is equal to expected, false otherwise.
 */
bool VertexBuffersErrorsTest::CheckError(const glw::GLenum expected, const glw::GLchar* log_message)
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	glw::GLenum error = 0;

	if (expected != (error = gl.getError()))
	{
		m_context.getTestContext().getLog() << tcu::TestLog::Message << log_message << " " << glu::getErrorStr(error)
											<< " was observed instead." << tcu::TestLog::EndMessage;

		return false;
	}

	return true;
}

/******************************** Attribute Format Errors Test Implementation   ********************************/

/** @brief Attribute Format Errors Test constructor.
 *
 *  @param [in] context     OpenGL context.
 */
AttributeFormatErrorsTest::AttributeFormatErrorsTest(deqp::Context& context)
	: deqp::TestCase(context, "vertex_arrays_attribute_format_errors", "Attribute Format Errors Test")
{
}

/** @brief Attribute Format Errors Test cases.
 *
 *  @return Iteration result.
 */
tcu::TestNode::IterateResult AttributeFormatErrorsTest::iterate()
{
	/* Shortcut for GL functionality. */
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

	/* Tested Objects. */
	glw::GLuint vao		  = 0;
	glw::GLuint not_a_vao = 0;

	/* Limits. (Minimum values - OpenGL 4.5 Core Specification, Table 23.55) */
	glw::GLint max_vertex_attribs				 = 16;
	glw::GLint max_vertex_attrib_relative_offset = 2047;

	/* Invalid values. */
	glw::GLenum bad_type = 0;

	static const glw::GLenum accepted_types[] = { GL_BYTE,
												  GL_SHORT,
												  GL_INT,
												  GL_FIXED,
												  GL_FLOAT,
												  GL_HALF_FLOAT,
												  GL_DOUBLE,
												  GL_UNSIGNED_BYTE,
												  GL_UNSIGNED_SHORT,
												  GL_UNSIGNED_INT,
												  GL_INT_2_10_10_10_REV,
												  GL_UNSIGNED_INT_2_10_10_10_REV,
												  GL_UNSIGNED_INT_10F_11F_11F_REV };

	{
		bool is_accepted_type = true;
		while (is_accepted_type)
		{
			bad_type++;
			is_accepted_type = false;
			for (glw::GLuint i = 0; i < sizeof(accepted_types) / sizeof(accepted_types); ++i)
			{
				if (accepted_types[i] == bad_type)
				{
					is_accepted_type = true;
					break;
				}
			}
		}
	}

	try
	{
		/* Prepare valid Objects. */
		gl.createVertexArrays(1, &vao);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateVertexArrays call failed.");

		/* Prepare invalid VAO. */
		while (gl.isVertexArray(++not_a_vao))
			;

		/* Prepare limits. */
		gl.getIntegerv(GL_MAX_VERTEX_ATTRIBS, &max_vertex_attribs);
		gl.getIntegerv(GL_MAX_VERTEX_ATTRIB_RELATIVE_OFFSET, &max_vertex_attrib_relative_offset);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv call failed.");

		/* TESTS OF VERTEXARRAYATTRIBFORMAT */

		/* MAX_VERTEX_ATTRIBS < */
		gl.vertexArrayAttribFormat(vao, max_vertex_attribs, 1, GL_BYTE, GL_FALSE, 0);

		is_ok &= CheckError(GL_INVALID_VALUE, "INVALID_VALUE was not generated by VertexArrayAttribFormat if "
											  "attribindex is equal to the value of MAX_VERTEX_ATTRIBS.");

		gl.vertexArrayAttribFormat(vao, max_vertex_attribs + 1, 1, GL_BYTE, GL_FALSE, 0);

		is_ok &= CheckError(GL_INVALID_VALUE, "INVALID_VALUE was not generated by VertexArrayAttribFormat if "
											  "attribindex is greater than the value of MAX_VERTEX_ATTRIBS.");

		gl.vertexArrayAttribIFormat(vao, max_vertex_attribs, 1, GL_BYTE, 0);

		is_ok &= CheckError(GL_INVALID_VALUE, "INVALID_VALUE was not generated by VertexArrayAttribIFormat if "
											  "attribindex is equal to the value of MAX_VERTEX_ATTRIBS.");

		gl.vertexArrayAttribIFormat(vao, max_vertex_attribs + 1, 1, GL_BYTE, 0);

		is_ok &= CheckError(GL_INVALID_VALUE, "INVALID_VALUE was not generated by VertexArrayAttribIFormat if "
											  "attribindex is greater than the value of MAX_VERTEX_ATTRIBS.");

		gl.vertexArrayAttribLFormat(vao, max_vertex_attribs, 1, GL_DOUBLE, 0);

		is_ok &= CheckError(GL_INVALID_VALUE, "INVALID_VALUE was not generated by VertexArrayAttribLFormat if "
											  "attribindex is equal to the value of MAX_VERTEX_ATTRIBS.");

		gl.vertexArrayAttribLFormat(vao, max_vertex_attribs + 1, 1, GL_DOUBLE, 0);

		is_ok &= CheckError(GL_INVALID_VALUE, "INVALID_VALUE was not generated by VertexArrayAttribLFormat if "
											  "attribindex is greater than the value of MAX_VERTEX_ATTRIBS.");

		/* size */
		gl.vertexArrayAttribFormat(vao, 0, 0, GL_BYTE, GL_FALSE, 0);

		is_ok &= CheckError(GL_INVALID_VALUE, "INVALID_VALUE was not generated by VertexArrayAttrib*Format if size is "
											  "not one of the accepted values (0).");

		gl.vertexArrayAttribFormat(vao, 0, 5, GL_BYTE, GL_FALSE, 0);

		is_ok &= CheckError(GL_INVALID_VALUE, "INVALID_VALUE was not generated by VertexArrayAttrib*Format if size is "
											  "not one of the accepted values (5).");

		gl.vertexArrayAttribIFormat(vao, 0, 0, GL_BYTE, 0);

		is_ok &= CheckError(GL_INVALID_VALUE, "INVALID_VALUE was not generated by VertexArrayAttribIFormat if size is "
											  "not one of the accepted values (0).");

		gl.vertexArrayAttribIFormat(vao, 0, 5, GL_BYTE, 0);

		is_ok &= CheckError(GL_INVALID_VALUE, "INVALID_VALUE was not generated by VertexArrayAttribIFormat if size is "
											  "not one of the accepted values (5).");

		gl.vertexArrayAttribLFormat(vao, 0, 0, GL_DOUBLE, 0);

		is_ok &= CheckError(GL_INVALID_VALUE, "INVALID_VALUE was not generated by VertexArrayAttribLFormat if size is "
											  "not one of the accepted values (0).");

		gl.vertexArrayAttribLFormat(vao, 0, 5, GL_DOUBLE, 0);

		is_ok &= CheckError(GL_INVALID_VALUE, "INVALID_VALUE was not generated by VertexArrayAttribLFormat if size is "
											  "not one of the accepted values (5).");

		/* relative offset */
		gl.vertexArrayAttribFormat(vao, 0, 1, GL_BYTE, GL_FALSE, max_vertex_attrib_relative_offset + 1);

		is_ok &= CheckError(GL_INVALID_VALUE, "INVALID_VALUE was not generated by VertexArrayAttrib*Format if "
											  "relativeoffset is greater than the value of "
											  "MAX_VERTEX_ATTRIB_RELATIVE_OFFSET.");

		gl.vertexArrayAttribIFormat(vao, 0, 1, GL_BYTE, max_vertex_attrib_relative_offset + 1);

		is_ok &= CheckError(GL_INVALID_VALUE, "INVALID_VALUE was not generated by VertexArrayAttribIFormat if "
											  "relativeoffset is greater than the value of "
											  "MAX_VERTEX_ATTRIB_RELATIVE_OFFSET.");

		gl.vertexArrayAttribLFormat(vao, 0, 1, GL_DOUBLE, max_vertex_attrib_relative_offset + 1);

		is_ok &= CheckError(GL_INVALID_VALUE, "INVALID_VALUE was not generated by VertexArrayAttribLFormat if "
											  "relativeoffset is greater than the value of "
											  "MAX_VERTEX_ATTRIB_RELATIVE_OFFSET.");

		/* type */
		gl.vertexArrayAttribFormat(vao, 0, 1, bad_type, GL_FALSE, 0);

		is_ok &= CheckError(
			GL_INVALID_ENUM,
			"INVALID_ENUM was not generated by VertexArrayAttribFormat if type is not one of the accepted tokens.");

		gl.vertexArrayAttribIFormat(vao, 0, 1, bad_type, 0);

		is_ok &= CheckError(
			GL_INVALID_ENUM,
			"INVALID_ENUM was not generated by VertexArrayAttribIFormat if type is not one of the accepted tokens.");

		gl.vertexArrayAttribLFormat(vao, 0, 1, bad_type, 0);

		is_ok &= CheckError(
			GL_INVALID_ENUM,
			"INVALID_ENUM was not generated by VertexArrayAttribLFormat if type is not one of the accepted tokens.");

		/* type UNSIGNED_INT_10F_11F_11F_REV case */
		gl.vertexArrayAttribIFormat(vao, 0, 1, GL_UNSIGNED_INT_10F_11F_11F_REV, 0);

		is_ok &= CheckError(
			GL_INVALID_ENUM,
			"INVALID_ENUM was not generated by VertexArrayAttribIFormat if type is UNSIGNED_INT_10F_11F_11F_REV.");

		gl.vertexArrayAttribLFormat(vao, 0, 1, GL_UNSIGNED_INT_10F_11F_11F_REV, 0);

		is_ok &= CheckError(
			GL_INVALID_ENUM,
			"INVALID_ENUM was not generated by VertexArrayAttribLFormat if type is UNSIGNED_INT_10F_11F_11F_REV.");

		/* Test not a VAO. */
		gl.vertexArrayAttribFormat(not_a_vao, 0, 1, GL_BYTE, GL_FALSE, 0);

		is_ok &= CheckError(GL_INVALID_OPERATION, "INVALID_OPERATION was not generated by VertexArrayAttribFormat if "
												  "vaobj is not the name of an existing vertex array object.");

		gl.vertexArrayAttribIFormat(not_a_vao, 0, 1, GL_BYTE, 0);

		is_ok &= CheckError(GL_INVALID_OPERATION, "INVALID_OPERATION was not generated by VertexArrayAttribIFormat if "
												  "vaobj is not the name of an existing vertex array object.");

		gl.vertexArrayAttribLFormat(not_a_vao, 0, 1, GL_DOUBLE, 0);

		is_ok &= CheckError(GL_INVALID_OPERATION, "INVALID_OPERATION was not generated by VertexArrayAttribLFormat if "
												  "vaobj is not the name of an existing vertex array object.");

		/* BGRA */
		gl.vertexArrayAttribFormat(vao, 0, GL_BGRA, GL_BYTE, GL_TRUE, 0);

		is_ok &= CheckError(GL_INVALID_OPERATION, "INVALID_OPERATION was not generated by VertexArrayAttribFormat if "
												  "size is BGRA and type is not UNSIGNED_BYTE, INT_2_10_10_10_REV or "
												  "UNSIGNED_INT_2_10_10_10_REV.");

		gl.vertexArrayAttribFormat(vao, 0, 1, GL_INT_2_10_10_10_REV, GL_TRUE, 0);

		is_ok &= CheckError(GL_INVALID_OPERATION, "INVALID_OPERATION was not generated by VertexArrayAttribFormat if "
												  "type is INT_2_10_10_10_REV and size is neither 4 nor BGRA.");

		gl.vertexArrayAttribFormat(vao, 0, 1, GL_UNSIGNED_INT_2_10_10_10_REV, GL_TRUE, 0);

		is_ok &=
			CheckError(GL_INVALID_OPERATION, "INVALID_OPERATION was not generated by VertexArrayAttribFormat if type "
											 "is UNSIGNED_INT_2_10_10_10_REV and size is neither 4 nor BGRA.");

		gl.vertexArrayAttribFormat(vao, 0, 1, GL_UNSIGNED_INT_10F_11F_11F_REV, GL_TRUE, 0);

		is_ok &= CheckError(GL_INVALID_OPERATION, "INVALID_OPERATION was not generated by VertexArrayAttribFormat if "
												  "type is UNSIGNED_INT_10F_11F_11F_REV and size is not 3.");

		gl.vertexArrayAttribFormat(vao, 0, GL_BGRA, GL_UNSIGNED_BYTE, GL_FALSE, 0);

		is_ok &= CheckError(
			GL_INVALID_OPERATION,
			"INVALID_OPERATION was not generated by VertexArrayAttribFormat if size is BGRA and normalized is FALSE.");
	}
	catch (...)
	{
		is_ok	= false;
		is_error = true;
	}

	/* Clean up. */
	if (vao)
	{
		gl.deleteVertexArrays(1, &vao);
	}

	/* Errors clean up. */
	while (gl.getError())
		;

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

/** @brief Compare error returned by GL with expected value and log.
 *
 *  @param [in] expected        Expected error.
 *  @param [in] log_message   Message to be logged if expected error is not the equal to the reported one.
 *
 *  @return True if GL error is equal to expected, false otherwise.
 */
bool AttributeFormatErrorsTest::CheckError(const glw::GLenum expected, const glw::GLchar* log_message)
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	glw::GLenum error = 0;

	if (expected != (error = gl.getError()))
	{
		m_context.getTestContext().getLog() << tcu::TestLog::Message << log_message << " " << glu::getErrorStr(error)
											<< " was observed instead." << tcu::TestLog::EndMessage;

		return false;
	}

	return true;
}

/******************************** Attribute Binding Errors Test Implementation   ********************************/

/** @brief Attribute Binding Errors Test constructor.
 *
 *  @param [in] context     OpenGL context.
 */
AttributeBindingErrorsTest::AttributeBindingErrorsTest(deqp::Context& context)
	: deqp::TestCase(context, "vertex_arrays_attribute_binding_errors", "Attribute Binding Errors Test")
{
}

/** @brief Attribute Binding Errors Test cases.
 *
 *  @return Iteration result.
 */
tcu::TestNode::IterateResult AttributeBindingErrorsTest::iterate()
{
	/* Shortcut for GL functionality. */
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

	/* Tested Objects. */
	glw::GLuint vao		  = 0;
	glw::GLuint not_a_vao = 0;

	/* Limits. (Minimum values - OpenGL 4.5 Core Specification, Table 23.55) */
	glw::GLint max_vertex_attribs		  = 16;
	glw::GLint max_vertex_attrib_bindings = 16;

	try
	{
		/* Prepare valid Objects. */
		gl.createVertexArrays(1, &vao);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateVertexArrays call failed.");

		/* Prepare invalid VAO. */
		while (gl.isVertexArray(++not_a_vao))
			;

		/* Prepare limits. */
		gl.getIntegerv(GL_MAX_VERTEX_ATTRIBS, &max_vertex_attribs);
		gl.getIntegerv(GL_MAX_VERTEX_ATTRIB_BINDINGS, &max_vertex_attrib_bindings);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv call failed.");

		/* Not a VAO. */
		gl.vertexArrayAttribBinding(not_a_vao, 0, 0);

		is_ok &= CheckError(GL_INVALID_OPERATION, "INVALID_OPERATION was not generated by VertexArrayAttribBinding if "
												  "vaobj is not the name of an existing vertex array object.");

		/* Too big attribute index. */
		gl.vertexArrayAttribBinding(vao, max_vertex_attribs, 0);

		is_ok &= CheckError(GL_INVALID_VALUE, "INVALID_VALUE was not generated by VertexArrayAttribBinding if "
											  "attribindex is equal to the value of MAX_VERTEX_ATTRIBS.");

		gl.vertexArrayAttribBinding(vao, max_vertex_attribs + 1, 0);

		is_ok &= CheckError(GL_INVALID_VALUE, "INVALID_VALUE was not generated by VertexArrayAttribBinding if "
											  "attribindex is greater than the value of MAX_VERTEX_ATTRIBS.");

		/* Too big binding index. */
		gl.vertexArrayAttribBinding(vao, 0, max_vertex_attrib_bindings);

		is_ok &= CheckError(GL_INVALID_VALUE, "INVALID_VALUE was not generated by VertexArrayAttribBinding if "
											  "bindingindex is equal to the value of MAX_VERTEX_ATTRIB_BINDINGS.");

		gl.vertexArrayAttribBinding(vao, 0, max_vertex_attrib_bindings + 1);

		is_ok &= CheckError(GL_INVALID_VALUE, "INVALID_VALUE was not generated by VertexArrayAttribBinding if "
											  "bindingindex is greater than the value of MAX_VERTEX_ATTRIB_BINDINGS.");
	}
	catch (...)
	{
		is_ok	= false;
		is_error = true;
	}

	/* Clean up. */
	if (vao)
	{
		gl.deleteVertexArrays(1, &vao);
	}

	/* Errors clean up. */
	while (gl.getError())
		;

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

/** @brief Compare error returned by GL with expected value and log.
 *
 *  @param [in] expected        Expected error.
 *  @param [in] log_message   Message to be logged if expected error is not the equal to the reported one.
 *
 *  @return True if GL error is equal to expected, false otherwise.
 */
bool AttributeBindingErrorsTest::CheckError(const glw::GLenum expected, const glw::GLchar* log_message)
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	glw::GLenum error = 0;

	if (expected != (error = gl.getError()))
	{
		m_context.getTestContext().getLog() << tcu::TestLog::Message << log_message << " " << glu::getErrorStr(error)
											<< " was observed instead." << tcu::TestLog::EndMessage;

		return false;
	}

	return true;
}

/******************************** Attribute Binding Divisor Errors Test Implementation   ********************************/

/** @brief Attribute Binding Divisor Errors Test constructor.
 *
 *  @param [in] context     OpenGL context.
 */
AttributeBindingDivisorErrorsTest::AttributeBindingDivisorErrorsTest(deqp::Context& context)
	: deqp::TestCase(context, "vertex_arrays_attribute_binding_divisor_errors", "Attribute Binding Divisor Errors Test")
{
}

/** @brief Attribute Binding Divisor Errors Test cases.
 *
 *  @return Iteration result.
 */
tcu::TestNode::IterateResult AttributeBindingDivisorErrorsTest::iterate()
{
	/* Shortcut for GL functionality. */
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

	/* Tested Objects. */
	glw::GLuint vao		  = 0;
	glw::GLuint not_a_vao = 0;

	/* Limits. (Minimum values - OpenGL 4.5 Core Specification, Table 23.55) */
	glw::GLint max_vertex_attrib_bindings = 16;

	try
	{
		/* Prepare valid Objects. */
		gl.createVertexArrays(1, &vao);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateVertexArrays call failed.");

		/* Prepare invalid VAO. */
		while (gl.isVertexArray(++not_a_vao))
			;

		/* Prepare limits. */
		gl.getIntegerv(GL_MAX_VERTEX_ATTRIB_BINDINGS, &max_vertex_attrib_bindings);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv call failed.");

		/* Not a VAO. */
		gl.vertexArrayBindingDivisor(not_a_vao, 0, 0);

		is_ok &= CheckError(GL_INVALID_OPERATION, "INVALID_OPERATION was not generated by VertexArrayBindingDivisor if "
												  "vaobj is not the name of an existing vertex array object.");

		/* Too big binding index. */
		gl.vertexArrayBindingDivisor(vao, max_vertex_attrib_bindings, 0);

		is_ok &= CheckError(GL_INVALID_VALUE, "INVALID_VALUE was not generated by VertexArrayBindingDivisor if "
											  "bindingindex is equal to the value of MAX_VERTEX_ATTRIB_BINDINGS.");

		gl.vertexArrayBindingDivisor(vao, max_vertex_attrib_bindings + 1, 0);

		is_ok &= CheckError(GL_INVALID_VALUE, "INVALID_VALUE was not generated by VertexArrayBindingDivisor if "
											  "bindingindex is greater than the value of MAX_VERTEX_ATTRIB_BINDINGS.");
	}
	catch (...)
	{
		is_ok	= false;
		is_error = true;
	}

	/* Clean up. */
	if (vao)
	{
		gl.deleteVertexArrays(1, &vao);
	}

	/* Errors clean up. */
	while (gl.getError())
		;

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

/** @brief Compare error returned by GL with expected value and log.
 *
 *  @param [in] expected        Expected error.
 *  @param [in] log_message   Message to be logged if expected error is not the equal to the reported one.
 *
 *  @return True if GL error is equal to expected, false otherwise.
 */
bool AttributeBindingDivisorErrorsTest::CheckError(const glw::GLenum expected, const glw::GLchar* log_message)
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	glw::GLenum error = 0;

	if (expected != (error = gl.getError()))
	{
		m_context.getTestContext().getLog() << tcu::TestLog::Message << log_message << " " << glu::getErrorStr(error)
											<< " was observed instead." << tcu::TestLog::EndMessage;

		return false;
	}

	return true;
}

/******************************** Get Vertex Array Errors Test Implementation   ********************************/

/** @brief Get Vertex Array Errors Test constructor.
 *
 *  @param [in] context     OpenGL context.
 */
GetVertexArrayErrorsTest::GetVertexArrayErrorsTest(deqp::Context& context)
	: deqp::TestCase(context, "vertex_arrays_get_vertex_array_errors", "Get Vertex Array Errors Test")
{
}

/** @brief Iterate over Get Vertex Array Errors Test cases.
 *
 *  @return Iteration result.
 */
tcu::TestNode::IterateResult GetVertexArrayErrorsTest::iterate()
{
	/* Shortcut for GL functionality. */
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

	/* Tested Objects. */
	glw::GLuint vao		  = 0;
	glw::GLuint not_a_vao = 0;

	glw::GLint storage = 0;

	try
	{
		/* Prepare valid Objects. */
		gl.createVertexArrays(1, &vao);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateVertexArrays call failed.");

		/* Prepare invalid VAO. */
		while (gl.isVertexArray(++not_a_vao))
			;

		/* Not a VAO. */
		gl.getVertexArrayiv(not_a_vao, GL_ELEMENT_ARRAY_BUFFER_BINDING, &storage);

		is_ok &= CheckError(GL_INVALID_OPERATION, "INVALID_OPERATION error was not generated by GetVertexArrayiv if "
												  "vaobj is not the name of an existing vertex array object.");

		/* Bad parameter. */
		gl.getVertexArrayiv(vao, GL_ELEMENT_ARRAY_BUFFER_BINDING + 1, &storage);

		is_ok &= CheckError(
			GL_INVALID_ENUM,
			"INVALID_ENUM error was not generated by GetVertexArrayiv if pname is not ELEMENT_ARRAY_BUFFER_BINDING.");
	}
	catch (...)
	{
		is_ok	= false;
		is_error = true;
	}

	/* Clean up. */
	if (vao)
	{
		gl.deleteVertexArrays(1, &vao);
	}

	/* Errors clean up. */
	while (gl.getError())
		;

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

/** @brief Compare error returned by GL with expected value and log.
 *
 *  @param [in] expected        Expected error.
 *  @param [in] log_message   Message to be logged if expected error is not the equal to the reported one.
 *
 *  @return True if GL error is equal to expected, false otherwise.
 */
bool GetVertexArrayErrorsTest::CheckError(const glw::GLenum expected, const glw::GLchar* log_message)
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	glw::GLenum error = 0;

	if (expected != (error = gl.getError()))
	{
		m_context.getTestContext().getLog() << tcu::TestLog::Message << log_message << " " << glu::getErrorStr(error)
											<< " was observed instead." << tcu::TestLog::EndMessage;

		return false;
	}

	return true;
}

/******************************** Get Vertex Array Indexed Errors Test Implementation   ********************************/

/** @brief Get Vertex Array Indexed Errors Test constructor.
 *
 *  @param [in] context     OpenGL context.
 */
GetVertexArrayIndexedErrorsTest::GetVertexArrayIndexedErrorsTest(deqp::Context& context)
	: deqp::TestCase(context, "vertex_arrays_get_vertex_array_indexed_errors", "Get Vertex Array Indexed Errors Test")
{
}

/** @brief Iterate over Get Vertex Array Indexed Errors Test cases.
 *
 *  @return Iteration result.
 */
tcu::TestNode::IterateResult GetVertexArrayIndexedErrorsTest::iterate()
{
	/* Shortcut for GL functionality. */
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

	/* Tested Objects. */
	glw::GLuint vao		  = 0;
	glw::GLuint not_a_vao = 0;

	/* Dummy storage. */
	glw::GLint   storage   = 0;
	glw::GLint64 storage64 = 0;

	/* Bad parameter setup. */
	glw::GLenum bad_pname = 0;

	static const glw::GLenum accepted_pnames[] = { GL_VERTEX_ATTRIB_ARRAY_ENABLED,	GL_VERTEX_ATTRIB_ARRAY_SIZE,
												   GL_VERTEX_ATTRIB_ARRAY_STRIDE,	 GL_VERTEX_ATTRIB_ARRAY_TYPE,
												   GL_VERTEX_ATTRIB_ARRAY_NORMALIZED, GL_VERTEX_ATTRIB_ARRAY_INTEGER,
												   GL_VERTEX_ATTRIB_ARRAY_LONG,		  GL_VERTEX_ATTRIB_ARRAY_DIVISOR,
												   GL_VERTEX_ATTRIB_RELATIVE_OFFSET };

	{
		bool is_accepted_pname = true;
		while (is_accepted_pname)
		{
			bad_pname++;
			is_accepted_pname = false;
			for (glw::GLuint i = 0; i < sizeof(accepted_pnames) / sizeof(accepted_pnames); ++i)
			{
				if (accepted_pnames[i] == bad_pname)
				{
					is_accepted_pname = true;
					break;
				}
			}
		}
	}

	try
	{
		/* Prepare valid Objects. */
		gl.createVertexArrays(1, &vao);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateVertexArrays call failed.");

		/* Prepare invalid VAO. */
		while (gl.isVertexArray(++not_a_vao))
			;

		/* Not a VAO. */
		gl.getVertexArrayIndexediv(not_a_vao, 0, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &storage);

		is_ok &=
			CheckError(GL_INVALID_OPERATION, "INVALID_OPERATION error was not generated by GetVertexArrayIndexediv if "
											 "vaobj is not the name of an existing vertex array object.");

		gl.getVertexArrayIndexed64iv(not_a_vao, 0, GL_VERTEX_BINDING_OFFSET, &storage64);

		is_ok &=
			CheckError(GL_INVALID_OPERATION, "INVALID_OPERATION error was not generated by GetVertexArrayIndexed64iv "
											 "if vaobj is not the name of an existing vertex array object.");

		/* Bad parameter. */
		gl.getVertexArrayIndexediv(vao, 0, bad_pname, &storage);

		is_ok &= CheckError(
			GL_INVALID_ENUM,
			"INVALID_ENUM error was not generated by GetVertexArrayIndexediv if pname is not one of the valid values.");

		/* Bad parameter 64. */
		gl.getVertexArrayIndexed64iv(vao, 0, GL_VERTEX_BINDING_OFFSET + 1, &storage64);

		is_ok &= CheckError(
			GL_INVALID_ENUM,
			"INVALID_ENUM error was not generated by GetVertexArrayIndexed64iv if pname is not VERTEX_BINDING_OFFSET.");
	}
	catch (...)
	{
		is_ok	= false;
		is_error = true;
	}

	/* Clean up. */
	if (vao)
	{
		gl.deleteVertexArrays(1, &vao);
	}

	/* Errors clean up. */
	while (gl.getError())
		;

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

/** @brief Compare error returned by GL with expected value and log.
 *
 *  @param [in] expected        Expected error.
 *  @param [in] log_message   Message to be logged if expected error is not the equal to the reported one.
 *
 *  @return True if GL error is equal to expected, false otherwise.
 */
bool GetVertexArrayIndexedErrorsTest::CheckError(const glw::GLenum expected, const glw::GLchar* log_message)
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	glw::GLenum error = 0;

	if (expected != (error = gl.getError()))
	{
		m_context.getTestContext().getLog() << tcu::TestLog::Message << log_message << " " << glu::getErrorStr(error)
											<< " was observed instead." << tcu::TestLog::EndMessage;

		return false;
	}

	return true;
}
} /* VertexArrays namespace. */
} /* DirectStateAccess namespace. */
} /* gl4cts namespace. */
