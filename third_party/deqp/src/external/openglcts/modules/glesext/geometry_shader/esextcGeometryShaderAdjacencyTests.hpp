#ifndef _ESEXTCGEOMETRYSHADERADJACENCYTESTS_HPP
#define _ESEXTCGEOMETRYSHADERADJACENCYTESTS_HPP
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

#include "../esextcTestCaseBase.hpp"
#include "esextcGeometryShaderAdjacency.hpp"

namespace glcts
{
/** Test configuration of "Group 2" from CTS_EXT_geometry_shader. Description follows
 *
 * 1. Make sure that draw calls properly accept and pass the "lines with
 *    adjacency" data to a geometry shader. The geometry shader should then
 *    forward adjacent line segments into one transform feedback buffer object
 *    and actual line segments into another one.
 *
 *    Category: API;
 *              Functional Test
 *
 *    Create a program object and a fragment, geometry and vertex shader
 *    objects, as well as a buffer object and a transform feedback object.
 *
 *    The vertex shader object can be boilerplate.
 *
 *    The geometry shader should define:
 *
 *    * output vec4 variable called out_adjacent_geometry;
 *    * output vec4 variable called out_geometry;
 *
 *    It should take lines_adjacency data and emit two adjacent line segments
 *    to out_adjacent_geometry variable and one "actual" line segment to
 *    out_geometry. Since there will be two adjacent segments and only one
 *    "actual" line segment, the missing line segment should use (0, 0, 0, 0)
 *    coordinates for its start/end points.
 *
 *    The fragment shader object can have a boilerplate implementation.
 *
 *    Compile the shaders, attach them to the program object, link the program
 *    object.
 *
 *    Generate and bind a vertex array object.
 *
 *    Initialize a buffer object (A) to have enough space to hold information
 *    for 3 coordinates * 4  vertex locations * 2 (output primitive
 *    type-specific data topology). The data should describe 4 lines (including
 *    adjacency data) that - as a whole - make a rectangle from (-1, -1, 0) to
 *    (1, 1, 0). The buffer data should then be bound to an attribute, whose
 *    index corresponds to location of the input in variable. Enable the vertex
 *    attribute array for this index.
 *
 *    Initialize a buffer object (B) to have enough space to hold result
 *    adjacent data information.
 *
 *    Initialize a buffer object (C) to have enough space to hold result
 *    geometry information.
 *
 *    Configure transform feedback so that it captures aforementioned data
 *    written by geometry shader and stores:
 *
 *    * values stored into out_adjacency_geometry in buffer object B;
 *    * values stored into out_geometry in buffer object C;
 *
 *    Use separate transform feedback mode. Enable transform feedback. Enable
 *    rasterizer discard mode.
 *
 *    Finally, issue a draw call in GL_LINES_ADJACENCY_EXT mode for 8 points.
 *
 *    Map the buffer objects to client space. The test is assumed to have
 *    passed if the mapped buffer objects contain correct data.
 *
 *    NOTE: This test should verify both arrayed and indexed draw calls work
 *    correctly.
 *
 *
 * 2. Make sure that draw calls properly accept and pass the "lines with
 *    adjacency" data to a vertex shader without the adjacency information,
 *    assuming there is no geometry shader defined for the pipeline.
 *
 *    Category: API;
 *              Functional Test.
 *
 *    Create a program object and a fragment and vertex shader object, as well
 *    as a buffer object and a transform feedback object.
 *
 *    The fragment shader object can have a boilerplate implementation.
 *
 *    The vertex shader object should define a single input vec3 variable
 *    called in_vertex and a single output vec3 variable called out_vertex. It
 *    should write data from in_vertex in an unchanged form to out_vertex.
 *
 *    Compile the shaders, attach them to the program object, link the program
 *    object.
 *
 *    Bind the vertex array object.
 *
 *    Initialize a buffer object to have enough space to hold information about
 *    3-component floating point-based 32 vertices. The data should represent 4
 *    lines (including adjacency data) that - as a whole - make a rectangle
 *    from (-1, -1, 0) to (1, 1, 0). The buffer data should then be bound to an
 *    attribute, whose index corresponds to location of the input in variable.
 *    Enable the vertex attribute array for this index..
 *
 *    Initialize another buffer object to have enough space to hold information
 *    about at most 32 vertices, each consisting of 3 floating-point components.
 *    Configure transform feed-back so that it captures data written by vertex
 *    shader output variable to this buffer object. Enable transform feedback.
 *    Enable rasterizer discard mode.
 *
 *    Finally, issue a draw call in GL_LINES_ADJACENCY_EXT mode for 8 points.
 *
 *    Map the buffer object to client space. The test is assumed to have passed
 *    if the result data corresponds to locations of 8 vertices making up the
 *    rectangle *without* the adjacency data.
 *
 *
 * 3. Make sure that draw calls properly accept and pass the "line strips with
 *    adjacency" data to a geometry shader, if there is one included in the
 *    pipeline.
 *
 *    Category: API;
 *              Functional Test.
 *
 *    Modify test case 2.1 accordingly to work on "line strips with adjacency"
 *    data instead of "lines with adjacency" data.
 *
 *
 * 4. Make sure that draw calls properly accept and pass the "line strips with
 *    adjacency" data to a vertex shader without the adjacency information,
 *    assuming there is no geometry shader defined for the pipeline.
 *
 *    Category: API;
 *              Functional Test.
 *
 *    Modify test case 2.2 accordingly to work on "line strips with adjacency"
 *    data instead of "lines with adjacency" data.
 *
 *
 * 5. Make sure that draw calls properly accept and pass the "triangles with
 *    adjacency" data to a geometry shader, if there is one included in the
 *    pipeline.
 *
 *    Category: API;
 *              Functional Test.
 *
 *    Modify test case 2.1 accordingly to work on "triangles with adjacency"
 *    data instead of "lines with adjacency" data.
 *
 *
 * 6. Make sure that draw calls properly accept and pass the "triangles with
 *    adjacency" data to a vertex shader without the adjacency information,
 *    assuming there is no geometry shader defined for the pipeline.
 *
 *    Category: API;
 *              Functional Test.
 *
 *    Modify test case 2.2 accordingly to work on "triangles with adjacency"
 *    data instead of "lines with adjacency" data
 *
 *
 * 7. Make sure that draw calls properly accept and pass the "triangle strips
 *    with adjacency" data to a geometry shader, if there is one included in
 *    the pipeline.
 *
 *    Category: API;
 *              Functional Test.
 *
 *    Modify test case 2.1 accordingly to work on "triangle strips with
 *    adjacency" data instead of "lines with adjacency" data.
 *
 *
 * 8. Make sure that draw calls properly accept and pass the "triangle strips
 *    with adjacency" data to a vertex shader without the adjacency information,
 *    assuming there is no geometry shader defined for the pipeline.
 *
 *    Category: API;
 *              Functional Test.
 *
 *    Modify test case 2.2 accordingly to work on "triangle strips with
 *    adjacency" data instead of "lines with adjacency" data.
 *
 **/
class GeometryShaderAdjacencyTests : public glcts::TestCaseGroupBase
{
public:
	/* Public methods */
	GeometryShaderAdjacencyTests(glcts::Context& context, const ExtParameters& extParams, const char* name,
								 const char* description);

