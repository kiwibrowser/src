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

#include "esextcGeometryShaderLayeredRendering.hpp"

#include "gluContextInfo.hpp"
#include "gluDefs.hpp"
#include "glwEnums.hpp"
#include "glwFunctions.hpp"
#include "tcuTestLog.hpp"

#include <algorithm>
#include <cstring>
#include <string>
#include <vector>

namespace glcts
{
/* Array holding vector values describing contents of layers
 * at subsequent indices.
 *
 * Contents of this array are directly related to layered_rendering_fs_code.
 **/
const unsigned char GeometryShaderLayeredRendering::m_layered_rendering_expected_layer_data[6 * 4] = {
	/* Layer 0 */
	255, 0, 0, 0,
	/* Layer 1 */
	0, 255, 0, 0,
	/* Layer 2 */
	0, 0, 255, 0,
	/* Layer 3 */
	0, 0, 0, 255,
	/* Layer 4 */
	255, 255, 0, 0,
	/* Layer 5 */
	255, 0, 255, 0
};

/* Fragment shader code */
const char* GeometryShaderLayeredRendering::m_layered_rendering_fs_code =
	"${VERSION}\n"
	"\n"
	"precision highp float;\n"
	"\n"
	"flat in  int  layer_id;\n"
	"     out vec4 color;\n"
	"\n"
	"void main()\n"
	"{\n"
	"    switch (layer_id)\n"
	"    {\n"
	"        case 0:  color = vec4(1, 0, 0, 0); break;\n"
	"        case 1:  color = vec4(0, 1, 0, 0); break;\n"
	"        case 2:  color = vec4(0, 0, 1, 0); break;\n"
	"        case 3:  color = vec4(0, 0, 0, 1); break;\n"
	"        case 4:  color = vec4(1, 1, 0, 0); break;\n"
	"        case 5:  color = vec4(1, 0, 1, 0); break;\n"
	"        default: color = vec4(1, 1, 1, 1); break;\n"
	"    }\n"
	"}\n";

/* Geometry shader code parts */
const char* GeometryShaderLayeredRendering::m_layered_rendering_gs_code_preamble = "${VERSION}\n"
																				   "${GEOMETRY_SHADER_REQUIRE}\n"
																				   "\n";

const char* GeometryShaderLayeredRendering::m_layered_rendering_gs_code_2d_array = "#define MAX_VERTICES 64\n"
																				   "#define N_LAYERS     4\n";

const char* GeometryShaderLayeredRendering::m_layered_rendering_gs_code_2d_marray = "#define MAX_VERTICES 64\n"
																					"#define N_LAYERS     4\n";

const char* GeometryShaderLayeredRendering::m_layered_rendering_gs_code_3d = "#define MAX_VERTICES 64\n"
																			 "#define N_LAYERS     4\n";

const char* GeometryShaderLayeredRendering::m_layered_rendering_gs_code_cm = "#define MAX_VERTICES 96\n"
																			 "#define N_LAYERS     6\n";

/* NOTE: provoking_vertex_index holds an integer value which represents platform-reported
 *       GL_LAYER_PROVOKING_VERTEX_EXT value. The meaning is as follows:
 *
 *       0: Property carries a GL_UNDEFINED_VERTEX_EXT value. Need to set gl_Layer for all
 *          vertices.
 *       1: Property carries a GL_FIRST_VERTEX_CONVENTION_EXT value. Need to set gl_Layer for
 *          the first two vertices, since these are a part of the two triangles, emitted
 *          separately for each layer.
 *       2: Property carries a GL_LAST_VERTEX_CONVENTION_EXT value. Need to set gl_Layer for
 *          the last two vertices, since these are a part of the two triangles, emitted
 *          separately for each layer.
 */
const char* GeometryShaderLayeredRendering::m_layered_rendering_gs_code_main =
	"layout(points)                                    in;\n"
	"layout(triangle_strip, max_vertices=MAX_VERTICES) out;\n"
	"\n"
	"precision highp float;\n"
	"\n"
	"flat out int layer_id;\n"
	"uniform  int provoking_vertex_index;\n"
	"\n"
	"void main()\n"
	"{\n"
	"    for (int n = 0; n < N_LAYERS; ++n)\n"
	"    {\n"
	"        #ifndef SHOULD_NOT_SET_GL_LAYER\n"
	"            if (provoking_vertex_index == 0 || provoking_vertex_index == 1) gl_Layer = n;\n"
	"        #endif\n"
	"\n"
	"        layer_id    = gl_Layer;\n"
	"        gl_Position = vec4(1, 1, 0, 1);\n"
	"        EmitVertex();\n"
	"\n"
	"        #ifndef SHOULD_NOT_SET_GL_LAYER\n"
	"            if (provoking_vertex_index == 0 || provoking_vertex_index == 1) gl_Layer = n;\n"
	"        #endif\n"
	"\n"
	"        layer_id    = gl_Layer;\n"
	"        gl_Position = vec4(1, -1, 0, 1);\n"
	"        EmitVertex();\n"
	"\n"
	"        #ifndef SHOULD_NOT_SET_GL_LAYER\n"
	"            if (provoking_vertex_index == 0 || provoking_vertex_index == 2) gl_Layer = n;\n"
	"        #endif\n"
	"\n"
	"        layer_id    = gl_Layer;\n"
	"        gl_Position = vec4(-1, 1, 0, 1);\n"
	"        EmitVertex();\n"
	"\n"
	"        #ifndef SHOULD_NOT_SET_GL_LAYER\n"
	"            gl_Layer = n;\n"
	"        #endif\n"
	"\n"
	"        layer_id    = gl_Layer;\n"
	"        gl_Position = vec4(-1, -1, 0, 1);\n"
	"        EmitVertex();\n"
	"\n"
	"        EndPrimitive();\n"
	"    }\n"
	"}\n";

/* Vertex shader */
const char* GeometryShaderLayeredRendering::m_layered_rendering_vs_code = "${VERSION}\n"
																		  "\n"
																		  "precision highp float;\n"
																		  "\n"
																		  "flat out int layer_id;\n"
																		  "void main()\n"
																		  "{\n"
																		  "    layer_id = 0;\n"
																		  "}\n";

/* Constants used for various test iterations */
#define TEXTURE_DEPTH (64)
#define TEXTURE_HEIGHT (32)
#define TEXTURE_N_COMPONENTS (4)
#define TEXTURE_WIDTH (32)

/* Constructor */
GeometryShaderLayeredRendering::GeometryShaderLayeredRendering(Context& context, const ExtParameters& extParams,
															   const char* name, const char* description)
	: TestCaseBase(context, extParams, name, description), m_vao_id(0)
{
	memset(m_tests, 0, sizeof(m_tests));
}

/** Builds a GL program specifically for a layer rendering test instance.
 *
 *  @param test Layered Rendering test to consider.
 *
 *  @return GTFtrue if successful, false otherwise.
 **/
bool GeometryShaderLayeredRendering::buildProgramForLRTest(_layered_rendering_test* test)
{
	return buildProgram(test->po_id, test->fs_id, test->n_fs_parts, test->fs_parts, test->gs_id, test->n_gs_parts,
						test->gs_parts, test->vs_id, test->n_vs_parts, test->vs_parts);
}

/** Executes the test.
 *  Sets the test result to QP_TEST_RESULT_FAIL if the test failed, QP_TEST_RESULT_PASS otherwise.
 *  @return STOP if the test has finished, CONTINUE to indicate iterate should be called once again.
 *  Note the function throws exception should an error occur!
 **/
tcu::TestNode::IterateResult GeometryShaderLayeredRendering::iterate(void)
{
	const glu::ContextType& context_type = m_context.getRenderContext().getType();
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Helper variables to support shader compilation process */
	const char* cm_fs_parts[] = { m_layered_rendering_fs_code };
	const char* cm_gs_parts[] = { m_layered_rendering_gs_code_preamble, m_layered_rendering_gs_code_cm,
								  m_layered_rendering_gs_code_main };
	const char* cm_vs_parts[]				= { m_layered_rendering_vs_code };
	const char* threedimensional_fs_parts[] = { m_layered_rendering_fs_code };
	const char* threedimensional_gs_parts[] = { m_layered_rendering_gs_code_preamble, m_layered_rendering_gs_code_3d,
												m_layered_rendering_gs_code_main };
	const char* threedimensional_vs_parts[] = { m_layered_rendering_vs_code };
	const char* twodimensionala_fs_parts[]  = { m_layered_rendering_fs_code };
	const char* twodimensionala_gs_parts[]  = { m_layered_rendering_gs_code_preamble,
											   m_layered_rendering_gs_code_2d_array, m_layered_rendering_gs_code_main };
	const char* twodimensionala_vs_parts[]  = { m_layered_rendering_vs_code };
	const char* twodimensionalma_fs_parts[] = { m_layered_rendering_fs_code };
	const char* twodimensionalma_gs_parts[] = { m_layered_rendering_gs_code_preamble,
												m_layered_rendering_gs_code_2d_marray,
												m_layered_rendering_gs_code_main };
	const char*		   twodimensionalma_vs_parts[] = { m_layered_rendering_vs_code };
	const unsigned int n_cm_fs_parts			   = sizeof(cm_fs_parts) / sizeof(cm_fs_parts[0]);
	const unsigned int n_cm_gs_parts			   = sizeof(cm_gs_parts) / sizeof(cm_gs_parts[0]);
	const unsigned int n_cm_vs_parts			   = sizeof(cm_vs_parts) / sizeof(cm_vs_parts[0]);
	const unsigned int n_threedimensional_fs_parts =
		sizeof(threedimensional_fs_parts) / sizeof(threedimensional_fs_parts[0]);
	const unsigned int n_threedimensional_gs_parts =
		sizeof(threedimensional_gs_parts) / sizeof(threedimensional_gs_parts[0]);
	const unsigned int n_threedimensional_vs_parts =
		sizeof(threedimensional_vs_parts) / sizeof(threedimensional_vs_parts[0]);
	const unsigned int n_twodimensionala_fs_parts =
		sizeof(twodimensionala_fs_parts) / sizeof(twodimensionala_fs_parts[0]);
	const unsigned int n_twodimensionala_gs_parts =
		sizeof(twodimensionala_gs_parts) / sizeof(twodimensionala_gs_parts[0]);
	const unsigned int n_twodimensionala_vs_parts =
		sizeof(twodimensionala_vs_parts) / sizeof(twodimensionala_vs_parts[0]);
	const unsigned int n_twodimensionalma_fs_parts =
		sizeof(twodimensionalma_fs_parts) / sizeof(twodimensionalma_fs_parts[0]);
	const unsigned int n_twodimensionalma_gs_parts =
		sizeof(twodimensionalma_gs_parts) / sizeof(twodimensionalma_gs_parts[0]);
	const unsigned int n_twodimensionalma_vs_parts =
		sizeof(twodimensionalma_vs_parts) / sizeof(twodimensionalma_vs_parts[0]);

	/* General-use helper variables */
	unsigned int n_current_test = 0;

	/* This test should only run if EXT_geometry_shader is supported */
	if (!m_is_geometry_shader_extension_supported)
	{
		throw tcu::NotSupportedError(GEOMETRY_SHADER_EXTENSION_NOT_SUPPORTED, "", __FILE__, __LINE__);
	}

	/* Configure test descriptors */
	m_tests[LAYERED_RENDERING_TEST_ITERATION_CUBEMAP].iteration  = LAYERED_RENDERING_TEST_ITERATION_CUBEMAP;
	m_tests[LAYERED_RENDERING_TEST_ITERATION_CUBEMAP].n_layers   = 6; /* faces */
	m_tests[LAYERED_RENDERING_TEST_ITERATION_CUBEMAP].fs_parts   = cm_fs_parts;
	m_tests[LAYERED_RENDERING_TEST_ITERATION_CUBEMAP].gs_parts   = cm_gs_parts;
	m_tests[LAYERED_RENDERING_TEST_ITERATION_CUBEMAP].vs_parts   = cm_vs_parts;
	m_tests[LAYERED_RENDERING_TEST_ITERATION_CUBEMAP].n_fs_parts = n_cm_fs_parts;
	m_tests[LAYERED_RENDERING_TEST_ITERATION_CUBEMAP].n_gs_parts = n_cm_gs_parts;
	m_tests[LAYERED_RENDERING_TEST_ITERATION_CUBEMAP].n_vs_parts = n_cm_vs_parts;

	m_tests[LAYERED_RENDERING_TEST_ITERATION_2D_ARRAY].iteration  = LAYERED_RENDERING_TEST_ITERATION_2D_ARRAY;
	m_tests[LAYERED_RENDERING_TEST_ITERATION_2D_ARRAY].n_layers   = 4; /* layers */
	m_tests[LAYERED_RENDERING_TEST_ITERATION_2D_ARRAY].fs_parts   = twodimensionala_fs_parts;
	m_tests[LAYERED_RENDERING_TEST_ITERATION_2D_ARRAY].gs_parts   = twodimensionala_gs_parts;
	m_tests[LAYERED_RENDERING_TEST_ITERATION_2D_ARRAY].vs_parts   = twodimensionala_vs_parts;
	m_tests[LAYERED_RENDERING_TEST_ITERATION_2D_ARRAY].n_fs_parts = n_twodimensionala_fs_parts;
	m_tests[LAYERED_RENDERING_TEST_ITERATION_2D_ARRAY].n_gs_parts = n_twodimensionala_gs_parts;
	m_tests[LAYERED_RENDERING_TEST_ITERATION_2D_ARRAY].n_vs_parts = n_twodimensionala_vs_parts;

	m_tests[LAYERED_RENDERING_TEST_ITERATION_3D].iteration  = LAYERED_RENDERING_TEST_ITERATION_3D;
	m_tests[LAYERED_RENDERING_TEST_ITERATION_3D].n_layers   = 4; /* layers */
	m_tests[LAYERED_RENDERING_TEST_ITERATION_3D].fs_parts   = threedimensional_fs_parts;
	m_tests[LAYERED_RENDERING_TEST_ITERATION_3D].gs_parts   = threedimensional_gs_parts;
	m_tests[LAYERED_RENDERING_TEST_ITERATION_3D].vs_parts   = threedimensional_vs_parts;
	m_tests[LAYERED_RENDERING_TEST_ITERATION_3D].n_fs_parts = n_threedimensional_fs_parts;
	m_tests[LAYERED_RENDERING_TEST_ITERATION_3D].n_gs_parts = n_threedimensional_gs_parts;
	m_tests[LAYERED_RENDERING_TEST_ITERATION_3D].n_vs_parts = n_threedimensional_vs_parts;

	m_tests[LAYERED_RENDERING_TEST_ITERATION_2D_MULTISAMPLE_ARRAY].iteration =
		LAYERED_RENDERING_TEST_ITERATION_2D_MULTISAMPLE_ARRAY;
	m_tests[LAYERED_RENDERING_TEST_ITERATION_2D_MULTISAMPLE_ARRAY].n_layers   = 4; /* layers */
	m_tests[LAYERED_RENDERING_TEST_ITERATION_2D_MULTISAMPLE_ARRAY].fs_parts   = twodimensionalma_fs_parts;
	m_tests[LAYERED_RENDERING_TEST_ITERATION_2D_MULTISAMPLE_ARRAY].gs_parts   = twodimensionalma_gs_parts;
	m_tests[LAYERED_RENDERING_TEST_ITERATION_2D_MULTISAMPLE_ARRAY].vs_parts   = twodimensionalma_vs_parts;
	m_tests[LAYERED_RENDERING_TEST_ITERATION_2D_MULTISAMPLE_ARRAY].n_fs_parts = n_twodimensionalma_fs_parts;
	m_tests[LAYERED_RENDERING_TEST_ITERATION_2D_MULTISAMPLE_ARRAY].n_gs_parts = n_twodimensionalma_gs_parts;
	m_tests[LAYERED_RENDERING_TEST_ITERATION_2D_MULTISAMPLE_ARRAY].n_vs_parts = n_twodimensionalma_vs_parts;

	/* Create shader objects we'll use for the test */
	m_tests[LAYERED_RENDERING_TEST_ITERATION_CUBEMAP].fs_id = gl.createShader(GL_FRAGMENT_SHADER);
	m_tests[LAYERED_RENDERING_TEST_ITERATION_CUBEMAP].gs_id = gl.createShader(m_glExtTokens.GEOMETRY_SHADER);
	m_tests[LAYERED_RENDERING_TEST_ITERATION_CUBEMAP].vs_id = gl.createShader(GL_VERTEX_SHADER);

	m_tests[LAYERED_RENDERING_TEST_ITERATION_2D_ARRAY].fs_id = gl.createShader(GL_FRAGMENT_SHADER);
	m_tests[LAYERED_RENDERING_TEST_ITERATION_2D_ARRAY].gs_id = gl.createShader(m_glExtTokens.GEOMETRY_SHADER);
	m_tests[LAYERED_RENDERING_TEST_ITERATION_2D_ARRAY].vs_id = gl.createShader(GL_VERTEX_SHADER);

	m_tests[LAYERED_RENDERING_TEST_ITERATION_3D].fs_id = gl.createShader(GL_FRAGMENT_SHADER);
	m_tests[LAYERED_RENDERING_TEST_ITERATION_3D].gs_id = gl.createShader(m_glExtTokens.GEOMETRY_SHADER);
	m_tests[LAYERED_RENDERING_TEST_ITERATION_3D].vs_id = gl.createShader(GL_VERTEX_SHADER);

	m_tests[LAYERED_RENDERING_TEST_ITERATION_2D_MULTISAMPLE_ARRAY].fs_id = gl.createShader(GL_FRAGMENT_SHADER);
	m_tests[LAYERED_RENDERING_TEST_ITERATION_2D_MULTISAMPLE_ARRAY].gs_id =
		gl.createShader(m_glExtTokens.GEOMETRY_SHADER);
	m_tests[LAYERED_RENDERING_TEST_ITERATION_2D_MULTISAMPLE_ARRAY].vs_id = gl.createShader(GL_VERTEX_SHADER);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not create shaders!");

	/* Create program objects as well */
	m_tests[LAYERED_RENDERING_TEST_ITERATION_CUBEMAP].po_id  = gl.createProgram();
	m_tests[LAYERED_RENDERING_TEST_ITERATION_2D_ARRAY].po_id = gl.createProgram();
	m_tests[LAYERED_RENDERING_TEST_ITERATION_3D].po_id		 = gl.createProgram();

	m_tests[LAYERED_RENDERING_TEST_ITERATION_2D_MULTISAMPLE_ARRAY].po_id = gl.createProgram();

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not create programs!");

	/* Build the programs */
	if (!buildProgramForLRTest(&m_tests[LAYERED_RENDERING_TEST_ITERATION_CUBEMAP]) ||
		!buildProgramForLRTest(&m_tests[LAYERED_RENDERING_TEST_ITERATION_2D_ARRAY]) ||
		!buildProgramForLRTest(&m_tests[LAYERED_RENDERING_TEST_ITERATION_3D]) ||
		!buildProgramForLRTest(&m_tests[LAYERED_RENDERING_TEST_ITERATION_2D_MULTISAMPLE_ARRAY]))
	{
		TCU_FAIL("Could not create a program for cube-map texture layered rendering test!");
	}

	/* Set up provoking vertex uniform value, given the GL_LAYER_PROVOKING_VERTEX_EXT value. */
	glw::GLint layer_provoking_vertex_gl_value		= -1;
	glw::GLint layer_provoking_vertex_uniform_value = -1;

	gl.getIntegerv(m_glExtTokens.LAYER_PROVOKING_VERTEX, &layer_provoking_vertex_gl_value);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv() failed for GL_LAYER_PROVOKING_VERTEX_EXT pname");

	if (!glu::isContextTypeES(context_type) && ((glw::GLenum)layer_provoking_vertex_gl_value == GL_PROVOKING_VERTEX))
	{
		gl.getIntegerv(GL_PROVOKING_VERTEX, &layer_provoking_vertex_gl_value);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv() failed for GL_PROVOKING_VERTEX pname");
	}

	if ((glw::GLenum)layer_provoking_vertex_gl_value == m_glExtTokens.FIRST_VERTEX_CONVENTION)
	{
		layer_provoking_vertex_uniform_value = 1; /* as per geometry shader implementation */
	}
	else if ((glw::GLenum)layer_provoking_vertex_gl_value == m_glExtTokens.LAST_VERTEX_CONVENTION)
	{
		layer_provoking_vertex_uniform_value = 2; /* as per geometry shader implementation */
	}
	else if ((glw::GLenum)layer_provoking_vertex_gl_value == m_glExtTokens.UNDEFINED_VERTEX)
	{
		layer_provoking_vertex_uniform_value = 0; /* as per geometry shader implementation */
	}
	else
	{
		TCU_FAIL("Unrecognized value returned by glGetIntegerv() for GL_LAYER_PROVOKING_VERTEX_EXT pname.");
	}

	for (unsigned int test_index = 0; test_index < LAYERED_RENDERING_TEST_ITERATION_LAST; ++test_index)
	{
		glw::GLint provoking_vertex_index_uniform_location =
			gl.getUniformLocation(m_tests[test_index].po_id, "provoking_vertex_index");

		/* Sanity checks */
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGetUniformLocation() call generated an error");
		DE_ASSERT(provoking_vertex_index_uniform_location != -1);

		/* Assign the uniform value */
		gl.programUniform1i(m_tests[test_index].po_id, provoking_vertex_index_uniform_location,
							layer_provoking_vertex_uniform_value);

		GLU_EXPECT_NO_ERROR(gl.getError(), "glProgramUniform1i() call failed.");
	} /* for (all test iterations) */

	/* Initialize texture objects */
	gl.genTextures(1, &m_tests[LAYERED_RENDERING_TEST_ITERATION_CUBEMAP].to_id);
	gl.genTextures(1, &m_tests[LAYERED_RENDERING_TEST_ITERATION_2D_ARRAY].to_id);
	gl.genTextures(1, &m_tests[LAYERED_RENDERING_TEST_ITERATION_3D].to_id);
	gl.genTextures(1, &m_tests[LAYERED_RENDERING_TEST_ITERATION_2D_MULTISAMPLE_ARRAY].to_id);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not create texture object(s)!");

	gl.bindTexture(GL_TEXTURE_CUBE_MAP, m_tests[LAYERED_RENDERING_TEST_ITERATION_CUBEMAP].to_id);
	gl.texStorage2D(GL_TEXTURE_CUBE_MAP, 1 /* mip-map only */, GL_RGBA8, TEXTURE_WIDTH, TEXTURE_HEIGHT);

	gl.bindTexture(GL_TEXTURE_2D_ARRAY, m_tests[LAYERED_RENDERING_TEST_ITERATION_2D_ARRAY].to_id);
	gl.texStorage3D(GL_TEXTURE_2D_ARRAY, 1 /* mip-map only */, GL_RGBA8, TEXTURE_WIDTH, TEXTURE_HEIGHT, TEXTURE_DEPTH);

	gl.bindTexture(GL_TEXTURE_3D, m_tests[LAYERED_RENDERING_TEST_ITERATION_3D].to_id);
	gl.texStorage3D(GL_TEXTURE_3D, 1 /* mip-map only */, GL_RGBA8, TEXTURE_WIDTH, TEXTURE_HEIGHT, TEXTURE_DEPTH);

	gl.bindTexture(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES,
				   m_tests[LAYERED_RENDERING_TEST_ITERATION_2D_MULTISAMPLE_ARRAY].to_id);
	gl.texStorage3DMultisample(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES, 1 /* samples */, GL_RGBA8, TEXTURE_WIDTH,
							   TEXTURE_HEIGHT, TEXTURE_DEPTH, GL_FALSE /* fixed sample locations */);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not initialize texture object(s)!");

	/* Initialize framebuffer objects */
	gl.genFramebuffers(1, &m_tests[LAYERED_RENDERING_TEST_ITERATION_CUBEMAP].fbo_id);
	gl.genFramebuffers(1, &m_tests[LAYERED_RENDERING_TEST_ITERATION_2D_ARRAY].fbo_id);
	gl.genFramebuffers(1, &m_tests[LAYERED_RENDERING_TEST_ITERATION_3D].fbo_id);
	gl.genFramebuffers(1, &m_tests[LAYERED_RENDERING_TEST_ITERATION_2D_MULTISAMPLE_ARRAY].fbo_id);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not create framebuffer object(s)!");

	gl.bindFramebuffer(GL_DRAW_FRAMEBUFFER, m_tests[LAYERED_RENDERING_TEST_ITERATION_CUBEMAP].fbo_id);
	gl.framebufferTexture(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
						  m_tests[LAYERED_RENDERING_TEST_ITERATION_CUBEMAP].to_id, 0 /* base mip-map */);

	gl.bindFramebuffer(GL_DRAW_FRAMEBUFFER, m_tests[LAYERED_RENDERING_TEST_ITERATION_2D_ARRAY].fbo_id);
	gl.framebufferTexture(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
						  m_tests[LAYERED_RENDERING_TEST_ITERATION_2D_ARRAY].to_id, 0 /* base mip-map */);

	gl.bindFramebuffer(GL_DRAW_FRAMEBUFFER, m_tests[LAYERED_RENDERING_TEST_ITERATION_3D].fbo_id);
	gl.framebufferTexture(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, m_tests[LAYERED_RENDERING_TEST_ITERATION_3D].to_id,
						  0 /* base mip-map */);

	gl.bindFramebuffer(GL_DRAW_FRAMEBUFFER, m_tests[LAYERED_RENDERING_TEST_ITERATION_2D_MULTISAMPLE_ARRAY].fbo_id);
	gl.framebufferTexture(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
						  m_tests[LAYERED_RENDERING_TEST_ITERATION_2D_MULTISAMPLE_ARRAY].to_id, 0 /* base mip-map */);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not configure framebuffer object(s)!");

	/* Initialize vertex array object. We don't really use any attributes, but ES does not
	 * permit draw calls with an unbound VAO
	 */
	gl.genVertexArrays(1, &m_vao_id);
	gl.bindVertexArray(m_vao_id);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not bind a vertex array object!");

	/* Execute all iterations */
	for (n_current_test = 0; n_current_test < sizeof(m_tests) / sizeof(m_tests[0]); ++n_current_test)
	{
		unsigned char buffer[TEXTURE_WIDTH * TEXTURE_HEIGHT * TEXTURE_N_COMPONENTS] = { 0 };
		glw::GLuint   program_id													= 0;
		unsigned int  n_layer														= 0;
		glw::GLuint   texture_id													= 0;

		/* Bind the iteration-specific framebuffer */
		gl.bindFramebuffer(GL_FRAMEBUFFER, m_tests[n_current_test].fbo_id);

		program_id = m_tests[n_current_test].po_id;
		texture_id = m_tests[n_current_test].to_id;

		/* Clear the color attachment with (1, 1, 1, 1) which is not used for
		 * any layers.
		 */
		gl.clearColor(1.0f, 1.0f, 1.0f, 1.0f);
		gl.clear(GL_COLOR_BUFFER_BIT);

		/* Render a single point. This should result in full-screen quads drawn
		 * for each face/layer of the attachment bound to current FBO */
		gl.useProgram(program_id);
		gl.drawArrays(GL_POINTS, 0 /* first index */, 1 /* n points */);

		GLU_EXPECT_NO_ERROR(gl.getError(), "Error rendering geometry!");

		/* Read contents of each layer we rendered to and verify the contents */
		for (n_layer = 0; n_layer < m_tests[n_current_test].n_layers; ++n_layer)
		{
			const unsigned char* expected_data =
				m_layered_rendering_expected_layer_data + TEXTURE_N_COMPONENTS * n_layer;
			unsigned int n				 = 0;
			bool		 texture_layered = false;
			glw::GLenum  texture_target  = GL_NONE;

			/* What is the source attachment's texture target? */
			switch (m_tests[n_current_test].iteration)
			{
			case LAYERED_RENDERING_TEST_ITERATION_CUBEMAP:
			{
				texture_layered = false;
				texture_target  = (n_layer == 0) ?
									 GL_TEXTURE_CUBE_MAP_POSITIVE_X :
									 (n_layer == 1) ?
									 GL_TEXTURE_CUBE_MAP_NEGATIVE_X :
									 (n_layer == 2) ? GL_TEXTURE_CUBE_MAP_POSITIVE_Y :
													  (n_layer == 3) ? GL_TEXTURE_CUBE_MAP_NEGATIVE_Y :
																	   (n_layer == 4) ? GL_TEXTURE_CUBE_MAP_POSITIVE_Z :
																						GL_TEXTURE_CUBE_MAP_NEGATIVE_Z;

				break;
			}

			case LAYERED_RENDERING_TEST_ITERATION_2D_MULTISAMPLE_ARRAY:
			{
				texture_layered = true;
				texture_target  = GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES;

				break;
			}

			case LAYERED_RENDERING_TEST_ITERATION_3D:
			{
				texture_layered = true;
				texture_target  = GL_TEXTURE_3D;

				break;
			}

			case LAYERED_RENDERING_TEST_ITERATION_2D_ARRAY:
			{
				texture_layered = true;
				texture_target  = GL_TEXTURE_2D_ARRAY;

				break;
			}

			default:
			{
				TCU_FAIL("This location should never be reached");
			}
			}

			/* Configure the read framebuffer's read buffer, depending on whether the attachment
			 * uses layers or not
			 */
			if (texture_layered)
			{
				gl.framebufferTextureLayer(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, texture_id, 0 /* base mip-map */,
										   n_layer);
			}
			else
			{
				if (texture_target == GL_TEXTURE_CUBE_MAP_POSITIVE_X ||
					texture_target == GL_TEXTURE_CUBE_MAP_POSITIVE_Y ||
					texture_target == GL_TEXTURE_CUBE_MAP_POSITIVE_Z ||
					texture_target == GL_TEXTURE_CUBE_MAP_NEGATIVE_X ||
					texture_target == GL_TEXTURE_CUBE_MAP_NEGATIVE_Y ||
					texture_target == GL_TEXTURE_CUBE_MAP_NEGATIVE_Z)
				{
					gl.framebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, texture_target, texture_id,
											0 /* base mip-map */);
				}
				else
				{
					TCU_FAIL("This location should never be reached");
				}
			}

			/* Make sure read framebuffer was configured successfully */
			GLU_EXPECT_NO_ERROR(gl.getError(), "Error setting read framebuffer!");

			/* Read the data */
			if (m_tests[n_current_test].iteration == LAYERED_RENDERING_TEST_ITERATION_2D_MULTISAMPLE_ARRAY)
			{
				glw::GLuint new_dst_to = 0;
				glw::GLuint dst_fbo_id = 0;

				gl.genFramebuffers(1, &dst_fbo_id);

				gl.bindFramebuffer(GL_READ_FRAMEBUFFER, m_tests[n_current_test].fbo_id);
				gl.bindFramebuffer(GL_DRAW_FRAMEBUFFER, dst_fbo_id);

				gl.genTextures(1, &new_dst_to);
				gl.bindTexture(GL_TEXTURE_2D, new_dst_to);
				gl.texStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, TEXTURE_WIDTH, TEXTURE_HEIGHT);

				GLU_EXPECT_NO_ERROR(gl.getError(),
									"Could not setup texture object for draw framebuffer color attachment.");

				gl.framebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, new_dst_to, 0);

				GLU_EXPECT_NO_ERROR(gl.getError(),
									"Could not attach texture object to draw framebuffer color attachment.");

				gl.blitFramebuffer(0, 0, TEXTURE_WIDTH, TEXTURE_HEIGHT, 0, 0, TEXTURE_WIDTH, TEXTURE_HEIGHT,
								   GL_COLOR_BUFFER_BIT, GL_LINEAR);

				GLU_EXPECT_NO_ERROR(gl.getError(), "Error blitting from read framebuffer to draw framebuffer.");

				gl.bindFramebuffer(GL_READ_FRAMEBUFFER, dst_fbo_id);

				gl.readPixels(0 /* x */, 0 /* y */, TEXTURE_WIDTH, TEXTURE_HEIGHT, GL_RGBA, GL_UNSIGNED_BYTE, buffer);

				GLU_EXPECT_NO_ERROR(gl.getError(), "Error reading attachment's data!");

				gl.bindFramebuffer(GL_FRAMEBUFFER, m_tests[n_current_test].fbo_id);

				gl.deleteFramebuffers(1, &dst_fbo_id);
				gl.deleteTextures(1, &new_dst_to);
			}
			else
			{
				gl.readPixels(0 /* x */, 0 /* y */, TEXTURE_WIDTH, TEXTURE_HEIGHT, GL_RGBA, GL_UNSIGNED_BYTE, buffer);

				GLU_EXPECT_NO_ERROR(gl.getError(), "Error reading attachment's data!");
			}

