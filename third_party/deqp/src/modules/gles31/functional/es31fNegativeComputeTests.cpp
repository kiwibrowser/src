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
 * \brief Negative Compute tests.
 *//*--------------------------------------------------------------------*/

#include "es31fNegativeComputeTests.hpp"
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
namespace
{

using tcu::TestLog;
using namespace glw;

static const char* const vertexShaderSource			=	"${GLSL_VERSION_STRING}\n"
														"\n"
														"void main (void)\n"
														"{\n"
														"	gl_Position = vec4(0.0);\n"
														"}\n";

static const char* const fragmentShaderSource		=	"${GLSL_VERSION_STRING}\n"
														"precision mediump float;\n"
														"layout(location = 0) out mediump vec4 fragColor;\n"
														"\n"
														"void main (void)\n"
														"{\n"
														"	fragColor = vec4(1.0);\n"
														"}\n";

static const char* const computeShaderSource		=	"${GLSL_VERSION_STRING}\n"
														"layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;\n"
														"void main (void)\n"
														"{\n"
														"}\n";

static const char* const invalidComputeShaderSource	=	"${GLSL_VERSION_STRING}\n"
														"layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;\n"
														"void main (void)\n"
														"{\n"
														"	highp uint var = -1;\n" // error
														"}\n";

int getResourceLimit (NegativeTestContext& ctx, GLenum resource)
{
	int limit = 0;
	ctx.glGetIntegerv(resource, &limit);

	return limit;
}

void verifyLinkError (NegativeTestContext& ctx, const glu::ShaderProgram& program)
{
	bool testFailed = false;

	tcu::TestLog& log = ctx.getLog();
	log << program;

	testFailed = program.getProgramInfo().linkOk;

	if (testFailed)
	{
		const char* const message("Program was not expected to link.");
		log << tcu::TestLog::Message << message << tcu::TestLog::EndMessage;
		ctx.fail(message);
	}
}

void verifyCompileError (NegativeTestContext& ctx, const glu::ShaderProgram& program, glu::ShaderType shaderType)
{
	bool testFailed = false;

	tcu::TestLog& log = ctx.getLog();
	log << program;

	testFailed = program.getShaderInfo(shaderType).compileOk;

	if (testFailed)
	{
		const char* const message("Program was not expected to compile.");
		log << tcu::TestLog::Message << message << tcu::TestLog::EndMessage;
		ctx.fail(message);
	}
}

string generateComputeShader (NegativeTestContext& ctx, const string& shaderDeclarations, const string& shaderBody)
{
	const bool			isES32			= glu::contextSupports(ctx.getRenderContext().getType(), glu::ApiType::es(3, 2));
	const char* const	shaderVersion	= isES32
										? getGLSLVersionDeclaration(glu::GLSL_VERSION_320_ES)
										: getGLSLVersionDeclaration(glu::GLSL_VERSION_310_ES);

	std::ostringstream compShaderSource;

	compShaderSource	<<	shaderVersion << "\n"
						<<	"layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;\n"
						<<	shaderDeclarations << "\n"
						<<	"void main (void)\n"
						<<	"{\n"
						<<	shaderBody
						<<	"}\n";

	return compShaderSource.str();
}

string genBuiltInSource (glu::ShaderType shaderType)
{
	std::ostringstream		source;
	source << "${GLSL_VERSION_STRING}\n";

	switch (shaderType)
	{
		case glu::SHADERTYPE_VERTEX:
		case glu::SHADERTYPE_FRAGMENT:
			break;

		case glu::SHADERTYPE_COMPUTE:
			source << "layout (local_size_x = 1) in;\n";
			break;

		case glu::SHADERTYPE_GEOMETRY:
			source << "layout(points) in;\n"
				   << "layout(line_strip, max_vertices = 3) out;\n";
			break;

		case glu::SHADERTYPE_TESSELLATION_CONTROL:
			source << "${GLSL_TESS_EXTENSION_STRING}\n"
				   << "layout(vertices = 10) out;\n";
			break;

		case glu::SHADERTYPE_TESSELLATION_EVALUATION:
			source << "${GLSL_TESS_EXTENSION_STRING}\n"
				   << "layout(triangles) in;\n";
			break;

		default:
			DE_FATAL("Unknown shader type");
			break;
	}

	source	<< "\n"
			<< "void main(void)\n"
			<< "{\n"
			<< "${COMPUTE_BUILT_IN_CONSTANTS_STRING}"
			<< "}\n";

	return source.str();
}

void exceed_uniform_block_limit (NegativeTestContext& ctx)
{
	std::ostringstream shaderDecl;
	std::ostringstream shaderBody;

	shaderDecl	<< "layout(std140, binding = 0) uniform Block\n"
				<< "{\n"
				<< "    highp vec4 val;\n"
				<< "} block[" << getResourceLimit(ctx, GL_MAX_COMPUTE_UNIFORM_BLOCKS) + 1 << "];\n";

	glu::ShaderProgram	program(ctx.getRenderContext(), glu::ProgramSources()
			<< glu::ComputeSource(generateComputeShader(ctx, shaderDecl.str(), shaderBody.str())));

	ctx.beginSection("Link error is generated if a compute shader exceeds GL_MAX_COMPUTE_UNIFORM_BLOCKS.");
	verifyLinkError(ctx, program);
	ctx.endSection();
}

void exceed_shader_storage_block_limit (NegativeTestContext& ctx)
{
	std::ostringstream shaderDecl;
	std::ostringstream shaderBody;

	shaderDecl	<< "layout(std140, binding = 0) buffer Block\n"
				<< "{\n"
				<< "    highp vec4 val;\n"
				<< "} block[" << getResourceLimit(ctx, GL_MAX_COMPUTE_SHADER_STORAGE_BLOCKS) + 1 << "];\n";

	glu::ShaderProgram	program(ctx.getRenderContext(), glu::ProgramSources()
			<< glu::ComputeSource(generateComputeShader(ctx, shaderDecl.str(), shaderBody.str())));

	ctx.beginSection("Link error is generated if compute shader exceeds GL_MAX_COMPUTE_SHADER_STORAGE_BLOCKS.");
	verifyLinkError(ctx, program);
	ctx.endSection();
}

void exceed_texture_image_units_limit (NegativeTestContext& ctx)
{
	const int			limit			= getResourceLimit(ctx, GL_MAX_COMPUTE_TEXTURE_IMAGE_UNITS) + 1;
	std::ostringstream	shaderDecl;
	std::ostringstream	shaderBody;

	shaderDecl	<< "layout(binding = 0) "
				<< "uniform highp sampler2D u_sampler[" << limit + 1 << "];\n"
				<< "\n"
				<< "layout(binding = 0) buffer Output {\n"
				<< "    vec4 values[ " << limit + 1 << " ];\n"
				<< "} sb_out;\n";

	for (int i = 0; i < limit + 1; ++i)
		shaderBody	<< "   sb_out.values[" << i << "] = texture(u_sampler[" << i << "], vec2(1.0f));\n";

	glu::ShaderProgram	program(ctx.getRenderContext(), glu::ProgramSources()
			<< glu::ComputeSource(generateComputeShader(ctx, shaderDecl.str(), shaderBody.str())));

	tcu::TestLog& log = ctx.getLog();
	log << tcu::TestLog::Message << "Possible link error is generated if compute shader exceeds GL_MAX_COMPUTE_TEXTURE_IMAGE_UNITS." << tcu::TestLog::EndMessage;
	log << program;

	if (program.getProgramInfo().linkOk)
	{
		log << tcu::TestLog::Message << "Quality Warning: program was not expected to link." << tcu::TestLog::EndMessage;
		ctx.glUseProgram(program.getProgram());
		ctx.expectError(GL_NO_ERROR);

		ctx.beginSection("GL_INVALID_OPERATION error is generated if the sum of the number of active samplers for each active program exceeds the maximum number of texture image units allowed");
		ctx.glDispatchCompute(1, 1, 1);
		ctx.expectError(GL_INVALID_OPERATION);
		ctx.endSection();
	}
}

void exceed_image_uniforms_limit (NegativeTestContext& ctx)
{
	const int			limit = getResourceLimit(ctx, GL_MAX_COMPUTE_IMAGE_UNIFORMS);
	std::ostringstream	shaderDecl;
	std::ostringstream	shaderBody;

	shaderDecl	<< "layout(rgba8, binding = 0) "
				<< "uniform readonly highp image2D u_image[" << limit + 1 << "];\n"
				<< "\n"
				<< "layout(binding = 0) buffer Output {\n"
				<< "    float values[" << limit + 1 << "];\n"
				<< "} sb_out;\n";

	for (int i = 0; i < limit + 1; ++i)
		shaderBody	<< "    sb_out.values[" << i << "]" << "  = imageLoad(u_image[" << i << "], ivec2(gl_GlobalInvocationID.xy)).x;\n";

	glu::ShaderProgram	program(ctx.getRenderContext(), glu::ProgramSources()
			<< glu::ComputeSource(generateComputeShader(ctx, shaderDecl.str(), shaderBody.str())));

	ctx.beginSection("Link error is generated if compute shader exceeds GL_MAX_COMPUTE_IMAGE_UNIFORMS.");
	verifyLinkError(ctx, program);
	ctx.endSection();
}

void exceed_shared_memory_size_limit (NegativeTestContext& ctx)
{
	const int			limit				= getResourceLimit(ctx, GL_MAX_COMPUTE_SHARED_MEMORY_SIZE);
	const long			numberOfElements	= limit / sizeof(GLuint);
	std::ostringstream	shaderDecl;
	std::ostringstream	shaderBody;

	shaderDecl	<< "shared uint values[" << numberOfElements + 1 << "];\n"
				<< "\n"
				<< "layout(binding = 0) buffer Output {\n"
				<< "    uint values;\n"
				<< "} sb_out;\n";

	shaderBody	<< "    sb_out.values = values[" << numberOfElements << "];\n";

	glu::ShaderProgram	program(ctx.getRenderContext(), glu::ProgramSources()
			<< glu::ComputeSource(generateComputeShader(ctx, shaderDecl.str(), shaderBody.str())));

	ctx.beginSection("Link error is generated if compute shader exceeds GL_MAX_COMPUTE_SHARED_MEMORY_SIZE.");
	verifyLinkError(ctx, program);
	ctx.endSection();
}

void exceed_uniform_components_limit (NegativeTestContext& ctx)
{
	const int			limit = getResourceLimit(ctx, GL_MAX_COMPUTE_UNIFORM_COMPONENTS);
	std::ostringstream	shaderDecl;
	std::ostringstream	shaderBody;

	shaderDecl	<< "uniform highp uint u_value[" << limit + 1 << "];\n"
				<< "\n"
				<< "layout(binding = 0) buffer Output {\n"
				<< "    uint values[2];\n"
				<< "} sb_out;\n";

	shaderBody << "    sb_out.values[0] = u_value[" << limit << "];\n";
	shaderBody << "    sb_out.values[1] = u_value[0];\n";

	glu::ShaderProgram	program(ctx.getRenderContext(), glu::ProgramSources()
			<< glu::ComputeSource(generateComputeShader(ctx, shaderDecl.str(), shaderBody.str())));

	ctx.beginSection("Link error is generated if compute shader exceeds GL_MAX_COMPUTE_UNIFORM_COMPONENTS.");
	verifyLinkError(ctx, program);
	ctx.endSection();
}

void exceed_atomic_counter_buffer_limit (NegativeTestContext& ctx)
{
	const int			limit = getResourceLimit(ctx, GL_MAX_COMPUTE_ATOMIC_COUNTER_BUFFERS);
	std::ostringstream	shaderDecl;
	std::ostringstream	shaderBody;

	for (int i = 0; i < limit + 1; ++i)
	{
		shaderDecl	<< "layout(binding = " << i << ") "
					<< "uniform atomic_uint u_atomic" << i << ";\n";

		if (i == 0)
			shaderBody	<< "    uint oldVal = atomicCounterIncrement(u_atomic" << i << ");\n";
		else
			shaderBody	<< "    oldVal = atomicCounterIncrement(u_atomic" << i << ");\n";
	}

	shaderBody	<< "    sb_out.value = oldVal;\n";

	shaderDecl	<< "\n"
				<< "layout(binding = 0) buffer Output {\n"
				<< "    uint value;\n"
				<< "} sb_out;\n";

	glu::ShaderProgram	program(ctx.getRenderContext(), glu::ProgramSources()
			<< glu::ComputeSource(generateComputeShader(ctx, shaderDecl.str(), shaderBody.str())));

	ctx.beginSection("Link error is generated if compute shader exceeds GL_MAX_COMPUTE_ATOMIC_COUNTER_BUFFERS.");
	verifyLinkError(ctx, program);
	ctx.endSection();
}

void exceed_atomic_counters_limit (NegativeTestContext& ctx)
{
	std::ostringstream	shaderDecl;
	std::ostringstream	shaderBody;

	shaderDecl	<< "layout(binding = 0, offset = 0) uniform atomic_uint u_atomic0;\n"
				<< "layout(binding = " << sizeof(GLuint) * getResourceLimit(ctx, GL_MAX_COMPUTE_ATOMIC_COUNTERS) << ", offset = 0) uniform atomic_uint u_atomic1;\n"
				<< "\n"
				<< "layout(binding = 0) buffer Output {\n"
				<< "    uint value;\n"
				<< "} sb_out;\n";

	shaderBody	<< "    uint oldVal = 0u;\n"
				<< "    oldVal = atomicCounterIncrement(u_atomic0);\n"
				<< "    oldVal = atomicCounterIncrement(u_atomic1);\n"
				<< "    sb_out.value = oldVal;\n";

	glu::ShaderProgram	program(ctx.getRenderContext(), glu::ProgramSources()
			<< glu::ComputeSource(generateComputeShader(ctx, shaderDecl.str(), shaderBody.str())));

	ctx.beginSection("Link error is generated if compute shader exceeds GL_MAX_COMPUTE_ATOMIC_COUNTERS.");
	verifyLinkError(ctx, program);
	ctx.endSection();
}

void program_not_active (NegativeTestContext& ctx)
{
	const bool					isES32	= glu::contextSupports(ctx.getRenderContext().getType(), glu::ApiType::es(3, 2));
	map<string, string>			args;
	args["GLSL_VERSION_STRING"]			= isES32 ? getGLSLVersionDeclaration(glu::GLSL_VERSION_320_ES) : getGLSLVersionDeclaration(glu::GLSL_VERSION_310_ES);

	const glu::VertexSource		vertSource(tcu::StringTemplate(vertexShaderSource).specialize(args));
	const glu::FragmentSource	fragSource(tcu::StringTemplate(fragmentShaderSource).specialize(args));

	glu::ProgramPipeline		pipeline(ctx.getRenderContext());

	glu::ShaderProgram			vertProgram	(ctx.getRenderContext(), glu::ProgramSources() << glu::ProgramSeparable(true) << vertSource);
	glu::ShaderProgram			fragProgram	(ctx.getRenderContext(), glu::ProgramSources() << glu::ProgramSeparable(true) << fragSource);

	tcu::TestLog& log			= ctx.getLog();
	log << vertProgram << fragProgram;

	if (!vertProgram.isOk() || !fragProgram.isOk())
		TCU_THROW(InternalError, "failed to build program");

	ctx.glBindProgramPipeline(pipeline.getPipeline());
	ctx.expectError(GL_NO_ERROR);

	ctx.beginSection("Program not set at all");
	{
		ctx.beginSection("GL_INVALID_OPERATION is generated by glDispatchCompute if there is no active program for the compute shader stage.");
		ctx.glDispatchCompute(1, 1, 1);
		ctx.expectError(GL_INVALID_OPERATION);
		ctx.endSection();

		ctx.beginSection("GL_INVALID_OPERATION is generated by glDispatchComputeIndirect if there is no active program for the compute shader stage.");
		GLintptr indirect = 0;
		ctx.glDispatchComputeIndirect(indirect);
		ctx.expectError(GL_INVALID_OPERATION);
		ctx.endSection();
	}
	ctx.endSection();

	ctx.beginSection("Program contains graphic pipeline stages");
	{
		ctx.glUseProgramStages(pipeline.getPipeline(), GL_VERTEX_SHADER_BIT, vertProgram.getProgram());
		ctx.glUseProgramStages(pipeline.getPipeline(), GL_FRAGMENT_SHADER_BIT, fragProgram.getProgram());
		ctx.expectError(GL_NO_ERROR);

		ctx.beginSection("GL_INVALID_OPERATION is generated by glDispatchCompute if there is no active program for the compute shader stage.");
		ctx.glDispatchCompute(1, 1, 1);
		ctx.expectError(GL_INVALID_OPERATION);
		ctx.endSection();

		ctx.beginSection("GL_INVALID_OPERATION is generated by glDispatchComputeIndirect if there is no active program for the compute shader stage.");
		GLintptr indirect = 0;
		ctx.glDispatchComputeIndirect(indirect);
		ctx.expectError(GL_INVALID_OPERATION);
		ctx.endSection();
	}
	ctx.endSection();

	ctx.glBindProgramPipeline(0);
	ctx.expectError(GL_NO_ERROR);
}

void invalid_program_query (NegativeTestContext& ctx)
{
	const bool					isES32	= glu::contextSupports(ctx.getRenderContext().getType(), glu::ApiType::es(3, 2));
	map<string, string>			args;
	args["GLSL_VERSION_STRING"]			= isES32 ? getGLSLVersionDeclaration(glu::GLSL_VERSION_320_ES) : getGLSLVersionDeclaration(glu::GLSL_VERSION_310_ES);

	GLint data[3] = { 0, 0, 0 };

	ctx.beginSection("Compute shader that does not link");
	{
		const glu::ComputeSource	compSource(tcu::StringTemplate(invalidComputeShaderSource).specialize(args));
		glu::ShaderProgram			invalidComputeProgram (ctx.getRenderContext(), glu::ProgramSources() << compSource);

		tcu::TestLog& log = ctx.getLog();
		log << invalidComputeProgram;

		if (invalidComputeProgram.isOk())
			TCU_THROW(InternalError, "program should not of built");

		ctx.beginSection("GL_INVALID_OPERATION is generated if GL_COMPUTE_WORK_GROUP_SIZE is queried for a program which has not been linked properly.");
		ctx.glGetProgramiv(invalidComputeProgram.getProgram(), GL_COMPUTE_WORK_GROUP_SIZE, &data[0]);
		ctx.expectError(GL_INVALID_OPERATION);
		ctx.endSection();

		ctx.glUseProgram(0);
	}
	ctx.endSection();

	ctx.beginSection("Compute shader not present");
	{
		const glu::VertexSource		vertSource(tcu::StringTemplate(vertexShaderSource).specialize(args));
		const glu::FragmentSource	fragSource(tcu::StringTemplate(fragmentShaderSource).specialize(args));
		glu::ShaderProgram			graphicsPipelineProgram	(ctx.getRenderContext(), glu::ProgramSources() << vertSource << fragSource);

		tcu::TestLog& log = ctx.getLog();
		log << graphicsPipelineProgram;

		if (!graphicsPipelineProgram.isOk())
			TCU_THROW(InternalError, "failed to build program");

		ctx.beginSection("GL_INVALID_OPERATION is generated if GL_COMPUTE_WORK_GROUP_SIZE is queried for a program which has not been linked properly.");
		ctx.glGetProgramiv(graphicsPipelineProgram.getProgram(), GL_COMPUTE_WORK_GROUP_SIZE, &data[0]);
		ctx.expectError(GL_INVALID_OPERATION);
		ctx.endSection();

		ctx.glUseProgram(0);
	}
	ctx.endSection();
}

void invalid_dispatch_compute_indirect (NegativeTestContext& ctx)
{
	const bool					isES32	= glu::contextSupports(ctx.getRenderContext().getType(), glu::ApiType::es(3, 2));
	map<string, string>			args;
	args["GLSL_VERSION_STRING"]			= isES32 ? getGLSLVersionDeclaration(glu::GLSL_VERSION_320_ES) : getGLSLVersionDeclaration(glu::GLSL_VERSION_310_ES);

	const glu::ComputeSource	compSource(tcu::StringTemplate(computeShaderSource).specialize(args));
	glu::ShaderProgram			program(ctx.getRenderContext(), glu::ProgramSources() << compSource);

	tcu::TestLog& log = ctx.getLog();
	log << program;

	if (!program.isOk())
		TCU_THROW(InternalError, "failed to build program");

	ctx.glUseProgram(program.getProgram());
	ctx.expectError(GL_NO_ERROR);

	static const struct
	{
		GLuint numGroupsX;
		GLuint numGroupsY;
		GLuint numGroupsZ;
	} data = {0, 0, 0};

	{
		GLuint buffer;
		ctx.glGenBuffers(1, &buffer);
		ctx.glBindBuffer(GL_DISPATCH_INDIRECT_BUFFER, 0);
		ctx.expectError(GL_NO_ERROR);

		ctx.beginSection("GL_INVALID_OPERATION is generated by glDispatchComputeIndirect if zero is bound to GL_DISPATCH_INDIRECT_BUFFER.");
		GLintptr indirect = 0;
		ctx.glDispatchComputeIndirect(indirect);
		ctx.expectError(GL_INVALID_OPERATION);
		ctx.endSection();

		ctx.glDeleteBuffers(1, &buffer);
	}

	{
		GLuint buffer;
		ctx.glGenBuffers(1, &buffer);
		ctx.expectError(GL_NO_ERROR);

		ctx.glBindBuffer(GL_DISPATCH_INDIRECT_BUFFER, buffer);
		ctx.expectError(GL_NO_ERROR);

		ctx.glBufferData(GL_DISPATCH_INDIRECT_BUFFER, sizeof(data), &data, GL_STATIC_DRAW);
		ctx.expectError(GL_NO_ERROR);

		ctx.beginSection("GL_INVALID_OPERATION is generated by glDispatchComputeIndirect if data is sourced beyond the end of the buffer object.");
		GLintptr indirect = 1 << 10;
		ctx.glDispatchComputeIndirect(indirect);
		ctx.expectError(GL_INVALID_OPERATION);
		ctx.endSection();

		ctx.glBindBuffer(GL_DISPATCH_INDIRECT_BUFFER, 0);
		ctx.glDeleteBuffers(1, &buffer);
	}

	{
		ctx.beginSection("GL_INVALID_VALUE is generated by glDispatchComputeIndirect if the value of indirect is less than zero.");
		GLintptr indirect = -1;
		ctx.glDispatchComputeIndirect(indirect);
		ctx.expectError(GL_INVALID_VALUE);
		ctx.endSection();
	}

	{
		GLuint buffer;
		ctx.glGenBuffers(1, &buffer);
		ctx.expectError(GL_NO_ERROR);

		ctx.glBindBuffer(GL_DISPATCH_INDIRECT_BUFFER, buffer);
		ctx.expectError(GL_NO_ERROR);

		ctx.glBufferData(GL_DISPATCH_INDIRECT_BUFFER, sizeof(data), &data, GL_STATIC_DRAW);
		ctx.expectError(GL_NO_ERROR);

		ctx.beginSection("GL_INVALID_VALUE is generated by glDispatchComputeIndirect if indirect is not a multiple of the size, in basic machine units, of uint.");
		GLintptr indirect = sizeof(data) + 1;
		ctx.glDispatchComputeIndirect(indirect);
		ctx.expectError(GL_INVALID_VALUE);
		ctx.endSection();

		ctx.glBindBuffer(GL_DISPATCH_INDIRECT_BUFFER, 0);
		ctx.glDeleteBuffers(1, &buffer);
	}
}

void invalid_maximum_work_group_counts (NegativeTestContext& ctx)
{
	const bool					isES32	= glu::contextSupports(ctx.getRenderContext().getType(), glu::ApiType::es(3, 2));
	map<string, string>			args;
	args["GLSL_VERSION_STRING"]			= isES32 ? getGLSLVersionDeclaration(glu::GLSL_VERSION_320_ES) : getGLSLVersionDeclaration(glu::GLSL_VERSION_310_ES);

	const glu::ComputeSource	compSource(tcu::StringTemplate(computeShaderSource).specialize(args));
	glu::ShaderProgram			program(ctx.getRenderContext(), glu::ProgramSources() << compSource);

	tcu::TestLog& log = ctx.getLog();
	log << program;

	if (!program.isOk())
		TCU_THROW(InternalError, "failed to build program");

	ctx.glUseProgram(program.getProgram());
	ctx.expectError(GL_NO_ERROR);

	GLint workGroupCountX;
	ctx.glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, (GLuint)0, &workGroupCountX);
	ctx.expectError(GL_NO_ERROR);

