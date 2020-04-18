#ifndef _GL4CSHADERTEXTUREIMAGESAMPLESTESTS_HPP
#define _GL4CSHADERTEXTUREIMAGESAMPLESTESTS_HPP
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
 * \file  gl4cShaderTextureImageSamples.hpp
 * \brief Declares test classes that verify conformance of the
 *        GL implementation with GL_ARB_shader_texture_image_samples
 *        extension specification.
 */ /*-------------------------------------------------------------------*/

#include "glcTestCase.hpp"
#include "glwDefs.hpp"

namespace glcts
{
/** Base test class for GL_ARB_shader_texture_image_samples conformance tests. */
class ShaderTextureImageSamplesTestBase : public deqp::TestCase
{
public:
	/* Public methods */
	ShaderTextureImageSamplesTestBase(deqp::Context& context, const char* name, const char* description);

	void		 deinit();
	virtual void init();

protected:
	/* Protected type definitions */
	typedef enum {
		SAMPLER_TYPE_ISAMPLER2DMS,
		SAMPLER_TYPE_ISAMPLER2DMSARRAY,
		SAMPLER_TYPE_SAMPLER2DMS,
		SAMPLER_TYPE_SAMPLER2DMSARRAY,
		SAMPLER_TYPE_USAMPLER2DMS,
		SAMPLER_TYPE_USAMPLER2DMSARRAY,

		SAMPLER_TYPE_COUNT
	} _sampler_type;

	typedef enum {
		TEST_TYPE_IMAGE,
		TEST_TYPE_TEXTURE,

		TEST_TYPE_COUNT
	} _test_type;

	/* Protected methods */
	bool buildComputeProgram(const char* cs_body, bool should_link_po, bool should_succeed);

	void executeFunctionalTest(const _sampler_type& sampler_type, const _test_type& test_type);

private:
	/* Private methods */
	void deinitProgramAndShaderObjects();

	/* Private members */
	glw::GLint  m_internalformat_n_samples_count;
	glw::GLint* m_internalformat_n_samples_data;

	glw::GLuint m_bo_id;
	glw::GLuint m_cs_id;
	glw::GLuint m_po_id;
	glw::GLuint m_to_id;

	const unsigned int m_to_depth;
	const unsigned int m_to_height;
	const unsigned int m_to_width;
};

/** Implements the following functional tests from CTS_ARB_shader_texture_image_samples:
 *
 *  Basic Texture Functionality
 *
 *  * Create a multisample texture and use a compute shader with an SSBO
 *    to verify the result of calling textureSamples() on that texture. The
 *    compute shader would look like this:
 *
 *      buffer Result {
 *          int samples;
 *      }
 *      uniform gsampler% tex;
 *      void main() {
 *          samples = textureSamples(tex);
 *      }
 *
 *      For gsampler% the test should iterate through all the texture targets;
 *      sampler*, isampler* and usamples* for 2DMS and 2DMSArray. For the
 *      2SMSArray case a multisample array texture should be bound to the
 *      texture unit.
 *
 *      Readback the SSBO "samples" result and verify that it matches the
 *      <samples> parameter used to create the texture. Repeat the test for
 *      all <samples> values supported by the implementation.
 *
 *  Basic Image Functionality
 *
 *  * Similar to the test above, but bind an image from the multisample
 *    texture:
 *
 *    Create a multisample texture, bind an image from the texture and use
 *    a compute shader with an SSBO to verify the result of calling
 *    imageSamples() on that image. The compute shader would look like this:
 *
 *      buffer Result {
 *          int samples;
 *      }
 *      layout(format) uniform gimage% tex;
 *      void main() {
 *          samples = imageSamples(tex);
 *      }
 *
 *      For gimage% the test should iterate through all the image targets;
 *      image*, iimage* and uimage* for 2DMS and 2DMSArray. For the
 *      2SMSArray case a multisample array image should be bound to the
 *      image unit.
 *
 *      Readback the SSBO "samples" result and verify that it matches the
 *      <samples> parameter used to create the texture. Repeat the test for
 *      all <samples> values supported by the implementation.
 *
 **/
class ShaderTextureImageSampleFunctionalTest : public ShaderTextureImageSamplesTestBase
{
public:
	/* Public methods */
	ShaderTextureImageSampleFunctionalTest(deqp::Context& context, const char* name);
	void init();

	virtual tcu::TestNode::IterateResult iterate();

private:
	_test_type type_to_test;
	/* Private methods */
};

/** Implements the following GLSL #extension test from CTS_ARB_shader_texture_image_samples:
 *
 *  * Verify a shader with #extension GL_ARB_shader_texture_image_samples : enable
 *    defines the pre-processor macro "GL_ARB_shader_texture_image_samples"
 *    to the value "1".
 *
 **/
class ShaderTextureImageSamplesGLSLExtensionEnableTest : public ShaderTextureImageSamplesTestBase
{
public:
	/* Public methods */
	ShaderTextureImageSamplesGLSLExtensionEnableTest(deqp::Context& context);

	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private methods */
};

/** Implements the following GLSL #extension test from CTS_ARB_shader_texture_image_samples:
 *
 *  * Verify a shader with
 *    #extension GL_ARB_shader_texture_image_samples : require
 *    compiles without error.
 *
 **/
class ShaderTextureImageSamplesGLSLExtensionRequireTest : public ShaderTextureImageSamplesTestBase
{
public:
	/* Public methods */
	ShaderTextureImageSamplesGLSLExtensionRequireTest(deqp::Context& context);

	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private methods */
};

/** Test group which encapsulates all conformance tests for GL_ARB_shader_texture_image_samples
 *  extension.
 */
class ShaderTextureImageSamplesTests : public deqp::TestCaseGroup
{
public:
	/* Public methods */
	ShaderTextureImageSamplesTests(deqp::Context& context);

	void init(void);

private:
	ShaderTextureImageSamplesTests(const ShaderTextureImageSamplesTests& other);
	ShaderTextureImageSamplesTests& operator=(const ShaderTextureImageSamplesTests& other);
};

} /* glcts namespace */

#endif // _GL4CSHADERTEXTUREIMAGESAMPLESTESTS_HPP
