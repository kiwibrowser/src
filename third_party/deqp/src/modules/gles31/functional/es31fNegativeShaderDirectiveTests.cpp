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
 * \brief Negative Shader Directive Tests
 *//*--------------------------------------------------------------------*/

#include "es31fNegativeShaderDirectiveTests.hpp"

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

void verifyProgram(NegativeTestContext& ctx, glu::ProgramSources sources, ExpectResult expect)
{
	DE_ASSERT(expect >= EXPECT_RESULT_PASS && expect < EXPECT_RESULT_LAST);

	tcu::TestLog&				log			= ctx.getLog();
	const glu::ShaderProgram	program		(ctx.getRenderContext(), sources);
	bool						testFailed	= false;
	std::string					message;

	log << program;

	if (expect == EXPECT_RESULT_PASS)
	{
		testFailed = !program.getProgramInfo().linkOk;
		message = "Program did not link.";
	}
	else
	{
		testFailed = program.getProgramInfo().linkOk;
		message = "Program was not expected to link.";
	}

	if (testFailed)
	{
		log << tcu::TestLog::Message << message << tcu::TestLog::EndMessage;
		ctx.fail(message);
	}
}

void verifyShader(NegativeTestContext& ctx, glu::ShaderType shaderType, std::string shaderSource, ExpectResult expect)
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

void primitive_bounding_box (NegativeTestContext& ctx)
{
	if (ctx.isShaderSupported(glu::SHADERTYPE_TESSELLATION_CONTROL))
	{
		ctx.beginSection("GL_EXT_primitive_bounding_box features require enabling the extension in 310 es shaders.");
		std::ostringstream source;
		source <<	"#version 310 es\n"
					"void main()\n"
					"{\n"
					"	gl_BoundingBoxEXT[0] = vec4(0.0, 0.0, 0.0, 0.0);\n"
					"	gl_BoundingBoxEXT[1] = vec4(1.0, 1.0, 1.0, 1.0);\n"
					"}\n";
		verifyShader(ctx, glu::SHADERTYPE_TESSELLATION_CONTROL, source.str(), EXPECT_RESULT_FAIL);
		ctx.endSection();
	}

	if (contextSupports(ctx.getRenderContext().getType() , glu::ApiType::es(3, 2)))
	{
		ctx.beginSection("gl_BoundingBox does not require the OES/EXT suffix in a 320 es shader.");
		{
			const std::string source =	"#version 320 es\n"
										"layout(vertices = 3) out;\n"
										"void main()\n"
										"{\n"
										"	gl_BoundingBox[0] = vec4(0.0, 0.0, 0.0, 0.0);\n"
										"	gl_BoundingBox[1] = vec4(0.0, 0.0, 0.0, 0.0);\n"
										"}\n";
			verifyShader(ctx, glu::SHADERTYPE_TESSELLATION_CONTROL, source, EXPECT_RESULT_PASS);
		}
		ctx.endSection();

		ctx.beginSection("Invalid index used when assigning to gl_BoundingBox in 320 es shader.");
		{
			const std::string source =	"#version 320 es\n"
										"layout(vertices = 3) out;\n"
										"void main()\n"
										"{\n"
										"	gl_BoundingBox[0] = vec4(0.0, 0.0, 0.0, 0.0);\n"
										"	gl_BoundingBox[2] = vec4(0.0, 0.0, 0.0, 0.0);\n"
										"}\n";
			verifyShader(ctx, glu::SHADERTYPE_TESSELLATION_CONTROL, source, EXPECT_RESULT_FAIL);
		}
		ctx.endSection();

		ctx.beginSection("Invalid type assignment to per-patch output array in 320 es shader.");
		{
			const std::string source =	"#version 320 es\n"
										"layout(vertices = 3) out;\n"
										"void main()\n"
										"{\n"
										"	gl_BoundingBox[0] = ivec4(0, 0, 0, 0);\n"
										"	gl_BoundingBox[1] = ivec4(0, 0, 0, 0);\n"
										"}\n";
			verifyShader(ctx, glu::SHADERTYPE_TESSELLATION_CONTROL, source, EXPECT_RESULT_FAIL);
		}
		ctx.endSection();
	}
}

