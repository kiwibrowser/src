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
 * \file esextcGPUShader5ImagesArrayIndexing.cpp
 * \brief GPUShader5 Images Array Indexing (Test 2)
 */ /*-------------------------------------------------------------------*/

#include "esextcGPUShader5ImagesArrayIndexing.hpp"
#include "gluContextInfo.hpp"
#include "gluDefs.hpp"
#include "glwEnums.hpp"
#include "glwFunctions.hpp"
#include "tcuTestLog.hpp"
#include <string.h>

namespace glcts
{

const glw::GLuint GPUShader5ImagesArrayIndexing::m_array_size			= 4;
const glw::GLint  GPUShader5ImagesArrayIndexing::m_texture_n_components = 1;

/** Constructor
 *
 *  @param context     Test context
 *  @param name        Test case's name
 *  @param description Test case's description
 **/
GPUShader5ImagesArrayIndexing::GPUShader5ImagesArrayIndexing(Context& context, const ExtParameters& extParams,
															 const char* name, const char* description)
	: TestCaseBase(context, extParams, name, description)
	, m_compute_shader_id(0)
	, m_data_buffer(DE_NULL)
	, m_program_id(0)
	, m_texture_height(0)
	, m_texture_width(0)
	, m_to_ids(DE_NULL)
	, m_fbo_id(0)
{
	/* Nothing to be done here */
}

/** Initializes GLES objects used during the test.
 *
 */
void GPUShader5ImagesArrayIndexing::initTest(void)
{
	/* Check if gpu_shader5 extension is supported */
	if (!m_is_gpu_shader5_supported)
	{
		throw tcu::NotSupportedError(GPU_SHADER5_EXTENSION_NOT_SUPPORTED, "", __FILE__, __LINE__);
	}

	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Calculate platform-specific value that should be used for local_size_x, local_size_y in compute shader */
	glw::GLint max_compute_work_group_invocations_value = 0;

	gl.getIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, &m_texture_width);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not query GL_MAX_COMPUTE_WORK_GROUP_SIZE!");

	gl.getIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 1, &m_texture_height);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not query GL_MAX_COMPUTE_WORK_GROUP_SIZE!");

	gl.getIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &max_compute_work_group_invocations_value);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not query GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS!");

	if (m_texture_width * m_texture_height > max_compute_work_group_invocations_value)
	{
		m_texture_width = (max_compute_work_group_invocations_value / m_texture_height);
	}

	/* Construct compute shader code */
	std::string		  compute_shader_code;
	const char*		  compute_shader_code_ptr = DE_NULL;
	std::stringstream local_size_x_stringstream;
	std::stringstream local_size_y_stringstream;

	local_size_x_stringstream << m_texture_width;
	local_size_y_stringstream << m_texture_height;

	compute_shader_code		= getComputeShaderCode(local_size_x_stringstream.str(), local_size_y_stringstream.str());
	compute_shader_code_ptr = (const char*)compute_shader_code.c_str();

	/* Create a program object */
	m_program_id = gl.createProgram();
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateProgram() call failed");

	/* Create a compute shader object */
	m_compute_shader_id = gl.createShader(GL_COMPUTE_SHADER);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateShader(GL_COMPUTE_SHADER) failed");

	/* Build a program object that consists only of the compute shader */
	if (!buildProgram(m_program_id, m_compute_shader_id, 1, &compute_shader_code_ptr))
	{
		TCU_FAIL("Could not create program object!");
	}

	/* Generate texture objects */
	m_to_ids = new glw::GLuint[m_array_size];
	memset(m_to_ids, 0, m_array_size * sizeof(glw::GLuint));
	gl.genTextures(m_array_size, m_to_ids);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not generate texture objects!");

	/* Allocate a buffer we will later fill with data and use as a data source for a texture object */
	glw::GLuint dataSize = m_texture_width * m_texture_height * m_texture_n_components;
	m_data_buffer		 = new glw::GLuint[dataSize * m_array_size];

	for (glw::GLuint array_index = 0; array_index < m_array_size; ++array_index)
	{
		for (glw::GLuint index = 0; index < dataSize; ++index)
		{
			m_data_buffer[index + array_index * dataSize] = 1 + array_index;
		}
	}

	/* Initialize storage for the texture objects */
	for (glw::GLuint index = 0; index < m_array_size; index++)
	{
		gl.activeTexture(GL_TEXTURE0 + index);
		gl.bindTexture(GL_TEXTURE_2D, m_to_ids[index]);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Could not bind a texture object to texture unit!");

		gl.texStorage2D(GL_TEXTURE_2D, 1 /* levels */, GL_R32UI, m_texture_width, m_texture_height);

		gl.texSubImage2D(GL_TEXTURE_2D, 0 /* level */, 0 /* x offset */, 0 /* y offset */, m_texture_width,
						 m_texture_height, GL_RED_INTEGER, GL_UNSIGNED_INT, &m_data_buffer[index * dataSize]);

		gl.texParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		gl.texParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		GLU_EXPECT_NO_ERROR(gl.getError(), "Could not allocate storage for a texture object!");
	}

	delete[] m_data_buffer;
	m_data_buffer = DE_NULL;
}

