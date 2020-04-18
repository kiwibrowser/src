#ifndef _GL4CSPIRVEXTENSIONSTESTS_HPP
#define _GL4CSPIRVEXTENSIONSTESTS_HPP
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
 * \file  gl4cSpirvExtensionsTests.cpp
 * \brief Conformance tests for the GL_ARB_spirv_extensions functionality.
 */ /*-------------------------------------------------------------------*/

#include "glcTestCase.hpp"
#include "glwDefs.hpp"
#include "glwEnums.hpp"
#include "tcuDefs.hpp"

using namespace glw;
using namespace glu;

namespace gl4cts
{

/** Test verifies GetIntegerv query for NUM_SPIR_V_EXTENSIONS <pname>
 *  and GetStringi query for SPIR_V_EXTENSIONS <pname>
 **/
class SpirvExtensionsQueriesTestCase : public deqp::TestCase
{
public:
	/* Public methods */
	SpirvExtensionsQueriesTestCase(deqp::Context& context);

	void						 init();
	tcu::TestNode::IterateResult iterate();

private:
	/* Private members */
	/* Private methods */
};

/** Test group which encapsulates spirv extensions conformance tests */
class SpirvExtensionsTests : public deqp::TestCaseGroup
{
public:
	/* Public methods */
	SpirvExtensionsTests(deqp::Context& context);

	void init();

private:
	SpirvExtensionsTests(const SpirvExtensionsTests& other);
	SpirvExtensionsTests& operator=(const SpirvExtensionsTests& other);
};

} /* gl4cts namespace */

#endif // _GL4CSPIRVEXTENSIONSTESTS_HPP
