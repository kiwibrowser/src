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
 * \file esextcGPUShader5FmaPrecision.cpp
 * \brief gpu_shader5 extension - fma precision Test (Test 8)
 */ /*-------------------------------------------------------------------*/

#include "esextcGPUShader5FmaPrecision.hpp"

#include "deDefs.hpp"
#include "deMath.h"
#include "gluDefs.hpp"
#include "glwEnums.hpp"
#include "glwFunctions.hpp"
#include "tcuTestLog.hpp"
#include <cstring>
#include <sstream>

namespace glcts
{
/** Constructor
 *  @param S             Type of input data
 *  @param context       Test context
 *  @param name          Test case's name
 *  @param description   Test case's description
 */
template <INPUT_DATA_TYPE S>
GPUShader5FmaPrecision<S>::GPUShader5FmaPrecision(Context& context, const ExtParameters& extParams, const char* name,
												  const char* description)
	: TestCaseBase(context, extParams, name, description)
	, m_amplitude(100.0f)
	, m_fs_id(0)
	, m_po_id(0)
	, m_vao_id(0)
	, m_vbo_a_id(0)
	, m_vbo_b_id(0)
	, m_vbo_c_id(0)
	, m_vbo_result_fma_id(0)
	, m_vbo_result_std_id(0)
	, m_vs_id(0)
{
	/* Nothing to be done here */
}

/** Deinitializes GLES objects created during the test.
 *
 */
template <INPUT_DATA_TYPE S>
void					  GPUShader5FmaPrecision<S>::deinit(void)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Reset OpenGL ES state */
	gl.useProgram(0);
	gl.bindBuffer(GL_ARRAY_BUFFER, 0);
	gl.bindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, 0);
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

	if (m_vbo_a_id != 0)
	{
		gl.deleteBuffers(1, &m_vbo_a_id);

		m_vbo_a_id = 0;
	}

	if (m_vbo_b_id != 0)
	{
		gl.deleteBuffers(1, &m_vbo_b_id);

		m_vbo_b_id = 0;
	}

	if (m_vbo_c_id != 0)
	{
		gl.deleteBuffers(1, &m_vbo_c_id);

		m_vbo_c_id = 0;
	}

	if (m_vbo_result_fma_id != 0)
	{
		gl.deleteBuffers(1, &m_vbo_result_fma_id);

		m_vbo_result_fma_id = 0;
	}

	if (m_vbo_result_std_id != 0)
	{
		gl.deleteBuffers(1, &m_vbo_result_std_id);

		m_vbo_result_std_id = 0;
	}

	if (m_vao_id != 0)
	{
		gl.deleteVertexArrays(1, &m_vao_id);

		m_vao_id = 0;
	}

	/* Call base class' deinit() */
	TestCaseBase::deinit();
}

/** Initializes GLES objects used during the test.
 *
 */
