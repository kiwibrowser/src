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
namespace TransformFeedback
{
/******************************** Creation Test Implementation   ********************************/

/** @brief Creation Test constructor.
 *
 *  @param [in] context     OpenGL context.
 */
CreationTest::CreationTest(deqp::Context& context)
	: deqp::TestCase(context, "xfb_creation", "Transform Feedback Creation Test")
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

	/* Transform feedback objects */
	static const glw::GLuint xfb_count = 2;

	glw::GLuint xfb_dsa[xfb_count]	= {};
	glw::GLuint xfb_legacy[xfb_count] = {};

	try
	{
		/* Sanity default setup. */
		gl.bindTransformFeedback(GL_TRANSFORM_FEEDBACK, 0);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTransformFeedback have failed");

		/* Check legacy way. */
		gl.genTransformFeedbacks(xfb_count, xfb_legacy);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGenTransformFeedbacks have failed");

		for (glw::GLuint i = 0; i < xfb_count; ++i)
		{
			if (gl.isTransformFeedback(xfb_legacy[i]))
			{
				is_ok = false;

				/* Log. */
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message
					<< "GenTransformFeedbacks has created defualt objects, but only shall reserve names for them."
					<< tcu::TestLog::EndMessage;
			}
		}

		/* Check direct state access way. */
		gl.createTransformFeedbacks(xfb_count, xfb_dsa);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateTransformFeedbacks have failed");

		for (glw::GLuint i = 0; i < xfb_count; ++i)
		{
			if (!gl.isTransformFeedback(xfb_dsa[i]))
			{
				is_ok = false;

				/* Log. */
				m_context.getTestContext().getLog() << tcu::TestLog::Message
													<< "CreateTransformFeedbacks has not created defualt objects."
													<< tcu::TestLog::EndMessage;
			}
		}

		/* Check binding point. */
		glw::GLint xfb_binding_point = -1;

		gl.getIntegerv(GL_TRANSFORM_FEEDBACK_BINDING, &xfb_binding_point);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv have failed");

		if (0 != xfb_binding_point)
		{
			if (-1 == xfb_binding_point)
			{
				m_context.getTestContext().getLog() << tcu::TestLog::Message
													<< "glGetIntegerv used with GL_TRANSFORM_FEEDBACK_BINDING have not "
													   "returned anything and did not generate error."
													<< tcu::TestLog::EndMessage;

				throw 0;
			}
			else
			{
				m_context.getTestContext().getLog() << tcu::TestLog::Message
													<< "The usage of glCreateTransformFeedbacks have changed "
													   "GL_TRANSFORM_FEEDBACK_BINDING binding point."
													<< tcu::TestLog::EndMessage;

				is_ok = false;
			}
		}
	}
	catch (...)
	{
		is_ok	= false;
		is_error = true;
	}

	/* Cleanup. */
	for (glw::GLuint i = 0; i < xfb_count; ++i)
	{
		if (xfb_legacy[i])
		{
			gl.deleteTransformFeedbacks(1, &xfb_legacy[i]);

			xfb_legacy[i] = 0;
		}

		if (xfb_dsa[i])
		{
			gl.deleteTransformFeedbacks(1, &xfb_dsa[i]);

			xfb_dsa[i] = 0;
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
	: deqp::TestCase(context, "xfb_defaults", "Transform Feedback Defaults Test")
	, m_gl_getTransformFeedbackiv(DE_NULL)
	, m_gl_getTransformFeedbacki_v(DE_NULL)
	, m_gl_getTransformFeedbacki64_v(DE_NULL)
	, m_xfb_dsa(0)
	, m_xfb_indexed_binding_points_count(0)
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

		is_ok &= testBuffersBindingPoints();
		is_ok &= testBuffersDimensions();
		is_ok &= testActive();
		is_ok &= testPaused();
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

/** @brief Create XFB and Buffer Objects. Prepare function pointers.
 *
 *  @note The function may throw if unexpected error has occured.
 *
 *  @return True if test succeeded, false otherwise.
 */
void DefaultsTest::prepare()
{
	/* Shortcut for GL functionality */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Fetching function pointers. */
	m_gl_getTransformFeedbackiv	= (GetTransformFeedbackiv_ProcAddress)gl.getTransformFeedbackiv;
	m_gl_getTransformFeedbacki_v   = (GetTransformFeedbacki_v_ProcAddress)gl.getTransformFeedbacki_v;
	m_gl_getTransformFeedbacki64_v = (GetTransformFeedbacki64_v_ProcAddress)gl.getTransformFeedbacki64_v;

	if ((DE_NULL == m_gl_getTransformFeedbackiv) || (DE_NULL == m_gl_getTransformFeedbacki_v) ||
		(DE_NULL == m_gl_getTransformFeedbacki64_v))
	{
		m_context.getTestContext().getLog() << tcu::TestLog::Message << "Function pointers are set to NULL values."
											<< tcu::TestLog::EndMessage;

		throw 0;
	}

	/* XFB object creation */
	gl.createTransformFeedbacks(1, &m_xfb_dsa);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateTransformFeedbacks have failed");

	/* Query limits. */
	gl.getIntegerv(GL_MAX_TRANSFORM_FEEDBACK_SEPARATE_ATTRIBS, &m_xfb_indexed_binding_points_count);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glIntegerv have failed");
}

/** @brief Test default value of GL_TRANSFORM_FEEDBACK_BUFFER_BINDING.
 *
 *  @note The function may throw if unexpected error has occured.
 *
 *  @return True if test succeeded, false otherwise.
 */
bool DefaultsTest::testBuffersBindingPoints()
{
	/* Shortcut for GL functionality */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Check default binding points value. */
	for (glw::GLint i = 0; i < m_xfb_indexed_binding_points_count; ++i)
	{
		glw::GLint buffer_binding = -1;

		m_gl_getTransformFeedbacki_v(m_xfb_dsa, GL_TRANSFORM_FEEDBACK_BUFFER_BINDING, i, &buffer_binding);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGetTransformFeedbacki_v have failed");

		if (-1 == buffer_binding)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message
				<< "glGetTransformFeedbacki_v with parameter GL_TRANSFORM_FEEDBACK_BUFFER_BINDING has not returned "
				   "anything and error has not been generated."
				<< tcu::TestLog::EndMessage;

			return false;
		}
		else
		{
			if (0 != buffer_binding)
			{
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message
					<< "glGetTransformFeedbacki_v with parameter GL_TRANSFORM_FEEDBACK_BUFFER_BINDING has returned "
					<< buffer_binding << ", however 0 is expected." << tcu::TestLog::EndMessage;

				return false;
			}
		}
	}

	return true;
}

/** @brief Test default values of GL_TRANSFORM_FEEDBACK_START and GL_TRANSFORM_FEEDBACK_SIZE.
 *
 *  @note The function may throw if unexpected error has occured.
 *
 *  @return True if test succeeded, false otherwise.
 */
bool DefaultsTest::testBuffersDimensions()
{
	/* Shortcut for GL functionality */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Check default buffers' start value. */
	for (glw::GLint i = 0; i < m_xfb_indexed_binding_points_count; ++i)
	{
		glw::GLint64 buffer_start = -1;

		m_gl_getTransformFeedbacki64_v(m_xfb_dsa, GL_TRANSFORM_FEEDBACK_BUFFER_START, i, &buffer_start);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGetTransformFeedbacki_v have failed");

		if (-1 == buffer_start)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message
				<< "glGetTransformFeedbacki_v with parameter GL_TRANSFORM_FEEDBACK_BUFFER_START has not returned "
				   "anything and error has not been generated."
				<< tcu::TestLog::EndMessage;

			return false;
		}
		else
		{
			if (0 != buffer_start)
			{
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message
					<< "glGetTransformFeedbacki_v with parameter GL_TRANSFORM_FEEDBACK_BUFFER_START has returned "
					<< buffer_start << ", however 0 is expected." << tcu::TestLog::EndMessage;

				return false;
			}
		}
	}

	/** @brief Check default buffers' size value.
	 *
	 *  @note The function may throw if unexpected error has occured.
	 */
	for (glw::GLint i = 0; i < m_xfb_indexed_binding_points_count; ++i)
	{
		glw::GLint64 buffer_size = -1;

		m_gl_getTransformFeedbacki64_v(m_xfb_dsa, GL_TRANSFORM_FEEDBACK_BUFFER_SIZE, i, &buffer_size);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGetTransformFeedbacki_v have failed");

		if (-1 == buffer_size)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message
				<< "glGetTransformFeedbacki_v with parameter GL_TRANSFORM_FEEDBACK_BUFFER_SIZE has not returned "
				   "anything and error has not been generated."
				<< tcu::TestLog::EndMessage;

			return false;
		}
		else
		{
			if (0 != buffer_size)
			{
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message
					<< "glGetTransformFeedbacki_v with parameter GL_TRANSFORM_FEEDBACK_BUFFER_SIZE has returned "
					<< buffer_size << ", however 0 is expected." << tcu::TestLog::EndMessage;

				return false;
			}
		}
	}

