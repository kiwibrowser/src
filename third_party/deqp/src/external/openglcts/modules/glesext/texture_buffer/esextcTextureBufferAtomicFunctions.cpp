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
 * \file  esextcTextureBufferAtomicFunctions.cpp
 * \brief Texture Buffer Atomic Functions (Test 5)
 */ /*-------------------------------------------------------------------*/

#include "esextcTextureBufferAtomicFunctions.hpp"
#include "gluContextInfo.hpp"
#include "gluDefs.hpp"
#include "glwEnums.hpp"
#include "glwFunctions.hpp"
#include "tcuTestLog.hpp"
#include <vector>

namespace glcts
{

/** Constructor
 *
 *  @param context     Test context
 *  @param name        Test case's name
 *  @param description Test case's description
 **/
TextureBufferAtomicFunctions::TextureBufferAtomicFunctions(Context& context, const ExtParameters& extParams,
														   const char* name, const char* description)
	: TestCaseBase(context, extParams, name, description)
	, m_cs_id(0)
	, m_po_id(0)
	, m_tbo_id(0)
	, m_tbo_tex_id(0)
	, m_n_texels_in_texture_buffer(0)
{
}

/** Initializes GLES objects used during the test */
void TextureBufferAtomicFunctions::initTest(void)
{
	/* Check if required extensions are supported */
	if (!m_is_texture_buffer_supported)
	{
		throw tcu::NotSupportedError(TEXTURE_BUFFER_EXTENSION_NOT_SUPPORTED, "", __FILE__, __LINE__);
	}

	if (!m_is_shader_image_atomic_supported)
	{
		throw tcu::NotSupportedError(SHADER_IMAGE_ATOMIC_EXTENSION_NOT_SUPPORTED, "", __FILE__, __LINE__);
	}

	/* Get GL entry points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	glw::GLint work_group_size = 0;
	gl.getIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, &work_group_size);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error getting GL_MAX_COMPUTE_WORK_GROUP_SIZE parameter value!");

	m_n_texels_in_texture_buffer = (glw::GLuint)work_group_size + 1;
	std::vector<glw::GLuint> data_buffer(m_n_texels_in_texture_buffer);

	for (glw::GLuint i = 0; i < m_n_texels_in_texture_buffer; ++i)
	{
		data_buffer[i] = i;
	}

	/* Create buffer object */
	gl.genBuffers(1, &m_tbo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error generating buffer object!");

	gl.bindBuffer(m_glExtTokens.TEXTURE_BUFFER, m_tbo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error binding buffer object!");

	gl.bufferData(m_glExtTokens.TEXTURE_BUFFER, m_n_texels_in_texture_buffer * sizeof(glw::GLuint), &data_buffer[0],
				  GL_DYNAMIC_DRAW);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error allocating buffer object's data store!");

	/* Initialize texture buffer */
	gl.genTextures(1, &m_tbo_tex_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error generating texture object!");

	gl.activeTexture(GL_TEXTURE0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error setting active texture unit!");

	gl.bindTexture(m_glExtTokens.TEXTURE_BUFFER, m_tbo_tex_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error binding texture object to GL_TEXTURE_BUFFER_EXT target!");

	gl.texBuffer(m_glExtTokens.TEXTURE_BUFFER, GL_R32UI, m_tbo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error setting buffer object as texture buffer's data store!");

	/* Create program object */
	m_po_id = gl.createProgram();
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error creating program object!");

	m_cs_id = gl.createShader(GL_COMPUTE_SHADER);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error creating shader object!");

	std::string csSource = getComputeShaderCode(work_group_size);
	const char* csCode   = csSource.c_str();

	if (!buildProgram(m_po_id, m_cs_id, 1, &csCode))
	{
		TCU_FAIL("Could not create a program object from valid compute shader object!");
	}
}

/** Returns Compute shader Code
 *
 * @return pointer to literal with Compute Shader Code
 */
std::string TextureBufferAtomicFunctions::getComputeShaderCode(glw::GLint work_group_size) const
{
	std::stringstream strstream;

	strstream << "${VERSION}\n"
				 "\n"
				 "${TEXTURE_BUFFER_REQUIRE}\n"
				 "${SHADER_IMAGE_ATOMIC_REQUIRE}\n"
				 "\n"
				 "precision highp float;\n"
				 "\n"
				 "layout (local_size_x = "
			  << work_group_size << " ) in;\n"
									"\n"
									"layout(r32ui, binding = 0) coherent uniform highp uimageBuffer uimage_buffer;\n"
									"\n"
									"void main(void)\n"
									"{\n"
									"   uint value = imageLoad( uimage_buffer, int(gl_LocalInvocationID.x) + 1 ).x;\n"
									"   imageAtomicAdd( uimage_buffer, 0 , value );\n"
									"\n"
									"   memoryBarrier();\n"
									"   barrier();\n"
									"\n"
									"   value = imageLoad( uimage_buffer, 0 ).x;\n"
									"   imageAtomicCompSwap( uimage_buffer, int(gl_LocalInvocationID.x) + 1, "
									"gl_LocalInvocationID.x + 1u, value );\n"
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
tcu::TestNode::IterateResult TextureBufferAtomicFunctions::iterate(void)
{
	/* Initialize */
	initTest();

	/* Get Gl entry points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	bool test_result = true;

	gl.useProgram(m_po_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error setting active program object!");

	gl.bindImageTexture(0, m_tbo_tex_id, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32UI);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error binding texture object to image unit 0!");

	gl.dispatchCompute(1, 1, 1);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error running compute shader!");

	gl.memoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error setting memory barrier!");

	/* Get result data */
	glw::GLuint* result = (glw::GLuint*)gl.mapBufferRange(
		m_glExtTokens.TEXTURE_BUFFER, 0, m_n_texels_in_texture_buffer * sizeof(glw::GLuint), GL_MAP_READ_BIT);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error mapping buffer object's data store to client address space!");

	glw::GLuint expected_value = (m_n_texels_in_texture_buffer * (m_n_texels_in_texture_buffer - 1)) / 2;

	for (glw::GLuint i = 0; i < m_n_texels_in_texture_buffer; ++i)
	{
		/* Log error if expected data and result data are not equal */
		if (result[i] != expected_value)
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "Result is different than expected at index: " << i << "\n"
							   << "Expected value: " << expected_value << "\n"
							   << "Result   value: " << result[i] << "\n"
							   << tcu::TestLog::EndMessage;

			test_result = false;
			break;
		}
	}

	gl.unmapBuffer(m_glExtTokens.TEXTURE_BUFFER);

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

/** Deinitializes GLES objects created during the test */
void TextureBufferAtomicFunctions::deinit(void)
{
	/* Get Gl entry points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Reset GLES state */
	gl.useProgram(0);
	gl.bindBuffer(m_glExtTokens.TEXTURE_BUFFER, 0);
	gl.bindTexture(m_glExtTokens.TEXTURE_BUFFER, 0);

	/* Delete GLES objects */
	if (0 != m_po_id)
	{
		gl.deleteProgram(m_po_id);
		m_po_id = 0;
	}

	if (0 != m_cs_id)
	{
		gl.deleteShader(m_cs_id);
		m_cs_id = 0;
	}

	if (0 != m_tbo_tex_id)
	{
		gl.deleteTextures(1, &m_tbo_tex_id);
		m_tbo_tex_id = 0;
	}

	if (0 != m_tbo_id)
	{
		gl.deleteBuffers(1, &m_tbo_id);
		m_tbo_id = 0;
	}

	/* Deinitialize base class */
	TestCaseBase::deinit();
}

} // namespace glcts
