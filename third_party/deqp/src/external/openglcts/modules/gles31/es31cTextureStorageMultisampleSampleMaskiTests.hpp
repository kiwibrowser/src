#ifndef _ES31CTEXTURESTORAGEMULTISAMPLESAMPLEMASKITESTS_HPP
#define _ES31CTEXTURESTORAGEMULTISAMPLESAMPLEMASKITESTS_HPP
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
 */ /*!
 * \file  es31cTextureStorageMultisampleSampleMaskiTests.hpp
 * \brief Declares test classes for glSampleMaski() conformance
 *        tests. (ES3.1 only)
 */ /*-------------------------------------------------------------------*/

#include "es31cTextureStorageMultisampleTests.hpp"

namespace glcts
{
/** Test case: No error is reported for calls using (0 < index < MAX_SAMPLE_MASK_WORDS value) argument.
 */
class MultisampleTextureSampleMaskiIndexLowerThanGLMaxSampleMaskWordsTest : public glcts::TestCase
{
public:
	/* Public methods */
	MultisampleTextureSampleMaskiIndexLowerThanGLMaxSampleMaskWordsTest(Context& context);

	virtual tcu::TestNode::IterateResult iterate();
};

/** Test case: glSampleMaski(): GL_INVALID_VALUE error is reported if index argument is equal to GL_MAX_SAMPLE_MASK_WORDS value. */
class MultisampleTextureSampleMaskiIndexEqualToGLMaxSampleMaskWordsTest : public glcts::TestCase
{
public:
	/* Public methods */
	MultisampleTextureSampleMaskiIndexEqualToGLMaxSampleMaskWordsTest(Context& context);

	virtual tcu::TestNode::IterateResult iterate();
};

/** Test case: glSampleMaski(): GL_INVALID_VALUE error is reported if index argument is larger than GL_MAX_SAMPLE_MASK_WORDS value. */
class MultisampleTextureSampleMaskiIndexGreaterGLMaxSampleMaskWordsTest : public glcts::TestCase
{
public:
	/* Public methods */
	MultisampleTextureSampleMaskiIndexGreaterGLMaxSampleMaskWordsTest(Context& context);

	virtual tcu::TestNode::IterateResult iterate();
};

/** Test case:  glSampleMaski(): Sample masks set with the call are correctly reported for
 *                                  glGetIntegeri_v() calls with GL_SAMPLE_MASK_VALUE pname.
 **/
class MultisampleTextureSampleMaskiGettersTest : public glcts::TestCase
{
public:
	/* Public methods */
	MultisampleTextureSampleMaskiGettersTest(Context& context);

	virtual tcu::TestNode::IterateResult iterate();
};
} /* glcts namespace */

#endif // _ES31CTEXTURESTORAGEMULTISAMPLESAMPLEMASKITESTS_HPP
