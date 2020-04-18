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

#include "esextcGeometryShaderPrimitiveQueries.hpp"

#include "gluDefs.hpp"
#include "glwEnums.hpp"
#include "glwFunctions.hpp"
#include "tcuTestLog.hpp"

namespace glcts
{

/* Fragment shader */
const char* GeometryShaderPrimitiveQueries::m_fs_code = "${VERSION}\n"
														"\n"
														"precision highp float;\n"
														"\n"
														"void main()\n"
														"{\n"
														"}\n";

/* Vertex shader */
const char* GeometryShaderPrimitiveQueries::m_vs_code = "${VERSION}\n"
														"\n"
														"precision highp float;\n"
														"\n"
														"void main()\n"
														"{\n"
														"    gl_Position = vec4(gl_VertexID, 0, 0, 1);\n"
														"}\n";

/* Geometry shader */
const char* GeometryShaderPrimitiveQueriesPoints::m_gs_code =
	"${VERSION}\n"
	"\n"
	"${GEOMETRY_SHADER_REQUIRE}\n"
	"\n"
	"precision highp float;\n"
	"\n"
	"layout(points)                          in;\n"
	"layout(points, max_vertices=8)          out;\n"
	"\n"
	"void main()\n"
	"{\n"
	"    for (int n = 0; n < 8; ++n)\n"
	"    {\n"
	"        gl_Position = vec4(1.0 / (float(n) + 1.0), 1.0 / (float(n) + 2.0), 0.0, 1.0);\n"
	"        EmitVertex();\n"
	"    }\n"
	"\n"
	"    EndPrimitive();\n"
	"}\n";

/* Geometry shader */
const char* GeometryShaderPrimitiveQueriesLines::m_gs_code =
	"${VERSION}\n"
	"\n"
	"${GEOMETRY_SHADER_REQUIRE}\n"
	"\n"
	"precision highp float;\n"
	"\n"
	"layout(points)                          in;\n"
	"layout(line_strip, max_vertices=10)     out;\n"
	"\n"
	"void main()\n"
	"{\n"
	"    for (int n = 0; n < 10; ++n)\n"
	"    {\n"
	"        gl_Position = vec4(1.0 / (float(n) + 1.0), 1.0 / (float(n) + 2.0), 0.0, 1.0);\n"
	"        EmitVertex();\n"
	"    }\n"
	"\n"
	"    EndPrimitive();\n"
	"}\n";

/* Geometry shader */
const char* GeometryShaderPrimitiveQueriesTriangles::m_gs_code =
	"${VERSION}\n"
	"\n"
	"${GEOMETRY_SHADER_REQUIRE}\n"
	"\n"
	"precision highp float;\n"
	"\n"
	"layout(points)                          in;\n"
	"layout(triangle_strip, max_vertices=12) out;\n"
	"\n"
	"void main()\n"
	"{\n"
	"    for (int n = 0; n < 12; ++n)\n"
	"    {\n"
	"        gl_Position = vec4(1.0 / (float(n) + 1.0), 1.0 / (float(n) + 2.0), 0.0, 1.0);\n"
	"        EmitVertex();\n"
	"    }\n"
	"\n"
	"    EndPrimitive();\n"
	"}\n";

/** Constructor
 *
 * @param context       Test context
 * @param name          Test case's name
 * @param description   Test case's description
 **/
GeometryShaderPrimitiveQueriesPoints::GeometryShaderPrimitiveQueriesPoints(Context&				context,
																		   const ExtParameters& extParams,
																		   const char* name, const char* description)
	: GeometryShaderPrimitiveQueries(context, extParams, name, description)
{
}

/** Gets geometry shader code
 *
 * @return geometry shader code
 **/
const char* GeometryShaderPrimitiveQueriesPoints::getGeometryShaderCode()
{
	return m_gs_code;
}

/** Gets the number of emitted vertices
 *
 * @return number of emitted vertices
 **/
glw::GLint GeometryShaderPrimitiveQueriesPoints::getAmountOfEmittedVertices()
{
	return 8;
}

/** Gets the transform feedback mode
 *
 * @return transform feedback mode
 **/
glw::GLenum GeometryShaderPrimitiveQueriesPoints::getTFMode()
{
	return GL_POINTS;
}

/** Constructor
 *
 * @param context       Test context
 * @param name          Test case's name
 * @param description   Test case's desricption
 **/
GeometryShaderPrimitiveQueriesLines::GeometryShaderPrimitiveQueriesLines(Context&			  context,
																		 const ExtParameters& extParams,
																		 const char* name, const char* description)
	: GeometryShaderPrimitiveQueries(context, extParams, name, description)
{
}

/** Gets geometry shader code
 *
 * @return geometry shader code
 **/
const char* GeometryShaderPrimitiveQueriesLines::getGeometryShaderCode()
{
	return m_gs_code;
}

/** Gets the number of emitted vertices
 *
 * @return number of emitted vertices
 **/
glw::GLint GeometryShaderPrimitiveQueriesLines::getAmountOfEmittedVertices()
{
	return 18;
}

/** Gets the transform feedback mode
 *
 * @return transform feedback mode
 **/
glw::GLenum GeometryShaderPrimitiveQueriesLines::getTFMode()
{
	return GL_LINES;
}

/** Constructor
 *
 * @param context       Test context
 * @param name          Test case's name
 * @param description   Test case's desricption
 **/
GeometryShaderPrimitiveQueriesTriangles::GeometryShaderPrimitiveQueriesTriangles(Context&			  context,
																				 const ExtParameters& extParams,
																				 const char*		  name,
																				 const char*		  description)
	: GeometryShaderPrimitiveQueries(context, extParams, name, description)
{
}

/** Gets geometry shader code
 *
 * @return geometry shader code
 **/
const char* GeometryShaderPrimitiveQueriesTriangles::getGeometryShaderCode()
{
	return m_gs_code;
}

/** Gets the number of emitted vertices
 *
 * @return number of emitted vertices
 **/
glw::GLint GeometryShaderPrimitiveQueriesTriangles::getAmountOfEmittedVertices()
{
	return 30;
}

/** Gets the transform feedback mode
 *
 * @return transform feedback mode
 **/
glw::GLenum GeometryShaderPrimitiveQueriesTriangles::getTFMode()
{
	return GL_TRIANGLES;
}

/** Constructor
 *
 * @param context       Test context
 * @param name          Test case's name
 * @param description   Test case's desricption
 **/
GeometryShaderPrimitiveQueries::GeometryShaderPrimitiveQueries(Context& context, const ExtParameters& extParams,
															   const char* name, const char* description)
	: TestCaseBase(context, extParams, name, description)
	, m_n_texture_components(4)
	, m_bo_large_id(0)
	, m_bo_small_id(0)
	, m_fs_id(0)
	, m_gs_id(0)
	, m_po_id(0)
	, m_qo_primitives_generated_id(0)
	, m_qo_tf_primitives_written_id(0)
	, m_vao_id(0)
	, m_vs_id(0)
{
	/* Nothing to be done here */
}

/** Deinitializes GLES objects created during the test.
 *
 */
void GeometryShaderPrimitiveQueries::deinit(void)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Reset OpenGL ES state */
	gl.useProgram(0);
	gl.bindBuffer(GL_ARRAY_BUFFER, 0);
	gl.bindVertexArray(0);

