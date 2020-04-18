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

#include "esextcGeometryShaderLayeredRenderingBoundaryCondition.hpp"

#include "gluDefs.hpp"
#include "glwEnums.hpp"
#include "glwFunctions.hpp"
#include "tcuTestLog.hpp"
#include <cstring>
#include <sstream>
#include <string>

namespace glcts
{
/* Configure constant values */
const glw::GLint GeometryShaderLayeredRenderingBoundaryCondition::m_width			   = 4;
const glw::GLint GeometryShaderLayeredRenderingBoundaryCondition::m_height			   = 4;
const glw::GLint GeometryShaderLayeredRenderingBoundaryCondition::m_max_depth		   = 4;
const glw::GLint GeometryShaderLayeredRenderingBoundaryCondition::m_texture_components = 4;

/** Constructor
 *
 * @param context       Test context
 * @param name          Test case's name
 * @param description   Test case's desricption
 **/
GeometryShaderLayeredRenderingBoundaryCondition::GeometryShaderLayeredRenderingBoundaryCondition(
	Context& context, const ExtParameters& extParams, const char* name, const char* description)
	: TestCaseBase(context, extParams, name, description)
	, m_draw_mode(GL_TEXTURE_3D)
	, m_n_points(0)
	, m_is_fbo_layered(false)
	, m_fbo_draw_id(0)
	, m_fbo_read_id(0)
	, m_fs_id(0)
	, m_gs_id(0)
	, m_po_id(0)
	, m_vao_id(0)
	, m_vs_id(0)
{
	unsigned char blue[]  = { 0, 0, 255, 255 };
	unsigned char green[] = { 0, 255, 0, 255 };
	unsigned char red[]   = { 255, 0, 0, 255 };
	unsigned char white[] = { 255, 255, 255, 255 };

	m_blue_color  = new unsigned char[m_texture_components];
	m_green_color = new unsigned char[m_texture_components];
	m_red_color   = new unsigned char[m_texture_components];
	m_white_color = new unsigned char[m_texture_components];

	memcpy(m_blue_color, blue, sizeof(blue));
	memcpy(m_green_color, green, sizeof(green));
	memcpy(m_red_color, red, sizeof(red));
	memcpy(m_white_color, white, sizeof(white));
}

GeometryShaderLayeredRenderingBoundaryCondition::~GeometryShaderLayeredRenderingBoundaryCondition(void)
{
	if (m_blue_color)
	{
		delete[] m_blue_color;
		m_blue_color = 0;
	}

	if (m_green_color)
	{
		delete[] m_green_color;
		m_green_color = 0;
	}

	if (m_red_color)
	{
		delete[] m_red_color;
		m_red_color = 0;
	}
	if (m_white_color)
	{
		delete[] m_white_color;
		m_white_color = 0;
	}
}

/** Check if given data contains the same values as reference pixel
 *
 *   @param width          Texture width
 *   @param height         Texture height
 *   @param pixelSize      Size of pixel
 *   @param textureData    buffer with data read from texture
 *   @param referencePixel contains expected color value
 *   @param attachment     Attachment number (written to log on failure)
 *   @param layer          Layer number (written to log on failure)
 *
 *   @return  true    If all data in scope from textureData contains the same color as in referencePixel
 *            false   in other case
 **/
bool GeometryShaderLayeredRenderingBoundaryCondition::comparePixels(glw::GLint width, glw::GLint height,
																	glw::GLint			 pixelSize,
																	const unsigned char* textureData,
																	const unsigned char* referencePixel, int attachment,
																	int layer)
{
	unsigned int rowWidth = pixelSize * width;

	for (int y = 0; y < height; ++y)
	{
		for (int x = 0; x < width; ++x)
		{
			const unsigned char* renderedData = textureData + y * rowWidth + x * m_texture_components;

			if (memcmp(referencePixel, renderedData, m_texture_components) != 0)
			{
				m_testCtx.getLog() << tcu::TestLog::Message << "Rendered data for [x=" << x << " y=" << y
								   << " attachment=" << attachment << " layer=" << layer << "] "
								   << "[" << (int)renderedData[0] << ", " << (int)renderedData[1] << ", "
								   << (int)renderedData[2] << ", " << (int)renderedData[3] << "]"
								   << " are different from reference data [" << (int)referencePixel[0] << ", "
								   << (int)referencePixel[1] << ", " << (int)referencePixel[2] << ", "
								   << (int)referencePixel[3] << "] !" << tcu::TestLog::EndMessage;
				return false;
			} /* if (data comparison failed) */
		}	 /* for (all columns) */
	}		  /* for (all rows) */
	return true;
}

/** Deinitializes GLES objects created during the test.
 *
 */
void GeometryShaderLayeredRenderingBoundaryCondition::deinit(void)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Reset OpenGL ES state */
	gl.useProgram(0);
	gl.bindVertexArray(0);
	gl.bindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	gl.bindFramebuffer(GL_READ_FRAMEBUFFER, 0);