	ctx.beginSection("GL_INVALID_VALUE is generated by glDispatchCompute if <numGroupsX> array is larger than the maximum work group count for the x dimension.");
	ctx.glDispatchCompute(workGroupCountX+1, 1, 1);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();

	GLint workGroupCountY;
	ctx.glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, (GLuint)1, &workGroupCountY);
	ctx.expectError(GL_NO_ERROR);

	ctx.beginSection("GL_INVALID_VALUE is generated by glDispatchCompute if <numGroupsY> array is larger than the maximum work group count for the y dimension.");
	ctx.glDispatchCompute(1, workGroupCountY+1, 1);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();

	GLint workGroupCountZ;
	ctx.glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, (GLuint)2, &workGroupCountZ);
	ctx.expectError(GL_NO_ERROR);

	ctx.beginSection("GL_INVALID_VALUE is generated by glDispatchCompute if <numGroupsZ> array is larger than the maximum work group count for the z dimension.");
	ctx.glDispatchCompute(1, 1, workGroupCountZ+1);
	ctx.expectError(GL_INVALID_VALUE);
	ctx.endSection();
}

void invalid_maximum_work_group_sizes (NegativeTestContext& ctx)
{
	GLint maxWorkGroupSizeX;
	ctx.glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, (GLuint)0, &maxWorkGroupSizeX);
	ctx.expectError(GL_NO_ERROR);

	GLint maxWorkGroupSizeY;
	ctx.glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, (GLuint)1, &maxWorkGroupSizeY);
	ctx.expectError(GL_NO_ERROR);

	GLint maxWorkGroupSizeZ;
	ctx.glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, (GLuint)2, &maxWorkGroupSizeZ);
	ctx.expectError(GL_NO_ERROR);

	GLint maxWorkGroupInvocations;
	ctx.glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &maxWorkGroupInvocations);
	ctx.expectError(GL_NO_ERROR);

	DE_ASSERT(((deInt64) maxWorkGroupSizeX * maxWorkGroupSizeY * maxWorkGroupSizeZ) > maxWorkGroupInvocations );

	const bool				isES32			= glu::contextSupports(ctx.getRenderContext().getType(), glu::ApiType::es(3, 2));
	const char* const		shaderVersion	= isES32
											? getGLSLVersionDeclaration(glu::GLSL_VERSION_320_ES)
											: getGLSLVersionDeclaration(glu::GLSL_VERSION_310_ES);

	static const struct
	{
		GLint x;
		GLint y;
		GLint z;
	} localWorkGroupSizeCases[] =
	{
		{	maxWorkGroupSizeX+1,	1,						1						},
		{	1,						maxWorkGroupSizeY+1,	1						},
		{	1,						1,						maxWorkGroupSizeZ+1		},
		{	maxWorkGroupSizeX,		maxWorkGroupSizeY,		maxWorkGroupSizeZ		},
	};

	for (int testCase = 0; testCase < DE_LENGTH_OF_ARRAY(localWorkGroupSizeCases); ++testCase)
	{
		std::ostringstream compShaderSource;

		compShaderSource	<<	shaderVersion << "\n"
							<<	"layout(local_size_x = " << localWorkGroupSizeCases[testCase].x << ", local_size_y = " << localWorkGroupSizeCases[testCase].y << ", local_size_z = " << localWorkGroupSizeCases[testCase].z << ") in;\n"
							<<	"void main (void)\n"
							<<	"{\n"
							<<	"}\n";

		const glu::ComputeSource	compSource(compShaderSource.str());
		glu::ShaderProgram			program(ctx.getRenderContext(), glu::ProgramSources() << compSource);

		if (testCase == DE_LENGTH_OF_ARRAY(localWorkGroupSizeCases)-1)
		{
			bool testFailed = false;
			ctx.beginSection("A compile time or link error is generated if the maximum number of invocations in a single local work group (product of the three dimensions) is greater than GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS.");

			ctx.getLog() << program;
			testFailed = (program.getProgramInfo().linkOk) && (program.getShaderInfo(glu::SHADERTYPE_COMPUTE).compileOk);

			if (testFailed)
			{
				const char* const message("Program was not expected to compile or link.");
				ctx.getLog() << tcu::TestLog::Message << message << tcu::TestLog::EndMessage;
				ctx.fail(message);
			}
		}
		else
		{
			ctx.beginSection("A compile time error is generated if the fixed local group size of the shader in any dimension is greater than the maximum supported.");
			verifyCompileError(ctx, program, glu::SHADERTYPE_COMPUTE);
		}

		ctx.endSection();
	}
}

