#ifndef _ES31CSHADERBITFIELDOPERATIONTESTS_HPP
#define _ES31CSHADERBITFIELDOPERATIONTESTS_HPP
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
#include "tes31TestCase.hpp"

namespace glcts
{

class ShaderBitfieldOperationTests : public glcts::TestCaseGroup
{
public:
	ShaderBitfieldOperationTests(glcts::Context& context, glu::GLSLVersion glslVersion);
	~ShaderBitfieldOperationTests(void);

	void init(void);

private:
	ShaderBitfieldOperationTests(const ShaderBitfieldOperationTests& other);
	ShaderBitfieldOperationTests& operator=(const ShaderBitfieldOperationTests& other);

	glu::GLSLVersion m_glslVersion;

	enum
	{
		ITERATIONS = 4
	};
};

} // glcts

#endif // _ES31CSHADERBITFIELDOPERATIONTESTS_HPP
