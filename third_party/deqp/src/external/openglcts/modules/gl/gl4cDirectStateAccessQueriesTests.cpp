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
 * \file  gl4cDirectStateAccessQueriesTests.cpp
 * \brief Conformance tests for the Direct State Access feature functionality (Queries access part).
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

/* Define OpenGL enumerations not available in framework. */
#ifndef GL_QUERY_TARGET
#define GL_QUERY_TARGET 0x82EA
#endif

namespace gl4cts
{
namespace DirectStateAccess
{
namespace Queries
{
/******************************** Creation Test Implementation   ********************************/

/** @brief Creation Test constructor.
 *
 *  @param [in] context     OpenGL context.
 */
CreationTest::CreationTest(deqp::Context& context)
	: deqp::TestCase(context, "queries_creation", "Query Objects Creation Test")
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

	/* Query targets */
	static const glw::GLenum targets[] = {
		GL_SAMPLES_PASSED, GL_ANY_SAMPLES_PASSED,   GL_ANY_SAMPLES_PASSED_CONSERVATIVE,		 GL_TIME_ELAPSED,
		GL_TIMESTAMP,	  GL_PRIMITIVES_GENERATED, GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN
	};
	static const glw::GLuint targets_count = sizeof(targets) / sizeof(targets[0]);

	/* Queries objects */
	static const glw::GLuint queries_count = 2;

	glw::GLuint queries_legacy[queries_count]			  = {};
	glw::GLuint queries_dsa[targets_count][queries_count] = {};

	try
	{
		/* Check legacy state creation. */
		gl.genQueries(queries_count, queries_legacy);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGenQueries have failed");

		for (glw::GLuint i = 0; i < queries_count; ++i)
		{
			if (gl.isQuery(queries_legacy[i]))
			{
				is_ok = false;

				/* Log. */
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message
					<< "GenQueries has created default objects, but it should create only a names."
					<< tcu::TestLog::EndMessage;
			}
		}

		/* Check direct state creation. */
		for (glw::GLuint i = 0; i < targets_count; ++i)
		{
			gl.createQueries(targets[i], queries_count, queries_dsa[i]);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateQueries have failed");

			for (glw::GLuint j = 0; j < queries_count; ++j)
			{
				if (!gl.isQuery(queries_dsa[i][j]))
				{
					is_ok = false;

					/* Log. */
					m_context.getTestContext().getLog() << tcu::TestLog::Message
														<< "CreateQueries has not created default objects."
														<< tcu::TestLog::EndMessage;
				}
			}
		}
	}
	catch (...)
	{
		is_ok	= false;
		is_error = true;
	}

	/* Cleanup. */
	for (glw::GLuint j = 0; j < queries_count; ++j)
	{
		if (queries_legacy[j])
		{
			gl.deleteQueries(1, &queries_legacy[j]);

			queries_legacy[j] = 0;
		}

		for (glw::GLuint i = 0; i < targets_count; ++i)
		{
			if (queries_dsa[i][j])
			{
				gl.deleteQueries(1, &queries_dsa[i][j]);

				queries_dsa[i][j] = 0;
			}
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
	: deqp::TestCase(context, "queries_defaults", "Queries Defaults Test"), m_query_dsa(0)
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

	/* Query targets. */
	static const glw::GLenum targets[] = {
		GL_SAMPLES_PASSED, GL_ANY_SAMPLES_PASSED,   GL_ANY_SAMPLES_PASSED_CONSERVATIVE,		 GL_TIME_ELAPSED,
		GL_TIMESTAMP,	  GL_PRIMITIVES_GENERATED, GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN
	};

	static const glw::GLchar* target_names[] = {
		"GL_SAMPLES_PASSED", "GL_ANY_SAMPLES_PASSED",   "GL_ANY_SAMPLES_PASSED_CONSERVATIVE",	  "GL_TIME_ELAPSED",
		"GL_TIMESTAMP",		 "GL_PRIMITIVES_GENERATED", "GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN"
	};

	static const glw::GLuint targets_count = sizeof(targets) / sizeof(targets[0]);

	try
	{
		/* Check direct state creation. */
		for (glw::GLuint i = 0; i < targets_count; ++i)
		{
			prepare(targets[i]);

			is_ok &= testQueryParameter(GL_QUERY_RESULT, GL_FALSE, target_names[i]);
			is_ok &= testQueryParameter(GL_QUERY_RESULT_AVAILABLE, GL_TRUE, target_names[i]);

			clean();
		}
	}
	catch (...)
	{
		is_ok	= false;
		is_error = true;

		clean();
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

/** @brief Create Query Objects.
 *
 *  @note The function may throw if unexpected error has occured.
 *
 *  @return True if test succeeded, false otherwise.
 */
void DefaultsTest::prepare(const glw::GLenum target)
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Query object creation */
	gl.createQueries(target, 1, &m_query_dsa);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateTransformFeedbacks have failed");
}

/** @brief Test Query Integer Parameter.
 *
 *  @note The function may throw if unexpected error has occured.
 *
 *  @param [in] pname           Parameter name to be tested.
 *  @param [in] expected_value  Expected value for comparison.
 *  @param [in] target_name     Target name of the tested query object - for logging purposes.
 *
 *  @return True if test succeeded, false otherwise.
 */
bool DefaultsTest::testQueryParameter(const glw::GLenum pname, const glw::GLuint expected_value,
									  const glw::GLchar* target_name)
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Get data. */
	glw::GLuint value = 0;

	gl.getQueryObjectuiv(m_query_dsa, pname, &value);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetQueryObjectuiv have failed");

	if (expected_value != value)
	{
		m_context.getTestContext().getLog()
			<< tcu::TestLog::Message << "glGetQueryObjectuiv of query object with target " << target_name
			<< " with parameter " << pname << " has returned " << value << ", however " << expected_value
			<< " was expected." << tcu::TestLog::EndMessage;

		return false;
	}

	return true;
}

/** @brief Release GL objects.
 */
void DefaultsTest::clean()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (m_query_dsa)
	{
		gl.deleteQueries(1, &m_query_dsa);

		m_query_dsa = 0;
	}
}

/******************************** Errors Test Implementation   ********************************/

/** @brief Errors Test constructor.
 *
 *  @param [in] context     OpenGL context.
 */
ErrorsTest::ErrorsTest(deqp::Context& context)
	: deqp::TestCase(context, "queries_errors", "Queries Errors Test")
	, m_pGetQueryBufferObjectiv(DE_NULL)
	, m_pGetQueryBufferObjectuiv(DE_NULL)
	, m_pGetQueryBufferObjecti64v(DE_NULL)
	, m_pGetQueryBufferObjectui64v(DE_NULL)
{
	/* Intentionally left blank. */
}

/** @brief Iterate Errors Test cases.
 *
 *  @return Iteration result.
 */
tcu::TestNode::IterateResult ErrorsTest::iterate()
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

