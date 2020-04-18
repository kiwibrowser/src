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
 * \brief Program State Query tests.
 *//*--------------------------------------------------------------------*/

#include "es31fProgramStateQueryTests.hpp"
#include "es31fInfoLogQueryShared.hpp"
#include "glsStateQueryUtil.hpp"
#include "gluRenderContext.hpp"
#include "gluCallLogWrapper.hpp"
#include "gluContextInfo.hpp"
#include "gluObjectWrapper.hpp"
#include "gluShaderProgram.hpp"
#include "glwFunctions.hpp"
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
namespace
{

using namespace gls::StateQueryUtil;

static const char* getVerifierSuffix (QueryType type)
{
	switch (type)
	{
		case QUERY_PROGRAM_INTEGER_VEC3:
		case QUERY_PROGRAM_INTEGER:
			return "get_programiv";

		default:
			DE_ASSERT(DE_FALSE);
			return DE_NULL;
	}
}

static std::string specializeShader(Context& context, const char* code)
{
	const glu::GLSLVersion				glslVersion			= glu::getContextTypeGLSLVersion(context.getRenderContext().getType());
	std::map<std::string, std::string>	specializationMap;

	specializationMap["GLSL_VERSION_DECL"] = glu::getGLSLVersionDeclaration(glslVersion);

	if (glu::contextSupports(context.getRenderContext().getType(), glu::ApiType::es(3, 2)))
	{
		specializationMap["GEOMETRY_SHADER_REQUIRE"] = "";
		specializationMap["TESSELLATION_SHADER_REQUIRE"] = "";
	}
	else
	{
		specializationMap["GEOMETRY_SHADER_REQUIRE"] = "#extension GL_EXT_geometry_shader : require";
		specializationMap["TESSELLATION_SHADER_REQUIRE"] = "#extension GL_EXT_tessellation_shader : require";
	}

	return tcu::StringTemplate(code).specialize(specializationMap);
}


class GeometryShaderCase : public TestCase
{
public:
						GeometryShaderCase		(Context& context, QueryType verifier, const char* name, const char* desc);
	IterateResult		iterate					(void);

private:
	const QueryType		m_verifier;
};

GeometryShaderCase::GeometryShaderCase (Context& context, QueryType verifier, const char* name, const char* desc)
	: TestCase		(context, name, desc)
	, m_verifier	(verifier)
{
}

GeometryShaderCase::IterateResult GeometryShaderCase::iterate (void)
{
	const bool supportsES32 = glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::es(3, 2));

	if (!supportsES32 && !m_context.getContextInfo().isExtensionSupported("GL_EXT_geometry_shader"))
		TCU_THROW(NotSupportedError, "Geometry shader tests require GL_EXT_geometry_shader extension or an OpenGL ES 3.2 or higher context.");


	static const char* const	s_vtxFragTemplate	=	"${GLSL_VERSION_DECL}\n"
														"void main()\n"
														"{\n"
														"}\n";

	static const char* const	s_geometryTemplate1	=	"${GLSL_VERSION_DECL}\n"
														"${GEOMETRY_SHADER_REQUIRE}\n"
														"layout(triangles) in;"
														"layout(triangle_strip, max_vertices = 3) out;\n"
														"void main()\n"
														"{\n"
														"   EndPrimitive();\n"
														"}\n";

	static const char* const	s_geometryTemplate2	=	"${GLSL_VERSION_DECL}\n"
														"${GEOMETRY_SHADER_REQUIRE}\n"
														"layout(points) in;"
														"layout(line_strip, max_vertices = 5) out;\n"
														"void main()\n"
														"{\n"
														"   EndPrimitive();\n"
														"}\n";

	static const char* const	s_geometryTemplate3	=	"${GLSL_VERSION_DECL}\n"
														"${GEOMETRY_SHADER_REQUIRE}\n"
														"layout(points) in;"
														"layout(points, max_vertices = 50) out;\n"
														"void main()\n"
														"{\n"
														"   EndPrimitive();\n"
														"}\n";

	glu::CallLogWrapper			gl						(m_context.getRenderContext().getFunctions(), m_testCtx.getLog());
	tcu::ResultCollector		result					(m_testCtx.getLog(), " // ERROR: ");

	gl.enableLogging(true);

