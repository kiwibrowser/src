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
 * \file esextcGPUShader5SamplerArrayIndexing.cpp
 * \brief  gpu_shader5 extension - Sampler Array Indexing (Test 1)
 */ /*-------------------------------------------------------------------*/

#include "esextcGPUShader5SamplerArrayIndexing.hpp"

#include "gluDefs.hpp"
#include "glwEnums.hpp"
#include "glwFunctions.hpp"
#include "tcuTestLog.hpp"
#include <cstring>

namespace glcts
{

const int GPUShader5SamplerArrayIndexing::m_n_small_textures	 = 4;
const int GPUShader5SamplerArrayIndexing::m_n_texture_components = 4;
const int GPUShader5SamplerArrayIndexing::m_big_texture_height   = 3;
const int GPUShader5SamplerArrayIndexing::m_big_texture_width	= 3;
const int GPUShader5SamplerArrayIndexing::m_n_texture_levels	 = 1;
const int GPUShader5SamplerArrayIndexing::m_small_texture_height = 1;
const int GPUShader5SamplerArrayIndexing::m_small_texture_width  = 1;

/** Constructor
 *
 * @param context       Test context
 * @param name          Test case's name
 * @param description   Test case's description
 **/
GPUShader5SamplerArrayIndexing::GPUShader5SamplerArrayIndexing(Context& context, const ExtParameters& extParams,
															   const char* name, const char* description)
	: TestCaseBase(context, extParams, name, description)
	, m_big_to_id(0)
	, m_fbo_id(0)
	, m_fs_id(0)
	, m_po_id(0)
	, m_small_to_ids(DE_NULL)
	, m_vao_id(0)
	, m_vbo_id(0)
	, m_vs_id(0)
{
}

/** Deinitializes GLES objects created during the test.
 *
 */
void GPUShader5SamplerArrayIndexing::deinit(void)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Reset OpenGL ES state */
	gl.useProgram(0);
	gl.bindBuffer(GL_ARRAY_BUFFER, 0);
	gl.activeTexture(GL_TEXTURE0);
	gl.bindTexture(GL_TEXTURE_2D, 0);
	gl.bindFramebuffer(GL_FRAMEBUFFER, 0);
	gl.bindVertexArray(0);

	if (m_po_id != 0)
	{
		gl.deleteProgram(m_po_id);
		m_po_id = 0;
	}

	if (m_fs_id != 0)
	{
		gl.deleteShader(m_fs_id);
		m_fs_id = 0;
	}

	if (m_vs_id != 0)
	{
		gl.deleteShader(m_vs_id);
		m_vs_id = 0;
	}

	if (m_vbo_id != 0)
	{
		gl.deleteBuffers(1, &m_vbo_id);
		m_vbo_id = 0;
	}

	if (m_fbo_id != 0)
	{
		gl.deleteFramebuffers(1, &m_fbo_id);
		m_fbo_id = 0;
	}

	if (m_vao_id != 0)
	{
		gl.deleteVertexArrays(1, &m_vao_id);
		m_vao_id = 0;
	}

	if (m_big_to_id != 0)
	{
		gl.deleteTextures(1, &m_big_to_id);
		m_big_to_id = 0;
	}

	if (m_small_to_ids != DE_NULL)
	{
		gl.deleteTextures(m_n_small_textures, m_small_to_ids);
		delete[] m_small_to_ids;
		m_small_to_ids = DE_NULL;
	}

	/* Release base class */
	TestCaseBase::deinit();
}

/** Initializes GLES objects used during the test.
 *
 */
