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
 * \brief Negative Sample Variables Tests
 *//*--------------------------------------------------------------------*/

#include "es31fNegativeSampleVariablesTests.hpp"
#include "gluShaderProgram.hpp"

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

enum ExpectResult
{
	EXPECT_RESULT_PASS = 0,
	EXPECT_RESULT_FAIL,
	EXPECT_RESULT_LAST
};

void verifyShader (NegativeTestContext& ctx, glu::ShaderType shaderType, std::string shaderSource, ExpectResult expect)
{
	DE_ASSERT(expect >= EXPECT_RESULT_PASS && expect < EXPECT_RESULT_LAST);

	tcu::TestLog&		log			= ctx.getLog();
	bool				testFailed	= false;
	const char* const	source		= shaderSource.c_str();
	const int			length		= (int) shaderSource.size();
	glu::Shader			shader		(ctx.getRenderContext(), shaderType);
	std::string			message;

	shader.setSources(1, &source, &length);
	shader.compile();

	log << shader;

	if (expect == EXPECT_RESULT_PASS)
	{
		testFailed = !shader.getCompileStatus();
		message = "Shader did not compile.";
	}
	else
	{
		testFailed = shader.getCompileStatus();
		message = "Shader was not expected to compile.";
	}

	if (testFailed)
	{
		log << tcu::TestLog::Message << message << tcu::TestLog::EndMessage;
		ctx.fail(message);
	}
}

std::string getVersionAndExtension (NegativeTestContext& ctx)
{
	const bool				isES32	= contextSupports(ctx.getRenderContext().getType(), glu::ApiType::es(3, 2));
	const glu::GLSLVersion	version	= isES32 ? glu::GLSL_VERSION_320_ES : glu::GLSL_VERSION_310_ES;

	std::string versionAndExtension = glu::getGLSLVersionDeclaration(version);
	versionAndExtension += " \n";

	if (!isES32)
		versionAndExtension += "#extension GL_OES_sample_variables : require \n";

	return versionAndExtension;
}

void checkSupported (NegativeTestContext& ctx)
{
	const bool isES32 = contextSupports(ctx.getRenderContext().getType(), glu::ApiType::es(3, 2));

	if (!isES32 && !ctx.isExtensionSupported("GL_OES_sample_variables"))
		TCU_THROW(NotSupportedError, "GL_OES_sample_variables is not supported.");
}

void write_to_read_only_types (NegativeTestContext& ctx)
{
	checkSupported(ctx);

	std::ostringstream	shader;

	struct testConfig
	{
		std::string builtInType;
		std::string varyingCheck;
	} testConfigs[] =
	{
		{"gl_SampleID",			"	lowp int writeValue = 1; \n	gl_SampleID = writeValue; \n"},
		{"gl_SamplePosition",	"	mediump vec2 writeValue = vec2(1.0f, 1.0f); \n	gl_SamplePosition = writeValue; \n"},
		{"gl_SampleMaskIn",		"	lowp int writeValue = 1; \n	gl_SampleMaskIn[0] = writeValue; \n"}
	};

	for (int idx = 0; idx < DE_LENGTH_OF_ARRAY(testConfigs); idx++)
	{
		shader.str("");
		shader
			<< getVersionAndExtension(ctx)
			<< "layout (location = 0) out mediump vec4 fs_color; \n"
			<< "void main() \n"
			<< "{ \n"
			<<		testConfigs[idx].varyingCheck
			<< "	fs_color = vec4(1.0f, 0.0f, 0.0f, 1.0f); \n"
			<< "} \n";

		ctx.beginSection("OES_sample_variables: trying to write to built-in read-only variable" + testConfigs[idx].builtInType);
		verifyShader(ctx, glu::SHADERTYPE_FRAGMENT, shader.str(), EXPECT_RESULT_FAIL);
		ctx.endSection();
	}
}

