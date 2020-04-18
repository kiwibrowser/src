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

/**
 * \file  gl3cTransformFeedbackOverflowQueryTests.cpp
 * \brief Implements conformance tests for "Transform Feedback Overflow
 *        Query" functionality.
 */ /*-------------------------------------------------------------------*/

#include "gl3cTransformFeedbackOverflowQueryTests.hpp"

#include "deMath.h"
#include "deSharedPtr.hpp"

#include "gluContextInfo.hpp"
#include "gluDefs.hpp"
#include "gluPixelTransfer.hpp"
#include "gluShaderProgram.hpp"

#include "tcuFuzzyImageCompare.hpp"
#include "tcuImageCompare.hpp"
#include "tcuRenderTarget.hpp"
#include "tcuSurface.hpp"
#include "tcuTestLog.hpp"

#include "glw.h"
#include "glwFunctions.hpp"

namespace gl3cts
{

/*
 Base class of all test cases of the feature. Enforces the requirements below:

 * Check that the extension string is available.
 */
class TransformFeedbackOverflowQueryBaseTest : public deqp::TestCase
{
protected:
	TransformFeedbackOverflowQueryBaseTest(deqp::Context& context, TransformFeedbackOverflowQueryTests::API api,
										   const char* name, const char* description)
		: TestCase(context, name, description), m_api(api), m_max_vertex_streams(0)
	{
	}

	/* Checks whether the feature is supported. */
	bool featureSupported()
	{
		if (m_api == TransformFeedbackOverflowQueryTests::API_GL_ARB_transform_feedback_overflow_query)
		{
			glu::ContextType contextType = m_context.getRenderContext().getType();
			if (m_context.getContextInfo().isExtensionSupported("GL_ARB_transform_feedback_overflow_query") ||
				glu::contextSupports(contextType, glu::ApiType::core(4, 6)))
			{
				return true;
			}
		}
		return false;
	}

	/* Checks whether transform_feedback2 is supported. */
	bool supportsTransformFeedback2()
	{
		return (m_context.getContextInfo().isExtensionSupported("GL_ARB_transform_feedback2") ||
				glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType(4, 0, glu::PROFILE_CORE)));
	}

	/* Checks whether transform_feedback3 is supported. */
	bool supportsTransformFeedback3()
	{
		return (m_context.getContextInfo().isExtensionSupported("GL_ARB_transform_feedback3") ||
				glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType(4, 0, glu::PROFILE_CORE)));
	}

	/* Checks whether gpu_shader5 is supported. */
	bool supportsGpuShader5()
	{
		return (m_context.getContextInfo().isExtensionSupported("GL_ARB_gpu_shader5") ||
				glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType(4, 0, glu::PROFILE_CORE)));
	}

	/* Checks whether conditional_render_inverted is supported. */
	bool supportsConditionalRenderInverted()
	{
		return (m_context.getContextInfo().isExtensionSupported("GL_ARB_conditional_render_inverted") ||
				glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType(4, 5, glu::PROFILE_CORE)));
	}

	/* Checks whether query_buffer_object are supported. */
	bool supportsQueryBufferObject()
	{
		return (m_context.getContextInfo().isExtensionSupported("GL_ARB_query_buffer_object") ||
				glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType(4, 4, glu::PROFILE_CORE)));
	}

	/* Returns the maximum number of vertex streams. */
	GLuint getMaxVertexStreams() const
	{
		return m_max_vertex_streams;
	}

	/* Basic test init, child classes must call it. */
	virtual void init()
	{
		if (!featureSupported())
		{
			throw tcu::NotSupportedError("Required transform_feedback_overflow_query extension is not supported");
		}

		if (supportsTransformFeedback3())
		{
			m_max_vertex_streams = (GLuint)m_context.getContextInfo().getInt(GL_MAX_VERTEX_STREAMS);
		}
	}

protected:
	const TransformFeedbackOverflowQueryTests::API m_api;

private:
	GLuint m_max_vertex_streams;
};

/*
 API Implementation Dependent State Test

 * Check that calling GetQueryiv with target TRANSFORM_FEEDBACK_OVERFLOW
 and pname QUERY_COUNTER_BITS returns a non-negative value without error.

 * If GL 4.0 or ARB_transform_feedback3 is supported, check that calling
 GetQueryiv with target TRANSFORM_FEEDBACK_STREAM_OVERFLOW and pname
 QUERY_COUNTER_BITS returns a non-negative value without error.
 */
class TransformFeedbackOverflowQueryImplDepState : public TransformFeedbackOverflowQueryBaseTest
{
public:
	TransformFeedbackOverflowQueryImplDepState(deqp::Context& context, TransformFeedbackOverflowQueryTests::API api,
											   const char* name)
		: TransformFeedbackOverflowQueryBaseTest(
			  context, api, name,
			  "Tests whether the implementation dependent state defined by the feature matches the requirements.")
	{
	}

	/* Test case iterate function. Contains the actual test case logic. */
	IterateResult iterate()
	{
		const glw::Functions& gl = m_context.getRenderContext().getFunctions();
		GLint				  counterBits;

		gl.getQueryiv(GL_TRANSFORM_FEEDBACK_OVERFLOW, GL_QUERY_COUNTER_BITS, &counterBits);
		if (counterBits < 0)
		{
			TCU_FAIL("Value of QUERY_COUNTER_BITS for query target TRANSFORM_FEEDBACK_OVERFLOW is invalid");
		}

		if (supportsTransformFeedback3())
		{
			gl.getQueryiv(GL_TRANSFORM_FEEDBACK_STREAM_OVERFLOW, GL_QUERY_COUNTER_BITS, &counterBits);
			if (counterBits < 0)
			{
				TCU_FAIL("Value of QUERY_COUNTER_BITS for query target TRANSFORM_FEEDBACK_STREAM_OVERFLOW is invalid");
			}
		}

		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

		return STOP;
	}
};

/*
 Base class for all test cases of the feature that verify newly introduced context state.
 */
class TransformFeedbackOverflowQueryContextStateBase : public TransformFeedbackOverflowQueryBaseTest
{
protected:
	TransformFeedbackOverflowQueryContextStateBase(deqp::Context& context, TransformFeedbackOverflowQueryTests::API api,
												   const char* name, const char* description)
		: TransformFeedbackOverflowQueryBaseTest(context, api, name, description)
	{
	}

	/* Returns whether CURRENT_QUERY state for the specified target and index matches the given value. */
	bool verifyCurrentQueryState(GLenum target, GLuint index, GLuint value)
	{
		const glw::Functions& gl	   = m_context.getRenderContext().getFunctions();
		GLint				  expected = (GLint)value;
		GLint				  actual;

		// Use GetQueryIndexediv by default
		gl.getQueryIndexediv(target, index, GL_CURRENT_QUERY, &actual);
		if (actual != expected)
		{
			return false;
		}

		if (index == 0)
		{
			// If index is zero then GetQueryiv should also return the expected value
			gl.getQueryiv(target, GL_CURRENT_QUERY, &actual);
			if (actual != expected)
			{
				return false;
			}
		}

		return true;
	}
};

/*
 API Default Context State Test

 * Check that calling GetQueryiv with target TRANSFORM_FEEDBACK_OVERFLOW
 and pname CURRENT_QUERY returns zero by default.

 * If GL 4.0 or ARB_transform_feedback3 is supported, check that calling
 GetQueryIndexediv with target TRANSFORM_FEEDBACK_STREAM_OVERFLOW and
 pname CURRENT_QUERY returns zero for any index between zero and MAX_-
 VERTEX_STREAMS.
 */
class TransformFeedbackOverflowQueryDefaultState : public TransformFeedbackOverflowQueryContextStateBase
{
public:
	TransformFeedbackOverflowQueryDefaultState(deqp::Context& context, TransformFeedbackOverflowQueryTests::API api,
											   const char* name)
		: TransformFeedbackOverflowQueryContextStateBase(
			  context, api, name,
			  "Tests whether the new context state defined by the feature has the expected default values.")
	{
	}

	/* Test case iterate function. Contains the actual test case logic. */
	IterateResult iterate()
	{
		if (!verifyCurrentQueryState(GL_TRANSFORM_FEEDBACK_OVERFLOW, 0, 0))
		{
			TCU_FAIL("Default value of CURRENT_QUERY for query target TRANSFORM_FEEDBACK_OVERFLOW is non-zero");
		}

		if (supportsTransformFeedback3())
		{
			for (GLuint i = 0; i < getMaxVertexStreams(); ++i)
			{
				if (!verifyCurrentQueryState(GL_TRANSFORM_FEEDBACK_STREAM_OVERFLOW, 0, 0))
				{
					TCU_FAIL("Default value of CURRENT_QUERY for query target TRANSFORM_FEEDBACK_STREAM_OVERFLOW "
							 "is non-zero");
				}
			}
		}

		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

		return STOP;
	}
};

/*
 API Context State Update Test

 * Check that after a successful call to BeginQuery with target TRANSFORM_-
 FEEDBACK_OVERFLOW_ARB calling GetQueryiv with the same target and with
 pname CURRENT_QUERY returns the name of the query previously passed to
 BeginQuery. Also check that after calling EndQuery with the same target
 GetQueryiv returns zero for the same parameters.

 * If GL 4.0 or ARB_transform_feedback3 is supported, check that after a
 successful call to BeginQueryIndexed with target TRANSFORM_FEEDBACK_-
 STREAM_OVERFLOW_ARB calling GetQueryIndexediv with the same target and
 with pname CURRENT_QUERY returns the name of the query previously passed
 to BeginQueryIndexed if the index parameters match and otherwise it
 returns zero. Also check that after calling EndQueryIndexed with the
 same target and index GetQueryIndexediv returns zero for the same
 parameters for all indices. Indices used should be between zero and
 MAX_VERTEX_STREAMS.
 */
