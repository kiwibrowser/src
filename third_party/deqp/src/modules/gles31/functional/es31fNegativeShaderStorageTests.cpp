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
 * \brief Negative Shader Storage Tests
 *//*--------------------------------------------------------------------*/

#include "es31fNegativeShaderStorageTests.hpp"

#include "gluShaderProgram.hpp"
#include "glwDefs.hpp"
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

void verifyProgram(NegativeTestContext& ctx, glu::ProgramSources sources)
{
	tcu::TestLog&				log			= ctx.getLog();
	const glu::ShaderProgram	program		(ctx.getRenderContext(), sources);
	bool						testFailed	= false;

	log << program;

	testFailed = program.getProgramInfo().linkOk;

	if (testFailed)
	{
		const char* const message("Program was not expected to link.");
		log << tcu::TestLog::Message << message << tcu::TestLog::EndMessage;
		ctx.fail(message);
	}
}

const char* getShaderExtensionDeclaration (glw::GLenum glShaderType)
{
	switch (glShaderType)
	{
		case GL_TESS_CONTROL_SHADER:
		case GL_TESS_EVALUATION_SHADER: return "#extension GL_EXT_tessellation_shader : require\n";
		case GL_GEOMETRY_SHADER:		return "#extension GL_EXT_geometry_shader : require\n";
		default:
			return "";
	}
}

glu::ShaderType getGLUShaderType (glw::GLenum glShaderType)
{
	switch (glShaderType)
	{
		case GL_VERTEX_SHADER:			 return glu::SHADERTYPE_VERTEX;
		case GL_FRAGMENT_SHADER:		 return glu::SHADERTYPE_FRAGMENT;
		case GL_TESS_CONTROL_SHADER:	 return glu::SHADERTYPE_TESSELLATION_CONTROL;
		case GL_TESS_EVALUATION_SHADER:	 return glu::SHADERTYPE_TESSELLATION_EVALUATION;
		case GL_GEOMETRY_SHADER:		 return glu::SHADERTYPE_GEOMETRY;
		case GL_COMPUTE_SHADER:			 return glu::SHADERTYPE_COMPUTE;
		default:
			DE_FATAL("Unknown shader type");
			return glu::SHADERTYPE_LAST;
	}
}

glw::GLenum getMaxSSBlockSizeEnum (glw::GLenum glShaderType)
{
	switch (glShaderType)
	{
		case GL_VERTEX_SHADER:			 return GL_MAX_VERTEX_SHADER_STORAGE_BLOCKS;
		case GL_FRAGMENT_SHADER:		 return GL_MAX_FRAGMENT_SHADER_STORAGE_BLOCKS;
		case GL_TESS_CONTROL_SHADER:	 return GL_MAX_TESS_CONTROL_SHADER_STORAGE_BLOCKS;
		case GL_TESS_EVALUATION_SHADER:	 return GL_MAX_TESS_EVALUATION_SHADER_STORAGE_BLOCKS;
		case GL_GEOMETRY_SHADER:		 return GL_MAX_GEOMETRY_SHADER_STORAGE_BLOCKS;
		case GL_COMPUTE_SHADER:			 return GL_MAX_COMPUTE_SHADER_STORAGE_BLOCKS;
		default:
			 DE_FATAL("Unknown shader type");
			 return -1;
	}
}

int getMaxSSBlockSize (NegativeTestContext& ctx, glw::GLenum glShaderType)
{
	int maxSSBlocks = 0;
	ctx.glGetIntegerv(getMaxSSBlockSizeEnum(glShaderType), &maxSSBlocks);

	return maxSSBlocks;
}

