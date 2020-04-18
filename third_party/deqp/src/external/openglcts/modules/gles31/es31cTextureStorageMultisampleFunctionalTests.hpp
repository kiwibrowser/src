#ifndef _ES31CTEXTURESTORAGEMULTISAMPLEFUNCTIONALTESTS_HPP
#define _ES31CTEXTURESTORAGEMULTISAMPLEFUNCTIONALTESTS_HPP
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
 * \file  es31cTextureStorageMultisampleFunctionalTests.hpp
 * \brief Declares test classes for multisample texture functional conformance
 *        tests. (ES3.1 only)
 */ /*-------------------------------------------------------------------*/

#include "es31cTextureStorageMultisampleTests.hpp"

namespace glcts
{
/** Test case: Verify that blitting from a multi-sampled framebuffer object
 *             to a single-sampled framebuffer object.
 */
class MultisampleTextureFunctionalTestsBlittingTest : public glcts::TestCase
{
public:
	/* Public methods */
	MultisampleTextureFunctionalTestsBlittingTest(Context& context);

	virtual void						 deinit();
	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private variables */
	glw::GLuint dst_fbo_id;
	glw::GLuint dst_to_color_id;
	glw::GLuint dst_to_depth_stencil_id;
	glw::GLuint src_fbo_id;
	glw::GLuint src_to_color_id;
	glw::GLuint src_to_depth_stencil_id;
};

/** Test case: Verifies data rendered as a result of blitting depth multisample data
 *             to a single-sampled depth attachment is valid.
 */
class MultisampleTextureFunctionalTestsBlittingMultisampledDepthAttachmentTest : public glcts::TestCase
{
public:
	/* Public methods */
	MultisampleTextureFunctionalTestsBlittingMultisampledDepthAttachmentTest(Context& context);

	virtual void						 deinit();
	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private variables */
	glw::GLuint fbo_dst_id;
	glw::GLuint fbo_src_id;
	glw::GLuint fs_depth_preview_id;
	glw::GLuint fs_id;
	glw::GLuint po_depth_preview_id;
	glw::GLuint po_id;
	glw::GLuint to_dst_preview_id;
	glw::GLuint to_dst_id;
	glw::GLuint to_src_id;
	glw::GLuint vao_id;
	glw::GLuint vs_id;
};

/** Test case: Verifies data rendered as a result of blitting integer multisample data
 *             to a single-sampled color attachment is valid.
 */
class MultisampleTextureFunctionalTestsBlittingMultisampledIntegerAttachmentTest : public glcts::TestCase
{
public:
	/* Public methods */
	MultisampleTextureFunctionalTestsBlittingMultisampledIntegerAttachmentTest(Context& context);

	virtual void						 deinit();
	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private variables */
	glw::GLuint dst_to_id;
	glw::GLuint fbo_draw_id;
	glw::GLuint fbo_read_id;
	glw::GLuint fs_id;
	glw::GLuint po_id;
	glw::GLuint src_to_id;
	glw::GLuint vs_id;
};

/** Test case: Verify that blitting to a multisampled framebuffer object results
 *             in GL_INVALID_OPERATION error.
 */
class MultisampleTextureFunctionalTestsBlittingToMultisampledFBOIsForbiddenTest : public glcts::TestCase
{
public:
	/* Public methods */
	MultisampleTextureFunctionalTestsBlittingToMultisampledFBOIsForbiddenTest(Context& context);

	virtual void						 deinit();
	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private variables */
	glw::GLuint dst_to_id;
	glw::GLuint fbo_draw_id;
	glw::GLuint fbo_read_id;
	glw::GLuint src_to_id;
};

/** Test case: Verify sample masking mechanism for non-integer color-renderable internalformats used for 2D multisample textures */
class MultisampleTextureFunctionalTestsSampleMaskingForNonIntegerColorRenderableTexturesTest : public glcts::TestCase
{
public:
	/* Public methods */
	MultisampleTextureFunctionalTestsSampleMaskingForNonIntegerColorRenderableTexturesTest(Context& context);

	virtual void						 deinit();
	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private methods */
	virtual void compileShader(glw::GLuint id, const glw::GLchar* source);
	virtual void linkProgram(glw::GLuint id);

	void initInternals();

	/* Private variables */
	glw::GLuint bo_id;
	glw::GLuint fbo_id;
	glw::GLuint fs_draw_id;
	glw::GLuint po_draw_id;
	glw::GLuint po_verify_id;
	glw::GLuint tfo_id;
	glw::GLuint to_2d_multisample_id;
	glw::GLuint vs_draw_id;
	glw::GLuint vs_verify_id;
};

/** Test case: Verify sample masking mechanism for normalized/unsigned integer/signed integer color-renderable
 *             and depth-renderable internalformats used for 2D multisample textures */
class MultisampleTextureFunctionalTestsSampleMaskingTexturesTest : public glcts::TestCase
{
public:
	/* Public methods */
	MultisampleTextureFunctionalTestsSampleMaskingTexturesTest(Context& context);

	virtual void						 deinit();
	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private methods */
	virtual void compileShader(glw::GLuint id, const glw::GLchar* source);
	void		 initInternals();
	virtual void linkProgram(glw::GLuint id);

	/* Private variables */
	glw::GLuint bo_id;
	glw::GLuint fbo_id;
	glw::GLuint fs_draw_id;
	glw::GLuint po_draw_id;
	glw::GLuint po_verify_id;
	glw::GLuint tfo_id;
	glw::GLuint to_2d_multisample_id;
	glw::GLuint vs_draw_id;
	glw::GLuint vs_verify_id;
};

/** Test case: Verify 2D/2D array multisample texture size can be queried successfully
 *             from within fragment shaders.
 */
class MultisampleTextureFunctionalTestsTextureSizeFragmentShadersTest : public glcts::TestCase
{
public:
	/* Public methods */
	MultisampleTextureFunctionalTestsTextureSizeFragmentShadersTest(Context& context);

	virtual void						 deinit();
	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private variables */
	glw::GLuint fbo_id;
	glw::GLuint fs_id;
	glw::GLuint po_id;
	glw::GLuint to_2d_multisample_id;
	glw::GLuint to_2d_multisample_array_id;
	glw::GLuint vs_id;
};

/** Test case: Verify 2D/2D array multisample texture size can be queried successfully
 *             from within vertex shaders.
 */
class MultisampleTextureFunctionalTestsTextureSizeVertexShadersTest : public glcts::TestCase
{
public:
	/* Public methods */
	MultisampleTextureFunctionalTestsTextureSizeVertexShadersTest(Context& context);

	virtual void						 deinit();
	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private variables */
	glw::GLuint bo_id;
	glw::GLuint fbo_id;
	glw::GLuint fs_id;
	glw::GLuint po_id;
	glw::GLuint tfo_id;
	glw::GLuint to_2d_multisample_id;
	glw::GLuint to_2d_multisample_array_id;
	glw::GLuint vs_2d_array_id;
	glw::GLuint vs_2d_id;
};
} /* glcts namespace */

#endif // _ES31CTEXTURESTORAGEMULTISAMPLEFUNCTIONALTESTS_HPP
