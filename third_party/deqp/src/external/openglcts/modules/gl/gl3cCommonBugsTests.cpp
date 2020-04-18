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

/**
 */ /*!
 * \file  gl3cCommonBugsTests.cpp
 * \brief Short conformance tests which verify functionality which has either
 *        been found to be broken on one publically available driver, or whose
 *        behavior varies between vendors.
 */ /*-------------------------------------------------------------------*/

#include "gl3cCommonBugsTests.hpp"
#include "gluContextInfo.hpp"
#include "gluDefs.hpp"
#include "gluRenderContext.hpp"
#include "glwEnums.hpp"
#include "glwFunctions.hpp"
#include "tcuTestLog.hpp"

#include <cstring>
#include <string>
#include <vector>

#ifndef GL_SPARSE_BUFFER_PAGE_SIZE_ARB
#define GL_SPARSE_BUFFER_PAGE_SIZE_ARB 0x82F8
#endif
#ifndef GL_SPARSE_STORAGE_BIT_ARB
#define GL_SPARSE_STORAGE_BIT_ARB 0x0400
#endif

namespace gl3cts
{
/** Constructor.
 *
 *  @param context     Rendering context
 *  @param name        Test name
 *  @param description Test description
 */
GetProgramivActiveUniformBlockMaxNameLengthTest::GetProgramivActiveUniformBlockMaxNameLengthTest(deqp::Context& context)
	: TestCase(context, "CommonBug_GetProgramivActiveUniformBlockMaxNameLength",
			   "Verifies GL_ACTIVE_UNIFORM_BLOCK_MAX_NAME_LENGTH pname is recognized by glGetProgramiv()")
	, m_fs_id(0)
	, m_po_id(0)
	, m_vs_id(0)
{
	/* Left blank intentionally */
}

/** Tears down any GL objects set up to run the test. */
void GetProgramivActiveUniformBlockMaxNameLengthTest::deinit()
{
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

	if (m_vs_id != 0)
	{
		gl.deleteShader(m_vs_id);

		m_vs_id = 0;
	}
}

/** Stub init method */
void GetProgramivActiveUniformBlockMaxNameLengthTest::init()
{
	/* Nothing to do here */
}

/** Initializes all GL objects required to run the test */
bool GetProgramivActiveUniformBlockMaxNameLengthTest::initTest()
{
	glw::GLint			  compile_status = false;
	const glw::Functions& gl			 = m_context.getRenderContext().getFunctions();
	glw::GLint			  link_status	= false;
	bool				  result		 = true;

	const char* fs_body = "#version 140\n"
						  "\n"
						  "uniform data\n"
						  "{\n"
						  "    vec4 temp;\n"
						  "};\n"
						  "\n"
						  "out vec4 result;\n"
						  "\n"
						  "void main()\n"
						  "{\n"
						  "    result = temp;\n"
						  "}\n";

	const char* vs_body = "#version 140\n"
						  "\n"
						  "uniform data2\n"
						  "{\n"
						  "    vec4 temp2;\n"
						  "};\n"
						  "\n"
						  "void main()\n"
						  "{\n"
						  "    gl_Position = temp2;\n"
						  "}\n";

	m_po_id = gl.createProgram();
	m_fs_id = gl.createShader(GL_FRAGMENT_SHADER);
	m_vs_id = gl.createShader(GL_VERTEX_SHADER);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateProgram() / glCreateShader() call(s) failed.");

	gl.attachShader(m_po_id, m_fs_id);
	gl.attachShader(m_po_id, m_vs_id);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glAttachShader() call(s) failed.");

	gl.shaderSource(m_fs_id, 1,			/* count */
					&fs_body, DE_NULL); /* length */
	gl.shaderSource(m_vs_id, 1,			/* count */
					&vs_body, DE_NULL); /* length */

	GLU_EXPECT_NO_ERROR(gl.getError(), "glShaderSource() call(s) failed.");

	gl.compileShader(m_fs_id);
	gl.compileShader(m_vs_id);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glCompileShader() call(s) failed.");

	/* Have the shaders compiled successfully? */
	const glw::GLuint  shader_ids[] = { m_fs_id, m_vs_id };
	const unsigned int n_shader_ids = static_cast<const unsigned int>(sizeof(shader_ids) / sizeof(shader_ids[0]));

	for (unsigned int n_shader_id = 0; n_shader_id < n_shader_ids; ++n_shader_id)
	{
		gl.getShaderiv(shader_ids[n_shader_id], GL_COMPILE_STATUS, &compile_status);

		GLU_EXPECT_NO_ERROR(gl.getError(), "glGetShaderiv() call failed.");

		if (compile_status != GL_TRUE)
		{
			m_context.getTestContext().getLog() << tcu::TestLog::Message << "Shader compilation failure"
												<< tcu::TestLog::EndMessage;

			result = false;
			goto end;
		}
	} /* for (all shader IDs) */

	/* Link the PO */
	gl.linkProgram(m_po_id);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glLinkProgram() call failed.");

	gl.getProgramiv(m_po_id, GL_LINK_STATUS, &link_status);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetProgramiv() call failed.");

	if (link_status != GL_TRUE)
	{
		m_context.getTestContext().getLog() << tcu::TestLog::Message << "Program linking failure"
											<< tcu::TestLog::EndMessage;

		result = false;
		goto end;
	}

end:
	return result;
}

/** Executes test iteration.
 *
 *  @return Returns STOP when test has finished executing, CONTINUE if more iterations are needed.
 */
tcu::TestNode::IterateResult GetProgramivActiveUniformBlockMaxNameLengthTest::iterate()
{
	bool result = true;

	/* Execute the query */
	glw::GLenum			  error_code			= GL_NO_ERROR;
	const glw::GLint	  expected_result_value = static_cast<glw::GLint>(strlen("data2") + 1 /* terminator */);
	const glw::Functions& gl					= m_context.getRenderContext().getFunctions();
	glw::GLint			  result_value			= 0;

	/* Only execute if we're daeling with GL 3.1 or newer.. */
	if (!glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::core(3, 1)))
	{
		goto end;
	}

	/* Set up the test program object */
	if (!initTest())
	{
		result = false;

		goto end;
	}

	gl.getProgramiv(m_po_id, GL_ACTIVE_UNIFORM_BLOCK_MAX_NAME_LENGTH, &result_value);

	error_code = gl.getError();

	if (error_code != GL_NO_ERROR)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "glGetIntegerv() generated error [" << error_code
						   << "] for GL_ACTIVE_UNIFORM_BLOCK_MAX_NAME_LENGTH" << tcu::TestLog::EndMessage;

		result = false;
	}
	else if (result_value != expected_result_value)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "glGetIntegerv() returned an invalid value of " << result_value
						   << " instead of the expected value of " << (strlen("data2") + 1 /* terminator */)
						   << " for the GL_ACTIVE_UNIFORM_BLOCK_MAX_NAME_LENGTH, "
							  "where the longest uniform block name is data2."
						   << tcu::TestLog::EndMessage;

		result = false;
	}
end:
	m_testCtx.setTestResult(result ? QP_TEST_RESULT_PASS : QP_TEST_RESULT_FAIL, result ? "Pass" : "Fail");

	return STOP;
}

/** Constructor.
 *
 *  @param context     Rendering context
 *  @param name        Test name
 *  @param description Test description
 */
InputVariablesCannotBeModifiedTest::InputVariablesCannotBeModifiedTest(deqp::Context& context)
	: TestCase(context, "CommonBug_InputVariablesCannotBeModified", "Verifies that input variables cannot be modified.")
	, m_fs_id(0)
	, m_gs_id(0)
	, m_tc_id(0)
	, m_te_id(0)
	, m_vs_id(0)
{
	/* Left blank on purpose */
}

/** Deinitializes all GL objects created for the purpose of running the test,
 *  as well as any client-side buffers allocated at initialization time
 */
void InputVariablesCannotBeModifiedTest::deinit()
{
	const glw::Functions& gl		   = m_context.getRenderContext().getFunctions();
	glw::GLuint*		  so_id_ptrs[] = {
		&m_fs_id, &m_gs_id, &m_tc_id, &m_te_id, &m_vs_id,
	};
	const unsigned int n_so_id_ptrs = static_cast<const unsigned int>(sizeof(so_id_ptrs) / sizeof(so_id_ptrs[0]));

	for (unsigned int n_so_id_ptr = 0; n_so_id_ptr < n_so_id_ptrs; ++n_so_id_ptr)
	{
		gl.deleteShader(*so_id_ptrs[n_so_id_ptr]);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glDeleteShader() call failed.");

		*so_id_ptrs[n_so_id_ptr] = 0;
	} /* for (all created shader objects) */
}

/** Dummy init function */
void InputVariablesCannotBeModifiedTest::init()
{
	/* Left blank on purpose */
}

/** Retrieves a literal corresponding to the test iteration enum.
 *
 *  @param iteration Enum to retrieve the string for.
 *
 *  @return Requested object.
 **/
std::string InputVariablesCannotBeModifiedTest::getIterationName(_test_iteration iteration) const
{
	std::string result;

	switch (iteration)
	{
	case TEST_ITERATION_INPUT_FS_VARIABLE:
		result = "Fragment shader input variable";
		break;
	case TEST_ITERATION_INPUT_FS_VARIABLE_IN_INPUT_BLOCK:
		result = "Fragment shader input variable wrapped in an input block";
		break;
	case TEST_ITERATION_INPUT_FS_VARIABLE_PASSED_TO_INOUT_FUNCTION_PARAMETER:
		result = "Fragment shader input variable passed to an inout function parameter";
		break;
	case TEST_ITERATION_INPUT_FS_VARIABLE_PASSED_TO_OUT_FUNCTION_PARAMETER:
		result = "Fragment shader input variable passed to an out function parameter";
		break;
	case TEST_ITERATION_INPUT_GS_VARIABLE:
		result = "Geometry shader input variable";
		break;
	case TEST_ITERATION_INPUT_GS_VARIABLE_IN_INPUT_BLOCK:
		result = "Geometry shader input variable wrapped in an input block";
		break;
	case TEST_ITERATION_INPUT_GS_VARIABLE_PASSED_TO_INOUT_FUNCTION_PARAMETER:
		result = "Geometry shader input variable passed to an inout function parameter";
		break;
	case TEST_ITERATION_INPUT_GS_VARIABLE_PASSED_TO_OUT_FUNCTION_PARAMETER:
		result = "Geometry shader input variable passed to an out function parameter";
		break;
	case TEST_ITERATION_INPUT_TC_VARIABLE_IN_INPUT_BLOCK:
		result = "Tessellation control shader variable wrapped in an input block";
		break;
	case TEST_ITERATION_INPUT_TC_VARIABLE_PASSED_TO_INOUT_FUNCTION_PARAMETER:
		result = "Tessellation control shader variable passed to an inout function parameter";
		break;
	case TEST_ITERATION_INPUT_TC_VARIABLE_PASSED_TO_OUT_FUNCTION_PARAMETER:
		result = "Tessellation control shader variable passed to an out function parameter";
		break;
	case TEST_ITERATION_INPUT_TE_PATCH_VARIABLE:
		result = "Tessellation evaluation shader patch input variable";
		break;
	case TEST_ITERATION_INPUT_TE_VARIABLE:
		result = "Tessellation evaluation shader input variable";
		break;
	case TEST_ITERATION_INPUT_TE_VARIABLE_IN_INPUT_BLOCK:
		result = "Tessellation evaluation shader patch input variable wrapped in an input block";
		break;
	case TEST_ITERATION_INPUT_TE_VARIABLE_PASSED_TO_INOUT_FUNCTION_PARAMETER:
		result = "Tessellation evlauation shader patch input variable passed to an inout function parameter";
		break;
	case TEST_ITERATION_INPUT_TE_VARIABLE_PASSED_TO_OUT_FUNCTION_PARAMETER:
		result = "Tessellation evaluation shader patch input variable passed to an out function parameter";
		break;
	case TEST_ITERATION_INPUT_VS_VARIABLE:
		result = "Vertex shader input variable";
		break;
	case TEST_ITERATION_INPUT_VS_VARIABLE_PASSED_TO_INOUT_FUNCTION_PARAMETER:
		result = "Vertex shader input variable passed to an inout function parameter";
		break;
	case TEST_ITERATION_INPUT_VS_VARIABLE_PASSED_TO_OUT_FUNCTION_PARAMETER:
		result = "Vertex shader input variable passed to an out function parameter";
		break;

	default:
		TCU_FAIL("Unrecognized test iteration type.");
	} /* switch (iteration) */

	return result;
}

/** Retrieves a vertex shader body for the user-specified iteration enum.
 *
 *  @param iteration Iteration to retrieve the shader body for.
 *
 *  @return Requested string object.
 */
void InputVariablesCannotBeModifiedTest::getIterationData(_test_iteration iteration,
														  glu::ApiType*   out_required_min_context_type_ptr,
														  _shader_stage*  out_target_shader_stage_ptr,
														  std::string*	out_body_ptr) const
{
	switch (iteration)
	{
	case TEST_ITERATION_INPUT_FS_VARIABLE:
	{
		*out_required_min_context_type_ptr = glu::ApiType::core(3, 1);
		*out_target_shader_stage_ptr	   = SHADER_STAGE_FRAGMENT;

		*out_body_ptr = "#version 140\n"
						"\n"
						"in  vec4 data;\n"
						"out vec4 result;\n"
						"\n"
						"void main()\n"
						"{\n"
						"    data   += vec4(1.0);\n"
						"    result  = data;\n"
						"}\n";

		break;
	}

	case TEST_ITERATION_INPUT_FS_VARIABLE_IN_INPUT_BLOCK:
	{
		*out_required_min_context_type_ptr = glu::ApiType::core(4, 0);
		*out_target_shader_stage_ptr	   = SHADER_STAGE_FRAGMENT;

		*out_body_ptr = "#version 400\n"
						"\n"
						"in inputBlock\n"
						"{\n"
						"    vec4 data;\n"
						"};\n"
						"\n"
						"out vec4 result;\n"
						"\n"
						"void main()\n"
						"{\n"
						"    data   += vec4(1.0);\n"
						"    result  = data;\n"
						"}\n";

		break;
	}

	case TEST_ITERATION_INPUT_FS_VARIABLE_PASSED_TO_INOUT_FUNCTION_PARAMETER:
	{
		*out_required_min_context_type_ptr = glu::ApiType::core(3, 1);
		*out_target_shader_stage_ptr	   = SHADER_STAGE_FRAGMENT;

		*out_body_ptr = "#version 140\n"
						"\n"
						"in  vec4 data;\n"
						"out vec4 result;\n"
						"\n"
						"void testFunc(inout vec4 arg)\n"
						"{\n"
						"    arg += vec4(1.0);\n"
						"}\n"
						"\n"
						"void main()\n"
						"{\n"
						"    testFunc(data);\n"
						"\n"
						"    result = data;\n"
						"}\n";

		break;
	}

	case TEST_ITERATION_INPUT_FS_VARIABLE_PASSED_TO_OUT_FUNCTION_PARAMETER:
	{
		*out_required_min_context_type_ptr = glu::ApiType::core(3, 1);
		*out_target_shader_stage_ptr	   = SHADER_STAGE_FRAGMENT;

		*out_body_ptr = "#version 140\n"
						"\n"
						"in  vec4 data;\n"
						"out vec4 result;\n"
						"\n"
						"void testFunc(out vec4 arg)\n"
						"{\n"
						"    arg = vec4(1.0);\n"
						"}\n"
						"\n"
						"void main()\n"
						"{\n"
						"    testFunc(data);\n"
						"\n"
						"    result = data;\n"
						"}\n";

		break;
	}

	case TEST_ITERATION_INPUT_GS_VARIABLE:
	{
		*out_required_min_context_type_ptr = glu::ApiType::core(3, 2);
		*out_target_shader_stage_ptr	   = SHADER_STAGE_GEOMETRY;

		*out_body_ptr = "#version 150\n"
						"\n"
						"layout(points)                   in;\n"
						"layout(points, max_vertices = 1) out;\n"
						"\n"
						"in vec4 data[];\n"
						"\n"
						"void main()\n"
						"{\n"
						"    data[0]     += vec4(1.0);\n"
						"    gl_Position  = data[0];\n"
						"\n"
						"    EmitVertex();\n"
						"}\n";

		break;
	}

	case TEST_ITERATION_INPUT_GS_VARIABLE_IN_INPUT_BLOCK:
	{
		*out_required_min_context_type_ptr = glu::ApiType::core(4, 0);
		*out_target_shader_stage_ptr	   = SHADER_STAGE_GEOMETRY;

		*out_body_ptr = "#version 400\n"
						"\n"
						"layout(points)                   in;\n"
						"layout(points, max_vertices = 1) out;\n"
						"\n"
						"in inputBlock\n"
						"{\n"
						"    vec4 data[];\n"
						"};\n"
						"\n"
						"void main()\n"
						"{\n"
						"    data[0]     += vec4(1.0);\n"
						"    gl_Position  = data[0];\n"
						"\n"
						"    EmitVertex();\n"
						"}\n";

		break;
	}

	case TEST_ITERATION_INPUT_GS_VARIABLE_PASSED_TO_INOUT_FUNCTION_PARAMETER:
	{
		*out_required_min_context_type_ptr = glu::ApiType::core(3, 2);
		*out_target_shader_stage_ptr	   = SHADER_STAGE_GEOMETRY;

		*out_body_ptr = "#version 150\n"
						"\n"
						"layout(points)                   in;\n"
						"layout(points, max_vertices = 1) out;\n"
						"\n"
						"in vec4 data[];\n"
						"\n"
						"void testFunc(inout vec4 arg)\n"
						"{\n"
						"    arg += vec4(1.0);\n"
						"}\n"
						"\n"
						"void main()\n"
						"{\n"
						"    testFunc(data[0]);\n"
						"\n"
						"    gl_Position = data[0];\n"
						"\n"
						"    EmitVertex();\n"
						"}\n";

		break;
	}

	case TEST_ITERATION_INPUT_GS_VARIABLE_PASSED_TO_OUT_FUNCTION_PARAMETER:
	{
		*out_required_min_context_type_ptr = glu::ApiType::core(3, 2);
		*out_target_shader_stage_ptr	   = SHADER_STAGE_GEOMETRY;

		*out_body_ptr = "#version 150\n"
						"\n"
						"layout(points)                   in;\n"
						"layout(points, max_vertices = 1) out;\n"
						"\n"
						"in vec4 data[];\n"
						"\n"
						"void testFunc(out vec4 arg)\n"
						"{\n"
						"    arg = vec4(1.0);\n"
						"}\n"
						"\n"
						"void main()\n"
						"{\n"
						"    testFunc(data[0]);\n"
						"\n"
						"    gl_Position = data[0];\n"
						"\n"
						"    EmitVertex();\n"
						"}\n";

		break;
	}

	case TEST_ITERATION_INPUT_TC_VARIABLE:
	{
		*out_required_min_context_type_ptr = glu::ApiType::core(4, 0);
		*out_target_shader_stage_ptr	   = SHADER_STAGE_TESSELLATION_CONTROL;

		*out_body_ptr = "#version 400\n"
						"\n"
						"layout(vertices = 4) out;\n"
						"\n"
						"in  vec4 data[];\n"
						"out vec4 result;\n"
						"\n"
						"void main()\n"
						"{\n"
						"    data[0] += vec4(1.0);\n"
						"    result   = data[0];\n"
						"\n"
						"}\n";

		break;
	}

	case TEST_ITERATION_INPUT_TC_VARIABLE_IN_INPUT_BLOCK:
	{
		*out_required_min_context_type_ptr = glu::ApiType::core(4, 0);
		*out_target_shader_stage_ptr	   = SHADER_STAGE_TESSELLATION_CONTROL;

		*out_body_ptr = "#version 400\n"
						"\n"
						"layout(vertices = 4) out;\n"
						"\n"
						"in inputBlock\n"
						"{\n"
						"    vec4 data;\n"
						"} inData[];\n"
						"\n"
						"patch out vec4 result[];\n"
						"\n"
						"void main()\n"
						"{\n"
						"    inData[0].data          += vec4(1.0);\n"
						"    result[gl_InvocationID]  = inData[0].data;\n"
						"\n"
						"    gl_TessLevelInner[0] = 1.0;\n"
						"    gl_TessLevelInner[1] = 1.0;\n"
						"    gl_TessLevelOuter[0] = 1.0;\n"
						"    gl_TessLevelOuter[1] = 1.0;\n"
						"    gl_TessLevelOuter[2] = 1.0;\n"
						"    gl_TessLevelOuter[3] = 1.0;\n"
						"}\n";

		break;
	}

	case TEST_ITERATION_INPUT_TC_VARIABLE_PASSED_TO_INOUT_FUNCTION_PARAMETER:
	{
		*out_required_min_context_type_ptr = glu::ApiType::core(4, 0);
		*out_target_shader_stage_ptr	   = SHADER_STAGE_TESSELLATION_CONTROL;

		*out_body_ptr = "#version 400\n"
						"\n"
						"layout(vertices = 4) out;\n"
						"\n"
						"in  vec4 data[];\n"
						"out vec4 result;\n"
						"\n"
						"void testFunc(inout vec4 arg)\n"
						"{\n"
						"    arg += vec4(1.0);\n"
						"}\n"
						"\n"
						"void main()\n"
						"{\n"
						"    testFunc(data[0]);\n"
						"\n"
						"    result = data[0];\n"
						"\n"
						"}\n";

		break;
	}

	case TEST_ITERATION_INPUT_TC_VARIABLE_PASSED_TO_OUT_FUNCTION_PARAMETER:
	{
		*out_required_min_context_type_ptr = glu::ApiType::core(4, 0);
		*out_target_shader_stage_ptr	   = SHADER_STAGE_TESSELLATION_CONTROL;

		*out_body_ptr = "#version 400\n"
						"\n"
						"layout(vertices = 4) out;\n"
						"\n"
						"in  vec4 data[];\n"
						"out vec4 result;\n"
						"\n"
						"void testFunc(out vec4 arg)\n"
						"{\n"
						"    arg = vec4(1.0);\n"
						"}\n"
						"\n"
						"void main()\n"
						"{\n"
						"    testFunc(data[0]);\n"
						"\n"
						"    result = data[0];\n"
						"\n"
						"}\n";

		break;
	}

	case TEST_ITERATION_INPUT_TE_PATCH_VARIABLE:
	case TEST_ITERATION_INPUT_TE_VARIABLE:
	{
		std::stringstream body_sstream;

		*out_required_min_context_type_ptr = glu::ApiType::core(4, 0);
		*out_target_shader_stage_ptr	   = SHADER_STAGE_TESSELLATION_EVALUATION;

		body_sstream << "#version 400\n"
						"\n"
						"layout(triangles) in;\n"
						"\n"
					 << ((iteration == TEST_ITERATION_INPUT_TE_PATCH_VARIABLE) ? "patch " : "") << "in  vec4 data[];\n"
					 << "      out vec4 result;\n"
						"\n"
						"void main()\n"
						"{\n"
						"    data[0]     += vec4(1.0);\n"
						"    gl_Position  = data[0];\n"
						"}\n";

		*out_body_ptr = body_sstream.str();
		break;
	}

	case TEST_ITERATION_INPUT_TE_VARIABLE_IN_INPUT_BLOCK:
	{
		*out_required_min_context_type_ptr = glu::ApiType::core(4, 0);
		*out_target_shader_stage_ptr	   = SHADER_STAGE_TESSELLATION_EVALUATION;

		*out_body_ptr = "#version 400\n"
						"\n"
						"layout(triangles) in;\n"
						"\n"
						"in inputBlock\n"
						"{\n"
						"    vec4 data;\n"
						"} inData[];\n"
						"\n"
						"void main()\n"
						"{\n"
						"    inData[0].data += vec4(1.0);\n"
						"    gl_Position     = inData[0].data;\n"
						"}\n";

		break;
	}

	case TEST_ITERATION_INPUT_TE_VARIABLE_PASSED_TO_INOUT_FUNCTION_PARAMETER:
	{
		*out_required_min_context_type_ptr = glu::ApiType::core(4, 0);
		*out_target_shader_stage_ptr	   = SHADER_STAGE_TESSELLATION_EVALUATION;

		*out_body_ptr = "#version 400\n"
						"\n"
						"layout(triangles) in;\n"
						"\n"
						"in  vec4 data[];\n"
						"out vec4 result;\n"
						"\n"
						"void testFunc(inout vec4 arg)\n"
						"{\n"
						"    arg += vec4(1.0);\n"
						"}\n"
						"\n"
						"void main()\n"
						"{\n"
						"    testFunc(data[0]);\n"
						"\n"
						"    gl_Position  = data[0];\n"
						"}\n";

		break;
	}

	case TEST_ITERATION_INPUT_TE_VARIABLE_PASSED_TO_OUT_FUNCTION_PARAMETER:
	{
		*out_required_min_context_type_ptr = glu::ApiType::core(4, 0);
		*out_target_shader_stage_ptr	   = SHADER_STAGE_TESSELLATION_EVALUATION;

		*out_body_ptr = "#version 400\n"
						"\n"
						"layout(triangles) in;\n"
						"\n"
						"in  vec4 data[];\n"
						"out vec4 result;\n"
						"\n"
						"void testFunc(out vec4 arg)\n"
						"{\n"
						"    arg = vec4(1.0);\n"
						"}\n"
						"\n"
						"void main()\n"
						"{\n"
						"    testFunc(data[0]);\n"
						"\n"
						"    gl_Position = data[0];\n"
						"}\n";

		break;
	}

	case TEST_ITERATION_INPUT_VS_VARIABLE:
	{
		*out_required_min_context_type_ptr = glu::ApiType::core(3, 1);
		*out_target_shader_stage_ptr	   = SHADER_STAGE_VERTEX;

		*out_body_ptr = "#version 140\n"
						"\n"
						"in vec4 data;\n"
						"\n"
						"void main()\n"
						"{\n"
						"    data        += vec4(1.0);\n"
						"    gl_Position  = data;\n"
						"}\n";

		break;
	}

	case TEST_ITERATION_INPUT_VS_VARIABLE_PASSED_TO_INOUT_FUNCTION_PARAMETER:
	{
		*out_required_min_context_type_ptr = glu::ApiType::core(3, 1);
		*out_target_shader_stage_ptr	   = SHADER_STAGE_VERTEX;

		*out_body_ptr = "#version 140\n"
						"\n"
						"in vec4 data;\n"
						"\n"
						"void testFunc(inout vec4 argument)\n"
						"{\n"
						"    argument += vec4(1.0);\n"
						"}\n"
						"\n"
						"void main()\n"
						"{\n"
						"    testFunc(data);\n"
						"\n"
						"    gl_Position = data;\n"
						"}\n";

		break;
	}

	case TEST_ITERATION_INPUT_VS_VARIABLE_PASSED_TO_OUT_FUNCTION_PARAMETER:
	{
		*out_required_min_context_type_ptr = glu::ApiType::core(3, 1);
		*out_target_shader_stage_ptr	   = SHADER_STAGE_VERTEX;

		*out_body_ptr = "#version 140\n"
						"\n"
						"in vec4 data;\n"
						"\n"
						"void testFunc(out vec4 argument)\n"
						"{\n"
						"    argument = vec4(1.0);\n"
						"}\n"
						"\n"
						"void main()\n"
						"{\n"
						"    testFunc(data);\n"
						"\n"
						"    gl_Position = data;\n"
						"}\n";

		break;
	}

	default:
		TCU_FAIL("Unrecognized test iteration type");
	} /* switch (iteration) */
}

