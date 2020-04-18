/*-------------------------------------------------------------------------
 * drawElements Quality Program EGL Module
 * ---------------------------------------
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
 * \brief Multi threaded EGL tests
 *//*--------------------------------------------------------------------*/
#include "teglMultiThreadTests.hpp"

#include "egluNativeWindow.hpp"
#include "egluNativePixmap.hpp"
#include "egluUtil.hpp"

#include "tcuTestLog.hpp"
#include "tcuCommandLine.hpp"

#include "deRandom.hpp"

#include "deThread.hpp"
#include "deMutex.hpp"
#include "deSemaphore.hpp"

#include "deAtomic.h"
#include "deClock.h"

#include "eglwLibrary.hpp"
#include "eglwEnums.hpp"

#include <vector>
#include <set>
#include <string>
#include <sstream>

using std::vector;
using std::string;
using std::pair;
using std::set;
using std::ostringstream;

using namespace eglw;

namespace deqp
{
namespace egl
{

class ThreadLog
{
public:
	class BeginMessageToken	{};
	class EndMessageToken	{};

	struct Message
	{
					Message	(deUint64 timeUs_, const char* msg_) : timeUs(timeUs_), msg(msg_) {}

		deUint64	timeUs;
		string		msg;
	};

								ThreadLog	(void)						{ m_messages.reserve(100); }

	ThreadLog&					operator<<	(const BeginMessageToken&)	{ return *this; }
	ThreadLog&					operator<<	(const EndMessageToken&);

	template<class T>
	ThreadLog&					operator<<	(const T& t)				{ m_message << t; return *this; }
	const vector<Message>&		getMessages (void) const				{ return m_messages; }

	static BeginMessageToken	BeginMessage;
	static EndMessageToken		EndMessage;

private:
	ostringstream		m_message;
	vector<Message>		m_messages;
};

ThreadLog& ThreadLog::operator<< (const EndMessageToken&)
{
	m_messages.push_back(Message(deGetMicroseconds(), m_message.str().c_str()));
	m_message.str("");
	return *this;
}

ThreadLog::BeginMessageToken	ThreadLog::BeginMessage;
ThreadLog::EndMessageToken		ThreadLog::EndMessage;

class MultiThreadedTest;

class TestThread : public de::Thread
{
public:
	enum ThreadStatus
	{
		THREADSTATUS_NOT_STARTED = 0,
		THREADSTATUS_RUNNING,
		THREADSTATUS_READY,
	};

					TestThread	(MultiThreadedTest& test, int id);
	void			run			(void);

	ThreadStatus	getStatus	(void) const	{ return m_status; }
	ThreadLog&		getLog		(void)			{ return m_log; }

	int				getId		(void) const	{ return m_id; }

	void			setStatus	(ThreadStatus status)	{ m_status = status; }

	const Library&	getLibrary	(void) const;

	// Test has stopped
	class TestStop {};


private:
	MultiThreadedTest&	m_test;
	const int			m_id;
	ThreadStatus		m_status;
	ThreadLog			m_log;
};

class MultiThreadedTest : public TestCase
{
public:
							MultiThreadedTest	(EglTestContext& eglTestCtx, const char* name, const char* description, int threadCount, deUint64 timeoutUs);
	virtual					~MultiThreadedTest	(void);

	void					init				(void);
	void					deinit				(void);

	virtual bool			runThread			(TestThread& thread) = 0;
	virtual IterateResult	iterate				(void);
	void					execTest			(TestThread& thread);

	const Library&			getLibrary			(void) const { return m_eglTestCtx.getLibrary(); }

protected:
	void					barrier				(void);

private:
	int						m_threadCount;
	bool					m_initialized;
	deUint64				m_startTimeUs;
	const deUint64			m_timeoutUs;
	bool					m_ok;
	bool					m_supported;
	vector<TestThread*>		m_threads;

	volatile deInt32		m_barrierWaiters;
	de::Semaphore			m_barrierSemaphore1;
	de::Semaphore			m_barrierSemaphore2;

protected:
	EGLDisplay				m_display;
};

inline const Library& TestThread::getLibrary (void) const
{
	return m_test.getLibrary();
}

TestThread::TestThread (MultiThreadedTest& test, int id)
	: m_test	(test)
	, m_id		(id)
	, m_status	(THREADSTATUS_NOT_STARTED)
{
}

void TestThread::run (void)
{
	m_status = THREADSTATUS_RUNNING;

	try
	{
		m_test.execTest(*this);
	}
	catch (const TestThread::TestStop&)
	{
		getLog() << ThreadLog::BeginMessage << "Thread stopped" << ThreadLog::EndMessage;
	}
	catch (const tcu::NotSupportedError& e)
	{
		getLog() << ThreadLog::BeginMessage << "Not supported: '" << e.what() << "'" << ThreadLog::EndMessage;
	}
	catch (const std::exception& e)
	{
		getLog() << ThreadLog::BeginMessage << "Got exception: '" << e.what() << "'" << ThreadLog::EndMessage;
	}
	catch (...)
	{
		getLog() << ThreadLog::BeginMessage << "Unknown exception" << ThreadLog::EndMessage;
	}

	getLibrary().releaseThread();
	m_status = THREADSTATUS_READY;
}

void MultiThreadedTest::execTest (TestThread& thread)
{
	try
	{
		if (!runThread(thread))
			m_ok = false;
	}
	catch (const TestThread::TestStop&)
	{
		// Thread exited due to error in other thread
		throw;
	}
	catch (const tcu::NotSupportedError&)
	{
		m_supported = false;

		// Release barriers
		for (int threadNdx = 0; threadNdx < (int)m_threads.size(); threadNdx++)
		{
			m_barrierSemaphore1.increment();
			m_barrierSemaphore2.increment();
		}

		throw;
	}
	catch(...)
	{
		m_ok = false;

		// Release barriers
		for (int threadNdx = 0; threadNdx < (int)m_threads.size(); threadNdx++)
		{
			m_barrierSemaphore1.increment();
			m_barrierSemaphore2.increment();
		}

		throw;
	}
}

MultiThreadedTest::MultiThreadedTest (EglTestContext& eglTestCtx, const char* name, const char* description, int threadCount, deUint64 timeoutUs)
	: TestCase				(eglTestCtx, name, description)
	, m_threadCount			(threadCount)
	, m_initialized			(false)
	, m_startTimeUs			(0)
	, m_timeoutUs			(timeoutUs)
	, m_ok					(true)
	, m_supported			(true)
	, m_barrierWaiters		(0)
	, m_barrierSemaphore1	(0, 0)
	, m_barrierSemaphore2	(1, 0)

	, m_display				(EGL_NO_DISPLAY)
{
}

MultiThreadedTest::~MultiThreadedTest (void)
{
	for (int threadNdx = 0; threadNdx < (int)m_threads.size(); threadNdx++)
		delete m_threads[threadNdx];
	m_threads.clear();
}

void MultiThreadedTest::init (void)
{
	m_display = eglu::getAndInitDisplay(m_eglTestCtx.getNativeDisplay());
}

void MultiThreadedTest::deinit (void)
{
	if (m_display != EGL_NO_DISPLAY)
	{
		m_eglTestCtx.getLibrary().terminate(m_display);
		m_display = EGL_NO_DISPLAY;
	}
}

void MultiThreadedTest::barrier (void)
{
	{
		const deInt32 waiters = deAtomicIncrement32(&m_barrierWaiters);

		if (waiters == m_threadCount)
		{
			m_barrierSemaphore2.decrement();
			m_barrierSemaphore1.increment();
		}
		else
		{
			m_barrierSemaphore1.decrement();
			m_barrierSemaphore1.increment();
		}
	}

	{
		const deInt32 waiters = deAtomicDecrement32(&m_barrierWaiters);

		if (waiters == 0)
		{
			m_barrierSemaphore1.decrement();
			m_barrierSemaphore2.increment();
		}
		else
		{
			m_barrierSemaphore2.decrement();
			m_barrierSemaphore2.increment();
		}
	}

	// Barrier was released due an error in other thread
	if (!m_ok || !m_supported)
		throw TestThread::TestStop();
}

TestCase::IterateResult MultiThreadedTest::iterate (void)
{
	if (!m_initialized)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Thread timeout limit: " << m_timeoutUs << "us" << tcu::TestLog::EndMessage;

		m_ok = true;
		m_supported = true;

		// Create threads
		m_threads.reserve(m_threadCount);

		for (int threadNdx = 0; threadNdx < m_threadCount; threadNdx++)
			m_threads.push_back(new TestThread(*this, threadNdx));

		m_startTimeUs = deGetMicroseconds();

		// Run threads
		for (int threadNdx = 0; threadNdx < (int)m_threads.size(); threadNdx++)
			m_threads[threadNdx]->start();

		m_initialized = true;
	}

