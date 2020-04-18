/*-------------------------------------------------------------------------
 * OpenGL Conformance Test Suite
 * -----------------------------
 *
 * Copyright (c) 2015-2016 The Khronos Group Inc.
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
#include "esextcGeometryShaderLinking.hpp"

#include "gluDefs.hpp"
#include "glwEnums.hpp"
#include "glwFunctions.hpp"
#include "tcuTestLog.hpp"
#include <cstring>

namespace glcts
{

static const char* dummy_fs_code = "${VERSION}\n"
								   "\n"
								   "precision highp float;\n"
								   "\n"
								   "out vec4 result;\n"
								   "\n"
								   "void main()\n"
								   "{\n"
								   "    result = vec4(1.0);\n"
								   "}\n";

static const char* dummy_gs_code = "${VERSION}\n"
								   "${GEOMETRY_SHADER_REQUIRE}\n"
								   "\n"
								   "layout (points)                   in;\n"
								   "layout (points, max_vertices = 1) out;\n"
								   "\n"
								   "${OUT_PER_VERTEX_DECL}"
								   "${IN_DATA_DECL}"
								   "\n"
								   "void main()\n"
								   "{\n"
								   "${POSITION_WITH_IN_DATA}"
								   "    EmitVertex();\n"
								   "}\n";

static const char* dummy_vs_code = "${VERSION}\n"
								   "\n"
								   "${OUT_PER_VERTEX_DECL}"
								   "\n"
								   "void main()\n"
								   "{\n"
								   "    gl_Position = vec4(1.0, 0.0, 0.0, 1.0);\n"
								   "}\n";

/** Constructor
 *
 * @param context       Test context
 * @param extParams     Not used.
 * @param name          Test case's name
 * @param description   Test case's description
 **/
GeometryShaderIncompleteProgramObjectsTest::GeometryShaderIncompleteProgramObjectsTest(Context&				context,
																					   const ExtParameters& extParams,
																					   const char*			name,
																					   const char*			description)
	: TestCaseBase(context, extParams, name, description), m_fs_id(0), m_gs_id(0), m_po_id(0)
{
}

/** Deinitializes GLES objects created during the test. */
void GeometryShaderIncompleteProgramObjectsTest::deinit()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (m_fs_id != 0)
	{
		gl.deleteShader(m_fs_id);

		m_fs_id = 0;
	}

	if (m_gs_id != 0)
	{
		gl.deleteShader(m_gs_id);

		m_gs_id = 0;
	}

	if (m_po_id != 0)
	{
		gl.deleteProgram(m_po_id);

		m_po_id = 0;
	}

	/* Release base class */
	TestCaseBase::deinit();
}

/** Initializes shader objects for the conformance test */
void GeometryShaderIncompleteProgramObjectsTest::initShaderObjects()
{
	const glw::Functions& gl					  = m_context.getRenderContext().getFunctions();
	std::string			  specialized_fs_code	 = specializeShader(1, &dummy_fs_code);
	const char*			  specialized_fs_code_raw = specialized_fs_code.c_str();
	std::string			  specialized_gs_code	 = specializeShader(1, &dummy_gs_code);
	const char*			  specialized_gs_code_raw = specialized_gs_code.c_str();

	m_fs_id = gl.createShader(GL_FRAGMENT_SHADER);
	m_gs_id = gl.createShader(GL_GEOMETRY_SHADER);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateShader() call failed.");

	for (unsigned int n_shader_type = 0; n_shader_type < 2; /* fs, gs */
		 n_shader_type++)
	{
		glw::GLint  compile_status = GL_FALSE;
		const char* so_code		   = (n_shader_type == 0) ? specialized_fs_code_raw : specialized_gs_code_raw;
		glw::GLuint so_id		   = (n_shader_type == 0) ? m_fs_id : m_gs_id;

		gl.shaderSource(so_id, 1,			/* count */
						&so_code, DE_NULL); /* length */

		GLU_EXPECT_NO_ERROR(gl.getError(), "glShaderSource() call failed");

		gl.compileShader(so_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glCompileShader() call failed");

		gl.getShaderiv(so_id, GL_COMPILE_STATUS, &compile_status);

		GLU_EXPECT_NO_ERROR(gl.getError(), "glGetShaderiv() call failed");

		if (compile_status != GL_TRUE)
		{
			TCU_FAIL("Shader compilation process failed.");
		}
	} /* for (both shader stages) */
}

/** Initializes test runs, to be executed by the conformance test. */
void GeometryShaderIncompleteProgramObjectsTest::initTestRuns()
{
	/*                         use_fs| use_gs| use_separable_po
	 *                         ------|-------|-----------------*/
	m_test_runs.push_back(_run(false, true, false));
	m_test_runs.push_back(_run(false, true, true));
	m_test_runs.push_back(_run(true, true, false));
	m_test_runs.push_back(_run(true, true, true));
}

/** Executes the test.
 *
 *  Sets the test result to QP_TEST_RESULT_FAIL if the test failed, QP_TEST_RESULT_PASS otherwise.
 *  @return STOP if the test has finished, CONTINUE to indicate iterate should be called once again.
 *  Note the function throws exception should an error occur!
 **/
tcu::TestNode::IterateResult GeometryShaderIncompleteProgramObjectsTest::iterate()
{
	bool result = true;

	/* This test should only run if EXT_geometry_shader is supported. */
	if (!m_is_geometry_shader_extension_supported)
	{
		throw tcu::NotSupportedError(GEOMETRY_SHADER_EXTENSION_NOT_SUPPORTED, "", __FILE__, __LINE__);
	}

	/* Initialize test runs */
	initTestRuns();

	/* Set up shader objects */
	initShaderObjects();

	/* Iterate over the test run set */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	for (unsigned int run_index = 0; run_index < m_test_runs.size(); ++run_index)
	{
		const _run& current_run = m_test_runs[run_index];

		/* Set up a program object */
		m_po_id = gl.createProgram();
		GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateProgram() call failed");

		if (current_run.use_separable_po)
		{
			gl.programParameteri(m_po_id, GL_PROGRAM_SEPARABLE, GL_TRUE);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glProgramParameteri() call failed");
		} /* if (current_run.use_separable_po) */

		if (current_run.use_fs)
		{
			gl.attachShader(m_po_id, m_fs_id);
		}

		if (current_run.use_gs)
		{
			gl.attachShader(m_po_id, m_gs_id);
		}

		GLU_EXPECT_NO_ERROR(gl.getError(), "glAttachShader() call(s) failed");

		/* Try to link the PO */
		gl.linkProgram(m_po_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glLinkProgram() call failed.");

		/* Verify the link status */
		glw::GLint link_status = GL_FALSE;

		gl.getProgramiv(m_po_id, GL_LINK_STATUS, &link_status);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGetProgramiv() call failed");

		if ((current_run.use_separable_po && link_status != GL_TRUE) ||
			(!current_run.use_separable_po && link_status == GL_TRUE))
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "Invalid link status reported for a "
							   << ((current_run.use_separable_po) ? "separable" : "")
							   << " program object, to which the following SOs were attached: "
							   << "FS:" << ((current_run.use_fs) ? "YES" : "NO")
							   << ", GS:" << ((current_run.use_gs) ? "YES" : "NO") << tcu::TestLog::EndMessage;

			result = false;
		}

		/* Clean up for the next iteration */
		gl.deleteProgram(m_po_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glDeleteProgram() call failed");
	} /* for (all test runs) */

	if (result)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
	}

	return STOP;
}

/** Constructor
 *
 * @param context       Test context
 * @param extParams     Not used.
 * @param name          Test case's name
 * @param description   Test case's description
 **/
GeometryShaderIncompleteGSTest::GeometryShaderIncompleteGSTest(Context& context, const ExtParameters& extParams,
															   const char* name, const char* description)
	: TestCaseBase(context, extParams, name, description), m_fs_id(0), m_gs_id(0), m_po_id(0), m_vs_id(0)
{
}

/** Deinitializes GLES objects created during the test. */
void GeometryShaderIncompleteGSTest::deinit()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	deinitSOs();

	if (m_po_id != 0)
	{
		gl.deleteProgram(m_po_id);

		m_po_id = 0;
	}

	/* Release base class */
	TestCaseBase::deinit();
}

/** Deinitializes shader objects created for the conformance test. */
void GeometryShaderIncompleteGSTest::deinitSOs()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (m_fs_id != 0)
	{
		gl.deleteShader(m_fs_id);

		m_fs_id = 0;
	}

	if (m_gs_id != 0)
	{
		gl.deleteShader(m_gs_id);

		m_gs_id = 0;
	}

	if (m_vs_id != 0)
	{
		gl.deleteShader(m_vs_id);

		m_vs_id = 0;
	}
}

/** Returns geometry shader's source code, built according to the test run's settings.
 *
 *  @param current_run Test run descriptor.
 *
 *  @return Requested string.
 */
std::string GeometryShaderIncompleteGSTest::getGeometryShaderCode(const _run& current_run)
{
	std::stringstream gs_code_sstream;

	gs_code_sstream << "${VERSION}\n"
					   "${GEOMETRY_SHADER_REQUIRE}\n"
					   "\n";

	if (current_run.is_input_primitive_type_defined)
	{
		gs_code_sstream << "layout(points) in;\n";
	}

	if (current_run.is_max_vertices_defined || current_run.is_output_primitive_type_defined)
	{
		gs_code_sstream << "layout(";

		if (current_run.is_max_vertices_defined)
		{
			gs_code_sstream << "max_vertices = 1";

			if (current_run.is_output_primitive_type_defined)
			{
				gs_code_sstream << ", ";
			}
		} /* if (current_run.is_max_vertices_defined) */

		if (current_run.is_output_primitive_type_defined)
		{
			gs_code_sstream << "points";
		}

		gs_code_sstream << ") out;\n";
	}

	gs_code_sstream << "\n"
					   "void main()\n"
					   "{\n"
					   "    gl_Position = gl_in[0].gl_Position;\n"
					   "    EmitVertex();\n"
					   "}\n";

	return gs_code_sstream.str();
}

