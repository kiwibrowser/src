#ifndef _TEGLAPICASE_HPP
#define _TEGLAPICASE_HPP
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
 * \brief API test case.
 *//*--------------------------------------------------------------------*/

#include "tcuDefs.hpp"
#include "teglTestCase.hpp"
#include "egluCallLogWrapper.hpp"
#include "egluConfigFilter.hpp"
#include "eglwEnums.hpp"

#include <vector>

namespace deqp
{
namespace egl
{

class ApiCase : public TestCase, protected eglu::CallLogWrapper
{
public:
						ApiCase					(EglTestContext& eglTestCtx, const char* name, const char* description);
	virtual				~ApiCase				(void);

	void				init					(void);
	void				deinit					(void);

	IterateResult		iterate					(void);

protected:
	virtual void		test					(void) = DE_NULL;

	void				expectError				(eglw::EGLenum error);
	void				expectEitherError		(eglw::EGLenum errorA, eglw::EGLenum errorB);
	void				expectBoolean			(eglw::EGLBoolean expected, eglw::EGLBoolean got);

	void				expectNoContext			(eglw::EGLContext got);
	void				expectNoSurface			(eglw::EGLSurface got);
	void				expectNoDisplay			(eglw::EGLDisplay got);
	void				expectNull				(const void* got);

	inline void			expectTrue				(eglw::EGLBoolean got) { expectBoolean(EGL_TRUE, got); }
	inline void			expectFalse				(eglw::EGLBoolean got) { expectBoolean(EGL_FALSE, got); }

	eglw::EGLDisplay	getDisplay				(void)						{ return m_display;							}
	bool				isAPISupported			(eglw::EGLenum api) const;
	bool				getConfig				(eglw::EGLConfig* cfg, const eglu::FilterList& filters);

private:
	eglw::EGLDisplay							m_display;
	std::vector<eglw::EGLenum>					m_supportedClientAPIs;
};

} // egl
} // deqp

// Helper macro for declaring ApiCases.
#define TEGL_ADD_API_CASE(NAME, DESCRIPTION, TEST_FUNC_BODY)									\
	do {																						\
		class ApiCase_##NAME : public deqp::egl::ApiCase {										\
		public:																					\
			ApiCase_##NAME (EglTestContext& context) : ApiCase(context, #NAME, DESCRIPTION) {}	\
		protected:																				\
			void test (void) TEST_FUNC_BODY	 /* NOLINT(TEST_FUNC_BODY) */						\
		};																						\
		addChild(new ApiCase_##NAME(m_eglTestCtx));												\
	} while (deGetFalse())

#endif // _TEGLAPICASE_HPP
