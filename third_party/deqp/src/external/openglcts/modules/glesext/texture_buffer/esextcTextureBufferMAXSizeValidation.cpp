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
 * \file  esextcTextureBufferMAXSizeValidation.cpp
 * \brief Texture Buffer Max Size Validation (Test 2)
 */ /*-------------------------------------------------------------------*/

#include "esextcTextureBufferMAXSizeValidation.hpp"
#include "gluContextInfo.hpp"
#include "gluDefs.hpp"
#include "glwEnums.hpp"
#include "glwFunctions.hpp"
#include "tcuTestLog.hpp"
#include <vector>

namespace glcts
{

/* Number of components in computeSSBO.outValue vector */
const glw::GLuint TextureBufferMAXSizeValidation::m_n_vec_components = 3;

/** Constructor
 *
 *  @param context     Test context
 *  @param name        Test case's name
 *  @param description Test case's description
 **/
TextureBufferMAXSizeValidation::TextureBufferMAXSizeValidation(Context& context, const ExtParameters& extParams,
															   const char* name, const char* description)
	: TestCaseBase(context, extParams, name, description)
	, m_cs_id(0)
	, m_max_tex_buffer_size(0)
	, m_po_id(0)
	, m_ssbo_id(0)
	, m_tbo_id(0)
	, m_tbo_tex_id(0)
{
	/* Nothing to be done here */
}

/** Initializes GLES objects used during the test.
 *
 */
void TextureBufferMAXSizeValidation::initTest(void)
{
	/* Check if texture buffer extension is supported */
	if (!m_is_texture_buffer_supported)
	{
		throw tcu::NotSupportedError(TEXTURE_BUFFER_EXTENSION_NOT_SUPPORTED, "", __FILE__, __LINE__);
	}

	/* Get GL entry points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Query for the value of max texture buffer size */
	gl.getIntegerv(m_glExtTokens.MAX_TEXTURE_BUFFER_SIZE, &m_max_tex_buffer_size);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error getting gl parameter value!");

	/* Create texture buffer object*/
	gl.genBuffers(1, &m_tbo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error generating buffer object!");
	gl.bindBuffer(m_glExtTokens.TEXTURE_BUFFER, m_tbo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error binding buffer object!");
	gl.bufferData(m_glExtTokens.TEXTURE_BUFFER, m_max_tex_buffer_size * sizeof(glw::GLubyte), DE_NULL, GL_DYNAMIC_DRAW);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error allocating buffer object's data store!");

	glw::GLubyte* buf = 0;

	buf = (glw::GLubyte*)gl.mapBufferRange(m_glExtTokens.TEXTURE_BUFFER, 0,
										   m_max_tex_buffer_size * sizeof(glw::GLubyte), GL_MAP_WRITE_BIT);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error mapping buffer object's data store to client's address space!");

	for (glw::GLuint i = 0; i < (glw::GLuint)m_max_tex_buffer_size; ++i)
	{
		buf[i] = (glw::GLubyte)(i % 256);
	}

	gl.unmapBuffer(m_glExtTokens.TEXTURE_BUFFER);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error ummapping buffer object's data store from client's address space!");

	/* Initialize texture buffer */
	gl.genTextures(1, &m_tbo_tex_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error generating texture object!");
	gl.activeTexture(GL_TEXTURE0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error asetting active texture unit!");
	gl.bindTexture(m_glExtTokens.TEXTURE_BUFFER, m_tbo_tex_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error binding texture object!");

	gl.texBuffer(m_glExtTokens.TEXTURE_BUFFER, GL_R8UI, m_tbo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error binding buffer object as data store for texture buffer!");

	/* Create program object */
	m_po_id = gl.createProgram();
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error creating program object!");

	m_cs_id = gl.createShader(GL_COMPUTE_SHADER);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error creating shader object!");

	std::string csSource = getComputeShaderCode();
	const char* csCode   = csSource.c_str();

	if (!buildProgram(m_po_id, m_cs_id, 1, &csCode))
	{
		TCU_FAIL("Could not create program from valid compute shader object!");
	}

	/* Create Shader Storage Buffer Object */
	gl.genBuffers(1, &m_ssbo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error generating buffer object!");
	gl.bindBuffer(GL_ARRAY_BUFFER, m_ssbo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error binding  buffer object!");
	gl.bufferData(GL_ARRAY_BUFFER, m_n_vec_components * sizeof(glw::GLuint), 0, GL_DYNAMIC_COPY);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error generating buffer object!");
}

/** Returns Compute shader Code
 *
 * @return pointer to literal with Compute Shader Code
 */
const char* TextureBufferMAXSizeValidation::getComputeShaderCode()
{
	const char* result =
		"${VERSION}\n"
		"\n"
		"${TEXTURE_BUFFER_REQUIRE}\n"
		"\n"
		"precision highp float;\n"
		"\n"
		"uniform highp usamplerBuffer sampler_buffer;\n"
		"\n"
		"buffer ComputeSSBO\n"
		"{\n"
		"    uvec3 outValue;\n"
		"} computeSSBO;\n"
		"\n"
		"layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;\n"
		"\n"
		"void main(void)\n"
		"{\n"
		"    computeSSBO.outValue.x = uint( textureSize(sampler_buffer) );\n"
		"    computeSSBO.outValue.y = uint( texelFetch (sampler_buffer, 0).x );\n"
		"    computeSSBO.outValue.z = uint( texelFetch (sampler_buffer, int(computeSSBO.outValue.x) - 1 ).x );\n"
		"}\n";

	return result;
}

/** Executes the test.
 *
 *  Sets the test result to QP_TEST_RESULT_FAIL if the test failed, QP_TEST_RESULT_PASS otherwise.
 *
 *  Note the function throws exception should an error occur!
 *
 *  @return STOP if the test has finished, CONTINUE to indicate iterate should be called once again.
 **/
tcu::TestNode::IterateResult TextureBufferMAXSizeValidation::iterate(void)
{
	initTest();

	/* Get GL entry points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	bool testResult = true;

	glw::GLint textureOffset = -1;

	gl.getTexLevelParameteriv(m_glExtTokens.TEXTURE_BUFFER, 0, m_glExtTokens.TEXTURE_BUFFER_OFFSET, &textureOffset);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error getting texture buffer offset parameter value!");

	/* Check if offset is equal to 0 */
	const int expectedOffset = 0;

	if (expectedOffset != textureOffset)
	{
		m_testCtx.getLog() << tcu::TestLog::Message
						   << "Expected GL_TEXTURE_BUFFER_OFFSET_EXT parameter value : " << expectedOffset << "\n"
						   << "Result   GL_TEXTURE_BUFFER_OFFSET_EXT parameter value : " << textureOffset << "\n"
						   << tcu::TestLog::EndMessage;
		testResult = false;
	}

	glw::GLint textureSize = 0;

	gl.getTexLevelParameteriv(m_glExtTokens.TEXTURE_BUFFER, 0, m_glExtTokens.TEXTURE_BUFFER_SIZE, &textureSize);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error getting texture buffer paramter value!");

	/* Check if texture size is equal to m_max_tex_buffer_size * sizeof(glw::GLubyte) */
	glw::GLint expectedSize = static_cast<glw::GLint>(m_max_tex_buffer_size * sizeof(glw::GLubyte));

	if (expectedSize != textureSize)
	{
		m_testCtx.getLog() << tcu::TestLog::Message
						   << "Expected GL_TEXTURE_BUFFER_SIZE_EXT parameter value : " << expectedSize << "\n"
						   << "Result   GL_TEXTURE_BUFFER_SIZE_EXT parameter value : " << textureSize << "\n"
						   << tcu::TestLog::EndMessage;
		testResult = false;
	}

	gl.useProgram(m_po_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error setting program!");

	gl.bindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_ssbo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not bind buffer object to shader storage binding point!");

	gl.activeTexture(GL_TEXTURE0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error activating texture unit!");

	gl.bindTexture(m_glExtTokens.TEXTURE_BUFFER, m_tbo_tex_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error binding texture object!");

	glw::GLint location = gl.getUniformLocation(m_po_id, "sampler_buffer");
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error getting uniform location!");

	gl.uniform1i(location, 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error setting value for uniform location!");

	gl.dispatchCompute(1, 1, 1);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error running compute shader!");

	gl.memoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error setting memory barrier!");

	/* Get result data */
	glw::GLuint* result = (glw::GLuint*)gl.mapBufferRange(GL_SHADER_STORAGE_BUFFER, 0,
														  m_n_vec_components * sizeof(glw::GLuint), GL_MAP_READ_BIT);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error mapping buffer object to client's address space!");

	glw::GLuint expectedValue = (m_max_tex_buffer_size - 1) % 256;
	/* Log error if expected values and result data are not equal */
	if (result[0] != (glw::GLuint)m_max_tex_buffer_size || result[1] != 0 || result[2] != expectedValue)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Result is different than expected\n"
						   << "Expected size:                            " << m_max_tex_buffer_size << "\n"
						   << "Result value  (textureSize):              " << result[0] << "\n"
						   << "Expected Value (for index 0)              " << 0 << "\n"
						   << "Result value                              " << result[1] << "\n"
						   << "ExpectedValue (for last index)            " << expectedValue << "\n"
						   << "Result value                              " << result[2] << "\n"
						   << tcu::TestLog::EndMessage;

		testResult = false;
	}

	if (testResult)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
	}

	gl.unmapBuffer(GL_SHADER_STORAGE_BUFFER);

	return STOP;
}

/** Deinitializes GLES objects created during the test.
 *
 */
void TextureBufferMAXSizeValidation::deinit(void)
{
	/* Get GL entry points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Reset GLES state */
	gl.useProgram(0);
	gl.bindBuffer(GL_ARRAY_BUFFER, 0);
	gl.bindBuffer(m_glExtTokens.TEXTURE_BUFFER, 0);
	gl.bindTexture(m_glExtTokens.TEXTURE_BUFFER, 0);
	gl.bindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, 0);

	/* Delete GLES objects */
	if (0 != m_cs_id)
	{
		gl.deleteShader(m_cs_id);
		m_cs_id = 0;
	}

	if (0 != m_po_id)
	{
		gl.deleteProgram(m_po_id);
		m_po_id = 0;
	}

	if (0 != m_ssbo_id)
	{
		gl.deleteBuffers(1, &m_ssbo_id);
		m_ssbo_id = 0;
	}

	if (0 != m_tbo_id)
	{
		gl.deleteBuffers(1, &m_tbo_id);
		m_tbo_id = 0;
	}

	if (0 != m_tbo_tex_id)
	{
		gl.deleteTextures(1, &m_tbo_tex_id);
		m_tbo_tex_id = 0;
	}

	/* Deinitialize base class */
	TestCaseBase::deinit();
}

} // namespace glcts
