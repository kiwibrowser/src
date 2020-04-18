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
 * \brief Negative Atomic Counter Tests
 *//*--------------------------------------------------------------------*/

#include "es31fNegativeAtomicCounterTests.hpp"

#include "deUniquePtr.hpp"

#include "glwEnums.hpp"
#include "gluShaderProgram.hpp"

#include "tcuTestLog.hpp"

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

enum TestCase
{
	TESTCASE_LAYOUT_LARGE_BINDING = 0,
	TESTCASE_LAYOUT_MEDIUMP_PRECISION,
	TESTCASE_LAYOUT_LOWP_PRECISION,
	TESTCASE_LAYOUT_BINDING_OFFSET_OVERLAP,
	TESTCASE_LAYOUT_BINDING_OMITTED,
	TESTCASE_STRUCT,
	TESTCASE_BODY_WRITE,
	TESTCASE_BODY_DECLARE,

	TESTCASE_LAST
};

static const glu::ShaderType s_shaders[] =
{
	glu::SHADERTYPE_VERTEX,
	glu::SHADERTYPE_FRAGMENT,
	glu::SHADERTYPE_GEOMETRY,
	glu::SHADERTYPE_TESSELLATION_CONTROL,
	glu::SHADERTYPE_TESSELLATION_EVALUATION,
	glu::SHADERTYPE_COMPUTE
};

std::string genShaderSource (NegativeTestContext& ctx, TestCase test, glu::ShaderType type)
{
	DE_ASSERT(test < TESTCASE_LAST && type < glu::SHADERTYPE_LAST);

	glw::GLint maxBuffers = -1;
	std::ostringstream shader;

	ctx.glGetIntegerv(GL_MAX_ATOMIC_COUNTER_BUFFER_BINDINGS, &maxBuffers);

	shader << getGLSLVersionDeclaration(glu::GLSL_VERSION_310_ES) << "\n";

	switch (type)
	{
		case glu::SHADERTYPE_GEOMETRY:
			shader << "#extension GL_EXT_geometry_shader : enable\n";
			shader << "layout(max_vertices = 3) out;\n";
			break;

		case glu::SHADERTYPE_TESSELLATION_CONTROL:
		case glu::SHADERTYPE_TESSELLATION_EVALUATION:
			shader << "#extension GL_EXT_tessellation_shader : enable\n";
			break;

		default:
			break;
	}

	switch (test)
	{
		case TESTCASE_LAYOUT_LARGE_BINDING:
			shader << "layout (binding = " << maxBuffers << ", offset = 0) uniform atomic_uint counter0;\n";
			break;

		case TESTCASE_LAYOUT_MEDIUMP_PRECISION:
			shader << "layout (binding = 1, offset = 0) " << glu::getPrecisionName(glu::PRECISION_MEDIUMP) << " uniform atomic_uint counter0;\n";
			break;

		case TESTCASE_LAYOUT_LOWP_PRECISION:
			shader << "layout (binding = 1, offset = 0) " << glu::getPrecisionName(glu::PRECISION_LOWP) << " uniform atomic_uint counter0;\n";
			break;

		case TESTCASE_LAYOUT_BINDING_OFFSET_OVERLAP:
			shader << "layout (binding = 1, offset = 0) uniform atomic_uint counter0;\n"
				   << "layout (binding = 1, offset = 2) uniform atomic_uint counter1;\n";
			break;

		case TESTCASE_LAYOUT_BINDING_OMITTED:
			shader << "layout (offset = 0) uniform atomic_uint counter0;\n";
			break;

		case TESTCASE_STRUCT:
			shader << "struct\n"
				   << "{\n"
				   << "  int a;\n"
				   << "  atomic_uint counter;\n"
				   << "} S;\n";
			break;

		case TESTCASE_BODY_WRITE:
			shader << "layout (binding = 1) uniform atomic_uint counter;\n";
			break;

		default:
			break;
	}

	shader << "void main (void)\n"
				 << "{\n";

	switch (test)
	{
		case TESTCASE_BODY_WRITE:
			shader << "counter = 1;\n";
			break;

		case TESTCASE_BODY_DECLARE:
			shader << "atomic_uint counter;\n";
			break;

		default:
			break;
	}

	shader << "}\n";

	return shader.str();
}

