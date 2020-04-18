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

#include "esextcGPUShader5PreciseQualifier.hpp"
#include "gluContextInfo.hpp"
#include "gluDefs.hpp"
#include "glwEnums.hpp"
#include "glwFunctions.hpp"
#include "tcuTestLog.hpp"
#include <assert.h>
#include <cstdlib>
#include <cstring>
#include <ctime>

namespace glcts
{
/* Fragment Shader code */
const char* GPUShader5PreciseQualifier::m_fragment_shader_code = "${VERSION}\n"
																 "\n"
																 "${GPU_SHADER5_REQUIRE}\n"
																 "\n"
																 "precision highp float;\n"
																 "\n"
																 "out vec4 color;\n"
																 "\n"
																 "void main()\n"
																 "{\n"
																 "   color = vec4(1, 1, 1, 1);\n"
																 "}\n";

/* Vertex Shader code */
const char* GPUShader5PreciseQualifier::m_vertex_shader_code =
	"${VERSION}\n"
	"\n"
	"${GPU_SHADER5_REQUIRE}\n"
	"\n"
	"precision highp float;\n"
	"\n"
	"layout(location = 0) in  vec4 positions;\n"
	"layout(location = 1) in  vec4 weights;\n"
	"out vec4 weightedSum;\n"
	"\n"
	"void eval(in vec4 p, in vec4 w, precise out float result)\n"
	"{\n"
	"    result = (p.x*w.x + p.y*w.y) + (p.z*w.z + p.w*w.w);\n"
	"}\n"
	"\n"
	"float eval(in vec4 p, in vec4 w)\n"
	"{\n"
	"    precise float result = (p.x*w.x + p.y*w.y) + (p.z*w.z + p.w*w.w);\n"
	"\n"
	"    return result;\n"
	"}\n"
	"void main()\n"
	"{\n"
	"    eval(positions, weights, weightedSum.x);\n"
	"\n"
	"    weightedSum.y = eval(positions, weights);\n"
	"\n"
	"    precise float result = 0.0f;\n"
	"\n"
	"    result        = (positions.x * weights.x + positions.y * weights.y) +\n"
	"                    (positions.z * weights.z + positions.w * weights.w);\n"
	"    weightedSum.z = result;\n"
	"    weightedSum.w = (positions.x * weights.x + positions.y * weights.y) + \n"
	"                    (positions.z * weights.z + positions.w * weights.w);\n"
	"}\n";

const glw::GLuint GPUShader5PreciseQualifier::m_n_components   = 4;
const glw::GLuint GPUShader5PreciseQualifier::m_n_iterations   = 100;
const glw::GLint  GPUShader5PreciseQualifier::m_position_range = 1000;

/** Constructor
 *
 *  @param context       Test context
 *  @param name          Test case's name
 *  @param description   Test case's description
 **/
GPUShader5PreciseQualifier::GPUShader5PreciseQualifier(Context& context, const ExtParameters& extParams,
													   const char* name, const char* description)
	: TestCaseBase(context, extParams, name, description)
	, m_fragment_shader_id(0)
	, m_program_id(0)
	, m_tf_buffer_id(0)
	, m_vao_id(0)
	, m_vertex_shader_id(0)
	, m_vertex_positions_buffer_id(0)
	, m_vertex_weights_buffer_id(0)
{
	/* Nothing to be done here */
}

/** Initializes GLES objects used during the test.
 *
 */
void GPUShader5PreciseQualifier::initTest(void)
{
	/* Check if gpu_shader5 extension is supported */
	if (!m_is_gpu_shader5_supported)
	{
		throw tcu::NotSupportedError(GPU_SHADER5_EXTENSION_NOT_SUPPORTED, "", __FILE__, __LINE__);
	}

	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Create program object */
	m_program_id = gl.createProgram();

	/* Setup transform feedback varyings */
	const char* feedback_varyings[] = { "weightedSum" };

	gl.transformFeedbackVaryings(m_program_id, 1, feedback_varyings, GL_INTERLEAVED_ATTRIBS);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not set transform feedback varyings!");

	/* Create shaders */
	m_vertex_shader_id   = gl.createShader(GL_VERTEX_SHADER);
	m_fragment_shader_id = gl.createShader(GL_FRAGMENT_SHADER);

	/* Build program */
	if (!buildProgram(m_program_id, m_fragment_shader_id, 1, &m_fragment_shader_code, m_vertex_shader_id, 1,
					  &m_vertex_shader_code))
	{
		TCU_FAIL("Program could not have been created sucessfully from a valid vertex/geometry/fragment shader!");
	}

	/* Generate & bind a VAO */
	gl.genVertexArrays(1, &m_vao_id);
	gl.bindVertexArray(m_vao_id);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not create a vertex array object!");

	/* Create transform feedback buffer object */
	gl.genBuffers(1, &m_tf_buffer_id);
	gl.bindBuffer(GL_ARRAY_BUFFER, m_tf_buffer_id);
	gl.bufferData(GL_ARRAY_BUFFER, sizeof(glw::GLfloat) * m_n_components, DE_NULL, GL_STATIC_COPY);
	gl.bindBuffer(GL_ARRAY_BUFFER, 0);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not create buffer object!");

	/* Bind buffer object to transform feedback binding point */
	gl.bindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, m_tf_buffer_id);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not bind buffer object to transform feedback binding point!");

