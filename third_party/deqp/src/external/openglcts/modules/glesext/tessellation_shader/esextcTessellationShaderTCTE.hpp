#ifndef _ESEXTCTESSELLATIONSHADERTCTE_HPP
#define _ESEXTCTESSELLATIONSHADERTCTE_HPP
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

namespace glcts
{

/** A DEQP CTS test group that collects all tests that verify various
 *  interactions between tessellation control and tessellation evaluation
 *  shaders
 */
class TessellationShaderTCTETests : public glcts::TestCaseGroupBase
{
public:
	/* Public methods */
	TessellationShaderTCTETests(glcts::Context& context, const ExtParameters& extParams);

	virtual ~TessellationShaderTCTETests(void)
	{
	}

	void init(void);

private:
	/* Private methods */
	TessellationShaderTCTETests(const TessellationShaderTCTETests& other);
	TessellationShaderTCTETests& operator=(const TessellationShaderTCTETests& other);
};

/** Implementation of Test Case 50
 *
 *  Make sure that tessellation control shader can correctly read per-vertex
 *  values, as modified by a vertex shader. Verify per-vertex gl_Position
 *  and gl_PointSize values are assigned values as in vertex shader.
 *  Make sure that per-vertex & per-patch outputs written to in tessellation
 *  control shader can be correctly read by tessellation evaluation shader.
 *  Pay special attention to gl_Position and gl_PointSize.
 *  Make sure that per-vertex output variables of a tessellation evaluation
 *  shader can be correctly read by a geometry shader.
 *
 * Note: gl_PointSize should only be passed down the rendering pipeline and
 *          then verified by the test if GL_EXT_tessellation_point_size
 *          extension support is reported.
 *
 *  1. The test should run in three iterations:
 *  1a. Vertex + TC + TE stages should be defined;
 *  1b. Vertex + TC + TE + GS stages should be defined (if geometry
 *      shaders are supported);
 *  2. The test should be run for all three tessellator primitive types,
 *     with inner and outer tessellation levels set to reasonably small
 *     values but not equal to 1 for the levels that affect the tessellation
 *     process for the primitive type considered.
 *  3. Vertex shader should set:
 *  3a. gl_Position to vec4(gl_VertexID);
 *  3b. gl_PointSize to 1.0 / float(gl_VertexID);
 *  3c. an output vec4 variable to:
 *      vec4(gl_VertexID, gl_VertexID * 0.5, gl_VertexID * 0.25, gl_VertexID * 0.125);
 *  3d. an outpt ivec4 variable to:
 *      ivec4(gl_VertexID, gl_VertexID + 1, gl_VertexID + 2, gl_VertexID + 3);
 *  4. TC shader should define corresponding input variables and patch
 *     their contents through (for gl_InvocationID invocation) to
 *     differently named output variables;
 *     gl_Position and gl_PointSize values for the invocation
 *     considered should also be forwarded.
 *     One of the invocations for each patch should also set a vec4
 *     and ivec4 per-patch variables to values as above, multiplied by two.
 *  5. TE shader should define corresponding input variables and patch
 *     their contents through to a differently named output variables;
 *     gl_Position, gl_PointSize, gl_TessLevelOuter and gl_TessLevelInner
 *     values for the primitive being processed should also be forwarded.
 *     If TC is present in the pipeline, TE stage should also define
 *     two new output variables and set them to per-patch variable
 *     values, as set by TC.
 *  6. Geometry shader should define corresponding input variables and
 *     patch their contents through to a differently named output
 *     variables;
 *  7. Test implementation should retrieve the captured data once a single
 *     instance of geometry is rendered and verify it.
 *
 **/
class TessellationShaderTCTEDataPassThrough : public TestCaseBase
{
public:
	/* Public methods */
	TessellationShaderTCTEDataPassThrough(Context& context, const ExtParameters& extParams);

	virtual ~TessellationShaderTCTEDataPassThrough(void)
	{
	}

