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

#include "esextcTessellationShaderVertexOrdering.hpp"
#include "esextcTessellationShaderUtils.hpp"
#include "gluContextInfo.hpp"
#include "gluDefs.hpp"
#include "glwEnums.hpp"
#include "glwFunctions.hpp"
#include "tcuTestLog.hpp"

namespace glcts
{
/** Constructor
 *
 * @param context Test context
 **/
TessellationShaderVertexOrdering::TessellationShaderVertexOrdering(Context& context, const ExtParameters& extParams)
	: TestCaseBase(context, extParams, "vertex_ordering", "Verifies vertex ordering property affects the tessellation"
														  " process as per extension specification")
	, m_bo_id(0)
	, m_fs_id(0)
	, m_tc_id(0)
	, m_vs_id(0)
	, m_vao_id(0)
	, m_utils(DE_NULL)
{
	/* Left blank on purpose */
}

/** Deinitializes ES objects created for the test. */
void TessellationShaderVertexOrdering::deinit()
{
	/* Call base class' deinit() */
	TestCaseBase::deinit();

	if (!m_is_tessellation_shader_supported)
	{
		return;
	}

	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Reset TF buffer object bindings */
	gl.bindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, 0 /* buffer */);
	gl.bindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0 /* index */, 0 /* buffer */);

	/* Restore GL_PATCH_VERTICES_EXT value */
	gl.patchParameteri(m_glExtTokens.PATCH_VERTICES, 3);

	/* Disable GL_RASTERIZER_DISCARD rendering mode */
	gl.disable(GL_RASTERIZER_DISCARD);

	/* Reset active program object */
	gl.useProgram(0);

	/* Unbind vertex array object */
	gl.bindVertexArray(0);

	/* Free all ES objects we allocated for the test */
	if (m_bo_id != 0)
	{
		gl.deleteBuffers(1, &m_bo_id);

		m_bo_id = 0;
	}

	if (m_fs_id != 0)
	{
		gl.deleteShader(m_fs_id);

		m_fs_id = 0;
	}

	if (m_tc_id != 0)
	{
		gl.deleteShader(m_tc_id);

		m_tc_id = 0;
	}

	if (m_vs_id != 0)
	{
		gl.deleteShader(m_vs_id);

		m_vs_id = 0;
	}

	if (m_vao_id != 0)
	{
		gl.deleteVertexArrays(1, &m_vao_id);

		m_vao_id = 0;
	}

	/* Denitialize utils instance */
	if (m_utils != DE_NULL)
	{
		delete m_utils;

		m_utils = DE_NULL;
	}

	/* Deinitialize all test descriptors */
	_test_iterations::iterator it;
	for (it = m_tests.begin(); it != m_tests.end(); ++it)
	{
		deinitTestIteration(*it);
	}
	m_tests.clear();

	for (it = m_tests_points.begin(); it != m_tests_points.end(); ++it)
	{
		deinitTestIteration(*it);
	}
	m_tests_points.clear();
}

/** Deinitialize all test pass-specific ES objects.
 *
 *  @param test Descriptor of a test pass to deinitialize.
 **/
void TessellationShaderVertexOrdering::deinitTestIteration(_test_iteration& test_iteration)
{
	if (test_iteration.data != DE_NULL)
	{
		delete[] test_iteration.data;

		test_iteration.data = DE_NULL;
	}
}

