/*-------------------------------------------------------------------------
 * OpenGL Conformance Test Suite
 * -----------------------------
 *
 * Copyright (c) 2015-2016 The Khronos Group Inc.
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
#include "gluDefs.hpp"
#include "glwEnums.hpp"
#include "glwFunctions.hpp"
#include "tcuTestLog.hpp"
#include <math.h>
#include <string.h>

#include "esextcGeometryShaderQualifiers.hpp"

namespace glcts
{
/** Constructor
 *
 * @param context       Test context
 * @param name          Test case's name
 * @param description   Test case's desricption
 **/
GeometryShaderFlatInterpolationTest::GeometryShaderFlatInterpolationTest(Context&			  context,
																		 const ExtParameters& extParams,
																		 const char* name, const char* description)
	: TestCaseBase(context, extParams, name, description)
	, m_bo_id(0)
	, m_fs_id(0)
	, m_gs_id(0)
	, m_po_id(0)
	, m_vao_id(0)
	, m_vs_id(0)
{
	/* Nothing to be done here */
}

/** Deinitializes GLES objects created during the test. */
void GeometryShaderFlatInterpolationTest::deinit(void)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Deinitialize ES objects */
	if (m_bo_id != 0)
	{
		gl.deleteBuffers(1, &m_bo_id);

		m_bo_id = 0;
	}

	if (m_fs_id != 0)
	{
		gl.deleteShader(m_fs_id);

		m_fs_id = 0;
	}

	if (m_gs_id != 0)
	{
		gl.deleteShader(m_gs_id);

		m_gs_id = 0;
	}

	if (m_po_id != 0)
	{
		gl.deleteProgram(m_po_id);

		m_po_id = 0;
	}

	if (m_vao_id != 0)
	{
		gl.deleteVertexArrays(1, &m_vao_id);

		m_vao_id = 0;
	}

	if (m_vs_id != 0)
	{
		gl.deleteShader(m_vs_id);

		m_vs_id = 0;
	}

	/* Deinitialize base implementation */
	TestCaseBase::deinit();
}

/* Initializes a program object to be used for the conformance test. */
void GeometryShaderFlatInterpolationTest::initProgram()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Create shader objects */
	m_fs_id = gl.createShader(GL_FRAGMENT_SHADER);
	m_gs_id = gl.createShader(GL_GEOMETRY_SHADER);
	m_vs_id = gl.createShader(GL_VERTEX_SHADER);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateShader() call(s) failed.");

	/* Create the program object */
	m_po_id = gl.createProgram();

	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateProgram() call failed.");

	/* Set the test program object up */
	static const char* fs_code_raw = "${VERSION}\n"
									 "\n"
									 "precision highp float;\n"
									 "\n"
									 "out vec4 result;\n"
									 "\n"
									 "void main()\n"
									 "{\n"
									 "    result = vec4(1.0);\n"
									 "}\n";

	static const char* gs_code_raw = "${VERSION}\n"
									 "${GEOMETRY_SHADER_REQUIRE}\n"
									 "\n"
									 "layout(triangles)                in;\n"
									 "layout(points, max_vertices = 1) out;\n"
									 "\n"
									 "flat in int out_vertex[];\n"
									 "\n"
									 "void main()\n"
									 "{\n"
									 "    if (out_vertex[0] != 0 || out_vertex[1] != 1 || out_vertex[2] != 2)\n"
									 "    {\n"
									 "        gl_Position = vec4(0.0);\n"
									 "    }\n"
									 "    else\n"
									 "    {\n"
									 "        gl_Position = vec4(1.0);\n"
									 "    }\n"
									 "    EmitVertex();\n"
									 "}\n";

	static const char* po_varying  = "gl_Position";
	static const char* vs_code_raw = "${VERSION}\n"
									 "\n"
									 "flat out int out_vertex;\n"
									 "\n"
									 "void main()\n"
									 "{\n"
									 "    gl_Position = vec4(0.0);\n"
									 "    out_vertex  = gl_VertexID;\n"
									 "}\n";

	std::string fs_code_specialized = TestCaseBase::specializeShader(1, /* parts */
																	 &fs_code_raw);
	const char* fs_code_specialized_raw = fs_code_specialized.c_str();
	std::string gs_code_specialized		= TestCaseBase::specializeShader(1, /* parts */
																	 &gs_code_raw);
	const char* gs_code_specialized_raw = gs_code_specialized.c_str();
	std::string vs_code_specialized		= TestCaseBase::specializeShader(1, /* parts */
																	 &vs_code_raw);
	const char* vs_code_specialized_raw = vs_code_specialized.c_str();

	gl.transformFeedbackVaryings(m_po_id, 1, /* count */
								 &po_varying, GL_INTERLEAVED_ATTRIBS);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTransformFeedbackVaryings() call failed.");

	if (!TestCaseBase::buildProgram(m_po_id, m_gs_id, 1,				  /* n_sh1_body_parts */
									&gs_code_specialized_raw, m_vs_id, 1, /* n_sh2_body_parts */
									&vs_code_specialized_raw, m_fs_id, 1, /* n_sh3_body_parts */
									&fs_code_specialized_raw))
	{
		TCU_FAIL("Could not build test program object");
	}
}

