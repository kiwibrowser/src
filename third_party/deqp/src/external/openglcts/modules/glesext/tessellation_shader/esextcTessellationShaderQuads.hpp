#ifndef _ESEXTCTESSELLATIONSHADERQUADS_HPP
#define _ESEXTCTESSELLATIONSHADERQUADS_HPP
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
#include <vector>

namespace glcts
{
/** A DEQP CTS test group that collects all tests that verify quad
 *  tessellation.
 */
class TessellationShaderQuadsTests : public glcts::TestCaseGroupBase
{
public:
	/* Public methods */
	TessellationShaderQuadsTests(glcts::Context& context, const ExtParameters& extParams);
	virtual ~TessellationShaderQuadsTests(void)
	{
	}

	void init(void);

private:
	/* Private methods */
	TessellationShaderQuadsTests(const TessellationShaderQuadsTests& other);
	TessellationShaderQuadsTests& operator=(const TessellationShaderQuadsTests& other);
};

/** Implementation of Test Case 32
 *
 *  Consider quad tessellation.
 *  Make sure that only a single triangle pair covering the outer rectangle
 *  is generated, if both clamped inner tessellation levels and all four
 *  clamped outer tessellation levels are exactly one.
 *
 *  Consider a few different inner and outer tessellation level pairs
 *  combined with vertex spacing modes that clamp/round to the values as
 *  per test summary.
 *
 *  The test should capture vertices output in TE stage, given the
 *  pre-conditions described in the test summary, and then verify vertex
 *  locations. Assume epsilon 1e-5. A single triangle should be drawn.
 *
 **/
class TessellationShaderQuadsDegenerateCase : public TestCaseBase
{
public:
	/* Public methods */
	TessellationShaderQuadsDegenerateCase(Context& context, const ExtParameters& extParams);

	virtual ~TessellationShaderQuadsDegenerateCase(void)
	{
	}

	virtual void		  deinit(void);
	void				  initTest(void);
	virtual IterateResult iterate(void);

private:
	/* Private type definitions */
	typedef struct _run
	{
		float								inner[2];
		float								outer[4];
		_tessellation_shader_vertex_spacing vertex_spacing;

		std::vector<char> data;
		unsigned int	  n_vertices;

		_run()
		{
			memset(inner, 0, sizeof(inner));
			memset(outer, 0, sizeof(outer));

			n_vertices	 = 0;
			vertex_spacing = TESSELLATION_SHADER_VERTEX_SPACING_UNKNOWN;
		}
	} _run;

	typedef std::vector<_run>	 _runs;
	typedef _runs::const_iterator _runs_const_iterator;

	/* Private variables */
	glw::GLuint				 m_vao_id;
	_runs					 m_runs;
	TessellationShaderUtils* m_utils;
};

/** Implementation of Test Case 33
 *
 *  Consider quad tessellation.
 *  Make sure that if either clamped inner tessellation level is set to one, that
 *  tessellation level is treated as though it were originally specified as
 *  2, which would rounded up to result in a two- or three-segment subdivision
 *  according to the tessellation spacing.
 *
 *  Technical details:
 *
 *  1. Consider all vertex spacing modes. For each vertex spacing mode, take
 *     a level value that, given the mode active, would clamp to one. In first
 *     iteration use that value for the first inner tessellation level
 *     (setting the other inner tessellation level to any valid value), then
 *     in the other iteration swap the second inner tessellation level with
 *     the first inner tessellation level.
 *     For equal and fractional even vertex spacing modes used for every
 *     inner tessellation configuration, do:
 *  1a. Using any valid outer tessellation configuration, "draw" four patches.
 *      Capture output vertices from TE stage.
 *  1b. Using the same outer tessellation configuration, but replacing the
 *      rounding value to the value we actually expect it to round to for the
 *      given iteration, again capture output vertices from TE stage.
 *  1c. Iteration passes if primitives captured are identical in both cases.
 *      Assume epsilon 1e-5.
 *
 *     In case of fractional odd vertex spacing, verify that two marker
 *     triangles capping the opposite ends of the inner quad tessellation
 *     region exist. More information about the technique (and the rationale)
 *     can be found in
 *     TessellationShaderQuadsInnerTessellationLevelRounding::iterate().
 *
 *  2. Test passes if all iterations passed successfully.
 *
 **/
class TessellationShaderQuadsInnerTessellationLevelRounding : public TestCaseBase
{
public:
	/* Public methods */
	TessellationShaderQuadsInnerTessellationLevelRounding(Context& context, const ExtParameters& extParams);

	virtual ~TessellationShaderQuadsInnerTessellationLevelRounding(void)
	{
	}

	virtual void		  deinit(void);
	void				  initTest(void);
	virtual IterateResult iterate(void);

private:
	/* Private type definitions */
	typedef struct _run
	{
		float								set1_inner[2];
		float								set1_outer[4];
		float								set2_inner[2];
		float								set2_outer[4];
		_tessellation_shader_vertex_spacing vertex_spacing;

		std::vector<char> set1_data;
		std::vector<char> set2_data;
		unsigned int	  n_vertices;

		_run()
		{
			memset(set1_inner, 0, sizeof(set1_inner));
			memset(set1_outer, 0, sizeof(set1_outer));
			memset(set2_inner, 0, sizeof(set2_inner));
			memset(set2_outer, 0, sizeof(set2_outer));

			n_vertices	 = 0;
			vertex_spacing = TESSELLATION_SHADER_VERTEX_SPACING_UNKNOWN;
		}
	} _run;

	typedef std::vector<_run>	 _runs;
	typedef _runs::const_iterator _runs_const_iterator;

	/* Private methods */
	std::vector<_vec2> getUniqueTessCoordinatesFromVertexDataSet(const float*		raw_data,
																 const unsigned int n_raw_data_vertices);

	/* Private variables */
	glw::GLuint				 m_vao_id;
	_runs					 m_runs;
	TessellationShaderUtils* m_utils;
};

} // namespace glcts

#endif // _ESEXTCTESSELLATIONSHADERQUADS_HPP