	for (unsigned int i = 0; i < m_textures_info.size(); i++)
	{
		gl.bindTexture(m_textures_info[i].m_texture_target, 0);
	}

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

	for (unsigned int i = 0; i < m_textures_info.size(); i++)
	{
		gl.deleteTextures(1, &m_textures_info[i].m_id);
	}

	if (m_fbo_read_id != 0)
	{
		gl.deleteFramebuffers(1, &m_fbo_read_id);
	}

	if (m_fbo_draw_id != 0)
	{
		gl.deleteFramebuffers(1, &m_fbo_draw_id);
	}

	if (m_vao_id != 0)
	{
		gl.deleteVertexArrays(1, &m_vao_id);
	}

	/* Release base class */
	TestCaseBase::deinit();
}

/** Returns code for Geometry Shader.
 *
 *  @return NULL
 **/
const char* GeometryShaderLayeredRenderingBoundaryCondition::getGeometryShaderCode()
{
	return 0;
}

/** Returns code for Fragment Shader
 * @return pointer to literal with Fragment Shader code
 **/
const char* GeometryShaderLayeredRenderingBoundaryCondition::getFragmentShaderCode()
{
	static const char* result = "${VERSION}\n"
								"\n"
								"precision highp float;\n"
								"\n"
								"flat in  int  layer_id;\n"
								"     out vec4 color;\n"
								"\n"
								"void main()\n"
								"{\n"
								"    color = vec4(1, 1, 1, 1);\n"
								"}\n";
	return result;
}

/** Returns code for Vertex Shader
 *
 * @return pointer to literal with Vertex Shader code
 **/
const char* GeometryShaderLayeredRenderingBoundaryCondition::getVertexShaderCode()
{
	static const char* result = "${VERSION}\n"
								"\n"
								"precision highp float;\n"
								"\n"
								"flat out int layer_id;\n"
								"\n"
								"void main()\n"
								"{\n"
								"    layer_id = 0;\n"
								"}\n";

	return result;
}

/** Initializes GLES objects used during the test.
 *
 **/
void GeometryShaderLayeredRenderingBoundaryCondition::initTest(void)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Create shader and program objects */
	const char* fsCode = getFragmentShaderCode();
	const char* gsCode = getGeometryShaderCode();
	const char* vsCode = getVertexShaderCode();

	m_fs_id = gl.createShader(GL_FRAGMENT_SHADER);
	m_vs_id = gl.createShader(GL_VERTEX_SHADER);
	m_gs_id = gl.createShader(m_glExtTokens.GEOMETRY_SHADER);
	m_po_id = gl.createProgram();

	GLU_EXPECT_NO_ERROR(gl.getError(), "Error creating program/shader objects.");

	if (!buildProgram(m_po_id, (fsCode) ? m_fs_id : 0, (fsCode) ? 1 : 0 /* part */, (fsCode) ? &fsCode : 0,
					  (gsCode) ? m_gs_id : 0, (gsCode) ? 1 : 0 /* part */, (gsCode) ? &gsCode : 0,
					  (vsCode) ? m_vs_id : 0, (vsCode) ? 1 : 0 /* part */, (vsCode) ? &vsCode : 0))
	{
		TCU_FAIL("Could not create a program object from a valid vertex/geometry/fragment shader!");
	}

	/* Set up framebuffer objects */
	gl.genFramebuffers(1, &m_fbo_read_id);
	gl.genFramebuffers(1, &m_fbo_draw_id);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Error creating framebuffer objects!");

	/* Set up vertex array object */
	gl.genVertexArrays(1, &m_vao_id);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Error creating vertex array object!");

	for (unsigned int i = 0; i < m_textures_info.size(); i++)
	{
		gl.genTextures(1, &m_textures_info[i].m_id);
	}

	GLU_EXPECT_NO_ERROR(gl.getError(), "Error creating texture objects!");
}

