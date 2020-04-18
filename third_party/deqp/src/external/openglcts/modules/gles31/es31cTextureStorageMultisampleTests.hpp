#ifndef _ES31CTEXTURESTORAGEMULTISAMPLETESTS_HPP
#define _ES31CTEXTURESTORAGEMULTISAMPLETESTS_HPP
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

/**
 * \file  es31cTextureStorageMultisampleTests.hpp
 * \brief Declares test group class that verifies multisample texture
 *        functionality (ES3.1 only)
 */

#include "glcTestCase.hpp"
#include "glwDefs.hpp"
#include "tcuDefs.hpp"
#include "tes31TestCase.hpp"

namespace glcts
{
/** Group class for multisample texture storage conformance tests */
class TextureStorageMultisampleTests : public glcts::TestCaseGroup
{
public:
	/* Public methods */
	TextureStorageMultisampleTests(glcts::Context& context);

	void init(void);

private:
	/* Private methods */
	TextureStorageMultisampleTests(const TextureStorageMultisampleTests&);
	TextureStorageMultisampleTests& operator=(const TextureStorageMultisampleTests&);
};

} /* glcts namespace */

#endif // _ES31CTEXTURESTORAGEMULTISAMPLETESTS_HPP
