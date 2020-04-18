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
 * \file  gl4cGlSpirvTests.cpp
 * \brief Conformance tests for the GL_ARB_gl_spirv functionality.
 */ /*-------------------------------------------------------------------*/

#include "gl4cGlSpirvTests.hpp"
#include "deArrayUtil.hpp"
#include "deSingleton.h"
#include "deStringUtil.hpp"
#include "gluContextInfo.hpp"
#include "gluDefs.hpp"
#include "gluShaderProgram.hpp"
#include "gluStrUtil.hpp"
#include "glwEnums.hpp"
#include "glwFunctions.hpp"
#include "tcuRenderTarget.hpp"
#include "tcuResource.hpp"
#include "tcuTestLog.hpp"

#if defined DEQP_HAVE_GLSLANG
#include "SPIRV/GlslangToSpv.h"
#include "SPIRV/disassemble.h"
#include "SPIRV/doc.h"
#include "glslang/MachineIndependent/localintermediate.h"
#include "glslang/Public/ShaderLang.h"
#endif // DEQP_HAVE_GLSLANG

#if defined DEQP_HAVE_SPIRV_TOOLS
#include "spirv-tools/libspirv.hpp"
#include "spirv-tools/optimizer.hpp"
#endif // DEQP_HAVE_SPIRV_TOOLS

using namespace glu;
using namespace glw;

namespace gl4cts
{

namespace glslangUtils
{

#if defined DEQP_HAVE_GLSLANG

EShLanguage getGlslangStage(glu::ShaderType type)
{
	static const EShLanguage stageMap[] = {
		EShLangVertex, EShLangFragment, EShLangGeometry, EShLangTessControl, EShLangTessEvaluation, EShLangCompute,
	};

	return de::getSizedArrayElement<glu::SHADERTYPE_LAST>(stageMap, type);
}

static volatile deSingletonState s_glslangInitState = DE_SINGLETON_STATE_NOT_INITIALIZED;

void initGlslang(void*)
{
	// Main compiler
	glslang::InitializeProcess();

	// SPIR-V disassembly
	spv::Parameterize();
}

void prepareGlslang(void)
{
	deInitSingleton(&s_glslangInitState, initGlslang, DE_NULL);
}

void getDefaultLimits(TLimits* limits)
{
	limits->nonInductiveForLoops				 = true;
	limits->whileLoops							 = true;
	limits->doWhileLoops						 = true;
	limits->generalUniformIndexing				 = true;
	limits->generalAttributeMatrixVectorIndexing = true;
	limits->generalVaryingIndexing				 = true;
	limits->generalSamplerIndexing				 = true;
	limits->generalVariableIndexing				 = true;
	limits->generalConstantMatrixVectorIndexing  = true;
}

void getDefaultBuiltInResources(TBuiltInResource* builtin)
{
	getDefaultLimits(&builtin->limits);

	builtin->maxLights								   = 32;
	builtin->maxClipPlanes							   = 6;
	builtin->maxTextureUnits						   = 32;
	builtin->maxTextureCoords						   = 32;
	builtin->maxVertexAttribs						   = 64;
	builtin->maxVertexUniformComponents				   = 4096;
	builtin->maxVaryingFloats						   = 64;
	builtin->maxVertexTextureImageUnits				   = 32;
	builtin->maxCombinedTextureImageUnits			   = 80;
	builtin->maxTextureImageUnits					   = 32;
	builtin->maxFragmentUniformComponents			   = 4096;
	builtin->maxDrawBuffers							   = 32;
	builtin->maxVertexUniformVectors				   = 128;
	builtin->maxVaryingVectors						   = 8;
	builtin->maxFragmentUniformVectors				   = 16;
	builtin->maxVertexOutputVectors					   = 16;
	builtin->maxFragmentInputVectors				   = 15;
	builtin->minProgramTexelOffset					   = -8;
	builtin->maxProgramTexelOffset					   = 7;
	builtin->maxClipDistances						   = 8;
	builtin->maxComputeWorkGroupCountX				   = 65535;
	builtin->maxComputeWorkGroupCountY				   = 65535;
	builtin->maxComputeWorkGroupCountZ				   = 65535;
	builtin->maxComputeWorkGroupSizeX				   = 1024;
	builtin->maxComputeWorkGroupSizeY				   = 1024;
	builtin->maxComputeWorkGroupSizeZ				   = 64;
	builtin->maxComputeUniformComponents			   = 1024;
	builtin->maxComputeTextureImageUnits			   = 16;
	builtin->maxComputeImageUniforms				   = 8;
	builtin->maxComputeAtomicCounters				   = 8;
	builtin->maxComputeAtomicCounterBuffers			   = 1;
	builtin->maxVaryingComponents					   = 60;
	builtin->maxVertexOutputComponents				   = 64;
	builtin->maxGeometryInputComponents				   = 64;
	builtin->maxGeometryOutputComponents			   = 128;
	builtin->maxFragmentInputComponents				   = 128;
	builtin->maxImageUnits							   = 8;
	builtin->maxCombinedImageUnitsAndFragmentOutputs   = 8;
	builtin->maxCombinedShaderOutputResources		   = 8;
	builtin->maxImageSamples						   = 0;
	builtin->maxVertexImageUniforms					   = 0;
	builtin->maxTessControlImageUniforms			   = 0;
	builtin->maxTessEvaluationImageUniforms			   = 0;
	builtin->maxGeometryImageUniforms				   = 0;
	builtin->maxFragmentImageUniforms				   = 8;
	builtin->maxCombinedImageUniforms				   = 8;
	builtin->maxGeometryTextureImageUnits			   = 16;
	builtin->maxGeometryOutputVertices				   = 256;
	builtin->maxGeometryTotalOutputComponents		   = 1024;
	builtin->maxGeometryUniformComponents			   = 1024;
	builtin->maxGeometryVaryingComponents			   = 64;
	builtin->maxTessControlInputComponents			   = 128;
	builtin->maxTessControlOutputComponents			   = 128;
	builtin->maxTessControlTextureImageUnits		   = 16;
	builtin->maxTessControlUniformComponents		   = 1024;
	builtin->maxTessControlTotalOutputComponents	   = 4096;
	builtin->maxTessEvaluationInputComponents		   = 128;
	builtin->maxTessEvaluationOutputComponents		   = 128;
	builtin->maxTessEvaluationTextureImageUnits		   = 16;
	builtin->maxTessEvaluationUniformComponents		   = 1024;
	builtin->maxTessPatchComponents					   = 120;
	builtin->maxPatchVertices						   = 32;
	builtin->maxTessGenLevel						   = 64;
	builtin->maxViewports							   = 16;
	builtin->maxVertexAtomicCounters				   = 0;
	builtin->maxTessControlAtomicCounters			   = 0;
	builtin->maxTessEvaluationAtomicCounters		   = 0;
	builtin->maxGeometryAtomicCounters				   = 0;
	builtin->maxFragmentAtomicCounters				   = 8;
	builtin->maxCombinedAtomicCounters				   = 8;
	builtin->maxAtomicCounterBindings				   = 1;
	builtin->maxVertexAtomicCounterBuffers			   = 0;
	builtin->maxTessControlAtomicCounterBuffers		   = 0;
	builtin->maxTessEvaluationAtomicCounterBuffers	 = 0;
	builtin->maxGeometryAtomicCounterBuffers		   = 0;
	builtin->maxFragmentAtomicCounterBuffers		   = 1;
	builtin->maxCombinedAtomicCounterBuffers		   = 1;
	builtin->maxAtomicCounterBufferSize				   = 16384;
	builtin->maxTransformFeedbackBuffers			   = 4;
	builtin->maxTransformFeedbackInterleavedComponents = 64;
	builtin->maxCullDistances						   = 8;
	builtin->maxCombinedClipAndCullDistances		   = 8;
	builtin->maxSamples								   = 4;
};

bool compileGlslToSpirV(tcu::TestLog& log, std::string source, glu::ShaderType type, ShaderBinaryDataType* dst)
{
	TBuiltInResource builtinRes;

	prepareGlslang();
	getDefaultBuiltInResources(&builtinRes);

	const EShLanguage shaderStage = getGlslangStage(type);

	glslang::TShader  shader(shaderStage);
	glslang::TProgram program;

	const char* src[] = { source.c_str() };

	shader.setStrings(src, 1);
	program.addShader(&shader);

	const int compileRes = shader.parse(&builtinRes, 100, false, EShMsgSpvRules);
	if (compileRes != 0)
	{
		const int linkRes = program.link(EShMsgSpvRules);

		if (linkRes != 0)
		{
			const glslang::TIntermediate* const intermediate = program.getIntermediate(shaderStage);
			glslang::GlslangToSpv(*intermediate, *dst);

			return true;
		}
		else
		{
			log << tcu::TestLog::Message << "Program linking error:\n"
				<< program.getInfoLog() << "\n"
				<< "Source:\n"
				<< source << "\n"
				<< tcu::TestLog::EndMessage;
		}
	}
	else
	{
		log << tcu::TestLog::Message << "Shader compilation error:\n"
			<< shader.getInfoLog() << "\n"
			<< "Source:\n"
			<< source << "\n"
			<< tcu::TestLog::EndMessage;
	}

	return false;
}

#else // DEQP_HAVE_GLSLANG

bool compileGlslToSpirV(tcu::TestLog& log, std::string source, glu::ShaderType type, ShaderBinaryDataType* dst)
{
	DE_UNREF(log);
	DE_UNREF(source);
	DE_UNREF(type);
	DE_UNREF(dst);

	TCU_THROW(InternalError, "Glslang not available.");

	return false;
}

#endif // DEQP_HAVE_GLSLANG

#if defined DEQP_HAVE_SPIRV_TOOLS

void consumer(spv_message_level_t, const char*, const spv_position_t&, const char* m)
{
	std::cerr << "error: " << m << std::endl;
}

void spirvAssemble(ShaderBinaryDataType& dst, const std::string& src)
{
	spvtools::SpirvTools core(SPV_ENV_OPENGL_4_5);

	core.SetMessageConsumer(consumer);

	if (!core.Assemble(src, &dst))
		TCU_THROW(InternalError, "Failed to assemble Spir-V source.");
}

void spirvDisassemble(std::string& dst, const ShaderBinaryDataType& src)
{
	spvtools::SpirvTools core(SPV_ENV_OPENGL_4_5);

	core.SetMessageConsumer(consumer);

	if (!core.Disassemble(src, &dst))
		TCU_THROW(InternalError, "Failed to disassemble Spir-V module.");
}

bool spirvValidate(ShaderBinaryDataType& dst, bool throwOnError)
{
	spvtools::SpirvTools core(SPV_ENV_OPENGL_4_5);

	if (throwOnError)
		core.SetMessageConsumer(consumer);

	if (!core.Validate(dst))
	{
		if (throwOnError)
			TCU_THROW(InternalError, "Failed to validate Spir-V module.");
		return false;
	}

	return true;
}

#else //DEQP_HAVE_SPIRV_TOOLS

void spirvAssemble(ShaderBinaryDataType& dst, const std::string& src)
{
	DE_UNREF(dst);
	DE_UNREF(src);

	TCU_THROW(InternalError, "Spirv-tools not available.");
}

void spirvDisassemble(std::string& dst, ShaderBinaryDataType& src)
{
	DE_UNREF(dst);
	DE_UNREF(src);

	TCU_THROW(InternalError, "Spirv-tools not available.");
}

bool spirvValidate(ShaderBinaryDataType& dst, bool throwOnError)
{
	DE_UNREF(dst);
	DE_UNREF(throwOnError);

	TCU_THROW(InternalError, "Spirv-tools not available.");
}

#endif // DEQP_HAVE_SPIRV_TOOLS

ShaderBinary makeSpirV(tcu::TestLog& log, ShaderSource source)
{
	ShaderBinary binary;

	if (!glslangUtils::compileGlslToSpirV(log, source.source, source.shaderType, &binary.binary))
		TCU_THROW(InternalError, "Failed to convert GLSL to Spir-V");

	binary << source.shaderType << "main";

	return binary;
}

/** Verifying if GLSL to SpirV mapping was performed correctly
 *
 * @param glslSource       GLSL shader template
 * @param spirVSource      SpirV disassembled source
 * @param mappings         Glsl to SpirV mappings vector
 * @param anyOf            any occurence indicator
 *
 * @return true if GLSL code occurs as many times as all of SpirV code for each mapping if anyOf is false
 *         or true if SpirV code occurs at least once if GLSL code found, false otherwise.
 **/
bool verifyMappings(std::string glslSource, std::string spirVSource, SpirVMapping& mappings, bool anyOf)
{
	std::vector<std::string> spirVSourceLines = de::splitString(spirVSource, '\n');

	// Iterate through all glsl functions
	for (SpirVMapping::iterator it = mappings.begin(); it != mappings.end(); it++)
	{
		int glslCodeCount  = 0;
		int spirVCodeCount = 0;

		// To avoid finding functions with similar names (ie. "cos", "acos", "cosh")
		// add characteristic characters that delimits finding results
		std::string glslCode = it->first;

		// Count GLSL code occurrences in GLSL source
		size_t codePosition = glslSource.find(glslCode);
		while (codePosition != std::string::npos)
		{
			glslCodeCount++;
			codePosition = glslSource.find(glslCode, codePosition + 1);
		}

		if (glslCodeCount > 0)
		{
			// Count all SpirV code variants occurrences in SpirV source
			for (int s = 0; s < it->second.size(); ++s)
			{
				std::vector<std::string> spirVCodes = de::splitString(it->second[s], ' ');

				for (int v = 0; v < spirVSourceLines.size(); ++v)
				{
					std::vector<std::string> spirVLineCodes = de::splitString(spirVSourceLines[v], ' ');

					bool matchAll = true;
					for (int j = 0; j < spirVCodes.size(); ++j)
					{
						bool match = false;
						for (int i = 0; i < spirVLineCodes.size(); ++i)
						{
							if (spirVLineCodes[i] == spirVCodes[j])
								match = true;
						}

						matchAll = matchAll && match;
					}

					if (matchAll)
						spirVCodeCount++;
				}
			}

			// Check if both counts match
			if (anyOf && (glslCodeCount > 0 && spirVCodeCount == 0))
				return false;
			else if (!anyOf && glslCodeCount != spirVCodeCount)
				return false;
		}
	}

	return true;
}

} // namespace glslangUtils

namespace commonUtils
{

void writeSpirV(const char* filename, ShaderBinary binary)
{
	FILE* file = fopen(filename, "wb");
	if (file)
	{
		// As one binary could be associated with many shader objects it should be stored either a type of each shader
		// This will be extended in the future
		deUint8 count = (deUint8)binary.shaderTypes.size();
		fwrite((void*)&count, 1, 1, file);
		for (int i = 0; i < binary.shaderTypes.size(); ++i)
		{
			fwrite((void*)&binary.shaderTypes[i], 1, sizeof(ShaderType), file);

			if (count > 1)
			{
				deUint8 strLen = (deUint8)binary.shaderEntryPoints[i].size();
				fwrite((void*)&strLen, 1, 1, file);
				fwrite((void*)binary.shaderEntryPoints[i].data(), 1, strLen, file);
			}
		}

		fwrite((void*)binary.binary.data(), 1, binary.binary.size() * 4, file);
		fclose(file);
	}
}

ShaderBinary readSpirV(tcu::Resource* resource)
{
	ShaderBinary binary;
	if (!resource)
		return binary;

	// As one binary could be associated with many shader objects it should be stored either a type of each shader
	deUint8 count;
	resource->read(&count, 1);
	binary.shaderTypes.resize(count);
	binary.shaderEntryPoints.resize(count);
	for (int i = 0; i < binary.shaderTypes.size(); ++i)
	{
		resource->read((deUint8*)&binary.shaderTypes[i], sizeof(ShaderType));

		if (count > 1)
		{
			deUint8 strLen;
			resource->read(&strLen, 1);

			binary.shaderEntryPoints[i].resize(strLen);
			resource->read((deUint8*)binary.shaderEntryPoints[i].data(), strLen);
		}
		else
			binary.shaderEntryPoints[i] = "main";
	}

	binary.binary.resize((resource->getSize() - resource->getPosition()) / sizeof(deUint32));
	resource->read((deUint8*)binary.binary.data(), binary.binary.size() * sizeof(deUint32));

	return binary;
}

/** Replace all occurance of <token> with <text> in <string>
 *
 * @param token           Token string
 * @param text            String th at will be used as replacement for <token>
 * @param string          String to work on
 **/
void replaceToken(const GLchar* token, const GLchar* text, std::string& string)
{
	const size_t text_length  = strlen(text);
	const size_t token_length = strlen(token);

	size_t token_position;
	while ((token_position = string.find(token, 0)) != std::string::npos)
	{
		string.replace(token_position, token_length, text, text_length);
	}
}

bool compareUintColors(const GLuint inColor, const GLuint refColor, const int epsilon)
{
	int r1 = (inColor & 0xFF);
	int g1 = ((inColor >> 8) & 0xFF);
	int b1 = ((inColor >> 16) & 0xFF);
	int a1 = ((inColor >> 24) & 0xFF);

	int r2 = (refColor & 0xFF);
	int g2 = ((refColor >> 8) & 0xFF);
	int b2 = ((refColor >> 16) & 0xFF);
	int a2 = ((refColor >> 24) & 0xFF);

	if (r1 >= r2 - epsilon && r1 <= r2 + epsilon && g1 >= g2 - epsilon && g1 <= g2 + epsilon && b1 >= b2 - epsilon &&
		b1 <= b2 + epsilon)
	{
		return true;
	}

	return false;
}

bool checkGlSpirvSupported(deqp::Context& m_context)
{
	bool is_at_least_gl_46 = (glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::core(4, 6)));
	bool is_arb_gl_spirv = m_context.getContextInfo().isExtensionSupported("GL_ARB_gl_spirv");

	if ((!is_at_least_gl_46) && (!is_arb_gl_spirv))
		TCU_THROW(NotSupportedError, "GL_ARB_gl_spirv is not supported");
}
} // namespace commonUtils

/** Constructor.
 *
 *  @param context     Rendering context
 *  @param name        Test name
 *  @param description Test description
 */
SpirvModulesPositiveTest::SpirvModulesPositiveTest(deqp::Context& context)
	: TestCase(context, "spirv_modules_positive_test",
			   "Test verifies if using SPIR-V modules for each shader stage works as expected")
{
	/* Left blank intentionally */
}

/** Stub init method */
void SpirvModulesPositiveTest::init()
{
	commonUtils::checkGlSpirvSupported(m_context);

	m_vertex = "#version 450\n"
			   "\n"
			   "layout (location = 0) in vec3 position;\n"
			   "\n"
			   "layout (location = 1) out vec4 vColor;\n"
			   "\n"
			   "void main()\n"
			   "{\n"
			   "    gl_Position = vec4(position, 1.0);\n"
			   "    vColor = vec4(0.0, 0.0, 0.0, 1.0);\n"
			   "}\n";

	m_tesselationCtrl = "#version 450\n"
						"\n"
						"layout (vertices = 3) out;\n"
						"\n"
						"layout (location = 1) in vec4 vColor[];\n"
						"layout (location = 2) out vec4 tcColor[];\n"
						"\n"
						"void main()\n"
						"{\n"
						"    tcColor[gl_InvocationID] = vColor[gl_InvocationID];\n"
						"    tcColor[gl_InvocationID].r = 1.0;\n"
						"\n"
						"    if (gl_InvocationID == 0) {\n"
						"        gl_TessLevelOuter[0] = 1.0;\n"
						"        gl_TessLevelOuter[1] = 1.0;\n"
						"        gl_TessLevelOuter[2] = 1.0;\n"
						"        gl_TessLevelInner[0] = 1.0;\n"
						"    }\n"
						"\n"
						"    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;\n"
						"}\n";

	m_tesselationEval = "#version 450\n"
						"\n"
						"layout (triangles) in;\n"
						"\n"
						"layout (location = 2) in vec4 tcColor[];\n"
						"layout (location = 3) out vec4 teColor;\n"
						"\n"
						"void main()\n"
						"{\n"
						"    teColor = tcColor[0];\n"
						"    teColor.g = 1.0;\n"
						"\n"
						"    gl_Position = gl_TessCoord.x * gl_in[0].gl_Position +\n"
						"                  gl_TessCoord.y * gl_in[1].gl_Position +\n"
						"                  gl_TessCoord.z * gl_in[2].gl_Position;\n"
						"}\n";

	m_geometry = "#version 450\n"
				 "\n"
				 "layout (triangles) in;\n"
				 "layout (triangle_strip, max_vertices = 3) out;\n"
				 "\n"
				 "layout (location = 3) in vec4 teColor[];\n"
				 "layout (location = 4) out vec4 gColor;\n"
				 "\n"
				 "void main()\n"
				 "{\n"
				 "    gColor = teColor[0];\n"
				 "    gColor.b = 1.0;\n"
				 "\n"
				 "    for (int i = 0; i < 3; ++i) {\n"
				 "        gl_Position = gl_in[i].gl_Position;\n"
				 "        EmitVertex();\n"
				 "    }\n"
				 "    EndPrimitive();\n"
				 "}\n";

	m_fragment = "#version 450\n"
				 "\n"
				 "layout (location = 4) in vec4 gColor;\n"
				 "layout (location = 0) out vec4 fColor;\n"
				 "\n"
				 "void main()\n"
				 "{\n"
				 "    fColor = gColor;\n"
				 "}\n";

	const Functions& gl = m_context.getRenderContext().getFunctions();

	gl.genTextures(1, &m_texture);
	GLU_EXPECT_NO_ERROR(gl.getError(), "genTextures");
	gl.bindTexture(GL_TEXTURE_2D, m_texture);
	GLU_EXPECT_NO_ERROR(gl.getError(), "bindTexture");
	gl.texStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, 32, 32);
	GLU_EXPECT_NO_ERROR(gl.getError(), "texStorage2D");

	gl.genFramebuffers(1, &m_fbo);
	GLU_EXPECT_NO_ERROR(gl.getError(), "genFramenuffers");
	gl.bindFramebuffer(GL_FRAMEBUFFER, m_fbo);
	GLU_EXPECT_NO_ERROR(gl.getError(), "bindFramebuffer");
	gl.framebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_texture, 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "framebufferTexture2D");

	gl.viewport(0, 0, 32, 32);
	GLU_EXPECT_NO_ERROR(gl.getError(), "viewport");
}

