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

#include "esextcTessellationShaderTessellation.hpp"
#include "gluContextInfo.hpp"
#include "gluDefs.hpp"
#include "glwEnums.hpp"
#include "glwFunctions.hpp"
#include "tcuTestLog.hpp"
#include <cstdlib>

namespace glcts
{

/** Vertex shader source code for max_in_out_attributes test. */
const char* TessellationShaderTessellationMaxInOut::m_vs_code =
	"${VERSION}\n"
	"\n"
	"${TESSELLATION_SHADER_REQUIRE}\n"
	"${SHADER_IO_BLOCKS_ENABLE}\n"
	"\n"
	"precision highp float;\n"
	"\n"
	"layout(location = 0) in vec4 in_fv;\n"
	"\n"
	"out Vertex\n"
	"{\n"
	"    /* Note: we need to leave some space for gl_Position */\n"
	"    vec4 value[(gl_MaxTessControlInputComponents) / 4 - 1];\n"
	"} outVertex;\n"
	"\n"
	"void main()\n"
	"{\n"
	"    gl_Position = in_fv;\n"
	"\n"
	"    for (int i = 0 ; i < (gl_MaxTessControlInputComponents - "
	"4) / 4 ; i++)\n" /* Max vec4 output attributes - gl_Position */
	"    {\n"
	"        outVertex.value[i] = vec4(float(4*i), float(4*i) + "
	"1.0, float(4*i) + 2.0, float(4*i) + 3.0);\n"
	"    }\n"
	"}\n";

/* Tessellation Control Shader code for max_in_out_attributes test */
const char* TessellationShaderTessellationMaxInOut::m_tcs_code_1 =
	"${VERSION}\n"
	"\n"
	"${TESSELLATION_SHADER_REQUIRE}\n"
	"\n"
	"precision highp float;\n"
	"\n"
	"layout(vertices = 2) out;\n"
	"\n"
	"in Vertex\n"
	"{\n"
	"    /* Note: we need to leave some space for gl_Position */\n"
	"    vec4 value[(gl_MaxTessControlInputComponents - 4) / 4];\n"
	"} inVertex[];\n"
	"\n"
	"out Vertex\n"
	"{\n"
	"    /* Note: we need to leave some space for gl_Position */\n"
	"    vec4 value[(gl_MaxTessControlOutputComponents - 4) / 4];\n"
	"} outVariables[];\n"
	"\n"
	"void main()\n"
	"{\n"
	"    gl_TessLevelInner[0] = 1.0;\n"
	"    gl_TessLevelInner[1] = 1.0;\n"
	"    gl_TessLevelOuter[0] = 1.0;\n"
	"    gl_TessLevelOuter[1] = 1.0;\n"
	"    gl_TessLevelOuter[2] = 1.0;\n"
	"    gl_TessLevelOuter[3] = 1.0;\n"
	"\n"
	"    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;\n"
	"\n"
	"    for (int j = 0; j < (gl_MaxTessControlOutputComponents - 4) / 4; j++)\n"
	"    {\n"
	"        outVariables[gl_InvocationID].value[j] = vec4(float(4*j), float(4*j) + 1.0, float(4*j) + 2.0, float(4*j) "
	"+ 3.0);\n"
	"\n"
	"        for (int i = 0; i < (gl_MaxTessControlInputComponents-4)/4; i++)\n"
	"        {\n"
	"            outVariables[gl_InvocationID].value[j] += inVertex[gl_InvocationID].value[i];\n"
	"        }\n"
	"    }\n"
	"}\n";

/* Tessellation Control Shader code for max_in_out_attributes test */
const char* TessellationShaderTessellationMaxInOut::m_tcs_code_2 =
	"${VERSION}\n"
	"\n"
	"${TESSELLATION_SHADER_REQUIRE}\n"
	"\n"
	"precision highp float;\n"
	"\n"
	"layout(vertices = 2) out;\n"
	"\n"
	"patch out vec4 value[gl_MaxTessPatchComponents/4];\n"
	"\n"
	"void main()\n"
	"{\n"
	"    gl_TessLevelInner[0] = 1.0;\n"
	"    gl_TessLevelInner[1] = 1.0;\n"
	"    gl_TessLevelOuter[0] = 1.0;\n"
	"    gl_TessLevelOuter[1] = 1.0;\n"
	"    gl_TessLevelOuter[2] = 1.0;\n"
	"    gl_TessLevelOuter[3] = 1.0;\n"
	"\n"
	"    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;\n"
	"\n"
	"    for (int j = 0; j < gl_MaxTessPatchComponents / 4; j++)\n"
	"    {\n"
	"        value[j] = vec4(float(4*j), float(4*j) + 1.0, float(4*j) + 2.0, float(4*j) + 3.0);\n"
	"    }\n"
	"}\n";

/* Tessellation Evaluation Shader code for max_in_out_attributes test */
const char* TessellationShaderTessellationMaxInOut::m_tes_code_1 =
	"${VERSION}\n"
	"\n"
	"${TESSELLATION_SHADER_REQUIRE}\n"
	"\n"
	"precision highp float;\n"
	"\n"
	"layout (isolines, point_mode) in;\n"
	"\n"
	"in Vertex\n"
	"{\n"
	"    vec4 value[(gl_MaxTessEvaluationInputComponents - 4) / 4];\n"
	"} inVariables[];\n"
	"\n"
	"out Vertex\n"
	"{\n"
	"    vec4 value[(gl_MaxTessEvaluationOutputComponents - 4) / 4];\n"
	"} outVariables;\n"
	"\n"
	"void main()\n"
	"{\n"
	"    gl_Position = gl_in[0].gl_Position;\n"
	"\n"
	"    for (int j = 0; j < (gl_MaxTessEvaluationOutputComponents - 4) / 4; j++)\n"
	"    {\n"
	"        outVariables.value[j] = vec4(float(4*j), float(4*j) + 1.0, float(4*j) + 2.0, float(4*j) + 3.0);\n"
	"\n"
	"        for (int i = 0 ; i < (gl_MaxTessEvaluationInputComponents - 4) / 4; i++)\n"
	"        {\n"
	"            outVariables.value[j] += inVariables[0].value[i];\n"
	"        }\n"
	"    }\n"
	"}\n";

/* Tessellation Evaluation Shader code for max_in_out_attributes test */
const char* TessellationShaderTessellationMaxInOut::m_tes_code_2 =
	"${VERSION}\n"
	"\n"
	"${TESSELLATION_SHADER_REQUIRE}\n"
	"\n"
	"precision highp float;\n"
	"\n"
	"layout (isolines, point_mode) in;\n"
	"\n"
	"patch in vec4 value[gl_MaxTessPatchComponents / 4];\n"
	"\n"
	"out vec4 out_value;\n"
	"\n"
	"void main()\n"
	"{\n"
	"    gl_Position    = gl_in[0].gl_Position;\n"
	"    out_value = vec4(0.0);\n"
	"\n"
	"    for (int i = 0; i < gl_MaxTessPatchComponents / 4; i++)\n"
	"    {\n"
	"        out_value += value[i];\n"
	"    }\n"
	"}\n";

/* Fragment Shader code for max_in_out_attributes test */
const char* TessellationShaderTessellationMaxInOut::m_fs_code = "${VERSION}\n"
																"\n"
																"void main()\n"
																"{\n"
																"}\n";

/** Constructor
 *
 * @param context Test context
 **/
TessellationShaderTessellationTests::TessellationShaderTessellationTests(glcts::Context&	  context,
																		 const ExtParameters& extParams)
	: TestCaseGroupBase(context, extParams, "tessellation_shader_tessellation",
						"Verifies general tessellation functionality")
{
	/* No implementation needed */
}

/**
 * Initializes test groups for geometry shader tests
 **/
void TessellationShaderTessellationTests::init(void)
{
	addChild(
		new glcts::TessellationShaderTessellationgl_InvocationID_PatchVerticesIn_PrimitiveID(m_context, m_extParams));
	addChild(
		new glcts::TessellationShaderTessellationgl_TessCoord(m_context, m_extParams, TESSELLATION_TEST_TYPE_TCS_TES));
	addChild(new glcts::TessellationShaderTessellationgl_TessCoord(m_context, m_extParams, TESSELLATION_TEST_TYPE_TES));
	addChild(new glcts::TessellationShaderTessellationInputPatchDiscard(m_context, m_extParams));
	addChild(new glcts::TessellationShaderTessellationMaxInOut(m_context, m_extParams));
}

/** Constructor
 *
 * @param context Test context
 **/
TessellationShaderTessellationInputPatchDiscard::TessellationShaderTessellationInputPatchDiscard(
	Context& context, const ExtParameters& extParams)
	: TestCaseBase(context, extParams, "input_patch_discard",
				   "Verifies that patches, for which relevant outer tessellation levels have"
				   " been defined to 0 or less, are discard by the tessellation primitive "
				   " generator.")
	, m_bo_id(0)
	, m_fs_id(0)
	, m_vs_id(0)
	, m_vao_id(0)
	, m_utils_ptr(0)
{
	/* Left blank on purpose */
}

/** Deinitializes ES objects created for the test. */
void TessellationShaderTessellationInputPatchDiscard::deinit()
{
	/* Call base class' deinit() */
	TestCaseBase::deinit();

	if (!m_is_tessellation_shader_supported)
	{
		return;
	}

	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Remove TF buffer object bindings */
	gl.bindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, 0 /* buffer */);
	gl.bindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0 /* index */, 0 /* buffer */);

	/* Disable GL_RASTERIZER_DISCARD mode */
	gl.disable(GL_RASTERIZER_DISCARD);

	/* Restore GL_PATCH_VERTICES_EXT value */
	gl.patchParameteri(m_glExtTokens.PATCH_VERTICES, 3);

	/* Unbind vertex array object */
	gl.bindVertexArray(0);

	/* Free all ES objects we allocated for the test */
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

	/* Deinitialize all test descriptors */
	for (_runs::iterator it = m_runs.begin(); it != m_runs.end(); ++it)
	{
		deinitRun(*it);
	}
	m_runs.clear();

	/* Release tessellation shader test utilities instance */
	if (m_utils_ptr != NULL)
	{
		delete m_utils_ptr;

		m_utils_ptr = NULL;
	}
}

/** Deinitialize all test pass-specific ES objects.
 *
 *  @param test Descriptor of a test pass to deinitialize.
 **/
void TessellationShaderTessellationInputPatchDiscard::deinitRun(_run& run)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (run.po_id != 0)
	{
		gl.deleteProgram(run.po_id);

		run.po_id = 0;
	}

	if (run.tc_id != 0)
	{
		gl.deleteShader(run.tc_id);

		run.tc_id = 0;
	}

	if (run.te_id != 0)
	{
		gl.deleteShader(run.te_id);

		run.te_id = 0;
	}
}

/** Returns source code of a tessellation control shader for the test.
 *
 *  @return Requested string.
 **/
std::string TessellationShaderTessellationInputPatchDiscard::getTCCode()
{
	std::string result;

	result = "${VERSION}\n"
			 "\n"
			 "${TESSELLATION_SHADER_REQUIRE}\n"
			 "\n"
			 "layout(vertices = 2) out;\n"
			 "\n"
			 "out int tc_primitive_id[];\n"
			 "\n"
			 "void main()\n"
			 "{\n"
			 "    if ((gl_PrimitiveID % 4) == 0)\n"
			 "    {\n"
			 "        gl_TessLevelOuter[0] = 0.0;\n"
			 "        gl_TessLevelOuter[1] = 0.0;\n"
			 "        gl_TessLevelOuter[2] = 0.0;\n"
			 "        gl_TessLevelOuter[3] = 0.0;\n"
			 "    }\n"
			 "    else\n"
			 "    if ((gl_PrimitiveID % 4) == 2)\n"
			 "    {\n"
			 "        gl_TessLevelOuter[0] = -1.0;\n"
			 "        gl_TessLevelOuter[1] = -1.0;\n"
			 "        gl_TessLevelOuter[2] = -1.0;\n"
			 "        gl_TessLevelOuter[3] = -1.0;\n"
			 "    }\n"
			 "    else\n"
			 "    {\n"
			 "        gl_TessLevelOuter[0] = 1.0;\n"
			 "        gl_TessLevelOuter[1] = 1.0;\n"
			 "        gl_TessLevelOuter[2] = 1.0;\n"
			 "        gl_TessLevelOuter[3] = 1.0;\n"
			 "    }\n"
			 "\n"
			 "    gl_TessLevelInner[0]                           = 1.0;\n"
			 "    gl_TessLevelInner[1]                           = 1.0;\n"
			 "    gl_out           [gl_InvocationID].gl_Position = gl_in[0].gl_Position;\n"
			 "    tc_primitive_id  [gl_InvocationID]             = gl_PrimitiveID;\n"
			 "}\n";

	return result;
}

/** Returns source code of a tessellation evaluation shader for the test,
 *  given user-specified vertex spacing and primitive modes.
 *
 *  Throws TestError exception if either of the arguments is invalid.
 *
 *  @param primitive_mode Primitive mode to use in the shader.
 *
 *  @return Requested string.
 **/