class TransformFeedbackOverflowQueryStateUpdate : public TransformFeedbackOverflowQueryContextStateBase
{
public:
	TransformFeedbackOverflowQueryStateUpdate(deqp::Context& context, TransformFeedbackOverflowQueryTests::API api,
											  const char* name)
		: TransformFeedbackOverflowQueryContextStateBase(
			  context, api, name,
			  "Tests whether the new context state defined by the feature is correctly updated after a successful "
			  "call to {Begin|End}Query[Indexed] if the target of the query is one of the newly introduced ones.")
		, m_overflow_query(0)
		, m_stream_overflow_query(0)
	{
	}

	/* Test case init. */
	virtual void init()
	{
		TransformFeedbackOverflowQueryContextStateBase::init();

		const glw::Functions& gl = m_context.getRenderContext().getFunctions();

		gl.genQueries(1, &m_overflow_query);
		gl.genQueries(1, &m_stream_overflow_query);
	}

	/* Test case deinit */
	virtual void deinit()
	{
		const glw::Functions& gl = m_context.getRenderContext().getFunctions();

		gl.deleteQueries(1, &m_overflow_query);
		gl.deleteQueries(1, &m_stream_overflow_query);

		TransformFeedbackOverflowQueryContextStateBase::deinit();
	}

	/* Test case iterate function. Contains the actual test case logic. */
	IterateResult iterate()
	{
		const glw::Functions& gl = m_context.getRenderContext().getFunctions();

		// Call BeginQuery
		gl.beginQuery(GL_TRANSFORM_FEEDBACK_OVERFLOW, m_overflow_query);

		// Verify that CURRENT_QUERY is set to the name of the query
		if (!verifyCurrentQueryState(GL_TRANSFORM_FEEDBACK_OVERFLOW, 0, m_overflow_query))
		{
			TCU_FAIL("Value of CURRENT_QUERY for query target TRANSFORM_FEEDBACK_OVERFLOW is not updated properly "
					 "after a call to BeginQuery");
		}

		// Call EndQuery
		gl.endQuery(GL_TRANSFORM_FEEDBACK_OVERFLOW);

		// Verify that CURRENT_QUERY is reset to zero
		if (!verifyCurrentQueryState(GL_TRANSFORM_FEEDBACK_OVERFLOW, 0, 0))
		{
			TCU_FAIL("Value of CURRENT_QUERY for query target TRANSFORM_FEEDBACK_OVERFLOW is not reset properly "
					 "after a call to EndQuery");
		}

		if (supportsTransformFeedback3())
		{
			for (GLuint i = 0; i < getMaxVertexStreams(); ++i)
			{
				// Call BeginQueryIndexed with specified index
				gl.beginQueryIndexed(GL_TRANSFORM_FEEDBACK_STREAM_OVERFLOW, i, m_stream_overflow_query);

				// Verify that CURRENT_QUERY is set to the name of the query for the specified index, but remains zero for other indices
				for (GLuint j = 0; j < getMaxVertexStreams(); ++j)
				{
					if (i == j)
					{
						if (!verifyCurrentQueryState(GL_TRANSFORM_FEEDBACK_STREAM_OVERFLOW, j, m_stream_overflow_query))
						{
							TCU_FAIL("Value of CURRENT_QUERY for query target TRANSFORM_FEEDBACK_STREAM_OVERFLOW "
									 "is not updated properly after a call to BeginQueryIndexed");
						}
					}
					else
					{
						if (!verifyCurrentQueryState(GL_TRANSFORM_FEEDBACK_STREAM_OVERFLOW, j, 0))
						{
							TCU_FAIL("Value of CURRENT_QUERY for query target TRANSFORM_FEEDBACK_STREAM_OVERFLOW "
									 "is incorrectly updated for an unrelated vertex stream"
									 "index after a call to BeginQueryIndexed");
						}
					}
				}

				// Call EndQueryIndexed with specified index
				gl.endQueryIndexed(GL_TRANSFORM_FEEDBACK_STREAM_OVERFLOW, i);

				// Verify that CURRENT_QUERY is reset to zero for the specified index and still remains zero for other indices
				for (GLuint j = 0; j < getMaxVertexStreams(); ++j)
				{
					if (i == j)
					{
						if (!verifyCurrentQueryState(GL_TRANSFORM_FEEDBACK_STREAM_OVERFLOW, j, 0))
						{
							TCU_FAIL("Value of CURRENT_QUERY for query target TRANSFORM_FEEDBACK_STREAM_OVERFLOW "
									 "is not reset properly after a call to EndQueryIndexed");
						}
					}
					else
					{
						if (!verifyCurrentQueryState(GL_TRANSFORM_FEEDBACK_STREAM_OVERFLOW, j, 0))
						{
							TCU_FAIL("Value of CURRENT_QUERY for query target TRANSFORM_FEEDBACK_STREAM_OVERFLOW "
									 "is incorrectly updated for an unrelated vertex stream"
									 "index after a call to EndQueryIndexed");
						}
					}
				}
			}
		}

		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

		return STOP;
	}

protected:
	GLuint m_overflow_query;
	GLuint m_stream_overflow_query;
};

/*
 Base class for all test cases of the feature that verify various error scenarios.
 */
class TransformFeedbackOverflowQueryErrorBase : public TransformFeedbackOverflowQueryBaseTest
{
protected:
	TransformFeedbackOverflowQueryErrorBase(deqp::Context& context, TransformFeedbackOverflowQueryTests::API api,
											const char* name, const char* description)
		: TransformFeedbackOverflowQueryBaseTest(context, api, name, description), m_case_name(0)
	{
	}

	/* Starts a new error scenario sub-test with the given name. The name is used in error messages if the sub-test fails. */
	void startTest(const char* caseName)
	{
		m_case_name = caseName;
	}

	/* Verifies whether the actually generated error matches that of the expected one. If not then it triggers the failure
	 of the test case with the sub-case name used as the failure message. */
	void verifyError(GLenum expectedError)
	{
		const glw::Functions& gl = m_context.getRenderContext().getFunctions();

		GLenum actualError = gl.getError();

		if (actualError != expectedError)
		{
			TCU_FAIL(m_case_name);
		}
	}

private:
	const char* m_case_name;
};

/*
 API Invalid Index Error Test

 * Check that calling GetQueryIndexediv with target TRANSFORM_FEEDBACK_-
 OVERFLOW_ARB and a non-zero index generates an INVALID_VALUE error.

 * If GL 4.0 or ARB_transform_feedback3 is supported, check that calling
 GetQueryIndexediv with target TRANSFORM_FEEDBACK_STREAM_OVERFLOW
 and an index greater than or equal to MAX_VERTEX_STREAMS generates an
 INVALID_VALUE error.

 * Check that calling BeginQueryIndexed with target TRANSFORM_FEEDBACK_-
 OVERFLOW_ARB and a non-zero index generates an INVALID_VALUE error.

 * If GL 4.0 or ARB_transform_feedback3 is supported, check that calling
 BeginQueryIndexed with target TRANSFORM_FEEDBACK_STREAM_OVERFLOW
 and an index greater than or equal to MAX_VERTEX_STREAMS generates an
 INVALID_VALUE error.
 */
class TransformFeedbackOverflowQueryErrorInvalidIndex : public TransformFeedbackOverflowQueryErrorBase
{
public:
	TransformFeedbackOverflowQueryErrorInvalidIndex(deqp::Context&							 context,
													TransformFeedbackOverflowQueryTests::API api, const char* name)
		: TransformFeedbackOverflowQueryErrorBase(
			  context, api, name, "Verifies whether an INVALID_VALUE error is properly generated if GetQueryIndexediv "
								  "or BeginQueryIndexed is called "
								  "with an invalid index when using the new targets introduced by the feature.")
		, m_query(0)
	{
	}

	/* Test case init. */
	virtual void init()
	{
		TransformFeedbackOverflowQueryErrorBase::init();

		const glw::Functions& gl = m_context.getRenderContext().getFunctions();

		gl.genQueries(1, &m_query);
	}

	/* Test case deinit */
	virtual void deinit()
	{
		const glw::Functions& gl = m_context.getRenderContext().getFunctions();

		gl.deleteQueries(1, &m_query);

		TransformFeedbackOverflowQueryErrorBase::deinit();
	}

	/* Test case iterate function. Contains the actual test case logic. */
	IterateResult iterate()
	{
		const glw::Functions& gl = m_context.getRenderContext().getFunctions();
		GLint				  value;

		startTest("GetQueryIndexediv must generate INVALID_VALUE if <target> is "
				  "TRANSFORM_FEEDBACK_OVERFLOW and <index> is non-zero.");

		for (GLuint i = 1; i < getMaxVertexStreams(); ++i)
		{
			gl.getQueryIndexediv(GL_TRANSFORM_FEEDBACK_OVERFLOW, i, GL_CURRENT_QUERY, &value);
			verifyError(GL_INVALID_VALUE);
		}

		if (supportsTransformFeedback3())
		{
			startTest("GetQueryIndexediv must generate INVALID_VALUE if <target> is "
					  "TRANSFORM_FEEDBACK_STREAM_OVERFLOW and <index> is greater "
					  "than or equal to MAX_VERTEX_STREAMS.");

			for (GLuint i = getMaxVertexStreams(); i < getMaxVertexStreams() + 4; ++i)
			{
				gl.getQueryIndexediv(GL_TRANSFORM_FEEDBACK_STREAM_OVERFLOW, i, GL_CURRENT_QUERY, &value);
				verifyError(GL_INVALID_VALUE);
			}
		}

		startTest("BeginQueryIndexed must generate INVALID_VALUE if <target> is "
				  "TRANSFORM_FEEDBACK_OVERFLOW and <index> is non-zero.");

		for (GLuint i = 1; i < getMaxVertexStreams(); ++i)
		{
			gl.beginQueryIndexed(GL_TRANSFORM_FEEDBACK_OVERFLOW, i, m_query);
			verifyError(GL_INVALID_VALUE);
		}

		if (supportsTransformFeedback3())
		{
			startTest("BeginQueryIndexed must generate INVALID_VALUE if <target> is "
					  "TRANSFORM_FEEDBACK_STREAM_OVERFLOW and <index> is greater "
					  "than or equal to MAX_VERTEX_STREAMS.");

			for (GLuint i = getMaxVertexStreams(); i < getMaxVertexStreams() + 4; ++i)
			{
				gl.beginQueryIndexed(GL_TRANSFORM_FEEDBACK_STREAM_OVERFLOW, i, m_query);
				verifyError(GL_INVALID_VALUE);
			}
		}

		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

		return STOP;
	}

protected:
	GLuint m_query;
};

