#ifndef _GLCEXPOSEDEXTENSIONSTESTS_HPP
#define _GLCEXPOSEDEXTENSIONSTESTS_HPP
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
 * \file  glcExtensionsExposeTests.cpp
 * \brief Test that check if specified substring is not in extension name.
 */ /*-------------------------------------------------------------------*/

#include "glcTestCase.hpp"
#include "glwDefs.hpp"
#include "tcuDefs.hpp"

namespace glcts
{
/** Test verifies if extensions with specified phrase are not exposed.
	 */

class ExposedExtensionsTests : public deqp::TestCaseGroup
{
public:
	/* Public methods */
	ExposedExtensionsTests(deqp::Context& context);
	virtual ~ExposedExtensionsTests()
	{
	}

	virtual void init(void);

private:
	/* Private methods */
	ExposedExtensionsTests(const ExposedExtensionsTests&);
	ExposedExtensionsTests& operator=(const ExposedExtensionsTests&);
};

} /* glcts namespace */

#endif // _GLCEXPOSEDEXTENSIONSTESTS_HPP
