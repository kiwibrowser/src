#ifndef _ESEXTCTEXTURECUBEMAPARRAYTESTS_HPP
#define _ESEXTCTEXTURECUBEMAPARRAYTESTS_HPP
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

#include "../esextcTestCaseBase.hpp"

namespace glcts
{

class TextureCubeMapArrayTests : public TestCaseGroupBase
{
public:
	/* Public methods */
	TextureCubeMapArrayTests(glcts::Context& context, const ExtParameters& extParams);

	virtual ~TextureCubeMapArrayTests(void)
	{
	}

	void init(void);

private:
	/* Private methods */
	TextureCubeMapArrayTests(const TextureCubeMapArrayTests& other);
	TextureCubeMapArrayTests& operator=(const TextureCubeMapArrayTests& other);
};

} // namespace glcts

#endif // _ESEXTCTEXTURECUBEMAPARRAYTESTS_HPP