/*
 API Already Active Error Test

 * Check that calling BeginQuery with target TRANSFORM_FEEDBACK_OVERFLOW
 generates an INVALID_OPERATION error if there is already an active
 query for TRANSFORM_FEEDBACK_OVERFLOW.

 * If GL 4.0 or ARB_transform_feedback3 is supported, check that calling
 BeginQueryIndexed with target TRANSFORM_FEEDBACK_STREAM_OVERFLOW
 generates an INVALID_OPERATION error if there is already an active
 query for TRANSFORM_FEEDBACK_STREAM_OVERFLOW for the specified
 index.

 * If GL 4.0 or ARB_transform_feedback3 is supported, check that calling
 BeginQueryIndexed with target TRANSFORM_FEEDBACK_STREAM_OVERFLOW
 generates an INVALID_OPERATION error if the specified query is already
 active on another TRANSFORM_FEEDBACK_STREAM_OVERFLOW target with
 a different index.
 */
class TransformFeedbackOverflowQueryErrorAlreadyActive : public TransformFeedbackOverflowQueryErrorBase
{
public:
	TransformFeedbackOverflowQueryErrorAlreadyActive(deqp::Context&							  context,
													 TransformFeedbackOverflowQueryTests::API api, const char* name)
		: TransformFeedbackOverflowQueryErrorBase(context, api, name,
												  "Verifies whether an INVALID_OPERATION error is properly generated "
												  "if BeginQuery[Indexed] is used to try to start "
												  "a query on an index that has already a query active, or the query "
												  "object itself is active on another index.")
		, m_query(0)
		, m_active_overflow_query(0)
		, m_active_stream_overflow_query(0)
		, m_active_query_stream_index(0)
	{
	}

	/* Test case init. */
	virtual void init()
	{
		TransformFeedbackOverflowQueryErrorBase::init();

		const glw::Functions& gl = m_context.getRenderContext().getFunctions();

		gl.genQueries(1, &m_query);

		gl.genQueries(1, &m_active_overflow_query);
		gl.beginQuery(GL_TRANSFORM_FEEDBACK_OVERFLOW, m_active_overflow_query);

		if (supportsTransformFeedback3())
		{
			gl.genQueries(1, &m_active_stream_overflow_query);
			m_active_query_stream_index = 2;
			gl.beginQueryIndexed(GL_TRANSFORM_FEEDBACK_STREAM_OVERFLOW, m_active_query_stream_index,
								 m_active_stream_overflow_query);
		}
	}

	/* Test case deinit */
	virtual void deinit()
	{
		const glw::Functions& gl = m_context.getRenderContext().getFunctions();

		if (supportsTransformFeedback3())
		{
			gl.endQueryIndexed(GL_TRANSFORM_FEEDBACK_STREAM_OVERFLOW, m_active_query_stream_index);
			gl.deleteQueries(1, &m_active_stream_overflow_query);
		}

		gl.endQuery(GL_TRANSFORM_FEEDBACK_OVERFLOW);
		gl.deleteQueries(1, &m_active_overflow_query);

		gl.deleteQueries(1, &m_query);

		TransformFeedbackOverflowQueryErrorBase::deinit();
	}

	/* Test case iterate function. Contains the actual test case logic. */
	IterateResult iterate()
	{
		const glw::Functions& gl = m_context.getRenderContext().getFunctions();

		startTest("BeginQuery[Indexed] must generate INVALID_OPERATION if <target> is "
				  "TRANSFORM_FEEDBACK_OVERFLOW and there is already an active "
				  "query for TRANSFORM_FEEDBACK_ARB.");

		gl.beginQuery(GL_TRANSFORM_FEEDBACK_OVERFLOW, m_query);
		verifyError(GL_INVALID_OPERATION);
		gl.beginQueryIndexed(GL_TRANSFORM_FEEDBACK_OVERFLOW, 0, m_query);
		verifyError(GL_INVALID_OPERATION);

		if (supportsTransformFeedback3())
		{
			startTest("BeginQueryIndexed must generate INVALID_OPERATION if <target> is "
					  "TRANSFORM_FEEDBACK_STREAM_OVERFLOW and there is already an active "
					  "query for TRANSFORM_FEEDBACK_STREAM_OVERFLOW for the specified index.");

			gl.beginQueryIndexed(GL_TRANSFORM_FEEDBACK_STREAM_OVERFLOW, m_active_query_stream_index, m_query);
			verifyError(GL_INVALID_OPERATION);

			startTest("BeginQuery[Indexed] must generate INVALID_OPERATION if <target> is "
					  "TRANSFORM_FEEDBACK_STREAM_OVERFLOW and the specified query is "
					  "already active on another TRANSFORM_FEEDBACK_STREAM_OVERFLOW "
					  "target with a different index.");

			for (GLuint i = 0; i < getMaxVertexStreams(); ++i)
			{
				if (i != m_active_query_stream_index)
				{
					gl.beginQueryIndexed(GL_TRANSFORM_FEEDBACK_STREAM_OVERFLOW, i, m_active_stream_overflow_query);
					verifyError(GL_INVALID_OPERATION);

					if (i == 0)
					{
						gl.beginQuery(GL_TRANSFORM_FEEDBACK_STREAM_OVERFLOW, m_active_stream_overflow_query);
						verifyError(GL_INVALID_OPERATION);
					}
				}
			}
		}

		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

		return STOP;
	}

protected:
	GLuint m_query;
	GLuint m_active_overflow_query;
	GLuint m_active_stream_overflow_query;
	GLuint m_active_query_stream_index;
};

/*
 API Incompatible Target Error Test

 * If GL 4.0 or ARB_transform_feedback3 is supported, check that calling
 BeginQueryIndexed with target TRANSFORM_FEEDBACK_STREAM_OVERFLOW
 generates an INVALID_OPERATION error if the specified query was
 previously used as a TRANSFORM_FEEDBACK_OVERFLOW query. Also check
 the other way around.
 */
class TransformFeedbackOverflowQueryErrorIncompatibleTarget : public TransformFeedbackOverflowQueryErrorBase
{
public:
	TransformFeedbackOverflowQueryErrorIncompatibleTarget(deqp::Context&						   context,
														  TransformFeedbackOverflowQueryTests::API api,
														  const char*							   name)
		: TransformFeedbackOverflowQueryErrorBase(context, api, name,
												  "Verifies whether an INVALID_OPERATION error is properly generated "
												  "if BeginQuery[Indexed] is called with one of "
												  "the newly introduced query targets but one that is different than "
												  "that used earlier on the same query object.")
		, m_overflow_query(0)
		, m_stream_overflow_query(0)
		, m_incompatible_query(0)
	{
	}

	/* Test case init. */
	virtual void init()
	{
		TransformFeedbackOverflowQueryErrorBase::init();

		const glw::Functions& gl = m_context.getRenderContext().getFunctions();

		gl.genQueries(1, &m_incompatible_query);
		gl.beginQuery(GL_SAMPLES_PASSED, m_incompatible_query);
		gl.endQuery(GL_SAMPLES_PASSED);

		gl.genQueries(1, &m_overflow_query);
		gl.beginQuery(GL_TRANSFORM_FEEDBACK_OVERFLOW, m_overflow_query);
		gl.endQuery(GL_TRANSFORM_FEEDBACK_OVERFLOW);

		if (supportsTransformFeedback3())
		{
			gl.genQueries(1, &m_stream_overflow_query);
			gl.beginQuery(GL_TRANSFORM_FEEDBACK_STREAM_OVERFLOW, m_stream_overflow_query);
			gl.endQuery(GL_TRANSFORM_FEEDBACK_STREAM_OVERFLOW);
		}
	}

	/* Test case deinit */
	virtual void deinit()
	{
		const glw::Functions& gl = m_context.getRenderContext().getFunctions();

		gl.deleteQueries(1, &m_incompatible_query);

		gl.deleteQueries(1, &m_overflow_query);

		if (supportsTransformFeedback3())
		{
			gl.deleteQueries(1, &m_stream_overflow_query);
		}

		TransformFeedbackOverflowQueryErrorBase::deinit();
	}

	/* Test case iterate function. Contains the actual test case logic. */
	IterateResult iterate()
	{
		const glw::Functions& gl = m_context.getRenderContext().getFunctions();

		startTest("BeginQuery[Indexed] must generate INVALID_OPERATION if <target> is "
				  "TRANSFORM_FEEDBACK_OVERFLOW and the specified query was "
				  "previously used with another target.");

		gl.beginQuery(GL_TRANSFORM_FEEDBACK_OVERFLOW, m_incompatible_query);
		verifyError(GL_INVALID_OPERATION);
		gl.beginQueryIndexed(GL_TRANSFORM_FEEDBACK_OVERFLOW, 0, m_incompatible_query);
		verifyError(GL_INVALID_OPERATION);

		if (supportsTransformFeedback3())
		{
			gl.beginQuery(GL_TRANSFORM_FEEDBACK_OVERFLOW, m_stream_overflow_query);
			verifyError(GL_INVALID_OPERATION);
			gl.beginQueryIndexed(GL_TRANSFORM_FEEDBACK_OVERFLOW, 0, m_stream_overflow_query);
			verifyError(GL_INVALID_OPERATION);

			startTest("BeginQuery[Indexed] must generate INVALID_OPERATION if <target> is "
					  "TRANSFORM_FEEDBACK_STREAM_OVERFLOW and the specified query "
					  "was previously used with another target.");

			gl.beginQuery(GL_TRANSFORM_FEEDBACK_STREAM_OVERFLOW, m_incompatible_query);
			verifyError(GL_INVALID_OPERATION);
			gl.beginQueryIndexed(GL_TRANSFORM_FEEDBACK_STREAM_OVERFLOW, 2, m_incompatible_query);
			verifyError(GL_INVALID_OPERATION);

			gl.beginQuery(GL_TRANSFORM_FEEDBACK_STREAM_OVERFLOW, m_overflow_query);
			verifyError(GL_INVALID_OPERATION);
			gl.beginQueryIndexed(GL_TRANSFORM_FEEDBACK_STREAM_OVERFLOW, 2, m_overflow_query);
			verifyError(GL_INVALID_OPERATION);
		}

		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

		return STOP;
	}

protected:
	GLuint m_overflow_query;
	GLuint m_stream_overflow_query;
	GLuint m_incompatible_query;
};

