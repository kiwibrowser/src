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

#include "esextcGPUShader5FmaAccuracy.hpp"

#include "gluContextInfo.hpp"
#include "glwEnums.hpp"
#include "glwFunctions.hpp"
#include "tcuTestLog.hpp"

#include <vector>

namespace glcts
{
/* Fragment Shader */
const glw::GLchar* const GPUShader5FmaAccuracyTest::m_fragment_shader_code =
	"${VERSION}\n"
	"\n"
	"${GPU_SHADER5_REQUIRE}\n"
	"\n"
	"precision highp float;\n"
	"\n"
	"layout(location = 0) out vec4 fs_out_color;\n"
	"\n"
	"void main()\n"
	"{\n"
	"    fs_out_color = vec4(1, 1, 1, 1);\n"
	"}\n";

/* Vertex Shader for fma pass */
const glw::GLchar* const GPUShader5FmaAccuracyTest::m_vertex_shader_code_for_fma_pass =
	"${VERSION}\n"
	"\n"
	"${GPU_SHADER5_REQUIRE}\n"
	"\n"
	"precision highp float;\n"
	"\n"
	"uniform uint  uni_number_of_steps;\n"
	"out     float vs_fs_result;\n"
	"\n"
	"void main()\n"
	"{\n"
	"    float border_case   = 1.0;\n"
	"    float h             = 1.0 / float(uni_number_of_steps);\n"
	"    float current_value = border_case;\n"
	"\n"
	"    for (uint step = 0u; step < uni_number_of_steps; ++step)\n"
	"    {\n"
	"        float next_value = fma(h, current_value, current_value);\n"
	"\n"
	"        current_value = next_value;\n"
	"    }\n"
	"\n"
	"    vs_fs_result = current_value;\n"
	"}\n";

/* Vertex Shader for float pass */
const glw::GLchar* const GPUShader5FmaAccuracyTest::m_vertex_shader_code_for_float_pass =
	"${VERSION}\n"
	"\n"
	"${GPU_SHADER5_REQUIRE}\n"
	"\n"
	"precision highp float;\n"
	"\n"
	"uniform uint  uni_number_of_steps;\n"
	"out     float vs_fs_result;\n"
	"\n"
	"void main()\n"
	"{\n"
	"    float border_case   = 1.0;\n"
	"    float h             = 1.0 / float(uni_number_of_steps);\n"
	"    float current_value = border_case;\n"
	"\n"
	"    for (uint step = 0u; step < uni_number_of_steps; ++step)\n"
	"    {\n"
	"        float next_value = h * current_value + current_value;\n"
	"\n"
	"        current_value = next_value;\n"
	"    }\n"
	"\n"
	"    vs_fs_result = current_value;\n"
	"}\n";

/* Constants */
const glw::GLuint  GPUShader5FmaAccuracyTest::m_buffer_size			   = sizeof(glw::GLfloat);
const unsigned int GPUShader5FmaAccuracyTest::m_n_draw_call_executions = 10;
const float		   GPUShader5FmaAccuracyTest::m_expected_result		   = 2.71828f;

/** Constructor
 *
 *  @param context       Test context
 *  @param name          Test case's name
 *  @param description   Test case's description
 **/
GPUShader5FmaAccuracyTest::GPUShader5FmaAccuracyTest(Context& context, const ExtParameters& extParams, const char* name,
													 const char* description)
	: TestCaseBase(context, extParams, name, description)
	, m_fragment_shader_id(0)
	, m_program_object_id_for_float_pass(0)
	, m_program_object_id_for_fma_pass(0)
	, m_vertex_shader_id_for_float_pass(0)
	, m_vertex_shader_id_for_fma_pass(0)
	, m_buffer_object_id(0)
	, m_vertex_array_object_id(0)
{
	/* Nothing to be done here */
}

/** Initializes GLES objects used during the test.
 *
 **/
void GPUShader5FmaAccuracyTest::initTest()
{
	/* This test should only run if EXT_gpu_shader5 is supported */
	if (!m_is_gpu_shader5_supported)
	{
		throw tcu::NotSupportedError(GPU_SHADER5_EXTENSION_NOT_SUPPORTED, "", __FILE__, __LINE__);
	}

	/* Retrieve ES entry points. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Create programs and shaders */
	m_program_object_id_for_fma_pass   = gl.createProgram();
	m_program_object_id_for_float_pass = gl.createProgram();

	m_fragment_shader_id			  = gl.createShader(GL_FRAGMENT_SHADER);
	m_vertex_shader_id_for_fma_pass   = gl.createShader(GL_VERTEX_SHADER);
	m_vertex_shader_id_for_float_pass = gl.createShader(GL_VERTEX_SHADER);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Failed to create program or shader object(s)");

	/* Declare a name of a varying we will be capturing via TF */
	const glw::GLchar* const captured_varying_name = "vs_fs_result";

	/* Set up transform feedback for fma pass*/
	gl.transformFeedbackVaryings(m_program_object_id_for_fma_pass, 1 /* count */, &captured_varying_name,
								 GL_INTERLEAVED_ATTRIBS);

	/* Build program for fma pass */
	if (false == buildProgram(m_program_object_id_for_fma_pass, m_fragment_shader_id, 1 /* number of FS parts */,
							  &m_fragment_shader_code, m_vertex_shader_id_for_fma_pass, 1 /* number of VS parts */,
							  &m_vertex_shader_code_for_fma_pass))
	{
		TCU_FAIL("Could not create program from a valid vertex/fragment shader");
	}

	/* Set up transform feedback for float pass*/
	gl.transformFeedbackVaryings(m_program_object_id_for_float_pass, 1 /* count */, &captured_varying_name,
								 GL_INTERLEAVED_ATTRIBS);

	/* Build program for float pass */
	if (false == buildProgram(m_program_object_id_for_float_pass, m_fragment_shader_id, 1 /* number of FS parts */,
							  &m_fragment_shader_code, m_vertex_shader_id_for_float_pass, 1 /* number of VS parts */,
							  &m_vertex_shader_code_for_float_pass))
	{
		TCU_FAIL("Could not create program from valid vertex/fragment shader");
	}

	/* Generate and bind VAO */
	gl.genVertexArrays(1, &m_vertex_array_object_id);
	gl.bindVertexArray(m_vertex_array_object_id);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not create and bind vertex array object");

	/* Generate, bind and allocate buffer */
	gl.genBuffers(1, &m_buffer_object_id);
	gl.bindBuffer(GL_ARRAY_BUFFER, m_buffer_object_id);
	gl.bufferData(GL_ARRAY_BUFFER, m_buffer_size, DE_NULL /* undefined start data */, GL_STATIC_DRAW);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not create vertex array object");
}

/** Executes the test.
 *  Sets the test result to QP_TEST_RESULT_FAIL if the test failed, QP_TEST_RESULT_PASS otherwise.
 *
 *  Note the function throws exception should an error occur!
 *
 *  @return STOP if the test has finished, CONTINUE to indicate iterate should be called once again.
 *
 **/
tcu::TestCase::IterateResult GPUShader5FmaAccuracyTest::iterate()
{
	initTest();

	/* Storage space for result values */
	std::vector<glw::GLfloat> results_of_float_pass;
	std::vector<glw::GLfloat> results_of_fma_pass;

	results_of_fma_pass.resize(m_n_draw_call_executions);
	results_of_float_pass.resize(m_n_draw_call_executions);

	/* Execute fma pass */
	executePass(m_program_object_id_for_fma_pass, &results_of_fma_pass[0]);

	/* Execute float pass */
	executePass(m_program_object_id_for_float_pass, &results_of_float_pass[0]);

	/* Storage space for relative errors */
	std::vector<glw::GLfloat> relative_errors_for_float_pass;
	std::vector<glw::GLfloat> relative_errors_for_fma_pass;

	relative_errors_for_fma_pass.resize(m_n_draw_call_executions);
	relative_errors_for_float_pass.resize(m_n_draw_call_executions);

	/* Calculate relative errors */
	for (glw::GLuint i = 0; i < m_n_draw_call_executions; ++i)
	{
		calculateRelativeError(results_of_fma_pass[i], m_expected_result, relative_errors_for_fma_pass[i]);
		calculateRelativeError(results_of_float_pass[i], m_expected_result, relative_errors_for_float_pass[i]);
	}

	/* Sum relative errors */
	glw::GLfloat relative_error_sum_for_fma_pass   = 0.0f;
	glw::GLfloat relative_error_sum_for_float_pass = 0.0f;

	for (glw::GLuint i = 0; i < m_n_draw_call_executions; ++i)
	{
		relative_error_sum_for_fma_pass += relative_errors_for_fma_pass[i];
		relative_error_sum_for_float_pass += relative_errors_for_float_pass[i];
	}

	if (relative_error_sum_for_fma_pass <= relative_error_sum_for_float_pass)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Routine fma has lower accuracy than a * b + c !"
						   << tcu::TestLog::EndMessage;

		/* Log sum of relative errors */
		m_testCtx.getLog() << tcu::TestLog::Section("Sum of relative error", "");
		m_testCtx.getLog() << tcu::TestLog::Message << "fma       " << relative_error_sum_for_fma_pass
						   << tcu::TestLog::EndMessage;
		m_testCtx.getLog() << tcu::TestLog::Message << "a * b + c " << relative_error_sum_for_float_pass
						   << tcu::TestLog::EndMessage;
		m_testCtx.getLog() << tcu::TestLog::EndSection;

		/* Log relative errors */
		m_testCtx.getLog() << tcu::TestLog::Section("Relative errors", "");
		logArray("fma", &relative_errors_for_fma_pass[0], m_n_draw_call_executions);
		logArray("a * b + c", &relative_errors_for_float_pass[0], m_n_draw_call_executions);
		m_testCtx.getLog() << tcu::TestLog::EndSection;

		/* Log results */
		m_testCtx.getLog() << tcu::TestLog::Section("Results", "");
		logArray("fma", &results_of_fma_pass[0], m_n_draw_call_executions);
		logArray("a * b + c", &results_of_float_pass[0], m_n_draw_call_executions);
		m_testCtx.getLog() << tcu::TestLog::EndSection;

		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
	}