			/* Validate it */
			for (; n < TEXTURE_WIDTH * TEXTURE_HEIGHT; ++n)
			{
				unsigned char* data_ptr = buffer + n * TEXTURE_N_COMPONENTS;

				if (memcmp(data_ptr, expected_data, TEXTURE_N_COMPONENTS) != 0)
				{
					m_testCtx.getLog() << tcu::TestLog::Message << "Rendered data [" << data_ptr[0] << ", "
									   << data_ptr[1] << ", " << data_ptr[2] << ", " << data_ptr[3]
									   << "] is different from reference data ["
									   << m_layered_rendering_expected_layer_data[0] << ", "
									   << m_layered_rendering_expected_layer_data[1] << ", "
									   << m_layered_rendering_expected_layer_data[2] << ", "
									   << m_layered_rendering_expected_layer_data[3] << "] !"
									   << tcu::TestLog::EndMessage;

					m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
					return STOP;
				} /* if (data comparison failed) */
			}	 /* for (all pixels) */
		}		  /* for (all layers) */
	}			  /* for (all iterations) */

	/* Done! */
	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	return STOP;
}

/** Deinitializes GLES objects created during the test.
 *
 */
void GeometryShaderLayeredRendering::deinit(void)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Reset OpenGL ES state */
	gl.useProgram(0);
	gl.framebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0 /* texture */, 0 /* level */);
	gl.bindTexture(GL_TEXTURE_2D, 0 /* texture */);
	gl.bindFramebuffer(GL_FRAMEBUFFER, 0);
	gl.bindVertexArray(0);

	for (unsigned int n_current_test = 0; n_current_test < sizeof(m_tests) / sizeof(m_tests[0]); ++n_current_test)
	{
		if (m_tests[n_current_test].po_id != 0)
		{
			gl.deleteProgram(m_tests[n_current_test].po_id);
		}

		if (m_tests[n_current_test].fs_id != 0)
		{
			gl.deleteShader(m_tests[n_current_test].fs_id);
		}

		if (m_tests[n_current_test].gs_id != 0)
		{
			gl.deleteShader(m_tests[n_current_test].gs_id);
		}

		if (m_tests[n_current_test].vs_id != 0)
		{
			gl.deleteShader(m_tests[n_current_test].vs_id);
		}

		if (m_tests[n_current_test].to_id != 0)
		{
			gl.deleteTextures(1, &m_tests[n_current_test].to_id);
		}

		if (m_tests[n_current_test].fbo_id != 0)
		{
			gl.deleteFramebuffers(1, &m_tests[n_current_test].fbo_id);
		}
	} /* for (all tests) */

	if (m_vao_id != 0)
	{
		gl.deleteVertexArrays(1, &m_vao_id);
	}

	/* Release base class */
	TestCaseBase::deinit();
}

} // namespace glcts