	return true;
}

/** @brief Test default value of GL_TRANSFORM_FEEDBACK_ACTIVE.
 *
 *  @return True if test succeeded, false otherwise.
 */
bool DefaultsTest::testActive()
{
	/* Check that it is not active. */
	glw::GLint is_active = -1;
	m_gl_getTransformFeedbackiv(m_xfb_dsa, GL_TRANSFORM_FEEDBACK_ACTIVE, &is_active);

	if (-1 == is_active)
	{
		m_context.getTestContext().getLog() << tcu::TestLog::Message
											<< "glGetTransformFeedbackiv with parameter GL_TRANSFORM_FEEDBACK_ACTIVE "
											   "has not returned anything and error has not been generated."
											<< tcu::TestLog::EndMessage;

		return false;
	}
	else
	{
		if (0 != is_active)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message
				<< "glGetTransformFeedbackiv with parameter GL_TRANSFORM_FEEDBACK_ACTIVE has returned " << is_active
				<< ", however FALSE is expected." << tcu::TestLog::EndMessage;

			return false;
		}
	}

	return true;
}

/** @brief Test default value of GL_TRANSFORM_FEEDBACK_PAUSED.
 *
 *  @return True if test succeeded, false otherwise.
 */
bool DefaultsTest::testPaused()
{
	/* Check that it is not paused. */
	glw::GLint is_paused = -1;
	m_gl_getTransformFeedbackiv(m_xfb_dsa, GL_TRANSFORM_FEEDBACK_PAUSED, &is_paused);

	if (-1 == is_paused)
	{
		m_context.getTestContext().getLog() << tcu::TestLog::Message
											<< "glGetTransformFeedbacki_v with parameter GL_TRANSFORM_FEEDBACK_PAUSED "
											   "has not returned anything and error has not been generated."
											<< tcu::TestLog::EndMessage;

		return false;
	}
	else
	{
		if (0 != is_paused)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message
				<< "glGetTransformFeedbackiv with parameter GL_TRANSFORM_FEEDBACK_PAUSED has returned " << is_paused
				<< ", however FALSE is expected." << tcu::TestLog::EndMessage;

			return false;
		}
	}

	return true;
}

/** @brief Release GL objects.
 */
void DefaultsTest::clean()
{
	/* Shortcut for GL functionality */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (m_xfb_dsa)
	{
		gl.deleteTransformFeedbacks(1, &m_xfb_dsa);

		m_xfb_dsa = 0;
	}
}

/******************************** Buffers Test Implementation   ********************************/

/** @brief Buffers Test constructor.
 *
 *  @param [in] context     OpenGL context.
 */
BuffersTest::BuffersTest(deqp::Context& context)
	: deqp::TestCase(context, "xfb_buffers", "Transform Feedback Buffers Test")
	, m_gl_getTransformFeedbacki_v(DE_NULL)
	, m_gl_getTransformFeedbacki64_v(DE_NULL)
	, m_gl_TransformFeedbackBufferBase(DE_NULL)
	, m_gl_TransformFeedbackBufferRange(DE_NULL)
	, m_xfb_dsa(0)
	, m_bo_a(0)
	, m_bo_b(0)
{
	/* Intentionally left blank. */
}

/** @brief Iterate Buffers Test cases.
 *
 *  @return Iteration result.
 */
tcu::TestNode::IterateResult BuffersTest::iterate()
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
		/* Prepare function pointers, transform feedback and buffer objects. */
		prepareObjects();

		/* Setup transform feedback object binding points with buffer objects. */
		is_ok = prepareTestSetup();

		/* Continue only if test setup succeeded */
		if (is_ok)
		{
			is_ok &= testBindingPoint(0, m_bo_a, "glTransformFeedbackBufferBase");
			is_ok &= testBindingPoint(1, m_bo_b, "glTransformFeedbackBufferRange");
			is_ok &= testBindingPoint(2, m_bo_b, "glTransformFeedbackBufferRange");

			is_ok &= testStart(0, 0, "glTransformFeedbackBufferBase");
			is_ok &= testStart(1, 0, "glTransformFeedbackBufferRange");
			is_ok &= testStart(2, s_bo_size / 2, "glTransformFeedbackBufferRange");

			is_ok &= testSize(0, 0, "glTransformFeedbackBufferBase");
			is_ok &= testSize(1, s_bo_size / 2, "glTransformFeedbackBufferRange");
			is_ok &= testSize(2, s_bo_size / 2, "glTransformFeedbackBufferRange");
		}
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

/** @brief Create XFB amd BO objects. Setup function pointers.
 *
 *  @note The function may throw if unexpected error has occured.
 */
