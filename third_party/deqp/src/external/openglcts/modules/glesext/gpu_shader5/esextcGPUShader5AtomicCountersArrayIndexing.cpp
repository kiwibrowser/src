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
 * \file esextcGPUShader5AtomicCountersArrayIndexing.cpp
 * \brief GPUShader5 Atomic Counters Array Indexing (Test 3)
 */ /*-------------------------------------------------------------------*/

#include "esextcGPUShader5AtomicCountersArrayIndexing.hpp"
#include "gluContextInfo.hpp"
#include "gluDefs.hpp"
#include "glwEnums.hpp"
#include "glwFunctions.hpp"
#include "tcuTestLog.hpp"
#include <cstring>

namespace glcts
{
const glw::GLuint GPUShader5AtomicCountersArrayIndexing::m_atomic_counters_array_size = 4;

/* Compute Shader code */
const char* GPUShader5AtomicCountersArrayIndexing::m_compute_shader_code =
	"${VERSION}\n"
	"\n"
	"${GPU_SHADER5_REQUIRE}\n"
	"\n"
	"layout (local_size_x = 2, local_size_y = 2) in;\n"
	"layout (binding      = 0, offset       = 0) uniform atomic_uint counters[4];\n"
	"\n"
	"void main(void)\n"
	"{\n"
	"    for (uint index = 0u; index <= 3u; ++index)\n"
	"    {\n"
	"        for (uint iteration = 0u; iteration <= gl_LocalInvocationID.x * 2u + gl_LocalInvocationID.y; "
	"++iteration)\n"
	"        {\n"
	"            atomicCounterIncrement( counters[index] );\n"
	"        }\n"
	"    }\n"
	"}\n";

/** Constructor
 *
 *  @param context     Test context
 *  @param name        Test case's name
 *  @param description Test case's description
 **/
GPUShader5AtomicCountersArrayIndexing::GPUShader5AtomicCountersArrayIndexing(Context&			  context,
																			 const ExtParameters& extParams,
																			 const char* name, const char* description)
	: TestCaseBase(context, extParams, name, description), m_buffer_id(0), m_compute_shader_id(0), m_program_id(0)
{
	/* Nothing to be done here */
}

/** Initializes GLES objects used during the test.
 *
 */
void GPUShader5AtomicCountersArrayIndexing::initTest(void)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Check if gpu_shader5 extension is supported */
	if (!m_is_gpu_shader5_supported)
	{
		throw tcu::NotSupportedError(GPU_SHADER5_EXTENSION_NOT_SUPPORTED, "", __FILE__, __LINE__);
	}

	/* Create program object from the supplied compute shader code */
	m_program_id = gl.createProgram();
	GLU_EXPECT_NO_ERROR(gl.getError(), "Creating program object failed");

	m_compute_shader_id = gl.createShader(GL_COMPUTE_SHADER);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Creating compute shader object failed");

	if (!buildProgram(m_program_id, m_compute_shader_id, 1, &m_compute_shader_code))
	{
		TCU_FAIL("Could not create program object!");
	}

	glw::GLuint bufferData[m_atomic_counters_array_size];
	for (glw::GLuint index = 0; index < m_atomic_counters_array_size; ++index)
	{
		bufferData[index] = index;
	}

	gl.genBuffers(1, &m_buffer_id);
	gl.bindBuffer(GL_ATOMIC_COUNTER_BUFFER, m_buffer_id);
	gl.bufferData(GL_ATOMIC_COUNTER_BUFFER, m_atomic_counters_array_size * sizeof(glw::GLuint), bufferData,
				  GL_DYNAMIC_COPY);
	gl.bindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 0, m_buffer_id);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not setup buffer object!");
}

/** Executes the test.
 *
 *  Sets the test result to QP_TEST_RESULT_FAIL if the test failed, QP_TEST_RESULT_PASS otherwise.
 *
 *  Note the function throws exception should an error occur!
 *
 *  @return STOP if the test has finished, CONTINUE to indicate iterate should be called once again.
 **/
tcu::TestNode::IterateResult GPUShader5AtomicCountersArrayIndexing::iterate(void)
{
	initTest();

	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Do the computations */
	gl.useProgram(m_program_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not use program object!");

	gl.dispatchCompute(10, /* num_groups_x */
					   10, /* num_groups_y */
					   1); /* num_groups_z */
	GLU_EXPECT_NO_ERROR(gl.getError(), "Failed to dispatch compute operation!");

	/* Map the buffer object storage into user space */
	glw::GLuint* bufferData =
		(glw::GLuint*)gl.mapBufferRange(GL_ATOMIC_COUNTER_BUFFER, 0, /* offset */
										m_atomic_counters_array_size * sizeof(glw::GLuint), GL_MAP_READ_BIT);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Failed to map buffer range to client space!");

	/* Compare the result values with reference value */
	const glw::GLuint expectedResult = 1000;

	if (expectedResult != bufferData[0] || expectedResult + 1 != bufferData[1] || expectedResult + 2 != bufferData[2] ||
		expectedResult + 3 != bufferData[3])
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Invalid data rendered. Expected Data"
						   << " (" << expectedResult << ", " << expectedResult << ", " << expectedResult << ", "
						   << expectedResult << ") Result Data"
						   << " (" << bufferData[0] << " ," << bufferData[1] << " ," << bufferData[2] << " ,"
						   << bufferData[3] << ")" << tcu::TestLog::EndMessage;

		gl.unmapBuffer(GL_ATOMIC_COUNTER_BUFFER);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Failed to unmap buffer!");

		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
		return STOP;
	}

	gl.unmapBuffer(GL_ATOMIC_COUNTER_BUFFER);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Failed to unmap buffer!");

	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	return STOP;
}

/** Deinitializes GLES objects created during the test.
 *
 */
void GPUShader5AtomicCountersArrayIndexing::deinit(void)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Reset OpenGL ES state */
	gl.useProgram(0);
	gl.bindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);

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

	if (m_buffer_id != 0)
	{
		gl.deleteBuffers(1, &m_buffer_id);
		m_buffer_id = 0;
	}

	/* Release base class */
	TestCaseBase::deinit();
}

} // namespace glcts
