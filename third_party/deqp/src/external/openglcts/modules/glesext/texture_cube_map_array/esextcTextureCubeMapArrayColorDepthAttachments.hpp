#ifndef _ESEXTCTEXTURECUBEMAPARRAYCOLORDEPTHATTACHMENTS_HPP
#define _ESEXTCTEXTURECUBEMAPARRAYCOLORDEPTHATTACHMENTS_HPP
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

#include "../esextcTestCaseBase.hpp"

namespace glcts
{
/** Implementation of Test 2. Test description follows:
 *
 *  Make sure cube-map texture layers can be used as color- or depth-
 *  attachments.
 *
 *  Category: Functionality tests,
 *            Optional dependency on EXT_geometry_shader;
 *  Priority: Must-have.
 *
 *  Verify that cube-map array texture layers using color- and depth-
 *  renderable internalformats can be used as color and depth attachments
 *  for a framebuffer object by rendering a static (but differentiable between
 *  faces and layers, along the lines of how test 1 behaves) color to the
 *  attachment and then checking the outcome.
 *
 *  Both layered (if supported) and non-layered framebuffers should be checked.
 *
 *  Test four different cube-map array texture resolutions, as described
 *  in test 1. Both immutable and mutable textures should be checked.
 **/
class TextureCubeMapArrayColorDepthAttachmentsTest : public TestCaseBase
{
public:
	/* Public methods */
	TextureCubeMapArrayColorDepthAttachmentsTest(Context& context, const ExtParameters& extParams, const char* name,
												 const char* description);

	virtual ~TextureCubeMapArrayColorDepthAttachmentsTest()
	{
	}

	virtual void		  deinit(void);
	virtual IterateResult iterate(void);

private:
	/* Private type definitions */
	struct _texture_size
	{
		_texture_size(glw::GLuint size, glw::GLuint m_n_cubemaps);

		/* Face width & height */
		glw::GLuint m_size;

		/* Number of cubemaps in array */
		glw::GLuint m_n_cubemaps;
	};

	typedef std::vector<_texture_size> _texture_size_vector;

	/* Private methods */
	void determineSupportedDepthFormat();
	void draw(const _texture_size& texture_size);
	void generateAndConfigureTextureObjects(glw::GLuint texture_size, glw::GLuint n_cubemaps,
											bool should_generate_mutable_textures);
	void initTest();
	void prepareImmutableTextureObject(glw::GLuint texture_id, glw::GLuint texture_size, glw::GLuint n_cubemaps,
									   bool should_take_color_texture_properties);
	void prepareMutableTextureObject(glw::GLuint texture_id, glw::GLuint texture_size, glw::GLuint n_cubemaps,
									 bool should_take_color_texture_properties);
	void releaseAndDetachTextureObject(glw::GLuint texture_id, bool is_color);
	void configureLayeredFramebufferAttachment(glw::GLuint texture_id, bool should_use_as_color_attachment);
	void configureNonLayeredFramebufferAttachment(glw::GLuint texture_id, glw::GLuint n_layer,
												  bool should_use_as_color_attachment,
												  bool should_update_draw_framebuffer = true);
	void testLayeredRendering(const _texture_size& texture_size, bool should_use_mutable_textures);
	void testNonLayeredRendering(const _texture_size& texture_size, bool should_use_mutable_texture);
	bool verifyColorData(const _texture_size& texture_size, glw::GLuint n_layer);
	bool verifyDepth16Data(const _texture_size& texture_size, glw::GLuint n_layer);
	bool verifyDepth24Data(const _texture_size& texture_size, glw::GLuint n_layer);
	bool verifyDepth32FData(const _texture_size& texture_size, glw::GLuint n_layer);

	/* Private fields */
	glw::GLuint m_vao_id;
	glw::GLuint m_color_texture_id;
	glw::GLuint m_depth_texture_id;
	glw::GLuint m_fragment_shader_id;
	glw::GLuint m_framebuffer_object_id;
	glw::GLuint m_layered_geometry_shader_id;
	glw::GLuint m_layered_program_id;
	glw::GLuint m_non_layered_geometry_shader_id;
	glw::GLuint m_non_layered_program_id;
	glw::GLint  m_non_layered_program_id_uni_layer_uniform_location;
	glw::GLuint m_vertex_shader_id;

	_texture_size_vector m_resolutions;

	static const glw::GLenum m_color_internal_format;
	static const glw::GLenum m_color_format;
	static const glw::GLenum m_color_type;
	glw::GLenum				 m_depth_internal_format;
	static const glw::GLenum m_depth_format;
	glw::GLenum				 m_depth_type;

	glw::GLuint m_n_invalid_color_checks;
	glw::GLuint m_n_invalid_depth_checks;

	static const glw::GLchar* const m_fragment_shader_code;
	static const glw::GLchar* const m_geometry_shader_code_preamble;
	static const glw::GLchar* const m_geometry_shader_code_layered;
	static const glw::GLchar* const m_geometry_shader_code_non_layered;
	static const glw::GLchar* const m_geometry_shader_code_body;
	static const glw::GLchar* const m_vertex_shader_code;
};

} /* glcts */

#endif // _ESEXTCTEXTURECUBEMAPARRAYCOLORDEPTHATTACHMENTS_HPP
