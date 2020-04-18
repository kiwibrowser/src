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
 * \file  gl4cKHRDebugTests.cpp
 * \brief Implements conformance tests for "KHR Debug" functionality.
 */ /*-------------------------------------------------------------------*/

#include "gl4cKHRDebugTests.hpp"

#include "gluPlatform.hpp"
#include "gluRenderConfig.hpp"
#include "gluRenderContext.hpp"
#include "gluStrUtil.hpp"
#include "glwEnums.hpp"
#include "glwFunctions.hpp"
#include "tcuCommandLine.hpp"
#include "tcuTestLog.hpp"
//
//#include <string>

#define DEBUG_ENBALE_MESSAGE_CALLBACK 0

#if DEBUG_ENBALE_MESSAGE_CALLBACK
//#include <iomanip>
#endif /* DEBUG_ENBALE_MESSAGE_CALLBACK */

using namespace glw;

namespace gl4cts
{
namespace KHRDebug
{
/** Macro, verifies generated error, logs error message and throws failure
 *
 * @param expected_error Expected error value
 * @param error_message  Message logged if generated error is not the expected one
 **/
#define CHECK_ERROR(expected_error, error_message)                                                      \
	{                                                                                                   \
		GLenum generated_error = m_gl->getError();                                                      \
                                                                                                        \
		if (expected_error != generated_error)                                                          \
		{                                                                                               \
			m_context.getTestContext().getLog()                                                         \
				<< tcu::TestLog::Message << "File: " << __FILE__ << ", line: " << __LINE__              \
				<< ". Got wrong error: " << glu::getErrorStr(generated_error)                           \
				<< ", expected: " << glu::getErrorStr(expected_error) << ", message: " << error_message \
				<< tcu::TestLog::EndMessage;                                                            \
			TCU_FAIL("Invalid error generated");                                                        \
		}                                                                                               \
	}

/** Pop all groups from stack
 *
 * @param gl GL functions
 **/
void cleanGroupStack(const Functions* gl)
{
	while (1)
	{
		gl->popDebugGroup();

		const GLenum err = gl->getError();

		if (GL_STACK_UNDERFLOW == err)
		{
			break;
		}

		GLU_EXPECT_NO_ERROR(err, "PopDebugGroup");
	}
}

/** Extracts all messages from log
 *
 * @param gl GL functions
 **/
void cleanMessageLog(const Functions* gl)
{
	static const GLuint count = 16;

	while (1)
	{
		GLuint ret = gl->getDebugMessageLog(count /* count */, 0 /* bufSize */, 0 /* sources */, 0 /* types */,
											0 /* ids */, 0 /* severities */, 0 /* lengths */, 0 /* messageLog */);

		GLU_EXPECT_NO_ERROR(gl->getError(), "GetDebugMessageLog");

		if (0 == ret)
		{
			break;
		}
	}
}

/** Fill stack of groups
 *
 * @param gl GL functions
 **/
void fillGroupStack(const Functions* gl)
{
	static const GLchar  message[] = "Foo";
	static const GLsizei length	= (GLsizei)(sizeof(message) / sizeof(message[0]));

	while (1)
	{
		gl->pushDebugGroup(GL_DEBUG_SOURCE_APPLICATION /* source */, 1 /* id */, length /* length */,
						   message /* message */);

		const GLenum err = gl->getError();

		if (GL_STACK_OVERFLOW == err)
		{
			break;
		}

		GLU_EXPECT_NO_ERROR(err, "PopDebugGroup");
	}
}

/** Constructor
 * Creates and set as current new context that should be used by test.
 *
 * @param context  Test context
 * @param is_debug Selects if debug or non-debug context should be created
 **/
TestBase::TestBase(deqp::Context& context, bool is_debug)
	: m_gl(0), m_is_debug(is_debug), m_rc(0), m_test_base_context(context), m_orig_rc(0)
{
	/* Nothing to be done here */
}

/** Destructor
 * Destroys context used by test and set original context as current
 **/
TestBase::~TestBase()
{
	if (0 != m_rc)
	{
		done();
	}
}

/** Initialize rendering context
 **/
void TestBase::init()
{
	if (true == m_is_debug)
	{
		initDebug();
	}
	else
	{
		initNonDebug();
	}

	m_orig_rc = &m_test_base_context.getRenderContext();
	m_test_base_context.setRenderContext(m_rc);

	/* Get functions */
	m_gl = &m_rc->getFunctions();
}

/** Prepares debug context
 **/
void TestBase::initDebug()
{
	tcu::Platform&	platform = m_test_base_context.getTestContext().getPlatform();
	glu::RenderConfig renderCfg(
		glu::ContextType(m_test_base_context.getRenderContext().getType().getAPI(), glu::CONTEXT_DEBUG));

	const tcu::CommandLine& commandLine = m_test_base_context.getTestContext().getCommandLine();
	parseRenderConfig(&renderCfg, commandLine);

	if (commandLine.getSurfaceType() == tcu::SURFACETYPE_WINDOW)
	{
		renderCfg.surfaceType = glu::RenderConfig::SURFACETYPE_OFFSCREEN_GENERIC;
	}
	else
	{
		throw tcu::NotSupportedError("Test not supported in non-windowed context");
	}

	m_rc = createRenderContext(platform, commandLine, renderCfg);
	m_rc->makeCurrent();
}

/** Prepares non-debug context
 **/
void TestBase::initNonDebug()
{
	tcu::Platform&	platform = m_test_base_context.getTestContext().getPlatform();
	glu::RenderConfig renderCfg(
		glu::ContextType(m_test_base_context.getRenderContext().getType().getAPI(), glu::ContextFlags(0)));

	const tcu::CommandLine& commandLine = m_test_base_context.getTestContext().getCommandLine();
	parseRenderConfig(&renderCfg, commandLine);

	if (commandLine.getSurfaceType() == tcu::SURFACETYPE_WINDOW)
	{
		renderCfg.surfaceType = glu::RenderConfig::SURFACETYPE_OFFSCREEN_GENERIC;
	}
	else
	{
		throw tcu::NotSupportedError("Test not supported in non-windowed context");
	}

	m_rc = createRenderContext(platform, commandLine, renderCfg);
	m_rc->makeCurrent();
}

/** Finalize rendering context
 **/
void TestBase::done()
{
	/* Delete context used by test */
	m_test_base_context.setRenderContext(m_orig_rc);

	delete m_rc;

	/* Switch back to original context */
	m_test_base_context.getRenderContext().makeCurrent();

	m_rc = 0;
	m_gl = 0;
}

/** Constructor
 *
 * @param context  Test context
 * @param is_debug Selects if debug or non-debug context should be used
 * @param name     Name of test
 **/
APIErrorsTest::APIErrorsTest(deqp::Context& context, bool is_debug, const GLchar* name)
	: TestCase(context, name, "Verifies that errors are generated as expected"), TestBase(context, is_debug)
{
	/* Nothing to be done */
}

/** Execute test
 *
 * @return tcu::TestNode::STOP
 **/
tcu::TestNode::IterateResult APIErrorsTest::iterate()
{
	/* Initialize rendering context */
	TestBase::init();

	/* Get maximum label length */
	GLint max_label = 0;

	m_gl->getIntegerv(GL_MAX_LABEL_LENGTH, &max_label);
	GLU_EXPECT_NO_ERROR(m_gl->getError(), "GetIntegerv");

	/* Prepare too long label */
	std::vector<GLchar> too_long_label;

	too_long_label.resize(max_label + 2);

	for (GLint i = 0; i <= max_label; ++i)
	{
		too_long_label[i] = 'f';
	}

	too_long_label[max_label + 1] = 0;

	/* Get maximum message length */
	GLint max_length = 0;

	m_gl->getIntegerv(GL_MAX_DEBUG_MESSAGE_LENGTH, &max_length);
	GLU_EXPECT_NO_ERROR(m_gl->getError(), "GetIntegerv");

	/* Prepare too long message */
	std::vector<GLchar> too_long_message;

	too_long_message.resize(max_length + 2);

	for (GLint i = 0; i <= max_length; ++i)
	{
		too_long_message[i] = 'f';
	}

	too_long_message[max_length + 1] = 0;

	/* Get maximum number of groups on stack */
	GLint max_groups = 0;

	m_gl->getIntegerv(GL_MAX_DEBUG_GROUP_STACK_DEPTH, &max_groups);
	GLU_EXPECT_NO_ERROR(m_gl->getError(), "GetIntegerv");

	/*
	 * DebugMessageControl function should generate:
	 * - INVALID_ENUM when <source> is invalid;
	 * - INVALID_ENUM when <type> is invalid;
	 * - INVALID_ENUM when <severity> is invalid;
	 * - INVALID_VALUE when <count> is negative;
	 * - INVALID_OPERATION when <count> is not zero and <source> is DONT_CARE;
	 * - INVALID_OPERATION when <count> is not zero and <type> is DONT_CARE;
	 * - INVALID_OPERATION when <count> is not zero and <severity> is not
	 * DONT_CARE.
	 */
	{
		static const GLuint  ids[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8 };
		static const GLsizei n_ids = (GLsizei)(sizeof(ids) / sizeof(ids[0]));

		m_gl->debugMessageControl(GL_ARRAY_BUFFER /* source */, GL_DEBUG_TYPE_ERROR /* type */,
								  GL_DEBUG_SEVERITY_LOW /* severity */, 0 /* count */, 0 /* ids */,
								  GL_TRUE /* enabled */);
		CHECK_ERROR(GL_INVALID_ENUM, "DebugMessageControl with <source> set to GL_ARRAY_BUFFER");

		m_gl->debugMessageControl(GL_DEBUG_SOURCE_API /* source */, GL_ARRAY_BUFFER /* type */,
								  GL_DEBUG_SEVERITY_LOW /* severity */, 0 /* count */, 0 /* ids */,
								  GL_TRUE /* enabled */);
		CHECK_ERROR(GL_INVALID_ENUM, "DebugMessageControl with <type> set to GL_ARRAY_BUFFER");

		m_gl->debugMessageControl(GL_DEBUG_SOURCE_API /* source */, GL_DEBUG_TYPE_ERROR /* type */,
								  GL_ARRAY_BUFFER /* severity */, 0 /* count */, 0 /* ids */, GL_TRUE /* enabled */);
		CHECK_ERROR(GL_INVALID_ENUM, "DebugMessageControl with <severity> set to GL_ARRAY_BUFFER");

		m_gl->debugMessageControl(GL_DEBUG_SOURCE_API /* source */, GL_DEBUG_TYPE_ERROR /* type */,
								  GL_DEBUG_SEVERITY_LOW /* severity */, -1 /* count */, ids /* ids */,
								  GL_TRUE /* enabled */);
		CHECK_ERROR(GL_INVALID_VALUE, "DebugMessageControl with <count> set to -1");

		m_gl->debugMessageControl(GL_DONT_CARE /* source */, GL_DEBUG_TYPE_ERROR /* type */,
								  GL_DONT_CARE /* severity */, n_ids /* count */, ids /* ids */, GL_TRUE /* enabled */);
		CHECK_ERROR(GL_INVALID_OPERATION, "DebugMessageControl with <source> set to GL_DONT_CARE and non zero <count>");

		m_gl->debugMessageControl(GL_DEBUG_SOURCE_API /* source */, GL_DONT_CARE /* type */,
								  GL_DONT_CARE /* severity */, n_ids /* count */, ids /* ids */, GL_TRUE /* enabled */);
		CHECK_ERROR(GL_INVALID_OPERATION, "DebugMessageControl with <type> set to GL_DONT_CARE and non zero <count>");

		m_gl->debugMessageControl(GL_DEBUG_SOURCE_API /* source */, GL_DEBUG_TYPE_ERROR /* type */,
								  GL_DEBUG_SEVERITY_LOW /* severity */, n_ids /* count */, ids /* ids */,
								  GL_TRUE /* enabled */);
		CHECK_ERROR(GL_INVALID_OPERATION,
					"DebugMessageControl with <severity> set to GL_DEBUG_SEVERITY_LOW and non zero <count>");
	}

	/*
	 * GetDebugMessageLog function should generate:
	 * - INVALID_VALUE when <bufSize> is negative and messageLog is not NULL.
	 */
	{
		static const GLsizei bufSize = 32;
		static const GLuint  count   = 4;

		GLenum  ids[count];
		GLsizei lengths[count];
		GLchar  messageLog[bufSize];
		GLenum  types[count];
		GLenum  severities[count];
		GLenum  sources[count];

		m_gl->getDebugMessageLog(count /* count */, -1 /* bufSize */, sources, types, ids, severities, lengths,
								 messageLog);
		CHECK_ERROR(GL_INVALID_VALUE, "GetDebugMessageLog with <bufSize> set to -1");
	}

	/*
	 * DebugMessageInsert function should generate:
	 * - INVALID_ENUM when <source> is not DEBUG_SOURCE_APPLICATION or
	 * DEBUG_SOURCE_THIRD_PARTY;
	 * - INVALID_ENUM when <type> is invalid;
	 * - INVALID_ENUM when <severity> is invalid;
	 * - INVALID_VALUE when length of string <buf> is not less than
	 * MAX_DEBUG_MESSAGE_LENGTH.
	 */
	{
		static const GLchar  message[] = "Foo";
		static const GLsizei length	= (GLsizei)(sizeof(message) / sizeof(message[0]));

		m_gl->debugMessageInsert(GL_DEBUG_SOURCE_API /* source */, GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR /* type */,
								 0 /* id */, GL_DEBUG_SEVERITY_LOW /* severity */, length /* length */,
								 message /* message */);
		CHECK_ERROR(GL_INVALID_ENUM, "DebugMessageInsert with <source> set to GL_DEBUG_SOURCE_API");

		m_gl->debugMessageInsert(GL_DEBUG_SOURCE_APPLICATION /* source */, GL_ARRAY_BUFFER /* type */, 0 /* id */,
								 GL_DEBUG_SEVERITY_LOW /* severity */, length /* length */, message /* message */);
		CHECK_ERROR(GL_INVALID_ENUM, "DebugMessageInsert with <type> set to GL_ARRAY_BUFFER");

		m_gl->debugMessageInsert(GL_DEBUG_SOURCE_APPLICATION /* source */, GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR /* type */,
								 0 /* id */, GL_ARRAY_BUFFER /* severity */, length /* length */,
								 message /* message */);
		CHECK_ERROR(GL_INVALID_ENUM, "DebugMessageInsert with <severity> set to GL_ARRAY_BUFFER");

		m_gl->debugMessageInsert(GL_DEBUG_SOURCE_APPLICATION /* source */, GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR /* type */,
								 0 /* id */, GL_DEBUG_SEVERITY_LOW /* severity */, max_length + 1 /* length */,
								 message /* message */);
		CHECK_ERROR(GL_INVALID_VALUE, "DebugMessageInsert with <length> set to GL_MAX_DEBUG_MESSAGE_LENGTH + 1");

		m_gl->debugMessageInsert(GL_DEBUG_SOURCE_APPLICATION /* source */, GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR /* type */,
								 0 /* id */, GL_DEBUG_SEVERITY_LOW /* severity */, -1 /* length */,
								 &too_long_message[0] /* message */);
		CHECK_ERROR(GL_INVALID_VALUE, "DebugMessageInsert with too long message");
	}

	/*
	 * PushDebugGroup function should generate:
	 * - INVALID_ENUM when <source> is not DEBUG_SOURCE_APPLICATION or
	 * DEBUG_SOURCE_THIRD_PARTY;
	 * - INVALID_VALUE when length of string <message> is not less than
	 * MAX_DEBUG_MESSAGE_LENGTH;
	 * - STACK_OVERFLOW when stack contains MAX_DEBUG_GROUP_STACK_DEPTH entries.
	 */
	{
		static const GLchar  message[] = "Foo";
		static const GLsizei length	= (GLsizei)(sizeof(message) / sizeof(message[0]));

		m_gl->pushDebugGroup(GL_DEBUG_SOURCE_API /* source */, 1 /* id */, length /* length */, message /* message */);
		CHECK_ERROR(GL_INVALID_ENUM, "PushDebugGroup with <source> set to GL_DEBUG_SOURCE_API");

		m_gl->pushDebugGroup(GL_DEBUG_SOURCE_APPLICATION /* source */, 1 /* id */, max_length + 1 /* length */,
							 message /* message */);
		CHECK_ERROR(GL_INVALID_VALUE, "PushDebugGroup with <length> set to GL_MAX_DEBUG_MESSAGE_LENGTH + 1");

		m_gl->pushDebugGroup(GL_DEBUG_SOURCE_APPLICATION /* source */, 1 /* id */, -1 /* length */,
							 &too_long_message[0] /* message */);
		CHECK_ERROR(GL_INVALID_VALUE, "PushDebugGroup with too long message");

		/* Clean stack */
		cleanGroupStack(m_gl);

		/* Fill stack */
		for (GLint i = 0; i < max_groups - 1; ++i)
		{
			m_gl->pushDebugGroup(GL_DEBUG_SOURCE_APPLICATION /* source */, 1 /* id */, length /* length */,
								 message /* message */);
			GLU_EXPECT_NO_ERROR(m_gl->getError(), "PushDebugGroup");
		}

		/* Overflow stack */
		m_gl->pushDebugGroup(GL_DEBUG_SOURCE_APPLICATION /* source */, 1 /* id */, length /* length */,
							 message /* message */);
		CHECK_ERROR(GL_STACK_OVERFLOW, "PushDebugGroup called GL_MAX_DEBUG_GROUP_STACK_DEPTH times");

		/* Clean stack */
		cleanGroupStack(m_gl);
	}

	/*
	 * PopDebugGroup function should generate:
	 * - STACK_UNDERFLOW when stack contains no entries.
	 */
	{
		fillGroupStack(m_gl);

		for (GLint i = 0; i < max_groups - 1; ++i)
		{
			m_gl->popDebugGroup();

			GLU_EXPECT_NO_ERROR(m_gl->getError(), "PopDebugGroup");
		}

		m_gl->popDebugGroup();
		CHECK_ERROR(GL_STACK_UNDERFLOW, "PopDebugGroup called GL_MAX_DEBUG_GROUP_STACK_DEPTH times");
	}

	/*
	 * ObjectLabel function should generate:
	 * - INVALID_ENUM when <identifier> is invalid;
	 * - INVALID_VALUE when if <name> is not valid object name of type specified by
	 * <identifier>;
	 * - INVALID_VALUE when length of string <label> is not less than
	 * MAX_LABEL_LENGTH.
	 */
	{
		static const GLchar  label[] = "Foo";
		static const GLsizei length  = (GLsizei)(sizeof(label) / sizeof(label[0]));

		GLuint texture_id = 0;
		GLuint invalid_id = 1;
		m_gl->genTextures(1, &texture_id);
		GLU_EXPECT_NO_ERROR(m_gl->getError(), "GenTextures");
		m_gl->bindTexture(GL_TEXTURE_BUFFER, texture_id);
		GLU_EXPECT_NO_ERROR(m_gl->getError(), "BindTexture");

		try
		{
			m_gl->objectLabel(GL_TEXTURE_BUFFER /* identifier */, texture_id /* name */, length /* length */,
							  label /* label */);
			CHECK_ERROR(GL_INVALID_ENUM, "ObjectLabel with <identifier> set to GL_TEXTURE_BUFFER");

			while (GL_TRUE == m_gl->isTexture(invalid_id))
			{
				invalid_id += 1;
			}

			m_gl->objectLabel(GL_TEXTURE /* identifier */, invalid_id /* name */, length /* length */,
							  label /* label */);
			CHECK_ERROR(GL_INVALID_VALUE, "ObjectLabel with <name> set to not generated value");

			m_gl->objectLabel(GL_TEXTURE /* identifier */, texture_id /* name */, max_label + 1 /* length */,
							  label /* label */);
			CHECK_ERROR(GL_INVALID_VALUE, "ObjectLabel with <label> set to MAX_LABEL_LENGTH + 1");

			m_gl->objectLabel(GL_TEXTURE /* identifier */, texture_id /* name */, -1 /* length */,
							  &too_long_label[0] /* label */);
			CHECK_ERROR(GL_INVALID_VALUE, "ObjectLabel with too long label");
		}
		catch (const std::exception& exc)
		{
			m_gl->deleteTextures(1, &texture_id);
			TCU_FAIL(exc.what());
		}

		m_gl->deleteTextures(1, &texture_id);
	}

	/*
	 * GetObjectLabel function should generate:
	 * - INVALID_ENUM when <identifier> is invalid;
	 * - INVALID_VALUE when if <name> is not valid object name of type specified by
	 * <identifier>;
	 * - INVALID_VALUE when <bufSize> is negative.
	 */
	{
		static const GLsizei bufSize = 32;

		GLchar  label[bufSize];
		GLsizei length = 0;

		GLuint texture_id = 0;
		GLuint invalid_id = 1;
		m_gl->genTextures(1, &texture_id);
		m_gl->bindTexture(GL_TEXTURE_2D, texture_id);
		GLU_EXPECT_NO_ERROR(m_gl->getError(), "GenTextures");

		try
		{
			m_gl->getObjectLabel(GL_TEXTURE_BUFFER /* identifier */, texture_id /* name */, bufSize /* bufSize */,
								 &length /* length */, label /* label */);
			CHECK_ERROR(GL_INVALID_ENUM, "GetObjectLabel with <identifier> set to GL_TEXTURE_BUFFER");

			while (GL_TRUE == m_gl->isTexture(invalid_id))
			{
				invalid_id += 1;
			}

			m_gl->getObjectLabel(GL_TEXTURE /* identifier */, invalid_id /* name */, bufSize /* bufSize */,
								 &length /* length */, label /* label */);
			CHECK_ERROR(GL_INVALID_VALUE, "GetObjectLabel with <name> set to not generated value");

			m_gl->getObjectLabel(GL_TEXTURE /* identifier */, invalid_id /* name */, -1 /* bufSize */,
								 &length /* length */, label /* label */);
			CHECK_ERROR(GL_INVALID_VALUE, "GetObjectLabel with <bufSize> set to -1");
		}
		catch (const std::exception& exc)
		{
			m_gl->deleteTextures(1, &texture_id);
			TCU_FAIL(exc.what());
		}

		m_gl->deleteTextures(1, &texture_id);
	}

	/*
	 * ObjectPtrLabel function should generate:
	 * - INVALID_VALUE when <ptr> is not the name of sync object;
	 * - INVALID_VALUE when length of string <label> is not less than
	 * MAX_LABEL_LENGTH.
	 */
	{
		static const GLchar  label[] = "Foo";
		static const GLsizei length  = (GLsizei)(sizeof(label) / sizeof(label[0]));

		GLsync sync_id	= 0;
		GLsync invalid_id = 0;
		sync_id			  = m_gl->fenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
		GLU_EXPECT_NO_ERROR(m_gl->getError(), "FenceSync");

		try
		{
			while (GL_TRUE == m_gl->isSync(invalid_id))
			{
				invalid_id = (GLsync)(((unsigned long long)invalid_id) + 1);
			}

			m_gl->objectPtrLabel(invalid_id /* name */, length /* length */, label /* label */);
			CHECK_ERROR(GL_INVALID_VALUE, "ObjectPtrLabel with <ptr> set to not generated value");

			m_gl->objectPtrLabel(sync_id /* name */, max_label + 1 /* length */, label /* label */);
			CHECK_ERROR(GL_INVALID_VALUE, "ObjectPtrLabel with <length> set to MAX_LABEL_LENGTH + 1");

			m_gl->objectPtrLabel(sync_id /* name */, -1 /* length */, &too_long_label[0] /* label */);
			CHECK_ERROR(GL_INVALID_VALUE, "ObjectPtrLabel with too long label");
		}
		catch (const std::exception& exc)
		{
			m_gl->deleteSync(sync_id);
			TCU_FAIL(exc.what());
		}

		m_gl->deleteSync(sync_id);
	}

	/*
	 * GetObjectPtrLabel function should generate:
	 * - INVALID_VALUE when <ptr> is not the name of sync object;
	 * - INVALID_VALUE when <bufSize> is negative.
	 */
	{
		static const GLsizei bufSize = 32;

		GLchar  label[bufSize];
		GLsizei length = 0;

		GLsync sync_id	= 0;
		GLsync invalid_id = 0;
		sync_id			  = m_gl->fenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
		GLU_EXPECT_NO_ERROR(m_gl->getError(), "FenceSync");

		try
		{
			while (GL_TRUE == m_gl->isSync(invalid_id))
			{
				invalid_id = (GLsync)(((unsigned long long)invalid_id) + 1);
			}

			m_gl->getObjectPtrLabel(invalid_id /* name */, bufSize /* bufSize */, &length /* length */,
									label /* label */);
			CHECK_ERROR(GL_INVALID_VALUE, "GetObjectPtrLabel with <ptr> set to not generated value");

			m_gl->getObjectPtrLabel(sync_id /* name */, -1 /* bufSize */, &length /* length */, label /* label */);
			CHECK_ERROR(GL_INVALID_VALUE, "GetObjectPtrLabel with <bufSize> set to -1");
		}
		catch (const std::exception& exc)
		{
			m_gl->deleteSync(sync_id);
			TCU_FAIL(exc.what());
		}

		m_gl->deleteSync(sync_id);
	}

	/*
	 * GetPointerv function should generate:
	 * - INVALID_ENUM when <pname> is invalid.
	 **/
	{
		GLuint  uint;
		GLuint* uint_ptr = &uint;

		m_gl->getPointerv(GL_ARRAY_BUFFER, (GLvoid**)&uint_ptr);
		CHECK_ERROR(GL_INVALID_ENUM, "GetPointerv with <pname> set to GL_ARRAY_BUFFER");
	}

	/* Set result */
	m_context.getTestContext().setTestResult(QP_TEST_RESULT_PASS, "Pass");

	/* Done */
	TestBase::done();

	return tcu::TestNode::STOP;
}

/** Constructor
 *
 * @param context  Test context
 * @param is_debug Selects if debug or non-debug context should be used
 * @param name     Name of test
 **/
LabelsTest::LabelsTest(deqp::Context& context, bool is_debug, const GLchar* name)
	: TestCase(context, name, "Verifies that labels can be assigned and queried"), TestBase(context, is_debug)
{
	/* Nothing to be done */
}

/** Represnets case for LabelsTest **/
struct labelsTestCase
{
	GLenum m_identifier;
	GLuint (*m_create)(const glw::Functions* gl, const glu::RenderContext*);
	GLvoid (*m_destroy)(const glw::Functions* gl, GLuint id);
	const GLchar* m_name;
};

/** Execute test
 *
 * @return tcu::TestNode::STOP
 **/
tcu::TestNode::IterateResult LabelsTest::iterate()
{
	static const labelsTestCase test_cases[] = {
		{ GL_BUFFER, createBuffer, deleteBuffer, "Buffer" },
		{ GL_FRAMEBUFFER, createFramebuffer, deleteFramebuffer, "Framebuffer" },
		{ GL_PROGRAM, createProgram, deleteProgram, "Program" },
		{ GL_PROGRAM_PIPELINE, createProgramPipeline, deleteProgramPipeline, "ProgramPipeline" },
		{ GL_QUERY, createQuery, deleteQuery, "Query" },
		{ GL_RENDERBUFFER, createRenderbuffer, deleteRenderbuffer, "Renderbuffer" },
		{ GL_SAMPLER, createSampler, deleteSampler, "Sampler" },
		{ GL_SHADER, createShader, deleteShader, "Shader" },
		{ GL_TEXTURE, createTexture, deleteTexture, "Texture" },
		{ GL_TRANSFORM_FEEDBACK, createTransformFeedback, deleteTransformFeedback, "TransformFeedback" },
		{ GL_VERTEX_ARRAY, createVertexArray, deleteVertexArray, "VertexArray" },
	};

	static const size_t n_test_cases = sizeof(test_cases) / sizeof(test_cases[0]);

	static const GLsizei bufSize	  = 32;
	static const GLchar  label[]	  = "foo";
	static const GLsizei label_length = (GLsizei)(sizeof(label) / sizeof(label[0]) - 1);

	/* Initialize render context */
	TestBase::init();

	/* For each test case */
	for (size_t test_case_index = 0; test_case_index < n_test_cases; ++test_case_index)
	{
		const labelsTestCase& test_case = test_cases[test_case_index];

		const GLenum identifier = test_case.m_identifier;
		const GLuint id			= test_case.m_create(m_gl, m_rc);

		try
		{
			GLchar  buffer[bufSize] = "HelloWorld";
			GLsizei length;

			/*
			 * - query label; It is expected that result will be an empty string and length
			 * will be zero;
			 */
			m_gl->getObjectLabel(identifier, id, bufSize, &length, buffer);
			GLU_EXPECT_NO_ERROR(m_gl->getError(), "GetObjectLabel");

			if (0 != length)
			{
				TCU_FAIL("Just created object has label of length != 0");
			}

			if (0 != buffer[0])
			{
				TCU_FAIL("Just created object has not empty label");
			}

			/*
			 * - assign label to object;
			 * - query label; It is expected that result will be equal to the provided
			 * label and length will be correct;
			 */
			m_gl->objectLabel(identifier, id, -1 /* length */, label);
			GLU_EXPECT_NO_ERROR(m_gl->getError(), "ObjectLabel");

			m_gl->getObjectLabel(identifier, id, bufSize, &length, buffer);
			GLU_EXPECT_NO_ERROR(m_gl->getError(), "GetObjectLabel");

			if (label_length != length)
			{
				TCU_FAIL("Length is different than length of set label");
			}

			if (0 != strcmp(buffer, label))
			{
				TCU_FAIL("Different label returned");
			}

			/*
			 * - query length only; Correct value is expected;
			 */
			length = 0;

			m_gl->getObjectLabel(identifier, id, 0 /* bufSize */, &length, 0 /* label */);
			GLU_EXPECT_NO_ERROR(m_gl->getError(), "GetObjectLabel");

			if (label_length != length)
			{
				TCU_FAIL("Invalid length returned when label and bufSize are set to 0");
			}

			/*
			 * - query label with <bufSize> less than actual length of label; It is
			 * expected that only <bufSize> characters will be stored in buffer (including
			 * NULL);
			 */
			length = 0;
			strcpy(buffer, "HelloWorld");

			m_gl->getObjectLabel(identifier, id, 2 /* bufSize */, &length, buffer);
			GLU_EXPECT_NO_ERROR(m_gl->getError(), "GetObjectLabel");

			if (buffer[0] != label[0])
			{
				TCU_FAIL("Different label returned");
			}

			if (buffer[1] != 0)
			{
				TCU_FAIL("GetObjectLabel did not stored NULL at the end of string");
			}

			if (buffer[2] != 'l')
			{
				TCU_FAIL("GetObjectLabel overflowed buffer");
			}

			/*
			 * - query label with <bufSize> equal zero; It is expected that buffer contents
			 * will not be modified;
			 */
			length = 0;
			strcpy(buffer, "HelloWorld");

			m_gl->getObjectLabel(identifier, id, 0 /* bufSize */, &length, buffer);
			GLU_EXPECT_NO_ERROR(m_gl->getError(), "GetObjectLabel");

			if (0 != strcmp(buffer, "HelloWorld"))
			{
				TCU_FAIL("GetObjectLabel modified buffer, bufSize set to 0");
			}

			/*
			 * - assign empty string as label to object;
			 * - query label, it is expected that result will be an empty string and length
			 * will be zero;
			 */
			m_gl->objectLabel(identifier, id, -1 /* length */, "");
			GLU_EXPECT_NO_ERROR(m_gl->getError(), "ObjectLabel");

			m_gl->getObjectLabel(identifier, id, bufSize, &length, buffer);
			GLU_EXPECT_NO_ERROR(m_gl->getError(), "GetObjectLabel");

			if (0 != length)
			{
				TCU_FAIL("Label length is != 0, empty string was set");
			}

			if (0 != buffer[0])
			{
				TCU_FAIL("Non empty label returned, empty string was set");
			}

			/*
			 * - assign NULL as label to object;
			 * - query label, it is expected that result will be an empty string and length
			 * will be zero;
			 */
			m_gl->objectLabel(identifier, id, 2, 0 /* label */);
			GLU_EXPECT_NO_ERROR(m_gl->getError(), "ObjectLabel");

			m_gl->getObjectLabel(identifier, id, bufSize, &length, buffer);
			GLU_EXPECT_NO_ERROR(m_gl->getError(), "GetObjectLabel");

			if (0 != length)
			{
				TCU_FAIL("Label length is != 0, label was removed");
			}

			if (0 != buffer[0])
			{
				TCU_FAIL("Different label returned, label was removed");
			}
		}
		catch (const std::exception& exc)
		{
			test_case.m_destroy(m_gl, id);

			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "Error during test case: " << test_case.m_name << tcu::TestLog::EndMessage;

			TCU_FAIL(exc.what());
		}

		test_case.m_destroy(m_gl, id);
	}

	/* Set result */
	m_context.getTestContext().setTestResult(QP_TEST_RESULT_PASS, "Pass");

	/* Done */
	TestBase::done();

	return tcu::TestNode::STOP;
}

/** Create buffer
 *
 * @param gl GL functions
 *
 * @return ID of created resource
 **/
GLuint LabelsTest::createBuffer(const Functions* gl, const glu::RenderContext* rc)
{
	GLuint id = 0;

	if (glu::contextSupports(rc->getType(), glu::ApiType::core(4, 5)))
	{
		gl->createBuffers(1, &id);
		GLU_EXPECT_NO_ERROR(gl->getError(), "CreateBuffers");
	}
	else
	{
		gl->genBuffers(1, &id);
		gl->bindBuffer(GL_ARRAY_BUFFER, id);
		GLU_EXPECT_NO_ERROR(gl->getError(), "GenBuffers");
	}

	return id;
}

/** Create FBO
 *
 * @param gl GL functions
 *
 * @return ID of created resource
 **/
GLuint LabelsTest::createFramebuffer(const Functions* gl, const glu::RenderContext* rc)
{
	GLuint id = 0;
	if (glu::contextSupports(rc->getType(), glu::ApiType::core(4, 5)))
	{
		gl->createFramebuffers(1, &id);
		GLU_EXPECT_NO_ERROR(gl->getError(), "CreateFramebuffers");
	}
	else
	{
		GLint currentFbo;
		gl->getIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &currentFbo);
		gl->genFramebuffers(1, &id);
		gl->bindFramebuffer(GL_DRAW_FRAMEBUFFER, id);
		gl->bindFramebuffer(GL_DRAW_FRAMEBUFFER, currentFbo);
		GLU_EXPECT_NO_ERROR(gl->getError(), "GenFramebuffers / BindFramebuffer");
	}

