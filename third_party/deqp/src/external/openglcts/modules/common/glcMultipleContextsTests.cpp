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
 * \file  glcMultipleContextsTests.cpp
 * \brief
 */ /*-------------------------------------------------------------------*/

#include "glcMultipleContextsTests.hpp"
#include "deSharedPtr.hpp"
#include "gl4cShaderSubroutineTests.hpp"
#include "gluContextInfo.hpp"
#include "glwEnums.hpp"
#include "glwFunctions.hpp"
#include "tcuMatrix.hpp"
#include <cmath>
#include <cstring>
#include <deMath.h>

using namespace glw;
using namespace gl4cts::ShaderSubroutine;

namespace glcts
{

/**
 * * Create multiple contexts and verify that subroutine uniforms values
 *   are preserved for each program stage when switching rendering context.
 *
 * OpenGL 4.1 or ARB_separate_shader_objects support required
 * * Same as above, but use pipelines instead of monolithic program.
 **/
class UniformPreservationTest : public tcu::TestCase
{
public:
	/* Public methods */
	UniformPreservationTest(tcu::TestContext& testCtx, glu::ApiType apiType);

	virtual void						 deinit();
	virtual tcu::TestNode::IterateResult iterate();

private:
	/* Private types */
	struct subroutineUniformSet
	{
		bool operator!=(const subroutineUniformSet& arg) const;
		void set(glw::GLuint bit_field, const subroutineUniformSet subroutine_indices[2]);

		glw::GLuint m_vertex_shader_stage;
		glw::GLuint m_tesselation_control_shader_stage;
		glw::GLuint m_tesselation_evaluation_shader_stage;
		glw::GLuint m_geometry_shader_stage;
		glw::GLuint m_fragment_shader_stage;
	};

	/* Private methods */
	void captureCurrentSubroutineSet(subroutineUniformSet& set);

	void getShaders(const glw::GLchar*& out_vertex_shader_code, const glw::GLchar*& out_tesselation_control_shader_code,
					const glw::GLchar*& out_tesselation_evaluation_shader_code,
					const glw::GLchar*& out_geometry_shader_code, const glw::GLchar*& out_fragment_shader_code);

	void initSharedContexts();

	void prepareProgram(Utils::program** programs, bool is_separable);

	void prepareProgramPipeline(glw::GLuint& pipeline_id, Utils::program** programs);

	bool testCase(const glw::GLuint bit_field[5]);

	bool testProgram(Utils::program** programs, bool is_separable, const glw::GLuint test_cases[][5],
					 glw::GLuint n_test_cases);

	void updateCurrentSubroutineSet(const subroutineUniformSet& set);

	/* Private fields */
	static const glw::GLuint m_n_shared_contexts;
	static const glw::GLuint m_fragment_stage_index;
	static const glw::GLuint m_geometry_stage_index;
	static const glw::GLuint m_tesselation_control_stage_index;
	static const glw::GLuint m_tesselation_evaluation_stage_index;
	static const glw::GLuint m_vertex_stage_index;

