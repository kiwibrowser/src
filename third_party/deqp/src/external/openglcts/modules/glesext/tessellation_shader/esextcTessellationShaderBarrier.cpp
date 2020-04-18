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

#include "esextcTessellationShaderBarrier.hpp"
#include "gluContextInfo.hpp"
#include "gluDefs.hpp"
#include "glwEnums.hpp"
#include "glwFunctions.hpp"
#include "tcuTestLog.hpp"

namespace glcts
{

/** Constructor
 *
 * @param context       Test context
 * @param name          Test case's name
 * @param description   Test case's description
 **/
TessellationShaderBarrierTests::TessellationShaderBarrierTests(Context& context, const ExtParameters& extParams)
	: TestCaseGroupBase(context, extParams, "tessellation_shader_tc_barriers",
						"Verifies memory barrier work correctly when used in"
						"tessellation control shaders")
{
	/* Left blank on purpose */
}

/* Instantiates all tests and adds them as children to the node */
void TessellationShaderBarrierTests::init(void)
{
	addChild(new glcts::TessellationShaderBarrier1(m_context, m_extParams));
	addChild(new glcts::TessellationShaderBarrier2(m_context, m_extParams));
	addChild(new glcts::TessellationShaderBarrier3(m_context, m_extParams));
}

/** Constructor
 *
 * @param context       Test context
 * @param name          Test case's name
 * @param description   Test case's desricption
 **/
TessellationShaderBarrierTestCase::TessellationShaderBarrierTestCase(Context& context, const ExtParameters& extParams,
																	 const char* name, const char* description)
	: TestCaseBase(context, extParams, name, description)
	, m_bo_id(0)
	, m_fs_id(0)
	, m_po_id(0)
	, m_tcs_id(0)
	, m_tes_id(0)
	, m_vao_id(0)
	, m_vs_id(0)
{
	/* Left blank on purpose */
}

/** Deinitializes all ES objects created for the test. */
void TessellationShaderBarrierTestCase::deinit()
{
	/** Call base class' deinit() function */
	TestCaseBase::deinit();

	if (!m_is_tessellation_shader_supported)
	{
		return;
	}

	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Disable GL_RASTERIZER_DISCARD mode the test has enabled */
	gl.disable(GL_RASTERIZER_DISCARD);

	/* Bring back the original GL_PATCH_VERTICES_EXT setting */
	gl.patchParameteri(m_glExtTokens.PATCH_VERTICES, 3);

	/* Revert buffer object bindings */
	gl.bindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, 0 /* buffer */);
	gl.bindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0 /* index */, 0 /* buffer */);

	/* Unbind vertex array object */
	gl.bindVertexArray(0);

	/* Free all the objects that might have been initialized */
	if (m_bo_id != 0)
	{
		gl.deleteBuffers(1, &m_bo_id);

		m_bo_id = 0;
	}

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

	if (m_tcs_id != 0)
	{
		gl.deleteShader(m_tcs_id);

		m_tcs_id = 0;
	}

	if (m_tes_id != 0)
	{
		gl.deleteShader(m_tes_id);

		m_tes_id = 0;
	}

	if (m_vs_id != 0)
	{
		gl.deleteShader(m_vs_id);

		m_vs_id = 0;
	}

	if (m_vao_id != 0)
	{
		gl.deleteVertexArrays(1, &m_vao_id);

		m_vao_id = 0;
	}
}