	int readyCount = 0;
	for (int threadNdx = 0; threadNdx < (int)m_threads.size(); threadNdx++)
	{
		if (m_threads[threadNdx]->getStatus() != TestThread::THREADSTATUS_RUNNING)
			readyCount++;
	}

	if (readyCount == m_threadCount)
	{
		// Join threads
		for (int threadNdx = 0; threadNdx < (int)m_threads.size(); threadNdx++)
			m_threads[threadNdx]->join();

		// Get logs
		{
			vector<int> messageNdx;

			messageNdx.resize(m_threads.size(), 0);

			while (true)
			{
				int			nextThreadNdx		= -1;
				deUint64	nextThreadTimeUs	= 0;

				for (int threadNdx = 0; threadNdx < (int)m_threads.size(); threadNdx++)
				{
					if (messageNdx[threadNdx] >= (int)m_threads[threadNdx]->getLog().getMessages().size())
						continue;

					if (nextThreadNdx == -1 || nextThreadTimeUs > m_threads[threadNdx]->getLog().getMessages()[messageNdx[threadNdx]].timeUs)
					{
						nextThreadNdx		= threadNdx;
						nextThreadTimeUs	= m_threads[threadNdx]->getLog().getMessages()[messageNdx[threadNdx]].timeUs;
					}
				}

				if (nextThreadNdx == -1)
					break;

				m_testCtx.getLog() << tcu::TestLog::Message << "[" << (nextThreadTimeUs - m_startTimeUs) << "] (" << nextThreadNdx << ") " << m_threads[nextThreadNdx]->getLog().getMessages()[messageNdx[nextThreadNdx]].msg << tcu::TestLog::EndMessage;

				messageNdx[nextThreadNdx]++;
			}
		}

		// Destroy threads
		for (int threadNdx = 0; threadNdx < (int)m_threads.size(); threadNdx++)
			delete m_threads[threadNdx];

		m_threads.clear();

		// Set result
		if (m_ok)
		{
			if (!m_supported)
				m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "Not Supported");
			else
				m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
		}
		else
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");

		return STOP;
	}
	else
	{
		// Check for timeout
		const deUint64 currentTimeUs = deGetMicroseconds();

		if (currentTimeUs - m_startTimeUs > m_timeoutUs)
		{
			// Get logs
			{
				vector<int> messageNdx;

				messageNdx.resize(m_threads.size(), 0);

				while (true)
				{
					int			nextThreadNdx		= -1;
					deUint64	nextThreadTimeUs	= 0;

					for (int threadNdx = 0; threadNdx < (int)m_threads.size(); threadNdx++)
					{
						if (messageNdx[threadNdx] >= (int)m_threads[threadNdx]->getLog().getMessages().size())
							continue;

						if (nextThreadNdx == -1 || nextThreadTimeUs > m_threads[threadNdx]->getLog().getMessages()[messageNdx[threadNdx]].timeUs)
						{
							nextThreadNdx		= threadNdx;
							nextThreadTimeUs	= m_threads[threadNdx]->getLog().getMessages()[messageNdx[threadNdx]].timeUs;
						}
					}

					if (nextThreadNdx == -1)
						break;

					m_testCtx.getLog() << tcu::TestLog::Message << "[" << (nextThreadTimeUs - m_startTimeUs) << "] (" << nextThreadNdx << ") " << m_threads[nextThreadNdx]->getLog().getMessages()[messageNdx[nextThreadNdx]].msg << tcu::TestLog::EndMessage;

					messageNdx[nextThreadNdx]++;
				}
			}

			m_testCtx.getLog() << tcu::TestLog::Message << "[" << (currentTimeUs - m_startTimeUs) << "] (-) Timeout, Limit: " << m_timeoutUs << "us" << tcu::TestLog::EndMessage;
			m_testCtx.getLog() << tcu::TestLog::Message << "[" << (currentTimeUs - m_startTimeUs) << "] (-) Trying to perform resource cleanup..." << tcu::TestLog::EndMessage;

			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
			return STOP;
		}

		// Sleep
		deSleep(10);
	}

	return CONTINUE;
}

namespace
{

const char* configAttributeToString (EGLint e)
{
	switch (e)
	{
		case EGL_BUFFER_SIZE:				return "EGL_BUFFER_SIZE";
		case EGL_RED_SIZE:					return "EGL_RED_SIZE";
		case EGL_GREEN_SIZE:				return "EGL_GREEN_SIZE";
		case EGL_BLUE_SIZE:					return "EGL_BLUE_SIZE";
		case EGL_LUMINANCE_SIZE:			return "EGL_LUMINANCE_SIZE";
		case EGL_ALPHA_SIZE:				return "EGL_ALPHA_SIZE";
		case EGL_ALPHA_MASK_SIZE:			return "EGL_ALPHA_MASK_SIZE";
		case EGL_BIND_TO_TEXTURE_RGB:		return "EGL_BIND_TO_TEXTURE_RGB";
		case EGL_BIND_TO_TEXTURE_RGBA:		return "EGL_BIND_TO_TEXTURE_RGBA";
		case EGL_COLOR_BUFFER_TYPE:			return "EGL_COLOR_BUFFER_TYPE";
		case EGL_CONFIG_CAVEAT:				return "EGL_CONFIG_CAVEAT";
		case EGL_CONFIG_ID:					return "EGL_CONFIG_ID";
		case EGL_CONFORMANT:				return "EGL_CONFORMANT";
		case EGL_DEPTH_SIZE:				return "EGL_DEPTH_SIZE";
		case EGL_LEVEL:						return "EGL_LEVEL";
		case EGL_MAX_PBUFFER_WIDTH:			return "EGL_MAX_PBUFFER_WIDTH";
		case EGL_MAX_PBUFFER_HEIGHT:		return "EGL_MAX_PBUFFER_HEIGHT";
		case EGL_MAX_PBUFFER_PIXELS:		return "EGL_MAX_PBUFFER_PIXELS";
		case EGL_MAX_SWAP_INTERVAL:			return "EGL_MAX_SWAP_INTERVAL";
		case EGL_MIN_SWAP_INTERVAL:			return "EGL_MIN_SWAP_INTERVAL";
		case EGL_NATIVE_RENDERABLE:			return "EGL_NATIVE_RENDERABLE";
		case EGL_NATIVE_VISUAL_ID:			return "EGL_NATIVE_VISUAL_ID";
		case EGL_NATIVE_VISUAL_TYPE:		return "EGL_NATIVE_VISUAL_TYPE";
		case EGL_RENDERABLE_TYPE:			return "EGL_RENDERABLE_TYPE";
		case EGL_SAMPLE_BUFFERS:			return "EGL_SAMPLE_BUFFERS";
		case EGL_SAMPLES:					return "EGL_SAMPLES";
		case EGL_STENCIL_SIZE:				return "EGL_STENCIL_SIZE";
		case EGL_SURFACE_TYPE:				return "EGL_SURFACE_TYPE";
		case EGL_TRANSPARENT_TYPE:			return "EGL_TRANSPARENT_TYPE";
		case EGL_TRANSPARENT_RED_VALUE:		return "EGL_TRANSPARENT_RED_VALUE";
		case EGL_TRANSPARENT_GREEN_VALUE:	return "EGL_TRANSPARENT_GREEN_VALUE";
		case EGL_TRANSPARENT_BLUE_VALUE:	return "EGL_TRANSPARENT_BLUE_VALUE";
		default:							return "<Unknown>";
	}
}

} // anonymous