	glu::ApiType				 m_api_type;
	de::SharedPtr<deqp::Context> m_base_context;
	glu::RenderContext*			 m_shared_contexts[4];
	glw::GLuint					 m_program_pipelines[5];
	subroutineUniformSet		 m_subroutine_indices[2];
	subroutineUniformSet		 m_subroutine_uniform_locations;
};

/* Constants used by FunctionalTest20_21 */
const GLuint UniformPreservationTest::m_n_shared_contexts				   = 4;
const GLuint UniformPreservationTest::m_fragment_stage_index			   = 0;
const GLuint UniformPreservationTest::m_geometry_stage_index			   = 1;
const GLuint UniformPreservationTest::m_tesselation_control_stage_index	= 2;
const GLuint UniformPreservationTest::m_tesselation_evaluation_stage_index = 3;
const GLuint UniformPreservationTest::m_vertex_stage_index				   = 4;

/** Set subroutine indices, indices are taken from one of two sets according to provided <bit_field>
 *
 * @param bit_field          Selects source of of index for each stage
 * @param subroutine_indices Array of two indices sets
 **/
void UniformPreservationTest::subroutineUniformSet::set(GLuint					   bit_field,
														const subroutineUniformSet subroutine_indices[2])
{
	GLuint vertex_stage					= ((bit_field & (0x01 << 0)) >> 0);
	GLuint tesselation_control_stage	= ((bit_field & (0x01 << 1)) >> 1);
	GLuint tesselation_evaluation_stage = ((bit_field & (0x01 << 2)) >> 2);
	GLuint geometry_stage				= ((bit_field & (0x01 << 3)) >> 3);
	GLuint fragment_stage				= ((bit_field & (0x01 << 4)) >> 4);

	m_vertex_shader_stage = subroutine_indices[vertex_stage].m_vertex_shader_stage;
	m_tesselation_control_shader_stage =
		subroutine_indices[tesselation_control_stage].m_tesselation_control_shader_stage;
	m_tesselation_evaluation_shader_stage =
		subroutine_indices[tesselation_evaluation_stage].m_tesselation_evaluation_shader_stage;
	m_geometry_shader_stage = subroutine_indices[geometry_stage].m_geometry_shader_stage;
	m_fragment_shader_stage = subroutine_indices[fragment_stage].m_fragment_shader_stage;
}

/** Negated comparison of two sets
 *
 * @param arg Instance that will be compared to this
 *
 * @return false when both objects are equal, true otherwise
 **/
bool UniformPreservationTest::subroutineUniformSet::operator!=(const subroutineUniformSet& arg) const
{
	if ((arg.m_vertex_shader_stage != m_vertex_shader_stage) ||
		(arg.m_tesselation_control_shader_stage != m_tesselation_control_shader_stage) ||
		(arg.m_tesselation_evaluation_shader_stage != m_tesselation_evaluation_shader_stage) ||
		(arg.m_geometry_shader_stage != m_geometry_shader_stage) ||
		(arg.m_fragment_shader_stage != m_fragment_shader_stage))
	{
		return true;
	}

	return false;
}

/** Constructor.
 *
 *  @param context Rendering context.
 *
 **/
UniformPreservationTest::UniformPreservationTest(tcu::TestContext& testCtx, glu::ApiType apiType)
	: tcu::TestCase(testCtx, "uniform_preservation",
					"Verifies that shader uniforms are preserved when rendering context is switched.")
	, m_api_type(apiType)
{
	for (GLuint i = 0; i < m_n_shared_contexts + 1; ++i)
	{
		m_program_pipelines[i] = 0;
	}

	for (GLuint i = 0; i < m_n_shared_contexts; ++i)
	{
		m_shared_contexts[i] = 0;
	}
}

/** Deinitializes all GL objects that may have been created during
 *  test execution.
 **/
void UniformPreservationTest::deinit()
{
	/* GL entry points */
	const glw::Functions& gl = m_base_context->getRenderContext().getFunctions();

	for (GLuint i = 0; i < m_n_shared_contexts + 1; ++i)
	{
		if (0 != m_program_pipelines[i])
		{
			gl.deleteProgramPipelines(1, &m_program_pipelines[i]);
			m_program_pipelines[i] = 0;
		}
	}

	for (GLuint i = 0; i < m_n_shared_contexts; ++i)
	{
		if (0 != m_shared_contexts[i])
		{
			delete m_shared_contexts[i];
			m_shared_contexts[i] = 0;
		}
	}
}

/** Executes test iteration.
 *
 *  @return Returns STOP
 */
tcu::TestNode::IterateResult UniformPreservationTest::iterate()
{
	/* Test cases, values stored here are used as bit fields */
	static const GLuint test_cases[][m_n_shared_contexts + 1] = {
		{ 0, 1, 2, 3, 4 },		{ 1, 2, 3, 4, 0 },		{ 2, 3, 4, 0, 1 },		{ 3, 4, 0, 1, 2 },
		{ 4, 0, 1, 2, 3 },		{ 27, 28, 29, 30, 31 }, { 28, 29, 30, 31, 27 }, { 29, 30, 31, 27, 28 },
		{ 30, 31, 27, 28, 29 }, { 31, 27, 28, 29, 30 },
	};
	static const GLuint n_test_cases = sizeof(test_cases) / sizeof(test_cases[0]);

	glu::ContextType context_type(m_api_type);
	m_base_context = de::SharedPtr<deqp::Context>(new deqp::Context(m_testCtx, context_type));

	/* Do not execute the test if GL_ARB_shader_subroutine is not supported */
	if (!m_base_context->getContextInfo().isExtensionSupported("GL_ARB_shader_subroutine"))
	{
		throw tcu::NotSupportedError("GL_ARB_shader_subroutine is not supported.");
	}

	/* Prepare contexts */
	initSharedContexts();

	/* Test result */
	bool result = true;

	/* Program pointers */
	Utils::program* program_pointers[5];

	/* Test monolithic program */
	{
		/* Prepare program */
		Utils::program program(*m_base_context.get());

		program_pointers[m_fragment_stage_index] = &program;

		prepareProgram(program_pointers, false);

		/* Execute test */
		if (false == testProgram(program_pointers, false, test_cases, n_test_cases))
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "Last error message was caused by monolithic program."
							   << tcu::TestLog::EndMessage;

			result = false;
		}
	}

