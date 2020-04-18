#ifndef _GL4CSTENCILTEXTURINGTESTS_HPP
#define _GL4CSTENCILTEXTURINGTESTS_HPP
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

#include "glcTestCase.hpp"
#include "glwDefs.hpp"

namespace gl4cts
{
namespace StencilTexturing
{
/**
 * * Create packed GL_DEPTH_STENCIL GL_DEPTH24_STENCIL8 format 2D texture
 *   with a gradient in both depth and stencil components. Set
 *   GL_DEPTH_STENCIL_TEXTURE_MODE texture parameter to GL_DEPTH_COMPONENT,
 *   verify that no error is generated. Create framebuffer and bind it for
 *   drawing. Use a fragment shader with a sampler2D to copy depth component
 *   value to the output color. Draw triangle strip and read color pixels.
 *   Verify that gradient values are copied correctly.
 *
 * * Set GL_DEPTH_STENCIL_TEXTURE_MODE texture parameter to GL_STENCIL_INDEX,
 *   verify that no error is generated. Use a fragment shader with usampler2D
 *   to copy stencil index value to output color. Draw triangle strip and read
 *   color pixels. Verify that gradient values are copied correctly.
 *
 * * Repeat all the above for GL_DEPTH32F_STENCIL8 format
 **/
class FunctionalTest : public deqp::TestCase
{
public:
	FunctionalTest(deqp::Context& context);

	~FunctionalTest()
	{
	}

	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private routines */
	void dispatch(glw::GLuint program_id, bool is_stencil, glw::GLuint dst_texture_id, glw::GLuint src_texture_id);

	void draw(glw::GLuint program_id, glw::GLuint dst_texture_id, glw::GLuint src_texture_id);

	glw::GLuint prepareDestinationTexture(bool is_stencil);

	glw::GLuint prepareProgram(bool is_draw, bool is_stencil);

	glw::GLuint prepareSourceTexture(glw::GLenum internal_format, bool is_stencil,
									 const std::vector<glw::GLubyte>& texture_data);

	void prepareSourceTextureData(glw::GLenum internal_format, std::vector<glw::GLubyte>& texture_data);

	bool verifyTexture(glw::GLuint id, glw::GLenum source_internal_format, bool is_stencil,
					   const std::vector<glw::GLubyte>& src_texture_data);

	bool test(glw::GLenum internal_format, bool is_stencil);

	/* Private constants */
	static const glw::GLchar* m_compute_shader_code;
	static const glw::GLchar* m_fragment_shader_code;
	static const glw::GLchar* m_geometry_shader_code;
	static const glw::GLchar* m_tesselation_control_shader_code;
	static const glw::GLchar* m_tesselation_evaluation_shader_code;
	static const glw::GLchar* m_vertex_shader_code;
	static const glw::GLchar* m_expected_value_depth;
	static const glw::GLchar* m_expected_value_stencil;
	static const glw::GLchar* m_image_definition_depth;
	static const glw::GLchar* m_image_definition_stencil;
	static const glw::GLchar* m_output_type_depth;
	static const glw::GLchar* m_output_type_stencil;
	static const glw::GLchar* m_sampler_definition_depth;
	static const glw::GLchar* m_sampler_definition_stencil;
	static const glw::GLuint  m_height;
	static const glw::GLint   m_image_unit;
	static const glw::GLint   m_texture_unit;
	static const glw::GLuint  m_width;
};
} /* namespace StencilTexturing */

class StencilTexturingTests : public deqp::TestCaseGroup
{
public:
	StencilTexturingTests(deqp::Context& context);
	~StencilTexturingTests(void);

	virtual void init(void);

private:
	StencilTexturingTests(const StencilTexturingTests& other);
	StencilTexturingTests& operator=(const StencilTexturingTests& other);
};
} /* namespace gl4cts */

#endif // _GL4CSTENCILTEXTURINGTESTS_HPP