class MultiThreadedConfigTest : public MultiThreadedTest
{
public:
				MultiThreadedConfigTest		(EglTestContext& context, const char* name, const char* description, int getConfigs, int chooseConfigs, int query);
	bool		runThread					(TestThread& thread);

private:
	const int	m_getConfigs;
	const int	m_chooseConfigs;
	const int	m_query;
};

MultiThreadedConfigTest::MultiThreadedConfigTest (EglTestContext& context, const char* name, const char* description, int getConfigs, int chooseConfigs, int query)
	: MultiThreadedTest (context, name, description, 2, 20000000/*us = 20s*/) // \todo [mika] Set timeout to something relevant to frameworks timeout?
	, m_getConfigs		(getConfigs)
	, m_chooseConfigs	(chooseConfigs)
	, m_query			(query)
{
}

bool MultiThreadedConfigTest::runThread (TestThread& thread)
{
	const Library&		egl		= getLibrary();
	de::Random			rnd		(deInt32Hash(thread.getId() + 10435));
	vector<EGLConfig>	configs;

	barrier();

	for (int getConfigsNdx = 0; getConfigsNdx < m_getConfigs; getConfigsNdx++)
	{
		EGLint configCount;

		// Get number of configs
		{
			EGLBoolean result;

			result = egl.getConfigs(m_display, NULL, 0, &configCount);
			thread.getLog() << ThreadLog::BeginMessage << result << " = eglGetConfigs(" << m_display << ", NULL, 0, " << configCount << ")" <<  ThreadLog::EndMessage;
			EGLU_CHECK_MSG(egl, "eglGetConfigs()");

			if (!result)
				return false;
		}

		configs.resize(configs.size() + configCount);

		// Get configs
		if (configCount != 0)
		{
			EGLBoolean result;

			result = egl.getConfigs(m_display, &(configs[configs.size() - configCount]), configCount, &configCount);
			thread.getLog() << ThreadLog::BeginMessage << result << " = eglGetConfigs(" << m_display << ", &configs' " << configCount << ", " << configCount << ")" <<  ThreadLog::EndMessage;
			EGLU_CHECK_MSG(egl, "eglGetConfigs()");

			if (!result)
				return false;
		}

		// Pop configs to stop config list growing
		if (configs.size() > 40)
		{
			configs.erase(configs.begin() + 40, configs.end());
		}
		else
		{
			const int popCount = rnd.getInt(0, (int)(configs.size()-2));

			configs.erase(configs.begin() + (configs.size() - popCount), configs.end());
		}
	}

	for (int chooseConfigsNdx = 0; chooseConfigsNdx < m_chooseConfigs; chooseConfigsNdx++)
	{
		EGLint configCount;

		static const EGLint attribList[] = {
			EGL_NONE
		};

		// Get number of configs
		{
			EGLBoolean result;

			result = egl.chooseConfig(m_display, attribList, NULL, 0, &configCount);
			thread.getLog() << ThreadLog::BeginMessage << result << " = eglChooseConfig(" << m_display << ", { EGL_NONE }, NULL, 0, " << configCount << ")" <<  ThreadLog::EndMessage;
			EGLU_CHECK_MSG(egl, "eglChooseConfig()");

			if (!result)
				return false;
		}

		configs.resize(configs.size() + configCount);

		// Get configs
		if (configCount != 0)
		{
			EGLBoolean result;

			result = egl.chooseConfig(m_display, attribList, &(configs[configs.size() - configCount]), configCount, &configCount);
			thread.getLog() << ThreadLog::BeginMessage << result << " = eglChooseConfig(" << m_display << ", { EGL_NONE }, &configs, " << configCount << ", " << configCount << ")" <<  ThreadLog::EndMessage;
			EGLU_CHECK_MSG(egl, "eglChooseConfig()");

			if (!result)
				return false;
		}

		// Pop configs to stop config list growing
		if (configs.size() > 40)
		{
			configs.erase(configs.begin() + 40, configs.end());
		}
		else
		{
			const int popCount = rnd.getInt(0, (int)(configs.size()-2));

			configs.erase(configs.begin() + (configs.size() - popCount), configs.end());
		}
	}

	{
		// Perform queries on configs
		static const EGLint attributes[] =
		{
			EGL_BUFFER_SIZE,
			EGL_RED_SIZE,
			EGL_GREEN_SIZE,
			EGL_BLUE_SIZE,
			EGL_LUMINANCE_SIZE,
			EGL_ALPHA_SIZE,
			EGL_ALPHA_MASK_SIZE,
			EGL_BIND_TO_TEXTURE_RGB,
			EGL_BIND_TO_TEXTURE_RGBA,
			EGL_COLOR_BUFFER_TYPE,
			EGL_CONFIG_CAVEAT,
			EGL_CONFIG_ID,
			EGL_CONFORMANT,
			EGL_DEPTH_SIZE,
			EGL_LEVEL,
			EGL_MAX_PBUFFER_WIDTH,
			EGL_MAX_PBUFFER_HEIGHT,
			EGL_MAX_PBUFFER_PIXELS,
			EGL_MAX_SWAP_INTERVAL,
			EGL_MIN_SWAP_INTERVAL,
			EGL_NATIVE_RENDERABLE,
			EGL_NATIVE_VISUAL_ID,
			EGL_NATIVE_VISUAL_TYPE,
			EGL_RENDERABLE_TYPE,
			EGL_SAMPLE_BUFFERS,
			EGL_SAMPLES,
			EGL_STENCIL_SIZE,
			EGL_SURFACE_TYPE,
			EGL_TRANSPARENT_TYPE,
			EGL_TRANSPARENT_RED_VALUE,
			EGL_TRANSPARENT_GREEN_VALUE,
			EGL_TRANSPARENT_BLUE_VALUE
		};

		for (int queryNdx = 0; queryNdx < m_query; queryNdx++)
		{
			const EGLint	attribute	= attributes[rnd.getInt(0, DE_LENGTH_OF_ARRAY(attributes)-1)];
			EGLConfig		config		= configs[rnd.getInt(0, (int)(configs.size()-1))];
			EGLint			value;
			EGLBoolean		result;

			result = egl.getConfigAttrib(m_display, config, attribute, &value);
			thread.getLog() << ThreadLog::BeginMessage << result << " = eglGetConfigAttrib(" << m_display << ", " << config << ", " << configAttributeToString(attribute) << ", " << value << ")" <<  ThreadLog::EndMessage;
			EGLU_CHECK_MSG(egl, "eglGetConfigAttrib()");

			if (!result)
				return false;
		}
	}

	return true;
}

class MultiThreadedObjectTest : public MultiThreadedTest
{
public:
	enum Type
	{
		TYPE_PBUFFER			= (1<<0),
		TYPE_PIXMAP				= (1<<1),
		TYPE_WINDOW				= (1<<2),
		TYPE_SINGLE_WINDOW		= (1<<3),
		TYPE_CONTEXT			= (1<<4)
	};

					MultiThreadedObjectTest			(EglTestContext& context, const char* name, const char* description, deUint32 types);
					~MultiThreadedObjectTest		(void);

	virtual void	deinit							(void);

	bool			runThread						(TestThread& thread);

	void			createDestroyObjects			(TestThread& thread, int count);
	void			pushObjectsToShared				(TestThread& thread);
	void			pullObjectsFromShared			(TestThread& thread, int pbufferCount, int pixmapCount, int windowCount, int contextCount);
	void			querySetSharedObjects			(TestThread& thread, int count);
	void			destroyObjects					(TestThread& thread);

private:
	EGLConfig			m_config;
	de::Random			m_rnd0;
	de::Random			m_rnd1;
	Type				m_types;