/** Initializes fragment / geometry / vertex shader objects, according to the test run descriptor.
 *
 *  @param current_run                      Test run descriptor.
 *  @param out_has_fs_compiled_successfully Deref will be set to false, if FS has failed to compile
 *                                          successfully. Otherwise, it will be set to true.
 *  @param out_has_gs_compiled_successfully Deref will be set to false, if GS has failed to compile
 *                                          successfully. Otherwise, it will be set to true.
 *  @param out_has_vs_compiled_successfully Deref will be set to false, if VS has failed to compile
 *                                          successfully. Otherwise, it will be set to true.
 *
 */
void GeometryShaderIncompleteGSTest::initShaderObjects(const _run& current_run, bool* out_has_fs_compiled_successfully,
													   bool* out_has_gs_compiled_successfully,
													   bool* out_has_vs_compiled_successfully)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	std::string specialized_fs_code = specializeShader(1, &dummy_fs_code);
	std::string gs_code				= getGeometryShaderCode(current_run);
	const char* gs_code_raw			= gs_code.c_str();
	std::string specialized_gs_code = specializeShader(1, &gs_code_raw);
	std::string specialized_vs_code = specializeShader(1, &dummy_vs_code);

	const char* specialized_fs_code_raw = specialized_fs_code.c_str();
	const char* specialized_gs_code_raw = specialized_gs_code.c_str();
	const char* specialized_vs_code_raw = specialized_vs_code.c_str();

	m_fs_id = gl.createShader(GL_FRAGMENT_SHADER);
	m_gs_id = gl.createShader(GL_GEOMETRY_SHADER);
	m_vs_id = gl.createShader(GL_VERTEX_SHADER);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateShader() call failed.");

	for (unsigned int n_shader_type = 0; n_shader_type < 3; /* fs, gs, vs */
		 n_shader_type++)
	{
		glw::GLint compile_status = GL_FALSE;
		bool*	  out_current_compile_result =
			(n_shader_type == 0) ?
				out_has_fs_compiled_successfully :
				(n_shader_type == 1) ? out_has_gs_compiled_successfully : out_has_vs_compiled_successfully;

		const char* so_code = (n_shader_type == 0) ?
								  specialized_fs_code_raw :
								  (n_shader_type == 1) ? specialized_gs_code_raw : specialized_vs_code_raw;

		glw::GLuint so_id = (n_shader_type == 0) ? m_fs_id : (n_shader_type == 1) ? m_gs_id : m_vs_id;

		gl.shaderSource(so_id, 1,			/* count */
						&so_code, DE_NULL); /* length */

		GLU_EXPECT_NO_ERROR(gl.getError(), "glShaderSource() call failed");

		gl.compileShader(so_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glCompileShader() call failed");

		gl.getShaderiv(so_id, GL_COMPILE_STATUS, &compile_status);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGetShaderiv() call failed");

		*out_current_compile_result = (compile_status == GL_TRUE);
	} /* for (both shader stages) */
}

/** Initializes all test runs */
void GeometryShaderIncompleteGSTest::initTestRuns()
{
	/*                         input_primitive_defined | max_vertices_defined | output_primitive_defined
	 *                         ------------------------|----------------------|-------------------------*/
	m_test_runs.push_back(_run(false, false, false));
	m_test_runs.push_back(_run(false, false, true));
	m_test_runs.push_back(_run(false, true, false));
	m_test_runs.push_back(_run(true, true, false));
	m_test_runs.push_back(_run(false, true, true));
}

/** Executes the test.
 *
 *  Sets the test result to QP_TEST_RESULT_FAIL if the test failed, QP_TEST_RESULT_PASS otherwise.
 *  @return STOP if the test has finished, CONTINUE to indicate iterate should be called once again.
 *  Note the function throws exception should an error occur!
 **/
tcu::TestNode::IterateResult GeometryShaderIncompleteGSTest::iterate()
{
	bool result = true;

	/* This test should only run if EXT_geometry_shader is supported. */
	if (!m_is_geometry_shader_extension_supported)
	{
		throw tcu::NotSupportedError(GEOMETRY_SHADER_EXTENSION_NOT_SUPPORTED, "", __FILE__, __LINE__);
	}

	/* Initialize test runs */
	initTestRuns();

	/* Iterate over the test run set */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	for (unsigned int run_index = 0; run_index < m_test_runs.size(); ++run_index)
	{
		const _run& current_run = m_test_runs[run_index];

		/* Release shader objects initialized in previous iterations */
		deinitSOs();

		/* Set up shader objects */
		bool has_fs_compiled = false;
		bool has_gs_compiled = false;
		bool has_vs_compiled = false;

		initShaderObjects(current_run, &has_fs_compiled, &has_gs_compiled, &has_vs_compiled);

		if (!has_fs_compiled || !has_vs_compiled)
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "Dummy FS and/or dummy VS failed to compile"
							   << tcu::TestLog::EndMessage;

			result = false;
			break;
		}

		/* Set up a program object */
		m_po_id = gl.createProgram();
		GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateProgram() call failed");

		gl.attachShader(m_po_id, m_fs_id);
		gl.attachShader(m_po_id, m_gs_id);
		gl.attachShader(m_po_id, m_vs_id);

		GLU_EXPECT_NO_ERROR(gl.getError(), "glAttachShader() call(s) failed");

		/* Try to link the PO */
		gl.linkProgram(m_po_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glLinkProgram() call failed.");

		/* Verify the link status */
		glw::GLint link_status = GL_FALSE;

		gl.getProgramiv(m_po_id, GL_LINK_STATUS, &link_status);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGetProgramiv() call failed");

		if (link_status == GL_TRUE)
		{
			m_testCtx.getLog() << tcu::TestLog::Message
							   << "PO with a malformed Geometry Shader was linked successfully."
							   << " [input primitive type]:"
							   << ((current_run.is_input_primitive_type_defined) ? "DEFINED" : "NOT DEFINED")
							   << " [output primitive type]:"
							   << ((current_run.is_output_primitive_type_defined) ? "DEFINED" : "NOT DEFINED")
							   << " [max_vertices]:"
							   << ((current_run.is_max_vertices_defined) ? "DEFINED" : "NOT DEFINED")
							   << tcu::TestLog::EndMessage;

			result = false;
		}

		/* Clean up for the next iteration */
		gl.deleteProgram(m_po_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glDeleteProgram() call failed");
	} /* for (all test runs) */

	if (result)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
	}

	return STOP;
}

/** Constructor
 *
 * @param context       Test context
 * @param extParams     Not used.
 * @param name          Test case's name
 * @param description   Test case's description
 **/
GeometryShaderInvalidArrayedInputVariablesTest::GeometryShaderInvalidArrayedInputVariablesTest(
	Context& context, const ExtParameters& extParams, const char* name, const char* description)
	: TestCaseBase(context, extParams, name, description), m_fs_id(0), m_gs_id(0), m_po_id(0), m_vs_id(0)
{
}

/** Deinitializes GLES objects created during the test. */
void GeometryShaderInvalidArrayedInputVariablesTest::deinit()
{
	deinitSOs();

	/* Release the PO */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (m_po_id != 0)
	{
		gl.deleteProgram(m_po_id);

		m_po_id = 0;
	}

	/* Release base class */
	TestCaseBase::deinit();
}

/** Deinitializes shader objects created for the conformance test. */
void GeometryShaderInvalidArrayedInputVariablesTest::deinitSOs()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (m_fs_id != 0)
	{
		gl.deleteShader(m_fs_id);

		m_fs_id = 0;
	}

	if (m_gs_id != 0)
	{
		gl.deleteShader(m_gs_id);

		m_gs_id = 0;
	}

	if (m_vs_id != 0)
	{
		gl.deleteShader(m_vs_id);

		m_vs_id = 0;
	}
}

/** Returns test-specific geometry shader source code, built for caller-specified input primitive type.
 *
 *  @param gs_input_primitive_type Input primitive type to be used for the process.
 *
 *  @return Requested shader source code.
 **/
std::string GeometryShaderInvalidArrayedInputVariablesTest::getGSCode(glw::GLenum gs_input_primitive_type) const
{
	std::stringstream  code_sstream;
	const unsigned int valid_array_size = getValidInputVariableArraySize(gs_input_primitive_type);

	code_sstream << "${VERSION}\n"
					"${GEOMETRY_SHADER_REQUIRE}\n"
					"\n"
					"layout ("
				 << getInputPrimitiveTypeQualifier(gs_input_primitive_type)
				 << ") in;\n"
					"layout (points, max_vertices = 1) out;\n"
					"\n"
					"in vec4 data["
				 << (valid_array_size + 1) << "];\n"
											  "\n"
											  "void main()\n"
											  "{\n"
											  "    gl_Position = data["
				 << valid_array_size << "];\n"
										"    EmitVertex();\n"
										"}\n";

	return code_sstream.str();
}

/** Returns a string holding the ES SL layout qualifier corresponding to user-specified input primitive type
 *  expressed as a GLenum value.
 *
 *  @param gs_input_primitive_type Geometry Shader's input primitive type, expressed as a GLenum value.
 *
 *  @return Requested string
 */
