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

/* Includes. */
#include "gl4cES31CompatibilityTests.hpp"
#include "gluContextInfo.hpp"
#include "gluDefs.hpp"
#include "gluRenderContext.hpp"
#include "gluStrUtil.hpp"
#include "tcuTestLog.hpp"

/******************************** Test Group Implementation       ********************************/

/** @brief ES3.1 Compatibility tests group constructor.
 *
 *  @param [in] context     OpenGL context.
 */
gl4cts::es31compatibility::Tests::Tests(deqp::Context& context)
	: TestCaseGroup(context, "es_31_compatibility", "ES3.1 Compatibility Test Suite")
{
	/* Intentionally left blank */
}

/** @brief ES3.1 Compatibility Tests initializer. */
void gl4cts::es31compatibility::Tests::init()
{
	/* New tests. */
	addChild(new gl4cts::es31compatibility::ShaderCompilationCompatibilityTests(m_context));
	addChild(new gl4cts::es31compatibility::ShaderFunctionalCompatibilityTest(m_context));

	/* Ported tests. */
	addChild(new gl4cts::es31compatibility::SampleVariablesTests(m_context, glu::GLSL_VERSION_310_ES));
	addChild(new gl4cts::es31compatibility::ShaderImageLoadStoreTests(m_context));
	addChild(new gl4cts::es31compatibility::ShaderStorageBufferObjectTests(m_context));
}

/******************************** Shader Compilation Compatibility Tests Implementation   ********************************/

/** @brief ShaderCompilationCompatibilityTests constructor.
 *
 *  @param [in] context     OpenGL context.
 */
gl4cts::es31compatibility::ShaderCompilationCompatibilityTests::ShaderCompilationCompatibilityTests(
	deqp::Context& context)
	: deqp::TestCase(context, "shader_compilation", "Shader Compilation Compatibility Test")
{
}

/** @brief ShaderCompilationCompatibilityTests test cases iterations.
 */
tcu::TestNode::IterateResult gl4cts::es31compatibility::ShaderCompilationCompatibilityTests::iterate()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* OpenGL support query. */
	bool is_at_least_gl_45 = (glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::core(4, 5)));
	bool is_arb_es31_compatibility = m_context.getContextInfo().isExtensionSupported("GL_ARB_ES3_1_compatibility");

	/* Running tests. */
	bool is_ok	= true;
	bool is_error = false;

	glw::GLuint shader = 0;

	/* Test */
	try
	{
		if (is_at_least_gl_45 || is_arb_es31_compatibility)
		{
			for (glw::GLsizei i = 0; i < s_shaders_count; ++i)
			{
				/* Shader compilation. */
				shader = gl.createShader(s_shaders[i].type);

				GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateShader call failed.");

				if (0 == shader)
				{
					throw 0;
				}

				gl.shaderSource(shader, 1, &(s_shaders[i].source), NULL);

				GLU_EXPECT_NO_ERROR(gl.getError(), "glShaderSource call failed.");

				gl.compileShader(shader);

				GLU_EXPECT_NO_ERROR(gl.getError(), "glCompileShader call failed.");

				/* Checking for errors. */
				glw::GLint status = GL_FALSE;

				gl.getShaderiv(shader, GL_COMPILE_STATUS, &status);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glGetShaderiv call failed.");

				if (GL_FALSE == status)
				{
					/* Setup result. */
					is_ok = false;

					/* Getting compilation informations. */
					glw::GLint log_size = 0;

					gl.getShaderiv(shader, GL_INFO_LOG_LENGTH, &log_size);

					GLU_EXPECT_NO_ERROR(gl.getError(), "glGetShaderiv() call failed.");

					if (log_size)
					{
						glw::GLchar* log = new glw::GLchar[log_size];

						if (log)
						{
							memset(log, 0, log_size);

							gl.getShaderInfoLog(shader, log_size, DE_NULL, log);

							/* Logging. */
							m_context.getTestContext().getLog() << tcu::TestLog::Message << "Compilation of "
																<< s_shaders[i].type_name
																<< " shader have failed.\n Shader source was:\n"
																<< s_shaders[i].source << "\nCompillation log:\n"
																<< log << tcu::TestLog::EndMessage;

							delete[] log;

							GLU_EXPECT_NO_ERROR(gl.getError(), "glGetShaderInfoLog() call failed.");
						}
					}
				}

				/* Cleanup. */
				gl.deleteShader(shader);

				shader = 0;

				GLU_EXPECT_NO_ERROR(gl.getError(), "glDeleteShader call failed.");
			}
		}
	}
	catch (...)
	{
		is_ok	= false;
		is_error = true;
	}

	/* Cleanup. */
	if (0 != shader)
	{
		gl.deleteShader(shader);

		shader = 0;
	}

	/* Result's setup and logging. */
	if (is_ok)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		if (is_error)
		{
			m_context.getTestContext().getLog() << tcu::TestLog::Message
												<< "Internal error has occured during the Shader Version Test."
												<< tcu::TestLog::EndMessage;

			m_testCtx.setTestResult(QP_TEST_RESULT_INTERNAL_ERROR, "Test error.");
		}
		else
		{
			m_context.getTestContext().getLog() << tcu::TestLog::Message << "The Shader Version Test has failed."
												<< tcu::TestLog::EndMessage;

			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
		}
	}

	return STOP;
}

