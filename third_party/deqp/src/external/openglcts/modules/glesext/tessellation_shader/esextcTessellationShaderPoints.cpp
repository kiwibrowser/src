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

#include "esextcTessellationShaderPoints.hpp"
#include "gluContextInfo.hpp"
#include "gluDefs.hpp"
#include "glwEnums.hpp"
#include "glwFunctions.hpp"
#include "tcuTestLog.hpp"

namespace glcts
{

const unsigned int TessellationShaderPointsgl_PointSize::m_rt_height =
	16; /* note: update shaders if you change this value */
const unsigned int TessellationShaderPointsgl_PointSize::m_rt_width =
	16; /* note: update shaders if you change this value */

/** Constructor
 *
 * @param context Test context
 **/
TessellationShaderPointsTests::TessellationShaderPointsTests(glcts::Context& context, const ExtParameters& extParams)
	: TestCaseGroupBase(context, extParams, "tessellation_shader_point_mode", "Verifies point mode functionality")
{
	/* No implementation needed */
}

/**
 * Initializes test groups for geometry shader tests
 **/
void TessellationShaderPointsTests::init(void)
{
	addChild(new glcts::TessellationShaderPointsgl_PointSize(m_context, m_extParams));
	addChild(new glcts::TessellationShaderPointsVerification(m_context, m_extParams));
}

/** Constructor
 *
 * @param context       Test context
 * @param name          Test case's name
 * @param description   Test case's desricption
 **/
TessellationShaderPointsgl_PointSize::TessellationShaderPointsgl_PointSize(Context&				context,
																		   const ExtParameters& extParams)
	: TestCaseBase(context, extParams, "point_rendering", "Verifies point size used to render points is taken from"
														  " the right stage")
	, m_fbo_id(0)
	, m_to_id(0)
	, m_vao_id(0)
{
	/* Left blank on purpose */
}

/** Deinitializes all ES objects created for the test. */
void TessellationShaderPointsgl_PointSize::deinit()
{
	/** Call base class' deinit() function */
	TestCaseBase::deinit();

	if (!m_is_tessellation_shader_supported)
	{
		return;
	}

	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (!glu::isContextTypeES(m_context.getRenderContext().getType()))
	{
		/* Disable point size */
		gl.disable(GL_PROGRAM_POINT_SIZE);
	}

	/* Reset the program object */
	gl.useProgram(0);

	/* Revert GL_PATCH_VERTICES_EXT to default value */
	gl.patchParameteri(m_glExtTokens.PATCH_VERTICES, 3);

	/* Unbind vertex array object */
	gl.bindVertexArray(0);

	/* Deinitialize test-specific objects */
	for (_tests_iterator it = m_tests.begin(); it != m_tests.end(); ++it)
	{
		const _test_descriptor& test = *it;

		if (test.fs_id != 0)
		{
			gl.deleteShader(test.fs_id);
		}

		if (test.gs_id != 0)
		{
			gl.deleteShader(test.gs_id);
		}

		if (test.po_id != 0)
		{
			gl.deleteProgram(test.po_id);
		}

		if (test.tes_id != 0)
		{
			gl.deleteShader(test.tes_id);
		}

		if (test.tcs_id != 0)
		{
			gl.deleteShader(test.tcs_id);
		}

		if (test.vs_id != 0)
		{
			gl.deleteShader(test.vs_id);
		}
	}
	m_tests.clear();

	if (m_fbo_id != 0)
	{
		gl.deleteFramebuffers(1, &m_fbo_id);

		m_fbo_id = 0;
	}

	if (m_to_id != 0)
	{
		gl.deleteTextures(1, &m_to_id);

		m_to_id = 0;
	}

	if (m_vao_id != 0)
	{
		gl.deleteVertexArrays(1, &m_vao_id);

		m_vao_id = 0;
	}
}

/** Initializes all ES objects that will be used for the test. */
void TessellationShaderPointsgl_PointSize::initTest()
{
	/* The test should only execute if maximum point size is at least 2 */
	const glw::Functions& gl						 = m_context.getRenderContext().getFunctions();
	glw::GLint			  gl_max_point_size_value[2] = { 0 };
	const int			  min_max_point_size		 = 2;

	if (!glu::isContextTypeES(m_context.getRenderContext().getType()))
	{
		gl.getIntegerv(GL_POINT_SIZE_RANGE, gl_max_point_size_value);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv() for GL_POINT_SIZE_RANGE failed.");
	}
	else
	{
		gl.getIntegerv(GL_ALIASED_POINT_SIZE_RANGE, gl_max_point_size_value);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv() for GL_ALIASED_POINT_SIZE_RANGE failed.");
	}

	if (gl_max_point_size_value[1] < min_max_point_size)
	{
		throw tcu::NotSupportedError("Maximum point size is lower than 2.");
	}

	/* The test requires EXT_tessellation_shader, EXT_tessellation_shader_point_size */
	if (!m_is_tessellation_shader_supported || !m_is_tessellation_shader_point_size_supported)
	{
		throw tcu::NotSupportedError("At least one of the required extensions is not supported.");
	}

	/* Initialize vertex array object */
	gl.genVertexArrays(1, &m_vao_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not generate vertex array object");

	gl.bindVertexArray(m_vao_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error binding vertex array object!");

	/* Initialize fs+gs+vs test descriptor */
	if (m_is_geometry_shader_extension_supported)
	{
		_test_descriptor pass_fs_gs_tes_vs;

		/* Configure shader bodies */
		pass_fs_gs_tes_vs.fs_body = "${VERSION}\n"
									"\n"
									"precision highp float;\n"
									"\n"
									"in  vec4 color;\n"
									"out vec4 result;\n"
									"\n"
									"void main()\n"
									"{\n"
									"    result = color;\n"
									"}\n";

		pass_fs_gs_tes_vs.gs_body = "${VERSION}\n"
									"\n"
									"${GEOMETRY_SHADER_REQUIRE}\n"
									"${GEOMETRY_POINT_SIZE_REQUIRE}\n"
									"\n"
									"layout(points)                 in;\n"
									"layout(points, max_vertices=5) out;\n"
									"\n"
									"out vec4 color;\n"
									"\n"
									"void main()\n"
									"{\n"
									"    const float point_dx = 2.0 / 16.0 /* rendertarget width */;\n"
									"    const float point_dy = 2.0 / 16.0 /* rendertarget_height */;\n"
									"\n"
									/* Center */
									"    color        = vec4(0.1, 0.2, 0.3, 0.4);\n"
									"    gl_PointSize = 2.0;\n"
									"    gl_Position  = vec4(point_dx + 0.0, point_dy + 0.0, 0.0, 1.0);\n"
									"    EmitVertex();\n"
									"\n"
									/* Top-left corner */
									"    color        = vec4(0.2, 0.3, 0.4, 0.5);\n"
									"    gl_PointSize = 2.0;\n"
									"    gl_Position  = vec4(point_dx - 1.0, -point_dy + 1.0, 0.0, 1.0);\n"
									"    EmitVertex();\n"
									"\n"
									/* Top-right corner */
									"    color        = vec4(0.3, 0.4, 0.5, 0.6);\n"
									"    gl_PointSize = 2.0;\n"
									"    gl_Position  = vec4(-point_dx + 1.0, -point_dy + 1.0, 0.0, 1.0);\n"
									"    EmitVertex();\n"
									"\n"
									/* Bottom-left corner */
									"    color        = vec4(0.4, 0.5, 0.6, 0.7);\n"
									"    gl_PointSize = 2.0;\n"
									"    gl_Position  = vec4(point_dx - 1.0, point_dy - 1.0, 0.0, 1.0);\n"
									"    EmitVertex();\n"
									"\n"
									/* Bottom-right corner */
									"    color        = vec4(0.5, 0.6, 0.7, 0.8);\n"
									"    gl_PointSize = 2.0;\n"
									"    gl_Position  = vec4(-point_dx + 1.0, point_dy - 1.0, 0.0, 1.0);\n"
									"    EmitVertex();\n"
									"\n"
									"}\n";

		pass_fs_gs_tes_vs.tes_body = "${VERSION}\n"
									 "\n"
									 "${TESSELLATION_SHADER_REQUIRE}\n"
									 "${TESSELLATION_POINT_SIZE_REQUIRE}\n"
									 "\n"
									 "layout(isolines, point_mode) in;\n"
									 "\n"
									 "void main()\n"
									 "{\n"
									 "    gl_PointSize = 0.1;\n"
									 "}\n";

		pass_fs_gs_tes_vs.tcs_body = "${VERSION}\n"
									 "\n"
									 "${TESSELLATION_SHADER_REQUIRE}\n"
									 "${TESSELLATION_POINT_SIZE_REQUIRE}\n"
									 "\n"
									 "layout(vertices=1) out;\n"
									 "\n"
									 "void main()\n"
									 "{\n"
									 "    gl_out[gl_InvocationID].gl_Position =\n"
									 "        gl_in[gl_InvocationID].gl_Position;\n"
									 "    gl_out[gl_InvocationID].gl_PointSize =\n"
									 "        gl_in[gl_InvocationID].gl_PointSize;\n"
									 "\n"
									 "    gl_TessLevelOuter[0] = 1.0;\n"
									 "    gl_TessLevelOuter[1] = 1.0;\n"
									 "    gl_TessLevelOuter[2] = 1.0;\n"
									 "    gl_TessLevelOuter[3] = 1.0;\n"
									 "    gl_TessLevelInner[0] = 1.0;\n"
									 "    gl_TessLevelInner[1] = 1.0;\n"
									 "}\n";

		pass_fs_gs_tes_vs.vs_body = "${VERSION}\n"
									"\n"
									"void main()\n"
									"{\n"
									"    gl_PointSize = 0.01;\n"
									"}\n";

		pass_fs_gs_tes_vs.draw_call_count = 1;

		/* Store the descriptor in a vector that will be used by iterate() */
		m_tests.push_back(pass_fs_gs_tes_vs);
	} /* if (m_is_geometry_shader_extension_supported) */

	/* Initialize fs+te+vs test descriptor */
	if (m_is_tessellation_shader_supported)
	{
		_test_descriptor pass_fs_tes_vs;

		/* Configure shader bodies */
		pass_fs_tes_vs.fs_body = "${VERSION}\n"
								 "\n"
								 "precision highp float;\n"
								 "\n"
								 "in  vec4 result_color;\n"
								 "out vec4 result;\n"
								 "\n"
								 "void main()\n"
								 "{\n"
								 "    result = result_color;\n"
								 "}\n";

		pass_fs_tes_vs.tes_body = "${VERSION}\n"
								  "\n"
								  "${TESSELLATION_SHADER_REQUIRE}\n"
								  "${TESSELLATION_POINT_SIZE_REQUIRE}\n"
								  "\n"
								  "layout(isolines, point_mode) in;\n"
								  "\n"
								  "in  vec4 tcColor[];\n"
								  "out vec4 result_color;\n"
								  "\n"
								  "void main()\n"
								  "{\n"
								  "    gl_PointSize = 2.0;\n"
								  "    gl_Position  = gl_in[0].gl_Position;\n"
								  "    result_color = tcColor[0];\n"
								  "}\n";

		pass_fs_tes_vs.tcs_body = "${VERSION}\n"
								  "\n"
								  "${TESSELLATION_SHADER_REQUIRE}\n"
								  "${TESSELLATION_POINT_SIZE_REQUIRE}\n"
								  "\n"
								  "layout(vertices=1) out;\n"
								  "\n"
								  "in  vec4 color[];\n"
								  "out vec4 tcColor[];\n"
								  "\n"
								  "void main()\n"
								  "{\n"
								  "    tcColor[gl_InvocationID] = color[gl_InvocationID];\n"
								  "    gl_out[gl_InvocationID].gl_Position =\n"
								  "        gl_in[gl_InvocationID].gl_Position;\n"
								  "    gl_out[gl_InvocationID].gl_PointSize =\n"
								  "        gl_in[gl_InvocationID].gl_PointSize;\n"
								  "\n"
								  "    gl_TessLevelOuter[0] = 1.0;\n"
								  "    gl_TessLevelOuter[1] = 1.0;\n"
								  "    gl_TessLevelOuter[2] = 1.0;\n"
								  "    gl_TessLevelOuter[3] = 1.0;\n"
								  "    gl_TessLevelInner[0] = 1.0;\n"
								  "    gl_TessLevelInner[1] = 1.0;\n"
								  "}\n";

		pass_fs_tes_vs.vs_body = "${VERSION}\n"
								 "\n"
								 "out vec4 color;\n"
								 "\n"
								 "void main()\n"
								 "{\n"
								 "    const float point_dx = 2.0 / 16.0 /* rendertarget width */;\n"
								 "    const float point_dy = 2.0 / 16.0 /* rendertarget_height */;\n"
								 "\n"
								 "    gl_PointSize = 0.1;\n"
								 "\n"
								 "    switch (gl_VertexID)\n"
								 "    {\n"
								 "        case 0:\n"
								 "        {\n"
								 /* Center */
								 "            color       = vec4(0.1, 0.2, 0.3, 0.4);\n"
								 "            gl_Position = vec4(point_dx + 0.0, point_dy + 0.0, 0.0, 1.0);\n"
								 "\n"
								 "            break;\n"
								 "        }\n"
								 "\n"
								 "        case 1:\n"
								 "        {\n"
								 /* Top-left corner */
								 "            color       = vec4(0.2, 0.3, 0.4, 0.5);\n"
								 "            gl_Position = vec4(point_dx - 1.0, -point_dy + 1.0, 0.0, 1.0);\n"
								 "\n"
								 "            break;\n"
								 "        }\n"
								 "\n"
								 "        case 2:\n"
								 "        {\n"
								 /* Top-right corner */
								 "            color       = vec4(0.3, 0.4, 0.5, 0.6);\n"
								 "            gl_Position = vec4(-point_dx + 1.0, -point_dy + 1.0, 0.0, 1.0);\n"
								 "\n"
								 "            break;\n"
								 "        }\n"
								 "\n"
								 "        case 3:\n"
								 "        {\n"
								 /* Bottom-left corner */
								 "            color       = vec4(0.4, 0.5, 0.6, 0.7);\n"
								 "            gl_Position = vec4(point_dx - 1.0, point_dy - 1.0, 0.0, 1.0);\n"
								 "\n"
								 "            break;\n"
								 "        }\n"
								 "\n"
								 "        case 4:\n"
								 "        {\n"
								 /* Bottom-right corner */
								 "            color       = vec4(0.5, 0.6, 0.7, 0.8);\n"
								 "            gl_Position = vec4(-point_dx + 1.0, point_dy - 1.0, 0.0, 1.0);\n"
								 "\n"
								 "            break;\n"
								 "        }\n"
								 "    }\n"
								 "}\n";

		pass_fs_tes_vs.draw_call_count = 5; /* points in total */

		/* Store the descriptor in a vector that will be used by iterate() */
		m_tests.push_back(pass_fs_tes_vs);
	} /* if (m_is_tessellation_shader_supported) */

	/* Set up a color texture we will be rendering to */
	gl.genTextures(1, &m_to_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenTextures() failed");

	gl.bindTexture(GL_TEXTURE_2D, m_to_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture() failed");

	gl.texStorage2D(GL_TEXTURE_2D, 1 /* levels */, GL_RGBA8, m_rt_width, m_rt_height);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTexStorage2D() failed");

	/* Set up a FBO we'll use for rendering */
	gl.genFramebuffers(1, &m_fbo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenFramebuffers() failed");

	gl.bindFramebuffer(GL_FRAMEBUFFER, m_fbo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffer() failed");

	gl.framebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_to_id, 0 /* level */);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glFramebufferTexture2D() failed");

	if (!glu::isContextTypeES(m_context.getRenderContext().getType()))
	{
		/* Enable point size */
		gl.enable(GL_PROGRAM_POINT_SIZE);
	}

	/* We're good to execute the test! */
}

/** Executes the test.
 *
 *  Sets the test result to QP_TEST_RESULT_FAIL if the test failed, QP_TEST_RESULT_PASS otherwise.
 *
 *  Note the function throws exception should an error occur!
 *
 *  @return STOP if the test has finished, CONTINUE to indicate iterate() should be called once again.
 **/
tcu::TestNode::IterateResult TessellationShaderPointsgl_PointSize::iterate(void)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	initTest();

	gl.patchParameteri(m_glExtTokens.PATCH_VERTICES, 1);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glPatchParameteriEXT() failed");

	/* Iterate through all test descriptors.. */
	for (_tests_iterator test_iterator = m_tests.begin(); test_iterator != m_tests.end(); test_iterator++)
	{
		_test_descriptor& test = *test_iterator;

		/* Generate all shader objects we'll need */
		if (test.fs_body != NULL)
		{
			test.fs_id = gl.createShader(GL_FRAGMENT_SHADER);
		}

		if (test.gs_body != NULL)
		{
			test.gs_id = gl.createShader(m_glExtTokens.GEOMETRY_SHADER);
		}

		if (test.tes_body != NULL)
		{
			test.tes_id = gl.createShader(m_glExtTokens.TESS_EVALUATION_SHADER);
		}

		if (test.tcs_body != NULL)
		{
			test.tcs_id = gl.createShader(m_glExtTokens.TESS_CONTROL_SHADER);
		}

		if (test.vs_body != NULL)
		{
			test.vs_id = gl.createShader(GL_VERTEX_SHADER);
		}
		GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateShader() call(s) failed");

		/* Generate a test program object before we continue */
		test.po_id = gl.createProgram();
		GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateProgram() call failed");

		bool link_success =
			buildProgram(test.po_id, test.fs_id, test.fs_id ? 1 : 0, &test.fs_body, test.gs_id, test.gs_id ? 1 : 0,
						 &test.gs_body, test.tes_id, test.tes_id ? 1 : 0, &test.tes_body, test.tcs_id,
						 test.tcs_id ? 1 : 0, &test.tcs_body, test.vs_id, test.vs_id ? 1 : 0, &test.vs_body);

		if (!link_success)
		{
			TCU_FAIL("Program linking failed");
		}

		/* Prepare for rendering */
		gl.clearColor(0.0f /* red */, 0.0f /* green */, 0.0f /* blue */, 0.0f /* alpha */);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glClearColor() failed");

		gl.clear(GL_COLOR_BUFFER_BIT);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glClear(GL_COLOR_BUFFER_BIT) failed");

		gl.viewport(0 /* x */, 0 /* x */, m_rt_width, m_rt_height);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glViewport() failed");

		gl.useProgram(test.po_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram() failed");

		/* Render */
		gl.drawArrays(m_glExtTokens.PATCHES, 0 /* first */, test.draw_call_count);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawArrays() failed");

		/* Read back the rendered data */
		unsigned char buffer[m_rt_width * m_rt_height * 4 /* components */] = { 0 };

		gl.readPixels(0, /* x */
					  0, /* y */
					  m_rt_width, m_rt_height, GL_RGBA, GL_UNSIGNED_BYTE, buffer);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glReadPixels() failed");

		/* Verify all 5 points were rendered correctly */
		const float		   epsilon		= (float)1.0f / 255.0f;
		const unsigned int pixel_size   = 4; /* components, GL_UNSIGNED_BYTE */
		const float		   point_data[] = {
			/* x */ /* y */ /* r */ /* g */ /* b */ /* a */
			0.5f, 0.5f, 0.1f, 0.2f, 0.3f, 0.4f,		/* center */
			0.0f, 1.0f, 0.2f, 0.3f, 0.4f, 0.5f,		/* top-left */
			1.0f, 1.0f, 0.3f, 0.4f, 0.5f, 0.6f,		/* top-right */
			0.0f, 0.0f, 0.4f, 0.5f, 0.6f, 0.7f,		/* bottom-left */
			1.0f, 0.0f, 0.5f, 0.6f, 0.7f, 0.8f		/* bottom-right */
		};
		const unsigned int row_size			  = pixel_size * m_rt_width;
		const unsigned int n_fields_per_point = 6;
		const unsigned int n_points			  = sizeof(point_data) / sizeof(point_data[0]) / n_fields_per_point;

		for (unsigned int n_point = 0; n_point < n_points; ++n_point)
		{
			int   x = (int)(point_data[n_point * n_fields_per_point + 0] * float(m_rt_width - 1) + 0.5f);
			int   y = (int)(point_data[n_point * n_fields_per_point + 1] * float(m_rt_height - 1) + 0.5f);
			float expected_color_r = point_data[n_point * n_fields_per_point + 2];
			float expected_color_g = point_data[n_point * n_fields_per_point + 3];
			float expected_color_b = point_data[n_point * n_fields_per_point + 4];
			float expected_color_a = point_data[n_point * n_fields_per_point + 5];

			const unsigned char* rendered_color_ubyte_ptr = buffer + row_size * y + x * pixel_size;
			const float			 rendered_color_r		  = float(rendered_color_ubyte_ptr[0]) / 255.0f;
			const float			 rendered_color_g		  = float(rendered_color_ubyte_ptr[1]) / 255.0f;
			const float			 rendered_color_b		  = float(rendered_color_ubyte_ptr[2]) / 255.0f;
			const float			 rendered_color_a		  = float(rendered_color_ubyte_ptr[3]) / 255.0f;

			/* Compare the pixels */
			if (de::abs(expected_color_r - rendered_color_r) > epsilon ||
				de::abs(expected_color_g - rendered_color_g) > epsilon ||
				de::abs(expected_color_b - rendered_color_b) > epsilon ||
				de::abs(expected_color_a - rendered_color_a) > epsilon)
			{
				m_testCtx.getLog() << tcu::TestLog::Message << "Pixel data comparison failed; expected: "
								   << "(" << expected_color_r << ", " << expected_color_g << ", " << expected_color_b
								   << ", " << expected_color_a << ") rendered: "
								   << "(" << rendered_color_r << ", " << rendered_color_g << ", " << rendered_color_b
								   << ", " << rendered_color_a << ") epsilon: " << epsilon << tcu::TestLog::EndMessage;

				TCU_FAIL("Pixel data comparison failed");
			}
		} /* for (all points) */
	}	 /* for (all tests) */

	/* All done */
	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	return STOP;
}

/** Constructor
 *
 * @param context Test context
 **/
TessellationShaderPointsVerification::TessellationShaderPointsVerification(Context&				context,
																		   const ExtParameters& extParams)
	: TestCaseBase(context, extParams, "points_verification",
				   "Verifies points generated by the tessellator unit do not duplicate "
				   "and that their amount is correct")
	, m_utils(DE_NULL)
	, m_vao_id(0)
{
	/* Left blank on purpose */
}

/* Deinitializes all ES Instances generated for the test */
void TessellationShaderPointsVerification::deinit()
{
	/* Call base class' deinit() */
	TestCaseBase::deinit();

	if (!m_is_tessellation_shader_supported)
	{
		return;
	}

	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Unbind vertex array object */
	gl.bindVertexArray(0);

	/* Delete utils instances */
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
void TessellationShaderPointsVerification::initTest()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Skip if required extensions are not supported. */
	if (!m_is_tessellation_shader_supported)
	{
		throw tcu::NotSupportedError(TESSELLATION_SHADER_EXTENSION_NOT_SUPPORTED);
	}

	/* Initialize and bind vertex array object */
	gl.genVertexArrays(1, &m_vao_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not generate vertex array object");

	gl.bindVertexArray(m_vao_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error binding vertex array object!");

	/* Retrieve GL_MAX_TESS_GEN_LEVEL_EXT value */
	glw::GLint gl_max_tess_gen_level_value = 0;

	gl.getIntegerv(m_glExtTokens.MAX_TESS_GEN_LEVEL, &gl_max_tess_gen_level_value);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv() failed for GL_MAX_TESS_GEN_LEVEL_EXT pname");

	/* Initialize all test iterations */
	const _tessellation_primitive_mode primitive_modes[] = { TESSELLATION_SHADER_PRIMITIVE_MODE_QUADS,
															 TESSELLATION_SHADER_PRIMITIVE_MODE_ISOLINES,
															 TESSELLATION_SHADER_PRIMITIVE_MODE_TRIANGLES };
	const unsigned int n_primitive_modes = sizeof(primitive_modes) / sizeof(primitive_modes[0]);

	const _tessellation_shader_vertex_spacing vertex_spacings[] = {
		TESSELLATION_SHADER_VERTEX_SPACING_FRACTIONAL_ODD, TESSELLATION_SHADER_VERTEX_SPACING_EQUAL,
		TESSELLATION_SHADER_VERTEX_SPACING_FRACTIONAL_EVEN,
	};
	const unsigned int n_vertex_spacings = sizeof(vertex_spacings) / sizeof(vertex_spacings[0]);

	for (unsigned int n_primitive_mode = 0; n_primitive_mode < n_primitive_modes; ++n_primitive_mode)
	{
		_tessellation_levels_set	 levels_set;
		_tessellation_primitive_mode primitive_mode = primitive_modes[n_primitive_mode];

		levels_set = TessellationShaderUtils::getTessellationLevelSetForPrimitiveMode(
			primitive_mode, gl_max_tess_gen_level_value,
			TESSELLATION_LEVEL_SET_FILTER_INNER_AND_OUTER_LEVELS_USE_DIFFERENT_VALUES);

		for (unsigned int n_vertex_spacing = 0; n_vertex_spacing < n_vertex_spacings; ++n_vertex_spacing)
		{
			_tessellation_shader_vertex_spacing vertex_spacing = vertex_spacings[n_vertex_spacing];

			for (_tessellation_levels_set_const_iterator levels_set_iterator = levels_set.begin();
				 levels_set_iterator != levels_set.end(); levels_set_iterator++)
			{
				const _tessellation_levels& levels = *levels_set_iterator;

				/* Skip border cases that this test cannot handle */
				if ((primitive_mode == TESSELLATION_SHADER_PRIMITIVE_MODE_QUADS ||
					 primitive_mode == TESSELLATION_SHADER_PRIMITIVE_MODE_TRIANGLES) &&
					vertex_spacing == TESSELLATION_SHADER_VERTEX_SPACING_FRACTIONAL_ODD &&
					(levels.inner[0] <= 1 || levels.inner[1] <= 1))
				{
					continue;
				}

				/* Initialize a test run descriptor for the iteration-specific properties */
				_run run;

				memcpy(run.inner, levels.inner, sizeof(run.inner));
				memcpy(run.outer, levels.outer, sizeof(run.outer));

				run.primitive_mode = primitive_mode;
				run.vertex_spacing = vertex_spacing;

				m_runs.push_back(run);
			} /* for (all level sets) */
		}	 /* for (all vertex spacing modes) */
	}		  /* for (all primitive modes) */

	/* Initialize utils instance.
	 */
	m_utils = new TessellationShaderUtils(gl, this);
}

/** Executes the test.
 *
 *  Sets the test result to QP_TEST_RESULT_FAIL if the test failed, QP_TEST_RESULT_PASS otherwise.
 *
 *  Note the function throws exception should an error occur!
 *
 *  @return STOP if the test has finished, CONTINUE to indicate iterate() should be called once again.
 **/
tcu::TestNode::IterateResult TessellationShaderPointsVerification::iterate(void)
{
	initTest();

	/* Do not execute if required extensions are not supported. */
	if (!m_is_tessellation_shader_supported)
	{
		throw tcu::NotSupportedError(TESSELLATION_SHADER_EXTENSION_NOT_SUPPORTED);
	}

	/* Iterate through all the test descriptors */
	for (std::vector<_run>::const_iterator run_iterator = m_runs.begin(); run_iterator != m_runs.end(); run_iterator++)
	{
		const _run&		  run = *run_iterator;
		std::vector<char> run_data;
		unsigned int	  run_n_vertices = 0;

		run_data = m_utils->getDataGeneratedByTessellator(run.inner, true, /* point_mode */
														  run.primitive_mode, TESSELLATION_SHADER_VERTEX_ORDERING_CCW,
														  run.vertex_spacing, run.outer);

		run_n_vertices = m_utils->getAmountOfVerticesGeneratedByTessellator(run.primitive_mode, run.inner, run.outer,
																			run.vertex_spacing, true); /* point_mode */

		/* First, make sure a valid amount of duplicate vertices was found for a single data set */
		verifyCorrectAmountOfDuplicateVertices(run, &run_data[0], run_n_vertices);

		/* Now, verify that amount of generated vertices is correct, given
		 * tessellation shader stage configuration */
		verifyCorrectAmountOfVertices(run, &run_data[0], run_n_vertices);
	} /* for (all tests) */

	/* All done */
	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	return STOP;
}

/** Verifies that a correct amount of vertices was generated, given test iteration-specific properties.
 *  Throws a TestError exception in if an incorrect amount of vertices was generated by the tessellator.
 *
 *  @param run            Run descriptor.
 *  @param run_data       Data generated for the run.
 *  @param run_n_vertices Amount of vertices present at @param run_data.
 */
void TessellationShaderPointsVerification::verifyCorrectAmountOfVertices(const _run& run, const void* run_data,
																		 unsigned int run_n_vertices)
{
	(void)run_data;

	const float			  epsilon					   = 1e-5f;
	const glw::Functions& gl						   = m_context.getRenderContext().getFunctions();
	unsigned int		  n_expected_vertices		   = 0;
	float				  post_vs_inner_tess_levels[2] = { 0.0f };
	float				  post_vs_outer_tess_levels[4] = { 0.0f };

	/* Retrieve GL_MAX_TESS_GEN_LEVEL_EXT value*/
	glw::GLint gl_max_tess_gen_level_value = 0;

	gl.getIntegerv(m_glExtTokens.MAX_TESS_GEN_LEVEL, &gl_max_tess_gen_level_value);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv() call failed for GL_MAX_TESS_GEN_LEVEL_EXT pname");

	/* Determine vertex spacing that the tessellator should have used for current primitive mode */
	glw::GLfloat						actual_inner_levels[2]  = { 0.0f };
	_tessellation_shader_vertex_spacing actual_vs_mode			= run.vertex_spacing;
	glw::GLfloat						clamped_inner_levels[2] = { 0.0f };

	memcpy(actual_inner_levels, run.inner, sizeof(run.inner));

	TessellationShaderUtils::getTessellationLevelAfterVertexSpacing(
		run.vertex_spacing, actual_inner_levels[0], gl_max_tess_gen_level_value, clamped_inner_levels + 0,
		DE_NULL); /* out_clamped_and_rounded */

	TessellationShaderUtils::getTessellationLevelAfterVertexSpacing(
		run.vertex_spacing, actual_inner_levels[1], gl_max_tess_gen_level_value, clamped_inner_levels + 1,
		DE_NULL); /* out_clamped_and_rounded */

	if (run.primitive_mode == TESSELLATION_SHADER_PRIMITIVE_MODE_ISOLINES)
	{
		/* For isolines tessellation, outer[1] is subdivided as per specified vertex spacing as specified.
		 * outer[0] should be subdivided using equal vertex spacing.
		 *
		 * This is owing to the following language in the spec (* marks important subtleties):
		 *
		 * The *u==0* and *u==1* edges of the rectangle are subdivided according to the first outer
		 * tessellation level. For the purposes of *this* subdivision, the tessellation spacing mode
		 * is ignored and treated as "equal_spacing".
		 */
		TessellationShaderUtils::getTessellationLevelAfterVertexSpacing(TESSELLATION_SHADER_VERTEX_SPACING_EQUAL,
																		run.outer[0], gl_max_tess_gen_level_value,
																		DE_NULL, /* out_clamped */
																		post_vs_outer_tess_levels);

		TessellationShaderUtils::getTessellationLevelAfterVertexSpacing(
			actual_vs_mode, run.outer[1], gl_max_tess_gen_level_value, DE_NULL, /* out_clamped */
			post_vs_outer_tess_levels + 1);
	}

	if (run.primitive_mode == TESSELLATION_SHADER_PRIMITIVE_MODE_QUADS)
	{
		/* As per extension spec:
		 *
		 * if either clamped inner tessellation level is one, that tessellation level
		 * is treated as though it were originally specified as 1+epsilon, which would
		 * rounded up to result in a two- or three-segment subdivision according to the
		 * tessellation spacing.
		 *
		 **/
		if (de::abs(clamped_inner_levels[0] - 1.0f) < epsilon)
		{
			TessellationShaderUtils::getTessellationLevelAfterVertexSpacing(
				run.vertex_spacing, clamped_inner_levels[0] + 1.0f, /* epsilon */
				gl_max_tess_gen_level_value, DE_NULL,				/* out_clamped */
				actual_inner_levels + 0);
		}

		if (de::abs(clamped_inner_levels[1] - 1.0f) < epsilon)
		{
			TessellationShaderUtils::getTessellationLevelAfterVertexSpacing(
				run.vertex_spacing, clamped_inner_levels[1] + 1.0f, /* epsilon */
				gl_max_tess_gen_level_value, DE_NULL,				/* out_clamped */
				actual_inner_levels + 1);
		}
	}

	/* Retrieve tessellation level values, taking vertex spacing setting into account */
	for (int n = 0; n < 2 /* inner tessellation level values */; ++n)
	{
		TessellationShaderUtils::getTessellationLevelAfterVertexSpacing(
			actual_vs_mode, actual_inner_levels[n], gl_max_tess_gen_level_value, DE_NULL, /* out_clamped */
			post_vs_inner_tess_levels + n);
	}

	if (run.primitive_mode != TESSELLATION_SHADER_PRIMITIVE_MODE_ISOLINES)
	{
		for (int n = 0; n < 4 /* outer tessellation level values */; ++n)
		{
			TessellationShaderUtils::getTessellationLevelAfterVertexSpacing(
				actual_vs_mode, run.outer[n], gl_max_tess_gen_level_value, DE_NULL, /* out_clamped */
				post_vs_outer_tess_levels + n);
		}
	}

	/* Calculate amount of vertices that should be generated in point mode */
	switch (run.primitive_mode)
	{
	case TESSELLATION_SHADER_PRIMITIVE_MODE_ISOLINES:
	{
		n_expected_vertices = int(post_vs_outer_tess_levels[0]) * int(post_vs_outer_tess_levels[1] + 1);

		break;
	}

	case TESSELLATION_SHADER_PRIMITIVE_MODE_QUADS:
	{
		n_expected_vertices = /* outer quad */
			int(post_vs_outer_tess_levels[0]) + int(post_vs_outer_tess_levels[1]) + int(post_vs_outer_tess_levels[2]) +
			int(post_vs_outer_tess_levels[3]) +
			/* inner quad */
			(int(post_vs_inner_tess_levels[0]) - 1) * (int(post_vs_inner_tess_levels[1]) - 1);

		break;
	}

	case TESSELLATION_SHADER_PRIMITIVE_MODE_TRIANGLES:
	{
		/* If the first inner tessellation level and all three outer tessellation
		 * levels are exactly one after clamping and rounding, only a single triangle
		 * with (u,v,w) coordinates of (0,0,1), (1,0,0), and (0,1,0) is generated.
		 */
		if (de::abs(run.inner[0] - 1.0f) < epsilon && de::abs(run.outer[0] - 1.0f) < epsilon &&
			de::abs(run.outer[1] - 1.0f) < epsilon && de::abs(run.outer[2] - 1.0f) < epsilon)
		{
			n_expected_vertices = 3;
		}
		else
		{
			/* If the inner tessellation level is one and any of the outer tessellation
			 * levels is greater than one, the inner tessellation level is treated as
			 * though it were originally specified as 1+epsilon and will be rounded up to
			 * result in a two- or three-segment subdivision according to the
			 * tessellation spacing.
			 */
			if (de::abs(run.inner[0] - 1.0f) < epsilon &&
				(run.outer[0] > 1.0f || run.outer[1] > 1.0f || run.outer[2] > 1.0f))
			{
				TessellationShaderUtils::getTessellationLevelAfterVertexSpacing(
					run.vertex_spacing, 2.0f, gl_max_tess_gen_level_value, DE_NULL, /* out_clamped */
					post_vs_inner_tess_levels);
			}

			/* Count vertices making up concentric inner triangles */
			n_expected_vertices = (int)post_vs_outer_tess_levels[0] + (int)post_vs_outer_tess_levels[1] +
								  (int)post_vs_outer_tess_levels[2];

			for (int n = (int)post_vs_inner_tess_levels[0]; n >= 0; n -= 2)
			{
				/* For the outermost inner triangle, the inner triangle is degenerate -
				 * a single point at the center of the triangle -- if <n> is two.
				 */
				if (n == 2)
				{
					n_expected_vertices++; /* degenerate vertex */

					break;
				}

				/* If <n> is three, the edges of the inner triangle are not subdivided and is
				 * the final triangle in the set of concentric triangles.
				 */
				if (n == 3)
				{
					n_expected_vertices += 3 /* vertices per triangle */;

					break;
				}

				/* Otherwise, each edge of the inner triangle is divided into <n>-2 segments,
				 * with the <n>-1 vertices of this subdivision produced by intersecting the
				 * inner edge with lines perpendicular to the edge running through the <n>-1
				 * innermost vertices of the subdivision of the outer edge.
				 */
				if (n >= 2)
				{
					n_expected_vertices += (n - 2) * 3 /* triangle edges */;
				}
				else
				{
					/* Count in the degenerate point instead */
					n_expected_vertices++;
				}
			} /* for (all inner triangles) */
		}

		break;
	}

	default:
	{
		TCU_FAIL("Unrecognized primitive mode");
	}
	} /* switch (test.primitive_mode) */

	/* Compare two values */
	if (run_n_vertices != n_expected_vertices)
	{
		std::string primitive_mode = TessellationShaderUtils::getESTokenForPrimitiveMode(run.primitive_mode);
		std::string vertex_spacing = TessellationShaderUtils::getESTokenForVertexSpacingMode(run.vertex_spacing);

		m_testCtx.getLog() << tcu::TestLog::Message << run_n_vertices
						   << " vertices were generated by the tessellator instead of expected " << n_expected_vertices
						   << " for primitive mode [" << primitive_mode << "], vertex spacing mode [" << vertex_spacing
						   << "], inner tessellation levels:[" << run.inner[0] << ", " << run.inner[1]
						   << "], outer tessellation levels:[" << run.outer[0] << ", " << run.outer[1] << ", "
						   << run.outer[2] << ", " << run.outer[3] << "], point mode enabled."
						   << tcu::TestLog::EndMessage;

		TCU_FAIL("Amount of vertices generated in point mode was incorrect");
	} /* if (test.n_vertices != n_expected_vertices) */
}

/** Verifies a valid amount of duplicate vertices is present in the set of coordinates
 *  generated by the tessellator, as described by user-provided test iteration descriptor.
 *  Throws a TestError exception if the vertex set does not meet the requirements.
 *
 *  @param test           Test iteration descriptor.
 *  @param run_data       Data generated for the run.
 *  @param run_n_vertices Amount of vertices present at @param run_data.
 **/
void TessellationShaderPointsVerification::verifyCorrectAmountOfDuplicateVertices(const _run& run, const void* run_data,
																				  unsigned int run_n_vertices)
{
	const float  epsilon			  = 1e-5f;
	unsigned int n_duplicate_vertices = 0;

	for (unsigned int n_vertex_a = 0; n_vertex_a < run_n_vertices; ++n_vertex_a)
	{
		const float* vertex_a = (const float*)run_data + n_vertex_a * 3; /* components */

		for (unsigned int n_vertex_b = n_vertex_a + 1; n_vertex_b < run_n_vertices; ++n_vertex_b)
		{
			const float* vertex_b = (const float*)run_data + n_vertex_b * 3; /* components */

			if (de::abs(vertex_a[0] - vertex_b[0]) < epsilon && de::abs(vertex_a[1] - vertex_b[1]) < epsilon &&
				de::abs(vertex_a[2] - vertex_b[2]) < epsilon)
			{
				n_duplicate_vertices++;
			}
		} /* for (all vertices) */
	}	 /* for (all vertices) */

	if (n_duplicate_vertices != 0)
	{
		std::string vertex_spacing = TessellationShaderUtils::getESTokenForVertexSpacingMode(run.vertex_spacing);

		m_testCtx.getLog() << tcu::TestLog::Message << "Duplicate vertices found for the following tesselelation"
													   " configuration: tessellation level:"
													   "["
						   << run.inner[0] << ", " << run.inner[1] << "], "
																	  "outer tessellation level:"
																	  " ["
						   << run.outer[0] << ", " << run.outer[1] << ", " << run.outer[2] << ", " << run.outer[3]
						   << "], "
						   << "vertex spacing mode:[" << vertex_spacing.c_str() << "]" << tcu::TestLog::EndMessage;

		TCU_FAIL("Duplicate vertex found");
	}
}

} /* namespace glcts */
