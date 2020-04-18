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

#include "esextcGeometryShaderLayeredRenderingFBONoAttachment.hpp"

#include "gluDefs.hpp"
#include "tcuTestLog.hpp"

namespace glcts
{
/* Fragment shader code */
const char* GeometryShaderLayeredRenderingFBONoAttachment::m_fs_code =
	"${VERSION}\n"
	"\n"
	"${GEOMETRY_SHADER_REQUIRE}\n"
	"\n"
	"precision highp float;\n"
	"\n"
	"layout(rgba32i, binding = 0) writeonly uniform highp iimage2DArray array_image;\n"
	"\n"
	"     in  vec2 uv;\n"
	"flat in  int  layer_id;\n"
	"out vec4 color;\n"
	"\n"
	"void main()\n"
	"{\n"
	"    imageStore(array_image, ivec3( int(128.0 * uv.x), int(128.0 * uv.y), layer_id ), ivec4(0, 255, 0, 0) );\n"
	"}\n";

/* Geometry shader code */
const char* GeometryShaderLayeredRenderingFBONoAttachment::m_gs_code = "${VERSION}\n"
																	   "\n"
																	   "${GEOMETRY_SHADER_REQUIRE}\n"
																	   "\n"
																	   "precision highp float;\n"
																	   "\n"
																	   "layout(points)                          in;\n"
																	   "layout(triangle_strip, max_vertices=16) out;\n"
																	   "\n"
																	   "     out vec2 uv;\n"
																	   "flat out int  layer_id;\n"
																	   "\n"
																	   "void main()\n"
																	   "{\n"
																	   "    for (int n = 0; n < 4; ++n)\n"
																	   "    {\n"
																	   "        gl_Position = vec4(1, -1, 0, 1);\n"
																	   "        gl_Layer    = n;\n"
																	   "        layer_id    = n;\n"
																	   "        uv          = vec2(1, 0);\n"
																	   "        EmitVertex();\n"
																	   "\n"
																	   "        gl_Position = vec4(1,  1, 0, 1);\n"
																	   "        gl_Layer    = n;\n"
																	   "        layer_id    = n;\n"
																	   "        uv          = vec2(1, 1);\n"
																	   "        EmitVertex();\n"
																	   "\n"
																	   "        gl_Position = vec4(-1, -1, 0, 1);\n"
																	   "        gl_Layer    = n;\n"
																	   "        layer_id    = n;\n"
																	   "        uv          = vec2(0, 0);\n"
																	   "        EmitVertex();\n"
																	   "\n"
																	   "        gl_Position = vec4(-1,  1, 0, 1);\n"
																	   "        gl_Layer    = n;\n"
																	   "        layer_id    = n;\n"
																	   "        uv          = vec2(0, 1);\n"
																	   "        EmitVertex();\n"
																	   "\n"
																	   "        EndPrimitive();\n"
																	   "    }\n"
																	   "}\n";

/* Vertex shader code */
const char* GeometryShaderLayeredRenderingFBONoAttachment::m_vs_code = "${VERSION}\n"
																	   "\n"
																	   "${GEOMETRY_SHADER_REQUIRE}\n"
																	   "\n"
																	   "precision highp float;\n"
																	   "\n"
																	   "void main()\n"
																	   "{\n"
																	   "}\n";

/* Constants */
const glw::GLint GeometryShaderLayeredRenderingFBONoAttachment::m_height			   = 128;
const glw::GLint GeometryShaderLayeredRenderingFBONoAttachment::m_width				   = 128;
const int		 GeometryShaderLayeredRenderingFBONoAttachment::m_n_layers			   = 4;
const glw::GLint GeometryShaderLayeredRenderingFBONoAttachment::m_n_texture_components = 4;

/** Constructor
 *
 * @param context       Test context
 * @param name          Test case's name
 * @param description   Test case's description
 **/
GeometryShaderLayeredRenderingFBONoAttachment::GeometryShaderLayeredRenderingFBONoAttachment(
	Context& context, const ExtParameters& extParams, const char* name, const char* description)
	: TestCaseBase(context, extParams, name, description)
	, m_fbo_id(0)
	, m_fs_id(0)
	, m_gs_id(0)
	, m_po_id(0)
	, m_to_id(0)
	, m_vao_id(0)
	, m_vs_id(0)
	, m_all_layers_data(DE_NULL)
	, m_layer_data(DE_NULL)
{
	/* Nothing to be done here */
}

/** Deinitializes GLES objects created during the test.
 *
 */
void GeometryShaderLayeredRenderingFBONoAttachment::deinit(void)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Reset OpenGL ES state */
	gl.useProgram(0);
	gl.bindTexture(GL_TEXTURE_2D_ARRAY, 0 /* texture */);
	gl.bindFramebuffer(GL_FRAMEBUFFER, 0);
	gl.bindVertexArray(0);