void BuffersTest::prepareObjects()
{
	/* Shortcut for GL functionality */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Fetching function pointers. */
	m_gl_getTransformFeedbacki_v	  = (GetTransformFeedbacki_v_ProcAddress)gl.getTransformFeedbacki_v;
	m_gl_getTransformFeedbacki64_v	= (GetTransformFeedbacki64_v_ProcAddress)gl.getTransformFeedbacki64_v;
	m_gl_TransformFeedbackBufferBase  = (TransformFeedbackBufferBase_ProcAddress)gl.transformFeedbackBufferBase;
	m_gl_TransformFeedbackBufferRange = (TransformFeedbackBufferRange_ProcAddress)gl.transformFeedbackBufferRange;

	if ((DE_NULL == m_gl_getTransformFeedbacki_v) || (DE_NULL == m_gl_getTransformFeedbacki64_v) ||
		(DE_NULL == m_gl_TransformFeedbackBufferBase) || (DE_NULL == m_gl_TransformFeedbackBufferRange))
	{
		m_context.getTestContext().getLog() << tcu::TestLog::Message << "Function pointers are set to NULL values."
											<< tcu::TestLog::EndMessage;

		throw 0;
	}

	/** @brief XFB object creation */
	gl.createTransformFeedbacks(1, &m_xfb_dsa);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateTransformFeedbacks have failed");

	/* Buffer Objects creation. */
	gl.genBuffers(1, &m_bo_a);
	gl.genBuffers(1, &m_bo_b);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenBuffers have failed");

	if ((0 == m_bo_a) || (0 == m_bo_b))
	{
		m_context.getTestContext().getLog() << tcu::TestLog::Message
											<< "Buffer object has not been generated and no error has been triggered."
											<< tcu::TestLog::EndMessage;

		throw 0;
	}

	/* First buffer memory allocation. */
	gl.bindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, m_bo_a);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffers have failed");

	gl.bufferData(GL_TRANSFORM_FEEDBACK_BUFFER, s_bo_size, NULL, GL_DYNAMIC_COPY);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferData have failed");

	/* Sainty check of buffer size */
	glw::GLint allocated_size = -1;

	gl.getBufferParameteriv(GL_TRANSFORM_FEEDBACK_BUFFER, GL_BUFFER_SIZE, &allocated_size);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetBufferParameteriv have failed");

	if (allocated_size != (glw::GLint)s_bo_size)
	{
		m_context.getTestContext().getLog() << tcu::TestLog::Message << "Buffer allocation failed."
											<< tcu::TestLog::EndMessage;

		throw 0;
	}

	/* Second buffer memory allocation. */
	gl.bindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, m_bo_b);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffers have failed");

	gl.bufferData(GL_TRANSFORM_FEEDBACK_BUFFER, s_bo_size, NULL, GL_DYNAMIC_COPY);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferData have failed");

	/* Sainty check of buffer size */
	allocated_size = -1;

	gl.getBufferParameteriv(GL_TRANSFORM_FEEDBACK_BUFFER, GL_BUFFER_SIZE, &allocated_size);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetBufferParameteriv have failed");

	if (allocated_size != (glw::GLint)s_bo_size)
	{
		m_context.getTestContext().getLog() << tcu::TestLog::Message << "Buffer allocation failed."
											<< tcu::TestLog::EndMessage;

		throw 0;
	}

	gl.bindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffers have failed");
}

/** @brief Setup indexed buffer binding points in the xfb object using
 *         glTransformFeedbackBufferBase and glTransformFeedbackBufferRange
 *         functions.
 *
 *  @return True if setup succeeded, false otherwise (functions triggered errors).
 */
bool BuffersTest::prepareTestSetup()
{
	/* Shortcut for GL functionality */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Bind Buffer Object to first indexed binding point. */
	m_gl_TransformFeedbackBufferBase(m_xfb_dsa, 0, m_bo_a);

	/* Check errors. */
	glw::GLenum error_value = gl.getError();

	if (GL_NONE != error_value)
	{
		m_context.getTestContext().getLog()
			<< tcu::TestLog::Message << "glTransformFeedbackBufferBase has generated unexpected error ("
			<< glu::getErrorStr(error_value) << "). Test Failed." << tcu::TestLog::EndMessage;

		return false;
	}

	/* Bind Buffer Object to second and third indexed binding point. */
	m_gl_TransformFeedbackBufferRange(m_xfb_dsa, 1, m_bo_b, 0, s_bo_size / 2);
	m_gl_TransformFeedbackBufferRange(m_xfb_dsa, 2, m_bo_b, s_bo_size / 2, s_bo_size / 2);

	/* Check errors. */
	error_value = gl.getError();

	if (GL_NONE != error_value)
	{
		m_context.getTestContext().getLog()
			<< tcu::TestLog::Message << "glTransformFeedbackBufferRange has generated unexpected error ("
			<< glu::getErrorStr(error_value) << "). Test Failed." << tcu::TestLog::EndMessage;

		return false;
	}

	return true;
}

/** @brief Test that xfb object's binding point #<index> has <expected_value>.
 *
 *  @param [in] index                   Tested index point.
 *  @param [in] expected_value          Value to be expected (buffer name).
 *  @param [in] tested_function_name    Name of function which this function is going to test
 *                                      (glTransformFeedbackBufferBase or glTransformFeedbackBufferRange)
 *                                      for logging purposes.
 *
 *  @note The function may throw if unexpected error has occured.
 *
 *  @return True if test succeeded, false otherwise.
 */
bool BuffersTest::testBindingPoint(glw::GLuint const index, glw::GLint const expected_value,
								   glw::GLchar const* const tested_function_name)
{
	/* Shortcut for GL functionality */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Check default binding points value. */
	glw::GLint buffer_binding = -1;

	m_gl_getTransformFeedbacki_v(m_xfb_dsa, GL_TRANSFORM_FEEDBACK_BUFFER_BINDING, index, &buffer_binding);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetTransformFeedbacki_v have failed");

	if (-1 == buffer_binding)
	{
		m_context.getTestContext().getLog()
			<< tcu::TestLog::Message << "glGetTransformFeedbacki_v with parameter GL_TRANSFORM_FEEDBACK_BUFFER_BINDING "
										"has not returned anything and error has not been generated."
			<< tcu::TestLog::EndMessage;

		return false;
	}
	else
	{
		if (expected_value != buffer_binding)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message
				<< "glGetTransformFeedbacki_v with parameter GL_TRANSFORM_FEEDBACK_BUFFER_BINDING has returned "
				<< buffer_binding << ", however " << expected_value << " is expected. As a consequence function "
				<< tested_function_name << " have failed to setup proper value." << tcu::TestLog::EndMessage;

			return false;
		}
	}

	return true;
}

/** @brief Test that buffer object at xfb object's binding point #<index> has starting offset set to the <expected_value>.
 *
 *  @param [in] index                   Tested index point.
 *  @param [in] expected_value          Value to be expected (starting offset).
 *  @param [in] tested_function_name    Name of function which this function is going to test
 *                                      (glTransformFeedbackBufferBase or glTransformFeedbackBufferRange)
 *                                      for logging purposes.
 *
 *  @note The function may throw if unexpected error has occured.
 *
 *  @return True if test succeeded, false otherwise.
 */