void invalid_layout_qualifiers (NegativeTestContext& ctx)
{
	const bool				isES32			= glu::contextSupports(ctx.getRenderContext().getType(), glu::ApiType::es(3, 2));
	const char* const		shaderVersion	= isES32
											? getGLSLVersionDeclaration(glu::GLSL_VERSION_320_ES)
											: getGLSLVersionDeclaration(glu::GLSL_VERSION_310_ES);

	{
		std::ostringstream compShaderSource;
		compShaderSource	<<	shaderVersion << "\n"
							<<	"void main (void)\n"
							<<	"{\n"
							<<	"}\n";

		const glu::ComputeSource	compSource(compShaderSource.str());
		glu::ShaderProgram			program(ctx.getRenderContext(), glu::ProgramSources() << compSource);

		ctx.beginSection("A link error is generated if the compute shader program does not contain an input layout qualifier specifying a fixed local group size.");
		verifyLinkError(ctx, program);
		ctx.endSection();
	}

	{
		std::ostringstream compShaderSource;
		compShaderSource	<<	shaderVersion << "\n"
							<<	"layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;\n"
							<<	"layout(local_size_x = 2, local_size_y = 2, local_size_z = 2) in;\n"
							<<	"void main (void)\n"
							<<	"{\n"
							<<	"}\n";

		const glu::ComputeSource	compSource(compShaderSource.str());
		glu::ShaderProgram			program(ctx.getRenderContext(), glu::ProgramSources() << compSource);

		ctx.beginSection("A compile-time error is generated if a local work group size qualifier is declared more than once in the same shader.");
		verifyCompileError(ctx, program, glu::SHADERTYPE_COMPUTE);
		ctx.endSection();
	}

	{
		std::ostringstream compShaderSource;
		compShaderSource	<<	shaderVersion << "\n"
							<<	"out mediump vec4 fragColor;\n"
							<<	"\n"
							<<	"layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;\n"
							<<	"void main (void)\n"
							<<	"{\n"
							<<	"}\n";

		const glu::ComputeSource	compSource(compShaderSource.str());
		glu::ShaderProgram			program(ctx.getRenderContext(), glu::ProgramSources() << compSource);

		ctx.beginSection("A compile-time error is generated if a user defined output variable is declared in a compute shader.");
		verifyCompileError(ctx, program, glu::SHADERTYPE_COMPUTE);
		ctx.endSection();
	}

	{
		std::ostringstream compShaderSource;
		compShaderSource	<<	shaderVersion << "\n"
							<<	"uvec3 gl_NumWorkGroups;\n"
							<<	"uvec3 gl_WorkGroupSize;\n"
							<<	"uvec3 gl_WorkGroupID;\n"
							<<	"uvec3 gl_LocalInvocationID;\n"
							<<	"uvec3 gl_GlobalInvocationID;\n"
							<<	"uvec3 gl_LocalInvocationIndex;\n"
							<<	"\n"
							<<	"layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;\n"
							<<	"void main (void)\n"
							<<	"{\n"
							<<	"}\n";

		const glu::ComputeSource	compSource(compShaderSource.str());
		glu::ShaderProgram			program(ctx.getRenderContext(), glu::ProgramSources() << compSource);

		ctx.beginSection("A compile time or link error is generated if compute shader built-in variables are redeclared.");
		bool testFailed = false;

		tcu::TestLog& log = ctx.getLog();
		log << program;

		testFailed = (program.getProgramInfo().linkOk) && (program.getShaderInfo(glu::SHADERTYPE_COMPUTE).compileOk);

		if (testFailed)
		{
			const char* const message("Program was not expected to compile or link.");
			log << tcu::TestLog::Message << message << tcu::TestLog::EndMessage;
			ctx.fail(message);
		}

		ctx.endSection();
	}
}

