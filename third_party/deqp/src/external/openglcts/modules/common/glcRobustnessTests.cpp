/*-------------------------------------------------------------------------
 * OpenGL Conformance Test Suite
 * -----------------------------
 *
 * Copyright (c) 2017 The Khronos Group Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *	  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */ /*!
 * \file glcRobustnessTests.cpp
 * \brief Conformance tests for the Robustness feature functionality.
 */ /*-------------------------------------------------------------------*/

#include "glcRobustnessTests.hpp"
#include "deSharedPtr.hpp"
#include "glcRobustBufferAccessBehaviorTests.hpp"
#include "gluContextInfo.hpp"
#include "gluPlatform.hpp"
#include "gluRenderContext.hpp"
#include "glwEnums.hpp"
#include "glwFunctions.hpp"
#include "tcuCommandLine.hpp"
#include "tcuTestLog.hpp"
#include <cstring>

using namespace glw;
using namespace glcts::RobustBufferAccessBehavior;

namespace glcts
{

namespace ResetNotificationStrategy
{

class RobustnessBase : public tcu::TestCase
{
public:
	RobustnessBase(tcu::TestContext& testCtx, const char* name, const char* description, glu::ApiType apiType);

	glu::RenderContext* createRobustContext(glu::ResetNotificationStrategy reset);

private:
	glu::ApiType m_ApiType;
};

RobustnessBase::RobustnessBase(tcu::TestContext& testCtx, const char* name, const char* description,
							   glu::ApiType apiType)
	: tcu::TestCase(testCtx, name, description), m_ApiType(apiType)
{
}

glu::RenderContext* RobustnessBase::createRobustContext(glu::ResetNotificationStrategy reset)
{
	/* Create test context to verify if GL_KHR_robustness extension is available */
	{
		deqp::Context context(m_testCtx, glu::ContextType(m_ApiType));
		if (!context.getContextInfo().isExtensionSupported("GL_KHR_robustness") &&
			!contextSupports(context.getRenderContext().getType(), glu::ApiType::es(3, 2)))
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED,
									"GL_KHR_robustness extension not supported");
			return NULL;
		}
	}

	glu::RenderConfig		renderCfg(glu::ContextType(m_ApiType, glu::CONTEXT_ROBUST));
	const tcu::CommandLine& commandLine = m_testCtx.getCommandLine();
	glu::parseRenderConfig(&renderCfg, commandLine);

	if (commandLine.getSurfaceType() == tcu::SURFACETYPE_WINDOW)
		renderCfg.resetNotificationStrategy = reset;
	else
		throw tcu::NotSupportedError("Test not supported in non-windowed context");

	/* Try to create core/es robusness context */
	return createRenderContext(m_testCtx.getPlatform(), commandLine, renderCfg);
}

class NoResetNotificationCase : public RobustnessBase
{
	typedef glw::GLenum(GLW_APIENTRY* PFNGLGETGRAPHICSRESETSTATUS)();

public:
	NoResetNotificationCase(tcu::TestContext& testCtx, const char* name, const char* description, glu::ApiType apiType)
		: RobustnessBase(testCtx, name, description, apiType)
	{
	}

	virtual IterateResult iterate(void)
	{
		glu::ResetNotificationStrategy	strategy = glu::RESET_NOTIFICATION_STRATEGY_NO_RESET_NOTIFICATION;
		de::SharedPtr<glu::RenderContext> robustContext(createRobustContext(strategy));
		if (!robustContext.get())
			return STOP;

		PFNGLGETGRAPHICSRESETSTATUS pGetGraphicsResetStatus =
			(PFNGLGETGRAPHICSRESETSTATUS)robustContext->getProcAddress("glGetGraphicsResetStatus");

		if (DE_NULL == pGetGraphicsResetStatus)
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_INTERNAL_ERROR,
									"Pointer to function glGetGraphicsResetStatus is NULL.");
			return STOP;
		}

		glw::GLint reset = 0;

		const glw::Functions& gl = robustContext->getFunctions();
		gl.getIntegerv(GL_RESET_NOTIFICATION_STRATEGY, &reset);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv");

		if (reset != GL_NO_RESET_NOTIFICATION)
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "Test failed! glGet returned wrong value [" << reset
							   << ", expected " << GL_NO_RESET_NOTIFICATION << "]." << tcu::TestLog::EndMessage;

			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
			return STOP;
		}

		glw::GLint status = pGetGraphicsResetStatus();
		if (status != GL_NO_ERROR)
		{
			m_testCtx.getLog() << tcu::TestLog::Message
							   << "Test failed! glGetGraphicsResetStatus returned wrong value [" << status
							   << ", expected " << GL_NO_ERROR << "]." << tcu::TestLog::EndMessage;

			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
			return STOP;
		}

		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
		return STOP;
	}
};

