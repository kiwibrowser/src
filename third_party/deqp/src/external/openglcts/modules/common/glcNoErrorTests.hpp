#ifndef _GLCNOERRORTESTS_HPP
#define _GLCNOERRORTESTS_HPP
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
 * \file
 * \brief
 */ /*-------------------------------------------------------------------*/

/**
 */ /*!
 * \file  glcNoErrorTests.hpp
 * \brief Conformance tests for the GL_KHR_no_error functionality.
 */ /*--------------------------------------------------------------------*/

#include "glcTestCase.hpp"
#include "glwDefs.hpp"
#include "tcuDefs.hpp"

namespace glcts
{
/** Test verifies if it is possible to create context with
	 *  CONTEXT_FLAG_NO_ERROR_BIT_KHR flag set in CONTEXT_FLAGS.
	 */
class NoErrorContextTest : public tcu::TestCase
{
public:
	/* Public methods */
	NoErrorContextTest(tcu::TestContext& testCtx, glu::ApiType apiType);

	void						 deinit(void);
	void						 init(void);
	tcu::TestNode::IterateResult iterate(void);

private:
	/* Private methods */
	bool verifyNoErrorContext(void);
	/* Private members */
	glu::ApiType m_ApiType;
};

/** Test group which encapsulates all sparse buffer conformance tests */
class NoErrorTests : public tcu::TestCaseGroup
{
public:
	/* Public methods */
	NoErrorTests(tcu::TestContext& testCtx, glu::ApiType apiType);

	void init(void);

private:
	NoErrorTests(const NoErrorTests& other);
	NoErrorTests& operator=(const NoErrorTests& other);

	/* Private members */
	glu::ApiType m_ApiType;
};
} /* glcts namespace */

#endif // _GLCNOERRORTESTS_HPP