/** Initializes all ES objects that will be used for the test. */
void TessellationShaderBarrierTestCase::initTest()
{
	/* The test requires EXT_tessellation_shader */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (!m_is_tessellation_shader_supported)
	{
		return;
	}

	gl.genVertexArrays(1, &m_vao_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not generate vertex array object");

	gl.bindVertexArray(m_vao_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error binding vertex array object!");

	/* Set up storage for XFB data. Also configure the BO binding points */
	const unsigned int xfb_data_size = getXFBBufferSize();

	gl.genBuffers(1, &m_bo_id);
	gl.bindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, m_bo_id);
	gl.bindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0 /* index */, m_bo_id);
	gl.bufferData(GL_TRANSFORM_FEEDBACK_BUFFER, xfb_data_size, NULL /* data */, GL_STATIC_DRAW);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not set up storage for XFB data");

	/* Set up shader objects */
	m_fs_id  = gl.createShader(GL_FRAGMENT_SHADER);
	m_tcs_id = gl.createShader(m_glExtTokens.TESS_CONTROL_SHADER);
	m_tes_id = gl.createShader(m_glExtTokens.TESS_EVALUATION_SHADER);
	m_vs_id  = gl.createShader(GL_VERTEX_SHADER);

	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not create shader objects");

	/* Set up fragment shader body */
	const char* fs_body = "${VERSION}\n"
						  "\n"
						  "void main()\n"
						  "{\n"
						  "}\n";

	/* Set up tessellation control shader body */
	const char* tcs_body = getTCSCode();

	/* Set up tessellation evaluation shader body */
	const char* tes_body = getTESCode();

	/* Set up vertex shader body */
	const char* vs_body = getVSCode();

	/* Set up a program object */
	m_po_id = gl.createProgram();

	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateProgram() failed");

	/* Set up XFB */
	const char** names   = NULL;
	int			 n_names = 0;

	getXFBProperties(&n_names, &names);

	gl.transformFeedbackVaryings(m_po_id, n_names, names, GL_INTERLEAVED_ATTRIBS);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTransformFeedbackVaryings() failed");

	/* Try to compile and link all the shader objects */
	if (!buildProgram(m_po_id, m_fs_id, 1, &fs_body, m_tcs_id, 1, &tcs_body, m_tes_id, 1, &tes_body, m_vs_id, 1,
					  &vs_body))
	{
		TCU_FAIL("Program linking failed");
	}

	/* All set! */
}

/** Executes the test.
 *
 *  Sets the test result to QP_TEST_RESULT_FAIL if the test failed, QP_TEST_RESULT_PASS otherwise.
 *
 *  Note the function throws exception should an error occur!
 *
 *  @return STOP if the test has finished, CONTINUE to indicate iterate() should be called once again.
 **/