bool BuffersTest::testStart(glw::GLuint const index, glw::GLint const expected_value,
							glw::GLchar const* const tested_function_name)
{
	/* Shortcut for GL functionality */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Check default buffers' start value. */
	glw::GLint64 buffer_start = -1;

	m_gl_getTransformFeedbacki64_v(m_xfb_dsa, GL_TRANSFORM_FEEDBACK_BUFFER_START, index, &buffer_start);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetTransformFeedbacki_v have failed");

	/* Checking results and errors. */
	if (-1 == buffer_start)
	{
		m_context.getTestContext().getLog()
			<< tcu::TestLog::Message << "glGetTransformFeedbacki_v with parameter GL_TRANSFORM_FEEDBACK_BUFFER_START "
										"has not returned anything and error has not been generated."
			<< tcu::TestLog::EndMessage;

		return false;
	}
	else
	{
		if (expected_value != buffer_start)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message
				<< "glGetTransformFeedbacki_v with parameter GL_TRANSFORM_FEEDBACK_BUFFER_START has returned "
				<< buffer_start << ", however " << expected_value << " is expected. As a consequence function "
				<< tested_function_name << " have failed to setup proper value." << tcu::TestLog::EndMessage;

			return false;
		}
	}

	return true;
}

/** @brief Test that buffer object at xfb object's binding point #<index> has size set to the <expected_value>.
 *
 *  @param [in] index                   Tested index point.
 *  @param [in] expected_value          Value to be expected (buffer's size).
 *  @param [in] tested_function_name    Name of function which this function is going to test
 *                                      (glTransformFeedbackBufferBase or glTransformFeedbackBufferRange)
 *                                      for logging purposes.
 *
 *  @note The function may throw if unexpected error has occured.
 *
 *  @return True if test succeeded, false otherwise.
 */
bool BuffersTest::testSize(glw::GLuint const index, glw::GLint const expected_value,
						   glw::GLchar const* const tested_function_name)
{
	/* Shortcut for GL functionality */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Check default buffer's size value. */
	glw::GLint64 buffer_size = -1;

	m_gl_getTransformFeedbacki64_v(m_xfb_dsa, GL_TRANSFORM_FEEDBACK_BUFFER_SIZE, index, &buffer_size);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetTransformFeedbacki_v have failed");

	/* Checking results and errors. */
	if (-1 == buffer_size)
	{
		m_context.getTestContext().getLog()
			<< tcu::TestLog::Message << "glGetTransformFeedbacki_v with parameter GL_TRANSFORM_FEEDBACK_BUFFER_SIZE "
										"has not returned anything and error has not been generated."
			<< tcu::TestLog::EndMessage;

		return false;
	}
	else
	{
		if (expected_value != buffer_size)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message
				<< "glGetTransformFeedbacki_v with parameter GL_TRANSFORM_FEEDBACK_BUFFER_SIZE has returned "
				<< buffer_size << ", however " << expected_value << " is expected. As a consequence function "
				<< tested_function_name << " have failed to setup proper value." << tcu::TestLog::EndMessage;

			return false;
		}
	}

	return true;
}

/** @brief Clean al GL objects
 */
void BuffersTest::clean()
{
	/* Shortcut for GL functionality */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Release transform feedback object. */
	if (m_xfb_dsa)
	{
		gl.deleteTransformFeedbacks(1, &m_xfb_dsa);

		m_xfb_dsa = 0;
	}

	/* Release buffer objects. */
	if (m_bo_a)
	{
		gl.deleteBuffers(1, &m_bo_a);

		m_bo_a = 0;
	}

	if (m_bo_b)
	{
		gl.deleteBuffers(1, &m_bo_b);

		m_bo_b = 0;
	}
}

/** @brief Buffer Object Size */
const glw::GLuint BuffersTest::s_bo_size = 512;

/******************************** Errors Test Implementation   ********************************/

/** @brief Errors Test constructor.
 *
 *  @param [in] context     OpenGL context.
 */
ErrorsTest::ErrorsTest(deqp::Context& context)
	: deqp::TestCase(context, "xfb_errors", "Transform Feedback Errors Test")
	, m_gl_getTransformFeedbackiv(DE_NULL)
	, m_gl_getTransformFeedbacki_v(DE_NULL)
	, m_gl_getTransformFeedbacki64_v(DE_NULL)
{
	/* Intentionally left blank. */
}

/** @brief Iterate Errors Test cases.
 *
 *  @return Iteration result.
 */