/** Executes test iteration.
 *
 *  @return Returns STOP when test has finished executing, CONTINUE if more iterations are needed.
 */
tcu::TestNode::IterateResult InputVariablesCannotBeModifiedTest::iterate()
{
	const glu::ContextType context_type = m_context.getRenderContext().getType();
	const glw::Functions&  gl			= m_context.getRenderContext().getFunctions();
	bool				   result		= true;

	/* Create shader objects */
	if (glu::contextSupports(context_type, glu::ApiType::core(3, 2)))
	{
		m_gs_id = gl.createShader(GL_GEOMETRY_SHADER);
	}

	if (glu::contextSupports(context_type, glu::ApiType::core(4, 0)))
	{
		m_tc_id = gl.createShader(GL_TESS_CONTROL_SHADER);
		m_te_id = gl.createShader(GL_TESS_EVALUATION_SHADER);
	}

	m_fs_id = gl.createShader(GL_FRAGMENT_SHADER);
	m_vs_id = gl.createShader(GL_VERTEX_SHADER);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateShader() call failed.");

	/* Execute all test iterations.. */
	for (int current_iteration = static_cast<int>(TEST_ITERATION_FIRST);
		 current_iteration < static_cast<int>(TEST_ITERATION_COUNT); current_iteration++)
	{
		glw::GLint	compile_status = GL_FALSE;
		std::string   current_iteration_body;
		const char*   current_iteration_body_raw_ptr = NULL;
		glu::ApiType  current_iteration_min_context_type;
		_shader_stage current_iteration_shader_stage;
		glw::GLuint   so_id = 0;

		getIterationData(static_cast<_test_iteration>(current_iteration), &current_iteration_min_context_type,
						 &current_iteration_shader_stage, &current_iteration_body);

		current_iteration_body_raw_ptr = current_iteration_body.c_str();

		/* Determine shader ID for the iteration. If the shader stage is not supported
		 * for the running context, skip it. */
		if (!glu::contextSupports(context_type, current_iteration_min_context_type))
		{
			continue;
		}

		switch (current_iteration_shader_stage)
		{
		case SHADER_STAGE_FRAGMENT:
			so_id = m_fs_id;
			break;
		case SHADER_STAGE_GEOMETRY:
			so_id = m_gs_id;
			break;
		case SHADER_STAGE_TESSELLATION_CONTROL:
			so_id = m_tc_id;
			break;
		case SHADER_STAGE_TESSELLATION_EVALUATION:
			so_id = m_te_id;
			break;
		case SHADER_STAGE_VERTEX:
			so_id = m_vs_id;
			break;

		default:
		{
			TCU_FAIL("Unrecognized shader stage type");
		}
		} /* switch (current_iteration_shader_stage) */

		DE_ASSERT(so_id != 0);

		/* Assign the source code to the SO */
		gl.shaderSource(so_id, 1,								   /* count */
						&current_iteration_body_raw_ptr, DE_NULL); /* length */
		GLU_EXPECT_NO_ERROR(gl.getError(), "glShaderSource() call failed.");

		/* Try to compile the shader object. */
		gl.compileShader(so_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glCompileShader() call failed.");

		gl.getShaderiv(so_id, GL_COMPILE_STATUS, &compile_status);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGetShaderiv() call failed.");

		char temp[1024];

		gl.getShaderInfoLog(so_id, 1024, NULL, temp);

		if (compile_status == GL_TRUE)
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "The following "
							   << getShaderStageName(current_iteration_shader_stage)
							   << " shader, used for test iteration ["
							   << getIterationName(static_cast<_test_iteration>(current_iteration))
							   << "] "
								  "was compiled successfully, even though it is invalid. Body:"
								  "\n>>\n"
							   << current_iteration_body << "\n<<\n"
							   << tcu::TestLog::EndMessage;

			result = false;
		}
	} /* for (all test iterations) */

	m_testCtx.setTestResult(result ? QP_TEST_RESULT_PASS : QP_TEST_RESULT_FAIL, result ? "Pass" : "Fail");

	return STOP;
}

/** Returns a literal corresponding to the shader stage enum used by the test.
 *
 *  @param stage Shader stage to use for the query.
 *
 *  @return Requested string.
 **/
std::string InputVariablesCannotBeModifiedTest::getShaderStageName(_shader_stage stage) const
{
	std::string result = "?!";

	switch (stage)
	{
	case SHADER_STAGE_FRAGMENT:
		result = "fragment shader";
		break;
	case SHADER_STAGE_GEOMETRY:
		result = "geometry shader";
		break;
	case SHADER_STAGE_TESSELLATION_CONTROL:
		result = "tessellation control shader";
		break;
	case SHADER_STAGE_TESSELLATION_EVALUATION:
		result = "tessellation evaluation shader";
		break;
	case SHADER_STAGE_VERTEX:
		result = "vertex shader";
		break;
	} /* switch (stage) */

	return result;
}

/** Constructor.
 *
 *  @param context     Rendering context
 *  @param name        Test name
 *  @param description Test description
 */
InvalidUseCasesForAllNotFuncsAndExclMarkOpTest::InvalidUseCasesForAllNotFuncsAndExclMarkOpTest(deqp::Context& context)
	: deqp::TestCase(context, "CommonBug_InvalidUseCasesForAllNotFuncsAndExclMarkOp",
					 "Verifies that ! operator does not accept bvec{2,3,4} arguments, "
					 "all() and not() functions do not accept a bool argument.")
	, m_vs_id(0)
{
	/* Left blank on purpose */
}

/** Deinitializes all GL objects created for the purpose of running the test,
 *  as well as any client-side buffers allocated at initialization time
 */
void InvalidUseCasesForAllNotFuncsAndExclMarkOpTest::deinit()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (m_vs_id != 0)
	{
		gl.deleteShader(m_vs_id);

		m_vs_id = 0;
	}
}

/** Dummy init function */
void InvalidUseCasesForAllNotFuncsAndExclMarkOpTest::init()
{
	/* Left blank on purpose */
}

/** Retrieves a literal corresponding to the test iteration enum.
 *
 *  @param iteration Enum to retrieve the string for.
 *
 *  @return Requested object.
 **/
std::string InvalidUseCasesForAllNotFuncsAndExclMarkOpTest::getIterationName(_test_iteration iteration) const
{
	std::string result;

	switch (iteration)
	{
	case TEST_ITERATION_EXCL_MARK_OP_MUST_NOT_ACCEPT_BVEC2:
		result = "! operator must not accept bvec2 arg";
		break;
	case TEST_ITERATION_EXCL_MARK_OP_MUST_NOT_ACCEPT_BVEC3:
		result = "! operator must not accept bvec3 arg";
		break;
	case TEST_ITERATION_EXCL_MARK_OP_MUST_NOT_ACCEPT_BVEC4:
		result = "! operator must not accept bvec4 arg";
		break;
	case TEST_ITERATION_ALL_FUNC_MUST_NOT_ACCEPT_BOOL:
		result = "all() function must not accept bool arg";
		break;
	case TEST_ITERATION_NOT_FUNC_MUST_NOT_ACCEPT_BOOL:
		result = "not() function must not accept bool arg";
		break;
	default:
	{
		TCU_FAIL("Unrecognized test iteration type.");
	}
	} /* switch (iteration) */

	return result;
}

/** Retrieves a vertex shader body for the user-specified iteration enum.
 *
 *  @param iteration Iteration to retrieve the shader body for.
 *
 *  @return Requested string object.
 */
std::string InvalidUseCasesForAllNotFuncsAndExclMarkOpTest::getShaderBody(_test_iteration iteration) const
{
	std::string result;

	switch (iteration)
	{
	case TEST_ITERATION_EXCL_MARK_OP_MUST_NOT_ACCEPT_BVEC2:
	case TEST_ITERATION_EXCL_MARK_OP_MUST_NOT_ACCEPT_BVEC3:
	case TEST_ITERATION_EXCL_MARK_OP_MUST_NOT_ACCEPT_BVEC4:
	{
		/* From GL SL spec:
		 *
		 * The logical unary operator not (!). It operates only on a Boolean expression and results in a Boolean
		 * expression. To operate on a vector, use the built-in function not.
		 */
		std::stringstream result_sstream;
		std::string		  type_string;

		type_string = (iteration == TEST_ITERATION_EXCL_MARK_OP_MUST_NOT_ACCEPT_BVEC2) ?
						  "bvec2" :
						  (iteration == TEST_ITERATION_EXCL_MARK_OP_MUST_NOT_ACCEPT_BVEC3) ? "bvec3" : "bvec4";

		result_sstream << "#version 140\n"
						  "\n"
						  "void main()\n"
						  "{\n"
						  "    if (!"
					   << type_string << "(false))\n"
										 "    {\n"
										 "        gl_Position = vec4(1.0, 2.0, 3.0, 4.0);\n"
										 "    }\n"
										 "    else\n"
										 "    {\n"
										 "        gl_Position = vec4(2.0, 3.0, 4.0, 5.0);\n"
										 "    }\n"
										 "}\n";

		result = result_sstream.str();
		break;
	}

	case TEST_ITERATION_ALL_FUNC_MUST_NOT_ACCEPT_BOOL:
	case TEST_ITERATION_NOT_FUNC_MUST_NOT_ACCEPT_BOOL:
	{
		std::string		  op_string;
		std::stringstream result_sstream;

		/* From GLSL spec, all() and not() functions are defined as:
		 *
		 * bool all(bvec x)
		 * bvec not(bvec x)
		 *
		 * where bvec is bvec2, bvec3 or bvec4.
		 */
		op_string = (iteration == TEST_ITERATION_ALL_FUNC_MUST_NOT_ACCEPT_BOOL) ? "all" : "not";

		result_sstream << "#version 140\n"
						  "\n"
						  "void main()\n"
						  "{\n"
						  "    gl_Position = vec4("
					   << op_string << "(false, true) ? 1.0 : 2.0);\n"
									   "}\n";

		result = result_sstream.str();
		break;
	}

	default:
		TCU_FAIL("Unrecognized test iteration type");
	} /* switch (iteration) */

	return result;
}

/** Executes test iteration.
 *
 *  @return Returns STOP when test has finished executing, CONTINUE if more iterations are needed.
 */
tcu::TestNode::IterateResult InvalidUseCasesForAllNotFuncsAndExclMarkOpTest::iterate()
{
	const glw::Functions& gl	 = m_context.getRenderContext().getFunctions();
	bool				  result = true;

	/* Create a vertex shader object */
	m_vs_id = gl.createShader(GL_VERTEX_SHADER);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateShader() call failed.");

	/* Execute all test iterations.. */
	for (int current_iteration = static_cast<int>(TEST_ITERATION_FIRST);
		 current_iteration < static_cast<int>(TEST_ITERATION_COUNT); ++current_iteration)
	{
		const std::string body			 = getShaderBody(static_cast<_test_iteration>(current_iteration));
		const char*		  body_raw_ptr   = body.c_str();
		glw::GLint		  compile_status = GL_FALSE;

		/* Assign the source code to the SO */
		gl.shaderSource(m_vs_id, 1,				 /* count */
						&body_raw_ptr, DE_NULL); /* length */
		GLU_EXPECT_NO_ERROR(gl.getError(), "glShaderSource() call failed.");

		/* Try to compile the shader object. */
		gl.compileShader(m_vs_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glCompileShader() call failed.");

		gl.getShaderiv(m_vs_id, GL_COMPILE_STATUS, &compile_status);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGetShaderiv() call failed.");

		if (compile_status == GL_TRUE)
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "The following vertex shader, used for test iteration ["
							   << getIterationName(static_cast<_test_iteration>(current_iteration))
							   << "] "
								  "was compiled successfully, even though it is invalid. Body:"
								  "\n>>\n"
							   << body << "\n<<\n"
							   << tcu::TestLog::EndMessage;

			result = false;
		}
	} /* for (all test iterations) */

	m_testCtx.setTestResult(result ? QP_TEST_RESULT_PASS : QP_TEST_RESULT_FAIL, result ? "Pass" : "Fail");

	return STOP;
}

InvalidVSInputsTest::InvalidVSInputsTest(deqp::Context& context)
	: TestCase(context, "CommonBug_InvalidVSInputs",
			   "Verifies that invalid types, as well as incompatible qualifiers, are "
			   "not accepted for vertex shader input variable declarations")
	, m_vs_id(0)
{
	/* Left blank on purpose */
}

/** Deinitializes all GL objects created for the purpose of running the test,
 *  as well as any client-side buffers allocated at initialization time
 */
void InvalidVSInputsTest::deinit()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (m_vs_id != 0)
	{
		gl.deleteShader(m_vs_id);

		m_vs_id = 0;
	}
}

/** Dummy init function */
void InvalidVSInputsTest::init()
{
	/* Left blank on purpose */
}

/** Retrieves a literal corresponding to the test iteration enum.
 *
 *  @param iteration Enum to retrieve the string for.
 *
 *  @return Requested object.
 **/
std::string InvalidVSInputsTest::getIterationName(_test_iteration iteration) const
{
	std::string result;

	switch (iteration)
	{
	case TEST_ITERATION_INVALID_BOOL_INPUT:
		result = "Invalid bool input";
		break;
	case TEST_ITERATION_INVALID_BVEC2_INPUT:
		result = "Invalid bvec2 input";
		break;
	case TEST_ITERATION_INVALID_BVEC3_INPUT:
		result = "Invalid bvec3 input";
		break;
	case TEST_ITERATION_INVALID_BVEC4_INPUT:
		result = "Invalid bvec4 input";
		break;
	case TEST_ITERATION_INVALID_CENTROID_QUALIFIED_INPUT:
		result = "Invalid centroid-qualified input";
		break;
	case TEST_ITERATION_INVALID_PATCH_QUALIFIED_INPUT:
		result = "Invalid patch-qualified input";
		break;
	case TEST_ITERATION_INVALID_OPAQUE_TYPE_INPUT:
		result = "Invalid opaque type input";
		break;
	case TEST_ITERATION_INVALID_STRUCTURE_INPUT:
		result = "Invalid structure input";
		break;
	case TEST_ITERATION_INVALID_SAMPLE_QUALIFIED_INPUT:
		result = "Invalid sample-qualified input";
		break;

	default:
		TCU_FAIL("Unrecognized test iteration type.");
	} /* switch (iteration) */

	return result;
}

/** Retrieves a vertex shader body for the user-specified iteration enum.
 *
 *  @param iteration Iteration to retrieve the shader body for.
 *
 *  @return Requested string object.
 */
std::string InvalidVSInputsTest::getShaderBody(_test_iteration iteration) const
{
	std::string result;

	switch (iteration)
	{
	case TEST_ITERATION_INVALID_BOOL_INPUT:
	case TEST_ITERATION_INVALID_BVEC2_INPUT:
	case TEST_ITERATION_INVALID_BVEC3_INPUT:
	case TEST_ITERATION_INVALID_BVEC4_INPUT:
	{
		std::stringstream body_sstream;
		const char*		  type = (iteration == TEST_ITERATION_INVALID_BOOL_INPUT) ?
							   "bool" :
							   (iteration == TEST_ITERATION_INVALID_BVEC2_INPUT) ?
							   "bvec2" :
							   (iteration == TEST_ITERATION_INVALID_BVEC3_INPUT) ? "bvec3" : "bvec4";

		body_sstream << "#version 140\n"
						"\n"
						"in "
					 << type << " data;\n"
								"\n"
								"void main()\n"
								"{\n"
								"}\n";

		result = body_sstream.str();
		break;
	}

	case TEST_ITERATION_INVALID_OPAQUE_TYPE_INPUT:
	{
		result = "#version 140\n"
				 "\n"
				 "in sampler2D data;\n"
				 "\n"
				 "void main()\n"
				 "{\n"
				 "}\n";

		break;
	}

	case TEST_ITERATION_INVALID_STRUCTURE_INPUT:
	{
		result = "#version 140\n"
				 "\n"
				 "in struct\n"
				 "{\n"
				 "    vec4 test;\n"
				 "} data;\n"
				 "\n"
				 "void main()\n"
				 "{\n"
				 "}\n";

		break;
	}

	case TEST_ITERATION_INVALID_CENTROID_QUALIFIED_INPUT:
	case TEST_ITERATION_INVALID_PATCH_QUALIFIED_INPUT:
	case TEST_ITERATION_INVALID_SAMPLE_QUALIFIED_INPUT:
	{
		std::stringstream body_sstream;
		const char*		  qualifier = (iteration == TEST_ITERATION_INVALID_CENTROID_QUALIFIED_INPUT) ?
									"centroid" :
									(iteration == TEST_ITERATION_INVALID_PATCH_QUALIFIED_INPUT) ? "patch" : "sample";

		body_sstream << "#version 140\n"
						"\n"
					 << qualifier << " in vec4 data;\n"
									 "\n"
									 "void main()\n"
									 "{\n"
									 "}\n";

		result = body_sstream.str();
		break;
	}

	default:
		TCU_FAIL("Unrecognized test iteration type");
	} /* switch (iteration) */

	return result;
}

/** Executes test iteration.
 *
 *  @return Returns STOP when test has finished executing, CONTINUE if more iterations are needed.
 */
tcu::TestNode::IterateResult InvalidVSInputsTest::iterate()
{
	const glw::Functions& gl	 = m_context.getRenderContext().getFunctions();
	bool				  result = true;

	/* Create a vertex shader object */
	m_vs_id = gl.createShader(GL_VERTEX_SHADER);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateShader() call failed.");

	/* Execute all test iterations.. */
	for (int current_iteration = static_cast<int>(TEST_ITERATION_FIRST);
		 current_iteration < static_cast<int>(TEST_ITERATION_COUNT); current_iteration++)
	{
		const std::string body			 = getShaderBody(static_cast<_test_iteration>(current_iteration));
		const char*		  body_raw_ptr   = body.c_str();
		glw::GLint		  compile_status = GL_FALSE;

		/* Assign the source code to the SO */
		gl.shaderSource(m_vs_id, 1,				 /* count */
						&body_raw_ptr, DE_NULL); /* length */
		GLU_EXPECT_NO_ERROR(gl.getError(), "glShaderSource() call failed.");

		/* Try to compile the shader object. */
		gl.compileShader(m_vs_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glCompileShader() call failed.");

		gl.getShaderiv(m_vs_id, GL_COMPILE_STATUS, &compile_status);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGetShaderiv() call failed.");

		if (compile_status == GL_TRUE)
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "The following vertex shader, used for test iteration ["
							   << getIterationName(static_cast<_test_iteration>(current_iteration))
							   << "] "
								  "was compiled successfully, even though it is invalid. Body:"
								  "\n>>\n"
							   << body << "\n<<\n"
							   << tcu::TestLog::EndMessage;

			result = false;
		}
	} /* for (all test iterations) */

	m_testCtx.setTestResult(result ? QP_TEST_RESULT_PASS : QP_TEST_RESULT_FAIL, result ? "Pass" : "Fail");

	return STOP;
}

/** Constructor.
 *
 *  @param context     Rendering context
 *  @param name        Test name
 *  @param description Test description
 */
ParenthesisInLayoutQualifierIntegerValuesTest::ParenthesisInLayoutQualifierIntegerValuesTest(deqp::Context& context)
	: TestCase(context, "CommonBug_ParenthesisInLayoutQualifierIntegerValue",
			   "Verifies parenthesis are not accepted in compute shaders, prior to GL4.4, "
			   "unless GL_ARB_enhanced_layouts is supported.")
	, m_cs_id(0)
	, m_po_id(0)
{
	/* Left blank on purpose */
}

/** Deinitializes all GL objects created for the purpose of running the test,
 *  as well as any client-side buffers allocated at initialization time
 */
void ParenthesisInLayoutQualifierIntegerValuesTest::deinit()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (m_cs_id != 0)
	{
		gl.deleteShader(m_cs_id);

		m_cs_id = 0;
	}

	if (m_po_id != 0)
	{
		gl.deleteProgram(m_po_id);

		m_po_id = 0;
	}
}

/** Dummy init function */
void ParenthesisInLayoutQualifierIntegerValuesTest::init()
{
	/* Left blank on purpose */
}

/** Executes test iteration.
 *
 *  @return Returns STOP when test has finished executing, CONTINUE if more iterations are needed.
 */