void access_built_in_types_inside_other_shaders (NegativeTestContext& ctx)
{
	checkSupported(ctx);

	if ((!ctx.isExtensionSupported("GL_EXT_tessellation_shader") && !ctx.isExtensionSupported("GL_OES_tessellation_shader")) ||
		(!ctx.isExtensionSupported("GL_EXT_geometry_shader") && !ctx.isExtensionSupported("GL_OES_geometry_shader")))
	{
		TCU_THROW(NotSupportedError, "tessellation and geometry shader extensions not supported");
	}

	std::ostringstream	shader;

	struct testConfig
	{
		std::string builtInType;
		std::string varyingCheck;
	} testConfigs[] =
	{
		{"gl_SampleID",			"	lowp int writeValue = 1; \n	gl_SampleID = writeValue; \n"},
		{"gl_SamplePosition",	"	mediump vec2 writeValue = vec2(1.0f, 1.0f); \n	gl_SamplePosition = writeValue; \n"},
		{"gl_SampleMaskIn",		"	lowp int writeValue = 1; \n	gl_SampleMaskIn[0] = writeValue; \n"},
		{"gl_SampleMask",		"	highp int readValue = gl_SampleMask[0]; \n"},
	};

	for (int idx = 0; idx < DE_LENGTH_OF_ARRAY(testConfigs); idx++)
	{
		shader.str("");
		shader
			<< getVersionAndExtension(ctx)
			<< "void main () \n"
			<< "{ \n"
			<<		testConfigs[idx].varyingCheck
			<< "	gl_Position = vec4(1.0f, 0.0f, 0.0f , 1.0f); \n"
			<< "} \n";

		ctx.beginSection("OES_sample_variables: trying to use fragment shader built-in sampler variable " + testConfigs[idx].builtInType + " inside vertex shader");
		verifyShader(ctx, glu::SHADERTYPE_VERTEX, shader.str(), EXPECT_RESULT_FAIL);
		ctx.endSection();
	}

	for (int idx = 0; idx < DE_LENGTH_OF_ARRAY(testConfigs); idx++)
	{
		shader.str("");
		shader
			<< getVersionAndExtension(ctx)
			<< "layout (vertices = 3) out; \n"
			<< "void main () \n"
			<< "{ \n"
			<<		testConfigs[idx].varyingCheck
			<< "} \n";

		ctx.beginSection("OES_sample_variables: trying to use fragment shader built-in sampler variable " + testConfigs[idx].builtInType + " inside tessellation control shader");
		verifyShader(ctx, glu::SHADERTYPE_TESSELLATION_CONTROL, shader.str(), EXPECT_RESULT_FAIL);
		ctx.endSection();
	}

	for (int idx = 0; idx < DE_LENGTH_OF_ARRAY(testConfigs); idx++)
	{
		shader.str("");
		shader
			<< getVersionAndExtension(ctx)
			<< "layout (triangles, equal_spacing, ccw) in; \n"
			<< "void main () \n"
			<< "{ \n"
			<<		testConfigs[idx].varyingCheck
			<< "} \n";

		ctx.beginSection("OES_sample_variables: trying to use fragment shader built-in sampler variable " + testConfigs[idx].builtInType + " inside tessellation evaluation shader");
		verifyShader(ctx, glu::SHADERTYPE_TESSELLATION_EVALUATION, shader.str(), EXPECT_RESULT_FAIL);
		ctx.endSection();
	}

	for (int idx = 0; idx < DE_LENGTH_OF_ARRAY(testConfigs); idx++)
	{
		shader.str("");
		shader
			<< getVersionAndExtension(ctx)
			<< "layout (triangles) in; \n"
			<< "layout (triangle_strip, max_vertices = 32) out; \n"
			<< "void main () \n"
			<< "{ \n"
			<<		testConfigs[idx].varyingCheck
			<< "} \n";

		ctx.beginSection("OES_sample_variables: trying to use fragment shader built-in sampler variable " + testConfigs[idx].builtInType + " inside geometry shader");
		verifyShader(ctx, glu::SHADERTYPE_GEOMETRY, shader.str(), EXPECT_RESULT_FAIL);
		ctx.endSection();
	}
}

void index_outside_sample_mask_range (NegativeTestContext& ctx)
{
	checkSupported(ctx);

	std::ostringstream	shader;
	const int			MAX_TYPES	= 2;
	const int			MAX_INDEXES = 2;

	struct testConfig
	{
		std::string builtInType[MAX_TYPES];
		std::string invalidIndex[MAX_INDEXES];
	} testConfigs =
	{
		{
			"gl_SampleMask",
			"gl_SampleMaskIn"
		},
		{
			"	const highp int invalidIndex = (gl_MaxSamples + 31) / 32; \n",
			"	const highp int invalidIndex = -1; \n"
		}
	};

	for (int typeIdx = 0; typeIdx < MAX_TYPES; typeIdx++)
	{
		for (int invalidIdx = 0; invalidIdx < MAX_INDEXES; invalidIdx++)
		{
			shader.str("");
			shader
				<< getVersionAndExtension(ctx)
				<< "layout (location = 0) out mediump vec4 fs_color; \n"
				<< "void main() \n"
				<< "{ \n"
				<<		testConfigs.invalidIndex[invalidIdx]
				<< "	highp int invalidValue = " << testConfigs.builtInType[typeIdx] << "[invalidIndex]; \n"
				<< "	fs_color = vec4(1.0f, 0.0f, 0.0f, 1.0f); \n"
				<< "} \n";

			ctx.beginSection("OES_sample_variables: using constant integral expression outside of " + testConfigs.builtInType[typeIdx] + " bounds");
			verifyShader(ctx, glu::SHADERTYPE_FRAGMENT, shader.str(), EXPECT_RESULT_FAIL);
			ctx.endSection();
		}
	}
}

