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

#include "esextcGeometryShaderProgramResource.hpp"
#include "gluContextInfo.hpp"
#include "glwEnums.hpp"
#include "glwFunctions.hpp"
#include "tcuTestLog.hpp"

namespace glcts
{
/* Shared shaders' code */
const char* const GeometryShaderProgramResourceTest::m_common_shader_code_definitions_body =
	"// uniforms\n"
	"uniform mat4 uni_model_view_projection; // not referenced in GS\n"
	"uniform vec4 uni_colors_white;          // referenced in GS\n"
	"\n"
	"// uniforms blocks\n"
	"uniform Matrices\n"
	"{\n"
	"    mat4 model;\n"
	"    mat4 view;\n"
	"    mat4 projection;\n"
	"} uni_matrices; // not referenced at all\n"
	"\n"
	"uniform Colors\n"
	"{\n"
	"    vec4 red;\n"
	"    vec4 green;\n"
	"    vec4 blue;\n"
	"} uni_colors; // referenced in GS\n"
	"\n";

const char* const GeometryShaderProgramResourceTest::m_common_shader_code_definitions_atomic_counter_body =
	"// atomic counter buffers\n"
	"layout (binding = 0) uniform atomic_uint uni_atom_horizontal; // not referenced in GS\n"
	"layout (binding = 1) uniform atomic_uint uni_atom_vertical;   // referenced in GS\n"
	"\n";

const char* const GeometryShaderProgramResourceTest::m_common_shader_code_definitions_ssbo_body =
	"// ssbos\n"
	"buffer Positions\n"
	"{\n"
	"    vec4 position[4]; // referenced in GS\n"
	"} storage_positions;  // referenced in GS\n"
	"\n"
	"buffer Ids\n"
	"{\n"
	"    int ids[4]; // not referenced in GS\n"
	"} storage_ids;  // not referenced in GS\n"
	"\n";

/* Vertex shader */
const char* const GeometryShaderProgramResourceTest::m_vertex_shader_code_preamble = "${VERSION}\n"
																					 "\n"
																					 "precision highp float;\n"
																					 "\n"
																					 "// uniforms included here\n";

const char* const GeometryShaderProgramResourceTest::m_vertex_shader_code_body =
	"// attributes\n"
	"in vec4 vs_in_position; // not referenced in GS\n"
	"in vec4 vs_in_color;    // not referenced in GS\n"
	"\n"
	"// output\n"
	"out vec4 vs_out_color; // referenced in GS\n"
	"\n"
	"void main()\n"
	"{\n"
	"    gl_Position  = uni_model_view_projection * vs_in_position;\n"
	"    vs_out_color = vs_in_color;\n";

const char* const GeometryShaderProgramResourceTest::m_vertex_shader_code_atomic_counter_body =
	"    // write atomic counters\n"
	"    if (0.0 <= gl_Position.x)\n"
	"    {\n"
	"        atomicCounterIncrement(uni_atom_vertical);\n"
	"    }\n"
	"\n"
	"    if (0.0 <= gl_Position.y)\n"
	"    {\n"
	"        atomicCounterIncrement(uni_atom_horizontal);\n"
	"    }\n";

const char* const GeometryShaderProgramResourceTest::m_vertex_shader_code_ssbo_body =
	"    // write shader storage buffers\n"
	"    storage_positions.position[gl_VertexID] = gl_Position;\n"
	"    storage_ids.ids[gl_VertexID]            = gl_VertexID;\n";

/* Geometry shader */
const char* const GeometryShaderProgramResourceTest::m_geometry_shader_code_preamble =
	"${VERSION}\n"
	"\n"
	"${GEOMETRY_SHADER_REQUIRE}\n"
	"\n"
	"precision highp float;\n"
	"\n"
	"layout (points)                           in;\n"
	"layout (triangle_strip, max_vertices = 6) out;\n"
	"\n"
	"// uniforms included here\n";

const char* const GeometryShaderProgramResourceTest::m_geometry_shader_code_body =
	"// input from vs + gl_Position\n"
	"in vec4 vs_out_color[1];\n"
	"\n"
	"out vec4 gs_out_color;\n"
	"\n"
	"void main()\n"
	"{\n"
	"    // access uniform\n"
	"    gl_Position = vec4(0, 0, 0, 0);\n"
	"    gs_out_color = uni_colors_white;\n"
	"    EmitVertex();\n"
	"\n"
	"    // access uniform block\n"
	"    gl_Position = gl_in[0].gl_Position + vec4(-0.1, 0.1, 0, 0);\n"
	"    gs_out_color = vs_out_color[0] + uni_colors.red;\n"
	"    EmitVertex();\n"
	"\n"
	"    gl_Position = gl_in[0].gl_Position + vec4(-0.1, -0.1, 0, 0);\n"
	"    gs_out_color = vs_out_color[0] + uni_colors.green;\n"
	"    EmitVertex();\n"
	"\n"
	"    gl_Position = gl_in[0].gl_Position + vec4(0.1, -0.1, 0, 0);\n"
	"    gs_out_color = vs_out_color[0] + uni_colors.blue;\n"
	"    EmitVertex();\n";

const char* const GeometryShaderProgramResourceTest::m_geometry_shader_code_atomic_counter_body =
	"    // access atomic counter\n"
	"    gl_Position = vec4(0, 0, 0, 0);\n"
	"    uint  counter = atomicCounter(uni_atom_vertical);\n"
	"    gs_out_color     = vec4(counter / 255u);\n"
	"    EmitVertex();\n";

const char* const GeometryShaderProgramResourceTest::m_geometry_shader_code_ssbo_body =
	"    // access shader storage buffers\n"
	"    gl_Position = storage_positions.position[0] + vec4(0.1, 0.1, 0, 0);\n"
	"    gs_out_color     = vec4(1.0);\n"
	"    EmitVertex();\n";

/* Fragment shader */
const char* const GeometryShaderProgramResourceTest::m_fragment_shader_code =
	"${VERSION}\n"
	"\n"
	"precision highp float;\n"
	"\n"
	"// input from gs\n"
	"in vec4 gs_out_color;\n"
	"layout(location = 0) out vec4 fs_out_color;\n"
	"\n"
	"void main()\n"
	"{\n"
	"    fs_out_color = gs_out_color;\n"
	"}\n"
	"\n";

/** Constructor
 *
 * @param context     Test context
 * @param name        Test case's name
 * @param description Test case's desricption
 **/
GeometryShaderProgramResourceTest::GeometryShaderProgramResourceTest(Context& context, const ExtParameters& extParams,
																	 const char* name, const char* description)
	: TestCaseBase(context, extParams, name, description)
	, m_fragment_shader_id(0)
	, m_geometry_shader_id(0)
	, m_vertex_shader_id(0)
	, m_program_object_id(0)
	, m_atomic_counters_supported(false)
	, m_ssbos_supported(false)
{
	/* Nothing to be done here */
}

/** Initializes GLES objects used during the test.
 *
 **/
void GeometryShaderProgramResourceTest::initTest()
{
	/* Check if geometry_shader extension is supported */
	if (!m_is_geometry_shader_extension_supported)
	{
		throw tcu::NotSupportedError(GEOMETRY_SHADER_EXTENSION_NOT_SUPPORTED, "", __FILE__, __LINE__);
	}

	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	glw::GLint maxVSAtomicCounters = 0;
	gl.getIntegerv(GL_MAX_VERTEX_ATOMIC_COUNTER_BUFFERS, &maxVSAtomicCounters);

	glw::GLint maxGSAtomicCounters = 0;
	gl.getIntegerv(m_glExtTokens.MAX_GEOMETRY_ATOMIC_COUNTER_BUFFERS, &maxGSAtomicCounters);

	m_atomic_counters_supported = maxVSAtomicCounters >= 2 && maxGSAtomicCounters >= 1;

	glw::GLint maxVSStorageBlocks = 0;
	gl.getIntegerv(GL_MAX_VERTEX_SHADER_STORAGE_BLOCKS, &maxVSStorageBlocks);

	glw::GLint maxGSStorageBlocks = 0;
	gl.getIntegerv(m_glExtTokens.MAX_GEOMETRY_SHADER_STORAGE_BLOCKS, &maxGSStorageBlocks);

	m_ssbos_supported = maxVSStorageBlocks >= 2 && maxGSStorageBlocks >= 1;

	glw::GLuint nCodeParts = 0;
	const char* vertex_shader_code_parts[8];
	const char* geometry_shader_code_parts[8];

	/* Vertex & geometry shaders */
	if (m_atomic_counters_supported && m_ssbos_supported)
	{
		vertex_shader_code_parts[0] = m_vertex_shader_code_preamble;
		vertex_shader_code_parts[1] = m_common_shader_code_definitions_body;
		vertex_shader_code_parts[2] = m_common_shader_code_definitions_atomic_counter_body;
		vertex_shader_code_parts[3] = m_common_shader_code_definitions_ssbo_body;
		vertex_shader_code_parts[4] = m_vertex_shader_code_body;
		vertex_shader_code_parts[5] = m_vertex_shader_code_atomic_counter_body;
		vertex_shader_code_parts[6] = m_vertex_shader_code_ssbo_body;
		vertex_shader_code_parts[7] = "}\n";

		geometry_shader_code_parts[0] = m_geometry_shader_code_preamble;
		geometry_shader_code_parts[1] = m_common_shader_code_definitions_body;
		geometry_shader_code_parts[2] = m_common_shader_code_definitions_atomic_counter_body;
		geometry_shader_code_parts[3] = m_common_shader_code_definitions_ssbo_body;
		geometry_shader_code_parts[4] = m_geometry_shader_code_body;
		geometry_shader_code_parts[5] = m_geometry_shader_code_atomic_counter_body;
		geometry_shader_code_parts[6] = m_geometry_shader_code_ssbo_body;
		geometry_shader_code_parts[7] = "}\n";

		nCodeParts = 8;
	}
	else if (m_atomic_counters_supported)
	{
		vertex_shader_code_parts[0] = m_vertex_shader_code_preamble;
		vertex_shader_code_parts[1] = m_common_shader_code_definitions_body;
		vertex_shader_code_parts[2] = m_common_shader_code_definitions_atomic_counter_body;
		vertex_shader_code_parts[3] = m_vertex_shader_code_body;
		vertex_shader_code_parts[4] = m_vertex_shader_code_atomic_counter_body;
		vertex_shader_code_parts[5] = "}\n";

		geometry_shader_code_parts[0] = m_geometry_shader_code_preamble;
		geometry_shader_code_parts[1] = m_common_shader_code_definitions_body;
		geometry_shader_code_parts[2] = m_common_shader_code_definitions_atomic_counter_body;
		geometry_shader_code_parts[3] = m_geometry_shader_code_body;
		geometry_shader_code_parts[4] = m_geometry_shader_code_atomic_counter_body;
		geometry_shader_code_parts[5] = "}\n";

		nCodeParts = 6;
	}
	else if (m_ssbos_supported)
	{
		vertex_shader_code_parts[0] = m_vertex_shader_code_preamble;
		vertex_shader_code_parts[1] = m_common_shader_code_definitions_body;
		vertex_shader_code_parts[2] = m_common_shader_code_definitions_ssbo_body;
		vertex_shader_code_parts[3] = m_vertex_shader_code_body;
		vertex_shader_code_parts[4] = m_vertex_shader_code_ssbo_body;
		vertex_shader_code_parts[5] = "}\n";

		geometry_shader_code_parts[0] = m_geometry_shader_code_preamble;
		geometry_shader_code_parts[1] = m_common_shader_code_definitions_body;
		geometry_shader_code_parts[2] = m_common_shader_code_definitions_ssbo_body;
		geometry_shader_code_parts[3] = m_geometry_shader_code_body;
		geometry_shader_code_parts[4] = m_geometry_shader_code_ssbo_body;
		geometry_shader_code_parts[5] = "}\n";

		nCodeParts = 6;
	}
	else
	{
		vertex_shader_code_parts[0] = m_vertex_shader_code_preamble;
		vertex_shader_code_parts[1] = m_common_shader_code_definitions_body;
		vertex_shader_code_parts[2] = m_vertex_shader_code_body;
		vertex_shader_code_parts[3] = "}\n";

		geometry_shader_code_parts[0] = m_geometry_shader_code_preamble;
		geometry_shader_code_parts[1] = m_common_shader_code_definitions_body;
		geometry_shader_code_parts[2] = m_geometry_shader_code_body;
		geometry_shader_code_parts[3] = "}\n";

		nCodeParts = 4;
	}

	/* Create shader objects */
	m_fragment_shader_id = gl.createShader(GL_FRAGMENT_SHADER);
	m_geometry_shader_id = gl.createShader(m_glExtTokens.GEOMETRY_SHADER);
	m_vertex_shader_id   = gl.createShader(GL_VERTEX_SHADER);

	/* Create program object */
	m_program_object_id = gl.createProgram();

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not create program object");

	/* Build program object */
	if (true !=
		buildProgram(m_program_object_id, m_fragment_shader_id, 1,				 /* Fragment shader parts number */
					 &m_fragment_shader_code, m_geometry_shader_id, nCodeParts,  /* Geometry shader parts number */
					 geometry_shader_code_parts, m_vertex_shader_id, nCodeParts, /* Vertex shader parts number */
					 vertex_shader_code_parts))
	{
		TCU_FAIL("Could not create program from valid vertex/geometry/fragment shader");
	}
}

/** Executes the test.
 *  Sets the test result to QP_TEST_RESULT_FAIL if the test failed, QP_TEST_RESULT_PASS otherwise.
 *  @return STOP if the test has finished, CONTINUE to indicate iterate should be called once again.
 *  Note the function throws exception should an error occur!
 **/
tcu::TestCase::IterateResult GeometryShaderProgramResourceTest::iterate()
{
	initTest();

	/* Results */
	bool result = true;

	/* Results of checks, names come from shaders */
	bool is_gl_fs_out_color_program_output_referenced		  = false;
	bool is_ids_ids_buffer_variable_referenced				  = false;
	bool is_positions_position_buffer_variable_referenced	 = false;
	bool is_storage_ids_shader_storage_block_referenced		  = false;
	bool is_storage_positions_shader_storage_block_referenced = false;
	bool is_uni_atom_horizontal_uniform_referenced			  = false;
	bool is_uni_atom_vertical_uniform_referenced			  = false;
	bool is_uni_colors_uniform_referenced					  = false;
	bool is_uni_colors_white_uniform_referenced				  = false;
	bool is_uni_matrices_uniform_referenced					  = false;
	bool is_uni_model_view_projection_uniform_referenced	  = false;
	bool is_vs_in_color_program_input_referenced			  = false;
	bool is_vs_in_position_program_input_referenced			  = false;

	/* Check whether uniform variables are referenced */
	is_uni_model_view_projection_uniform_referenced =
		checkIfResourceIsReferenced(m_program_object_id, GL_UNIFORM, "uni_model_view_projection");
	is_uni_colors_white_uniform_referenced =
		checkIfResourceIsReferenced(m_program_object_id, GL_UNIFORM, "uni_colors_white");

	/* For: uniform Matrices {} uni_matrices; uniform block name is: Matrices */
	is_uni_matrices_uniform_referenced = checkIfResourceIsReferenced(m_program_object_id, GL_UNIFORM_BLOCK, "Matrices");
	is_uni_colors_uniform_referenced   = checkIfResourceIsReferenced(m_program_object_id, GL_UNIFORM_BLOCK, "Colors");

	/* For: buffer Positions {} storage_positions; storage block name is: Positions */
	if (m_ssbos_supported)
	{
		is_storage_positions_shader_storage_block_referenced =
			checkIfResourceIsReferenced(m_program_object_id, GL_SHADER_STORAGE_BLOCK, "Positions");
		is_storage_ids_shader_storage_block_referenced =
			checkIfResourceIsReferenced(m_program_object_id, GL_SHADER_STORAGE_BLOCK, "Ids");

		is_positions_position_buffer_variable_referenced =
			checkIfResourceIsReferenced(m_program_object_id, GL_BUFFER_VARIABLE, "Positions.position");
		is_ids_ids_buffer_variable_referenced =
			checkIfResourceIsReferenced(m_program_object_id, GL_BUFFER_VARIABLE, "Ids.ids");
	}

	/* Check whether input attributes are referenced */
	is_vs_in_position_program_input_referenced =
		checkIfResourceIsReferenced(m_program_object_id, GL_PROGRAM_INPUT, "vs_in_position");
	is_vs_in_color_program_input_referenced =
		checkIfResourceIsReferenced(m_program_object_id, GL_PROGRAM_INPUT, "vs_in_color");

	/* Check whether output attributes are referenced */
	is_gl_fs_out_color_program_output_referenced =
		checkIfResourceIsReferenced(m_program_object_id, GL_PROGRAM_OUTPUT, "fs_out_color");

	/*
	 *     For the ATOMIC_COUNTER_BUFFER interface, the list of active buffer binding
	 *     points is built by identifying each unique binding point associated with
	 *     one or more active atomic counter uniform variables.  Active atomic
	 *     counter buffers do not have an associated name string.
	 */
	if (m_atomic_counters_supported)
	{
		const glw::Functions& gl = m_context.getRenderContext().getFunctions();

		/* First get corresponding uniform indices */
		glw::GLuint atom_horizontal_uniform_indx =
			gl.getProgramResourceIndex(m_program_object_id, GL_UNIFORM, "uni_atom_horizontal");
		glw::GLuint atom_vertical_uniform_indx =
			gl.getProgramResourceIndex(m_program_object_id, GL_UNIFORM, "uni_atom_vertical");

		/* Then get atomic buffer indices */
		glw::GLuint atom_horizontal_uniform_buffer_indx = GL_INVALID_INDEX;
		glw::GLuint atom_vertical_uniform_buffer_indx   = GL_INVALID_INDEX;

		/* Data for getProgramResourceiv */
		glw::GLint  params[] = { 0 };
		glw::GLenum props[]  = { GL_ATOMIC_COUNTER_BUFFER_INDEX };

		/* Get property value */
		gl.getProgramResourceiv(m_program_object_id, GL_UNIFORM, atom_horizontal_uniform_indx, 1, /* propCount */
								props, 1,														  /* bufSize */
								0,																  /* length */
								params);
		atom_horizontal_uniform_buffer_indx = params[0];

		gl.getProgramResourceiv(m_program_object_id, GL_UNIFORM, atom_vertical_uniform_indx, 1, /* propCount */
								props, 1,														/* bufSize */
								0,																/* length */
								params);
		atom_vertical_uniform_buffer_indx = params[0];

		/* Check whether atomic counters are referenced using the atomic buffer indices */
		is_uni_atom_horizontal_uniform_referenced = checkIfResourceAtIndexIsReferenced(
			m_program_object_id, GL_ATOMIC_COUNTER_BUFFER, atom_horizontal_uniform_buffer_indx);
		is_uni_atom_vertical_uniform_referenced = checkIfResourceAtIndexIsReferenced(
			m_program_object_id, GL_ATOMIC_COUNTER_BUFFER, atom_vertical_uniform_buffer_indx);
	}

	/* Verify results: referenced properties */
	if (true != is_uni_colors_white_uniform_referenced)
	{
		m_testCtx.getLog() << tcu::TestLog::Message
						   << "Wrong value of property GL_REFERENCED_BY_GEOMETRY_SHADER_EXT for GL_UNIFORM"
						   << tcu::TestLog::EndMessage;

		result = false;
	}

	if (true != is_uni_colors_uniform_referenced)
	{
		m_testCtx.getLog() << tcu::TestLog::Message
						   << "Wrong value of property GL_REFERENCED_BY_GEOMETRY_SHADER_EXT for GL_UNIFORM_BLOCK"
						   << tcu::TestLog::EndMessage;

		result = false;
	}

	if (true != is_uni_atom_vertical_uniform_referenced && m_atomic_counters_supported)
	{
		m_testCtx.getLog()
			<< tcu::TestLog::Message
			<< "Wrong value of property GL_REFERENCED_BY_GEOMETRY_SHADER_EXT for GL_ATOMIC_COUNTER_BUFFER"
			<< tcu::TestLog::EndMessage;

		result = false;
	}

	if (false != is_uni_atom_horizontal_uniform_referenced && m_atomic_counters_supported)
	{
		m_testCtx.getLog()
			<< tcu::TestLog::Message
			<< "Wrong value of property GL_REFERENCED_BY_GEOMETRY_SHADER_EXT for GL_ATOMIC_COUNTER_BUFFER"
			<< tcu::TestLog::EndMessage;

		result = false;
	}

	if (true != is_storage_positions_shader_storage_block_referenced && m_ssbos_supported)
	{
		m_testCtx.getLog() << tcu::TestLog::Message
						   << "Wrong value of property GL_REFERENCED_BY_GEOMETRY_SHADER_EXT for GL_SHADER_STORAGE_BLOCK"
						   << tcu::TestLog::EndMessage;

		result = false;
	}

	if (false != is_storage_ids_shader_storage_block_referenced && m_ssbos_supported)
	{
		m_testCtx.getLog() << tcu::TestLog::Message
						   << "Wrong value of property GL_REFERENCED_BY_GEOMETRY_SHADER_EXT for GL_SHADER_STORAGE_BLOCK"
						   << tcu::TestLog::EndMessage;

		result = false;
	}

	if (true != is_positions_position_buffer_variable_referenced && m_ssbos_supported)
	{
		m_testCtx.getLog() << tcu::TestLog::Message
						   << "Wrong value of property GL_REFERENCED_BY_GEOMETRY_SHADER_EXT for GL_BUFFER_VARIABLE"
						   << tcu::TestLog::EndMessage;

		result = false;
	}

	if (false != is_ids_ids_buffer_variable_referenced && m_ssbos_supported)
	{
		m_testCtx.getLog() << tcu::TestLog::Message
						   << "Wrong value of property GL_REFERENCED_BY_GEOMETRY_SHADER_EXT for GL_BUFFER_VARIABLE"
						   << tcu::TestLog::EndMessage;

		result = false;
	}

	/* Verify results: properties that are not referenced */
	if (false != is_uni_model_view_projection_uniform_referenced)
	{
		m_testCtx.getLog() << tcu::TestLog::Message
						   << "Wrong value of property GL_REFERENCED_BY_GEOMETRY_SHADER_EXT for GL_UNIFORM"
						   << tcu::TestLog::EndMessage;

		result = false;
	}

	if (false != is_uni_matrices_uniform_referenced)
	{
		m_testCtx.getLog() << tcu::TestLog::Message
						   << "Wrong value of property GL_REFERENCED_BY_GEOMETRY_SHADER_EXT for GL_UNIFORM_BLOCK"
						   << tcu::TestLog::EndMessage;

		result = false;
	}

	if (false != is_vs_in_position_program_input_referenced)
	{
		m_testCtx.getLog() << tcu::TestLog::Message
						   << "Wrong value of property GL_REFERENCED_BY_GEOMETRY_SHADER_EXT for GL_PROGRAM_INPUT"
						   << tcu::TestLog::EndMessage;

		result = false;
	}

	if (false != is_vs_in_color_program_input_referenced)
	{
		m_testCtx.getLog() << tcu::TestLog::Message
						   << "Wrong value of property GL_REFERENCED_BY_GEOMETRY_SHADER_EXT for GL_PROGRAM_INPUT"
						   << tcu::TestLog::EndMessage;

		result = false;
	}

	if (false != is_gl_fs_out_color_program_output_referenced)
	{
		m_testCtx.getLog() << tcu::TestLog::Message
						   << "Wrong value of property GL_REFERENCED_BY_GEOMETRY_SHADER_EXT for GL_PROGRAM_OUTPUT"
						   << tcu::TestLog::EndMessage;

		result = false;
	}

	/* Set test result */
	if (true == result)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
	}