/*
 API No Query Active Error Test

 * Check that calling EndQuery with target TRANSFORM_FEEDBACK_OVERFLOW
 generates an INVALID_OPERATION error if no query is active for
 TRANSFORM_FEEDBACK_OVERFLOW.

 * If GL 4.0 or ARB_transform_feedback3 is supported, check that calling
 EndQueryIndexed with target TRANSFORM_FEEDBACK_STREAM_OVERFLOW
 generates an INVALID_OPERATION error if no query is active for
 TRANSFORM_FEEDBACK_STREAM_OVERFLOW for the specified index, even
 if there is an active query for another index.
 */
class TransformFeedbackOverflowQueryErrorNoActiveQuery : public TransformFeedbackOverflowQueryErrorBase
{
public:
	TransformFeedbackOverflowQueryErrorNoActiveQuery(deqp::Context&							  context,
													 TransformFeedbackOverflowQueryTests::API api, const char* name)
		: TransformFeedbackOverflowQueryErrorBase(context, api, name,
												  "Verifies whether an INVALID_OPERATION error is properly generated "
												  "if EndQuery[Indexed] is called with a target "
												  "(and index) for which there isn't a currently active query.")
		, m_active_stream_overflow_query(0)
		, m_active_query_stream_index(0)
	{
	}

	/* Test case init. */
	virtual void init()
	{
		TransformFeedbackOverflowQueryErrorBase::init();

		const glw::Functions& gl = m_context.getRenderContext().getFunctions();

		if (supportsTransformFeedback3())
		{
			gl.genQueries(1, &m_active_stream_overflow_query);
			m_active_query_stream_index = 2;
			gl.beginQueryIndexed(GL_TRANSFORM_FEEDBACK_STREAM_OVERFLOW, m_active_query_stream_index,
								 m_active_stream_overflow_query);
		}
	}

	/* Test case deinit */
	virtual void deinit()
	{
		const glw::Functions& gl = m_context.getRenderContext().getFunctions();

		if (supportsTransformFeedback3())
		{
			gl.endQueryIndexed(GL_TRANSFORM_FEEDBACK_STREAM_OVERFLOW, m_active_query_stream_index);
			gl.deleteQueries(1, &m_active_stream_overflow_query);
		}

		TransformFeedbackOverflowQueryErrorBase::deinit();
	}

	/* Test case iterate function. Contains the actual test case logic. */
	IterateResult iterate()
	{
		const glw::Functions& gl = m_context.getRenderContext().getFunctions();

		startTest("EndQuery[Indexed] must generate INVALID_OPERATION if <target> is "
				  "TRANSFORM_FEEDBACK_OVERFLOW and there is no query active "
				  "for TRANSFORM_FEEDBACK_OVERFLOW.");

		gl.endQuery(GL_TRANSFORM_FEEDBACK_OVERFLOW);
		verifyError(GL_INVALID_OPERATION);
		gl.endQueryIndexed(GL_TRANSFORM_FEEDBACK_OVERFLOW, 0);
		verifyError(GL_INVALID_OPERATION);

		if (supportsTransformFeedback3())
		{
			startTest("EndQuery[Indexed] must generate INVALID_OPERATION if <target> is "
					  "TRANSFORM_FEEDBACK_STREAM_OVERFLOW and there is no query active "
					  "for TRANSFORM_FEEDBACK_STREAM_OVERFLOW for the given index.");

			for (GLuint i = 0; i < getMaxVertexStreams(); ++i)
			{
				if (i != m_active_query_stream_index)
				{
					gl.endQueryIndexed(GL_TRANSFORM_FEEDBACK_STREAM_OVERFLOW, i);
					verifyError(GL_INVALID_OPERATION);

					if (i == 0)
					{
						gl.endQuery(GL_TRANSFORM_FEEDBACK_STREAM_OVERFLOW);
						verifyError(GL_INVALID_OPERATION);
					}
				}
			}
		}

		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

		return STOP;
	}

protected:
	GLuint m_active_stream_overflow_query;
	GLuint m_active_query_stream_index;
};

/*
 Base class of all functionality tests. Helps enforce the following requirements:

 * Ensuring that QUERY_COUNTER_BITS is at least one for the TRANSFORM_FEEDBACK_OVERFLOW query
 target before running any test that uses such a query's result.

 * Ensuring that GL 4.0 or ARB_transform_feedback3 is supported and QUERY_COUNTER_BITS is at least
 one for the TRANSFORM_FEEDBACK_STREAM_OVERFLOW query target before running any test that
 uses such a query's result.
 */
class TransformFeedbackOverflowQueryFunctionalBase : public TransformFeedbackOverflowQueryBaseTest
{
protected:
	TransformFeedbackOverflowQueryFunctionalBase(deqp::Context& context, TransformFeedbackOverflowQueryTests::API api,
												 const char* name, const char* description)
		: TransformFeedbackOverflowQueryBaseTest(context, api, name, description)
		, m_overflow_query(0)
		, m_stream_overflow_query(NULL)
		, m_query_buffer(0)
		, m_tf_buffer_count(0)
		, m_tf_buffer(NULL)
		, m_vao(0)
		, m_program(0)
		, m_checker_program(NULL)
	{
	}

	/* Tells whether functional tests using TRANSFORM_FEEDBACK_OVERFLOW are runnable */
	bool canTestOverflow()
	{
		const glw::Functions& gl = m_context.getRenderContext().getFunctions();
		GLint				  counterBits;

		gl.getQueryiv(GL_TRANSFORM_FEEDBACK_OVERFLOW, GL_QUERY_COUNTER_BITS, &counterBits);

		return counterBits > 0;
	}

	/* Tells whether functional tests using TRANSFORM_FEEDBACK_STREAM_OVERFLOW are runnable */
	bool canTestStreamOverflow()
	{
		if (supportsTransformFeedback3())
		{
			const glw::Functions& gl = m_context.getRenderContext().getFunctions();
			GLint				  counterBits;

			gl.getQueryiv(GL_TRANSFORM_FEEDBACK_STREAM_OVERFLOW, GL_QUERY_COUNTER_BITS, &counterBits);

			return counterBits > 0;
		}
		else
		{
			return false;
		}
	}

	/* Dummy vertex shader. */
	const char* dummyVsh()
	{
		return "#version 150 core\n"
			   "void main() {\n"
			   "    gl_Position = vec4(0.0, 0.0, 0.0, 1.0);\n"
			   "}\n";
	}

	/* Dummy fragment shader */
	const char* dummyFsh()
	{
		return "#version 150 core\n"
			   "void main() {}\n";
	}

	/* Functional test init. Creates necessary query objects. */
	virtual void init()
	{
		TransformFeedbackOverflowQueryBaseTest::init();

		const glw::Functions& gl = m_context.getRenderContext().getFunctions();

		if (canTestOverflow())
		{
			// Setup vertex array
			gl.genVertexArrays(1, &m_vao);
			gl.bindVertexArray(m_vao);

			// Setup queries
			gl.genQueries(1, &m_overflow_query);

			if (canTestStreamOverflow())
			{
				m_stream_overflow_query = new GLuint[getMaxVertexStreams()];

				gl.genQueries(getMaxVertexStreams(), m_stream_overflow_query);
			}

			// Setup checker program
			m_checker_program =
				new glu::ShaderProgram(m_context.getRenderContext(), glu::makeVtxFragSources(dummyVsh(), dummyFsh()));
			if (!m_checker_program->isOk())
			{
				TCU_FAIL("Checker program compilation failed");
			}

			// Setup transform feedback shader and buffers
			buildTransformFeedbackProgram();
			setupTransformFeedbackBuffers();
		}
		else
		{
			throw tcu::NotSupportedError(
				"QUERY_COUNTER_BITS for TRANSFORM_FEEDBACK_OVERFLOW queries is zero, skipping test");
		}
	}

	/* Functional test deinit. Deletes created query objects */
	virtual void deinit()
	{
		const glw::Functions& gl = m_context.getRenderContext().getFunctions();

		gl.deleteVertexArrays(1, &m_vao);

		gl.deleteQueries(1, &m_overflow_query);

		if (canTestStreamOverflow())
		{
			if (m_stream_overflow_query != NULL)
			{
				gl.deleteQueries(getMaxVertexStreams(), m_stream_overflow_query);

				delete[] m_stream_overflow_query;
			}
		}

		if (m_checker_program != NULL)
		{
			delete m_checker_program;
		}

		gl.useProgram(0);
		gl.deleteProgram(m_program);

		if (m_tf_buffer != NULL)
		{
			gl.deleteBuffers(m_tf_buffer_count, m_tf_buffer);

			delete[] m_tf_buffer;
		}

		TransformFeedbackOverflowQueryBaseTest::deinit();
	}