std::string GeometryShaderInvalidArrayedInputVariablesTest::getInputPrimitiveTypeQualifier(
	glw::GLenum gs_input_primitive_type) const
{
	std::string result;

	switch (gs_input_primitive_type)
	{
	case GL_POINTS:
		result = "points";
		break;
	case GL_LINES:
		result = "lines";
		break;
	case GL_LINES_ADJACENCY:
		result = "lines_adjacency";
		break;
	case GL_TRIANGLES:
		result = "triangles";
		break;
	case GL_TRIANGLES_ADJACENCY:
		result = "triangles_adjacency";
		break;

	default:
	{
		DE_ASSERT(0);
	}
	} /* switch (gs_input_primitive_type) */

	return result;
}

/** Retrieves a specialized version of the vertex shader to be used for the conformance test. */
std::string GeometryShaderInvalidArrayedInputVariablesTest::getSpecializedVSCode() const
{
	std::string vs_code = "${VERSION}\n"
						  "\n"
						  "out vec4 data;\n"
						  "\n"
						  "void main()\n"
						  "{\n"
						  "    data = vec4(gl_VertexID, 0, 0, 1);\n"
						  "}\n";
	const char* vs_code_raw			= vs_code.c_str();
	std::string specialized_vs_code = specializeShader(1, /* parts */
													   &vs_code_raw);

	return specialized_vs_code;
}

/** Returns array size that should be used for input variable declaration in GS, specific to
 *  to the caller-specified input primitive type.
 *
 *  @param gs_input_primitive_type Input primitive type to use for the query.
 *
 *  @return Requested value.
 */
glw::GLuint GeometryShaderInvalidArrayedInputVariablesTest::getValidInputVariableArraySize(
	glw::GLenum gs_input_primitive_type) const
{
	glw::GLuint result = 0;

	switch (gs_input_primitive_type)
	{
	case GL_POINTS:
		result = 1;
		break;
	case GL_LINES:
		result = 2;
		break;
	case GL_LINES_ADJACENCY:
		result = 4;
		break;
	case GL_TRIANGLES:
		result = 3;
		break;
	case GL_TRIANGLES_ADJACENCY:
		result = 6;
		break;

	default:
	{
		DE_ASSERT(0);
	}
	} /* switch (gs_input_primitive_type) */

	return result;
}

/** Initializes fragment / geometry / vertex shader objects, according to the user-specified GS input primitive type.
 *
 *  @param gs_input_primitive_type          Input primitive type, to be used for GS.
 *  @param out_has_fs_compiled_successfully Deref will be set to false, if FS has failed to compile
 *                                          successfully. Otherwise, it will be set to true.
 *  @param out_has_gs_compiled_successfully Deref will be set to false, if GS has failed to compile
 *                                          successfully. Otherwise, it will be set to true.
 *  @param out_has_vs_compiled_successfully Deref will be set to false, if VS has failed to compile
 *                                          successfully. Otherwise, it will be set to true.
 *
 */
void GeometryShaderInvalidArrayedInputVariablesTest::initShaderObjects(glw::GLenum gs_input_primitive_type,
																	   bool*	   out_has_fs_compiled_successfully,
																	   bool*	   out_has_gs_compiled_successfully,
																	   bool*	   out_has_vs_compiled_successfully)
{
	const glw::Functions& gl					  = m_context.getRenderContext().getFunctions();
	std::string			  specialized_fs_code	 = specializeShader(1, &dummy_fs_code);
	const char*			  specialized_fs_code_raw = specialized_fs_code.c_str();
	std::string			  gs_code				  = getGSCode(gs_input_primitive_type);
	const char*			  gs_code_raw			  = gs_code.c_str();
	std::string			  specialized_gs_code	 = specializeShader(1, &gs_code_raw);
	const char*			  specialized_gs_code_raw = specialized_gs_code.c_str();
	std::string			  specialized_vs_code	 = getSpecializedVSCode();
	const char*			  specialized_vs_code_raw = specialized_vs_code.c_str();

	m_fs_id = gl.createShader(GL_FRAGMENT_SHADER);
	m_gs_id = gl.createShader(GL_GEOMETRY_SHADER);
	m_vs_id = gl.createShader(GL_VERTEX_SHADER);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateShader() call failed.");

	for (unsigned int n_shader_type = 0; n_shader_type < 3; /* fs, gs, vs */
		 n_shader_type++)
	{
		glw::GLint compile_status	 = GL_FALSE;
		bool*	  out_compile_result = (n_shader_type == 0) ? out_has_fs_compiled_successfully :
														  (n_shader_type == 1) ? out_has_gs_compiled_successfully :
																				 out_has_vs_compiled_successfully;

		const char* so_code = (n_shader_type == 0) ?
								  specialized_fs_code_raw :
								  (n_shader_type == 1) ? specialized_gs_code_raw : specialized_vs_code_raw;

		glw::GLuint so_id = (n_shader_type == 0) ? m_fs_id : (n_shader_type == 1) ? m_gs_id : m_vs_id;

		gl.shaderSource(so_id, 1,			/* count */
						&so_code, DE_NULL); /* length */

		GLU_EXPECT_NO_ERROR(gl.getError(), "glShaderSource() call failed");

		gl.compileShader(so_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glCompileShader() call failed");

		gl.getShaderiv(so_id, GL_COMPILE_STATUS, &compile_status);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGetShaderiv() call failed");

		*out_compile_result = (compile_status == GL_TRUE);
	} /* for (both shader stages) */
}

/** Executes the test.
 *
 *  Sets the test result to QP_TEST_RESULT_FAIL if the test failed, QP_TEST_RESULT_PASS otherwise.
 *  @return STOP if the test has finished, CONTINUE to indicate iterate should be called once again.
 *  Note the function throws exception should an error occur!
 **/
tcu::TestNode::IterateResult GeometryShaderInvalidArrayedInputVariablesTest::iterate()
{
	const glw::Functions& gl	 = m_context.getRenderContext().getFunctions();
	bool				  result = true;

	/* This test should only run if EXT_geometry_shader is supported. */
	if (!m_is_geometry_shader_extension_supported)
	{
		throw tcu::NotSupportedError(GEOMETRY_SHADER_EXTENSION_NOT_SUPPORTED, "", __FILE__, __LINE__);
	}

	/* Iterate over all valid input primitive types */
	const glw::GLenum input_primitive_types[] = { GL_POINTS, GL_LINES, GL_LINES_ADJACENCY, GL_TRIANGLES,
												  GL_TRIANGLES_ADJACENCY };
	const unsigned int n_input_primitive_types = sizeof(input_primitive_types) / sizeof(input_primitive_types[0]);

	for (unsigned int n_input_primitive_type = 0; n_input_primitive_type < n_input_primitive_types;
		 ++n_input_primitive_type)
	{
		const glw::GLenum input_primitive_type = input_primitive_types[n_input_primitive_type];

		/* Release shader objects initialized in previous iterations */
		deinitSOs();

		/* Set up shader objects */
		bool has_fs_compiled = false;
		bool has_gs_compiled = false;
		bool has_vs_compiled = false;

		initShaderObjects(input_primitive_type, &has_fs_compiled, &has_gs_compiled, &has_vs_compiled);

		if (!has_fs_compiled || !has_gs_compiled || !has_vs_compiled)
		{
			m_testCtx.getLog()
				<< tcu::TestLog::Message
				<< "One of the shaders failed to compile (but shouldn't have). Shaders that failed to compile:"
				<< ((!has_fs_compiled) ? "FS " : "") << ((!has_gs_compiled) ? "GS " : "")
				<< ((!has_vs_compiled) ? "VS" : "") << ". Input primitive type: ["
				<< getInputPrimitiveTypeQualifier(input_primitive_type) << "]" << tcu::TestLog::EndMessage;

			continue;
		}

		/* Set up a program object */
		m_po_id = gl.createProgram();
		GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateProgram() call failed");

		gl.attachShader(m_po_id, m_fs_id);
		gl.attachShader(m_po_id, m_gs_id);
		gl.attachShader(m_po_id, m_vs_id);

		GLU_EXPECT_NO_ERROR(gl.getError(), "glAttachShader() call(s) failed");

		/* Try to link the PO */
		gl.linkProgram(m_po_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glLinkProgram() call failed.");

		/* Verify the link status */
		glw::GLint link_status = GL_FALSE;

		gl.getProgramiv(m_po_id, GL_LINK_STATUS, &link_status);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGetProgramiv() call failed");

		if (link_status == GL_TRUE)
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "A PO using a malformed GS has linked successfully. "
							   << "Test input primitive type: " << getInputPrimitiveTypeQualifier(input_primitive_type)
							   << tcu::TestLog::EndMessage;

			result = false;
		}
	} /* for (all input primitive types) */

	if (result)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
	}

	return STOP;
}

/** Constructor
 *
 * @param context       Test context
 * @param extParams     Not used.
 * @param name          Test case's name
 * @param description   Test case's description
 **/
GeometryShaderVSGSVariableTypeMismatchTest::GeometryShaderVSGSVariableTypeMismatchTest(Context&				context,
																					   const ExtParameters& extParams,
																					   const char*			name,
																					   const char*			description)
	: TestCaseBase(context, extParams, name, description), m_fs_id(0), m_gs_id(0), m_po_id(0), m_vs_id(0)
{
}