	/* Getting function pointers. */
	m_pGetQueryBufferObjectiv	= (PFNGLGETQUERYBUFFEROBJECT)gl.getQueryBufferObjectiv;
	m_pGetQueryBufferObjectuiv   = (PFNGLGETQUERYBUFFEROBJECT)gl.getQueryBufferObjectuiv;
	m_pGetQueryBufferObjecti64v  = (PFNGLGETQUERYBUFFEROBJECT)gl.getQueryBufferObjecti64v;
	m_pGetQueryBufferObjectui64v = (PFNGLGETQUERYBUFFEROBJECT)gl.getQueryBufferObjectui64v;

	/* Running tests. */
	bool is_ok	= true;
	bool is_error = false;

	try
	{
		if ((DE_NULL == m_pGetQueryBufferObjectiv) || (DE_NULL == m_pGetQueryBufferObjectuiv) ||
			(DE_NULL == m_pGetQueryBufferObjecti64v) || (DE_NULL == m_pGetQueryBufferObjectui64v))
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "Test could not get the pointers for glGetQueryBufferObject* functions."
				<< tcu::TestLog::EndMessage;

			throw 0;
		}

		is_ok &= testNegativeNumberOfObjects();
		is_ok &= testInvalidTarget();
		is_ok &= testInvalidQueryName();
		is_ok &= testInvalidBufferName();
		is_ok &= testInvalidParameterName();
		is_ok &= testBufferOverflow();
		is_ok &= testBufferNegativeOffset();
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

/** @brief Check that CreateQueries generates INVALID_VALUE
 *         error if number of query objects to create is
 *         negative.
 *
 *  @return True if test succeded, false otherwise.
 */
bool ErrorsTest::testNegativeNumberOfObjects()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Test for each target. */
	for (glw::GLuint i = 0; i < s_targets_count; ++i)
	{
		glw::GLuint query = 0;

		gl.createQueries(s_targets[i], -1, &query); /* Create negative number of queries. */

		glw::GLenum error = gl.getError();

		if (GL_INVALID_VALUE != error)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "glCreateQueries called with target " << s_target_names[i]
				<< " with negative number of objects to be created (-1) has generated error " << glu::getErrorStr(error)
				<< ", however GL_INVALID_VALUE was expected." << tcu::TestLog::EndMessage;

			if (query)
			{
				gl.deleteQueries(1, &query);

				while (error == gl.getError())
					;

				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message << "glCreateQueries called with target " << s_target_names[i]
					<< " with negative number of objects to be created (-1) has created at least one object."
					<< tcu::TestLog::EndMessage;
			}

			return false;
		}
	}

	return true;
}

/** @brief Check that CreateQueries generates INVALID_ENUM error if target is not
 *         one of accepted values:
 *          -  SAMPLES_PASSED,
 *          -  ANY_SAMPLES_PASSED,
 *          -  ANY_SAMPLES_PASSED_CONSERVATIVE,
 *          -  TIME_ELAPSED,
 *          -  TIMESTAMP,
 *          -  PRIMITIVES_GENERATED or
 *          -  TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN.
 *
 *  @return True if test succeded, false otherwise.
 */
bool ErrorsTest::testInvalidTarget()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Creating invalid target. */
	glw::GLenum invalid_target = 0;

	while (isTarget(++invalid_target))
		;

	/* Test. */
	glw::GLuint query = 0;

	gl.createQueries(invalid_target, 1, &query); /* Create negative number of queries. */

	glw::GLenum error = gl.getError();

	if (GL_INVALID_ENUM != error)
	{
		m_context.getTestContext().getLog() << tcu::TestLog::Message << "glCreateQueries called with invalid target ("
											<< invalid_target << ") has generated error " << glu::getErrorStr(error)
											<< ", however GL_INVALID_ENUM was expected." << tcu::TestLog::EndMessage;

		if (query)
		{
			gl.deleteQueries(1, &query);

			while (error == gl.getError())
				;

			m_context.getTestContext().getLog() << tcu::TestLog::Message
												<< "glCreateQueries called with invalid target (" << invalid_target
												<< ") has created an object." << tcu::TestLog::EndMessage;
		}

		return false;
	}

	return true;
}

/** @brief Check that GetQueryBufferObjectiv, GetQueryBufferObjectuiv,
 *         GetQueryBufferObjecti64v and GetQueryBufferObjectui64v generate
 *         INVALID_OPERATION error if <id> is not the name of a query object, or
 *         if the query object named by <id> is currently active.
 *
 *  @return True if test succeded, false otherwise.
 */
