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

#include "esextcTessellationShaderQuads.hpp"
#include "gluContextInfo.hpp"
#include "gluDefs.hpp"
#include "glwEnums.hpp"
#include "glwFunctions.hpp"
#include "tcuTestLog.hpp"
#include <algorithm>

namespace glcts
{

/** Constructor
 *
 * @param context Test context
 **/
TessellationShaderQuadsTests::TessellationShaderQuadsTests(glcts::Context& context, const ExtParameters& extParams)
	: TestCaseGroupBase(context, extParams, "tessellation_shader_quads_tessellation",
						"Verifies quad tessellation functionality")
{
	/* No implementation needed */
}

/**
 * Initializes test groups for geometry shader tests
 **/
void TessellationShaderQuadsTests::init(void)
{
	addChild(new glcts::TessellationShaderQuadsDegenerateCase(m_context, m_extParams));
	addChild(new glcts::TessellationShaderQuadsInnerTessellationLevelRounding(m_context, m_extParams));
}

/** Constructor
 *
 * @param context Test context
 **/
TessellationShaderQuadsDegenerateCase::TessellationShaderQuadsDegenerateCase(Context&			  context,
																			 const ExtParameters& extParams)
	: TestCaseBase(context, extParams, "degenerate_case",
				   "Verifies that only a single triangle pair covering the outer rectangle"
				   " is generated, if both clamped inner and outer tessellation levels are "
				   "set to one.")
	, m_vao_id(0)
	, m_utils(DE_NULL)
{
	/* Left blank on purpose */
}

/** Deinitializes ES objects created for the test. */
void TessellationShaderQuadsDegenerateCase::deinit()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Call base class' deinit() */
	TestCaseBase::deinit();

	/* Unbind vertex array object */
	gl.bindVertexArray(0);

	/* Deinitialize utils instance */
	if (m_utils != DE_NULL)
	{
		delete m_utils;

		m_utils = DE_NULL;
	}

	/* Delete vertex array object */
	if (m_vao_id != 0)
	{
		gl.deleteVertexArrays(1, &m_vao_id);

		m_vao_id = 0;
	}
}