	/*
	 Basic Checking Mechanism

	 * Call BeginConditionalRender with mode QUERY_WAIT and with the given
	 query object as parameters. Draw something, then call EndConditional-
	 Render. If the expected result for the query is FALSE, expect
	 conditional render to discard the previous draw command.

	 * If GL 4.5 or ARB_conditional_render_inverted is supported, call Begin-
	 ConditionalRender with mode QUERY_WAIT_INVERTED and with the given query
	 object as parameters. Draw something, then call EndConditionalRender. If
	 the expected result for the query is TRUE, expect conditional render to
	 discard the previous draw command.

	 * Finally, check using GetQueryObjectiv with QUERY_RESULT that the result
	 of the query matches the expected result.

	 * If GL 4.4 or ARB_query_buffer_object is supported then check the result
	 of the query against the expected result also by having a query buffer
	 bound at the time of calling GetQueryObjectiv.
	 */
	bool verifyQueryResult(GLuint query, GLboolean expected)
	{
		const glw::Functions& gl	 = m_context.getRenderContext().getFunctions();
		bool				  result = true;
		GLuint				  actual;

		GLuint current_program = m_context.getContextInfo().getInt(GL_CURRENT_PROGRAM);
		GLuint checker_query, check_result;

		// We'll use a PRIMITIVES_GENERATED query to test whether conditional
		// rendering actually executed the draw command or not. If the draw command
		// was discarded then the PRIMITIVES_GENERATED query should have a result
		// of zero.
		gl.genQueries(1, &checker_query);

		gl.useProgram(m_checker_program->getProgram());

		// Verify that conditional render discards the rendering if the expected
		// result is FALSE and renders otherwise.
		gl.beginConditionalRender(query, GL_QUERY_WAIT);
		gl.beginQuery(GL_PRIMITIVES_GENERATED, checker_query);
		gl.drawArrays(GL_POINTS, 0, 1);
		gl.endQuery(GL_PRIMITIVES_GENERATED);
		gl.endConditionalRender();
		gl.getQueryObjectuiv(checker_query, GL_QUERY_RESULT, &check_result);
		if (check_result != (GLuint)expected)
		{
			result = false;
		}

		// Verify that an inverted conditional render discards the rendering if
		// the expected result is TRUE and renders otherwise.
		if (supportsConditionalRenderInverted())
		{
			gl.beginConditionalRender(query, GL_QUERY_WAIT_INVERTED);
			gl.beginQuery(GL_PRIMITIVES_GENERATED, checker_query);
			gl.drawArrays(GL_POINTS, 0, 1);
			gl.endQuery(GL_PRIMITIVES_GENERATED);
			gl.endConditionalRender();
			gl.getQueryObjectuiv(checker_query, GL_QUERY_RESULT, &check_result);
			if (check_result == (GLuint)expected)
			{
				result = false;
			}
		}

		gl.useProgram(current_program);

		// Verify that the result of the query matches the expected result.
		gl.getQueryObjectuiv(query, GL_QUERY_RESULT, &actual);
		if ((GLboolean)actual != expected)
		{
			result = false;
		}

		// Verify that the result of the query matches the expected result even
		// when using a query buffer.
		if (supportsQueryBufferObject())
		{
			const GLuint initValue = 0xDEADBEEF;

			gl.genBuffers(1, &m_query_buffer);
			gl.bindBuffer(GL_QUERY_BUFFER, m_query_buffer);
			gl.bufferData(GL_QUERY_BUFFER, sizeof(initValue), &initValue, GL_STREAM_READ);
			gl.getQueryObjectuiv(query, GL_QUERY_RESULT, NULL);
			gl.getBufferSubData(GL_QUERY_BUFFER, 0, sizeof(actual), &actual);
			gl.deleteBuffers(1, &m_query_buffer);

			if ((GLboolean)actual != expected)
			{
				result = false;
			}
		}

		gl.deleteQueries(1, &checker_query);

		return result;
	}

	/* Verifies the result of all queries. There can only be up to 5 non-FALSE result queries as none of
	 the tests use more than 4 vertex streams so the rest is assumed to always have a FALSE result. */
	void verifyQueryResults(GLboolean any, GLboolean stream0, GLboolean stream1, GLboolean stream2 = GL_FALSE,
							GLboolean stream3 = GL_FALSE)
	{
		bool result = true;

		// Verify the result of the TRANSFORM_FEEDBACK_OVERFLOW query.
		result &= verifyQueryResult(m_overflow_query, any);

		if (supportsTransformFeedback3())
		{
			// Verify the result of the TRANSFORM_FEEDBACK_STREAM_OVERFLOW queries
			// corresponding to the first 4 vertex streams.
			result &= verifyQueryResult(m_stream_overflow_query[0], stream0);
			result &= verifyQueryResult(m_stream_overflow_query[1], stream1);
			result &= verifyQueryResult(m_stream_overflow_query[2], stream2);
			result &= verifyQueryResult(m_stream_overflow_query[3], stream3);

			// Expect the rest of the TRANSFORM_FEEDBACK_STREAM_OVERFLOW queries
			// to have a FALSE result.
			for (GLuint i = 4; i < getMaxVertexStreams(); ++i)
			{
				result &= verifyQueryResult(m_stream_overflow_query[i], GL_FALSE);
			}
		}

		if (!result)
		{
			TCU_FAIL("Failed to validate the results of the queries");
		}
	}

	/* Single stream version of verifyQueryResults */
	void verifyQueryResults(GLboolean result)
	{
		verifyQueryResults(result, result, GL_FALSE, GL_FALSE, GL_FALSE);
	}

	/* Compiles and links transform feedback program. */
	void buildTransformFeedbackProgram()
	{
		const glw::Functions& gl = m_context.getRenderContext().getFunctions();

		GLint status;

		m_program = gl.createProgram();

		const char* vsSource = transformFeedbackVertexShader();

		GLuint vShader = gl.createShader(GL_VERTEX_SHADER);
		gl.shaderSource(vShader, 1, (const char**)&vsSource, NULL);
		gl.compileShader(vShader);
		gl.getShaderiv(vShader, GL_COMPILE_STATUS, &status);
		if (status == GL_FALSE)
		{
			GLint infoLogLength = 0;
			gl.getShaderiv(vShader, GL_INFO_LOG_LENGTH, &infoLogLength);

			std::vector<char> infoLogBuf(infoLogLength + 1);
			gl.getShaderInfoLog(vShader, (GLsizei)infoLogBuf.size(), NULL, &infoLogBuf[0]);

			std::string infoLog = &infoLogBuf[0];
			m_testCtx.getLog().writeShader(QP_SHADER_TYPE_VERTEX, vsSource, false, infoLog.c_str());

			gl.deleteShader(vShader);

			TCU_FAIL("Failed to compile transform feedback vertex shader");
		}
		gl.attachShader(m_program, vShader);
		gl.deleteShader(vShader);

		const char* gsSource = transformFeedbackGeometryShader();

		if (gsSource)
		{
			GLuint gShader = gl.createShader(GL_GEOMETRY_SHADER);
			gl.shaderSource(gShader, 1, (const char**)&gsSource, NULL);
			gl.compileShader(gShader);
			gl.getShaderiv(gShader, GL_COMPILE_STATUS, &status);
			if (status == GL_FALSE)
			{
				GLint infoLogLength = 0;
				gl.getShaderiv(gShader, GL_INFO_LOG_LENGTH, &infoLogLength);

				std::vector<char> infoLogBuf(infoLogLength + 1);
				gl.getShaderInfoLog(gShader, (GLsizei)infoLogBuf.size(), NULL, &infoLogBuf[0]);

				std::string infoLog = &infoLogBuf[0];
				m_testCtx.getLog().writeShader(QP_SHADER_TYPE_GEOMETRY, gsSource, false, infoLog.c_str());

				gl.deleteShader(gShader);

				TCU_FAIL("Failed to compile transform feedback geometry shader");
			}
			gl.attachShader(m_program, gShader);
			gl.deleteShader(gShader);
		}

		gl.transformFeedbackVaryings(m_program, varyingsCount(), varyings(), bufferMode());
		gl.linkProgram(m_program);
		gl.getProgramiv(m_program, GL_LINK_STATUS, &status);
		if (status == GL_FALSE)
		{
			GLint infoLogLength = 0;
			gl.getProgramiv(m_program, GL_INFO_LOG_LENGTH, &infoLogLength);

			std::vector<char> infoLogBuf(infoLogLength + 1);
			gl.getProgramInfoLog(m_program, (GLsizei)infoLogBuf.size(), NULL, &infoLogBuf[0]);

			std::string infoLog = &infoLogBuf[0];
			m_testCtx.getLog().writeShader(QP_SHADER_TYPE_VERTEX, vsSource, true, infoLog.c_str());

			TCU_FAIL("Failed to link transform feedback program");
		}

		gl.useProgram(m_program);
	}

	/* Generates a number of transform feedback buffers and binds them. */
	void setupTransformFeedbackBuffers()
	{
		const glw::Functions& gl = m_context.getRenderContext().getFunctions();

		m_tf_buffer_count = bufferCount();

		m_tf_buffer = new GLuint[m_tf_buffer_count];

		gl.genBuffers(m_tf_buffer_count, m_tf_buffer);

		for (GLint i = 0; i < m_tf_buffer_count; ++i)
		{
			gl.bindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, i, m_tf_buffer[i]);
			gl.bufferData(GL_TRANSFORM_FEEDBACK_BUFFER, bufferSize(i), NULL, GL_DYNAMIC_COPY);
		}
	}

	/* Starts all overflow queries. */
	void beginQueries()
	{
		const glw::Functions& gl = m_context.getRenderContext().getFunctions();

		gl.beginQuery(GL_TRANSFORM_FEEDBACK_OVERFLOW, m_overflow_query);

		if (supportsTransformFeedback3())
		{
			for (GLuint i = 0; i < getMaxVertexStreams(); ++i)
			{
				gl.beginQueryIndexed(GL_TRANSFORM_FEEDBACK_STREAM_OVERFLOW, i, m_stream_overflow_query[i]);
			}
		}
	}

	/* Stops all overflow queries. */
	void endQueries()
	{
		const glw::Functions& gl = m_context.getRenderContext().getFunctions();

		gl.endQuery(GL_TRANSFORM_FEEDBACK_OVERFLOW);

		if (supportsTransformFeedback3())
		{
			for (GLuint i = 0; i < getMaxVertexStreams(); ++i)
			{
				gl.endQueryIndexed(GL_TRANSFORM_FEEDBACK_STREAM_OVERFLOW, i);
			}
		}
	}

	/* Draws a set of points to vertex stream #0 while having the overflow queries active. */
	void drawPoints(GLsizei count)
	{
		const glw::Functions& gl = m_context.getRenderContext().getFunctions();

		beginQueries();
		gl.drawArrays(GL_POINTS, 0, count);
		endQueries();
	}

	/* Draws a set of triangles to vertex stream #0 while having the overflow queries active. */
	void drawTriangles(GLsizei count)
	{
		const glw::Functions& gl = m_context.getRenderContext().getFunctions();

		beginQueries();
		gl.drawArrays(GL_TRIANGLES, 0, count * 3);
		endQueries();
	}

	/* Vertex shader to use for transform feedback. */
	virtual const char* transformFeedbackVertexShader() = 0;

	/* Geometry shader to use for transform feedback. */
	virtual const char* transformFeedbackGeometryShader() = 0;

	/* Returns the number of transform feedback varyings. */
	virtual GLsizei varyingsCount() = 0;

	/* Returns the array of transform feedback varying names. */
	virtual const char** varyings() = 0;

	/* Returns the transform feedback buffer mode. */
	virtual GLenum bufferMode() = 0;

	/* Returns the number of transform feedback buffers. */
	virtual GLsizei bufferCount() = 0;

	/* Returns the size of the specified transform feedback buffer. */
	virtual GLsizei bufferSize(GLint index) = 0;

