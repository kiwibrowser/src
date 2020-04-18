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
 * \file  gl4cIndirectParametersTests.cpp
 * \brief Conformance tests for the GL_ARB_indirect_parameters functionality.
 */ /*-------------------------------------------------------------------*/

#include "gl4cIndirectParametersTests.hpp"
#include "gluContextInfo.hpp"
#include "gluDefs.hpp"
#include "gluDrawUtil.hpp"
#include "gluShaderProgram.hpp"
#include "glwEnums.hpp"
#include "glwFunctions.hpp"
#include "tcuRenderTarget.hpp"
#include "tcuTestLog.hpp"

using namespace glw;
using namespace glu;

namespace gl4cts
{

static const char* c_vertShader = "#version 430\n"
								  "\n"
								  "in vec3 vertex;\n"
								  "\n"
								  "void main()\n"
								  "{\n"
								  "    gl_Position = vec4(vertex, 1);\n"
								  "}\n";

static const char* c_fragShader = "#version 430\n"
								  "\n"
								  "out vec4 fragColor;\n"
								  "\n"
								  "void main()\n"
								  "{\n"
								  "    fragColor = vec4(1, 1, 1, 0.5);\n"
								  "}\n";

/** Constructor.
 *
 *  @param context     Rendering context
 */
ParameterBufferOperationsCase::ParameterBufferOperationsCase(deqp::Context& context)
	: TestCase(context, "ParameterBufferOperations",
			   "Verifies if operations on new buffer object PARAMETER_BUFFER_ARB works as expected.")
{
	/* Left blank intentionally */
}

/** Stub init method */
void ParameterBufferOperationsCase::init()
{
}

/** Executes test iteration.
 *
 *  @return Returns STOP when test has finished executing, CONTINUE if more iterations are needed.
 */
tcu::TestNode::IterateResult ParameterBufferOperationsCase::iterate()
{
	glu::ContextType contextType = m_context.getRenderContext().getType();
	if (!glu::contextSupports(contextType, glu::ApiType::core(4, 6)) &&
		!m_context.getContextInfo().isExtensionSupported("GL_ARB_indirect_parameters"))
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "Not Supported");
		return STOP;
	}

	const Functions& gl = m_context.getRenderContext().getFunctions();

	GLuint paramBuffer;

	GLint data[]	= { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
	GLint subData[] = { 10, 11, 12, 13, 14 };
	GLint expData[] = { 0, 1, 10, 11, 12, 13, 14, 7, 8, 9 };

	bool result = true;

	// Test buffer generating and binding
	gl.genBuffers(1, &paramBuffer);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenBuffers");

	gl.bindBuffer(GL_PARAMETER_BUFFER_ARB, paramBuffer);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer");

	GLint paramBinding;
	gl.getIntegerv(GL_PARAMETER_BUFFER_BINDING_ARB, &paramBinding);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetIntegerv");

	if ((GLuint)paramBinding != paramBuffer)
	{
		result = false;
		m_testCtx.getLog() << tcu::TestLog::Message << "Buffer binding mismatch" << tcu::TestLog::EndMessage;
	}
	else
	{
		// Test filling buffer with data
		gl.bufferData(GL_PARAMETER_BUFFER_ARB, 10 * sizeof(GLint), data, GL_DYNAMIC_DRAW);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferData");

		gl.bufferSubData(GL_PARAMETER_BUFFER_ARB, 2 * sizeof(GLint), 5 * sizeof(GLint), subData);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferData");

		// Test buffer mapping
		GLvoid* buffer = gl.mapBuffer(GL_PARAMETER_BUFFER_ARB, GL_READ_ONLY);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glMapBuffer");

		if (memcmp(buffer, expData, 10 * sizeof(GLint)) != 0)
		{
			result = false;
			m_testCtx.getLog() << tcu::TestLog::Message << "Buffer data mismatch" << tcu::TestLog::EndMessage;
		}
		else
		{
			GLvoid* bufferPointer;
			gl.getBufferPointerv(GL_PARAMETER_BUFFER_ARB, GL_BUFFER_MAP_POINTER, &bufferPointer);
			GLU_EXPECT_NO_ERROR(gl.getError(), "glGetBufferPointerv");

			if (buffer != bufferPointer)
			{
				result = false;
				m_testCtx.getLog() << tcu::TestLog::Message << "Buffer pointer mismatch" << tcu::TestLog::EndMessage;
			}
			else
			{
				GLint bufferSize;
				GLint bufferUsage;
				gl.getBufferParameteriv(GL_PARAMETER_BUFFER_ARB, GL_BUFFER_SIZE, &bufferSize);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glGetBufferParameteriv");
				gl.getBufferParameteriv(GL_PARAMETER_BUFFER_ARB, GL_BUFFER_USAGE, &bufferUsage);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glGetBufferParameteriv");

				if (bufferSize != 10 * sizeof(GLint))
				{
					result = false;
					m_testCtx.getLog() << tcu::TestLog::Message << "Buffer size mismatch" << tcu::TestLog::EndMessage;
				}
				else if (bufferUsage != GL_DYNAMIC_DRAW)
				{
					result = false;
					m_testCtx.getLog() << tcu::TestLog::Message << "Buffer usage mismatch" << tcu::TestLog::EndMessage;
				}
			}
		}

		gl.unmapBuffer(GL_PARAMETER_BUFFER_ARB);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glUnmapBuffer");

		// Test buffer ranged mapping
		buffer =
			gl.mapBufferRange(GL_PARAMETER_BUFFER_ARB, 0, sizeof(GLint), GL_MAP_WRITE_BIT | GL_MAP_FLUSH_EXPLICIT_BIT);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glMapBufferRange");

		// Test buffer flushing
		GLint* bufferInt = (GLint*)buffer;

		bufferInt[0] = 100;
		gl.flushMappedBufferRange(GL_PARAMETER_BUFFER_ARB, 0, sizeof(GLint));
		GLU_EXPECT_NO_ERROR(gl.getError(), "glFlushMappedBufferRange");

		gl.unmapBuffer(GL_PARAMETER_BUFFER_ARB);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glUnmapBuffer");

		// Test buffers data copying
		GLuint arrayBuffer;
		gl.genBuffers(1, &arrayBuffer);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glGenBuffers");

		gl.bindBuffer(GL_ARRAY_BUFFER, arrayBuffer);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer");

		gl.bufferData(GL_ARRAY_BUFFER, 10 * sizeof(GLint), data, GL_DYNAMIC_DRAW);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferData");

		gl.copyBufferSubData(GL_PARAMETER_BUFFER_ARB, GL_ARRAY_BUFFER, 0, 0, sizeof(GLint));
		GLU_EXPECT_NO_ERROR(gl.getError(), "glCopyBufferSubData");

		gl.mapBufferRange(GL_ARRAY_BUFFER, 0, 1, GL_MAP_READ_BIT);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glMapBufferRange");

		bufferInt = (GLint*)buffer;
		if (bufferInt[0] != 100)
		{
			result = false;
			m_testCtx.getLog() << tcu::TestLog::Message << "Buffer copy operation failed" << tcu::TestLog::EndMessage;
		}

		gl.unmapBuffer(GL_ARRAY_BUFFER);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glUnmapBuffer");

		// Release array buffer
		gl.deleteBuffers(1, &arrayBuffer);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glDeleteBuffers");
	}

	// Release parameter buffer
	gl.deleteBuffers(1, &paramBuffer);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glDeleteBuffers");

	if (result)
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	else
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");

	return STOP;
}

