#ifndef _TEGLTESTCASE_HPP
#define _TEGLTESTCASE_HPP
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

#include "tcuDefs.hpp"
#include "tcuTestCase.hpp"
#include "tcuFunctionLibrary.hpp"

#include "egluNativeDisplay.hpp"
#include "egluGLFunctionLoader.hpp"
#include "egluConfigInfo.hpp"

#include "eglwDefs.hpp"

#include "gluRenderContext.hpp"

#include "deUniquePtr.hpp"

namespace eglu
{
class NativeDisplay;
class NativeWindow;
class NativePixmap;
class NativeDisplayFactory;
class NativeWindowFactory;
class NativePixmapFactory;
}

namespace eglw
{
class Library;
}

namespace deqp
{
namespace egl
{

class EglTestContext
{
public:
										EglTestContext			(tcu::TestContext& testCtx, const eglu::NativeDisplayFactory& displayFactory);
										~EglTestContext			(void);

	tcu::TestContext&					getTestContext			(void) const { return m_testCtx;				}
	const eglu::NativeDisplayFactory&	getNativeDisplayFactory	(void) const { return m_nativeDisplayFactory;	}
	eglu::NativeDisplay&				getNativeDisplay		(void) const { return *m_nativeDisplay;			}
	const eglw::Library&				getLibrary				(void) const;

	void								initGLFunctions			(glw::Functions* dst, glu::ApiType apiType) const;
	void								initGLFunctions			(glw::Functions* dst, glu::ApiType apiType, int numExtensions, const char* const* extensions) const;

private:
										EglTestContext			(const EglTestContext&);
	EglTestContext&						operator=				(const EglTestContext&);

	tcu::TestContext&					m_testCtx;
	const eglu::NativeDisplayFactory&	m_nativeDisplayFactory;
	de::UniquePtr<eglu::NativeDisplay>	m_nativeDisplay;
	mutable eglu::GLLibraryCache		m_glLibraryCache;
};

class TestCaseGroup : public tcu::TestCaseGroup
{
public:
						TestCaseGroup	(EglTestContext& eglTestCtx, const char* name, const char* description);
	virtual				~TestCaseGroup	(void);

protected:
	EglTestContext&		m_eglTestCtx;
};

class TestCase : public tcu::TestCase
{
public:
						TestCase		(EglTestContext& eglTestCtx, const char* name, const char* description);
						TestCase		(EglTestContext& eglTestCtx, tcu::TestNodeType type, const char* name, const char* description);
	virtual				~TestCase		(void);

protected:
	EglTestContext&		m_eglTestCtx;
};

} // egl
} // deqp

#endif // _TEGLTESTCASE_HPP