/** Executes the test.
 *  Sets the test result to QP_TEST_RESULT_FAIL if the test failed, QP_TEST_RESULT_PASS otherwise.
 *  @return STOP if the test has finished, CONTINUE to indicate iterate should be called once again.
 *  Note the function throws exception should an error occur!
 **/
tcu::TestNode::IterateResult GeometryShaderLayeredRenderingBoundaryCondition::iterate(void)
{
	/* check if EXT_geometry_shader extension is supported */
	if (!m_is_geometry_shader_extension_supported)
	{
		throw tcu::NotSupportedError(GEOMETRY_SHADER_EXTENSION_NOT_SUPPORTED, "", __FILE__, __LINE__);
	}

	initTest();

	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Bind draw framebuffer */
	gl.bindFramebuffer(GL_DRAW_FRAMEBUFFER, m_fbo_draw_id);

	/* Set up all textures */
	unsigned char buffer[m_width * m_height * m_max_depth * m_texture_components];

	memset(buffer, 0, sizeof(buffer));

	for (unsigned int i = 0; i < m_textures_info.size(); i++)
	{
		gl.bindTexture(m_textures_info[i].m_texture_target, m_textures_info[i].m_id);
		gl.texStorage3D(m_textures_info[i].m_texture_target, 1, GL_RGBA8, m_width, m_height,
						m_textures_info[i].m_depth);
		gl.texSubImage3D(m_textures_info[i].m_texture_target, 0, 0, 0, 0, m_width, m_height, m_textures_info[i].m_depth,
						 GL_RGBA, GL_UNSIGNED_BYTE, buffer);

		GLU_EXPECT_NO_ERROR(gl.getError(), "Error configuring a texture object!");
	}

	/* Set up draw buffers */
	{
		glw::GLenum* drawBuffers = new glw::GLenum[m_textures_info.size()];

		for (unsigned int i = 0; i < m_textures_info.size(); i++)
		{
			drawBuffers[i] = m_textures_info[i].m_draw_buffer;
		}

		gl.drawBuffers((glw::GLsizei)m_textures_info.size(), drawBuffers);

		delete[] drawBuffers;
		drawBuffers = 0;
	}

	GLU_EXPECT_NO_ERROR(gl.getError(), "Error setting draw buffers!");

	/* Configure draw FBO so that it uses texture attachments */
	for (unsigned int i = 0; i < m_textures_info.size(); i++)
	{
		if (m_is_fbo_layered)
		{
			gl.framebufferTexture(GL_DRAW_FRAMEBUFFER, m_textures_info[i].m_draw_buffer, m_textures_info[i].m_id,
								  0 /* level */);
		}
		else
		{
			gl.framebufferTextureLayer(GL_DRAW_FRAMEBUFFER, m_textures_info[i].m_draw_buffer, m_textures_info[i].m_id,
									   0 /* level */, i /* layer */);
		}

		GLU_EXPECT_NO_ERROR(gl.getError(), "Error configuring framebuffer objects!");
	} /* for (all textures considered) */

	/* Verify draw framebuffer is considered complete */
	glw::GLenum fboCompleteness = gl.checkFramebufferStatus(GL_DRAW_FRAMEBUFFER);

	if (fboCompleteness != GL_FRAMEBUFFER_COMPLETE)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Draw FBO is incomplete: "
						   << "[" << fboCompleteness << "]" << tcu::TestLog::EndMessage;

		TCU_FAIL("Draw FBO is incomplete.");
	}

	/* Set up viewport */
	gl.viewport(0, 0, m_width, m_height);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Error setting up viewport!");

	/** Bind a vertex array object */
	gl.bindVertexArray(m_vao_id);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Error configuring vertex array object!");

	/* Render */
	gl.useProgram(m_po_id);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Error using program object!");

	gl.drawArrays(m_draw_mode, 0, m_n_points);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Rendering failed!");

	/* Bind read framebuffer object. */
	gl.bindFramebuffer(GL_READ_FRAMEBUFFER, m_fbo_read_id);

	/* Compare the rendered data against reference representation */
	unsigned int min_depth = 0;

	if (m_textures_info.size() > 0)
	{
		min_depth = m_textures_info[0].m_depth;

		for (unsigned int nTexture = 1; nTexture < m_textures_info.size(); nTexture++)
		{
			if (min_depth > (unsigned)m_textures_info[nTexture].m_depth)
			{
				min_depth = m_textures_info[nTexture].m_depth;
			}
		}
	}

	for (unsigned int nTexture = 0; nTexture < m_textures_info.size(); nTexture++)
	{
		for (unsigned int nLayer = 0; nLayer < min_depth; nLayer++)
		{
			/* Configure read FBO's color attachment */
			gl.framebufferTextureLayer(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, m_textures_info[nTexture].m_id, 0,
									   nLayer);

			GLU_EXPECT_NO_ERROR(gl.getError(), "Could not set up read framebuffer!");

			/* Verify read framebuffer is considered complete */
			glw::GLenum _fboCompleteness = gl.checkFramebufferStatus(GL_DRAW_FRAMEBUFFER);

			if (_fboCompleteness != GL_FRAMEBUFFER_COMPLETE)
			{
				m_testCtx.getLog() << tcu::TestLog::Message << "Read FBO is incomplete: "
								   << "[" << _fboCompleteness << "]" << tcu::TestLog::EndMessage;

				TCU_FAIL("Read FBO is incomplete.");
			}
			gl.viewport(0, 0, m_width, m_height);

			/* Read the rendered data */
			gl.readPixels(0 /* x */, 0 /* y */, m_width /* width */, m_height /* height */, GL_RGBA, GL_UNSIGNED_BYTE,
						  buffer);

			GLU_EXPECT_NO_ERROR(gl.getError(), "Could not read pixels using glReadPixels()");

			/* Retrieve reference color for layer */
			unsigned char expectedData[m_texture_components];

			getReferenceColor(nLayer, expectedData, m_texture_components);

			/* Compare the retrieved data with reference data */
			if (!comparePixels(m_width, m_height, m_texture_components, buffer, expectedData, nTexture, nLayer))
			{
				m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
				return STOP;
			} /* if (data comparison failed) */
		}	 /* for (all layers) */
	}		  /* for (all texture objects) */

	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	return STOP;
}

