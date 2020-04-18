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

/*!
 * \file  esextcTextureBufferPrecision.hpp
 * \brief Texture Buffer Precision Qualifier (Test 10)
 */ /*-------------------------------------------------------------------*/

#include "esextcTextureBufferPrecision.hpp"
#include "gluContextInfo.hpp"
#include "gluDefs.hpp"
#include "gluShaderUtil.hpp"
#include "glwEnums.hpp"
#include "glwFunctions.hpp"
#include "tcuTestLog.hpp"
#include <cstring>
#include <stddef.h>

namespace glcts
{
/* Head of the compute shader. */
const char* const TextureBufferPrecision::m_cs_code_head = "${VERSION}\n"
														   "\n"
														   "${TEXTURE_BUFFER_REQUIRE}\n"
														   "\n";

/* Variables declarations for the compute shader without usage of the precision qualifier. */
const char* const TextureBufferPrecision::m_cs_code_declaration_without_precision[3] = {
	"layout (r32f)  uniform  imageBuffer image_buffer;\n"
	"\n"
	"buffer ComputeSSBO\n"
	"{\n"
	"    float value;\n"
	"} computeSSBO;\n",

	"layout (r32i)  uniform iimageBuffer image_buffer;\n"
	"\n"
	"buffer ComputeSSBO\n"
	"{\n"
	"    int value;\n"
	"} computeSSBO;\n",

	"layout (r32ui) uniform uimageBuffer image_buffer;\n"
	"\n"
	"buffer ComputeSSBO\n"
	"{\n"
	"    uint value;\n"
	"} computeSSBO;\n"
};

/* Variables declarations for the compute shader with usage of the precision qualifier. */
const char* const TextureBufferPrecision::m_cs_code_declaration_with_precision[3] = {
	"layout (r32f)  uniform highp  imageBuffer image_buffer;\n"
	"\n"
	"buffer ComputeSSBO\n"
	"{\n"
	"    float value;\n"
	"} computeSSBO;\n",

	"layout (r32i)  uniform highp iimageBuffer image_buffer;\n"
	"\n"
	"buffer ComputeSSBO\n"
	"{\n"
	"    int value;\n"
	"} computeSSBO;\n",

	"layout (r32ui) uniform highp uimageBuffer image_buffer;\n"
	"\n"
	"buffer ComputeSSBO\n"
	"{\n"
	"    uint value;\n"
	"} computeSSBO;\n"
};

/* The default precision qualifier declarations for the compute shader. */
const char* const TextureBufferPrecision::m_cs_code_global_precision[3] = { "precision highp  imageBuffer;\n",
																			"precision highp iimageBuffer;\n",
																			"precision highp uimageBuffer;\n" };

/* The compute shader body. */
const char* const TextureBufferPrecision::m_cs_code_body = "\n"
														   "void main()\n"
														   "{\n"
														   "    computeSSBO.value = imageLoad(image_buffer, 0).x;\n"
														   "}";

/* Head of the fragment shader. */
const char* const TextureBufferPrecision::m_fs_code_head = "${VERSION}\n"
														   "\n"
														   "${TEXTURE_BUFFER_REQUIRE}\n"
														   "\n";

/* Variables declarations for the fragment shader without usage of the precision qualifier. */
const char* const TextureBufferPrecision::m_fs_code_declaration_without_precision[3] = {
	"uniform  samplerBuffer sampler_buffer;\n"
	"\n"
	"layout(location = 0) out highp vec4 color;\n",

	"uniform isamplerBuffer sampler_buffer;\n"
	"\n"
	"layout(location = 0) out highp ivec4 color;\n",

	"uniform usamplerBuffer sampler_buffer;\n"
	"\n"
	"layout(location = 0) out highp uvec4 color;\n"
};

/* Variables declarations for the fragment shader with usage of the precision qualifier. */
const char* const TextureBufferPrecision::m_fs_code_declaration_with_precision[3] = {
	"uniform highp  samplerBuffer sampler_buffer;\n"
	"\n"
	"layout(location = 0) out highp vec4 color;\n",

	"uniform highp isamplerBuffer sampler_buffer;\n"
	"\n"
	"layout(location = 0) out highp ivec4 color;\n",

	"uniform highp usamplerBuffer sampler_buffer;\n"
	"\n"
	"layout(location = 0) out highp uvec4 color;\n",
};

/* The default precision qualifier declarations for the fragment shader. */
const char* const TextureBufferPrecision::m_fs_code_global_precision[3] = {
	"precision highp  samplerBuffer;\n", "precision highp isamplerBuffer;\n", "precision highp usamplerBuffer;\n",
};

/* The fragment shader body. */
const char* const TextureBufferPrecision::m_fs_code_body = "\n"
														   "void main()\n"
														   "{\n"
														   "    color = texelFetch( sampler_buffer, 0 );\n"
														   "}";

/** Constructor
 *
 * @param context     Test context
 * @param name        Test case's name
 * @param description Test case's description
 **/
TextureBufferPrecision::TextureBufferPrecision(Context& context, const ExtParameters& extParams, const char* name,
											   const char* description)
	: TestCaseBase(context, extParams, name, description), m_po_id(0), m_sh_id(0)
{
	//Bug-15063 Only GLSL 4.50 supports opaque types
	if (m_glslVersion >= glu::GLSL_VERSION_130)
	{
		m_glslVersion = glu::GLSL_VERSION_450;
	}
}

/** Deinitializes GLES objects created during the test */
void TextureBufferPrecision::deinit(void)
{
	/* Retrieve GLES entry points. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Delete GLES objects */
	if (m_po_id != 0)
	{
		gl.deleteProgram(m_po_id);
		m_po_id = 0;
	}

	if (m_sh_id != 0)
	{
		gl.deleteShader(m_sh_id);
		m_sh_id = 0;
	}

	/* Deinitialize base class */
	TestCaseBase::deinit();
}

/** Check if the shader compiles.
 *
 *  Note the function throws exception should an error occur!
 *
 *  @param shader_type      GL shader type.
 *  @param sh_code_parts    Shader source parts
 *  @param parts_count      Shader source parts count
 *
 *  @return true if the shader compiles, return false otherwise.
 **/
glw::GLboolean TextureBufferPrecision::verifyShaderCompilationStatus(glw::GLenum	   shader_type,
																	 const char**	  sh_code_parts,
																	 const glw::GLuint parts_count,
																	 const glw::GLint  expected_status)
{
	/* Retrieve GLES entry points. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	glw::GLboolean test_passed = true;

	m_po_id = gl.createProgram();
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not create program object");

	m_sh_id = gl.createShader(shader_type);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not create shader object");

	std::string shader_code		= specializeShader(parts_count, sh_code_parts);
	const char* shader_code_ptr = shader_code.c_str();
	gl.shaderSource(m_sh_id, 1, &shader_code_ptr, DE_NULL);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not set shader source");

	gl.compileShader(m_sh_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Failed to perform compilation of shader object");

	/* Retrieving compilation status */
	glw::GLint compilation_status = GL_FALSE;
	gl.getShaderiv(m_sh_id, GL_COMPILE_STATUS, &compilation_status);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not retrieve shader compilation status");

	if (compilation_status != expected_status)
	{
		test_passed = false;

		glw::GLsizei info_log_length = 0;
		gl.getShaderiv(m_sh_id, GL_INFO_LOG_LENGTH, &info_log_length);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Could not get shader info log length");

		if (info_log_length > 0)
		{
			glw::GLchar* info_log = new glw::GLchar[info_log_length];
			memset(info_log, 0, info_log_length * sizeof(glw::GLchar));
			gl.getShaderInfoLog(m_sh_id, info_log_length, DE_NULL, info_log);

			m_testCtx.getLog() << tcu::TestLog::Message << "The following shader:\n\n"
							   << shader_code << "\n\n did compile with result different than expected:\n"
							   << expected_status << "\n\n Compilation info log: \n"
							   << info_log << "\n"
							   << tcu::TestLog::EndMessage;

			delete[] info_log;

			GLU_EXPECT_NO_ERROR(gl.getError(), "Could not get shader info log");
		}
	}

	/* Clean up */
	if (m_po_id != 0)
	{
		gl.deleteProgram(m_po_id);
		m_po_id = 0;
	}

	if (m_sh_id != 0)
	{
		gl.deleteShader(m_sh_id);
		m_sh_id = 0;
	}

	return test_passed;
}

/** Executes the test.
 *
 *  Sets the test result to QP_TEST_RESULT_FAIL if the test failed, QP_TEST_RESULT_PASS otherwise.
 *
 *  Note the function throws exception should an error occur!
 *
 *  @return STOP if the test has finished, CONTINUE to indicate iterate should be called once again.
 **/
tcu::TestNode::IterateResult TextureBufferPrecision::iterate(void)
{
	/* Skip if required extensions are not supported. */
	if (!m_is_texture_buffer_supported)
	{
		throw tcu::NotSupportedError(TEXTURE_BUFFER_EXTENSION_NOT_SUPPORTED, "", __FILE__, __LINE__);
	}

	glu::ContextType contextType   = m_context.getRenderContext().getType();
	glw::GLboolean   test_passed   = true;
	glw::GLint		 expected_fail = glu::glslVersionIsES(m_glslVersion) ? GL_FALSE : GL_TRUE;

	/* Default precision qualifiers for opaque types are not supported prior to GLSL 4.50. */
	if (glu::contextSupports(contextType, glu::ApiType::core(4, 5)) ||
		glu::contextSupports(contextType, glu::ApiType::es(3, 1)))
	{
		/* Compute Shader Tests */
		for (glw::GLuint i = 0; i < 3; ++i) /* For each from the set {imageBuffer, iimageBuffer, uimageBuffer} */
		{
			const char* cs_code_without_precision[3] = { m_cs_code_head, m_cs_code_declaration_without_precision[i],
														 m_cs_code_body };

			const char* cs_code_with_precision[3] = { m_cs_code_head, m_cs_code_declaration_with_precision[i],
													  m_cs_code_body };

			const char* cs_code_with_global_precision[4] = { m_cs_code_head, m_cs_code_global_precision[i],
															 m_cs_code_declaration_without_precision[i],
															 m_cs_code_body };

			test_passed =
				verifyShaderCompilationStatus(GL_COMPUTE_SHADER, cs_code_without_precision, 3, expected_fail) &&
				test_passed;
			test_passed =
				verifyShaderCompilationStatus(GL_COMPUTE_SHADER, cs_code_with_precision, 3, GL_TRUE) && test_passed;
			test_passed = verifyShaderCompilationStatus(GL_COMPUTE_SHADER, cs_code_with_global_precision, 4, GL_TRUE) &&
						  test_passed;
		}
	}

	/* Fragment Shader Tests */
	for (glw::GLuint i = 0; i < 3; ++i) /* For each from the set {samplerBuffer, isamplerBuffer, usamplerBuffer} */
	{
		const char* fs_code_without_precision[3] = { m_fs_code_head, m_fs_code_declaration_without_precision[i],
													 m_fs_code_body };

		const char* fs_code_with_precision[3] = { m_fs_code_head, m_fs_code_declaration_with_precision[i],
												  m_fs_code_body };

		const char* fs_code_with_global_precision[4] = { m_fs_code_head, m_fs_code_global_precision[i],
														 m_fs_code_declaration_without_precision[i], m_fs_code_body };

		test_passed = verifyShaderCompilationStatus(GL_FRAGMENT_SHADER, fs_code_without_precision, 3, expected_fail) &&
					  test_passed;
		test_passed =
			verifyShaderCompilationStatus(GL_FRAGMENT_SHADER, fs_code_with_precision, 3, GL_TRUE) && test_passed;
		test_passed =
			verifyShaderCompilationStatus(GL_FRAGMENT_SHADER, fs_code_with_global_precision, 4, GL_TRUE) && test_passed;
	}

	if (test_passed)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
	}

	return STOP;
}

} /* namespace glcts */
