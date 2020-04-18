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
 * \file esextcGPUShader5UniformBlocksArrayIndexing.cpp
 * \brief GPUShader5 Uniform Blocks Array Indexing Test (Test Group 4)
 */ /*-------------------------------------------------------------------*/

#include "esextcGPUShader5UniformBlocksArrayIndexing.hpp"
#include "gluContextInfo.hpp"
#include "gluDefs.hpp"
#include "glwEnums.hpp"
#include "glwFunctions.hpp"
#include "tcuTestLog.hpp"
#include <cstring>

namespace glcts
{

const glw::GLuint GPUShader5UniformBlocksArrayIndexing::m_n_array_size			= 4;
const glw::GLuint GPUShader5UniformBlocksArrayIndexing::m_n_position_components = 4;

/* Data to fill in the buffer object associated with positionBlocks uniform array */
const glw::GLfloat GPUShader5UniformBlocksArrayIndexing::m_position_data[] = { -1.0, -1.0, 0.0, 1.0,  -1.0, 1.0,
																			   0.0,  1.0,  1.0, -1.0, 0.0,  1.0,
																			   1.0,  1.0,  0.0, 1.0 };

/* Fragment Shader code */
const char* GPUShader5UniformBlocksArrayIndexing::m_fragment_shader_code = "${VERSION}\n"
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
const char* GPUShader5UniformBlocksArrayIndexing::m_vertex_shader_code =
	"${VERSION}\n"
	"\n"
	"${GPU_SHADER5_REQUIRE}\n"
	"\n"
	"precision highp float;\n"
	"\n"
	"uniform PositionBlock\n"
	"{\n"
	"   vec4 position;\n"
	"} positionBlocks[4];\n"
	"\n"
	"uniform uint index;\n"
	"\n"
	"void main()\n"
	"{\n"
	"   gl_Position = positionBlocks[index].position;\n"
	"}\n";

/** Constructor
 *
 * @param context     Test context
 * @param name        Test case's name
 * @param description Test case's description
 **/
GPUShader5UniformBlocksArrayIndexing::GPUShader5UniformBlocksArrayIndexing(Context&				context,
																		   const ExtParameters& extParams,
																		   const char* name, const char* description)
	: TestCaseBase(context, extParams, name, description)
	, m_fragment_shader_id(0)
	, m_program_id(0)
	, m_tf_buffer_id(0)
	, m_uniform_buffer_ids(DE_NULL)
	, m_vertex_shader_id(0)
	, m_vao_id(0)
{
	/* Nothing to be done here */
}

/** Initializes GLES objects used during the test.
 *
 **/
void GPUShader5UniformBlocksArrayIndexing::initTest(void)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Check if gpu_shader5 extension is supported */
	if (!m_is_gpu_shader5_supported)
	{
		throw tcu::NotSupportedError(GPU_SHADER5_EXTENSION_NOT_SUPPORTED, "", __FILE__, __LINE__);
	}

	/* Feedback varyings */
	const char*		   feedbackVaryings[] = { "gl_Position" };
	const unsigned int nVaryings		  = sizeof(feedbackVaryings) / sizeof(char*);

	/* Generate and bind VAO */
	gl.genVertexArrays(1, &m_vao_id);
	gl.bindVertexArray(m_vao_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not create vertex array object");

	/* Create program object */
	m_program_id = gl.createProgram();
	GLU_EXPECT_NO_ERROR(gl.getError(), "Creating program object failed!");

	gl.transformFeedbackVaryings(m_program_id, nVaryings, feedbackVaryings, GL_INTERLEAVED_ATTRIBS);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not set transform feedback varyings!");

	/* Create shader objects */
	m_vertex_shader_id   = gl.createShader(GL_VERTEX_SHADER);
	m_fragment_shader_id = gl.createShader(GL_FRAGMENT_SHADER);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Creating shader objects failed!");

	/* Build program */
	if (!buildProgram(m_program_id, m_fragment_shader_id, 1, &m_fragment_shader_code, m_vertex_shader_id, 1,
					  &m_vertex_shader_code))
	{
		TCU_FAIL("Program could not have been created sucessfully from a valid vertex/fragment shader!");
	}

	/* Create a buffer object */
	gl.genBuffers(1, &m_tf_buffer_id);
	gl.bindBuffer(GL_ARRAY_BUFFER, m_tf_buffer_id);
	gl.bufferData(GL_ARRAY_BUFFER, m_n_position_components * sizeof(glw::GLfloat) * nVaryings, DE_NULL,
				  GL_DYNAMIC_COPY);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not create and initialize a buffer object to be used for XFB!");

	/* Bind buffer object to transform feedback binding point */
	gl.bindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, /* index */
					  m_tf_buffer_id);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not bind buffer object to transform feedback binding point!");
}

/** Executes the test.
 *  Sets the test result to QP_TEST_RESULT_FAIL if the test failed, QP_TEST_RESULT_PASS otherwise.
 *
 *  Note the function throws exception should an error occur!
 *
 *  @return STOP if the test has finished, CONTINUE to indicate iterate should be called once again.
 *
 **/