/** Constructor
 *
 * @param context       Test context
 * @param name          Test case's name
 * @param description   Test case's desricption
 **/
GeometryShaderLayeredRenderingBoundaryConditionVariousTextures::
	GeometryShaderLayeredRenderingBoundaryConditionVariousTextures(Context& context, const ExtParameters& extParams,
																   const char* name, const char* description)
	: GeometryShaderLayeredRenderingBoundaryCondition(context, extParams, name, description)
{
	TextureInfo texInfo;

	texInfo.m_depth			 = 2;
	texInfo.m_draw_buffer	= GL_COLOR_ATTACHMENT0;
	texInfo.m_id			 = 0;
	texInfo.m_texture_target = GL_TEXTURE_3D;

	m_textures_info.push_back(texInfo);

	texInfo.m_depth			 = 4;
	texInfo.m_draw_buffer	= GL_COLOR_ATTACHMENT1;
	texInfo.m_id			 = 0;
	texInfo.m_texture_target = GL_TEXTURE_3D;

	m_textures_info.push_back(texInfo);

	m_draw_mode		 = GL_POINTS;
	m_n_points		 = 1;
	m_is_fbo_layered = true;
}

/** Returns code for Fragment Shader
 *
 * @return pointer to literal with Fragment Shader code
 **/
