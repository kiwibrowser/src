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

#include "gluDefs.hpp"
#include "glwEnums.hpp"
#include "glwFunctions.hpp"
#include "tcuTestLog.hpp"

#include "esextcGeometryShaderAdjacency.hpp"
#include <math.h>

namespace glcts
{
/** Constructor
 *
 **/
AdjacencyGrid::AdjacencyGrid()
	: m_line_segments(0), m_points(0), m_triangles(0), m_n_points(0), m_n_segments(0), m_n_triangles(0)
{
	/* Nothing to be done here */
}

/** Destructor
 *
 **/
AdjacencyGrid::~AdjacencyGrid()
{
	if (m_line_segments)
	{
		delete[] m_line_segments;
		m_line_segments = 0;
	}

	if (m_points)
	{
		delete[] m_points;
		m_points = 0;
	}

	if (m_triangles)
	{
		delete[] m_triangles;
		m_triangles = 0;
	}
}

/** Constructor
 *
 **/
AdjacencyGridStrip::AdjacencyGridStrip() : m_n_points(0), m_points(0)
{
	/* Nothing to be done here */
}

/** Destructor
 *
 **/
AdjacencyGridStrip::~AdjacencyGridStrip()
{
	if (m_points)
	{
		delete[] m_points;
	}
}

/** Constructor
 *
 **/
AdjacencyTestData::AdjacencyTestData()
	: m_gs_code(0)
	, m_mode(0)
	, m_n_vertices(0)
	, m_grid(0)
	, m_geometry_bo_size(0)
	, m_index_data_bo_size(0)
	, m_vertex_data_bo_size(0)
	, m_expected_adjacency_geometry(0)
	, m_expected_geometry(0)
	, m_index_data(0)
	, m_tf_mode(0)
	, m_vertex_data(0)
{
	/* Nothing to be done here */
}

/** Destructor
 *
 **/
AdjacencyTestData::~AdjacencyTestData()
{
	if (m_expected_adjacency_geometry)
	{
		delete[] m_expected_adjacency_geometry;
		m_expected_adjacency_geometry = 0;
	}

	if (m_expected_geometry)
	{
		delete[] m_expected_geometry;
		m_expected_geometry = 0;
	}

	if (m_vertex_data)
	{
		delete[] m_vertex_data;
		m_vertex_data = 0;
	}

	if (m_index_data)
	{
		delete[] m_index_data;
		m_index_data = 0;
	}

	if (m_grid)
	{
		delete m_grid;
		m_grid = 0;
	}
}

/** Constructor
 *
 * @param context       Test context
 * @param name          Test case's name
 * @param description   Test case's desricption
 **/
GeometryShaderAdjacency::GeometryShaderAdjacency(Context& context, const ExtParameters& extParams, const char* name,
												 const char* description, AdjacencyTestData& testData)
	: TestCaseBase(context, extParams, name, description)
	, m_adjacency_geometry_bo_id(0)
	, m_fs_id(0)
	, m_geometry_bo_id(0)
	, m_gs_id(0)
	, m_index_data_bo_id(0)
	, m_vertex_data_bo_id(0)
	, m_po_id(0)
	, m_test_data(testData)
	, m_vao_id(0)
	, m_vs_id(0)
	, m_components_input(2)
	, m_epsilon(0.00001F)
	, m_position_attribute_location(0)
{
	/* Nothing to be done here */
}

/** Deinitializes GLES objects created during the test.
 *
 */
void GeometryShaderAdjacency::deinit(void)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Reset OpenGL ES state */
	gl.useProgram(0);
	gl.bindVertexArray(0);
	gl.bindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, 0);
	gl.bindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 1, 0);
	gl.bindBuffer(GL_ARRAY_BUFFER, 0);

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

	if (m_adjacency_geometry_bo_id != 0)
	{
		gl.deleteBuffers(1, &m_adjacency_geometry_bo_id);
	}
	if (m_geometry_bo_id != 0)
	{
		gl.deleteBuffers(1, &m_geometry_bo_id);
	}

	if (m_index_data_bo_id != 0)
	{
		gl.deleteBuffers(1, &m_index_data_bo_id);
	}

	if (m_vertex_data_bo_id != 0)
	{
		gl.deleteBuffers(1, &m_vertex_data_bo_id);
	}

	if (m_vao_id != 0)
	{
		gl.deleteVertexArrays(1, &m_vao_id);
	}

	TestCaseBase::deinit();
}

