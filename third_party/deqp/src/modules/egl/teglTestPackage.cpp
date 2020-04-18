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
 * \brief EGL Test Package
 *//*--------------------------------------------------------------------*/

#include "teglTestPackage.hpp"

#include "tcuTestLog.hpp"
#include "tcuPlatform.hpp"
#include "tcuCommandLine.hpp"

#include "egluPlatform.hpp"
#include "egluUtil.hpp"

#include "teglInfoTests.hpp"
#include "teglCreateContextTests.hpp"
#include "teglQueryContextTests.hpp"
#include "teglCreateSurfaceTests.hpp"
#include "teglQuerySurfaceTests.hpp"
#include "teglChooseConfigTests.hpp"
#include "teglQueryConfigTests.hpp"
#include "teglColorClearTests.hpp"
#include "teglRenderTests.hpp"
#include "teglImageTests.hpp"
#include "teglGLES2SharingTests.hpp"
#include "teglNegativeApiTests.hpp"
#include "teglSyncTests.hpp"
#include "teglMultiThreadTests.hpp"
#include "teglGetProcAddressTests.hpp"
#include "teglMemoryStressTests.hpp"
#include "teglMakeCurrentPerfTests.hpp"
#include "teglGLES2SharedRenderingPerfTests.hpp"
#include "teglPreservingSwapTests.hpp"
#include "teglClientExtensionTests.hpp"
#include "teglCreateContextExtTests.hpp"
#include "teglSurfacelessContextTests.hpp"
#include "teglSwapBuffersTests.hpp"
#include "teglNativeColorMappingTests.hpp"
#include "teglNativeCoordMappingTests.hpp"
#include "teglResizeTests.hpp"
#include "teglBufferAgeTests.hpp"
#include "teglPartialUpdateTests.hpp"
#include "teglNegativePartialUpdateTests.hpp"
#include "teglSwapBuffersWithDamageTests.hpp"
#include "teglMultiContextTests.hpp"
#include "teglThreadCleanUpTests.hpp"
#include "teglMutableRenderBufferTests.hpp"
#include "teglGetFrameTimestampsTests.hpp"
#include "teglRobustnessTests.hpp"
#include "teglWideColorTests.hpp"

namespace deqp
{
namespace egl
{

class StressTests : public TestCaseGroup
{
public:
	StressTests (EglTestContext& eglTestCtx)
		: TestCaseGroup(eglTestCtx, "stress", "EGL stress tests")
	{
	}

	void init (void)
	{
		addChild(new MemoryStressTests(m_eglTestCtx));
	}
};

class PerformanceTests : public TestCaseGroup
{
public:
	PerformanceTests (EglTestContext& eglTestCtx)
		: TestCaseGroup(eglTestCtx, "performance", "EGL performance tests")
	{
	}

	void init (void)
	{
		addChild(new MakeCurrentPerfTests			(m_eglTestCtx));
		addChild(new GLES2SharedRenderingPerfTests	(m_eglTestCtx));
	}
};

class FunctionalTests : public TestCaseGroup
{
public:
	FunctionalTests (EglTestContext& eglTestCtx)
		: TestCaseGroup(eglTestCtx, "functional", "EGL functional tests")
	{
	}

	void init (void)
	{
		addChild(new CreateContextTests			(m_eglTestCtx));
		addChild(new QueryContextTests			(m_eglTestCtx));
		addChild(new CreateSurfaceTests			(m_eglTestCtx));
		addChild(new QuerySurfaceTests			(m_eglTestCtx));
		addChild(new QueryConfigTests			(m_eglTestCtx));
		addChild(new ChooseConfigTests			(m_eglTestCtx));
		addChild(new ColorClearTests			(m_eglTestCtx));
		addChild(new RenderTests				(m_eglTestCtx));
		addChild(new ImageTests					(m_eglTestCtx));
		addChild(new SharingTests				(m_eglTestCtx));
		addChild(new NegativeApiTests			(m_eglTestCtx));
		addChild(new FenceSyncTests				(m_eglTestCtx));
		addChild(new MultiThreadedTests			(m_eglTestCtx));
		addChild(new GetProcAddressTests		(m_eglTestCtx));
		addChild(new PreservingSwapTests		(m_eglTestCtx));
		addChild(new ClientExtensionTests		(m_eglTestCtx));
		addChild(new CreateContextExtTests		(m_eglTestCtx));
		addChild(new SurfacelessContextTests	(m_eglTestCtx));
		addChild(new SwapBuffersTests			(m_eglTestCtx));
		addChild(new NativeColorMappingTests	(m_eglTestCtx));
		addChild(new NativeCoordMappingTests	(m_eglTestCtx));
		addChild(new ReusableSyncTests			(m_eglTestCtx));
		addChild(new ResizeTests				(m_eglTestCtx));
		addChild(new BufferAgeTests				(m_eglTestCtx));
		addChild(new PartialUpdateTests			(m_eglTestCtx));
		addChild(new NegativePartialUpdateTests	(m_eglTestCtx));
		addChild(new SwapBuffersWithDamageTests	(m_eglTestCtx));
		addChild(createMultiContextTests		(m_eglTestCtx));
		addChild(createThreadCleanUpTest		(m_eglTestCtx));
		addChild(new MutableRenderBufferTests	(m_eglTestCtx));
		addChild(createGetFrameTimestampsTests	(m_eglTestCtx));
		addChild(createRobustnessTests			(m_eglTestCtx));
		addChild(createWideColorTests			(m_eglTestCtx));
	}
};

class TestCaseWrapper : public tcu::TestCaseExecutor
{
public:
	TestCaseWrapper (void)
	{
	}

	~TestCaseWrapper (void)
	{
	}

	void init (tcu::TestCase* testCase, const std::string&)
	{
		testCase->init();
	}

	void deinit (tcu::TestCase* testCase)
	{
		testCase->deinit();
	}

	tcu::TestNode::IterateResult iterate (tcu::TestCase* testCase)
	{
		return testCase->iterate();
	}
};

static const eglu::NativeDisplayFactory& getDefaultDisplayFactory (tcu::TestContext& testCtx)
{
	const eglu::NativeDisplayFactory& factory = eglu::selectNativeDisplayFactory(testCtx.getPlatform().getEGLPlatform().getNativeDisplayFactoryRegistry(), testCtx.getCommandLine());

	return factory;
}

TestPackage::TestPackage (tcu::TestContext& testCtx)
	: tcu::TestPackage	(testCtx, "dEQP-EGL", "dEQP EGL Tests")
	, m_eglTestCtx		(DE_NULL)
{
}

TestPackage::~TestPackage (void)
{
	// Destroy children first since destructors may access context.
	TestNode::deinit();
	delete m_eglTestCtx;
}

void TestPackage::init (void)
{
	DE_ASSERT(!m_eglTestCtx);
	m_eglTestCtx = new EglTestContext(m_testCtx, getDefaultDisplayFactory(m_testCtx));

	try
	{
		addChild(new InfoTests			(*m_eglTestCtx));
		addChild(new FunctionalTests	(*m_eglTestCtx));
		addChild(new PerformanceTests	(*m_eglTestCtx));
		addChild(new StressTests		(*m_eglTestCtx));
	}
	catch (...)
	{
		delete m_eglTestCtx;
		m_eglTestCtx = DE_NULL;

		throw;
	}
}

void TestPackage::deinit (void)
{
	tcu::TestNode::deinit();
	delete m_eglTestCtx;
	m_eglTestCtx = DE_NULL;
}

tcu::TestCaseExecutor* TestPackage::createExecutor (void) const
{
	return new TestCaseWrapper();
}

} // egl
} // deqp