/** Stub de-init method */
void SpirvModulesPositiveTest::deinit()
{
	const Functions& gl = m_context.getRenderContext().getFunctions();

	if (m_fbo)
	{
		gl.deleteFramebuffers(1, &m_fbo);
		GLU_EXPECT_NO_ERROR(gl.getError(), "deleteFramebuffers");
	}
	if (m_texture)
	{
		gl.deleteTextures(1, &m_texture);
		GLU_EXPECT_NO_ERROR(gl.getError(), "deleteTextures");
	}
}

/** Executes test iteration.
 *
 *  @return Returns STOP when test has finished executing, CONTINUE if more iterations are needed.
 */
tcu::TestNode::IterateResult SpirvModulesPositiveTest::iterate()
{
	const Functions& gl = m_context.getRenderContext().getFunctions();

	const GLfloat vertices[] = { -1.0f, -1.0f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f };

	GLuint vao;
	gl.genVertexArrays(1, &vao);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenVertexArrays");
	gl.bindVertexArray(vao);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindVertexArray");

	GLuint vbo;
	gl.genBuffers(1, &vbo);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenBuffers");
	gl.bindBuffer(GL_ARRAY_BUFFER, vbo);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer");

	gl.bufferData(GL_ARRAY_BUFFER, 9 * sizeof(GLfloat), (GLvoid*)vertices, GL_DYNAMIC_DRAW);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferData");

	enum Iterates
	{
		ITERATE_GLSL,
		ITERATE_SPIRV,
		ITERATE_LAST
	};

	deUint32 outputs[ITERATE_LAST];
	for (int it = ITERATE_GLSL; it < ITERATE_LAST; ++it)
	{
		ShaderProgram* program = DE_NULL;
		if (it == ITERATE_GLSL)
		{
			ProgramSources sources;
			sources << VertexSource(m_vertex);
			sources << TessellationControlSource(m_tesselationCtrl);
			sources << TessellationEvaluationSource(m_tesselationEval);
			sources << GeometrySource(m_geometry);
			sources << FragmentSource(m_fragment);
			program = new ShaderProgram(gl, sources);
		}
		else if (it == ITERATE_SPIRV)
		{
#if defined					DEQP_HAVE_GLSLANG
			ProgramBinaries binaries;
			binaries << glslangUtils::makeSpirV(m_context.getTestContext().getLog(), VertexSource(m_vertex));
			binaries << glslangUtils::makeSpirV(m_context.getTestContext().getLog(),
												TessellationControlSource(m_tesselationCtrl));
			binaries << glslangUtils::makeSpirV(m_context.getTestContext().getLog(),
												TessellationEvaluationSource(m_tesselationEval));
			binaries << glslangUtils::makeSpirV(m_context.getTestContext().getLog(), GeometrySource(m_geometry));
			binaries << glslangUtils::makeSpirV(m_context.getTestContext().getLog(), FragmentSource(m_fragment));
			program = new ShaderProgram(gl, binaries);
#else  // DEQP_HAVE_GLSLANG
			tcu::Archive&   archive = m_testCtx.getArchive();
			ProgramBinaries binaries;
			binaries << commonUtils::readSpirV(archive.getResource("spirv/modules_positive/vertex.nspv"));
			binaries << commonUtils::readSpirV(archive.getResource("spirv/modules_positive/tess_control.nspv"));
			binaries << commonUtils::readSpirV(archive.getResource("spirv/modules_positive/tess_evaluation.nspv"));
			binaries << commonUtils::readSpirV(archive.getResource("spirv/modules_positive/geometry.nspv"));
			binaries << commonUtils::readSpirV(archive.getResource("spirv/modules_positive/fragment.nspv"));
			program		  = new ShaderProgram(gl, binaries);
#endif // DEQP_HAVE_GLSLANG
		}

		if (!program->isOk())
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "Shader build failed.\n"
							   << "Vertex: " << program->getShaderInfo(SHADERTYPE_VERTEX).infoLog << "\n"
							   << m_vertex << "\n"
							   << "TesselationCtrl: " << program->getShaderInfo(SHADERTYPE_TESSELLATION_CONTROL).infoLog
							   << "\n"
							   << m_tesselationCtrl << "\n"
							   << "TesselationEval: "
							   << program->getShaderInfo(SHADERTYPE_TESSELLATION_EVALUATION).infoLog << "\n"
							   << m_tesselationEval << "\n"
							   << "Geometry: " << program->getShaderInfo(SHADERTYPE_GEOMETRY).infoLog << "\n"
							   << m_geometry << "\n"
							   << "Fragment: " << program->getShaderInfo(SHADERTYPE_FRAGMENT).infoLog << "\n"
							   << m_fragment << "\n"
							   << "Program: " << program->getProgramInfo().infoLog << tcu::TestLog::EndMessage;

			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
			return STOP;
		}

		gl.useProgram(program->getProgram());
		GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram");

		gl.clearColor(0.0f, 0.0f, 0.0f, 1.0f);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glClearColor");
		gl.clear(GL_COLOR_BUFFER_BIT);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glClear");

		gl.enableVertexAttribArray(0);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glEnableVertexAttribArray");

		gl.vertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, DE_NULL);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glVertexAttribPointer");

		gl.patchParameteri(GL_PATCH_VERTICES, 3);
		gl.drawArrays(GL_PATCHES, 0, 3);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawArray");

		gl.disableVertexAttribArray(0);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glDisableVertexAttribArray");

		gl.readPixels(16, 16, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, (GLvoid*)&outputs[it]);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glReadPixels");

		if (program)
			delete program;
	}

	if (vbo)
	{
		gl.deleteBuffers(1, &vbo);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glDeleteBuffers");
	}

	if (vao)
	{
		gl.deleteVertexArrays(1, &vao);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glDeleteVertexArrays");
	}

	if ((outputs[ITERATE_GLSL] & outputs[ITERATE_SPIRV]) != 0xFFFFFFFF)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
		m_testCtx.getLog() << tcu::TestLog::Message << "Wrong output color read from framebuffer.\n"
						   << "GLSL: " << outputs[ITERATE_GLSL] << ", SPIR-V: " << outputs[ITERATE_SPIRV]
						   << "Expected: " << (deUint32)0xFFFFFFFF << tcu::TestLog::EndMessage;
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
SpirvShaderBinaryMultipleShaderObjectsTest::SpirvShaderBinaryMultipleShaderObjectsTest(deqp::Context& context)
	: TestCase(context, "spirv_modules_shader_binary_multiple_shader_objects_test",
			   "Test verifies if one binary module can be associated with multiple shader objects.")
{
	/* Left blank intentionally */
}

/** Stub init method */
void SpirvShaderBinaryMultipleShaderObjectsTest::init()
{
	commonUtils::checkGlSpirvSupported(m_context);

	m_spirv = "OpCapability Shader\n"
			  "%1 = OpExtInstImport \"GLSL.std.450\"\n"
			  "OpMemoryModel Logical GLSL450\n"
			  "OpEntryPoint Vertex %mainv \"mainv\" %_ %position %gl_VertexID %gl_InstanceID\n"
			  "OpEntryPoint Fragment %mainf \"mainf\" %fColor\n"
			  "OpSource GLSL 450\n"
			  "OpName %mainv \"mainv\"\n"
			  "OpName %mainf \"mainf\"\n"
			  "OpName %gl_PerVertex \"gl_PerVertex\"\n"
			  "OpMemberName %gl_PerVertex 0 \"gl_Position\"\n"
			  "OpMemberName %gl_PerVertex 1 \"gl_PointSize\"\n"
			  "OpMemberName %gl_PerVertex 2 \"gl_ClipDistance\"\n"
			  "OpMemberName %gl_PerVertex 3 \"gl_CullDistance\"\n"
			  "OpName %_ \"\"\n"
			  "OpName %position \"position\"\n"
			  "OpName %gl_VertexID \"gl_VertexID\"\n"
			  "OpName %gl_InstanceID \"gl_InstanceID\"\n"
			  "OpMemberDecorate %gl_PerVertex 0 BuiltIn Position\n"
			  "OpMemberDecorate %gl_PerVertex 1 BuiltIn PointSize\n"
			  "OpMemberDecorate %gl_PerVertex 2 BuiltIn ClipDistance\n"
			  "OpMemberDecorate %gl_PerVertex 3 BuiltIn CullDistance\n"
			  "OpDecorate %gl_PerVertex Block\n"
			  "OpDecorate %position Location 0\n"
			  "OpDecorate %gl_VertexID BuiltIn VertexId\n"
			  "OpDecorate %gl_InstanceID BuiltIn InstanceId\n"
			  "OpDecorate %fColor Location 0\n"
			  "%void = OpTypeVoid\n"
			  "%3 = OpTypeFunction %void\n"
			  "%float = OpTypeFloat 32\n"
			  "%v4float = OpTypeVector %float 4\n"
			  "%uint = OpTypeInt 32 0\n"
			  "%uint_1 = OpConstant %uint 1\n"
			  "%_arr_float_uint_1 = OpTypeArray %float %uint_1\n"
			  "%gl_PerVertex = OpTypeStruct %v4float %float %_arr_float_uint_1 %_arr_float_uint_1\n"
			  "%_ptr_Output_gl_PerVertex = OpTypePointer Output %gl_PerVertex\n"
			  "%_ = OpVariable %_ptr_Output_gl_PerVertex Output\n"
			  "%int = OpTypeInt 32 1\n"
			  "%int_0 = OpConstant %int 0\n"
			  "%v3float = OpTypeVector %float 3\n"
			  "%_ptr_Input_v3float = OpTypePointer Input %v3float\n"
			  "%position = OpVariable %_ptr_Input_v3float Input\n"
			  "%float_1 = OpConstant %float 1\n"
			  "%_ptr_Output_v4float = OpTypePointer Output %v4float\n"
			  "%_ptr_Input_int = OpTypePointer Input %int\n"
			  "%gl_VertexID = OpVariable %_ptr_Input_int Input\n"
			  "%gl_InstanceID = OpVariable %_ptr_Input_int Input\n"
			  "%fColor = OpVariable %_ptr_Output_v4float Output\n"
			  "%fVec4_1 = OpConstantComposite %v4float %float_1 %float_1 %float_1 %float_1\n"
			  "\n"
			  "%mainv = OpFunction %void None %3\n"
			  "%5 = OpLabel\n"
			  "%19 = OpLoad %v3float %position\n"
			  "%21 = OpCompositeExtract %float %19 0\n"
			  "%22 = OpCompositeExtract %float %19 1\n"
			  "%23 = OpCompositeExtract %float %19 2\n"
			  "%24 = OpCompositeConstruct %v4float %21 %22 %23 %float_1\n"
			  "%26 = OpAccessChain %_ptr_Output_v4float %_ %int_0\n"
			  "OpStore %26 %24\n"
			  "OpReturn\n"
			  "OpFunctionEnd\n"
			  "\n"
			  "%mainf = OpFunction %void None %3\n"
			  "%32 = OpLabel\n"
			  "OpStore %fColor %fVec4_1\n"
			  "OpReturn\n"
			  "OpFunctionEnd\n";
}

/** Stub init method */
void SpirvShaderBinaryMultipleShaderObjectsTest::deinit()
{
}

/** Executes test iteration.
 *
 *  @return Returns STOP when test has finished executing, CONTINUE if more iterations are needed.
 */
tcu::TestNode::IterateResult SpirvShaderBinaryMultipleShaderObjectsTest::iterate()
{
	const Functions& gl = m_context.getRenderContext().getFunctions();

	const GLfloat vertices[] = { -1.0f, -1.0f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f };

	GLuint texture;
	GLuint fbo;

	gl.genTextures(1, &texture);
	GLU_EXPECT_NO_ERROR(gl.getError(), "genTextures");
	gl.bindTexture(GL_TEXTURE_2D, texture);
	GLU_EXPECT_NO_ERROR(gl.getError(), "bindTexture");
	gl.texStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, 32, 32);
	GLU_EXPECT_NO_ERROR(gl.getError(), "texStorage2D");

	gl.genFramebuffers(1, &fbo);
	GLU_EXPECT_NO_ERROR(gl.getError(), "genFramenuffers");
	gl.bindFramebuffer(GL_FRAMEBUFFER, fbo);
	GLU_EXPECT_NO_ERROR(gl.getError(), "bindFramebuffer");
	gl.framebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "framebufferTexture2D");

	GLuint vao;
	gl.genVertexArrays(1, &vao);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenVertexArrays");
	gl.bindVertexArray(vao);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindVertexArray");

	GLuint vbo;
	gl.genBuffers(1, &vbo);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenBuffers");
	gl.bindBuffer(GL_ARRAY_BUFFER, vbo);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer");

	gl.bufferData(GL_ARRAY_BUFFER, 9 * sizeof(GLfloat), (GLvoid*)vertices, GL_DYNAMIC_DRAW);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferData");

#if DEQP_HAVE_SPIRV_TOOLS
	ShaderBinary binary;
	binary << SHADERTYPE_VERTEX << "mainv";
	binary << SHADERTYPE_FRAGMENT << "mainf";

	glslangUtils::spirvAssemble(binary.binary, m_spirv);
	glslangUtils::spirvValidate(binary.binary, true);
#else  // DEQP_HAVE_SPIRV_TOOLS
	tcu::Archive& archive = m_testCtx.getArchive();
	ShaderBinary  binary  = commonUtils::readSpirV(
		archive.getResource("spirv/spirv_modules_shader_binary_multiple_shader_objects/binary.nspv"));
#endif // DEQP_HAVE_SPIRV_TOOLS

	ProgramBinaries binaries;
	binaries << binary;
	ShaderProgram program(gl, binaries);

	if (!program.isOk())
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Shader build failed.\n"
						   << "Vertex: " << program.getShaderInfo(SHADERTYPE_VERTEX).infoLog << "\n"
						   << "Fragment: " << program.getShaderInfo(SHADERTYPE_FRAGMENT).infoLog << "\n"
						   << "Program: " << program.getProgramInfo().infoLog << tcu::TestLog::EndMessage;

		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
		return STOP;
	}

	gl.viewport(0, 0, 32, 32);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glViewport");

	gl.useProgram(program.getProgram());
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram");

	gl.clearColor(0.0f, 0.0f, 0.0f, 1.0f);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glClearColor");
	gl.clear(GL_COLOR_BUFFER_BIT);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glClear");

	gl.enableVertexAttribArray(0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glEnableVertexAttribArray");

	gl.vertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, DE_NULL);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glVertexAttribPointer");

	gl.drawArrays(GL_TRIANGLE_STRIP, 0, 3);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawArray");

	gl.disableVertexAttribArray(0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glDisableVertexAttribArray");

	GLuint insidePixel;
	GLuint outsidePixel;
	gl.readPixels(16, 16, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, (GLvoid*)&insidePixel);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glReadPixels");
	gl.readPixels(2, 30, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, (GLvoid*)&outsidePixel);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glReadPixels");

	if (vbo)
	{
		gl.deleteBuffers(1, &vbo);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glDeleteBuffers");
	}

	if (vao)
	{
		gl.deleteVertexArrays(1, &vao);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glDeleteVertexArrays");
	}

	if (fbo)
	{
		gl.deleteFramebuffers(1, &fbo);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glDeleteFramebuffers");
	}

	if (texture)
	{
		gl.deleteTextures(1, &texture);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glDeleteTextures");
	}

	if (insidePixel == 0xFFFFFFFF && outsidePixel == 0xFF000000)
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	else
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Wrong pixels color read.\n"
						   << "Expected (inside/outside): " << 0xFFFFFFFF << "/" << 0xFF000000 << "\n"
						   << "Read: " << insidePixel << "/" << outsidePixel << tcu::TestLog::EndMessage;

		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
	}

	return STOP;
}

/** Constructor.
 *
 *  @param context     Rendering context
 *  @param name        Test name
 *  @param description Test description
 */
SpirvModulesStateQueriesTest::SpirvModulesStateQueriesTest(deqp::Context& context)
	: TestCase(context, "spirv_modules_state_queries_test",
			   "Test verifies if state queries for new features added by ARB_gl_spirv works as expected.")
{
	/* Left blank intentionally */
}

/** Stub init method */
void SpirvModulesStateQueriesTest::init()
{
	commonUtils::checkGlSpirvSupported(m_context);

	m_vertex = "#version 450\n"
			   "\n"
			   "layout (location = 0) in vec4 position;\n"
			   "layout (location = 20) uniform vec4 extPosition;\n"
			   "layout (binding = 5) uniform ComponentsBlock\n"
			   "{\n"
			   "    vec4 c1;\n"
			   "    vec2 c2;\n"
			   "} components;\n"
			   "\n"
			   "void main()\n"
			   "{\n"
			   "    gl_Position = position + extPosition + components.c1 + vec4(components.c2, 0.0, 0.0);\n"
			   "}\n";
}

/** Stub de-init method */
void SpirvModulesStateQueriesTest::deinit()
{
}

/** Executes test iteration.
 *
 *  @return Returns STOP when test has finished executing, CONTINUE if more iterations are needed.
 */