/** Constructor.
 *
 *  @param context     Rendering context
 */
VertexArrayIndirectDrawingBaseCase::VertexArrayIndirectDrawingBaseCase(deqp::Context& context, const char* name,
																	   const char* description)
	: TestCase(context, name, description)
{
	/* Left blank intentionally */
}

/** Executes test iteration.
 *
 *  @return Returns STOP when test has finished executing, CONTINUE if more iterations are needed.
 */
tcu::TestNode::IterateResult VertexArrayIndirectDrawingBaseCase::iterate()
{
	glu::ContextType contextType = m_context.getRenderContext().getType();
	if (!glu::contextSupports(contextType, glu::ApiType::core(4, 6)) &&
		!m_context.getContextInfo().isExtensionSupported("GL_ARB_indirect_parameters"))
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "Not Supported");
		return STOP;
	}

	if (draw() && verify())
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	else
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");

	return STOP;
}

/** This method verifies if drawn quads are as expected.
 *
 *  @return Returns true if quads are drawn properly, false otherwise.
 */
bool VertexArrayIndirectDrawingBaseCase::verify()
{
	const Functions&		 gl = m_context.getRenderContext().getFunctions();
	const tcu::RenderTarget& rt = m_context.getRenderContext().getRenderTarget();

	const int width  = rt.getWidth();
	const int height = rt.getHeight();

	std::vector<GLubyte> pixels;
	pixels.resize(width * height);

	gl.pixelStorei(GL_PACK_ALIGNMENT, 1);
	gl.readPixels(0, 0, width, height, GL_RED, GL_UNSIGNED_BYTE, pixels.data());
	gl.pixelStorei(GL_PACK_ALIGNMENT, 4);

	//Verify first quad
	for (int y = 2; y < height - 2; ++y)
	{
		for (int x = 2; x < width / 2 - 2; ++x)
		{
			GLubyte value = pixels[x + y * width];
			if (value < 190 || value > 194)
			{
				m_testCtx.getLog() << tcu::TestLog::Message << "First quad verification failed. "
								   << "Wrong value read from framebuffer at " << x << "/" << y << " value: " << value
								   << ", expected: <190-194>" << tcu::TestLog::EndMessage;
				return false;
			}
		}
	}

	//Verify second quad
	for (int y = 2; y < height - 2; ++y)
	{
		for (int x = width / 2 + 2; x < width - 2; ++x)
		{
			GLubyte value = pixels[x + y * width];
			if (value < 126 || value > 130)
			{
				m_testCtx.getLog() << tcu::TestLog::Message << "Second quad verification failed. "
								   << "Wrong value read from framebuffer at " << x << "/" << y << " value: " << value
								   << ", expected: <126-130>" << tcu::TestLog::EndMessage;
				return false;
			}
		}
	}

	return verifyErrors();
}

