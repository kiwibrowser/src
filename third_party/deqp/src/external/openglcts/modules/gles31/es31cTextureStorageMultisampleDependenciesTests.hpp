#ifndef _ES31CTEXTURESTORAGEMULTISAMPLEDEPENDENCIESTESTS_HPP
#define _ES31CTEXTURESTORAGEMULTISAMPLEDEPENDENCIESTESTS_HPP
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
 * \file  es31cTextureStorageMultisampleDependenciesTests.hpp
 * \brief Declares test classes for testing dependencies of multisample
 *        textures with other parts of the API (ES3.1 only)
 */ /*-------------------------------------------------------------------*/

#include "es31cTextureStorageMultisampleTests.hpp"

namespace glcts
{
/** Test case: FBOs with multisample texture attachments, whose amount of samples differs
 *  between attachments, should be considered incomplete (GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE)
 */
class MultisampleTextureDependenciesFBOIncompleteness1Test : public glcts::TestCase
{
public:
	/* Public methods */
	MultisampleTextureDependenciesFBOIncompleteness1Test(Context& context);

	virtual void						 deinit();
	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private variables */
	glw::GLuint fbo_id;
	glw::GLuint to_id_multisample_2d_array;
	glw::GLuint to_ids_multisample_2d[2];
};

/** Test case: FBOs with multisample texture and normal 2D texture attachments should be
 *  considered incomplete (GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE)
 */
class MultisampleTextureDependenciesFBOIncompleteness2Test : public glcts::TestCase
{
public:
	/* Public methods */
	MultisampleTextureDependenciesFBOIncompleteness2Test(Context& context);

	virtual void						 deinit();
	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private variables */
	glw::GLuint fbo_id;
	glw::GLuint to_id_2d;
	glw::GLuint to_id_multisample_2d;
	glw::GLuint to_id_multisample_2d_array;
};

/** Test case: FBOs with multisample texture attachments of different "fixed sample
 *             location" settings should be considered incomplete
 *             (GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE)
 */
class MultisampleTextureDependenciesFBOIncompleteness3Test : public glcts::TestCase
{
public:
	/* Public methods */
	MultisampleTextureDependenciesFBOIncompleteness3Test(Context& context);

	virtual void						 deinit();
	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private variables */
	glw::GLuint fbo_id;
	glw::GLuint to_id_2d_multisample_color_1;
	glw::GLuint to_id_2d_multisample_color_2;
	glw::GLuint to_id_2d_multisample_depth;
	glw::GLuint to_id_2d_multisample_depth_stencil;
};

/** Test case: FBOs with multisample texture attachments of different "fixed sample
 *             location" settings and with multisampled renderbuffers (of the same amount
 *             of samples) should be considered incomplete
 *             (GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE)
 */
class MultisampleTextureDependenciesFBOIncompleteness4Test : public glcts::TestCase
{
public:
	/* Public methods */
	MultisampleTextureDependenciesFBOIncompleteness4Test(Context& context);

	virtual void						 deinit();
	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private variables */
	glw::GLuint fbo_id;
	glw::GLuint rbo_id;
	glw::GLuint to_id_2d_multisample_array_color;
	glw::GLuint to_id_2d_multisample_color;
};

/** Test case: FBOs with renderbuffer and multisample texture attachments, where amount
 *             of samples used for multisample texture attachments differs from the
 *             amount of samples used for renderbuffer attachments, should be considered
 *             incomplete (GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE)
 */
class MultisampleTextureDependenciesFBOIncompleteness5Test : public glcts::TestCase
{
public:
	/* Public methods */
	MultisampleTextureDependenciesFBOIncompleteness5Test(Context& context);

	virtual void						 deinit();
	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private variables */
	glw::GLuint fbo_id;
	glw::GLuint rbo_id;
	glw::GLuint to_id_multisample_2d;
	glw::GLuint to_id_multisample_2d_array;
};

/** Test case: GL_INVALID_OPERATION error is reported if 2D or cube-map texture target is
 *             used with a multisample 2D texture for a glFramebufferTexture2D() call.
 */
class MultisampleTextureDependenciesInvalidFramebufferTexture2DCalls1Test : public glcts::TestCase
{
public:
	/* Public methods */
	MultisampleTextureDependenciesInvalidFramebufferTexture2DCalls1Test(Context& context);

	virtual void						 deinit();
	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private methods */
	void initInternals();

	/* Private variables */
	glw::GLuint fbo_id;
	glw::GLuint to_id;
};

/** Test case: GL_INVALID_VALUE error is reported if a glFramebufferTexture2D() call is
 *             made with level not equal to zero for a 2D multisample texture.
 */
class MultisampleTextureDependenciesInvalidFramebufferTexture2DCalls2Test : public glcts::TestCase
{
public:
	/* Public methods */
	MultisampleTextureDependenciesInvalidFramebufferTexture2DCalls2Test(Context& context);

