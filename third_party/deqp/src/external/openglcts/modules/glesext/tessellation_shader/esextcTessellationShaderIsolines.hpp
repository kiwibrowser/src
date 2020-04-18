#ifndef _ESEXTCTESSELLATIONSHADERISOLINES_HPP
#define _ESEXTCTESSELLATIONSHADERISOLINES_HPP
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
#include <map>
#include <vector>

namespace glcts
{
/** Implementation of Test Case 34
 *
 *  Consider isoline tessellation.
 *  Make sure that the number of isolines generated is derived from the first
 *  outer tessellation level;
 *  Make sure that the number of segments in each isoline is derived
 *  from the second outer tessellation level.
 *  Make sure that both inner tessellation levels and the third and the fourth
 *  outer tessellation levels do not affect the tessellation process.
 *  Make sure that 'equal_spacing' vertex spacing mode is always used for
 *  vertical subdivision of the input quad.
 *  Make sure no line is drawn between (0, 1) and (1, 1) in (u, v) domain.
 *
 *  0. Consider the following set: {-1, 1, MAX_TESS_GEN_LEVEL_EXT / 2,
 *     MAX_TESS_GEN_LEVEL_EXT}. All combinations of values from this set
 *     in regard to the first two outer tessellation levels for isolines
 *     generator mode should be checked by this test.
 *  1. For each combination and case described in the test summary, output
 *     vertices processed by TE should be XFBed and verified by the test
 *     implementation.
 *  2. For the case where we verify that inner tessellation level and
 *     the 3rd and the 4th outer tessellation levels are ignored,
 *     the test should work along the lines of test case 28.
 *
 **/
class TessellationShadersIsolines : public TestCaseBase
{
public:
	/* Public methods */
	TessellationShadersIsolines(Context& context, const ExtParameters& extParams);

	virtual ~TessellationShadersIsolines(void)
	{
	}

	virtual void		  deinit(void);
	virtual IterateResult iterate(void);

private:
	/* Forward declarations */
	struct _test_descriptor;

	/* Private type definitions */
	/** Stores:
	 *
	 *  a) properties used to generate tessellated coordinates.
	 *  b) pointer to owning test descriptor
	 *  c) captured tessellated coordinates.
	 **/
	typedef struct _test_result
	{
		unsigned int			n_isolines;
		unsigned int			n_vertices;
		const _test_descriptor* parent;
		std::vector<float>		rendered_data;

		int irrelevant_tess_level;
		int outer1_tess_level;
		int outer2_tess_level;

		_test_result()
		{
			n_isolines = 0;
			n_vertices = 0;
			parent	 = DE_NULL;
			rendered_data.clear();

			irrelevant_tess_level = 0;
			outer1_tess_level	 = 0;
			outer2_tess_level	 = 0;
		}
	} _test_result;

	/** Encapsulates:
	 *
	 *  a) Tessellation properties corresponding to what is set
	 *     in TC and TE stages, when the particular program object
	 *     is used for draw calls.
	 *  b) Pointer to test instance.
	 **/
	typedef struct _test_descriptor
	{
		TessellationShadersIsolines* parent;

		float								inner_tess_levels[2];
		float								irrelevant_tess_level;
		float								outer_tess_levels[4];
		_tessellation_shader_vertex_spacing vertex_spacing_mode;

		_test_descriptor() : irrelevant_tess_level(0)
		{
			parent = DE_NULL;

			memset(inner_tess_levels, 0, sizeof(inner_tess_levels));
			memset(outer_tess_levels, 0, sizeof(outer_tess_levels));

			vertex_spacing_mode = TESSELLATION_SHADER_VERTEX_SPACING_UNKNOWN;
		}
	} _test_descriptor;

	/** Function pointer used to refer to verification functions that operate on
	 *  a single test result descriptor.
	 **/
	typedef void (*PFNTESTRESULTPROCESSORPROC)(_test_result& test_result, glw::GLenum glToken);

	typedef std::vector<_test_descriptor> _tests;
	typedef _tests::const_iterator		  _tests_const_iterator;
	typedef _tests::iterator			  _tests_iterator;
	typedef std::map<_tessellation_shader_vertex_spacing, _tests> _tests_per_vertex_spacing_map;
	typedef _tests_per_vertex_spacing_map::const_iterator _tests_per_vertex_spacing_map_const_iterator;
	typedef _tests_per_vertex_spacing_map::iterator		  _tests_per_vertex_spacing_map_iterator;
	typedef std::vector<_test_result>					  _test_results;
	typedef _test_results::iterator						  _test_results_iterator;
	typedef std::map<_tessellation_shader_vertex_spacing, _test_results> _test_results_per_vertex_spacing_map;
	typedef _test_results_per_vertex_spacing_map::const_iterator _test_results_per_vertex_spacing_map_const_iterator;
	typedef _test_results_per_vertex_spacing_map::iterator		 _test_results_per_vertex_spacing_map_iterator;

	typedef int _irrelevant_tess_level;
	typedef int _outer1_tess_level;
	typedef int _outer2_tess_level;

	/* Private methods */
	void countIsolines(_test_result& test_result);

	_test_result findTestResult(_irrelevant_tess_level irrelevant_tess_level, _outer1_tess_level outer1_tess_level,
								_outer2_tess_level					outer2_tess_level,
								_tessellation_shader_vertex_spacing vertex_spacing_mode);

	Context& getContext();
	void	 initTest(void);

	void initTestDescriptor(_tessellation_shader_vertex_spacing vertex_spacing_mode, const float* inner_tess_levels,
							const float* outer_tess_levels, float irrelevant_tess_level, _test_descriptor& test);

	void runForAllTestResults(PFNTESTRESULTPROCESSORPROC pProcessTestResult);

	static void checkFirstOuterTessellationLevelEffect(_test_result&	 test_result,
													   const glw::GLenum glMaxTessGenLevelToken);

	void checkIrrelevantTessellationLevelsHaveNoEffect();

	static void checkNoLineSegmentIsDefinedAtHeightOne(_test_result& test_result, const glw::GLenum unused);

	static void checkSecondOuterTessellationLevelEffect(_test_result&	 test_result,
														const glw::GLenum glMaxTessGenLevelToken);

	void checkVertexSpacingDoesNotAffectAmountOfGeneratedIsolines();

	/* Private variables */
	float m_irrelevant_tess_value_1;
	float m_irrelevant_tess_value_2;

	_test_results_per_vertex_spacing_map m_test_results;
	_tests_per_vertex_spacing_map		 m_tests;
	TessellationShaderUtils*			 m_utils_ptr;
	glw::GLuint							 m_vao_id;
};

} // namespace glcts

#endif // _ESEXTCTESSELLATIONSHADERISOLINES_HPP