tcu::TestNode::IterateResult SpirvModulesStateQueriesTest::iterate()
{
	const Functions& gl = m_context.getRenderContext().getFunctions();

	ProgramBinaries binaries;
	ShaderBinary	vertexBinary;

#if defined DEQP_HAVE_GLSLANG && DEQP_HAVE_SPIRV_TOOLS
	{
		vertexBinary = glslangUtils::makeSpirV(m_context.getTestContext().getLog(), VertexSource(m_vertex));

		// Disassemble Spir-V module
		std::string output;
		glslangUtils::spirvDisassemble(output, vertexBinary.binary);

		// Remove name reflection for defined variables
		std::vector<std::string> lines = de::splitString(output, '\n');
		std::string				 input;
		for (int i = 0; i < lines.size(); ++i)
		{
			if (lines[i].find("OpName %position") != std::string::npos)
				continue;
			if (lines[i].find("OpName %extPosition") != std::string::npos)
				continue;
			if (lines[i].find("OpName %ComponentsBlock") != std::string::npos)
				continue;
			if (lines[i].find("OpName %components") != std::string::npos)
				continue;

			input.append(lines[i] + "\n");
		}

		// Assemble Spir-V module
		vertexBinary.binary.clear();
		glslangUtils::spirvAssemble(vertexBinary.binary, input);
		glslangUtils::spirvValidate(vertexBinary.binary, true);
	}
#else  // DEQP_HAVE_GLSLANG && DEQP_HAVE_SPIRV_TOOLS
	tcu::Archive& archive = m_testCtx.getArchive();
	vertexBinary		  = commonUtils::readSpirV(archive.getResource("spirv/modules_state_queries/vertex.nspv"));
#endif // DEQP_HAVE_GLSLANG && DEQP_HAVE_SPIRV_TOOLS

	binaries << vertexBinary;
	ShaderProgram program(gl, binaries);

	Shader* shader = program.getShader(SHADERTYPE_VERTEX);

	// 1) Check compile status
	if (!program.getShaderInfo(SHADERTYPE_VERTEX).compileOk)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Check compile status failed.\n"
						   << "Vertex: " << program.getShaderInfo(SHADERTYPE_VERTEX).infoLog << "\n"
						   << m_vertex << "\n"
						   << "Program: " << program.getProgramInfo().infoLog << tcu::TestLog::EndMessage;

		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
		return STOP;
	}

	// 2) Check if SPIR_V_BINARY_ARB state is TRUE
	GLint shaderState;
	gl.getShaderiv(shader->getShader(), GL_SPIR_V_BINARY_ARB, &shaderState);
	GLU_EXPECT_NO_ERROR(gl.getError(), "getShaderiv");
	if (shaderState != GL_TRUE)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "SPIR_V_BINARY_ARB state set to FALSE. Expected TRUE."
						   << tcu::TestLog::EndMessage;

		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
		return STOP;
	}

	// 3) Check if queries for ACTIVE_ATTRIBUTE_MAX_LENGTH, ACTIVE_UNIFORM_MAX_LENGTH,
	//    ACTIVE_UNIFORM_BLOCK_MAX_NAME_LENGTH return value equal to 1, and
	//    TRANSFORM_FEEDBACK_VARYING_MAX_LENGTH value equals to 0
	GLint programState[4];
	GLint expectedValues[4] = {1, 1, 0, 1};
	gl.getProgramiv(program.getProgram(), GL_ACTIVE_ATTRIBUTE_MAX_LENGTH, &programState[0]);
	GLU_EXPECT_NO_ERROR(gl.getError(), "getProgramiv");

	gl.getProgramiv(program.getProgram(), GL_ACTIVE_UNIFORM_MAX_LENGTH, &programState[1]);
	GLU_EXPECT_NO_ERROR(gl.getError(), "getProgramiv");

	// We expect 0 for GL_TRANSFORM_FEEDBACK_VARYING_MAX_LENGTH because the current program
	// doesn't activate transform feedback so there isn't any active varying.
	gl.getProgramiv(program.getProgram(), GL_TRANSFORM_FEEDBACK_VARYING_MAX_LENGTH, &programState[2]);
	GLU_EXPECT_NO_ERROR(gl.getError(), "getProgramiv");

	gl.getProgramiv(program.getProgram(), GL_ACTIVE_UNIFORM_BLOCK_MAX_NAME_LENGTH, &programState[3]);
	GLU_EXPECT_NO_ERROR(gl.getError(), "getProgramiv");

	bool programStateResult = true;
	for (int i = 0; i < 4; ++i)
	{
		if (programState[i] != expectedValues[i])
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "Check max name length [" << i << "] failed. "
                                                          << "Expected: " << expectedValues[i] <<", Queried: "
                                                          << programState[i] << "\n"
                                                          << tcu::TestLog::EndMessage;
			programStateResult = false;
		}
	}

	if (!programStateResult)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
		return STOP;
	}

	// 4) Check if ShaderSource command usage on Spir-V binary shader will change SPIR_V_BINARY_ARB state to FALSE
	const char* source = m_vertex.c_str();
	const int   length = m_vertex.length();
	gl.shaderSource(shader->getShader(), 1, &source, &length);
	GLU_EXPECT_NO_ERROR(gl.getError(), "shaderSource");

	gl.getShaderiv(shader->getShader(), GL_SPIR_V_BINARY_ARB, &shaderState);
	GLU_EXPECT_NO_ERROR(gl.getError(), "getShaderiv");
	if (shaderState != GL_FALSE)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "SPIR_V_BINARY_ARB state set to TRUE. Expected FALSE."
						   << tcu::TestLog::EndMessage;

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
SpirvModulesErrorVerificationTest::SpirvModulesErrorVerificationTest(deqp::Context& context)
	: TestCase(context, "spirv_modules_error_verification_test",
			   "Test verifies if new features added by ARB_gl_spirv generate error messages as expected.")
{
	/* Left blank intentionally */
}

/** Stub init method */
void SpirvModulesErrorVerificationTest::init()
{
	commonUtils::checkGlSpirvSupported(m_context);

	const Functions& gl = m_context.getRenderContext().getFunctions();

	m_vertex = "#version 450\n"
			   "\n"
			   "layout (location = 0) in vec4 position;\n"
			   "\n"
			   "void main()\n"
			   "{\n"
			   "    gl_Position = position;\n"
			   "}\n";

	m_glslShaderId = gl.createShader(GL_VERTEX_SHADER);
	GLU_EXPECT_NO_ERROR(gl.getError(), "createShader");

	m_spirvShaderId = gl.createShader(GL_VERTEX_SHADER);
	GLU_EXPECT_NO_ERROR(gl.getError(), "createShader");

	m_programId = gl.createProgram();
	GLU_EXPECT_NO_ERROR(gl.getError(), "createProgram");

	gl.genTextures(1, &m_textureId);
	GLU_EXPECT_NO_ERROR(gl.getError(), "genTextures");
}

/** Stub de-init method */
void SpirvModulesErrorVerificationTest::deinit()
{
	const Functions& gl = m_context.getRenderContext().getFunctions();

	gl.deleteTextures(1, &m_textureId);
	GLU_EXPECT_NO_ERROR(gl.getError(), "deleteTextures");

	gl.deleteProgram(m_programId);
	GLU_EXPECT_NO_ERROR(gl.getError(), "deleteProgram");

	gl.deleteShader(m_glslShaderId);
	GLU_EXPECT_NO_ERROR(gl.getError(), "deleteShader");

	gl.deleteShader(m_spirvShaderId);
	GLU_EXPECT_NO_ERROR(gl.getError(), "deleteShader");
}

/** Executes test iteration.
 *
 *  @return Returns STOP when test has finished executing, CONTINUE if more iterations are needed.
 */
tcu::TestNode::IterateResult SpirvModulesErrorVerificationTest::iterate()
{
	const Functions& gl = m_context.getRenderContext().getFunctions();

	const char* shaderSrc = m_vertex.c_str();
	const int   shaderLen = m_vertex.length();

	ShaderBinary vertexBinary;

#if defined DEQP_HAVE_GLSLANG
	vertexBinary = glslangUtils::makeSpirV(m_context.getTestContext().getLog(), VertexSource(m_vertex));
#else  // DEQP_HAVE_GLSLANG
	tcu::Archive& archive = m_testCtx.getArchive();
	vertexBinary		  = commonUtils::readSpirV(archive.getResource("spirv/modules_error_verification/vertex.nspv"));
#endif // DEQP_HAVE_GLSLANG

	gl.shaderSource(m_glslShaderId, 1, &shaderSrc, &shaderLen);
	GLU_EXPECT_NO_ERROR(gl.getError(), "shaderSource");

	gl.shaderBinary(1, &m_spirvShaderId, GL_SHADER_BINARY_FORMAT_SPIR_V_ARB, (GLvoid*)vertexBinary.binary.data(),
					vertexBinary.binary.size() * sizeof(deUint32));
	GLU_EXPECT_NO_ERROR(gl.getError(), "shaderBinary");

	gl.attachShader(m_programId, m_spirvShaderId);
	GLU_EXPECT_NO_ERROR(gl.getError(), "attachShader");

	GLint err;

	// 1) Verify if CompileShader function used on shader with SPIR_V_BINARY_ARB state
	//    will result in generating INVALID_OPERATION error.
	gl.compileShader(m_spirvShaderId);
	err = gl.getError();
	if (err != GL_INVALID_OPERATION)
	{
		m_testCtx.getLog()
			<< tcu::TestLog::Message
			<< "Unexpected error code generated by CompileShader [1]. Expected INVALID_OPERATION, generated: "
			<< glu::getErrorName(err) << tcu::TestLog::EndMessage;

		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
		return STOP;
	}

	// 2) Verify if SpecializeShader function generate INVALID_VALUE error when
	//    <shader> is not the name of either a program or shader object.
	gl.specializeShader(0xFFFF, "main", 0, DE_NULL, DE_NULL);
	err = gl.getError();
	if (err != GL_INVALID_VALUE)
	{
		m_testCtx.getLog()
			<< tcu::TestLog::Message
			<< "Unexpected error code generated by SpecializeShader [2]. Expected INVALID_VALUE, generated: "
			<< glu::getErrorName(err) << tcu::TestLog::EndMessage;

		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
		return STOP;
	}

	// 3) Verify if SpecializeShader function generate INVALID_OPERATION error when
	//    <shader> is the name of a program object.
	gl.specializeShader(m_programId, "main", 0, DE_NULL, DE_NULL);
	err = gl.getError();
	if (err != GL_INVALID_OPERATION)
	{
		m_testCtx.getLog()
			<< tcu::TestLog::Message
			<< "Unexpected error code generated by SpecializeShader [3]. Expected INVALID_OPERATION, generated: "
			<< glu::getErrorName(err) << tcu::TestLog::EndMessage;

		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
		return STOP;
	}

	// 4) Verify if SpecializeShader function generate INVALID_OPERATION error when
	//    SPIR_V_BINARY_ARB state for <shader> is not TRUE.
	gl.specializeShader(m_glslShaderId, "main", 0, DE_NULL, DE_NULL);
	err = gl.getError();
	if (err != GL_INVALID_OPERATION)
	{
		m_testCtx.getLog()
			<< tcu::TestLog::Message
			<< "Unexpected error code generated by SpecializeShader [4]. Expected INVALID_OPERATION, generated: "
			<< glu::getErrorName(err) << tcu::TestLog::EndMessage;

		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
		return STOP;
	}

	// 5) Verify if SpecializeShader function generate INVALID_VALUE when <pEntryPoint>
	//    does not name a valid entry point for <shader>.
	gl.specializeShader(m_spirvShaderId, "entry", 0, DE_NULL, DE_NULL);
	err = gl.getError();
	if (err != GL_INVALID_VALUE)
	{
		m_testCtx.getLog()
			<< tcu::TestLog::Message
			<< "Unexpected error code generated by SpecializeShader [5]. Expected INVALID_VALUE, generated: "
			<< glu::getErrorName(err) << tcu::TestLog::EndMessage;

		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
		return STOP;
	}

	// 6) Verify if SpecializeShader function generate INVALID_VALUE when any element
	//    of <pConstantIndex> refers to a specialization constant that does not exist
	//    in the shader module contained in <shader>.
	const GLuint specID	= 10;
	const GLuint specValue = 10;
	gl.specializeShader(m_spirvShaderId, "main", 1, &specID, &specValue);
	err = gl.getError();
	if (err != GL_INVALID_VALUE)
	{
		m_testCtx.getLog()
			<< tcu::TestLog::Message
			<< "Unexpected error code generated by SpecializeShader [6]. Expected INVALID_VALUE, generated: "
			<< glu::getErrorName(err) << tcu::TestLog::EndMessage;

		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
		return STOP;
	}

	// 7) Verify if LinkProgram fail when one or more of the shader objects attached to
	//    <program> are not specialized.
	gl.linkProgram(m_programId);
	err = gl.getError();
	if (err == GL_NO_ERROR)
	{
		GLint linkStatus;
		gl.getProgramiv(m_programId, GL_LINK_STATUS, &linkStatus);
		GLU_EXPECT_NO_ERROR(gl.getError(), "getProgramiv");

		if (linkStatus != 0)
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "Unexpected result of LinkProgram [7]."
							   << tcu::TestLog::EndMessage;

			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
			return STOP;
		}
	}

	// 8) Verify if SpecializeShader function generate INVALID_OPERATION error if the
	//    shader has already been specialized.
	gl.specializeShader(m_spirvShaderId, "main", 0, DE_NULL, DE_NULL);
	GLU_EXPECT_NO_ERROR(gl.getError(), "specializeShader");

	gl.specializeShader(m_spirvShaderId, "main", 0, DE_NULL, DE_NULL);
	err = gl.getError();
	if (err != GL_INVALID_OPERATION)
	{
		m_testCtx.getLog()
			<< tcu::TestLog::Message
			<< "Unexpected error code generated by SpecializeShader [8]. Expected INVALID_OPERATION, generated: "
			<< glu::getErrorName(err) << tcu::TestLog::EndMessage;

		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
		return STOP;
	}

	// 9) Verify if LinkProgram fail when not all of shaders attached to <program> have
	//    the same value for the SPIR_V_BINARY_ARB state.
	gl.compileShader(m_glslShaderId);
	GLU_EXPECT_NO_ERROR(gl.getError(), "compileShader");

	gl.attachShader(m_programId, m_glslShaderId);
	GLU_EXPECT_NO_ERROR(gl.getError(), "attachShader");

	gl.linkProgram(m_programId);
	err = gl.getError();
	if (err == GL_NO_ERROR)
	{
		GLint linkStatus;
		gl.getProgramiv(m_programId, GL_LINK_STATUS, &linkStatus);
		GLU_EXPECT_NO_ERROR(gl.getError(), "getProgramiv");

		if (linkStatus != 0)
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "Unexpected result of LinkProgram [9]."
							   << tcu::TestLog::EndMessage;

			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
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
SpirvGlslToSpirVEnableTest::SpirvGlslToSpirVEnableTest(deqp::Context& context)
	: TestCase(context, "spirv_glsl_to_spirv_enable_test", "Test verifies if glsl supports Spir-V features.")
{
	/* Left blank intentionally */
}

/** Stub init method */
void SpirvGlslToSpirVEnableTest::init()
{
	commonUtils::checkGlSpirvSupported(m_context);

	m_vertex = "#version 450\n"
			   "\n"
			   "#ifdef GL_SPIRV\n"
			   "  layout (location = 0) in vec4 enabled;\n"
			   "#else\n"
			   "  layout (location = 0) in vec4 notEnabled;\n"
			   "#endif // GL_SPIRV\n"
			   "\n"
			   "void main()\n"
			   "{\n"
			   "    gl_Position = vec4(0.0, 0.0, 0.0, 0.0);\n"
			   "}\n";
}

/** Stub de-init method */
void SpirvGlslToSpirVEnableTest::deinit()
{
}

/** Executes test iteration.
 *
 *  @return Returns STOP when test has finished executing, CONTINUE if more iterations are needed.
 */
tcu::TestNode::IterateResult SpirvGlslToSpirVEnableTest::iterate()
{

#if defined DEQP_HAVE_GLSLANG && DEQP_HAVE_SPIRV_TOOLS
	{
		const Functions& gl = m_context.getRenderContext().getFunctions();

		ProgramBinaries binaries;
		ShaderBinary	vertexBinary =
			glslangUtils::makeSpirV(m_context.getTestContext().getLog(), VertexSource(m_vertex));
		binaries << vertexBinary;
		ShaderProgram spirvProgram(gl, binaries);

		std::string spirvSource;
		glslangUtils::spirvDisassemble(spirvSource, vertexBinary.binary);

		if (spirvSource.find("OpName %enabled") == std::string::npos)
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "GL_SPIRV not defined. Spir-V source:\n"
							   << spirvSource.c_str() << tcu::TestLog::EndMessage;

			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
			return STOP;
		}

		if (!spirvProgram.isOk())
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "Shader compilation failed. Source:\n"
							   << spirvSource.c_str() << "InfoLog:\n"
							   << spirvProgram.getShaderInfo(SHADERTYPE_VERTEX).infoLog << "\n"
							   << tcu::TestLog::EndMessage;

			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
			return STOP;
		}

		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
#else // DEQP_HAVE_GLSLANG && DEQP_HAVE_SPIRV_TOOLS

	TCU_THROW(InternalError, "Either glslang or spirv-tools not available.");

#endif // DEQP_HAVE_GLSLANG && DEQP_HAVE_SPIRV_TOOLS

	return STOP;
}

enum EShaderTemplate
{
	COMPUTE_TEMPLATE,
	TESSCTRL_TEMPLATE,
	GEOMETRY_TEMPLATE,
	FRAGMENT_TEMPLATE
};

struct FunctionMapping
{
	EShaderTemplate shaderTemplate;
	std::string		glslFunc;
	std::string		glslArgs;
	std::string		spirVFunc;

	FunctionMapping() : shaderTemplate(COMPUTE_TEMPLATE), glslFunc(""), glslArgs(""), spirVFunc("")
	{
	}

	FunctionMapping(EShaderTemplate shaderTemplate_, std::string glslFunc_, std::string glslArgs_,
					std::string spirVFunc_)
		: shaderTemplate(shaderTemplate_), glslFunc(glslFunc_), glslArgs(glslArgs_), spirVFunc(spirVFunc_)
	{
	}
};

/** Constructor.
 *
 *  @param context     Rendering context
 *  @param name        Test name
 *  @param description Test description
 */
SpirvGlslToSpirVBuiltInFunctionsTest::SpirvGlslToSpirVBuiltInFunctionsTest(deqp::Context& context)
	: TestCase(context, "spirv_glsl_to_spirv_builtin_functions_test",
			   "Test verifies if GLSL built-in functions are supported by Spir-V.")
{
	/* Left blank intentionally */
}

