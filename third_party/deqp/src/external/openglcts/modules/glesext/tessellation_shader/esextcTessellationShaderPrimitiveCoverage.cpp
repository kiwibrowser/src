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

/*!
 * \file esextcTessellationShaderPrimitiveCoverage.cpp
 * \brief TessellationShadePrimitiveCoverage (Test 31)
 */ /*-------------------------------------------------------------------*/

#include "esextcTessellationShaderPrimitiveCoverage.hpp"
#include "esextcTessellationShaderUtils.hpp"
#include "gluContextInfo.hpp"
#include "gluDefs.hpp"
#include "glwEnums.hpp"
#include "glwFunctions.hpp"
#include "tcuTestLog.hpp"
#include <cstdarg>
#include <cstddef>
#include <cstdlib>

namespace glcts
{

/* Vertex shader */
const char* TessellationShaderPrimitiveCoverage::m_vs_code = "${VERSION}\n"
															 "\n"
															 "precision highp float;\n"
															 "\n"
															 "layout(location = 0) in vec4 position;\n"
															 "\n"
															 "void main()\n"
															 "{\n"
															 "    gl_Position = position;\n"
															 "}\n";

/* Tessellation Control Shaders' source code  */
const char* TessellationShaderPrimitiveCoverage::m_quad_tessellation_tcs_code =
	"${VERSION}\n"
	"\n"
	"${TESSELLATION_SHADER_REQUIRE}\n"
	"\n"
	"precision highp float;\n"
	"\n"
	"layout(vertices = 4) out;\n"
	"\n"
	"uniform vec2 innerLevel;"
	"uniform vec4 outerLevel;"
	"\n"
	"void main()\n"
	"{\n"
	"    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;\n"
	"\n"
	"    gl_TessLevelInner[0] = innerLevel.x;\n"
	"    gl_TessLevelInner[1] = innerLevel.y;\n"
	"    gl_TessLevelOuter[0] = outerLevel.x;\n"
	"    gl_TessLevelOuter[1] = outerLevel.y;\n"
	"    gl_TessLevelOuter[2] = outerLevel.z;\n"
	"    gl_TessLevelOuter[3] = outerLevel.w;\n"
	"}\n";

/* Tessellation Evaluation Shaders' source code  */
const char* TessellationShaderPrimitiveCoverage::m_quad_tessellation_tes_code =
	"${VERSION}\n"
	"\n"
	"${TESSELLATION_SHADER_REQUIRE}\n"
	"\n"
	"precision highp float;\n"
	"\n"
	"layout (quads) in;\n"
	"\n"
	"void main()\n"
	"{\n"
	"    gl_Position = gl_in[0].gl_Position * (1.0 - gl_TessCoord.x) * (1.0 - "
	"gl_TessCoord.y)\n" /* Specifying the vertex’s position */
	"                + gl_in[1].gl_Position * (      gl_TessCoord.x) * (1.0 - "
	"gl_TessCoord.y)\n" /* using the bilinear interpolation */
	"                + gl_in[2].gl_Position * (      gl_TessCoord.x) * (      "
	"gl_TessCoord.y)\n"
	"                + gl_in[3].gl_Position * (1.0 - gl_TessCoord.x) * (      "
	"gl_TessCoord.y);\n"
	"}\n";

/* Tessellation Control Shaders' source code  */
const char* TessellationShaderPrimitiveCoverage::m_triangles_tessellation_tcs_code =
	"${VERSION}\n"
	"\n"
	"${TESSELLATION_SHADER_REQUIRE}\n"
	"\n"
	"precision highp float;\n"
	"\n"
	"layout(vertices = 3) out;\n"
	"\n"
	"uniform vec2 innerLevel;"
	"uniform vec4 outerLevel;"
	"\n"
	"void main()\n"
	"{\n"
	"    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;\n"
	"\n"
	"    gl_TessLevelInner[0] = innerLevel.x;\n"
	"    gl_TessLevelInner[1] = innerLevel.y;\n"
	"    gl_TessLevelOuter[0] = outerLevel.x;\n"
	"    gl_TessLevelOuter[1] = outerLevel.y;\n"
	"    gl_TessLevelOuter[2] = outerLevel.z;\n"
	"    gl_TessLevelOuter[3] = outerLevel.w;\n"
	"}\n";

/* Tessellation Evaluation Shader code for triangle test */
const char* TessellationShaderPrimitiveCoverage::m_triangles_tessellation_tes_code =
	"${VERSION}\n"
	"\n"
	"${TESSELLATION_SHADER_REQUIRE}\n"
	"\n"
	"precision highp float;\n"
	"\n"
	"layout (triangles) in;\n"
	"\n"
	"void main()\n"
	"{\n"
	"    gl_Position = gl_in[0].gl_Position * gl_TessCoord.x\n" /* Specifying the vertex’s position */
	"                + gl_in[1].gl_Position * gl_TessCoord.y\n" /* using the barycentric interpolation */
	"                + gl_in[2].gl_Position * gl_TessCoord.z;\n"
	"}\n";

/* Fragment Shader code */
const char* TessellationShaderPrimitiveCoverage::m_fs_code = "${VERSION}\n"
															 "\n"
															 "precision highp float;\n"
															 "\n"
															 "uniform highp vec4 stencil_fail_color;\n"
															 "\n"
															 "layout(location = 0) out highp vec4 color;\n"
															 "\n"
															 "void main()\n"
															 "{\n"
															 "    color = stencil_fail_color;\n"
															 "}\n";

/* A clear color used to set up framebuffer */
const glw::GLfloat TessellationShaderPrimitiveCoverage::m_clear_color[4] = {
	255.f / 255.f, /* red   */
	128.f / 255.f, /* green */
	64.f / 255.f,  /* blue  */
	32.f / 255.f   /* alpha */
};

/* A color used to draw differences between the tesselated primitive
 * and the reference primitive (when stencil test passes)
 */
const glw::GLfloat TessellationShaderPrimitiveCoverage::m_stencil_pass_color[4] = {
	32.f / 255.f,  /* red   */
	64.f / 255.f,  /* green */
	128.f / 255.f, /* blue  */
	255.f / 255.f  /* alpha */
};

/* Rendering area height */
const glw::GLuint TessellationShaderPrimitiveCoverage::m_height =
	2048; /* minimum maximum as required by ES specification */
/* Number of components as used for color attachment */
const glw::GLuint TessellationShaderPrimitiveCoverage::m_n_components = 4;
/* Rendering area width */
const glw::GLuint TessellationShaderPrimitiveCoverage::m_width =
	2048; /* minimum maximum as required by ES specification */

/* Buffer size for fetched pixels */
const glw::GLuint TessellationShaderPrimitiveCoverage::m_rendered_data_buffer_size = m_width	/* width */
																					 * m_height /* height */
																					 * m_n_components /* components */;

/** Constructor
 *
 * @param context     Test context
 * @param name        Test case's name
 * @param description Test case's description
 **/
TessellationShaderPrimitiveCoverage::TessellationShaderPrimitiveCoverage(Context&			  context,
																		 const ExtParameters& extParams)
	: TestCaseBase(context, extParams, "primitive_coverage",
				   "Verifies that no fragments are generated more than once when the "
				   "rendering pipeline (consisting of TC+TE stages) generates a "
				   "tessellated full-screen quad or two tessellated triangles.")
	, m_vao_id(0)
	, m_quad_tessellation_po_id(0)
	, m_stencil_verification_po_id(0)
	, m_triangles_tessellation_po_id(0)
	, m_bo_id(0)
	, m_fs_id(0)
	, m_quad_tessellation_tcs_id(0)
	, m_quad_tessellation_tes_id(0)
	, m_triangles_tessellation_tcs_id(0)
	, m_triangles_tessellation_tes_id(0)
	, m_vs_id(0)
	, m_fbo_id(0)
	, m_color_rbo_id(0)
	, m_stencil_rbo_id(0)
	, m_rendered_data_buffer(DE_NULL)
{
	/* Nothing to be done here */
}

/** Deinitializes all ES objects created for the test. */
void TessellationShaderPrimitiveCoverage::deinit(void)
{
	/* Deinitialize base class */
	TestCaseBase::deinit();

	if (!m_is_tessellation_shader_supported)
	{
		return;
	}

	/* Retrieve ES entry-points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Reset OpenGL ES state */
	gl.useProgram(0);
	gl.bindBuffer(GL_ARRAY_BUFFER, 0);
	gl.bindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	gl.bindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	gl.disable(GL_STENCIL_TEST);
	gl.bindVertexArray(0);

	/* Reset GL_PATCH_VERTICES_EXT to the default value. */
	gl.patchParameteri(m_glExtTokens.PATCH_VERTICES, 3);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glPatchParameteriEXT() failed!");

	/* Delete buffers */
	if (m_rendered_data_buffer != DE_NULL)
	{
		free(m_rendered_data_buffer);

		m_rendered_data_buffer = DE_NULL;
	}

	/* Delete program and shader objects */
	if (m_quad_tessellation_po_id != 0)
	{
		gl.deleteProgram(m_quad_tessellation_po_id);

		m_quad_tessellation_po_id = 0;
	}

	if (m_stencil_verification_po_id != 0)
	{
		gl.deleteProgram(m_stencil_verification_po_id);

		m_stencil_verification_po_id = 0;
	}

	if (m_triangles_tessellation_po_id != 0)
	{
		gl.deleteProgram(m_triangles_tessellation_po_id);

		m_triangles_tessellation_po_id = 0;
	}

	if (m_fs_id != 0)
	{
		gl.deleteShader(m_fs_id);

		m_fs_id = 0;
	}

	if (m_quad_tessellation_tcs_id != 0)
	{
		gl.deleteShader(m_quad_tessellation_tcs_id);

		m_quad_tessellation_tcs_id = 0;
	}

	if (m_quad_tessellation_tes_id != 0)
	{
		gl.deleteShader(m_quad_tessellation_tes_id);

		m_quad_tessellation_tes_id = 0;
	}

	if (m_triangles_tessellation_tcs_id != 0)
	{
		gl.deleteShader(m_triangles_tessellation_tcs_id);

		m_triangles_tessellation_tcs_id = 0;
	}

	if (m_triangles_tessellation_tes_id != 0)
	{
		gl.deleteShader(m_triangles_tessellation_tes_id);

		m_triangles_tessellation_tes_id = 0;
	}

	if (m_vs_id != 0)
	{
		gl.deleteShader(m_vs_id);

		m_vs_id = 0;
	}

	/* Delete framebuffer and renderbuffer objects */
	if (m_fbo_id != 0)
	{
		gl.deleteFramebuffers(1, &m_fbo_id);

		m_fbo_id = 0;
	}

	if (m_color_rbo_id != 0)
	{
		gl.deleteRenderbuffers(1, &m_color_rbo_id);

		m_color_rbo_id = 0;
	}

	if (m_stencil_rbo_id != 0)
	{
		gl.deleteRenderbuffers(1, &m_stencil_rbo_id);

		m_stencil_rbo_id = 0;
	}

	/* Delete buffer objects */
	if (m_bo_id != 0)
	{
		gl.deleteBuffers(1, &m_bo_id);

		m_bo_id = 0;
	}

	if (m_vao_id != 0)
	{
		gl.deleteVertexArrays(1, &m_vao_id);

		m_vao_id = 0;
	}
}

/** Initializes the test.
 *
 *  Note the function throws exception should an error occur!
 **/
void TessellationShaderPrimitiveCoverage::initTest(void)
{
	/* Skip if GL_EXT_tessellation_shader extension is not supported. */
	if (!m_is_tessellation_shader_supported)
	{
		throw tcu::NotSupportedError(TESSELLATION_SHADER_EXTENSION_NOT_SUPPORTED, "", __FILE__, __LINE__);
	}

	/* Retrieve ES entry-points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Generate and bind VAO */
	gl.genVertexArrays(1, &m_vao_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not generate vertex array object");

	gl.bindVertexArray(m_vao_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error binding vertex array object!");

	/* Initialize objects used by the test */
	initProgramObjects();
	initFramebuffer();
	initBufferObjects();

	/* setup of pixels buffer for fetching data*/
	m_rendered_data_buffer = (glw::GLubyte*)malloc(m_rendered_data_buffer_size);

	/* Enable stencil test. */
	gl.enable(GL_STENCIL_TEST);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Stencil test could not be enabled!");

	/* Set up viewport */
	gl.viewport(0 /* x */, 0 /* y */, m_width, m_height);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glViewport() failed!");
}

/** Initializes program objects used by the test.
 *
 *  Note the function throws exception should an error occur!
 **/
void TessellationShaderPrimitiveCoverage::initProgramObjects(void)
{
	/* Retrieve ES entry-points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Create program objects needed for the test */
	m_stencil_verification_po_id = gl.createProgram();
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateProgram() failed!");

	m_quad_tessellation_po_id = gl.createProgram();
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateProgram() failed!");

	m_triangles_tessellation_po_id = gl.createProgram();
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateProgram() failed!");

	/* Set up shader objects */
	m_vs_id = gl.createShader(GL_VERTEX_SHADER);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateShader(GL_VERTEX_SHADER) failed!");

	m_quad_tessellation_tcs_id = gl.createShader(m_glExtTokens.TESS_CONTROL_SHADER);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateShader(GL_TESS_CONTROL_SHADER_EXT) failed!");

	m_quad_tessellation_tes_id = gl.createShader(m_glExtTokens.TESS_EVALUATION_SHADER);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateShader(GL_TESS_EVALUATION_SHADER_EXT) failed!");

	m_triangles_tessellation_tcs_id = gl.createShader(m_glExtTokens.TESS_CONTROL_SHADER);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateShader(GL_TESS_CONTROL_SHADER_EXT) failed!");

	m_triangles_tessellation_tes_id = gl.createShader(m_glExtTokens.TESS_EVALUATION_SHADER);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateShader(GL_TESS_EVALUATION_SHADER_EXT) failed!");

	m_fs_id = gl.createShader(GL_FRAGMENT_SHADER);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateShader(GL_FRAGMENT_SHADER) failed!");

	/* Build a program object that does not define the tessellation stage */
	if (!buildProgram(m_stencil_verification_po_id, m_vs_id, 1, &m_vs_code, m_fs_id, 1, &m_fs_code))
	{
		TCU_FAIL("Could not create a program object");
	}

	/* Build a program object that uses quad tessellation */
	if (!buildProgram(m_quad_tessellation_po_id, m_vs_id, 1, &m_vs_code, m_quad_tessellation_tcs_id, 1,
					  &m_quad_tessellation_tcs_code, m_quad_tessellation_tes_id, 1, &m_quad_tessellation_tes_code,
					  m_fs_id, 1, &m_fs_code))
	{
		TCU_FAIL("Could not create a program object");
	}

	/* Build a program object that uses triangle tessellation */
	if (!buildProgram(m_triangles_tessellation_po_id, m_vs_id, 1, &m_vs_code, m_triangles_tessellation_tcs_id, 1,
					  &m_triangles_tessellation_tcs_code, m_triangles_tessellation_tes_id, 1,
					  &m_triangles_tessellation_tes_code, m_fs_id, 1, &m_fs_code))
	{
		TCU_FAIL("Could not create a program object");
	}
}

/** Initializes a framebuffer object.
 *
 *  Note the function throws exception should an error occur!
 **/
void TessellationShaderPrimitiveCoverage::initFramebuffer(void)
{
	/* Retrieve ES entry-points. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Framebuffer setup */
	gl.genFramebuffers(1, &m_fbo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenFramebuffers() failed!");

	gl.bindFramebuffer(GL_FRAMEBUFFER, m_fbo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffer() failed!");

	/* Set up a renderbuffer object and bind it as a color attachment to the
	 * framebuffer object */
	gl.genRenderbuffers(1, &m_color_rbo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenRenderbuffers() failed!");

	gl.bindRenderbuffer(GL_RENDERBUFFER, m_color_rbo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindRenderbuffer() failed!");

	gl.renderbufferStorage(GL_RENDERBUFFER, GL_RGBA8, m_width, m_height);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glRenderbufferStorage() failed!");

	gl.framebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, m_color_rbo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glFramebufferRenderbuffer() failed!");

	/* Set up a renderbuffer object and bind it as a stencil attachment to
	 * the framebuffer object */
	gl.genRenderbuffers(1, &m_stencil_rbo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenRenderbuffers() failed!");

	gl.bindRenderbuffer(GL_RENDERBUFFER, m_stencil_rbo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindRenderbuffer() failed!");

	gl.renderbufferStorage(GL_RENDERBUFFER, GL_STENCIL_INDEX8, m_width, m_height);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glRenderbufferStorage() failed!");

	gl.framebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_stencil_rbo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glFramebufferRenderbuffer() failed!");

	gl.bindRenderbuffer(GL_RENDERBUFFER, 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindRenderbuffer() failed!");

	/* Check framebuffer completness */
	if (gl.checkFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		TCU_FAIL("Test framebuffer has been reported as incomplete");
	}
}

/** Initializes buffer objects used by the test.
 *
 *  Note the function throws exception should an error occur!
 **/
void TessellationShaderPrimitiveCoverage::initBufferObjects(void)
{
	/* Retrieve ES entry-points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Set up an array of vertices that will be fed into vertex shader */
	glw::GLfloat vertices[6 /* vertices */ * m_n_components /* components */] = {
		-1.0f, 1.0f, 0.0f, 1.0f, -1.0f, -1.0f, 0.0f, 1.0f, 1.0f,
		-1.0f, 0.0f, 1.0f,		 /* A half-screen triangle (patch) (CCW) is defined until here */
		1.0f,  1.0f, 0.0f, 1.0f, /* A full screen quad (patch) (CCW) is defined until here */
		-1.0f, 1.0f, 0.0f, 1.0f, 1.0f,  -1.0f, 0.0f, 1.0f /* A full screen quad (2 triangles) (CCW) is defined until here */
	};

	/* Configure a buffer object to hold vertex data */
	gl.genBuffers(1, &m_bo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenBuffers() failed!");

	gl.bindBuffer(GL_ARRAY_BUFFER, m_bo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer() failed!");

	gl.bufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferData() failed!");

	/* Configure "position" vertex attribute array and enable it */
	gl.vertexAttribPointer(0,		 /* index */
						   4,		 /* size */
						   GL_FLOAT, /* type */
						   GL_FALSE, /* normalized */
						   0,		 /* stride */
						   0);		 /* pointer */
	GLU_EXPECT_NO_ERROR(gl.getError(), "glVertexAttribPointer() failed!");

	gl.enableVertexAttribArray(0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glEnableVertexAttribArray() failed!");
}

/** The function:
 *
 *  1) Clears the FBO's color/stencil attachments with zeros;
 *  2) Issues a draw call using a program object, for which either triangles or quads
 *     tessellation has been enabled. This draw call updates both color and stencil
 *     buffers of the framebuffer: fragment shader stores vec4(1) in the color attachment
 *     and stencil index is set to 0x1 for all affected fragments;
 *  3) Issues a draw call using another program object, this time without tessellation
 *     stage enabled. Stencil test is configured to only pass those fragments, for which
 *     stencil index is not equal to 0xFF.
 *
 *  Note the function throws exception should an error occur!
 *
 *  @param po_id                   program object handle to draw tesselated primitive
 *  @param n_patch_vertices        number of input vertices for a single patch (must
 *                                 be 3 for triangles or 4 for quads).
 *  @param n_draw_call_vertices    number of input vertices for a reference draw call
 *                                 (must be 3 for triangle or 6 for quad (2 triangles)).
 *  @param inner_levels            array of the inner tessellation levels
 *  @param outer_levels            array of the outer tessellation levels
 *
 *  @return false if the test failed, or true if test passed.
 **/
void TessellationShaderPrimitiveCoverage::drawPatch(glw::GLuint po_id, glw::GLuint n_patch_vertices,
													glw::GLuint n_draw_call_vertices, const glw::GLfloat inner_levels[],
													const glw::GLfloat outer_levels[])
{
	/* Sanity check */
	DE_ASSERT(n_draw_call_vertices == 3 || n_draw_call_vertices == 6);

	/* Retrieve ES entry-points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Activate user-provided program object with tessellation stage defined*/
	gl.useProgram(po_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram() failed!");

	/* Set up tessellation levels */
	glw::GLint innerLevelUniformLocation = -1;
	glw::GLint outerLevelUniformLocation = -1;

	innerLevelUniformLocation = gl.getUniformLocation(po_id, "innerLevel");
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetUniformLocation() failed!");

	outerLevelUniformLocation = gl.getUniformLocation(po_id, "outerLevel");
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetUniformLocation() failed!");

	gl.uniform2fv(innerLevelUniformLocation, 1 /* count */, inner_levels);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUniform2fv() failed!");

	gl.uniform4fv(outerLevelUniformLocation, 1 /* count */, outer_levels);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUniform4fv() failed!");

	gl.patchParameteri(m_glExtTokens.PATCH_VERTICES, n_patch_vertices);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glPatchParameteriEXT() failed!");

	/* Set up clear color */
	gl.clearColor(m_clear_color[0],  /* red */
				  m_clear_color[1],  /* green */
				  m_clear_color[2],  /* blue */
				  m_clear_color[3]); /* alpha */
	GLU_EXPECT_NO_ERROR(gl.getError(), "glClearColor() failed!");

	/* Set up fragment color to be used for the first stage */
	glw::GLint stencilPassColorUniformLocation = -1;

	stencilPassColorUniformLocation = gl.getUniformLocation(po_id, "stencil_fail_color");
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetUniformLocation() failed!");

	gl.uniform4fv(stencilPassColorUniformLocation, 1, m_stencil_pass_color);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUniform4fv() failed!");

	/* Draw to stencil buffer */
	gl.clearStencil(0 /* s */);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glClearStencil() failed!");

	gl.clear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glClear() failed!");

	gl.stencilOp(GL_REPLACE /* sfail */, GL_KEEP /* dpfail */, GL_KEEP /* dppass */);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glStencilOp() failed!");

	gl.stencilFunc(GL_NEVER /* func */, 1 /* ref */, 0xFF /* mask */);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glStencilFunc() failed!");

	gl.drawArrays(m_glExtTokens.PATCHES, 0 /* first */, n_patch_vertices);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawArrays() failed");

	/* Now verify the stencil buffer data contents by doing another
	 * full-screen draw call. This time without any tessellation.
	 * The pass will output fragments of predefined color, if stencil
	 * test passes (which will only happen if the stencil buffer is
	 * not filled with predefined index value).
	 */
	gl.useProgram(m_stencil_verification_po_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram() failed!");

	/* Color setup for 2nd program*/
	stencilPassColorUniformLocation = gl.getUniformLocation(m_stencil_verification_po_id, "stencil_fail_color");

	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetUniformLocation() failed!");

	gl.uniform4fv(stencilPassColorUniformLocation, 1 /* count */, m_stencil_pass_color);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUniform4fv() failed!");

	/* Draw to framebuffer */
	gl.stencilOp(GL_KEEP /* sfail */, GL_KEEP /* dpfail */, GL_KEEP /* dppass */);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glStencilOp() failed!");

	gl.stencilFunc(GL_NOTEQUAL /* func */, 1 /* ref */, 0xFF /* mask */);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glStencilFunc() failed!");

	gl.drawArrays(GL_TRIANGLES, 0 /* first */, n_draw_call_vertices);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawArrays() failed");
}

/** Retrieves data from test FBO's zeroth color attachment and verifies
 *  no cracks are present.
 *
 *  Note the function throws exception should an error occur!
 *
 *  @return false if the check failed or true if check passed.
 **/
bool TessellationShaderPrimitiveCoverage::verifyDrawBufferContents(void)
{
	/* Retrieve ES entry-points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Fetch the data */
	gl.readBuffer(GL_COLOR_ATTACHMENT0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glReadBuffer() failed");

	gl.readPixels(0,						  /* x */
				  0,						  /* y */
				  m_height, m_width, GL_RGBA, /* format */
				  GL_UNSIGNED_BYTE,			  /* type */
				  m_rendered_data_buffer);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glReadPixels() failed");

	/* Verify result data */
	bool result = true;

	for (glw::GLuint n_pixel = 0; n_pixel < (m_rendered_data_buffer_size >> 2); n_pixel += m_n_components)
	{
		glw::GLubyte expected_result_ubyte[] = { (glw::GLubyte)(m_clear_color[0] * 255.0f),
												 (glw::GLubyte)(m_clear_color[1] * 255.0f),
												 (glw::GLubyte)(m_clear_color[2] * 255.0f),
												 (glw::GLubyte)(m_clear_color[3] * 255.0f) };
		glw::GLubyte rendered_result_ubyte[] = { m_rendered_data_buffer[n_pixel + 0],
												 m_rendered_data_buffer[n_pixel + 1],
												 m_rendered_data_buffer[n_pixel + 2],
												 m_rendered_data_buffer[n_pixel + 3] };

		if (memcmp(expected_result_ubyte, rendered_result_ubyte, sizeof(rendered_result_ubyte)) != 0)
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "Rendered pixel at index (" << n_pixel << ") of value"
																									 " ("
							   << rendered_result_ubyte[0] << ", " << rendered_result_ubyte[1] << ", "
							   << rendered_result_ubyte[2] << ", " << rendered_result_ubyte[3]
							   << ") does not match expected pixel"
								  " ("
							   << expected_result_ubyte[0] << ", " << expected_result_ubyte[1] << ", "
							   << expected_result_ubyte[2] << ", " << expected_result_ubyte[3] << ")"
							   << tcu::TestLog::EndMessage;

			result = false;

			break;
		} /* if (rendered pixel is invalid) */
	}	 /* for (all pixels) */

	return result;
}

/** Executes the test.
 *
 *  Sets the test result to QP_TEST_RESULT_FAIL if the test failed, QP_TEST_RESULT_PASS otherwise.
 *
 *  Note the function throws exception should an error occur!
 *
 *  @return STOP if the test has finished, CONTINUE to indicate iterate should be called once again.
 **/
tcu::TestNode::IterateResult TessellationShaderPrimitiveCoverage::iterate(void)
{
	/* Retriveing GL. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Initialize test */
	initTest();

	bool has_test_passed = true;

	/* Define tessellation level values that we will use to draw single tessellated primitive */
	const glw::GLfloat inner_tess_levels_single[] = { 1.0f, 1.0f };
	const glw::GLfloat outer_tess_levels_single[] = { 1.0f, 1.0f, 1.0f, 1.0f };

	/* Retrieve GL_MAX_TESS_GEN_LEVEL_EXT value */
	glw::GLint gl_max_tess_gen_level_value = 0;

	gl.getIntegerv(m_glExtTokens.MAX_TESS_GEN_LEVEL, &gl_max_tess_gen_level_value);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv() call failed for GL_MAX_TESS_GEN_LEVEL_EXT pname");

	/* Verify no cracks can be found if a degenerate triangle
	 * is outputted by the tessellator */
	drawPatch(m_triangles_tessellation_po_id, 3 /* n_patch_vertices */, 3 /* n_triangle_vertices */,
			  inner_tess_levels_single, outer_tess_levels_single);

	has_test_passed &= verifyDrawBufferContents();

	/* Verify no cracks can be found if multiple triangles
	 * are outputted by the tessellator.
	 */
	_tessellation_levels_set levels_sets = TessellationShaderUtils::getTessellationLevelSetForPrimitiveMode(
		TESSELLATION_SHADER_PRIMITIVE_MODE_TRIANGLES, gl_max_tess_gen_level_value,
		TESSELLATION_LEVEL_SET_FILTER_ALL_LEVELS_USE_THE_SAME_VALUE);

	for (_tessellation_levels_set_const_iterator set_iterator = levels_sets.begin(); set_iterator != levels_sets.end();
		 set_iterator++)
	{
		const _tessellation_levels& set = *set_iterator;

		drawPatch(m_triangles_tessellation_po_id, 3 /* n_patch_vertices */, 3 /* n_triangle_vertices */, set.inner,
				  set.outer);

		has_test_passed &= verifyDrawBufferContents();
	}

	/* Verify no cracks can be found if a degenerate quad
	 * is outputted by the tessellator.
	 */
	drawPatch(m_quad_tessellation_po_id, 4 /* n_patch_vertices */, 6 /* n_triangle_vertices (quad == 2 triangles) */,
			  inner_tess_levels_single, outer_tess_levels_single);

	has_test_passed &= verifyDrawBufferContents();

	/* Verify no cracks can be found if multiple triangles
	 * (to which the generated quads will be broken into)
	 * are outputted by the tessellator.
	 */
	levels_sets = TessellationShaderUtils::getTessellationLevelSetForPrimitiveMode(
		TESSELLATION_SHADER_PRIMITIVE_MODE_QUADS, gl_max_tess_gen_level_value,
		TESSELLATION_LEVEL_SET_FILTER_ALL_LEVELS_USE_THE_SAME_VALUE);

	for (_tessellation_levels_set_const_iterator set_iterator = levels_sets.begin(); set_iterator != levels_sets.end();
		 set_iterator++)
	{
		const _tessellation_levels& set = *set_iterator;

		drawPatch(m_quad_tessellation_po_id, 4 /* n_patch_vertices */,
				  6 /* n_triangle_vertices (quad == 2 triangles) */, set.inner, set.outer);
		has_test_passed &= verifyDrawBufferContents();
	}

	/* Has the test passed? */
	if (has_test_passed)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
	}

	return STOP;
}

} /* namespace glcts */
