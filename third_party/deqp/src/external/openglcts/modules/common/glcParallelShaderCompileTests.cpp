/*-------------------------------------------------------------------------
 * OpenGL Conformance Test Suite
 * -----------------------------
 *
 * Copyright (c) 2016-2017 The Khronos Group Inc.
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
 * \file  glcParallelShaderCompileTests.cpp
 * \brief Conformance tests for the GL_KHR_parallel_shader_compile functionality.
 */ /*-------------------------------------------------------------------*/

#include "glcParallelShaderCompileTests.hpp"
#include "deClock.h"
#include "gluContextInfo.hpp"
#include "gluDefs.hpp"
#include "gluShaderProgram.hpp"
#include "glwEnums.hpp"
#include "glwFunctions.hpp"
#include "tcuTestLog.hpp"

using namespace glu;
using namespace glw;

namespace glcts
{

static const char* shaderVersionES = "#version 300 es\n";
static const char* shaderVersionGL = "#version 450\n";
static const char* vShader		   = "\n"
							 "in vec3 vertex;\n"
							 "\n"
							 "int main() {\n"
							 "    gl_Position = vec4(vertex, 1);\n"
							 "}\n";

static const char* fShader = "\n"
							 "out ver4 fragColor;\n"
							 "\n"
							 "int main() {\n"
							 "    fragColor = vec4(1, 1, 1, 1);\n"
							 "}\n";

/** Constructor.
 *
 *  @param context     Rendering context
 *  @param name        Test name
 *  @param description Test description
 */
SimpleQueriesTest::SimpleQueriesTest(deqp::Context& context)
	: TestCase(context, "simple_queries",
			   "Tests verifies if simple queries works as expected for MAX_SHADER_COMPILER_THREADS_KHR <pname>")
{
	/* Left blank intentionally */
}

/** Executes test iteration.
 *
 *  @return Returns STOP when test has finished executing, CONTINUE if more iterations are needed.
 */
tcu::TestNode::IterateResult SimpleQueriesTest::iterate()
{
	const glu::ContextInfo& contextInfo		= m_context.getContextInfo();
	const glu::ContextType& contextType		= m_context.getRenderContext().getType();
	const bool				isGL			= glu::isContextTypeGLCore(contextType);
	const bool				supportParallel	= (isGL && contextInfo.isExtensionSupported("GL_ARB_parallel_shader_compile")) ||
												contextInfo.isExtensionSupported("GL_KHR_parallel_shader_compile");

	if (!supportParallel)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "Not supported");
		return STOP;
	}

	const Functions&		gl			 = m_context.getRenderContext().getFunctions();

	GLboolean boolValue;
	GLint	 intValue;
	GLint64   int64Value;
	GLfloat   floatValue;
	GLdouble  doubleValue;

	bool supportsInt64  = isGL || glu::contextSupports(contextType, glu::ApiType::es(3, 0));
	bool supportsDouble = isGL;

	gl.getBooleanv(GL_MAX_SHADER_COMPILER_THREADS_KHR, &boolValue);
	GLU_EXPECT_NO_ERROR(gl.getError(), "getBooleanv");

	gl.getIntegerv(GL_MAX_SHADER_COMPILER_THREADS_KHR, &intValue);
	GLU_EXPECT_NO_ERROR(gl.getError(), "getIntegerv");

	if (supportsInt64)
	{
		gl.getInteger64v(GL_MAX_SHADER_COMPILER_THREADS_KHR, &int64Value);
		GLU_EXPECT_NO_ERROR(gl.getError(), "getInteger64v");
	}

	gl.getFloatv(GL_MAX_SHADER_COMPILER_THREADS_KHR, &floatValue);
	GLU_EXPECT_NO_ERROR(gl.getError(), "getFloatv");

	if (supportsDouble)
	{
		gl.getDoublev(GL_MAX_SHADER_COMPILER_THREADS_KHR, &doubleValue);
		GLU_EXPECT_NO_ERROR(gl.getError(), "getDoublev");
	}

	if (boolValue != (intValue != 0) || intValue != (GLint)floatValue ||
		(supportsInt64 && intValue != (GLint)int64Value) || (supportsDouble && intValue != (GLint)doubleValue))
	{
		tcu::MessageBuilder message = m_testCtx.getLog() << tcu::TestLog::Message;

		message << "Simple queries returned different values: "
				<< "bool(" << (int)boolValue << "), "
				<< "int(" << intValue << "), ";

		if (supportsInt64)
			message << "int64(" << int64Value << "), ";

		message << "float(" << floatValue << ")";

		if (supportsDouble)
			message << ", double(" << doubleValue << ")";

		message << tcu::TestLog::EndMessage;

		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
		return STOP;
	}

	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	return STOP;
}