std::string genBlockSource (NegativeTestContext& ctx, deInt64 numSSBlocks, glw::GLenum shaderType)
{
	const bool				isES32		= contextSupports(ctx.getRenderContext().getType(), glu::ApiType::es(3, 2));
	const glu::GLSLVersion	version		= isES32 ? glu::GLSL_VERSION_320_ES : glu::GLSL_VERSION_310_ES;
	std::ostringstream		source;

	source	<< glu::getGLSLVersionDeclaration(version) << "\n"
			<< ((isES32) ? "" : getShaderExtensionDeclaration(shaderType));

	switch (shaderType)
	{
		case GL_VERTEX_SHADER:
		case GL_FRAGMENT_SHADER:
			break;

		case GL_COMPUTE_SHADER:
			source << "layout (local_size_x = 1) in;\n";
			break;

		case GL_GEOMETRY_SHADER:
			source << "layout(points) in;\n"
				   << "layout(line_strip, max_vertices = 3) out;\n";
			break;

		case GL_TESS_CONTROL_SHADER:
			source << "layout(vertices = 10) out;\n";
			break;

		case GL_TESS_EVALUATION_SHADER:
			source << "layout(triangles) in;\n";
			break;

		default:
			DE_FATAL("Unknown shader type");
			break;
	}

	source  << "\n"
			<< "layout(std430, binding = 0) buffer Block {\n"
			<< "    int value;\n"
			<< "} sb_in[" << numSSBlocks << "];\n"
			<< "void main(void) { sb_in[0].value = 1; }\n";

	return source.str();
}

std::string genCommonSource (NegativeTestContext& ctx, glw::GLenum shaderType)
{
	const bool				isES32		= contextSupports(ctx.getRenderContext().getType(), glu::ApiType::es(3, 2));
	const glu::GLSLVersion	version		= isES32 ? glu::GLSL_VERSION_320_ES : glu::GLSL_VERSION_310_ES;
	std::ostringstream		source;

	source << glu::getGLSLVersionDeclaration(version) << "\n"
		   << ((isES32) ? "" : getShaderExtensionDeclaration(shaderType));

	switch (shaderType)
	{
		case GL_TESS_CONTROL_SHADER:
			source	<< "layout(vertices = 3) out;\n"
					<< "void main() {}\n";
			break;

		case GL_TESS_EVALUATION_SHADER:
			source	<< "layout(triangles, equal_spacing, cw) in;\n"
					<< "void main() {}\n";
			break;

		default:
			source  << "void main() {}\n";
			break;
	}

	return source.str();
}

int genMaxSSBlocksSource (NegativeTestContext& ctx, glw::GLenum glShaderType, glu::ProgramSources& sources)
{
	int		maxSSBlocks				= getMaxSSBlockSize(ctx, glShaderType);
	const	std::string shaderSrc	= genBlockSource(ctx, (maxSSBlocks), glShaderType);

	sources.sources[getGLUShaderType(glShaderType)].push_back(shaderSrc);

	return maxSSBlocks;
}

void block_number_limits (NegativeTestContext& ctx)
{
	const glw::GLenum glShaderTypes[] =
	{
		GL_VERTEX_SHADER,
		GL_FRAGMENT_SHADER,
		GL_TESS_CONTROL_SHADER,
		GL_TESS_EVALUATION_SHADER,
		GL_GEOMETRY_SHADER,
		GL_COMPUTE_SHADER,
	};

	const std::string	vertSource			= genCommonSource(ctx, GL_VERTEX_SHADER);
	const std::string	fragSource			= genCommonSource(ctx, GL_FRAGMENT_SHADER);
	const std::string	tessControlSource	= genCommonSource(ctx, GL_TESS_CONTROL_SHADER);
	const std::string	tessEvalSource		= genCommonSource(ctx, GL_TESS_EVALUATION_SHADER);

	for (int ndx = 0; ndx < DE_LENGTH_OF_ARRAY(glShaderTypes); ndx++)
	{
		ctx.beginSection("maxShaderStorageBlocks: Exceed limits");

		if (!ctx.isShaderSupported(static_cast<glu::ShaderType>(getGLUShaderType(glShaderTypes[ndx]))))
		{
			ctx.endSection();
			continue;
		}

		int					maxSSBlocks			= getMaxSSBlockSize(ctx, glShaderTypes[ndx]);
		std::string			source				= genBlockSource(ctx, maxSSBlocks+1, glShaderTypes[ndx]);

		glu::ProgramSources sources;

		if (maxSSBlocks == 0)
		{
			ctx.endSection();
			continue;
		}

		switch (glShaderTypes[ndx])
		{
			case GL_VERTEX_SHADER:
				sources << glu::VertexSource(source)
						<< glu::FragmentSource(fragSource);
				break;

			case GL_FRAGMENT_SHADER:
				sources << glu::VertexSource(vertSource)
						<< glu::FragmentSource(source);
				break;

			case GL_TESS_CONTROL_SHADER:
				sources << glu::VertexSource(vertSource)
						<< glu::FragmentSource(fragSource)
						<< glu::TessellationControlSource(source)
						<< glu::TessellationEvaluationSource(tessEvalSource);
				break;

			case GL_TESS_EVALUATION_SHADER:
				sources << glu::VertexSource(vertSource)
						<< glu::FragmentSource(fragSource)
						<< glu::TessellationControlSource(tessControlSource)
						<< glu::TessellationEvaluationSource(source);
				break;

			case GL_GEOMETRY_SHADER:
				sources << glu::VertexSource(vertSource)
						<< glu::FragmentSource(fragSource)
						<< glu::GeometrySource(source);
				break;

			case GL_COMPUTE_SHADER:
				sources << glu::ComputeSource(source);
				break;

			default:
				DE_FATAL("Unknown shader type");
				break;
		}

		verifyProgram(ctx, sources);
		ctx.endSection();
	}
}

