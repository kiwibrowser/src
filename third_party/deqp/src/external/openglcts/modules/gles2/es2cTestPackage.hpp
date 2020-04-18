#ifndef _ES2CTESTPACKAGE_HPP
#define _ES2CTESTPACKAGE_HPP
/*-------------------------------------------------------------------------
 * OpenGL Conformance Test Suite
 * -----------------------------
 *
 * Copyright (c) 2016 Google Inc.
 * Copyright (c) 2016 The Khronos Group Inc.
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
 * \brief OpenGL ES 2 Test Package.
 */ /*-------------------------------------------------------------------*/

#ifndef _TCUDEFS_HPP
#include "tcuDefs.hpp"
#endif
#ifndef _GLCTESTPACKAGE_HPP
#include "glcTestPackage.hpp"
#endif

namespace es2cts
{

class TestPackage : public deqp::TestPackage
{
public:
	TestPackage(tcu::TestContext& testCtx, const char* packageName);
	~TestPackage(void);

	void init(void);

	virtual tcu::TestCaseExecutor* createExecutor(void) const;

private:
	TestPackage(const TestPackage& other);
	TestPackage& operator=(const TestPackage& other);
};

} // es2cts

#endif // _ES2CTESTPACKAGE_HPP