std::string TessellationShaderTessellationInputPatchDiscard::getTECode(_tessellation_primitive_mode primitive_mode)
{
	std::string result = "${VERSION}\n"
						 "\n"
						 "${TESSELLATION_SHADER_REQUIRE}\n"
						 "\n"
						 "layout(PRIMITIVE_MODE) in;\n"
						 "\n"
						 "in  int   tc_primitive_id   [];\n"
						 "out ivec2 te_tc_primitive_id;\n"
						 "out int   te_primitive_id;\n"
						 "\n"
						 "void main()\n"
						 "{\n"
						 "    te_tc_primitive_id[0] = tc_primitive_id[0];\n"
						 "    te_tc_primitive_id[1] = tc_primitive_id[1];\n"
						 "    te_primitive_id       = gl_PrimitiveID;\n"
						 "}\n";

	/* Replace PRIMITIVE_MODE token with actual primitive_mode */
	std::string primitive_mode_string	  = TessellationShaderUtils::getESTokenForPrimitiveMode(primitive_mode);
	const char* primitive_mode_token	   = "PRIMITIVE_MODE";
	std::size_t primitive_mode_token_index = std::string::npos;

	while ((primitive_mode_token_index = result.find(primitive_mode_token)) != std::string::npos)
	{
		result = result.replace(primitive_mode_token_index, strlen(primitive_mode_token), primitive_mode_string);

		primitive_mode_token_index = result.find(primitive_mode_token);
	}

	/* Done */
	return result;
}