void blend_equation_advanced (NegativeTestContext& ctx)
{
	static const char* const s_qualifiers[] =
	{
		"blend_support_multiply",
		"blend_support_screen",
		"blend_support_overlay",
		"blend_support_darken",
		"blend_support_lighten",
		"blend_support_colordodge",
		"blend_support_colorburn",
		"blend_support_hardlight",
		"blend_support_softlight",
		"blend_support_difference",
		"blend_support_exclusion",
		"blend_support_hsl_hue",
		"blend_support_hsl_saturation",
		"blend_support_hsl_color",
		"blend_support_hsl_luminosity",
	};

	ctx.beginSection("GL_KHR_blend_equation_advanced features require enabling the extension in 310 es shaders.");
	for (int ndx = 0; ndx < DE_LENGTH_OF_ARRAY(s_qualifiers); ++ndx)
	{
		std::ostringstream source;
		source <<	"#version 310 es\n"
					"layout(" << s_qualifiers[ndx] << ") out;\n"
					"void main()\n"
					"{\n"
					"}\n";
		verifyShader(ctx, glu::SHADERTYPE_FRAGMENT, source.str(), EXPECT_RESULT_FAIL);
	}
	ctx.endSection();
}

void sample_variables (NegativeTestContext& ctx)
{
	TCU_CHECK_AND_THROW(NotSupportedError, contextSupports(ctx.getRenderContext().getType() , glu::ApiType::es(3, 2)), "Test requires a context version 3.2 or higher.");

	static const char* const s_tests[] =
	{
		"int sampleId = gl_SampleID;",
		"vec2 samplePos = gl_SamplePosition;",
		"int sampleMaskIn0 = gl_SampleMaskIn[0];",
		"int sampleMask0 = gl_SampleMask[0];",
		"int numSamples = gl_NumSamples;",
	};

	ctx.beginSection("GL_OES_sample_variables features require enabling the extension in 310 es shaders.");
	for (int ndx = 0; ndx < DE_LENGTH_OF_ARRAY(s_tests); ++ndx)
	{
		std::ostringstream source;
		source <<	"#version 310 es\n"
					"precision mediump float;\n"
					"void main()\n"
					"{\n"
					"	" << s_tests[ndx] << "\n"
					"}\n";
		verifyShader(ctx, glu::SHADERTYPE_FRAGMENT, source.str(), EXPECT_RESULT_FAIL);
	}
	ctx.endSection();
}

void shader_image_atomic (NegativeTestContext& ctx)
{
	static const char* const s_tests[] =
	{
		"imageAtomicAdd(u_image, ivec2(1, 1), 1u);",
		"imageAtomicMin(u_image, ivec2(1, 1), 1u);",
		"imageAtomicMax(u_image, ivec2(1, 1), 1u);",
		"imageAtomicAnd(u_image, ivec2(1, 1), 1u);",
		"imageAtomicOr(u_image, ivec2(1, 1), 1u);",
		"imageAtomicXor(u_image, ivec2(1, 1), 1u);",
		"imageAtomicExchange(u_image, ivec2(1, 1), 1u);",
		"imageAtomicCompSwap(u_image, ivec2(1, 1), 1u, 1u);",
	};

	ctx.beginSection("GL_OES_shader_image_atomic features require enabling the extension in 310 es shaders.");
	for (int ndx = 0; ndx < DE_LENGTH_OF_ARRAY(s_tests); ++ndx)
	{
		std::ostringstream source;
		source <<	"#version 310 es\n"
					"layout(binding=0, r32ui) coherent uniform highp uimage2D u_image;\n"
					"precision mediump float;\n"
					"void main()\n"
					"{\n"
					"	" << s_tests[ndx] << "\n"
					"}\n";
		verifyShader(ctx, glu::SHADERTYPE_FRAGMENT, source.str(), EXPECT_RESULT_FAIL);
	}
	ctx.endSection();
}