class LoseContextOnResetCase : public RobustnessBase
{
public:
	LoseContextOnResetCase(tcu::TestContext& testCtx, const char* name, const char* description, glu::ApiType apiType)
		: RobustnessBase(testCtx, name, description, apiType)
	{
	}

	virtual IterateResult iterate(void)
	{
		glu::ResetNotificationStrategy	strategy = glu::RESET_NOTIFICATION_STRATEGY_LOSE_CONTEXT_ON_RESET;
		de::SharedPtr<glu::RenderContext> robustContext(createRobustContext(strategy));
		if (!robustContext.get())
			return STOP;

		glw::GLint reset = 0;

		const glw::Functions& gl = robustContext->getFunctions();
		gl.getIntegerv(GL_RESET_NOTIFICATION_STRATEGY, &reset);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv");

		if (reset != GL_LOSE_CONTEXT_ON_RESET)
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "Test failed! glGet returned wrong value [" << reset
							   << ", expected " << GL_LOSE_CONTEXT_ON_RESET << "]." << tcu::TestLog::EndMessage;

			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
			return STOP;
		}

		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
		return STOP;
	}
};

} // ResetNotificationStrategy namespace

namespace RobustBufferAccessBehavior
{

static deqp::Context* createContext(tcu::TestContext& testCtx, glu::ApiType apiType)
{
	deqp::Context* context = new deqp::Context(testCtx, glu::ContextType(apiType));
	if (!context)
	{
		testCtx.setTestResult(QP_TEST_RESULT_INTERNAL_ERROR, "Pointer to context is NULL.");
		return DE_NULL;
	}

	if (!(contextSupports(context->getRenderContext().getType(), glu::ApiType::es(3, 2)) ||
		  (context->getContextInfo().isExtensionSupported("GL_KHR_robustness") &&
		   context->getContextInfo().isExtensionSupported("GL_KHR_robust_buffer_access_behavior"))))
	{
		testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "Not Supported");
		delete context;
		return DE_NULL;
	}

	return context;
}

/** Implementation of test GetnUniformTest. Description follows:
 *
 * This test verifies if read uniform variables to the buffer with bufSize less than expected result with GL_INVALID_OPERATION error;
 **/
class GetnUniformTest : public tcu::TestCase
{
public:
	/* Public methods */
	GetnUniformTest(tcu::TestContext& testCtx, glu::ApiType apiType);
	virtual ~GetnUniformTest()
	{
	}

	/* Public methods inherited from TestCase */
	virtual tcu::TestNode::IterateResult iterate(void);

private:
	/* Private methods */
	std::string getComputeShader(bool glslES320);

	bool verifyResult(const void* inputData, const void* resultData, int size, const char* method);
	bool verifyError(glw::GLint error, glw::GLint expectedError, const char* method);

	glu::ApiType m_ApiType;
};

/** Constructor
 *
 * @param context Test context
 **/
GetnUniformTest::GetnUniformTest(tcu::TestContext& testCtx, glu::ApiType apiType)
	: tcu::TestCase(testCtx, "getnuniform", "Verifies if read uniform variables to the buffer with bufSize less than "
											"expected result with GL_INVALID_OPERATION")
	, m_ApiType(apiType)
{
	/* Nothing to be done here */
}

/** Execute test
 *
 * @return tcu::TestNode::STOP
 **/
