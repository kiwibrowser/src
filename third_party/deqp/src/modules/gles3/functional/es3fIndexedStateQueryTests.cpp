/*-------------------------------------------------------------------------
 * drawElements Quality Program OpenGL ES 3.0 Module
 * -------------------------------------------------
 *
 * Copyright 2014 The Android Open Source Project
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
 *//*!
 * \file
 * \brief Indexed State Query tests.
 *//*--------------------------------------------------------------------*/

#include "es3fIndexedStateQueryTests.hpp"
#include "es3fApiCase.hpp"
#include "glsStateQueryUtil.hpp"
#include "tcuRenderTarget.hpp"
#include "glwEnums.hpp"

using namespace glw; // GLint and other GL types
using deqp::gls::StateQueryUtil::StateQueryMemoryWriteGuard;

namespace deqp
{
namespace gles3
{
namespace Functional
{
namespace
{

void checkIntEquals (tcu::TestContext& testCtx, GLint got, GLint expected)
{
	using tcu::TestLog;

	if (got != expected)
	{
		testCtx.getLog() << TestLog::Message << "// ERROR: Expected " << expected << "; got " << got << TestLog::EndMessage;
		if (testCtx.getTestResult() == QP_TEST_RESULT_PASS)
			testCtx.setTestResult(QP_TEST_RESULT_FAIL, "got invalid value");
	}
}

void checkIntEquals (tcu::TestContext& testCtx, GLint64 got, GLint64 expected)
{
	using tcu::TestLog;

	if (got != expected)
	{
		testCtx.getLog() << TestLog::Message << "// ERROR: Expected " << expected << "; got " << got << TestLog::EndMessage;
		if (testCtx.getTestResult() == QP_TEST_RESULT_PASS)
			testCtx.setTestResult(QP_TEST_RESULT_FAIL, "got invalid value");
	}
}

class TransformFeedbackCase : public ApiCase
{
public:
	TransformFeedbackCase (Context& context, const char* name, const char* description)
		: ApiCase(context, name, description)
	{
	}

	virtual void testTransformFeedback (void) = DE_NULL;

	void test (void)
	{
		static const char* transformFeedbackTestVertSource	=	"#version 300 es\n"
																"out highp vec4 anotherOutput;\n"
																"void main (void)\n"
																"{\n"
																"	gl_Position = vec4(0.0);\n"
																"	anotherOutput = vec4(0.0);\n"
																"}\n\0";
		static const char* transformFeedbackTestFragSource	=	"#version 300 es\n"
																"layout(location = 0) out mediump vec4 fragColor;"
																"void main (void)\n"
																"{\n"
																"	fragColor = vec4(0.0);\n"
																"}\n\0";

		GLuint shaderVert = glCreateShader(GL_VERTEX_SHADER);
		GLuint shaderFrag = glCreateShader(GL_FRAGMENT_SHADER);

		glShaderSource(shaderVert, 1, &transformFeedbackTestVertSource, DE_NULL);
		glShaderSource(shaderFrag, 1, &transformFeedbackTestFragSource, DE_NULL);

		glCompileShader(shaderVert);
		glCompileShader(shaderFrag);
		expectError(GL_NO_ERROR);

		GLuint shaderProg = glCreateProgram();
		glAttachShader(shaderProg, shaderVert);
		glAttachShader(shaderProg, shaderFrag);

		const char* transformFeedbackOutputs[] =
		{
			"gl_Position",
			"anotherOutput"
		};

		glTransformFeedbackVaryings(shaderProg, 2, transformFeedbackOutputs, GL_INTERLEAVED_ATTRIBS);
		glLinkProgram(shaderProg);
		expectError(GL_NO_ERROR);

		GLuint transformFeedbackId = 0;
		glGenTransformFeedbacks(1, &transformFeedbackId);
		glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, transformFeedbackId);
		expectError(GL_NO_ERROR);

		testTransformFeedback();

		// cleanup

		glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, 0);