	virtual void		  deinit();
	void				  initTest(void);
	virtual IterateResult iterate(void);

private:
	/* Private type definitions */
	/* Stores all properties of a single test run */
	typedef struct _run
	{
		glw::GLuint fs_id;
		glw::GLuint gs_id;
		glw::GLuint po_id;
		glw::GLuint tcs_id;
		glw::GLuint tes_id;
		glw::GLuint vs_id;

		_tessellation_primitive_mode primitive_mode;
		unsigned int				 n_result_vertices_per_patch;

		std::vector<glw::GLfloat> result_tc_pointSize_data;
		std::vector<_vec4>		  result_tc_position_data;
		std::vector<_vec4>		  result_tc_value1_data;
		std::vector<_ivec4>		  result_tc_value2_data;
		std::vector<_vec4>		  result_te_patch_data;
		std::vector<glw::GLfloat> result_te_pointSize_data;
		std::vector<_vec4>		  result_te_position_data;

		/* Constructor */
		_run()
		{
			fs_id						= 0;
			gs_id						= 0;
			n_result_vertices_per_patch = 0;
			po_id						= 0;
			primitive_mode				= TESSELLATION_SHADER_PRIMITIVE_MODE_UNKNOWN;
			tcs_id						= 0;
			tes_id						= 0;
			vs_id						= 0;
		}
	} _run;

	/* Encapsulates all test runs */
	typedef std::vector<_run>	 _runs;
	typedef _runs::const_iterator _runs_const_iterator;

	/* Private methods */
	void deinitTestRun(_run& run);
	void executeTestRun(_run& run, _tessellation_primitive_mode primitive_mode, bool should_use_geometry_shader,
						bool should_pass_point_size_data_in_gs, bool should_pass_point_size_data_in_ts);

	/* Private variables */
	glw::GLuint				 m_bo_id;
	const unsigned int		 m_n_input_vertices_per_run;
	_runs					 m_runs;
	TessellationShaderUtils* m_utils_ptr;
	glw::GLuint				 m_vao_id;
};

/** Implementation of Test Case 52
 *
 *  This test should iterate over all vertex ordering / spacing / primitive /
 *  point mode permutations, as well as over a number of inner/outer tessellation
 *  level configurations.
 *  The tessellation evaluation shaders used for the test should:
 *
 *  - Make sure that up to gl_MaxPatchVertices vertices' data can be read in
 *    a tessellation evaluation shader.
 *  - Make sure gl_Position and gl_PointSize per-vertex variables can be
 *    accessed for all vertices.
 *
 *  The tessellation control shader used for the test should set iteration-
 *  -specific properties and configure aforementioned per-vertex variables
 *  accordingly.
 *
 *  Both pipeline objects and program objects should be used for the purpose
 *  of the test. For each case, the test should verify that correct objects
 *  defining tessellation control and tessellation stages are reported.
 *  For program objects' case, the test should also confirm that valid shader
 *  types are reported for TC and TE shader objects.
 *  The test should also verify that no objects are assigned to either
 *  stage by default.
 *
 *  The test should check that tessellation-specific properties are reported
 *  correctly.
 *
 **/
class TessellationShaderTCTEgl_MaxPatchVertices_Position_PointSize : public TestCaseBase
{
public:
	/* Public methods */
	TessellationShaderTCTEgl_MaxPatchVertices_Position_PointSize(Context& context, const ExtParameters& extParams);

	virtual ~TessellationShaderTCTEgl_MaxPatchVertices_Position_PointSize(void)
	{
	}

	virtual void		  deinit();
	void				  initTest(void);
	virtual IterateResult iterate(void);

private:
	/* Private type definitions */
	/** Describes a single test run */
	typedef struct _run
	{
		glw::GLuint fs_id;
		glw::GLuint fs_program_id;
		glw::GLuint pipeline_object_id;
		glw::GLuint po_id;
		glw::GLuint tc_id;
		glw::GLuint tc_program_id;
		glw::GLuint te_id;
		glw::GLuint te_program_id;
		glw::GLuint vs_id;
		glw::GLuint vs_program_id;

		glw::GLfloat						 inner[2];
		glw::GLfloat						 outer[4];
		bool								 point_mode;
		_tessellation_primitive_mode		 primitive_mode;
		_tessellation_shader_vertex_ordering vertex_ordering;
		_tessellation_shader_vertex_spacing  vertex_spacing;

		std::vector<float>  result_pointsize_data;
		std::vector<_vec4>  result_position_data;
		std::vector<_vec2>  result_value1_data;
		std::vector<_ivec4> result_value2_data;

		/* Constructor */
		_run()
			: fs_id(0)
			, fs_program_id(0)
			, pipeline_object_id(0)
			, po_id(0)
			, tc_id(0)
			, tc_program_id(0)
			, te_id(0)
			, te_program_id(0)
			, vs_id(0)
			, vs_program_id(0)
			, point_mode(false)
			, primitive_mode(TESSELLATION_SHADER_PRIMITIVE_MODE_UNKNOWN)
			, vertex_ordering(TESSELLATION_SHADER_VERTEX_ORDERING_UNKNOWN)
			, vertex_spacing(TESSELLATION_SHADER_VERTEX_SPACING_UNKNOWN)
		{
			memset(inner, 0, sizeof(inner));
			memset(outer, 0, sizeof(outer));
		}
	} _run;

