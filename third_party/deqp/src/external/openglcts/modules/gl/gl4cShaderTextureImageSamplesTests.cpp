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
 * \file  gl4cShaderTextureImageSamplesTests.cpp
 * \brief Implements conformance tests for GL_ARB_shader_texture_image_samples functionality
 */ /*-------------------------------------------------------------------*/

#include "gl4cShaderTextureImageSamplesTests.hpp"
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
 *  @param context     Rendering context
 *  @param name        Test name
 *  @param description Test description
 */
ShaderTextureImageSamplesTestBase::ShaderTextureImageSamplesTestBase(deqp::Context& context, const char* name,
																	 const char* description)
	: deqp::TestCase(context, name, description)
	, m_internalformat_n_samples_count(0)
	, m_internalformat_n_samples_data(DE_NULL)
	, m_bo_id(0)
	, m_cs_id(0)
	, m_po_id(0)
	, m_to_id(0)
	, m_to_depth(3)
	, m_to_height(4)
	, m_to_width(8)
{
	/* Left blank intentionally */
}

/** Compiles a compute shader and optionally attaches it to a program object, which the
 *  method can then try to link.
 *
 *  Shader object ID will be stored in m_cs_id.
 *  Program object ID will be stored in m_po_id.
 *
 *  @param cs_body        Source code of the compute shader to use for compilation.
 *  @param should_link_po true if the method should also attempt to link the program object.
 *  @param should_succeed true if the compilation and linking (depending on @param should_link_po)
 *                        processes should succeed, false otherwise. If GL implementation
 *                        is found not to accept a valid compute shader, the method will
 *                        throw a TestError exception.
 *
 *   Upon failure, the method *does not* delete the program & shader objects.
 *
 *  @return true if the compilation / linking process was successful, false otherwise.
 **/