/** Deinitializes GLES objects created during the test. */
void GeometryShaderVSGSVariableTypeMismatchTest::deinit()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (m_fs_id != 0)
	{
		gl.deleteShader(m_fs_id);

		m_fs_id = 0;
	}

	if (m_gs_id != 0)
	{
		gl.deleteShader(m_gs_id);

		m_gs_id = 0;
	}

	if (m_po_id != 0)
	{
		gl.deleteProgram(m_po_id);

		m_po_id = 0;
	}

	if (m_vs_id != 0)
	{
		gl.deleteShader(m_vs_id);

		m_vs_id = 0;
	}

	/* Release base class */
	TestCaseBase::deinit();
}

/** Executes the test.
 *
 *  Sets the test result to QP_TEST_RESULT_FAIL if the test failed, QP_TEST_RESULT_PASS otherwise.
 *  @return STOP if the test has finished, CONTINUE to indicate iterate should be called once again.
 *  Note the function throws exception should an error occur!
 **/
tcu::TestNode::IterateResult GeometryShaderVSGSVariableTypeMismatchTest::iterate()
{
	bool has_shader_compilation_failed = true;
	bool result						   = true;

	/* This test should only run if EXT_geometry_shader is supported. */
	if (!m_is_geometry_shader_extension_supported)
	{
		throw tcu::NotSupportedError(GEOMETRY_SHADER_EXTENSION_NOT_SUPPORTED, "", __FILE__, __LINE__);
	}

	/* Create a program object */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	m_po_id = gl.createProgram();

	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateProgram() call failed.");

	/* Create shader objects */
	m_fs_id = gl.createShader(GL_FRAGMENT_SHADER);
	m_gs_id = gl.createShader(GL_GEOMETRY_SHADER);
	m_vs_id = gl.createShader(GL_VERTEX_SHADER);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateShader() call(s) failed.");

	/* Try to link the test program object */
	const char* gs_code_raw = "${VERSION}\n"
							  "${GEOMETRY_SHADER_REQUIRE}\n"
							  "\n"
							  "layout (points)                   in;\n"
							  "layout (points, max_vertices = 1) out;\n"
							  "\n"
							  "in vec4 test[];\n"
							  "\n"
							  "void main()\n"
							  "{\n"
							  "    gl_Position = test[0];\n"
							  "    EmitVertex();\n"
							  "}\n";

	const char* vs_code_raw = "${VERSION}\n"
							  "\n"
							  "out vec3 test;\n"
							  "\n"
							  "void main()\n"
							  "{\n"
							  "    test = vec3(gl_VertexID);\n"
							  "}\n";

	std::string fs_code_specialized		= specializeShader(1, &dummy_fs_code);
	const char* fs_code_specialized_raw = fs_code_specialized.c_str();
	std::string gs_code_specialized		= specializeShader(1, &gs_code_raw);
	const char* gs_code_specialized_raw = gs_code_specialized.c_str();
	std::string vs_code_specialized		= specializeShader(1, &vs_code_raw);
	const char* vs_code_specialized_raw = vs_code_specialized.c_str();

	if (TestCaseBase::buildProgram(m_po_id, m_gs_id, 1,					 /* n_sh1_body_parts */
								   &gs_code_specialized_raw, m_vs_id, 1, /* n_sh2_body_parts */
								   &vs_code_specialized_raw, m_fs_id, 1, /* n_sh3_body_parts */
								   &fs_code_specialized_raw, &has_shader_compilation_failed))
	{
		m_testCtx.getLog() << tcu::TestLog::Message
						   << "Program object was linked successfully, whereas a failure was expected."
						   << tcu::TestLog::EndMessage;

		result = false;
	}

	if (has_shader_compilation_failed)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Shader compilation failed unexpectedly."
						   << tcu::TestLog::EndMessage;

		result = false;
	}

	if (result)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
	}

	return STOP;
}

/** Constructor
 *
 * @param context       Test context
 * @param extParams     Not used.
 * @param name          Test case's name
 * @param description   Test case's description
 **/
GeometryShaderVSGSVariableQualifierMismatchTest::GeometryShaderVSGSVariableQualifierMismatchTest(
	Context& context, const ExtParameters& extParams, const char* name, const char* description)
	: TestCaseBase(context, extParams, name, description), m_fs_id(0), m_gs_id(0), m_po_id(0), m_vs_id(0)
{
}

/** Deinitializes GLES objects created during the test. */
void GeometryShaderVSGSVariableQualifierMismatchTest::deinit()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (m_fs_id != 0)
	{
		gl.deleteShader(m_fs_id);

		m_fs_id = 0;
	}

	if (m_gs_id != 0)
	{
		gl.deleteShader(m_gs_id);

		m_gs_id = 0;
	}

	if (m_po_id != 0)
	{
		gl.deleteProgram(m_po_id);

		m_po_id = 0;
	}

	if (m_vs_id != 0)
	{
		gl.deleteShader(m_vs_id);

		m_vs_id = 0;
	}

	/* Release base class */
	TestCaseBase::deinit();
}

/** Executes the test.
 *
 *  Sets the test result to QP_TEST_RESULT_FAIL if the test failed, QP_TEST_RESULT_PASS otherwise.
 *  @return STOP if the test has finished, CONTINUE to indicate iterate should be called once again.
 *  Note the function throws exception should an error occur!
 **/
tcu::TestNode::IterateResult GeometryShaderVSGSVariableQualifierMismatchTest::iterate()
{
	bool has_shader_compilation_failed = true;
	bool result						   = true;

	/* This test should only run if EXT_geometry_shader is supported. */
	if (!m_is_geometry_shader_extension_supported)
	{
		throw tcu::NotSupportedError(GEOMETRY_SHADER_EXTENSION_NOT_SUPPORTED, "", __FILE__, __LINE__);
	}

	/* Create a program object */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	m_po_id = gl.createProgram();

	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateProgram() call failed.");

	/* Create shader objects */
	m_fs_id = gl.createShader(GL_FRAGMENT_SHADER);
	m_gs_id = gl.createShader(GL_GEOMETRY_SHADER);
	m_vs_id = gl.createShader(GL_VERTEX_SHADER);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateShader() call(s) failed.");

	/* Try to link the test program object */
	const char* gs_code_raw = "${VERSION}\n"
							  "${GEOMETRY_SHADER_REQUIRE}\n"
							  "\n"
							  "layout (points)                   in;\n"
							  "layout (points, max_vertices = 1) out;\n"
							  "\n"
							  "in flat vec4 test[];\n"
							  "\n"
							  "void main()\n"
							  "{\n"
							  "    gl_Position = test[0];\n"
							  "    EmitVertex();\n"
							  "}\n";

	const char* vs_code_raw = "${VERSION}\n"
							  "${GEOMETRY_SHADER_REQUIRE}\n"
							  "\n"
							  "out vec4 test;\n"
							  "\n"
							  "void main()\n"
							  "{\n"
							  "    test = vec4(gl_VertexID);\n"
							  "}\n";

	std::string fs_code_specialized		= specializeShader(1, &dummy_fs_code);
	const char* fs_code_specialized_raw = fs_code_specialized.c_str();
	std::string gs_code_specialized		= specializeShader(1, &gs_code_raw);
	const char* gs_code_specialized_raw = gs_code_specialized.c_str();
	std::string vs_code_specialized		= specializeShader(1, &vs_code_raw);
	const char* vs_code_specialized_raw = vs_code_specialized.c_str();

	bool buildSuccess = TestCaseBase::buildProgram(m_po_id, m_gs_id, 1,					 /* n_sh1_body_parts */
												   &gs_code_specialized_raw, m_vs_id, 1, /* n_sh2_body_parts */
												   &vs_code_specialized_raw, m_fs_id, 1, /* n_sh3_body_parts */
												   &fs_code_specialized_raw, &has_shader_compilation_failed);

	if (!buildSuccess && glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::core(3, 0)))
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Program object linking failed. Success was expected."
						   << tcu::TestLog::EndMessage;

		result = false;
	}

	if (buildSuccess && !glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::core(3, 0)))
	{
		m_testCtx.getLog() << tcu::TestLog::Message
						   << "Program object was linked successfully, whereas a failure was expected."
						   << tcu::TestLog::EndMessage;
		result = false;
	}

	if (has_shader_compilation_failed)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Shader compilation failed unexpectedly."
						   << tcu::TestLog::EndMessage;

		result = false;
	}

	if (result)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
	}

	return STOP;
}

/** Constructor
 *
 * @param context       Test context
 * @param extParams     Not used.
 * @param name          Test case's name
 * @param description   Test case's description
 **/
GeometryShaderVSGSArrayedVariableSizeMismatchTest::GeometryShaderVSGSArrayedVariableSizeMismatchTest(
	Context& context, const ExtParameters& extParams, const char* name, const char* description)
	: TestCaseBase(context, extParams, name, description), m_fs_id(0), m_gs_id(0), m_po_id(0), m_vs_id(0)
{
}

/** Deinitializes GLES objects created during the test. */
void GeometryShaderVSGSArrayedVariableSizeMismatchTest::deinit()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (m_fs_id != 0)
	{
		gl.deleteShader(m_fs_id);

		m_fs_id = 0;
	}

	if (m_gs_id != 0)
	{
		gl.deleteShader(m_gs_id);

		m_gs_id = 0;
	}

	if (m_po_id != 0)
	{
		gl.deleteProgram(m_po_id);

		m_po_id = 0;
	}

	if (m_vs_id != 0)
	{
		gl.deleteShader(m_vs_id);

		m_vs_id = 0;
	}

	/* Release base class */
	TestCaseBase::deinit();
}

/** Executes the test.
 *
 *  Sets the test result to QP_TEST_RESULT_FAIL if the test failed, QP_TEST_RESULT_PASS otherwise.
 *  @return STOP if the test has finished, CONTINUE to indicate iterate should be called once again.
 *  Note the function throws exception should an error occur!
 **/