	{
		const tcu::ScopedLogSection	section		(m_testCtx.getLog(), "Layout", "triangles in, triangle strip out, 3 vertices");
		glu::ShaderProgram			program		(m_context.getRenderContext(), glu::ProgramSources()
			<< glu::VertexSource(specializeShader(m_context, s_vtxFragTemplate))
			<< glu::FragmentSource(specializeShader(m_context, s_vtxFragTemplate))
			<< glu::GeometrySource(specializeShader(m_context, s_geometryTemplate1)));

		TCU_CHECK_MSG(program.isOk(), "Compile failed");

		m_testCtx.getLog() << program;

		verifyStateProgramInteger(result, gl, program.getProgram(), GL_GEOMETRY_VERTICES_OUT, 3, m_verifier);
		verifyStateProgramInteger(result, gl, program.getProgram(), GL_GEOMETRY_INPUT_TYPE, GL_TRIANGLES, m_verifier);
		verifyStateProgramInteger(result, gl, program.getProgram(), GL_GEOMETRY_OUTPUT_TYPE, GL_TRIANGLE_STRIP, m_verifier);
		verifyStateProgramInteger(result, gl, program.getProgram(), GL_GEOMETRY_SHADER_INVOCATIONS, 1, m_verifier);
	}

	{
		const tcu::ScopedLogSection	section		(m_testCtx.getLog(), "Layout", "points in, line strip out, 5 vertices");
		glu::ShaderProgram			program		(m_context.getRenderContext(), glu::ProgramSources()
			<< glu::VertexSource(specializeShader(m_context, s_vtxFragTemplate))
			<< glu::FragmentSource(specializeShader(m_context, s_vtxFragTemplate))
			<< glu::GeometrySource(specializeShader(m_context, s_geometryTemplate2)));

		TCU_CHECK_MSG(program.isOk(), "Compile failed");

		m_testCtx.getLog() << program;

		verifyStateProgramInteger(result, gl, program.getProgram(), GL_GEOMETRY_VERTICES_OUT, 5, m_verifier);
		verifyStateProgramInteger(result, gl, program.getProgram(), GL_GEOMETRY_INPUT_TYPE, GL_POINTS, m_verifier);
		verifyStateProgramInteger(result, gl, program.getProgram(), GL_GEOMETRY_OUTPUT_TYPE, GL_LINE_STRIP, m_verifier);
	}

	{
		const tcu::ScopedLogSection	section		(m_testCtx.getLog(), "Layout", "points in, points out, 50 vertices");
		glu::ShaderProgram			program		(m_context.getRenderContext(), glu::ProgramSources()
			<< glu::VertexSource(specializeShader(m_context, s_vtxFragTemplate))
			<< glu::FragmentSource(specializeShader(m_context, s_vtxFragTemplate))
			<< glu::GeometrySource(specializeShader(m_context, s_geometryTemplate3)));

		TCU_CHECK_MSG(program.isOk(), "Compile failed");

		m_testCtx.getLog() << program;

		verifyStateProgramInteger(result, gl, program.getProgram(), GL_GEOMETRY_VERTICES_OUT, 50, m_verifier);
		verifyStateProgramInteger(result, gl, program.getProgram(), GL_GEOMETRY_INPUT_TYPE, GL_POINTS, m_verifier);
		verifyStateProgramInteger(result, gl, program.getProgram(), GL_GEOMETRY_OUTPUT_TYPE, GL_POINTS, m_verifier);
	}

	result.setTestContextResult(m_testCtx);
	return STOP;
}

class TessellationShaderCase : public TestCase
{
public:
						TessellationShaderCase		(Context& context, QueryType verifier, const char* name, const char* desc);
	IterateResult		iterate					(void);

private:
	const QueryType		m_verifier;
};

TessellationShaderCase::TessellationShaderCase (Context& context, QueryType verifier, const char* name, const char* desc)
	: TestCase		(context, name, desc)
	, m_verifier	(verifier)
{
}

