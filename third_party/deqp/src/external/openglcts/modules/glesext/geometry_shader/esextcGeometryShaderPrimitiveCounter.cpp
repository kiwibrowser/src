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

#include "esextcGeometryShaderPrimitiveCounter.hpp"
#include "deMath.h"
#include "gluContextInfo.hpp"
#include "gluDefs.hpp"
#include "glwEnums.hpp"
#include "glwFunctions.hpp"
#include "tcuTestLog.hpp"
#include <string.h>

namespace glcts
{
/* Fragment shader code */
const char* GeometryShaderPrimitiveCounter::m_fragment_shader_code = "${VERSION}\n"
																	 "\n"
																	 "precision highp float;\n"
																	 "\n"
																	 "layout(location = 0) out vec4 color;\n"
																	 "\n"
																	 "flat in int test_gl_PrimitiveIDIn;\n"
																	 "\n"
																	 "void main()\n"
																	 "{\n"
																	 "    color = vec4(1, 1, 1, 1);\n"
																	 "}\n";

/* Vertex shader code */
const char* GeometryShaderPrimitiveCounter::m_vertex_shader_code = "${VERSION}\n"
																   "\n"
																   "precision highp float;\n"
																   "\n"
																   "in vec4 vertex_position;\n"
																   "\n"
																   "void main()\n"
																   "{\n"
																   "    gl_Position = vertex_position;\n"
																   "}\n";

/** Constructor
 *
 * @param context       Test context
 * @param name          Test group's name
 * @param description   Test group's desricption
 **/
GeometryShaderPrimitiveCounterTestGroup::GeometryShaderPrimitiveCounterTestGroup(Context&			  context,
																				 const ExtParameters& extParams,
																				 const char*		  name,
																				 const char*		  description)
	: TestCaseGroupBase(context, extParams, name, description)
{
	/* Left blank on purpose */
}

/** Initializes the test group creating test cases that this group consists of
 **/
void GeometryShaderPrimitiveCounterTestGroup::init(void)
{
	DataPrimitiveIDInCounter testConfiguration;

	/* Test case 6.1 */

	/* Points-to-Points Configuration */
	testConfiguration.m_drawMode						= GL_POINTS;
	testConfiguration.m_emitVertexCodeForGeometryShader = "        test_gl_PrimitiveIDIn = gl_PrimitiveIDIn;\n"
														  "        gl_Position           = gl_in[0].gl_Position;\n"
														  "        EmitVertex();\n";
	testConfiguration.m_feedbackMode						  = GL_POINTS;
	testConfiguration.m_layoutIn							  = "points";
	testConfiguration.m_layoutOut							  = "points";
	testConfiguration.m_numberOfDrawnPrimitives				  = 1024;
	testConfiguration.m_numberOfVerticesPerOneOutputPrimitive = 1;
	testConfiguration.m_numberOfVerticesPerOneInputPrimitive  = 1;

	addChild(new GeometryShaderPrimitiveCounter(getContext(), m_extParams, "point_to_point",
												"Test Group 6.1 points to points", testConfiguration));

	/* Points-to-Line Strip Configuration*/
	testConfiguration.m_drawMode						= GL_POINTS;
	testConfiguration.m_emitVertexCodeForGeometryShader = "        test_gl_PrimitiveIDIn = gl_PrimitiveIDIn;\n"
														  "        gl_Position           = gl_in[0].gl_Position;\n"
														  "        EmitVertex();\n"
														  "\n"
														  "        test_gl_PrimitiveIDIn = gl_PrimitiveIDIn;\n"
														  "        gl_Position           = gl_in[0].gl_Position;\n"
														  "        EmitVertex();\n";
	testConfiguration.m_feedbackMode						  = GL_LINES;
	testConfiguration.m_layoutIn							  = "points";
	testConfiguration.m_layoutOut							  = "line_strip";
	testConfiguration.m_numberOfDrawnPrimitives				  = 1024;
	testConfiguration.m_numberOfVerticesPerOneOutputPrimitive = 2;
	testConfiguration.m_numberOfVerticesPerOneInputPrimitive  = 1;

	addChild(new GeometryShaderPrimitiveCounter(getContext(), m_extParams, "points_to_line_strip",
												"Test Group 6.1 points to line strip", testConfiguration));

	/* Points-to-Triangle Strip Configuration */
	testConfiguration.m_drawMode						= GL_POINTS;
	testConfiguration.m_emitVertexCodeForGeometryShader = "        test_gl_PrimitiveIDIn = gl_PrimitiveIDIn;\n"
														  "        gl_Position           = gl_in[0].gl_Position;\n"
														  "        EmitVertex();\n"
														  "\n"
														  "        test_gl_PrimitiveIDIn = gl_PrimitiveIDIn;\n"
														  "        gl_Position           = gl_in[0].gl_Position;\n"
														  "        EmitVertex();\n"
														  "\n"
														  "        test_gl_PrimitiveIDIn = gl_PrimitiveIDIn;\n"
														  "        gl_Position           = gl_in[0].gl_Position;\n"
														  "        EmitVertex();\n";
	testConfiguration.m_feedbackMode						  = GL_TRIANGLES;
	testConfiguration.m_layoutIn							  = "points";
	testConfiguration.m_layoutOut							  = "triangle_strip";
	testConfiguration.m_numberOfDrawnPrimitives				  = 1024;
	testConfiguration.m_numberOfVerticesPerOneOutputPrimitive = 3;
	testConfiguration.m_numberOfVerticesPerOneInputPrimitive  = 1;

	addChild(new GeometryShaderPrimitiveCounter(getContext(), m_extParams, "points_to_triangle_strip",
												"Test Group 6.1 points to triangle strip", testConfiguration));

	/* Lines-to-Points Configuration */
	testConfiguration.m_drawMode						= GL_LINES;
	testConfiguration.m_emitVertexCodeForGeometryShader = "        test_gl_PrimitiveIDIn = gl_PrimitiveIDIn;\n"
														  "        gl_Position           = gl_in[0].gl_Position;\n"
														  "        EmitVertex();\n";
	testConfiguration.m_feedbackMode						  = GL_POINTS;
	testConfiguration.m_layoutIn							  = "lines";
	testConfiguration.m_layoutOut							  = "points";
	testConfiguration.m_numberOfDrawnPrimitives				  = 1024;
	testConfiguration.m_numberOfVerticesPerOneOutputPrimitive = 1;
	testConfiguration.m_numberOfVerticesPerOneInputPrimitive  = 2;

	addChild(new GeometryShaderPrimitiveCounter(getContext(), m_extParams, "lines_to_points",
												"Test Group 6.1 lines to points", testConfiguration));

	/* Lines-to-Line Strip Configuration */
	testConfiguration.m_drawMode						= GL_LINES;
	testConfiguration.m_emitVertexCodeForGeometryShader = "        test_gl_PrimitiveIDIn = gl_PrimitiveIDIn;\n"
														  "        gl_Position           = gl_in[0].gl_Position;\n"
														  "        EmitVertex();\n"
														  "\n"
														  "        test_gl_PrimitiveIDIn = gl_PrimitiveIDIn;\n"
														  "        gl_Position           = gl_in[1].gl_Position;\n"
														  "        EmitVertex();\n";
	testConfiguration.m_feedbackMode						  = GL_LINES;
	testConfiguration.m_layoutIn							  = "lines";
	testConfiguration.m_layoutOut							  = "line_strip";
	testConfiguration.m_numberOfVerticesPerOneOutputPrimitive = 2;
	testConfiguration.m_numberOfVerticesPerOneInputPrimitive  = 2;
	testConfiguration.m_numberOfDrawnPrimitives				  = 1024;

	addChild(new GeometryShaderPrimitiveCounter(getContext(), m_extParams, "lines_to_line_strip",
												"Test Group 6.1 lines to line strip", testConfiguration));

	/* Lines-to-Triangle Strip Configuration */
	testConfiguration.m_drawMode						= GL_LINES;
	testConfiguration.m_emitVertexCodeForGeometryShader = "        test_gl_PrimitiveIDIn = gl_PrimitiveIDIn;\n"
														  "        gl_Position           = gl_in[0].gl_Position;\n"
														  "        EmitVertex();\n"
														  "\n"
														  "        test_gl_PrimitiveIDIn = gl_PrimitiveIDIn;\n"
														  "        gl_Position           = gl_in[1].gl_Position;\n"
														  "        EmitVertex();\n"
														  "\n"
														  "        test_gl_PrimitiveIDIn = gl_PrimitiveIDIn;\n"
														  "        gl_Position           = gl_in[0].gl_Position;\n"
														  "        EmitVertex();\n";
	testConfiguration.m_feedbackMode						  = GL_TRIANGLES;
	testConfiguration.m_layoutIn							  = "lines";
	testConfiguration.m_layoutOut							  = "triangle_strip";
	testConfiguration.m_numberOfDrawnPrimitives				  = 1024;
	testConfiguration.m_numberOfVerticesPerOneOutputPrimitive = 3;
	testConfiguration.m_numberOfVerticesPerOneInputPrimitive  = 2;

	addChild(new GeometryShaderPrimitiveCounter(getContext(), m_extParams, "lines_to_triangle_strip",
												"Test Group 6.1 lines to triangle strip", testConfiguration));

	/* Triangles-to-Points Configuration */
	testConfiguration.m_drawMode						= GL_TRIANGLES;
	testConfiguration.m_emitVertexCodeForGeometryShader = "        test_gl_PrimitiveIDIn = gl_PrimitiveIDIn;\n"
														  "        gl_Position           = gl_in[0].gl_Position;\n"
														  "        EmitVertex();\n";
	testConfiguration.m_feedbackMode						  = GL_POINTS;
	testConfiguration.m_layoutIn							  = "triangles";
	testConfiguration.m_layoutOut							  = "points";
	testConfiguration.m_numberOfDrawnPrimitives				  = 1024;
	testConfiguration.m_numberOfVerticesPerOneOutputPrimitive = 1;
	testConfiguration.m_numberOfVerticesPerOneInputPrimitive  = 3;

	addChild(new GeometryShaderPrimitiveCounter(getContext(), m_extParams, "triangles_to_points",
												"Test Group 6.1 triangles to points", testConfiguration));

	/* Triangles-to-Line Strip Configuration */
	testConfiguration.m_drawMode						= GL_TRIANGLES;
	testConfiguration.m_emitVertexCodeForGeometryShader = "        test_gl_PrimitiveIDIn = gl_PrimitiveIDIn;\n"
														  "        gl_Position           = gl_in[0].gl_Position;\n"
														  "        EmitVertex();\n"
														  "\n"
														  "        test_gl_PrimitiveIDIn = gl_PrimitiveIDIn;\n"
														  "        gl_Position           = gl_in[1].gl_Position;\n"
														  "        EmitVertex();\n";
	testConfiguration.m_feedbackMode						  = GL_LINES;
	testConfiguration.m_layoutIn							  = "triangles";
	testConfiguration.m_layoutOut							  = "line_strip";
	testConfiguration.m_numberOfDrawnPrimitives				  = 1024;
	testConfiguration.m_numberOfVerticesPerOneOutputPrimitive = 2;
	testConfiguration.m_numberOfVerticesPerOneInputPrimitive  = 3;

	addChild(new GeometryShaderPrimitiveCounter(getContext(), m_extParams, "triangles_to_line_strip",
												"Test Group 6.1 triangles to line strip", testConfiguration));

	/* Triangles-to-Triangle Strip Configuration */
	testConfiguration.m_drawMode						= GL_TRIANGLES;
	testConfiguration.m_emitVertexCodeForGeometryShader = "        test_gl_PrimitiveIDIn=gl_PrimitiveIDIn;\n"
														  "        gl_Position = gl_in[0].gl_Position;\n"
														  "        EmitVertex();\n"
														  "\n"
														  "        test_gl_PrimitiveIDIn=gl_PrimitiveIDIn;\n"
														  "        gl_Position = gl_in[1].gl_Position;\n"
														  "        EmitVertex();\n"
														  "\n"
														  "        test_gl_PrimitiveIDIn=gl_PrimitiveIDIn;\n"
														  "        gl_Position = gl_in[2].gl_Position;\n"
														  "        EmitVertex();\n";
	testConfiguration.m_feedbackMode						  = GL_TRIANGLES;
	testConfiguration.m_layoutIn							  = "triangles";
	testConfiguration.m_layoutOut							  = "triangle_strip";
	testConfiguration.m_numberOfDrawnPrimitives				  = 1024;
	testConfiguration.m_numberOfVerticesPerOneOutputPrimitive = 3;
	testConfiguration.m_numberOfVerticesPerOneInputPrimitive  = 3;

	addChild(new GeometryShaderPrimitiveCounter(getContext(), m_extParams, "triangles_to_triangle_strip",
												"Test Group 6.1 triangles to triangle strip", testConfiguration));

	/* Test case 6.2 */

	/* Points-to-Points Configuration */
	testConfiguration.m_drawMode						= GL_POINTS;
	testConfiguration.m_emitVertexCodeForGeometryShader = "        test_gl_PrimitiveIDIn = gl_PrimitiveIDIn;\n"
														  "        gl_Position           = gl_in[0].gl_Position;\n"
														  "        EmitVertex();\n";
	testConfiguration.m_feedbackMode						  = GL_POINTS;
	testConfiguration.m_layoutIn							  = "points";
	testConfiguration.m_layoutOut							  = "points";
	testConfiguration.m_numberOfDrawnPrimitives				  = 1024;
	testConfiguration.m_numberOfVerticesPerOneOutputPrimitive = 1;
	testConfiguration.m_numberOfVerticesPerOneInputPrimitive  = 1;

	addChild(new GeometryShaderPrimitiveCounterRestartingPrimitive(
		getContext(), m_extParams, "points_to_points_rp", "Test Group 6.2 points to points", testConfiguration));

	/* Points-to-Line Strip Configuration */
	testConfiguration.m_drawMode						= GL_POINTS;
	testConfiguration.m_emitVertexCodeForGeometryShader = "        test_gl_PrimitiveIDIn = gl_PrimitiveIDIn;\n"
														  "        gl_Position           = gl_in[0].gl_Position;\n"
														  "        EmitVertex();\n"
														  "\n"
														  "        test_gl_PrimitiveIDIn = gl_PrimitiveIDIn;\n"
														  "        gl_Position           = gl_in[0].gl_Position;\n"
														  "        EmitVertex();\n";
	testConfiguration.m_feedbackMode						  = GL_LINES;
	testConfiguration.m_layoutIn							  = "points";
	testConfiguration.m_layoutOut							  = "line_strip";
	testConfiguration.m_numberOfDrawnPrimitives				  = 1024;
	testConfiguration.m_numberOfVerticesPerOneOutputPrimitive = 2;
	testConfiguration.m_numberOfVerticesPerOneInputPrimitive  = 1;

	addChild(new GeometryShaderPrimitiveCounterRestartingPrimitive(getContext(), m_extParams, "points_to_line_strip_rp",
																   "Test Group 6.2 point to line", testConfiguration));

	/* Points-to-Triangle Strip Configuration */
	testConfiguration.m_drawMode						= GL_POINTS;
	testConfiguration.m_emitVertexCodeForGeometryShader = "        test_gl_PrimitiveIDIn = gl_PrimitiveIDIn;\n"
														  "        gl_Position           = gl_in[0].gl_Position;\n"
														  "        EmitVertex();\n"
														  "\n"
														  "        test_gl_PrimitiveIDIn = gl_PrimitiveIDIn;\n"
														  "        gl_Position           = gl_in[0].gl_Position;\n"
														  "        EmitVertex();\n"
														  "\n"
														  "        test_gl_PrimitiveIDIn = gl_PrimitiveIDIn;\n"
														  "        gl_Position           = gl_in[0].gl_Position;\n"
														  "        EmitVertex();\n";
	testConfiguration.m_feedbackMode						  = GL_TRIANGLES;
	testConfiguration.m_layoutIn							  = "points";
	testConfiguration.m_layoutOut							  = "triangle_strip";
	testConfiguration.m_numberOfDrawnPrimitives				  = 1024;
	testConfiguration.m_numberOfVerticesPerOneOutputPrimitive = 3;
	testConfiguration.m_numberOfVerticesPerOneInputPrimitive  = 1;

	addChild(new GeometryShaderPrimitiveCounterRestartingPrimitive(
		getContext(), m_extParams, "points_to_triangle_strip_rp", "Test Group 6.2 points to triangle strip",
		testConfiguration));

	/* Lines-to-Points Configuration */
	testConfiguration.m_layoutIn						= "lines";
	testConfiguration.m_layoutOut						= "points";
	testConfiguration.m_emitVertexCodeForGeometryShader = "        test_gl_PrimitiveIDIn = gl_PrimitiveIDIn;\n"
														  "        gl_Position           = gl_in[0].gl_Position;\n"
														  "        EmitVertex();\n";
	testConfiguration.m_numberOfVerticesPerOneOutputPrimitive = 1;
	testConfiguration.m_numberOfVerticesPerOneInputPrimitive  = 2;
	testConfiguration.m_feedbackMode						  = GL_POINTS;
	testConfiguration.m_drawMode							  = GL_LINES;
	testConfiguration.m_numberOfDrawnPrimitives				  = 1024;

	addChild(new GeometryShaderPrimitiveCounterRestartingPrimitive(
		getContext(), m_extParams, "lines_to_points_rp", "Test Group 6.2 lines to points", testConfiguration));

	/* Lines to Line Strip Configuration*/
	testConfiguration.m_drawMode						= GL_LINES;
	testConfiguration.m_emitVertexCodeForGeometryShader = "        test_gl_PrimitiveIDIn = gl_PrimitiveIDIn;\n"
														  "        gl_Position           = gl_in[0].gl_Position;\n"
														  "        EmitVertex();\n"
														  "\n"
														  "        test_gl_PrimitiveIDIn = gl_PrimitiveIDIn;\n"
														  "        gl_Position           = gl_in[1].gl_Position;\n"
														  "        EmitVertex();\n";
	testConfiguration.m_feedbackMode						  = GL_LINES;
	testConfiguration.m_layoutIn							  = "lines";
	testConfiguration.m_layoutOut							  = "line_strip";
	testConfiguration.m_numberOfDrawnPrimitives				  = 1024;
	testConfiguration.m_numberOfVerticesPerOneOutputPrimitive = 2;
	testConfiguration.m_numberOfVerticesPerOneInputPrimitive  = 2;

	addChild(new GeometryShaderPrimitiveCounterRestartingPrimitive(
		getContext(), m_extParams, "lines_to_line_strip_rp", "Test Group 6.2 lines to line strip", testConfiguration));

	/* Lines to Triangle Strip Configuration */
	testConfiguration.m_drawMode						= GL_LINES;
	testConfiguration.m_feedbackMode					= GL_TRIANGLES;
	testConfiguration.m_emitVertexCodeForGeometryShader = "        test_gl_PrimitiveIDIn = gl_PrimitiveIDIn;\n"
														  "        gl_Position           = gl_in[0].gl_Position;\n"
														  "        EmitVertex();\n"
														  "\n"
														  "        test_gl_PrimitiveIDIn = gl_PrimitiveIDIn;\n"
														  "        gl_Position           = gl_in[1].gl_Position;\n"
														  "        EmitVertex();\n"
														  "\n"
														  "        test_gl_PrimitiveIDIn = gl_PrimitiveIDIn;\n"
														  "        gl_Position           = gl_in[0].gl_Position;\n"
														  "        EmitVertex();\n";
	testConfiguration.m_layoutIn							  = "lines";
	testConfiguration.m_layoutOut							  = "triangle_strip";
	testConfiguration.m_numberOfDrawnPrimitives				  = 1024;
	testConfiguration.m_numberOfVerticesPerOneOutputPrimitive = 3;
	testConfiguration.m_numberOfVerticesPerOneInputPrimitive  = 2;

	addChild(new GeometryShaderPrimitiveCounterRestartingPrimitive(
		getContext(), m_extParams, "lines_to_triangle_strip_rp", "Test Group 6.2 line to triangle", testConfiguration));

	/* Triangles to Points Configuration */
	testConfiguration.m_drawMode						= GL_TRIANGLES;
	testConfiguration.m_emitVertexCodeForGeometryShader = "        test_gl_PrimitiveIDIn = gl_PrimitiveIDIn;\n"
														  "        gl_Position           = gl_in[0].gl_Position;\n"
														  "        EmitVertex();\n";
	testConfiguration.m_feedbackMode						  = GL_POINTS;
	testConfiguration.m_layoutIn							  = "triangles";
	testConfiguration.m_layoutOut							  = "points";
	testConfiguration.m_numberOfDrawnPrimitives				  = 1024;
	testConfiguration.m_numberOfVerticesPerOneOutputPrimitive = 1;
	testConfiguration.m_numberOfVerticesPerOneInputPrimitive  = 3;

	addChild(new GeometryShaderPrimitiveCounterRestartingPrimitive(
		getContext(), m_extParams, "triangles_to_points_rp", "Test Group 6.2 triangles to points", testConfiguration));

	/* Triangles to Line Strip Configuration */
	testConfiguration.m_drawMode						= GL_TRIANGLES;
	testConfiguration.m_emitVertexCodeForGeometryShader = "        test_gl_PrimitiveIDIn = gl_PrimitiveIDIn;\n"
														  "        gl_Position           = gl_in[0].gl_Position;\n"
														  "        EmitVertex();\n"
														  "\n"
														  "        test_gl_PrimitiveIDIn = gl_PrimitiveIDIn;\n"
														  "        gl_Position           = gl_in[1].gl_Position;\n"
														  "        EmitVertex();\n";
	testConfiguration.m_feedbackMode						  = GL_LINES;
	testConfiguration.m_layoutIn							  = "triangles";
	testConfiguration.m_layoutOut							  = "line_strip";
	testConfiguration.m_numberOfDrawnPrimitives				  = 1024;
	testConfiguration.m_numberOfVerticesPerOneOutputPrimitive = 2;
	testConfiguration.m_numberOfVerticesPerOneInputPrimitive  = 3;

	addChild(new GeometryShaderPrimitiveCounterRestartingPrimitive(
		getContext(), m_extParams, "triangles_to_line_strip_rp", "Test Group 6.2 triangles to line strip",
		testConfiguration));

	/* Triangles to Triangle Strip Configuration*/
	testConfiguration.m_drawMode						= GL_TRIANGLES;
	testConfiguration.m_emitVertexCodeForGeometryShader = "        test_gl_PrimitiveIDIn = gl_PrimitiveIDIn;\n"
														  "        gl_Position           = gl_in[0].gl_Position;\n"
														  "        EmitVertex();\n"
														  "\n"
														  "        test_gl_PrimitiveIDIn = gl_PrimitiveIDIn;\n"
														  "        gl_Position           = gl_in[1].gl_Position;\n"
														  "        EmitVertex();\n"
														  "\n"
														  "        test_gl_PrimitiveIDIn = gl_PrimitiveIDIn;\n"
														  "        gl_Position           = gl_in[2].gl_Position;\n"
														  "        EmitVertex();\n";
	testConfiguration.m_feedbackMode						  = GL_TRIANGLES;
	testConfiguration.m_layoutIn							  = "triangles";
	testConfiguration.m_layoutOut							  = "triangle_strip";
	testConfiguration.m_numberOfDrawnPrimitives				  = 1024;
	testConfiguration.m_numberOfVerticesPerOneOutputPrimitive = 3;
	testConfiguration.m_numberOfVerticesPerOneInputPrimitive  = 3;

	addChild(new GeometryShaderPrimitiveCounterRestartingPrimitive(
		getContext(), m_extParams, "triangles_to_triangle_strip_rp", "Test Group 6.2 triangles to triangle strip",
		testConfiguration));

	/* Test case 6.3 */
	addChild(new GeometryShaderPrimitiveIDFromFragmentShader(getContext(), m_extParams, "primitive_id_from_fragment",
															 "Test Group 6.3 gl_PrimitiveID from fragment shader"));
}

/** Constructor
 *
 * @param context               Test context
 * @param name                  Test case's name
 * @param description           Test case's description
 * @param testConfiguration     Configuration that specifies the "in" and "out" primitive types
 *                              for the geometry shader and the shader itself
 **/
GeometryShaderPrimitiveCounter::GeometryShaderPrimitiveCounter(Context& context, const ExtParameters& extParams,
															   const char* name, const char* description,
															   const DataPrimitiveIDInCounter& testConfiguration)
	: TestCaseBase(context, extParams, name, description)
	, m_testConfiguration(testConfiguration)
	, m_nrVaryings(0)
	, m_sizeOfDataArray(0)
	, m_sizeOfFeedbackBuffer(0)
	, m_n_components(4)
	, m_maxGeometryOutputVertices(0)
	, m_fragment_shader_id(0)
	, m_geometry_shader_id(0)
	, m_vertex_shader_id(0)
	, m_program_id(0)
	, m_tbo(0)
	, m_vao(0)
	, m_vbo(0)
{
	/* Left blank on purpose */
}

/** Verifies the data generated by XFB during a draw call.
 *
 *  @param data Input data. Must not be NULL.
 *
 *  @return true if successful, false otherwise.
 */
bool GeometryShaderPrimitiveCounter::checkResult(const glw::GLint* data)
{
	bool testPassed = true;

	for (unsigned int nDrawnPrimitive = 0; nDrawnPrimitive < m_testConfiguration.m_numberOfDrawnPrimitives;
		 nDrawnPrimitive++)
	{
		unsigned int nEmittedValuesPerPrimitive = m_maxGeometryOutputVertices * m_nrVaryings;

		for (unsigned int nValue = (nEmittedValuesPerPrimitive * (nDrawnPrimitive));
			 nValue < (nEmittedValuesPerPrimitive * (nDrawnPrimitive + 1)); nValue++)
		{
			if ((unsigned int)data[nValue] != nDrawnPrimitive)
			{
				m_testCtx.getLog() << tcu::TestLog::Message << "Rendered value at index "
								   << "[" << nValue << "/" << (m_testConfiguration.m_numberOfDrawnPrimitives *
															   m_maxGeometryOutputVertices * m_nrVaryings)
								   << "]: " << data[nValue] << " != " << nEmittedValuesPerPrimitive
								   << tcu::TestLog::EndMessage;

				testPassed = false;
			}
		} /* for (all values) */
	}	 /* for (all primitives) */

	return testPassed;
}

/** Creates a program object using predefined fragment & vertex shaders
 *   and a geometry shader that is constructed on-the-fly, according
 *   to testConfiguration parameters. The function also sets up transform
 *   feedback varyings, prior to linking.
 **/
void GeometryShaderPrimitiveCounter::createAndBuildProgramWithFeedback(void)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Create a program object */
	m_program_id = gl.createProgram();