	/* Test separable programs */
	if (true == m_base_context->getContextInfo().isExtensionSupported("GL_ARB_separate_shader_objects"))
	{
		/* Prepare programs */
		Utils::program vertex_program(*m_base_context.get());
		Utils::program tesselation_control_program(*m_base_context.get());
		Utils::program tesselation_evaluation_program(*m_base_context.get());
		Utils::program geometry_program(*m_base_context.get());
		Utils::program fragment_program(*m_base_context.get());

		program_pointers[m_fragment_stage_index]			   = &fragment_program;
		program_pointers[m_geometry_stage_index]			   = &geometry_program;
		program_pointers[m_tesselation_control_stage_index]	= &tesselation_control_program;
		program_pointers[m_tesselation_evaluation_stage_index] = &tesselation_evaluation_program;
		program_pointers[m_vertex_stage_index]				   = &vertex_program;

		prepareProgram(program_pointers, true);

		/* Execute test */
		if (false == testProgram(program_pointers, true, test_cases, n_test_cases))
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "Last error message was caused by separable program."
							   << tcu::TestLog::EndMessage;
			result = false;
		}
	}

	/* All done */
	if (true == result)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	else
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
	}

	return tcu::TestNode::STOP;
}

/** Query state of subroutine uniforms of current program/pipeline
 *
 * @param set Storage for results
 **/
void UniformPreservationTest::captureCurrentSubroutineSet(subroutineUniformSet& set)
{
	/* GL entry points */
	const glw::Functions& gl = m_base_context->getRenderContext().getFunctions();

	/* Fragment */
	gl.getUniformSubroutineuiv(GL_FRAGMENT_SHADER, m_subroutine_uniform_locations.m_fragment_shader_stage,
							   &set.m_fragment_shader_stage);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GetUniformSubroutineuiv");

	/* Geometry */
	gl.getUniformSubroutineuiv(GL_GEOMETRY_SHADER, m_subroutine_uniform_locations.m_geometry_shader_stage,
							   &set.m_geometry_shader_stage);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GetUniformSubroutineuiv");

	/* Tess ctrl */
	gl.getUniformSubroutineuiv(GL_TESS_CONTROL_SHADER,
							   m_subroutine_uniform_locations.m_tesselation_control_shader_stage,
							   &set.m_tesselation_control_shader_stage);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GetUniformSubroutineuiv");

	/* Tess eval */
	gl.getUniformSubroutineuiv(GL_TESS_EVALUATION_SHADER,
							   m_subroutine_uniform_locations.m_tesselation_evaluation_shader_stage,
							   &set.m_tesselation_evaluation_shader_stage);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GetUniformSubroutineuiv");

	/* Vertex */
	gl.getUniformSubroutineuiv(GL_VERTEX_SHADER, m_subroutine_uniform_locations.m_vertex_shader_stage,
							   &set.m_vertex_shader_stage);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GetUniformSubroutineuiv");
}

/** Get shaders' source code
 *
 * @param out_vertex_shader_code                 Vertex source code
 * @param out_tesselation_control_shader_code    Tess ctrl source code
 * @param out_tesselation_evaluation_shader_code Tess eval source code
 * @param out_geometry_shader_code               Geometry source code
 * @param out_fragment_shader_code               Fragment source code
 **/