bool ErrorsTest::testInvalidQueryName()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Creating invalid query name. */
	glw::GLuint invalid_query = 0;

	/* Default result. */
	bool is_ok	= true;
	bool is_error = false;

	while (gl.isQuery(++invalid_query))
		;

	/* Test's objects. */
	glw::GLuint buffer = 0;
	glw::GLuint query  = 0;

	try
	{
		/* Creating buffer for the test. */
		gl.genBuffers(1, &buffer);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGenBuffers have failed");

		gl.bindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, buffer);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffers have failed");

		gl.bufferData(GL_TRANSFORM_FEEDBACK_BUFFER, sizeof(glw::GLint64), DE_NULL, GL_DYNAMIC_COPY);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffers have failed");

		/* Test invalid query object name (integer version). */
		m_pGetQueryBufferObjectiv(invalid_query, buffer, GL_QUERY_RESULT, 0);

		glw::GLenum error = gl.getError();

		if (GL_INVALID_OPERATION != error)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message
				<< "glGetQueryBufferObjectiv called with invalid query name has generated error "
				<< glu::getErrorStr(error) << ", however GL_INVALID_OPERATION was expected."
				<< tcu::TestLog::EndMessage;

			is_ok = false;
		}

		/* Test invalid query object name (unsigned integer version). */
		m_pGetQueryBufferObjectuiv(invalid_query, buffer, GL_QUERY_RESULT, 0);

		error = gl.getError();

		if (GL_INVALID_OPERATION != error)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message
				<< "glGetQueryBufferObjectuiv called with invalid query name has generated error "
				<< glu::getErrorStr(error) << ", however GL_INVALID_OPERATION was expected."
				<< tcu::TestLog::EndMessage;

			is_ok = false;
		}

		/* Test invalid query object name (64-bit integer version). */
		m_pGetQueryBufferObjecti64v(invalid_query, buffer, GL_QUERY_RESULT, 0);

		error = gl.getError();

		if (GL_INVALID_OPERATION != error)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message
				<< "glGetQueryBufferObjecti64v called with invalid query name has generated error "
				<< glu::getErrorStr(error) << ", however GL_INVALID_OPERATION was expected."
				<< tcu::TestLog::EndMessage;

			is_ok = false;
		}

		/* Test invalid query object name (64-bit unsigned integer version). */
		m_pGetQueryBufferObjectui64v(invalid_query, buffer, GL_QUERY_RESULT, 0);

		error = gl.getError();

		if (GL_INVALID_OPERATION != error)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message
				<< "glGetQueryBufferObjectui64v called with invalid query name has generated error "
				<< glu::getErrorStr(error) << ", however GL_INVALID_OPERATION was expected."
				<< tcu::TestLog::EndMessage;

			is_ok = false;
		}

		/* Create query object for the test. */
		gl.createQueries(s_targets[0], 1, &query);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateQueries have failed");

		gl.beginQuery(s_targets[0], query);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateQueries have failed");

		/* Test query of active query object name (integer version). */
		m_pGetQueryBufferObjectiv(query, buffer, GL_QUERY_RESULT, 0);

		error = gl.getError();

		if (GL_INVALID_OPERATION != error)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message
				<< "glGetQueryBufferObjectiv called with active query object has generated error "
				<< glu::getErrorStr(error) << ", however GL_INVALID_OPERATION was expected."
				<< tcu::TestLog::EndMessage;

			is_ok = false;
		}

		/* Test query of active query object name (unsigned integer version). */
		m_pGetQueryBufferObjectuiv(query, buffer, GL_QUERY_RESULT, 0);

		error = gl.getError();

		if (GL_INVALID_OPERATION != error)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message
				<< "glGetQueryBufferObjectuiv called with active query object has generated error "
				<< glu::getErrorStr(error) << ", however GL_INVALID_OPERATION was expected."
				<< tcu::TestLog::EndMessage;

			is_ok = false;
		}

		/* Test query of active query object name (64-bit integer version). */
		m_pGetQueryBufferObjecti64v(query, buffer, GL_QUERY_RESULT, 0);

		error = gl.getError();

		if (GL_INVALID_OPERATION != error)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message
				<< "glGetQueryBufferObjecti64v called with active query object has generated error "
				<< glu::getErrorStr(error) << ", however GL_INVALID_OPERATION was expected."
				<< tcu::TestLog::EndMessage;

			is_ok = false;
		}

		/* Test query of active query object name (64-bit unsigned integer version). */
		m_pGetQueryBufferObjectui64v(query, buffer, GL_QUERY_RESULT, 0);

		error = gl.getError();

		if (GL_INVALID_OPERATION != error)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message
				<< "glGetQueryBufferObjectui64v called with active query object has generated error "
				<< glu::getErrorStr(error) << ", however GL_INVALID_OPERATION was expected."
				<< tcu::TestLog::EndMessage;

			is_ok = false;
		}
	}
	catch (...)
	{
		is_error = true;
	}

	/* Releasing objects. */
	if (query)
	{
		gl.endQuery(s_targets[0]);

		gl.deleteQueries(1, &query);
	}

	if (buffer)
	{
		gl.deleteBuffers(1, &buffer);
	}

	/* Error cleanup. */
	while (gl.getError())
		;

	if (is_error)
	{
		throw 0;
	}

	return is_ok;
}

/** @brief Check that GetQueryBufferObjectiv, GetQueryBufferObjectuiv,
 *         GetQueryBufferObjecti64v and GetQueryBufferObjectui64v generate
 *         INVALID_OPERATION error if <buffer> is not the name of an existing
 *         buffer object.
 *
 *  @return True if test succeded, false otherwise.
 */
bool ErrorsTest::testInvalidBufferName()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Default result. */
	bool is_ok	= true;
	bool is_error = false;

	/* Creating invalid buffer name. */
	glw::GLuint invalid_buffer = 0;

	while (gl.isBuffer(++invalid_buffer))
		;

	/* Test's objects. */
	glw::GLuint query = 0;

	try
	{
		/* Create query object for the test. */
		gl.createQueries(s_targets[0], 1, &query);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateQueries have failed");

		/* Test query of invalid buffer name (integer version). */
		m_pGetQueryBufferObjectiv(query, invalid_buffer, GL_QUERY_RESULT_AVAILABLE, 0);

		glw::GLenum error = gl.getError();

		if (GL_INVALID_OPERATION != error)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message
				<< "glGetQueryBufferObjectiv which could generate buffers overflow generated error "
				<< glu::getErrorStr(error) << ", however GL_INVALID_OPERATION was expected."
				<< tcu::TestLog::EndMessage;

			is_ok = false;
		}

		/* Test query of invalid buffer name (unsigned integer version). */
		m_pGetQueryBufferObjectuiv(query, invalid_buffer, GL_QUERY_RESULT_AVAILABLE, 0);

		error = gl.getError();

		if (GL_INVALID_OPERATION != error)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message
				<< "glGetQueryBufferObjectuiv which could generate buffers overflow generated error "
				<< glu::getErrorStr(error) << ", however GL_INVALID_OPERATION was expected."
				<< tcu::TestLog::EndMessage;

			is_ok = false;
		}

		/* Test query of invalid buffer name (64-bit integer version). */
		m_pGetQueryBufferObjecti64v(query, invalid_buffer, GL_QUERY_RESULT_AVAILABLE, 0);

		error = gl.getError();

		if (GL_INVALID_OPERATION != error)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message
				<< "glGetQueryBufferObjecti64v which could generate buffers overflow generated error "
				<< glu::getErrorStr(error) << ", however GL_INVALID_OPERATION was expected."
				<< tcu::TestLog::EndMessage;

			is_ok = false;
		}

		/* Test query of invalid buffer name (64-bit unsigned integer version). */
		m_pGetQueryBufferObjectui64v(query, invalid_buffer, GL_QUERY_RESULT_AVAILABLE, 0);

		error = gl.getError();

		if (GL_INVALID_OPERATION != error)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message
				<< "glGetQueryBufferObjectui64v which could generate buffers overflow generated error "
				<< glu::getErrorStr(error) << ", however GL_INVALID_OPERATION was expected."
				<< tcu::TestLog::EndMessage;

			is_ok = false;
		}
	}
	catch (...)
	{
		is_error = true;
	}

	/* Releasing objects. */
	if (query)
	{
		gl.deleteQueries(1, &query);
	}

	/* Error cleanup. */
	while (gl.getError())
		;

	if (is_error)
	{
		throw 0;
	}

	return is_ok;
}

