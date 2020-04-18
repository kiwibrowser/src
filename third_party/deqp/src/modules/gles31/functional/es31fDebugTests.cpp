/*-------------------------------------------------------------------------
 * drawElements Quality Program OpenGL ES 3.1 Module
 * -------------------------------------------------
 *
 * Copyright 2014 The Android Open Source Project
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
 *//*!
 * \file
 * \brief Debug output (KHR_debug) tests
 *//*--------------------------------------------------------------------*/

#include "es31fDebugTests.hpp"

#include "es31fNegativeTestShared.hpp"
#include "es31fNegativeBufferApiTests.hpp"
#include "es31fNegativeTextureApiTests.hpp"
#include "es31fNegativeShaderApiTests.hpp"
#include "es31fNegativeFragmentApiTests.hpp"
#include "es31fNegativeVertexArrayApiTests.hpp"
#include "es31fNegativeStateApiTests.hpp"
#include "es31fNegativeAtomicCounterTests.hpp"
#include "es31fNegativeShaderImageLoadStoreTests.hpp"
#include "es31fNegativeShaderFunctionTests.hpp"
#include "es31fNegativeShaderDirectiveTests.hpp"
#include "es31fNegativeSSBOBlockTests.hpp"
#include "es31fNegativePreciseTests.hpp"
#include "es31fNegativeAdvancedBlendEquationTests.hpp"
#include "es31fNegativeShaderStorageTests.hpp"
#include "es31fNegativeTessellationTests.hpp"
#include "es31fNegativeComputeTests.hpp"
#include "es31fNegativeSampleVariablesTests.hpp"
#include "es31fNegativeShaderFramebufferFetchTests.hpp"

#include "deUniquePtr.hpp"
#include "deRandom.hpp"
#include "deStringUtil.hpp"
#include "deSTLUtil.hpp"
#include "deMutex.hpp"
#include "deThread.h"

#include "gluRenderContext.hpp"
#include "gluContextInfo.hpp"
#include "gluCallLogWrapper.hpp"
#include "gluStrUtil.hpp"

#include "glwDefs.hpp"
#include "glwEnums.hpp"
#include "glwFunctions.hpp"

#include "tes31Context.hpp"
#include "tcuTestContext.hpp"
#include "tcuCommandLine.hpp"
#include "tcuResultCollector.hpp"

#include "glsStateQueryUtil.hpp"

namespace deqp
{
namespace gles31
{
namespace Functional
{
namespace
{
using namespace glw;

using std::string;
using std::vector;
using std::set;
using std::map;
using de::MovePtr;

using tcu::ResultCollector;
using tcu::TestLog;
using glu::CallLogWrapper;

using NegativeTestShared::NegativeTestContext;

static const GLenum s_debugTypes[] =
{
	GL_DEBUG_TYPE_ERROR,
	GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR,
	GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR,
	GL_DEBUG_TYPE_PORTABILITY,
	GL_DEBUG_TYPE_PERFORMANCE,
	GL_DEBUG_TYPE_OTHER,
	GL_DEBUG_TYPE_MARKER,
	GL_DEBUG_TYPE_PUSH_GROUP,
	GL_DEBUG_TYPE_POP_GROUP,
};

static const GLenum s_debugSeverities[] =
{
	GL_DEBUG_SEVERITY_HIGH,
    GL_DEBUG_SEVERITY_MEDIUM,
    GL_DEBUG_SEVERITY_LOW,
    GL_DEBUG_SEVERITY_NOTIFICATION,
};

static bool isKHRDebugSupported (Context& ctx)
{
	const bool supportsES32 = glu::contextSupports(ctx.getRenderContext().getType(), glu::ApiType::es(3, 2));
	return supportsES32 || ctx.getContextInfo().isExtensionSupported("GL_KHR_debug");
}

class BaseCase;

class DebugMessageTestContext : public NegativeTestContext
{
public:
				DebugMessageTestContext		(BaseCase&					host,
											 glu::RenderContext&		renderCtx,
											 const glu::ContextInfo&	ctxInfo,
											 tcu::TestLog&				log,
											 tcu::ResultCollector&		results,
											 bool						enableLog);
				~DebugMessageTestContext	(void);

	void		expectMessage				(GLenum source, GLenum type);

private:
	BaseCase&	m_debugHost;
};

class TestFunctionWrapper
{
public:
	typedef void (*CoreTestFunc)(NegativeTestContext& ctx);
	typedef void (*DebugTestFunc)(DebugMessageTestContext& ctx);

				TestFunctionWrapper	(void);
	explicit	TestFunctionWrapper	(CoreTestFunc func);
	explicit	TestFunctionWrapper	(DebugTestFunc func);

	void		call				(DebugMessageTestContext& ctx) const;

private:
	enum FuncType
	{
		TYPE_NULL = 0,
		TYPE_CORE,
		TYPE_DEBUG,
	};
	FuncType m_type;

	union
	{
		CoreTestFunc	coreFn;
		DebugTestFunc	debugFn;
	} m_func;
};

TestFunctionWrapper::TestFunctionWrapper (void)
	: m_type(TYPE_NULL)
{
}

TestFunctionWrapper::TestFunctionWrapper (CoreTestFunc func)
	: m_type(TYPE_CORE)
{
	m_func.coreFn = func;
}

TestFunctionWrapper::TestFunctionWrapper (DebugTestFunc func)
	: m_type(TYPE_DEBUG)
{
	m_func.debugFn = func;
}

void TestFunctionWrapper::call (DebugMessageTestContext& ctx) const
{
	if (m_type == TYPE_CORE)
		m_func.coreFn(static_cast<NegativeTestContext&>(ctx));
	else if (m_type == TYPE_DEBUG)
		m_func.debugFn(ctx);
	else
		DE_ASSERT(false);
}

void emitMessages (DebugMessageTestContext& ctx, GLenum source)
{
	for (int typeNdx = 0; typeNdx < DE_LENGTH_OF_ARRAY(s_debugTypes); typeNdx++)
	{
		for (int severityNdx = 0; severityNdx < DE_LENGTH_OF_ARRAY(s_debugSeverities); severityNdx++)
		{
			const GLenum type		= s_debugTypes[typeNdx];
			const GLenum severity	= s_debugSeverities[severityNdx];
			const string msg		= string("Application generated message with type ") + glu::getDebugMessageTypeName(type)
									  + " and severity " + glu::getDebugMessageSeverityName(severity);

			// Use severity as ID, guaranteed unique
			ctx.glDebugMessageInsert(source, type, severity, severity, -1, msg.c_str());
			ctx.expectMessage(source, type);
		}
	}
}

void application_messages (DebugMessageTestContext& ctx)
{
	ctx.beginSection("Messages with source of GL_DEBUG_SOURCE_APPLICATION");
	emitMessages(ctx, GL_DEBUG_SOURCE_APPLICATION);
	ctx.endSection();
}

void thirdparty_messages (DebugMessageTestContext& ctx)
{
	ctx.beginSection("Messages with source of GL_DEBUG_SOURCE_THIRD_PARTY");
	emitMessages(ctx, GL_DEBUG_SOURCE_THIRD_PARTY);
	ctx.endSection();
}

void push_pop_messages (DebugMessageTestContext& ctx)
{
	ctx.beginSection("Push/Pop Debug Group");

	ctx.glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 1, -1, "Application group 1");
	ctx.expectMessage(GL_DEBUG_SOURCE_APPLICATION, GL_DEBUG_TYPE_PUSH_GROUP);
	ctx.glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 2, -1, "Application group 1-1");
	ctx.expectMessage(GL_DEBUG_SOURCE_APPLICATION, GL_DEBUG_TYPE_PUSH_GROUP);
	ctx.glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 3, -1, "Application group 1-1-1");
	ctx.expectMessage(GL_DEBUG_SOURCE_APPLICATION, GL_DEBUG_TYPE_PUSH_GROUP);
	ctx.glPopDebugGroup();
	ctx.expectMessage(GL_DEBUG_SOURCE_APPLICATION, GL_DEBUG_TYPE_POP_GROUP);
	ctx.glPopDebugGroup();
	ctx.expectMessage(GL_DEBUG_SOURCE_APPLICATION, GL_DEBUG_TYPE_POP_GROUP);

	ctx.glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 4, -1, "Application group 1-2");
	ctx.expectMessage(GL_DEBUG_SOURCE_APPLICATION, GL_DEBUG_TYPE_PUSH_GROUP);
	ctx.glPopDebugGroup();
	ctx.expectMessage(GL_DEBUG_SOURCE_APPLICATION, GL_DEBUG_TYPE_POP_GROUP);

	ctx.glPushDebugGroup(GL_DEBUG_SOURCE_THIRD_PARTY, 4, -1, "3rd Party group 1-3");
	ctx.expectMessage(GL_DEBUG_SOURCE_THIRD_PARTY, GL_DEBUG_TYPE_PUSH_GROUP);
	ctx.glPopDebugGroup();
	ctx.expectMessage(GL_DEBUG_SOURCE_THIRD_PARTY, GL_DEBUG_TYPE_POP_GROUP);
	ctx.glPopDebugGroup();
	ctx.expectMessage(GL_DEBUG_SOURCE_APPLICATION, GL_DEBUG_TYPE_POP_GROUP);

	ctx.glPushDebugGroup(GL_DEBUG_SOURCE_THIRD_PARTY, 4, -1, "3rd Party group 2");
	ctx.expectMessage(GL_DEBUG_SOURCE_THIRD_PARTY, GL_DEBUG_TYPE_PUSH_GROUP);
	ctx.glPopDebugGroup();
	ctx.expectMessage(GL_DEBUG_SOURCE_THIRD_PARTY, GL_DEBUG_TYPE_POP_GROUP);

	ctx.endSection();
}

struct FunctionContainer
{
	TestFunctionWrapper	function;
	const char*			name;
	const char*			desc;
};

vector<FunctionContainer> getUserMessageFuncs (void)
{
	FunctionContainer funcs[] =
	{
		{ TestFunctionWrapper(application_messages),	"application_messages", "Externally generated messages from the application"	},
		{ TestFunctionWrapper(thirdparty_messages),		"third_party_messages",	"Externally generated messages from a third party"		},
		{ TestFunctionWrapper(push_pop_messages),		"push_pop_stack",		"Messages from pushing/popping debug groups"			},
	};

	return std::vector<FunctionContainer>(DE_ARRAY_BEGIN(funcs), DE_ARRAY_END(funcs));
}

// Data required to uniquely identify a debug message
struct MessageID
{
	GLenum source;
	GLenum type;
	GLuint id;

	MessageID (void) : source(GL_NONE), type(GL_NONE), id(0) {}
	MessageID (GLenum source_, GLenum type_, GLuint id_) : source(source_), type(type_), id(id_) {}

	bool operator== (const MessageID& rhs) const { return source == rhs.source && type == rhs.type && id == rhs.id;}
	bool operator!= (const MessageID& rhs) const { return source != rhs.source || type != rhs.type || id != rhs.id;}
	bool operator<  (const MessageID& rhs) const
	{
		return source < rhs.source || (source == rhs.source && (type < rhs.type || (type == rhs.type && id < rhs.id)));
	}
};

std::ostream& operator<< (std::ostream& str, const MessageID &id)
{
	return str << glu::getDebugMessageSourceStr(id.source) << ", "	<< glu::getDebugMessageTypeStr(id.type) << ", " << id.id;
}

// All info from a single debug message
struct MessageData
{
	MessageID	id;
	GLenum		severity;
	string		message;

	MessageData (void) : id(MessageID()), severity(GL_NONE) {}
	MessageData (const MessageID& id_, GLenum severity_, const string& message_) : id(id_) , severity(severity_) , message(message_) {}
};

extern "C" typedef void GLW_APIENTRY DebugCallbackFunc(GLenum, GLenum, GLuint, GLenum, GLsizei, const char*, const void*);

// Base class
class BaseCase : public NegativeTestShared::ErrorCase
{
public:
								BaseCase			(Context&					ctx,
													 const char*				name,
													 const char*				desc);
	virtual						~BaseCase			(void) {}

	virtual IterateResult		iterate				(void) = 0;

	virtual void				expectMessage		(GLenum source, GLenum type);
	virtual void				expectError			(GLenum error0, GLenum error1);

protected:
	struct VerificationResult
	{
		const qpTestResult	result;
		const string		resultMessage;
		const string		logMessage;

		VerificationResult (qpTestResult result_, const string& resultMessage_, const string& logMessage_)
			: result(result_), resultMessage(resultMessage_), logMessage(logMessage_) {}
	};

	static DebugCallbackFunc	callbackHandle;
	virtual void				callback			(GLenum source, GLenum type, GLuint id, GLenum severity, const std::string& message);


	VerificationResult			verifyMessageCount	(const MessageID& id, GLenum severity, int refCount, int resCount, bool messageEnabled) const;

	// Verify a single message instance against expected attributes
	void						verifyMessage		(const MessageData& message, GLenum source, GLenum type, GLuint id, GLenum severity);
	void						verifyMessage		(const MessageData& message, GLenum source, GLenum type);

	bool						verifyMessageExists	(const MessageData& message, GLenum source, GLenum type);
	void						verifyMessageGroup	(const MessageData& message, GLenum source, GLenum type);
	void						verifyMessageString	(const MessageData& message);

	bool						isDebugContext		(void) const;

	tcu::ResultCollector		m_results;
};

void BaseCase::callbackHandle (GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const char* message, const void* userParam)
{
	static_cast<BaseCase*>(const_cast<void*>(userParam))->callback(source, type, id, severity, string(message, &message[length]));
}

BaseCase::BaseCase (Context& ctx, const char* name, const char* desc)
	: ErrorCase(ctx, name, desc)
{
}

void BaseCase::expectMessage (GLenum source, GLenum type)
{
	DE_UNREF(source);
	DE_UNREF(type);
}

void BaseCase::expectError (GLenum error0, GLenum error1)
{
	if (error0 != GL_NO_ERROR || error1 != GL_NO_ERROR)
		expectMessage(GL_DEBUG_SOURCE_API, GL_DEBUG_TYPE_ERROR);
	else
		expectMessage(GL_DONT_CARE, GL_DONT_CARE);
}

void BaseCase::callback (GLenum source, GLenum type, GLuint id, GLenum severity, const string& message)
{
	DE_UNREF(source);
	DE_UNREF(type);
	DE_UNREF(id);
	DE_UNREF(severity);
	DE_UNREF(message);
}

BaseCase::VerificationResult BaseCase::verifyMessageCount (const MessageID& id, GLenum severity, int refCount, int resCount, bool messageEnabled) const
{
	std::stringstream log;

	// This message should not be filtered out
	if (messageEnabled)
	{
		if (resCount != refCount)
		{
			/*
			 * Technically nothing requires the implementation to be consistent in terms
			 * of the messages it produces in most situations, allowing the set of messages
			 * produced to vary between executions. This function splits messages
			 * into deterministic and non-deterministic to facilitate handling of such messages.
			 *
			 * Non-deterministic messages that are present in differing quantities in filtered and
			 * unfiltered runs will not fail the test case unless in direct violation of a filter:
			 * the implementation may produce an arbitrary number of such messages when they are
			 * not filtered out and none when they are filtered.
			 *
			 * A list of error source/type combinations with their assumed behaviour and
			 * the rationale for expecting such behaviour follows
			 *
			 * For API/shader messages we assume that the following types are deterministic:
			 *   DEBUG_TYPE_ERROR                 Errors specified by spec and should always be produced
			 *
			 * For API messages the following types are assumed to be non-deterministic
			 * and treated as quality warnings since the underlying reported issue does not change between calls:
			 *   DEBUG_TYPE_DEPRECATED_BEHAVIOR   Reasonable to only report first instance
             *   DEBUG_TYPE_UNDEFINED_BEHAVIOR    Reasonable to only report first instance
             *   DEBUG_TYPE_PORTABILITY           Reasonable to only report first instance
			 *
			 * For API messages the following types are assumed to be non-deterministic
			 * and do not affect test results.
			 *   DEBUG_TYPE_PERFORMANCE           May be tied to arbitrary factors, reasonable to report only first instance
             *   DEBUG_TYPE_OTHER                 Definition allows arbitrary contents
			 *
			 * For 3rd party and application messages the following types are deterministic:
             *   DEBUG_TYPE_MARKER                Only generated by test
			 *   DEBUG_TYPE_PUSH_GROUP            Only generated by test
			 *   DEBUG_TYPE_POP_GROUP             Only generated by test
			 *   All others                       Only generated by test
			 *
			 * All messages with category of window system or other are treated as non-deterministic
			 * and do not effect test results since they can be assumed to be outside control of
			 * both the implementation and test case
			 *
			 */

			const bool isDeterministic	= id.source == GL_DEBUG_SOURCE_APPLICATION ||
										  id.source == GL_DEBUG_SOURCE_THIRD_PARTY ||
										  ((id.source == GL_DEBUG_SOURCE_API || id.source == GL_DEBUG_SOURCE_SHADER_COMPILER) && id.type == GL_DEBUG_TYPE_ERROR);

			const bool canIgnore		= id.source == GL_DEBUG_SOURCE_WINDOW_SYSTEM || id.source == GL_DEBUG_SOURCE_OTHER;

			if (isDeterministic)
			{
				if (resCount > refCount)
				{
					log << "Extra instances of message were found: (" << id << ") with "
						<< glu::getDebugMessageSeverityStr(severity)
						<< " (got " << resCount << ", expected " << refCount << ")";
					return VerificationResult(QP_TEST_RESULT_FAIL, "Extra instances of a deterministic message were present", log.str());
				}
				else
				{
					log << "Instances of message were missing: (" << id << ") with "
						<< glu::getDebugMessageSeverityStr(severity)
						<< " (got " << resCount << ", expected " << refCount << ")";
					return VerificationResult(QP_TEST_RESULT_FAIL, "Message missing", log.str());
				}
			}
			else if(!canIgnore)
			{
				if (resCount > refCount)
				{
					log << "Extra instances of message were found but the message is non-deterministic(warning): (" << id << ") with "
						<< glu::getDebugMessageSeverityStr(severity)
						<< " (got " << resCount << ", expected " << refCount << ")";
					return VerificationResult(QP_TEST_RESULT_QUALITY_WARNING, "Extra instances of a message were present", log.str());
				}
				else
				{
					log << "Instances of message were missing but the message is non-deterministic(warning): (" << id << ") with "
						<< glu::getDebugMessageSeverityStr(severity)
						<< " (got " << resCount << ", expected " << refCount << ")";
					return VerificationResult(QP_TEST_RESULT_QUALITY_WARNING, "Message missing", log.str());
				}
			}
			else
			{
				if (resCount > refCount)
				{
					log << "Extra instances of message were found but the message is non-deterministic(ignored): (" << id << ") with "
						<< glu::getDebugMessageSeverityStr(severity)
						<< " (got " << resCount << ", expected " << refCount << ")";
					return VerificationResult(QP_TEST_RESULT_PASS, "", log.str());
				}
				else
				{
					log << "Instances of message were missing but the message is non-deterministic(ignored): (" << id << ") with "
						<< glu::getDebugMessageSeverityStr(severity)
						<< " (got " << resCount << ", expected " << refCount << ")";
					return VerificationResult(QP_TEST_RESULT_PASS, "", log.str());
				}
			}
		}
		else // Passed as appropriate
		{
			log << "Message was found when expected: ("<< id << ") with "
				<< glu::getDebugMessageSeverityStr(severity);
			return VerificationResult(QP_TEST_RESULT_PASS, "", log.str());
		}
	}
	// Message should be filtered out
	else
	{
		// Filtered out
		if (resCount == 0)
		{
			log << "Message was excluded correctly:  (" << id << ") with "
				<< glu::getDebugMessageSeverityStr(severity);
			return VerificationResult(QP_TEST_RESULT_PASS, "", log.str());
		}
		// Only present in filtered run (ERROR)
		else if (resCount > 0 && refCount == 0)
		{
			log << "A message was not excluded as it should have been: (" << id << ") with "
				<< glu::getDebugMessageSeverityStr(severity)
				<< ". This message was not present in the reference run";
			return VerificationResult(QP_TEST_RESULT_FAIL, "A message was not filtered out", log.str());
		}
		// Present in both runs (ERROR)
		else
		{
			log << "A message was not excluded as it should have been: (" << id << ") with "
				<< glu::getDebugMessageSeverityStr(severity);
			return VerificationResult(QP_TEST_RESULT_FAIL, "A message was not filtered out", log.str());
		}
	}
}