tcu::TestNode::IterateResult ErrorsTest::iterate()
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
		prepareFunctionPointers();

		is_ok &= testCreateTransformFeedbacksForInvalidNumberOfObjects();
		cleanErrors();

		is_ok &= testQueriesForInvalidNameOfObject();
		cleanErrors();

		is_ok &= testGetTransformFeedbackivQueryForInvalidParameterName();
		cleanErrors();

		is_ok &= testGetTransformFeedbacki_vQueryForInvalidParameterName();
		cleanErrors();

		is_ok &= testGetTransformFeedbacki64_vQueryForInvalidParameterName();
		cleanErrors();

		is_ok &= testIndexedQueriesForInvalidBindingPoint();
		cleanErrors();
	}
	catch (...)
	{
		is_ok	= false;
		is_error = true;
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

/** @brief Fetch GL function pointers.
 *
 *  @note The function may throw if unexpected error has occured.
 */
void ErrorsTest::prepareFunctionPointers()
{
	/* Shortcut for GL functionality */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Fetching function pointers. */
	m_gl_getTransformFeedbackiv	= (GetTransformFeedbackiv_ProcAddress)gl.getTransformFeedbackiv;
	m_gl_getTransformFeedbacki_v   = (GetTransformFeedbacki_v_ProcAddress)gl.getTransformFeedbacki_v;
	m_gl_getTransformFeedbacki64_v = (GetTransformFeedbacki64_v_ProcAddress)gl.getTransformFeedbacki64_v;

	if ((DE_NULL == m_gl_getTransformFeedbackiv) || (DE_NULL == m_gl_getTransformFeedbacki_v) ||
		(DE_NULL == m_gl_getTransformFeedbacki64_v))
	{
		m_context.getTestContext().getLog() << tcu::TestLog::Message << "Function pointers are set to NULL values."
											<< tcu::TestLog::EndMessage;

		throw 0;
	}
}

/** @brief Sanity clean-up of GL errors.
 *
 *  @note This function is to only make sure that failing test will not affect other tests.
 */
void ErrorsTest::cleanErrors()
{
	/* Shortcut for GL functionality */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Cleaning errors. */
	while (GL_NO_ERROR != gl.getError())
		;
}

/** @brief Test Creation of Transform Feedbacks using Invalid Number Of Objects
 *
 *  @note Test checks that CreateTransformFeedbacks generates INVALID_VALUE error if
 *        number of transform feedback objects to create is negative.
 *
 *  @return true if test succeded, false otherwise.
 */
bool ErrorsTest::testCreateTransformFeedbacksForInvalidNumberOfObjects()
{
	/* Shortcut for GL functionality */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	glw::GLuint xfbs = 314159;

	gl.createTransformFeedbacks(-1 /* invalid count */, &xfbs);

	glw::GLenum error = gl.getError();

	if (GL_INVALID_VALUE != error)
	{
		m_context.getTestContext().getLog() << tcu::TestLog::Message
											<< "glCreateTransformFeedbacks called with negative number of objects had "
											   "been expected to generate GL_INVALID_VALUE. However, "
											<< glu::getErrorStr(error) << " was captured." << tcu::TestLog::EndMessage;

		return false;
	}

	if (314159 != xfbs)
	{
		m_context.getTestContext().getLog() << tcu::TestLog::Message
											<< "glCreateTransformFeedbacks called with negative number of objects had "
											   "been expected not to change the given buffer."
											<< tcu::TestLog::EndMessage;

		return false;
	}

	return true;
}

/** @brief Test Direct State Access queries with invalid object name
 *
 *  @note Test checks that GetTransformFeedbackiv, GetTransformFeedbacki_v and
 *        GetTransformFeedbacki64_v generate INVALID_OPERATION error if xfb is not
 *        zero or the name of an existing transform feedback object.
 *
 *  @return true if test succeded, false otherwise.
 */
bool ErrorsTest::testQueriesForInvalidNameOfObject()
{
	/* Shortcut for GL functionality */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Generating not a-TransformFeedback name. */
	glw::GLuint invalid_name = 0;

	while (GL_TRUE == gl.isTransformFeedback(++invalid_name))
		;

	/* Dummy storage. */
	glw::GLint   buffer   = 314159;
	glw::GLint64 buffer64 = 314159;

	/* Error variable. */
	glw::GLenum error = 0;

	/* Test of GetTransformFeedbackiv. */
	m_gl_getTransformFeedbackiv(invalid_name, GL_TRANSFORM_FEEDBACK_PAUSED, &buffer);

	if (GL_INVALID_OPERATION != (error = gl.getError()))
	{
		m_context.getTestContext().getLog() << tcu::TestLog::Message
											<< "glGetTransformFeedbackiv called with invalid object name had been "
											   "expected to generate GL_INVALID_OPERATION. However, "
											<< glu::getErrorStr(error) << " was captured." << tcu::TestLog::EndMessage;

		return false;
	}

	if (314159 != buffer)
	{
		m_context.getTestContext().getLog() << tcu::TestLog::Message
											<< "glGetTransformFeedbackiv called with invalid object name had been "
											   "expected not to change the given buffer."
											<< tcu::TestLog::EndMessage;

		return false;
	}

	while (GL_NO_ERROR != (error = gl.getError()))
	{
		m_context.getTestContext().getLog() << tcu::TestLog::Message
											<< "Warning! glGetTransformFeedbackiv called with invalid object name has "
											   "generated more than one error, The next error was  "
											<< glu::getErrorStr(error) << "." << tcu::TestLog::EndMessage;
	}

	/* Test of GetTransformFeedbacki_v. */
	m_gl_getTransformFeedbacki_v(invalid_name, GL_TRANSFORM_FEEDBACK_BUFFER_BINDING, 0, &buffer);

	if (GL_INVALID_OPERATION != (error = gl.getError()))
	{
		m_context.getTestContext().getLog() << tcu::TestLog::Message
											<< "glGetTransformFeedbacki_v called with invalid object name had been "
											   "expected to generate GL_INVALID_OPERATION. However, "
											<< glu::getErrorStr(error) << " was captured." << tcu::TestLog::EndMessage;

		return false;
	}

	if (314159 != buffer)
	{
		m_context.getTestContext().getLog() << tcu::TestLog::Message
											<< "glGetTransformFeedbacki_v called with invalid object name had been "
											   "expected not to change the given buffer."
											<< tcu::TestLog::EndMessage;

		return false;
	}

	while (GL_NO_ERROR != (error = gl.getError()))
	{
		m_context.getTestContext().getLog() << tcu::TestLog::Message
											<< "Warning! glGetTransformFeedbacki_v called with invalid object name has "
											   "unexpectedly generated more than one error, The next error was  "
											<< glu::getErrorStr(error) << "." << tcu::TestLog::EndMessage;
	}

	/* Test of GetTransformFeedbacki64_v. */
	m_gl_getTransformFeedbacki64_v(invalid_name, GL_TRANSFORM_FEEDBACK_BUFFER_START, 0, &buffer64);

	if (GL_INVALID_OPERATION != (error = gl.getError()))
	{
		m_context.getTestContext().getLog() << tcu::TestLog::Message
											<< "glGetTransformFeedbacki64_v called with invalid object name had been "
											   "expected to generate GL_INVALID_OPERATION. However, "
											<< glu::getErrorStr(error) << " was captured." << tcu::TestLog::EndMessage;

		return false;
	}

	if (314159 != buffer64)
	{
		m_context.getTestContext().getLog() << tcu::TestLog::Message
											<< "glGetTransformFeedbacki64_v called with invalid object name had been "
											   "expected not to change the given buffer."
											<< tcu::TestLog::EndMessage;

		return false;
	}

	while (GL_NO_ERROR != (error = gl.getError()))
	{
		m_context.getTestContext().getLog() << tcu::TestLog::Message
											<< "Warning! glGetTransformFeedbacki64_v called with invalid object name "
											   "has unexpectedly generated more than one error, The next error was  "
											<< glu::getErrorStr(error) << "." << tcu::TestLog::EndMessage;
	}

	return true;
}

/** @brief Test Direct State Access queries with invalid parameter name
 *
 *  @note Test checks that GetTransformFeedbackiv generates INVALID_ENUM error if pname
 *        is not TRANSFORM_FEEDBACK_PAUSED or TRANSFORM_FEEDBACK_ACTIVE.
 *
 *  @note The function may throw if unexpected error has occured.
 *
 *  @return true if test succeded, false otherwise.
 */
bool ErrorsTest::testGetTransformFeedbackivQueryForInvalidParameterName()
{
	/* Shortcut for GL functionality */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Creating XFB object. */
	glw::GLuint xfb = 0;

	gl.createTransformFeedbacks(1, &xfb);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateTransformFeedbacks have failed");

	/* Generating invalid parameter name. */
	glw::GLuint invalid_parameter_name = 0;

	/* Dummy storage. */
	glw::GLint buffer = 314159;

	/* Error variable. */
	glw::GLenum error = 0;

	/* Default result. */
	bool is_ok = true;

	/* Test of GetTransformFeedbackiv. */
	m_gl_getTransformFeedbackiv(xfb, invalid_parameter_name, &buffer);

	if (GL_INVALID_ENUM != (error = gl.getError()))
	{
		m_context.getTestContext().getLog() << tcu::TestLog::Message
											<< "glGetTransformFeedbackiv called with invalid parameter name had been "
											   "expected to generate GL_INVALID_ENUM. However, "
											<< glu::getErrorStr(error) << " was captured." << tcu::TestLog::EndMessage;

		is_ok = false;
	}

	if (314159 != buffer)
	{
		m_context.getTestContext().getLog() << tcu::TestLog::Message
											<< "glGetTransformFeedbackiv called with invalid parameter name had been "
											   "expected not to change the given buffer."
											<< tcu::TestLog::EndMessage;

		is_ok = false;
	}

	while (GL_NO_ERROR != (error = gl.getError()))
	{
		m_context.getTestContext().getLog() << tcu::TestLog::Message
											<< "Warning! glGetTransformFeedbackiv called with invalid parameter name "
											   "has generated more than one error, The next error was  "
											<< glu::getErrorStr(error) << "." << tcu::TestLog::EndMessage;
	}

	/* Clean-up. */
	gl.deleteTransformFeedbacks(1, &xfb);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glDeleteTransformFeedbacks have failed");

	return is_ok;
}

/** @brief Test Direct State Access indexed integer query with invalid parameter name
 *
 *  @note Test checks that GetTransformFeedbacki_v generates INVALID_ENUM error if pname
 *        is not TRANSFORM_FEEDBACK_BUFFER_BINDING.
 *
 *  @note The function may throw if unexpected error has occured.
 *
 *  @return true if test succeded, false otherwise.
 */
bool ErrorsTest::testGetTransformFeedbacki_vQueryForInvalidParameterName()
{
	/* Shortcut for GL functionality */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Creating XFB object. */
	glw::GLuint xfb = 0;

	gl.createTransformFeedbacks(1, &xfb);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateTransformFeedbacks have failed");

	/* Generating invalid parameter name. */
	glw::GLuint invalid_parameter_name = 0;

	/* Dummy storage. */
	glw::GLint buffer = 314159;

	/* Error variable. */
	glw::GLenum error = 0;

	/* Default result. */
	bool is_ok = true;

	/* Test of GetTransformFeedbackiv. */
	m_gl_getTransformFeedbacki_v(xfb, invalid_parameter_name, 0, &buffer);

	if (GL_INVALID_ENUM != (error = gl.getError()))
	{
		m_context.getTestContext().getLog() << tcu::TestLog::Message
											<< "glGetTransformFeedbacki_v called with invalid parameter name had been "
											   "expected to generate GL_INVALID_ENUM. However, "
											<< glu::getErrorStr(error) << " was captured." << tcu::TestLog::EndMessage;

		is_ok = false;
	}

	if (314159 != buffer)
	{
		m_context.getTestContext().getLog() << tcu::TestLog::Message
											<< "glGetTransformFeedbacki_v called with invalid parameter name had been "
											   "expected not to change the given buffer."
											<< tcu::TestLog::EndMessage;

		is_ok = false;
	}

	while (GL_NO_ERROR != (error = gl.getError()))
	{
		m_context.getTestContext().getLog() << tcu::TestLog::Message
											<< "Warning! glGetTransformFeedbacki_v called with invalid parameter name "
											   "has generated more than one error, The next error was  "
											<< glu::getErrorStr(error) << "." << tcu::TestLog::EndMessage;
	}

	/* Clean-up. */
	gl.deleteTransformFeedbacks(1, &xfb);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glDeleteTransformFeedbacks have failed");

	return is_ok;
}

/** @brief Test Direct State Access indexed 64 bit integer query with invalid parameter name
 *
 *  @note Test checks that GetTransformFeedbacki64_v generates INVALID_ENUM error if
 *        pname is not TRANSFORM_FEEDBACK_BUFFER_START or
 *        TRANSFORM_FEEDBACK_BUFFER_SIZE.
 *
 *  @note The function may throw if unexpected error has occured.
 *
 *  @return true if test succeded, false otherwise.
 */
bool ErrorsTest::testGetTransformFeedbacki64_vQueryForInvalidParameterName()
{
	/* Shortcut for GL functionality */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Creating XFB object. */
	glw::GLuint xfb = 0;

	gl.createTransformFeedbacks(1, &xfb);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateTransformFeedbacks have failed");

	/* Generating invalid parameter name. */
	glw::GLuint invalid_parameter_name = 0;

	/* Dummy storage. */
	glw::GLint64 buffer = 314159;

	/* Error variable. */
	glw::GLenum error = 0;

	/* Default result. */
	bool is_ok = true;

	/* Test of GetTransformFeedbackiv. */
	m_gl_getTransformFeedbacki64_v(xfb, invalid_parameter_name, 0, &buffer);

	if (GL_INVALID_ENUM != (error = gl.getError()))
	{
		m_context.getTestContext().getLog() << tcu::TestLog::Message
											<< "glGetTransformFeedbacki64_v called with invalid parameter name had "
											   "been expected to generate GL_INVALID_ENUM. However, "
											<< glu::getErrorStr(error) << " was captured." << tcu::TestLog::EndMessage;

		is_ok = false;
	}

	if (314159 != buffer)
	{
		m_context.getTestContext().getLog() << tcu::TestLog::Message
											<< "glGetTransformFeedbacki64_v called with invalid parameter name had "
											   "been expected not to change the given buffer."
											<< tcu::TestLog::EndMessage;

		is_ok = false;
	}

	while (GL_NO_ERROR != (error = gl.getError()))
	{
		m_context.getTestContext().getLog() << tcu::TestLog::Message
											<< "Warning! glGetTransformFeedbacki64_v called with invalid parameter "
											   "name has generated more than one error, The next error was  "
											<< glu::getErrorStr(error) << "." << tcu::TestLog::EndMessage;
	}

	/* Clean-up. */
	gl.deleteTransformFeedbacks(1, &xfb);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glDeleteTransformFeedbacks have failed");

	return is_ok;
}

/** @brief Test Direct State Access indexed queries with invalid index
 *
 *  @note Test checks that GetTransformFeedbacki_v and GetTransformFeedbacki64_v
 *        generate INVALID_VALUE error by GetTransformFeedbacki_v and
 *        GetTransformFeedbacki64_v if index is greater than or equal to the
 *        number of binding points for transform feedback (the value of
 *        MAX_TRANSFORM_FEEDBACK_BUFFERS).
 *
 *  @note The function may throw if unexpected error has occured.
 *
 *  @return true if test succeded, false otherwise.
 */
bool ErrorsTest::testIndexedQueriesForInvalidBindingPoint()
{
	/* Shortcut for GL functionality */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Generating invalid index. */
	glw::GLint max_transform_feedback_buffers =
		4; /* Default limit is 4 - OpenGL 4.5 Core Specification, Table 23.72: Implementation Dependent Transform Feedback Limits. */

	gl.getIntegerv(GL_MAX_TRANSFORM_FEEDBACK_BUFFERS, &max_transform_feedback_buffers);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv have failed");

	/* Creating XFB object. */
	glw::GLuint xfb = 0;

	gl.createTransformFeedbacks(1, &xfb);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateTransformFeedbacks have failed");

	/* Dummy storage. */
	glw::GLint   buffer   = 314159;
	glw::GLint64 buffer64 = 314159;

	/* Error variable. */
	glw::GLenum error = 0;

	/* Default result. */
	bool is_ok = true;

	/* Test of GetTransformFeedbacki_v. */
	m_gl_getTransformFeedbacki_v(xfb, GL_TRANSFORM_FEEDBACK_BUFFER_BINDING, max_transform_feedback_buffers, &buffer);

	if (GL_INVALID_VALUE != (error = gl.getError()))
	{
		m_context.getTestContext().getLog() << tcu::TestLog::Message
											<< "glGetTransformFeedbacki_v called with invalid index had been expected "
											   "to generate GL_INVALID_VALUE. However, "
											<< glu::getErrorStr(error) << " was captured." << tcu::TestLog::EndMessage;

		is_ok = false;
	}

	if (314159 != buffer)
	{
		m_context.getTestContext().getLog()
			<< tcu::TestLog::Message
			<< "glGetTransformFeedbacki_v called with invalid index had been expected not to change the given buffer."
			<< tcu::TestLog::EndMessage;

		is_ok = false;
	}

	while (GL_NO_ERROR != (error = gl.getError()))
	{
		m_context.getTestContext().getLog() << tcu::TestLog::Message
											<< "Warning! glGetTransformFeedbacki_v called with invalid index has "
											   "unexpectedly generated more than one error, The next error was  "
											<< glu::getErrorStr(error) << "." << tcu::TestLog::EndMessage;
	}

	/* Test of GetTransformFeedbacki64_v. */
	m_gl_getTransformFeedbacki64_v(xfb, GL_TRANSFORM_FEEDBACK_BUFFER_START, max_transform_feedback_buffers, &buffer64);

	if (GL_INVALID_VALUE != (error = gl.getError()))
	{
		m_context.getTestContext().getLog() << tcu::TestLog::Message
											<< "glGetTransformFeedbacki64_v called with invalid index had been "
											   "expected to generate GL_INVALID_VALUE. However, "
											<< glu::getErrorStr(error) << " was captured." << tcu::TestLog::EndMessage;

		is_ok = false;
	}

	if (314159 != buffer64)
	{
		m_context.getTestContext().getLog()
			<< tcu::TestLog::Message
			<< "glGetTransformFeedbacki64_v called with invalid index had been expected not to change the given buffer."
			<< tcu::TestLog::EndMessage;

		is_ok = false;
	}

	while (GL_NO_ERROR != (error = gl.getError()))
	{
		m_context.getTestContext().getLog() << tcu::TestLog::Message
											<< "Warning! glGetTransformFeedbacki64_v called with invalid index has "
											   "unexpectedly generated more than one error, The next error was  "
											<< glu::getErrorStr(error) << "." << tcu::TestLog::EndMessage;
	}

	/* Clean-up. */
	gl.deleteTransformFeedbacks(1, &xfb);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glDeleteTransformFeedbacks have failed");

	return is_ok;
}

/******************************** Functional Test Implementation   ********************************/

/** @brief Functional Test constructor.
 *
 *  @param [in] context     OpenGL context.
 */
FunctionalTest::FunctionalTest(deqp::Context& context)
	: deqp::TestCase(context, "xfb_functional", "Transform Feedback Functional Test")
	, m_gl_getTransformFeedbackiv(DE_NULL)
	, m_gl_TransformFeedbackBufferBase(DE_NULL)
	, m_xfb_dsa(0)
	, m_bo(0)
	, m_po(0)
	, m_vao(0)
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
		prepareFunctionPointers();
		prepareTransformFeedback();
		prepareBuffer();
		prepareProgram();
		prepareVertexArrayObject();

		is_ok &= draw();
		is_ok &= verifyBufferContent();
	}
	catch (...)
	{
		is_ok	= false;
		is_error = true;
	}

	/* Releasing GL objects. */
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

