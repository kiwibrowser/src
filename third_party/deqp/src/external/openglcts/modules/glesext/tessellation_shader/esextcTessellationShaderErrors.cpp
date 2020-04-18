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

#include "esextcTessellationShaderErrors.hpp"
#include "gluContextInfo.hpp"
#include "gluDefs.hpp"
#include "glwEnums.hpp"
#include "glwFunctions.hpp"
#include "tcuTestLog.hpp"
#include <cstdarg>
#include <cstdio>
#include <cstring>

namespace glcts
{

/** Constructor
 *
 * @param context       Test context
 * @param name          Test case's name
 * @param description   Test case's description
 **/
TessellationShaderErrors::TessellationShaderErrors(Context& context, const ExtParameters& extParams)
	: TestCaseGroupBase(context, extParams, "compilation_and_linking_errors",
						"Checks that the implementation correctly responds to"
						"various errors in shaders which should result in compilation"
						"or linking errors")
{
	/* Left blank on purpose */
}

/* Instantiates all tests and adds them as children to the node */
void TessellationShaderErrors::init(void)
{
	addChild(new glcts::TessellationShaderError1InputBlocks(m_context, m_extParams));
	addChild(new glcts::TessellationShaderError1InputVariables(m_context, m_extParams));
	addChild(new glcts::TessellationShaderError2OutputBlocks(m_context, m_extParams));
	addChild(new glcts::TessellationShaderError2OutputVariables(m_context, m_extParams));
	addChild(new glcts::TessellationShaderError3InputBlocks(m_context, m_extParams));
	addChild(new glcts::TessellationShaderError3InputVariables(m_context, m_extParams));
	addChild(new glcts::TessellationShaderError4InputBlocks(m_context, m_extParams));
	addChild(new glcts::TessellationShaderError4InputVariables(m_context, m_extParams));
	addChild(new glcts::TessellationShaderError5InputBlocks(m_context, m_extParams));
	addChild(new glcts::TessellationShaderError5InputVariables(m_context, m_extParams));
	addChild(new glcts::TessellationShaderError6(m_context, m_extParams));
	addChild(new glcts::TessellationShaderError7(m_context, m_extParams));
	addChild(new glcts::TessellationShaderError8(m_context, m_extParams));
	addChild(new glcts::TessellationShaderError9(m_context, m_extParams));
	addChild(new glcts::TessellationShaderError10(m_context, m_extParams));
	addChild(new glcts::TessellationShaderError11(m_context, m_extParams));
	addChild(new glcts::TessellationShaderError12(m_context, m_extParams));
	addChild(new glcts::TessellationShaderError13(m_context, m_extParams));
}

/** Constructor
 *
 * @param context       Test context
 * @param name          Test case's name
 * @param description   Test case's description
 **/
TessellationShaderErrorsTestCaseBase::TessellationShaderErrorsTestCaseBase(Context&				context,
																		   const ExtParameters& extParams,
																		   const char* name, const char* description)
	: TestCaseBase(context, extParams, name, description)
	, m_fs_ids(DE_NULL)
	, m_n_program_objects(0)
	, m_po_ids(DE_NULL)
	, m_tc_ids(DE_NULL)
	, m_te_ids(DE_NULL)
	, m_vs_ids(DE_NULL)
{
	/* Left blank intentionally */
}

/** Deinitializes all ES objects that were created for the test */
void TessellationShaderErrorsTestCaseBase::deinit()
{
	/* Call base class' deinit() */
	TestCaseBase::deinit();

	if (!m_is_tessellation_shader_supported)
	{
		return;
	}

	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Release all shader objects created for the test */
	for (unsigned int n_program_object = 0; n_program_object < m_n_program_objects; ++n_program_object)
	{
		if (m_fs_ids != DE_NULL && m_fs_ids[n_program_object] != 0)
		{
			gl.deleteShader(m_fs_ids[n_program_object]);

			m_fs_ids[n_program_object] = 0;
		}

		if (m_po_ids != DE_NULL && m_po_ids[n_program_object] != 0)
		{
			gl.deleteProgram(m_po_ids[n_program_object]);

			m_po_ids[n_program_object] = 0;
		}

		if (m_tc_ids != DE_NULL && m_tc_ids[n_program_object] != 0)
		{
			gl.deleteShader(m_tc_ids[n_program_object]);

			m_tc_ids[n_program_object] = 0;
		}

		if (m_te_ids != DE_NULL && m_te_ids[n_program_object] != 0)
		{
			gl.deleteShader(m_te_ids[n_program_object]);

			m_te_ids[n_program_object] = 0;
		}

		if (m_vs_ids != DE_NULL && m_vs_ids[n_program_object] != 0)
		{
			gl.deleteShader(m_vs_ids[n_program_object]);

			m_vs_ids[n_program_object] = 0;
		}
	} /* for (all shader objects) */

	/* Release buffers allocated for the test */
	if (m_fs_ids != DE_NULL)
	{
		delete[] m_fs_ids;

		m_fs_ids = DE_NULL;
	}

	if (m_po_ids != DE_NULL)
	{
		delete[] m_po_ids;

		m_po_ids = DE_NULL;
	}

	if (m_tc_ids != DE_NULL)
	{
		delete[] m_tc_ids;

		m_tc_ids = DE_NULL;
	}

	if (m_te_ids != DE_NULL)
	{
		delete[] m_te_ids;

		m_te_ids = DE_NULL;
	}

	if (m_vs_ids != DE_NULL)
	{
		delete[] m_vs_ids;

		m_vs_ids = DE_NULL;
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
tcu::TestNode::IterateResult TessellationShaderErrorsTestCaseBase::iterate()
{
	/* Do not execute if required extensions are not supported. */
	if (!m_is_tessellation_shader_supported)
	{
		throw tcu::NotSupportedError(TESSELLATION_SHADER_EXTENSION_NOT_SUPPORTED);
	}

	/* Retrieve ES entry-points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Create as many program objects as will be needed */
	m_n_program_objects = getAmountOfProgramObjects();
	m_po_ids			= new glw::GLuint[m_n_program_objects];

	memset(m_po_ids, 0, sizeof(glw::GLuint) * m_n_program_objects);

	for (unsigned int n_po = 0; n_po < m_n_program_objects; ++n_po)
	{
		m_po_ids[n_po] = gl.createProgram();

		GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateProgram() failed");
	}

	/* Allocate space for shader IDs */
	bool is_fs_used = isPipelineStageUsed(PIPELINE_STAGE_FRAGMENT);
	bool is_tc_used = isPipelineStageUsed(PIPELINE_STAGE_TESSELLATION_CONTROL);
	bool is_te_used = isPipelineStageUsed(PIPELINE_STAGE_TESSELLATION_EVALUATION);
	bool is_vs_used = isPipelineStageUsed(PIPELINE_STAGE_VERTEX);

	if (is_fs_used)
	{
		m_fs_ids = new glw::GLuint[m_n_program_objects];

		memset(m_fs_ids, 0, sizeof(glw::GLuint) * m_n_program_objects);
	}

	if (is_tc_used)
	{
		m_tc_ids = new glw::GLuint[m_n_program_objects];

		memset(m_tc_ids, 0, sizeof(glw::GLuint) * m_n_program_objects);
	}

	if (is_te_used)
	{
		m_te_ids = new glw::GLuint[m_n_program_objects];

		memset(m_te_ids, 0, sizeof(glw::GLuint) * m_n_program_objects);
	}

	if (is_vs_used)
	{
		m_vs_ids = new glw::GLuint[m_n_program_objects];

		memset(m_vs_ids, 0, sizeof(glw::GLuint) * m_n_program_objects);
	}

	/* Iterate through all program objects the test wants to check */
	for (unsigned int n_po = 0; n_po < m_n_program_objects; ++n_po)
	{
		_linking_result expected_linking_result = getLinkingResult();
		bool			should_try_to_link		= true;

		/* Iterate through all shader types */
		for (int stage = static_cast<int>(PIPELINE_STAGE_FIRST); stage < static_cast<int>(PIPELINE_STAGE_COUNT);
			 stage++)
		{
			if (!isPipelineStageUsed(static_cast<_pipeline_stage>(stage)))
			{
				continue;
			}

			_compilation_result expected_compilation_result = getCompilationResult(static_cast<_pipeline_stage>(stage));
			std::string			so_code;
			glw::GLuint*		so_id_ptr = DE_NULL;
			std::string			so_type;

			switch (static_cast<_pipeline_stage>(stage))
			{
			case PIPELINE_STAGE_FRAGMENT:
			{
				so_code   = getFragmentShaderCode(n_po);
				so_id_ptr = m_fs_ids + n_po;
				so_type   = "fragment";

				break;
			}

			case PIPELINE_STAGE_TESSELLATION_CONTROL:
			{
				so_code   = getTessellationControlShaderCode(n_po);
				so_id_ptr = m_tc_ids + n_po;
				so_type   = "tessellation control";

				break;
			}

			case PIPELINE_STAGE_TESSELLATION_EVALUATION:
			{
				so_code   = getTessellationEvaluationShaderCode(n_po);
				so_id_ptr = m_te_ids + n_po;
				so_type   = "tessellation evaluation";

				break;
			}

			case PIPELINE_STAGE_VERTEX:
			{
				so_code   = getVertexShaderCode(n_po);
				so_id_ptr = m_vs_ids + n_po;
				so_type   = "vertex";

				break;
			}

			default:
			{
				TCU_FAIL("Unrecognized pipeline stage");
			}
			} /* switch (stage) */

			/* Generate the shader object */
			*so_id_ptr = gl.createShader(getGLEnumForPipelineStage(static_cast<_pipeline_stage>(stage)));
			GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateShader() failed");

			/* Assign source code to the object */
			const char* so_unspecialized_code_ptr = so_code.c_str();
			std::string so_specialized_code		  = specializeShader(1, &so_unspecialized_code_ptr);
			const char* so_code_ptr				  = so_specialized_code.c_str();

			gl.shaderSource(*so_id_ptr, 1 /* count */, &so_code_ptr, NULL /* length */);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glShaderSource() failed");

			/* Try to compile the shader object */
			glw::GLint compile_status = GL_FALSE;

			gl.compileShader(*so_id_ptr);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glCompileShader() failed");

			/* Retrieve the compile status and make sure it matches the desired outcome */
			gl.getShaderiv(*so_id_ptr, GL_COMPILE_STATUS, &compile_status);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glGetShaderiv() failed");

			m_context.getTestContext().getLog() << tcu::TestLog::Message << so_type << " shader source:\n"
												<< so_code_ptr << tcu::TestLog::EndMessage;

			glw::GLint length = 0;
			gl.getShaderiv(*so_id_ptr, GL_INFO_LOG_LENGTH, &length);
			if (length > 1)
			{
				std::vector<glw::GLchar> log(length);
				gl.getShaderInfoLog(*so_id_ptr, length, NULL, &log[0]);
				m_context.getTestContext().getLog() << tcu::TestLog::Message << "shader info log\n"
													<< &log[0] << tcu::TestLog::EndMessage;
			}

			switch (expected_compilation_result)
			{
			case COMPILATION_RESULT_CAN_FAIL:
			{
				if (compile_status == GL_FALSE)
				{
					/* OK, this is valid. However, it no longer makes sense to try to
					 * link the program object at this point. */
					should_try_to_link = false;
					m_context.getTestContext().getLog()
						<< tcu::TestLog::Message << "Compilation failed as allowed." << tcu::TestLog::EndMessage;
				}
				else
				{
					/* That's fine. */
					m_context.getTestContext().getLog()
						<< tcu::TestLog::Message << "Compilation passed as allowed." << tcu::TestLog::EndMessage;
				}

				break;
			}

			case COMPILATION_RESULT_MUST_FAIL:
			{
				if (compile_status == GL_TRUE)
				{
					/* Test has failed */
					TCU_FAIL("A shader compiled successfully, even though it should have failed "
							 "to do so");
				}
				else
				{
					/* OK. Mark the program object as non-linkable */
					should_try_to_link = false;
					m_context.getTestContext().getLog()
						<< tcu::TestLog::Message << "Compilation failed as expected." << tcu::TestLog::EndMessage;
				}

				break;
			}

			case COMPILATION_RESULT_MUST_SUCCEED:
			{
				if (compile_status != GL_TRUE)
				{
					/* Test has failed */
					TCU_FAIL("A shader failed to compile, even though it should have succeeded "
							 "to do so");
				}
				else
				{
					/* That's fine. */
					m_context.getTestContext().getLog()
						<< tcu::TestLog::Message << "Compilation successful as expected." << tcu::TestLog::EndMessage;
				}

				break;
			}

			default:
			{
				TCU_FAIL("Unrecognized expected compilation result");
			}
			} /* switch (expected_compilation_result) */

			/* If it still makes sense to do so, attach the shader object to
			 * the test program object */
			if (should_try_to_link)
			{
				gl.attachShader(m_po_ids[n_po], *so_id_ptr);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glAttachShader() failed");
			}
		} /* for (all pipeline stages) */

		/* If it still makes sense, try to link the program object */
		if (should_try_to_link)
		{
			gl.linkProgram(m_po_ids[n_po]);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glLinkProgram() failed");

			/* Retrieve the link status and compare it against the expected linking result */
			glw::GLint link_status = GL_FALSE;

			gl.getProgramiv(m_po_ids[n_po], GL_LINK_STATUS, &link_status);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glGetProgramiv() failed");

			glw::GLint length = 0;
			gl.getProgramiv(m_po_ids[n_po], GL_INFO_LOG_LENGTH, &length);
			if (length > 1)
			{
				std::vector<glw::GLchar> log(length);
				gl.getProgramInfoLog(m_po_ids[n_po], length, NULL, &log[0]);
				m_context.getTestContext().getLog() << tcu::TestLog::Message << "program info log\n"
													<< &log[0] << tcu::TestLog::EndMessage;
			}

			switch (expected_linking_result)
			{
			case LINKING_RESULT_MUST_FAIL:
			{
				if (link_status != GL_FALSE)
				{
					TCU_FAIL("Program object was expected not to link but linking operation succeeded.");
				}
				else
				{
					/* That's OK */
					m_context.getTestContext().getLog()
						<< tcu::TestLog::Message << "Linking failed as expected." << tcu::TestLog::EndMessage;
				}

				break;
			}

			case LINKING_RESULT_MUST_SUCCEED:
			{
				if (link_status != GL_TRUE)
				{
					TCU_FAIL("Program object was expected to link successfully but linking operation failed.");
				}
				else
				{
					/* That's OK */
					m_context.getTestContext().getLog()
						<< tcu::TestLog::Message << "Linking succeeded as expected." << tcu::TestLog::EndMessage;
				}

				break;
			}

			default:
			{
				TCU_FAIL("Unrecognized expected linking result");
			}
			} /* switch (expected_linking_result) */
		}	 /* if (should_try_to_link) */
	}		  /* for (all program objects) */

	/* If this point was reached, the test executed successfully */
	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

	return STOP;
}

/** Retrieves amount of program objects the test that should be linked for
 *  the prupose of the test.
 *
 *  @return As per description.
 */
unsigned int TessellationShaderErrorsTestCaseBase::getAmountOfProgramObjects()
{
	return 1;
}

/** Retrieves source code of fragment shader that should be attached to the test
 *  program object.
 *
 *  @param n_program_object Index of the program object the source code should
 *                          be returned for.
 *
 *  @return Fragment shader source code to be used for user-specified program object.
 */
std::string TessellationShaderErrorsTestCaseBase::getFragmentShaderCode(unsigned int n_program_object)
{
	DE_UNREF(n_program_object);

	return "${VERSION}\n"
		   "\n"
		   "void main()\n"
		   "{\n"
		   "}\n";
}

/** Retrieves GLenum equivalent of a pipeline stage value.
 *
 *  Throws TestError exception if @param stage is invalid.
 *
 *  @param stage Pipeline stage to convert from
 *
 *  @return GL_*_SHADER equivalent of the user-provided value.
 **/
glw::GLenum TessellationShaderErrorsTestCaseBase::getGLEnumForPipelineStage(_pipeline_stage stage)
{
	glw::GLenum result = GL_NONE;

	switch (stage)
	{
	case PIPELINE_STAGE_FRAGMENT:
		result = GL_FRAGMENT_SHADER;
		break;
	case PIPELINE_STAGE_TESSELLATION_CONTROL:
		result = m_glExtTokens.TESS_CONTROL_SHADER;
		break;
	case PIPELINE_STAGE_TESSELLATION_EVALUATION:
		result = m_glExtTokens.TESS_EVALUATION_SHADER;
		break;
	case PIPELINE_STAGE_VERTEX:
		result = GL_VERTEX_SHADER;
		break;

	default:
	{
		TCU_FAIL("Unrecognized pipeline stage");
	}
	}

	return result;
}

/** Retrieves source code of vertex shader that should be attached to the test
 *  program object.
 *
 *  @param n_program_object Index of the program object the source code should
 *                          be returned for.
 *
 *  @return Vertex shader source code to be used for user-specified program object.
 */
std::string TessellationShaderErrorsTestCaseBase::getVertexShaderCode(unsigned int n_program_object)
{
	DE_UNREF(n_program_object);

	return "${VERSION}\n"
		   "\n"
		   "void main()\n"
		   "{\n"
		   "}\n";
}

/** Constructor
 *
 * @param context Test context
 **/
TessellationShaderError1InputBlocks::TessellationShaderError1InputBlocks(Context&			  context,
																		 const ExtParameters& extParams)
	: TessellationShaderErrorsTestCaseBase(context, extParams, "tc_non_arrayed_per_vertex_input_blocks",
										   "Tries to use non-arrayed per-vertex input blocks"
										   "in a tessellation control shader")
{
	/* Left blank on purpose */
}

/** Determines what compilation result is anticipated for each of the pipeline stages.
 *
 *  @param pipeline_stage Pipeline stage to return compilation result for.
 *
 *  @return Requested compilation result.
 **/
TessellationShaderErrorsTestCaseBase::_compilation_result TessellationShaderError1InputBlocks::getCompilationResult(
	_pipeline_stage pipeline_stage)
{
	/* Tessellation control shader is allowed to fail to compile,
	 * shaders for all other stages should compile successfully
	 *
	 * NOTE: Vertex shader compilation can fail if underlying implementation does not support
	 *       GL_EXT_shader_io_blocks.
	 **/
	switch (pipeline_stage)
	{
	case PIPELINE_STAGE_VERTEX:
		return COMPILATION_RESULT_CAN_FAIL;
	case PIPELINE_STAGE_TESSELLATION_CONTROL:
		return COMPILATION_RESULT_CAN_FAIL;
	default:
		return COMPILATION_RESULT_MUST_SUCCEED;
	}
}

/** Determines what linking result is anticipated for all program objects created by the test.
 *
 *  @return Expected linking result.
 **/
TessellationShaderErrorsTestCaseBase::_linking_result TessellationShaderError1InputBlocks::getLinkingResult()
{
	return LINKING_RESULT_MUST_FAIL;
}

/** Retrieves source code of tessellation control shader that should be attached to the test
 *  program object.
 *
 *  @param n_program_object Index of the program object the source code should
 *                          be returned for.
 *
 *  @return Tessellation control shader source code to be used for user-specified program object.
 */
std::string TessellationShaderError1InputBlocks::getTessellationControlShaderCode(unsigned int n_program_object)
{
	DE_UNREF(n_program_object);

	return "${VERSION}\n"
		   "\n"
		   "${TESSELLATION_SHADER_REQUIRE}\n"
		   "\n"
		   "layout (vertices=4) out;\n"
		   "\n"
		   /* Invalid input block declaration */
		   "in IN_TC\n"
		   "{\n"
		   "    vec4 test_block_field;\n"
		   "} test_block;\n"
		   "\n"
		   "void main()\n"
		   "{\n"
		   "    gl_out[gl_InvocationID].gl_Position = test_block.test_block_field;\n"
		   "    gl_TessLevelOuter[0]                = 1.0;\n"
		   "    gl_TessLevelOuter[1]                = 1.0;\n"
		   "    gl_TessLevelOuter[2]                = 1.0;\n"
		   "    gl_TessLevelOuter[3]                = 1.0;\n"
		   "    gl_TessLevelInner[0]                = 1.0;\n"
		   "    gl_TessLevelInner[1]                = 1.0;\n"
		   "}\n";
}

/** Retrieves source code of tessellation evaluation shader that should be attached to the test
 *  program object.
 *
 *  @param n_program_object Index of the program object the source code should
 *                          be returned for.
 *
 *  @return Tessellation evaluation shader source code to be used for user-specified program object.
 */
std::string TessellationShaderError1InputBlocks::getTessellationEvaluationShaderCode(unsigned int n_program_object)
{
	DE_UNREF(n_program_object);

	return "${VERSION}\n"
		   "\n"
		   "${TESSELLATION_SHADER_REQUIRE}\n"
		   "\n"
		   "layout (quads) in;\n"
		   "\n"
		   "void main()\n"
		   "{\n"
		   "    gl_Position = gl_in[0].gl_Position;\n"
		   "}\n";
}

/** Retrieves source code of vertex shader that should be attached to the test
 *  program object.
 *
 *  @param n_program_object Index of the program object the source code should
 *                          be returned for.
 *
 *  @return Vertex shader source code to be used for user-specified program object.
 */
std::string TessellationShaderError1InputBlocks::getVertexShaderCode(unsigned int n_program_object)
{
	DE_UNREF(n_program_object);

	return "${VERSION}\n"
		   "\n"
		   "${SHADER_IO_BLOCKS_REQUIRE}\n"
		   "\n"
		   "out IN_TC\n"
		   "{\n"
		   "    vec4 test_block_field;\n"
		   "} test_block;\n"
		   "\n"
		   "void main()\n"
		   "{\n"
		   "    test_block.test_block_field = vec4(1.0, 2.0, 3.0, gl_VertexID);\n"
		   "}\n";
}

/** Tells whether given pipeline stage should be used for the purpose of the test
 *  (for all program objects).
 *
 *  @param Stage to query.
 *
 *  @return True if a shader object implementing the stage should be attached to
 *          test program object;
 *          False otherwise.
 **/
bool TessellationShaderError1InputBlocks::isPipelineStageUsed(_pipeline_stage stage)
{
	DE_UNREF(stage);

	/* All stages used */
	return true;
}

/** Constructor
 *
 * @param context Test context
 **/
TessellationShaderError1InputVariables::TessellationShaderError1InputVariables(Context&				context,
																			   const ExtParameters& extParams)
	: TessellationShaderErrorsTestCaseBase(context, extParams, "tc_non_arrayed_per_vertex_input_variables",
										   "Tries to use non-arrayed per-vertex input variables"
										   "in a tessellation control shader")
{
	/* Left blank on purpose */
}

/** Determines what compilation result is anticipated for each of the pipeline stages.
 *
 *  @param pipeline_stage Pipeline stage to return compilation result for.
 *
 *  @return Requested compilation result.
 **/
TessellationShaderErrorsTestCaseBase::_compilation_result TessellationShaderError1InputVariables::getCompilationResult(
	_pipeline_stage pipeline_stage)
{
	/* Tessellation control shader is allowed to fail to compile,
	 * shaders for all other stages should compile successfully */
	switch (pipeline_stage)
	{
	case PIPELINE_STAGE_TESSELLATION_CONTROL:
		return COMPILATION_RESULT_CAN_FAIL;
	default:
		return COMPILATION_RESULT_MUST_SUCCEED;
	}
}

/** Determines what linking result is anticipated for all program objects created by the test.
 *
 *  @return Expected linking result.
 **/
TessellationShaderErrorsTestCaseBase::_linking_result TessellationShaderError1InputVariables::getLinkingResult()
{
	return LINKING_RESULT_MUST_FAIL;
}

/** Retrieves source code of tessellation control shader that should be attached to the test
 *  program object.
 *
 *  @param n_program_object Index of the program object the source code should
 *                          be returned for.
 *
 *  @return Tessellation control shader source code to be used for user-specified program object.
 */
std::string TessellationShaderError1InputVariables::getTessellationControlShaderCode(unsigned int n_program_object)
{
	DE_UNREF(n_program_object);

	return "${VERSION}\n"
		   "\n"
		   "${TESSELLATION_SHADER_REQUIRE}\n"
		   "\n"
		   "layout (vertices=4) out;\n"
		   "\n"
		   /* Invalid input declaration */
		   "in vec4 test_field;\n"
		   "\n"
		   "void main()\n"
		   "{\n"
		   "    gl_out[gl_InvocationID].gl_Position = test_field;\n"
		   "    gl_TessLevelOuter[0]                = 1.0;\n"
		   "    gl_TessLevelOuter[1]                = 1.0;\n"
		   "    gl_TessLevelOuter[2]                = 1.0;\n"
		   "    gl_TessLevelOuter[3]                = 1.0;\n"
		   "    gl_TessLevelInner[0]                = 1.0;\n"
		   "    gl_TessLevelInner[1]                = 1.0;\n"
		   "}\n";
}

/** Retrieves source code of tessellation evaluation shader that should be attached to the test
 *  program object.
 *
 *  @param n_program_object Index of the program object the source code should
 *                          be returned for.
 *
 *  @return Tessellation evaluation shader source code to be used for user-specified program object.
 */
std::string TessellationShaderError1InputVariables::getTessellationEvaluationShaderCode(unsigned int n_program_object)
{
	DE_UNREF(n_program_object);

	return "${VERSION}\n"
		   "\n"
		   "${TESSELLATION_SHADER_REQUIRE}\n"
		   "\n"
		   "layout (quads) in;\n"
		   "\n"
		   "void main()\n"
		   "{\n"
		   "    gl_Position = gl_in[0].gl_Position;\n"
		   "}\n";
}

/** Retrieves source code of vertex shader that should be attached to the test
 *  program object.
 *
 *  @param n_program_object Index of the program object the source code should
 *                          be returned for.
 *
 *  @return Vertex shader source code to be used for user-specified program object.
 */
std::string TessellationShaderError1InputVariables::getVertexShaderCode(unsigned int n_program_object)
{
	DE_UNREF(n_program_object);

	return "${VERSION}\n"
		   "\n"
		   "out vec4 test_field;\n"
		   "\n"
		   "void main()\n"
		   "{\n"
		   "    test_field = vec4(1.0, 2.0, 3.0, gl_VertexID);\n"
		   "}\n";
}

/** Tells whether given pipeline stage should be used for the purpose of the test
 *  (for all program objects).
 *
 *  @param Stage to query.
 *
 *  @return True if a shader object implementing the stage should be attached to
 *          test program object;
 *          False otherwise.
 **/
bool TessellationShaderError1InputVariables::isPipelineStageUsed(_pipeline_stage stage)
{
	DE_UNREF(stage);

	/* All stages used */
	return true;
}

/** Constructor
 *
 * @param context Test context
 **/
TessellationShaderError2OutputBlocks::TessellationShaderError2OutputBlocks(Context&				context,
																		   const ExtParameters& extParams)
	: TessellationShaderErrorsTestCaseBase(context, extParams, "tc_non_arrayed_per_vertex_output_blocks",
										   "Tries to use non-arrayed per-vertex output blocks"
										   "in a tessellation control shader")
{
	/* Left blank on purpose */
}

/** Determines what compilation result is anticipated for each of the pipeline stages.
 *
 *  @param pipeline_stage Pipeline stage to return compilation result for.
 *
 *  @return Requested compilation result.
 **/
TessellationShaderErrorsTestCaseBase::_compilation_result TessellationShaderError2OutputBlocks::getCompilationResult(
	_pipeline_stage pipeline_stage)
{
	/* Tessellation control shader is allowed to fail to compile,
	 * shaders for all other stages should compile successfully */
	switch (pipeline_stage)
	{
	case PIPELINE_STAGE_TESSELLATION_CONTROL:
		return COMPILATION_RESULT_CAN_FAIL;
	default:
		return COMPILATION_RESULT_MUST_SUCCEED;
	}
}

/** Determines what linking result is anticipated for all program objects created by the test.
 *
 *  @return Expected linking result.
 **/
TessellationShaderErrorsTestCaseBase::_linking_result TessellationShaderError2OutputBlocks::getLinkingResult()
{
	return LINKING_RESULT_MUST_FAIL;
}

/** Retrieves source code of tessellation control shader that should be attached to the test
 *  program object.
 *
 *  @param n_program_object Index of the program object the source code should
 *                          be returned for.
 *
 *  @return Tessellation control shader source code to be used for user-specified program object.
 */
std::string TessellationShaderError2OutputBlocks::getTessellationControlShaderCode(unsigned int n_program_object)
{
	DE_UNREF(n_program_object);

	return "${VERSION}\n"
		   "\n"
		   "${TESSELLATION_SHADER_REQUIRE}\n"
		   "\n"
		   "layout (vertices=4) out;\n"
		   "\n"
		   /* Invalid output block declaration */
		   "out OUT_TC\n"
		   "{\n"
		   "    vec4 test_block_field;\n"
		   "} test_block;\n"
		   "\n"
		   "void main()\n"
		   "{\n"
		   "    gl_out[gl_InvocationID].gl_Position = test_block.test_block_field;\n"
		   "    test_block.test_block_field         = gl_in[gl_InvocationID].gl_Position;\n"
		   "    gl_TessLevelOuter[0]                = 1.0;\n"
		   "    gl_TessLevelOuter[1]                = 1.0;\n"
		   "    gl_TessLevelOuter[2]                = 1.0;\n"
		   "    gl_TessLevelOuter[3]                = 1.0;\n"
		   "    gl_TessLevelInner[0]                = 1.0;\n"
		   "    gl_TessLevelInner[1]                = 1.0;\n"
		   "}\n";
}

/** Retrieves source code of tessellation evaluation shader that should be attached to the test
 *  program object.
 *
 *  @param n_program_object Index of the program object the source code should
 *                          be returned for.
 *
 *  @return Tessellation evaluation shader source code to be used for user-specified program object.
 */
std::string TessellationShaderError2OutputBlocks::getTessellationEvaluationShaderCode(unsigned int n_program_object)
{
	DE_UNREF(n_program_object);

	return "${VERSION}\n"
		   "\n"
		   "${TESSELLATION_SHADER_REQUIRE}\n"
		   "\n"
		   "layout (quads) in;\n"
		   "\n"
		   "void main()\n"
		   "{\n"
		   "    gl_Position = gl_in[0].gl_Position;\n"
		   "}\n";
}

/** Tells whether given pipeline stage should be used for the purpose of the test
 *  (for all program objects).
 *
 *  @param Stage to query.
 *
 *  @return True if a shader object implementing the stage should be attached to
 *          test program object;
 *          False otherwise.
 **/
bool TessellationShaderError2OutputBlocks::isPipelineStageUsed(_pipeline_stage stage)
{
	DE_UNREF(stage);

	/* All stages used */
	return true;
}

/** Constructor
 *
 * @param context Test context
 **/
TessellationShaderError2OutputVariables::TessellationShaderError2OutputVariables(Context&			  context,
																				 const ExtParameters& extParams)
	: TessellationShaderErrorsTestCaseBase(context, extParams, "tc_non_arrayed_per_vertex_output_variabless",
										   "Tries to use non-arrayed per-vertex output variables"
										   "in a tessellation control shader")
{
	/* Left blank on purpose */
}

/** Determines what compilation result is anticipated for each of the pipeline stages.
 *
 *  @param pipeline_stage Pipeline stage to return compilation result for.
 *
 *  @return Requested compilation result.
 **/
TessellationShaderErrorsTestCaseBase::_compilation_result TessellationShaderError2OutputVariables::getCompilationResult(
	_pipeline_stage pipeline_stage)
{
	/* Tessellation control shader is allowed to fail to compile,
	 * shaders for all other stages should compile successfully */
	switch (pipeline_stage)
	{
	case PIPELINE_STAGE_TESSELLATION_CONTROL:
		return COMPILATION_RESULT_CAN_FAIL;
	default:
		return COMPILATION_RESULT_MUST_SUCCEED;
	}
}

/** Determines what linking result is anticipated for all program objects created by the test.
 *
 *  @return Expected linking result.
 **/
TessellationShaderErrorsTestCaseBase::_linking_result TessellationShaderError2OutputVariables::getLinkingResult()
{
	return LINKING_RESULT_MUST_FAIL;
}

/** Retrieves source code of tessellation control shader that should be attached to the test
 *  program object.
 *
 *  @param n_program_object Index of the program object the source code should
 *                          be returned for.
 *
 *  @return Tessellation control shader source code to be used for user-specified program object.
 */
std::string TessellationShaderError2OutputVariables::getTessellationControlShaderCode(unsigned int n_program_object)
{
	DE_UNREF(n_program_object);

	return "${VERSION}\n"
		   "\n"
		   "${TESSELLATION_SHADER_REQUIRE}\n"
		   "\n"
		   "layout (vertices=4) out;\n"
		   "\n"
		   /* Invalid output declaration */
		   "out vec4 test_field;\n"
		   "\n"
		   "void main()\n"
		   "{\n"
		   "    gl_out[gl_InvocationID].gl_Position = vec4(2.0);\n"
		   "    test_field                          = vec4(3.0);\n"
		   "    gl_TessLevelOuter[0]                = 1.0;\n"
		   "    gl_TessLevelOuter[1]                = 1.0;\n"
		   "    gl_TessLevelOuter[2]                = 1.0;\n"
		   "    gl_TessLevelOuter[3]                = 1.0;\n"
		   "    gl_TessLevelInner[0]                = 1.0;\n"
		   "    gl_TessLevelInner[1]                = 1.0;\n"
		   "}\n";
}

/** Retrieves source code of tessellation evaluation shader that should be attached to the test
 *  program object.
 *
 *  @param n_program_object Index of the program object the source code should
 *                          be returned for.
 *
 *  @return Tessellation evaluation shader source code to be used for user-specified program object.
 */
std::string TessellationShaderError2OutputVariables::getTessellationEvaluationShaderCode(unsigned int n_program_object)
{
	DE_UNREF(n_program_object);

	return "${VERSION}\n"
		   "\n"
		   "${TESSELLATION_SHADER_REQUIRE}\n"
		   "\n"
		   "layout (quads) in;\n"
		   "\n"
		   "void main()\n"
		   "{\n"
		   "    gl_Position = gl_in[0].gl_Position;\n"
		   "}\n";
}

/** Tells whether given pipeline stage should be used for the purpose of the test
 *  (for all program objects).
 *
 *  @param Stage to query.
 *
 *  @return True if a shader object implementing the stage should be attached to
 *          test program object;
 *          False otherwise.
 **/
bool TessellationShaderError2OutputVariables::isPipelineStageUsed(_pipeline_stage stage)
{
	DE_UNREF(stage);

	/* All stages used */
	return true;
}

/** Constructor
 *
 * @param context Test context
 **/
TessellationShaderError3InputBlocks::TessellationShaderError3InputBlocks(Context&			  context,
																		 const ExtParameters& extParams)
	: TessellationShaderErrorsTestCaseBase(context, extParams, "te_non_arrayed_per_vertex_input_blocks",
										   "Tries to use non-arrayed per-vertex input blocks"
										   "in a tessellation evaluation shader")
{
	/* Left blank on purpose */
}

/** Determines what compilation result is anticipated for each of the pipeline stages.
 *
 *  @param pipeline_stage Pipeline stage to return compilation result for.
 *
 *  @return Requested compilation result.
 **/
TessellationShaderErrorsTestCaseBase::_compilation_result TessellationShaderError3InputBlocks::getCompilationResult(
	_pipeline_stage pipeline_stage)
{
	/* Tessellation evaluation shader is allowed to fail to compile,
	 * shaders for all other stages should compile successfully */
	switch (pipeline_stage)
	{
	case PIPELINE_STAGE_TESSELLATION_EVALUATION:
		return COMPILATION_RESULT_CAN_FAIL;
	default:
		return COMPILATION_RESULT_MUST_SUCCEED;
	}
}

/** Determines what linking result is anticipated for all program objects created by the test.
 *
 *  @return Expected linking result.
 **/
TessellationShaderErrorsTestCaseBase::_linking_result TessellationShaderError3InputBlocks::getLinkingResult()
{
	return LINKING_RESULT_MUST_FAIL;
}

/** Retrieves source code of tessellation control shader that should be attached to the test
 *  program object.
 *
 *  @param n_program_object Index of the program object the source code should
 *                          be returned for.
 *
 *  @return Tessellation control shader source code to be used for user-specified program object.
 */
std::string TessellationShaderError3InputBlocks::getTessellationControlShaderCode(unsigned int n_program_object)
{
	DE_UNREF(n_program_object);

	return "${VERSION}\n"
		   "\n"
		   "${TESSELLATION_SHADER_REQUIRE}\n"
		   "\n"
		   "layout (vertices=4) out;\n"
		   "\n"
		   "out IN_TE\n"
		   "{\n"
		   "    vec4 test_block_field;\n"
		   "} test_block[];\n"
		   "\n"
		   "void main()\n"
		   "{\n"
		   "    gl_out           [gl_InvocationID].gl_Position      = gl_in[gl_InvocationID].gl_Position;\n"
		   "    test_block       [gl_InvocationID].test_block_field = vec4(2.0);\n"
		   "    gl_TessLevelOuter[0]                                = 1.0;\n"
		   "    gl_TessLevelOuter[1]                                = 1.0;\n"
		   "    gl_TessLevelOuter[2]                                = 1.0;\n"
		   "    gl_TessLevelOuter[3]                                = 1.0;\n"
		   "    gl_TessLevelInner[0]                                = 1.0;\n"
		   "    gl_TessLevelInner[1]                                = 1.0;\n"
		   "}\n";
}

/** Retrieves source code of tessellation evaluation shader that should be attached to the test
 *  program object.
 *
 *  @param n_program_object Index of the program object the source code should
 *                          be returned for.
 *
 *  @return Tessellation evaluation shader source code to be used for user-specified program object.
 */
std::string TessellationShaderError3InputBlocks::getTessellationEvaluationShaderCode(unsigned int n_program_object)
{
	DE_UNREF(n_program_object);

	return "${VERSION}\n"
		   "\n"
		   "${TESSELLATION_SHADER_REQUIRE}\n"
		   "\n"
		   "layout (quads) in;\n"
		   "\n"
		   /* Invalid input block declaration */
		   "in IN_TE\n"
		   "{\n"
		   "    vec4 test_block_field;\n"
		   "} test_block;\n"
		   "\n"
		   "void main()\n"
		   "{\n"
		   "    gl_Position = gl_in[0].gl_Position * test_block.test_block_field;\n"
		   "}\n";
}

/** Tells whether given pipeline stage should be used for the purpose of the test
 *  (for all program objects).
 *
 *  @param Stage to query.
 *
 *  @return True if a shader object implementing the stage should be attached to
 *          test program object;
 *          False otherwise.
 **/
bool TessellationShaderError3InputBlocks::isPipelineStageUsed(_pipeline_stage stage)
{
	DE_UNREF(stage);

	/* All stages used */
	return true;
}

/** Constructor
 *
 * @param context Test context
 **/
TessellationShaderError3InputVariables::TessellationShaderError3InputVariables(Context&				context,
																			   const ExtParameters& extParams)
	: TessellationShaderErrorsTestCaseBase(context, extParams, "te_non_arrayed_per_vertex_input_variables",
										   "Tries to use non-arrayed per-vertex input variables "
										   "in a tessellation evaluation shader")
{
	/* Left blank on purpose */
}

/** Determines what compilation result is anticipated for each of the pipeline stages.
 *
 *  @param pipeline_stage Pipeline stage to return compilation result for.
 *
 *  @return Requested compilation result.
 **/
TessellationShaderErrorsTestCaseBase::_compilation_result TessellationShaderError3InputVariables::getCompilationResult(
	_pipeline_stage pipeline_stage)
{
	/* Tessellation evaluation shader is allowed to fail to compile,
	 * shaders for all other stages should compile successfully */
	switch (pipeline_stage)
	{
	case PIPELINE_STAGE_TESSELLATION_EVALUATION:
		return COMPILATION_RESULT_CAN_FAIL;
	default:
		return COMPILATION_RESULT_MUST_SUCCEED;
	}
}

/** Determines what linking result is anticipated for all program objects created by the test.
 *
 *  @return Expected linking result.
 **/
TessellationShaderErrorsTestCaseBase::_linking_result TessellationShaderError3InputVariables::getLinkingResult()
{
	return LINKING_RESULT_MUST_FAIL;
}

/** Retrieves source code of tessellation control shader that should be attached to the test
 *  program object.
 *
 *  @param n_program_object Index of the program object the source code should
 *                          be returned for.
 *
 *  @return Tessellation control shader source code to be used for user-specified program object.
 */
std::string TessellationShaderError3InputVariables::getTessellationControlShaderCode(unsigned int n_program_object)
{
	DE_UNREF(n_program_object);

	return "${VERSION}\n"
		   "\n"
		   "${TESSELLATION_SHADER_REQUIRE}\n"
		   "\n"
		   "layout (vertices=4) out;\n"
		   "\n"
		   "out vec4 test_field[];\n"
		   "\n"
		   "void main()\n"
		   "{\n"
		   "    gl_out           [gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;\n"
		   "    test_field       [gl_InvocationID]             = vec4(4.0);\n"
		   "    gl_TessLevelOuter[0]                           = 1.0;\n"
		   "    gl_TessLevelOuter[1]                           = 1.0;\n"
		   "    gl_TessLevelOuter[2]                           = 1.0;\n"
		   "    gl_TessLevelOuter[3]                           = 1.0;\n"
		   "    gl_TessLevelInner[0]                           = 1.0;\n"
		   "    gl_TessLevelInner[1]                           = 1.0;\n"
		   "}\n";
}

/** Retrieves source code of tessellation evaluation shader that should be attached to the test
 *  program object.
 *
 *  @param n_program_object Index of the program object the source code should
 *                          be returned for.
 *
 *  @return Tessellation evaluation shader source code to be used for user-specified program object.
 */
std::string TessellationShaderError3InputVariables::getTessellationEvaluationShaderCode(unsigned int n_program_object)
{
	DE_UNREF(n_program_object);

	return "${VERSION}\n"
		   "\n"
		   "${TESSELLATION_SHADER_REQUIRE}\n"
		   "\n"
		   "layout (quads) in;\n"
		   "\n"
		   /* Invalid input declaration */
		   "in vec4 test_field;\n"
		   "\n"
		   "void main()\n"
		   "{\n"
		   "    gl_Position = gl_in[0].gl_Position * test_field;\n"
		   "}\n";
}

/** Tells whether given pipeline stage should be used for the purpose of the test
 *  (for all program objects).
 *
 *  @param Stage to query.
 *
 *  @return True if a shader object implementing the stage should be attached to
 *          test program object;
 *          False otherwise.
 **/
bool TessellationShaderError3InputVariables::isPipelineStageUsed(_pipeline_stage stage)
{
	DE_UNREF(stage);

	/* All stages used */
	return true;
}

/** Constructor
 *
 * @param context Test context
 **/
TessellationShaderError4InputBlocks::TessellationShaderError4InputBlocks(Context&			  context,
																		 const ExtParameters& extParams)
	: TessellationShaderErrorsTestCaseBase(context, extParams, "tc_invalid_array_size_used_for_input_blocks",
										   "Tries to use invalid array size when defining input blocks "
										   "in a tessellation control shader")
{
	/* Left blank on purpose */
}

/** Determines what compilation result is anticipated for each of the pipeline stages.
 *
 *  @param pipeline_stage Pipeline stage to return compilation result for.
 *
 *  @return Requested compilation result.
 **/
TessellationShaderErrorsTestCaseBase::_compilation_result TessellationShaderError4InputBlocks::getCompilationResult(
	_pipeline_stage pipeline_stage)
{
	/* Tessellation control shader is allowed to fail to compile,
	 * shaders for all other stages should compile successfully */
	switch (pipeline_stage)
	{
	case PIPELINE_STAGE_TESSELLATION_CONTROL:
		return COMPILATION_RESULT_CAN_FAIL;
	default:
		return COMPILATION_RESULT_MUST_SUCCEED;
	}
}

/** Determines what linking result is anticipated for all program objects created by the test.
 *
 *  @return Expected linking result.
 **/
TessellationShaderErrorsTestCaseBase::_linking_result TessellationShaderError4InputBlocks::getLinkingResult()
{
	return LINKING_RESULT_MUST_FAIL;
}

/** Retrieves source code of tessellation control shader that should be attached to the test
 *  program object.
 *
 *  @param n_program_object Index of the program object the source code should
 *                          be returned for.
 *
 *  @return Tessellation control shader source code to be used for user-specified program object.
 */
std::string TessellationShaderError4InputBlocks::getTessellationControlShaderCode(unsigned int n_program_object)
{
	DE_UNREF(n_program_object);

	return "${VERSION}\n"
		   "\n"
		   "${TESSELLATION_SHADER_REQUIRE}\n"
		   "\n"
		   "layout (vertices=4) out;\n"
		   "\n"
		   /* Invalid input block size */
		   "in IN_TC\n"
		   "{\n"
		   "    vec4 input_block_input;\n"
		   "} input_block[11];\n"
		   "\n"
		   "void main()\n"
		   "{\n"
		   "    gl_out[gl_InvocationID].gl_Position = input_block[gl_InvocationID].input_block_input;\n"
		   "    gl_TessLevelOuter[0]                = 1.0;\n"
		   "    gl_TessLevelOuter[1]                = 1.0;\n"
		   "}\n";
}

/** Retrieves source code of tessellation evaluation shader that should be attached to the test
 *  program object.
 *
 *  @param n_program_object Index of the program object the source code should
 *                          be returned for.
 *
 *  @return Tessellation evaluation shader source code to be used for user-specified program object.
 */
std::string TessellationShaderError4InputBlocks::getTessellationEvaluationShaderCode(unsigned int n_program_object)
{
	DE_UNREF(n_program_object);

	return "${VERSION}\n"
		   "\n"
		   "${TESSELLATION_SHADER_REQUIRE}\n"
		   "\n"
		   "layout(isolines) in;\n"
		   "\n"
		   "void main()\n"
		   "{\n"
		   "    gl_Position = gl_in[0].gl_Position;\n"
		   "}\n";
}

/** Tells whether given pipeline stage should be used for the purpose of the test
 *  (for all program objects).
 *
 *  @param Stage to query.
 *
 *  @return True if a shader object implementing the stage should be attached to
 *          test program object;
 *          False otherwise.
 **/
bool TessellationShaderError4InputBlocks::isPipelineStageUsed(_pipeline_stage stage)
{
	DE_UNREF(stage);

	/* All stages used */
	return true;
}

/** Constructor
 *
 * @param context Test context
 **/
TessellationShaderError4InputVariables::TessellationShaderError4InputVariables(Context&				context,
																			   const ExtParameters& extParams)
	: TessellationShaderErrorsTestCaseBase(context, extParams, "tc_invalid_array_size_used_for_input_variables",
										   "Tries to use invalid array size when defining input variables"
										   " in a tessellation control shader")
{
	/* Left blank on purpose */
}

/** Determines what compilation result is anticipated for each of the pipeline stages.
 *
 *  @param pipeline_stage Pipeline stage to return compilation result for.
 *
 *  @return Requested compilation result.
 **/
TessellationShaderErrorsTestCaseBase::_compilation_result TessellationShaderError4InputVariables::getCompilationResult(
	_pipeline_stage pipeline_stage)
{
	/* Tessellation control shader is allowed to fail to compile,
	 * shaders for all other stages should compile successfully */
	switch (pipeline_stage)
	{
	case PIPELINE_STAGE_TESSELLATION_CONTROL:
		return COMPILATION_RESULT_CAN_FAIL;
	default:
		return COMPILATION_RESULT_MUST_SUCCEED;
	}
}

/** Determines what linking result is anticipated for all program objects created by the test.
 *
 *  @return Expected linking result.
 **/
TessellationShaderErrorsTestCaseBase::_linking_result TessellationShaderError4InputVariables::getLinkingResult()
{
	return LINKING_RESULT_MUST_FAIL;
}

/** Retrieves source code of tessellation control shader that should be attached to the test
 *  program object.
 *
 *  @param n_program_object Index of the program object the source code should
 *                          be returned for.
 *
 *  @return Tessellation control shader source code to be used for user-specified program object.
 */
std::string TessellationShaderError4InputVariables::getTessellationControlShaderCode(unsigned int n_program_object)
{
	DE_UNREF(n_program_object);

	return "${VERSION}\n"
		   "\n"
		   "${TESSELLATION_SHADER_REQUIRE}\n"
		   "\n"
		   "layout (vertices=4) out;\n"
		   "\n"
		   /* Invalid array size */
		   "in vec4 test_input[11];\n"
		   "\n"
		   "void main()\n"
		   "{\n"
		   "    gl_out[gl_InvocationID].gl_Position = test_input[gl_InvocationID];\n"
		   "    gl_TessLevelOuter[0]                = 1.0;\n"
		   "    gl_TessLevelOuter[1]                = 1.0;\n"
		   "}\n";
}

/** Retrieves source code of tessellation evaluation shader that should be attached to the test
 *  program object.
 *
 *  @param n_program_object Index of the program object the source code should
 *                          be returned for.
 *
 *  @return Tessellation evaluation shader source code to be used for user-specified program object.
 */
std::string TessellationShaderError4InputVariables::getTessellationEvaluationShaderCode(unsigned int n_program_object)
{
	DE_UNREF(n_program_object);

	return "${VERSION}\n"
		   "\n"
		   "${TESSELLATION_SHADER_REQUIRE}\n"
		   "\n"
		   "layout(isolines) in;\n"
		   "\n"
		   "void main()\n"
		   "{\n"
		   "    gl_Position = gl_in[0].gl_Position;\n"
		   "}\n";
}

/** Tells whether given pipeline stage should be used for the purpose of the test
 *  (for all program objects).
 *
 *  @param Stage to query.
 *
 *  @return True if a shader object implementing the stage should be attached to
 *          test program object;
 *          False otherwise.
 **/
bool TessellationShaderError4InputVariables::isPipelineStageUsed(_pipeline_stage stage)
{
	DE_UNREF(stage);

	/* All stages used */
	return true;
}

/** Constructor
 *
 * @param context Test context
 **/
TessellationShaderError5InputBlocks::TessellationShaderError5InputBlocks(Context&			  context,
																		 const ExtParameters& extParams)
	: TessellationShaderErrorsTestCaseBase(context, extParams, "te_invalid_array_size_used_for_input_blocks",
										   "Tries to use invalid array size when defining input "
										   "blocks in a tessellation evaluation shader")
{
	/* Left blank on purpose */
}

/** Determines what compilation result is anticipated for each of the pipeline stages.
 *
 *  @param pipeline_stage Pipeline stage to return compilation result for.
 *
 *  @return Requested compilation result.
 **/
TessellationShaderErrorsTestCaseBase::_compilation_result TessellationShaderError5InputBlocks::getCompilationResult(
	_pipeline_stage pipeline_stage)
{
	/* Tessellation evaluation shader is allowed to fail to compile,
	 * shaders for all other stages should compile successfully */
	switch (pipeline_stage)
	{
	case PIPELINE_STAGE_TESSELLATION_EVALUATION:
		return COMPILATION_RESULT_CAN_FAIL;
	default:
		return COMPILATION_RESULT_MUST_SUCCEED;
	}
}

/** Determines what linking result is anticipated for all program objects created by the test.
 *
 *  @return Expected linking result.
 **/
TessellationShaderErrorsTestCaseBase::_linking_result TessellationShaderError5InputBlocks::getLinkingResult()
{
	return LINKING_RESULT_MUST_FAIL;
}

/** Retrieves source code of tessellation control shader that should be attached to the test
 *  program object.
 *
 *  @param n_program_object Index of the program object the source code should
 *                          be returned for.
 *
 *  @return Tessellation control shader source code to be used for user-specified program object.
 */
std::string TessellationShaderError5InputBlocks::getTessellationControlShaderCode(unsigned int n_program_object)
{
	DE_UNREF(n_program_object);

	return "${VERSION}\n"
		   "\n"
		   "${TESSELLATION_SHADER_REQUIRE}\n"
		   "\n"
		   "layout (vertices=4) out;\n"
		   "\n"
		   "void main()\n"
		   "{\n"
		   "    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;\n"
		   "    gl_TessLevelOuter[0]                = 1.0;\n"
		   "    gl_TessLevelOuter[1]                = 1.0;\n"
		   "}\n";
}

/** Retrieves source code of tessellation evaluation shader that should be attached to the test
 *  program object.
 *
 *  @param n_program_object Index of the program object the source code should
 *                          be returned for.
 *
 *  @return Tessellation evaluation shader source code to be used for user-specified program object.
 */
std::string TessellationShaderError5InputBlocks::getTessellationEvaluationShaderCode(unsigned int n_program_object)
{
	DE_UNREF(n_program_object);

	return "${VERSION}\n"
		   "\n"
		   "${TESSELLATION_SHADER_REQUIRE}\n"
		   "\n"
		   "layout(isolines) in;\n"
		   "\n"
		   /* Invalid input block size */
		   "in IN_TC\n"
		   "{\n"
		   "    vec4 input_block_input;\n"
		   "} input_block[11];\n"
		   "\n"
		   "void main()\n"
		   "{\n"
		   "    gl_Position = gl_in[0].gl_Position * input_block[0].input_block_input;\n"
		   "}\n";
}

/** Tells whether given pipeline stage should be used for the purpose of the test
 *  (for all program objects).
 *
 *  @param Stage to query.
 *
 *  @return True if a shader object implementing the stage should be attached to
 *          test program object;
 *          False otherwise.
 **/
bool TessellationShaderError5InputBlocks::isPipelineStageUsed(_pipeline_stage stage)
{
	DE_UNREF(stage);

	/* All stages used */
	return true;
}

/** Constructor
 *
 * @param context Test context
 **/
TessellationShaderError5InputVariables::TessellationShaderError5InputVariables(Context&				context,
																			   const ExtParameters& extParams)
	: TessellationShaderErrorsTestCaseBase(context, extParams, "te_invalid_array_size_used_for_input_variables",
										   "Tries to use invalid array size when defining input "
										   "variables in a tessellation evaluation shader")
{
	/* Left blank on purpose */
}

/** Determines what compilation result is anticipated for each of the pipeline stages.
 *
 *  @param pipeline_stage Pipeline stage to return compilation result for.
 *
 *  @return Requested compilation result.
 **/
TessellationShaderErrorsTestCaseBase::_compilation_result TessellationShaderError5InputVariables::getCompilationResult(
	_pipeline_stage pipeline_stage)
{
	/* Tessellation evaluation shader is allowed to fail to compile,
	 * shaders for all other stages should compile successfully */
	switch (pipeline_stage)
	{
	case PIPELINE_STAGE_TESSELLATION_EVALUATION:
		return COMPILATION_RESULT_CAN_FAIL;
	default:
		return COMPILATION_RESULT_MUST_SUCCEED;
	}
}

/** Determines what linking result is anticipated for all program objects created by the test.
 *
 *  @return Expected linking result.
 **/
TessellationShaderErrorsTestCaseBase::_linking_result TessellationShaderError5InputVariables::getLinkingResult()
{
	return LINKING_RESULT_MUST_FAIL;
}

/** Retrieves source code of tessellation control shader that should be attached to the test
 *  program object.
 *
 *  @param n_program_object Index of the program object the source code should
 *                          be returned for.
 *
 *  @return Tessellation control shader source code to be used for user-specified program object.
 */
std::string TessellationShaderError5InputVariables::getTessellationControlShaderCode(unsigned int n_program_object)
{
	DE_UNREF(n_program_object);

	return "${VERSION}\n"
		   "\n"
		   "${TESSELLATION_SHADER_REQUIRE}\n"
		   "\n"
		   "layout (vertices=4) out;\n"
		   "\n"
		   "void main()\n"
		   "{\n"
		   "    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;\n"
		   "    gl_TessLevelOuter[0]                = 1.0;\n"
		   "    gl_TessLevelOuter[1]                = 1.0;\n"
		   "}\n";
}

/** Retrieves source code of tessellation evaluation shader that should be attached to the test
 *  program object.
 *
 *  @param n_program_object Index of the program object the source code should
 *                          be returned for.
 *
 *  @return Tessellation evaluation shader source code to be used for user-specified program object.
 */
std::string TessellationShaderError5InputVariables::getTessellationEvaluationShaderCode(unsigned int n_program_object)
{
	DE_UNREF(n_program_object);

	return "${VERSION}\n"
		   "\n"
		   "${TESSELLATION_SHADER_REQUIRE}\n"
		   "\n"
		   "layout(isolines) in;\n"
		   "\n"
		   /* Invalid array size */
		   "in vec4 test_input[11];\n"
		   "\n"
		   "void main()\n"
		   "{\n"
		   "    gl_Position = gl_in[0].gl_Position * test_input[0];\n"
		   "}\n";
}

/** Tells whether given pipeline stage should be used for the purpose of the test
 *  (for all program objects).
 *
 *  @param Stage to query.
 *
 *  @return True if a shader object implementing the stage should be attached to
 *          test program object;
 *          False otherwise.
 **/
bool TessellationShaderError5InputVariables::isPipelineStageUsed(_pipeline_stage stage)
{
	DE_UNREF(stage);

	/* All stages used */
	return true;
}

/** Constructor
 *
 * @param context Test context
 **/
TessellationShaderError6::TessellationShaderError6(Context& context, const ExtParameters& extParams)
	: TessellationShaderErrorsTestCaseBase(context, extParams, "tc_invalid_output_patch_vertex_count",
										   "Tries to use invalid output patch vertex count in"
										   " a tessellation control shader")
{
	/* Left blank on purpose */
}

/** Retrieves amount of program objects the test that should be linked for
 *  the prupose of the test.
 *
 *  @return As per description.
 */
unsigned int TessellationShaderError6::getAmountOfProgramObjects()
{
	return 2;
}

/** Determines what compilation result is anticipated for each of the pipeline stages.
 *
 *  @param pipeline_stage Pipeline stage to return compilation result for.
 *
 *  @return Requested compilation result.
 **/
TessellationShaderErrorsTestCaseBase::_compilation_result TessellationShaderError6::getCompilationResult(
	_pipeline_stage pipeline_stage)
{
	/* Tessellation control shader is allowed to fail to compile */
	switch (pipeline_stage)
	{
	case PIPELINE_STAGE_TESSELLATION_CONTROL:
		return COMPILATION_RESULT_CAN_FAIL;
	default:
		return COMPILATION_RESULT_MUST_SUCCEED;
	}
}

/** Determines what linking result is anticipated for all program objects created by the test.
 *
 *  @return Expected linking result.
 **/
TessellationShaderErrorsTestCaseBase::_linking_result TessellationShaderError6::getLinkingResult()
{
	return LINKING_RESULT_MUST_FAIL;
}

/** Retrieves source code of tessellation control shader that should be attached to the test
 *  program object.
 *
 *  @param n_program_object Index of the program object the source code should
 *                          be returned for.
 *
 *  @return Tessellation control shader source code to be used for user-specified program object.
 */
std::string TessellationShaderError6::getTessellationControlShaderCode(unsigned int n_program_object)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();
	std::stringstream	 result;
	const char*			  tc_code_preamble = "${VERSION}\n"
								   "\n"
								   "${TESSELLATION_SHADER_REQUIRE}\n"
								   "\n";
	const char* tc_code_body_excl_layout_qualifiers = "\n"
													  "in vec4 test_input[];\n"
													  "\n"
													  "void main()\n"
													  "{\n"
													  "    gl_out[gl_InvocationID].gl_Position = test_input[0];\n"
													  "    gl_TessLevelOuter[0]                = 1.0;\n"
													  "    gl_TessLevelOuter[1]                = 1.0;\n"
													  "}\n";

	/* Prepare the line with layout qualifier */
	std::stringstream tc_code_layout_qualifier_sstream;
	std::string		  tc_code_layout_qualifier_string;

	if (n_program_object == 0)
	{
		tc_code_layout_qualifier_string = "layout (vertices = 0) out;\n";
	}
	else
	{
		/* Retrieve GL_MAX_PATCH_VERTICES_EXT value first */
		glw::GLint gl_max_patch_vertices_value = 0;

		gl.getIntegerv(m_glExtTokens.MAX_PATCH_VERTICES, &gl_max_patch_vertices_value);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv() failed for GL_MAX_PATCH_VERTICES_EXT pname");

		/* Construct the string */
		tc_code_layout_qualifier_sstream << "layout (vertices = " << (gl_max_patch_vertices_value + 1) << ") out;\n";

		tc_code_layout_qualifier_string = tc_code_layout_qualifier_sstream.str();
	}

	result << tc_code_preamble << tc_code_layout_qualifier_string << tc_code_body_excl_layout_qualifiers;

	return result.str();
}

/** Retrieves source code of tessellation evaluation shader that should be attached to the test
 *  program object.
 *
 *  @param n_program_object Index of the program object the source code should
 *                          be returned for.
 *
 *  @return Tessellation evaluation shader source code to be used for user-specified program object.
 */
std::string TessellationShaderError6::getTessellationEvaluationShaderCode(unsigned int n_program_object)
{
	DE_UNREF(n_program_object);

	return "${VERSION}\n"
		   "\n"
		   "${TESSELLATION_SHADER_REQUIRE}\n"
		   "\n"
		   "layout(isolines) in;\n"
		   "\n"
		   "void main()\n"
		   "{\n"
		   "    gl_Position = gl_in[0].gl_Position;\n"
		   "}\n";
}

/** Tells whether given pipeline stage should be used for the purpose of the test
 *  (for all program objects).
 *
 *  @param Stage to query.
 *
 *  @return True if a shader object implementing the stage should be attached to
 *          test program object;
 *          False otherwise.
 **/
bool TessellationShaderError6::isPipelineStageUsed(_pipeline_stage stage)
{
	DE_UNREF(stage);

	/* All stages used */
	return true;
}

/** Constructor
 *
 * @param context Test context
 **/
TessellationShaderError7::TessellationShaderError7(Context& context, const ExtParameters& extParams)
	: TessellationShaderErrorsTestCaseBase(context, extParams,
										   "tc_invalid_write_operation_at_non_gl_invocation_id_index",
										   "Tries to write to a per-vertex output variable at index"
										   " which is not equal to gl_InvocationID")
{
	/* Left blank on purpose */
}

/** Determines what compilation result is anticipated for each of the pipeline stages.
 *
 *  @param pipeline_stage Pipeline stage to return compilation result for.
 *
 *  @return Requested compilation result.
 **/
TessellationShaderErrorsTestCaseBase::_compilation_result TessellationShaderError7::getCompilationResult(
	_pipeline_stage pipeline_stage)
{
	/* TC stage should _may_ fail to compile (but linking *must* fail) */
	return (pipeline_stage == PIPELINE_STAGE_TESSELLATION_CONTROL) ? COMPILATION_RESULT_CAN_FAIL :
																	 COMPILATION_RESULT_MUST_SUCCEED;
}

/** Determines what linking result is anticipated for all program objects created by the test.
 *
 *  @return Expected linking result.
 **/
TessellationShaderErrorsTestCaseBase::_linking_result TessellationShaderError7::getLinkingResult()
{
	return LINKING_RESULT_MUST_FAIL;
}

/** Retrieves source code of tessellation control shader that should be attached to the test
 *  program object.
 *
 *  @param n_program_object Index of the program object the source code should
 *                          be returned for.
 *
 *  @return Tessellation control shader source code to be used for user-specified program object.
 */
std::string TessellationShaderError7::getTessellationControlShaderCode(unsigned int n_program_object)
{
	DE_UNREF(n_program_object);

	return "${VERSION}\n"
		   "\n"
		   "${TESSELLATION_SHADER_REQUIRE}\n"
		   "\n"
		   "layout (vertices=4) out;\n"
		   "\n"
		   "out vec4 test[];\n"
		   "\n"
		   "void main()\n"
		   "{\n"
		   "    gl_out[2].gl_Position = gl_in[0].gl_Position;\n"
		   "    test[2]               = gl_in[1].gl_Position;\n"
		   "}\n";
}

/** Retrieves source code of tessellation evaluation shader that should be attached to the test
 *  program object.
 *
 *  @param n_program_object Index of the program object the source code should
 *                          be returned for.
 *
 *  @return Tessellation evaluation shader source code to be used for user-specified program object.
 */
std::string TessellationShaderError7::getTessellationEvaluationShaderCode(unsigned int n_program_object)
{
	DE_UNREF(n_program_object);

	return "${VERSION}\n"
		   "\n"
		   "${TESSELLATION_SHADER_REQUIRE}\n"
		   "\n"
		   "layout (quads) in;\n"
		   "\n"
		   "void main()\n"
		   "{\n"
		   "    gl_Position = gl_in[0].gl_Position;\n"
		   "}\n";
}

/** Tells whether given pipeline stage should be used for the purpose of the test
 *  (for all program objects).
 *
 *  @param Stage to query.
 *
 *  @return True if a shader object implementing the stage should be attached to
 *          test program object;
 *          False otherwise.
 **/
bool TessellationShaderError7::isPipelineStageUsed(_pipeline_stage stage)
{
	DE_UNREF(stage);

	/* All stages used */
	return true;
}

/** Constructor
 *
 * @param context Test context
 **/
TessellationShaderError8::TessellationShaderError8(Context& context, const ExtParameters& extParams)
	: TessellationShaderErrorsTestCaseBase(context, extParams, "tc_invalid_input_per_patch_attribute_definition",
										   "Tries to define input per-patch attributes in "
										   "a tessellation control shader")
{
	/* Left blank on purpose */
}

/** Determines what compilation result is anticipated for each of the pipeline stages.
 *
 *  @param pipeline_stage Pipeline stage to return compilation result for.
 *
 *  @return Requested compilation result.
 **/
TessellationShaderErrorsTestCaseBase::_compilation_result TessellationShaderError8::getCompilationResult(
	_pipeline_stage pipeline_stage)
{
	/* TC stage should never compile, all stages are out of scope */
	return (pipeline_stage == PIPELINE_STAGE_TESSELLATION_CONTROL) ? COMPILATION_RESULT_MUST_FAIL :
																	 COMPILATION_RESULT_UNKNOWN;
}

/** Determines what linking result is anticipated for all program objects created by the test.
 *
 *  @return Expected linking result.
 **/
TessellationShaderErrorsTestCaseBase::_linking_result TessellationShaderError8::getLinkingResult()
{
	return LINKING_RESULT_MUST_FAIL;
}

/** Retrieves source code of tessellation control shader that should be attached to the test
 *  program object.
 *
 *  @param n_program_object Index of the program object the source code should
 *                          be returned for.
 *
 *  @return Tessellation control shader source code to be used for user-specified program object.
 */
std::string TessellationShaderError8::getTessellationControlShaderCode(unsigned int n_program_object)
{
	DE_UNREF(n_program_object);

	return "${VERSION}\n"
		   "\n"
		   "${TESSELLATION_SHADER_REQUIRE}\n"
		   "\n"
		   "layout (vertices=4) out;\n"
		   "\n"
		   "patch in vec4 test;\n"
		   "\n"
		   "void main()\n"
		   "{\n"
		   "    gl_out[gl_InvocationID].gl_Position = test;\n"
		   "}\n";
}

/** Retrieves source code of tessellation evaluation shader that should be attached to the test
 *  program object.
 *
 *  @param n_program_object Index of the program object the source code should
 *                          be returned for.
 *
 *  @return Tessellation evaluation shader source code to be used for user-specified program object.
 */
std::string TessellationShaderError8::getTessellationEvaluationShaderCode(unsigned int n_program_object)
{
	DE_UNREF(n_program_object);

	/* This function should never be called */
	DE_ASSERT(false);

	return "";
}

/** Tells whether given pipeline stage should be used for the purpose of the test
 *  (for all program objects).
 *
 *  @param Stage to query.
 *
 *  @return True if a shader object implementing the stage should be attached to
 *          test program object;
 *          False otherwise.
 **/
bool TessellationShaderError8::isPipelineStageUsed(_pipeline_stage stage)
{
	/* Only TC stage is used */
	return (stage == PIPELINE_STAGE_TESSELLATION_CONTROL);
}

/** Constructor
 *
 * @param context Test context
 **/
TessellationShaderError9::TessellationShaderError9(Context& context, const ExtParameters& extParams)
	: TessellationShaderErrorsTestCaseBase(context, extParams, "te_invalid_output_per_patch_attribute_definition",
										   "Tries to define output per-patch attributes in "
										   "a tessellation evaluation shader")
{
	/* Left blank on purpose */
}

/** Determines what compilation result is anticipated for each of the pipeline stages.
 *
 *  @param pipeline_stage Pipeline stage to return compilation result for.
 *
 *  @return Requested compilation result.
 **/
TessellationShaderErrorsTestCaseBase::_compilation_result TessellationShaderError9::getCompilationResult(
	_pipeline_stage pipeline_stage)
{
	/* TE stage should never compile, all stages are out of scope */
	return (pipeline_stage == PIPELINE_STAGE_TESSELLATION_EVALUATION) ? COMPILATION_RESULT_MUST_FAIL :
																		COMPILATION_RESULT_UNKNOWN;
}

/** Determines what linking result is anticipated for all program objects created by the test.
 *
 *  @return Expected linking result.
 **/
TessellationShaderErrorsTestCaseBase::_linking_result TessellationShaderError9::getLinkingResult()
{
	return LINKING_RESULT_MUST_FAIL;
}

/** Retrieves source code of tessellation control shader that should be attached to the test
 *  program object.
 *
 *  @param n_program_object Index of the program object the source code should
 *                          be returned for.
 *
 *  @return Tessellation control shader source code to be used for user-specified program object.
 */
std::string TessellationShaderError9::getTessellationControlShaderCode(unsigned int n_program_object)
{
	DE_UNREF(n_program_object);

	/* This function should never be called */
	DE_ASSERT(false);

	return "";
}

/** Retrieves source code of tessellation evaluation shader that should be attached to the test
 *  program object.
 *
 *  @param n_program_object Index of the program object the source code should
 *                          be returned for.
 *
 *  @return Tessellation evaluation shader source code to be used for user-specified program object.
 */
std::string TessellationShaderError9::getTessellationEvaluationShaderCode(unsigned int n_program_object)
{
	DE_UNREF(n_program_object);

	return "${VERSION}\n"
		   "\n"
		   "${TESSELLATION_SHADER_REQUIRE}\n"
		   "\n"
		   "layout (isolines) in;\n"
		   "\n"
		   "patch out vec4 test;\n"
		   "\n"
		   "void main()\n"
		   "{\n"
		   "    gl_Position = vec4(1.0, 0.0, 0.0, 0.0);\n"
		   "    test        = vec4(0.0, 1.0, 0.0, 0.0);\n"
		   "}\n";
}

/** Tells whether given pipeline stage should be used for the purpose of the test
 *  (for all program objects).
 *
 *  @param Stage to query.
 *
 *  @return True if a shader object implementing the stage should be attached to
 *          test program object;
 *          False otherwise.
 **/
bool TessellationShaderError9::isPipelineStageUsed(_pipeline_stage stage)
{
	/* Only TE stage is used */
	return (stage == PIPELINE_STAGE_TESSELLATION_EVALUATION);
}

/** Constructor
 *
 * @param context Test context
 **/
TessellationShaderError10::TessellationShaderError10(Context& context, const ExtParameters& extParams)
	: TessellationShaderErrorsTestCaseBase(context, extParams, "tc_non_matching_variable_declarations",
										   "Tries to define variables of different types/qualifications"
										   " in tessellation control and tessellation evaluation shaders")
{
	/* Left blank on purpose */
}

/** Determines what compilation result is anticipated for each of the pipeline stages.
 *
 *  @param pipeline_stage Pipeline stage to return compilation result for.
 *
 *  @return Requested compilation result.
 **/
TessellationShaderErrorsTestCaseBase::_compilation_result TessellationShaderError10::getCompilationResult(
	_pipeline_stage pipeline_stage)
{
	DE_UNREF(pipeline_stage);
	/* All stages should compile */
	return COMPILATION_RESULT_MUST_SUCCEED;
}

/** Determines what linking result is anticipated for all program objects created by the test.
 *
 *  @return Expected linking result.
 **/
TessellationShaderErrorsTestCaseBase::_linking_result TessellationShaderError10::getLinkingResult()
{
	return LINKING_RESULT_MUST_FAIL;
}

/** Retrieves source code of tessellation control shader that should be attached to the test
 *  program object.
 *
 *  @param n_program_object Index of the program object the source code should
 *                          be returned for.
 *
 *  @return Tessellation control shader source code to be used for user-specified program object.
 */
std::string TessellationShaderError10::getTessellationControlShaderCode(unsigned int n_program_object)
{
	DE_UNREF(n_program_object);

	return "${VERSION}\n"
		   "\n"
		   "${TESSELLATION_SHADER_REQUIRE}\n"
		   "\n"
		   "layout (vertices=4) out;\n"
		   "\n"
		   "patch out float test;\n"
		   "patch out vec4  test2;\n"
		   "\n"
		   "void main()\n"
		   "{\n"
		   "    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;\n"
		   "    gl_TessLevelOuter[0]                = 1.0;\n"
		   "    gl_TessLevelOuter[1]                = 1.0;\n"
		   "    test                                = 1.0;\n"
		   "    test2                               = vec4(0.1, 0.2, 0.3, 0.4);\n"
		   "}\n";
}

/** Retrieves source code of tessellation evaluation shader that should be attached to the test
 *  program object.
 *
 *  @param n_program_object Index of the program object the source code should
 *                          be returned for.
 *
 *  @return Tessellation evaluation shader source code to be used for user-specified program object.
 */
std::string TessellationShaderError10::getTessellationEvaluationShaderCode(unsigned int n_program_object)
{
	DE_UNREF(n_program_object);

	return "${VERSION}\n"
		   "\n"
		   "${TESSELLATION_SHADER_REQUIRE}\n"
		   "\n"
		   "layout(isolines) in;\n"
		   "\n"
		   "patch in uint  test;\n"
		   "patch in float test2;\n"
		   "\n"
		   "void main()\n"
		   "{\n"
		   "    gl_Position = gl_in[0].gl_Position * float(int(test)) * vec4(test2);\n"
		   "}\n";
}

/** Tells whether given pipeline stage should be used for the purpose of the test
 *  (for all program objects).
 *
 *  @param Stage to query.
 *
 *  @return True if a shader object implementing the stage should be attached to
 *          test program object;
 *          False otherwise.
 **/
bool TessellationShaderError10::isPipelineStageUsed(_pipeline_stage stage)
{
	DE_UNREF(stage);

	return true;
}

/** Constructor
 *
 * @param context Test context
 **/
TessellationShaderError11::TessellationShaderError11(Context& context, const ExtParameters& extParams)
	: TessellationShaderErrorsTestCaseBase(context, extParams, "te_lacking_primitive_mode_declaration",
										   "Tries to link a tessellation evaluation shader"
										   " without a primitive mode declaration")
{
	/* Left blank on purpose */
}

/** Determines what compilation result is anticipated for each of the pipeline stages.
 *
 *  @param pipeline_stage Pipeline stage to return compilation result for.
 *
 *  @return Requested compilation result.
 **/
TessellationShaderErrorsTestCaseBase::_compilation_result TessellationShaderError11::getCompilationResult(
	_pipeline_stage pipeline_stage)
{
	DE_UNREF(pipeline_stage);

	/* All stages should compile */
	return COMPILATION_RESULT_MUST_SUCCEED;
}

/** Determines what linking result is anticipated for all program objects created by the test.
 *
 *  @return Expected linking result.
 **/
TessellationShaderErrorsTestCaseBase::_linking_result TessellationShaderError11::getLinkingResult()
{
	return LINKING_RESULT_MUST_FAIL;
}

/** Retrieves source code of tessellation control shader that should be attached to the test
 *  program object.
 *
 *  @param n_program_object Index of the program object the source code should
 *                          be returned for.
 *
 *  @return Tessellation control shader source code to be used for user-specified program object.
 */
std::string TessellationShaderError11::getTessellationControlShaderCode(unsigned int n_program_object)
{
	DE_UNREF(n_program_object);

	return "${VERSION}\n"
		   "\n"
		   "${TESSELLATION_SHADER_REQUIRE}\n"
		   "\n"
		   "layout (vertices=4) out;\n"
		   "\n"
		   "void main()\n"
		   "{\n"
		   "    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;\n"
		   "    gl_TessLevelInner[0]                = 1.0;\n"
		   "    gl_TessLevelOuter[0]                = 1.0;\n"
		   "}\n";
}

/** Retrieves source code of tessellation evaluation shader that should be attached to the test
 *  program object.
 *
 *  @param n_program_object Index of the program object the source code should
 *                          be returned for.
 *
 *  @return Tessellation evaluation shader source code to be used for user-specified program object.
 */
std::string TessellationShaderError11::getTessellationEvaluationShaderCode(unsigned int n_program_object)
{
	DE_UNREF(n_program_object);

	return "${VERSION}\n"
		   "\n"
		   "${TESSELLATION_SHADER_REQUIRE}\n"
		   "\n"
		   "void main()\n"
		   "{\n"
		   "    gl_Position = gl_in[0].gl_Position;\n"
		   "}\n";
}

/** Tells whether given pipeline stage should be used for the purpose of the test
 *  (for all program objects).
 *
 *  @param Stage to query.
 *
 *  @return True if a shader object implementing the stage should be attached to
 *          test program object;
 *          False otherwise.
 **/
bool TessellationShaderError11::isPipelineStageUsed(_pipeline_stage stage)
{
	DE_UNREF(stage);

	return true;
}

/** Constructor
 *
 * @param context Test context
 **/
TessellationShaderError12::TessellationShaderError12(Context& context, const ExtParameters& extParams)
	: TessellationShaderErrorsTestCaseBase(context, extParams, "te_accessing_glTessCoord_as_array",
										   "Tries to access gl_TessCoord as if it was an array")
{
	/* Left blank on purpose */
}

/** Determines what compilation result is anticipated for each of the pipeline stages.
 *
 *  @param pipeline_stage Pipeline stage to return compilation result for.
 *
 *  @return Requested compilation result.
 **/
TessellationShaderErrorsTestCaseBase::_compilation_result TessellationShaderError12::getCompilationResult(
	_pipeline_stage pipeline_stage)
{
	/* It is OK for TE stage to fail to compile. Other stages must compile successfully */
	/* All stages should compile */
	return (pipeline_stage == PIPELINE_STAGE_TESSELLATION_EVALUATION) ? COMPILATION_RESULT_CAN_FAIL :
																		COMPILATION_RESULT_MUST_SUCCEED;
}

/** Determines what linking result is anticipated for all program objects created by the test.
 *
 *  @return Expected linking result.
 **/
TessellationShaderErrorsTestCaseBase::_linking_result TessellationShaderError12::getLinkingResult()
{
	return LINKING_RESULT_MUST_FAIL;
}

/** Retrieves source code of tessellation control shader that should be attached to the test
 *  program object.
 *
 *  @param n_program_object Index of the program object the source code should
 *                          be returned for.
 *
 *  @return Tessellation control shader source code to be used for user-specified program object.
 */
std::string TessellationShaderError12::getTessellationControlShaderCode(unsigned int n_program_object)
{
	DE_UNREF(n_program_object);

	return "${VERSION}\n"
		   "\n"
		   "${TESSELLATION_SHADER_REQUIRE}\n"
		   "\n"
		   "layout (vertices=4) out;\n"
		   "\n"
		   "void main()\n"
		   "{\n"
		   "    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;\n"
		   "    gl_TessLevelOuter[0]                = 1.0;\n"
		   "    gl_TessLevelOuter[1]                = 1.0;\n"
		   "}\n";
}

/** Retrieves source code of tessellation evaluation shader that should be attached to the test
 *  program object.
 *
 *  @param n_program_object Index of the program object the source code should
 *                          be returned for.
 *
 *  @return Tessellation evaluation shader source code to be used for user-specified program object.
 */
std::string TessellationShaderError12::getTessellationEvaluationShaderCode(unsigned int n_program_object)
{
	DE_UNREF(n_program_object);

	return "${VERSION}\n"
		   "\n"
		   "${TESSELLATION_SHADER_REQUIRE}\n"
		   "\n"
		   "layout(isolines) in;\n"
		   "\n"
		   "out vec3 test;\n"
		   "\n"
		   "void main()\n"
		   "{\n"
		   "    test = gl_TessCoord[0].xyz;\n"
		   "}\n";
}

/** Tells whether given pipeline stage should be used for the purpose of the test
 *  (for all program objects).
 *
 *  @param Stage to query.
 *
 *  @return True if a shader object implementing the stage should be attached to
 *          test program object;
 *          False otherwise.
 **/
bool TessellationShaderError12::isPipelineStageUsed(_pipeline_stage stage)
{
	DE_UNREF(stage);

	return true;
}

/** Constructor
 *
 * @param context Test context
 **/
TessellationShaderError13::TessellationShaderError13(Context& context, const ExtParameters& extParams)
	: TessellationShaderErrorsTestCaseBase(context, extParams, "te_accessing_glTessCoord_as_gl_in_member",
										   "Tries to access gl_TessCoord as if it was a gl_in[] member")
{
	/* Left blank on purpose */
}

/** Determines what compilation result is anticipated for each of the pipeline stages.
 *
 *  @param pipeline_stage Pipeline stage to return compilation result for.
 *
 *  @return Requested compilation result.
 **/
TessellationShaderErrorsTestCaseBase::_compilation_result TessellationShaderError13::getCompilationResult(
	_pipeline_stage pipeline_stage)
{
	/* It is OK for TE stage to fail to compile. Other stages must compile successfully */
	/* All stages should compile */
	return (pipeline_stage == PIPELINE_STAGE_TESSELLATION_EVALUATION) ? COMPILATION_RESULT_CAN_FAIL :
																		COMPILATION_RESULT_MUST_SUCCEED;
}

/** Determines what linking result is anticipated for all program objects created by the test.
 *
 *  @return Expected linking result.
 **/
TessellationShaderErrorsTestCaseBase::_linking_result TessellationShaderError13::getLinkingResult()
{
	return LINKING_RESULT_MUST_FAIL;
}

/** Retrieves source code of tessellation control shader that should be attached to the test
 *  program object.
 *
 *  @param n_program_object Index of the program object the source code should
 *                          be returned for.
 *
 *  @return Tessellation control shader source code to be used for user-specified program object.
 */
std::string TessellationShaderError13::getTessellationControlShaderCode(unsigned int n_program_object)
{
	DE_UNREF(n_program_object);

	return "${VERSION}\n"
		   "\n"
		   "${TESSELLATION_SHADER_REQUIRE}\n"
		   "\n"
		   "layout (vertices=4) out;\n"
		   "\n"
		   "void main()\n"
		   "{\n"
		   "    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;\n"
		   "    gl_TessLevelOuter[0]                = 1.0;\n"
		   "    gl_TessLevelOuter[1]                = 1.0;\n"
		   "}\n";
}

/** Retrieves source code of tessellation evaluation shader that should be attached to the test
 *  program object.
 *
 *  @param n_program_object Index of the program object the source code should
 *                          be returned for.
 *
 *  @return Tessellation evaluation shader source code to be used for user-specified program object.
 */
std::string TessellationShaderError13::getTessellationEvaluationShaderCode(unsigned int n_program_object)
{
	DE_UNREF(n_program_object);

	return "${VERSION}\n"
		   "\n"
		   "${TESSELLATION_SHADER_REQUIRE}\n"
		   "\n"
		   "layout(isolines) in;\n"
		   "\n"
		   "out vec3 test;\n"
		   "\n"
		   "void main()\n"
		   "{\n"
		   "    test = gl_in[0].gl_TessCoord;\n"
		   "}\n";
}

/** Tells whether given pipeline stage should be used for the purpose of the test
 *  (for all program objects).
 *
 *  @param Stage to query.
 *
 *  @return True if a shader object implementing the stage should be attached to
 *          test program object;
 *          False otherwise.
 **/
bool TessellationShaderError13::isPipelineStageUsed(_pipeline_stage stage)
{
	DE_UNREF(stage);

	return true;
}

} /* namespace glcts */
