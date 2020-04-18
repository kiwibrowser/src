#ifndef _TEGLRENDERCASE_HPP
#define _TEGLRENDERCASE_HPP
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
 * \brief Base class for rendering tests.
 *//*--------------------------------------------------------------------*/

#include "tcuDefs.hpp"
#include "teglTestCase.hpp"
#include "teglSimpleConfigCase.hpp"

#include <vector>

namespace deqp
{
namespace egl
{

class RenderCase : public SimpleConfigCase
{
public:
						RenderCase				(EglTestContext& eglTestCtx, const char* name, const char* description, eglw::EGLint surfaceTypeMask, const eglu::FilterList& filters);
	virtual				~RenderCase				(void);

protected:
	struct Config
	{
		eglw::EGLConfig		config;
		eglw::EGLint		surfaceTypeBit;
		eglw::EGLint		apiBits;

		Config (eglw::EGLConfig config_, eglw::EGLint surfaceTypeBit_, eglw::EGLint apiBits_)
			: config			(config_)
			, surfaceTypeBit	(surfaceTypeBit_)
			, apiBits			(apiBits_)
		{
		}
	};

	virtual void		executeForConfig		(eglw::EGLDisplay display, eglw::EGLConfig config);
	virtual void		executeForSurface		(eglw::EGLDisplay display, eglw::EGLSurface surface, const Config& config) = DE_NULL;

	eglw::EGLint		m_surfaceTypeMask;
};

class SingleContextRenderCase : public RenderCase
{
public:
					SingleContextRenderCase		(EglTestContext& eglTestCtx, const char* name, const char* description, eglw::EGLint apiMask, eglw::EGLint surfaceTypeMask, const eglu::FilterList& filters);
	virtual			~SingleContextRenderCase	(void);

protected:
	virtual void	executeForSurface			(eglw::EGLDisplay display, eglw::EGLSurface surface, const Config& config);
	virtual void	executeForContext			(eglw::EGLDisplay display, eglw::EGLContext context, eglw::EGLSurface surface, const Config& config) = DE_NULL;

	eglw::EGLint	m_apiMask;
};

class MultiContextRenderCase : public RenderCase
{
public:
					MultiContextRenderCase		(EglTestContext& eglTestCtx, const char* name, const char* description, eglw::EGLint api, eglw::EGLint surfaceType, const eglu::FilterList& filters, int numContextsPerApi);
	virtual			~MultiContextRenderCase		(void);

protected:
	virtual void	executeForSurface			(eglw::EGLDisplay display, eglw::EGLSurface surface, const Config& config);
	virtual void	executeForContexts			(eglw::EGLDisplay display, eglw::EGLSurface surface, const Config& config, const std::vector<std::pair<eglw::EGLint, eglw::EGLContext> >& contexts) = DE_NULL;

	int				m_numContextsPerApi;
	eglw::EGLint	m_apiMask;
};

class RenderFilterList : public NamedFilterList
{
public:
	RenderFilterList (const char* name, const char* description, eglw::EGLint surfaceTypeMask)
		: NamedFilterList	(name, description)
		, m_surfaceTypeMask	(surfaceTypeMask)
	{
	}

	eglw::EGLint getSurfaceTypeMask (void) const
	{
		return m_surfaceTypeMask;
	}

private:
	eglw::EGLint	m_surfaceTypeMask;
};

void			getDefaultRenderFilterLists	(std::vector<RenderFilterList>& configSets, const eglu::FilterList& baseFilters);

//! Client APIs supported in current build
eglw::EGLint	getBuildClientAPIMask		(void);

} // egl
} // deqp

#endif // _TEGLRENDERCASE_HPP