const gl4cts::es31compatibility::ShaderCompilationCompatibilityTests::TestShader
	gl4cts::es31compatibility::ShaderCompilationCompatibilityTests::s_shaders[] = {
		{ /* Shader for testing ES 3.1 version string support.*/
		  GL_VERTEX_SHADER, "vertex", "#version 310 es\n"
									  "\n"
									  "void main()\n"
									  "{\n"
									  "    gl_Position = vec4(1.0);\n"
									  "}\n" },
		{ /* Shader for testing ES 3.1 version string support.*/
		  GL_FRAGMENT_SHADER, "fragment", "#version 310 es\n"
										  "\n"
										  "out highp vec4 color;"
										  "\n"
										  "void main()\n"
										  "{\n"
										  "    color = vec4(1.0);\n"
										  "}\n" },
		{ /* Shader for testing that gl_HelperInvocation variable is supported.*/
		  GL_FRAGMENT_SHADER, "fragment", "#version 310 es\n"
										  "\n"
										  "out highp vec4 color;"
										  "\n"
										  "void main()\n"
										  "{\n"
										  "    if(gl_HelperInvocation)\n"
										  "    {\n"
										  "        color = vec4(1.0);\n"
										  "    }\n"
										  "    else\n"
										  "    {\n"
										  "        color = vec4(0.0);\n"
										  "    }\n"
										  "}\n" },
		{ /* Shader for testing ES 3.1 version string support.*/
		  GL_COMPUTE_SHADER, "compute",
		  "#version 310 es\n"
		  "\n"
		  "layout(local_size_x = 128) in;\n"
		  "layout(std140, binding = 0) buffer Output\n"
		  "{\n"
		  "    uint elements[];\n"
		  "} output_data;\n"
		  "\n"
		  "void main()\n"
		  "{\n"
		  "    output_data.elements[gl_GlobalInvocationID.x] = gl_GlobalInvocationID.x * gl_GlobalInvocationID.x;\n"
		  "}\n" }
	};

const glw::GLsizei gl4cts::es31compatibility::ShaderCompilationCompatibilityTests::s_shaders_count =
	sizeof(s_shaders) / sizeof(s_shaders[0]);

/******************************** Shader Functional Compatibility Test Implementation   ********************************/

/** @brief Shader Functional Compatibility Test constructor.
 *
 *  @param [in] context     OpenGL context.
 */
gl4cts::es31compatibility::ShaderFunctionalCompatibilityTest::ShaderFunctionalCompatibilityTest(deqp::Context& context)
	: deqp::TestCase(context, "shader_functional", "Shader Functional Compatibility Test")
	, m_po_id(0)
	, m_fbo_id(0)
	, m_rbo_id(0)
	, m_vao_id(0)
{
}

/** @brief ShaderCompilationCompatibilityTests test cases iterations.
 */
