#ifndef _ESEXTCTESSELLATIONSHADERVERTEXSPACING_HPP
#define _ESEXTCTESSELLATIONSHADERVERTEXSPACING_HPP
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
#include "esextcTessellationShaderUtils.hpp"
#include "gluShaderUtil.hpp"
#include "tcuDefs.hpp"
#include <deMath.h>

namespace glcts
{
/** Implementation of Test Case 25
 *
 *  Make sure that vertex spacing mode defined in a tessellation evaluation
 *  shader affects the tessellation primitive generator as per specification,
 *  to the limit enforced by implementation-dependent behaviour.
 *  Consider all three tessellation primitive generator modes (triangles,
 *  quads, isolines). TE stage should be run in point mode.
 *  Make sure that by default the tessellation primitive generator works in
 *  equal_spacing spacing mode.
 *  Make sure that negative inner levels are clamped as defined for active
 *  vertex spacing mode.
 *
 *  Technical details:
 *
 *  0. Consider the following set: {-1 (where valid), 1, MAX_TESS_GEN_LEVEL_EXT / 2,
 *     MAX_TESS_GEN_LEVEL_EXT}. All combinations of values from this set
 *     in regard to relevant inner/outer tessellation levels for all
 *     primitive generator modes should be checked by this test.
 *
 *  1. This test should capture vertices output by TE stage and verify their
 *     locations. If an edge is defined by function y = a * t + b (a, b
 *     computed from locations of edge start & end points), t should be
 *     calculated for each vertex that is a part of the edge considered.
 *  2. Test passes if t values are in agreement with the specification
 *     (assume epsilon 1e-5) and vertex spacing mode considered.
 *
 *  This test implementation skips configurations meeting all of the following
 *  properties:
 *
 *  - primitive mode:      QUADS
 *  - vertex spacing mode: FRACTIONAL ODD
 *  - inner tess level[0]: <= 1
 *  - inner tess level[1]: <= 1
 *
 *  These configurations are affected by a nuance described in greater
 *  detail in Khronos Bugzilla#11979, which this test cannot handle.
 **/
class TessellationShaderVertexSpacing : public TestCaseBase
{
public:
	/* Public methods */
	TessellationShaderVertexSpacing(Context& context, const ExtParameters& extParams);

	virtual ~TessellationShaderVertexSpacing(void)
	{
	}

	virtual void		  deinit(void);
	void				  initTest(void);
	virtual IterateResult iterate(void);

private:
	/* Private type definitions */
	/** Stores properties of a single test run */
	typedef struct _run
	{
		float								inner[2];
		float								outer[4];
		_tessellation_primitive_mode		primitive_mode;
		_tessellation_shader_vertex_spacing vertex_spacing;

		std::vector<char> data;
		float*			  data_cartesian; /* only used for 'triangles' case */
		unsigned int	  n_vertices;

		/* Constructor. Resets all fields to default values */
		_run()
		{
			memset(inner, 0, sizeof(inner));
			memset(outer, 0, sizeof(outer));

			n_vertices	 = 0;
			primitive_mode = TESSELLATION_SHADER_PRIMITIVE_MODE_UNKNOWN;
			vertex_spacing = TESSELLATION_SHADER_VERTEX_SPACING_UNKNOWN;
		}
	} _run;

	/** Stores either barycentric or Cartesian coordinate data
	 *  (depending on primitive mode of a test run this structure
	 *  will be instantiated for)
	 */
	typedef struct _tess_coordinate
	{
		float u;
		float v;
		float w;

		/* Constructor. Resets all fields to 0 */
		_tess_coordinate()
		{
			u = 0.0f;
			v = 0.0f;
			w = 0.0f;
		}

		/* Constructor.
		 *
		 * @param u Value to set for U component;
		 * @param v Value to set for V component;
		 * @param w Value to set for W component;
		 */
		_tess_coordinate(float _u, float _v, float _w)
		{
			this->u = _u;
			this->v = _v;
			this->w = _w;
		}

		/** Compares two barycentric/Cartesian coordinates, using test-wide epsilon.
		 *
		 *  @param in Coordinate to compare current instance against.
		 *
		 *  @return true if the coordinates are equal, false otherwise.
		 **/
		bool operator==(const _tess_coordinate& in) const;
	} _tess_coordinate;

	/** Stores Cartesian coordinate data. */
	typedef struct _tess_coordinate_cartesian
	{
		float x;
		float y;

		/* Constructor. Resets all values to 0 */
		_tess_coordinate_cartesian()
		{
			x = 0.0f;
			y = 0.0f;
		}

		/* Constructor.
		 *
		 * @param x Value to use for X component;
		 * @param y Value to use for Y component;
		 */
		_tess_coordinate_cartesian(float _x, float _y)
		{
			this->x = _x;
			this->y = _y;
		}

		/** Compares two Cartesian coordinates, using test-wide epsilon.
		 *
		 *  @param in Coordinate to compare current instance against.
		 *
		 *  @return true if the coordinates are equal, false otherwise.
		 **/
		bool operator==(const _tess_coordinate_cartesian& in) const;
	} _tess_coordinate_cartesian;

