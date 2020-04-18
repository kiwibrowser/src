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
 * \brief Memory object allocation stress tests
 *//*--------------------------------------------------------------------*/

#include "teglMemoryStressTests.hpp"

#include "tcuTestLog.hpp"
#include "tcuCommandLine.hpp"

#include "deRandom.hpp"
#include "deClock.h"
#include "deString.h"

#include "gluDefs.hpp"
#include "glwFunctions.hpp"
#include "glwDefs.hpp"
#include "glwEnums.hpp"

#include "egluUtil.hpp"

#include "eglwLibrary.hpp"
#include "eglwEnums.hpp"

#include <vector>
#include <string>

using std::vector;
using std::string;
using tcu::TestLog;

using namespace eglw;

namespace deqp
{
namespace egl
{

namespace
{

enum ObjectType
{
	OBJECTTYPE_PBUFFER = (1<<0),
	OBJECTTYPE_CONTEXT = (1<<1),

//	OBJECTTYPE_WINDOW,
//	OBJECTTYPE_PIXMAP,
};

class MemoryAllocator
{
public:
					MemoryAllocator			(EglTestContext& eglTestCtx, EGLDisplay display, EGLConfig config, int seed, ObjectType types, int minWidth, int minHeight, int maxWidth, int maxHeight, bool use);
					~MemoryAllocator		(void);

	bool			allocateUntilFailure	(void);
	int				getAllocationCount		(void) const { return (int)(m_pbuffers.size() + m_contexts.size());	}
	int				getContextCount			(void) const { return (int)m_contexts.size();						}
	int				getPBufferCount			(void) const { return (int)m_pbuffers.size();						}
	const string&	getErrorString			(void) const { return m_errorString;							}

private:
	void			allocatePBuffer			(void);
	void			allocateContext			(void);

	EglTestContext&			m_eglTestCtx;
	EGLDisplay				m_display;
	EGLConfig				m_config;
	glw::Functions			m_gl;

	de::Random				m_rnd;
	bool					m_failed;
	string					m_errorString;

	ObjectType				m_types;
	int						m_minWidth;
	int						m_minHeight;
	int						m_maxWidth;
	int						m_maxHeight;
	bool					m_use;

	vector<EGLSurface>		m_pbuffers;
	vector<EGLContext>		m_contexts;
};

MemoryAllocator::MemoryAllocator (EglTestContext& eglTestCtx, EGLDisplay display, EGLConfig config, int seed, ObjectType types, int minWidth, int minHeight, int maxWidth, int maxHeight, bool use)
	: m_eglTestCtx	(eglTestCtx)
	, m_display		(display)
	, m_config		(config)

	, m_rnd			(seed)
	, m_failed		(false)