tcu::TestNode::IterateResult GeometryShaderVSGSArrayedVariableSizeMismatchTest::iterate()
{
	bool has_shader_compilation_failed = true;
	bool result						   = true;

	/* This test should only run if EXT_geometry_shader is supported. */
	if (!m_is_geometry_shader_extension_supported)
	{
		throw tcu::NotSupportedError(GEOMETRY_SHADER_EXTENSION_NOT_SUPPORTED, "", __FILE__, __LINE__);
	}

	/* Create a program object */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	m_po_id = gl.createProgram();

	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateProgram() call failed.");

	/* Create shader objects */
	m_fs_id = gl.createShader(GL_FRAGMENT_SHADER);
	m_gs_id = gl.createShader(GL_GEOMETRY_SHADER);
	m_vs_id = gl.createShader(GL_VERTEX_SHADER);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateShader() call(s) failed.");

	/* Try to link the test program object */
	const char* gs_code_raw = "${VERSION}\n"
							  "${GEOMETRY_SHADER_REQUIRE}\n"
							  "\n"
							  "layout (points)                   in;\n"
							  "layout (points, max_vertices = 1) out;\n"
							  "\n"
							  "in vec4 Color1[];\n"
							  "in vec4 Color2[2];\n"
							  "in vec4 Color3[3];\n"
							  "\n"
							  "void main()\n"
							  "{\n"
							  "    gl_Position = Color1[0] + Color2[1] + Color3[2];\n"
							  "    EmitVertex();\n"
							  "}\n";

	const char* vs_code_raw = "${VERSION}\n"
							  "${GEOMETRY_SHADER_REQUIRE}\n"
							  "\n"
							  "out vec4 Color1;\n"
							  "out vec4 Color2;\n"
							  "out vec4 Color3;\n"
							  "\n"
							  "void main()\n"
							  "{\n"
							  "    Color1 = vec4(gl_VertexID, 0.0,         0.0,         0.0);\n"
							  "    Color2 = vec4(0.0,         gl_VertexID, 0.0,         0.0);\n"
							  "    Color3 = vec4(0.0,         0.0,         gl_VertexID, 0.0);\n"
							  "}\n";

	std::string fs_code_specialized		= specializeShader(1, &dummy_fs_code);
	const char* fs_code_specialized_raw = fs_code_specialized.c_str();
	std::string gs_code_specialized		= specializeShader(1, &gs_code_raw);
	const char* gs_code_specialized_raw = gs_code_specialized.c_str();
	std::string vs_code_specialized		= specializeShader(1, &vs_code_raw);
	const char* vs_code_specialized_raw = vs_code_specialized.c_str();

	if (TestCaseBase::buildProgram(m_po_id, m_gs_id, 1,					 /* n_sh1_body_parts */
								   &gs_code_specialized_raw, m_vs_id, 1, /* n_sh2_body_parts */
								   &vs_code_specialized_raw, m_fs_id, 1, /* n_sh3_body_parts */
								   &fs_code_specialized_raw, &has_shader_compilation_failed))
	{
		m_testCtx.getLog() << tcu::TestLog::Message
						   << "Program object was linked successfully, whereas a failure was expected."
						   << tcu::TestLog::EndMessage;

		result = false;
	}

	if (result)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
	}

	return STOP;
}

/** Constructor
 *
 * @param context       Test context
 * @param extParams     Not used.
 * @param name          Test case's name
 * @param description   Test case's description
 **/
GeometryShaderFragCoordRedeclarationTest::GeometryShaderFragCoordRedeclarationTest(Context&				context,
																				   const ExtParameters& extParams,
																				   const char*			name,
																				   const char*			description)
	: TestCaseBase(context, extParams, name, description), m_fs_id(0), m_gs_id(0), m_po_id(0), m_vs_id(0)
{
}

/** Deinitializes GLES objects created during the test. */
void GeometryShaderFragCoordRedeclarationTest::deinit()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (m_fs_id != 0)
	{
		gl.deleteShader(m_fs_id);

		m_fs_id = 0;
	}

	if (m_gs_id != 0)
	{
		gl.deleteShader(m_gs_id);

		m_gs_id = 0;
	}

	if (m_po_id != 0)
	{
		gl.deleteProgram(m_po_id);

		m_po_id = 0;
	}

	if (m_vs_id != 0)
	{
		gl.deleteShader(m_vs_id);

		m_vs_id = 0;
	}

	/* Release base class */
	TestCaseBase::deinit();
}

/** Executes the test.
 *
 *  Sets the test result to QP_TEST_RESULT_FAIL if the test failed, QP_TEST_RESULT_PASS otherwise.
 *  @return STOP if the test has finished, CONTINUE to indicate iterate should be called once again.
 *  Note the function throws exception should an error occur!
 **/
tcu::TestNode::IterateResult GeometryShaderFragCoordRedeclarationTest::iterate()
{
	bool has_shader_compilation_failed = true;
	bool result						   = true;

	/* This test should only run if EXT_geometry_shader is supported. */
	if (!m_is_geometry_shader_extension_supported)
	{
		throw tcu::NotSupportedError(GEOMETRY_SHADER_EXTENSION_NOT_SUPPORTED, "", __FILE__, __LINE__);
	}

	/* Create a program object */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	m_po_id = gl.createProgram();

	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateProgram() call failed.");

	/* Create shader objects */
	m_fs_id = gl.createShader(GL_FRAGMENT_SHADER);
	m_gs_id = gl.createShader(GL_GEOMETRY_SHADER);
	m_vs_id = gl.createShader(GL_VERTEX_SHADER);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateShader() call(s) failed.");

	/* Try to link the test program object */
	const char* gs_code_raw = "${VERSION}\n"
							  "${GEOMETRY_SHADER_REQUIRE}\n"
							  "\n"
							  "layout (points)                   in;\n"
							  "layout (points, max_vertices = 1) out;\n"
							  "\n"
							  "in vec4 gl_FragCoord;\n"
							  "\n"
							  "void main()\n"
							  "{\n"
							  "    gl_Position = gl_FragCoord;\n"
							  "    EmitVertex();\n"
							  "}\n";

	std::string fs_code_specialized		= specializeShader(1, &dummy_fs_code);
	const char* fs_code_specialized_raw = fs_code_specialized.c_str();
	std::string gs_code_specialized		= specializeShader(1, &gs_code_raw);
	const char* gs_code_specialized_raw = gs_code_specialized.c_str();
	std::string vs_code_specialized		= specializeShader(1, &dummy_vs_code);
	const char* vs_code_specialized_raw = vs_code_specialized.c_str();

	if (TestCaseBase::buildProgram(m_po_id, m_gs_id, 1,					 /* n_sh1_body_parts */
								   &gs_code_specialized_raw, m_vs_id, 1, /* n_sh2_body_parts */
								   &vs_code_specialized_raw, m_fs_id, 1, /* n_sh3_body_parts */
								   &fs_code_specialized_raw, &has_shader_compilation_failed))
	{
		m_testCtx.getLog() << tcu::TestLog::Message
						   << "Program object was linked successfully, whereas a failure was expected."
						   << tcu::TestLog::EndMessage;

		result = false;
	}

	if (result)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
	}

	return STOP;
}

/** Constructor
 *
 * @param context       Test context
 * @param extParams     Not used.
 * @param name          Test case's name
 * @param description   Test case's description
 **/
GeometryShaderLocationAliasingTest::GeometryShaderLocationAliasingTest(Context& context, const ExtParameters& extParams,
																	   const char* name, const char* description)
	: TestCaseBase(context, extParams, name, description), m_fs_id(0), m_gs_id(0), m_po_id(0), m_vs_id(0)
{
}

/** Deinitializes GLES objects created during the test. */
void GeometryShaderLocationAliasingTest::deinit()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (m_fs_id != 0)
	{
		gl.deleteShader(m_fs_id);

		m_fs_id = 0;
	}

	if (m_gs_id != 0)
	{
		gl.deleteShader(m_gs_id);

		m_gs_id = 0;
	}

	if (m_po_id != 0)
	{
		gl.deleteProgram(m_po_id);

		m_po_id = 0;
	}

	if (m_vs_id != 0)
	{
		gl.deleteShader(m_vs_id);

		m_vs_id = 0;
	}

	/* Release base class */
	TestCaseBase::deinit();
}

/** Executes the test.
 *
 *  Sets the test result to QP_TEST_RESULT_FAIL if the test failed, QP_TEST_RESULT_PASS otherwise.
 *  @return STOP if the test has finished, CONTINUE to indicate iterate should be called once again.
 *  Note the function throws exception should an error occur!
 **/
