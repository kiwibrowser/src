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
 * \brief EGL Test Case
 *//*--------------------------------------------------------------------*/

#include "teglTestCase.hpp"

#include "tcuPlatform.hpp"

#include "egluUtil.hpp"
#include "egluGLFunctionLoader.hpp"
#include "egluPlatform.hpp"

#include "eglwLibrary.hpp"
#include "eglwEnums.hpp"

#include "gluRenderContext.hpp"
#include "glwInitFunctions.hpp"

namespace deqp
{
namespace egl
{

using namespace eglw;

EglTestContext::EglTestContext (tcu::TestContext& testCtx, const eglu::NativeDisplayFactory& displayFactory)
	: m_testCtx					(testCtx)
	, m_nativeDisplayFactory	(displayFactory)
	, m_nativeDisplay			(m_nativeDisplayFactory.createDisplay())
	, m_glLibraryCache			(testCtx.getPlatform().getEGLPlatform(), testCtx.getCommandLine())
{
}

EglTestContext::~EglTestContext (void)
{
}

const eglw::Library& EglTestContext::getLibrary (void) const
{
	return m_nativeDisplay->getLibrary();
}

void EglTestContext::initGLFunctions (glw::Functions* dst, glu::ApiType apiType) const
{
	initGLFunctions(dst, apiType, 0, DE_NULL);
}

void EglTestContext::initGLFunctions (glw::Functions* dst, glu::ApiType apiType, int numExtensions, const char* const* extensions) const
{
	const tcu::FunctionLibrary*		platformLib		= m_glLibraryCache.getLibrary(apiType);
	const eglu::GLFunctionLoader	loader			(getLibrary(), platformLib);

	glu::initCoreFunctions(dst, &loader, apiType);
	glu::initExtensionFunctions(dst, &loader, apiType, numExtensions, extensions);
}

TestCaseGroup::TestCaseGroup (EglTestContext& eglTestCtx, const char* name, const char* description)
	: tcu::TestCaseGroup	(eglTestCtx.getTestContext(), name, description)
	, m_eglTestCtx			(eglTestCtx)
{
}

TestCaseGroup::~TestCaseGroup (void)
{
}

TestCase::TestCase (EglTestContext& eglTestCtx, const char* name, const char* description)
	: tcu::TestCase		(eglTestCtx.getTestContext(), name, description)
	, m_eglTestCtx		(eglTestCtx)
{
}

TestCase::TestCase (EglTestContext& eglTestCtx, tcu::TestNodeType type,  const char* name, const char* description)
	: tcu::TestCase		(eglTestCtx.getTestContext(), type, name, description)
	, m_eglTestCtx		(eglTestCtx)
{
}

TestCase::~TestCase (void)
{
}

} // egl
} // deqp
