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

#include "esextcGeometryShaderClipping.hpp"

#include "gluDefs.hpp"
#include "glwEnums.hpp"
#include "glwFunctions.hpp"
#include "tcuTestLog.hpp"
#include <cstring>

namespace glcts
{
/* Fragment shader code */
const char* GeometryShaderClipping::m_fs_code = "${VERSION}\n"
												"\n"
												"precision highp float;\n"
												"\n"
												"out vec4 result;\n"
												"\n"
												"void main()\n"
												"{\n"
												"    result = vec4(0, 1, 0, 0);\n"
												"}\n";

/* Vertex shader code */
const char* GeometryShaderClipping::m_vs_code = "${VERSION}\n"
												"\n"
												"precision highp float;\n"
												"\n"
												"void main()\n"
												"{\n"
												"    gl_Position = vec4(-10, -10, -10, 0);\n"
												"}\n";

/* Geometry shader code */
const char* GeometryShaderClipping::m_gs_code = "${VERSION}\n"
												"${GEOMETRY_SHADER_REQUIRE}\n"
												"\n"
												"precision highp float;\n"
												"\n"
												"layout(points)                                    in;\n"
												"layout(triangle_strip, max_vertices=4) out;\n"
												"\n"
												"void main()\n"
												"{\n"
												"\n"
												"    gl_Position = vec4(1, 1, 0, 1);\n"
												"    EmitVertex();\n"
												"\n"
												"    gl_Position = vec4(1, -1, 0, 1);\n"
												"    EmitVertex();\n"
												"\n"
												"    gl_Position = vec4(-1, 1, 0, 1);\n"
												"    EmitVertex();\n"
												"\n"
												"    gl_Position = vec4(-1, -1, 0, 1);\n"
												"    EmitVertex();\n"
												"\n"
												"    EndPrimitive();\n"
												"}\n";

/** Constructor
 *
 * @param context       Test context
 * @param name          Test case's name
 * @param description   Test case's desricption
 **/
GeometryShaderClipping::GeometryShaderClipping(Context& context, const ExtParameters& extParams, const char* name,
											   const char* description)
	: TestCaseBase(context, extParams, name, description)
	, m_fbo_id(0)
	, m_fs_id(0)
	, m_gs_id(0)
	, m_po_id(0)
	, m_to_id(0)
	, m_vao_id(0)
	, m_vs_id(0)
{
}

/** Deinitializes GLES objects created during the test.
 *
 */
void GeometryShaderClipping::deinit(void)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Reset OpenGL ES state */
	gl.useProgram(0);
	gl.bindTexture(GL_TEXTURE_2D, 0);
	gl.bindFramebuffer(GL_FRAMEBUFFER, 0);
	gl.bindVertexArray(0);

	if (m_po_id != 0)
	{
		gl.deleteProgram(m_po_id);
	}

	if (m_fs_id != 0)
	{
		gl.deleteShader(m_fs_id);
	}

	if (m_gs_id != 0)
	{
		gl.deleteShader(m_gs_id);
	}

	if (m_vs_id != 0)
	{
		gl.deleteShader(m_vs_id);
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

/** Executes the test.
 *  Sets the test result to QP_TEST_RESULT_FAIL if the test failed, QP_TEST_RESULT_PASS otherwise.
 *  @return STOP if the test has finished, CONTINUE to indicate iterate should be called once again.
 *  Note the function throws exception should an error occur!
 **/
tcu::TestNode::IterateResult GeometryShaderClipping::iterate(void)
{
	unsigned char buffer[m_texture_width * m_texture_height * m_texture_n_components];

	const glw::Functions& gl				= m_context.getRenderContext().getFunctions();
	const unsigned char   reference_color[] = { 0, 255, 0, 0 };
	const unsigned int	row_size			= m_texture_width * m_texture_n_components;

	if (!m_is_geometry_shader_extension_supported)
	{
		throw tcu::NotSupportedError(GEOMETRY_SHADER_EXTENSION_NOT_SUPPORTED, "", __FILE__, __LINE__);
	}

	/* Create shader objects we'll need */
	m_fs_id = gl.createShader(GL_FRAGMENT_SHADER);
	m_gs_id = gl.createShader(m_glExtTokens.GEOMETRY_SHADER);
	m_vs_id = gl.createShader(GL_VERTEX_SHADER);
	m_po_id = gl.createProgram();

	if (!buildProgram(m_po_id, m_fs_id, 1 /* part */, &m_fs_code, m_gs_id, 1 /* part */, &m_gs_code, m_vs_id,
					  1 /* part */, &m_vs_code))
	{
		TCU_FAIL("Could not create program object from valid vertex/geometry/fragment shaders!");
	}

	/* Create and configure texture object we'll be rendering to */
	gl.genTextures(1, &m_to_id);
	gl.bindTexture(GL_TEXTURE_2D, m_to_id);
	gl.texStorage2D(GL_TEXTURE_2D, m_texture_n_levels, GL_RGBA8, m_texture_width, m_texture_height);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Error creating texture object!");

	/* Create and configure the framebuffer object */
	gl.genFramebuffers(1, &m_fbo_id);
	gl.bindFramebuffer(GL_DRAW_FRAMEBUFFER, m_fbo_id);
	gl.framebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_to_id, 0 /* level */);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Error configuring framebuffer object!");

	/* Create and bind vertex array object */
	gl.genVertexArrays(1, &m_vao_id);
	gl.bindVertexArray(m_vao_id);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Error configuring vertex array object");

	/* Render */
	gl.viewport(0 /* x */, 0 /* y */, m_texture_width /* width */, m_texture_height /* height */);
	gl.useProgram(m_po_id);

	gl.clearColor(1.0f, 0.0f, 0.0f, 0.0f);
	gl.clear(GL_COLOR_BUFFER_BIT);

	gl.drawArrays(GL_POINTS, 0 /* first */, 1 /* count */);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Rendering failed!");

	/* Read the rendered data */
	gl.bindFramebuffer(GL_READ_FRAMEBUFFER, m_fbo_id);
	gl.readPixels(0 /* x */, 0 /* y */, m_texture_height /* width */, m_texture_width /* height */, GL_RGBA,
				  GL_UNSIGNED_BYTE, buffer);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Reading pixels failed!");

	/* Loop over all pixels and compare the rendered data with reference value */
	for (int y = 0; y < m_texture_height; ++y)
	{
		unsigned char* data_row = buffer + y * row_size;

		for (int x = 0; x < m_texture_width; ++x)
		{
			unsigned char* data = data_row + x * m_texture_n_components;

			if (memcmp(data, reference_color, sizeof(reference_color)) != 0)
			{
				m_testCtx.getLog() << tcu::TestLog::Message << "Rendered data [" << data[0] << ", " << data[1] << ", "
								   << data[2] << ", " << data[3] << "] is different from reference data ["
								   << reference_color[0] << ", " << reference_color[1] << ", " << reference_color[2]
								   << ", " << reference_color[3] << "] !" << tcu::TestLog::EndMessage;

				m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");

				return STOP;
			} /* if (memcmp(data, reference_color, sizeof(reference_color)) != 0) */
		}	 /* for (all columns) */
	}		  /* for (all rows) */

	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	return STOP;
}

} // namespace glcts
