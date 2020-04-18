#ifndef _GLCMULTIPLECONTEXTSTESTS_HPP
#define _GLCMULTIPLECONTEXTSTESTS_HPP
/*-------------------------------------------------------------------------
 * OpenGL Conformance Test Suite
 * -----------------------------
 *
 * Copyright (c) 2017 The Khronos Group Inc.
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
 */ /*!
 * \file  glcMultipleContextsTests.hpp
 * \brief
 */ /*-------------------------------------------------------------------*/

#include "glcTestCase.hpp"
#include "glwDefs.hpp"
#include <queue>

#include "tcuTestLog.hpp"

namespace glcts
{

/** Group class for Shader Subroutine conformance tests */
class MultipleContextsTests : public tcu::TestCaseGroup
{
public:
	/* Public methods */
	MultipleContextsTests(tcu::TestContext& testCtx, glu::ApiType apiType);
	virtual ~MultipleContextsTests()
	{
	}

	virtual void init(void);

private:
	/* Private methods */
	MultipleContextsTests(const MultipleContextsTests&);
	MultipleContextsTests& operator=(const MultipleContextsTests&);

	/* Private members */
	glu::ApiType m_apiType;
};

} /* gl4cts namespace */

#endif // _GLCMULTIPLECONTEXTSTESTS_HPP