tcu::TestNode::IterateResult gl4cts::es31compatibility::ShaderFunctionalCompatibilityTest::iterate()
{
	/* OpenGL support query. */
	bool is_at_least_gl_45 = (glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::core(4, 5)));
	bool is_arb_es31_compatibility = m_context.getContextInfo().isExtensionSupported("GL_ARB_ES3_1_compatibility");

	/* Running tests. */
	bool is_ok	= true;
	bool is_error = false;

	/* Test */
	try
	{
		if (is_at_least_gl_45 || is_arb_es31_compatibility)
		{
			createFramebufferAndVertexArrayObject();

			for (glw::GLsizei i = 0; i < s_shaders_count; ++i)
			{
				if (!createProgram(s_shaders[i]))
				{
					is_ok = false;

					continue; /* if createProgram failed we shall omit this iteration */
				}

				is_ok &= test();

				cleanProgram();
			}

			cleanFramebufferAndVertexArrayObject();
		}
	}
	catch (...)
	{
		/* Result setup. */
		is_ok	= false;
		is_error = true;

		/* Cleanup. */
		cleanProgram();
		cleanFramebufferAndVertexArrayObject();
	}

	/* Result's setup. */
	if (is_ok)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		if (is_error)
		{
			m_context.getTestContext().getLog() << tcu::TestLog::Message
												<< "Internal error has occured during the Shader Version Test."
												<< tcu::TestLog::EndMessage;

			m_testCtx.setTestResult(QP_TEST_RESULT_INTERNAL_ERROR, "Test error.");
		}
		else
		{
			m_context.getTestContext().getLog() << tcu::TestLog::Message << "The Shader Version Test has failed."
												<< tcu::TestLog::EndMessage;

			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
		}
	}

	return STOP;
}

/** @brief Create program object
 *
 *  @note Program object is going to be stored into m_po_id.
 *        If building succeeded program will be set current (glUseProgram).
 *
 *  @param [in] shader_source   Shader source to be builded.
 *
 *  @return True if succeeded, false otherwise.
 */
bool gl4cts::es31compatibility::ShaderFunctionalCompatibilityTest::createProgram(const struct Shader shader_source)
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	struct _Shader
	{
		const glw::GLchar* const* source;
		const glw::GLenum		  type;
		glw::GLuint				  id;
	} shader[] = { { shader_source.vertex, GL_VERTEX_SHADER, 0 }, { shader_source.fragment, GL_FRAGMENT_SHADER, 0 } };

	glw::GLuint const shader_count = sizeof(shader) / sizeof(shader[0]);

	try
	{
		/* Make sure m_po_id is cleaned. */
		if (m_po_id)
		{
			cleanProgram();
		}

		/* Create program. */
		m_po_id = gl.createProgram();
		GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateProgram call failed.");

		/* Shader compilation. */

		for (glw::GLuint i = 0; i < shader_count; ++i)
		{
			if (DE_NULL != shader[i].source)
			{
				shader[i].id = gl.createShader(shader[i].type);

				GLU_EXPECT_NO_ERROR(gl.getError(), "glCreateShader call failed.");

				gl.attachShader(m_po_id, shader[i].id);

				GLU_EXPECT_NO_ERROR(gl.getError(), "glAttachShader call failed.");

				gl.shaderSource(shader[i].id, 3, shader[i].source, NULL);

				GLU_EXPECT_NO_ERROR(gl.getError(), "glShaderSource call failed.");

				gl.compileShader(shader[i].id);

				GLU_EXPECT_NO_ERROR(gl.getError(), "glCompileShader call failed.");

				glw::GLint status = GL_FALSE;

				gl.getShaderiv(shader[i].id, GL_COMPILE_STATUS, &status);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glGetShaderiv call failed.");

				if (GL_FALSE == status)
				{
					glw::GLint log_size = 0;

					gl.getShaderiv(shader[i].id, GL_INFO_LOG_LENGTH, &log_size);

					GLU_EXPECT_NO_ERROR(gl.getError(), "glGetShaderiv() call failed.");

					if (log_size)
					{
						glw::GLchar* log = new glw::GLchar[log_size];

						if (log)
						{
							memset(log, 0, log_size);

							gl.getShaderInfoLog(shader[i].id, log_size, DE_NULL, log);

							m_context.getTestContext().getLog()
								<< tcu::TestLog::Message << "Compilation of shader has failed.\nShader source:\n"
								<< shader[i].source[0] << shader[i].source[1] << shader[i].source[2]
								<< "\nCompillation log:\n"
								<< log << tcu::TestLog::EndMessage;

							delete[] log;

							GLU_EXPECT_NO_ERROR(gl.getError(), "glGetShaderInfoLog() call failed.");
						}
					}

					throw 0;
				}
			}
		}

		/* Link. */
		gl.linkProgram(m_po_id);

		GLU_EXPECT_NO_ERROR(gl.getError(), "glLinkProgram call failed.");

		glw::GLint status = GL_FALSE;

		gl.getProgramiv(m_po_id, GL_LINK_STATUS, &status);

		if (GL_TRUE == status)
		{
			for (glw::GLuint i = 0; i < shader_count; ++i)
			{
				if (shader[i].id)
				{
					gl.detachShader(m_po_id, shader[i].id);

					GLU_EXPECT_NO_ERROR(gl.getError(), "glDetachShader call failed.");
				}
			}
		}
		else
		{
			glw::GLint log_size = 0;

			gl.getProgramiv(m_po_id, GL_INFO_LOG_LENGTH, &log_size);

			GLU_EXPECT_NO_ERROR(gl.getError(), "glGetProgramiv() call failed.");

			if (log_size)
			{
				glw::GLchar* log = new glw::GLchar[log_size];

				if (log)
				{
					memset(log, 0, log_size);

					gl.getProgramInfoLog(m_po_id, log_size, DE_NULL, log);

					m_context.getTestContext().getLog() << tcu::TestLog::Message
														<< "Linkage of shader program has failed.\nLinkage log:\n"
														<< log << tcu::TestLog::EndMessage;

					delete[] log;

					GLU_EXPECT_NO_ERROR(gl.getError(), "glGetShaderInfoLog() call failed.");
				}
			}

			throw 0;
		}
	}
	catch (...)
	{
		if (m_po_id)
		{
			gl.deleteProgram(m_po_id);

			m_po_id = 0;
		}
	}

	for (glw::GLuint i = 0; i < shader_count; ++i)
	{
		if (0 != shader[i].id)
		{
			gl.deleteShader(shader[i].id);

			shader[i].id = 0;
		}
	}

	if (m_po_id)
	{
		gl.useProgram(m_po_id);

		GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram call failed.");

		return true;
	}

	return false;
}

