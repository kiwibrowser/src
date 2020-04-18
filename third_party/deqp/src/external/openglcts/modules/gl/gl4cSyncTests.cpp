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
 * \file  gl4cSyncTests.cpp
 * \brief Declares test classes for synchronization functionality.
 */ /*-------------------------------------------------------------------*/

#include "gl4cSyncTests.hpp"

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

/* Timeout of the test in nanoseconds. */
#define TEST_SYNC_WAIT_TIMEOUT 16000000000

namespace gl4cts
{
namespace Sync
{
/****************************************** Tests Group ***********************************************/

/** @brief Sync Tests Group constructor.
 *
 *  @param [in] context     OpenGL context.
 */
Tests::Tests(deqp::Context& context) : TestCaseGroup(context, "sync", "Sync Tests Suite")
{
}

/** @brief Sync Tests initializer. */
void Tests::init()
{
	addChild(new Sync::FlushCommandsTest(m_context));
}

/*************************************** Flush Commands Test *******************************************/

/** @brief Sync Flush Commands Test constructor.
 *
 *  @param [in] context     OpenGL context.
 */
FlushCommandsTest::FlushCommandsTest(deqp::Context& context)
	: deqp::TestCase(context, "flush_commands", "Sync Flush Commands Test")
{
	/* Intentionally left blank. */
}

/** @brief Iterate Sync Flush Commands Test cases.
 *
 *  @return Iteration result.
 */
tcu::TestNode::IterateResult FlushCommandsTest::iterate()
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
	bool is_ok		= false;
	bool is_error   = false;
	bool is_timeout = false;

	/* Test constants. */
	static const glw::GLuint reference[2]   = { 3, 1415927 };
	static const glw::GLuint reference_size = sizeof(reference);

	/* Test objects. */
	glw::GLuint buffer_src = 0;
	glw::GLuint buffer_dst = 0;
	glw::GLsync sync	   = 0;
	glw::GLenum error	  = GL_NO_ERROR;

	try
	{
		/* Prepare buffers. */
		gl.createBuffers(1, &buffer_src);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateBuffers have failed");
		gl.namedBufferData(buffer_src, reference_size, reference, GL_STATIC_COPY);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glNamedBufferData have failed");

		gl.createBuffers(1, &buffer_dst);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateBuffers have failed");
		gl.namedBufferStorage(buffer_dst, reference_size, NULL,
							  GL_MAP_READ_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glNamedBufferStorage have failed");

		/* Map perisistently buffer range. */
		glw::GLuint* data_dst = (glw::GLuint*)gl.mapNamedBufferRange(
			buffer_dst, 0, reference_size, GL_MAP_READ_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glMapNamedBufferRange have failed");

		/* Copy data from source to destination buffer */
		gl.copyNamedBufferSubData(buffer_src, buffer_dst, 0, 0, reference_size);

		if (GL_NO_ERROR != (error = gl.getError()))
		{
			gl.unmapNamedBuffer(buffer_dst);

			GLU_EXPECT_NO_ERROR(error, "glCopyNamedBufferSubData have failed");
		}

		/* Create fence sync object. */
		sync = gl.fenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);

		if (GL_NO_ERROR == (error = gl.getError()))
		{
			/* Wait until done. */
			glw::GLenum wait_result = gl.clientWaitSync(sync, GL_SYNC_FLUSH_COMMANDS_BIT, TEST_SYNC_WAIT_TIMEOUT);

			/* Check for error. */
			if (GL_NO_ERROR == (error = gl.getError()))
			{
				/* Check for timeout. */
				if (GL_TIMEOUT_EXPIRED == wait_result)
				{
					m_context.getTestContext().getLog()
						<< tcu::TestLog::Message
						<< "ClientWaitSync with SYNC_FLUSH_COMMANDS_BIT flag has returned TIMEOUT_EXPIRED after "
						<< TEST_SYNC_WAIT_TIMEOUT << " nanoseconds. Potentially test may not be done in finite time "
													 "which is expected (OpenGL 4.5 Core Profile, Chapter 4.1.2)."
						<< " However this cannot be proven in finite time. Test timeouts." << tcu::TestLog::EndMessage;

					is_timeout = true;
				} /* Check for proper wait result. */
				else if ((GL_CONDITION_SATISFIED == wait_result) || (GL_ALREADY_SIGNALED == wait_result))
				{
					/* Compare destination buffer data with reference. */
					if ((reference[0] == data_dst[0]) || (reference[1] == data_dst[1]))
					{
						is_ok = true;
					}
					else
					{
						m_context.getTestContext().getLog() << tcu::TestLog::Message << "Result data [" << data_dst[0]
															<< ", " << data_dst[1] << "is not equal to the reference ["
															<< reference[0] << ", " << reference[1] << "]. Tests fails."
															<< tcu::TestLog::EndMessage;
					}
				}
			}
			else
			{
				m_context.getTestContext().getLog()
					<< tcu::TestLog::Message << "ClientWaitSync unexpectedly generated error "
					<< glu::getErrorStr(error) << ". Tests fails." << tcu::TestLog::EndMessage;
			}
		}

		/* Unmapping. */
		gl.unmapNamedBuffer(buffer_dst);
		GLU_EXPECT_NO_ERROR(error, "glUnmapNamedBuffer have failed");
	}
	catch (...)
	{
		is_ok	= false;
		is_error = true;
	}

	/* Cleanup. */
	if (buffer_src)
	{
		gl.deleteBuffers(1, &buffer_src);
	}

	if (buffer_dst)
	{
		gl.deleteBuffers(1, &buffer_dst);
	}

	if (sync)
	{
		gl.deleteSync(sync);
	}

	/* Result's setup. */
	if (is_timeout)
	{
		m_testCtx.setTestResult(
			QP_TEST_RESULT_TIMEOUT,
			"Timeout (Potentially, ClientWaitSync with SYNC_FLUSH_COMMANDS does not return in finite time).");
	}
	else
	{
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
	}

	return STOP;
}
} /* Sync namespace */
} /* gl4cts namespace */