	volatile deUint32	m_hasWindow;

	vector<pair<eglu::NativePixmap*, EGLSurface> >	m_sharedNativePixmaps;
	vector<pair<eglu::NativePixmap*, EGLSurface> >	m_nativePixmaps0;
	vector<pair<eglu::NativePixmap*, EGLSurface> >	m_nativePixmaps1;

	vector<pair<eglu::NativeWindow*, EGLSurface> >	m_sharedNativeWindows;
	vector<pair<eglu::NativeWindow*, EGLSurface> >	m_nativeWindows0;
	vector<pair<eglu::NativeWindow*, EGLSurface> >	m_nativeWindows1;

	vector<EGLSurface>								m_sharedPbuffers;
	vector<EGLSurface>								m_pbuffers0;
	vector<EGLSurface>								m_pbuffers1;

	vector<EGLContext>								m_sharedContexts;
	vector<EGLContext>								m_contexts0;
	vector<EGLContext>								m_contexts1;
};

MultiThreadedObjectTest::MultiThreadedObjectTest (EglTestContext& context, const char* name, const char* description, deUint32 type)
	: MultiThreadedTest (context, name, description, 2, 20000000/*us = 20s*/) // \todo [mika] Set timeout to something relevant to frameworks timeout?
	, m_config			(DE_NULL)
	, m_rnd0			(58204327)
	, m_rnd1			(230983)
	, m_types			((Type)type)
	, m_hasWindow		(0)
{
}

MultiThreadedObjectTest::~MultiThreadedObjectTest (void)
{
	deinit();
}

void MultiThreadedObjectTest::deinit (void)
{
	const Library&		egl		= getLibrary();

	// Clear pbuffers
	for (int pbufferNdx = 0; pbufferNdx < (int)m_pbuffers0.size(); pbufferNdx++)
	{
		if (m_pbuffers0[pbufferNdx] != EGL_NO_SURFACE)
		{
			egl.destroySurface(m_display, m_pbuffers0[pbufferNdx]);
			EGLU_CHECK_MSG(egl, "eglDestroySurface()");
			m_pbuffers0[pbufferNdx] = EGL_NO_SURFACE;
		}
	}
	m_pbuffers0.clear();

	for (int pbufferNdx = 0; pbufferNdx < (int)m_pbuffers1.size(); pbufferNdx++)
	{
		if (m_pbuffers1[pbufferNdx] != EGL_NO_SURFACE)
		{
			egl.destroySurface(m_display, m_pbuffers1[pbufferNdx]);
			EGLU_CHECK_MSG(egl, "eglDestroySurface()");
			m_pbuffers1[pbufferNdx] = EGL_NO_SURFACE;
		}
	}
	m_pbuffers1.clear();

	for (int pbufferNdx = 0; pbufferNdx < (int)m_sharedPbuffers.size(); pbufferNdx++)
	{
		if (m_sharedPbuffers[pbufferNdx] != EGL_NO_SURFACE)
		{
			egl.destroySurface(m_display, m_sharedPbuffers[pbufferNdx]);
			EGLU_CHECK_MSG(egl, "eglDestroySurface()");
			m_sharedPbuffers[pbufferNdx] = EGL_NO_SURFACE;
		}
	}
	m_sharedPbuffers.clear();

	for (int contextNdx = 0; contextNdx < (int)m_sharedContexts.size(); contextNdx++)
	{
		if (m_sharedContexts[contextNdx] != EGL_NO_CONTEXT)
		{
			egl.destroyContext(m_display, m_sharedContexts[contextNdx]);
			EGLU_CHECK_MSG(egl, "eglDestroyContext()");
			m_sharedContexts[contextNdx] =  EGL_NO_CONTEXT;
		}
	}
	m_sharedContexts.clear();

	for (int contextNdx = 0; contextNdx < (int)m_contexts0.size(); contextNdx++)
	{
		if (m_contexts0[contextNdx] != EGL_NO_CONTEXT)
		{
			egl.destroyContext(m_display, m_contexts0[contextNdx]);
			EGLU_CHECK_MSG(egl, "eglDestroyContext()");
			m_contexts0[contextNdx] =  EGL_NO_CONTEXT;
		}
	}
	m_contexts0.clear();

	for (int contextNdx = 0; contextNdx < (int)m_contexts1.size(); contextNdx++)
	{
		if (m_contexts1[contextNdx] != EGL_NO_CONTEXT)
		{
			egl.destroyContext(m_display, m_contexts1[contextNdx]);
			EGLU_CHECK_MSG(egl, "eglDestroyContext()");
			m_contexts1[contextNdx] =  EGL_NO_CONTEXT;
		}
	}
	m_contexts1.clear();

	// Clear pixmaps
	for (int pixmapNdx = 0; pixmapNdx < (int)m_nativePixmaps0.size(); pixmapNdx++)
	{
		if (m_nativePixmaps0[pixmapNdx].second != EGL_NO_SURFACE)
			EGLU_CHECK_CALL(egl, destroySurface(m_display, m_nativePixmaps0[pixmapNdx].second));

		m_nativePixmaps0[pixmapNdx].second = EGL_NO_SURFACE;
		delete m_nativePixmaps0[pixmapNdx].first;
		m_nativePixmaps0[pixmapNdx].first = NULL;
	}
	m_nativePixmaps0.clear();

	for (int pixmapNdx = 0; pixmapNdx < (int)m_nativePixmaps1.size(); pixmapNdx++)
	{
		if (m_nativePixmaps1[pixmapNdx].second != EGL_NO_SURFACE)
			EGLU_CHECK_CALL(egl, destroySurface(m_display, m_nativePixmaps1[pixmapNdx].second));

		m_nativePixmaps1[pixmapNdx].second = EGL_NO_SURFACE;
		delete m_nativePixmaps1[pixmapNdx].first;
		m_nativePixmaps1[pixmapNdx].first = NULL;
	}
	m_nativePixmaps1.clear();

	for (int pixmapNdx = 0; pixmapNdx < (int)m_sharedNativePixmaps.size(); pixmapNdx++)
	{
		if (m_sharedNativePixmaps[pixmapNdx].second != EGL_NO_SURFACE)
			EGLU_CHECK_CALL(egl, destroySurface(m_display, m_sharedNativePixmaps[pixmapNdx].second));

		m_sharedNativePixmaps[pixmapNdx].second = EGL_NO_SURFACE;
		delete m_sharedNativePixmaps[pixmapNdx].first;
		m_sharedNativePixmaps[pixmapNdx].first = NULL;
	}
	m_sharedNativePixmaps.clear();

	// Clear windows
	for (int windowNdx = 0; windowNdx < (int)m_nativeWindows1.size(); windowNdx++)
	{
		if (m_nativeWindows1[windowNdx].second != EGL_NO_SURFACE)
			EGLU_CHECK_CALL(egl, destroySurface(m_display, m_nativeWindows1[windowNdx].second));

		m_nativeWindows1[windowNdx].second = EGL_NO_SURFACE;
		delete m_nativeWindows1[windowNdx].first;
		m_nativeWindows1[windowNdx].first = NULL;
	}
	m_nativeWindows1.clear();

	for (int windowNdx = 0; windowNdx < (int)m_nativeWindows0.size(); windowNdx++)
	{
		if (m_nativeWindows0[windowNdx].second != EGL_NO_SURFACE)
			EGLU_CHECK_CALL(egl, destroySurface(m_display, m_nativeWindows0[windowNdx].second));

		m_nativeWindows0[windowNdx].second = EGL_NO_SURFACE;
		delete m_nativeWindows0[windowNdx].first;
		m_nativeWindows0[windowNdx].first = NULL;
	}
	m_nativeWindows0.clear();

	for (int windowNdx = 0; windowNdx < (int)m_sharedNativeWindows.size(); windowNdx++)
	{
		if (m_sharedNativeWindows[windowNdx].second != EGL_NO_SURFACE)
			EGLU_CHECK_CALL(egl, destroySurface(m_display, m_sharedNativeWindows[windowNdx].second));

		m_sharedNativeWindows[windowNdx].second = EGL_NO_SURFACE;
		delete m_sharedNativeWindows[windowNdx].first;
		m_sharedNativeWindows[windowNdx].first = NULL;
	}
	m_sharedNativeWindows.clear();

	MultiThreadedTest::deinit();
}