// Return true if message needs further verification
bool BaseCase::verifyMessageExists (const MessageData& message, GLenum source, GLenum type)
{
	TestLog& log = m_testCtx.getLog();

	if (source == GL_DONT_CARE || type == GL_DONT_CARE)
		return false;
	else if (message.id.source == GL_NONE || message.id.type == GL_NONE)
	{
		if (isDebugContext())
		{
			m_results.addResult(QP_TEST_RESULT_FAIL, "Message was not reported as expected");
			log << TestLog::Message << "A message was expected but none was reported" << TestLog::EndMessage;
		}
		else
		{
			m_results.addResult(QP_TEST_RESULT_QUALITY_WARNING, "Verification accuracy is lacking without a debug context");
			log << TestLog::Message << "A message was expected but none was reported. Running without a debug context" << TestLog::EndMessage;
		}
		return false;
	}
	else
		return true;
}

void BaseCase::verifyMessageGroup (const MessageData& message, GLenum source, GLenum type)
{
	TestLog& log = m_testCtx.getLog();

	if (message.id.source != source)
	{
		m_results.addResult(QP_TEST_RESULT_FAIL, "Incorrect message source");
		log << TestLog::Message << "Message source was " << glu::getDebugMessageSourceStr(message.id.source)
			<< " when it should have been "  << glu::getDebugMessageSourceStr(source) << TestLog::EndMessage;
	}

	if (message.id.type != type)
	{
		m_results.addResult(QP_TEST_RESULT_FAIL, "Incorrect message type");
		log << TestLog::Message << "Message type was " << glu::getDebugMessageTypeStr(message.id.type)
			<< " when it should have been " << glu::getDebugMessageTypeStr(type) << TestLog::EndMessage;
	}
}

void BaseCase::verifyMessageString (const MessageData& message)
{
	TestLog& log = m_testCtx.getLog();

	log << TestLog::Message << "Driver says: \"" << message.message << "\"" << TestLog::EndMessage;

	if (message.message.empty())
	{
		m_results.addResult(QP_TEST_RESULT_QUALITY_WARNING, "Empty message");
		log << TestLog::Message << "Message message was empty" << TestLog::EndMessage;
	}
}

void BaseCase::verifyMessage (const MessageData& message, GLenum source, GLenum type)
{
	if (verifyMessageExists(message, source, type))
	{
		verifyMessageString(message);
		verifyMessageGroup(message, source, type);
	}
}

void BaseCase::verifyMessage (const MessageData& message, GLenum source, GLenum type, GLuint id, GLenum severity)
{
	TestLog& log = m_testCtx.getLog();

	if (verifyMessageExists(message, source, type))
	{
		verifyMessageString(message);
		verifyMessageGroup(message, source, type);

		if (message.id.id != id)
		{
			m_results.addResult(QP_TEST_RESULT_FAIL, "Incorrect message id");
			log << TestLog::Message << "Message id was " << message.id.id
				<< " when it should have been " << id << TestLog::EndMessage;
		}

		if (message.severity != severity)
		{
			m_results.addResult(QP_TEST_RESULT_FAIL, "Incorrect message severity");
			log << TestLog::Message << "Message severity was " << glu::getDebugMessageSeverityStr(message.severity)
				<< " when it should have been " << glu::getDebugMessageSeverityStr(severity) << TestLog::EndMessage;
		}
	}
}

bool BaseCase::isDebugContext (void) const
{
	return (m_context.getRenderContext().getType().getFlags() & glu::CONTEXT_DEBUG) != 0;
}

// Generate errors, verify that each error results in a callback call
class CallbackErrorCase : public BaseCase
{
public:
								CallbackErrorCase	(Context&				ctx,
													 const char*			name,
													 const char*			desc,
													 TestFunctionWrapper	errorFunc);
	virtual						~CallbackErrorCase	(void) {}

	virtual IterateResult		iterate				(void);

	virtual void				expectMessage		(GLenum source, GLenum type);

private:
	virtual void				callback			(GLenum source, GLenum type, GLuint id, GLenum severity, const string& message);

	const TestFunctionWrapper	m_errorFunc;
	MessageData					m_lastMessage;
};

CallbackErrorCase::CallbackErrorCase (Context&				ctx,
									  const char*			name,
									  const char*			desc,
									  TestFunctionWrapper	errorFunc)
	: BaseCase		(ctx, name, desc)
	, m_errorFunc	(errorFunc)
{
}

CallbackErrorCase::IterateResult CallbackErrorCase::iterate (void)
{
	TCU_CHECK_AND_THROW(NotSupportedError, isKHRDebugSupported(m_context), "GL_KHR_debug is not supported");

	const glw::Functions&	gl		= m_context.getRenderContext().getFunctions();
	tcu::TestLog&			log		= m_testCtx.getLog();
	DebugMessageTestContext	context	= DebugMessageTestContext(*this, m_context.getRenderContext(), m_context.getContextInfo(), log, m_results, true);

	gl.enable(GL_DEBUG_OUTPUT);
	gl.enable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
	gl.debugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, DE_NULL, false); // disable all
	gl.debugMessageControl(GL_DEBUG_SOURCE_API, GL_DEBUG_TYPE_ERROR, GL_DONT_CARE, 0, DE_NULL, true); // enable API errors
	gl.debugMessageControl(GL_DEBUG_SOURCE_APPLICATION, GL_DONT_CARE, GL_DONT_CARE, 0, DE_NULL, true); // enable application messages
	gl.debugMessageControl(GL_DEBUG_SOURCE_THIRD_PARTY, GL_DONT_CARE, GL_DONT_CARE, 0, DE_NULL, true); // enable third party messages
	gl.debugMessageCallback(callbackHandle, this);

	m_errorFunc.call(context);

	gl.debugMessageCallback(DE_NULL, DE_NULL);
	gl.disable(GL_DEBUG_OUTPUT);

	m_results.setTestContextResult(m_testCtx);

	return STOP;
}

void CallbackErrorCase::expectMessage (GLenum source, GLenum type)
{
	verifyMessage(m_lastMessage, source, type);
	m_lastMessage = MessageData();

	// Reset error so that code afterwards (such as glu::ShaderProgram) doesn't break because of
	// lingering error state.
	m_context.getRenderContext().getFunctions().getError();
}

void CallbackErrorCase::callback (GLenum source, GLenum type, GLuint id, GLenum severity, const string& message)
{
	m_lastMessage = MessageData(MessageID(source, type, id), severity, message);
}

// Generate errors, verify that each error results in a log entry
class LogErrorCase : public BaseCase
{
public:
								LogErrorCase	(Context&				context,
												 const char*			name,
												 const char*			desc,
												 TestFunctionWrapper	errorFunc);
	virtual						~LogErrorCase	(void) {}

	virtual IterateResult		iterate			(void);

	virtual void				expectMessage	(GLenum source, GLenum type);

private:
	const TestFunctionWrapper	m_errorFunc;
	MessageData					m_lastMessage;
};

LogErrorCase::LogErrorCase (Context&			ctx,
							const char*			name,
							const char*			desc,
							TestFunctionWrapper	errorFunc)
	: BaseCase		(ctx, name, desc)
	, m_errorFunc	(errorFunc)
{
}

LogErrorCase::IterateResult LogErrorCase::iterate (void)
{
	TCU_CHECK_AND_THROW(NotSupportedError, isKHRDebugSupported(m_context), "GL_KHR_debug is not supported");

	const glw::Functions&	gl		= m_context.getRenderContext().getFunctions();
	tcu::TestLog&			log		= m_testCtx.getLog();
	DebugMessageTestContext	context	= DebugMessageTestContext(*this, m_context.getRenderContext(), m_context.getContextInfo(), log, m_results, true);
	GLint					numMsg	= 0;

	gl.enable(GL_DEBUG_OUTPUT);
	gl.enable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
	gl.debugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, DE_NULL, false); // disable all
	gl.debugMessageControl(GL_DEBUG_SOURCE_API, GL_DEBUG_TYPE_ERROR, GL_DONT_CARE, 0, DE_NULL, true); // enable API errors
	gl.debugMessageCallback(DE_NULL, DE_NULL); // enable logging
	gl.getIntegerv(GL_DEBUG_LOGGED_MESSAGES, &numMsg);
	gl.getDebugMessageLog(numMsg, 0, DE_NULL, DE_NULL, DE_NULL, DE_NULL, DE_NULL, DE_NULL); // clear log

	m_errorFunc.call(context);

	gl.disable(GL_DEBUG_OUTPUT);
	m_results.setTestContextResult(m_testCtx);

	return STOP;
}

void LogErrorCase::expectMessage (GLenum source, GLenum type)
{
	const glw::Functions&	gl			= m_context.getRenderContext().getFunctions();
	int						numMsg		= 0;
	TestLog&				log			= m_testCtx.getLog();
	MessageData				lastMsg;

	if (source == GL_DONT_CARE || type == GL_DONT_CARE)
		return;

	gl.getIntegerv(GL_DEBUG_LOGGED_MESSAGES, &numMsg);

	if (numMsg == 0)
	{
		if (isDebugContext())
		{
			m_results.addResult(QP_TEST_RESULT_FAIL, "Error was not reported as expected");
			log << TestLog::Message << "A message was expected but none was reported (empty message log)" << TestLog::EndMessage;
		}
		else
		{
			m_results.addResult(QP_TEST_RESULT_QUALITY_WARNING, "Verification accuracy is lacking without a debug context");
			log << TestLog::Message << "A message was expected but none was reported (empty message log). Running without a debug context" << TestLog::EndMessage;
		}
		return;
	}

	// There may be messages other than the error we are looking for in the log.
	// Strictly nothing prevents the implementation from producing more than the
	// required error from an API call with a defined error. however we assume that
	// since calls that produce an error should not change GL state the implementation
	// should have nothing else to report.
	if (numMsg > 1)
		gl.getDebugMessageLog(numMsg-1, 0, DE_NULL, DE_NULL, DE_NULL, DE_NULL, DE_NULL, DE_NULL); // Clear all but last

	{
		int  msgLen = 0;
		gl.getIntegerv(GL_DEBUG_NEXT_LOGGED_MESSAGE_LENGTH, &msgLen);

		TCU_CHECK_MSG(msgLen >= 0, "Negative message length");
		TCU_CHECK_MSG(msgLen < 100000, "Excessively long message");

		lastMsg.message.resize(msgLen);
		gl.getDebugMessageLog(1, msgLen, &lastMsg.id.source, &lastMsg.id.type, &lastMsg.id.id, &lastMsg.severity, &msgLen, &lastMsg.message[0]);
	}

	log << TestLog::Message << "Driver says: \"" << lastMsg.message << "\"" << TestLog::EndMessage;

	verifyMessage(lastMsg, source, type);

	// Reset error so that code afterwards (such as glu::ShaderProgram) doesn't break because of
	// lingering error state.
	m_context.getRenderContext().getFunctions().getError();
}

// Generate errors, verify that calling glGetError afterwards produces desired result
class GetErrorCase : public BaseCase
{
public:
								GetErrorCase	(Context&				ctx,
												 const char*			name,
												 const char*			desc,
												 TestFunctionWrapper	errorFunc);
	virtual						~GetErrorCase	(void) {}

	virtual IterateResult		iterate			(void);

	virtual void				expectMessage	(GLenum source, GLenum type);
	virtual void				expectError		(glw::GLenum error0, glw::GLenum error1);

private:
	const TestFunctionWrapper	m_errorFunc;
};

GetErrorCase::GetErrorCase (Context&			ctx,
							const char*			name,
							const char*			desc,
							TestFunctionWrapper	errorFunc)
	: BaseCase		(ctx, name, desc)
	, m_errorFunc	(errorFunc)
{
}

GetErrorCase::IterateResult GetErrorCase::iterate (void)
{
	tcu::TestLog&			log		= m_testCtx.getLog();
	DebugMessageTestContext	context	= DebugMessageTestContext(*this, m_context.getRenderContext(), m_context.getContextInfo(), log, m_results, true);

	m_errorFunc.call(context);

	m_results.setTestContextResult(m_testCtx);

	return STOP;
}

void GetErrorCase::expectMessage (GLenum source, GLenum type)
{
	DE_UNREF(source);
	DE_UNREF(type);
	DE_FATAL("GetErrorCase cannot handle anything other than error codes");
}

void GetErrorCase::expectError (glw::GLenum error0, glw::GLenum error1)
{
	const glw::Functions&	gl		= m_context.getRenderContext().getFunctions();
	TestLog&				log		= m_testCtx.getLog();

	const GLenum			result	= gl.getError();

	if (result != error0 && result != error1)
	{
		m_results.addResult(QP_TEST_RESULT_FAIL, "Incorrect error was reported");
		if (error0 == error1)
			log << TestLog::Message
				<< glu::getErrorStr(error0) << " was expected but got "
				<< glu::getErrorStr(result)
				<< TestLog::EndMessage;
		else
			log << TestLog::Message
				<< glu::getErrorStr(error0) << " or "
				<< glu::getErrorStr(error1) << " was expected but got "
				<< glu::getErrorStr(result)
				<< TestLog::EndMessage;
		return;
	}
}

// Generate errors, log the types, disable some, regenerate errors, verify correct errors (not)reported
class FilterCase : public BaseCase
{
public:
										FilterCase		(Context&							ctx,
														 const char*						name,
														 const char*						desc,
														 const vector<TestFunctionWrapper>&	errorFuncs);
	virtual								~FilterCase		(void) {}

	virtual IterateResult				iterate			(void);

	virtual void						expectMessage	(GLenum source, GLenum type);

protected:
	struct MessageFilter
	{
		MessageFilter() : source(GL_DONT_CARE), type(GL_DONT_CARE), severity(GL_DONT_CARE), enabled(true) {} // Default to enable all
		MessageFilter(GLenum source_, GLenum type_, GLenum severity_, const vector<GLuint>& ids_, bool enabled_) : source(source_), type(type_), severity(severity_), ids(ids_), enabled(enabled_) {}

		GLenum			source;
		GLenum			type;
		GLenum			severity;
		vector<GLuint>	ids;
		bool			enabled;
	};

	virtual void						callback			(GLenum source, GLenum type, GLuint id, GLenum severity, const string& message);

	vector<MessageData>					genMessages			(bool uselog, const string& desc);

	vector<MessageFilter>				genFilters			(const vector<MessageData>& messages, const vector<MessageFilter>& initial, deUint32 seed, int iterations) const;
	void								applyFilters		(const vector<MessageFilter>& filters) const;
	bool								isEnabled			(const vector<MessageFilter>& filters, const MessageData& message) const;