/** @brief Check that GetQueryBufferObjectiv, GetQueryBufferObjectuiv,
 *         GetQueryBufferObjecti64v and GetQueryBufferObjectui64v generate
 *         INVALID_ENUM error if <pname> is not QUERY_RESULT,
 *         QUERY_RESULT_AVAILABLE, QUERY_RESULT_NO_WAIT or QUERY_TARGET.
 *
 *  @return True if test succeded, false otherwise.
 */
bool ErrorsTest::testInvalidParameterName()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Creating invalid parameter name. */
	glw::GLuint invalid_pname = 0;

	while (isParameterName(++invalid_pname))
		;

	/* Default result. */
	bool is_ok	= true;
	bool is_error = false;

	/* Test's objects. */
	glw::GLuint buffer = 0;
	glw::GLuint query  = 0;

	try
	{
		/* Creating buffer for the test. */
		gl.genBuffers(1, &buffer);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGenBuffers have failed");

		gl.bindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, buffer);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffers have failed");

		gl.bufferData(GL_TRANSFORM_FEEDBACK_BUFFER, sizeof(glw::GLint64), DE_NULL, GL_DYNAMIC_COPY);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffers have failed");

		/* Create query object for the test. */
		gl.createQueries(s_targets[0], 1, &query);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateQueries have failed");

		/* Test query of invalid parameter name (integer version). */
		m_pGetQueryBufferObjectiv(query, buffer, invalid_pname, 0);

		glw::GLenum error = gl.getError();

		if (GL_INVALID_ENUM != error)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message
				<< "glGetQueryBufferObjectiv called with invalid parameter name has generated error "
				<< glu::getErrorStr(error) << ", however GL_INVALID_ENUM was expected." << tcu::TestLog::EndMessage;

			is_ok = false;
		}

		/* Test query of invalid parameter name (unsigned integer version). */
		m_pGetQueryBufferObjectuiv(query, buffer, invalid_pname, 0);

		error = gl.getError();

		if (GL_INVALID_ENUM != error)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message
				<< "glGetQueryBufferObjectuiv called with invalid parameter name has generated error "
				<< glu::getErrorStr(error) << ", however GL_INVALID_ENUM was expected." << tcu::TestLog::EndMessage;

			is_ok = false;
		}

		/* Test query of invalid parameter name (64-bit integer version). */
		m_pGetQueryBufferObjecti64v(query, buffer, invalid_pname, 0);

		error = gl.getError();

		if (GL_INVALID_ENUM != error)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message
				<< "glGetQueryBufferObjecti64v called with invalid parameter name has generated error "
				<< glu::getErrorStr(error) << ", however GL_INVALID_ENUM was expected." << tcu::TestLog::EndMessage;

			is_ok = false;
		}

		/* Test query of invalid parameter name (64-bit unsigned integer version). */
		m_pGetQueryBufferObjectui64v(query, buffer, invalid_pname, 0);

		error = gl.getError();

		if (GL_INVALID_ENUM != error)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message
				<< "glGetQueryBufferObjectui64v called with invalid parameter name has generated error "
				<< glu::getErrorStr(error) << ", however GL_INVALID_ENUM was expected." << tcu::TestLog::EndMessage;

			is_ok = false;
		}
	}
	catch (...)
	{
		is_error = true;
	}

	/* Releasing objects. */
	if (query)
	{
		gl.deleteQueries(1, &query);
	}

	if (buffer)
	{
		gl.deleteBuffers(1, &buffer);
	}

	/* Error cleanup. */
	while (gl.getError())
		;

	if (is_error)
	{
		throw 0;
	}

	return is_ok;
}

/** @brief Check that GetQueryBufferObjectiv, GetQueryBufferObjectuiv,
 *         GetQueryBufferObjecti64v and GetQueryBufferObjectui64v generate
 *         INVALID_OPERATION error if the query writes to a buffer object, and the
 *         specified buffer offset would cause data to be written beyond the bounds
 *         of that buffer object.
 *
 *  @return True if test succeded, false otherwise.
 */
bool ErrorsTest::testBufferOverflow()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Default result. */
	bool is_ok	= true;
	bool is_error = false;

	/* Test's objects. */
	glw::GLuint buffer = 0;
	glw::GLuint query  = 0;

	try
	{
		/* Creating buffer for the test. */
		gl.genBuffers(1, &buffer);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGenBuffers have failed");

		gl.bindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, buffer);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffers have failed");

		gl.bufferData(GL_TRANSFORM_FEEDBACK_BUFFER, sizeof(glw::GLint64), DE_NULL, GL_DYNAMIC_COPY);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffers have failed");

		/* Create query object for the test. */
		gl.createQueries(s_targets[0], 1, &query);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateQueries have failed");

		/* Test query of buffer overflow (integer version). */
		m_pGetQueryBufferObjectiv(query, buffer, GL_QUERY_RESULT_AVAILABLE, sizeof(glw::GLint64));

		glw::GLenum error = gl.getError();

		if (GL_INVALID_OPERATION != error)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message
				<< "glGetQueryBufferObjectiv which could generate buffers overflow generated error "
				<< glu::getErrorStr(error) << ", however GL_INVALID_OPERATION was expected."
				<< tcu::TestLog::EndMessage;

			is_ok = false;
		}

		/* Test query of buffer overflow (unsigned integer version). */
		m_pGetQueryBufferObjectuiv(query, buffer, GL_QUERY_RESULT_AVAILABLE, sizeof(glw::GLint64));

		error = gl.getError();

		if (GL_INVALID_OPERATION != error)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message
				<< "glGetQueryBufferObjectuiv which could generate buffers overflow generated error "
				<< glu::getErrorStr(error) << ", however GL_INVALID_OPERATION was expected."
				<< tcu::TestLog::EndMessage;

			is_ok = false;
		}

		/* Test query of  buffer overflow (64-bit integer version). */
		m_pGetQueryBufferObjecti64v(query, buffer, GL_QUERY_RESULT_AVAILABLE, sizeof(glw::GLint64));

		error = gl.getError();

		if (GL_INVALID_OPERATION != error)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message
				<< "glGetQueryBufferObjecti64v which could generate buffers overflow generated error "
				<< glu::getErrorStr(error) << ", however GL_INVALID_OPERATION was expected."
				<< tcu::TestLog::EndMessage;

			is_ok = false;
		}

		/* Test query of  buffer overflow (64-bit unsigned integer version). */
		m_pGetQueryBufferObjectui64v(query, buffer, GL_QUERY_RESULT_AVAILABLE, sizeof(glw::GLint64));

		error = gl.getError();

		if (GL_INVALID_OPERATION != error)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message
				<< "glGetQueryBufferObjectui64v which could generate buffers overflow generated error "
				<< glu::getErrorStr(error) << ", however GL_INVALID_OPERATION was expected."
				<< tcu::TestLog::EndMessage;

			is_ok = false;
		}
	}
	catch (...)
	{
		is_error = true;
	}

	/* Releasing objects. */
	if (query)
	{
		gl.deleteQueries(1, &query);
	}

	if (buffer)
	{
		gl.deleteBuffers(1, &buffer);
	}

	/* Error cleanup. */
	while (gl.getError())
		;

	if (is_error)
	{
		throw 0;
	}

	return is_ok;
}