	/* Create positions buffer object */
	gl.genBuffers(1, &m_vertex_positions_buffer_id);
	gl.bindBuffer(GL_ARRAY_BUFFER, m_vertex_positions_buffer_id);
	gl.bufferData(GL_ARRAY_BUFFER, sizeof(glw::GLfloat) * m_n_components, DE_NULL, GL_STATIC_DRAW);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not create buffer object!");

	/* Create weights buffer object */
	gl.genBuffers(1, &m_vertex_weights_buffer_id);
	gl.bindBuffer(GL_ARRAY_BUFFER, m_vertex_weights_buffer_id);
	gl.bufferData(GL_ARRAY_BUFFER, sizeof(glw::GLfloat) * m_n_components, DE_NULL, GL_STATIC_DRAW);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not create buffer object!");

	gl.useProgram(m_program_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not use a program object!");

	gl.bindBuffer(GL_ARRAY_BUFFER, m_vertex_positions_buffer_id);
	gl.vertexAttribPointer(0 /* index */, 4 /* size */, GL_FLOAT, GL_FALSE, 0, 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not configure vertex attribute array for positions attribute!");

	gl.bindBuffer(GL_ARRAY_BUFFER, m_vertex_weights_buffer_id);
	gl.vertexAttribPointer(1 /* index */, 4 /* size */, GL_FLOAT, GL_FALSE, 0, 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not configure vertex attribute array for weights attribute!");

	gl.enableVertexAttribArray(0 /* index */);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not enable vertex attribute array for index 0!");
	gl.enableVertexAttribArray(1 /* index */);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not enable vertex attribute array for index 1!");
}

/** Deinitializes GLES objects created during the test.
 *
 */
void GPUShader5PreciseQualifier::deinit(void)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Reset OpenGL ES state */
	gl.disableVertexAttribArray(0);
	gl.disableVertexAttribArray(1);
	gl.useProgram(0);
	gl.bindBuffer(GL_ARRAY_BUFFER, 0);
	gl.bindVertexArray(0);

	/* Delete program object and shaders */
	if (m_program_id != 0)
	{
		gl.deleteProgram(m_program_id);

		m_program_id = 0;
	}

	if (m_vertex_shader_id != 0)
	{
		gl.deleteShader(m_vertex_shader_id);

		m_vertex_shader_id = 0;
	}

	if (m_fragment_shader_id != 0)
	{
		gl.deleteShader(m_fragment_shader_id);

		m_fragment_shader_id = 0;
	}

	/* Delete buffer objects */
	if (m_tf_buffer_id != 0)
	{
		gl.deleteBuffers(1, &m_tf_buffer_id);

		m_tf_buffer_id = 0;
	}

	if (m_vertex_positions_buffer_id != 0)
	{
		gl.deleteBuffers(1, &m_vertex_positions_buffer_id);

		m_vertex_positions_buffer_id = 0;
	}

	if (m_vertex_weights_buffer_id != 0)
	{
		gl.deleteBuffers(1, &m_vertex_weights_buffer_id);

		m_vertex_weights_buffer_id = 0;
	}

	/* Delete vertex array object */
	if (m_vao_id != 0)
	{
		gl.deleteVertexArrays(1, &m_vao_id);

		m_vao_id = 0;
	}

	/* Call base class' deinit() */
	TestCaseBase::deinit();
}

/** Executes the test.
 *  Sets the test result to QP_TEST_RESULT_FAIL if the test failed, QP_TEST_RESULT_PASS otherwise.
 *
 *  Note the function throws exception should an error occur!
 *
 *  @return STOP if the test has finished, CONTINUE to indicate iterate should be called once again.
 *
 **/
tcu::TestNode::IterateResult GPUShader5PreciseQualifier::iterate(void)
{
	initTest();

	/* Set the seed for the random generator */
	randomSeed(1);

	glw::GLboolean test_failed = false;

	for (glw::GLuint test_iter = 0; test_iter < m_n_iterations; ++test_iter)
	{
		/* Create the data for positions and weights attributes */
		glw::GLfloat vertex_data_positions[m_n_components];
		glw::GLfloat vertex_data_weights[m_n_components];
		glw::GLfloat weights_sum = 1.0f;

		for (glw::GLuint component_nr = 0; component_nr < m_n_components; ++component_nr)
		{
			vertex_data_positions[component_nr] = static_cast<glw::GLfloat>(
				static_cast<glw::GLint>(randomFormula(2 * m_position_range)) - m_position_range);

			if (component_nr != (m_n_components - 1))
			{
				glw::GLfloat random_value = static_cast<glw::GLfloat>(randomFormula(m_position_range)) /
											static_cast<glw::GLfloat>(m_position_range);

				vertex_data_weights[component_nr] = random_value;
				weights_sum -= random_value;
			}
			else
			{
				vertex_data_weights[component_nr] = weights_sum;
			}
		}

		/* Compute forward weighted sum */
		WeightedSum weighted_sum_forward[m_n_components];

		drawAndGetFeedbackResult(vertex_data_positions, vertex_data_weights, weighted_sum_forward);

		/* Reverse the data for positions and weights attributes */
		glw::GLfloat temp_value = 0.0f;

		for (glw::GLuint component_nr = 0; component_nr < (m_n_components / 2); ++component_nr)
		{
			temp_value = vertex_data_positions[component_nr];

			vertex_data_positions[component_nr] = vertex_data_positions[m_n_components - 1 - component_nr];
			vertex_data_positions[m_n_components - 1 - component_nr] = temp_value;

			temp_value = vertex_data_weights[component_nr];

			vertex_data_weights[component_nr] = vertex_data_weights[m_n_components - 1 - component_nr];
			vertex_data_weights[m_n_components - 1 - component_nr] = temp_value;
		}

		/* Compute backward weighted sum */
		WeightedSum weighted_sum_backward[m_n_components];

		drawAndGetFeedbackResult(vertex_data_positions, vertex_data_weights, weighted_sum_backward);

		/* Check if results are bitwise accurate */
		if (weighted_sum_backward[0].intv != weighted_sum_backward[1].intv ||
			weighted_sum_backward[0].intv != weighted_sum_backward[2].intv ||
			weighted_sum_backward[0].intv != weighted_sum_forward[0].intv ||
			weighted_sum_backward[0].intv != weighted_sum_forward[1].intv ||
			weighted_sum_backward[0].intv != weighted_sum_forward[2].intv)
		{
			test_failed = true;
			break;
		}
	}

	if (test_failed)
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
	else
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

	return STOP;
}

/** Draw and get feedback result
 *
 *  @param vertex_data_positions data for positions attribute
 *  @param vertex_data_weights   data for weights attribute
 *  @param feedback_result       space to store result fetched via transform feedback
 **/
void GPUShader5PreciseQualifier::drawAndGetFeedbackResult(const glw::GLfloat* vertex_data_positions,
														  const glw::GLfloat* vertex_data_weights,
														  WeightedSum*		  feedback_result)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.bindBuffer(GL_ARRAY_BUFFER, m_vertex_positions_buffer_id);
	gl.bufferData(GL_ARRAY_BUFFER, sizeof(glw::GLfloat) * m_n_components, vertex_data_positions, GL_STATIC_DRAW);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not set buffer object's data!");

	gl.bindBuffer(GL_ARRAY_BUFFER, m_vertex_weights_buffer_id);
	gl.bufferData(GL_ARRAY_BUFFER, sizeof(glw::GLfloat) * m_n_components, vertex_data_weights, GL_STATIC_DRAW);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not set buffer object's data!");

	gl.enable(GL_RASTERIZER_DISCARD);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glEnable(GL_RASTERIZER_DISCARD) failed");

	gl.beginTransformFeedback(GL_POINTS);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBeginTransformFeedback(GL_POINTS) failed");

	gl.drawArrays(GL_POINTS, 0 /* first */, 1 /* count */);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Rendering Failed!");

	gl.endTransformFeedback();
	GLU_EXPECT_NO_ERROR(gl.getError(), "glEndTransformFeedback() failed");

	/* Fetch the results via transform feedback */
	WeightedSum* result = (WeightedSum*)gl.mapBufferRange(GL_TRANSFORM_FEEDBACK_BUFFER, 0,
														  sizeof(glw::GLfloat) * m_n_components, GL_MAP_READ_BIT);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glMapBufferRange() failed");

	memcpy(feedback_result, result, sizeof(glw::GLfloat) * m_n_components);

	gl.unmapBuffer(GL_TRANSFORM_FEEDBACK_BUFFER);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUnmapBuffer() failed");

	gl.disable(GL_RASTERIZER_DISCARD);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glDisable(GL_RASTERIZER_DISCARD) failed");
}

} // namespace glcts