void shader_multisample_interpolation (NegativeTestContext& ctx)
{
	static const char* const s_sampleTests[] =
	{
		"sample in highp float v_var;",
		"sample out highp float v_var;"
	};

	static const char* const s_interpolateAtTests[] =
	{
		"interpolateAtCentroid(interpolant);",
		"interpolateAtSample(interpolant, 1);",
		"interpolateAtOffset(interpolant, vec2(1.0, 0.0));"
	};

	ctx.beginSection("GL_OES_shader_multisample_interpolation features require enabling the extension in 310 es shaders.");
	ctx.beginSection("Test sample in/out qualifiers.");
	for (int ndx = 0; ndx < DE_LENGTH_OF_ARRAY(s_sampleTests); ++ndx)
	{
		std::ostringstream source;
		source <<	"#version 310 es\n"
					"	" << s_sampleTests[ndx] << "\n"
					"precision mediump float;\n"
					"void main()\n"
					"{\n"
					"}\n";
		verifyShader(ctx, glu::SHADERTYPE_FRAGMENT, source.str(), EXPECT_RESULT_FAIL);
	}
	ctx.endSection();

	ctx.beginSection("Test interpolateAt* functions.");
	for (int ndx = 0; ndx < DE_LENGTH_OF_ARRAY(s_sampleTests); ++ndx)
	{
		std::ostringstream source;
		source <<	"#version 310 es\n"
					"in mediump float interpolant;\n"
					"precision mediump float;\n"
					"void main()\n"
					"{\n"
					"	" << s_interpolateAtTests[ndx] << "\n"
					"}\n";
		verifyShader(ctx, glu::SHADERTYPE_FRAGMENT, source.str(), EXPECT_RESULT_FAIL);
	}
	ctx.endSection();
	ctx.endSection();
}

void texture_storage_multisample_2d_array (NegativeTestContext& ctx)
{
	static const char* const s_samplerTypeTests[] =
	{
		"uniform mediump sampler2DMSArray u_sampler;",
		"uniform mediump isampler2DMSArray u_sampler;",
		"uniform mediump usampler2DMSArray u_sampler;",
	};

	ctx.beginSection("GL_OES_texture_storage_multisample_2d_array features require enabling the extension in 310 es shaders.");
	for (int ndx = 0; ndx < DE_LENGTH_OF_ARRAY(s_samplerTypeTests); ++ndx)
	{
		std::ostringstream source;
		source <<	"#version 310 es\n"
					"	" << s_samplerTypeTests[ndx] << "\n"
					"precision mediump float;\n"
					"void main()\n"
					"{\n"
					"}\n";
		verifyShader(ctx, glu::SHADERTYPE_FRAGMENT, source.str(), EXPECT_RESULT_FAIL);
	}
	ctx.endSection();
}

void geometry_shader (NegativeTestContext& ctx)
{
	if (ctx.isShaderSupported(glu::SHADERTYPE_GEOMETRY))
	{
		const std::string	simpleVtxFrag	=	"#version 310 es\n"
												"void main()\n"
												"{\n"
												"}\n";
		const std::string	geometry		=	"#version 310 es\n"
												"layout(points, invocations = 1) in;\n"
												"layout(points, max_vertices = 3) out;\n"
												"precision mediump float;\n"
												"void main()\n"
												"{\n"
												"	EmitVertex();\n"
												"	EndPrimitive();\n"
												"}\n";
		ctx.beginSection("GL_EXT_geometry_shader features require enabling the extension in 310 es shaders.");
		verifyProgram(ctx, glu::ProgramSources() << glu::VertexSource(simpleVtxFrag) << glu::GeometrySource(geometry) << glu::FragmentSource(simpleVtxFrag), EXPECT_RESULT_FAIL);
		ctx.endSection();
	}
}

