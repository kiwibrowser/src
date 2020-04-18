#ifndef _ESEXTCDRAWBUFFERSINDEXEDTESTS_HPP
#define _ESEXTCDRAWBUFFERSINDEXEDTESTS_HPP
/*-------------------------------------------------------------------------
 * OpenGL Conformance Test Suite
 * -----------------------------
 *
 * Copyright (c) 2015-2016 The Khronos Group Inc.
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
 * \file
 * \brief
 */ /*-------------------------------------------------------------------*/

/*!
 * \file esextcDrawBuffersIndexedTests.hpp
 * \brief Test group for Draw Buffers Indexed tests
 */ /*-------------------------------------------------------------------*/

#include "../esextcTestCaseBase.hpp"
#include "glcTestCase.hpp"
#include "gluShaderUtil.hpp"
#include "tcuDefs.hpp"

namespace glcts
{
/* Base Test Group for Draw Buffers Indexed tests */
class DrawBuffersIndexedTests : public TestCaseGroupBase
{
public:
	/* Public methods */
	DrawBuffersIndexedTests(glcts::Context& context, const ExtParameters& extParams);
	virtual ~DrawBuffersIndexedTests(void)
	{
	}

	virtual void init(void);

private:
	/* Private methods */
	DrawBuffersIndexedTests(const DrawBuffersIndexedTests& other);
	DrawBuffersIndexedTests& operator=(const DrawBuffersIndexedTests& other);
};

} // namespace glcts

#endif // _ESEXTCDRAWBUFFERSINDEXEDTESTS_HPP