tcu::TestNode::IterateResult GetnUniformTest::iterate()
{
	de::SharedPtr<deqp::Context> context(createContext(m_testCtx, m_ApiType));
	if (!context.get())
		return STOP;

	/* GL funtion pointers. */
	typedef void(GLW_APIENTRY * PFNGLGETNUNIFORMFV)(glw::GLuint program, glw::GLint location, glw::GLsizei bufSize,
													glw::GLfloat * params);
	typedef void(GLW_APIENTRY * PFNGLGETNUNIFORMIV)(glw::GLuint program, glw::GLint location, glw::GLsizei bufSize,
													glw::GLint * params);
	typedef void(GLW_APIENTRY * PFNGLGETNUNIFORMUIV)(glw::GLuint program, glw::GLint location, glw::GLsizei bufSize,
													 glw::GLuint * params);

	/* Function pointers need to be grabbed only for GL4.5 but it is done also for ES for consistency */
	glu::RenderContext& renderContext   = context->getRenderContext();
	PFNGLGETNUNIFORMFV  pGetnUniformfv  = (PFNGLGETNUNIFORMFV)renderContext.getProcAddress("glGetnUniformfv");
	PFNGLGETNUNIFORMIV  pGetnUniformiv  = (PFNGLGETNUNIFORMIV)renderContext.getProcAddress("glGetnUniformiv");
	PFNGLGETNUNIFORMUIV pGetnUniformuiv = (PFNGLGETNUNIFORMUIV)renderContext.getProcAddress("glGetnUniformuiv");

	if ((DE_NULL == pGetnUniformfv) || (DE_NULL == pGetnUniformiv) || (DE_NULL == pGetnUniformuiv))
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_INTERNAL_ERROR, "Pointer to function glGetnUniform* is NULL.");
		return STOP;
	}

	const Functions& gl = renderContext.getFunctions();

	const GLfloat input4f[]  = { 1.0f, 5.4f, 3.14159f, 1.28f };
	const GLint   input3i[]  = { 10, -20, -30 };
	const GLuint  input4ui[] = { 10, 20, 30, 40 };

	/* Test result indicator */
	bool test_result = true;

	/* Iterate over all cases */
	Program program(gl);

	/* Compute Shader */
	bool			   glslES320 = contextSupports(renderContext.getType(), glu::ApiType::es(3, 2));
	const std::string& cs		 = getComputeShader(glslES320);

	/* Shaders initialization */
	program.Init(cs /* cs */, "" /* fs */, "" /* gs */, "" /* tcs */, "" /* tes */, "" /* vs */);
	program.Use();

	/* For gl4.5 use shader storage buffer */
	GLuint buf;
	if (!glslES320)
	{
		gl.genBuffers(1, &buf);
		GLU_EXPECT_NO_ERROR(gl.getError(), "GenBuffers");

		gl.bindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, buf);
		GLU_EXPECT_NO_ERROR(gl.getError(), "BindBufferBase");

		gl.bufferData(GL_SHADER_STORAGE_BUFFER, 16, DE_NULL, GL_STREAM_DRAW);
		GLU_EXPECT_NO_ERROR(gl.getError(), "BufferData");
	}

	/* passing uniform values */
	gl.programUniform4fv(program.m_id, 11, 1, input4f);
	GLU_EXPECT_NO_ERROR(gl.getError(), "ProgramUniform4fv");

	gl.programUniform3iv(program.m_id, 12, 1, input3i);
	GLU_EXPECT_NO_ERROR(gl.getError(), "ProgramUniform3iv");

	gl.programUniform4uiv(program.m_id, 13, 1, input4ui);
	GLU_EXPECT_NO_ERROR(gl.getError(), "ProgramUniform4uiv");

	gl.dispatchCompute(1, 1, 1);
	GLU_EXPECT_NO_ERROR(gl.getError(), "DispatchCompute");

	/* veryfing gfetnUniform error messages */
	GLfloat result4f[4];
	GLint   result3i[3];
	GLuint  result4ui[4];

	pGetnUniformfv(program.m_id, 11, sizeof(GLfloat) * 4, result4f);
	test_result = test_result &&
				  verifyResult((void*)input4f, (void*)result4f, sizeof(GLfloat) * 4, "getnUniformfv [false negative]");
	test_result = test_result && verifyError(gl.getError(), GL_NO_ERROR, "getnUniformfv [false negative]");

	pGetnUniformfv(program.m_id, 11, sizeof(GLfloat) * 3, result4f);
	test_result = test_result && verifyError(gl.getError(), GL_INVALID_OPERATION, "getnUniformfv [false positive]");

	pGetnUniformiv(program.m_id, 12, sizeof(GLint) * 3, result3i);
	test_result = test_result &&
				  verifyResult((void*)input3i, (void*)result3i, sizeof(GLint) * 3, "getnUniformiv [false negative]");
	test_result = test_result && verifyError(gl.getError(), GL_NO_ERROR, "getnUniformiv [false negative]");

	pGetnUniformiv(program.m_id, 12, sizeof(GLint) * 2, result3i);
	test_result = test_result && verifyError(gl.getError(), GL_INVALID_OPERATION, "getnUniformiv [false positive]");

	pGetnUniformuiv(program.m_id, 13, sizeof(GLuint) * 4, result4ui);
	test_result = test_result && verifyResult((void*)input4ui, (void*)result4ui, sizeof(GLuint) * 4,
											  "getnUniformuiv [false negative]");
	test_result = test_result && verifyError(gl.getError(), GL_NO_ERROR, "getnUniformuiv [false negative]");

	pGetnUniformuiv(program.m_id, 13, sizeof(GLuint) * 3, result4ui);
	test_result = test_result && verifyError(gl.getError(), GL_INVALID_OPERATION, "getnUniformuiv [false positive]");

	/* Set result */
	if (true == test_result)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
	}

	if (!glslES320)
	{
		gl.deleteBuffers(1, &buf);
	}

	/* Done */
	return tcu::TestNode::STOP;
}