	return id;
}

/** Create program
 *
 * @param gl GL functions
 *
 * @return ID of created resource
 **/
GLuint LabelsTest::createProgram(const Functions* gl, const glu::RenderContext*)
{
	GLuint id = gl->createProgram();
	GLU_EXPECT_NO_ERROR(gl->getError(), "CreateProgram");

	return id;
}

/** Create pipeline
 *
 * @param gl GL functions
 *
 * @return ID of created resource
 **/
GLuint LabelsTest::createProgramPipeline(const Functions* gl, const glu::RenderContext* rc)
{
	GLuint id = 0;
	if (glu::contextSupports(rc->getType(), glu::ApiType::core(4, 5)))
	{
		gl->createProgramPipelines(1, &id);
		GLU_EXPECT_NO_ERROR(gl->getError(), "CreateProgramPipelines");
	}
	else
	{
		gl->genProgramPipelines(1, &id);
		gl->bindProgramPipeline(id);
		GLU_EXPECT_NO_ERROR(gl->getError(), "GenProgramPipelines / BindProgramPipeline");
	}

	return id;
}

/** Create query
 *
 * @param gl GL functions
 *
 * @return ID of created resource
 **/
GLuint LabelsTest::createQuery(const Functions* gl, const glu::RenderContext* rc)
{
	GLuint id = 0;
	if (glu::contextSupports(rc->getType(), glu::ApiType::core(4, 5)))
	{
		gl->createQueries(GL_TIMESTAMP, 1, &id);
		GLU_EXPECT_NO_ERROR(gl->getError(), "CreateQueries");
	}
	else
	{
		gl->genQueries(1, &id);
		gl->beginQuery(GL_SAMPLES_PASSED, id);
		gl->endQuery(GL_SAMPLES_PASSED);
		GLU_EXPECT_NO_ERROR(gl->getError(), "GenQueries / BeginQuery / EndQuery");
	}

	return id;
}

/** Create render buffer
 *
 * @param gl GL functions
 *
 * @return ID of created resource
 **/
GLuint LabelsTest::createRenderbuffer(const Functions* gl, const glu::RenderContext* rc)
{
	GLuint id = 0;

	if (glu::contextSupports(rc->getType(), glu::ApiType::core(4, 5)))
	{
		gl->createRenderbuffers(1, &id);
		GLU_EXPECT_NO_ERROR(gl->getError(), "CreateRenderbuffers");
	}
	else
	{
		gl->genRenderbuffers(1, &id);
		gl->bindRenderbuffer(GL_RENDERBUFFER, id);
		gl->bindRenderbuffer(GL_RENDERBUFFER, 0);
		GLU_EXPECT_NO_ERROR(gl->getError(), "GenRenderbuffers / BindRenderbuffer");
	}

	return id;
}

/** Create sampler
 *
 * @param gl GL functions
 *
 * @return ID of created resource
 **/
GLuint LabelsTest::createSampler(const Functions* gl, const glu::RenderContext* rc)
{
	GLuint id = 0;
	if (glu::contextSupports(rc->getType(), glu::ApiType::core(4, 5)))
	{
		gl->createSamplers(1, &id);
		GLU_EXPECT_NO_ERROR(gl->getError(), "CreateSamplers");
	}
	else
	{
		gl->genSamplers(1, &id);
		gl->bindSampler(0, id);
		gl->bindSampler(0, 0);
		GLU_EXPECT_NO_ERROR(gl->getError(), "GenSamplers / BindSampler");
	}

	return id;
}

/** Create shader
 *
 * @param gl GL functions
 *
 * @return ID of created resource
 **/
GLuint LabelsTest::createShader(const Functions* gl, const glu::RenderContext*)
{
	GLuint id = gl->createShader(GL_VERTEX_SHADER);
	GLU_EXPECT_NO_ERROR(gl->getError(), "CreateShader");

	return id;
}

/** Create texture
 *
 * @param gl GL functions
 *
 * @return ID of created resource
 **/
GLuint LabelsTest::createTexture(const Functions* gl, const glu::RenderContext* rc)
{
	GLuint id = 0;
	if (glu::contextSupports(rc->getType(), glu::ApiType::core(4, 5)))
	{
		gl->createTextures(GL_TEXTURE_2D, 1, &id);
		GLU_EXPECT_NO_ERROR(gl->getError(), "CreateTextures");
	}
	else
	{
		gl->genTextures(1, &id);
		gl->bindTexture(GL_TEXTURE_2D, id);
		gl->bindTexture(GL_TEXTURE_2D, 0);
		GLU_EXPECT_NO_ERROR(gl->getError(), "GenTextures / BindTexture");
	}

	return id;
}

/** Create XFB
 *
 * @param gl GL functions
 *
 * @return ID of created resource
 **/
GLuint LabelsTest::createTransformFeedback(const Functions* gl, const glu::RenderContext* rc)
{
	GLuint id = 0;
	if (glu::contextSupports(rc->getType(), glu::ApiType::core(4, 5)))
	{
		gl->createTransformFeedbacks(1, &id);
		GLU_EXPECT_NO_ERROR(gl->getError(), "CreateTransformFeedbacks");
	}
	else
	{
		gl->genTransformFeedbacks(1, &id);
		gl->bindTransformFeedback(GL_TRANSFORM_FEEDBACK, id);
		gl->bindTransformFeedback(GL_TRANSFORM_FEEDBACK, 0);
		GLU_EXPECT_NO_ERROR(gl->getError(), "GenTransformFeedbacks / BindTransformFeedback");
	}

	return id;
}

/** Create VAO
 *
 * @param gl GL functions
 *
 * @return ID of created resource
 **/
GLuint LabelsTest::createVertexArray(const Functions* gl, const glu::RenderContext* rc)
{
	GLuint id = 0;

	if (glu::contextSupports(rc->getType(), glu::ApiType::core(4, 5)))
	{
		gl->createVertexArrays(1, &id);
		GLU_EXPECT_NO_ERROR(gl->getError(), "CreateVertexArrays");
	}
	else
	{
		gl->genVertexArrays(1, &id);
		gl->bindVertexArray(id);
		gl->bindVertexArray(0);
		GLU_EXPECT_NO_ERROR(gl->getError(), "GenVertexArrays / BindVertexArrays");
	}

	return id;
}

/** Destroy buffer
 *
 * @param gl GL functions
 * @param id ID of resource
 **/
GLvoid LabelsTest::deleteBuffer(const Functions* gl, GLuint id)
{
	gl->deleteBuffers(1, &id);
	GLU_EXPECT_NO_ERROR(gl->getError(), "DeleteBuffers");
}

/** Destroy FBO
 *
 * @param gl GL functions
 * @param id ID of resource
 **/
GLvoid LabelsTest::deleteFramebuffer(const Functions* gl, GLuint id)
{
	gl->deleteFramebuffers(1, &id);
	GLU_EXPECT_NO_ERROR(gl->getError(), "DeleteFramebuffers");
}

/** Destroy program
 *
 * @param gl GL functions
 * @param id ID of resource
 **/
GLvoid LabelsTest::deleteProgram(const Functions* gl, GLuint id)
{
	gl->deleteProgram(id);
	GLU_EXPECT_NO_ERROR(gl->getError(), "DeleteProgram");
}

/** Destroy pipeline
 *
 * @param gl GL functions
 * @param id ID of resource
 **/
GLvoid LabelsTest::deleteProgramPipeline(const Functions* gl, GLuint id)
{
	gl->deleteProgramPipelines(1, &id);
	GLU_EXPECT_NO_ERROR(gl->getError(), "DeleteProgramPipelines");
}

/** Destroy query
 *
 * @param gl GL functions
 * @param id ID of resource
 **/
GLvoid LabelsTest::deleteQuery(const Functions* gl, GLuint id)
{
	gl->deleteQueries(1, &id);
	GLU_EXPECT_NO_ERROR(gl->getError(), "DeleteQueries");
}

/** Destroy render buffer
 *
 * @param gl GL functions
 * @param id ID of resource
 **/
GLvoid LabelsTest::deleteRenderbuffer(const Functions* gl, GLuint id)
{
	gl->deleteRenderbuffers(1, &id);
	GLU_EXPECT_NO_ERROR(gl->getError(), "DeleteRenderbuffers");
}

/** Destroy sampler
 *
 * @param gl GL functions
 * @param id ID of resource
 **/
GLvoid LabelsTest::deleteSampler(const Functions* gl, GLuint id)
{
	gl->deleteSamplers(1, &id);
	GLU_EXPECT_NO_ERROR(gl->getError(), "DeleteSamplers");
}

/** Destroy shader
 *
 * @param gl GL functions
 * @param id ID of resource
 **/
GLvoid LabelsTest::deleteShader(const Functions* gl, GLuint id)
{
	gl->deleteShader(id);
	GLU_EXPECT_NO_ERROR(gl->getError(), "DeleteShader");
}

/** Destroy texture
 *
 * @param gl GL functions
 * @param id ID of resource
 **/
GLvoid LabelsTest::deleteTexture(const Functions* gl, GLuint id)
{
	gl->deleteTextures(1, &id);
	GLU_EXPECT_NO_ERROR(gl->getError(), "DeleteTextures");
}

/** Destroy XFB
 *
 * @param gl GL functions
 * @param id ID of resource
 **/
GLvoid LabelsTest::deleteTransformFeedback(const Functions* gl, GLuint id)
{
	gl->deleteTransformFeedbacks(1, &id);
	GLU_EXPECT_NO_ERROR(gl->getError(), "DeleteTransformFeedbacks");
}

/** Destroy VAO
 *
 * @param gl GL functions
 * @param id ID of resource
 **/
GLvoid LabelsTest::deleteVertexArray(const Functions* gl, GLuint id)
{
	gl->deleteVertexArrays(1, &id);
	GLU_EXPECT_NO_ERROR(gl->getError(), "DeleteVertexArrays");
}

/** Constructor
 *
 * @param context  Test context
 * @param is_debug Selects if debug or non-debug context should be used
 * @param name     Name of test
 **/
ReceiveingMessagesTest::ReceiveingMessagesTest(deqp::Context& context)
	: TestCase(context, "receiveing_messages", "Verifies that messages can be received")
	, TestBase(context, true /* is_debug */)
{
	/* Nothing to be done */
}

/** Execute test
 *
 * @return tcu::TestNode::STOP
 **/
tcu::TestNode::IterateResult ReceiveingMessagesTest::iterate()
{
	static const size_t bufSize		  = 32;
	static const GLchar label[]		  = "foo";
	static const size_t label_length  = sizeof(label) / sizeof(label[0]) - 1;
	static const size_t read_messages = 4;

	GLuint callback_counter   = 0;
	GLint  max_debug_messages = 0;

	/* Initialize render context */
	TestBase::init();

	/* Get max number of debug messages */
	m_gl->getIntegerv(GL_MAX_DEBUG_LOGGED_MESSAGES, &max_debug_messages);
	GLU_EXPECT_NO_ERROR(m_gl->getError(), "GetIntegerv");

	/*
	 * - verify that the state of DEBUG_OUTPUT is enabled as it should be by
	 * default;
	 * - verify that state of DEBUG_CALLBACK_FUNCTION and
	 * DEBUG_CALLBACK_USER_PARAM are NULL;
	 */
	{
		inspectDebugState(GL_TRUE, 0 /* cb */, 0 /* info */);
	}

	/*
	 * Ignore spurious performance messages
	 */
	m_gl->debugMessageControl(GL_DONT_CARE /* source */, GL_DEBUG_TYPE_PERFORMANCE /* type */,
							  GL_DONT_CARE /* severity */, 0 /* counts */, 0 /* ids */, GL_FALSE /* enabled */);

	/*
	 * - insert a message with DebugMessageInsert;
	 * - inspect message log to check if the message is reported;
	 * - inspect message log again, there should be no messages;
	 */
	{
		GLchar  messageLog[bufSize];
		GLenum  sources[read_messages];
		GLenum  types[read_messages];
		GLuint  ids[read_messages];
		GLenum  severities[read_messages];
		GLsizei lengths[read_messages];

		cleanMessageLog(m_gl);

		m_gl->debugMessageInsert(GL_DEBUG_SOURCE_APPLICATION /* source */, GL_DEBUG_TYPE_ERROR /* type */, 11 /* id */,
								 GL_DEBUG_SEVERITY_HIGH /* severity */, label_length /* length */, label);
		GLU_EXPECT_NO_ERROR(m_gl->getError(), "DebugMessageInsert");

		GLuint ret = m_gl->getDebugMessageLog(read_messages /* count */, bufSize /* bufSize */, sources /* sources */,
											  types /* types */, ids /* ids */, severities /* severities */,
											  lengths /* lengths */, messageLog /* messageLog */);
		GLU_EXPECT_NO_ERROR(m_gl->getError(), "GetDebugMessageLog");

		if (1 != ret)
		{
			m_context.getTestContext().getLog() << tcu::TestLog::Message
												<< "GetDebugMessageLog returned invalid number of messages: " << ret
												<< ", expected 1" << tcu::TestLog::EndMessage;

			TCU_FAIL("Invalid value returned by GetDebugMessageLog");
		}

		if (GL_DEBUG_SOURCE_APPLICATION != sources[0])
		{
			TCU_FAIL("Invalid source value returned by GetDebugMessageLog");
		}

		if (GL_DEBUG_TYPE_ERROR != types[0])
		{
			TCU_FAIL("Invalid type value returned by GetDebugMessageLog");
		}

		if (11 != ids[0])
		{
			TCU_FAIL("Invalid id value returned by GetDebugMessageLog");
		}

		if (GL_DEBUG_SEVERITY_HIGH != severities[0])
		{
			TCU_FAIL("Invalid severity value returned by GetDebugMessageLog");
		}

		// DebugMessageInsert's length does not include null-terminated character (if length is positive)
		// But GetDebugMessageLog's length include null-terminated character
		// OpenGL 4.5 Core Spec, Page 530 and Page 535
		if (label_length + 1 != lengths[0])
		{
			TCU_FAIL("Invalid length value returned by GetDebugMessageLog");
		}

		if (0 != strcmp(label, messageLog))
		{
			TCU_FAIL("Invalid message returned by GetDebugMessageLog");
		}
	}

	/*
	 * - disable DEBUG_OUTPUT;
	 * - insert a message with DebugMessageInsert;
	 * - inspect message log again, there should be no messages;
	 */
	{
		m_gl->disable(GL_DEBUG_OUTPUT);
		GLU_EXPECT_NO_ERROR(m_gl->getError(), "Disable");

		m_gl->debugMessageInsert(GL_DEBUG_SOURCE_APPLICATION /* source */, GL_DEBUG_TYPE_ERROR /* type */, 11 /* id */,
								 GL_DEBUG_SEVERITY_HIGH /* severity */, label_length /* length */, label);
		GLU_EXPECT_NO_ERROR(m_gl->getError(), "DebugMessageInsert");

		inspectMessageLog(0);
	}

	/*
	 * - enable DEBUG_OUTPUT;
	 * - register debug message callback with DebugMessageCallback;
	 * - verify that the state of DEBUG_CALLBACK_FUNCTION and
	 * DEBUG_CALLBACK_USER_PARAM are correct;
	 * - insert a message with DebugMessageInsert;
	 * - it is expected that debug message callback will be executed for
	 * the message;
	 * - inspect message log to check there are no messages;
	 */
	{
		m_gl->enable(GL_DEBUG_OUTPUT);
		GLU_EXPECT_NO_ERROR(m_gl->getError(), "Enable");

		m_gl->debugMessageCallback(debug_proc, &callback_counter);
		GLU_EXPECT_NO_ERROR(m_gl->getError(), "DebugMessageCallback");

		inspectDebugState(GL_TRUE, debug_proc, &callback_counter);

		m_gl->debugMessageInsert(GL_DEBUG_SOURCE_APPLICATION /* source */, GL_DEBUG_TYPE_ERROR /* type */, 11 /* id */,
								 GL_DEBUG_SEVERITY_HIGH /* severity */, label_length /* length */, label);
		GLU_EXPECT_NO_ERROR(m_gl->getError(), "DebugMessageInsert");

		inspectCallbackCounter(callback_counter, 1);

		inspectMessageLog(0);
	}

	/*
	 * - disable DEBUG_OUTPUT;
	 * - insert a message with DebugMessageInsert;
	 * - debug message callback should not be called;
	 * - inspect message log to check there are no messages;
	 */
	{
		m_gl->disable(GL_DEBUG_OUTPUT);
		GLU_EXPECT_NO_ERROR(m_gl->getError(), "Disable");

		m_gl->debugMessageInsert(GL_DEBUG_SOURCE_APPLICATION /* source */, GL_DEBUG_TYPE_ERROR /* type */, 11 /* id */,
								 GL_DEBUG_SEVERITY_HIGH /* severity */, label_length /* length */, label);
		GLU_EXPECT_NO_ERROR(m_gl->getError(), "DebugMessageInsert");

		inspectCallbackCounter(callback_counter, 1);

		inspectMessageLog(0);
	}

	/*
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
	 */
	{
		m_gl->enable(GL_DEBUG_OUTPUT);
		GLU_EXPECT_NO_ERROR(m_gl->getError(), "Enable");

		m_gl->debugMessageControl(GL_DONT_CARE /* source */, GL_DEBUG_TYPE_ERROR /* type */,
								  GL_DONT_CARE /* severity */, 0 /* counts */, 0 /* ids */, GL_FALSE /* enabled */);

		m_gl->debugMessageControl(GL_DONT_CARE /* source */, GL_DONT_CARE /* type */,
								  GL_DEBUG_SEVERITY_HIGH /* severity */, 0 /* counts */, 0 /* ids */,
								  GL_FALSE /* enabled */);
		GLU_EXPECT_NO_ERROR(m_gl->getError(), "DebugMessageControl");

		m_gl->debugMessageInsert(GL_DEBUG_SOURCE_APPLICATION /* source */, GL_DEBUG_TYPE_ERROR /* type */, 11 /* id */,
								 GL_DEBUG_SEVERITY_MEDIUM /* severity */, label_length /* length */, label);
		GLU_EXPECT_NO_ERROR(m_gl->getError(), "DebugMessageInsert");

		m_gl->debugMessageInsert(GL_DEBUG_SOURCE_APPLICATION /* source */, GL_DEBUG_TYPE_OTHER /* type */, 11 /* id */,
								 GL_DEBUG_SEVERITY_HIGH /* severity */, label_length /* length */, label);
		GLU_EXPECT_NO_ERROR(m_gl->getError(), "DebugMessageInsert");

		m_gl->debugMessageInsert(GL_DEBUG_SOURCE_APPLICATION /* source */, GL_DEBUG_TYPE_OTHER /* type */, 11 /* id */,
								 GL_DEBUG_SEVERITY_LOW /* severity */, label_length /* length */, label);
		GLU_EXPECT_NO_ERROR(m_gl->getError(), "DebugMessageInsert");

		inspectCallbackCounter(callback_counter, 1);

		inspectMessageLog(0);
	}

	/*
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
	 */
	{
		m_gl->debugMessageCallback(0, 0);

		inspectDebugState(GL_TRUE, 0, 0);

		m_gl->debugMessageInsert(GL_DEBUG_SOURCE_APPLICATION /* source */, GL_DEBUG_TYPE_ERROR /* type */, 11 /* id */,
								 GL_DEBUG_SEVERITY_MEDIUM /* severity */, label_length /* length */, label);
		GLU_EXPECT_NO_ERROR(m_gl->getError(), "DebugMessageInsert");

		m_gl->debugMessageInsert(GL_DEBUG_SOURCE_APPLICATION /* source */, GL_DEBUG_TYPE_OTHER /* type */, 11 /* id */,
								 GL_DEBUG_SEVERITY_HIGH /* severity */, label_length /* length */, label);
		GLU_EXPECT_NO_ERROR(m_gl->getError(), "DebugMessageInsert");

		m_gl->debugMessageInsert(GL_DEBUG_SOURCE_APPLICATION /* source */, GL_DEBUG_TYPE_OTHER /* type */, 11 /* id */,
								 GL_DEBUG_SEVERITY_LOW /* severity */, label_length /* length */, label);
		GLU_EXPECT_NO_ERROR(m_gl->getError(), "DebugMessageInsert");

		inspectMessageLog(0);
	}

	/*
	 * - execute DebugMessageControl to enable messages of <type> DEBUG_TYPE_ERROR
	 * and <severity> DEBUG_SEVERITY_HIGH.
	 */
	{
		m_gl->debugMessageControl(GL_DONT_CARE /* source */, GL_DEBUG_TYPE_ERROR /* type */,
								  GL_DONT_CARE /* severity */, 0 /* counts */, 0 /* ids */, GL_TRUE /* enabled */);

		m_gl->debugMessageControl(GL_DONT_CARE /* source */, GL_DONT_CARE /* type */,
								  GL_DEBUG_SEVERITY_HIGH /* severity */, 0 /* counts */, 0 /* ids */,
								  GL_TRUE /* enabled */);
		GLU_EXPECT_NO_ERROR(m_gl->getError(), "DebugMessageControl");
	}

	/*
	 * - insert MAX_DEBUG_LOGGED_MESSAGES + 1 unique messages with
	 * DebugMessageInsert;
	 * - check state of DEBUG_LOGGED_MESSAGES; It is expected that
	 * MAX_DEBUG_LOGGED_MESSAGES will be reported;
	 */
	{
		for (GLint i = 0; i < max_debug_messages + 1; ++i)
		{
			m_gl->debugMessageInsert(GL_DEBUG_SOURCE_APPLICATION /* source */, GL_DEBUG_TYPE_ERROR /* type */,
									 i /* id */, GL_DEBUG_SEVERITY_MEDIUM /* severity */, label_length /* length */,
									 label);
			GLU_EXPECT_NO_ERROR(m_gl->getError(), "DebugMessageInsert");
		}

		GLint n_debug_messages = 0;

		m_gl->getIntegerv(GL_DEBUG_LOGGED_MESSAGES, &n_debug_messages);
		GLU_EXPECT_NO_ERROR(m_gl->getError(), "GetIntegerv");

		if (n_debug_messages != max_debug_messages)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "State of DEBUG_LOGGED_MESSAGES: " << n_debug_messages << ", expected "
				<< max_debug_messages << tcu::TestLog::EndMessage;

			TCU_FAIL("Invalid state of DEBUG_LOGGED_MESSAGES");
		}
	}