bool MultiThreadedObjectTest::runThread (TestThread& thread)
{
	const Library&		egl		= getLibrary();

	if (thread.getId() == 0)
	{
		EGLint surfaceTypes = 0;

		if ((m_types & TYPE_WINDOW) != 0)
			surfaceTypes |= EGL_WINDOW_BIT;

		if ((m_types & TYPE_PBUFFER) != 0)
			surfaceTypes |= EGL_PBUFFER_BIT;

		if ((m_types & TYPE_PIXMAP) != 0)
			surfaceTypes |= EGL_PIXMAP_BIT;

		EGLint configCount;
		EGLint attribList[] =
		{
			EGL_SURFACE_TYPE, surfaceTypes,
			EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
			EGL_NONE
		};

		EGLU_CHECK_CALL(egl, chooseConfig(m_display, attribList, &m_config, 1, &configCount));

		if (configCount == 0)
			TCU_THROW(NotSupportedError, "No usable config found");
	}

	barrier();

	// Create / Destroy Objects
	if ((m_types & TYPE_SINGLE_WINDOW) != 0 && (m_types & TYPE_PBUFFER) == 0 && (m_types & TYPE_PIXMAP) == 0 && (m_types & TYPE_CONTEXT) == 0)
	{
		if (thread.getId() == 0)
			createDestroyObjects(thread, 1);
	}
	else
		createDestroyObjects(thread, 100);

	// Push first threads objects to shared
	if (thread.getId() == 0)
		pushObjectsToShared(thread);

	barrier();

	// Push second threads objects to shared
	if (thread.getId() == 1)
		pushObjectsToShared(thread);

	barrier();

	// Make queries from shared surfaces
	querySetSharedObjects(thread, 100);

	barrier();

	// Pull surfaces for first thread from shared surfaces
	if (thread.getId() == 0)
		pullObjectsFromShared(thread, (int)(m_sharedPbuffers.size()/2), (int)(m_sharedNativePixmaps.size()/2), (int)(m_sharedNativeWindows.size()/2), (int)(m_sharedContexts.size()/2));

	barrier();

	// Pull surfaces for second thread from shared surfaces
	if (thread.getId() == 1)
		pullObjectsFromShared(thread, (int)m_sharedPbuffers.size(), (int)m_sharedNativePixmaps.size(), (int)m_sharedNativeWindows.size(), (int)m_sharedContexts.size());

	barrier();

	// Create / Destroy Objects
	if ((m_types & TYPE_SINGLE_WINDOW) == 0)
		createDestroyObjects(thread, 100);

	// Destroy surfaces
	destroyObjects(thread);

	return true;
}

