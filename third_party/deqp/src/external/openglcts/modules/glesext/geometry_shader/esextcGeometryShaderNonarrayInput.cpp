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

#include "esextcGeometryShaderNonarrayInput.hpp"

#include "gluContextInfo.hpp"
#include "gluDefs.hpp"
#include "glwEnums.hpp"
#include "glwFunctions.hpp"
#include "tcuTestLog.hpp"

#include <algorithm>
#include <string>
#include <vector>

namespace glcts
{

/* Fragment shader code */
const char* GeometryShaderNonarrayInputCase::m_fs_code = "${VERSION}\n"
														 "\n"
														 "precision highp float;\n"
														 "\n"
														 "out vec4 color;\n"
														 "\n"
														 "void main()\n"
														 "{\n"
														 "    color = vec4(0, 1, 0, 1);\n"
														 "}\n";

/* Geometry shader body parts */
const char* GeometryShaderNonarrayInputCase::m_gs_code_preamble = "${VERSION}\n"
																  "${GEOMETRY_SHADER_REQUIRE}\n"
																  "\n";

const char* GeometryShaderNonarrayInputCase::m_gs_code_body = "layout(points)                         in;\n"
															  "layout(triangle_strip, max_vertices=4) out;\n"
															  "\n"
															  "#ifndef USE_BLOCK\n"
															  "    #define V1 v1[0]\n"
															  "    #define V2 v2[0]\n"
															  "    #define V3 v3[0]\n"
															  "\n"
															  "    in vec4 v1[];\n"
															  "    in vec4 v2[];\n"
															  "    in vec4 v3[];\n"
															  "    \n"
															  "    #ifdef CORRUPT\n"
															  "        #define V4 v4\n"
															  "        in vec4 v4;\n"
															  "    #else\n"
															  "        #define V4 v4[0]\n"
															  "        in vec4 v4[];\n"
															  "    #endif\n"
															  "#else\n"
															  "    in VS_GS\n"
															  "    {\n"
															  "        in vec4 v1;\n"
															  "        in vec4 v2;\n"
															  "        in vec4 v3;\n"
															  "        in vec4 v4;\n"
															  "    #ifdef CORRUPT\n"
															  "        } interface_block;\n"
															  "\n"
															  "        #define V1 interface_block.v1\n"
															  "        #define V2 interface_block.v2\n"
															  "        #define V3 interface_block.v3\n"
															  "        #define V4 interface_block.v4\n"
															  "    #else\n"
															  "        } interface_block[];\n"
															  "\n"
															  "        #define V1 interface_block[0].v1\n"
															  "        #define V2 interface_block[0].v2\n"
															  "        #define V3 interface_block[0].v3\n"
															  "        #define V4 interface_block[0].v4\n"
															  "    #endif\n"
															  "#endif\n"
															  "\n"
															  "void main()\n"
															  "{\n"
															  "    gl_Position = V1;\n"
															  "    EmitVertex();\n"
															  "    gl_Position = V2;\n"
															  "    EmitVertex();\n"
															  "    gl_Position = V3;\n"
															  "    EmitVertex();\n"
															  "    gl_Position = V4;\n"
															  "    EmitVertex();\n"
															  "    EndPrimitive();\n"
															  "}\n";

/* Vertex shader body parts */
const char* GeometryShaderNonarrayInputCase::m_vs_code_preamble = "${VERSION}\n"
																  "\n";

const char* GeometryShaderNonarrayInputCase::m_vs_code_body = "#ifndef USE_BLOCK\n"
															  "    #define V1 v1\n"
															  "    #define V2 v2\n"
															  "    #define V3 v3\n"
															  "    #define V4 v4\n"
															  "\n"
															  "    out vec4 v1;\n"
															  "    out vec4 v2;\n"
															  "    out vec4 v3;\n"
															  "    out vec4 v4;\n"
															  "#else\n"
															  "    ${SHADER_IO_BLOCKS_ENABLE}\n"
															  "    #define V1 interface_block.v1\n"
															  "    #define V2 interface_block.v2\n"
															  "    #define V3 interface_block.v3\n"
															  "    #define V4 interface_block.v4\n"
															  "\n"
															  "    out VS_GS\n"
															  "    {\n"
															  "        vec4 v1;\n"
															  "        vec4 v2;\n"
															  "        vec4 v3;\n"
															  "        vec4 v4;\n"
															  "    } interface_block;\n"
															  "#endif\n"
															  "\n"
															  "void main()\n"
															  "{\n"
															  "    V1 = vec4(-1, -1, 0, 1);\n"
															  "    V2 = vec4(-1,  1, 0, 1);\n"
															  "    V3 = vec4( 1, -1, 0, 1);\n"
															  "    V4 = vec4( 1,  1, 0, 1);\n"
															  "}\n";

/* Definitions */
#define TEXTURE_HEIGHT (4)
#define TEXTURE_WIDTH (4)

/** Constructor
 *
 * @param context       Test context
 * @param name          Test case's name
 * @param description   Test case's desricption
 **/
GeometryShaderNonarrayInputCase::GeometryShaderNonarrayInputCase(Context& context, const ExtParameters& extParams,
																 const char* name, const char* description)
	: TestCaseBase(context, extParams, name, description)
	, m_fbo_id(0)
	, m_fs(0)
	, m_gs_invalid_non_ib(0)
	, m_gs_invalid_ib(0)
	, m_gs_valid_non_ib(0)
	, m_gs_valid_ib(0)
	, m_po_a_invalid(0)
	, m_po_b_invalid(0)
	, m_po_a_valid(0)
	, m_po_b_valid(0)
	, m_to_id(0)
	, m_vao_id(0)
	, m_vs_valid_ib(0)
	, m_vs_valid_non_ib(0)
{
}

/** Executes the test.
 *  Sets the test result to QP_TEST_RESULT_FAIL if the test failed, QP_TEST_RESULT_PASS otherwise.
 *  @return STOP if the test has finished, CONTINUE to indicate iterate should be called once again.
 *  Note the function throws exception should an error occur!
 **/
tcu::TestNode::IterateResult GeometryShaderNonarrayInputCase::iterate(void)
{
	glw::GLint			  compile_status = GL_FALSE;
	const glw::Functions& gl			 = m_context.getRenderContext().getFunctions();
	glw::GLint			  link_status	= GL_FALSE;
	unsigned int		  m				 = 0;
	unsigned int		  n				 = 0;

	/* Form the shaders */
	const char* fs_parts[]				  = { m_fs_code };
	const char* gs_invalid_non_ib_parts[] = { m_gs_code_preamble, "#define CORRUPT\n", m_gs_code_body };
	const char* gs_invalid_ib_parts[] = { m_gs_code_preamble, "#define CORRUPT\n#define USE_BLOCK\n", m_gs_code_body };
	const char* gs_valid_non_ib_parts[] = { m_gs_code_preamble, m_gs_code_body };
	const char* gs_valid_ib_parts[]		= { m_gs_code_preamble, "#define USE_BLOCK\n", m_gs_code_body };
	const char* vs_valid_non_ib_parts[] = { m_vs_code_preamble, m_vs_code_body };
	const char* vs_valid_ib_parts[]		= { m_vs_code_preamble, "#define USE_BLOCK\n", m_vs_code_body };

	/* This test should only run if EXT_geometry_shader is supported.
	 * Note that EXT_shader_io_blocks support is implied. */
	if (!m_is_geometry_shader_extension_supported)
	{
		throw tcu::NotSupportedError(GEOMETRY_SHADER_EXTENSION_NOT_SUPPORTED, "", __FILE__, __LINE__);
	}

	/* Create program objects */
	m_po_a_invalid = gl.createProgram();
	m_po_b_invalid = gl.createProgram();

	/* Create shader objects */
	m_fs				= gl.createShader(GL_FRAGMENT_SHADER);
	m_gs_invalid_non_ib = gl.createShader(m_glExtTokens.GEOMETRY_SHADER);
	m_gs_invalid_ib		= gl.createShader(m_glExtTokens.GEOMETRY_SHADER);
	m_gs_valid_non_ib   = gl.createShader(m_glExtTokens.GEOMETRY_SHADER);
	m_gs_valid_ib		= gl.createShader(m_glExtTokens.GEOMETRY_SHADER);
	m_vs_valid_non_ib   = gl.createShader(GL_VERTEX_SHADER);
	m_vs_valid_ib		= gl.createShader(GL_VERTEX_SHADER);

	shaderSourceSpecialized(m_fs, DE_LENGTH_OF_ARRAY(fs_parts), fs_parts);
	shaderSourceSpecialized(m_gs_invalid_non_ib, DE_LENGTH_OF_ARRAY(gs_invalid_non_ib_parts), gs_invalid_non_ib_parts);
	shaderSourceSpecialized(m_gs_invalid_ib, DE_LENGTH_OF_ARRAY(gs_invalid_ib_parts), gs_invalid_ib_parts);
	shaderSourceSpecialized(m_gs_valid_non_ib, DE_LENGTH_OF_ARRAY(gs_valid_non_ib_parts), gs_valid_non_ib_parts);
	shaderSourceSpecialized(m_gs_valid_ib, DE_LENGTH_OF_ARRAY(gs_valid_ib_parts), gs_valid_ib_parts);
	shaderSourceSpecialized(m_vs_valid_non_ib, DE_LENGTH_OF_ARRAY(vs_valid_non_ib_parts), vs_valid_non_ib_parts);
	shaderSourceSpecialized(m_vs_valid_ib, DE_LENGTH_OF_ARRAY(vs_valid_ib_parts), vs_valid_ib_parts);

	/* Create and form invalid programs */
	gl.attachShader(m_po_a_invalid, m_fs);
	gl.attachShader(m_po_a_invalid, m_gs_invalid_non_ib);
	gl.attachShader(m_po_a_invalid, m_vs_valid_non_ib);

	gl.attachShader(m_po_b_invalid, m_fs);
	gl.attachShader(m_po_b_invalid, m_gs_invalid_ib);
	gl.attachShader(m_po_b_invalid, m_vs_valid_ib);

	/* Try to compile the shaders. Do not check GL_COMPILE_STATUS - we expect a linking failure */
	gl.compileShader(m_fs);
	gl.compileShader(m_gs_invalid_non_ib);
	gl.compileShader(m_gs_invalid_ib);
	gl.compileShader(m_vs_valid_non_ib);
	gl.compileShader(m_vs_valid_ib);

	/* Try to link the programs */
	gl.linkProgram(m_po_a_invalid);
	gl.getProgramiv(m_po_a_invalid, GL_LINK_STATUS, &link_status);

	if (link_status != GL_FALSE)
	{
		m_testCtx.getLog() << tcu::TestLog::Message
						   << "Program linked sucessfully although it shouldn't because geometry shaders are not "
							  "expected to support non-array input attributes."
						   << tcu::TestLog::EndMessage;

		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
		return STOP;
	}

	gl.linkProgram(m_po_b_invalid);
	gl.getProgramiv(m_po_b_invalid, GL_LINK_STATUS, &link_status);

	if (link_status != GL_FALSE)
	{
		m_testCtx.getLog() << tcu::TestLog::Message
						   << "Program linked sucessfully although it shouldn't because geometry shaders are not "
							  "expected to support non-array block input attributes."
						   << tcu::TestLog::EndMessage;

		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
		return STOP;
	}

	/* Release the programs before we continue */
	gl.deleteProgram(m_po_a_invalid);
	gl.deleteProgram(m_po_b_invalid);

	m_po_a_invalid = 0;
	m_po_b_invalid = 0;

	/* Release the invalid geometry shaders */
	gl.deleteShader(m_gs_invalid_non_ib);
	gl.deleteShader(m_gs_invalid_ib);

	m_gs_invalid_non_ib = 0;
	m_gs_invalid_ib		= 0;

	/* Create and form valid programs */
	m_po_a_valid = gl.createProgram();
	m_po_b_valid = gl.createProgram();

	gl.attachShader(m_po_a_valid, m_fs);
	gl.attachShader(m_po_a_valid, m_gs_valid_non_ib);
	gl.attachShader(m_po_a_valid, m_vs_valid_non_ib);

	gl.attachShader(m_po_b_valid, m_fs);
	gl.attachShader(m_po_b_valid, m_gs_valid_ib);
	gl.attachShader(m_po_b_valid, m_vs_valid_ib);

	gl.compileShader(m_gs_valid_non_ib);
	gl.compileShader(m_gs_valid_ib);

	gl.getShaderiv(m_gs_valid_non_ib, GL_COMPILE_STATUS, &compile_status);

	if (compile_status != GL_TRUE)
	{
		std::string log = getCompilationInfoLog(m_gs_valid_non_ib);

		m_testCtx.getLog() << tcu::TestLog::Message << "Valid geometry shader didn't compile. Error message: " << log
						   << tcu::TestLog::EndMessage;

		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
		return STOP;
	}

	gl.getShaderiv(m_gs_valid_ib, GL_COMPILE_STATUS, &compile_status);

	if (compile_status != GL_TRUE)
	{
		std::string log = getCompilationInfoLog(m_gs_valid_ib);

		m_testCtx.getLog() << tcu::TestLog::Message << "Valid geometry shader didn't compile. Error message: " << log
						   << tcu::TestLog::EndMessage;

		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
		return STOP;
	}

	gl.linkProgram(m_po_a_valid);
	gl.getProgramiv(m_po_a_valid, GL_LINK_STATUS, &link_status);

	if (link_status != GL_TRUE)
	{
		std::string log = getLinkingInfoLog(m_po_a_valid);

		m_testCtx.getLog() << tcu::TestLog::Message
						   << "Program failed to link although it was expected to link successfully. Error message: "
						   << log << tcu::TestLog::EndMessage;

		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
		return STOP;
	}

	gl.linkProgram(m_po_b_valid);
	gl.getProgramiv(m_po_b_valid, GL_LINK_STATUS, &link_status);

	if (link_status != GL_TRUE)
	{
		std::string log = getLinkingInfoLog(m_po_b_valid);

		m_testCtx.getLog() << tcu::TestLog::Message
						   << "Program failed to link although it was expected to link successfully. Error message: "
						   << log << tcu::TestLog::EndMessage;

		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
		return STOP;
	}

	/* Set up a FBO */
	gl.genFramebuffers(1, &m_fbo_id);
	gl.bindFramebuffer(GL_FRAMEBUFFER, m_fbo_id);

	gl.genTextures(1, &m_to_id);
	gl.bindTexture(GL_TEXTURE_2D, m_to_id);
	gl.texStorage2D(GL_TEXTURE_2D, 1 /* levels */, GL_RGBA8, TEXTURE_WIDTH, TEXTURE_HEIGHT);
	gl.framebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_to_id, 0 /* level */);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Failed to set up a framebuffer object");

