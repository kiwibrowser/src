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
 * \file  gl3cGLSLnoperspectiveTests.cpp
 * \brief Implements conformance tests for "GLSL no perspective" functionality.
 */ /*-------------------------------------------------------------------*/

#include "gl3cGLSLnoperspectiveTests.hpp"
#include "gluContextInfo.hpp"
#include "glwFunctions.hpp"
#include "tcuTestLog.hpp"

namespace gl3cts
{
/** Compiles shader
 *
 * @param context     Context of test framework
 * @param shader_id   Shader id
 * @param shader_code Shader source code
 **/
void compile_shader(deqp::Context& context, glw::GLuint shader_id, const glw::GLchar* shader_code)
{
	/* GL entry points */
	const glw::Functions& gl = context.getRenderContext().getFunctions();

	/* Compilation status */
	glw::GLint status = GL_FALSE;

	/* Set source code */
	gl.shaderSource(shader_id, 1 /* count */, &shader_code, 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "ShaderSource");

	/* Compile */
	gl.compileShader(shader_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "CompileShader");

	/* Get compilation status */
	gl.getShaderiv(shader_id, GL_COMPILE_STATUS, &status);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GetShaderiv");

	/* Log compilation error */
	if (GL_TRUE != status)
	{
		glw::GLint				 length = 0;
		std::vector<glw::GLchar> message;

		/* Error log length */
		gl.getShaderiv(shader_id, GL_INFO_LOG_LENGTH, &length);
		GLU_EXPECT_NO_ERROR(gl.getError(), "GetShaderiv");

		/* Prepare storage */
		message.resize(length);

		/* Get error log */
		gl.getShaderInfoLog(shader_id, length, 0, &message[0]);
		GLU_EXPECT_NO_ERROR(gl.getError(), "GetShaderInfoLog");

		/* Log */
		context.getTestContext().getLog() << tcu::TestLog::Message << "Failed to compile shader:\n"
										  << &message[0] << tcu::TestLog::EndMessage
										  << tcu::TestLog::KernelSource(shader_code);

		TCU_FAIL("Failed to compile shader");
	}
}

/** Creates and compiles shader
 *
 * @param context Context of test framework
 * @param stage   Shader stage
 * @param code    Shader source code
 *
 * @return Id of created shader
 **/
glw::GLuint prepare_shader(deqp::Context& context, glw::GLenum stage, const glw::GLchar* code)
{
	/* GL entry points */
	const glw::Functions& gl = context.getRenderContext().getFunctions();

	/* Compilation status */
	glw::GLuint id = 0;

	/* Create */
	id = gl.createShader(stage);
	GLU_EXPECT_NO_ERROR(gl.getError(), "CreateShader");

	try
	{
		compile_shader(context, id, code);
	}
	catch (const std::exception& exc)
	{
		gl.deleteShader(id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "DeleteShader");

		TCU_FAIL(exc.what());
	}

	return id;
}

/** Attach shaders and link program
 *
 * @param context            Context of test framework
 * @param fragment_shader_id Id of fragment shader
 * @param vertex_shader_id   Id of vertex shader
 * @param program_object_id  Id of program object
 **/
void link_program(deqp::Context& context, glw::GLuint fragment_shader_id, glw::GLuint vertex_shader_id,
				  glw::GLuint program_object_id)
{
	/* GL entry points */
	const glw::Functions& gl = context.getRenderContext().getFunctions();

	/* Link status */
	glw::GLint status = GL_FALSE;

	/* Attach shaders */
	if (0 != fragment_shader_id)
	{
		gl.attachShader(program_object_id, fragment_shader_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "AttachShader");
	}

	if (0 != vertex_shader_id)
	{
		gl.attachShader(program_object_id, vertex_shader_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "AttachShader");
	}

	/* Link */
	gl.linkProgram(program_object_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "LinkProgram");

	/* Get link status */
	gl.getProgramiv(program_object_id, GL_LINK_STATUS, &status);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GetProgramiv");

	/* Log link error */
	if (GL_TRUE != status)
	{
		glw::GLint				 length = 0;
		std::vector<glw::GLchar> message;

		/* Get error log length */
		gl.getProgramiv(program_object_id, GL_INFO_LOG_LENGTH, &length);
		GLU_EXPECT_NO_ERROR(gl.getError(), "GetProgramiv");

		message.resize(length);

		/* Get error log */
		gl.getProgramInfoLog(program_object_id, length, 0, &message[0]);
		GLU_EXPECT_NO_ERROR(gl.getError(), "GetProgramInfoLog");

		/* Log */
		context.getTestContext().getLog() << tcu::TestLog::Message << "Failed to link program:\n"
										  << &message[0] << tcu::TestLog::EndMessage;

		TCU_FAIL("Failed to link program");
	}
}

/** Create and build program
 *
 * @param context              Context of test framework
 * @param fragment_shader_code Fragment shader source code
 * @param vertex_shader_code   Vertex shader source code
 *
 * @return Id of program object
 **/
glw::GLuint prepare_program(deqp::Context& context, const glw::GLchar* fragment_shader_code,
							const glw::GLchar* vertex_shader_code)
{
	/* GL entry points */
	const glw::Functions& gl = context.getRenderContext().getFunctions();

	glw::GLuint fragment_shader_id = 0;
	glw::GLuint program_object_id  = 0;
	glw::GLuint vertex_shader_id   = 0;

	try
	{
		/* Create shader objects and compile */
		if (0 != fragment_shader_code)
		{
			fragment_shader_id = prepare_shader(context, GL_FRAGMENT_SHADER, fragment_shader_code);
		}

		if (0 != vertex_shader_code)
		{
			vertex_shader_id = prepare_shader(context, GL_VERTEX_SHADER, vertex_shader_code);
		}

		/* Create program object */
		program_object_id = gl.createProgram();
		GLU_EXPECT_NO_ERROR(gl.getError(), "CreateProgram");

		/* Link program */
		link_program(context, fragment_shader_id, vertex_shader_id, program_object_id);
	}
	catch (const std::exception& exc)
	{
		if (0 != program_object_id)
			gl.deleteProgram(program_object_id);
		if (0 != fragment_shader_id)
			gl.deleteShader(fragment_shader_id);
		if (0 != vertex_shader_id)
			gl.deleteShader(vertex_shader_id);

		TCU_FAIL(exc.what());
	}

	/* Shader ids can now be deleted */
	if (0 != fragment_shader_id)
		gl.deleteShader(fragment_shader_id);
	if (0 != vertex_shader_id)
		gl.deleteShader(vertex_shader_id);

	/* Done */
	return program_object_id;
}

/** Constructor.
 *
 *  @param context Rendering context.
 *
 **/
FunctionalTest::FunctionalTest(deqp::Context& context)
	: TestCase(context, "functionaltest", "Verifies that interpolation qualifier has imact on results of rendering")
{
	/* Left blank intentionally */
}

/** Executes test iteration.
 *
 *  @return Returns STOP when test has finished executing, CONTINUE if more iterations are needed.
 */
tcu::TestNode::IterateResult FunctionalTest::iterate()
{
	static const char* vs_default = "#version 130\n"
									"\n"
									"in vec4 in_position;\n"
									"in vec4 in_color;\n"
									"\n"
									"out vec4 vs_fs_color;\n"
									"\n"
									"void main()\n"
									"{\n"
									"    gl_Position = in_position;\n"
									"    vs_fs_color = in_color;\n"
									"}\n"
									"\n";

	static const char* vs_flat = "#version 130\n"
								 "\n"
								 "in vec4 in_position;\n"
								 "in vec4 in_color;\n"
								 "\n"
								 "flat out vec4 vs_fs_color;\n"
								 "\n"
								 "void main()\n"
								 "{\n"
								 "    gl_Position = in_position;\n"
								 "    vs_fs_color = in_color;\n"
								 "}\n"
								 "\n";

	static const char* vs_noperspective = "#version 130\n"
										  "\n"
										  "in vec4 in_position;\n"
										  "in vec4 in_color;\n"
										  "\n"
										  "noperspective out vec4 vs_fs_color;\n"
										  "\n"
										  "void main()\n"
										  "{\n"
										  "    gl_Position = in_position;\n"
										  "    vs_fs_color = in_color;\n"
										  "}\n"
										  "\n";

	static const char* vs_smooth = "#version 130\n"
								   "\n"
								   "in vec4 in_position;\n"
								   "in vec4 in_color;\n"
								   "\n"
								   "smooth out vec4 vs_fs_color;\n"
								   "\n"
								   "void main()\n"
								   "{\n"
								   "    gl_Position = in_position;\n"
								   "    vs_fs_color = in_color;\n"
								   "}\n"
								   "\n";

	static const char* fs_default = "#version 130\n"
									"\n"
									"in vec4 vs_fs_color;\n"
									"\n"
									"out vec4 out_color;\n"
									"\n"
									"void main()\n"
									"{\n"
									"    out_color = vs_fs_color;\n"
									"}\n"
									"\n";

	static const char* fs_flat = "#version 130\n"
								 "\n"
								 "flat in vec4 vs_fs_color;\n"
								 "\n"
								 "out vec4 out_color;\n"
								 "\n"
								 "void main()\n"
								 "{\n"
								 "    out_color = vs_fs_color;\n"
								 "}\n"
								 "\n";

	static const char* fs_noperspective = "#version 130\n"
										  "\n"
										  "noperspective in vec4 vs_fs_color;\n"
										  "\n"
										  "out vec4 out_color;\n"
										  "\n"
										  "void main()\n"
										  "{\n"
										  "    out_color = vs_fs_color;\n"
										  "}\n"
										  "\n";

	static const char* fs_smooth = "#version 130\n"
								   "\n"
								   "smooth in vec4 vs_fs_color;\n"
								   "\n"
								   "out vec4 out_color;\n"
								   "\n"
								   "void main()\n"
								   "{\n"
								   "    out_color = vs_fs_color;\n"
								   "}\n"
								   "\n";

	static const glw::GLfloat positions_data[] = { -1.0f, 1.0f,  -1.0f, 1.0f, 3.0f, 3.0f,  3.0f, 3.0f,
												   -1.0f, -1.0f, -1.0f, 1.0f, 3.0f, -3.0f, 3.0f, 3.0f };
	static const glw::GLubyte colors_data[] = { 0xff, 0x00, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff,
												0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };

	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	static const char* shaders[][2] = { { vs_noperspective, fs_noperspective },
										{ vs_default, fs_default },
										{ vs_flat, fs_flat },
										{ vs_smooth, fs_smooth } };
	static const size_t		  n_shaders			= sizeof(shaders) / sizeof(shaders[0]);
	static const size_t		  noperspective_idx = 0;
	static const glw::GLsizei w					= 64;
	static const glw::GLsizei h					= 64;
	static const glw::GLsizei image_length		= w * h;

	bool test_result = true;

	glw::GLuint po_ids[n_shaders]  = { 0 };
	glw::GLuint tex_ids[n_shaders] = { 0 };
	glw::GLuint fbo_ids[n_shaders] = { 0 };
	glw::GLuint vab_id			   = 0;
	glw::GLuint vao_id			   = 0;

	try
	{
		/* Buffer */
		gl.genBuffers(1, &vab_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "GenBuffers");

		gl.bindBuffer(GL_ARRAY_BUFFER, vab_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "BindBuffer");

		gl.bufferData(GL_ARRAY_BUFFER, sizeof(colors_data) + sizeof(positions_data), 0 /* data */, GL_STATIC_DRAW);
		GLU_EXPECT_NO_ERROR(gl.getError(), "BufferData");

		gl.bufferSubData(GL_ARRAY_BUFFER, 0 /* offset */, sizeof(positions_data) /* size */, positions_data);
		GLU_EXPECT_NO_ERROR(gl.getError(), "BufferSubData");

		gl.bufferSubData(GL_ARRAY_BUFFER, sizeof(positions_data) /* offset */, sizeof(colors_data) /* size */,
						 colors_data);
		GLU_EXPECT_NO_ERROR(gl.getError(), "BufferSubData");

		/* FBOs */
		gl.genFramebuffers(n_shaders, fbo_ids);
		GLU_EXPECT_NO_ERROR(gl.getError(), "GenFramebuffers");

		/* Textures */
		gl.genTextures(n_shaders, tex_ids);
		GLU_EXPECT_NO_ERROR(gl.getError(), "GenTextures");

		/* VAO */
		gl.genVertexArrays(1, &vao_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "GenVertexArrays");

		gl.bindVertexArray(vao_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "BindVertexArrays");

		for (size_t i = 0; i < n_shaders; ++i)
		{
			/* Program */
			po_ids[i] = prepare_program(m_context, shaders[i][1], shaders[i][0]);

			/* Texture */
			gl.bindTexture(GL_TEXTURE_2D, tex_ids[i]);
			GLU_EXPECT_NO_ERROR(gl.getError(), "BindTexture");

			gl.texImage2D(GL_TEXTURE_2D, 0 /* level */, GL_RGBA8, w, h, 0 /* border */, GL_RGBA, GL_UNSIGNED_BYTE,
						  0 /* data */);
			GLU_EXPECT_NO_ERROR(gl.getError(), "TexImage2D");

			gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

			gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
			GLU_EXPECT_NO_ERROR(gl.getError(), "TexParameteri");

			/* FBO */
			gl.bindFramebuffer(GL_FRAMEBUFFER, fbo_ids[i]);
			GLU_EXPECT_NO_ERROR(gl.getError(), "BindFramebuffer");

			gl.framebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex_ids[i], 0 /* level */);
			GLU_EXPECT_NO_ERROR(gl.getError(), "FramebufferTexture2D");

			/* Viewport */
			gl.viewport(0, 0, w, h);
			GLU_EXPECT_NO_ERROR(gl.getError(), "Viewport");

			/* VAO */
			glw::GLint in_position_loc = gl.getAttribLocation(po_ids[i], "in_position");
			glw::GLint in_color_loc	= gl.getAttribLocation(po_ids[i], "in_color");
			GLU_EXPECT_NO_ERROR(gl.getError(), "GetAttribLocation");
			if ((-1 == in_position_loc) || (-1 == in_color_loc))
			{
				TCU_FAIL("Attributes are not available");
			}

			gl.vertexAttribPointer(in_position_loc, 4 /* size */, GL_FLOAT, GL_FALSE /* normalizeed */, 0 /* stride */,
								   0 /* offset */);
			GLU_EXPECT_NO_ERROR(gl.getError(), "VertexAttribPointer");

			gl.vertexAttribPointer(in_color_loc, 4 /* size */, GL_UNSIGNED_BYTE, GL_TRUE /* normalizeed */,
								   0 /* stride */, (glw::GLvoid*)sizeof(positions_data) /* offset */);
			GLU_EXPECT_NO_ERROR(gl.getError(), "VertexAttribPointer");

			gl.enableVertexAttribArray(in_position_loc);
			GLU_EXPECT_NO_ERROR(gl.getError(), "EnableVertexAttribArray");

			gl.enableVertexAttribArray(in_color_loc);
			GLU_EXPECT_NO_ERROR(gl.getError(), "EnableVertexAttribArray");

			/* Clear */
			gl.clearColor(0.0f, 0.0f, 0.0f, 1.0f);
			GLU_EXPECT_NO_ERROR(gl.getError(), "ClearColor");

			gl.clear(GL_COLOR_BUFFER_BIT);
			GLU_EXPECT_NO_ERROR(gl.getError(), "Clear");

			/* Activate program */
			gl.useProgram(po_ids[i]);
			GLU_EXPECT_NO_ERROR(gl.getError(), "UseProgram");

			/* Draw */
			gl.drawArrays(GL_TRIANGLE_STRIP, 0 /* first */, 4 /* count */);
			GLU_EXPECT_NO_ERROR(gl.getError(), "DrawArrays");

			/* Disable VAO */
			gl.disableVertexAttribArray(in_position_loc);
			GLU_EXPECT_NO_ERROR(gl.getError(), "DisableVertexAttribArray");

			gl.disableVertexAttribArray(in_color_loc);
			GLU_EXPECT_NO_ERROR(gl.getError(), "DisableVertexAttribArray");
		}

		/* Verify results */
		{
			/* Storage for images */
			std::vector<glw::GLuint> fbo_data;
			std::vector<glw::GLuint> noperspective_data;

			fbo_data.resize(image_length);
			noperspective_data.resize(image_length);

			/* Get noperspective image */
			gl.bindTexture(GL_TEXTURE_2D, tex_ids[noperspective_idx]);
			GLU_EXPECT_NO_ERROR(gl.getError(), "BindTexture");

			gl.getTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, &noperspective_data[0]);
			GLU_EXPECT_NO_ERROR(gl.getError(), "GetTexImage");

			/* Compare noperspective with the rest */
			for (size_t i = 0; i < n_shaders; ++i)
			{
				/* Skip noperspective */
				if (noperspective_idx == i)
				{
					continue;
				}

				gl.bindTexture(GL_TEXTURE_2D, tex_ids[i]);
				GLU_EXPECT_NO_ERROR(gl.getError(), "BindTexture");

				gl.getTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, &fbo_data[0]);
				GLU_EXPECT_NO_ERROR(gl.getError(), "GetTexImage");

				bool are_same = true;

				for (size_t pixel = 0; pixel < image_length; ++pixel)
				{
					const glw::GLuint left  = noperspective_data[pixel];
					const glw::GLuint right = fbo_data[pixel];

					if (left != right)
					{
						are_same = false;
						break;
					}
				}

				if (true == are_same)
				{
					test_result = false;
					break;
				}
			}
		}

		gl.bindBuffer(GL_ARRAY_BUFFER, 0);
		gl.bindFramebuffer(GL_FRAMEBUFFER, 0);
		gl.bindTexture(GL_TEXTURE_2D, 0);
		gl.bindVertexArray(0);
		gl.useProgram(0);

		gl.deleteBuffers(1, &vab_id);
		vab_id = 0;

		gl.deleteVertexArrays(1, &vao_id);
		vao_id = 0;

		gl.deleteFramebuffers(n_shaders, fbo_ids);
		gl.deleteTextures(n_shaders, tex_ids);

		for (size_t idx = 0; idx < n_shaders; ++idx)
		{
			fbo_ids[idx] = 0;
			tex_ids[idx] = 0;
		}

		for (size_t i = 0; i < n_shaders; ++i)
		{
			if (0 != po_ids[i])
			{
				gl.deleteProgram(po_ids[i]);
				po_ids[i] = 0;
			}
		}
	}
	catch (...)
	{
		/* Unbind */
		gl.bindBuffer(GL_ARRAY_BUFFER, 0);

		gl.bindFramebuffer(GL_FRAMEBUFFER, 0);

		gl.bindTexture(GL_TEXTURE_2D, 0);

		gl.bindVertexArray(0);

		gl.useProgram(0);

		/* Delete */
		if (0 != vao_id)
		{
			gl.deleteVertexArrays(1, &vao_id);
		}

		if (0 != vab_id)
		{
			gl.deleteBuffers(1, &vab_id);
		}

		if (0 != fbo_ids[0])
		{
			gl.deleteFramebuffers(n_shaders, fbo_ids);
		}

		for (size_t i = 0; i < n_shaders; ++i)
		{
			if (0 != po_ids[i])
			{
				gl.deleteProgram(po_ids[i]);
			}
		}

		if (0 != tex_ids[0])
		{
			gl.deleteTextures(n_shaders, tex_ids);
		}

		/* Clean any error */
		gl.getError();

		/* Rethrow */
		throw;
	}

	/* Set test result */
	if (true == test_result)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
	}

	/* Clean */
	{
		/* Unbind */
		gl.bindBuffer(GL_ARRAY_BUFFER, 0);

		gl.bindFramebuffer(GL_FRAMEBUFFER, 0);

		gl.bindTexture(GL_TEXTURE_2D, 0);

		gl.bindVertexArray(0);

		gl.useProgram(0);

		/* Delete */
		if (0 != vao_id)
		{
			gl.deleteVertexArrays(1, &vao_id);
		}

		if (0 != vab_id)
		{
			gl.deleteBuffers(1, &vab_id);
		}

		if (0 != fbo_ids[0])
		{
			gl.deleteFramebuffers(1, fbo_ids);
		}

		for (size_t i = 0; i < n_shaders; ++i)
		{
			if (0 != po_ids[i])
			{
				gl.deleteProgram(po_ids[i]);
			}
		}

		if (0 != tex_ids[0])
		{
			gl.deleteTextures(1, tex_ids);
		}
	}

	/* Clean any error */
	gl.getError();

	/* Done */
	return STOP;
}

/** Constructor.
 *
 *  @param context Rendering context.
 **/
GLSLnoperspectiveTests::GLSLnoperspectiveTests(deqp::Context& context)
	: TestCaseGroup(context, "glsl_noperspective", "Verifies \"GLSL_noperspective\" functionality")
{
	/* Left blank on purpose */
}

/** Initializes a gpu_shader_5 test group.
 *
 **/
void GLSLnoperspectiveTests::init(void)
{
	addChild(new FunctionalTest(m_context));
}
} /* gl3cts namespace */