tcu::TestNode::IterateResult GeometryShaderLocationAliasingTest::iterate()
{
	bool has_program_link_succeeded	= true;
	bool result						   = true;

	/* This test should only run if EXT_geometry_shader is supported. */
	if (!m_is_geometry_shader_extension_supported)
	{
		throw tcu::NotSupportedError(GEOMETRY_SHADER_EXTENSION_NOT_SUPPORTED, "", __FILE__, __LINE__);
	}

	/* Create a program object */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	m_po_id = gl.createProgram();

	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateProgram() call failed.");

	/* Create shader objects */
	m_fs_id = gl.createShader(GL_FRAGMENT_SHADER);
	m_gs_id = gl.createShader(GL_GEOMETRY_SHADER);
	m_vs_id = gl.createShader(GL_VERTEX_SHADER);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateShader() call(s) failed.");

	/* Try to link the test program object */
	const char* gs_code_raw = "${VERSION}\n"
							  "${GEOMETRY_SHADER_REQUIRE}\n"
							  "\n"
							  "layout (points)                   in;\n"
							  "layout (points, max_vertices = 1) out;\n"
							  "\n"
							  "layout (location = 2) out vec4 test;\n"
							  "layout (location = 2) out vec4 test2;\n"
							  "\n"
							  "void main()\n"
							  "{\n"
							  "    gl_Position = gl_in[0].gl_Position;\n"
							  "    test = vec4(1.0, 0.0, 0.0, 1.0);\n"
							  "    test2 = vec4(1.0, 0.0, 0.0, 1.0);\n"
							  "    EmitVertex();\n"
							  "}\n";

	std::string fs_code_specialized		= specializeShader(1, &dummy_fs_code);
	const char* fs_code_specialized_raw = fs_code_specialized.c_str();
	std::string gs_code_specialized		= specializeShader(1, &gs_code_raw);
	const char* gs_code_specialized_raw = gs_code_specialized.c_str();
	std::string vs_code_specialized		= specializeShader(1, &dummy_vs_code);
	const char* vs_code_specialized_raw = vs_code_specialized.c_str();

	has_program_link_succeeded = TestCaseBase::buildProgram(
		m_po_id, m_gs_id, 1 /* n_sh1_body_parts */, &gs_code_specialized_raw, m_vs_id, 1 /* n_sh2_body_parts */,
		&vs_code_specialized_raw, m_fs_id, 1 /* n_sh3_body_parts */, &fs_code_specialized_raw, NULL);
	if (has_program_link_succeeded)
	{
		m_testCtx.getLog() << tcu::TestLog::Message
						   << "Program object was compiled and linked successfully, whereas a failure was expected."
						   << tcu::TestLog::EndMessage;

		result = false;
	}

	if (result)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
	}

	return STOP;
}

/** Constructor
 *
 * @param context       Test context
 * @param extParams     Not used.
 * @param name          Test case's name
 * @param description   Test case's description
 **/
GeometryShaderMoreACsInGSThanSupportedTest::GeometryShaderMoreACsInGSThanSupportedTest(Context&				context,
																					   const ExtParameters& extParams,
																					   const char*			name,
																					   const char*			description)
	: TestCaseBase(context, extParams, name, description), m_fs_id(0), m_gs_id(0), m_po_id(0), m_vs_id(0)
{
}

/** Deinitializes GLES objects created during the test. */
void GeometryShaderMoreACsInGSThanSupportedTest::deinit()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (m_fs_id != 0)
	{
		gl.deleteShader(m_fs_id);

		m_fs_id = 0;
	}

	if (m_gs_id != 0)
	{
		gl.deleteShader(m_gs_id);

		m_gs_id = 0;
	}

	if (m_po_id != 0)
	{
		gl.deleteProgram(m_po_id);

		m_po_id = 0;
	}

	if (m_vs_id != 0)
	{
		gl.deleteShader(m_vs_id);

		m_vs_id = 0;
	}

	/* Release base class */
	TestCaseBase::deinit();
}

/* Retrieves test-specific geometry shader source code.
 *
 * @return Requested string.
 */
std::string GeometryShaderMoreACsInGSThanSupportedTest::getGSCode()
{
	std::stringstream	 code_sstream;
	const glw::Functions& gl			   = m_context.getRenderContext().getFunctions();
	glw::GLint			  gl_max_ACs_value = 0;

	/* Retrieve GL_MAX_GEOMETRY_ATOMIC_COUNTERS_EXT pname value */
	gl.getIntegerv(m_glExtTokens.MAX_GEOMETRY_ATOMIC_COUNTERS, &gl_max_ACs_value);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv() failed for GL_MAX_GEOMETRY_ATOMIC_COUNTERS_EXT pname");

	/* Form the GS */
	code_sstream << "${VERSION}\n"
					"${GEOMETRY_SHADER_REQUIRE}\n"
					"\n"
					"layout (points)                   in;\n"
					"layout (points, max_vertices = 1) out;\n"
					"\n";

	for (glw::GLint n_ac = 0; n_ac < (gl_max_ACs_value + 1); ++n_ac)
	{
		code_sstream << "layout(binding = 0) uniform atomic_uint counter" << n_ac << ";\n";
	}

	code_sstream << "\n"
					"void main()\n"
					"{\n";

	for (glw::GLint n_ac = 0; n_ac < (gl_max_ACs_value + 1); ++n_ac)
	{
		code_sstream << "    if ((gl_PrimitiveIDIn % " << (n_ac + 1) << ") == 0) atomicCounterIncrement(counter" << n_ac
					 << ");\n";
	}

	code_sstream << "\n"
					"    gl_Position = vec4(0.0, 0.0, 0.0, 1.0);\n"
					"    EmitVertex();\n"
					"}\n";

	/* Form a specialized version of the GS source code */
	std::string gs_code				= code_sstream.str();
	const char* gs_code_raw			= gs_code.c_str();
	std::string gs_code_specialized = specializeShader(1, /* parts */
													   &gs_code_raw);

	return gs_code_specialized;
}

/** Executes the test.
 *
 *  Sets the test result to QP_TEST_RESULT_FAIL if the test failed, QP_TEST_RESULT_PASS otherwise.
 *  @return STOP if the test has finished, CONTINUE to indicate iterate should be called once again.
 *  Note the function throws exception should an error occur!
 **/
tcu::TestNode::IterateResult GeometryShaderMoreACsInGSThanSupportedTest::iterate()
{
	bool has_shader_compilation_failed = true;
	bool result						   = true;

	/* This test should only run if EXT_geometry_shader is supported. */
	if (!m_is_geometry_shader_extension_supported)
	{
		throw tcu::NotSupportedError(GEOMETRY_SHADER_EXTENSION_NOT_SUPPORTED, "", __FILE__, __LINE__);
	}

	/* Create a program object */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	m_po_id = gl.createProgram();

	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateProgram() call failed.");

	/* Create shader objects */
	m_fs_id = gl.createShader(GL_FRAGMENT_SHADER);
	m_gs_id = gl.createShader(GL_GEOMETRY_SHADER);
	m_vs_id = gl.createShader(GL_VERTEX_SHADER);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateShader() call(s) failed.");

	/* Try to link the test program object */
	std::string fs_code_specialized		= specializeShader(1, &dummy_fs_code);
	const char* fs_code_specialized_raw = fs_code_specialized.c_str();
	std::string gs_code_specialized		= getGSCode();
	const char* gs_code_specialized_raw = gs_code_specialized.c_str();
	std::string vs_code_specialized		= specializeShader(1, &dummy_vs_code);
	const char* vs_code_specialized_raw = vs_code_specialized.c_str();

	if (TestCaseBase::buildProgram(m_po_id, m_gs_id, 1,					 /* n_sh1_body_parts */
								   &gs_code_specialized_raw, m_vs_id, 1, /* n_sh2_body_parts */
								   &vs_code_specialized_raw, m_fs_id, 1, /* n_sh3_body_parts */
								   &fs_code_specialized_raw, &has_shader_compilation_failed))
	{
		m_testCtx.getLog() << tcu::TestLog::Message
						   << "Program object was linked successfully, whereas a failure was expected."
						   << tcu::TestLog::EndMessage;

		result = false;
	}

	if (result)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
	}

	return STOP;
}

/** Constructor
 *
 * @param context       Test context
 * @param extParams     Not used.
 * @param name          Test case's name
 * @param description   Test case's description
 **/
GeometryShaderMoreACBsInGSThanSupportedTest::GeometryShaderMoreACBsInGSThanSupportedTest(Context&			  context,
																						 const ExtParameters& extParams,
																						 const char*		  name,
																						 const char* description)
	: TestCaseBase(context, extParams, name, description), m_fs_id(0), m_gs_id(0), m_po_id(0), m_vs_id(0)
{
}

/** Deinitializes GLES objects created during the test. */
void GeometryShaderMoreACBsInGSThanSupportedTest::deinit()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (m_fs_id != 0)
	{
		gl.deleteShader(m_fs_id);

		m_fs_id = 0;
	}

	if (m_gs_id != 0)
	{
		gl.deleteShader(m_gs_id);

		m_gs_id = 0;
	}

	if (m_po_id != 0)
	{
		gl.deleteProgram(m_po_id);

		m_po_id = 0;
	}

	if (m_vs_id != 0)
	{
		gl.deleteShader(m_vs_id);

		m_vs_id = 0;
	}

	/* Release base class */
	TestCaseBase::deinit();
}

/* Retrieves test-specific geometry shader source code.
 *
 * @return Requested string.
 */
std::string GeometryShaderMoreACBsInGSThanSupportedTest::getGSCode()
{
	std::stringstream	 code_sstream;
	const glw::Functions& gl				= m_context.getRenderContext().getFunctions();
	glw::GLint			  gl_max_ACBs_value = 0;

	/* Retrieve GL_MAX_GEOMETRY_ATOMIC_COUNTER_BUFFERS_EXT pname value */
	gl.getIntegerv(m_glExtTokens.MAX_GEOMETRY_ATOMIC_COUNTER_BUFFERS, &gl_max_ACBs_value);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv() failed for GL_MAX_GEOMETRY_ATOMIC_COUNTER_BUFFERS_EXT pname");

	/* Form the GS */
	code_sstream << "${VERSION}\n"
					"${GEOMETRY_SHADER_REQUIRE}\n"
					"\n"
					"layout (points)                   in;\n"
					"layout (points, max_vertices = 1) out;\n"
					"\n";

	for (glw::GLint n_acb = 0; n_acb < (gl_max_ACBs_value + 1); ++n_acb)
	{
		code_sstream << "layout(binding = " << n_acb << ") uniform atomic_uint counter" << n_acb << ";\n";
	}

	code_sstream << "\n"
					"void main()\n"
					"{\n";

	for (glw::GLint n_acb = 0; n_acb < (gl_max_ACBs_value + 1); ++n_acb)
	{
		code_sstream << "    if ((gl_PrimitiveIDIn % " << (n_acb + 1) << ") == 0) atomicCounterIncrement(counter"
					 << n_acb << ");\n";
	}

	code_sstream << "\n"
					"    gl_Position = vec4(0.0, 0.0, 0.0, 1.0);\n"
					"    EmitVertex();\n"
					"}\n";

	/* Form a specialized version of the GS source code */
	std::string gs_code				= code_sstream.str();
	const char* gs_code_raw			= gs_code.c_str();
	std::string gs_code_specialized = specializeShader(1, /* parts */
													   &gs_code_raw);

	return gs_code_specialized;
}