void invalid_write_built_in_constants (NegativeTestContext& ctx)
{
	if ((!ctx.isExtensionSupported("GL_EXT_tessellation_shader") && !ctx.isExtensionSupported("GL_OES_tessellation_shader")) ||
	    (!ctx.isExtensionSupported("GL_EXT_geometry_shader") && !ctx.isExtensionSupported("GL_OES_geometry_shader")))
	{
		TCU_THROW(NotSupportedError, "tessellation and geometry shader extensions not supported");
	}

	const bool					isES32			= glu::contextSupports(ctx.getRenderContext().getType(), glu::ApiType::es(3, 2));
	map<string, string>			args;

	args["GLSL_VERSION_STRING"]					=	isES32 ? getGLSLVersionDeclaration(glu::GLSL_VERSION_320_ES) : getGLSLVersionDeclaration(glu::GLSL_VERSION_310_ES);
	args["GLSL_TESS_EXTENSION_STRING"]			=	isES32 ? "" : "#extension GL_EXT_tessellation_shader : require";
	args["COMPUTE_BUILT_IN_CONSTANTS_STRING"]	=	"	gl_MaxComputeWorkGroupCount       = ivec3(65535, 65535, 65535);\n"
													"	gl_MaxComputeWorkGroupCount       = ivec3(1024, 1024, 64);\n"
													"	gl_MaxComputeWorkGroupSize        = ivec3(512);\n"
													"	gl_MaxComputeUniformComponents    = 512;\n"
													"	gl_MaxComputeTextureImageUnits    = 16;\n"
													"	gl_MaxComputeImageUniforms        = 8;\n"
													"	gl_MaxComputeAtomicCounters       = 8;\n"
													"	gl_MaxComputeAtomicCounterBuffers = 1;\n";

	const glu::VertexSource					vertSource		(tcu::StringTemplate(genBuiltInSource(glu::SHADERTYPE_VERTEX)).specialize(args));
	const glu::FragmentSource				fragSource		(tcu::StringTemplate(genBuiltInSource(glu::SHADERTYPE_FRAGMENT)).specialize(args));
	const glu::TessellationControlSource	tessCtrlSource	(tcu::StringTemplate(genBuiltInSource(glu::SHADERTYPE_TESSELLATION_CONTROL)).specialize(args));
	const glu::TessellationEvaluationSource	tessEvalSource	(tcu::StringTemplate(genBuiltInSource(glu::SHADERTYPE_TESSELLATION_EVALUATION)).specialize(args));
	const glu::GeometrySource				geometrySource	(tcu::StringTemplate(genBuiltInSource(glu::SHADERTYPE_GEOMETRY)).specialize(args));
	const glu::ComputeSource				computeSource	(tcu::StringTemplate(genBuiltInSource(glu::SHADERTYPE_COMPUTE)).specialize(args));

	glu::ShaderProgram	vertProgram		(ctx.getRenderContext(), glu::ProgramSources() << glu::ProgramSeparable(true) << vertSource);
	glu::ShaderProgram	fragProgram		(ctx.getRenderContext(), glu::ProgramSources() << glu::ProgramSeparable(true) << fragSource);
	glu::ShaderProgram	tessCtrlProgram	(ctx.getRenderContext(), glu::ProgramSources() << glu::ProgramSeparable(true) << tessCtrlSource);
	glu::ShaderProgram	tessEvalProgram	(ctx.getRenderContext(), glu::ProgramSources() << glu::ProgramSeparable(true) << tessEvalSource);
	glu::ShaderProgram	geometryProgram	(ctx.getRenderContext(), glu::ProgramSources() << glu::ProgramSeparable(true) << geometrySource);
	glu::ShaderProgram	computeProgram	(ctx.getRenderContext(), glu::ProgramSources() << glu::ProgramSeparable(true) << computeSource);

	ctx.beginSection("A compile time is generated if compute built-in constants provided in all shaders are written to.");
	verifyCompileError(ctx, vertProgram, glu::SHADERTYPE_VERTEX);
	verifyCompileError(ctx, fragProgram, glu::SHADERTYPE_FRAGMENT);
	verifyCompileError(ctx, tessCtrlProgram, glu::SHADERTYPE_TESSELLATION_CONTROL);
	verifyCompileError(ctx, tessEvalProgram, glu::SHADERTYPE_TESSELLATION_EVALUATION);
	verifyCompileError(ctx, geometryProgram, glu::SHADERTYPE_GEOMETRY);
	verifyCompileError(ctx, computeProgram, glu::SHADERTYPE_COMPUTE);
	ctx.endSection();
}

} // anonymous