void gpu_shader_5 (NegativeTestContext& ctx)
{
	ctx.beginSection("GL_EXT_gpu_shader5 features require enabling the extension in 310 es shaders.");
	ctx.beginSection("Testing the precise qualifier.");
	{
		std::ostringstream source;
		source <<	"#version 310 es\n"
					"void main()\n"
					"{\n"
					"	int low = 0;\n"
					"	int high = 10;\n"
					"	precise int middle = low + ((high - low) / 2);\n"
					"}\n";
		verifyShader(ctx, glu::SHADERTYPE_FRAGMENT, source.str(), EXPECT_RESULT_FAIL);
	}
	ctx.endSection();

	ctx.beginSection("Testing fused multiply-add.");
	{
		std::ostringstream source;
		source <<	"#version 310 es\n"
					"in mediump float v_var;"
					"void main()\n"
					"{\n"
					"	float fmaResult = fma(v_var, v_var, v_var);"
					"}\n";
		verifyShader(ctx, glu::SHADERTYPE_FRAGMENT, source.str(), EXPECT_RESULT_FAIL);
	}
	ctx.endSection();

	ctx.beginSection("Testing textureGatherOffsets.");
	{
		std::ostringstream source;
		source <<	"#version 310 es\n"
					"uniform mediump sampler2D u_tex;\n"
					"void main()\n"
					"{\n"
					"	highp vec2 coords = vec2(0.0, 1.0);\n"
					"	const ivec2 offsets[4] = ivec2[](ivec2(0,0), ivec2(1, 0), ivec2(0, 1), ivec2(1, 1));\n"
					"	textureGatherOffsets(u_tex, coords, offsets);\n"
					"}\n";
		verifyShader(ctx, glu::SHADERTYPE_FRAGMENT, source.str(), EXPECT_RESULT_FAIL);
	}
	ctx.endSection();

	ctx.endSection();
}

void shader_io_blocks (NegativeTestContext& ctx)
{
	ctx.beginSection("GL_EXT_shader_io_blocks features require enabling the extension in 310 es shaders.");
	{
		std::ostringstream source;
		source <<	"#version 310 es\n"
					"in Data\n"
					"{\n"
					"	mediump vec3 a;\n"
					"} data;\n"
					"void main()\n"
					"{\n"
					"}\n";
		verifyShader(ctx, glu::SHADERTYPE_FRAGMENT, source.str(), EXPECT_RESULT_FAIL);
	}
	ctx.endSection();
}

void tessellation_shader (NegativeTestContext& ctx)
{
	if (ctx.isShaderSupported(glu::SHADERTYPE_TESSELLATION_CONTROL))
	{
		const std::string	simpleVtxFrag	=	"#version 310 es\n"
												"void main()\n"
												"{\n"
												"}\n";
		const std::string	tessControl		=	"#version 310 es\n"
												"layout(vertices = 3) out;\n"
												"void main()\n"
												"{\n"
												"}\n";
		const std::string	tessEvaluation	=	"#version 310 es\n"
												"layout(triangles, equal_spacing, cw) in;\n"
												"void main()\n"
												"{\n"
												"}\n";
		ctx.beginSection("GL_EXT_tessellation_shader features require enabling the extension in 310 es shaders.");
		glu::ProgramSources sources;
		sources << glu::VertexSource(simpleVtxFrag)
				<< glu::TessellationControlSource(tessControl)
				<< glu::TessellationEvaluationSource(tessEvaluation)
				<< glu::FragmentSource(simpleVtxFrag);
		verifyProgram(ctx, sources, EXPECT_RESULT_FAIL);
		ctx.endSection();
	}
}

void texture_buffer (NegativeTestContext& ctx)
{
	static const char* const s_samplerBufferTypes[] =
	{
		"uniform mediump samplerBuffer",
	    "uniform mediump isamplerBuffer",
	    "uniform mediump usamplerBuffer",
	    "layout(rgba32f) uniform mediump writeonly imageBuffer",
	    "layout(rgba32i) uniform mediump writeonly iimageBuffer",
	    "layout(rgba32ui) uniform mediump writeonly uimageBuffer"
	};

	ctx.beginSection("GL_EXT_texture_buffer features require enabling the extension in 310 es shaders.");
	for (int ndx = 0; ndx < DE_LENGTH_OF_ARRAY(s_samplerBufferTypes); ++ndx)
	{
		std::ostringstream source;
		source <<	"#version 310 es\n"
					"" << s_samplerBufferTypes[ndx] << " u_buffer;\n"
					"void main()\n"
					"{\n"
					"}\n";
		verifyShader(ctx, glu::SHADERTYPE_FRAGMENT, source.str(), EXPECT_RESULT_FAIL);
	}
	ctx.endSection();
}