/** Stub init method */
void SpirvGlslToSpirVBuiltInFunctionsTest::init()
{
	commonUtils::checkGlSpirvSupported(m_context);

	initMappings();

	m_commonVertex = "#version 450\n"
					 "\n"
					 "layout (location = 0) in vec3 position;\n"
					 "layout (location = 1) out vec2 texCoord;\n"
					 "\n"
					 "void main()\n"
					 "{\n"
					 "    texCoord = vec2(0.0, 0.0);\n"
					 "    gl_Position = vec4(position, 1.0);\n"
					 "}\n";

	m_commonTessEval = "#version 450\n"
					   "\n"
					   "layout (triangles) in;\n"
					   "\n"
					   "void main()\n"
					   "{\n"
					   "    gl_Position = gl_TessCoord.x * gl_in[0].gl_Position +\n"
					   "                  gl_TessCoord.y * gl_in[1].gl_Position +\n"
					   "                  gl_TessCoord.z * gl_in[2].gl_Position;\n"
					   "}\n";

	m_sources.clear();

	// Angle Trigonometry
	m_sources.push_back(ComputeSource("#version 450\n"
									  "\n"
									  "layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;\n"
									  "\n"
									  "void main()\n"
									  "{\n"
									  "    float tmp0 = 0.5;\n"
									  "    float value;\n"
									  "    value = radians(tmp0) +\n"
									  "            degrees(tmp0) +\n"
									  "            sin(tmp0) +\n"
									  "            cos(tmp0) +\n"
									  "            tan(tmp0) +\n"
									  "            asin(tmp0) +\n"
									  "            acos(tmp0) +\n"
									  "            atan(tmp0) +\n"
									  "            atan(tmp0) +\n"
									  "            sinh(tmp0) +\n"
									  "            cosh(tmp0) +\n"
									  "            tanh(tmp0) +\n"
									  "            asinh(tmp0) +\n"
									  "            acosh(tmp0) +\n"
									  "            atanh(tmp0);\n"
									  "}\n"));

	// To avoid duplicated mappings create additional shaders for specific functions
	const std::string strAnlgeVariants = "#version 450\n"
										 "\n"
										 "layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;\n"
										 "\n"
										 "void main()\n"
										 "{\n"
										 "    float tmp0 = 0.5;\n"
										 "    float value = <ATANGENT>;\n"
										 "}\n";
	std::string strATan  = strAnlgeVariants;
	std::string strATan2 = strAnlgeVariants;
	commonUtils::replaceToken("<ATANGENT>", "atan(tmp0, tmp0)", strATan);
	commonUtils::replaceToken("<ATANGENT>", "atan(tmp0)", strATan2);

	m_sources.push_back(ComputeSource(strATan));
	m_sources.push_back(ComputeSource(strATan2));

	// Exponential
	m_sources.push_back(ComputeSource("#version 450\n"
									  "\n"
									  "layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;\n"
									  "\n"
									  "void main()\n"
									  "{\n"
									  "    float tmp0;\n"
									  "    float tmp1;\n"
									  "    float value;\n"
									  "    value = pow(tmp1, tmp0) +\n"
									  "            exp(tmp0) +\n"
									  "            log(tmp1) +\n"
									  "            exp2(tmp0) +\n"
									  "            log2(tmp1) +\n"
									  "            sqrt(tmp1) +\n"
									  "            inversesqrt(tmp1);\n"
									  "}\n"));

	// Common (without bit operations)
	m_sources.push_back(ComputeSource("#version 450\n"
									  "\n"
									  "layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;\n"
									  "\n"
									  "void main()\n"
									  "{\n"
									  "    float value;\n"
									  "    float outval;\n"
									  "    float fpval = 0.5;\n"
									  "    float fnval = -0.5;\n"
									  "    int ival = 0x43800000;\n"
									  "    uint uival= 0xC3800000;\n"
									  "    value = abs(fnval) +\n"
									  "            sign(fpval) +\n"
									  "            floor(fpval) +\n"
									  "            trunc(fpval) +\n"
									  "            round(fpval) +\n"
									  "            roundEven(fpval) +\n"
									  "            ceil(fpval) +\n"
									  "            fract(fpval) +\n"
									  "            mod(fpval, 2.0) +\n"
									  "            modf(fpval, outval) +\n"
									  "            min(fpval, 0.2) +\n"
									  "            max(fpval, 0.2) +\n"
									  "            clamp(fpval, 0.8, 2.0) +\n"
									  "            mix(fnval, fpval, 0.5) +\n"
									  "            step(1.0, fpval) +\n"
									  "            smoothstep(0.0, 1.0, fpval) +\n"
									  "            float( isnan(fpval)) +\n"
									  "            float( isinf(fpval)) +\n"
									  "            fma(fpval, 1.0, fnval) +\n"
									  "            frexp(4.0, ival) +\n"
									  "            ldexp(4.0, ival);\n"
									  "}\n"));

	// To avoid duplicated mappings create additional shaders for specific functions
	const std::string strBitsOpsVariants = "#version 450\n"
										   "\n"
										   "layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;\n"
										   "\n"
										   "void main()\n"
										   "{\n"
										   "    float value;\n"
										   "    int ival = 0x43800000;\n"
										   "    uint uval = 0x43800000;\n"
										   "    value = <BITS_TO_FLOAT>;\n"
										   "}\n";
	std::string strIntBits  = strBitsOpsVariants;
	std::string strUIntBits = strBitsOpsVariants;
	commonUtils::replaceToken("<BITS_TO_FLOAT>", "intBitsToFloat(ival)", strIntBits);
	commonUtils::replaceToken("<BITS_TO_FLOAT>", "uintBitsToFloat(uval)", strUIntBits);

	m_sources.push_back(ComputeSource(strIntBits));
	m_sources.push_back(ComputeSource(strUIntBits));

	// Float Pack Unpack
	m_sources.push_back(ComputeSource("#version 450\n"
									  "\n"
									  "layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;\n"
									  "\n"
									  "void main()\n"
									  "{\n"
									  "    vec2 v2val = vec2(0.1, 0.2);\n"
									  "    vec4 v4val = vec4(0.1, 0.2, 0.3, 0.4);\n"
									  "    uint uival1 = packUnorm2x16(v2val);\n"
									  "    uint uival2 = packSnorm2x16(v2val);\n"
									  "    uint uival3 = packUnorm4x8(v4val);\n"
									  "    uint uival4 = packSnorm4x8(v4val);\n"
									  "    v2val = unpackUnorm2x16(uival1);\n"
									  "    v2val = unpackSnorm2x16(uival2);\n"
									  "    v4val = unpackUnorm4x8(uival3);\n"
									  "    v4val = unpackSnorm4x8(uival4);\n"
									  "    uvec2 uv2val = uvec2(10, 20);\n"
									  "    double dval = packDouble2x32(uv2val);\n"
									  "    uv2val = unpackDouble2x32(dval);\n"
									  "    uint uival5 = packHalf2x16(v2val);\n"
									  "    v2val = unpackHalf2x16(uival5);\n"
									  "}\n"));

	// Geometric
	m_sources.push_back(ComputeSource("#version 450\n"
									  "\n"
									  "layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;\n"
									  "\n"
									  "void main()\n"
									  "{\n"
									  "    vec3 v3val1 = vec3(0.1, 0.5, 1.0);\n"
									  "    vec3 v3val2 = vec3(0.5, 0.3, 0.9);\n"
									  "    vec3 v3val3 = vec3(1.0, 0.0, 0.0);\n"
									  "    float fval = length(v3val1) +\n"
									  "                 distance(v3val1, v3val2) +\n"
									  "                 dot(v3val1, v3val2);\n"
									  "    vec3 crossp = cross(v3val1, v3val2);\n"
									  "    vec3 norm = normalize(crossp);\n"
									  "    vec3 facef = faceforward(v3val1, v3val2, v3val3);\n"
									  "    vec3 refl = reflect(v3val1, v3val2);\n"
									  "    float eta = 0.1;\n"
									  "    vec3 refr = refract(v3val1, v3val2, eta);"
									  "}\n"));

	// Matrix
	m_sources.push_back(ComputeSource("#version 450\n"
									  "\n"
									  "layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;\n"
									  "\n"
									  "void main()\n"
									  "{\n"
									  "    mat2 m2val1 = mat2(\n"
									  "        0.1, 0.5,\n"
									  "        0.2, 0.4\n"
									  "    );\n"
									  "    mat2 m2val2 = mat2(\n"
									  "        0.8, 0.2,\n"
									  "        0.9, 0.1\n"
									  "    );\n"
									  "    vec2 v2val1 = vec2(0.3, 0.4);\n"
									  "    vec2 v2val2 = vec2(0.5, 0.6);\n"
									  "\n"
									  "    mat2 m2comp = matrixCompMult(m2val1, m2val2);\n"
									  "    mat2 m2outerp = outerProduct(v2val1, v2val2);\n"
									  "    mat2 m2trans = transpose(m2val1);\n"
									  "    float fdet = determinant(m2val2);\n"
									  "    mat2 m2inv = inverse(m2trans);\n"
									  "}\n"));

	// Vector Relational
	m_sources.push_back(ComputeSource("#version 450\n"
									  "\n"
									  "layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;\n"
									  "\n"
									  "void main()\n"
									  "{\n"
									  "    vec2 v2val1 = vec2(0.5, 0.2);\n"
									  "    vec2 v2val2 = vec2(0.1, 0.8);\n"
									  "    bvec2 bv2val1 = lessThan(v2val1, v2val2);\n"
									  "    bvec2 bv2val2 = lessThanEqual(v2val1, v2val2);\n"
									  "    bvec2 bv2val3 = greaterThan(v2val1, v2val2);\n"
									  "    bvec2 bv2val4 = greaterThanEqual(v2val1, v2val2);\n"
									  "    bvec2 bv2val5 = equal(v2val1, v2val2);\n"
									  "    bvec2 bv2val6 = notEqual(v2val1, v2val2);\n"
									  "    bool bval1 = any(bv2val1);\n"
									  "    bool bval2 = all(bv2val1);\n"
									  "    bvec2 bv2val7 = not(bv2val1);\n"
									  "}\n"));

	// Integer
	m_sources.push_back(ComputeSource("#version 450\n"
									  "\n"
									  "layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;\n"
									  "\n"
									  "void main()\n"
									  "{\n"
									  "    int ival = 0;\n"
									  "    uint uival = 200;\n"
									  "    uint uivalRet1;\n"
									  "    uint uivalRet2;\n"
									  "    uivalRet2 = uaddCarry(uival, 0xFFFFFFFF, uivalRet1);\n"
									  "    uivalRet2 = usubBorrow(uival, 0xFFFFFFFF, uivalRet1);\n"
									  "    umulExtended(uival, 0xFFFFFFFF, uivalRet1, uivalRet2);\n"
									  "    uivalRet1 = bitfieldExtract(uival, 3, 8);\n"
									  "    uivalRet1 = bitfieldInsert(uival, 0xFFFFFFFF, 3, 8);\n"
									  "    uivalRet1 = bitfieldReverse(uival);\n"
									  "    ival = bitCount(uival);\n"
									  "    ival = findLSB(uival);\n"
									  "    ival = findMSB(uival);\n"
									  "}\n"));

	// Texture
	m_sources.push_back(
		FragmentSource("#version 450\n"
					   "\n"
					   "layout (location = 0) out vec4 fragColor;\n"
					   "\n"
					   "layout (location = 1) uniform sampler2D tex2D;\n"
					   "layout (location = 2) uniform sampler2DMS tex2DMS;\n"
					   "\n"
					   "void main()\n"
					   "{\n"
					   "    ivec2 iv2size = textureSize(tex2D, 0);\n"
					   "    vec2 v2lod = textureQueryLod(tex2D, vec2(0.0));\n"
					   "    int ilev = textureQueryLevels(tex2D);\n"
					   "    int isamp = textureSamples(tex2DMS);\n"
					   "    vec4 v4pix = textureLod(tex2D, vec2(0.0), 0.0) +\n"
					   "                 textureOffset(tex2D, vec2(0.0), ivec2(2)) +\n"
					   "                 texelFetch(tex2D, ivec2(2), 0) +\n"
					   "                 texelFetchOffset(tex2D, ivec2(2), 0, ivec2(2)) +\n"
					   "                 textureProjOffset(tex2D, vec3(0.0), ivec2(2)) +\n"
					   "                 textureLodOffset(tex2D, vec2(0.0), 0.0, ivec2(2)) +\n"
					   "                 textureProjLod(tex2D, vec3(0.0), 0.0) +\n"
					   "                 textureProjLodOffset(tex2D, vec3(0.0), 0.0, ivec2(2)) +\n"
					   "                 textureGrad(tex2D, vec2(0.0), vec2(0.2), vec2(0.5)) +\n"
					   "                 textureGradOffset(tex2D, vec2(0.0), vec2(0.2), vec2(0.5), ivec2(2)) +\n"
					   "                 textureProjGrad(tex2D, vec3(0.0), vec2(0.2), vec2(0.5)) +\n"
					   "                 textureProjGradOffset(tex2D, vec3(0.0), vec2(0.2), vec2(0.5), ivec2(2)) +\n"
					   "                 textureGatherOffset(tex2D, vec2(0.0), ivec2(2), 0);\n"
					   "    fragColor = vec4(0.0);\n"
					   "}\n"));

	// To avoid duplicated mappings create additional shaders for specific functions
	const std::string strTextureVariants = "#version 450\n"
										   "\n"
										   "layout (location = 0) out vec4 fragColor;\n"
										   "\n"
										   "layout (location = 1) uniform sampler2D tex2D;\n"
										   "\n"
										   "void main()\n"
										   "{\n"
										   "    fragColor = <TEXTURE>;\n"
										   "}\n";
	std::string strTexture		 = strTextureVariants;
	std::string strTextureProj   = strTextureVariants;
	std::string strTextureGather = strTextureVariants;
	commonUtils::replaceToken("<TEXTURE>", "texture(tex2D, vec2(0.0))", strTexture);
	commonUtils::replaceToken("<TEXTURE>", "textureProj(tex2D, vec3(0.0))", strTextureProj);
	commonUtils::replaceToken("<TEXTURE>", "textureGather(tex2D, vec2(0.0), 0)", strTextureGather);

	m_sources.push_back(FragmentSource(strTexture));
	m_sources.push_back(FragmentSource(strTextureProj));
	m_sources.push_back(FragmentSource(strTextureGather));

	// Atomic Counter
	m_sources.push_back(ComputeSource("#version 450\n"
									  "\n"
									  "layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;\n"
									  "\n"
									  "layout (binding = 0) uniform atomic_uint auival;\n"
									  "\n"
									  "void main()\n"
									  "{\n"
									  "    uint uival = atomicCounterIncrement(auival) +\n"
									  "                 atomicCounterDecrement(auival) +\n"
									  "                 atomicCounter(auival);\n"
									  "}\n"));

	// Atomic Memory
	m_sources.push_back(ComputeSource("#version 450\n"
									  "\n"
									  "layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;\n"
									  "\n"
									  "shared uint uishared;\n"
									  "\n"
									  "void main()\n"
									  "{\n"
									  "    uint uival2 = 5;\n"
									  "    uint uivalRet = atomicAdd(uishared, uival2) +\n"
									  "                    atomicMin(uishared, uival2) +\n"
									  "                    atomicMax(uishared, uival2) +\n"
									  "                    atomicAnd(uishared, uival2) +\n"
									  "                    atomicOr(uishared, uival2) +\n"
									  "                    atomicXor(uishared, uival2) +\n"
									  "                    atomicExchange(uishared, uival2) +\n"
									  "                    atomicCompSwap(uishared, uishared, uival2);\n"
									  "}\n"));

	// Image
	m_sources.push_back(ComputeSource("#version 450\n"
									  "\n"
									  "layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;\n"
									  "\n"
									  "layout (location = 1, rgba8ui) uniform readonly uimage2D rimg2D;\n"
									  "layout (location = 2, rgba8ui) uniform readonly uimage2DMS rimg2DMS;\n"
									  "layout (location = 3, rgba8ui) uniform writeonly uimage2D wimg2D;\n"
									  "layout (location = 4, r32ui) uniform uimage2D aimg2D;\n"
									  "\n"
									  "void main()\n"
									  "{\n"
									  "    ivec2 size = imageSize(rimg2D);\n"
									  "    int samp = imageSamples(rimg2DMS);\n"
									  "    uvec4 v4pix = imageLoad(rimg2D, ivec2(0));\n"
									  "    imageStore(wimg2D, ivec2(0), uvec4(255));\n"
									  "    uint uivalRet = imageAtomicAdd(aimg2D, ivec2(0), 1) +\n"
									  "                    imageAtomicMin(aimg2D, ivec2(0), 1) +\n"
									  "                    imageAtomicMax(aimg2D, ivec2(0), 1) +\n"
									  "                    imageAtomicAnd(aimg2D, ivec2(0), 1) +\n"
									  "                    imageAtomicOr(aimg2D, ivec2(0), 1) +\n"
									  "                    imageAtomicXor(aimg2D, ivec2(0), 1) +\n"
									  "                    imageAtomicExchange(aimg2D, ivec2(0), 1) +\n"
									  "                    imageAtomicCompSwap(aimg2D, ivec2(0), 1, 2);\n"
									  "}\n"));

	// Fragment Processing
	m_sources.push_back(FragmentSource("#version 450\n"
									   "\n"
									   "layout (location = 0) out vec4 fragColor;\n"
									   "layout (location = 1) in vec2 texCoord;\n"
									   "\n"
									   "void main()\n"
									   "{\n"
									   "    vec2 p = vec2(0.0);\n"
									   "    vec2 dx = dFdx(p);\n"
									   "    vec2 dy = dFdy(p);\n"
									   "    dx = dFdxFine(p);\n"
									   "    dy = dFdyFine(p);\n"
									   "    dx = dFdxCoarse(p);\n"
									   "    dy = dFdyCoarse(p);\n"
									   "    vec2 fw = fwidth(p);\n"
									   "    fw = fwidthFine(p);\n"
									   "    fw = fwidthCoarse(p);\n"
									   "    vec2 interp = interpolateAtCentroid(texCoord) +\n"
									   "                  interpolateAtSample(texCoord, 0) +\n"
									   "                  interpolateAtOffset(texCoord, vec2(0.0));\n"
									   "    fragColor = vec4(1.0);\n"
									   "}\n"));

	// To avoid duplicated mappings create additional shaders for specific functions
	const std::string strEmitVariants = "#version 450\n"
										"\n"
										"layout (points) in;\n"
										"layout (points, max_vertices = 3) out;\n"
										"\n"
										"void main()\n"
										"{\n"
										"    gl_Position = vec4(0.0);\n"
										"    <EMIT>;\n"
										"    <END>;\n"
										"}\n";
	std::string strEmit		  = strEmitVariants;
	std::string strEmitStream = strEmitVariants;
	commonUtils::replaceToken("<EMIT>", "EmitVertex()", strEmit);
	commonUtils::replaceToken("<EMIT>", "EmitStreamVertex(0)", strEmitStream);
	commonUtils::replaceToken("<END>", "EndPrimitive()", strEmit);
	commonUtils::replaceToken("<END>", "EndStreamPrimitive(0)", strEmitStream);

	m_sources.push_back(GeometrySource(strEmit));
	m_sources.push_back(GeometrySource(strEmitStream));

	// Shader Invocation Control
	m_sources.push_back(
		TessellationControlSource("#version 450\n"
								  "\n"
								  "layout (vertices = 3) out;\n"
								  "\n"
								  "void main()\n"
								  "{\n"
								  "    barrier();\n"
								  "\n"
								  "    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;\n"
								  "}\n"));

	// Shared Memory Control
	// To avoid duplicated mappings create additional shaders for specific functions
	const std::string strMemoryBarrierSource = "#version 450\n"
											   "\n"
											   "layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;\n"
											   "\n"
											   "void main()\n"
											   "{\n"
											   "    <MEMORY_BARRIER>;\n"
											   "}\n";
	std::string strMemoryBarrier			  = strMemoryBarrierSource;
	std::string strMemoryBarrierAtomicCounter = strMemoryBarrierSource;
	std::string strMemoryBarrierBuffer		  = strMemoryBarrierSource;
	std::string strMemoryBarrierShared		  = strMemoryBarrierSource;
	std::string strMemoryBarrierImage		  = strMemoryBarrierSource;
	std::string strGroupMemoryBarrier		  = strMemoryBarrierSource;
	commonUtils::replaceToken("<MEMORY_BARRIER>", "memoryBarrier()", strMemoryBarrier);
	commonUtils::replaceToken("<MEMORY_BARRIER>", "memoryBarrierAtomicCounter()", strMemoryBarrierAtomicCounter);
	commonUtils::replaceToken("<MEMORY_BARRIER>", "memoryBarrierBuffer()", strMemoryBarrierBuffer);
	commonUtils::replaceToken("<MEMORY_BARRIER>", "memoryBarrierShared()", strMemoryBarrierShared);
	commonUtils::replaceToken("<MEMORY_BARRIER>", "memoryBarrierImage()", strMemoryBarrierImage);
	commonUtils::replaceToken("<MEMORY_BARRIER>", "groupMemoryBarrier()", strGroupMemoryBarrier);

	m_sources.push_back(ComputeSource(strMemoryBarrier));
	m_sources.push_back(ComputeSource(strMemoryBarrierAtomicCounter));
	m_sources.push_back(ComputeSource(strMemoryBarrierBuffer));
	m_sources.push_back(ComputeSource(strMemoryBarrierShared));
	m_sources.push_back(ComputeSource(strMemoryBarrierImage));
	m_sources.push_back(ComputeSource(strGroupMemoryBarrier));
}

/** Stub de-init method */
void SpirvGlslToSpirVBuiltInFunctionsTest::deinit()
{
}

/** Executes test iteration.
 *
 *  @return Returns STOP when test has finished executing, CONTINUE if more iterations are needed.
 */
tcu::TestNode::IterateResult SpirvGlslToSpirVBuiltInFunctionsTest::iterate()
{
	const Functions& gl = m_context.getRenderContext().getFunctions();

	for (int i = 0; i < m_sources.size(); ++i)
	{
		ShaderSource shaderSource = m_sources[i];

		ProgramSources  sources;
		ProgramBinaries binaries;

		if (shaderSource.shaderType != glu::SHADERTYPE_COMPUTE)
		{
			ShaderSource vertexSource(glu::SHADERTYPE_VERTEX, m_commonVertex);

			sources << vertexSource;
			ShaderBinary vertexBinary;
#if defined				 DEQP_HAVE_GLSLANG
			vertexBinary = glslangUtils::makeSpirV(m_context.getTestContext().getLog(), vertexSource);
#else  // DEQP_HAVE_GLSLANG
			tcu::Archive& archive = m_testCtx.getArchive();
			vertexBinary =
				commonUtils::readSpirV(archive.getResource("spirv/glsl_to_spirv_builtin_functions/common_vertex.nspv"));
#endif //DEQP_HAVE_GLSLANG
			binaries << vertexBinary;
		}

		sources << shaderSource;
		ShaderBinary shaderBinary;
		std::string  spirvSource;

#if defined DEQP_HAVE_GLSLANG
		shaderBinary = glslangUtils::makeSpirV(m_context.getTestContext().getLog(), shaderSource);
#else  // DEQP_HAVE_GLSLANG
		{
			std::stringstream ss;
			ss << "spirv/glsl_to_spirv_builtin_functions/binary_" << i << ".nspv";

			tcu::Archive& archive = m_testCtx.getArchive();
			shaderBinary		  = commonUtils::readSpirV(archive.getResource(ss.str().c_str()));
		}
#endif // DEQP_HAVE_GLSLANG

#if defined DEQP_HAVE_SPIRV_TOOLS
		{
			glslangUtils::spirvDisassemble(spirvSource, shaderBinary.binary);

			if (!glslangUtils::verifyMappings(shaderSource.source, spirvSource, m_mappings, false))
			{
				m_testCtx.getLog() << tcu::TestLog::Message << "Mappings for shader failed.\n"
								   << "GLSL source:\n"
								   << shaderSource.source.c_str() << "\n"
								   << "SpirV source:\n"
								   << spirvSource.c_str() << tcu::TestLog::EndMessage;

				TCU_THROW(InternalError, "Mappings for shader failed.");
			}
		}
#else  // DEQP_HAVE_SPIRV_TOOLS
		spirvSource				  = "Could not disassemble Spir-V module. SPIRV-TOOLS not available.";
#endif // DEQP_HAVE_SPIRV_TOOLS

		binaries << shaderBinary;

		if (shaderSource.shaderType == glu::SHADERTYPE_TESSELLATION_CONTROL)
		{
			ShaderSource tessEvalSource(glu::SHADERTYPE_TESSELLATION_EVALUATION, m_commonTessEval);

			sources << tessEvalSource;
			ShaderBinary tessEvalBinary;
#if defined				 DEQP_HAVE_GLSLANG
			tessEvalBinary = glslangUtils::makeSpirV(m_context.getTestContext().getLog(), tessEvalSource);
#else  // DEQP_HAVE_GLSLANG
			tcu::Archive& archive = m_testCtx.getArchive();
			tessEvalBinary		  = commonUtils::readSpirV(
				archive.getResource("spirv/glsl_to_spirv_builtin_functions/common_tesseval.nspv"));
#endif // DEQP_HAVE_GLSLANG
			binaries << tessEvalBinary;
		}

		ShaderProgram glslProgram(gl, sources);
		if (!glslProgram.isOk())
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "GLSL shader compilation failed. Source:\n"
							   << shaderSource.source.c_str() << "InfoLog:\n"
							   << glslProgram.getShaderInfo(shaderSource.shaderType).infoLog << "\n"
							   << tcu::TestLog::EndMessage;

			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
			return STOP;
		}

		ShaderProgram spirvProgram(gl, binaries);
		if (!spirvProgram.isOk())
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "SpirV shader compilation failed. Source:\n"
							   << spirvSource.c_str() << "InfoLog:\n"
							   << spirvProgram.getShaderInfo(shaderSource.shaderType).infoLog << "\n"
							   << tcu::TestLog::EndMessage;

			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
			return STOP;
		}
	}

	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	return STOP;
}

