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

#include "esextcTessellationShaderMaxPatchVertices.hpp"
#include "gluContextInfo.hpp"
#include "gluDefs.hpp"
#include "glwEnums.hpp"
#include "glwFunctions.hpp"
#include "tcuTestLog.hpp"
#include <cstddef>
#include <cstdlib>

/* The test creates a patch with max patch vertices count size.
 * The output from the tesselation stage is a single segment (2 vertices).
 * Use this define when allocating/using the output TF buffer.
 */
#define OUTPUT_VERTEX_COUNT 2

namespace glcts
{

/* Vertex Shader code */
const char* TessellationShaderMaxPatchVertices::m_vs_code = "${VERSION}\n"
															"\n"
															"${SHADER_IO_BLOCKS_ENABLE}\n"
															"\n"
															"precision highp float;\n"
															"\n"
															"layout(location = 0) in vec4  in_fv;\n"
															"layout(location = 1) in ivec4 in_iv;\n"
															"\n"
															"out Vertex\n"
															"{\n"
															"    ivec4 iv;\n"
															"    vec4  fv;\n"
															"} outVertex;\n"
															"\n"
															"void main()\n"
															"{\n"
															"    gl_Position  = in_fv;\n"
															"    outVertex.iv = in_iv;\n"
															"    outVertex.fv = in_fv;\n"
															"}\n";

/* Tessellation Control Shader code (for case with explicit array size) */
const char* TessellationShaderMaxPatchVertices::m_tc_code =
	"${VERSION}\n"
	"\n"
	"${TESSELLATION_SHADER_REQUIRE}\n"
	"\n"
	"precision highp float;\n"
	"\n"
	"in Vertex\n"
	"{\n"
	"    ivec4 iv;\n"
	"    vec4  fv;\n"
	"} inVertex[];\n"
	"\n"
	"layout(vertices = 2) out;\n" /* One segment only. */
	"\n"
	"out Vertex\n"
	"{\n"
	"    ivec4 iv;\n"
	"    vec4  fv;\n"
	"} outVertex[];\n"
	"\n"
	"void main()\n"
	"{\n"
	"    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;\n"
	"    outVertex[gl_InvocationID].iv = ivec4(0);\n"
	"    outVertex[gl_InvocationID].fv = vec4(0);\n"
	"\n"
	"    for (int i = 0; i < gl_PatchVerticesIn; i++)\n"
	"    {\n"
	"        outVertex[gl_InvocationID].iv += inVertex[i].iv;\n"
	"        outVertex[gl_InvocationID].fv += inVertex[i].fv;\n"
	"    }\n"
	"\n"
	"    gl_TessLevelInner[0] = 1.0;\n"
	"    gl_TessLevelInner[1] = 1.0;\n"
	"    gl_TessLevelOuter[0] = 1.0;\n"
	"    gl_TessLevelOuter[1] = 1.0;\n"
	"    gl_TessLevelOuter[2] = 1.0;\n"
	"    gl_TessLevelOuter[3] = 1.0;\n"
	"}\n";

/* Tessellation Evaluation Shader code (for case */
const char* TessellationShaderMaxPatchVertices::m_te_code = "${VERSION}\n"
															"\n"
															"${TESSELLATION_SHADER_REQUIRE}\n"
															"\n"
															"precision highp float;\n"
															"\n"
															"layout (isolines, point_mode) in;\n"
															"\n"
															"in Vertex\n"
															"{\n"
															"    ivec4 iv;\n"
															"    vec4  fv;\n"
															"} inVertex[];\n"
															"\n"
															"out  vec4 result_fv;\n"
															"out ivec4 result_iv;\n"
															"\n"
															"void main()\n"
															"{\n"
															"    gl_Position = gl_in[0].gl_Position;\n"
															"    result_iv   = ivec4(0);\n"
															"    result_fv   = vec4(0.0);\n"
															"\n"
															"    for (int i = 0 ; i < gl_PatchVerticesIn; i++)\n"
															"    {\n"
															"        result_iv += inVertex[i].iv;\n"
															"        result_fv += inVertex[i].fv;\n"
															"    }\n"
															"}\n";

/* Fragment Shader code */
const char* TessellationShaderMaxPatchVertices::m_fs_code = "${VERSION}\n"
															"\n"
															"void main()\n"
															"{\n"
															"}\n";

/* Transform Feedback varyings */
const char* const TessellationShaderMaxPatchVertices::m_tf_varyings[] = { "result_fv", "result_iv" };

/** Constructor
 *
 * @param context     Test context
 * @param name        Test case's name
 * @param description Test case's description
 **/
TessellationShaderMaxPatchVertices::TessellationShaderMaxPatchVertices(Context& context, const ExtParameters& extParams)
	: TestCaseBase(context, extParams, "max_patch_vertices",
				   "Make sure it is possible to use up to gl_MaxPatchVertices vertices."
				   " TCS must be able to correctly access all vertices in an input patch")
	, m_bo_id_f_1(0)
	, m_bo_id_f_2(0)
	, m_bo_id_i_1(0)
	, m_bo_id_i_2(0)
	, m_fs_id(0)
	, m_po_id_1(0)
	, m_po_id_2(0)
	, m_tc_id(0)
	, m_te_id(0)
	, m_tf_id_1(0)
	, m_tf_id_2(0)
	, m_vs_id(0)
	, m_vao_id(0)
	, m_gl_max_patch_vertices(0)
	, m_patch_vertices_bo_f_id(0)
	, m_patch_vertices_bo_i_id(0)
	, m_patch_vertices_f(DE_NULL)
	, m_patch_vertices_i(DE_NULL)
{
}

/** Deinitializes all ES objects created for the test. */
void TessellationShaderMaxPatchVertices::deinit(void)
{
	/* Deinitialize parent. */
	TestCaseBase::deinit();

	if (!m_is_tessellation_shader_supported)
	{
		return;
	}

	/* Dealocate input array of patch vertices. */
	if (m_patch_vertices_f != DE_NULL)
	{
		free(m_patch_vertices_f);

		m_patch_vertices_f = DE_NULL;
	}

	if (m_patch_vertices_i != DE_NULL)
	{
		free(m_patch_vertices_i);

		m_patch_vertices_i = DE_NULL;
	}

	/* Retrieve ES entry-points. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Set back to default program */
	gl.useProgram(0);

	/* Revert GL_PATCH_VERTICES_EXT value to the default setting */
	gl.patchParameteri(m_glExtTokens.PATCH_VERTICES, 3);

	if (!glu::isContextTypeES(m_context.getRenderContext().getType()))
	{
		/* Revert GL_PATCH_DEFAULT_INNER_LEVEL and GL_PATCH_DEFAULT_OUTER_LEVEL pname
		 * values to the default settings */
		const float default_levels[] = { 1.0f, 1.0f, 1.0f, 1.0f };

		gl.patchParameterfv(GL_PATCH_DEFAULT_INNER_LEVEL, default_levels);
		gl.patchParameterfv(GL_PATCH_DEFAULT_OUTER_LEVEL, default_levels);
	}

	/* Disable vertex attribute arrays that may have been enabled for the test */
	gl.disableVertexAttribArray(0);
	gl.disableVertexAttribArray(1);

	/* Unbind buffer objects from TF binding points */
	gl.bindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, 0 /* buffer */);
	gl.bindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0 /* index */, 0 /* buffer */);
	gl.bindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 1 /* index */, 0 /* buffer */);

	/* Unbind transform feedback object */
	gl.bindTransformFeedback(GL_TRANSFORM_FEEDBACK, 0 /* id */);

	/* Disable GL_RASTERIZER_DISCARD mode */
	gl.disable(GL_RASTERIZER_DISCARD);

	/* Unbind vertex array object */
	gl.bindVertexArray(0);

	/* Delete OpenGL objects */
	if (m_bo_id_f_1 != 0)
	{
		gl.deleteBuffers(1, &m_bo_id_f_1);

		m_bo_id_f_1 = 0;
	}

	if (m_bo_id_f_2 != 0)
	{
		gl.deleteBuffers(1, &m_bo_id_f_2);

		m_bo_id_f_2 = 0;
	}

	if (m_bo_id_i_1 != 0)
	{
		gl.deleteBuffers(1, &m_bo_id_i_1);

		m_bo_id_i_1 = 0;
	}

	if (m_bo_id_i_2 != 0)
	{
		gl.deleteBuffers(1, &m_bo_id_i_2);

		m_bo_id_i_2 = 0;
	}

	if (m_patch_vertices_bo_f_id != 0)
	{
		gl.deleteBuffers(1, &m_patch_vertices_bo_f_id);

		m_patch_vertices_bo_f_id = 0;
	}

	if (m_patch_vertices_bo_i_id != 0)
	{
		gl.deleteBuffers(1, &m_patch_vertices_bo_i_id);

		m_patch_vertices_bo_i_id = 0;
	}

	if (m_vs_id != 0)
	{
		gl.deleteShader(m_vs_id);

		m_vs_id = 0;
	}

	if (m_tc_id != 0)
	{
		gl.deleteShader(m_tc_id);

		m_tc_id = 0;
	}

	if (m_te_id != 0)
	{
		gl.deleteShader(m_te_id);

		m_te_id = 0;
	}

	if (m_fs_id != 0)
	{
		gl.deleteShader(m_fs_id);

		m_fs_id = 0;
	}

	if (m_tf_id_1 != 0)
	{
		gl.deleteTransformFeedbacks(1, &m_tf_id_1);

		m_tf_id_1 = 0;
	}

	if (m_tf_id_2 != 0)
	{
		gl.deleteTransformFeedbacks(1, &m_tf_id_2);

		m_tf_id_2 = 0;
	}

	if (m_po_id_1 != 0)
	{
		gl.deleteProgram(m_po_id_1);

		m_po_id_1 = 0;
	}

	if (m_po_id_2 != 0)
	{
		gl.deleteProgram(m_po_id_2);

		m_po_id_2 = 0;
	}

	if (m_vao_id != 0)
	{
		gl.deleteVertexArrays(1, &m_vao_id);

		m_vao_id = 0;
	}
}