/** Constructor.
 *
 *  @param context     Rendering context
 *  @param name        Test name
 *  @param description Test description
 */
MaxShaderCompileThreadsTest::MaxShaderCompileThreadsTest(deqp::Context& context)
	: TestCase(context, "max_shader_compile_threads",
			   "Tests verifies if MaxShaderCompileThreadsKHR function works as expected")
{
	/* Left blank intentionally */
}

/** Executes test iteration.
 *
 *  @return Returns STOP when test has finished executing, CONTINUE if more iterations are needed.
 */
tcu::TestNode::IterateResult MaxShaderCompileThreadsTest::iterate()
{
	const glu::ContextInfo& contextInfo		= m_context.getContextInfo();
	const glu::ContextType& contextType		= m_context.getRenderContext().getType();
	const bool				isGL			= glu::isContextTypeGLCore(contextType);
	const bool				supportParallel	= (isGL && contextInfo.isExtensionSupported("GL_ARB_parallel_shader_compile")) ||
												contextInfo.isExtensionSupported("GL_KHR_parallel_shader_compile");

	if (!supportParallel)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "Not supported");
		return STOP;
	}

	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	GLint intValue;

	gl.maxShaderCompilerThreadsKHR(0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "maxShaderCompilerThreadsKHR");

	gl.getIntegerv(GL_MAX_SHADER_COMPILER_THREADS_KHR, &intValue);
	GLU_EXPECT_NO_ERROR(gl.getError(), "getIntegerv");

	if (intValue != 0)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Failed to disable parallel shader compilation.");
		return STOP;
	}

	gl.maxShaderCompilerThreadsKHR(0xFFFFFFFF);
	GLU_EXPECT_NO_ERROR(gl.getError(), "maxShaderCompilerThreadsKHR");

	gl.getIntegerv(GL_MAX_SHADER_COMPILER_THREADS_KHR, &intValue);
	GLU_EXPECT_NO_ERROR(gl.getError(), "getIntegerv");

	if (intValue != GLint(0xFFFFFFFF))
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Failed to set maximum shader compiler threads.");
		return STOP;
	}

	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	return STOP;
}

/** Constructor.
 *
 *  @param context     Rendering context
 *  @param name        Test name
 *  @param description Test description
 */
CompilationCompletionNonParallelTest::CompilationCompletionNonParallelTest(deqp::Context& context)
	: TestCase(context, "compilation_completion_non_parallel",
			   "Tests verifies if shader COMPLETION_STATUS query works as expected for non parallel compilation")
{
	/* Left blank intentionally */
}

/** Executes test iteration.
 *
 *  @return Returns STOP when test has finished executing, CONTINUE if more iterations are needed.
 */