	/* Specify transform feedback varyings */
	const char* feedbackVaryings[] = { "test_gl_PrimitiveIDIn" };

	m_nrVaryings = sizeof(feedbackVaryings) / sizeof(char*);

	gl.transformFeedbackVaryings(m_program_id, m_nrVaryings, feedbackVaryings, GL_INTERLEAVED_ATTRIBS);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not set transform feedback varyings!");

	/* Create shader objects that will make up the program object */
	m_geometry_shader_id = gl.createShader(m_glExtTokens.GEOMETRY_SHADER);
	m_fragment_shader_id = gl.createShader(GL_FRAGMENT_SHADER);
	m_vertex_shader_id   = gl.createShader(GL_VERTEX_SHADER);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateShader() call(s) failed");

	/* Retrieve GL_MAX_GEOMETRY_OUTPUT_VERTICES_EXT pname value and use it as argument for
	 * max_vertices layout qualifier of the geometry shader. */
	std::stringstream max_vertices_sstream;
	std::string		  max_vertices_string;

	gl.getIntegerv(m_glExtTokens.MAX_GEOMETRY_OUTPUT_VERTICES, &m_maxGeometryOutputVertices);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not query GL_MAX_GEOMETRY_OUTPUT_VERTICES_EXT!");

	m_maxGeometryOutputVertices = m_maxGeometryOutputVertices / (m_n_components + m_nrVaryings);