	if (m_po_id != 0)
	{
		gl.deleteProgram(m_po_id);
		m_po_id = 0;
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

	if (m_vs_id != 0)
	{
		gl.deleteShader(m_vs_id);
		m_vs_id = 0;
	}

	if (m_to_id != 0)
	{
		gl.deleteTextures(1, &m_to_id);
		m_to_id = 0;
	}

	if (m_fbo_id != 0)
	{
		gl.deleteFramebuffers(1, &m_fbo_id);
		m_fbo_id = 0;
	}

	if (m_vao_id != 0)
	{
		gl.deleteVertexArrays(1, &m_vao_id);
		m_vao_id = 0;
	}

	if (m_all_layers_data != DE_NULL)
	{
		delete[] m_all_layers_data;
		m_all_layers_data = DE_NULL;
	}

	if (m_layer_data != DE_NULL)
	{
		delete[] m_layer_data;
		m_layer_data = DE_NULL;
	}

	/* Release base class */
	TestCaseBase::deinit();
}

/** Executes the test.
 *  Sets the test result to QP_TEST_RESULT_FAIL if the test failed, QP_TEST_RESULT_PASS otherwise.
 *
 *  Note the function throws exception should an error occur!
 *
 *  @return STOP if the test has finished, CONTINUE to indicate iterate should be called once again.
 *
 **/
tcu::TestNode::IterateResult GeometryShaderLayeredRenderingFBONoAttachment::iterate(void)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Check if required extensions are supported */
	if (!m_is_geometry_shader_extension_supported)
	{
		throw tcu::NotSupportedError(GEOMETRY_SHADER_EXTENSION_NOT_SUPPORTED, "", __FILE__, __LINE__);
	}

	if (!m_is_framebuffer_no_attachments_supported)
	{
		throw tcu::NotSupportedError("framebuffer_no_attachment, is not supported", "", __FILE__, __LINE__);
	}

	if (!m_is_shader_image_load_store_supported)
	{
		throw tcu::NotSupportedError("shader_image_load_store, is not supported", "", __FILE__, __LINE__);
	}

	/* Generate and bind a framebuffer object */
	gl.genFramebuffers(1, &m_fbo_id);
	gl.bindFramebuffer(GL_DRAW_FRAMEBUFFER, m_fbo_id);

	/* Get the default number of layers of a newly created framebuffer object */
	glw::GLint nLayers = -1;

	gl.getFramebufferParameteriv(GL_DRAW_FRAMEBUFFER, m_glExtTokens.FRAMEBUFFER_DEFAULT_LAYERS, &nLayers);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not query a framebuffer object for the number of default layers!");