TessellationShaderCase::IterateResult TessellationShaderCase::iterate (void)
{
	const bool supportsES32 = glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::es(3, 2));

	if (!supportsES32 && !m_context.getContextInfo().isExtensionSupported("GL_EXT_tessellation_shader"))
		TCU_THROW(NotSupportedError, "Tessellation shader tests require GL_EXT_tessellation_shader extension or an OpenGL ES 3.2 or higher context.");


	static const char* const	s_vtxFragTemplate	=	"${GLSL_VERSION_DECL}\n"
														"void main()\n"
														"{\n"
														"}\n";

	static const char* const	s_tessCtrlTemplate1	=	"${GLSL_VERSION_DECL}\n"
														"${TESSELLATION_SHADER_REQUIRE}\n"
														"layout(vertices = 3) out;\n"
														"void main()\n"
														"{\n"
														"}\n";

	static const char* const	s_tessEvalTemplate1	=	"${GLSL_VERSION_DECL}\n"
														"${TESSELLATION_SHADER_REQUIRE}\n"
														"layout(triangles, equal_spacing, cw) in;\n"
														"void main()\n"
														"{\n"
														"}\n";

	static const char* const	s_tessCtrlTemplate2	=	"${GLSL_VERSION_DECL}\n"
														"${TESSELLATION_SHADER_REQUIRE}\n"
														"layout(vertices = 5) out;\n"
														"void main()\n"
														"{\n"
														"}\n";

	static const char* const	s_tessEvalTemplate2	=	"${GLSL_VERSION_DECL}\n"
														"${TESSELLATION_SHADER_REQUIRE}\n"
														"layout(quads, fractional_even_spacing, ccw) in;\n"
														"void main()\n"
														"{\n"
														"}\n";

	static const char* const	s_tessEvalTemplate3	=	"${GLSL_VERSION_DECL}\n"
														"${TESSELLATION_SHADER_REQUIRE}\n"
														"layout(isolines, fractional_odd_spacing, ccw, point_mode) in;\n"
														"void main()\n"
														"{\n"
														"}\n";

	glu::CallLogWrapper			gl						(m_context.getRenderContext().getFunctions(), m_testCtx.getLog());
	tcu::ResultCollector		result					(m_testCtx.getLog(), " // ERROR: ");

	gl.enableLogging(true);

	{
		const tcu::ScopedLogSection	section		(m_testCtx.getLog(), "Query State", "3 vertices, triangles, equal_spacing, cw");
		glu::ShaderProgram			program		(m_context.getRenderContext(), glu::ProgramSources()
			<< glu::VertexSource(specializeShader(m_context, s_vtxFragTemplate))
			<< glu::FragmentSource(specializeShader(m_context, s_vtxFragTemplate))
			<< glu::TessellationControlSource(specializeShader(m_context, s_tessCtrlTemplate1))
			<< glu::TessellationEvaluationSource(specializeShader(m_context, s_tessEvalTemplate1)));

		TCU_CHECK_MSG(program.isOk(), "Compile failed");

		m_testCtx.getLog() << program;

		verifyStateProgramInteger(result, gl, program.getProgram(), GL_TESS_CONTROL_OUTPUT_VERTICES, 3, m_verifier);
		verifyStateProgramInteger(result, gl, program.getProgram(), GL_TESS_GEN_MODE, GL_TRIANGLES, m_verifier);
		verifyStateProgramInteger(result, gl, program.getProgram(), GL_TESS_GEN_SPACING, GL_EQUAL, m_verifier);
		verifyStateProgramInteger(result, gl, program.getProgram(), GL_TESS_GEN_VERTEX_ORDER, GL_CW, m_verifier);
		verifyStateProgramInteger(result, gl, program.getProgram(), GL_TESS_GEN_POINT_MODE, GL_FALSE, m_verifier);
	}

	{
		const tcu::ScopedLogSection	section		(m_testCtx.getLog(), "Query State", "5 vertices, quads, fractional_even_spacing, ccw");
		glu::ShaderProgram			program		(m_context.getRenderContext(), glu::ProgramSources()
			<< glu::VertexSource(specializeShader(m_context, s_vtxFragTemplate))
			<< glu::FragmentSource(specializeShader(m_context, s_vtxFragTemplate))
			<< glu::TessellationControlSource(specializeShader(m_context, s_tessCtrlTemplate2))
			<< glu::TessellationEvaluationSource(specializeShader(m_context, s_tessEvalTemplate2)));

		TCU_CHECK_MSG(program.isOk(), "Compile failed");

		m_testCtx.getLog() << program;

		verifyStateProgramInteger(result, gl, program.getProgram(), GL_TESS_CONTROL_OUTPUT_VERTICES, 5, m_verifier);
		verifyStateProgramInteger(result, gl, program.getProgram(), GL_TESS_GEN_MODE, GL_QUADS, m_verifier);
		verifyStateProgramInteger(result, gl, program.getProgram(), GL_TESS_GEN_SPACING, GL_FRACTIONAL_EVEN, m_verifier);
		verifyStateProgramInteger(result, gl, program.getProgram(), GL_TESS_GEN_VERTEX_ORDER, GL_CCW, m_verifier);
		verifyStateProgramInteger(result, gl, program.getProgram(), GL_TESS_GEN_POINT_MODE, GL_FALSE, m_verifier);
	}

	{
		const tcu::ScopedLogSection	section		(m_testCtx.getLog(), "Query State", "5 vertices, isolines, fractional_odd_spacing, ccw, point_mode");
		glu::ShaderProgram			program		(m_context.getRenderContext(), glu::ProgramSources()
			<< glu::VertexSource(specializeShader(m_context, s_vtxFragTemplate))
			<< glu::FragmentSource(specializeShader(m_context, s_vtxFragTemplate))
			<< glu::TessellationControlSource(specializeShader(m_context, s_tessCtrlTemplate2))
			<< glu::TessellationEvaluationSource(specializeShader(m_context, s_tessEvalTemplate3)));

		TCU_CHECK_MSG(program.isOk(), "Compile failed");

		m_testCtx.getLog() << program;

		verifyStateProgramInteger(result, gl, program.getProgram(), GL_TESS_CONTROL_OUTPUT_VERTICES, 5, m_verifier);
		verifyStateProgramInteger(result, gl, program.getProgram(), GL_TESS_GEN_MODE, GL_ISOLINES, m_verifier);
		verifyStateProgramInteger(result, gl, program.getProgram(), GL_TESS_GEN_SPACING, GL_FRACTIONAL_ODD, m_verifier);
		verifyStateProgramInteger(result, gl, program.getProgram(), GL_TESS_GEN_VERTEX_ORDER, GL_CCW, m_verifier);
		verifyStateProgramInteger(result, gl, program.getProgram(), GL_TESS_GEN_POINT_MODE, GL_TRUE, m_verifier);
	}

	result.setTestContextResult(m_testCtx);
	return STOP;
}