void GPUShader5SamplerArrayIndexing::initTest(void)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Check if gpu_shader5 extension is supported */
	if (!m_is_gpu_shader5_supported)
	{
		throw tcu::NotSupportedError(GPU_SHADER5_EXTENSION_NOT_SUPPORTED, "", __FILE__, __LINE__);
	}

	/* Create shader objects */
	m_fs_id = gl.createShader(GL_FRAGMENT_SHADER);
	m_vs_id = gl.createShader(GL_VERTEX_SHADER);

	/* Create progream object */
	m_po_id = gl.createProgram();

	const char* fsCode = getFragmentShaderCode();
	const char* vsCode = getVertexShaderCode();

	if (!buildProgram(m_po_id, m_fs_id, 1 /* part */, &fsCode, m_vs_id, 1 /* part */, &vsCode))
	{
		TCU_FAIL("Could not create program object!");
	}

	/* Create and bind vertex array object */
	gl.genVertexArrays(1, &m_vao_id);
	gl.bindVertexArray(m_vao_id);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Error configuring vertex array object");

	/* Configure vertex buffer */
	const glw::GLfloat vertices[] = { -1.0f, -1.0f, 0.0f, 1.0f, 1.0f, -1.0f, 0.0f, 1.0f,
									  -1.0f, 1.0f,  0.0f, 1.0f, 1.0f, 1.0f,  0.0f, 1.0f };

	gl.genBuffers(1, &m_vbo_id);
	gl.bindBuffer(GL_ARRAY_BUFFER, m_vbo_id);
	gl.bufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Error creating vertex buffer object!");

	/* Create and configure texture object used as color attachment */
	gl.genTextures(1, &m_big_to_id);
	gl.bindTexture(GL_TEXTURE_2D, m_big_to_id);
	gl.texStorage2D(GL_TEXTURE_2D, m_n_texture_levels, GL_RGBA8, m_big_texture_width, m_big_texture_height);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error configuring texture object!");

	/* Create and configure the framebuffer object */
	gl.genFramebuffers(1, &m_fbo_id);
	gl.bindFramebuffer(GL_DRAW_FRAMEBUFFER, m_fbo_id);
	gl.framebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_big_to_id, 0 /* level */);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error configuring framebuffer object!");

	/* Configure textures used in fragment shader */
	const glw::GLfloat alpha[] = { 0.0f, 0.0f, 0.0f, 1.0f };
	const glw::GLfloat blue[]  = { 0.0f, 0.0f, 1.0f, 0.0f };
	const glw::GLfloat green[] = { 0.0f, 1.0f, 0.0f, 0.0f };
	const glw::GLfloat red[]   = { 1.0f, 0.0f, 0.0f, 0.0f };

	m_small_to_ids = new glw::GLuint[m_n_small_textures];
	memset(m_small_to_ids, 0, m_n_small_textures * sizeof(glw::GLuint));

	gl.genTextures(m_n_small_textures, m_small_to_ids);

	gl.activeTexture(GL_TEXTURE0);
	gl.bindTexture(GL_TEXTURE_2D, m_small_to_ids[0]);
	gl.texImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, m_small_texture_width, m_small_texture_height, 0 /* border */, GL_RGBA,
				  GL_FLOAT, red);
	gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error configuring texture object");

	gl.activeTexture(GL_TEXTURE1);
	gl.bindTexture(GL_TEXTURE_2D, m_small_to_ids[1]);
	gl.texImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, m_small_texture_width, m_small_texture_height, 0 /* border */, GL_RGBA,
				  GL_FLOAT, green);
	gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error configuring texture object");

	gl.activeTexture(GL_TEXTURE2);
	gl.bindTexture(GL_TEXTURE_2D, m_small_to_ids[2]);
	gl.texImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, m_small_texture_width, m_small_texture_height, 0 /* border */, GL_RGBA,
				  GL_FLOAT, blue);
	gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error configuring texture object");

	gl.activeTexture(GL_TEXTURE3);
	gl.bindTexture(GL_TEXTURE_2D, m_small_to_ids[3]);
	gl.texImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, m_small_texture_width, m_small_texture_height, 0 /* border */, GL_RGBA,
				  GL_FLOAT, alpha);
	gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error configuring texture object");
}

/** Executes the test.
 *  Sets the test result to QP_TEST_RESULT_FAIL if the test failed, QP_TEST_RESULT_PASS otherwise.
 *
 *  @return STOP if the test has finished, CONTINUE to indicate iterate should be called once again.
 *
 *  Note the function throws exception should an error occur!
 **/
