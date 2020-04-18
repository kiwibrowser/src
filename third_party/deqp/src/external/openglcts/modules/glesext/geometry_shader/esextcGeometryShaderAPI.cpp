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
#include "esextcGeometryShaderAPI.hpp"

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

/* createShaderProgramv conformance test shaders */
const char* GeometryShaderCreateShaderProgramvTest::fs_code = "${VERSION}\n"
															  "\n"
															  "precision highp float;\n"
															  "\n"
															  "out vec4 result;\n"
															  "\n"
															  "void main()\n"
															  "{\n"
															  "    result = vec4(0.0, 1.0, 0.0, 0.0);\n"
															  "}\n";

const char* GeometryShaderCreateShaderProgramvTest::gs_code = "${VERSION}\n"
															  "${GEOMETRY_SHADER_REQUIRE}\n"
															  "\n"
															  "layout (points)                           in;\n"
															  "layout (triangle_strip, max_vertices = 4) out;\n"
															  "\n"
															  "${OUT_PER_VERTEX_DECL}"
															  "\n"
															  "void main()\n"
															  "{\n"
															  "    gl_Position = vec4(-1.0, -1.0, 0.0, 1.0);\n"
															  "    EmitVertex();\n"
															  "\n"
															  "    gl_Position = vec4(-1.0, 1.0, 0.0, 1.0);\n"
															  "    EmitVertex();\n"
															  "\n"
															  "    gl_Position = vec4(1.0, -1.0, 0.0, 1.0);\n"
															  "    EmitVertex();\n"
															  "\n"
															  "    gl_Position = vec4(1.0, 1.0, 0.0, 1.0);\n"
															  "    EmitVertex();\n"
															  "    EndPrimitive();\n"
															  "}\n";

const char* GeometryShaderCreateShaderProgramvTest::vs_code = "${VERSION}\n"
															  "\n"
															  "${OUT_PER_VERTEX_DECL}"
															  "\n"
															  "void main()\n"
															  "{\n"
															  "    gl_Position = vec4(-10.0, -10.0, -10.0, 0.0);\n"
															  "}\n";

const unsigned int GeometryShaderCreateShaderProgramvTest::m_to_height = 4;
const unsigned int GeometryShaderCreateShaderProgramvTest::m_to_width  = 4;

/** Constructor
 *
 * @param context       Test context
 * @param extParams     Not used.
 * @param name          Test case's name
 * @param description   Test case's description
 **/
GeometryShaderCreateShaderProgramvTest::GeometryShaderCreateShaderProgramvTest(Context&				context,
																			   const ExtParameters& extParams,
																			   const char*			name,
																			   const char*			description)
	: TestCaseBase(context, extParams, name, description)
	, m_fbo_id(0)
	, m_fs_po_id(0)
	, m_gs_po_id(0)
	, m_pipeline_object_id(0)
	, m_to_id(0)
	, m_vao_id(0)
	, m_vs_po_id(0)
{
}

/** Deinitializes GLES objects created during the test. */
void GeometryShaderCreateShaderProgramvTest::deinit()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (m_fbo_id != 0)
	{
		gl.deleteFramebuffers(1, &m_fbo_id);

		m_fbo_id = 0;
	}

	if (m_fs_po_id != 0)
	{
		gl.deleteProgram(m_fs_po_id);

		m_fs_po_id = 0;
	}

	if (m_gs_po_id != 0)
	{
		gl.deleteProgram(m_gs_po_id);

		m_gs_po_id = 0;
	}

	if (m_pipeline_object_id != 0)
	{
		gl.deleteProgramPipelines(1, &m_pipeline_object_id);

		m_pipeline_object_id = 0;
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

	if (m_vs_po_id != 0)
	{
		gl.deleteProgram(m_vs_po_id);

		m_vs_po_id = 0;
	}

	/* Release base class */
	TestCaseBase::deinit();
}

