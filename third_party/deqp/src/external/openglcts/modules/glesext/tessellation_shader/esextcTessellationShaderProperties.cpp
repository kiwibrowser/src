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

#include "esextcTessellationShaderProperties.hpp"
#include "gluContextInfo.hpp"
#include "gluDefs.hpp"
#include "glwEnums.hpp"
#include "glwFunctions.hpp"
#include "tcuTestLog.hpp"
#include <cmath>
#include <cstring>

namespace glcts
{
/** Constructor
 *
 * @param context Test context
 **/
TessellationShaderPropertiesDefaultContextWideValues::TessellationShaderPropertiesDefaultContextWideValues(
	Context& context, const ExtParameters& extParams)
	: TestCaseBase(context, extParams, "default_values_of_context_wide_properties",
				   "Verifies default values of context-wide tessellation stage properties")
{
	/* Left blank on purpose */
}

/** Executes the test.
 *
 *  Sets the test result to QP_TEST_RESULT_FAIL if the test failed, QP_TEST_RESULT_PASS otherwise.
 *
 *  Note the function throws exception should an error occur!
 *
 *  @return STOP if the test has finished, CONTINUE to indicate iterate() should be called once again.
 **/
tcu::TestNode::IterateResult TessellationShaderPropertiesDefaultContextWideValues::iterate(void)
{
	/* Do not execute if required extensions are not supported. */
	if (!m_is_tessellation_shader_supported)
	{
		throw tcu::NotSupportedError(TESSELLATION_SHADER_EXTENSION_NOT_SUPPORTED);
	}

	/* Iterate through all context-wide properties and compare expected values
	 * against the reference ones
	 */
	const float			  epsilon = (float)1e-5;
	const glw::Functions& gl	  = m_context.getRenderContext().getFunctions();

	const glw::GLint property_value_data[] = { /* pname */ /* n components */ /* default value */
											   static_cast<glw::GLint>(m_glExtTokens.PATCH_VERTICES), 1, 3,
											   /* The following values are only applicable to Desktop OpenGL. */
											   GL_PATCH_DEFAULT_OUTER_LEVEL, 4, 1, GL_PATCH_DEFAULT_INNER_LEVEL, 2, 1 };

	const unsigned int n_properties = (glu::isContextTypeES(m_context.getRenderContext().getType())) ? 1 : 3;

	for (unsigned int n_property = 0; n_property < n_properties; ++n_property)
	{
		glw::GLboolean bool_value[4]  = { GL_FALSE };
		glw::GLfloat   float_value[4] = { 0.0f };
		glw::GLint	 int_value[4]   = { 0 };

		glw::GLenum pname		   = property_value_data[n_property * 3 + 0];
		glw::GLint  n_components   = property_value_data[n_property * 3 + 1];
		glw::GLint  expected_value = property_value_data[n_property * 3 + 2];

		/* Call all relevant getters */
		gl.getBooleanv(pname, bool_value);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGetBooleanv() failed.");

		gl.getFloatv(pname, float_value);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGetFloatv() failed.");

		gl.getIntegerv(pname, int_value);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv() failed.");

		/* Compare retrieved vector value components against expected value */
		glw::GLboolean expected_bool_value[4] = {
			(expected_value != 0) ? (glw::GLboolean)GL_TRUE : (glw::GLboolean)GL_FALSE,
			(expected_value != 0) ? (glw::GLboolean)GL_TRUE : (glw::GLboolean)GL_FALSE,
			(expected_value != 0) ? (glw::GLboolean)GL_TRUE : (glw::GLboolean)GL_FALSE,
			(expected_value != 0) ? (glw::GLboolean)GL_TRUE : (glw::GLboolean)GL_FALSE
		};
		glw::GLint expected_int_value[4] = { expected_value, expected_value, expected_value, expected_value };

		if (memcmp(expected_bool_value, bool_value, sizeof(bool) * n_components) != 0)
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "glGetBooleanv() called for pname " << pname
							   << " reported invalid value." << tcu::TestLog::EndMessage;

			TCU_FAIL("Invalid value reported by glGetBooleanv()");
		}

		if (memcmp(expected_int_value, int_value, sizeof(int) * n_components) != 0)
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "glGetIntegerv() called for pname " << pname
							   << " reported invalid value." << tcu::TestLog::EndMessage;

			TCU_FAIL("Invalid value reported by glGetIntegerv()");
		}