tcu::TestNode::IterateResult GPUShader5SamplerArrayIndexing::iterate(void)
{
	initTest();

	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.viewport(0 /* x */, 0 /* y */, m_big_texture_width, m_big_texture_height);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glViewport() call failed");

	gl.useProgram(m_po_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram() call failed");

	/* Configure position vertex array */
	gl.bindBuffer(GL_ARRAY_BUFFER, m_vbo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer() call failed");

	glw::GLint position_attribute_location = gl.getAttribLocation(m_po_id, "position");

	gl.vertexAttribPointer(position_attribute_location, 4 /* size */, GL_FLOAT, GL_FALSE, 0 /* stride */,
						   DE_NULL /* pointer */);
	gl.enableVertexAttribArray(position_attribute_location);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error configuring position vertex attribute array!");

	glw::GLint samplers_uniform_location = gl.getUniformLocation(m_po_id, "samplers");

	gl.activeTexture(GL_TEXTURE0);
	gl.bindTexture(GL_TEXTURE_2D, m_small_to_ids[0]);
	gl.uniform1i(samplers_uniform_location + 0, 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error assigning texture unit 0 to samplers[0] uniform location!");

	gl.activeTexture(GL_TEXTURE1);
	gl.bindTexture(GL_TEXTURE_2D, m_small_to_ids[1]);
	gl.uniform1i(samplers_uniform_location + 1, 1);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error assigning texture unit 1 to samplers[1] uniform location!");

	gl.activeTexture(GL_TEXTURE2);
	gl.bindTexture(GL_TEXTURE_2D, m_small_to_ids[2]);
	gl.uniform1i(samplers_uniform_location + 2, 2);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error assigning texture unit 2 to samplers[2] uniform location!");

	gl.activeTexture(GL_TEXTURE3);
	gl.bindTexture(GL_TEXTURE_2D, m_small_to_ids[3]);
	gl.uniform1i(samplers_uniform_location + 3, 3);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error assigning texture unit 3 to samplers[3] uniform location!");

	/* Render */
	gl.clearColor(0.0f, 0.0f, 0.0f, 0.0f);
	gl.clear(GL_COLOR_BUFFER_BIT);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Error clearing color buffer!");

	gl.drawArrays(GL_TRIANGLE_STRIP, 0 /* first */, 4 /* count */);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Rendering error");

	/* Verify results */
	const glw::GLubyte referenceColor[] = { 255, 255, 255, 255 };
	glw::GLubyte	   buffer[m_n_texture_components];

	memset(buffer, 0, m_n_texture_components * sizeof(glw::GLubyte));

	/* Reading data */
	gl.bindFramebuffer(GL_READ_FRAMEBUFFER, m_fbo_id);
	gl.readPixels(1, /* x */
				  1, /* y */
				  1, /* width */
				  1, /* height */
				  GL_RGBA, GL_UNSIGNED_BYTE, buffer);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error reading pixel data!");

	/* Fail if result color is different from reference color */
	if (memcmp(referenceColor, buffer, sizeof(referenceColor)))
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Rendered color [" << (int)buffer[0] << ", " << (int)buffer[1]
						   << ", " << (int)buffer[2] << ", " << (int)buffer[3]
						   << "] is different from reference color [" << (int)referenceColor[0] << ", "
						   << (int)referenceColor[1] << ", " << (int)referenceColor[2] << ", " << (int)referenceColor[3]
						   << "] !" << tcu::TestLog::EndMessage;

		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
		return STOP;
	}

	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	return STOP;
}

/** Returns code for Vertex Shader
 *
 * @return pointer to literal with Vertex Shader code
 **/
const char* GPUShader5SamplerArrayIndexing::getVertexShaderCode()
{
	static const char* result = "${VERSION}\n"
								"\n"
								"${GPU_SHADER5_REQUIRE}\n"
								"\n"
								"precision highp float;\n"
								"\n"
								"in vec4 position;"
								"\n"
								"void main()\n"
								"{\n"
								"   gl_Position = position;"
								"}\n";

	return result;
}

/** Returns code for Fragment Shader
 *
 * @return pointer to literal with Fragment Shader code
 **/
const char* GPUShader5SamplerArrayIndexing::getFragmentShaderCode()
{
	static const char* result = "${VERSION}\n"
								"\n"
								"${GPU_SHADER5_REQUIRE}\n"
								"\n"
								"precision highp float;\n"
								"\n"
								"uniform sampler2D samplers[4];\n"
								"\n"
								"layout(location = 0) out vec4 outColor;\n"
								"\n"
								"void main(void)\n"
								"{\n"
								"    outColor = vec4(0, 0, 0, 0);\n"
								"\n"
								"    for (int i = 0;i < 4; ++i)\n"
								"    {\n"
								"        outColor +=  texture(samplers[i],vec2(0,0));\n"
								"    }\n"
								"}\n";

	return result;
}

} // namespace glcts
