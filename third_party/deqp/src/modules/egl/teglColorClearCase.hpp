#ifndef _TEGLCOLORCLEARCASE_HPP
#define _TEGLCOLORCLEARCASE_HPP
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
 * \brief Color clear case.
 *//*--------------------------------------------------------------------*/

#include "tcuDefs.hpp"
#include "teglTestCase.hpp"
#include "teglRenderCase.hpp"

#include <vector>

namespace deqp
{
namespace egl
{

class SingleThreadColorClearCase : public MultiContextRenderCase
{
public:
					SingleThreadColorClearCase	(EglTestContext&					eglTestCtx,
												 const char*						name,
												 const char*						description,
												 eglw::EGLint						api,
												 eglw::EGLint						surfaceType,
												 const eglu::FilterList&			filters,
												 int								numContextsPerApi);

private:
	virtual void	executeForContexts			(eglw::EGLDisplay												display,
												 eglw::EGLSurface												surface,
												 const Config&													config,
												 const std::vector<std::pair<eglw::EGLint, eglw::EGLContext> >&	contexts);
};

class MultiThreadColorClearCase : public MultiContextRenderCase
{
public:
					MultiThreadColorClearCase	(EglTestContext&					eglTestCtx,
												 const char*						name,
												 const char*						description,
												 eglw::EGLint						api,
												 eglw::EGLint						surfaceType,
												 const eglu::FilterList&			filters,
												 int								numContextsPerApi);

private:
	virtual void	executeForContexts			(eglw::EGLDisplay												display,
												 eglw::EGLSurface												surface,
												 const Config&													config,
												 const std::vector<std::pair<eglw::EGLint, eglw::EGLContext> >&	contexts);
};

} // egl
} // deqp

#endif // _TEGLCOLORCLEARCASE_HPP