/** Initializes ES objects necessary to run the test. */
void TessellationShaderQuadsDegenerateCase::initTest()
{
	/* Skip if required extensions are not supported. */
	if (!m_is_tessellation_shader_supported)
	{
		throw tcu::NotSupportedError(TESSELLATION_SHADER_EXTENSION_NOT_SUPPORTED);
	}

	/* Initialize Utils instance */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	m_utils = new TessellationShaderUtils(gl, this);

	/* Initialize vertex array object */
	gl.genVertexArrays(1, &m_vao_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not generate vertex array object");

	gl.bindVertexArray(m_vao_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error binding vertex array object!");

	/* Retrieve GL_MAX_TESS_GEN_LEVEL_EXT value */
	glw::GLint gl_max_tess_gen_level_value = 0;

	gl.getIntegerv(m_glExtTokens.MAX_TESS_GEN_LEVEL, &gl_max_tess_gen_level_value);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv() call failed for GL_MAX_TESS_GEN_LEVEL_EXT pname");

	/* Initialize all test runs */
	const glw::GLint   tess_levels[] = { -gl_max_tess_gen_level_value / 2, -1, 1 };
	const unsigned int n_tess_levels = sizeof(tess_levels) / sizeof(tess_levels[0]);

	const _tessellation_shader_vertex_spacing vs_modes[] = {
		/* NOTE: We do not check "fractional even" vertex spacing since it will always
		 *       clamp to 2 which is out of scope for this test.
		 */
		TESSELLATION_SHADER_VERTEX_SPACING_DEFAULT,
		TESSELLATION_SHADER_VERTEX_SPACING_EQUAL, TESSELLATION_SHADER_VERTEX_SPACING_FRACTIONAL_ODD
	};
	const unsigned int n_vs_modes = sizeof(vs_modes) / sizeof(vs_modes[0]);

	/* Iterate through all vertex spacing modes */
	bool has_failed = false;

	for (unsigned int n_vs_mode = 0; n_vs_mode < n_vs_modes; ++n_vs_mode)
	{
		_tessellation_shader_vertex_spacing vs_mode = vs_modes[n_vs_mode];

		/* Iterate through all values that should be used for irrelevant tessellation levels */
		for (unsigned int n_tess_level = 0; n_tess_level < n_tess_levels; ++n_tess_level)
		{
			const glw::GLint tess_level = tess_levels[n_tess_level];

			/* Set up the run descriptor.
			 *
			 * Round outer tesellation levels to 1 if necessary, since otherwise no geometry will
			 * be generated.
			 **/
			_run run;

			run.inner[0]	   = (float)tess_level;
			run.inner[1]	   = (float)tess_level;
			run.outer[0]	   = (float)((tess_level < 0) ? 1 : tess_level);
			run.outer[1]	   = (float)((tess_level < 0) ? 1 : tess_level);
			run.outer[2]	   = (float)((tess_level < 0) ? 1 : tess_level);
			run.outer[3]	   = (float)((tess_level < 0) ? 1 : tess_level);
			run.vertex_spacing = vs_mode;

			/* Retrieve vertex data for both passes */
			run.n_vertices = m_utils->getAmountOfVerticesGeneratedByTessellator(
				TESSELLATION_SHADER_PRIMITIVE_MODE_QUADS, run.inner, run.outer, run.vertex_spacing,
				false); /* is_point_mode_enabled */

			if (run.n_vertices == 0)
			{
				std::string vs_mode_string = TessellationShaderUtils::getESTokenForVertexSpacingMode(vs_mode);

				m_testCtx.getLog() << tcu::TestLog::Message << "No vertices were generated by tessellator for: "
															   "inner tess levels:"
															   "["
								   << run.inner[0] << ", " << run.inner[1] << "]"
																			  ", outer tess levels:"
																			  "["
								   << run.outer[0] << ", " << run.outer[1] << ", " << run.outer[2] << ", "
								   << run.outer[3] << "]"
													  ", primitive mode: quads, "
													  "vertex spacing: "
								   << vs_mode_string << tcu::TestLog::EndMessage;

				has_failed = true;
			}
			else
			{
				/* Retrieve the data buffers */
				run.data = m_utils->getDataGeneratedByTessellator(run.inner, false, /* is_point_mode_enabled */
																  TESSELLATION_SHADER_PRIMITIVE_MODE_QUADS,
																  TESSELLATION_SHADER_VERTEX_ORDERING_CCW,
																  run.vertex_spacing, run.outer);
			}

			/* Store the run data */
			m_runs.push_back(run);
		} /* for (all tessellation levels) */
	}	 /* for (all vertex spacing modes) */

	if (has_failed)
	{
		TCU_FAIL("Zero vertices were generated by tessellator for at least one run which is not "
				 "a correct behavior");
	}
}

/** Executes the test.
 *
 *  Sets the test result to QP_TEST_RESULT_FAIL if the test failed, QP_TEST_RESULT_PASS otherwise.
 *
 *  Note the function throws exception should an error occur!
 *
 *  @return STOP if the test has finished, CONTINUE to indicate iterate() should be called once again.
 **/
tcu::TestNode::IterateResult TessellationShaderQuadsDegenerateCase::iterate(void)
{
	/* Do not execute if required extensions are not supported. */
	if (!m_is_tessellation_shader_supported)
	{
		throw tcu::NotSupportedError(TESSELLATION_SHADER_EXTENSION_NOT_SUPPORTED);
	}

	/* Initialize the test */
	initTest();

	/* Iterate through all runs */

	/* The test fails if any of the runs did not generate exactly 6 coordinates */
	for (_runs_const_iterator run_iterator = m_runs.begin(); run_iterator != m_runs.end(); run_iterator++)
	{
		const _run& run = *run_iterator;

		if (run.n_vertices != (2 /* triangles */ * 3 /* vertices */))
		{
			std::string vs_mode_string = TessellationShaderUtils::getESTokenForVertexSpacingMode(run.vertex_spacing);

			m_testCtx.getLog() << tcu::TestLog::Message << "Invalid number of coordinates (" << run.n_vertices
							   << ", instead of 6)"
								  " was generated for the following tessellation configuration: "
								  "primitive mode:quads, "
								  "vertex spacing mode:"
							   << vs_mode_string << " inner tessellation levels:" << run.inner[0] << ", "
							   << run.inner[1] << " outer tessellation levels:" << run.outer[0] << ", " << run.outer[1]
							   << ", " << run.outer[2] << ", " << run.outer[3] << tcu::TestLog::EndMessage;

			TCU_FAIL("Invalid number of coordinates was generated for at least one run");
		}
	} /* for (all runs) */

	/* All runs should generate exactly the same set of triangles.
	 *
	 * Note: we must not assume any specific vertex ordering, so we cannot
	 *       just do a plain memcmp() here.
	 */
	const _run& base_run = *m_runs.begin();

	for (unsigned int n_triangle = 0; n_triangle < base_run.n_vertices / 3 /* vertices per triangle */; n_triangle++)
	{
		const float* base_triangle_data = (const float*)(&base_run.data[0]) +
										  3		  /* vertices per triangle */
											  * 3 /* components */
											  * n_triangle;

		for (_runs_const_iterator ref_run_iterator = m_runs.begin() + 1; ref_run_iterator != m_runs.end();
			 ref_run_iterator++)
		{
			const _run& ref_run = *ref_run_iterator;

			const float* ref_triangle_data1 = (const float*)(&ref_run.data[0]);
			const float* ref_triangle_data2 =
				(const float*)(&ref_run.data[0]) + 3 /* vertices per triangle */ * 3 /* components */;

			if (!TessellationShaderUtils::isTriangleDefined(base_triangle_data, ref_triangle_data1) &&
				!TessellationShaderUtils::isTriangleDefined(base_triangle_data, ref_triangle_data2))
			{
				std::string base_vs_mode_string =
					TessellationShaderUtils::getESTokenForVertexSpacingMode(base_run.vertex_spacing);
				std::string ref_vs_mode_string =
					TessellationShaderUtils::getESTokenForVertexSpacingMode(ref_run.vertex_spacing);

				m_testCtx.getLog() << tcu::TestLog::Message
								   << "Reference run does not contain a triangle found in a base run"
									  " generated for the following tessellation configuration: "
									  "primitive mode:quads, "
									  "base vertex spacing mode:"
								   << base_vs_mode_string << " base inner tessellation levels:" << base_run.inner[0]
								   << ", " << base_run.inner[1]
								   << " base outer tessellation levels:" << base_run.outer[0] << ", "
								   << base_run.outer[1] << ", " << base_run.outer[2] << ", " << base_run.outer[3]
								   << "reference vertex spacing mode:" << ref_vs_mode_string
								   << " reference inner tessellation levels:" << ref_run.inner[0] << ", "
								   << ref_run.inner[1] << " reference outer tessellation levels:" << ref_run.outer[0]
								   << ", " << ref_run.outer[1] << ", " << ref_run.outer[2] << ", " << ref_run.outer[3]
								   << tcu::TestLog::EndMessage;

				TCU_FAIL("Reference run does not contain a triangle found in a base run");
			}
		} /* for (all reference runs) */
	}	 /* for (all triangles) */

	/* All done */
	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	return STOP;
}

/** Constructor
 *
 * @param context Test context
 **/
TessellationShaderQuadsInnerTessellationLevelRounding::TessellationShaderQuadsInnerTessellationLevelRounding(
	Context& context, const ExtParameters& extParams)
	: TestCaseBase(context, extParams, "inner_tessellation_level_rounding",
				   "Verifies that either inner tessellation level is rounded to 2 or 3,"
				   " when the tessellator is run in quads primitive mode and "
				   "corresponding inner tessellation level is set to 1.")
	, m_vao_id(0)
	, m_utils(DE_NULL)
{
	/* Left blank on purpose */
}

/** Deinitializes ES objects created for the test. */
void TessellationShaderQuadsInnerTessellationLevelRounding::deinit()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Call base class' deinit() */
	TestCaseBase::deinit();

	/* Unbind vertex array object */
	gl.bindVertexArray(0);

	/* Deinitialize utils instance */
	if (m_utils != DE_NULL)
	{
		delete m_utils;

		m_utils = DE_NULL;
	}

	/* Delete vertex array object */
	if (m_vao_id != 0)
	{
		gl.deleteVertexArrays(1, &m_vao_id);

		m_vao_id = 0;
	}
}

/** Takes a vertex data set and returns a vector of unique vec2s found in the set.
 *
 *  @param raw_data            Vertex data set to process.
 *  @param n_raw_data_vertices Amount of 3-component vertices found under @param raw_data.
 *
 *  @return As per description.
 **/
std::vector<_vec2> TessellationShaderQuadsInnerTessellationLevelRounding::getUniqueTessCoordinatesFromVertexDataSet(
	const float* raw_data, const unsigned int n_raw_data_vertices)
{
	std::vector<_vec2> result;

	for (unsigned int n_vertex = 0; n_vertex < n_raw_data_vertices; n_vertex += 2)
	{
		const float* vertex_data = raw_data + n_vertex * 3 /* components */;
		_vec2		 vertex(vertex_data[0], vertex_data[1]);

		if (std::find(result.begin(), result.end(), vertex) == result.end())
		{
			result.push_back(vertex);
		}

	} /* for (all vertices) */

	return result;
}

/** Initializes ES objects necessary to run the test. */
void TessellationShaderQuadsInnerTessellationLevelRounding::initTest()
{
	/* Skip if required extensions are not supported. */
	if (!m_is_tessellation_shader_supported)
	{
		throw tcu::NotSupportedError(TESSELLATION_SHADER_EXTENSION_NOT_SUPPORTED);
	}

	/* Initialize Utils instance */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	m_utils = new TessellationShaderUtils(gl, this);

	/* Initialize vertex array object */
	gl.genVertexArrays(1, &m_vao_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not generate vertex array object");

	gl.bindVertexArray(m_vao_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error binding vertex array object!");

	/* Retrieve GL_MAX_TESS_GEN_LEVEL_EXT value */
	glw::GLint gl_max_tess_gen_level_value = 0;

	gl.getIntegerv(m_glExtTokens.MAX_TESS_GEN_LEVEL, &gl_max_tess_gen_level_value);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv() call failed for GL_MAX_TESS_GEN_LEVEL_EXT pname");

	/* Initialize all test runs */
	const glw::GLint tess_levels[] = { 2, gl_max_tess_gen_level_value / 2, gl_max_tess_gen_level_value };

	const glw::GLint tess_levels_odd[] = { 2 + 1, gl_max_tess_gen_level_value / 2 + 1,
										   gl_max_tess_gen_level_value - 1 };
	const unsigned int n_tess_levels = sizeof(tess_levels) / sizeof(tess_levels[0]);

	const _tessellation_shader_vertex_spacing vs_modes[] = { TESSELLATION_SHADER_VERTEX_SPACING_EQUAL,
															 TESSELLATION_SHADER_VERTEX_SPACING_FRACTIONAL_EVEN,
															 TESSELLATION_SHADER_VERTEX_SPACING_FRACTIONAL_ODD };
	const unsigned int n_vs_modes = sizeof(vs_modes) / sizeof(vs_modes[0]);

	/* Iterate through all vertex spacing modes */
	for (unsigned int n_vs_mode = 0; n_vs_mode < n_vs_modes; ++n_vs_mode)
	{
		_tessellation_shader_vertex_spacing vs_mode = vs_modes[n_vs_mode];

		/* Iterate through all values that should be used for irrelevant tessellation levels */
		for (unsigned int n_tess_level = 0; n_tess_level < n_tess_levels; ++n_tess_level)
		{
			/* We need to test two cases in this test:
			 *
			 * a) inner[0] is set to 1 for set A and, for set B, to the value we're expecting the level to
			 *    round, given the vertex spacing mode. inner[1] is irrelevant.
			 * b) inner[0] is irrelevant. inner[1] is set to 1 for set A and, for set B, to the value we're
			 *    expecting the level to round, given the vertex spacing mode.
			 */
			for (unsigned int n_inner_tess_level_combination = 0;
				 n_inner_tess_level_combination < 2; /* inner[0], inner[1] */
				 ++n_inner_tess_level_combination)
			{
				/* Set up the run descriptor */
				glw::GLint tess_level = (vs_mode == TESSELLATION_SHADER_VERTEX_SPACING_FRACTIONAL_ODD) ?
											tess_levels_odd[n_tess_level] :
											tess_levels[n_tess_level];
				_run run;

				/* Determine inner tessellation level values for two cases */
				switch (n_inner_tess_level_combination)
				{
				case 0:
				{
					run.set1_inner[0] = 1.0f;
					run.set1_inner[1] = (glw::GLfloat)tess_level;
					run.set2_inner[1] = (glw::GLfloat)tess_level;

					TessellationShaderUtils::getTessellationLevelAfterVertexSpacing(
						vs_mode, run.set1_inner[0] + 1.0f /* epsilon */, gl_max_tess_gen_level_value,
						DE_NULL, /* out_clamped */
						run.set2_inner);
					break;
				} /* case 0: */

				case 1:
				{
					run.set1_inner[0] = (glw::GLfloat)tess_level;
					run.set1_inner[1] = 1.0f;
					run.set2_inner[0] = (glw::GLfloat)tess_level;

					TessellationShaderUtils::getTessellationLevelAfterVertexSpacing(
						vs_mode, run.set1_inner[1] + 1.0f /* epsilon */, gl_max_tess_gen_level_value,
						DE_NULL, /* out_clamped */
						run.set2_inner + 1);
					break;
				} /* case 1: */

				default:
					TCU_FAIL("Invalid inner tessellation level combination index");
				} /* switch (n_inner_tess_level_combination) */

				/* Configure outer tessellation level values */
				run.set1_outer[0] = (glw::GLfloat)tess_level;
				run.set2_outer[0] = (glw::GLfloat)tess_level;
				run.set1_outer[1] = (glw::GLfloat)tess_level;
				run.set2_outer[1] = (glw::GLfloat)tess_level;
				run.set1_outer[2] = (glw::GLfloat)tess_level;
				run.set2_outer[2] = (glw::GLfloat)tess_level;
				run.set1_outer[3] = (glw::GLfloat)tess_level;
				run.set2_outer[3] = (glw::GLfloat)tess_level;

				/* Set up remaining run properties */
				run.vertex_spacing = vs_mode;

				/* Retrieve vertex data for both passes */
				glw::GLint n_set1_vertices = 0;
				glw::GLint n_set2_vertices = 0;

				n_set1_vertices = m_utils->getAmountOfVerticesGeneratedByTessellator(
					TESSELLATION_SHADER_PRIMITIVE_MODE_QUADS, run.set1_inner, run.set1_outer, run.vertex_spacing,
					false); /* is_point_mode_enabled */
				n_set2_vertices = m_utils->getAmountOfVerticesGeneratedByTessellator(
					TESSELLATION_SHADER_PRIMITIVE_MODE_QUADS, run.set2_inner, run.set2_outer, run.vertex_spacing,
					false); /* is_point_mode_enabled */

				if (n_set1_vertices == 0)
				{
					std::string vs_mode_string = TessellationShaderUtils::getESTokenForVertexSpacingMode(vs_mode);

					m_testCtx.getLog() << tcu::TestLog::Message << "No vertices were generated by tessellator for: "
																   "inner tess levels:"
																   "["
									   << run.set1_inner[0] << ", " << run.set1_inner[1] << "]"
																							", outer tess levels:"
																							"["
									   << run.set1_outer[0] << ", " << run.set1_outer[1] << ", " << run.set1_outer[2]
									   << ", " << run.set1_outer[3] << "]"
																	   ", primitive mode: quads, "
																	   "vertex spacing: "
									   << vs_mode_string << tcu::TestLog::EndMessage;

					TCU_FAIL("Zero vertices were generated by tessellator for first test pass");
				}

				if (n_set2_vertices == 0)
				{
					std::string vs_mode_string = TessellationShaderUtils::getESTokenForVertexSpacingMode(vs_mode);

					m_testCtx.getLog() << tcu::TestLog::Message << "No vertices were generated by tessellator for: "
																   "inner tess levels:"
																   "["
									   << run.set2_inner[0] << ", " << run.set2_inner[1] << "]"
																							", outer tess levels:"
																							"["
									   << run.set2_outer[0] << ", " << run.set2_outer[1] << ", " << run.set2_outer[2]
									   << ", " << run.set2_outer[3] << "]"
																	   ", primitive mode: quads, "
																	   "vertex spacing: "
									   << vs_mode_string << tcu::TestLog::EndMessage;

					TCU_FAIL("Zero vertices were generated by tessellator for second test pass");
				}

				if (n_set1_vertices != n_set2_vertices)
				{
					std::string vs_mode_string = TessellationShaderUtils::getESTokenForVertexSpacingMode(vs_mode);

					m_testCtx.getLog() << tcu::TestLog::Message
									   << "Amount of vertices generated by the tessellator differs"
										  " for the following inner/outer configs: "
										  "inner tess levels:"
										  "["
									   << run.set1_inner[0] << ", " << run.set1_inner[1] << "]"
																							", outer tess levels:"
																							"["
									   << run.set1_outer[0] << ", " << run.set1_outer[1] << ", " << run.set1_outer[2]
									   << ", " << run.set1_outer[3] << "]"
																	   " and inner tess levels:"
																	   "["
									   << run.set2_inner[0] << ", " << run.set2_inner[1] << "]"
																							", outer tess levels:"
																							"["
									   << run.set2_outer[0] << ", " << run.set2_outer[1] << ", " << run.set2_outer[2]
									   << ", " << run.set2_outer[3] << "]"
																	   ", primitive mode: quads, vertex spacing: "
									   << vs_mode_string << tcu::TestLog::EndMessage;

					TCU_FAIL("Amount of vertices generated by tessellator differs between base and references passes");
				}

				/* Store the amount of vertices in run properties */
				run.n_vertices = n_set1_vertices;

				/* Retrieve the data buffers */
				run.set1_data = m_utils->getDataGeneratedByTessellator(
					run.set1_inner, false, /* is_point_mode_enabled */
					TESSELLATION_SHADER_PRIMITIVE_MODE_QUADS, TESSELLATION_SHADER_VERTEX_ORDERING_CCW,
					run.vertex_spacing, run.set1_outer);
				run.set2_data = m_utils->getDataGeneratedByTessellator(
					run.set2_inner, false, /* is_point_mode_enabled */
					TESSELLATION_SHADER_PRIMITIVE_MODE_QUADS, TESSELLATION_SHADER_VERTEX_ORDERING_CCW,
					run.vertex_spacing, run.set2_outer);

				/* Store the run data */
				m_runs.push_back(run);
			} /* for (all inner tess level combinations) */
		}	 /* for (all sets) */
	}		  /* for (all vertex spacing modes) */
}

/** Executes the test.
 *
 *  Sets the test result to QP_TEST_RESULT_FAIL if the test failed, QP_TEST_RESULT_PASS otherwise.
 *
 *  Note the function throws exception should an error occur!
 *
 *  @return STOP if the test has finished, CONTINUE to indicate iterate() should be called once again.
 **/
tcu::TestNode::IterateResult TessellationShaderQuadsInnerTessellationLevelRounding::iterate(void)
{
	/* Do not execute if required extensions are not supported. */
	if (!m_is_tessellation_shader_supported)
	{
		throw tcu::NotSupportedError(TESSELLATION_SHADER_EXTENSION_NOT_SUPPORTED);
	}

	/* Initialize the test */
	initTest();

	/* Iterate through all runs */

	for (_runs_const_iterator run_iterator = m_runs.begin(); run_iterator != m_runs.end(); run_iterator++)
	{
		const _run& run = *run_iterator;

		if (run.vertex_spacing != TESSELLATION_SHADER_VERTEX_SPACING_FRACTIONAL_ODD)
		{
			/* Make sure the vertex data generated for two passes matches */
			const glw::GLint n_triangles = run.n_vertices / 3 /* vertices per triangle */;

			std::vector<bool> triangleMatched;
			triangleMatched.assign(n_triangles, false);

			for (int n_triangle = 0; n_triangle < n_triangles; ++n_triangle)
			{
				const float* triangle_a = (const float*)(&run.set1_data[0]) +
										  n_triangle * 3 /* vertices */
											  * 3;		 /* components */
				/* Look up matching triangle */
				bool triangleFound = false;

				for (int n_triangle_b = 0; n_triangle_b < n_triangles; ++n_triangle_b)
				{
					if (!triangleMatched[n_triangle_b])
					{
						const float* triangle_b = (const float*)(&run.set2_data[0]) +
												  n_triangle_b * 3 /* vertices */
													  * 3;		   /* components */

						if (TessellationShaderUtils::isTriangleDefined(triangle_a, triangle_b))
						{
							triangleMatched[n_triangle_b] = true;
							triangleFound				  = true;
							break;
						}
					}
				}

				if (!triangleFound)
				{
					std::string vs_mode_string =
						TessellationShaderUtils::getESTokenForVertexSpacingMode(run.vertex_spacing);

					m_testCtx.getLog() << tcu::TestLog::Message
									   << "The following triangle, generated in the first pass, was not "
										  "generated in the second one. "
										  "First pass' configuration: inner tess levels:"
										  "["
									   << run.set1_inner[0] << ", " << run.set1_inner[1] << "]"
																							", outer tess levels:"
																							"["
									   << run.set1_outer[0] << ", " << run.set1_outer[1] << ", " << run.set1_outer[2]
									   << ", " << run.set1_outer[3]
									   << "]"
										  "; second pass' configuration: inner tess levels:"
										  "["
									   << run.set2_inner[0] << ", " << run.set2_inner[1] << "]"
																							", outer tess levels:"
																							"["
									   << run.set2_outer[0] << ", " << run.set2_outer[1] << ", " << run.set2_outer[2]
									   << ", " << run.set2_outer[3] << "]"
																	   ", primitive mode: quads, vertex spacing: "
									   << vs_mode_string << tcu::TestLog::EndMessage;

					TCU_FAIL("A triangle from first pass' data set was not found in second pass' data set.");
				}
			} /* for (all vertices) */
		}	 /* if (run.vertex_spacing != TESSELLATION_SHADER_VERTEX_SPACING_FRACTIONAL_ODD) */
		else
		{
			/* For Fractional Odd vertex spacing, we cannot apply the above methodology because of the fact
			 * two of the inner edges (the ones subdivided with tessellation level of 1.0) will have been
			 * subdivided differently between two runs, causing the vertex positions to shift, ultimately
			 * leading to isTriangleDefined() failure.
			 *
			 * Hence, we need to take a bit different approach for this case. If you look at a visualization
			 * of a tessellated quad, for which one of the inner tess levels has been set to 1 (let's assume
			 * inner[0] for the sake of discussion), and then looked at a list of points that were generated by the
			 * tessellator, you'll notice that even though it looks like we only have one span of triangles
			 * horizontally, there are actually three. The two outermost spans on each side (the "outer
			 * tessellation regions") are actually degenerate, because they have epsilon spacing allocated to
			 * them.
			 *
			 * Using the theory above, we look for a so-called "marker triangle". In the inner[0] = 1.0 case,
			 * it's one of the two triangles, one edge of which spans horizontally across the whole domain, and
			 * the other two edges touch the outermost edge of the quad. In the inner[1] = 1.0 case, Xs are flipped
			 * with Ys, but the general idea stays the same.
			 *
			 * So for fractional odd vertex spacing, this test verifies that the two marker triangles capping the
			 * opposite ends of the inner quad tessellation region exist.
			 */

			/* Convert arrayed triangle-based representation to a vector storing unique tessellation
			 * coordinates.
			 */
			std::vector<_vec2> set1_tess_coordinates =
				getUniqueTessCoordinatesFromVertexDataSet((const float*)(&run.set1_data[0]), run.n_vertices);

			/* Extract and sort the coordinate components we have from
			 * the minimum to the maximum */
			std::vector<float> set1_tess_coordinates_x_sorted;
			std::vector<float> set1_tess_coordinates_y_sorted;

			for (std::vector<_vec2>::iterator coordinate_iterator = set1_tess_coordinates.begin();
				 coordinate_iterator != set1_tess_coordinates.end(); coordinate_iterator++)
			{
				const _vec2& coordinate = *coordinate_iterator;

				if (std::find(set1_tess_coordinates_x_sorted.begin(), set1_tess_coordinates_x_sorted.end(),
							  coordinate.x) == set1_tess_coordinates_x_sorted.end())
				{
					set1_tess_coordinates_x_sorted.push_back(coordinate.x);
				}

				if (std::find(set1_tess_coordinates_y_sorted.begin(), set1_tess_coordinates_y_sorted.end(),
							  coordinate.y) == set1_tess_coordinates_y_sorted.end())
				{
					set1_tess_coordinates_y_sorted.push_back(coordinate.y);
				}
			} /* for (all tessellation coordinates) */

			std::sort(set1_tess_coordinates_x_sorted.begin(), set1_tess_coordinates_x_sorted.end());
			std::sort(set1_tess_coordinates_y_sorted.begin(), set1_tess_coordinates_y_sorted.end());

			/* Sanity checks */
			DE_ASSERT(set1_tess_coordinates_x_sorted.size() > 2);
			DE_ASSERT(set1_tess_coordinates_y_sorted.size() > 2);

			/* NOTE: This code could have been merged for both cases at the expense of code readability. */
			if (run.set1_inner[0] == 1.0f)
			{
				/* Look for the second horizontal line segment, starting from top and bottom */
				const float second_y_from_top = set1_tess_coordinates_y_sorted[1];
				const float second_y_from_bottom =
					set1_tess_coordinates_y_sorted[set1_tess_coordinates_y_sorted.size() - 2];

				/* In this particular case, there should be exactly one triangle spanning
				 * from U=0 to U=1 at both these heights, with the third coordinate located
				 * at the "outer" edge.
				 */
				for (int n = 0; n < 2 /* cases */; ++n)
				{
					float y1_y2 = 0.0f;
					float y3	= 0.0f;

					if (n == 0)
					{
						y1_y2 = second_y_from_bottom;
						y3	= 1.0f;
					}
					else
					{
						y1_y2 = second_y_from_top;
						y3	= 0.0f;
					}

					/* Try to find the triangle */
					bool has_found_triangle = false;

					DE_ASSERT((run.n_vertices % 3) == 0);

					for (unsigned int n_triangle = 0; n_triangle < run.n_vertices / 3; ++n_triangle)
					{
						const float* vertex1_data = (const float*)(&run.set1_data[0]) +
													3		/* vertices */
														* 3 /* components */
														* n_triangle;
						const float* vertex2_data = vertex1_data + 3 /* components */;
						const float* vertex3_data = vertex2_data + 3 /* components */;

						/* Make sure at least two Y coordinates are equal to y1_y2. */
						const float* y1_vertex_data = DE_NULL;
						const float* y2_vertex_data = DE_NULL;
						const float* y3_vertex_data = DE_NULL;

						if (vertex1_data[1] == y1_y2)
						{
							if (vertex2_data[1] == y1_y2 && vertex3_data[1] == y3)
							{
								y1_vertex_data = vertex1_data;
								y2_vertex_data = vertex2_data;
								y3_vertex_data = vertex3_data;
							}
							else if (vertex2_data[1] == y3 && vertex3_data[1] == y1_y2)
							{
								y1_vertex_data = vertex1_data;
								y2_vertex_data = vertex3_data;
								y3_vertex_data = vertex2_data;
							}
						}
						else if (vertex2_data[1] == y1_y2 && vertex3_data[1] == y1_y2 && vertex1_data[1] == y3)
						{
							y1_vertex_data = vertex2_data;
							y2_vertex_data = vertex3_data;
							y3_vertex_data = vertex1_data;
						}

						if (y1_vertex_data != DE_NULL && y2_vertex_data != DE_NULL && y3_vertex_data != DE_NULL)
						{
							/* Vertex 1 and 2 should span across whole domain horizontally */
							if ((y1_vertex_data[0] == 0.0f && y2_vertex_data[0] == 1.0f) ||
								(y1_vertex_data[0] == 1.0f && y2_vertex_data[0] == 0.0f))
							{
								has_found_triangle = true;

								break;
							}
						}
					} /* for (all triangles) */

					if (!has_found_triangle)
					{
						TCU_FAIL("Could not find a marker triangle");
					}
				} /* for (both cases) */
			}	 /* if (run.set1_inner[0] == 1.0f) */
			else
			{
				DE_ASSERT(run.set1_inner[1] == 1.0f);

				/* Look for the second vertical line segment, starting from left and right */
				const float second_x_from_left = set1_tess_coordinates_x_sorted[1];
				const float second_x_from_right =
					set1_tess_coordinates_x_sorted[set1_tess_coordinates_x_sorted.size() - 2];

				/* In this particular case, there should be exactly one triangle spanning
				 * from V=0 to V=1 at both these widths, with the third coordinate located
				 * at the "outer" edge.
				 */
				for (int n = 0; n < 2 /* cases */; ++n)
				{
					float x1_x2 = 0.0f;
					float x3	= 0.0f;

					if (n == 0)
					{
						x1_x2 = second_x_from_right;
						x3	= 1.0f;
					}
					else
					{
						x1_x2 = second_x_from_left;
						x3	= 0.0f;
					}

					/* Try to find the triangle */
					bool has_found_triangle = false;

					DE_ASSERT((run.n_vertices % 3) == 0);

					for (unsigned int n_triangle = 0; n_triangle < run.n_vertices / 3; ++n_triangle)
					{
						const float* vertex1_data =
							(const float*)(&run.set1_data[0]) + 3 /* vertices */ * 3 /* components */ * n_triangle;
						const float* vertex2_data = vertex1_data + 3 /* components */;
						const float* vertex3_data = vertex2_data + 3 /* components */;

						/* Make sure at least two X coordinates are equal to x1_x2. */
						const float* x1_vertex_data = DE_NULL;
						const float* x2_vertex_data = DE_NULL;
						const float* x3_vertex_data = DE_NULL;

						if (vertex1_data[0] == x1_x2)
						{
							if (vertex2_data[0] == x1_x2 && vertex3_data[0] == x3)
							{
								x1_vertex_data = vertex1_data;
								x2_vertex_data = vertex2_data;
								x3_vertex_data = vertex3_data;
							}
							else if (vertex2_data[0] == x3 && vertex3_data[0] == x1_x2)
							{
								x1_vertex_data = vertex1_data;
								x2_vertex_data = vertex3_data;
								x3_vertex_data = vertex2_data;
							}
						}
						else if (vertex2_data[0] == x1_x2 && vertex3_data[0] == x1_x2 && vertex1_data[0] == x3)
						{
							x1_vertex_data = vertex2_data;
							x2_vertex_data = vertex3_data;
							x3_vertex_data = vertex1_data;
						}

						if (x1_vertex_data != DE_NULL && x2_vertex_data != DE_NULL && x3_vertex_data != DE_NULL)
						{
							/* Vertex 1 and 2 should span across whole domain vertically */
							if ((x1_vertex_data[1] == 0.0f && x2_vertex_data[1] == 1.0f) ||
								(x1_vertex_data[1] == 1.0f && x2_vertex_data[1] == 0.0f))
							{
								has_found_triangle = true;

								break;
							}
						}
					} /* for (all triangles) */

					if (!has_found_triangle)
					{
						TCU_FAIL("Could not find a marker triangle (implies invalid quad tessellation for the case "
								 "considered)");
					}
				} /* for (both cases) */
			}
		}
	} /* for (all runs) */

	/* All done */
	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	return STOP;
}

} /* namespace glcts */