	, m_types		(types)
	, m_minWidth	(minWidth)
	, m_minHeight	(minHeight)
	, m_maxWidth	(maxWidth)
	, m_maxHeight	(maxHeight)
	, m_use			(use)
{
	m_eglTestCtx.initGLFunctions(&m_gl, glu::ApiType::es(2,0));
}

MemoryAllocator::~MemoryAllocator (void)
{
	const Library& egl = m_eglTestCtx.getLibrary();

	for (vector<EGLSurface>::const_iterator iter = m_pbuffers.begin(); iter != m_pbuffers.end(); ++iter)
		egl.destroySurface(m_display, *iter);

	m_pbuffers.clear();

	for (vector<EGLContext>::const_iterator iter = m_contexts.begin(); iter != m_contexts.end(); ++iter)
		egl.destroyContext(m_display, *iter);

	m_contexts.clear();
}

bool MemoryAllocator::allocateUntilFailure (void)
{
	const deUint64		timeLimitUs		= 10000000; // 10s
	deUint64			beginTimeUs		= deGetMicroseconds();
	vector<ObjectType>	types;

	if ((m_types & OBJECTTYPE_CONTEXT) != 0)
		types.push_back(OBJECTTYPE_CONTEXT);

	if ((m_types & OBJECTTYPE_PBUFFER) != 0)
		types.push_back(OBJECTTYPE_PBUFFER);

	// If objects should be used. Create one of both at beginning to allow using them.
	if (m_contexts.size() == 0 && m_pbuffers.size() == 0 && m_use)
	{
		allocateContext();
		allocatePBuffer();
	}

	while (!m_failed)
	{
		ObjectType type = m_rnd.choose<ObjectType>(types.begin(), types.end());

		switch (type)
		{
			case OBJECTTYPE_PBUFFER:
				allocatePBuffer();
				break;

			case OBJECTTYPE_CONTEXT:
				allocateContext();
				break;

			default:
				DE_ASSERT(false);
		}

		if (deGetMicroseconds() - beginTimeUs > timeLimitUs)
			return true;
	}

	return false;
}

void MemoryAllocator::allocatePBuffer (void)
{
	// Reserve space for new allocations
	try
	{
		m_pbuffers.reserve(m_pbuffers.size() + 1);
	}
	catch (const std::bad_alloc&)
	{
		m_errorString	= "std::bad_alloc when allocating more space for testcase. Out of host memory.";
		m_failed		= true;
		return;
	}

	// Allocate pbuffer
	try
	{
		const Library&	egl				= m_eglTestCtx.getLibrary();
		const EGLint	width			= m_rnd.getInt(m_minWidth, m_maxWidth);
		const EGLint	height			= m_rnd.getInt(m_minHeight, m_maxHeight);
		const EGLint	attribList[]	=
		{
			EGL_WIDTH,	width,
			EGL_HEIGHT, height,
			EGL_NONE
		};

		EGLSurface		surface			= egl.createPbufferSurface(m_display, m_config, attribList);
		EGLU_CHECK_MSG(egl, "eglCreatePbufferSurface");

		DE_ASSERT(surface != EGL_NO_SURFACE);

		m_pbuffers.push_back(surface);

		if (m_use && m_contexts.size() > 0)
		{
			EGLContext				context		= m_rnd.choose<EGLContext>(m_contexts.begin(), m_contexts.end());
			const float				red			= m_rnd.getFloat();
			const float				green		= m_rnd.getFloat();
			const float				blue		= m_rnd.getFloat();
			const float				alpha		= m_rnd.getFloat();

			EGLU_CHECK_CALL(egl, makeCurrent(m_display, surface, surface, context));

			m_gl.clearColor(red, green, blue, alpha);
			GLU_EXPECT_NO_ERROR(m_gl.getError(), "glClearColor()");

			m_gl.clear(GL_COLOR_BUFFER_BIT);
			GLU_EXPECT_NO_ERROR(m_gl.getError(), "glClear()");

			EGLU_CHECK_CALL(egl, makeCurrent(m_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT));
		}
	}
	catch (const eglu::Error& error)
	{
		if (error.getError() == EGL_BAD_ALLOC)
		{
			m_errorString	= "eglCreatePbufferSurface returned EGL_BAD_ALLOC";
			m_failed		= true;
			return;
		}
		else
			throw;
	}
}

void MemoryAllocator::allocateContext (void)
{
	// Reserve space for new allocations
	try
	{
		m_contexts.reserve(m_contexts.size() + 1);
	}
	catch (const std::bad_alloc&)
	{
		m_errorString	= "std::bad_alloc when allocating more space for testcase. Out of host memory.";
		m_failed		= true;
		return;
	}

	// Allocate context
	try
	{
		const Library&	egl				= m_eglTestCtx.getLibrary();
		const EGLint	attribList[]	=
		{
			EGL_CONTEXT_CLIENT_VERSION, 2,
			EGL_NONE
		};

		EGLU_CHECK_CALL(egl, bindAPI(EGL_OPENGL_ES_API));
		EGLContext context = egl.createContext(m_display, m_config, EGL_NO_CONTEXT, attribList);
		EGLU_CHECK_MSG(egl, "eglCreateContext");

		DE_ASSERT(context != EGL_NO_CONTEXT);

		m_contexts.push_back(context);

		if (m_use && m_pbuffers.size() > 0)
		{
			EGLSurface				surface		= m_rnd.choose<EGLSurface>(m_pbuffers.begin(), m_pbuffers.end());
			const float				red			= m_rnd.getFloat();
			const float				green		= m_rnd.getFloat();
			const float				blue		= m_rnd.getFloat();
			const float				alpha		= m_rnd.getFloat();

			EGLU_CHECK_CALL(egl, makeCurrent(m_display, surface, surface, context));

			m_gl.clearColor(red, green, blue, alpha);
			GLU_EXPECT_NO_ERROR(m_gl.getError(), "glClearColor()");

			m_gl.clear(GL_COLOR_BUFFER_BIT);
			GLU_EXPECT_NO_ERROR(m_gl.getError(), "glClear()");

			EGLU_CHECK_CALL(egl, makeCurrent(m_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT));
		}
	}
	catch (const eglu::Error& error)
	{
		if (error.getError() == EGL_BAD_ALLOC)
		{
			m_errorString	= "eglCreateContext returned EGL_BAD_ALLOC";
			m_failed		= true;
			return;
		}
		else
			throw;
	}
}

} // anonymous

class MemoryStressCase : public TestCase
{
public:
	struct Spec
	{
		ObjectType	types;
		int			minWidth;
		int			minHeight;
		int			maxWidth;
		int			maxHeight;
		bool		use;
	};