const char* GeometryShaderLayeredRenderingBoundaryConditionVariousTextures::getFragmentShaderCode()
{
	static const char* result = "${VERSION}\n"
								"\n"
								"precision highp float;\n"
								"\n"
								"flat in  int  layer_id;\n"
								"layout(location=0) out vec4 color0;\n"
								"layout(location=1) out vec4 color1;\n"
								"\n"
								"void main()\n"
								"{\n"
								"    vec4 color;\n"
								"    switch (layer_id)\n"
								"    {\n"
								"        case 0:  color = vec4(1, 0, 0, 1); break;\n"
								"        case 1:  color = vec4(0, 1, 0, 1); break;\n"
								"        case 2:  color = vec4(0, 0, 1, 1); break;\n"
								"        case 3:  color = vec4(1, 1, 1, 1); break;\n"
								"        default: color = vec4(0, 0, 0, 0); break;\n"
								"    }\n"
								"    color0 = color;\n"
								"    color1 = color;\n"
								"}\n";
	return result;
}

/** Returns code for Geometry Shader
 *
 * @return pointer to literal with Geometry Shader code
 **/
const char* GeometryShaderLayeredRenderingBoundaryConditionVariousTextures::getGeometryShaderCode()
{
	static const char* result = "${VERSION}\n"
								"\n"
								"${GEOMETRY_SHADER_REQUIRE}\n"
								"\n"
								"precision highp float;\n"
								"\n"
								"#define MAX_VERTICES 16\n"
								"#define N_LAYERS     2\n"
								"\n"
								"layout(points)                                    in;\n"
								"layout(triangle_strip, max_vertices=MAX_VERTICES) out;\n"
								"\n"
								"precision highp float;\n"
								"\n"
								"flat out int layer_id;\n"
								"\n"
								"void main()\n"
								"{\n"
								"    for (int n = 0;n < N_LAYERS;++n)\n"
								"    {\n"
								"        gl_Layer    = n;\n"
								"        layer_id    = gl_Layer;\n"
								"        gl_Position = vec4(1, 1, 0, 1);\n"
								"        EmitVertex();\n"
								"\n"
								"        gl_Layer    = n;\n"
								"        layer_id    = gl_Layer;\n"
								"        gl_Position = vec4(1, -1, 0, 1);\n"
								"        EmitVertex();\n"
								"\n"
								"        gl_Layer    = n;\n"
								"        layer_id    = gl_Layer;\n"
								"        gl_Position = vec4(-1, 1, 0, 1);\n"
								"        EmitVertex();\n"
								"\n"
								"        gl_Layer    = n;\n"
								"        layer_id    = gl_Layer;\n"
								"        gl_Position = vec4(-1, -1, 0, 1);\n"
								"        EmitVertex();\n"
								"\n"
								"        EndPrimitive();\n"
								"    }\n"
								"}\n";
	return result;
}

/** Get reference color for test result verification
 * @param layerIndex      index of layer
 * @param colorBuffer     will be used to store the requested data(buffor size should be greater than or equal colorBufferSize)
 * @param colorBufferSize components number
 **/
void GeometryShaderLayeredRenderingBoundaryConditionVariousTextures::getReferenceColor(glw::GLint	 layerIndex,
																					   unsigned char* colorBuffer,
																					   int			  colorBufferSize)
{
	if (layerIndex == 0)
	{
		memcpy(colorBuffer, m_red_color, colorBufferSize);
	}
	else if (layerIndex == 1)
	{
		memcpy(colorBuffer, m_green_color, colorBufferSize);
	}
	else
	{
		memset(colorBuffer, 0, colorBufferSize);
	}
}

/** Constructor
 *
 * @param context     Test context
 * @param name        Test case's name
 * @param description Test case's description
 **/