void access_built_in_types_without_extension (NegativeTestContext& ctx)
{
	checkSupported(ctx);

	std::ostringstream	shader;

	struct testConfig
	{
		std::string builtInType;
		std::string varyingCheck;
	} testConfigs[] =
	{
		{"gl_SampleID",			"	lowp int writeValue = 1; \n	gl_SampleID = writeValue; \n"},
		{"gl_SamplePosition",	"	mediump vec2 writeValue = vec2(1.0f, 1.0f); \n	gl_SamplePosition = writeValue; \n"},
		{"gl_SampleMaskIn",		"	lowp int writeValue = 1; \n	gl_SampleMaskIn[0] = writeValue; \n"},
		{"gl_SampleMask",		"	highp int readValue = gl_SampleMask[0]; \n"},
	};

	for (int idx = 0; idx < DE_LENGTH_OF_ARRAY(testConfigs); idx++)
	{
		shader.str("");
		shader
			<< "#version 310 es \n"
			<< "layout (location = 0) out mediump vec4 fs_color; \n"
			<< "void main() \n"
			<< "{ \n"
			<<		testConfigs[idx].varyingCheck
			<< "	fs_color = vec4(1.0f, 0.0f, 0.0f, 1.0f); \n"
			<< "} \n";

		ctx.beginSection("OES_sample_variables: accessing built-in type " + testConfigs[idx].builtInType + " in shader version 310 ES without required extension");
		verifyShader(ctx, glu::SHADERTYPE_FRAGMENT, shader.str(), EXPECT_RESULT_FAIL);
		ctx.endSection();
	}
}

void redeclare_built_in_types (NegativeTestContext& ctx)
{
	checkSupported(ctx);

	std::ostringstream	shader;
	std::ostringstream	testName;

	const char* const testConfigs[] =
	{
		"gl_SampleID",
		"gl_SamplePosition",
		"gl_SampleMaskIn",
		"gl_SampleMask",
	};

	for (int idx = 0; idx < DE_LENGTH_OF_ARRAY(testConfigs); idx++)
	{
		shader.str("");
		shader
			<< getVersionAndExtension(ctx)
			<< "layout (location = 0) out mediump vec4 fs_color; \n"
			<< "uniform lowp int " << testConfigs[idx] << "; \n"
			<< "void main() \n"
			<< "{ \n"
			<< "	if (" << testConfigs[idx] << " == 0) \n"
			<< "		fs_color = vec4(1.0f, 0.0f, 0.0f, 1.0f); \n"
			<< "	else \n"
			<< "		fs_color = vec4(0.0f, 1.0f, 0.0f, 1.0f); \n"
			<< "} \n";

		testName.str("");
		testName << "OES_sample_variables: redeclare built-in type " << testConfigs[idx];
		ctx.beginSection(testName.str());
		verifyShader(ctx, glu::SHADERTYPE_FRAGMENT, shader.str(), EXPECT_RESULT_FAIL);
		ctx.endSection();
	}
}

} // anonymous

std::vector<FunctionContainer> getNegativeSampleVariablesTestFunctions (void)
{
	const FunctionContainer funcs[] =
	{
		{write_to_read_only_types,						"write_to_read_only_types",						"tests trying writing to read-only built-in sample variables"},
		{access_built_in_types_inside_other_shaders,	"access_built_in_types_inside_other_shaders",	"Tests try to access fragment shader sample variables in other shaders"},
		{index_outside_sample_mask_range,				"index_outside_sample_mask_range",				"tests try to index into built-in sample array types out of bounds"},
		{access_built_in_types_without_extension,		"access_built_in_types_without_extension",		"tests try to access built-in sample types without the correct extension using version 310 es"},
		{redeclare_built_in_types,						"redeclare_built_in_types",						"Tests try to redeclare built-in sample types"},
	};

	return std::vector<FunctionContainer>(DE_ARRAY_BEGIN(funcs), DE_ARRAY_END(funcs));
}

} // NegativeTestShared
} // Functional
} // gles31
} // deqp