class ProgramSeparableCase : public TestCase
{
public:
						ProgramSeparableCase	(Context& context, QueryType verifier, const char* name, const char* desc);
	IterateResult		iterate					(void);

private:
	const QueryType		m_verifier;
};

ProgramSeparableCase::ProgramSeparableCase (Context& context, QueryType verifier, const char* name, const char* desc)
	: TestCase		(context, name, desc)
	, m_verifier	(verifier)
{
}

ProgramSeparableCase::IterateResult ProgramSeparableCase::iterate (void)
{
	const string				vtxTemplate	=	"${GLSL_VERSION_DECL}\n"
												"out highp vec4 v_color;\n"
												"void main()\n"
												"{\n"
												"	gl_Position = vec4(float(gl_VertexID) * 0.5, float(gl_VertexID+1) * 0.5, 0.0, 1.0);\n"
												"	v_color = vec4(float(gl_VertexID), 1.0, 0.0, 1.0);\n"
												"}\n";
	const string				fragTemplate =	"${GLSL_VERSION_DECL}\n"
												"in highp vec4 v_color;\n"
												"layout(location=0) out highp vec4 o_color;\n"
												"void main()\n"
												"{\n"
												"	o_color = v_color;\n"
												"}\n";

	glu::CallLogWrapper			gl				(m_context.getRenderContext().getFunctions(), m_testCtx.getLog());
	tcu::ResultCollector		result			(m_testCtx.getLog(), " // ERROR: ");
	glu::Shader					vtxShader		(m_context.getRenderContext(), glu::SHADERTYPE_VERTEX);
	glu::Shader					frgShader		(m_context.getRenderContext(), glu::SHADERTYPE_FRAGMENT);

	const std::string			vStr			= specializeShader(m_context, vtxTemplate.c_str());
	const std::string			fStr			= specializeShader(m_context, fragTemplate.c_str());
	const char* const			vtxSourcePtr	= vStr.c_str();
	const char* const			fragSourcePtr	= fStr.c_str();

	vtxShader.setSources(1, &vtxSourcePtr, DE_NULL);
	frgShader.setSources(1, &fragSourcePtr, DE_NULL);

	vtxShader.compile();
	frgShader.compile();

	{
		const tcu::ScopedLogSection section(m_testCtx.getLog(), "VtxShader", "Vertex shader");
		m_testCtx.getLog() << vtxShader;
	}

	{
		const tcu::ScopedLogSection section(m_testCtx.getLog(), "FrgShader", "Fragment shader");
		m_testCtx.getLog() << frgShader;
	}

	TCU_CHECK_MSG(vtxShader.getCompileStatus() && frgShader.getCompileStatus(), "failed to build shaders");

	gl.enableLogging(true);

	{
		const tcu::ScopedLogSection	section	(m_testCtx.getLog(), "Initial", "Initial");
		glu::Program				program	(m_context.getRenderContext());

		verifyStateProgramInteger(result, gl, program.getProgram(), GL_PROGRAM_SEPARABLE, 0, m_verifier);
	}

	{
		const tcu::ScopedLogSection section		(m_testCtx.getLog(), "SetFalse", "SetFalse");
		glu::Program				program		(m_context.getRenderContext());
		int							linkStatus	= 0;

		gl.glAttachShader(program.getProgram(), vtxShader.getShader());
		gl.glAttachShader(program.getProgram(), frgShader.getShader());
		gl.glProgramParameteri(program.getProgram(), GL_PROGRAM_SEPARABLE, GL_FALSE);
		gl.glLinkProgram(program.getProgram());
		GLU_EXPECT_NO_ERROR(gl.glGetError(), "setup program");

		gl.glGetProgramiv(program.getProgram(), GL_LINK_STATUS, &linkStatus);
		GLU_EXPECT_NO_ERROR(gl.glGetError(), "query link status");

		TCU_CHECK_MSG(linkStatus == GL_TRUE, "failed to link program");

		verifyStateProgramInteger(result, gl, program.getProgram(), GL_PROGRAM_SEPARABLE, 0, m_verifier);
	}

	{
		const tcu::ScopedLogSection section		(m_testCtx.getLog(), "SetTrue", "SetTrue");
		glu::Program				program		(m_context.getRenderContext());
		int							linkStatus	= 0;

		gl.glAttachShader(program.getProgram(), vtxShader.getShader());
		gl.glAttachShader(program.getProgram(), frgShader.getShader());
		gl.glProgramParameteri(program.getProgram(), GL_PROGRAM_SEPARABLE, GL_TRUE);
		gl.glLinkProgram(program.getProgram());
		GLU_EXPECT_NO_ERROR(gl.glGetError(), "setup program");

		gl.glGetProgramiv(program.getProgram(), GL_LINK_STATUS, &linkStatus);
		GLU_EXPECT_NO_ERROR(gl.glGetError(), "query link status");

		TCU_CHECK_MSG(linkStatus == GL_TRUE, "failed to link program");

		verifyStateProgramInteger(result, gl, program.getProgram(), GL_PROGRAM_SEPARABLE, GL_TRUE, m_verifier);
	}

	result.setTestContextResult(m_testCtx);
	return STOP;
}

