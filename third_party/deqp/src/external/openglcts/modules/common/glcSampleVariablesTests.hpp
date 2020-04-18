#ifndef _GLCSAMPLEVARIABLESTESTS_HPP
#define _GLCSAMPLEVARIABLESTESTS_HPP
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

#include "glcTestCase.hpp"
#include "gluShaderUtil.hpp"
#include "tcuDefs.hpp"

namespace deqp
{

class SampleVariablesTests : public TestCaseGroup
{
public:
	SampleVariablesTests(Context& context, glu::GLSLVersion glslVersion);
	~SampleVariablesTests();

	void init();

private:
	SampleVariablesTests(const SampleVariablesTests& other);
	SampleVariablesTests& operator=(const SampleVariablesTests& other);

	enum
	{
		SAMPLE_MASKS = 8
	};

	glu::GLSLVersion m_glslVersion;
};

} // deqp

#endif // _GLCSAMPLEVARIABLESTESTS_HPP