/** Executes the test.
 *
 *  Sets the test result to QP_TEST_RESULT_FAIL if the test failed, QP_TEST_RESULT_PASS otherwise.
 *  @return STOP if the test has finished, CONTINUE to indicate iterate should be called once again.
 *  Note the function throws exception should an error occur!
 **/
tcu::TestNode::IterateResult GeometryShaderMoreACBsInGSThanSupportedTest::iterate()
{
	bool has_shader_compilation_failed = true;
	bool result						   = true;

	/* This test should only run if EXT_geometry_shader is supported. */
	if (!m_is_geometry_shader_extension_supported)
	{
		throw tcu::NotSupportedError(GEOMETRY_SHADER_EXTENSION_NOT_SUPPORTED, "", __FILE__, __LINE__);
	}

	/* Create a program object */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	m_po_id = gl.createProgram();

	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateProgram() call failed.");

	/* Create shader objects */
	m_fs_id = gl.createShader(GL_FRAGMENT_SHADER);
	m_gs_id = gl.createShader(GL_GEOMETRY_SHADER);
	m_vs_id = gl.createShader(GL_VERTEX_SHADER);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateShader() call(s) failed.");

	/* Try to link the test program object */
	std::string fs_code_specialized		= specializeShader(1, &dummy_fs_code);
	const char* fs_code_specialized_raw = fs_code_specialized.c_str();
	std::string gs_code_specialized		= getGSCode();
	const char* gs_code_specialized_raw = gs_code_specialized.c_str();
	std::string vs_code_specialized		= specializeShader(1, &dummy_vs_code);
	const char* vs_code_specialized_raw = vs_code_specialized.c_str();

	if (TestCaseBase::buildProgram(m_po_id, m_gs_id, 1,					 /* n_sh1_body_parts */
								   &gs_code_specialized_raw, m_vs_id, 1, /* n_sh2_body_parts */
								   &vs_code_specialized_raw, m_fs_id, 1, /* n_sh3_body_parts */
								   &fs_code_specialized_raw, &has_shader_compilation_failed))
	{
		m_testCtx.getLog() << tcu::TestLog::Message
						   << "Program object was linked successfully, whereas a failure was expected."
						   << tcu::TestLog::EndMessage;

		result = false;
	}

	if (result)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
	}

	return STOP;
}

/** Constructor
 *
 * @param context       Test context
 * @param extParams     Not used.
 * @param name          Test case's name
 * @param description   Test case's description
 **/
GeometryShaderCompilationFailTest::GeometryShaderCompilationFailTest(Context& context, const ExtParameters& extParams,
																	 const char* name, const char* description)
	: TestCaseBase(context, extParams, name, description), m_fs_id(0), m_gs_id(0), m_po_id(0), m_vs_id(0)
{
}

/** Deinitializes GLES objects created during the test. */
void GeometryShaderCompilationFailTest::deinit()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (m_fs_id != 0)
	{
		gl.deleteShader(m_fs_id);

		m_fs_id = 0;
	}

	if (m_gs_id != 0)
	{
		gl.deleteShader(m_gs_id);

		m_gs_id = 0;
	}

	if (m_po_id != 0)
	{
		gl.deleteProgram(m_po_id);

		m_po_id = 0;
	}

	if (m_vs_id != 0)
	{
		gl.deleteShader(m_vs_id);

		m_vs_id = 0;
	}

	/* Release base class */
	TestCaseBase::deinit();
}

/** Executes the test.
 *
 *  Sets the test result to QP_TEST_RESULT_FAIL if the test failed, QP_TEST_RESULT_PASS otherwise.
 *  @return STOP if the test has finished, CONTINUE to indicate iterate should be called once again.
 *  Note the function throws exception should an error occur!
 **/
tcu::TestNode::IterateResult GeometryShaderCompilationFailTest::iterate()
{
	/* Define Vertex Shader's code for the purpose of this test. */
	const char* gs_code = "${VERSION}\n"
						  "${GEOMETRY_SHADER_REQUIRE}\n"
						  "\n"
						  "layout (points)                   in;\n"
						  "layout (points, max_vertices = 1) out;\n"
						  "\n"
						  "void main()\n"
						  "{\n"
						  "    gl_Position = gl_in[0].gl_Position;\n"
						  "    mitVertex();\n"
						  "}\n";

	bool has_shader_compilation_failed = true;
	bool result						   = true;

	/* This test should only run if EXT_geometry_shader is supported. */
	if (!m_is_geometry_shader_extension_supported)
	{
		throw tcu::NotSupportedError(GEOMETRY_SHADER_EXTENSION_NOT_SUPPORTED, "", __FILE__, __LINE__);
	}

	/* Create a program object */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	m_po_id = gl.createProgram();

	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateProgram() call failed.");

	/* Create shader objects */
	m_fs_id = gl.createShader(GL_FRAGMENT_SHADER);
	m_gs_id = gl.createShader(GL_GEOMETRY_SHADER);
	m_vs_id = gl.createShader(GL_VERTEX_SHADER);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateShader() call(s) failed.");

	/* Try to link the test program object */
	std::string fs_code_specialized		= specializeShader(1, &dummy_fs_code);
	const char* fs_code_specialized_raw = fs_code_specialized.c_str();

	std::string gs_code_specialized		= specializeShader(1, &gs_code);
	const char* gs_code_specialized_raw = gs_code_specialized.c_str();

	std::string vs_code_specialized		= specializeShader(1, &dummy_vs_code);
	const char* vs_code_specialized_raw = vs_code_specialized.c_str();

	if (TestCaseBase::buildProgram(m_po_id, m_gs_id, 1,					 /* n_sh1_body_parts */
								   &gs_code_specialized_raw, m_vs_id, 1, /* n_sh2_body_parts */
								   &vs_code_specialized_raw, m_fs_id, 1, /* n_sh3_body_parts */
								   &fs_code_specialized_raw, &has_shader_compilation_failed))
	{
		m_testCtx.getLog() << tcu::TestLog::Message
						   << "Program object was linked successfully, whereas a failure was expected."
						   << tcu::TestLog::EndMessage;

		result = false;
	}

	if (!has_shader_compilation_failed)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Shader compilation succeeded, whereas a failure was expected."
						   << tcu::TestLog::EndMessage;

		result = false;
	}

	if (result)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
	}

	return STOP;
}

/** Constructor
 *
 * @param context       Test context
 * @param extParams     Not used.
 * @param name          Test case's name
 * @param description   Test case's description
 **/
GeometryShaderMoreInputVerticesThanAvailableTest::GeometryShaderMoreInputVerticesThanAvailableTest(
	Context& context, const ExtParameters& extParams, const char* name, const char* description)
	: TestCaseBase(context, extParams, name, description)
	, m_fs_id(0)
	, m_gs_ids(NULL)
	, m_number_of_gs(5 /*taken from test spec*/)
	, m_po_ids(NULL)
	, m_vs_id(0)
	, m_vao_id(0)
{
}

/** Deinitializes GLES objects created during the test. */
void GeometryShaderMoreInputVerticesThanAvailableTest::deinit()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (m_fs_id != 0)
	{
		gl.deleteShader(m_fs_id);
		m_fs_id = 0;
	}

	if (m_gs_ids != 0)
	{
		for (glw::GLuint i = 0; i < m_number_of_gs; ++i)
		{
			gl.deleteShader(m_gs_ids[i]);
			m_gs_ids[i] = 0;
		}

		delete[] m_gs_ids;
		m_gs_ids = NULL;
	}

	if (m_po_ids != 0)
	{
		for (glw::GLuint i = 0; i < m_number_of_gs; ++i)
		{
			gl.deleteProgram(m_po_ids[i]);
			m_po_ids[i] = 0;
		}

		delete[] m_po_ids;
		m_po_ids = NULL;
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

	/* Release base class */
	TestCaseBase::deinit();
}

/** Executes the test.
 *
 *  Sets the test result to QP_TEST_RESULT_FAIL if the test failed, QP_TEST_RESULT_PASS otherwise.
 *  @return STOP if the test has finished, CONTINUE to indicate iterate should be called once again.
 *  Note the function throws exception should an error occur!
 **/
