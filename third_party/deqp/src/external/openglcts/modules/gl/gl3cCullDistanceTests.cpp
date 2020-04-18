/*-------------------------------------------------------------------------
 * OpenGL Conformance Test Suite
 * -----------------------------
 *
 * Copyright (c) 2015-2016 The Khronos Group Inc.
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

/**
 */ /*!
 * \file  gl3cCullDistanceTests.cpp
 * \brief Cull Distance Test Suite Implementation
 */ /*-------------------------------------------------------------------*/

#include "gl3cCullDistanceTests.hpp"
#include "gluContextInfo.hpp"
#include "gluDefs.hpp"
#include "gluStrUtil.hpp"
#include "glwEnums.hpp"
#include "glwFunctions.hpp"
#include "tcuRenderTarget.hpp"
#include "tcuTestLog.hpp"

#include <cmath>
#include <sstream>
#include <string>
#include <vector>

#ifndef GL_MAX_CULL_DISTANCES
#define GL_MAX_CULL_DISTANCES (0x82F9)
#endif
#ifndef GL_MAX_COMBINED_CLIP_AND_CULL_DISTANCES
#define GL_MAX_COMBINED_CLIP_AND_CULL_DISTANCES (0x82FA)
#endif

namespace glcts
{
/** @brief Build OpenGL program
 *
 *  @param [in]  gl             OpenGL function bindings
 *  @param [in]  testCtx        Context
 *  @param [in]  cs_body        Compute shader source code
 *  @param [in]  fs_body        Fragment shader source code
 *  @param [in]  gs_body        Geometric shader source code
 *  @param [in]  tc_body        Tessellation control shader source code
 *  @param [in]  te_body        Tessellation evaluation shader source code
 *  @param [in]  vs_body        Vertex shader source code
 *  @param [in]  n_tf_varyings  Number of transform feedback varyings
 *  @param [in]  tf_varyings    Transform feedback varyings names
 *
 *  @param [out] out_program    If succeeded output program GL handle, 0 otherwise.
 */
void CullDistance::Utilities::buildProgram(const glw::Functions& gl, tcu::TestContext& testCtx,
										   const glw::GLchar* cs_body, const glw::GLchar* fs_body,
										   const glw::GLchar* gs_body, const glw::GLchar* tc_body,
										   const glw::GLchar* te_body, const glw::GLchar* vs_body,
										   const glw::GLuint& n_tf_varyings, const glw::GLchar** tf_varyings,
										   glw::GLuint* out_program)
{
	glw::GLuint po_id = 0;

	struct _shaders_configuration
	{
		glw::GLenum		   type;
		const glw::GLchar* body;
		glw::GLuint		   id;
	} shaders_configuration[] = { { GL_COMPUTE_SHADER, cs_body, 0 },		 { GL_FRAGMENT_SHADER, fs_body, 0 },
								  { GL_GEOMETRY_SHADER, gs_body, 0 },		 { GL_TESS_CONTROL_SHADER, tc_body, 0 },
								  { GL_TESS_EVALUATION_SHADER, te_body, 0 }, { GL_VERTEX_SHADER, vs_body, 0 } };

	const glw::GLuint n_shaders_configuration = sizeof(shaders_configuration) / sizeof(shaders_configuration[0]);

	/* Guard allocated OpenGL resources */
	try
	{
		/* Create needed programs */
		po_id = gl.createProgram();
		GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateProgram() call failed.");

		for (glw::GLuint n_shader_index = 0; n_shader_index < n_shaders_configuration; n_shader_index++)
		{
			if (shaders_configuration[n_shader_index].body != DE_NULL)
			{
				/* Generate shader object */
				shaders_configuration[n_shader_index].id = gl.createShader(shaders_configuration[n_shader_index].type);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateShader() call failed");

				glw::GLint		  compile_status = GL_FALSE;
				const glw::GLuint so_id			 = shaders_configuration[n_shader_index].id;

				/* Assign shader source code */
				gl.shaderSource(shaders_configuration[n_shader_index].id, 1,		   /* count */
								&shaders_configuration[n_shader_index].body, DE_NULL); /* length */
				GLU_EXPECT_NO_ERROR(gl.getError(), "glShaderSource() call failed");

				gl.compileShader(so_id);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glCompileShader() call failed.");

				gl.getShaderiv(so_id, GL_COMPILE_STATUS, &compile_status);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glGetShaderiv() call failed.");

				if (compile_status == GL_FALSE)
				{
					std::vector<glw::GLchar> log_array(1);
					glw::GLint				 log_length = 0;
					std::string				 log_string("Failed to retrieve log");

					/* Retrive compilation log length */
					gl.getShaderiv(so_id, GL_INFO_LOG_LENGTH, &log_length);
					GLU_EXPECT_NO_ERROR(gl.getError(), "glGetShaderiv() call failed.");

					log_array.resize(log_length + 1, 0);

					gl.getShaderInfoLog(so_id, log_length, DE_NULL, &log_array[0]);
					GLU_EXPECT_NO_ERROR(gl.getError(), "glGetShaderInfoLog() call failed.");

					log_string = std::string(&log_array[0]);

					testCtx.getLog() << tcu::TestLog::Message << "Shader compilation has failed.\n"
									 << "Shader type: " << shaders_configuration[n_shader_index].type << "\n"
									 << "Shader compilation error log:\n"
									 << log_string << "\n"
									 << "Shader source code:\n"
									 << shaders_configuration[n_shader_index].body << "\n"
									 << tcu::TestLog::EndMessage;

					TCU_FAIL("Shader compilation has failed.");
				}

				/* Also attach the shader to the corresponding program object */
				gl.attachShader(po_id, so_id);

				GLU_EXPECT_NO_ERROR(gl.getError(), "glAttachShader() call failed");
			} /* if (shaders_configuration[n_shader_index].body != DE_NULL) */
		}	 /* for (all shader object IDs) */

		/* Set transform feedback if requested */
		if (n_tf_varyings > 0)
		{
			gl.transformFeedbackVaryings(po_id, n_tf_varyings, tf_varyings, GL_INTERLEAVED_ATTRIBS);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glTransformFeedbackVaryings() call failed");
		}

		/* Try to link the program objects */
		if (po_id != 0)
		{
			glw::GLint link_status = GL_FALSE;

			gl.linkProgram(po_id);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glLinkProgram() call failed");

			gl.getProgramiv(po_id, GL_LINK_STATUS, &link_status);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glGetProgramiv() call failed");

			if (link_status == GL_FALSE)
			{
				std::vector<glw::GLchar> log_array(1);
				glw::GLsizei			 log_length = 0;
				std::string				 log_string;

				/* Retreive compilation log length */
				gl.getProgramiv(po_id, GL_INFO_LOG_LENGTH, &log_length);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glGetProgramiv() call failed.");

				log_array.resize(log_length + 1, 0);

				/* Retreive compilation log */
				gl.getProgramInfoLog(po_id, log_length, DE_NULL, &log_array[0]);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glGetProgramInfoLog() call failed.");

				log_string = std::string(&log_array[0]);

				/* Log linking error message */
				testCtx.getLog() << tcu::TestLog::Message << "Program linking has failed.\n"
								 << "Linking error log:\n"
								 << log_string << "\n"
								 << tcu::TestLog::EndMessage;

				/* Log shader source code of shaders involved */
				for (glw::GLuint n_shader_index = 0; n_shader_index < n_shaders_configuration; n_shader_index++)
				{
					if (shaders_configuration[n_shader_index].body != DE_NULL)
					{
						testCtx.getLog() << tcu::TestLog::Message << "Shader source code of type "
										 << shaders_configuration[n_shader_index].type << " follows:\n"
										 << shaders_configuration[n_shader_index].body << "\n"
										 << tcu::TestLog::EndMessage;
					}
				}

				TCU_FAIL("Program linking failed");
			}
		} /* if (po_id != 0) */

		/* Delete all shaders we've created */
		for (glw::GLuint n_shader_index = 0; n_shader_index < n_shaders_configuration; n_shader_index++)
		{
			const glw::GLuint so_id = shaders_configuration[n_shader_index].id;

			if (so_id != 0)
			{
				gl.deleteShader(so_id);

				shaders_configuration[n_shader_index].id = 0;

				GLU_EXPECT_NO_ERROR(gl.getError(), "glDeleteShader() call failed.");
			}
		}

		/* Store the result progrtam IDs */
		*out_program = po_id;
	}
	catch (...)
	{
		/* Delete all shaders we've created */
		for (glw::GLuint n_shader_index = 0; n_shader_index < n_shaders_configuration; n_shader_index++)
		{
			const glw::GLuint so_id = shaders_configuration[n_shader_index].id;

			if (so_id != 0)
			{
				gl.deleteShader(so_id);

				shaders_configuration[n_shader_index].id = 0;
			}
		}

		/* Delete the program object */
		if (po_id != 0)
		{
			gl.deleteProgram(po_id);

			po_id = 0;
		}

		/* Rethrow */
		throw;
	}
}

/** @brief Replace all occurences of a substring in a string by a substring
 *
 *  @param [in,out] str    string to be edited
 *  @param [in]     from   substring to be replaced
 *  @param [out]    to     new substring
 */
void CullDistance::Utilities::replaceAll(std::string& str, const std::string& from, const std::string& to)
{
	for (size_t start_pos = str.find(from, 0); start_pos != std::string::npos; start_pos = str.find(from, start_pos))
	{
		str.replace(start_pos, from.length(), to);

		start_pos += to.length();
	}

	return;
}

/** @brief Convert integer to string representation
 *
 *  @param [in] integer     input integer to be converted
 *
 *  @return String representation of integer
 */
std::string CullDistance::Utilities::intToString(glw::GLint integer)
{
	std::stringstream temp_sstream;

	temp_sstream << integer;

	return temp_sstream.str();
}

/** Constructor.
 *
 *  @param context Rendering context handle.
 **/
CullDistance::APICoverageTest::APICoverageTest(deqp::Context& context)
	: TestCase(context, "coverage", "Cull Distance API Coverage Test")
	, m_bo_id(0)
	, m_cs_id(0)
	, m_cs_to_id(0)
	, m_fbo_draw_id(0)
	, m_fbo_draw_to_id(0)
	, m_fbo_read_id(0)
	, m_fs_id(0)
	, m_gs_id(0)
	, m_po_id(0)
	, m_tc_id(0)
	, m_te_id(0)
	, m_vao_id(0)
	, m_vs_id(0)
{
	/* Left blank on purpose */
}

/** @brief Cull Distance API Coverage Test deinitialization */
void CullDistance::APICoverageTest::deinit()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (m_bo_id != 0)
	{
		gl.deleteBuffers(1, &m_bo_id);

		m_bo_id = 0;
	}

	if (m_cs_id != 0)
	{
		gl.deleteShader(m_cs_id);

		m_cs_id = 0;
	}

	if (m_cs_to_id != 0)
	{
		gl.deleteTextures(1, &m_cs_to_id);

		m_cs_to_id = 0;
	}

	if (m_fbo_draw_id != 0)
	{
		gl.deleteFramebuffers(1, &m_fbo_draw_id);

		m_fbo_draw_id = 0;
	}

	if (m_fbo_draw_to_id != 0)
	{
		gl.deleteTextures(1, &m_fbo_draw_to_id);

		m_fbo_draw_to_id = 0;
	}