template <INPUT_DATA_TYPE S>
void					  GPUShader5FmaPrecision<S>::initTest(void)
{
	/* Check if gpu_shader5 extension is supported */
	if (!m_is_gpu_shader5_supported)
	{
		throw tcu::NotSupportedError(GPU_SHADER5_EXTENSION_NOT_SUPPORTED, "", __FILE__, __LINE__);
	}

	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* generate test data */
	generateData();

	/* Set up shader and program objects */
	m_fs_id = gl.createShader(GL_FRAGMENT_SHADER);
	m_vs_id = gl.createShader(GL_VERTEX_SHADER);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not create shader objects!");

	m_po_id = gl.createProgram();
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not create program object!");

	/* Set up transform feedback */
	gl.enable(GL_RASTERIZER_DISCARD);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glEnable(GL_RASTERIZER_DISCARD) call failed");

	const char* varyings[] = { "resultFma", "resultStd" };

	gl.transformFeedbackVaryings(m_po_id, 2, varyings, GL_SEPARATE_ATTRIBS);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTransformFeedbackVaryings() failed");

	/* Get shader code */
	const char* fsCode	= getFragmentShaderCode();
	std::string vsCode	= generateVertexShaderCode();
	const char* vsCodeStr = vsCode.c_str();

	if (!buildProgram(m_po_id, m_fs_id, 1 /* part */, &fsCode, m_vs_id, 1 /* part */, &vsCodeStr))
	{
		TCU_FAIL("Could not create a program from valid vertex/fragment shader!");
	}

	/* Create and bind vertex array object */
	gl.genVertexArrays(1, &m_vao_id);
	gl.bindVertexArray(m_vao_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error configuring vertex array object!");

	/* Configure buffer objects with input data*/
	gl.genBuffers(1, &m_vbo_a_id);
	gl.bindBuffer(GL_ARRAY_BUFFER, m_vbo_a_id);
	gl.bufferData(GL_ARRAY_BUFFER, sizeof(m_data_a), m_data_a, GL_STATIC_READ);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error configuring buffer object!");

	gl.genBuffers(1, &m_vbo_b_id);
	gl.bindBuffer(GL_ARRAY_BUFFER, m_vbo_b_id);
	gl.bufferData(GL_ARRAY_BUFFER, sizeof(m_data_b), m_data_b, GL_STATIC_READ);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error configuring buffer object!");

	gl.genBuffers(1, &m_vbo_c_id);
	gl.bindBuffer(GL_ARRAY_BUFFER, m_vbo_c_id);
	gl.bufferData(GL_ARRAY_BUFFER, sizeof(m_data_c), m_data_c, GL_STATIC_READ);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error configuring buffer object!");

	gl.useProgram(m_po_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not use program!");

	/* Configure vertex attrib pointers */
	glw::GLint posAttribA = gl.getAttribLocation(m_po_id, "a");

	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetAttribLocation() failed");

	gl.bindBuffer(GL_ARRAY_BUFFER, m_vbo_a_id);
	gl.vertexAttribPointer(posAttribA, S, GL_FLOAT, GL_FALSE, 0 /* stride */, DE_NULL);
	gl.enableVertexAttribArray(posAttribA);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Error configuring input vertex data attrib pointer!");

	glw::GLint posAttribB = gl.getAttribLocation(m_po_id, "b");

	gl.bindBuffer(GL_ARRAY_BUFFER, m_vbo_b_id);
	gl.vertexAttribPointer(posAttribB, S, GL_FLOAT, GL_FALSE, 0 /* stride */, DE_NULL);
	gl.enableVertexAttribArray(posAttribB);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Error configuring input vertex data attrib pointer!");

	glw::GLint posAttribC = gl.getAttribLocation(m_po_id, "c");

	gl.bindBuffer(GL_ARRAY_BUFFER, m_vbo_c_id);
	gl.vertexAttribPointer(posAttribC, S, GL_FLOAT, GL_FALSE, 0 /* stride */, DE_NULL);
	gl.enableVertexAttribArray(posAttribC);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Error configuring input vertex data attrib pointer!");

	/* Configure buffer objects for captured results */
	gl.genBuffers(1, &m_vbo_result_fma_id);
	gl.bindBuffer(GL_ARRAY_BUFFER, m_vbo_result_fma_id);
	gl.bufferData(GL_ARRAY_BUFFER, m_n_elements * S * sizeof(glw::GLfloat), DE_NULL, GL_DYNAMIC_COPY);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error configuring buffer object!");

	gl.genBuffers(1, &m_vbo_result_std_id);
	gl.bindBuffer(GL_ARRAY_BUFFER, m_vbo_result_std_id);
	gl.bufferData(GL_ARRAY_BUFFER, m_n_elements * S * sizeof(glw::GLfloat), DE_NULL, GL_DYNAMIC_COPY);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error configuring buffer object!");

	gl.bindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0 /* index */, m_vbo_result_fma_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error binding buffer object to transform feedback binding point!");

	gl.bindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 1 /* index */, m_vbo_result_std_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error binding buffer object to transform feedback binding point!");
}

/** Executes the test.
 *  Sets the test result to QP_TEST_RESULT_FAIL if the test failed, QP_TEST_RESULT_PASS otherwise.
 *
 *  @return STOP if the test has finished, CONTINUE to indicate iterate should be called once again.
 *
 *  Note the function throws exception should an error occur!
 */
template <INPUT_DATA_TYPE	S>
tcu::TestNode::IterateResult GPUShader5FmaPrecision<S>::iterate(void)
{
	initTest();

	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Render */
	gl.beginTransformFeedback(GL_POINTS);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBeginTransformFeedback() call failed");

	gl.drawArrays(GL_POINTS, 0, m_n_elements);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Rendering failed!");

	gl.endTransformFeedback();
	GLU_EXPECT_NO_ERROR(gl.getError(), "glEndTransformFeedback() call failed");

	/* Retrieve the result data */
	glw::GLfloat		resultFma[m_n_elements * S];
	glw::GLfloat		resultStd[m_n_elements * S];
	const glw::GLfloat* resultTmp = DE_NULL;

	gl.bindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, m_vbo_result_fma_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer() call failed");

	resultTmp = (const glw::GLfloat*)gl.mapBufferRange(GL_TRANSFORM_FEEDBACK_BUFFER, 0, /* offset */
													   m_n_elements * S * sizeof(glw::GLfloat), GL_MAP_READ_BIT);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error mapping buffer object's data to client space!");

	memcpy(resultFma, resultTmp, m_n_elements * S * sizeof(glw::GLfloat));

	gl.unmapBuffer(GL_TRANSFORM_FEEDBACK_BUFFER);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error unmapping buffer object's data!");

	gl.bindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, m_vbo_result_std_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer() call failed");

	resultTmp = (const glw::GLfloat*)gl.mapBufferRange(GL_TRANSFORM_FEEDBACK_BUFFER, 0, /* offset */
													   m_n_elements * S * sizeof(glw::GLfloat), GL_MAP_READ_BIT);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error mapping buffer object's data to client space!");

	memcpy(resultStd, resultTmp, m_n_elements * S * sizeof(glw::GLfloat));

	gl.unmapBuffer(GL_TRANSFORM_FEEDBACK_BUFFER);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error unmapping buffer object's data!");

	/* Execute the algorithm from shader on CPU */
	glw::GLfloat resultCPURNE[m_n_elements * S];
	glw::GLfloat resultCPURTZ[m_n_elements * S];

	deRoundingMode previousRoundingMode = deGetRoundingMode();

	deSetRoundingMode(DE_ROUNDINGMODE_TO_NEAREST_EVEN);
	for (glw::GLuint i = 0; i < m_n_elements; ++i)
	{
		for (glw::GLuint j = 0; j < S; ++j)
		{
			resultCPURNE[i * S + j] = m_data_a[i * S + j] * m_data_b[i * S + j] + m_data_c[i * S + j];
		}
	}

	deSetRoundingMode(DE_ROUNDINGMODE_TO_ZERO);
	for (glw::GLuint i = 0; i < m_n_elements; ++i)
	{
		for (glw::GLuint j = 0; j < S; ++j)
		{
			resultCPURTZ[i * S + j] = m_data_a[i * S + j] * m_data_b[i * S + j] + m_data_c[i * S + j];
		}
	}

	/* Restore the rounding mode so subsequent tests aren't affected */
	deSetRoundingMode(previousRoundingMode);

	/* Check results */
	const glw::GLfloat* resultsCPU[] = { resultCPURNE, resultCPURTZ };
	FloatConverter		cpuU;
	FloatConverter		fmaU;
	FloatConverter		stdU;
	glw::GLboolean		test_failed = true;

	for (glw::GLuint roundingMode = 0; test_failed && roundingMode < 2; ++roundingMode)
	{
		glw::GLboolean rounding_mode_failed = false;
		for (glw::GLuint i = 0; i < m_n_elements; ++i)
		{
			for (int j = 0; j < S; ++j)
			{
				/* Assign float value to the union */
				cpuU.m_float = resultsCPU[roundingMode][i * S + j];
				fmaU.m_float = resultFma[i * S + j];
				stdU.m_float = resultStd[i * S + j];

				/* Convert float to int bitwise */
				glw::GLint cpuTemp = cpuU.m_int;
				glw::GLint fmaTemp = fmaU.m_int;
				glw::GLint stdTemp = stdU.m_int;

				glw::GLboolean diffCpuFma = de::abs(cpuTemp - fmaTemp) > 2;
				glw::GLboolean diffCpuStd = de::abs(cpuTemp - stdTemp) > 2;
				glw::GLboolean diffFmaStd = de::abs(fmaTemp - stdTemp) > 2;

				if (diffCpuFma || diffCpuStd || diffFmaStd)
				{
					rounding_mode_failed = true;
					break;
				}
			}

			if (rounding_mode_failed)
			{
				break;
			}
			else
			{
				test_failed = false;
			}
		} /* for (all elements) */
	}	 /* for (all rounding modes) */

	if (test_failed)
	{
		m_testCtx.getLog()
			<< tcu::TestLog::Message
			<< "The values of resultStd[i] & 0xFFFFFFFE and resultFma[i] & 0xFFFFFFFE and resultCPU[i] & 0xFFFFFFFE "
			<< "are not bitwise equal for i = 0..99\n"
			<< tcu::TestLog::EndMessage;

		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
	}
	else
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}

	return STOP;
}

/** Generate random input data */
template <INPUT_DATA_TYPE S>
void					  GPUShader5FmaPrecision<S>::generateData()
{
	/* Intialize with 1, because we want the same sequence of random values everytime we run test */
	randomSeed(1);

	/* Data generation */
	for (unsigned int i = 0; i < m_n_elements; i++)
	{
		for (int j = 0; j < S; j++)
		{
			float a, b, c;

			a = static_cast<float>(randomFormula(RAND_MAX)) /
					(static_cast<float>(static_cast<float>(RAND_MAX) / static_cast<float>(m_amplitude * 2.0f))) -
				m_amplitude;
			b = static_cast<float>(randomFormula(RAND_MAX)) /
					(static_cast<float>(static_cast<float>(RAND_MAX) / static_cast<float>(m_amplitude * 2.0f))) -
				m_amplitude;
			c = static_cast<float>(randomFormula(RAND_MAX)) /
					(static_cast<float>(static_cast<float>(RAND_MAX) / static_cast<float>(m_amplitude * 2.0f))) -
				m_amplitude;

			// If values are of opposite sign, catastrophic cancellation is possible. 1 LSB of error
			// tolerance is relative to the larger intermediate terms, and once you compute a*b+c
			// you might get values with smaller exponents. Scale down one of the terms so that either
			// |a*b| < 0.5*|c| or |c| < 0.5 * |a*b| so that the result is no smaller than half of the larger of a*b or c.

			float axb = a * b;
			if (deFloatSign(axb) != deFloatSign(c))
			{
				if (de::inRange(de::abs(axb), de::abs(c), 2 * de::abs(c)))
				{
					c /= 2.0f;
				}
				else if (de::inRange(de::abs(c), de::abs(axb), 2 * de::abs(axb)))
				{
					a /= 2.0f;
				}
			}

			m_data_a[i * S + j] = a;
			m_data_b[i * S + j] = b;
			m_data_c[i * S + j] = c;
		}
	}
}

/** Returns code for Vertex Shader
 *
 *  @return pointer to literal with Vertex Shader code
 */
template <INPUT_DATA_TYPE S>
std::string				  GPUShader5FmaPrecision<S>::generateVertexShaderCode()
{
	std::string type;

	switch (S)
	{
	case IDT_FLOAT:
	{
		type = "float";

		break;
	}

	case IDT_VEC2:
	{
		type = "vec2";

		break;
	}

	case IDT_VEC3:
	{
		type = "vec3";

		break;
	}

	case IDT_VEC4:
	{
		type = "vec4";

		break;
	}

	default:
	{
		TCU_FAIL("Incorrect variable type!");
		break;
	}
	} /* switch(S) */

	/* Generate the vertex shader code */
	std::stringstream vsCode;

	vsCode << "${VERSION}\n"
			  "\n"
			  "${GPU_SHADER5_REQUIRE}\n"
			  "\n"
			  "precision highp float;\n"
			  "\n"
			  "layout(location = 0) in "
		   << type << " a;\n"
		   << "layout(location = 1) in " << type << " b;\n"
		   << "layout(location = 2) in " << type << " c;\n"
		   << "\n"
		   << "layout(location = 0) out " << type << " resultFma;\n"
		   << "layout(location = 1) precise out " << type << " resultStd;\n"
		   << "\n"
		   << "\n"
		   << "void main()\n"
		   << "{\n"
		   << "    resultFma = fma(a,b,c);\n"
		   << "    resultStd = a * b + c;\n"
		   << "}\n";

	return vsCode.str();
}

/** Returns code for Fragment Shader
 *
 *  @return pointer to literal with Fragment Shader code
 */
template <INPUT_DATA_TYPE S>
const char*				  GPUShader5FmaPrecision<S>::getFragmentShaderCode()
{
	static const char* result = "${VERSION}\n"
								"\n"
								"${GPU_SHADER5_REQUIRE}\n"
								"\n"
								"precision highp float;\n"
								"\n"
								"void main(void)\n"
								"{\n"
								"}\n";

	return result;
}

} // namespace glcts
