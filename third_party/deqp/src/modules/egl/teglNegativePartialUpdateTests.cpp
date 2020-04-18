/*-------------------------------------------------------------------------
 * drawElements Quality Program EGL Module
 * ---------------------------------------
 *
 * Copyright 2015 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 *//*!
 * \file
 * \brief Test negative use case of KHR_partial_update
 *//*--------------------------------------------------------------------*/

#include "teglNegativePartialUpdateTests.hpp"

#include "tcuTestLog.hpp"
#include "tcuSurface.hpp"

#include "egluCallLogWrapper.hpp"
#include "egluConfigFilter.hpp"
#include "egluNativeWindow.hpp"
#include "egluStrUtil.hpp"
#include "egluUnique.hpp"
#include "egluUtil.hpp"

#include "eglwLibrary.hpp"
#include "eglwEnums.hpp"

using namespace eglu;
using namespace eglw;
using tcu::TestLog;

namespace deqp
{
namespace egl
{
namespace
{

class NegativePartialUpdateTest : public TestCase
{
public:
	enum SurfaceType
	{
		SURFACETYPE_WINDOW = 0,
		SURFACETYPE_PBUFFER
	};

								NegativePartialUpdateTest		(EglTestContext& eglTestCtx, bool preserveBuffer, SurfaceType surfaceType, const char* name, const char* description);
								~NegativePartialUpdateTest		(void);
	void						init							(void);
	void						deinit							(void);
	virtual IterateResult		iterate							(void) = 0;

protected:
	void						expectError						(eglw::EGLenum error);
	void						expectBoolean					(EGLBoolean expected, EGLBoolean got);
	inline void					expectTrue						(eglw::EGLBoolean got) { expectBoolean(EGL_TRUE, got); }
	inline void					expectFalse						(eglw::EGLBoolean got) { expectBoolean(EGL_FALSE, got); }