void texture_cube_map_array (NegativeTestContext& ctx)
{
	static const char* const s_samplerCubeArrayTypes[] =
	{
	    "uniform mediump samplerCubeArray",
	    "uniform mediump isamplerCubeArray",
	    "uniform mediump usamplerCubeArray",
	    "uniform mediump samplerCubeArrayShadow",
	    "layout(rgba32f) uniform mediump writeonly imageCubeArray",
	    "layout(rgba32i) uniform mediump writeonly iimageCubeArray",
	    "layout(rgba32ui) uniform mediump writeonly uimageCubeArray"
	};

	ctx.beginSection("GL_EXT_texture_cube_map_array features require enabling the extension in 310 es shaders.");
	for (int ndx = 0; ndx < DE_LENGTH_OF_ARRAY(s_samplerCubeArrayTypes); ++ndx)
	{
		std::ostringstream source;
		source <<	"#version 310 es\n"
					"" << s_samplerCubeArrayTypes[ndx] << " u_cube;\n"
					"void main()\n"
					"{\n"
					"}\n";
		verifyShader(ctx, glu::SHADERTYPE_FRAGMENT, source.str(), EXPECT_RESULT_FAIL);
	}
	ctx.endSection();
}

void executeAccessingBoundingBoxType (NegativeTestContext& ctx, const std::string builtInTypeName, glu::GLSLVersion glslVersion)
{
	std::ostringstream	sourceStream;
	std::string			version;
	std::string			extensionPrim;
	std::string			extensionTess;

	if (glslVersion == glu::GLSL_VERSION_310_ES)
	{
		version = "#version 310 es\n";
		extensionPrim = "#extension GL_EXT_primitive_bounding_box : require\n";
		extensionTess = "#extension GL_EXT_tessellation_shader : require\n";
	}
	else if (glslVersion >= glu::GLSL_VERSION_320_ES)
	{
		version = "#version 320 es\n";
		extensionPrim = "";
		extensionTess = "";
	}
	else
	{
		DE_FATAL("error: context below 3.1 and not supported");
	}

	ctx.beginSection("cannot access built-in type " + builtInTypeName + "[]" + " in vertex shader");
	sourceStream	<< version
					<< extensionPrim
					<< "void main()\n"
					<< "{\n"
					<< "	" + builtInTypeName + "[0] = vec4(1.0, 1.0, 1.0, 1.0);\n"
					<< "	gl_Position = " + builtInTypeName + "[0];\n"
					<< "}\n";
	verifyShader(ctx, glu::SHADERTYPE_VERTEX, sourceStream.str(), EXPECT_RESULT_FAIL);
	ctx.endSection();

	sourceStream.str(std::string());

	if (ctx.isShaderSupported(glu::SHADERTYPE_TESSELLATION_EVALUATION))
	{
		ctx.beginSection("cannot access built-in type " + builtInTypeName + "[]" + " in tessellation evaluation shader");
		sourceStream	<< version
						<< extensionPrim
						<< extensionTess
						<< "layout (triangles, equal_spacing, ccw) in;\n"
						<< "void main()\n"
						<< "{\n"
						<< "	" + builtInTypeName + "[0] = vec4(1.0, 1.0, 1.0, 1.0);\n"
						<< "	gl_Position = (	gl_TessCoord.x * " +  builtInTypeName + "[0] +\n"
						<< "					gl_TessCoord.y * " +  builtInTypeName + "[0] +\n"
						<< "					gl_TessCoord.z * " +  builtInTypeName + "[0]);\n"
						<< "}\n";
		verifyShader(ctx, glu::SHADERTYPE_TESSELLATION_EVALUATION, sourceStream.str(), EXPECT_RESULT_FAIL);
		ctx.endSection();

		sourceStream.str(std::string());
	}

	if (ctx.isShaderSupported(glu::SHADERTYPE_GEOMETRY))
	{
		ctx.beginSection("cannot access built-in type " + builtInTypeName + "[]" + " in geometry shader");
		sourceStream	<< version
						<< extensionPrim
						<< "layout (triangles) in;\n"
						<< "layout (triangle_strip, max_vertices = 3) out;\n"
						<< "void main()\n"
						<< "{\n"
						<< "	" + builtInTypeName + "[0] = vec4(1.0, 1.0, 1.0, 1.0);\n"
						<< "	for (int idx = 0; idx < 3; idx++)\n"
						<< "	{\n"
						<< "		gl_Position = gl_in[idx].gl_Position * " + builtInTypeName + "[0];\n"
						<< "		EmitVertex();\n"
						<< "	}\n"
						<< "	EndPrimitive();\n"
						<< "}\n";
		verifyShader(ctx, glu::SHADERTYPE_GEOMETRY, sourceStream.str(), EXPECT_RESULT_FAIL);
		ctx.endSection();

		sourceStream.str(std::string());
	}

	ctx.beginSection("cannot access built-in type " + builtInTypeName + "[]" + " in fragment shader");
	sourceStream	<< version
					<< extensionPrim
					<< "layout (location = 0) out mediump vec4 fs_colour;\n"
					<< "void main()\n"
					<< "{\n"
					<< "	" + builtInTypeName + "[0] = vec4(1.0, 1.0, 1.0, 1.0);\n"
					<< "	fs_colour = " + builtInTypeName + "[0];\n"
					<< "}\n";
	verifyShader(ctx, glu::SHADERTYPE_FRAGMENT, sourceStream.str(), EXPECT_RESULT_FAIL);
	ctx.endSection();
}

