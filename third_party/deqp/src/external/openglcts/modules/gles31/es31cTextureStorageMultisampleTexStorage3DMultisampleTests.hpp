#ifndef _ES31CTEXTURESTORAGEMULTISAMPLETEXSTORAGE3DMULTISAMPLETESTS_HPP
#define _ES31CTEXTURESTORAGEMULTISAMPLETEXSTORAGE3DMULTISAMPLETESTS_HPP
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
 * \file  es31cTextureStorageMultisampleTexStorage3DMultisampleTests.hpp
 * \brief Declares test classes for glTexStorage3DMultisampleOES() conformance
 *        tests. (ES3.1 only)
 */ /*-------------------------------------------------------------------*/

#include "es31cTextureStorageMultisampleTests.hpp"

namespace glcts
{
/** Test case: Invalid multisample texture sizes are rejected with GL_INVALID_VALUE error;
 *             border cases are correctly accepted. */
class InvalidTextureSizesAreRejectedValidAreAcceptedTest : public glcts::TestCase
{
public:
	/* Public methods */
	InvalidTextureSizesAreRejectedValidAreAcceptedTest(Context& context);

	virtual void						 deinit();
	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private methods */
	void initInternals();

	/* Private variables */
	glw::GLboolean gl_oes_texture_multisample_2d_array_supported;
	glw::GLint	 max_texture_size;
	glw::GLint	 max_array_texture_layers;
	glw::GLuint	to_id_2d_array_1;
	glw::GLuint	to_id_2d_array_2;
	glw::GLuint	to_id_2d_array_3;
};

/** Test case: zero sample requests are rejected by generating glTexStorage3DMultisampleOES() with GL_INVALID_VALUE error */
class MultisampleTextureTexStorage3DZeroSampleTest : public glcts::TestCase
{
public:
	/* Public methods */
	MultisampleTextureTexStorage3DZeroSampleTest(Context& context);

	virtual void						 deinit();
	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private methods */
	void initInternals();

	/* Private variables */
	glw::GLboolean gl_oes_texture_multisample_2d_array_supported;
	glw::GLuint	to_id;
};

/** Test case: Requests to set up multisample textures for internal formats that are not color-renderable,
 *             depth-renderable nor stencil-renderable result in GL_INVALID_ENUM error. */
class NonColorDepthStencilRenderableInternalformatsAreRejectedTest : public glcts::TestCase
{
public:
	/* Public methods */
	NonColorDepthStencilRenderableInternalformatsAreRejectedTest(Context& context);

	virtual void						 deinit();
	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private methods */
	void initInternals();

	/* Private variables */
	glw::GLboolean gl_oes_texture_multisample_2d_array_supported;
	glw::GLuint	to_id;
};

/** Test case: Requests to set up multisample color textures with unsupported number of samples
 *             are rejected with GL_INVALID_OPERATION error. */
class RequestsToSetUpMultisampleColorTexturesWithUnsupportedNumberOfSamplesAreRejectedTest : public glcts::TestCase
{
public:
	/* Public methods */
	RequestsToSetUpMultisampleColorTexturesWithUnsupportedNumberOfSamplesAreRejectedTest(Context& context);

	virtual void						 deinit();
	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private methods */
	void initInternals();

	/* Private variables */
	glw::GLboolean gl_oes_texture_multisample_2d_array_supported;
	glw::GLuint	to_id;
};

/** Test case: Requests to set up multisample depth textures with unsupported number of samples
 *             are rejected with GL_INVALID_OPERATION error. */
class RequestsToSetUpMultisampleDepthTexturesWithUnsupportedNumberOfSamplesAreRejectedTest : public glcts::TestCase
{
public:
	/* Public methods */
	RequestsToSetUpMultisampleDepthTexturesWithUnsupportedNumberOfSamplesAreRejectedTest(Context& context);

	virtual void						 deinit();
	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private methods */
	void initInternals();

	/* Private variables */
	glw::GLboolean gl_oes_texture_multisample_2d_array_supported;
	glw::GLuint	to_id;
};

/** Test case: Requests to set up multisample stencil textures with unsupported number of samples
 *             are rejected with GL_INVALID_OPERATION error. */
class RequestsToSetUpMultisampleStencilTexturesWithUnsupportedNumberOfSamplesAreRejectedTest : public glcts::TestCase
{
public:
	/* Public methods */
	RequestsToSetUpMultisampleStencilTexturesWithUnsupportedNumberOfSamplesAreRejectedTest(Context& context);

	virtual void						 deinit();
	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private methods */
	void initInternals();

	/* Private variables */
	glw::GLboolean gl_oes_texture_multisample_2d_array_supported;
	glw::GLuint	to_id;
};

/** Test case: Requests to set up multisample textures using exactly the number of samples reported
 *            for a particular internal format by glGetI in an GL_INVALID_OPERATION error. */
class RequestsToSetUpMultisampleTexturesWithValidAndInvalidNumberOfSamplesTest : public glcts::TestCase
{
public:
	/* Public methods */
	RequestsToSetUpMultisampleTexturesWithValidAndInvalidNumberOfSamplesTest(Context& context);

	virtual void						 deinit();
	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private functions and variables */
	void createAssets();
	void releaseAssets();

	glw::GLboolean gl_oes_texture_multisample_2d_array_supported;
	glw::GLuint	to_id;
};

/** Test case: GL_TEXTURE_2D_MULTISAMPLE texture target is rejected by generating GL_INVALID_ENUM error. */
class Texture2DMultisampleTargetIsRejectedTest : public glcts::TestCase
{
public:
	/* Public methods */
	Texture2DMultisampleTargetIsRejectedTest(Context& context);

	virtual void						 deinit();
	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private methods */
	void initInternals();

	/* Private variables */
	glw::GLboolean gl_oes_texture_multisample_2d_array_supported;
	glw::GLuint	to_id;
};

/** Test case: Requests to set up multisample color/depth/stencil textures
 *             with disabled/enabled fixed sample locations and
 *             valid internalformats are accepted. */
class ValidInternalformatAndSamplesValuesAreAcceptedTest : public glcts::TestCase
{
public:
	/* Public methods */
	ValidInternalformatAndSamplesValuesAreAcceptedTest(Context& context);

	virtual void						 deinit();
	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private methods */
	void initInternals();

	/* Private variables */
	glw::GLboolean gl_oes_texture_multisample_2d_array_supported;
	glw::GLuint	to_id;
};
} /* glcts namespace */

#endif // _ES31CTEXTURESTORAGEMULTISAMPLETEXSTORAGE3DMULTISAMPLETESTS_HPP