	max_vertices_sstream << m_maxGeometryOutputVertices;
	max_vertices_string = max_vertices_sstream.str();

	/* Calculate the possible for loops count in Geometry Shader */
	glw::GLint n_loop_iterations =
		m_maxGeometryOutputVertices / m_testConfiguration.m_numberOfVerticesPerOneOutputPrimitive;

	/* adjust the m_maxGeometryOutputVertices to be the number of emitted vertices  */
	m_maxGeometryOutputVertices = n_loop_iterations * m_testConfiguration.m_numberOfVerticesPerOneOutputPrimitive;

	std::stringstream n_loop_iterations_sstream;
	std::string		  n_loop_iterations_string;

	n_loop_iterations_sstream << n_loop_iterations;
	n_loop_iterations_string = n_loop_iterations_sstream.str();

	/* Construct geometry shader code */
	std::string geometry_shader_code =
		GetGeometryShaderCode(max_vertices_string, m_testConfiguration.m_layoutIn, m_testConfiguration.m_layoutOut,
							  m_testConfiguration.m_emitVertexCodeForGeometryShader, n_loop_iterations_string);

	const char* geometry_shader_code_ptr = (const char*)geometry_shader_code.c_str();

	/* Build program */
	if (!buildProgram(m_program_id, m_fragment_shader_id, 1, &m_fragment_shader_code, m_geometry_shader_id, 1,
					  &geometry_shader_code_ptr, m_vertex_shader_id, 1, &m_vertex_shader_code))
	{
		TCU_FAIL("Program could not have been created sucessfully");
	}
}

