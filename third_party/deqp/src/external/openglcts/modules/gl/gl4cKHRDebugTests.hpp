#ifndef _GL4CKHRDEBUGTESTS_HPP
#define _GL4CKHRDEBUGTESTS_HPP
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
 * \file  gl4cKHRDebugTests.hpp
 * \brief Declares test classes for "KHR Debug" functionality.
 */ /*-------------------------------------------------------------------*/

#include "glcTestCase.hpp"
#include "glwDefs.hpp"

#include "deThreadLocal.hpp"

namespace glu
{
class RenderContext;
}

namespace glw
{
class Functions;
}

namespace gl4cts
{
namespace KHRDebug
{
/** Base of all test cases.
 * Manages rendering context
 **/
class TestBase
{
public:
	/* Public methods */
	TestBase(deqp::Context& context, bool is_debug);
	virtual ~TestBase();

protected:
	/* Protected methods */
	void init();
	void done();

	/* Protected fields */
	const glw::Functions* m_gl;
	const bool			  m_is_debug;
	glu::RenderContext*   m_rc;

private:
	/* Private methods */
	void initDebug();
	void initNonDebug();

	/* Private fields */
	deqp::Context&		m_test_base_context;
	glu::RenderContext* m_orig_rc;
};

/** Implementation of test APIErrors. Description follows:
 *
 * This test verifies that errors are generated as expected.
 *
 * This test should be executed for DEBUG and NON-DEBUG contexts.
 *
 * DebugMessageControl function should generate:
 * - INVALID_ENUM when <source> is invalid;
 * - INVALID_ENUM when <type> is invalid;
 * - INVALID_ENUM when <severity> is invalid;
 * - INVALID_VALUE when <count> is negative;
 * - INVALID_OPERATION when <count> is not zero and <source> is DONT_CARE;
 * - INVALID_OPERATION when <count> is not zero and <type> is DONT_CARE;
 * - INVALID_OPERATION when <count> is not zero and <severity> is not
 * DONT_CARE.
 *
 * GetDebugMessageLog function should generate:
 * - INVALID_VALUE when <bufSize> is negative and messageLog is not NULL.
 *
 * DebugMessageInsert function should generate:
 * - INVALID_ENUM when <source> is not DEBUG_SOURCE_APPLICATION or
 * DEBUG_SOURCE_THIRD_PARTY;
 * - INVALID_ENUM when <type> is invalid;
 * - INVALID_ENUM when <severity> is invalid;
 * - INVALID_VALUE when length of string <buf> is not less than
 * MAX_DEBUG_MESSAGE_LENGTH.
 *
 * PushDebugGroup function should generate:
 * - INVALID_ENUM when <source> is not DEBUG_SOURCE_APPLICATION or
 * DEBUG_SOURCE_THIRD_PARTY;
 * - INVALID_VALUE when length of string <message> is not less than
 * MAX_DEBUG_MESSAGE_LENGTH;
 * - STACK_OVERFLOW when stack contains MAX_DEBUG_GROUP_STACK_DEPTH entries.
 *
 * PopDebugGroup function should generate:
 * - STACK_UNDERFLOW when stack contains no entries.
 *
 * ObjectLabel function should generate:
 * - INVALID_ENUM when <identifier> is invalid;
 * - INVALID_VALUE when if <name> is not valid object name of type specified by
 * <identifier>;
 * - INVALID_VALUE when length of string <label> is not less than
 * MAX_LABEL_LENGTH.
 *
 * GetObjectLabel function should generate:
 * - INVALID_ENUM when <identifier> is invalid;
 * - INVALID_VALUE when if <name> is not valid object name of type specified by
 * <identifier>;
 * - INVALID_VALUE when <bufSize> is negative.
 *
 * ObjectPtrLabel function should generate:
 * - INVALID_VALUE when <ptr> is not the name of sync object;
 * - INVALID_VALUE when length of string <label> is not less than
 * MAX_LABEL_LENGTH.
 *
 * GetObjectPtrLabel function should generate:
 * - INVALID_VALUE when <ptr> is not the name of sync object;
 * - INVALID_VALUE when <bufSize> is negative.
 *
 * GetPointerv function should generate:
 * - INVALID_ENUM when <pname> is invalid.
 **/
class APIErrorsTest : public deqp::TestCase, public TestBase
{
public:
	/* Public methods */
	APIErrorsTest(deqp::Context& context, bool is_debug, const glw::GLchar* name);

	virtual ~APIErrorsTest()
	{
	}