class ComputeWorkGroupSizeCase : public TestCase
{
public:
						ComputeWorkGroupSizeCase	(Context& context, QueryType verifier, const char* name, const char* desc);
	IterateResult		iterate						(void);

private:
	const QueryType		m_verifier;
};

ComputeWorkGroupSizeCase::ComputeWorkGroupSizeCase (Context& context, QueryType verifier, const char* name, const char* desc)
	: TestCase		(context, name, desc)
	, m_verifier	(verifier)
{
}

ComputeWorkGroupSizeCase::IterateResult ComputeWorkGroupSizeCase::iterate (void)
{
	static const char* const	s_computeTemplate1D =	"${GLSL_VERSION_DECL}\n"
														"layout (local_size_x = 3) in;\n"
														"layout(binding = 0) buffer Output\n"
														"{\n"
														"	highp float val;\n"
														"} sb_out;\n"
														"\n"
														"void main (void)\n"
														"{\n"
														"	sb_out.val = 1.0;\n"
														"}\n";
	static const char* const	s_computeTemplate2D =	"${GLSL_VERSION_DECL}\n"
														"layout (local_size_x = 3, local_size_y = 2) in;\n"
														"layout(binding = 0) buffer Output\n"
														"{\n"
														"	highp float val;\n"
														"} sb_out;\n"
														"\n"
														"void main (void)\n"
														"{\n"
														"	sb_out.val = 1.0;\n"
														"}\n";
	static const char* const	s_computeTemplate3D =	"${GLSL_VERSION_DECL}\n"
														"layout (local_size_x = 3, local_size_y = 2, local_size_z = 4) in;\n"
														"layout(binding = 0) buffer Output\n"
														"{\n"
														"	highp float val;\n"
														"} sb_out;\n"
														"\n"
														"void main (void)\n"
														"{\n"
														"	sb_out.val = 1.0;\n"
														"}\n";

	glu::CallLogWrapper			gl						(m_context.getRenderContext().getFunctions(), m_testCtx.getLog());
	tcu::ResultCollector		result					(m_testCtx.getLog(), " // ERROR: ");

	gl.enableLogging(true);

	{
		const tcu::ScopedLogSection section		(m_testCtx.getLog(), "OneDimensional", "1D");
		glu::ShaderProgram			program		(m_context.getRenderContext(), glu::ProgramSources() << glu::ComputeSource(specializeShader(m_context, s_computeTemplate1D)));

		m_testCtx.getLog() << program;

		TCU_CHECK_MSG(program.isOk(), "failed to build program");

		verifyStateProgramIntegerVec3(result, gl, program.getProgram(), GL_COMPUTE_WORK_GROUP_SIZE, tcu::IVec3(3, 1, 1), m_verifier);
	}

	{
		const tcu::ScopedLogSection section		(m_testCtx.getLog(), "TwoDimensional", "2D");
		glu::ShaderProgram			program		(m_context.getRenderContext(), glu::ProgramSources() << glu::ComputeSource(specializeShader(m_context, s_computeTemplate2D)));

		m_testCtx.getLog() << program;

		TCU_CHECK_MSG(program.isOk(), "failed to build program");

		verifyStateProgramIntegerVec3(result, gl, program.getProgram(), GL_COMPUTE_WORK_GROUP_SIZE, tcu::IVec3(3, 2, 1), m_verifier);
	}

	{
		const tcu::ScopedLogSection section		(m_testCtx.getLog(), "TreeDimensional", "3D");
		glu::ShaderProgram			program		(m_context.getRenderContext(), glu::ProgramSources() << glu::ComputeSource(specializeShader(m_context, s_computeTemplate3D)));

		m_testCtx.getLog() << program;

		TCU_CHECK_MSG(program.isOk(), "failed to build program");

		verifyStateProgramIntegerVec3(result, gl, program.getProgram(), GL_COMPUTE_WORK_GROUP_SIZE, tcu::IVec3(3, 2, 4), m_verifier);
	}

	result.setTestContextResult(m_testCtx);
	return STOP;
}