		if ((n_components >= 1 && de::abs(float_value[0] - (float)expected_value) > epsilon) ||
			(n_components >= 2 && de::abs(float_value[1] - (float)expected_value) > epsilon) ||
			(n_components >= 3 && de::abs(float_value[2] - (float)expected_value) > epsilon) ||
			(n_components >= 4 && de::abs(float_value[3] - (float)expected_value) > epsilon))
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "glGetFloatv() called for pname " << pname
							   << " reported invalid value." << tcu::TestLog::EndMessage;

			TCU_FAIL("Invalid value reported by glGetFloatv()");
		}
	} /* for (all properties) */

	/* All done */
	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	return STOP;
}

/** Constructor
 *
 * @param context Test context
 **/
TessellationShaderPropertiesProgramObject::TessellationShaderPropertiesProgramObject(Context&			  context,
																					 const ExtParameters& extParams)
	: TestCaseBase(context, extParams, "program_object_properties",
				   "Verifies tessellation-specific properties of program objects are reported correctly.")
	, m_fs_id(0)
	, m_po_id(0)
	, m_tc_id(0)
	, m_te_id(0)
	, m_vs_id(0)
{
	/* Left blank on purpose */
}

/** Deinitializes ES objects created for the test */
void TessellationShaderPropertiesProgramObject::deinit(void)
{
	/* Call base class' deinit() */
	TestCaseBase::deinit();

	/* Release any ES objects created */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (m_fs_id != 0)
	{
		gl.deleteShader(m_fs_id);

		m_fs_id = 0;
	}

	if (m_po_id != 0)
	{
		gl.deleteProgram(m_po_id);

		m_po_id = 0;
	}

	if (m_tc_id != 0)
	{
		gl.deleteShader(m_tc_id);

		m_tc_id = 0;
	}

	if (m_te_id != 0)
	{
		gl.deleteShader(m_te_id);

		m_te_id = 0;
	}

	if (m_vs_id != 0)
	{
		gl.deleteShader(m_vs_id);

		m_vs_id = 0;
	}
}