	/* Public methods inherited from TestCase */
	virtual tcu::TestNode::IterateResult iterate(void);
};

/** Implementation of test Labels. Description follows:
 *
 * This test verifies that it is possible to assign and query labels.
 *
 * This test should be executed for DEBUG and NON-DEBUG contexts.
 *
 * For each valid object type:
 * - create new object;
 * - query label; It is expected that result will be an empty string and length
 * will be zero;
 * - assign label to object;
 * - query label; It is expected that result will be equal to the provided
 * label and length will be correct;
 * - query length only; Correct value is expected;
 * - query label with <bufSize> less than actual length of label; It is
 * expected that only <bufSize> characters will be stored in buffer (including
 * NULL);
 * - query label with <bufSize> equal zero; It is expected that buffer contents
 * will not be modified;
 * - assign empty string as label to object;
 * - query label, it is expected that result will be an empty string and length
 * will be zero;
 * - assign NULL as label to object;
 * - query label, it is expected that result will be an empty string and length
 * will be zero;
 * - delete object.
 **/
class LabelsTest : public deqp::TestCase, public TestBase
{
public:
	/* Public methods */
	LabelsTest(deqp::Context& context, bool is_debug, const glw::GLchar* name);

	virtual ~LabelsTest()
	{
	}

	/* Public methods inherited from TestCase */
	virtual tcu::TestNode::IterateResult iterate(void);

private:
	/* Private routines */
	static glw::GLuint createBuffer(const glw::Functions* gl, const glu::RenderContext* rc);
	static glw::GLuint createFramebuffer(const glw::Functions* gl, const glu::RenderContext* rc);
	static glw::GLuint createProgram(const glw::Functions* gl, const glu::RenderContext* rc);
	static glw::GLuint createProgramPipeline(const glw::Functions* gl, const glu::RenderContext* rc);
	static glw::GLuint createQuery(const glw::Functions* gl, const glu::RenderContext* rc);
	static glw::GLuint createRenderbuffer(const glw::Functions* gl, const glu::RenderContext* rc);
	static glw::GLuint createSampler(const glw::Functions* gl, const glu::RenderContext* rc);
	static glw::GLuint createShader(const glw::Functions* gl, const glu::RenderContext* rc);
	static glw::GLuint createTexture(const glw::Functions* gl, const glu::RenderContext* rc);
	static glw::GLuint createTransformFeedback(const glw::Functions* gl, const glu::RenderContext* rc);
	static glw::GLuint createVertexArray(const glw::Functions* gl, const glu::RenderContext* rc);