/** @brief Check that GetQueryBufferObjectiv, GetQueryBufferObjectuiv,
 *         GetQueryBufferObjecti64v and GetQueryBufferObjectui64v generate
 *         INVALID_VALUE error if <offset> is negative.
 *
 *  @return True if test succeded, false otherwise.
 */
bool ErrorsTest::testBufferNegativeOffset()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Default result. */
	bool is_ok	= true;
	bool is_error = false;

	/* Test's objects. */
	glw::GLuint buffer = 0;
	glw::GLuint query  = 0;

	try
	{
		/* Creating buffer for the test. */
		gl.genBuffers(1, &buffer);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGenBuffers have failed");

		gl.bindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, buffer);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffers have failed");

		gl.bufferData(GL_TRANSFORM_FEEDBACK_BUFFER, sizeof(glw::GLint64), DE_NULL, GL_DYNAMIC_COPY);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffers have failed");

		/* Create query object for the test. */
		gl.createQueries(s_targets[0], 1, &query);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateQueries have failed");

		/* Test query with negative offset (integer version). */
		m_pGetQueryBufferObjectiv(query, buffer, GL_QUERY_RESULT_AVAILABLE, -1);

		glw::GLenum error = gl.getError();

		if (GL_INVALID_VALUE != error)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "glGetQueryBufferObjectiv called with negative offset generated error "
				<< glu::getErrorStr(error) << ", however GL_INVALID_VALUE was expected." << tcu::TestLog::EndMessage;

			is_ok = false;
		}

		/* Test query with negative offset (unsigned integer version). */
		m_pGetQueryBufferObjectuiv(query, buffer, GL_QUERY_RESULT_AVAILABLE, -1);

		error = gl.getError();

		if (GL_INVALID_VALUE != error)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "glGetQueryBufferObjectuiv called with negative offset generated error "
				<< glu::getErrorStr(error) << ", however GL_INVALID_VALUE was expected." << tcu::TestLog::EndMessage;

			is_ok = false;
		}

		/* Test query with negative offset (64-bit integer version). */
		m_pGetQueryBufferObjecti64v(query, buffer, GL_QUERY_RESULT_AVAILABLE, -1);

		error = gl.getError();

		if (GL_INVALID_VALUE != error)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "glGetQueryBufferObjecti64v called with negative offset generated error "
				<< glu::getErrorStr(error) << ", however GL_INVALID_VALUE was expected." << tcu::TestLog::EndMessage;

			is_ok = false;
		}

		/* Test query with negative offset (64-bit unsigned integer version). */
		m_pGetQueryBufferObjectui64v(query, buffer, GL_QUERY_RESULT_AVAILABLE, -1);

		error = gl.getError();

		if (GL_INVALID_VALUE != error)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "glGetQueryBufferObjectui64v called with negative offset generated error "
				<< glu::getErrorStr(error) << ", however GL_INVALID_VALUE was expected." << tcu::TestLog::EndMessage;

			is_ok = false;
		}
	}
	catch (...)
	{
		is_error = true;
	}

	/* Releasing objects. */
	if (query)
	{
		gl.deleteQueries(1, &query);
	}

	if (buffer)
	{
		gl.deleteBuffers(1, &buffer);
	}

	/* Error cleanup. */
	while (gl.getError())
		;

	if (is_error)
	{
		throw 0;
	}

	return is_ok;
}

/** @brief Check if argument is one of the target names:
 *          -  SAMPLES_PASSED,
 *          -  TIME_ELAPSED,
 *          -  PRIMITIVES_GENERATED,
 *          -  TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN.
 *
 *  @param [in] maybe_target   Target to be checked.
 *
 *  @return True if argument is one of the listed targets, false otherwise.
 */
bool ErrorsTest::isTarget(glw::GLenum maybe_target)
{
	for (glw::GLuint i = 0; i < s_targets_count; ++i)
	{
		if (maybe_target == s_targets[i])
		{
			return true;
		}
	}

	return false;
}

/** @brief Check if argument is one of the parameter names:
 *          -  QUERY_RESULT,
 *          -  QUERY_RESULT_AVAILABLE,
 *          -  QUERY_RESULT_NO_WAIT,
 *          -  QUERY_TARGET.
 *
 *  @param [in] maybe_pname   Parameter name to be checked.
 *
 *  @return True if argument is one of the listed parameters, false otherwise.
 */
bool ErrorsTest::isParameterName(glw::GLenum maybe_pname)
{
	glw::GLenum pnames[]	 = { GL_QUERY_RESULT, GL_QUERY_RESULT_AVAILABLE, GL_QUERY_RESULT_NO_WAIT, GL_QUERY_TARGET };
	glw::GLuint pnames_count = sizeof(pnames) / sizeof(pnames[0]);

	for (glw::GLuint i = 0; i < pnames_count; ++i)
	{
		if (maybe_pname == pnames[i])
		{
			return true;
		}
	}

	return false;
}

/** Targets to be tested. */
const glw::GLenum ErrorsTest::s_targets[] = {
	GL_SAMPLES_PASSED, GL_ANY_SAMPLES_PASSED,   GL_ANY_SAMPLES_PASSED_CONSERVATIVE,		 GL_TIME_ELAPSED,
	GL_TIMESTAMP,	  GL_PRIMITIVES_GENERATED, GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN
};

/** Names of targets to be tested. */
const glw::GLchar* ErrorsTest::s_target_names[] = {
	"GL_SAMPLES_PASSED", "GL_ANY_SAMPLES_PASSED",   "GL_ANY_SAMPLES_PASSED_CONSERVATIVE",	  "GL_TIME_ELAPSED",
	"GL_TIMESTAMP",		 "GL_PRIMITIVES_GENERATED", "GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN"
};

/** Number of targets. */
const glw::GLuint ErrorsTest::s_targets_count = sizeof(s_targets) / sizeof(s_targets[0]);