void MultiThreadedObjectTest::createDestroyObjects (TestThread& thread, int count)
{
	const Library&									egl			= getLibrary();
	de::Random&										rnd			= (thread.getId() == 0 ? m_rnd0 : m_rnd1);
	vector<EGLSurface>&								pbuffers	= (thread.getId() == 0 ? m_pbuffers0 : m_pbuffers1);
	vector<pair<eglu::NativeWindow*, EGLSurface> >&	windows		= (thread.getId() == 0 ? m_nativeWindows0 : m_nativeWindows1);
	vector<pair<eglu::NativePixmap*, EGLSurface> >&	pixmaps		= (thread.getId() == 0 ? m_nativePixmaps0 : m_nativePixmaps1);
	vector<EGLContext>&								contexts	= (thread.getId() == 0 ? m_contexts0 : m_contexts1);
	set<Type>										objectTypes;

	if ((m_types & TYPE_PBUFFER) != 0)
		objectTypes.insert(TYPE_PBUFFER);

	if ((m_types & TYPE_PIXMAP) != 0)
		objectTypes.insert(TYPE_PIXMAP);

	if ((m_types & TYPE_WINDOW) != 0)
		objectTypes.insert(TYPE_WINDOW);

	if ((m_types & TYPE_CONTEXT) != 0)
		objectTypes.insert(TYPE_CONTEXT);

	for (int createDestroyNdx = 0; createDestroyNdx < count; createDestroyNdx++)
	{
		bool create;
		Type type;

		if (pbuffers.size() > 5 && ((m_types & TYPE_PBUFFER) != 0))
		{
			create	= false;
			type	= TYPE_PBUFFER;
		}
		else if (windows.size() > 5 && ((m_types & TYPE_WINDOW) != 0))
		{
			create	= false;
			type	= TYPE_WINDOW;
		}
		else if (pixmaps.size() > 5 && ((m_types & TYPE_PIXMAP) != 0))
		{
			create	= false;
			type	= TYPE_PIXMAP;
		}
		else if (contexts.size() > 5 && ((m_types & TYPE_CONTEXT) != 0))
		{
			create	= false;
			type	= TYPE_CONTEXT;
		}
		else if (pbuffers.size() < 3 && ((m_types & TYPE_PBUFFER) != 0))
		{
			create	= true;
			type	= TYPE_PBUFFER;
		}
		else if (pixmaps.size() < 3 && ((m_types & TYPE_PIXMAP) != 0))
		{
			create	= true;
			type	= TYPE_PIXMAP;
		}
		else if (contexts.size() < 3 && ((m_types & TYPE_CONTEXT) != 0))
		{
			create	= true;
			type	= TYPE_CONTEXT;
		}
		else if (windows.size() < 3 && ((m_types & TYPE_WINDOW) != 0) && ((m_types & TYPE_SINGLE_WINDOW) == 0))
		{
			create	= true;
			type	= TYPE_WINDOW;
		}
		else if (windows.empty() && (m_hasWindow == 0) && ((m_types & TYPE_WINDOW) != 0) && ((m_types & TYPE_SINGLE_WINDOW) != 0))
		{
			create	= true;
			type	= TYPE_WINDOW;
		}
		else
		{
			create = rnd.getBool();

			if (!create && windows.empty())
				objectTypes.erase(TYPE_WINDOW);

			type = rnd.choose<Type>(objectTypes.begin(), objectTypes.end());
		}

		if (create)
		{
			switch (type)
			{
				case TYPE_PBUFFER:
				{
					EGLSurface surface;

					const EGLint attributes[] =
					{
						EGL_WIDTH,	64,
						EGL_HEIGHT,	64,

						EGL_NONE
					};

					surface = egl.createPbufferSurface(m_display, m_config, attributes);
					thread.getLog() << ThreadLog::BeginMessage << surface << " = eglCreatePbufferSurface(" << m_display << ", " << m_config << ", { EGL_WIDTH, 64, EGL_HEIGHT, 64, EGL_NONE })" << ThreadLog::EndMessage;
					EGLU_CHECK_MSG(egl, "eglCreatePbufferSurface()");

					pbuffers.push_back(surface);

					break;
				}

				case TYPE_WINDOW:
				{
					const eglu::NativeWindowFactory&	windowFactory	= eglu::selectNativeWindowFactory(m_eglTestCtx.getNativeDisplayFactory(), m_testCtx.getCommandLine());

					if ((m_types & TYPE_SINGLE_WINDOW) != 0)
					{
						if (deAtomicCompareExchange32(&m_hasWindow, 0, 1) == 0)
						{
							eglu::NativeWindow* window	= DE_NULL;
							EGLSurface			surface = EGL_NO_SURFACE;

							try
							{
								window = windowFactory.createWindow(&m_eglTestCtx.getNativeDisplay(), m_display, m_config, DE_NULL, eglu::WindowParams(64, 64, eglu::parseWindowVisibility(m_testCtx.getCommandLine())));
								surface = eglu::createWindowSurface(m_eglTestCtx.getNativeDisplay(), *window, m_display, m_config, DE_NULL);

								thread.getLog() << ThreadLog::BeginMessage << surface << " = eglCreateWindowSurface()" << ThreadLog::EndMessage;
								windows.push_back(std::make_pair(window, surface));
							}
							catch (const std::exception&)
							{
								if (surface != EGL_NO_SURFACE)
									EGLU_CHECK_CALL(egl, destroySurface(m_display, surface));
								delete window;
								m_hasWindow = 0;
								throw;
							}
						}
						else
						{
							createDestroyNdx--;
						}
					}
					else
					{
						eglu::NativeWindow* window	= DE_NULL;
						EGLSurface			surface = EGL_NO_SURFACE;

						try
						{
							window	= windowFactory.createWindow(&m_eglTestCtx.getNativeDisplay(), m_display, m_config, DE_NULL, eglu::WindowParams(64, 64, eglu::parseWindowVisibility(m_testCtx.getCommandLine())));
							surface	= eglu::createWindowSurface(m_eglTestCtx.getNativeDisplay(), *window, m_display, m_config, DE_NULL);

							thread.getLog() << ThreadLog::BeginMessage << surface << " = eglCreateWindowSurface()" << ThreadLog::EndMessage;
							windows.push_back(std::make_pair(window, surface));
						}
						catch (const std::exception&)
						{
							if (surface != EGL_NO_SURFACE)
								EGLU_CHECK_CALL(egl, destroySurface(m_display, surface));
							delete window;
							throw;
						}
					}
					break;
				}

				case TYPE_PIXMAP:
				{
					const eglu::NativePixmapFactory&	pixmapFactory	= eglu::selectNativePixmapFactory(m_eglTestCtx.getNativeDisplayFactory(), m_testCtx.getCommandLine());
					eglu::NativePixmap*					pixmap			= DE_NULL;
					EGLSurface							surface			= EGL_NO_SURFACE;

					try
					{
						pixmap	= pixmapFactory.createPixmap(&m_eglTestCtx.getNativeDisplay(), m_display, m_config, DE_NULL, 64, 64);
						surface	= eglu::createPixmapSurface(m_eglTestCtx.getNativeDisplay(), *pixmap, m_display, m_config, DE_NULL);

						thread.getLog() << ThreadLog::BeginMessage << surface << " = eglCreatePixmapSurface()" << ThreadLog::EndMessage;
						pixmaps.push_back(std::make_pair(pixmap, surface));
					}
					catch (const std::exception&)
					{
						if (surface != EGL_NO_SURFACE)
							EGLU_CHECK_CALL(egl, destroySurface(m_display, surface));
						delete pixmap;
						throw;
					}
					break;
				}

				case TYPE_CONTEXT:
				{
					EGLContext context;

					EGLU_CHECK_CALL(egl, bindAPI(EGL_OPENGL_ES_API));
					thread.getLog() << ThreadLog::BeginMessage << "eglBindAPI(EGL_OPENGL_ES_API)" << ThreadLog::EndMessage;

					const EGLint attributes[] =
					{
						EGL_CONTEXT_CLIENT_VERSION, 2,
						EGL_NONE
					};

					context = egl.createContext(m_display, m_config, EGL_NO_CONTEXT, attributes);
					thread.getLog() << ThreadLog::BeginMessage << context << " = eglCreateContext(" << m_display << ", " << m_config << ", EGL_NO_CONTEXT, { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE })" << ThreadLog::EndMessage;
					EGLU_CHECK_MSG(egl, "eglCreateContext()");
					contexts.push_back(context);
					break;
				}

				default:
					DE_ASSERT(false);
			};
		}
		else
		{
			switch (type)
			{
				case TYPE_PBUFFER:
				{
					const int pbufferNdx = rnd.getInt(0, (int)(pbuffers.size()-1));
					EGLBoolean result;

					result = egl.destroySurface(m_display, pbuffers[pbufferNdx]);
					thread.getLog() << ThreadLog::BeginMessage << result << " = eglDestroySurface(" << m_display << ", " << pbuffers[pbufferNdx] << ")" << ThreadLog::EndMessage;
					EGLU_CHECK_MSG(egl, "eglDestroySurface()");

					pbuffers.erase(pbuffers.begin() + pbufferNdx);

					break;
				}

				case TYPE_WINDOW:
				{
					const int windowNdx = rnd.getInt(0, (int)(windows.size()-1));

					thread.getLog() << ThreadLog::BeginMessage << "eglDestroySurface(" << m_display << ", " << windows[windowNdx].second << ")" << ThreadLog::EndMessage;

					EGLU_CHECK_CALL(egl, destroySurface(m_display, windows[windowNdx].second));
					windows[windowNdx].second = EGL_NO_SURFACE;
					delete windows[windowNdx].first;
					windows[windowNdx].first = DE_NULL;
					windows.erase(windows.begin() + windowNdx);

					if ((m_types & TYPE_SINGLE_WINDOW) != 0)
						m_hasWindow = 0;

					break;
				}

				case TYPE_PIXMAP:
				{
					const int pixmapNdx = rnd.getInt(0, (int)(pixmaps.size()-1));

					thread.getLog() << ThreadLog::BeginMessage << "eglDestroySurface(" << m_display << ", " << pixmaps[pixmapNdx].second << ")" << ThreadLog::EndMessage;
					EGLU_CHECK_CALL(egl, destroySurface(m_display, pixmaps[pixmapNdx].second));
					pixmaps[pixmapNdx].second = EGL_NO_SURFACE;
					delete pixmaps[pixmapNdx].first;
					pixmaps[pixmapNdx].first = DE_NULL;
					pixmaps.erase(pixmaps.begin() + pixmapNdx);

					break;
				}

				case TYPE_CONTEXT:
				{
					const int contextNdx = rnd.getInt(0, (int)(contexts.size()-1));

					EGLU_CHECK_CALL(egl, destroyContext(m_display, contexts[contextNdx]));
					thread.getLog() << ThreadLog::BeginMessage << "eglDestroyContext(" << m_display << ", " << contexts[contextNdx]  << ")" << ThreadLog::EndMessage;
					contexts.erase(contexts.begin() + contextNdx);

					break;
				}

				default:
					DE_ASSERT(false);
			}

		}
	}
}

void MultiThreadedObjectTest::pushObjectsToShared (TestThread& thread)
{
	vector<EGLSurface>&									pbuffers	= (thread.getId() == 0 ? m_pbuffers0 : m_pbuffers1);
	vector<pair<eglu::NativeWindow*, EGLSurface> >&		windows		= (thread.getId() == 0 ? m_nativeWindows0 : m_nativeWindows1);
	vector<pair<eglu::NativePixmap*, EGLSurface> >&		pixmaps		= (thread.getId() == 0 ? m_nativePixmaps0 : m_nativePixmaps1);
	vector<EGLContext>&									contexts	= (thread.getId() == 0 ? m_contexts0 : m_contexts1);

	for (int pbufferNdx = 0; pbufferNdx < (int)pbuffers.size(); pbufferNdx++)
		m_sharedPbuffers.push_back(pbuffers[pbufferNdx]);

	pbuffers.clear();

	for (int windowNdx = 0; windowNdx < (int)windows.size(); windowNdx++)
		m_sharedNativeWindows.push_back(windows[windowNdx]);

	windows.clear();

	for (int pixmapNdx = 0; pixmapNdx < (int)pixmaps.size(); pixmapNdx++)
		m_sharedNativePixmaps.push_back(pixmaps[pixmapNdx]);

	pixmaps.clear();

	for (int contextNdx = 0; contextNdx < (int)contexts.size(); contextNdx++)
		m_sharedContexts.push_back(contexts[contextNdx]);

	contexts.clear();
}

