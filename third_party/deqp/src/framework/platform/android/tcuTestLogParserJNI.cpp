/*-------------------------------------------------------------------------
 * drawElements Quality Program Tester Core
 * ----------------------------------------
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
 * \brief Android JNI interface for instrumentations log parsing.
 *//*--------------------------------------------------------------------*/

#include "tcuDefs.hpp"

#include "xeTestResultParser.hpp"
#include "xeTestCaseResult.hpp"
#include "xeContainerFormatParser.hpp"
#include "xeTestLogWriter.hpp"
#include "xeXMLWriter.hpp"

#include <jni.h>
#include <stdlib.h>
#include <android/log.h>

#include <sstream>

namespace
{
static const char*	TESTCASE_STYLESHEET	= "testlog.xsl";
static const char*	LOG_TAG				= "dEQP-TestLog";

class TestLogListener
{
public:
						TestLogListener		(JNIEnv* env, jobject object);
						~TestLogListener	(void);

	void				beginSession		(void);
	void				endSession			(void);
	void				sessionInfo			(const char* name, const char* value);

	void				beginTestCase		(const char* testCasePath);
	void				endTestCase			(void);

	void				terminateTestCase	(const char* reason);
	void				testCaseResult		(const char* statusCode, const char* details);

	void				testLogData			(const char* data);

private:
	JNIEnv*				m_env;
	jobject				m_object;
	jclass				m_class;

	jmethodID			m_sessionInfoID;
	jmethodID			m_beginSessionID;
	jmethodID			m_endSessionID;

	jmethodID			m_beginTestCaseID;
	jmethodID			m_endTestCaseID;
	jmethodID			m_terminateTestCaseID;
	jmethodID			m_testCaseResultID;
	jmethodID			m_testLogData;

						TestLogListener		(const TestLogListener&);
	TestLogListener&	operator=			(const TestLogListener&);
};

TestLogListener::TestLogListener (JNIEnv* env, jobject object)
	: m_env		(env)
	, m_object	(object)
{
	m_class					= m_env->GetObjectClass(m_object);
	m_sessionInfoID			= m_env->GetMethodID(m_class, "sessionInfo",		"(Ljava/lang/String;Ljava/lang/String;)V");
	m_beginSessionID		= m_env->GetMethodID(m_class, "beginSession",		"()V");
	m_endSessionID			= m_env->GetMethodID(m_class, "endSession",			"()V");
	m_beginTestCaseID		= m_env->GetMethodID(m_class, "beginTestCase",		"(Ljava/lang/String;)V");
	m_endTestCaseID			= m_env->GetMethodID(m_class, "endTestCase",		"()V");
	m_terminateTestCaseID	= m_env->GetMethodID(m_class, "terminateTestCase",	"(Ljava/lang/String;)V");
	m_testCaseResultID		= m_env->GetMethodID(m_class, "testCaseResult",		"(Ljava/lang/String;Ljava/lang/String;)V");
	m_testLogData			= m_env->GetMethodID(m_class, "testLogData",		"(Ljava/lang/String;)V");

	TCU_CHECK_INTERNAL(m_beginSessionID);
	TCU_CHECK_INTERNAL(m_endSessionID);
	TCU_CHECK_INTERNAL(m_sessionInfoID);
	TCU_CHECK_INTERNAL(m_beginTestCaseID);
	TCU_CHECK_INTERNAL(m_endTestCaseID);
	TCU_CHECK_INTERNAL(m_terminateTestCaseID);
	TCU_CHECK_INTERNAL(m_testCaseResultID);
	TCU_CHECK_INTERNAL(m_testLogData);
}

TestLogListener::~TestLogListener (void)
{
}

void TestLogListener::beginSession (void)
{
	m_env->CallVoidMethod(m_object, m_beginSessionID);
}

void TestLogListener::endSession (void)
{
	m_env->CallVoidMethod(m_object, m_endSessionID);
}

void TestLogListener::sessionInfo (const char* name, const char* value)
{
	jstring jName	= m_env->NewStringUTF(name);
	jstring jValue	= m_env->NewStringUTF(value);

	m_env->CallVoidMethod(m_object, m_sessionInfoID, jName, jValue);
	m_env->DeleteLocalRef(jName);
	m_env->DeleteLocalRef(jValue);
}

void TestLogListener::beginTestCase (const char* testCasePath)
{
	jstring jTestCasePath = m_env->NewStringUTF(testCasePath);

	m_env->CallVoidMethod(m_object, m_beginTestCaseID, jTestCasePath);
	m_env->DeleteLocalRef(jTestCasePath);
}

void TestLogListener::endTestCase (void)
{
	m_env->CallVoidMethod(m_object, m_endTestCaseID);
}

void TestLogListener::terminateTestCase (const char* reason)
{
	jstring	 jReason = m_env->NewStringUTF(reason);

	m_env->CallVoidMethod(m_object, m_terminateTestCaseID, jReason);
	m_env->DeleteLocalRef(jReason);
}

void TestLogListener::testCaseResult (const char* statusCode, const char* details)
{
	jstring	 jStatusCode	= m_env->NewStringUTF(statusCode);
	jstring	 jDetails		= m_env->NewStringUTF(details);

	m_env->CallVoidMethod(m_object, m_testCaseResultID, jStatusCode, jDetails);
	m_env->DeleteLocalRef(jStatusCode);
	m_env->DeleteLocalRef(jDetails);
}

void TestLogListener::testLogData (const char* data)
{
	jstring logData = m_env->NewStringUTF(data);

	m_env->CallVoidMethod(m_object, m_testLogData, logData);
	m_env->DeleteLocalRef(logData);
}

class TestLogParser
{
public:
								TestLogParser	(bool logData);
								~TestLogParser	(void);

