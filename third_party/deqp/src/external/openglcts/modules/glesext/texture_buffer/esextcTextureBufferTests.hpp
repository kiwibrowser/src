#ifndef _ESEXTCTEXTUREBUFFERTESTS_HPP
#define _ESEXTCTEXTUREBUFFERTESTS_HPP
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

/*!
 * \file  glcTextureBufferTests.hpp
 * \brief Base test group for texture buffer tests
 */ /*-------------------------------------------------------------------*/

#include "../esextcTestCaseBase.hpp"
#include "gluShaderUtil.hpp"
#include "tcuDefs.hpp"

namespace glcts
{

/* Base test group for texture buffer tests */
class TextureBufferTests : public TestCaseGroupBase
{
public:
	/* Public methods */
	TextureBufferTests(glcts::Context& context, const ExtParameters& extParams);

	virtual ~TextureBufferTests(void)
	{
	}

	void init(void);

private:
	/* Private methods */
	TextureBufferTests(const TextureBufferTests& other);
	TextureBufferTests& operator=(const TextureBufferTests& other);
};

} // namespace glcts

#endif // _ESEXTCTEXTUREBUFFERTESTS_HPP