void MultiThreadedObjectTest::pullObjectsFromShared (TestThread& thread, int pbufferCount, int pixmapCount, int windowCount, int contextCount)
{
	de::Random&											rnd			= (thread.getId() == 0 ? m_rnd0 : m_rnd1);
	vector<EGLSurface>&									pbuffers	= (thread.getId() == 0 ? m_pbuffers0 : m_pbuffers1);
	vector<pair<eglu::NativeWindow*, EGLSurface> >&		windows		= (thread.getId() == 0 ? m_nativeWindows0 : m_nativeWindows1);
	vector<pair<eglu::NativePixmap*, EGLSurface> >&		pixmaps		= (thread.getId() == 0 ? m_nativePixmaps0 : m_nativePixmaps1);
	vector<EGLContext>&									contexts	= (thread.getId() == 0 ? m_contexts0 : m_contexts1);

	for (int pbufferNdx = 0; pbufferNdx < pbufferCount; pbufferNdx++)
	{
		const int ndx = rnd.getInt(0, (int)(m_sharedPbuffers.size()-1));

		pbuffers.push_back(m_sharedPbuffers[ndx]);
		m_sharedPbuffers.erase(m_sharedPbuffers.begin() + ndx);
	}

	for (int pixmapNdx = 0; pixmapNdx < pixmapCount; pixmapNdx++)
	{
		const int ndx = rnd.getInt(0, (int)(m_sharedNativePixmaps.size()-1));

		pixmaps.push_back(m_sharedNativePixmaps[ndx]);
		m_sharedNativePixmaps.erase(m_sharedNativePixmaps.begin() + ndx);
	}

	for (int windowNdx = 0; windowNdx < windowCount; windowNdx++)
	{
		const int ndx = rnd.getInt(0, (int)(m_sharedNativeWindows.size()-1));

		windows.push_back(m_sharedNativeWindows[ndx]);
		m_sharedNativeWindows.erase(m_sharedNativeWindows.begin() + ndx);
	}

	for (int contextNdx = 0; contextNdx < contextCount; contextNdx++)
	{
		const int ndx = rnd.getInt(0, (int)(m_sharedContexts.size()-1));

		contexts.push_back(m_sharedContexts[ndx]);
		m_sharedContexts.erase(m_sharedContexts.begin() + ndx);
	}
}

void MultiThreadedObjectTest::querySetSharedObjects (TestThread& thread, int count)
{
	const Library&		egl		= getLibrary();
	de::Random&			rnd		= (thread.getId() == 0 ? m_rnd0 : m_rnd1);
	vector<Type>		objectTypes;

	if ((m_types & TYPE_PBUFFER) != 0)
		objectTypes.push_back(TYPE_PBUFFER);

	if ((m_types & TYPE_PIXMAP) != 0)
		objectTypes.push_back(TYPE_PIXMAP);

	if (!m_sharedNativeWindows.empty() && (m_types & TYPE_WINDOW) != 0)
		objectTypes.push_back(TYPE_WINDOW);

	if ((m_types & TYPE_CONTEXT) != 0)
		objectTypes.push_back(TYPE_CONTEXT);

	for (int queryNdx = 0; queryNdx < count; queryNdx++)
	{
		const Type	type		= rnd.choose<Type>(objectTypes.begin(), objectTypes.end());
		EGLSurface	surface		= EGL_NO_SURFACE;
		EGLContext	context		= EGL_NO_CONTEXT;

		switch (type)
		{
			case TYPE_PBUFFER:
				surface = m_sharedPbuffers[rnd.getInt(0, (int)(m_sharedPbuffers.size()-1))];
				break;

			case TYPE_PIXMAP:
				surface = m_sharedNativePixmaps[rnd.getInt(0, (int)(m_sharedNativePixmaps.size()-1))].second;
				break;

			case TYPE_WINDOW:
				surface = m_sharedNativeWindows[rnd.getInt(0, (int)(m_sharedNativeWindows.size()-1))].second;
				break;

			case TYPE_CONTEXT:
				context = m_sharedContexts[rnd.getInt(0, (int)(m_sharedContexts.size()-1))];
				break;

			default:
				DE_ASSERT(false);
		}

		if (surface != EGL_NO_SURFACE)
		{
			static const EGLint queryAttributes[] =
			{
				EGL_LARGEST_PBUFFER,
				EGL_HEIGHT,
				EGL_WIDTH
			};

			const EGLint	attribute	= queryAttributes[rnd.getInt(0, DE_LENGTH_OF_ARRAY(queryAttributes) - 1)];
			EGLBoolean		result;
			EGLint			value;

			result = egl.querySurface(m_display, surface, attribute, &value);
			thread.getLog() << ThreadLog::BeginMessage << result << " = eglQuerySurface(" << m_display << ", " << surface << ", " << attribute << ", " << value << ")" << ThreadLog::EndMessage;
			EGLU_CHECK_MSG(egl, "eglQuerySurface()");

		}
		else if (context != EGL_NO_CONTEXT)
		{
			static const EGLint attributes[] =
			{
				EGL_CONFIG_ID,
				EGL_CONTEXT_CLIENT_TYPE,
				EGL_CONTEXT_CLIENT_VERSION,
				EGL_RENDER_BUFFER
			};

			const EGLint	attribute = attributes[rnd.getInt(0, DE_LENGTH_OF_ARRAY(attributes)-1)];
			EGLint			value;
			EGLBoolean		result;

			result = egl.queryContext(m_display, context, attribute, &value);
			thread.getLog() << ThreadLog::BeginMessage << result << " = eglQueryContext(" << m_display << ", " << context << ", " << attribute << ", " << value << ")" << ThreadLog::EndMessage;
			EGLU_CHECK_MSG(egl, "eglQueryContext()");

		}
		else
			DE_ASSERT(false);
	}
}

void MultiThreadedObjectTest::destroyObjects (TestThread& thread)
{
	const Library&										egl			= getLibrary();
	vector<EGLSurface>&									pbuffers	= (thread.getId() == 0 ? m_pbuffers0 : m_pbuffers1);
	vector<pair<eglu::NativeWindow*, EGLSurface> >&		windows		= (thread.getId() == 0 ? m_nativeWindows0 : m_nativeWindows1);
	vector<pair<eglu::NativePixmap*, EGLSurface> >&		pixmaps		= (thread.getId() == 0 ? m_nativePixmaps0 : m_nativePixmaps1);
	vector<EGLContext>&									contexts	= (thread.getId() == 0 ? m_contexts0 : m_contexts1);

	for (int pbufferNdx = 0; pbufferNdx < (int)pbuffers.size(); pbufferNdx++)
	{
		if (pbuffers[pbufferNdx] != EGL_NO_SURFACE)
		{
			// Destroy EGLSurface
			EGLBoolean result;

			result = egl.destroySurface(m_display, pbuffers[pbufferNdx]);
			thread.getLog() << ThreadLog::BeginMessage << result << " = eglDestroySurface(" << m_display << ", " << pbuffers[pbufferNdx] << ")" << ThreadLog::EndMessage;
			EGLU_CHECK_MSG(egl, "eglDestroySurface()");
			pbuffers[pbufferNdx] = EGL_NO_SURFACE;
		}
	}
	pbuffers.clear();

	for (int windowNdx = 0; windowNdx < (int)windows.size(); windowNdx++)
	{
		if (windows[windowNdx].second != EGL_NO_SURFACE)
		{
			thread.getLog() << ThreadLog::BeginMessage << "eglDestroySurface(" << m_display << ", " << windows[windowNdx].second << ")" << ThreadLog::EndMessage;
			EGLU_CHECK_CALL(egl, destroySurface(m_display, windows[windowNdx].second));
			windows[windowNdx].second = EGL_NO_SURFACE;
		}

		if (windows[windowNdx].first)
		{
			delete windows[windowNdx].first;
			windows[windowNdx].first = NULL;
		}
	}
	windows.clear();

	for (int pixmapNdx = 0; pixmapNdx < (int)pixmaps.size(); pixmapNdx++)
	{
		if (pixmaps[pixmapNdx].first != EGL_NO_SURFACE)
		{
			thread.getLog() << ThreadLog::BeginMessage << "eglDestroySurface(" << m_display << ", " << pixmaps[pixmapNdx].second << ")" << ThreadLog::EndMessage;
			EGLU_CHECK_CALL(egl, destroySurface(m_display, pixmaps[pixmapNdx].second));
			pixmaps[pixmapNdx].second = EGL_NO_SURFACE;
		}

		if (pixmaps[pixmapNdx].first)
		{
			delete pixmaps[pixmapNdx].first;
			pixmaps[pixmapNdx].first = NULL;
		}
	}
	pixmaps.clear();

	for (int contextNdx = 0; contextNdx < (int)contexts.size(); contextNdx++)
	{
		if (contexts[contextNdx] != EGL_NO_CONTEXT)
		{
			EGLU_CHECK_CALL(egl, destroyContext(m_display, contexts[contextNdx]));
			thread.getLog() << ThreadLog::BeginMessage << "eglDestroyContext(" << m_display << ", " << contexts[contextNdx]  << ")" << ThreadLog::EndMessage;
			contexts[contextNdx] = EGL_NO_CONTEXT;
		}
	}
	contexts.clear();
}