GeometryShaderLayeredRenderingBoundaryConditionNoGS::GeometryShaderLayeredRenderingBoundaryConditionNoGS(
	Context& context, const ExtParameters& extParams, const char* name, const char* description)
	: GeometryShaderLayeredRenderingBoundaryCondition(context, extParams, name, description)
{
	TextureInfo texInfo;

	texInfo.m_depth			 = 4;
	texInfo.m_draw_buffer	= GL_COLOR_ATTACHMENT0;
	texInfo.m_id			 = 0;
	texInfo.m_texture_target = GL_TEXTURE_3D;

	m_textures_info.push_back(texInfo);

	m_draw_mode		 = GL_TRIANGLE_FAN;
	m_n_points		 = 4;
	m_is_fbo_layered = true;
}

/** Get reference color for test result verification
 * @param layerIndex      index of layer
 * @param colorBuffer     will be used to store the requested data(buffer size should be greater than or equal colorBufferSize)
 * @param colorBufferSize components number
 **/
void GeometryShaderLayeredRenderingBoundaryConditionNoGS::getReferenceColor(glw::GLint	 layerIndex,
																			unsigned char* colorBuffer,
																			int			   colorBufferSize)
{
	if (layerIndex == 0)
	{
		memcpy(colorBuffer, m_white_color, colorBufferSize);
	}
	else
	{
		memset(colorBuffer, 0, colorBufferSize);
	}
}

/** Returns code for Vertex Shader
 * @return pointer to literal with Vertex Shader code
 **/
const char* GeometryShaderLayeredRenderingBoundaryConditionNoGS::getVertexShaderCode()
{
	static const char* result = "${VERSION}\n"
								"\n"
								"precision highp float;\n"
								"\n"
								"flat out int layer_id;\n"
								"\n"
								"void main()\n"
								"{\n"
								"    layer_id = 0;\n"
								"\n"
								"    switch (gl_VertexID)\n"
								"    {\n"
								"        case 0:  gl_Position = vec4(-1, -1, 0, 1); break;\n"
								"        case 1:  gl_Position = vec4(-1,  1, 0, 1); break;\n"
								"        case 2:  gl_Position = vec4( 1,  1, 0, 1); break;\n"
								"        default: gl_Position = vec4( 1, -1, 0, 1); break;\n"
								"    }\n"
								"}\n";
	return result;
}

/** Constructor
 *
 * @param context       Test context
 * @param name          Test case's name
 * @param description   Test case's desricption
 **/
GeometryShaderLayeredRenderingBoundaryConditionNoLayerSet::GeometryShaderLayeredRenderingBoundaryConditionNoLayerSet(
	Context& context, const ExtParameters& extParams, const char* name, const char* description)
	: GeometryShaderLayeredRenderingBoundaryCondition(context, extParams, name, description)
{
	TextureInfo texInfo;

	texInfo.m_depth			 = 4;
	texInfo.m_draw_buffer	= GL_COLOR_ATTACHMENT0;
	texInfo.m_id			 = 0;
	texInfo.m_texture_target = GL_TEXTURE_3D;

	m_textures_info.push_back(texInfo);

	m_draw_mode		 = GL_POINTS;
	m_n_points		 = 1;
	m_is_fbo_layered = true;
}

/** Returns code for Geometry Shader
 * @return pointer to literal with Geometry Shader code
 **/
const char* GeometryShaderLayeredRenderingBoundaryConditionNoLayerSet::getGeometryShaderCode()
{
	static const char* result = "${VERSION}\n"
								"\n"
								"${GEOMETRY_SHADER_REQUIRE}\n"
								"\n"
								"precision highp float;\n"
								"\n"
								"#define MAX_VERTICES 4\n"
								"\n"
								"layout(points)                                    in;\n"
								"layout(triangle_strip, max_vertices=MAX_VERTICES) out;\n"
								"\n"
								"precision highp float;\n"
								"\n"
								"flat out int layer_id;\n"
								"\n"
								"void main()\n"
								"{\n"
								"    layer_id    = 0;\n"
								"    gl_Position = vec4(1, 1, 0, 1);\n"
								"    EmitVertex();\n"
								"\n"
								"    layer_id    = 0;\n"
								"    gl_Position = vec4(1, -1, 0, 1);\n"
								"    EmitVertex();\n"
								"\n"
								"    layer_id    = 0;\n"
								"    gl_Position = vec4(-1, 1, 0, 1);\n"
								"    EmitVertex();\n"
								"\n"
								"    layer_id    = 0;\n"
								"    gl_Position = vec4(-1, -1, 0, 1);\n"
								"    EmitVertex();\n"
								"\n"
								"    EndPrimitive();\n"
								"}\n";
	return result;
}