					MemoryStressCase	(EglTestContext& eglTestCtx, Spec spec, const char* name, const char* description);
	void			init				(void);
	void			deinit				(void);
	IterateResult	iterate				(void);

private:
	Spec				m_spec;
	vector<int>			m_allocationCounts;
	MemoryAllocator*	m_allocator;

	int					m_iteration;
	int					m_iterationCount;
	int					m_seed;
	EGLDisplay			m_display;
	EGLConfig			m_config;
};

MemoryStressCase::MemoryStressCase (EglTestContext& eglTestCtx, Spec spec, const char* name, const char* description)
	: TestCase			(eglTestCtx, name, description)
	, m_spec			(spec)
	, m_allocator		(NULL)
	, m_iteration		(0)
	, m_iterationCount	(10)
	, m_seed			(deStringHash(name))
	, m_display			(EGL_NO_DISPLAY)
	, m_config			(DE_NULL)
{
}

void MemoryStressCase::init (void)
{
	const Library&	egl				= m_eglTestCtx.getLibrary();
	EGLint			configCount		= 0;
	const EGLint	attribList[]	=
	{
		EGL_SURFACE_TYPE,		EGL_PBUFFER_BIT,
		EGL_RENDERABLE_TYPE,	EGL_OPENGL_ES2_BIT,
		EGL_NONE
	};

	if (!m_testCtx.getCommandLine().isOutOfMemoryTestEnabled())
	{
		m_testCtx.getLog() << TestLog::Message << "Tests that exhaust memory are disabled, use --deqp-test-oom=enable command line option to enable." << TestLog::EndMessage;
		throw tcu::NotSupportedError("OOM tests disabled");
	}

	m_display = eglu::getAndInitDisplay(m_eglTestCtx.getNativeDisplay());

	EGLU_CHECK_CALL(egl, chooseConfig(m_display, attribList, &m_config, 1, &configCount));

	TCU_CHECK(configCount != 0);
}

void MemoryStressCase::deinit (void)
{
	delete m_allocator;
	m_allocator = DE_NULL;

	if (m_display != EGL_NO_DISPLAY)
	{
		m_eglTestCtx.getLibrary().terminate(m_display);
		m_display = EGL_NO_DISPLAY;
	}
}

TestCase::IterateResult MemoryStressCase::iterate (void)
{
	TestLog& log = m_testCtx.getLog();

	if (m_iteration < m_iterationCount)
	{
		try
		{
			if (!m_allocator)
				m_allocator = new MemoryAllocator(m_eglTestCtx, m_display, m_config, m_seed, m_spec.types, m_spec.minWidth, m_spec.minHeight, m_spec.maxWidth, m_spec.maxHeight, m_spec.use);

			if (m_allocator->allocateUntilFailure())
			{
				log << TestLog::Message << "Couldn't exhaust memory before timeout. Allocated " << m_allocator->getAllocationCount() << " objects." << TestLog::EndMessage;
				m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

				delete m_allocator;
				m_allocator = NULL;

				return STOP;
			}

			log << TestLog::Message << "Iteration " << m_iteration << ": Allocated " << m_allocator->getAllocationCount() << " objects; " << m_allocator->getContextCount() << " contexts, " << m_allocator->getPBufferCount() << " PBuffers." << TestLog::EndMessage;
			log << TestLog::Message << "Got expected error: " << m_allocator->getErrorString() << TestLog::EndMessage;
			m_allocationCounts.push_back(m_allocator->getAllocationCount());

			delete m_allocator;
			m_allocator = NULL;

			m_iteration++;

			return CONTINUE;
		} catch (...)
		{
			log << TestLog::Message << "Iteration " << m_iteration << ": Allocated " << m_allocator->getAllocationCount() << " objects; " << m_allocator->getContextCount() << " contexts, " << m_allocator->getPBufferCount() << " PBuffers." << TestLog::EndMessage;
			log << TestLog::Message << "Unexpected error" << TestLog::EndMessage;
			throw;
		}
	}
	else
	{
		// Analyze number of passed allocations.
		int min = m_allocationCounts[0];
		int max = m_allocationCounts[0];

		float threshold = 50.0f;

		for (int allocNdx = 0; allocNdx < (int)m_allocationCounts.size(); allocNdx++)
		{
			min = deMin32(m_allocationCounts[allocNdx], min);
			max = deMax32(m_allocationCounts[allocNdx], max);
		}

		if (min == 0 && max != 0)
		{
			log << TestLog::Message << "Allocation count zero" << TestLog::EndMessage;
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
		}
		else
		{
			float change = (float)(min - max) / ((float)(max));

			if (change > threshold)
			{
				log << TestLog::Message << "Allocated objects max: " << max << ", min: " << min << ", difference: " << change << "% threshold: " << threshold << "%" << TestLog::EndMessage;
				m_testCtx.setTestResult(QP_TEST_RESULT_QUALITY_WARNING, "Allocation count variation");
			}
			else
				m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
		}

		return STOP;
	}
}

MemoryStressTests::MemoryStressTests (EglTestContext& eglTestCtx)
	: TestCaseGroup(eglTestCtx, "memory", "Memory allocation stress tests")
{
}

void MemoryStressTests::init (void)
{
	// Check small pbuffers 256x256
	{
		MemoryStressCase::Spec spec;

		spec.types		= OBJECTTYPE_PBUFFER;
		spec.minWidth	= 256;
		spec.minHeight	= 256;
		spec.maxWidth	= 256;
		spec.maxHeight	= 256;
		spec.use		= false;

		addChild(new MemoryStressCase(m_eglTestCtx, spec, "pbuffer_256x256", "PBuffer allocation stress tests"));
	}

	// Check small pbuffers 256x256 and use them
	{
		MemoryStressCase::Spec spec;

		spec.types		= OBJECTTYPE_PBUFFER;
		spec.minWidth	= 256;
		spec.minHeight	= 256;
		spec.maxWidth	= 256;
		spec.maxHeight	= 256;
		spec.use		= true;

		addChild(new MemoryStressCase(m_eglTestCtx, spec, "pbuffer_256x256_use", "PBuffer allocation stress tests"));
	}

	// Check big pbuffers 1024x1024
	{
		MemoryStressCase::Spec spec;

		spec.types		= OBJECTTYPE_PBUFFER;
		spec.minWidth	= 1024;
		spec.minHeight	= 1024;
		spec.maxWidth	= 1024;
		spec.maxHeight	= 1024;
		spec.use		= false;

		addChild(new MemoryStressCase(m_eglTestCtx, spec, "pbuffer_1024x1024", "PBuffer allocation stress tests"));
	}

	// Check big pbuffers 1024x1024 and use them
	{
		MemoryStressCase::Spec spec;

		spec.types		= OBJECTTYPE_PBUFFER;
		spec.minWidth	= 1024;
		spec.minHeight	= 1024;
		spec.maxWidth	= 1024;
		spec.maxHeight	= 1024;
		spec.use		= true;

		addChild(new MemoryStressCase(m_eglTestCtx, spec, "pbuffer_1024x1024_use", "PBuffer allocation stress tests"));
	}

	// Check different sized pbuffers
	{
		MemoryStressCase::Spec spec;

		spec.types		= OBJECTTYPE_PBUFFER;
		spec.minWidth	= 64;
		spec.minHeight	= 64;
		spec.maxWidth	= 1024;
		spec.maxHeight	= 1024;
		spec.use		= false;

		addChild(new MemoryStressCase(m_eglTestCtx, spec, "pbuffer", "PBuffer allocation stress tests"));
	}

	// Check different sized pbuffers and use them
	{
		MemoryStressCase::Spec spec;

		spec.types		= OBJECTTYPE_PBUFFER;
		spec.minWidth	= 64;
		spec.minHeight	= 64;
		spec.maxWidth	= 1024;
		spec.maxHeight	= 1024;
		spec.use		= true;

		addChild(new MemoryStressCase(m_eglTestCtx, spec, "pbuffer_use", "PBuffer allocation stress tests"));
	}

	// Check contexts
	{
		MemoryStressCase::Spec spec;

		spec.types		= OBJECTTYPE_CONTEXT;
		spec.minWidth	= 1024;
		spec.minHeight	= 1024;
		spec.maxWidth	= 1024;
		spec.maxHeight	= 1024;
		spec.use		= false;

		addChild(new MemoryStressCase(m_eglTestCtx, spec, "context", "Context allocation stress tests"));
	}

	// Check contexts and use them
	{
		MemoryStressCase::Spec spec;

		spec.types		= OBJECTTYPE_CONTEXT;
		spec.minWidth	= 1024;
		spec.minHeight	= 1024;
		spec.maxWidth	= 1024;
		spec.maxHeight	= 1024;
		spec.use		= true;

		addChild(new MemoryStressCase(m_eglTestCtx, spec, "context_use", "Context allocation stress tests"));
	}

	// Check contexts and pbuffers
	{
		MemoryStressCase::Spec spec;

		spec.types		= (ObjectType)(OBJECTTYPE_PBUFFER|OBJECTTYPE_CONTEXT);
		spec.minWidth	= 64;
		spec.minHeight	= 64;
		spec.maxWidth	= 1024;
		spec.maxHeight	= 1024;
		spec.use		= false;

		addChild(new MemoryStressCase(m_eglTestCtx, spec, "pbuffer_context", "PBuffer and context allocation stress tests"));
	}

	// Check contexts and pbuffers and use
	{
		MemoryStressCase::Spec spec;

		spec.types		= (ObjectType)(OBJECTTYPE_PBUFFER|OBJECTTYPE_CONTEXT);
		spec.minWidth	= 64;
		spec.minHeight	= 64;
		spec.maxWidth	= 1024;
		spec.maxHeight	= 1024;
		spec.use		= true;

		addChild(new MemoryStressCase(m_eglTestCtx, spec, "pbuffer_context_use", "PBuffer and context allocation stress tests"));
	}
}

} // egl
} // deqp
