/*-------------------------------------------------------------------------
 * Vulkan CTS Framework
 * --------------------
 *
 * Copyright (c) 2015 Google Inc.
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
 * \brief Shading language (GLSL/HLSL) to SPIR-V.
 *//*--------------------------------------------------------------------*/

#include "vkShaderToSpirV.hpp"
#include "deArrayUtil.hpp"
#include "deSingleton.h"
#include "deMemory.h"
#include "deClock.h"
#include "qpDebugOut.h"

#if defined(DEQP_HAVE_GLSLANG)
#	include "SPIRV/GlslangToSpv.h"
#	include "SPIRV/disassemble.h"
#	include "SPIRV/SPVRemapper.h"
#	include "SPIRV/doc.h"
#	include "glslang/Include/InfoSink.h"
#	include "glslang/Include/ShHandle.h"
#	include "glslang/MachineIndependent/localintermediate.h"
#	include "glslang/Public/ShaderLang.h"
#endif

namespace vk
{

using std::string;
using std::vector;

#if defined(DEQP_HAVE_GLSLANG)

namespace
{

EShLanguage getGlslangStage (glu::ShaderType type)
{
	static const EShLanguage stageMap[] =
	{
		EShLangVertex,
		EShLangFragment,
		EShLangGeometry,
		EShLangTessControl,
		EShLangTessEvaluation,
		EShLangCompute,
	};
	return de::getSizedArrayElement<glu::SHADERTYPE_LAST>(stageMap, type);
}

static volatile deSingletonState	s_glslangInitState	= DE_SINGLETON_STATE_NOT_INITIALIZED;

void initGlslang (void*)
{
	// Main compiler
	glslang::InitializeProcess();

	// SPIR-V disassembly
	spv::Parameterize();
}

void prepareGlslang (void)
{
	deInitSingleton(&s_glslangInitState, initGlslang, DE_NULL);
}

// \todo [2015-06-19 pyry] Specialize these per GLSL version

// Fail compilation if more members are added to TLimits or TBuiltInResource
struct LimitsSizeHelper_s			{ bool m0, m1, m2, m3, m4, m5, m6, m7, m8; };
struct BuiltInResourceSizeHelper_s	{ int m[83]; LimitsSizeHelper_s l; };

DE_STATIC_ASSERT(sizeof(TLimits)			== sizeof(LimitsSizeHelper_s));
DE_STATIC_ASSERT(sizeof(TBuiltInResource)	== sizeof(BuiltInResourceSizeHelper_s));

void getDefaultLimits (TLimits* limits)
{
	limits->nonInductiveForLoops					= true;
	limits->whileLoops								= true;
	limits->doWhileLoops							= true;
	limits->generalUniformIndexing					= true;
	limits->generalAttributeMatrixVectorIndexing	= true;
	limits->generalVaryingIndexing					= true;
	limits->generalSamplerIndexing					= true;
	limits->generalVariableIndexing					= true;
	limits->generalConstantMatrixVectorIndexing		= true;
}

void getDefaultBuiltInResources (TBuiltInResource* builtin)
{
	getDefaultLimits(&builtin->limits);

	builtin->maxLights									= 32;
	builtin->maxClipPlanes								= 6;
	builtin->maxTextureUnits							= 32;
	builtin->maxTextureCoords							= 32;
	builtin->maxVertexAttribs							= 64;
	builtin->maxVertexUniformComponents					= 4096;
	builtin->maxVaryingFloats							= 64;
	builtin->maxVertexTextureImageUnits					= 32;
	builtin->maxCombinedTextureImageUnits				= 80;
	builtin->maxTextureImageUnits						= 32;
	builtin->maxFragmentUniformComponents				= 4096;
	builtin->maxDrawBuffers								= 32;
	builtin->maxVertexUniformVectors					= 128;
	builtin->maxVaryingVectors							= 8;
	builtin->maxFragmentUniformVectors					= 16;
	builtin->maxVertexOutputVectors						= 16;
	builtin->maxFragmentInputVectors					= 15;
	builtin->minProgramTexelOffset						= -8;
	builtin->maxProgramTexelOffset						= 7;
	builtin->maxClipDistances							= 8;
	builtin->maxComputeWorkGroupCountX					= 65535;
	builtin->maxComputeWorkGroupCountY					= 65535;
	builtin->maxComputeWorkGroupCountZ					= 65535;
	builtin->maxComputeWorkGroupSizeX					= 1024;
	builtin->maxComputeWorkGroupSizeY					= 1024;
	builtin->maxComputeWorkGroupSizeZ					= 64;
	builtin->maxComputeUniformComponents				= 1024;
	builtin->maxComputeTextureImageUnits				= 16;
	builtin->maxComputeImageUniforms					= 8;
	builtin->maxComputeAtomicCounters					= 8;
	builtin->maxComputeAtomicCounterBuffers				= 1;
	builtin->maxVaryingComponents						= 60;
	builtin->maxVertexOutputComponents					= 64;
	builtin->maxGeometryInputComponents					= 64;
	builtin->maxGeometryOutputComponents				= 128;
	builtin->maxFragmentInputComponents					= 128;
	builtin->maxImageUnits								= 8;
	builtin->maxCombinedImageUnitsAndFragmentOutputs	= 8;
	builtin->maxCombinedShaderOutputResources			= 8;
	builtin->maxImageSamples							= 0;
	builtin->maxVertexImageUniforms						= 0;
	builtin->maxTessControlImageUniforms				= 0;
	builtin->maxTessEvaluationImageUniforms				= 0;
	builtin->maxGeometryImageUniforms					= 0;
	builtin->maxFragmentImageUniforms					= 8;
	builtin->maxCombinedImageUniforms					= 8;
	builtin->maxGeometryTextureImageUnits				= 16;
	builtin->maxGeometryOutputVertices					= 256;
	builtin->maxGeometryTotalOutputComponents			= 1024;
	builtin->maxGeometryUniformComponents				= 1024;
	builtin->maxGeometryVaryingComponents				= 64;
	builtin->maxTessControlInputComponents				= 128;
	builtin->maxTessControlOutputComponents				= 128;
	builtin->maxTessControlTextureImageUnits			= 16;
	builtin->maxTessControlUniformComponents			= 1024;
	builtin->maxTessControlTotalOutputComponents		= 4096;
	builtin->maxTessEvaluationInputComponents			= 128;
	builtin->maxTessEvaluationOutputComponents			= 128;
	builtin->maxTessEvaluationTextureImageUnits			= 16;
	builtin->maxTessEvaluationUniformComponents			= 1024;
	builtin->maxTessPatchComponents						= 120;
	builtin->maxPatchVertices							= 32;
	builtin->maxTessGenLevel							= 64;
	builtin->maxViewports								= 16;
	builtin->maxVertexAtomicCounters					= 0;
	builtin->maxTessControlAtomicCounters				= 0;
	builtin->maxTessEvaluationAtomicCounters			= 0;
	builtin->maxGeometryAtomicCounters					= 0;
	builtin->maxFragmentAtomicCounters					= 8;
	builtin->maxCombinedAtomicCounters					= 8;
	builtin->maxAtomicCounterBindings					= 1;
	builtin->maxVertexAtomicCounterBuffers				= 0;
	builtin->maxTessControlAtomicCounterBuffers			= 0;
	builtin->maxTessEvaluationAtomicCounterBuffers		= 0;
	builtin->maxGeometryAtomicCounterBuffers			= 0;
	builtin->maxFragmentAtomicCounterBuffers			= 1;
	builtin->maxCombinedAtomicCounterBuffers			= 1;
	builtin->maxAtomicCounterBufferSize					= 16384;
	builtin->maxTransformFeedbackBuffers				= 4;
	builtin->maxTransformFeedbackInterleavedComponents	= 64;
	builtin->maxCullDistances							= 8;
	builtin->maxCombinedClipAndCullDistances			= 8;
	builtin->maxSamples									= 4;
};

int getNumShaderStages (const std::vector<std::string>* sources)
{
	int numShaderStages = 0;

	for (int shaderType = 0; shaderType < glu::SHADERTYPE_LAST; ++shaderType)
	{
		if (!sources[shaderType].empty())
			numShaderStages += 1;
	}

	return numShaderStages;
}

std::string getShaderStageSource (const std::vector<std::string>* sources, const ShaderBuildOptions buildOptions, glu::ShaderType shaderType)
{
	if (sources[shaderType].size() != 1)
		TCU_THROW(InternalError, "Linking multiple compilation units is not supported");

	if ((buildOptions.flags & ShaderBuildOptions::FLAG_USE_STORAGE_BUFFER_STORAGE_CLASS) != 0)
	{
		// Hack to inject #pragma right after first #version statement
		std::string src			= sources[shaderType][0];
		size_t		injectPos	= 0;

		if (de::beginsWith(src, "#version"))
			injectPos = src.find('\n') + 1;

		src.insert(injectPos, "#pragma use_storage_buffer\n");

		return src;
	}
	else
		return sources[shaderType][0];
}

EShMessages getCompileFlags (const ShaderBuildOptions& buildOpts, const ShaderLanguage shaderLanguage)
{
	EShMessages		flags	= (EShMessages)(EShMsgSpvRules | EShMsgVulkanRules);

	if ((buildOpts.flags & ShaderBuildOptions::FLAG_ALLOW_RELAXED_OFFSETS) != 0)
		flags = (EShMessages)(flags | EShMsgHlslOffsets);

	if (shaderLanguage == SHADER_LANGUAGE_HLSL)
		flags = (EShMessages)(flags | EShMsgReadHlsl);

	return flags;
}

} // anonymous

bool compileShaderToSpirV (const std::vector<std::string>* sources, const ShaderBuildOptions& buildOptions, const ShaderLanguage shaderLanguage, std::vector<deUint32>* dst, glu::ShaderProgramInfo* buildInfo)
{
	TBuiltInResource	builtinRes;
	const EShMessages	compileFlags	= getCompileFlags(buildOptions, shaderLanguage);

	if (buildOptions.targetVersion != SPIRV_VERSION_1_0)
		TCU_THROW(InternalError, "Unsupported SPIR-V target version");

	if (getNumShaderStages(sources) > 1)
		TCU_THROW(InternalError, "Linking multiple shader stages into a single SPIR-V binary is not supported");

	prepareGlslang();
	getDefaultBuiltInResources(&builtinRes);

	// \note Compiles only first found shader
	for (int shaderType = 0; shaderType < glu::SHADERTYPE_LAST; shaderType++)
	{
		if (!sources[shaderType].empty())
		{
			const std::string&		srcText				= getShaderStageSource(sources, buildOptions, (glu::ShaderType)shaderType);
			const char*				srcPtrs[]			= { srcText.c_str() };
			const int				srcLengths[]		= { (int)srcText.size() };
			const EShLanguage		shaderStage			= getGlslangStage(glu::ShaderType(shaderType));
			glslang::TShader		shader				(shaderStage);
			glslang::TProgram		glslangProgram;

			shader.setStrings(srcPtrs, DE_LENGTH_OF_ARRAY(srcPtrs));
			glslangProgram.addShader(&shader);

			if (shaderLanguage == SHADER_LANGUAGE_HLSL)
			{
				// Entry point assumed to be named main.
				shader.setEntryPoint("main");
			}

			{
				const deUint64	compileStartTime	= deGetMicroseconds();
				const int		compileRes			= shader.parse(&builtinRes, 110, false, compileFlags);
				glu::ShaderInfo	shaderBuildInfo;

				shaderBuildInfo.type			= (glu::ShaderType)shaderType;
				shaderBuildInfo.source			= srcText;
				shaderBuildInfo.infoLog			= shader.getInfoLog(); // \todo [2015-07-13 pyry] Include debug log?
				shaderBuildInfo.compileTimeUs	= deGetMicroseconds()-compileStartTime;
				shaderBuildInfo.compileOk		= (compileRes != 0);

				buildInfo->shaders.push_back(shaderBuildInfo);
			}

			DE_ASSERT(buildInfo->shaders.size() == 1);
			if (buildInfo->shaders[0].compileOk)
			{
				const deUint64	linkStartTime	= deGetMicroseconds();
				const int		linkRes			= glslangProgram.link(compileFlags);

				buildInfo->program.infoLog		= glslangProgram.getInfoLog(); // \todo [2015-11-05 scygan] Include debug log?
				buildInfo->program.linkOk		= (linkRes != 0);
				buildInfo->program.linkTimeUs	= deGetMicroseconds()-linkStartTime;
			}

			if (buildInfo->program.linkOk)
			{
				const glslang::TIntermediate* const	intermediate	= glslangProgram.getIntermediate(shaderStage);
				glslang::GlslangToSpv(*intermediate, *dst);
			}

			return buildInfo->program.linkOk;
		}
	}

	TCU_THROW(InternalError, "Can't compile empty program");
}

bool compileGlslToSpirV (const GlslSource& program, std::vector<deUint32>* dst, glu::ShaderProgramInfo* buildInfo)
{
	return compileShaderToSpirV(program.sources, program.buildOptions, program.shaderLanguage, dst, buildInfo);
}

bool compileHlslToSpirV (const HlslSource& program, std::vector<deUint32>* dst, glu::ShaderProgramInfo* buildInfo)
{
	return compileShaderToSpirV(program.sources, program.buildOptions, program.shaderLanguage, dst, buildInfo);
}

void stripSpirVDebugInfo (const size_t numSrcInstrs, const deUint32* srcInstrs, std::vector<deUint32>* dst)
{
	spv::spirvbin_t remapper;

	// glslang operates in-place
	dst->resize(numSrcInstrs);
	std::copy(srcInstrs, srcInstrs+numSrcInstrs, dst->begin());
	remapper.remap(*dst, spv::spirvbin_base_t::STRIP);
}

#else // defined(DEQP_HAVE_GLSLANG)

bool compileGlslToSpirV (const GlslSource&, std::vector<deUint32>*, glu::ShaderProgramInfo*)
{
	TCU_THROW(NotSupportedError, "GLSL to SPIR-V compilation not supported (DEQP_HAVE_GLSLANG not defined)");
}

bool compileHlslToSpirV (const HlslSource&, std::vector<deUint32>*, glu::ShaderProgramInfo*)
{
	TCU_THROW(NotSupportedError, "HLSL to SPIR-V compilation not supported (DEQP_HAVE_GLSLANG not defined)");
}

void stripSpirVDebugInfo (const size_t, const deUint32*, std::vector<deUint32>*)
{
	TCU_THROW(NotSupportedError, "SPIR-V stripping not supported (DEQP_HAVE_GLSLANG not defined)");
}

#endif // defined(DEQP_HAVE_GLSLANG)

} // vk