void max_combined_block_number_limit (NegativeTestContext& ctx)
{
	ctx.beginSection("maxCombinedShaderStorageBlocks: Exceed limits");

	glu::ProgramSources sources;

	int combinedSSBlocks	= 0;
	int maxCombinedSSBlocks = 0;

	combinedSSBlocks += genMaxSSBlocksSource(ctx, GL_VERTEX_SHADER, sources);
	combinedSSBlocks += genMaxSSBlocksSource(ctx, GL_FRAGMENT_SHADER, sources);

	if ((ctx.isShaderSupported(glu::SHADERTYPE_TESSELLATION_CONTROL)) && (ctx.isShaderSupported(glu::SHADERTYPE_TESSELLATION_EVALUATION)))
	{
		combinedSSBlocks += genMaxSSBlocksSource(ctx, GL_TESS_CONTROL_SHADER, sources);
		combinedSSBlocks += genMaxSSBlocksSource(ctx, GL_TESS_EVALUATION_SHADER, sources);
	}

	if (ctx.isShaderSupported(glu::SHADERTYPE_GEOMETRY))
		combinedSSBlocks += genMaxSSBlocksSource(ctx, GL_GEOMETRY_SHADER, sources);

	ctx.glGetIntegerv(GL_MAX_COMBINED_SHADER_STORAGE_BLOCKS, &maxCombinedSSBlocks);

	ctx.getLog() << tcu::TestLog::Message << "GL_MAX_COMBINED_SHADER_STORAGE_BLOCKS: " << maxCombinedSSBlocks << tcu::TestLog::EndMessage;
	ctx.getLog() << tcu::TestLog::Message << "Combined shader storage blocks: " << combinedSSBlocks << tcu::TestLog::EndMessage;

	if (combinedSSBlocks > maxCombinedSSBlocks)
		verifyProgram(ctx, sources);
	else
		ctx.getLog() << tcu::TestLog::Message << "Test skipped: Combined shader storage blocks < GL_MAX_COMBINED_SHADER_STORAGE_BLOCKS: " << tcu::TestLog::EndMessage;

	ctx.endSection();
}

} // anonymous

std::vector<FunctionContainer> getNegativeShaderStorageTestFunctions ()
{
	const FunctionContainer funcs[] =
	{
		{ block_number_limits,				"block_number_limits",				"Invalid shader linkage" },
		{ max_combined_block_number_limit,	"max_combined_block_number_limit",	"Invalid shader linkage" },
	};

	return std::vector<FunctionContainer>(DE_ARRAY_BEGIN(funcs), DE_ARRAY_END(funcs));
}

} // NegativeTestShared
} // Functional
} // gles31
} // deqp
