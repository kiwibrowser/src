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

#include "esextcTessellationShaderXFB.hpp"
#include "esextcTessellationShaderUtils.hpp"
#include "gluContextInfo.hpp"
#include "gluDefs.hpp"
#include "glwEnums.hpp"
#include "glwFunctions.hpp"
#include "tcuTestLog.hpp"

namespace glcts
{

/** Constructor
 *
 * @param context Test context
 **/
TessellationShaderXFB::TessellationShaderXFB(Context& context, const ExtParameters& extParams)
	: TestCaseBase(context, extParams, "xfb_captures_data_from_correct_stage",
				   "Verifies transform-feedback captures data from appropriate shader stage.")
	, m_bo_id(0)
	, m_fs_id(0)
	, m_gs_id(0)
	, m_po_id(0)
	, m_tc_id(0)
	, m_te_id(0)
	, m_vs_id(0)
	, m_pipeline_id(0)
	, m_fs_program_id(0)
	, m_gs_program_id(0)
	, m_tc_program_id(0)
	, m_te_program_id(0)
	, m_vs_program_id(0)
	, m_vao_id(0)
{
	/* Left blank on purpose */
}

/** Deinitializes ES objects created for the test. */
void TessellationShaderXFB::deinit()
{
	/* Call base class' deinit() */
	TestCaseBase::deinit();

	if (!m_is_tessellation_shader_supported)
	{
		return;
	}

	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Reset GL_PATCH_VERTICES_EXT value */
	gl.patchParameteri(m_glExtTokens.PATCH_VERTICES, 3);

	/* Disable any pipeline object that may still be active */
	gl.bindProgramPipeline(0);

	/* Reset TF buffer object bindings */
	gl.bindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, 0 /* buffer */);
	gl.bindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0 /* index */, 0 /* buffer */);

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

	if (m_fs_program_id != 0)
	{
		gl.deleteProgram(m_fs_program_id);

		m_fs_program_id = 0;
	}

	if (m_gs_id != 0)
	{
		gl.deleteShader(m_gs_id);

		m_gs_id = 0;
	}

	if (m_gs_program_id != 0)
	{
		gl.deleteProgram(m_gs_program_id);

		m_gs_program_id = 0;
	}

	if (m_pipeline_id != 0)
	{
		gl.deleteProgramPipelines(1, &m_pipeline_id);

		m_pipeline_id = 0;
	}

	if (m_po_id != 0)
	{
		gl.deleteProgram(m_po_id);

		m_po_id = 0;
	}

	if (m_tc_id != 0)
	{
		gl.deleteShader(m_tc_id);

		m_tc_id = 0;
	}

	if (m_tc_program_id != 0)
	{
		gl.deleteProgram(m_tc_program_id);

		m_tc_program_id = 0;
	}

	if (m_te_id != 0)
	{
		gl.deleteShader(m_te_id);

		m_te_id = 0;
	}

	if (m_te_program_id != 0)
	{
		gl.deleteProgram(m_te_program_id);

		m_te_program_id = 0;
	}

	if (m_vs_id != 0)
	{
		gl.deleteShader(m_vs_id);

		m_vs_id = 0;
	}

	if (m_vs_program_id != 0)
	{
		gl.deleteProgram(m_vs_program_id);

		m_vs_program_id = 0;
	}

	if (m_vao_id != 0)
	{
		gl.deleteVertexArrays(1, &m_vao_id);

		m_vao_id = 0;
	}
}

