#ifndef _GL3CGLSLNOPERSPECTIVETESTS_HPP
#define _GL3CGLSLNOPERSPECTIVETESTS_HPP
/*-------------------------------------------------------------------------
 * OpenGL Conformance Test Suite
 * -----------------------------
 *
 * Copyright (c) 2015-2016 The Khronos Group Inc.
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
 * \file  gl3cGLSLnoperspectiveTests.hpp
 * \brief Declares test classes for "GLSL no perspective" functionality.
 */ /*-------------------------------------------------------------------*/

#include "glcTestCase.hpp"
#include "glwDefs.hpp"
#include "glwEnums.hpp"
#include "tcuDefs.hpp"

namespace gl3cts
{
/** Implements FunctionalTest, description follows:
 *
 * Steps:
 * - prepare a vertex shader which passes position and color from input to
 * output;
 * - prepare four fragment shaders that passes color from input to output;
 * Each of shaders should qualify input with different interpolation:
 *   * default - no qualifier
 *   * smooth
 *   * flat
 *   * noperspective
 * - prepare a vertex array buffer that contains following vertices:
 *     IDX | position       | color
 *   * 1   | -1,  1, -1,  1 | red
 *   * 2   |  3,  3,  3,  3 | green
 *   * 3   | -1, -1, -1,  1 | blue
 *   * 4   |  3, -3,  3,  3 | white
 * - for each fragment shader:
 *   * prepare framebuffer with 2D 64x64 RGBA8 textures attached as color 0;
 *   * prepare a program consisting of vertex and tested fragment shader;
 *   * execute DrawArrays to draw triangle strip made of four vertices;
 * - it is expected that contents of framebuffer corresponding with
 * noperspective qualifier will differ from the others.
 **/
class FunctionalTest : public deqp::TestCase
{
public:
	/* Public methods */
	FunctionalTest(deqp::Context& context);

	virtual tcu::TestNode::IterateResult iterate();
};

/** Group class for GPU Shader FP64 conformance tests */
class GLSLnoperspectiveTests : public deqp::TestCaseGroup
{
public:
	/* Public methods */
	GLSLnoperspectiveTests(deqp::Context& context);

	virtual ~GLSLnoperspectiveTests()
	{
	}

	virtual void init(void);

private:
	/* Private methods */
	GLSLnoperspectiveTests(const GLSLnoperspectiveTests&);
	GLSLnoperspectiveTests& operator=(const GLSLnoperspectiveTests&);
};
} /* gl3cts namespace */

#endif // _GL3CGLSLNOPERSPECTIVETESTS_HPP