/** Executes the test.
 *
 *  Sets the test result to QP_TEST_RESULT_FAIL if the test failed, QP_TEST_RESULT_PASS otherwise.
 *
 *  Note the function throws exception should an error occur!
 *
 *  @return STOP if the test has finished, CONTINUE to indicate iterate should be called once again.
 **/
tcu::TestNode::IterateResult GPUShader5ImagesArrayIndexing::iterate(void)
{
	initTest();

	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.useProgram(m_program_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram() call failed");

	for (glw::GLuint index = 0; index < m_array_size; index++)
	{
		gl.bindImageTexture(index /* unit */, m_to_ids[index], 0 /* level */, GL_FALSE, /* layered */
							0,															/* layer */
							GL_READ_WRITE, GL_R32UI);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Could not bind a texture object to image unit!");
	}

	gl.dispatchCompute(1,  /* num_groups_x */
					   1,  /* num_groups_y */
					   1); /* num_groups_z */
	GLU_EXPECT_NO_ERROR(gl.getError(), "Failed to dispatch compute operation");

	/* Create and configure a framebuffer object */
	gl.genFramebuffers(1, &m_fbo_id);
	gl.bindFramebuffer(GL_READ_FRAMEBUFFER, m_fbo_id);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not create framebuffer object");

	/* Set viewport */
	gl.viewport(0, 0, m_texture_width, m_texture_height);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glViewport() call failed");

	/* Allocate space for result data */
	const glw::GLuint dataSize = m_texture_width * m_texture_height * m_texture_n_components * 4;
	m_data_buffer			   = new glw::GLuint[dataSize];

	gl.memoryBarrier(GL_PIXEL_BUFFER_BARRIER_BIT);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not set memory barrier!");

	for (unsigned int i = 0; i < m_array_size; ++i)
	{
		/* Attach texture to framebuffer's color attachment 0 */
		gl.framebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_to_ids[i], 0 /* level */);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Error configuring color attachment for framebuffer object!");

		/* Read the rendered data */
		gl.readPixels(0 /* x */, 0 /* y */, m_texture_width, m_texture_height, GL_RGBA_INTEGER, GL_UNSIGNED_INT,
					  m_data_buffer);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glReadPixels() call failed!");

		glw::GLuint resultExpected[m_texture_n_components];

		/* Loop over all pixels and compare the rendered data with reference value */
		for (glw::GLint y = 0; y < m_texture_height; ++y)
		{
			glw::GLuint* data_row = m_data_buffer + y * m_texture_width * m_texture_n_components * 4;

			for (glw::GLint x = 0; x < m_texture_width; ++x)
			{
				glw::GLuint* data = data_row + x * m_texture_n_components * 4;

				resultExpected[0] = x + y + 1 + i;

				if (resultExpected[0] != data[0])
				{
					m_testCtx.getLog() << tcu::TestLog::Message << "Invalid image data acquired for image at index "
									   << i << ", position: (" << x << "," << y << ")"
									   << ". Rendered data [" << data[0] << "]"
									   << " Expected data [" << resultExpected[0] << "]" << tcu::TestLog::EndMessage;

					delete[] m_data_buffer;
					m_data_buffer = DE_NULL;

					m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
					return STOP;
				} /* if (data mismatch) */

			} /* for (all columns) */
		}	 /* for (all rows) */
	}		  /*for (m_sizeOfArray)*/

	delete[] m_data_buffer;
	m_data_buffer = DE_NULL;

	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	return STOP;
}

/** Deinitializes GLES objects created during the test.
 *
 */
