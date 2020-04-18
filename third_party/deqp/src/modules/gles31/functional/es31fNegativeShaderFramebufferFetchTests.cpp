/*-------------------------------------------------------------------------
 * drawElements Quality Program OpenGL ES 3.1 Module
 * -------------------------------------------------
 *
 * Copyright 2017 The Android Open Source Project
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
 * \brief Negative ShaderFramebufferFetch tests.
 *//*--------------------------------------------------------------------*/

#include "es31fNegativeShaderFramebufferFetchTests.hpp"
#include "gluContextInfo.hpp"
#include "gluShaderProgram.hpp"
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
namespace
{

static const char* vertexShaderSource		=	"${GLSL_VERSION_STRING}\n"
												"\n"
												"void main (void)\n"
												"{\n"
												"	gl_Position = vec4(0.0);\n"
												"}\n";

static const char* fragmentShaderSource		=	"${GLSL_VERSION_STRING}\n"
												"layout(location = 0) out mediump vec4 fragColor;\n"
												"\n"
												"void main (void)\n"
												"{\n"
												"	fragColor = vec4(1.0);\n"
												"}\n";

static void checkExtensionSupport (NegativeTestContext& ctx, const char* extName)
{
	if (!ctx.getContextInfo().isExtensionSupported(extName))
		throw tcu::NotSupportedError(string(extName) + " not supported");
}

static void checkFramebufferFetchSupport (NegativeTestContext& ctx)
{
	checkExtensionSupport(ctx, "GL_EXT_shader_framebuffer_fetch");
}

enum ProgramError
{
	PROGRAM_ERROR_LINK = 0,
	PROGRAM_ERROR_COMPILE,
	PROGRAM_ERROR_COMPILE_OR_LINK,
};

void verifyProgramError (NegativeTestContext& ctx, const glu::ShaderProgram& program,  ProgramError error, glu::ShaderType shaderType)
{
	bool	testFailed = false;
	string	message;

	ctx.getLog() << program;

	switch (error)
	{
		case PROGRAM_ERROR_LINK:
		{
			message = "Program was not expected to link.";
			testFailed = (program.getProgramInfo().linkOk);
			break;
		}
		case PROGRAM_ERROR_COMPILE:
		{
			message = "Program was not expected to compile.";
			testFailed = program.getShaderInfo(shaderType).compileOk;
			break;
		}
		case PROGRAM_ERROR_COMPILE_OR_LINK:
		{
			message = "Program was not expected to compile or link.";
			testFailed = (program.getProgramInfo().linkOk) && (program.getShaderInfo(shaderType).compileOk);
			break;
		}
		default:
		{
			DE_FATAL("Invalid program error type");
			break;
		}
	}

	if (testFailed)
	{
		ctx.getLog() << tcu::TestLog::Message << message << tcu::TestLog::EndMessage;
		ctx.fail(message);
	}
}

void last_frag_data_not_defined (NegativeTestContext& ctx)
{
	checkFramebufferFetchSupport(ctx);

	const bool					isES32	= glu::contextSupports(ctx.getRenderContext().getType(), glu::ApiType::es(3, 2));
	map<string, string>			args;
	args["GLSL_VERSION_STRING"]			= isES32 ? getGLSLVersionDeclaration(glu::GLSL_VERSION_320_ES) : getGLSLVersionDeclaration(glu::GLSL_VERSION_310_ES);

	const char* const fragShaderSource	=	"${GLSL_VERSION_STRING}\n"
											"#extension GL_EXT_shader_framebuffer_fetch : require\n"
											"layout(location = 0) out mediump vec4 fragColor;\n"
											"\n"
											"void main (void)\n"
											"{\n"
											"	fragColor = gl_LastFragData[0];\n"
											"}\n";

	glu::ShaderProgram program(ctx.getRenderContext(), glu::ProgramSources()
			<< glu::VertexSource(tcu::StringTemplate(vertexShaderSource).specialize(args))
			<< glu::FragmentSource(tcu::StringTemplate(fragShaderSource).specialize(args)));

	ctx.beginSection("A link error is generated if the built-in fragment outputs of ES 2.0 are used in #version 300 es shaders");
	verifyProgramError(ctx, program, PROGRAM_ERROR_LINK, glu::SHADERTYPE_FRAGMENT);
	ctx.endSection();
}

void last_frag_data_readonly (NegativeTestContext& ctx)
{
	checkFramebufferFetchSupport(ctx);

	map<string, string>			args;
	args["GLSL_VERSION_STRING"]			=	getGLSLVersionDeclaration(glu::GLSL_VERSION_100_ES);

	const char* const fragShaderSource	=	"${GLSL_VERSION_STRING}\n"
											"#extension GL_EXT_shader_framebuffer_fetch : require\n"
											"\n"
											"void main (void)\n"
											"{\n"
											"	gl_LastFragData[0] = vec4(1.0);\n"
											"	gl_FragColor = gl_LastFragData[0];\n"
											"}\n";

	glu::ShaderProgram program(ctx.getRenderContext(), glu::ProgramSources()
			<< glu::VertexSource(tcu::StringTemplate(vertexShaderSource).specialize(args))
			<< glu::FragmentSource(tcu::StringTemplate(fragShaderSource).specialize(args)));

	ctx.beginSection("A compile-time or link error is generated if the built-in fragment outputs of ES 2.0 are written to.");
	verifyProgramError(ctx, program, PROGRAM_ERROR_COMPILE_OR_LINK, glu::SHADERTYPE_FRAGMENT);
	ctx.endSection();
}

void invalid_inout_version (NegativeTestContext& ctx)
{
	checkFramebufferFetchSupport(ctx);

	map<string, string>			args;
	args["GLSL_VERSION_STRING"]			=	getGLSLVersionDeclaration(glu::GLSL_VERSION_100_ES);

	const char* const fragShaderSource	=	"${GLSL_VERSION_STRING}\n"
											"#extension GL_EXT_shader_framebuffer_fetch : require\n"
											"inout highp vec4 fragColor;\n"
											"\n"
											"void main (void)\n"
											"{\n"
											"	highp float product = dot(vec3(0.5), fragColor.rgb);\n"
											"	gl_FragColor = vec4(product);\n"
											"}\n";

	glu::ShaderProgram program(ctx.getRenderContext(), glu::ProgramSources()
			<< glu::VertexSource(tcu::StringTemplate(vertexShaderSource).specialize(args))
			<< glu::FragmentSource(tcu::StringTemplate(fragShaderSource).specialize(args)));

	ctx.beginSection("A compile-time or link error is generated if user-defined inout arrays are used in earlier versions of GLSL before ES 3.0");
	verifyProgramError(ctx, program, PROGRAM_ERROR_COMPILE_OR_LINK, glu::SHADERTYPE_FRAGMENT);
	ctx.endSection();
}

void invalid_redeclaration_inout (NegativeTestContext& ctx)
{
	checkFramebufferFetchSupport(ctx);

	const bool					isES32	= glu::contextSupports(ctx.getRenderContext().getType(), glu::ApiType::es(3, 2));
	map<string, string>			args;
	args["GLSL_VERSION_STRING"]			= isES32 ? getGLSLVersionDeclaration(glu::GLSL_VERSION_320_ES) : getGLSLVersionDeclaration(glu::GLSL_VERSION_310_ES);

	const char* const fragShaderSource	=	"${GLSL_VERSION_STRING}\n"
											"#extension GL_EXT_shader_framebuffer_fetch : require\n"
											"layout(location = 0) out mediump vec4 fragColor;\n"
											"inout highp float gl_FragDepth;\n"
											"\n"
											"void main (void)\n"
											"{\n"
											"	gl_FragDepth += 0.5f;\n"
											"	fragColor = vec4(1.0f);\n"
											"}\n";

	glu::ShaderProgram program(ctx.getRenderContext(), glu::ProgramSources()
			<< glu::VertexSource(tcu::StringTemplate(vertexShaderSource).specialize(args))
			<< glu::FragmentSource(tcu::StringTemplate(fragShaderSource).specialize(args)));

	ctx.beginSection("A compile-time or link error is generated if re-declaring an existing fragment output such as gl_FragDepth as inout");
	verifyProgramError(ctx, program, PROGRAM_ERROR_COMPILE_OR_LINK, glu::SHADERTYPE_FRAGMENT);
	ctx.endSection();
}

void invalid_vertex_inout (NegativeTestContext& ctx)
{
	checkFramebufferFetchSupport(ctx);

	const bool					isES32	= glu::contextSupports(ctx.getRenderContext().getType(), glu::ApiType::es(3, 2));
	map<string, string>			args;
	args["GLSL_VERSION_STRING"]			= isES32 ? getGLSLVersionDeclaration(glu::GLSL_VERSION_320_ES) : getGLSLVersionDeclaration(glu::GLSL_VERSION_310_ES);

	const char* const vertShaderSource	=	"${GLSL_VERSION_STRING}\n"
											"#extension GL_EXT_shader_framebuffer_fetch : require\n"
											"inout mediump vec4 v_color;\n"
											"\n"
											"void main (void)\n"
											"{\n"
											"}\n";

	glu::ShaderProgram program(ctx.getRenderContext(), glu::ProgramSources()
			<< glu::VertexSource(tcu::StringTemplate(vertShaderSource).specialize(args))
			<< glu::FragmentSource(tcu::StringTemplate(fragmentShaderSource).specialize(args)));

	ctx.beginSection("A compile-time error or link error is generated if inout variables are declared in the vertex shader\n");
	verifyProgramError(ctx, program, PROGRAM_ERROR_COMPILE_OR_LINK, glu::SHADERTYPE_VERTEX);
	ctx.endSection();
}

} // anonymous

std::vector<FunctionContainer> getNegativeShaderFramebufferFetchTestFunctions (void)
{
	const FunctionContainer funcs[] =
	{
		{ last_frag_data_not_defined,		"last_frag_data_not_defined",		"The built-in gl_LastFragData not defined in #version 300 es shaders"				},
		{ last_frag_data_readonly,			"last_frag_data_readonly",			"Invalid write to readonly builtin in gl_LastFragData"								},
		{ invalid_inout_version,			"invalid_inout_version",			"Invalid use of user-defined inout arrays in versions before GLSL #version 300 es."	},
		{ invalid_redeclaration_inout,		"invalid_redeclaration_inout",		"Existing fragment shader built-ins cannot be redeclared as inout arrays"			},
		{ invalid_vertex_inout,				"invalid_vertex_inout",				"User defined inout arrays are not allowed in the vertex shader"					},
	};

	return std::vector<FunctionContainer>(DE_ARRAY_BEGIN(funcs), DE_ARRAY_END(funcs));
}

} // NegativeTestShared
} // Functional
} // gles31
} // deqp
