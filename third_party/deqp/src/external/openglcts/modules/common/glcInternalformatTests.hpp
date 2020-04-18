#ifndef _GLCINTERNALFORMATTESTS_HPP
#define _GLCINTERNALFORMATTESTS_HPP
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
 * \file  InternalformatTests.hpp
 * \brief
 */ /*-------------------------------------------------------------------*/

#include "glcTestCase.hpp"
#include "glwDefs.hpp"
#include "tcuDefs.hpp"

namespace glcts
{

struct TestData;

class InternalformatTests : public deqp::TestCaseGroup
{
public:
	/* Public methods */
	InternalformatTests(deqp::Context& context);

	void init(void);

protected:
	void getESTestData(TestData& testData, glu::ContextType& contextType);
	void getGLTestData(TestData& testData, glu::ContextType& contextType);

private:
	InternalformatTests(const InternalformatTests& other);
	InternalformatTests& operator=(const InternalformatTests& other);

	template <typename Data, unsigned int Size>
	void append(std::vector<Data>& dataVector, const Data (&dataArray)[Size]);
};

} /* glcts namespace */

#endif // _GLCINTERNALFORMATTESTS_HPP
