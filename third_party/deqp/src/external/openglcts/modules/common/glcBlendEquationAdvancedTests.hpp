#ifndef _GLCBLENDEQUATIONADVANCEDTESTS_HPP
#define _GLCBLENDEQUATIONADVANCEDTESTS_HPP
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

/*
 New Tests

 Blending mode tests

 * For each new blending mode, render a simple test image where two constant
 color primites overlap and are blended together. Use ReadPixels to obtain
 a sample pixel from the overlap area and compare it to an expected result.
 - Each blending mode is tested with multiple preselected color
 combinations. The color combinations should be selected for each
 blending equation so that they exercise all equivalance classes and
 boundary values.
 - Expected results are calculated at run-time using the blending equation
 definitions.
 - Comparison needs to use a tolerance that takes the possible precision
 differences into account.
 - Each blending mode needs to be tested with fragment shaders that use
 1. blend_support_[mode] and 2. blend_support_all layout qualifiers.


 Coherent blending tests

 * These tests are otherwise similar to "Blending mode tests" but instead of rendering
 2 overlapping primitives, render N primitives using different blending modes
 - If XXX_blend_equation_advanced_coherent is supported, enable
 BLEND_ADVANCED_COHERENT_XXX setting before running the tests. If it is
 not supported, execute BlendBarrierXXX after rendering each primitive.
 - Each blending mode needs to be tested with a fragment shaders that uses
 blend_support_all layout qualifier and a fragment shader that
 lists all the used blending modes explicitly.


 Other tests

 * If XXX_blend_equation_advanced_coherent is supported, test
 BLEND_ADVANCED_COHERENT_XXX setting:
 - The setting should work with Enable, Disable and IsEnable without producing errors
 - Default value should be TRUE

 * Test that rendering into more than one color buffers at once produces
 INVALID_OPERATION error when calling drawArrays/drawElements

 * Each blending mode needs to be tested without specifying the proper
 blend_support_[mode] or blend_support_all layout qualifier in the
 fragment shader. Expect INVALID_OPERATION GL error after calling
 DrawElements/Arrays.

 * Test that the new blending modes cannot be used with
 BlendEquationSeparate(i). Expect INVALID_ENUM GL error.

 * Test different behaviors for GLSL #extension
 GL_XXX_blend_equation_advanced

 * Check that GLSL GL_XXX_blend_equation_advanced #define exists and is 1
 */

namespace glcts
{

class BlendEquationAdvancedTests : public deqp::TestCaseGroup
{
public:
	BlendEquationAdvancedTests(deqp::Context& context, glu::GLSLVersion glslVersion);
	~BlendEquationAdvancedTests();

	void init(void);

private:
	glu::GLSLVersion m_glslVersion;
};

} // glcts

#endif // _GLCBLENDEQUATIONADVANCEDTESTS_HPP