	void						parse			(TestLogListener& listener, const char* buffer, size_t size);

private:
	const bool					m_logData;

	bool						m_inTestCase;
	bool						m_loggedResult;
	xe::ContainerFormatParser	m_containerParser;
	xe::TestCaseResult			m_testCaseResult;
	xe::TestResultParser		m_testResultParser;

								TestLogParser	(const TestLogParser&);
	TestLogParser&				operator=		(const TestLogParser&);
};

TestLogParser::TestLogParser (bool logData)
	: m_logData			(logData)
	, m_inTestCase		(DE_FALSE)
	, m_loggedResult	(DE_FALSE)
{
}

TestLogParser::~TestLogParser (void)
{
}

void TestLogParser::parse (TestLogListener& listener, const char* buffer, size_t size)
{
	m_containerParser.feed((const deUint8*)buffer, size);

	while (m_containerParser.getElement() != xe::CONTAINERELEMENT_INCOMPLETE)
	{
		switch (m_containerParser.getElement())
		{
			case xe::CONTAINERELEMENT_END_OF_STRING:
				// Do nothing
				break;

			case xe::CONTAINERELEMENT_BEGIN_SESSION:
				listener.beginSession();
				break;

			case xe::CONTAINERELEMENT_END_SESSION:
				listener.endSession();
				break;

			case xe::CONTAINERELEMENT_SESSION_INFO:
				listener.sessionInfo(m_containerParser.getSessionInfoAttribute(), m_containerParser.getSessionInfoValue());
				break;

			case xe::CONTAINERELEMENT_BEGIN_TEST_CASE_RESULT:
				listener.beginTestCase(m_containerParser.getTestCasePath());

				m_inTestCase		= DE_TRUE;
				m_loggedResult		= DE_FALSE;
				m_testCaseResult	= xe::TestCaseResult();

				m_testResultParser.init(&m_testCaseResult);
				break;

			case xe::CONTAINERELEMENT_END_TEST_CASE_RESULT:
				if (m_testCaseResult.statusCode != xe::TESTSTATUSCODE_LAST && !m_loggedResult)
				{
					listener.testCaseResult(xe::getTestStatusCodeName(m_testCaseResult.statusCode), m_testCaseResult.statusDetails.c_str());
					m_loggedResult = DE_TRUE;
				}

				if (m_logData)
				{
					std::ostringstream	testLog;
					xe::xml::Writer		xmlWriter(testLog);

					testLog << "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
							<< "<?xml-stylesheet href=\"" << TESTCASE_STYLESHEET << "\" type=\"text/xsl\"?>\n";

					xe::writeTestResult(m_testCaseResult, xmlWriter);

					listener.testLogData(testLog.str().c_str());
				}

				listener.endTestCase();

				m_inTestCase = DE_FALSE;
				break;

			case xe::CONTAINERELEMENT_TERMINATE_TEST_CASE_RESULT:
				if (m_logData)
				{
					std::ostringstream	testLog;
					xe::xml::Writer		xmlWriter(testLog);

					testLog << "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
							<< "<?xml-stylesheet href=\"" << TESTCASE_STYLESHEET << "\" type=\"text/xsl\"?>\n";

					xe::writeTestResult(m_testCaseResult, xmlWriter);

					listener.testLogData(testLog.str().c_str());
				}

				if (m_testCaseResult.statusCode != xe::TESTSTATUSCODE_LAST && !m_loggedResult)
				{
					listener.testCaseResult(xe::getTestStatusCodeName(m_testCaseResult.statusCode), m_testCaseResult.statusDetails.c_str());
					m_loggedResult = DE_TRUE;
				}

				listener.terminateTestCase(m_containerParser.getTerminateReason());
				m_inTestCase = DE_FALSE;
				break;

			case xe::CONTAINERELEMENT_TEST_LOG_DATA:
			{
				if (m_inTestCase)
				{
					std::vector<deUint8> data(m_containerParser.getDataSize());
					m_containerParser.getData(&(data[0]), (int)data.size(), 0);

					//tcu::print("%d %s :%s %s", __LINE__, std::string((const char*)&data[0], data.size()).c_str(), __func__, __FILE__);

					if (m_testResultParser.parse(&(data[0]), (int)data.size()) == xe::TestResultParser::PARSERESULT_CHANGED)
					{
						if (m_testCaseResult.statusCode != xe::TESTSTATUSCODE_LAST && !m_loggedResult)
						{
							listener.testCaseResult(xe::getTestStatusCodeName(m_testCaseResult.statusCode), m_testCaseResult.statusDetails.c_str());
							m_loggedResult = DE_TRUE;
						}
					}
				}

				break;
			}

			default:
				DE_ASSERT(DE_FALSE);

		};

		m_containerParser.advance();
	}
}

void throwJNIException (JNIEnv* env, const std::exception& e)
{
	jclass exClass;

	exClass = env->FindClass("java/lang/Exception");

	TCU_CHECK_INTERNAL(exClass != DE_NULL);

	TCU_CHECK_INTERNAL(env->ThrowNew(exClass, e.what()) == 0);
}

} // anonymous