/******************************** Functional Test Implementation   ********************************/

/** @brief Functional Test constructor.
 *
 *  @param [in] context     OpenGL context.
 */
FunctionalTest::FunctionalTest(deqp::Context& context)
	: deqp::TestCase(context, "queries_functional", "Queries Functional Test")
	, m_pGetQueryBufferObjectiv(DE_NULL)
	, m_pGetQueryBufferObjectuiv(DE_NULL)
	, m_pGetQueryBufferObjecti64v(DE_NULL)
	, m_pGetQueryBufferObjectui64v(DE_NULL)
	, m_fbo(0)
	, m_rbo(0)
	, m_vao(0)
	, m_bo_query(0)
	, m_bo_xfb(0)
	, m_qo(DE_NULL)
	, m_po(0)
{
	/* Intentionally left blank. */
}

/** @brief Do comparison (a == b).
 *
 *  @tparam         Type of the values to be compared.
 *
 *  @param [in] a   First  value to be compared.
 *  @param [in] b   Second value to be compared.
 *
 *  @return Result of the comparison.
 */
template <typename T>
bool FunctionalTest::equal(T a, T b)
{
	return (a == b);
}

/** @brief Do comparison (a < b).
 *
 *  @tparam         Type of the values to be compared.
 *
 *  @param [in] a   First  value to be compared.
 *  @param [in] b   Second value to be compared.
 *
 *  @return Result of the comparison.
 */
template <typename T>
bool FunctionalTest::less(T a, T b)
{
	return (a < b);
}

/** @brief Template specialization of glGetQueryBufferObject* function for GLint.
 *         This is pass through function to glGetQueryBufferObjectiv
 *
 *  @param [in] id          Query object identifier.
 *  @param [in] buffer      Buffer object identifier.
 *  @param [in] pname       Parameter name to be queried.
 *  @param [in] offset      Offset of the buffer to be saved at.
 */
template <>
void FunctionalTest::GetQueryBufferObject<glw::GLint>(glw::GLuint id, glw::GLuint buffer, glw::GLenum pname,
													  glw::GLintptr offset)
{
	m_pGetQueryBufferObjectiv(id, buffer, pname, offset);
}

/** @brief Template specialization of glGetQueryBufferObject* function for GLuint.
 *         This is pass through function to glGetQueryBufferObjectuiv
 *
 *  @param [in] id          Query object identifier.
 *  @param [in] buffer      Buffer object identifier.
 *  @param [in] pname       Parameter name to be queried.
 *  @param [in] offset      Offset of the buffer to be saved at.
 */
template <>
void FunctionalTest::GetQueryBufferObject<glw::GLuint>(glw::GLuint id, glw::GLuint buffer, glw::GLenum pname,
													   glw::GLintptr offset)
{
	m_pGetQueryBufferObjectuiv(id, buffer, pname, offset);
}

/** @brief Template specialization of glGetQueryBufferObject* function for GLint64.
 *         This is pass through function to glGetQueryBufferObjecti64v
 *
 *  @param [in] id          Query object identifier.
 *  @param [in] buffer      Buffer object identifier.
 *  @param [in] pname       Parameter name to be queried.
 *  @param [in] offset      Offset of the buffer to be saved at.
 */
template <>
void FunctionalTest::GetQueryBufferObject<glw::GLint64>(glw::GLuint id, glw::GLuint buffer, glw::GLenum pname,
														glw::GLintptr offset)
{
	m_pGetQueryBufferObjecti64v(id, buffer, pname, offset);
}

/** @brief Template specialization of glGetQueryBufferObject* function for GLuint64.
 *         This is pass through function to glGetQueryBufferObjectui64v
 *
 *  @param [in] id          Query object identifier.
 *  @param [in] buffer      Buffer object identifier.
 *  @param [in] pname       Parameter name to be queried.
 *  @param [in] offset      Offset of the buffer to be saved at.
 */
template <>
void FunctionalTest::GetQueryBufferObject<glw::GLuint64>(glw::GLuint id, glw::GLuint buffer, glw::GLenum pname,
														 glw::GLintptr offset)
{
	m_pGetQueryBufferObjectui64v(id, buffer, pname, offset);
}

/** @brief Function template fetches query result to buffer object using
 *         glGetQueryBufferObject* function (selected on the basis of template parameter).
 *         Then buffer is mapped and the result is compared with expected_value using comparison function.
 *
 *  @tparam                     Templated type of fetched data.
 *                              It shall be one of glw::GL[u]int[64].
 *
 *  @param [in] query           Query object to be queried.
 *  @param [in] pname           Parameter name to be queried.
 *  @param [in] expected_value  Reference value to be compared.
 *  @param [in] comparison      Comparison function pointer.
 *                              Comparsion function shall NOT throw.
 *
 *  @note The function may throw if unexpected error has occured.
 *
 *  @return True if comparison succeeded, false otherwise.
 */
template <typename T>
bool FunctionalTest::checkQueryBufferObject(glw::GLuint query, glw::GLenum pname, T expected_value,
											bool (*comparison)(T, T))
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Default result. */
	bool is_ok = true;

	/* Saving results to buffer. */
	GetQueryBufferObject<T>(query, m_bo_query, pname, 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetQueryBufferObject* have failed");

	/* Mapping buffer to user space. */
	T* value = (T*)gl.mapBuffer(GL_QUERY_BUFFER, GL_READ_ONLY);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glMapBuffer have failed");

	/* Doing test. */
	if (!comparison(expected_value, *value))
	{
		is_ok = false;
	}

	/* Cleanup. */
	gl.unmapBuffer(GL_QUERY_BUFFER);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUnmapBuffer have failed");

	/* Return test result. */
	return is_ok;
}

/** @brief Iterate Functional Test cases.
 *
 *  @return Iteration result.
 */