/** Deinitializes GLES objects created during the test.
 *
 */
void GeometryShaderPrimitiveCounter::deinit(void)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Reset OpenGL ES state */
	gl.useProgram(0);
	gl.bindBuffer(GL_ARRAY_BUFFER, 0);
	gl.bindVertexArray(0);

	/* Delete program object and shader objects */
	if (m_program_id != 0)
	{
		gl.deleteProgram(m_program_id);
	}

	if (m_vertex_shader_id != 0)
	{
		gl.deleteShader(m_vertex_shader_id);
	}

	if (m_geometry_shader_id != 0)
	{
		gl.deleteShader(m_geometry_shader_id);
	}

	if (m_fragment_shader_id != 0)
	{
		gl.deleteShader(m_fragment_shader_id);
	}

	if (m_vbo != 0)
	{
		gl.deleteBuffers(1, &m_vbo);
	}

	if (m_tbo != 0)
	{
		gl.deleteBuffers(1, &m_tbo);
	}

	if (m_vao != 0)
	{
		gl.deleteVertexArrays(1, &m_vao);
	}

	/* Deinitialize base class */
	TestCaseBase::deinit();
}

/** Render a test geometry and retrieve data generated by XFB.
 *
 *  @param feedbackResult Will be filled with data generated by XFB.
 *                        Must provide m_sizeOfFeedbackBuffer bytes of space.
 *                        Must not be NULL.
 */