	/** Describes a set of test runs */
	typedef std::vector<_run>	 _runs;
	typedef _runs::const_iterator _runs_const_iterator;

	/* Private methods */
	void deinitTestRun(_run& run);
	std::string getFragmentShaderCode(bool should_accept_pointsize_data);

	std::string getTessellationControlShaderCode(bool should_pass_pointsize_data, const glw::GLfloat* inner_tess_levels,
												 const glw::GLfloat* outer_tess_levels);

	std::string getTessellationEvaluationShaderCode(bool								 should_pass_pointsize_data,
													_tessellation_primitive_mode		 primitive_mode,
													_tessellation_shader_vertex_ordering vertex_ordering,
													_tessellation_shader_vertex_spacing  vertex_spacing,
													bool								 is_point_mode_enabled);

	std::string getVertexShaderCode(bool should_pass_pointsize_data);
	void initTestRun(_run& run);

	/* Private variables */
	glw::GLuint				 m_bo_id;
	glw::GLint				 m_gl_max_patch_vertices_value;
	glw::GLint				 m_gl_max_tess_gen_level_value;
	_runs					 m_runs;
	TessellationShaderUtils* m_utils_ptr;
	glw::GLuint				 m_vao_id;
};

/** Implementation of Test Case 36
 *
 *  Make sure that values of gl_in[] in a tessellation evaluation shader are
 *  taken from output variables of a tessellation control shader if one is
 *  present. This test should verify that the values are not taken from
 *  a vertex shader.
 *
 *  Technical details:
 *
 *  0. A program consisting of vertex, TC and TE stages should be considered.
 *  1. Vertex shader should output a set of output variables of different types.
 *  2. TC shader should define exactly the same set of output variables.
 *     The data the shader writes to these variables must be different than
 *     in the vertex shader's case.
 *  3. TE shader should define these variables as input variables. It should
 *     copy their values to a set of corresponding output variables, which
 *     the test should then validate by means of Transform Feedback.
 *
 **/
class TessellationShaderTCTEgl_in : public TestCaseBase
{
public:
	/* Public methods */
	TessellationShaderTCTEgl_in(Context& context, const ExtParameters& extParams);

	virtual ~TessellationShaderTCTEgl_in(void)
	{
	}

	virtual void		  deinit();
	void				  initTest(void);
	virtual IterateResult iterate(void);

private:
	/* Private methods */
	void getXFBProperties(const glw::GLchar*** out_names, glw::GLint* out_n_names, glw::GLint* out_xfb_size);

	/* Private variables */
	glw::GLuint m_bo_id;
	glw::GLuint m_fs_id;
	glw::GLuint m_po_id;
	glw::GLuint m_tcs_id;
	glw::GLuint m_tes_id;
	glw::GLuint m_vao_id;
	glw::GLuint m_vs_id;
};

/**  Implementation of Test Case 37
 *
 *   Make sure that gl_TessLevelOuter[] and gl_TessLevelInner[] accessed from
 *   tessellation evaluation shader hold values, as used in tessellation control
 *   shader for a processed patch (assuming tessellation control shader is
 *   present in the pipeline)
 *   Make sure that gl_TessLevelOuter[] and gl_TessLevelInner[] accessed from
 *   tessellation evaluation shader hold patch parameter values, if no
 *   tessellation control shader is present in the pipeline.
 *   Reported values should not be clamped and rounded, owing to active
 *   vertex spacing mode.
 *
 *   Technical details:
 *
 *   0. The test should use two program objects: one defining a TC stage,
 *      the other one should lack a tessellation control shader.
 *   1. For the first case, the test implementation should check a couple
 *      of different inner/outer tessellation level configurations by reading
 *      them from an uniform that the test will update prior to doing a draw
 *      call.
 *      For the other case, the parameters can be modified using ES API.
 *   2. TE should output inner and output tessellation levels to a varying,
 *      for which Transform Feedback should be configured.
 *   3. Test passes if tessellation levels in TE stage match the ones
 *      defined in TC stage. Pay attention to making sure no clamping
 *      or rounding occurs for any vertex spacing mode.
 *
 **/
class TessellationShaderTCTEgl_TessLevel : public TestCaseBase
{
public:
	/* Public methods */
	TessellationShaderTCTEgl_TessLevel(Context& context, const ExtParameters& extParams);