void accessing_bounding_box_type (NegativeTestContext& ctx)
{
	// Extension requirements and name differences depending on the context
	if ((ctx.getRenderContext().getType().getMajorVersion() == 3) && (ctx.getRenderContext().getType().getMinorVersion() == 1))
	{
		executeAccessingBoundingBoxType(ctx, "gl_BoundingBoxEXT", glu::GLSL_VERSION_310_ES);
	}
	else
	{
		executeAccessingBoundingBoxType(ctx, "gl_BoundingBox", glu::GLSL_VERSION_320_ES);
	}

}

} // anonymous

std::vector<FunctionContainer> getNegativeShaderDirectiveTestFunctions (void)
{
	const FunctionContainer funcs[] =
	{
		{primitive_bounding_box,				"primitive_bounding_box",				"GL_EXT_primitive_bounding_box required in 310 es shaders to use AEP features. Version 320 es shaders do not require suffixes."	},
		{blend_equation_advanced,				"blend_equation_advanced",				"GL_KHR_blend_equation_advanced is required in 310 es shaders to use AEP features"												},
		{sample_variables,						"sample_variables",						"GL_OES_sample_variables is required in 310 es shaders to use AEP features"														},
		{shader_image_atomic,					"shader_image_atomic",					"GL_OES_shader_image_atomic is required in 310 es shaders to use AEP features"													},
		{shader_multisample_interpolation,		"shader_multisample_interpolation",		"GL_OES_shader_multisample_interpolation is required in 310 es shaders to use AEP features"										},
		{texture_storage_multisample_2d_array,	"texture_storage_multisample_2d_array",	"GL_OES_texture_storage_multisample_2d_array is required in 310 es shaders to use AEP features"									},
		{geometry_shader,						"geometry_shader",						"GL_EXT_geometry_shader is required in 310 es shaders to use AEP features"														},
		{gpu_shader_5,							"gpu_shader_5",							"GL_EXT_gpu_shader5 is required in 310 es shaders to use AEP features"															},
		{shader_io_blocks,						"shader_io_blocks",						"GL_EXT_shader_io_blocks is required in 310 es shaders to use AEP features"														},
		{tessellation_shader,					"tessellation_shader",					"GL_EXT_tessellation_shader is required in 310 es shaders to use AEP features"													},
		{texture_buffer,						"texture_buffer",						"GL_EXT_texture_buffer is required in 310 es shaders to use AEP features"														},
		{texture_cube_map_array,				"texture_cube_map_array",				"GL_EXT_texture_cube_map_array is required in 310 es shaders to use AEP features"												},
		{accessing_bounding_box_type,			"accessing_bounding_box_type",			"Should not be able to access gl_BoundingBoxEXT[] and gl_BoundingBox[] in shaders other than tess control and evaluation"		},
	};

	return std::vector<FunctionContainer>(DE_ARRAY_BEGIN(funcs), DE_ARRAY_END(funcs));
}

} // NegativeTestShared
} // Functional
} // gles31
} // deqp