std::string GetnUniformTest::getComputeShader(bool glslES320)
{
	std::stringstream shader;
	shader << "#version " << (glslES320 ? "320 es\n" : "450\n");
	shader << "layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;\n"
			  "layout (location = 11) uniform vec4 inputf;\n"
			  "layout (location = 12) uniform ivec3 inputi;\n"
			  "layout (location = 13) uniform uvec4 inputu;\n";
	if (glslES320)
	{
		shader << "shared float valuef;\n"
				  "shared int valuei;\n"
				  "shared uint valueu;\n";
	}
	else
	{
		shader << "layout (std140, binding = 0) buffer ssbo {"
				  "   float valuef;\n"
				  "   int valuei;\n"
				  "   uint valueu;\n"
				  "};\n";
	}
	shader << "void main()\n"
			  "{\n"
			  "   valuef = inputf.r + inputf.g + inputf.b + inputf.a;\n"
			  "   valuei = inputi.r + inputi.g + inputi.b;\n"
			  "   valueu = inputu.r + inputu.g + inputu.b + inputu.a;\n"
			  "}\n";

	return shader.str();
}

bool GetnUniformTest::verifyResult(const void* inputData, const void* resultData, int size, const char* method)
{
	int diff = memcmp(inputData, resultData, size);
	if (diff != 0)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Test failed! " << method << " result is not as expected."
						   << tcu::TestLog::EndMessage;

		return false;
	}

	return true;
}

bool GetnUniformTest::verifyError(GLint error, GLint expectedError, const char* method)
{
	if (error != expectedError)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Test failed! " << method << " throws unexpected error ["
						   << error << "]." << tcu::TestLog::EndMessage;

		return false;
	}

	return true;
}

/** Implementation of test ReadnPixelsTest. Description follows:
 *
 * This test verifies if read pixels to the buffer with bufSize less than expected result with GL_INVALID_OPERATION error;
 **/
class ReadnPixelsTest : public tcu::TestCase
{
public:
	/* Public methods */
	ReadnPixelsTest(tcu::TestContext& testCtx, glu::ApiType apiType);
	virtual ~ReadnPixelsTest()
	{
	}

	/* Public methods inherited from TestCase */
	virtual tcu::TestNode::IterateResult iterate(void);

private:
	/* Private methods */
	void cleanTexture(deqp::Context& context, glw::GLuint texture_id);
	bool verifyResults(deqp::Context& context);
	bool verifyError(glw::GLint error, glw::GLint expectedError, const char* method);

	glu::ApiType m_ApiType;
};

/** Constructor
 *
 * @param context Test context
 **/
ReadnPixelsTest::ReadnPixelsTest(tcu::TestContext& testCtx, glu::ApiType apiType)
	: tcu::TestCase(testCtx, "readnpixels",
					"Verifies if read pixels to the buffer with bufSize less than expected result "
					"with GL_INVALID_OPERATION error")
	, m_ApiType(apiType)
{
	/* Nothing to be done here */
}

/** Execute test
 *
 * @return tcu::TestNode::STOP
 **/