	/*
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
	 */
	if (1 != max_debug_messages)
	{
		GLint half_count	   = max_debug_messages / 2;
		GLint n_debug_messages = 0;
		GLint rest_count	   = max_debug_messages - half_count;

		GLsizei buf_size = (GLsizei)((half_count + 1) * (label_length + 1));

		std::vector<GLchar>  messageLog;
		std::vector<GLenum>  sources;
		std::vector<GLenum>  types;
		std::vector<GLuint>  ids;
		std::vector<GLenum>  severities;
		std::vector<GLsizei> lengths;

		messageLog.resize(buf_size);
		sources.resize(half_count + 1);
		types.resize(half_count + 1);
		ids.resize(half_count + 1);
		severities.resize(half_count + 1);
		lengths.resize(half_count + 1);

		GLuint ret = m_gl->getDebugMessageLog(half_count /* count */, buf_size /* bufSize */, &sources[0] /* sources */,
											  &types[0] /* types */, &ids[0] /* ids */, &severities[0] /* severities */,
											  &lengths[0] /* lengths */, &messageLog[0] /* messageLog */);
		GLU_EXPECT_NO_ERROR(m_gl->getError(), "GetDebugMessageLog");

		if (ret != (GLuint)half_count)
		{
			m_context.getTestContext().getLog() << tcu::TestLog::Message
												<< "GetDebugMessageLog returned unexpected number of messages: " << ret
												<< ", expected " << half_count << tcu::TestLog::EndMessage;

			TCU_FAIL("Invalid number of meessages");
		}

		m_gl->getIntegerv(GL_DEBUG_LOGGED_MESSAGES, &n_debug_messages);
		GLU_EXPECT_NO_ERROR(m_gl->getError(), "GetIntegerv");

		if (n_debug_messages != rest_count)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "State of DEBUG_LOGGED_MESSAGES: " << n_debug_messages << ", expected "
				<< rest_count << tcu::TestLog::EndMessage;

			TCU_FAIL("Invalid state of DEBUG_LOGGED_MESSAGES");
		}