void UniformPreservationTest::getShaders(const glw::GLchar*& out_vertex_shader_code,
										 const glw::GLchar*& out_tesselation_control_shader_code,
										 const glw::GLchar*& out_tesselation_evaluation_shader_code,
										 const glw::GLchar*& out_geometry_shader_code,
										 const glw::GLchar*& out_fragment_shader_code)
{
	static const GLchar* vertex_shader_code = "#version 400 core\n"
											  "#extension GL_ARB_shader_subroutine : require\n"
											  "\n"
											  "precision highp float;\n"
											  "\n"
											  "// Subroutine type\n"
											  "subroutine vec4 routine_type(in vec4 left, in vec4 right);\n"
											  "\n"
											  "// Subroutine definition\n"
											  "subroutine(routine_type) vec4 add(in vec4 left, in vec4 right)\n"
											  "{\n"
											  "    return left + right;\n"
											  "}\n"
											  "\n"
											  "subroutine(routine_type) vec4 multiply(in vec4 left, in vec4 right)\n"
											  "{\n"
											  "    return left * right;\n"
											  "}\n"
											  "\n"
											  "// Sub routine uniform\n"
											  "subroutine uniform routine_type routine;\n"
											  "\n"
											  "// Input data\n"
											  "uniform vec4 uni_vs_left;\n"
											  "uniform vec4 uni_vs_right;\n"
											  "\n"
											  "// Output\n"
											  "out vec4 vs_tcs_result;\n"
											  "\n"
											  "void main()\n"
											  "{\n"
											  "    vs_tcs_result = routine(uni_vs_left, uni_vs_right);\n"
											  "}\n"
											  "\n";

	static const GLchar* tesselation_control_shader_code =
		"#version 400 core\n"
		"#extension GL_ARB_shader_subroutine : require\n"
		"\n"
		"precision highp float;\n"
		"\n"
		"layout(vertices = 1) out;\n"
		"\n"
		"// Subroutine type\n"
		"subroutine vec4 routine_type(in vec4 left, in vec4 right);\n"
		"\n"
		"// Subroutine definition\n"
		"subroutine(routine_type) vec4 add(in vec4 left, in vec4 right)\n"
		"{\n"
		"    return left + right;\n"
		"}\n"
		"\n"
		"subroutine(routine_type) vec4 multiply(in vec4 left, in vec4 right)\n"
		"{\n"
		"    return left * right;\n"
		"}\n"
		"\n"
		"// Sub routine uniform\n"
		"subroutine uniform routine_type routine;\n"
		"\n"
		"// Input data\n"
		"uniform vec4 uni_tcs_left;\n"
		"uniform vec4 uni_tcs_right;\n"
		"\n"
		"in vec4 vs_tcs_result[];\n"
		"\n"
		"// Output\n"
		"out vec4 tcs_tes_result[];\n"
		"\n"
		"void main()\n"
		"{\n"
		"    gl_TessLevelOuter[0] = 1.0;\n"
		"    gl_TessLevelOuter[1] = 1.0;\n"
		"    gl_TessLevelOuter[2] = 1.0;\n"
		"    gl_TessLevelOuter[3] = 1.0;\n"
		"    gl_TessLevelInner[0] = 1.0;\n"
		"    gl_TessLevelInner[1] = 1.0;\n"
		"\n"
		"    tcs_tes_result[gl_InvocationID] = routine(uni_tcs_left, uni_tcs_right) + vs_tcs_result[gl_InvocationID];\n"
		"}\n"
		"\n";

	static const GLchar* tesselation_evaluation_shader_code =
		"#version 400 core\n"
		"#extension GL_ARB_shader_subroutine : require\n"
		"\n"
		"precision highp float;\n"
		"\n"
		"layout(isolines, point_mode) in;\n"
		"\n"
		"// Subroutine type\n"
		"subroutine vec4 routine_type(in vec4 left, in vec4 right);\n"
		"\n"
		"// Subroutine definition\n"
		"subroutine(routine_type) vec4 add(in vec4 left, in vec4 right)\n"
		"{\n"
		"    return left + right;\n"
		"}\n"
		"\n"
		"subroutine(routine_type) vec4 multiply(in vec4 left, in vec4 right)\n"
		"{\n"
		"    return left * right;\n"
		"}\n"
		"\n"
		"// Sub routine uniform\n"
		"subroutine uniform routine_type routine;\n"
		"\n"
		"// Input data\n"
		"uniform vec4 uni_tes_left;\n"
		"uniform vec4 uni_tes_right;\n"
		"\n"
		"in vec4 tcs_tes_result[];\n"
		"\n"
		"// Output\n"
		"out vec4 tes_gs_result;\n"
		"\n"
		"void main()\n"
		"{\n"
		"    tes_gs_result = routine(uni_tes_left, uni_tes_right) + tcs_tes_result[0];\n"
		"}\n"
		"\n";

	static const GLchar* geometry_shader_code =
		"#version 400 core\n"
		"#extension GL_ARB_shader_subroutine : require\n"
		"\n"
		"precision highp float;\n"
		"\n"
		"layout(points)                   in;\n"
		"layout(points, max_vertices = 1) out;\n"
		"\n"
		"// Subroutine type\n"
		"subroutine vec4 routine_type(in vec4 left, in vec4 right);\n"
		"\n"
		"// Subroutine definition\n"
		"subroutine(routine_type) vec4 add(in vec4 left, in vec4 right)\n"
		"{\n"
		"    return left + right;\n"
		"}\n"
		"\n"
		"subroutine(routine_type) vec4 multiply(in vec4 left, in vec4 right)\n"
		"{\n"
		"    return left * right;\n"
		"}\n"
		"\n"
		"// Sub routine uniform\n"
		"subroutine uniform routine_type routine;\n"
		"\n"
		"// Input data\n"
		"uniform vec4 uni_gs_left;\n"
		"uniform vec4 uni_gs_right;\n"
		"\n"
		"in vec4 tes_gs_result[];\n"
		"\n"
		"// Output\n"
		"out vec4 gs_fs_result;\n"
		"\n"
		"void main()\n"
		"{\n"
		"    gs_fs_result = routine(uni_gs_left, uni_gs_right) + tes_gs_result[0];\n"
		"}\n"
		"\n";

	static const GLchar* fragmenty_shader_code =
		"#version 400 core\n"
		"#extension GL_ARB_shader_subroutine : require\n"
		"\n"
		"precision highp float;\n"
		"\n"
		"// Subroutine type\n"
		"subroutine vec4 routine_type(in vec4 left, in vec4 right);\n"
		"\n"
		"// Subroutine definition\n"
		"subroutine(routine_type) vec4 add(in vec4 left, in vec4 right)\n"
		"{\n"
		"    return left + right;\n"
		"}\n"
		"\n"
		"subroutine(routine_type) vec4 multiply(in vec4 left, in vec4 right)\n"
		"{\n"
		"    return left * right;\n"
		"}\n"
		"\n"
		"// Sub routine uniform\n"
		"subroutine uniform routine_type routine;\n"
		"\n"
		"// Input data\n"
		"uniform vec4 uni_fs_left;\n"
		"uniform vec4 uni_fs_right;\n"
		"\n"
		"in vec4 gs_fs_result;\n"
		"\n"
		"// Output\n"
		"out vec4 fs_out_result;\n"
		"\n"
		"void main()\n"
		"{\n"
		"    fs_out_result = routine(uni_fs_left, uni_fs_right) + gs_fs_result;\n"
		"}\n"
		"\n";

	out_vertex_shader_code				   = vertex_shader_code;
	out_tesselation_control_shader_code	= tesselation_control_shader_code;
	out_tesselation_evaluation_shader_code = tesselation_evaluation_shader_code;
	out_geometry_shader_code			   = geometry_shader_code;
	out_fragment_shader_code			   = fragmenty_shader_code;
}