MultiThreadedTests::MultiThreadedTests (EglTestContext& context)
	: TestCaseGroup(context, "multithread", "Multithreaded EGL tests")
{
}

void MultiThreadedTests::init (void)
{
	// Config tests
	addChild(new MultiThreadedConfigTest(m_eglTestCtx,	"config",	"",	30,	30,	30));

	// Object tests
	addChild(new MultiThreadedObjectTest(m_eglTestCtx,	"pbuffer",								"", MultiThreadedObjectTest::TYPE_PBUFFER));
	addChild(new MultiThreadedObjectTest(m_eglTestCtx,	"pixmap",								"", MultiThreadedObjectTest::TYPE_PIXMAP));
	addChild(new MultiThreadedObjectTest(m_eglTestCtx,	"window",								"", MultiThreadedObjectTest::TYPE_WINDOW));
	addChild(new MultiThreadedObjectTest(m_eglTestCtx,	"single_window",						"", MultiThreadedObjectTest::TYPE_WINDOW|MultiThreadedObjectTest::TYPE_SINGLE_WINDOW));
	addChild(new MultiThreadedObjectTest(m_eglTestCtx,	"context",								"", MultiThreadedObjectTest::TYPE_CONTEXT));

	addChild(new MultiThreadedObjectTest(m_eglTestCtx,	"pbuffer_pixmap",						"", MultiThreadedObjectTest::TYPE_PBUFFER|MultiThreadedObjectTest::TYPE_PIXMAP));
	addChild(new MultiThreadedObjectTest(m_eglTestCtx,	"pbuffer_window",						"", MultiThreadedObjectTest::TYPE_PBUFFER|MultiThreadedObjectTest::TYPE_WINDOW));
	addChild(new MultiThreadedObjectTest(m_eglTestCtx,	"pbuffer_single_window",				"", MultiThreadedObjectTest::TYPE_PBUFFER|MultiThreadedObjectTest::TYPE_WINDOW|MultiThreadedObjectTest::TYPE_SINGLE_WINDOW));
	addChild(new MultiThreadedObjectTest(m_eglTestCtx,	"pbuffer_context",						"", MultiThreadedObjectTest::TYPE_PBUFFER|MultiThreadedObjectTest::TYPE_CONTEXT));

	addChild(new MultiThreadedObjectTest(m_eglTestCtx,	"pixmap_window",						"", MultiThreadedObjectTest::TYPE_PIXMAP|MultiThreadedObjectTest::TYPE_WINDOW));
	addChild(new MultiThreadedObjectTest(m_eglTestCtx,	"pixmap_single_window",					"", MultiThreadedObjectTest::TYPE_PIXMAP|MultiThreadedObjectTest::TYPE_WINDOW|MultiThreadedObjectTest::TYPE_SINGLE_WINDOW));
	addChild(new MultiThreadedObjectTest(m_eglTestCtx,	"pixmap_context",						"", MultiThreadedObjectTest::TYPE_PIXMAP|MultiThreadedObjectTest::TYPE_CONTEXT));

	addChild(new MultiThreadedObjectTest(m_eglTestCtx,	"window_context",						"", MultiThreadedObjectTest::TYPE_WINDOW|MultiThreadedObjectTest::TYPE_CONTEXT));
	addChild(new MultiThreadedObjectTest(m_eglTestCtx,	"single_window_context",				"", MultiThreadedObjectTest::TYPE_WINDOW|MultiThreadedObjectTest::TYPE_SINGLE_WINDOW|MultiThreadedObjectTest::TYPE_CONTEXT));

	addChild(new MultiThreadedObjectTest(m_eglTestCtx,	"pbuffer_pixmap_window",				"", MultiThreadedObjectTest::TYPE_PBUFFER|MultiThreadedObjectTest::TYPE_PIXMAP|MultiThreadedObjectTest::TYPE_WINDOW));
	addChild(new MultiThreadedObjectTest(m_eglTestCtx,	"pbuffer_pixmap_single_window",			"", MultiThreadedObjectTest::TYPE_PBUFFER|MultiThreadedObjectTest::TYPE_PIXMAP|MultiThreadedObjectTest::TYPE_WINDOW|MultiThreadedObjectTest::TYPE_SINGLE_WINDOW));
	addChild(new MultiThreadedObjectTest(m_eglTestCtx,	"pbuffer_pixmap_context",				"", MultiThreadedObjectTest::TYPE_PBUFFER|MultiThreadedObjectTest::TYPE_PIXMAP|MultiThreadedObjectTest::TYPE_CONTEXT));

	addChild(new MultiThreadedObjectTest(m_eglTestCtx,	"pbuffer_window_context",				"", MultiThreadedObjectTest::TYPE_PBUFFER|MultiThreadedObjectTest::TYPE_WINDOW|MultiThreadedObjectTest::TYPE_CONTEXT));
	addChild(new MultiThreadedObjectTest(m_eglTestCtx,	"pbuffer_single_window_context",		"", MultiThreadedObjectTest::TYPE_PBUFFER|MultiThreadedObjectTest::TYPE_WINDOW|MultiThreadedObjectTest::TYPE_SINGLE_WINDOW|MultiThreadedObjectTest::TYPE_CONTEXT));

	addChild(new MultiThreadedObjectTest(m_eglTestCtx,	"pixmap_window_context",				"", MultiThreadedObjectTest::TYPE_PIXMAP|MultiThreadedObjectTest::TYPE_WINDOW|MultiThreadedObjectTest::TYPE_CONTEXT));
	addChild(new MultiThreadedObjectTest(m_eglTestCtx,	"pixmap_single_window_context",			"", MultiThreadedObjectTest::TYPE_PIXMAP|MultiThreadedObjectTest::TYPE_WINDOW|MultiThreadedObjectTest::TYPE_SINGLE_WINDOW|MultiThreadedObjectTest::TYPE_CONTEXT));

	addChild(new MultiThreadedObjectTest(m_eglTestCtx,	"pbuffer_pixmap_window_context",		"", MultiThreadedObjectTest::TYPE_PBUFFER|MultiThreadedObjectTest::TYPE_PIXMAP|MultiThreadedObjectTest::TYPE_WINDOW|MultiThreadedObjectTest::TYPE_CONTEXT));
	addChild(new MultiThreadedObjectTest(m_eglTestCtx,	"pbuffer_pixmap_single_window_context",	"", MultiThreadedObjectTest::TYPE_PBUFFER|MultiThreadedObjectTest::TYPE_PIXMAP|MultiThreadedObjectTest::TYPE_WINDOW|MultiThreadedObjectTest::TYPE_SINGLE_WINDOW|MultiThreadedObjectTest::TYPE_CONTEXT));
}

} // egl
} // deqp