/** Mappings init method */
void SpirvGlslToSpirVBuiltInFunctionsTest::initMappings()
{
	m_mappings.clear();
	m_mappings["radians"].push_back("OpExtInst Radians");
	m_mappings["degrees"].push_back("OpExtInst Degrees");
	m_mappings["sin"].push_back("OpExtInst Sin");
	m_mappings["cos"].push_back("OpExtInst Cos");
	m_mappings["tan"].push_back("OpExtInst Tan");
	m_mappings["asin"].push_back("OpExtInst Asin");
	m_mappings["acos"].push_back("OpExtInst Acos");
	m_mappings["atan"].push_back("OpExtInst Atan2");
	m_mappings["atan"].push_back("OpExtInst Atan");
	m_mappings["sinh"].push_back("OpExtInst Sinh");
	m_mappings["cosh"].push_back("OpExtInst Cosh");
	m_mappings["tanh"].push_back("OpExtInst Tanh");
	m_mappings["asinh"].push_back("OpExtInst Asinh");
	m_mappings["acosh"].push_back("OpExtInst Acosh");
	m_mappings["atanh"].push_back("OpExtInst Atanh");
	m_mappings["pow"].push_back("OpExtInst Pow");
	m_mappings["exp"].push_back("OpExtInst Exp");
	m_mappings["log"].push_back("OpExtInst Log");
	m_mappings["exp2"].push_back("OpExtInst Exp2");
	m_mappings["log2"].push_back("OpExtInst Log2");
	m_mappings["sqrt"].push_back("OpExtInst Sqrt");
	m_mappings["inversesqrt"].push_back("OpExtInst InverseSqrt");
	m_mappings["abs"].push_back("OpExtInst FAbs");
	m_mappings["sign"].push_back("OpExtInst FSign");
	m_mappings["floor"].push_back("OpExtInst Floor");
	m_mappings["trunc"].push_back("OpExtInst Trunc");
	m_mappings["round"].push_back("OpExtInst Round");
	m_mappings["roundEven"].push_back("OpExtInst RoundEven");
	m_mappings["ceil"].push_back("OpExtInst Ceil");
	m_mappings["fract"].push_back("OpExtInst Fract");
	m_mappings["mod"].push_back("OpFMod");
	m_mappings["modf"].push_back("OpExtInst Modf");
	m_mappings["min"].push_back("OpExtInst FMin");
	m_mappings["max"].push_back("OpExtInst FMax");
	m_mappings["clamp"].push_back("OpExtInst FClamp");
	m_mappings["mix"].push_back("OpExtInst FMix");
	m_mappings["step"].push_back("OpExtInst Step");
	m_mappings["smoothstep"].push_back("OpExtInst SmoothStep");
	m_mappings["intBitsToFloat"].push_back("OpBitcast");
	m_mappings["uintBitsToFloat"].push_back("OpBitcast");
	m_mappings["isnan"].push_back("OpIsNan");
	m_mappings["isinf"].push_back("OpIsInf");
	m_mappings["fma"].push_back("OpExtInst Fma");
	m_mappings["frexp"].push_back("OpExtInst FrexpStruct");
	m_mappings["ldexp"].push_back("OpExtInst Ldexp");
	m_mappings["packUnorm2x16"].push_back("OpExtInst PackUnorm2x16");
	m_mappings["packSnorm2x16"].push_back("OpExtInst PackSnorm2x16");
	m_mappings["packUnorm4x8"].push_back("OpExtInst PackUnorm4x8");
	m_mappings["packSnorm4x8"].push_back("OpExtInst PackSnorm4x8");
	m_mappings["unpackUnorm2x16"].push_back("OpExtInst UnpackUnorm2x16");
	m_mappings["unpackSnorm2x16"].push_back("OpExtInst UnpackSnorm2x16");
	m_mappings["unpackUnorm4x8"].push_back("OpExtInst UnpackUnorm4x8");
	m_mappings["unpackSnorm4x8"].push_back("OpExtInst UnpackSnorm4x8");
	m_mappings["packDouble2x32"].push_back("OpExtInst PackDouble2x32");
	m_mappings["unpackDouble2x32"].push_back("OpExtInst UnpackDouble2x32");
	m_mappings["packHalf2x16"].push_back("OpExtInst PackHalf2x16");
	m_mappings["unpackHalf2x16"].push_back("OpExtInst UnpackHalf2x16");
	m_mappings["length"].push_back("OpExtInst Length");
	m_mappings["distance"].push_back("OpExtInst Distance");
	m_mappings["dot"].push_back("OpDot");
	m_mappings["cross"].push_back("OpExtInst Cross");
	m_mappings["normalize"].push_back("OpExtInst Normalize");
	m_mappings["faceforward"].push_back("OpExtInst FaceForward");
	m_mappings["reflect"].push_back("OpExtInst Reflect");
	m_mappings["refract"].push_back("OpExtInst Refract");
	// This one could not be mapped as Spir-V equivalent need more steps
	// m_mappings["matrixCompMult"].push_back("");
	m_mappings["outerProduct"].push_back("OpOuterProduct");
	m_mappings["transpose"].push_back("OpTranspose");
	m_mappings["determinant"].push_back("OpExtInst Determinant");
	m_mappings["inverse"].push_back("OpExtInst MatrixInverse");
	m_mappings["lessThan"].push_back("OpFOrdLessThan");
	m_mappings["lessThanEqual"].push_back("OpFOrdLessThanEqual");
	m_mappings["greaterThan"].push_back("OpFOrdGreaterThan");
	m_mappings["greaterThanEqual"].push_back("OpFOrdGreaterThanEqual");
	m_mappings["equal"].push_back("OpFOrdEqual");
	m_mappings["notEqual"].push_back("OpFOrdNotEqual");
	m_mappings["any"].push_back("OpAny");
	m_mappings["all"].push_back("OpAll");
	m_mappings["not"].push_back("OpLogicalNot");
	m_mappings["uaddCarry"].push_back("OpIAddCarry");
	m_mappings["usubBorrow"].push_back("OpISubBorrow");
	m_mappings["umulExtended"].push_back("OpUMulExtended");
	m_mappings["bitfieldExtract"].push_back("OpBitFieldUExtract");
	m_mappings["bitfieldInsert"].push_back("OpBitFieldInsert");
	m_mappings["bitfieldReverse"].push_back("OpBitReverse");
	m_mappings["bitCount"].push_back("OpBitCount");
	m_mappings["findLSB"].push_back("OpExtInst FindILsb");
	m_mappings["findMSB"].push_back("OpExtInst FindUMsb");
	m_mappings["textureSize"].push_back("OpImageQuerySizeLod");
	m_mappings["textureQueryLod"].push_back("OpImageQueryLod");
	m_mappings["textureQueryLevels"].push_back("OpImageQueryLevels");
	m_mappings["textureSamples"].push_back("OpImageQuerySamples");
	m_mappings["texture"].push_back("OpImageSampleImplicitLod");
	m_mappings["textureProj"].push_back("OpImageSampleProjImplicitLod");
	m_mappings["textureLod"].push_back("OpImageSampleExplicitLod Lod");
	m_mappings["textureOffset"].push_back("OpImageSampleImplicitLod ConstOffset");
	m_mappings["texelFetch"].push_back("OpImageFetch Lod");
	m_mappings["texelFetchOffset"].push_back("OpImageFetch Lod|ConstOffset");
	m_mappings["textureProjOffset"].push_back("OpImageSampleProjImplicitLod ConstOffset");
	m_mappings["textureLodOffset"].push_back("OpImageSampleExplicitLod Lod|ConstOffset");
	m_mappings["textureProjLod"].push_back("OpImageSampleProjExplicitLod Lod");
	m_mappings["textureProjLodOffset"].push_back("OpImageSampleProjExplicitLod Lod|ConstOffset");
	m_mappings["textureGrad"].push_back("OpImageSampleExplicitLod Grad");
	m_mappings["textureGradOffset"].push_back("OpImageSampleExplicitLod Grad|ConstOffset");
	m_mappings["textureProjGrad"].push_back("OpImageSampleProjExplicitLod Grad");
	m_mappings["textureProjGradOffset"].push_back("OpImageSampleProjExplicitLod Grad|ConstOffset");
	m_mappings["textureGather"].push_back("OpImageGather");
	m_mappings["textureGatherOffset"].push_back("OpImageGather ConstOffset");
	m_mappings["atomicCounterIncrement"].push_back("OpAtomicIIncrement");
	m_mappings["atomicCounterDecrement"].push_back("OpAtomicIDecrement");
	m_mappings["atomicCounter"].push_back("OpAtomicLoad");
	m_mappings["atomicAdd"].push_back("OpAtomicIAdd");
	m_mappings["atomicMin"].push_back("OpAtomicUMin");
	m_mappings["atomicMax"].push_back("OpAtomicUMax");
	m_mappings["atomicAnd"].push_back("OpAtomicAnd");
	m_mappings["atomicOr"].push_back("OpAtomicOr");
	m_mappings["atomicXor"].push_back("OpAtomicXor");
	m_mappings["atomicExchange"].push_back("OpAtomicExchange");
	m_mappings["atomicCompSwap"].push_back("OpAtomicCompareExchange");
	m_mappings["imageSize"].push_back("OpImageQuerySize");
	m_mappings["imageSamples"].push_back("OpImageQuerySamples");
	m_mappings["imageLoad"].push_back("OpImageRead");
	m_mappings["imageStore"].push_back("OpImageWrite");
	m_mappings["imageAtomicAdd"].push_back("OpAtomicIAdd");
	m_mappings["imageAtomicMin"].push_back("OpAtomicUMin");
	m_mappings["imageAtomicMax"].push_back("OpAtomicUMax");
	m_mappings["imageAtomicAnd"].push_back("OpAtomicAnd");
	m_mappings["imageAtomicOr"].push_back("OpAtomicOr");
	m_mappings["imageAtomicXor"].push_back("OpAtomicXor");
	m_mappings["imageAtomicExchange"].push_back("OpAtomicExchange");
	m_mappings["imageAtomicCompSwap"].push_back("OpAtomicCompareExchange");
	m_mappings["dFdx"].push_back("OpDPdx");
	m_mappings["dFdy"].push_back("OpDPdy");
	m_mappings["dFdxFine"].push_back("OpDPdxFine");
	m_mappings["dFdyFine"].push_back("OpDPdyFine");
	m_mappings["dFdxCoarse"].push_back("OpDPdxCoarse");
	m_mappings["dFdyCoarse"].push_back("OpDPdyCoarse");
	m_mappings["fwidth"].push_back("OpFwidth");
	m_mappings["fwidthFine"].push_back("OpFwidthFine");
	m_mappings["fwidthCoarse"].push_back("OpFwidthCoarse");
	m_mappings["interpolateAtCentroid"].push_back("OpExtInst InterpolateAtCentroid");
	m_mappings["interpolateAtSample"].push_back("OpExtInst InterpolateAtSample");
	m_mappings["interpolateAtOffset"].push_back("OpExtInst InterpolateAtOffset");
	m_mappings["EmitStreamVertex"].push_back("OpEmitStreamVertex");
	m_mappings["EndStreamPrimitive"].push_back("OpEndStreamPrimitive");
	m_mappings["EmitVertex"].push_back("OpEmitVertex");
	m_mappings["EndPrimitive"].push_back("OpEndPrimitive");
	m_mappings["barrier"].push_back("OpControlBarrier");
	m_mappings["memoryBarrier"].push_back("OpMemoryBarrier");
	m_mappings["memoryBarrierAtomicCounter"].push_back("OpMemoryBarrier");
	m_mappings["memoryBarrierBuffer"].push_back("OpMemoryBarrier");
	m_mappings["memoryBarrierShared"].push_back("OpMemoryBarrier");
	m_mappings["memoryBarrierImage"].push_back("OpMemoryBarrier");
	m_mappings["groupMemoryBarrier"].push_back("OpMemoryBarrier");

	// Add a space prefix and parenthesis sufix to avoid searching for similar names
	SpirVMapping		   tempMappings;
	SpirVMapping::iterator it;
	for (it = m_mappings.begin(); it != m_mappings.end(); ++it)
	{
		tempMappings[std::string(" ") + it->first + "("] = it->second;
	}

	m_mappings = tempMappings;
}

/** Constructor.
 *
 *  @param context     Rendering context
 *  @param name        Test name
 *  @param description Test description
 */
SpirvGlslToSpirVSpecializationConstantsTest::SpirvGlslToSpirVSpecializationConstantsTest(deqp::Context& context)
	: TestCase(context, "spirv_glsl_to_spirv_specialization_constants_test",
			   "Test verifies if constant specialization feature works as expected.")
{
	/* Left blank intentionally */
}

/** Stub init method */
void SpirvGlslToSpirVSpecializationConstantsTest::init()
{
	commonUtils::checkGlSpirvSupported(m_context);

	const Functions& gl = m_context.getRenderContext().getFunctions();

	m_vertex = "#version 450\n"
			   "\n"
			   "layout (location = 0) in vec3 position;\n"
			   "\n"
			   "void main()\n"
			   "{\n"
			   "    gl_Position = vec4(position, 1.0);\n"
			   "}\n";

	m_fragment = "#version 450\n"
				 "\n"
				 "layout (constant_id = 10) const int red = 255;\n"
				 "\n"
				 "layout (location = 0) out vec4 fragColor;\n"
				 "\n"
				 "void main()\n"
				 "{\n"
				 "    fragColor = vec4(float(red) / 255, 0.0, 1.0, 1.0);\n"
				 "}\n";

	gl.genTextures(1, &m_texture);
	GLU_EXPECT_NO_ERROR(gl.getError(), "genTextures");
	gl.bindTexture(GL_TEXTURE_2D, m_texture);
	GLU_EXPECT_NO_ERROR(gl.getError(), "bindTexture");
	gl.texStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, 32, 32);
	GLU_EXPECT_NO_ERROR(gl.getError(), "texStorage2D");

	gl.genFramebuffers(1, &m_fbo);
	GLU_EXPECT_NO_ERROR(gl.getError(), "genFramenuffers");
	gl.bindFramebuffer(GL_FRAMEBUFFER, m_fbo);
	GLU_EXPECT_NO_ERROR(gl.getError(), "bindFramebuffer");
	gl.framebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_texture, 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "framebufferTexture2D");

	gl.viewport(0, 0, 32, 32);
	GLU_EXPECT_NO_ERROR(gl.getError(), "viewport");
}

/** Stub de-init method */
void SpirvGlslToSpirVSpecializationConstantsTest::deinit()
{
	const Functions& gl = m_context.getRenderContext().getFunctions();

	if (m_fbo)
	{
		gl.deleteFramebuffers(1, &m_fbo);
		GLU_EXPECT_NO_ERROR(gl.getError(), "deleteFramebuffers");
	}
	if (m_texture)
	{
		gl.deleteTextures(1, &m_texture);
		GLU_EXPECT_NO_ERROR(gl.getError(), "deleteTextures");
	}
}

/** Executes test iteration.
 *
 *  @return Returns STOP when test has finished executing, CONTINUE if more iterations are needed.
 */