	if (m_po_id != 0)
	{
		gl.deleteProgram(m_po_id);
	}

	if (m_fs_id != 0)
	{
		gl.deleteShader(m_fs_id);
	}

	if (m_gs_id != 0)
	{
		gl.deleteShader(m_gs_id);
	}

	if (m_vs_id != 0)
	{
		gl.deleteShader(m_vs_id);
	}

	if (m_bo_small_id != 0)
	{
		gl.deleteBuffers(1, &m_bo_small_id);
	}

	if (m_bo_large_id != 0)
	{
		gl.deleteBuffers(1, &m_bo_large_id);
	}

	if (m_qo_primitives_generated_id != 0)
	{
		gl.deleteQueries(1, &m_qo_primitives_generated_id);
	}

	if (m_qo_tf_primitives_written_id != 0)
	{
		gl.deleteQueries(1, &m_qo_tf_primitives_written_id);
	}

	if (m_vao_id != 0)
	{
		gl.deleteVertexArrays(1, &m_vao_id);
	}

	/* Release base class */
	TestCaseBase::deinit();
}

/** Executes the test.
 *  Sets the test result to QP_TEST_RESULT_FAIL if the test failed, QP_TEST_RESULT_PASS otherwise.
 *  @return STOP if the test has finished, CONTINUE to indicate iterate should be called once again.
 *  Note the function throws exception should an error occur!
 **/
