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
 * \file  esextcTextureBufferOperations.cpp
 * \brief Texture Buffer Operations (Test 1)
 */ /*-------------------------------------------------------------------*/

#include "esextcTextureBufferOperations.hpp"
#include "gluContextInfo.hpp"
#include "gluDefs.hpp"
#include "glwEnums.hpp"
#include "glwFunctions.hpp"
#include "tcuTestLog.hpp"
#include <cstring>
#include <iostream>
#include <vector>

namespace glcts
{

const glw::GLuint TextureBufferOperations::m_n_vector_components = 4;

/** Constructor
 *
 *  @param context     Test context
 *  @param name        Test case's name
 *  @param description Test case's description
 **/
TextureBufferOperations::TextureBufferOperations(Context& context, const ExtParameters& extParams, const char* name,
												 const char* description)
	: TestCaseBase(context, extParams, name, description)
	, m_n_vectors_in_buffer_texture(0)
	, m_tb_bo_id(0)
	, m_texbuff_id(0)
	, m_cs_id(0)
	, m_po_cs_id(0)
	, m_ssbo_bo_id(0)
	, m_fbo_id(0)
	, m_fs_id(0)
	, m_po_vs_fs_id(0)
	, m_to_id(0)
	, m_vao_id(0)
	, m_vbo_id(0)
	, m_vbo_indicies_id(0)
	, m_vs_id(0)
	, m_vertex_location(-1)
	, m_index_location(-1)
{
	/* Nothing to be done here */
}

/** Initializes GLES objects used during the test.
 *
 */
void TextureBufferOperations::initTest(void)
{
	/* Check if texture buffer extension is supported */
	if (!m_is_texture_buffer_supported)
	{
		throw tcu::NotSupportedError(TEXTURE_BUFFER_EXTENSION_NOT_SUPPORTED, "", __FILE__, __LINE__);
	}

	/* Get GL entry points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.getIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, &m_n_vectors_in_buffer_texture);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error getting GL_MAX_COMPUTE_WORK_GROUP_SIZE parameter value!");

	/* Create buffer object*/
	gl.genBuffers(1, &m_tb_bo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error generating buffer object!");

	gl.bindBuffer(m_glExtTokens.TEXTURE_BUFFER, m_tb_bo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error binding buffer object !");

	gl.bufferData(m_glExtTokens.TEXTURE_BUFFER,
				  m_n_vectors_in_buffer_texture * m_n_vector_components * sizeof(glw::GLint), 0, GL_DYNAMIC_DRAW);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error allocating data store for buffer object!");

	/* Create texture buffer */
	gl.genTextures(1, &m_texbuff_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error generating texture object!");

	gl.activeTexture(GL_TEXTURE0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error setting active texture unit!");

	gl.bindTexture(m_glExtTokens.TEXTURE_BUFFER, m_texbuff_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error binding texture object!");

	gl.texBuffer(m_glExtTokens.TEXTURE_BUFFER, GL_RGBA32I, m_tb_bo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error binding buffer object to texture buffer!");

	/* Initialize texture buffer object */
	initializeBufferObjectData();

	/* Initialize first phase */
	initFirstPhase();

	/* Initialize second phase */
	initSecondPhase();
}

void TextureBufferOperations::initFirstPhase(void)
{
	/* Get GL entry points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Create program object */
	m_po_cs_id = gl.createProgram();
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error creating program object!");

	m_cs_id = gl.createShader(GL_COMPUTE_SHADER);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error creating shader object!");

	std::string csSource = getComputeShaderCode();
	const char* csCode   = csSource.c_str();

	if (!buildProgram(m_po_cs_id, m_cs_id, 1, &csCode))
	{
		TCU_FAIL("Could not create a program from valid compute shader code!");
	}

	/* Create Shader Storage Buffer Object */
	gl.genBuffers(1, &m_ssbo_bo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error generating buffer object!");
	gl.bindBuffer(GL_ARRAY_BUFFER, m_ssbo_bo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error binding  buffer object!");
	gl.bufferData(GL_ARRAY_BUFFER, m_n_vectors_in_buffer_texture * m_n_vector_components * sizeof(glw::GLint), 0,
				  GL_DYNAMIC_DRAW);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error allocating data store for buffer object!");
	gl.bindBuffer(GL_ARRAY_BUFFER, 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error binding  buffer object!");
}

/** Returns Compute shader Code
 *
 * @return pointer to literal with Compute Shader Code
 */
std::string TextureBufferOperations::getComputeShaderCode() const
{
	std::stringstream strstream;

	strstream << "${VERSION}\n"
				 "\n"
				 "${TEXTURE_BUFFER_REQUIRE}\n"
				 "\n"
				 "precision highp float;\n"
				 "\n"
				 "layout(rgba32i, binding = 0) uniform readonly highp iimageBuffer image_buffer;\n"
				 "\n"
				 "buffer ComputeSSBO\n"
				 "{\n"
				 "    ivec4 value[];\n"
				 "} computeSSBO;\n"
				 "\n"
				 "layout (local_size_x = "
			  << m_n_vectors_in_buffer_texture << " ) in;\n"
												  "\n"
												  "void main(void)\n"
												  "{\n"
												  "    int index = int(gl_LocalInvocationID.x);\n"
												  "    computeSSBO.value[index] = imageLoad( image_buffer, index);\n"
												  "}\n";

	return strstream.str();
}

void TextureBufferOperations::initSecondPhase(void)
{
	/* Get GL entry points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Create vertex array object */
	gl.genVertexArrays(1, &m_vao_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error generating vertex array object!");

	/* Create framebuffer object */
	gl.genFramebuffers(1, &m_fbo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error genertaing framebuffer object!");

	/* Prepare texture object */
	gl.genTextures(1, &m_to_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error genertaing texture buffer!");

	gl.bindTexture(GL_TEXTURE_2D, m_to_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error binding texture object!");

	gl.texStorage2D(GL_TEXTURE_2D, 1, GL_RGBA32I, m_n_vectors_in_buffer_texture /* width */, 1 /* height */);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error allocating texture object's texel data!");

	/* Set GL_TEXTURE_MAG_FILTER and GL_TEXTURE_MIN_FILTER parameters values to GL_NEAREST*/
	gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error setting value for GL_TEXTURE_MAG_FILTER parameter!");
	gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error setting value for GL_TEXTURE_MIN_FILTER parameter!");

	gl.bindTexture(GL_TEXTURE_2D, 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error binding texture object!");

	/* Create program object */
	m_po_vs_fs_id = gl.createProgram();
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error creating program object!");

	m_fs_id = gl.createShader(GL_FRAGMENT_SHADER);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error creating fragment shader object!");

	m_vs_id = gl.createShader(GL_VERTEX_SHADER);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error creating vertex shader object!");

	std::string fsSource = getFragmentShaderCode();
	std::string vsSource = getVertexShaderCode();

	const char* fsCode = fsSource.c_str();
	const char* vsCode = vsSource.c_str();

	if (!buildProgram(m_po_vs_fs_id, m_fs_id, 1, &fsCode, m_vs_id, 1, &vsCode))
	{
		TCU_FAIL("Could not create a program from valid vertex/fragment shader source!");
	}

	/* Full screen quad */
	glw::GLfloat vertices[] = { -1.0f, -1.0f, 0.0f, 1.0f, -1.0f, 1.0f, 0.0f, 1.0f,
								1.0f,  -1.0f, 0.0f, 1.0f, 1.0f,  1.0f, 0.0f, 1.0f };

	/* Generate buffer object */
	gl.genBuffers(1, &m_vbo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error generating buffer object!");
	/* Bind buffer object */
	gl.bindBuffer(GL_ARRAY_BUFFER, m_vbo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error binding buffer object!");
	/* Set data for buffer object */
	gl.bufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error setting data for buffer object!");

	/* Indicies */
	glw::GLfloat indicies[] = { 0.f, 0.f, static_cast<glw::GLfloat>(m_n_vectors_in_buffer_texture) * 1.f,
								static_cast<glw::GLfloat>(m_n_vectors_in_buffer_texture) * 1.f };

	/* Generate buffer object */
	gl.genBuffers(1, &m_vbo_indicies_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error generating buffer object!");
	/* Bind buffer object */
	gl.bindBuffer(GL_ARRAY_BUFFER, m_vbo_indicies_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error binding buffer object!");
	/* Set data for buffer object */
	gl.bufferData(GL_ARRAY_BUFFER, sizeof(indicies), indicies, GL_STATIC_DRAW);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error seting data for buffer object!");

	gl.bindBuffer(GL_ARRAY_BUFFER, 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error binding buffer object!");
}

/** Returns Fragment Shader Code
 *
 * @return pointer to literal with Fragment Shader Code
 */
std::string TextureBufferOperations::getFragmentShaderCode() const
{
	std::stringstream strstream;

	strstream << "${VERSION}\n"
				 "\n"
				 "${TEXTURE_BUFFER_REQUIRE}\n"
				 "\n"
				 "precision highp float;\n"
				 "\n"
				 "in float fs_index;\n"
				 "\n"
				 "layout(location = 0) out ivec4 color;\n"
				 "\n"
				 "uniform highp isamplerBuffer sampler_buffer;\n"
				 "\n"
				 "void main(void)\n"
				 "{\n"
				 "    color = texelFetch( sampler_buffer, int(fs_index) );\n "
				 "}\n";

	return strstream.str();
}

/** Returns Vertex Shader Code
 *
 * @return pointer to literal with Vertex Shader Code
 */
std::string TextureBufferOperations::getVertexShaderCode() const
{
	std::stringstream strstream;

	strstream << "${VERSION}\n"
				 "\n"
				 "${TEXTURE_BUFFER_REQUIRE}\n"
				 "\n"
				 "precision highp float;\n"
				 "\n"
				 "in vec4 vs_position;\n"
				 "in float  vs_index;\n"
				 "\n"
				 "out float fs_index;\n"
				 "\n"
				 "void main(void)\n"
				 "{\n"
				 "    gl_Position = vs_position;\n"
				 "    fs_index    = vs_index;\n"
				 "}\n";

	return strstream.str();
}

/** Executes the test.
 *
 *  Sets the test result to QP_TEST_RESULT_FAIL if the test failed, QP_TEST_RESULT_PASS otherwise.
 *
 *  Note the function throws exception should an error occur!
 *
 *  @return STOP if the test has finished, CONTINUE to indicate iterate should be called once again.
 **/
tcu::TestNode::IterateResult TextureBufferOperations::iterate(void)
{
	/* Initialize */
	initTest();

	/* Get GL entry points */
	glw::GLboolean test_result = true;

	/* Prepare expected data */
	std::vector<glw::GLint> reference(m_n_vectors_in_buffer_texture * m_n_vector_components);
	fillBufferWithData(&reference[0], m_n_vectors_in_buffer_texture * m_n_vector_components);

	std::vector<glw::GLint> result(m_n_vectors_in_buffer_texture * m_n_vector_components);

	iterateFirstPhase(&result[0], static_cast<glw::GLuint>(m_n_vectors_in_buffer_texture * m_n_vector_components *
														   sizeof(glw::GLint)));
	if (!verifyResults(&reference[0], &result[0], static_cast<glw::GLuint>(m_n_vectors_in_buffer_texture *
																		   m_n_vector_components * sizeof(glw::GLint)),
					   "1st Phase Compute Shader\n"))
	{
		test_result = false;
	}

	iterateSecondPhase(&result[0]);
	if (!verifyResults(&reference[0], &result[0], static_cast<glw::GLuint>(m_n_vectors_in_buffer_texture *
																		   m_n_vector_components * sizeof(glw::GLint)),
					   "2st Phase Vertex + Fragment Shader\n"))
	{
		test_result = false;
	}

	if (test_result)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
	}

	return STOP;
}

void TextureBufferOperations::iterateFirstPhase(glw::GLint* result, glw::GLuint size)
{
	/* Get GL entry points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.bindBuffer(GL_ARRAY_BUFFER, m_ssbo_bo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error binding  buffer object!");

	gl.bindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_ssbo_bo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not bind buffer object to shader storage binding point!");

	gl.useProgram(m_po_cs_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error setting active program!");

	gl.bindImageTexture(0, m_texbuff_id, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32I);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error binding texture to image unit!");

	gl.dispatchCompute(1, 1, 1);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error running compute shader!");

	gl.memoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error setting memory barrier!");

	/* Get result data */
	glw::GLint* result_mapped = (glw::GLint*)gl.mapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, size, GL_MAP_READ_BIT);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error mapping buffer object's data store to client's address space!");

	memcpy(result, result_mapped, size);

	gl.unmapBuffer(GL_SHADER_STORAGE_BUFFER);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error unmapping buffer object's data store from client's address space!");

	gl.bindBuffer(GL_ARRAY_BUFFER, 0);
	gl.bindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, 0);
	gl.useProgram(0);
}

void TextureBufferOperations::iterateSecondPhase(glw::GLint* result)
{
	/* Get GL entry points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.bindVertexArray(m_vao_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error binding vertex array object!");

	gl.bindFramebuffer(GL_DRAW_FRAMEBUFFER, m_fbo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error binding framebuffer object!");

	gl.bindTexture(GL_TEXTURE_2D, m_to_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error binding texture object!");

	/* Attach output texture to framebuffer */
	gl.framebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_to_id, 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error attaching texture to framebuffer's color attachment");

	/* Check framebuffer status */
	checkFramebufferStatus(GL_DRAW_FRAMEBUFFER);

	/* Configure view port */
	gl.viewport(0, 0, m_n_vectors_in_buffer_texture, 1);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error setting viewport!");

	/* Use program */
	gl.useProgram(m_po_vs_fs_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error using program object!");

	glw::GLint sampler_location = gl.getUniformLocation(m_po_vs_fs_id, "sampler_buffer");
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not get attribute location!");
	if (sampler_location == -1)
	{
		TCU_FAIL("Could not get uniform location");
	}

	gl.uniform1i(sampler_location, 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error binding sampler with texture unit!");

	/* Configure vertex position attribute */
	gl.bindBuffer(GL_ARRAY_BUFFER, m_vbo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not bind buffer object!");

	m_vertex_location = gl.getAttribLocation(m_po_vs_fs_id, "vs_position");
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not get attribute location!");
	if (m_vertex_location == -1)
	{
		TCU_FAIL("Could not get uniform location");
	}

	gl.vertexAttribPointer(m_vertex_location, 4, GL_FLOAT, GL_FALSE, 0, 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not set vertex attribute pointer!");

	gl.enableVertexAttribArray(m_vertex_location);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not enable vertex attribute array!");

	/* Configure index attribute */
	gl.bindBuffer(GL_ARRAY_BUFFER, m_vbo_indicies_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not bind buffer object!");

	m_index_location = gl.getAttribLocation(m_po_vs_fs_id, "vs_index");
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not get attribute location!");
	if (m_index_location == -1)
	{
		TCU_FAIL("Could not get uniform location");
	}

	gl.vertexAttribPointer(m_index_location, 1, GL_FLOAT, GL_FALSE, 0, 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not set vertex attribute pointer!");

	gl.enableVertexAttribArray(m_index_location);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not enable vertex attribute array!");

	/* Clear texture */
	gl.clearColor(0.0f, 0.0f, 0.0f, 0.0f);
	gl.clear(GL_COLOR_BUFFER_BIT);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error clearing color buffer");

	/* Render */
	gl.drawArrays(GL_TRIANGLE_STRIP, 0, 4);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Rendering failed!");

	/* Read result data */
	gl.bindFramebuffer(GL_READ_FRAMEBUFFER, m_fbo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error binding framebuffer object !");

	gl.readPixels(0, 0, m_n_vectors_in_buffer_texture, 1, GL_RGBA_INTEGER, GL_INT, result);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error reading pixels !");

	gl.bindVertexArray(0);
	gl.bindTexture(GL_TEXTURE_2D, 0);
	gl.bindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	gl.bindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	gl.bindBuffer(GL_ARRAY_BUFFER, 0);
	gl.disableVertexAttribArray(m_vertex_location);
	gl.disableVertexAttribArray(m_index_location);
	gl.useProgram(0);

	m_vertex_location = -1;
	m_index_location  = -1;
}

/** Check if result data is the same as reference data - log error if not
 *
 * @param  reference pointer to buffer with reference data
 * @param  result    pointer to buffer with result data
 * @param  size      size of buffers
 * @param  message   pointer to literal with message (informing about test phase)
 *
 * @return           returns true if reference data equals result data, otherwise log error and return false
 */
glw::GLboolean TextureBufferOperations::verifyResults(glw::GLint* reference, glw::GLint* result, glw::GLuint size,
													  const char* message)
{
	/* Log error if expected and result data is not equal */
	if (memcmp(reference, result, size))
	{
		std::stringstream referenceData;
		std::stringstream resultData;

		referenceData << "[";
		resultData << "[";

		glw::GLuint n_entries = static_cast<glw::GLuint>(size / sizeof(glw::GLint));

		for (glw::GLuint i = 0; i < n_entries; ++i)
		{
			referenceData << reference[i] << ",";
			resultData << result[i] << ",";
		}

		referenceData << "]";
		resultData << "]";

		m_testCtx.getLog() << tcu::TestLog::Message << message
						   << "Result buffer contains different data than reference buffer\n"
						   << "Reference Buffer: " << referenceData.str() << "\n"
						   << "Result Buffer: " << resultData.str() << "\n"
						   << tcu::TestLog::EndMessage;
		return false;
	}

	return true;
}

/** Fill buffer with test data
 *
 * @param buffer      pointer to buffer
 * @param bufferLenth buffer length
 */
void TextureBufferOperations::fillBufferWithData(glw::GLint* buffer, glw::GLuint bufferLength)
{
	for (glw::GLuint i = 0; i < bufferLength; ++i)
	{
		buffer[i] = (glw::GLint)i;
	}
}

/** Check Framebuffer Status - throw exception if status is different than GL_FRAMEBUFFER_COMPLETE
 *
 * @param framebuffer  - GL_DRAW_FRAMEBUFFER, GL_READ_FRAMEBUFFER or GL_FRAMEBUFFER
 *
 */
void TextureBufferOperations::checkFramebufferStatus(glw::GLenum framebuffer)
{
	/* Get GL entry points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Check framebuffer status */
	glw::GLenum framebufferStatus = gl.checkFramebufferStatus(framebuffer);

	if (GL_FRAMEBUFFER_COMPLETE != framebufferStatus)
	{
		switch (framebufferStatus)
		{
		case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
		{
			TCU_FAIL("Framebuffer incomplete, status: GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT");
		}

		case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS:
		{
			TCU_FAIL("Framebuffer incomplete, status: GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS");
		}

		case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
		{
			TCU_FAIL("Framebuffer incomplete, status: GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT");
		}

		case GL_FRAMEBUFFER_UNSUPPORTED:
		{
			TCU_FAIL("Framebuffer incomplete, status: GL_FRAMEBUFFER_UNSUPPORTED");
		}

		default:
		{
			TCU_FAIL("Framebuffer incomplete, status not recognized");
		}
		};
	} /* if (GL_FRAMEBUFFER_COMPLETE != framebuffer_status) */
}

/** Deinitializes GLES objects created during the test.
 *
 */
void TextureBufferOperations::deinit(void)
{
	/* Get GLES entry points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Reset GLES state */
	gl.bindBuffer(m_glExtTokens.TEXTURE_BUFFER, 0);
	gl.bindTexture(m_glExtTokens.TEXTURE_BUFFER, 0);

	/* Delete GLES objects */
	if (0 != m_texbuff_id)
	{
		gl.deleteTextures(1, &m_texbuff_id);
		m_texbuff_id = 0;
	}

	if (0 != m_tb_bo_id)
	{
		gl.deleteBuffers(1, &m_tb_bo_id);
		m_tb_bo_id = 0;
	}

	deinitFirstPhase();
	deinitSecondPhase();

	/* Deinitialize base class */
	TestCaseBase::deinit();
}

void TextureBufferOperations::deinitFirstPhase(void)
{
	/* Get GLES entry points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Reset GLES state */
	gl.bindBuffer(GL_ARRAY_BUFFER, 0);
	gl.bindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, 0);
	gl.useProgram(0);

	/* Delete GLES objects */
	if (0 != m_po_cs_id)
	{
		gl.deleteProgram(m_po_cs_id);
		m_po_cs_id = 0;
	}

	if (0 != m_cs_id)
	{
		gl.deleteShader(m_cs_id);
		m_cs_id = 0;
	}

	if (0 != m_ssbo_bo_id)
	{
		gl.deleteBuffers(1, &m_ssbo_bo_id);
		m_ssbo_bo_id = 0;
	}
}

void TextureBufferOperations::deinitSecondPhase(void)
{
	/* Get GLES entry points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Reset GLES state */
	gl.bindVertexArray(0);
	gl.bindTexture(GL_TEXTURE_2D, 0);
	gl.bindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	gl.bindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	gl.bindBuffer(GL_ARRAY_BUFFER, 0);

	if (m_vertex_location != -1)
	{
		gl.disableVertexAttribArray(m_vertex_location);
		m_vertex_location = -1;
	}

	if (m_index_location != -1)
	{
		gl.disableVertexAttribArray(m_index_location);
		m_index_location = -1;
	}

	gl.useProgram(0);

	/* Delete GLES objects */
	if (0 != m_po_vs_fs_id)
	{
		gl.deleteProgram(m_po_vs_fs_id);
		m_po_vs_fs_id = 0;
	}

	if (0 != m_fs_id)
	{
		gl.deleteShader(m_fs_id);
		m_fs_id = 0;
	}

	if (0 != m_vs_id)
	{
		gl.deleteShader(m_vs_id);
		m_vs_id = 0;
	}

	if (0 != m_fbo_id)
	{
		gl.deleteFramebuffers(1, &m_fbo_id);
		m_fbo_id = 0;
	}

	if (0 != m_to_id)
	{
		gl.deleteTextures(1, &m_to_id);
		m_to_id = 0;
	}

	if (0 != m_vao_id)
	{
		gl.deleteVertexArrays(1, &m_vao_id);
		m_vao_id = 0;
	}

	if (0 != m_vbo_id)
	{
		gl.deleteBuffers(1, &m_vbo_id);
		m_vbo_id = 0;
	}

	if (0 != m_vbo_indicies_id)
	{
		gl.deleteBuffers(1, &m_vbo_indicies_id);
		m_vbo_indicies_id = 0;
	}
}

/** Constructor for Test Case 1
 *
 *  @param context     Test context
 *  @param name        Test case's name
 *  @param description Test case's description
 **/
TextureBufferOperationsViaBufferObjectLoad::TextureBufferOperationsViaBufferObjectLoad(Context&				context,
																					   const ExtParameters& extParams,
																					   const char*			name,
																					   const char*			description)
	: TextureBufferOperations(context, extParams, name, description)
{
}

/** Initialize texture buffer object with test data using glBufferSubData()
 *
 **/
void TextureBufferOperationsViaBufferObjectLoad::initializeBufferObjectData(void)
{
	/* Get GL entry points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	std::vector<glw::GLint> data(m_n_vectors_in_buffer_texture * m_n_vector_components);
	fillBufferWithData(&data[0], m_n_vectors_in_buffer_texture * m_n_vector_components);

	gl.bufferSubData(m_glExtTokens.TEXTURE_BUFFER, 0,
					 m_n_vectors_in_buffer_texture * m_n_vector_components * sizeof(glw::GLint), &data[0]);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error setting buffer object data!");
}

/** Constructor for Test Case 2
 *
 *  @param context     Test context
 *  @param name        Test case's name
 *  @param description Test case's description
 **/
TextureBufferOperationsViaCPUWrites::TextureBufferOperationsViaCPUWrites(Context&			  context,
																		 const ExtParameters& extParams,
																		 const char* name, const char* description)
	: TextureBufferOperations(context, extParams, name, description)
{
}

/** Initialize texture buffer object with test data using CPU Write
 *
 **/
void TextureBufferOperationsViaCPUWrites::initializeBufferObjectData(void)
{
	/* Get GL entry points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	std::vector<glw::GLint> data(m_n_vectors_in_buffer_texture * m_n_vector_components);
	fillBufferWithData(&data[0], m_n_vectors_in_buffer_texture * m_n_vector_components);

	glw::GLint* tempBuffer = (glw::GLint*)gl.mapBufferRange(
		m_glExtTokens.TEXTURE_BUFFER, 0, m_n_vectors_in_buffer_texture * m_n_vector_components * sizeof(glw::GLint),
		GL_MAP_WRITE_BIT);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error mapping buffer object's data store to client's address space!");

	for (glw::GLuint i = 0; i < data.size(); ++i)
	{
		tempBuffer[i] = data[i];
	}

	gl.unmapBuffer(m_glExtTokens.TEXTURE_BUFFER);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error ummapping buffer object's data store from client's address space!");
}

/** Constructor for Test Case 3
 *
 *  @param context     Test context
 *  @param name        Test case's name
 *  @param description Test case's description
 **/
TextureBufferOperationsViaFrambufferReadBack::TextureBufferOperationsViaFrambufferReadBack(
	Context& context, const ExtParameters& extParams, const char* name, const char* description)
	: TextureBufferOperations(context, extParams, name, description)
	, m_fb_fbo_id(0)
	, m_fb_fs_id(0)
	, m_fb_po_id(0)
	, m_fb_to_id(0)
	, m_fb_vao_id(0)
	, m_fb_vbo_id(0)
	, m_fb_vs_id(0)
	, m_position_location(-1)
{
}

/** Initialize texture buffer object with test data using framebuffer readbacks to pixel buffer object
 *
 **/
void TextureBufferOperationsViaFrambufferReadBack::initializeBufferObjectData()
{
	/* Get GL entry points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	std::vector<glw::GLint> data(m_n_vectors_in_buffer_texture * m_n_vector_components);
	fillBufferWithData(&data[0], m_n_vectors_in_buffer_texture * m_n_vector_components);

	/* Configure vertex array object */
	gl.genVertexArrays(1, &m_fb_vao_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error generating vertex array object!");

	gl.bindVertexArray(m_fb_vao_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error binding vertex array object!");

	/* Prepare framebuffer object */
	gl.genFramebuffers(1, &m_fb_fbo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error genertaing framebuffer object!");

	gl.bindFramebuffer(GL_DRAW_FRAMEBUFFER, m_fb_fbo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error binding framebuffer object!");

	/* Prepare texture object */
	gl.genTextures(1, &m_fb_to_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error generating texture object!");

	gl.bindTexture(GL_TEXTURE_2D, m_fb_to_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error binding texture object!");

	gl.texStorage2D(GL_TEXTURE_2D, 1, GL_RGBA32I, m_n_vectors_in_buffer_texture /* width */, 1 /* height */);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error allocating texture object's data store!");

	/* Attach texture to framebuffer */
	gl.framebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_fb_to_id, 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error attaching texture to framebuffer's color attachment!");

	/* Check framebuffer status */
	checkFramebufferStatus(GL_DRAW_FRAMEBUFFER);

	/* Configure view port */
	gl.viewport(0, 0, m_n_vectors_in_buffer_texture, 1);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error setting viewport!");

	/* Create program object */
	m_fb_po_id = gl.createProgram();
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error creating program object!");

	m_fb_fs_id = gl.createShader(GL_FRAGMENT_SHADER);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error creating fragment shader object!");

	m_fb_vs_id = gl.createShader(GL_VERTEX_SHADER);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error creating vertex shader object!");

	std::string fsSource = getFBFragmentShaderCode();
	std::string vsSource = getFBVertexShaderCode();

	const char* fsCode = fsSource.c_str();
	const char* vsCode = vsSource.c_str();

	if (!buildProgram(m_fb_po_id, m_fb_fs_id, 1, &fsCode, m_fb_vs_id, 1, &vsCode))
	{
		TCU_FAIL("Could not create a program from valid vertex/fragment shader code!");
	}

	/* Full screen quad */
	glw::GLfloat vertices[] = { -1.0f, -1.0f, 0.0f, 1.0f, -1.0f, 1.0f, 0.0f, 1.0f,
								1.0f,  -1.0f, 0.0f, 1.0f, 1.0f,  1.0f, 0.0f, 1.0f };

	/* Generate buffer object */
	gl.genBuffers(1, &m_fb_vbo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error generating buffer object!");

	/* Bind buffer object */
	gl.bindBuffer(GL_ARRAY_BUFFER, m_fb_vbo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error binding buffer object!");

	/* Set data for buffer object */
	gl.bufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error setting data for buffer object!");

	gl.useProgram(m_fb_po_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error setting active program object!");

	m_position_location = gl.getAttribLocation(m_fb_po_id, "inPosition");
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not get attribute location!");
	if (m_position_location == -1)
	{
		TCU_FAIL("Could not get uniform location");
	}

	gl.vertexAttribPointer(m_position_location, 4, GL_FLOAT, GL_FALSE, 0, 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not set vertex attribute pointer!");

	gl.enableVertexAttribArray(m_position_location);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not enable vertex attribute array!");

	/* Clear texture */
	gl.clearColor(0.0f, 0.0f, 0.0f, 0.0f);
	gl.clear(GL_COLOR_BUFFER_BIT);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error clearing color buffer");

	/* Render */
	gl.drawArrays(GL_TRIANGLE_STRIP, 0, 4);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Rendering failed!");

	gl.bindFramebuffer(GL_READ_FRAMEBUFFER, m_fb_fbo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error binding frame buffer object!");

	/* Bind buffer object to pixel pack buffer */
	gl.bindBuffer(GL_PIXEL_PACK_BUFFER, m_tb_bo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error binding buffer object !");

	gl.pixelStorei(GL_UNPACK_ALIGNMENT, 1);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error setting GL_UNPACK_ALIGNMENT parameter to 1");

	gl.pixelStorei(GL_PACK_ALIGNMENT, 1);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error setting GL_PACK_ALIGNMENT parameter to 1");

	/* Fill buffer object with data from framebuffer object's color attachment */
	gl.readPixels(0, 0, m_n_vectors_in_buffer_texture, 1, GL_RGBA_INTEGER, GL_INT, 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error reading pixels !");

	gl.pixelStorei(GL_UNPACK_ALIGNMENT, 4);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error setting GL_UNPACK_ALIGNMENT parameter to default value");

	gl.pixelStorei(GL_PACK_ALIGNMENT, 4);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error setting GL_PACK_ALIGNMENT parameter to default value");

	gl.bindBuffer(GL_PIXEL_PACK_BUFFER, 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error binding buffer object!");

	gl.bindBuffer(m_glExtTokens.TEXTURE_BUFFER, m_tb_bo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error binding buffer object!");

	gl.bindVertexArray(0);
	gl.bindTexture(GL_TEXTURE_2D, 0);
	gl.bindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	gl.bindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	gl.bindBuffer(GL_ARRAY_BUFFER, 0);

	if (glu::isContextTypeES(m_context.getRenderContext().getType()))
		gl.disableVertexAttribArray(m_position_location);

	gl.useProgram(0);

	m_position_location = -1;
}

/** Returns Fragment Shader Code
 *
 * @return pointer to literal with Fragment Shader Code
 */
std::string TextureBufferOperationsViaFrambufferReadBack::getFBFragmentShaderCode() const
{
	std::stringstream strstream;

	strstream << "${VERSION}\n"
				 "\n"
				 "precision highp float;\n"
				 "\n"
				 "out ivec4 color;\n"
				 "\n"
				 "void main(void)\n"
				 "{\n"
				 "    color = ivec4(0, 1, 0, 1);\n"
				 "}\n";

	return strstream.str();
}

/** Returns Vertex Shader Code
 *
 * @return pointer to literal with Vertex Shader Code
 */
std::string TextureBufferOperationsViaFrambufferReadBack::getFBVertexShaderCode() const
{
	std::stringstream strstream;

	strstream << "${VERSION}\n"
				 "\n"
				 "precision highp float;\n"
				 "\n"
				 "in vec4 inPosition;\n"
				 "\n"
				 "void main(void)\n"
				 "{\n"
				 "    gl_Position = inPosition;\n"
				 "}\n";

	return strstream.str();
}

/** Fills buffer with test data
 *
 * @param buffer      pointer to buffer
 * @param bufferLenth buffer length
 */
void TextureBufferOperationsViaFrambufferReadBack::fillBufferWithData(glw::GLint* buffer, glw::GLuint bufferLength)
{
	for (glw::GLuint i = 0; i < bufferLength; ++i)
	{
		buffer[i] = (glw::GLint)(i % 2); /* Reference color is 0, 1, 0, 1 */
	}
}

/** Deinitializes GLES objects created during the test.
 *
 */
void TextureBufferOperationsViaFrambufferReadBack::deinit(void)
{
	/* Get GL entry points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Reset GLES state */
	gl.bindVertexArray(0);
	gl.bindTexture(GL_TEXTURE_2D, 0);
	gl.bindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	gl.bindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	gl.bindBuffer(GL_ARRAY_BUFFER, 0);

	if (m_position_location != -1)
	{
		gl.disableVertexAttribArray(m_position_location);
		m_position_location = -1;
	}

	gl.useProgram(0);

	/* Delete GLES objects */
	if (0 != m_fb_po_id)
	{
		gl.deleteProgram(m_fb_po_id);
		m_fb_po_id = 0;
	}

	if (0 != m_fb_fs_id)
	{
		gl.deleteShader(m_fb_fs_id);
		m_fb_fs_id = 0;
	}

	if (0 != m_fb_vs_id)
	{
		gl.deleteShader(m_fb_vs_id);
		m_fb_vs_id = 0;
	}

	if (0 != m_fb_fbo_id)
	{
		gl.deleteFramebuffers(1, &m_fb_fbo_id);
		m_fb_fbo_id = 0;
	}

	if (0 != m_fb_to_id)
	{
		gl.deleteTextures(1, &m_fb_to_id);
		m_fb_to_id = 0;
	}

	if (0 != m_fb_vbo_id)
	{
		gl.deleteBuffers(1, &m_fb_vbo_id);
		m_fb_vbo_id = 0;
	}

	if (0 != m_fb_vao_id)
	{
		gl.deleteVertexArrays(1, &m_fb_vao_id);
		m_fb_vao_id = 0;
	}

	/* Deinitialize base class */
	TextureBufferOperations::deinit();
}

/** Constructor for Test Case 4
 *
 *  @param context     Test context
 *  @param name        Test case's name
 *  @param description Test case's description
 **/
TextureBufferOperationsViaTransformFeedback::TextureBufferOperationsViaTransformFeedback(Context&			  context,
																						 const ExtParameters& extParams,
																						 const char*		  name,
																						 const char* description)
	: TextureBufferOperations(context, extParams, name, description)
	, m_tf_fs_id(0)
	, m_tf_po_id(0)
	, m_tf_vao_id(0)
	, m_tf_vbo_id(0)
	, m_tf_vs_id(0)
	, m_position_location(-1)
{
}

/** Initialize buffer object with test data using transform feedback
 *
 **/
void TextureBufferOperationsViaTransformFeedback::initializeBufferObjectData()
{
	/* Get GL entry points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	std::vector<glw::GLint> data(m_n_vectors_in_buffer_texture * m_n_vector_components);
	fillBufferWithData(&data[0], m_n_vectors_in_buffer_texture * m_n_vector_components);

	/* Configure vertex array object */
	gl.genVertexArrays(1, &m_tf_vao_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error generating vertex array object!");

	gl.bindVertexArray(m_tf_vao_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error binding vertex array object!");

	/* Configure buffer object*/
	gl.genBuffers(1, &m_tf_vbo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error generating vertex buffer object!");

	gl.bindBuffer(GL_ARRAY_BUFFER, m_tf_vbo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error binding vertex buffer object!");

	gl.bufferData(GL_ARRAY_BUFFER, m_n_vectors_in_buffer_texture * m_n_vector_components * sizeof(glw::GLint), &data[0],
				  GL_DYNAMIC_DRAW);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error allocating buffer object's data store!");

	m_tf_po_id = gl.createProgram();
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error creating program object!");

	/* Setup transform feedback varyings */
	const char* varyings[] = { "outPosition" };
	gl.transformFeedbackVaryings(m_tf_po_id, 1, varyings, GL_SEPARATE_ATTRIBS);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error specifying transform feedback varyings!");

	m_tf_fs_id = gl.createShader(GL_FRAGMENT_SHADER);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error creating fragment shader object!");

	m_tf_vs_id = gl.createShader(GL_VERTEX_SHADER);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error creating vertex shader object!");

	std::string fsSource = getTFFragmentShaderCode();
	std::string vsSource = getTFVertexShaderCode();

	const char* fsCode = fsSource.c_str();
	const char* vsCode = vsSource.c_str();

	if (!buildProgram(m_tf_po_id, m_tf_fs_id, 1, &fsCode, m_tf_vs_id, 1, &vsCode))
	{
		TCU_FAIL("Could not create a program from valid vertex/fragment shader code!");
	}

	gl.useProgram(m_tf_po_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error setting active program object!");

	m_position_location = gl.getAttribLocation(m_tf_po_id, "inPosition");
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error getting attrib location!");

	gl.vertexAttribIPointer(m_position_location, m_n_vector_components, GL_INT, 0, 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error setting vertex attrib pointer!");

	gl.enableVertexAttribArray(m_position_location);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error enabling vertex attrib array!");

	gl.bindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, m_tb_bo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error binding buffer object to transform feedback binding point!");

	gl.beginTransformFeedback(GL_POINTS);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error starting transform feedback!");

	/* Render */
	gl.drawArrays(GL_POINTS, 0, m_n_vectors_in_buffer_texture);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error during rendering!");

	gl.endTransformFeedback();
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error ending transform feedback!");

	gl.bindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, 0);
	gl.bindBuffer(GL_ARRAY_BUFFER, 0);
	gl.bindVertexArray(0);

	if (glu::isContextTypeES(m_context.getRenderContext().getType()))
		gl.disableVertexAttribArray(m_position_location);

	gl.useProgram(0);

	m_position_location = -1;
}

/** Returns Fragment shader Code
 *
 * @return pointer to literal with Fragment Shader Code
 */
std::string TextureBufferOperationsViaTransformFeedback::getTFFragmentShaderCode() const
{
	std::stringstream strstream;

	strstream << "${VERSION}\n"
				 "\n"
				 "${TEXTURE_BUFFER_REQUIRE}\n"
				 "\n"
				 "in flat ivec4 outPosition;\n"
				 "\n"
				 "precision highp float;\n"
				 "\n"
				 "void main(void)\n"
				 "{\n"
				 "}\n";

	return strstream.str();
}

/** Returns Vertex Shader Code
 *
 * @return pointer to literal with Vertex Shader Code
 */
std::string TextureBufferOperationsViaTransformFeedback::getTFVertexShaderCode() const
{
	std::stringstream strstream;

	strstream << "${VERSION}\n"
				 "\n"
				 "${TEXTURE_BUFFER_REQUIRE}\n"
				 "\n"
				 "precision highp float;\n"
				 "\n"
				 "in ivec4 inPosition;\n"
				 "\n"
				 "flat out ivec4 outPosition;\n"
				 "\n"
				 "void main(void)\n"
				 "{\n"
				 "    gl_Position = vec4(0.0, 0.0, 0.0, 0.0);\n"
				 "    outPosition = inPosition;\n"
				 "}\n";

	return strstream.str();
}

/** Deinitializes GLES objects created during the test. */
void TextureBufferOperationsViaTransformFeedback::deinit(void)
{
	/* Get GL entry points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Reset GLES state */
	gl.bindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, 0);
	gl.bindBuffer(GL_ARRAY_BUFFER, 0);
	gl.bindVertexArray(0);

	if (m_position_location != -1)
	{
		gl.disableVertexAttribArray(m_position_location);
		m_position_location = -1;
	}

	gl.useProgram(0);

	/* Delete GLES objects */
	if (0 != m_tf_po_id)
	{
		gl.deleteProgram(m_tf_po_id);
		m_tf_po_id = 0;
	}

	if (0 != m_tf_vs_id)
	{
		gl.deleteShader(m_tf_vs_id);
		m_tf_vs_id = 0;
	}

	if (0 != m_tf_fs_id)
	{
		gl.deleteShader(m_tf_fs_id);
		m_tf_fs_id = 0;
	}

	if (0 != m_tf_vbo_id)
	{
		gl.deleteBuffers(1, &m_tf_vbo_id);
		m_tf_vbo_id = 0;
	}

	if (0 != m_tf_vao_id)
	{
		gl.deleteVertexArrays(1, &m_tf_vao_id);
		m_tf_vao_id = 0;
	}

	/* Deinitialize base class */
	TextureBufferOperations::deinit();
}

const glw::GLuint TextureBufferOperationsViaImageStore::m_image_unit = 0;

/** Constructor for Test Case 5
 *
 *  @param context     Test context
 *  @param name        Test case's name
 *  @param description Test case's description
 **/
TextureBufferOperationsViaImageStore::TextureBufferOperationsViaImageStore(Context&				context,
																		   const ExtParameters& extParams,
																		   const char* name, const char* description)
	: TextureBufferOperations(context, extParams, name, description), m_is_cs_id(0), m_is_po_id(0)
{
}

/** Initialize buffer object with test data using image store
 *
 **/
void TextureBufferOperationsViaImageStore::initializeBufferObjectData()
{
	/* Get Gl entry points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Configure program object */
	m_is_po_id = gl.createProgram();
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error creating program object!");

	m_is_cs_id = gl.createShader(GL_COMPUTE_SHADER);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error creating compute shader object!");

	std::string csSource = getISComputeShaderCode();
	const char* csCode   = csSource.c_str();

	if (!buildProgram(m_is_po_id, m_is_cs_id, 1, &csCode))
	{
		TCU_FAIL("Could not create a program from valid compute shader source!");
	}

	gl.useProgram(m_is_po_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error setting active program object!");

	gl.bindImageTexture(m_image_unit, m_texbuff_id, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32I);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error binding texture object to image unit!");

	gl.dispatchCompute(1, 1, 1);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error running compute shader!");

	gl.memoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error setting memory barrier!");

	gl.useProgram(0);
}

/** Returns Compute Shader Code
 *
 * @return pointer to literal with Compute Shader Code
 */
std::string TextureBufferOperationsViaImageStore::getISComputeShaderCode() const
{
	std::stringstream strstream;

	strstream << "${VERSION}\n"
				 "\n"
				 "${TEXTURE_BUFFER_REQUIRE}\n"
				 "\n"
				 "precision highp float;\n"
				 "\n"
				 "layout(rgba32i, binding = 0) uniform writeonly highp iimageBuffer image_buffer;\n"
				 "\n"
				 "layout (local_size_x = "
			  << m_n_vectors_in_buffer_texture
			  << " ) in;\n"
				 "\n"
				 "void main(void)\n"
				 "{\n"
				 "    imageStore(image_buffer, int(gl_LocalInvocationID.x), ivec4(gl_LocalInvocationID.x) );\n"
				 "}\n";

	return strstream.str();
}

/** Fill buffer with test data
 *
 * @param buffer      pointer to buffer
 * @param bufferLenth buffer length
 */
void TextureBufferOperationsViaImageStore::fillBufferWithData(glw::GLint* buffer, glw::GLuint bufferLength)
{
	for (glw::GLuint i = 0; i < bufferLength; ++i)
	{
		buffer[i] = (glw::GLint)(i / m_n_vector_components);
	}
}

/** Deinitializes GLES objects created during the test. */
void TextureBufferOperationsViaImageStore::deinit(void)
{
	/* Get Gl entry points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Reset GLES state */
	gl.useProgram(0);

	/* Delete GLES objects */
	if (0 != m_is_po_id)
	{
		gl.deleteProgram(m_is_po_id);
		m_is_po_id = 0;
	}

	if (0 != m_is_cs_id)
	{
		gl.deleteShader(m_is_cs_id);
		m_is_cs_id = 0;
	}

	/* Deinitalize base class */
	TextureBufferOperations::deinit();
}

/** Constructor for Test Case 6
 *
 *  @param context     Test context
 *  @param name        Test case's name
 *  @param description Test case's description
 **/
TextureBufferOperationsViaSSBOWrites::TextureBufferOperationsViaSSBOWrites(Context&				context,
																		   const ExtParameters& extParams,
																		   const char* name, const char* description)
	: TextureBufferOperations(context, extParams, name, description), m_ssbo_cs_id(0), m_ssbo_po_id(0)
{
}

/** Initialize buffer object with test data using ssbo writes
 *
 **/
void TextureBufferOperationsViaSSBOWrites::initializeBufferObjectData()
{
	/* Get Gl entry points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Configure program object */
	m_ssbo_po_id = gl.createProgram();
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error creating program object!");

	m_ssbo_cs_id = gl.createShader(GL_COMPUTE_SHADER);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error creating compute shader object!");

	std::string csSource = getSSBOComputeShaderCode();
	const char* csCode   = csSource.c_str();

	if (!buildProgram(m_ssbo_po_id, m_ssbo_cs_id, 1, &csCode))
	{
		TCU_FAIL("Could not create a program from valid compute shader source!");
	}

	gl.bindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_tb_bo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not bind buffer object to shader storage binding point!");

	gl.useProgram(m_ssbo_po_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error setting active program object!");

	gl.dispatchCompute(1, 1, 1);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error running compute shader!");

	gl.memoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error setting memory barrier!");

	gl.bindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	gl.useProgram(0);
}

/** Returns Compute Shader Code
 *
 * @return pointer to literal with Compute Shader Code
 */
std::string TextureBufferOperationsViaSSBOWrites::getSSBOComputeShaderCode() const
{
	std::stringstream strstream;

	strstream << "${VERSION}\n"
				 "\n"
				 "${TEXTURE_BUFFER_REQUIRE}\n"
				 "\n"
				 "precision highp float;\n"
				 "\n"
				 "buffer ComputeSSBO\n"
				 "{\n"
				 "    ivec4 value[];\n"
				 "} computeSSBO;\n"
				 "\n"
				 "layout (local_size_x = "
			  << m_n_vectors_in_buffer_texture
			  << " ) in;\n"
				 "\n"
				 "void main(void)\n"
				 "{\n"
				 "    computeSSBO.value[gl_LocalInvocationID.x] = ivec4(gl_LocalInvocationID.x);\n"
				 "}\n";

	return strstream.str();
}

/** Fill buffer with test data
 *
 * @param buffer      pointer to buffer
 * @param bufferLenth buffer length
 */
void TextureBufferOperationsViaSSBOWrites::fillBufferWithData(glw::GLint* buffer, glw::GLuint bufferLength)
{
	for (glw::GLuint i = 0; i < bufferLength; ++i)
	{
		buffer[i] = (glw::GLint)(i / m_n_vector_components);
	}
}

/** Deinitializes GLES objects created during the test. */
void TextureBufferOperationsViaSSBOWrites::deinit(void)
{
	/* Get Gl entry points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Reset GLES state */
	gl.bindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	gl.useProgram(0);

	/* Delete GLES objects */
	if (0 != m_ssbo_po_id)
	{
		gl.deleteProgram(m_ssbo_po_id);
		m_ssbo_po_id = 0;
	}

	if (0 != m_ssbo_cs_id)
	{
		gl.deleteShader(m_ssbo_cs_id);
		m_ssbo_cs_id = 0;
	}

	/* Deinitalize base class */
	TextureBufferOperations::deinit();
}

} // namespace glcts
