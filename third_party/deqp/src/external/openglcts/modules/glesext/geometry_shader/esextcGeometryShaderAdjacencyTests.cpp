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

#include "glwEnums.inl"

#include "deMath.h"
#include "esextcGeometryShaderAdjacencyTests.hpp"
#include <cstring>

namespace glcts
{

/** Constructor
 *
 * @param context       Test context
 * @param name          Test case's name
 * @param description   Test case's description
 **/
GeometryShaderAdjacencyTests::GeometryShaderAdjacencyTests(Context& context, const ExtParameters& extParams,
														   const char* name, const char* description)
	: TestCaseGroupBase(context, extParams, name, description)
	, m_grid_granulity(1)
	, m_n_components_input(2)
	, m_n_components_output(4)
	, m_n_line_segments(4)
	, m_n_vertices_per_triangle(3)
{
	/* Nothing to be done here */
}

/** Deinitializes tests data
 *
 **/
void GeometryShaderAdjacencyTests::deinit(void)
{
	for (std::vector<AdjacencyTestData*>::iterator it = m_tests_data.begin(); it != m_tests_data.end(); ++it)
	{
		delete *it;
		*it = NULL;
	}

	m_tests_data.clear();

	/* Call base class' deinit() function. */
	glcts::TestCaseGroupBase::deinit();
}

/** Initializes tests data
 *
 **/
void GeometryShaderAdjacencyTests::init(void)
{
	/* Tests for GL_LINES_ADJACENCY_EXT */

	/* Test 2.1 Non indiced Data */
	m_tests_data.push_back(new AdjacencyTestData());
	configureTestDataLines(*m_tests_data.back(), true, false);
	addChild(new GeometryShaderAdjacency(getContext(), m_extParams, "adjacency_non_indiced_lines",
										 "Test 2.1 non indiced", *m_tests_data.back()));
	/* Test 2.1 indiced Data */
	m_tests_data.push_back(new AdjacencyTestData());
	configureTestDataLines(*m_tests_data.back(), true, true);
	addChild(new GeometryShaderAdjacency(getContext(), m_extParams, "adjacency_indiced_lines", "Test 2.1 indiced",
										 *m_tests_data.back()));

	/* Tests for GL_LINE_STRIP_ADJACENCY_EXT */

	/* Test 2.3 Non indiced Data */
	m_tests_data.push_back(new AdjacencyTestData());
	configureTestDataLineStrip(*m_tests_data.back(), true, false);
	addChild(new GeometryShaderAdjacency(getContext(), m_extParams, "adjacency_non_indiced_line_strip",
										 "Test 2.3 non indiced", *m_tests_data.back()));
	/* Test 2.3 indiced Data */
	m_tests_data.push_back(new AdjacencyTestData());
	configureTestDataLineStrip(*m_tests_data.back(), true, true);
	addChild(new GeometryShaderAdjacency(getContext(), m_extParams, "adjacency_indiced_line_strip", "Test 2.3 indiced",
										 *m_tests_data.back()));

	/* Tests for GL_TRIANGLES_ADJACENCY_EXT */

	/* Test 2.5 Non indiced Data */
	m_tests_data.push_back(new AdjacencyTestData());
	configureTestDataTriangles(*m_tests_data.back(), true, false);
	addChild(new GeometryShaderAdjacency(getContext(), m_extParams, "adjacency_non_indiced_triangles",
										 "Test 2.5 non indiced", *m_tests_data.back()));
	/* Test 2.5 indiced Data */
	m_tests_data.push_back(new AdjacencyTestData());
	configureTestDataTriangles(*m_tests_data.back(), true, true);
	addChild(new GeometryShaderAdjacency(getContext(), m_extParams, "adjacency_indiced_triangles", "Test 2.5 indiced",
										 *m_tests_data.back()));

	/* Tests for GL_TRIANGLE_STRIP_ADJACENCY_EXT */

	/* Test 2.7 Non indiced Data */
	m_tests_data.push_back(new AdjacencyTestData());
	configureTestDataTriangleStrip(*m_tests_data.back(), true, false);
	addChild(new GeometryShaderAdjacency(getContext(), m_extParams, "adjacency_non_indiced_triangle_strip",
										 "Test 2.7 non indiced", *m_tests_data.back()));
	/* Test 2.7 indiced Data */
	m_tests_data.push_back(new AdjacencyTestData());
	configureTestDataTriangleStrip(*m_tests_data.back(), true, true);
	addChild(new GeometryShaderAdjacency(getContext(), m_extParams, "adjacency_indiced_triangle_strip",
										 "Test 2.7 indiced", *m_tests_data.back()));
}

/** Configure Test Data for GL_LINES_ADJACENCY_EXT drawing mode
 *
 *  @param testData reference to AdjacencyTestData instance to be configured
 *                  accordingly;
 *  @param withGS   if true, geometry shader code will be attached to test data;
 *  @param indiced  if true, indices will be stored in testData.
 **/
void GeometryShaderAdjacencyTests::configureTestDataLines(AdjacencyTestData& test_data, bool withGS, bool indiced)
{
	static const char* gsCode = "${VERSION}\n"
								"\n"
								"${GEOMETRY_SHADER_REQUIRE}\n"
								"\n"
								"precision highp float;\n"
								"\n"
								"layout(lines_adjacency)            in;\n"
								"layout(line_strip, max_vertices=2) out;\n"
								"\n"
								"layout(location = 0) out vec4 out_adjacent_geometry;\n"
								"layout(location = 1) out vec4 out_geometry;\n"
								"\n"
								"void main()\n"
								"{\n"
								"    out_adjacent_geometry = gl_in[0].gl_Position;\n"
								"    out_geometry          = gl_in[1].gl_Position;\n"
								"    EmitVertex();\n"
								"    out_adjacent_geometry = gl_in[3].gl_Position;\n"
								"    out_geometry          = gl_in[2].gl_Position;\n"
								"    EmitVertex();\n"
								"    EndPrimitive();\n"
								"}\n";

	test_data.m_gs_code = (withGS) ? gsCode : 0;
	test_data.m_mode	= GL_LINES_ADJACENCY_EXT;
	test_data.m_tf_mode = GL_LINES;

	createGrid(test_data);

	if (indiced)
	{
		setLinePointsindiced(test_data);
	}
	else
	{
		setLinePointsNonindiced(test_data);
	}
}

/** Configure Test Data for GL_LINE_STRIP_ADJACENCY_EXT drawing mode
 *
 * @param testData reference to AdjacencyTestData instance to be configured
 *                  accordingly;
 * @param withGS   if true geometry shader code will be attached to test data;
 * @param indiced  if true indices will be stored in testData.
 **/
void GeometryShaderAdjacencyTests::configureTestDataLineStrip(AdjacencyTestData& test_data, bool withGS, bool indiced)
{
	static const char* gsCode = "${VERSION}\n"
								"\n"
								"${GEOMETRY_SHADER_REQUIRE}\n"
								"\n"
								"precision highp float;\n"
								"\n"
								"layout(lines_adjacency)            in;\n"
								"layout(line_strip, max_vertices=2) out;\n"
								"\n"
								"out vec4 out_adjacent_geometry;\n"
								"out vec4 out_geometry;\n"
								"\n"
								"void main()\n"
								"{\n"
								"    out_adjacent_geometry = gl_in[0].gl_Position;\n"
								"    out_geometry          = gl_in[1].gl_Position;\n"
								"    EmitVertex();\n"
								"    out_adjacent_geometry = gl_in[3].gl_Position;\n"
								"    out_geometry          = gl_in[2].gl_Position;\n"
								"    EmitVertex();\n"
								"    EndPrimitive();\n"
								"}\n";

	test_data.m_gs_code = (withGS) ? gsCode : 0;
	test_data.m_mode	= GL_LINE_STRIP_ADJACENCY_EXT;
	test_data.m_tf_mode = GL_LINES;

	createGrid(test_data);

	if (indiced)
	{
		setLineStripPointsIndiced(test_data);
	}
	else
	{
		setLineStripPointsNonindiced(test_data);
	}
}

/** Configure Test Data for GL_TRIANGLES_ADJACENCY_EXT drawing mode
 *
 * @param testData reference to AdjacencyTestData instance to be configured
 *                  accordingly;
 * @param withGS   if true geometry shader code will be attached to test data;
 * @param indiced  if true indices will be stored in testData.
 **/
void GeometryShaderAdjacencyTests::configureTestDataTriangles(AdjacencyTestData& test_data, bool withGS, bool indiced)
{
	static const char* gsCode = "${VERSION}\n"
								"\n"
								"${GEOMETRY_SHADER_REQUIRE}\n"
								"\n"
								"precision highp float;\n"
								"\n"
								"layout(triangles_adjacency)            in;\n"
								"layout(triangle_strip, max_vertices=3) out;\n"
								"\n"
								"out vec4 out_adjacent_geometry;\n"
								"out vec4 out_geometry;\n"
								"\n"
								"void main()\n"
								"{\n"
								"    out_adjacent_geometry = gl_in[1].gl_Position;\n"
								"    out_geometry          = gl_in[0].gl_Position;\n"
								"    EmitVertex();\n"
								"    out_adjacent_geometry = gl_in[3].gl_Position;\n"
								"    out_geometry          = gl_in[2].gl_Position;\n"
								"    EmitVertex();\n"
								"    out_adjacent_geometry = gl_in[5].gl_Position;\n"
								"    out_geometry          = gl_in[4].gl_Position;\n"
								"    EmitVertex();\n"
								"    EndPrimitive();\n"
								"}\n";

	test_data.m_gs_code = (withGS) ? gsCode : 0;
	test_data.m_mode	= GL_TRIANGLES_ADJACENCY_EXT;
	test_data.m_tf_mode = GL_TRIANGLES;

	createGrid(test_data);

	if (indiced)
	{
		setTrianglePointsIndiced(test_data);
	}
	else
	{
		setTrianglePointsNonindiced(test_data);
	}
}

/** Configure Test Data for GL_TRIANGLE_STRIP_ADJACENCY_EXT drawing mode
 *
 * @param testData reference to AdjacencyTestData instance to be configured
 *                  accordingly;
 * @param withGS   if true geometry shader code will be attached to test data;
 * @param indiced  if true indices will be stored in test_data.
 **/
void GeometryShaderAdjacencyTests::configureTestDataTriangleStrip(AdjacencyTestData& test_data, bool withGS,
																  bool indiced)
{
	static const char* gsCode = "${VERSION}\n"
								"\n"
								"${GEOMETRY_SHADER_REQUIRE}\n"
								"\n"
								"precision highp float;\n"
								"\n"
								"layout(triangles_adjacency)            in;\n"
								"layout(triangle_strip, max_vertices=3) out;\n"
								"\n"
								"out vec4 out_adjacent_geometry;\n"
								"out vec4 out_geometry;\n"
								"\n"
								"void main()\n"
								"{\n"
								"    out_adjacent_geometry = gl_in[1].gl_Position;\n"
								"    out_geometry          = gl_in[0].gl_Position;\n"
								"    EmitVertex();\n"
								"    out_adjacent_geometry = gl_in[3].gl_Position;\n"
								"    out_geometry          = gl_in[2].gl_Position;\n"
								"    EmitVertex();\n"
								"    out_adjacent_geometry = gl_in[5].gl_Position;\n"
								"    out_geometry          = gl_in[4].gl_Position;\n"
								"    EmitVertex();\n"
								"    EndPrimitive();\n"
								"}\n";

	test_data.m_gs_code = (withGS) ? gsCode : 0;
	test_data.m_mode	= GL_TRIANGLE_STRIP_ADJACENCY_EXT;
	test_data.m_tf_mode = GL_TRIANGLES;

	createGrid(test_data);

	if (indiced)
	{
		setTriangleStripPointsIndiced(test_data);
	}
	else
	{
		setTriangleStripPointsNonindiced(test_data);
	}
}

/** Create vertex grid for the test instance.
 *
 * @param test_data AdjacencyTestData instance to be filled with grid data.
 */
void GeometryShaderAdjacencyTests::createGrid(AdjacencyTestData& test_data)
{
	/* Create a grid object */
	test_data.m_grid = new AdjacencyGrid;

	/* Allocate space for grid elements */
	test_data.m_grid->m_n_points = (m_grid_granulity + 1) * (m_grid_granulity + 1);
	test_data.m_grid->m_line_segments =
		new AdjacencyGridLineSegment[m_n_line_segments * m_grid_granulity * m_grid_granulity];
	test_data.m_grid->m_points = new AdjacencyGridPoint[test_data.m_grid->m_n_points];

	/* Generate points */
	float		 dx		= 1.0f / static_cast<float>(m_grid_granulity);
	float		 dy		= 1.0f / static_cast<float>(m_grid_granulity);
	unsigned int nPoint = 0;

	for (unsigned int y = 0; y < m_grid_granulity + 1; ++y)
	{
		for (unsigned int x = 0; x < m_grid_granulity + 1; ++x, ++nPoint)
		{
			test_data.m_grid->m_points[nPoint].index = nPoint;
			test_data.m_grid->m_points[nPoint].x	 = dx * static_cast<float>(x);
			test_data.m_grid->m_points[nPoint].y	 = dy * static_cast<float>(y);
		}
	}

	switch (test_data.m_mode)
	{
	case GL_LINES_ADJACENCY_EXT:
	{
		/* Generate line segment data*/
		createGridLineSegments(test_data);

		break;
	}

	case GL_LINE_STRIP_ADJACENCY_EXT:
	{
		/* Line strip data generation requires line segment data to be present */
		createGridLineSegments(test_data);

		/* Generate line strip data */
		createGridLineStrip(test_data);

		break;
	}

	case GL_TRIANGLES_ADJACENCY_EXT:
	{
		/* Generate triangles data */
		createGridTriangles(test_data);

		break;
	}

	case GL_TRIANGLE_STRIP_ADJACENCY_EXT:
	{
		/* Triangle strip data generation requires triangle data to be present */
		createGridTriangles(test_data);

		/* Generate triangle strip data */
		createGridTriangleStrip(test_data);

		break;
	}

	default:
	{
		TCU_FAIL("Unrecognized test mode");
	}
	}
}

/** Generate Line segment data.
 *
 * @param test_data AdjacencyTestData instance to be filled with line segment data.
 **/
void GeometryShaderAdjacencyTests::createGridLineSegments(AdjacencyTestData& test_data)
{
	/* Generate line segments.
	 *
	 * For simplicity, we consider all possible line segments for the grid. If a given line segment
	 * is already stored, it is discarded.
	 */
	unsigned int nAddedSegments = 0;

	for (unsigned int nPoint = 0; nPoint < m_grid_granulity * m_grid_granulity; ++nPoint)
	{
		AdjacencyGridPoint* pointTL = test_data.m_grid->m_points + nPoint;
		AdjacencyGridPoint* pointTR = 0;
		AdjacencyGridPoint* pointBL = 0;
		AdjacencyGridPoint* pointBR = 0;

		/* Retrieve neighbor point instances */
		pointTR = test_data.m_grid->m_points + nPoint + 1;
		pointBL = test_data.m_grid->m_points + m_grid_granulity + 1;
		pointBR = test_data.m_grid->m_points + m_grid_granulity + 2;

		/* For each quad, we need to to add at most 4 line segments.
		 *
		 * NOTE: Adjacent points are determined in later stage.
		 **/
		AdjacencyGridLineSegment* candidateSegments = new AdjacencyGridLineSegment[m_n_line_segments];

		candidateSegments[0].m_point_start = pointTL;
		candidateSegments[0].m_point_end   = pointTR;
		candidateSegments[1].m_point_start = pointTR;
		candidateSegments[1].m_point_end   = pointBR;
		candidateSegments[2].m_point_start = pointBR;
		candidateSegments[2].m_point_end   = pointBL;
		candidateSegments[3].m_point_start = pointBL;
		candidateSegments[3].m_point_end   = pointTL;

		for (unsigned int nSegment = 0; nSegment < m_n_line_segments; ++nSegment)
		{
			bool					  alreadyAdded		  = false;
			AdjacencyGridLineSegment* candidateSegmentPtr = candidateSegments + nSegment;

			for (unsigned int n = 0; n < nAddedSegments; ++n)
			{
				AdjacencyGridLineSegment* segmentPtr = test_data.m_grid->m_line_segments + n;

				/* Do not pay attention to direction of the line segment */
				if ((segmentPtr->m_point_end == candidateSegmentPtr->m_point_end ||
					 segmentPtr->m_point_end == candidateSegmentPtr->m_point_start) &&
					(segmentPtr->m_point_start == candidateSegmentPtr->m_point_end ||
					 segmentPtr->m_point_start == candidateSegmentPtr->m_point_start))
				{
					alreadyAdded = true;

					break;
				}
			}

			/* If not already added, store in the array */
			if (!alreadyAdded)
			{
				test_data.m_grid->m_line_segments[nAddedSegments].m_point_end   = candidateSegmentPtr->m_point_end;
				test_data.m_grid->m_line_segments[nAddedSegments].m_point_start = candidateSegmentPtr->m_point_start;

				++nAddedSegments;
			}
		} /* for (all line segments) */

		delete[] candidateSegments;
		candidateSegments = DE_NULL;
	} /* for (all grid points) */

	test_data.m_grid->m_n_segments = nAddedSegments;

	/* Determine adjacent points for line segments */
	for (unsigned int nSegment = 0; nSegment < nAddedSegments; ++nSegment)
	{
		float					  endToAdjacentPointDelta   = 2.0f * static_cast<float>(m_grid_granulity);
		AdjacencyGridLineSegment* segmentPtr				= test_data.m_grid->m_line_segments + nSegment;
		float					  startToAdjacentPointDelta = 2.0f * static_cast<float>(m_grid_granulity);

		/* For start and end points, find an adjacent vertex that is not a part of the considered line segment */
		for (unsigned int nPoint = 0; nPoint < test_data.m_grid->m_n_points; ++nPoint)
		{
			AdjacencyGridPoint* pointPtr = test_data.m_grid->m_points + nPoint;

			if (pointPtr != segmentPtr->m_point_end && pointPtr != segmentPtr->m_point_start)
			{
				float deltaStart = deFloatSqrt(
					(segmentPtr->m_point_start->x - pointPtr->x) * (segmentPtr->m_point_start->x - pointPtr->x) +
					(segmentPtr->m_point_start->y - pointPtr->y) * (segmentPtr->m_point_start->y - pointPtr->y));
				float deltaEnd = deFloatSqrt(
					(segmentPtr->m_point_end->x - pointPtr->x) * (segmentPtr->m_point_end->x - pointPtr->x) +
					(segmentPtr->m_point_end->y - pointPtr->y) * (segmentPtr->m_point_end->y - pointPtr->y));

				if (deltaStart < startToAdjacentPointDelta)
				{
					/* New adjacent point found for start point */
					segmentPtr->m_point_start_adjacent = pointPtr;
					startToAdjacentPointDelta		   = deltaStart;
				}

				if (deltaEnd < endToAdjacentPointDelta)
				{
					/* New adjacent point found for end point */
					segmentPtr->m_point_end_adjacent = pointPtr;
					endToAdjacentPointDelta			 = deltaEnd;
				}
			} /* if (point found) */
		}	 /* for (all points) */
	}		  /* for (all line segments) */
}

/** Generate Line Strip data
 *
 * @param test_data AdjacencyTestData instance ot be filled with line strip data.
 **/
void GeometryShaderAdjacencyTests::createGridLineStrip(AdjacencyTestData& test_data)
{
	/* Add 2 extra point for adjacency start+end points */
	test_data.m_grid->m_line_strip.m_n_points = test_data.m_grid->m_n_points + 2;
	test_data.m_grid->m_line_strip.m_points   = new AdjacencyGridPoint[test_data.m_grid->m_line_strip.m_n_points];

	memset(test_data.m_grid->m_line_strip.m_points, 0,
		   sizeof(AdjacencyGridPoint) * test_data.m_grid->m_line_strip.m_n_points);

	for (unsigned int n = 0; n < test_data.m_grid->m_line_strip.m_n_points; ++n)
	{
		AdjacencyGridPoint* pointPtr = test_data.m_grid->m_line_strip.m_points + n;

		pointPtr->index = n;

		/* If this is a start point, use any of the adjacent points */
		if (n == 0)
		{
			pointPtr->x = test_data.m_grid->m_line_segments[0].m_point_start_adjacent->x;
			pointPtr->y = test_data.m_grid->m_line_segments[0].m_point_start_adjacent->y;
		}
		else
			/* Last point should be handled analogously */
			if (n == (test_data.m_grid->m_line_strip.m_n_points - 1))
		{
			pointPtr->x = test_data.m_grid->m_line_segments[test_data.m_grid->m_n_segments - 1].m_point_end_adjacent->x;
			pointPtr->y = test_data.m_grid->m_line_segments[test_data.m_grid->m_n_segments - 1].m_point_end_adjacent->y;
		}
		else
		/* Intermediate points */
		{
			pointPtr->x = test_data.m_grid->m_line_segments[n - 1].m_point_start->x;
			pointPtr->y = test_data.m_grid->m_line_segments[n - 1].m_point_start->y;
		}
	} /* for (all points) */
}

/** Generate Triangles data.
 *
 * @param test_data AdjacencyTestData instance to be filled with triangles data.
 **/
void GeometryShaderAdjacencyTests::createGridTriangles(AdjacencyTestData& test_data)
{
	const int	nTrianglesPerQuad = 2;
	unsigned int nTriangles		   = m_grid_granulity * m_grid_granulity * nTrianglesPerQuad;

	test_data.m_grid->m_triangles   = new AdjacencyGridTriangle[nTriangles];
	test_data.m_grid->m_n_triangles = nTriangles;

	for (unsigned int nQuad = 0; nQuad < (nTriangles / nTrianglesPerQuad); ++nQuad)
	{
		unsigned int quadTLX = (nQuad) % m_grid_granulity;
		unsigned int quadTLY = (nQuad) / m_grid_granulity;

		/* Grid is built off points row-by-row. */
		AdjacencyGridPoint* pointTL = test_data.m_grid->m_points + (quadTLY * (m_grid_granulity + 1) + quadTLX);
		AdjacencyGridPoint* pointTR = test_data.m_grid->m_points + (quadTLY * (m_grid_granulity + 1) + quadTLX + 1);
		AdjacencyGridPoint* pointBL = test_data.m_grid->m_points + ((quadTLY + 1) * (m_grid_granulity + 1) + quadTLX);
		AdjacencyGridPoint* pointBR =
			test_data.m_grid->m_points + ((quadTLY + 1) * (m_grid_granulity + 1) + quadTLX + 1);

		/* Note: In many cases, the adjacency data used below is not correct topologically-wise.
		 *       However, since we're not doing any rendering, we're safe as long as unique data
		 *       is used.
		 */
		test_data.m_grid->m_triangles[nQuad * nTrianglesPerQuad + 0].m_vertex_x			 = pointTL;
		test_data.m_grid->m_triangles[nQuad * nTrianglesPerQuad + 0].m_vertex_x_adjacent = pointTR;
		test_data.m_grid->m_triangles[nQuad * nTrianglesPerQuad + 0].m_vertex_y			 = pointBR;
		test_data.m_grid->m_triangles[nQuad * nTrianglesPerQuad + 0].m_vertex_y_adjacent = pointBL;
		test_data.m_grid->m_triangles[nQuad * nTrianglesPerQuad + 0].m_vertex_z			 = pointBL;
		test_data.m_grid->m_triangles[nQuad * nTrianglesPerQuad + 0].m_vertex_z_adjacent = pointBR;

		test_data.m_grid->m_triangles[nQuad * nTrianglesPerQuad + 1].m_vertex_x			 = pointTL;
		test_data.m_grid->m_triangles[nQuad * nTrianglesPerQuad + 1].m_vertex_x_adjacent = pointTR;
		test_data.m_grid->m_triangles[nQuad * nTrianglesPerQuad + 1].m_vertex_y			 = pointTR;
		test_data.m_grid->m_triangles[nQuad * nTrianglesPerQuad + 1].m_vertex_y_adjacent = pointTL;
		test_data.m_grid->m_triangles[nQuad * nTrianglesPerQuad + 1].m_vertex_z			 = pointBR;
		test_data.m_grid->m_triangles[nQuad * nTrianglesPerQuad + 1].m_vertex_z_adjacent = pointBL;
	}
}

/** Generate Triangle Strip data.
 *
 * @param test_data AdjacencyTestData instance to be filled with relevant data.
 **/
void GeometryShaderAdjacencyTests::createGridTriangleStrip(AdjacencyTestData& test_data)
{
	/* For simplicity, reuse adjacency data we have already defined for single triangles.
	 * This does not make a correct topology, but our point is to verify that shaders
	 * are fed valid values (as per spec), not to confirm rendering works correctly.
	 */
	const int nVerticesPerTriangleStripPrimitive = 6;

	test_data.m_grid->m_triangle_strip.m_n_points =
		test_data.m_grid->m_n_triangles * nVerticesPerTriangleStripPrimitive;
	test_data.m_grid->m_triangle_strip.m_points = new AdjacencyGridPoint[test_data.m_grid->m_triangle_strip.m_n_points];

	memset(test_data.m_grid->m_triangle_strip.m_points, 0,
		   sizeof(AdjacencyGridPoint) * test_data.m_grid->m_triangle_strip.m_n_points);

	for (unsigned int n = 0; n < test_data.m_grid->m_triangle_strip.m_n_points; ++n)
	{
		AdjacencyGridPoint*	pointPtr		 = test_data.m_grid->m_triangle_strip.m_points + n;
		unsigned int		   triangleIndex = n / nVerticesPerTriangleStripPrimitive;
		AdjacencyGridTriangle* trianglePtr   = test_data.m_grid->m_triangles + triangleIndex;

		pointPtr->index = n;

		switch (n % nVerticesPerTriangleStripPrimitive)
		{
		case 0:
			pointPtr->x = trianglePtr->m_vertex_x->x;
			pointPtr->y = trianglePtr->m_vertex_x->y;
			break;
		case 1:
			pointPtr->x = trianglePtr->m_vertex_x_adjacent->x;
			pointPtr->y = trianglePtr->m_vertex_x_adjacent->y;
			break;
		case 2:
			pointPtr->x = trianglePtr->m_vertex_y->x;
			pointPtr->y = trianglePtr->m_vertex_y->y;
			break;
		case 3:
			pointPtr->x = trianglePtr->m_vertex_y_adjacent->x;
			pointPtr->y = trianglePtr->m_vertex_y_adjacent->y;
			break;
		case 4:
			pointPtr->x = trianglePtr->m_vertex_z->x;
			pointPtr->y = trianglePtr->m_vertex_z->y;
			break;
		case 5:
			pointPtr->x = trianglePtr->m_vertex_z_adjacent->x;
			pointPtr->y = trianglePtr->m_vertex_z_adjacent->y;
			break;
		}
	} /* for (all points) */
}

/** Set line vertex data used to be used by non-indiced draw calls.
 *
 * @param test_data AdjacencyTestData instance to be filled with relevant data.
 **/
void GeometryShaderAdjacencyTests::setLinePointsNonindiced(AdjacencyTestData& test_data)
{
	float* travellerExpectedAdjacencyGeometryPtr = 0;
	float* travellerExpectedGeometryPtr			 = 0;
	float* travellerPtr							 = 0;

	/* Set buffer sizes */
	test_data.m_n_vertices = test_data.m_grid->m_n_segments * 2 /* start + end points form a segment */;
	test_data.m_geometry_bo_size =
		static_cast<glw::GLuint>(test_data.m_n_vertices * m_n_components_output * sizeof(float));
	test_data.m_vertex_data_bo_size = static_cast<glw::GLuint>(test_data.m_n_vertices * m_n_components_input *
															   2 /* include adjacency info */ * sizeof(float));

	/* Allocate memory for input and expected data */
	test_data.m_expected_adjacency_geometry = new float[test_data.m_geometry_bo_size / sizeof(float)];
	test_data.m_expected_geometry			= new float[test_data.m_geometry_bo_size / sizeof(float)];
	test_data.m_vertex_data					= new float[test_data.m_vertex_data_bo_size / sizeof(float)];

	travellerExpectedAdjacencyGeometryPtr = test_data.m_expected_adjacency_geometry;
	travellerExpectedGeometryPtr		  = test_data.m_expected_geometry;
	travellerPtr						  = test_data.m_vertex_data;

	/* Set input and expected values */
	for (unsigned int n = 0; n < test_data.m_grid->m_n_segments; ++n)
	{
		AdjacencyGridLineSegment* segmentPtr = test_data.m_grid->m_line_segments + n;

		*travellerPtr = segmentPtr->m_point_start_adjacent->x;
		++travellerPtr;
		*travellerPtr = segmentPtr->m_point_start_adjacent->y;
		++travellerPtr;
		*travellerPtr = segmentPtr->m_point_start->x;
		++travellerPtr;
		*travellerPtr = segmentPtr->m_point_start->y;
		++travellerPtr;
		*travellerPtr = segmentPtr->m_point_end->x;
		++travellerPtr;
		*travellerPtr = segmentPtr->m_point_end->y;
		++travellerPtr;
		*travellerPtr = segmentPtr->m_point_end_adjacent->x;
		++travellerPtr;
		*travellerPtr = segmentPtr->m_point_end_adjacent->y;
		++travellerPtr;

		*travellerExpectedAdjacencyGeometryPtr = segmentPtr->m_point_start_adjacent->x;
		++travellerExpectedAdjacencyGeometryPtr;
		*travellerExpectedAdjacencyGeometryPtr = segmentPtr->m_point_start_adjacent->y;
		++travellerExpectedAdjacencyGeometryPtr;
		*travellerExpectedAdjacencyGeometryPtr = 0;
		++travellerExpectedAdjacencyGeometryPtr;
		*travellerExpectedAdjacencyGeometryPtr = 1;
		++travellerExpectedAdjacencyGeometryPtr;
		*travellerExpectedAdjacencyGeometryPtr = segmentPtr->m_point_end_adjacent->x;
		++travellerExpectedAdjacencyGeometryPtr;
		*travellerExpectedAdjacencyGeometryPtr = segmentPtr->m_point_end_adjacent->y;
		++travellerExpectedAdjacencyGeometryPtr;
		*travellerExpectedAdjacencyGeometryPtr = 0;
		++travellerExpectedAdjacencyGeometryPtr;
		*travellerExpectedAdjacencyGeometryPtr = 1;
		++travellerExpectedAdjacencyGeometryPtr;

		*travellerExpectedGeometryPtr = segmentPtr->m_point_start->x;
		++travellerExpectedGeometryPtr;
		*travellerExpectedGeometryPtr = segmentPtr->m_point_start->y;
		++travellerExpectedGeometryPtr;
		*travellerExpectedGeometryPtr = 0;
		++travellerExpectedGeometryPtr;
		*travellerExpectedGeometryPtr = 1;
		++travellerExpectedGeometryPtr;
		*travellerExpectedGeometryPtr = segmentPtr->m_point_end->x;
		++travellerExpectedGeometryPtr;
		*travellerExpectedGeometryPtr = segmentPtr->m_point_end->y;
		++travellerExpectedGeometryPtr;
		*travellerExpectedGeometryPtr = 0;
		++travellerExpectedGeometryPtr;
		*travellerExpectedGeometryPtr = 1;
		++travellerExpectedGeometryPtr;
	} /* for (all line segments) */
}

/** Set line vertex data used to be used by indiced draw calls.
 *
 * @param test_data AdjacencyTestData instance to be filled with relevant data.
 **/
void GeometryShaderAdjacencyTests::setLinePointsindiced(AdjacencyTestData& test_data)
{
	float*		  travellerExpectedAdjacencyGeometryPtr = 0;
	float*		  travellerExpectedGeometryPtr			= 0;
	unsigned int* travellerIndicesPtr					= 0;
	float*		  travellerPtr							= 0;

	/* Set buffer sizes */
	test_data.m_n_vertices = test_data.m_grid->m_n_segments * 2 /* start + end points form a segment */;
	test_data.m_geometry_bo_size =
		static_cast<glw::GLuint>(test_data.m_n_vertices * m_n_components_output * sizeof(float));
	test_data.m_vertex_data_bo_size =
		static_cast<glw::GLuint>(test_data.m_grid->m_n_points * m_n_components_input * sizeof(float));
	test_data.m_index_data_bo_size =
		static_cast<glw::GLuint>(test_data.m_n_vertices * 2 /* include adjacency info */ * sizeof(unsigned int));

	/* Allocate memory for input and expected data */
	test_data.m_expected_adjacency_geometry = new float[test_data.m_geometry_bo_size / sizeof(float)];
	test_data.m_expected_geometry			= new float[test_data.m_geometry_bo_size / sizeof(float)];
	test_data.m_index_data					= new unsigned int[test_data.m_index_data_bo_size / sizeof(unsigned int)];
	test_data.m_vertex_data					= new float[test_data.m_vertex_data_bo_size / sizeof(float)];

	travellerExpectedAdjacencyGeometryPtr = test_data.m_expected_adjacency_geometry;
	travellerExpectedGeometryPtr		  = test_data.m_expected_geometry;
	travellerIndicesPtr					  = test_data.m_index_data;
	travellerPtr						  = test_data.m_vertex_data;

	/* Set input and expected values */
	for (unsigned int n = 0; n < test_data.m_grid->m_n_points; ++n)
	{
		*travellerPtr = test_data.m_grid->m_points[n].x;
		++travellerPtr;
		*travellerPtr = test_data.m_grid->m_points[n].y;
		++travellerPtr;
	}

	for (unsigned int n = 0; n < test_data.m_grid->m_n_segments; ++n)
	{
		AdjacencyGridLineSegment* segmentPtr = test_data.m_grid->m_line_segments + n;

		*travellerIndicesPtr = segmentPtr->m_point_end_adjacent->index;
		++travellerIndicesPtr;
		*travellerIndicesPtr = segmentPtr->m_point_end->index;
		++travellerIndicesPtr;
		*travellerIndicesPtr = segmentPtr->m_point_start->index;
		++travellerIndicesPtr;
		*travellerIndicesPtr = segmentPtr->m_point_start_adjacent->index;
		++travellerIndicesPtr;

		*travellerExpectedAdjacencyGeometryPtr = segmentPtr->m_point_end_adjacent->x;
		++travellerExpectedAdjacencyGeometryPtr;
		*travellerExpectedAdjacencyGeometryPtr = segmentPtr->m_point_end_adjacent->y;
		++travellerExpectedAdjacencyGeometryPtr;
		*travellerExpectedAdjacencyGeometryPtr = 0;
		++travellerExpectedAdjacencyGeometryPtr;
		*travellerExpectedAdjacencyGeometryPtr = 1;
		++travellerExpectedAdjacencyGeometryPtr;
		*travellerExpectedAdjacencyGeometryPtr = segmentPtr->m_point_start_adjacent->x;
		++travellerExpectedAdjacencyGeometryPtr;
		*travellerExpectedAdjacencyGeometryPtr = segmentPtr->m_point_start_adjacent->y;
		++travellerExpectedAdjacencyGeometryPtr;
		*travellerExpectedAdjacencyGeometryPtr = 0;
		++travellerExpectedAdjacencyGeometryPtr;
		*travellerExpectedAdjacencyGeometryPtr = 1;
		++travellerExpectedAdjacencyGeometryPtr;

		*travellerExpectedGeometryPtr = segmentPtr->m_point_end->x;
		++travellerExpectedGeometryPtr;
		*travellerExpectedGeometryPtr = segmentPtr->m_point_end->y;
		++travellerExpectedGeometryPtr;
		*travellerExpectedGeometryPtr = 0;
		++travellerExpectedGeometryPtr;
		*travellerExpectedGeometryPtr = 1;
		++travellerExpectedGeometryPtr;
		*travellerExpectedGeometryPtr = segmentPtr->m_point_start->x;
		++travellerExpectedGeometryPtr;
		*travellerExpectedGeometryPtr = segmentPtr->m_point_start->y;
		++travellerExpectedGeometryPtr;
		*travellerExpectedGeometryPtr = 0;
		++travellerExpectedGeometryPtr;
		*travellerExpectedGeometryPtr = 1;
		++travellerExpectedGeometryPtr;
	} /* for (all line segments) */
}

/** Set line strip vertex data used to be used by non-indiced draw calls.
 *
 * @param test_data AdjacencyTestData instance to be filled with relevant data.
 **/
void GeometryShaderAdjacencyTests::setLineStripPointsNonindiced(AdjacencyTestData& test_data)
{
	float* travellerExpectedAdjacencyGeometryPtr = 0;
	float* travellerExpectedGeometryPtr			 = 0;
	float* travellerPtr							 = 0;

	/* Set buffer sizes */
	test_data.m_n_vertices		 = test_data.m_grid->m_line_strip.m_n_points;
	test_data.m_geometry_bo_size = static_cast<glw::GLuint>((test_data.m_n_vertices - 3) * m_n_components_output *
															2 /* start/end */ * sizeof(float));
	test_data.m_vertex_data_bo_size =
		static_cast<glw::GLuint>(test_data.m_n_vertices * m_n_components_input * sizeof(float));

	/* Allocate memory for input and expected data */
	test_data.m_expected_adjacency_geometry = new float[test_data.m_geometry_bo_size / sizeof(float)];
	test_data.m_expected_geometry			= new float[test_data.m_geometry_bo_size / sizeof(float)];
	test_data.m_vertex_data					= new float[test_data.m_vertex_data_bo_size / sizeof(float)];

	travellerExpectedAdjacencyGeometryPtr = test_data.m_expected_adjacency_geometry;
	travellerExpectedGeometryPtr		  = test_data.m_expected_geometry;
	travellerPtr						  = test_data.m_vertex_data;

	/* Set input and expected values */
	for (unsigned int n = 0; n < test_data.m_n_vertices; ++n)
	{
		*travellerPtr = test_data.m_grid->m_line_strip.m_points[n].x;
		++travellerPtr;
		*travellerPtr = test_data.m_grid->m_line_strip.m_points[n].y;
		++travellerPtr;
	}

	for (unsigned int n = 0; n < test_data.m_n_vertices - 3; ++n)
	{
		*travellerExpectedAdjacencyGeometryPtr = test_data.m_grid->m_line_strip.m_points[n].x;
		++travellerExpectedAdjacencyGeometryPtr;
		*travellerExpectedAdjacencyGeometryPtr = test_data.m_grid->m_line_strip.m_points[n].y;
		++travellerExpectedAdjacencyGeometryPtr;
		*travellerExpectedAdjacencyGeometryPtr = 0;
		++travellerExpectedAdjacencyGeometryPtr;
		*travellerExpectedAdjacencyGeometryPtr = 1;
		++travellerExpectedAdjacencyGeometryPtr;
		*travellerExpectedAdjacencyGeometryPtr = test_data.m_grid->m_line_strip.m_points[n + 3].x;
		++travellerExpectedAdjacencyGeometryPtr;
		*travellerExpectedAdjacencyGeometryPtr = test_data.m_grid->m_line_strip.m_points[n + 3].y;
		++travellerExpectedAdjacencyGeometryPtr;
		*travellerExpectedAdjacencyGeometryPtr = 0;
		++travellerExpectedAdjacencyGeometryPtr;
		*travellerExpectedAdjacencyGeometryPtr = 1;
		++travellerExpectedAdjacencyGeometryPtr;

		*travellerExpectedGeometryPtr = test_data.m_grid->m_line_strip.m_points[n + 1].x;
		++travellerExpectedGeometryPtr;
		*travellerExpectedGeometryPtr = test_data.m_grid->m_line_strip.m_points[n + 1].y;
		++travellerExpectedGeometryPtr;
		*travellerExpectedGeometryPtr = 0;
		++travellerExpectedGeometryPtr;
		*travellerExpectedGeometryPtr = 1;
		++travellerExpectedGeometryPtr;
		*travellerExpectedGeometryPtr = test_data.m_grid->m_line_strip.m_points[n + 2].x;
		++travellerExpectedGeometryPtr;
		*travellerExpectedGeometryPtr = test_data.m_grid->m_line_strip.m_points[n + 2].y;
		++travellerExpectedGeometryPtr;
		*travellerExpectedGeometryPtr = 0;
		++travellerExpectedGeometryPtr;
		*travellerExpectedGeometryPtr = 1;
		++travellerExpectedGeometryPtr;
	} /* for (all vertices (apart from the three last ones) ) */
}

/** Set line strip vertex data used to be used by indiced draw calls.
 *
 * @param test_data AdjacencyTestData instance to be filled with relevant data.
 **/
void GeometryShaderAdjacencyTests::setLineStripPointsIndiced(AdjacencyTestData& test_data)
{

	float*		  travellerExpectedAdjacencyGeometryPtr = 0;
	float*		  travellerExpectedGeometryPtr			= 0;
	unsigned int* travellerIndicesPtr					= 0;
	float*		  travellerPtr							= 0;

	/* Set buffer sizes */
	test_data.m_n_vertices		 = test_data.m_grid->m_line_strip.m_n_points;
	test_data.m_geometry_bo_size = static_cast<glw::GLuint>((test_data.m_n_vertices - 3) * m_n_components_output *
															2 /* start/end */ * sizeof(float));
	test_data.m_vertex_data_bo_size =
		static_cast<glw::GLuint>(test_data.m_n_vertices * m_n_components_input * sizeof(float));
	test_data.m_index_data_bo_size = static_cast<glw::GLuint>(test_data.m_n_vertices * sizeof(unsigned int));

	/* Allocate memory for input and expected data */
	test_data.m_expected_adjacency_geometry = new float[test_data.m_geometry_bo_size / sizeof(float)];
	test_data.m_expected_geometry			= new float[test_data.m_geometry_bo_size / sizeof(float)];
	test_data.m_index_data					= new unsigned int[test_data.m_index_data_bo_size / sizeof(unsigned int)];
	test_data.m_vertex_data					= new float[test_data.m_vertex_data_bo_size / sizeof(float)];

	travellerExpectedAdjacencyGeometryPtr = test_data.m_expected_adjacency_geometry;
	travellerExpectedGeometryPtr		  = test_data.m_expected_geometry;
	travellerIndicesPtr					  = test_data.m_index_data;
	travellerPtr						  = test_data.m_vertex_data;

	/* Set input and expected value s*/
	for (unsigned int n = 0; n < test_data.m_n_vertices; ++n)
	{
		*travellerPtr = test_data.m_grid->m_line_strip.m_points[n].x;
		++travellerPtr;
		*travellerPtr = test_data.m_grid->m_line_strip.m_points[n].y;
		++travellerPtr;
	}

	for (unsigned int n = 0; n < test_data.m_n_vertices; ++n)
	{
		*travellerIndicesPtr = (test_data.m_n_vertices - n - 1);
		++travellerIndicesPtr;
	}

	for (unsigned int n = 0; n < test_data.m_n_vertices - 3; ++n)
	{
		AdjacencyGridPoint* pointN0 = test_data.m_grid->m_line_strip.m_points + test_data.m_index_data[n];
		AdjacencyGridPoint* pointN1 = test_data.m_grid->m_line_strip.m_points + test_data.m_index_data[n + 1];
		AdjacencyGridPoint* pointN2 = test_data.m_grid->m_line_strip.m_points + test_data.m_index_data[n + 2];
		AdjacencyGridPoint* pointN3 = test_data.m_grid->m_line_strip.m_points + test_data.m_index_data[n + 3];

		*travellerExpectedAdjacencyGeometryPtr = pointN0->x;
		++travellerExpectedAdjacencyGeometryPtr;
		*travellerExpectedAdjacencyGeometryPtr = pointN0->y;
		++travellerExpectedAdjacencyGeometryPtr;
		*travellerExpectedAdjacencyGeometryPtr = 0;
		++travellerExpectedAdjacencyGeometryPtr;
		*travellerExpectedAdjacencyGeometryPtr = 1;
		++travellerExpectedAdjacencyGeometryPtr;
		*travellerExpectedAdjacencyGeometryPtr = pointN3->x;
		++travellerExpectedAdjacencyGeometryPtr;
		*travellerExpectedAdjacencyGeometryPtr = pointN3->y;
		++travellerExpectedAdjacencyGeometryPtr;
		*travellerExpectedAdjacencyGeometryPtr = 0;
		++travellerExpectedAdjacencyGeometryPtr;
		*travellerExpectedAdjacencyGeometryPtr = 1;
		++travellerExpectedAdjacencyGeometryPtr;

		*travellerExpectedGeometryPtr = pointN1->x;
		++travellerExpectedGeometryPtr;
		*travellerExpectedGeometryPtr = pointN1->y;
		++travellerExpectedGeometryPtr;
		*travellerExpectedGeometryPtr = 0;
		++travellerExpectedGeometryPtr;
		*travellerExpectedGeometryPtr = 1;
		++travellerExpectedGeometryPtr;
		*travellerExpectedGeometryPtr = pointN2->x;
		++travellerExpectedGeometryPtr;
		*travellerExpectedGeometryPtr = pointN2->y;
		++travellerExpectedGeometryPtr;
		*travellerExpectedGeometryPtr = 0;
		++travellerExpectedGeometryPtr;
		*travellerExpectedGeometryPtr = 1;
		++travellerExpectedGeometryPtr;
	} /* for (all vertices apart from the three last ones) */
}

/** Set triangle vertex data used to be used by non-indiced draw calls.
 *
 * @param test_data AdjacencyTestData instance to be filled with relevant data.
 **/
void GeometryShaderAdjacencyTests::setTrianglePointsNonindiced(AdjacencyTestData& test_data)
{
	float* travellerExpectedAdjacencyGeometryPtr = NULL;
	float* travellerExpectedGeometryPtr			 = NULL;
	float* travellerPtr							 = NULL;

	/* Set buffer sizes */
	test_data.m_n_vertices		 = test_data.m_grid->m_n_triangles * m_n_vertices_per_triangle;
	test_data.m_geometry_bo_size = static_cast<glw::GLuint>(
		test_data.m_grid->m_n_triangles * m_n_vertices_per_triangle * m_n_components_output * sizeof(float));
	test_data.m_vertex_data_bo_size = static_cast<glw::GLuint>(test_data.m_n_vertices * m_n_components_input *
															   sizeof(float) * 2); /* include adjacency info */

	/* Allocate memory for input and expected data */
	test_data.m_expected_adjacency_geometry = new float[test_data.m_geometry_bo_size / sizeof(float)];
	test_data.m_expected_geometry			= new float[test_data.m_geometry_bo_size / sizeof(float)];
	test_data.m_vertex_data					= new float[test_data.m_vertex_data_bo_size / sizeof(float)];

	travellerExpectedAdjacencyGeometryPtr = test_data.m_expected_adjacency_geometry;
	travellerExpectedGeometryPtr		  = test_data.m_expected_geometry;
	travellerPtr						  = test_data.m_vertex_data;

	/* Set input and expected values */
	for (unsigned int n = 0; n < test_data.m_grid->m_n_triangles; ++n)
	{
		AdjacencyGridTriangle* trianglePtr = test_data.m_grid->m_triangles + n;

		*travellerPtr = trianglePtr->m_vertex_x->x;
		++travellerPtr;
		*travellerPtr = trianglePtr->m_vertex_x->y;
		++travellerPtr;
		*travellerPtr = trianglePtr->m_vertex_x_adjacent->x;
		++travellerPtr;
		*travellerPtr = trianglePtr->m_vertex_x_adjacent->y;
		++travellerPtr;
		*travellerPtr = trianglePtr->m_vertex_y->x;
		++travellerPtr;
		*travellerPtr = trianglePtr->m_vertex_y->y;
		++travellerPtr;
		*travellerPtr = trianglePtr->m_vertex_y_adjacent->x;
		++travellerPtr;
		*travellerPtr = trianglePtr->m_vertex_y_adjacent->y;
		++travellerPtr;
		*travellerPtr = trianglePtr->m_vertex_z->x;
		++travellerPtr;
		*travellerPtr = trianglePtr->m_vertex_z->y;
		++travellerPtr;
		*travellerPtr = trianglePtr->m_vertex_z_adjacent->x;
		++travellerPtr;
		*travellerPtr = trianglePtr->m_vertex_z_adjacent->y;
		++travellerPtr;

		*travellerExpectedAdjacencyGeometryPtr = trianglePtr->m_vertex_x_adjacent->x;
		++travellerExpectedAdjacencyGeometryPtr;
		*travellerExpectedAdjacencyGeometryPtr = trianglePtr->m_vertex_x_adjacent->y;
		++travellerExpectedAdjacencyGeometryPtr;
		*travellerExpectedAdjacencyGeometryPtr = 0;
		++travellerExpectedAdjacencyGeometryPtr;
		*travellerExpectedAdjacencyGeometryPtr = 1;
		++travellerExpectedAdjacencyGeometryPtr;
		*travellerExpectedAdjacencyGeometryPtr = trianglePtr->m_vertex_y_adjacent->x;
		++travellerExpectedAdjacencyGeometryPtr;
		*travellerExpectedAdjacencyGeometryPtr = trianglePtr->m_vertex_y_adjacent->y;
		++travellerExpectedAdjacencyGeometryPtr;
		*travellerExpectedAdjacencyGeometryPtr = 0;
		++travellerExpectedAdjacencyGeometryPtr;
		*travellerExpectedAdjacencyGeometryPtr = 1;
		++travellerExpectedAdjacencyGeometryPtr;
		*travellerExpectedAdjacencyGeometryPtr = trianglePtr->m_vertex_z_adjacent->x;
		++travellerExpectedAdjacencyGeometryPtr;
		*travellerExpectedAdjacencyGeometryPtr = trianglePtr->m_vertex_z_adjacent->y;
		++travellerExpectedAdjacencyGeometryPtr;
		*travellerExpectedAdjacencyGeometryPtr = 0;
		++travellerExpectedAdjacencyGeometryPtr;
		*travellerExpectedAdjacencyGeometryPtr = 1;
		++travellerExpectedAdjacencyGeometryPtr;

		*travellerExpectedGeometryPtr = trianglePtr->m_vertex_x->x;
		++travellerExpectedGeometryPtr;
		*travellerExpectedGeometryPtr = trianglePtr->m_vertex_x->y;
		++travellerExpectedGeometryPtr;
		*travellerExpectedGeometryPtr = 0;
		++travellerExpectedGeometryPtr;
		*travellerExpectedGeometryPtr = 1;
		++travellerExpectedGeometryPtr;
		*travellerExpectedGeometryPtr = trianglePtr->m_vertex_y->x;
		++travellerExpectedGeometryPtr;
		*travellerExpectedGeometryPtr = trianglePtr->m_vertex_y->y;
		++travellerExpectedGeometryPtr;
		*travellerExpectedGeometryPtr = 0;
		++travellerExpectedGeometryPtr;
		*travellerExpectedGeometryPtr = 1;
		++travellerExpectedGeometryPtr;
		*travellerExpectedGeometryPtr = trianglePtr->m_vertex_z->x;
		++travellerExpectedGeometryPtr;
		*travellerExpectedGeometryPtr = trianglePtr->m_vertex_z->y;
		++travellerExpectedGeometryPtr;
		*travellerExpectedGeometryPtr = 0;
		++travellerExpectedGeometryPtr;
		*travellerExpectedGeometryPtr = 1;
		++travellerExpectedGeometryPtr;
	} /* for (all triangles) */
}

/** Set triangle vertex data used to be used by indiced draw calls.
 *
 * @param test_data AdjacencyTestDatainstance to be filled with relevant data.
 **/
void GeometryShaderAdjacencyTests::setTrianglePointsIndiced(AdjacencyTestData& test_data)
{
	float*		  travellerExpectedAdjacencyGeometryPtr = 0;
	float*		  travellerExpectedGeometryPtr			= 0;
	unsigned int* travellerIndicesPtr					= 0;
	float*		  travellerPtr							= 0;

	/* Set buffer sizes */
	test_data.m_n_vertices		 = test_data.m_grid->m_n_triangles * m_n_vertices_per_triangle;
	test_data.m_geometry_bo_size = static_cast<glw::GLuint>(
		test_data.m_grid->m_n_triangles * m_n_vertices_per_triangle * m_n_components_output * sizeof(float));
	test_data.m_vertex_data_bo_size =
		static_cast<glw::GLuint>(test_data.m_grid->m_n_points * m_n_components_input * sizeof(float));
	test_data.m_index_data_bo_size =
		static_cast<glw::GLuint>(test_data.m_n_vertices * sizeof(unsigned int) * 2); /* include adjacency info */

	/* Allocate memory for input and expected data */
	test_data.m_expected_adjacency_geometry = new float[test_data.m_geometry_bo_size / sizeof(float)];
	test_data.m_expected_geometry			= new float[test_data.m_geometry_bo_size / sizeof(float)];
	test_data.m_index_data					= new unsigned int[test_data.m_index_data_bo_size / sizeof(unsigned int)];
	test_data.m_vertex_data					= new float[test_data.m_vertex_data_bo_size / sizeof(float)];

	travellerExpectedAdjacencyGeometryPtr = test_data.m_expected_adjacency_geometry;
	travellerExpectedGeometryPtr		  = test_data.m_expected_geometry;
	travellerPtr						  = test_data.m_vertex_data;
	travellerIndicesPtr					  = test_data.m_index_data;

	/* Set input and expected values */
	for (unsigned int n = 0; n < test_data.m_grid->m_n_points; ++n)
	{
		*travellerPtr = test_data.m_grid->m_points[n].x;
		++travellerPtr;
		*travellerPtr = test_data.m_grid->m_points[n].y;
		++travellerPtr;
	}

	for (unsigned int n = 0; n < test_data.m_grid->m_n_triangles; ++n)
	{
		AdjacencyGridTriangle* trianglePtr = test_data.m_grid->m_triangles + (n + 1) % 2;

		*travellerIndicesPtr = trianglePtr->m_vertex_x->index;
		++travellerIndicesPtr;
		*travellerIndicesPtr = trianglePtr->m_vertex_x_adjacent->index;
		++travellerIndicesPtr;
		*travellerIndicesPtr = trianglePtr->m_vertex_y->index;
		++travellerIndicesPtr;
		*travellerIndicesPtr = trianglePtr->m_vertex_y_adjacent->index;
		++travellerIndicesPtr;
		*travellerIndicesPtr = trianglePtr->m_vertex_z->index;
		++travellerIndicesPtr;
		*travellerIndicesPtr = trianglePtr->m_vertex_z_adjacent->index;
		++travellerIndicesPtr;

		*travellerExpectedAdjacencyGeometryPtr = trianglePtr->m_vertex_x_adjacent->x;
		++travellerExpectedAdjacencyGeometryPtr;
		*travellerExpectedAdjacencyGeometryPtr = trianglePtr->m_vertex_x_adjacent->y;
		++travellerExpectedAdjacencyGeometryPtr;
		*travellerExpectedAdjacencyGeometryPtr = 0;
		++travellerExpectedAdjacencyGeometryPtr;
		*travellerExpectedAdjacencyGeometryPtr = 1;
		++travellerExpectedAdjacencyGeometryPtr;
		*travellerExpectedAdjacencyGeometryPtr = trianglePtr->m_vertex_y_adjacent->x;
		++travellerExpectedAdjacencyGeometryPtr;
		*travellerExpectedAdjacencyGeometryPtr = trianglePtr->m_vertex_y_adjacent->y;
		++travellerExpectedAdjacencyGeometryPtr;
		*travellerExpectedAdjacencyGeometryPtr = 0;
		++travellerExpectedAdjacencyGeometryPtr;
		*travellerExpectedAdjacencyGeometryPtr = 1;
		++travellerExpectedAdjacencyGeometryPtr;
		*travellerExpectedAdjacencyGeometryPtr = trianglePtr->m_vertex_z_adjacent->x;
		++travellerExpectedAdjacencyGeometryPtr;
		*travellerExpectedAdjacencyGeometryPtr = trianglePtr->m_vertex_z_adjacent->y;
		++travellerExpectedAdjacencyGeometryPtr;
		*travellerExpectedAdjacencyGeometryPtr = 0;
		++travellerExpectedAdjacencyGeometryPtr;
		*travellerExpectedAdjacencyGeometryPtr = 1;
		++travellerExpectedAdjacencyGeometryPtr;

		*travellerExpectedGeometryPtr = trianglePtr->m_vertex_x->x;
		++travellerExpectedGeometryPtr;
		*travellerExpectedGeometryPtr = trianglePtr->m_vertex_x->y;
		++travellerExpectedGeometryPtr;
		*travellerExpectedGeometryPtr = 0;
		++travellerExpectedGeometryPtr;
		*travellerExpectedGeometryPtr = 1;
		++travellerExpectedGeometryPtr;
		*travellerExpectedGeometryPtr = trianglePtr->m_vertex_y->x;
		++travellerExpectedGeometryPtr;
		*travellerExpectedGeometryPtr = trianglePtr->m_vertex_y->y;
		++travellerExpectedGeometryPtr;
		*travellerExpectedGeometryPtr = 0;
		++travellerExpectedGeometryPtr;
		*travellerExpectedGeometryPtr = 1;
		++travellerExpectedGeometryPtr;
		*travellerExpectedGeometryPtr = trianglePtr->m_vertex_z->x;
		++travellerExpectedGeometryPtr;
		*travellerExpectedGeometryPtr = trianglePtr->m_vertex_z->y;
		++travellerExpectedGeometryPtr;
		*travellerExpectedGeometryPtr = 0;
		++travellerExpectedGeometryPtr;
		*travellerExpectedGeometryPtr = 1;
		++travellerExpectedGeometryPtr;
	} /* For (all triangles) */
}

/** Set triangle strip vertex data used to be used by non-indiced draw calls.
 *
 * @param test_data AdjacencyTestData instance to be filled with relevant data.
 **/
void GeometryShaderAdjacencyTests::setTriangleStripPointsNonindiced(AdjacencyTestData& test_data)
{
	/* Generate ordered vertex GL_TRIANGLE_STRIP_ADJACENCY_EXT data for actual test.
	 *
	 * "In triangle strips with adjacency, n triangles are drawn where there are
	 *  2 * (n+2) + k vertices passed. k is either 0 or 1; if k is 1, the final
	 *  vertex is ignored. "
	 *
	 * implies: for k input vertices, floor((n - 4) / 2) triangles will be drawn.
	 */
	unsigned int nTriangles = (test_data.m_grid->m_triangle_strip.m_n_points - 4) / 2;

	float* travellerExpectedAdjacencyGeometryPtr = 0;
	float* travellerExpectedGeometryPtr			 = 0;
	float* travellerPtr							 = 0;

	/* Set buffer sizes */
	test_data.m_n_vertices = test_data.m_grid->m_triangle_strip.m_n_points;
	test_data.m_geometry_bo_size =
		static_cast<glw::GLuint>(nTriangles * m_n_components_output * 3 /* adjacent vertices */ * sizeof(float));
	test_data.m_vertex_data_bo_size =
		static_cast<glw::GLuint>(test_data.m_n_vertices * m_n_components_input * sizeof(float));

	/* Allocate memory for input and expected data */
	test_data.m_expected_adjacency_geometry = new float[test_data.m_geometry_bo_size / sizeof(float)];
	test_data.m_expected_geometry			= new float[test_data.m_geometry_bo_size / sizeof(float)];
	test_data.m_vertex_data					= new float[test_data.m_vertex_data_bo_size / sizeof(float)];

	travellerExpectedAdjacencyGeometryPtr = test_data.m_expected_adjacency_geometry;
	travellerExpectedGeometryPtr		  = test_data.m_expected_geometry;
	travellerPtr						  = test_data.m_vertex_data;

	/* Set input and expected values */
	for (unsigned int n = 0; n < test_data.m_n_vertices; ++n)
	{
		*travellerPtr = test_data.m_grid->m_triangle_strip.m_points[n].x;
		++travellerPtr;
		*travellerPtr = test_data.m_grid->m_triangle_strip.m_points[n].y;
		++travellerPtr;
	}

	for (unsigned int n = 0; n < nTriangles; ++n)
	{
		/* Derived from per table 2.X1 from the spec */
		int vertexIndex[3]	= { -1, -1, -1 };
		int adjVertexIndex[3] = { -1, -1, -1 };

		if (n == 0)
		{
			/* first (i==0) */
			adjVertexIndex[0] = 2;
			adjVertexIndex[1] = 7;
			adjVertexIndex[2] = 4;
			vertexIndex[0]	= 1;
			vertexIndex[1]	= 3;
			vertexIndex[2]	= 5;
		}
		else if (n == nTriangles - 1)
		{
			if (n % 2 == 0)
			{
				/* last (i==n-1, i even) */
				adjVertexIndex[0] = 2 * n - 1;
				adjVertexIndex[1] = 2 * n + 6;
				adjVertexIndex[2] = 2 * n + 4;
				vertexIndex[0]	= 2 * n + 1;
				vertexIndex[1]	= 2 * n + 3;
				vertexIndex[2]	= 2 * n + 5;
			}
			else
			{
				/* last (i==n-1, i odd) */
				adjVertexIndex[0] = 2 * n - 1;
				adjVertexIndex[1] = 2 * n + 4;
				adjVertexIndex[2] = 2 * n + 6;
				vertexIndex[0]	= 2 * n + 3;
				vertexIndex[1]	= 2 * n + 1;
				vertexIndex[2]	= 2 * n + 5;
			}
		}
		else
		{
			if (n % 2 == 0)
			{
				/* middle (i even) */
				adjVertexIndex[0] = 2 * n - 1;
				adjVertexIndex[1] = 2 * n + 7;
				adjVertexIndex[2] = 2 * n + 4;
				vertexIndex[0]	= 2 * n + 1;
				vertexIndex[1]	= 2 * n + 3;
				vertexIndex[2]	= 2 * n + 5;
			}
			else
			{
				/* middle (i odd) */
				adjVertexIndex[0] = 2 * n - 1;
				adjVertexIndex[1] = 2 * n + 4;
				adjVertexIndex[2] = 2 * n + 7;
				vertexIndex[0]	= 2 * n + 3;
				vertexIndex[1]	= 2 * n + 1;
				vertexIndex[2]	= 2 * n + 5;
			}
		}

		/* Spec assumes vertices are indexed from 1 */
		vertexIndex[0]--;
		vertexIndex[1]--;
		vertexIndex[2]--;
		adjVertexIndex[0]--;
		adjVertexIndex[1]--;
		adjVertexIndex[2]--;

		*travellerExpectedAdjacencyGeometryPtr = test_data.m_grid->m_triangle_strip.m_points[adjVertexIndex[0]].x;
		++travellerExpectedAdjacencyGeometryPtr;
		*travellerExpectedAdjacencyGeometryPtr = test_data.m_grid->m_triangle_strip.m_points[adjVertexIndex[0]].y;
		++travellerExpectedAdjacencyGeometryPtr;
		*travellerExpectedAdjacencyGeometryPtr = 0;
		++travellerExpectedAdjacencyGeometryPtr;
		*travellerExpectedAdjacencyGeometryPtr = 1;
		++travellerExpectedAdjacencyGeometryPtr;
		*travellerExpectedAdjacencyGeometryPtr = test_data.m_grid->m_triangle_strip.m_points[adjVertexIndex[1]].x;
		++travellerExpectedAdjacencyGeometryPtr;
		*travellerExpectedAdjacencyGeometryPtr = test_data.m_grid->m_triangle_strip.m_points[adjVertexIndex[1]].y;
		++travellerExpectedAdjacencyGeometryPtr;
		*travellerExpectedAdjacencyGeometryPtr = 0;
		++travellerExpectedAdjacencyGeometryPtr;
		*travellerExpectedAdjacencyGeometryPtr = 1;
		++travellerExpectedAdjacencyGeometryPtr;
		*travellerExpectedAdjacencyGeometryPtr = test_data.m_grid->m_triangle_strip.m_points[adjVertexIndex[2]].x;
		++travellerExpectedAdjacencyGeometryPtr;
		*travellerExpectedAdjacencyGeometryPtr = test_data.m_grid->m_triangle_strip.m_points[adjVertexIndex[2]].y;
		++travellerExpectedAdjacencyGeometryPtr;
		*travellerExpectedAdjacencyGeometryPtr = 0;
		++travellerExpectedAdjacencyGeometryPtr;
		*travellerExpectedAdjacencyGeometryPtr = 1;
		++travellerExpectedAdjacencyGeometryPtr;

		*travellerExpectedGeometryPtr = test_data.m_grid->m_triangle_strip.m_points[vertexIndex[0]].x;
		++travellerExpectedGeometryPtr;
		*travellerExpectedGeometryPtr = test_data.m_grid->m_triangle_strip.m_points[vertexIndex[0]].y;
		++travellerExpectedGeometryPtr;
		*travellerExpectedGeometryPtr = 0;
		++travellerExpectedGeometryPtr;
		*travellerExpectedGeometryPtr = 1;
		++travellerExpectedGeometryPtr;
		*travellerExpectedGeometryPtr = test_data.m_grid->m_triangle_strip.m_points[vertexIndex[1]].x;
		++travellerExpectedGeometryPtr;
		*travellerExpectedGeometryPtr = test_data.m_grid->m_triangle_strip.m_points[vertexIndex[1]].y;
		++travellerExpectedGeometryPtr;
		*travellerExpectedGeometryPtr = 0;
		++travellerExpectedGeometryPtr;
		*travellerExpectedGeometryPtr = 1;
		++travellerExpectedGeometryPtr;
		*travellerExpectedGeometryPtr = test_data.m_grid->m_triangle_strip.m_points[vertexIndex[2]].x;
		++travellerExpectedGeometryPtr;
		*travellerExpectedGeometryPtr = test_data.m_grid->m_triangle_strip.m_points[vertexIndex[2]].y;
		++travellerExpectedGeometryPtr;
		*travellerExpectedGeometryPtr = 0;
		++travellerExpectedGeometryPtr;
		*travellerExpectedGeometryPtr = 1;
		++travellerExpectedGeometryPtr;
	} /* for (all triangles) */
}

/** Set triangle strip vertex data used to be used by indiced draw calls.
 *
 * @param test_data AdjacencyTestData instance to be filled with relevant data.
 **/
void GeometryShaderAdjacencyTests::setTriangleStripPointsIndiced(AdjacencyTestData& test_data)
{
	unsigned int nTriangles = (test_data.m_grid->m_triangle_strip.m_n_points - 4) / 2;

	float*		  travellerExpectedAdjacencyGeometryPtr = 0;
	float*		  travellerExpectedGeometryPtr			= 0;
	unsigned int* travellerIndicesPtr					= 0;
	float*		  travellerPtr							= 0;

	/* Set buffer sizes */
	test_data.m_n_vertices = test_data.m_grid->m_triangle_strip.m_n_points;
	test_data.m_geometry_bo_size =
		static_cast<glw::GLuint>(nTriangles * m_n_components_output * 3 /* adjacent vertices */ * sizeof(float));
	test_data.m_vertex_data_bo_size =
		static_cast<glw::GLuint>(test_data.m_n_vertices * m_n_components_input * sizeof(float));
	test_data.m_index_data_bo_size = static_cast<glw::GLuint>(test_data.m_n_vertices * sizeof(unsigned int));

	/* Allocate memory for input and expected data */
	test_data.m_expected_adjacency_geometry = new float[test_data.m_geometry_bo_size / sizeof(float)];
	test_data.m_expected_geometry			= new float[test_data.m_geometry_bo_size / sizeof(float)];
	test_data.m_index_data					= new unsigned int[test_data.m_index_data_bo_size / sizeof(unsigned int)];
	test_data.m_vertex_data					= new float[test_data.m_vertex_data_bo_size / sizeof(float)];

	travellerExpectedAdjacencyGeometryPtr = test_data.m_expected_adjacency_geometry;
	travellerExpectedGeometryPtr		  = test_data.m_expected_geometry;
	travellerIndicesPtr					  = test_data.m_index_data;
	travellerPtr						  = test_data.m_vertex_data;

	/* Set input and expected values */
	for (unsigned int n = 0; n < test_data.m_n_vertices; ++n)
	{
		*travellerIndicesPtr = (test_data.m_n_vertices - 1) - n;
		++travellerIndicesPtr;
	}

	for (unsigned int n = 0; n < test_data.m_n_vertices; ++n)
	{
		*travellerPtr = test_data.m_grid->m_triangle_strip.m_points[n].x;
		++travellerPtr;
		*travellerPtr = test_data.m_grid->m_triangle_strip.m_points[n].y;
		++travellerPtr;
	}

	for (unsigned int n = 0; n < nTriangles; ++n)
	{
		/* Derived from per table 2.X1 from the spec */
		int vertexIndex[3]	= { -1, -1, -1 };
		int adjVertexIndex[3] = { -1, -1, -1 };

		if (n == 0)
		{
			/* first (i==0) */
			adjVertexIndex[0] = 2;
			adjVertexIndex[1] = 7;
			adjVertexIndex[2] = 4;
			vertexIndex[0]	= 1;
			vertexIndex[1]	= 3;
			vertexIndex[2]	= 5;
		}
		else if (n == nTriangles - 1)
		{
			if (n % 2 == 0)
			{
				/* last (i==n-1, i even) */
				adjVertexIndex[0] = 2 * n - 1;
				adjVertexIndex[1] = 2 * n + 6;
				adjVertexIndex[2] = 2 * n + 4;
				vertexIndex[0]	= 2 * n + 1;
				vertexIndex[1]	= 2 * n + 3;
				vertexIndex[2]	= 2 * n + 5;
			}
			else
			{
				/* last (i==n-1, i odd) */
				adjVertexIndex[0] = 2 * n - 1;
				adjVertexIndex[1] = 2 * n + 4;
				adjVertexIndex[2] = 2 * n + 6;
				vertexIndex[0]	= 2 * n + 3;
				vertexIndex[1]	= 2 * n + 1;
				vertexIndex[2]	= 2 * n + 5;
			}
		}
		else
		{
			if (n % 2 == 0)
			{
				/* middle (i even) */
				adjVertexIndex[0] = 2 * n - 1;
				adjVertexIndex[1] = 2 * n + 7;
				adjVertexIndex[2] = 2 * n + 4;
				vertexIndex[0]	= 2 * n + 1;
				vertexIndex[1]	= 2 * n + 3;
				vertexIndex[2]	= 2 * n + 5;
			}
			else
			{
				/* middle (i odd) */
				adjVertexIndex[0] = 2 * n - 1;
				adjVertexIndex[1] = 2 * n + 4;
				adjVertexIndex[2] = 2 * n + 7;
				vertexIndex[0]	= 2 * n + 3;
				vertexIndex[1]	= 2 * n + 1;
				vertexIndex[2]	= 2 * n + 5;
			}
		}

		/* Spec assumes vertices are indexed from 1 */
		vertexIndex[0]--;
		vertexIndex[1]--;
		vertexIndex[2]--;
		adjVertexIndex[0]--;
		adjVertexIndex[1]--;
		adjVertexIndex[2]--;

		*travellerExpectedAdjacencyGeometryPtr =
			test_data.m_grid->m_triangle_strip.m_points[test_data.m_index_data[adjVertexIndex[0]]].x;
		++travellerExpectedAdjacencyGeometryPtr;
		*travellerExpectedAdjacencyGeometryPtr =
			test_data.m_grid->m_triangle_strip.m_points[test_data.m_index_data[adjVertexIndex[0]]].y;
		++travellerExpectedAdjacencyGeometryPtr;
		*travellerExpectedAdjacencyGeometryPtr = 0;
		++travellerExpectedAdjacencyGeometryPtr;
		*travellerExpectedAdjacencyGeometryPtr = 1;
		++travellerExpectedAdjacencyGeometryPtr;
		*travellerExpectedAdjacencyGeometryPtr =
			test_data.m_grid->m_triangle_strip.m_points[test_data.m_index_data[adjVertexIndex[1]]].x;
		++travellerExpectedAdjacencyGeometryPtr;
		*travellerExpectedAdjacencyGeometryPtr =
			test_data.m_grid->m_triangle_strip.m_points[test_data.m_index_data[adjVertexIndex[1]]].y;
		++travellerExpectedAdjacencyGeometryPtr;
		*travellerExpectedAdjacencyGeometryPtr = 0;
		++travellerExpectedAdjacencyGeometryPtr;
		*travellerExpectedAdjacencyGeometryPtr = 1;
		++travellerExpectedAdjacencyGeometryPtr;
		*travellerExpectedAdjacencyGeometryPtr =
			test_data.m_grid->m_triangle_strip.m_points[test_data.m_index_data[adjVertexIndex[2]]].x;
		++travellerExpectedAdjacencyGeometryPtr;
		*travellerExpectedAdjacencyGeometryPtr =
			test_data.m_grid->m_triangle_strip.m_points[test_data.m_index_data[adjVertexIndex[2]]].y;
		++travellerExpectedAdjacencyGeometryPtr;
		*travellerExpectedAdjacencyGeometryPtr = 0;
		++travellerExpectedAdjacencyGeometryPtr;
		*travellerExpectedAdjacencyGeometryPtr = 1;
		++travellerExpectedAdjacencyGeometryPtr;

		*travellerExpectedGeometryPtr =
			test_data.m_grid->m_triangle_strip.m_points[test_data.m_index_data[vertexIndex[0]]].x;
		++travellerExpectedGeometryPtr;
		*travellerExpectedGeometryPtr =
			test_data.m_grid->m_triangle_strip.m_points[test_data.m_index_data[vertexIndex[0]]].y;
		++travellerExpectedGeometryPtr;
		*travellerExpectedGeometryPtr = 0;
		++travellerExpectedGeometryPtr;
		*travellerExpectedGeometryPtr = 1;
		++travellerExpectedGeometryPtr;
		*travellerExpectedGeometryPtr =
			test_data.m_grid->m_triangle_strip.m_points[test_data.m_index_data[vertexIndex[1]]].x;
		++travellerExpectedGeometryPtr;
		*travellerExpectedGeometryPtr =
			test_data.m_grid->m_triangle_strip.m_points[test_data.m_index_data[vertexIndex[1]]].y;
		++travellerExpectedGeometryPtr;
		*travellerExpectedGeometryPtr = 0;
		++travellerExpectedGeometryPtr;
		*travellerExpectedGeometryPtr = 1;
		++travellerExpectedGeometryPtr;
		*travellerExpectedGeometryPtr =
			test_data.m_grid->m_triangle_strip.m_points[test_data.m_index_data[vertexIndex[2]]].x;
		++travellerExpectedGeometryPtr;
		*travellerExpectedGeometryPtr =
			test_data.m_grid->m_triangle_strip.m_points[test_data.m_index_data[vertexIndex[2]]].y;
		++travellerExpectedGeometryPtr;
		*travellerExpectedGeometryPtr = 0;
		++travellerExpectedGeometryPtr;
		*travellerExpectedGeometryPtr = 1;
		++travellerExpectedGeometryPtr;
	} /* for (all triangles) */
}
}