	if (m_fbo_read_id != 0)
	{
		gl.deleteFramebuffers(1, &m_fbo_read_id);

		m_fbo_read_id = 0;
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

	if (m_te_id != 0)
	{
		gl.deleteShader(m_te_id);

		m_te_id = 0;
	}

	if (m_vao_id != 0)
	{
		gl.deleteVertexArrays(1, &m_vao_id);

		m_vao_id = 0;
	}

	if (m_vs_id != 0)
	{
		gl.deleteShader(m_vs_id);

		m_vs_id = 0;
	}

	/* Restore default pack alignment value */
	gl.pixelStorei(GL_PACK_ALIGNMENT, 4);
}

/** Executes test iteration.
 *
 *  @return Returns STOP when test has finished executing, CONTINUE if more iterations are needed.
 */
tcu::TestNode::IterateResult CullDistance::APICoverageTest::iterate()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* This test should only be executed if ARB_cull_distance is supported, or if
	 * we're running a GL4.5 context
	 */
	if (!m_context.getContextInfo().isExtensionSupported("GL_ARB_cull_distance") &&
		!glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::core(4, 5)))
	{
		throw tcu::NotSupportedError("GL_ARB_cull_distance is not supported");
	}

	/* Check that calling GetIntegerv with MAX_CULL_DISTANCES doesn't generate
	 * any errors and returns a value at least 8.
	 *
	 * Check that calling GetIntegerv with MAX_COMBINED_CLIP_AND_CULL_DISTANCES
	 * doesn't generate any errors and returns a value at least 8.
	 *
	 */
	glw::GLint error_code									 = GL_NO_ERROR;
	glw::GLint gl_max_cull_distances_value					 = 0;
	glw::GLint gl_max_combined_clip_and_cull_distances_value = 0;

	gl.getIntegerv(GL_MAX_CULL_DISTANCES, &gl_max_cull_distances_value);

	error_code = gl.getError();
	if (error_code != GL_NO_ERROR)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "glGetIntegerv() returned error code "
						   << "[" << glu::getErrorStr(error_code) << "] for GL_MAX_CULL_DISTANCES"
																	 " query instead of GL_NO_ERROR"
						   << tcu::TestLog::EndMessage;

		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");

		return STOP;
	}

	gl.getIntegerv(GL_MAX_COMBINED_CLIP_AND_CULL_DISTANCES, &gl_max_combined_clip_and_cull_distances_value);

	error_code = gl.getError();
	if (error_code != GL_NO_ERROR)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "glGetIntegerv() returned error code "
						   << "[" << glu::getErrorStr(error_code) << "] for "
																	 "GL_MAX_COMBINED_CLIP_AND_CULL_DISTANCES query "
																	 "instead of GL_NO_ERROR"
						   << tcu::TestLog::EndMessage;

		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");

		return STOP;
	}

	/* Before we proceed with the two other tests, initialize a buffer & a texture
	 * object we will need to capture data from the programs */
	static const glw::GLuint bo_size = sizeof(int) * 4 /* components */ * 4 /* result points */;

	gl.genBuffers(1, &m_bo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenBuffers() call failed.");

	gl.genFramebuffers(1, &m_fbo_draw_id);
	gl.genFramebuffers(1, &m_fbo_read_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenFramebuffers() call(s) failed.");

	gl.genTextures(1, &m_cs_to_id);
	gl.genTextures(1, &m_fbo_draw_to_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenTextures() call failed.");

	gl.genVertexArrays(1, &m_vao_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenVertexArrays() call failed.");

	gl.bindVertexArray(m_vao_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindVertexArray() call failed.");

	gl.bindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, m_bo_id);
	gl.bindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, /* index */
					  m_bo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer() or glBindBufferBase() call(s) failed.");

	gl.bufferData(GL_TRANSFORM_FEEDBACK_BUFFER, bo_size, DE_NULL, GL_STATIC_DRAW);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferData() call failed.");

	for (glw::GLuint n_to_id = 0; n_to_id < 2; /* CS, FBO */ ++n_to_id)
	{
		gl.bindTexture(GL_TEXTURE_2D, (n_to_id == 0) ? m_cs_to_id : m_fbo_draw_to_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture() call failed.");

		gl.texStorage2D(GL_TEXTURE_2D, 1, /* levels */
						GL_R32I, 1,		  /* width */
						1);				  /* height */
		GLU_EXPECT_NO_ERROR(gl.getError(), "glTexStorage2D() call failed.");
	}

	gl.bindImageTexture(0,			   /* unit */
						m_cs_to_id, 0, /* level */
						GL_FALSE,	  /* layered */
						0,			   /* layer */
						GL_WRITE_ONLY, GL_R32I);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindImageTexture() call failed.");

	gl.bindFramebuffer(GL_DRAW_FRAMEBUFFER, m_fbo_draw_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffer() call failed.");

	gl.framebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_fbo_draw_to_id, 0); /* level */
	GLU_EXPECT_NO_ERROR(gl.getError(), "glFramebufferTexture2D() call failed.");

	gl.bindFramebuffer(GL_READ_FRAMEBUFFER, m_fbo_read_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffer() call failed.");

	gl.viewport(0,  /* x */
				0,  /* y */
				1,  /* width */
				1); /* height */
	GLU_EXPECT_NO_ERROR(gl.getError(), "glViewport() call failed.");

	gl.pixelStorei(GL_PACK_ALIGNMENT, 1);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glPixelStorei() call failed.");

	/* There are two new GL constants, where value we need to verify */
	struct _run
	{
		const glw::GLchar* essl_token_value;
		glw::GLenum		   gl_enum;
		glw::GLint		   gl_value;
		glw::GLint		   min_value;
		const glw::GLchar* name;
	} runs[] = { { "gl_MaxCullDistances", GL_MAX_CULL_DISTANCES, gl_max_cull_distances_value, 8 /*minimum required */,
				   "GL_MAX_CULL_DISTANCES" },
				 { "gl_MaxCombinedClipAndCullDistances", GL_MAX_COMBINED_CLIP_AND_CULL_DISTANCES,
				   gl_max_combined_clip_and_cull_distances_value, 8 /*minimum required */,
				   "GL_MAX_COMBINED_CLIP_AND_CULL_DISTANCES" } };

	static const glw::GLuint n_runs = sizeof(runs) / sizeof(runs[0]);

	for (glw::GLuint n_run = 0; n_run < n_runs; ++n_run)
	{
		_run& current_run = runs[n_run];

		static const struct _stage
		{
			bool use_cs;
			bool use_fs;
			bool use_gs;
			bool use_tc;
			bool use_te;
			bool use_vs;

			const glw::GLchar* fs_input;
			const glw::GLchar* gs_input;
			const glw::GLchar* tc_input;
			const glw::GLchar* te_input;

			const glw::GLchar* tf_output_name;
			const glw::GLenum  tf_mode;

			glw::GLenum draw_call_mode;
			glw::GLuint n_draw_call_vertices;
		} stages[] = { /* CS only test */
					   {
						   /* use_cs|use_fs|use_gs|use_tc|use_te|use_vs */
						   true, false, false, false, false, false,

						   NULL,	/* fs_input             */
						   NULL,	/* gs_input             */
						   NULL,	/* tc_input             */
						   NULL,	/* te_input             */
						   NULL,	/* tf_output_name       */
						   GL_NONE, /* tf_mode              */
						   GL_NONE, /* draw_call_mode       */
						   0,		/* n_draw_call_vertices */
					   },
					   /* VS+GS+TC+TE+FS test */
					   {
						   /* use_cs|use_fs|use_gs|use_tc|use_te|use_vs */
						   false, true, true, true, true, true,

						   "out_gs",	 /* fs_input             */
						   "out_te",	 /* gs_input             */
						   "out_vs",	 /* tc_input             */
						   "out_tc",	 /* te_input             */
						   "out_gs",	 /* tf_output_name       */
						   GL_TRIANGLES, /* tf_mode              */
						   GL_PATCHES,   /* draw_call_mode       */
						   3,			 /* n_draw_call_vertices */
					   },
					   /* VS+GS+FS test */
					   {
						   /* use_cs|use_fs|use_gs|use_tc|use_te|use_vs */
						   false, true, true, false, false, true,

						   "out_gs",	 /* fs_input             */
						   "out_vs",	 /* gs_input             */
						   NULL,		 /* tc_input             */
						   NULL,		 /* te_input             */
						   "out_gs",	 /* tf_output_name       */
						   GL_TRIANGLES, /* tf_mode              */
						   GL_POINTS,	/* draw_call_mode       */
						   1,			 /* n_draw_call_vertices */
					   },
					   /* VS+TC+TE+FS test */
					   {
						   /* use_cs|use_fs|use_gs|use_tc|use_te|use_vs */
						   false, true, false, true, true, true,

						   "out_te",   /* fs_input             */
						   NULL,	   /* gs_input             */
						   "out_vs",   /* tc_input             */
						   "out_tc",   /* te_input             */
						   "out_te",   /* tf_output_name       */
						   GL_POINTS,  /* tf_mode              */
						   GL_PATCHES, /* draw_call_mode       */
						   3		   /* n_draw_call_vertices */
					   },
					   /* VS test */
					   {
						   /* use_cs|use_fs|use_gs|use_tc|use_te|use_vs */
						   false, false, false, false, false, true,

						   "out_vs",  /* fs_input             */
						   NULL,	  /* gs_input             */
						   NULL,	  /* tc_input             */
						   NULL,	  /* te_input             */
						   "out_vs",  /* tf_output_name       */
						   GL_POINTS, /* tf_mode              */
						   GL_POINTS, /* draw_call_mode       */
						   1		  /* n_draw_call_vertices */
					   }
		};
		const glw::GLuint n_stages = sizeof(stages) / sizeof(stages[0]);

		/* Run through all test stages */
		for (glw::GLuint n_stage = 0; n_stage < n_stages; ++n_stage)
		{
			/* Check for OpenGL feature support */
			if (stages[n_stage].use_cs)
			{
				if (!glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::core(4, 3)) &&
					!m_context.getContextInfo().isExtensionSupported("GL_ARB_compute_shader"))
				{
					continue; // no compute shader support
				}
			}
			if (stages[n_stage].use_tc || stages[n_stage].use_te)
			{
				if (!glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::core(4, 0)) &&
					!m_context.getContextInfo().isExtensionSupported("GL_ARB_tessellation_shader"))
				{
					continue; // no tessellation shader support
				}
			}

			/* Check that use of the GLSL built-in constant gl_MaxCullDistance in any
			 * shader stage (including compute shader) does not affect the shader
			 * compilation & program linking process.
			 */
			static const glw::GLchar* cs_body_template =
				"#version 150\n"
				"\n"
				"#extension GL_ARB_compute_shader          : require\n"
				"#extension GL_ARB_cull_distance           : require\n"
				"#extension GL_ARB_shader_image_load_store : require\n"
				"\n"
				"layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;\n"
				"\n"
				"layout(r32i) uniform writeonly iimage2D result;\n"
				"\n"
				"void main()\n"
				"{\n"
				"    imageStore(result, ivec2(0),ivec4(TOKEN) );\n"
				"}\n";
			std::string cs_body = cs_body_template;

			static const glw::GLchar* fs_body_template = "#version 150\n"
														 "\n"
														 "#extension GL_ARB_cull_distance : require\n"
														 "\n"
														 "flat in  int INPUT_FS_NAME;\n"
														 "out int out_fs;\n"
														 "\n"
														 "void main()\n"
														 "{\n"
														 "    if (INPUT_FS_NAME == TOKEN)\n"
														 "    {\n"
														 "        out_fs = TOKEN;\n"
														 "    }\n"
														 "    else\n"
														 "    {\n"
														 "        out_fs = -1;\n"
														 "    }\n"
														 "}\n";
			std::string fs_body = fs_body_template;

			static const glw::GLchar* gs_body_template =
				"#version 150\n"
				"\n"
				"#extension GL_ARB_cull_distance : require\n"
				"\n"
				"flat in  int INPUT_GS_NAME[];\n"
				"flat out int out_gs;\n"
				"\n"
				"layout(points)                           in;\n"
				"layout(triangle_strip, max_vertices = 4) out;\n"
				"\n"
				"void main()\n"
				"{\n"
				"    int result_value = (INPUT_GS_NAME[0] == TOKEN) ? TOKEN : -1;\n"
				"\n"
				/* Draw a full-screen quad */
				"    gl_Position = vec4(-1.0, 1.0, 0.0, 1.0);\n"
				"    out_gs      = result_value;\n"
				"    EmitVertex();\n"
				"\n"
				"    gl_Position = vec4(-1.0, -1.0, 0.0, 1.0);\n"
				"    out_gs      = result_value;\n"
				"    EmitVertex();\n"
				"\n"
				"    gl_Position = vec4(1.0, 1.0, 0.0, 1.0);\n"
				"    out_gs      = result_value;\n"
				"    EmitVertex();\n"
				"\n"
				"    gl_Position = vec4(1.0, -1.0, 0.0, 1.0);\n"
				"    out_gs      = result_value;\n"
				"    EmitVertex();\n"
				"    EndPrimitive();\n"
				"}\n";
			std::string gs_body = gs_body_template;

			static const glw::GLchar* tc_body_template =
				"#version 400\n"
				"\n"
				"#extension GL_ARB_cull_distance : require\n"
				"\n"
				"layout(vertices = 1) out;\n"
				"\n"
				"flat in  int INPUT_TC_NAME[];\n"
				"flat out int out_tc       [];\n"
				"\n"
				"void main()\n"
				"{\n"
				"    int result_value = (INPUT_TC_NAME[0] == TOKEN) ? TOKEN : -1;\n"
				"\n"
				"    out_tc[gl_InvocationID]             = result_value;\n"
				"    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;\n"
				"    gl_TessLevelInner[0]                = 1.0;\n"
				"    gl_TessLevelInner[1]                = 1.0;\n"
				"    gl_TessLevelOuter[0]                = 1.0;\n"
				"    gl_TessLevelOuter[1]                = 1.0;\n"
				"    gl_TessLevelOuter[2]                = 1.0;\n"
				"    gl_TessLevelOuter[3]                = 1.0;\n"
				"}\n";
			std::string tc_body = tc_body_template;

			static const glw::GLchar* te_body_template =
				"#version 400\n"
				"\n"
				"#extension GL_ARB_cull_distance : require\n"
				"\n"
				"flat in  int INPUT_TE_NAME[];\n"
				"flat out int out_te;\n"
				"\n"
				"layout(isolines, point_mode) in;\n"
				"\n"
				"void main()\n"
				"{\n"
				"    int result_value = (INPUT_TE_NAME[0] == TOKEN) ? TOKEN : 0;\n"
				"\n"
				"    out_te = result_value;\n"
				"\n"
				"    gl_Position = vec4(0.0, 0.0, 0.0, 1.);\n"
				"}\n";
			std::string te_body = te_body_template;

			static const glw::GLchar* vs_body_template = "#version 150\n"
														 "\n"
														 "#extension GL_ARB_cull_distance : require\n"
														 "\n"
														 "flat out int out_vs;\n"
														 "\n"
														 "void main()\n"
														 "{\n"
														 "    gl_Position = vec4(0.0, 0.0, 0.0, 1.0);\n"
														 "    out_vs      = TOKEN;\n"
														 "}\n";
			std::string vs_body = vs_body_template;

			const _stage& current_stage = stages[n_stage];

			/* Build shader bodies */
			struct _shader_body
			{
				std::string* body_ptr;
				glw::GLenum  gl_type;
			} shader_bodies[] = { { &cs_body, GL_COMPUTE_SHADER },		   { &fs_body, GL_FRAGMENT_SHADER },
								  { &gs_body, GL_GEOMETRY_SHADER },		   { &tc_body, GL_TESS_CONTROL_SHADER },
								  { &te_body, GL_TESS_EVALUATION_SHADER }, { &vs_body, GL_VERTEX_SHADER } };
			static const glw::GLchar* input_fs_token_string = "INPUT_FS_NAME";
			static const glw::GLchar* input_gs_token_string = "INPUT_GS_NAME";
			static const glw::GLchar* input_te_token_string = "INPUT_TE_NAME";
			static const glw::GLchar* input_tc_token_string = "INPUT_TC_NAME";
			static const glw::GLuint  n_shader_bodies		= sizeof(shader_bodies) / sizeof(shader_bodies[0]);

			std::size_t				  token_position = std::string::npos;
			static const glw::GLchar* token_string   = "TOKEN";

			for (glw::GLuint n_shader_body = 0; n_shader_body < n_shader_bodies; ++n_shader_body)
			{
				_shader_body& current_body = shader_bodies[n_shader_body];

				/* Is this stage actually used? */
				if (((current_body.gl_type == GL_COMPUTE_SHADER) && (!current_stage.use_cs)) ||
					((current_body.gl_type == GL_FRAGMENT_SHADER) && (!current_stage.use_fs)) ||
					((current_body.gl_type == GL_TESS_CONTROL_SHADER) && (!current_stage.use_tc)) ||
					((current_body.gl_type == GL_TESS_EVALUATION_SHADER) && (!current_stage.use_te)) ||
					((current_body.gl_type == GL_VERTEX_SHADER) && (!current_stage.use_vs)))
				{
					/* Skip the iteration. */
					continue;
				}

				/* Iterate over all token and replace them with stage-specific values */
				struct _token_value_pair
				{
					const glw::GLchar* token;
					const glw::GLchar* value;
				} token_value_pairs[] = {
					/* NOTE: The last entry is filled by the switch() block below */
					{ token_string, current_run.essl_token_value },
					{ NULL, NULL },
				};

				const size_t n_token_value_pairs = sizeof(token_value_pairs) / sizeof(token_value_pairs[0]);

				switch (current_body.gl_type)
				{
				case GL_COMPUTE_SHADER:
				case GL_VERTEX_SHADER:
					break;

				case GL_FRAGMENT_SHADER:
				{
					token_value_pairs[1].token = input_fs_token_string;
					token_value_pairs[1].value = current_stage.fs_input;

					break;
				}

				case GL_GEOMETRY_SHADER:
				{
					token_value_pairs[1].token = input_gs_token_string;
					token_value_pairs[1].value = current_stage.gs_input;

					break;
				}

				case GL_TESS_CONTROL_SHADER:
				{
					token_value_pairs[1].token = input_tc_token_string;
					token_value_pairs[1].value = current_stage.tc_input;

					break;
				}

				case GL_TESS_EVALUATION_SHADER:
				{
					token_value_pairs[1].token = input_te_token_string;
					token_value_pairs[1].value = current_stage.te_input;

					break;
				}

				default:
					TCU_FAIL("Unrecognized shader body type");
				}

				for (glw::GLuint n_pair = 0; n_pair < n_token_value_pairs; ++n_pair)
				{
					const _token_value_pair& current_pair = token_value_pairs[n_pair];

					if (current_pair.token == NULL || current_pair.value == NULL)
					{
						continue;
					}

					while ((token_position = current_body.body_ptr->find(current_pair.token)) != std::string::npos)
					{
						current_body.body_ptr->replace(token_position, strlen(current_pair.token), current_pair.value);
					}
				} /* for (all token+value pairs) */
			}	 /* for (all sader bodies) */

			/* Build the test program */
			CullDistance::Utilities::buildProgram(
				gl, m_testCtx, current_stage.use_cs ? cs_body.c_str() : DE_NULL,
				current_stage.use_fs ? fs_body.c_str() : DE_NULL, current_stage.use_gs ? gs_body.c_str() : DE_NULL,
				current_stage.use_tc ? tc_body.c_str() : DE_NULL, current_stage.use_te ? te_body.c_str() : DE_NULL,
				current_stage.use_vs ? vs_body.c_str() : DE_NULL, (current_stage.tf_output_name != NULL) ? 1 : 0,
				(const glw::GLchar**)&current_stage.tf_output_name, &m_po_id);

			/* Bind the test program */
			DE_ASSERT(m_po_id != 0);

			gl.useProgram(m_po_id);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram() call failed.");

			/* Execute the draw call. Transform Feed-back should be enabled for all iterations
			 * par the CS one, since we use a different tool to capture the result data in the
			 * latter case.
			 */
			if (!current_stage.use_cs)
			{
				gl.beginTransformFeedback(current_stage.tf_mode);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glBeginTransformFeedback() call failed.");

				gl.drawArrays(current_stage.draw_call_mode, 0,	 /* first */
							  current_stage.n_draw_call_vertices); /* count */
				GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawArrays() call failed.");

				gl.endTransformFeedback();
				GLU_EXPECT_NO_ERROR(gl.getError(), "glEndTransformFeedback() call failed.");
			} /* if (uses_tf) */
			else
			{
				gl.dispatchCompute(1,  /* num_groups_x */
								   1,  /* num_groups_y */
								   1); /* num_groups_z */
				GLU_EXPECT_NO_ERROR(gl.getError(), "glDispatchCompute() call failed.");
			}

			/* Verify the result values */
			if (!current_stage.use_cs)
			{
				glw::GLint* result_data_ptr = DE_NULL;

				/* Retrieve the data captured by Transform Feedback */
				result_data_ptr = (glw::GLint*)gl.mapBufferRange(GL_TRANSFORM_FEEDBACK_BUFFER, 0, /* offset */
																 sizeof(unsigned int) * 1, GL_MAP_READ_BIT);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glMapBufferRange() call failed.");

				if (*result_data_ptr != current_run.gl_value)
				{
					m_testCtx.getLog() << tcu::TestLog::Message << current_run.name << " value "
																					   "["
									   << *result_data_ptr << "]"
															  " does not match the one reported by glGetIntegerv() "
															  "["
									   << current_run.gl_value << "]" << tcu::TestLog::EndMessage;

					TCU_FAIL("GL constant value does not match the ES SL equivalent");
				}

				if (*result_data_ptr < current_run.min_value)
				{
					m_testCtx.getLog() << tcu::TestLog::Message << current_run.name << " value "
																					   "["
									   << *result_data_ptr << "]"
															  " does not meet the minimum specification requirements "
															  "["
									   << current_run.min_value << "]" << tcu::TestLog::EndMessage;

					TCU_FAIL("GL constant value does not meet minimum specification requirements");
				}

				gl.unmapBuffer(GL_TRANSFORM_FEEDBACK_BUFFER);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glUnmapBuffer() call failed.");
			}

			for (glw::GLuint n_stage_internal = 0; n_stage_internal < 2; /* CS, FS write to separate textures */
				 ++n_stage_internal)
			{
				glw::GLuint to_id = (n_stage_internal == 0) ? m_cs_to_id : m_fbo_draw_to_id;

				if (((n_stage_internal == 0) && (!current_stage.use_cs)) ||
					((n_stage_internal == 1) && (!current_stage.use_fs)))
				{
					/* Skip the iteration */
					continue;
				}

				/* Check the image data the test CS / FS should have written */
				glw::GLint result_value = 0;

				gl.framebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, to_id, 0); /* level */
				GLU_EXPECT_NO_ERROR(gl.getError(), "glFramebufferTexture2D() call failed.");

				/* NOTE: We're using our custom read framebuffer here, so we'll be reading
				 *       from the texture, that the writes have been issued to earlier. */
				gl.finish();
				GLU_EXPECT_NO_ERROR(gl.getError(), "glMemoryBarrier() call failed.");

				gl.readPixels(0, /* x */
							  0, /* y */
							  1, /* width */
							  1, /* height */
							  GL_RED_INTEGER, GL_INT, &result_value);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glReadPixels() call failed.");

				if (result_value != current_run.gl_value)
				{
					m_testCtx.getLog() << tcu::TestLog::Message << current_run.name
									   << " value accessible to the compute / fragment shader "
										  "["
									   << result_value << "]"
														  " does not match the one reported by glGetIntegerv() "
														  "["
									   << current_run.gl_value << "]" << tcu::TestLog::EndMessage;

					TCU_FAIL("GL constant value does not match the ES SL equivalent");
				}

				if (result_value < current_run.min_value)
				{
					m_testCtx.getLog() << tcu::TestLog::Message << current_run.name
									   << " value accessible to the compute / fragment shader "
										  "["
									   << result_value << "]"
														  " does not meet the minimum specification requirements "
														  "["
									   << current_run.min_value << "]" << tcu::TestLog::EndMessage;

					TCU_FAIL("GL constant value does not meet minimum specification requirements");
				}
			}

			/* Clear the data buffer before we continue */
			static const glw::GLubyte bo_clear_data[bo_size] = { 0 };

			gl.bufferSubData(GL_TRANSFORM_FEEDBACK_BUFFER, 0, /* offset */
							 bo_size, bo_clear_data);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferSubData() call failed.");

			/* Clear the texture mip-map before we continue */
			glw::GLint clear_values[4] = { 0, 0, 0, 0 };

			gl.clearBufferiv(GL_COLOR, 0, /* drawbuffer */
							 clear_values);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glClearBufferiv() call failed.");

			/* Release program before we move on to the next iteration */
			if (m_po_id != 0)
			{
				gl.deleteProgram(m_po_id);

				m_po_id = 0;
			}
		} /* for (all stages) */
	}	 /* for (both runs) */

	/* All done */
	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

	return STOP;
}

/** Constructor.
 *
 *  @param context Rendering context handle.
 **/
CullDistance::FunctionalTest::FunctionalTest(deqp::Context& context)
	: TestCase(context, "functional", "Cull Distance Functional Test")
	, m_bo_data()
	, m_bo_id(0)
	, m_fbo_id(0)
	, m_po_id(0)
	, m_render_primitives(0)
	, m_render_vertices(0)
	, m_sub_grid_cell_size(0)
	, m_to_id(0)
	, m_vao_id(0)
	, m_to_height(512)
	, m_to_width(512)
	, m_to_pixel_data_cache()
{
	/* Left blank on purpose */
}

/** @brief Build OpenGL program for functional tests
 *
 *  @param [in]  clipdistances_array_size   use size of gl_ClipDistance array
 *  @param [in]  culldistances_array_size   use size of gl_CullDistance array
 *  @param [in]  dynamic_index_writes       use dunamic indexing for setting  the gl_ClipDistance and gl_CullDistance arrays
 *  @param [in]  primitive_mode             primitive_mode will be used for rendering
 *  @param [in]  redeclare_clipdistances    redeclare gl_ClipDistance
 *  @param [in]  redeclare_culldistances    redeclare gl_CullDistance
 *  @param [in]  use_core_functionality     use core OpenGL functionality
 *  @param [in]  use_gs                     use geometry shader
 *  @param [in]  use_ts                     use tessellation shader
 *  @param [in]  fetch_culldistance_from_fs fetch check sum of gl_ClipDistance and gl_CullDistance from fragment shader
 */
void CullDistance::FunctionalTest::buildPO(glw::GLuint clipdistances_array_size, glw::GLuint culldistances_array_size,
										   bool dynamic_index_writes, _primitive_mode primitive_mode,
										   bool redeclare_clipdistances, bool redeclare_culldistances,
										   bool use_core_functionality, bool use_gs, bool use_ts,
										   bool fetch_culldistance_from_fs)
{
	deinitPO();

	/* Form the vertex shader */
	glw::GLuint clipdistances_input_size =
		clipdistances_array_size > 0 ? clipdistances_array_size : 1; /* Avoid zero-sized array compilation error */
	glw::GLuint culldistances_input_size =
		culldistances_array_size > 0 ? culldistances_array_size : 1; /* Avoid zero-sized array compilation error */
	static const glw::GLchar* dynamic_array_setters =
		"\n"
		"#if TEMPLATE_N_GL_CLIPDISTANCE_ENTRIES\n"
		"	 for (int n_clipdistance_entry = 0;\n"
		"		  n_clipdistance_entry < TEMPLATE_N_GL_CLIPDISTANCE_ENTRIES;\n"
		"		++n_clipdistance_entry)\n"
		"	 {\n"
		"	     ASSIGN_CLIP_DISTANCE(n_clipdistance_entry);\n"
		"	 }\n"
		"#endif"
		"\n"
		"#if TEMPLATE_N_GL_CULLDISTANCE_ENTRIES \n"
		"	 for (int n_culldistance_entry = 0;\n"
		"		  n_culldistance_entry < TEMPLATE_N_GL_CULLDISTANCE_ENTRIES;\n"
		"		++n_culldistance_entry)\n"
		"	 {\n"
		"	     ASSIGN_CULL_DISTANCE(n_culldistance_entry);\n"
		"	 }\n"
		"#endif\n";

	static const glw::GLchar* core_functionality = "#version 450\n";

	static const glw::GLchar* extention_functionality = "#version 440\n"
														"\n"
														"#extension GL_ARB_cull_distance : require\n"
														"\n"
														"#ifndef GL_ARB_cull_distance\n"
														"    #error GL_ARB_cull_distance is undefined\n"
														"#endif\n";

	static const glw::GLchar* fetch_function = "highp float fetch()\n"
											   "{\n"
											   "    highp float sum = 0.0;\n"
											   "\n"
											   "TEMPLATE_SUM_SETTER"
											   "\n"
											   "    return sum / TEMPLATE_SUM_DIVIDER;\n"
											   "}\n"
											   "\n"
											   "#define ASSIGN_RETURN_VALUE fetch()";

	static const glw::GLchar* fs_template = "TEMPLATE_HEADER_DECLARATION\n"
											"\n"
											"TEMPLATE_REDECLARE_CLIPDISTANCE\n"
											"TEMPLATE_REDECLARE_CULLDISTANCE\n"
											"\n"
											"TEMPLATE_ASSIGN_RETURN_VALUE\n"
											"\n"
											"out vec4 out_fs;\n"
											"\n"
											"/* Fragment shader main function */\n"
											"void main()\n"
											"{\n"
											"    out_fs = vec4(ASSIGN_RETURN_VALUE, 1.0, 1.0, 1.0);\n"
											"}\n";

	static const glw::GLchar* gs_template = "TEMPLATE_HEADER_DECLARATION\n"
											"\n"
											"TEMPLATE_LAYOUT_IN\n"
											"TEMPLATE_LAYOUT_OUT\n"
											"\n"
											"TEMPLATE_REDECLARE_CLIPDISTANCE\n"
											"TEMPLATE_REDECLARE_CULLDISTANCE\n"
											"\n"
											"#define ASSIGN_CLIP_DISTANCE(IDX) TEMPLATE_ASSIGN_CLIP_DISTANCE\n"
											"#define ASSIGN_CULL_DISTANCE(IDX) TEMPLATE_ASSIGN_CULL_DISTANCE\n"
											"\n"
											"/* Geometry shader (passthrough) main function */\n"
											"void main()\n"
											"{\n"
											"    for (int n_vertex_index = 0;\n"
											"             n_vertex_index < gl_in.length();\n"
											"             n_vertex_index ++)\n"
											"    {\n"
											"        gl_Position = gl_in[n_vertex_index].gl_Position;\n"
											"\n"
											"        TEMPLATE_ARRAY_SETTERS\n"
											"\n"
											"        EmitVertex();\n"
											"    }\n"
											"\n"
											"    EndPrimitive();\n"
											"}\n";

	static const glw::GLchar* tc_template =
		"TEMPLATE_HEADER_DECLARATION\n"
		"\n"
		"TEMPLATE_LAYOUT_OUT\n"
		"\n"
		"out gl_PerVertex {\n"
		"TEMPLATE_REDECLARE_CLIPDISTANCE\n"
		"TEMPLATE_REDECLARE_CULLDISTANCE\n"
		"vec4 gl_Position;\n"
		"} gl_out[];\n"
		"\n"
		"#define ASSIGN_CLIP_DISTANCE(IDX) TEMPLATE_ASSIGN_CLIP_DISTANCE\n"
		"#define ASSIGN_CULL_DISTANCE(IDX) TEMPLATE_ASSIGN_CULL_DISTANCE\n"
		"\n"
		"/* Tesselation control shader main function */\n"
		"void main()\n"
		"{\n"
		"    gl_TessLevelInner[0] = 1.0;\n"
		"    gl_TessLevelInner[1] = 1.0;\n"
		"    gl_TessLevelOuter[0] = 1.0;\n"
		"    gl_TessLevelOuter[1] = 1.0;\n"
		"    gl_TessLevelOuter[2] = 1.0;\n"
		"    gl_TessLevelOuter[3] = 1.0;\n"
		"    /* Clipdistance and culldistance array setters */\n"
		"    {\n"
		"        TEMPLATE_ARRAY_SETTERS\n"
		"    }\n"
		"    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;\n"
		"}\n";

	static const glw::GLchar* te_template = "TEMPLATE_HEADER_DECLARATION\n"
											"\n"
											"TEMPLATE_LAYOUT_IN\n"
											"\n"
											"in gl_PerVertex {\n"
											"TEMPLATE_REDECLARE_IN_CLIPDISTANCE\n"
											"TEMPLATE_REDECLARE_IN_CULLDISTANCE\n"
											"vec4 gl_Position;\n"
											"} gl_in[];\n"
											"\n"
											"TEMPLATE_REDECLARE_CLIPDISTANCE\n"
											"TEMPLATE_REDECLARE_CULLDISTANCE\n"
											"\n"
											"#define ASSIGN_CLIP_DISTANCE(IDX) TEMPLATE_ASSIGN_CLIP_DISTANCE\n"
											"#define ASSIGN_CULL_DISTANCE(IDX) TEMPLATE_ASSIGN_CULL_DISTANCE\n"
											"\n"
											"/* Tesselation evaluation shader main function */\n"
											"void main()\n"
											"{\n"
											"    /* Clipdistance and culldistance array setters */\n"
											"    {\n"
											"        TEMPLATE_ARRAY_SETTERS\n"
											"    }\n"
											"    gl_Position = TEMPLATE_OUT_FORMULA;\n"
											"}\n";

	static const glw::GLchar* vs_template =
		"TEMPLATE_HEADER_DECLARATION\n"
		"\n"
		"in float clipdistance_data[TEMPLATE_CLIPDISTANCE_INPUT_SIZE];\n"
		"in float culldistance_data[TEMPLATE_CULLDISTANCE_INPUT_SIZE];\n"
		"in vec2  position;\n"
		"\n"
		"TEMPLATE_REDECLARE_CLIPDISTANCE\n"
		"TEMPLATE_REDECLARE_CULLDISTANCE\n"
		"\n"
		"#define ASSIGN_CLIP_DISTANCE(IDX) TEMPLATE_ASSIGN_CLIP_DISTANCE\n"
		"#define ASSIGN_CULL_DISTANCE(IDX) TEMPLATE_ASSIGN_CULL_DISTANCE\n"
		"\n"
		"/* Vertex shader main function */\n"
		"void main()\n"
		"{\n"
		"    /* Clipdistance and culldistance array setters */\n"
		"    {\n"
		"        TEMPLATE_ARRAY_SETTERS\n"
		"    }\n"
		"    gl_Position = vec4(2.0 * position.x - 1.0, 2.0 * position.y - 1.0, 0.0, 1.0);\n"
		"}\n";

	std::string* shader_body_string_fs	 = DE_NULL;
	std::string* shader_body_string_gs	 = DE_NULL;
	std::string* shader_body_string_tc	 = DE_NULL;
	std::string* shader_body_string_te	 = DE_NULL;
	std::string* shader_body_string_vs	 = DE_NULL;
	std::string  shader_header_declaration = use_core_functionality ? core_functionality : extention_functionality;

	struct _shaders_configuration
	{
		glw::GLenum		   type;
		const glw::GLchar* shader_template;
		std::string		   body;
		const bool		   use;
	} shaders_configuration[] = { {
									  GL_FRAGMENT_SHADER, fs_template, std::string(), true,
								  },
								  {
									  GL_GEOMETRY_SHADER, gs_template, std::string(), use_gs,
								  },
								  {
									  GL_TESS_CONTROL_SHADER, tc_template, std::string(), use_ts,
								  },
								  {
									  GL_TESS_EVALUATION_SHADER, te_template, std::string(), use_ts,
								  },
								  {
									  GL_VERTEX_SHADER, vs_template, std::string(), true,
								  } };

	const glw::GLuint n_shaders_configuration = sizeof(shaders_configuration) / sizeof(shaders_configuration[0]);

	/* Construct shader bodies out of templates */
	for (glw::GLuint n_shader_index = 0; n_shader_index < n_shaders_configuration; n_shader_index++)
	{
		if (shaders_configuration[n_shader_index].use)
		{
			std::string  array_setters;
			std::string  clipdistance_array_declaration;
			std::string  culldistance_array_declaration;
			std::string  clipdistance_in_array_declaration;
			std::string  culldistance_in_array_declaration;
			std::string& shader_source = shaders_configuration[n_shader_index].body;

			/* Copy template into shader body source */
			shader_source = shaders_configuration[n_shader_index].shader_template;

			CullDistance::Utilities::replaceAll(shader_source, std::string("TEMPLATE_HEADER_DECLARATION"),
												shader_header_declaration);

			/* Shader-specific actions */
			switch (shaders_configuration[n_shader_index].type)
			{
			case GL_FRAGMENT_SHADER:
			{
				shader_body_string_fs = &shaders_configuration[n_shader_index].body;

				if (fetch_culldistance_from_fs)
				{
					CullDistance::Utilities::replaceAll(shader_source, std::string("TEMPLATE_ASSIGN_RETURN_VALUE"),
														std::string(fetch_function));

					std::string fetch_sum_setters = "";
					for (glw::GLuint i = 0; i < clipdistances_array_size; ++i)
					{
						fetch_sum_setters.append("    sum += abs(gl_ClipDistance[");
						fetch_sum_setters.append(CullDistance::Utilities::intToString(i));
						fetch_sum_setters.append("]) * ");
						fetch_sum_setters.append(CullDistance::Utilities::intToString(i + 1));
						fetch_sum_setters.append(".0;\n");
					}

					fetch_sum_setters.append("\n");

					for (glw::GLuint i = 0; i < culldistances_array_size; ++i)
					{
						fetch_sum_setters.append("    sum += abs(gl_CullDistance[");
						fetch_sum_setters.append(CullDistance::Utilities::intToString(i));
						fetch_sum_setters.append("]) * ");
						fetch_sum_setters.append(
							CullDistance::Utilities::intToString(i + 1 + clipdistances_array_size));
						fetch_sum_setters.append(".0;\n");
					}

					CullDistance::Utilities::replaceAll(shader_source, std::string("TEMPLATE_SUM_SETTER"),
														std::string(fetch_sum_setters));
					CullDistance::Utilities::replaceAll(
						shader_source, std::string("TEMPLATE_SUM_DIVIDER"),
						std::string(CullDistance::Utilities::intToString(
										(clipdistances_array_size + culldistances_array_size) *
										((clipdistances_array_size + culldistances_array_size + 1))))
							.append(".0"));
				}
				else
				{
					CullDistance::Utilities::replaceAll(shader_source, std::string("TEMPLATE_ASSIGN_RETURN_VALUE"),
														std::string("#define ASSIGN_RETURN_VALUE 1.0"));
				}

				break;
			}

			case GL_GEOMETRY_SHADER:
			{
				shader_body_string_gs = &shaders_configuration[n_shader_index].body;

				CullDistance::Utilities::replaceAll(
					shader_source, std::string("TEMPLATE_ASSIGN_CLIP_DISTANCE"),
					std::string("gl_ClipDistance[IDX] = gl_in[n_vertex_index].gl_ClipDistance[IDX]"));
				CullDistance::Utilities::replaceAll(
					shader_source, std::string("TEMPLATE_ASSIGN_CULL_DISTANCE"),
					std::string("gl_CullDistance[IDX] = gl_in[n_vertex_index].gl_CullDistance[IDX]"));

				switch (primitive_mode)
				{
				case PRIMITIVE_MODE_LINES:
				{
					CullDistance::Utilities::replaceAll(shader_source, std::string("TEMPLATE_LAYOUT_IN"),
														std::string("layout(lines)                        in;"));
					CullDistance::Utilities::replaceAll(shader_source, std::string("TEMPLATE_LAYOUT_OUT"),
														std::string("layout(line_strip, max_vertices = 2) out;"));

					break;
				}
				case PRIMITIVE_MODE_POINTS:
				{
					CullDistance::Utilities::replaceAll(shader_source, std::string("TEMPLATE_LAYOUT_IN"),
														std::string("layout(points)                   in;"));
					CullDistance::Utilities::replaceAll(shader_source, std::string("TEMPLATE_LAYOUT_OUT"),
														std::string("layout(points, max_vertices = 1) out;"));

					break;
				}
				case PRIMITIVE_MODE_TRIANGLES:
				{
					CullDistance::Utilities::replaceAll(shader_source, std::string("TEMPLATE_LAYOUT_IN"),
														std::string("layout(triangles)                        in;"));
					CullDistance::Utilities::replaceAll(shader_source, std::string("TEMPLATE_LAYOUT_OUT"),
														std::string("layout(triangle_strip, max_vertices = 3) out;"));

					break;
				}
				default:
					TCU_FAIL("Unknown primitive mode");
				}

				break;
			}

			case GL_TESS_CONTROL_SHADER:
			{
				shader_body_string_tc = &shaders_configuration[n_shader_index].body;

				CullDistance::Utilities::replaceAll(
					shader_source, std::string("TEMPLATE_ASSIGN_CLIP_DISTANCE"),
					std::string(
						"gl_out[gl_InvocationID].gl_ClipDistance[IDX] = gl_in[gl_InvocationID].gl_ClipDistance[IDX]"));
				CullDistance::Utilities::replaceAll(
					shader_source, std::string("TEMPLATE_ASSIGN_CULL_DISTANCE"),
					std::string(
						"gl_out[gl_InvocationID].gl_CullDistance[IDX] = gl_in[gl_InvocationID].gl_CullDistance[IDX]"));

				switch (primitive_mode)
				{
				case PRIMITIVE_MODE_LINES:
				{
					CullDistance::Utilities::replaceAll(shader_source, std::string("TEMPLATE_LAYOUT_OUT"),
														std::string("layout(vertices = 2) out;"));

					break;
				}
				case PRIMITIVE_MODE_POINTS:
				{
					CullDistance::Utilities::replaceAll(shader_source, std::string("TEMPLATE_LAYOUT_OUT"),
														std::string("layout(vertices = 1) out;"));

					break;
				}
				case PRIMITIVE_MODE_TRIANGLES:
				{
					CullDistance::Utilities::replaceAll(shader_source, std::string("TEMPLATE_LAYOUT_OUT"),
														std::string("layout(vertices = 3) out;"));

					break;
				}
				default:
					TCU_FAIL("Unknown primitive mode");
				}

				break;
			}

			case GL_TESS_EVALUATION_SHADER:
			{
				shader_body_string_te = &shaders_configuration[n_shader_index].body;

				switch (primitive_mode)
				{
				case PRIMITIVE_MODE_LINES:
				{
					CullDistance::Utilities::replaceAll(shader_source, std::string("TEMPLATE_LAYOUT_IN"),
														std::string("layout(isolines) in;"));
					CullDistance::Utilities::replaceAll(
						shader_source, std::string("TEMPLATE_OUT_FORMULA"),
						std::string("mix(gl_in[0].gl_Position, gl_in[1].gl_Position, gl_TessCoord.x)"));
					CullDistance::Utilities::replaceAll(
						shader_source, std::string("TEMPLATE_ASSIGN_CLIP_DISTANCE"),
						std::string("gl_ClipDistance[IDX] = mix(gl_in[0].gl_ClipDistance[IDX], "
									"gl_in[1].gl_ClipDistance[IDX], gl_TessCoord.x)"));
					CullDistance::Utilities::replaceAll(
						shader_source, std::string("TEMPLATE_ASSIGN_CULL_DISTANCE"),
						std::string("gl_CullDistance[IDX] = mix(gl_in[0].gl_CullDistance[IDX], "
									"gl_in[1].gl_CullDistance[IDX], gl_TessCoord.x)"));

					break;
				}
				case PRIMITIVE_MODE_POINTS:
				{
					CullDistance::Utilities::replaceAll(shader_source, std::string("TEMPLATE_LAYOUT_IN"),
														std::string("layout(isolines, point_mode) in;"));
					CullDistance::Utilities::replaceAll(shader_source, std::string("TEMPLATE_OUT_FORMULA"),
														std::string("gl_in[0].gl_Position"));
					CullDistance::Utilities::replaceAll(
						shader_source, std::string("TEMPLATE_ASSIGN_CLIP_DISTANCE"),
						std::string("gl_ClipDistance[IDX] = gl_in[0].gl_ClipDistance[IDX]"));
					CullDistance::Utilities::replaceAll(
						shader_source, std::string("TEMPLATE_ASSIGN_CULL_DISTANCE"),
						std::string("gl_CullDistance[IDX] = gl_in[0].gl_CullDistance[IDX]"));

					break;
				}
				case PRIMITIVE_MODE_TRIANGLES:
				{
					CullDistance::Utilities::replaceAll(shader_source, std::string("TEMPLATE_LAYOUT_IN"),
														std::string("layout(triangles) in;"));
					CullDistance::Utilities::replaceAll(
						shader_source, std::string("TEMPLATE_OUT_FORMULA"),
						std::string("vec4(mat3(gl_in[0].gl_Position.xyz, gl_in[1].gl_Position.xyz, "
									"gl_in[2].gl_Position.xyz) * gl_TessCoord, 1.0)"));
					CullDistance::Utilities::replaceAll(
						shader_source, std::string("TEMPLATE_ASSIGN_CLIP_DISTANCE"),
						std::string("gl_ClipDistance[IDX] = dot(vec3(gl_in[0].gl_ClipDistance[IDX], "
									"gl_in[1].gl_ClipDistance[IDX], gl_in[2].gl_ClipDistance[IDX]), gl_TessCoord)"));
					CullDistance::Utilities::replaceAll(
						shader_source, std::string("TEMPLATE_ASSIGN_CULL_DISTANCE"),
						std::string("gl_CullDistance[IDX] = dot(vec3(gl_in[0].gl_CullDistance[IDX], "
									"gl_in[1].gl_CullDistance[IDX], gl_in[2].gl_CullDistance[IDX]), gl_TessCoord)"));

					break;
				}
				default:
					TCU_FAIL("Unknown primitive mode");
				}

				break;
			}

			case GL_VERTEX_SHADER:
			{
				shader_body_string_vs = &shaders_configuration[n_shader_index].body;

				/* Specify input data size for clipdistances data */
				CullDistance::Utilities::replaceAll(shader_source, std::string("TEMPLATE_CLIPDISTANCE_INPUT_SIZE"),
													CullDistance::Utilities::intToString(clipdistances_input_size));

				/* Specify input data size for culldistances data */
				CullDistance::Utilities::replaceAll(shader_source, std::string("TEMPLATE_CULLDISTANCE_INPUT_SIZE"),
													CullDistance::Utilities::intToString(culldistances_input_size));

				CullDistance::Utilities::replaceAll(shader_source, std::string("TEMPLATE_ASSIGN_CLIP_DISTANCE"),
													std::string("gl_ClipDistance[IDX] = clipdistance_data[IDX]"));
				CullDistance::Utilities::replaceAll(shader_source, std::string("TEMPLATE_ASSIGN_CULL_DISTANCE"),
													std::string("gl_CullDistance[IDX] = culldistance_data[IDX]"));

				break;
			}

			default:
				TCU_FAIL("Unknown shader type");
			}

			/* Adjust clipdistances declaration */
			if (redeclare_clipdistances && clipdistances_array_size > 0)
			{
				if (shaders_configuration[n_shader_index].type == GL_FRAGMENT_SHADER)
				{
					if (fetch_culldistance_from_fs)
					{
						clipdistance_array_declaration =
							std::string("in float gl_ClipDistance[") +
							CullDistance::Utilities::intToString(clipdistances_array_size) + std::string("];");
					}
				}
				else if (shaders_configuration[n_shader_index].type == GL_TESS_CONTROL_SHADER)
				{
					clipdistance_array_declaration = std::string("float gl_ClipDistance[") +
													 CullDistance::Utilities::intToString(clipdistances_array_size) +
													 std::string("];");
				}
				else
				{
					clipdistance_array_declaration = std::string("out float gl_ClipDistance[") +
													 CullDistance::Utilities::intToString(clipdistances_array_size) +
													 std::string("];");
					clipdistance_in_array_declaration = std::string("in float gl_ClipDistance[") +
														CullDistance::Utilities::intToString(clipdistances_array_size) +
														std::string("];");
				}
			}
			CullDistance::Utilities::replaceAll(shader_source, std::string("TEMPLATE_REDECLARE_CLIPDISTANCE"),
												clipdistance_array_declaration);
			CullDistance::Utilities::replaceAll(shader_source, std::string("TEMPLATE_REDECLARE_IN_CLIPDISTANCE"),
												clipdistance_in_array_declaration);

			/* Adjust culldistances declaration */
			if (redeclare_culldistances && culldistances_array_size > 0)
			{
				if (shaders_configuration[n_shader_index].type == GL_FRAGMENT_SHADER)
				{
					if (fetch_culldistance_from_fs)
					{
						culldistance_array_declaration =
							std::string("in float gl_CullDistance[") +
							CullDistance::Utilities::intToString(culldistances_array_size) + std::string("];");
					}
				}
				else if (shaders_configuration[n_shader_index].type == GL_TESS_CONTROL_SHADER)
				{
					culldistance_array_declaration = std::string("float gl_CullDistance[") +
													 CullDistance::Utilities::intToString(culldistances_array_size) +
													 std::string("];");
				}
				else
				{
					culldistance_array_declaration = std::string("out float gl_CullDistance[") +
													 CullDistance::Utilities::intToString(culldistances_array_size) +
													 std::string("];");
					culldistance_in_array_declaration = std::string("in float gl_CullDistance[") +
														CullDistance::Utilities::intToString(culldistances_array_size) +
														std::string("];");
				}
			}
			CullDistance::Utilities::replaceAll(shader_source, std::string("TEMPLATE_REDECLARE_CULLDISTANCE"),
												culldistance_array_declaration);
			CullDistance::Utilities::replaceAll(shader_source, std::string("TEMPLATE_REDECLARE_IN_CULLDISTANCE"),
												culldistance_in_array_declaration);

			/* Adjust clip/cull distances setters */
			if (dynamic_index_writes)
			{
				array_setters = dynamic_array_setters;

				CullDistance::Utilities::replaceAll(array_setters, std::string("TEMPLATE_N_GL_CLIPDISTANCE_ENTRIES"),
													CullDistance::Utilities::intToString(clipdistances_array_size));
				CullDistance::Utilities::replaceAll(array_setters, std::string("TEMPLATE_N_GL_CULLDISTANCE_ENTRIES"),
													CullDistance::Utilities::intToString(culldistances_array_size));
			}
			else
			{
				std::stringstream static_array_setters_sstream;

				static_array_setters_sstream << "\n";

				for (glw::GLuint clipdistances_array_entry = 0; clipdistances_array_entry < clipdistances_array_size;
					 ++clipdistances_array_entry)
				{
					static_array_setters_sstream << "        ASSIGN_CLIP_DISTANCE(" << clipdistances_array_entry
												 << ");\n";
				}

				static_array_setters_sstream << "\n";

				for (glw::GLuint culldistances_array_entry = 0; culldistances_array_entry < culldistances_array_size;
					 ++culldistances_array_entry)
				{
					static_array_setters_sstream << "        ASSIGN_CULL_DISTANCE(" << culldistances_array_entry
												 << ");\n";
				}

				array_setters = static_array_setters_sstream.str();
			}

			CullDistance::Utilities::replaceAll(shader_source, std::string("TEMPLATE_ARRAY_SETTERS"), array_setters);
		}
	}

	/* Build the geometry shader */
	CullDistance::Utilities::buildProgram(
		m_context.getRenderContext().getFunctions(), m_testCtx, DE_NULL, /* Compute shader                    */
		shader_body_string_fs != DE_NULL ? shader_body_string_fs->c_str() :
										   DE_NULL, /* Fragment shader                   */
		shader_body_string_gs != DE_NULL ? shader_body_string_gs->c_str() :
										   DE_NULL, /* Geometry shader                   */
		shader_body_string_tc != DE_NULL ? shader_body_string_tc->c_str() :
										   DE_NULL, /* Tesselation control shader        */
		shader_body_string_te != DE_NULL ? shader_body_string_te->c_str() :
										   DE_NULL, /* Tesselation evaluation shader     */
		shader_body_string_vs != DE_NULL ? shader_body_string_vs->c_str() :
										   DE_NULL, /* Vertex shader                     */
		0,											/* Transform feedback varyings count */
		DE_NULL,									/* Transform feedback varyings       */
		&m_po_id									/* Program object id                 */
		);
}

/** Generates primitive data required to test a case with specified
 *  gl_ClipDistance and glCullDistance array sizes for specified
 *  primitive mode. Generated primitive data is stored in m_bo_data
 *  as well uploaded into buffer specified in m_bo_id buffer.
 *  Also the procedure binds vertex attribute locations to
 *  program object m_po_id.
 *
 *  @param clipdistances_array_size gl_ClipDistance array size. Can be 0.
 *  @param culldistances_array_size gl_CullDistance array size. Can be 0.
 *  @param _primitive_mode          Primitives to be generated. Can be:
 *                                  PRIMITIVE_MODE_POINTS,
 *                                  PRIMITIVE_MODE_LINES,
 *                                  PRIMITIVE_MODE_TRIANGLES.
 */
void CullDistance::FunctionalTest::configureVAO(glw::GLuint clipdistances_array_size,
												glw::GLuint culldistances_array_size, _primitive_mode primitive_mode)
{
	/* Detailed test description.
	 *
	 * configureVAO() generates primitives layouted in grid. Primitve
	 * consists of up to 3 vertices and each vertex is accompanied by:
	 * - array of clipdistances (clipdistances_array_size floats);
	 * - array of culldistances (culldistances_array_size floats);
	 * - rendering position coordinates (x and y);
	 * - check position coordinates (x and y).
	 *
	 * The grid has following layout:
	 *
	 *     Grid                       |         gl_CullDistance[x]         |
	 *                                |  0 .. culldistances_array_size - 1 |
	 *                                |  0th  |  1st  |  2nd  | .......... |
	 *     ---------------------------+-------+-------+-------+------------+
	 *     0th  gl_ClipDistance       |Subgrid|Subgrid|Subgrid| .......... |
	 *     1st  gl_ClipDistance       |Subgrid|Subgrid|Subgrid| .......... |
	 *     ...                        |  ...  |  ...  |  ...  | .......... |
	 *     y-th gl_ClipDistance       |Subgrid|Subgrid|Subgrid| .......... |
	 *     ...                        |  ...  |  ...  |  ...  | .......... |
	 *     clipdistances_array_size-1 |Subgrid|Subgrid|Subgrid| .......... |
	 *
	 * Each grid cell contains subgrid of 3*3 items in size with following
	 * structure:
	 *
	 *     Subgrid        |        x-th gl_CullDistance test           |
	 *                    |                                            |
	 *     y-th           | all vertices | 0th vertex   | all vertices |
	 *     gl_ClipDistance| in primitive | in primitive | in primitive |
	 *     tests          | dist[x] > 0  | dist[x] < 0  | dist[x] < 0  |
	 *     ---------------+--------------+--------------+--------------+
	 *        all vertices| primitive #0 | primitive #1 | primitive #2 |
	 *        in primitive|              |              |              |
	 *        dist[y] > 0 |   visible    |   visible    |    culled    |
	 *     ---------------+--------------+--------------+--------------+
	 *        0th vertex  | primitive #3 | primitive #4 | primitive #5 |
	 *        in primitive|  0th vertex  |  0th vertex  |              |
	 *        dist[y] < 0 |   clipped    |   clipped    |    culled    |
	 *     ---------------+--------------+--------------+--------------+
	 *        all vertices| primitive #6 | primitive #7 | primitive #8 |
	 *        in primitive|              |              |              |
	 *        dist[y] < 0 |   clipped    |   clipped    |    culled    |
	 *     ---------------+--------------+--------------+--------------+
	 *
	 * Expected rendering result is specified in cell bottom.
	 * It can be one of the following:
	 * - "visible" means the primitive is not affected neither by gl_CullDistance
	 *             nor by gl_ClipDistance and rendered as a whole;
	 * - "clipped" for the vertex means the vertex is not rendered, while other
	 *             primitive vertices and some filling fragments are rendered;
	 * - "clipped" for primitive means none of primitive vertices and fragments
	 *             are rendered and thus primitive is not rendered and is invisible;
	 * - "culled"  means, that neither primitive vertices, nor primitive filling
	 *             fragments are rendered (primitive is invisible).
	 *
	 * All subgrid items contain same primitive rendered. Depending on
	 * test case running it would be either triangle, or line, or point:
	 *
	 *     triangle    line        point
	 *     8x8 box     8x8 box     3x3 box
	 *     ........    ........    ...
	 *     .0----2.    .0......    .0.
	 *     ..\@@@|.    ..\.....    ...
	 *     ...\@@|.    ...\....
	 *     ....\@|.    ....\...
	 *     .....\|.    .....\..
	 *     ......1.    ......1.
	 *     ........    ........
	 *
	 *     where 0 - is a 0th vertex primitive
	 *           1 - is a 1st vertex primitive
	 *           2 - is a 2nd vertex primitive
	 *
	 * The culldistances_array_size can be 0. In that case, grid height
	 * is assumed equal to 1, but 0 glCullDistances is specified.
	 * Similar handled clipdistances_array_size.
	 *
	 * The data generated is used and checked in executeRenderTest().
	 * After rendering each primitive vertex is tested:
	 * - if it is rendered, if it have to be rendered (according distance);
	 * - if it is not rendered, if it have to be not rendered (according distance).
	 * Due to "top-left" rasterization rule check position is
	 * different from rendering vertex position.
	 *
	 * Also one pixel width guarding box is checked to be clear.
	 */

	const glw::Functions& gl			   = m_context.getRenderContext().getFunctions();
	const glw::GLuint	 n_sub_grid_cells = 3; /* Tested distance is positive for all vertices in the primitive;
	 * Tested distance is negative for 0th vertex in the primitive;
	 * Tested distance is negative for all vertices in the primitive;
	 */
	const glw::GLuint	 sub_grid_cell_size =
		((primitive_mode == PRIMITIVE_MODE_LINES) ? 8 : (primitive_mode == PRIMITIVE_MODE_POINTS) ? 3 : 8);

	const glw::GLuint grid_cell_size = n_sub_grid_cells * sub_grid_cell_size;
	const glw::GLuint n_primitive_vertices =
		((primitive_mode == PRIMITIVE_MODE_LINES) ? 2 : (primitive_mode == PRIMITIVE_MODE_POINTS) ? 1 : 3);

	const glw::GLuint n_grid_cells_x			   = culldistances_array_size != 0 ? culldistances_array_size : 1;
	const glw::GLuint n_grid_cells_y			   = clipdistances_array_size != 0 ? clipdistances_array_size : 1;
	const glw::GLuint n_pervertex_float_attributes = clipdistances_array_size + culldistances_array_size +
													 2 /* vertex' draw x, y */ + 2 /* vertex' checkpoint x, y */;
	const glw::GLuint n_primitives_total	 = n_grid_cells_x * n_sub_grid_cells * n_grid_cells_y * n_sub_grid_cells;
	const glw::GLuint n_vertices_total		 = n_primitives_total * n_primitive_vertices;
	const glw::GLuint offsets_line_draw_x[2] = {
		1, sub_grid_cell_size - 1
	}; /* vertex x offsets to subgrid cell origin for line primitive     */
	const glw::GLuint offsets_line_draw_y[2] = {
		1, sub_grid_cell_size - 1
	}; /* vertex y offsets to subgrid cell origin for line primitive     */
	const glw::GLuint offsets_line_checkpoint_x[2] = {
		1, sub_grid_cell_size - 2
	}; /* pixel x offsets to subgrid cell origin for line primitive      */
	const glw::GLuint offsets_line_checkpoint_y[2] = {
		1, sub_grid_cell_size - 2
	}; /* pixel y offsets to subgrid cell origin for line primitive      */
	const glw::GLuint offsets_point_draw_x[1] = {
		1
	}; /* vertex x offsets to subgrid cell origin for point primitive    */
	const glw::GLuint offsets_point_draw_y[1] = {
		1
	}; /* vertex y offsets to subgrid cell origin for point primitive    */
	const glw::GLuint offsets_point_checkpoint_x[1] = {
		1
	}; /* pixel x offsets to subgrid cell origin for point primitive     */
	const glw::GLuint offsets_point_checkpoint_y[1] = {
		1
	}; /* pixel y offsets to subgrid cell origin for point primitive     */
	const glw::GLuint offsets_triangle_draw_x[3] = {
		1, sub_grid_cell_size - 1, sub_grid_cell_size - 1
	}; /* vertex x offsets to subgrid cell origin for triangle primitive */
	const glw::GLuint offsets_triangle_draw_y[3] = {
		1, sub_grid_cell_size - 1, 1
	}; /* vertex y offsets to subgrid cell origin for triangle primitive */
	const glw::GLuint offsets_triangle_checkpoint_x[3] = {
		1, sub_grid_cell_size - 2, sub_grid_cell_size - 2
	}; /* pixel x offsets to subgrid cell origin for triangle primitive  */
	const glw::GLuint offsets_triangle_checkpoint_y[3] = {
		1, sub_grid_cell_size - 2, 1
	}; /* pixel y offsets to subgrid cell origin for triangle primitive  */
	const glw::GLfloat offsets_pixel_center_x = (primitive_mode == PRIMITIVE_MODE_POINTS) ? 0.5f : 0;
	const glw::GLfloat offsets_pixel_center_y = (primitive_mode == PRIMITIVE_MODE_POINTS) ? 0.5f : 0;
	/* Clear data left from previous tests. */
	m_bo_data.clear();

	/* No data to render */
	m_render_primitives = 0;
	m_render_vertices   = 0;

	/* Preallocate space for bo_points_count */
	m_bo_data.reserve(n_vertices_total * n_pervertex_float_attributes);

	/* Generate test data for cell_y-th clip distance */
	for (glw::GLuint cell_y = 0; cell_y < n_grid_cells_y; cell_y++)
	{
		/* Generate test data for cell_x-th cull distance */
		for (glw::GLuint cell_x = 0; cell_x < n_grid_cells_x; cell_x++)
		{
			/* Check clip distance sub cases:
			 * 0. Tested distance is positive for all vertices in the primitive;
			 * 1. Tested distance is negative for 0th vertex in the primitive;
			 * 2. Tested distance is negative for all vertices in the primitive;
			 */
			for (glw::GLuint n_sub_cell_y = 0; n_sub_cell_y < n_sub_grid_cells; n_sub_cell_y++)
			{
				/* Check cull distance sub cases:
				 * 0. Tested distance is positive for all vertices in the primitive;
				 * 1. Tested distance is negative for 0th vertex in the primitive;
				 * 2. Tested distance is negative for all vertices in the primitive;
				 */
				for (glw::GLuint n_sub_cell_x = 0; n_sub_cell_x < n_sub_grid_cells; n_sub_cell_x++)
				{
					/* Generate vertices in primitive */
					for (glw::GLuint n_primitive_vertex = 0; n_primitive_vertex < n_primitive_vertices;
						 n_primitive_vertex++)
					{
						/* Fill in clipdistance array for the n_primitive_vertex vertex in primitive */
						for (glw::GLuint n_clipdistance_entry = 0; n_clipdistance_entry < clipdistances_array_size;
							 n_clipdistance_entry++)
						{
							glw::GLfloat distance_value = 0.0f;
							bool		 negative		= true;

							/* Special approach to tested clipdistance entry. */
							if (n_clipdistance_entry == cell_y)
							{
								/* The primitive vertex should be affected by the clip distance */
								switch (n_sub_cell_y)
								{
								case 0:
								{
									/* subgrid row 0: all primitive vertices have tested distance value positive */
									negative = false;

									break;
								}
								case 1:
								{
									/* subgrid row 1: tested distance value for 0th primitive vertex is negative,
									 all other primitive vertices have tested distance value positive */
									negative = (n_primitive_vertex == 0) ? true : false;

									break;
								}
								case 2:
								{
									/* subgrid row 2: tested distance value is negative for all primitive vertices */
									negative = true;

									break;
								}
								default:
									TCU_FAIL("Invalid subgrid cell index");
								}

								distance_value = (negative ? -1.0f : 1.0f) * glw::GLfloat(n_clipdistance_entry + 1);
							}
							else
							{
								/* For clip distances other than tested: assign positive value to avoid its influence. */
								distance_value = glw::GLfloat(clipdistances_array_size + n_clipdistance_entry + 1);
							}

							m_bo_data.push_back(distance_value / glw::GLfloat(clipdistances_array_size));
						} /* for (all gl_ClipDistance[] array values) */

						/* Fill in culldistance array for the n_primitive_vertex vertex in primitive */
						for (glw::GLuint n_culldistance_entry = 0; n_culldistance_entry < culldistances_array_size;
							 n_culldistance_entry++)
						{
							glw::GLfloat distance_value = 0.0f;
							bool		 negative		= true;

							/* Special approach to tested culldistance entry. */
							if (n_culldistance_entry == cell_x)
							{
								/* The primitive vertex should be affected by the cull distance */
								switch (n_sub_cell_x)
								{
								case 0:
								{
									/* subgrid column 0: all primitive vertices have tested distance value positive */
									negative = false;

									break;
								}
								case 1:
								{
									/* subgrid column 1: tested distance value for 0th primitive vertex is negative,
									 all other primitive vertices have tested distance value positive */
									negative = (n_primitive_vertex == 0) ? true : false;

									break;
								}
								case 2:
								{
									/* subgrid column 2: tested distance value is negative for all primitive vertices */
									negative = true;

									break;
								}
								default:
									TCU_FAIL("Invalid subgrid cell index");
								}

								distance_value = (negative ? -1.0f : 1.0f) * glw::GLfloat(n_culldistance_entry + 1);
							}
							else
							{
								/* For cull distances other than tested: assign 0th vertex negative value,
								 to check absence of between-distances influence. */
								if (n_primitive_vertices > 1 && n_primitive_vertex == 0)
								{
									distance_value = -glw::GLfloat(culldistances_array_size + n_culldistance_entry + 1);
								}
								else
								{
									/* This culldistance is out of interest: assign positive value. */
									distance_value = glw::GLfloat(culldistances_array_size + n_culldistance_entry + 1);
								}
							}

							m_bo_data.push_back(distance_value / glw::GLfloat(culldistances_array_size));
						} /* for (all gl_CullDistance[] array values) */

						/* Generate primitve vertex draw and checkpoint coordinates */
						glw::GLint vertex_draw_pixel_offset_x		= 0;
						glw::GLint vertex_draw_pixel_offset_y		= 0;
						glw::GLint vertex_checkpoint_pixel_offset_x = 0;
						glw::GLint vertex_checkpoint_pixel_offset_y = 0;

						switch (primitive_mode)
						{
						case PRIMITIVE_MODE_LINES:
						{
							vertex_draw_pixel_offset_x		 = offsets_line_draw_x[n_primitive_vertex];
							vertex_draw_pixel_offset_y		 = offsets_line_draw_y[n_primitive_vertex];
							vertex_checkpoint_pixel_offset_x = offsets_line_checkpoint_x[n_primitive_vertex];
							vertex_checkpoint_pixel_offset_y = offsets_line_checkpoint_y[n_primitive_vertex];

							break;
						}

						case PRIMITIVE_MODE_POINTS:
						{
							vertex_draw_pixel_offset_x		 = offsets_point_draw_x[n_primitive_vertex];
							vertex_draw_pixel_offset_y		 = offsets_point_draw_y[n_primitive_vertex];
							vertex_checkpoint_pixel_offset_x = offsets_point_checkpoint_x[n_primitive_vertex];
							vertex_checkpoint_pixel_offset_y = offsets_point_checkpoint_y[n_primitive_vertex];

							break;
						}

						case PRIMITIVE_MODE_TRIANGLES:
						{
							vertex_draw_pixel_offset_x		 = offsets_triangle_draw_x[n_primitive_vertex];
							vertex_draw_pixel_offset_y		 = offsets_triangle_draw_y[n_primitive_vertex];
							vertex_checkpoint_pixel_offset_x = offsets_triangle_checkpoint_x[n_primitive_vertex];
							vertex_checkpoint_pixel_offset_y = offsets_triangle_checkpoint_y[n_primitive_vertex];

							break;
						}

						default:
							TCU_FAIL("Unknown primitive mode");
						}

						/* Origin of sub_cell */
						glw::GLint sub_cell_origin_x = cell_x * grid_cell_size + n_sub_cell_x * sub_grid_cell_size;
						glw::GLint sub_cell_origin_y = cell_y * grid_cell_size + n_sub_cell_y * sub_grid_cell_size;
						/* Normalized texture coordinates of vertex draw position. */
						glw::GLfloat x =
							(glw::GLfloat(sub_cell_origin_x + vertex_draw_pixel_offset_x) + offsets_pixel_center_x) /
							glw::GLfloat(m_to_width);
						glw::GLfloat y =
							(glw::GLfloat(sub_cell_origin_y + vertex_draw_pixel_offset_y) + offsets_pixel_center_y) /
							glw::GLfloat(m_to_height);
						/* Normalized texture coordinates of vertex checkpoint position. */
						glw::GLfloat checkpoint_x = glw::GLfloat(sub_cell_origin_x + vertex_checkpoint_pixel_offset_x) /
													glw::GLfloat(m_to_width);
						glw::GLfloat checkpoint_y = glw::GLfloat(sub_cell_origin_y + vertex_checkpoint_pixel_offset_y) /
													glw::GLfloat(m_to_height);

						/* Add vertex draw coordinates into buffer. */
						m_bo_data.push_back(x);
						m_bo_data.push_back(y);

						/* Add vertex checkpoint coordinates into buffer. */
						m_bo_data.push_back(checkpoint_x);
						m_bo_data.push_back(checkpoint_y);
					} /* for (all vertices in primitive) */
				}	 /* for (all horizontal sub cells) */
			}		  /* for (all vertical sub cells) */
		}			  /* for (all horizontal cells) */
	}				  /* for (all vertical cells) */

	/* Sanity check: make sure we pushed required amount of data */
	DE_ASSERT(m_bo_data.size() == n_vertices_total * n_pervertex_float_attributes);

	/* Save number of primitives to render */
	m_render_primitives  = n_primitives_total;
	m_render_vertices	= n_vertices_total;
	m_sub_grid_cell_size = sub_grid_cell_size;

	/* Copy the data to the buffer object */
	gl.bindBuffer(GL_ARRAY_BUFFER, m_bo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer() call failed.");

	gl.bufferData(GL_ARRAY_BUFFER, m_bo_data.size() * sizeof(glw::GLfloat), &m_bo_data[0], GL_STATIC_DRAW);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferData() call failed.");

	DE_ASSERT(m_po_id != 0);

	/* Bind VAO data to program */
	glw::GLint po_clipdistance_array_location = -1;
	glw::GLint po_culldistance_array_location = -1;
	glw::GLint po_position_location			  = -1;

	/* Retrieve clipdistance and culldistance attribute locations */
	gl.bindVertexArray(m_vao_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindVertexArray() call failed.");

	po_clipdistance_array_location = gl.getAttribLocation(m_po_id, "clipdistance_data[0]");
	po_culldistance_array_location = gl.getAttribLocation(m_po_id, "culldistance_data[0]");
	po_position_location		   = gl.getAttribLocation(m_po_id, "position");

	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetAttribLocation() call(s) failed.");

	if (clipdistances_array_size > 0)
	{
		DE_ASSERT(po_clipdistance_array_location != -1);
	}

	if (culldistances_array_size > 0)
	{
		DE_ASSERT(po_culldistance_array_location != -1);
	}

	DE_ASSERT(po_position_location != -1);

	glw::GLintptr	current_offset = 0;
	const glw::GLint stride			= static_cast<glw::GLint>(n_pervertex_float_attributes * sizeof(glw::GLfloat));

	gl.bindVertexArray(m_vao_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindVertexArray() call failed.");

	for (glw::GLuint n_clipdistance_entry = 0; n_clipdistance_entry < clipdistances_array_size; ++n_clipdistance_entry)
	{
		gl.vertexAttribPointer(po_clipdistance_array_location + n_clipdistance_entry, 1, /* size */
							   GL_FLOAT, GL_FALSE,										 /* normalized */
							   stride, (const glw::GLvoid*)current_offset);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glVertexAttribPointer() call failed.");

		gl.enableVertexAttribArray(po_clipdistance_array_location + n_clipdistance_entry);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glEnableVertexAttribArray() call failed.");

		current_offset += sizeof(glw::GLfloat);
	} /* for (all clip distance array value attributes) */

	for (glw::GLuint n_culldistance_entry = 0; n_culldistance_entry < culldistances_array_size; ++n_culldistance_entry)
	{
		gl.vertexAttribPointer(po_culldistance_array_location + n_culldistance_entry, 1, /* size */
							   GL_FLOAT, GL_FALSE,										 /* normalized */
							   stride, (const glw::GLvoid*)current_offset);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glVertexAttribPointer() call failed.");

		gl.enableVertexAttribArray(po_culldistance_array_location + n_culldistance_entry);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glEnableVertexAttribArray() call failed.");

		current_offset += sizeof(glw::GLfloat);
	} /* for (all cull distance array value attributes) */

	gl.vertexAttribPointer(po_position_location, 2, /* size */
						   GL_FLOAT, GL_FALSE,		/* normalized */
						   stride, (const glw::GLvoid*)current_offset);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glVertexAttribPointer() call failed");

	gl.enableVertexAttribArray(po_position_location);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glEnableVertexAttribArray() call failed");
}

/** @brief Cull Distance Functional Test deinitialization */
void CullDistance::FunctionalTest::deinit()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (m_fbo_id != 0)
	{
		gl.deleteFramebuffers(1, &m_fbo_id);

		m_fbo_id = 0;
	}

	if (m_to_id != 0)
	{
		gl.deleteTextures(1, &m_to_id);

		m_to_id = 0;
	}

	if (m_vao_id != 0)
	{
		gl.deleteVertexArrays(1, &m_vao_id);

		m_vao_id = 0;
	}

	deinitPO();
}

/** @brief Cull Distance Functional Test deinitialization of OpenGL programs */
void CullDistance::FunctionalTest::deinitPO()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (m_po_id != 0)
	{
		gl.deleteProgram(m_po_id);

		m_po_id = 0;
	}
}

/** @brief Executes single render test case
 *
 * @param [in]  clipdistances_array_size    Size of gl_ClipDistance[] array
 * @param [in]  culldistances_array_size    Size of gl_CullDistance[] array
 * @param [in]  primitive_mode              Type of primitives to be rendered (see enum _primitive_mode)
 * @param [in]  use_tesselation             Indicate whether to use tessellation shader
 * @param [in]  fetch_culldistance_from_fs  Indicate whether to fetch gl_CullDistance and gl_ClipDistance values from the fragment shader
 */
void CullDistance::FunctionalTest::executeRenderTest(glw::GLuint	 clipdistances_array_size,
													 glw::GLuint	 culldistances_array_size,
													 _primitive_mode primitive_mode, bool use_tesselation,
													 bool fetch_culldistance_from_fs)
{
	const glw::Functions& gl						  = m_context.getRenderContext().getFunctions();
	glw::GLenum			  mode						  = GL_NONE;
	glw::GLuint			  n_clipped_vertices_real	 = 0;
	glw::GLuint			  n_culled_primitives_real	= 0;
	glw::GLuint			  n_not_clipped_vertices_real = 0;
	const glw::GLuint	 primitive_vertices_count =
		((primitive_mode == PRIMITIVE_MODE_LINES) ? 2 : (primitive_mode == PRIMITIVE_MODE_POINTS) ? 1 : 3);
	const glw::GLuint stride_in_floats =
		clipdistances_array_size + culldistances_array_size + 2 /* position's x, y*/ + 2 /* checkpoint x,y */;

	switch (primitive_mode)
	{
	case PRIMITIVE_MODE_LINES:
	{
		mode = GL_LINES;

		break;
	}
	case PRIMITIVE_MODE_POINTS:
	{
		mode = GL_POINTS;

		break;
	}
	case PRIMITIVE_MODE_TRIANGLES:
	{
		mode = GL_TRIANGLES;

		break;
	}
	default:
		TCU_FAIL("Unknown primitive mode");
	}

	if (use_tesselation)
	{
		mode = GL_PATCHES;

		gl.patchParameteri(GL_PATCH_VERTICES, primitive_vertices_count);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glPatchParameteri() call failed.");
	}

	gl.clear(GL_COLOR_BUFFER_BIT);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glClear() call failed.");

	gl.useProgram(m_po_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram() call failed.");

	for (glw::GLuint n_clipdistance_entry = 0; n_clipdistance_entry < clipdistances_array_size; n_clipdistance_entry++)
	{
		gl.enable(GL_CLIP_DISTANCE0 + n_clipdistance_entry);
		GLU_EXPECT_NO_ERROR(gl.getError(), "gl.enable(GL_CLIP_DISTANCE)() call failed.");
	} /* for (all clip distance array value attributes) */

	gl.drawArrays(mode, 0, m_render_vertices);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawArray() call(s) failed.");

	for (glw::GLuint n_clipdistance_entry = 0; n_clipdistance_entry < clipdistances_array_size; n_clipdistance_entry++)
	{
		gl.disable(GL_CLIP_DISTANCE0 + n_clipdistance_entry);
		GLU_EXPECT_NO_ERROR(gl.getError(), "gl.disable(GL_CLIP_DISTANCE)() call failed.");
	} /* for (all clip distance array value attributes) */

	gl.useProgram(0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram() call failed.");

	/* Read generated texture into m_to_pixel_data_cache */
	readTexturePixels();

	for (glw::GLint n_primitive_index = 0; n_primitive_index < m_render_primitives; n_primitive_index++)
	{
		glw::GLuint base_index_of_primitive		 = n_primitive_index * primitive_vertices_count * stride_in_floats;
		bool		primitive_culled			 = false;
		glw::GLint  primitive_culled_by_distance = -1;

		/* Check the bounding box is clear */
		glw::GLuint base_index_of_vertex	  = base_index_of_primitive;
		glw::GLuint checkpoint_position_index = base_index_of_vertex + clipdistances_array_size +
												culldistances_array_size + 2 /* ignore vertex coordinates */;
		glw::GLint checkpoint_x = glw::GLint(glw::GLfloat(m_to_width) * m_bo_data[checkpoint_position_index]);
		glw::GLint checkpoint_y = glw::GLint(glw::GLfloat(m_to_height) * m_bo_data[checkpoint_position_index + 1]);
		glw::GLint origin_x		= checkpoint_x - 1;
		glw::GLint origin_y		= checkpoint_y - 1;
		for (glw::GLint pixel_offset = 0; pixel_offset < m_sub_grid_cell_size; pixel_offset++)
		{
			if (readRedPixelValue(origin_x + pixel_offset, origin_y) != 0)
			{
				TCU_FAIL("Top edge of bounding box is overwritten");
			}

			if (readRedPixelValue(origin_x + m_sub_grid_cell_size - 1, origin_y + pixel_offset) != 0)
			{
				TCU_FAIL("Right edge of bounding box is overwritten");
			}

			if (readRedPixelValue(origin_x + m_sub_grid_cell_size - 1 - pixel_offset,
								  origin_y + m_sub_grid_cell_size - 1) != 0)
			{
				TCU_FAIL("Bottom edge of bounding box is overwritten");
			}

			if (readRedPixelValue(origin_x, origin_y + m_sub_grid_cell_size - 1 - pixel_offset) != 0)
			{
				TCU_FAIL("Left edge of bounding box is overwritten");
			}
		}

		/* Determine if primitive has been culled */
		for (glw::GLuint n_culldistance_entry = 0; n_culldistance_entry < culldistances_array_size;
			 n_culldistance_entry++)
		{
			bool distance_negative_in_all_primitive_vertices = true;

			for (glw::GLuint n_primitive_vertex = 0; n_primitive_vertex < primitive_vertices_count;
				 n_primitive_vertex++)
			{
				glw::GLint base_index_of_vertex_internal =
					base_index_of_primitive + n_primitive_vertex * stride_in_floats;
				glw::GLint	culldistance_array_offset = base_index_of_vertex_internal + clipdistances_array_size;
				glw::GLfloat* vertex_culldistance_array = &m_bo_data[culldistance_array_offset];

				if (vertex_culldistance_array[n_culldistance_entry] >= 0)
				{
					/* Primitive is not culled, due to one of its distances is not negative */
					distance_negative_in_all_primitive_vertices = false;

					/* Skip left vertices for this distance */
					break;
				}
			}

			/* The distance is negative in all primitive vertices, so this distance culls the primitive */
			if (distance_negative_in_all_primitive_vertices)
			{
				primitive_culled			 = true;
				primitive_culled_by_distance = n_culldistance_entry;

				n_culled_primitives_real++;

				/* Skip left distances from check */
				break;
			}
		}

		/* Validate culling */
		if (primitive_culled)
		{
			/* Check whether primitive was culled and all its vertices are invisible */
			for (glw::GLuint n_primitive_vertex = 0; n_primitive_vertex < primitive_vertices_count;
				 n_primitive_vertex++)
			{
				glw::GLint base_index_of_vertex_internal =
					base_index_of_primitive + n_primitive_vertex * stride_in_floats;
				glw::GLint checkpoint_position_index_internal = base_index_of_vertex_internal +
																clipdistances_array_size + culldistances_array_size +
																2 /* ignore vertex coordinates */;
				glw::GLint checkpoint_x_internal =
					glw::GLint(glw::GLfloat(m_to_width) * m_bo_data[checkpoint_position_index_internal]);
				glw::GLint checkpoint_y_internal =
					glw::GLint(glw::GLfloat(m_to_height) * m_bo_data[checkpoint_position_index_internal + 1]);
				glw::GLint vertex_color_red_value = readRedPixelValue(checkpoint_x_internal, checkpoint_y_internal);

				/* Make sure vertex is invisible */
				if (vertex_color_red_value != 0)
				{
					m_testCtx.getLog() << tcu::TestLog::Message << "Primitive number [" << n_primitive_index << "] "
									   << "should be culled by distance [" << primitive_culled_by_distance << "]"
									   << "but primitive vertex at (" << checkpoint_x << "," << checkpoint_y
									   << ") is visible." << tcu::TestLog::EndMessage;

					TCU_FAIL("Primitive is expected to be culled, but one of its vertices is visible.");
				}
			}

			/* Primitive is culled, no reason to check clipping */
			continue;
		}

		bool all_vertices_are_clipped = true;

		for (glw::GLuint n_primitive_vertex = 0; n_primitive_vertex < primitive_vertices_count; n_primitive_vertex++)
		{
			glw::GLuint base_index_of_vertex_internal = base_index_of_primitive + n_primitive_vertex * stride_in_floats;
			glw::GLuint clipdistance_array_index	  = base_index_of_vertex_internal;
			glw::GLuint checkpoint_position_index_internal = base_index_of_vertex_internal + clipdistances_array_size +
															 culldistances_array_size +
															 2 /* ignore vertex coordinates */;
			glw::GLint checkpoint_x_internal =
				glw::GLint(glw::GLfloat(m_to_width) * m_bo_data[checkpoint_position_index_internal]);
			glw::GLint checkpoint_y_internal =
				glw::GLint(glw::GLfloat(m_to_height) * m_bo_data[checkpoint_position_index_internal + 1]);
			glw::GLfloat* vertex_clipdistance_array  = &m_bo_data[clipdistance_array_index];
			bool		  vertex_clipped			 = false;
			glw::GLint	vertex_clipped_by_distance = 0;
			glw::GLint	vertex_color_red_value	 = readRedPixelValue(checkpoint_x_internal, checkpoint_y_internal);

			/* Check whether pixel should be clipped */
			for (glw::GLuint n_clipdistance_entry = 0; n_clipdistance_entry < clipdistances_array_size;
				 n_clipdistance_entry++)
			{
				if (vertex_clipdistance_array[n_clipdistance_entry] < 0)
				{
					vertex_clipped			   = true;
					vertex_clipped_by_distance = n_clipdistance_entry;

					break;
				}
			}

			all_vertices_are_clipped &= vertex_clipped;

			/* Validate whether real data same as expected */
			if (vertex_clipped)
			{
				if (vertex_color_red_value != 0)
				{
					m_testCtx.getLog() << tcu::TestLog::Message << "In primitive number [" << n_primitive_index << "] "
									   << "vertex at (" << checkpoint_x << "," << checkpoint_y << ") "
									   << "should be clipped by distance [" << vertex_clipped_by_distance << "] "
									   << "(distance value [" << vertex_clipdistance_array[vertex_clipped_by_distance]
									   << "])" << tcu::TestLog::EndMessage;

					TCU_FAIL("Vertex is expected to be clipped and invisible, while it is visible.");
				}
				else
				{
					n_clipped_vertices_real++;
				}
			}
			else
			{
				if (vertex_color_red_value == 0)
				{
					m_testCtx.getLog() << tcu::TestLog::Message << "In primitive number [" << n_primitive_index << "] "
									   << "vertex at (" << checkpoint_x << "," << checkpoint_y << ") "
									   << "should not be clipped." << tcu::TestLog::EndMessage;

					TCU_FAIL("Vertex is unexpectedly clipped or invisible");
				}
				else
				{
					n_not_clipped_vertices_real++;
				}
			}
		}

		if (!all_vertices_are_clipped)
		{
			/* Check fetched values from the shader (Point 2 of Basic Outline : "Use program that...") */
			if (fetch_culldistance_from_fs)
			{
				for (glw::GLuint n_primitive_vertex = 0; n_primitive_vertex < primitive_vertices_count;
					 n_primitive_vertex++)
				{
					/* Get shader output value */
					glw::GLuint base_index_of_vertex_internal =
						base_index_of_primitive + n_primitive_vertex * stride_in_floats;
					glw::GLuint checkpoint_position_index_internal =
						base_index_of_vertex_internal + clipdistances_array_size + culldistances_array_size +
						2 /* ignore vertex coordinates */;
					glw::GLuint culldistances_index = base_index_of_vertex_internal + clipdistances_array_size;
					glw::GLint  checkpoint_x_internal =
						glw::GLint(glw::GLfloat(m_to_width) * m_bo_data[checkpoint_position_index_internal]);
					glw::GLint checkpoint_y_internal =
						glw::GLint(glw::GLfloat(m_to_height) * m_bo_data[checkpoint_position_index_internal + 1]);
					glw::GLint vertex_color_red_value = readRedPixelValue(checkpoint_x_internal, checkpoint_y_internal);

					/* Calculate culldistances check sum hash */
					float sum = 0.f;

					for (glw::GLuint n_clipdistance_entry = 0; n_clipdistance_entry < clipdistances_array_size;
						 ++n_clipdistance_entry)
					{
						sum += de::abs(m_bo_data[base_index_of_vertex_internal + n_clipdistance_entry]) *
							   float(n_clipdistance_entry + 1);
					}

					for (glw::GLuint n_culldistance_entry = 0; n_culldistance_entry < culldistances_array_size;
						 ++n_culldistance_entry)
					{
						sum += de::abs(m_bo_data[culldistances_index + n_culldistance_entry]) *
							   float(n_culldistance_entry + 1 + clipdistances_array_size);
					}

					/* limit sum and return */
					glw::GLint sum_hash =
						glw::GLint(sum / glw::GLfloat((clipdistances_array_size + culldistances_array_size) *
													  (clipdistances_array_size + culldistances_array_size + 1)) *
								   65535.f /* normalizing to short */);
					sum_hash = (sum_hash < 65536) ? sum_hash : 65535; /* clamping to short */

					/* Compare against setup value */
					if (std::abs(vertex_color_red_value - sum_hash) > 4 /* precision 4/65536 */)
					{
						m_testCtx.getLog() << tcu::TestLog::Message << "Primitive number [" << n_primitive_index << "] "
										   << "should have culldistance hash sum " << sum_hash
										   << "but primitive vertex at (" << checkpoint_x << "," << checkpoint_y
										   << ") has sum hash equal to " << vertex_color_red_value
										   << tcu::TestLog::EndMessage;

						TCU_FAIL("Culled distances returned from fragment shader dose not match expected values.");
					}
				}
			}
		}
	}

	/* sub_grid cell size is 3*3 */
	DE_ASSERT(m_render_primitives % 9 == 0);

	/* Sanity check */
	switch (primitive_mode)
	{
	case PRIMITIVE_MODE_LINES:
	case PRIMITIVE_MODE_TRIANGLES:
	{
		/* Validate culled primitives */
		if (culldistances_array_size == 0)
		{
			DE_ASSERT(n_culled_primitives_real == 0);
		}
		else
		{
			/* Each 3rd line or triangle should be culled by test design */
			DE_ASSERT(glw::GLsizei(n_culled_primitives_real) == m_render_primitives / 3);
		}

		/* Validate clipped vertices */
		if (clipdistances_array_size == 0)
		{
			DE_ASSERT(n_clipped_vertices_real == 0);
		}
		else
		{
#if defined(DE_DEBUG) && !defined(DE_COVERAGE_BUILD)
			glw::GLint one_third_of_rendered_primitives = (m_render_primitives - n_culled_primitives_real) / 3;
			glw::GLint n_clipped_vertices_expected		= /* One third of primitives has 0th vertex clipped */
				one_third_of_rendered_primitives +
				/* One third of primitives clipped completely     */
				one_third_of_rendered_primitives * primitive_vertices_count;

			DE_ASSERT(glw::GLint(n_clipped_vertices_real) == n_clipped_vertices_expected);
#endif
		}
		break;
	}

	case PRIMITIVE_MODE_POINTS:
	{
		/* Validate culled primitives */
		if (culldistances_array_size == 0)
		{
			DE_ASSERT(n_culled_primitives_real == 0);
		}
		else
		{
			/* 2/3 points should be culled by test design */
			DE_ASSERT(glw::GLsizei(n_culled_primitives_real) == m_render_primitives * 2 / 3);
		}

		/* Validate clipped vertices */
		if (clipdistances_array_size == 0)
		{
			DE_ASSERT(n_clipped_vertices_real == 0);
		}
		else
		{
#if defined(DE_DEBUG) && !defined(DE_COVERAGE_BUILD)
			glw::GLint one_third_of_rendered_primitives = (m_render_primitives - n_culled_primitives_real) / 3;

			/* 2/3 of rendered points should be clipped by test design */
			DE_ASSERT(glw::GLint(n_clipped_vertices_real) == 2 * one_third_of_rendered_primitives);
#endif
		}

		break;
	}
	default:
		TCU_FAIL("Unknown primitive mode");
	}
}

/** Executes test iteration.
 *
 *  @return Returns STOP when test has finished executing, CONTINUE if more iterations are needed.
 */
tcu::TestNode::IterateResult CullDistance::FunctionalTest::iterate()
{
	/* This test should only be executed if ARB_cull_distance is supported, or if
	 * we're running a GL4.5 context
	 */
	if (!m_context.getContextInfo().isExtensionSupported("GL_ARB_cull_distance") &&
		!glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::core(4, 5)))
	{
		throw tcu::NotSupportedError("GL_ARB_cull_distance is not supported");
	}

	const glw::Functions& gl			= m_context.getRenderContext().getFunctions();
	bool				  has_succeeded = true;
	bool is_core = glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::core(4, 5));

	/* Retrieve important GL constant values */
	glw::GLint gl_max_clip_distances_value					 = 0;
	glw::GLint gl_max_combined_clip_and_cull_distances_value = 0;
	glw::GLint gl_max_cull_distances_value					 = 0;

	gl.getIntegerv(GL_MAX_CLIP_DISTANCES, &gl_max_clip_distances_value);
	gl.getIntegerv(GL_MAX_COMBINED_CLIP_AND_CULL_DISTANCES, &gl_max_combined_clip_and_cull_distances_value);
	gl.getIntegerv(GL_MAX_CULL_DISTANCES, &gl_max_cull_distances_value);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv() call(s) failed.");

	gl.genTextures(1, &m_to_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenTextures() call failed.");

	gl.bindTexture(GL_TEXTURE_2D, m_to_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture() call failed.");

	gl.texStorage2D(GL_TEXTURE_2D, 1, /* levels */
					GL_R32F, m_to_width, m_to_height);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTexStorage2D() call failed.");

	/* Set up the draw/read FBO */
	gl.genFramebuffers(1, &m_fbo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenFramebuffers() call failed.");

	gl.bindFramebuffer(GL_FRAMEBUFFER, m_fbo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffer() call failed.");

	gl.framebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_to_id, 0); /* level */
	GLU_EXPECT_NO_ERROR(gl.getError(), "glFramebufferTexture2D() call failed.");

	/* Prepare a buffer object */
	gl.genBuffers(1, &m_bo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenBuffers() call failed.");

	/* Prepare a VAO. We will configure separately for each iteration. */
	gl.genVertexArrays(1, &m_vao_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenVertexArrays() call failed.");

	/* Iterate over all functional tests */
	struct _test_item
	{
		bool redeclare_clipdistances_array;
		bool redeclare_culldistances_array;
		bool dynamic_index_writes;
		bool use_passthrough_gs;
		bool use_passthrough_ts;
		bool use_core_functionality;
		bool fetch_culldistances;
	} test_items[] = { /* Use the basic outline to test the basic functionality of cull distances. */
					   {
						   true,	/* redeclare_clipdistances_array */
						   true,	/* redeclare_culldistances_array */
						   false,   /* dynamic_index_writes          */
						   false,   /* use_passthrough_gs            */
						   false,   /* use_passthrough_ts            */
						   is_core, /* use_core_functionality        */
						   false	/* fetch_culldistances           */
					   },
					   /* Use the basic outline but don't redeclare gl_ClipDistance with a size. */
					   {
						   false,   /* redeclare_clipdistances_array */
						   true,	/* redeclare_culldistances_array */
						   false,   /* dynamic_index_writes          */
						   false,   /* use_passthrough_gs            */
						   false,   /* use_passthrough_ts            */
						   is_core, /* use_core_functionality        */
						   false	/* fetch_culldistances           */
					   },
					   /* Use the basic outline but don't redeclare gl_CullDistance with a size. */
					   {
						   true,	/* redeclare_clipdistances_array  */
						   false,   /* redeclare_culldistances_array  */
						   false,   /* dynamic_index_writes           */
						   false,   /* use_passthrough_gs             */
						   false,   /* use_passthrough_ts             */
						   is_core, /* use_core_functionality         */
						   false	/* fetch_culldistances            */
					   },
					   /* Use the basic outline but don't redeclare either gl_ClipDistance or
		 * gl_CullDistance with a size.
		 */
					   {
						   false,   /* redeclare_clipdistances_array */
						   false,   /* redeclare_culldistances_array */
						   false,   /* dynamic_index_writes          */
						   false,   /* use_passthrough_gs            */
						   false,   /* use_passthrough_ts            */
						   is_core, /* use_core_functionality        */
						   false	/* fetch_culldistances           */
					   },
					   /* Use the basic outline but use dynamic indexing when writing the elements
		 * of the gl_ClipDistance and gl_CullDistance arrays.
		 */
					   {
						   true,	/* redeclare_clipdistances_array */
						   true,	/* redeclare_culldistances_array */
						   true,	/* dynamic_index_writes          */
						   false,   /* use_passthrough_gs            */
						   false,   /* use_passthrough_ts            */
						   is_core, /* use_core_functionality        */
						   false	/* fetch_culldistances           */
					   },
					   /* Use the basic outline but add a geometry shader to the program that
		 * simply passes through all written clip and cull distances.
		 */
					   {
						   true,	/* redeclare_clipdistances_array */
						   true,	/* redeclare_culldistances_array */
						   false,   /* dynamic_index_writes          */
						   true,	/* use_passthrough_gs            */
						   false,   /* use_passthrough_ts            */
						   is_core, /* use_core_functionality        */
						   false	/* fetch_culldistances           */
					   },
					   /* Use the basic outline but add a tessellation control and tessellation
		 * evaluation shader to the program which simply pass through all written
		 * clip and cull distances.
		 */
					   {
						   true,	/* redeclare_clipdistances_array */
						   true,	/* redeclare_culldistances_array */
						   false,   /* dynamic_index_writes          */
						   false,   /* use_passthrough_gs            */
						   true,	/* use_passthrough_ts            */
						   is_core, /* use_core_functionality        */
						   false	/* fetch_culldistances           */
					   },
					   /* Test that using #extension with GL_ARB_cull_distance allows using the
		 * feature even with an earlier version of GLSL. Also test that the
		 * extension name is available as preprocessor #define.
		 */
					   {
						   true,  /* redeclare_clipdistances_array */
						   true,  /* redeclare_culldistances_array */
						   false, /* dynamic_index_writes          */
						   false, /* use_passthrough_gs            */
						   false, /* use_passthrough_ts            */
						   false, /* use_core_functionality        */
						   false  /* fetch_culldistances           */
					   },
					   /* Use a program that has only a vertex shader and a fragment shader.
		 * The vertex shader should redeclare gl_ClipDistance with a size that
		 * fits all enabled cull distances. Also redeclare gl_CullDistance with a
		 * size. The sum of the two sizes should not be more than MAX_COMBINED_-
		 * CLIP_AND_CULL_DISTANCES. The fragment shader should output the cull
		 * distances written by the vertex shader by reading them from the built-in
		 * array gl_CullDistance.
		 */
					   {
						   true,  /* redeclare_clipdistances_array */
						   true,  /* redeclare_culldistances_array */
						   false, /* dynamic_index_writes          */
						   false, /* use_passthrough_gs            */
						   false, /* use_passthrough_ts            */
						   false, /* use_core_functionality        */
						   true   /* fetch_culldistances           */
					   }
	};
	const glw::GLuint n_test_items = sizeof(test_items) / sizeof(test_items[0]);

	gl.viewport(0, 0, m_to_width, m_to_height);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glViewport() call failed.");

	gl.clearColor(0.0f, 0.0f, 0.0f, 1.0f);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glClearColor() call failed.");

	for (glw::GLuint n_test_item = 0; n_test_item < n_test_items; ++n_test_item)
	{
		/* Check for OpenGL feature support */
		if (test_items[n_test_item].use_passthrough_ts)
		{
			if (!glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::core(4, 0)) &&
				!m_context.getContextInfo().isExtensionSupported("GL_ARB_tessellation_shader"))
			{
				continue; // no tessellation shader support
			}
		}

		const _test_item&	 current_test_item						= test_items[n_test_item];
		const _primitive_mode primitive_modes[PRIMITIVE_MODE_COUNT] = { PRIMITIVE_MODE_LINES, PRIMITIVE_MODE_POINTS,
																		PRIMITIVE_MODE_TRIANGLES };

		for (glw::GLuint primitive_mode_index = 0; primitive_mode_index < PRIMITIVE_MODE_COUNT; ++primitive_mode_index)
		{
			_primitive_mode primitive_mode = primitive_modes[primitive_mode_index];

			/* Iterate over a set of gl_ClipDistances[] and gl_CullDistances[] array sizes */
			for (glw::GLint n_iteration = 0; n_iteration <= gl_max_combined_clip_and_cull_distances_value;
				 ++n_iteration)
			{
				glw::GLuint clipdistances_array_size = 0;
				glw::GLuint culldistances_array_size = 0;

				if (n_iteration != 0 && n_iteration <= gl_max_clip_distances_value)
				{
					clipdistances_array_size = n_iteration;
				}

				if ((gl_max_combined_clip_and_cull_distances_value - n_iteration) < gl_max_cull_distances_value)
				{
					culldistances_array_size = gl_max_combined_clip_and_cull_distances_value - n_iteration;
				}
				else
				{
					culldistances_array_size = gl_max_cull_distances_value;
				}

				if (clipdistances_array_size == 0 && culldistances_array_size == 0)
				{
					/* Skip the dummy iteration */
					continue;
				}

				if (current_test_item.fetch_culldistances && (primitive_mode != PRIMITIVE_MODE_POINTS))
				{
					continue;
				}

				/* Create a program to run */
				buildPO(clipdistances_array_size, culldistances_array_size, current_test_item.dynamic_index_writes,
						primitive_mode, current_test_item.redeclare_clipdistances_array,
						current_test_item.redeclare_culldistances_array, current_test_item.use_core_functionality,
						current_test_item.use_passthrough_gs, current_test_item.use_passthrough_ts,
						current_test_item.fetch_culldistances);

				/* Initialize VAO data */
				configureVAO(clipdistances_array_size, culldistances_array_size, primitive_mode);

				/* Run GLSL program and check results */
				executeRenderTest(clipdistances_array_size, culldistances_array_size, primitive_mode,
								  current_test_item.use_passthrough_ts, current_test_item.fetch_culldistances);

			} /* for (all iterations) */
		}	 /* for (all test modes) */
	}		  /* for (all test items) */

	/* All done */
	if (has_succeeded)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
	}

	return STOP;
}

/** Returns pixel red component read from texture at position x, y.
 *
 *  @param x x-coordinate to read pixel color component from
 *  @param y y-coordinate to read pixel color component from
 **/
glw::GLint CullDistance::FunctionalTest::readRedPixelValue(glw::GLint x, glw::GLint y)
{
	glw::GLint result = -1;

	DE_ASSERT(x >= 0 && (glw::GLuint)x < m_to_width);
	DE_ASSERT(y >= 0 && (glw::GLuint)y < m_to_height);

	result = m_to_pixel_data_cache[(m_to_width * y + x) * m_to_pixel_data_cache_color_components];

	return result;
}

/** Reads texture into m_to_pixel_data_cache.
 *  Texture size determined by fields m_to_width, m_to_height
 **/
void CullDistance::FunctionalTest::readTexturePixels()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	m_to_pixel_data_cache.clear();

	m_to_pixel_data_cache.resize(m_to_width * m_to_height * m_to_pixel_data_cache_color_components);

	/* Read vertex from texture */
	gl.readPixels(0,		   /* x      */
				  0,		   /* y      */
				  m_to_width,  /* width  */
				  m_to_height, /* height */
				  GL_RGBA, GL_UNSIGNED_SHORT, &m_to_pixel_data_cache[0]);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glReadPixels() call failed.");
}

/** Constructor.
 *
 *  @param context Rendering context handle.
 **/
CullDistance::NegativeTest::NegativeTest(deqp::Context& context)
	: TestCase(context, "negative", "Cull Distance Negative Test")
	, m_fs_id(0)
	, m_po_id(0)
	, m_temp_buffer(DE_NULL)
	, m_vs_id(0)
{
	/* Left blank on purpose */
}

/** @brief Cull Distance Negative Test deinitialization */
void CullDistance::NegativeTest::deinit()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

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

	if (m_vs_id != 0)
	{
		gl.deleteShader(m_vs_id);

		m_vs_id = 0;
	}

	if (m_temp_buffer != DE_NULL)
	{
		delete[] m_temp_buffer;

		m_temp_buffer = DE_NULL;
	}
}

/** @brief Get string description of test with given parameters
 *
 *  @param [in] n_test_iteration                    Test iteration number
 *  @param [in] should_redeclare_output_variables   Indicate whether test redeclared gl_ClipDistance and gl_CullDistance
 *  @param [in] use_dynamic_index_based_writes      Indicate whether test used dynamic index-based setters
 *
 *  @return String containing description.
 */
std::string CullDistance::NegativeTest::getTestDescription(int n_test_iteration, bool should_redeclare_output_variables,
														   bool use_dynamic_index_based_writes)
{
	std::stringstream stream;

	stream << "Test iteration [" << n_test_iteration << "] which uses a vertex shader that:\n\n"
		   << ((should_redeclare_output_variables) ?
				   "* redeclares gl_ClipDistance and gl_CullDistance arrays\n" :
				   "* does not redeclare gl_ClipDistance and gl_CullDistance arrays\n")
		   << ((use_dynamic_index_based_writes) ? "* uses dynamic index-based writes\n" : "* uses static writes\n");

	return stream.str();
}

/** Executes test iteration.
 *
 *  @return Returns STOP when test has finished executing, CONTINUE if more iterations are needed.
 */
tcu::TestNode::IterateResult CullDistance::NegativeTest::iterate()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Build the test shaders. */
	const glw::GLchar* token_dynamic_index_based_writes = "DYNAMIC_INDEX_BASED_WRITES";
	const glw::GLchar* token_insert_static_writes		= "INSERT_STATIC_WRITES";
	const glw::GLchar* token_n_gl_clipdistance_entries  = "N_GL_CLIPDISTANCE_ENTRIES";
	const glw::GLchar* token_n_gl_culldistance_entries  = "N_GL_CULLDISTANCE_ENTRIES";
	const glw::GLchar* token_redeclare_output_variables = "REDECLARE_OUTPUT_VARIABLES";

	const glw::GLchar* fs_body = "#version 130\n"
								 "\n"
								 "void main()\n"
								 "{\n"
								 "}\n";

	const glw::GLchar* vs_body_preamble = "#version 130\n"
										  "\n"
										  "    #extension GL_ARB_cull_distance : require\n"
										  "\n";

	const glw::GLchar* vs_body_main = "#ifdef REDECLARE_OUTPUT_VARIABLES\n"
									  "    out float gl_ClipDistance[N_GL_CLIPDISTANCE_ENTRIES];\n"
									  "    out float gl_CullDistance[N_GL_CULLDISTANCE_ENTRIES];\n"
									  "#endif\n"
									  "\n"
									  "void main()\n"
									  "{\n"
									  "#ifdef DYNAMIC_INDEX_BASED_WRITES\n"
									  "    for (int n_clipdistance_entry = 0;\n"
									  "             n_clipdistance_entry < N_GL_CLIPDISTANCE_ENTRIES;\n"
									  "           ++n_clipdistance_entry)\n"
									  "    {\n"
									  "        gl_ClipDistance[n_clipdistance_entry] = float(n_clipdistance_entry) / "
									  "float(N_GL_CLIPDISTANCE_ENTRIES);\n"
									  "    }\n"
									  "\n"
									  "    for (int n_culldistance_entry = 0;\n"
									  "             n_culldistance_entry < N_GL_CULLDISTANCE_ENTRIES;\n"
									  "           ++n_culldistance_entry)\n"
									  "    {\n"
									  "        gl_CullDistance[n_culldistance_entry] = float(n_culldistance_entry) / "
									  "float(N_GL_CULLDISTANCE_ENTRIES);\n"
									  "    }\n"
									  "#else\n"
									  "    INSERT_STATIC_WRITES\n"
									  "#endif\n"
									  "}\n";

	/* This test should only be executed if ARB_cull_distance is supported, or if
	 * we're running a GL4.5 context
	 */
	if (!m_context.getContextInfo().isExtensionSupported("GL_ARB_cull_distance") &&
		!glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::core(4, 5)))
	{
		throw tcu::NotSupportedError("GL_ARB_cull_distance is not supported");
	}

	/* It only makes sense to run this test if GL_MAX_COMBINED_CLIP_AND_CULL_DISTANCES
	 * is lower than a sum of GL_MAX_CLIP_DISTANCES and GL_MAX_CLIP_CULL_DISTANCES.
	 */
	glw::GLint  gl_max_clip_distances_value					  = 0;
	glw::GLint  gl_max_combined_clip_and_cull_distances_value = 0;
	glw::GLint  gl_max_cull_distances_value					  = 0;
	glw::GLuint n_gl_clipdistance_array_items				  = 0;
	std::string n_gl_clipdistance_array_items_string;
	glw::GLuint n_gl_culldistance_array_items = 0;
	std::string n_gl_culldistance_array_items_string;
	std::string static_write_shader_body_part;

	gl.getIntegerv(GL_MAX_CLIP_DISTANCES, &gl_max_clip_distances_value);
	gl.getIntegerv(GL_MAX_COMBINED_CLIP_AND_CULL_DISTANCES, &gl_max_combined_clip_and_cull_distances_value);
	gl.getIntegerv(GL_MAX_CULL_DISTANCES, &gl_max_cull_distances_value);

	if (gl_max_clip_distances_value + gl_max_cull_distances_value < gl_max_combined_clip_and_cull_distances_value)
	{
		m_testCtx.getLog() << tcu::TestLog::Message
						   << "GL_MAX_COMBINED_CLIP_AND_CULL_DISTANCES is larger than or equal to "
							  "the sum of GL_MAX_CLIP_DISTANCES and GL_MAX_CULL_DISTANCES. Skipping."
						   << tcu::TestLog::EndMessage;

		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

		return STOP;
	}

	n_gl_clipdistance_array_items = gl_max_clip_distances_value;
	n_gl_culldistance_array_items = gl_max_combined_clip_and_cull_distances_value - gl_max_clip_distances_value + 1;

	/* Determine the number of items we will want the gl_ClipDistance and gl_CullDistance arrays
	 * to hold for test iterations that will re-declare the built-in output variables.
	 */
	{
		std::stringstream temp_sstream;

		temp_sstream << n_gl_clipdistance_array_items;

		n_gl_clipdistance_array_items_string = temp_sstream.str();
	}

	{
		std::stringstream temp_sstream;

		temp_sstream << n_gl_culldistance_array_items;

		n_gl_culldistance_array_items_string = temp_sstream.str();
	}

	/* Form the "static write" shader body part. */
	{
		std::stringstream temp_sstream;

		temp_sstream << "gl_ClipDistance[" << n_gl_clipdistance_array_items_string.c_str() << "] = 0.0f;\n"
					 << "gl_CullDistance[" << n_gl_culldistance_array_items_string.c_str() << "] = 0.0f;\n";

		static_write_shader_body_part = temp_sstream.str();
	}

	/* Prepare GL objects before we continue */
	glw::GLint compile_status = GL_FALSE;

	m_fs_id = gl.createShader(GL_FRAGMENT_SHADER);
	m_po_id = gl.createProgram();
	m_vs_id = gl.createShader(GL_VERTEX_SHADER);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateProgram() / glCreateShader() calls failed.");

	gl.attachShader(m_po_id, m_fs_id);
	gl.attachShader(m_po_id, m_vs_id);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glAttachShader() call(s) failed.");

	gl.shaderSource(m_fs_id, 1,			/* count */
					&fs_body, DE_NULL); /* length */
	GLU_EXPECT_NO_ERROR(gl.getError(), "glShaderSource() call failed.");

	gl.compileShader(m_fs_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glCompileShader() call failed.");

	gl.getShaderiv(m_fs_id, GL_COMPILE_STATUS, &compile_status);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetShaderiv() call failed.");

	if (compile_status == GL_FALSE)
	{
		TCU_FAIL("Fragment shader failed to compile.");
	}

	/* Run three separate test iterations. */
	struct _test_item
	{
		bool should_redeclare_output_variables;
		bool use_dynamic_index_based_writes;
	} test_items[] = { /* Negative Test 1 */
					   { true, false },

					   /* Negative Test 2 */
					   { false, false },

					   /* Negative Test 3 */
					   { false, true }
	};
	const unsigned int n_test_items = sizeof(test_items) / sizeof(test_items[0]);

	for (unsigned int n_test_item = 0; n_test_item < n_test_items; ++n_test_item)
	{
		const _test_item& current_test_item = test_items[n_test_item];

		/* Prepare vertex shader body */
		std::size_t		  token_position = std::string::npos;
		std::stringstream vs_body_sstream;
		std::string		  vs_body_string;

		vs_body_sstream << vs_body_preamble << "\n";

		if (current_test_item.should_redeclare_output_variables)
		{
			vs_body_sstream << "#define " << token_redeclare_output_variables << "\n";
		}

		if (current_test_item.use_dynamic_index_based_writes)
		{
			vs_body_sstream << "#define " << token_dynamic_index_based_writes << "\n";
		}

		vs_body_sstream << vs_body_main;

		/* Replace tokens with meaningful values */
		vs_body_string = vs_body_sstream.str();

		while ((token_position = vs_body_string.find(token_n_gl_clipdistance_entries)) != std::string::npos)
		{
			vs_body_string = vs_body_string.replace(token_position, strlen(token_n_gl_clipdistance_entries),
													n_gl_clipdistance_array_items_string);
		}

		while ((token_position = vs_body_string.find(token_n_gl_culldistance_entries)) != std::string::npos)
		{
			vs_body_string = vs_body_string.replace(token_position, strlen(token_n_gl_clipdistance_entries),
													n_gl_culldistance_array_items_string);
		}

		while ((token_position = vs_body_string.find(token_insert_static_writes)) != std::string::npos)
		{
			vs_body_string = vs_body_string.replace(token_position, strlen(token_insert_static_writes),
													static_write_shader_body_part);
		}

		/* Try to compile the vertex shader */
		glw::GLint  compile_status_internal = GL_FALSE;
		const char* vs_body_raw_ptr			= vs_body_string.c_str();

		gl.shaderSource(m_vs_id, 1,					/* count */
						&vs_body_raw_ptr, DE_NULL); /* length */
		GLU_EXPECT_NO_ERROR(gl.getError(), "glShaderSource() call failed.");

		gl.compileShader(m_vs_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glCompileShader() call failed.");

		gl.getShaderiv(m_vs_id, GL_COMPILE_STATUS, &compile_status_internal);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGetShaderiv() call failed.");

		if (compile_status_internal == GL_FALSE)
		{
			glw::GLint buffer_size = 0;

			/* Log the compilation error */
			m_testCtx.getLog() << tcu::TestLog::Message
							   << getTestDescription(n_test_item, current_test_item.should_redeclare_output_variables,
													 current_test_item.use_dynamic_index_based_writes)
							   << "has failed (as expected) to compile with the following info log:\n\n"
							   << tcu::TestLog::EndMessage;

			gl.getShaderiv(m_vs_id, GL_INFO_LOG_LENGTH, &buffer_size);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glGetShaderiv() call failed.");

			m_temp_buffer = new glw::GLchar[buffer_size + 1];

			memset(m_temp_buffer, 0, buffer_size + 1);

			gl.getShaderInfoLog(m_vs_id, buffer_size, DE_NULL, /* length */
								m_temp_buffer);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glGetShaderInfoLog() call failed.");

			m_testCtx.getLog() << tcu::TestLog::Message << m_temp_buffer << tcu::TestLog::EndMessage;

			delete[] m_temp_buffer;
			m_temp_buffer = DE_NULL;

			/* Move on to the next iteration */
			continue;
		}

		/* Try to link the program object */
		glw::GLint link_status = GL_FALSE;

		gl.linkProgram(m_po_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glLinkProgram() call failed.");

		gl.getProgramiv(m_po_id, GL_LINK_STATUS, &link_status);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGetProgramiv() call failed.");

		if (link_status == GL_TRUE)
		{
			m_testCtx.getLog() << tcu::TestLog::Message
							   << getTestDescription(n_test_item, current_test_item.should_redeclare_output_variables,
													 current_test_item.use_dynamic_index_based_writes)
							   << "has linked successfully which is invalid!" << tcu::TestLog::EndMessage;

			TCU_FAIL("Program object has linked successfully, even though the process should have failed.");
		}
		else
		{
			glw::GLint buffer_size = 0;

			m_testCtx.getLog() << tcu::TestLog::Message
							   << getTestDescription(n_test_item, current_test_item.should_redeclare_output_variables,
													 current_test_item.use_dynamic_index_based_writes)
							   << "has failed (as expected) to link with the following info log:\n\n"
							   << tcu::TestLog::EndMessage;

			gl.getProgramiv(m_po_id, GL_INFO_LOG_LENGTH, &buffer_size);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glGetProgramiv() call failed.");

			m_temp_buffer = new glw::GLchar[buffer_size + 1];

			memset(m_temp_buffer, 0, buffer_size + 1);

			gl.getProgramInfoLog(m_po_id, buffer_size, DE_NULL, /* length */
								 m_temp_buffer);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glGetProgramInfoLog() call failed.");

			m_testCtx.getLog() << tcu::TestLog::Message << m_temp_buffer << tcu::TestLog::EndMessage;

			delete[] m_temp_buffer;
			m_temp_buffer = DE_NULL;
		}
	} /* for (all test items) */

	/* All done */
	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

	return STOP;
}

/** Constructor.
 *
 *  @param context Rendering context.
 */
CullDistance::Tests::Tests(deqp::Context& context) : TestCaseGroup(context, "cull_distance", "Cull Distance Test Suite")
{
}

/** Initializes the test group contents. */
void CullDistance::Tests::init()
{
	addChild(new CullDistance::APICoverageTest(m_context));
	addChild(new CullDistance::FunctionalTest(m_context));
	addChild(new CullDistance::NegativeTest(m_context));
}
} /* glcts namespace */
