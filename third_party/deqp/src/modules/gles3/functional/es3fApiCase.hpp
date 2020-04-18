#ifndef _ES3FAPICASE_HPP
#define _ES3FAPICASE_HPP
/*-------------------------------------------------------------------------
 * drawElements Quality Program OpenGL ES 3.0 Module
 * -------------------------------------------------
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
#include "tes3TestCase.hpp"
#include "gluCallLogWrapper.hpp"
#include "tcuTestLog.hpp"

namespace deqp
{
namespace gles3
{
namespace Functional
{

class ApiCase : public TestCase, protected glu::CallLogWrapper
{
public:
						ApiCase					(Context& context, const char* name, const char* description);
	virtual				~ApiCase				(void);

	IterateResult		iterate					(void);

protected:
	virtual void		test					(void) = DE_NULL;

	void				expectError				(deUint32 error);
	void				expectError				(deUint32 error0, deUint32 error1);
	void				getSupportedExtensions	(const deUint32 numSupportedValues, const deUint32 extension, std::vector<int>& values);
	void				checkBooleans			(deUint8 value, deUint8 expected);
	void				checkBooleans			(deInt32 value, deUint8 expected);

	tcu::TestLog&		m_log;
};

// Helper macro for declaring ApiCases.
#define ES3F_ADD_API_CASE(NAME, DESCRIPTION, TEST_FUNC_BODY)							\
	do {																				\
		class ApiCase_##NAME : public ApiCase {											\
		public:																			\
			ApiCase_##NAME (Context& context) : ApiCase(context, #NAME, DESCRIPTION) {}	\
		protected:																		\
			void test (void) TEST_FUNC_BODY  /* NOLINT(TEST_FUNC_BODY) */				\
		};																				\
		addChild(new ApiCase_##NAME(m_context));										\
	} while (deGetFalse())

} // Functional
} // gles3
} // deqp

#endif // _ES3FAPICASE_HPP
