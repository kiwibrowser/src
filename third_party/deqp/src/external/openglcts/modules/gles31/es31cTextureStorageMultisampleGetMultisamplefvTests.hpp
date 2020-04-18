#ifndef _ES31CTEXTURESTORAGEMULTISAMPLEGETMULTISAMPLEFVTESTS_HPP
#define _ES31CTEXTURESTORAGEMULTISAMPLEGETMULTISAMPLEFVTESTS_HPP
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
 * \file  es31cTextureStorageMultisampleGetMultisamplefvTests.hpp
 * \brief Declares test classes for glGetMultisamplefv() conformance
 *        tests. (ES3.1 only)
 */ /*-------------------------------------------------------------------*/

#include "es31cTextureStorageMultisampleTests.hpp"

namespace glcts
{
/** Test case: GL_INVALID_VALUE error is reported if index argument is equal to GL_SAMPLES value */
class MultisampleTextureGetMultisamplefvIndexEqualGLSamplesRejectedTest : public glcts::TestCase
{
public:
	/* Public methods */
	MultisampleTextureGetMultisamplefvIndexEqualGLSamplesRejectedTest(Context& context);

	virtual tcu::TestNode::IterateResult iterate();
};

/** Test case: GL_INVALID_VALUE error is reported if index argument is greater than GL_SAMPLES value */
class MultisampleTextureGetMultisamplefvIndexGreaterGLSamplesRejectedTest : public glcts::TestCase
{
public:
	/* Public methods */
	MultisampleTextureGetMultisamplefvIndexGreaterGLSamplesRejectedTest(Context& context);

	virtual tcu::TestNode::IterateResult iterate();
};

/** Test case: GL_INVALID_ENUM error is reported for invalid pname.*/
class MultisampleTextureGetMultisamplefvInvalidPnameRejectedTest : public glcts::TestCase
{
public:
	/* Public methods */
	MultisampleTextureGetMultisamplefvInvalidPnameRejectedTest(Context& context);

	virtual tcu::TestNode::IterateResult iterate();
};

/** Test case: NULL val arguments accepted for valid glGetMultisamplefv() calls. */
class MultisampleTextureGetMultisamplefvNullValArgumentsAcceptedTest : public glcts::TestCase
{
public:
	/* Public methods */
	MultisampleTextureGetMultisamplefvNullValArgumentsAcceptedTest(Context& context);

	virtual tcu::TestNode::IterateResult iterate();
	virtual void						 deinit();

private:
	glw::GLuint fbo_id;
	glw::GLuint to_2d_multisample_id;
};

/** Test case: Spec-wise correct values are reported for valid
 *             calls with GL_SAMPLE_POSITION pname.
 **/
class MultisampleTextureGetMultisamplefvSamplePositionValuesValidationTest : public glcts::TestCase
{
public:
	/* Public methods */
	MultisampleTextureGetMultisamplefvSamplePositionValuesValidationTest(Context& context);

	virtual tcu::TestNode::IterateResult iterate();
};
} /* glcts namespace */

#endif // _ES31CTEXTURESTORAGEMULTISAMPLEGETMULTISAMPLEFVTESTS_HPP