/** Get reference color for test result verification
 * @param layerIndex      index of layer
 * @param colorBuffer     will be used to store the requested data(buffer size should be greater than or equal colorBufferSize)
 * @param colorBufferSize components number
 **/
void GeometryShaderLayeredRenderingBoundaryConditionNoLayerSet::getReferenceColor(glw::GLint	 layerIndex,
																				  unsigned char* colorBuffer,
																				  int			 colorBufferSize)
{
	if (layerIndex == 0)
	{
		memcpy(colorBuffer, m_white_color, colorBufferSize);
	}
	else
	{
		memset(colorBuffer, 0, colorBufferSize);
	}
}

/** Constructor
 *
 * @param context       Test context
 * @param name          Test case's name
 * @param description   Test case's desricption
 **/
GeometryShaderLayeredRenderingBoundaryConditionNoLayeredFBO::
	GeometryShaderLayeredRenderingBoundaryConditionNoLayeredFBO(Context& context, const ExtParameters& extParams,
																const char* name, const char* description)
	: GeometryShaderLayeredRenderingBoundaryCondition(context, extParams, name, description)
{
	TextureInfo texInfo;

	texInfo.m_depth			 = 4;
	texInfo.m_draw_buffer	= GL_COLOR_ATTACHMENT0;
	texInfo.m_id			 = 0;
	texInfo.m_texture_target = GL_TEXTURE_3D;

	m_textures_info.push_back(texInfo);

	m_draw_mode		 = GL_POINTS;
	m_n_points		 = 1;
	m_is_fbo_layered = false;
}

/** Returns code for Geometry Shader
 * @return pointer to literal with Geometry Shader code
 **/
const char* GeometryShaderLayeredRenderingBoundaryConditionNoLayeredFBO::getGeometryShaderCode()
{
	static const char* result = "${VERSION}\n"
								"\n"
								"${GEOMETRY_SHADER_REQUIRE}\n"
								"\n"
								"precision highp float;\n"
								"\n"
								"#define MAX_VERTICES 4\n"
								"\n"
								"layout(points)                                    in;\n"
								"layout(triangle_strip, max_vertices=MAX_VERTICES) out;\n"
								"\n"
								"precision highp float;\n"
								"\n"
								"flat out int layer_id;\n"
								"\n"
								"void main()\n"
								"{\n"
								"    gl_Layer = 1;\n"
								"\n"
								"    layer_id    = gl_Layer;\n"
								"    gl_Position = vec4(1, 1, 0, 1);\n"
								"    EmitVertex();\n"
								"\n"
								"    layer_id    = gl_Layer;\n"
								"    gl_Position = vec4(1, -1, 0, 1);\n"
								"    EmitVertex();\n"
								"\n"
								"    layer_id    = gl_Layer;\n"
								"    gl_Position = vec4(-1, 1, 0, 1);\n"
								"    EmitVertex();\n"
								"\n"
								"    layer_id    = gl_Layer;\n"
								"    gl_Position = vec4(-1, -1, 0, 1);\n"
								"    EmitVertex();\n"
								"\n"
								"    EndPrimitive();\n"
								"}\n";
	return result;
}

/** Get reference color for test result verification
 * @param layerIndex      index of layer
 * @param colorBuffer     will be used to store the requested data(buffer size should be greater than or equal colorBufferSize)
 * @param colorBufferSize components number
 **/
void GeometryShaderLayeredRenderingBoundaryConditionNoLayeredFBO::getReferenceColor(glw::GLint	 layerIndex,
																					unsigned char* colorBuffer,
																					int			   colorBufferSize)
{
	if (layerIndex == 0)
	{
		memcpy(colorBuffer, m_white_color, colorBufferSize);
	}
	else
	{
		memset(colorBuffer, 0, colorBufferSize);
	}
}

} // namespace glcts