	void								verify				(const vector<MessageData>&		refMessages,
															 const vector<MessageData>&		filteredMessages,
															 const vector<MessageFilter>&	filters);

	const vector<TestFunctionWrapper>	m_errorFuncs;

	vector<MessageData>*				m_currentErrors;
};

FilterCase::FilterCase (Context&							ctx,
						const char*							name,
						const char*							desc,
						const vector<TestFunctionWrapper>&	errorFuncs)
	: BaseCase			(ctx, name, desc)
	, m_errorFuncs		(errorFuncs)
	, m_currentErrors	(DE_NULL)
{
}

FilterCase::IterateResult FilterCase::iterate (void)
{
	TCU_CHECK_AND_THROW(NotSupportedError, isKHRDebugSupported(m_context), "GL_KHR_debug is not supported");

	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.enable(GL_DEBUG_OUTPUT);
	gl.enable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
	gl.debugMessageCallback(callbackHandle, this);

	try
	{
		gl.debugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, DE_NULL, true);

		{
			const vector<MessageData>	refMessages		= genMessages(true, "Reference run");
			const MessageFilter			baseFilter		(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, vector<GLuint>(), true);
			const deUint32				baseSeed		= deStringHash(getName()) ^ m_testCtx.getCommandLine().getBaseSeed();
			const vector<MessageFilter>	filters			= genFilters(refMessages, vector<MessageFilter>(1, baseFilter), baseSeed, 4);
			vector<MessageData>			filteredMessages;

			applyFilters(filters);

			// Generate errors
			filteredMessages = genMessages(false, "Filtered run");

			// Verify
			verify(refMessages, filteredMessages, filters);

			if (!isDebugContext() && refMessages.empty())
				m_results.addResult(QP_TEST_RESULT_QUALITY_WARNING, "Verification accuracy is lacking without a debug context");
		}
	}
	catch (...)
	{
		gl.disable(GL_DEBUG_OUTPUT);
		gl.debugMessageCallback(DE_NULL, DE_NULL);
		throw;
	}

	gl.disable(GL_DEBUG_OUTPUT);
	gl.debugMessageCallback(DE_NULL, DE_NULL);
	m_results.setTestContextResult(m_testCtx);

	return STOP;
}

void FilterCase::expectMessage (GLenum source, GLenum type)
{
	DE_UNREF(source);
	DE_UNREF(type);
}

void FilterCase::callback (GLenum source, GLenum type, GLuint id, GLenum severity, const string& message)
{
	if (m_currentErrors)
		m_currentErrors->push_back(MessageData(MessageID(source, type, id), severity, message));
}

vector<MessageData> FilterCase::genMessages (bool uselog, const string& desc)
{
	tcu::TestLog&			log			= m_testCtx.getLog();
	DebugMessageTestContext	context		= DebugMessageTestContext(*this, m_context.getRenderContext(), m_context.getContextInfo(), log, m_results, uselog);
	tcu::ScopedLogSection	section		(log, "message gen", desc);
	vector<MessageData>		messages;

	m_currentErrors = &messages;

	for (int ndx = 0; ndx < int(m_errorFuncs.size()); ndx++)
		m_errorFuncs[ndx].call(context);

	m_currentErrors = DE_NULL;

	return messages;
}

vector<FilterCase::MessageFilter> FilterCase::genFilters (const vector<MessageData>& messages, const vector<MessageFilter>& initial, deUint32 seed, int iterations) const
{
	de::Random				rng				(seed ^ deInt32Hash(deStringHash(getName())));

	set<MessageID>			tempMessageIds;
	set<GLenum>				tempSources;
	set<GLenum>				tempTypes;
	set<GLenum>				tempSeverities;

	if (messages.empty())
		return initial;

	for (int ndx = 0; ndx < int(messages.size()); ndx++)
	{
		const MessageData& msg = messages[ndx];

		tempMessageIds.insert(msg.id);
		tempSources.insert(msg.id.source);
		tempTypes.insert(msg.id.type);
		tempSeverities.insert(msg.severity);
	}

	{
		// Fetchable by index
		const vector<MessageID> messageIds	(tempMessageIds.begin(), tempMessageIds.end());
		const vector<GLenum>	sources		(tempSources.begin(), tempSources.end());
		const vector<GLenum>	types		(tempTypes.begin(), tempTypes.end());
		const vector<GLenum>	severities	(tempSeverities.begin(), tempSeverities.end());

		vector<MessageFilter>	filters		= initial;

		for (int iteration = 0; iteration < iterations; iteration++)
		{
			switch(rng.getInt(0, 8)) // Distribute so that per-message randomization (the default branch) is prevalent
			{
				case 0:
				{
					const GLenum	source	= sources[rng.getInt(0, int(sources.size()-1))];
					const bool		enabled	= rng.getBool();

					filters.push_back(MessageFilter(source, GL_DONT_CARE, GL_DONT_CARE, vector<GLuint>(), enabled));
					break;
				}

				case 1:
				{
					const GLenum	type	= types[rng.getUint32()%types.size()];
					const bool		enabled	= rng.getBool();

					filters.push_back(MessageFilter(GL_DONT_CARE, type, GL_DONT_CARE, vector<GLuint>(), enabled));
					break;
				}

				case 2:
				{
					const GLenum	severity	= severities[rng.getUint32()%severities.size()];
					const bool		enabled		= rng.getBool();

					filters.push_back(MessageFilter(GL_DONT_CARE, GL_DONT_CARE, severity, vector<GLuint>(), enabled));
					break;
				}

				default:
				{
					const int start = rng.getInt(0, int(messageIds.size()));

					for (int itr = 0; itr < 4; itr++)
					{
						const MessageID&	id		= messageIds[(start+itr)%messageIds.size()];
						const bool			enabled = rng.getBool();

						filters.push_back(MessageFilter(id.source, id.type, GL_DONT_CARE, vector<GLuint>(1, id.id), enabled));
					}
				}
			}
		}

		return filters;
	}
}

void FilterCase::applyFilters (const vector<MessageFilter>& filters) const
{
	TestLog&					log		= m_testCtx.getLog();
	const tcu::ScopedLogSection	section	(log, "", "Setting message filters");
	const glw::Functions&		gl		= m_context.getRenderContext().getFunctions();

	for (size_t filterNdx = 0; filterNdx < filters.size(); filterNdx++)
	{
		const MessageFilter& filter = filters[filterNdx];

		if (filter.ids.empty())
			log << TestLog::Message << "Setting messages with"
				<< " source " << glu::getDebugMessageSourceStr(filter.source)
				<< ", type " << glu::getDebugMessageTypeStr(filter.type)
				<< " and severity " << glu::getDebugMessageSeverityStr(filter.severity)
				<< (filter.enabled ? " to enabled" : " to disabled")
				<< TestLog::EndMessage;
		else
		{
			for (size_t ndx = 0; ndx < filter.ids.size(); ndx++)
				log << TestLog::Message << "Setting message (" << MessageID(filter.source, filter.type, filter.ids[ndx]) << ") to " << (filter.enabled ? "enabled" : "disabled") << TestLog::EndMessage;
		}

		gl.debugMessageControl(filter.source, filter.type, filter.severity, GLsizei(filter.ids.size()), filter.ids.empty() ? DE_NULL : &filter.ids[0], filter.enabled);
	}
}

bool FilterCase::isEnabled (const vector<MessageFilter>& filters, const MessageData& message) const
{
	bool retval = true;

	for (size_t filterNdx = 0; filterNdx < filters.size(); filterNdx++)
	{
		const MessageFilter&	filter	= filters[filterNdx];

		if (filter.ids.empty())
		{
			if (filter.source != GL_DONT_CARE && filter.source != message.id.source)
				continue;

			if (filter.type != GL_DONT_CARE && filter.type != message.id.type)
				continue;

			if (filter.severity != GL_DONT_CARE && filter.severity != message.severity)
				continue;
		}
		else
		{
			DE_ASSERT(filter.source != GL_DONT_CARE);
			DE_ASSERT(filter.type != GL_DONT_CARE);
			DE_ASSERT(filter.severity == GL_DONT_CARE);

			if (filter.source != message.id.source || filter.type != message.id.type)
				continue;

			if (!de::contains(filter.ids.begin(), filter.ids.end(), message.id.id))
				continue;
		}

		retval = filter.enabled;
	}

	return retval;
}

struct MessageMeta
{
	int		refCount;
	int		resCount;
	GLenum	severity;

	MessageMeta (void) : refCount(0), resCount(0), severity(GL_NONE) {}
};

void FilterCase::verify (const vector<MessageData>& refMessages, const vector<MessageData>& resMessages, const vector<MessageFilter>& filters)
{
	TestLog&						log		= m_testCtx.getLog();
	map<MessageID, MessageMeta>		counts;

	log << TestLog::Section("verification", "Verifying");

	// Gather message counts & severities, report severity mismatches if found
	for (size_t refNdx = 0; refNdx < refMessages.size(); refNdx++)
	{
		const MessageData&	msg  = refMessages[refNdx];
		MessageMeta&		meta = counts[msg.id];

		if (meta.severity != GL_NONE && meta.severity != msg.severity)
		{
			log << TestLog::Message << "A message has variable severity between instances: (" << msg.id << ") with severity "
				<< glu::getDebugMessageSeverityStr(meta.severity) << " and " << glu::getDebugMessageSeverityStr(msg.severity) << TestLog::EndMessage;
			m_results.addResult(QP_TEST_RESULT_FAIL, "Message severity changed between instances of the same message");
		}

		meta.refCount++;
		meta.severity = msg.severity;
	}

	for (size_t resNdx = 0; resNdx < resMessages.size(); resNdx++)
	{
		const MessageData&	msg  = resMessages[resNdx];
		MessageMeta&		meta = counts[msg.id];

		if (meta.severity != GL_NONE && meta.severity != msg.severity)
		{
			log << TestLog::Message << "A message has variable severity between instances: (" << msg.id << ") with severity "
				<< glu::getDebugMessageSeverityStr(meta.severity) << " and " << glu::getDebugMessageSeverityStr(msg.severity) << TestLog::EndMessage;
			m_results.addResult(QP_TEST_RESULT_FAIL, "Message severity changed between instances of the same message");
		}

		meta.resCount++;
		meta.severity = msg.severity;
	}

	for (map<MessageID, MessageMeta>::const_iterator itr = counts.begin(); itr != counts.end(); itr++)
	{
		const MessageID&	id			= itr->first;
		const GLenum		severity	= itr->second.severity;

		const int			refCount	= itr->second.refCount;
		const int			resCount	= itr->second.resCount;
		const bool			enabled		= isEnabled(filters, MessageData(id, severity, ""));

		VerificationResult	result		= verifyMessageCount(id, severity, refCount, resCount, enabled);

		log << TestLog::Message << result.logMessage << TestLog::EndMessage;

		if (result.result != QP_TEST_RESULT_PASS)
			m_results.addResult(result.result, result.resultMessage);
	}

	log << TestLog::EndSection;
}

// Filter case that uses debug groups
class GroupFilterCase : public FilterCase
{
public:
							GroupFilterCase		(Context&							ctx,
												 const char*						name,
												 const char*						desc,
												 const vector<TestFunctionWrapper>&	errorFuncs);
	virtual					~GroupFilterCase	(void) {}

	virtual IterateResult	iterate				(void);
};

GroupFilterCase::GroupFilterCase (Context&								ctx,
								  const char*							name,
								  const char*							desc,
								  const vector<TestFunctionWrapper>&	errorFuncs)
	: FilterCase(ctx, name, desc, errorFuncs)
{
}

template<typename T>
vector<T> join(const vector<T>& a, const vector<T>&b)
{
	vector<T> retval;

	retval.reserve(a.size()+b.size());
	retval.insert(retval.end(), a.begin(), a.end());
	retval.insert(retval.end(), b.begin(), b.end());
	return retval;
}

GroupFilterCase::IterateResult GroupFilterCase::iterate (void)
{
	TCU_CHECK_AND_THROW(NotSupportedError, isKHRDebugSupported(m_context), "GL_KHR_debug is not supported");

	const glw::Functions&	gl			= m_context.getRenderContext().getFunctions();
	tcu::TestLog&			log			= m_testCtx.getLog();

	gl.enable(GL_DEBUG_OUTPUT);
	gl.enable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
	gl.debugMessageCallback(callbackHandle, this);

	try
	{
		gl.debugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, DE_NULL, true);

		{

			// Generate reference (all errors)
			const vector<MessageData>	refMessages		= genMessages(true, "Reference run");
			const deUint32				baseSeed		= deStringHash(getName()) ^ m_testCtx.getCommandLine().getBaseSeed();
			const MessageFilter			baseFilter		 (GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, vector<GLuint>(), true);
			const vector<MessageFilter>	filter0			= genFilters(refMessages, vector<MessageFilter>(1, baseFilter), baseSeed, 4);
			vector<MessageData>			resMessages0;

			applyFilters(filter0);

			resMessages0 = genMessages(false, "Filtered run, default debug group");

			// Initial verification
			verify(refMessages, resMessages0, filter0);

			{
				// Generate reference (filters inherited from parent)
				const vector<MessageFilter> filter1base		= genFilters(refMessages, vector<MessageFilter>(), baseSeed ^ 0xDEADBEEF, 4);
				const vector<MessageFilter>	filter1full		= join(filter0, filter1base);
				tcu::ScopedLogSection		section1		(log, "", "Pushing Debug Group");
				vector<MessageData>			resMessages1;

				gl.pushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 1, -1, "Test Group");
				applyFilters(filter1base);

				// First nested verification
				resMessages1 = genMessages(false, "Filtered run, pushed one debug group");
				verify(refMessages, resMessages1, filter1full);

				{
					// Generate reference (filters iherited again)
					const vector<MessageFilter>	filter2base		= genFilters(refMessages, vector<MessageFilter>(), baseSeed ^ 0x43211234, 4);
					const vector<MessageFilter>	filter2full		= join(filter1full, filter2base);
					tcu::ScopedLogSection		section2		(log, "", "Pushing Debug Group");
					vector<MessageData>			resMessages2;

					gl.pushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 1, -1, "Nested Test Group");
					applyFilters(filter2base);

					// Second nested verification
					resMessages2 = genMessages(false, "Filtered run, pushed two debug groups");
					verify(refMessages, resMessages2, filter2full);

					gl.popDebugGroup();
				}

				// First restore verification
				resMessages1 = genMessages(false, "Filtered run, popped second debug group");
				verify(refMessages, resMessages1, filter1full);

				gl.popDebugGroup();
			}

			// restore verification
			resMessages0 = genMessages(false, "Filtered run, popped first debug group");
			verify(refMessages, resMessages0, filter0);

			if (!isDebugContext() && refMessages.empty())
				m_results.addResult(QP_TEST_RESULT_QUALITY_WARNING, "Verification accuracy is lacking without a debug context");
		}
	}
	catch (...)
	{
		gl.disable(GL_DEBUG_OUTPUT);
		gl.debugMessageCallback(DE_NULL, DE_NULL);
		throw;
	}

	gl.disable(GL_DEBUG_OUTPUT);
	gl.debugMessageCallback(DE_NULL, DE_NULL);
	m_results.setTestContextResult(m_testCtx);
	return STOP;
}

// Basic grouping functionality
class GroupCase : public BaseCase
{
public:
							GroupCase	(Context&				ctx,
										 const char*			name,
										 const char*			desc);
	virtual					~GroupCase	() {}

	virtual IterateResult	iterate		(void);

private:
	virtual void			callback	(GLenum source, GLenum type, GLuint id, GLenum severity, const string& message);

	MessageData				m_lastMessage;
};

GroupCase::GroupCase (Context&				ctx,
					  const char*			name,
					  const char*			desc)
	: BaseCase(ctx, name, desc)
{
}

GroupCase::IterateResult GroupCase::iterate (void)
{
	TCU_CHECK_AND_THROW(NotSupportedError, isKHRDebugSupported(m_context), "GL_KHR_debug is not supported");

	const glw::Functions&	gl		= m_context.getRenderContext().getFunctions();
	tcu::TestLog&			log		= m_testCtx.getLog();
	glu::CallLogWrapper		wrapper	(gl, log);

	gl.enable(GL_DEBUG_OUTPUT);
	gl.enable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
	gl.debugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, DE_NULL, false); // disable all
	gl.debugMessageControl(GL_DEBUG_SOURCE_API, GL_DEBUG_TYPE_ERROR, GL_DONT_CARE, 0, DE_NULL, true); // enable API errors
	gl.debugMessageControl(GL_DEBUG_SOURCE_APPLICATION, GL_DONT_CARE, GL_DONT_CARE, 0, DE_NULL, true); // enable application messages
	gl.debugMessageControl(GL_DEBUG_SOURCE_THIRD_PARTY, GL_DONT_CARE, GL_DONT_CARE, 0, DE_NULL, true); // enable third party messages
	gl.debugMessageCallback(callbackHandle, this);

	wrapper.enableLogging(true);
	wrapper.glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 1234, -1, "Pushed debug stack");
	verifyMessage(m_lastMessage, GL_DEBUG_SOURCE_APPLICATION, GL_DEBUG_TYPE_PUSH_GROUP, 1234, GL_DEBUG_SEVERITY_NOTIFICATION);
	wrapper.glPopDebugGroup();
	verifyMessage(m_lastMessage, GL_DEBUG_SOURCE_APPLICATION, GL_DEBUG_TYPE_POP_GROUP, 1234, GL_DEBUG_SEVERITY_NOTIFICATION);

	wrapper.glPushDebugGroup(GL_DEBUG_SOURCE_THIRD_PARTY, 4231, -1, "Pushed debug stack");
	verifyMessage(m_lastMessage, GL_DEBUG_SOURCE_THIRD_PARTY, GL_DEBUG_TYPE_PUSH_GROUP, 4231, GL_DEBUG_SEVERITY_NOTIFICATION);
	wrapper.glPopDebugGroup();
	verifyMessage(m_lastMessage, GL_DEBUG_SOURCE_THIRD_PARTY, GL_DEBUG_TYPE_POP_GROUP, 4231, GL_DEBUG_SEVERITY_NOTIFICATION);

	gl.debugMessageCallback(DE_NULL, DE_NULL);
	gl.disable(GL_DEBUG_OUTPUT);

	m_results.setTestContextResult(m_testCtx);

	return STOP;
}