/** Returns code for Fragment Shader
 * @return pointer to literal with Fragment Shader code
 **/
const char* GeometryShaderAdjacency::getFragmentShaderCode()
{
	static const char* result = "${VERSION}\n"
								"\n"
								"precision highp float;\n"
								"\n"
								"void main()\n"
								"{\n"
								"}\n";
	return result;
}

/** Returns code for Vertex Shader
 * @return pointer to literal with Vertex Shader code
 **/
const char* GeometryShaderAdjacency::getVertexShaderCode()
{
	static const char* result = "${VERSION}\n"
								"\n"
								"precision highp float;\n"
								"\n"
								"layout(location = 0) in vec2 position_data;\n"
								"\n"
								"void main()\n"
								"{\n"
								"    gl_Position = vec4(position_data, 0, 1);\n"
								"}\n";
	return result;
}

/** Initializes GLES objects used during the test.
 *
 **/
void GeometryShaderAdjacency::initTest(void)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* check if EXT_geometry_shader extension is supported */
	if (!m_is_geometry_shader_extension_supported)
	{
		throw tcu::NotSupportedError(GEOMETRY_SHADER_EXTENSION_NOT_SUPPORTED, "", __FILE__, __LINE__);
	}

	gl.genVertexArrays(1, &m_vao_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not generate vertex array object");

	/* Get shader code */
	const char* fsCode = getFragmentShaderCode();
	const char* gsCode = m_test_data.m_gs_code;
	const char* vsCode = getVertexShaderCode();

	/* Create shader and program objects */
	m_fs_id = gl.createShader(GL_FRAGMENT_SHADER);
	m_vs_id = gl.createShader(GL_VERTEX_SHADER);
	m_gs_id = gl.createShader(m_glExtTokens.GEOMETRY_SHADER);
	m_po_id = gl.createProgram();

	GLU_EXPECT_NO_ERROR(gl.getError(), "Error creating program/shader objects.");

	/* If gs code is available set gs out data for transformfeedback*/
	if (m_test_data.m_gs_code)
	{
		const char* varyings[] = { "out_adjacent_geometry", "out_geometry" };

		gl.transformFeedbackVaryings(m_po_id, 2, varyings, GL_SEPARATE_ATTRIBS);
	}
	else
	{
		const char* varyings[] = { "gl_Position" };

		gl.transformFeedbackVaryings(m_po_id, 1, varyings, GL_SEPARATE_ATTRIBS);
	}
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error configuring vertex array object!");

	/* Build program */
	if (!buildProgram(m_po_id, m_fs_id, 1, /* parts */ &fsCode, (gsCode) ? m_gs_id : 0, (gsCode) ? 1 : 0,
					  (gsCode) ? &gsCode : 0, m_vs_id, 1, /* parts */ &vsCode))
	{
		TCU_FAIL("Could not create a program object from a valid shader!");
	}

	/* Generate buffers for input/output vertex data */
	gl.genBuffers(1, &m_vertex_data_bo_id);
	gl.genBuffers(1, &m_adjacency_geometry_bo_id);
	gl.genBuffers(1, &m_geometry_bo_id);

	/* Configure buffers for input/output vertex data */
	gl.bindBuffer(GL_ARRAY_BUFFER, m_adjacency_geometry_bo_id);
	gl.bufferData(GL_ARRAY_BUFFER, m_test_data.m_geometry_bo_size * 4, 0, GL_DYNAMIC_DRAW);
	gl.bindBuffer(GL_ARRAY_BUFFER, m_geometry_bo_id);
	gl.bufferData(GL_ARRAY_BUFFER, m_test_data.m_geometry_bo_size * 4, 0, GL_DYNAMIC_DRAW);
	gl.bindBuffer(GL_ARRAY_BUFFER, m_vertex_data_bo_id);
	gl.bufferData(GL_ARRAY_BUFFER, m_test_data.m_vertex_data_bo_size, m_test_data.m_vertex_data, GL_DYNAMIC_DRAW);
	gl.bindBuffer(GL_ARRAY_BUFFER, 0);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Error configuring vertex buffer objects for vertex data!");

	/* Configure buffer for index data */
	if (m_test_data.m_index_data_bo_size > 0)
	{
		gl.genBuffers(1, &m_index_data_bo_id);
		gl.bindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_index_data_bo_id);
		gl.bufferData(GL_ELEMENT_ARRAY_BUFFER, m_test_data.m_index_data_bo_size, m_test_data.m_index_data,
					  GL_DYNAMIC_DRAW);
		gl.bindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

		GLU_EXPECT_NO_ERROR(gl.getError(), "Error configuring vertex buffer objects for index data!");
	}
}