tcu::TestNode::IterateResult CompilationCompletionNonParallelTest::iterate()
{
	const glu::ContextInfo& contextInfo		= m_context.getContextInfo();
	const glu::ContextType& contextType		= m_context.getRenderContext().getType();
	const bool				isGL			= glu::isContextTypeGLCore(contextType);
	const bool				supportParallel	= (isGL && contextInfo.isExtensionSupported("GL_ARB_parallel_shader_compile")) ||
												contextInfo.isExtensionSupported("GL_KHR_parallel_shader_compile");

	if (!supportParallel)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "Not supported");
		return STOP;
	}

	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	GLint completionStatus;

	gl.maxShaderCompilerThreadsKHR(0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "maxShaderCompilerThreadsKHR");

	{
		Program program(gl);
		Shader  vertexShader(gl, SHADERTYPE_VERTEX);
		Shader  fragmentShader(gl, SHADERTYPE_FRAGMENT);

		bool		isContextES   = (glu::isContextTypeES(m_context.getRenderContext().getType()));
		const char* shaderVersion = isContextES ? shaderVersionES : shaderVersionGL;

		const char* vSources[] = { shaderVersion, vShader };
		const int   vLengths[] = { int(strlen(shaderVersion)), int(strlen(vShader)) };
		vertexShader.setSources(2, vSources, vLengths);

		const char* fSources[] = { shaderVersion, fShader };
		const int   fLengths[] = { int(strlen(shaderVersion)), int(strlen(fShader)) };
		fragmentShader.setSources(2, fSources, fLengths);

		gl.compileShader(vertexShader.getShader());
		GLU_EXPECT_NO_ERROR(gl.getError(), "compileShader");
		gl.compileShader(fragmentShader.getShader());
		GLU_EXPECT_NO_ERROR(gl.getError(), "compileShader");

		gl.getShaderiv(fragmentShader.getShader(), GL_COMPLETION_STATUS_KHR, &completionStatus);
		GLU_EXPECT_NO_ERROR(gl.getError(), "getShaderiv");
		if (!completionStatus)
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL,
									"Failed reading completion status for non parallel shader compiling");
			return STOP;
		}

		program.attachShader(vertexShader.getShader());
		program.attachShader(fragmentShader.getShader());
		gl.linkProgram(program.getProgram());

		gl.getProgramiv(program.getProgram(), GL_COMPLETION_STATUS_KHR, &completionStatus);
		GLU_EXPECT_NO_ERROR(gl.getError(), "getProgramiv");
		if (!completionStatus)
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL,
									"Failed reading completion status for non parallel program linking");
			return STOP;
		}
	}

	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	return STOP;
}

/** Constructor.
 *
 *  @param context     Rendering context
 *  @param name        Test name
 *  @param description Test description
 */
CompilationCompletionParallelTest::CompilationCompletionParallelTest(deqp::Context& context)
	: TestCase(context, "compilation_completion_parallel",
			   "Tests verifies if shader COMPLETION_STATUS query works as expected for parallel compilation")
{
	/* Left blank intentionally */
}

/** Executes test iteration.
 *
 *  @return Returns STOP when test has finished executing, CONTINUE if more iterations are needed.
 */