tcu::TestNode::IterateResult ReadnPixelsTest::iterate()
{
	de::SharedPtr<deqp::Context> context(createContext(m_testCtx, m_ApiType));
	if (!context.get())
		return STOP;

	/* GL funtion pointers. */
	typedef void(GLW_APIENTRY * PFNGLREADNPIXELS)(glw::GLint x, glw::GLint y, glw::GLsizei width, glw::GLsizei height,
												  glw::GLenum format, glw::GLenum type, glw::GLsizei bufSize,
												  glw::GLvoid * data);

	PFNGLREADNPIXELS pReadnPixels = (PFNGLREADNPIXELS)context->getRenderContext().getProcAddress("glReadnPixels");

	if (DE_NULL == pReadnPixels)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_INTERNAL_ERROR, "Pointer to function glReadnPixels is NULL.");
		return STOP;
	}

	static const GLuint elements[] = {
		0, 1, 2, 0, 2, 3, 0, 3, 4, 0, 4, 5, 0, 5, 6, 0, 6, 7, 0, 7, 8, 0, 8, 1,
	};

	static const GLfloat vertices[] = {
		0.0f,  0.0f,  0.0f, 1.0f, /* 0 */
		-1.0f, 0.0f,  0.0f, 1.0f, /* 1 */
		-1.0f, 1.0f,  0.0f, 1.0f, /* 2 */
		0.0f,  1.0f,  0.0f, 1.0f, /* 3 */
		1.0f,  1.0f,  0.0f, 1.0f, /* 4 */
		1.0f,  0.0f,  0.0f, 1.0f, /* 5 */
		1.0f,  -1.0f, 0.0f, 1.0f, /* 6 */
		0.0f,  -1.0f, 0.0f, 1.0f, /* 7 */
		-1.0f, -1.0f, 0.0f, 1.0f, /* 8 */
	};

	bool glslES320 = contextSupports(context->getRenderContext().getType(), glu::ApiType::es(3, 2));
	std::string fs("#version ");
	fs += (glslES320 ? "320 es\n" : "450\n");
	fs += "layout (location = 0) out lowp uvec4 out_fs_color;\n"
		  "\n"
		  "void main()\n"
		  "{\n"
		  "	out_fs_color = uvec4(1, 0, 0, 1);\n"
		  "}\n"
		  "\n";

	std::string vs("#version ");
	vs += (glslES320 ? "320 es\n" : "450\n");
	vs += "layout (location = 0) in vec4 in_vs_position;\n"
		  "\n"
		  "void main()\n"
		  "{\n"
		  "	gl_Position = in_vs_position;\n"
		  "}\n"
		  "\n";

	static const GLuint height	 = 8;
	static const GLuint width	  = 8;
	static const GLuint n_vertices = 24;

	/* GL entry points */
	const Functions& gl = context->getRenderContext().getFunctions();

	/* Test case objects */
	Program		program(gl);
	Texture		texture(gl);
	Buffer		elements_buffer(gl);
	Buffer		vertices_buffer(gl);
	VertexArray vao(gl);

	/* Vertex array initialization */
	VertexArray::Generate(gl, vao.m_id);
	VertexArray::Bind(gl, vao.m_id);

	/* Texture initialization */
	Texture::Generate(gl, texture.m_id);
	Texture::Bind(gl, texture.m_id, GL_TEXTURE_2D);
	Texture::Storage(gl, GL_TEXTURE_2D, 1, GL_R8UI, width, height, 0);
	Texture::Bind(gl, 0, GL_TEXTURE_2D);

	/* Framebuffer initialization */
	GLuint fbo;
	gl.genFramebuffers(1, &fbo);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GenFramebuffers");
	gl.bindFramebuffer(GL_FRAMEBUFFER, fbo);
	GLU_EXPECT_NO_ERROR(gl.getError(), "BindFramebuffer");
	gl.framebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture.m_id, 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "FramebufferTexture2D");

	/* Buffers initialization */
	elements_buffer.InitData(GL_ELEMENT_ARRAY_BUFFER, GL_DYNAMIC_DRAW, sizeof(elements), elements);
	vertices_buffer.InitData(GL_ARRAY_BUFFER, GL_DYNAMIC_DRAW, sizeof(vertices), vertices);

	/* Shaders initialization */
	program.Init("" /* cs */, fs, "" /* gs */, "" /* tcs */, "" /* tes */, vs);
	Program::Use(gl, program.m_id);

	/* Vertex buffer initialization */
	vertices_buffer.Bind();
	gl.bindVertexBuffer(0 /* bindindex = location */, vertices_buffer.m_id, 0 /* offset */, 16 /* stride */);
	gl.vertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 16, NULL);
	gl.enableVertexAttribArray(0 /* location */);

	/* Binding elements/indices buffer */
	elements_buffer.Bind();

	cleanTexture(*context, texture.m_id);

	gl.drawElements(GL_TRIANGLES, n_vertices, GL_UNSIGNED_INT, 0 /* indices */);
	GLU_EXPECT_NO_ERROR(gl.getError(), "DrawElements");

	/* Set result */
	if (verifyResults(*context))
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
	}

	/* Done */
	return tcu::TestNode::STOP;
}

