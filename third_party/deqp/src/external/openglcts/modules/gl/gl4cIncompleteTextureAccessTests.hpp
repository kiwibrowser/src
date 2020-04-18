#ifndef _GL4CINCOMPLETETEXTUREACCESSTESTS_HPP
#define _GL4CINCOMPLETETEXTUREACCESSTESTS_HPP
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
 * \file  gl4cIncompleteTextureAccessTests.hpp
 * \brief Declares test classes for incomplete texture access cases.
 */ /*-------------------------------------------------------------------*/

/* Includes. */

#include "glcTestCase.hpp"
#include "glwDefs.hpp"
#include "tcuDefs.hpp"

namespace gl4cts
{
namespace IncompleteTextureAccess
{
/** @class Tests
 *
 *  @brief Incomplete texture access test group.
 */
class Tests : public deqp::TestCaseGroup
{
public:
	/* Public member functions */
	Tests(deqp::Context& context);

	void init();

private:
	/* Private member functions */
	Tests(const Tests& other);
	Tests& operator=(const Tests& other);
};
/* Tests class */

/** @class IncompleteTextureAccessTest
 *
 *  Description:
 *
 *    This tests checks access to incomplete texture from shader using
 *    texture sampler functions. For OpenGL 4.5 and higher (0.0, 0.0, 0.0, 1.0)
 *    is expected for floating point non-shadow samplers. 0 is expected
 *    for the shadow sampler.
 *
 *  Steps:
 *
 *      Prepare incomplete texture of given type.
 *      Prepare framebuffer with RGBA renderbuffer 1x1 pixels in size
 *      Prepare program which draws full screen textured quad using given sampler.
 *      Make draw call.
 *      Fetch framebuffer data using glReadPixels.
 *      Compare the values with expected value.
 *
 *  Repeat the steps for following samplers:
 *   -  sampler1D​,
 *   -  sampler2D​,
 *   -  sampler3D​,
 *   -  samplerCube,​
 *   -  sampler2DRect,
 *   -  sampler1DArray​,
 *   -  sampler2DArray​,
 *   -  samplerCubeArray
 *  expecting ​(0.0, 0.0, 0.0, 1.0) and:
 *   -  sampler1DShadow​,
 *   -  sampler2DShadow​,
 *   -  samplerCubeShadow,
 *   -  sampler2DRectShadow,
 *   -  sampler1DArrayShadow​,
 *   -  sampler2DArrayShadow​,
 *   -  samplerCubeArrayShadow​
 *  expecting ​0.0.
 */
class SamplerTest : public deqp::TestCase
{
public:
	/* Public member functions */
	SamplerTest(deqp::Context& context);

	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private member functions */
	SamplerTest(const SamplerTest& other);
	SamplerTest& operator=(const SamplerTest& other);

	glw::GLuint m_po;
	glw::GLuint m_to;
	glw::GLuint m_fbo;
	glw::GLuint m_rbo;
	glw::GLuint m_vao;

	static const struct Configuration
	{
		glw::GLenum		   texture_target;
		const glw::GLchar* sampler_template;
		const glw::GLchar* fetch_template;
		glw::GLfloat	   expected_result[4];
	} s_configurations[];

	static const glw::GLuint s_configurations_count;

	static const glw::GLchar* s_vertex_shader;
	static const glw::GLchar* s_fragment_shader_head;
	static const glw::GLchar* s_fragment_shader_body;
	static const glw::GLchar* s_fragment_shader_tail;

	void PrepareProgram(Configuration configuration);
	void PrepareTexture(Configuration configuration);
	void PrepareVertexArrays();
	void PrepareFramebuffer();
	void Draw();
	bool Check(Configuration configuration);
	void CleanCase();
	void CleanTest();
};

/* SamplerTest class */
} /* IncompleteTextureAccess namespace */
} /* gl4cts */

#endif // _GL4CINCOMPLETETEXTUREACCESSTESTS_HPP