/** Create <m_n_shared_contexts> shared contexts
 *
 **/
void UniformPreservationTest::initSharedContexts()
{
	glu::ContextType		context_type(m_api_type);
	glu::RenderConfig		render_config(context_type);
	const tcu::CommandLine& command_line(m_testCtx.getCommandLine());
	glu::RenderContext*		shared_context = &(m_base_context->getRenderContext());
	glu::parseRenderConfig(&render_config, command_line);

#if (DE_OS == DE_OS_ANDROID)
	// Android can only have one Window created at a time
	// Note that this surface type is not supported on all platforms
	render_config.surfaceType = glu::RenderConfig::SURFACETYPE_OFFSCREEN_GENERIC;
#endif

	for (GLuint i = 0; i < m_n_shared_contexts; ++i)
	{
		m_shared_contexts[i] =
			glu::createRenderContext(m_testCtx.getPlatform(), command_line, render_config, shared_context);
	}
	m_base_context->getRenderContext().makeCurrent();
}

/** Prepare program(s)
 *
 * @param programs     An array of 5 programs' pointers. If monolithic program is prepared that only index m_fragment_stage_index should be initialized, otherwise all 5
 * @param is_separable Select if monolithic or separable programs should be prepared
 **/
void UniformPreservationTest::prepareProgram(Utils::program** programs, bool is_separable)
{
	/* Get shader sources */
	const GLchar* vertex_shader_code;
	const GLchar* tesselation_control_shader_code;
	const GLchar* tesselation_evaluation_shader_code;
	const GLchar* geometry_shader_code;
	const GLchar* fragmenty_shader_code;

	getShaders(vertex_shader_code, tesselation_control_shader_code, tesselation_evaluation_shader_code,
			   geometry_shader_code, fragmenty_shader_code);

	/* Subroutines and uniform names */
	static const GLchar* subroutine_names[] = { "add", "multiply" };
	static const GLuint  n_subroutines		= sizeof(subroutine_names) / sizeof(subroutine_names[0]);

	static const GLchar* subroutine_uniform_name = "routine";

	/* Build program */
	if (false == is_separable)
	{
		programs[0]->build(0 /* compute shader source */, fragmenty_shader_code, geometry_shader_code,
						   tesselation_control_shader_code, tesselation_evaluation_shader_code, vertex_shader_code,
						   0 /* varying_names */, 0 /* n_varying_names */);

		programs[m_geometry_stage_index]			   = programs[m_fragment_stage_index];
		programs[m_tesselation_control_stage_index]	= programs[m_fragment_stage_index];
		programs[m_tesselation_evaluation_stage_index] = programs[m_fragment_stage_index];
		programs[m_vertex_stage_index]				   = programs[m_fragment_stage_index];
	}
	else
	{
		programs[m_fragment_stage_index]->build(0, fragmenty_shader_code, 0, 0, 0, 0, 0, 0, true);
		programs[m_geometry_stage_index]->build(0, 0, geometry_shader_code, 0, 0, 0, 0, 0, true);
		programs[m_tesselation_control_stage_index]->build(0, 0, 0, tesselation_control_shader_code, 0, 0, 0, 0, true);
		programs[m_tesselation_evaluation_stage_index]->build(0, 0, 0, 0, tesselation_evaluation_shader_code, 0, 0, 0,
															  true);
		programs[m_vertex_stage_index]->build(0, 0, 0, 0, 0, vertex_shader_code, 0, 0, true);
	}

	/* Get subroutine indices */
	for (GLuint i = 0; i < n_subroutines; ++i)
	{
		m_subroutine_indices[i].m_fragment_shader_stage =
			programs[m_fragment_stage_index]->getSubroutineIndex(subroutine_names[i], GL_FRAGMENT_SHADER);

		m_subroutine_indices[i].m_geometry_shader_stage =
			programs[m_geometry_stage_index]->getSubroutineIndex(subroutine_names[i], GL_GEOMETRY_SHADER);

		m_subroutine_indices[i].m_tesselation_control_shader_stage =
			programs[m_tesselation_control_stage_index]->getSubroutineIndex(subroutine_names[i],
																			GL_TESS_CONTROL_SHADER);

		m_subroutine_indices[i].m_tesselation_evaluation_shader_stage =
			programs[m_tesselation_evaluation_stage_index]->getSubroutineIndex(subroutine_names[i],
																			   GL_TESS_EVALUATION_SHADER);

		m_subroutine_indices[i].m_vertex_shader_stage =
			programs[m_vertex_stage_index]->getSubroutineIndex(subroutine_names[i], GL_VERTEX_SHADER);
	}

	/* Get subroutine uniform locations */
	m_subroutine_uniform_locations.m_fragment_shader_stage =
		programs[m_fragment_stage_index]->getSubroutineUniformLocation(subroutine_uniform_name, GL_FRAGMENT_SHADER);

	m_subroutine_uniform_locations.m_geometry_shader_stage =
		programs[m_geometry_stage_index]->getSubroutineUniformLocation(subroutine_uniform_name, GL_GEOMETRY_SHADER);

	m_subroutine_uniform_locations.m_tesselation_control_shader_stage =
		programs[m_tesselation_control_stage_index]->getSubroutineUniformLocation(subroutine_uniform_name,
																				  GL_TESS_CONTROL_SHADER);

	m_subroutine_uniform_locations.m_tesselation_evaluation_shader_stage =
		programs[m_tesselation_evaluation_stage_index]->getSubroutineUniformLocation(subroutine_uniform_name,
																					 GL_TESS_EVALUATION_SHADER);

	m_subroutine_uniform_locations.m_vertex_shader_stage =
		programs[m_vertex_stage_index]->getSubroutineUniformLocation(subroutine_uniform_name, GL_VERTEX_SHADER);
}

