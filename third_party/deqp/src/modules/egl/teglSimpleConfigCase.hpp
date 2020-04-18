#ifndef _TEGLSIMPLECONFIGCASE_HPP
#define _TEGLSIMPLECONFIGCASE_HPP
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
 * \brief Simple Context construction test.
 *//*--------------------------------------------------------------------*/

#include "tcuDefs.hpp"
#include "teglTestCase.hpp"
#include "egluConfigFilter.hpp"

#include <vector>
#include <string>

namespace deqp
{
namespace egl
{

class SimpleConfigCase : public TestCase
{
public:
							SimpleConfigCase	(EglTestContext& eglTestCtx, const char* name, const char* description, const eglu::FilterList& filters);
	virtual					~SimpleConfigCase	(void);

	void					init				(void);
	void					deinit				(void);
	IterateResult			iterate				(void);

protected:
	eglw::EGLDisplay		getDisplay			(void) { return m_display; }

private:
	virtual void			executeForConfig	(eglw::EGLDisplay display, eglw::EGLConfig config)	= DE_NULL;

							SimpleConfigCase	(const SimpleConfigCase& other);
	SimpleConfigCase&		operator=			(const SimpleConfigCase& other);

	const eglu::FilterList						m_filters;

	eglw::EGLDisplay							m_display;
	std::vector<eglw::EGLConfig>				m_configs;
	std::vector<eglw::EGLConfig>::iterator		m_configIter;
};

class NamedFilterList : public eglu::FilterList
{
public:
					NamedFilterList		(const char* name, const char* description) : m_name(name), m_description(description) {}

	const char*		getName				(void) const	{ return m_name.c_str();		}
	const char*		getDescription		(void) const	{ return m_description.c_str();	}

private:
	std::string		m_name;
	std::string		m_description;
};

void getDefaultFilterLists (std::vector<NamedFilterList>& lists, const eglu::FilterList& baseFilters);

} // egl
} // deqp

#endif // _TEGLSIMPLECONFIGCASE_HPP