		for (GLint i = 0; i < half_count; ++i)
		{
			if (GL_DEBUG_SOURCE_APPLICATION != sources[i])
			{
				TCU_FAIL("Invalid source value returned by GetDebugMessageLog");
			}

			if (GL_DEBUG_TYPE_ERROR != types[i])
			{
				TCU_FAIL("Invalid type value returned by GetDebugMessageLog");
			}

			if ((GLuint)i != ids[i])
			{
				TCU_FAIL("Invalid id value returned by GetDebugMessageLog");
			}

			if (GL_DEBUG_SEVERITY_MEDIUM != severities[i])
			{
				TCU_FAIL("Invalid severity value returned by GetDebugMessageLog");
			}

			// DebugMessageInsert's length does not include null-terminated character (if length is positive)
			// But GetDebugMessageLog's length include null-terminated character
			// OpenGL 4.5 Core Spec, Page 530 and Page 535
			if (label_length + 1 != lengths[i])
			{
				TCU_FAIL("Invalid length value returned by GetDebugMessageLog");
			}

			if (0 != strcmp(label, &messageLog[i * (label_length + 1)]))
			{
				TCU_FAIL("Invalid message returned by GetDebugMessageLog");
			}
		}

		/* */
		buf_size = (GLsizei)((rest_count - 1) * (label_length + 1) + label_length);
		memset(&messageLog[0], 0, messageLog.size());

