#ifndef _ES31CSHADERSTORAGEBUFFEROBJECTTESTS_HPP
#define _ES31CSHADERSTORAGEBUFFEROBJECTTESTS_HPP
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

class ShaderStorageBufferObjectTests : public glcts::TestCaseGroup
{
public:
	ShaderStorageBufferObjectTests(glcts::Context& context);
	~ShaderStorageBufferObjectTests(void);

	void init(void);

private:
	ShaderStorageBufferObjectTests(const ShaderStorageBufferObjectTests& other);
	ShaderStorageBufferObjectTests& operator=(const ShaderStorageBufferObjectTests& other);
};
}

#endif // _ES31CSHADERSTORAGEBUFFEROBJECTTESTS_HPP