tcu::TestNode::IterateResult SpirvGlslToSpirVSpecializationConstantsTest::iterate()
{
	const Functions& gl = m_context.getRenderContext().getFunctions();

	const GLfloat vertices[] = { -1.0f, -1.0f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f };

	GLuint vao;
	gl.genVertexArrays(1, &vao);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenVertexArrays");
	gl.bindVertexArray(vao);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindVertexArray");

	GLuint vbo;
	gl.genBuffers(1, &vbo);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenBuffers");
	gl.bindBuffer(GL_ARRAY_BUFFER, vbo);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer");

	gl.bufferData(GL_ARRAY_BUFFER, 9 * sizeof(GLfloat), (GLvoid*)vertices, GL_DYNAMIC_DRAW);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferData");

	ShaderBinary vertexBinary;
	ShaderBinary fragmentBinary;
#if defined		 DEQP_HAVE_GLSLANG
	{
		vertexBinary   = glslangUtils::makeSpirV(m_context.getTestContext().getLog(), VertexSource(m_vertex));
		fragmentBinary = glslangUtils::makeSpirV(m_context.getTestContext().getLog(), FragmentSource(m_fragment));
	}
#else  // DEQP_HAVE_GLSLANG
	{
		tcu::Archive& archive = m_testCtx.getArchive();
		vertexBinary =
			commonUtils::readSpirV(archive.getResource("spirv/glsl_to_spirv_specialization_constants/vertex.nspv"));
		fragmentBinary =
			commonUtils::readSpirV(archive.getResource("spirv/glsl_to_spirv_specialization_constants/fragment.nspv"));
	}
#endif // DEQP_HAVE_GLSLANG
	fragmentBinary << SpecializationData(10, 128);

	ProgramBinaries binaries;
	binaries << vertexBinary;
	binaries << fragmentBinary;
	ShaderProgram spirvProgram(gl, binaries);

	if (!spirvProgram.isOk())
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Shader compilation failed.\n"
						   << "Vertex:\n"
						   << m_vertex.c_str() << "Fragment:\n"
						   << m_fragment.c_str() << "InfoLog:\n"
						   << spirvProgram.getShaderInfo(SHADERTYPE_VERTEX).infoLog << tcu::TestLog::EndMessage;

		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
		return STOP;
	}

	gl.useProgram(spirvProgram.getProgram());
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram");

	gl.clearColor(0.0f, 0.0f, 0.0f, 1.0f);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glClearColor");
	gl.clear(GL_COLOR_BUFFER_BIT);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glClear");

	gl.enableVertexAttribArray(0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glEnableVertexAttribArray");

	gl.vertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, DE_NULL);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glVertexAttribPointer");

	gl.drawArrays(GL_TRIANGLES, 0, 3);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawArray");

	gl.disableVertexAttribArray(0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glDisableVertexAttribArray");

	GLuint output;

	gl.readPixels(16, 16, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, (GLvoid*)&output);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glReadPixels");

	if (output != 0xFFFF0080)
	{
		m_testCtx.getLog() << tcu::TestLog::Message
						   << "Color value read from framebuffer is wrong. Expected: " << 0xFFFF0080
						   << ", Read: " << output << tcu::TestLog::EndMessage;

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
SpirvValidationBuiltInVariableDecorationsTest::SpirvValidationBuiltInVariableDecorationsTest(deqp::Context& context)
	: TestCase(context, "spirv_validation_builtin_variable_decorations_test",
			   "Test verifies if Spir-V built in variable decorations works as expected.")
{
	/* Left blank intentionally */
}

/** Stub init method */
void SpirvValidationBuiltInVariableDecorationsTest::init()
{
	commonUtils::checkGlSpirvSupported(m_context);

	m_compute = "#version 450\n"
				"\n"
				"layout (local_size_x = 1, local_size_y = 2, local_size_z = 1) in;\n"
				"\n"
				"layout (location = 0, rgba8ui) uniform uimage2D img0;\n"
				"layout (location = 1, rgba8ui) uniform uimage2D img1;\n"
				"layout (location = 2, rgba8ui) uniform uimage2D img2;\n"
				"layout (location = 3, rgba8ui) uniform uimage2D img3;\n"
				"layout (location = 4, rgba8ui) uniform uimage2D img4;\n"
				"\n"
				"void main()\n"
				"{\n"
				"    ivec3 point = ivec3(gl_GlobalInvocationID);\n"
				"    uvec3 color0 = uvec3(gl_NumWorkGroups);\n"
				"    uvec3 color1 = uvec3(gl_WorkGroupSize);\n"
				"    uvec3 color2 = uvec3(gl_WorkGroupID);\n"
				"    uvec3 color3 = uvec3(gl_LocalInvocationID);\n"
				"    uvec3 color4 = uvec3(gl_LocalInvocationIndex);\n"
				"    imageStore(img0, point.xy, uvec4(color0, 0xFF));\n"
				"    imageStore(img1, point.xy, uvec4(color1, 0xFF));\n"
				"    imageStore(img2, point.xy, uvec4(color2, 0xFF));\n"
				"    imageStore(img3, point.xy, uvec4(color3, 0xFF));\n"
				"    imageStore(img4, point.xy, uvec4(color4, 0xFF));\n"
				"    memoryBarrier();\n"
				"}\n";

	m_vertex = "#version 450\n"
			   "\n"
			   "layout (location = 0) in vec3 position;\n"
			   "\n"
			   "layout (location = 1) out vec4 vColor;\n"
			   "\n"
			   "void main()\n"
			   "{\n"
			   "    gl_PointSize = 10.0f;\n"
			   "    gl_Position = vec4(position.x, position.y + 0.3 * gl_InstanceID, position.z, 1.0);\n"
			   "    gl_ClipDistance[0] = <CLIP_DISTANCE>;\n"
			   "    gl_CullDistance[0] = <CULL_DISTANCE>;\n"
			   "    vColor = <VERTEX_COLOR>;\n"
			   "}\n";

	m_tesselationCtrl = "#version 450\n"
						"\n"
						"layout (vertices = 3) out;\n"
						"\n"
						"layout (location = 1) in vec4 vColor[];\n"
						"layout (location = 2) out vec4 tcColor[];\n"
						"\n"
						"void main()\n"
						"{\n"
						"    tcColor[gl_InvocationID] = vColor[gl_InvocationID];\n"
						"    tcColor[gl_InvocationID].r = float(gl_PatchVerticesIn) / 3;\n"
						"\n"
						"    if (gl_InvocationID == 0) {\n"
						"        gl_TessLevelOuter[0] = 1.0;\n"
						"        gl_TessLevelOuter[1] = 1.0;\n"
						"        gl_TessLevelOuter[2] = 1.0;\n"
						"        gl_TessLevelInner[0] = 1.0;\n"
						"    }\n"
						"\n"
						"    gl_out[gl_InvocationID].gl_ClipDistance[0] = gl_in[gl_InvocationID].gl_ClipDistance[0];\n"
						"    gl_out[gl_InvocationID].gl_CullDistance[0] = gl_in[gl_InvocationID].gl_CullDistance[0];\n"
						"    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;\n"
						"}\n";

	m_tesselationEval = "#version 450\n"
						"\n"
						"layout (triangles) in;\n"
						"\n"
						"layout (location = 2) in vec4 tcColor[];\n"
						"layout (location = 3) out vec4 teColor;\n"
						"\n"
						"void main()\n"
						"{\n"
						"    teColor = tcColor[0];\n"
						"\n"
						"    gl_ClipDistance[0] = gl_in[0].gl_ClipDistance[0];\n"
						"    gl_CullDistance[0] = gl_in[0].gl_CullDistance[0];\n"
						"    gl_Position = gl_TessCoord.x * gl_in[0].gl_Position +\n"
						"                  gl_TessCoord.y * gl_in[1].gl_Position +\n"
						"                  gl_TessCoord.z * gl_in[2].gl_Position;\n"
						"}\n";

	m_geometry = "#version 450\n"
				 "\n"
				 "layout (triangles) in;\n"
				 "layout (triangle_strip, max_vertices = 3) out;\n"
				 "\n"
				 "layout (location = 3) in vec4 teColor[];\n"
				 "layout (location = 4) out vec4 gColor;\n"
				 "\n"
				 "void main()\n"
				 "{\n"
				 "    gColor = teColor[0];\n"
				 "    gColor.b = float(gl_PrimitiveIDIn);\n"
				 "\n"
				 "    gl_Layer = 1;\n"
				 "    gl_ViewportIndex = 1;\n"
				 "\n"
				 "    for (int i = 0; i < 3; ++i) {\n"
				 "        gl_ClipDistance[0] = gl_in[i].gl_ClipDistance[0];\n"
				 "        gl_CullDistance[0] = gl_in[i].gl_CullDistance[0];\n"
				 "        gl_Position = gl_in[i].gl_Position;\n"
				 "        EmitVertex();\n"
				 "    }\n"
				 "    EndPrimitive();\n"
				 "}\n";

	m_fragment = "#version 450\n"
				 "\n"
				 "layout (location = <INPUT_LOCATION>) in vec4 <INPUT_NAME>;\n"
				 "layout (location = 0) out vec4 fColor;\n"
				 "\n"
				 "void main()\n"
				 "{\n"
				 "    vec4 color = <INPUT_NAME>;\n"
				 "    <ADDITIONAL_CODE>\n"
				 "    fColor = color;\n"
				 "}\n";

	ValidationStruct validationCompute(&SpirvValidationBuiltInVariableDecorationsTest::validComputeFunc);
	validationCompute.shaders.push_back(ComputeSource(m_compute));
	m_validations.push_back(validationCompute);

	std::string clipNegativeVertex   = m_vertex;
	std::string clipNegativeFragment = m_fragment;
	commonUtils::replaceToken("<CLIP_DISTANCE>", "-1.0", clipNegativeVertex);
	commonUtils::replaceToken("<CULL_DISTANCE>", "1.0", clipNegativeVertex);
	commonUtils::replaceToken("<VERTEX_COLOR>", "vec4(1.0, 1.0, 1.0, 1.0)", clipNegativeVertex);
	commonUtils::replaceToken("<INPUT_LOCATION>", "1", clipNegativeFragment);
	commonUtils::replaceToken("<INPUT_NAME>", "vColor", clipNegativeFragment);
	commonUtils::replaceToken("<ADDITIONAL_CODE>", "", clipNegativeFragment);
	ValidationStruct validationClipNegative(&SpirvValidationBuiltInVariableDecorationsTest::validPerVertexFragFunc);
	validationClipNegative.shaders.push_back(VertexSource(clipNegativeVertex));
	validationClipNegative.shaders.push_back(FragmentSource(clipNegativeFragment));
	validationClipNegative.outputs.push_back(ValidationOutputStruct(32, 32, 0xFF000000));
	m_validations.push_back(validationClipNegative);

	std::string perVertexFragVertex   = m_vertex;
	std::string perVertexFragFragment = m_fragment;
	std::string fragCode			  = "vec4 coord = gl_FragCoord;\n"
						   "color = vec4(0.0, coord.s / 64, coord.t / 64, 1.0);\n";
	commonUtils::replaceToken("<CLIP_DISTANCE>", "1.0", perVertexFragVertex);
	commonUtils::replaceToken("<CULL_DISTANCE>", "1.0", perVertexFragVertex);
	commonUtils::replaceToken("<VERTEX_COLOR>", "vec4(1.0, 1.0, 1.0, 1.0)", perVertexFragVertex);
	commonUtils::replaceToken("<INPUT_LOCATION>", "1", perVertexFragFragment);
	commonUtils::replaceToken("<INPUT_NAME>", "vColor", perVertexFragFragment);
	commonUtils::replaceToken("<ADDITIONAL_CODE>", fragCode.c_str(), perVertexFragFragment);
	ValidationStruct validationFrag(&SpirvValidationBuiltInVariableDecorationsTest::validPerVertexFragFunc);
	validationFrag.shaders.push_back(VertexSource(perVertexFragVertex));
	validationFrag.shaders.push_back(FragmentSource(perVertexFragFragment));
	validationFrag.outputs.push_back(ValidationOutputStruct(32, 32, 0xFF7F7F00));
	m_validations.push_back(validationFrag);

	std::string perVertexPointVertex   = m_vertex;
	std::string perVertexPointFragment = m_fragment;
	std::string pointCode			   = "vec2 coord = gl_PointCoord;\n"
							"color.b = coord.s * coord.t;\n";
	commonUtils::replaceToken("<CLIP_DISTANCE>", "1.0", perVertexPointVertex);
	commonUtils::replaceToken("<CULL_DISTANCE>", "1.0", perVertexPointVertex);
	commonUtils::replaceToken("<VERTEX_COLOR>", "vec4(float(gl_VertexID) / 3, 0.0, 0.0, 1.0)", perVertexPointVertex);
	commonUtils::replaceToken("<INPUT_LOCATION>", "1", perVertexPointFragment);
	commonUtils::replaceToken("<INPUT_NAME>", "vColor", perVertexPointFragment);
	commonUtils::replaceToken("<ADDITIONAL_CODE>", pointCode.c_str(), perVertexPointFragment);
	ValidationStruct validationPoint(&SpirvValidationBuiltInVariableDecorationsTest::validPerVertexPointFunc);
	validationPoint.shaders.push_back(VertexSource(perVertexPointVertex));
	validationPoint.shaders.push_back(FragmentSource(perVertexPointFragment));
	validationPoint.outputs.push_back(ValidationOutputStruct(64, 64, 0xFF3F0055));
	validationPoint.outputs.push_back(ValidationOutputStruct(45, 45, 0xFF3F0000));
	validationPoint.outputs.push_back(ValidationOutputStruct(83, 83, 0xFF3F00AA));
	m_validations.push_back(validationPoint);

	std::string tessGeomVertex   = m_vertex;
	std::string tessGeomFragment = m_fragment;
	commonUtils::replaceToken("<CLIP_DISTANCE>", "1.0", tessGeomVertex);
	commonUtils::replaceToken("<CULL_DISTANCE>", "1.0", tessGeomVertex);
	commonUtils::replaceToken("<VERTEX_COLOR>", "vec4(1.0, 1.0, 1.0, 1.0)", tessGeomVertex);
	commonUtils::replaceToken("<INPUT_LOCATION>", "4", tessGeomFragment);
	commonUtils::replaceToken("<INPUT_NAME>", "gColor", tessGeomFragment);
	commonUtils::replaceToken("<ADDITIONAL_CODE>", "", tessGeomFragment);
	ValidationStruct validationTessGeom(&SpirvValidationBuiltInVariableDecorationsTest::validTesselationGeometryFunc);
	validationTessGeom.shaders.push_back(VertexSource(tessGeomVertex));
	validationTessGeom.shaders.push_back(TessellationControlSource(m_tesselationCtrl));
	validationTessGeom.shaders.push_back(TessellationEvaluationSource(m_tesselationEval));
	validationTessGeom.shaders.push_back(GeometrySource(m_geometry));
	validationTessGeom.shaders.push_back(FragmentSource(tessGeomFragment));
	validationTessGeom.outputs.push_back(ValidationOutputStruct(48, 32, 1, 0xFF00FFFF));
	m_validations.push_back(validationTessGeom);

	std::string multisampleVertex   = m_vertex;
	std::string multisampleFragment = m_fragment;
	std::string samplingCode		= "if (gl_SampleID == 0)\n"
							   "{\n"
							   "   vec2 sampPos = gl_SamplePosition;\n"
							   "	color = vec4(1.0, sampPos.x, sampPos.y, 1.0);\n"
							   "}\n"
							   "else\n"
							   "{\n"
							   "	color = vec4(0.0, 1.0, 0.0, 1.0);\n"
							   "}\n"
							   "gl_SampleMask[0] = 0x02;";
	commonUtils::replaceToken("<CLIP_DISTANCE>", "1.0", multisampleVertex);
	commonUtils::replaceToken("<CULL_DISTANCE>", "1.0", multisampleVertex);
	commonUtils::replaceToken("<VERTEX_COLOR>", "vec4(1.0, 1.0, 1.0, 1.0)", multisampleVertex);
	commonUtils::replaceToken("<INPUT_LOCATION>", "1", multisampleFragment);
	commonUtils::replaceToken("<INPUT_NAME>", "vColor", multisampleFragment);
	commonUtils::replaceToken("<ADDITIONAL_CODE>", samplingCode.c_str(), multisampleFragment);
	ValidationStruct validationMultisample(&SpirvValidationBuiltInVariableDecorationsTest::validMultiSamplingFunc);
	validationMultisample.shaders.push_back(VertexSource(multisampleVertex));
	validationMultisample.shaders.push_back(FragmentSource(multisampleFragment));
	validationMultisample.outputs.push_back(ValidationOutputStruct(16, 16, 0xFF00BC00));
	m_validations.push_back(validationMultisample);

	m_mappings["gl_NumWorkGroups"].push_back("BuiltIn NumWorkgroups");
	m_mappings["gl_WorkGroupSize"].push_back("BuiltIn WorkgroupSize");
	m_mappings["gl_WorkGroupID"].push_back("BuiltIn WorkgroupId");
	m_mappings["gl_LocalInvocationID"].push_back("BuiltIn LocalInvocationId");
	m_mappings["gl_GlobalInvocationID"].push_back("BuiltIn GlobalInvocationId");
	m_mappings["gl_LocalInvocationIndex"].push_back("BuiltIn LocalInvocationIndex");
	m_mappings["gl_VertexID"].push_back("BuiltIn VertexId");
	m_mappings["gl_InstanceID"].push_back("BuiltIn InstanceId");
	m_mappings["gl_Position"].push_back("BuiltIn Position");
	m_mappings["gl_PointSize"].push_back("BuiltIn PointSize");
	m_mappings["gl_ClipDistance"].push_back("BuiltIn ClipDistance");
	m_mappings["gl_CullDistance"].push_back("BuiltIn CullDistance");
	m_mappings["gl_PrimitiveIDIn"].push_back("BuiltIn PrimitiveId");
	m_mappings["gl_InvocationID"].push_back("BuiltIn InvocationId");
	m_mappings["gl_Layer"].push_back("BuiltIn Layer");
	m_mappings["gl_ViewportIndex"].push_back("BuiltIn ViewportIndex");
	m_mappings["gl_PatchVerticesIn"].push_back("BuiltIn PatchVertices");
	m_mappings["gl_TessLevelOuter"].push_back("BuiltIn TessLevelOuter");
	m_mappings["gl_TessLevelInner"].push_back("BuiltIn TessLevelInner");
	m_mappings["gl_TessCoord"].push_back("BuiltIn TessCoord");
	m_mappings["gl_FragCoord"].push_back("BuiltIn FragCoord");
	m_mappings["gl_FrontFacing"].push_back("BuiltIn FrontFacing");
	m_mappings["gl_PointCoord"].push_back("BuiltIn PointCoord");
	m_mappings["gl_SampleId"].push_back("BuiltIn SampleId");
	m_mappings["gl_SamplePosition"].push_back("BuiltIn SamplePosition");
	m_mappings["gl_SampleMask"].push_back("BuiltIn SampleMask");
}

/** Stub de-init method */
void SpirvValidationBuiltInVariableDecorationsTest::deinit()
{
}

/** Executes test iteration.
 *
 *  @return Returns STOP when test has finished executing, CONTINUE if more iterations are needed.
 */
tcu::TestNode::IterateResult SpirvValidationBuiltInVariableDecorationsTest::iterate()
{
	const Functions& gl = m_context.getRenderContext().getFunctions();

	GLuint vao;
	gl.genVertexArrays(1, &vao);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenVertexArrays");
	gl.bindVertexArray(vao);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindVertexArray");

	GLuint vbo;
	gl.genBuffers(1, &vbo);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenBuffers");
	gl.bindBuffer(GL_ARRAY_BUFFER, vbo);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer");

	enum Iterates
	{
		ITERATE_GLSL,
		ITERATE_SPIRV,
		ITERATE_LAST
	};

	bool result = true;

	for (int v = 0; v < m_validations.size(); ++v)
	{
		for (int it = ITERATE_GLSL; it < ITERATE_LAST; ++it)
		{
			ShaderProgram* program = DE_NULL;
			if (it == ITERATE_GLSL)
			{
				ProgramSources sources;
				for (int s = 0; s < m_validations[v].shaders.size(); ++s)
					sources << m_validations[v].shaders[s];

				program = new ShaderProgram(gl, sources);
			}
			else if (it == ITERATE_SPIRV)
			{
				std::vector<ShaderBinary> binariesVec;

#if defined						DEQP_HAVE_GLSLANG
				ProgramBinaries binaries;
				for (int s = 0; s < m_validations[v].shaders.size(); ++s)
				{
					ShaderBinary shaderBinary =
						glslangUtils::makeSpirV(m_context.getTestContext().getLog(), m_validations[v].shaders[s]);
					binariesVec.push_back(shaderBinary);
					binaries << shaderBinary;
				}
#else  // DEQP_HAVE_GLSLANG
				tcu::Archive&   archive = m_testCtx.getArchive();
				ProgramBinaries binaries;
				for (int s = 0; s < m_validations[v].shaders.size(); ++s)
				{
					std::stringstream ss;
					ss << "spirv/spirv_validation_builtin_variable_decorations/shader_" << v << "_" << s << ".nspv";

					ShaderBinary shaderBinary = commonUtils::readSpirV(archive.getResource(ss.str().c_str()));
					binariesVec.push_back(shaderBinary);
					binaries << shaderBinary;
				}
#endif // DEQP_HAVE_GLSLANG
				program = new ShaderProgram(gl, binaries);

#if defined					DEQP_HAVE_SPIRV_TOOLS
				std::string spirvSource;

				for (int s = 0; s < m_validations[v].shaders.size(); ++s)
				{
					ShaderSource shaderSource = m_validations[v].shaders[s];

					glslangUtils::spirvDisassemble(spirvSource, binariesVec[s].binary);

					if (!glslangUtils::verifyMappings(shaderSource.source, spirvSource, m_mappings, true))
					{
						m_testCtx.getLog() << tcu::TestLog::Message << "Mappings for shader failed.\n"
										   << "GLSL source:\n"
										   << shaderSource.source.c_str() << "\n"
										   << "SpirV source:\n"
										   << spirvSource.c_str() << tcu::TestLog::EndMessage;

						TCU_THROW(InternalError, "Mappings for shader failed.");
					}
				}
#endif // DEQP_HAVE_SPIRV_TOOLS
			}

			if (!program->isOk())
			{
				std::stringstream message;
				message << "Shader build failed.\n";

				if (program->hasShader(SHADERTYPE_COMPUTE))
					message << "ComputeInfo: " << program->getShaderInfo(SHADERTYPE_COMPUTE).infoLog << "\n"
							<< "ComputeSource: " << program->getShader(SHADERTYPE_COMPUTE)->getSource() << "\n";
				if (program->hasShader(SHADERTYPE_VERTEX))
					message << "VertexInfo: " << program->getShaderInfo(SHADERTYPE_VERTEX).infoLog << "\n"
							<< "VertexSource: " << program->getShader(SHADERTYPE_VERTEX)->getSource() << "\n";
				if (program->hasShader(SHADERTYPE_TESSELLATION_CONTROL))
					message << "TesselationCtrlInfo: "
							<< program->getShaderInfo(SHADERTYPE_TESSELLATION_CONTROL).infoLog << "\n"
							<< "TesselationCtrlSource: "
							<< program->getShader(SHADERTYPE_TESSELLATION_CONTROL)->getSource() << "\n";
				if (program->hasShader(SHADERTYPE_TESSELLATION_EVALUATION))
					message << "TesselationEvalInfo: "
							<< program->getShaderInfo(SHADERTYPE_TESSELLATION_EVALUATION).infoLog << "\n"
							<< "TesselationEvalSource: "
							<< program->getShader(SHADERTYPE_TESSELLATION_EVALUATION)->getSource() << "\n";
				if (program->hasShader(SHADERTYPE_GEOMETRY))
					message << "GeometryInfo: " << program->getShaderInfo(SHADERTYPE_GEOMETRY).infoLog << "\n"
							<< "GeometrySource: " << program->getShader(SHADERTYPE_GEOMETRY)->getSource() << "\n";
				if (program->hasShader(SHADERTYPE_FRAGMENT))
					message << "FragmentInfo: " << program->getShaderInfo(SHADERTYPE_FRAGMENT).infoLog << "\n"
							<< "FragmentSource: " << program->getShader(SHADERTYPE_FRAGMENT)->getSource() << "\n";

				message << "ProgramInfo: " << program->getProgramInfo().infoLog;

				m_testCtx.getLog() << tcu::TestLog::Message << message.str() << tcu::TestLog::EndMessage;

				m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
				return STOP;
			}

			gl.useProgram(program->getProgram());
			GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram");

			ValidationFuncPtr funcPtr = m_validations[v].validationFuncPtr;
			result					  = (this->*funcPtr)(m_validations[v].outputs);

			if (program)
				delete program;

			if (!result)
			{
				m_testCtx.getLog() << tcu::TestLog::Message << "Validation " << v << " failed!"
								   << tcu::TestLog::EndMessage;

				break;
			}
		}
	}

	if (vbo)
	{
		gl.deleteBuffers(1, &vbo);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glDeleteBuffers");
	}

	if (vao)
	{
		gl.deleteVertexArrays(1, &vao);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glDeleteVertexArrays");
	}

	if (result)
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	else
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
	return STOP;
}

bool SpirvValidationBuiltInVariableDecorationsTest::validComputeFunc(ValidationOutputVec& outputs)
{
	const Functions& gl = m_context.getRenderContext().getFunctions();

	GLuint textures[5];

	gl.genTextures(5, textures);
	GLU_EXPECT_NO_ERROR(gl.getError(), "genTextures");
	for (int i = 0; i < 5; ++i)
	{
		gl.bindTexture(GL_TEXTURE_2D, textures[i]);
		GLU_EXPECT_NO_ERROR(gl.getError(), "bindTexture");
		gl.texStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8UI, 4, 4);
		GLU_EXPECT_NO_ERROR(gl.getError(), "texStorage2D");
	}

	gl.bindImageTexture(0, textures[0], 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA8UI);
	GLU_EXPECT_NO_ERROR(gl.getError(), "bindImageTexture");
	gl.bindImageTexture(1, textures[1], 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA8UI);
	GLU_EXPECT_NO_ERROR(gl.getError(), "bindImageTexture");
	gl.bindImageTexture(2, textures[2], 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA8UI);
	GLU_EXPECT_NO_ERROR(gl.getError(), "bindImageTexture");
	gl.bindImageTexture(3, textures[3], 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA8UI);
	GLU_EXPECT_NO_ERROR(gl.getError(), "bindImageTexture");
	gl.bindImageTexture(4, textures[4], 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA8UI);
	GLU_EXPECT_NO_ERROR(gl.getError(), "bindImageTexture");
	gl.uniform1i(0, 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "uniform1i");
	gl.uniform1i(1, 1);
	GLU_EXPECT_NO_ERROR(gl.getError(), "uniform1i");
	gl.uniform1i(2, 2);
	GLU_EXPECT_NO_ERROR(gl.getError(), "uniform1i");
	gl.uniform1i(3, 3);
	GLU_EXPECT_NO_ERROR(gl.getError(), "uniform1i");
	gl.uniform1i(4, 4);
	GLU_EXPECT_NO_ERROR(gl.getError(), "uniform1i");
	gl.dispatchCompute(4, 2, 1);
	GLU_EXPECT_NO_ERROR(gl.getError(), "dispatchCompute");

	gl.memoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
	GLU_EXPECT_NO_ERROR(gl.getError(), "memoryBarrier");

	std::vector<GLubyte> expectedResults[5];
	for (int i = 0; i < 5; ++i)
	{
		for (int y = 0; y < 4; ++y)
		{
			for (int x = 0; x < 4; ++x)
			{
				//"uvec3 color0 = uvec3(gl_NumWorkGroups);"
				if (i == 0)
				{
					expectedResults[i].push_back(4);
					expectedResults[i].push_back(2);
					expectedResults[i].push_back(1);
					expectedResults[i].push_back(0xFF);
				}
				//"uvec3 color1 = uvec3(gl_WorkGroupSize);"
				else if (i == 1)
				{
					expectedResults[i].push_back(1);
					expectedResults[i].push_back(2);
					expectedResults[i].push_back(1);
					expectedResults[i].push_back(0xFF);
				}
				//"uvec3 color2 = uvec3(gl_WorkGroupID);"
				else if (i == 2)
				{
					expectedResults[i].push_back(x);
					expectedResults[i].push_back(y / 2);
					expectedResults[i].push_back(0);
					expectedResults[i].push_back(0xFF);
				}
				//"uvec3 color3 = uvec3(gl_LocalInvocationID);"
				else if (i == 3)
				{
					expectedResults[i].push_back(0);
					expectedResults[i].push_back(y % 2);
					expectedResults[i].push_back(0);
					expectedResults[i].push_back(0xFF);
				}
				//"uvec3 color4 = uvec3(gl_LocalInvocationIndex);"
				else if (i == 4)
				{
					expectedResults[i].push_back(y % 2);
					expectedResults[i].push_back(y % 2);
					expectedResults[i].push_back(y % 2);
					expectedResults[i].push_back(0xFF);
				}
			}
		}
	}

	bool result = true;
	for (int i = 0; i < 5; ++i)
	{
		gl.bindTexture(GL_TEXTURE_2D, textures[i]);
		GLU_EXPECT_NO_ERROR(gl.getError(), "bindTexture");

		std::vector<GLubyte> pixels;
		pixels.resize(4 * 4 * 4);
		gl.getTexImage(GL_TEXTURE_2D, 0, GL_RGBA_INTEGER, GL_UNSIGNED_BYTE, (GLvoid*)pixels.data());
		GLU_EXPECT_NO_ERROR(gl.getError(), "getTexImage");

		if (pixels != expectedResults[i])
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "Invalid image computed [" << i << "]."
							   << tcu::TestLog::EndMessage;

			result = false;
		}
	}

	if (textures)
	{
		gl.deleteTextures(5, textures);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glDeleteTextures");
	}

	return result;
}