	virtual void						 deinit();
	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private methods */
	void initInternals();

	/* Private variables */
	glw::GLuint fbo_id;
	glw::GLuint to_id;
};

/** Test case: GL_INVALID_OPERATION error is reported if a multisample 2D texture
 *  is used for a glFramebufferTextureLayer() call.
 */
class MultisampleTextureDependenciesInvalidFramebufferTextureLayerCalls1Test : public glcts::TestCase
{
public:
	/* Public methods */
	MultisampleTextureDependenciesInvalidFramebufferTextureLayerCalls1Test(Context& context);

	virtual void						 deinit();
	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private methods */
	void initInternals();

	/* Private variables */
	glw::GLuint fbo_id;
	glw::GLuint to_id;
};

/** Test case: GL_INVALID_VALUE error is reported if a glFramebufferTextureLayer() call
 *             is made with level exceeding amount of layers defined for a 2D multisample
 *             array texture.
 */
class MultisampleTextureDependenciesInvalidFramebufferTextureLayerCalls2Test : public glcts::TestCase
{
public:
	/* Public methods */
	MultisampleTextureDependenciesInvalidFramebufferTextureLayerCalls2Test(Context& context);

	virtual void						 deinit();
	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private methods */
	void initInternals();

	/* Private variables */
	glw::GLuint fbo_id;
	glw::GLuint to_id;
};

/** Test case: GL_INVALID_OPERATION error is reported for
 *             glRenderbufferStorageMultisample() calls, for which samples argument
 *             exceeds MAX_SAMPLES for non-integer internalformats.
 */
class MultisampleTextureDependenciesInvalidRenderbufferStorageMultisampleCalls1Test : public glcts::TestCase
{
public:
	/* Public methods */
	MultisampleTextureDependenciesInvalidRenderbufferStorageMultisampleCalls1Test(Context& context);

	virtual void						 deinit();
	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private methods */
	void initInternals();

	/* Private variables */
	glw::GLuint rbo_id;
};

/** Test case: GL_INVALID_OPERATION error is reported for
 *             glRenderbufferStorageMultisample() calls, for which samples argument
 *             exceeds MAX_INTEGER_SAMPLES for integer internalformats.
 */
class MultisampleTextureDependenciesInvalidRenderbufferStorageMultisampleCalls2Test : public glcts::TestCase
{
public:
	/* Public methods */
	MultisampleTextureDependenciesInvalidRenderbufferStorageMultisampleCalls2Test(Context& context);

	virtual void						 deinit();
	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private methods */
	void initInternals();

	/* Private variables */
	glw::GLuint rbo_id;
};

/** Test case: No error is reported by glFramebufferTexture2D() calls, for
 *             which GL_TEXTURE_2D_MULTISAMPLE texture target is used.
 */
class MultisampleTextureDependenciesNoErrorGeneratedForValidFramebufferTexture2DCallsTest : public glcts::TestCase
{
public:
	/* Public methods */
	MultisampleTextureDependenciesNoErrorGeneratedForValidFramebufferTexture2DCallsTest(Context& context);

	virtual void						 deinit();
	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private methods */
	void initInternals();

	/* Private variables */
	glw::GLuint fbo_id;
	glw::GLuint to_id;
};

/** Test case: No error is reported for glRenderbufferStorageMultisample() calls, for
 *             which samples argument is in 0 < samples <= GL_MAX_SAMPLES range for
 *             non-integer internalformats, and within 0 < samples <= GL_MAX_INTEGER_SAMPLES
 *             for integer internalformats.
 */
class MultisampleTextureDependenciesNoErrorGeneratedForValidRenderbufferStorageMultisampleCallsTest
	: public glcts::TestCase
{
public:
	/* Public methods */
	MultisampleTextureDependenciesNoErrorGeneratedForValidRenderbufferStorageMultisampleCallsTest(Context& context);

	virtual void						 deinit();
	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private methods */
	void initInternals();

	/* Private variables */
	glw::GLuint rbo_id;
};

/** Test case: When used against multisample texture targets, glTexParameter*() should not generate
 *             any error if GL_TEXTURE_BASE_LEVEL is set to 0. Using any other value should generate
 *             GL_INVALID_OPERATION.
 *             Modifying sampler states should generate GL_INVALID_ENUM.
 */
class MultisampleTextureDependenciesTexParameterTest : public glcts::TestCase
{
public:
	/* Public methods */
	MultisampleTextureDependenciesTexParameterTest(Context& context);

	virtual void						 deinit();
	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private methods */
	void checkAllTexParameterInvocations(glw::GLenum expected_error_code, glw::GLint value, glw::GLenum pname,
										 glw::GLenum texture_target);

	/* Private variables */
	glw::GLuint to_id_multisample_2d;
	glw::GLuint to_id_multisample_2d_array;
};
} /* glcts namespace */

#endif // _ES31CTEXTURESTORAGEMULTISAMPLEDEPENDENCIESTESTS_HPP