tcu::TestNode::IterateResult GeometryShaderMoreInputVerticesThanAvailableTest::iterate()
{
	/* Define 5 Geometry Shaders for purpose of this test. */
	const char* gs_code_points = "${VERSION}\n"
								 "${GEOMETRY_SHADER_REQUIRE}\n"
								 "\n"
								 "layout (points)                   in;\n"
								 "layout (points, max_vertices = 1) out;\n"
								 "\n"
								 "void main()\n"
								 "{\n"
								 "    gl_Position = gl_in[1].gl_Position;\n"
								 "    EmitVertex();\n"
								 "}\n";

	const char* gs_code_lines = "${VERSION}\n"
								"${GEOMETRY_SHADER_REQUIRE}\n"
								"\n"
								"layout (lines)                    in;\n"
								"layout (points, max_vertices = 1) out;\n"
								"\n"
								"void main()\n"
								"{\n"
								"    gl_Position = gl_in[2].gl_Position;\n"
								"    EmitVertex();\n"
								"}\n";

	const char* gs_code_lines_adjacency = "${VERSION}\n"
										  "${GEOMETRY_SHADER_REQUIRE}\n"
										  "\n"
										  "layout (lines_adjacency)          in;\n"
										  "layout (points, max_vertices = 1) out;\n"
										  "\n"
										  "void main()\n"
										  "{\n"
										  "    gl_Position = gl_in[4].gl_Position;\n"
										  "    EmitVertex();\n"
										  "}\n";

	const char* gs_code_triangles = "${VERSION}\n"
									"${GEOMETRY_SHADER_REQUIRE}\n"
									"\n"
									"layout (triangles)                in;\n"
									"layout (points, max_vertices = 1) out;\n"
									"\n"
									"void main()\n"
									"{\n"
									"    gl_Position = gl_in[3].gl_Position;\n"
									"    EmitVertex();\n"
									"}\n";

	const char* gs_code_triangles_adjacency = "${VERSION}\n"
											  "${GEOMETRY_SHADER_REQUIRE}\n"
											  "\n"
											  "layout (triangles_adjacency)      in;\n"
											  "layout (points, max_vertices = 1) out;\n"
											  "\n"
											  "void main()\n"
											  "{\n"
											  "    gl_Position = gl_in[6].gl_Position;\n"
											  "    EmitVertex();\n"
											  "}\n";

	bool has_shader_compilation_failed = true;
	bool result						   = true;

	m_gs_ids = new glw::GLuint[m_number_of_gs];
	m_po_ids = new glw::GLuint[m_number_of_gs];

	/* This test should only run if EXT_geometry_shader is supported. */
	if (!m_is_geometry_shader_extension_supported)
	{
		throw tcu::NotSupportedError(GEOMETRY_SHADER_EXTENSION_NOT_SUPPORTED, "", __FILE__, __LINE__);
	}

	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Create program objects & geometry shader objects. */
	for (glw::GLuint i = 0; i < m_number_of_gs; ++i)
	{
		m_gs_ids[i] = gl.createShader(GL_GEOMETRY_SHADER);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateProgram() call failed.");

		m_po_ids[i] = gl.createProgram();
		GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateShader() call(s) failed.");
	}

	/* Create shader object. */
	m_fs_id = gl.createShader(GL_FRAGMENT_SHADER);
	m_vs_id = gl.createShader(GL_VERTEX_SHADER);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateShader() call(s) failed.");

	/* Try to link the test program object */
	std::string fs_code_specialized		= specializeShader(1, &dummy_fs_code);
	const char* fs_code_specialized_raw = fs_code_specialized.c_str();

	std::string gs_codes_specialized[] = { specializeShader(1, &gs_code_points), specializeShader(1, &gs_code_lines),
										   specializeShader(1, &gs_code_lines_adjacency),
										   specializeShader(1, &gs_code_triangles),
										   specializeShader(1, &gs_code_triangles_adjacency) };

	const char* gs_codes_specialized_raw[] = { gs_codes_specialized[0].c_str(), gs_codes_specialized[1].c_str(),
											   gs_codes_specialized[2].c_str(), gs_codes_specialized[3].c_str(),
											   gs_codes_specialized[4].c_str() };

	std::string vs_code_specialized		= specializeShader(1, &dummy_vs_code);
	const char* vs_code_specialized_raw = vs_code_specialized.c_str();

	for (glw::GLuint i = 0; i < m_number_of_gs; ++i)
	{
		if (TestCaseBase::buildProgram(m_po_ids[i], m_fs_id, 1,					 /* n_sh1_body_parts */
									   &fs_code_specialized_raw, m_gs_ids[i], 1, /* n_sh2_body_parts */
									   &gs_codes_specialized_raw[i], m_vs_id, 1, /* n_sh3_body_parts */
									   &vs_code_specialized_raw, &has_shader_compilation_failed))
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "Program object linking successful for i = "
							   << "[" << i << "], whereas a failure was expected." << tcu::TestLog::EndMessage;

			result = false;
			break;
		}
	}

	if (result)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
	}

	return STOP;
}

/** Constructor
 *
 * @param context       Test context
 * @param extParams     Not used.
 * @param name          Test case's name
 * @param description   Test case's description
 **/
GeometryShaderTransformFeedbackVertexAndGeometryShaderCaptureTest::
	GeometryShaderTransformFeedbackVertexAndGeometryShaderCaptureTest(Context& context, const ExtParameters& extParams,
																	  const char* name, const char* description)
	: TestCaseBase(context, extParams, name, description), m_fs_id(0), m_gs_id(0), m_po_id(0), m_vs_id(0)
{
}

/** Deinitializes GLES objects created during the test. */
void GeometryShaderTransformFeedbackVertexAndGeometryShaderCaptureTest::deinit()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (m_fs_id != 0)
	{
		gl.deleteShader(m_fs_id);
		m_fs_id = 0;
	}

	if (m_gs_id != 0)
	{
		gl.deleteShader(m_gs_id);
		m_gs_id = 0;
	}

	if (m_po_id != 0)
	{
		gl.deleteProgram(m_po_id);
		m_po_id = 0;
	}

	if (m_vs_id != 0)
	{
		gl.deleteShader(m_vs_id);
		m_vs_id = 0;
	}

	/* Release base class */
	TestCaseBase::deinit();
}

/** Executes the test.
 *
 *  Sets the test result to QP_TEST_RESULT_FAIL if the test failed, QP_TEST_RESULT_PASS otherwise.
 *  @return STOP if the test has finished, CONTINUE to indicate iterate should be called once again.
 *  Note the function throws exception should an error occur!
 **/
tcu::TestNode::IterateResult GeometryShaderTransformFeedbackVertexAndGeometryShaderCaptureTest::iterate()
{
	/* Define Geometry Shader for purpose of this test. */
	const char* gs_code =
		"${VERSION}\n"
		"${GEOMETRY_SHADER_REQUIRE}\n"
		"\n"
		"layout (points)                   in;\n"
		"layout (points, max_vertices = 1) out;\n"
		"\n"
		"in int vertexID;\n"
		"out vec4 out_gs_1;\n"
		"\n"
		"void main()\n"
		"{\n"
		"    out_gs_1 = vec4(vertexID[0] * 2, vertexID[0] * 2 + 1, vertexID[0] * 2 + 2, vertexID[0] * 2 + 3);\n"
		"    gl_Position = vec4(0, 0, 0, 1);\n"
		"    EmitVertex();\n"
		"}\n";

	/* Define Vertex Shader for purpose of this test. */
	const char* vs_code = "${VERSION}\n"
						  "\n"
						  "flat out ivec4 out_vs_1;\n"
						  "flat out int vertexID;\n"
						  "\n"
						  "void main()\n"
						  "{\n"
						  "    vertexID = gl_VertexID;\n"
						  "    out_vs_1 = ivec4(gl_VertexID, gl_VertexID + 1, gl_VertexID + 2, gl_VertexID + 3);\n"
						  "    gl_Position = vec4(1.0, 0.0, 0.0, 1.0);\n"
						  "}\n";

	bool has_shader_compilation_failed = true;
	bool result						   = true;

	/* This test should only run if EXT_geometry_shader is supported. */
	if (!m_is_geometry_shader_extension_supported)
	{
		throw tcu::NotSupportedError(GEOMETRY_SHADER_EXTENSION_NOT_SUPPORTED, "", __FILE__, __LINE__);
	}

	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Create program object. */
	m_po_id = gl.createProgram();
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateProgram() call(s) failed.");

	/* Specify output variables to be captured. */
	const char* tf_varyings[] = { "out_vs_1", "out_gs_1" };

	gl.transformFeedbackVaryings(m_po_id, 2 /*count*/, tf_varyings, GL_INTERLEAVED_ATTRIBS);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTransformFeedbackVaryings() call(s) failed.");

	/* Create shader objects. */
	m_fs_id = gl.createShader(GL_FRAGMENT_SHADER);
	m_gs_id = gl.createShader(GL_GEOMETRY_SHADER);
	m_vs_id = gl.createShader(GL_VERTEX_SHADER);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateShader() call(s) failed.");

	/* Try to link the test program object */
	std::string fs_code_specialized		= specializeShader(1, &dummy_fs_code);
	const char* fs_code_specialized_raw = fs_code_specialized.c_str();

	std::string gs_code_specialized		= specializeShader(1, &gs_code);
	const char* gs_code_specialized_raw = fs_code_specialized.c_str();

	std::string vs_code_specialized		= specializeShader(1, &vs_code);
	const char* vs_code_specialized_raw = vs_code_specialized.c_str();

	if (TestCaseBase::buildProgram(m_po_id, m_fs_id, 1,					 /* n_sh1_body_parts */
								   &fs_code_specialized_raw, m_gs_id, 1, /* n_sh2_body_parts */
								   &gs_code_specialized_raw, m_vs_id, 1, /* n_sh3_body_parts */
								   &vs_code_specialized_raw, &has_shader_compilation_failed))
	{
		m_testCtx.getLog() << tcu::TestLog::Message
						   << "Program object linking successful, whereas a failure was expected."
						   << tcu::TestLog::EndMessage;

		result = false;
	}

	if (result)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
	}

	return STOP;
}

} // namespace glcts