void GPUShader5ImagesArrayIndexing::deinit(void)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Reset OpenGL ES state */
	gl.useProgram(0);
	gl.bindBuffer(GL_ARRAY_BUFFER, 0);
	gl.activeTexture(GL_TEXTURE0);
	gl.bindTexture(GL_TEXTURE_2D, 0);
	gl.bindVertexArray(0);

	/* Delete program object and shaders */
	if (m_program_id != 0)
	{
		gl.deleteProgram(m_program_id);

		m_program_id = 0;
	}

	if (m_compute_shader_id != 0)
	{
		gl.deleteShader(m_compute_shader_id);

		m_compute_shader_id = 0;
	}

	if (m_data_buffer != DE_NULL)
	{
		delete[] m_data_buffer;
		m_data_buffer = DE_NULL;
	}

	if (m_to_ids != DE_NULL)
	{
		gl.deleteTextures(m_array_size, m_to_ids);
		delete[] m_to_ids;
		m_to_ids = DE_NULL;
	}

	if (m_fbo_id != 0)
	{
		gl.deleteFramebuffers(1, &m_fbo_id);
		m_fbo_id = 0;
	}

	/* Call base class' deinit() */
	TestCaseBase::deinit();
}

/** Fill compute shader template
 *
 *  @param _local_size_x    String storing a "local_size_x" layout qualifier definition;
 *  @param _local_size_y    String storing a "local_size_y" layout qualifier definition;
 *
 *  @return string containing compute shader code
 */
std::string GPUShader5ImagesArrayIndexing::getComputeShaderCode(const std::string& local_size_x,
																const std::string& local_size_y)
{
	/* Compute shader template code */
	std::string m_compute_shader_template =
		"${VERSION}\n"
		"\n"
		"${GPU_SHADER5_REQUIRE}\n"
		"\n"
		"layout (local_size_x = <-MAX-LOCAL-SIZE-X->,\n"
		"        local_size_y = <-MAX-LOCAL-SIZE-Y->,\n"
		"        local_size_z = 1) in;\n"
		"\n"
		"layout (r32ui, binding = 0) uniform highp uimage2D image[4];\n"
		"\n"
		"void main(void)\n"
		"{\n"
		"   uvec4 texel0 = imageLoad(image[0], ivec2(gl_LocalInvocationID.x, gl_LocalInvocationID.y) );\n"
		"   uvec4 texel1 = imageLoad(image[1], ivec2(gl_LocalInvocationID.x, gl_LocalInvocationID.y) );\n"
		"   uvec4 texel2 = imageLoad(image[2], ivec2(gl_LocalInvocationID.x, gl_LocalInvocationID.y) );\n"
		"   uvec4 texel3 = imageLoad(image[3], ivec2(gl_LocalInvocationID.x, gl_LocalInvocationID.y) );\n"
		"   uvec4 addon  = uvec4(gl_LocalInvocationID.x+gl_LocalInvocationID.y);\n"
		"\n"
		"   imageStore(image[0], ivec2( gl_LocalInvocationID.x, gl_LocalInvocationID.y), texel0  + addon);\n"
		"   imageStore(image[1], ivec2( gl_LocalInvocationID.x, gl_LocalInvocationID.y), texel1  + addon);\n"
		"   imageStore(image[2], ivec2( gl_LocalInvocationID.x, gl_LocalInvocationID.y), texel2  + addon);\n"
		"   imageStore(image[3], ivec2( gl_LocalInvocationID.x, gl_LocalInvocationID.y), texel3  + addon);\n"
		"}\n";

	/* Insert information on local size in X direction */
	std::string template_name	 = "<-MAX-LOCAL-SIZE-X->";
	std::size_t template_position = m_compute_shader_template.find(template_name);

	while (template_position != std::string::npos)
	{
		m_compute_shader_template =
			m_compute_shader_template.replace(template_position, template_name.length(), local_size_x);

		template_position = m_compute_shader_template.find(template_name);
	}

	/* Insert information on local size in Y direction */
	template_name	 = "<-MAX-LOCAL-SIZE-Y->";
	template_position = m_compute_shader_template.find(template_name);

	while (template_position != std::string::npos)
	{
		m_compute_shader_template =
			m_compute_shader_template.replace(template_position, template_name.length(), local_size_y);

		template_position = m_compute_shader_template.find(template_name);
	}

	return m_compute_shader_template;
}

} // namespace glcts
