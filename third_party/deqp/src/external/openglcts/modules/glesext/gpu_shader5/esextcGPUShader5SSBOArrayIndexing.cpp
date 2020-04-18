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
 * \file esextcGPUShader5SSBOArrayIndexing.cpp
 * \brief GPUShader5 SSBO Array Indexing (Test 5)
 */ /*-------------------------------------------------------------------*/

#include "esextcGPUShader5SSBOArrayIndexing.hpp"
#include "gluContextInfo.hpp"
#include "gluDefs.hpp"
#include "glwEnums.hpp"
#include "glwFunctions.hpp"
#include "tcuTestLog.hpp"
#include <cstring>

namespace glcts
{

/* Compute shader code */
const char* GPUShader5SSBOArrayIndexing::m_compute_shader_code =
	"${VERSION}\n"
	"\n"
	"${GPU_SHADER5_REQUIRE}\n"
	"\n"
	"#define LOCAL_SIZE_X 3u\n"
	"#define LOCAL_SIZE_Y 3u\n"
	"\n"
	"layout (local_size_x = LOCAL_SIZE_X, local_size_y = LOCAL_SIZE_Y) in;\n"
	"\n"
	"layout(binding = 0) buffer ComputeSSBO\n"
	"{\n"
	"    uint value;\n"
	"} computeSSBO[4];\n"
	"\n"
	"uniform uint index;\n"
	"\n"
	"void main(void)\n"
	"{\n"
	"    uint id = gl_LocalInvocationID.x * LOCAL_SIZE_Y + gl_LocalInvocationID.y;\n"
	"\n"
	"    for (uint i = 0u; i < LOCAL_SIZE_X * LOCAL_SIZE_Y; ++i)\n"
	"    {\n"
	"        if (id == i)\n"
	"        {\n"
	"            computeSSBO[index].value += id;\n"
	"        }\n"
	"\n"
	"        memoryBarrier();\n"
	"    }\n"
	"}\n";

/* Size of the ssbo array */
const glw::GLuint GPUShader5SSBOArrayIndexing::m_n_arrays = 4;

/** Constructor
 *
 *  @param context       Test context
 *  @param name          Test case's name
 *  @param description   Test case's description
 **/
GPUShader5SSBOArrayIndexing::GPUShader5SSBOArrayIndexing(Context& context, const ExtParameters& extParams,
														 const char* name, const char* description)
	: TestCaseBase(context, extParams, name, description)
	, m_compute_shader_id(0)
	, m_program_id(0)
	, m_ssbo_buffer_ids(DE_NULL)
{
	/* Nothing to be done here */
}

/** Initializes GLES objects used during the test.
 *
 */
void GPUShader5SSBOArrayIndexing::initTest(void)
{
	/* Check if gpu_shader5 extension is supported */
	if (!m_is_gpu_shader5_supported)
	{
		throw tcu::NotSupportedError(GPU_SHADER5_EXTENSION_NOT_SUPPORTED, "", __FILE__, __LINE__);
	}

	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	m_program_id = gl.createProgram();
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not create program object!");

	m_compute_shader_id = gl.createShader(GL_COMPUTE_SHADER);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not create shader object!");

	if (!buildProgram(m_program_id, m_compute_shader_id, 1, &m_compute_shader_code))
	{
		TCU_FAIL("Could not build program from valid compute shader!");
	}

	m_ssbo_buffer_ids = new glw::GLuint[m_n_arrays];
	memset(m_ssbo_buffer_ids, 0, m_n_arrays * sizeof(glw::GLuint));

	gl.genBuffers(m_n_arrays, m_ssbo_buffer_ids);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenBuffers() call failed");

	for (glw::GLuint index = 0; index < m_n_arrays; ++index)
	{
		gl.bindBuffer(GL_ARRAY_BUFFER, m_ssbo_buffer_ids[index]);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Could not bind buffer object!");

		const glw::GLuint ssbo_data = 0;

		gl.bufferData(GL_ARRAY_BUFFER, sizeof(glw::GLuint), &ssbo_data, GL_DYNAMIC_COPY);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Could not set buffer object data!");

		gl.bindBufferBase(GL_SHADER_STORAGE_BUFFER, index, m_ssbo_buffer_ids[index]);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Could not bind buffer object to shader storage binding point!");
	}
}

/** Executes the test.
 *  Sets the test result to QP_TEST_RESULT_FAIL if the test failed, QP_TEST_RESULT_PASS otherwise.
 *
 *  @return STOP if the test has finished, CONTINUE to indicate iterate should be called once again.
 *
 *  Note the function throws exception should an error occur!
 **/
tcu::TestNode::IterateResult GPUShader5SSBOArrayIndexing::iterate(void)
{
	initTest();

	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Activate the program object and retrieve index uniform location */
	gl.useProgram(m_program_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not use program!");

	glw::GLint index_location = gl.getUniformLocation(m_program_id, "index");
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetUniformLocation() call failed!");

	if (index_location == -1)
	{
		TCU_FAIL("Index uniform location is inactive which is invalid!");
	}

	/* Issue a few compute operations */
	gl.uniform1ui(index_location, 1);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not set value of index uniform variable!");

	gl.dispatchCompute(1,  /* num_groups_x */
					   1,  /* num_groups_y */
					   1); /* num_groups_z */
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not dispatch compute!");

	gl.memoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not set memory barrier!");

	gl.uniform1ui(index_location, 3);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not set value to index uniform variable!");

	gl.dispatchCompute(1,  /* num_groups_x */
					   1,  /* num_groups_y */
					   1); /* num_groups_z */
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not dispatch compute!");

	gl.memoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not set memory barrier!");

	/* Validate the results */
	glw::GLuint expected_result[m_n_arrays] = { 0, 36, 0, 36 };
	bool		test_failed					= false;

	for (glw::GLuint index = 0; index < m_n_arrays; ++index)
	{
		gl.bindBuffer(GL_SHADER_STORAGE_BUFFER, m_ssbo_buffer_ids[index]);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Could not bind a buffer object!");

		const glw::GLuint* compute_SSBO =
			(const glw::GLuint*)gl.mapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, sizeof(glw::GLuint), GL_MAP_READ_BIT);

		GLU_EXPECT_NO_ERROR(gl.getError(), "Could not map buffer data to client memory");

		if (compute_SSBO[0] != expected_result[index])
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "Expected Data (" << expected_result[index] << ") "
							   << "Result Data (" << compute_SSBO[0] << ")" << tcu::TestLog::EndMessage;

			test_failed = true;
		}

		gl.unmapBuffer(GL_SHADER_STORAGE_BUFFER);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Could not unmap buffer");
	}

	if (test_failed)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
	}
	else
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}

	return STOP;
}

/** Deinitializes GLES objects created during the test.
 *
 */
void GPUShader5SSBOArrayIndexing::deinit(void)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Reset OpenGL ES state */
	gl.useProgram(0);
	gl.bindBuffer(GL_ARRAY_BUFFER, 0);
	gl.bindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

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

	if (m_ssbo_buffer_ids != DE_NULL)
	{
		gl.deleteBuffers(m_n_arrays, m_ssbo_buffer_ids);

		delete[] m_ssbo_buffer_ids;
		m_ssbo_buffer_ids = DE_NULL;
	}

	/* Call base class' deinit() */
	TestCaseBase::deinit();
}

} // namespace glcts