void iterateShaders (NegativeTestContext& ctx, TestCase testCase)
{
	tcu::TestLog& log = ctx.getLog();
	for (int ndx = 0; ndx < DE_LENGTH_OF_ARRAY(s_shaders); ndx++)
	{
		if (ctx.isShaderSupported(s_shaders[ndx]))
		{
			ctx.beginSection(std::string("Verify shader: ") + glu::getShaderTypeName(s_shaders[ndx]));
			const glu::ShaderProgram program(ctx.getRenderContext(), glu::ProgramSources() << glu::ShaderSource(s_shaders[ndx], genShaderSource(ctx, testCase, s_shaders[ndx])));
			if (program.getShaderInfo(s_shaders[ndx]).compileOk)
			{
				log << program;
				log << tcu::TestLog::Message << "Expected program to fail, but compilation passed." << tcu::TestLog::EndMessage;
				ctx.fail("Shader was not expected to compile.");
			}
			ctx.endSection();
		}
	}
}

void atomic_max_counter_bindings (NegativeTestContext& ctx)
{
	ctx.beginSection("It is a compile-time error to bind an atomic counter with a binding value greater than or equal to gl_MaxAtomicCounterBindings.");
	iterateShaders(ctx, TESTCASE_LAYOUT_LARGE_BINDING);
	ctx.endSection();
}

void atomic_precision (NegativeTestContext& ctx)
{
	ctx.beginSection("It is an error to declare an atomic type with a lowp or mediump precision.");
	iterateShaders(ctx, TESTCASE_LAYOUT_MEDIUMP_PRECISION);
	iterateShaders(ctx, TESTCASE_LAYOUT_LOWP_PRECISION);
	ctx.endSection();
}

void atomic_binding_offset_overlap (NegativeTestContext& ctx)
{
	ctx.beginSection("Atomic counters may not have overlapping offsets in the same binding.");
	iterateShaders(ctx, TESTCASE_LAYOUT_BINDING_OFFSET_OVERLAP);
	ctx.endSection();
}

void atomic_binding_omitted (NegativeTestContext& ctx)
{
	ctx.beginSection("Atomic counters must specify a binding point");
	iterateShaders(ctx, TESTCASE_LAYOUT_BINDING_OMITTED);
	ctx.endSection();
}

void atomic_struct (NegativeTestContext& ctx)
{
	ctx.beginSection("Structures may not have an atomic_uint variable.");
	iterateShaders(ctx, TESTCASE_STRUCT);
	ctx.endSection();
}

void atomic_body_write (NegativeTestContext& ctx)
{
	ctx.beginSection("An atomic_uint variable cannot be directly written to.");
	iterateShaders(ctx, TESTCASE_BODY_WRITE);
	ctx.endSection();
}

void atomic_body_declare (NegativeTestContext& ctx)
{
	ctx.beginSection("An atomic_uint variable cannot be declared in local scope");
	iterateShaders(ctx, TESTCASE_BODY_DECLARE);
	ctx.endSection();
}

} // anonymous

std::vector<FunctionContainer> getNegativeAtomicCounterTestFunctions ()
{
	const FunctionContainer funcs[] =
	{
		{atomic_max_counter_bindings,		"atomic_max_counter_bindings",		"Invalid atomic counter buffer binding."	},
		{atomic_precision,					"atomic_precision",					"Invalid precision qualifier."				},
		{atomic_binding_offset_overlap,		"atomic_binding_offset_overlap",	"Invalid offset."							},
		{atomic_binding_omitted,			"atomic_binding_omitted",			"Binding not specified."					},
		{atomic_struct,						"atomic_struct",					"Invalid atomic_uint usage in struct."		},
		{atomic_body_write,					"atomic_body_write",				"Invalid write access to atomic_uint."		},
		{atomic_body_declare,				"atomic_body_declare",				"Invalid precision qualifier."				},
	};

	return std::vector<FunctionContainer>(DE_ARRAY_BEGIN(funcs), DE_ARRAY_END(funcs));
}

} // NegativeTestShared
} // Functional
} // gles31
} // deqp