/** @brief Get access pointers to GL functions.
 *
 *  @note The function may throw if unexpected error has occured.
 */
void FunctionalTest::prepareFunctionPointers()
{
	/* Shortcut for GL functionality */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Fetching function pointers. */
	m_gl_getTransformFeedbackiv		 = (GetTransformFeedbackiv_ProcAddress)gl.getTransformFeedbackiv;
	m_gl_TransformFeedbackBufferBase = (TransformFeedbackBufferBase_ProcAddress)gl.transformFeedbackBufferBase;

	if ((DE_NULL == m_gl_getTransformFeedbackiv) || (DE_NULL == m_gl_TransformFeedbackBufferBase))
	{
		m_context.getTestContext().getLog() << tcu::TestLog::Message << "Function pointers are set to NULL values."
											<< tcu::TestLog::EndMessage;

		throw 0;
	}
}

/** @brief Create transform feedback object using direct access function.
 *
 *  @note The function may throw if unexpected error has occured.
 */
void FunctionalTest::prepareTransformFeedback()
{
	/* Shortcut for GL functionality */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* XFB object creation. */
	gl.createTransformFeedbacks(1, &m_xfb_dsa);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateTransformFeedbacks have failed");
}

/** @brief Create buffer object and bind it to transform feedback object using direct access function.
 *
 *  @note The function may throw if unexpected error has occured.
 */