/** Initializes all ES objects and reference values for the test. */
void TessellationShaderMaxPatchVertices::initTest(void)
{
	/* This test runs for two cases:
	 *
	 * 1) The patch size is explicitly defined to be equal to gl_MaxPatchVertices.
	 *    (The Tessellation Control Shader gets 32 vertices, then it access them
	 *    and it outputs 2 vertices to Tessllation Evaluation Shader. Next Tessllation
	 *    Evaluation Shader sends 1 segment of an isoline  to the output).
	 * 2) The patch size is implicitly defined to be equal to gl_MaxPatchVertices.
	 *    (There is no Tessellation Control Shader. Tessllation Evaluation Shader
	 *    gets 32 vertices, then it access them. Next (gl_MaxPatchVertices-1) segments
	 *    of the isoline are send to the output.)
	 */

	/* Retrieve ES entry/state points. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Initialize vertex array object */
	gl.genVertexArrays(1, &m_vao_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not generate vertex array object");

	gl.bindVertexArray(m_vao_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error binding vertex array object!");

	/* Query GL_MAX_PATCH_VERTICES_EXT value */
	gl.getIntegerv(m_glExtTokens.MAX_PATCH_VERTICES, &m_gl_max_patch_vertices);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv() failed for GL_MAX_PATCH_VERTICES_EXT pname!");

	/* Set maximum number of vertices in the patch. */
	gl.patchParameteri(m_glExtTokens.PATCH_VERTICES, m_gl_max_patch_vertices);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glPatchParameteriEXT() failed for GL_PATCH_VERTICES_EXT pname!");

	/* Build programs */
	initProgramObjects();

	/* Initialize tessellation buffers */
	initTransformFeedbackBufferObjects();

	/* Initialize input vertices */
	initVertexBufferObjects();

	/* Reference values setup */
	initReferenceValues();
}

/** Initializes buffer objects for the test. */
void TessellationShaderMaxPatchVertices::initVertexBufferObjects(void)
{
	/* Retrieve ES entry/state points. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Input patch vertices buffer setup. */
	m_patch_vertices_f = (glw::GLfloat*)malloc(m_gl_max_patch_vertices * 4 /* components */ * sizeof(glw::GLfloat));

	if (m_patch_vertices_f == DE_NULL)
	{
		TCU_FAIL("Memory allocation failed!");
	}

	m_patch_vertices_i = (glw::GLint*)malloc(m_gl_max_patch_vertices * 4 /* components */ * sizeof(glw::GLint));

	if (m_patch_vertices_i == DE_NULL)
	{
		TCU_FAIL("Memory allocation failed!");
	}

	for (int i = 0; i < m_gl_max_patch_vertices * 4 /* components */; i += 4 /* components */)
	{
		m_patch_vertices_f[i]	 = 1.0f;
		m_patch_vertices_f[i + 1] = 2.0f;
		m_patch_vertices_f[i + 2] = 3.0f;
		m_patch_vertices_f[i + 3] = 4.0f;

		m_patch_vertices_i[i]	 = 1;
		m_patch_vertices_i[i + 1] = 2;
		m_patch_vertices_i[i + 2] = 3;
		m_patch_vertices_i[i + 3] = 4;
	}

	/* Vec4 vertex attribute array setup. */
	gl.genBuffers(1, &m_patch_vertices_bo_f_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenBuffers() failed!");

	gl.bindBuffer(GL_ARRAY_BUFFER, m_patch_vertices_bo_f_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer() failed!");

	gl.bufferData(GL_ARRAY_BUFFER, m_gl_max_patch_vertices * 4 /* components */ * sizeof(glw::GLfloat),
				  m_patch_vertices_f, GL_STATIC_DRAW);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferData() failed!");

	gl.vertexAttribPointer(0,		 /* index */
						   4,		 /* size */
						   GL_FLOAT, /* type */
						   GL_FALSE, /* normalized */
						   0,		 /* stride */
						   0);		 /* pointer */
	GLU_EXPECT_NO_ERROR(gl.getError(), "glVertexAttribPointer(fv) failed!");

	gl.enableVertexAttribArray(0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glEnableVertexAttribArray(fv) failed!");

	gl.bindBuffer(GL_ARRAY_BUFFER, 0);

	/* Ivec4 vertex attribute array setup. */
	gl.genBuffers(1, &m_patch_vertices_bo_i_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenBuffers(ARRAY_BUFFER) failed!");

	gl.bindBuffer(GL_ARRAY_BUFFER, m_patch_vertices_bo_i_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer(ARRAY_BUFFER) failed!");

	gl.bufferData(GL_ARRAY_BUFFER, m_gl_max_patch_vertices * 4 /* components */ * sizeof(glw::GLint),
				  m_patch_vertices_i, GL_STATIC_DRAW);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferData(ARRAY_BUFFER) failed!");

	gl.vertexAttribIPointer(1,		   /* index */
							4,		   /* size */
							GL_INT, 0, /* stride */
							0);		   /* pointer */
	GLU_EXPECT_NO_ERROR(gl.getError(), "glVertexAttribPointer(iv) failed!");

	gl.enableVertexAttribArray(1);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glEnableVertexAttribArray(iv) failed!");

	gl.bindBuffer(GL_ARRAY_BUFFER, 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer() failed!");
}

/** Initializes buffer objects for the test. */
void TessellationShaderMaxPatchVertices::initTransformFeedbackBufferObjects(void)
{
	/* Retrieve ES entry/state points. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Creating Transform Feedback buffer objects. */
	gl.genBuffers(1, &m_bo_id_f_1);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenBuffers() failed");

	gl.genBuffers(1, &m_bo_id_f_2);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenBuffers() failed");

	gl.genBuffers(1, &m_bo_id_i_1);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenBuffers() failed");

	gl.genBuffers(1, &m_bo_id_i_2);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenBuffers() failed");

	/* Transform feedback buffers for case 1*/
	gl.bindTransformFeedback(GL_TRANSFORM_FEEDBACK, m_tf_id_1);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTransformFeedback() failed!");

	gl.bindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, m_bo_id_f_1);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer() failed!");

	gl.bufferData(GL_TRANSFORM_FEEDBACK_BUFFER,
				  ((sizeof(glw::GLfloat)) * 4 /* components */ * OUTPUT_VERTEX_COUNT /* vertices */), DE_NULL,
				  GL_STATIC_DRAW);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferData() failed");

	gl.bindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, m_bo_id_i_1);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer() failed!");

	gl.bufferData(GL_TRANSFORM_FEEDBACK_BUFFER,
				  ((sizeof(glw::GLint)) * 4 /* components */ * OUTPUT_VERTEX_COUNT /* vertices */), DE_NULL,
				  GL_STATIC_DRAW);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferData() failed");

	gl.bindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTransformFeedback(0) failed");

	if (!glu::isContextTypeES(m_context.getRenderContext().getType()))
	{
		/* Transform feedback buffers for case 2*/
		gl.bindTransformFeedback(GL_TRANSFORM_FEEDBACK, m_tf_id_2);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTransformFeedback() failed!");

		gl.bindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, m_bo_id_f_2);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer() failed!");

		gl.bufferData(GL_TRANSFORM_FEEDBACK_BUFFER,
					  ((sizeof(glw::GLfloat)) * 4 /* components */ * OUTPUT_VERTEX_COUNT /* vertices */), DE_NULL,
					  GL_STATIC_DRAW);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferData() failed");

		gl.bindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, m_bo_id_i_2);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer() failed!");

		gl.bufferData(GL_TRANSFORM_FEEDBACK_BUFFER,
					  ((sizeof(glw::GLint)) * 4 /* components */ * OUTPUT_VERTEX_COUNT /* vertices */), DE_NULL,
					  GL_STATIC_DRAW);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferData() failed");

		gl.bindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, 0);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTransformFeedback(0) failed");

		gl.transformFeedbackVaryings(m_po_id_2, 2 /* count */, m_tf_varyings, GL_SEPARATE_ATTRIBS);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glTransformFeedbackVaryings() failed!");
	}
}

/** Initializes program objects for the test. */
void TessellationShaderMaxPatchVertices::initProgramObjects(void)
{
	/* Retrieve ES entry/state points. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Create and configure shader objects. */
	m_po_id_1 = gl.createProgram();
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateProgram() failed!");

	m_po_id_2 = gl.createProgram();
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateProgram() failed!");

	/* Creating Transform Feedback objects. */
	gl.genTransformFeedbacks(1, &m_tf_id_1);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenTransformFeedbacks() failed!");

	gl.genTransformFeedbacks(1, &m_tf_id_2);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenTransformFeedbacks() failed!");

	/* Create and configure shader objects. */
	m_vs_id = gl.createShader(GL_VERTEX_SHADER);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateShader(VERTEX_SHADER) failed!");

	m_tc_id = gl.createShader(m_glExtTokens.TESS_CONTROL_SHADER);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateShader() failed!");

	m_te_id = gl.createShader(m_glExtTokens.TESS_EVALUATION_SHADER);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateShader() failed!");

	m_fs_id = gl.createShader(GL_FRAGMENT_SHADER);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateShader() failed!");

	/* Transform Feedback setup case 1 (explicit arrays).*/
	gl.bindTransformFeedback(GL_TRANSFORM_FEEDBACK, m_tf_id_1);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTransformFeedback() failed!");

	gl.transformFeedbackVaryings(m_po_id_1, 2 /* count */, m_tf_varyings, GL_SEPARATE_ATTRIBS);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTransformFeedbackVaryings() failed!");

	/* Build the program object case 1 (explicit arrays).*/
	if (!buildProgram(m_po_id_1, m_vs_id, 1, &m_vs_code, m_tc_id, 1, &m_tc_code, m_te_id, 1, &m_te_code, m_fs_id, 1,
					  &m_fs_code))
	{
		TCU_FAIL("Program linking failed!");
	}

	if (!glu::isContextTypeES(m_context.getRenderContext().getType()))
	{
		/* Transform Feedback setup case 2 (implicit arrays).*/
		gl.bindTransformFeedback(GL_TRANSFORM_FEEDBACK, m_tf_id_2);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTransformFeedback() failed!");

		gl.transformFeedbackVaryings(m_po_id_2, 2 /* count */, m_tf_varyings, GL_SEPARATE_ATTRIBS);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glTransformFeedbackVaryings() failed!");

		/* Build the program object case 2 (implicit arrays). */
		if (!buildProgram(m_po_id_2, m_vs_id, 1, &m_vs_code, m_te_id, 1, &m_te_code, m_fs_id, 1, &m_fs_code))
		{
			TCU_FAIL("Program linking failed!");
		}
	}
}

/** Initializes reference values for the test. */
void TessellationShaderMaxPatchVertices::initReferenceValues(void)
{
	/* Reference values setup. */
	m_ref_fv_case_1[0] = ((glw::GLfloat)(m_gl_max_patch_vertices * 2) * 1.0f);
	m_ref_fv_case_1[1] = ((glw::GLfloat)(m_gl_max_patch_vertices * 2) * 2.0f);
	m_ref_fv_case_1[2] = ((glw::GLfloat)(m_gl_max_patch_vertices * 2) * 3.0f);
	m_ref_fv_case_1[3] = ((glw::GLfloat)(m_gl_max_patch_vertices * 2) * 4.0f);

	m_ref_iv_case_1[0] = (m_gl_max_patch_vertices * 2 * 1);
	m_ref_iv_case_1[1] = (m_gl_max_patch_vertices * 2 * 2);
	m_ref_iv_case_1[2] = (m_gl_max_patch_vertices * 2 * 3);
	m_ref_iv_case_1[3] = (m_gl_max_patch_vertices * 2 * 4);

	if (!glu::isContextTypeES(m_context.getRenderContext().getType()))
	{
		m_ref_fv_case_2[0] = ((glw::GLfloat)m_gl_max_patch_vertices * 1.0f);
		m_ref_fv_case_2[1] = ((glw::GLfloat)m_gl_max_patch_vertices * 2.0f);
		m_ref_fv_case_2[2] = ((glw::GLfloat)m_gl_max_patch_vertices * 3.0f);
		m_ref_fv_case_2[3] = ((glw::GLfloat)m_gl_max_patch_vertices * 4.0f);

		m_ref_iv_case_2[0] = (m_gl_max_patch_vertices * 1);
		m_ref_iv_case_2[1] = (m_gl_max_patch_vertices * 2);
		m_ref_iv_case_2[2] = (m_gl_max_patch_vertices * 3);
		m_ref_iv_case_2[3] = (m_gl_max_patch_vertices * 4);
	}
}

/** Compares values of vec4 results with the reference data.
 *
 *  Sets the test result to QP_TEST_RESULT_FAIL if the test failed.
 *
 *  Note the function throws exception should an error occur!
 *
 *  @param description Test case's description.
 *  @param ref_fv      Reference value.
 *
 *  @return true       if test passed;
 *          false      if test failed.
 **/
bool TessellationShaderMaxPatchVertices::compareResults(const char* description, glw::GLfloat ref_fv[4])
{
	/* Retrieve ES entry/state points. */
	const glw::Functions& gl		   = m_context.getRenderContext().getFunctions();
	const glw::GLfloat*   resultFloats = DE_NULL;

	resultFloats = (const glw::GLfloat*)gl.mapBufferRange(
		GL_TRANSFORM_FEEDBACK_BUFFER, 0, /* offset */
		(sizeof(glw::GLfloat) /* GLfloat size */ * 4 /* components */ * OUTPUT_VERTEX_COUNT), GL_MAP_READ_BIT);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glMapBufferRange(floats) failed");

	/* Comparison of vec4. */
	const glw::GLfloat epsilon	 = (glw::GLfloat)1e-5f;
	bool			   test_failed = false;

	for (int i = 0; i < 4 /* components */; i++)
	{
		if (de::abs(resultFloats[i] - ref_fv[i]) > epsilon)
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");

			m_testCtx.getLog() << tcu::TestLog::Message << description << " captured results "
							   << "vec4(" << resultFloats[0] << ", " << resultFloats[1] << ", " << resultFloats[2]
							   << ", " << resultFloats[3] << ") "
							   << "are different from the expected values "
							   << "vec4(" << ref_fv[0] << ", " << ref_fv[1] << ", " << ref_fv[2] << ", " << ref_fv[3]
							   << ")." << tcu::TestLog::EndMessage;

			test_failed = true;
			break;
		}
	}

	gl.unmapBuffer(GL_TRANSFORM_FEEDBACK_BUFFER);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUnmapBuffer() failed!");

	return !test_failed;
}

/** Compares values of ivec4 results with the reference data.
 *
 *  Sets the test result to QP_TEST_RESULT_FAIL if the test failed.
 *
 *  Note the function throws exception should an error occur!
 *
 *  @param description   Test case's description.
 *  @param ref_iv        Reference value.
 *
 *  @return true         if test passed.
 *          false        if test failed.
 **/
bool TessellationShaderMaxPatchVertices::compareResults(const char* description, glw::GLint ref_iv[4])
{
	/* Retrieve ES entry/state points. */
	const glw::Functions& gl		 = m_context.getRenderContext().getFunctions();
	const glw::GLint*	 resultInts = DE_NULL;

	resultInts = (const glw::GLint*)gl.mapBufferRange(
		GL_TRANSFORM_FEEDBACK_BUFFER, 0,
		(sizeof(glw::GLint) /* GLfloat size */ * 4 /* components */ * OUTPUT_VERTEX_COUNT), GL_MAP_READ_BIT);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glMapBufferRange(ints) failed");

	bool test_failed = false;

	/* Comparison of ivec4. */
	for (int i = 0; i < 4 /* components */; i++)
	{
		if (resultInts[i] != ref_iv[i])
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");

			m_testCtx.getLog() << tcu::TestLog::Message << description << " captured results ivec4(" << resultInts[0]
							   << ", " << resultInts[1] << ", " << resultInts[2] << ", " << resultInts[3] << ") "
							   << "are different from the expected values ivec4(" << ref_iv[0] << ", " << ref_iv[1]
							   << ", " << ref_iv[2] << ", " << ref_iv[3] << ")." << tcu::TestLog::EndMessage;

			test_failed = true;
			break;
		}
	}

	gl.unmapBuffer(GL_TRANSFORM_FEEDBACK_BUFFER);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUnmapBuffer() failed!");

	return !test_failed;
}

/** Executes the test.
 *
 *  Sets the test result to QP_TEST_RESULT_FAIL if the test failed, QP_TEST_RESULT_PASS otherwise.
 *
 *  Note the function throws exception should an error occur!
 *
 *  @return STOP if the test has finished, CONTINUE to indicate iterate should be called once again.
 **/
tcu::TestNode::IterateResult TessellationShaderMaxPatchVertices::iterate(void)
{
	/* Skip if required extensions are not supported. */
	if (!m_is_tessellation_shader_supported)
	{
		throw tcu::NotSupportedError(TESSELLATION_SHADER_EXTENSION_NOT_SUPPORTED);
	}

	/* Retrieve ES entry/state points. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Initialization */
	initTest();

	/* Render setup case 1. */
	gl.useProgram(m_po_id_1);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram() failed!");

	gl.enable(GL_RASTERIZER_DISCARD);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glEnable(RASTERIZER_DISCARD) failed!");

	gl.bindTransformFeedback(GL_TRANSFORM_FEEDBACK, m_tf_id_1);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTransformFeedback() failed!");

	gl.bindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, m_bo_id_f_1);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBufferRange() failed!");

	gl.bindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 1, m_bo_id_i_1);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBufferRange() failed!");

	/* Rendering case 1 with transform feedback.*/
	gl.beginTransformFeedback(GL_POINTS);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBeginTransformFeedback() failed");

	gl.drawArrays(m_glExtTokens.PATCHES, 0, m_gl_max_patch_vertices);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawArrays() failed");

	gl.endTransformFeedback();
	GLU_EXPECT_NO_ERROR(gl.getError(), "glEndTransformFeedback() failed");

	/* Compare vec4 results from transform feedback for case 1. */
	bool test_passed = true;

	gl.bindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, m_bo_id_f_1);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer() failed!");

	test_passed &= compareResults("Case 1 (explicit arrays)", m_ref_fv_case_1);

	/* Compare ivec4 results from transform feedback for case 2. */
	gl.bindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, m_bo_id_i_1);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer() failed!");

	test_passed &= compareResults("Case 1 (explicit arrays)", m_ref_iv_case_1);

	// Case 2 tests with no TCS (default TCS is expected to be used).
	// Since this is not allowed by ES 3.1, just skip it.
	// Leaving the code for Desktop
	if (!glu::isContextTypeES(m_context.getRenderContext().getType()))
	{
		/* Set up rendering for case 2. */
		gl.useProgram(m_po_id_2);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram() failed!");

		gl.bindTransformFeedback(GL_TRANSFORM_FEEDBACK, m_tf_id_2);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTransformFeedback() failed!");

		gl.bindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0 /* index */, m_bo_id_f_2);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBufferRange() failed!");

		gl.bindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 1 /* index */, m_bo_id_i_2);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBufferRange() failed!");

		/* Tessellation Control Levels setup. */
		const glw::GLfloat inner[] = { 1.0, 1.0 };
		const glw::GLfloat outer[] = { 1.0, 1.0, 1.0, 1.0 };

		gl.patchParameterfv(GL_PATCH_DEFAULT_OUTER_LEVEL, outer);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glPatchParameterfv() failed!");

		gl.patchParameterfv(GL_PATCH_DEFAULT_INNER_LEVEL, inner);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glPatchParameterfv() failed!");

		/* Rendering case 2 with transform feedback. */
		gl.beginTransformFeedback(GL_POINTS);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBeginTransformFeedback(GL_POINTS) failed");

		gl.drawArrays(GL_PATCHES, 0, m_gl_max_patch_vertices);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawArrays() failed");

		gl.endTransformFeedback();
		GLU_EXPECT_NO_ERROR(gl.getError(), "glEndTransformFeedback() failed");

		/* Compare vec4 results from transform feedback for case 2. */
		gl.bindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, m_bo_id_f_2);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer() failed!");

		test_passed &= compareResults("Case 2 (implicit arrays)", m_ref_fv_case_2);

		/* Compare ivec4 results from transform feedback for case 2. */
		gl.bindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, m_bo_id_i_2);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer() failed!");

		test_passed &= compareResults("Case 2 (implicit arrays)", m_ref_iv_case_2);
	}
	/* All done */
	if (test_passed)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}

	return STOP;
}

} /* namespace glcts */