tcu::TestNode::IterateResult CompilationCompletionParallelTest::iterate()
{
	const glu::ContextInfo& contextInfo		= m_context.getContextInfo();
	const glu::ContextType& contextType		= m_context.getRenderContext().getType();
	const bool				isGL			= glu::isContextTypeGLCore(contextType);
	const bool				supportParallel	= (isGL && contextInfo.isExtensionSupported("GL_ARB_parallel_shader_compile")) ||
												contextInfo.isExtensionSupported("GL_KHR_parallel_shader_compile");

	if (!supportParallel)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "Not supported");
		return STOP;
	}

	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	GLint completionStatus;

	gl.maxShaderCompilerThreadsKHR(8);
	GLU_EXPECT_NO_ERROR(gl.getError(), "maxShaderCompilerThreadsKHR");

	{
		Shader   vertexShader(gl, SHADERTYPE_VERTEX);
		deUint32 fragmentShader[8];
		deUint32 program[8];

		bool		isContextES   = (glu::isContextTypeES(m_context.getRenderContext().getType()));
		const char* shaderVersion = isContextES ? shaderVersionES : shaderVersionGL;

		for (int i = 0; i < 8; ++i)
		{
			fragmentShader[i] = gl.createShader(GL_FRAGMENT_SHADER);
			program[i]		  = gl.createProgram();
		}

		const char* vSources[] = { shaderVersion, vShader };
		const int   vLengths[] = { int(strlen(shaderVersion)), int(strlen(vShader)) };
		vertexShader.setSources(2, vSources, vLengths);

		//Compilation test
		for (int i = 0; i < 8; ++i)
		{
			const char* fSources[] = { shaderVersion, fShader };
			const int   fLengths[] = { int(strlen(shaderVersion)), int(strlen(fShader)) };
			gl.shaderSource(fragmentShader[i], 2, fSources, fLengths);
		}

		gl.compileShader(vertexShader.getShader());
		GLU_EXPECT_NO_ERROR(gl.getError(), "compileShader");
		for (int i = 0; i < 8; ++i)
		{
			gl.compileShader(fragmentShader[i]);
			GLU_EXPECT_NO_ERROR(gl.getError(), "compileShader");
		}

		{
			int		 completion  = 0;
			deUint64 shLoopStart = deGetMicroseconds();
			while (completion != 8 && deGetMicroseconds() < shLoopStart + 1000000)
			{
				completion = 0;
				for (int i = 0; i < 8; ++i)
				{
					gl.getShaderiv(fragmentShader[i], GL_COMPLETION_STATUS_KHR, &completionStatus);
					GLU_EXPECT_NO_ERROR(gl.getError(), "getShaderiv");
					if (completionStatus)
						completion++;
				}
			}
			if (completion != 8)
			{
				m_testCtx.setTestResult(QP_TEST_RESULT_FAIL,
										"Failed reading completion status for parallel shader compiling");
				for (int i = 0; i < 8; ++i)
				{
					gl.deleteProgram(program[i]);
					gl.deleteShader(fragmentShader[i]);
				}
				return STOP;
			}
		}

		for (int i = 0; i < 8; ++i)
		{
			gl.attachShader(program[i], vertexShader.getShader());
			GLU_EXPECT_NO_ERROR(gl.getError(), "attachShader");
			gl.attachShader(program[i], fragmentShader[i]);
			GLU_EXPECT_NO_ERROR(gl.getError(), "attachShader");
		}

		//Linking test
		for (int i = 0; i < 8; ++i)
		{
			gl.linkProgram(program[i]);
			GLU_EXPECT_NO_ERROR(gl.getError(), "linkProgram");
		}

		{
			int		 completion  = 0;
			deUint64 prLoopStart = deGetMicroseconds();
			while (completion != 8 && deGetMicroseconds() < prLoopStart + 1000000)
			{
				completion = 0;
				for (int i = 0; i < 8; ++i)
				{
					gl.getProgramiv(program[i], GL_COMPLETION_STATUS_KHR, &completionStatus);
					GLU_EXPECT_NO_ERROR(gl.getError(), "getProgramiv");
					if (completionStatus)
						completion++;
				}
			}
			if (completion != 8)
			{
				m_testCtx.setTestResult(QP_TEST_RESULT_FAIL,
										"Failed reading completion status for parallel program linking");
				for (int i = 0; i < 8; ++i)
				{
					gl.deleteProgram(program[i]);
					gl.deleteShader(fragmentShader[i]);
				}
				return STOP;
			}
		}
	}

	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	return STOP;
}

/** Constructor.
 *
 *  @param context Rendering context.
 */
ParallelShaderCompileTests::ParallelShaderCompileTests(deqp::Context& context)
	: TestCaseGroup(context, "parallel_shader_compile",
					"Verify conformance of KHR_parallel_shader_compile implementation")
{
}

/** Initializes the test group contents. */
void ParallelShaderCompileTests::init()
{
	addChild(new SimpleQueriesTest(m_context));
	addChild(new MaxShaderCompileThreadsTest(m_context));
	addChild(new CompilationCompletionNonParallelTest(m_context));
	addChild(new CompilationCompletionParallelTest(m_context));
}

} /* glcts namespace */