void FunctionalTest::prepareBuffer()
{
	/* Shortcut for GL functionality */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Buffer creation and memory allocation. */
	gl.genBuffers(1, &m_bo);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenBuffers have failed");

	gl.bindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, m_bo);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer have failed");

	gl.bufferData(GL_TRANSFORM_FEEDBACK_BUFFER, s_bo_size, DE_NULL, GL_DYNAMIC_COPY);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferData have failed");

	gl.bindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer have failed");

	/* Bind buffer to xfb object (using direct state access function). */
	m_gl_TransformFeedbackBufferBase(m_xfb_dsa, 0, m_bo);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTransformFeedbackBufferBase have failed");
}

/** @brief Build test's GLSL program.
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

		/* Transform Feedback setup. */
		gl.transformFeedbackVaryings(m_po, 1, &s_xfb_varying, GL_INTERLEAVED_ATTRIBS);
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

/** @brief Create and bind empty vertex array object.
 *
 *  @note The function may throw if unexpected error has occured.
 */
void FunctionalTest::prepareVertexArrayObject()
{
	/* Shortcut for GL functionality */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Creating and binding empty vertex array object. */
	gl.genVertexArrays(1, &m_vao);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenVertexArrays call failed.");

	gl.bindVertexArray(m_vao);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindVertexArray call failed.");
}

/** @brief Draw with XFB.
 *
 *  @note Function follows steps:
 *           Begin transform feedback environment.
 *
 *           Using the program with discarded rasterizer, draw array of 4 indices
 *           using POINTS.
 *
 *           Pause transform feedback environment.
 *
 *           Query parameter TRANSFORM_FEEDBACK_PAUSED using GetTransformFeedbackiv.
 *           Expect value equal to TRUE.
 *
 *           Query parameter TRANSFORM_FEEDBACK_ACTIVE using GetTransformFeedbackiv.
 *           Expect value equal to TRUE.
 *
 *           Resume transform feedback environment.
 *
 *           Query parameter TRANSFORM_FEEDBACK_PAUSED using GetTransformFeedbackiv.
 *           Expect value equal to FALSE.
 *
 *           Query parameter TRANSFORM_FEEDBACK_ACTIVE using GetTransformFeedbackiv.
 *           Expect value equal to TRUE.
 *
 *           End Transform feedback environment.
 *
 *           Query parameter TRANSFORM_FEEDBACK_ACTIVE using GetTransformFeedbackiv.
 *           Expect value equal to FALSE.
 *
 *  @note The function may throw if unexpected error has occured.
 *
 *  @return True if included tests succeded, false otherwise.
 */