/** Create separable programs **/
glw::GLuint TessellationShaderXFB::createSeparableProgram(glw::GLenum shader_type, unsigned int n_strings,
														  const char* const* strings, unsigned int n_varyings,
														  const char* const* varyings, bool should_succeed)
{
	const glw::Functions& gl	= m_context.getRenderContext().getFunctions();
	glw::GLuint			  po_id = 0;
	glw::GLuint			  so_id = 0;

	/* Create a shader object */
	so_id = gl.createShader(shader_type);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateShader() failed");

	/* Create a program object */
	po_id = gl.createProgram();
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateProgram() failed");

	/* Mark the program object as separable */
	gl.programParameteri(po_id, GL_PROGRAM_SEPARABLE, GL_TRUE);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glProgramParameteri() call failed");

	/* Configure XFB for the program object */
	if (n_varyings != 0)
	{
		gl.transformFeedbackVaryings(po_id, n_varyings, varyings, GL_SEPARATE_ATTRIBS);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glTransformFeedbackVaryings() call failed");
	}

	bool build_success = buildProgram(po_id, so_id, n_strings, strings);

	/* Safe to delete the shader object at this point */
	gl.deleteShader(so_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glDeleteShader() call failed");

	if (!build_success)
	{
		gl.deleteProgram(po_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glDeleteProgram() call failed");
		po_id = 0;

		if (should_succeed)
		{
			TCU_FAIL("Separable program should have succeeded");
		}
	}
	else if (!should_succeed)
	{
		std::string shader_source = getShaderSource(so_id);
		m_testCtx.getLog() << tcu::TestLog::Message << "Shader source:\n\n"
						   << shader_source << "\n\n"
						   << tcu::TestLog::EndMessage;
		TCU_FAIL("Separable program should have failed");
	}

	return po_id;
}

/** Initializes ES objects necessary to run the test. */
void TessellationShaderXFB::initTest()
{
	/* Skip if required extensions are not supported. */
	if (!m_is_tessellation_shader_supported)
	{
		return;
	}

	/* Generate all objects needed for the test */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.genVertexArrays(1, &m_vao_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Could not generate vertex array object");

	gl.bindVertexArray(m_vao_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Error binding vertex array object!");

	gl.genBuffers(1, &m_bo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenBuffers() failed");

	m_fs_id = gl.createShader(GL_FRAGMENT_SHADER);
	m_tc_id = gl.createShader(m_glExtTokens.TESS_CONTROL_SHADER);
	m_te_id = gl.createShader(m_glExtTokens.TESS_EVALUATION_SHADER);
	m_vs_id = gl.createShader(GL_VERTEX_SHADER);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateShader() failed");

	m_po_id = gl.createProgram();
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateProgram() failed");

	if (m_is_geometry_shader_extension_supported)
	{
		m_gs_id = gl.createShader(m_glExtTokens.GEOMETRY_SHADER);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateShader() failed for GL_GEOMETRY_SHADER_EXT");
	}

	gl.genProgramPipelines(1, &m_pipeline_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenProgramPipelines() failed");

	/* Configure fragment shader body */
	const char* fs_body = "${VERSION}\n"
						  "\n"
						  "${SHADER_IO_BLOCKS_REQUIRE}\n"
						  "\n"
						  "precision highp float;\n"
						  "in BLOCK_INOUT { vec4 value; } user_in;\n"
						  "\n"
						  "void main()\n"
						  "{\n"
						  "}\n";

	shaderSourceSpecialized(m_fs_id, 1 /* count */, &fs_body);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glShaderSource() failed for fragment shader");

	/* Create a fragment shader program */
	glw::GLint		   link_status  = GL_FALSE;
	const glw::GLchar* varying_name = "BLOCK_INOUT.value";

	m_fs_program_id = createSeparableProgram(GL_FRAGMENT_SHADER, 1, /* n_strings */
											 &fs_body, 0,			/* n_varyings */
											 DE_NULL,				/* varyings */
											 true);					/* should_succeed */

	gl.getProgramiv(m_fs_program_id, GL_LINK_STATUS, &link_status);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetProgramiv() failed");

	if (link_status != GL_TRUE)
	{
		TCU_FAIL("Fragment shader program failed to link.");
	}

	/* Configure geometry shader body */
	const char* gs_body = "${VERSION}\n"
						  "\n"
						  "${GEOMETRY_SHADER_REQUIRE}\n"
						  "\n"
						  "layout(points)                   in;\n"
						  "layout(points, max_vertices = 1) out;\n"
						  "\n"
						  "precision highp float;\n"
						  "${IN_PER_VERTEX_DECL_ARRAY}"
						  "${OUT_PER_VERTEX_DECL}"
						  "in  BLOCK_INOUT { vec4 value; } user_in[];\n"
						  "out BLOCK_INOUT { vec4 value; } user_out;\n"
						  "\n"
						  "void main()\n"
						  "{\n"
						  "    user_out.value = vec4(1.0, 2.0, 3.0, 4.0);\n"
						  "    gl_Position    = vec4(0.0, 0.0, 0.0, 1.0);\n"
						  "\n"
						  "    EmitVertex();\n"
						  "}\n";

	if (m_is_geometry_shader_extension_supported)
	{
		shaderSourceSpecialized(m_gs_id, 1 /* count */, &gs_body);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glShaderSource() failed for geometry shader");

		/* Create a geometry shader program */
		m_gs_program_id = createSeparableProgram(m_glExtTokens.GEOMETRY_SHADER, 1, /* n_strings */
												 &gs_body, 1,					   /* n_varyings */
												 &varying_name, true);			   /* should_succeed */

		if (m_gs_program_id == 0)
		{
			TCU_FAIL("Could not create a separate geometry program object");
		}

		gl.getProgramiv(m_gs_program_id, GL_LINK_STATUS, &link_status);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGetProgramiv() failed");

		if (link_status != GL_TRUE)
		{
			TCU_FAIL("Geometry shader program failed to link.");
		}
	}

	/* Configure tessellation control shader body */
	const char* tc_body = "${VERSION}\n"
						  "\n"
						  "${TESSELLATION_SHADER_REQUIRE}\n"
						  "\n"
						  "layout (vertices=4) out;\n"
						  "\n"
						  "precision highp float;\n"
						  "${IN_PER_VERTEX_DECL_ARRAY}"
						  "${OUT_PER_VERTEX_DECL_ARRAY}"
						  "in  BLOCK_INOUT { vec4 value; } user_in[];\n"
						  "out BLOCK_INOUT { vec4 value; } user_out[];\n"
						  "\n"
						  "void main()\n"
						  "{\n"
						  "    gl_out   [gl_InvocationID].gl_Position = vec4(0.0, 0.0, 0.0, 1.0);\n"
						  "    user_out [gl_InvocationID].value       = vec4(2.0, 3.0, 4.0, 5.0);\n"
						  "\n"
						  "    gl_TessLevelOuter[0] = 1.0;\n"
						  "    gl_TessLevelOuter[1] = 1.0;\n"
						  "}\n";

	shaderSourceSpecialized(m_tc_id, 1 /* count */, &tc_body);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glShaderSource() failed for tessellation control shader");

	/* Test creating a tessellation control shader program with feedback.
	 * For Desktop, if GL_NV_gpu_shader5 is available this will succeed, and
	 * so we'll use it for our testing.
	 * For ES, and for Desktop implementations that don't have
	 * GL_NV_gpu_shader5, this will fail, and so we will create a different
	 * program without the feedback varyings that we can use for our testing.
	 * (We can safely ignore the return value for the expected failure case.
	 * In the event that the failure case incorrectly succeeds,
	 * createSeparableProgram will generate a test failure exception.)
	 */

	bool tc_feedback_valid;
	if (!glu::isContextTypeES(m_context.getRenderContext().getType()) && isExtensionSupported("GL_NV_gpu_shader5"))
	{
		tc_feedback_valid = true;
	}
	else
	{
		tc_feedback_valid = false;
	}

	/* Create a tessellation control shader program */
	m_tc_program_id = createSeparableProgram(m_glExtTokens.TESS_CONTROL_SHADER, 1, /* n_strings */
											 &tc_body, 1,						   /* n_varyings */
											 &varying_name,						   /* varyings */
											 tc_feedback_valid);				   /* should_succeed */

	if (!tc_feedback_valid)
	{
		/* Create a valid tessellation control shader program for ES */
		m_tc_program_id = createSeparableProgram(m_glExtTokens.TESS_CONTROL_SHADER, 1, /* n_strings */
												 &tc_body, 0,						   /* n_varyings */
												 DE_NULL,							   /* varyings */
												 true);								   /* should_succeed */
	}

	if (m_tc_program_id == 0)
	{
		TCU_FAIL("Could not create a separate tessellation control program object");
	}

	gl.getProgramiv(m_tc_program_id, GL_LINK_STATUS, &link_status);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetProgramiv() failed");

	if (link_status != GL_TRUE)
	{
		TCU_FAIL("Tessellation control shader program failed to link.");
	}

	/* Configure tessellation evaluation shader body */
	const char* te_body = "${VERSION}\n"
						  "\n"
						  "${TESSELLATION_SHADER_REQUIRE}\n"
						  "\n"
						  "layout (isolines, point_mode) in;\n"
						  "\n"
						  "precision highp float;\n"
						  "${IN_PER_VERTEX_DECL_ARRAY}"
						  "${OUT_PER_VERTEX_DECL}"
						  "in  BLOCK_INOUT { vec4 value; } user_in[];\n"
						  "out BLOCK_INOUT { vec4 value; } user_out;\n"
						  "\n"
						  "void main()\n"
						  "{\n"
						  "    gl_Position     = gl_in[0].gl_Position;\n"
						  "    user_out.value = vec4(3.0, 4.0, 5.0, 6.0);\n"
						  "}\n";

	shaderSourceSpecialized(m_te_id, 1 /* count */, &te_body);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glShaderSource() failed for tessellation evaluation shader");

	/* Create a tessellation evaluation shader program */
	m_te_program_id = createSeparableProgram(m_glExtTokens.TESS_EVALUATION_SHADER, 1, /* n_strings */
											 &te_body, 1,							  /* n_varyings */
											 &varying_name, true);					  /* should_succeed */

	if (m_te_program_id == 0)
	{
		TCU_FAIL("Could not create a separate tessellation evaluation program object");
	}

	gl.getProgramiv(m_te_program_id, GL_LINK_STATUS, &link_status);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetProgramiv() failed");

	if (link_status != GL_TRUE)
	{
		TCU_FAIL("Tessellation evaluation shader program failed to link.");
	}

	/* Configure vertex shader body */
	const char* vs_body = "${VERSION}\n"
						  "\n"
						  "${SHADER_IO_BLOCKS_REQUIRE}\n"
						  "\n"
						  "precision highp float;\n"
						  "${OUT_PER_VERTEX_DECL}"
						  "out BLOCK_INOUT { vec4 value; } user_out;\n"
						  "\n"
						  "void main()\n"
						  "{\n"
						  "    gl_Position    = vec4(1.0, 0.0, 0.0, 1.0);\n"
						  "    user_out.value = vec4(4.0, 5.0, 6.0, 7.0);\n"
						  "}\n";

	shaderSourceSpecialized(m_vs_id, 1 /* count */, &vs_body);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glShaderSource() failed for vertex shader");

	/* Configure vertex shader program */
	m_vs_program_id = createSeparableProgram(GL_VERTEX_SHADER, 1,  /* n_strings */
											 &vs_body, 1,		   /* n_varyings */
											 &varying_name, true); /* should_succeed */

	/* Compile all the shaders */
	const glw::GLuint shaders[] = { m_fs_id, (m_is_geometry_shader_extension_supported) ? m_gs_id : 0, m_tc_id, m_te_id,
									m_vs_id };
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
				const char* src[] = { fs_body, gs_body, tc_body, te_body, vs_body };

				m_testCtx.getLog() << tcu::TestLog::Message << "Compilation of shader object at index " << n_shader
								   << " failed.\n"
								   << "Info log:\n"
								   << getCompilationInfoLog(shader) << "Shader:\n"
								   << src[n_shader] << tcu::TestLog::EndMessage;

				TCU_FAIL("Shader compilation failed");
			}
		}
	} /* for (all shaders) */

	/* Attach fragment & vertex shaders to the program object */
	gl.attachShader(m_po_id, m_fs_id);
	gl.attachShader(m_po_id, m_vs_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glAttachShader() failed");

	/* Configure pipeline object's fragment & vertex stages */
	gl.useProgramStages(m_pipeline_id, GL_FRAGMENT_SHADER_BIT, m_fs_program_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgramStages call failed for fragment stage");

	gl.useProgramStages(m_pipeline_id, GL_VERTEX_SHADER_BIT, m_vs_program_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgramStages call failed for vertex stage");

	/* Set up XFB for conventional program object */
	gl.transformFeedbackVaryings(m_po_id, 1 /* count */, &varying_name, GL_SEPARATE_ATTRIBS);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTransformFeedbackVaryings() failed");

	/* Set up buffer object storage.
	 * We allocate enough space for a 4 vertex patch, which is the size
	 * needed by desktop GL for the tessellation control shader feedback
	 * whenever GL_NV_gpu_shader5 is present. */
	gl.bindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, m_bo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer() failed");

	gl.bufferData(GL_TRANSFORM_FEEDBACK_BUFFER, sizeof(float) * 4 /* components */ * 4 /* vertices per patch */,
				  NULL, /* data */
				  GL_STATIC_DRAW);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferData() failed");

	/* Bind the buffer object to indiced TF binding point */
	gl.bindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0 /* index */, m_bo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBufferBase() failed");
}

/** Executes the test.
 *
 *  Sets the test result to QP_TEST_RESULT_FAIL if the test failed, QP_TEST_RESULT_PASS otherwise.
 *
 *  Note the function throws exception should an error occur!
 *
 *  @return STOP if the test has finished, CONTINUE to indicate iterate() should be called once again.
 **/
tcu::TestNode::IterateResult TessellationShaderXFB::iterate(void)
{
	/* Do not execute if required extensions are not supported. */
	if (!m_is_tessellation_shader_supported)
	{
		throw tcu::NotSupportedError(TESSELLATION_SHADER_EXTENSION_NOT_SUPPORTED);
	}

	typedef std::vector<_test_descriptor> _tests;
	typedef _tests::const_iterator		  _tests_const_iterator;

	/* Initialize ES test objects */
	initTest();

	/* Describe test iterations */
	_test_descriptor test_1; /* vs+tc+te+gs */
	_test_descriptor test_2; /* vs+tc+te */
	_test_descriptor test_3; /* vs+tc */
	_tests			 tests;

	if (m_is_geometry_shader_extension_supported)
	{
		test_1.expected_data_source  = m_glExtTokens.GEOMETRY_SHADER;
		test_1.expected_n_values	 = 2;
		test_1.should_draw_call_fail = false;
		test_1.requires_pipeline	 = false;
		test_1.tf_mode				 = GL_POINTS;
		test_1.use_gs				 = true;
		test_1.use_tc				 = true;
		test_1.use_te				 = true;

		tests.push_back(test_1);
	}

	test_2.expected_data_source  = m_glExtTokens.TESS_EVALUATION_SHADER;
	test_2.expected_n_values	 = 2;
	test_2.should_draw_call_fail = false;
	test_2.requires_pipeline	 = false;
	test_2.tf_mode				 = GL_POINTS;
	test_2.use_gs				 = false;
	test_2.use_tc				 = true;
	test_2.use_te				 = true;

	tests.push_back(test_2);

	/* Note: This is a special negative case */
	test_3.expected_data_source = m_glExtTokens.TESS_CONTROL_SHADER;
	test_3.expected_n_values	= 4;
	if (!glu::isContextTypeES(m_context.getRenderContext().getType()) && isExtensionSupported("GL_NV_gpu_shader5"))
	{
		test_3.should_draw_call_fail = false;
		test_3.tf_mode				 = m_glExtTokens.PATCHES;
	}
	else
	{
		test_3.should_draw_call_fail = true;
		test_3.tf_mode				 = GL_POINTS;
	}
	test_3.requires_pipeline = true;
	test_3.use_gs			 = false;
	test_3.use_tc			 = true;
	test_3.use_te			 = false;

	tests.push_back(test_3);

	/* Use only one vertex per patch */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.patchParameteri(m_glExtTokens.PATCH_VERTICES, 1);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glPatchParameteriEXT() failed.");

	/* This test runs in two iterations:
	 *
	 * 1) Shaders are attached to a program object at the beginning of
	 *    each test. The test then executes. Once it's completed, the
	 *    shaders are detached from the program object;
	 * 2) A pipeline object is used instead of a program object.
	 */
	for (int n_iteration = 0; n_iteration < 2; ++n_iteration)
	{
		bool use_pipeline_object = (n_iteration == 1);

		if (use_pipeline_object)
		{
			gl.bindProgramPipeline(m_pipeline_id);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glBindProgramPipeline() failed.");

			gl.useProgram(0);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram() failed");
		}
		else
		{
			gl.bindProgramPipeline(0);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glBindProgramPipeline() failed.");

			/* The program object will be shortly re-linked so defer the glUseProgram() call */
		}

		/* Iterate through all tests */
		for (_tests_const_iterator test_iterator = tests.begin(); test_iterator != tests.end(); test_iterator++)
		{
			const _test_descriptor& test = *test_iterator;

			if (use_pipeline_object)
			{
				/* Configure the pipeline object */
				if (m_is_geometry_shader_extension_supported)
				{
					gl.useProgramStages(m_pipeline_id, m_glExtTokens.GEOMETRY_SHADER_BIT,
										test.use_gs ? m_gs_program_id : 0);
					GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgramStages() failed for GL_GEOMETRY_SHADER_BIT_EXT");
				}

				gl.useProgramStages(m_pipeline_id, m_glExtTokens.TESS_CONTROL_SHADER_BIT,
									test.use_tc ? m_tc_program_id : 0);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgramStages() failed for GL_TESS_CONTROL_SHADER_BIT_EXT");

				gl.useProgramStages(m_pipeline_id, m_glExtTokens.TESS_EVALUATION_SHADER_BIT,
									test.use_te ? m_te_program_id : 0);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgramStages() failed for GL_TESS_EVALUATION_SHADER_BIT_EXT");

				/* Validate the pipeline object */
				gl.validateProgramPipeline(m_pipeline_id);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glValidateProgramPipeline() failed");

				/* Retrieve the validation result */
				glw::GLint validate_status = GL_FALSE;

				gl.getProgramPipelineiv(m_pipeline_id, GL_VALIDATE_STATUS, &validate_status);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glGetProgramPipelineiv() failed");

				if (validate_status == GL_FALSE && !test.should_draw_call_fail)
				{
					m_testCtx.getLog() << tcu::TestLog::Message << "A pipeline object consisting of: "
									   << "[fragment stage] " << ((test.use_gs) ? "[geometry stage] " : "")
									   << ((test.use_tc) ? "[tessellation control stage] " : "")
									   << ((test.use_te) ? "[tessellation evaluation stage] " : "") << "[vertex stage] "
									   << "was not validated successfully, even though it should."
									   << tcu::TestLog::EndMessage;

					TCU_FAIL("Pipeline object is considered invalid, even though the stage combination is valid");
				}
			}
			else
			{
				if (test.requires_pipeline)
				{
					continue;
				}

				/* Attach the shaders to the program object as described in
				 * the test descriptor */
				if (test.use_gs)
				{
					gl.attachShader(m_po_id, m_gs_id);
					GLU_EXPECT_NO_ERROR(gl.getError(), "Could not attach geometry shader");
				}

				if (test.use_tc)
				{
					gl.attachShader(m_po_id, m_tc_id);
					GLU_EXPECT_NO_ERROR(gl.getError(), "Could not attach tessellation control shader");
				}

				if (test.use_te)
				{
					gl.attachShader(m_po_id, m_te_id);
					GLU_EXPECT_NO_ERROR(gl.getError(), "Could not attach tessellation evaluation shader");
				}

				/* Link the program object */
				gl.linkProgram(m_po_id);
				GLU_EXPECT_NO_ERROR(gl.getError(), "Could not link program object");

				/* Has the linking succeeded? */
				glw::GLint link_status = GL_FALSE;

				gl.getProgramiv(m_po_id, GL_LINK_STATUS, &link_status);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glGetProgramiv() failed");

				if (link_status != GL_TRUE)
				{
					m_testCtx.getLog() << tcu::TestLog::Message << "A program object consisting of: "
									   << "[fragment shader] " << ((test.use_gs) ? "[geometry shader] " : "")
									   << ((test.use_tc) ? "[tessellation control shader] " : "")
									   << ((test.use_te) ? "[tessellation evaluation shader] " : "")
									   << "[vertex shader] "
									   << "failed to link, even though it should link successfully."
									   << tcu::TestLog::EndMessage;

					TCU_FAIL("Program linking failed, even though the shader combination was valid");
				}

				gl.useProgram(m_po_id);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram() failed.");
			}

			/* Render a single point */
			gl.enable(GL_RASTERIZER_DISCARD);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glEnable(GL_RASTERIZER_DISCARD) failed");

			gl.beginTransformFeedback(test.tf_mode);

			bool didBeginXFBFail = false;
			if (!test.should_draw_call_fail)
			{
				GLU_EXPECT_NO_ERROR(gl.getError(), "glBeginTransformFeedback(GL_POINTS) failed");
			}
			else
			{
				/* For the negative case, i.e. beginTransformFeedback with an invalid pipeline of {VS, TCS, FS},
				 * ES spec is not clear if beginTransformFeedback should error, so relax the requirment here so
				 * that test passes as long as either beginTransformFeedback or the next draw call raises
				 * INVALID_OPERATION */
				glw::GLint err = gl.getError();
				if (err == GL_INVALID_OPERATION)
				{
					didBeginXFBFail = true;
				}
				else if (err != GL_NO_ERROR)
				{
					TCU_FAIL("Unexpected GL error in a beginTransformFeedback made on the program pipeline whose"
							 "program closest to TFB has no output varying specified");
				}
			}

			{
				gl.drawArrays(m_glExtTokens.PATCHES, 0 /* first */, 1 /* count */);

				if (!test.should_draw_call_fail)
				{
					GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawArrays() failed");
				}
				else
				{
					if (gl.getError() != GL_INVALID_OPERATION && !didBeginXFBFail)
					{
						TCU_FAIL("A draw call made using a program object lacking TES stage has"
								 " not generated a GL_INVALID_OPERATION as specified");
					}
				}
			}
			gl.endTransformFeedback();

			if (!didBeginXFBFail)
			{
				GLU_EXPECT_NO_ERROR(gl.getError(), "glEndTransformFeedback() failed");
			}
			else
			{
				if (gl.getError() != GL_INVALID_OPERATION)
				{
					TCU_FAIL("An endTransformFeedback made on inactive xfb has not generated a "
							 "GL_INVALID_OPERATION as specified");
				}
			}

			gl.disable(GL_RASTERIZER_DISCARD);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glDisable(GL_RASTERIZER_DISCARD) failed");

			if (!test.should_draw_call_fail)
			{
				/* Retrieve the captured result values */
				glw::GLfloat* result_ptr = (glw::GLfloat*)gl.mapBufferRange(
					GL_TRANSFORM_FEEDBACK_BUFFER, 0, /* offset */
					sizeof(float) * 4 /* components */ * test.expected_n_values, GL_MAP_READ_BIT | GL_MAP_WRITE_BIT);

				GLU_EXPECT_NO_ERROR(gl.getError(), "glMapBufferRange() failed");

				/* Verify the data */
				const glw::GLfloat epsilon			  = (glw::GLfloat)1e-5;
				const glw::GLfloat expected_gs_data[] = { 1.0f, 2.0f, 3.0f, 4.0f };
				const glw::GLfloat expected_tc_data[] = { 2.0f, 3.0f, 4.0f, 5.0f };
				const glw::GLfloat expected_te_data[] = { 3.0f, 4.0f, 5.0f, 6.0f };
				const glw::GLfloat expected_vs_data[] = { 4.0f, 5.0f, 6.0f, 7.0f };

				for (int n_value = 0; n_value < test.expected_n_values; ++n_value)
				{
					const glw::GLfloat* expected_data_ptr = NULL;
					const glw::GLfloat* captured_data_ptr = result_ptr + n_value * 4 /* components */;

					if (test.expected_data_source == m_glExtTokens.GEOMETRY_SHADER)
					{
						expected_data_ptr = expected_gs_data;
					}
					else if (test.expected_data_source == m_glExtTokens.TESS_CONTROL_SHADER)
					{
						expected_data_ptr = expected_tc_data;
					}
					else if (test.expected_data_source == m_glExtTokens.TESS_EVALUATION_SHADER)
					{
						expected_data_ptr = expected_te_data;
					}
					else if (test.expected_data_source == GL_VERTEX_SHADER)
					{
						expected_data_ptr = expected_vs_data;
					}
					else
					{
						TCU_FAIL("Unrecognized expected data source");
					}

					if (de::abs(captured_data_ptr[0] - expected_data_ptr[0]) > epsilon ||
						de::abs(captured_data_ptr[1] - expected_data_ptr[1]) > epsilon ||
						de::abs(captured_data_ptr[2] - expected_data_ptr[2]) > epsilon ||
						de::abs(captured_data_ptr[3] - expected_data_ptr[3]) > epsilon)
					{
						m_testCtx.getLog() << tcu::TestLog::Message << "Captured data "
										   << "(" << captured_data_ptr[0] << ", " << captured_data_ptr[1] << ", "
										   << captured_data_ptr[2] << ", " << captured_data_ptr[3] << ")"
										   << "is different from the expected value "
										   << "(" << expected_data_ptr[0] << ", " << expected_data_ptr[1] << ", "
										   << expected_data_ptr[2] << ", " << expected_data_ptr[3] << ")"
										   << tcu::TestLog::EndMessage;

						TCU_FAIL("Invalid data captured");
					}
				}

				/* Unmap the buffer object, since we're done */
				memset(result_ptr, 0, sizeof(float) * 4 /* components */ * test.expected_n_values);

				gl.unmapBuffer(GL_TRANSFORM_FEEDBACK_BUFFER);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glUnmapBuffer() failed");
			} /* if (!test.should_draw_call_fail) */

			if (!use_pipeline_object)
			{
				/* Detach all shaders we attached to the program object at the beginning
				 * of the iteration */
				if (test.use_gs)
				{
					gl.detachShader(m_po_id, m_gs_id);
					GLU_EXPECT_NO_ERROR(gl.getError(), "Could not detach geometry shader");
				}

				if (test.use_tc)
				{
					gl.detachShader(m_po_id, m_tc_id);
					GLU_EXPECT_NO_ERROR(gl.getError(), "Could not detach tessellation control shader");
				}

				if (test.use_te)
				{
					gl.detachShader(m_po_id, m_te_id);
					GLU_EXPECT_NO_ERROR(gl.getError(), "Could not detach tessellation evaluation shader");
				}

			} /* if (!use_pipeline_object) */
			else
			{
				/* We don't need to do anything with the pipeline object - stages will be
				 * re-defined in next iteration */
			}
		} /* for (all tests) */
	}	 /* for (all iterations) */

	/* All done */
	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	return STOP;
}

} /* namespace glcts */