		ret = m_gl->getDebugMessageLog(rest_count /* count */, buf_size /* bufSize */, &sources[0] /* sources */,
									   &types[0] /* types */, &ids[0] /* ids */, &severities[0] /* severities */,
									   &lengths[0] /* lengths */, &messageLog[0] /* messageLog */);
		GLU_EXPECT_NO_ERROR(m_gl->getError(), "GetDebugMessageLog");

		if (ret != (GLuint)(rest_count - 1))
		{
			m_context.getTestContext().getLog() << tcu::TestLog::Message
												<< "GetDebugMessageLog returned unexpected number of messages: " << ret
												<< ", expected " << (rest_count - 1) << tcu::TestLog::EndMessage;

			TCU_FAIL("Invalid number of meessages");
		}

		m_gl->getIntegerv(GL_DEBUG_LOGGED_MESSAGES, &n_debug_messages);
		GLU_EXPECT_NO_ERROR(m_gl->getError(), "GetIntegerv");

		if (n_debug_messages != 1)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "State of DEBUG_LOGGED_MESSAGES: " << n_debug_messages << ", expected "
				<< (rest_count - 1) << tcu::TestLog::EndMessage;

			TCU_FAIL("Invalid state of DEBUG_LOGGED_MESSAGES");
		}

		for (GLint i = 0; i < (rest_count - 1); ++i)
		{
			if (GL_DEBUG_SOURCE_APPLICATION != sources[i])
			{
				TCU_FAIL("Invalid source value returned by GetDebugMessageLog");
			}

			if (GL_DEBUG_TYPE_ERROR != types[i])
			{
				TCU_FAIL("Invalid type value returned by GetDebugMessageLog");
			}

			if ((GLuint)(i + half_count) != ids[i])
			{
				TCU_FAIL("Invalid id value returned by GetDebugMessageLog");
			}

			if (GL_DEBUG_SEVERITY_MEDIUM != severities[i])
			{
				TCU_FAIL("Invalid severity value returned by GetDebugMessageLog");
			}

			// DebugMessageInsert's length does not include null-terminated character (if length is positive)
			// But GetDebugMessageLog's length include null-terminated character
			// OpenGL 4.5 Core Spec, Page 530 and Page 535
			if (label_length + 1 != lengths[i])
			{
				TCU_FAIL("Invalid length value returned by GetDebugMessageLog");
			}

			if (0 != strcmp(label, &messageLog[i * (label_length + 1)]))
			{
				TCU_FAIL("Invalid message returned by GetDebugMessageLog");
			}
		}

		/* */
		memset(&messageLog[0], 0, messageLog.size());

		ret = m_gl->getDebugMessageLog(rest_count /* count */, buf_size /* bufSize */, &sources[0] /* sources */,
									   &types[0] /* types */, &ids[0] /* ids */, &severities[0] /* severities */,
									   &lengths[0] /* lengths */, &messageLog[0] /* messageLog */);
		GLU_EXPECT_NO_ERROR(m_gl->getError(), "GetDebugMessageLog");

		if (ret != 1)
		{
			m_context.getTestContext().getLog() << tcu::TestLog::Message
												<< "GetDebugMessageLog returned unexpected number of messages: " << ret
												<< ", expected 1" << tcu::TestLog::EndMessage;

			TCU_FAIL("Invalid number of meessages");
		}

		m_gl->getIntegerv(GL_DEBUG_LOGGED_MESSAGES, &n_debug_messages);
		GLU_EXPECT_NO_ERROR(m_gl->getError(), "GetIntegerv");

		if (n_debug_messages != 0)
		{
			m_context.getTestContext().getLog()
				<< tcu::TestLog::Message << "State of DEBUG_LOGGED_MESSAGES: " << n_debug_messages << ", expected 1"
				<< tcu::TestLog::EndMessage;

			TCU_FAIL("Invalid state of DEBUG_LOGGED_MESSAGES");
		}

		if (GL_DEBUG_SOURCE_APPLICATION != sources[0])
		{
			TCU_FAIL("Invalid source value returned by GetDebugMessageLog");
		}

		if (GL_DEBUG_TYPE_ERROR != types[0])
		{
			TCU_FAIL("Invalid type value returned by GetDebugMessageLog");
		}

		if ((GLuint)(max_debug_messages - 1) != ids[0])
		{
			TCU_FAIL("Invalid id value returned by GetDebugMessageLog");
		}

		if (GL_DEBUG_SEVERITY_MEDIUM != severities[0])
		{
			TCU_FAIL("Invalid severity value returned by GetDebugMessageLog");
		}

		// DebugMessageInsert's length does not include null-terminated character (if length is positive)
		// But GetDebugMessageLog's length include null-terminated character
		// OpenGL 4.5 Core Spec, Page 530 and Page 535
		if (label_length + 1 != lengths[0])
		{
			TCU_FAIL("Invalid length value returned by GetDebugMessageLog");
		}

		if (0 != strcmp(label, &messageLog[0]))
		{
			TCU_FAIL("Invalid message returned by GetDebugMessageLog");
		}
	}

	/* Set result */
	m_context.getTestContext().setTestResult(QP_TEST_RESULT_PASS, "Pass");

	/* Done */
	TestBase::done();

	return tcu::TestNode::STOP;
}