class ActiveAtomicCounterBuffersCase : public TestCase
{
public:
						ActiveAtomicCounterBuffersCase	(Context& context, QueryType verifier, const char* name, const char* desc);
	IterateResult		iterate							(void);

private:
	const QueryType		m_verifier;
};

ActiveAtomicCounterBuffersCase::ActiveAtomicCounterBuffersCase (Context& context, QueryType verifier, const char* name, const char* desc)
	: TestCase		(context, name, desc)
	, m_verifier	(verifier)
{
}

ActiveAtomicCounterBuffersCase::IterateResult ActiveAtomicCounterBuffersCase::iterate (void)
{
	static const char* const	s_computeTemplate0	=	"${GLSL_VERSION_DECL}\n"
														"layout (local_size_x = 3) in;\n"
														"layout(binding = 0) buffer Output\n"
														"{\n"
														"	highp float val;\n"
														"} sb_out;\n"
														"\n"
														"void main (void)\n"
														"{\n"
														"	sb_out.val = 1.0;\n"
														"}\n";
	static const char* const	s_computeTemplate1	=	"${GLSL_VERSION_DECL}\n"
														"layout (local_size_x = 3) in;\n"
														"layout(binding = 0) uniform highp atomic_uint u_counters[2];\n"
														"layout(binding = 0) buffer Output\n"
														"{\n"
														"	highp float val;\n"
														"} sb_out;\n"
														"\n"
														"void main (void)\n"
														"{\n"
														"	sb_out.val = float(atomicCounterIncrement(u_counters[0])) + float(atomicCounterIncrement(u_counters[1]));\n"
														"}\n";

	glu::CallLogWrapper			gl						(m_context.getRenderContext().getFunctions(), m_testCtx.getLog());
	tcu::ResultCollector		result					(m_testCtx.getLog(), " // ERROR: ");

	gl.enableLogging(true);

	{
		const tcu::ScopedLogSection	section		(m_testCtx.getLog(), "Initial", "Initial");
		glu::Program				program		(m_context.getRenderContext());

		verifyStateProgramInteger(result, gl, program.getProgram(), GL_ACTIVE_ATOMIC_COUNTER_BUFFERS, 0, m_verifier);
	}

	{
		const tcu::ScopedLogSection	section		(m_testCtx.getLog(), "NoBuffers", "No buffers");
		glu::ShaderProgram			program		(m_context.getRenderContext(), glu::ProgramSources() << glu::ComputeSource(specializeShader(m_context, s_computeTemplate0)));

		m_testCtx.getLog() << program;

		TCU_CHECK_MSG(program.isOk(), "failed to build program");

		verifyStateProgramInteger(result, gl, program.getProgram(), GL_ACTIVE_ATOMIC_COUNTER_BUFFERS, 0, m_verifier);
	}

	{
		const tcu::ScopedLogSection	section		(m_testCtx.getLog(), "OneBuffer", "One buffer");
		glu::ShaderProgram			program		(m_context.getRenderContext(), glu::ProgramSources() << glu::ComputeSource(specializeShader(m_context, s_computeTemplate1)));

		m_testCtx.getLog() << program;

		TCU_CHECK_MSG(program.isOk(), "failed to build program");

		verifyStateProgramInteger(result, gl, program.getProgram(), GL_ACTIVE_ATOMIC_COUNTER_BUFFERS, 1, m_verifier);
	}

	result.setTestContextResult(m_testCtx);
	return STOP;
}

class ProgramLogCase : public TestCase
{
public:
	enum BuildErrorType
	{
		BUILDERROR_VERTEX_FRAGMENT = 0,
		BUILDERROR_COMPUTE,
		BUILDERROR_GEOMETRY,
		BUILDERROR_TESSELLATION,
	};

							ProgramLogCase		(Context& ctx, const char* name, const char* desc, BuildErrorType errorType);

private:
	void					init				(void);
	IterateResult			iterate				(void);
	glu::ProgramSources		getProgramSources	(void) const;

	const BuildErrorType	m_buildErrorType;
};

ProgramLogCase::ProgramLogCase (Context& ctx, const char* name, const char* desc, BuildErrorType errorType)
	: TestCase			(ctx, name, desc)
	, m_buildErrorType	(errorType)
{
}

void ProgramLogCase::init (void)
{
	const bool supportsES32 = glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::es(3, 2));

	switch (m_buildErrorType)
	{
		case BUILDERROR_VERTEX_FRAGMENT:
		case BUILDERROR_COMPUTE:
			break;

		case BUILDERROR_GEOMETRY:
			if (!supportsES32 && !m_context.getContextInfo().isExtensionSupported("GL_EXT_geometry_shader"))
				TCU_THROW(NotSupportedError, "Test requires GL_EXT_geometry_shader extension");
			break;

		case BUILDERROR_TESSELLATION:
			if (!supportsES32 && !m_context.getContextInfo().isExtensionSupported("GL_EXT_tessellation_shader"))
				TCU_THROW(NotSupportedError, "Test requires GL_EXT_tessellation_shader extension");
			break;

		default:
			DE_ASSERT(false);
			break;
	}
}