	virtual ~GeometryShaderAdjacencyTests()
	{
	}

	virtual void init(void);
	virtual void deinit(void);

private:
	/* Private methods */
	void configureTestDataLines(AdjacencyTestData& test_data, bool withGS = false, bool indiced = false);
	void configureTestDataLineStrip(AdjacencyTestData& test_data, bool withGS = false, bool indiced = false);
	void configureTestDataTriangles(AdjacencyTestData& test_data, bool withGS = false, bool indiced = false);
	void configureTestDataTriangleStrip(AdjacencyTestData& test_data, bool withGS = false, bool indiced = false);

	void createGrid(AdjacencyTestData& test_data);
	void createGridLineSegments(AdjacencyTestData& test_data);
	void createGridLineStrip(AdjacencyTestData& test_data);
	void createGridTriangles(AdjacencyTestData& test_data);
	void createGridTriangleStrip(AdjacencyTestData& test_data);

	void setLinePointsNonindiced(AdjacencyTestData& test_data);
	void setLinePointsindiced(AdjacencyTestData& test_data);
	void setLineStripPointsNonindiced(AdjacencyTestData& test_data);
	void setLineStripPointsIndiced(AdjacencyTestData& test_data);
	void setTrianglePointsNonindiced(AdjacencyTestData& test_data);
	void setTrianglePointsIndiced(AdjacencyTestData& test_data);
	void setTriangleStripPointsIndiced(AdjacencyTestData& test_data);
	void setTriangleStripPointsNonindiced(AdjacencyTestData& test_data);

	/* Private constants */
	const unsigned int m_grid_granulity;
	const unsigned int m_n_components_input;
	const unsigned int m_n_components_output;
	const unsigned int m_n_line_segments;
	const unsigned int m_n_vertices_per_triangle;

	/* Private variables */
	std::vector<AdjacencyTestData*> m_tests_data;
};

} // namespace glcts

#endif // _ESEXTCGEOMETRYSHADERADJACENCYTESTS_HPP