void GeometryShaderPrimitiveCounter::drawAndGetFeedback(glw::GLint* feedbackResult)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.useProgram(m_program_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram() failed!");

	gl.enable(GL_RASTERIZER_DISCARD);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glEnable(GL_RASTERIZER_DISCARD) failed!");

	/* Perform transform feedback */
	gl.beginTransformFeedback(m_testConfiguration.m_feedbackMode);
	{
		drawFunction();
	}
	gl.endTransformFeedback();

	GLU_EXPECT_NO_ERROR(gl.getError(), "Rendering failed");

	gl.disable(GL_RASTERIZER_DISCARD);
	GLU_EXPECT_NO_ERROR(gl.getError(), "giDisable(GL_RASTERIZER_DISCARD) failed");

	/* Fetch the generated data */
	glw::GLint* feedback = NULL;

	feedback = (glw::GLint*)gl.mapBufferRange(GL_TRANSFORM_FEEDBACK_BUFFER, 0, /* offset */
											  m_sizeOfFeedbackBuffer, GL_MAP_READ_BIT);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glMapBufferRange() call failed.");

	memcpy(feedbackResult, feedback, m_sizeOfFeedbackBuffer);

	gl.unmapBuffer(GL_TRANSFORM_FEEDBACK_BUFFER);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUnmapBuffer() call failed");
}

/**  Renders the test geometry.
 *
 */
void GeometryShaderPrimitiveCounter::drawFunction()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.drawArrays(m_testConfiguration.m_drawMode, 0, /* first */
				  m_testConfiguration.m_numberOfDrawnPrimitives *
					  m_testConfiguration.m_numberOfVerticesPerOneInputPrimitive);
}

/** Fill geometry shader template
 *
 *  @param max_vertices  String in "max_vertices=N" format (where N is a valid value);
 *  @param layout_in     String storing an "in" layout qualifier definition;
 *  @param layout_out    String storing an "out" layout qualifier definition;
 *  @param emit_vertices String storing the vertex emitting code;
 *  @param n_iterations  String storing a number of vertex emitting iterations;
 */
std::string GeometryShaderPrimitiveCounter::GetGeometryShaderCode(const std::string& max_vertices,
																  const std::string& layout_in,
																  const std::string& layout_out,
																  const std::string& emit_vertices,
																  const std::string& n_iterations)
{
	/* Geometry shader template code */
	std::string m_geometry_shader_template = "${VERSION}\n"
											 "\n"
											 "${GEOMETRY_SHADER_REQUIRE}\n"
											 "\n"
											 "precision highp float;\n"
											 "\n"
											 "layout(<-LAYOUT-IN->)                                  in;\n"
											 "layout(<-LAYOUT-OUT->, max_vertices =<-MAX-VERTICES->) out;\n"
											 "\n"
											 "flat out int test_gl_PrimitiveIDIn;\n"
											 "\n"
											 "void main()\n"
											 "{\n"
											 "    for (int i=0; i<<-N_ITERATIONS->; i++)\n"
											 "    {\n"
											 "        <-EMIT-VERTICES->\n"
											 "        EndPrimitive();\n"
											 "    }\n"
											 "}\n";

	/* Max number of emited vertices */
	std::string template_name	 = "<-MAX-VERTICES->";
	std::size_t template_position = m_geometry_shader_template.find(template_name);

	while (template_position != std::string::npos)
	{
		m_geometry_shader_template =
			m_geometry_shader_template.replace(template_position, template_name.length(), max_vertices);

		template_position = m_geometry_shader_template.find(template_name);
	}

	/* In primitive type */
	template_name	 = "<-LAYOUT-IN->";
	template_position = m_geometry_shader_template.find(template_name);

	while (template_position != std::string::npos)
	{
		m_geometry_shader_template =
			m_geometry_shader_template.replace(template_position, template_name.length(), layout_in);

		template_position = m_geometry_shader_template.find(template_name);
	}

	/* Out primitive type */
	template_name	 = "<-LAYOUT-OUT->";
	template_position = m_geometry_shader_template.find(template_name);

	while (template_position != std::string::npos)
	{
		m_geometry_shader_template =
			m_geometry_shader_template.replace(template_position, template_name.length(), layout_out);

		template_position = m_geometry_shader_template.find(template_name);
	}

	/* Vertex emit code */
	template_name	 = "<-EMIT-VERTICES->";
	template_position = m_geometry_shader_template.find(template_name);

	while (template_position != std::string::npos)
	{
		m_geometry_shader_template =
			m_geometry_shader_template.replace(template_position, template_name.length(), emit_vertices);

		template_position = m_geometry_shader_template.find(template_name);
	}

	/* Number of iterations, during which we'll be emitting vertices */
	template_name	 = "<-N_ITERATIONS->";
	template_position = m_geometry_shader_template.find(template_name);

	while (template_position != std::string::npos)
	{
		m_geometry_shader_template =
			m_geometry_shader_template.replace(template_position, template_name.length(), n_iterations);

		template_position = m_geometry_shader_template.find(template_name);
	}

	return m_geometry_shader_template;
}

/** Executes the test.
 *  Sets the test result to QP_TEST_RESULT_FAIL if the test failed, QP_TEST_RESULT_PASS otherwise.
 *  @return STOP if the test has finished, CONTINUE to indicate iterate should be called once again.
 *  Note the function throws exception should an error occur!
 **/