void GroupCase::callback (GLenum source, GLenum type, GLuint id, GLenum severity, const string& message)
{
	m_lastMessage = MessageData(MessageID(source, type, id), severity, message);
}

// Asynchronous debug output
class AsyncCase : public BaseCase
{
public:
										AsyncCase			(Context&							ctx,
															 const char*						name,
															 const char*						desc,
															 const vector<TestFunctionWrapper>&	errorFuncs,
															 bool								useCallbacks);
	virtual								~AsyncCase			(void) {}

	virtual IterateResult				iterate				(void);

	virtual void						expectMessage		(glw::GLenum source, glw::GLenum type);

private:
	struct MessageCount
	{
		int received;
		int expected;

		MessageCount(void) : received(0), expected(0) {}
	};
	typedef map<MessageID, MessageCount> MessageCounter;

	enum VerifyState
	{
		VERIFY_PASS = 0,
		VERIFY_MINIMUM,
		VERIFY_FAIL,

		VERIFY_LAST
	};

	virtual void						callback			(glw::GLenum source, glw::GLenum type, glw::GLuint id, glw::GLenum severity, const std::string& message);
	VerifyState							verify				(bool uselog);
	void								fetchLogMessages	(void);

	const vector<TestFunctionWrapper>	m_errorFuncs;
	const bool							m_useCallbacks;

	MessageCounter						m_counts;

	de::Mutex							m_mutex;
};

AsyncCase::AsyncCase (Context&								ctx,
					  const char*							name,
					  const char*							desc,
					  const vector<TestFunctionWrapper>&	errorFuncs,
					  bool									useCallbacks)
	: BaseCase			(ctx, name, desc)
	, m_errorFuncs		(errorFuncs)
	, m_useCallbacks	(useCallbacks)
{
}

AsyncCase::IterateResult AsyncCase::iterate (void)
{
	TCU_CHECK_AND_THROW(NotSupportedError, isKHRDebugSupported(m_context), "GL_KHR_debug is not supported");

	const glw::Functions&	gl			= m_context.getRenderContext().getFunctions();
	tcu::TestLog&			log			= m_testCtx.getLog();
	DebugMessageTestContext	context		= DebugMessageTestContext(*this, m_context.getRenderContext(), m_context.getContextInfo(), log, m_results, true);
	const int				maxWait		= 10000; // ms
	const int				warnWait	= 100;

	// Clear log from earlier messages
	{
		GLint numMessages = 0;
		gl.getIntegerv(GL_DEBUG_LOGGED_MESSAGES, &numMessages);
		gl.getDebugMessageLog(numMessages, 0, DE_NULL, DE_NULL, DE_NULL, DE_NULL, DE_NULL, DE_NULL);
	}

	gl.enable(GL_DEBUG_OUTPUT);
	gl.enable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
	gl.debugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, DE_NULL, false);

	// Some messages could be dependent on the value of DEBUG_OUTPUT_SYNCHRONOUS so only use API errors which should be generated in all cases
	gl.debugMessageControl(GL_DEBUG_SOURCE_API, GL_DEBUG_TYPE_ERROR, GL_DONT_CARE, 0, DE_NULL, true);

	if (m_useCallbacks) // will use log otherwise
		gl.debugMessageCallback(callbackHandle, this);
	else
		gl.debugMessageCallback(DE_NULL, DE_NULL);

	// Reference run (synchoronous)
	{
		tcu::ScopedLogSection section(log, "reference run", "Reference run (synchronous)");

		for (int ndx = 0; ndx < int(m_errorFuncs.size()); ndx++)
			m_errorFuncs[ndx].call(context);
	}

	if (m_counts.empty())
	{
		if (!isDebugContext())
			m_results.addResult(QP_TEST_RESULT_QUALITY_WARNING, "Need debug context to guarantee implementation behaviour (see command line options)");

		log << TestLog::Message << "Reference run produced no messages, nothing to verify" << TestLog::EndMessage;

		gl.debugMessageCallback(DE_NULL, DE_NULL);
		gl.disable(GL_DEBUG_OUTPUT);

		m_results.setTestContextResult(m_testCtx);
		return STOP;
	}

	for (MessageCounter::iterator itr = m_counts.begin(); itr != m_counts.end(); itr++)
	{
		itr->second.expected = itr->second.received;
		itr->second.received = 0;
	}

	gl.disable(GL_DEBUG_OUTPUT_SYNCHRONOUS);

	// Result run (async)
	for (int ndx = 0; ndx < int(m_errorFuncs.size()); ndx++)
		m_errorFuncs[ndx].call(context);

	// Repatedly try verification, new results may be added to m_receivedMessages at any time
	{
		tcu::ScopedLogSection	section			(log, "result run", "Result run (asynchronous)");
		VerifyState				lastTimelyState = VERIFY_FAIL;

		for (int waited = 0;;)
		{
			const VerifyState	pass = verify(false);
			const int			wait = de::max(50, waited>>2);

			// Pass (possibly due to time limit)
			if (pass == VERIFY_PASS || (pass == VERIFY_MINIMUM && waited >= maxWait))
			{
				verify(true); // log

				// State changed late
				if (waited >= warnWait && lastTimelyState != pass)
					m_results.addResult(QP_TEST_RESULT_QUALITY_WARNING, "Async messages were returned to application somewhat slowly");

				log << TestLog::Message << "Passed after ~" << waited << "ms of waiting" << TestLog::EndMessage;
				break;
			}
			// fail
			else if (waited >= maxWait)
			{
				verify(true); // log

				log << TestLog::Message << "Waited for ~" << waited << "ms without getting all expected messages" << TestLog::EndMessage;
				m_results.addResult(QP_TEST_RESULT_FAIL, "Async messages were not returned to application within a reasonable timeframe");
				break;
			}

			if (waited < warnWait)
				lastTimelyState = pass;

			deSleep(wait);
			waited += wait;

			if (!m_useCallbacks)
				fetchLogMessages();
		}
	}

	gl.debugMessageCallback(DE_NULL, DE_NULL);

	gl.disable(GL_DEBUG_OUTPUT);
	m_results.setTestContextResult(m_testCtx);

	return STOP;
}

void AsyncCase::expectMessage (GLenum source, GLenum type)
{
	// Good time to clean up the queue as this should be called after most messages are generated
	if (!m_useCallbacks)
		fetchLogMessages();

	DE_UNREF(source);
	DE_UNREF(type);
}

void AsyncCase::callback (GLenum source, GLenum type, GLuint id, GLenum severity, const string& message)
{
	DE_ASSERT(m_useCallbacks);
	DE_UNREF(severity);
	DE_UNREF(message);

	de::ScopedLock lock(m_mutex);

	m_counts[MessageID(source, type, id)].received++;
}

// Note that we can never guarantee getting all messages back when using logs/fetching as the GL may create more than its log size limit during an arbitrary period of time
void AsyncCase::fetchLogMessages (void)
{
	const glw::Functions&	gl		= m_context.getRenderContext().getFunctions();
	GLint					numMsg	= 0;

	gl.getIntegerv(GL_DEBUG_LOGGED_MESSAGES, &numMsg);

	for(int msgNdx = 0; msgNdx < numMsg; msgNdx++)
	{
		int			msgLen = 0;
		MessageData msg;

		gl.getIntegerv(GL_DEBUG_NEXT_LOGGED_MESSAGE_LENGTH, &msgLen);

		TCU_CHECK_MSG(msgLen >= 0, "Negative message length");
		TCU_CHECK_MSG(msgLen < 100000, "Excessively long message");

		msg.message.resize(msgLen);
		gl.getDebugMessageLog(1, msgLen, &msg.id.source, &msg.id.type, &msg.id.id, &msg.severity, &msgLen, &msg.message[0]);

		{
			const de::ScopedLock lock(m_mutex); // Don't block during API call

			m_counts[MessageID(msg.id)].received++;
		}
	}
}

AsyncCase::VerifyState AsyncCase::verify (bool uselog)
{
	using std::map;

	VerifyState			retval		= VERIFY_PASS;
	TestLog&			log			= m_testCtx.getLog();

	const de::ScopedLock lock(m_mutex);

	for (map<MessageID, MessageCount>::const_iterator itr = m_counts.begin(); itr != m_counts.end(); itr++)
	{
		const MessageID&	id			= itr->first;

		const int			refCount	= itr->second.expected;
		const int			resCount	= itr->second.received;
		const bool			enabled		= true;

		VerificationResult	result		= verifyMessageCount(id, GL_DONT_CARE, refCount, resCount, enabled);

		if (uselog)
			log << TestLog::Message << result.logMessage << TestLog::EndMessage;

		if (result.result == QP_TEST_RESULT_FAIL)
			retval = VERIFY_FAIL;
		else if (result.result != QP_TEST_RESULT_PASS && retval == VERIFY_PASS)
			retval = VERIFY_MINIMUM;
	}

	return retval;
}

// Tests debug labels
class LabelCase : public TestCase
{
public:
							LabelCase	(Context&				ctx,
										 const char*			name,
										 const char*			desc,
										 GLenum					identifier);
	virtual					~LabelCase	(void) {}

	virtual IterateResult	iterate		(void);

private:
	GLenum					m_identifier;
};

LabelCase::LabelCase (Context&		ctx,
					  const char*			name,
					  const char*			desc,
					  GLenum				identifier)
	: TestCase		(ctx, name, desc)
	, m_identifier	(identifier)
{
}

LabelCase::IterateResult LabelCase::iterate (void)
{
	TCU_CHECK_AND_THROW(NotSupportedError, isKHRDebugSupported(m_context), "GL_KHR_debug is not supported");

	const glw::Functions&	gl			= m_context.getRenderContext().getFunctions();
	const char*	const		msg			= "This is a debug label";
	GLuint					object		= 0;
	int						outlen		= -1;
	char					buffer[64];

	switch(m_identifier)
	{
		case GL_BUFFER:
			gl.genBuffers(1, &object);
			gl.bindBuffer(GL_ARRAY_BUFFER, object);
			gl.bindBuffer(GL_ARRAY_BUFFER, 0);
			break;

		case GL_SHADER:
			object = gl.createShader(GL_FRAGMENT_SHADER);
			break;

		case GL_PROGRAM:
			object = gl.createProgram();
			break;

		case GL_QUERY:
			gl.genQueries(1, &object);
			gl.beginQuery(GL_ANY_SAMPLES_PASSED, object); // Create
			gl.endQuery(GL_ANY_SAMPLES_PASSED); // Cleanup
			break;

		case GL_PROGRAM_PIPELINE:
			gl.genProgramPipelines(1, &object);
			gl.bindProgramPipeline(object); // Create
			gl.bindProgramPipeline(0); // Cleanup
			break;

		case GL_TRANSFORM_FEEDBACK:
			gl.genTransformFeedbacks(1, &object);
			gl.bindTransformFeedback(GL_TRANSFORM_FEEDBACK, object);
			gl.bindTransformFeedback(GL_TRANSFORM_FEEDBACK, 0);
			break;

		case GL_SAMPLER:
			gl.genSamplers(1, &object);
			gl.bindSampler(0, object);
			gl.bindSampler(0, 0);
			break;

		case GL_TEXTURE:
			gl.genTextures(1, &object);
			gl.bindTexture(GL_TEXTURE_2D, object);
			gl.bindTexture(GL_TEXTURE_2D, 0);
			break;

		case GL_RENDERBUFFER:
			gl.genRenderbuffers(1, &object);
			gl.bindRenderbuffer(GL_RENDERBUFFER, object);
			gl.bindRenderbuffer(GL_RENDERBUFFER, 0);
			break;

		case GL_FRAMEBUFFER:
			gl.genFramebuffers(1, &object);
			gl.bindFramebuffer(GL_DRAW_FRAMEBUFFER, object);
			gl.bindFramebuffer(GL_DRAW_FRAMEBUFFER, m_context.getRenderContext().getDefaultFramebuffer());
			break;

		default:
			DE_FATAL("Invalid identifier");
	}

	gl.objectLabel(m_identifier, object, -1, msg);

	deMemset(buffer, 'X', sizeof(buffer));
	gl.getObjectLabel(m_identifier, object, sizeof(buffer), &outlen, buffer);

	if (outlen == 0)
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Failed to query debug label from object");
	else if (deStringEqual(msg, buffer))
	{
		m_testCtx.getLog() << TestLog::Message << "Query returned string: \"" << buffer << "\"" << TestLog::EndMessage;
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		buffer[63] = '\0'; // make sure buffer is null terminated before printing
		m_testCtx.getLog() << TestLog::Message << "Query returned wrong string: expected \"" << msg << "\" but got \"" << buffer << "\"" << TestLog::EndMessage;
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Query returned wrong label");
	}

	switch(m_identifier)
	{
		case GL_BUFFER:				gl.deleteBuffers(1, &object);				break;
		case GL_SHADER:				gl.deleteShader(object);					break;
		case GL_PROGRAM:			gl.deleteProgram(object);					break;
		case GL_QUERY:				gl.deleteQueries(1, &object);				break;
		case GL_PROGRAM_PIPELINE:	gl.deleteProgramPipelines(1, &object);		break;
		case GL_TRANSFORM_FEEDBACK:	gl.deleteTransformFeedbacks(1, &object);	break;
		case GL_SAMPLER:			gl.deleteSamplers(1, &object);				break;
		case GL_TEXTURE:			gl.deleteTextures(1, &object);				break;
		case GL_RENDERBUFFER:		gl.deleteRenderbuffers(1, &object);			break;
		case GL_FRAMEBUFFER:		gl.deleteFramebuffers(1, &object);			break;

		default:
			DE_FATAL("Invalid identifier");
	}

	return STOP;
}


DebugMessageTestContext::DebugMessageTestContext (BaseCase&					host,
												  glu::RenderContext&		renderCtx,
												  const glu::ContextInfo&	ctxInfo,
												  tcu::TestLog&				log,
												  tcu::ResultCollector&		results,
												  bool						enableLog)
	: NegativeTestContext	(host, renderCtx, ctxInfo, log, results, enableLog)
	, m_debugHost			(host)
{
}

DebugMessageTestContext::~DebugMessageTestContext (void)
{
}

void DebugMessageTestContext::expectMessage (GLenum source, GLenum type)
{
	m_debugHost.expectMessage(source, type);
}

class SyncLabelCase : public TestCase
{
public:
							SyncLabelCase	(Context& ctx, const char* name, const char* desc);
	virtual IterateResult	iterate			(void);
};

SyncLabelCase::SyncLabelCase (Context& ctx, const char* name, const char* desc)
	: TestCase(ctx, name, desc)
{
}

SyncLabelCase::IterateResult SyncLabelCase::iterate (void)
{
	TCU_CHECK_AND_THROW(NotSupportedError, isKHRDebugSupported(m_context), "GL_KHR_debug is not supported");

	const glw::Functions&	gl			= m_context.getRenderContext().getFunctions();
	const char*	const		msg			= "This is a debug label";
	int						outlen		= -1;
	char					buffer[64];

	glw::GLsync				sync		= gl.fenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "fenceSync");

	gl.objectPtrLabel(sync, -1, msg);

	deMemset(buffer, 'X', sizeof(buffer));
	gl.getObjectPtrLabel(sync, sizeof(buffer), &outlen, buffer);

	if (outlen == 0)
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Failed to query debug label from object");
	else if (deStringEqual(msg, buffer))
	{
		m_testCtx.getLog() << TestLog::Message << "Query returned string: \"" << buffer << "\"" << TestLog::EndMessage;
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		buffer[63] = '\0'; // make sure buffer is null terminated before printing
		m_testCtx.getLog() << TestLog::Message << "Query returned wrong string: expected \"" << msg << "\" but got \"" << buffer << "\"" << TestLog::EndMessage;
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Query returned wrong label");
	}

	gl.deleteSync(sync);

	return STOP;
}

class InitialLabelCase : public TestCase
{
public:
							InitialLabelCase	(Context& ctx, const char* name, const char* desc);
	virtual IterateResult	iterate				(void);
};

InitialLabelCase::InitialLabelCase (Context& ctx, const char* name, const char* desc)
	: TestCase(ctx, name, desc)
{
}