tcu::TestNode::IterateResult GPUShader5UniformBlocksArrayIndexing::iterate(void)
{
	initTest();

	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Use the test program object */
	gl.useProgram(m_program_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not use program object!");

	/* Set up uniform buffer bindings */
	m_uniform_buffer_ids = new glw::GLuint[m_n_array_size];
	memset(m_uniform_buffer_ids, 0, m_n_array_size * sizeof(glw::GLuint));

	gl.genBuffers(m_n_array_size, m_uniform_buffer_ids);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenBuffers() failed");

	for (glw::GLuint index_value = 0; index_value < m_n_array_size; ++index_value)
	{
		glw::GLuint		  blockIndex = 0;
		std::stringstream positionBlock;

		positionBlock << "PositionBlock[" << index_value << "]";

		blockIndex = gl.getUniformBlockIndex(m_program_id, positionBlock.str().c_str());
		GLU_EXPECT_NO_ERROR(gl.getError(), "Could not get uniform block index");

		gl.uniformBlockBinding(m_program_id, blockIndex, index_value);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Could not assign uniform block binding");

		gl.bindBuffer(GL_UNIFORM_BUFFER, m_uniform_buffer_ids[index_value]);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Could not bind buffer object");

		gl.bufferData(GL_UNIFORM_BUFFER, m_n_position_components * sizeof(float),
					  m_position_data + m_n_position_components * index_value, GL_STATIC_READ);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Could not set buffer object data");

		gl.bindBufferBase(GL_UNIFORM_BUFFER, index_value, m_uniform_buffer_ids[index_value]);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Could not bind buffer object to uniform block binding point");
	}

	/* Retrieve 'index' uniform location. */
	glw::GLint index_uniform_location = gl.getUniformLocation(m_program_id, "index");

	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetUniformLocation() call failed.");

	if (index_uniform_location == -1)
	{
		TCU_FAIL("Could not get index uniform location!");
	}

	/* Run the test */
	bool testFailed = false;

	for (glw::GLuint index_value = 0; index_value < m_n_array_size; ++index_value)
	{
		if (!drawAndCheckResult(index_uniform_location, index_value))
		{
			testFailed = true;

			break;
		}
	}

	if (testFailed)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
	}
	else
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}

	return STOP;
}

/** Draws and checks result data fetched via transform feedback
 *
 *   @param index_value value to be set for the index uniform variable.
 *
 *   @return true if the result data is correct, false otherwise
 */
bool GPUShader5UniformBlocksArrayIndexing::drawAndCheckResult(glw::GLuint index_location, glw::GLuint index_value)
{
	const glw::Functions& gl	 = m_context.getRenderContext().getFunctions();
	bool				  result = true;

	gl.uniform1ui(index_location, index_value);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUniform1ui() call failed");

	gl.enable(GL_RASTERIZER_DISCARD);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glEnable(GL_RASTERIZER_DISCARD) call failed");

	gl.beginTransformFeedback(GL_POINTS);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBeginTransformFeedback(GL_POINTS) call failed");

	gl.drawArrays(GL_POINTS, 0, /* first */
				  1);			/* count */
	GLU_EXPECT_NO_ERROR(gl.getError(), "Rendering failed!");

	gl.endTransformFeedback();
	GLU_EXPECT_NO_ERROR(gl.getError(), "glEndTransformFeedback() call failed");

	gl.disable(GL_RASTERIZER_DISCARD);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glDisable(GL_RASTERIZER_DISCARD) call failed");

	/* Fetch the results via transform feedback */
	const glw::GLfloat* feedback_result =
		(glw::GLfloat*)gl.mapBufferRange(GL_TRANSFORM_FEEDBACK_BUFFER, 0, /* offset */
										 sizeof(float) * m_n_position_components, GL_MAP_READ_BIT);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not map buffer to process space");

	if (de::abs(feedback_result[0] - m_position_data[index_value * m_n_position_components + 0]) > m_epsilon_float ||
		de::abs(feedback_result[1] - m_position_data[index_value * m_n_position_components + 1]) > m_epsilon_float ||
		de::abs(feedback_result[2] - m_position_data[index_value * m_n_position_components + 2]) > m_epsilon_float ||
		de::abs(feedback_result[3] - m_position_data[index_value * m_n_position_components + 3]) > m_epsilon_float)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Expected Data ("
						   << m_position_data[index_value * m_n_position_components + 0] << ", "
						   << m_position_data[index_value * m_n_position_components + 1] << ", "
						   << m_position_data[index_value * m_n_position_components + 2] << ", "
						   << m_position_data[index_value * m_n_position_components + 3] << ") Result Data ("
						   << feedback_result[0] << ", " << feedback_result[1] << ", " << feedback_result[2] << ", "
						   << feedback_result[3] << ")" << tcu::TestLog::EndMessage;
		result = false;
	}

	gl.unmapBuffer(GL_TRANSFORM_FEEDBACK_BUFFER);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUnmapBuffer() call failed");

	return result;
}

/** Deinitializes GLES objects created during the test.
 *
 */
void GPUShader5UniformBlocksArrayIndexing::deinit(void)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Reset OpenGL ES state */
	gl.useProgram(0);
	gl.bindBuffer(GL_ARRAY_BUFFER, 0);
	gl.bindBuffer(GL_UNIFORM_BUFFER, 0);
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

	if (m_tf_buffer_id != 0)
	{
		gl.deleteBuffers(1, &m_tf_buffer_id);

		m_tf_buffer_id = 0;
	}

	if (m_uniform_buffer_ids != DE_NULL)
	{
		gl.deleteBuffers(m_n_array_size, m_uniform_buffer_ids);

		delete[] m_uniform_buffer_ids;
		m_uniform_buffer_ids = DE_NULL;
	}

	if (m_vao_id != 0)
	{
		gl.deleteVertexArrays(1, &m_vao_id);

		m_vao_id = 0;
	}

	/* Call base class' deinit() */
	TestCaseBase::deinit();
}

} // namespace glcts