tcu::TestNode::IterateResult GeometryShaderPrimitiveQueries::iterate(void)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Check if geometry_shader extension is supported */
	if (!m_is_geometry_shader_extension_supported)
	{
		throw tcu::NotSupportedError(GEOMETRY_SHADER_EXTENSION_NOT_SUPPORTED, "", __FILE__, __LINE__);
	}

	/* Create shader objects and a program object */
	m_vs_id = gl.createShader(GL_VERTEX_SHADER);
	m_fs_id = gl.createShader(GL_FRAGMENT_SHADER);
	m_gs_id = gl.createShader(m_glExtTokens.GEOMETRY_SHADER);
	m_po_id = gl.createProgram();

	GLU_EXPECT_NO_ERROR(gl.getError(), "Error creating program/shader objects.");

	/* Try to build test-specific program object */
	const char* tf_varyings[] = { "gl_Position" };
	const char* gs_code		  = getGeometryShaderCode();

	gl.transformFeedbackVaryings(m_po_id, sizeof(tf_varyings) / sizeof(tf_varyings[0]), tf_varyings,
								 GL_SEPARATE_ATTRIBS);

	if (!buildProgram(m_po_id, m_fs_id, 1 /* part */, &m_fs_code, m_gs_id, 1 /* part */, &gs_code, m_vs_id,
					  1 /* part */, &m_vs_code))
	{
		TCU_FAIL("Could not create a program for GeometryShaderPrimitiveQueries!");
	}

	/* Create and bind a vertex array object */
	gl.genVertexArrays(1, &m_vao_id);
	gl.bindVertexArray(m_vao_id);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Error creating a vertex array object.");

	/* Create two buffer objects
	 *
	 * One with sufficiently large storage space to hold position data for particular output.
	 * Another one of insufficient size.
	 */
	gl.genBuffers(1, &m_bo_large_id);
	gl.genBuffers(1, &m_bo_small_id);

	gl.bindBuffer(GL_ARRAY_BUFFER, m_bo_large_id);
	gl.bufferData(GL_ARRAY_BUFFER,
				  getAmountOfEmittedVertices() * m_n_texture_components /* components */ * sizeof(float), NULL,
				  GL_STATIC_DRAW);

	gl.bindBuffer(GL_ARRAY_BUFFER, m_bo_small_id);
	gl.bufferData(GL_ARRAY_BUFFER,
				  (getAmountOfEmittedVertices() / 2) * m_n_texture_components /* components */ * sizeof(float), NULL,
				  GL_STATIC_DRAW);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Error allocating buffer objects.");

	/* Create primitive query objects */
	gl.genQueries(1, &m_qo_tf_primitives_written_id);
	gl.genQueries(1, &m_qo_primitives_generated_id);

	glw::GLuint nPrimitivesGenerated = 0;
	glw::GLuint nTFPrimitivesWritten = 0;

	/* Test case 13.1 */
	readPrimitiveQueryValues(m_bo_large_id, &nPrimitivesGenerated, &nTFPrimitivesWritten);

	if (nPrimitivesGenerated == 0)
	{
		m_testCtx.getLog()
			<< tcu::TestLog::Message
			<< "Retrieved GL_PRIMITIVES_GENERATED_EXT query object value is zero which should never happen."
			<< tcu::TestLog::EndMessage;

		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
		return STOP;
	}

	if (nTFPrimitivesWritten == 0)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Retrieved GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN query "
													   "object value is zero which should never happen."
						   << tcu::TestLog::EndMessage;

		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
		return STOP;
	}

	if (nPrimitivesGenerated != nTFPrimitivesWritten)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Retrieved GL_PRIMITIVES_GENERATED_EXT query object value("
						   << nPrimitivesGenerated
						   << ") is different than for GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN ("
						   << nTFPrimitivesWritten << ")" << tcu::TestLog::EndMessage;

		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
		return STOP;
	}

	/* Test case 13.2 */
	nPrimitivesGenerated = 0;
	nTFPrimitivesWritten = 0;

	readPrimitiveQueryValues(m_bo_small_id, &nPrimitivesGenerated, &nTFPrimitivesWritten);

	if (nPrimitivesGenerated == 0)
	{
		m_testCtx.getLog()
			<< tcu::TestLog::Message
			<< "Retrieved GL_PRIMITIVES_GENERATED_EXT query object value is zero which should never happen."
			<< tcu::TestLog::EndMessage;

		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
		return STOP;
	}

	if (nTFPrimitivesWritten == 0)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Retrieved GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN query "
													   "object value is zero which should never happen."
						   << tcu::TestLog::EndMessage;

		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
		return STOP;
	}

	if ((nPrimitivesGenerated / 2) != nTFPrimitivesWritten)
	{
		m_testCtx.getLog() << tcu::TestLog::Message
						   << "Retrieved GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN query object value("
						   << nPrimitivesGenerated << ") should be half the amount of GL_PRIMITIVES_GENERATED_EXT ("
						   << nTFPrimitivesWritten << ")" << tcu::TestLog::EndMessage;

		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
		return STOP;
	}

	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	return STOP;
}