InitialLabelCase::IterateResult InitialLabelCase::iterate (void)
{
	TCU_CHECK_AND_THROW(NotSupportedError, isKHRDebugSupported(m_context), "GL_KHR_debug is not supported");

	const glw::Functions&	gl			= m_context.getRenderContext().getFunctions();
	tcu::ResultCollector	result		(m_testCtx.getLog(), " // ERROR: ");
	int						outlen		= -1;
	GLuint					shader;
	glw::GLsync				sync;
	char					buffer[64];

	sync = gl.fenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
	GLS_COLLECT_GL_ERROR(result, gl.getError(), "fenceSync");

	shader = gl.createShader(GL_FRAGMENT_SHADER);
	GLS_COLLECT_GL_ERROR(result, gl.getError(), "createShader");

	{
		const tcu::ScopedLogSection section(m_testCtx.getLog(), "Shader", "Shader object");
		m_testCtx.getLog() << TestLog::Message << "Querying initial value" << TestLog::EndMessage;

		buffer[0] = 'X';
		outlen = -1;
		gl.getObjectLabel(GL_SHADER, shader, sizeof(buffer), &outlen, buffer);
		GLS_COLLECT_GL_ERROR(result, gl.getError(), "getObjectLabel");

		if (outlen != 0)
			result.fail("'length' was not zero, got " + de::toString(outlen));
		else if (buffer[0] != '\0')
			result.fail("label was not null terminated");
		else
			m_testCtx.getLog() << TestLog::Message << "Got 0-sized null-terminated string." << TestLog::EndMessage;
	}

	{
		const tcu::ScopedLogSection section(m_testCtx.getLog(), "Sync", "Sync object");
		m_testCtx.getLog() << TestLog::Message << "Querying initial value" << TestLog::EndMessage;

		buffer[0] = 'X';
		outlen = -1;
		gl.getObjectPtrLabel(sync, sizeof(buffer), &outlen, buffer);
		GLS_COLLECT_GL_ERROR(result, gl.getError(), "getObjectPtrLabel");

		if (outlen != 0)
			result.fail("'length' was not zero, got " + de::toString(outlen));
		else if (buffer[0] != '\0')
			result.fail("label was not null terminated");
		else
			m_testCtx.getLog() << TestLog::Message << "Got 0-sized null-terminated string." << TestLog::EndMessage;
	}

	gl.deleteShader(shader);
	gl.deleteSync(sync);

	result.setTestContextResult(m_testCtx);
	return STOP;
}

class ClearLabelCase : public TestCase
{
public:
							ClearLabelCase		(Context& ctx, const char* name, const char* desc);
	virtual IterateResult	iterate				(void);
};

ClearLabelCase::ClearLabelCase (Context& ctx, const char* name, const char* desc)
	: TestCase(ctx, name, desc)
{
}

ClearLabelCase::IterateResult ClearLabelCase::iterate (void)
{
	TCU_CHECK_AND_THROW(NotSupportedError, isKHRDebugSupported(m_context), "GL_KHR_debug is not supported");

	static const struct
	{
		const char*	description;
		int			length;
	} s_clearMethods[] =
	{
		{ " with NULL label and 0 length",			0	},
		{ " with NULL label and 1 length",			1	},
		{ " with NULL label and negative length",	-1	},
	};

	const glw::Functions&	gl			= m_context.getRenderContext().getFunctions();
	tcu::ResultCollector	result		(m_testCtx.getLog(), " // ERROR: ");
	const char*	const		msg			= "This is a debug label";
	int						outlen		= -1;
	GLuint					shader;
	glw::GLsync				sync;
	char					buffer[64];

	sync = gl.fenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
	GLS_COLLECT_GL_ERROR(result, gl.getError(), "fenceSync");

	shader = gl.createShader(GL_FRAGMENT_SHADER);
	GLS_COLLECT_GL_ERROR(result, gl.getError(), "createShader");

	{
		const tcu::ScopedLogSection section(m_testCtx.getLog(), "Shader", "Shader object");

		for (int methodNdx = 0; methodNdx < DE_LENGTH_OF_ARRAY(s_clearMethods); ++methodNdx)
		{
			m_testCtx.getLog() << TestLog::Message << "Setting label to string: \"" << msg << "\"" << TestLog::EndMessage;
			gl.objectLabel(GL_SHADER, shader, -2,  msg);
			GLS_COLLECT_GL_ERROR(result, gl.getError(), "objectLabel");

			m_testCtx.getLog() << TestLog::Message << "Clearing label " << s_clearMethods[methodNdx].description << TestLog::EndMessage;
			gl.objectLabel(GL_SHADER, shader, s_clearMethods[methodNdx].length, DE_NULL);
			GLS_COLLECT_GL_ERROR(result, gl.getError(), "objectLabel");

			m_testCtx.getLog() << TestLog::Message << "Querying label" << TestLog::EndMessage;
			buffer[0] = 'X';
			outlen = -1;
			gl.getObjectLabel(GL_SHADER, shader, sizeof(buffer), &outlen, buffer);
			GLS_COLLECT_GL_ERROR(result, gl.getError(), "getObjectLabel");

			if (outlen != 0)
				result.fail("'length' was not zero, got " + de::toString(outlen));
			else if (buffer[0] != '\0')
				result.fail("label was not null terminated");
			else
				m_testCtx.getLog() << TestLog::Message << "Got 0-sized null-terminated string." << TestLog::EndMessage;
		}
	}

	{
		const tcu::ScopedLogSection section(m_testCtx.getLog(), "Sync", "Sync object");

		for (int methodNdx = 0; methodNdx < DE_LENGTH_OF_ARRAY(s_clearMethods); ++methodNdx)
		{
			m_testCtx.getLog() << TestLog::Message << "Setting label to string: \"" << msg << "\"" << TestLog::EndMessage;
			gl.objectPtrLabel(sync, -2, msg);
			GLS_COLLECT_GL_ERROR(result, gl.getError(), "objectPtrLabel");

			m_testCtx.getLog() << TestLog::Message << "Clearing label " << s_clearMethods[methodNdx].description << TestLog::EndMessage;
			gl.objectPtrLabel(sync, s_clearMethods[methodNdx].length, DE_NULL);
			GLS_COLLECT_GL_ERROR(result, gl.getError(), "objectPtrLabel");

			m_testCtx.getLog() << TestLog::Message << "Querying label" << TestLog::EndMessage;
			buffer[0] = 'X';
			outlen = -1;
			gl.getObjectPtrLabel(sync, sizeof(buffer), &outlen, buffer);
			GLS_COLLECT_GL_ERROR(result, gl.getError(), "getObjectPtrLabel");

			if (outlen != 0)
				result.fail("'length' was not zero, got " + de::toString(outlen));
			else if (buffer[0] != '\0')
				result.fail("label was not null terminated");
			else
				m_testCtx.getLog() << TestLog::Message << "Got 0-sized null-terminated string." << TestLog::EndMessage;
		}
	}

	gl.deleteShader(shader);
	gl.deleteSync(sync);

	result.setTestContextResult(m_testCtx);
	return STOP;
}

class SpecifyWithLengthCase : public TestCase
{
public:
							SpecifyWithLengthCase	(Context& ctx, const char* name, const char* desc);
	virtual IterateResult	iterate					(void);
};

SpecifyWithLengthCase::SpecifyWithLengthCase (Context& ctx, const char* name, const char* desc)
	: TestCase(ctx, name, desc)
{
}

SpecifyWithLengthCase::IterateResult SpecifyWithLengthCase::iterate (void)
{
	TCU_CHECK_AND_THROW(NotSupportedError, isKHRDebugSupported(m_context), "GL_KHR_debug is not supported");

	const glw::Functions&	gl			= m_context.getRenderContext().getFunctions();
	tcu::ResultCollector	result		(m_testCtx.getLog(), " // ERROR: ");
	const char*	const		msg			= "This is a debug label";
	const char*	const		clipMsg		= "This is a de";
	int						outlen		= -1;
	GLuint					shader;
	glw::GLsync				sync;
	char					buffer[64];

	sync = gl.fenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
	GLS_COLLECT_GL_ERROR(result, gl.getError(), "fenceSync");

	shader = gl.createShader(GL_FRAGMENT_SHADER);
	GLS_COLLECT_GL_ERROR(result, gl.getError(), "createShader");

	{
		const tcu::ScopedLogSection section(m_testCtx.getLog(), "Shader", "Shader object");

		m_testCtx.getLog() << TestLog::Message << "Setting label to string: \"" << msg << "\" with length 12" << TestLog::EndMessage;
		gl.objectLabel(GL_SHADER, shader, 12, msg);
		GLS_COLLECT_GL_ERROR(result, gl.getError(), "objectLabel");

		m_testCtx.getLog() << TestLog::Message << "Querying label" << TestLog::EndMessage;
		deMemset(buffer, 'X', sizeof(buffer));
		gl.getObjectLabel(GL_SHADER, shader, sizeof(buffer), &outlen, buffer);
		GLS_COLLECT_GL_ERROR(result, gl.getError(), "getObjectLabel");

		if (outlen != 12)
			result.fail("'length' was not 12, got " + de::toString(outlen));
		else if (deStringEqual(clipMsg, buffer))
		{
			m_testCtx.getLog() << TestLog::Message << "Query returned string: \"" << buffer << "\"" << TestLog::EndMessage;
		}
		else
		{
			buffer[63] = '\0'; // make sure buffer is null terminated before printing
			m_testCtx.getLog() << TestLog::Message << "Query returned wrong string: expected \"" << clipMsg << "\" but got \"" << buffer << "\"" << TestLog::EndMessage;
			result.fail("Query returned wrong label");
		}
	}

	{
		const tcu::ScopedLogSection section(m_testCtx.getLog(), "Sync", "Sync object");

		m_testCtx.getLog() << TestLog::Message << "Setting label to string: \"" << msg << "\" with length 12" << TestLog::EndMessage;
		gl.objectPtrLabel(sync, 12, msg);
		GLS_COLLECT_GL_ERROR(result, gl.getError(), "objectPtrLabel");

		m_testCtx.getLog() << TestLog::Message << "Querying label" << TestLog::EndMessage;
		deMemset(buffer, 'X', sizeof(buffer));
		gl.getObjectPtrLabel(sync, sizeof(buffer), &outlen, buffer);
		GLS_COLLECT_GL_ERROR(result, gl.getError(), "getObjectPtrLabel");

		if (outlen != 12)
			result.fail("'length' was not 12, got " + de::toString(outlen));
		else if (deStringEqual(clipMsg, buffer))
		{
			m_testCtx.getLog() << TestLog::Message << "Query returned string: \"" << buffer << "\"" << TestLog::EndMessage;
		}
		else
		{
			buffer[63] = '\0'; // make sure buffer is null terminated before printing
			m_testCtx.getLog() << TestLog::Message << "Query returned wrong string: expected \"" << clipMsg << "\" but got \"" << buffer << "\"" << TestLog::EndMessage;
			result.fail("Query returned wrong label");
		}
	}

	{
		const tcu::ScopedLogSection section(m_testCtx.getLog(), "ZeroSized", "ZeroSized");

		m_testCtx.getLog() << TestLog::Message << "Setting label to string: \"" << msg << "\" with length 0" << TestLog::EndMessage;
		gl.objectLabel(GL_SHADER, shader, 0, msg);
		GLS_COLLECT_GL_ERROR(result, gl.getError(), "objectLabel");

		m_testCtx.getLog() << TestLog::Message << "Querying label" << TestLog::EndMessage;
		deMemset(buffer, 'X', sizeof(buffer));
		gl.getObjectLabel(GL_SHADER, shader, sizeof(buffer), &outlen, buffer);
		GLS_COLLECT_GL_ERROR(result, gl.getError(), "getObjectLabel");

		if (outlen != 0)
			result.fail("'length' was not zero, got " + de::toString(outlen));
		else if (buffer[0] != '\0')
			result.fail("label was not null terminated");
		else
			m_testCtx.getLog() << TestLog::Message << "Got 0-sized null-terminated string." << TestLog::EndMessage;
	}

	gl.deleteShader(shader);
	gl.deleteSync(sync);

	result.setTestContextResult(m_testCtx);
	return STOP;
}

class BufferLimitedLabelCase : public TestCase
{
public:
							BufferLimitedLabelCase	(Context& ctx, const char* name, const char* desc);
	virtual IterateResult	iterate					(void);
};

BufferLimitedLabelCase::BufferLimitedLabelCase (Context& ctx, const char* name, const char* desc)
	: TestCase(ctx, name, desc)
{
}

BufferLimitedLabelCase::IterateResult BufferLimitedLabelCase::iterate (void)
{
	TCU_CHECK_AND_THROW(NotSupportedError, isKHRDebugSupported(m_context), "GL_KHR_debug is not supported");

	const glw::Functions&	gl			= m_context.getRenderContext().getFunctions();
	tcu::ResultCollector	result		(m_testCtx.getLog(), " // ERROR: ");
	const char*	const		msg			= "This is a debug label";
	int						outlen		= -1;
	GLuint					shader;
	glw::GLsync				sync;
	char					buffer[64];

	sync = gl.fenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
	GLS_COLLECT_GL_ERROR(result, gl.getError(), "fenceSync");

	shader = gl.createShader(GL_FRAGMENT_SHADER);
	GLS_COLLECT_GL_ERROR(result, gl.getError(), "createShader");

	{
		const tcu::ScopedLogSection superSection(m_testCtx.getLog(), "Shader", "Shader object");

		m_testCtx.getLog() << TestLog::Message << "Setting label to string: \"" << msg << "\"" << TestLog::EndMessage;
		gl.objectLabel(GL_SHADER, shader, -1, msg);
		GLS_COLLECT_GL_ERROR(result, gl.getError(), "objectLabel");

		{
			const tcu::ScopedLogSection section(m_testCtx.getLog(), "QueryAll", "Query All");

			m_testCtx.getLog() << TestLog::Message << "Querying whole label, buffer size = 22" << TestLog::EndMessage;
			deMemset(buffer, 'X', sizeof(buffer));
			gl.getObjectLabel(GL_SHADER, shader, 22, &outlen, buffer);
			GLS_COLLECT_GL_ERROR(result, gl.getError(), "getObjectLabel");

			if (outlen != 21)
				result.fail("'length' was not 21, got " + de::toString(outlen));
			else if (buffer[outlen] != '\0')
				result.fail("Buffer was not null-terminated");
			else if (buffer[outlen+1] != 'X')
				result.fail("Query wrote over buffer bound");
			else if (!deStringEqual(msg, buffer))
			{
				buffer[63] = '\0'; // make sure buffer is null terminated before printing
				m_testCtx.getLog() << TestLog::Message << "Query returned string: \"" << buffer << "\"" << TestLog::EndMessage;
				result.fail("Query returned wrong label");
			}
			else
				m_testCtx.getLog() << TestLog::Message << "Query returned string: \"" << buffer << "\"" << TestLog::EndMessage;
		}
		{
			const tcu::ScopedLogSection section(m_testCtx.getLog(), "QueryAllNoSize", "Query all without size");

			m_testCtx.getLog() << TestLog::Message << "Querying whole label, buffer size = 22" << TestLog::EndMessage;
			deMemset(buffer, 'X', sizeof(buffer));
			gl.getObjectLabel(GL_SHADER, shader, 22, DE_NULL, buffer);
			GLS_COLLECT_GL_ERROR(result, gl.getError(), "getObjectLabel");

			buffer[63] = '\0'; // make sure buffer is null terminated before strlen

			if (strlen(buffer) != 21)
				result.fail("Buffer length was not 21");
			else if (buffer[21] != '\0')
				result.fail("Buffer was not null-terminated");
			else if (buffer[22] != 'X')
				result.fail("Query wrote over buffer bound");
			else if (!deStringEqual(msg, buffer))
			{
				m_testCtx.getLog() << TestLog::Message << "Query returned string: \"" << buffer << "\"" << TestLog::EndMessage;
				result.fail("Query returned wrong label");
			}
			else
				m_testCtx.getLog() << TestLog::Message << "Query returned string: \"" << buffer << "\"" << TestLog::EndMessage;
		}
		{
			const tcu::ScopedLogSection section(m_testCtx.getLog(), "QueryLess", "Query substring");

			m_testCtx.getLog() << TestLog::Message << "Querying whole label, buffer size = 2" << TestLog::EndMessage;
			deMemset(buffer, 'X', sizeof(buffer));
			gl.getObjectLabel(GL_SHADER, shader, 2, &outlen, buffer);
			GLS_COLLECT_GL_ERROR(result, gl.getError(), "getObjectLabel");

			if (outlen != 1)
				result.fail("'length' was not 1, got " + de::toString(outlen));
			else if (buffer[outlen] != '\0')
				result.fail("Buffer was not null-terminated");
			else if (buffer[outlen+1] != 'X')
				result.fail("Query wrote over buffer bound");
			else if (!deStringBeginsWith(msg, buffer))
			{
				buffer[63] = '\0'; // make sure buffer is null terminated before printing
				m_testCtx.getLog() << TestLog::Message << "Query returned string: \"" << buffer << "\"" << TestLog::EndMessage;
				result.fail("Query returned wrong label");
			}
			else
				m_testCtx.getLog() << TestLog::Message << "Query returned string: \"" << buffer << "\"" << TestLog::EndMessage;
		}
		{
			const tcu::ScopedLogSection section(m_testCtx.getLog(), "QueryNone", "Query one character");

			m_testCtx.getLog() << TestLog::Message << "Querying whole label, buffer size = 1" << TestLog::EndMessage;
			deMemset(buffer, 'X', sizeof(buffer));
			gl.getObjectLabel(GL_SHADER, shader, 1, &outlen, buffer);
			GLS_COLLECT_GL_ERROR(result, gl.getError(), "getObjectLabel");

			if (outlen != 0)
				result.fail("'length' was not 0, got " + de::toString(outlen));
			else if (buffer[outlen] != '\0')
				result.fail("Buffer was not null-terminated");
			else if (buffer[outlen+1] != 'X')
				result.fail("Query wrote over buffer bound");
			else
				m_testCtx.getLog() << TestLog::Message << "Query returned zero-sized null-terminated string" << TestLog::EndMessage;
		}
	}

	{
		const tcu::ScopedLogSection superSection(m_testCtx.getLog(), "Sync", "Sync object");

		m_testCtx.getLog() << TestLog::Message << "Setting label to string: \"" << msg << "\"" << TestLog::EndMessage;
		gl.objectPtrLabel(sync, -1, msg);
		GLS_COLLECT_GL_ERROR(result, gl.getError(), "objectPtrLabel");

		{
			const tcu::ScopedLogSection section(m_testCtx.getLog(), "QueryAll", "Query All");

			m_testCtx.getLog() << TestLog::Message << "Querying whole label, buffer size = 22" << TestLog::EndMessage;
			deMemset(buffer, 'X', sizeof(buffer));
			gl.getObjectPtrLabel(sync, 22, &outlen, buffer);
			GLS_COLLECT_GL_ERROR(result, gl.getError(), "getObjectPtrLabel");

			if (outlen != 21)
				result.fail("'length' was not 21, got " + de::toString(outlen));
			else if (buffer[outlen] != '\0')
				result.fail("Buffer was not null-terminated");
			else if (buffer[outlen+1] != 'X')
				result.fail("Query wrote over buffer bound");
			else if (!deStringEqual(msg, buffer))
			{
				buffer[63] = '\0'; // make sure buffer is null terminated before printing
				m_testCtx.getLog() << TestLog::Message << "Query returned string: \"" << buffer << "\"" << TestLog::EndMessage;
				result.fail("Query returned wrong label");
			}
			else
				m_testCtx.getLog() << TestLog::Message << "Query returned string: \"" << buffer << "\"" << TestLog::EndMessage;
		}
		{
			const tcu::ScopedLogSection section(m_testCtx.getLog(), "QueryAllNoSize", "Query all without size");

			m_testCtx.getLog() << TestLog::Message << "Querying whole label, buffer size = 22" << TestLog::EndMessage;
			deMemset(buffer, 'X', sizeof(buffer));
			gl.getObjectPtrLabel(sync, 22, DE_NULL, buffer);
			GLS_COLLECT_GL_ERROR(result, gl.getError(), "getObjectPtrLabel");

			buffer[63] = '\0'; // make sure buffer is null terminated before strlen

			if (strlen(buffer) != 21)
				result.fail("Buffer length was not 21");
			else if (buffer[21] != '\0')
				result.fail("Buffer was not null-terminated");
			else if (buffer[22] != 'X')
				result.fail("Query wrote over buffer bound");
			else if (!deStringEqual(msg, buffer))
			{
				m_testCtx.getLog() << TestLog::Message << "Query returned string: \"" << buffer << "\"" << TestLog::EndMessage;
				result.fail("Query returned wrong label");
			}
			else
				m_testCtx.getLog() << TestLog::Message << "Query returned string: \"" << buffer << "\"" << TestLog::EndMessage;
		}
		{
			const tcu::ScopedLogSection section(m_testCtx.getLog(), "QueryLess", "Query substring");

			m_testCtx.getLog() << TestLog::Message << "Querying whole label, buffer size = 2" << TestLog::EndMessage;
			deMemset(buffer, 'X', sizeof(buffer));
			gl.getObjectPtrLabel(sync, 2, &outlen, buffer);
			GLS_COLLECT_GL_ERROR(result, gl.getError(), "getObjectPtrLabel");

			if (outlen != 1)
				result.fail("'length' was not 1, got " + de::toString(outlen));
			else if (buffer[outlen] != '\0')
				result.fail("Buffer was not null-terminated");
			else if (buffer[outlen+1] != 'X')
				result.fail("Query wrote over buffer bound");
			else if (!deStringBeginsWith(msg, buffer))
			{
				buffer[63] = '\0'; // make sure buffer is null terminated before printing
				m_testCtx.getLog() << TestLog::Message << "Query returned string: \"" << buffer << "\"" << TestLog::EndMessage;
				result.fail("Query returned wrong label");
			}
			else
				m_testCtx.getLog() << TestLog::Message << "Query returned string: \"" << buffer << "\"" << TestLog::EndMessage;
		}
		{
			const tcu::ScopedLogSection section(m_testCtx.getLog(), "QueryNone", "Query one character");

			m_testCtx.getLog() << TestLog::Message << "Querying whole label, buffer size = 1" << TestLog::EndMessage;
			deMemset(buffer, 'X', sizeof(buffer));
			gl.getObjectPtrLabel(sync, 1, &outlen, buffer);
			GLS_COLLECT_GL_ERROR(result, gl.getError(), "getObjectPtrLabel");

			if (outlen != 0)
				result.fail("'length' was not 0, got " + de::toString(outlen));
			else if (buffer[outlen] != '\0')
				result.fail("Buffer was not null-terminated");
			else if (buffer[outlen+1] != 'X')
				result.fail("Query wrote over buffer bound");
			else
				m_testCtx.getLog() << TestLog::Message << "Query returned zero-sized null-terminated string" << TestLog::EndMessage;
		}
	}

	gl.deleteShader(shader);
	gl.deleteSync(sync);

	result.setTestContextResult(m_testCtx);
	return STOP;
}