protected:
	GLuint				m_overflow_query;
	GLuint*				m_stream_overflow_query;
	GLuint				m_query_buffer;
	GLsizei				m_tf_buffer_count;
	GLuint*				m_tf_buffer;
	GLuint				m_vao;
	GLuint				m_program;
	glu::ShaderProgram* m_checker_program;
};

/*
 Base class for all single stream test cases.
 */
class TransformFeedbackOverflowQuerySingleStreamBase : public TransformFeedbackOverflowQueryFunctionalBase
{
protected:
	TransformFeedbackOverflowQuerySingleStreamBase(deqp::Context& context, TransformFeedbackOverflowQueryTests::API api,
												   const char* name, const char* description)
		: TransformFeedbackOverflowQueryFunctionalBase(context, api, name, description)
	{
	}

	/* Vertex shader to use for transform feedback. */
	virtual const char* transformFeedbackVertexShader()
	{
		return "#version 150 core\n"
			   "out float output1;\n"
			   "out float output2;\n"
			   "out float output3;\n"
			   "out float output4;\n"
			   "void main() {\n"
			   "    output1 = 1.0;\n"
			   "    output2 = 2.0;\n"
			   "    output3 = 3.0;\n"
			   "    output4 = 4.0;\n"
			   "}";
	}

	/* No geometry shader for single stream test cases. */
	virtual const char* transformFeedbackGeometryShader()
	{
		return NULL;
	}

	/* There are a total of 4 varyings. */
	virtual GLsizei varyingsCount()
	{
		return 4;
	}

	/* The varying name list contains all outputs in order. */
	virtual const char** varyings()
	{
		static const char* vars[] = { "output1", "output2", "output3", "output4" };
		return vars;
	}
};

/*
 Test case #1 - Basic single stream, interleaved attributes.
 */
class TransformFeedbackOverflowQueryBasicSingleStreamInterleavedAttribs
	: public TransformFeedbackOverflowQuerySingleStreamBase
{
public:
	TransformFeedbackOverflowQueryBasicSingleStreamInterleavedAttribs(deqp::Context&						   context,
																	  TransformFeedbackOverflowQueryTests::API api,
																	  const char*							   name)
		: TransformFeedbackOverflowQuerySingleStreamBase(context, api, name,
														 "Basic single stream, interleaved attributes.")
	{
	}

	/* Use interleaved attributes. */
	virtual GLenum bufferMode()
	{
		return GL_INTERLEAVED_ATTRIBS;
	}

	/* A single transform feedback buffer is enough. */
	virtual GLsizei bufferCount()
	{
		return 1;
	}

	/* The transform feedback buffer should be able to capture exactly 10 vertices. */
	virtual GLsizei bufferSize(GLint index)
	{
		(void)index;
		return static_cast<GLsizei>(10 * sizeof(GLfloat) * varyingsCount());
	}

	/* Test case iterate function. Contains the actual test case logic. */
	IterateResult iterate()
	{
		const glw::Functions& gl = m_context.getRenderContext().getFunctions();

		// Call BeginTransformFeedback with mode POINTS.
		gl.beginTransformFeedback(GL_POINTS);

		// Start the query, submit draw that results in feeding back exactly 10
		// points, then stop the query.
		drawPoints(10);

		// Call EndTransformFeedback.
		gl.endTransformFeedback();

		// Use the basic checking mechanism to validate that the result of the
		// query is FALSE.
		verifyQueryResults(GL_FALSE);

		// Repeat the above steps, but this time feed back more than 10 vertices
		// and expect the result of the query to be TRUE.
		gl.beginTransformFeedback(GL_POINTS);
		drawPoints(11);
		gl.endTransformFeedback();
		verifyQueryResults(GL_TRUE);

		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

		return STOP;
	}
};

/*
 Test case #2 - Basic single stream, separate attributes.
 */
class TransformFeedbackOverflowQueryBasicSingleStreamSeparateAttribs
	: public TransformFeedbackOverflowQuerySingleStreamBase
{
public:
	TransformFeedbackOverflowQueryBasicSingleStreamSeparateAttribs(deqp::Context&							context,
																   TransformFeedbackOverflowQueryTests::API api,
																   const char*								name)
		: TransformFeedbackOverflowQuerySingleStreamBase(context, api, name,
														 "Basic single stream, separate attributes.")
	{
	}

	/* Use separate attributes. */
	virtual GLenum bufferMode()
	{
		return GL_SEPARATE_ATTRIBS;
	}

	/* We need a separate buffer for each varying. */
	virtual GLsizei bufferCount()
	{
		return varyingsCount();
	}

	/* One of the transform feedback buffers should be able to capture exactly 12 vertices,
	 the others should be able to capture at least 15 vertices. */
	virtual GLsizei bufferSize(GLint index)
	{
		if (index == 1)
		{
			return 12 * sizeof(GLfloat);
		}
		else
		{
			return 15 * sizeof(GLfloat);
		}
	}

	/* Test case iterate function. Contains the actual test case logic. */
	IterateResult iterate()
	{
		const glw::Functions& gl = m_context.getRenderContext().getFunctions();

		// Call BeginTransformFeedback with mode TRIANGLES.
		gl.beginTransformFeedback(GL_TRIANGLES);

		// Start the query, submit draw that results in feeding back exactly 4
		// triangles, then stop the query.
		drawTriangles(4);

		// Call EndTransformFeedback.
		gl.endTransformFeedback();

		// Use the basic checking mechanism to validate that the result of the
		// query is FALSE.
		verifyQueryResults(GL_FALSE);

		// Repeat the above steps, but this time feed back exactly 5 triangles
		// and expect the result of the query to be TRUE.
		gl.beginTransformFeedback(GL_TRIANGLES);
		drawTriangles(5);
		gl.endTransformFeedback();
		verifyQueryResults(GL_TRUE);

		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

		return STOP;
	}
};

/*
 Test case #3 - Advanced single stream, interleaved attributes.
 */
class TransformFeedbackOverflowQueryAdvancedSingleStreamInterleavedAttribs
	: public TransformFeedbackOverflowQuerySingleStreamBase
{
public:
	TransformFeedbackOverflowQueryAdvancedSingleStreamInterleavedAttribs(deqp::Context& context,
																		 TransformFeedbackOverflowQueryTests::API api,
																		 const char*							  name)
		: TransformFeedbackOverflowQuerySingleStreamBase(context, api, name,
														 "Advanced single stream, interleaved attributes.")
	{
	}

	/* Use interleaved attributes. */
	virtual GLenum bufferMode()
	{
		return GL_INTERLEAVED_ATTRIBS;
	}

	/* A single transform feedback buffer is enough. */
	virtual GLsizei bufferCount()
	{
		return 1;
	}

	/* The transform feedback buffer should be able to capture exactly 10 vertices. */
	virtual GLsizei bufferSize(GLint index)
	{
		(void)index;
		return static_cast<GLsizei>(10 * sizeof(GLfloat) * varyingsCount());
	}

	/* Test case iterate function. Contains the actual test case logic. */
	IterateResult iterate()
	{
		const glw::Functions& gl = m_context.getRenderContext().getFunctions();

		// If GL 4.0 and ARB_transform_feedback2 are not supported then skip this
		// test case.
		if (!supportsTransformFeedback2())
		{
			throw tcu::NotSupportedError("Required transform_feedback2 extension is not supported");
		}

		// Call BeginTransformFeedback with mode POINTS.
		gl.beginTransformFeedback(GL_POINTS);

		// Start the query, submit draw that results in feeding back exactly 8
		// triangles, then stop the query.
		drawPoints(8);

		// Call PauseTransformFeedback.
		gl.pauseTransformFeedback();

		// Use the basic checking mechanism to validate that the result of the
		// query is FALSE.
		verifyQueryResults(GL_FALSE);

		// Start the query, submit draw that would result in feeding back more than
		// 10 points if transform feedback wasn't paused, then stop the query.
		drawPoints(11);

		// Use the basic checking mechanism to validate that the result of the
		// query is FALSE.
		verifyQueryResults(GL_FALSE);

		// Call ResumeTransformFeedback.
		gl.resumeTransformFeedback();

		// Start the query, submit draw that results in feeding back exactly 1
		// point, then stop the query.
		drawPoints(1);

		// Call PauseTransformFeedback.
		gl.pauseTransformFeedback();

		// Use the basic checking mechanism to validate that the result of the
		// query is FALSE.
		verifyQueryResults(GL_FALSE);

		// Call ResumeTransformFeedback.
		gl.resumeTransformFeedback();

		// Start the query, submit draw that results in feeding back exactly 2
		// point, then stop the query.
		drawPoints(2);

		// Call EndTransformFeedback.
		gl.endTransformFeedback();

		// Use the basic checking mechanism to validate that the result of the
		// query is TRUE.
		verifyQueryResults(GL_TRUE);

		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

		return STOP;
	}
};