bool ShaderTextureImageSamplesTestBase::buildComputeProgram(const char* cs_body, bool should_link_po,
															bool should_succeed)
{
	bool result = false;

	/* Deinitialize any program / shader objects that may have already been initialized
	 * for this test instance.
	 */
	deinitProgramAndShaderObjects();

	/* Create the objects */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	m_cs_id = gl.createShader(GL_COMPUTE_SHADER);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateShader() call failed.");

	m_po_id = gl.createProgram();
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateProgram() call failed.");

	/* Set up the shader object */
	gl.shaderSource(m_cs_id, 1,			/* count */
					&cs_body, DE_NULL); /* length */
	GLU_EXPECT_NO_ERROR(gl.getError(), "glShaderSource() call failed.");

	/* Compile the shader object */
	glw::GLint compile_status = GL_FALSE;

	gl.compileShader(m_cs_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCompileShader() call failed.");

	gl.getShaderiv(m_cs_id, GL_COMPILE_STATUS, &compile_status);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetShaderiv() call failed.");

	if (compile_status != GL_TRUE)
	{
		if (should_succeed)
		{
			TCU_FAIL("Shader compilation failed.");
		}

		goto end;
	}
	else
	{
		if (!should_succeed)
		{
			TCU_FAIL("Shader compilation has succeeded, even though it should not have.");
		}
	}

	if (should_link_po)
	{
		/* Prepare the program object for linking */
		gl.attachShader(m_po_id, m_cs_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glAttachShader() call failed.");

		/* Link the program object */
		glw::GLint link_status = GL_FALSE;

		gl.linkProgram(m_po_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glLinkProgram() call failed.");

		gl.getProgramiv(m_po_id, GL_LINK_STATUS, &link_status);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGetProgramiv() call failed.");

		if (link_status != GL_TRUE)
		{
			if (should_succeed)
			{
				TCU_FAIL("Program linking failed.");
			}

			goto end;
		}
	}

	result = true;
end:
	return result;
}

/** Deinitializes all GL objects that may have been created during test execution. */
void ShaderTextureImageSamplesTestBase::deinit()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	deinitProgramAndShaderObjects();

	if (m_bo_id != 0)
	{
		gl.deleteBuffers(1, &m_bo_id);

		m_bo_id = 0;
	}

	if (m_internalformat_n_samples_data != DE_NULL)
	{
		delete[] m_internalformat_n_samples_data;

		m_internalformat_n_samples_data = DE_NULL;
	}

	if (m_to_id != 0)
	{
		gl.deleteTextures(1, &m_to_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glDeleteTextures() call failed.");

		m_to_id = 0;
	}
}

/** Deinitializes program & shader objects that may have been created
 *  during test execution.
 **/
void ShaderTextureImageSamplesTestBase::deinitProgramAndShaderObjects()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (m_cs_id != 0)
	{
		gl.deleteShader(m_cs_id);

		m_cs_id = 0;
	}

	if (m_po_id != 0)
	{
		gl.deleteProgram(m_po_id);

		m_po_id = 0;
	}
}

/** Executes the functional test as described in CTS_ARB_shader_texture_image_samples.
 *
 *  This method throws a TestError exception if OpenGL implementation reports any
 *  error OR the "samples" value reported by OpenGL is found to be incorrect.
 *
 *  @param sampler_type Tells which sampler type should be used for the test. Note that
 *                      even though sampler type enum values are named after texture samplers,
 *                      the actual ES SL types used during the test will depend on whether
 *                      the caller requested images or textures to be used (see @param test_type)
 *  @param test_type    Tells whether the test should verify image or texture samplers.
 **/
void ShaderTextureImageSamplesTestBase::executeFunctionalTest(const _sampler_type& sampler_type,
															  const _test_type&	test_type)
{
	std::string cs_body;
	const char* cs_template_code = "#version 430\n"
								   "\n"
								   "#extension GL_ARB_shader_texture_image_samples : require\n"
								   "\n"
								   "layout(local_size_x = 1) in;\n"
								   "\n"
								   "buffer Result\n"
								   "{\n"
								   "    int samples;\n"
								   "};\n"
								   "\n"
								   "SAMPLER_TYPE data;\n"
								   "\n"
								   "void main()\n"
								   "{\n"
								   "    samples = SAMPLES_MODIFIER\n"
								   "}\n";

	/* Any program or shader objects lying around at this point? If so, release them
	 * before we proceed.
	 */
	deinitProgramAndShaderObjects();

	/* Construct the shader body */
	const std::string samples_modifier_token		  = "SAMPLES_MODIFIER";
	size_t			  samples_modifier_token_location = std::string::npos;
	std::string		  samples_modifier_token_value;
	std::string		  sampler_type_string;
	const std::string sampler_type_token		  = "SAMPLER_TYPE";
	size_t			  sampler_type_token_location = std::string::npos;
	std::string		  sampler_type_token_value;

	switch (test_type)
	{
	case TEST_TYPE_IMAGE:
	{
		samples_modifier_token_value = "imageSamples(data);\n";

		switch (sampler_type)
		{
		case SAMPLER_TYPE_ISAMPLER2DMS:
		{
			sampler_type_string		 = "iimage2DMS";
			sampler_type_token_value = "layout(rgba8i) uniform iimage2DMS";

			break;
		}

		case SAMPLER_TYPE_ISAMPLER2DMSARRAY:
		{
			sampler_type_string		 = "iimage2DMSArray";
			sampler_type_token_value = "layout(rgba8i) uniform iimage2DMSArray";

			break;
		}

		case SAMPLER_TYPE_SAMPLER2DMS:
		{
			sampler_type_string		 = "image2DMS";
			sampler_type_token_value = "layout(rgba8) uniform image2DMS";

			break;
		}

		case SAMPLER_TYPE_SAMPLER2DMSARRAY:
		{
			sampler_type_string		 = "image2DMSArray";
			sampler_type_token_value = "layout(rgba8) uniform image2DMSArray";

			break;
		}

		case SAMPLER_TYPE_USAMPLER2DMS:
		{
			sampler_type_string		 = "uimage2DMS";
			sampler_type_token_value = "layout(rgba8ui) uniform uimage2DMS";

			break;
		}

		case SAMPLER_TYPE_USAMPLER2DMSARRAY:
		{
			sampler_type_string		 = "uimage2DMSArray";
			sampler_type_token_value = "layout(rgba8ui) uniform uimage2DMSArray";

			break;
		}

		default:
		{
			TCU_FAIL("Unrecognized sampler type");
		}
		} /* switch (sampler_type) */

		break;
	} /* case TEST_TYPE_IMAGE: */

	case TEST_TYPE_TEXTURE:
	{
		samples_modifier_token_value = "textureSamples(data);\n";

		switch (sampler_type)
		{
		case SAMPLER_TYPE_ISAMPLER2DMS:
		{
			sampler_type_string		 = "isampler2DMS";
			sampler_type_token_value = "uniform isampler2DMS";

			break;
		}

		case SAMPLER_TYPE_ISAMPLER2DMSARRAY:
		{
			sampler_type_string		 = "isampler2DMSArray";
			sampler_type_token_value = "uniform isampler2DMSArray";

			break;
		}

		case SAMPLER_TYPE_SAMPLER2DMS:
		{
			sampler_type_string		 = "sampler2DMS";
			sampler_type_token_value = "uniform sampler2DMS";

			break;
		}

		case SAMPLER_TYPE_SAMPLER2DMSARRAY:
		{
			sampler_type_string		 = "sampler2DMSArray";
			sampler_type_token_value = "uniform sampler2DMSArray";

			break;
		}

		case SAMPLER_TYPE_USAMPLER2DMS:
		{
			sampler_type_string		 = "usampler2DMS";
			sampler_type_token_value = "uniform usampler2DMS";

			break;
		}

		case SAMPLER_TYPE_USAMPLER2DMSARRAY:
		{
			sampler_type_string		 = "usampler2DMSArray";
			sampler_type_token_value = "uniform usampler2DMSArray";

			break;
		}

		default:
		{
			TCU_FAIL("Unrecognized sampler type");
		}
		} /* switch (sampler_type) */

		break;
	} /* case TEST_TYPE_TEXTURE: */

	default:
	{
		TCU_FAIL("Unrecognized test type");
	}
	} /* switch (test_type) */

	cs_body = cs_template_code;

	while ((samples_modifier_token_location = cs_body.find(samples_modifier_token)) != std::string::npos)
	{
		cs_body.replace(samples_modifier_token_location, samples_modifier_token.length(), samples_modifier_token_value);
	}

	while ((sampler_type_token_location = cs_body.find(sampler_type_token)) != std::string::npos)
	{
		cs_body.replace(sampler_type_token_location, sampler_type_token.length(), sampler_type_token_value);
	}

	/* Build the compute program */
	if (!buildComputeProgram(cs_body.c_str(), true, /* should_link_po */
							 true))					/* should_succeed */
	{
		TCU_FAIL("Could not link a test program");
	}

	/* Set up SSBO */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.shaderStorageBlockBinding(m_po_id, 0, /* storageBlockIndex */
								 0);		 /* storageBlockBinding */
	GLU_EXPECT_NO_ERROR(gl.getError(), "glShaderStorageBlockBinding() call failed.");

	gl.bindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, /* index */
					  m_bo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBufferBase() call failed.");

	/* Determine what numbers of samples we can use for GL_RGBA8, GL_RGBA8I
	 * or GL_RGBA8UI internalformats. Specific choice depends on the sampler type
	 * requested bty the caller.
	 */
	glw::GLenum texture_internalformat = GL_NONE;
	glw::GLenum texture_target		   = GL_NONE;

	if (sampler_type == SAMPLER_TYPE_ISAMPLER2DMS || sampler_type == SAMPLER_TYPE_ISAMPLER2DMSARRAY)
	{
		texture_internalformat = GL_RGBA8I;
	}
	else if (sampler_type == SAMPLER_TYPE_SAMPLER2DMS || sampler_type == SAMPLER_TYPE_SAMPLER2DMSARRAY)
	{
		texture_internalformat = GL_RGBA8;
	}
	else
	{
		texture_internalformat = GL_RGBA8UI;
	}

	if (sampler_type == SAMPLER_TYPE_ISAMPLER2DMS || sampler_type == SAMPLER_TYPE_SAMPLER2DMS ||
		sampler_type == SAMPLER_TYPE_USAMPLER2DMS)
	{
		texture_target = GL_TEXTURE_2D_MULTISAMPLE;
	}
	else
	{
		texture_target = GL_TEXTURE_2D_MULTISAMPLE_ARRAY;
	}

	gl.getInternalformativ(texture_target, texture_internalformat, GL_NUM_SAMPLE_COUNTS, 1, /* bufSize */
						   &m_internalformat_n_samples_count);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetInternalformativ() call failed.");

	if (m_internalformat_n_samples_count < 1)
	{
		TCU_FAIL("Invalid value returned by glGetInternalformativ() for a GL_NUM_SAMPLE_COUNTS query");
	}

	m_internalformat_n_samples_data = new glw::GLint[m_internalformat_n_samples_count];

	gl.getInternalformativ(texture_target, texture_internalformat, GL_SAMPLES, m_internalformat_n_samples_count,
						   m_internalformat_n_samples_data);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetInternalformativ() call failed.");

	/* Iterate over all sample counts. For each iteration, set up texture storage,
	 * run one work-item which will store the value retrieved by one of the two new
	 * ES SL functions into the buffer object. Once that happens, map the buffer
	 * object storage contents into process space and validate the value */
	for (int n_value = 0; n_value < m_internalformat_n_samples_count; ++n_value)
	{
		if (test_type == TEST_TYPE_IMAGE)
		{
			/* Shader images do not necessarily support all sample counts. */
			int max_image_samples;

			gl.getIntegerv(GL_MAX_IMAGE_SAMPLES, &max_image_samples);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv(GL_MAX_IMAGE_SAMPLES) call failed.");

			if (m_internalformat_n_samples_data[n_value] > max_image_samples)
				continue;
		}

		gl.genTextures(1, &m_to_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGenTextures() call failed.");

		gl.bindTexture(texture_target, m_to_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture() call failed.");

		if (texture_target == GL_TEXTURE_2D_MULTISAMPLE)
		{
			gl.texStorage2DMultisample(texture_target, m_internalformat_n_samples_data[n_value], texture_internalformat,
									   m_to_width, m_to_height, GL_FALSE); /* fixedsamplelocations */
			GLU_EXPECT_NO_ERROR(gl.getError(), "glTexStorage2DMultisample() call failed");
		}
		else
		{
			DE_ASSERT(texture_target == GL_TEXTURE_2D_MULTISAMPLE_ARRAY);

			gl.texStorage3DMultisample(texture_target, m_internalformat_n_samples_data[n_value], texture_internalformat,
									   m_to_width, m_to_height, m_to_depth, GL_TRUE); /* fixedsamplelocations */
			GLU_EXPECT_NO_ERROR(gl.getError(), "glTexStorage3DMultisample() call failed");
		}

		/* If we're running the "image" test, bind the texture to zeroth image unit */
		if (test_type == TEST_TYPE_IMAGE)
		{
			gl.bindImageTexture(0,			/* unit */
								m_to_id, 0, /* level */
								GL_FALSE,   /* layered */
								0,			/* lyer */
								GL_READ_ONLY, texture_internalformat);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glBindImageTexture() call failed.");
		}

		/* Dispatch a compute request */
		gl.useProgram(m_po_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram() call failed.");

		gl.dispatchCompute(1,  /* num_groups_x */
						   1,  /* num_groups_y */
						   1); /* num_groups_z */
		GLU_EXPECT_NO_ERROR(gl.getError(), "glDispatchCompute() call failed.");

		gl.memoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glMemoryBarrier() call failed.");

		/* Map the buffer object storage into process space */
		const void* bo_ptr		   = gl.mapBuffer(GL_ARRAY_BUFFER, GL_READ_ONLY);
		glw::GLint  expected_value = 0;

		GLU_EXPECT_NO_ERROR(gl.getError(), "glMapBuffer() call failed");

		expected_value = m_internalformat_n_samples_data[n_value];

		if (*(int*)bo_ptr != expected_value)
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "Value reported for the " << sampler_type_string << " test ["
							   << *(int*)bo_ptr << "] "
												   " is different from the expected one ["
							   << expected_value << "]." << tcu::TestLog::EndMessage;

			TCU_FAIL("Invalid value reported by ES SL function");
		}

		/* Safe to unmap the BO at this point */
		gl.unmapBuffer(GL_ARRAY_BUFFER);

		gl.deleteTextures(1, &m_to_id);
		m_to_id = 0;
		GLU_EXPECT_NO_ERROR(gl.getError(), "glDeleteTextures() call failed.");
	} /* for (all "n samples" values) */
}

/** Initializes all GL objects required to run the test. Also throws a
 *  NotSupportedError exception if GL_ARB_shader_texture_image_samples is not
 *  reported as a supported extension.
 **/
void ShaderTextureImageSamplesTestBase::init()
{
	/* Make sure GL_ARB_shader_texture_image_samples is supported before continuing */
	if (!m_context.getContextInfo().isExtensionSupported("GL_ARB_shader_texture_image_samples"))
	{
		throw tcu::NotSupportedError("GL_ARB_shader_texture_image_samples extension is not supported.");
	}

	/* Allocate BO storage to hold the result int value */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.genBuffers(1, &m_bo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenBuffers() call failed.");

	gl.bindBuffer(GL_ARRAY_BUFFER, m_bo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer() call failed.");

	gl.bufferData(GL_ARRAY_BUFFER, sizeof(int), DE_NULL, /* data */
				  GL_STATIC_DRAW);						 /* usage */
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferData() call failed.");
}

/** Constructor.
 *
 *  @param context Rendering context handle.
 **/
ShaderTextureImageSampleFunctionalTest::ShaderTextureImageSampleFunctionalTest(deqp::Context& context,
																			   const char*	test_name)
	: ShaderTextureImageSamplesTestBase(context, test_name, "Verifies that the new ES SL functions (imageSamples() and "
															"textureSamples() ) work as per spec for all supported "
															"numbers of samples + texture target combinations.")
{
	type_to_test = test_name[0] == 'i' ? TEST_TYPE_IMAGE : TEST_TYPE_TEXTURE;
}

/** Initializes all GL objects required to run the test.  Also throws a
 *  NotSupportedError exception if testing images and GL_MAX_IMAGE_SAMPLES = 0.
 **/
void ShaderTextureImageSampleFunctionalTest::init()
{
	ShaderTextureImageSamplesTestBase::init();

	if (type_to_test == TEST_TYPE_IMAGE)
	{
		const glw::Functions& gl = m_context.getRenderContext().getFunctions();

		glw::GLint max_image_samples;
		gl.getIntegerv(GL_MAX_IMAGE_SAMPLES, &max_image_samples);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv() call failed.");

		if (max_image_samples == 0)
		{
			throw tcu::NotSupportedError("GL_MAX_IMAGE_SAMPLES == 0");
		}
	}
}

/** Executes test iteration.
 *
 *  @return Returns STOP when test has finished executing, CONTINUE if more iterations are needed.
 */
tcu::TestNode::IterateResult ShaderTextureImageSampleFunctionalTest::iterate()
{
	for (unsigned int n_sampler_type = 0; n_sampler_type < SAMPLER_TYPE_COUNT; ++n_sampler_type)
	{
		_sampler_type sampler_type = (_sampler_type)n_sampler_type;

		for (unsigned int n_test_type = 0; n_test_type < TEST_TYPE_COUNT; ++n_test_type)
		{
			_test_type test_type = (_test_type)n_test_type;

			if (test_type == type_to_test)
			{
				executeFunctionalTest(sampler_type, test_type);
			}
		} /* for (all test types) */
	}	 /* for (all sampler types) */

	/* Test case passed */
	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

	return STOP;
}

/** Constructor.
 *
 *  @param context Rendering context handle.
 **/
ShaderTextureImageSamplesGLSLExtensionEnableTest::ShaderTextureImageSamplesGLSLExtensionEnableTest(
	deqp::Context& context)
	: ShaderTextureImageSamplesTestBase(
		  context, "glsl_extension_enable",
		  "Verifies a shader with \"#extension GL_ARB_shader_texture_image_samples : enable\" "
		  "line defines a corresponding pre-processor macro.")
{
	/* Left blank on purpose */
}

/** Executes test iteration.
 *
 *  @return Returns STOP when test has finished executing, CONTINUE if more iterations are needed.
 */
tcu::TestNode::IterateResult ShaderTextureImageSamplesGLSLExtensionEnableTest::iterate()
{
	const char* cs_body = "#version 440\n"
						  "\n"
						  "#extension GL_ARB_shader_texture_image_samples : enable\n"
						  "\n"
						  "void main()\n"
						  "{\n"
						  "#ifndef GL_ARB_shader_texture_image_samples\n"
						  "    force_compilation_error\n"
						  "#endif\n"
						  "}\n";

	if (!buildComputeProgram(cs_body, false, /* should_link_po */
							 true))			 /* should_succeed */
	{
		TCU_FAIL("GL_ARB_shader_image_load_store preprocessor #define is not set to the value of 1");
	}

	/* Test case passed */
	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

	return STOP;
}

/** Constructor.
 *
 *  @param context Rendering context handle.
 **/
ShaderTextureImageSamplesGLSLExtensionRequireTest::ShaderTextureImageSamplesGLSLExtensionRequireTest(
	deqp::Context& context)
	: ShaderTextureImageSamplesTestBase(
		  context, "glsl_extension_require",
		  "Verifies a shader with \"#extension GL_ARB_shader_texture_image_samples : require\" "
		  "line compiles without error.")
{
	/* Left blank on purpose */
}

/** Executes test iteration.
 *
 *  @return Returns STOP when test has finished executing, CONTINUE if more iterations are needed.
 */
tcu::TestNode::IterateResult ShaderTextureImageSamplesGLSLExtensionRequireTest::iterate()
{
	const char* cs_body = "#version 440\n"
						  "\n"
						  "#extension GL_ARB_shader_texture_image_samples : require\n"
						  "\n"
						  "void main()\n"
						  "{\n"
						  "}\n";

	if (!buildComputeProgram(cs_body, false, /* should_link_po */
							 true))			 /* should_succeed */
	{
		TCU_FAIL("A valid compute program has failed to build");
	}

	/* Test case passed */
	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

	return STOP;
}

/** Constructor.
 *
 *  @param context Rendering context.
 */
ShaderTextureImageSamplesTests::ShaderTextureImageSamplesTests(deqp::Context& context)
	: TestCaseGroup(context, "shader_texture_image_samples_tests",
					"Contains conformance tests that verify GL implementation's support "
					"for GL_ARB_shader_texture_image_samples extension.")
{
}

/** Initializes the test group contents. */
void ShaderTextureImageSamplesTests::init()
{
	addChild(new ShaderTextureImageSampleFunctionalTest(m_context, "image_functional_test"));
	addChild(new ShaderTextureImageSampleFunctionalTest(m_context, "texture_functional_test"));
	addChild(new ShaderTextureImageSamplesGLSLExtensionEnableTest(m_context));
	addChild(new ShaderTextureImageSamplesGLSLExtensionRequireTest(m_context));
}

} /* glcts namespace */