	/* Done */
	return STOP;
}

/** Deinitializes GLES objects created during the test.
 *
 */
void GeometryShaderProgramResourceTest::deinit()
{
	/* GL */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Reset OpenGL ES state */
	gl.useProgram(0);

	/* Delete program objects and shader objects */
	if (m_program_object_id != 0)
	{
		gl.deleteProgram(m_program_object_id);
	}

	if (m_vertex_shader_id != 0)
	{
		gl.deleteShader(m_vertex_shader_id);
	}

	if (m_geometry_shader_id != 0)
	{
		gl.deleteShader(m_geometry_shader_id);
	}

	if (m_fragment_shader_id != 0)
	{
		gl.deleteShader(m_fragment_shader_id);
	}

	/* Release base class */
	TestCaseBase::deinit();
}

/** Queries property GL_REFERENCED_BY_GEOMETRY_SHADER_EXT for resource at specified index in specific interface.
 *  Program object must be linked.
 *
 *  @param program_object_id Program which will be inspected;
 *  @param interface         Queried interface;
 *  @param index             Resource's index.
 *
 *  @return  true            Property value is 1
 *           false           Property value is 0
 **/
bool GeometryShaderProgramResourceTest::checkIfResourceAtIndexIsReferenced(glw::GLuint program_object_id,
																		   glw::GLenum interface,
																		   glw::GLuint index) const
{
	(void)program_object_id;

	/* GL */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Data for getProgramResourceiv */
	glw::GLint  params[] = { 0 };
	glw::GLenum props[]  = { m_glExtTokens.REFERENCED_BY_GEOMETRY_SHADER };

	/* Get property value */
	gl.getProgramResourceiv(m_program_object_id, interface, index, 1, /* propCount */
							props, 1,								  /* bufSize */
							0,										  /* length */
							params);

	/**
	 *     The value one is written to <params> if an active
	 *     variable is referenced by the corresponding shader, or if an active
	 *     uniform block, shader storage block, or atomic counter buffer contains
	 *     at least one variable referenced by the corresponding shader.  Otherwise,
	 *     the value zero is written to <params>.
	 **/
	if (1 == params[0])
	{
		return true;
	}
	else
	{
		return false;
	}
}

/** Queries property GL_REFERENCED_BY_GEOMETRY_SHADER_EXT for resource with specified name in specific interface.
 *  Program object must be linked.
 *
 *  @param program_object_id Program which will be inspected
 *  @param interface         Queried interface
 *  @param name              Resource's name
 *
 *  @return  true            Property value is 1
 *           false           Property value is 0, or resource is not available
 **/
bool GeometryShaderProgramResourceTest::checkIfResourceIsReferenced(glw::GLuint program_object_id,
																	glw::GLenum interface, const char* name) const
{
	/* GL */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Resource index */
	glw::GLuint index = GL_INVALID_INDEX;

	/* Get resource's index by name */
	index = gl.getProgramResourceIndex(program_object_id, interface, name);

	/**
	 *     Otherwise, <name> is considered not to be the name
	 *     of an active resource, and INVALID_INDEX is returned.
	 **/
	if (GL_INVALID_INDEX == index)
	{
		return false;
	}

	/* Get property by index */
	return checkIfResourceAtIndexIsReferenced(program_object_id, interface, index);
}

} /* glcts */