class LabelMaxSizeCase : public TestCase
{
public:
							LabelMaxSizeCase	(Context& ctx, const char* name, const char* desc);
	virtual IterateResult	iterate				(void);
};

LabelMaxSizeCase::LabelMaxSizeCase (Context& ctx, const char* name, const char* desc)
	: TestCase(ctx, name, desc)
{
}

LabelMaxSizeCase::IterateResult LabelMaxSizeCase::iterate (void)
{
	TCU_CHECK_AND_THROW(NotSupportedError, isKHRDebugSupported(m_context), "GL_KHR_debug is not supported");

	const glw::Functions&	gl			= m_context.getRenderContext().getFunctions();
	tcu::ResultCollector	result		(m_testCtx.getLog(), " // ERROR: ");
	int						maxLabelLen	= -1;
	int						outlen		= -1;
	GLuint					shader;
	glw::GLsync				sync;

	gl.getIntegerv(GL_MAX_LABEL_LENGTH, &maxLabelLen);
	GLS_COLLECT_GL_ERROR(result, gl.getError(), "GL_MAX_LABEL_LENGTH");

	m_testCtx.getLog() << TestLog::Message << "GL_MAX_LABEL_LENGTH = " << maxLabelLen << TestLog::EndMessage;

	if (maxLabelLen < 256)
		throw tcu::TestError("maxLabelLen was less than required (256)");
	if (maxLabelLen > 8192)
	{
		m_testCtx.getLog()
			<< TestLog::Message
			<< "GL_MAX_LABEL_LENGTH is very large. Application having larger labels is unlikely, skipping test."
			<< TestLog::EndMessage;
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
		return STOP;
	}

	sync = gl.fenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
	GLS_COLLECT_GL_ERROR(result, gl.getError(), "fenceSync");

	shader = gl.createShader(GL_FRAGMENT_SHADER);
	GLS_COLLECT_GL_ERROR(result, gl.getError(), "createShader");

	{
		const tcu::ScopedLogSection	section		(m_testCtx.getLog(), "Shader", "Shader object");
		std::vector<char>			buffer		(maxLabelLen, 'X');
		std::vector<char>			readBuffer	(maxLabelLen, 'X');

		buffer[maxLabelLen-1] = '\0';

		m_testCtx.getLog() << TestLog::Message << "Setting max length label, with implicit size. (length = -1)" << TestLog::EndMessage;
		gl.objectLabel(GL_SHADER, shader, -1,  &buffer[0]);
		GLS_COLLECT_GL_ERROR(result, gl.getError(), "objectLabel");

		m_testCtx.getLog() << TestLog::Message << "Querying label back" << TestLog::EndMessage;
		outlen = -1;
		gl.getObjectLabel(GL_SHADER, shader, maxLabelLen, &outlen, &readBuffer[0]);
		GLS_COLLECT_GL_ERROR(result, gl.getError(), "getObjectLabel");

		if (outlen != maxLabelLen-1)
			result.fail("'length' was not " + de::toString(maxLabelLen-1) + ", got " + de::toString(outlen));
		else if (readBuffer[outlen] != '\0')
			result.fail("Buffer was not null-terminated");

		m_testCtx.getLog() << TestLog::Message << "Setting max length label, with explicit size. (length = " << (maxLabelLen-1) << ")" << TestLog::EndMessage;
		gl.objectLabel(GL_SHADER, shader, maxLabelLen-1,  &buffer[0]);
		GLS_COLLECT_GL_ERROR(result, gl.getError(), "objectLabel");

		m_testCtx.getLog() << TestLog::Message << "Querying label back" << TestLog::EndMessage;
		outlen = -1;
		readBuffer[maxLabelLen-1] = 'X';
		gl.getObjectLabel(GL_SHADER, shader, maxLabelLen, &outlen, &readBuffer[0]);
		GLS_COLLECT_GL_ERROR(result, gl.getError(), "getObjectLabel");

		if (outlen != maxLabelLen-1)
			result.fail("'length' was not " + de::toString(maxLabelLen-1) + ", got " + de::toString(outlen));
		else if (readBuffer[outlen] != '\0')
			result.fail("Buffer was not null-terminated");
	}

	{
		const tcu::ScopedLogSection section		(m_testCtx.getLog(), "Sync", "Sync object");
		std::vector<char>			buffer		(maxLabelLen, 'X');
		std::vector<char>			readBuffer	(maxLabelLen, 'X');

		buffer[maxLabelLen-1] = '\0';

		m_testCtx.getLog() << TestLog::Message << "Setting max length label, with implicit size. (length = -1)" << TestLog::EndMessage;
		gl.objectPtrLabel(sync, -1,  &buffer[0]);
		GLS_COLLECT_GL_ERROR(result, gl.getError(), "objectPtrLabel");

		m_testCtx.getLog() << TestLog::Message << "Querying label back" << TestLog::EndMessage;
		outlen = -1;
		gl.getObjectPtrLabel(sync, maxLabelLen, &outlen, &readBuffer[0]);
		GLS_COLLECT_GL_ERROR(result, gl.getError(), "getObjectPtrLabel");

		if (outlen != maxLabelLen-1)
			result.fail("'length' was not " + de::toString(maxLabelLen-1) + ", got " + de::toString(outlen));
		else if (readBuffer[outlen] != '\0')
			result.fail("Buffer was not null-terminated");

		m_testCtx.getLog() << TestLog::Message << "Setting max length label, with explicit size. (length = " << (maxLabelLen-1) << ")" << TestLog::EndMessage;
		gl.objectPtrLabel(sync, maxLabelLen-1,  &buffer[0]);
		GLS_COLLECT_GL_ERROR(result, gl.getError(), "objectPtrLabel");

		m_testCtx.getLog() << TestLog::Message << "Querying label back" << TestLog::EndMessage;
		outlen = -1;
		readBuffer[maxLabelLen-1] = 'X';
		gl.getObjectPtrLabel(sync, maxLabelLen, &outlen, &readBuffer[0]);
		GLS_COLLECT_GL_ERROR(result, gl.getError(), "getObjectPtrLabel");

		if (outlen != maxLabelLen-1)
			result.fail("'length' was not " + de::toString(maxLabelLen-1) + ", got " + de::toString(outlen));
		else if (readBuffer[outlen] != '\0')
			result.fail("Buffer was not null-terminated");
	}

	gl.deleteShader(shader);
	gl.deleteSync(sync);

	result.setTestContextResult(m_testCtx);
	return STOP;
}

class LabelLengthCase : public TestCase
{
public:
							LabelLengthCase	(Context& ctx, const char* name, const char* desc);
	virtual IterateResult	iterate			(void);
};

LabelLengthCase::LabelLengthCase (Context& ctx, const char* name, const char* desc)
	: TestCase(ctx, name, desc)
{
}

LabelLengthCase::IterateResult LabelLengthCase::iterate (void)
{
	TCU_CHECK_AND_THROW(NotSupportedError, isKHRDebugSupported(m_context), "GL_KHR_debug is not supported");

	const glw::Functions&	gl			= m_context.getRenderContext().getFunctions();
	tcu::ResultCollector	result		(m_testCtx.getLog(), " // ERROR: ");
	const char*	const		msg			= "This is a debug label";
	int						outlen		= -1;
	GLuint					shader;
	glw::GLsync				sync;

	sync = gl.fenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
	GLS_COLLECT_GL_ERROR(result, gl.getError(), "fenceSync");

	shader = gl.createShader(GL_FRAGMENT_SHADER);
	GLS_COLLECT_GL_ERROR(result, gl.getError(), "createShader");

	{
		const tcu::ScopedLogSection section(m_testCtx.getLog(), "Shader", "Shader object");

		m_testCtx.getLog() << TestLog::Message << "Querying label length" << TestLog::EndMessage;
		outlen = -1;
		gl.getObjectLabel(GL_SHADER, shader, 0, &outlen, DE_NULL);
		GLS_COLLECT_GL_ERROR(result, gl.getError(), "getObjectLabel");

		if (outlen != 0)
			result.fail("'length' was not 0, got " + de::toString(outlen));
		else
			m_testCtx.getLog() << TestLog::Message << "Query returned length: " << outlen << TestLog::EndMessage;

		m_testCtx.getLog() << TestLog::Message << "Setting label to string: \"" << msg << "\"" << TestLog::EndMessage;
		gl.objectLabel(GL_SHADER, shader, -1, msg);
		GLS_COLLECT_GL_ERROR(result, gl.getError(), "objectLabel");

		m_testCtx.getLog() << TestLog::Message << "Querying label length" << TestLog::EndMessage;
		outlen = -1;
		gl.getObjectLabel(GL_SHADER, shader, 0, &outlen, DE_NULL);
		GLS_COLLECT_GL_ERROR(result, gl.getError(), "getObjectLabel");

		if (outlen != 21)
			result.fail("'length' was not 21, got " + de::toString(outlen));
		else
			m_testCtx.getLog() << TestLog::Message << "Query returned length: " << outlen << TestLog::EndMessage;
	}

	{
		const tcu::ScopedLogSection section(m_testCtx.getLog(), "Sync", "Sync object");

		m_testCtx.getLog() << TestLog::Message << "Querying label length" << TestLog::EndMessage;
		outlen = -1;
		gl.getObjectPtrLabel(sync, 0, &outlen, DE_NULL);
		GLS_COLLECT_GL_ERROR(result, gl.getError(), "getObjectPtrLabel");

		if (outlen != 0)
			result.fail("'length' was not 0, got " + de::toString(outlen));
		else
			m_testCtx.getLog() << TestLog::Message << "Query returned length: " << outlen << TestLog::EndMessage;

		m_testCtx.getLog() << TestLog::Message << "Setting label to string: \"" << msg << "\"" << TestLog::EndMessage;
		gl.objectPtrLabel(sync, -1, msg);
		GLS_COLLECT_GL_ERROR(result, gl.getError(), "objectPtrLabel");

		m_testCtx.getLog() << TestLog::Message << "Querying label length" << TestLog::EndMessage;
		outlen = -1;
		gl.getObjectPtrLabel(sync, 0, &outlen, DE_NULL);
		GLS_COLLECT_GL_ERROR(result, gl.getError(), "getObjectPtrLabel");

		if (outlen != 21)
			result.fail("'length' was not 21, got " + de::toString(outlen));
		else
			m_testCtx.getLog() << TestLog::Message << "Query returned length: " << outlen << TestLog::EndMessage;
	}

	gl.deleteShader(shader);
	gl.deleteSync(sync);

	result.setTestContextResult(m_testCtx);
	return STOP;
}

class LimitQueryCase : public TestCase
{
public:
											LimitQueryCase	(Context&						context,
															 const char*					name,
															 const char*					description,
															 glw::GLenum					target,
															 int							limit,
															 gls::StateQueryUtil::QueryType	type);

	IterateResult							iterate			(void);
private:
	const gls::StateQueryUtil::QueryType	m_type;
	const int								m_limit;
	const glw::GLenum						m_target;
};

LimitQueryCase::LimitQueryCase (Context&						context,
								const char*						name,
								const char*						description,
								glw::GLenum						target,
								int								limit,
								gls::StateQueryUtil::QueryType	type)
	: TestCase	(context, name, description)
	, m_type	(type)
	, m_limit	(limit)
	, m_target	(target)
{
}

LimitQueryCase::IterateResult LimitQueryCase::iterate (void)
{
	TCU_CHECK_AND_THROW(NotSupportedError, isKHRDebugSupported(m_context), "GL_KHR_debug is not supported");

	glu::CallLogWrapper		gl		(m_context.getRenderContext().getFunctions(), m_testCtx.getLog());
	tcu::ResultCollector	result	(m_testCtx.getLog(), " // ERROR: ");

	gl.enableLogging(true);
	gls::StateQueryUtil::verifyStateIntegerMin(result, gl, m_target, m_limit, m_type);

	result.setTestContextResult(m_testCtx);
	return STOP;
}

class IsEnabledCase : public TestCase
{
public:
	enum InitialValue
	{
		INITIAL_CTX_IS_DEBUG = 0,
		INITIAL_FALSE,
	};

											IsEnabledCase	(Context&						context,
															 const char*					name,
															 const char*					description,
															 glw::GLenum					target,
															 InitialValue					initial,
															 gls::StateQueryUtil::QueryType	type);

	IterateResult							iterate			(void);
private:
	const gls::StateQueryUtil::QueryType	m_type;
	const glw::GLenum						m_target;
	const InitialValue						m_initial;
};