/** Initializes ES objects necessary to run the test. */
void TessellationShaderTessellationInputPatchDiscard::initTest()
{
	/* Skip if required extensions are not supported. */
	if (!m_is_tessellation_shader_supported)
	{
		throw tcu::NotSupportedError(TESSELLATION_SHADER_EXTENSION_NOT_SUPPORTED);
	}

	/* Generate all test-wide objects needed for test execution */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();
	m_utils_ptr				 = new TessellationShaderUtils(gl, this);

	gl.genVertexArrays(1, &m_vao_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not generate vertex array object");

	gl.bindVertexArray(m_vao_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error binding vertex array object!");

	gl.genBuffers(1, &m_bo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenBuffers() failed");

	m_fs_id = gl.createShader(GL_FRAGMENT_SHADER);
	m_vs_id = gl.createShader(GL_VERTEX_SHADER);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateShader() failed");

	/* Configure fragment shader body */
	const char* fs_body = "${VERSION}\n"
						  "\n"
						  "void main()\n"
						  "{\n"
						  "}\n";

	shaderSourceSpecialized(m_fs_id, 1 /* count */, &fs_body);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glShaderSource() failed for fragment shader");

	/* Configure vertex shader body */
	const char* vs_body = "${VERSION}\n"
						  "\n"
						  "void main()\n"
						  "{\n"
						  "    gl_Position = vec4(1.0, 0.0, 0.0, 1.0);\n"
						  "}\n";

	shaderSourceSpecialized(m_vs_id, 1 /* count */, &vs_body);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glShaderSource() failed for vertex shader");

	/* Compile all the shaders */
	const glw::GLuint  shaders[] = { m_fs_id, m_vs_id };
	const unsigned int n_shaders = sizeof(shaders) / sizeof(shaders[0]);

	m_utils_ptr->compileShaders(n_shaders, shaders, true);

	/* Initialize all the runs. */
	const _tessellation_primitive_mode primitive_modes[] = { TESSELLATION_SHADER_PRIMITIVE_MODE_ISOLINES,
															 TESSELLATION_SHADER_PRIMITIVE_MODE_QUADS,
															 TESSELLATION_SHADER_PRIMITIVE_MODE_TRIANGLES };
	const unsigned int n_primitive_modes = sizeof(primitive_modes) / sizeof(primitive_modes[0]);

	for (unsigned int n_primitive_mode = 0; n_primitive_mode < n_primitive_modes; ++n_primitive_mode)
	{
		/* Initialize the run */
		_tessellation_primitive_mode primitive_mode = primitive_modes[n_primitive_mode];
		_run						 run;

		initRun(run, primitive_mode);

		/* Store the run */
		m_runs.push_back(run);
	} /* for (all primitive modes) */

	/* Set up buffer object bindings. Storage size will be determined on
	 * a per-iteration basis.
	 **/
	gl.bindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, m_bo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer() failed");

	gl.bindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0 /* index */, m_bo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBufferBase() failed");
}

/** Initializes all ES objects necessary to run a specific test pass.
 *
 *  Throws TestError exception if any of the arguments is found invalid.
 *
 *  @param run              Run descriptor to fill with IDs of initialized objects.
 *  @param primitive_mode   Primitive mode to use for the pass.
 **/
void TessellationShaderTessellationInputPatchDiscard::initRun(_run& run, _tessellation_primitive_mode primitive_mode)
{
	run.primitive_mode = primitive_mode;

	/* Set up a program object for the descriptor */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	run.po_id = gl.createProgram();
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateProgram() failed");

	/* Set up tessellation shader objects. */
	run.tc_id = gl.createShader(m_glExtTokens.TESS_CONTROL_SHADER);
	run.te_id = gl.createShader(m_glExtTokens.TESS_EVALUATION_SHADER);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateShader() call(s) failed");

	/* Configure tessellation control shader body */
	std::string tc_body			= getTCCode();
	const char* tc_body_raw_ptr = tc_body.c_str();

	shaderSourceSpecialized(run.tc_id, 1 /* count */, &tc_body_raw_ptr);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glShaderSource() failed for tessellation control shader");

	/* Configure tessellation evaluation shader body */
	std::string te_body			= getTECode(primitive_mode);
	const char* te_body_raw_ptr = te_body.c_str();

	shaderSourceSpecialized(run.te_id, 1 /* count */, &te_body_raw_ptr);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glShaderSource() failed for tessellation evaluation shader");

	/* Compile the tessellation evaluation shader */
	glw::GLuint		   shaders[] = { run.tc_id, run.te_id };
	const unsigned int n_shaders = sizeof(shaders) / sizeof(shaders[0]);

	m_utils_ptr->compileShaders(n_shaders, shaders, true);

	/* Attach all shader to the program object */
	gl.attachShader(run.po_id, m_fs_id);
	gl.attachShader(run.po_id, run.tc_id);
	gl.attachShader(run.po_id, run.te_id);
	gl.attachShader(run.po_id, m_vs_id);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glAttachShader() failed");

	/* Set up XFB */
	const char*		   varyings[] = { "te_tc_primitive_id", "te_primitive_id" };
	const unsigned int n_varyings = sizeof(varyings) / sizeof(varyings[0]);

	gl.transformFeedbackVaryings(run.po_id, n_varyings, varyings, GL_INTERLEAVED_ATTRIBS);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTransformFeedbackVaryings() failed");

	/* Link the program object */
	glw::GLint link_status = GL_FALSE;

	gl.linkProgram(run.po_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glLinkProgram() failed");

	gl.getProgramiv(run.po_id, GL_LINK_STATUS, &link_status);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetProgramiv() failed");

	if (link_status != GL_TRUE)
	{
		TCU_FAIL("Program linking failed");
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
tcu::TestNode::IterateResult TessellationShaderTessellationInputPatchDiscard::iterate(void)
{
	/* Do not execute if required extensions are not supported. */
	if (!m_is_tessellation_shader_supported)
	{
		throw tcu::NotSupportedError(TESSELLATION_SHADER_EXTENSION_NOT_SUPPORTED);
	}

	/* Initialize ES test objects */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	initTest();

	/* We don't need rasterization for this test */
	gl.enable(GL_RASTERIZER_DISCARD);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glEnable(GL_RASTERIZER_DISCARD) failed.");

	/* Configure amount of vertices per input patch */
	gl.patchParameteri(m_glExtTokens.PATCH_VERTICES, 1);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glPatchParameteriEXT() failed for GL_PATCH_VERTICES_EXT pname");

	/* Iterate through all tests configured */
	for (_runs_const_iterator run_iterator = m_runs.begin(); run_iterator != m_runs.end(); run_iterator++)
	{
		const _run& run = *run_iterator;

		/* Set up XFB target BO storage size. Each input patch will generate:
		 *
		 * a) 1 ivec2 and 1 int IF it is an odd patch;
		 * b) 0 bytes otherwise.
		 *
		 * This gives us a total of 2 * (sizeof(int)*2 + sizeof(int)) = 24 bytes per result coordinate,
		 * assuming it does not get discarded along the way.
		 *
		 * Amount of vertices that TE processes is mode-dependent. Note that for 'quads' we use 6 instead
		 * of 4 because the geometry will be broken down to triangles (1 quad = 2 triangles = 6 vertices)
		 * later in the pipeline.
		 */
		const unsigned int n_total_primitives = 4;
		const unsigned int n_vertices_per_patch =
			((run.primitive_mode == TESSELLATION_SHADER_PRIMITIVE_MODE_ISOLINES) ?
				 2 :
				 (run.primitive_mode == TESSELLATION_SHADER_PRIMITIVE_MODE_QUADS) ? 6 : 3);
		const unsigned int n_bytes_per_result_vertex = sizeof(int) + 2 * sizeof(int);
		const unsigned int n_bytes_needed = (n_total_primitives / 2) * n_vertices_per_patch * n_bytes_per_result_vertex;

		gl.bufferData(GL_TRANSFORM_FEEDBACK_BUFFER, n_bytes_needed, NULL, /* data */
					  GL_STATIC_DRAW);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferData() failed");

		/* Activate the program object */
		gl.useProgram(run.po_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram() failed");

		/* Draw the test geometry. */
		glw::GLenum tf_mode =
			TessellationShaderUtils::getTFModeForPrimitiveMode(run.primitive_mode, false); /* is_point_mode_enabled */

		gl.beginTransformFeedback(tf_mode);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBeginTransformFeedback() failed.");

		gl.drawArrays(m_glExtTokens.PATCHES, 0 /* first */, 1 /* vertices per patch */ * n_total_primitives);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawArrays() failed");

		gl.endTransformFeedback();
		GLU_EXPECT_NO_ERROR(gl.getError(), "glEndTransformFeedback() failed");

		/* Map the BO with result data into user space */
		int* result_data = (int*)gl.mapBufferRange(GL_TRANSFORM_FEEDBACK_BUFFER, 0, /* offset */
												   n_bytes_needed, GL_MAP_READ_BIT | GL_MAP_WRITE_BIT);

		GLU_EXPECT_NO_ERROR(gl.getError(), "glMapBufferRange() call failed");

		/* Verification is based on the following reasoning:
		 *
		 * a) Both TC and TE stages should operate on the same primitive IDs (no re-ordering
		 *    of the IDs is allowed)
		 * b) Under test-specific configuration, tessellator will output 2 line segments (4 coordinates).
		 *    Two first two coordinates will be generated during tessellation of the second primitive,
		 *    and the other two coordinates will be generated during tessellation of the fourth primitive
		 *    (out of all four primitives that will enter the pipeline).
		 * c) In case of quads, 6 first coordinates will be generated during tessellation of the second primitive,
		 *    and the other six will be generated during tessellation of the fourth primitive.
		 * d) Finally, tessellator will output 2 triangles (6 coordinates). The first three coordinates will
		 *    be generated during tessellation of the second primitive, and the other two for the fourth
		 *    primitive.
		 * */
		const int expected_primitive_ids[] = {
			1, /* second primitive */
			3  /* fourth primitive */
		};
		const unsigned int n_expected_primitive_ids =
			sizeof(expected_primitive_ids) / sizeof(expected_primitive_ids[0]);
		const unsigned int n_expected_repetitions =
			(run.primitive_mode == TESSELLATION_SHADER_PRIMITIVE_MODE_ISOLINES) ?
				2 :
				(run.primitive_mode == TESSELLATION_SHADER_PRIMITIVE_MODE_QUADS) ? 6 : 3;
		const int* traveller_ptr = result_data;

		for (unsigned int n_primitive_id = 0; n_primitive_id < n_expected_primitive_ids; ++n_primitive_id)
		{
			int expected_primitive_id = expected_primitive_ids[n_primitive_id];

			for (unsigned int n_repetition = 0; n_repetition < n_expected_repetitions; ++n_repetition)
			{
				for (unsigned int n_integer = 0; n_integer < 3 /* ivec2 + int */; ++n_integer)
				{
					if (*traveller_ptr != expected_primitive_id)
					{
						std::string primitive_mode_string =
							TessellationShaderUtils::getESTokenForPrimitiveMode(run.primitive_mode);

						m_testCtx.getLog() << tcu::TestLog::Message << "For primitive mode: " << primitive_mode_string
										   << " invalid gl_PrimitiveID of value " << *traveller_ptr
										   << " was found instead of expected " << expected_primitive_id
										   << " at index:" << n_primitive_id * 3 /* ivec2 + int */ + n_integer
										   << tcu::TestLog::EndMessage;

						TCU_FAIL("Discard mechanism failed");
					}

					traveller_ptr++;
				} /* for (all captured integers) */
			}	 /* for (all repetitions) */
		}		  /* for (all non-discarded primitive ids) */

		/* Clear the buffer storage space before we unmap it */
		memset(result_data, 0, n_bytes_needed);

		/* Unmap the BO */
		gl.unmapBuffer(GL_TRANSFORM_FEEDBACK_BUFFER);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glUnmapBuffer() call failed");
	}

	/* All done */
	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	return STOP;
}

/** Constructor
 *
 * @param context Test context
 **/
TessellationShaderTessellationgl_InvocationID_PatchVerticesIn_PrimitiveID::
	TessellationShaderTessellationgl_InvocationID_PatchVerticesIn_PrimitiveID(Context&			   context,
																			  const ExtParameters& extParams)
	: TestCaseBase(context, extParams, "gl_InvocationID_PatchVerticesIn_PrimitiveID",
				   "Verifies that gl_InvocationID, gl_PatchVerticesIn and gl_PrimitiveID "
				   "are assigned correct values in TC and TE stages (where appropriate), "
				   "when invoked with arrayed and indiced draw calls. Also verifies that "
				   "restarting primitive topology does not restart primitive ID counter.")
	, m_bo_id(0)
	, m_fs_id(0)
	, m_vs_id(0)
	, m_vao_id(0)
	, m_utils_ptr(0)
{
	/* Left blank on purpose */
}

/** Deinitializes ES objects created for the test. */
void TessellationShaderTessellationgl_InvocationID_PatchVerticesIn_PrimitiveID::deinit()
{
	/* Call base class' deinit() */
	TestCaseBase::deinit();

	if (!m_is_tessellation_shader_supported)
	{
		return;
	}

	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Disable modes this test has enabled */
	gl.disable(GL_RASTERIZER_DISCARD);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glDisable(GL_RASTERIZER_DISCARD) failed");

	gl.disable(GL_PRIMITIVE_RESTART_FIXED_INDEX);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glDisable(GL_PRIMITIVE_RESTART_FIXED_INDEX) failed");

	/* Remove TF buffer object bindings */
	gl.bindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, 0 /* buffer */);
	gl.bindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0 /* index */, 0 /* buffer */);

	/* Restore GL_PATCH_VERTICES_EXT value */
	gl.patchParameteri(m_glExtTokens.PATCH_VERTICES, 3);

	/* Unbind vertex array object */
	gl.bindVertexArray(0);

	/* Free all ES objects we allocated for the test */
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

	/* Deinitialize all run descriptors */
	for (_runs::iterator it = m_runs.begin(); it != m_runs.end(); ++it)
	{
		deinitRun(*it);
	}
	m_runs.clear();

	/* Release tessellation shader test utilities instance */
	if (m_utils_ptr != NULL)
	{
		delete m_utils_ptr;

		m_utils_ptr = NULL;
	}
}

/** Deinitialize all test pass-specific ES objects.
 *
 *  @param test Descriptor of a test pass to deinitialize.
 **/
void TessellationShaderTessellationgl_InvocationID_PatchVerticesIn_PrimitiveID::deinitRun(_run& run)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (run.bo_indices_id != 0)
	{
		gl.deleteBuffers(1, &run.bo_indices_id);

		run.bo_indices_id = 0;
	}

	if (run.po_id != 0)
	{
		gl.deleteProgram(run.po_id);

		run.po_id = 0;
	}

	if (run.tc_id != 0)
	{
		gl.deleteShader(run.tc_id);

		run.tc_id = 0;
	}

	if (run.te_id != 0)
	{
		gl.deleteShader(run.te_id);

		run.te_id = 0;
	}
}

/** Returns source code of a tessellation control shader.
 *
 *  @param n_patch_vertices Amount of vertices per patch to
 *                          output in TC stage;
 *
 *  @return Requested string.
 **/
std::string TessellationShaderTessellationgl_InvocationID_PatchVerticesIn_PrimitiveID::getTCCode(
	glw::GLuint n_patch_vertices)
{
	static const char* tc_body =
		"${VERSION}\n"
		"\n"
		"${TESSELLATION_SHADER_REQUIRE}\n"
		"\n"
		"layout(vertices = N_PATCH_VERTICES) out;\n"
		"\n"
		"out TC_OUT\n"
		"{\n"
		"    int tc_invocation_id;\n"
		"    int tc_patch_vertices_in;\n"
		"    int tc_primitive_id;\n"
		"} out_te[];\n"
		"\n"
		"void main()\n"
		"{\n"
		"    gl_out[gl_InvocationID].gl_Position          = gl_in[gl_InvocationID].gl_Position;\n"
		"    out_te[gl_InvocationID].tc_invocation_id     = gl_InvocationID;\n"
		"    out_te[gl_InvocationID].tc_patch_vertices_in = gl_PatchVerticesIn;\n"
		"    out_te[gl_InvocationID].tc_primitive_id      = gl_PrimitiveID;\n"
		"\n"
		"    gl_TessLevelInner[0] = 4.0;\n"
		"    gl_TessLevelInner[1] = 4.0;\n"
		"    gl_TessLevelOuter[0] = 4.0;\n"
		"    gl_TessLevelOuter[1] = 4.0;\n"
		"    gl_TessLevelOuter[2] = 4.0;\n"
		"    gl_TessLevelOuter[3] = 4.0;\n"
		"}\n";

	/* Construct a string out of user-provided integer value */
	std::stringstream n_patch_vertices_sstream;
	std::string		  n_patch_vertices_string;

	n_patch_vertices_sstream << n_patch_vertices;
	n_patch_vertices_string = n_patch_vertices_sstream.str();

	/* Replace N_PATCH_VERTICES with user-provided value */
	std::string		  result	  = tc_body;
	const std::string token		  = "N_PATCH_VERTICES";
	std::size_t		  token_index = std::string::npos;

	while ((token_index = result.find(token)) != std::string::npos)
	{
		result = result.replace(token_index, token.length(), n_patch_vertices_string.c_str());

		token_index = result.find(token);
	}

	return result;
}

/** Returns source code of a tessellation evaluation shader for the test,
 *  given user-specified primitive mode.
 *
 *  Throws TestError exception if either of the arguments is invalid.
 *
 *  @param primitive_mode Primitive mode to use in the shader.
 *
 *  @return Requested string.
 **/
std::string TessellationShaderTessellationgl_InvocationID_PatchVerticesIn_PrimitiveID::getTECode(
	_tessellation_primitive_mode primitive_mode)
{
	static const char* te_body = "${VERSION}\n"
								 "\n"
								 "${TESSELLATION_SHADER_REQUIRE}\n"
								 "\n"
								 "layout(PRIMITIVE_MODE) in;\n"
								 "\n"
								 "in TC_OUT\n"
								 "{\n"
								 "    int tc_invocation_id;\n"
								 "    int tc_patch_vertices_in;\n"
								 "    int tc_primitive_id;\n"
								 "} in_tc[];\n"
								 "\n"
								 "out int te_tc_invocation_id;\n"
								 "out int te_tc_patch_vertices_in;\n"
								 "out int te_tc_primitive_id;\n"
								 "out int te_patch_vertices_in;\n"
								 "out int te_primitive_id;\n"
								 "\n"
								 "void main()\n"
								 "{\n"
								 "    te_tc_invocation_id     = in_tc[gl_PatchVerticesIn-1].tc_invocation_id;\n"
								 "    te_tc_patch_vertices_in = in_tc[gl_PatchVerticesIn-1].tc_patch_vertices_in;\n"
								 "    te_tc_primitive_id      = in_tc[gl_PatchVerticesIn-1].tc_primitive_id;\n"
								 "    te_patch_vertices_in    = gl_PatchVerticesIn;\n"
								 "    te_primitive_id         = gl_PrimitiveID;\n"
								 "}";

	/* Replace PRIMITIVE_MODE with user-provided value */
	std::string		  primitive_mode_string = TessellationShaderUtils::getESTokenForPrimitiveMode(primitive_mode);
	std::string		  result				= te_body;
	const std::string token					= "PRIMITIVE_MODE";
	std::size_t		  token_index			= std::string::npos;

	while ((token_index = result.find(token)) != std::string::npos)
	{
		result = result.replace(token_index, token.length(), primitive_mode_string.c_str());

		token_index = result.find(token);
	}

	return result;
}

/** Initializes ES objects necessary to run the test. */
void TessellationShaderTessellationgl_InvocationID_PatchVerticesIn_PrimitiveID::initTest()
{
	/* Skip if required extensions are not supported. */
	if (!m_is_tessellation_shader_supported)
	{
		throw tcu::NotSupportedError(TESSELLATION_SHADER_EXTENSION_NOT_SUPPORTED);
	}

	/* Set up Utils instance */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	m_utils_ptr = new TessellationShaderUtils(gl, this);

	/* Initialize vertex array object */
	gl.genVertexArrays(1, &m_vao_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not generate vertex array object");

	gl.bindVertexArray(m_vao_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error binding vertex array object!");

	/* Generate all test-wide objects needed for test execution */
	gl.genBuffers(1, &m_bo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenBuffers() failed");

	m_fs_id = gl.createShader(GL_FRAGMENT_SHADER);
	m_vs_id = gl.createShader(GL_VERTEX_SHADER);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateShader() failed");

	/* Configure fragment shader body */
	const char* fs_body = "${VERSION}\n"
						  "\n"
						  "void main()\n"
						  "{\n"
						  "}\n";

	shaderSourceSpecialized(m_fs_id, 1 /* count */, &fs_body);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glShaderSource() failed for fragment shader");

	/* Configure vertex shader body */
	const char* vs_body = "${VERSION}\n"
						  "\n"
						  "in vec4 vertex_data;\n"
						  "\n"
						  "void main()\n"
						  "{\n"
						  "    gl_Position = vertex_data;\n"
						  "}\n";

	shaderSourceSpecialized(m_vs_id, 1 /* count */, &vs_body);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glShaderSource() failed for vertex shader");

	/* Compile all the shaders */
	const glw::GLuint  shaders[] = { m_fs_id, m_vs_id };
	const unsigned int n_shaders = sizeof(shaders) / sizeof(shaders[0]);

	m_utils_ptr->compileShaders(n_shaders, shaders, true /* should_succeed */);

	/* Retrieve GL_MAX_PATCH_VERTICES_EXT value before we continue */
	glw::GLint gl_max_patch_vertices_value = 0;

	gl.getIntegerv(m_glExtTokens.MAX_PATCH_VERTICES, &gl_max_patch_vertices_value);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv() failed for GL_MAX_PATCH_VERTICES_EXT pname");

	/* Initialize all test passes */
	const unsigned int drawcall_count_multipliers[] = { 3, 6 };
	const bool		   is_indiced_draw_call_flags[] = { false, true };
	const glw::GLint   n_instances[]				= { 1, 4 };
	const glw::GLint   n_patch_vertices[] = { 4, gl_max_patch_vertices_value / 2, gl_max_patch_vertices_value };
	const _tessellation_primitive_mode primitive_modes[] = { TESSELLATION_SHADER_PRIMITIVE_MODE_ISOLINES,
															 TESSELLATION_SHADER_PRIMITIVE_MODE_QUADS,
															 TESSELLATION_SHADER_PRIMITIVE_MODE_TRIANGLES };
	const unsigned int n_drawcall_count_multipliers =
		sizeof(drawcall_count_multipliers) / sizeof(drawcall_count_multipliers[0]);
	const unsigned int n_is_indiced_draw_call_flags =
		sizeof(is_indiced_draw_call_flags) / sizeof(is_indiced_draw_call_flags[0]);
	const unsigned int n_n_instances	  = sizeof(n_instances) / sizeof(n_instances[0]);
	const unsigned int n_n_patch_vertices = sizeof(n_patch_vertices) / sizeof(n_patch_vertices[0]);
	const unsigned int n_primitive_modes  = sizeof(primitive_modes) / sizeof(primitive_modes[0]);

	for (unsigned int n_primitive_mode = 0; n_primitive_mode < n_primitive_modes; ++n_primitive_mode)
	{
		_tessellation_primitive_mode current_primitive_mode = primitive_modes[n_primitive_mode];

		for (unsigned int n_patch_vertices_item = 0; n_patch_vertices_item < n_n_patch_vertices;
			 ++n_patch_vertices_item)
		{
			glw::GLint current_n_patch_vertices = n_patch_vertices[n_patch_vertices_item];

			for (unsigned int n_is_indiced_draw_call_flag = 0;
				 n_is_indiced_draw_call_flag < n_is_indiced_draw_call_flags; ++n_is_indiced_draw_call_flag)
			{
				bool current_is_indiced_draw_call = is_indiced_draw_call_flags[n_is_indiced_draw_call_flag];

				for (unsigned int n_instances_item = 0; n_instances_item < n_n_instances; ++n_instances_item)
				{
					glw::GLint current_n_instances = n_instances[n_instances_item];

					for (unsigned int n_drawcall_count_multiplier = 0;
						 n_drawcall_count_multiplier < n_drawcall_count_multipliers; ++n_drawcall_count_multiplier)
					{
						const unsigned int drawcall_count_multiplier =
							drawcall_count_multipliers[n_drawcall_count_multiplier];

						/* Form the run descriptor */
						_run run;

						initRun(run, current_primitive_mode, current_n_patch_vertices, current_is_indiced_draw_call,
								current_n_instances, drawcall_count_multiplier);

						/* Store the descriptor for later execution */
						m_runs.push_back(run);
					}
				} /* for (all 'number of instances' settings) */
			}	 /* for (all 'is indiced draw call' flags) */
		}		  /* for (all 'n patch vertices' settings) */
	}			  /* for (all primitive modes) */

	/* Set up buffer object bindings. Storage size will be determined on
	 * a per-iteration basis.
	 **/
	gl.bindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, m_bo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer() failed");

	gl.bindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0 /* index */, m_bo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBufferBase() failed");
}

/** Initializes all ES objects necessary to run a specific test pass.
 *
 *  Throws TestError exception if any of the arguments is found invalid.
 *
 *  @param run                       Test run descriptor to fill with IDs of initialized objects.
 *  @param primitive_mode            Primitive mode to use for the pass.
 *  @param n_patch_vertices          Amount of output patch vertices to use for the pass.
 *  @param is_indiced                true  if the draw call to be used for the test run should be indiced;
 *                                   false to use the non-indiced one.
 *  @param n_instances               Amount of instances to use for the draw call. Set to 1 if the draw
 *                                   call should be non-instanced.
 *  @param drawcall_count_multiplier Will be used to multiply the "count" argument of the draw call API
 *                                   function, effectively multiplying amount of primitives that will be
 *                                   generated by the tessellator.
 **/
void TessellationShaderTessellationgl_InvocationID_PatchVerticesIn_PrimitiveID::initRun(
	_run& run, _tessellation_primitive_mode primitive_mode, glw::GLint n_patch_vertices, bool is_indiced,
	glw::GLint n_instances, unsigned int drawcall_count_multiplier)
{
	run.drawcall_count_multiplier = drawcall_count_multiplier;
	run.drawcall_is_indiced		  = is_indiced;
	run.n_instances				  = n_instances;
	run.n_patch_vertices		  = n_patch_vertices;
	run.primitive_mode			  = primitive_mode;

	/* Set up a program object for the descriptor */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	run.po_id = gl.createProgram();
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateProgram() failed");

	/* Set up tessellation control shader object */
	std::string tc_body			= getTCCode(n_patch_vertices);
	const char* tc_body_raw_ptr = tc_body.c_str();

	run.tc_id = gl.createShader(m_glExtTokens.TESS_CONTROL_SHADER);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateShader() call failed");

	shaderSourceSpecialized(run.tc_id, 1 /* count */, &tc_body_raw_ptr);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glShaderSource() failed for tessellation control shader");

	/* Set up tessellation evaluation shader object. */
	std::string te_body			= getTECode(primitive_mode);
	const char* te_body_raw_ptr = te_body.c_str();

	run.te_id = gl.createShader(m_glExtTokens.TESS_EVALUATION_SHADER);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateShader() call failed");

	shaderSourceSpecialized(run.te_id, 1 /* count */, &te_body_raw_ptr);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glShaderSource() failed for tessellation evaluation shader");

	/* Compile the shaders */
	const glw::GLuint  shader_ids[] = { run.tc_id, run.te_id };
	const unsigned int n_shader_ids = sizeof(shader_ids) / sizeof(shader_ids[0]);

	m_utils_ptr->compileShaders(n_shader_ids, shader_ids, true /* should_succeed */);

	/* Attach all shader to the program object */
	gl.attachShader(run.po_id, m_fs_id);
	gl.attachShader(run.po_id, run.te_id);
	gl.attachShader(run.po_id, m_vs_id);
	gl.attachShader(run.po_id, run.tc_id);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glAttachShader() failed");

	/* Set up XFB */
	const char* varyings[] = { "te_tc_invocation_id", "te_tc_patch_vertices_in", "te_tc_primitive_id",
							   "te_patch_vertices_in", "te_primitive_id" };
	const unsigned int n_varyings = sizeof(varyings) / sizeof(varyings[0]);

	gl.transformFeedbackVaryings(run.po_id, n_varyings, varyings, GL_INTERLEAVED_ATTRIBS);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTransformFeedbackVaryings() failed");

	/* Link the program object */
	glw::GLint link_status = GL_FALSE;

	gl.linkProgram(run.po_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glLinkProgram() failed");

	gl.getProgramiv(run.po_id, GL_LINK_STATUS, &link_status);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetProgramiv() failed");

	if (link_status != GL_TRUE)
	{
		TCU_FAIL("Program linking failed");
	}

	/* If this is going to be an indiced draw call, we need to initialize a buffer
	 * object that will hold index data. GL_UNSIGNED_BYTE index type will be always
	 * used for the purpose of this test.
	 */
	if (is_indiced)
	{
		gl.genBuffers(1, &run.bo_indices_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGenBuffers() failed");

		/* Implementations are allowed NOT to support primitive restarting for patches.
		 * Take this into account and do not insert restart indices, if ES reports no
		 * support.
		 */
		glw::GLboolean is_primitive_restart_supported = GL_TRUE;

		gl.getBooleanv(GL_PRIMITIVE_RESTART_FOR_PATCHES_SUPPORTED, &is_primitive_restart_supported);
		GLU_EXPECT_NO_ERROR(gl.getError(),
							"glGetIntegerv() failed for GL_PRIMITIVE_RESTART_FOR_PATCHES_SUPPORTED pname");

		/* Set up index buffer storage. Note that we're not using any attributes
		 * in any stage - our goal is just to make sure the primitive counter does
		 * not restart whenever restart index is encountered during a draw call */
		DE_ASSERT(run.n_patch_vertices > 3);
		DE_ASSERT(run.drawcall_count_multiplier > 1);

		const unsigned int interleave_rate = run.n_patch_vertices * 2;
		unsigned char*	 bo_contents	 = DE_NULL;
		unsigned int	   bo_size =
			static_cast<unsigned int>(sizeof(unsigned char) * run.n_patch_vertices * run.drawcall_count_multiplier);

		/* Count in restart indices if necessary */
		if (is_primitive_restart_supported)
		{
			run.n_restart_indices = (bo_size / interleave_rate);
			bo_size += 1 /* restart index */ * run.n_restart_indices;
		}

		/* Allocate space for the index buffer */
		bo_contents = new unsigned char[bo_size];

		/* Interleave the restart index every two complete sets of vertices, each set
		 * making up a full set of vertices. Fill all other indices with zeros. The
		 * indices don't really matter since test shaders do not use any attributes -
		 * what we want to verify is that the restart index does not break the primitive
		 * id counter.
		 *
		 * NOTE: Our interleave rate is just an arbitrary value that makes
		 *       sense, given the multipliers we use for the test */
		const unsigned char restart_index = 0xFF;

		memset(bo_contents, 0, bo_size);

		if (is_primitive_restart_supported)
		{
			for (unsigned int n_index = interleave_rate; n_index < bo_size; n_index += interleave_rate)
			{
				bo_contents[n_index] = restart_index;

				/* Move one index ahead */
				n_index++;
			}
		}

		/* Set up the buffer object storage */
		gl.bindBuffer(GL_ELEMENT_ARRAY_BUFFER, run.bo_indices_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer() failed");

		gl.bufferData(GL_ELEMENT_ARRAY_BUFFER, bo_size, bo_contents, GL_STATIC_DRAW);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferData() failed");

		/* Release the buffer */
		delete[] bo_contents;

		bo_contents = NULL;
	}

	/* Retrieve amount of tessellation coordinates.
	 *
	 * Note: this test assumes a constant tessellation level value of 4 for all
	 *       inner/outer tessellation levels */
	const glw::GLfloat tess_levels[] = { 4.0f, 4.0f, 4.0f, 4.0f };

	run.n_result_vertices = m_utils_ptr->getAmountOfVerticesGeneratedByTessellator(
		run.primitive_mode, tess_levels, tess_levels, TESSELLATION_SHADER_VERTEX_SPACING_EQUAL,
		false); /* is_point_mode_enabled */

	/* The value we have at this point is for single patch only. To end up
	 * with actual amount of coordinates that will be generated by the tessellator,
	 * we need to multiply it by drawcall_count_multiplier * n_instances */
	run.n_result_vertices *= run.drawcall_count_multiplier * run.n_instances;

	/* We're done! */
}

/** Executes the test.
 *
 *  Sets the test result to QP_TEST_RESULT_FAIL if the test failed, QP_TEST_RESULT_PASS otherwise.
 *
 *  Note the function throws exception should an error occur!
 *
 *  @return STOP if the test has finished, CONTINUE to indicate iterate() should be called once again.
 **/
tcu::TestNode::IterateResult TessellationShaderTessellationgl_InvocationID_PatchVerticesIn_PrimitiveID::iterate(void)
{
	/* Do not execute if required extensions are not supported. */
	if (!m_is_tessellation_shader_supported)
	{
		throw tcu::NotSupportedError(TESSELLATION_SHADER_EXTENSION_NOT_SUPPORTED);
	}

	/* Initialize ES test objects */
	initTest();

	/* Initialize tessellation shader utilities */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* We don't need rasterization for this test */
	gl.enable(GL_RASTERIZER_DISCARD);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glEnable(GL_RASTERIZER_DISCARD) failed.");

	/* Enable GL_PRIMITIVE_RESTART_FIXED_INDEX mode for indiced draw calls. */
	gl.enable(GL_PRIMITIVE_RESTART_FIXED_INDEX);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glEnable(GL_PRIMITIVE_RESTART_FIXED_INDEX) failed.");

	/* Iterate through all test runs configured */
	for (_runs_const_iterator run_iterator = m_runs.begin(); run_iterator != m_runs.end(); run_iterator++)
	{
		const _run& run = *run_iterator;

		/* Configure run-specific amount of vertices per patch */
		gl.patchParameteri(m_glExtTokens.PATCH_VERTICES, run.n_patch_vertices);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glPatchParameteriEXT() failed for GL_PATCH_VERTICES_EXT pname");

		/* Activate run-specific program object */
		gl.useProgram(run.po_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram() failed");

		/* Update GL_ELEMENT_ARRAY_BUFFER binding, depending on run properties */
		gl.bindBuffer(GL_ELEMENT_ARRAY_BUFFER, (run.drawcall_is_indiced) ? run.bo_indices_id : 0);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Could not update GL_ELEMENT_ARRAY_BUFFER binding");

		/* Update transform feedback buffer bindings */
		gl.bindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, m_bo_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer() failed");

		gl.bindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0 /* index */, m_bo_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBufferBase() failed");

		/* Update the transform feedback buffer object storage. For each generated
		 * tessellated coordinate, TE stage will output 5 integers. */
		glw::GLint bo_size = static_cast<glw::GLint>(run.n_result_vertices * 5 /* ints */ * sizeof(int));

		gl.bufferData(GL_TRANSFORM_FEEDBACK_BUFFER, bo_size, DE_NULL, GL_STATIC_DRAW);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferData() failed");

		/* Render the geometry */
		glw::GLint  drawcall_count = run.n_patch_vertices * run.drawcall_count_multiplier + run.n_restart_indices;
		glw::GLenum tf_mode =
			TessellationShaderUtils::getTFModeForPrimitiveMode(run.primitive_mode, false); /* is_point_mode_enabled */

		gl.beginTransformFeedback(tf_mode);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBeginTransformFeedback() failed");
		{
			if (run.drawcall_is_indiced)
			{
				if (run.n_instances != 1)
				{
					gl.drawElementsInstanced(m_glExtTokens.PATCHES, drawcall_count, GL_UNSIGNED_BYTE,
											 DE_NULL, /* indices */
											 run.n_instances);
					GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawElementsInstanced() failed");
				} /* if (run.n_instances != 0) */
				else
				{
					gl.drawElements(m_glExtTokens.PATCHES, drawcall_count, GL_UNSIGNED_BYTE, DE_NULL); /* indices */
					GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawElements() failed");
				}
			} /* if (run.drawcall_is_indiced) */
			else
			{
				if (run.n_instances != 1)
				{
					gl.drawArraysInstanced(m_glExtTokens.PATCHES, 0, /* first */
										   drawcall_count, run.n_instances);
					GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawArraysInstanced() failed");
				}
				else
				{
					gl.drawArrays(m_glExtTokens.PATCHES, 0, /* first */
								  drawcall_count);
					GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawArrays() failed");
				}
			}
		}
		gl.endTransformFeedback();
		GLU_EXPECT_NO_ERROR(gl.getError(), "glEndTransformFeedback() failed");

		/* Map the result buffer object */
		const int* result_data = (const int*)gl.mapBufferRange(GL_TRANSFORM_FEEDBACK_BUFFER, 0, /* offset */
															   bo_size, GL_MAP_READ_BIT);

		GLU_EXPECT_NO_ERROR(gl.getError(), "glMapBufferRange() failed");

		/* Verify gl_InvocationID values used in both stages were correct. In TE:
		 *
		 * te_tc_invocation_id = in_tc[gl_PatchVerticesIn-1].tc_invocation_id;
		 *
		 * In the list of varyings passed to glTransformFeedbackVaryings(),
		 * te_tc_invocation_id is the very first item, so no need to offset
		 * result_data when initializing result_traveller_ptr below.
		 */
		const unsigned int n_int_varyings_per_tess_coordinate = 5;
		const int*		   result_traveller_ptr				  = result_data;

		for (unsigned int n_coordinate = 0; n_coordinate < run.n_result_vertices;
			 ++n_coordinate, result_traveller_ptr += n_int_varyings_per_tess_coordinate)
		{
			const int expected_invocation_id = run.n_patch_vertices - 1;
			const int invocation_id_value	= *result_traveller_ptr;

			if (invocation_id_value != expected_invocation_id)
			{
				m_testCtx.getLog() << tcu::TestLog::Message << "Invalid gl_InvocationID value (" << invocation_id_value
								   << ") "
									  "was found for result coordinate at index "
								   << n_coordinate << " "
													  "instead of expected value ("
								   << expected_invocation_id << ")." << tcu::TestLog::EndMessage;

				TCU_FAIL("Invalid gl_InvocationID value used in TC stage");
			}
		} /* for (all result coordinates) */

		/* Verify gl_PrimitiveID values used in both stages were correct. In TE:
		 *
		 * te_tc_primitive_id = in_tc[gl_PatchVerticesIn-1].tc_primitive_id;
		 * te_primitive_id    = gl_PrimitiveID;
		 *
		 * In the list of varyings passed to glTransformFeedbackVaryings(),
		 * te_tc_primitive_id is passed as 3rd string and te_primitive_id is located on
		 * 5th location.
		 */
		const unsigned int n_result_vertices_per_patch_vertex_batch =
			run.n_result_vertices / run.drawcall_count_multiplier / run.n_instances;
		const unsigned int n_result_vertices_per_instance = run.n_result_vertices / run.n_instances;

		for (unsigned int n_coordinate = 0; n_coordinate < run.n_result_vertices; ++n_coordinate)
		{
			unsigned int actual_n_coordinate = n_coordinate;

			/* Subsequent instances reset gl_PrimitiveID counter */
			while (actual_n_coordinate >= n_result_vertices_per_instance)
			{
				actual_n_coordinate -= n_result_vertices_per_instance;
			}

			/* Calculate expected gl_PrimitiveID value */
			const int expected_primitive_id = actual_n_coordinate / n_result_vertices_per_patch_vertex_batch;

			/* te_tc_primitive_id */
			result_traveller_ptr =
				result_data + n_coordinate * n_int_varyings_per_tess_coordinate + 2; /* as per comment */

			if (*result_traveller_ptr != expected_primitive_id)
			{
				m_testCtx.getLog() << tcu::TestLog::Message << "Invalid gl_PrimitiveID value (" << *result_traveller_ptr
								   << ") "
									  "was used in TC stage instead of expected value ("
								   << expected_primitive_id << ") "
															   " as stored for result coordinate at index "
								   << n_coordinate << "." << tcu::TestLog::EndMessage;

				TCU_FAIL("Invalid gl_PrimitiveID value used in TC stage");
			}

			/* te_primitive_id */
			result_traveller_ptr =
				result_data + n_coordinate * n_int_varyings_per_tess_coordinate + 4; /* as per comment */

			if (*result_traveller_ptr != expected_primitive_id)
			{
				m_testCtx.getLog() << tcu::TestLog::Message << "Invalid gl_PrimitiveID value (" << *result_traveller_ptr
								   << ") "
									  "was used in TE stage instead of expected value ("
								   << expected_primitive_id << ") "
															   " as stored for result coordinate at index "
								   << n_coordinate << "." << tcu::TestLog::EndMessage;

				TCU_FAIL("Invalid gl_PrimitiveID value used in TE stage");
			}
		} /* for (all result coordinates) */

		/* Verify gl_PatchVerticesIn values used in both stages were correct. In TE:
		 *
		 * te_tc_patch_vertices_in = in_tc[gl_PatchVerticesIn-1].tc_patch_vertices_in;
		 * te_patch_vertices_in    = gl_PatchVerticesIn;
		 *
		 * In the list of varyings passed to glTransformFeedbackVaryings(),
		 * te_tc_patch_vertices_in takes 2nd location and te_patch_vertices_in is
		 * located at 4th position.
		 *
		 **/
		for (unsigned int n_coordinate = 0; n_coordinate < run.n_result_vertices; ++n_coordinate)
		{
			const int expected_patch_vertices_in_value = run.n_patch_vertices;

			/* te_tc_patch_vertices_in */
			result_traveller_ptr =
				result_data + n_coordinate * n_int_varyings_per_tess_coordinate + 1; /* as per comment */

			if (*result_traveller_ptr != expected_patch_vertices_in_value)
			{
				m_testCtx.getLog() << tcu::TestLog::Message << "Invalid gl_PatchVerticesIn value ("
								   << *result_traveller_ptr << ") "
															   "was used in TC stage instead of expected value ("
								   << expected_patch_vertices_in_value << ") "
																		  " as stored for result coordinate at index "
								   << n_coordinate << "." << tcu::TestLog::EndMessage;

				TCU_FAIL("Invalid gl_PatchVerticesIn value used in TC stage");
			}

			/* te_patch_vertices_in */
			result_traveller_ptr =
				result_data + n_coordinate * n_int_varyings_per_tess_coordinate + 3; /* as per comment */

			if (*result_traveller_ptr != expected_patch_vertices_in_value)
			{
				m_testCtx.getLog() << tcu::TestLog::Message << "Invalid gl_PatchVerticesIn value ("
								   << *result_traveller_ptr << ") "
															   "was used in TE stage instead of expected value ("
								   << expected_patch_vertices_in_value << ") "
																		  " as stored for result coordinate at index "
								   << n_coordinate << "." << tcu::TestLog::EndMessage;
			}
		} /* for (all result coordinates) */

		/* Unmap the buffer object - we're done with this iteration */
		gl.unmapBuffer(GL_TRANSFORM_FEEDBACK_BUFFER);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glUnmapBuffer() failed");
	} /* for (all runs) */

	/* All done */
	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	return STOP;
}

std::string TessellationShaderTessellationgl_TessCoord::getTypeName(_tessellation_test_type test_type)
{
	static const char* names[2] = { "TCS_TES", "TES" };
	DE_ASSERT(0 <= test_type && test_type <= DE_LENGTH_OF_ARRAY(names));
	DE_STATIC_ASSERT(0 == TESSELLATION_TEST_TYPE_TCS_TES && 1 == TESSELLATION_TEST_TYPE_TES);
	return names[test_type];
}

/** Constructor
 *
 * @param context Test context
 **/
TessellationShaderTessellationgl_TessCoord::TessellationShaderTessellationgl_TessCoord(
	Context& context, const ExtParameters& extParams, _tessellation_test_type test_type)
	: TestCaseBase(context, extParams, getTypeName(test_type).c_str(),
				   "Verifies that u, v, w components of gl_TessCoord are within "
				   "range for a variety of input/outer tessellation level combinations "
				   "for all primitive modes. Verifies each component is within valid "
				   " range. Also checks that w is always equal to 0 for isolines mode.")
	, m_test_type(test_type)
	, m_bo_id(0)
	, m_broken_ts_id(0)
	, m_fs_id(0)
	, m_vs_id(0)
	, m_vao_id(0)
	, m_utils_ptr(0)
{
	/* Left blank on purpose */
}

/** Deinitializes ES objects created for the test. */
void TessellationShaderTessellationgl_TessCoord::deinit()
{
	/* Call base class' deinit() */
	TestCaseBase::deinit();

	if (!m_is_tessellation_shader_supported)
	{
		return;
	}

	if (glu::isContextTypeES(m_context.getRenderContext().getType()) && m_test_type == TESSELLATION_TEST_TYPE_TES)
	{
		return;
	}

	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Revert buffer object bindings */
	gl.bindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, 0 /* buffer */);
	gl.bindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0 /* index */, 0 /* buffer */);

	/* Disable GL_RASTERIZER_DISCARD mode */
	gl.disable(GL_RASTERIZER_DISCARD);

	/* Restore GL_PATCH_VERTICES_EXT, GL_PATCH_DEFAULT_INNER_LEVEL and
	 * GL_PATCH_DEFAULT_OUTER_LEVEL values */
	if (!glu::isContextTypeES(m_context.getRenderContext().getType()))
	{
		const float default_tess_levels[] = { 1.0f, 1.0f, 1.0f, 1.0f };
		gl.patchParameterfv(GL_PATCH_DEFAULT_INNER_LEVEL, default_tess_levels);
		gl.patchParameterfv(GL_PATCH_DEFAULT_OUTER_LEVEL, default_tess_levels);
	}
	gl.patchParameteri(m_glExtTokens.PATCH_VERTICES, 3);

	/* Unbind vertex array object */
	gl.bindVertexArray(0);

	/* Free all ES objects we allocated for the test */
	if (m_bo_id != 0)
	{
		gl.deleteBuffers(1, &m_bo_id);

		m_bo_id = 0;
	}

	if (m_broken_ts_id != 0)
	{
		gl.deleteShader(m_broken_ts_id);

		m_broken_ts_id = 0;
	}

	if (m_fs_id != 0)
	{
		gl.deleteShader(m_fs_id);

		m_fs_id = 0;
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

	/* Deinitialize all test descriptors */
	for (_tests::iterator it = m_tests.begin(); it != m_tests.end(); ++it)
	{
		deinitTestDescriptor(*it);
	}
	m_tests.clear();

	/* Release tessellation shader test utilities instance */
	if (m_utils_ptr != NULL)
	{
		delete m_utils_ptr;

		m_utils_ptr = NULL;
	}
}

/** Deinitialize all test pass-specific ES objects.
 *
 *  @param test Descriptor of a test pass to deinitialize.
 **/
void TessellationShaderTessellationgl_TessCoord::deinitTestDescriptor(_test_descriptor& test)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (test.po_id != 0)
	{
		gl.deleteProgram(test.po_id);

		test.po_id = 0;
	}

	if (test.tc_id != 0)
	{
		gl.deleteShader(test.tc_id);

		test.tc_id = 0;
	}

	if (test.te_id != 0)
	{
		gl.deleteShader(test.te_id);

		test.te_id = 0;
	}
}

/** Returns source code of a tessellation control shader for the test,
 *  given user-specified amount of output patch vertices.
 *
 *  @param n_patch_vertices Amount of output patch vertices for TC stage.
 *
 *  @return Requested string.
 **/
std::string TessellationShaderTessellationgl_TessCoord::getTCCode(glw::GLint n_patch_vertices)
{
	return TessellationShaderUtils::getGenericTCCode(n_patch_vertices, true);
}

/** Returns source code of a tessellation evaluation shader for the test,
 *  given user-specified vertex spacing and primitive modes.
 *
 *  Throws TestError exception if either of the arguments is invalid.
 *
 *  @param vertex_spacing Vertex spacing mode to use in the shader.
 *  @param primitive_mode Primitive mode to use in the shader.
 *
 *  @return Requested string.
 **/
std::string TessellationShaderTessellationgl_TessCoord::getTECode(_tessellation_shader_vertex_spacing vertex_spacing,
																  _tessellation_primitive_mode		  primitive_mode)
{
	return TessellationShaderUtils::getGenericTECode(vertex_spacing, primitive_mode,
													 TESSELLATION_SHADER_VERTEX_ORDERING_CCW, false);
}

/** Initializes ES objects necessary to run the test. */
void TessellationShaderTessellationgl_TessCoord::initTest()
{
	/* Skip if required extensions are not supported. */
	if (!m_is_tessellation_shader_supported)
	{
		throw tcu::NotSupportedError(TESSELLATION_SHADER_EXTENSION_NOT_SUPPORTED);
	}

	if (glu::isContextTypeES(m_context.getRenderContext().getType()) && m_test_type == TESSELLATION_TEST_TYPE_TES)
	{
		throw tcu::NotSupportedError("Test can't be run in ES context");
	}

	/* Generate all test-wide objects needed for test execution */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.genVertexArrays(1, &m_vao_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not generate vertex array object");

	gl.bindVertexArray(m_vao_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error binding vertex array object!");

	gl.genBuffers(1, &m_bo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenBuffers() failed");

	m_broken_ts_id = gl.createShader(m_glExtTokens.TESS_EVALUATION_SHADER);
	m_fs_id		   = gl.createShader(GL_FRAGMENT_SHADER);
	m_vs_id		   = gl.createShader(GL_VERTEX_SHADER);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateShader() failed");

	/* Configure fragment shader body */
	const char* fs_body = "${VERSION}\n"
						  "\n"
						  "void main()\n"
						  "{\n"
						  "}\n";

	shaderSourceSpecialized(m_fs_id, 1 /* count */, &fs_body);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glShaderSource() failed for fragment shader");

	/* Configure vertex shader body */
	const char* vs_body = "${VERSION}\n"
						  "\n"
						  "void main()\n"
						  "{\n"
						  "    gl_Position = vec4(1.0, 0.0, 0.0, 1.0);\n"
						  "}\n";

	shaderSourceSpecialized(m_vs_id, 1 /* count */, &vs_body);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glShaderSource() failed for vertex shader");

	/* Compile all the shaders */
	const glw::GLuint  shaders[] = { m_fs_id, m_vs_id };
	const unsigned int n_shaders = sizeof(shaders) / sizeof(shaders[0]);

	for (unsigned int n_shader = 0; n_shader < n_shaders; ++n_shader)
	{
		glw::GLuint shader = shaders[n_shader];

		if (shader != 0)
		{
			glw::GLint compile_status = GL_FALSE;

			gl.compileShader(shader);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glCompileShader() failed");

			gl.getShaderiv(shader, GL_COMPILE_STATUS, &compile_status);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glGetShaderiv() failed");

			if (compile_status != GL_TRUE)
			{
				TCU_FAIL("Shader compilation failed");
			}
		}
	} /* for (all shaders) */

	/* Retrieve GL_MAX_TESS_GEN_LEVEL_EXT value before we start looping */
	glw::GLint gl_max_tess_gen_level_value = 0;

	gl.getIntegerv(m_glExtTokens.MAX_TESS_GEN_LEVEL, &gl_max_tess_gen_level_value);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv() failed for GL_MAX_TESS_GEN_LEVEL_EXT pname");

	/* Initialize all test passes - iterate over all primitive modes supported.. */
	for (int primitive_mode = static_cast<int>(TESSELLATION_SHADER_PRIMITIVE_MODE_FIRST);
		 primitive_mode != static_cast<int>(TESSELLATION_SHADER_PRIMITIVE_MODE_COUNT); primitive_mode++)
	{
		/* Iterate over all tessellation level combinations defined for current primitive mode */
		_tessellation_levels_set tessellation_levels = TessellationShaderUtils::getTessellationLevelSetForPrimitiveMode(
			static_cast<_tessellation_primitive_mode>(primitive_mode), gl_max_tess_gen_level_value,
			TESSELLATION_LEVEL_SET_FILTER_ALL_LEVELS_USE_THE_SAME_VALUE);

		for (_tessellation_levels_set_const_iterator levels_iterator = tessellation_levels.begin();
			 levels_iterator != tessellation_levels.end(); levels_iterator++)
		{
			const _tessellation_levels& levels = *levels_iterator;
			_test_descriptor			test;

			initTestDescriptor(test, TESSELLATION_SHADER_VERTEX_SPACING_EQUAL,
							   static_cast<_tessellation_primitive_mode>(primitive_mode), 1, /* n_patch_vertices */
							   levels.inner, levels.outer, m_test_type);

			/* Store the test descriptor */
			m_tests.push_back(test);
		} /* for (all tessellation level combinations for current primitive mode) */
	}	 /* for (all primitive modes) */

	/* Set up buffer object bindings. Storage size will be determined on
	 * a per-iteration basis.
	 **/
	gl.bindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, m_bo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer() failed");

	gl.bindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0 /* index */, m_bo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBufferBase() failed");
}

/** Initializes all ES objects necessary to run a specific test pass.
 *
 *  Throws TestError exception if any of the arguments is found invalid.
 *
 *  @param test             Test descriptor to fill with IDs of initialized objects.
 *  @param vertex_spacing   Vertex spacing mode to use for the pass.
 *  @param primitive_mode   Primitive mode to use for the pass.
 *  @param n_patch_vertices Amount of output patch vertices to use for the pass.
 *  @param inner_tess_level Inner tessellation level values to be used for the pass.
 *                          Must not be NULL.
 *  @param outer_tess_level Outer tessellation level values to be used for the pass.
 *                          Must not be NULL.
 *  @param test_type        Defines which tessellation stages should be defined for the pass.
 **/
void TessellationShaderTessellationgl_TessCoord::initTestDescriptor(
	_test_descriptor& test, _tessellation_shader_vertex_spacing vertex_spacing,
	_tessellation_primitive_mode primitive_mode, glw::GLint n_patch_vertices, const float* inner_tess_level,
	const float* outer_tess_level, _tessellation_test_type test_type)
{
	test.n_patch_vertices = n_patch_vertices;
	test.primitive_mode   = primitive_mode;
	test.type			  = test_type;
	test.vertex_spacing   = vertex_spacing;

	memcpy(test.tess_level_inner, inner_tess_level, sizeof(float) * 2 /* components */);
	memcpy(test.tess_level_outer, outer_tess_level, sizeof(float) * 4 /* components */);

	/* Set up a program object for the descriptor */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	test.po_id = gl.createProgram();
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateProgram() failed");

	/* Set up a pass-specific tessellation shader objects. */
	if (test_type == TESSELLATION_TEST_TYPE_TCS_TES)
	{
		test.tc_id = gl.createShader(m_glExtTokens.TESS_CONTROL_SHADER);
	}

	test.te_id = gl.createShader(m_glExtTokens.TESS_EVALUATION_SHADER);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateShader() call(s) failed");

	/* Configure tessellation control shader body */
	if (test.tc_id != 0)
	{
		std::string tc_body			= getTCCode(n_patch_vertices);
		const char* tc_body_raw_ptr = tc_body.c_str();

		shaderSourceSpecialized(test.tc_id, 1 /* count */, &tc_body_raw_ptr);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glShaderSource() failed for tessellation control shader");
	}

	/* Configure tessellation evaluation shader body */
	std::string te_body			= getTECode(vertex_spacing, primitive_mode);
	const char* te_body_raw_ptr = te_body.c_str();

	shaderSourceSpecialized(test.te_id, 1 /* count */, &te_body_raw_ptr);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glShaderSource() failed for tessellation evaluation shader");

	/* Compile the tessellation evaluation shader */
	glw::GLint		   compile_status = GL_FALSE;
	glw::GLuint		   shaders[]	  = { test.tc_id, test.te_id };
	const unsigned int n_shaders	  = sizeof(shaders) / sizeof(shaders[0]);

	for (unsigned int n_shader = 0; n_shader < n_shaders; ++n_shader)
	{
		glw::GLuint shader = shaders[n_shader];

		if (shader != 0)
		{
			gl.compileShader(shader);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glCompileShader() failed for tessellation shader");

			gl.getShaderiv(shader, GL_COMPILE_STATUS, &compile_status);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glGetShaderiv() failed for tessellation shader");

			if (compile_status != GL_TRUE)
			{
				TCU_FAIL("Tessellation shader compilation failed");
			}
		}
	} /* for (all shaders) */

	/* Attach all shader to the program object */
	gl.attachShader(test.po_id, m_fs_id);
	gl.attachShader(test.po_id, test.te_id);
	gl.attachShader(test.po_id, m_vs_id);

	if (test.tc_id != 0)
	{
		gl.attachShader(test.po_id, test.tc_id);
	}

	GLU_EXPECT_NO_ERROR(gl.getError(), "glAttachShader() failed");

	/* Set up XFB */
	const char*		   varyings[] = { "result_uvw" };
	const unsigned int n_varyings = sizeof(varyings) / sizeof(varyings[0]);

	gl.transformFeedbackVaryings(test.po_id, n_varyings, varyings, GL_INTERLEAVED_ATTRIBS);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTransformFeedbackVaryings() failed");

	/* Link the program object */
	glw::GLint link_status = GL_FALSE;

	gl.linkProgram(test.po_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glLinkProgram() failed");

	gl.getProgramiv(test.po_id, GL_LINK_STATUS, &link_status);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetProgramiv() failed");

	if (link_status != GL_TRUE)
	{
		TCU_FAIL("Program linking failed");
	}

	/* If TCS stage is present, set up the corresponding uniforms as needed */
	if (test.type == TESSELLATION_TEST_TYPE_TCS_TES)
	{
		test.inner_tess_level_uniform_location = gl.getUniformLocation(test.po_id, "inner_tess_level");
		test.outer_tess_level_uniform_location = gl.getUniformLocation(test.po_id, "outer_tess_level");

		DE_ASSERT(test.inner_tess_level_uniform_location != -1);
		DE_ASSERT(test.outer_tess_level_uniform_location != -1);

		/* Now that we have the locations, let's configure the uniforms */
		gl.useProgram(test.po_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram() call failed");

		gl.uniform2fv(test.inner_tess_level_uniform_location, 1, /* count */
					  test.tess_level_inner);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glUniform2fv() call failed");

		gl.uniform4fv(test.outer_tess_level_uniform_location, 1, /* count */
					  test.tess_level_outer);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glUniform4fv() call failed");
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
tcu::TestNode::IterateResult TessellationShaderTessellationgl_TessCoord::iterate(void)
{
	/* Do not execute if required extensions are not supported. */
	if (!m_is_geometry_shader_extension_supported || !m_is_tessellation_shader_supported)
	{
		throw tcu::NotSupportedError(TESSELLATION_SHADER_EXTENSION_NOT_SUPPORTED);
	}

	/* On ES skip test configurations that don't have TCS. */
	if (isContextTypeES(m_context.getRenderContext().getType()) && (m_test_type == TESSELLATION_TEST_TYPE_TES))
	{
		throw tcu::NotSupportedError("Implementation requires TCS and TES be used together; skipping.");
	}

	/* Initialize ES test objects */
	initTest();

	/* Initialize tessellation shader utilities */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	m_utils_ptr = new TessellationShaderUtils(gl, this);

	/* We don't need rasterization for this test */
	gl.enable(GL_RASTERIZER_DISCARD);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glEnable(GL_RASTERIZER_DISCARD) failed.");

	/* Iterate through all tests configured */
	for (_tests_const_iterator test_iterator = m_tests.begin(); test_iterator != m_tests.end(); test_iterator++)
	{
		const _test_descriptor& test = *test_iterator;

		if (!glu::isContextTypeES(m_context.getRenderContext().getType()))
		{
			/* If there is no TCS defined, define inner/outer tessellation levels */
			if (test.type == TESSELLATION_TEST_TYPE_TES)
			{
				gl.patchParameterfv(GL_PATCH_DEFAULT_INNER_LEVEL, test.tess_level_inner);
				GLU_EXPECT_NO_ERROR(gl.getError(),
									"glPatchParameterfv() failed for GL_PATCH_DEFAULT_INNER_LEVEL pname");

				gl.patchParameterfv(GL_PATCH_DEFAULT_OUTER_LEVEL, test.tess_level_outer);
				GLU_EXPECT_NO_ERROR(gl.getError(),
									"glPatchParameterfv() failed for GL_PATCH_DEFAULT_OUTER_LEVEL pname");
			}
		}

		/* Configure amount of vertices per patch */
		gl.patchParameteri(m_glExtTokens.PATCH_VERTICES, test.n_patch_vertices);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glPatchParameteriEXT() failed for GL_PATCH_VERTICES_EXT pname");

		/* Set up XFB target BO storage size. We will be capturing a total of 12 FP components per
		 * result vertex.
		 */
		unsigned int n_bytes_needed	= 0;
		unsigned int n_result_vertices = 0;

		n_result_vertices = m_utils_ptr->getAmountOfVerticesGeneratedByTessellator(
			test.primitive_mode, test.tess_level_inner, test.tess_level_outer, test.vertex_spacing, false);
		n_bytes_needed = static_cast<unsigned int>(n_result_vertices * sizeof(float) * 12 /* components */);

		gl.bufferData(GL_TRANSFORM_FEEDBACK_BUFFER, n_bytes_needed, NULL, /* data */
					  GL_STATIC_DRAW);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferData() failed");

		/* Activate the program object */
		gl.useProgram(test.po_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram() failed");

		/* Draw the test geometry. */
		glw::GLenum tf_mode = TessellationShaderUtils::getTFModeForPrimitiveMode(test.primitive_mode, false);

		gl.beginTransformFeedback(tf_mode);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBeginTransformFeedback(GL_PATCHES_EXT) failed.");

		gl.drawArrays(m_glExtTokens.PATCHES, 0 /* first */, test.n_patch_vertices);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawArrays() failed");

		gl.endTransformFeedback();
		GLU_EXPECT_NO_ERROR(gl.getError(), "glEndTransformFeedback() failed");

		/* Map the BO with result data into user space */
		const float* vertex_data = (const float*)gl.mapBufferRange(GL_TRANSFORM_FEEDBACK_BUFFER, 0, /* offset */
																   n_bytes_needed, GL_MAP_READ_BIT);

		GLU_EXPECT_NO_ERROR(gl.getError(), "glMapBufferRange() call failed");

		/* (test 1): Make sure that u+v+w == 1 (applicable for triangles only) */
		const float epsilon = 1e-5f;

		if (test.primitive_mode == TESSELLATION_SHADER_PRIMITIVE_MODE_TRIANGLES)
		{
			for (unsigned int n_vertex = 0; n_vertex < n_result_vertices; ++n_vertex)
			{
				const float* vertex_uvw = vertex_data + 3 /* components */ * n_vertex;
				float		 sum_uvw	= vertex_uvw[0] + vertex_uvw[1] + vertex_uvw[2];

				if (de::abs(sum_uvw - 1.0f) > epsilon)
				{
					m_testCtx.getLog() << tcu::TestLog::Message << "For triangles, U+V+W coordinates outputted "
																   "by tessellator should sum up to 1.0. Instead, the "
																   "following coordinates:"
									   << " (" << vertex_uvw[0] << ", " << vertex_uvw[1] << ", " << vertex_uvw[2]
									   << ") "
										  "sum up to "
									   << sum_uvw << "." << tcu::TestLog::EndMessage;

					TCU_FAIL("U+V+W coordinates do not add up to 1, even though triangle/tessellation"
							 " was requested");
				}
			} /* for (all vertices) */
		}	 /* if (we're dealing with triangles or quads) */

		/* (test 2): Make sure that u, v, w e <0, 1> (always applicable) */
		for (unsigned int n_vertex = 0; n_vertex < n_result_vertices; ++n_vertex)
		{
			const float* vertex_uvw = vertex_data + 3 /* components */ * n_vertex;

			if (!(vertex_uvw[0] >= 0.0f && vertex_uvw[0] <= 1.0f && vertex_uvw[1] >= 0.0f && vertex_uvw[1] <= 1.0f &&
				  vertex_uvw[2] >= 0.0f && vertex_uvw[2] <= 1.0f))
			{
				m_testCtx.getLog() << tcu::TestLog::Message
								   << "U, V and W coordinates outputted by the tessellator should "
									  "be within <0, 1> range. However, "
								   << "vertex at index: " << n_vertex << "is defined by the following triple:"
								   << " (" << vertex_uvw[0] << ", " << vertex_uvw[1] << ", " << vertex_uvw[2] << ")."
								   << tcu::TestLog::EndMessage;

				TCU_FAIL("U/V/W coordinate outputted by the tessellator is outside allowed range.");
			}
		} /* for (all vertices) */

		/* (test 3): Make sure w is always zero (applicable to quads and isolines) */
		if (test.primitive_mode == TESSELLATION_SHADER_PRIMITIVE_MODE_QUADS ||
			test.primitive_mode == TESSELLATION_SHADER_PRIMITIVE_MODE_ISOLINES)
		{
			for (unsigned int n_vertex = 0; n_vertex < n_result_vertices; ++n_vertex)
			{
				const float* vertex_uvw = vertex_data + 3 /* components */ * n_vertex;

				if (de::abs(vertex_uvw[2]) > epsilon)
				{
					m_testCtx.getLog() << tcu::TestLog::Message
									   << "W coordinate should be zero for all vertices outputted "
										  "for isolines and quads; for at least one vertex, W was "
										  "found to be equal to: "
									   << vertex_uvw[2] << tcu::TestLog::EndMessage;

					TCU_FAIL("W coordinate was found to be non-zero for at least one tessellation coordinate"
							 " generated in either quads or isolines primitive mode");
				}
			} /* for (all vertices) */
		}	 /* if (we're dealing with quads or isolines) */

		/* Unmap the BO */
		gl.unmapBuffer(GL_TRANSFORM_FEEDBACK_BUFFER);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glUnmapBuffer() call failed");
	}

	/* All done */
	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	return STOP;
}

/** Constructor
 *
 * @param context     Test context
 * @param name        Test case's name
 * @param description Test case's desricption
 **/
TessellationShaderTessellationMaxInOut::TessellationShaderTessellationMaxInOut(Context&				context,
																			   const ExtParameters& extParams)
	: TestCaseBase(context, extParams, "max_in_out_attributes",
				   "Make sure it is possible to use up GL_MAX_TESS_*_COMPONENTS_EXT.")
	, m_po_id_1(0)
	, m_po_id_2(0)
	, m_fs_id(0)
	, m_tcs_id_1(0)
	, m_tcs_id_2(0)
	, m_tes_id_1(0)
	, m_tes_id_2(0)
	, m_vs_id_1(0)
	, m_vs_id_2(0)
	, m_tf_bo_id_1(0)
	, m_tf_bo_id_2(0)
	, m_patch_data_bo_id(0)
	, m_vao_id(0)
	, m_gl_max_tess_control_input_components_value(0)
	, m_gl_max_tess_control_output_components_value(0)
	, m_gl_max_tess_evaluation_input_components_value(0)
	, m_gl_max_tess_evaluation_output_components_value(0)
	, m_gl_max_transform_feedback_interleaved_components_value(0)
	, m_gl_max_tess_patch_components_value(0)
	, m_gl_max_vertex_output_components_value(0)
	, m_ref_vertex_attributes(DE_NULL)
	, m_tf_varyings_names(DE_NULL)
{
	m_ref_patch_attributes[0] = 0.0f;
	m_ref_patch_attributes[1] = 0.0f;
	m_ref_patch_attributes[2] = 0.0f;
	m_ref_patch_attributes[3] = 0.0f;
}

/** Deinitializes all ES objects created for the test. */
void TessellationShaderTessellationMaxInOut::deinit(void)
{
	/* Call base class deinitialization routine */
	TestCaseBase::deinit();

	if (!m_is_tessellation_shader_supported)
	{
		return;
	}

	/* Deallocate dynamic arrays */
	if (m_ref_vertex_attributes != DE_NULL)
	{
		free(m_ref_vertex_attributes);

		m_ref_vertex_attributes = DE_NULL;
	}

	/* Deallocate the varyings array */
	if (m_tf_varyings_names != DE_NULL)
	{
		for (int i = 0; i < (m_gl_max_tess_evaluation_output_components_value) / 4 - 1 /* gl_Position */; i++)
		{
			if (m_tf_varyings_names[i] != DE_NULL)
			{
				free(m_tf_varyings_names[i]);

				m_tf_varyings_names[i] = DE_NULL;
			}
		}

		free(m_tf_varyings_names);

		m_tf_varyings_names = DE_NULL;
	}

	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Reset GL_PATCH_VERTICES_EXT pname value */
	gl.patchParameteri(m_glExtTokens.PATCH_VERTICES, 3);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glPatchParameteriEXT() failed!");

	/* Unbind vertex array object */
	gl.bindVertexArray(0);

	/* Release ES objects */
	if (m_fs_id != 0)
	{
		gl.deleteShader(m_fs_id);

		m_fs_id = 0;
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

	if (m_tcs_id_1 != 0)
	{
		gl.deleteShader(m_tcs_id_1);

		m_tcs_id_1 = 0;
	}

	if (m_tcs_id_2 != 0)
	{
		gl.deleteShader(m_tcs_id_2);

		m_tcs_id_2 = 0;
	}

	if (m_tes_id_1 != 0)
	{
		gl.deleteShader(m_tes_id_1);

		m_tes_id_1 = 0;
	}

	if (m_tes_id_2 != 0)
	{
		gl.deleteShader(m_tes_id_2);

		m_tes_id_2 = 0;
	}

	if (m_vs_id_1 != 0)
	{
		gl.deleteShader(m_vs_id_1);

		m_vs_id_1 = 0;
	}

	if (m_vs_id_2 != 0)
	{
		gl.deleteShader(m_vs_id_2);

		m_vs_id_2 = 0;
	}

	if (m_tf_bo_id_1 != 0)
	{
		gl.deleteBuffers(1, &m_tf_bo_id_1);

		m_tf_bo_id_1 = 0;
	}

	if (m_tf_bo_id_2 != 0)
	{
		gl.deleteBuffers(1, &m_tf_bo_id_2);

		m_tf_bo_id_2 = 0;
	}

	if (m_patch_data_bo_id != 0)
	{
		gl.deleteBuffers(1, &m_patch_data_bo_id);

		m_patch_data_bo_id = 0;
	}

	if (m_vao_id != 0)
	{
		gl.deleteVertexArrays(1, &m_vao_id);

		m_vao_id = 0;
	}
}

/** Executes the test.
 *
 *  Sets the test result to QP_TEST_RESULT_FAIL if the test failed, QP_TEST_RESULT_PASS otherwise.
 *
 *  Note the function throws exception should an error occur!
 *
 *  @return STOP if the test has finished, CONTINUE to indicate iterate should be called once again.
 **/
tcu::TestNode::IterateResult TessellationShaderTessellationMaxInOut::iterate(void)
{
	/* Retrieve ES entry-points. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Initialize test-specific ES objects. */
	initTest();

	/* Execute test case 1 */
	gl.useProgram(m_po_id_1);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram() failed!");

	gl.bindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, /* index */
					  m_tf_bo_id_1);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBufferBase() failed!");

	gl.beginTransformFeedback(GL_POINTS);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBeginTransformFeedback() failed");

	gl.drawArrays(m_glExtTokens.PATCHES, 0, /* first */
				  2);						/* count */
	GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawArrays() failed");

	gl.endTransformFeedback();
	GLU_EXPECT_NO_ERROR(gl.getError(), "glEndTransformFeedback() failed");

	/* Verify the rendered data. */
	bool test_passed = true;

	test_passed &= compareValues("Per-vertex components test ", m_ref_vertex_attributes,
								 m_gl_max_tess_evaluation_output_components_value / 4);

	/* Execute test case 2 */
	gl.useProgram(m_po_id_2);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram() failed!");

	gl.bindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, /* index */
					  m_tf_bo_id_2);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBufferBase() failed!");

	gl.beginTransformFeedback(GL_POINTS);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBeginTransformFeedback() failed");

	gl.drawArrays(m_glExtTokens.PATCHES, 0, /* first */
				  2);						/* count */
	GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawArrays() failed");

	gl.endTransformFeedback();
	GLU_EXPECT_NO_ERROR(gl.getError(), "glEndTransformFeedback() failed");

	/* Verify the rendered data */
	test_passed &=
		compareValues("Per-patch components test ", m_ref_patch_attributes, 1 /* amount of output vectors */);

	/* Test passed. */
	if (test_passed)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
	}

	return STOP;
}

/** Sets up buffer objects.
 *
 *   Note the function throws exception should an error occur!
 **/
void TessellationShaderTessellationMaxInOut::initBufferObjects(void)
{
	/* Retrieve ES entry-points. */
	glw::GLint			  bo_size = 0;
	const glw::Functions& gl	  = m_context.getRenderContext().getFunctions();

	/* Transform feedback buffer object for case 1 */
	gl.genBuffers(1, &m_tf_bo_id_1);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenBuffers() failed!");

	gl.bindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, m_tf_bo_id_1);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer() failed!");

	bo_size = static_cast<glw::GLint>(2														 /* vertices       */
									  * 4													 /* components     */
									  * m_gl_max_tess_evaluation_output_components_value / 4 /* attributes     */
									  * sizeof(glw::GLfloat));								 /* attribute size */

	gl.bufferData(GL_TRANSFORM_FEEDBACK_BUFFER, bo_size, NULL, GL_STATIC_DRAW);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferData() failed");

	/* Transform feedback buffer object for case 2 */
	bo_size = 2 *						/* vertices */
			  sizeof(glw::GLfloat) * 4; /* components */

	gl.genBuffers(1, &m_tf_bo_id_2);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenBuffers() failed!");

	gl.bindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, m_tf_bo_id_2);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer() failed!");

	gl.bufferData(GL_TRANSFORM_FEEDBACK_BUFFER, bo_size, NULL, GL_STATIC_DRAW);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferData() failed");

	/* Set up vertex data buffer storage */
	glw::GLfloat vertices[2 /* vertices */ * 4 /* components */];

	bo_size		= sizeof(vertices);
	vertices[0] = 0.f;
	vertices[1] = 0.f;
	vertices[2] = 0.f;
	vertices[3] = 1.f;
	vertices[4] = 1.f;
	vertices[5] = 1.f;
	vertices[6] = 1.f;
	vertices[7] = 1.f;

	gl.genBuffers(1, &m_patch_data_bo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenBuffers() failed!");

	gl.bindBuffer(GL_ARRAY_BUFFER, m_patch_data_bo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer() failed!");

	gl.bufferData(GL_ARRAY_BUFFER, bo_size, vertices, GL_STATIC_DRAW);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferData() failed!");

	gl.enableVertexAttribArray(0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glEnableVertexAttribArray() failed!");

	gl.vertexAttribPointer(0,				   /* index */
						   4,				   /* size */
						   GL_FLOAT, GL_FALSE, /* normalized */
						   0,				   /* stride */
						   0);				   /* pointer */
	GLU_EXPECT_NO_ERROR(gl.getError(), "glVertexAttribPointer() failed!");
}

/** Initializes the test.
 *
 *  Note the function throws exception should an error occur!
 **/
void TessellationShaderTessellationMaxInOut::initProgramObjects(void)
{
	/* Retrieve ES entry-points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Create program objects. */
	m_po_id_1 = gl.createProgram();
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateProgram() failed!");

	m_po_id_2 = gl.createProgram();
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateProgram() failed!");

	/* Set up all the shader objects that will be used for the test */
	m_vs_id_1 = gl.createShader(GL_VERTEX_SHADER);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateShader(GL_VERTEX_SHADER) failed!");

	m_vs_id_2 = gl.createShader(GL_VERTEX_SHADER);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateShader(GL_VERTEX_SHADER) failed!");

	m_tcs_id_1 = gl.createShader(m_glExtTokens.TESS_CONTROL_SHADER);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateShader(GL_TESS_CONTROL_SHADER_EXT) failed!");

	m_tcs_id_2 = gl.createShader(m_glExtTokens.TESS_CONTROL_SHADER);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateShader(GL_TESS_CONTROL_SHADER_EXT) failed!");

	m_tes_id_1 = gl.createShader(m_glExtTokens.TESS_EVALUATION_SHADER);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateShader(GL_TESS_EVALUATION_SHADER_EXT) failed!");

	m_tes_id_2 = gl.createShader(m_glExtTokens.TESS_EVALUATION_SHADER);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateShader(GL_TESS_EVALUATION_SHADER_EXT) failed!");

	m_fs_id = gl.createShader(GL_FRAGMENT_SHADER);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateShader(GL_FRAGMENT_SHADER) failed!");

	/* Transform Feedback setup for case 1
	 *
	 * Varyings array: m_tf_varyings_names[i <  m_gl_max_tess_evaluation_output_components_value / 4-1] == "Vertex.value[i]"
	 *                 m_tf_varyings_names[i == m_gl_max_tess_evaluation_output_components_value / 4-1] == "gl_Position"
	 */
	const char position_varying[] = "gl_Position";

	m_tf_varyings_names = (char**)malloc((m_gl_max_tess_evaluation_output_components_value / 4) * sizeof(char*));

	if (m_tf_varyings_names == DE_NULL)
	{
		throw tcu::ResourceError("Unable to allocate memory!");
	}

	for (int i = 0; i < (m_gl_max_tess_evaluation_output_components_value) / 4 /* attributes */ - 1 /* gl_Position */;
		 i++)
	{
		std::stringstream tf_varying_stream;
		const char*		  tf_varying_raw_ptr = DE_NULL;
		std::string		  tf_varying_string;

		tf_varying_stream << "Vertex.value[" << i << "]";
		tf_varying_string  = tf_varying_stream.str();
		tf_varying_raw_ptr = tf_varying_string.c_str();

		m_tf_varyings_names[i] = (char*)malloc(strlen(tf_varying_raw_ptr) + 1 /* '\0' */);
		if (m_tf_varyings_names[i] == DE_NULL)
		{
			throw tcu::ResourceError("Unable to allocate memory!");
		}

		memcpy(m_tf_varyings_names[i], tf_varying_raw_ptr, strlen(tf_varying_raw_ptr) + 1);
	}

	m_tf_varyings_names[m_gl_max_tess_evaluation_output_components_value / 4 - 1 /* gl_Position */] =
		(char*)malloc(sizeof(position_varying));
	if (m_tf_varyings_names[m_gl_max_tess_evaluation_output_components_value / 4 - 1 /* gl_Position */] == DE_NULL)
	{
		throw tcu::ResourceError("Unable to allocate memory!");
	}

	memcpy(m_tf_varyings_names[m_gl_max_tess_evaluation_output_components_value / 4 - 1 /* gl_Position */],
		   position_varying, sizeof(position_varying));

	/* Set up XFB */
	gl.transformFeedbackVaryings(m_po_id_1, m_gl_max_tess_evaluation_output_components_value / 4, m_tf_varyings_names,
								 GL_INTERLEAVED_ATTRIBS);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTransformFeedbackVaryings() failed!");

	/* Set up program objects */
	const char* vs_code_raw_ptr	= m_vs_code;
	const char* tcs_code_1_raw_ptr = m_tcs_code_1;
	const char* tes_code_1_raw_ptr = m_tes_code_1;
	const char* tcs_code_2_raw_ptr = m_tcs_code_2;
	const char* tes_code_2_raw_ptr = m_tes_code_2;

	/* Build a program object to test case 1. */
	if (!TessellationShaderTessellationMaxInOut::buildProgram(m_po_id_1, m_vs_id_1, 1, &vs_code_raw_ptr, m_tcs_id_1, 1,
															  &tcs_code_1_raw_ptr, m_tes_id_1, 1, &tes_code_1_raw_ptr,
															  m_fs_id, 1, &m_fs_code))
	{
		TCU_FAIL("Could not build first test program object");
	}

	/* Tranform Feedback setup for case 2 */
	const char* const tf_varying_2 = "out_value";

	gl.transformFeedbackVaryings(m_po_id_2, 1 /* count */, &tf_varying_2, GL_INTERLEAVED_ATTRIBS);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTransformFeedbackVaryings() failed!");

	/* Build a program object for case 2 */
	if (!(TessellationShaderTessellationMaxInOut::buildProgram(m_po_id_2, m_vs_id_2, 1, &vs_code_raw_ptr, m_tcs_id_2, 1,
															   &tcs_code_2_raw_ptr, m_tes_id_2, 1, &tes_code_2_raw_ptr,
															   m_fs_id, 1, &m_fs_code)))
	{
		TCU_FAIL("Could not link second test program object");
	}
}

/** Initializes the test.
 *
 *  Note the function throws exception should an error occur!
 **/
void TessellationShaderTessellationMaxInOut::initTest(void)
{
	/* Render state setup */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Skip if required extensions are not supported. */
	if (!m_is_tessellation_shader_supported)
	{
		throw tcu::NotSupportedError(TESSELLATION_SHADER_EXTENSION_NOT_SUPPORTED);
	}

	/* Initialize vertex array object */
	gl.genVertexArrays(1, &m_vao_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not generate vertex array object");

	gl.bindVertexArray(m_vao_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error binding vertex array object!");

	/* All tessellation control shaders used by this test assume two
	 * vertices are going to be provided per input patch. */
	gl.patchParameteri(m_glExtTokens.PATCH_VERTICES, 2);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glPatchParameteriEXT() failed!");

	/* Carry on with initialization */
	retrieveGLConstantValues();
	initProgramObjects();
	initBufferObjects();
	initReferenceValues();

	gl.enable(GL_RASTERIZER_DISCARD);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glEnable(RASTERIZER_DISCARD) failed!");
}

/** Initializes reference values that will be compared against
 *  values generated by the program object.
 *  Fills m_ref_vertex_attributes and m_ref_patch_attributes arrays.
 *  These arrays are later used by compareValues() function.
 **/
void TessellationShaderTessellationMaxInOut::initReferenceValues(void)
{
	/* Allocate vertex attribute array data needed for reference values preparation. */
	int max_array_size = de::max(m_gl_max_tess_control_input_components_value,
								 de::max(m_gl_max_tess_control_output_components_value,
										 de::max(m_gl_max_tess_evaluation_input_components_value,
												 m_gl_max_tess_evaluation_output_components_value)));

	m_ref_vertex_attributes = (glw::GLfloat*)malloc(sizeof(glw::GLfloat) * (max_array_size));
	if (m_ref_vertex_attributes == DE_NULL)
	{
		throw tcu::ResourceError("Unable to allocate memory!");
	}

	/* We need to create an array consisting of gl_max_tess_evaluation_output_components items.
	 * The array will be filled with the following values:
	 *
	 * reference_value[0],
	 * (...)
	 * reference_value[gl_max_tess_evaluation_output_components / 4 - 2],
	 * reference_gl_Position
	 *
	 * which corresponds to output block defined for Tessellation Evaluation Stage:
	 *
	 * out Vertex
	 * {
	 *     vec4 value[(gl_MaxTessControlInputComponents) / 4 - 1];
	 * } outVertex;
	 *
	 * + gl_Position.
	 */
	glw::GLfloat sumInTCS[] = { 0.0f, 0.0f, 0.0f, 0.0f };
	glw::GLfloat sumInTES[] = { 0.0f, 0.0f, 0.0f, 0.0f };

	for (int i = 0; i < m_gl_max_tess_control_input_components_value - 4; /* gl_Position */
		 i++)
	{
		m_ref_vertex_attributes[i] = (glw::GLfloat)i;
	}

	for (int i = 0; i < m_gl_max_tess_control_input_components_value - 4; /* gl_Position */
		 i++)
	{
		sumInTCS[i % 4 /* component selector */] += m_ref_vertex_attributes[i];
	}

	for (int i = 0; i < m_gl_max_tess_control_output_components_value - 4; /* gl_Position */
		 i++)
	{
		m_ref_vertex_attributes[i] = sumInTCS[i % 4] + (glw::GLfloat)i;
	}

	for (int i = m_gl_max_tess_control_input_components_value - 4; /* gl_Position */
		 i < m_gl_max_tess_control_output_components_value - 4;	/* gl_Position */
		 i++)
	{
		m_ref_vertex_attributes[i] = (glw::GLfloat)i;
	}

	for (int i = 0; i < m_gl_max_tess_evaluation_input_components_value - 4; /* gl_Position */
		 i++)
	{
		sumInTES[i % 4 /* component selector */] += m_ref_vertex_attributes[i];
	}

	for (int i = 0; i < m_gl_max_tess_evaluation_output_components_value - 4; /* gl_Position */
		 i++)
	{
		m_ref_vertex_attributes[i] = sumInTES[i % 4 /* component selector */] + (glw::GLfloat)i;
	}

	for (int i = m_gl_max_tess_evaluation_input_components_value - 4; /* gl_Position */
		 i < m_gl_max_tess_evaluation_output_components_value - 4;	/* gl_Position */
		 i++)
	{
		m_ref_vertex_attributes[i] = (glw::GLfloat)i;
	}

	/* Store gl_Position reference values (only first vertex will be compared) */
	m_ref_vertex_attributes[m_gl_max_tess_evaluation_output_components_value - 4] = 0.0f;
	m_ref_vertex_attributes[m_gl_max_tess_evaluation_output_components_value - 3] = 0.0f;
	m_ref_vertex_attributes[m_gl_max_tess_evaluation_output_components_value - 2] = 0.0f;
	m_ref_vertex_attributes[m_gl_max_tess_evaluation_output_components_value - 1] = 1.0f;

	/* Set up reference data for case 2
	 *
	 * Only one output vector will be needed for comparison.
	 */
	m_ref_patch_attributes[0] = 0.0f;
	m_ref_patch_attributes[1] = 0.0f;
	m_ref_patch_attributes[2] = 0.0f;
	m_ref_patch_attributes[3] = 0.0f;

	for (int i = 0; i < m_gl_max_tess_patch_components_value; i++)
	{
		m_ref_patch_attributes[i % 4] += (glw::GLfloat)i;
	}
}

/** Retrieve OpenGL state and implementation values.
 *
 *  Note the function throws exception should an error occur!
 **/
void TessellationShaderTessellationMaxInOut::retrieveGLConstantValues(void)
{
	/* Retrieve ES entry-points. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Query implementation constants */
	gl.getIntegerv(GL_MAX_VERTEX_OUTPUT_COMPONENTS, &m_gl_max_vertex_output_components_value);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv() failed for GL_MAX_VERTEX_OUTPUT_COMPONENTS pname!");

	gl.getIntegerv(m_glExtTokens.MAX_TESS_CONTROL_INPUT_COMPONENTS, &m_gl_max_tess_control_input_components_value);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv() failed for GL_MAX_TESS_CONTROL_INPUT_COMPONENTS_EXT pname!");

	gl.getIntegerv(m_glExtTokens.MAX_TESS_CONTROL_OUTPUT_COMPONENTS, &m_gl_max_tess_control_output_components_value);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv() failed for GL_MAX_TESS_CONTROL_OUTPUT_COMPONENTS_EXT pname!");

	gl.getIntegerv(m_glExtTokens.MAX_TESS_PATCH_COMPONENTS, &m_gl_max_tess_patch_components_value);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv() failed for GL_MAX_TESS_PATCH_COMPONENTS_EXT pname!");

	gl.getIntegerv(m_glExtTokens.MAX_TESS_EVALUATION_INPUT_COMPONENTS,
				   &m_gl_max_tess_evaluation_input_components_value);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv() failed for GL_MAX_TESS_EVALUATION_INPUT_COMPONENTS_EXT pname!");

	gl.getIntegerv(m_glExtTokens.MAX_TESS_EVALUATION_OUTPUT_COMPONENTS,
				   &m_gl_max_tess_evaluation_output_components_value);
	GLU_EXPECT_NO_ERROR(gl.getError(),
						"glGetIntegerv() failed for GL_MAX_TESS_EVALUATION_OUTPUT_COMPONENTS_EXT pname!");

	gl.getIntegerv(GL_MAX_TRANSFORM_FEEDBACK_INTERLEAVED_COMPONENTS,
				   &m_gl_max_transform_feedback_interleaved_components_value);
	GLU_EXPECT_NO_ERROR(gl.getError(),
						"glGetIntegerv() failed for GL_MAX_TRANSFORM_FEEDBACK_INTERLEAVED_COMPONENTS pname!");

	/* Sanity checks */
	DE_ASSERT(m_gl_max_vertex_output_components_value != 0);
	DE_ASSERT(m_gl_max_tess_control_input_components_value != 0);
	DE_ASSERT(m_gl_max_tess_control_output_components_value != 0);
	DE_ASSERT(m_gl_max_tess_patch_components_value != 0);
	DE_ASSERT(m_gl_max_tess_evaluation_input_components_value != 0);
	DE_ASSERT(m_gl_max_tess_evaluation_output_components_value != 0);
	DE_ASSERT(m_gl_max_transform_feedback_interleaved_components_value != 0);

	/* Make sure it is possible to transfer all components through all the stages.
	 * If not, the test may fail, so we throw not supported. */
	if (m_gl_max_vertex_output_components_value < m_gl_max_tess_control_input_components_value)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Warning: GL_MAX_VERTEX_OUTPUT_COMPONENTS value:"
						   << m_gl_max_vertex_output_components_value
						   << " is less than GL_MAX_TESS_CONTROL_INPUT_COMPONENTS_EXT: "
						   << m_gl_max_tess_control_input_components_value
						   << ". It may not be possible to pass all GL_MAX_TESS_CONTROL_INPUT_COMPONENTS_EXT "
							  "per-vertex components from Vertex Shader to Tessellation Control Shader."
						   << tcu::TestLog::EndMessage;
		throw tcu::NotSupportedError("GL_MAX_VERTEX_OUTPUT_COMPONENTS < GL_MAX_TESS_CONTROL_INPUT_COMPONENTS");
	}

	if (m_gl_max_tess_control_output_components_value != m_gl_max_tess_evaluation_input_components_value)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Warning: GL_MAX_TESS_CONTROL_OUTPUT_COMPONENTS_EXT:"
						   << m_gl_max_tess_control_output_components_value
						   << " is not equal to GL_MAX_TESS_EVALUATION_INPUT_COMPONENTS_EXT:"
						   << m_gl_max_tess_evaluation_input_components_value
						   << ". It may not be possible to pass all GL_MAX_TESS_CONTROL_OUTPUT_COMPONENTS_EXT "
							  "per-vertex components from Tessellation Control Shader to Tessellation "
							  "Evaluation Shader."
						   << tcu::TestLog::EndMessage;
		throw tcu::NotSupportedError("GL_MAX_TESS_CONTROL_OUTPUT_COMPONENTS != "
									 "GL_MAX_TESS_EVALUATION_INPUT_COMPONENTS");
	}

	if (m_gl_max_tess_evaluation_output_components_value > m_gl_max_transform_feedback_interleaved_components_value)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Warning: GL_MAX_TESS_EVALUATION_OUTPUT_COMPONENTS_EXT:"
						   << m_gl_max_tess_evaluation_output_components_value
						   << " is greater than GL_MAX_TRANSFORM_FEEDBACK_INTERLEAVED_COMPONENTS:"
						   << m_gl_max_transform_feedback_interleaved_components_value
						   << ". It may not be possible to check all GL_MAX_TESS_EVALUATION_OUTPUT_COMPONENTS_EXT "
							  "per-vertex components from Tessellation Evaluation Shader."
						   << tcu::TestLog::EndMessage;
		throw tcu::NotSupportedError("GL_MAX_TESS_EVALUATION_OUTPUT_COMPONENTS > "
									 "GL_MAX_TRANSFORM_FEEDBACK_INTERLEAVED_COMPONENTS");
	}
}

/** Maps buffer object storage bound to GL_TRANSFORM_FEEDBACK_BUFFER binding point into process space
 *  and verifies the downloaded data matches the user-provided reference data.
 *
 *  Note the function throws exception should an error occur!
 *
 *  @param description        Case description;
 *  @param reference_values   Array storing reference data;
 *  @param n_reference_values Number of vec4s available for reading under @param reference_values;
 *
 *  @return false if the comparison failed, or true otherwise.
 **/
bool TessellationShaderTessellationMaxInOut::compareValues(char const* description, glw::GLfloat* reference_values,
														   int n_reference_values)
{
	/* Retrieve ES entry-points */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Map the buffer storage into process space. */
	glw::GLint bo_size = static_cast<glw::GLint>(2 /* number of vertices */ * sizeof(glw::GLfloat) *
												 n_reference_values * 4); /* number of components */
	glw::GLfloat* resultFloats =
		(glw::GLfloat*)gl.mapBufferRange(GL_TRANSFORM_FEEDBACK_BUFFER, 0, bo_size, GL_MAP_READ_BIT);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glMapBufferRange() failed");

	/* Verify the data */
	const glw::GLfloat epsilon	 = (glw::GLfloat)1e-5f;
	bool			   test_passed = true;

	for (int i = 0; i < n_reference_values * 4 /* number of components */; i += 4 /* number of components */)
	{
		if ((de::abs(resultFloats[i] - reference_values[i]) > epsilon) ||
			(de::abs(resultFloats[i + 1] - reference_values[i + 1]) > epsilon) ||
			(de::abs(resultFloats[i + 2] - reference_values[i + 2]) > epsilon) ||
			(de::abs(resultFloats[i + 3] - reference_values[i + 3]) > epsilon))
		{
			m_testCtx.getLog() << tcu::TestLog::Message << description << ": captured results "
							   << "vec4(" << resultFloats[i + 0] << ", " << resultFloats[i + 1] << ", "
							   << resultFloats[i + 2] << ", " << resultFloats[i + 3] << ") "
							   << "are different from the expected values "
							   << "vec4(" << reference_values[i + 0] << ", " << reference_values[i + 1] << ", "
							   << reference_values[i + 2] << ", " << reference_values[i + 3] << ")."
							   << tcu::TestLog::EndMessage;

			test_passed = false;
			break;
		}
	}

	gl.unmapBuffer(GL_TRANSFORM_FEEDBACK_BUFFER);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUnmapBuffer() failed!");

	return test_passed;
}

} /* namespace glcts */
