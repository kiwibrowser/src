#ifndef _GL4CLIMITSTESTS_HPP
#define _GL4CLIMITSTESTS_HPP
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
 * \file  gl4cLimitsTests.hpp
 * \brief Declares test classes for testing all limits and builtins.
 */ /*-------------------------------------------------------------------*/

#include "glcTestCase.hpp"

namespace gl4cts
{

class LimitsTests : public deqp::TestCaseGroup
{
public:
	/* Public methods */
	LimitsTests(deqp::Context& context);
	virtual ~LimitsTests()
	{
	}

	virtual void init(void);

private:
	/* Private methods */
	LimitsTests(const LimitsTests&);
	LimitsTests& operator=(const LimitsTests&);
};

} /* gl4cts namespace */

#endif // _GL4CLIMITSTESTS_HPP
