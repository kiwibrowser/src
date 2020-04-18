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
 * \brief Negative Precise Tests
 *//*--------------------------------------------------------------------*/

#include "es31fNegativePreciseTests.hpp"

#include "gluShaderProgram.hpp"
#include "glwEnums.hpp"

namespace deqp
{
namespace gles31
{
namespace Functional
{
namespace NegativeTestShared
{
namespace
{

enum TestPrecise
{
	TEST_PRECISE_AS_VARIABLE_NAME = 0,
	TEST_PRECISE_AS_FUNCTION_NAME,
	TEST_PRECISE_AS_ARGUMENT_NAME,
	TEST_PRECISE_AS_MACRO_NAME,
	TEST_PRECISE_MACRO_AND_VARIABLE,
	TEST_PRECISE_MACRO_AND_FUNCTION,
	TEST_PRECISE_MACRO_AND_ARGUMENT,

	TEST_PRECISE_LAST
};

static const glu::ShaderType s_shaderTypes[] =
{
	glu::SHADERTYPE_VERTEX,
	glu::SHADERTYPE_FRAGMENT,
	glu::SHADERTYPE_GEOMETRY,
	glu::SHADERTYPE_COMPUTE,
	glu::SHADERTYPE_TESSELLATION_CONTROL,
	glu::SHADERTYPE_TESSELLATION_EVALUATION
};

std::string generateShaderSource (NegativeTestContext& ctx, glu::ShaderType shaderType, TestPrecise test)
{
	const bool				supportsES32	= contextSupports(ctx.getRenderContext().getType(), glu::ApiType::es(3, 2));
	const glu::GLSLVersion	version	= supportsES32 ? glu::GLSL_VERSION_320_ES : glu::GLSL_VERSION_310_ES;
	std::ostringstream		source;

	source	<< glu::getGLSLVersionDeclaration(version) << "\n"
			<< (supportsES32 ? "" : "#extension GL_EXT_gpu_shader5 : enable\n");

	switch (test)
	{
		case TEST_PRECISE_AS_MACRO_NAME:		source << "#define precise 0\n";		break;

		case TEST_PRECISE_MACRO_AND_VARIABLE:
		case TEST_PRECISE_MACRO_AND_FUNCTION:
		case TEST_PRECISE_MACRO_AND_ARGUMENT:	source << "#define precise aName\n";	break;
		default:
			break;
	}

    switch (shaderType)
    {
        case glu::SHADERTYPE_GEOMETRY:
            source  << (supportsES32 ? "" : "#extension GL_EXT_geometry_shader : enable\n")
                    << "layout(max_vertices = 5) out;\n";
            break;

        case glu::SHADERTYPE_TESSELLATION_CONTROL:
            source  << (supportsES32 ? "" : "#extension GL_EXT_tessellation_shader : enable\n")
                    << "layout(vertices = 3) out;\n";
            break;

        case glu::SHADERTYPE_TESSELLATION_EVALUATION:
            source  << (supportsES32 ? "" : "#extension GL_EXT_tessellation_shader : enable\n")
                    << "layout(triangles, equal_spacing, cw) in;\n";
            break;

        default:
            break;
    }

	switch (test)
	{
		case TEST_PRECISE_AS_FUNCTION_NAME:
		case TEST_PRECISE_MACRO_AND_FUNCTION:
			source	<< "\n"
					<< "void precise()\n"
					<< "{\n"
					<< "}\n";
			break;

		case TEST_PRECISE_AS_ARGUMENT_NAME:
		case TEST_PRECISE_MACRO_AND_ARGUMENT:
			source	<< "\n"
					<< "void example(int precise)\n"
					<< "{\n"
					<< "}\n";
			break;

		default:
			break;
	}

    source  << "void main()\n"
			<< "{\n";

	switch (test)
	{
		case TEST_PRECISE_AS_VARIABLE_NAME:
		case TEST_PRECISE_MACRO_AND_VARIABLE:	source << "	int precise = 1;\n";		break;
		case TEST_PRECISE_AS_MACRO_NAME:		source << "	int number = precise;\n";	break;
		default:
			break;
	}

	source << "}\n";

	return source.str();
}

void generateAndVerifyShader (NegativeTestContext& ctx, glu::ShaderType shaderType, TestPrecise test)
{
	glu::Shader			shader			(ctx.getRenderContext(), shaderType);
	std::string			shaderSource	= generateShaderSource(ctx, shaderType, test);
	const char* const	source			= shaderSource.c_str();
	const int			length			= (int) shaderSource.size();

	shader.setSources(1, &source, &length);
	shader.compile();

	ctx.getLog() << shader;

	if (shader.getCompileStatus())
		ctx.fail("Shader was not expected to compile.");
}

void precise_as_variable_name (NegativeTestContext& ctx)
{
	TCU_CHECK_AND_THROW(NotSupportedError,
		ctx.isExtensionSupported("GL_EXT_gpu_shader5") || contextSupports(ctx.getRenderContext().getType(), glu::ApiType::es(3, 2)),
		"This test requires support for the extension GL_EXT_gpu_shader5 or context version 3.2 or higher.");

	ctx.beginSection("Test that precise cannot be used as a variable name.");
	for (int ndx = 0; ndx < DE_LENGTH_OF_ARRAY(s_shaderTypes); ++ndx)
	{
		if (ctx.isShaderSupported(s_shaderTypes[ndx]))
			generateAndVerifyShader(ctx, s_shaderTypes[ndx], TEST_PRECISE_AS_VARIABLE_NAME);
	}
	ctx.endSection();
}

void precise_as_function_name (NegativeTestContext& ctx)
{
	TCU_CHECK_AND_THROW(NotSupportedError,
		ctx.isExtensionSupported("GL_EXT_gpu_shader5") || contextSupports(ctx.getRenderContext().getType(), glu::ApiType::es(3, 2)),
		"This test requires support for the extension GL_EXT_gpu_shader5 or context version 3.2 or higher.");

	ctx.beginSection("Test that precise cannot be used as a function name.");
	for (int ndx = 0; ndx < DE_LENGTH_OF_ARRAY(s_shaderTypes); ++ndx)
	{
		if (ctx.isShaderSupported(s_shaderTypes[ndx]))
			generateAndVerifyShader(ctx, s_shaderTypes[ndx], TEST_PRECISE_AS_FUNCTION_NAME);
	}
	ctx.endSection();
}

void precise_as_function_argument (NegativeTestContext& ctx)
{
	TCU_CHECK_AND_THROW(NotSupportedError,
		ctx.isExtensionSupported("GL_EXT_gpu_shader5") || contextSupports(ctx.getRenderContext().getType(), glu::ApiType::es(3, 2)),
		"This test requires support for the extension GL_EXT_gpu_shader5 or context version 3.2 or higher.");

	ctx.beginSection("Test that precise cannot be used as a argument name.");
	for (int ndx = 0; ndx < DE_LENGTH_OF_ARRAY(s_shaderTypes); ++ndx)
	{
		if (ctx.isShaderSupported(s_shaderTypes[ndx]))
			generateAndVerifyShader(ctx, s_shaderTypes[ndx], TEST_PRECISE_AS_ARGUMENT_NAME);
	}
	ctx.endSection();
}

} // anonymous

std::vector<FunctionContainer> getNegativePreciseTestFunctions (void)
{
	const FunctionContainer funcs[] =
	{
        {precise_as_variable_name,			"precise_as_variable_name",			"Test precise keyword as variable name."			},
        {precise_as_function_name,			"precise_as_function_name",			"Test precise keyword as function name."			},
        {precise_as_function_argument,		"precise_as_function_argument",		"Test precise keyword as argument name."			},
	};

	return std::vector<FunctionContainer>(DE_ARRAY_BEGIN(funcs), DE_ARRAY_END(funcs));
}

} // NegativeTestShared
} // Functional
} // gles31
} // deqp