		glDeleteTransformFeedbacks(1, &transformFeedbackId);
		glDeleteShader(shaderVert);
		glDeleteShader(shaderFrag);
		glDeleteProgram(shaderProg);
		expectError(GL_NO_ERROR);
	}
};

class TransformFeedbackBufferBindingCase : public TransformFeedbackCase
{
public:
	TransformFeedbackBufferBindingCase (Context& context, const char* name, const char* description)
		: TransformFeedbackCase(context, name, description)
	{
	}

	void testTransformFeedback (void)
	{
		const int feedbackPositionIndex = 0;
		const int feedbackOutputIndex = 1;
		const int feedbackIndex[2] = {feedbackPositionIndex, feedbackOutputIndex};

		// bind bffers

		GLuint feedbackBuffers[2];
		glGenBuffers(2, feedbackBuffers);
		expectError(GL_NO_ERROR);

		for (int ndx = 0; ndx < 2; ++ndx)
		{
			glBindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, feedbackBuffers[ndx]);
			glBufferData(GL_TRANSFORM_FEEDBACK_BUFFER, 16, NULL, GL_DYNAMIC_READ);
			glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, feedbackIndex[ndx], feedbackBuffers[ndx]);
			expectError(GL_NO_ERROR);
		}

		// test TRANSFORM_FEEDBACK_BUFFER_BINDING

		for (int ndx = 0; ndx < 2; ++ndx)
		{
			StateQueryMemoryWriteGuard<GLint> boundBuffer;
			glGetIntegeri_v(GL_TRANSFORM_FEEDBACK_BUFFER_BINDING, feedbackIndex[ndx], &boundBuffer);
			boundBuffer.verifyValidity(m_testCtx);
			checkIntEquals(m_testCtx, boundBuffer, feedbackBuffers[ndx]);
		}


		// cleanup

		glDeleteBuffers(2, feedbackBuffers);
	}
};

class TransformFeedbackBufferBufferCase : public TransformFeedbackCase
{
public:
	TransformFeedbackBufferBufferCase (Context& context, const char* name, const char* description)
		: TransformFeedbackCase(context, name, description)
	{
	}

	void testTransformFeedback (void)
	{
		const int feedbackPositionIndex = 0;
		const int feedbackOutputIndex = 1;

		const int rangeBufferOffset = 4;
		const int rangeBufferSize = 8;

		// bind buffers

		GLuint feedbackBuffers[2];
		glGenBuffers(2, feedbackBuffers);
		expectError(GL_NO_ERROR);

		glBindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, feedbackBuffers[0]);
		glBufferData(GL_TRANSFORM_FEEDBACK_BUFFER, 16, NULL, GL_DYNAMIC_READ);
		glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, feedbackPositionIndex, feedbackBuffers[0]);
		expectError(GL_NO_ERROR);

		glBindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, feedbackBuffers[1]);
		glBufferData(GL_TRANSFORM_FEEDBACK_BUFFER, 16, NULL, GL_DYNAMIC_READ);
		glBindBufferRange(GL_TRANSFORM_FEEDBACK_BUFFER, feedbackOutputIndex, feedbackBuffers[1], rangeBufferOffset, rangeBufferSize);
		expectError(GL_NO_ERROR);

		// test TRANSFORM_FEEDBACK_BUFFER_START and TRANSFORM_FEEDBACK_BUFFER_SIZE

		const struct BufferRequirements
		{
			GLint	index;
			GLenum	pname;
			GLint64 value;
		} requirements[] =
		{
			{ feedbackPositionIndex,	GL_TRANSFORM_FEEDBACK_BUFFER_START, 0					},
			{ feedbackPositionIndex,	GL_TRANSFORM_FEEDBACK_BUFFER_SIZE,	0					},
			{ feedbackOutputIndex,		GL_TRANSFORM_FEEDBACK_BUFFER_START, rangeBufferOffset	},
			{ feedbackOutputIndex,		GL_TRANSFORM_FEEDBACK_BUFFER_SIZE,	rangeBufferSize		}
		};

		for (int ndx = 0; ndx < DE_LENGTH_OF_ARRAY(requirements); ++ndx)
		{
			StateQueryMemoryWriteGuard<GLint64> state;
			glGetInteger64i_v(requirements[ndx].pname, requirements[ndx].index, &state);

			if (state.verifyValidity(m_testCtx))
				checkIntEquals(m_testCtx, state, requirements[ndx].value);
		}

		// cleanup

		glDeleteBuffers(2, feedbackBuffers);
	}
};