	/** Stores information on:
	 *
	 *  - a delta between two coordinates;
	 *  - amount of segments that had exactly that length.
	 **/
	typedef struct _tess_coordinate_delta
	{
		unsigned int counter;
		float		 delta;

		/* Constructor. Resets all values to 0 */
		_tess_coordinate_delta()
		{
			counter = 0;
			delta   = 0.0f;
		}
	} _tess_coordinate_delta;

	/** Vector of coordinate deltas */
	typedef std::vector<_tess_coordinate_delta>		_tess_coordinate_deltas;
	typedef _tess_coordinate_deltas::const_iterator _tess_coordinate_deltas_const_iterator;
	typedef _tess_coordinate_deltas::iterator		_tess_coordinate_deltas_iterator;

	/** Vector of Cartesian coordinates making up an edge. */
	typedef std::vector<_tess_coordinate_cartesian> _tess_edge_points;
	typedef _tess_edge_points::const_iterator		_tess_edge_points_const_iterator;
	typedef _tess_edge_points::iterator				_tess_edge_points_iterator;

	/** Defines a single edge of a quad/triangle *or* a single isoline (depending
	 *  on the primitive mode used for a test run, for which the edge is defined)
	 */
	typedef struct _tess_edge
	{
		_tess_edge_points points;
		float			  edge_length;
		float			  outermost_tess_level;
		float			  tess_level;

		/* Constructor.
		 *
		 * @param in_tess_level  Tessellation level value specific to the edge.
		 * @param in_edge_length Total Euclidean length of the edge.
		 */
		_tess_edge(const float& in_tess_level, const float& in_outermost_tess_level, const float& in_edge_length)
			: edge_length(in_edge_length), outermost_tess_level(in_outermost_tess_level), tess_level(in_tess_level)
		{
		}
	} _tess_edge;

	/** Vector of edges */
	typedef std::vector<_tess_edge>		_tess_edges;
	typedef _tess_edges::const_iterator _tess_edges_const_iterator;
	typedef _tess_edges::iterator		_tess_edges_iterator;

	/** Vector of test runs */
	typedef std::vector<_run>	 _runs;
	typedef _runs::const_iterator _runs_const_iterator;

	/** Comparator that is used to sort points relative to a certain origin. */
	struct _comparator_relative_to_base_point
	{
		/* Constructor. Sets all fields to 0 */
		_comparator_relative_to_base_point() : base_point(0, 0)
		{
		}

		/* Constructor.
		 *
		 * @param base_point Origin, against which all comparisons should be run against.
		 */
		_comparator_relative_to_base_point(const _tess_coordinate_cartesian& _base_point) : base_point(_base_point)
		{
		}

		/* Tells which of the user-provided points is closer to the instance-specific
		 * "origin".
		 *
		 * @param a First point to use.
		 * @param b Second point to use.
		 *
		 * @return true if point @param a is closer to the "origin", false otherwise.
		 */
		bool operator()(_tess_coordinate_cartesian a, _tess_coordinate_cartesian b)
		{
			float distance_a_to_base =
				deFloatSqrt((a.x - base_point.x) * (a.x - base_point.x) + (a.y - base_point.y) * (a.y - base_point.y));
			float distance_b_to_base =
				deFloatSqrt((b.x - base_point.x) * (b.x - base_point.x) + (b.y - base_point.y) * (b.y - base_point.y));

			return distance_a_to_base < distance_b_to_base;
		}

		_tess_coordinate_cartesian base_point;
	};

	/** Comparator that is used to compare two tessellation coordinates using FP value
	 *  equal operator.
	 */
	struct _comparator_exact_tess_coordinate_match : public std::unary_function<float, bool>
	{
		/* Constructor.
		 *
		 * @param in_base_coordinate Base tessellation coordinate to compare against.
		 */
		_comparator_exact_tess_coordinate_match(const _tess_coordinate_cartesian& in_base_coordinate)
			: base_coordinate(in_base_coordinate)
		{
		}

		/* Tells if the user-provided tessellation coordinate exactly matches the base tessellation
		 * coordinate.
		 *
		 * @param value Tessellation coordinate to use for the operation.
		 *
		 * @return true if the coordinates are equal, false otherwise.
		 */
		bool operator()(const _tess_coordinate_cartesian& value)
		{
			return (value.x == base_coordinate.x) && (value.y == base_coordinate.y);
		}

		_tess_coordinate_cartesian base_coordinate;
	};

	/* Private methods */
	static bool compareEdgeByX(_tess_coordinate_cartesian a, _tess_coordinate_cartesian b);
	static bool compareEdgeByY(_tess_coordinate_cartesian a, _tess_coordinate_cartesian b);

	bool isPointOnLine(const _tess_coordinate_cartesian& line_v1, const _tess_coordinate_cartesian& line_v2,
					   const _tess_coordinate_cartesian& point);

	_tess_edges getEdgesForIsolinesTessellation(const _run& run);
	_tess_edges getEdgesForQuadsTessellation(const _run& run);
	_tess_edges getEdgesForTrianglesTessellation(const _run& run);
	void verifyEdges(const _tess_edges& edges, const _run& run);

	/* Private variables */
	glw::GLint				 m_gl_max_tess_gen_level_value;
	glw::GLuint				 m_vao_id;
	_runs					 m_runs;
	TessellationShaderUtils* m_utils;
};

} // namespace glcts

#endif // _ESEXTCTESSELLATIONSHADERVERTEXSPACING_HPP