/** Constructor.
 *
 *  @param context     Rendering context
 */
MultiDrawArraysIndirectCountCase::MultiDrawArraysIndirectCountCase(deqp::Context& context)
	: VertexArrayIndirectDrawingBaseCase(context, "MultiDrawArraysIndirectCount",
										 "Test verifies if MultiDrawArraysIndirectCountARB function works as expected.")
	, m_vao(0)
	, m_arrayBuffer(0)
	, m_drawIndirectBuffer(0)
	, m_parameterBuffer(0)
{
	/* Left blank intentionally */
}

/** Stub init method */
void MultiDrawArraysIndirectCountCase::init()
{
	glu::ContextType contextType = m_context.getRenderContext().getType();
	if (!glu::contextSupports(contextType, glu::ApiType::core(4, 6)) &&
		!m_context.getContextInfo().isExtensionSupported("GL_ARB_indirect_parameters"))
		return;

	const Functions& gl = m_context.getRenderContext().getFunctions();

	const GLfloat vertices[] = { -1.0f, -1.0f, 0.0f, -1.0f, 1.0f,  0.0f, 0.0f, -1.0f, 0.0f,
								 0.0f,  1.0f,  0.0f, 1.0f,  -1.0f, 0.0f, 1.0f, 1.0f,  0.0f };

	const DrawArraysIndirectCommand indirect[] = {
		{ 4, 2, 0, 0 }, //4 vertices, 2 instanceCount, 0 first, 0 baseInstance
		{ 4, 1, 2, 0 }  //4 vertices, 1 instanceCount, 2 first, 0 baseInstance
	};

	const GLushort parameters[] = { 2, 1 };

	// Generate vertex array object
	gl.genVertexArrays(1, &m_vao);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenVertexArrays");

	gl.bindVertexArray(m_vao);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindVertexArray");

	// Setup vertex array buffer
	gl.genBuffers(1, &m_arrayBuffer);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenBuffers");

	gl.bindBuffer(GL_ARRAY_BUFFER, m_arrayBuffer);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer");

	gl.bufferData(GL_ARRAY_BUFFER, 24 * sizeof(GLfloat), vertices, GL_STATIC_DRAW);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferData");

	// Setup indirect command buffer
	gl.genBuffers(1, &m_drawIndirectBuffer);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenBuffers");

	gl.bindBuffer(GL_DRAW_INDIRECT_BUFFER, m_drawIndirectBuffer);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer");

	gl.bufferData(GL_DRAW_INDIRECT_BUFFER, 2 * sizeof(DrawArraysIndirectCommand), indirect, GL_STATIC_DRAW);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferData");

	// Setup parameter buffer
	gl.genBuffers(1, &m_parameterBuffer);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenBuffers");

	gl.bindBuffer(GL_PARAMETER_BUFFER_ARB, m_parameterBuffer);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer");

	gl.bufferData(GL_PARAMETER_BUFFER_ARB, 100 * sizeof(GLushort), parameters, GL_STATIC_DRAW);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferData");
}