/** Fill texture with value 128
 *
 * @param texture_id Id of texture
 **/
void ReadnPixelsTest::cleanTexture(deqp::Context& context, glw::GLuint texture_id)
{
	static const GLuint height = 8;
	static const GLuint width  = 8;

	const Functions& gl = context.getRenderContext().getFunctions();

	GLubyte pixels[width * height];
	for (GLuint i = 0; i < width * height; ++i)
	{
		pixels[i] = 64;
	}

	Texture::Bind(gl, texture_id, GL_TEXTURE_2D);

	Texture::SubImage(gl, GL_TEXTURE_2D, 0 /* level  */, 0 /* x */, 0 /* y */, 0 /* z */, width, height, 0 /* depth */,
					  GL_RED_INTEGER, GL_UNSIGNED_BYTE, pixels);

	/* Unbind */
	Texture::Bind(gl, 0, GL_TEXTURE_2D);
}

/** Verifies glReadnPixels results
 *
 * @return true when glReadnPixels , false otherwise
 **/
bool ReadnPixelsTest::verifyResults(deqp::Context& context)
{
	static const GLuint height	 = 8;
	static const GLuint width	  = 8;
	static const GLuint pixel_size = 4 * sizeof(GLuint);

	const Functions& gl = context.getRenderContext().getFunctions();

	//Valid buffer size test
	const GLint bufSizeValid = width * height * pixel_size;
	GLubyte		pixelsValid[bufSizeValid];

	gl.readnPixels(0, 0, width, height, GL_RGBA_INTEGER, GL_UNSIGNED_INT, bufSizeValid, pixelsValid);
	GLU_EXPECT_NO_ERROR(gl.getError(), "ReadnPixels");

	//Verify glReadnPixels result
	for (unsigned int i = 0; i < width * height; ++i)
	{
		const size_t offset = i * pixel_size;
		const GLuint value  = *(GLuint*)(pixelsValid + offset);

		if (value != 1)
		{
			context.getTestContext().getLog() << tcu::TestLog::Message << "Invalid pixel value: " << value
											  << ". Offset: " << offset << tcu::TestLog::EndMessage;
			return false;
		}
	}

	//Invalid buffer size test
	const GLint bufSizeInvalid = width * height * pixel_size - 1;
	GLubyte		pixelsInvalid[bufSizeInvalid];

	gl.readnPixels(0, 0, width, height, GL_RGBA_INTEGER, GL_UNSIGNED_INT, bufSizeInvalid, pixelsInvalid);
	if (!verifyError(gl.getError(), GL_INVALID_OPERATION, "ReadnPixels [false positive]"))
		return false;

	return true;
}

/** Verify operation errors
 *
 * @param error OpenGL ES error code
 * @param expectedError Expected error code
 * @param method Method name marker
 *
 * @return true when error is as expected, false otherwise
 **/
bool ReadnPixelsTest::verifyError(GLint error, GLint expectedError, const char* method)
{
	if (error != expectedError)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Test failed! " << method << " throws unexpected error ["
						   << error << "]." << tcu::TestLog::EndMessage;

		return false;
	}

	return true;
}

} // RobustBufferAccessBehavior namespace

RobustnessTests::RobustnessTests(tcu::TestContext& testCtx, glu::ApiType apiType)
	: tcu::TestCaseGroup(testCtx, "robustness",
						 "Verifies API coverage and functionality of GL_KHR_robustness extension.")
	, m_ApiType(apiType)
{
}

void RobustnessTests::init(void)
{
	tcu::TestCaseGroup::init();

	try
	{
		addChild(new ResetNotificationStrategy::NoResetNotificationCase(
			m_testCtx, "no_reset_notification", "Verifies if NO_RESET_NOTIFICATION strategy works as expected.",
			m_ApiType));
		addChild(new ResetNotificationStrategy::LoseContextOnResetCase(
			m_testCtx, "lose_context_on_reset", "Verifies if LOSE_CONTEXT_ON_RESET strategy works as expected.",
			m_ApiType));

		addChild(new RobustBufferAccessBehavior::GetnUniformTest(m_testCtx, m_ApiType));
		addChild(new RobustBufferAccessBehavior::ReadnPixelsTest(m_testCtx, m_ApiType));
	}
	catch (...)
	{
		// Destroy context.
		tcu::TestCaseGroup::deinit();
		throw;
	}
}

} // glcts namespace