class UniformBufferCase : public ApiCase
{
public:
	UniformBufferCase (Context& context, const char* name, const char* description)
		: ApiCase	(context, name, description)
		, m_program	(0)
	{
	}

	virtual void testUniformBuffers (void) = DE_NULL;

	void test (void)
	{
		static const char* testVertSource	=	"#version 300 es\n"
												"uniform highp vec4 input1;\n"
												"uniform highp vec4 input2;\n"
												"void main (void)\n"
												"{\n"
												"	gl_Position = input1 + input2;\n"
												"}\n\0";
		static const char* testFragSource	=	"#version 300 es\n"
												"layout(location = 0) out mediump vec4 fragColor;"
												"void main (void)\n"
												"{\n"
												"	fragColor = vec4(0.0);\n"
												"}\n\0";

		GLuint shaderVert = glCreateShader(GL_VERTEX_SHADER);
		GLuint shaderFrag = glCreateShader(GL_FRAGMENT_SHADER);

		glShaderSource(shaderVert, 1, &testVertSource, DE_NULL);
		glShaderSource(shaderFrag, 1, &testFragSource, DE_NULL);

		glCompileShader(shaderVert);
		glCompileShader(shaderFrag);
		expectError(GL_NO_ERROR);

		m_program = glCreateProgram();
		glAttachShader(m_program, shaderVert);
		glAttachShader(m_program, shaderFrag);
		glLinkProgram(m_program);
		glUseProgram(m_program);
		expectError(GL_NO_ERROR);

		testUniformBuffers();

		glUseProgram(0);
		glDeleteShader(shaderVert);
		glDeleteShader(shaderFrag);
		glDeleteProgram(m_program);
		expectError(GL_NO_ERROR);
	}

protected:
	GLuint	m_program;
};

class UniformBufferBindingCase : public UniformBufferCase
{
public:
	UniformBufferBindingCase (Context& context, const char* name, const char* description)
		: UniformBufferCase(context, name, description)
	{
	}

	void testUniformBuffers (void)
	{
		const char* uniformNames[] =
		{
			"input1",
			"input2"
		};
		GLuint uniformIndices[2] = {0};
		glGetUniformIndices(m_program, 2, uniformNames, uniformIndices);

		GLuint buffers[2];
		glGenBuffers(2, buffers);

		for (int ndx = 0; ndx < 2; ++ndx)
		{
			glBindBuffer(GL_UNIFORM_BUFFER, buffers[ndx]);
			glBufferData(GL_UNIFORM_BUFFER, 32, DE_NULL, GL_DYNAMIC_DRAW);
			glBindBufferBase(GL_UNIFORM_BUFFER, uniformIndices[ndx], buffers[ndx]);
			expectError(GL_NO_ERROR);
		}

		for (int ndx = 0; ndx < 2; ++ndx)
		{
			StateQueryMemoryWriteGuard<GLint> boundBuffer;
			glGetIntegeri_v(GL_UNIFORM_BUFFER_BINDING, uniformIndices[ndx], &boundBuffer);

			if (boundBuffer.verifyValidity(m_testCtx))
				checkIntEquals(m_testCtx, boundBuffer, buffers[ndx]);
			expectError(GL_NO_ERROR);
		}

		glDeleteBuffers(2, buffers);
	}
};

class UniformBufferBufferCase : public UniformBufferCase
{
public:
	UniformBufferBufferCase (Context& context, const char* name, const char* description)
		: UniformBufferCase(context, name, description)
	{
	}