/** Executes the test.
 *  Sets the test result to QP_TEST_RESULT_FAIL if the test failed, QP_TEST_RESULT_PASS otherwise.
 *  @return STOP if the test has finished, CONTINUE to indicate iterate should be called once again.
 *  Note the function throws exception should an error occur!
 **/
tcu::TestNode::IterateResult GeometryShaderFlatInterpolationTest::iterate(void)
{
	const glw::Functions& gl	 = m_context.getRenderContext().getFunctions();
	bool				  result = true;

	/* This test should only run if EXT_geometry_shader is supported. */
	if (!m_is_geometry_shader_extension_supported)
	{
		throw tcu::NotSupportedError(GEOMETRY_SHADER_EXTENSION_NOT_SUPPORTED, "", __FILE__, __LINE__);
	}

	/* Set up the buffer object */
	gl.genBuffers(1, &m_bo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenBuffers() call failed.");

	gl.bindBuffer(GL_ARRAY_BUFFER, m_bo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer() call failed.");

	gl.bufferData(GL_ARRAY_BUFFER, sizeof(float) * 4, /* components */
				  DE_NULL,							  /* data */
				  GL_STATIC_DRAW);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferData() call failed.");

	gl.bindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, /* index */
					  m_bo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBufferBase() call failed.");

	/* Set up the vertex array object */
	gl.genVertexArrays(1, &m_vao_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenVertexArrays() call failed.");

	gl.bindVertexArray(m_vao_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindVertexArray() call failed.");

	/* Set up the test program */
	initProgram();

	gl.useProgram(m_po_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram() call failed.");

	gl.beginTransformFeedback(GL_POINTS);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBeginTransformFeedback() call failed.");

	/* Draw the input triangle */
	gl.drawArrays(GL_TRIANGLES, 0, /* first */
				  3);			   /* count */
	GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawArrays() call failed.");

	gl.endTransformFeedback();
	GLU_EXPECT_NO_ERROR(gl.getError(), "glEndTransformFeedback() call failed.");

	/* Map the BO contents to the process space */
	const float* result_bo_data = (const float*)gl.mapBufferRange(GL_ARRAY_BUFFER, 0, /* offset */
																  sizeof(float) * 4, GL_MAP_READ_BIT);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glMapBufferRange() call failed.");

	if (fabs(result_bo_data[0] - 1.0f) >= 1e-5f || fabs(result_bo_data[1] - 1.0f) >= 1e-5f ||
		fabs(result_bo_data[2] - 1.0f) >= 1e-5f || fabs(result_bo_data[3] - 1.0f) >= 1e-5f)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Invalid texel data was retrieved." << tcu::TestLog::EndMessage;

		result = false;
	}

	gl.unmapBuffer(GL_ARRAY_BUFFER);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUnmapBuffer() call failed.");

	/* All done */
	if (result)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
	}

	return STOP;
}

} // namespace glcts
