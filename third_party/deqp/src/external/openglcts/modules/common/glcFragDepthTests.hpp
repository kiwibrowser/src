#ifndef _GLCFRAGDEPTHTESTS_HPP
#define _GLCFRAGDEPTHTESTS_HPP
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
 * \brief gl_FragDepth tests.
 */ /*-------------------------------------------------------------------*/

#include "glcTestCase.hpp"
#include "gluShaderUtil.hpp"
#include "tcuDefs.hpp"

namespace deqp
{

class FragDepthTests : public TestCaseGroup
{
public:
	FragDepthTests(Context& context, glu::GLSLVersion glslVersion);
	~FragDepthTests(void);

	void init(void);

private:
	FragDepthTests(const FragDepthTests& other);
	FragDepthTests& operator=(const FragDepthTests& other);

	glu::GLSLVersion m_glslVersion;
};

} // deqp

#endif // _GLCFRAGDEPTHTESTS_HPP
