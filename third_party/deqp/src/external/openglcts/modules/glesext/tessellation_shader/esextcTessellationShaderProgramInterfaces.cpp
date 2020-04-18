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

#include "esextcTessellationShaderProgramInterfaces.hpp"
#include "gluContextInfo.hpp"
#include "gluDefs.hpp"
#include "glwEnums.hpp"
#include "glwFunctions.hpp"
#include "tcuTestLog.hpp"

namespace glcts
{
/** Constructor
 *
 * @param context       Test context
 * @param name          Test case's name
 * @param description   Test case's desricption
 **/
TessellationShaderProgramInterfaces::TessellationShaderProgramInterfaces(Context&			  context,
																		 const ExtParameters& extParams)
	: TestCaseBase(context, extParams, "ext_program_interface_query_dependency",
				   "Verifies EXT_program_interface_query works correctly for tessellation"
				   " control and tessellation evaluation shaders")
	, m_fs_shader_id(0)
	, m_po_id(0)
	, m_tc_shader_id(0)
	, m_te_shader_id(0)
	, m_vs_shader_id(0)
	, m_is_atomic_counters_supported(false)
	, m_is_shader_storage_blocks_supported(false)
{
	/* Left blank on purpose */
}

/** Deinitializes all ES objects created for the test. */
void TessellationShaderProgramInterfaces::deinit()
{
	/** Call base class' deinit() function */
	TestCaseBase::deinit();

	/* Release all objects we might've created */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (m_fs_shader_id != 0)
	{
		gl.deleteShader(m_fs_shader_id);

		m_fs_shader_id = 0;
	}

	if (m_po_id != 0)
	{
		gl.deleteProgram(m_po_id);

		m_po_id = 0;
	}

	if (m_tc_shader_id != 0)
	{
		gl.deleteShader(m_tc_shader_id);

		m_tc_shader_id = 0;
	}

	if (m_te_shader_id != 0)
	{
		gl.deleteShader(m_te_shader_id);

		m_te_shader_id = 0;
	}

	if (m_vs_shader_id != 0)
	{
		gl.deleteShader(m_vs_shader_id);

		m_vs_shader_id = 0;
	}
}

/** Initializes all ES objects that will be used for the test. */
void TessellationShaderProgramInterfaces::initTest()
{
	/* The test requires EXT_tessellation_shader and EXT_program_interfaces_query extensions */
	if (!m_is_tessellation_shader_supported || !m_is_program_interface_query_supported)
	{
		return;
	}

	/* Generate a program object we will later configure */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	m_po_id = gl.createProgram();

	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateProgram() failed");

	/* Generate shader objects the test will use */
	m_fs_shader_id = gl.createShader(GL_FRAGMENT_SHADER);
	m_tc_shader_id = gl.createShader(m_glExtTokens.TESS_CONTROL_SHADER);
	m_te_shader_id = gl.createShader(m_glExtTokens.TESS_EVALUATION_SHADER);
	m_vs_shader_id = gl.createShader(GL_VERTEX_SHADER);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateShader() failed");

	glw::GLint gl_max_tess_control_shader_storage_blocks;
	glw::GLint gl_max_tess_control_atomic_counter_buffers;
	glw::GLint gl_max_tess_control_atomic_counters;
	glw::GLint gl_max_tess_evaluation_shader_storage_blocks;
	glw::GLint gl_max_tess_evaluation_atomic_counter_buffers;
	glw::GLint gl_max_tess_evaluation_atomic_counters;

	gl.getIntegerv(m_glExtTokens.MAX_TESS_CONTROL_ATOMIC_COUNTER_BUFFERS, &gl_max_tess_control_atomic_counter_buffers);
	GLU_EXPECT_NO_ERROR(gl.getError(),
						"glGetIntegerv() failed for GL_MAX_TESS_CONTROL_ATOMIC_COUNTER_BUFFERS_EXT pname");
	gl.getIntegerv(m_glExtTokens.MAX_TESS_CONTROL_ATOMIC_COUNTERS, &gl_max_tess_control_atomic_counters);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv() failed for GL_MAX_TESS_CONTROL_ATOMIC_COUNTERS_EXT pname");
	gl.getIntegerv(m_glExtTokens.MAX_TESS_CONTROL_SHADER_STORAGE_BLOCKS, &gl_max_tess_control_shader_storage_blocks);
	GLU_EXPECT_NO_ERROR(gl.getError(),
						"glGetIntegerv() failed for GL_MAX_TESS_CONTROL_SHADER_STORAGE_BLOCKS_EXT pname");
	gl.getIntegerv(m_glExtTokens.MAX_TESS_EVALUATION_ATOMIC_COUNTER_BUFFERS,
				   &gl_max_tess_evaluation_atomic_counter_buffers);
	GLU_EXPECT_NO_ERROR(gl.getError(),
						"glGetIntegerv() failed for GL_MAX_TESS_EVALUATION_ATOMIC_COUNTER_BUFFERS_EXT pname");
	gl.getIntegerv(m_glExtTokens.MAX_TESS_EVALUATION_ATOMIC_COUNTERS, &gl_max_tess_evaluation_atomic_counters);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv() failed for GL_MAX_TESS_EVALUATION_ATOMIC_COUNTERS_EXT pname");
	gl.getIntegerv(m_glExtTokens.MAX_TESS_EVALUATION_SHADER_STORAGE_BLOCKS,
				   &gl_max_tess_evaluation_shader_storage_blocks);
	GLU_EXPECT_NO_ERROR(gl.getError(),
						"glGetIntegerv() failed for GL_MAX_TESS_EVALUATION_SHADER_STORAGE_BLOCKS_EXT pname");

	m_is_atomic_counters_supported =
		(gl_max_tess_control_atomic_counter_buffers > 1) && (gl_max_tess_evaluation_atomic_counter_buffers > 1) &&
		(gl_max_tess_control_atomic_counters > 0) && (gl_max_tess_evaluation_atomic_counters > 0);

	m_is_shader_storage_blocks_supported =
		(gl_max_tess_control_shader_storage_blocks > 0) && (gl_max_tess_evaluation_shader_storage_blocks > 0);

	const char* atomic_counters_header = NULL;
	if (m_is_atomic_counters_supported)
	{
		atomic_counters_header = "#define USE_ATOMIC_COUNTERS 1\n";
	}
	else
	{
		atomic_counters_header = "\n";
	}

	const char* shader_storage_blocks_header = NULL;
	if (m_is_shader_storage_blocks_supported)
	{
		shader_storage_blocks_header = "#define USE_SHADER_STORAGE_BLOCKS\n";
	}
	else
	{
		shader_storage_blocks_header = "\n";
	}

	/* Build shader program */
	const char* fs_body = "${VERSION}\n"
						  "\n"
						  "precision highp float;\n"
						  "\n"
						  "out vec4 test_output;\n"
						  "\n"
						  "void main()\n"
						  "{\n"
						  "    test_output = vec4(1, 0, 0, 0);\n"
						  "}\n";

	const char* tc_body = "\n"
						  "layout (vertices = 1) out;\n"
						  "\n"
						  /* Uniforms */
						  "uniform vec2 tc_uniform1;\n"
						  "uniform mat4 tc_uniform2;\n"
						  "\n"
						  /* Uniform blocks */
						  "uniform tc_uniform_block1\n"
						  "{\n"
						  "    float tc_uniform_block1_1;\n"
						  "};\n"
						  /* Atomic counter buffers */
						  "#ifdef USE_ATOMIC_COUNTERS\n"
						  "layout(binding = 1, offset = 0) uniform atomic_uint tc_atomic_counter1;\n"
						  "#endif\n"
						  /* Shader storage blocks & buffer variables */
						  "#ifdef USE_SHADER_STORAGE_BLOCKS\n"
						  "layout(std140, binding = 0) buffer tc_shader_storage_block1\n"
						  "{\n"
						  "    vec4 tc_shader_storage_buffer_object_1[];\n"
						  "};\n"
						  "#endif\n"
						  /* Body */
						  "void main()\n"
						  "{\n"
						  "    int test = 1;\n"
						  "\n"
						  /* Uniforms */
						  "    if (tc_uniform1.x    == 0.0) test = 2;\n"
						  "    if (tc_uniform2[0].y == 1.0) test = 3;\n"
						  /* Uniform blocks */
						  "    if (tc_uniform_block1_1 == 3.0) test = 4;\n"
						  /* Atomic counter buffers */
						  "#ifdef USE_ATOMIC_COUNTERS\n"
						  "    if (atomicCounter(tc_atomic_counter1) == 1u) test = 5;\n"
						  "#endif\n"
						  /* Shader storage blocks & buffer variables */
						  "#ifdef USE_SHADER_STORAGE_BLOCKS\n"
						  "    if (tc_shader_storage_buffer_object_1[0].x == 0.0) test = 6;\n"
						  "#endif\n"
						  "\n"
						  "    gl_out           [gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;\n"
						  "    gl_TessLevelOuter[0]                           = 2.0 * float(test);\n"
						  "    gl_TessLevelOuter[1]                           = 3.0;\n"
						  "}\n";

	const char* tc_code[] = { "${VERSION}\n",
							  /* Required EXT_tessellation_shader functionality */
							  "${TESSELLATION_SHADER_REQUIRE}\n", atomic_counters_header, shader_storage_blocks_header,
							  tc_body };

	const char* te_body = "\n"
						  "layout (isolines, ccw, equal_spacing, point_mode) in;\n"
						  "\n"
						  /* Uniforms */
						  "uniform vec2 te_uniform1;\n"
						  "uniform mat4 te_uniform2;\n"
						  "\n"
						  /* Uniform blocks */
						  "uniform te_uniform_block1\n"
						  "{\n"
						  "    float te_uniform_block1_1;\n"
						  "};\n"
						  /* Atomic counter buffers */
						  "#ifdef USE_ATOMIC_COUNTERS\n"
						  "layout(binding = 2, offset = 0) uniform atomic_uint te_atomic_counter1;\n"
						  "#endif\n"
						  /* Shader storage blocks & buffer variables */
						  "#ifdef USE_SHADER_STORAGE_BLOCKS\n"
						  "layout(std140, binding = 0) buffer te_shader_storage_block1\n"
						  "{\n"
						  "    vec4 te_shader_storage_buffer_object_1[];\n"
						  "};\n"
						  "#endif\n"
						  "void main()\n"
						  "{\n"
						  "    int test = 1;\n"
						  "\n"
						  /* Uniforms */
						  "    if (te_uniform1.x    == 0.0) test = 2;\n"
						  "    if (te_uniform2[0].y == 1.0) test = 3;\n"
						  /* Uniform blocks */
						  "    if (te_uniform_block1_1 == 3.0) test = 4;\n"
						  /* Atomic counter buffers */
						  "#ifdef USE_ATOMIC_COUNTERS\n"
						  "    if (atomicCounter(te_atomic_counter1) == 1u) test = 5;\n"
						  "#endif\n"
						  /* Shader storage blocks & buffer variables */
						  "#ifdef USE_SHADER_STORAGE_BLOCKS\n"
						  "   if (te_shader_storage_buffer_object_1[0].x == 0.0) test = 6;\n"
						  "#endif\n"
						  "\n"
						  "    gl_Position = gl_in[0].gl_Position * float(test);\n"
						  "}\n";

	const char* te_code[] = { "${VERSION}\n",
							  /* Required EXT_tessellation_shader functionality */
							  "${TESSELLATION_SHADER_REQUIRE}\n", atomic_counters_header, shader_storage_blocks_header,
							  te_body };

	const char* vs_body = "${VERSION}\n"
						  "\n"
						  "in vec4 test_input;\n"
						  "\n"
						  "void main()\n"
						  "{\n"
						  "    gl_Position = vec4(gl_VertexID, test_input.y, 0, 1);\n"
						  "}\n";

	bool link_success = buildProgram(m_po_id, m_fs_shader_id, 1, &fs_body, m_tc_shader_id, 5, tc_code, m_te_shader_id,
									 5, te_code, m_vs_shader_id, 1, &vs_body);

	if (!link_success)
	{
		TCU_FAIL("Program compilation and linking failed");
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
tcu::TestNode::IterateResult TessellationShaderProgramInterfaces::iterate(void)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Do not execute if required extensions are not supported. */
	if (!m_is_tessellation_shader_supported)
	{
		throw tcu::NotSupportedError(TESSELLATION_SHADER_EXTENSION_NOT_SUPPORTED);
	}

	/* Initialize ES objects needed to run the test */
	initTest();

	/* Iterate through all interfaces */
	const glw::GLenum interfaces[] = {
		GL_UNIFORM,			GL_UNIFORM_BLOCK, GL_ATOMIC_COUNTER_BUFFER, GL_SHADER_STORAGE_BLOCK,
		GL_BUFFER_VARIABLE, GL_PROGRAM_INPUT, GL_PROGRAM_OUTPUT
	};
	const unsigned int n_interfaces = sizeof(interfaces) / sizeof(interfaces[0]);

	for (unsigned int n_interface = 0; n_interface < n_interfaces; ++n_interface)
	{
		glw::GLenum interface = interfaces[n_interface];

		if ((interface == GL_SHADER_STORAGE_BLOCK || interface == GL_BUFFER_VARIABLE) &&
			!m_is_shader_storage_blocks_supported)
		{
			continue;
		}

		if (interface == GL_ATOMIC_COUNTER_BUFFER && !m_is_atomic_counters_supported)
		{
			continue;
		}

		/* For each interface, we want to check whether a specific resource
		 * is recognized by the implementation. If the name is unknown,
		 * the test should fail; if it's recognized, we should verify it's referenced
		 * by both TC and TE shaders
		 */
		const char* tc_resource_name = DE_NULL;
		const char* te_resource_name = DE_NULL;

		switch (interface)
		{
		case GL_UNIFORM:
		{
			tc_resource_name = "tc_uniform1";
			te_resource_name = "te_uniform1";

			break;
		}

		case GL_UNIFORM_BLOCK:
		{
			tc_resource_name = "tc_uniform_block1";
			te_resource_name = "te_uniform_block1";

			break;
		}

		case GL_ATOMIC_COUNTER_BUFFER:
		{
			/* Atomic counter buffers are tested in a separate codepath. */
			break;
		}

		case GL_SHADER_STORAGE_BLOCK:
		{
			tc_resource_name = "tc_shader_storage_block1";
			te_resource_name = "te_shader_storage_block1";

			break;
		}

		case GL_BUFFER_VARIABLE:
		{
			tc_resource_name = "tc_shader_storage_buffer_object_1";
			te_resource_name = "te_shader_storage_buffer_object_1";

			break;
		}

		case GL_PROGRAM_INPUT:
		{
			tc_resource_name = DE_NULL;
			te_resource_name = DE_NULL;

			break;
		}

		case GL_PROGRAM_OUTPUT:
		{
			tc_resource_name = DE_NULL;
			te_resource_name = DE_NULL;

			break;
		}

		default:
		{
			TCU_FAIL("Unrecognized interface type");
		}
		} /* switch (interface) */

		/* Run in two iterations - first for TC, then for TE */
		for (int n_iteration = 0; n_iteration < 2; ++n_iteration)
		{
			glw::GLenum property = (n_iteration == 0) ? m_glExtTokens.REFERENCED_BY_TESS_CONTROL_SHADER :
														m_glExtTokens.REFERENCED_BY_TESS_EVALUATION_SHADER;

			if (interface == GL_ATOMIC_COUNTER_BUFFER)
			{
				/* We only need a single iteration run for this interface */
				if (n_iteration == 1)
				{
					continue;
				}

				/* Atomic counter buffers are not assigned names, hence they need to be
				 * tested slightly differently.
				 *
				 * Exactly two atomic counter buffers should be defined. Make sure that's the case.
				 */
				glw::GLint n_active_resources = 0;

				gl.getProgramInterfaceiv(m_po_id, interface, GL_ACTIVE_RESOURCES, &n_active_resources);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glGetProgramInterfaceiv() failed.");

				if (n_active_resources != 2)
				{
					TCU_FAIL("Invalid amount of atomic counter buffer binding points reported");
				}

				/* Check both resources and make sure they report separate atomic counters */
				bool was_tc_atomic_counter_buffer_reported = false;
				bool was_te_atomic_counter_buffer_reported = false;

				for (int n_resource = 0; n_resource < n_active_resources; ++n_resource)
				{
					const glw::GLenum tc_property		= m_glExtTokens.REFERENCED_BY_TESS_CONTROL_SHADER;
					glw::GLint		  tc_property_value = 0;
					const glw::GLenum te_property		= m_glExtTokens.REFERENCED_BY_TESS_EVALUATION_SHADER;
					glw::GLint		  te_property_value = 0;

					gl.getProgramResourceiv(m_po_id, interface, n_resource, 1, /* propCount */
											&tc_property, 1,				   /* bufSize */
											NULL,							   /* length */
											&tc_property_value);
					GLU_EXPECT_NO_ERROR(
						gl.getError(),
						"glGetProgramResourceiv() failed for GL_REFERENCED_BY_TESS_CONTROL_SHADER_EXT property");

					gl.getProgramResourceiv(m_po_id, interface, n_resource, 1, /* propCount */
											&te_property, 1,				   /* bufSize */
											NULL,							   /* length */
											&te_property_value);
					GLU_EXPECT_NO_ERROR(
						gl.getError(),
						"glGetProgramResourceiv() failed for GL_REFERENCED_BY_TESS_EVALUATION_SHADER_EXT property");

					if (tc_property_value == GL_TRUE)
					{
						if (was_tc_atomic_counter_buffer_reported)
						{
							TCU_FAIL("Tessellation control-specific atomic counter buffer is reported twice");
						}

						was_tc_atomic_counter_buffer_reported = true;
					}

					if (te_property_value == GL_TRUE)
					{
						if (was_te_atomic_counter_buffer_reported)
						{
							TCU_FAIL("Tessellation evaluation-specific atomic counter buffer is reported twice");
						}

						was_te_atomic_counter_buffer_reported = true;
					}
				} /* for (all active resources) */

				if (!was_tc_atomic_counter_buffer_reported || !was_te_atomic_counter_buffer_reported)
				{
					TCU_FAIL("Either tessellation control or tessellation evaluation atomic counter buffer was not "
							 "reported");
				}
			}
			else
			{
				/* Retrieve resource index first, as long as the name is not NULL.
				 * If it's NULL, the property's value is assumed to be GL_FALSE for
				 * all reported active resources.
				 **/
				const char* resource_name = (n_iteration == 0) ? tc_resource_name : te_resource_name;

				if (resource_name == DE_NULL)
				{
					/* Make sure the property has GL_FALSE value for any resources
					 * reported for this interface. */
					glw::GLint n_active_resources = 0;

					gl.getProgramInterfaceiv(m_po_id, interface, GL_ACTIVE_RESOURCES, &n_active_resources);
					GLU_EXPECT_NO_ERROR(gl.getError(), "glGetProgramInterfaceiv() failed.");

					for (glw::GLint n_resource = 0; n_resource < n_active_resources; ++n_resource)
					{
						verifyPropertyValue(interface, property, n_resource, GL_FALSE);
					} /* for (all resource indices) */
				}
				else
				{
					glw::GLuint resource_index = gl.getProgramResourceIndex(m_po_id, interface, resource_name);
					GLU_EXPECT_NO_ERROR(gl.getError(), "glGetProgramResourceIndex() failed");

					if (resource_index == GL_INVALID_INDEX)
					{
						m_testCtx.getLog() << tcu::TestLog::Message << "Resource [" << resource_name
										   << "] was not recognized." << tcu::TestLog::EndMessage;

						TCU_FAIL("Resource not recognized.");
					}

					/* Now that we know the index, we can check the GL_REFERENCED_BY_...
					 * property value */
					verifyPropertyValue(interface, property, resource_index, GL_TRUE);
				}
			} /* (interface != GL_ATOMIC_COUNTER_BUFFER) */
		}	 /* for (both iterations) */
	}		  /* for (all interfaces) */

	/* All done */
	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	return STOP;
}

/** Checks if a property value reported for user-specified program object interface
 *  at given index is as expected.
 *
 *  NOTE: This function throws TestError exception if retrieved value does not
 *        match @param expected_value.
 *
 *  @param interface      Program object interface to use for the query;
 *  @param property       Interface property to check;
 *  @param index          Property index to use for the test;
 *  @param expected_value Value that is expected to be reported by ES implementation.
 **/
void TessellationShaderProgramInterfaces::verifyPropertyValue(glw::GLenum interface, glw::GLenum property,
															  glw::GLuint index, glw::GLint expected_value)
{
	const glw::Functions& gl			 = m_context.getRenderContext().getFunctions();
	glw::GLint			  property_value = 0;

	gl.getProgramResourceiv(m_po_id, interface, index, 1, /* propCount */
							&property, 1,				  /* bufSize */
							NULL,						  /* length */
							&property_value);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetProgramResourceiv() failed");

	if (property_value != expected_value)
	{
		TCU_FAIL("Invalid GL_REFERENCED_BY_... property value reported");
	}
}

} /* namespace glcts */
