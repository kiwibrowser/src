#ifndef _ES31CTEXTURESTORAGEMULTISAMPLEGETACTIVEUNIFORMTESTS_HPP
#define _ES31CTEXTURESTORAGEMULTISAMPLEGETACTIVEUNIFORMTESTS_HPP
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
 * \file  es31cTextureStorageMultisampleGetActiveUniformTests.hpp
 * \brief Declares test classes for testing glGetActiveUniform() interactions with
 *        glGetActiveUniform(). (ES3.1 only)
 */ /*-------------------------------------------------------------------*/

#include "es31cTextureStorageMultisampleTests.hpp"

namespace glcts
{
/** Test case: glGetActiveUniform(): Make sure multisample texture uniform types are
 *             recognized by ESSL compiler and reported correctly
 *             by glGetActiveUniform().
 **/
class MultisampleTextureGetActiveUniformSamplersTest : public glcts::TestCase
{
public:
	/* Public methods */
	MultisampleTextureGetActiveUniformSamplersTest(Context& context);

	virtual void						 deinit();
	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private methods */
	void initInternals();
	void deinitInternals();

	/* Private variables */
	glw::GLint	 fs_id;
	glw::GLboolean gl_oes_texture_storage_multisample_2d_array_supported;
	glw::GLint	 po_id;
	glw::GLint	 vs_id;

	static const char* fs_body;
	static const char* fs_body_oes;
	static const char* vs_body;
	static const char* vs_body_oes;
};
} /* glcts namespace */

#endif // _ES31CTEXTURESTORAGEMULTISAMPLEGETACTIVEUNIFORMTESTS_HPP