	const bool					m_preserveBuffer;
	SurfaceType					m_surfaceType;
	EGLDisplay					m_eglDisplay;
	EGLConfig					m_eglConfig;
	NativeWindow*				m_window;
	EGLSurface					m_eglSurface;
	EGLContext					m_eglContext;
};

bool isWindow (const CandidateConfig& c)
{
	return (c.surfaceType() & EGL_WINDOW_BIT) == EGL_WINDOW_BIT;
}

bool isPbuffer (const CandidateConfig& c)
{
	return (c.surfaceType() & EGL_PBUFFER_BIT) == EGL_PBUFFER_BIT;
}

bool isES2Renderable (const CandidateConfig& c)
{
	return (c.get(EGL_RENDERABLE_TYPE) & EGL_OPENGL_ES2_BIT) == EGL_OPENGL_ES2_BIT;
}

bool hasPreserveSwap (const CandidateConfig& c)
{
	return (c.surfaceType() & EGL_SWAP_BEHAVIOR_PRESERVED_BIT) == EGL_SWAP_BEHAVIOR_PRESERVED_BIT;
}

EGLConfig getEGLConfig (const Library& egl, EGLDisplay eglDisplay, NegativePartialUpdateTest::SurfaceType surfaceType, bool preserveBuffer)
{
	FilterList filters;
	if (surfaceType == NegativePartialUpdateTest::SURFACETYPE_WINDOW)
		filters << isWindow;
	else if (surfaceType == NegativePartialUpdateTest::SURFACETYPE_PBUFFER)
		filters << isPbuffer;
	else
		DE_FATAL("Invalid surfaceType");

	filters << isES2Renderable;

	if (preserveBuffer)
		filters << hasPreserveSwap;

	return chooseSingleConfig(egl, eglDisplay, filters);
}

EGLContext initAndMakeCurrentEGLContext (const Library& egl, EGLDisplay eglDisplay, EGLSurface eglSurface, EGLConfig eglConfig, const EGLint* attribList)
{
	EGLContext eglContext = EGL_NO_CONTEXT;

	egl.bindAPI(EGL_OPENGL_ES_API);
	eglContext = egl.createContext(eglDisplay, eglConfig, EGL_NO_CONTEXT, attribList);
	EGLU_CHECK_MSG(egl, "eglCreateContext");
	TCU_CHECK(eglSurface != EGL_NO_SURFACE);
	egl.makeCurrent(eglDisplay, eglSurface, eglSurface, eglContext);
	EGLU_CHECK_MSG(egl, "eglMakeCurrent");

	return eglContext;
}

NegativePartialUpdateTest::NegativePartialUpdateTest (EglTestContext& eglTestCtx, bool preserveBuffer, SurfaceType surfaceType, const char* name, const char* description)
	: TestCase			(eglTestCtx, name, description)
	, m_preserveBuffer	(preserveBuffer)
	, m_surfaceType		(surfaceType)
	, m_eglDisplay		(EGL_NO_DISPLAY)
	, m_window			(DE_NULL)
	, m_eglSurface		(EGL_NO_SURFACE)
	, m_eglContext		(EGL_NO_CONTEXT)
{
}

NegativePartialUpdateTest::~NegativePartialUpdateTest (void)
{
	deinit();
}

void NegativePartialUpdateTest::init (void)
{
	const Library&		egl						= m_eglTestCtx.getLibrary();
	static const EGLint	contextAttribList[]		= { EGL_CONTEXT_CLIENT_VERSION, 2,	EGL_NONE };
	const int			width					= 480;
	const int			height					= 480;

	m_eglDisplay = getAndInitDisplay(m_eglTestCtx.getNativeDisplay());

	if (!hasExtension(egl, m_eglDisplay, "EGL_KHR_partial_update"))
		TCU_THROW(NotSupportedError, "EGL_KHR_partial_update is not supported");

	m_eglConfig = getEGLConfig(egl, m_eglDisplay, m_surfaceType, m_preserveBuffer);

	if (m_surfaceType == SURFACETYPE_PBUFFER)
	{
		const EGLint pbufferAttribList[] = { EGL_WIDTH, width, EGL_HEIGHT, height, EGL_NONE };
		m_eglSurface = egl.createPbufferSurface(m_eglDisplay, m_eglConfig, pbufferAttribList);
	}
	else
	{
		const NativeWindowFactory&	factory	= selectNativeWindowFactory(m_eglTestCtx.getNativeDisplayFactory(), m_testCtx.getCommandLine());
		m_window = factory.createWindow(&m_eglTestCtx.getNativeDisplay(), m_eglDisplay, m_eglConfig, DE_NULL,
										WindowParams(width, height, parseWindowVisibility(m_testCtx.getCommandLine())));
		m_eglSurface = createWindowSurface(m_eglTestCtx.getNativeDisplay(), *m_window, m_eglDisplay, m_eglConfig, DE_NULL);
	}
	m_eglContext = initAndMakeCurrentEGLContext(egl, m_eglDisplay, m_eglSurface, m_eglConfig, contextAttribList);
}

void NegativePartialUpdateTest::deinit (void)
{
	const Library& egl = m_eglTestCtx.getLibrary();

	if (m_eglContext != EGL_NO_CONTEXT)
	{
		EGLU_CHECK_CALL(egl, makeCurrent(m_eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT));
		EGLU_CHECK_CALL(egl, destroyContext(m_eglDisplay, m_eglContext));
		m_eglContext = EGL_NO_CONTEXT;
	}

	if (m_eglSurface != EGL_NO_SURFACE)
	{
		EGLU_CHECK_CALL(egl, destroySurface(m_eglDisplay, m_eglSurface));
		m_eglSurface = EGL_NO_SURFACE;
	}

	if (m_eglDisplay != EGL_NO_DISPLAY)
	{
		EGLU_CHECK_CALL(egl, terminate(m_eglDisplay));
		m_eglDisplay = EGL_NO_DISPLAY;
	}

	delete m_window;
	m_window = DE_NULL;
}

void NegativePartialUpdateTest::expectError (EGLenum expected)
{
	const EGLenum err = m_eglTestCtx.getLibrary().getError();

	if (err != expected)
	{
		m_testCtx.getLog() << TestLog::Message << "// ERROR expected: " << eglu::getErrorStr(expected) << ", Got: " << eglu::getErrorStr(err) << TestLog::EndMessage;
		if (m_testCtx.getTestResult() == QP_TEST_RESULT_PASS)
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Got invalid error");
	}
}

void NegativePartialUpdateTest::expectBoolean (EGLBoolean expected, EGLBoolean got)
{
	if (expected != got)
	{
		m_testCtx.getLog() << TestLog::Message << "// ERROR expected: " << eglu::getBooleanStr(expected) <<  ", Got: " << eglu::getBooleanStr(got) << TestLog::EndMessage;
		if (m_testCtx.getTestResult() == QP_TEST_RESULT_PASS)
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Got invalid value");
	}
}

class NotPostableTest : public NegativePartialUpdateTest
{
public:
							NotPostableTest (EglTestContext& context);
	TestCase::IterateResult iterate			(void);
};

NotPostableTest::NotPostableTest (EglTestContext& context)
	: NegativePartialUpdateTest (context, false, SURFACETYPE_PBUFFER, "not_postable_surface",  "Call setDamageRegion() on pbuffer")
{
}

TestCase::IterateResult NotPostableTest::iterate (void)
{
	const Library&			egl				= m_eglTestCtx.getLibrary();
	TestLog&				log				= m_testCtx.getLog();
	CallLogWrapper			wrapper			(egl, log);
	EGLint					damageRegion[]	= { 10, 10, 10, 10 };
	int						bufferAge		= -1;

	wrapper.enableLogging(true);
	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	{
		tcu::ScopedLogSection(log, "Test1", "If the surface is pbuffer (not postable) --> EGL_BAD_MATCH");
		EGLU_CHECK_CALL(egl, surfaceAttrib(m_eglDisplay, m_eglSurface, EGL_SWAP_BEHAVIOR, EGL_BUFFER_DESTROYED));
		EGLU_CHECK_CALL(egl, querySurface(m_eglDisplay, m_eglSurface, EGL_BUFFER_AGE_KHR, &bufferAge));
		expectFalse(wrapper.eglSetDamageRegionKHR(m_eglDisplay, m_eglSurface, damageRegion, DE_LENGTH_OF_ARRAY(damageRegion)/4));
		expectError(EGL_BAD_MATCH);
	}

	return STOP;
}

class NotCurrentSurfaceTest : public NegativePartialUpdateTest
{
public:
							NotCurrentSurfaceTest	(EglTestContext& context);
	TestCase::IterateResult iterate					(void);
};

NotCurrentSurfaceTest::NotCurrentSurfaceTest (EglTestContext& context)
	: NegativePartialUpdateTest (context, false, SURFACETYPE_WINDOW, "not_current_surface",  "Call setDamageRegion() on pbuffer")
{
}

TestCase::IterateResult NotCurrentSurfaceTest::iterate (void)
{
	const int					impossibleBufferAge = -26084;
	const Library&				egl					= m_eglTestCtx.getLibrary();
	const EGLConfig				config				= getEGLConfig(egl, m_eglDisplay, SURFACETYPE_PBUFFER, false);
	const EGLint				attribList[]		=
	{
		EGL_WIDTH,	64,
		EGL_HEIGHT,	64,
		EGL_NONE
	};
	const eglu::UniqueSurface	dummyPbuffer		(egl, m_eglDisplay, egl.createPbufferSurface(m_eglDisplay, config, attribList));
	TestLog&					log					= m_testCtx.getLog();
	CallLogWrapper				wrapper				(egl, log);
	EGLint						damageRegion[]		= { 10, 10, 10, 10 };
	int							bufferAge			= impossibleBufferAge;

	wrapper.enableLogging(true);
	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	EGLU_CHECK_CALL(egl, makeCurrent(m_eglDisplay, *dummyPbuffer, *dummyPbuffer, m_eglContext));
	{
		tcu::ScopedLogSection(log, "Test2.1", "If query buffer age on a surface that is not the current draw surface --> EGL_BAD_SURFACE");
		EGLU_CHECK_CALL(egl, surfaceAttrib(m_eglDisplay, m_eglSurface, EGL_SWAP_BEHAVIOR, EGL_BUFFER_DESTROYED));
		expectFalse(wrapper.eglQuerySurface(m_eglDisplay, m_eglSurface, EGL_BUFFER_AGE_KHR, &bufferAge));
		expectError(EGL_BAD_SURFACE);

		if (bufferAge != impossibleBufferAge)
		{
			log << tcu::TestLog::Message << "On failure, eglQuerySurface shouldn't change buffer age but buffer age has been changed to " << bufferAge << tcu::TestLog::EndMessage;
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail, bufferAge shouldn't be changed");
		}
	}
	{
		tcu::ScopedLogSection(log, "Test2.2", "If call setDamageRegion() on a surface that is not the current draw surface --> EGL_BAD_MATCH");
		expectFalse(wrapper.eglSetDamageRegionKHR(m_eglDisplay, m_eglSurface, damageRegion, 1));
		expectError(EGL_BAD_MATCH);
	}

	EGLU_CHECK_CALL(egl, makeCurrent(m_eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT));
	{
		tcu::ScopedLogSection(log, "Test3.1", "If query buffer age on a surface that is not the current draw surface --> EGL_BAD_SURFACE");
		EGLU_CHECK_CALL(egl, surfaceAttrib(m_eglDisplay, m_eglSurface, EGL_SWAP_BEHAVIOR, EGL_BUFFER_DESTROYED));
		expectFalse(wrapper.eglQuerySurface(m_eglDisplay, m_eglSurface, EGL_BUFFER_AGE_KHR, &bufferAge));
		expectError(EGL_BAD_SURFACE);

		if (bufferAge != impossibleBufferAge)
		{
			log << tcu::TestLog::Message << "On failure, eglQuerySurface shouldn't change buffer age but buffer age has been changed to " << bufferAge << tcu::TestLog::EndMessage;
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail, bufferAge shouldn't be changed");
		}
	}
	{
		tcu::ScopedLogSection(log, "Test3.2", "If call setDamageRegion() on a surface that is not the current draw surface --> EGL_BAD_MATCH");
		expectFalse(wrapper.eglSetDamageRegionKHR(m_eglDisplay, m_eglSurface, damageRegion, 1));
		expectError(EGL_BAD_MATCH);
	}

	if (hasExtension(egl, m_eglDisplay, "EGL_KHR_surfaceless_context"))
	{
		EGLU_CHECK_CALL(egl, makeCurrent(m_eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, m_eglContext));
		{
			tcu::ScopedLogSection(log, "Test4.1", "If query buffer age on a surface that is not the current draw surface --> EGL_BAD_SURFACE");
			EGLU_CHECK_CALL(egl, surfaceAttrib(m_eglDisplay, m_eglSurface, EGL_SWAP_BEHAVIOR, EGL_BUFFER_DESTROYED));
			expectFalse(wrapper.eglQuerySurface(m_eglDisplay, m_eglSurface, EGL_BUFFER_AGE_KHR, &bufferAge));
			expectError(EGL_BAD_SURFACE);

			if (bufferAge != impossibleBufferAge)
			{
				log << tcu::TestLog::Message << "On failure, eglQuerySurface shouldn't change buffer age but buffer age has been changed to " << bufferAge << tcu::TestLog::EndMessage;
				m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail, bufferAge shouldn't be changed");
			}
		}
		{
			tcu::ScopedLogSection(log, "Test4.2", "If call setDamageRegion() on a surface that is not the current draw surface --> EGL_BAD_MATCH");
			expectFalse(wrapper.eglSetDamageRegionKHR(m_eglDisplay, m_eglSurface, damageRegion, 1));
			expectError(EGL_BAD_MATCH);
		}
	}

	return STOP;
}

class BufferPreservedTest : public NegativePartialUpdateTest
{
public:
							BufferPreservedTest (EglTestContext& context);
	TestCase::IterateResult iterate				(void);
};

BufferPreservedTest::BufferPreservedTest (EglTestContext& context)
	: NegativePartialUpdateTest (context, true, SURFACETYPE_WINDOW, "buffer_preserved",  "Call setDamageRegion() on pbuffer")
{
}

TestCase::IterateResult BufferPreservedTest::iterate (void)
{
	const Library&			egl				= m_eglTestCtx.getLibrary();
	TestLog&				log				= m_testCtx.getLog();
	CallLogWrapper			wrapper			(egl, log);
	EGLint					damageRegion[]	= { 10, 10, 10, 10 };
	int						bufferAge		= -1;

	wrapper.enableLogging(true);
	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	{
		tcu::ScopedLogSection(log, "Test3", "If buffer_preserved --> EGL_BAD_MATCH");
		EGLU_CHECK_CALL(egl, surfaceAttrib(m_eglDisplay, m_eglSurface, EGL_SWAP_BEHAVIOR, EGL_BUFFER_PRESERVED));
		EGLU_CHECK_CALL(egl, querySurface(m_eglDisplay, m_eglSurface, EGL_BUFFER_AGE_KHR, &bufferAge));
		expectFalse(wrapper.eglSetDamageRegionKHR(m_eglDisplay, m_eglSurface, damageRegion, DE_LENGTH_OF_ARRAY(damageRegion)/4));
		expectError(EGL_BAD_MATCH);
	}

	return STOP;
}

class SetTwiceTest : public NegativePartialUpdateTest
{
public:
							SetTwiceTest		(EglTestContext& context);
	TestCase::IterateResult iterate				(void);
};

SetTwiceTest::SetTwiceTest (EglTestContext& context)
	: NegativePartialUpdateTest (context, false, SURFACETYPE_WINDOW, "set_damage_region_twice",  "Call setDamageRegion() twice")
{
}

TestCase::IterateResult SetTwiceTest::iterate (void)
{
	const Library&			egl				= m_eglTestCtx.getLibrary();
	TestLog&				log				= m_testCtx.getLog();
	CallLogWrapper			wrapper			(egl, log);
	EGLint					damageRegion[]	= { 10, 10, 10, 10 };
	int						bufferAge		= -1;

	wrapper.enableLogging(true);
	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	{
		tcu::ScopedLogSection(log, "Test4", "If call setDamageRegion() twice --> EGL_BAD_ACCESS");
		EGLU_CHECK_CALL(egl, surfaceAttrib(m_eglDisplay, m_eglSurface, EGL_SWAP_BEHAVIOR, EGL_BUFFER_DESTROYED));
		EGLU_CHECK_CALL(egl, querySurface(m_eglDisplay, m_eglSurface, EGL_BUFFER_AGE_KHR, &bufferAge));
		expectTrue(wrapper.eglSetDamageRegionKHR(m_eglDisplay, m_eglSurface, damageRegion, DE_LENGTH_OF_ARRAY(damageRegion)/4));
		expectFalse(wrapper.eglSetDamageRegionKHR(m_eglDisplay, m_eglSurface, damageRegion, DE_LENGTH_OF_ARRAY(damageRegion)/4));
		expectError(EGL_BAD_ACCESS);
	}

	return STOP;
}


class NoAgeTest : public NegativePartialUpdateTest
{
public:
							NoAgeTest			(EglTestContext& context);
	TestCase::IterateResult iterate				(void);
};

NoAgeTest::NoAgeTest (EglTestContext& context)
	: NegativePartialUpdateTest (context, false, SURFACETYPE_WINDOW, "set_damage_region_before_query_age",  "Call setDamageRegion() without querying buffer age")
{
}

TestCase::IterateResult NoAgeTest::iterate (void)
{
	const Library&			egl				= m_eglTestCtx.getLibrary();
	TestLog&				log				= m_testCtx.getLog();
	CallLogWrapper			wrapper			(egl, log);
	EGLint					damageRegion[]	= { 10, 10, 10, 10 };

	wrapper.enableLogging(true);
	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	{
		tcu::ScopedLogSection(log, "Test5", "If buffer age is not queried --> EGL_BAD_ACCESS");
		EGLU_CHECK_CALL(egl, surfaceAttrib(m_eglDisplay, m_eglSurface, EGL_SWAP_BEHAVIOR, EGL_BUFFER_DESTROYED));
		expectFalse(wrapper.eglSetDamageRegionKHR(m_eglDisplay, m_eglSurface, damageRegion, DE_LENGTH_OF_ARRAY(damageRegion)/4));
		expectError(EGL_BAD_ACCESS);
	}

	return STOP;
}

class PassNullTest : public NegativePartialUpdateTest
{
public:
							PassNullTest			(EglTestContext& context);
	TestCase::IterateResult iterate					(void);
};

PassNullTest::PassNullTest (EglTestContext& context)
	: NegativePartialUpdateTest (context, false, SURFACETYPE_WINDOW, "pass_null_0_as_params",  "Call setDamageRegion() with (NULL, 0)")
{
}

TestCase::IterateResult PassNullTest::iterate (void)
{
	const Library&			egl				= m_eglTestCtx.getLibrary();
	TestLog&				log				= m_testCtx.getLog();
	CallLogWrapper			wrapper			(egl, log);
	int						bufferAge		= -1;

	wrapper.enableLogging(true);
	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	{
		tcu::ScopedLogSection(log, "Test6", "If pass (null, 0) to setDamageRegion(), no error");
		EGLU_CHECK_CALL(egl, surfaceAttrib(m_eglDisplay, m_eglSurface, EGL_SWAP_BEHAVIOR, EGL_BUFFER_DESTROYED));
		EGLU_CHECK_CALL(egl, querySurface(m_eglDisplay, m_eglSurface, EGL_BUFFER_AGE_KHR, &bufferAge));
		expectTrue(wrapper.eglSetDamageRegionKHR(m_eglDisplay, m_eglSurface, DE_NULL, 0));
		expectError(EGL_SUCCESS);
	}

	return STOP;
}

class NotCurrentSurfaceTest2 : public NegativePartialUpdateTest
{
public:
							NotCurrentSurfaceTest2	(EglTestContext& context);
	TestCase::IterateResult iterate					(void);
};

NotCurrentSurfaceTest2::NotCurrentSurfaceTest2 (EglTestContext& context)
	: NegativePartialUpdateTest (context, false, SURFACETYPE_WINDOW, "not_current_surface2",  "Call setDamageRegion() on pbuffer")
{
}

TestCase::IterateResult NotCurrentSurfaceTest2::iterate (void)
{
	const Library&				egl				= m_eglTestCtx.getLibrary();
	const EGLConfig				config			= getEGLConfig(egl, m_eglDisplay, SURFACETYPE_PBUFFER, false);
	const EGLint				attribList[]	=
	{
		EGL_WIDTH,	64,
		EGL_HEIGHT,	64,
		EGL_NONE
	};
	const eglu::UniqueSurface	dummyPbuffer	(egl, m_eglDisplay, egl.createPbufferSurface(m_eglDisplay, config, attribList));
	TestLog&					log				= m_testCtx.getLog();
	CallLogWrapper				wrapper			(egl, log);
	EGLint						damageRegion[]	= { 10, 10, 10, 10 };
	int							bufferAge		= -1;

	wrapper.enableLogging(true);
	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

	EGLU_CHECK_CALL(egl, surfaceAttrib(m_eglDisplay, m_eglSurface, EGL_SWAP_BEHAVIOR, EGL_BUFFER_DESTROYED));
	EGLU_CHECK_CALL(egl, querySurface(m_eglDisplay, m_eglSurface, EGL_BUFFER_AGE_KHR, &bufferAge));

	{
		tcu::ScopedLogSection(log, "Test7", "If call setDamageRegion() on a surface that is not the current draw surface --> EGL_BAD_MATCH");
		EGLU_CHECK_CALL(egl, makeCurrent(m_eglDisplay, *dummyPbuffer, *dummyPbuffer, m_eglContext));
		expectFalse(wrapper.eglSetDamageRegionKHR(m_eglDisplay, m_eglSurface, damageRegion, 1));
		expectError(EGL_BAD_MATCH);
	}
	{
		tcu::ScopedLogSection(log, "Test8", "If call setDamageRegion() on a surface that is not the current draw surface --> EGL_BAD_MATCH");
		EGLU_CHECK_CALL(egl, makeCurrent(m_eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT));
		expectFalse(wrapper.eglSetDamageRegionKHR(m_eglDisplay, m_eglSurface, damageRegion, 1));
		expectError(EGL_BAD_MATCH);
	}
	if (hasExtension(egl, m_eglDisplay, "EGL_KHR_surfaceless_context"))
	{
		tcu::ScopedLogSection(log, "Test9", "If call setDamageRegion() on a surface that is not the current draw surface --> EGL_BAD_MATCH");
		EGLU_CHECK_CALL(egl, makeCurrent(m_eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, m_eglContext));
		expectFalse(wrapper.eglSetDamageRegionKHR(m_eglDisplay, m_eglSurface, damageRegion, 1));
		expectError(EGL_BAD_MATCH);
	}

	return STOP;
}

} // anonymous

NegativePartialUpdateTests::NegativePartialUpdateTests (EglTestContext& eglTestCtx)
	: TestCaseGroup(eglTestCtx, "negative_partial_update", "Negative partial update tests")
{
}

void NegativePartialUpdateTests::init (void)
{
	addChild(new NotPostableTest(m_eglTestCtx));
	addChild(new NotCurrentSurfaceTest(m_eglTestCtx));
	addChild(new BufferPreservedTest(m_eglTestCtx));
	addChild(new SetTwiceTest(m_eglTestCtx));
	addChild(new NoAgeTest(m_eglTestCtx));
	addChild(new PassNullTest(m_eglTestCtx));
	addChild(new NotCurrentSurfaceTest2(m_eglTestCtx));
}

} // egl
} // deqp