/** Stub deinit method */
void MultiDrawArraysIndirectCountCase::deinit()
{
	const Functions& gl = m_context.getRenderContext().getFunctions();

	if (m_vao)
		gl.deleteVertexArrays(1, &m_vao);
	if (m_arrayBuffer)
		gl.deleteBuffers(1, &m_arrayBuffer);
	if (m_drawIndirectBuffer)
		gl.deleteBuffers(1, &m_drawIndirectBuffer);
	if (m_parameterBuffer)
		gl.deleteBuffers(1, &m_parameterBuffer);
}

/** Drawing quads method using drawArrays.
 */
bool MultiDrawArraysIndirectCountCase::draw()
{
	const Functions& gl = m_context.getRenderContext().getFunctions();

	ProgramSources sources = makeVtxFragSources(c_vertShader, c_fragShader);
	ShaderProgram  program(gl, sources);

	if (!program.isOk())
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Shader build failed.\n"
						   << "Vertex: " << program.getShaderInfo(SHADERTYPE_VERTEX).infoLog << "\n"
						   << "Fragment: " << program.getShaderInfo(SHADERTYPE_FRAGMENT).infoLog << "\n"
						   << "Program: " << program.getProgramInfo().infoLog << tcu::TestLog::EndMessage;
		return false;
	}

	gl.useProgram(program.getProgram());
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram");

	gl.clearColor(0.0f, 0.0f, 0.0f, 1.0f);
	gl.clear(GL_COLOR_BUFFER_BIT);

	gl.enable(GL_BLEND);
	gl.blendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	gl.enableVertexAttribArray(0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glEnableVertexAttribArray");
	gl.vertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glVertexAttribPointer");

	gl.multiDrawArraysIndirectCount(GL_TRIANGLE_STRIP, 0, 0, 2, 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glMultiDrawArraysIndirectCountARB");

	gl.disableVertexAttribArray(0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glDisableVertexAttribArray");

	gl.disable(GL_BLEND);

	return true;
}

/** Verify MultiDrawArrayIndirectCountARB errors
 */
bool MultiDrawArraysIndirectCountCase::verifyErrors()
{
	const Functions& gl = m_context.getRenderContext().getFunctions();

	GLint errorCode;

	bool result = true;

	// INVALID_VALUE - drawcount offset not multiple of 4
	gl.multiDrawArraysIndirectCount(GL_TRIANGLE_STRIP, 0, 2, 1, 0);
	errorCode = gl.getError();
	if (errorCode != GL_INVALID_VALUE)
	{
		m_testCtx.getLog() << tcu::TestLog::Message
						   << "MultiDrawArraysIndirectCount error verifying failed (1). Expected code: "
						   << GL_INVALID_VALUE << ", current code: " << errorCode << tcu::TestLog::EndMessage;
		result = false;
	}

	// INVALID_OPERATION - maxdrawcount greater then parameter buffer size
	gl.multiDrawArraysIndirectCount(GL_TRIANGLE_STRIP, 0, 0, 4, 0);
	errorCode = gl.getError();
	if (errorCode != GL_INVALID_OPERATION)
	{
		m_testCtx.getLog() << tcu::TestLog::Message
						   << "MultiDrawArraysIndirectCount error verifying failed (2). Expected code: "
						   << GL_INVALID_OPERATION << ", current code: " << errorCode << tcu::TestLog::EndMessage;
		result = false;
	}

	gl.bindBuffer(GL_PARAMETER_BUFFER_ARB, 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer");

	// INVALID_OPERATION - GL_PARAMETER_BUFFER_ARB not bound
	gl.multiDrawArraysIndirectCount(GL_TRIANGLE_STRIP, 0, 0, 2, 0);
	errorCode = gl.getError();
	if (errorCode != GL_INVALID_OPERATION)
	{
		m_testCtx.getLog() << tcu::TestLog::Message
						   << "MultiDrawArraysIndirectCount error verifying failed (3). Expected code: "
						   << GL_INVALID_OPERATION << ", current code: " << errorCode << tcu::TestLog::EndMessage;
		result = false;
	}

	return result;
}

/** Constructor.
 *
 *  @param context     Rendering context
 */
MultiDrawElementsIndirectCountCase::MultiDrawElementsIndirectCountCase(deqp::Context& context)
	: VertexArrayIndirectDrawingBaseCase(
		  context, "MultiDrawElementsIndirectCount",
		  "Test verifies if MultiDrawElementsIndirectCountARB function works as expected.")
	, m_vao(0)
	, m_arrayBuffer(0)
	, m_drawIndirectBuffer(0)
	, m_parameterBuffer(0)
{
	/* Left blank intentionally */
}

/** Stub init method */
void MultiDrawElementsIndirectCountCase::init()
{
	glu::ContextType contextType = m_context.getRenderContext().getType();
	if (!glu::contextSupports(contextType, glu::ApiType::core(4, 6)) &&
		!m_context.getContextInfo().isExtensionSupported("GL_ARB_indirect_parameters"))
		return;

	const Functions& gl = m_context.getRenderContext().getFunctions();

	const GLfloat vertices[] = { -1.0f, -1.0f, 0.0f, -1.0f, 1.0f,  0.0f, 0.0f, -1.0f, 0.0f,
								 0.0f,  1.0f,  0.0f, 1.0f,  -1.0f, 0.0f, 1.0f, 1.0f,  0.0f };

	const GLushort elements[] = { 0, 1, 2, 3, 4, 5 };

	const DrawElementsIndirectCommand indirect[] = {
		{ 4, 2, 0, 0, 0 }, //4 indices, 2 instanceCount, 0 firstIndex, 0 baseVertex, 0 baseInstance
		{ 4, 1, 2, 0, 0 }  //4 indices, 1 instanceCount, 2 firstIndex, 0 baseVertex, 0 baseInstance
	};

	const GLushort parameters[] = { 2, 1 };

	// Generate vertex array object
	gl.genVertexArrays(1, &m_vao);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenVertexArrays");

	gl.bindVertexArray(m_vao);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindVertexArray");

	// Setup vertex array buffer
	gl.genBuffers(1, &m_arrayBuffer);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenBuffers");

	gl.bindBuffer(GL_ARRAY_BUFFER, m_arrayBuffer);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer");

	gl.bufferData(GL_ARRAY_BUFFER, 24 * sizeof(GLfloat), vertices, GL_STATIC_DRAW);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferData");

	// Setup element array buffer
	gl.genBuffers(1, &m_elementBuffer);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenBuffers");

	gl.bindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_elementBuffer);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer");

	gl.bufferData(GL_ELEMENT_ARRAY_BUFFER, 6 * sizeof(GLushort), elements, GL_STATIC_DRAW);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferData");

	// Setup indirect command buffer
	gl.genBuffers(1, &m_drawIndirectBuffer);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenBuffers");

	gl.bindBuffer(GL_DRAW_INDIRECT_BUFFER, m_drawIndirectBuffer);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer");

	gl.bufferData(GL_DRAW_INDIRECT_BUFFER, 3 * sizeof(DrawElementsIndirectCommand), indirect, GL_STATIC_DRAW);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferData");

	// Setup parameters Re: buffer
	gl.genBuffers(1, &m_parameterBuffer);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenBuffers");

	gl.bindBuffer(GL_PARAMETER_BUFFER_ARB, m_parameterBuffer);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer");

	gl.bufferData(GL_PARAMETER_BUFFER_ARB, 2 * sizeof(GLushort), parameters, GL_STATIC_DRAW);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferData");
}

/** Stub deinit method */
void MultiDrawElementsIndirectCountCase::deinit()
{
	const Functions& gl = m_context.getRenderContext().getFunctions();

	if (m_vao)
		gl.deleteVertexArrays(1, &m_vao);
	if (m_arrayBuffer)
		gl.deleteBuffers(1, &m_arrayBuffer);
	if (m_elementBuffer)
		gl.deleteBuffers(1, &m_elementBuffer);
	if (m_drawIndirectBuffer)
		gl.deleteBuffers(1, &m_drawIndirectBuffer);
	if (m_parameterBuffer)
		gl.deleteBuffers(1, &m_parameterBuffer);
}

/** Drawing quads method using drawArrays.
 */
bool MultiDrawElementsIndirectCountCase::draw()
{
	const Functions& gl = m_context.getRenderContext().getFunctions();

	ProgramSources sources = makeVtxFragSources(c_vertShader, c_fragShader);
	ShaderProgram  program(gl, sources);

	if (!program.isOk())
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Shader build failed.\n"
						   << "Vertex: " << program.getShaderInfo(SHADERTYPE_VERTEX).infoLog << "\n"
						   << "Fragment: " << program.getShaderInfo(SHADERTYPE_FRAGMENT).infoLog << "\n"
						   << "Program: " << program.getProgramInfo().infoLog << tcu::TestLog::EndMessage;
		return false;
	}

	gl.useProgram(program.getProgram());
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram");

	gl.clearColor(0.0f, 0.0f, 0.0f, 1.0f);
	gl.clear(GL_COLOR_BUFFER_BIT);

	gl.enable(GL_BLEND);
	gl.blendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	gl.enableVertexAttribArray(0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glEnableVertexAttribArray");
	gl.vertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glVertexAttribPointer");

	gl.multiDrawElementsIndirectCount(GL_TRIANGLE_STRIP, GL_UNSIGNED_SHORT, 0, 0, 2, 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glMultiDrawElementsIndirectCountARB");

	gl.disableVertexAttribArray(0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glDisableVertexAttribArray");

	gl.disable(GL_BLEND);

	return true;
}

/** Verify MultiDrawElementsIndirectCountARB errors
 */
bool MultiDrawElementsIndirectCountCase::verifyErrors()
{
	const Functions& gl = m_context.getRenderContext().getFunctions();

	GLint errorCode;

	bool result = true;

	// INVALID_VALUE - drawcount offset not multiple of 4
	gl.multiDrawElementsIndirectCount(GL_TRIANGLE_STRIP, GL_UNSIGNED_BYTE, 0, 2, 1, 0);
	errorCode = gl.getError();
	if (errorCode != GL_INVALID_VALUE)
	{
		m_testCtx.getLog() << tcu::TestLog::Message
						   << "MultiDrawElementIndirectCount error verifying failed (1). Expected code: "
						   << GL_INVALID_VALUE << ", current code: " << errorCode << tcu::TestLog::EndMessage;
		result = false;
	}

	// INVALID_OPERATION - maxdrawcount greater then parameter buffer size
	gl.multiDrawElementsIndirectCount(GL_TRIANGLE_STRIP, GL_UNSIGNED_BYTE, 0, 0, 4, 0);
	errorCode = gl.getError();
	if (errorCode != GL_INVALID_OPERATION)
	{
		m_testCtx.getLog() << tcu::TestLog::Message
						   << "MultiDrawElementIndirectCount error verifying failed (2). Expected code: "
						   << GL_INVALID_OPERATION << ", current code: " << errorCode << tcu::TestLog::EndMessage;
		result = false;
	}

	gl.bindBuffer(GL_PARAMETER_BUFFER_ARB, 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer");

	// INVALID_OPERATION - GL_PARAMETER_BUFFER_ARB not bound
	gl.multiDrawElementsIndirectCount(GL_TRIANGLE_STRIP, GL_UNSIGNED_BYTE, 0, 0, 3, 0);
	errorCode = gl.getError();
	if (errorCode != GL_INVALID_OPERATION)
	{
		m_testCtx.getLog() << tcu::TestLog::Message
						   << "MultiDrawElementIndirectCount error verifying failed (3). Expected code: "
						   << GL_INVALID_OPERATION << ", current code: " << errorCode << tcu::TestLog::EndMessage;
		result = false;
	}

	return result;
}

/** Constructor.
 *
 *  @param context Rendering context.
 */
IndirectParametersTests::IndirectParametersTests(deqp::Context& context)
	: TestCaseGroup(context, "indirect_parameters_tests",
					"Verify conformance of CTS_ARB_indirect_parameters implementation")
{
}

/** Initializes the test group contents. */
void IndirectParametersTests::init()
{
	addChild(new ParameterBufferOperationsCase(m_context));
	addChild(new MultiDrawArraysIndirectCountCase(m_context));
	addChild(new MultiDrawElementsIndirectCountCase(m_context));
}

} /* gl4cts namespace */
