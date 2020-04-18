#ifndef _ES31CTEXTURESTORAGEMULTISAMPLEGLCOVERAGETESTS_HPP
#define _ES31CTEXTURESTORAGEMULTISAMPLEGLCOVERAGETESTS_HPP
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
 * \file  es31cTextureStorageMultisampleGLCoverageTests.hpp
 * \brief Declares test classes for coverage conformance tests for
 *        multisample texture functionality (ES3.1 only)
 */ /*-------------------------------------------------------------------*/

#include "es31cTextureStorageMultisampleTests.hpp"

namespace glcts
{
/** Test case: GL_MAX_SAMPLE_MASK_WORDS, GL_MAX_COLOR_TEXTURE_SAMPLES,
 *             GL_MAX_DEPTH_TEXTURE_SAMPLES, GL_MAX_INTEGER_SAMPLES,
 *             GL_TEXTURE_BINDING_2D_MULTISAMPLE and GL_TEXTURE_BINDING_2D_MULTISAMPLE_ARRAY_OES
 *             are recognized by glGet*() functions.
 */
class GLCoverageExtensionSpecificEnumsAreRecognizedTest : public glcts::TestCase
{
public:
	/* Public methods */
	GLCoverageExtensionSpecificEnumsAreRecognizedTest(Context& context);

	virtual void						 deinit();
	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private methods */

	/* Private fields */
	glw::GLboolean gl_oes_texture_storage_multisample_2d_array_supported;
	glw::GLuint	to_id_2d_multisample;
	glw::GLuint	to_id_2d_multisample_array;
};

/** Test case: glGetTexParameter*() should accept GL_TEXTURE_2D_MULTISAMPLE
 and GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES texture targets. Default
 state information should be as per-spec for both targets.
 */
class GLCoverageGLGetTexParameterReportsCorrectDefaultValuesForMultisampleTextureTargets : public glcts::TestCase
{
public:
	/* Public methods */
	GLCoverageGLGetTexParameterReportsCorrectDefaultValuesForMultisampleTextureTargets(Context& context);

	virtual void						 deinit();
	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private methods */
	glw::GLboolean gl_oes_texture_storage_multisample_2d_array_supported;
	glw::GLuint	to_id_2d;
	glw::GLuint	to_id_2d_array;
};

/** Test case: Checks disabled/enabled status of GL_SAMPLE_MASK mode is reported correctly.
 */
class GLCoverageGLSampleMaskModeStatusIsReportedCorrectlyTest : public glcts::TestCase
{
public:
	/* Public methods */
	GLCoverageGLSampleMaskModeStatusIsReportedCorrectlyTest(Context& context);

	virtual tcu::TestNode::IterateResult iterate();
};

/** Test case: glTexParameter*() should not generate an error if application
 *             attempts to set zero texture mipmap base level for
 *             GL_TEXTURE_2D_MULTISAMPLE and
 *             GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES
 *             texture targets.
 */
class GLCoverageGLTexParameterHandlersAcceptZeroBaseLevelForExtensionSpecificTextureTargetsTest : public glcts::TestCase
{
public:
	/* Public methods */
	GLCoverageGLTexParameterHandlersAcceptZeroBaseLevelForExtensionSpecificTextureTargetsTest(Context& context);

	virtual void						 deinit();
	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private methods */
	void initInternals();

	/* Private variables */
	bool		are_2d_array_multisample_tos_supported;
	glw::GLuint to_id_2d;
	glw::GLuint to_id_2d_array;
};
} /* glcts namespace */

#endif // _ES31CTEXTURESTORAGEMULTISAMPLEGLCOVERAGETESTS_HPP