/*
 Test case #4 - Advanced single stream, separate attributes.
 */
class TransformFeedbackOverflowQueryAdvancedSingleStreamSeparateAttribs
	: public TransformFeedbackOverflowQuerySingleStreamBase
{
public:
	TransformFeedbackOverflowQueryAdvancedSingleStreamSeparateAttribs(deqp::Context&						   context,
																	  TransformFeedbackOverflowQueryTests::API api,
																	  const char*							   name)
		: TransformFeedbackOverflowQuerySingleStreamBase(context, api, name,
														 "Advanced single stream, separate attributes.")
	{
	}

	/* Use separate attributes. */
	virtual GLenum bufferMode()
	{
		return GL_SEPARATE_ATTRIBS;
	}

	/* We need a separate buffer for each varying. */
	virtual GLsizei bufferCount()
	{
		return varyingsCount();
	}

	/* One of the transform feedback buffers should be able to capture exactly 12 vertices,
	 the others should be able to capture at least 15 vertices. */
	virtual GLsizei bufferSize(GLint index)
	{
		if (index == 2)
		{
			return 12 * sizeof(GLfloat);
		}
		else
		{
			return 15 * sizeof(GLfloat);
		}
	}

	/* Test case iterate function. Contains the actual test case logic. */
	IterateResult iterate()
	{
		const glw::Functions& gl = m_context.getRenderContext().getFunctions();

		// If GL 4.0 and ARB_transform_feedback2 are not supported then skip this
		// test case.
		if (!supportsTransformFeedback2())
		{
			throw tcu::NotSupportedError("Required transform_feedback2 extension is not supported");
		}

		// Call BeginTransformFeedback with mode TRIANGLES.
		gl.beginTransformFeedback(GL_TRIANGLES);

		// Start the query, submit draw that results in feeding back exactly 2
		// triangles, then stop the query.
		drawTriangles(2);

		// Call PauseTransformFeedback.
		gl.pauseTransformFeedback();

		// Use the basic checking mechanism to validate that the result of the
		// query is FALSE.
		verifyQueryResults(GL_FALSE);

		// Start the query, submit draw that would result in feeding back more than
		// 4 triangles if transform feedback wasn't paused, then stop the query.
		drawTriangles(4);

		// Use the basic checking mechanism to validate that the result of the
		// query is FALSE.
		verifyQueryResults(GL_FALSE);

		// Call ResumeTransformFeedback.
		gl.resumeTransformFeedback();

		// Start the query, submit draw that results in feeding back exactly 2
		// triangles, then stop the query.
		drawTriangles(2);

		// Call PauseTransformFeedback.
		gl.pauseTransformFeedback();

		// Use the basic checking mechanism to validate that the result of the
		// query is FALSE.
		verifyQueryResults(GL_FALSE);

		// Call ResumeTransformFeedback.
		gl.resumeTransformFeedback();

		// Start the query, submit draw that results in feeding back exactly 1
		// triangles, then stop the query.
		drawTriangles(1);

		// Call EndTransformFeedback.
		gl.endTransformFeedback();

		// Use the basic checking mechanism to validate that the result of the
		// query is TRUE.
		verifyQueryResults(GL_TRUE);

		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

		return STOP;
	}
};

/*
 Base class for all multiple stream test cases.
 */
class TransformFeedbackOverflowQueryMultipleStreamsBase : public TransformFeedbackOverflowQueryFunctionalBase
{
protected:
	TransformFeedbackOverflowQueryMultipleStreamsBase(deqp::Context&						   context,
													  TransformFeedbackOverflowQueryTests::API api, const char* name,
													  const char* description)
		: TransformFeedbackOverflowQueryFunctionalBase(context, api, name, description)
	{
	}

	/* Vertex shader to use for transform feedback. */
	virtual const char* transformFeedbackVertexShader()
	{
		return "#version 150 core\n"
			   "void main() {\n"
			   "}";
	}

	/* Use interleaved attributes. */
	virtual GLenum bufferMode()
	{
		return GL_INTERLEAVED_ATTRIBS;
	}

	/* Always use 4 transform feedback buffers. */
	virtual GLsizei bufferCount()
	{
		return 4;
	}

	/* Draws a set of points to each vertex stream while having the overflow queries active. */
	void drawStreams(GLsizei count0, GLsizei count1 = 0, GLsizei count2 = 0, GLsizei count3 = 0)
	{
		const glw::Functions& gl = m_context.getRenderContext().getFunctions();

		GLint streamLoc = gl.getUniformLocation(m_program, "stream");

		beginQueries();

		gl.uniform1ui(streamLoc, 0);
		gl.drawArrays(GL_POINTS, 0, count0);

		gl.uniform1ui(streamLoc, 1);
		gl.drawArrays(GL_POINTS, 0, count1);

		gl.uniform1ui(streamLoc, 2);
		gl.drawArrays(GL_POINTS, 0, count2);

		gl.uniform1ui(streamLoc, 3);
		gl.drawArrays(GL_POINTS, 0, count3);

		endQueries();
	}
};

/*
 Test case #5 - Advanced multiple streams, one buffer per stream.
 */
