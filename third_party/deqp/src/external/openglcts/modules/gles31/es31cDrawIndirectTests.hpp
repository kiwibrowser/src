#ifndef _ES31CDRAWINDIRECTTESTS_HPP
#define _ES31CDRAWINDIRECTTESTS_HPP
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

class DrawIndirectTestsGL40 : public glcts::TestCaseGroup
{
public:
	DrawIndirectTestsGL40(glcts::Context& context);
	~DrawIndirectTestsGL40(void);

	void init(void);

private:
	DrawIndirectTestsGL40(const DrawIndirectTestsGL40& other);
	DrawIndirectTestsGL40& operator=(const DrawIndirectTestsGL40& other);
};

class DrawIndirectTestsGL43 : public glcts::TestCaseGroup
{
public:
	DrawIndirectTestsGL43(glcts::Context& context);
	~DrawIndirectTestsGL43(void);

	void init(void);

private:
	DrawIndirectTestsGL43(const DrawIndirectTestsGL43& other);
	DrawIndirectTestsGL43& operator=(const DrawIndirectTestsGL43& other);
};

class DrawIndirectTestsES31 : public glcts::TestCaseGroup
{
public:
	DrawIndirectTestsES31(glcts::Context& context);
	~DrawIndirectTestsES31(void);

	void init(void);

private:
	DrawIndirectTestsES31(const DrawIndirectTestsES31& other);
	DrawIndirectTestsES31& operator=(const DrawIndirectTestsES31& other);
};

} // gl4cts

#endif // _ES31CDRAWINDIRECTTESTS_HPP