DE_BEGIN_EXTERN_C

JNIEXPORT jlong JNICALL Java_com_drawelements_deqp_testercore_TestLogParser_nativeCreate (JNIEnv* env, jclass, jboolean logData)
{
	DE_UNREF(env);

	try
	{
		return (jlong)new TestLogParser(logData);
	}
	catch (const std::exception& e)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "%s", e.what());

		throwJNIException(env, e);
		return 0;
	}
}

JNIEXPORT void JNICALL Java_com_drawelements_deqp_testercore_TestLogParser_nativeDestroy (JNIEnv* env, jclass, jlong nativePointer)
{
	DE_UNREF(env);

	try
	{
		delete ((TestLogParser*)nativePointer);
	}
	catch (const std::exception& e)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "%s", e.what());

		throwJNIException(env, e);
	}
}

JNIEXPORT void JNICALL Java_com_drawelements_deqp_testercore_TestLogParser_nativeParse (JNIEnv* env, jclass, jlong nativePointer, jobject instrumentation, jbyteArray buffer, jint size)
{
	jbyte* logData = DE_NULL;

	try
	{
		TestLogParser*	parser		= (TestLogParser*)nativePointer;
		TestLogListener	listener	(env, instrumentation);

		logData = env->GetByteArrayElements(buffer, NULL);

		parser->parse(listener, (const char*)logData, (size_t)size);
		env->ReleaseByteArrayElements(buffer, logData, JNI_ABORT);
		logData = DE_NULL;
	}
	catch (const std::exception& e)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "%s", e.what());

		if (logData)
			env->ReleaseByteArrayElements(buffer, logData, JNI_ABORT);

		throwJNIException(env, e);
	}
}

DE_END_EXTERN_C