	virtual ~TessellationShaderTCTEgl_TessLevel(void)
	{
	}

	virtual void		  deinit();
	void				  initTest(void);
	virtual IterateResult iterate(void);

private:
	/* Private type definitions */
	typedef struct _test_descriptor
	{
		_tessellation_test_type				type;
		_tessellation_shader_vertex_spacing vertex_spacing;

		glw::GLuint fs_id;
		glw::GLuint po_id;
		glw::GLuint tcs_id;
		glw::GLuint tes_id;
		glw::GLuint vs_id;

		glw::GLint inner_tess_levels_uniform_location;
		glw::GLint outer_tess_levels_uniform_location;

		_test_descriptor()
		{
			fs_id  = 0;
			po_id  = 0;
			tcs_id = 0;
			tes_id = 0;
			vs_id  = 0;

			inner_tess_levels_uniform_location = 0;
			outer_tess_levels_uniform_location = 0;

			type		   = TESSELLATION_TEST_TYPE_COUNT;
			vertex_spacing = TESSELLATION_SHADER_VERTEX_SPACING_UNKNOWN;
		}
	} _test_descriptor;

	typedef std::vector<_test_descriptor> _tests;
	typedef _tests::const_iterator		  _tests_const_iterator;

	/* Private methods */
	void deinitTestDescriptor(_test_descriptor* test_ptr);

	void initTestDescriptor(_tessellation_test_type test_type, _test_descriptor* out_test_ptr,
							_tessellation_shader_vertex_spacing vertex_spacing_mode);

	/* Private variables */
	glw::GLint m_gl_max_tess_gen_level_value;

	glw::GLuint m_bo_id;
	_tests		m_tests;
	glw::GLuint m_vao_id;
};

/**  Implementation of Test Case 35
 *
 *   Make sure that the number of vertices in input patch of a tessellation
 *   evaluation shader is:
 *
 *   * fixed and equal to tessellation control shader output patch size parameter
 *     from the time the program object was linked last time, should tessellation
 *     control shader be present in the pipeline;
 *   * equal to patch size parameter at the time of a draw call, if there
 *     is no tessellation control shader present in the pipeline.
 *
 *   Technical details:
 *
 *   0. Apart from the two cases described in the test summary, the implementation
 *      should also iterate over a number of different patch size values.
 *   1. TE shader should save gl_PatchVerticesIn.length() in an output
 *      variable. This variable should be XFBed to a buffer object and then
 *      verified in the actual test.
 *
 **/
class TessellationShaderTCTEgl_PatchVerticesIn : public TestCaseBase
{
public:
	/* Public methods */
	TessellationShaderTCTEgl_PatchVerticesIn(Context& context, const ExtParameters& extParams);

	virtual ~TessellationShaderTCTEgl_PatchVerticesIn(void)
	{
	}

	virtual void		  deinit();
	void				  initTest(void);
	virtual IterateResult iterate(void);

private:
	/* Private type definitions */
	typedef struct _test_descriptor
	{
		_tessellation_test_type type;

		glw::GLuint fs_id;
		glw::GLuint po_id;
		glw::GLuint tcs_id;
		glw::GLuint tes_id;
		glw::GLuint vs_id;

		unsigned int input_patch_size;

		_test_descriptor()
		{
			fs_id  = 0;
			po_id  = 0;
			tcs_id = 0;
			tes_id = 0;
			vs_id  = 0;

			input_patch_size = 0;
			type			 = TESSELLATION_TEST_TYPE_COUNT;
		}
	} _test_descriptor;

	typedef std::vector<_test_descriptor> _tests;
	typedef _tests::const_iterator		  _tests_const_iterator;

	/* Private methods */
	void deinitTestDescriptor(_test_descriptor* test_ptr);
	void initTestDescriptor(_tessellation_test_type test_type, _test_descriptor* out_test_ptr,
							unsigned int input_patch_size);

	/* Private variables */
	glw::GLint m_gl_max_patch_vertices_value;

	glw::GLuint m_bo_id;
	_tests		m_tests;
	glw::GLuint m_vao_id;
};

} // namespace glcts

#endif // _ESEXTCTESSELLATIONSHADERTCTE_HPP
