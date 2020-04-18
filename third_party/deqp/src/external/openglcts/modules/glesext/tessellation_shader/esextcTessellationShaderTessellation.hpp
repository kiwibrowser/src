#ifndef _ESEXTCTESSELLATIONSHADERTESSELLATION_HPP
#define _ESEXTCTESSELLATIONSHADERTESSELLATION_HPP
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
/** A DEQP CTS test group that collects all tests that verify tessellation
 *  functionality for multiple primitive modes at once, as opposed to some
 *  other tests that are mode-specific.
 */
class TessellationShaderTessellationTests : public glcts::TestCaseGroupBase
{
public:
	/* Public methods */
	TessellationShaderTessellationTests(glcts::Context& context, const ExtParameters& extParams);

	virtual ~TessellationShaderTessellationTests(void)
	{
	}

	void init(void);

private:
	/* Private methods */
	TessellationShaderTessellationTests(const TessellationShaderTessellationTests& other);
	TessellationShaderTessellationTests& operator=(const TessellationShaderTessellationTests& other);
};

/** Implementation of Test Case 24
 *
 *  Make sure that patches, for which relevant outer tessellation levels have
 *  been defined to zero or less, are discarded by the tessellation
 *  primitive generator. Confirm that such patches never reach tessellation
 *  evaluation shader program.
 *  Cover all three tessellation primitive generator modes (triangles, quads,
 *  isolines).
 *  Note that an assumption was made here that TE's primitive id counter
 *  works on output patches that are generated afresh from data fed by TC,
 *  meaning XFBed TE-stage gl_PrimitiveID should be a sequential set, and
 *  XFBed TC-stage gl_PrimitiveID should be missing every 4th and 6th patch
 *  vertices.
 *  This is backed by http://www.khronos.org/bugzilla/show_bug.cgi?id=754
 *
 *  Technical details:
 *
 *  0. If (gl_PrimitiveID % 4) == 0, TC should set all relevant outer
 *     tessellation levels to 0.
 *  1. If (gl_PrimitiveID % 4) == 2, TC should set all relevant outer
 *     tessellation level to -1.
 *  2. If (gl_PrimitiveID % 4) == 1 OR (gl_PrimitiveID % 4) == 3, TC should
 *     set all relevant outer tessellation levels to 1.
 *  3. Inner tessellation level should always be set to 1.
 *  4. TC should also set a per-vertex output variable to gl_PrimitiveID
 *     value.
 *  5. TC should also pass gl_PrimitiveID to TE.
 *  6. TE should store both pieces of data using Transform Feedback for each
 *     patch vertex processed. Test passes if the data retrieved is valid.
 *
 *
 **/
class TessellationShaderTessellationInputPatchDiscard : public TestCaseBase
{
public:
	/* Public methods */
	TessellationShaderTessellationInputPatchDiscard(Context& context, const ExtParameters& extParams);

	virtual ~TessellationShaderTessellationInputPatchDiscard(void)
	{
	}

	virtual void		  deinit(void);
	void				  initTest(void);
	virtual IterateResult iterate(void);

private:
	/* Private type definitions */
	/** Defines a single test pass */
	typedef struct _run
	{
		glw::GLuint					 po_id;
		_tessellation_primitive_mode primitive_mode;
		glw::GLuint					 tc_id;
		glw::GLuint					 te_id;

		_run()
		{
			po_id		   = 0;
			primitive_mode = TESSELLATION_SHADER_PRIMITIVE_MODE_UNKNOWN;
			tc_id		   = 0;
			te_id		   = 0;
		}
	} _run;

	/** Defines a vector of test passes */
	typedef std::vector<_run>	 _runs;
	typedef _runs::const_iterator _runs_const_iterator;

	/* Private methods */
	std::string getTCCode();
	std::string getTECode(_tessellation_primitive_mode primitive_mode);