/** Generate program pipeline for current context and attach separable programs
 *
 * @param out_pipeline_id Id of generated pipeline
 * @param programs        Collection of separable programs
 **/
void UniformPreservationTest::prepareProgramPipeline(glw::GLuint& out_pipeline_id, Utils::program** programs)
{
	/* GL entry points */
	const glw::Functions& gl = m_base_context->getRenderContext().getFunctions();

	/* Generate */
	gl.genProgramPipelines(1, &out_pipeline_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GenProgramPipelines");

	/* Bind */
	gl.bindProgramPipeline(out_pipeline_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "BindProgramPipeline");

	/* Set up programs */
	gl.useProgramStages(out_pipeline_id, GL_FRAGMENT_SHADER_BIT, programs[m_fragment_stage_index]->m_program_object_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "UseProgramStages");

	gl.useProgramStages(out_pipeline_id, GL_GEOMETRY_SHADER_BIT, programs[m_geometry_stage_index]->m_program_object_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "UseProgramStages");

	gl.useProgramStages(out_pipeline_id, GL_TESS_CONTROL_SHADER_BIT,
						programs[m_tesselation_control_stage_index]->m_program_object_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "UseProgramStages");

	gl.useProgramStages(out_pipeline_id, GL_TESS_EVALUATION_SHADER_BIT,
						programs[m_tesselation_evaluation_stage_index]->m_program_object_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "UseProgramStages");

	gl.useProgramStages(out_pipeline_id, GL_VERTEX_SHADER_BIT, programs[m_vertex_stage_index]->m_program_object_id);
	GLU_EXPECT_NO_ERROR(gl.getError(), "UseProgramStages");
}

/** Test specific case
 *
 * @param bit_field An array of 5 bit fields used to set up subroutine uniforms, one element per context
 *
 * @return True if test pass, false otherwise
 **/
bool UniformPreservationTest::testCase(const glw::GLuint bit_field[5])
{
	/* Storage for subroutine indices */
	subroutineUniformSet captured_subroutine_indices[m_n_shared_contexts + 1];
	subroutineUniformSet subroutine_indices[m_n_shared_contexts + 1];

	/* Prepare subroutine_indices with bit fields */
	for (GLuint i = 0; i < m_n_shared_contexts + 1; ++i)
	{
		subroutine_indices[i].set(bit_field[i], m_subroutine_indices);
	};

	/* Update subroutine uniforms, each context gets different set */
	for (GLuint i = 0; i < m_n_shared_contexts; ++i)
	{
		m_shared_contexts[i]->makeCurrent();
		updateCurrentSubroutineSet(subroutine_indices[i]);
	}

	m_base_context->getRenderContext().makeCurrent();
	updateCurrentSubroutineSet(subroutine_indices[m_n_shared_contexts]);

	/* Capture subroutine uniforms */
	for (GLuint i = 0; i < m_n_shared_contexts; ++i)
	{
		m_shared_contexts[i]->makeCurrent();
		captureCurrentSubroutineSet(captured_subroutine_indices[i]);
	}

	m_base_context->getRenderContext().makeCurrent();
	captureCurrentSubroutineSet(captured_subroutine_indices[m_n_shared_contexts]);

	/* Verify that captured uniforms match expected values */
	for (GLuint i = 0; i < m_n_shared_contexts + 1; ++i)
	{
		if (subroutine_indices[i] != captured_subroutine_indices[i])
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "Error."
							   << " Context: " << i << " VS, expected: " << subroutine_indices[i].m_vertex_shader_stage
							   << " captured: " << captured_subroutine_indices[i].m_vertex_shader_stage
							   << " TCS, expected: " << subroutine_indices[i].m_tesselation_control_shader_stage
							   << " captured: " << captured_subroutine_indices[i].m_tesselation_control_shader_stage
							   << " TES, expected: " << subroutine_indices[i].m_tesselation_evaluation_shader_stage
							   << " captured: " << captured_subroutine_indices[i].m_tesselation_evaluation_shader_stage
							   << " GS, expected: " << subroutine_indices[i].m_geometry_shader_stage
							   << " captured: " << captured_subroutine_indices[i].m_geometry_shader_stage
							   << " FS, expected: " << subroutine_indices[i].m_fragment_shader_stage
							   << " captured: " << captured_subroutine_indices[i].m_fragment_shader_stage
							   << tcu::TestLog::EndMessage;

			return false;
		}
	}

	return true;
}

