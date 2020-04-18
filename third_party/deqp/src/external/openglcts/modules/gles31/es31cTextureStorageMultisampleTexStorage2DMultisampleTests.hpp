#ifndef _ES31CTEXTURESTORAGEMULTISAMPLETEXSTORAGE2DMULTISAMPLETESTS_HPP
#define _ES31CTEXTURESTORAGEMULTISAMPLETEXSTORAGE2DMULTISAMPLETESTS_HPP
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
 * \file  es31cTextureStorageMultisampleTexStorage2DMultisampleTests.hpp
 * \brief Declares test classes for glTexStorage2DMultisample() conformance
 *        tests. (ES3.1 only)
 */ /*-------------------------------------------------------------------*/

#include "es31cTextureStorageMultisampleTests.hpp"

namespace glcts
{
/** Test case: glTexStorage2DMultisample() requests to set up multisample textures using exactly the number of samples reported for a particular
 *  internal format by glGetInternalformativ() function should succeed. Using larger values should result in an GL_INVALID_OPERATION error. */
class MultisampleTextureTexStorage2DGeneralSamplesNumberTest : public glcts::TestCase
{
public:
	/* Public methods */
	MultisampleTextureTexStorage2DGeneralSamplesNumberTest(Context& context);

	virtual void						 deinit();
	virtual void						 deinitInternalIteration();
	virtual void						 initInternalIteration();
	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private methods and variables */
	glw::GLuint to_id;
};

/** Test case: Invalid multisample texture sizes are rejected; border cases are correctly accepted. */
class MultisampleTextureTexStorage2DInvalidAndBorderCaseTextureSizesTest : public glcts::TestCase
{
public:
	/* Public methods */
	MultisampleTextureTexStorage2DInvalidAndBorderCaseTextureSizesTest(Context& context);

	virtual void						 deinit();
	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private methods */
	void initInternals();

	/* Private variables */
	glw::GLuint to_id;
};

/** Test case: Requests to set up multisample textures for internal formats that are not color-renderable, depth-renderable and stencil-renderable result in GL_INVALID_ENUM error */
class MultisampleTextureTexStorage2DNonColorDepthOrStencilInternalFormatsTest : public glcts::TestCase
{
public:
	/* Public methods */
	MultisampleTextureTexStorage2DNonColorDepthOrStencilInternalFormatsTest(Context& context);

	virtual void						 deinit();
	virtual void						 deinitInternalIteration();
	virtual void						 initInternalIteration();
	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private variables */
	glw::GLuint to_id;
};

/** Test case: reconfiguration of immutable 2D texture storage is forbidden and should result in GL_INVALID_OPERATION error. */
class MultisampleTextureTexStorage2DReconfigurationRejectedTest : public glcts::TestCase
{
public:
	/* Public methods */
	MultisampleTextureTexStorage2DReconfigurationRejectedTest(Context& context);

	virtual void deinit();
	virtual void deinitTexture(glw::GLuint& to_id, glw::GLenum texture_target);
	virtual void initTexture(glw::GLuint& to_id, glw::GLenum texture_target);
	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private methods */
	void initInternals();

	/* Private variables */
	glw::GLboolean gl_oes_texture_storage_multisample_2d_array_supported;
	glw::GLuint	to_id_2d;
	glw::GLuint	to_id_2d_array;
};

/** Test case: GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES  texture target is rejected and GL_INVALID_ENUM error code set */
class MultisampleTextureTexStorage2DTexture2DMultisampleArrayTest : public glcts::TestCase
{
public:
	/* Public methods */
	MultisampleTextureTexStorage2DTexture2DMultisampleArrayTest(Context& context);

	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private methods */
	void initInternals();
};

/** Test case: Requests to set up multisample color textures with unsupported number of samples are rejected with GL_INVALID_OPERATION error */
class MultisampleTextureTexStorage2DUnsupportedSamplesCountForColorTexturesTest : public glcts::TestCase
{
public:
	/* Public methods */
	MultisampleTextureTexStorage2DUnsupportedSamplesCountForColorTexturesTest(Context& context);

	virtual void						 deinit();
	virtual void						 deinitInternalIteration();
	virtual void						 initInternalIteration();
	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private variables */
	glw::GLuint to_id;
};

/** Test case: Requests to set up multisample depth textures with unsupported number of samples are rejected with GL_INVALID_OPERATION error */
class MultisampleTextureTexStorage2DUnsupportedSamplesCountForDepthTexturesTest : public glcts::TestCase
{
public:
	/* Public methods */
	MultisampleTextureTexStorage2DUnsupportedSamplesCountForDepthTexturesTest(Context& context);

	virtual void						 deinit();
	virtual void						 deinitInternalIteration();
	virtual void						 initInternalIteration();
	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private variables */
	glw::GLuint to_id;
};

/** Test case: Requests to set up multisample depth+stencil textures with unsupported number of samples are rejected with GL_INVALID_OPERATION error */
class MultisampleTextureTexStorage2DUnsupportedSamplesCountForDepthStencilTexturesTest : public glcts::TestCase
{
public:
	/* Public methods */
	MultisampleTextureTexStorage2DUnsupportedSamplesCountForDepthStencilTexturesTest(Context& context);

	virtual void						 deinit();
	virtual void						 deinitInternalIteration();
	virtual void						 initInternalIteration();
	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private variables */
	glw::GLuint to_id;
};

/** Test case: Requests to set up multisample color/depth/stencil textures are accepted. */
class MultisampleTextureTexStorage2DValidCallsTest : public glcts::TestCase
{
public:
	/* Public methods */
	MultisampleTextureTexStorage2DValidCallsTest(Context& context);

	virtual void						 deinit();
	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private methods */
	void initInternals();

	/* Private variables */
	glw::GLuint to_id;
};

/** Test case: zero sample requests are rejected by generating a GL_INVALID_VALUE error. */
class MultisampleTextureTexStorage2DZeroSampleTest : public glcts::TestCase
{
public:
	/* Public methods */
	MultisampleTextureTexStorage2DZeroSampleTest(Context& context);

	virtual void						 deinit();
	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private methods */
	void initInternals();

	/* Private variables */
	glw::GLuint to_id;
};

} /* glcts namespace */

#endif // _ES31CTEXTURESTORAGEMULTISAMPLETEXSTORAGE2DMULTISAMPLETESTS_HPP
