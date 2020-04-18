/*-------------------------------------------------------------------------
 * drawElements Quality Program OpenGL ES 3.1 Module
 * -------------------------------------------------
 *
 * Copyright 2015 The Android Open Source Project
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
 * \brief Program Pipeline State Query tests.
 *//*--------------------------------------------------------------------*/

#include "es31fProgramPipelineStateQueryTests.hpp"
#include "es31fInfoLogQueryShared.hpp"
#include "glsStateQueryUtil.hpp"
#include "gluRenderContext.hpp"
#include "gluCallLogWrapper.hpp"
#include "gluObjectWrapper.hpp"
#include "gluShaderProgram.hpp"
#include "glwFunctions.hpp"
#include "glwEnums.hpp"

namespace deqp
{
namespace gles31
{
namespace Functional
{
namespace
{

using namespace gls::StateQueryUtil;

static const char* getVerifierSuffix (QueryType type)
{
	switch (type)
	{
		case QUERY_PIPELINE_INTEGER:	return "get_program_pipelineiv";
		default:
			DE_ASSERT(DE_FALSE);
			return DE_NULL;
	}
}

static const char* const s_vertexSource =		"#version 310 es\n"
												"out highp vec4 v_color;\n"
												"void main()\n"
												"{\n"
												"	gl_Position = vec4(float(gl_VertexID) * 0.5, float(gl_VertexID+1) * 0.5, 0.0, 1.0);\n"
												"	v_color = vec4(float(gl_VertexID), 1.0, 0.0, 1.0);\n"
												"}\n";
static const char* const s_fragmentSource =		"#version 310 es\n"
												"in highp vec4 v_color;\n"
												"layout(location=0) out highp vec4 o_color;\n"
												"void main()\n"
												"{\n"
												"	o_color = v_color;\n"
												"}\n";
static const char* const s_computeSource =		"#version 310 es\n"
												"layout (local_size_x = 1, local_size_y = 1) in;\n"
												"layout(binding = 0) buffer Output\n"
												"{\n"
												"	highp float val;\n"
												"} sb_out;\n"
												"\n"
												"void main (void)\n"
												"{\n"
												"	sb_out.val = 1.0;\n"
												"}\n";

class ActiveProgramCase : public TestCase
{
public:
						ActiveProgramCase	(Context& context, QueryType verifier, const char* name, const char* desc);
	IterateResult		iterate				(void);

private:
	const QueryType		m_verifier;
};

ActiveProgramCase::ActiveProgramCase (Context& context, QueryType verifier, const char* name, const char* desc)
	: TestCase		(context, name, desc)
	, m_verifier	(verifier)
{
}

ActiveProgramCase::IterateResult ActiveProgramCase::iterate (void)
{
	const glu::ShaderProgram	vtxProgram	(m_context.getRenderContext(), glu::ProgramSources() << glu::ProgramSeparable(true) << glu::VertexSource(s_vertexSource));
	const glu::ShaderProgram	frgProgram	(m_context.getRenderContext(), glu::ProgramSources() << glu::ProgramSeparable(true) << glu::FragmentSource(s_fragmentSource));
	const glu::ProgramPipeline	pipeline	(m_context.getRenderContext());
	glu::CallLogWrapper			gl			(m_context.getRenderContext().getFunctions(), m_testCtx.getLog());
	tcu::ResultCollector		result		(m_testCtx.getLog(), " // ERROR: ");

	{
		const tcu::ScopedLogSection section(m_testCtx.getLog(), "VtxProg", "Vertex program");
		m_testCtx.getLog() << vtxProgram;
	}

	{
		const tcu::ScopedLogSection section(m_testCtx.getLog(), "FrgProg", "Fragment program");
		m_testCtx.getLog() << frgProgram;
	}

	if (!vtxProgram.isOk() || !frgProgram.isOk())
		throw tcu::TestError("failed to build program");

	gl.enableLogging(true);
	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

	gl.glBindProgramPipeline(pipeline.getPipeline());
	gl.glUseProgramStages(pipeline.getPipeline(), GL_VERTEX_SHADER_BIT, vtxProgram.getProgram());
	gl.glUseProgramStages(pipeline.getPipeline(), GL_FRAGMENT_SHADER_BIT, frgProgram.getProgram());
	GLU_EXPECT_NO_ERROR(gl.glGetError(), "gen pipeline");
	gl.glBindProgramPipeline(0);
	GLU_EXPECT_NO_ERROR(gl.glGetError(), "unbind pipeline");

	{
		const tcu::ScopedLogSection section(m_testCtx.getLog(), "Initial", "Initial");
		verifyStatePipelineInteger(result, gl, pipeline.getPipeline(), GL_ACTIVE_PROGRAM, 0, m_verifier);
	}

	{
		const tcu::ScopedLogSection section(m_testCtx.getLog(), "Set", "Set");

		gl.glActiveShaderProgram(pipeline.getPipeline(), frgProgram.getProgram());
		GLU_EXPECT_NO_ERROR(gl.glGetError(), "gen pipeline");
		verifyStatePipelineInteger(result, gl, pipeline.getPipeline(), GL_ACTIVE_PROGRAM, (int)frgProgram.getProgram(), m_verifier);
	}

	result.setTestContextResult(m_testCtx);
	return STOP;
}

class PipelineProgramCase : public TestCase
{
public:
						PipelineProgramCase	(Context& context, QueryType verifier, const char* name, const char* desc, glw::GLenum stage);
	IterateResult		iterate				(void);

private:
	const QueryType		m_verifier;
	const glw::GLenum	m_targetStage;
};

PipelineProgramCase::PipelineProgramCase (Context& context, QueryType verifier, const char* name, const char* desc, glw::GLenum stage)
	: TestCase		(context, name, desc)
	, m_verifier	(verifier)
	, m_targetStage	(stage)
{
}

PipelineProgramCase::IterateResult PipelineProgramCase::iterate (void)
{
	glu::CallLogWrapper		gl			(m_context.getRenderContext().getFunctions(), m_testCtx.getLog());
	tcu::ResultCollector	result		(m_testCtx.getLog(), " // ERROR: ");
	const int				stageBit	= (m_targetStage == GL_VERTEX_SHADER)	? (GL_VERTEX_SHADER_BIT)
										: (m_targetStage == GL_FRAGMENT_SHADER)	? (GL_FRAGMENT_SHADER_BIT)
										: (GL_COMPUTE_SHADER_BIT);
	glu::ProgramSources	sources;

	if (m_targetStage == GL_VERTEX_SHADER)
		sources << glu::ProgramSeparable(true) << glu::VertexSource(s_vertexSource);
	else if (m_targetStage == GL_FRAGMENT_SHADER)
		sources << glu::ProgramSeparable(true) << glu::FragmentSource(s_fragmentSource);
	else if (m_targetStage == GL_COMPUTE_SHADER)
		sources << glu::ProgramSeparable(true) << glu::ComputeSource(s_computeSource);
	else
		DE_ASSERT(false);

	gl.enableLogging(true);

	{
		glu::ShaderProgram program(m_context.getRenderContext(), sources);

		{
			const tcu::ScopedLogSection section(m_testCtx.getLog(), "program", "Program");
			m_testCtx.getLog() << program;
		}

		if (!program.isOk())
			throw tcu::TestError("failed to build program");

		{
			const tcu::ScopedLogSection section		(m_testCtx.getLog(), "Initial", "Initial");
			glu::ProgramPipeline		pipeline	(m_context.getRenderContext());

			gl.glBindProgramPipeline(pipeline.getPipeline());
			GLU_EXPECT_NO_ERROR(gl.glGetError(), "setup pipeline");

			verifyStatePipelineInteger(result, gl, pipeline.getPipeline(), m_targetStage, 0, m_verifier);
		}

		{
			const tcu::ScopedLogSection section		(m_testCtx.getLog(), "Set", "Set");
			glu::ProgramPipeline		pipeline	(m_context.getRenderContext());

			gl.glBindProgramPipeline(pipeline.getPipeline());
			gl.glUseProgramStages(pipeline.getPipeline(), stageBit, program.getProgram());
			GLU_EXPECT_NO_ERROR(gl.glGetError(), "setup pipeline");

			verifyStatePipelineInteger(result, gl, pipeline.getPipeline(), m_targetStage, program.getProgram(), m_verifier);
		}
	}

	result.setTestContextResult(m_testCtx);
	return STOP;
}

class ValidateStatusCase : public TestCase
{
public:
						ValidateStatusCase	(Context& context, QueryType verifier, const char* name, const char* desc);
	IterateResult		iterate				(void);

private:
	const QueryType		m_verifier;
};

ValidateStatusCase::ValidateStatusCase (Context& context, QueryType verifier, const char* name, const char* desc)
	: TestCase		(context, name, desc)
	, m_verifier	(verifier)
{
}

ValidateStatusCase::IterateResult ValidateStatusCase::iterate (void)
{
	glu::ShaderProgram		vtxProgram	(m_context.getRenderContext(), glu::ProgramSources() << glu::ProgramSeparable(true) << glu::VertexSource(s_vertexSource));
	glu::ShaderProgram		frgProgram	(m_context.getRenderContext(), glu::ProgramSources() << glu::ProgramSeparable(true) << glu::FragmentSource(s_fragmentSource));
	glu::ProgramPipeline	pipeline	(m_context.getRenderContext());
	glu::CallLogWrapper		gl			(m_context.getRenderContext().getFunctions(), m_testCtx.getLog());
	tcu::ResultCollector	result		(m_testCtx.getLog(), " // ERROR: ");

	{
		const tcu::ScopedLogSection section(m_testCtx.getLog(), "VtxProg", "Vertex program");
		m_testCtx.getLog() << vtxProgram;
	}

	{
		const tcu::ScopedLogSection section(m_testCtx.getLog(), "FrgProg", "Fragment program");
		m_testCtx.getLog() << frgProgram;
	}

	if (!vtxProgram.isOk() || !frgProgram.isOk())
		throw tcu::TestError("failed to build program");

	gl.enableLogging(true);

	gl.glBindProgramPipeline(pipeline.getPipeline());
	gl.glUseProgramStages(pipeline.getPipeline(), GL_VERTEX_SHADER_BIT, vtxProgram.getProgram());
	gl.glUseProgramStages(pipeline.getPipeline(), GL_FRAGMENT_SHADER_BIT, frgProgram.getProgram());
	GLU_EXPECT_NO_ERROR(gl.glGetError(), "gen pipeline");
	gl.glBindProgramPipeline(0);
	GLU_EXPECT_NO_ERROR(gl.glGetError(), "unbind pipeline");

	{
		const tcu::ScopedLogSection section(m_testCtx.getLog(), "Initial", "Initial");
		verifyStatePipelineInteger(result, gl, pipeline.getPipeline(), GL_VALIDATE_STATUS, 0, m_verifier);
	}

	{
		const tcu::ScopedLogSection section(m_testCtx.getLog(), "Set", "Validate");

		gl.glValidateProgramPipeline(pipeline.getPipeline());
		GLU_EXPECT_NO_ERROR(gl.glGetError(), "gen pipeline");
		verifyStatePipelineInteger(result, gl, pipeline.getPipeline(), GL_VALIDATE_STATUS, GL_TRUE, m_verifier);
	}

	result.setTestContextResult(m_testCtx);
	return STOP;
}

class InfoLogCase : public TestCase
{
public:
						InfoLogCase	(Context& context, const char* name, const char* desc);
	IterateResult		iterate		(void);
};

InfoLogCase::InfoLogCase (Context& context, const char* name, const char* desc)
	: TestCase(context, name, desc)
{
}

InfoLogCase::IterateResult InfoLogCase::iterate (void)
{
	using gls::StateQueryUtil::StateQueryMemoryWriteGuard;

	static const char* const s_incompatibleFragmentSource = "#version 310 es\n"
															"in mediump vec2 v_colorB;\n"
															"in mediump vec2 v_colorC;\n"
															"layout(location=0) out highp vec4 o_color;\n"
															"void main()\n"
															"{\n"
															"	o_color = v_colorB.xxyy + v_colorC.yyxy;\n"
															"}\n";

	glu::ShaderProgram		vtxProgram	(m_context.getRenderContext(), glu::ProgramSources() << glu::ProgramSeparable(true) << glu::VertexSource(s_vertexSource));
	glu::ShaderProgram		frgProgram	(m_context.getRenderContext(), glu::ProgramSources() << glu::ProgramSeparable(true) << glu::FragmentSource(s_incompatibleFragmentSource));
	glu::CallLogWrapper		gl			(m_context.getRenderContext().getFunctions(), m_testCtx.getLog());
	tcu::ResultCollector	result		(m_testCtx.getLog(), " // ERROR: ");

	{
		const tcu::ScopedLogSection section(m_testCtx.getLog(), "VtxProg", "Vertex program");
		m_testCtx.getLog() << vtxProgram;
	}

	{
		const tcu::ScopedLogSection section(m_testCtx.getLog(), "FrgProg", "Fragment program");
		m_testCtx.getLog() << frgProgram;
	}

	if (!vtxProgram.isOk() || !frgProgram.isOk())
		throw tcu::TestError("failed to build program");

	gl.enableLogging(true);

	{
		const tcu::ScopedLogSection section		(m_testCtx.getLog(), "Initial", "Initial");
		glu::ProgramPipeline		pipeline	(m_context.getRenderContext());
		std::string					buf			(3, 'X');
		int							written		= -1;

		verifyStatePipelineInteger(result, gl, pipeline.getPipeline(), GL_INFO_LOG_LENGTH, 0, QUERY_PIPELINE_INTEGER);

		gl.glGetProgramPipelineInfoLog(pipeline.getPipeline(), 2, &written, &buf[0]);
		GLU_EXPECT_NO_ERROR(gl.glGetError(), "query log");

		if (written == -1)
			result.fail("'length' was not written to");
		else if (written != 0)
			result.fail("'length' was not 0");
		else if (buf[0] != '\0')
			result.fail("log was not 0-sized null-terminated string");
		else if (buf[1] != 'X' || buf[2] != 'X')
			result.fail("buffer after returned length modified");
	}

	{
		const tcu::ScopedLogSection				superSection	(m_testCtx.getLog(), "ValidationFail", "Failed validation");
		glu::ProgramPipeline					pipeline		(m_context.getRenderContext());
		StateQueryMemoryWriteGuard<glw::GLint>	logLen;

		gl.glBindProgramPipeline(pipeline.getPipeline());
		gl.glUseProgramStages(pipeline.getPipeline(), GL_VERTEX_SHADER_BIT, vtxProgram.getProgram());
		gl.glUseProgramStages(pipeline.getPipeline(), GL_FRAGMENT_SHADER_BIT, frgProgram.getProgram());
		GLU_EXPECT_NO_ERROR(gl.glGetError(), "gen pipeline");

		gl.glBindProgramPipeline(0);
		GLU_EXPECT_NO_ERROR(gl.glGetError(), "unbind pipeline");
		gl.glValidateProgramPipeline(pipeline.getPipeline());
		GLU_EXPECT_NO_ERROR(gl.glGetError(), "gen pipeline");

		gl.glGetProgramPipelineiv(pipeline.getPipeline(), GL_INFO_LOG_LENGTH, &logLen);
		GLU_EXPECT_NO_ERROR(gl.glGetError(), "get INFO_LOG_LENGTH");

		if (logLen.verifyValidity(result))
			verifyInfoLogQuery(result, gl, logLen, pipeline.getPipeline(), &glu::CallLogWrapper::glGetProgramPipelineInfoLog, "glGetProgramPipelineInfoLog");
	}

	result.setTestContextResult(m_testCtx);
	return STOP;
}

} // anonymous

ProgramPipelineStateQueryTests::ProgramPipelineStateQueryTests (Context& context)
	: TestCaseGroup(context, "program_pipeline", "Program Pipeline State Query tests")
{
}

ProgramPipelineStateQueryTests::~ProgramPipelineStateQueryTests (void)
{
}

void ProgramPipelineStateQueryTests::init (void)
{
	static const QueryType verifiers[] =
	{
		QUERY_PIPELINE_INTEGER,
	};

#define FOR_EACH_VERIFIER(X) \
	for (int verifierNdx = 0; verifierNdx < DE_LENGTH_OF_ARRAY(verifiers); ++verifierNdx)	\
	{																						\
		const char* verifierSuffix = getVerifierSuffix(verifiers[verifierNdx]);				\
		const QueryType verifier = verifiers[verifierNdx];									\
		this->addChild(X);																	\
	}

	FOR_EACH_VERIFIER(new ActiveProgramCase(m_context, verifier, (std::string("active_program_") + verifierSuffix).c_str(), "Test ACTIVE_PROGRAM"));
	FOR_EACH_VERIFIER(new PipelineProgramCase(m_context, verifier, (std::string("vertex_shader_") + verifierSuffix).c_str(), "Test VERTEX_SHADER", GL_VERTEX_SHADER));
	FOR_EACH_VERIFIER(new PipelineProgramCase(m_context, verifier, (std::string("fragment_shader_") + verifierSuffix).c_str(), "Test FRAGMENT_SHADER", GL_FRAGMENT_SHADER));
	FOR_EACH_VERIFIER(new PipelineProgramCase(m_context, verifier, (std::string("compute_shader_") + verifierSuffix).c_str(), "Test COMPUTE_SHADER", GL_COMPUTE_SHADER));
	FOR_EACH_VERIFIER(new ValidateStatusCase(m_context, verifier, (std::string("validate_status_") + verifierSuffix).c_str(), "Test VALIDATE_STATUS"));

#undef FOR_EACH_VERIFIER

	this->addChild(new InfoLogCase(m_context, "info_log", "Test info log"));
}

} // Functional
} // gles31
} // deqp
