#ifndef _GL4CCONDITIONALRENDERINVERTEDTESTS_HPP
#define _GL4CCONDITIONALRENDERINVERTEDTESTS_HPP
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
 */ /*!
 * \file  gl4cConditionalRenderInvertedTests.hpp
 * \brief Conformance tests for Conditional Render Inverted feature functionality.
 */ /*------------------------------------------------------------------------------*/

/* Includes. */

#include "glcTestCase.hpp"
#include "glwDefs.hpp"
#include "tcuDefs.hpp"

#include "glwEnums.hpp"
#include "glwFunctions.hpp"

/* Interface. */

namespace gl4cts
{
namespace ConditionalRenderInverted
{
/** @class Tests
 *
 *  @brief Conditional Render Inverted Test Group.
 */
class Tests : public deqp::TestCaseGroup
{
public:
	/* Public member functions. */
	Tests(deqp::Context& context);

	void init();

private:
	/* Private member functions. */
	Tests(const Tests& other);
	Tests& operator=(const Tests& other);
};

/** @class CoverageTest
 *
 *  @brief Conditional Render Inverted API Coverage Test.
 *
 *  The test checks that following modes:
 *      QUERY_WAIT_INVERTED                             0x8E17,
 *      QUERY_NO_WAIT_INVERTED                          0x8E18,
 *      QUERY_BY_REGION_WAIT_INVERTED                   0x8E19,
 *      QUERY_BY_REGION_NO_WAIT_INVERTED                0x8E1A,
 *   are accepted by BeginConditionalRender.
 *
 *  See reference: ARB_conditional_render_inverted extension specification or
 *                 Chapter 10 of the OpenGL 4.4 (Core Profile) Specification.
 */
class CoverageTest : public deqp::TestCase
{
public:
	/* Public member functions. */
	CoverageTest(deqp::Context& context);

	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private member functions */
	CoverageTest(const CoverageTest& other);
	CoverageTest& operator=(const CoverageTest& other);

	void createQueryObject();
	void clean();
	bool test(glw::GLenum mode);

	/* Private member variables. */
	glw::GLuint m_qo_id;
};
/* class CoverageTest */

/** @class FunctionalTest
 *
 *  @brief Conditional Render Inverted Functional Test.
 *
 *  The test runs as follows:
 *
 *  Prepare program consisting of vertex and fragment shader which draws
 *  full screen quad depending on vertex ID. Fragment shader shall be able
 *  discard fragments depending on uniform value. Return color shall also be
 *  controlled by uniform. Prepare 1x1 pixels' size framebuffer object with
 *  R8 format.
 *
 *  For each render case,
 *      for each query case,
 *          for each conditional render inverted mode,
 *              do the following:
 *              - create query object;
 *              - setup program to pass or discard fragments depending on
 *                render case;
 *              - setup color uniform to red component equal to 1;
 *              - clear framebuffer with red color component equal to 0.5;
 *              - draw quad using query object;
 *              - check that fragments passed or not using query object
 *                query;
 *                if program behaved not as expected return failure;
 *                note: query shall finish;
 *              - setup program to pass all fragments;
 *              - setup color uniform to red component equal to 0;
 *              - draw using the conditional rendering;
 *              - read framebuffer pixel;
 *              - expect that red component of the pixel is 1.0 if render
 *                case passes all pixels or 0.0 otherwise; if read color is
 *                different than expected, return failure;
 *              - cleanup query object.
 *  After loop, cleanup program and framebuffer. Return pass if all tests
 *  passed.
 *
 *  Test for following render cases:
 *  - all fragments passes,
 *  - all fragments are discarded.
 *
 *  Test for following query cases:
 *  - SAMPLES_PASSED,
 *  - GL_ANY_SAMPLES_PASSED.
 *
 *  Test for following conditional render inverted modes:
 *  - QUERY_WAIT_INVERTED,
 *  - QUERY_NO_WAIT_INVERTED,
 *  - QUERY_BY_REGION_WAIT_INVERTED,
 *  - QUERY_BY_REGION_NO_WAIT_INVERTED.
 *
 *  See reference: ARB_conditional_render_inverted extension specification or
 *                 Chapter 10 of the OpenGL 4.4 (Core Profile) Specification.
 */
class FunctionalTest : public deqp::TestCase
{
public:
	/* Public member functions. */
	FunctionalTest(deqp::Context& context);

	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private member functions. */
	FunctionalTest(const FunctionalTest& other);
	FunctionalTest& operator=(const FunctionalTest& other);

	void createProgram();
	void createVertexArrayObject();
	void createView();
	void createQueryObject();
	void setupColor(const glw::GLfloat red);
	void setupPassSwitch(const bool shall_pass);
	void clearView();

	void draw(const bool conditional_or_query_draw, const glw::GLenum condition_mode_or_query_target);

	bool		 fragmentsPassed();
	glw::GLfloat readPixel();
	void		 cleanQueryObject();
	void		 cleanProgramViewAndVAO();

	/* Private member variables. */
	glw::GLuint m_fbo_id; //!<    Test's framebuffer object id.
	glw::GLuint m_rbo_id; //!<    Test's renderbuffer object id.
	glw::GLuint m_vao_id; //!<    Test's vertex array object id.
	glw::GLuint m_po_id;  //!<    Test's program object id.
	glw::GLuint m_qo_id;  //!<    Test's query object id.

	/* Private static constants. */
	static const glw::GLchar s_vertex_shader[];		 //!<    Vertex shader source code.
	static const glw::GLchar s_fragment_shader[];	//!<    Fragment shader source code.
	static const glw::GLchar s_color_uniform_name[]; //!<    Name of the color uniform.
	static const glw::GLchar
							 s_pass_switch_uniform_name[]; //!<    Name of the fragment pass or discarded uniform switch.
	static const glw::GLuint s_view_size;				   //!<    Size of view (1 by design).
};
/* class FunctionalTest*/

namespace Utilities
{
const glw::GLchar* modeToChars(glw::GLenum mode);
const glw::GLchar* queryTargetToChars(glw::GLenum mode);
}

} /* ConditionalRenderInverted namespace */
} /* gl4cts namespace */

#endif // _GL4CCONDITIONALRENDERINVERTEDTESTS_HPP