ProgramLogCase::IterateResult ProgramLogCase::iterate (void)
{
	using gls::StateQueryUtil::StateQueryMemoryWriteGuard;

	tcu::ResultCollector					result		(m_testCtx.getLog());
	glu::CallLogWrapper						gl			(m_context.getRenderContext().getFunctions(), m_testCtx.getLog());
	glu::ShaderProgram						program		(m_context.getRenderContext(), getProgramSources());
	StateQueryMemoryWriteGuard<glw::GLint>	logLen;

	gl.enableLogging(true);

	m_testCtx.getLog() << tcu::TestLog::Message << "Trying to link a broken program." << tcu::TestLog::EndMessage;

	gl.glGetProgramiv(program.getProgram(), GL_INFO_LOG_LENGTH, &logLen);
	logLen.verifyValidity(result);

	if (logLen.verifyValidity(result))
		verifyInfoLogQuery(result, gl, logLen, program.getProgram(), &glu::CallLogWrapper::glGetProgramInfoLog, "glGetProgramInfoLog");

	result.setTestContextResult(m_testCtx);
	return STOP;
}

glu::ProgramSources ProgramLogCase::getProgramSources (void) const
{
	const char* const	vertexTemplate1 =	"${GLSL_VERSION_DECL}\n"
											"in highp vec4 a_pos;\n"
											"uniform highp vec4 u_uniform;\n"
											"void main()\n"
											"{\n"
											"	gl_Position = a_pos + u_uniform;\n"
											"}\n";
	const char* const	vertexTemplate2 =	"${GLSL_VERSION_DECL}\n"
											"in highp vec4 a_pos;\n"
											"void main()\n"
											"{\n"
											"	gl_Position = a_pos;\n"
											"}\n";
	const char* const	fragmentTemplate1 =	"${GLSL_VERSION_DECL}\n"
											"in highp vec4 v_missingVar;\n"
											"uniform highp int u_uniform;\n"
											"layout(location = 0) out mediump vec4 fragColor;\n"
											"void main()\n"
											"{\n"
											"	fragColor = v_missingVar + vec4(float(u_uniform));\n"
											"}\n";

	const char* const	fragmentTemplate2 =	"${GLSL_VERSION_DECL}\n"
											"layout(location = 0) out mediump vec4 fragColor;\n"
											"void main()\n"
											"{\n"
											"	fragColor = vec4(1.0);\n"
											"}\n";
	const char* const	computeTemplate1 =	"${GLSL_VERSION_DECL}\n"
											"layout (binding = 0) buffer IOBuffer { highp float buf_var; };\n"
											"uniform highp vec4 u_uniform;\n"
											"void main()\n"
											"{\n"
											"	buf_var = u_uniform.x;\n"
											"}\n";
	const char* const	geometryTemplate1 =	"${GLSL_VERSION_DECL}\n"
											"${GEOMETRY_SHADER_REQUIRE}\n"
											"layout(triangles) in;\n"
											"layout(max_vertices=1, points) out;\n"
											"in highp vec4 v_missingVar[];\n"
											"uniform highp int u_uniform;\n"
											"void main()\n"
											"{\n"
											"	gl_Position = gl_in[0].gl_Position + v_missingVar[2] + vec4(float(u_uniform));\n"
											"	EmitVertex();\n"
											"}\n";
	const char* const	tessCtrlTemplate1 =	"${GLSL_VERSION_DECL}\n"
											"${TESSELLATION_SHADER_REQUIRE}\n"
											"layout(vertices=2) out;"
											"patch out highp vec2 vp_var;\n"
											"void main()\n"
											"{\n"
											"	gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position\n"
											"	gl_TessLevelOuter[0] = 0.8;\n"
											"	gl_TessLevelOuter[1] = 0.8;\n"
											"	if (gl_InvocationID == 0)\n"
											"		vp_var = gl_in[gl_InvocationID].gl_Position.xy;\n"
											"}\n";
	const char* const	tessEvalTemplate1 =	"${GLSL_VERSION_DECL}\n"
											"${TESSELLATION_SHADER_REQUIRE}\n"
											"layout(isolines) in;"
											"in highp float vp_var[];\n"
											"void main()\n"
											"{\n"
											"	gl_Position = gl_in[gl_InvocationID].gl_Position + vec4(vp_var[1]);\n"
											"}\n";

	switch (m_buildErrorType)
	{
		case BUILDERROR_VERTEX_FRAGMENT:
			return glu::ProgramSources()
					<< glu::VertexSource(specializeShader(m_context, vertexTemplate1))
					<< glu::FragmentSource(specializeShader(m_context, fragmentTemplate1));

		case BUILDERROR_COMPUTE:
			return glu::ProgramSources()
					<< glu::ComputeSource(specializeShader(m_context, computeTemplate1));

		case BUILDERROR_GEOMETRY:
			return glu::ProgramSources()
					<< glu::VertexSource(specializeShader(m_context, vertexTemplate1))
					<< glu::GeometrySource(specializeShader(m_context, geometryTemplate1))
					<< glu::FragmentSource(specializeShader(m_context, fragmentTemplate2));

		case BUILDERROR_TESSELLATION:
			return glu::ProgramSources()
					<< glu::VertexSource(specializeShader(m_context, vertexTemplate2))
					<< glu::TessellationControlSource(specializeShader(m_context, tessCtrlTemplate1))
					<< glu::TessellationEvaluationSource(specializeShader(m_context, tessEvalTemplate1))
					<< glu::FragmentSource(specializeShader(m_context, fragmentTemplate2));

		default:
			DE_ASSERT(false);
			return glu::ProgramSources();
	}
}

} // anonymous