	void testUniformBuffers (void)
	{
		const char* uniformNames[] =
		{
			"input1",
			"input2"
		};
		GLuint uniformIndices[2] = {0};
		glGetUniformIndices(m_program, 2, uniformNames, uniformIndices);

		const GLint alignment = GetAlignment();
		if (alignment == -1) // cannot continue without this
			return;

		m_testCtx.getLog() << tcu::TestLog::Message << "Alignment is " << alignment << tcu::TestLog::EndMessage;

		int rangeBufferOffset		= alignment;
		int rangeBufferSize			= alignment * 2;
		int rangeBufferTotalSize	= rangeBufferOffset + rangeBufferSize + 8; // + 8 has no special meaning, just to make it != with the size of the range

		GLuint buffers[2];
		glGenBuffers(2, buffers);

		glBindBuffer(GL_UNIFORM_BUFFER, buffers[0]);
		glBufferData(GL_UNIFORM_BUFFER, 32, DE_NULL, GL_DYNAMIC_DRAW);
		glBindBufferBase(GL_UNIFORM_BUFFER, uniformIndices[0], buffers[0]);
		expectError(GL_NO_ERROR);

		glBindBuffer(GL_UNIFORM_BUFFER, buffers[1]);
		glBufferData(GL_UNIFORM_BUFFER, rangeBufferTotalSize, DE_NULL, GL_DYNAMIC_DRAW);
		glBindBufferRange(GL_UNIFORM_BUFFER, uniformIndices[1], buffers[1], rangeBufferOffset, rangeBufferSize);
		expectError(GL_NO_ERROR);

		// test UNIFORM_BUFFER_START and UNIFORM_BUFFER_SIZE

		const struct BufferRequirements
		{
			GLuint	index;
			GLenum	pname;
			GLint64 value;
		} requirements[] =
		{
			{ uniformIndices[0], GL_UNIFORM_BUFFER_START,	0					},
			{ uniformIndices[0], GL_UNIFORM_BUFFER_SIZE,	0					},
			{ uniformIndices[1], GL_UNIFORM_BUFFER_START,	rangeBufferOffset	},
			{ uniformIndices[1], GL_UNIFORM_BUFFER_SIZE,	rangeBufferSize		}
		};

		for (int ndx = 0; ndx < DE_LENGTH_OF_ARRAY(requirements); ++ndx)
		{
			StateQueryMemoryWriteGuard<GLint64> state;
			glGetInteger64i_v(requirements[ndx].pname, requirements[ndx].index, &state);

			if (state.verifyValidity(m_testCtx))
				checkIntEquals(m_testCtx, state, requirements[ndx].value);
			expectError(GL_NO_ERROR);
		}

		glDeleteBuffers(2, buffers);
	}

	int GetAlignment()
	{
		StateQueryMemoryWriteGuard<GLint> state;
		glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &state);

		if (!state.verifyValidity(m_testCtx))
			return -1;

		if (state <= 256)
			return state;

		m_testCtx.getLog() << tcu::TestLog::Message << "// ERROR: UNIFORM_BUFFER_OFFSET_ALIGNMENT has a maximum value of 256." << tcu::TestLog::EndMessage;
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "invalid UNIFORM_BUFFER_OFFSET_ALIGNMENT value");

		return -1;
	}
};

} // anonymous

IndexedStateQueryTests::IndexedStateQueryTests (Context& context)
	: TestCaseGroup(context, "indexed", "Indexed Integer Values")
{
}

void IndexedStateQueryTests::init (void)
{
	// transform feedback
	addChild(new TransformFeedbackBufferBindingCase(m_context, "transform_feedback_buffer_binding", "TRANSFORM_FEEDBACK_BUFFER_BINDING"));
	addChild(new TransformFeedbackBufferBufferCase(m_context, "transform_feedback_buffer_start_size", "TRANSFORM_FEEDBACK_BUFFER_START and TRANSFORM_FEEDBACK_BUFFER_SIZE"));

	// uniform buffers
	addChild(new UniformBufferBindingCase(m_context, "uniform_buffer_binding", "UNIFORM_BUFFER_BINDING"));
	addChild(new UniformBufferBufferCase(m_context, "uniform_buffer_start_size", "UNIFORM_BUFFER_START and UNIFORM_BUFFER_SIZE"));
}

} // Functional
} // gles3
} // deqp