/** Initializes a framebuffer object used by the conformance test. */
void GeometryShaderCreateShaderProgramvTest::initFBO()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Generate a FBO */
	gl.genFramebuffers(1, &m_fbo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenFramebuffers() call failed.");

	/* Generate a TO */
	gl.genTextures(1, &m_to_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenTextures() call failed.");

	/* Set the TO up */
	gl.bindTexture(GL_TEXTURE_2D, m_to_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture() call failed.");

	gl.texStorage2D(GL_TEXTURE_2D, 1, /* levels */
					GL_RGBA8, m_to_width, m_to_height);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTexStorage2D() call failed.");

	/* Set up the FBO */
	gl.bindFramebuffer(GL_FRAMEBUFFER, m_fbo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffer() call failed.");

	gl.framebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_to_id, 0); /* level */
	GLU_EXPECT_NO_ERROR(gl.getError(), "glFramebufferTexture2D() call failed.");

	/* Set up the viewport */
	gl.viewport(0, /* x */
				0, /* y */
				m_to_width, m_to_height);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glViewport() call failed.");
}

/* Initializes a pipeline object used by the conformance test */
void GeometryShaderCreateShaderProgramvTest::initPipelineObject()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	DE_ASSERT(m_fs_po_id != 0 && m_gs_po_id != 0 && m_vs_po_id != 0);

	gl.genProgramPipelines(1, &m_pipeline_object_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenProgramPipelines() call failed.");

	gl.useProgramStages(m_pipeline_object_id, GL_FRAGMENT_SHADER_BIT, m_fs_po_id);
	gl.useProgramStages(m_pipeline_object_id, GL_GEOMETRY_SHADER_BIT, m_gs_po_id);
	gl.useProgramStages(m_pipeline_object_id, GL_VERTEX_SHADER_BIT, m_vs_po_id);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgramStages() call(s) failed.");
}

/** Executes the test.
 *
 *  Sets the test result to QP_TEST_RESULT_FAIL if the test failed, QP_TEST_RESULT_PASS otherwise.
 *  @return STOP if the test has finished, CONTINUE to indicate iterate should be called once again.
 *  Note the function throws exception should an error occur!
 **/
tcu::TestNode::IterateResult GeometryShaderCreateShaderProgramvTest::iterate()
{
	const glw::Functions& gl		  = m_context.getRenderContext().getFunctions();
	const unsigned int	n_so_po_ids = 3;
	bool				  result	  = true;
	glw::GLuint			  so_po_ids[n_so_po_ids];

	/* This test should only run if EXT_geometry_shader is supported. */
	if (!m_is_geometry_shader_extension_supported)
	{
		throw tcu::NotSupportedError(GEOMETRY_SHADER_EXTENSION_NOT_SUPPORTED, "", __FILE__, __LINE__);
	}

	/* Initialize off-screen rendering */
	initFBO();

	/* Form shader sources */
	std::string fs_specialized_code = specializeShader(1,
													   /* parts */ &fs_code);
	const char* fs_specialized_code_raw = fs_specialized_code.c_str();
	std::string gs_specialized_code		= specializeShader(1,
													   /* parts */ &gs_code);
	const char* gs_specialized_code_raw = gs_specialized_code.c_str();
	std::string vs_specialized_code		= specializeShader(1,
													   /* parts */ &vs_code);
	const char* vs_specialized_code_raw = vs_specialized_code.c_str();

	/* Try to create an invalid geometry shader program first */
	glw::GLint link_status = GL_TRUE;

	m_gs_po_id = gl.createShaderProgramv(GL_GEOMETRY_SHADER, 1, /* count */
										 &gs_code);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateShaderProgramv() call failed.");

	if (m_gs_po_id == 0)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "glCreateShaderProgramv() call returned 0."
						   << tcu::TestLog::EndMessage;

		result = false;
		goto end;
	}

	gl.getProgramiv(m_gs_po_id, GL_LINK_STATUS, &link_status);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetProgramiv() call failed.");

	if (link_status != GL_FALSE)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "An invalid shader program was linked successfully."
						   << tcu::TestLog::EndMessage;

		result = false;
		goto end;
	}

	gl.deleteProgram(m_gs_po_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glDeleteProgram() call failed.");

	/* Create shader programs */
	m_fs_po_id = gl.createShaderProgramv(GL_FRAGMENT_SHADER, 1, /* count */
										 &fs_specialized_code_raw);
	m_gs_po_id = gl.createShaderProgramv(GL_GEOMETRY_SHADER, 1, /* count */
										 &gs_specialized_code_raw);
	m_vs_po_id = gl.createShaderProgramv(GL_VERTEX_SHADER, 1, /* count */
										 &vs_specialized_code_raw);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateShaderProgramv() call(s) failed.");

	if (m_fs_po_id == 0 || m_gs_po_id == 0 || m_vs_po_id == 0)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "At least one glCreateShaderProgramv() call returned 0."
						   << tcu::TestLog::EndMessage;

		result = false;
		goto end;
	}

	/* Make sure all shader programs were linked successfully */
	so_po_ids[0] = m_fs_po_id;
	so_po_ids[1] = m_gs_po_id;
	so_po_ids[2] = m_vs_po_id;

	for (unsigned int n_po_id = 0; n_po_id != n_so_po_ids; ++n_po_id)
	{
		gl.getProgramiv(so_po_ids[n_po_id], GL_LINK_STATUS, &link_status);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGetProgramiv() call failed.");

		if (link_status != GL_TRUE)
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "A valid shader program with id [" << so_po_ids[n_po_id]
							   << "] was not linked successfully." << tcu::TestLog::EndMessage;

			result = false;
			goto end;
		}
	}

	/* Set up the vertex array object */
	gl.genVertexArrays(1, &m_vao_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenVertexArrays() call failed.");

	gl.bindVertexArray(m_vao_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindVertexArray() call failed.");

	/* Set up the pipeline object */
	initPipelineObject();

	/* Render a full-screen quad */
	gl.bindProgramPipeline(m_pipeline_object_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindProgramPipeline() call failed.");

	gl.drawArrays(GL_POINTS, 0, /* first */
				  1);			/* count */
	GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawArrays() call failed.");

	/* Verify the rendering result */
	unsigned char result_data[m_to_width * m_to_height * 4 /* rgba */];

	gl.readPixels(0, /* x */
				  0, /* y */
				  m_to_width, m_to_height, GL_RGBA, GL_UNSIGNED_BYTE, result_data);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glReadPixels() call failed.");

	for (unsigned int y = 0; y < m_to_height; ++y)
	{
		unsigned char* traveller_ptr = result_data + 4 * y;

		for (unsigned int x = 0; x < m_to_width; ++x)
		{
			if (traveller_ptr[0] != 0 || traveller_ptr[1] != 255 || traveller_ptr[2] != 0 || traveller_ptr[3] != 0)
			{
				m_testCtx.getLog() << tcu::TestLog::Message << "Invalid result texel found at (" << x << ", " << y
								   << ")." << tcu::TestLog::EndMessage;

				result = false;
			}

			traveller_ptr += 4; /* rgba */
		}
	}

end:
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
GeometryShaderGetShaderivTest::GeometryShaderGetShaderivTest(Context& context, const ExtParameters& extParams,
															 const char* name, const char* description)
	: TestCaseBase(context, extParams, name, description), m_gs_id(0)
{
}

/** Deinitializes GLES objects created during the test. */
void GeometryShaderGetShaderivTest::deinit()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (m_gs_id != 0)
	{
		gl.deleteShader(m_gs_id);

		m_gs_id = 0;
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
tcu::TestNode::IterateResult GeometryShaderGetShaderivTest::iterate()
{
	const glw::Functions& gl	 = m_context.getRenderContext().getFunctions();
	bool				  result = true;

	/* This test should only run if EXT_geometry_shader is supported. */
	if (!m_is_geometry_shader_extension_supported)
	{
		throw tcu::NotSupportedError(GEOMETRY_SHADER_EXTENSION_NOT_SUPPORTED, "", __FILE__, __LINE__);
	}

	/* Create a GS */
	m_gs_id = gl.createShader(GL_GEOMETRY_SHADER);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateShader() call failed.");

	/* Check the type reported for the SO */
	glw::GLint shader_type = GL_NONE;

	gl.getShaderiv(m_gs_id, GL_SHADER_TYPE, &shader_type);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetShaderiv() call failed.");

	if ((glw::GLenum)shader_type != m_glExtTokens.GEOMETRY_SHADER)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Invalid shader type [" << shader_type
						   << "] reported for a Geometry Shader" << tcu::TestLog::EndMessage;

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
GeometryShaderGetProgramivTest::GeometryShaderGetProgramivTest(Context& context, const ExtParameters& extParams,
															   const char* name, const char* description)
	: TestCaseBase(context, extParams, name, description), m_po_id(0)
{
}

/** Deinitializes GLES objects created during the test. */
void GeometryShaderGetProgramivTest::deinit()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (m_po_id != 0)
	{
		gl.deleteProgram(m_po_id);

		m_po_id = 0;
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
tcu::TestNode::IterateResult GeometryShaderGetProgramivTest::iterate()
{
	const glw::Functions& gl	 = m_context.getRenderContext().getFunctions();
	bool				  result = true;

	/* This test should only run if EXT_geometry_shader is supported. */
	if (!m_is_geometry_shader_extension_supported)
	{
		throw tcu::NotSupportedError(GEOMETRY_SHADER_EXTENSION_NOT_SUPPORTED, "", __FILE__, __LINE__);
	}

	/* Create a program object */
	m_po_id = gl.createProgram();

	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateProgram() call failed.");

	/* Verify that GS-specific queries cause a GL_INVALID_OPERATION error */
	const glw::GLenum pnames[] = { m_glExtTokens.GEOMETRY_LINKED_VERTICES_OUT, m_glExtTokens.GEOMETRY_LINKED_INPUT_TYPE,
								   m_glExtTokens.GEOMETRY_LINKED_OUTPUT_TYPE,
								   m_glExtTokens.GEOMETRY_SHADER_INVOCATIONS };
	const unsigned int n_pnames = sizeof(pnames) / sizeof(pnames[0]);

	for (unsigned int n_pname = 0; n_pname < n_pnames; ++n_pname)
	{
		glw::GLenum error_code = GL_NO_ERROR;
		glw::GLenum pname	  = pnames[n_pname];
		glw::GLint  rv		   = -1;

		gl.getProgramiv(m_po_id, pname, &rv);

		error_code = gl.getError();

		if (error_code != GL_INVALID_OPERATION)
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "No error generated by glGetProgramiv() for pname [" << pname
							   << "]" << tcu::TestLog::EndMessage;

			result = false;
		}
	} /* for (all pnames) */

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
GeometryShaderGetProgramiv2Test::GeometryShaderGetProgramiv2Test(Context& context, const ExtParameters& extParams,
																 const char* name, const char* description)
	: TestCaseBase(context, extParams, name, description), m_fs_id(0), m_po_id(0), m_vs_id(0)
{
}

/** Deinitializes GLES objects created during the test. */
void GeometryShaderGetProgramiv2Test::deinit()
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

	/* Release base class */
	TestCaseBase::deinit();
}

/** Executes the test.
 *
 *  Sets the test result to QP_TEST_RESULT_FAIL if the test failed, QP_TEST_RESULT_PASS otherwise.
 *  @return STOP if the test has finished, CONTINUE to indicate iterate should be called once again.
 *  Note the function throws exception should an error occur!
 **/
tcu::TestNode::IterateResult GeometryShaderGetProgramiv2Test::iterate()
{
	const glw::Functions& gl   = m_context.getRenderContext().getFunctions();
	const glw::GLenum pnames[] = { m_glExtTokens.GEOMETRY_LINKED_VERTICES_OUT, m_glExtTokens.GEOMETRY_LINKED_INPUT_TYPE,
								   m_glExtTokens.GEOMETRY_LINKED_OUTPUT_TYPE,
								   m_glExtTokens.GEOMETRY_SHADER_INVOCATIONS };
	const unsigned int n_pnames = sizeof(pnames) / sizeof(pnames[0]);
	bool			   result   = true;

	/* This test should only run if EXT_geometry_shader is supported. */
	if (!m_is_geometry_shader_extension_supported)
	{
		throw tcu::NotSupportedError(GEOMETRY_SHADER_EXTENSION_NOT_SUPPORTED, "", __FILE__, __LINE__);
	}

	/* Initialize the program object */
	std::string specialized_dummy_fs = specializeShader(1,
														/* parts */ &dummy_fs_code);
	const char* specialized_dummy_fs_raw = specialized_dummy_fs.c_str();
	std::string specialized_dummy_vs	 = specializeShader(1,
														/* parts */ &dummy_vs_code);
	const char* specialized_dummy_vs_raw = specialized_dummy_vs.c_str();

	m_fs_id = gl.createShader(GL_FRAGMENT_SHADER);
	m_vs_id = gl.createShader(GL_VERTEX_SHADER);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateShader() call(s) failed.");

	m_po_id = gl.createProgram();

	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateProgram() call failed.");

	if (!TestCaseBase::buildProgram(m_po_id, m_fs_id, 1, &specialized_dummy_fs_raw, m_vs_id, 1,
									&specialized_dummy_vs_raw))
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Failed to build a dummy test program object"
						   << tcu::TestLog::EndMessage;

		result = false;
		goto end;
	}

	/* Verify that GS-specific queries cause a GL_INVALID_OPERATION error
	 * for a linked PO lacking the GS stage.
	 */
	for (unsigned int n_pname = 0; n_pname < n_pnames; ++n_pname)
	{
		glw::GLenum error_code = GL_NO_ERROR;
		glw::GLenum pname	  = pnames[n_pname];
		glw::GLint  rv		   = -1;

		gl.getProgramiv(m_po_id, pname, &rv);

		error_code = gl.getError();

		if (error_code != GL_INVALID_OPERATION)
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "No error generated by glGetProgramiv() for pname [" << pname
							   << "]" << tcu::TestLog::EndMessage;

			result = false;
		}
	} /* for (all pnames) */

end:
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
GeometryShaderGetProgramiv3Test::GeometryShaderGetProgramiv3Test(Context& context, const ExtParameters& extParams,
																 const char* name, const char* description)
	: TestCaseBase(context, extParams, name, description)
	, m_fs_id(0)
	, m_fs_po_id(0)
	, m_gs_id(0)
	, m_gs_po_id(0)
	, m_pipeline_object_id(0)
	, m_po_id(0)
	, m_vs_id(0)
	, m_vs_po_id(0)
{
}

/* Compiles a shader object using caller-specified data.
 *
 * @param so_id   ID of a Shader Object to compile.
 * @param so_body Body to use for the compilation process.
 *
 * @return true if the compilation succeeded, false otherwise */
bool GeometryShaderGetProgramiv3Test::buildShader(glw::GLuint so_id, const char* so_body)
{
	glw::GLint			  compile_status = GL_FALSE;
	const glw::Functions& gl			 = m_context.getRenderContext().getFunctions();
	bool				  result		 = false;

	gl.shaderSource(so_id, 1,			/* count */
					&so_body, DE_NULL); /* length */
	GLU_EXPECT_NO_ERROR(gl.getError(), "glShaderSource() call failed.");

	gl.compileShader(so_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCompileShader() call failed.");

	gl.getShaderiv(so_id, GL_COMPILE_STATUS, &compile_status);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetShaderiv() call failed.");

	result = (compile_status == GL_TRUE);

	return result;
}

/** Builds a single shader program object using caller-specified data.
 *
 *  @param out_spo_id Deref will be set to the ID of the created shader program object.
 *                    Must not be NULL.
 *  @param spo_bits   Bits to be passed to the glCreateShaderProgramv() call.
 *  @param spo_body   Body to use for the glCreateShaderProgramv() call.
 *
 *  @return true if the shader program object was linked successfully, false otherwise.
 */
bool GeometryShaderGetProgramiv3Test::buildShaderProgram(glw::GLuint* out_spo_id, glw::GLenum spo_bits,
														 const char* spo_body)
{
	const glw::Functions& gl		  = m_context.getRenderContext().getFunctions();
	glw::GLint			  link_status = GL_FALSE;
	bool				  result	  = true;

	*out_spo_id = gl.createShaderProgramv(spo_bits, 1, /* count */
										  &spo_body);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateShaderProgramv() call failed.");

	gl.getProgramiv(*out_spo_id, GL_LINK_STATUS, &link_status);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetProgramiv() call failed.");

	result = (link_status == GL_TRUE);

	return result;
}

/** Deinitializes GLES objects created during the test. */
void GeometryShaderGetProgramiv3Test::deinit()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	deinitPO();
	deinitSOs(true);
	deinitSPOs(true);

	if (m_pipeline_object_id != 0)
	{
		gl.deleteProgramPipelines(1, &m_pipeline_object_id);

		m_pipeline_object_id = 0;
	}

	/* Release base class */
	TestCaseBase::deinit();
}

/** Deinitializes a program object created for the conformance test. */
void GeometryShaderGetProgramiv3Test::deinitPO()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (m_po_id != 0)
	{
		gl.deleteProgram(m_po_id);

		m_po_id = 0;
	}
}

/** Deinitializes shader objects created for the conformance test. */
void GeometryShaderGetProgramiv3Test::deinitSOs(bool release_all_SOs)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (m_fs_id != 0 && release_all_SOs)
	{
		gl.deleteShader(m_fs_id);

		m_fs_id = 0;
	}

	if (m_gs_id != 0)
	{
		gl.deleteShader(m_gs_id);

		m_gs_id = 0;
	}

	if (m_vs_id != 0 && release_all_SOs)
	{
		gl.deleteShader(m_vs_id);

		m_vs_id = 0;
	}
}

/** Deinitializes shader program objects created for the conformance test. */
void GeometryShaderGetProgramiv3Test::deinitSPOs(bool release_all_SPOs)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (m_fs_po_id != 0 && release_all_SPOs)
	{
		gl.deleteProgram(m_fs_po_id);

		m_fs_po_id = 0;
	}

	if (m_gs_po_id != 0)
	{
		gl.deleteProgram(m_gs_po_id);

		m_gs_po_id = 0;
	}

	if (m_vs_po_id != 0 && release_all_SPOs)
	{
		gl.deleteProgram(m_vs_po_id);

		m_vs_po_id = 0;
	}
}

/** Retrieves ES SL layout qualifier, corresponding to user-specified
 *  primitive type.
 *
 *  @param primitive_type Primitive type (described by a GLenum value)
 *                        to use for the query.
 *
 *  @return Requested layout qualifier.
 */
std::string GeometryShaderGetProgramiv3Test::getLayoutQualifierForPrimitiveType(glw::GLenum primitive_type)
{
	std::string result;

	switch (primitive_type)
	{
	case GL_LINE_STRIP:
		result = "line_strip";
		break;
	case GL_LINES_ADJACENCY:
		result = "lines_adjacency";
		break;
	case GL_POINTS:
		result = "points";
		break;
	case GL_TRIANGLES:
		result = "triangles";
		break;
	case GL_TRIANGLE_STRIP:
		result = "triangle_strip";
		break;

	default:
	{
		DE_ASSERT(0);
	}
	} /* switch (primitive_type) */

	return result;
}

/** Retrieves body of a geometry shadet to be used for the conformance test.
 *  The body is generated, according to the properties described by the
 *  run descriptor passed as an argument.
 *
 *  @param run Test run descriptor.
 *
 *  @return Requested string.
 */
std::string GeometryShaderGetProgramiv3Test::getGSCode(const _run& run)
{
	std::stringstream code_sstream;

	code_sstream << "${VERSION}\n"
					"${GEOMETRY_SHADER_REQUIRE}\n"
					"\n"
					"layout("
				 << getLayoutQualifierForPrimitiveType(run.input_primitive_type) << ", "
																					"invocations = "
				 << run.invocations << ") in;\n"
									   "layout("
				 << getLayoutQualifierForPrimitiveType(run.output_primitive_type) << ", "
																					 "max_vertices = "
				 << run.max_vertices << ") out;\n"
										"\n"
										"out gl_PerVertex {\n"
										"    vec4 gl_Position;\n"
										"};\n"
										"\n"
										"void main()\n"
										"{\n"
										"    for (int n = 0; n < "
				 << run.max_vertices << "; ++n)\n"
										"    {\n"
										"        gl_Position = vec4(n, 0.0, 0.0, 1.0);\n"
										"        EmitVertex();\n"
										"    }\n"
										"\n"
										"    EndPrimitive();\n"
										"}\n";

	return code_sstream.str();
}

/** Initializes internal _runs member with test iteration settings for all test runs. */
void GeometryShaderGetProgramiv3Test::initTestRuns()
{
	/*                   input primitive type | invocations | max vertices | output primitive type *
	 *----------------------------------------+-------------+--------------+-----------------------*/
	_runs.push_back(_run(GL_LINES_ADJACENCY, 3, 16, GL_POINTS));
	_runs.push_back(_run(GL_TRIANGLES, 12, 37, GL_LINE_STRIP));
	_runs.push_back(_run(GL_POINTS, 31, 75, GL_TRIANGLE_STRIP));
}

/** Executes the test.
 *
 *  Sets the test result to QP_TEST_RESULT_FAIL if the test failed, QP_TEST_RESULT_PASS otherwise.
 *  @return STOP if the test has finished, CONTINUE to indicate iterate should be called once again.
 *  Note the function throws exception should an error occur!
 **/
tcu::TestNode::IterateResult GeometryShaderGetProgramiv3Test::iterate()
{
	const glw::Functions& gl					  = m_context.getRenderContext().getFunctions();
	glw::GLint			  gs_spo_id				  = 0;
	unsigned int		  n_run					  = 0;
	unsigned int		  n_separable_object_case = 0;
	bool				  result				  = true;

	/* This test should only run if EXT_geometry_shader is supported. */
	if (!m_is_geometry_shader_extension_supported)
	{
		throw tcu::NotSupportedError(GEOMETRY_SHADER_EXTENSION_NOT_SUPPORTED, "", __FILE__, __LINE__);
	}

	/* Prepare specialized versions of dummy fragment & vertex shaders */
	std::string dummy_fs_specialized = specializeShader(1,
														/* parts */ &dummy_fs_code);
	const char* dummy_fs_specialized_raw = dummy_fs_specialized.c_str();
	std::string dummy_vs_specialized	 = specializeShader(1, &dummy_vs_code);
	const char* dummy_vs_specialized_raw = dummy_vs_specialized.c_str();

	/* Set up the fragment & the vertex shaders */
	m_fs_id = gl.createShader(GL_FRAGMENT_SHADER);
	m_gs_id = gl.createShader(GL_GEOMETRY_SHADER);
	m_vs_id = gl.createShader(GL_VERTEX_SHADER);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateShader() call(s) failed.");

	if (!buildShader(m_fs_id, dummy_fs_specialized_raw) || !buildShader(m_vs_id, dummy_vs_specialized_raw))
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Either FS or VS failed to build." << tcu::TestLog::EndMessage;

		result = false;
		goto end;
	}

	/* Set up the test program object */
	m_po_id = gl.createProgram();

	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateProgram() call failed.");

	gl.attachShader(m_po_id, m_fs_id);
	gl.attachShader(m_po_id, m_gs_id);
	gl.attachShader(m_po_id, m_vs_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glAttachShader() call(s) failed.");

	/* Set up the fragment & the vertex shader programs */
	if (!buildShaderProgram(&m_fs_po_id, GL_FRAGMENT_SHADER, dummy_fs_specialized_raw) ||
		!buildShaderProgram(&m_vs_po_id, GL_VERTEX_SHADER, dummy_vs_specialized_raw))
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Either FS or VS SPOs failed to build."
						   << tcu::TestLog::EndMessage;

		result = false;
		goto end;
	}

	/* Set up test runs */
	initTestRuns();

	/* The test should check both a geometry shader program object and a full-blown PO
	 * consisting of FS, GS and VS. */
	for (n_separable_object_case = 0; n_separable_object_case < 2; /* PO, SPO cases */
		 ++n_separable_object_case)
	{
		bool should_use_separable_object = (n_separable_object_case != 0);

		/* Iterate over all test runs */
		for (n_run = 0; n_run < _runs.size(); ++n_run)
		{
			const _run& current_run			= _runs[n_run];
			std::string gs_code				= getGSCode(current_run);
			const char* gs_code_raw			= gs_code.c_str();
			std::string gs_code_specialized = specializeShader(1, /* parts */
															   &gs_code_raw);
			const char* gs_code_specialized_raw = gs_code_specialized.c_str();

			if (should_use_separable_object)
			{
				/* Deinitialize any objects that may have been created in previous iterations */
				deinitSPOs(false);

				/* Set up the geometry shader program object */
				if (!buildShaderProgram(&m_gs_po_id, GL_GEOMETRY_SHADER, gs_code_specialized_raw))
				{
					m_testCtx.getLog() << tcu::TestLog::Message << "Failed to compile a geometry shader program object"
									   << tcu::TestLog::EndMessage;

					result = false;
					goto end;
				}
			} /* if (should_use_pipeline_object) */
			else
			{
				gl.bindProgramPipeline(0);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glBindProgramPipeline() call failed.");

				/* Set up the geometry shader object */
				if (!buildShader(m_gs_id, gs_code_specialized_raw))
				{
					m_testCtx.getLog() << tcu::TestLog::Message << "Failed to compile a geometry shader object."
									   << tcu::TestLog::EndMessage;

					result = false;
					goto end;
				}

				/* Set up the program object */
				glw::GLint link_status = GL_FALSE;

				gl.linkProgram(m_po_id);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glLinkProgram() call failed.");

				gl.getProgramiv(m_po_id, GL_LINK_STATUS, &link_status);

				if (link_status == GL_FALSE)
				{
					m_testCtx.getLog() << tcu::TestLog::Message << "Test program object failed to link"
									   << tcu::TestLog::EndMessage;

					result = false;
					goto end;
				}

				/* Bind the PO to the rendering context */
				gl.useProgram(m_po_id);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram() call failed.");
			}

			/* Execute the queries */
			glw::GLuint po_id								= (should_use_separable_object) ? m_gs_po_id : m_po_id;
			glw::GLint  result_geometry_linked_vertices_out = 0;
			glw::GLint  result_geometry_linked_input_type   = 0;
			glw::GLint  result_geometry_linked_output_type  = 0;
			glw::GLint  result_geometry_shader_invocations  = 0;

			gl.getProgramiv(po_id, m_glExtTokens.GEOMETRY_LINKED_VERTICES_OUT, &result_geometry_linked_vertices_out);
			GLU_EXPECT_NO_ERROR(gl.getError(),
								"glGetProgramiv() call failed for GL_GEOMETRY_LINKED_VERTICES_OUT_EXT query.");

			gl.getProgramiv(po_id, m_glExtTokens.GEOMETRY_LINKED_INPUT_TYPE, &result_geometry_linked_input_type);
			GLU_EXPECT_NO_ERROR(gl.getError(),
								"glGetProgramiv() call failed for GL_GEOMETRY_LINKED_INPUT_TYPE_EXT query.");

			gl.getProgramiv(po_id, m_glExtTokens.GEOMETRY_LINKED_OUTPUT_TYPE, &result_geometry_linked_output_type);
			GLU_EXPECT_NO_ERROR(gl.getError(),
								"glGetProgramiv() call failed for GL_GEOMETRY_LINKED_OUTPUT_TYPE_EXT query.");

			gl.getProgramiv(po_id, m_glExtTokens.GEOMETRY_SHADER_INVOCATIONS, &result_geometry_shader_invocations);
			GLU_EXPECT_NO_ERROR(gl.getError(),
								"glGetProgramiv() call failed for GL_GEOMETRY_LINKED_INPUT_TYPE_EXT query.");

			if (current_run.input_primitive_type != (glw::GLenum)result_geometry_linked_input_type)
			{
				m_testCtx.getLog() << tcu::TestLog::Message << "GL_GEOMETRY_LINKED_INPUT_TYPE_EXT query value "
								   << "[" << result_geometry_linked_input_type
								   << "]"
									  " does not match the test run setting "
									  "["
								   << current_run.input_primitive_type << "]" << tcu::TestLog::EndMessage;

				result = false;
			}

			if (current_run.invocations != result_geometry_shader_invocations)
			{
				m_testCtx.getLog() << tcu::TestLog::Message << "GL_GEOMETRY_SHADER_INVOCATIONS_EXT query value "
								   << "[" << result_geometry_shader_invocations
								   << "]"
									  " does not match the test run setting "
									  "["
								   << current_run.input_primitive_type << "]" << tcu::TestLog::EndMessage;

				result = false;
			}

			if (current_run.max_vertices != result_geometry_linked_vertices_out)
			{
				m_testCtx.getLog() << tcu::TestLog::Message << "GL_GEOMETRY_LINKED_VERTICES_OUT query value "
								   << "[" << result_geometry_linked_vertices_out
								   << "]"
									  " does not match the test run setting "
									  "["
								   << current_run.max_vertices << "]" << tcu::TestLog::EndMessage;

				result = false;
			}

			if (current_run.output_primitive_type != (glw::GLenum)result_geometry_linked_output_type)
			{
				m_testCtx.getLog() << tcu::TestLog::Message << "GL_GEOMETRY_LINKED_OUTPUT_TYPE_EXT query value "
								   << "[" << result_geometry_linked_output_type
								   << "]"
									  " does not match the test run setting "
									  "["
								   << current_run.output_primitive_type << "]" << tcu::TestLog::EndMessage;

				result = false;
			}
		} /* for (all test runs) */
	}	 /* for (PO & SPO cases) */

	/* One more check: build a pipeline object which only defines a FS & VS stages,
	 *                 and check what GS SPO ID the object reports. */
	gl.genProgramPipelines(1, &m_pipeline_object_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenProgramPipelines() call failed.");

	gl.useProgramStages(m_pipeline_object_id, GL_FRAGMENT_SHADER_BIT, m_fs_po_id);
	gl.useProgramStages(m_pipeline_object_id, GL_VERTEX_SHADER_BIT, m_vs_po_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgramStages() call failed.");

	gl.getProgramPipelineiv(m_pipeline_object_id, m_glExtTokens.GEOMETRY_SHADER, &gs_spo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetProgramPipelineiv() call failed.");

	if (gs_spo_id != 0)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Pipeline object reported [" << gs_spo_id << "]"
						   << " for GL_GEOMETRY_SHADER_EXT query, even though no GS SPO was bound."
						   << tcu::TestLog::EndMessage;

		result = false;
	}

end:
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
GeometryShaderDrawCallWithFSAndGS::GeometryShaderDrawCallWithFSAndGS(Context& context, const ExtParameters& extParams,
																	 const char* name, const char* description)
	: TestCaseBase(context, extParams, name, description)
	, m_fs_po_id(0)
	, m_gs_po_id(0)
	, m_pipeline_object_id(0)
	, m_vao_id(0)
{
}

/** Deinitializes GLES objects created during the test. */
void GeometryShaderDrawCallWithFSAndGS::deinit()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (m_fs_po_id != 0)
	{
		gl.deleteProgram(m_fs_po_id);

		m_fs_po_id = 0;
	}

	if (m_gs_po_id != 0)
	{
		gl.deleteProgram(m_gs_po_id);

		m_gs_po_id = 0;
	}

	if (m_pipeline_object_id != 0)
	{
		gl.deleteProgramPipelines(1, &m_pipeline_object_id);

		m_pipeline_object_id = 0;
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
tcu::TestNode::IterateResult GeometryShaderDrawCallWithFSAndGS::iterate()
{
	glw::GLenum			  error_code = GL_NO_ERROR;
	const glw::Functions& gl		 = m_context.getRenderContext().getFunctions();
	bool				  result	 = true;

	/* This test should only run if EXT_geometry_shader is supported. */
	if (!m_is_geometry_shader_extension_supported)
	{
		throw tcu::NotSupportedError(GEOMETRY_SHADER_EXTENSION_NOT_SUPPORTED, "", __FILE__, __LINE__);
	}

	/* Create & bind a VAO */
	gl.genVertexArrays(1, &m_vao_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenVertexArrays() call failed.");

	gl.bindVertexArray(m_vao_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindVertexArray() call failed.");

	/* Create shader program objects */
	std::string code_fs_specialized = specializeShader(1, /* parts */
													   &dummy_fs_code);
	const char* code_fs_specialized_raw = code_fs_specialized.c_str();
	std::string code_gs_specialized		= specializeShader(1, /* parts */
													   &dummy_gs_code);
	const char* code_gs_specialized_raw = code_gs_specialized.c_str();
	glw::GLint  link_status				= GL_FALSE;

	m_fs_po_id = gl.createShaderProgramv(GL_FRAGMENT_SHADER, 1, /* count */
										 &code_fs_specialized_raw);
	m_gs_po_id = gl.createShaderProgramv(GL_GEOMETRY_SHADER, 1, /* count */
										 &code_gs_specialized_raw);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateShaderProgramv() call(s) failed.");

	gl.getProgramiv(m_fs_po_id, GL_LINK_STATUS, &link_status);

	if (link_status != GL_TRUE)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Dummy fragment shader program failed to link."
						   << tcu::TestLog::EndMessage;

		result = false;
		goto end;
	}

	gl.getProgramiv(m_gs_po_id, GL_LINK_STATUS, &link_status);

	if (link_status != GL_TRUE)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Dummy geometry shader program failed to link."
						   << tcu::TestLog::EndMessage;

		result = false;
		goto end;
	}

	/* Create & set up a pipeline object */
	gl.genProgramPipelines(1, &m_pipeline_object_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenProgramPipelines() call failed.");

	gl.useProgramStages(m_pipeline_object_id, GL_FRAGMENT_SHADER_BIT, m_fs_po_id);
	gl.useProgramStages(m_pipeline_object_id, GL_GEOMETRY_SHADER_BIT, m_gs_po_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgramStages() call(s) failed.");

	gl.bindProgramPipeline(m_pipeline_object_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindProgramPipeline() call failed.");

	/* Try to do a draw call */
	gl.drawArrays(GL_POINTS, 0, /* first */
				  1);			/* count */

	error_code = gl.getError();
	if (error_code != GL_INVALID_OPERATION)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Invalid draw call generated an error code [" << error_code
						   << "]"
							  " which is different from the expected GL_INVALID_OPERATION."
						   << tcu::TestLog::EndMessage;

		result = false;
	}

end:
	// m_pipeline_object_id is generated in this function, need to be freed
	if (m_pipeline_object_id)
	{
		gl.deleteProgramPipelines(1, &m_pipeline_object_id);
		m_pipeline_object_id = 0;
	}

	// m_gs_po_id is generated in this function, need to be freed
	if (m_gs_po_id)
	{
		gl.deleteProgram(m_gs_po_id);
		m_gs_po_id = 0;
	}

	// m_fs_po_id is generated in this function, need to be freed
	if (m_fs_po_id)
	{
		gl.deleteProgram(m_fs_po_id);
		m_fs_po_id = 0;
	}

	// m_vao_id is generated in this function, need to be freed
	if (m_vao_id)
	{
		gl.deleteVertexArrays(1, &m_vao_id);
		m_vao_id = 0;
	}

	/* All done */
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
GeometryShaderMaxImageUniformsTest::GeometryShaderMaxImageUniformsTest(Context& context, const ExtParameters& extParams,
																	   const char* name, const char* description)
	: TestCaseBase(context, extParams, name, description)
	, m_fs_id(0)
	, m_gl_max_geometry_image_uniforms_ext_value(0)
	, m_gs_id(0)
	, m_po_id(0)
	, m_texture_ids(NULL)
	, m_tfbo_id(0)
	, m_vao_id(0)
	, m_vs_id(0)
{
	//Bug-15063 Only GLSL 4.50 supports opaque types
	if (m_glslVersion >= glu::GLSL_VERSION_130)
	{
		m_glslVersion = glu::GLSL_VERSION_450;
	}
}

/** Deinitializes GLES objects created during the test. */
void GeometryShaderMaxImageUniformsTest::deinit()
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

	if (m_texture_ids != NULL)
	{
		gl.deleteTextures(m_gl_max_geometry_image_uniforms_ext_value, m_texture_ids);

		delete[] m_texture_ids;
		m_texture_ids = NULL;
	}

	if (m_tfbo_id != 0)
	{
		gl.deleteBuffers(1, &m_tfbo_id);
		m_tfbo_id = 0;
	}

	if (m_vao_id != 0)
	{
		gl.deleteVertexArrays(1, &m_vao_id);
		m_vao_id = 0;
	}

	if (m_vs_id != 0)
	{
		gl.deleteShader(m_vs_id);

		m_vs_id = 0;
	}

	/* Set GL_PACK_ALIGNMENT to default value. */
	gl.pixelStorei(GL_PACK_ALIGNMENT, 4 /*default value taken from specification*/);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glPixelStorei() call failed for GL_PACK_ALIGNMENT pname.");

	/* Release base class */
	TestCaseBase::deinit();
}

/* Retrieves test-specific geometry shader source code.
 *
 * @return Requested string.
 */
std::string GeometryShaderMaxImageUniformsTest::getGSCode()
{
	std::stringstream code_sstream;

	/* Form the GS */
	code_sstream << "${VERSION}\n"
					"${GEOMETRY_SHADER_REQUIRE}\n"
					"\n"
					"layout (points)                   in;\n"
					"layout (points, max_vertices = 1) out;\n"
					"\n"
					"precision highp iimage2D;\n"
					"\n"
					"ivec4 counter = ivec4(0);\n"
					"\n";

	for (glw::GLint n_img = 0; n_img < (m_gl_max_geometry_image_uniforms_ext_value); ++n_img)
	{
		code_sstream << "layout(binding = " << n_img << ", r32i) uniform iimage2D img" << n_img << ";\n";
	}

	code_sstream << "\n"
					"void main()\n"
					"{\n";

	for (glw::GLint n_img = 0; n_img < (m_gl_max_geometry_image_uniforms_ext_value); ++n_img)
	{
		code_sstream << "    counter += imageLoad(img" << n_img << ", ivec2(0, 0));\n";
	}

	code_sstream << "\n"
					"    gl_Position = vec4(float(counter.x), 0.0, 0.0, 1.0);\n"
					"    EmitVertex();\n"
					"}\n";

	/* Form a specialized version of the GS source code */
	std::string gs_code				= code_sstream.str();
	const char* gs_code_raw			= gs_code.c_str();
	std::string gs_code_specialized = specializeShader(1 /* parts */, &gs_code_raw);

	return gs_code_specialized;
}

/** Executes the test.
 *
 *  Sets the test result to QP_TEST_RESULT_FAIL if the test failed, QP_TEST_RESULT_PASS otherwise.
 *  @return STOP if the test has finished, CONTINUE to indicate iterate should be called once again.
 *  Note the function throws exception should an error occur!
 **/
tcu::TestNode::IterateResult GeometryShaderMaxImageUniformsTest::iterate()
{
	glw::GLint		   counter						 = 0;
	glw::GLint		   expectedValue				 = 0;
	bool			   has_shader_compilation_failed = true;
	glw::GLfloat*	  ptr							 = DE_NULL;
	bool			   result						 = true;
	const glw::GLchar* feedbackVaryings[]			 = { "gl_Position" };

	std::string fs_code_specialized		= "";
	const char* fs_code_specialized_raw = DE_NULL;
	std::string gs_code_specialized		= "";
	const char* gs_code_specialized_raw = DE_NULL;
	std::string vs_code_specialized		= "";
	const char* vs_code_specialized_raw = DE_NULL;

	/* This test should only run if EXT_geometry_shader is supported. */
	if (!m_is_geometry_shader_extension_supported)
	{
		throw tcu::NotSupportedError(GEOMETRY_SHADER_EXTENSION_NOT_SUPPORTED, "", __FILE__, __LINE__);
	}

	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Retrieve GL_MAX_GEOMETRY_IMAGE_UNIFORMS_EXT pname value */
	gl.getIntegerv(m_glExtTokens.MAX_GEOMETRY_IMAGE_UNIFORMS, &m_gl_max_geometry_image_uniforms_ext_value);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv() failed for GL_MAX_GEOMETRY_IMAGE_UNIFORMS_EXT pname");

	/* Retrieve GL_MAX_GEOMETRY_TEXTURE_IMAGE_UNITS_EXT pname value */
	glw::GLint m_gl_max_geometry_texture_image_units_ext_value = 0;
	gl.getIntegerv(m_glExtTokens.MAX_GEOMETRY_TEXTURE_IMAGE_UNITS, &m_gl_max_geometry_texture_image_units_ext_value);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv() failed for GL_MAX_GEOMETRY_IMAGE_UNIFORMS_EXT pname");

	/* Check if m_gl_max_geometry_image_uniforms_value is less than or equal zero. */
	if (m_gl_max_geometry_image_uniforms_ext_value <= 0)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "GL_MAX_GEOMETRY_IMAGE_UNIFORMS_EXT query value "
						   << "[" << m_gl_max_geometry_image_uniforms_ext_value
						   << "]"
							  " is less than or equal zero. Image uniforms in Geometry Shader"
							  " are not supported."
						   << tcu::TestLog::EndMessage;

		if (m_gl_max_geometry_image_uniforms_ext_value == 0)
		{
			throw tcu::NotSupportedError("GL_MAX_GEOMETRY_IMAGE_UNIFORMS_EXT is 0");
		}
		else
		{
			result = false;
			goto end;
		}
	}

	/* Check if m_gl_max_geometry_texture_image_units_value is less than m_gl_max_geometry_image_uniforms_value. */
	if (m_gl_max_geometry_texture_image_units_ext_value < m_gl_max_geometry_image_uniforms_ext_value)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "GL_MAX_GEOMETRY_IMAGE_UNIFORMS_EXT query value "
						   << "[" << m_gl_max_geometry_image_uniforms_ext_value
						   << "]"
							  " is greater than GL_MAX_GEOMETRY_TEXTURE_IMAGE_UNITS_EXT query value "
							  "["
						   << m_gl_max_geometry_texture_image_units_ext_value << "]." << tcu::TestLog::EndMessage;

		result = false;
		goto end;
	}

	/* Create a program object. */
	m_po_id = gl.createProgram();
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateProgram() call failed.");

	/* Create shader objects. */
	m_fs_id = gl.createShader(GL_FRAGMENT_SHADER);
	m_gs_id = gl.createShader(GL_GEOMETRY_SHADER);
	m_vs_id = gl.createShader(GL_VERTEX_SHADER);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateShader() call(s) failed.");

	/* Configure which outputs should be captured by Transform Feedback. */
	gl.transformFeedbackVaryings(m_po_id, 1 /* varyings count */, feedbackVaryings, GL_INTERLEAVED_ATTRIBS);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glTransformFeedbackVaryings() call failed.");

	/* Try to link the test program object */
	fs_code_specialized		= specializeShader(1, &dummy_fs_code);
	fs_code_specialized_raw = fs_code_specialized.c_str();

	gs_code_specialized		= getGSCode();
	gs_code_specialized_raw = gs_code_specialized.c_str();

	vs_code_specialized		= specializeShader(1, &dummy_vs_code);
	vs_code_specialized_raw = vs_code_specialized.c_str();

	if (!TestCaseBase::buildProgram(m_po_id, m_gs_id, 1,				  /* n_sh1_body_parts */
									&gs_code_specialized_raw, m_vs_id, 1, /* n_sh2_body_parts */
									&vs_code_specialized_raw, m_fs_id, 1, /* n_sh3_body_parts */
									&fs_code_specialized_raw, &has_shader_compilation_failed))
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Program object linking failed." << tcu::TestLog::EndMessage;

		result = false;
		goto end;
	}

	/* Use program. */
	gl.useProgram(m_po_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram() call failed.");

	/* Allocate memory for m_max_image_units_value Texture Objects. */
	m_texture_ids = new glw::GLuint[m_gl_max_geometry_image_uniforms_ext_value];

	/* Generate m_max_image_units_value Texture Objects. */
	gl.genTextures(m_gl_max_geometry_image_uniforms_ext_value, m_texture_ids);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenTextures() call failed.");

	/* Set GL_PACK_ALIGNMENT to 1. */
	gl.pixelStorei(GL_PACK_ALIGNMENT, 1);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glPixelStorei() call failed for GL_PACK_ALIGNMENT pname.");

	/* Bind integer 2D texture objects of resolution 1x1 to image units. */
	for (glw::GLint n_img = 0; n_img < (m_gl_max_geometry_image_uniforms_ext_value); ++n_img)
	{
		glw::GLint texture = m_texture_ids[n_img];
		glw::GLint value   = n_img + 1;

		gl.bindTexture(GL_TEXTURE_2D, texture);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture() call failed.");

		gl.texStorage2D(GL_TEXTURE_2D, 1 /*levels*/, GL_R32I, 1 /*width*/, 1 /*height*/);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glTexStorage2D() call failed.");

		gl.texSubImage2D(GL_TEXTURE_2D, 0 /*level*/, 0 /*xoffset*/, 0 /*yoffset*/, 1 /*width*/, 1 /*height*/,
						 GL_RED_INTEGER, GL_INT, &value);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glTexSubImage2D() call failed.");

		gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		GLU_EXPECT_NO_ERROR(gl.getError(), "texParameteri() call failed.");

		gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		GLU_EXPECT_NO_ERROR(gl.getError(), "texParameteri() call failed.");

		gl.bindImageTexture(n_img, texture, 0 /*level*/, GL_FALSE /*is layered?*/, 0 /*layer*/, GL_READ_ONLY, GL_R32I);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindImageTexture() call failed.");
	}

	/* Configure VAO. */
	gl.genVertexArrays(1, &m_vao_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenVertexArrays() call failed.");

	gl.bindVertexArray(m_vao_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindVertexArray() call failed.");

	/* Create a Buffer Object for Transform Feedback's outputs. */
	gl.genBuffers(1, &m_tfbo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenBuffers() call failed.");

	gl.bindBuffer(GL_ARRAY_BUFFER, m_tfbo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer() call failed.");

	gl.bufferData(GL_ARRAY_BUFFER, sizeof(glw::GLfloat) * 4 /* four float vector components */, NULL, GL_STATIC_READ);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferData() call failed.");

	/* Bind Buffer Object m_tfbo_id to GL_TRANSFORM_FEEDBACK_BUFFER binding point. */
	gl.bindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, m_tfbo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBufferBase() call failed.");

	/* Disable rasterization and make a draw call. After that, turn on rasterization. */
	gl.enable(GL_RASTERIZER_DISCARD);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glEnable() call failed for GL_RASTERIZER_DISCARD pname.");

	gl.beginTransformFeedback(GL_POINTS);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBeginTransformFeedback() call failed.");

	gl.drawArrays(GL_POINTS, 0 /*starting index*/, 1 /*number of indices*/);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawArrays() call failed for GL_POINTS pname.");

	gl.endTransformFeedback();
	GLU_EXPECT_NO_ERROR(gl.getError(), "glEndTransformFeedback() call failed.");

	gl.disable(GL_RASTERIZER_DISCARD);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glDisable() call failed for GL_RASTERIZER_DISCARD pname.");

	/* Retrieve value from Transform Feedback. */
	counter = 0;
	ptr		= (glw::GLfloat*)gl.mapBufferRange(
		GL_ARRAY_BUFFER, 0, sizeof(glw::GLfloat) * 4 /* four float vector components */, GL_MAP_READ_BIT);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glMapBufferRange() call failed.");

	counter = int(ptr[0] + 0.5f);

	gl.unmapBuffer(GL_ARRAY_BUFFER);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUnmapBuffer() call failed.");

	/* Calculate expected value. */
	expectedValue = m_gl_max_geometry_image_uniforms_ext_value * (m_gl_max_geometry_image_uniforms_ext_value + 1) / 2;

	if (counter != expectedValue)
	{
		result = false;
	}

end:
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
GeometryShaderMaxShaderStorageBlocksTest::GeometryShaderMaxShaderStorageBlocksTest(Context&				context,
																				   const ExtParameters& extParams,
																				   const char*			name,
																				   const char*			description)
	: TestCaseBase(context, extParams, name, description)
	, m_fs_id(0)
	, m_gl_max_geometry_shader_storage_blocks_ext_value(0)
	, m_gs_id(0)
	, m_po_id(0)
	, m_ssbo_id(0)
	, m_tfbo_id(0)
	, m_vao_id(0)
	, m_vs_id(0)
{
}

/** Deinitializes GLES objects created during the test. */
void GeometryShaderMaxShaderStorageBlocksTest::deinit()
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

	if (m_ssbo_id != 0)
	{
		gl.deleteBuffers(1, &m_ssbo_id);
		m_ssbo_id = 0;
	}

	if (m_tfbo_id != 0)
	{
		gl.deleteBuffers(1, &m_tfbo_id);
		m_tfbo_id = 0;
	}

	if (m_vao_id != 0)
	{
		gl.deleteVertexArrays(1, &m_vao_id);
		m_vao_id = 0;
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
std::string GeometryShaderMaxShaderStorageBlocksTest::getGSCode()
{
	std::stringstream code_sstream;

	/* Form the GS */
	code_sstream << "${VERSION}\n"
					"${GEOMETRY_SHADER_REQUIRE}\n"
					"\n"
					"layout (points)                   in;\n"
					"layout (points, max_vertices = 1) out;\n"
					"\n"
					"int counter = 0;\n"
					"\n";

	for (glw::GLint n_ssb = 0; n_ssb < (m_gl_max_geometry_shader_storage_blocks_ext_value); ++n_ssb)
	{
		code_sstream << "layout(binding = " << n_ssb << ") buffer ssb" << n_ssb << " \n{\n"
					 << "    int value;\n"
					 << "} S_SSB" << n_ssb << ";\n\n";
	}

	code_sstream << "\n"
					"void main()\n"
					"{\n";

	for (glw::GLint n_ssb = 0; n_ssb < (m_gl_max_geometry_shader_storage_blocks_ext_value); ++n_ssb)
	{
		code_sstream << "    counter += S_SSB" << n_ssb << ".value++;\n";
	}

	code_sstream << "\n"
					"    gl_Position = vec4(float(counter), 0.0, 0.0, 1.0);\n"
					"    EmitVertex();\n"
					"}\n";

	/* Form a specialized version of the GS source code */
	std::string gs_code				= code_sstream.str();
	const char* gs_code_raw			= gs_code.c_str();
	std::string gs_code_specialized = specializeShader(1 /* parts */, &gs_code_raw);

	return gs_code_specialized;
}

/** Executes the test.
 *
 *  Sets the test result to QP_TEST_RESULT_FAIL if the test failed, QP_TEST_RESULT_PASS otherwise.
 *  @return STOP if the test has finished, CONTINUE to indicate iterate should be called once again.
 *  Note the function throws exception should an error occur!
 **/
tcu::TestNode::IterateResult GeometryShaderMaxShaderStorageBlocksTest::iterate()
{
	glw::GLint		   counter						 = 0;
	glw::GLint		   expectedValue				 = 0;
	const glw::GLchar* feedbackVaryings[]			 = { "gl_Position" };
	bool			   has_shader_compilation_failed = true;
	const glw::GLfloat initial_buffer_data[4]		 = { 0.0f, 0.0f, 0.0f, 0.0f };
	glw::GLint		   int_alignment				 = 0;
	const glw::GLint   int_size						 = sizeof(glw::GLint);
	glw::GLint*		   ptrSSBO_data					 = DE_NULL;
	glw::GLfloat*	  ptrTF_data					 = DE_NULL;
	bool			   result						 = true;
	glw::GLint		   ssbo_alignment				 = 0;
	glw::GLint*		   ssbo_data					 = DE_NULL;
	glw::GLint		   ssbo_data_size				 = 0;

	std::string fs_code_specialized		= "";
	const char* fs_code_specialized_raw = DE_NULL;
	std::string gs_code_specialized		= "";
	const char* gs_code_specialized_raw = DE_NULL;
	std::string vs_code_specialized		= "";
	const char* vs_code_specialized_raw = DE_NULL;

	/* This test should only run if EXT_geometry_shader is supported. */
	if (!m_is_geometry_shader_extension_supported)
	{
		throw tcu::NotSupportedError(GEOMETRY_SHADER_EXTENSION_NOT_SUPPORTED, "", __FILE__, __LINE__);
	}

	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Retrieve GL_MAX_GEOMETRY_SHADER_STORAGE_BLOCKS_EXT pname value */
	gl.getIntegerv(m_glExtTokens.MAX_GEOMETRY_SHADER_STORAGE_BLOCKS,
				   &m_gl_max_geometry_shader_storage_blocks_ext_value);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv() failed for GL_MAX_GEOMETRY_SHADER_STORAGE_BLOCKS_EXT pname");

	/* Retrieve GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS pname value */
	glw::GLint m_gl_max_shader_storage_buffer_bindings_value = 0;

	gl.getIntegerv(GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS, &m_gl_max_shader_storage_buffer_bindings_value);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv() failed for GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS pname");

	/* Check if m_gl_max_shader_storage_blocks_value is less than or equal zero. */
	if (m_gl_max_geometry_shader_storage_blocks_ext_value <= 0)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "GL_MAX_GEOMETRY_SHADER_STORAGE_BLOCKS_EXT query value "
						   << "[" << m_gl_max_geometry_shader_storage_blocks_ext_value
						   << "]"
							  " is less than or equal zero. Shader Storage Blocks"
							  " in Geometry Shader are not supported."
						   << tcu::TestLog::EndMessage;

		if (m_gl_max_geometry_shader_storage_blocks_ext_value == 0)
		{
			throw tcu::NotSupportedError("GL_MAX_GEOMETRY_SHADER_STORAGE_BLOCKS_EXT is 0");
		}
		else
		{
			result = false;
			goto end;
		}
	}

	/* Check if m_gl_max_shader_storage_buffer_bindings_value is less than m_gl_max_shader_storage_blocks_value. */
	if (m_gl_max_shader_storage_buffer_bindings_value < m_gl_max_geometry_shader_storage_blocks_ext_value)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "GL_MAX_GEOMETRY_SHADER_STORAGE_BLOCKS_EXT query value "
						   << "[" << m_gl_max_geometry_shader_storage_blocks_ext_value
						   << "]"
							  " is greater than GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS query value "
							  "["
						   << m_gl_max_shader_storage_buffer_bindings_value << "]." << tcu::TestLog::EndMessage;

		result = false;
		goto end;
	}

	/* Create a program object. */
	m_po_id = gl.createProgram();
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateProgram() call failed.");

	/* Create shader objects. */
	m_fs_id = gl.createShader(GL_FRAGMENT_SHADER);
	m_gs_id = gl.createShader(GL_GEOMETRY_SHADER);
	m_vs_id = gl.createShader(GL_VERTEX_SHADER);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateShader() call(s) failed.");

	/* Configure which outputs should be captured by Transform Feedback. */
	gl.transformFeedbackVaryings(m_po_id, 1 /* varyings count */, feedbackVaryings, GL_INTERLEAVED_ATTRIBS);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glTransformFeedbackVaryings() call failed.");

	/* Try to link the test program object */
	fs_code_specialized		= specializeShader(1, &dummy_fs_code);
	fs_code_specialized_raw = fs_code_specialized.c_str();

	gs_code_specialized		= getGSCode();
	gs_code_specialized_raw = gs_code_specialized.c_str();

	vs_code_specialized		= specializeShader(1, &dummy_vs_code);
	vs_code_specialized_raw = vs_code_specialized.c_str();

	if (!TestCaseBase::buildProgram(m_po_id, m_gs_id, 1,				  /* n_sh1_body_parts */
									&gs_code_specialized_raw, m_vs_id, 1, /* n_sh2_body_parts */
									&vs_code_specialized_raw, m_fs_id, 1, /* n_sh3_body_parts */
									&fs_code_specialized_raw, &has_shader_compilation_failed))
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Program object linking failed." << tcu::TestLog::EndMessage;

		result = false;
		goto end;
	}

	/* Prepare data for Shader Storage Buffer Object. */
	gl.getIntegerv(GL_SHADER_STORAGE_BUFFER_OFFSET_ALIGNMENT, &ssbo_alignment);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv() call failed.");

	int_alignment  = ssbo_alignment / int_size;
	ssbo_data_size = m_gl_max_geometry_shader_storage_blocks_ext_value * ssbo_alignment;
	ssbo_data	  = new glw::GLint[ssbo_data_size];

	if ((ssbo_alignment % int_size) != 0)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "GL_SHADER_STORAGE_BUFFER_OFFSET_ALIGNMENT query value "
													   "["
						   << ssbo_alignment << "]"
												"divide with remainder by the size of GLint "
												"["
						   << int_size << "]" << tcu::TestLog::EndMessage;

		result = false;
		goto end;
	}

	for (int i = 0; i < m_gl_max_geometry_shader_storage_blocks_ext_value; ++i)
	{
		ssbo_data[i * int_alignment] = i + 1;
	}

	/* Create Shader Storage Buffer Object. */
	gl.genBuffers(1, &m_ssbo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenBuffers() call failed.");

	gl.bindBuffer(GL_SHADER_STORAGE_BUFFER, m_ssbo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer() call failed for GL_SHADER_STORAGE_BUFFER pname.");

	gl.bufferData(GL_SHADER_STORAGE_BUFFER, ssbo_data_size, ssbo_data, GL_DYNAMIC_COPY);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferData() call failed.");

	/* Free unused memory. */
	delete[] ssbo_data;
	ssbo_data = NULL;

	/* Bind specific m_ssbo_id buffer region to a specific Shader Storage Buffer binding point. */
	for (glw::GLint n_ssb = 0; n_ssb < (m_gl_max_geometry_shader_storage_blocks_ext_value); ++n_ssb)
	{
		glw::GLuint offset = n_ssb * ssbo_alignment;

		gl.bindBufferRange(GL_SHADER_STORAGE_BUFFER, n_ssb /*binding index*/, m_ssbo_id, offset, int_size);
		GLU_EXPECT_NO_ERROR(gl.getError(), "bindBufferRange() call failed.");
	}

	/* Configure VAO. */
	gl.genVertexArrays(1, &m_vao_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenVertexArrays() call failed.");

	gl.bindVertexArray(m_vao_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindVertexArray() call failed.");

	/* Create a Buffer Object for Transform Feedback's outputs. */
	gl.genBuffers(1, &m_tfbo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenBuffers() call failed.");

	gl.bindBuffer(GL_ARRAY_BUFFER, m_tfbo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer() call failed.");

	gl.bufferData(GL_ARRAY_BUFFER, sizeof(glw::GLfloat) * 4 /* four float vector components */, initial_buffer_data,
				  GL_STATIC_READ);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferData() call failed.");

	/* Bind Buffer Object m_tfbo_id to GL_TRANSFORM_FEEDBACK_BUFFER binding point. */
	gl.bindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0 /*binding index*/, m_tfbo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBufferBase() call failed.");

	/* Use program. */
	gl.useProgram(m_po_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram() call failed.");

	/* Disable rasterization and make a draw call. After that, turn on rasterization. */
	gl.enable(GL_RASTERIZER_DISCARD);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glEnable() call failed for GL_RASTERIZER_DISCARD pname.");

	gl.beginTransformFeedback(GL_POINTS);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBeginTransformFeedback() call failed.");

	gl.drawArrays(GL_POINTS, 0 /*starting index*/, 1 /*number of indices*/);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawArrays() call failed for GL_POINTS pname.");

	gl.endTransformFeedback();
	GLU_EXPECT_NO_ERROR(gl.getError(), "glEndTransformFeedback() call failed.");

	gl.disable(GL_RASTERIZER_DISCARD);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glDisable() call failed for GL_RASTERIZER_DISCARD pname.");

	/* Retrieve value from Transform Feedback. */
	ptrTF_data = (glw::GLfloat*)gl.mapBufferRange(
		GL_ARRAY_BUFFER, 0 /*offset*/, sizeof(glw::GLfloat) * 4 /* four float vector components */, GL_MAP_READ_BIT);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glMapBufferRange() call failed.");

	counter = int(ptrTF_data[0] + 0.5f);

	gl.unmapBuffer(GL_ARRAY_BUFFER);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUnmapBuffer() call failed.");

	ptrTF_data = NULL;

	/* Retrieve values from Shader Storage Buffer Object. */
	ptrSSBO_data =
		(glw::GLint*)gl.mapBufferRange(GL_SHADER_STORAGE_BUFFER, 0 /*offset*/, ssbo_data_size, GL_MAP_READ_BIT);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glMapBufferRange() call failed.");

	for (int i = 0; i < m_gl_max_geometry_shader_storage_blocks_ext_value; ++i)
	{
		if (ptrSSBO_data[i * int_alignment] != i + 2)
		{
			result = false;

			m_testCtx.getLog() << tcu::TestLog::Message << "Value read from Shader Storage Buffer "
														   "["
							   << ptrSSBO_data[i * int_alignment] << "] "
																	 "at index "
																	 "["
							   << i * int_alignment << "]"
													   "is not equal to expected value "
													   "["
							   << i + 2 << "]" << tcu::TestLog::EndMessage;

			break;
		}
	}

	gl.unmapBuffer(GL_SHADER_STORAGE_BUFFER);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUnmapBuffer() call failed.");

	ptrSSBO_data = NULL;

	/* Calculate expected value. */
	expectedValue =
		m_gl_max_geometry_shader_storage_blocks_ext_value * (m_gl_max_geometry_shader_storage_blocks_ext_value + 1) / 2;

	if (counter != expectedValue)
	{
		result = false;
	}

end:
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
GeometryShaderMaxAtomicCountersTest::GeometryShaderMaxAtomicCountersTest(Context&			  context,
																		 const ExtParameters& extParams,
																		 const char* name, const char* description)
	: TestCaseBase(context, extParams, name, description)
	, m_acbo_id(0)
	, m_fs_id(0)
	, m_gl_max_geometry_atomic_counters_ext_value(0)
	, m_gs_id(0)
	, m_po_id(0)
	, m_vao_id(0)
	, m_vs_id(0)
{
}

/** Deinitializes GLES objects created during the test. */
void GeometryShaderMaxAtomicCountersTest::deinit()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (m_acbo_id != 0)
	{
		gl.deleteBuffers(1, &m_acbo_id);
		m_acbo_id = 0;
	}

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

	if (m_vao_id != 0)
	{
		gl.deleteVertexArrays(1, &m_vao_id);
		m_vao_id = 0;
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
std::string GeometryShaderMaxAtomicCountersTest::getGSCode()
{
	std::stringstream code_sstream;

	/* Form the GS */
	code_sstream << "${VERSION}\n"
					"${GEOMETRY_SHADER_REQUIRE}\n"
					"\n"
					"layout (points)                   in;\n"
					"layout (points, max_vertices = 1) out;\n"
					"\n"
					"uniform int n_loop_iterations;\n"
					"flat in int vertex_id[];\n"
					"\n";

	code_sstream << "layout(binding = 0) uniform atomic_uint acs[" << m_gl_max_geometry_atomic_counters_ext_value
				 << "];\n"
				 << "\n"
					"void main()\n"
					"{\n"
					"    for (int counter_id = 1;\n"
					"             counter_id <= n_loop_iterations;\n"
					"           ++counter_id)\n"
					"    {\n"
					"        if ((vertex_id[0] % counter_id) == 0)\n"
					"        {\n"
					"            atomicCounterIncrement(acs[counter_id - 1]);\n"
					"        }\n"
					"    }\n"
					"\n"
					"    gl_Position = vec4(1.0, 0.0, 0.0, 1.0);\n"
					"    EmitVertex();\n"
					"}\n";

	/* Form a specialized version of the GS source code */
	std::string gs_code				= code_sstream.str();
	const char* gs_code_raw			= gs_code.c_str();
	std::string gs_code_specialized = specializeShader(1, /* parts */ &gs_code_raw);

	return gs_code_specialized;
}

/** Executes the test.
 *
 *  Sets the test result to QP_TEST_RESULT_FAIL if the test failed, QP_TEST_RESULT_PASS otherwise.
 *  @return STOP if the test has finished, CONTINUE to indicate iterate should be called once again.
 *  Note the function throws exception should an error occur!
 **/
tcu::TestNode::IterateResult GeometryShaderMaxAtomicCountersTest::iterate()
{
	/* Define Vertex Shader's code for the purpose of this test. */
	const char* vs_code = "${VERSION}\n"
						  "\n"
						  "flat out int vertex_id;\n"
						  "\n"
						  "void main()\n"
						  "{\n"
						  "    vertex_id    = gl_VertexID;\n"
						  "    gl_Position  = vec4(1.0, 0.0, 0.0, 1.0);\n"
						  "}\n";

	bool			   has_shader_compilation_failed	  = true;
	glw::GLuint*	   initial_ac_data					  = DE_NULL;
	const unsigned int n_draw_call_vertices				  = 4;
	glw::GLint		   n_loop_iterations_uniform_location = -1;
	glw::GLuint*	   ptrACBO_data						  = DE_NULL;
	bool			   result							  = true;

	std::string fs_code_specialized		= "";
	const char* fs_code_specialized_raw = DE_NULL;
	std::string gs_code_specialized		= "";
	const char* gs_code_specialized_raw = DE_NULL;
	std::string vs_code_specialized		= "";
	const char* vs_code_specialized_raw = DE_NULL;

	/* This test should only run if EXT_geometry_shader is supported. */
	if (!m_is_geometry_shader_extension_supported)
	{
		throw tcu::NotSupportedError(GEOMETRY_SHADER_EXTENSION_NOT_SUPPORTED, "", __FILE__, __LINE__);
	}

	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Retrieve GL_MAX_GEOMETRY_ATOMIC_COUNTERS_EXT pname value */
	gl.getIntegerv(m_glExtTokens.MAX_GEOMETRY_ATOMIC_COUNTERS, &m_gl_max_geometry_atomic_counters_ext_value);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv() failed for GL_MAX_GEOMETRY_ATOMIC_COUNTERS_EXT pname");

	/* Check if m_gl_max_atomic_counters_value is less than or equal zero. */
	if (m_gl_max_geometry_atomic_counters_ext_value <= 0)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "GL_MAX_GEOMETRY_ATOMIC_COUNTERS_EXT query value "
						   << "[" << m_gl_max_geometry_atomic_counters_ext_value
						   << "]"
							  " is less than or equal to zero. Atomic Counters"
							  " in Geometry Shader are not supported."
						   << tcu::TestLog::EndMessage;

		if (m_gl_max_geometry_atomic_counters_ext_value == 0)
		{
			throw tcu::NotSupportedError("GL_MAX_GEOMETRY_ATOMIC_COUNTERS_EXT is 0");
		}
		else
		{
			result = false;
			goto end;
		}
	}

	/* Create a program object. */
	m_po_id = gl.createProgram();
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateProgram() call failed.");

	/* Create shader objects. */
	m_fs_id = gl.createShader(GL_FRAGMENT_SHADER);
	m_gs_id = gl.createShader(GL_GEOMETRY_SHADER);
	m_vs_id = gl.createShader(GL_VERTEX_SHADER);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateShader() call(s) failed.");

	/* Try to link the test program object */
	fs_code_specialized		= specializeShader(1, &dummy_fs_code);
	fs_code_specialized_raw = fs_code_specialized.c_str();

	gs_code_specialized		= getGSCode();
	gs_code_specialized_raw = gs_code_specialized.c_str();

	vs_code_specialized		= specializeShader(1, &vs_code);
	vs_code_specialized_raw = vs_code_specialized.c_str();

	if (!TestCaseBase::buildProgram(m_po_id, m_gs_id, 1,				  /* n_sh1_body_parts */
									&gs_code_specialized_raw, m_vs_id, 1, /* n_sh2_body_parts */
									&vs_code_specialized_raw, m_fs_id, 1, /* n_sh3_body_parts */
									&fs_code_specialized_raw, &has_shader_compilation_failed))
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Program object linking failed." << tcu::TestLog::EndMessage;

		result = false;
		goto end;
	}

	/* Create Atomic Counter Buffer Objects. */
	gl.genBuffers(1, &m_acbo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenBuffers() call failed.");

	/* Prepare initial data - zeroes - to fill the Atomic Counter Buffer Object. */
	initial_ac_data = new glw::GLuint[m_gl_max_geometry_atomic_counters_ext_value];
	memset(initial_ac_data, 0, sizeof(glw::GLuint) * m_gl_max_geometry_atomic_counters_ext_value);

	gl.bindBuffer(GL_ATOMIC_COUNTER_BUFFER, m_acbo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer() call failed for GL_SHADER_STORAGE_BUFFER pname.");

	gl.bufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(glw::GLuint) * m_gl_max_geometry_atomic_counters_ext_value, NULL,
				  GL_DYNAMIC_COPY);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferData() call failed.");

	gl.bufferSubData(GL_ATOMIC_COUNTER_BUFFER, 0 /*offset*/,
					 sizeof(glw::GLuint) * m_gl_max_geometry_atomic_counters_ext_value,
					 initial_ac_data /*initialize with zeroes*/);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferSubData() call failed.");

	gl.bindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 0 /*binding index*/, m_acbo_id /*buffer*/);
	GLU_EXPECT_NO_ERROR(gl.getError(), "bindBufferRange() call failed.");

	/* Configure VAO. */
	gl.genVertexArrays(1, &m_vao_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenVertexArrays() call failed.");

	gl.bindVertexArray(m_vao_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindVertexArray() call failed.");

	/* Use program. */
	n_loop_iterations_uniform_location = gl.getUniformLocation(m_po_id, "n_loop_iterations");

	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetUniformLocation() call failed.");
	if (n_loop_iterations_uniform_location == -1)
	{
		TCU_FAIL("n_loop_iterations uniform is considered inactive");
	}
	else
	{
		gl.useProgram(m_po_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram() call failed.");

		gl.uniform1i(n_loop_iterations_uniform_location, m_gl_max_geometry_atomic_counters_ext_value);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glUniform1i() call failed.");
	}

	/* Issue the draw call */
	gl.drawArrays(GL_POINTS, 0, n_draw_call_vertices);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawArrays() call failed for GL_POINTS pname.");

	/* Retrieve values from Atomic Counter Buffer Objects and check if these values are valid. */
	ptrACBO_data = (glw::GLuint*)gl.mapBufferRange(GL_ATOMIC_COUNTER_BUFFER, 0 /*offset*/,
												   sizeof(glw::GLuint) * m_gl_max_geometry_atomic_counters_ext_value,
												   GL_MAP_READ_BIT);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glMapBufferRange() call failed.");

	for (glw::GLint n_ac = 0; n_ac < m_gl_max_geometry_atomic_counters_ext_value; ++n_ac)
	{
		unsigned int expected_value = 0;

		for (unsigned int n_draw_call_vertex = 0; n_draw_call_vertex < n_draw_call_vertices; ++n_draw_call_vertex)
		{
			if ((n_draw_call_vertex % (n_ac + 1)) == 0)
			{
				++expected_value;
			}
		}

		if (ptrACBO_data[n_ac] != expected_value)
		{
			result = false;
			break;
		}
	}

	gl.unmapBuffer(GL_ATOMIC_COUNTER_BUFFER);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUnmapBuffer() call failed.");

	ptrACBO_data = NULL;

end:
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
GeometryShaderMaxAtomicCounterBuffersTest::GeometryShaderMaxAtomicCounterBuffersTest(Context&			  context,
																					 const ExtParameters& extParams,
																					 const char*		  name,
																					 const char*		  description)
	: TestCaseBase(context, extParams, name, description)
	, m_acbo_ids(NULL)
	, m_fs_id(0)
	, m_gl_max_atomic_counter_buffer_bindings_value(0)
	, m_gl_max_geometry_atomic_counter_buffers_ext_value(0)
	, m_gs_id(0)
	, m_po_id(0)
	, m_vao_id(0)
	, m_vs_id(0)
{
}

/** Deinitializes GLES objects created during the test. */
void GeometryShaderMaxAtomicCounterBuffersTest::deinit()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (m_acbo_ids != NULL)
	{
		if (m_gl_max_geometry_atomic_counter_buffers_ext_value > 0)
		{
			gl.deleteBuffers(m_gl_max_geometry_atomic_counter_buffers_ext_value, m_acbo_ids);

			delete[] m_acbo_ids;
			m_acbo_ids = NULL;
		}
	}

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

	if (m_vao_id != 0)
	{
		gl.deleteVertexArrays(1, &m_vao_id);
		m_vao_id = 0;
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
std::string GeometryShaderMaxAtomicCounterBuffersTest::getGSCode()
{
	std::stringstream code_sstream;

	/* Form the GS */
	code_sstream << "${VERSION}\n"
					"${GEOMETRY_SHADER_REQUIRE}\n"
					"\n"
					"layout (points)                   in;\n"
					"layout (points, max_vertices = 1) out;\n"
					"\n"
					"flat in int vertex_id[];\n"
					"\n";

	for (glw::GLint n_ac = 0; n_ac < (m_gl_max_geometry_atomic_counter_buffers_ext_value); ++n_ac)
	{
		code_sstream << "layout(binding = " << n_ac << ") uniform atomic_uint ac" << n_ac << ";\n";
	}

	code_sstream << "\n"
					"void main()\n"
					"{\n"
					"    for(int counter_id = 1; counter_id <= "
				 << m_gl_max_geometry_atomic_counter_buffers_ext_value
				 << "; ++counter_id)\n"
					"    {\n"
					"        if((vertex_id[0] % counter_id) == 0)\n"
					"        {\n";

	for (glw::GLint n_ac = 0; n_ac < (m_gl_max_geometry_atomic_counter_buffers_ext_value); ++n_ac)
	{
		code_sstream << "            atomicCounterIncrement(ac" << n_ac << ");\n";
	}

	code_sstream << "        }\n"
					"    }\n";

	code_sstream << "\n"
					"    gl_Position = vec4(1.0, 0.0, 0.0, 1.0);\n"
					"    EmitVertex();\n"
					"}\n";

	/* Form a specialized version of the GS source code */
	std::string gs_code				= code_sstream.str();
	const char* gs_code_raw			= gs_code.c_str();
	std::string gs_code_specialized = specializeShader(1, /* parts */ &gs_code_raw);

	return gs_code_specialized;
}

/** Executes the test.
 *
 *  Sets the test result to QP_TEST_RESULT_FAIL if the test failed, QP_TEST_RESULT_PASS otherwise.
 *  @return STOP if the test has finished, CONTINUE to indicate iterate should be called once again.
 *  Note the function throws exception should an error occur!
 **/
tcu::TestNode::IterateResult GeometryShaderMaxAtomicCounterBuffersTest::iterate()
{
	/* Define Vertex Shader's code for the purpose of this test. */
	const char* vs_code = "${VERSION}\n"
						  "\n"
						  "flat out int vertex_id;\n"
						  "\n"
						  "void main()\n"
						  "{\n"
						  "    vertex_id    = gl_VertexID;\n"
						  "    gl_Position  = vec4(1.0, 0.0, 0.0, 1.0);\n"
						  "}\n";

	unsigned int		  expected_value				= 0;
	const glw::Functions& gl							= m_context.getRenderContext().getFunctions();
	bool				  has_shader_compilation_failed = true;
	const glw::GLuint	 initial_ac_data				= 0;
	const glw::GLuint	 number_of_indices				= 128 * m_gl_max_geometry_atomic_counter_buffers_ext_value;
	bool				  result						= true;

	std::string fs_code_specialized		= "";
	const char* fs_code_specialized_raw = DE_NULL;
	std::string gs_code_specialized		= "";
	const char* gs_code_specialized_raw = DE_NULL;
	std::string vs_code_specialized		= "";
	const char* vs_code_specialized_raw = DE_NULL;

	/* This test should only run if EXT_geometry_shader is supported. */
	if (!m_is_geometry_shader_extension_supported)
	{
		throw tcu::NotSupportedError(GEOMETRY_SHADER_EXTENSION_NOT_SUPPORTED, "", __FILE__, __LINE__);
	}

	/* Retrieve GL_MAX_GEOMETRY_ATOMIC_COUNTER_BUFFERS_EXT pname value */
	gl.getIntegerv(m_glExtTokens.MAX_GEOMETRY_ATOMIC_COUNTER_BUFFERS,
				   &m_gl_max_geometry_atomic_counter_buffers_ext_value);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv() failed for GL_MAX_GEOMETRY_ATOMIC_COUNTER_BUFFERS_EXT pname");

	/* Retrieve GL_MAX_ATOMIC_COUNTER_BUFFER_BINDINGS pname value */
	gl.getIntegerv(GL_MAX_ATOMIC_COUNTER_BUFFER_BINDINGS, &m_gl_max_atomic_counter_buffer_bindings_value);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv() failed for GL_MAX_ATOMIC_COUNTER_BUFFER_BINDINGS pname");

	/* Check if m_gl_max_geometry_atomic_counter_buffers_ext_value is less than or equal zero. */
	if (m_gl_max_geometry_atomic_counter_buffers_ext_value <= 0)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "GL_MAX_GEOMETRY_ATOMIC_COUNTER_BUFFERS_EXT query value "
						   << "[" << m_gl_max_geometry_atomic_counter_buffers_ext_value
						   << "]"
							  " is less than or equal to zero. Atomic Counter Buffers"
							  " are not supported."
						   << tcu::TestLog::EndMessage;

		if (m_gl_max_geometry_atomic_counter_buffers_ext_value == 0)
		{
			throw tcu::NotSupportedError("GL_MAX_GEOMETRY_ATOMIC_COUNTER_BUFFERS_EXT is 0");
		}
		else
		{
			result = false;
			goto end;
		}
	}

	/* Check if m_gl_max_atomic_counter_buffer_bindings_value is less than m_gl_max_shader_storage_blocks_value. */
	if (m_gl_max_atomic_counter_buffer_bindings_value < m_gl_max_geometry_atomic_counter_buffers_ext_value)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "GL_MAX_GEOMETRY_ATOMIC_COUNTER_BUFFERS_EXT query value "
						   << "[" << m_gl_max_geometry_atomic_counter_buffers_ext_value
						   << "]"
							  " is greater than GL_MAX_ATOMIC_COUNTER_BUFFER_BINDINGS query value "
							  "["
						   << m_gl_max_atomic_counter_buffer_bindings_value << "]." << tcu::TestLog::EndMessage;

		result = false;
		goto end;
	}

	/* Create a program object. */
	m_po_id = gl.createProgram();
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateProgram() call failed.");

	/* Create shader objects. */
	m_fs_id = gl.createShader(GL_FRAGMENT_SHADER);
	m_gs_id = gl.createShader(GL_GEOMETRY_SHADER);
	m_vs_id = gl.createShader(GL_VERTEX_SHADER);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateShader() call(s) failed.");

	/* Try to link the test program object */
	fs_code_specialized		= specializeShader(1, &dummy_fs_code);
	fs_code_specialized_raw = fs_code_specialized.c_str();

	gs_code_specialized		= getGSCode();
	gs_code_specialized_raw = gs_code_specialized.c_str();

	vs_code_specialized		= specializeShader(1, &vs_code);
	vs_code_specialized_raw = vs_code_specialized.c_str();

	if (!TestCaseBase::buildProgram(m_po_id, m_gs_id, 1,				  /* n_sh1_body_parts */
									&gs_code_specialized_raw, m_vs_id, 1, /* n_sh2_body_parts */
									&vs_code_specialized_raw, m_fs_id, 1, /* n_sh3_body_parts */
									&fs_code_specialized_raw, &has_shader_compilation_failed))
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Program object linking failed." << tcu::TestLog::EndMessage;

		result = false;
		goto end;
	}

	/* Create Atomic Counter Buffer Objects. */
	m_acbo_ids = new glw::GLuint[m_gl_max_geometry_atomic_counter_buffers_ext_value];

	gl.genBuffers(m_gl_max_geometry_atomic_counter_buffers_ext_value, m_acbo_ids);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenBuffers() call failed.");

	for (glw::GLint n_acb = 0; n_acb < m_gl_max_geometry_atomic_counter_buffers_ext_value; ++n_acb)
	{
		gl.bindBuffer(GL_ATOMIC_COUNTER_BUFFER, m_acbo_ids[n_acb]);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer() call failed for GL_SHADER_STORAGE_BUFFER pname.");

		gl.bufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(glw::GLuint), &initial_ac_data /*initialize with zeroes*/,
					  GL_DYNAMIC_COPY);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferData() call failed.");

		gl.bindBufferBase(GL_ATOMIC_COUNTER_BUFFER, n_acb /*binding index*/, m_acbo_ids[n_acb] /*buffer*/);
		GLU_EXPECT_NO_ERROR(gl.getError(), "bindBufferRange() call failed.");
	}

	/* Configure VAO. */
	gl.genVertexArrays(1, &m_vao_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenVertexArrays() call failed.");

	gl.bindVertexArray(m_vao_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindVertexArray() call failed.");

	/* Use program. */
	gl.useProgram(m_po_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram() call failed.");

	gl.drawArrays(GL_POINTS, 0 /*starting index*/, number_of_indices);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawArrays() call failed for GL_POINTS pname.");

	/* Calculate expected value. */
	/* For each point being processed by Geometry Shader. */
	for (glw::GLuint vertex_id = 0; vertex_id < number_of_indices; ++vertex_id)
	{
		/* And for each atomic counter ID. */
		for (int atomic_counter_id = 1; atomic_counter_id <= m_gl_max_geometry_atomic_counter_buffers_ext_value;
			 ++atomic_counter_id)
		{
			/* Check if (vertex_id % atomic_counter_id) == 0. If it is true, increment expected_value. */
			if (vertex_id % atomic_counter_id == 0)
			{
				++expected_value;
			}
		}
	}

	/* Retrieve values from Atomic Counter Buffer Objects and check if these values are valid. */
	for (glw::GLint n_acb = 0; n_acb < m_gl_max_geometry_atomic_counter_buffers_ext_value; ++n_acb)
	{
		gl.bindBuffer(GL_ATOMIC_COUNTER_BUFFER, m_acbo_ids[n_acb]);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer() call failed.");

		glw::GLuint* ptrABO_data = (glw::GLuint*)gl.mapBufferRange(GL_ATOMIC_COUNTER_BUFFER, 0 /*offset*/,
																   sizeof(glw::GLuint) /*length*/, GL_MAP_READ_BIT);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glMapBufferRange() call failed.");

		if (ptrABO_data[0] != expected_value)
		{
			result = false;
			break;
		}

		gl.unmapBuffer(GL_ATOMIC_COUNTER_BUFFER);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glUnmapBuffer() call failed.");

		ptrABO_data = NULL;
	}

end:
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
GeometryShaderPiplineProgramObjectWithoutActiveVSProgramTest::
	GeometryShaderPiplineProgramObjectWithoutActiveVSProgramTest(Context& context, const ExtParameters& extParams,
																 const char* name, const char* description)
	: TestCaseBase(context, extParams, name, description)
	, m_fs_id(0)
	, m_fs_po_id(0)
	, m_gs_id(0)
	, m_gs_po_id(0)
	, m_ppo_id(0)
	, m_vao_id(0)
{
}

/** Deinitializes GLES objects created during the test. */
void GeometryShaderPiplineProgramObjectWithoutActiveVSProgramTest::deinit()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (m_fs_id != 0)
	{
		gl.deleteShader(m_fs_id);
		m_fs_id = 0;
	}

	if (m_fs_po_id != 0)
	{
		gl.deleteProgram(m_fs_po_id);
		m_fs_po_id = 0;
	}

	if (m_gs_id != 0)
	{
		gl.deleteShader(m_gs_id);
		m_gs_id = 0;
	}

	if (m_gs_po_id != 0)
	{
		gl.deleteProgram(m_gs_po_id);
		m_gs_po_id = 0;
	}

	if (m_ppo_id != 0)
	{
		gl.deleteProgramPipelines(1, &m_ppo_id);
		m_ppo_id = 0;
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
tcu::TestNode::IterateResult GeometryShaderPiplineProgramObjectWithoutActiveVSProgramTest::iterate()
{
	bool		has_shader_compilation_failed = true;
	bool		result						  = true;
	glw::GLenum error						  = GL_NO_ERROR;

	/* This test should only run if EXT_geometry_shader is supported. */
	if (!m_is_geometry_shader_extension_supported)
	{
		throw tcu::NotSupportedError(GEOMETRY_SHADER_EXTENSION_NOT_SUPPORTED, "", __FILE__, __LINE__);
	}

	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Create separable program objects. */
	m_fs_po_id = gl.createProgram();
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateProgram() call failed.");

	gl.programParameteri(m_fs_po_id, GL_PROGRAM_SEPARABLE, GL_TRUE);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glProgramParameteri() call failed.");

	m_gs_po_id = gl.createProgram();
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateProgram() call failed.");

	gl.programParameteri(m_gs_po_id, GL_PROGRAM_SEPARABLE, GL_TRUE);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glProgramParameteri() call failed.");

	/* Create shader objects. */
	m_fs_id = gl.createShader(GL_FRAGMENT_SHADER);
	m_gs_id = gl.createShader(GL_GEOMETRY_SHADER);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateShader() call(s) failed.");

	/* Try to link the test program object */
	std::string fs_code_specialized		= specializeShader(1, &dummy_fs_code);
	const char* fs_code_specialized_raw = fs_code_specialized.c_str();
	std::string gs_code_specialized		= specializeShader(1, &dummy_gs_code);
	const char* gs_code_specialized_raw = gs_code_specialized.c_str();

	if (!TestCaseBase::buildProgram(m_fs_po_id, m_fs_id, 1,			/* n_sh1_body_parts */
									&fs_code_specialized_raw, 0, 0, /* n_sh2_body_parts */
									NULL, 0, 0,						/* n_sh3_body_parts */
									NULL, &has_shader_compilation_failed))
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Fragment Shader Program object linking failed."
						   << tcu::TestLog::EndMessage;

		result = false;
		goto end;
	}

	if (!TestCaseBase::buildProgram(m_gs_po_id, m_gs_id, 1,			/* n_sh1_body_parts */
									&gs_code_specialized_raw, 0, 0, /* n_sh2_body_parts */
									NULL, 0, 0,						/* n_sh3_body_parts */
									NULL, &has_shader_compilation_failed))
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Geometry Shader Program object linking failed."
						   << tcu::TestLog::EndMessage;

		result = false;
		goto end;
	}

	/* Configure Pipeline Object. */
	gl.genProgramPipelines(1, &m_ppo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenProgramPipelines() call failed.");

	gl.useProgramStages(m_ppo_id, GL_FRAGMENT_SHADER_BIT, m_fs_po_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgramStages() call failed.");

	gl.useProgramStages(m_ppo_id, GL_GEOMETRY_SHADER_BIT, m_gs_po_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgramStages() call failed.");

	/* Configure VAO. */
	gl.genVertexArrays(1, &m_vao_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenVertexArrays() call failed.");

	gl.bindVertexArray(m_vao_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindVertexArray() call failed.");

	/* Use Program Pipeline Object. */
	gl.bindProgramPipeline(m_ppo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindProgramPipeline() call failed.");

	gl.drawArrays(GL_POINTS, 0 /*starting index*/, 1 /*number of indices*/);

	error = gl.getError();

	/* Check if correct error was generated. */
	if (GL_INVALID_OPERATION != error)
	{
		result = false;

		m_testCtx.getLog() << tcu::TestLog::Message << "Error different than GL_INVALID_OPEARATION was generated."
						   << tcu::TestLog::EndMessage;
	}

end:
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
GeometryShaderIncompatibleDrawCallModeTest::GeometryShaderIncompatibleDrawCallModeTest(Context&				context,
																					   const ExtParameters& extParams,
																					   const char*			name,
																					   const char*			description)
	: TestCaseBase(context, extParams, name, description)
	, m_fs_id(0)
	, m_gs_ids(NULL)
	, m_number_of_gs(5 /*taken from test spec*/)
	, m_po_ids(NULL)
{
	m_vao_id = 0;
	m_vs_id  = 0;
}

/** Deinitializes GLES objects created during the test. */
void GeometryShaderIncompatibleDrawCallModeTest::deinit()
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

	if (m_vao_id != 0)
	{
		gl.deleteVertexArrays(1, &m_vao_id);
		m_vao_id = 0;
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
tcu::TestNode::IterateResult GeometryShaderIncompatibleDrawCallModeTest::iterate()
{
	/* Define 5 Geometry Shaders for purpose of this test. */
	const char* gs_code_points = "${VERSION}\n"
								 "${GEOMETRY_SHADER_REQUIRE}\n"
								 "\n"
								 "layout (points)                   in;\n"
								 "layout (points, max_vertices = 1) out;\n"
								 "\n"
								 "${IN_PER_VERTEX_DECL_ARRAY}"
								 "\n"
								 "void main()\n"
								 "{\n"
								 "    gl_Position = gl_in[0].gl_Position;\n"
								 "    EmitVertex();\n"
								 "}\n";

	const char* gs_code_lines = "${VERSION}\n"
								"${GEOMETRY_SHADER_REQUIRE}\n"
								"\n"
								"layout (lines)                    in;\n"
								"layout (points, max_vertices = 1) out;\n"
								"\n"
								"${IN_PER_VERTEX_DECL_ARRAY}"
								"\n"
								"void main()\n"
								"{\n"
								"    gl_Position = gl_in[0].gl_Position;\n"
								"    EmitVertex();\n"
								"}\n";

	const char* gs_code_lines_adjacency = "${VERSION}\n"
										  "${GEOMETRY_SHADER_REQUIRE}\n"
										  "\n"
										  "layout (lines_adjacency)          in;\n"
										  "layout (points, max_vertices = 1) out;\n"
										  "\n"
										  "${IN_PER_VERTEX_DECL_ARRAY}"
										  "\n"
										  "void main()\n"
										  "{\n"
										  "    gl_Position = gl_in[0].gl_Position;\n"
										  "    EmitVertex();\n"
										  "}\n";

	const char* gs_code_triangles = "${VERSION}\n"
									"${GEOMETRY_SHADER_REQUIRE}\n"
									"\n"
									"layout (triangles)                in;\n"
									"layout (points, max_vertices = 1) out;\n"
									"\n"
									"${IN_PER_VERTEX_DECL_ARRAY}"
									"\n"
									"void main()\n"
									"{\n"
									"    gl_Position = gl_in[0].gl_Position;\n"
									"    EmitVertex();\n"
									"}\n";

	const char* gs_code_triangles_adjacency = "${VERSION}\n"
											  "${GEOMETRY_SHADER_REQUIRE}\n"
											  "\n"
											  "layout (triangles_adjacency)      in;\n"
											  "layout (points, max_vertices = 1) out;\n"
											  "\n"
											  "${IN_PER_VERTEX_DECL_ARRAY}"
											  "\n"
											  "void main()\n"
											  "{\n"
											  "    gl_Position = gl_in[0].gl_Position;\n"
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
		if (!TestCaseBase::buildProgram(m_po_ids[i], m_fs_id, 1,				  /* n_sh1_body_parts */
										&fs_code_specialized_raw, m_gs_ids[i], 1, /* n_sh2_body_parts */
										&gs_codes_specialized_raw[i], m_vs_id, 1, /* n_sh3_body_parts */
										&vs_code_specialized_raw, &has_shader_compilation_failed))
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "Program object linking failed for i = "
							   << "[" << i << "]." << tcu::TestLog::EndMessage;

			result = false;
			break;
		}
	}

	if (result)
	{
		/* Configure VAO. */
		gl.genVertexArrays(1, &m_vao_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGenVertexArrays() call failed.");

		gl.bindVertexArray(m_vao_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindVertexArray() call failed.");

		for (glw::GLuint po = 0; po < m_number_of_gs; ++po)
		{
			/* Use Program Object. */
			gl.useProgram(m_po_ids[po]);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram() call failed.");

			if (po != 0)
			{
				gl.drawArrays(GL_POINTS, 0 /*starting index*/, 1 /*number of indices*/);

				if (GL_INVALID_OPERATION != gl.getError())
				{
					result = false;

					m_testCtx.getLog() << tcu::TestLog::Message
									   << "Error different than GL_INVALID_OPEARATION was generated."
									   << tcu::TestLog::EndMessage;

					break;
				}
			}

			if (po != 1)
			{
				gl.drawArrays(GL_LINES, 0 /*starting index*/, 2 /*number of indices*/);

				if (GL_INVALID_OPERATION != gl.getError())
				{
					result = false;

					m_testCtx.getLog() << tcu::TestLog::Message
									   << "Error different than GL_INVALID_OPEARATION was generated."
									   << tcu::TestLog::EndMessage;

					break;
				}

				gl.drawArrays(GL_LINE_LOOP, 0 /*starting index*/, 2 /*number of indices*/);

				if (GL_INVALID_OPERATION != gl.getError())
				{
					result = false;

					m_testCtx.getLog() << tcu::TestLog::Message
									   << "Error different than GL_INVALID_OPEARATION was generated."
									   << tcu::TestLog::EndMessage;

					break;
				}

				gl.drawArrays(GL_LINE_STRIP, 0 /*starting index*/, 2 /*number of indices*/);

				if (GL_INVALID_OPERATION != gl.getError())
				{
					result = false;

					m_testCtx.getLog() << tcu::TestLog::Message
									   << "Error different than GL_INVALID_OPEARATION was generated."
									   << tcu::TestLog::EndMessage;

					break;
				}
			}

			if (po != 2)
			{
				gl.drawArrays(GL_LINES_ADJACENCY_EXT, 0 /*starting index*/, 4 /*number of indices*/);

				if (GL_INVALID_OPERATION != gl.getError())
				{
					result = false;

					m_testCtx.getLog() << tcu::TestLog::Message
									   << "Error different than GL_INVALID_OPEARATION was generated."
									   << tcu::TestLog::EndMessage;

					break;
				}

				gl.drawArrays(GL_LINE_STRIP_ADJACENCY_EXT, 0 /*starting index*/, 4 /*number of indices*/);

				if (GL_INVALID_OPERATION != gl.getError())
				{
					result = false;

					m_testCtx.getLog() << tcu::TestLog::Message
									   << "Error different than GL_INVALID_OPEARATION was generated."
									   << tcu::TestLog::EndMessage;

					break;
				}
			}

			if (po != 3)
			{
				gl.drawArrays(GL_TRIANGLES, 0 /*starting index*/, 3 /*number of indices*/);

				if (GL_INVALID_OPERATION != gl.getError())
				{
					result = false;

					m_testCtx.getLog() << tcu::TestLog::Message
									   << "Error different than GL_INVALID_OPEARATION was generated."
									   << tcu::TestLog::EndMessage;

					break;
				}

				gl.drawArrays(GL_TRIANGLE_FAN, 0 /*starting index*/, 3 /*number of indices*/);

				if (GL_INVALID_OPERATION != gl.getError())
				{
					result = false;

					m_testCtx.getLog() << tcu::TestLog::Message
									   << "Error different than GL_INVALID_OPEARATION was generated."
									   << tcu::TestLog::EndMessage;

					break;
				}

				gl.drawArrays(GL_TRIANGLE_STRIP, 0 /*starting index*/, 3 /*number of indices*/);

				if (GL_INVALID_OPERATION != gl.getError())
				{
					result = false;

					m_testCtx.getLog() << tcu::TestLog::Message
									   << "Error different than GL_INVALID_OPEARATION was generated."
									   << tcu::TestLog::EndMessage;

					break;
				}
			}

			if (po != 4)
			{
				gl.drawArrays(GL_TRIANGLES_ADJACENCY_EXT, 0 /*starting index*/, 6 /*number of indices*/);

				if (GL_INVALID_OPERATION != gl.getError())
				{
					result = false;

					m_testCtx.getLog() << tcu::TestLog::Message
									   << "Error different than GL_INVALID_OPEARATION was generated."
									   << tcu::TestLog::EndMessage;

					break;
				}

				gl.drawArrays(GL_TRIANGLE_STRIP_ADJACENCY_EXT, 0 /*starting index*/, 6 /*number of indices*/);

				if (GL_INVALID_OPERATION != gl.getError())
				{
					result = false;

					m_testCtx.getLog() << tcu::TestLog::Message
									   << "Error different than GL_INVALID_OPEARATION was generated."
									   << tcu::TestLog::EndMessage;

					break;
				}
			}
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
GeometryShaderInsufficientEmittedVerticesTest::GeometryShaderInsufficientEmittedVerticesTest(
	Context& context, const ExtParameters& extParams, const char* name, const char* description)
	: TestCaseBase(context, extParams, name, description)
	, m_fbo_id(0)
	, m_fs_id(0)
	, m_gs_ids(NULL)
	, m_number_of_color_components(4)
	, m_number_of_gs(2 /*taken from test spec*/)
	, m_po_ids(NULL)
	, m_texture_height(16)
	, m_texture_id(0)
	, m_texture_width(16)
{
	m_vao_id = 0;
	m_vs_id  = 0;

	/* Allocate enough memory for glReadPixels() data which is respectively: width, height, RGBA components number. */
	m_pixels = new glw::GLubyte[m_texture_height * m_texture_width * m_number_of_color_components];
}

/** Deinitializes GLES objects created during the test. */
void GeometryShaderInsufficientEmittedVerticesTest::deinit()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.clearColor(0.0f, 0.0f, 0.0f, 0.0f);

	if (m_pixels != NULL)
	{
		delete[] m_pixels;
		m_pixels = NULL;
	}

	if (m_fbo_id != 0)
	{
		gl.deleteFramebuffers(1, &m_fbo_id);
		m_fbo_id = 0;
	}

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

	if (m_texture_id != 0)
	{
		gl.deleteTextures(1, &m_texture_id);
		m_texture_id = 0;
	}

	if (m_vao_id != 0)
	{
		gl.deleteVertexArrays(1, &m_vao_id);
		m_vao_id = 0;
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
tcu::TestNode::IterateResult GeometryShaderInsufficientEmittedVerticesTest::iterate()
{
	/* Define Fragment Shader for purpose of this test. */
	const char* fs_code = "${VERSION}\n"
						  "\n"
						  "precision highp float;\n"
						  "\n"
						  "out vec4 result;\n"
						  "\n"
						  "void main()\n"
						  "{\n"
						  "    result = vec4(1.0, 0.0, 0.0, 0.0);\n"
						  "}\n";

	/* Define 2 Geometry Shaders for purpose of this test. */
	const char* gs_line_strip = "${VERSION}\n"
								"${GEOMETRY_SHADER_REQUIRE}\n"
								"\n"
								"layout (points)                       in;\n"
								"layout (line_strip, max_vertices = 2) out;\n"
								"\n"
								"${IN_PER_VERTEX_DECL_ARRAY}"
								"\n"
								"void main()\n"
								"{\n"
								"    gl_Position    = gl_in[0].gl_Position;\n"
								"    gl_Position.zw = vec2(0.0, 1.0);\n"
								"    EmitVertex();\n"
								"}\n";

	const char* gs_triangle_strip = "${VERSION}\n"
									"${GEOMETRY_SHADER_REQUIRE}\n"
									"\n"
									"layout (points)                           in;\n"
									"layout (triangle_strip, max_vertices = 3) out;\n"
									"\n"
									"${IN_PER_VERTEX_DECL_ARRAY}"
									"\n"
									"void main()\n"
									"{\n"
									"    gl_Position    = gl_in[0].gl_Position;\n"
									"    gl_Position.zw = vec2(0.0, 1.0);\n"
									"    EmitVertex();\n"

									"    gl_Position    = gl_in[0].gl_Position;\n"
									"    gl_Position.zw = vec2(0.0, 1.0);\n"
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
	std::string fs_code_specialized		= specializeShader(1, &fs_code);
	const char* fs_code_specialized_raw = fs_code_specialized.c_str();

	std::string gs_codes_specialized[] = { specializeShader(1, &gs_line_strip),
										   specializeShader(1, &gs_triangle_strip) };

	const char* gs_codes_specialized_raw[] = { gs_codes_specialized[0].c_str(), gs_codes_specialized[1].c_str() };

	std::string vs_code_specialized		= specializeShader(1, &dummy_vs_code);
	const char* vs_code_specialized_raw = vs_code_specialized.c_str();

	for (glw::GLuint i = 0; i < m_number_of_gs; ++i)
	{
		if (!TestCaseBase::buildProgram(m_po_ids[i], m_fs_id, 1,				  /* n_sh1_body_parts */
										&fs_code_specialized_raw, m_gs_ids[i], 1, /* n_sh2_body_parts */
										&gs_codes_specialized_raw[i], m_vs_id, 1, /* n_sh3_body_parts */
										&vs_code_specialized_raw, &has_shader_compilation_failed))
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "Program object linking failed for i = "
							   << "[" << i << "]." << tcu::TestLog::EndMessage;

			result = false;
			break;
		}
	}

	if (result)
	{
		/* Create a 2D texture. */
		gl.genTextures(1, &m_texture_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGenTextures() call failed.");

		gl.bindTexture(GL_TEXTURE_2D, m_texture_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture() call failed.");

		gl.texStorage2D(GL_TEXTURE_2D, 1 /*levels*/, GL_RGBA8, 16 /*width taken from spec*/,
						16 /*height taken from spec*/);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glTexStorage2D() call failed.");

		/* Configure FBO. */
		gl.genFramebuffers(1, &m_fbo_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGenFramebuffers() call failed.");

		gl.bindFramebuffer(GL_FRAMEBUFFER, m_fbo_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffer() call failed.");

		gl.framebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_texture_id, 0 /*level*/);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glFramebufferTexture2D() call failed.");

		/* Configure VAO. */
		gl.genVertexArrays(1, &m_vao_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGenVertexArrays() call failed.");

		gl.bindVertexArray(m_vao_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindVertexArray() call failed.");

		gl.clearColor(0.0f, 1.0f, 0.0f, 0.0f);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glClearColor() call failed.");

		for (glw::GLuint po = 0; po < m_number_of_gs; ++po)
		{
			/* Use Program Object. */
			gl.useProgram(m_po_ids[po]);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram() call failed.");

			gl.clear(GL_COLOR_BUFFER_BIT);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glClear() call failed.");

			gl.drawArrays(GL_POINTS, 0 /*first*/, 1 /*count*/);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawArrays() call failed for GL_POINTS pname.");

			gl.readPixels(0 /*x*/, 0 /*y*/, m_texture_width, m_texture_height, GL_RGBA, GL_UNSIGNED_BYTE, m_pixels);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glReadPixels() call failed.");

			for (glw::GLuint pixel = 0; pixel < (m_texture_width * m_texture_height * m_number_of_color_components -
												 m_number_of_color_components);
				 pixel += m_number_of_color_components)
			{
				if (m_pixels[pixel] != 0 && m_pixels[pixel + 1] != 255 && m_pixels[pixel + 2] != 0 &&
					m_pixels[pixel + 3] != 0)
				{
					result = false;

					m_testCtx.getLog() << tcu::TestLog::Message << "Pixel [" << pixel << "] has color = ["
									   << m_pixels[pixel] << ", " << m_pixels[pixel + 1] << ", " << m_pixels[pixel + 2]
									   << ", " << m_pixels[pixel + 3] << "] "
									   << "instead of [0, 255, 0, 0]." << tcu::TestLog::EndMessage;

					break;
				}
			}
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
GeometryShaderPipelineObjectTransformFeedbackVertexAndGeometryShaderCaptureTest::
	GeometryShaderPipelineObjectTransformFeedbackVertexAndGeometryShaderCaptureTest(Context&			 context,
																					const ExtParameters& extParams,
																					const char*			 name,
																					const char*			 description)
	: TestCaseBase(context, extParams, name, description)
	, m_gs_id(0)
	, m_gs_po_id(0)
	, m_ppo_id(0)
	, m_tfbo_id(0)
	, m_vao_id(0)
	, m_vs_id(0)
	, m_vs_po_id(0)
{
}

/** Deinitializes GLES objects created during the test. */
void GeometryShaderPipelineObjectTransformFeedbackVertexAndGeometryShaderCaptureTest::deinit()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (m_gs_id != 0)
	{
		gl.deleteShader(m_gs_id);
		m_gs_id = 0;
	}

	if (m_gs_po_id != 0)
	{
		gl.deleteProgram(m_gs_po_id);
		m_gs_po_id = 0;
	}

	if (m_ppo_id != 0)
	{
		gl.deleteProgramPipelines(1, &m_ppo_id);
		m_ppo_id = 0;
	}

	if (m_tfbo_id != 0)
	{
		gl.deleteBuffers(1, &m_tfbo_id);
		m_tfbo_id = 0;
	}

	if (m_vao_id != 0)
	{
		gl.deleteVertexArrays(1, &m_vao_id);
		m_vao_id = 0;
	}

	if (m_vs_id != 0)
	{
		gl.deleteShader(m_vs_id);
		m_vs_id = 0;
	}

	if (m_vs_po_id != 0)
	{
		gl.deleteProgram(m_vs_po_id);
		m_vs_po_id = 0;
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
tcu::TestNode::IterateResult GeometryShaderPipelineObjectTransformFeedbackVertexAndGeometryShaderCaptureTest::iterate()
{
	/* Define Geometry Shader for purpose of this test. */
	const char* gs_code =
		"${VERSION}\n"
		"${GEOMETRY_SHADER_REQUIRE}\n"
		"${IN_PER_VERTEX_DECL_ARRAY}\n"
		"${OUT_PER_VERTEX_DECL}\n"
		"\n"
		"layout (points)                   in;\n"
		"layout (points, max_vertices = 1) out;\n"
		"\n"
		"flat in int   vertexID[];\n"
		"flat in ivec4 out_vs_1[];\n"
		"\n"
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
						  "${OUT_PER_VERTEX_DECL}\n"
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

	bool		  has_shader_compilation_failed = true;
	bool		  result						= true;
	glw::GLfloat* ptrTF_data_f					= NULL;
	glw::GLuint*  ptrTF_data_ui					= NULL;
	glw::GLfloat  expected_geom_results[]		= { 0.0f, 1.0f, 2.0f, 3.0f };
	glw::GLuint   expected_vertex_results[]		= { 0, 1, 2, 3 };
	glw::GLfloat  epsilon						= 1e-5f;

	/* This test should only run if EXT_geometry_shader is supported. */
	if (!m_is_geometry_shader_extension_supported)
	{
		throw tcu::NotSupportedError(GEOMETRY_SHADER_EXTENSION_NOT_SUPPORTED, "", __FILE__, __LINE__);
	}

	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Create separable program objects. */
	m_gs_po_id = gl.createProgram();
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateProgram() call failed.");

	gl.programParameteri(m_gs_po_id, GL_PROGRAM_SEPARABLE, GL_TRUE);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glProgramParameteri() call failed.");

	m_vs_po_id = gl.createProgram();
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateProgram() call failed.");

	gl.programParameteri(m_vs_po_id, GL_PROGRAM_SEPARABLE, GL_TRUE);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glProgramParameteri() call failed.");

	/* Create shader objects. */
	m_gs_id = gl.createShader(GL_GEOMETRY_SHADER);
	m_vs_id = gl.createShader(GL_VERTEX_SHADER);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateShader() call(s) failed.");

	/* Try to link the test program object */
	std::string gs_code_specialized		= specializeShader(1, &gs_code);
	const char* gs_code_specialized_raw = gs_code_specialized.c_str();

	std::string vs_code_specialized		= specializeShader(1, &vs_code);
	const char* vs_code_specialized_raw = vs_code_specialized.c_str();

	/* Specify output variables to be captured. */
	const char* tf_varyings[2] = { "out_gs_1", "out_vs_1" };

	gl.transformFeedbackVaryings(m_gs_po_id, 1 /*count*/, &tf_varyings[0], GL_INTERLEAVED_ATTRIBS);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTransformFeedbackVaryings() call(s) failed.");

	gl.transformFeedbackVaryings(m_vs_po_id, 1 /*count*/, &tf_varyings[1], GL_INTERLEAVED_ATTRIBS);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTransformFeedbackVaryings() call(s) failed.");

	if (!TestCaseBase::buildProgram(m_gs_po_id, m_gs_id, 1,			/* n_sh1_body_parts */
									&gs_code_specialized_raw, 0, 0, /* n_sh2_body_parts */
									NULL, 0, 0,						/* n_sh3_body_parts */
									NULL, &has_shader_compilation_failed))
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Geometry Shader Program object linking failed."
						   << tcu::TestLog::EndMessage;

		result = false;
		goto end;
	}

	if (!TestCaseBase::buildProgram(m_vs_po_id, m_vs_id, 1,			/* n_sh1_body_parts */
									&vs_code_specialized_raw, 0, 0, /* n_sh2_body_parts */
									NULL, 0, 0,						/* n_sh3_body_parts */
									NULL, &has_shader_compilation_failed))
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Geometry Shader Program object linking failed."
						   << tcu::TestLog::EndMessage;

		result = false;
		goto end;
	}

	/* Create and configure Program Pipeline Object. */
	gl.genProgramPipelines(1, &m_ppo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenProgramPipelines() call(s) failed.");

	gl.useProgramStages(m_ppo_id, GL_GEOMETRY_SHADER_BIT, m_gs_po_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgramStages() call(s) failed.");

	gl.useProgramStages(m_ppo_id, GL_VERTEX_SHADER_BIT, m_vs_po_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgramStages() call(s) failed.");

	/* Create Vertex Array Object. */
	gl.genVertexArrays(1, &m_vao_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenVertexArrays() call(s) failed.");

	gl.bindVertexArray(m_vao_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindVertexArray() call(s) failed.");

	/* Create Buffer Object for Transform Feedback data. */
	gl.genBuffers(1, &m_tfbo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenBuffers() call(s) failed.");

	gl.bindBuffer(GL_ARRAY_BUFFER, m_tfbo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer() call(s) failed.");

	gl.bufferData(GL_ARRAY_BUFFER, sizeof(glw::GLfloat) * 4, NULL, GL_STREAM_READ);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferData() call(s) failed.");

	gl.bindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0 /*binding index*/, m_tfbo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBufferBase() call(s) failed.");

	/* Ensure that there is no program object already bound and bind program pipeline. */
	gl.useProgram(0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram() call(s) failed.");

	gl.bindProgramPipeline(m_ppo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindProgramPipeline() call(s) failed.");

	gl.enable(GL_RASTERIZER_DISCARD);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glEnable() call(s) failed.");

	/* First pass - Vertex and Geometry Shaders On. */
	gl.beginTransformFeedback(GL_POINTS);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBeginTransformFeedback() call(s) failed.");

	gl.drawArrays(GL_POINTS, 0 /*first*/, 1 /*count*/);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawArrays() call(s) failed.");

	gl.endTransformFeedback();
	GLU_EXPECT_NO_ERROR(gl.getError(), "glEndTransformFeedback() call(s) failed.");

	/* Retrieve data and check if it is correct. */
	ptrTF_data_f =
		(glw::GLfloat*)gl.mapBufferRange(GL_TRANSFORM_FEEDBACK_BUFFER, 0 /*offset*/,
										 sizeof(glw::GLfloat) * 4 /* four float vector components */, GL_MAP_READ_BIT);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glMapBufferRange() call failed.");

	for (size_t i = 0; i < 4; ++i)
	{
		if (fabs(ptrTF_data_f[i] - expected_geom_results[i]) >= epsilon)
		{
			result = false;
			break;
		}
	}

	gl.unmapBuffer(GL_TRANSFORM_FEEDBACK_BUFFER);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUnmapBuffer() call failed.");

	ptrTF_data_f = NULL;

	if (!result)
	{
		goto end;
	}

	/* Deactivate Geometry Shader Program Object from Program Pipeline Object. */
	gl.useProgramStages(m_ppo_id, GL_GEOMETRY_SHADER_BIT, 0 /* program */);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgramStages() call(s) failed.");

	/* Second pass - only Vertex Shader Program On. */
	gl.beginTransformFeedback(GL_POINTS);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBeginTransformFeedback() call(s) failed.");

	gl.drawArrays(GL_POINTS, 0 /*first*/, 1 /*count*/);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawArrays() call(s) failed.");

	gl.endTransformFeedback();
	GLU_EXPECT_NO_ERROR(gl.getError(), "glEndTransformFeedback() call(s) failed.");

	gl.disable(GL_RASTERIZER_DISCARD);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glDisable() call(s) failed.");

	/* Retrieve data and check if it is correct. */
	ptrTF_data_ui =
		(glw::GLuint*)gl.mapBufferRange(GL_TRANSFORM_FEEDBACK_BUFFER, 0 /*offset*/,
										sizeof(glw::GLuint) * 4 /* four float vector components */, GL_MAP_READ_BIT);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glMapBufferRange() call failed.");

	for (size_t i = 0; i < 4; ++i)
	{
		if (ptrTF_data_ui[i] != expected_vertex_results[i])
		{
			result = false;
			break;
		}
	}

	gl.unmapBuffer(GL_TRANSFORM_FEEDBACK_BUFFER);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUnmapBuffer() call failed.");

	ptrTF_data_ui = NULL;

end:
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
GeometryShaderDrawPrimitivesDoNotMatchOutputPrimitives::GeometryShaderDrawPrimitivesDoNotMatchOutputPrimitives(
	Context& context, const ExtParameters& extParams, const char* name, const char* description)
	: TestCaseBase(context, extParams, name, description), m_fs_id(0), m_gs_id(0), m_po_id(0), m_tfbo_id(0)
{
	m_vao_id = 0;
	m_vs_id  = 0;
}

/** Deinitializes GLES objects created during the test. */
void GeometryShaderDrawPrimitivesDoNotMatchOutputPrimitives::deinit()
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

	if (m_tfbo_id != 0)
	{
		gl.deleteBuffers(1, &m_tfbo_id);
		m_tfbo_id = 0;
	}

	if (m_vao_id != 0)
	{
		gl.deleteVertexArrays(1, &m_vao_id);
		m_vao_id = 0;
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
tcu::TestNode::IterateResult GeometryShaderDrawPrimitivesDoNotMatchOutputPrimitives::iterate()
{
	/* Define Geometry Shader for purpose of this test. */
	const char* gs_code = "${VERSION}\n"
						  "${GEOMETRY_SHADER_REQUIRE}\n"
						  "\n"
						  "layout (lines)                            in;\n"
						  "layout (triangle_strip, max_vertices = 3) out;\n"
						  "\n"
						  "out vec4 out_gs_1;\n"
						  "\n"
						  "void main()\n"
						  "{\n"
						  "    out_gs_1 = vec4(4.0, 3.0, 2.0, 1.0);\n"
						  "\n"
						  "    gl_Position = vec4(0, 0, 0, 1);\n"
						  "    EmitVertex();\n"
						  "\n"
						  "    gl_Position = vec4(1, 0, 0, 1);\n"
						  "    EmitVertex();\n"
						  "\n"
						  "    gl_Position = vec4(1, 1, 0, 1);\n"
						  "    EmitVertex();\n"
						  "\n"
						  "    EndPrimitive();"
						  "}\n";

	bool		has_shader_compilation_failed = true;
	bool		result						  = true;
	glw::GLenum error						  = GL_NO_ERROR;

	/* This test should only run if EXT_geometry_shader is supported. */
	if (!m_is_geometry_shader_extension_supported)
	{
		throw tcu::NotSupportedError(GEOMETRY_SHADER_EXTENSION_NOT_SUPPORTED, "", __FILE__, __LINE__);
	}

	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Create program object. */
	m_po_id = gl.createProgram();
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateProgram() call failed.");

	/* Specify output variables to be captured. */
	const char* tf_varyings[] = { "out_gs_1" };

	gl.transformFeedbackVaryings(m_po_id, 1 /*count*/, tf_varyings, GL_INTERLEAVED_ATTRIBS);
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
	const char* gs_code_specialized_raw = gs_code_specialized.c_str();

	std::string vs_code_specialized		= specializeShader(1, &dummy_vs_code);
	const char* vs_code_specialized_raw = vs_code_specialized.c_str();

	if (!TestCaseBase::buildProgram(m_po_id, m_fs_id, 1,				  /* n_sh1_body_parts */
									&fs_code_specialized_raw, m_gs_id, 1, /* n_sh2_body_parts */
									&gs_code_specialized_raw, m_vs_id, 1, /* n_sh3_body_parts */
									&vs_code_specialized_raw, &has_shader_compilation_failed))
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Program object linking failed whereas a success was expected."
						   << tcu::TestLog::EndMessage;

		result = false;
		goto end;
	}

	/* Create Vertex Array Object. */
	gl.genVertexArrays(1, &m_vao_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenVertexArrays() call(s) failed.");

	gl.bindVertexArray(m_vao_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindVertexArray() call(s) failed.");

	/* Create Buffer Object for Transform Feedback data. */
	gl.genBuffers(1, &m_tfbo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenBuffers() call(s) failed.");

	gl.bindBuffer(GL_ARRAY_BUFFER, m_tfbo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer() call(s) failed.");

	gl.bufferData(GL_ARRAY_BUFFER,
				  sizeof(glw::GLfloat) * 4 * 3 /* capture 4 float vector components times 3 triangle vertices */, NULL,
				  GL_STREAM_READ);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferData() call(s) failed.");

	gl.bindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0 /*binding index*/, m_tfbo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBufferBase() call(s) failed.");

	/* Turn on program object. */
	gl.useProgram(m_po_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram() call(s) failed.");

	gl.enable(GL_RASTERIZER_DISCARD);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glEnable() call(s) failed.");

	gl.beginTransformFeedback(GL_LINES);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBeginTransformFeedback() call(s) failed.");

	gl.drawArrays(GL_TRIANGLES, 0 /*first*/, 3 /*count*/);

	error = gl.getError();

	if (error != GL_INVALID_OPERATION)
	{
		result = false;

		m_testCtx.getLog() << tcu::TestLog::Message << "Error different than GL_INVALID_OPEARATION was generated."
						   << tcu::TestLog::EndMessage;
	}

	gl.endTransformFeedback();
	GLU_EXPECT_NO_ERROR(gl.getError(), "glEndTransformFeedback() call(s) failed.");

	gl.disable(GL_RASTERIZER_DISCARD);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glDisable() call(s) failed.");

end:

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
GeometryShaderDrawCallsWhileTFPaused::GeometryShaderDrawCallsWhileTFPaused(Context&				context,
																		   const ExtParameters& extParams,
																		   const char* name, const char* description)
	: TestCaseBase(context, extParams, name, description), m_fs_id(0), m_gs_id(0), m_tfbo_id(0)
{
	m_vao_id = 0;
	m_vs_id  = 0;
}

/** Deinitializes GLES objects created during the test. */
void GeometryShaderDrawCallsWhileTFPaused::deinit()
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

	for (int i = 0; i < 15 /* All combinations of possible inputs and outputs in GS */; ++i)
	{
		if (m_po_ids[i] != 0)
		{
			gl.deleteProgram(m_po_ids[i]);
			m_po_ids[i] = 0;
		}
	}

	if (m_tfbo_id != 0)
	{
		gl.deleteBuffers(1, &m_tfbo_id);
		m_tfbo_id = 0;
	}

	if (m_vao_id != 0)
	{
		gl.deleteVertexArrays(1, &m_vao_id);
		m_vao_id = 0;
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
tcu::TestNode::IterateResult GeometryShaderDrawCallsWhileTFPaused::iterate()
{
	/* Define 15 (all combinations of possible inputs and outputs in geometry shader) Geometry Shaders for purpose of this test. */
	const std::string gs_inputs[]  = { "points", "lines", "lines_adjacency", "triangles", "triangles_adjacency" };
	const std::string gs_outputs[] = { "points", "line_strip", "triangle_strip" };
	const std::string gs_max_output_vertices[] = { "1", "2", "3" };

	const unsigned short number_of_combinations =
		(sizeof(gs_inputs) / sizeof(gs_inputs[0]) * (sizeof(gs_outputs) / sizeof(gs_outputs[0])));

	std::string gs_codes[number_of_combinations];
	glw::GLenum errorCode;

	for (size_t i = 0; i < (sizeof(gs_inputs) / sizeof(gs_inputs[0])) /*5 possible GS inputs*/; ++i)
	{
		for (size_t j = 0; j < (sizeof(gs_outputs) / sizeof(gs_outputs[0])) /*3 possible GS outputs*/; ++j)
		{
			/* This shader will not emit primitives for anything but points.
			 * We do so, because we just need to make sure that, while transform feedback
			 * is paused, all draw calls executed with an active program object which
			 * includes a geometry shader, are valid.
			 */
			gs_codes[j + 3 * i] = "${VERSION}\n"
								  "${GEOMETRY_SHADER_REQUIRE}\n"
								  "\n"
								  "layout (" +
								  gs_inputs[i] + ") in;\n"
												 "layout (" +
								  gs_outputs[j] + ", max_vertices = " + gs_max_output_vertices[j] +
								  ") out;\n"
								  "\n"
								  "out vec2 out_gs_1;\n"
								  "\n"
								  "void main()\n"
								  "{\n"
								  "    out_gs_1    = vec2(1.0, 2.0);\n"
								  "    gl_Position = vec4(0, 0, 0, 1);\n"
								  "    EmitVertex();\n"
								  "}\n";
		}
	}

	bool			  has_shader_compilation_failed = true;
	bool			  result						= true;
	const glw::GLuint tf_modes[3]					= { GL_POINTS, GL_LINES, GL_TRIANGLES };
	const glw::GLuint draw_call_modes[5]			= { GL_POINTS, GL_LINES, GL_LINES_ADJACENCY, GL_TRIANGLES,
											 GL_TRIANGLES_ADJACENCY };

	/* This test should only run if EXT_geometry_shader is supported. */
	if (!m_is_geometry_shader_extension_supported)
	{
		throw tcu::NotSupportedError(GEOMETRY_SHADER_EXTENSION_NOT_SUPPORTED, "", __FILE__, __LINE__);
	}

	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Create program objects. */
	for (int i = 0; i < number_of_combinations; ++i)
	{
		m_po_ids[i] = gl.createProgram();
	}
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateProgram() call(s) failed.");

	/* Specify output variables to be captured. */
	const char* tf_varyings[] = { "out_gs_1" };

	for (int i = 0; i < number_of_combinations; ++i)
	{
		gl.transformFeedbackVaryings(m_po_ids[i], 1 /*count*/, tf_varyings, GL_INTERLEAVED_ATTRIBS);
	}
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTransformFeedbackVaryings() call(s) failed.");

	/* Create shader objects. */
	m_fs_id = gl.createShader(GL_FRAGMENT_SHADER);
	m_gs_id = gl.createShader(GL_GEOMETRY_SHADER);
	m_vs_id = gl.createShader(GL_VERTEX_SHADER);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateShader() call(s) failed.");

	/* Try to link the test program object */
	std::string fs_code_specialized		= specializeShader(1, &dummy_fs_code);
	const char* fs_code_specialized_raw = fs_code_specialized.c_str();

	std::string vs_code_specialized		= specializeShader(1, &dummy_vs_code);
	const char* vs_code_specialized_raw = vs_code_specialized.c_str();

	for (int i = 0; i < number_of_combinations; ++i)
	{
		const char* gs_code					= gs_codes[i].c_str();
		std::string gs_code_specialized		= specializeShader(1, &gs_code);
		const char* gs_code_specialized_raw = gs_code_specialized.c_str();

		if (!TestCaseBase::buildProgram(m_po_ids[i], m_fs_id, 1,			  /* n_sh1_body_parts */
										&fs_code_specialized_raw, m_gs_id, 1, /* n_sh2_body_parts */
										&gs_code_specialized_raw, m_vs_id, 1, /* n_sh3_body_parts */
										&vs_code_specialized_raw, &has_shader_compilation_failed))
		{
			m_testCtx.getLog() << tcu::TestLog::Message
							   << "Program object linking failed whereas a success was expected."
							   << tcu::TestLog::EndMessage;

			result = false;
		}
	}

	if (!result)
	{
		goto end;
	}

	/* Create Vertex Array Object. */
	gl.genVertexArrays(1, &m_vao_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenVertexArrays() call(s) failed.");

	gl.bindVertexArray(m_vao_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindVertexArray() call(s) failed.");

	/* Create Buffer Object for Transform Feedback data. */
	gl.genBuffers(1, &m_tfbo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenBuffers() call(s) failed.");

	gl.bindBuffer(GL_ARRAY_BUFFER, m_tfbo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer() call(s) failed.");

	gl.bufferData(GL_ARRAY_BUFFER,
				  sizeof(glw::GLfloat) * 2 * 3 /* capture 2 float vector components times 3 triangle vertices */, NULL,
				  GL_STREAM_READ);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferData() call(s) failed.");

	gl.bindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0 /*binding index*/, m_tfbo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBufferBase() call(s) failed.");

	gl.enable(GL_RASTERIZER_DISCARD);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glEnable() call(s) failed.");

	for (int i = 0; i < 3 /* number of TF modes */ && result; ++i)
	{
		for (int j = 0; j < 5 /*number of draw call modes*/ && result; ++j)
		{
			for (int k = 0; k < 3 /* number of output GS primitive types */; ++k)
			{
				/* Turn on program object. */
				gl.useProgram(m_po_ids[k + 3 * j]);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram() call(s) failed.");

				gl.beginTransformFeedback(tf_modes[i]);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glBeginTransformFeedback() call(s) failed.");

				gl.pauseTransformFeedback();
				GLU_EXPECT_NO_ERROR(gl.getError(), "glPauseTransformFeedback() call(s) failed.");

				gl.drawArrays(draw_call_modes[j], 0 /*first*/, 3 /*count*/);
				errorCode = gl.getError();

				gl.resumeTransformFeedback();
				GLU_EXPECT_NO_ERROR(gl.getError(), "glResumeTransformFeedback() call(s) failed.");

				gl.endTransformFeedback();
				GLU_EXPECT_NO_ERROR(gl.getError(), "glEndTransformFeedback() call(s) failed.");

				/* If draw call fails stop test execution. */
				if (GL_NO_ERROR != errorCode)
				{
					m_testCtx.getLog() << tcu::TestLog::Message
									   << "glDrawArrays() call generated an error while transform feedback was paused."
									   << tcu::TestLog::EndMessage;

					result = false;
					break;
				}
			}
		}
	}

	gl.disable(GL_RASTERIZER_DISCARD);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glDisable() call(s) failed.");

end:

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