/** Initializes ES objects necessary to execute the test */
void TessellationShaderPropertiesProgramObject::initTest(void)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Do not execute if required extension is not supported */
	if (!m_is_tessellation_shader_supported)
	{
		return;
	}

	/* Generate all objects */
	m_po_id = gl.createProgram();
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateProgram() failed");

	m_fs_id = gl.createShader(GL_FRAGMENT_SHADER);
	m_tc_id = gl.createShader(m_glExtTokens.TESS_CONTROL_SHADER);
	m_te_id = gl.createShader(m_glExtTokens.TESS_EVALUATION_SHADER);
	m_vs_id = gl.createShader(GL_VERTEX_SHADER);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateShader() failed");

	/* Attach the shader to the program object */
	gl.attachShader(m_po_id, m_fs_id);
	gl.attachShader(m_po_id, m_tc_id);
	gl.attachShader(m_po_id, m_te_id);
	gl.attachShader(m_po_id, m_vs_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glAttachShader() failed");

	/* Since this test does not care much about fragment & vertex shaders, set
	 * their bodies and compile these shaders now */
	const char* fs_body = "${VERSION}\n"
						  "\n"
						  "void main()\n"
						  "{\n"
						  "}\n";

	const char* vs_body = "${VERSION}\n"
						  "\n"
						  "void main()\n"
						  "{\n"
						  "}\n";

	glw::GLint fs_compile_status = GL_FALSE;
	glw::GLint vs_compile_status = GL_FALSE;

	shaderSourceSpecialized(m_fs_id, 1 /* count */, &fs_body);
	shaderSourceSpecialized(m_vs_id, 1 /* count */, &vs_body);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glShaderSource() failed");

	gl.compileShader(m_fs_id);
	gl.compileShader(m_vs_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCompileShader() failed");

	gl.getShaderiv(m_fs_id, GL_COMPILE_STATUS, &fs_compile_status);
	gl.getShaderiv(m_vs_id, GL_COMPILE_STATUS, &vs_compile_status);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetShaderiv() failed");

	if (fs_compile_status != GL_TRUE)
	{
		TCU_FAIL("Could not compile fragment shader");
	}

	if (vs_compile_status != GL_TRUE)
	{
		TCU_FAIL("Could not compile vertex shader");
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
tcu::TestNode::IterateResult TessellationShaderPropertiesProgramObject::iterate(void)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Do not execute if required extensions are not supported. */
	if (!m_is_tessellation_shader_supported)
	{
		throw tcu::NotSupportedError(TESSELLATION_SHADER_EXTENSION_NOT_SUPPORTED);
	}

	/* Initialize ES test objects */
	initTest();

	/* Test 1: Default values. Values as per spec, define as little qualifiers as possible */
	_test_descriptor test_1;

	test_1.expected_control_output_vertices_value = 4;
	test_1.expected_gen_mode_value				  = m_glExtTokens.QUADS;
	test_1.expected_gen_point_mode_value		  = GL_FALSE;
	test_1.expected_gen_spacing_value			  = GL_EQUAL;
	test_1.expected_gen_vertex_order_value		  = GL_CCW;
	test_1.tc_body								  = "${VERSION}\n"
					 "${TESSELLATION_SHADER_REQUIRE}\n"
					 "\n"
					 "layout(vertices=4) out;\n"
					 "\n"
					 "void main()\n"
					 "{\n"
					 "    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;\n"
					 "}\n";
	test_1.te_body = "${VERSION}\n"
					 "${TESSELLATION_SHADER_REQUIRE}\n"
					 "\n"
					 "layout(quads) in;\n"
					 "\n"
					 "void main()\n"
					 "{\n"
					 "    gl_Position = gl_in[0].gl_Position;\n"
					 "}\n";

	/* Test 2: 16 vertices per patch + isolines + fractional_even_spacing + cw combination */
	_test_descriptor test_2;

	test_2.expected_control_output_vertices_value = 16;
	test_2.expected_gen_mode_value				  = m_glExtTokens.ISOLINES;
	test_2.expected_gen_point_mode_value		  = GL_FALSE;
	test_2.expected_gen_spacing_value			  = m_glExtTokens.FRACTIONAL_EVEN;
	test_2.expected_gen_vertex_order_value		  = GL_CW;
	test_2.tc_body								  = "${VERSION}\n"
					 "${TESSELLATION_SHADER_REQUIRE}\n"
					 "\n"
					 "layout(vertices=16) out;\n"
					 "\n"
					 "void main()\n"
					 "{\n"
					 "    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;\n"
					 "}\n";
	test_2.te_body = "${VERSION}\n"
					 "${TESSELLATION_SHADER_REQUIRE}\n"
					 "\n"
					 "layout(isolines, fractional_even_spacing, cw) in;\n"
					 "\n"
					 "void main()\n"
					 "{\n"
					 "    gl_Position = gl_in[0].gl_Position;\n"
					 "}\n";

	/* Test 3: 32 vertices per patch + triangles + fractional_odd_spacing + ccw combination + point mode*/
	_test_descriptor test_3;

	test_3.expected_control_output_vertices_value = 32;
	test_3.expected_gen_mode_value				  = GL_TRIANGLES;
	test_3.expected_gen_point_mode_value		  = GL_TRUE;
	test_3.expected_gen_spacing_value			  = m_glExtTokens.FRACTIONAL_ODD;
	test_3.expected_gen_vertex_order_value		  = GL_CCW;
	test_3.tc_body								  = "${VERSION}\n"
					 "${TESSELLATION_SHADER_REQUIRE}\n"
					 "\n"
					 "layout(vertices=32) out;\n"
					 "\n"
					 "void main()\n"
					 "{\n"
					 "    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;\n"
					 "}\n";
	test_3.te_body = "${VERSION}\n"
					 "${TESSELLATION_SHADER_REQUIRE}\n"
					 "\n"
					 "layout(triangles, fractional_odd_spacing, ccw, point_mode) in;\n"
					 "\n"
					 "void main()\n"
					 "{\n"
					 "    gl_Position = gl_in[0].gl_Position;\n"
					 "}\n";

	/* Test 4: 8 vertices per patch + quads + equal_spacing + ccw combination + point mode*/
	_test_descriptor test_4;

	test_4.expected_control_output_vertices_value = 8;
	test_4.expected_gen_mode_value				  = m_glExtTokens.QUADS;
	test_4.expected_gen_point_mode_value		  = GL_TRUE;
	test_4.expected_gen_spacing_value			  = GL_EQUAL;
	test_4.expected_gen_vertex_order_value		  = GL_CCW;
	test_4.tc_body								  = "${VERSION}\n"
					 "${TESSELLATION_SHADER_REQUIRE}\n"
					 "\n"
					 "layout(vertices=8) out;\n"
					 "\n"
					 "void main()\n"
					 "{\n"
					 "    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;\n"
					 "}\n";
	test_4.te_body = "${VERSION}\n"
					 "${TESSELLATION_SHADER_REQUIRE}\n"
					 "\n"
					 "layout(quads, equal_spacing, ccw, point_mode) in;\n"
					 "\n"
					 "void main()\n"
					 "{\n"
					 "    gl_Position = gl_in[0].gl_Position;\n"
					 "}\n";

	/* Store all tests in a single vector */
	_tests tests;

	tests.push_back(test_1);
	tests.push_back(test_2);
	tests.push_back(test_3);
	tests.push_back(test_4);

	/* Iterate through all the tests and verify the values reported */
	for (_tests_const_iterator test_iterator = tests.begin(); test_iterator != tests.end(); test_iterator++)
	{
		const _test_descriptor& test = *test_iterator;

		/* Set tessellation control & evaluation shader bodies. */
		shaderSourceSpecialized(m_tc_id, 1 /* count */, &test.tc_body);
		shaderSourceSpecialized(m_te_id, 1 /* count */, &test.te_body);

		GLU_EXPECT_NO_ERROR(gl.getError(), "glShaderSource() failed");

		/* Compile the shaders */
		gl.compileShader(m_tc_id);
		gl.compileShader(m_te_id);

		GLU_EXPECT_NO_ERROR(gl.getError(), "glCompileShader() failed");

		/* Make sure the shaders compiled */
		glw::GLint tc_compile_status = GL_FALSE;
		glw::GLint te_compile_status = GL_FALSE;

		gl.getShaderiv(m_tc_id, GL_COMPILE_STATUS, &tc_compile_status);
		gl.getShaderiv(m_te_id, GL_COMPILE_STATUS, &te_compile_status);

		GLU_EXPECT_NO_ERROR(gl.getError(), "glGetShaderiv() failed");

		if (tc_compile_status != GL_TRUE)
		{
			TCU_FAIL("Could not compile tessellation control shader");
		}

		if (te_compile_status != GL_TRUE)
		{
			TCU_FAIL("Could not compile tessellation evaluation shader");
		}

		/* Try to link the program object */
		glw::GLint link_status = GL_FALSE;

		gl.linkProgram(m_po_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glLinkProgram() failed");

		gl.getProgramiv(m_po_id, GL_LINK_STATUS, &link_status);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGetProgramiv() failed");

		if (link_status != GL_TRUE)
		{
			TCU_FAIL("Program linking failed");
		}

		/* Query the tessellation properties of the program object and make sure
		 * the values reported are valid */
		glw::GLint control_output_vertices_value = 0;
		glw::GLint gen_mode_value				 = GL_NONE;
		glw::GLint gen_spacing_value			 = GL_NONE;
		glw::GLint gen_vertex_order_value		 = GL_NONE;
		glw::GLint gen_point_mode_value			 = GL_NONE;

		gl.getProgramiv(m_po_id, m_glExtTokens.TESS_CONTROL_OUTPUT_VERTICES, &control_output_vertices_value);
		gl.getProgramiv(m_po_id, m_glExtTokens.TESS_GEN_MODE, &gen_mode_value);
		gl.getProgramiv(m_po_id, m_glExtTokens.TESS_GEN_SPACING, &gen_spacing_value);
		gl.getProgramiv(m_po_id, m_glExtTokens.TESS_GEN_POINT_MODE, &gen_point_mode_value);
		gl.getProgramiv(m_po_id, m_glExtTokens.TESS_GEN_VERTEX_ORDER, &gen_vertex_order_value);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGetProgramiv() for tessellation-specific properties failed.");

		if (control_output_vertices_value != test.expected_control_output_vertices_value)
		{
			m_testCtx.getLog() << tcu::TestLog::Message
							   << "Invalid value reported for GL_TESS_CONTROL_OUTPUT_VERTICES_EXT property; "
							   << " expected: " << test.expected_control_output_vertices_value
							   << ", retrieved: " << control_output_vertices_value << tcu::TestLog::EndMessage;

			TCU_FAIL("Invalid value reported for GL_TESS_CONTROL_OUTPUT_VERTICES_EXT property.");
		}

		if ((glw::GLuint)gen_mode_value != test.expected_gen_mode_value)
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "Invalid value reported for GL_TESS_GEN_MODE_EXT property; "
							   << " expected: " << test.expected_gen_mode_value << ", retrieved: " << gen_mode_value
							   << tcu::TestLog::EndMessage;

			TCU_FAIL("Invalid value reported for GL_TESS_GEN_MODE_EXT property.");
		}

		if ((glw::GLuint)gen_spacing_value != test.expected_gen_spacing_value)
		{
			m_testCtx.getLog() << tcu::TestLog::Message
							   << "Invalid value reported for GL_TESS_GEN_SPACING_EXT property; "
							   << " expected: " << test.expected_gen_spacing_value
							   << ", retrieved: " << gen_spacing_value << tcu::TestLog::EndMessage;

			TCU_FAIL("Invalid value reported for GL_TESS_GEN_SPACING_EXT property.");
		}

		if ((glw::GLuint)gen_point_mode_value != test.expected_gen_point_mode_value)
		{
			m_testCtx.getLog() << tcu::TestLog::Message
							   << "Invalid value reported for GL_TESS_GEN_POINT_MODE_EXT property; "
							   << " expected: " << test.expected_gen_point_mode_value
							   << ", retrieved: " << gen_point_mode_value << tcu::TestLog::EndMessage;

			TCU_FAIL("Invalid value reported for GL_TESS_GEN_POINT_MODE_EXT property.");
		}

		if ((glw::GLuint)gen_vertex_order_value != test.expected_gen_vertex_order_value)
		{
			m_testCtx.getLog() << tcu::TestLog::Message
							   << "Invalid value reported for GL_TESS_GEN_VERTEX_ORDER_EXT property; "
							   << " expected: " << test.expected_gen_vertex_order_value
							   << ", retrieved: " << gen_vertex_order_value << tcu::TestLog::EndMessage;

			TCU_FAIL("Invalid value reported for GL_TESS_GEN_VERTEX_ORDER_EXT property.");
		}
	} /* for (all test descriptors) */

	/* All done */
	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	return STOP;
}

} /* namespace glcts */
