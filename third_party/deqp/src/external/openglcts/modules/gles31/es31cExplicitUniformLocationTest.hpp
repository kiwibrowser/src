#ifndef _ES31CEXPLICITUNIFORMLOCATIONTEST_HPP
#define _ES31CEXPLICITUNIFORMLOCATIONTEST_HPP
/*-------------------------------------------------------------------------
 * OpenGL Conformance Test Suite
 * -----------------------------
 *
 * Copyright (c) 2014-2016 The Khronos Group Inc.
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

#include "glcTestSubcase.hpp"
#include "tcuDefs.hpp"
#include "tes31TestCase.hpp"

namespace glcts
{

class ExplicitUniformLocationGLTests : public glcts::TestCaseGroup
{
public:
	ExplicitUniformLocationGLTests(glcts::Context& context);
	~ExplicitUniformLocationGLTests(void);

	void init(void);

private:
	ExplicitUniformLocationGLTests(const ExplicitUniformLocationGLTests& other);
	ExplicitUniformLocationGLTests& operator=(const ExplicitUniformLocationGLTests& other);
};

class ExplicitUniformLocationES31Tests : public glcts::TestCaseGroup
{
public:
	ExplicitUniformLocationES31Tests(glcts::Context& context);
	~ExplicitUniformLocationES31Tests(void);

	void init(void);

private:
	ExplicitUniformLocationES31Tests(const ExplicitUniformLocationES31Tests& other);
	ExplicitUniformLocationES31Tests& operator=(const ExplicitUniformLocationES31Tests& other);
};

} // glcts

#endif // _ES31CEXPLICITUNIFORMLOCATIONTEST_HPP
