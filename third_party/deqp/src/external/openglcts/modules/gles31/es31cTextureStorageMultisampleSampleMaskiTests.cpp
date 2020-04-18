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
 * \file  es31cTextureStorageMultisampleSampleMaskiTests.cpp
 * \brief Implements conformance tests for glSampleMaski() (ES3.1 only)
 */ /*-------------------------------------------------------------------*/

#include "es31cTextureStorageMultisampleSampleMaskiTests.hpp"
#include "gluContextInfo.hpp"
#include "gluDefs.hpp"
#include "glwEnums.hpp"
#include "glwFunctions.hpp"
#include "tcuRenderTarget.hpp"
#include "tcuTestLog.hpp"

#include <string>
#include <vector>

namespace glcts
{
/** Constructor.
 *
 *  @param context Rendering context handle.
 **/
MultisampleTextureSampleMaskiIndexLowerThanGLMaxSampleMaskWordsTest::
	MultisampleTextureSampleMaskiIndexLowerThanGLMaxSampleMaskWordsTest(Context& context)
	: TestCase(context, "multisample_texture_sample_maski_index_lower_than_gl_max_sample_mask_words",
			   "Verifies glSampleMaski() correctly accepts index arguments up to GL_MAX_SAMPLE_MASK_WORDS-1 value")
{
	/* Left blank on purpose */
}

/** Executes test iteration.
 *
 *  @return Returns STOP when test has finished executing, CONTINUE if more iterations are needed.
 */
tcu::TestNode::IterateResult MultisampleTextureSampleMaskiIndexLowerThanGLMaxSampleMaskWordsTest::iterate()
{
	/* Get GL_MAX_SAMPLE_MASK_WORDS value */
	const glw::Functions& gl							 = m_context.getRenderContext().getFunctions();
	glw::GLint			  gl_max_sample_mask_words_value = 0;

	gl.getIntegerv(GL_MAX_SAMPLE_MASK_WORDS, &gl_max_sample_mask_words_value);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Failed to retrieve GL_MAX_SAMPLE_MASK_WORDS value");

	/* Issue the calls */
	for (int sample_mask = 0; sample_mask < gl_max_sample_mask_words_value; ++sample_mask)
	{
		gl.sampleMaski(sample_mask, 0);

		GLU_EXPECT_NO_ERROR(gl.getError(), "An error was reported despite a valid glSampleMaski() call.");
	}

	/* Test case passed */
	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

	return STOP;
}

/** Constructor.
 *
 *  @param context Rendering context handle.
 **/
MultisampleTextureSampleMaskiIndexEqualToGLMaxSampleMaskWordsTest::
	MultisampleTextureSampleMaskiIndexEqualToGLMaxSampleMaskWordsTest(Context& context)
	: TestCase(context, "multisample_texture_sample_maski_index_equal_gl_max_sample_mask_words",
			   "Verifies glSampleMaski() rejects index equal to GL_MAX_SAMPLE_MASK_WORDS value")
{
	/* Left blank on purpose */
}

/** Executes test iteration.
 *
 *  @return Returns STOP when test has finished executing, CONTINUE if more iterations are needed.
 */
tcu::TestNode::IterateResult MultisampleTextureSampleMaskiIndexEqualToGLMaxSampleMaskWordsTest::iterate()
{
	/* Get GL_MAX_SAMPLE_MASK_WORDS value */
	const glw::Functions& gl							 = m_context.getRenderContext().getFunctions();
	glw::GLint			  gl_max_sample_mask_words_value = 0;

	gl.getIntegerv(GL_MAX_SAMPLE_MASK_WORDS, &gl_max_sample_mask_words_value);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Failed to retrieve GL_MAX_SAMPLE_MASK_WORDS value");

	/* Issue call with valid parameters, but invalid index equal to GL_MAX_SAMPLE_MASK_WORDS value */
	gl.sampleMaski(gl_max_sample_mask_words_value, 0);

	if (gl.getError() != GL_INVALID_VALUE)
	{
		TCU_FAIL("Invalid error code reported");
	}

	/* Test case passed */
	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

	return STOP;
}

/** Constructor.
 *
 *  @param context Rendering context handle.
 **/
MultisampleTextureSampleMaskiGettersTest::MultisampleTextureSampleMaskiGettersTest(Context& context)
	: TestCase(context, "multisample_texture_sample_maski_getters",
			   "Verifies valid glSampleMaski() calls modify GL_SAMPLE_MASK_VALUE "
			   "property value reported by glGetIntegeri_v()")
{
	/* Left blank on purpose */
}

/** Executes test iteration.
 *
 *  @return Returns STOP when test has finished executing, CONTINUE if more iterations are needed.
 */
tcu::TestNode::IterateResult MultisampleTextureSampleMaskiGettersTest::iterate()
{
	/* Get GL_MAX_SAMPLE_MASK_WORDS value */
	const glw::Functions& gl							 = m_context.getRenderContext().getFunctions();
	glw::GLint			  gl_max_sample_mask_words_value = 0;

	gl.getIntegerv(GL_MAX_SAMPLE_MASK_WORDS, &gl_max_sample_mask_words_value);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Failed to retrieve GL_MAX_SAMPLE_MASK_WORDS value");

	/* Iterate over valid index & mask values */
	const glw::GLuint  valid_masks[] = { 0, 0xFFFFFFFF, 0xFF000000, 0x00FF0000, 0x0000FF00, 0x000000FF };
	const unsigned int n_valid_masks = sizeof(valid_masks) / sizeof(valid_masks[0]);

	for (int index = 0; index < gl_max_sample_mask_words_value; ++index)
	{
		for (unsigned int n_mask = 0; n_mask < n_valid_masks; ++n_mask)
		{
			glw::GLint mask = valid_masks[n_mask];

			/* Make sure a valid glSampleMaski() call does not result in an error */
			gl.sampleMaski(index, mask);

			GLU_EXPECT_NO_ERROR(gl.getError(), "A valid glSampleMaski() call resulted in an error");

			/* Check the property value as reported by implementation */
			glw::GLint int_value = -1;

			gl.getIntegeri_v(GL_SAMPLE_MASK_VALUE, index, &int_value);
			GLU_EXPECT_NO_ERROR(gl.getError(), "A valid glGetIntegeri_v() call resulted in an error");

			if (int_value != mask)
			{
				TCU_FAIL("Invalid sample mask reported");
			}
		} /* for (all masks) */
	}	 /* for (all valid index argument values) */

	/* Test case passed */
	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

	return STOP;
}

/** Constructor.
 *
 *  @param context Rendering context handle.
 **/
MultisampleTextureSampleMaskiIndexGreaterGLMaxSampleMaskWordsTest::
	MultisampleTextureSampleMaskiIndexGreaterGLMaxSampleMaskWordsTest(Context& context)
	: TestCase(context, "multisample_texture_sample_maski_index_greater_gl_max_sample_mask_words",
			   "Verifies glSampleMaski() rejects index greater than GL_MAX_SAMPLE_MASK_WORDS value")
{
	/* Left blank on purpose */
}

/** Executes test iteration.
 *
 *  @return Returns STOP when test has finished executing, CONTINUE if more iterations are needed.
 */
tcu::TestNode::IterateResult MultisampleTextureSampleMaskiIndexGreaterGLMaxSampleMaskWordsTest::iterate()
{
	/* Get GL_MAX_SAMPLE_MASK_WORDS value */
	const glw::Functions& gl							 = m_context.getRenderContext().getFunctions();
	glw::GLint			  gl_max_sample_mask_words_value = 0;

	gl.getIntegerv(GL_MAX_SAMPLE_MASK_WORDS, &gl_max_sample_mask_words_value);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Failed to retrieve GL_MAX_SAMPLE_MASK_WORDS value");

	/* Issue call with valid parameters, but invalid index greater than GL_MAX_SAMPLE_MASK_WORDS value */
	gl.sampleMaski(gl_max_sample_mask_words_value + 1, 0);

	if (gl.getError() != GL_INVALID_VALUE)
	{
		TCU_FAIL("Invalid error code reported");
	}

	/* Test case passed */
	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

	return STOP;
}
} /* glcts namespace */