tcu::TestNode::IterateResult GeometryShaderPrimitiveCounter::iterate(void)
{
	if (!m_is_geometry_shader_extension_supported)
	{
		throw tcu::NotSupportedError(GEOMETRY_SHADER_EXTENSION_NOT_SUPPORTED, "", __FILE__, __LINE__);
	}

	/* Set up */
	glw::GLint* feedbackResult = NULL;

	createAndBuildProgramWithFeedback();
	prepareBufferObjects();

	feedbackResult = new glw::GLint[m_sizeOfFeedbackBuffer];

	/* Execute the test */
	drawAndGetFeedback(feedbackResult);

	/* Set the CTS test result, depending on the outcome */
	if (checkResult(feedbackResult))
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
	}

	if (feedbackResult != NULL)
	{
		delete[] feedbackResult;

		feedbackResult = DE_NULL;
	}

	return STOP;
}

/** Prepare buffer objects and initialize their storage. It will
 *   later be used as vertex attribute array data source, as well
 *   as transform feedback destination.
 **/
void GeometryShaderPrimitiveCounter::prepareBufferObjects()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Create a VAO */
	gl.genVertexArrays(1, &m_vao);
	gl.bindVertexArray(m_vao);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not create a VAO!");

	m_sizeOfDataArray = m_testConfiguration.m_numberOfDrawnPrimitives * m_n_components *
						m_testConfiguration.m_numberOfVerticesPerOneInputPrimitive;

	gl.genBuffers(1, &m_vbo);
	gl.bindBuffer(GL_ARRAY_BUFFER, m_vbo);
	gl.bufferData(GL_ARRAY_BUFFER, m_sizeOfDataArray * sizeof(glw::GLfloat), NULL, GL_STATIC_DRAW);

	GLU_EXPECT_NO_ERROR(gl.getError(),
						"Could not initialize a buffer object to hold data for vertex_position attribute!");

	/* Set up "vertex_position" vertex attribute array */
	glw::GLint vertex_position_attribute_location = -1;

	vertex_position_attribute_location = gl.getAttribLocation(m_program_id, "vertex_position");
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not retrieve vertex_position attribute location!");

	gl.vertexAttribPointer(vertex_position_attribute_location, 4, /* size */
						   GL_FLOAT, GL_FALSE,					  /* normalized */
						   0,									  /* stride */
						   0);									  /* pointer */
	GLU_EXPECT_NO_ERROR(gl.getError(), "glVertexAttribPointer() failed for vertex_position attribute!");

	gl.enableVertexAttribArray(vertex_position_attribute_location);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glEnableVertexAttribArray() failed for vertex_position attribute!");

	/* Set up buffer object to hold result transform feedback data */
	m_sizeOfFeedbackBuffer = static_cast<glw::GLuint>(m_testConfiguration.m_numberOfDrawnPrimitives *
													  m_maxGeometryOutputVertices * m_nrVaryings * sizeof(glw::GLint));

	gl.genBuffers(1, &m_tbo);
	gl.bindBuffer(GL_ARRAY_BUFFER, m_tbo);
	gl.bufferData(GL_ARRAY_BUFFER, m_sizeOfFeedbackBuffer, DE_NULL, GL_STATIC_COPY);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not set up a buffer object to hold transform feedback data!");

	gl.bindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0 /* index */, m_tbo);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not bind a buffer object to transform feedback binding point!");
}

/** Constructor
 *
 * @param context               Test context;
 * @param name                  Test case's name;
 * @param description           Test case's description;
 * @param testConfiguration     Configuration that specifies the "in" and "out" primitive type
 *                              for the geometry shader, as well as defines other miscellaneous
 *                              properties of the test the shader.
 **/
GeometryShaderPrimitiveCounterRestartingPrimitive::GeometryShaderPrimitiveCounterRestartingPrimitive(
	Context& context, const ExtParameters& extParams, const char* name, const char* description,
	const DataPrimitiveIDInCounter& testConfiguration)
	: GeometryShaderPrimitiveCounter(context, extParams, name, description, testConfiguration)
	, m_bo_id(0)
	, m_numberOfRestarts(2)
{
}

/** Deinitializes GLES objects created during the test.
 *
 */
void GeometryShaderPrimitiveCounterRestartingPrimitive::deinit(void)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (m_bo_id != 0)
	{
		gl.deleteBuffers(1, &m_bo_id);

		m_bo_id = 0;
	}

	/* Call base class' deinit() */
	GeometryShaderPrimitiveCounter::deinit();
}

/** Renders the test geometry.
 *
 */
void GeometryShaderPrimitiveCounterRestartingPrimitive::drawFunction()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.drawElements(m_testConfiguration.m_drawMode, (m_testConfiguration.m_numberOfDrawnPrimitives *
														 m_testConfiguration.m_numberOfVerticesPerOneInputPrimitive +
													 m_numberOfRestarts),
					GL_UNSIGNED_INT, 0 /* indices */);
}

/** Executes the test.
 *  Sets the test result to QP_TEST_RESULT_FAIL if the test failed, QP_TEST_RESULT_PASS otherwise.
 *  @return STOP if the test has finished, CONTINUE to indicate iterate should be called once again.
 *  Note the function throws exception should an error occur!
 **/
tcu::TestNode::IterateResult GeometryShaderPrimitiveCounterRestartingPrimitive::iterate(void)
{
	if (!m_is_geometry_shader_extension_supported)
	{
		throw tcu::NotSupportedError(GEOMETRY_SHADER_EXTENSION_NOT_SUPPORTED, "", __FILE__, __LINE__);
	}

	/* Set up */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	createAndBuildProgramWithFeedback();
	prepareBufferObjects();

	glw::GLenum restart_token;
	if (!glu::isContextTypeES(m_context.getRenderContext().getType()))
	{
		restart_token = GL_PRIMITIVE_RESTART;
	}
	else
	{
		restart_token = GL_PRIMITIVE_RESTART_FIXED_INDEX;
	}

	gl.enable(restart_token);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Enabling primitive restart failed");

	setUpVertexAttributeArrays();

	/* Render the test geometry */
	glw::GLint* feedbackResult = NULL;
	feedbackResult			   = new glw::GLint[m_sizeOfFeedbackBuffer];
	try
	{
		drawAndGetFeedback(feedbackResult);
	}
	catch (...)
	{
		delete[] feedbackResult;
		feedbackResult = NULL;
		throw;
	}

	gl.disable(restart_token);

	int error = gl.getError();
	if (error != GL_NO_ERROR)
	{
		delete[] feedbackResult;
		feedbackResult = NULL;
		GLU_EXPECT_NO_ERROR(error, "Disabling primitive restart failed");
	}

	/* Check the result and update the DEQP test result accordingly */
	if (checkResult(feedbackResult))
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
	}

	delete[] feedbackResult;
	feedbackResult = NULL;

	return STOP;
}

/** Prepare the element vertex attribute array data. Make sure to
 *  use primitive index functionality.
 */