tcu::TestNode::IterateResult TessellationShaderBarrierTestCase::iterate(void)
{
	initTest();

	/* Do not execute if required extensions are not supported. */
	if (!m_is_tessellation_shader_supported)
	{
		throw tcu::NotSupportedError(TESSELLATION_SHADER_EXTENSION_NOT_SUPPORTED);
	}

	/* Prepare for the draw call */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	glw::GLint  drawcall_count		 = 0;
	glw::GLenum drawcall_mode		 = GL_NONE;
	glw::GLint  drawcall_n_instances = 1;
	glw::GLint  n_patch_vertices	 = 0;
	glw::GLenum tf_mode				 = GL_NONE;

	getDrawCallArgs(&drawcall_mode, &drawcall_count, &tf_mode, &n_patch_vertices, &drawcall_n_instances);

	gl.enable(GL_RASTERIZER_DISCARD);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glEnable(GL_RASTERIZER_DISCARD) failed");

	gl.patchParameteri(m_glExtTokens.PATCH_VERTICES, n_patch_vertices);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glPatchParameteriEXT() failed");

	gl.useProgram(m_po_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram() failed");

	gl.beginTransformFeedback(tf_mode);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBeginTransformFeedback() failed");
	{
		if (drawcall_n_instances == 1)
		{
			gl.drawArrays(drawcall_mode, 0 /* first */, drawcall_count);
		}
		else
		{
			DE_ASSERT(drawcall_n_instances > 1);

			gl.drawArraysInstanced(drawcall_mode, 0 /* first */, drawcall_count, drawcall_n_instances);
		}
		GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawArrays() or glDrawArraysInstanced() failed");
	}
	gl.endTransformFeedback();
	GLU_EXPECT_NO_ERROR(gl.getError(), "glEndTransformFeedback() failed");

	/* Retrieve the data generated by XFB */
	int			bo_size  = getXFBBufferSize();
	const void* xfb_data = NULL;

	xfb_data = gl.mapBufferRange(GL_TRANSFORM_FEEDBACK_BUFFER, 0 /* offset */, bo_size, GL_MAP_READ_BIT);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glMapBufferRange() failed");

	/* Verify the data */
	bool is_xfb_data_valid = verifyXFBBuffer(xfb_data);

	/* Unmap the buffer object */
	gl.unmapBuffer(GL_TRANSFORM_FEEDBACK_BUFFER);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glUnmapBuffer() failed");

	/* Set the test result, depending on the verification outcome */
	if (is_xfb_data_valid)
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
 * @param context Test context
 **/
TessellationShaderBarrier1::TessellationShaderBarrier1(Context& context, const ExtParameters& extParams)
	: TessellationShaderBarrierTestCase(context, extParams, "barrier_guarded_read_calls",
										"Verifies invocation A can correctly read a per-vertex and"
										" per-patch attribute modified by invocation B after a barrier() call")
	, m_n_result_vertices(2048)
{
	m_n_input_vertices = (m_n_result_vertices / 2 /* result points per isoline */) * 4 /* vertices per patch */;
}

/** Retrieves arguments to be used for the rendering process.
 *
 *  @param out_mode             Deref will be used to store draw call mode. Must not be NULL.
 *  @param out_count            Deref will be used to store count argument to be used for the draw call.
 *                              Must not be NULL.
 *  @param out_tf_mode          Deref will be used to store transform feed-back mode to be used for
 *                              glBeginTransformFeedback() call, prior to issuing the draw call. Must
 *                              not be NULL.
 *  @param out_n_patch_vertices Deref will be used to store GL_PATCH_VERTICES_EXT pname value, to be set with
 *                              glPatchParameteriEXT() call prior to issuing the draw call. Must not be NULL.
 *  @param out_n_instances      Deref will be used to store amount of instances to use for the draw call.
 *                              Using a value of 1 will result in a glDrawArrays() call. Using values larger
 *                              than 1 will trigger glDrawArraysInstanced() call. Values smaller than 1 are
 *                              forbidden.
 **/
void TessellationShaderBarrier1::getDrawCallArgs(glw::GLenum* out_mode, glw::GLint* out_count, glw::GLenum* out_tf_mode,
												 glw::GLint* out_n_patch_vertices, glw::GLint* out_n_instances)
{
	*out_count			  = m_n_input_vertices;
	*out_mode			  = GL_PATCHES_EXT;
	*out_n_instances	  = 1;
	*out_n_patch_vertices = 4;
	*out_tf_mode		  = GL_POINTS;
}

/** Retrieves tessellation control shader body.
 *
 *  @return TC stage shader body.
 **/
const char* TessellationShaderBarrier1::getTCSCode()
{
	static const char* tcs_code =
		"${VERSION}\n"
		"\n"
		"${TESSELLATION_SHADER_REQUIRE}\n"
		"\n"
		"layout (vertices = 4) out;\n"
		"\n"
		"flat  in  int   vertex_id[];\n"
		"\n"
		"patch out int   test_patch_value;\n"
		"      out ivec4 test_vector [];\n"
		"      out ivec4 test_vector2[];\n"
		"\n"
		"void main()\n"
		"{\n"
		"    if (gl_InvocationID == 0)\n"
		"    {\n"
		"        test_patch_value = vertex_id[0];\n"
		"    }\n"
		"\n"
		"    test_vector[gl_InvocationID] = ivec4(vertex_id[gl_InvocationID],\n"
		"                                         vertex_id[gl_InvocationID] + 1,\n"
		"                                         vertex_id[gl_InvocationID] + 2,\n"
		"                                         vertex_id[gl_InvocationID] + 3);\n"
		"\n"
		"    barrier();\n"
		"\n"
		"    int next_invocation = (gl_InvocationID + 1) % 4;\n"
		"\n"
		"    test_vector2[gl_InvocationID] = ivec4(test_vector[next_invocation].xyz, test_patch_value);\n"
		"\n"
		"    gl_TessLevelOuter[0] = 1.0;\n"
		"    gl_TessLevelOuter[1] = 1.0;\n"
		"}\n";

	return tcs_code;
}

/** Retrieves tessellation evaluation shader body.
 *
 *  @return TE stage shader body.
 **/
const char* TessellationShaderBarrier1::getTESCode()
{
	static const char* tes_code = "${VERSION}\n"
								  "\n"
								  "${TESSELLATION_SHADER_REQUIRE}\n"
								  "\n"
								  "layout(isolines, point_mode) in;\n"
								  "\n"
								  "in ivec4 test_vector2[];\n"
								  "\n"
								  "out ivec4 result_vector2;\n"
								  "\n"
								  "void main()\n"
								  "{\n"
								  "    int base = gl_PrimitiveID * 4;\n"
								  "\n"
								  "    if (test_vector2[0].x == (base + 1) && test_vector2[0].y == (base + 2) && "
								  "test_vector2[0].z == (base + 3) && test_vector2[0].w == (base + 0)  &&\n"
								  "        test_vector2[1].x == (base + 2) && test_vector2[1].y == (base + 3) && "
								  "test_vector2[1].z == (base + 4) && test_vector2[1].w == (base + 0)  &&\n"
								  "        test_vector2[2].x == (base + 3) && test_vector2[2].y == (base + 4) && "
								  "test_vector2[2].z == (base + 5) && test_vector2[2].w == (base + 0)  &&\n"
								  "        test_vector2[3].x == (base + 0) && test_vector2[3].y == (base + 1) && "
								  "test_vector2[3].z == (base + 2) && test_vector2[3].w == (base + 0) )\n"
								  "    {\n"
								  "        result_vector2 = ivec4(1);\n"
								  "    }\n"
								  "    else\n"
								  "    {\n"
								  "        result_vector2 = ivec4(0);\n"
								  "    }\n"
								  "}\n";

	return tes_code;
}

/** Retrieves vertex shader body.
 *
 *  @return Vertex shader body.
 **/
const char* TessellationShaderBarrier1::getVSCode()
{
	static const char* vs_code = "${VERSION}\n"
								 "\n"
								 "flat out int vertex_id;\n"
								 "\n"
								 "void main()\n"
								 "{\n"
								 "    vertex_id = gl_VertexID;\n"
								 "}\n";

	return vs_code;
}

/** Retrieves amount of bytes that should be used for allocating storage space for
 *  a buffer object that will later be used to hold XFB result data.
 *
 *  @return Amount of bytes required by the test.
 */
int TessellationShaderBarrier1::getXFBBufferSize()
{
	return static_cast<int>(m_n_result_vertices * sizeof(int) * 4 /* components */ * 2 /* points per isoline */);
}

/** Retrieves names of transform feedback varyings and amount of those. These should be used
 *  prior to link the test program object.
 *
 *  @param out_n_names Deref will be used to store the number of names that @param out_names array
 *                     holds. Must not be NULL.
 *  @param out_names   Deref will be used to store a pointer to an array holding TF varying names;
 *                     Must not be NULL.
 **/
void TessellationShaderBarrier1::getXFBProperties(int* out_n_names, const char*** out_names)
{
	static const char* names[1] = { "result_vector2" };

	*out_n_names = 1;
	*out_names   = names;
}

/** Verifies data captured by XFB is correct.
 *
 *  @param data Buffer holding the result XFB data. Must not be NULL.
 *
 *  @return true if the result data is confirmed to be valid, false otherwise.
 **/
bool TessellationShaderBarrier1::verifyXFBBuffer(const void* data)
{
	int* data_int = (int*)data;

	/* Run through all vertices */
	for (unsigned int n_vertex = 0; n_vertex < m_n_result_vertices; ++n_vertex)
	{
		int retrieved_x = data_int[n_vertex * 4];
		int retrieved_y = data_int[n_vertex * 4 + 1];
		int retrieved_z = data_int[n_vertex * 4 + 2];
		int retrieved_w = data_int[n_vertex * 4 + 3];

		if (retrieved_x != 1 || retrieved_y != 1 || retrieved_z != 1 || retrieved_w != 1)
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "Invalid value returned: expected:[1, 1, 1, 1]"
							   << " retrieved: "
							   << "[" << retrieved_x << ", " << retrieved_y << ", " << retrieved_z << ", "
							   << retrieved_w << tcu::TestLog::EndMessage;

			TCU_FAIL("Invalid rendering result");
		}
	}

	return true;
}

/** Constructor
 *
 * @param context Test context
 **/
TessellationShaderBarrier2::TessellationShaderBarrier2(Context& context, const ExtParameters& extParams)
	: TessellationShaderBarrierTestCase(context, extParams, "barrier_guarded_write_calls",
										"Verifies it is safe to write to the same per-patch output "
										"from multiple TC invocations, as long as each write happens "
										"in a separate phase.")
	, m_n_result_vertices(2048)
{
	m_n_input_vertices = (m_n_result_vertices / 2 /* result points per isoline */) * 4 /* vertices per patch */;
}

/** Retrieves arguments to be used for the rendering process.
 *
 *  @param out_mode             Deref will be used to store draw call mode. Must not be NULL.
 *  @param out_count            Deref will be used to store count argument to be used for the draw call.
 *                              Must not be NULL.
 *  @param out_tf_mode          Deref will be used to store transform feed-back mode to be used for
 *                              glBeginTransformFeedback() call, prior to issuing the draw call. Must
 *                              not be NULL.
 *  @param out_n_patch_vertices Deref will be used to store GL_PATCH_VERTICES_EXT pname value, to be set with
 *                              glPatchParameteriEXT() call prior to issuing the draw call. Must not be NULL.
 *  @param out_n_instances      Deref will be used to store amount of instances to use for the draw call.
 *                              Using a value of 1 will result in a glDrawArrays() call. Using values larger
 *                              than 1 will trigger glDrawArraysInstanced() call. Values smaller than 1 are
 *                              forbidden.
 **/
void TessellationShaderBarrier2::getDrawCallArgs(glw::GLenum* out_mode, glw::GLint* out_count, glw::GLenum* out_tf_mode,
												 glw::GLint* out_n_patch_vertices, glw::GLint* out_n_instances)
{
	*out_count			  = m_n_input_vertices;
	*out_mode			  = GL_PATCHES_EXT;
	*out_n_instances	  = 1;
	*out_n_patch_vertices = 4;
	*out_tf_mode		  = GL_POINTS;
}

/** Retrieves tessellation control shader body.
 *
 *  @return TC stage shader body.
 **/
const char* TessellationShaderBarrier2::getTCSCode()
{
	static const char* tcs_code = "${VERSION}\n"
								  "\n"
								  "${TESSELLATION_SHADER_REQUIRE}\n"
								  "\n"
								  "layout (vertices = 4) out;\n"
								  "\n"
								  "      out float tcs_result[];\n"
								  "patch out int   test_patch_value;\n"
								  "\n"
								  "void main()\n"
								  "{\n"
								  /* Four invocations per input patch will be executed. Have the first one write its
		 * secret value to the output per-patch attribute.
		 */
								  "    if (gl_InvocationID == 0)\n"
								  "    {\n"
								  "        test_patch_value = 123;\n"
								  "    }\n"
								  "\n"
								  "    barrier();\n"
								  "\n"
								  /* Let's have the second invocation update the value at this point */
								  "    if (gl_InvocationID == 1)\n"
								  "    {\n"
								  "        test_patch_value = 234;\n"
								  "    }\n"
								  "\n"
								  "    barrier();\n"
								  "\n"
								  /* Finally, if this is invocation number one, check if test_patch_value
		 * stores a correct value.
		 */
								  "    tcs_result[gl_InvocationID] = 2.0;\n"
								  "\n"
								  "    if (gl_InvocationID == 0)\n"
								  "    {\n"
								  "        if (test_patch_value == 234)\n"
								  "        {\n"
								  "            tcs_result[gl_InvocationID] = 1.0;\n"
								  "        }\n"
								  "    }\n"
								  "\n"
								  "    gl_TessLevelOuter[0] = 1.0;\n"
								  "    gl_TessLevelOuter[1] = 1.0;\n"
								  "}\n";

	return tcs_code;
}

/** Retrieves tessellation evaluation shader body.
 *
 *  @return TE stage shader body.
 **/
const char* TessellationShaderBarrier2::getTESCode()
{
	static const char* tes_code = "${VERSION}\n"
								  "\n"
								  "${TESSELLATION_SHADER_REQUIRE}\n"
								  "\n"
								  "layout(isolines, point_mode) in;\n"
								  "\n"
								  "      in float tcs_result[];\n"
								  "\n"
								  "out float tes_result;\n"
								  "\n"
								  "void main()\n"
								  "{\n"
								  /* TCS invocation order is undefined so take a maximum of all values we received */
								  "    if (tcs_result[0] == 1.0 &&\n"
								  "        tcs_result[1] == 2.0 &&\n"
								  "        tcs_result[2] == 2.0 &&\n"
								  "        tcs_result[3] == 2.0)\n"
								  "    {\n"
								  "        tes_result = 1.0;\n"
								  "    }\n"
								  "    else\n"
								  "    {\n"
								  "        tes_result = 0.0;\n"
								  "    }\n"
								  "}\n";

	return tes_code;
}

/** Retrieves vertex shader body.
 *
 *  @return Vertex shader body.
 **/
const char* TessellationShaderBarrier2::getVSCode()
{
	static const char* vs_code = "${VERSION}\n"
								 "\n"
								 "void main()\n"
								 "{\n"
								 "}\n";

	return vs_code;
}

/** Retrieves amount of bytes that should be used for allocating storage space for
 *  a buffer object that will later be used to hold XFB result data.
 *
 *  @return Amount of bytes required by the test.
 */
int TessellationShaderBarrier2::getXFBBufferSize()
{
	return static_cast<int>(m_n_result_vertices * sizeof(float) * 2 /* points per isoline */);
}

/** Retrieves names of transform feedback varyings and amount of those. These should be used
 *  prior to link the test program object.
 *
 *  @param out_n_names Deref will be used to store the number of names that @param out_names array
 *                     holds. Must not be NULL.
 *  @param out_names   Deref will be used to store a pointer to an array holding TF varying names;
 *                     Must not be NULL.
 **/
void TessellationShaderBarrier2::getXFBProperties(int* out_n_names, const char*** out_names)
{
	static const char* names[1] = { "tes_result" };

	*out_n_names = 1;
	*out_names   = names;
}

/** Verifies data captured by XFB is correct.
 *
 *  @param data Buffer holding the result XFB data. Must not be NULL.
 *
 *  @return true if the result data is confirmed to be valid, false otherwise.
 **/
bool TessellationShaderBarrier2::verifyXFBBuffer(const void* data)
{
	float*		data_float = (float*)data;
	const float epsilon	= (float)1e-5;

	/* Run through all vertices */
	for (unsigned int n_vertex = 0; n_vertex < m_n_result_vertices; ++n_vertex)
	{
		if (de::abs(data_float[n_vertex] - 1.0f /* valid result value */) > epsilon)
		{
			TCU_FAIL("Invalid data retrieved");
		}
	}

	return true;
}

/** Constructor
 *
 * @param context Test context
 **/
TessellationShaderBarrier3::TessellationShaderBarrier3(Context& context, const ExtParameters& extParams)
	: TessellationShaderBarrierTestCase(context, extParams, "barrier_guarded_read_write_calls",
										"Verifies it is safe to write to the same per-patch output "
										"from multiple TC invocations, as long as each write happens "
										"in a separate phase.")
	, m_n_instances(10)				/* as per test spec */
	, m_n_invocations(16)			/* as per test spec */
	, m_n_patch_vertices(8)			/* as per test spec */
	, m_n_patches_per_invocation(2) /* as per test spec */
	, m_n_result_vertices(2 /* result points per isoline */ * m_n_patches_per_invocation * m_n_invocations *
						  m_n_instances)
{
	m_n_input_vertices = (m_n_result_vertices / 2 /* result points per isoline */) * 4 /* vertices per patch */;
}

/** Retrieves arguments to be used for the rendering process.
 *
 *  @param out_mode             Deref will be used to store draw call mode. Must not be NULL.
 *  @param out_count            Deref will be used to store count argument to be used for the draw call.
 *                              Must not be NULL.
 *  @param out_tf_mode          Deref will be used to store transform feed-back mode to be used for
 *                              glBeginTransformFeedback() call, prior to issuing the draw call. Must
 *                              not be NULL.
 *  @param out_n_patch_vertices Deref will be used to store GL_PATCH_VERTICES_EXT pname value, to be set with
 *                              glPatchParameteriEXT() call prior to issuing the draw call. Must not be NULL.
 *  @param out_n_instances      Deref will be used to store amount of instances to use for the draw call.
 *                              Using a value of 1 will result in a glDrawArrays() call. Using values larger
 *                              than 1 will trigger glDrawArraysInstanced() call. Values smaller than 1 are
 *                              forbidden.
 **/
void TessellationShaderBarrier3::getDrawCallArgs(glw::GLenum* out_mode, glw::GLint* out_count, glw::GLenum* out_tf_mode,
												 glw::GLint* out_n_patch_vertices, glw::GLint* out_n_instances)
{
	*out_count			  = m_n_patches_per_invocation * m_n_patch_vertices * m_n_instances;
	*out_mode			  = GL_PATCHES_EXT;
	*out_n_instances	  = m_n_instances;
	*out_n_patch_vertices = m_n_patch_vertices;
	*out_tf_mode		  = GL_POINTS;
}

/** Retrieves tessellation control shader body.
 *
 *  @return TC stage shader body.
 **/
const char* TessellationShaderBarrier3::getTCSCode()
{
	static const char* tcs_code =
		"${VERSION}\n"
		"\n"
		"${TESSELLATION_SHADER_REQUIRE}\n"
		"\n"
		"layout (vertices = 16) out;\n"
		"\n"
		"      out int tcs_data        [];\n"
		"patch out int tcs_patch_result[16];\n"
		"\n"
		"void main()\n"
		"{\n"
		/* Even invocations should write their gl_InvocationID value to their per-vertex output. */
		"    if ((gl_InvocationID % 2) == 0)\n"
		"    {\n"
		"        tcs_data[gl_InvocationID] = gl_InvocationID;\n"
		"    }\n"
		"\n"
		"    barrier();\n"
		"\n"
		/* Odd invocations should read values stored by preceding even invocation,
		 * add current invocation's ID to that value, and then write it to its per-vertex
		 * output.
		 */
		"    if ((gl_InvocationID % 2) == 1)\n"
		"    {\n"
		"        tcs_data[gl_InvocationID] = tcs_data[gl_InvocationID - 1] + gl_InvocationID;\n"
		"    }\n"
		"\n"
		"    barrier();\n"
		"\n"
		/* Every fourth invocation should now read & sum up per-vertex outputs for four invocations
		 * following it (including the one discussed), and store it in a per-patch variable */
		"    tcs_patch_result[gl_InvocationID] = 0;\n"
		"\n"
		"    if ((gl_InvocationID % 4) == 0)\n"
		"    {\n"
		"        tcs_patch_result[gl_InvocationID] += tcs_data[gl_InvocationID];\n"
		"        tcs_patch_result[gl_InvocationID] += tcs_data[gl_InvocationID+1];\n"
		"        tcs_patch_result[gl_InvocationID] += tcs_data[gl_InvocationID+2];\n"
		"        tcs_patch_result[gl_InvocationID] += tcs_data[gl_InvocationID+3];\n"
		"    }\n"
		"\n"
		"    gl_TessLevelOuter[0] = 1.0;\n"
		"    gl_TessLevelOuter[1] = 1.0;\n"
		"}\n";

	return tcs_code;
}

/** Retrieves tessellation evaluation shader body.
 *
 *  @return TC stage shader body.
 **/
const char* TessellationShaderBarrier3::getTESCode()
{
	static const char* tes_code = "${VERSION}\n"
								  "\n"
								  "${TESSELLATION_SHADER_REQUIRE}\n"
								  "\n"
								  "layout(isolines, point_mode) in;\n"
								  "\n"
								  "patch in int tcs_patch_result[16];\n"
								  "\n"
								  "flat out ivec4 tes_result1;\n"
								  "flat out ivec4 tes_result2;\n"
								  "flat out ivec4 tes_result3;\n"
								  "flat out ivec4 tes_result4;\n"
								  "\n"
								  "void main()\n"
								  "{\n"
								  "    tes_result1 = ivec4(tcs_patch_result[0],  tcs_patch_result[1],  "
								  "tcs_patch_result[2],  tcs_patch_result[3]);\n"
								  "    tes_result2 = ivec4(tcs_patch_result[4],  tcs_patch_result[5],  "
								  "tcs_patch_result[6],  tcs_patch_result[7]);\n"
								  "    tes_result3 = ivec4(tcs_patch_result[8],  tcs_patch_result[9],  "
								  "tcs_patch_result[10], tcs_patch_result[11]);\n"
								  "    tes_result4 = ivec4(tcs_patch_result[12], tcs_patch_result[13], "
								  "tcs_patch_result[14], tcs_patch_result[15]);\n"
								  "}\n";

	return tes_code;
}

/** Retrieves vertex shader body.
 *
 *  @return Vertex shader body.
 **/
const char* TessellationShaderBarrier3::getVSCode()
{
	static const char* vs_code = "${VERSION}\n"
								 "\n"
								 "void main()\n"
								 "{\n"
								 "}\n";

	return vs_code;
}

/** Retrieves amount of bytes that should be used for allocating storage space for
 *  a buffer object that will later be used to hold XFB result data.
 *
 *  @return Amount of bytes required by the test.
 */
int TessellationShaderBarrier3::getXFBBufferSize()
{
	return static_cast<int>(m_n_instances * m_n_result_vertices * sizeof(int) * 4 /* ivec4 */ *
							2 /* points per isoline */);
}

/** Retrieves names of transform feedback varyings and amount of those. These should be used
 *  prior to link the test program object.
 *
 *  @param out_n_names Deref will be used to store the number of names that @param out_names array
 *                     holds. Must not be NULL.
 *  @param out_names   Deref will be used to store a pointer to an array holding TF varying names;
 *                     Must not be NULL.
 **/
void TessellationShaderBarrier3::getXFBProperties(int* out_n_names, const char*** out_names)
{
	static const char* names[] = { "tes_result1", "tes_result2", "tes_result3", "tes_result4" };

	*out_n_names = 4;
	*out_names   = names;
}

/** Verifies data captured by XFB is correct.
 *
 *  @param data Buffer holding the result XFB data. Must not be NULL.
 *
 *  @return true if the result data is confirmed to be valid, false otherwise.
 **/
bool TessellationShaderBarrier3::verifyXFBBuffer(const void* data)
{
	const int*		 data_int = (const int*)data;
	std::vector<int> tcs_data(m_n_invocations, 0);
	std::vector<int> tcs_patch_result(m_n_invocations, 0);

	/* This is a simple C++ port of the TCS used for the test.
	 *
	 * Note: We only need to consider a single set of values stored by TES
	 *       for a single result point, as the same set of values will be
	 *       reported for the other point. Owing to the fact gl_InvocationID
	 *       in TCS will iterate from 0 to 15 for all input patches and instances,
	 *       we can re-use the data for all subsequent input patches. */
	/* Phase 1 */
	for (unsigned int n = 0; n < m_n_invocations; n += 2)
	{
		tcs_data[n] = n;
	}

	/* Phase 2 */
	for (unsigned int n = 1; n < m_n_invocations; n += 2)
	{
		tcs_data[n] = tcs_data[n - 1] + n;
	}

	/* Phase 3 */
	for (unsigned int n_patch_vertex = 0; n_patch_vertex < m_n_invocations; ++n_patch_vertex)
	{
		const unsigned int invocation_id = n_patch_vertex;

		tcs_patch_result[invocation_id] = 0;

		if ((invocation_id % 4) == 0)
		{
			tcs_patch_result[invocation_id] += tcs_data[invocation_id];
			tcs_patch_result[invocation_id] += tcs_data[invocation_id + 1];
			tcs_patch_result[invocation_id] += tcs_data[invocation_id + 2];
			tcs_patch_result[invocation_id] += tcs_data[invocation_id + 3];
		}
	} /* for (all patch vertices) */

	/* Time to do the actual comparison. */
	for (unsigned int n_patch = 0; n_patch < m_n_result_vertices / m_n_invocations; ++n_patch)
	{
		bool	   are_equal				 = true;
		const int  n_points_per_line_segment = 2;
		const int* patch_data_int			 = data_int + n_patch * m_n_invocations * n_points_per_line_segment;

		for (unsigned int n_invocation = 0; n_invocation < m_n_invocations; ++n_invocation)
		{
			if (patch_data_int[n_invocation] != tcs_patch_result[n_invocation])
			{
				are_equal = false;

				break;
			}
		} /* for (all patch vertices which have contributed for given input patch being considered) */

		if (!are_equal)
		{
			std::stringstream logMessage;

			logMessage << "Result data for patch [" << n_patch << "]: (";

			for (unsigned int n_patch_vertex = 0; n_patch_vertex < m_n_patch_vertices; ++n_patch_vertex)
			{
				logMessage << patch_data_int[n_patch_vertex];

				if (n_patch_vertex == (m_n_patch_vertices - 1))
				{
					logMessage << "), ";
				}
				else
				{
					logMessage << ", ";
				}
			} /* for (all patch vertices) */

			logMessage << "expected: ";

			for (unsigned int n_patch_vertex = 0; n_patch_vertex < m_n_patch_vertices; ++n_patch_vertex)
			{
				logMessage << tcs_patch_result[n_patch_vertex];

				if (n_patch_vertex == (m_n_patch_vertices - 1))
				{
					logMessage << "). ";
				}
				else
				{
					logMessage << ", ";
				}
			} /* for (all patch vertices) */

			/* Log the message */
			m_testCtx.getLog() << tcu::TestLog::Message << logMessage.str().c_str() << tcu::TestLog::EndMessage;

			/* Bail out */
			TCU_FAIL("Invalid data captured");
		} /* if (!are_equal) */
	}	 /* for (all patches) */

	return true;
}

} /* namespace glcts */