	void deinitRun(_run& test);
	void initRun(_run& test, _tessellation_primitive_mode primitive_mode);

	/* Private variables */
	glw::GLuint m_bo_id;
	glw::GLuint m_fs_id;
	glw::GLuint m_vs_id;
	glw::GLuint m_vao_id;

	_runs					 m_runs;
	TessellationShaderUtils* m_utils_ptr;
};

/**  Implementation for Test Case 18
 *
 *   Make sure that tessellation control shader is fed with correct gl_InvocationID
 *   values.
 *   Make sure that tessellation control and tessellation evaluation shaders are
 *   fed with correct gl_PatchVerticesIn values.
 *   Make sure that tessellation control and tessellation evaluation shaders are
 *   fed with correct gl_PrimitiveID values. Make sure restarting a primitive
 *   topology does not restart primitive counter.
 *
 *   Technical details:
 *
 *   0. The test to be executed for all three geometry types supported by
 *      the tessellator. The draw calls used should draw at least a few
 *      instances of a set of patches generating a few primitives for each type
 *      considered. Vertex arrayed and indiced draw calls should be tested.
 *      A few vertices-per-patch configurations should be considered.
 *
 *   1. Tessellation control shader to pass gl_InvocationID to tessellation
 *      evaluation shader for XFB, for later inspection. The values captured
 *      should run from 0 to the last invocation number for particular draw
 *      call. Whole range should be covered by exactly one appearance of each index.
 *
 *   2. Tessellation control shader should pass gl_PatchVerticesIn value to
 *      tessellation evaluation shader. The value passed from TC, as well as
 *      gl_PatchVerticesIn value exposed to TE should be captured for later
 *      inspection.
 *
 *   3. Step 2 should be repeated for gl_PrimitiveID. The test should confirm
 *      that using a primitive restart index does not reset the counter, when
 *      indiced draw calls are tested.
 **/
class TessellationShaderTessellationgl_InvocationID_PatchVerticesIn_PrimitiveID : public TestCaseBase
{
public:
	/* Public methods */
	TessellationShaderTessellationgl_InvocationID_PatchVerticesIn_PrimitiveID(Context&			   context,
																			  const ExtParameters& extParams);

	virtual ~TessellationShaderTessellationgl_InvocationID_PatchVerticesIn_PrimitiveID(void)
	{
	}

	virtual void		  deinit(void);
	void				  initTest(void);
	virtual IterateResult iterate(void);

private:
	/* Private type definitions */
	/** Defines a single test run */
	typedef struct _run
	{
		glw::GLuint					 bo_indices_id;
		unsigned int				 drawcall_count_multiplier;
		bool						 drawcall_is_indiced;
		glw::GLuint					 drawcall_n_instances;
		glw::GLuint					 n_instances;
		glw::GLint					 n_patch_vertices;
		unsigned int				 n_restart_indices;
		unsigned int				 n_result_vertices;
		glw::GLuint					 po_id;
		_tessellation_primitive_mode primitive_mode;
		glw::GLuint					 tc_id;
		glw::GLuint					 te_id;

		_run()
		{
			bo_indices_id			  = 0;
			drawcall_count_multiplier = 0;
			drawcall_is_indiced		  = false;
			drawcall_n_instances	  = 0;
			n_result_vertices		  = 0;
			n_instances				  = 0;
			n_patch_vertices		  = 0;
			n_restart_indices		  = 0;
			po_id					  = 0;
			primitive_mode			  = TESSELLATION_SHADER_PRIMITIVE_MODE_UNKNOWN;
			tc_id					  = 0;
			te_id					  = 0;
		}
	} _run;

	/** Defines a vector of test runs */
	typedef std::vector<_run>	 _runs;
	typedef _runs::const_iterator _runs_const_iterator;

	/* Private methods */
	std::string getTCCode(glw::GLuint n_patch_vertices);
	std::string getTECode(_tessellation_primitive_mode primitive_mode);