bool SpirvValidationBuiltInVariableDecorationsTest::validPerVertexFragFunc(ValidationOutputVec& outputs)
{
	const Functions& gl = m_context.getRenderContext().getFunctions();

	const GLfloat vertices[] = { -1.0f, -1.0f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f };

	GLuint texture;
	GLuint fbo;

	gl.genTextures(1, &texture);
	GLU_EXPECT_NO_ERROR(gl.getError(), "genTextures");
	gl.bindTexture(GL_TEXTURE_2D, texture);
	GLU_EXPECT_NO_ERROR(gl.getError(), "bindTexture");
	gl.texStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, 64, 64);
	GLU_EXPECT_NO_ERROR(gl.getError(), "texStorage2D");

	gl.genFramebuffers(1, &fbo);
	GLU_EXPECT_NO_ERROR(gl.getError(), "genFramenuffers");
	gl.bindFramebuffer(GL_FRAMEBUFFER, fbo);
	GLU_EXPECT_NO_ERROR(gl.getError(), "bindFramebuffer");
	gl.framebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "framebufferTexture2D");

	gl.viewport(0, 0, 64, 64);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glViewport");

	gl.bufferData(GL_ARRAY_BUFFER, 9 * sizeof(GLfloat), (GLvoid*)vertices, GL_DYNAMIC_DRAW);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferData");

	gl.enable(GL_CLIP_DISTANCE0);

	gl.clearColor(0.0f, 0.0f, 0.0f, 1.0f);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glClearColor");
	gl.clear(GL_COLOR_BUFFER_BIT);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glClear");

	gl.enableVertexAttribArray(0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glEnableVertexAttribArray");

	gl.vertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, DE_NULL);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glVertexAttribPointer");

	gl.drawArrays(GL_TRIANGLES, 0, 3);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawArray");

	gl.disableVertexAttribArray(0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glDisableVertexAttribArray");

	gl.disable(GL_CLIP_DISTANCE0);

	bool result = true;
	for (int o = 0; o < outputs.size(); ++o)
	{
		GLuint output;
		gl.readPixels(outputs[o].x, outputs[o].y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, (GLvoid*)&output);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glReadPixels");

		if (!commonUtils::compareUintColors(output, outputs[o].value, 2))
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "Invalid output color read at [" << (int)outputs[o].x << "/"
							   << (int)outputs[o].y << "]. Expected: " << outputs[o].value << ", "
							   << "Read: " << output << tcu::TestLog::EndMessage;

			result = false;
		}
	}

	if (fbo)
	{
		gl.deleteFramebuffers(1, &fbo);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glDeleteFramebuffers");
	}

	if (texture)
	{
		gl.deleteTextures(1, &texture);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glDeleteTextures");
	}

	return result;
}

bool SpirvValidationBuiltInVariableDecorationsTest::validPerVertexPointFunc(ValidationOutputVec& outputs)
{
	const Functions& gl = m_context.getRenderContext().getFunctions();

	const GLfloat vertices[] = { -0.3f, -0.3f, 0.0f, 0.0f, -0.3f, 0.0f, 0.3f, -0.3f, 0.0f };

	GLuint texture;
	GLuint fbo;

	gl.genTextures(1, &texture);
	GLU_EXPECT_NO_ERROR(gl.getError(), "genTextures");
	gl.bindTexture(GL_TEXTURE_2D, texture);
	GLU_EXPECT_NO_ERROR(gl.getError(), "bindTexture");
	gl.texStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, 128, 128);
	GLU_EXPECT_NO_ERROR(gl.getError(), "texStorage2D");

	gl.genFramebuffers(1, &fbo);
	GLU_EXPECT_NO_ERROR(gl.getError(), "genFramenuffers");
	gl.bindFramebuffer(GL_FRAMEBUFFER, fbo);
	GLU_EXPECT_NO_ERROR(gl.getError(), "bindFramebuffer");
	gl.framebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "framebufferTexture2D");

	gl.viewport(0, 0, 128, 128);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glViewport");

	gl.bufferData(GL_ARRAY_BUFFER, 9 * sizeof(GLfloat), (GLvoid*)vertices, GL_DYNAMIC_DRAW);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferData");

	gl.enable(GL_CLIP_DISTANCE0);
	gl.enable(GL_PROGRAM_POINT_SIZE);

	gl.clearColor(0.0f, 0.0f, 0.0f, 1.0f);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glClearColor");
	gl.clear(GL_COLOR_BUFFER_BIT);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glClear");

	gl.enableVertexAttribArray(0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glEnableVertexAttribArray");

	gl.vertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, DE_NULL);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glVertexAttribPointer");

	gl.drawArraysInstanced(GL_POINTS, 0, 3, 3);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawArray");

	gl.disableVertexAttribArray(0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glDisableVertexAttribArray");

	gl.disable(GL_PROGRAM_POINT_SIZE);
	gl.disable(GL_CLIP_DISTANCE0);

	bool result = true;
	for (int o = 0; o < outputs.size(); ++o)
	{
		GLuint output;
		gl.readPixels(outputs[o].x, outputs[o].y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, (GLvoid*)&output);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glReadPixels");

		if (!commonUtils::compareUintColors(output, outputs[o].value, 2))
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "Invalid output color read at [" << (int)outputs[o].x << "/"
							   << (int)outputs[o].y << "]. Expected: " << outputs[o].value << ", "
							   << "Read: " << output << tcu::TestLog::EndMessage;

			result = false;
		}
	}

	if (fbo)
	{
		gl.deleteFramebuffers(1, &fbo);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glDeleteFramebuffers");
	}

	if (texture)
	{
		gl.deleteTextures(1, &texture);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glDeleteTextures");
	}

	return result;
}

bool SpirvValidationBuiltInVariableDecorationsTest::validTesselationGeometryFunc(ValidationOutputVec& outputs)
{
	const Functions& gl = m_context.getRenderContext().getFunctions();

	const GLfloat vertices[] = { -1.0f, -1.0f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f };

	GLuint texture;
	GLuint fbo;

	gl.genTextures(1, &texture);
	GLU_EXPECT_NO_ERROR(gl.getError(), "genTextures");
	gl.bindTexture(GL_TEXTURE_2D_ARRAY, texture);
	GLU_EXPECT_NO_ERROR(gl.getError(), "bindTexture");
	gl.texStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_RGBA8, 64, 64, 2);
	GLU_EXPECT_NO_ERROR(gl.getError(), "texStorage2D");

	gl.genFramebuffers(1, &fbo);
	GLU_EXPECT_NO_ERROR(gl.getError(), "genFramenuffers");
	gl.bindFramebuffer(GL_FRAMEBUFFER, fbo);
	GLU_EXPECT_NO_ERROR(gl.getError(), "bindFramebuffer");
	gl.framebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, texture, 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "framebufferTexture2D");

	gl.viewportIndexedf(0, 0.0f, 0.0f, 32.0f, 64.0f);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glViewportIndexed");

	gl.viewportIndexedf(1, 32.0f, 0.0f, 32.0f, 64.0f);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glViewportIndexed");

	gl.bufferData(GL_ARRAY_BUFFER, 9 * sizeof(GLfloat), (GLvoid*)vertices, GL_DYNAMIC_DRAW);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferData");

	gl.clearColor(0.0f, 0.0f, 0.0f, 1.0f);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glClearColor");
	gl.clear(GL_COLOR_BUFFER_BIT);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glClear");

	gl.enableVertexAttribArray(0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glEnableVertexAttribArray");

	gl.vertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, DE_NULL);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glVertexAttribPointer");

	gl.patchParameteri(GL_PATCH_VERTICES, 3);
	gl.drawArrays(GL_PATCHES, 0, 3);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawArray");

	gl.disableVertexAttribArray(0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glDisableVertexAttribArray");

	gl.viewport(0, 0, 128, 64);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glViewport");

	std::vector<GLuint> pixels;
	pixels.resize(64 * 64 * 2);
	gl.getTexImage(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA, GL_UNSIGNED_BYTE, (GLvoid*)pixels.data());
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetTexImage");

	bool result = true;
	for (int o = 0; o < outputs.size(); ++o)
	{
		GLuint output = pixels[(outputs[o].x + outputs[o].y * 64) + outputs[o].z * 64 * 64];

		if (!commonUtils::compareUintColors(output, outputs[o].value, 2))
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "Invalid output color read at [" << (int)outputs[o].x << "/"
							   << (int)outputs[o].y << "/" << (int)outputs[o].z << "]. Expected: " << outputs[o].value
							   << ", "
							   << "Read: " << output << tcu::TestLog::EndMessage;

			result = false;
		}
	}

	if (fbo)
	{
		gl.deleteFramebuffers(1, &fbo);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glDeleteFramebuffers");
	}

	if (texture)
	{
		gl.deleteTextures(1, &texture);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glDeleteTextures");
	}

	return result;
}

bool SpirvValidationBuiltInVariableDecorationsTest::validMultiSamplingFunc(ValidationOutputVec& outputs)
{
	const Functions& gl = m_context.getRenderContext().getFunctions();

	const GLfloat vertices[] = { -1.0f, -1.0f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f };

	GLuint textureMS;
	GLuint texture;
	GLuint fboMS;
	GLuint fbo;

	gl.genTextures(1, &textureMS);
	GLU_EXPECT_NO_ERROR(gl.getError(), "genTextures");
	gl.bindTexture(GL_TEXTURE_2D_MULTISAMPLE, textureMS);
	GLU_EXPECT_NO_ERROR(gl.getError(), "bindTexture");
	gl.texStorage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 2, GL_RGBA8, 32, 32, GL_TRUE);
	GLU_EXPECT_NO_ERROR(gl.getError(), "texStorage2DMultisample");

	gl.genTextures(1, &texture);
	GLU_EXPECT_NO_ERROR(gl.getError(), "genTextures");
	gl.bindTexture(GL_TEXTURE_2D, texture);
	GLU_EXPECT_NO_ERROR(gl.getError(), "bindTexture");
	gl.texStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, 32, 32);
	GLU_EXPECT_NO_ERROR(gl.getError(), "texStorage2D");

	gl.genFramebuffers(1, &fboMS);
	GLU_EXPECT_NO_ERROR(gl.getError(), "genFramenuffers");
	gl.bindFramebuffer(GL_FRAMEBUFFER, fboMS);
	GLU_EXPECT_NO_ERROR(gl.getError(), "bindFramebuffer");
	gl.framebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, textureMS, 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "framebufferTexture2D");

	gl.genFramebuffers(1, &fbo);
	GLU_EXPECT_NO_ERROR(gl.getError(), "genFramenuffers");
	gl.bindFramebuffer(GL_FRAMEBUFFER, fbo);
	GLU_EXPECT_NO_ERROR(gl.getError(), "bindFramebuffer");
	gl.framebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "framebufferTexture2D");

	gl.bindFramebuffer(GL_FRAMEBUFFER, fboMS);
	GLU_EXPECT_NO_ERROR(gl.getError(), "bindFramebuffer");

	gl.viewport(0, 0, 32, 32);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glViewport");

	gl.bufferData(GL_ARRAY_BUFFER, 9 * sizeof(GLfloat), (GLvoid*)vertices, GL_DYNAMIC_DRAW);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferData");

	gl.enable(GL_CLIP_DISTANCE0);

	gl.clearColor(0.0f, 0.0f, 0.0f, 1.0f);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glClearColor");
	gl.clear(GL_COLOR_BUFFER_BIT);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glClear");

	gl.enableVertexAttribArray(0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glEnableVertexAttribArray");

	gl.vertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, DE_NULL);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glVertexAttribPointer");

	gl.drawArrays(GL_TRIANGLES, 0, 3);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawArray");

	gl.disableVertexAttribArray(0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glDisableVertexAttribArray");

	gl.disable(GL_CLIP_DISTANCE0);

	gl.bindFramebuffer(GL_READ_FRAMEBUFFER, fboMS);
	GLU_EXPECT_NO_ERROR(gl.getError(), "bindFramebuffer");
	gl.bindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "bindFramebuffer");

	gl.blitFramebuffer(0, 0, 32, 32, 0, 0, 32, 32, GL_COLOR_BUFFER_BIT, GL_NEAREST);
	GLU_EXPECT_NO_ERROR(gl.getError(), "blitFramebuffer");

	gl.bindFramebuffer(GL_FRAMEBUFFER, 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "bindFramebuffer");

	const int epsilon = 2;
	bool result = true;
	for (int o = 0; o < outputs.size(); ++o)
	{
		GLuint output;
		gl.readPixels(outputs[o].x, outputs[o].y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, (GLvoid*)&output);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glReadPixels");

		// The fragment shader for this case is rendering to a 2-sample FBO discarding
		// sample 0 and rendering 100% green to sample 1, so we expect a green output.
		// However, because sample locations may not be the same across implementations,
		// and that can influence their weights during the multisample resolve,
		// we can only check that there has to be some green in the output (since we know
		// that we have a green sample being selected) and that the level of green is not
		// 100% (since we know that pixel coverage is not 100% because we are
		// discarding one of the samples).

		int r1	= (output & 0xFF);
		int g1	= ((output >> 8) & 0xFF);
		int b1	= ((output >> 16) & 0xFF);
		int a1	= ((output >> 24) & 0xFF);

		int r2	= (outputs[o].value & 0xFF);
		int b2	= ((outputs[o].value >> 16) & 0xFF);
		int a2	= ((outputs[o].value >> 24) & 0xFF);

		if (r1 < r2 - epsilon || r1 > r2 + epsilon ||
		    g1 == 0x00 || g1 == 0xFF ||
		    b1 < b2 - epsilon || b1 > b2 + epsilon ||
		    a1 < a2 - epsilon || a1 > a2 + epsilon)
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "Invalid output color read at [" << (int)outputs[o].x << "/"
							   << (int)outputs[o].y << "]. Expected 0xff00xx00, with xx anything but ff or 00. "
							   << "Read: " << std::hex << output << tcu::TestLog::EndMessage;

			result = false;
		}
	}

	if (fboMS)
	{
		gl.deleteFramebuffers(1, &fboMS);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glDeleteFramebuffers");
	}

	if (fbo)
	{
		gl.deleteFramebuffers(1, &fbo);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glDeleteFramebuffers");
	}

	if (textureMS)
	{
		gl.deleteTextures(1, &texture);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glDeleteTextures");
	}

	if (texture)
	{
		gl.deleteTextures(1, &texture);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glDeleteTextures");
	}

	return result;
}

/** Constructor.
 *
 *  @param context     Rendering context
 *  @param name        Test name
 *  @param description Test description
 */
SpirvValidationCapabilitiesTest::SpirvValidationCapabilitiesTest(deqp::Context& context)
	: TestCase(context, "spirv_validation_capabilities_test", "Test verifies if Spir-V capabilities works as expected.")
{
	/* Left blank intentionally */
}