ProgramStateQueryTests::ProgramStateQueryTests (Context& context)
	: TestCaseGroup(context, "program", "Program State Query tests")
{
}

ProgramStateQueryTests::~ProgramStateQueryTests (void)
{
}

void ProgramStateQueryTests::init (void)
{
	static const QueryType intVerifiers[] =
	{
		QUERY_PROGRAM_INTEGER,
	};
	static const QueryType intVec3Verifiers[] =
	{
		QUERY_PROGRAM_INTEGER_VEC3,
	};

#define FOR_EACH_INT_VERIFIER(X) \
	for (int verifierNdx = 0; verifierNdx < DE_LENGTH_OF_ARRAY(intVerifiers); ++verifierNdx)	\
	{																							\
		const char* verifierSuffix = getVerifierSuffix(intVerifiers[verifierNdx]);				\
		const QueryType verifier = intVerifiers[verifierNdx];									\
		this->addChild(X);																		\
	}

#define FOR_EACH_VEC_VERIFIER(X) \
	for (int verifierNdx = 0; verifierNdx < DE_LENGTH_OF_ARRAY(intVec3Verifiers); ++verifierNdx)	\
	{																								\
		const char* verifierSuffix = getVerifierSuffix(intVec3Verifiers[verifierNdx]);				\
		const QueryType verifier = intVec3Verifiers[verifierNdx];									\
		this->addChild(X);																			\
	}

	FOR_EACH_INT_VERIFIER(new ProgramSeparableCase				(m_context, verifier, (std::string("program_separable_") + verifierSuffix).c_str(),				"Test PROGRAM_SEPARABLE"));
	FOR_EACH_VEC_VERIFIER(new ComputeWorkGroupSizeCase			(m_context, verifier, (std::string("compute_work_group_size_") + verifierSuffix).c_str(),		"Test COMPUTE_WORK_GROUP_SIZE"));
	FOR_EACH_INT_VERIFIER(new ActiveAtomicCounterBuffersCase	(m_context, verifier, (std::string("active_atomic_counter_buffers_") + verifierSuffix).c_str(),	"Test ACTIVE_ATOMIC_COUNTER_BUFFERS"));
	FOR_EACH_INT_VERIFIER(new GeometryShaderCase				(m_context, verifier, (std::string("geometry_shader_state_") + verifierSuffix).c_str(),			"Test Geometry Shader State"));
	FOR_EACH_INT_VERIFIER(new TessellationShaderCase			(m_context, verifier, (std::string("tesselation_shader_state_") + verifierSuffix).c_str(),		"Test Tesselation Shader State"));

#undef FOR_EACH_INT_VERIFIER
#undef FOR_EACH_VEC_VERIFIER

	// program info log tests
	// \note, there exists similar tests in gles3 module. However, the gles31 could use a different
	//        shader compiler with different INFO_LOG bugs.
	{
		static const struct
		{
			const char*						caseName;
			ProgramLogCase::BuildErrorType	caseType;
		} shaderTypes[] =
		{
			{ "info_log_vertex_fragment_link_fail",		ProgramLogCase::BUILDERROR_VERTEX_FRAGMENT	},
			{ "info_log_compute_link_fail",				ProgramLogCase::BUILDERROR_COMPUTE			},
			{ "info_log_geometry_link_fail",			ProgramLogCase::BUILDERROR_GEOMETRY			},
			{ "info_log_tessellation_link_fail",		ProgramLogCase::BUILDERROR_TESSELLATION		},
		};

		for (int ndx = 0; ndx < DE_LENGTH_OF_ARRAY(shaderTypes); ++ndx)
			addChild(new ProgramLogCase(m_context, shaderTypes[ndx].caseName, "", shaderTypes[ndx].caseType));
	}
}

} // Functional
} // gles31
} // deqp