	static glw::GLvoid deleteBuffer(const glw::Functions* gl, glw::GLuint id);
	static glw::GLvoid deleteFramebuffer(const glw::Functions* gl, glw::GLuint id);
	static glw::GLvoid deleteProgram(const glw::Functions* gl, glw::GLuint id);
	static glw::GLvoid deleteProgramPipeline(const glw::Functions* gl, glw::GLuint id);
	static glw::GLvoid deleteQuery(const glw::Functions* gl, glw::GLuint id);
	static glw::GLvoid deleteRenderbuffer(const glw::Functions* gl, glw::GLuint id);
	static glw::GLvoid deleteSampler(const glw::Functions* gl, glw::GLuint id);
	static glw::GLvoid deleteShader(const glw::Functions* gl, glw::GLuint id);
	static glw::GLvoid deleteTexture(const glw::Functions* gl, glw::GLuint id);
	static glw::GLvoid deleteTransformFeedback(const glw::Functions* gl, glw::GLuint id);
	static glw::GLvoid deleteVertexArray(const glw::Functions* gl, glw::GLuint id);
};

/** Implementation of test ReceiveingMessages. Description follows:
 *
 * This test verifies that it is possible to receive messages.
 *
 * This test should be executed for DEBUG contexts only.
 *
 * Callback used during the test should make use of <userParam> to inform the
 * test about any calls.
 *
 * Steps:
 * - verify that the state of DEBUG_OUTPUT is enabled as it should be by
 * default;
 * - verify that state of DEBUG_CALLBACK_FUNCTION and
 * DEBUG_CALLBACK_USER_PARAM are NULL;
 *
 * - insert a message with DebugMessageInsert;
 * - inspect message log to check if the message is reported;
 * - inspect message log again, there should be no messages;
 *
 * - disable DEBUG_OUTPUT;
 * - insert a message with DebugMessageInsert;
 * - inspect message log again, there should be no messages;
 *
 * - enable DEBUG_OUTPUT;
 * - register debug message callback with DebugMessageCallback;
 * - verify that the state of DEBUG_CALLBACK_FUNCTION and
 * DEBUG_CALLBACK_USER_PARAM are correct;
 * - insert a message with DebugMessageInsert;
 * - it is expected that debug message callback will be executed for
 * the message;
 * - inspect message log to check there are no messages;
 *
 * - disable DEBUG_OUTPUT;
 * - insert a message with DebugMessageInsert;
 * - debug message callback should not be called;
 * - inspect message log to check there are no messages;
 *
 * - enable DEBUG_OUTPUT;
 * - execute DebugMessageControl with <type> DEBUG_TYPE_ERROR, <severity>
 * DEBUG_SEVERITY_HIGH and <enabled> FALSE;
 * - insert a message with DebugMessageInsert, set <type> to DEBUG_TYPE_ERROR
 * and <severity> DEBUG_SEVERITY_MEDIUM;
 * - insert a message with DebugMessageInsert, set <type> to DEBUG_TYPE_OTHER
 * and <severity> DEBUG_SEVERITY_HIGH;
 * - insert a message with DebugMessageInsert, set <type> to DEBUG_TYPE_OTHER
 * and <severity> DEBUG_SEVERITY_LOW;
 * - debug message callback should not be called;
 * - inspect message log to check there are no messages;
 *
 * - set NULL as debug message callback;
 * - verify that state of DEBUG_CALLBACK_FUNCTION and
 * DEBUG_CALLBACK_USER_PARAM are NULL;
 * - insert a message with DebugMessageInsert, set <type> to DEBUG_TYPE_ERROR
 * and <severity> DEBUG_SEVERITY_MEDIUM;
 * - insert a message with DebugMessageInsert, set <type> to DEBUG_TYPE_OTHER
 * and <severity> DEBUG_SEVERITY_HIGH;
 * - insert a message with DebugMessageInsert, set <type> to DEBUG_TYPE_OTHER
 * and <severity> DEBUG_SEVERITY_LOW;
 * - inspect message log to check there are no messages;
 *
 * - execute DebugMessageControl to enable messages of <type> DEBUG_TYPE_ERROR
 * and <severity> DEBUG_SEVERITY_HIGH.
 *
 * - insert MAX_DEBUG_LOGGED_MESSAGES + 1 unique messages with
 * DebugMessageInsert;
 * - check state of DEBUG_LOGGED_MESSAGES; It is expected that
 * MAX_DEBUG_LOGGED_MESSAGES will be reported;
 *
 * If MAX_DEBUG_LOGGED_MESSAGES is greater than 1:
 * - inspect first half of the message log by specifying proper <count>; Verify
 * that messages are reported in order from the oldest to the newest; Check
 * that <count> messages were stored into provided buffers;
 * - check state of DEBUG_LOGGED_MESSAGES; It is expected that <count> messages
 * were removed from log;
 * - inspect rest of the message log with <bufSize> too small to held last
 * message; Verify that messages are reported in order from the oldest to the
 * newest; Verify that maximum <bufSize> characters were written to
 * <messageLog>;
 * - check state of DEBUG_LOGGED_MESSAGES; It is expected that one message is
 * available;
 * - fetch the message and verify it is the newest one;
 **/
class ReceiveingMessagesTest : public deqp::TestCase, public TestBase
{
public:
	/* Public methods */
	ReceiveingMessagesTest(deqp::Context& context);

	virtual ~ReceiveingMessagesTest()
	{
	}

	/* Public methods inherited from TestCase */
	virtual tcu::TestNode::IterateResult iterate(void);

private:
	/* Private routines */
	static void GLW_APIENTRY debug_proc(glw::GLenum source, glw::GLenum type, glw::GLuint id, glw::GLenum severity,
										glw::GLsizei length, const glw::GLchar* message, const void* info);

	void inspectCallbackCounter(glw::GLuint& callback_counter, glw::GLuint expected_number_of_messages) const;

	void inspectDebugState(glw::GLboolean expected_state, glw::GLDEBUGPROC expected_callback,
						   glw::GLvoid* expected_user_info) const;