bool FunctionalTest::draw()
{
	/* Shortcut for GL functionality */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Default result. */
	bool is_ok = true;

	/* Start transform feedback environment. */
	gl.bindTransformFeedback(GL_TRANSFORM_FEEDBACK, m_xfb_dsa);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTransformFeedback call failed.");

	gl.beginTransformFeedback(GL_POINTS);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBeginTransformFeedback call failed.");

	/* Use only xfb. No rendering. */
	gl.enable(GL_RASTERIZER_DISCARD);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glEnable(GL_RASTERIZER_DISCARD) call failed.");

	/* Draw. */
	gl.drawArrays(GL_POINTS, 0, 4);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawArrays call failed.");

	/* Pause Transform Feedback and tests direct state queries related to paused state. */
	gl.pauseTransformFeedback();
	GLU_EXPECT_NO_ERROR(gl.getError(), "glPauseTransformFeedback call failed.");

	is_ok &= testTransformFeedbackStatus(GL_TRANSFORM_FEEDBACK_PAUSED, GL_TRUE);
	is_ok &= testTransformFeedbackStatus(GL_TRANSFORM_FEEDBACK_ACTIVE, GL_TRUE);

	/* Activate Transform Feedback and tests direct state queries related to paused state. */
	gl.resumeTransformFeedback();
	GLU_EXPECT_NO_ERROR(gl.getError(), "glPauseTransformFeedback call failed.");

	is_ok &= testTransformFeedbackStatus(GL_TRANSFORM_FEEDBACK_PAUSED, GL_FALSE);
	is_ok &= testTransformFeedbackStatus(GL_TRANSFORM_FEEDBACK_ACTIVE, GL_TRUE);

	/* Finish transform feedback. */
	gl.endTransformFeedback();
	GLU_EXPECT_NO_ERROR(gl.getError(), "glEndTransformFeedback call failed.");

	is_ok &= testTransformFeedbackStatus(GL_TRANSFORM_FEEDBACK_ACTIVE, GL_FALSE);

	return is_ok;
}

/** @brief Check that selected Transform Feedback state has an expected value.
 *
 *  @param [in] parameter_name      Name of the parameter to be queried.
 *                                  It must be GL_TRANSFORM_FEEDBACK_PAUSED or
 *                                  GL_TRANSFORM_FEEDBACK_ACTIVE
 *  @param [in] expected_value      The expected value of the query.
 *
 *  @note The function may throw if unexpected error has occured.
 *
 *  @return True if the queried value is equal to expected value, false otherwise.
 */
bool FunctionalTest::testTransformFeedbackStatus(glw::GLenum parameter_name, glw::GLint expected_value)
{
	/* Shortcut for GL functionality */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Dummy storage. */
	glw::GLint value = 314159;

	/* Test of GetTransformFeedbackiv. */
	m_gl_getTransformFeedbackiv(m_xfb_dsa, parameter_name, &value);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetTransformFeedbackiv call failed.");

	if (expected_value != value)
	{
		m_context.getTestContext().getLog()
			<< tcu::TestLog::Message << "It was expected that glGetTransformFeedbackiv query of parameter "
			<< ((parameter_name == GL_TRANSFORM_FEEDBACK_PAUSED) ? "GL_TRANSFORM_FEEDBACK_PAUSED" :
																   "GL_TRANSFORM_FEEDBACK_ACTIVE")
			<< " shall return " << ((expected_value == GL_TRUE) ? "GL_TRUE" : "GL_FALSE") << "however, "
			<< ((value == GL_TRUE) ? "GL_TRUE" : "GL_FALSE") << " was returned." << tcu::TestLog::EndMessage;

		return false;
	}

	return true;
}

/** @brief Check that transform feedback buffer contains
 *         consecutive integer numbers from 0 to 3 (included).
 *
 *  @note The function may throw if unexpected error has occured.
 *
 *  @return True if buffer conatins consecutive integer
 *          numbers from 0 to 3 (included), false otherwise.
 */
bool FunctionalTest::verifyBufferContent()
{
	/* Shortcut for GL functionality */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Default result. */
	bool is_ok = true;

	/* Mapping buffer object to the user-space. */
	gl.bindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, m_bo);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer call failed.");

	glw::GLint* buffer = (glw::GLint*)gl.mapBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, GL_READ_ONLY);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glMapBuffer call failed.");

	for (glw::GLint i = 0; i < 4 /* Number of emitted vertices. */; ++i)
	{
		if (buffer[i] != i)
		{
			is_ok = false;

			break;
		}
	}

	gl.unmapBuffer(GL_TRANSFORM_FEEDBACK_BUFFER);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUnmapBuffer call failed.");

	gl.bindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer call failed.");

	return is_ok;
}

/** Release GL objects, return to the default state
 */
void FunctionalTest::clean()
{
	/* Shortcut for GL functionality */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Release transform feedback object. */
	if (m_xfb_dsa)
	{
		gl.deleteTransformFeedbacks(1, &m_xfb_dsa);

		m_xfb_dsa = 0;
	}

	/* Release buffer object. */
	if (m_bo)
	{
		gl.deleteBuffers(1, &m_bo);

		m_bo = 0;
	}

	/* Release GLSL program. */
	if (m_po)
	{
		gl.useProgram(0);

		gl.deleteProgram(m_po);

		m_po = 0;
	}

	/* Release vertex array object. */
	if (m_vao)
	{
		gl.bindVertexArray(0);

		gl.deleteVertexArrays(1, &m_vao);

		m_vao = 0;
	}

	/* Returning to default rasterizer state. */
	gl.disable(GL_RASTERIZER_DISCARD);
}

const glw::GLuint FunctionalTest::s_bo_size = 4 * sizeof(glw::GLint);

const glw::GLchar FunctionalTest::s_vertex_shader[] = "#version 130\n"
													  "\n"
													  "out int result;\n"
													  "\n"
													  "void main()\n"
													  "{\n"
													  "\n"
													  "    result      = gl_VertexID;\n"
													  "    gl_Position = vec4(1.0);\n"
													  "}\n";

const glw::GLchar FunctionalTest::s_fragment_shader[] = "#version 130\n"
														"\n"
														"out vec4 color;\n"
														"\n"
														"void main()\n"
														"{\n"
														"    color = vec4(0.0, 0.0, 0.0, 1.0);\n"
														"}\n";

const glw::GLchar* const FunctionalTest::s_xfb_varying = "result";

} /* TransformFeedback namespace */
} /* DirectStateAccess namespace */
} /* gl4cts namespace */