void GeometryShaderPrimitiveCounterRestartingPrimitive::setUpVertexAttributeArrays()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Create a buffer object to hold the element data */
	glw::GLuint*	   indices = DE_NULL;
	const unsigned int nIndices =
		m_testConfiguration.m_numberOfDrawnPrimitives * m_testConfiguration.m_numberOfVerticesPerOneInputPrimitive +
		m_numberOfRestarts;
	unsigned int restartIndex;
	if (!glu::isContextTypeES(m_context.getRenderContext().getType()))
	{
		restartIndex = nIndices;
	}
	else
	{
		restartIndex = 0xFFFFFFFF;
	}

	gl.genBuffers(1, &m_bo_id);
	gl.bindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_bo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not generate a buffer object");

	/* Prepare the data we will later use for configuring buffer object storage contents */
	indices = new unsigned int[nIndices];
	memset(indices, 0, nIndices * sizeof(unsigned int));

	for (unsigned int restartNr = 0; restartNr < m_numberOfRestarts; ++restartNr)
	{
		for (unsigned int i = m_testConfiguration.m_numberOfVerticesPerOneInputPrimitive * restartNr + 1; i < nIndices;
			 i++)
		{
			if (i < m_testConfiguration.m_numberOfVerticesPerOneInputPrimitive * (restartNr + 1) + restartNr)
			{
				indices[i] = i - restartNr;
			}
			else if (i > m_testConfiguration.m_numberOfVerticesPerOneInputPrimitive * (restartNr + 1) + restartNr)
			{
				indices[i] = (i - 1 - restartNr);
			}
		}
		indices[m_testConfiguration.m_numberOfVerticesPerOneInputPrimitive * (restartNr + 1) + restartNr] =
			restartIndex;
	}

	/*
	 m_testCtx.getLog() << tcu::TestLog::Message << "Indices table [ " << tcu::TestLog::EndMessage;
	 for (unsigned int i = 0; i < nIndices; i++)
	 {
	 m_testCtx.getLog() << tcu::TestLog::Message << indices[i] << ", " << tcu::TestLog::EndMessage;
	 }
	 m_testCtx.getLog() << tcu::TestLog::Message << " ]" << tcu::TestLog::EndMessage;
	 */

	/* Set up buffer object data storage */
	gl.bufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(glw::GLuint) * nIndices, indices, GL_STATIC_DRAW);
	/* Free the buffer we no longer need */
	delete[] indices;
	indices = NULL;
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not create element array buffer object");

	if (!glu::isContextTypeES(m_context.getRenderContext().getType()))
	{
		/* Set up primitive restarting */
		gl.primitiveRestartIndex(restartIndex);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Could not set primitive restart index");
	}
}

/* Vertex shader code */
const char* GeometryShaderPrimitiveIDFromFragmentShader::m_vertex_shader_code = "${VERSION}\n"
																				"\n"
																				"precision highp float;\n"
																				"\n"
																				"flat out int vs_vertex_id;"
																				"\n"
																				"     in vec4 vertex_position;\n"
																				"\n"
																				"void main()\n"
																				"{\n"
																				"    vs_vertex_id = gl_VertexID;"
																				"}\n";

/* Geometry Shader code */
const char* GeometryShaderPrimitiveIDFromFragmentShader::m_geometry_shader_code =
	"${VERSION}\n"
	"\n"
	"${GEOMETRY_SHADER_REQUIRE}\n"
	"\n"
	"precision highp float;\n"
	"\n"
	"layout(points)                           in;\n"
	"layout(triangle_strip, max_vertices = 4) out;\n"
	"\n"
	"flat in int vs_vertex_id[1];\n"
	"\n"
	"void main()\n"
	"{\n"
	"    int column = vs_vertex_id[0] % 64;\n"
	"    int row    = vs_vertex_id[0] / 64;\n"
	"\n"
	"    gl_PrimitiveID = vs_vertex_id[0];\n"
	"    gl_Position    = vec4(-1.0 + float(column+1) / 32.0, -1.0 + float(row + 1) / 32.0, 0, 1);\n"
	"    EmitVertex();\n"
	"\n"
	"    gl_PrimitiveID = vs_vertex_id[0];\n"
	"    gl_Position    = vec4(-1.0 + float(column+1) / 32.0, -1.0 + float(row) / 32.0, 0, 1);\n"
	"    EmitVertex();\n"
	"\n"
	"    gl_PrimitiveID = vs_vertex_id[0];\n"
	"    gl_Position    = vec4(-1.0 + float(column)   / 32.0, -1.0 + float(row + 1) / 32.0, 0, 1);\n"
	"    EmitVertex();\n"
	"\n"
	"    gl_PrimitiveID = vs_vertex_id[0];\n"
	"    gl_Position    = vec4(-1.0 + float(column)/ 32.0, -1.0 + float(row)/ 32.0, 0, 1);\n"
	"    EmitVertex();\n"
	"\n"
	"    EndPrimitive();\n"
	"}\n";

/* Fragment shader code */
const char* GeometryShaderPrimitiveIDFromFragmentShader::m_fragment_shader_code =
	"${VERSION}\n"
	"\n"
	"${GEOMETRY_SHADER_REQUIRE}\n"
	"\n"
	"precision highp float;\n"
	"\n"
	"layout(location = 0) out vec4 result;\n"
	"\n"
	"void main()\n"
	"{\n"
	"    result.x =       float(gl_PrimitiveID % 64)          / 64.0f;\n"
	"    result.y = floor(float(gl_PrimitiveID)      / 64.0f) / 64.0f;\n"
	"    result.z =       float(gl_PrimitiveID)               / 4096.0f;\n"
	"    result.w = ((gl_PrimitiveID % 2) == 0) ? 1.0f : 0.0f;\n"
	"}\n";

/** Constructor
 *
 * @param context               Test context;
 * @param name                  Test case's name;
 * @param description           Test case's description;
 **/
GeometryShaderPrimitiveIDFromFragmentShader::GeometryShaderPrimitiveIDFromFragmentShader(Context&			  context,
																						 const ExtParameters& extParams,
																						 const char*		  name,
																						 const char* description)
	: TestCaseBase(context, extParams, name, description)
	, m_n_drawn_vertices(4096)
	, m_squareEdgeSize(16)
	, m_texture_height(1024)
	, m_texture_n_components(4)
	, m_texture_n_levels(1)
	, m_texture_width(1024)
	, m_fbo_id(0)
	, m_fragment_shader_id(0)
	, m_geometry_shader_id(0)
	, m_program_id(0)
	, m_texture_id(0)
	, m_vao_id(0)
	, m_vbo_id(0)
	, m_vertex_shader_id(0)
{
}

/** Deinitializes GLES objects created during the test.
 *
 */
void GeometryShaderPrimitiveIDFromFragmentShader::deinit(void)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Reset OpenGL ES state */
	gl.useProgram(0);
	gl.bindTexture(GL_TEXTURE_2D, 0);
	gl.bindFramebuffer(GL_FRAMEBUFFER, 0);
	gl.bindVertexArray(0);

	if (m_program_id != 0)
	{
		gl.deleteProgram(m_program_id);

		m_program_id = 0;
	}

	if (m_fragment_shader_id != 0)
	{
		gl.deleteShader(m_fragment_shader_id);

		m_fragment_shader_id = 0;
	}

	if (m_geometry_shader_id != 0)
	{
		gl.deleteShader(m_geometry_shader_id);

		m_geometry_shader_id = 0;
	}

	if (m_vertex_shader_id != 0)
	{
		gl.deleteShader(m_vertex_shader_id);

		m_vertex_shader_id = 0;
	}

	if (m_texture_id != 0)
	{
		gl.deleteTextures(1, &m_texture_id);

		m_texture_id = 0;
	}

	if (m_fbo_id != 0)
	{
		gl.deleteFramebuffers(1, &m_fbo_id);

		m_fbo_id = 0;
	}

	if (m_vao_id != 0)
	{
		gl.deleteVertexArrays(1, &m_vao_id);

		m_vao_id = 0;
	}

	if (m_vbo_id != 0)
	{
		gl.deleteBuffers(1, &m_vbo_id);

		m_vbo_id = 0;
	}

	/* Release base class */
	TestCaseBase::deinit();
}

