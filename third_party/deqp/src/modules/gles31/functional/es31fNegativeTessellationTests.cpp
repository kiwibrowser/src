/*-------------------------------------------------------------------------
 * drawElements Quality Program OpenGL ES 3.1 Module
 * -------------------------------------------------
 *
 * Copyright 2016 The Android Open Source Project
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
 * \brief Negative Tessellation tests.
 *//*--------------------------------------------------------------------*/

#include "es31fNegativeTessellationTests.hpp"
#include "gluContextInfo.hpp"
#include "gluShaderProgram.hpp"
#include "glwDefs.hpp"
#include "glwEnums.hpp"
#include "tcuStringTemplate.hpp"

namespace deqp
{

using std::string;
using std::map;

namespace gles31
{
namespace Functional
{
namespace NegativeTestShared
{

using tcu::TestLog;
using namespace glw;

static const char* vertexShaderSource		=	"${GLSL_VERSION_STRING}\n"
												"\n"
												"void main (void)\n"
												"{\n"
												"	gl_Position = vec4(0.0);\n"
												"}\n";

static const char* fragmentShaderSource		=	"${GLSL_VERSION_STRING}\n"
												"precision mediump float;\n"
												"layout(location = 0) out mediump vec4 fragColor;\n"
												"\n"
												"void main (void)\n"
												"{\n"
												"	fragColor = vec4(1.0);\n"
												"}\n";

static const char* tessControlShaderSource	=	"${GLSL_VERSION_STRING}\n"
												"${GLSL_TESS_EXTENSION_STRING}\n"
												"layout (vertices=3) out;\n"
												"\n"
												"void main()\n"
												"{\n"
												"	gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;\n"
												"}\n";

static const char* tessEvalShaderSource		=	"${GLSL_VERSION_STRING}\n"
												"${GLSL_TESS_EXTENSION_STRING}\n"
												"layout(triangles) in;\n"
												"\n"
												"void main()\n"
												"{\n"
												"	gl_Position = gl_TessCoord[0] * gl_in[0].gl_Position;\n"
												"}\n";

static void checkExtensionSupport (NegativeTestContext& ctx, const char* extName)
{
	if (!ctx.getContextInfo().isExtensionSupported(extName))
		throw tcu::NotSupportedError(string(extName) + " not supported");
}

static void checkTessellationSupport (NegativeTestContext& ctx)
{
	checkExtensionSupport(ctx, "GL_EXT_tessellation_shader");
}

// Helper for constructing tessellation pipeline sources.
static glu::ProgramSources makeTessPipelineSources (const std::string& vertexSrc, const std::string& fragmentSrc, const std::string& tessCtrlSrc, const std::string& tessEvalSrc)
{
	glu::ProgramSources sources;
	sources.sources[glu::SHADERTYPE_VERTEX].push_back(vertexSrc);
	sources.sources[glu::SHADERTYPE_FRAGMENT].push_back(fragmentSrc);

	if (!tessCtrlSrc.empty())
		sources.sources[glu::SHADERTYPE_TESSELLATION_CONTROL].push_back(tessCtrlSrc);

	if (!tessEvalSrc.empty())
		sources.sources[glu::SHADERTYPE_TESSELLATION_EVALUATION].push_back(tessEvalSrc);

	return sources;
}

// Incomplete active tess shaders
void single_tessellation_stage (NegativeTestContext& ctx)
{
	const bool					isES32	= glu::contextSupports(ctx.getRenderContext().getType(), glu::ApiType::es(3, 2));
	const bool					requireTES = !ctx.getContextInfo().isExtensionSupported("GL_NV_gpu_shader5");
	map<string, string>			args;
	args["GLSL_VERSION_STRING"]			= isES32 ? getGLSLVersionDeclaration(glu::GLSL_VERSION_320_ES) : getGLSLVersionDeclaration(glu::GLSL_VERSION_310_ES);
	args["GLSL_TESS_EXTENSION_STRING"]	= isES32 ? "" : "#extension GL_EXT_tessellation_shader : require";

	checkTessellationSupport(ctx);

	{
		glu::ShaderProgram program(ctx.getRenderContext(),
								   makeTessPipelineSources(tcu::StringTemplate(vertexShaderSource).specialize(args),
														   tcu::StringTemplate(fragmentShaderSource).specialize(args),
														   tcu::StringTemplate(tessControlShaderSource).specialize(args),
														   "")); // missing tessEvalShaderSource
		tcu::TestLog& log = ctx.getLog();
		log << program;

		ctx.beginSection("A link error is generated if a non-separable program has a tessellation control shader but no tessellation evaluation shader, unless GL_NV_gpu_shader5 is supported.");

		if (requireTES && program.isOk())
			ctx.fail("Program was not expected to link");
		else if (!requireTES && !program.isOk())
			ctx.fail("Program was expected to link");

		ctx.endSection();
	}

	{
		glu::ShaderProgram program(ctx.getRenderContext(),
								   makeTessPipelineSources(tcu::StringTemplate(vertexShaderSource).specialize(args),
														   tcu::StringTemplate(fragmentShaderSource).specialize(args),
														   tcu::StringTemplate(tessControlShaderSource).specialize(args),
														   "") // missing tessEvalShaderSource
								   << glu::ProgramSeparable(true));
		tcu::TestLog& log = ctx.getLog();
		log << program;

		if (!program.isOk())
			TCU_THROW(TestError, "failed to build program");

		ctx.glUseProgram(program.getProgram());
		ctx.expectError(GL_NO_ERROR);

		ctx.beginSection("GL_INVALID_OPERATION is generated if current program state has tessellation control shader but no tessellation evaluation shader, unless GL_NV_gpu_shader5 is supported.");
		ctx.glDrawArrays(GL_PATCHES, 0, 3);
		ctx.expectError(requireTES ? GL_INVALID_OPERATION : GL_NO_ERROR);
		ctx.endSection();

		ctx.glUseProgram(0);
	}

	{
		glu::ShaderProgram program(ctx.getRenderContext(),
								   makeTessPipelineSources(tcu::StringTemplate(vertexShaderSource).specialize(args),
														   tcu::StringTemplate(fragmentShaderSource).specialize(args),
														   "", // missing tessControlShaderSource
														   tcu::StringTemplate(tessEvalShaderSource).specialize(args)));
		tcu::TestLog& log = ctx.getLog();
		log << program;

		ctx.beginSection("A link error is generated if a non-separable program has a tessellation evaluation shader but no tessellation control shader.");

		if (program.isOk())
			ctx.fail("Program was not expected to link");

		ctx.endSection();
	}

	{
		glu::ShaderProgram program(ctx.getRenderContext(),
								   makeTessPipelineSources(tcu::StringTemplate(vertexShaderSource).specialize(args),
														   tcu::StringTemplate(fragmentShaderSource).specialize(args),
														   "", // missing tessControlShaderSource
														   tcu::StringTemplate(tessEvalShaderSource).specialize(args))
									<< glu::ProgramSeparable(true));
		tcu::TestLog& log = ctx.getLog();
		log << program;

		if (!program.isOk())
			TCU_THROW(TestError, "failed to build program");

		ctx.glUseProgram(program.getProgram());
		ctx.expectError(GL_NO_ERROR);

		ctx.beginSection("GL_INVALID_OPERATION is generated if current program state has tessellation evaluation shader but no tessellation control shader.");
		ctx.glDrawArrays(GL_PATCHES, 0, 3);
		ctx.expectError(GL_INVALID_OPERATION);
		ctx.endSection();

		ctx.glUseProgram(0);
	}
}

// Complete active tess shaders invalid primitive mode
void invalid_primitive_mode (NegativeTestContext& ctx)
{
	checkTessellationSupport(ctx);

	const bool					isES32	= glu::contextSupports(ctx.getRenderContext().getType(), glu::ApiType::es(3, 2));
	map<string, string>			args;
	args["GLSL_VERSION_STRING"]			= isES32 ? getGLSLVersionDeclaration(glu::GLSL_VERSION_320_ES) : getGLSLVersionDeclaration(glu::GLSL_VERSION_310_ES);
	args["GLSL_TESS_EXTENSION_STRING"]	= isES32 ? "" : "#extension GL_EXT_tessellation_shader : require";

	glu::ShaderProgram program(ctx.getRenderContext(),
							   makeTessPipelineSources(tcu::StringTemplate(vertexShaderSource).specialize(args),
													   tcu::StringTemplate(fragmentShaderSource).specialize(args),
													   tcu::StringTemplate(tessControlShaderSource).specialize(args),
													   tcu::StringTemplate(tessEvalShaderSource).specialize(args)));
	tcu::TestLog& log = ctx.getLog();
	log << program;

	ctx.glUseProgram(program.getProgram());
	ctx.expectError(GL_NO_ERROR);

	ctx.beginSection("GL_INVALID_OPERATION is generated if tessellation is active and primitive mode is not GL_PATCHES.");
	ctx.glDrawArrays(GL_TRIANGLES, 0, 3);
	ctx.expectError(GL_INVALID_OPERATION);
	ctx.endSection();

	ctx.glUseProgram(0);
}

void tessellation_not_active (NegativeTestContext& ctx)
{
	checkTessellationSupport(ctx);

	const bool					isES32	= glu::contextSupports(ctx.getRenderContext().getType(), glu::ApiType::es(3, 2));
	const glw::GLenum			tessErr = ctx.getContextInfo().isExtensionSupported("GL_NV_gpu_shader5") ? GL_NO_ERROR : GL_INVALID_OPERATION;
	map<string, string>			args;
	args["GLSL_VERSION_STRING"]			= isES32 ? getGLSLVersionDeclaration(glu::GLSL_VERSION_320_ES) : getGLSLVersionDeclaration(glu::GLSL_VERSION_310_ES);
	args["GLSL_TESS_EXTENSION_STRING"]	= isES32 ? "" : "#extension GL_EXT_tessellation_shader : require";

	glu::ShaderProgram program(ctx.getRenderContext(),
							   makeTessPipelineSources(tcu::StringTemplate(vertexShaderSource).specialize(args),
													   tcu::StringTemplate(fragmentShaderSource).specialize(args),
													   "",		// missing tessControlShaderSource
													   ""));	// missing tessEvalShaderSource
	tcu::TestLog& log = ctx.getLog();
	log << program;

	ctx.glUseProgram(program.getProgram());
	ctx.expectError(GL_NO_ERROR);

	ctx.beginSection("GL_INVALID_OPERATION is generated if tessellation is not active and primitive mode is GL_PATCHES, unless GL_NV_gpu_shader5 is supported.");
	ctx.glDrawArrays(GL_PATCHES, 0, 3);
	ctx.expectError(tessErr);
	ctx.endSection();

	ctx.glUseProgram(0);
}

void invalid_program_state (NegativeTestContext& ctx)
{
	checkTessellationSupport(ctx);

	const bool					isES32	= glu::contextSupports(ctx.getRenderContext().getType(), glu::ApiType::es(3, 2));
	map<string, string>			args;
	args["GLSL_VERSION_STRING"]			= isES32 ? getGLSLVersionDeclaration(glu::GLSL_VERSION_320_ES) : getGLSLVersionDeclaration(glu::GLSL_VERSION_310_ES);
	args["GLSL_TESS_EXTENSION_STRING"]	= isES32 ? "" : "#extension GL_EXT_tessellation_shader : require";

	glu::FragmentSource frgSource(tcu::StringTemplate(fragmentShaderSource).specialize(args));
	glu::TessellationControlSource tessCtrlSource(tcu::StringTemplate(tessControlShaderSource).specialize(args));
	glu::TessellationEvaluationSource tessEvalSource(tcu::StringTemplate(tessEvalShaderSource).specialize(args));

	glu::ProgramPipeline pipeline(ctx.getRenderContext());

	glu::ShaderProgram	fragProgram	(ctx.getRenderContext(), glu::ProgramSources() << glu::ProgramSeparable(true) << frgSource);
	glu::ShaderProgram	tessCtrlProgram	(ctx.getRenderContext(), glu::ProgramSources() << glu::ProgramSeparable(true) << tessCtrlSource);
	glu::ShaderProgram	tessEvalProgram	(ctx.getRenderContext(), glu::ProgramSources() << glu::ProgramSeparable(true) << tessEvalSource);

	tcu::TestLog& log = ctx.getLog();
	log << fragProgram << tessCtrlProgram << tessEvalProgram;

	if (!fragProgram.isOk() || !tessCtrlProgram.isOk() || !tessEvalProgram.isOk())
		throw tcu::TestError("failed to build program");

	ctx.glBindProgramPipeline(pipeline.getPipeline());
	ctx.expectError(GL_NO_ERROR);

	ctx.glUseProgramStages(pipeline.getPipeline(), GL_FRAGMENT_SHADER_BIT, fragProgram.getProgram());
	ctx.glUseProgramStages(pipeline.getPipeline(), GL_TESS_CONTROL_SHADER_BIT, tessCtrlProgram.getProgram());
	ctx.glUseProgramStages(pipeline.getPipeline(), GL_TESS_EVALUATION_SHADER_BIT, tessEvalProgram.getProgram());
	ctx.expectError(GL_NO_ERROR);

	ctx.beginSection("GL_INVALID_OPERATION is generated if tessellation is active and vertex shader is missing.");
	ctx.glDrawArrays(GL_PATCHES, 0, 3);
	ctx.expectError(GL_INVALID_OPERATION);
	ctx.endSection();

	ctx.glBindProgramPipeline(0);
	ctx.expectError(GL_NO_ERROR);
}

void tessellation_control_invalid_vertex_count (NegativeTestContext& ctx)
{
	checkTessellationSupport(ctx);

	const char* const tessControlVertLimitSource	=	"${GLSL_VERSION_STRING}\n"
														"${GLSL_TESS_EXTENSION_STRING}\n"
														"layout (vertices=${GL_MAX_PATCH_LIMIT}) out;\n"
														"void main()\n"
														"{\n"
														"	gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;\n"
														"}\n";

	const bool					isES32	= glu::contextSupports(ctx.getRenderContext().getType(), glu::ApiType::es(3, 2));
	map<string, string>			args;
	args["GLSL_VERSION_STRING"]			= isES32 ? getGLSLVersionDeclaration(glu::GLSL_VERSION_320_ES) : getGLSLVersionDeclaration(glu::GLSL_VERSION_310_ES);
	args["GLSL_TESS_EXTENSION_STRING"]	= isES32 ? "" : "#extension GL_EXT_tessellation_shader : require";

	int maxPatchVertices= 0;

	ctx.beginSection("Output vertex count exceeds GL_MAX_PATCH_VERTICES.");
	ctx.glGetIntegerv(GL_MAX_PATCH_VERTICES, &maxPatchVertices);
	ctx.expectError(GL_NO_ERROR);

	std::ostringstream				oss;
	oss << (maxPatchVertices + 1);
	args["GL_MAX_PATCH_LIMIT"] =	oss.str();


	glu::ShaderProgram program(ctx.getRenderContext(),
							   makeTessPipelineSources(tcu::StringTemplate(vertexShaderSource).specialize(args),
													   tcu::StringTemplate(fragmentShaderSource).specialize(args),
													   tcu::StringTemplate(tessControlVertLimitSource).specialize(args),
													   tcu::StringTemplate(tessEvalShaderSource).specialize(args)));
	tcu::TestLog& log = ctx.getLog();
	log << program;

	bool testFailed = program.getProgramInfo().linkOk;

	if (testFailed)
		ctx.fail("Program was not expected to link");

	ctx.endSection();
}

void invalid_get_programiv (NegativeTestContext& ctx)
{
	checkTessellationSupport(ctx);

	GLuint	program		= ctx.glCreateProgram();
	GLint	params[1]	= { 0 };

	ctx.beginSection("GL_INVALID_OPERATION is generated if GL_TESS_CONTROL_OUTPUT_VERTICES is queried for a program which has not been linked properly.");
	ctx.glGetProgramiv(program, GL_TESS_CONTROL_OUTPUT_VERTICES, &params[0]);
	ctx.expectError(GL_INVALID_OPERATION);
	ctx.endSection();

	ctx.beginSection("GL_INVALID_OPERATION is generated if GL_TESS_GEN_MODE is queried for a program which has not been linked properly.");
	ctx.glGetProgramiv(program, GL_TESS_GEN_MODE, &params[0]);
	ctx.expectError(GL_INVALID_OPERATION);
	ctx.endSection();

	ctx.beginSection("GL_INVALID_OPERATION is generated if GL_TESS_GEN_SPACING is queried for a program which has not been linked properly.");
	ctx.glGetProgramiv(program, GL_TESS_GEN_SPACING, &params[0]);
	ctx.expectError(GL_INVALID_OPERATION);
	ctx.endSection();

	ctx.beginSection("GL_INVALID_OPERATION is generated if GL_TESS_GEN_VERTEX_ORDER is queried for a program which has not been linked properly.");
	ctx.glGetProgramiv(program, GL_TESS_GEN_VERTEX_ORDER, &params[0]);
	ctx.expectError(GL_INVALID_OPERATION);
	ctx.endSection();

	ctx.beginSection("GL_INVALID_OPERATION is generated if GL_TESS_GEN_POINT_MODE is queried for a program which has not been linked properly.");
	ctx.glGetProgramiv(program, GL_TESS_GEN_POINT_MODE, &params[0]);
	ctx.expectError(GL_INVALID_OPERATION);
	ctx.endSection();

	ctx.glDeleteProgram(program);
}

void invalid_patch_parameteri (NegativeTestContext& ctx)
{
	checkTessellationSupport(ctx);

	ctx.beginSection("GL_INVALID_ENUM is generated if pname is not GL_PATCH_VERTICES.");
	ctx.glPatchParameteri(-1, 1);
	ctx.expectError(GL_INVALID_ENUM);
	ctx.endSection();

	ctx.beginSection("GL_INVALID_VALUE is generated if value is less than or equal to zero.");
	ctx.glPatchParameteri(GL_PATCH_VERTICES, 0);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();

	int maxPatchVertices= 0;
	ctx.glGetIntegerv(GL_MAX_PATCH_VERTICES, &maxPatchVertices);
	ctx.expectError(GL_NO_ERROR);

	ctx.beginSection("GL_INVALID_VALUE is generated if value is greater than GL_MAX_PATCH_VERTICES.");
	ctx.glPatchParameteri(GL_PATCH_VERTICES, maxPatchVertices + 1);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();
}

std::vector<FunctionContainer> getNegativeTessellationTestFunctions (void)
{
	const FunctionContainer funcs[] =
	{
		{ single_tessellation_stage,					"single_tessellation_stage",					"Invalid program state with single tessellation stage"							},
		{ invalid_primitive_mode,						"invalid_primitive_mode",						"Invalid primitive mode when tessellation is active"							},
		{ tessellation_not_active,						"tessellation_not_active",						"Use of GL_PATCHES when tessellation is not active"								},
		{ invalid_program_state,						"invalid_program_state",						"Invalid program state when tessellation active but no vertex shader present"	},
		{ invalid_get_programiv,						"get_programiv",								"Invalid glGetProgramiv() usage"												},
		{ invalid_patch_parameteri,						"invalid_program_queries",						"Invalid glPatchParameteri() usage"												},
		{ tessellation_control_invalid_vertex_count,	"tessellation_control_invalid_vertex_count",	"Exceed vertex count limit in tessellation control shader"						},
	};

	return std::vector<FunctionContainer>(DE_ARRAY_BEGIN(funcs), DE_ARRAY_END(funcs));
}

} // NegativeTestShared
} // Functional
} // gles31
} // deqp
