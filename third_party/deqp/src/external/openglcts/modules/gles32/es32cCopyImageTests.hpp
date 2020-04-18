#ifndef _ES32CCOPYIMAGETESTS_HPP
#define _ES32CCOPYIMAGETESTS_HPP
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
 * \file es32cCopyImageTests.hpp
 * \brief CopyImageSubData functional tests.
 */ /*-------------------------------------------------------------------*/

#include "glcTestCase.hpp"
#include "glwDefs.hpp"

namespace glcts
{

class CopyImageTests : public deqp::TestCaseGroup
{
public:
	CopyImageTests(deqp::Context& context);
	~CopyImageTests(void);

	virtual void init(void);

private:
	CopyImageTests(const CopyImageTests& other);
	CopyImageTests& operator=(const CopyImageTests& other);
};

} /* namespace glcts */

#endif // _ES32CCOPYIMAGETESTS_HPP