/** Executes the test.
 *  Sets the test result to QP_TEST_RESULT_FAIL if the test failed, QP_TEST_RESULT_PASS otherwise.
 *  @return STOP if the test has finished, CONTINUE to indicate iterate should be called once again.
 *  Note the function throws exception should an error occur!
 **/
tcu::TestNode::IterateResult GeometryShaderPrimitiveIDFromFragmentShader::iterate(void)
{
	const float epsilon = 1.0f / 256.0f;

	if (!m_is_geometry_shader_extension_supported)
	{
		throw tcu::NotSupportedError(GEOMETRY_SHADER_EXTENSION_NOT_SUPPORTED, "", __FILE__, __LINE__);
	}

	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Create a program object */
	m_program_id = gl.createProgram();
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateProgram() failed");

	/* Create shader objects */
	m_vertex_shader_id   = gl.createShader(GL_VERTEX_SHADER);
	m_geometry_shader_id = gl.createShader(m_glExtTokens.GEOMETRY_SHADER);
	m_fragment_shader_id = gl.createShader(GL_FRAGMENT_SHADER);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateShader() call(s) failed");

	/* Build the program object */
	if (!buildProgram(m_program_id, m_fragment_shader_id, 1, &m_fragment_shader_code, m_geometry_shader_id, 1,
					  &m_geometry_shader_code, m_vertex_shader_id, 1, &m_vertex_shader_code))
	{
		TCU_FAIL("Program could not have been created sucessfully");
	}

	/* Create and configure a texture object we'll be rendering to */
	gl.genTextures(1, &m_texture_id);
	gl.bindTexture(GL_TEXTURE_2D, m_texture_id);
	gl.texStorage2D(GL_TEXTURE_2D, m_texture_n_levels, GL_RGBA8, m_texture_width, m_texture_height);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Error creating a texture object!");

	/* Create and configure the framebuffer object */
	gl.genFramebuffers(1, &m_fbo_id);
	gl.bindFramebuffer(GL_DRAW_FRAMEBUFFER, m_fbo_id);
	gl.framebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_texture_id, 0 /* level */);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Error configuring framebuffer object!");

	/* Create and bind a vertex array object */
	gl.genVertexArrays(1, &m_vao_id);
	gl.bindVertexArray(m_vao_id);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Error configuring a vertex array object");

	/* Set up a buffer object we will use to hold vertex_position attribute data */
	const glw::GLuint sizeOfDataArray =
		static_cast<glw::GLuint>(m_n_drawn_vertices * m_texture_n_components * sizeof(glw::GLfloat));

	gl.genBuffers(1, &m_vbo_id);
	gl.bindBuffer(GL_ARRAY_BUFFER, m_vbo_id);
	gl.bufferData(GL_ARRAY_BUFFER, sizeOfDataArray, DE_NULL, GL_STATIC_DRAW);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not set up a buffer object to hold vertex_position attribute data!");

	/* Set up the viewport */
	gl.viewport(0 /* x */, 0 /* y */, m_texture_width, m_texture_height);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glViewport() call failed");

	/* Clear the color buffer */
	gl.clearColor(1.0f, 1.0f, 1.0f, 1.0f);
	gl.clear(GL_COLOR_BUFFER_BIT);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not clear the color buffer!");

	/* Render the test geometry */
	gl.useProgram(m_program_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram() call failed");

	gl.drawArrays(GL_POINTS, 0 /* first */, m_n_drawn_vertices);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Rendering failed!");

	/* Read the rendered data */
	glw::GLubyte* resultBuffer = new glw::GLubyte[m_texture_width * m_texture_height * m_texture_n_components];

	gl.bindFramebuffer(GL_READ_FRAMEBUFFER, m_fbo_id);
	int error = gl.getError();
	if (error != GL_NO_ERROR)
	{
		delete[] resultBuffer;
		resultBuffer = NULL;
		GLU_EXPECT_NO_ERROR(error, "glBindFramebuffer() call failed");
	}

	gl.readPixels(0 /* x */, 0 /* y */, m_texture_width, m_texture_height, GL_RGBA, GL_UNSIGNED_BYTE, resultBuffer);

	error = gl.getError();
	if (error != GL_NO_ERROR)
	{
		delete[] resultBuffer;
		resultBuffer = NULL;
		GLU_EXPECT_NO_ERROR(error, "Reading pixels failed!");
	}

	/* Loop over all pixels and compare the rendered data with reference values */
	for (glw::GLuint y = 0; y < m_texture_height; ++y)
	{
		glw::GLubyte* data_row = resultBuffer + y * m_texture_width * m_texture_n_components;

		for (glw::GLuint x = 0; x < m_texture_width; ++x)
		{
			glw::GLuint   column	  = (x / m_squareEdgeSize);
			glw::GLubyte* data		  = data_row + x * m_texture_n_components;
			glw::GLuint   row		  = (y / m_squareEdgeSize);
			glw::GLuint   primitiveID = column + (row * (m_texture_width / m_squareEdgeSize));

			/* Calculate expected and rendered pixel color */
			float rendered_rgba[4] = { 0 };
			float expected_rgba[4];

			for (unsigned int n_channel = 0; n_channel < 4 /* RGBA */; ++n_channel)
			{
				rendered_rgba[n_channel] = float(data[n_channel]) / 255.0f;
			}

			expected_rgba[0] = (glw::GLfloat)(primitiveID % 64) / 64.0f;
			expected_rgba[1] = deFloatFloor((glw::GLfloat)(primitiveID) / 64.0f) / 64.0f;
			expected_rgba[2] = (glw::GLfloat)(primitiveID) / 4096.0f;
			expected_rgba[3] = (((primitiveID % 2) == 0) ? 1.0f : 0.0f);

			/* Compare the data */
			if (de::abs(rendered_rgba[0] - expected_rgba[0]) > epsilon ||
				de::abs(rendered_rgba[1] - expected_rgba[1]) > epsilon ||
				de::abs(rendered_rgba[2] - expected_rgba[2]) > epsilon ||
				de::abs(rendered_rgba[3] - expected_rgba[3]) > epsilon)
			{
				m_testCtx.getLog() << tcu::TestLog::Message << "Primitive ID: " << primitiveID << "Rendered data"
								   << "equal (" << rendered_rgba[0] << ", " << rendered_rgba[1] << ", "
								   << rendered_rgba[2] << ", " << rendered_rgba[3] << ") "
								   << "exceeds allowed epsilon when compared to expected data equal ("
								   << expected_rgba[0] << ", " << expected_rgba[1] << ", " << expected_rgba[2] << ", "
								   << expected_rgba[3] << ")." << tcu::TestLog::EndMessage;

				delete[] resultBuffer;
				resultBuffer = NULL;

				m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
				return STOP;
			} /* if (rendered data is invalid) */
		}	 /* for (all columns) */
	}		  /* for (all rows) */

	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

	delete[] resultBuffer;
	resultBuffer = NULL;

	return STOP;
}

} // namespace glcts