IsEnabledCase::IsEnabledCase (Context&							context,
							  const char*						name,
							  const char*						description,
							  glw::GLenum						target,
							  InitialValue						initial,
							  gls::StateQueryUtil::QueryType	type)
	: TestCase	(context, name, description)
	, m_type	(type)
	, m_target	(target)
	, m_initial	(initial)
{
}

IsEnabledCase::IterateResult IsEnabledCase::iterate (void)
{
	TCU_CHECK_AND_THROW(NotSupportedError, isKHRDebugSupported(m_context), "GL_KHR_debug is not supported");

	glu::CallLogWrapper		gl		(m_context.getRenderContext().getFunctions(), m_testCtx.getLog());
	tcu::ResultCollector	result	(m_testCtx.getLog(), " // ERROR: ");
	bool					initial;

	gl.enableLogging(true);

	if (m_initial == INITIAL_FALSE)
		initial = false;
	else
	{
		DE_ASSERT(m_initial == INITIAL_CTX_IS_DEBUG);
		initial = (m_context.getRenderContext().getType().getFlags() & glu::CONTEXT_DEBUG) != 0;
	}

	// check inital value
	gls::StateQueryUtil::verifyStateBoolean(result, gl, m_target, initial, m_type);

	// check toggle

	gl.glEnable(m_target);
	GLS_COLLECT_GL_ERROR(result, gl.glGetError(), "glEnable");

	gls::StateQueryUtil::verifyStateBoolean(result, gl, m_target, true, m_type);

	gl.glDisable(m_target);
	GLS_COLLECT_GL_ERROR(result, gl.glGetError(), "glDisable");

	gls::StateQueryUtil::verifyStateBoolean(result, gl, m_target, false, m_type);

	result.setTestContextResult(m_testCtx);
	return STOP;
}

class PositiveIntegerCase : public TestCase
{
public:
											PositiveIntegerCase	(Context&						context,
																 const char*					name,
																 const char*					description,
																 glw::GLenum					target,
																 gls::StateQueryUtil::QueryType	type);

	IterateResult							iterate			(void);
private:
	const gls::StateQueryUtil::QueryType	m_type;
	const glw::GLenum						m_target;
};

PositiveIntegerCase::PositiveIntegerCase (Context&							context,
										  const char*						name,
										  const char*						description,
										  glw::GLenum						target,
										  gls::StateQueryUtil::QueryType	type)
	: TestCase	(context, name, description)
	, m_type	(type)
	, m_target	(target)
{
}

PositiveIntegerCase::IterateResult PositiveIntegerCase::iterate (void)
{
	TCU_CHECK_AND_THROW(NotSupportedError, isKHRDebugSupported(m_context), "GL_KHR_debug is not supported");

	glu::CallLogWrapper		gl		(m_context.getRenderContext().getFunctions(), m_testCtx.getLog());
	tcu::ResultCollector	result	(m_testCtx.getLog(), " // ERROR: ");

	gl.enableLogging(true);
	gls::StateQueryUtil::verifyStateIntegerMin(result, gl, m_target, 0, m_type);

	result.setTestContextResult(m_testCtx);
	return STOP;
}

class GroupStackDepthQueryCase : public TestCase
{
public:
											GroupStackDepthQueryCase	(Context&						context,
																		 const char*					name,
																		 const char*					description,
																		 gls::StateQueryUtil::QueryType	type);

	IterateResult							iterate			(void);
private:
	const gls::StateQueryUtil::QueryType	m_type;
};

GroupStackDepthQueryCase::GroupStackDepthQueryCase (Context&						context,
													const char*						name,
													const char*						description,
													gls::StateQueryUtil::QueryType	type)
	: TestCase	(context, name, description)
	, m_type	(type)
{
}

GroupStackDepthQueryCase::IterateResult GroupStackDepthQueryCase::iterate (void)
{
	TCU_CHECK_AND_THROW(NotSupportedError, isKHRDebugSupported(m_context), "GL_KHR_debug is not supported");

	glu::CallLogWrapper		gl		(m_context.getRenderContext().getFunctions(), m_testCtx.getLog());
	tcu::ResultCollector	result	(m_testCtx.getLog(), " // ERROR: ");

	gl.enableLogging(true);

	{
		const tcu::ScopedLogSection	section(m_testCtx.getLog(), "Initial", "Initial");

		gls::StateQueryUtil::verifyStateInteger(result, gl, GL_DEBUG_GROUP_STACK_DEPTH, 1, m_type);
	}

	{
		const tcu::ScopedLogSection	section(m_testCtx.getLog(), "Scoped", "Scoped");

		gl.glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 1, -1, "Application group 1");
		gls::StateQueryUtil::verifyStateInteger(result, gl, GL_DEBUG_GROUP_STACK_DEPTH, 2, m_type);
		gl.glPopDebugGroup();
	}

	result.setTestContextResult(m_testCtx);
	return STOP;
}

extern "C" void GLW_APIENTRY dummyCallback(GLenum, GLenum, GLuint, GLenum, GLsizei, const char*, const void*)
{
	// dummy
}

class DebugCallbackFunctionCase : public TestCase
{
public:
					DebugCallbackFunctionCase	(Context& context, const char* name, const char* description);
	IterateResult	iterate						(void);
};

DebugCallbackFunctionCase::DebugCallbackFunctionCase (Context& context, const char* name, const char* description)
	: TestCase	(context, name, description)
{
}

DebugCallbackFunctionCase::IterateResult DebugCallbackFunctionCase::iterate (void)
{
	using namespace gls::StateQueryUtil;
	TCU_CHECK_AND_THROW(NotSupportedError, isKHRDebugSupported(m_context), "GL_KHR_debug is not supported");

	glu::CallLogWrapper		gl		(m_context.getRenderContext().getFunctions(), m_testCtx.getLog());
	tcu::ResultCollector	result	(m_testCtx.getLog(), " // ERROR: ");

	gl.enableLogging(true);

	{
		const tcu::ScopedLogSection	section(m_testCtx.getLog(), "Initial", "Initial");

		verifyStatePointer(result, gl, GL_DEBUG_CALLBACK_FUNCTION, 0, QUERY_POINTER);
	}

	{
		const tcu::ScopedLogSection	section(m_testCtx.getLog(), "Set", "Set");

		gl.glDebugMessageCallback(dummyCallback, DE_NULL);
		verifyStatePointer(result, gl, GL_DEBUG_CALLBACK_FUNCTION, (const void*)dummyCallback, QUERY_POINTER);
	}

	result.setTestContextResult(m_testCtx);
	return STOP;
}

class DebugCallbackUserParamCase : public TestCase
{
public:
					DebugCallbackUserParamCase	(Context& context, const char* name, const char* description);
	IterateResult	iterate						(void);
};

DebugCallbackUserParamCase::DebugCallbackUserParamCase (Context& context, const char* name, const char* description)
	: TestCase	(context, name, description)
{
}

DebugCallbackUserParamCase::IterateResult DebugCallbackUserParamCase::iterate (void)
{
	using namespace gls::StateQueryUtil;

	TCU_CHECK_AND_THROW(NotSupportedError, isKHRDebugSupported(m_context), "GL_KHR_debug is not supported");

	glu::CallLogWrapper		gl		(m_context.getRenderContext().getFunctions(), m_testCtx.getLog());
	tcu::ResultCollector	result	(m_testCtx.getLog(), " // ERROR: ");

	gl.enableLogging(true);

	{
		const tcu::ScopedLogSection	section(m_testCtx.getLog(), "Initial", "Initial");

		verifyStatePointer(result, gl, GL_DEBUG_CALLBACK_USER_PARAM, 0, QUERY_POINTER);
	}

	{
		const tcu::ScopedLogSection	section	(m_testCtx.getLog(), "Set", "Set");
		const void*					param	= (void*)(int*)0x123;

		gl.glDebugMessageCallback(dummyCallback, param);
		verifyStatePointer(result, gl, GL_DEBUG_CALLBACK_USER_PARAM, param, QUERY_POINTER);
	}

	result.setTestContextResult(m_testCtx);
	return STOP;
}

} // anonymous

DebugTests::DebugTests (Context& context)
	: TestCaseGroup(context, "debug", "Debug tests")
{
}

enum CaseType
{
	CASETYPE_CALLBACK = 0,
	CASETYPE_LOG,
	CASETYPE_GETERROR,

	CASETYPE_LAST
};

tcu::TestNode* createCase (CaseType type, Context& ctx, const char* name, const char* desc, TestFunctionWrapper function)
{
	switch(type)
	{
		case CASETYPE_CALLBACK: return new CallbackErrorCase(ctx, name, desc, function);
		case CASETYPE_LOG:		return new LogErrorCase(ctx, name, desc, function);
		case CASETYPE_GETERROR: return new GetErrorCase(ctx, name, desc, function);

		default:
			DE_FATAL("Invalid type");
	}

	return DE_NULL;
}

tcu::TestCaseGroup* createChildCases (CaseType type, Context& ctx, const char* name, const char* desc, const vector<FunctionContainer>& funcs)
{
	tcu::TestCaseGroup* host = new tcu::TestCaseGroup(ctx.getTestContext(), name, desc);

	for (size_t ndx = 0; ndx < funcs.size(); ndx++)
			host->addChild(createCase(type, ctx, funcs[ndx].name, funcs[ndx].desc, funcs[ndx].function));

	return host;
}

vector<FunctionContainer> wrapCoreFunctions (const vector<NegativeTestShared::FunctionContainer>& fns)
{
	vector<FunctionContainer> retVal;

	retVal.resize(fns.size());
	for (int ndx = 0; ndx < (int)fns.size(); ++ndx)
	{
		retVal[ndx].function = TestFunctionWrapper(fns[ndx].function);
		retVal[ndx].name = fns[ndx].name;
		retVal[ndx].desc = fns[ndx].desc;
	}

	return retVal;
}

