#ifndef _GLCPARALLELSHADERCOMPILETESTS_HPP
#define _GLCPARALLELSHADERCOMPILETESTS_HPP
/*-------------------------------------------------------------------------
 * OpenGL Conformance Test Suite
 * -----------------------------
 *
 * Copyright (c) 2016-2017 The Khronos Group Inc.
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
 * \file  glcParallelShaderCompileTests.hpp
 * \brief Conformance tests for the GL_KHR_parallel_shader_compile functionality.
 */ /*-------------------------------------------------------------------*/
#include "glcTestCase.hpp"
#include "glwDefs.hpp"
#include "tcuDefs.hpp"
#include <vector>

namespace glcts
{

/** Test verifies if GetBooleanv, GetIntegerv, GetInteger64v, GetFloatv, and GetDoublev
 *  queries for MAX_SHADER_COMPILER_THREADS_KHR <pname> returns the same value.
 **/
class SimpleQueriesTest : public deqp::TestCase
{
public:
	/* Public methods */
	SimpleQueriesTest(deqp::Context& context);

	tcu::TestNode::IterateResult iterate();

private:
	/* Private methods */
	/* Private members */
};

/** Test verifies if MaxShaderCompilerThreadsKHR function works as expected
 **/
class MaxShaderCompileThreadsTest : public deqp::TestCase
{
public:
	/* Public methods */
	MaxShaderCompileThreadsTest(deqp::Context& context);

	tcu::TestNode::IterateResult iterate();

private:
	/* Private methods */
	/* Private members */
};

/** Test verifies if GetShaderiv and GetProgramiv queries returns value as expected
 *  for COMPLETION_STATUS <pname> and non parallel compilation.
 **/
class CompilationCompletionNonParallelTest : public deqp::TestCase
{
public:
	/* Public methods */
	CompilationCompletionNonParallelTest(deqp::Context& context);

	tcu::TestNode::IterateResult iterate();

private:
	/* Private methods */
	/* Private members */
};

/** Test verifies if GetShaderiv and GetProgramiv queries returns value as expected
 *  for COMPLETION_STATUS <pname> and parallel compilation.
 **/
class CompilationCompletionParallelTest : public deqp::TestCase
{
public:
	/* Public methods */
	CompilationCompletionParallelTest(deqp::Context& context);

	tcu::TestNode::IterateResult iterate();

private:
	/* Private methods */
	/* Private members */
};

/** Test group which encapsulates all sparse buffer conformance tests */
class ParallelShaderCompileTests : public deqp::TestCaseGroup
{
public:
	/* Public methods */
	ParallelShaderCompileTests(deqp::Context& context);

	void init();

private:
	ParallelShaderCompileTests(const ParallelShaderCompileTests& other);
	ParallelShaderCompileTests& operator=(const ParallelShaderCompileTests& other);
};

} /* glcts namespace */

#endif // _GLCPARALLELSHADERCOMPILETESTS_HPP