	void deinitRun(_run& run);

	void initRun(_run& run, _tessellation_primitive_mode primitive_mode, glw::GLint n_patch_vertices, bool is_indiced,
				 glw::GLint n_instances, unsigned int drawcall_count_multiplier);

	/* Private variables */
	glw::GLuint m_bo_id;
	glw::GLuint m_fs_id;
	glw::GLuint m_vs_id;
	glw::GLuint m_vao_id;

	_runs					 m_runs;
	TessellationShaderUtils* m_utils_ptr;
};

/** Implementation of Test Case 51
 *
 *  Make sure that coordinates of all triangles generated by fixed-function
 *  tessellation primitive generator meet the barycentric coordinate requirement
 *                            u + v + w = 1
 *
 *  Consider a few inner/outer tessellation level combinations
 *  for triangle and quad inputs of a tessellation evaluation shader.
 *
 *  Epsilon: 1e-5. This is dictated by the language in ES 3.0 specification,
 *  which seems to be the best pick, given that the tessellator is
 *  a fixed-function unit.
 *
 *  Make sure that gl_TessCoord is not an array.
 *  Make sure that gl_TessCoord is not accessible for any of the vertices in
 *  gl_in[].
 *  Make sure that (u, v, w) coordinates are in range [0, 1] for all
 *  tessellation primitive modes.
 *  Make sure that the w coordinate is always zero for "quads" and "isolines"
 *  tessellation primitive modes.
 *
 *  This test should be executed in two invocations, depending on the test type:
 *
 *  * Without a tessellation control shader, where the patch tessellation levels
 *    are configured by using glPatchParameterfv() function (*);
 *  * With a tessellation control shader used to configure the levels;
 *
 *  (*) Only applies to Desktop
 *
 **/
class TessellationShaderTessellationgl_TessCoord : public TestCaseBase
{
	static std::string getTypeName(_tessellation_test_type test_type);

public:
	/* Public methods */
	TessellationShaderTessellationgl_TessCoord(Context& context, const ExtParameters& extParams,
											   _tessellation_test_type test_type);

	virtual ~TessellationShaderTessellationgl_TessCoord(void)
	{
	}

	virtual void		  deinit(void);
	void				  initTest(void);
	virtual IterateResult iterate(void);

private:
	/* Private type definitions */
	/** Defines a single test pass */
	typedef struct _test_descriptor
	{
		glw::GLint							n_patch_vertices;
		glw::GLuint							po_id;
		_tessellation_primitive_mode		primitive_mode;
		glw::GLuint							tc_id;
		glw::GLuint							te_id;
		glw::GLfloat						tess_level_inner[2];
		glw::GLfloat						tess_level_outer[4];
		_tessellation_test_type				type;
		_tessellation_shader_vertex_spacing vertex_spacing;

		glw::GLint inner_tess_level_uniform_location;
		glw::GLint outer_tess_level_uniform_location;

		_test_descriptor()
		{
			memset(tess_level_inner, 0, sizeof(tess_level_inner));
			memset(tess_level_outer, 0, sizeof(tess_level_outer));

			n_patch_vertices = 0;
			po_id			 = 0;
			primitive_mode   = TESSELLATION_SHADER_PRIMITIVE_MODE_UNKNOWN;
			tc_id			 = 0;
			te_id			 = 0;
			type			 = TESSELLATION_TEST_TYPE_UNKNOWN;
			vertex_spacing   = TESSELLATION_SHADER_VERTEX_SPACING_UNKNOWN;

			inner_tess_level_uniform_location = -1;
			outer_tess_level_uniform_location = -1;
		}
	} _test_descriptor;

	/** Defines a vector of test passes */
	typedef std::vector<_test_descriptor> _tests;
	typedef _tests::const_iterator		  _tests_const_iterator;

	/* Private methods */
	std::string getTCCode(glw::GLint n_patch_vertices);