/** Test a program or pipeline
 *
 * @param programs     An array of 5 programs\ pointers, as in preparePrograms
 * @param is_separable Selects if monolithic or separable programs should be used
 * @param test_cases   Collection of test cases
 * @param n_test_cases Number of test cases
 *
 * @return True if all cases pass, false otherwise
 **/
bool UniformPreservationTest::testProgram(Utils::program** programs, bool is_separable,
										  const glw::GLuint test_cases[][5], glw::GLuint n_test_cases)
{
	/* Set program/pipeline as current for all contexts */
	if (false == is_separable)
	{
		programs[0]->use();

		for (GLuint i = 0; i < m_n_shared_contexts; ++i)
		{
			m_shared_contexts[i]->makeCurrent();
			programs[0]->use();
		}
	}
	else
	{
		/* GL entry points */
		const glw::Functions& gl = m_base_context->getRenderContext().getFunctions();

		/* Make sure that program pipeline will be used */
		gl.useProgram(0);
		GLU_EXPECT_NO_ERROR(gl.getError(), "UseProgram");

		prepareProgramPipeline(m_program_pipelines[m_n_shared_contexts], programs);

		for (GLuint i = 0; i < m_n_shared_contexts; ++i)
		{
			m_shared_contexts[i]->makeCurrent();

			/* Make sure that program pipeline will be used */
			gl.useProgram(0);
			GLU_EXPECT_NO_ERROR(gl.getError(), "UseProgram");

			prepareProgramPipeline(m_program_pipelines[i], programs);
		}
	}

	/* Execute test */
	bool result = true;
	for (GLuint i = 0; i < n_test_cases; ++i)
	{
		if (false == testCase(test_cases[i]))
		{
			result = false;
			break;
		}
	}

	return result;
}