	/* check 14.2 test condition - if default number of layer equals 0 */
	if (nLayers != 0)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Number of default layers should be equal to 0 but is "
						   << nLayers << tcu::TestLog::EndMessage;

		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
		return STOP;
	}

	/* Set the default resolution to 128x128 */
	glw::GLint width  = 0;
	glw::GLint height = 0;

	gl.framebufferParameteri(GL_DRAW_FRAMEBUFFER, GL_FRAMEBUFFER_DEFAULT_WIDTH, m_width);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not set the default framebuffer's width!");
	gl.getFramebufferParameteriv(GL_DRAW_FRAMEBUFFER, GL_FRAMEBUFFER_DEFAULT_WIDTH, &width);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not get the default framebuffer's width!");
	gl.framebufferParameteri(GL_DRAW_FRAMEBUFFER, GL_FRAMEBUFFER_DEFAULT_HEIGHT, m_height);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not set the default framebuffer's height!");
	gl.getFramebufferParameteriv(GL_DRAW_FRAMEBUFFER, GL_FRAMEBUFFER_DEFAULT_HEIGHT, &height);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not get the default framebuffer's height!");

	if (m_width != width || m_height != height)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Framebuffer's default width/height is not equal to" << m_width
						   << "\\" << m_height << " but is " << width << "\\" << height << tcu::TestLog::EndMessage;

		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
		return STOP;
	}

	/* Configure program object to be used for functional part of the test */
	m_po_id = gl.createProgram();

	m_vs_id = gl.createShader(GL_VERTEX_SHADER);
	m_gs_id = gl.createShader(m_glExtTokens.GEOMETRY_SHADER);
	m_fs_id = gl.createShader(GL_FRAGMENT_SHADER);

	if (!buildProgram(m_po_id, m_fs_id, 1 /* part */, &m_fs_code, m_gs_id, 1 /* part */, &m_gs_code, m_vs_id,
					  1 /* part */, &m_vs_code))
	{
		TCU_FAIL("Could not create program object from a valid vertex/geometry/fragment shader code!");
	}

	/* Configure texture objects and fill them with initial data */
	m_all_layers_data = new glw::GLint[m_n_layers * m_width * m_height * m_n_texture_components];

	for (int n = 0; n < m_n_layers * m_width * m_height; ++n)
	{
		m_all_layers_data[n * m_n_texture_components + 0] = 255;
		m_all_layers_data[n * m_n_texture_components + 1] = 0;
		m_all_layers_data[n * m_n_texture_components + 2] = 0;
		m_all_layers_data[n * m_n_texture_components + 3] = 0;
	}

	gl.genTextures(1, &m_to_id);
	gl.bindTexture(GL_TEXTURE_2D_ARRAY, m_to_id);
	gl.texStorage3D(GL_TEXTURE_2D_ARRAY, 1 /* levels */, GL_RGBA32I, m_width, m_height, m_n_layers);
	gl.texParameterf(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	gl.texParameterf(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Error creating texture objects!");

	/* Activate the test program object */
	gl.useProgram(m_po_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not use program!");

	/* Bind texture to image unit */
	gl.bindImageTexture(0, m_to_id, 0 /* level */, GL_TRUE, 0 /* layer */, GL_WRITE_ONLY, GL_RGBA32I);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not bind texture to image unit!");

	/* Generate & bind a VAO */
	gl.genVertexArrays(1, &m_vao_id);
	gl.bindVertexArray(m_vao_id);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not create a vertex array object!");

	/* Set up the viewport */
	gl.viewport(0 /* x */, 0 /* y */, m_width /* width */, m_height /* height */);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glViewport() call failed.");

	/* Test 21.4 adjusts the behavior of the original tests. The test now needs to run
	 * in two iterations:
	 *
	 * a) Original behavior is left intact
	 * b) GL_FRAMEBUFFER_DEFAULT_LAYERS_EXT should be set to 0.
	 */
	glw::GLboolean test_failed = false;

	m_layer_data = new glw::GLint[m_width * m_height * m_n_texture_components];

	for (unsigned int n_test_iteration = 0; n_test_iteration < 2 && !test_failed; ++n_test_iteration)
	{
		const glw::GLint current_n_layers = (n_test_iteration == 1) ? 0 : m_n_layers;

		/* Reset render-target contents */
		gl.texSubImage3D(GL_TEXTURE_2D_ARRAY, 0, /* level   */
						 0,						 /* xoffset */
						 0,						 /* yoffset */
						 0,						 /* zoffset */
						 m_width, m_height, m_n_layers, GL_RGBA_INTEGER, GL_INT, m_all_layers_data);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glTexSubImage3D() call failed.");

		/* Set the default number of layers to m_n_layers */
		gl.framebufferParameteri(GL_DRAW_FRAMEBUFFER, m_glExtTokens.FRAMEBUFFER_DEFAULT_LAYERS, current_n_layers);

		GLU_EXPECT_NO_ERROR(gl.getError(), "Could not set the default number of layers of a framebuffer object!");

		gl.getFramebufferParameteriv(GL_DRAW_FRAMEBUFFER, m_glExtTokens.FRAMEBUFFER_DEFAULT_LAYERS, &nLayers);

		/* check if the reported value equals m_n_layers */
		if (current_n_layers != nLayers)
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "FBO's default layers should be equal to " << m_n_layers
							   << "but is " << nLayers << tcu::TestLog::EndMessage;

			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
			return STOP;
		}

		/* Render! */
		gl.drawArrays(GL_POINTS, 0 /* first */, 1 /* count */);

		GLU_EXPECT_NO_ERROR(gl.getError(), "Could not execute a draw call!");

		/* Verify result texture data */
		gl.bindFramebuffer(GL_READ_FRAMEBUFFER, m_fbo_id);

		for (int n_layer = 0; n_layer < m_n_layers && !test_failed; ++n_layer)
		{
			gl.framebufferTextureLayer(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, m_to_id, 0 /* level */, n_layer);

			GLU_EXPECT_NO_ERROR(gl.getError(), "Could not bind texture as color attachment to framebuffer!");

			if (gl.checkFramebufferStatus(GL_READ_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
			{
				TCU_FAIL("Read framebuffer is not complete!");
			}

			gl.memoryBarrier(GL_FRAMEBUFFER_BARRIER_BIT);

			gl.readPixels(0 /* x */, 0 /* y */, m_width, m_height, GL_RGBA_INTEGER, GL_INT, m_layer_data);

			GLU_EXPECT_NO_ERROR(gl.getError(),
								"Could not read back pixels from the texture bound to color attachment!");

			/* Perform the verification */
			const int referenceColor[4] = { 0, 255, 0, 0 };

			for (int nPx = 0; nPx < m_width * m_height; ++nPx)
			{
				if (m_layer_data[nPx * m_n_texture_components + 0] != referenceColor[0] ||
					m_layer_data[nPx * m_n_texture_components + 1] != referenceColor[1] ||
					m_layer_data[nPx * m_n_texture_components + 2] != referenceColor[2] ||
					m_layer_data[nPx * m_n_texture_components + 3] != referenceColor[3])
				{
					m_testCtx.getLog() << tcu::TestLog::Message << "The test failed: Pixel " << nPx << " from layer "
									   << n_layer << " is set to [" << m_layer_data[nPx * m_n_texture_components + 0]
									   << "," << m_layer_data[nPx * m_n_texture_components + 1] << ","
									   << m_layer_data[nPx * m_n_texture_components + 2] << ","
									   << m_layer_data[nPx * m_n_texture_components + 3] << "] but should be equal to ["
									   << referenceColor[0] << "," << referenceColor[1] << "," << referenceColor[2]
									   << "," << referenceColor[3] << ","
									   << "]" << tcu::TestLog::EndMessage;

					test_failed = true;
					break;
				} /* if (result pixel is invalid) */
			}	 /* for (all pixels) */
		}		  /* for (all layers) */

		/* Restore the FBO to no-attachment state for the next iteration */
		gl.framebufferTextureLayer(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, 0, 0, 0);

	} /* for (both iterations) */

	if (test_failed)
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
	else
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

	return STOP;
}

} // namespace glcts