	gl.viewport(0 /* x */, 0 /* y */, TEXTURE_WIDTH, TEXTURE_HEIGHT);

	/* Generate and bind a VAO */
	gl.genVertexArrays(1, &m_vao_id);
	gl.bindVertexArray(m_vao_id);

	/* Set up clear color */
	gl.clearColor(1.0f, 0, 0, 0);

	/* Use both program objects and verify they work correctly */
	for (m = 0; m < 2 /* programs */; ++m)
	{
		glw::GLuint   program = (m == 0) ? m_po_a_valid : m_po_b_valid;
		unsigned char result[TEXTURE_WIDTH * TEXTURE_HEIGHT * 4 /*components */];

		/* Clear the color attachment before we continue */
		gl.clear(GL_COLOR_BUFFER_BIT);

		/* Render! */
		gl.useProgram(program);
		gl.drawArrays(GL_POINTS, 0 /* first */, 1 /* count */);

		GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawArrays() call failed");

		/* Read back the result */
		gl.readPixels(0 /* x */, 0 /* y */, TEXTURE_WIDTH, TEXTURE_HEIGHT, GL_RGBA, GL_UNSIGNED_BYTE, result);

		GLU_EXPECT_NO_ERROR(gl.getError(), "Could not read back pixels from color attachment 0");

		/* Verify the result data is correct */
		for (n = 0; n < TEXTURE_WIDTH * TEXTURE_HEIGHT; ++n)
		{
			if (result[n * 4 + 0] != 0 || result[n * 4 + 1] != 255 || result[n * 4 + 2] != 0 ||
				result[n * 4 + 3] != 255)
			{
				m_testCtx.getLog() << tcu::TestLog::Message
								   << "Pixel data isn't correct. All pixels should have green color. Pixel at: "
								   << "[" << n / 4 << ", " << n % 4 << "] "
								   << "has color: "
								   << "[" << result[n * 4 + 0] << ", " << result[n * 4 + 1] << ", " << result[n * 4 + 2]
								   << ", " << result[n * 4 + 3] << "]" << tcu::TestLog::EndMessage;

				m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
				return STOP;
			} /* if (result data is invalid) */
		}	 /* for (all pixels) */
	}		  /* for (all programs) */

	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	return STOP;
}