void DebugTests::init (void)
{
	const vector<FunctionContainer> bufferFuncs				 = wrapCoreFunctions(NegativeTestShared::getNegativeBufferApiTestFunctions());
	const vector<FunctionContainer> textureFuncs			 = wrapCoreFunctions(NegativeTestShared::getNegativeTextureApiTestFunctions());
	const vector<FunctionContainer> shaderFuncs				 = wrapCoreFunctions(NegativeTestShared::getNegativeShaderApiTestFunctions());
	const vector<FunctionContainer> fragmentFuncs			 = wrapCoreFunctions(NegativeTestShared::getNegativeFragmentApiTestFunctions());
	const vector<FunctionContainer> vaFuncs					 = wrapCoreFunctions(NegativeTestShared::getNegativeVertexArrayApiTestFunctions());
	const vector<FunctionContainer> stateFuncs				 = wrapCoreFunctions(NegativeTestShared::getNegativeStateApiTestFunctions());
	const vector<FunctionContainer> tessellationFuncs		 = wrapCoreFunctions(NegativeTestShared::getNegativeTessellationTestFunctions());
	const vector<FunctionContainer> atomicCounterFuncs		 = wrapCoreFunctions(NegativeTestShared::getNegativeAtomicCounterTestFunctions());
	const vector<FunctionContainer> imageLoadFuncs			 = wrapCoreFunctions(NegativeTestShared::getNegativeShaderImageLoadTestFunctions());
	const vector<FunctionContainer> imageStoreFuncs			 = wrapCoreFunctions(NegativeTestShared::getNegativeShaderImageStoreTestFunctions());
	const vector<FunctionContainer> imageAtomicFuncs		 = wrapCoreFunctions(NegativeTestShared::getNegativeShaderImageAtomicTestFunctions());
	const vector<FunctionContainer> imageAtomicExchangeFuncs = wrapCoreFunctions(NegativeTestShared::getNegativeShaderImageAtomicExchangeTestFunctions());
	const vector<FunctionContainer> shaderFunctionFuncs		 = wrapCoreFunctions(NegativeTestShared::getNegativeShaderFunctionTestFunctions());
	const vector<FunctionContainer> shaderDirectiveFuncs	 = wrapCoreFunctions(NegativeTestShared::getNegativeShaderDirectiveTestFunctions());
	const vector<FunctionContainer> ssboBlockFuncs			 = wrapCoreFunctions(NegativeTestShared::getNegativeSSBOBlockTestFunctions());
	const vector<FunctionContainer> preciseFuncs			 = wrapCoreFunctions(NegativeTestShared::getNegativePreciseTestFunctions());
	const vector<FunctionContainer> advancedBlendFuncs		 = wrapCoreFunctions(NegativeTestShared::getNegativeAdvancedBlendEquationTestFunctions());
	const vector<FunctionContainer> shaderStorageFuncs		 = wrapCoreFunctions(NegativeTestShared::getNegativeShaderStorageTestFunctions());
	const vector<FunctionContainer> sampleVariablesFuncs	 = wrapCoreFunctions(NegativeTestShared::getNegativeSampleVariablesTestFunctions());
	const vector<FunctionContainer> computeFuncs			 = wrapCoreFunctions(NegativeTestShared::getNegativeComputeTestFunctions());
	const vector<FunctionContainer> framebufferFetchFuncs    = wrapCoreFunctions(NegativeTestShared::getNegativeShaderFramebufferFetchTestFunctions());
	const vector<FunctionContainer> externalFuncs			 = getUserMessageFuncs();

	{
		using namespace gls::StateQueryUtil;

		tcu::TestCaseGroup* const queries = new tcu::TestCaseGroup(m_testCtx, "state_query", "State query");

		static const struct
		{
			const char*	name;
			const char*	targetName;
			glw::GLenum	target;
			int			limit;
		} limits[] =
		{
			{ "max_debug_message_length",		"MAX_DEBUG_MESSAGE_LENGTH",		GL_MAX_DEBUG_MESSAGE_LENGTH,	1	},
			{ "max_debug_logged_messages",		"MAX_DEBUG_LOGGED_MESSAGES",	GL_MAX_DEBUG_LOGGED_MESSAGES,	1	},
			{ "max_debug_group_stack_depth",	"MAX_DEBUG_GROUP_STACK_DEPTH",	GL_MAX_DEBUG_GROUP_STACK_DEPTH,	64	},
			{ "max_label_length",				"MAX_LABEL_LENGTH",				GL_MAX_LABEL_LENGTH,			256	},
		};

		addChild(queries);

		#define FOR_ALL_TYPES(X) \
			do \
			{ \
				{ \
					const char* const	postfix = "_getboolean"; \
					const QueryType		queryType = QUERY_BOOLEAN; \
					X; \
				} \
				{ \
					const char* const	postfix = "_getinteger"; \
					const QueryType		queryType = QUERY_INTEGER; \
					X; \
				} \
				{ \
					const char* const	postfix = "_getinteger64"; \
					const QueryType		queryType = QUERY_INTEGER64; \
					X; \
				} \
				{ \
					const char* const	postfix = "_getfloat"; \
					const QueryType		queryType = QUERY_FLOAT; \
					X; \
				} \
			} \
			while (deGetFalse())
		#define FOR_ALL_ENABLE_TYPES(X) \
			do \
			{ \
				{ \
					const char* const	postfix = "_isenabled"; \
					const QueryType		queryType = QUERY_ISENABLED; \
					X; \
				} \
				FOR_ALL_TYPES(X); \
			} \
			while (deGetFalse())

		for (int ndx = 0; ndx < DE_LENGTH_OF_ARRAY(limits); ++ndx)
		{
			FOR_ALL_TYPES(queries->addChild(new LimitQueryCase(m_context,
															   (std::string(limits[ndx].name) + postfix).c_str(),
															   (std::string("Test ") + limits[ndx].targetName).c_str(),
															   limits[ndx].target, limits[ndx].limit, queryType)));
		}

		FOR_ALL_ENABLE_TYPES(queries->addChild(new IsEnabledCase	(m_context, (std::string("debug_output") + postfix).c_str(),						"Test DEBUG_OUTPUT",						GL_DEBUG_OUTPUT,				IsEnabledCase::INITIAL_CTX_IS_DEBUG,	queryType)));
		FOR_ALL_ENABLE_TYPES(queries->addChild(new IsEnabledCase	(m_context, (std::string("debug_output_synchronous") + postfix).c_str(),			"Test DEBUG_OUTPUT_SYNCHRONOUS",			GL_DEBUG_OUTPUT_SYNCHRONOUS,	IsEnabledCase::INITIAL_FALSE,			queryType)));

		FOR_ALL_TYPES(queries->addChild(new PositiveIntegerCase		(m_context, (std::string("debug_logged_messages") + postfix).c_str(),				"Test DEBUG_LOGGED_MESSAGES",				GL_DEBUG_LOGGED_MESSAGES,				queryType)));
		FOR_ALL_TYPES(queries->addChild(new PositiveIntegerCase		(m_context, (std::string("debug_next_logged_message_length") + postfix).c_str(),	"Test DEBUG_NEXT_LOGGED_MESSAGE_LENGTH",	GL_DEBUG_NEXT_LOGGED_MESSAGE_LENGTH,	queryType)));
		FOR_ALL_TYPES(queries->addChild(new GroupStackDepthQueryCase(m_context, (std::string("debug_group_stack_depth") + postfix).c_str(),				"Test DEBUG_GROUP_STACK_DEPTH",				queryType)));

		queries->addChild(new DebugCallbackFunctionCase	(m_context, "debug_callback_function_getpointer",	"Test DEBUG_CALLBACK_FUNCTION"));
		queries->addChild(new DebugCallbackUserParamCase(m_context, "debug_callback_user_param_getpointer", "Test DEBUG_CALLBACK_USER_PARAM"));

		#undef FOR_ALL_TYPES
		#undef FOR_ALL_ENABLE_TYPES
	}

	{
		tcu::TestCaseGroup* const	negative	= new tcu::TestCaseGroup(m_testCtx, "negative_coverage", "API error coverage with various reporting methods");

		addChild(negative);
		{
			tcu::TestCaseGroup* const	host	= new tcu::TestCaseGroup(m_testCtx, "callbacks", "Reporting of standard API errors via callback");

			negative->addChild(host);
			host->addChild(createChildCases(CASETYPE_CALLBACK, m_context, "buffer",						"Negative Buffer API Cases",						bufferFuncs));
			host->addChild(createChildCases(CASETYPE_CALLBACK, m_context, "texture",					"Negative Texture API Cases",						textureFuncs));
			host->addChild(createChildCases(CASETYPE_CALLBACK, m_context, "shader",						"Negative Shader API Cases",						shaderFuncs));
			host->addChild(createChildCases(CASETYPE_CALLBACK, m_context, "fragment",					"Negative Fragment API Cases",						fragmentFuncs));
			host->addChild(createChildCases(CASETYPE_CALLBACK, m_context, "vertex_array",				"Negative Vertex Array API Cases",					vaFuncs));
			host->addChild(createChildCases(CASETYPE_CALLBACK, m_context, "state",						"Negative GL State API Cases",						stateFuncs));
			host->addChild(createChildCases(CASETYPE_CALLBACK, m_context, "atomic_counter",				"Negative Atomic Counter API Cases",				atomicCounterFuncs));
			host->addChild(createChildCases(CASETYPE_CALLBACK, m_context, "shader_image_load",			"Negative Shader Image Load API Cases",				imageLoadFuncs));
			host->addChild(createChildCases(CASETYPE_CALLBACK, m_context, "shader_image_store",			"Negative Shader Image Store API Cases",			imageStoreFuncs));
			host->addChild(createChildCases(CASETYPE_CALLBACK, m_context, "shader_image_atomic",		"Negative Shader Image Atomic API Cases",			imageAtomicFuncs));
			host->addChild(createChildCases(CASETYPE_CALLBACK, m_context, "shader_image_exchange",		"Negative Shader Image Atomic Exchange API Cases",	imageAtomicExchangeFuncs));
			host->addChild(createChildCases(CASETYPE_CALLBACK, m_context, "shader_function",			"Negative Shader Function Cases",					shaderFunctionFuncs));
			host->addChild(createChildCases(CASETYPE_CALLBACK, m_context, "shader_directive",			"Negative Shader Directive Cases",					shaderDirectiveFuncs));
			host->addChild(createChildCases(CASETYPE_CALLBACK, m_context, "ssbo_block",					"Negative SSBO Block Cases",						ssboBlockFuncs));
			host->addChild(createChildCases(CASETYPE_CALLBACK, m_context, "precise",					"Negative Precise Cases",							preciseFuncs));
			host->addChild(createChildCases(CASETYPE_CALLBACK, m_context, "advanced_blend",				"Negative Advanced Blend Equation Cases",			advancedBlendFuncs));
			host->addChild(createChildCases(CASETYPE_CALLBACK, m_context, "shader_storage",				"Negative Shader Storage Cases",					shaderStorageFuncs));
			host->addChild(createChildCases(CASETYPE_CALLBACK, m_context, "tessellation",				"Negative Tessellation Cases",						tessellationFuncs));
			host->addChild(createChildCases(CASETYPE_CALLBACK, m_context, "oes_sample_variables",		"Negative Sample Variables Cases",					sampleVariablesFuncs));
			host->addChild(createChildCases(CASETYPE_CALLBACK, m_context, "compute",					"Negative Compute Cases",							computeFuncs));
			host->addChild(createChildCases(CASETYPE_CALLBACK, m_context, "framebuffer_fetch",			"Negative Framebuffer Fetch Cases",					framebufferFetchFuncs));
		}

		{
			tcu::TestCaseGroup* const	host	= new tcu::TestCaseGroup(m_testCtx, "log", "Reporting of standard API errors via log");

			negative->addChild(host);

			host->addChild(createChildCases(CASETYPE_LOG, m_context, "buffer",					"Negative Buffer API Cases",						bufferFuncs));
			host->addChild(createChildCases(CASETYPE_LOG, m_context, "texture",					"Negative Texture API Cases",						textureFuncs));
			host->addChild(createChildCases(CASETYPE_LOG, m_context, "shader",					"Negative Shader API Cases",						shaderFuncs));
			host->addChild(createChildCases(CASETYPE_LOG, m_context, "fragment",				"Negative Fragment API Cases",						fragmentFuncs));
			host->addChild(createChildCases(CASETYPE_LOG, m_context, "vertex_array",			"Negative Vertex Array API Cases",					vaFuncs));
			host->addChild(createChildCases(CASETYPE_LOG, m_context, "state",					"Negative GL State API Cases",						stateFuncs));
			host->addChild(createChildCases(CASETYPE_LOG, m_context, "atomic_counter",			"Negative Atomic Counter API Cases",				atomicCounterFuncs));
			host->addChild(createChildCases(CASETYPE_LOG, m_context, "shader_image_load",		"Negative Shader Image Load API Cases",				imageLoadFuncs));
			host->addChild(createChildCases(CASETYPE_LOG, m_context, "shader_image_store",		"Negative Shader Image Store API Cases",			imageStoreFuncs));
			host->addChild(createChildCases(CASETYPE_LOG, m_context, "shader_image_atomic",		"Negative Shader Image Atomic API Cases",			imageAtomicFuncs));
			host->addChild(createChildCases(CASETYPE_LOG, m_context, "shader_image_exchange",	"Negative Shader Image Atomic Exchange API Cases",	imageAtomicExchangeFuncs));
			host->addChild(createChildCases(CASETYPE_LOG, m_context, "shader_function",			"Negative Shader Function Cases",					shaderFunctionFuncs));
			host->addChild(createChildCases(CASETYPE_LOG, m_context, "shader_directive",		"Negative Shader Directive Cases",					shaderDirectiveFuncs));
			host->addChild(createChildCases(CASETYPE_LOG, m_context, "ssbo_block",				"Negative SSBO Block Cases",						ssboBlockFuncs));
			host->addChild(createChildCases(CASETYPE_LOG, m_context, "precise",					"Negative Precise Cases",							preciseFuncs));
			host->addChild(createChildCases(CASETYPE_LOG, m_context, "advanced_blend",			"Negative Advanced Blend Equation Cases",			advancedBlendFuncs));
			host->addChild(createChildCases(CASETYPE_LOG, m_context, "shader_storage",			"Negative Shader Storage Cases",					shaderStorageFuncs));
			host->addChild(createChildCases(CASETYPE_LOG, m_context, "tessellation",			"Negative Tessellation Cases",						tessellationFuncs));
			host->addChild(createChildCases(CASETYPE_LOG, m_context, "oes_sample_variables",	"Negative Sample Variables Cases",					sampleVariablesFuncs));
			host->addChild(createChildCases(CASETYPE_LOG, m_context, "compute",					"Negative Compute Cases",							computeFuncs));
			host->addChild(createChildCases(CASETYPE_LOG, m_context, "framebuffer_fetch",		"Negative Framebuffer Fetch Cases",					framebufferFetchFuncs));
		}

		{
			tcu::TestCaseGroup* const	host	= new tcu::TestCaseGroup(m_testCtx, "get_error", "Reporting of standard API errors via glGetError");

			negative->addChild(host);

			host->addChild(createChildCases(CASETYPE_GETERROR, m_context, "buffer",						"Negative Buffer API Cases",						bufferFuncs));
			host->addChild(createChildCases(CASETYPE_GETERROR, m_context, "texture",					"Negative Texture API Cases",						textureFuncs));
			host->addChild(createChildCases(CASETYPE_GETERROR, m_context, "shader",						"Negative Shader API Cases",						shaderFuncs));
			host->addChild(createChildCases(CASETYPE_GETERROR, m_context, "fragment",					"Negative Fragment API Cases",						fragmentFuncs));
			host->addChild(createChildCases(CASETYPE_GETERROR, m_context, "vertex_array",				"Negative Vertex Array API Cases",					vaFuncs));
			host->addChild(createChildCases(CASETYPE_GETERROR, m_context, "state",						"Negative GL State API Cases",						stateFuncs));
			host->addChild(createChildCases(CASETYPE_GETERROR, m_context, "atomic_counter",				"Negative Atomic Counter API Cases",				atomicCounterFuncs));
			host->addChild(createChildCases(CASETYPE_GETERROR, m_context, "shader_image_load",			"Negative Shader Image Load API Cases",				imageLoadFuncs));
			host->addChild(createChildCases(CASETYPE_GETERROR, m_context, "shader_image_store",			"Negative Shader Image Store API Cases",			imageStoreFuncs));
			host->addChild(createChildCases(CASETYPE_GETERROR, m_context, "shader_image_atomic",		"Negative Shader Image Atomic API Cases",			imageAtomicFuncs));
			host->addChild(createChildCases(CASETYPE_GETERROR, m_context, "shader_image_exchange",		"Negative Shader Image Atomic Exchange API Cases",	imageAtomicExchangeFuncs));
			host->addChild(createChildCases(CASETYPE_GETERROR, m_context, "shader_function",			"Negative Shader Function Cases",					shaderFunctionFuncs));
			host->addChild(createChildCases(CASETYPE_GETERROR, m_context, "shader_directive",			"Negative Shader Directive Cases",					shaderDirectiveFuncs));
			host->addChild(createChildCases(CASETYPE_GETERROR, m_context, "ssbo_block",					"Negative SSBO Block Cases",						ssboBlockFuncs));
			host->addChild(createChildCases(CASETYPE_GETERROR, m_context, "precise",					"Negative Precise Cases",							preciseFuncs));
			host->addChild(createChildCases(CASETYPE_GETERROR, m_context, "advanced_blend",				"Negative Advanced Blend Equation Cases",			advancedBlendFuncs));
			host->addChild(createChildCases(CASETYPE_GETERROR, m_context, "shader_storage",				"Negative Shader Storage Cases",					shaderStorageFuncs));
			host->addChild(createChildCases(CASETYPE_GETERROR, m_context, "tessellation",				"Negative Tessellation Cases",						tessellationFuncs));
			host->addChild(createChildCases(CASETYPE_GETERROR, m_context, "oes_sample_variables",		"Negative Sample Variables Cases",					sampleVariablesFuncs));
			host->addChild(createChildCases(CASETYPE_GETERROR, m_context, "compute",					"Negative Compute Cases",							computeFuncs));
			host->addChild(createChildCases(CASETYPE_GETERROR, m_context, "framebuffer_fetch",			"Negative Framebuffer Fetch Cases",					framebufferFetchFuncs));
		}
	}

	{
		tcu::TestCaseGroup* const host = createChildCases(CASETYPE_CALLBACK, m_context, "externally_generated", "Externally Generated Messages", externalFuncs);

		host->addChild(new GroupCase(m_context, "push_pop_consistency", "Push/pop message generation with full message output checking"));

		addChild(host);
	}

	{
		vector<FunctionContainer>	containers;
		vector<TestFunctionWrapper>	allFuncs;

		de::Random					rng			(0x53941903 ^ m_context.getTestContext().getCommandLine().getBaseSeed());

		containers.insert(containers.end(), bufferFuncs.begin(), bufferFuncs.end());
		containers.insert(containers.end(), textureFuncs.begin(), textureFuncs.end());
		containers.insert(containers.end(), externalFuncs.begin(), externalFuncs.end());

		for (size_t ndx = 0; ndx < containers.size(); ndx++)
			allFuncs.push_back(containers[ndx].function);

		rng.shuffle(allFuncs.begin(), allFuncs.end());

		{
			tcu::TestCaseGroup* const	filtering				= new tcu::TestCaseGroup(m_testCtx, "error_filters", "Filtering of reported errors");
			const int					errorFuncsPerCase		= 4;
			const int					maxFilteringCaseCount	= 32;
			const int					caseCount				= (int(allFuncs.size()) + errorFuncsPerCase-1) / errorFuncsPerCase;

			addChild(filtering);

			for (int caseNdx = 0; caseNdx < de::min(caseCount, maxFilteringCaseCount); caseNdx++)
			{
				const int					start		= caseNdx*errorFuncsPerCase;
				const int					end			= de::min((caseNdx+1)*errorFuncsPerCase, int(allFuncs.size()));
				const string				name		= "case_" + de::toString(caseNdx);
				vector<TestFunctionWrapper>	funcs		(allFuncs.begin()+start, allFuncs.begin()+end);

				// These produce lots of different message types, thus always include at least one when testing filtering
				funcs.insert(funcs.end(), externalFuncs[caseNdx%externalFuncs.size()].function);

				filtering->addChild(new FilterCase(m_context, name.c_str(), "DebugMessageControl usage", funcs));
			}
		}

		{
			tcu::TestCaseGroup* const	groups					= new tcu::TestCaseGroup(m_testCtx, "error_groups", "Filtering of reported errors with use of Error Groups");
			const int					errorFuncsPerCase		= 4;
			const int					maxFilteringCaseCount	= 16;
			const int					caseCount				= (int(allFuncs.size()) + errorFuncsPerCase-1) / errorFuncsPerCase;

			addChild(groups);

			for (int caseNdx = 0; caseNdx < caseCount && caseNdx < maxFilteringCaseCount; caseNdx++)
			{
				const int					start		= caseNdx*errorFuncsPerCase;
				const int					end			= de::min((caseNdx+1)*errorFuncsPerCase, int(allFuncs.size()));
				const string				name		= ("case_" + de::toString(caseNdx)).c_str();
				vector<TestFunctionWrapper>	funcs		(&allFuncs[0]+start, &allFuncs[0]+end);

				// These produce lots of different message types, thus always include at least one when testing filtering
				funcs.insert(funcs.end(), externalFuncs[caseNdx%externalFuncs.size()].function);

				groups->addChild(new GroupFilterCase(m_context, name.c_str(), "Debug Group usage", funcs));
			}
		}

		{
			tcu::TestCaseGroup* const	async				= new tcu::TestCaseGroup(m_testCtx, "async", "Asynchronous message generation");
			const int					errorFuncsPerCase	= 2;
			const int					maxAsyncCaseCount	= 16;
			const int					caseCount			= (int(allFuncs.size()) + errorFuncsPerCase-1) / errorFuncsPerCase;

			addChild(async);

			for (int caseNdx = 0; caseNdx < caseCount && caseNdx < maxAsyncCaseCount; caseNdx++)
			{
				const int					start		= caseNdx*errorFuncsPerCase;
				const int					end			= de::min((caseNdx+1)*errorFuncsPerCase, int(allFuncs.size()));
				const string				name		= ("case_" + de::toString(caseNdx)).c_str();
				vector<TestFunctionWrapper>	funcs		(&allFuncs[0]+start, &allFuncs[0]+end);

				if (caseNdx&0x1)
					async->addChild(new AsyncCase(m_context, (name+"_callback").c_str(), "Async message generation", funcs, true));
				else
					async->addChild(new AsyncCase(m_context, (name+"_log").c_str(), "Async message generation", funcs, false));
			}
		}
	}

	{
		tcu::TestCaseGroup* const labels = new tcu::TestCaseGroup(m_testCtx, "object_labels", "Labeling objects");

		const struct
		{
			GLenum		identifier;
			const char*	name;
			const char* desc;
		} cases[] =
		{
			{ GL_BUFFER,				"buffer",				"Debug label on a buffer object"				},
			{ GL_SHADER,				"shader",				"Debug label on a shader object"				},
			{ GL_PROGRAM,				"program",				"Debug label on a program object"				},
			{ GL_QUERY,					"query",				"Debug label on a query object"					},
			{ GL_PROGRAM_PIPELINE,		"program_pipeline",		"Debug label on a program pipeline object"		},
			{ GL_TRANSFORM_FEEDBACK,	"transform_feedback",	"Debug label on a transform feedback object"	},
			{ GL_SAMPLER,				"sampler",				"Debug label on a sampler object"				},
			{ GL_TEXTURE,				"texture",				"Debug label on a texture object"				},
			{ GL_RENDERBUFFER,			"renderbuffer",			"Debug label on a renderbuffer object"			},
			{ GL_FRAMEBUFFER,			"framebuffer",			"Debug label on a framebuffer object"			},
		};

		addChild(labels);

		labels->addChild(new InitialLabelCase		(m_context, "initial",				"Debug label initial value"));
		labels->addChild(new ClearLabelCase			(m_context, "clearing",				"Debug label clearing"));
		labels->addChild(new SpecifyWithLengthCase	(m_context, "specify_with_length",	"Debug label specified with length"));
		labels->addChild(new BufferLimitedLabelCase	(m_context, "buffer_limited_query",	"Debug label query to too short buffer"));
		labels->addChild(new LabelMaxSizeCase		(m_context, "max_label_length",		"Max sized debug label"));
		labels->addChild(new LabelLengthCase		(m_context, "query_length_only",	"Query debug label length"));

		for (int ndx = 0; ndx < DE_LENGTH_OF_ARRAY(cases); ndx++)
			labels->addChild(new LabelCase(m_context, cases[ndx].name, cases[ndx].desc, cases[ndx].identifier));
		labels->addChild(new SyncLabelCase(m_context, "sync", "Debug label on a sync object"));
	}
}

} // Functional
} // gles31
} // deqp