class TransformFeedbackOverflowQueryMultipleStreamsOneBufferPerStream
	: public TransformFeedbackOverflowQueryMultipleStreamsBase
{
public:
	TransformFeedbackOverflowQueryMultipleStreamsOneBufferPerStream(deqp::Context&							 context,
																	TransformFeedbackOverflowQueryTests::API api,
																	const char*								 name)
		: TransformFeedbackOverflowQueryMultipleStreamsBase(context, api, name,
															"Advanced multiple streams, one buffer per stream.")
	{
	}

	/* Geometry shader to use for transform feedback. */
	virtual const char* transformFeedbackGeometryShader()
	{
		return "#version 150 core\n"
			   "#extension GL_ARB_gpu_shader5 : require\n"
			   "layout(points) in;\n"
			   "layout(points, max_vertices = 1) out;\n"
			   "layout(stream=0) out float output1;\n"
			   "layout(stream=1) out float output2;\n"
			   "layout(stream=2) out float output3;\n"
			   "layout(stream=3) out float output4;\n"
			   "uniform uint stream;\n"
			   "void main() {\n"
			   "    if (stream == 0) {\n"
			   "        output1 = 1.0;\n"
			   "        EmitStreamVertex(0);\n"
			   "        EndStreamPrimitive(0);\n"
			   "    }\n"
			   "    if (stream == 1) {\n"
			   "        output2 = 2.0;\n"
			   "        EmitStreamVertex(1);\n"
			   "        EndStreamPrimitive(1);\n"
			   "    }\n"
			   "    if (stream == 2) {\n"
			   "        output3 = 3.0;\n"
			   "        EmitStreamVertex(2);\n"
			   "        EndStreamPrimitive(2);\n"
			   "    }\n"
			   "    if (stream == 3) {\n"
			   "        output4 = 4.0;\n"
			   "        EmitStreamVertex(3);\n"
			   "        EndStreamPrimitive(3);\n"
			   "    }\n"
			   "}";
	}

	/* Together with the separators there are a total of 7 varyings. */
	virtual GLsizei varyingsCount()
	{
		return 7;
	}

	/* Each output goes to different buffer. The mapping between vertex stream outputs and transform feedback buffers is non-identity. */
	virtual const char** varyings()
	{
		static const char* vars[] = { "output4", "gl_NextBuffer", "output3", "gl_NextBuffer",
									  "output2", "gl_NextBuffer", "output1" };
		return vars;
	}

	/* The size of the transform feedback buffers should be enough to be able to capture exactly 10 vertices for each vertex stream. */
	virtual GLsizei bufferSize(GLint index)
	{
		(void)index;
		return 10 * sizeof(GLfloat);
	}

	/* Test case iterate function. Contains the actual test case logic. */
	IterateResult iterate()
	{
		const glw::Functions& gl = m_context.getRenderContext().getFunctions();

		// If GL 4.0 and ARB_transform_feedback3 are not supported then skip this
		// test case.
		if (!supportsTransformFeedback3())
		{
			throw tcu::NotSupportedError("Required transform_feedback3 extension is not supported");
		}

		// If GL 4.0 and ARB_gpu_shader5 are not supported then skip this
		// test case.
		if (!supportsGpuShader5())
		{
			throw tcu::NotSupportedError("Required gpu_shader5 extension is not supported");
		}

		// Call BeginTransformFeedback with mode POINTS.
		gl.beginTransformFeedback(GL_POINTS);

		// Start all queries, submit draw that results in feeding back exactly 8
		// points for all four vertex streams, then stop the queries.
		drawStreams(8, 8, 8, 8);

		// Call PauseTransformFeedback.
		gl.pauseTransformFeedback();

		// Use the basic checking mechanism to validate that the result of the
		// queries are all FALSE.
		verifyQueryResults(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

		// Start the queries, submit draw that would result in feeding back more
		// than 10 points for all four vertex streams if transform feedback wasn't
		// paused, then stop the queries.
		drawStreams(11, 11, 11, 11);

		// Use the basic checking mechanism to validate that the result of the
		// queries are all FALSE.
		verifyQueryResults(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

		// Call ResumeTransformFeedback.
		gl.resumeTransformFeedback();

		// Start the queries, submit draw that results in feeding back exactly 3
		// points only for vertex streams #1 and #3, then stop the queries.
		drawStreams(0, 3, 0, 3);

		// Call PauseTransformFeedback.
		gl.pauseTransformFeedback();

		// Use the basic checking mechanism to validate that the result of the
		// queries are all FALSE, except for the TRANSFORM_FEEDBACK_OVERFLOW
		// query, and the TRANSFORM_FEEDBACK_STREAM_OVERFLOW queries for
		// vertex streams #1 and #3, which should have a TRUE result.
		verifyQueryResults(GL_TRUE, GL_FALSE, GL_TRUE, GL_FALSE, GL_TRUE);

		// Call ResumeTransformFeedback.
		gl.resumeTransformFeedback();

		// Start the queries, submit draw that results in feeding back exactly 2
		// points only for vertex streams #0 and #2, then stop the queries.
		drawStreams(2, 0, 2, 0);

		// Call PauseTransformFeedback.
		gl.pauseTransformFeedback();

		// Use the basic checking mechanism to validate that the result of the
		// queries are all FALSE.
		verifyQueryResults(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

		// Call ResumeTransformFeedback.
		gl.resumeTransformFeedback();

		// Start the queries, submit draw that results in feeding back exactly 1
		// point for vertex streams #2 and #3, then stop the queries.
		drawStreams(0, 0, 1, 1);

		// Call EndTransformFeedback.
		gl.endTransformFeedback();

		// Use the basic checking mechanism to validate that the result of the
		// queries are all FALSE, except for the TRANSFORM_FEEDBACK_OVERFLOW
		// query, and the TRANSFORM_FEEDBACK_STREAM_OVERFLOW queries for
		// vertex streams #2 and #3, which should have a TRUE result.
		verifyQueryResults(GL_TRUE, GL_FALSE, GL_FALSE, GL_TRUE, GL_TRUE);

		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

		return STOP;
	}
};

/*
 Test case #6 - Advanced multiple streams, multiple buffers per stream.
 */
class TransformFeedbackOverflowQueryMultipleStreamsMultipleBufferPerStream
	: public TransformFeedbackOverflowQueryMultipleStreamsBase
{
public:
	TransformFeedbackOverflowQueryMultipleStreamsMultipleBufferPerStream(deqp::Context& context,
																		 TransformFeedbackOverflowQueryTests::API api,
																		 const char*							  name)
		: TransformFeedbackOverflowQueryMultipleStreamsBase(context, api, name,
															"Advanced multiple streams, multiple buffers per stream.")
	{
	}

	/* Geometry shader to use for transform feedback. */
	virtual const char* transformFeedbackGeometryShader()
	{
		return "#version 150 core\n"
			   "#extension GL_ARB_gpu_shader5 : require\n"
			   "layout(points) in;\n"
			   "layout(points, max_vertices = 1) out;\n"
			   "layout(stream=0) out float output1;\n"
			   "layout(stream=0) out float output2;\n"
			   "layout(stream=1) out float output3;\n"
			   "layout(stream=1) out float output4;\n"
			   "uniform uint stream;\n"
			   "void main() {\n"
			   "    if (stream == 0) {\n"
			   "        output1 = 1.0;\n"
			   "        output2 = 2.0;\n"
			   "        EmitStreamVertex(0);\n"
			   "        EndStreamPrimitive(0);\n"
			   "    }\n"
			   "    if (stream == 1) {\n"
			   "        output3 = 3.0;\n"
			   "        output4 = 4.0;\n"
			   "        EmitStreamVertex(1);\n"
			   "        EndStreamPrimitive(1);\n"
			   "    }\n"
			   "}";
	}

	/* Together with the separators there are a total of 7 varyings. */
	virtual GLsizei varyingsCount()
	{
		return 7;
	}

	/* Vertex stream #0 is captured by transform feedback buffers #1 and #2, while
	 vertex stream #1 is captured by transform feedback buffers #3 and #0. */
	virtual const char** varyings()
	{
		static const char* vars[] = { "output4", "gl_NextBuffer", "output1", "gl_NextBuffer",
									  "output2", "gl_NextBuffer", "output3" };
		return vars;
	}

	/* Transform feedback buffers #0 and #1 should be able to capture exactly 10 vertices, while
	 transform feedback buffers #2 and #3 should be able to capture exactly 20 vertices. */
	virtual GLsizei bufferSize(GLint index)
	{
		if (index < 2)
		{
			return 10 * sizeof(GLfloat);
		}
		else
		{
			return 20 * sizeof(GLfloat);
		}
	}

	/* Test case iterate function. Contains the actual test case logic. */
	IterateResult iterate()
	{
		const glw::Functions& gl = m_context.getRenderContext().getFunctions();

		// If GL 4.0 and ARB_transform_feedback3 are not supported then skip this
		// test case.
		if (!supportsTransformFeedback3())
		{
			throw tcu::NotSupportedError("Required transform_feedback3 extension is not supported");
		}

		// If GL 4.0 and ARB_gpu_shader5 are not supported then skip this
		// test case.
		if (!supportsGpuShader5())
		{
			throw tcu::NotSupportedError("Required gpu_shader5 extension is not supported");
		}

		// Call BeginTransformFeedback with mode POINTS.
		gl.beginTransformFeedback(GL_POINTS);

		// Start all queries, submit draw that results in feeding back exactly 8
		// points to both vertex streams, then stop the queries.
		drawStreams(8, 8);

		// Call PauseTransformFeedback.
		gl.pauseTransformFeedback();

		// Use the basic checking mechanism to validate that the result of the
		// queries are all FALSE.
		verifyQueryResults(GL_FALSE, GL_FALSE, GL_FALSE);

		// Start the queries, submit draw that would result in feeding back more
		// than 10 points for both vertex streams if transform feedback wasn't
		// paused, then stop the queries.
		drawStreams(11, 11);

		// Use the basic checking mechanism to validate that the result of the
		// queries are all FALSE.
		verifyQueryResults(GL_FALSE, GL_FALSE, GL_FALSE);

		// Call ResumeTransformFeedback.
		gl.resumeTransformFeedback();

		// Start the queries, submit draw that results in feeding back exactly 1
		// point for vertex stream #0 and exactly 3 points for vertex stream #1,
		// then stop the queries.
		drawStreams(1, 3);

		// Call PauseTransformFeedback.
		gl.pauseTransformFeedback();

		// Use the basic checking mechanism to validate that the result of the
		// queries are all FALSE, except for the TRANSFORM_FEEDBACK_OVERFLOW
		// query, and the TRANSFORM_FEEDBACK_STREAM_OVERFLOW query for vertex
		// stream #1, which should have a TRUE result.
		verifyQueryResults(GL_TRUE, GL_FALSE, GL_TRUE);

		// Call ResumeTransformFeedback.
		gl.resumeTransformFeedback();

		// Start the queries, submit draw that results in feeding back exactly 1
		// point only for vertex stream #0, then stop the queries.
		drawStreams(1, 0);

		// Call PauseTransformFeedback.
		gl.pauseTransformFeedback();

		// Use the basic checking mechanism to validate that the result of the
		// queries are all FALSE.
		verifyQueryResults(GL_FALSE, GL_FALSE, GL_FALSE);

		// Call ResumeTransformFeedback.
		gl.resumeTransformFeedback();

		// Start the queries, submit draw that results in feeding back exactly 1
		// point for vertex streams #0 and #1, then stop the queries.
		drawStreams(1, 1);

		// Call EndTransformFeedback.
		gl.endTransformFeedback();

		// Use the basic checking mechanism to validate that the result of the
		// queries are all FALSE, except for the TRANSFORM_FEEDBACK_OVERFLOW
		// query, and the TRANSFORM_FEEDBACK_STREAM_OVERFLOW queries for
		// vertex streams #0 and #1, which should have a TRUE result.
		verifyQueryResults(GL_TRUE, GL_TRUE, GL_TRUE);

		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

		return STOP;
	}
};

const char* apiToTestName(TransformFeedbackOverflowQueryTests::API api)
{
	switch (api)
	{
	case TransformFeedbackOverflowQueryTests::API_GL_ARB_transform_feedback_overflow_query:
		return "transform_feedback_overflow_query_ARB";
	}
	DE_ASSERT(0);
	return "";
}

/** Constructor.
 *
 *  @param context Rendering context.
 *  @param api     API to test (core vs ARB extension)
 **/
TransformFeedbackOverflowQueryTests::TransformFeedbackOverflowQueryTests(deqp::Context& context, API api)
	: TestCaseGroup(context, apiToTestName(api), "Verifies \"transform_feedback_overflow_query\" functionality")
	, m_api(api)
{
	/* Left blank on purpose */
}

/** Destructor.
 *
 **/
TransformFeedbackOverflowQueryTests::~TransformFeedbackOverflowQueryTests()
{
}

/** Initializes the texture_barrier test group.
 *
 **/
void TransformFeedbackOverflowQueryTests::init(void)
{
	addChild(new TransformFeedbackOverflowQueryImplDepState(m_context, m_api, "implementation-dependent-state"));
	addChild(new TransformFeedbackOverflowQueryDefaultState(m_context, m_api, "default-context-state"));
	addChild(new TransformFeedbackOverflowQueryStateUpdate(m_context, m_api, "context-state-update"));
	addChild(new TransformFeedbackOverflowQueryErrorInvalidIndex(m_context, m_api, "error-invalid-index"));
	addChild(new TransformFeedbackOverflowQueryErrorAlreadyActive(m_context, m_api, "error-already-active"));
	addChild(new TransformFeedbackOverflowQueryErrorIncompatibleTarget(m_context, m_api, "error-incompatible-target"));
	addChild(new TransformFeedbackOverflowQueryErrorNoActiveQuery(m_context, m_api, "error-no-active-query"));
	addChild(new TransformFeedbackOverflowQueryBasicSingleStreamInterleavedAttribs(
		m_context, m_api, "basic-single-stream-interleaved-attribs"));
	addChild(new TransformFeedbackOverflowQueryBasicSingleStreamSeparateAttribs(
		m_context, m_api, "basic-single-stream-separate-attribs"));
	addChild(new TransformFeedbackOverflowQueryAdvancedSingleStreamInterleavedAttribs(
		m_context, m_api, "advanced-single-stream-interleaved-attribs"));
	addChild(new TransformFeedbackOverflowQueryAdvancedSingleStreamSeparateAttribs(
		m_context, m_api, "advanced-single-stream-separate-attribs"));
	addChild(new TransformFeedbackOverflowQueryMultipleStreamsOneBufferPerStream(
		m_context, m_api, "multiple-streams-one-buffer-per-stream"));
	addChild(new TransformFeedbackOverflowQueryMultipleStreamsMultipleBufferPerStream(
		m_context, m_api, "multiple-streams-multiple-buffers-per-stream"));
}
} /* glcts namespace */