/** Deinitializes GLES objects created during the test.
 *
 */
void GeometryShaderNonarrayInputCase::deinit(void)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Reset OpenGL ES state */
	gl.useProgram(0);
	gl.bindFramebuffer(GL_FRAMEBUFFER, 0);
	gl.bindVertexArray(0);

	if (m_po_a_valid != 0)
	{
		gl.deleteProgram(m_po_a_valid);
	}

	if (m_po_b_valid != 0)
	{
		gl.deleteProgram(m_po_b_valid);
	}

	if (m_fs != 0)
	{
		gl.deleteShader(m_fs);
	}

	if (m_gs_invalid_ib != 0)
	{
		gl.deleteShader(m_gs_invalid_ib);
	}

	if (m_gs_invalid_non_ib != 0)
	{
		gl.deleteShader(m_gs_invalid_non_ib);
	}

	if (m_gs_valid_ib != 0)
	{
		gl.deleteShader(m_gs_valid_ib);
	}

	if (m_gs_valid_non_ib != 0)
	{
		gl.deleteShader(m_gs_valid_non_ib);
	}

	if (m_vs_valid_ib != 0)
	{
		gl.deleteShader(m_vs_valid_ib);
	}

	if (m_vs_valid_non_ib != 0)
	{
		gl.deleteShader(m_vs_valid_non_ib);
	}

	if (m_to_id != 0)
	{
		gl.deleteTextures(1, &m_to_id);
	}

	if (m_fbo_id != 0)
	{
		gl.deleteFramebuffers(1, &m_fbo_id);
	}

	if (m_vao_id != 0)
	{
		gl.deleteVertexArrays(1, &m_vao_id);
	}

	/* Release base class */
	TestCaseBase::deinit();
}

} // namespace glcts