/** Initializes ES objects necessary to run the test. */
void TessellationShaderVertexOrdering::initTest()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Skip if required extensions are not supported. */
	if (!m_is_tessellation_shader_supported)
	{
		throw tcu::NotSupportedError(TESSELLATION_SHADER_EXTENSION_NOT_SUPPORTED);
	}

	/* Initialize vertex array object */
	gl.genVertexArrays(1, &m_vao_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not generate vertex array object");

	gl.bindVertexArray(m_vao_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error binding vertex array object!");

	/* Set up patch size */
	gl.patchParameteri(m_glExtTokens.PATCH_VERTICES, 1);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glPatchParameteriEXT() call failed for GL_PATCH_VERTICES_EXT pname");

	/* Disable rasterization */
	gl.enable(GL_RASTERIZER_DISCARD);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glEnable(GL_RASTERIZER_DISCARD) call failed");

	/* Initialize utils instance */
	m_utils = new TessellationShaderUtils(gl, this);

	/* Generate all test-wide objects needed for test execution */
	gl.genBuffers(1, &m_bo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenBuffers() failed");

	m_fs_id = gl.createShader(GL_FRAGMENT_SHADER);
	m_tc_id = gl.createShader(m_glExtTokens.TESS_CONTROL_SHADER);
	m_vs_id = gl.createShader(GL_VERTEX_SHADER);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateShader() failed");

	/* Configure buffer object bindings */
	gl.bindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, m_bo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer() call failed");

	gl.bindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, /* index */
					  m_bo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBufferBase() call failed");

	/* Configure fragment shader body */
	const char* fs_body = "${VERSION}\n"
						  "\n"
						  "void main()\n"
						  "{\n"
						  "}\n";

	shaderSourceSpecialized(m_fs_id, 1 /* count */, &fs_body);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glShaderSource() failed for fragment shader");

	/* Configure tessellation control shader body */
	std::string tc_body =
		TessellationShaderUtils::getGenericTCCode(4,	  /* n_patch_vertices */
												  false); /* should_use_glInvocationID_indexed_input */
	const char* tc_body_ptr = tc_body.c_str();

	shaderSourceSpecialized(m_tc_id, 1 /* count */, &tc_body_ptr);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glShaderSource() failed for tessellation control shader");

	/* Configure vertex shader body */
	const char* vs_body = "${VERSION}\n"
						  "\n"
						  "void main()\n"
						  "{\n"
						  "    gl_Position = vec4(1.0, 0.0, 0.0, 0.0);\n"
						  "}\n";

	shaderSourceSpecialized(m_vs_id, 1 /* count */, &vs_body);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glShaderSource() failed for vertex shader");

	/* Compile all the shaders */
	const glw::GLuint  shaders[] = { m_fs_id, m_tc_id, m_vs_id };
	const unsigned int n_shaders = sizeof(shaders) / sizeof(shaders[0]);

	for (unsigned int n_shader = 0; n_shader < n_shaders; ++n_shader)
	{
		glw::GLuint shader = shaders[n_shader];

		if (shader != 0)
		{
			glw::GLint compile_status = GL_FALSE;

			gl.compileShader(shader);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glCompileShader() failed");

			gl.getShaderiv(shader, GL_COMPILE_STATUS, &compile_status);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glGetShaderiv() failed");

			if (compile_status != GL_TRUE)
			{
				TCU_FAIL("Shader compilation failed");
			}
		}
	} /* for (all shaders) */

	/* Retrieve GL_MAX_TESS_GEN_LEVEL_EXT value */
	glw::GLint gl_max_tess_gen_level_value = 0;

	gl.getIntegerv(m_glExtTokens.MAX_TESS_GEN_LEVEL, &gl_max_tess_gen_level_value);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv() failed for GL_MAX_TESS_GEN_LEVEL_EXT pname");

	/* Initialize all test iterations */
	bool			   point_mode_statuses[] = { false, true };
	const unsigned int n_point_mode_statuses = sizeof(point_mode_statuses) / sizeof(point_mode_statuses[0]);

	const _tessellation_primitive_mode primitive_modes[] = { TESSELLATION_SHADER_PRIMITIVE_MODE_ISOLINES,
															 TESSELLATION_SHADER_PRIMITIVE_MODE_QUADS,
															 TESSELLATION_SHADER_PRIMITIVE_MODE_TRIANGLES };
	const unsigned int n_primitive_modes = sizeof(primitive_modes) / sizeof(primitive_modes[0]);

	const _tessellation_shader_vertex_ordering vertex_orderings[] = { TESSELLATION_SHADER_VERTEX_ORDERING_CCW,
																	  TESSELLATION_SHADER_VERTEX_ORDERING_CW,
																	  TESSELLATION_SHADER_VERTEX_ORDERING_DEFAULT };
	const unsigned int n_vertex_orderings = sizeof(vertex_orderings) / sizeof(vertex_orderings[0]);

	for (unsigned int n_primitive_mode = 0; n_primitive_mode < n_primitive_modes; ++n_primitive_mode)
	{
		_tessellation_levels_set	 levels_set;
		_tessellation_primitive_mode primitive_mode = primitive_modes[n_primitive_mode];

		levels_set = TessellationShaderUtils::getTessellationLevelSetForPrimitiveMode(
			primitive_mode, gl_max_tess_gen_level_value,
			TESSELLATION_LEVEL_SET_FILTER_INNER_AND_OUTER_LEVELS_USE_DIFFERENT_VALUES);

		for (unsigned int n_vertex_ordering = 0; n_vertex_ordering < n_vertex_orderings; ++n_vertex_ordering)
		{
			_tessellation_shader_vertex_ordering vertex_ordering = vertex_orderings[n_vertex_ordering];

			for (_tessellation_levels_set_const_iterator levels_set_iterator = levels_set.begin();
				 levels_set_iterator != levels_set.end(); levels_set_iterator++)
			{
				const _tessellation_levels& levels = *levels_set_iterator;

				for (unsigned int n_point_mode_status = 0; n_point_mode_status < n_point_mode_statuses;
					 ++n_point_mode_status)
				{
					bool point_mode_status = point_mode_statuses[n_point_mode_status];

					/* Initialize a test run descriptor for the iteration-specific properties */
					_test_iteration test_iteration = initTestIteration(levels.inner, levels.outer, primitive_mode,
																	   vertex_ordering, point_mode_status, m_utils);

					/* Store the test iteration descriptor */
					if (!point_mode_status)
					{
						m_tests.push_back(test_iteration);
					}
					else
					{
						m_tests_points.push_back(test_iteration);
					}
				} /* for (all point mode statuses) */
			}	 /* for (all level sets) */
		}		  /* for (all vertex orderings) */
	}			  /* for (all primitive modes) */

	/* Set up buffer object bindings */
	gl.bindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, m_bo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer() failed");

	gl.bindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0 /* index */, m_bo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBufferBase() failed");
}

/** Initializes all ES objects, runs the test, captures all vertices
 *  generated by the tessellator and stores them in the result
 *  descriptor.
 *
 *  NOTE: This function throws a TestError exception, should an error occur.
 *
 *  @param inner_tess_levels     Two FP values describing inner tessellation level values to be used
 *                               for the test run. Must not be NULL.
 *  @param outer_tess_levels     Four FP values describing outer tessellation level values to be used
 *                               for the test run. Must not be NULL.
 *  @param primitive_mode        Primitive mode to be used for the test run.
 *  @param vertex_ordering       Vertex ordering to be used for the test run.
 *  @param is_point_mode_enabled true if points mode should be used for the test run, false otherwise.
 *
 *  @return _test_iteration instance containing all described data.
 *
 **/
TessellationShaderVertexOrdering::_test_iteration TessellationShaderVertexOrdering::initTestIteration(
	const float* inner_tess_levels, const float* outer_tess_levels, _tessellation_primitive_mode primitive_mode,
	_tessellation_shader_vertex_ordering vertex_ordering, bool is_point_mode_enabled, TessellationShaderUtils* utils)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();
	_test_iteration		  test_iteration;

	/* Create & configure a tessellation evaluation shader for the iteration */
	const std::string te_code = TessellationShaderUtils::getGenericTECode(
		TESSELLATION_SHADER_VERTEX_SPACING_EQUAL, primitive_mode, vertex_ordering, is_point_mode_enabled);
	const char* te_code_ptr = te_code.c_str();

	glw::GLuint te_id = gl.createShader(m_glExtTokens.TESS_EVALUATION_SHADER);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateShader() call failed for GL_TESS_EVALUATION_SHADER_EXT pname");

	shaderSourceSpecialized(te_id, 1, /* count */
							&te_code_ptr);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glShaderSource() call failed");

	utils->compileShaders(1,			 /* n_shaders */
						  &te_id, true); /* should_succeed */

	/* Create & form iteration-specific program object */
	glw::GLuint po_id = gl.createProgram();
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateProgram() call failed");

	gl.attachShader(po_id, m_fs_id);
	gl.attachShader(po_id, m_tc_id);
	gl.attachShader(po_id, te_id);
	gl.attachShader(po_id, m_vs_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glAttachShader() call(s) failed");

	/* Set up XFB */
	const char* varyings[] = { "result_uvw" };

	gl.transformFeedbackVaryings(po_id, 1, /* count */
								 varyings, GL_INTERLEAVED_ATTRIBS);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTransformFeedbackVaryings() call failed");

	/* Link the program object */
	glw::GLint link_status = GL_FALSE;

	gl.linkProgram(po_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glLinkProgram() call failed");

	gl.getProgramiv(po_id, GL_LINK_STATUS, &link_status);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetProgramiv() call failed");

	if (link_status != GL_TRUE)
	{
		TCU_FAIL("Program linking failed");
	}

	gl.deleteShader(te_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glDeleteShader() call failed");

	/* Fill the remaining test iteration descriptor fields */
	memcpy(test_iteration.inner_tess_levels, inner_tess_levels, sizeof(test_iteration.inner_tess_levels));
	memcpy(test_iteration.outer_tess_levels, outer_tess_levels, sizeof(test_iteration.outer_tess_levels));

	test_iteration.is_point_mode_enabled = is_point_mode_enabled;
	test_iteration.primitive_mode		 = primitive_mode;
	test_iteration.vertex_ordering		 = vertex_ordering;
	test_iteration.n_vertices			 = m_utils->getAmountOfVerticesGeneratedByTessellator(
		primitive_mode, inner_tess_levels, outer_tess_levels, TESSELLATION_SHADER_VERTEX_SPACING_EQUAL,
		is_point_mode_enabled);

	/* Configure the buffer object storage to hold required amount of data */
	glw::GLuint bo_size = static_cast<glw::GLuint>(test_iteration.n_vertices * 3 /* components */ * sizeof(float));

	gl.bufferData(GL_TRANSFORM_FEEDBACK_BUFFER, bo_size, DE_NULL, /* data */
				  GL_STATIC_DRAW);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferData() call failed");

	/* Also configure the storage in the descriptor */
	test_iteration.data = new char[bo_size];

	/* Render the data set */
	glw::GLint  inner_tess_level_uniform_location = gl.getUniformLocation(po_id, "inner_tess_level");
	glw::GLint  outer_tess_level_uniform_location = gl.getUniformLocation(po_id, "outer_tess_level");
	glw::GLenum tf_mode = TessellationShaderUtils::getTFModeForPrimitiveMode(primitive_mode, is_point_mode_enabled);

	DE_ASSERT(inner_tess_level_uniform_location != -1);
	DE_ASSERT(outer_tess_level_uniform_location != -1);

	gl.useProgram(po_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram() call failed");

	gl.uniform2fv(inner_tess_level_uniform_location, 1 /* count */, test_iteration.inner_tess_levels);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUniform2fv() call failed");

	gl.uniform4fv(outer_tess_level_uniform_location, 1 /* count */, test_iteration.outer_tess_levels);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUniform4fv() call failed");

	gl.beginTransformFeedback(tf_mode);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBeginTransformFeedback() call failed");

	gl.drawArrays(m_glExtTokens.PATCHES, 0 /* first */, 1 /* count */);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawArrays() call failed");

	gl.endTransformFeedback();
	GLU_EXPECT_NO_ERROR(gl.getError(), "glEndTransformFeedback() call failed");

	/* Map the XFB buffer object and copy the rendered data */
	const float* xfb_data = (const float*)gl.mapBufferRange(GL_TRANSFORM_FEEDBACK_BUFFER, 0, /* offset */
															bo_size, GL_MAP_READ_BIT);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glMapBufferRange() call failed");

	memcpy(test_iteration.data, xfb_data, bo_size);

	/* Unmap the buffer object, now that we're done retrieving the captured data */
	gl.unmapBuffer(GL_TRANSFORM_FEEDBACK_BUFFER);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUnmapBuffer() call failed");

	gl.deleteProgram(po_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glDeleteProgram() call failed");

	return test_iteration;
}

/** Executes the test.
 *
 *  Sets the test result to QP_TEST_RESULT_FAIL if the test failed, QP_TEST_RESULT_PASS otherwise.
 *
 *  Note the function throws exception should an error occur!
 *
 *  @return STOP if the test has finished, CONTINUE to indicate iterate() should be called once again.
 **/
tcu::TestNode::IterateResult TessellationShaderVertexOrdering::iterate(void)
{
	initTest();

	/* Do not execute if required extensions are not supported. */
	if (!m_is_tessellation_shader_supported)
	{
		throw tcu::NotSupportedError(TESSELLATION_SHADER_EXTENSION_NOT_SUPPORTED);
	}

	/* There are two main separate cases to consider here:
	 *
	 * a) for runs executed in "points" mode, we need to verify that vertex
	 *    ordering does not modify the order in which the points are generated.
	 * b) for both run types, for all primitives but isolines we need to verify
	 *    that the vertex ordering is actually taken into account.
	 */
	const float epsilon = 1e-5f;

	for (_test_iterations_const_iterator test_iterator = m_tests.begin(); test_iterator != m_tests.end();
		 test_iterator++)
	{
		if (test_iterator->primitive_mode != TESSELLATION_SHADER_PRIMITIVE_MODE_ISOLINES)
		{
			verifyVertexOrderingCorrectness(*test_iterator);
		}
	} /* for (all non-points runs) */

	for (_test_iterations_const_iterator test_iterator = m_tests_points.begin(); test_iterator != m_tests_points.end();
		 test_iterator++)
	{
		const _test_iteration& test = *test_iterator;

		/* For points_mode checks, we need to find a corresponding cw+ccw test pairs */
		if (test.vertex_ordering == TESSELLATION_SHADER_VERTEX_ORDERING_CCW ||
			test.vertex_ordering == TESSELLATION_SHADER_VERTEX_ORDERING_DEFAULT)
		{
/* Find a corresponding CW test descriptor */
#if defined(DE_DEBUG) && !defined(DE_COVERAGE_BUILD)
			bool has_paired_test_been_found = false;
#endif
			_test_iteration paired_test;

			for (_test_iterations_const_iterator paired_test_iterator = m_tests_points.begin();
				 paired_test_iterator != m_tests_points.end(); ++paired_test_iterator)
			{
				if (de::abs(paired_test_iterator->inner_tess_levels[0] - test_iterator->inner_tess_levels[0]) <
						epsilon &&
					de::abs(paired_test_iterator->inner_tess_levels[1] - test_iterator->inner_tess_levels[1]) <
						epsilon &&
					de::abs(paired_test_iterator->outer_tess_levels[0] - test_iterator->outer_tess_levels[0]) <
						epsilon &&
					de::abs(paired_test_iterator->outer_tess_levels[1] - test_iterator->outer_tess_levels[1]) <
						epsilon &&
					de::abs(paired_test_iterator->outer_tess_levels[2] - test_iterator->outer_tess_levels[2]) <
						epsilon &&
					de::abs(paired_test_iterator->outer_tess_levels[3] - test_iterator->outer_tess_levels[3]) <
						epsilon &&
					paired_test_iterator->n_vertices == test_iterator->n_vertices &&
					paired_test_iterator->primitive_mode == test_iterator->primitive_mode &&
					paired_test_iterator->vertex_ordering == TESSELLATION_SHADER_VERTEX_ORDERING_CW)
				{
#if defined(DE_DEBUG) && !defined(DE_COVERAGE_BUILD)
					has_paired_test_been_found = true;
#endif
					paired_test = *paired_test_iterator;

					break;
				}
			}

			DE_ASSERT(has_paired_test_been_found);

			/* Good to call the verification routine */
			verifyVertexOrderingDoesNotChangeGeneratedPoints(test, paired_test);
		} /* if (base test 's vertex ordering is CCW) */
	}	 /* for (all other runs) */

	/* All done */
	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	return STOP;
}

/** Verifies that vertex ordering in the data set stored in user-provided
 *  test iteration descriptor matches the setting that was used in the
 *  tessellation evaluation stage.
 *
 *  @param test_iteration Test iteration descriptor
 *
 **/
void TessellationShaderVertexOrdering::verifyVertexOrderingCorrectness(const _test_iteration& test_iteration)
{
	/* Sanity check */
	DE_ASSERT(test_iteration.primitive_mode != TESSELLATION_SHADER_PRIMITIVE_MODE_ISOLINES);

	/* Iterate through all vertices */
	const float		   epsilon					= 1e-5f;
	const unsigned int n_vertices_per_primitive = 3;

	for (unsigned int n_primitive = 0; n_primitive < test_iteration.n_vertices / n_vertices_per_primitive;
		 ++n_primitive)
	{
		const float* primitive_data =
			(const float*)test_iteration.data + 3 /* components */ * n_primitive * n_vertices_per_primitive;
		const float* primitive_vertex1_data = primitive_data;
		const float* primitive_vertex2_data = primitive_vertex1_data + 3; /* components */
		const float* primitive_vertex3_data = primitive_vertex2_data + 3; /* components */

		float cartesian_vertex_data[6] = { primitive_vertex1_data[0], primitive_vertex1_data[1],
										   primitive_vertex2_data[0], primitive_vertex2_data[1],
										   primitive_vertex3_data[0], primitive_vertex3_data[1] };

		if (test_iteration.primitive_mode == TESSELLATION_SHADER_PRIMITIVE_MODE_TRIANGLES)
		{
			/* Triangles are described in barycentric coordinate. Convert to
			 * cartesian coordinates before we continue with actual test.
			 */
			const float barycentric_vertex_data[] = {
				primitive_vertex1_data[0], primitive_vertex1_data[1], primitive_vertex1_data[2],
				primitive_vertex2_data[0], primitive_vertex2_data[1], primitive_vertex2_data[2],
				primitive_vertex3_data[0], primitive_vertex3_data[1], primitive_vertex3_data[2],
			};

			/* Sanity checks .. */
			DE_UNREF(epsilon);
			DE_ASSERT(de::abs(barycentric_vertex_data[0] + barycentric_vertex_data[1] + barycentric_vertex_data[2] -
							  1.0f) < epsilon);
			DE_ASSERT(de::abs(barycentric_vertex_data[3] + barycentric_vertex_data[4] + barycentric_vertex_data[5] -
							  1.0f) < epsilon);
			DE_ASSERT(de::abs(barycentric_vertex_data[6] + barycentric_vertex_data[7] + barycentric_vertex_data[8] -
							  1.0f) < epsilon);

			for (unsigned int n_vertex = 0; n_vertex < 3; ++n_vertex)
			{
				TessellationShaderUtils::convertBarycentricCoordinatesToCartesian(
					barycentric_vertex_data + n_vertex * 3, cartesian_vertex_data + n_vertex * 2);
			}
		} /* if (test_iteration.primitive_mode == TESSELLATION_SHADER_PRIMITIVE_MODE_TRIANGLES) */

		/* Compute result of eq 3.6.1 */
		float determinant = 0.0f;

		for (unsigned int n_vertex = 0; n_vertex < n_vertices_per_primitive; ++n_vertex)
		{
			int i_op_1 = (n_vertex + 1) % n_vertices_per_primitive;

			determinant += (cartesian_vertex_data[n_vertex * 2 /* components */ + 0] *
								cartesian_vertex_data[i_op_1 * 2 /* components */ + 1] -
							cartesian_vertex_data[i_op_1 * 2 /* components */ + 0] *
								cartesian_vertex_data[n_vertex * 2 /* components */ + 1]);
		} /* for (all vertices) */

		determinant *= 0.5f;

		/* Positive determinant implies counterclockwise ordering */
		if (((test_iteration.vertex_ordering == TESSELLATION_SHADER_VERTEX_ORDERING_CCW ||
			  test_iteration.vertex_ordering == TESSELLATION_SHADER_VERTEX_ORDERING_DEFAULT) &&
			 determinant < 0.0f) ||
			(test_iteration.vertex_ordering == TESSELLATION_SHADER_VERTEX_ORDERING_CW && determinant >= 0.0f))
		{
			std::string primitive_mode =
				TessellationShaderUtils::getESTokenForPrimitiveMode(test_iteration.primitive_mode);
			std::string vertex_ordering =
				TessellationShaderUtils::getESTokenForVertexOrderingMode(test_iteration.vertex_ordering);

			m_testCtx.getLog() << tcu::TestLog::Message << "For primitive mode: [" << primitive_mode.c_str()
							   << "] "
								  "and inner tessellation levels:"
								  " ["
							   << test_iteration.inner_tess_levels[0] << ", " << test_iteration.inner_tess_levels[1]
							   << "] "
								  "and outer tessellation levels:"
								  " ["
							   << test_iteration.outer_tess_levels[0] << ", " << test_iteration.outer_tess_levels[1]
							   << ", " << test_iteration.outer_tess_levels[2] << ", "
							   << test_iteration.outer_tess_levels[3] << "] "
							   << "and vertex ordering: [" << vertex_ordering.c_str()
							   << "] "
								  ", vertex orientation has been found to be incompatible with the ordering requested."
							   << tcu::TestLog::EndMessage;

			if (test_iteration.vertex_ordering == TESSELLATION_SHADER_VERTEX_ORDERING_CCW ||
				test_iteration.vertex_ordering == TESSELLATION_SHADER_VERTEX_ORDERING_DEFAULT)
			{
				TCU_FAIL("Counter-clockwise ordering was expected but retrieved tessellation coordinates are laid out "
						 "in clockwise order");
			}
			else
			{
				TCU_FAIL("Clockwise ordering was expected but retrieved tessellation coordinates are laid out in "
						 "counter-clockwise order");
			}
		}
	} /* for (all triangles) */
}

/** Verifies that vertices generated by the tessellator do not differ when run for exactly
 *  the same tessellation evaluation shaders configure to run in point mode, with an exception
 *  that one invokation used CW ordering and the other one used CCW ordering.
 *
 *  Note: this function throws a TestError exception, should an error occur.
 *
 *  @param test_iteration_a Test iteration which was run in point mode and uses CCW vertex
 *                          ordering.
 *  @param test_iteration_b Test iteration which was run in point mode and uses CW vertex
 *                          ordering.
 *
 **/
void TessellationShaderVertexOrdering::verifyVertexOrderingDoesNotChangeGeneratedPoints(
	const _test_iteration& test_iteration_a, const _test_iteration& test_iteration_b)
{
	const float epsilon = 1e-5f;

	/* Sanity checks */
	DE_ASSERT(test_iteration_a.is_point_mode_enabled);
	DE_ASSERT(test_iteration_b.is_point_mode_enabled);
	DE_ASSERT(test_iteration_a.vertex_ordering == TESSELLATION_SHADER_VERTEX_ORDERING_CCW ||
			  test_iteration_a.vertex_ordering == TESSELLATION_SHADER_VERTEX_ORDERING_DEFAULT);
	DE_ASSERT(test_iteration_b.vertex_ordering == TESSELLATION_SHADER_VERTEX_ORDERING_CW);

	/* Iterate through all points in test set A and make sure they can be found in test set B */
	for (unsigned int n_vertex_a = 0; n_vertex_a < test_iteration_a.n_vertices; ++n_vertex_a)
	{
		bool		 has_been_found = false;
		const float* vertex_a_data  = (const float*)test_iteration_a.data + n_vertex_a * 3 /* components */;

		for (unsigned int n_vertex_b = 0; n_vertex_b < test_iteration_b.n_vertices; ++n_vertex_b)
		{
			const float* vertex_b_data = (const float*)test_iteration_b.data + n_vertex_b * 3 /* components */;

			if (de::abs(vertex_a_data[0] - vertex_b_data[0]) < epsilon &&
				de::abs(vertex_a_data[1] - vertex_b_data[1]) < epsilon &&
				de::abs(vertex_a_data[2] - vertex_b_data[2]) < epsilon)
			{
				has_been_found = true;

				break;
			}
		} /* for (all B set vertices) */

		if (!has_been_found)
		{
			std::string primitive_mode =
				TessellationShaderUtils::getESTokenForPrimitiveMode(test_iteration_a.primitive_mode);
			std::string vertex_ordering =
				TessellationShaderUtils::getESTokenForVertexOrderingMode(test_iteration_a.vertex_ordering);

			m_testCtx.getLog() << tcu::TestLog::Message << "For primitive mode: [" << primitive_mode.c_str()
							   << "] "
								  "and inner tessellation levels:"
								  " ["
							   << test_iteration_a.inner_tess_levels[0] << ", " << test_iteration_a.inner_tess_levels[1]
							   << "] "
								  "and outer tessellation levels:"
								  " ["
							   << test_iteration_a.outer_tess_levels[0] << ", " << test_iteration_a.outer_tess_levels[1]
							   << ", " << test_iteration_a.outer_tess_levels[2] << ", "
							   << test_iteration_a.outer_tess_levels[3] << "] "
							   << ", vertices generated for CW and CCW orientations do not match."
							   << tcu::TestLog::EndMessage;

			TCU_FAIL("For runs in which only vertex ordering setting differs, vertex from one run was not found in the "
					 "other run.");
		}
	} /* for (all A set vertices) */
}

} /* namespace glcts */