/** Stub init method */
void SpirvValidationCapabilitiesTest::init()
{
	ShaderStage computeStage;
	computeStage.source = ComputeSource("#version 450\n"
										"\n"
										"layout (local_size_x = 1, local_size_y = 2, local_size_z = 1) in;\n"
										"\n"
										"layout (location = 0, rgba8) uniform image2DMS img0;\n"
										"layout (location = 1, rgba8) uniform image2DMSArray img1;\n"
										"layout (location = 2, rgba8) uniform image2DRect img2;\n"
										"layout (location = 3, rgba8) uniform imageCube img3;\n"
										"layout (location = 4, rgba8) uniform imageCubeArray img4;\n"
										"layout (location = 5, rgba8) uniform imageBuffer img5;\n"
										"layout (location = 6, rgba8) uniform image2D img6;\n"
										"layout (location = 7, rgba8) uniform image1D img7;\n"
										"layout (location = 8) uniform writeonly image1D img8;\n"
										"layout (location = 9, rg32f) uniform image1D img9;\n"
										"layout (location = 10) uniform sampler2DRect img10;\n"
										"layout (location = 11) uniform samplerCubeArray img11;\n"
										"layout (location = 12) uniform samplerBuffer img12;\n"
										"layout (location = 13) uniform sampler1D img13;\n"
										"layout (location = 14) uniform sampler2D img14;\n"
										"\n"
										"layout (binding = 0) uniform atomic_uint atCounter;\n"
										"\n"
										"void main()\n"
										"{\n"
										"    ivec2 size = imageSize(img6);\n"
										"    ivec3 point = ivec3(gl_GlobalInvocationID);\n"
										"    imageStore(img0, point.xy, 0, vec4(0));\n"
										"    imageStore(img1, point, 0, vec4(0));\n"
										"    imageStore(img2, point.xy, vec4(0));\n"
										"    imageStore(img3, point, vec4(0));\n"
										"    imageStore(img4, point, vec4(0));\n"
										"    imageStore(img5, point.x, vec4(0));\n"
										"    imageStore(img6, point.xy, vec4(0));\n"
										"    imageStore(img7, point.x, vec4(0));\n"
										"    imageStore(img8, point.x, vec4(0));\n"
										"\n"
										"    vec3 coord = vec3(0);\n"
										"    ivec2 offset = ivec2(gl_GlobalInvocationID.xy);\n"
										"    vec4 color;\n"
										"    color = textureGather(img10, coord.xy);\n"
										"    color = textureGather(img11, vec4(0));\n"
										"    color = texelFetch(img12, point.x);\n"
										"    color = textureGatherOffset(img14, coord.xy, offset);\n"
										"    memoryBarrier();\n"
										"}\n");

	computeStage.caps.push_back("Shader");
	computeStage.caps.push_back("SampledRect Shader");
	computeStage.caps.push_back("SampledCubeArray Shader");
	computeStage.caps.push_back("SampledBuffer Shader");
	computeStage.caps.push_back("Sampled1D");
	computeStage.caps.push_back("ImageRect SampledRect Shader");
	computeStage.caps.push_back("Image1D Sampled1D");
	computeStage.caps.push_back("ImageCubeArray SampledCubeArray Shader");
	computeStage.caps.push_back("ImageBuffer SampledBuffer");
	computeStage.caps.push_back("ImageMSArray Shader");
	computeStage.caps.push_back("ImageQuery Shader");
	computeStage.caps.push_back("ImageGatherExtended Shader");
	computeStage.caps.push_back("StorageImageExtendedFormats Shader");
	computeStage.caps.push_back("StorageImageWriteWithoutFormat Shader");
	computeStage.caps.push_back("AtomicStorage Shader");

	ShaderStage vertexStage;
	vertexStage.source = VertexSource("#version 450\n"
									  "\n"
									  "layout (location = 0) in vec3 position;\n"
									  "layout (location = 1) in mat4 projMatrix;\n"
									  "\n"
									  "layout (location = 2, xfb_buffer = 0) out float xfbVal;\n"
									  "layout (location = 3) out vec2 texCoord;\n"
									  "\n"
									  "void main()\n"
									  "{\n"
									  "    double dval = double(position.x);\n"
									  "    gl_Position = vec4(position, 1.0) * projMatrix;\n"
									  "    gl_ClipDistance[0] = 0.0;\n"
									  "    gl_CullDistance[0] = 0.0;\n"
									  "\n"
									  "    xfbVal = 1.0;\n"
									  "    texCoord = vec2(0, 0);\n"
									  "}\n");

	vertexStage.caps.push_back("Matrix");
	vertexStage.caps.push_back("Shader Matrix");
	vertexStage.caps.push_back("Float64");
	vertexStage.caps.push_back("ClipDistance Shader");
	vertexStage.caps.push_back("CullDistance Shader");
	vertexStage.caps.push_back("TransformFeedback Shader");

	ShaderStage tessCtrlStage;
	tessCtrlStage.source =
		TessellationControlSource("#version 450\n"
								  "\n"
								  "layout (vertices = 3) out;\n"
								  "\n"
								  "void main()\n"
								  "{\n"
								  "    if (gl_InvocationID == 0) {\n"
								  "        gl_TessLevelOuter[0] = 1.0;\n"
								  "        gl_TessLevelOuter[1] = 1.0;\n"
								  "        gl_TessLevelOuter[2] = 1.0;\n"
								  "        gl_TessLevelInner[0] = 1.0;\n"
								  "    }\n"
								  "\n"
								  "    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;\n"
								  "    gl_out[gl_InvocationID].gl_PointSize = gl_in[gl_InvocationID].gl_PointSize;\n"
								  "}\n");

	tessCtrlStage.caps.push_back("Tessellation Shader");
	tessCtrlStage.caps.push_back("TessellationPointSize Tessellation");

	ShaderStage tessEvalStage;
	tessEvalStage.source = TessellationEvaluationSource("#version 450\n"
														"\n"
														"layout (triangles) in;\n"
														"\n"
														"void main()\n"
														"{\n"
														"    gl_Position = gl_TessCoord.x * gl_in[0].gl_Position +\n"
														"                  gl_TessCoord.y * gl_in[1].gl_Position +\n"
														"                  gl_TessCoord.z * gl_in[2].gl_Position;\n"
														"}\n");

	ShaderStage geometryStage;
	geometryStage.source = GeometrySource("#version 450\n"
										  "\n"
										  "layout (triangles) in;\n"
										  "layout (triangle_strip, max_vertices = 3) out;\n"
										  "\n"
										  "void main()\n"
										  "{\n"
										  "    gl_ViewportIndex = 0;\n"
										  "    for (int i = 0; i < 3; ++i) {\n"
										  "        gl_Position = gl_in[i].gl_Position;\n"
										  "        gl_PointSize = gl_in[i].gl_PointSize;\n"
										  "        EmitStreamVertex(0);\n"
										  "    }\n"
										  "    EndStreamPrimitive(0);\n"
										  "}\n");

	geometryStage.caps.push_back("Geometry Shader");
	geometryStage.caps.push_back("GeometryPointSize Geometry");
	geometryStage.caps.push_back("GeometryStreams Geometry");
	geometryStage.caps.push_back("MultiViewport Geometry");

	ShaderStage fragmentStage;
	fragmentStage.source = FragmentSource("#version 450\n"
										  "\n"
										  "layout (location = 3) in vec2 texCoord;\n"
										  "\n"
										  "layout (location = 0) out vec4 fColor;\n"
										  "\n"
										  "layout (location = 1) uniform sampler2D tex;\n"
										  "\n"
										  "void main()\n"
										  "{\n"
										  "    vec2 p = vec2(gl_SampleID);\n"
										  "    vec2 dx = dFdxFine(p);\n"
										  "\n"
										  "    interpolateAtCentroid(texCoord);"
										  "\n"
										  "    fColor = vec4(1.0);\n"
										  "}\n");

	fragmentStage.caps.push_back("Shader");
	fragmentStage.caps.push_back("DerivativeControl Shader");
	fragmentStage.caps.push_back("SampleRateShading");
	fragmentStage.caps.push_back("InterpolationFunction");

	ShaderStage dynamicIndexingStage;
	dynamicIndexingStage.source = ComputeSource("#version 450\n"
												"\n"
												"layout (location = 0) uniform sampler2D uniSamp[10];\n"
												"layout (location = 10, rgba8) uniform image2D uniImg[10];\n"
												"layout (binding = 5) uniform UniData\n"
												"{\n"
												"   int a[10];\n"
												"} uniBuff[10];\n"
												"layout (binding = 5) buffer StorageData\n"
												"{\n"
												"   int a[10];\n"
												"} storageBuff[10];\n"
												"\n"
												"void main()\n"
												"{\n"
												"    vec2 coord = vec2(0.0);\n"
												"    ivec2 point = ivec2(0);\n"
												"\n"
												"    int ret = 0;\n"
												"    for (int i = 0; i < 10; ++i)"
												"    {\n"
												"        ret = ret + uniBuff[i].a[i] + storageBuff[i].a[i];\n"
												"        textureGather(uniSamp[i], coord);\n"
												"        imageLoad(uniImg[i], point);\n"
												"    }\n"
												"    memoryBarrier();\n"
												"}\n");

	dynamicIndexingStage.caps.push_back("UniformBufferArrayDynamicIndexing");
	dynamicIndexingStage.caps.push_back("SampledImageArrayDynamicIndexing");
	dynamicIndexingStage.caps.push_back("StorageBufferArrayDynamicIndexing");
	dynamicIndexingStage.caps.push_back("StorageImageArrayDynamicIndexing");

	Pipeline computePipeline;
	computePipeline.push_back(computeStage);

	Pipeline standardPipeline;
	standardPipeline.push_back(vertexStage);
	standardPipeline.push_back(tessCtrlStage);
	standardPipeline.push_back(tessEvalStage);
	standardPipeline.push_back(geometryStage);
	standardPipeline.push_back(fragmentStage);

	Pipeline dynamicIndexingPipeline;
	dynamicIndexingPipeline.push_back(dynamicIndexingStage);

	m_pipelines.push_back(computePipeline);
	m_pipelines.push_back(standardPipeline);
	m_pipelines.push_back(dynamicIndexingPipeline);

	if (m_context.getContextInfo().isExtensionSupported("GL_ARB_gpu_shader_int64"))
	{
		ShaderStage computeStageExt("GL_ARB_gpu_shader_int64");
		computeStageExt.source = ComputeSource("#version 450\n"
											   "\n"
											   "#extension GL_ARB_gpu_shader_int64 : require\n"
											   "\n"
											   "void main()\n"
											   "{\n"
											   "    int64_t ival = int64_t(gl_GlobalInvocationID.x);\n"
											   "}\n");
		computeStageExt.caps.push_back("Int64");

		Pipeline extPipeline;
		extPipeline.push_back(computeStageExt);

		m_pipelines.push_back(extPipeline);
	}

	if (m_context.getContextInfo().isExtensionSupported("GL_ARB_sparse_texture2"))
	{
		ShaderStage computeStageExt("GL_ARB_sparse_texture2");
		computeStageExt.source = ComputeSource("#version 450\n"
											   "\n"
											   "#extension GL_ARB_sparse_texture2 : require\n"
											   "\n"
											   "layout (location = 0) uniform sampler2D tex;\n"
											   "\n"
											   "void main()\n"
											   "{\n"
											   "    vec2 p = vec2(0.0);\n"
											   "\n"
											   "    vec4 spCol;\n"
											   "    sparseTextureARB(tex, p, spCol);\n"
											   "}\n");

		computeStageExt.caps.push_back("SparseResidency");

		Pipeline extPipeline;
		extPipeline.push_back(computeStageExt);

		m_pipelines.push_back(extPipeline);

		if (m_context.getContextInfo().isExtensionSupported("GL_ARB_sparse_texture_clamp"))
		{
			ShaderStage vertexStageExt("GL_ARB_sparse_texture_clamp_vert");
			vertexStageExt.source = VertexSource("#version 450\n"
												 "\n"
												 "layout (location = 0) in vec4 pos;\n"
												 "\n"
												 "void main()\n"
												 "{\n"
												 "    gl_Position = pos;\n"
												 "}\n");

			ShaderStage fragmentStageExt("GL_ARB_sparse_texture_clamp_frag");
			fragmentStageExt.source = FragmentSource("#version 450\n"
													 "\n"
													 "#extension GL_ARB_sparse_texture2 : require\n"
													 "#extension GL_ARB_sparse_texture_clamp : require\n"
													 "\n"
													 "uniform sampler2D tex;\n"
													 "\n"
													 "layout (location = 0) out vec4 spCol;\n"
													 "\n"
													 "void main()\n"
													 "{\n"
													 "    vec2 p = vec2(0.0);\n"
													 "\n"
													 "    sparseTextureClampARB(tex, p, 0.5, spCol);\n"
													 "}\n");

			fragmentStageExt.caps.push_back("MinLod");

			Pipeline extPipeline;
			extPipeline.push_back(vertexStageExt);
			extPipeline.push_back(fragmentStageExt);

			m_pipelines.push_back(extPipeline);
		}
	}

	if (m_context.getContextInfo().isExtensionSupported("GL_EXT_shader_image_load_formatted"))
	{
		ShaderStage computeStageExt("GL_EXT_shader_image_load_formatted");
		computeStageExt.source = ComputeSource("#version 450\n"
											   "\n"
											   "#extension GL_EXT_shader_image_load_formatted : require\n"
											   "\n"
											   "layout (location = 0) uniform image2D img;\n"
											   "\n"
											   "void main()\n"
											   "{\n"
											   "    ivec3 point = ivec3(gl_GlobalInvocationID);\n"
											   "    vec4 color = imageLoad(img, point.xy);\n"
											   "}\n");

		computeStageExt.caps.push_back("StorageImageReadWithoutFormat");

		Pipeline extPipeline;
		extPipeline.push_back(computeStageExt);

		m_pipelines.push_back(extPipeline);
	}
}

/** Stub de-init method */
void SpirvValidationCapabilitiesTest::deinit()
{
}

/** Executes test iteration.
 *
 *  @return Returns STOP when test has finished executing, CONTINUE if more iterations are needed.
 */
tcu::TestNode::IterateResult SpirvValidationCapabilitiesTest::iterate()
{
	const Functions& gl = m_context.getRenderContext().getFunctions();

	bool result = true;

	for (int p = 0; p < m_pipelines.size(); ++p)
	{
		ProgramBinaries programBinaries;

		Pipeline& pipeline = m_pipelines[p];
		for (int s = 0; s < pipeline.size(); ++s)
		{
			ShaderStage& stage = pipeline[s];
#if defined				 DEQP_HAVE_GLSLANG
			stage.binary = glslangUtils::makeSpirV(m_context.getTestContext().getLog(), stage.source);
			std::stringstream ssw;
			if (stage.name.empty())
				ssw << "gl_cts/data/spirv/spirv_validation_capabilities/binary_p" << p << "s" << s << ".nspv";
			else
				ssw << "gl_cts/data/spirv/spirv_validation_capabilities/" << stage.name << ".nspv";
			commonUtils::writeSpirV(ssw.str().c_str(), stage.binary);
#else  // DEQP_HAVE_GLSLANG
			tcu::Archive&	 archive = m_testCtx.getArchive();
			std::stringstream ss;
			if (stage.name.empty())
				ss << "spirv/spirv_validation_capabilities/binary_p" << p << "s" << s << ".nspv";
			else
				ss << "spirv/spirv_validation_capabilities/" << stage.name << ".nspv";
			stage.binary = commonUtils::readSpirV(archive.getResource(ss.str().c_str()));
#endif // DEQP_HAVE_GLSLANG
			programBinaries << stage.binary;
		}

		ShaderProgram program(gl, programBinaries);
		if (!program.isOk())
		{
			std::stringstream ssLog;

			ssLog << "Program build failed [" << p << "].\n";
			if (program.hasShader(SHADERTYPE_COMPUTE))
				ssLog << "Compute: " << program.getShaderInfo(SHADERTYPE_COMPUTE).infoLog << "\n";
			if (program.hasShader(SHADERTYPE_VERTEX))
				ssLog << "Vertex: " << program.getShaderInfo(SHADERTYPE_VERTEX).infoLog << "\n";
			if (program.hasShader(SHADERTYPE_TESSELLATION_CONTROL))
				ssLog << "TessellationCtrl: " << program.getShaderInfo(SHADERTYPE_TESSELLATION_CONTROL).infoLog << "\n";
			if (program.hasShader(SHADERTYPE_TESSELLATION_EVALUATION))
				ssLog << "TessellationEval: " << program.getShaderInfo(SHADERTYPE_TESSELLATION_EVALUATION).infoLog
					  << "\n";
			if (program.hasShader(SHADERTYPE_GEOMETRY))
				ssLog << "Geometry: " << program.getShaderInfo(SHADERTYPE_GEOMETRY).infoLog << "\n";
			if (program.hasShader(SHADERTYPE_FRAGMENT))
				ssLog << "Fragment: " << program.getShaderInfo(SHADERTYPE_FRAGMENT).infoLog << "\n";
			ssLog << "Program: " << program.getProgramInfo().infoLog;

			m_testCtx.getLog() << tcu::TestLog::Message << ssLog.str() << tcu::TestLog::EndMessage;

			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
			return STOP;
		}

#if defined DEQP_HAVE_SPIRV_TOOLS
		for (int s = 0; s < pipeline.size(); ++s)
		{
			ShaderStage  stage  = pipeline[s];
			ShaderBinary binary = stage.binary;

			std::string spirVSource;
			glslangUtils::spirvDisassemble(spirVSource, binary.binary);

			for (int c = 0; c < stage.caps.size(); ++c)
			{
				std::string spirVSourceCut;
				int			foundCount = spirVCapabilityCutOff(spirVSource, spirVSourceCut, stage.caps, c);

				if (foundCount == 0)
				{
					m_testCtx.getLog()
						<< tcu::TestLog::Message << "OpCapability (" << stage.caps[c] << ") [" << p << "/" << s
						<< "].\n"
						<< "Neither capability nor capabilities that depends on this capability has been found."
						<< tcu::TestLog::EndMessage;
				}
				else
				{
					// Assemble and validate cut off SpirV source
					glslangUtils::spirvAssemble(binary.binary, spirVSourceCut);
					if (glslangUtils::spirvValidate(binary.binary, false))
					{
						m_testCtx.getLog() << tcu::TestLog::Message << "OpCapability (" << stage.caps[c] << ") [" << p
										   << "/" << s << "].\n"
										   << "Validation passed without corresponding OpCapability declared."
										   << tcu::TestLog::EndMessage;
					}
				}
			}
		}
#endif // DEQP_HAVE_SPIRV_TOOLS
	}

	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	return STOP;
}

int SpirvValidationCapabilitiesTest::spirVCapabilityCutOff(std::string spirVSrcInput, std::string& spirVSrcOutput,
														   CapabilitiesVec& capabilities, int& currentCapability)
{
	std::vector<std::string> current = de::splitString(capabilities[currentCapability], ' ');

	CapabilitiesVec toDisable;
	toDisable.push_back(current[0]);

	// Search for capabilities that depends on current one as it should be removed either
	for (int cr = 0; cr < capabilities.size(); ++cr)
	{
		std::vector<std::string> split = de::splitString(capabilities[cr], ' ');

		if (split[0] == current[0])
			continue;

		for (int s = 1; s < split.size(); ++s)
		{
			if (split[s] == current[0])
				toDisable.push_back(split[0]);
		}
	}

	// Disable current capability and capabilities that depends on it
	int foundCount = 0;
	spirVSrcOutput = spirVSrcInput;
	for (int d = 0; d < toDisable.size(); ++d)
	{
		std::string searchString = std::string("OpCapability ") + toDisable[d];

		size_t pos = spirVSrcOutput.find(searchString);

		if (pos != std::string::npos)
		{
			foundCount++;
			spirVSrcOutput.erase(pos, searchString.length());
		}
	}

	return foundCount;
}

/** Constructor.
 *
 *  @param context Rendering context.
 */
GlSpirvTests::GlSpirvTests(deqp::Context& context)
	: TestCaseGroup(context, "gl_spirv", "Verify conformance of ARB_gl_spirv implementation")
{
}

/** Initializes the test group contents. */
void GlSpirvTests::init()
{
	addChild(new SpirvModulesPositiveTest(m_context));
	addChild(new SpirvShaderBinaryMultipleShaderObjectsTest(m_context));
	addChild(new SpirvModulesStateQueriesTest(m_context));
	addChild(new SpirvModulesErrorVerificationTest(m_context));
	addChild(new SpirvGlslToSpirVEnableTest(m_context));
	addChild(new SpirvGlslToSpirVBuiltInFunctionsTest(m_context));
	addChild(new SpirvGlslToSpirVSpecializationConstantsTest(m_context));
	addChild(new SpirvValidationBuiltInVariableDecorationsTest(m_context));
	addChild(new SpirvValidationCapabilitiesTest(m_context));
}

} /* gl4cts namespace */