/** Debug callback used by the test, increase counter by one
 *
 * @param ignored
 * @param ignored
 * @param ignored
 * @param ignored
 * @param ignored
 * @param ignored
 * @param info    Pointer to uint counter
 **/
void ReceiveingMessagesTest::debug_proc(glw::GLenum /* source */, glw::GLenum /* type */, glw::GLuint /* id */,
										glw::GLenum /* severity */, glw::GLsizei /* length */,
										const glw::GLchar* /* message */, const void* info)
{
	GLuint* counter = (GLuint*)info;

	*counter += 1;
}

/** Inspects state of DEBUG_OUTPUT and debug callback
 *
 * @param expected_state     Expected state of DEBUG_OUTPUT
 * @param expected_callback  Expected state of DEBUG_CALLBACK_FUNCTION
 * @param expected_user_info Expected state of DEBUG_CALLBACK_USER_PARAM
 **/
void ReceiveingMessagesTest::inspectDebugState(GLboolean expected_state, GLDEBUGPROC expected_callback,
											   GLvoid* expected_user_info) const
{
	GLboolean debug_state = -1;
	m_gl->getBooleanv(GL_DEBUG_OUTPUT, &debug_state);
	GLU_EXPECT_NO_ERROR(m_gl->getError(), "GetBooleanv");

	if (expected_state != debug_state)
	{
		m_context.getTestContext().getLog() << tcu::TestLog::Message << "State of DEBUG_OUTPUT: " << debug_state
											<< ", expected " << expected_state << tcu::TestLog::EndMessage;

		TCU_FAIL("Invalid state of DEBUG_OUTPUT");
	}

	GLvoid* callback_procedure = 0;
	m_gl->getPointerv(GL_DEBUG_CALLBACK_FUNCTION, &callback_procedure);
	GLU_EXPECT_NO_ERROR(m_gl->getError(), "GetPointerv");

	if (expected_callback != callback_procedure)
	{
		TCU_FAIL("Invalid state of DEBUG_CALLBACK_FUNCTION");
	}

	GLvoid* callback_user_info = 0;
	m_gl->getPointerv(GL_DEBUG_CALLBACK_USER_PARAM, &callback_user_info);
	GLU_EXPECT_NO_ERROR(m_gl->getError(), "GetPointerv");

	if (expected_user_info != callback_user_info)
	{
		TCU_FAIL("Invalid state of DEBUG_CALLBACK_USER_PARAM");
	}
}

/** Inspects value of counter used by callback
 *
 * @param callback_counter            Reference to counter
 * @param expected_number_of_messages Expected value of counter
 **/
void ReceiveingMessagesTest::inspectCallbackCounter(GLuint& callback_counter, GLuint expected_number_of_messages) const
{
	m_gl->finish();
	GLU_EXPECT_NO_ERROR(m_gl->getError(), "Finish");

	if (expected_number_of_messages != callback_counter)
	{
		m_context.getTestContext().getLog()
			<< tcu::TestLog::Message << "Debug callback was executed invalid number of times: " << callback_counter
			<< ", expected " << expected_number_of_messages << tcu::TestLog::EndMessage;

		TCU_FAIL("Invalid execution of debug callback");
	}
}

/** Inspects amount of messages stored in log
 *
 * @param expected_number_of_messages Expected number of messages
 **/