	return STOP;
}

/** Deinitializes GLES objects created during the test.
 *
 */
void GPUShader5FmaAccuracyTest::deinit()
{
	/* GL */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Bind default values */
	gl.useProgram(0);
	gl.bindVertexArray(0);
	gl.bindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0 /* offset */, 0 /* id */);
	gl.bindBuffer(GL_ARRAY_BUFFER, 0);

	/* Delete everything */
	if (0 != m_vertex_array_object_id)
	{
		gl.deleteVertexArrays(1, &m_vertex_array_object_id);

		m_vertex_array_object_id = 0;
	}

	if (0 != m_buffer_object_id)
	{
		gl.deleteBuffers(1, &m_buffer_object_id);

		m_buffer_object_id = 0;
	}

	if (0 != m_program_object_id_for_fma_pass)
	{
		gl.deleteProgram(m_program_object_id_for_fma_pass);

		m_program_object_id_for_fma_pass = 0;
	}

	if (0 != m_program_object_id_for_float_pass)
	{
		gl.deleteProgram(m_program_object_id_for_float_pass);

		m_program_object_id_for_float_pass = 0;
	}

	if (0 != m_fragment_shader_id)
	{
		gl.deleteShader(m_fragment_shader_id);

		m_fragment_shader_id = 0;
	}

	if (0 != m_vertex_shader_id_for_fma_pass)
	{
		gl.deleteShader(m_vertex_shader_id_for_fma_pass);

		m_vertex_shader_id_for_fma_pass = 0;
	}

	if (0 != m_vertex_shader_id_for_float_pass)
	{
		gl.deleteShader(m_vertex_shader_id_for_float_pass);

		m_vertex_shader_id_for_float_pass = 0;
	}

	/* Call base class' deinit() */
	TestCaseBase::deinit();
}