std::vector<FunctionContainer> getNegativeComputeTestFunctions (void)
{
	const FunctionContainer funcs[] =
	{
		{ program_not_active,					"program_not_active",					"Use dispatch commands with no active program"									},
		{ invalid_program_query,				"invalid_program_query",				"Querying GL_COMPUTE_WORK_GROUP_SIZE with glGetProgramiv() on invalid programs"	},
		{ invalid_dispatch_compute_indirect,	"invalid_dispatch_compute_indirect",	"Invalid glDispatchComputeIndirect usage"										},
		{ invalid_maximum_work_group_counts,	"invalid_maximum_work_group_counts",	"Maximum workgroup counts for dispatch commands"								},
		{ invalid_maximum_work_group_sizes,		"invalid_maximum_work_group_sizes",		"Maximum local workgroup sizes declared in compute shaders"						},
		{ invalid_layout_qualifiers,			"invalid_layout_qualifiers",			"Invalid layout qualifiers in compute shaders"									},
		{ invalid_write_built_in_constants,		"invalid_write_built_in_constants",		"Invalid writes to built-in compute shader constants"							},
		{ exceed_uniform_block_limit,			"exceed_uniform_block_limit",			"Link error when shader exceeds GL_MAX_COMPUTE_UNIFORM_BLOCKS"					},
		{ exceed_shader_storage_block_limit,	"exceed_shader_storage_block_limit",	"Link error when shader exceeds GL_MAX_COMPUTE_SHADER_STORAGE_BLOCKS"			},
		{ exceed_texture_image_units_limit,		"exceed_texture_image_units_limit",		"Link error when shader exceeds GL_MAX_COMPUTE_TEXTURE_IMAGE_UNITS"				},
		{ exceed_image_uniforms_limit,			"exceed_image_uniforms_limit",			"Link error when shader exceeds GL_MAX_COMPUTE_IMAGE_UNIFORMS"					},
		{ exceed_shared_memory_size_limit,		"exceed_shared_memory_size_limit",		"Link error when shader exceeds GL_MAX_COMPUTE_SHARED_MEMORY_SIZE"				},
		{ exceed_uniform_components_limit,		"exceed_uniform_components_limit",		"Link error when shader exceeds GL_MAX_COMPUTE_UNIFORM_COMPONENTS"				},
		{ exceed_atomic_counter_buffer_limit,	"exceed_atomic_counter_buffer_limit",	"Link error when shader exceeds GL_MAX_COMPUTE_ATOMIC_COUNTER_BUFFERS"			},
		{ exceed_atomic_counters_limit,			"exceed_atomic_counters_limit",			"Link error when shader exceeds GL_MAX_COMPUTE_ATOMIC_COUNTERS"					},
	};

	return std::vector<FunctionContainer>(DE_ARRAY_BEGIN(funcs), DE_ARRAY_END(funcs));
}

} // NegativeTestShared
} // Functional
} // gles31
} // deqp