tcu::TestNode::IterateResult FunctionalTest::iterate()
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

	/* Fetching access point to GL functions. */
	m_pGetQueryBufferObjectiv	= (PFNGLGETQUERYBUFFEROBJECT)gl.getQueryBufferObjectiv;
	m_pGetQueryBufferObjectuiv   = (PFNGLGETQUERYBUFFEROBJECT)gl.getQueryBufferObjectuiv;
	m_pGetQueryBufferObjecti64v  = (PFNGLGETQUERYBUFFEROBJECT)gl.getQueryBufferObjecti64v;
	m_pGetQueryBufferObjectui64v = (PFNGLGETQUERYBUFFEROBJECT)gl.getQueryBufferObjectui64v;

	/* Running tests. */
	bool is_ok	= true;
	bool is_error = false;

	try
	{
		if ((DE_NULL == m_pGetQueryBufferObjectiv) || (DE_NULL == m_pGetQueryBufferObjectuiv) ||
			(DE_NULL == m_pGetQueryBufferObjecti64v) || (DE_NULL == m_pGetQueryBufferObjectui64v))
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "Test could not get the pointers for glGetQueryBufferObject* functions."
				<< tcu::TestLog::EndMessage;

			throw 0;
		}

		prepareView();
		prepareVertexArray();
		prepareBuffers();
		prepareQueries();
		prepareProgram();

		draw();

		/* Make sure that framebuffer and transform feedback buffer are filled with expectd data. */
		is_ok &= checkView();
		is_ok &= checkXFB();

		/* Make comparisons for each query object. */
		for (glw::GLuint i = 0; i < s_targets_count; ++i)
		{
			/* Checking targets. */
			is_ok &= checkQueryBufferObject<glw::GLint>(m_qo[i], GL_QUERY_TARGET, s_targets[i],
														&FunctionalTest::equal<glw::GLint>);
			is_ok &= checkQueryBufferObject<glw::GLuint>(m_qo[i], GL_QUERY_TARGET, s_targets[i],
														 &FunctionalTest::equal<glw::GLuint>);
			is_ok &= checkQueryBufferObject<glw::GLint64>(m_qo[i], GL_QUERY_TARGET, s_targets[i],
														  &FunctionalTest::equal<glw::GLint64>);
			is_ok &= checkQueryBufferObject<glw::GLuint64>(m_qo[i], GL_QUERY_TARGET, s_targets[i],
														   &FunctionalTest::equal<glw::GLuint64>);

			/* Checking result availability. */
			is_ok &= checkQueryBufferObject<glw::GLint>(m_qo[i], GL_QUERY_RESULT_AVAILABLE, GL_TRUE,
														&FunctionalTest::equal<glw::GLint>);
			is_ok &= checkQueryBufferObject<glw::GLuint>(m_qo[i], GL_QUERY_RESULT_AVAILABLE, GL_TRUE,
														 &FunctionalTest::equal<glw::GLuint>);
			is_ok &= checkQueryBufferObject<glw::GLint64>(m_qo[i], GL_QUERY_RESULT_AVAILABLE, GL_TRUE,
														  &FunctionalTest::equal<glw::GLint64>);
			is_ok &= checkQueryBufferObject<glw::GLuint64>(m_qo[i], GL_QUERY_RESULT_AVAILABLE, GL_TRUE,
														   &FunctionalTest::equal<glw::GLuint64>);

			if (GL_TIME_ELAPSED == s_targets[i])
			{
				/* Checking result. */
				is_ok &= checkQueryBufferObject<glw::GLint>(m_qo[i], GL_QUERY_RESULT, (glw::GLint)s_results[i],
															&FunctionalTest::less<glw::GLint>);
				is_ok &= checkQueryBufferObject<glw::GLuint>(m_qo[i], GL_QUERY_RESULT, (glw::GLuint)s_results[i],
															 &FunctionalTest::less<glw::GLuint>);
				is_ok &= checkQueryBufferObject<glw::GLint64>(m_qo[i], GL_QUERY_RESULT, (glw::GLint64)s_results[i],
															  &FunctionalTest::less<glw::GLint64>);
				is_ok &= checkQueryBufferObject<glw::GLuint64>(m_qo[i], GL_QUERY_RESULT, (glw::GLuint64)s_results[i],
															   &FunctionalTest::less<glw::GLuint64>);

				/* Checking result (no-wait). */
				is_ok &= checkQueryBufferObject<glw::GLint>(m_qo[i], GL_QUERY_RESULT_NO_WAIT, (glw::GLint)s_results[i],
															&FunctionalTest::less<glw::GLint>);
				is_ok &= checkQueryBufferObject<glw::GLuint>(
					m_qo[i], GL_QUERY_RESULT_NO_WAIT, (glw::GLuint)s_results[i], &FunctionalTest::less<glw::GLuint>);
				is_ok &= checkQueryBufferObject<glw::GLint64>(
					m_qo[i], GL_QUERY_RESULT_NO_WAIT, (glw::GLint64)s_results[i], &FunctionalTest::less<glw::GLint64>);
				is_ok &=
					checkQueryBufferObject<glw::GLuint64>(m_qo[i], GL_QUERY_RESULT_NO_WAIT, (glw::GLuint64)s_results[i],
														  &FunctionalTest::less<glw::GLuint64>);
			}
			else
			{
				/* Checking result. */
				is_ok &= checkQueryBufferObject<glw::GLint>(m_qo[i], GL_QUERY_RESULT, (glw::GLint)s_results[i],
															&FunctionalTest::equal<glw::GLint>);
				is_ok &= checkQueryBufferObject<glw::GLuint>(m_qo[i], GL_QUERY_RESULT, (glw::GLuint)s_results[i],
															 &FunctionalTest::equal<glw::GLuint>);
				is_ok &= checkQueryBufferObject<glw::GLint64>(m_qo[i], GL_QUERY_RESULT, (glw::GLint64)s_results[i],
															  &FunctionalTest::equal<glw::GLint64>);
				is_ok &= checkQueryBufferObject<glw::GLuint64>(m_qo[i], GL_QUERY_RESULT, (glw::GLuint64)s_results[i],
															   &FunctionalTest::equal<glw::GLuint64>);

				/* Checking result (no-wait). */
				is_ok &= checkQueryBufferObject<glw::GLint>(m_qo[i], GL_QUERY_RESULT_NO_WAIT, (glw::GLint)s_results[i],
															&FunctionalTest::equal<glw::GLint>);
				is_ok &= checkQueryBufferObject<glw::GLuint>(
					m_qo[i], GL_QUERY_RESULT_NO_WAIT, (glw::GLuint)s_results[i], &FunctionalTest::equal<glw::GLuint>);
				is_ok &= checkQueryBufferObject<glw::GLint64>(
					m_qo[i], GL_QUERY_RESULT_NO_WAIT, (glw::GLint64)s_results[i], &FunctionalTest::equal<glw::GLint64>);
				is_ok &=
					checkQueryBufferObject<glw::GLuint64>(m_qo[i], GL_QUERY_RESULT_NO_WAIT, (glw::GLuint64)s_results[i],
														  &FunctionalTest::equal<glw::GLuint64>);
			}
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

/** @brief Function prepares framebuffer with RGBA8 color attachment.
 *         Viewport is set up. Content of the framebuffer is cleared.
 *
 *  @note The function may throw if unexpected error has occured.
 */
void FunctionalTest::prepareView()
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

/** @brief Function creates and binds empty vertex array.
 *
 *  @note The function may throw if unexpected error has occured.
 */
void FunctionalTest::prepareVertexArray()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Creating and binding VAO. */
	gl.genVertexArrays(1, &m_vao);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenVertexArrays have failed");

	gl.bindVertexArray(m_vao);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindVertexArray have failed");
}