/** Set up subroutine uniforms for current program or pipeline
 *
 * @param set Set of subroutine indices
 **/
void UniformPreservationTest::updateCurrentSubroutineSet(const subroutineUniformSet& set)
{
	/* GL entry points */
	const glw::Functions& gl = m_base_context->getRenderContext().getFunctions();

	/* Fragment */
	gl.uniformSubroutinesuiv(GL_FRAGMENT_SHADER, 1 /* count */, &set.m_fragment_shader_stage);
	GLU_EXPECT_NO_ERROR(gl.getError(), "UniformSubroutinesuiv");

	/* Geometry */
	gl.uniformSubroutinesuiv(GL_GEOMETRY_SHADER, 1 /* count */, &set.m_geometry_shader_stage);
	GLU_EXPECT_NO_ERROR(gl.getError(), "UniformSubroutinesuiv");

	/* Tess ctrl */
	gl.uniformSubroutinesuiv(GL_TESS_CONTROL_SHADER, 1 /* count */, &set.m_tesselation_control_shader_stage);
	GLU_EXPECT_NO_ERROR(gl.getError(), "UniformSubroutinesuiv");

	/* Tess eval */
	gl.uniformSubroutinesuiv(GL_TESS_EVALUATION_SHADER, 1 /* count */, &set.m_tesselation_evaluation_shader_stage);
	GLU_EXPECT_NO_ERROR(gl.getError(), "UniformSubroutinesuiv");

	/* Vertex */
	gl.uniformSubroutinesuiv(GL_VERTEX_SHADER, 1 /* count */, &set.m_vertex_shader_stage);
	GLU_EXPECT_NO_ERROR(gl.getError(), "UniformSubroutinesuiv");
}

/** Constructor.
 *
 *  @param context Rendering context.
 **/
MultipleContextsTests::MultipleContextsTests(tcu::TestContext& testCtx, glu::ApiType apiType)
	: tcu::TestCaseGroup(testCtx, "multiple_contexts", "Verifies \"shader_subroutine\" functionality")
	, m_apiType(apiType)
{
	/* Left blank on purpose */
}

/** Initializes a texture_storage_multisample test group.
 *
 **/
void MultipleContextsTests::init(void)
{
	addChild(new UniformPreservationTest(m_testCtx, m_apiType));
}

} /* glcts namespace */
