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
 * \file  esextcTextureBufferParamValueIntToFloatConversion.cpp
 * \brief Texture Buffer - Integer To Float Conversion (Test 4)
 */ /*-------------------------------------------------------------------*/

#include "esextcTextureBufferParamValueIntToFloatConversion.hpp"
#include "gluContextInfo.hpp"
#include "gluDefs.hpp"
#include "glwEnums.hpp"
#include "glwFunctions.hpp"
#include "tcuTestLog.hpp"
#include <cmath>
#include <vector>

namespace glcts
{

const glw::GLfloat TextureBufferParamValueIntToFloatConversion::m_epsilon = 1.0f / 256.0f;
glw::GLuint		   TextureBufferParamValueIntToFloatConversion::m_texture_buffer_size;
glw::GLuint		   TextureBufferParamValueIntToFloatConversion::m_work_group_x_size;
glw::GLuint		   TextureBufferParamValueIntToFloatConversion::m_work_group_y_size;

/** Constructor
 *
 *  @param context     Test context
 *  @param name        Test case's name
 *  @param description Test case's description
 **/
TextureBufferParamValueIntToFloatConversion::TextureBufferParamValueIntToFloatConversion(Context&			  context,
																						 const ExtParameters& extParams,
																						 const char*		  name,
																						 const char* description)
	: TestCaseBase(context, extParams, name, description)
	, m_cs_id(0)
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
void TextureBufferParamValueIntToFloatConversion::initTest(void)
{
	/* Check if texture buffer extension is supported */
	if (!m_is_texture_buffer_supported)
	{
		throw tcu::NotSupportedError(TEXTURE_BUFFER_EXTENSION_NOT_SUPPORTED, "", __FILE__, __LINE__);
	}

	/* Get GL entry points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	glw::GLint max_workgroup_size;
	gl.getIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, &max_workgroup_size);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error getting GL_MAX_COMPUTE_WORK_GROUP_SIZE parameter value!");

	m_work_group_x_size = m_work_group_y_size = de::min((glw::GLuint)floor(sqrt((float)max_workgroup_size)), 16u);
	m_texture_buffer_size					  = m_work_group_x_size * m_work_group_y_size;

	glw::GLubyte* dataBuffer = new glw::GLubyte[m_texture_buffer_size];

	for (glw::GLuint i = 0; i < m_texture_buffer_size; ++i)
	{
		dataBuffer[i] = (glw::GLubyte)i;
	}

	/* Create texture buffer object*/
	gl.genBuffers(1, &m_tbo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error generating buffer object!");
	gl.bindBuffer(m_glExtTokens.TEXTURE_BUFFER, m_tbo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error binding buffer object!");
	gl.bufferData(m_glExtTokens.TEXTURE_BUFFER, m_texture_buffer_size * sizeof(glw::GLubyte), &dataBuffer[0],
				  GL_STATIC_READ);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error allocating buffer object's data store!");

	delete[] dataBuffer;

	/* Initialize texture buffer */
	gl.genTextures(1, &m_tbo_tex_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error generating texture object!");
	gl.activeTexture(GL_TEXTURE0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error setting active texture unit!");
	gl.bindTexture(m_glExtTokens.TEXTURE_BUFFER, m_tbo_tex_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error binding texture object to GL_TEXTURE_BUFFER_EXT target!");

	gl.texBuffer(m_glExtTokens.TEXTURE_BUFFER, GL_R8, m_tbo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error setting buffer object as texture buffer's data store!");

	/* Create program */
	m_po_id = gl.createProgram();
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error creating program object!");

	m_cs_id = gl.createShader(GL_COMPUTE_SHADER);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error creating shader object!");

	std::string csSource = getComputeShaderCode();
	const char* csCode   = csSource.c_str();

	if (!buildProgram(m_po_id, m_cs_id, 1, &csCode))
	{
		TCU_FAIL("Could not create a program object from valid compute shader object!");
	}

	/* Create Shader Storage Buffer Object */
	gl.genBuffers(1, &m_ssbo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error generating buffer object!");
	gl.bindBuffer(GL_ARRAY_BUFFER, m_ssbo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error binding  buffer object!");
	gl.bufferData(GL_ARRAY_BUFFER, m_texture_buffer_size * sizeof(glw::GLfloat), 0, GL_DYNAMIC_COPY);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error generating buffer object!");
}

/** Returns Compute shader Code
 *
 * @return pointer to literal with Compute Shader Code
 */
std::string TextureBufferParamValueIntToFloatConversion::getComputeShaderCode()
{
	std::stringstream result;

	result << "${VERSION}\n"
			  "\n"
			  "${TEXTURE_BUFFER_REQUIRE}\n"
			  "\n"
			  "precision highp float;\n"
			  "\n"
			  "uniform highp samplerBuffer sampler_buffer;\n"
			  "\n"
			  "layout(std430) buffer ComputeSSBO\n"
			  "{\n"
			  "    float value[];\n"
			  "} computeSSBO;\n"
			  "\n"
			  "layout (local_size_x = "
		   << m_work_group_x_size << ", local_size_y = " << m_work_group_y_size
		   << ") in;\n"
			  "\n"
			  "void main(void)\n"
			  "{\n"
			  "    int index = int(gl_LocalInvocationID.x * gl_WorkGroupSize.y + gl_LocalInvocationID.y);\n"
			  "\n"
			  "    computeSSBO.value[index] = texelFetch( sampler_buffer, index ).x;\n"
			  "}\n";

	return result.str();
}

/** Executes the test.
 *
 *  Sets the test result to QP_TEST_RESULT_FAIL if the test failed, QP_TEST_RESULT_PASS otherwise.
 *
 *  Note the function throws exception should an error occur!
 *
 *  @return STOP if the test has finished, CONTINUE to indicate iterate should be called once again.
 **/
tcu::TestNode::IterateResult TextureBufferParamValueIntToFloatConversion::iterate(void)
{
	initTest();

	/* Get Gl entry points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	bool testResult = true;

	gl.useProgram(m_po_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error using program!");

	gl.bindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_ssbo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not bind buffer object to shader storage binding point!");

	gl.activeTexture(GL_TEXTURE0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error setting active texture unit!");

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
	glw::GLfloat* result = (glw::GLfloat*)gl.mapBufferRange(
		GL_SHADER_STORAGE_BUFFER, 0, m_texture_buffer_size * sizeof(glw::GLfloat), GL_MAP_READ_BIT);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error mapping buffer object's data store to client address space!");

	for (glw::GLuint i = 0; i < m_texture_buffer_size; ++i)
	{
		/* Log error if expected data and result data are not equal with epsilon tolerance */

		/* Note: texture format is R8, so i value may wrap - hence the GLubyte cast */
		if (de::abs(result[i] - (static_cast<glw::GLubyte>(i) / 255.0f)) > m_epsilon)
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "Result is different than expected \n"
							   << "Expected value: " << static_cast<glw::GLfloat>(i) / 255.0f << "\n"
							   << "Result   value: " << result[i] << "\n"
							   << tcu::TestLog::EndMessage;

			testResult = false;
			break;
		}
	}

	gl.unmapBuffer(GL_SHADER_STORAGE_BUFFER);

	if (testResult)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
	}

	return STOP;
}

/** Deinitializes GLES objects created during the test.
 *
 */
void TextureBufferParamValueIntToFloatConversion::deinit(void)
{
	/* Get Gl entry points */
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