/** @brief Create framebuffer and vertex array object.
 *
 *  @note Frembuffer will be stored in m_fbo_id and m_rbo_id.
 *        Vertex array object will be stored in m_vao_id.
 *        Function will throw 0 if erro has occured.
 */
void gl4cts::es31compatibility::ShaderFunctionalCompatibilityTest::createFramebufferAndVertexArrayObject()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Prepare framebuffer. */
	gl.clearColor(0.0f, 0.0f, 0.0f, 0.0f);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glClearColor call failed.");

	gl.genFramebuffers(1, &m_fbo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenFramebuffers call failed.");

	gl.genRenderbuffers(1, &m_rbo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenRenderbuffers call failed.");

	gl.bindFramebuffer(GL_FRAMEBUFFER, m_fbo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffer call failed.");

	gl.bindRenderbuffer(GL_RENDERBUFFER, m_rbo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindRenderbuffer call failed.");

	gl.renderbufferStorage(GL_RENDERBUFFER, GL_R8, 1 /* x size */, 1 /* y size */);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glRenderbufferStorage call failed.");

	gl.framebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, m_rbo_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glFramebufferRenderbuffer call failed.");

	/* Check if all went ok. */
	if (gl.checkFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		throw 0;
	}

	/* View Setup. */
	gl.viewport(0, 0, 1 /* x size */, 1 /* y size */);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glViewport call failed.");

	/* Create and bind empty vertex array object. */
	gl.genVertexArrays(1, &m_vao_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenVertexArrays() call failed.");

	if (0 == m_vao_id)
	{
		throw 0;
	}

	gl.bindVertexArray(m_vao_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindVertexArray() call failed.");
}

/** @brief Run test case.
 *
 *  @note Test case run in following order:
 *         *  clear screen with 0 value (color setup in createFramebufferAndVertexArrayObject);
 *         *  draw full screen quad;
 *         *  fetch pixel from screen using glReadPixel (view is 1x1 pixel in size);
 *         *  compare results (1.0f is expected as the result of the shader).
 */
bool gl4cts::es31compatibility::ShaderFunctionalCompatibilityTest::test()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Make sure objects are cleaned. */
	if (m_fbo_id || m_rbo_id || m_vao_id)
	{
		cleanFramebufferAndVertexArrayObject();
	}

	/* Drawing quad which shall output result. */
	gl.clear(GL_COLOR_BUFFER_BIT);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glClear() call failed.");

	gl.drawArrays(GL_TRIANGLE_STRIP, 0, 4);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawArrays() call failed.");

	/* Fetching result. */
	glw::GLfloat red = -1.f;

	gl.readPixels(0, 0, 1, 1, GL_RED, GL_FLOAT, &red);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glReadPixels() call failed.");

	if (de::abs(1.f - red) <= 0.125 /* Precision. */)
	{
		return true;
	}

	return false;
}

/** @brief Release program object.
 */
void gl4cts::es31compatibility::ShaderFunctionalCompatibilityTest::cleanProgram()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Deleting program. */
	if (m_po_id)
	{
		gl.useProgram(0);

		gl.deleteProgram(m_po_id);

		m_po_id = 0;
	}
}

/** @brief Release framebuffer, renderbuffer and vertex array objects.
 */
void gl4cts::es31compatibility::ShaderFunctionalCompatibilityTest::cleanFramebufferAndVertexArrayObject()
{
	/* Shortcut for GL functionality. */
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	/* Deleting view. */
	if (m_fbo_id)
	{
		gl.deleteFramebuffers(1, &m_fbo_id);

		m_fbo_id = 0;
	}

	if (m_rbo_id)
	{
		gl.deleteRenderbuffers(1, &m_rbo_id);

		m_rbo_id = 0;
	}

	if (m_vao_id)
	{
		gl.deleteVertexArrays(1, &m_vao_id);

		m_vao_id = 0;
	}
}

const glw::GLchar* gl4cts::es31compatibility::ShaderFunctionalCompatibilityTest::s_shader_version = "#version 310 es\n";

const glw::GLchar* gl4cts::es31compatibility::ShaderFunctionalCompatibilityTest::s_vertex_shader_body =
	"\n"
	"out highp float dummy;\n"
	"\n"
	"void main()\n"
	"{\n"
	"    switch(gl_VertexID % 4)\n"
	"    {\n"
	"    case 0:\n"
	"       gl_Position = vec4(-1.0, -1.0,  0.0,  1.0);\n"
	"       break;\n"
	"    case 1:\n"
	"       gl_Position = vec4( 1.0, -1.0,  0.0,  1.0);\n"
	"       break;\n"
	"    case 2:\n"
	"       gl_Position = vec4(-1.0,  1.0,  0.0,  1.0);\n"
	"       break;\n"
	"    case 3:\n"
	"       gl_Position = vec4( 1.0,  1.0,  0.0,  1.0);\n"
	"       break;\n"
	"    }\n"
	"\n"
	"    dummy = float(gl_VertexID % 4);\n   /* Always less than 4. */"
	"}\n";

const glw::GLchar* gl4cts::es31compatibility::ShaderFunctionalCompatibilityTest::s_fragment_shader_body =
	"\n"
	"in highp float dummy;\n"
	"\n"
	"out highp vec4 result;\n"
	"\n"
	"void main()\n"
	"{\n"
	"    TTYPE a = LEFT;\n"
	"    TTYPE b = RIGHT;\n"
	"    BTYPE c = BDATA && BTYPE(dummy < 4.0);\n    /* Making sure that expression is not compile time constant. */"
	"\n"
	"    TTYPE mixed = mix(a, b, c);\n"
	"\n"
	"    if(REFERENCE == mixed)\n"
	"    {\n"
	"        result = vec4(1.0);\n"
	"    }\n"
	"    else\n"
	"    {\n"
	"        result = vec4(0.0);\n"
	"    }\n"
	"}\n";

const struct gl4cts::es31compatibility::ShaderFunctionalCompatibilityTest::Shader
	gl4cts::es31compatibility::ShaderFunctionalCompatibilityTest::s_shaders[] = {
		{ { s_shader_version, "", s_vertex_shader_body },
		  { s_shader_version, "#define TTYPE        highp int\n"
							  "#define BTYPE        bool\n"
							  "#define LEFT        -1\n"
							  "#define RIGHT       -2\n"
							  "#define BDATA        true\n"
							  "#define REFERENCE   -2\n",
			s_fragment_shader_body } },
		{ { s_shader_version, "", s_vertex_shader_body },
		  { s_shader_version, "#define TTYPE        highp uint\n"
							  "#define BTYPE        bool\n"
							  "#define LEFT         1u\n"
							  "#define RIGHT        2u\n"
							  "#define BDATA        true\n"
							  "#define REFERENCE    2u\n",
			s_fragment_shader_body } },
		{ { s_shader_version, "", s_vertex_shader_body },
		  { s_shader_version, "#define TTYPE        mediump int\n"
							  "#define BTYPE        bool\n"
							  "#define LEFT        -1\n"
							  "#define RIGHT       -2\n"
							  "#define BDATA        true\n"
							  "#define REFERENCE   -2\n",
			s_fragment_shader_body } },
		{ { s_shader_version, "", s_vertex_shader_body },
		  { s_shader_version, "#define TTYPE        mediump uint\n"
							  "#define BTYPE        bool\n"
							  "#define LEFT         1u\n"
							  "#define RIGHT        2u\n"
							  "#define BDATA        true\n"
							  "#define REFERENCE    2u\n",
			s_fragment_shader_body } },
		{ { s_shader_version, "", s_vertex_shader_body },
		  { s_shader_version, "#define TTYPE        lowp int\n"
							  "#define BTYPE        bool\n"
							  "#define LEFT        -1\n"
							  "#define RIGHT       -2\n"
							  "#define BDATA        true\n"
							  "#define REFERENCE   -2\n",
			s_fragment_shader_body } },
		{ { s_shader_version, "", s_vertex_shader_body },
		  { s_shader_version, "#define TTYPE        lowp uint\n"
							  "#define BTYPE        bool\n"
							  "#define LEFT         1u\n"
							  "#define RIGHT        2u\n"
							  "#define BDATA        true\n"
							  "#define REFERENCE    2u\n",
			s_fragment_shader_body } },
		{ { s_shader_version, "", s_vertex_shader_body },
		  { s_shader_version, "#define TTYPE        bool\n"
							  "#define BTYPE        bool\n"
							  "#define LEFT         false\n"
							  "#define RIGHT        true\n"
							  "#define BDATA        true\n"
							  "#define REFERENCE    true\n",
			s_fragment_shader_body } },
		{ { s_shader_version, "", s_vertex_shader_body },
		  { s_shader_version, "#define TTYPE        highp ivec2\n"
							  "#define BTYPE        bvec2\n"
							  "#define LEFT         ivec2(-1, -2)\n"
							  "#define RIGHT        ivec2(-3, -4)\n"
							  "#define BDATA        bvec2(true, false)\n"
							  "#define REFERENCE    ivec2(-3, -2)\n",
			s_fragment_shader_body } },
		{ { s_shader_version, "", s_vertex_shader_body },
		  { s_shader_version, "#define TTYPE        highp uvec2\n"
							  "#define BTYPE        bvec2\n"
							  "#define LEFT         uvec2(1, 2)\n"
							  "#define RIGHT        uvec2(3, 4)\n"
							  "#define BDATA        bvec2(true, false)\n"
							  "#define REFERENCE    uvec2(3, 2)\n",
			s_fragment_shader_body } },
		{ { s_shader_version, "", s_vertex_shader_body },
		  { s_shader_version, "#define TTYPE        mediump ivec2\n"
							  "#define BTYPE        bvec2\n"
							  "#define LEFT         ivec2(-1, -2)\n"
							  "#define RIGHT        ivec2(-3, -4)\n"
							  "#define BDATA        bvec2(true, false)\n"
							  "#define REFERENCE    ivec2(-3, -2)\n",
			s_fragment_shader_body } },
		{ { s_shader_version, "", s_vertex_shader_body },
		  { s_shader_version, "#define TTYPE        mediump uvec2\n"
							  "#define BTYPE        bvec2\n"
							  "#define LEFT         uvec2(1, 2)\n"
							  "#define RIGHT        uvec2(3, 4)\n"
							  "#define BDATA        bvec2(true, false)\n"
							  "#define REFERENCE    uvec2(3, 2)\n",
			s_fragment_shader_body } },
		{ { s_shader_version, "", s_vertex_shader_body },
		  { s_shader_version, "#define TTYPE        lowp ivec2\n"
							  "#define BTYPE        bvec2\n"
							  "#define LEFT         ivec2(-1, -2)\n"
							  "#define RIGHT        ivec2(-3, -4)\n"
							  "#define BDATA        bvec2(true, false)\n"
							  "#define REFERENCE    ivec2(-3, -2)\n",
			s_fragment_shader_body } },
		{ { s_shader_version, "", s_vertex_shader_body },
		  { s_shader_version, "#define TTYPE        lowp uvec2\n"
							  "#define BTYPE        bvec2\n"
							  "#define LEFT         uvec2(1, 2)\n"
							  "#define RIGHT        uvec2(3, 4)\n"
							  "#define BDATA        bvec2(true, false)\n"
							  "#define REFERENCE    uvec2(3, 2)\n",
			s_fragment_shader_body } },
		{ { s_shader_version, "", s_vertex_shader_body },
		  { s_shader_version, "#define TTYPE        bvec2\n"
							  "#define BTYPE        bvec2\n"
							  "#define LEFT         bvec2(true,  true)\n"
							  "#define RIGHT        bvec2(false, false)\n"
							  "#define BDATA        bvec2(true,  false)\n"
							  "#define REFERENCE    bvec2(false, true)\n",
			s_fragment_shader_body } },
		{ { s_shader_version, "", s_vertex_shader_body },
		  { s_shader_version, "#define TTYPE        highp ivec3\n"
							  "#define BTYPE        bvec3\n"
							  "#define LEFT         ivec3(-1, -2, -3)\n"
							  "#define RIGHT        ivec3(-4, -5, -6)\n"
							  "#define BDATA        bvec3(true, false, true)\n"
							  "#define REFERENCE    ivec3(-4, -2, -6)\n",
			s_fragment_shader_body } },
		{ { s_shader_version, "", s_vertex_shader_body },
		  { s_shader_version, "#define TTYPE        highp uvec3\n"
							  "#define BTYPE        bvec3\n"
							  "#define LEFT         uvec3(1, 2, 3)\n"
							  "#define RIGHT        uvec3(4, 5, 6)\n"
							  "#define BDATA        bvec3(true, false, true)\n"
							  "#define REFERENCE    uvec3(4, 2, 6)\n",
			s_fragment_shader_body } },
		{ { s_shader_version, "", s_vertex_shader_body },
		  { s_shader_version, "#define TTYPE        mediump ivec3\n"
							  "#define BTYPE        bvec3\n"
							  "#define LEFT         ivec3(-1, -2, -3)\n"
							  "#define RIGHT        ivec3(-4, -5, -6)\n"
							  "#define BDATA        bvec3(true, false, true)\n"
							  "#define REFERENCE    ivec3(-4, -2, -6)\n",
			s_fragment_shader_body } },
		{ { s_shader_version, "", s_vertex_shader_body },
		  { s_shader_version, "#define TTYPE        mediump uvec3\n"
							  "#define BTYPE        bvec3\n"
							  "#define LEFT         uvec3(1, 2, 3)\n"
							  "#define RIGHT        uvec3(4, 5, 6)\n"
							  "#define BDATA        bvec3(true, false, true)\n"
							  "#define REFERENCE    uvec3(4, 2, 6)\n",
			s_fragment_shader_body } },
		{ { s_shader_version, "", s_vertex_shader_body },
		  { s_shader_version, "#define TTYPE        lowp ivec3\n"
							  "#define BTYPE        bvec3\n"
							  "#define LEFT         ivec3(-1, -2, -3)\n"
							  "#define RIGHT        ivec3(-4, -5, -6)\n"
							  "#define BDATA        bvec3(true, false, true)\n"
							  "#define REFERENCE    ivec3(-4, -2, -6)\n",
			s_fragment_shader_body } },
		{ { s_shader_version, "", s_vertex_shader_body },
		  { s_shader_version, "#define TTYPE        lowp uvec3\n"
							  "#define BTYPE        bvec3\n"
							  "#define LEFT         uvec3(1, 2, 3)\n"
							  "#define RIGHT        uvec3(4, 5, 6)\n"
							  "#define BDATA        bvec3(true, false, true)\n"
							  "#define REFERENCE    uvec3(4, 2, 6)\n",
			s_fragment_shader_body } },
		{ { s_shader_version, "", s_vertex_shader_body },
		  { s_shader_version, "#define TTYPE        bvec3\n"
							  "#define BTYPE        bvec3\n"
							  "#define LEFT         bvec3(true,  true, true)\n"
							  "#define RIGHT        bvec3(false, false, false)\n"
							  "#define BDATA        bvec3(true,  false, true)\n"
							  "#define REFERENCE    bvec3(false, true, false)\n",
			s_fragment_shader_body } },
		{ { s_shader_version, "", s_vertex_shader_body },
		  { s_shader_version, "#define TTYPE        highp ivec4\n"
							  "#define BTYPE        bvec4\n"
							  "#define LEFT         ivec4(-1, -2, -3, -4)\n"
							  "#define RIGHT        ivec4(-5, -6, -7, -8)\n"
							  "#define BDATA        bvec4(true, false, true, false)\n"
							  "#define REFERENCE    ivec4(-5, -2, -7, -4)\n",
			s_fragment_shader_body } },
		{ { s_shader_version, "", s_vertex_shader_body },
		  { s_shader_version, "#define TTYPE        highp uvec4\n"
							  "#define BTYPE        bvec4\n"
							  "#define LEFT         uvec4(1, 2, 3, 4)\n"
							  "#define RIGHT        uvec4(5, 6, 7, 8)\n"
							  "#define BDATA        bvec4(true, false, true, false)\n"
							  "#define REFERENCE    uvec4(5, 2, 7, 4)\n",
			s_fragment_shader_body } },
		{ { s_shader_version, "", s_vertex_shader_body },
		  { s_shader_version, "#define TTYPE        mediump ivec4\n"
							  "#define BTYPE        bvec4\n"
							  "#define LEFT         ivec4(-1, -2, -3, -4)\n"
							  "#define RIGHT        ivec4(-5, -6, -7, -8)\n"
							  "#define BDATA        bvec4(true, false, true, false)\n"
							  "#define REFERENCE    ivec4(-5, -2, -7, -4)\n",
			s_fragment_shader_body } },
		{ { s_shader_version, "", s_vertex_shader_body },
		  { s_shader_version, "#define TTYPE        mediump uvec4\n"
							  "#define BTYPE        bvec4\n"
							  "#define LEFT         uvec4(1, 2, 3, 4)\n"
							  "#define RIGHT        uvec4(5, 6, 7, 8)\n"
							  "#define BDATA        bvec4(true, false, true, false)\n"
							  "#define REFERENCE    uvec4(5, 2, 7, 4)\n",
			s_fragment_shader_body } },
		{ { s_shader_version, "", s_vertex_shader_body },
		  { s_shader_version, "#define TTYPE        lowp ivec4\n"
							  "#define BTYPE        bvec4\n"
							  "#define LEFT         ivec4(-1, -2, -3, -4)\n"
							  "#define RIGHT        ivec4(-5, -6, -7, -8)\n"
							  "#define BDATA        bvec4(true, false, true, false)\n"
							  "#define REFERENCE    ivec4(-5, -2, -7, -4)\n",
			s_fragment_shader_body } },
		{ { s_shader_version, "", s_vertex_shader_body },
		  { s_shader_version, "#define TTYPE        lowp uvec4\n"
							  "#define BTYPE        bvec4\n"
							  "#define LEFT         uvec4(1, 2, 3, 4)\n"
							  "#define RIGHT        uvec4(5, 6, 7, 8)\n"
							  "#define BDATA        bvec4(true, false, true, false)\n"
							  "#define REFERENCE    uvec4(5, 2, 7, 4)\n",
			s_fragment_shader_body } },
		{ { s_shader_version, "", s_vertex_shader_body },
		  { s_shader_version, "#define TTYPE        bvec4\n"
							  "#define BTYPE        bvec4\n"
							  "#define LEFT         bvec4(true,  true,  true,  true)\n"
							  "#define RIGHT        bvec4(false, false, false, false)\n"
							  "#define BDATA        bvec4(true,  false, true,  false)\n"
							  "#define REFERENCE    bvec4(false, true,  false, true)\n",
			s_fragment_shader_body } }
	};

const glw::GLsizei gl4cts::es31compatibility::ShaderFunctionalCompatibilityTest::s_shaders_count =
	sizeof(s_shaders) / sizeof(s_shaders[0]);