	std::string getTECode(_tessellation_shader_vertex_spacing vertex_spacing,
						  _tessellation_primitive_mode		  primitive_mode);

	void deinitTestDescriptor(_test_descriptor& test);

	void initTestDescriptor(_test_descriptor& test, _tessellation_shader_vertex_spacing vertex_spacing,
							_tessellation_primitive_mode primitive_mode, glw::GLint n_patch_vertices,
							const float* inner_tess_levels, const float* outer_tess_levels,
							_tessellation_test_type test_type);

	/* Private variables */
	_tessellation_test_type m_test_type;
	glw::GLuint				m_bo_id;
	glw::GLuint				m_broken_ts_id;
	glw::GLuint				m_fs_id;
	glw::GLuint				m_vs_id;
	glw::GLuint				m_vao_id;

	_tests					 m_tests;
	TessellationShaderUtils* m_utils_ptr;
};

/* This test class implements the following test cases:
 *
 *   20. Make sure it is possible to use up to GL_MAX_TESS_CONTROL_INPUT_COMPONENTS_EXT/4
 *       vec4 input variables in a tessellation control shader.
 *       Make sure it is possible to use up to GL_MAX_TESS_EVALUATION_INPUT_COMPONENTS_EXT/4
 *       vec4 input variables in a tessellation evaluation shader.
 *       This test should issue at least one draw call and verify the results to
 *       make sure the implementation actually supports the maximums it reports.
 *
 *   21. Make sure it is possible to use up to GL_MAX_TESS_CONTROL_OUTPUT_COMPONENTS_EXT/4
 *       vec4 per-vertex output variables in a tessellation control shader.
 *       Make sure it is possible to use up to GL_MAX_TESS_PATCH_COMPONENTS_EXT/4
 *       vec4 per-patch output variables in a tessellation control shader.
 *       Make sure it is possible to use up to GL_MAX_TESS_PATCH_COMPONENTS_EXT/4
 *       vec4 per-patch input variables in a tessellation evaluation shader.
 *       Make sure it is possible to use up to GL_MAX_TESS_EVALUATION_OUTPUT_COMPONENTS_EXT/4
 *       vec4 per-vertex output variables in a tessellation evaluation shader.
 *
 *       NOTE: The test should separately check if a maximum number of per-vertex and
 *             per-patch output variables used in a tessellation control shader works
 *             correctly. This is due to a risk that, had both types been used at once,
 *             the maximum amount of output components supported for tessellation
 *             control shader GL_MAX_TESS_CONTROL_TOTAL_OUTPUT_COMPONENTS_EXT may
 *             have been exceeded for implementations, for which the property is
 *             not equal to:
 *
 *             (per-vertex output component count multiplied by output patch size +
 *              + per-patch output component count).
 *
 *       This test should issue at least one draw call and verify the results to
 *       make sure the implementation actually supports the maximums it reports.
 *
 *       Category: Functional Test.
 *       Priority: Must-Have
 *
 *  The test is implemented by checking two different cases:
 *  1) In first case, it makes sure it is possible to use up to:
 *     - GL_MAX_TESS_CONTROL_INPUT_COMPONENTS_EXT     / 4 vec4 per-vertex variables in tessellation control shader,
 *     - GL_MAX_TESS_CONTROL_OUTPUT_COMPONENTS_EXT    / 4 vec4 per-vertex variables in tessellation control shader,
 *     - GL_MAX_TESS_EVALUATION_INPUT_COMPONENTS_EXT  / 4 vec4 per-vertex variables in tessellation evaluation shader,
 *     - GL_MAX_TESS_EVALUATION_OUTPUT_COMPONENTS_EXT / 4 vec4 per-vertex variables in tessellation evaluation shader.
 *
 *  2) In second case, it makes sure it is possible to use up to:
 *     - GL_MAX_TESS_PATCH_COMPONENTS_EXT/4 vec4 per-patch variables in tessellation control shader,
 *     - GL_MAX_TESS_PATCH_COMPONENTS_EXT/4 vec4 per-patch variables in tessellation evaluation shader.
 */