/** @brief Function creates buffers for query and transform feedback data storage.
 *         The storage is allocated and buffers are bound to QUERY_BUFFER and
 *         TRANSFORM_FEEDBACK_BUFFER binding points respectively.
 *
 *  @note The function may throw if unexpected error has occured.
 */
void FunctionalTest::prepareBuffers()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Buffer for storing query's result. */
	gl.genBuffers(1, &m_bo_query);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenBuffers have failed");

	gl.bindBuffer(GL_QUERY_BUFFER, m_bo_query);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer have failed");

	gl.bufferData(GL_QUERY_BUFFER, sizeof(glw::GLint64), DE_NULL, GL_DYNAMIC_COPY);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferData have failed");

	/* Buffer for storing transform feedback results. */
	gl.genBuffers(1, &m_bo_xfb);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenBuffers have failed");

	gl.bindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, m_bo_xfb);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer have failed");

	gl.bufferData(GL_TRANSFORM_FEEDBACK_BUFFER,
				  3 /* number of vertices per triangle */ * 2 /* number of triangles */ * sizeof(glw::GLint), DE_NULL,
				  GL_DYNAMIC_COPY);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferData have failed");

	gl.bindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, m_bo_xfb);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBufferBase have failed");
}

/** @brief Function creates array of query objects using DSA-style method.
 *
 *  @note The function may throw if unexpected error has occured.
 */
void FunctionalTest::prepareQueries()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Allocating memory for queries array. */
	m_qo = new glw::GLuint[s_targets_count];

	if (DE_NULL == m_qo)
	{
		throw 0;
	}

	/* Creating query object for each target. */
	for (glw::GLuint i = 0; i < s_targets_count; ++i)
	{
		gl.createQueries(s_targets[i], 1, &m_qo[i]);

		/* Error checking. */
		if (GL_NO_ERROR != gl.getError())
		{
			/* Remove previous. */
			if (i > 0)
			{
				gl.deleteQueries(i, m_qo);
			}

			/* Deallocate storage. */
			delete[] m_qo;

			m_qo = DE_NULL;

			/* Signalise test failure. */
			throw 0;
		}
	}
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

		/* Transform Feedback setup. */
		gl.transformFeedbackVaryings(m_po, 1, &s_xfb_varying_name, GL_INTERLEAVED_ATTRIBS);
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

/** @brief Function draws full screen quad. Queries are measured during the process.
 *         Also, transform feedback data is captured.
 *
 *  @note The function may throw if unexpected error has occured.
 */
void FunctionalTest::draw()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Start queries. */
	for (glw::GLuint i = 0; i < s_targets_count; ++i)
	{
		gl.beginQuery(s_targets[i], m_qo[i]);
	}

	/* Start XFB. */
	gl.beginTransformFeedback(GL_TRIANGLES);

	/* Draw full screen quad. */
	gl.drawArrays(GL_TRIANGLE_STRIP, 0, 4);

	/* Finish XFB. */
	gl.endTransformFeedback();

	/* Finish queries. */
	for (glw::GLuint i = 0; i < s_targets_count; ++i)
	{
		gl.endQuery(s_targets[i]);
	}

	/* Make sure OpenGL finished drawing. */
	gl.finish();

	/* Error checking. */
	GLU_EXPECT_NO_ERROR(gl.getError(), "Drawing function have failed.");
}

/** @brief Check that framebuffer is filled with red color.
 */
bool FunctionalTest::checkView()
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

/** @brief Check that transform feedback buffer
 *         contains values representing quad.
 */
bool FunctionalTest::checkXFB()
{
	/* Shortcut for GL functionality */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Default result. */
	bool is_ok = true;

	/* Mapping buffer object to the user-space. */
	glw::GLint* buffer = (glw::GLint*)gl.mapBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, GL_READ_ONLY);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glMapBuffer call failed.");

	if ((0 != buffer[0]) || (1 != buffer[1]) || (2 != buffer[2]) ||

		(2 != buffer[3]) || (1 != buffer[4]) || (3 != buffer[5]))
	{
		is_ok = false;
	}

	gl.unmapBuffer(GL_TRANSFORM_FEEDBACK_BUFFER);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUnmapBuffer call failed.");

	return is_ok;
}

/** @brief Release all created objects.
 */
void FunctionalTest::clean()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Releasing queries. */
	if (DE_NULL != m_qo)
	{
		gl.deleteQueries(s_targets_count, m_qo);

		delete[] m_qo;

		m_qo = DE_NULL;
	}

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

	/* Release buffer object for storing queries' results. */
	if (m_bo_query)
	{
		gl.deleteBuffers(1, &m_bo_query);

		m_bo_query = 0;
	}

	/* Release transform feedback buffer. */
	if (m_bo_xfb)
	{
		gl.deleteBuffers(1, &m_bo_xfb);

		m_bo_xfb = 0;
	}

	/* Release GLSL program. */
	if (m_po)
	{
		gl.useProgram(0);

		gl.deleteProgram(m_po);

		m_po = 0;
	}
}

/** Targets to be tested. */
const glw::GLenum FunctionalTest::s_targets[] = { GL_SAMPLES_PASSED, GL_TIME_ELAPSED, GL_PRIMITIVES_GENERATED,
												  GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN };

/** Expected result for each target. */
const glw::GLint FunctionalTest::s_results[] = { 1, 0, 2, 2 };

/** Number of targets. */
const glw::GLuint FunctionalTest::s_targets_count = sizeof(s_targets) / sizeof(s_targets[0]);

/** Vertex shader source code. */
const glw::GLchar FunctionalTest::s_vertex_shader[] = "#version 450\n"
													  "\n"
													  "out int xfb_result;\n"
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
													  "\n"
													  "    xfb_result = gl_VertexID;\n"
													  "}\n";

/** Fragment shader source program. */
const glw::GLchar FunctionalTest::s_fragment_shader[] = "#version 450\n"
														"\n"
														"out vec4 color;\n"
														"\n"
														"void main()\n"
														"{\n"
														"    color = vec4(1.0, 0.0, 0.0, 1.0);\n"
														"}\n";

/** Name of transform feedback varying in vertex shader. */
const glw::GLchar* FunctionalTest::s_xfb_varying_name = "xfb_result";

} /* Queries namespace. */
} /* DirectStateAccess namespace. */
} /* gl4cts namespace. */