/**
 * The function binds the provided bufferId to transform feedback target and sets up a query
 * for GL_PRIMITIVES_GENERATED_EXT GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN.
 * It then issues a draw call to execute the vertex and geometry shaders. Then results
 * of the queries are written to nPrimitivesGenerated and nPrimitivesWritten variables.
 *
 * @param context bufferId   id of the buffer to be bound to transform feedback target
 * @return nPrimitivesGenerated      the result of GL_PRIMITIVES_GENERATED_EXT query
 * @return nPrimitivesWritten        the result of GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN query
 */
void GeometryShaderPrimitiveQueries::readPrimitiveQueryValues(glw::GLint bufferId, glw::GLuint* nPrimitivesGenerated,
															  glw::GLuint* nPrimitivesWritten)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Bind the buffer object to hold the captured data.*/
	gl.bindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, bufferId);

	/* Activate the program object */
	gl.useProgram(m_po_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error using program object");

	gl.beginTransformFeedback(getTFMode());
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error starting transform feedback");

	/* Activate the queries */
	gl.beginQuery(m_glExtTokens.PRIMITIVES_GENERATED, m_qo_primitives_generated_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error starting GL_PRIMITIVES_GENERATED_EXT query");

	gl.beginQuery(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN, m_qo_tf_primitives_written_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error starting GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN query");

	/* Render */
	gl.drawArrays(GL_POINTS, 0 /* first */, 1 /* count */);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error executing draw call");

	gl.endTransformFeedback();
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error finishing transform feedback");

	/* Query objects end here. */
	gl.endQuery(m_glExtTokens.PRIMITIVES_GENERATED);
	gl.endQuery(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN);

	/* Retrieve query values */
	gl.getQueryObjectuiv(m_qo_primitives_generated_id, GL_QUERY_RESULT, nPrimitivesGenerated);
	gl.getQueryObjectuiv(m_qo_tf_primitives_written_id, GL_QUERY_RESULT, nPrimitivesWritten);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Error retrieving query values.");
}

} // namespace glcts