tcu::TestNode::IterateResult ParenthesisInLayoutQualifierIntegerValuesTest::iterate()
{
	/* Only execute the test on implementations supporting GL_ARB_compute_shader */
	const glu::ContextType context_type = m_context.getRenderContext().getType();
	const glw::Functions&  gl			= m_context.getRenderContext().getFunctions();
	bool				   result		= true;

	glw::GLint link_status		= GL_TRUE;
	glw::GLint compile_status   = GL_TRUE;
	bool	   expected_outcome = glu::contextSupports(context_type, glu::ApiType::core(4, 4));

	/* Prepare a compute shader program */
	static const char* cs_body_core = "\n"
									  "layout(local_size_x = (4) ) in;\n"
									  "\n"
									  "void main()\n"
									  "{\n"
									  "}\n";

	const char* cs_body_parts[] = { (!glu::contextSupports(context_type, glu::ApiType::core(4, 3))) ?
										"#version 420 core\n"
										"#extension GL_ARB_compute_shader : enable\n" :
										(!glu::contextSupports(context_type, glu::ApiType::core(4, 4))) ?
										"#version 430 core\n" :
										"#version 440 core\n",
									cs_body_core };
	const unsigned int n_cs_body_parts =
		static_cast<const unsigned int>(sizeof(cs_body_parts) / sizeof(cs_body_parts[0]));

	if (!m_context.getContextInfo().isExtensionSupported("GL_ARB_compute_shader"))
	{
		goto end;
	}

	m_cs_id = gl.createShader(GL_COMPUTE_SHADER);
	m_po_id = gl.createProgram();

	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateShader() and/or glCraeteProgram() call(s) failed.");

	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateProgram() call failed.");

	gl.shaderSource(m_cs_id, n_cs_body_parts, cs_body_parts, DE_NULL); /* length */
	GLU_EXPECT_NO_ERROR(gl.getError(), "glShaderSource() call failed.");

	/* Try to compile the shader object.
	 *
	 * For GL contexts BEFORE 4.40, the test passes if either
	 * the compilation or the linking process fails.
	 *
	 * For GL contexts OF VERSION 4.40 or newer, the test passes
	 * if both the compilation and the linking process succeed.
	 *
	 * If GL_ARB_enhanced_layouts is supported, the latter holds for <= GL4.4 contexts.
	 **/
	if (m_context.getContextInfo().isExtensionSupported("GL_ARB_enhanced_layouts"))
	{
		expected_outcome = true;
	}

	gl.compileShader(m_cs_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCompileShader() call failed.");

	gl.getShaderiv(m_cs_id, GL_COMPILE_STATUS, &compile_status);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetShaderiv() call failed.");

	if (compile_status == GL_FALSE && !expected_outcome)
	{
		goto end;
	}

	gl.attachShader(m_po_id, m_cs_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glAttachShader() call failed.");

	gl.linkProgram(m_po_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glLinkProgram() call failed.");

	gl.getProgramiv(m_po_id, GL_LINK_STATUS, &link_status);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetProgramiv() call failed.");

	if (link_status == GL_FALSE && !expected_outcome)
	{
		goto end;
	}

	if (expected_outcome && (compile_status == GL_FALSE || link_status == GL_FALSE))
	{
		m_testCtx.getLog() << tcu::TestLog::Message
						   << "A compute program was expected to link successfully, but either the "
							  "compilation and/or linking process has/have failed"
						   << tcu::TestLog::EndMessage;

		result = false;
	}
	else if (!expected_outcome && (compile_status == GL_TRUE && link_status == GL_TRUE))
	{
		m_testCtx.getLog() << tcu::TestLog::Message
						   << "A compute program was expected not to compile and link, but both processes "
							  "have been executed successfully."
						   << tcu::TestLog::EndMessage;

		result = false;
	}

end:
	m_testCtx.setTestResult(result ? QP_TEST_RESULT_PASS : QP_TEST_RESULT_FAIL, result ? "Pass" : "Fail");

	return STOP;
}

/** Constructor.
 *
 *  @param context     Rendering context
 *  @param name        Test name
 *  @param description Test description
 */
PerVertexValidationTest::PerVertexValidationTest(deqp::Context& context)
	: TestCase(context, "CommonBug_PerVertexValidation",
			   "Conformance test which verifies that various requirements regarding gl_PerVertex block re-declarations,"
			   " as described by the spec, are followed by the implementation")
	, m_fs_id(0)
	, m_fs_po_id(0)
	, m_gs_id(0)
	, m_gs_po_id(0)
	, m_pipeline_id(0)
	, m_tc_id(0)
	, m_tc_po_id(0)
	, m_te_id(0)
	, m_te_po_id(0)
	, m_vs_id(0)
	, m_vs_po_id(0)
{
	/* Left blank on purpose */
}

/** Deinitializes all GL objects created for the purpose of running the test,
 *  as well as any client-side buffers allocated at initialization time
 */
void PerVertexValidationTest::deinit()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Release the pipeline object */
	if (m_pipeline_id != 0)
	{
		gl.deleteProgramPipelines(1, &m_pipeline_id);

		m_pipeline_id = 0;
	}

	/* Release all dangling shader and shader program objects */
	destroyPOsAndSOs();
}

/** Releases any allocated program and shader objects. */
void PerVertexValidationTest::destroyPOsAndSOs()
{
	const glw::Functions& gl		   = m_context.getRenderContext().getFunctions();
	glw::GLuint*		  po_id_ptrs[] = { &m_fs_po_id, &m_gs_po_id, &m_tc_po_id, &m_te_po_id, &m_vs_po_id };
	glw::GLuint*		  so_id_ptrs[] = { &m_fs_id, &m_gs_id, &m_tc_id, &m_te_id, &m_vs_id };
	const unsigned int	n_po_id_ptrs = static_cast<const unsigned int>(sizeof(po_id_ptrs) / sizeof(po_id_ptrs[0]));
	const unsigned int	n_so_id_ptrs = static_cast<const unsigned int>(sizeof(so_id_ptrs) / sizeof(so_id_ptrs[0]));

	for (unsigned int n_po_id = 0; n_po_id < n_po_id_ptrs; ++n_po_id)
	{
		glw::GLuint* po_id_ptr = po_id_ptrs[n_po_id];

		if (*po_id_ptr != 0)
		{
			gl.deleteProgram(*po_id_ptr);

			*po_id_ptr = 0;
		}
	} /* for (all shader program object ids) */

	for (unsigned int n_so_id = 0; n_so_id < n_so_id_ptrs; ++n_so_id)
	{
		glw::GLuint* so_id_ptr = so_id_ptrs[n_so_id];

		if (*so_id_ptr != 0)
		{
			gl.deleteShader(*so_id_ptr);

			*so_id_ptr = 0;
		}
	} /* for (all shader object ids) */
}

/** Tells whether the program pipeline validation process should fail for specified test iteration.
 *
 *  @return VALIDATION_RESULT_TRUE if the pipeline validation process should be positive,
 *          VALIDATION_RESULT_FALSE if the validation should be negative
 *          VALIDATION_RESULT_UNDEFINED when the validation result is not defined */
PerVertexValidationTest::_validation PerVertexValidationTest::getProgramPipelineValidationExpectedResult(void) const
{
	/** All "undeclared in.." and "undeclared out.." test shaders are expected not to link successfully
	 *  when used as separate programs - which leaves pipeline in undefined state.
	 *  All "declaration mismatch" shaders should link, but the results are undefined.
	 *
	 *  Currently the test does not exercise any defined case
	 */
	return VALIDATION_RESULT_UNDEFINED;
}

/** Returns a literal corresponding to the shader stage enum.
 *
 *  @param shader_stage Enum to return the string for.
 *
 *  @return As per description.
 */
std::string PerVertexValidationTest::getShaderStageName(_shader_stage shader_stage) const
{
	std::string result = "?!";

	switch (shader_stage)
	{
	case SHADER_STAGE_FRAGMENT:
		result = "fragment shader";
		break;
	case SHADER_STAGE_GEOMETRY:
		result = "geometry shader";
		break;
	case SHADER_STAGE_TESSELLATION_CONTROL:
		result = "tessellation control shader";
		break;
	case SHADER_STAGE_TESSELLATION_EVALUATION:
		result = "tessellation evaluation shader";
		break;
	case SHADER_STAGE_VERTEX:
		result = "vertex shader";
		break;
	}

	return result;
}

/** Returns a literal corresponding to the test iteration enum.
 *
 *  @param iteration Enum to return the string for.
 *
 *  @return As per description.
 **/
std::string PerVertexValidationTest::getTestIterationName(_test_iteration iteration) const
{
	std::string result = "?!";

	switch (iteration)
	{
	case TEST_ITERATION_UNDECLARED_IN_PERVERTEX_GS_GL_CLIPDISTANCE_USAGE:
		result = "Input gl_ClipDistance usage in Geometry Shader without gl_PerVertex block redeclaration";
		break;
	case TEST_ITERATION_UNDECLARED_IN_PERVERTEX_GS_GL_CULLDISTANCE_USAGE:
		result = "Input gl_CullDistance usage in Geometry Shader without gl_PerVertex block redeclaration";
		break;
	case TEST_ITERATION_UNDECLARED_IN_PERVERTEX_GS_GL_POINTSIZE_USAGE:
		result = "Input gl_PointSize usage in a separable Geometry Shader without gl_PerVertex block redeclaration";
		break;
	case TEST_ITERATION_UNDECLARED_IN_PERVERTEX_GS_GL_POSITION_USAGE:
		result = "Input gl_Position usage in a separable Geometry Shader without gl_PerVertex block redeclaration";
		break;
	case TEST_ITERATION_UNDECLARED_IN_PERVERTEX_TC_GL_CLIPDISTANCE_USAGE:
		result = "Input gl_ClipDistance usage in a separable Tessellation Control Shader without gl_PerVertex block "
				 "redeclaration";
		break;
	case TEST_ITERATION_UNDECLARED_IN_PERVERTEX_TC_GL_CULLDISTANCE_USAGE:
		result = "Input gl_CullDistance usage in a separable Tessellation Control Shader without gl_PerVertex block "
				 "redeclaration";
		break;
	case TEST_ITERATION_UNDECLARED_IN_PERVERTEX_TC_GL_POINTSIZE_USAGE:
		result = "Input gl_PointSize usage in a separable Tessellation Control Shader without gl_PerVertex block "
				 "redeclaration";
		break;
	case TEST_ITERATION_UNDECLARED_IN_PERVERTEX_TC_GL_POSITION_USAGE:
		result = "Input gl_Position usage in a separable Tessellation Control Shader without gl_PerVertex block "
				 "redeclaration";
		break;
	case TEST_ITERATION_UNDECLARED_IN_PERVERTEX_TE_GL_CLIPDISTANCE_USAGE:
		result = "Input gl_ClipDistance usage in a separable Tessellation Evaluation Shader without gl_PerVertex block "
				 "redeclaration";
		break;
	case TEST_ITERATION_UNDECLARED_IN_PERVERTEX_TE_GL_CULLDISTANCE_USAGE:
		result = "Input gl_CullDistance usage in a separable Tessellation Evaluation Shader without gl_PerVertex block "
				 "redeclaration";
		break;
	case TEST_ITERATION_UNDECLARED_IN_PERVERTEX_TE_GL_POINTSIZE_USAGE:
		result = "Input gl_PointSize usage in a separable Tessellation Evaluation Shader without gl_PerVertex block "
				 "redeclaration";
		break;
	case TEST_ITERATION_UNDECLARED_IN_PERVERTEX_TE_GL_POSITION_USAGE:
		result = "Input gl_Position usage in a separable Tessellation Evaluation Shader without gl_PerVertex block "
				 "redeclaration";
		break;
	case TEST_ITERATION_UNDECLARED_OUT_PERVERTEX_GS_GL_CLIPDISTANCE_USAGE:
		result = "Output gl_ClipDistance usage in Geometry Shader without gl_PerVertex block redeclaration";
		break;
	case TEST_ITERATION_UNDECLARED_OUT_PERVERTEX_GS_GL_CULLDISTANCE_USAGE:
		result = "Output gl_CullDistance usage in Geometry Shader without gl_PerVertex block redeclaration";
		break;
	case TEST_ITERATION_UNDECLARED_OUT_PERVERTEX_GS_GL_POINTSIZE_USAGE:
		result = "Output gl_PointSize usage in a separable Geometry Shader without gl_PerVertex block redeclaration";
		break;
	case TEST_ITERATION_UNDECLARED_OUT_PERVERTEX_GS_GL_POSITION_USAGE:
		result = "Output gl_Position usage in a separable Geometry Shader without gl_PerVertex block redeclaration";
		break;
	case TEST_ITERATION_UNDECLARED_OUT_PERVERTEX_TC_GL_CLIPDISTANCE_USAGE:
		result = "Output gl_ClipDistance usage in a separable Tessellation Control Shader without gl_PerVertex block "
				 "redeclaration";
		break;
	case TEST_ITERATION_UNDECLARED_OUT_PERVERTEX_TC_GL_CULLDISTANCE_USAGE:
		result = "Output gl_CullDistance usage in a separable Tessellation Control Shader without gl_PerVertex block "
				 "redeclaration";
		break;
	case TEST_ITERATION_UNDECLARED_OUT_PERVERTEX_TC_GL_POINTSIZE_USAGE:
		result = "Output gl_PointSize usage in a separable Tessellation Control Shader without gl_PerVertex block "
				 "redeclaration";
		break;
	case TEST_ITERATION_UNDECLARED_OUT_PERVERTEX_TC_GL_POSITION_USAGE:
		result = "Output gl_Position usage in a separable Tessellation Control Shader without gl_PerVertex block "
				 "redeclaration";
		break;
	case TEST_ITERATION_UNDECLARED_OUT_PERVERTEX_TE_GL_CLIPDISTANCE_USAGE:
		result = "Output gl_ClipDistance usage in a separable Tessellation Evaluation Shader without gl_PerVertex "
				 "block redeclaration";
		break;
	case TEST_ITERATION_UNDECLARED_OUT_PERVERTEX_TE_GL_CULLDISTANCE_USAGE:
		result = "Output gl_CullDistance usage in a separable Tessellation Evaluation Shader without gl_PerVertex "
				 "block redeclaration";
		break;
	case TEST_ITERATION_UNDECLARED_OUT_PERVERTEX_TE_GL_POINTSIZE_USAGE:
		result = "Output gl_PointSize usage in a separable Tessellation Evaluation Shader without gl_PerVertex block "
				 "redeclaration";
		break;
	case TEST_ITERATION_UNDECLARED_OUT_PERVERTEX_TE_GL_POSITION_USAGE:
		result = "Output gl_Position usage in a separable Tessellation Evaluation Shader without gl_PerVertex block "
				 "redeclaration";
		break;
	case TEST_ITERATION_UNDECLARED_OUT_PERVERTEX_VS_GL_CLIPDISTANCE_USAGE:
		result = "Output gl_ClipDistance usage in a separable Vertex Shader without gl_PerVertex block redeclaration";
		break;
	case TEST_ITERATION_UNDECLARED_OUT_PERVERTEX_VS_GL_CULLDISTANCE_USAGE:
		result = "Output gl_CullDistance usage in a separable Vertex Shader without gl_PerVertex block redeclaration";
		break;
	case TEST_ITERATION_UNDECLARED_OUT_PERVERTEX_VS_GL_POINTSIZE_USAGE:
		result = "Output gl_PointSize usage in a separable Vertex Shader without gl_PerVertex block redeclaration";
		break;
	case TEST_ITERATION_UNDECLARED_OUT_PERVERTEX_VS_GL_POSITION_USAGE:
		result = "Output gl_Position usage in a separable Vertex Shader without gl_PerVertex block redeclaration";
		break;

	case TEST_ITERATION_PERVERTEX_DECLARATION_MISMATCH_GS_VS:
		result = "Geometry and Vertex Shaders use different gl_PerVertex block redeclaration";
		break;
	case TEST_ITERATION_PERVERTEX_DECLARATION_MISMATCH_GS_TC_TE_VS:
		result = "Geometry, Tessellation Control, Tessellation Evaluation and Vertex Shaders use a different "
				 "gl_PerVertex block redeclaration";
		break;
	case TEST_ITERATION_PERVERTEX_DECLARATION_MISMATCH_TC_TE_VS:
		result = "Tesselation Control, Tessellation Evaluation and Vertex Shaders use a different gl_PerVertex block "
				 "redeclaration";
		break;

	case TEST_ITERATION_PERVERTEX_BLOCK_UNDEFINED:
		result = "No gl_PerVertex block defined in shader programs for all shader stages supported by the running "
				 "context";
		break;
	default:
		result = "Unknown";
		break;
	}

	return result;
}

/** Returns shader bodies, minimum context type and a bitfield describing shader stages used for the
 *  user-specified test iteration enum.
 *
 *  @param context_type               Running context's type.
 *  @param iteration                  Test iteration enum to return the properties for.
 *  @param out_min_context_type_ptr   Deref will be set to the minimum context type supported by the
 *                                    specified test iteration.
 *  @param out_used_shader_stages_ptr Deref will be set to a combination of _shader_stage enum values,
 *                                    describing which shader stages are used by the test iteration.
 *  @param out_gs_body_ptr            Deref will be updated with the geometry shader body, if used by the
 *                                    test iteration.
 *  @param out_tc_body_ptr            Deref will be updated with the tessellation control shader body, if
 *                                    used by the test iteration.
 *  @param out_te_body_ptr            Deref will be updated with the tessellation evaluation shader body, if
 *                                    used by the test iteration.
 *  @param out_vs_body_ptr            Deref will be updated with the vertex shader body, if used by the
 *                                    test iteration.
 *
 */
void PerVertexValidationTest::getTestIterationProperties(glu::ContextType context_type, _test_iteration iteration,
														 glu::ContextType* out_min_context_type_ptr,
														 _shader_stage*	out_used_shader_stages_ptr,
														 std::string* out_gs_body_ptr, std::string* out_tc_body_ptr,
														 std::string* out_te_body_ptr,
														 std::string* out_vs_body_ptr) const
{
	const bool include_cull_distance = glu::contextSupports(context_type, glu::ApiType::core(4, 5));

	switch (iteration)
	{
	case TEST_ITERATION_UNDECLARED_IN_PERVERTEX_GS_GL_CLIPDISTANCE_USAGE:
	case TEST_ITERATION_UNDECLARED_IN_PERVERTEX_GS_GL_CULLDISTANCE_USAGE:
	case TEST_ITERATION_UNDECLARED_IN_PERVERTEX_GS_GL_POINTSIZE_USAGE:
	case TEST_ITERATION_UNDECLARED_IN_PERVERTEX_GS_GL_POSITION_USAGE:
	case TEST_ITERATION_UNDECLARED_OUT_PERVERTEX_GS_GL_CLIPDISTANCE_USAGE:
	case TEST_ITERATION_UNDECLARED_OUT_PERVERTEX_GS_GL_CULLDISTANCE_USAGE:
	case TEST_ITERATION_UNDECLARED_OUT_PERVERTEX_GS_GL_POINTSIZE_USAGE:
	case TEST_ITERATION_UNDECLARED_OUT_PERVERTEX_GS_GL_POSITION_USAGE:
	{
		const bool is_cull_distance_iteration =
			(iteration == TEST_ITERATION_UNDECLARED_IN_PERVERTEX_GS_GL_CULLDISTANCE_USAGE ||
			 iteration == TEST_ITERATION_UNDECLARED_OUT_PERVERTEX_GS_GL_CULLDISTANCE_USAGE);
		std::stringstream gs_body_sstream;

		*out_min_context_type_ptr = (is_cull_distance_iteration) ? glu::ContextType(4, 5, glu::PROFILE_CORE) :
																   glu::ContextType(4, 1, glu::PROFILE_CORE);
		*out_used_shader_stages_ptr = (_shader_stage)(SHADER_STAGE_GEOMETRY | SHADER_STAGE_VERTEX);

		/* Form the geometry shader body */
		gs_body_sstream << ((!is_cull_distance_iteration) ? "#version 410\n" : "version 450\n")
						<< "\n"
						   "layout(points)                   in;\n"
						   "layout(points, max_vertices = 1) out;\n"
						   "\n"
						   "in gl_PerVertex\n"
						   "{\n";

		gs_body_sstream << ((iteration != TEST_ITERATION_UNDECLARED_IN_PERVERTEX_GS_GL_CLIPDISTANCE_USAGE &&
							 iteration != TEST_ITERATION_UNDECLARED_OUT_PERVERTEX_GS_GL_CLIPDISTANCE_USAGE) ?
								"float gl_ClipDistance[];\n" :
								"");
		gs_body_sstream << ((iteration != TEST_ITERATION_UNDECLARED_IN_PERVERTEX_GS_GL_POINTSIZE_USAGE &&
							 iteration != TEST_ITERATION_UNDECLARED_OUT_PERVERTEX_GS_GL_POINTSIZE_USAGE) ?
								"float gl_PointSize;\n" :
								"");
		gs_body_sstream << ((iteration != TEST_ITERATION_UNDECLARED_IN_PERVERTEX_GS_GL_POSITION_USAGE &&
							 iteration != TEST_ITERATION_UNDECLARED_OUT_PERVERTEX_GS_GL_POSITION_USAGE) ?
								"vec4  gl_Position;\n" :
								"");

		if (include_cull_distance)
		{
			gs_body_sstream << "float gl_CullDistance[];\n";
		}

		gs_body_sstream << "} gl_in[];\n"
						   "\n"
						   "out gl_PerVertex\n"
						   "{\n";

		gs_body_sstream << ((iteration != TEST_ITERATION_UNDECLARED_IN_PERVERTEX_GS_GL_CLIPDISTANCE_USAGE &&
							 iteration != TEST_ITERATION_UNDECLARED_OUT_PERVERTEX_GS_GL_CLIPDISTANCE_USAGE) ?
								"float gl_ClipDistance[];\n" :
								"");
		gs_body_sstream << ((iteration != TEST_ITERATION_UNDECLARED_IN_PERVERTEX_GS_GL_POINTSIZE_USAGE &&
							 iteration != TEST_ITERATION_UNDECLARED_OUT_PERVERTEX_GS_GL_POINTSIZE_USAGE) ?
								"float gl_PointSize;\n" :
								"");
		gs_body_sstream << ((iteration != TEST_ITERATION_UNDECLARED_IN_PERVERTEX_GS_GL_POSITION_USAGE &&
							 iteration != TEST_ITERATION_UNDECLARED_OUT_PERVERTEX_GS_GL_POSITION_USAGE) ?
								"vec4  gl_Position;\n" :
								"");

		if (include_cull_distance)
		{
			gs_body_sstream << "float gl_CullDistance[];\n";
		}

		gs_body_sstream << "};\n"
						   "\n"
						   "void main()\n"
						   "{\n";

		switch (iteration)
		{
		case TEST_ITERATION_UNDECLARED_IN_PERVERTEX_GS_GL_CLIPDISTANCE_USAGE:
			gs_body_sstream << "gl_Position = vec4(gl_in[0].gl_ClipDistance[0]);\n";
			break;
		case TEST_ITERATION_UNDECLARED_IN_PERVERTEX_GS_GL_CULLDISTANCE_USAGE:
			gs_body_sstream << "gl_Position = vec4(gl_in[0].gl_CullDistance[0]);\n";
			break;
		case TEST_ITERATION_UNDECLARED_IN_PERVERTEX_GS_GL_POINTSIZE_USAGE:
			gs_body_sstream << "gl_Position = vec4(gl_in[0].gl_PointSize);\n";
			break;
		case TEST_ITERATION_UNDECLARED_IN_PERVERTEX_GS_GL_POSITION_USAGE:
			gs_body_sstream << "gl_Position = gl_in[0].gl_Position;\n";
			break;
		case TEST_ITERATION_UNDECLARED_OUT_PERVERTEX_GS_GL_CLIPDISTANCE_USAGE:
			gs_body_sstream << "gl_ClipDistance[0] = gl_in[0].gl_Position.x;\n";
			break;
		case TEST_ITERATION_UNDECLARED_OUT_PERVERTEX_GS_GL_CULLDISTANCE_USAGE:
			gs_body_sstream << "gl_CullDistance[0] = gl_in[0].gl_Position.x;\n";
			break;
		case TEST_ITERATION_UNDECLARED_OUT_PERVERTEX_GS_GL_POINTSIZE_USAGE:
			gs_body_sstream << "gl_PointSize = gl_in[0].gl_Position.x;\n";
			break;
		case TEST_ITERATION_UNDECLARED_OUT_PERVERTEX_GS_GL_POSITION_USAGE:
			gs_body_sstream << "gl_Position = vec4(gl_in[0].gl_PointSize);\n";
			break;
		default:
			gs_body_sstream << "\n";
			break;
		} /* switch (iteration) */

		gs_body_sstream << "    EmitVertex();\n"
						   "}\n";

		/* Store geometry & vertex shader bodies */
		*out_gs_body_ptr = gs_body_sstream.str();
		*out_vs_body_ptr = getVertexShaderBody(context_type, iteration);

		break;
	}

	case TEST_ITERATION_UNDECLARED_IN_PERVERTEX_TC_GL_CLIPDISTANCE_USAGE:
	case TEST_ITERATION_UNDECLARED_IN_PERVERTEX_TC_GL_CULLDISTANCE_USAGE:
	case TEST_ITERATION_UNDECLARED_IN_PERVERTEX_TC_GL_POINTSIZE_USAGE:
	case TEST_ITERATION_UNDECLARED_IN_PERVERTEX_TC_GL_POSITION_USAGE:
	case TEST_ITERATION_UNDECLARED_IN_PERVERTEX_TE_GL_CLIPDISTANCE_USAGE:
	case TEST_ITERATION_UNDECLARED_IN_PERVERTEX_TE_GL_CULLDISTANCE_USAGE:
	case TEST_ITERATION_UNDECLARED_IN_PERVERTEX_TE_GL_POINTSIZE_USAGE:
	case TEST_ITERATION_UNDECLARED_IN_PERVERTEX_TE_GL_POSITION_USAGE:
	case TEST_ITERATION_UNDECLARED_OUT_PERVERTEX_TC_GL_CLIPDISTANCE_USAGE:
	case TEST_ITERATION_UNDECLARED_OUT_PERVERTEX_TC_GL_CULLDISTANCE_USAGE:
	case TEST_ITERATION_UNDECLARED_OUT_PERVERTEX_TC_GL_POINTSIZE_USAGE:
	case TEST_ITERATION_UNDECLARED_OUT_PERVERTEX_TC_GL_POSITION_USAGE:
	case TEST_ITERATION_UNDECLARED_OUT_PERVERTEX_TE_GL_CLIPDISTANCE_USAGE:
	case TEST_ITERATION_UNDECLARED_OUT_PERVERTEX_TE_GL_CULLDISTANCE_USAGE:
	case TEST_ITERATION_UNDECLARED_OUT_PERVERTEX_TE_GL_POINTSIZE_USAGE:
	case TEST_ITERATION_UNDECLARED_OUT_PERVERTEX_TE_GL_POSITION_USAGE:
	{
		std::stringstream tc_sstream;
		std::stringstream te_sstream;

		const bool is_clip_distance_iteration =
			(iteration == TEST_ITERATION_UNDECLARED_IN_PERVERTEX_TC_GL_CLIPDISTANCE_USAGE ||
			 iteration == TEST_ITERATION_UNDECLARED_IN_PERVERTEX_TE_GL_CLIPDISTANCE_USAGE ||
			 iteration == TEST_ITERATION_UNDECLARED_OUT_PERVERTEX_TC_GL_CLIPDISTANCE_USAGE ||
			 iteration == TEST_ITERATION_UNDECLARED_OUT_PERVERTEX_TE_GL_CLIPDISTANCE_USAGE);
		const bool is_cull_distance_iteration =
			(iteration == TEST_ITERATION_UNDECLARED_IN_PERVERTEX_TC_GL_CULLDISTANCE_USAGE ||
			 iteration == TEST_ITERATION_UNDECLARED_IN_PERVERTEX_TE_GL_CULLDISTANCE_USAGE ||
			 iteration == TEST_ITERATION_UNDECLARED_OUT_PERVERTEX_TC_GL_CULLDISTANCE_USAGE ||
			 iteration == TEST_ITERATION_UNDECLARED_OUT_PERVERTEX_TE_GL_CULLDISTANCE_USAGE);
		const bool is_pointsize_iteration =
			(iteration == TEST_ITERATION_UNDECLARED_IN_PERVERTEX_TC_GL_POINTSIZE_USAGE ||
			 iteration == TEST_ITERATION_UNDECLARED_IN_PERVERTEX_TE_GL_POINTSIZE_USAGE ||
			 iteration == TEST_ITERATION_UNDECLARED_OUT_PERVERTEX_TC_GL_POINTSIZE_USAGE ||
			 iteration == TEST_ITERATION_UNDECLARED_OUT_PERVERTEX_TE_GL_POINTSIZE_USAGE);
		const bool is_position_iteration = (iteration == TEST_ITERATION_UNDECLARED_IN_PERVERTEX_TC_GL_POSITION_USAGE ||
											iteration == TEST_ITERATION_UNDECLARED_IN_PERVERTEX_TE_GL_POSITION_USAGE ||
											iteration == TEST_ITERATION_UNDECLARED_OUT_PERVERTEX_TC_GL_POSITION_USAGE ||
											iteration == TEST_ITERATION_UNDECLARED_OUT_PERVERTEX_TE_GL_POSITION_USAGE);

		*out_min_context_type_ptr = (is_cull_distance_iteration) ? glu::ContextType(4, 5, glu::PROFILE_CORE) :
																   glu::ContextType(4, 0, glu::PROFILE_CORE);
		*out_used_shader_stages_ptr = (_shader_stage)(SHADER_STAGE_TESSELLATION_CONTROL |
													  SHADER_STAGE_TESSELLATION_EVALUATION | SHADER_STAGE_VERTEX);

		/* Form tessellation control & tessellation evaluation shader bodies */
		for (unsigned int n_iteration = 0; n_iteration < 2; ++n_iteration)
		{
			const bool		   is_tc_stage		   = (n_iteration == 0);
			std::stringstream* current_sstream_ptr = (is_tc_stage) ? &tc_sstream : &te_sstream;

			*current_sstream_ptr << ((is_cull_distance_iteration) ? "#version 450 core\n" : "#version 410\n") << "\n";

			if (is_tc_stage)
			{
				*current_sstream_ptr << "layout (vertices = 4) out;\n";
			}
			else
			{
				*current_sstream_ptr << "layout (isolines) in;\n";
			}

			*current_sstream_ptr << "\n"
									"in gl_PerVertex\n"
									"{\n";

			if (is_position_iteration)
			{
				*current_sstream_ptr << "vec4 gl_Position;\n";
			}

			if (!is_pointsize_iteration)
			{
				*current_sstream_ptr << "float gl_PointSize\n";
			}

			if (!is_clip_distance_iteration)
			{
				*current_sstream_ptr << "float gl_ClipDistance[];\n";
			}

			if (!is_cull_distance_iteration && include_cull_distance)
			{
				*current_sstream_ptr << "float gl_CullDistance[];\n";
			}

			*current_sstream_ptr << "} gl_in[gl_MaxPatchVertices];\n"
									"\n"
									"out gl_PerVertex\n"
									"{\n";

			if (!is_position_iteration)
			{
				*current_sstream_ptr << "vec4 gl_Position;\n";
			}

			if (!is_pointsize_iteration)
			{
				*current_sstream_ptr << "float gl_PointSize\n";
			}

			if (!is_clip_distance_iteration)
			{
				*current_sstream_ptr << "float gl_ClipDistance[];\n";
			}

			if (!is_cull_distance_iteration && include_cull_distance)
			{
				*current_sstream_ptr << "float gl_CullDistance[];\n";
			}

			if (is_tc_stage)
			{
				*current_sstream_ptr << "} gl_out[];\n";
			}
			else
			{
				*current_sstream_ptr << "};\n";
			}

			*current_sstream_ptr << "\n"
									"void main()\n"
									"{\n";

			if (is_tc_stage)
			{
				*current_sstream_ptr << "gl_out[gl_InvocationID].";
			}

			if (iteration == TEST_ITERATION_UNDECLARED_IN_PERVERTEX_TC_GL_CLIPDISTANCE_USAGE ||
				iteration == TEST_ITERATION_UNDECLARED_IN_PERVERTEX_TE_GL_CLIPDISTANCE_USAGE)
			{
				*current_sstream_ptr << "gl_Position        = vec4(gl_in[0].gl_ClipDistance[0]);\n";
			}
			else if (iteration == TEST_ITERATION_UNDECLARED_IN_PERVERTEX_TC_GL_CULLDISTANCE_USAGE ||
					 iteration == TEST_ITERATION_UNDECLARED_IN_PERVERTEX_TE_GL_CULLDISTANCE_USAGE)
			{
				*current_sstream_ptr << "gl_Position        = vec4(gl_in[0].gl_CullDistance[0]);\n";
			}
			else if (iteration == TEST_ITERATION_UNDECLARED_IN_PERVERTEX_TC_GL_POINTSIZE_USAGE ||
					 iteration == TEST_ITERATION_UNDECLARED_IN_PERVERTEX_TE_GL_POINTSIZE_USAGE)
			{
				*current_sstream_ptr << "gl_Position        = vec4(gl_in[0].gl_PointSize);\n";
			}
			else if (iteration == TEST_ITERATION_UNDECLARED_IN_PERVERTEX_TC_GL_POSITION_USAGE ||
					 iteration == TEST_ITERATION_UNDECLARED_IN_PERVERTEX_TE_GL_POSITION_USAGE)
			{
				*current_sstream_ptr << "gl_Position        = gl_in[0].gl_Position;\n";
			}
			else if (iteration == TEST_ITERATION_UNDECLARED_OUT_PERVERTEX_TC_GL_CLIPDISTANCE_USAGE ||
					 iteration == TEST_ITERATION_UNDECLARED_OUT_PERVERTEX_TE_GL_CLIPDISTANCE_USAGE)
			{
				*current_sstream_ptr << "gl_ClipDistance[0] = gl_in[0].gl_Position.x;\n";
			}
			else if (iteration == TEST_ITERATION_UNDECLARED_OUT_PERVERTEX_TC_GL_CULLDISTANCE_USAGE ||
					 iteration == TEST_ITERATION_UNDECLARED_OUT_PERVERTEX_TE_GL_CULLDISTANCE_USAGE)
			{
				*current_sstream_ptr << "gl_CullDistance[0] = gl_in[0].gl_Position.x;\n";
			}
			else if (iteration == TEST_ITERATION_UNDECLARED_OUT_PERVERTEX_TC_GL_POINTSIZE_USAGE ||
					 iteration == TEST_ITERATION_UNDECLARED_OUT_PERVERTEX_TE_GL_POINTSIZE_USAGE)
			{
				*current_sstream_ptr << "gl_PointSize       = gl_in[0].gl_Position.x;\n";
			}
			else if (iteration == TEST_ITERATION_UNDECLARED_OUT_PERVERTEX_TC_GL_POSITION_USAGE ||
					 iteration == TEST_ITERATION_UNDECLARED_OUT_PERVERTEX_TE_GL_POSITION_USAGE)
			{
				*current_sstream_ptr << "gl_Position        = vec4(gl_in[0].gl_PointSize);\n";
			}

			tc_sstream << "}\n";
		} /* for (both TC and TE stages) */

		/* Store the bodies */
		*out_tc_body_ptr = tc_sstream.str();
		*out_te_body_ptr = te_sstream.str();
		*out_vs_body_ptr = getVertexShaderBody(context_type, iteration);

		break;
	}

	case TEST_ITERATION_UNDECLARED_OUT_PERVERTEX_VS_GL_CLIPDISTANCE_USAGE:
	case TEST_ITERATION_UNDECLARED_OUT_PERVERTEX_VS_GL_CULLDISTANCE_USAGE:
	case TEST_ITERATION_UNDECLARED_OUT_PERVERTEX_VS_GL_POINTSIZE_USAGE:
	case TEST_ITERATION_UNDECLARED_OUT_PERVERTEX_VS_GL_POSITION_USAGE:
	{
		const bool is_cull_distance_iteration =
			(iteration == TEST_ITERATION_UNDECLARED_OUT_PERVERTEX_VS_GL_CULLDISTANCE_USAGE);

		*out_min_context_type_ptr = (is_cull_distance_iteration) ? glu::ContextType(4, 5, glu::PROFILE_CORE) :
																   glu::ContextType(4, 1, glu::PROFILE_CORE);
		*out_used_shader_stages_ptr = (_shader_stage)(SHADER_STAGE_VERTEX);

		/* Determine what the main() body contents should be. */
		std::string vs_main_body;

		switch (iteration)
		{
		case TEST_ITERATION_UNDECLARED_OUT_PERVERTEX_VS_GL_CLIPDISTANCE_USAGE:
			vs_main_body = "gl_ClipDistance[0] = 1.0;";
			break;
		case TEST_ITERATION_UNDECLARED_OUT_PERVERTEX_VS_GL_CULLDISTANCE_USAGE:
			vs_main_body = "gl_CullDistance[0] = 2.0;";
			break;
		case TEST_ITERATION_UNDECLARED_OUT_PERVERTEX_VS_GL_POINTSIZE_USAGE:
			vs_main_body = "gl_PointSize = 1.0;";
			break;
		case TEST_ITERATION_UNDECLARED_OUT_PERVERTEX_VS_GL_POSITION_USAGE:
			vs_main_body = "gl_Position = vec4(1.0f, 2.0, 3.0, 4.0);";
			break;
		default:
			vs_main_body = "";
			break;
		}

		/* Store the body */
		*out_vs_body_ptr = getVertexShaderBody(context_type, iteration, vs_main_body);

		break;
	}

	case TEST_ITERATION_PERVERTEX_DECLARATION_MISMATCH_GS_VS:
	case TEST_ITERATION_PERVERTEX_DECLARATION_MISMATCH_GS_TC_TE_VS:
	case TEST_ITERATION_PERVERTEX_DECLARATION_MISMATCH_TC_TE_VS:
	{
		*out_min_context_type_ptr   = glu::ContextType(4, 1, glu::PROFILE_CORE);
		int used_shader_stages = SHADER_STAGE_VERTEX;

		if (iteration == TEST_ITERATION_PERVERTEX_DECLARATION_MISMATCH_GS_TC_TE_VS ||
			iteration == TEST_ITERATION_PERVERTEX_DECLARATION_MISMATCH_GS_VS)
		{
			used_shader_stages |= SHADER_STAGE_GEOMETRY;
		}

		if (iteration == TEST_ITERATION_PERVERTEX_DECLARATION_MISMATCH_GS_TC_TE_VS ||
			iteration == TEST_ITERATION_PERVERTEX_DECLARATION_MISMATCH_TC_TE_VS)
		{
			used_shader_stages |=
				SHADER_STAGE_TESSELLATION_CONTROL | SHADER_STAGE_TESSELLATION_EVALUATION;
		}

		*out_used_shader_stages_ptr = (_shader_stage) used_shader_stages;

		/* Shader bodies are predefined in this case. */
		*out_gs_body_ptr = "#version 410\n"
						   "\n"
						   "layout (points)                   in;\n"
						   "layout (points, max_vertices = 4) out;\n"
						   "\n"
						   "in gl_PerVertex\n"
						   "{\n"
						   "    float gl_ClipDistance[];\n"
						   "} gl_in[];\n"
						   "\n"
						   "out gl_PerVertex\n"
						   "{\n"
						   "    float gl_ClipDistance[];\n"
						   "};\n"
						   "\n"
						   "void main()\n"
						   "{\n"
						   "    gl_ClipDistance[0] = 0.5;\n"
						   "    EmitVertex();\n"
						   "}\n";
		*out_tc_body_ptr = "#version 410\n"
						   "\n"
						   "layout (vertices = 4) out;\n"
						   "\n"
						   "in gl_PerVertex\n"
						   "{\n"
						   "    float gl_PointSize;\n"
						   "} gl_in[];\n"
						   "\n"
						   "out gl_PerVertex\n"
						   "{\n"
						   "    float gl_PointSize;\n"
						   "} gl_out[];\n"
						   "\n"
						   "void main()\n"
						   "{\n"
						   "    gl_out[gl_InvocationID].gl_PointSize = gl_in[0].gl_PointSize + 1.0;\n"
						   "}\n";
		*out_te_body_ptr = "#version 410\n"
						   "\n"
						   "layout (isolines) in;\n"
						   "\n"
						   "in gl_PerVertex\n"
						   "{\n"
						   "    float gl_PointSize;\n"
						   "    vec4  gl_Position;\n"
						   "} gl_in[gl_MaxPatchVertices];\n"
						   "\n"
						   "out gl_PerVertex\n"
						   "{\n"
						   "    float gl_PointSize;\n"
						   "    vec4  gl_Position;\n"
						   "};\n"
						   "\n"
						   "void main()\n"
						   "{\n"
						   "    gl_Position = vec4(gl_in[0].gl_PointSize) + gl_in[1].gl_Position;\n"
						   "}\n";
		*out_vs_body_ptr = "#version 410\n"
						   "\n"
						   "out gl_PerVertex\n"
						   "{\n"
						   "    vec4 gl_Position;\n"
						   "};\n"
						   "\n"
						   "void main()\n"
						   "{\n"
						   "    gl_Position = vec4(2.0);\n"
						   "}\n";

		break;
	}

	case TEST_ITERATION_PERVERTEX_BLOCK_UNDEFINED:
	{
		*out_min_context_type_ptr   = glu::ContextType(4, 1, glu::PROFILE_CORE);
		int used_shader_stages = SHADER_STAGE_VERTEX;

		if (glu::contextSupports(context_type, glu::ApiType::core(3, 2)))
		{
			used_shader_stages |= SHADER_STAGE_GEOMETRY;
		}

		if (glu::contextSupports(context_type, glu::ApiType::core(4, 0)))
		{
			used_shader_stages |=
				SHADER_STAGE_TESSELLATION_CONTROL | SHADER_STAGE_TESSELLATION_EVALUATION;
		}

		*out_used_shader_stages_ptr = (_shader_stage) used_shader_stages;

		*out_gs_body_ptr = "#version 410\n"
						   "\n"
						   "layout (points)                   in;\n"
						   "layout (points, max_vertices = 4) out;\n"
						   "\n"
						   "void main()\n"
						   "{\n"
						   "    gl_Position = vec4(1.0, 2.0, 3.0, 4.0);\n"
						   "    EmitVertex();\n"
						   "}\n";
		*out_tc_body_ptr = "#version 410\n"
						   "\n"
						   "layout(vertices = 4) out;\n"
						   "\n"
						   "void main()\n"
						   "{\n"
						   "    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;\n"
						   "}\n";
		*out_te_body_ptr = "#version 410\n"
						   "\n"
						   "layout (isolines) in;\n"
						   "\n"
						   "void main()\n"
						   "{\n"
						   "    gl_Position = gl_in[0].gl_Position;\n"
						   "}\n";
		*out_vs_body_ptr = "#version 410\n"
						   "\n"
						   "void main()\n"
						   "{\n"
						   "    gl_Position = vec4(1.0, 2.0, 3.0, 4.0);\n"
						   "}\n";

		break;
	}

	default:
		TCU_FAIL("Unrecognized test iteration");
	} /* switch (iteration) */
}

/** Returns a dummy vertex shader body, with main() entry-point using code passed by
 *  the @param main_body argument.
 *
 *  @param context_type Running rendering context's type.
 *  @param iteration    Test iteration to return the vertex shader body for.
 *  @param main_body    Body to use for the main() entry-point.
 *
 *  @return Requested object.
 **/
std::string PerVertexValidationTest::getVertexShaderBody(glu::ContextType context_type, _test_iteration iteration,
														 std::string main_body) const
{
	const bool include_clip_distance = (iteration != TEST_ITERATION_UNDECLARED_IN_PERVERTEX_GS_GL_CLIPDISTANCE_USAGE &&
										iteration != TEST_ITERATION_UNDECLARED_IN_PERVERTEX_TC_GL_CLIPDISTANCE_USAGE &&
										iteration != TEST_ITERATION_UNDECLARED_IN_PERVERTEX_TE_GL_CLIPDISTANCE_USAGE &&
										iteration != TEST_ITERATION_UNDECLARED_OUT_PERVERTEX_GS_GL_CLIPDISTANCE_USAGE &&
										iteration != TEST_ITERATION_UNDECLARED_OUT_PERVERTEX_TC_GL_CLIPDISTANCE_USAGE &&
										iteration != TEST_ITERATION_UNDECLARED_OUT_PERVERTEX_TE_GL_CLIPDISTANCE_USAGE &&
										iteration != TEST_ITERATION_UNDECLARED_OUT_PERVERTEX_VS_GL_CLIPDISTANCE_USAGE);
	const bool include_cull_distance = (iteration != TEST_ITERATION_UNDECLARED_IN_PERVERTEX_GS_GL_CULLDISTANCE_USAGE &&
										iteration != TEST_ITERATION_UNDECLARED_IN_PERVERTEX_TC_GL_CULLDISTANCE_USAGE &&
										iteration != TEST_ITERATION_UNDECLARED_IN_PERVERTEX_TE_GL_CULLDISTANCE_USAGE &&
										iteration != TEST_ITERATION_UNDECLARED_OUT_PERVERTEX_GS_GL_CULLDISTANCE_USAGE &&
										iteration != TEST_ITERATION_UNDECLARED_OUT_PERVERTEX_TC_GL_CULLDISTANCE_USAGE &&
										iteration != TEST_ITERATION_UNDECLARED_OUT_PERVERTEX_TE_GL_CULLDISTANCE_USAGE &&
										iteration != TEST_ITERATION_UNDECLARED_OUT_PERVERTEX_VS_GL_CULLDISTANCE_USAGE &&
										glu::contextSupports(context_type, glu::ApiType::core(4, 5)));
	const bool include_pointsize = (iteration != TEST_ITERATION_UNDECLARED_IN_PERVERTEX_GS_GL_POINTSIZE_USAGE &&
									iteration != TEST_ITERATION_UNDECLARED_IN_PERVERTEX_TC_GL_POINTSIZE_USAGE &&
									iteration != TEST_ITERATION_UNDECLARED_IN_PERVERTEX_TE_GL_POINTSIZE_USAGE &&
									iteration != TEST_ITERATION_UNDECLARED_OUT_PERVERTEX_GS_GL_POINTSIZE_USAGE &&
									iteration != TEST_ITERATION_UNDECLARED_OUT_PERVERTEX_TC_GL_POINTSIZE_USAGE &&
									iteration != TEST_ITERATION_UNDECLARED_OUT_PERVERTEX_TE_GL_POINTSIZE_USAGE &&
									iteration != TEST_ITERATION_UNDECLARED_OUT_PERVERTEX_VS_GL_POINTSIZE_USAGE);
	const bool include_position = (iteration != TEST_ITERATION_UNDECLARED_IN_PERVERTEX_GS_GL_POSITION_USAGE &&
								   iteration != TEST_ITERATION_UNDECLARED_IN_PERVERTEX_TC_GL_POSITION_USAGE &&
								   iteration != TEST_ITERATION_UNDECLARED_IN_PERVERTEX_TE_GL_POSITION_USAGE &&
								   iteration != TEST_ITERATION_UNDECLARED_OUT_PERVERTEX_GS_GL_POSITION_USAGE &&
								   iteration != TEST_ITERATION_UNDECLARED_OUT_PERVERTEX_TC_GL_POSITION_USAGE &&
								   iteration != TEST_ITERATION_UNDECLARED_OUT_PERVERTEX_TE_GL_POSITION_USAGE &&
								   iteration != TEST_ITERATION_UNDECLARED_OUT_PERVERTEX_VS_GL_POSITION_USAGE);
	std::stringstream vs_body_sstream;

	vs_body_sstream << "#version " << ((include_cull_distance) ? "450" : "410") << "\n"
					<< "\n"
					   "in gl_PerVertex\n"
					   "{\n";

	vs_body_sstream << ((include_clip_distance) ? "float gl_ClipDistance[];\n" : "");
	vs_body_sstream << ((include_pointsize) ? "float gl_PointSize;\n" : "");
	vs_body_sstream << ((include_position) ? "vec4  gl_Position;\n" : "");
	vs_body_sstream << ((include_cull_distance) ? "float gl_CullDistance[];\n" : "");

	vs_body_sstream << "};\n"
					   "\n"
					   "out gl_PerVertex\n"
					   "{\n";

	vs_body_sstream << ((include_clip_distance) ? "float gl_ClipDistance[];\n" : "");
	vs_body_sstream << ((include_pointsize) ? "float gl_PointSize;\n" : "");
	vs_body_sstream << ((include_position) ? "vec4  gl_Position;\n" : "");
	vs_body_sstream << ((include_cull_distance) ? "float gl_CullDistance[];\n" : "");

	vs_body_sstream << "};\n"
					   "\n"
					   "void main()\n"
					   "{\n"
					   "    "
					<< main_body << "\n"
									"}\n";

	return vs_body_sstream.str();
}

/** Dummy init function */
void PerVertexValidationTest::init()
{
	/* Left blank on purpose */
}

/** Executes test iteration.
 *
 *  @return Returns STOP when test has finished executing, CONTINUE if more iterations are needed.
 */
tcu::TestNode::IterateResult PerVertexValidationTest::iterate()
{
	const glu::ContextType context_type = m_context.getRenderContext().getType();
	const glw::Functions&  gl			= m_context.getRenderContext().getFunctions();
	bool				   result		= true;

	/* Separable program objects went into core in GL 4.1 */
	if (!glu::contextSupports(context_type, glu::ApiType::core(4, 1)))
	{
		throw tcu::NotSupportedError("Test implemented for OpenGL 4.1 contexts or newer.");
	}

	/* Each test iteration is going to be executed in two different modes:
	 *
	 * 1) Create separate shader programs for each stage. They should link just fine.
	 *    Then create a pipeline and attach to it all the programs. At this stage, the
	 *    validation should report failure.
	 *
	 * 2) Create a single separate shader program, holding all stages. Try to link it.
	 *    This process should report failure.
	 */
	for (int test_iteration = static_cast<int>(TEST_ITERATION_FIRST);
		 test_iteration != static_cast<int>(TEST_ITERATION_COUNT); test_iteration++)
	{
		for (unsigned int n_test_mode = 0; n_test_mode < 2; ++n_test_mode)
		{
			/* Skip the second iteration if any of the shaders is expected not to compile. */
			if (isShaderProgramLinkingFailureExpected(static_cast<_test_iteration>(test_iteration)))
			{
				continue;
			}

			/* Execute the actual test iteration */
			switch (n_test_mode)
			{
			case 0:
				result &= runPipelineObjectValidationTestMode(static_cast<_test_iteration>(test_iteration));
				break;
			case 1:
				result &= runSeparateShaderTestMode(static_cast<_test_iteration>(test_iteration));
				break;
			}

			/* Clean up */
			destroyPOsAndSOs();

			if (m_pipeline_id != 0)
			{
				gl.deleteProgramPipelines(1, /* n */
										  &m_pipeline_id);

				GLU_EXPECT_NO_ERROR(gl.getError(), "glDeleteProgramPipelines() call failed");
			}
		} /* for (both test modes) */
	}	 /* for (all test iterations) */
	m_testCtx.setTestResult(result ? QP_TEST_RESULT_PASS : QP_TEST_RESULT_FAIL, result ? "Pass" : "Fail");

	return STOP;
}

/** Tells whether the linking process should fail for specified test iteration.
 *
 *  @param iteration Test iteration to query.
 *
 *  @return true if the linking process should fail, false otherwise */
bool PerVertexValidationTest::isShaderProgramLinkingFailureExpected(_test_iteration iteration) const
{
	/** All "undeclared in.." and "undeclared out.." test shaders are expected not to link successfully
	 *  when used as separate programs.
	 *  Shaders built as a part of the remaining test iterations should compile and link successfully
	 *  for separate programs implementing only a single shader stage. Later on, however, pipeline
	 *  objects built of these programs should fail to validate, as they use incompatible gl_PerVertex
	 *  block redeclarations.
	 */
	return (iteration < TEST_ITERATION_PERVERTEX_DECLARATION_MISMATCH_GS_VS) ||
		   iteration == TEST_ITERATION_PERVERTEX_BLOCK_UNDEFINED;
}

/** Runs the specified test iteration in the following mode:
 *
 *  >>
 *  A pipeline object is created and shader programs are attached to it. It is expected that validation
 *  should fail.
 *  <<
 *
 *  @param iteration Test iteration to execute.
 *
 *  @return true if the test passed, false otherwise.
 */
bool PerVertexValidationTest::runPipelineObjectValidationTestMode(_test_iteration iteration)
{
	const glu::ContextType context_type = m_context.getRenderContext().getType();
	const glw::Functions&  gl			= m_context.getRenderContext().getFunctions();
	bool				   result		= false;

	const char*		 body_raw_ptr	= NULL;
	glw::GLenum		 expected_result = getProgramPipelineValidationExpectedResult();
	std::string		 fs_body;
	std::string		 gs_body;
	glw::GLint		 link_status;
	glu::ContextType min_context_type;
	std::string		 tc_body;
	std::string		 te_body;
	_shader_stage	used_shader_stages;
	glw::GLint		 validate_status;
	glw::GLint		 validate_expected_status;
	std::string		 vs_body;

	struct _shader_program
	{
		std::string*  body_ptr;
		glw::GLuint*  po_id_ptr;
		_shader_stage shader_stage;
		glw::GLenum   shader_stage_bit_gl;
		glw::GLenum   shader_stage_gl;
	} shader_programs[] = {
		{ &fs_body, &m_fs_po_id, SHADER_STAGE_FRAGMENT, GL_FRAGMENT_SHADER_BIT, GL_FRAGMENT_SHADER },
		{ &gs_body, &m_gs_po_id, SHADER_STAGE_GEOMETRY, GL_GEOMETRY_SHADER_BIT, GL_GEOMETRY_SHADER },
		{ &tc_body, &m_tc_po_id, SHADER_STAGE_TESSELLATION_CONTROL, GL_TESS_CONTROL_SHADER_BIT,
		  GL_TESS_CONTROL_SHADER },
		{ &te_body, &m_te_po_id, SHADER_STAGE_TESSELLATION_EVALUATION, GL_TESS_EVALUATION_SHADER_BIT,
		  GL_TESS_EVALUATION_SHADER },
		{ &vs_body, &m_vs_po_id, SHADER_STAGE_VERTEX, GL_VERTEX_SHADER_BIT, GL_VERTEX_SHADER },
	};
	const unsigned int n_shader_programs =
		static_cast<const unsigned int>(sizeof(shader_programs) / sizeof(shader_programs[0]));

	/* Make sure the test iteration can actually be run under the running rendering context
	 * version.
	 */
	getTestIterationProperties(context_type, iteration, &min_context_type, &used_shader_stages, &gs_body, &tc_body,
							   &te_body, &vs_body);

	if (!glu::contextSupports(context_type, min_context_type.getAPI()))
	{
		result = true;

		goto end;
	}

	/* Set up shader program objects. All shader bodies by themselves are valid, so all shaders should
	 * link just fine. */
	for (unsigned int n_shader_program = 0; n_shader_program < n_shader_programs; ++n_shader_program)
	{
		_shader_program& current_shader_program = shader_programs[n_shader_program];

		if ((used_shader_stages & current_shader_program.shader_stage) != 0)
		{
			body_raw_ptr = current_shader_program.body_ptr->c_str();
			*current_shader_program.po_id_ptr =
				gl.createShaderProgramv(current_shader_program.shader_stage_gl, 1, /* count */
										&body_raw_ptr);

			GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateShaderProgramv() call failed.");

			gl.getProgramiv(*current_shader_program.po_id_ptr, GL_LINK_STATUS, &link_status);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glGetProgramiv() call failed.");

			if (link_status != GL_TRUE)
			{
				char info_log[4096];

				if (!isShaderProgramLinkingFailureExpected(iteration))
				{
					gl.getProgramInfoLog(*current_shader_program.po_id_ptr, sizeof(info_log), DE_NULL, /* length */
										 info_log);

					m_testCtx.getLog() << tcu::TestLog::Message << "The separate "
									   << getShaderStageName(current_shader_program.shader_stage)
									   << " program "
										  "failed to link, even though the shader body is valid.\n"
										  "\n"
										  "Body:\n>>\n"
									   << *current_shader_program.body_ptr << "<<\nInfo log\n>>\n"
									   << info_log << "<<\n"
									   << tcu::TestLog::EndMessage;
				}
				else
				{
					result = true;
				}

				goto end;
			} /* if (link_status != GL_TRUE) */
		}	 /* for (all shader programs) */
	}		  /* for (all shader stages) */

	/* Now that all shader programs are ready, set up a test-specific pipeline object and validate it. */
	gl.genProgramPipelines(1, &m_pipeline_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenProgramPipelines() call failed.");

	gl.bindProgramPipeline(m_pipeline_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindProgramPipeline() call failed.");

	for (unsigned int n_shader_program = 0; n_shader_program < n_shader_programs; ++n_shader_program)
	{
		_shader_program& current_shader_program = shader_programs[n_shader_program];

		if ((used_shader_stages & current_shader_program.shader_stage) != 0)
		{
			gl.useProgramStages(m_pipeline_id, current_shader_program.shader_stage_bit_gl,
								*current_shader_program.po_id_ptr);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgramStages() call failed.");
		}
	} /* for (all shader programs) */

	gl.validateProgramPipeline(m_pipeline_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glValidateProgramPipeline() call failed.");

	gl.getProgramPipelineiv(m_pipeline_id, GL_VALIDATE_STATUS, &validate_status);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetProgramPipelineiv() call failed.");

	if (VALIDATION_RESULT_UNDEFINED != expected_result)
	{
		switch (expected_result)
		{
		case VALIDATION_RESULT_FALSE:
			validate_expected_status = GL_FALSE;
			break;
		case VALIDATION_RESULT_TRUE:
			validate_expected_status = GL_TRUE;
			break;
		default:
			TCU_FAIL("Invalid enum");
			break;
		}

		if (validate_status != validate_expected_status)
		{
			tcu::MessageBuilder message = m_testCtx.getLog().message();

			if (GL_FALSE == validate_expected_status)
			{
				message << "A pipeline object was validated successfully, even though one";
			}
			else
			{
				message << "A pipeline object was validated negatively, even though none";
			}

			message << " of the failure reasons given by spec was applicable.\n"
					<< "\n"
					   "Fragment shader body:\n>>\n"
					<< ((shader_programs[0].body_ptr->length() > 0) ? *shader_programs[0].body_ptr : "[not used]")
					<< "\n<<\nGeometry shader body:\n>>\n"
					<< ((shader_programs[1].body_ptr->length() > 0) ? *shader_programs[1].body_ptr : "[not used]")
					<< "\n<<\nTessellation control shader body:\n>>\n"
					<< ((shader_programs[2].body_ptr->length() > 0) ? *shader_programs[2].body_ptr : "[not used]")
					<< "\n<<\nTessellation evaluation shader body:\n>>\n"
					<< ((shader_programs[3].body_ptr->length() > 0) ? *shader_programs[3].body_ptr : "[not used]")
					<< "\n<<\nVertex shader body:\n>>\n"
					<< ((shader_programs[4].body_ptr->length() > 0) ? *shader_programs[4].body_ptr : "[not used]")
					<< tcu::TestLog::EndMessage;
		} /* if (validate_status != validate_expected_status) */
		else
		{
			result = true;
		}
	}
	else
	{
		result = true;
	}

end:
	return result;
}

/** Runs the specified test iteration in the following mode:
 *
 *  >>
 *  A single separate shader program, to which all shader stages used by the test are attached, is linked.
 *  It is expected the linking process should fail.
 *  <<
 *
 *  @param iteration Test iteration to execute.
 *
 *  @return true if the test passed, false otherwise.
 */
bool PerVertexValidationTest::runSeparateShaderTestMode(_test_iteration iteration)
{
	glw::GLint			  compile_status;
	glu::ContextType	  context_type = m_context.getRenderContext().getType();
	const glw::Functions& gl		   = m_context.getRenderContext().getFunctions();
	glw::GLint			  link_status;
	glu::ContextType	  min_context_type;
	bool				  result = false;
	_shader_stage		  used_shader_stages;

	std::string fs_body;
	std::string gs_body;
	std::string tc_body;
	std::string te_body;
	std::string vs_body;

	struct _shader
	{
		std::string*  body_ptr;
		glw::GLuint*  so_id_ptr;
		_shader_stage shader_stage;
		glw::GLenum   shader_stage_bit_gl;
		glw::GLenum   shader_stage_gl;
	} shaders[] = { { &fs_body, &m_fs_id, SHADER_STAGE_FRAGMENT, GL_FRAGMENT_SHADER_BIT, GL_FRAGMENT_SHADER },
					{ &gs_body, &m_gs_id, SHADER_STAGE_GEOMETRY, GL_GEOMETRY_SHADER_BIT, GL_GEOMETRY_SHADER },
					{ &tc_body, &m_tc_id, SHADER_STAGE_TESSELLATION_CONTROL, GL_TESS_CONTROL_SHADER_BIT,
					  GL_TESS_CONTROL_SHADER },
					{ &te_body, &m_te_id, SHADER_STAGE_TESSELLATION_EVALUATION, GL_TESS_EVALUATION_SHADER_BIT,
					  GL_TESS_EVALUATION_SHADER },
					{ &vs_body, &m_vs_id, SHADER_STAGE_VERTEX, GL_VERTEX_SHADER_BIT, GL_VERTEX_SHADER } };
	const unsigned int n_shaders = static_cast<const unsigned int>(sizeof(shaders) / sizeof(shaders[0]));

	/* Retrieve iteration properties */
	getTestIterationProperties(context_type, iteration, &min_context_type, &used_shader_stages, &gs_body, &tc_body,
							   &te_body, &vs_body);

	if (!glu::contextSupports(context_type, min_context_type.getAPI()))
	{
		result = true;

		goto end;
	}

	/* Bake a single shader with separate programs defined for all shader stages defined by the iteration
	 * and see what happens.
	 *
	 * For simplicity, we re-use m_vs_po_id to store the program object ID.
	 */
	m_vs_po_id = gl.createProgram();

	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateProgram() call failed.");

	for (unsigned int n_shader = 0; n_shader < n_shaders; ++n_shader)
	{
		const char*			 body_raw_ptr		  = DE_NULL;
		const std::string&   current_body		  = *shaders[n_shader].body_ptr;
		const _shader_stage& current_shader_stage = shaders[n_shader].shader_stage;
		glw::GLuint&		 current_so_id		  = *shaders[n_shader].so_id_ptr;
		const glw::GLenum&   current_so_type_gl   = shaders[n_shader].shader_stage_gl;

		if ((used_shader_stages & current_shader_stage) != 0)
		{
			body_raw_ptr  = current_body.c_str();
			current_so_id = gl.createShader(current_so_type_gl);

			GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateShader() call failed.");

			gl.shaderSource(current_so_id, 1,		 /* count */
							&body_raw_ptr, DE_NULL); /* length */
			GLU_EXPECT_NO_ERROR(gl.getError(), "glShaderSource() call failed.");

			/* Ensure the shader compiled successfully. */
			gl.compileShader(current_so_id);

			GLU_EXPECT_NO_ERROR(gl.getError(), "glCompileShader() call failed.");

			gl.getShaderiv(current_so_id, GL_COMPILE_STATUS, &compile_status);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glGetShaderiv() call failed.");

			if (compile_status != GL_TRUE)
			{
				m_testCtx.getLog() << tcu::TestLog::Message << getShaderStageName(current_shader_stage)
								   << " unexpectedly failed to compile.\n"
									  "\nBody:\n>>\n"
								   << current_body << "\n<<\n"
								   << tcu::TestLog::EndMessage;

				goto end;
			}

			/* Attach the shader object to the test program object */
			gl.attachShader(m_vs_po_id, current_so_id);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glAttachShader() call failed.");
		} /* if ((used_shader_stages & current_shader_stage) != 0) */
	}	 /* for (all shader objects) */

	/* Mark the program as separable */
	gl.programParameteri(m_vs_po_id, GL_PROGRAM_SEPARABLE, GL_TRUE);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glProgramParameteri() call failed.");

	/* Try to link the program and check the result. */
	gl.linkProgram(m_vs_po_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glLinkProgram() call failed.");

	gl.getProgramiv(m_vs_po_id, GL_LINK_STATUS, &link_status);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetProgramiv() call failed.");

	if (link_status == GL_TRUE)
	{
		m_testCtx.getLog() << tcu::TestLog::Message
						   << "Separable program, consisting of the following separate shaders, was linked "
							  "successfully, despite incompatible or missing gl_PerVertex block re-declarations.\n"
							  "\n"
							  "Fragment shader program:\n>>\n"
						   << ((fs_body.length() > 0) ? fs_body : "[not used]")
						   << "\n<<\nGeometry shader program:\n>>\n"
						   << ((gs_body.length() > 0) ? gs_body : "[not used]")
						   << "\n<<\nTessellation control shader program:\n>>\n"
						   << ((tc_body.length() > 0) ? tc_body : "[not used]")
						   << "\n<<\nTessellation evaluation shader program:\n>>\n"
						   << ((te_body.length() > 0) ? te_body : "[not used]") << "\n<<\nVertex shader program:\n>>\n"
						   << ((vs_body.length() > 0) ? vs_body : "[not used]") << tcu::TestLog::EndMessage;

		goto end;
	} /* if (link_status == GL_TRUE) */

	/* All done */
	result = true;
end:
	if (!result)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Failed test description: " << getTestIterationName(iteration)
						   << tcu::TestLog::EndMessage;
	}
	return result;
}

/** Constructor.
 *
 *  @param context     Rendering context
 *  @param name        Test name
 *  @param description Test description
 */
ReservedNamesTest::ReservedNamesTest(deqp::Context& context)
	: TestCase(context, "CommonBug_ReservedNames",
			   "Verifies that reserved variable names are rejected by the GL SL compiler"
			   " at the compilation time.")
	, m_max_fs_ssbos(0)
	, m_max_gs_acs(0)
	, m_max_gs_ssbos(0)
	, m_max_tc_acs(0)
	, m_max_tc_ssbos(0)
	, m_max_te_acs(0)
	, m_max_te_ssbos(0)
	, m_max_vs_acs(0)
	, m_max_vs_ssbos(0)
{
	memset(m_so_ids, 0, sizeof(m_so_ids));
}

/** Deinitializes all GL objects created for the purpose of running the test,
 *  as well as any client-side buffers allocated at initialization time
 */
void ReservedNamesTest::deinit()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	for (unsigned int n_so_id = 0; n_so_id < sizeof(m_so_ids) / sizeof(m_so_ids[0]); ++n_so_id)
	{
		const glw::GLuint current_so_id = m_so_ids[n_so_id];

		if (current_so_id != 0)
		{
			gl.deleteShader(current_so_id);
		}
	} /* for (all usedshader object IDs) */
}

/** Returns a literal corresponding to the specified @param language_feature value.
 *
 *  @param language_feature Enum to return the string object for.
 *
 *  @return As specified.
 */
std::string ReservedNamesTest::getLanguageFeatureName(_language_feature language_feature) const
{
	std::string result = "[?!]";

	switch (language_feature)
	{
	case LANGUAGE_FEATURE_ATOMIC_COUNTER:
		result = "atomic counter";
		break;
	case LANGUAGE_FEATURE_ATTRIBUTE:
		result = "attribute";
		break;
	case LANGUAGE_FEATURE_CONSTANT:
		result = "constant";
		break;
	case LANGUAGE_FEATURE_FUNCTION_ARGUMENT_NAME:
		result = "function argument name";
		break;
	case LANGUAGE_FEATURE_FUNCTION_NAME:
		result = "function name";
		break;
	case LANGUAGE_FEATURE_INPUT:
		result = "input variable";
		break;
	case LANGUAGE_FEATURE_INPUT_BLOCK_INSTANCE_NAME:
		result = "input block instance name";
		break;
	case LANGUAGE_FEATURE_INPUT_BLOCK_MEMBER_NAME:
		result = "input block member name";
		break;
	case LANGUAGE_FEATURE_INPUT_BLOCK_NAME:
		result = "input block name";
		break;
	case LANGUAGE_FEATURE_OUTPUT:
		result = "output variable";
		break;
	case LANGUAGE_FEATURE_OUTPUT_BLOCK_INSTANCE_NAME:
		result = "output block instance name";
		break;
	case LANGUAGE_FEATURE_OUTPUT_BLOCK_MEMBER_NAME:
		result = "output block member name";
		break;
	case LANGUAGE_FEATURE_OUTPUT_BLOCK_NAME:
		result = "output block name";
		break;
	case LANGUAGE_FEATURE_SHADER_STORAGE_BLOCK_INSTANCE_NAME:
		result = "shader storage block instance name";
		break;
	case LANGUAGE_FEATURE_SHADER_STORAGE_BLOCK_MEMBER_NAME:
		result = "shader storage block member name";
		break;
	case LANGUAGE_FEATURE_SHADER_STORAGE_BLOCK_NAME:
		result = "shader storage block name";
		break;
	case LANGUAGE_FEATURE_SHARED_VARIABLE:
		result = "shared variable";
		break;
	case LANGUAGE_FEATURE_STRUCTURE_MEMBER:
		result = "structure member";
		break;
	case LANGUAGE_FEATURE_STRUCTURE_INSTANCE_NAME:
		result = "structure instance name";
		break;
	case LANGUAGE_FEATURE_STRUCTURE_NAME:
		result = "structure name";
		break;
	case LANGUAGE_FEATURE_SUBROUTINE_FUNCTION_NAME:
		result = "subroutine function name";
		break;
	case LANGUAGE_FEATURE_SUBROUTINE_TYPE:
		result = "subroutine type";
		break;
	case LANGUAGE_FEATURE_SUBROUTINE_UNIFORM:
		result = "subroutine uniform";
		break;
	case LANGUAGE_FEATURE_UNIFORM:
		result = "uniform";
		break;
	case LANGUAGE_FEATURE_UNIFORM_BLOCK_INSTANCE_NAME:
		result = "uniform block instance name";
		break;
	case LANGUAGE_FEATURE_UNIFORM_BLOCK_MEMBER_NAME:
		result = "uniform block member name";
		break;
	case LANGUAGE_FEATURE_UNIFORM_BLOCK_NAME:
		result = "uniform block name";
		break;
	case LANGUAGE_FEATURE_VARIABLE:
		result = "variable";
		break;
	case LANGUAGE_FEATURE_VARYING:
		result = "varying";
		break;
	default:
		result = "unknown";
		break;
	} /* switch (language_feature) */

	return result;
}

/** Returns keywords and reserved names, specific to the running context version. */
std::vector<std::string> ReservedNamesTest::getReservedNames() const
{
	const glu::ContextType   context_type = m_context.getRenderContext().getType();
	std::vector<std::string> result;

	const char** context_keywords   = NULL;
	unsigned int context_n_keywords = 0;
	unsigned int context_n_reserved = 0;
	const char** context_reserved   = NULL;

	/* Keywords and reserved names were taken straight from relevant shading language specification documents */
	static const char* keywords_gl31[] = {
		"attribute",
		"const",
		"uniform",
		"varying",
		"layout",
		"centroid",
		"flat",
		"smooth",
		"noperspective",
		"break",
		"continue",
		"do",
		"for",
		"while",
		"switch",
		"case",
		"default",
		"if",
		"else",
		"in",
		"out",
		"inout",
		"float",
		"int",
		"void",
		"bool",
		"true",
		"false",
		"invariant",
		"discard",
		"return",
		"mat2",
		"mat3",
		"mat4",
		"mat2x2",
		"mat2x3",
		"mat2x4",
		"mat3x2",
		"mat3x3",
		"mat3x4",
		"mat4x2",
		"mat4x3",
		"mat4x4",
		"vec2",
		"vec3",
		"vec4",
		"ivec2",
		"ivec3",
		"ivec4",
		"bvec2",
		"bvec3",
		"bvec4",
		"uint",
		"uvec2",
		"uvec3",
		"uvec4",
		"lowp",
		"mediump",
		"highp",
		"precision",
		"sampler1D",
		"sampler2D",
		"sampler3D",
		"samplerCube",
		"sampler1DShadow",
		"sampler2DShadow",
		"samplerCubeShadow",
		"sampler1DArray",
		"sampler2DArray",
		"sampler1DArrayShadow",
		"sampler2DArrayShadow",
		"isampler1D",
		"isampler2D",
		"isampler3D",
		"isamplerCube",
		"isampler1DArray",
		"isampler2DArray",
		"usampler1D",
		"usampler2D",
		"usampler3D",
		"usamplerCube",
		"usampler1DArray",
		"usampler2DArray",
		"sampler2DRect",
		"sampler2DRectShadow",
		"isampler2DRect",
		"usampler2DRect",
		"samplerBuffer",
		"isamplerBuffer",
		"usamplerBuffer",
	};
	static const char* keywords_gl32[] = {
		"attribute",
		"const",
		"uniform",
		"varying",
		"layout",
		"centroid",
		"flat",
		"smooth",
		"noperspective",
		"break",
		"continue",
		"do",
		"for",
		"while",
		"switch",
		"case",
		"default",
		"if",
		"else",
		"in",
		"out",
		"inout",
		"float",
		"int",
		"void",
		"bool",
		"true",
		"false",
		"invariant",
		"discard",
		"return",
		"mat2",
		"mat3",
		"mat4",
		"mat2x2",
		"mat2x3",
		"mat2x4",
		"mat3x2",
		"mat3x3",
		"mat3x4",
		"mat4x2",
		"mat4x3",
		"mat4x4",
		"vec2",
		"vec3",
		"vec4",
		"ivec2",
		"ivec3",
		"ivec4",
		"bvec2",
		"bvec3",
		"bvec4",
		"uint",
		"uvec2",
		"uvec3",
		"uvec4",
		"lowp",
		"mediump",
		"highp",
		"precision",
		"sampler1D",
		"sampler2D",
		"sampler3D",
		"samplerCube",
		"sampler1DShadow",
		"sampler2DShadow",
		"samplerCubeShadow",
		"sampler1DArray",
		"sampler2DArray",
		"sampler1DArrayShadow",
		"sampler2DArrayShadow",
		"isampler1D",
		"isampler2D",
		"isampler3D",
		"isamplerCube",
		"isampler1DArray",
		"isampler2DArray",
		"usampler1D",
		"usampler2D",
		"usampler3D",
		"usamplerCube",
		"usampler1DArray",
		"usampler2DArray",
		"sampler2DRect",
		"sampler2DRectShadow",
		"isampler2DRect",
		"usampler2DRect",
		"samplerBuffer",
		"isamplerBuffer",
		"usamplerBuffer",
		"sampler2DMS",
		"isampler2DMS",
		"usampler2DMS",
		"sampler2DMSArray",
		"isampler2DMSArray",
		"usampler2DMSArray",
	};
	static const char* keywords_gl33[] = {
		"attribute",
		"const",
		"uniform",
		"varying",
		"layout",
		"centroid",
		"flat",
		"smooth",
		"noperspective",
		"break",
		"continue",
		"do",
		"for",
		"while",
		"switch",
		"case",
		"default",
		"if",
		"else",
		"in",
		"out",
		"inout",
		"float",
		"int",
		"void",
		"bool",
		"true",
		"false",
		"invariant",
		"discard",
		"return",
		"mat2",
		"mat3",
		"mat4",
		"mat2x2",
		"mat2x3",
		"mat2x4",
		"mat3x2",
		"mat3x3",
		"mat3x4",
		"mat4x2",
		"mat4x3",
		"mat4x4",
		"vec2",
		"vec3",
		"vec4",
		"ivec2",
		"ivec3",
		"ivec4",
		"bvec2",
		"bvec3",
		"bvec4",
		"uint",
		"uvec2",
		"uvec3",
		"uvec4",
		"lowp",
		"mediump",
		"highp",
		"precision",
		"sampler1D",
		"sampler2D",
		"sampler3D",
		"samplerCube",
		"sampler1DShadow",
		"sampler2DShadow",
		"samplerCubeShadow",
		"sampler1DArray",
		"sampler2DArray",
		"sampler1DArrayShadow",
		"sampler2DArrayShadow",
		"isampler1D",
		"isampler2D",
		"isampler3D",
		"isamplerCube",
		"isampler1DArray",
		"isampler2DArray",
		"usampler1D",
		"usampler2D",
		"usampler3D",
		"usamplerCube",
		"usampler1DArray",
		"usampler2DArray",
		"sampler2DRect",
		"sampler2DRectShadow",
		"isampler2DRect",
		"usampler2DRect",
		"samplerBuffer",
		"isamplerBuffer",
		"usamplerBuffer",
		"sampler2DMS",
		"isampler2DMS",
		"usampler2DMS",
		"sampler2DMSArray",
		"isampler2DMSArray",
		"usampler2DMSArray",
	};
	static const char* keywords_gl40[] = {
		"attribute",
		"const",
		"uniform",
		"varying",
		"layout",
		"centroid",
		"flat",
		"smooth",
		"noperspective",
		"patch",
		"sample",
		"break",
		"continue",
		"do",
		"for",
		"while",
		"switch",
		"case",
		"default",
		"if",
		"else",
		"subroutine",
		"in",
		"out",
		"inout",
		"float",
		"double",
		"int",
		"void",
		"bool",
		"true",
		"false",
		"invariant",
		"discard",
		"return",
		"mat2",
		"mat3",
		"mat4",
		"dmat2",
		"dmat3",
		"dmat4",
		"mat2x2",
		"mat2x3",
		"mat2x4",
		"dmat2x2",
		"dmat2x3",
		"dmat2x4",
		"mat3x2",
		"mat3x3",
		"mat3x4",
		"dmat3x2",
		"dmat3x3",
		"dmat3x4",
		"mat4x2",
		"mat4x3",
		"mat4x4",
		"dmat4x2",
		"dmat4x3",
		"dmat4x4",
		"vec2",
		"vec3",
		"vec4",
		"ivec2",
		"ivec3",
		"ivec4",
		"bvec2",
		"bvec3",
		"bvec4",
		"dvec2",
		"dvec3",
		"dvec4",
		"uint",
		"uvec2",
		"uvec3",
		"uvec4",
		"lowp",
		"mediump",
		"highp",
		"precision",
		"sampler1D",
		"sampler2D",
		"sampler3D",
		"samplerCube",
		"sampler1DShadow",
		"sampler2DShadow",
		"samplerCubeShadow",
		"sampler1DArray",
		"sampler2DArray",
		"sampler1DArrayShadow",
		"sampler2DArrayShadow",
		"isampler1D",
		"isampler2D",
		"isampler3D",
		"isamplerCube",
		"isampler1DArray",
		"isampler2DArray",
		"usampler1D",
		"usampler2D",
		"usampler3D",
		"usamplerCube",
		"usampler1DArray",
		"usampler2DArray",
		"sampler2DRect",
		"sampler2DRectShadow",
		"isampler2DRect",
		"usampler2DRect",
		"samplerBuffer",
		"isamplerBuffer",
		"usamplerBuffer",
		"sampler2DMS",
		"isampler2DMS",
		"usampler2DMS",
		"sampler2DMSArray",
		"isampler2DMSArray",
		"usampler2DMSArray",
		"samplerCubeArray",
		"samplerCubeArrayShadow",
		"isamplerCubeArray",
		"usamplerCubeArray",
	};
	static const char* keywords_gl41[] = {
		"attribute",
		"const",
		"uniform",
		"varying",
		"layout",
		"centroid",
		"flat",
		"smooth",
		"noperspective",
		"patch",
		"sample",
		"break",
		"continue",
		"do",
		"for",
		"while",
		"switch",
		"case",
		"default",
		"if",
		"else",
		"subroutine",
		"in",
		"out",
		"inout",
		"float",
		"double",
		"int",
		"void",
		"bool",
		"true",
		"false",
		"invariant",
		"discard",
		"return",
		"mat2",
		"mat3",
		"mat4",
		"dmat2",
		"dmat3",
		"dmat4",
		"mat2x2",
		"mat2x3",
		"mat2x4",
		"dmat2x2",
		"dmat2x3",
		"dmat2x4",
		"mat3x2",
		"mat3x3",
		"mat3x4",
		"dmat3x2",
		"dmat3x3",
		"dmat3x4",
		"mat4x2",
		"mat4x3",
		"mat4x4",
		"dmat4x2",
		"dmat4x3",
		"dmat4x4",
		"vec2",
		"vec3",
		"vec4",
		"ivec2",
		"ivec3",
		"ivec4",
		"bvec2",
		"bvec3",
		"bvec4",
		"dvec2",
		"dvec3",
		"dvec4",
		"uint",
		"uvec2",
		"uvec3",
		"uvec4",
		"lowp",
		"mediump",
		"highp",
		"precision",
		"sampler1D",
		"sampler2D",
		"sampler3D",
		"samplerCube",
		"sampler1DShadow",
		"sampler2DShadow",
		"samplerCubeShadow",
		"sampler1DArray",
		"sampler2DArray",
		"sampler1DArrayShadow",
		"sampler2DArrayShadow",
		"isampler1D",
		"isampler2D",
		"isampler3D",
		"isamplerCube",
		"isampler1DArray",
		"isampler2DArray",
		"usampler1D",
		"usampler2D",
		"usampler3D",
		"usamplerCube",
		"usampler1DArray",
		"usampler2DArray",
		"sampler2DRect",
		"sampler2DRectShadow",
		"isampler2DRect",
		"usampler2DRect",
		"samplerBuffer",
		"isamplerBuffer",
		"usamplerBuffer",
		"sampler2DMS",
		"isampler2DMS",
		"usampler2DMS",
		"sampler2DMSArray",
		"isampler2DMSArray",
		"usampler2DMSArray",
		"samplerCubeArray",
		"samplerCubeArrayShadow",
		"isamplerCubeArray",
		"usamplerCubeArray",
	};
	static const char* keywords_gl42[] = {
		"attribute",
		"const",
		"uniform",
		"varying",
		"coherent",
		"volatile",
		"restrict",
		"readonly",
		"writeonly",
		"atomic_uint",
		"layout",
		"centroid",
		"flat",
		"smooth",
		"noperspective",
		"patch",
		"sample",
		"break",
		"continue",
		"do",
		"for",
		"while",
		"switch",
		"case",
		"default",
		"if",
		"else",
		"subroutine",
		"in",
		"out",
		"inout",
		"float",
		"double",
		"int",
		"void",
		"bool",
		"true",
		"false",
		"invariant",
		"discard",
		"return",
		"mat2",
		"mat3",
		"mat4",
		"dmat2",
		"dmat3",
		"dmat4",
		"mat2x2",
		"mat2x3",
		"mat2x4",
		"dmat2x2",
		"dmat2x3",
		"dmat2x4",
		"mat3x2",
		"mat3x3",
		"mat3x4",
		"dmat3x2",
		"dmat3x3",
		"dmat3x4",
		"mat4x2",
		"mat4x3",
		"mat4x4",
		"dmat4x2",
		"dmat4x3",
		"dmat4x4",
		"vec2",
		"vec3",
		"vec4",
		"ivec2",
		"ivec3",
		"ivec4",
		"bvec2",
		"bvec3",
		"bvec4",
		"dvec2",
		"dvec3",
		"dvec4",
		"uint",
		"uvec2",
		"uvec3",
		"uvec4",
		"lowp",
		"mediump",
		"highp",
		"precision",
		"sampler1D",
		"sampler2D",
		"sampler3D",
		"samplerCube",
		"sampler1DShadow",
		"sampler2DShadow",
		"samplerCubeShadow",
		"sampler1DArray",
		"sampler2DArray",
		"sampler1DArrayShadow",
		"sampler2DArrayShadow",
		"isampler1D",
		"isampler2D",
		"isampler3D",
		"isamplerCube",
		"isampler1DArray",
		"isampler2DArray",
		"usampler1D",
		"usampler2D",
		"usampler3D",
		"usamplerCube",
		"usampler1DArray",
		"usampler2DArray",
		"sampler2DRect",
		"sampler2DRectShadow",
		"isampler2DRect",
		"usampler2DRect",
		"samplerBuffer",
		"isamplerBuffer",
		"usamplerBuffer",
		"sampler2DMS",
		"isampler2DMS",
		"usampler2DMS",
		"sampler2DMSArray",
		"isampler2DMSArray",
		"usampler2DMSArray",
		"samplerCubeArray",
		"samplerCubeArrayShadow",
		"isamplerCubeArray",
		"usamplerCubeArray",
		"image1D",
		"iimage1D",
		"uimage1D",
		"image2D",
		"iimage2D",
		"uimage2D",
		"image3D",
		"iimage3D",
		"uimage3D",
		"image2DRect",
		"iimage2DRect",
		"uimage2DRect",
		"imageCube",
		"iimageCube",
		"uimageCube",
		"imageBuffer",
		"iimageBuffer",
		"uimageBuffer",
		"image1DArray",
		"iimage1DArray",
		"uimage1DArray",
		"image2DArray",
		"iimage2DArray",
		"uimage2DArray",
		"imageCubeArray",
		"iimageCubeArray",
		"uimageCubeArray",
		"image2DMS",
		"iimage2DMS",
		"uimage2DMS",
		"image2DMSArray",
		"iimage2DMSArray",
		"uimage2DMSArray",
	};
	static const char* keywords_gl43[] = {
		"attribute",
		"const",
		"uniform",
		"varying",
		"buffer",
		"shared",
		"coherent",
		"volatile",
		"restrict",
		"readonly",
		"writeonly",
		"atomic_uint",
		"layout",
		"centroid",
		"flat",
		"smooth",
		"noperspective",
		"patch",
		"sample",
		"break",
		"continue",
		"do",
		"for",
		"while",
		"switch",
		"case",
		"default",
		"if",
		"else",
		"subroutine",
		"in",
		"out",
		"inout",
		"float",
		"double",
		"int",
		"void",
		"bool",
		"true",
		"false",
		"invariant",
		"precise",
		"discard",
		"return",
		"mat2",
		"mat3",
		"mat4",
		"dmat2",
		"dmat3",
		"dmat4",
		"mat2x2",
		"mat2x3",
		"mat2x4",
		"dmat2x2",
		"dmat2x3",
		"dmat2x4",
		"mat3x2",
		"mat3x3",
		"mat3x4",
		"dmat3x2",
		"dmat3x3",
		"dmat3x4",
		"mat4x2",
		"mat4x3",
		"mat4x4",
		"dmat4x2",
		"dmat4x3",
		"dmat4x4",
		"vec2",
		"vec3",
		"vec4",
		"ivec2",
		"ivec3",
		"ivec4",
		"bvec2",
		"bvec3",
		"bvec4",
		"dvec2",
		"dvec3",
		"dvec4",
		"uint",
		"uvec2",
		"uvec3",
		"uvec4",
		"lowp",
		"mediump",
		"highp",
		"precision",
		"sampler1D",
		"sampler2D",
		"sampler3D",
		"samplerCube",
		"sampler1DShadow",
		"sampler2DShadow",
		"samplerCubeShadow",
		"sampler1DArray",
		"sampler2DArray",
		"sampler1DArrayShadow",
		"sampler2DArrayShadow",
		"isampler1D",
		"isampler2D",
		"isampler3D",
		"isamplerCube",
		"isampler1DArray",
		"isampler2DArray",
		"usampler1D",
		"usampler2D",
		"usampler3D",
		"usamplerCube",
		"usampler1DArray",
		"usampler2DArray",
		"sampler2DRect",
		"sampler2DRectShadow",
		"isampler2DRect",
		"usampler2DRect",
		"samplerBuffer",
		"isamplerBuffer",
		"usamplerBuffer",
		"sampler2DMS",
		"isampler2DMS",
		"usampler2DMS",
		"sampler2DMSArray",
		"isampler2DMSArray",
		"usampler2DMSArray",
		"samplerCubeArray",
		"samplerCubeArrayShadow",
		"isamplerCubeArray",
		"usamplerCubeArray",
		"image1D",
		"iimage1D",
		"uimage1D",
		"image2D",
		"iimage2D",
		"uimage2D",
		"image3D",
		"iimage3D",
		"uimage3D",
		"image2DRect",
		"iimage2DRect",
		"uimage2DRect",
		"imageCube",
		"iimageCube",
		"uimageCube",
		"imageBuffer",
		"iimageBuffer",
		"uimageBuffer",
		"image1DArray",
		"iimage1DArray",
		"uimage1DArray",
		"image2DArray",
		"iimage2DArray",
		"uimage2DArray",
		"imageCubeArray",
		"iimageCubeArray",
		"uimageCubeArray",
		"image2DMS",
		"iimage2DMS",
		"uimage2DMS",
		"image2DMSArray",
		"iimage2DMSArray",
		"uimage2DMSArray",
	};
	static const char* keywords_gl44[] = {
		"attribute",
		"const",
		"uniform",
		"varying",
		"buffer",
		"shared",
		"coherent",
		"volatile",
		"restrict",
		"readonly",
		"writeonly",
		"atomic_uint",
		"layout",
		"centroid",
		"flat",
		"smooth",
		"noperspective",
		"patch",
		"sample",
		"break",
		"continue",
		"do",
		"for",
		"while",
		"switch",
		"case",
		"default",
		"if",
		"else",
		"subroutine",
		"in",
		"out",
		"inout",
		"float",
		"double",
		"int",
		"void",
		"bool",
		"true",
		"false",
		"invariant",
		"precise",
		"discard",
		"return",
		"mat2",
		"mat3",
		"mat4",
		"dmat2",
		"dmat3",
		"dmat4",
		"mat2x2",
		"mat2x3",
		"mat2x4",
		"dmat2x2",
		"dmat2x3",
		"dmat2x4",
		"mat3x2",
		"mat3x3",
		"mat3x4",
		"dmat3x2",
		"dmat3x3",
		"dmat3x4",
		"mat4x2",
		"mat4x3",
		"mat4x4",
		"dmat4x2",
		"dmat4x3",
		"dmat4x4",
		"vec2",
		"vec3",
		"vec4",
		"ivec2",
		"ivec3",
		"ivec4",
		"bvec2",
		"bvec3",
		"bvec4",
		"dvec2",
		"dvec3",
		"dvec4",
		"uint",
		"uvec2",
		"uvec3",
		"uvec4",
		"lowp",
		"mediump",
		"highp",
		"precision",
		"sampler1D",
		"sampler2D",
		"sampler3D",
		"samplerCube",
		"sampler1DShadow",
		"sampler2DShadow",
		"samplerCubeShadow",
		"sampler1DArray",
		"sampler2DArray",
		"sampler1DArrayShadow",
		"sampler2DArrayShadow",
		"isampler1D",
		"isampler2D",
		"isampler3D",
		"isamplerCube",
		"isampler1DArray",
		"isampler2DArray",
		"usampler1D",
		"usampler2D",
		"usampler3D",
		"usamplerCube",
		"usampler1DArray",
		"usampler2DArray",
		"sampler2DRect",
		"sampler2DRectShadow",
		"isampler2DRect",
		"usampler2DRect",
		"samplerBuffer",
		"isamplerBuffer",
		"usamplerBuffer",
		"sampler2DMS",
		"isampler2DMS",
		"usampler2DMS",
		"sampler2DMSArray",
		"isampler2DMSArray",
		"usampler2DMSArray",
		"samplerCubeArray",
		"samplerCubeArrayShadow",
		"isamplerCubeArray",
		"usamplerCubeArray",
		"image1D",
		"iimage1D",
		"uimage1D",
		"image2D",
		"iimage2D",
		"uimage2D",
		"image3D",
		"iimage3D",
		"uimage3D",
		"image2DRect",
		"iimage2DRect",
		"uimage2DRect",
		"imageCube",
		"iimageCube",
		"uimageCube",
		"imageBuffer",
		"iimageBuffer",
		"uimageBuffer",
		"image1DArray",
		"iimage1DArray",
		"uimage1DArray",
		"image2DArray",
		"iimage2DArray",
		"uimage2DArray",
		"imageCubeArray",
		"iimageCubeArray",
		"uimageCubeArray",
		"image2DMS",
		"iimage2DMS",
		"uimage2DMS",
		"image2DMSArray",
		"iimage2DMSArray",
		"uimage2DMSArray",
	};
	static const char* keywords_gl45[] = {
		"attribute",
		"const",
		"uniform",
		"varying",
		"buffer",
		"shared",
		"coherent",
		"volatile",
		"restrict",
		"readonly",
		"writeonly",
		"atomic_uint",
		"layout",
		"centroid",
		"flat",
		"smooth",
		"noperspective",
		"patch",
		"sample",
		"break",
		"continue",
		"do",
		"for",
		"while",
		"switch",
		"case",
		"default",
		"if",
		"else",
		"subroutine",
		"in",
		"out",
		"inout",
		"float",
		"double",
		"int",
		"void",
		"bool",
		"true",
		"false",
		"invariant",
		"precise",
		"discard",
		"return",
		"mat2",
		"mat3",
		"mat4",
		"dmat2",
		"dmat3",
		"dmat4",
		"mat2x2",
		"mat2x3",
		"mat2x4",
		"dmat2x2",
		"dmat2x3",
		"dmat2x4",
		"mat3x2",
		"mat3x3",
		"mat3x4",
		"dmat3x2",
		"dmat3x3",
		"dmat3x4",
		"mat4x2",
		"mat4x3",
		"mat4x4",
		"dmat4x2",
		"dmat4x3",
		"dmat4x4",
		"vec2",
		"vec3",
		"vec4",
		"ivec2",
		"ivec3",
		"ivec4",
		"bvec2",
		"bvec3",
		"bvec4",
		"dvec2",
		"dvec3",
		"dvec4",
		"uint",
		"uvec2",
		"uvec3",
		"uvec4",
		"lowp",
		"mediump",
		"highp",
		"precision",
		"sampler1D",
		"sampler2D",
		"sampler3D",
		"samplerCube",
		"sampler1DShadow",
		"sampler2DShadow",
		"samplerCubeShadow",
		"sampler1DArray",
		"sampler2DArray",
		"sampler1DArrayShadow",
		"sampler2DArrayShadow",
		"isampler1D",
		"isampler2D",
		"isampler3D",
		"isamplerCube",
		"isampler1DArray",
		"isampler2DArray",
		"usampler1D",
		"usampler2D",
		"usampler3D",
		"usamplerCube",
		"usampler1DArray",
		"usampler2DArray",
		"sampler2DRect",
		"sampler2DRectShadow",
		"isampler2DRect",
		"usampler2DRect",
		"samplerBuffer",
		"isamplerBuffer",
		"usamplerBuffer",
		"sampler2DMS",
		"isampler2DMS",
		"usampler2DMS",
		"sampler2DMSArray",
		"isampler2DMSArray",
		"usampler2DMSArray",
		"samplerCubeArray",
		"samplerCubeArrayShadow",
		"isamplerCubeArray",
		"usamplerCubeArray",
		"image1D",
		"iimage1D",
		"uimage1D",
		"image2D",
		"iimage2D",
		"uimage2D",
		"image3D",
		"iimage3D",
		"uimage3D",
		"image2DRect",
		"iimage2DRect",
		"uimage2DRect",
		"imageCube",
		"iimageCube",
		"uimageCube",
		"imageBuffer",
		"iimageBuffer",
		"uimageBuffer",
		"image1DArray",
		"iimage1DArray",
		"uimage1DArray",
		"image2DArray",
		"iimage2DArray",
		"uimage2DArray",
		"imageCubeArray",
		"iimageCubeArray",
		"uimageCubeArray",
		"image2DMS",
		"iimage2DMS",
		"uimage2DMS",
		"image2DMSArray",
		"iimage2DMSArray",
		"uimage2DMSArray",
	};
	static const char* keywords_gl46[] = {
		"attribute",
		"const",
		"uniform",
		"varying",
		"buffer",
		"shared",
		"coherent",
		"volatile",
		"restrict",
		"readonly",
		"writeonly",
		"atomic_uint",
		"layout",
		"centroid",
		"flat",
		"smooth",
		"noperspective",
		"patch",
		"sample",
		"break",
		"continue",
		"do",
		"for",
		"while",
		"switch",
		"case",
		"default",
		"if",
		"else",
		"subroutine",
		"in",
		"out",
		"inout",
		"float",
		"double",
		"int",
		"void",
		"bool",
		"true",
		"false",
		"invariant",
		"precise",
		"discard",
		"return",
		"mat2",
		"mat3",
		"mat4",
		"dmat2",
		"dmat3",
		"dmat4",
		"mat2x2",
		"mat2x3",
		"mat2x4",
		"dmat2x2",
		"dmat2x3",
		"dmat2x4",
		"mat3x2",
		"mat3x3",
		"mat3x4",
		"dmat3x2",
		"dmat3x3",
		"dmat3x4",
		"mat4x2",
		"mat4x3",
		"mat4x4",
		"dmat4x2",
		"dmat4x3",
		"dmat4x4",
		"vec2",
		"vec3",
		"vec4",
		"ivec2",
		"ivec3",
		"ivec4",
		"bvec2",
		"bvec3",
		"bvec4",
		"dvec2",
		"dvec3",
		"dvec4",
		"uint",
		"uvec2",
		"uvec3",
		"uvec4",
		"lowp",
		"mediump",
		"highp",
		"precision",
		"sampler1D",
		"sampler2D",
		"sampler3D",
		"samplerCube",
		"sampler1DShadow",
		"sampler2DShadow",
		"samplerCubeShadow",
		"sampler1DArray",
		"sampler2DArray",
		"sampler1DArrayShadow",
		"sampler2DArrayShadow",
		"isampler1D",
		"isampler2D",
		"isampler3D",
		"isamplerCube",
		"isampler1DArray",
		"isampler2DArray",
		"usampler1D",
		"usampler2D",
		"usampler3D",
		"usamplerCube",
		"usampler1DArray",
		"usampler2DArray",
		"sampler2DRect",
		"sampler2DRectShadow",
		"isampler2DRect",
		"usampler2DRect",
		"samplerBuffer",
		"isamplerBuffer",
		"usamplerBuffer",
		"sampler2DMS",
		"isampler2DMS",
		"usampler2DMS",
		"sampler2DMSArray",
		"isampler2DMSArray",
		"usampler2DMSArray",
		"samplerCubeArray",
		"samplerCubeArrayShadow",
		"isamplerCubeArray",
		"usamplerCubeArray",
		"image1D",
		"iimage1D",
		"uimage1D",
		"image2D",
		"iimage2D",
		"uimage2D",
		"image3D",
		"iimage3D",
		"uimage3D",
		"image2DRect",
		"iimage2DRect",
		"uimage2DRect",
		"imageCube",
		"iimageCube",
		"uimageCube",
		"imageBuffer",
		"iimageBuffer",
		"uimageBuffer",
		"image1DArray",
		"iimage1DArray",
		"uimage1DArray",
		"image2DArray",
		"iimage2DArray",
		"uimage2DArray",
		"imageCubeArray",
		"iimageCubeArray",
		"uimageCubeArray",
		"image2DMS",
		"iimage2DMS",
		"uimage2DMS",
		"image2DMSArray",
		"iimage2DMSArray",
		"uimage2DMSArray",
		"struct"
	};
	static const char* reserved_gl31[] = {
		"common",
		"partition",
		"active",
		"asm",
		"class",
		"union",
		"enum",
		"typedef",
		"template",
		"this",
		"packed",
		"goto",
		"inline",
		"noinline",
		"volatile",
		"public",
		"static",
		"extern",
		"external",
		"interface",
		"long",
		"short",
		"double",
		"half",
		"fixed",
		"unsigned",
		"superp",
		"input",
		"output",
		"hvec2",
		"hvec3",
		"hvec4",
		"dvec2",
		"dvec3",
		"dvec4",
		"fvec2",
		"fvec3",
		"fvec4",
		"sampler3DRect",
		"filter",
		"image1D",
		"image2D",
		"image3D",
		"imageCube",
		"iimage1D",
		"iimage2D",
		"iimage3D",
		"iimageCube",
		"uimage1D",
		"uimage2D",
		"uimage3D",
		"uimageCube",
		"image1DArray",
		"image2DArray",
		"iimage1DArray",
		"iimage2DArray",
		"uimage1DArray",
		"uimage2DArray",
		"image1DShadow",
		"image2DShadow",
		"image1DArrayShadow",
		"image2DArrayShadow",
		"imageBuffer",
		"iimageBuffer",
		"uimageBuffer",
		"sizeof",
		"cast",
		"namespace",
		"using",
		"row_major",
	};
	static const char* reserved_gl32[] = {
		"common",
		"partition",
		"active",
		"asm",
		"class",
		"union",
		"enum",
		"typedef",
		"template",
		"this",
		"packed",
		"goto",
		"inline",
		"noinline",
		"volatile",
		"public",
		"static",
		"extern",
		"external",
		"interface",
		"long",
		"short",
		"double",
		"half",
		"fixed",
		"unsigned",
		"superp",
		"input",
		"output",
		"hvec2",
		"hvec3",
		"hvec4",
		"dvec2",
		"dvec3",
		"dvec4",
		"fvec2",
		"fvec3",
		"fvec4",
		"sampler3DRect",
		"filter",
		"image1D",
		"image2D",
		"image3D",
		"imageCube",
		"iimage1D",
		"iimage2D",
		"iimage3D",
		"iimageCube",
		"uimage1D",
		"uimage2D",
		"uimage3D",
		"uimageCube",
		"image1DArray",
		"image2DArray",
		"iimage1DArray",
		"iimage2DArray",
		"uimage1DArray",
		"uimage2DArray",
		"image1DShadow",
		"image2DShadow",
		"image1DArrayShadow",
		"image2DArrayShadow",
		"imageBuffer",
		"iimageBuffer",
		"uimageBuffer",
		"sizeof",
		"cast",
		"namespace",
		"using",
		"row_major",
	};
	static const char* reserved_gl33[] = {
		"common",
		"partition",
		"active",
		"asm",
		"class",
		"union",
		"enum",
		"typedef",
		"template",
		"this",
		"packed",
		"goto",
		"inline",
		"noinline",
		"volatile",
		"public",
		"static",
		"extern",
		"external",
		"interface",
		"long",
		"short",
		"double",
		"half",
		"fixed",
		"unsigned",
		"superp",
		"input",
		"output",
		"hvec2",
		"hvec3",
		"hvec4",
		"dvec2",
		"dvec3",
		"dvec4",
		"fvec2",
		"fvec3",
		"fvec4",
		"sampler3DRect",
		"filter",
		"image1D",
		"image2D",
		"image3D",
		"imageCube",
		"iimage1D",
		"iimage2D",
		"iimage3D",
		"iimageCube",
		"uimage1D",
		"uimage2D",
		"uimage3D",
		"uimageCube",
		"image1DArray",
		"image2DArray",
		"iimage1DArray",
		"iimage2DArray",
		"uimage1DArray",
		"uimage2DArray",
		"image1DShadow",
		"image2DShadow",
		"image1DArrayShadow",
		"image2DArrayShadow",
		"imageBuffer",
		"iimageBuffer",
		"uimageBuffer",
		"sizeof",
		"cast",
		"namespace",
		"using",
		"row_major",
	};
	static const char* reserved_gl40[] = {
		"common",
		"partition",
		"active",
		"asm",
		"class",
		"union",
		"enum",
		"typedef",
		"template",
		"this",
		"packed",
		"goto",
		"inline",
		"noinline",
		"volatile",
		"public",
		"static",
		"extern",
		"external",
		"interface",
		"long",
		"short",
		"half",
		"fixed",
		"unsigned",
		"superp",
		"input",
		"output",
		"hvec2",
		"hvec3",
		"hvec4",
		"fvec2",
		"fvec3",
		"fvec4",
		"sampler3DRect",
		"filter",
		"image1D",
		"image2D",
		"image3D",
		"imageCube",
		"iimage1D",
		"iimage2D",
		"iimage3D",
		"iimageCube",
		"uimage1D",
		"uimage2D",
		"uimage3D",
		"uimageCube",
		"image1DArray",
		"image2DArray",
		"iimage1DArray",
		"iimage2DArray",
		"uimage1DArray",
		"uimage2DArray",
		"image1DShadow",
		"image2DShadow",
		"image1DArrayShadow",
		"image2DArrayShadow",
		"imageBuffer",
		"iimageBuffer",
		"uimageBuffer",
		"sizeof",
		"cast",
		"namespace",
		"using",
		"row_major",
	};
	static const char* reserved_gl41[] = {
		"common",
		"partition",
		"active",
		"asm",
		"class",
		"union",
		"enum",
		"typedef",
		"template",
		"this",
		"packed",
		"goto",
		"inline",
		"noinline",
		"volatile",
		"public",
		"static",
		"extern",
		"external",
		"interface",
		"long",
		"short",
		"half",
		"fixed",
		"unsigned",
		"superp",
		"input",
		"output",
		"hvec2",
		"hvec3",
		"hvec4",
		"fvec2",
		"fvec3",
		"fvec4",
		"sampler3DRect",
		"filter",
		"image1D",
		"image2D",
		"image3D",
		"imageCube",
		"iimage1D",
		"iimage2D",
		"iimage3D",
		"iimageCube",
		"uimage1D",
		"uimage2D",
		"uimage3D",
		"uimageCube",
		"image1DArray",
		"image2DArray",
		"iimage1DArray",
		"iimage2DArray",
		"uimage1DArray",
		"uimage2DArray",
		"image1DShadow",
		"image2DShadow",
		"image1DArrayShadow",
		"image2DArrayShadow",
		"imageBuffer",
		"iimageBuffer",
		"uimageBuffer",
		"sizeof",
		"cast",
		"namespace",
		"using",
		"row_major",
	};
	static const char* reserved_gl42[] = {
		"common",   "partition", "active",	"asm",   "class",		"union",	"enum",		"typedef",		 "template",
		"this",		"packed",	"resource",  "goto",  "inline",	"noinline", "public",   "static",		 "extern",
		"external", "interface", "long",	  "short", "half",		"fixed",	"unsigned", "superp",		 "input",
		"output",   "hvec2",	 "hvec3",	 "hvec4", "fvec2",		"fvec3",	"fvec4",	"sampler3DRect", "filter",
		"sizeof",   "cast",		 "namespace", "using", "row_major",
	};
	static const char* reserved_gl43[] = {
		"common",   "partition", "active",	"asm",   "class",		"union",	"enum",		"typedef",		 "template",
		"this",		"packed",	"resource",  "goto",  "inline",	"noinline", "public",   "static",		 "extern",
		"external", "interface", "long",	  "short", "half",		"fixed",	"unsigned", "superp",		 "input",
		"output",   "hvec2",	 "hvec3",	 "hvec4", "fvec2",		"fvec3",	"fvec4",	"sampler3DRect", "filter",
		"sizeof",   "cast",		 "namespace", "using", "row_major",
	};
	static const char* reserved_gl44[] = {
		"common",   "partition",	 "active",	"asm",	"class",  "union",	 "enum",   "typedef",
		"template", "this",			 "resource",  "goto",   "inline", "noinline",  "public", "static",
		"extern",   "external",		 "interface", "long",   "short",  "half",	  "fixed",  "unsigned",
		"superp",   "input",		 "output",	"hvec2",  "hvec3",  "hvec4",	 "fvec2",  "fvec3",
		"fvec4",	"sampler3DRect", "filter",	"sizeof", "cast",   "namespace", "using",
	};
	static const char* reserved_gl45[] = {
		"common",   "partition",	 "active",	"asm",	"class",  "union",	 "enum",   "typedef",
		"template", "this",			 "resource",  "goto",   "inline", "noinline",  "public", "static",
		"extern",   "external",		 "interface", "long",   "short",  "half",	  "fixed",  "unsigned",
		"superp",   "input",		 "output",	"hvec2",  "hvec3",  "hvec4",	 "fvec2",  "fvec3",
		"fvec4",	"sampler3DRect", "filter",	"sizeof", "cast",   "namespace", "using",
	};
	static const char* reserved_gl46[] = {
		"common",   "partition",	 "active",	"asm",	"class",  "union",	 "enum",   "typedef",
		"template", "this",			 "resource",  "goto",   "inline", "noinline",  "public", "static",
		"extern",   "external",		 "interface", "long",   "short",  "half",	  "fixed",  "unsigned",
		"superp",   "input",		 "output",	"hvec2",  "hvec3",  "hvec4",	 "fvec2",  "fvec3",
		"fvec4",	"sampler3DRect", "filter",	"sizeof", "cast",   "namespace", "using",
	};

	glu::ApiType apiType = context_type.getAPI();
	if (apiType == glu::ApiType::core(3, 1))
	{
		context_keywords   = keywords_gl31;
		context_reserved   = reserved_gl31;
		context_n_keywords = static_cast<unsigned int>(sizeof(keywords_gl31) / sizeof(keywords_gl31[0]));
		context_n_reserved = static_cast<unsigned int>(sizeof(reserved_gl31) / sizeof(reserved_gl31[0]));
	}
	else if (apiType == glu::ApiType::core(3, 2))
	{
		context_keywords   = keywords_gl32;
		context_reserved   = reserved_gl32;
		context_n_keywords = static_cast<unsigned int>(sizeof(keywords_gl32) / sizeof(keywords_gl32[0]));
		context_n_reserved = static_cast<unsigned int>(sizeof(reserved_gl32) / sizeof(reserved_gl32[0]));
	}
	else if (apiType == glu::ApiType::core(3, 3))
	{
		context_keywords   = keywords_gl33;
		context_reserved   = reserved_gl33;
		context_n_keywords = static_cast<unsigned int>(sizeof(keywords_gl33) / sizeof(keywords_gl33[0]));
		context_n_reserved = static_cast<unsigned int>(sizeof(reserved_gl33) / sizeof(reserved_gl33[0]));
	}
	else if (apiType == glu::ApiType::core(4, 0))
	{
		context_keywords   = keywords_gl40;
		context_reserved   = reserved_gl40;
		context_n_keywords = static_cast<unsigned int>(sizeof(keywords_gl40) / sizeof(keywords_gl40[0]));
		context_n_reserved = static_cast<unsigned int>(sizeof(reserved_gl40) / sizeof(reserved_gl40[0]));
	}
	else if (apiType == glu::ApiType::core(4, 1))
	{
		context_keywords   = keywords_gl41;
		context_reserved   = reserved_gl41;
		context_n_keywords = static_cast<unsigned int>(sizeof(keywords_gl41) / sizeof(keywords_gl41[0]));
		context_n_reserved = static_cast<unsigned int>(sizeof(reserved_gl41) / sizeof(reserved_gl41[0]));
	}
	else if (apiType == glu::ApiType::core(4, 2))
	{
		context_keywords   = keywords_gl42;
		context_reserved   = reserved_gl42;
		context_n_keywords = static_cast<unsigned int>(sizeof(keywords_gl42) / sizeof(keywords_gl42[0]));
		context_n_reserved = static_cast<unsigned int>(sizeof(reserved_gl42) / sizeof(reserved_gl42[0]));
	}
	else if (apiType == glu::ApiType::core(4, 3))
	{
		context_keywords   = keywords_gl43;
		context_reserved   = reserved_gl43;
		context_n_keywords = static_cast<unsigned int>(sizeof(keywords_gl43) / sizeof(keywords_gl43[0]));
		context_n_reserved = static_cast<unsigned int>(sizeof(reserved_gl43) / sizeof(reserved_gl43[0]));
	}
	else if (apiType == glu::ApiType::core(4, 4))
	{
		context_keywords   = keywords_gl44;
		context_reserved   = reserved_gl44;
		context_n_keywords = static_cast<unsigned int>(sizeof(keywords_gl44) / sizeof(keywords_gl44[0]));
		context_n_reserved = static_cast<unsigned int>(sizeof(reserved_gl44) / sizeof(reserved_gl44[0]));
	}
	else if (apiType == glu::ApiType::core(4, 5))
	{
		context_keywords   = keywords_gl45;
		context_reserved   = reserved_gl45;
		context_n_keywords = static_cast<unsigned int>(sizeof(keywords_gl45) / sizeof(keywords_gl45[0]));
		context_n_reserved = static_cast<unsigned int>(sizeof(reserved_gl45) / sizeof(reserved_gl45[0]));
	}
	else if (apiType == glu::ApiType::core(4, 6))
	{
		context_keywords   = keywords_gl46;
		context_reserved   = reserved_gl46;
		context_n_keywords = static_cast<unsigned int>(sizeof(keywords_gl46) / sizeof(keywords_gl46[0]));
		context_n_reserved = static_cast<unsigned int>(sizeof(reserved_gl46) / sizeof(reserved_gl46[0]));
	}
	else
	{
		TCU_FAIL("Unsupported GL context version - please implement.");
	}

	for (unsigned int n_current_context_keyword = 0; n_current_context_keyword < context_n_keywords;
		 ++n_current_context_keyword)
	{
		const char* current_context_keyword = context_keywords[n_current_context_keyword];

		result.push_back(current_context_keyword);
	} /* for (all context keywords) */

	for (unsigned int n_current_context_reserved = 0; n_current_context_reserved < context_n_reserved;
		 ++n_current_context_reserved)
	{
		const char* current_context_reserved = context_reserved[n_current_context_reserved];

		result.push_back(current_context_reserved);
	} /* for (all context reserved names) */

	/* All done! */
	return result;
}

/** Returns a shader body to use for the test. The body is formed, according to the user-specified
 *  requirements.
 *
 *  @param shader_type      Shader stage the shader body should be returned for.
 *  @param language_feature Language feature to test.
 *  @param invalid_name     Name to use for the language feature instance. The string should come
 *                          from the list of keywords or reserved names, specific to the currently
 *                          running rendering context's version.
 *
 *  @return Requested shader body.
 */
std::string ReservedNamesTest::getShaderBody(_shader_type shader_type, _language_feature language_feature,
											 const char* invalid_name) const
{
	std::stringstream	  body_sstream;
	const glu::ContextType context_type = m_context.getRenderContext().getType();

	/* Preamble: shader language version */
	body_sstream << "#version ";

	glu::ApiType apiType = context_type.getAPI();
	if (apiType == glu::ApiType::core(3, 1))
		body_sstream << "140";
	else if (apiType == glu::ApiType::core(3, 2))
		body_sstream << "150";
	else if (apiType == glu::ApiType::core(3, 3))
		body_sstream << "330";
	else if (apiType == glu::ApiType::core(4, 0))
		body_sstream << "400";
	else if (apiType == glu::ApiType::core(4, 1))
		body_sstream << "410";
	else if (apiType == glu::ApiType::core(4, 2))
		body_sstream << "420";
	else if (apiType == glu::ApiType::core(4, 3))
		body_sstream << "430";
	else if (apiType == glu::ApiType::core(4, 4))
		body_sstream << "440";
	else if (apiType == glu::ApiType::core(4, 5))
		body_sstream << "450";
	else if (apiType == glu::ApiType::core(4, 6))
		body_sstream << "460";
	else
	{
		TCU_FAIL("Unsupported GL context version - please implement");
	}

	body_sstream << "\n\n";

	/* Preamble: layout qualifiers - required for CS, TC and TE shader stages */
	if (shader_type == SHADER_TYPE_COMPUTE)
	{
		body_sstream << "layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;\n";
	}
	else if (shader_type == SHADER_TYPE_TESS_CONTROL)
	{
		body_sstream << "layout(vertices = 3) out;\n";
	}
	else if (shader_type == SHADER_TYPE_TESS_EVALUATION)
	{
		body_sstream << "layout(triangles) in;\n";
	}

	body_sstream << "\n\n";

	/* Language feature: insert incorrectly named atomic counter declaration if needed */
	if (language_feature == LANGUAGE_FEATURE_ATOMIC_COUNTER)
	{
		body_sstream << "layout(binding = 0, offset = 0) uniform atomic_uint " << invalid_name << ";\n";
	}

	/* Language feature: insert incorrectly named attribute declaration if needed */
	if (language_feature == LANGUAGE_FEATURE_ATTRIBUTE)
	{
		body_sstream << "attribute vec4 " << invalid_name << ";\n";
	}

	/* Language feature: insert incorrectly name constant declaration if needed */
	if (language_feature == LANGUAGE_FEATURE_CONSTANT)
	{
		body_sstream << "const vec4 " << invalid_name << " = vec4(2.0, 3.0, 4.0, 5.0);\n";
	}

	/* Language feature: insert a function with incorrectly named argument if needed */
	if (language_feature == LANGUAGE_FEATURE_FUNCTION_ARGUMENT_NAME)
	{
		body_sstream << "void test(in vec4 " << invalid_name << ")\n"
																"{\n"
																"}\n";
	}

	/* Language feature: insert incorrectly named function if needed */
	if (language_feature == LANGUAGE_FEATURE_FUNCTION_NAME)
	{
		body_sstream << "void " << invalid_name << "(in vec4 test)\n"
												   "{\n"
												   "}\n";
	}

	/* Language feature: insert incorrectly named input variable if needed */
	if (language_feature == LANGUAGE_FEATURE_INPUT)
	{
		body_sstream << "in vec4 " << invalid_name;

		if (shader_type == SHADER_TYPE_GEOMETRY || shader_type == SHADER_TYPE_TESS_CONTROL ||
			shader_type == SHADER_TYPE_TESS_EVALUATION)
		{
			body_sstream << "[]";
		}

		body_sstream << ";\n";
	}

	/* Language feature: insert declaration of an incorrectly named input block instance if needed */
	if (language_feature == LANGUAGE_FEATURE_INPUT_BLOCK_INSTANCE_NAME)
	{
		body_sstream << "in testBlock\n"
						"{\n"
						"    vec4 test;\n"
						"} "
					 << invalid_name;

		if (shader_type == SHADER_TYPE_GEOMETRY || shader_type == SHADER_TYPE_TESS_CONTROL ||
			shader_type == SHADER_TYPE_TESS_EVALUATION)
		{
			body_sstream << "[]";
		}

		body_sstream << ";\n";
	}

	/* Language feature: insert declaration of an input block holding an incorrectly named member variable */
	if (language_feature == LANGUAGE_FEATURE_INPUT_BLOCK_MEMBER_NAME)
	{
		body_sstream << "in testBlock\n"
						"{\n"
						"    vec4 "
					 << invalid_name << ";\n"
										"} testBlockInstance";

		if (shader_type == SHADER_TYPE_GEOMETRY || shader_type == SHADER_TYPE_TESS_CONTROL ||
			shader_type == SHADER_TYPE_TESS_EVALUATION)
		{
			body_sstream << "[]";
		}

		body_sstream << ";\n";
	}

	/* Language feature: insert declaration of an incorrectly named input block */
	if (language_feature == LANGUAGE_FEATURE_INPUT_BLOCK_NAME)
	{
		body_sstream << "in " << invalid_name << "\n"
												 "{\n"
												 "    vec4 test;\n"
												 "} testBlockInstance";

		if (shader_type == SHADER_TYPE_GEOMETRY || shader_type == SHADER_TYPE_TESS_CONTROL ||
			shader_type == SHADER_TYPE_TESS_EVALUATION)
		{
			body_sstream << "[]";
		}

		body_sstream << ";\n";
	}

	/* Language feature: insert incorrectly named output variable if needed */
	if (language_feature == LANGUAGE_FEATURE_OUTPUT)
	{
		body_sstream << "out vec4 " << invalid_name;

		if (shader_type == SHADER_TYPE_TESS_CONTROL)
		{
			body_sstream << "[]";
		}

		body_sstream << ";\n";
	}

	/* Language feature: insert declaration of an incorrectly named output block instance if needed */
	if (language_feature == LANGUAGE_FEATURE_OUTPUT_BLOCK_INSTANCE_NAME)
	{
		body_sstream << "out testBlock\n"
						"{\n"
						"    vec4 test;\n"
						"} "
					 << invalid_name;

		if (shader_type == SHADER_TYPE_TESS_CONTROL)
		{
			body_sstream << "[]";
		}

		body_sstream << ";\n";
	}

	/* Language feature: insert declaration of an output block holding an incorrectly named member variable */
	if (language_feature == LANGUAGE_FEATURE_OUTPUT_BLOCK_MEMBER_NAME)
	{
		body_sstream << "out testBlock\n"
						"{\n"
						"    vec4 "
					 << invalid_name << ";\n"
										"} testBlockInstance";

		if (shader_type == SHADER_TYPE_TESS_CONTROL)
		{
			body_sstream << "[]";
		}

		body_sstream << ";\n";
	}

	/* Language feature: insert declaration of an incorrectly named output block */
	if (language_feature == LANGUAGE_FEATURE_OUTPUT_BLOCK_NAME)
	{
		body_sstream << "out " << invalid_name << "\n"
												  "{\n"
												  "    vec4 test;\n"
												  "} testBlockInstance";

		if (shader_type == SHADER_TYPE_TESS_CONTROL)
		{
			body_sstream << "[]";
		}

		body_sstream << ";\n";
	}

	/* Language feature: insert declaration of an incorrectly named shader storage block instance if needed */
	if (language_feature == LANGUAGE_FEATURE_SHADER_STORAGE_BLOCK_INSTANCE_NAME)
	{
		body_sstream << "buffer testBlock\n"
						"{\n"
						"    vec4 test;\n"
						"} "
					 << invalid_name << ";\n";
	}

	/* Language feature: insert declaration of a shader storage block holding an incorrectly named member variable */
	if (language_feature == LANGUAGE_FEATURE_SHADER_STORAGE_BLOCK_MEMBER_NAME)
	{
		body_sstream << "buffer testBlock\n"
						"{\n"
						"    vec4 "
					 << invalid_name << ";\n"
										"};\n";
	}

	/* Language feature: insert declaration of an incorrectly named shader storage block */
	if (language_feature == LANGUAGE_FEATURE_SHADER_STORAGE_BLOCK_NAME)
	{
		body_sstream << "buffer " << invalid_name << "\n"
													 "{\n"
													 "    vec4 test;\n"
													 "};\n";
	}

	/* Language feature: insert declaration of a subroutine function with invalid name */
	if (language_feature == LANGUAGE_FEATURE_SUBROUTINE_FUNCTION_NAME)
	{
		body_sstream << "subroutine void exampleSubroutine(inout vec4 " << invalid_name
					 << ");\n"
						"\n"
						"subroutine (exampleSubroutine) void invert(inout vec4 "
					 << invalid_name << ")\n"
										"{\n"
										"    "
					 << invalid_name << " += vec4(0.0, 1.0, 2.0, 3.0);\n"
										"}\n"
										"\n"
										"subroutine uniform exampleSubroutine testSubroutine;\n";
	}

	/* Language feature: insert declaration of a subroutine of incorrectly named type */
	if (language_feature == LANGUAGE_FEATURE_SUBROUTINE_TYPE)
	{
		body_sstream << "subroutine void " << invalid_name << "(inout vec4 arg);\n"
															  "\n"
															  "subroutine ("
					 << invalid_name << ") void invert(inout vec4 arg)\n"
										"{\n"
										"    arg += vec4(0.0, 1.0, 2.0, 3.0);\n"
										"}\n"
										"\n"
										"subroutine uniform "
					 << invalid_name << " testSubroutine;\n";
	}

	/* Language feature: insert declaration of a subroutine, followed by a declaration of
	 *                   an incorrectly named subroutine uniform.
	 */
	if (language_feature == LANGUAGE_FEATURE_SUBROUTINE_UNIFORM)
	{
		body_sstream << "subroutine void exampleSubroutine(inout vec4 arg);\n"
						"\n"
						"subroutine (exampleSubroutine) void invert(inout vec4 arg)\n"
						"{\n"
						"    arg += vec4(0.0, 1.0, 2.0, 3.0);\n"
						"}\n"
						"\n"
						"subroutine uniform exampleSubroutine "
					 << invalid_name << ";\n";
	}

	/* Language feature: insert declaration of an incorrectly named uniform. */
	if (language_feature == LANGUAGE_FEATURE_UNIFORM)
	{
		body_sstream << "uniform sampler2D " << invalid_name << ";\n";
	}

	/* Language feature: insert declaration of an incorrectly named uniform block instance if needed */
	if (language_feature == LANGUAGE_FEATURE_UNIFORM_BLOCK_INSTANCE_NAME)
	{
		body_sstream << "uniform testBlock\n"
						"{\n"
						"    vec4 test;\n"
						"} "
					 << invalid_name << ";\n";
	}

	/* Language feature: insert declaration of an uniform block holding an incorrectly named member variable */
	if (language_feature == LANGUAGE_FEATURE_UNIFORM_BLOCK_MEMBER_NAME)
	{
		body_sstream << "uniform testBlock\n"
						"{\n"
						"    vec4 "
					 << invalid_name << ";\n"
										"};\n";
	}

	/* Language feature: insert declaration of an incorrectly named uniform block */
	if (language_feature == LANGUAGE_FEATURE_UNIFORM_BLOCK_NAME)
	{
		body_sstream << "uniform " << invalid_name << "\n"
													  "{\n"
													  "    vec4 test;\n"
													  "};\n";
	}

	/* Language feature: insert declaration of an incorrectly named varying */
	if (language_feature == LANGUAGE_FEATURE_VARYING)
	{
		body_sstream << "varying vec4 " << invalid_name << ";\n";
	}

	/* Start implementation of the main entry-point. */
	body_sstream << "void main()\n"
					"{\n";

	/* Language feature: insert declaration of an incorrectly named shared variable. */
	if (language_feature == LANGUAGE_FEATURE_SHARED_VARIABLE)
	{
		body_sstream << "shared vec4 " << invalid_name << ";\n";
	}

	/* Language feature: insert declaration of a structure, whose instance name is incorrect */
	if (language_feature == LANGUAGE_FEATURE_STRUCTURE_INSTANCE_NAME)
	{
		body_sstream << "struct\n"
						"{\n"
						"    vec4 test;\n"
						"} "
					 << invalid_name << ";\n";
	}

	/* Language feature: insert declaration of a structure with one of its member variables being incorrectly named. */
	if (language_feature == LANGUAGE_FEATURE_STRUCTURE_MEMBER)
	{
		body_sstream << "struct\n"
						"{\n"
						"    vec4 "
					 << invalid_name << ";\n"
										"} testInstance;\n";
	}

	/* Language feature: insert declaration of a structure whose name is incorrect */
	if (language_feature == LANGUAGE_FEATURE_STRUCTURE_NAME)
	{
		body_sstream << "struct " << invalid_name << "{\n"
													 "    vec4 test;\n"
					 << "};\n";
	}

	/* Language feature: insert declaration of a variable with incorrect name. */
	if (language_feature == LANGUAGE_FEATURE_VARIABLE)
	{
		body_sstream << "vec4 " << invalid_name << ";\n";
	}

	/* Close the main entry-point implementation */
	body_sstream << "}\n";

	return body_sstream.str();
}

/** Retrieves a literal corresponding to the user-specified shader type value.
 *
 *  @param shader_type Enum to return the string for.
 *
 *  @return As specified.
 */
std::string ReservedNamesTest::getShaderTypeName(_shader_type shader_type) const
{
	std::string result = "[?!]";

	switch (shader_type)
	{
	case SHADER_TYPE_COMPUTE:
		result = "compute shader";
		break;
	case SHADER_TYPE_FRAGMENT:
		result = "fragment shader";
		break;
	case SHADER_TYPE_GEOMETRY:
		result = "geometry shader";
		break;
	case SHADER_TYPE_TESS_CONTROL:
		result = "tessellation control shader";
		break;
	case SHADER_TYPE_TESS_EVALUATION:
		result = "tessellation evaluation shader";
		break;
	case SHADER_TYPE_VERTEX:
		result = "vertex shader";
		break;
	default:
		result = "unknown";
		break;
	} /* switch (shader_type) */

	return result;
}

/** Returns a vector of _language_feature enums, telling which language features are supported, given running context's
 *  version and shader type, in which the features are planned to be used.
 *
 *  @param shader_type Shader stage the language features will be used in.
 *
 *  @return As specified.
 **/
std::vector<ReservedNamesTest::_language_feature> ReservedNamesTest::getSupportedLanguageFeatures(
	_shader_type shader_type) const
{
	const glu::ContextType		   context_type = m_context.getRenderContext().getType();
	std::vector<_language_feature> result;

	/* Atomic counters are available, starting with GL 4.2. Availability for each shader stage
	 * depends on the reported GL constant values, apart from CS & FS, for which AC support is guaranteed.
	 */
	if (glu::contextSupports(context_type, glu::ApiType::core(4, 2)))
	{
		if (shader_type == SHADER_TYPE_COMPUTE || shader_type == SHADER_TYPE_FRAGMENT ||
			(shader_type == SHADER_TYPE_GEOMETRY && m_max_gs_acs > 0) ||
			(shader_type == SHADER_TYPE_TESS_CONTROL && m_max_tc_acs > 0) ||
			(shader_type == SHADER_TYPE_TESS_EVALUATION && m_max_te_acs > 0) ||
			(shader_type == SHADER_TYPE_VERTEX && m_max_vs_acs))
		{
			result.push_back(LANGUAGE_FEATURE_ATOMIC_COUNTER);
		}
	} /* if (context_type >= glu::CONTEXTTYPE_GL43_CORE) */

	/* Attributes are only supported until GL 4.1, for VS shader stage only. */
	if (shader_type == SHADER_TYPE_VERTEX && !glu::contextSupports(context_type, glu::ApiType::core(4, 2)))
	{
		result.push_back(LANGUAGE_FEATURE_ATTRIBUTE);
	}

	/* Constants are always supported */
	result.push_back(LANGUAGE_FEATURE_CONSTANT);

	/* Functions are supported in all GL SL versions for all shader types. */
	result.push_back(LANGUAGE_FEATURE_FUNCTION_ARGUMENT_NAME);
	result.push_back(LANGUAGE_FEATURE_FUNCTION_NAME);

	/* Inputs are supported in all GL SL versions for FS, GS, TC, TE and VS stages */
	if (shader_type == SHADER_TYPE_FRAGMENT || shader_type == SHADER_TYPE_GEOMETRY ||
		shader_type == SHADER_TYPE_TESS_CONTROL || shader_type == SHADER_TYPE_TESS_EVALUATION ||
		shader_type == SHADER_TYPE_VERTEX)
	{
		result.push_back(LANGUAGE_FEATURE_INPUT);
	}

	/* Input blocks are available, starting with GL 3.2 for FS, GS, TC and TE stages. */
	if ((shader_type == SHADER_TYPE_FRAGMENT || shader_type == SHADER_TYPE_GEOMETRY ||
		 shader_type == SHADER_TYPE_TESS_CONTROL || shader_type == SHADER_TYPE_TESS_EVALUATION ||
		 shader_type == SHADER_TYPE_VERTEX) &&
		glu::contextSupports(context_type, glu::ApiType::core(3, 2)))
	{
		result.push_back(LANGUAGE_FEATURE_INPUT_BLOCK_INSTANCE_NAME);
		result.push_back(LANGUAGE_FEATURE_INPUT_BLOCK_MEMBER_NAME);
		result.push_back(LANGUAGE_FEATURE_INPUT_BLOCK_NAME);
	}

	/* Outputs are supported in all GL SL versions for all shader stages expect CS */
	if (shader_type != SHADER_TYPE_COMPUTE)
	{
		result.push_back(LANGUAGE_FEATURE_OUTPUT);
	}

	/* Output blocks are available, starting with GL 3.2 for GS, TC, TE and VS stages. */
	if ((shader_type == SHADER_TYPE_GEOMETRY || shader_type == SHADER_TYPE_TESS_CONTROL ||
		 shader_type == SHADER_TYPE_TESS_EVALUATION || shader_type == SHADER_TYPE_VERTEX) &&
		glu::contextSupports(context_type, glu::ApiType::core(3, 2)))
	{
		result.push_back(LANGUAGE_FEATURE_OUTPUT_BLOCK_INSTANCE_NAME);
		result.push_back(LANGUAGE_FEATURE_OUTPUT_BLOCK_MEMBER_NAME);
		result.push_back(LANGUAGE_FEATURE_OUTPUT_BLOCK_NAME);
	}

	/* Shader storage blocks are available, starting with GL 4.3. Availability for each shader stage
	 * depends on the reported GL constant values, apart from CS, for which SSBO support is guaranteed.
	 */
	if (glu::contextSupports(context_type, glu::ApiType::core(4, 3)))
	{
		if (shader_type == SHADER_TYPE_COMPUTE || (shader_type == SHADER_TYPE_FRAGMENT && m_max_fs_ssbos > 0) ||
			(shader_type == SHADER_TYPE_GEOMETRY && m_max_gs_ssbos > 0) ||
			(shader_type == SHADER_TYPE_TESS_CONTROL && m_max_tc_ssbos > 0) ||
			(shader_type == SHADER_TYPE_TESS_EVALUATION && m_max_te_ssbos > 0) ||
			(shader_type == SHADER_TYPE_VERTEX && m_max_vs_ssbos))
		{
			result.push_back(LANGUAGE_FEATURE_SHADER_STORAGE_BLOCK_INSTANCE_NAME);
			result.push_back(LANGUAGE_FEATURE_SHADER_STORAGE_BLOCK_MEMBER_NAME);
			result.push_back(LANGUAGE_FEATURE_SHADER_STORAGE_BLOCK_NAME);
		}
	} /* if (context_type >= glu::CONTEXTTYPE_GL43_CORE) */

	/* Shared variables are only supported for compute shaders */
	if (shader_type == SHADER_TYPE_COMPUTE)
	{
		result.push_back(LANGUAGE_FEATURE_SHARED_VARIABLE);
	}

	/* Structures are available everywhere, and so are structures. */
	result.push_back(LANGUAGE_FEATURE_STRUCTURE_INSTANCE_NAME);
	result.push_back(LANGUAGE_FEATURE_STRUCTURE_MEMBER);
	result.push_back(LANGUAGE_FEATURE_STRUCTURE_NAME);

	/* Subroutines are available, starting with GL 4.0, for all shader stages except CS */
	if (glu::contextSupports(context_type, glu::ApiType::core(4, 0)))
	{
		if (shader_type != SHADER_TYPE_COMPUTE)
		{
			result.push_back(LANGUAGE_FEATURE_SUBROUTINE_FUNCTION_NAME);
			result.push_back(LANGUAGE_FEATURE_SUBROUTINE_TYPE);
			result.push_back(LANGUAGE_FEATURE_SUBROUTINE_UNIFORM);
		}
	} /* if (context_type >= glu::CONTEXTTYPE_GL40_CORE) */

	/* Uniform blocks and uniforms are available everywhere, for all shader stages except CS */
	if (shader_type != SHADER_TYPE_COMPUTE)
	{
		result.push_back(LANGUAGE_FEATURE_UNIFORM_BLOCK_INSTANCE_NAME);
		result.push_back(LANGUAGE_FEATURE_UNIFORM_BLOCK_MEMBER_NAME);
		result.push_back(LANGUAGE_FEATURE_UNIFORM_BLOCK_NAME);

		result.push_back(LANGUAGE_FEATURE_UNIFORM);
	}

	/* Variables are available, well, everywhere. */
	result.push_back(LANGUAGE_FEATURE_VARIABLE);

	/* Varyings are supported until GL 4.2 for FS and VS shader stages. Starting with GL 4.3,
	 * they are no longer legal. */
	if ((shader_type == SHADER_TYPE_FRAGMENT || shader_type == SHADER_TYPE_VERTEX) &&
		!glu::contextSupports(context_type, glu::ApiType::core(4, 3)))
	{
		result.push_back(LANGUAGE_FEATURE_VARYING);
	}

	return result;
}

/** Returns a vector of _shader_type enums, telling which shader stages are supported
 *  under running rendering context. For simplicity, the function ignores any extensions
 *  which extend the core functionality
 *
 * @return As specified.
 */
std::vector<ReservedNamesTest::_shader_type> ReservedNamesTest::getSupportedShaderTypes() const
{
	const glu::ContextType	context_type = m_context.getRenderContext().getType();
	std::vector<_shader_type> result;

	/* CS: Available, starting with GL 4.3 */
	if (glu::contextSupports(context_type, glu::ApiType::core(4, 3)))
	{
		result.push_back(SHADER_TYPE_COMPUTE);
	}

	/* FS: Always supported */
	result.push_back(SHADER_TYPE_FRAGMENT);

	/* GS: Available, starting with GL 3.2 */
	if (glu::contextSupports(context_type, glu::ApiType::core(3, 2)))
	{
		result.push_back(SHADER_TYPE_GEOMETRY);
	}

	/* TC: Available, starting with GL 4.0 */
	/* TE: Available, starting with GL 4.0 */
	if (glu::contextSupports(context_type, glu::ApiType::core(4, 0)))
	{
		result.push_back(SHADER_TYPE_TESS_CONTROL);
		result.push_back(SHADER_TYPE_TESS_EVALUATION);
	}

	/* VS: Always supported */
	result.push_back(SHADER_TYPE_VERTEX);

	return result;
}

bool ReservedNamesTest::isStructAllowed(_shader_type shader_type, _language_feature language_feature) const
{
	bool structAllowed = false;

	if (language_feature == LANGUAGE_FEATURE_UNIFORM || language_feature == LANGUAGE_FEATURE_UNIFORM_BLOCK_NAME)
	{
		return true;
	}

	switch (shader_type)
	{
	case SHADER_TYPE_FRAGMENT:
	{
		if (language_feature == LANGUAGE_FEATURE_INPUT_BLOCK_NAME)
		{
			structAllowed = true;
		}
	}
	break;
	case SHADER_TYPE_GEOMETRY:
	case SHADER_TYPE_TESS_CONTROL:
	case SHADER_TYPE_TESS_EVALUATION:
	{
		if (language_feature == LANGUAGE_FEATURE_OUTPUT_BLOCK_NAME)
		{
			structAllowed = true;
		}
		else if (language_feature == LANGUAGE_FEATURE_INPUT_BLOCK_NAME)
		{
			structAllowed = true;
		}
	}
	break;
	case SHADER_TYPE_VERTEX:
	{
		if (language_feature == LANGUAGE_FEATURE_OUTPUT_BLOCK_NAME)
		{
			structAllowed = true;
		}
	}
	break;
	case SHADER_TYPE_COMPUTE:
	default:
		break;
	}

	return structAllowed;
}

/** Dummy init function */
void ReservedNamesTest::init()
{
	/* Left blank on purpose */
}

/** Executes test iteration.
 *
 *  @return Returns STOP when test has finished executing, CONTINUE if more iterations are needed.
 */
tcu::TestNode::IterateResult ReservedNamesTest::iterate()
{
	glw::GLint					   compile_status = GL_TRUE;
	glu::ContextType			   context_type   = m_context.getRenderContext().getType();
	const glw::Functions&		   gl			  = m_context.getRenderContext().getFunctions();
	std::vector<_language_feature> language_features;
	std::vector<std::string>	   reserved_names;
	bool						   result = true;
	std::vector<_shader_type>	  shader_types;

	/* Retrieve important GL constant values */
	if (glu::contextSupports(context_type, glu::ApiType::core(4, 2)))
	{
		gl.getIntegerv(GL_MAX_GEOMETRY_ATOMIC_COUNTERS, &m_max_gs_acs);
		gl.getIntegerv(GL_MAX_TESS_CONTROL_ATOMIC_COUNTERS, &m_max_tc_acs);
		gl.getIntegerv(GL_MAX_TESS_EVALUATION_ATOMIC_COUNTERS, &m_max_te_acs);
		gl.getIntegerv(GL_MAX_VERTEX_ATOMIC_COUNTERS, &m_max_vs_acs);
	}

	if (glu::contextSupports(context_type, glu::ApiType::core(4, 3)))
	{
		gl.getIntegerv(GL_MAX_FRAGMENT_SHADER_STORAGE_BLOCKS, &m_max_fs_ssbos);
		gl.getIntegerv(GL_MAX_GEOMETRY_SHADER_STORAGE_BLOCKS, &m_max_gs_ssbos);
		gl.getIntegerv(GL_MAX_TESS_CONTROL_SHADER_STORAGE_BLOCKS, &m_max_tc_ssbos);
		gl.getIntegerv(GL_MAX_TESS_EVALUATION_SHADER_STORAGE_BLOCKS, &m_max_te_ssbos);
		gl.getIntegerv(GL_MAX_VERTEX_SHADER_STORAGE_BLOCKS, &m_max_vs_ssbos);

		GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv() call(s) failed.");
	}

	/* Create the shader objects */
	if (glu::contextSupports(context_type, glu::ApiType::core(4, 0)))
	{
		m_so_ids[SHADER_TYPE_TESS_CONTROL]	= gl.createShader(GL_TESS_CONTROL_SHADER);
		m_so_ids[SHADER_TYPE_TESS_EVALUATION] = gl.createShader(GL_TESS_EVALUATION_SHADER);
	}

	if (glu::contextSupports(context_type, glu::ApiType::core(4, 3)))
	{
		m_so_ids[SHADER_TYPE_COMPUTE] = gl.createShader(GL_COMPUTE_SHADER);
	}

	m_so_ids[SHADER_TYPE_FRAGMENT] = gl.createShader(GL_FRAGMENT_SHADER);
	m_so_ids[SHADER_TYPE_GEOMETRY] = gl.createShader(GL_GEOMETRY_SHADER);
	m_so_ids[SHADER_TYPE_VERTEX]   = gl.createShader(GL_VERTEX_SHADER);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateShader() call failed.");

	/* Retrieve context version-specific data */
	reserved_names = getReservedNames();
	shader_types   = getSupportedShaderTypes();

	/* Iterate over all supported shader stages.. */
	for (std::vector<_shader_type>::const_iterator shader_type_it = shader_types.begin();
		 shader_type_it != shader_types.end(); ++shader_type_it)
	{
		_shader_type current_shader_type = *shader_type_it;

		if (m_so_ids[current_shader_type] == 0)
		{
			/* Skip stages not supported by the currently running context version. */
			continue;
		}

		language_features = getSupportedLanguageFeatures(current_shader_type);

		/* ..and all language features we can test for the running context */
		for (std::vector<_language_feature>::const_iterator language_feature_it = language_features.begin();
			 language_feature_it != language_features.end(); ++language_feature_it)
		{
			_language_feature current_language_feature = *language_feature_it;

			bool structAllowed = isStructAllowed(current_shader_type, current_language_feature);

			/* Finally, all the reserved names we need to test - loop over them at this point */
			for (std::vector<std::string>::const_iterator reserved_name_it = reserved_names.begin();
				 reserved_name_it != reserved_names.end(); ++reserved_name_it)
			{
				std::string current_invalid_name = *reserved_name_it;
				std::string so_body_string;
				const char* so_body_string_raw = NULL;

				// There are certain shader types that allow struct for in/out declarations
				if (structAllowed && current_invalid_name.compare("struct") == 0)
				{
					continue;
				}

				/* Form the shader body */
				so_body_string =
					getShaderBody(current_shader_type, current_language_feature, current_invalid_name.c_str());
				so_body_string_raw = so_body_string.c_str();

				/* Try to compile the shader */
				gl.shaderSource(m_so_ids[current_shader_type], 1, /* count */
								&so_body_string_raw, NULL);		  /* length */
				GLU_EXPECT_NO_ERROR(gl.getError(), "glShaderSource() call failed.");

				gl.compileShader(m_so_ids[current_shader_type]);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glCompileShader() call failed.");

				gl.getShaderiv(m_so_ids[current_shader_type], GL_COMPILE_STATUS, &compile_status);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glGetShaderiv() call failed.");

/* Left for the debugging purposes for those in need .. */
#if 0
				char temp[4096];

				gl.getShaderInfoLog(m_so_ids[current_shader_type],
					4096,
					NULL,
					temp);

				m_testCtx.getLog() << tcu::TestLog::Message
					<< "\n"
					"-----------------------------\n"
					"Shader:\n"
					">>\n"
					<< so_body_string_raw
					<< "\n<<\n"
					"\n"
					"Info log:\n"
					">>\n"
					<< temp
					<< "\n<<\n\n"
					<< tcu::TestLog::EndMessage;
#endif

				if (compile_status != GL_FALSE)
				{
					m_testCtx.getLog() << tcu::TestLog::Message << "A "
									   << getLanguageFeatureName(current_language_feature) << " named ["
									   << current_invalid_name << "]"
									   << ", defined in " << getShaderTypeName(current_shader_type)
									   << ", was accepted by the compiler, "
										  "which is prohibited by the spec. Offending source code:\n"
										  ">>\n"
									   << so_body_string_raw << "\n<<\n\n"
									   << tcu::TestLog::EndMessage;

					result = false;
				}

			} /* for (all reserved names for the current context) */
		}	 /* for (all language features supported by the context) */
	}		  /* for (all shader types supported by the context) */

	m_testCtx.setTestResult(result ? QP_TEST_RESULT_PASS : QP_TEST_RESULT_FAIL, result ? "Pass" : "Fail");

	return STOP;
}

/** Constructor.
 *
 *  @param context     Rendering context
 *  @param name        Test name
 *  @param description Test description
 */
SparseBuffersWithCopyOpsTest::SparseBuffersWithCopyOpsTest(deqp::Context& context)
	: TestCase(context, "CommonBug_SparseBuffersWithCopyOps",
			   "Verifies sparse buffer functionality works correctly when CPU->GPU and GPU->GPU"
			   " memory transfers are involved.")
	, m_bo_id(0)
	, m_bo_read_id(0)
	, m_clear_buffer(DE_NULL)
	, m_page_size(0)
	, m_result_data_storage_size(0)
	, m_n_iterations_to_run(16)
	, m_n_pages_to_test(16)
	, m_virtual_bo_size(512 /* MB */ * 1024768)
{
	for (unsigned int n = 0; n < sizeof(m_reference_data) / sizeof(m_reference_data[0]); ++n)
	{
		m_reference_data[n] = static_cast<unsigned char>(n);
	}
}

/** Deinitializes all GL objects created for the purpose of running the test,
 *  as well as any client-side buffers allocated at initialization time
 */
void SparseBuffersWithCopyOpsTest::deinit()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (m_bo_id != 0)
	{
		gl.deleteBuffers(1, &m_bo_id);

		m_bo_id = 0;
	}

	if (m_bo_read_id != 0)
	{
		gl.deleteBuffers(1, &m_bo_read_id);

		m_bo_read_id = 0;
	}

	if (m_clear_buffer != DE_NULL)
	{
		delete[] m_clear_buffer;

		m_clear_buffer = DE_NULL;
	}
}

/** Dummy init function */
void SparseBuffersWithCopyOpsTest::init()
{
	/* Nothing to do here */
}

/** Initializes all buffers and GL objects required to run the test. */
bool SparseBuffersWithCopyOpsTest::initTest()
{
	const glw::Functions& gl	 = m_context.getRenderContext().getFunctions();
	bool				  result = true;

	/* Retrieve the platform-specific page size */
	gl.getIntegerv(GL_SPARSE_BUFFER_PAGE_SIZE_ARB, &m_page_size);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv() failed for GL_SPARSE_BUFFER_PAGE_SIZE_ARB query");

	/* Retrieve the func ptr */
	if (gl.bufferPageCommitmentARB == NULL)
	{
		m_testCtx.getLog() << tcu::TestLog::Message
						   << "Could not retrieve function pointer for the glBufferPageCommitmentARB() entry-point."
						   << tcu::TestLog::EndMessage;

		result = false;
		goto end;
	}

	/* Set up the test sparse buffer object */
	gl.genBuffers(1, &m_bo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenBuffers() call failed.");

	gl.bindBuffer(GL_ARRAY_BUFFER, m_bo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer() call failed.");

	gl.bufferStorage(GL_ARRAY_BUFFER, m_virtual_bo_size, DE_NULL, /* data */
					 GL_DYNAMIC_STORAGE_BIT | GL_SPARSE_STORAGE_BIT_ARB);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferStorage() call failed.");

	/* Set up the buffer object that will be used to read the result data */
	m_result_data_storage_size = static_cast<unsigned int>(
		(m_page_size * m_n_pages_to_test / sizeof(m_reference_data)) * sizeof(m_reference_data));
	m_clear_buffer = new unsigned char[m_result_data_storage_size];

	memset(m_clear_buffer, 0, m_result_data_storage_size);

	gl.genBuffers(1, &m_bo_read_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenBuffers() call failed.");

	gl.bindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_bo_read_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer() call failed.");

	gl.bufferStorage(GL_ELEMENT_ARRAY_BUFFER, m_result_data_storage_size, NULL, /* data */
					 GL_DYNAMIC_STORAGE_BIT | GL_MAP_READ_BIT);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferStorage() call failed.");

end:
	return result;
}

/** Executes test iteration.
 *
 *  @return Returns STOP when test has finished executing, CONTINUE if more iterations are needed.
 */
tcu::TestNode::IterateResult SparseBuffersWithCopyOpsTest::iterate()
{
	bool result = true;

	/* Execute the test */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Only execute if we're dealing with an OpenGL implementation which supports both:
	 *
	 * 1. GL_ARB_sparse_buffer extension
	 * 2. GL_ARB_buffer_storage extension
	 */
	if (!m_context.getContextInfo().isExtensionSupported("GL_ARB_sparse_buffer") ||
		!m_context.getContextInfo().isExtensionSupported("GL_ARB_buffer_storage"))
	{
		goto end;
	}

	/* Set up the test objects */
	if (!initTest())
	{
		result = false;

		goto end;
	}
	for (unsigned int n_test_case = 0; n_test_case < 2; ++n_test_case)
	{
		for (unsigned int n_iteration = 0; n_iteration < m_n_iterations_to_run; ++n_iteration)
		{
			if (n_iteration != 0)
			{
				gl.bufferPageCommitmentARB(GL_ARRAY_BUFFER, 0, /* offset */
										   m_n_pages_to_test * m_page_size, GL_FALSE);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferPageCommitmentARB() call failed.");
			}

			gl.bufferPageCommitmentARB(GL_ARRAY_BUFFER, 0, /* offset */
									   m_page_size, GL_TRUE);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferPageCommitmentARB() call failed.");

			gl.bufferSubData(GL_ARRAY_BUFFER, 0, /* offset */
							 sizeof(m_reference_data), m_reference_data);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferSubData() call failed.");

			for (unsigned int n_page = 0; n_page < m_n_pages_to_test; ++n_page)
			{
				/* Try committing pages in a redundant manner. This is a legal behavior in light of
				 * the GL_ARB_sparse_buffer spec */
				gl.bufferPageCommitmentARB(GL_ARRAY_BUFFER, n_page * m_page_size, m_page_size, GL_TRUE);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferPageCommitmentARB() call failed.");

				for (int copy_dst_page_offset = static_cast<int>((n_page == 0) ? sizeof(m_reference_data) : 0);
					 copy_dst_page_offset < static_cast<int>(m_page_size);
					 copy_dst_page_offset += static_cast<int>(sizeof(m_reference_data)))
				{
					const int copy_src_page_offset =
						static_cast<const int>(copy_dst_page_offset - sizeof(m_reference_data));

					switch (n_test_case)
					{
					case 0:
					{
						gl.copyBufferSubData(GL_ARRAY_BUFFER, GL_ARRAY_BUFFER,
											 n_page * m_page_size + copy_src_page_offset,
											 n_page * m_page_size + copy_dst_page_offset, sizeof(m_reference_data));
						GLU_EXPECT_NO_ERROR(gl.getError(), "glCopyBufferSubData() call failed.");

						break;
					}

					case 1:
					{
						gl.bufferSubData(GL_ARRAY_BUFFER, n_page * m_page_size + copy_dst_page_offset,
										 sizeof(m_reference_data), m_reference_data);
						GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferSubData() call failed.");

						break;
					}

					default:
						TCU_FAIL("Unrecognized test case index");
					} /* switch (n_test_case) */
				}	 /* for (all valid destination copy op offsets) */
			}		  /* for (all test pages) */

			/* Copy data from the sparse buffer to a mappable immutable buffer storage */
			gl.copyBufferSubData(GL_ARRAY_BUFFER, GL_ELEMENT_ARRAY_BUFFER, 0, /* readOffset */
								 0,											  /* writeOffset */
								 m_result_data_storage_size);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glCopyBufferSubData() call failed.");

			/* Map the data we have obtained */
			char* mapped_data = (char*)gl.mapBufferRange(GL_ELEMENT_ARRAY_BUFFER, 0, /* offset */
														 m_page_size * m_n_pages_to_test, GL_MAP_READ_BIT);

			GLU_EXPECT_NO_ERROR(gl.getError(), "glMapBufferRange() call failed.");

			/* Verify the data is valid */
			for (unsigned int n_temp_copy = 0; n_temp_copy < m_result_data_storage_size / sizeof(m_reference_data);
				 ++n_temp_copy)
			{
				const unsigned int cmp_offset = static_cast<const unsigned int>(n_temp_copy * sizeof(m_reference_data));

				if (memcmp(mapped_data + cmp_offset, m_reference_data, sizeof(m_reference_data)) != 0)
				{
					m_testCtx.getLog() << tcu::TestLog::Message << "Invalid data found for page index "
																   "["
									   << (cmp_offset / m_page_size) << "]"
																		", BO data offset:"
																		"["
									   << cmp_offset << "]." << tcu::TestLog::EndMessage;

					result = false;
					goto end;
				}
			} /* for (all datasets) */

			/* Clean up */
			gl.unmapBuffer(GL_ELEMENT_ARRAY_BUFFER);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glUnmapBuffer() call failed.");

			/* Also, zero out the other buffer object we copy the result data to, in case
			 * the glCopyBufferSubData() call does not modify it at all */
			gl.bufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, /* offset */
							 m_result_data_storage_size, m_clear_buffer);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferSubData() call failed.");

			/* NOTE: This test passes fine on the misbehaving driver *if* the swapbuffers operation
			 *       issued as a part of the call below is not executed. */
			m_context.getRenderContext().postIterate();
		} /* for (all test iterations) */
	}	 /* for (all test cases) */

end:
	m_testCtx.setTestResult(result ? QP_TEST_RESULT_PASS : QP_TEST_RESULT_FAIL, result ? "Pass" : "Fail");

	return STOP;
}

/** Constructor.
 *
 *  @param context Rendering context.
 */
CommonBugsTests::CommonBugsTests(deqp::Context& context)
	: TestCaseGroup(context, "CommonBugs", "Contains conformance tests that verify various pieces of functionality"
										   " which were found broken in public drivers.")
{
}

/** Initializes the test group contents. */
void CommonBugsTests::init()
{
	addChild(new GetProgramivActiveUniformBlockMaxNameLengthTest(m_context));
	addChild(new InputVariablesCannotBeModifiedTest(m_context));
	addChild(new InvalidUseCasesForAllNotFuncsAndExclMarkOpTest(m_context));
	addChild(new InvalidVSInputsTest(m_context));
	addChild(new ParenthesisInLayoutQualifierIntegerValuesTest(m_context));
	addChild(new PerVertexValidationTest(m_context));
	addChild(new ReservedNamesTest(m_context));
	addChild(new SparseBuffersWithCopyOpsTest(m_context));
}
} /* glcts namespace */