void ReceiveingMessagesTest::inspectMessageLog(GLuint expected_number_of_messages) const
{
	static const size_t bufSize		  = 32;
	static const size_t read_messages = 4;

	GLchar  messageLog[bufSize];
	GLenum  sources[read_messages];
	GLenum  types[read_messages];
	GLuint  ids[read_messages];
	GLenum  severities[read_messages];
	GLsizei lengths[read_messages];

	GLuint ret = m_gl->getDebugMessageLog(read_messages /* count */, bufSize /* bufSize */, sources /* sources */,
										  types /* types */, ids /* ids */, severities /* severities */,
										  lengths /* lengths */, messageLog /* messageLog */);
	GLU_EXPECT_NO_ERROR(m_gl->getError(), "GetDebugMessageLog");

	if (expected_number_of_messages != ret)
	{
		m_context.getTestContext().getLog() << tcu::TestLog::Message
											<< "GetDebugMessageLog returned invalid number of messages: " << ret
											<< ", expected " << expected_number_of_messages << tcu::TestLog::EndMessage;

		TCU_FAIL("Invalid value returned by GetDebugMessageLog");
	}
}

/** Constructor
 *
 * @param context  Test context
 * @param is_debug Selects if debug or non-debug context should be used
 * @param name     Name of test
 **/
GroupsTest::GroupsTest(deqp::Context& context)
	: TestCase(context, "groups", "Verifies that groups can be used to control generated messages")
	, TestBase(context, true /* is_debug */)
{
	/* Nothing to be done */
}

/** Execute test
 *
 * @return tcu::TestNode::STOP
 **/
tcu::TestNode::IterateResult GroupsTest::iterate()
{
	static const GLchar label[]		 = "foo";
	static const size_t label_length = sizeof(label) / sizeof(label[0]) - 1;

	/* Initialize render context */
	TestBase::init();

	cleanMessageLog(m_gl);

	/*
	 * - check state of DEBUG_GROUP_STACK_DEPTH; It should be 1;
	 */
	inspectGroupStack(1);

	/*
	 * - insert message with <type> DEBUG_TYPE_ERROR;
	 * - inspect message log to check if the message is reported;
	 * - insert message with <type> DEBUG_TYPE_OTHER;
	 * - inspect message log to check if the message is reported;
	 */
	{
		m_gl->debugMessageInsert(GL_DEBUG_SOURCE_APPLICATION /* source */, GL_DEBUG_TYPE_ERROR /* type */, 11 /* id */,
								 GL_DEBUG_SEVERITY_HIGH /* severity */, label_length /* length */, label);
		GLU_EXPECT_NO_ERROR(m_gl->getError(), "DebugMessageInsert");

		inspectMessageLog(GL_DEBUG_SOURCE_APPLICATION /* source */, GL_DEBUG_TYPE_ERROR /* type */, 11 /* id */,
						  GL_DEBUG_SEVERITY_HIGH /* severity */, label_length /* length */, label);

		m_gl->debugMessageInsert(GL_DEBUG_SOURCE_APPLICATION /* source */, GL_DEBUG_TYPE_OTHER /* type */, 11 /* id */,
								 GL_DEBUG_SEVERITY_HIGH /* severity */, label_length /* length */, label);
		GLU_EXPECT_NO_ERROR(m_gl->getError(), "DebugMessageInsert");

		inspectMessageLog(GL_DEBUG_SOURCE_APPLICATION /* source */, GL_DEBUG_TYPE_OTHER /* type */, 11 /* id */,
						  GL_DEBUG_SEVERITY_HIGH /* severity */, label_length /* length */, label);
	}

	/*
	 * - push debug group with unique <id> and <message>;
	 * - inspect message log to check if the message about push is reported;
	 * - disable messages with <type> DEBUG_TYPE_ERROR;
	 * - insert message with <type> DEBUG_TYPE_ERROR;
	 * - inspect message log to check there are no messages;
	 * - insert message with <type> DEBUG_TYPE_OTHER;
	 * - inspect message log to check if the message is reported;
	 */
	{
		m_gl->pushDebugGroup(GL_DEBUG_SOURCE_APPLICATION /* source */, 0xabcd0123 /* id */, -1 /* length */, label);
		GLU_EXPECT_NO_ERROR(m_gl->getError(), "PushDebugGroup");

		inspectMessageLog(GL_DEBUG_SOURCE_APPLICATION /* source */, GL_DEBUG_TYPE_PUSH_GROUP /* type */,
						  0xabcd0123 /* id */, GL_DEBUG_SEVERITY_NOTIFICATION /* severity */, label_length /* length */,
						  label);

		m_gl->debugMessageControl(GL_DONT_CARE /* source */, GL_DEBUG_TYPE_ERROR /* type */,
								  GL_DONT_CARE /* severity */, 0 /* counts */, 0 /* ids */, GL_FALSE /* enabled */);
		GLU_EXPECT_NO_ERROR(m_gl->getError(), "DebugMessageControl");

		m_gl->debugMessageInsert(GL_DEBUG_SOURCE_APPLICATION /* source */, GL_DEBUG_TYPE_ERROR /* type */, 11 /* id */,
								 GL_DEBUG_SEVERITY_HIGH /* severity */, label_length /* length */, label);
		GLU_EXPECT_NO_ERROR(m_gl->getError(), "DebugMessageInsert");

		verifyEmptyLog();

		m_gl->debugMessageInsert(GL_DEBUG_SOURCE_APPLICATION /* source */, GL_DEBUG_TYPE_OTHER /* type */, 11 /* id */,
								 GL_DEBUG_SEVERITY_HIGH /* severity */, label_length /* length */, label);
		GLU_EXPECT_NO_ERROR(m_gl->getError(), "DebugMessageInsert");

		inspectMessageLog(GL_DEBUG_SOURCE_APPLICATION /* source */, GL_DEBUG_TYPE_OTHER /* type */, 11 /* id */,
						  GL_DEBUG_SEVERITY_HIGH /* severity */, label_length /* length */, label);
	}

	/*
	 * - check state of DEBUG_GROUP_STACK_DEPTH; It should be 2;
	 */
	inspectGroupStack(2);

	/*
	 * - push debug group with unique <id> and <message>;
	 * - inspect message log to check if the message about push is reported;
	 * - disable messages with <type> DEBUG_TYPE_OTHER;
	 * - insert message with <type> DEBUG_TYPE_ERROR;
	 * - inspect message log to check there are no messages;
	 * - insert message with <type> DEBUG_TYPE_OTHER;
	 * - inspect message log to check there are no messages;
	 */
	{
		m_gl->pushDebugGroup(GL_DEBUG_SOURCE_APPLICATION /* source */, 0x0123abcd /* id */, -1 /* length */, label);
		GLU_EXPECT_NO_ERROR(m_gl->getError(), "PushDebugGroup");

		inspectMessageLog(GL_DEBUG_SOURCE_APPLICATION /* source */, GL_DEBUG_TYPE_PUSH_GROUP /* type */,
						  0x0123abcd /* id */, GL_DEBUG_SEVERITY_NOTIFICATION /* severity */, label_length /* length */,
						  label);

		m_gl->debugMessageControl(GL_DONT_CARE /* source */, GL_DEBUG_TYPE_OTHER /* type */,
								  GL_DONT_CARE /* severity */, 0 /* counts */, 0 /* ids */, GL_FALSE /* enabled */);
		GLU_EXPECT_NO_ERROR(m_gl->getError(), "DebugMessageControl");

		m_gl->debugMessageInsert(GL_DEBUG_SOURCE_APPLICATION /* source */, GL_DEBUG_TYPE_ERROR /* type */, 11 /* id */,
								 GL_DEBUG_SEVERITY_HIGH /* severity */, label_length /* length */, label);
		GLU_EXPECT_NO_ERROR(m_gl->getError(), "DebugMessageInsert");

		verifyEmptyLog();

		m_gl->debugMessageInsert(GL_DEBUG_SOURCE_APPLICATION /* source */, GL_DEBUG_TYPE_OTHER /* type */, 11 /* id */,
								 GL_DEBUG_SEVERITY_HIGH /* severity */, label_length /* length */, label);
		GLU_EXPECT_NO_ERROR(m_gl->getError(), "DebugMessageInsert");

		verifyEmptyLog();
	}

	/*
	 * - check state of DEBUG_GROUP_STACK_DEPTH; It should be 3;
	 */
	inspectGroupStack(3);

	/*
	 * - pop debug group;
	 * - inspect message log to check if the message about pop is reported and
	 * corresponds with the second push;
	 * - insert message with <type> DEBUG_TYPE_ERROR;
	 * - inspect message log to check there are no messages;
	 * - insert message with <type> DEBUG_TYPE_OTHER;
	 * - inspect message log to check if the message is reported;
	 */
	{
		m_gl->popDebugGroup();
		GLU_EXPECT_NO_ERROR(m_gl->getError(), "PopDebugGroup");

		inspectMessageLog(GL_DEBUG_SOURCE_APPLICATION /* source */, GL_DEBUG_TYPE_POP_GROUP /* type */,
						  0x0123abcd /* id */, GL_DEBUG_SEVERITY_NOTIFICATION /* severity */, label_length /* length */,
						  label);

		m_gl->debugMessageInsert(GL_DEBUG_SOURCE_APPLICATION /* source */, GL_DEBUG_TYPE_ERROR /* type */, 11 /* id */,
								 GL_DEBUG_SEVERITY_HIGH /* severity */, label_length /* length */, label);
		GLU_EXPECT_NO_ERROR(m_gl->getError(), "DebugMessageInsert");

		verifyEmptyLog();

		m_gl->debugMessageInsert(GL_DEBUG_SOURCE_APPLICATION /* source */, GL_DEBUG_TYPE_OTHER /* type */, 11 /* id */,
								 GL_DEBUG_SEVERITY_HIGH /* severity */, label_length /* length */, label);
		GLU_EXPECT_NO_ERROR(m_gl->getError(), "DebugMessageInsert");

		inspectMessageLog(GL_DEBUG_SOURCE_APPLICATION /* source */, GL_DEBUG_TYPE_OTHER /* type */, 11 /* id */,
						  GL_DEBUG_SEVERITY_HIGH /* severity */, label_length /* length */, label);
	}

	/*
	 * - check state of DEBUG_GROUP_STACK_DEPTH; It should be 2;
	 */
	inspectGroupStack(2);

	/*
	 * - pop debug group;
	 * - inspect message log to check if the message about pop is reported and
	 * corresponds with the first push;
	 * - insert message with <type> DEBUG_TYPE_ERROR;
	 * - inspect message log to check if the message is reported;
	 * - insert message with <type> DEBUG_TYPE_OTHER;
	 * - inspect message log to check if the message is reported;
	 */
	{
		m_gl->popDebugGroup();
		GLU_EXPECT_NO_ERROR(m_gl->getError(), "PopDebugGroup");

		inspectMessageLog(GL_DEBUG_SOURCE_APPLICATION /* source */, GL_DEBUG_TYPE_POP_GROUP /* type */,
						  0xabcd0123 /* id */, GL_DEBUG_SEVERITY_NOTIFICATION /* severity */, label_length /* length */,
						  label);

		m_gl->debugMessageInsert(GL_DEBUG_SOURCE_APPLICATION /* source */, GL_DEBUG_TYPE_ERROR /* type */, 11 /* id */,
								 GL_DEBUG_SEVERITY_HIGH /* severity */, label_length /* length */, label);
		GLU_EXPECT_NO_ERROR(m_gl->getError(), "DebugMessageInsert");

		inspectMessageLog(GL_DEBUG_SOURCE_APPLICATION /* source */, GL_DEBUG_TYPE_ERROR /* type */, 11 /* id */,
						  GL_DEBUG_SEVERITY_HIGH /* severity */, label_length /* length */, label);

		m_gl->debugMessageInsert(GL_DEBUG_SOURCE_APPLICATION /* source */, GL_DEBUG_TYPE_OTHER /* type */, 11 /* id */,
								 GL_DEBUG_SEVERITY_HIGH /* severity */, label_length /* length */, label);
		GLU_EXPECT_NO_ERROR(m_gl->getError(), "DebugMessageInsert");

		inspectMessageLog(GL_DEBUG_SOURCE_APPLICATION /* source */, GL_DEBUG_TYPE_OTHER /* type */, 11 /* id */,
						  GL_DEBUG_SEVERITY_HIGH /* severity */, label_length /* length */, label);
	}

	/*
	 * - check state of DEBUG_GROUP_STACK_DEPTH; It should be 1;
	 */
	inspectGroupStack(1);

	/* Set result */
	m_context.getTestContext().setTestResult(QP_TEST_RESULT_PASS, "Pass");

	/* Done */
	TestBase::done();

	return tcu::TestNode::STOP;
}