/** Calculate relative error for given result and expected value.
 *
 *  rel_err = delta / expected_result
 *  delta   = | expected_result - result |
 *
 *  @param result          Result value
 *  @param expected_result Expected value
 *  @param relative_error  Set to value of relative error
 **/
void GPUShader5FmaAccuracyTest::calculateRelativeError(glw::GLfloat result, glw::GLfloat expected_result,
													   glw::GLfloat& relative_error)
{
	const glw::GLfloat delta = de::abs(expected_result - result);

	relative_error = delta / expected_result;
}

/** Executes single pass of test.
 *
 *  @param program_object_id Gpu program that will be used during draw calls
 *  @param results           Storage used for result values. It is expected to have capacity for at least m_n_draw_call_executions elements
 **/
void GPUShader5FmaAccuracyTest::executePass(glw::GLuint program_object_id, glw::GLfloat* results)
{
	/* GL */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Uniform location */
	glw::GLint uniform_location = gl.getUniformLocation(program_object_id, "uni_number_of_steps");

	GLU_EXPECT_NO_ERROR(gl.getError(), "Failed to get uniform location");

	if (-1 == uniform_location)
	{
		TCU_FAIL("Unexpected inactive uniform location");
	}

	/* Setup transform feedback */
	gl.enable(GL_RASTERIZER_DISCARD);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glEnable(GL_RASTERIZER_DISCARD) call failed");

	gl.bindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, /* index */
					  m_buffer_object_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Failed to bind buffer object id to transform feedback binding point");

	/* Setup draw call */
	gl.useProgram(program_object_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram() failed");

	for (unsigned int i = 0; i < m_n_draw_call_executions; ++i)
	{
		/* Get number of steps for the Euler iterative algorithm */
		glw::GLuint number_of_steps = getNumberOfStepsForIndex(i);

		/* Set uniform data */
		gl.uniform1ui(uniform_location, number_of_steps);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Failed to set uniform data");

		/* Start transform feedback */
		gl.beginTransformFeedback(GL_POINTS);
		{
			/* Draw */
			gl.drawArrays(GL_POINTS, 0 /* first */, 1 /* one point */);
		}
		/* Stop transform feedback */
		gl.endTransformFeedback();

		GLU_EXPECT_NO_ERROR(gl.getError(), "Error doing a draw call");

		/* Map transfrom feedback results */
		const glw::GLfloat* transform_feedback_data = (const glw::GLfloat*)gl.mapBufferRange(
			GL_TRANSFORM_FEEDBACK_BUFFER, 0 /* offset */, m_buffer_size, GL_MAP_READ_BIT);

		GLU_EXPECT_NO_ERROR(gl.getError(), "Could not map the buffer object into process space");

		/* Copy data to results array */
		results[i] = *transform_feedback_data;

		/* Unmap transform feedback buffer */
		gl.unmapBuffer(GL_TRANSFORM_FEEDBACK_BUFFER);

		GLU_EXPECT_NO_ERROR(gl.getError(), "Error unmapping the buffer object");
	}
}

/** Get number of steps that will be used by vertex shader to compute its result for given index of iteration
 *
 *  @param index Index of iteration
 *
 *  @return Number of steps
 **/
glw::GLuint GPUShader5FmaAccuracyTest::getNumberOfStepsForIndex(glw::GLuint index)
{
	glw::GLuint number_of_steps = 10;

	for (glw::GLuint i = 0; i < index; ++i)
	{
		number_of_steps *= 2;
	}

	return number_of_steps;
}

/** Logs contents of array
 *
 *  @param description Description of data
 *  @param data        Data to log
 *  @param length      Number of values to log
 **/
void GPUShader5FmaAccuracyTest::logArray(const char* description, glw::GLfloat* data, glw::GLuint length)
{
	m_testCtx.getLog() << tcu::TestLog::Message << description << tcu::TestLog::EndMessage;

	tcu::MessageBuilder message = m_testCtx.getLog() << tcu::TestLog::Message;

	message << "| ";

	for (glw::GLuint i = 0; i < length; ++i)
	{
		message << data[i] << " | ";
	}

	message << tcu::TestLog::EndMessage;
}
} /* glcts */