	void inspectMessageLog(glw::GLuint expected_number_of_messages) const;
};

/** Implementation of test Groups. Description follows:
 *
 * This test verifies that debug message groups can be used to control which
 * messages are being generated.
 *
 * This test should be executed for DEBUG contexts only.
 *
 * Steps:
 * - check state of DEBUG_GROUP_STACK_DEPTH; It should be 1;
 *
 * - insert message with <type> DEBUG_TYPE_ERROR;
 * - inspect message log to check if the message is reported;
 * - insert message with <type> DEBUG_TYPE_OTHER;
 * - inspect message log to check if the message is reported;
 *
 * - push debug group with unique <id> and <message>;
 * - inspect message log to check if the message about push is reported;
 * - disable messages with <type> DEBUG_TYPE_ERROR;
 * - insert message with <type> DEBUG_TYPE_ERROR;
 * - inspect message log to check there are no messages;
 * - insert message with <type> DEBUG_TYPE_OTHER;
 * - inspect message log to check if the message is reported;
 *
 * - check state of DEBUG_GROUP_STACK_DEPTH; It should be 2;
 *
 * - push debug group with unique <id> and <message>;
 * - inspect message log to check if the message about push is reported;
 * - disable messages with <type> DEBUG_TYPE_OTHER;
 * - insert message with <type> DEBUG_TYPE_ERROR;
 * - inspect message log to check there are no messages;
 * - insert message with <type> DEBUG_TYPE_OTHER;
 * - inspect message log to check there are no messages;
 *
 * - check state of DEBUG_GROUP_STACK_DEPTH; It should be 3;
 *
 * - pop debug group;
 * - inspect message log to check if the message about pop is reported and
 * corresponds with the second push;
 * - insert message with <type> DEBUG_TYPE_ERROR;
 * - inspect message log to check there are no messages;
 * - insert message with <type> DEBUG_TYPE_OTHER;
 * - inspect message log to check if the message is reported;
 *
 * - check state of DEBUG_GROUP_STACK_DEPTH; It should be 2;
 *
 * - pop debug group;
 * - inspect message log to check if the message about pop is reported and
 * corresponds with the first push;
 * - insert message with <type> DEBUG_TYPE_ERROR;
 * - inspect message log to check if the message is reported;
 * - insert message with <type> DEBUG_TYPE_OTHER;
 * - inspect message log to check if the message is reported;
 *
 * - check state of DEBUG_GROUP_STACK_DEPTH; It should be 1.
 **/
class GroupsTest : public deqp::TestCase, public TestBase
{
public:
	/* Public methods */
	GroupsTest(deqp::Context& context);

	virtual ~GroupsTest()
	{
	}

	/* Public methods inherited from TestCase */
	virtual tcu::TestNode::IterateResult iterate(void);

private:
	/* Private routines */
	void inspectGroupStack(glw::GLuint expected_depth) const;
	void inspectMessageLog(glw::GLenum expected_source, glw::GLenum expected_type, glw::GLuint expected_id,
						   glw::GLenum expected_severity, glw::GLsizei expected_length,
						   const glw::GLchar* expected_label) const;
	void verifyEmptyLog() const;
};

/** Implementation of test SynchronousCalls. Description follows:
 *
 * This test verifies that implementation execute debug message callback in
 * synchronous way when DEBUG_OUTPUT_SYNCHRONOUS is enabled.
 *
 * This test should be executed for DEBUG contexts only.
 *
 * Steps:
 * - create an instance of the following structure
 * - set callback_executed to 0;
 * - enable DEBUG_OUTPUT_SYNCHRONOUS;
 * - register debug message callback with DebugMessageCallback; Provide the
 * instance of UserParam structure as <userParam>; Routine should do the
 * following:
 *   * set callback_executed to 1;
 *   * store thread_id;
 * - insert a message with DebugMessageInsert;
 * - check if:
 *   * callback_executed is set to 1;
 *   * the message is recorded by the current thread.
 * - reset userParam object;
 * - execute BindBufferBase with GL_ARRAY_BUFFER <target>, GL_INVALID_ENUM
 * error should be generated;
 * - test pass if:
 *   * callback_executed is set to 0 - implementation does not send messages;
 *   * callback_executed is set to 1 and thread_id is the same
 *   as "test" thread - implementation sent message to proper thread;
 **/
class SynchronousCallsTest : public deqp::TestCase, public TestBase
{
public:
	/* Public methods */
	SynchronousCallsTest(deqp::Context& context);
	~SynchronousCallsTest(void);

	/* Public methods inherited from TestCase */
	virtual tcu::TestNode::IterateResult iterate(void);

private:
	/* Private routines */
	static void GLW_APIENTRY debug_proc(glw::GLenum source, glw::GLenum type, glw::GLuint id, glw::GLenum severity,
										glw::GLsizei length, const glw::GLchar* message, const void* info);

	de::ThreadLocal m_tls;
	deUint32		m_uid;
};
} /* KHRDebug */

/** Group class for khr debug conformance tests */
class KHRDebugTests : public deqp::TestCaseGroup
{
public:
	/* Public methods */
	KHRDebugTests(deqp::Context& context);

	virtual ~KHRDebugTests(void)
	{
	}

	virtual void init(void);

private:
	/* Private methods */
	KHRDebugTests(const KHRDebugTests& other);
	KHRDebugTests& operator=(const KHRDebugTests& other);
};

} /* gl4cts */

#endif // _GL4CKHRDEBUGTESTS_HPP