/** Executes the test.
 *  Sets the test result to QP_TEST_RESULT_FAIL if the test failed, QP_TEST_RESULT_PASS otherwise.
 *  @return STOP if the test has finished, CONTINUE to indicate iterate should be called once again.
 *  Note the function throws exception should an error occur!
 **/
tcu::TestNode::IterateResult GeometryShaderAdjacency::iterate(void)
{
	initTest();

	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/** Bind a vertex array object */
	gl.bindVertexArray(m_vao_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error binding vertex array object!");

	/* Bind buffer objects used as data store for transform feedback to TF binding points*/
	if (m_test_data.m_gs_code)
	{
		gl.bindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, m_adjacency_geometry_bo_id);
		gl.bindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 1, m_geometry_bo_id);
	}
	else
	{
		gl.bindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, m_geometry_bo_id);
	}

	GLU_EXPECT_NO_ERROR(gl.getError(), "Error configuring transform feedback buffer binding points!");

	gl.bindBuffer(GL_ARRAY_BUFFER, m_vertex_data_bo_id);
	m_position_attribute_location = gl.getAttribLocation(m_po_id, "position_data");
	gl.vertexAttribPointer(m_position_attribute_location, m_components_input, GL_FLOAT, GL_FALSE, 0, 0);
	gl.enableVertexAttribArray(m_position_attribute_location);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error setting vertex attribute array for position_data attribute");

	/* bind index buffer */
	if (m_test_data.m_index_data_bo_size > 0)
	{
		gl.bindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_index_data_bo_id);
	}
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error binding index data buffer");

	/* Configure program */
	gl.enable(GL_RASTERIZER_DISCARD);
	gl.useProgram(m_po_id);
	gl.beginTransformFeedback(m_test_data.m_tf_mode);

	glw::GLuint nVertices = m_test_data.m_n_vertices * ((m_test_data.m_mode == m_glExtTokens.LINE_STRIP_ADJACENCY ||
														 m_test_data.m_mode == m_glExtTokens.TRIANGLE_STRIP_ADJACENCY) ?
															1 :
															2 /* include adjacency info */);

	/* Use glDrawElements if data is indicied */
	if (m_test_data.m_index_data_bo_size > 0)
	{
		gl.drawElements(m_test_data.m_mode, nVertices, GL_UNSIGNED_INT, 0);
	}
	/* Use glDrawArrays if data is non indicied */
	else
	{
		gl.drawArrays(m_test_data.m_mode, 0, nVertices);
	}
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error while trying to render");

	gl.disable(GL_RASTERIZER_DISCARD);
	gl.endTransformFeedback();

	/* Map result buffer objects into client space */
	float* result_adjacency_geometry_ptr = 0;
	float* result_geometry_ptr			 = 0;

	/* If gs is available read adjacency data using TF and compare with expected data*/
	if (m_test_data.m_gs_code)
	{
		gl.bindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, m_adjacency_geometry_bo_id);
		result_adjacency_geometry_ptr =
			(float*)gl.mapBufferRange(GL_TRANSFORM_FEEDBACK_BUFFER, 0, m_test_data.m_geometry_bo_size, GL_MAP_READ_BIT);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Error when mapping data to client space");

		std::stringstream sstreamExpected;
		std::stringstream sstreamResult;
		sstreamExpected << "[";
		sstreamResult << "[";

		for (unsigned int n = 0; n < m_test_data.m_geometry_bo_size / sizeof(float); ++n)
		{
			sstreamExpected << m_test_data.m_expected_adjacency_geometry[n] << ", ";
			sstreamResult << result_adjacency_geometry_ptr[n] << ", ";

			if (de::abs(result_adjacency_geometry_ptr[n] - m_test_data.m_expected_adjacency_geometry[n]) >= m_epsilon)
			{
				gl.unmapBuffer(GL_TRANSFORM_FEEDBACK_BUFFER);

				m_testCtx.getLog() << tcu::TestLog::Message << "At [" << n
								   << "] position adjacency buffer position Reference value is different than the "
									  "rendered data (epsilon "
								   << m_epsilon << " )"
								   << " (" << m_test_data.m_expected_adjacency_geometry[n] << ") vs "
								   << "(" << result_adjacency_geometry_ptr[n] << ")" << tcu::TestLog::EndMessage;

				m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
				return STOP;
			}
		}

		sstreamExpected << "]";
		sstreamResult << "]";
		m_testCtx.getLog() << tcu::TestLog::Message << "Adjacency Expected: " << sstreamExpected.str().c_str()
						   << tcu::TestLog::EndMessage;
		m_testCtx.getLog() << tcu::TestLog::Message << "Adjacency Result:  " << sstreamResult.str().c_str()
						   << tcu::TestLog::EndMessage;

		gl.unmapBuffer(GL_TRANSFORM_FEEDBACK_BUFFER);
	}

	/* Read vertex data using TF and compare with expected data*/
	gl.bindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, m_geometry_bo_id);
	result_geometry_ptr =
		(float*)gl.mapBufferRange(GL_TRANSFORM_FEEDBACK_BUFFER, 0, m_test_data.m_geometry_bo_size, GL_MAP_READ_BIT);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error when mapping data to client space");

	std::stringstream sstreamExpected;
	std::stringstream sstreamResult;
	sstreamExpected << "[";
	sstreamResult << "[";

	for (unsigned int n = 0; n < m_test_data.m_geometry_bo_size / sizeof(float); ++n)
	{
		sstreamExpected << m_test_data.m_expected_geometry[n] << ", ";
		sstreamResult << result_geometry_ptr[n] << ", ";

		if (de::abs(result_geometry_ptr[n] - m_test_data.m_expected_geometry[n]) >= m_epsilon)
		{
			gl.unmapBuffer(GL_TRANSFORM_FEEDBACK_BUFFER);

			m_testCtx.getLog()
				<< tcu::TestLog::Message << "At [" << n
				<< "] position geometry buffer position Reference value is different than the rendered data (epsilon "
				<< m_epsilon << " )"
				<< " (" << m_test_data.m_expected_geometry[n] << ") vs "
				<< "(" << result_geometry_ptr[n] << ")" << tcu::TestLog::EndMessage;

			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
			return STOP;
		}
	}

	sstreamExpected << "]";
	sstreamResult << "]";
	m_testCtx.getLog() << tcu::TestLog::Message << "Expected: " << sstreamExpected.str().c_str()
					   << tcu::TestLog::EndMessage;
	m_testCtx.getLog() << tcu::TestLog::Message << "Result:  " << sstreamResult.str().c_str()
					   << tcu::TestLog::EndMessage;

	gl.unmapBuffer(GL_TRANSFORM_FEEDBACK_BUFFER);

	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	return STOP;
}

} // namespace glcts