class TessellationShaderTessellationMaxInOut : public TestCaseBase
{
public:
	/* Public methods */
	TessellationShaderTessellationMaxInOut(Context& context, const ExtParameters& extParams);

	virtual ~TessellationShaderTessellationMaxInOut()
	{
	}

	virtual void		  deinit(void);
	virtual IterateResult iterate(void);

private:
	/* private methods */
	void initBufferObjects(void);
	void initProgramObjects(void);
	void initReferenceValues(void);
	void initTest(void);
	void retrieveGLConstantValues(void);

	bool compareValues(char const* description, glw::GLfloat* reference_values, int n_reference_values);

	/* private variables */
	glw::GLuint m_po_id_1; /* program object name for case 1 */
	glw::GLuint m_po_id_2; /* program object name for case 2 */

	glw::GLuint m_fs_id;	/* fragment shader object name */
	glw::GLuint m_tcs_id_1; /* tessellation control shader object name for case 1 */
	glw::GLuint m_tcs_id_2; /* tessellation control shader object name for case 2 */
	glw::GLuint m_tes_id_1; /* tessellation evaluation shader object name for case 1 */
	glw::GLuint m_tes_id_2; /* tessellation evaluation shader object name for case 2 */
	glw::GLuint m_vs_id_1;  /* vertex shader object name for case 1 */
	glw::GLuint m_vs_id_2;  /* vertex shader object name for case 2 */

	glw::GLuint m_tf_bo_id_1;		/* buffer object name */
	glw::GLuint m_tf_bo_id_2;		/* buffer object name */
	glw::GLuint m_patch_data_bo_id; /* buffer object name for patch submission */

	glw::GLuint m_vao_id; /* vertex array object */

	glw::GLint m_gl_max_tess_control_input_components_value;	 /* value of MAX_TESS_CONTROL_INPUT_COMPONENTS */
	glw::GLint m_gl_max_tess_control_output_components_value;	/* value of MAX_TESS_CONTROL_OUTPUT_COMPONENTS */
	glw::GLint m_gl_max_tess_evaluation_input_components_value;  /* value of MAX_TESS_EVALUATION_INPUT_COMPONENTS */
	glw::GLint m_gl_max_tess_evaluation_output_components_value; /* value of MAX_TESS_EVALUATION_OUTPUT_COMPONENTS */
	glw::GLint
			   m_gl_max_transform_feedback_interleaved_components_value; /* value of MAX_TRANSFORM_FEEDBACK_INTERLEAVED_COMPONENTS */
	glw::GLint m_gl_max_tess_patch_components_value;					 /* value of MAX_TESS_PATCH_COMPONENTS */
	glw::GLint m_gl_max_vertex_output_components_value;					 /* value of MAX_VERTEX_OUTPUT_COMPONENTS */

	glw::GLfloat  m_ref_patch_attributes[4]; /* reference values for max per-patch attributes case 2 */
	glw::GLfloat* m_ref_vertex_attributes;   /* reference values for max per-vertex attributes case 1 */

	static const char* m_fs_code;	/* fragment shader code */
	static const char* m_vs_code;	/* vertex shader code */
	static const char* m_tcs_code_1; /* tessellation control shader code for per vertex components check */
	static const char* m_tcs_code_2; /* tessellation control shader code per patch components check */
	static const char* m_tes_code_1; /* tessellation evaluation shader code per vertex components check */
	static const char* m_tes_code_2; /* tessellation evaluation shader code per patch components check */

	char** m_tf_varyings_names; /* transform feedback varyings names array */
};

} // namespace glcts

#endif // _ESEXTCTESSELLATIONSHADERTESSELLATION_HPP