/** Inspects amount of groups on stack
 *
 * @param expected_depth Expected number of groups
 **/
void GroupsTest::inspectGroupStack(GLuint expected_depth) const
{
	GLint stack_depth = 0;

	m_gl->getIntegerv(GL_DEBUG_GROUP_STACK_DEPTH, &stack_depth);
	GLU_EXPECT_NO_ERROR(m_gl->getError(), "GetIntegerv");

	if (expected_depth != (GLuint)stack_depth)
	{
		m_context.getTestContext().getLog() << tcu::TestLog::Message
											<< "State of DEBUG_GROUP_STACK_DEPTH: " << stack_depth << ", expected "
											<< expected_depth << tcu::TestLog::EndMessage;

		TCU_FAIL("Invalid state of DEBUG_GROUP_STACK_DEPTH");
	}
}

/** Inspects first message stored in log
 *
 * @param expected_source   Expected source of messages
 * @param expected_type     Expected type of messages
 * @param expected_id       Expected id of messages
 * @param expected_severity Expected severity of messages
 * @param expected_length   Expected length of messages
 * @param expected_label    Expected label of messages
 **/
void GroupsTest::inspectMessageLog(glw::GLenum expected_source, glw::GLenum expected_type, glw::GLuint expected_id,
								   glw::GLenum expected_severity, glw::GLsizei expected_length,
								   const glw::GLchar* expected_label) const
{
	static const size_t bufSize		  = 32;
	static const size_t read_messages = 1;

	GLchar  messageLog[bufSize];
	GLenum  source;
	GLenum  type;
	GLuint  id;
	GLenum  severity;
	GLsizei length;

	GLuint ret = m_gl->getDebugMessageLog(read_messages /* count */, bufSize /* bufSize */, &source /* sources */,
										  &type /* types */, &id /* ids */, &severity /* severities */,
										  &length /* lengths */, messageLog /* messageLog */);
	GLU_EXPECT_NO_ERROR(m_gl->getError(), "GetDebugMessageLog");

	if (0 == ret)
	{
		TCU_FAIL("GetDebugMessageLog returned 0 messages");
	}

	if (expected_source != source)
	{
		m_context.getTestContext().getLog() << tcu::TestLog::Message << "Got message with invalid source: " << source
											<< ", expected " << expected_source << tcu::TestLog::EndMessage;

		TCU_FAIL("Invalid source of message");
	}

	if (expected_type != type)
	{
		m_context.getTestContext().getLog() << tcu::TestLog::Message << "Got message with invalid type: " << type
											<< ", expected " << expected_type << tcu::TestLog::EndMessage;

		TCU_FAIL("Invalid type of message");
	}

	if (expected_id != id)
	{
		m_context.getTestContext().getLog() << tcu::TestLog::Message << "Got message with invalid id: " << id
											<< ", expected " << expected_id << tcu::TestLog::EndMessage;

		TCU_FAIL("Invalid id of message");
	}

	if (expected_severity != severity)
	{
		m_context.getTestContext().getLog() << tcu::TestLog::Message
											<< "Got message with invalid severity: " << severity << ", expected "
											<< expected_severity << tcu::TestLog::EndMessage;

		TCU_FAIL("Invalid severity of message");
	}

	// DebugMessageInsert's length does not include null-terminated character (if length is positive)
	// But GetDebugMessageLog's length include null-terminated character
	// OpenGL 4.5 Core Spec, Page 530 and Page 535
	if (expected_length + 1 != length)
	{
		m_context.getTestContext().getLog() << tcu::TestLog::Message << "Got message with invalid length: " << length
											<< ", expected " << expected_length << tcu::TestLog::EndMessage;

		TCU_FAIL("Invalid length of message");
	}

	if (0 != strcmp(expected_label, messageLog))
	{
		m_context.getTestContext().getLog() << tcu::TestLog::Message
											<< "Got message with invalid message: " << messageLog << ", expected "
											<< expected_label << tcu::TestLog::EndMessage;

		TCU_FAIL("Invalid message");
	}
}

/** Verifies that message log is empty
 *
 **/
void GroupsTest::verifyEmptyLog() const
{
	static const size_t bufSize		  = 32;
	static const size_t read_messages = 1;

	GLchar  messageLog[bufSize];
	GLenum  source;
	GLenum  type;
	GLuint  id;
	GLenum  severity;
	GLsizei length;

	GLuint ret = m_gl->getDebugMessageLog(read_messages /* count */, bufSize /* bufSize */, &source /* sources */,
										  &type /* types */, &id /* ids */, &severity /* severities */,
										  &length /* lengths */, messageLog /* messageLog */);
	GLU_EXPECT_NO_ERROR(m_gl->getError(), "GetDebugMessageLog");

	if (0 != ret)
	{
		TCU_FAIL("GetDebugMessageLog returned unexpected messages");
	}
}

/** Constructor
 *
 * @param context  Test context
 * @param is_debug Selects if debug or non-debug context should be used
 * @param name     Name of test
 **/
SynchronousCallsTest::SynchronousCallsTest(deqp::Context& context)
	: TestCase(context, "synchronous_calls", "Verifies that messages can be received")
	, TestBase(context, true /* is_debug */)
{
	/* Create pthread_key_t visible to all threads
	 * The key has value NULL associated with it in all existing
	 * or about to be created threads
	 */
	m_uid = 0;
}

/** Execute test
 *
 * @return tcu::TestNode::STOP
 **/
tcu::TestNode::IterateResult SynchronousCallsTest::iterate()
{
	m_uid++;

	/* associate  some unique id with the current thread */
	m_tls.set((void*)(deUintptr)m_uid);

	static const GLchar label[] = "foo";

	GLuint buffer_id = 0;

	/* Initialize render context */
	TestBase::init();

	/* - set callback_executed to 0; */
	int callback_executed = 0;

	/*
	 *- enable DEBUG_OUTPUT_SYNCHRONOUS;
	 */
	m_gl->enable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
	GLU_EXPECT_NO_ERROR(m_gl->getError(), "Enable");

	/*
	 * - register debug message callback with DebugMessageCallback; Provide the
	 * instance of UserParam structure as <userParam>; Routine should do the
	 * following:
	 *   * set callback_executed to 1;
	 */
	m_gl->debugMessageCallback(debug_proc, &callback_executed);
	try
	{
		GLU_EXPECT_NO_ERROR(m_gl->getError(), "DebugMessageCallback");

		/*
		 * - insert a message with DebugMessageInsert;
		 */
		m_gl->debugMessageInsert(GL_DEBUG_SOURCE_APPLICATION /* source */, GL_DEBUG_TYPE_ERROR /* type */, 11 /* id */,
								 GL_DEBUG_SEVERITY_HIGH /* severity */, -1 /* length */, label);
		GLU_EXPECT_NO_ERROR(m_gl->getError(), "DebugMessageInsert");

		/* Make sure execution finished before we check results */
		m_gl->finish();
		GLU_EXPECT_NO_ERROR(m_gl->getError(), "Finish");

		/*
		 * - check if:
		 *   * callback_executed is set to 1;
		 */
		if (1 != callback_executed)
		{
			TCU_FAIL("callback_executed is not set to 1");
		}

		/* Check that the message was recorded by the current thread */
		if ((deUintptr)(m_tls.get()) != m_uid)
		{
			TCU_FAIL("thread id stored by callback is not the same as \"test\" thread");
		}

		/*
		 * - reset callback_executed;
		 */
		callback_executed = 0;

		/*
		 * - execute BindBufferBase with GL_ARRAY_BUFFER <target>, GL_INVALID_ENUM
		 * error should be generated;
		 */
		m_gl->genBuffers(1, &buffer_id);
		GLU_EXPECT_NO_ERROR(m_gl->getError(), "GenBuffers");

		m_gl->bindBufferBase(GL_ARRAY_BUFFER, 0 /* index */, buffer_id);
		if (GL_INVALID_ENUM != m_gl->getError())
		{
			TCU_FAIL("Unexpected error generated");
		}

		/* Make sure execution finished before we check results */
		m_gl->finish();
		GLU_EXPECT_NO_ERROR(m_gl->getError(), "Finish");

		/*
		 * - test pass if:
		 *   * callback_executed is set to 0 - implementation does not send messages;
		 *   * callback_executed is set to 1 and thread_id is the same
		 *   as "test" thread - implementation sent message to proper thread;
		 */
		if (1 == callback_executed)
		{
			/* Check that the error was recorded by the current thread */
			if ((deUintptr)(m_tls.get()) != m_uid)
			{
				TCU_FAIL("thread id stored by callback is not the same as \"test\" thread");
			}
		}

		/* Clean */
		m_gl->deleteBuffers(1, &buffer_id);
		buffer_id = 0;
	}
	catch (const std::exception& exc)
	{
		if (0 != buffer_id)
		{
			m_gl->deleteBuffers(1, &buffer_id);
			buffer_id = 0;
		}

		TCU_FAIL(exc.what());
	}

	/* Set result */
	m_context.getTestContext().setTestResult(QP_TEST_RESULT_PASS, "Pass");

	/* Done */
	TestBase::done();

	return tcu::TestNode::STOP;
}

/** Destructor */
SynchronousCallsTest::~SynchronousCallsTest(void)
{
}

/** Debug callback used by the test, sets callback_executed to 1 and stores 0 to tls
 *
 * @param ignored
 * @param ignored
 * @param ignored
 * @param ignored
 * @param ignored
 * @param ignored
 * @param info    Pointer to uint counter
 **/
void SynchronousCallsTest::debug_proc(glw::GLenum /* source */, glw::GLenum /* type */, glw::GLuint /* id */,
									  glw::GLenum /* severity */, glw::GLsizei /* length */,
									  const glw::GLchar* /* message */, const void* info)
{
	int* callback_executed = (int*)info;

	*callback_executed = 1;
}
} /* KHRDebug */

/** Constructor.
 *
 *  @param context Rendering context.
 **/
KHRDebugTests::KHRDebugTests(deqp::Context& context)
	: TestCaseGroup(context, "khr_debug", "Verifies \"khr debug\" functionality")
{
	/* Left blank on purpose */
}

/** Initializes a khr_debug test group.
 *
 **/
void KHRDebugTests::init(void)
{
	addChild(new KHRDebug::APIErrorsTest(m_context, false, "api_errors_non_debug"));
	addChild(new KHRDebug::LabelsTest(m_context, false, "labels_non_debug"));
	addChild(new KHRDebug::ReceiveingMessagesTest(m_context));
	addChild(new KHRDebug::GroupsTest(m_context));
	addChild(new KHRDebug::APIErrorsTest(m_context, true, "api_errors_debug"));
	addChild(new KHRDebug::LabelsTest(m_context, true, "labels_debug"));
	addChild(new KHRDebug::SynchronousCallsTest(m_context));
}

} /* gl4cts namespace */
