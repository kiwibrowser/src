#ifndef _GL3CCOMMONBUGSTESTS_HPP
#define _GL3CCOMMONBUGSTESTS_HPP
/*-------------------------------------------------------------------------
 * OpenGL Conformance Test Suite
 * -----------------------------
 *
 * Copyright (c) 2015-2016 The Khronos Group Inc.
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
 * \file  gl3cCommonBugsTests.hpp
 * \brief Tiny conformance tests which verify various pieces of functionality which have
 *        either been found to be broken on at least one publically available driver,
 *        or whose behavior was found to differ across vendors.
 */ /*-------------------------------------------------------------------*/

#ifndef _GLCTESTCASE_HPP
#include "glcTestCase.hpp"
#endif
#ifndef _GLWDEFS_HPP
#include "glwDefs.hpp"
#endif
#ifndef _TCUDEFS_HPP
#include "tcuDefs.hpp"
#endif

namespace gl3cts
{
/* Conformance test which verifies that glGetProgramiv() accepts the GL_ACTIVE_UNIFORM_BLOCK_MAX_NAME_LENGTH
 * pname and returns meaningful values. */
class GetProgramivActiveUniformBlockMaxNameLengthTest : public deqp::TestCase
{
public:
	/* Public methods */
	GetProgramivActiveUniformBlockMaxNameLengthTest(deqp::Context& context);

	void						 deinit();
	void						 init();
	tcu::TestNode::IterateResult iterate();

private:
	/* Private methods */
	bool initTest();

	/* Private members */
	glw::GLuint m_fs_id;
	glw::GLuint m_po_id;
	glw::GLuint m_vs_id;
};

/** Conformance test which verifies that input variables used in the cases described below
 *  cannot be set to any value:
 *
 *  - input variable defined in a fragment shader.
 *  - input variable, wrapped in an input block, defined in a fragment shader.
 *  - input variable defined in a geometry shader.
 *  - input variable, wrapped in an input block, defined in a geometry shader.
 *  - input variable defined in a tessellation control shader.
 *  - input variable, wrapped in an input block, defined in a tessellation control shader.
 *  - input patch variable. defined in a tessellation evaluation shader.
 *  - input variable defined in a tessellation evaluation shader.
 *  - input variable, wrapped in an input block, defined in a tessellation evaluation shader.
 *  - input variable defined in a vertex shader.
 *
 *  Furthermore, the test also verifies that an input variable cannot be passed as an inout
 *  or out function argument in any of the five shader stages.
 * */
class InputVariablesCannotBeModifiedTest : public deqp::TestCase
{
public:
	/* Public methods */
	InputVariablesCannotBeModifiedTest(deqp::Context& context);

	void						 deinit();
	void						 init();
	tcu::TestNode::IterateResult iterate();

private:
	/* Private type declarations */
	typedef enum {
		SHADER_STAGE_FRAGMENT,
		SHADER_STAGE_GEOMETRY,
		SHADER_STAGE_TESSELLATION_CONTROL,
		SHADER_STAGE_TESSELLATION_EVALUATION,
		SHADER_STAGE_VERTEX
	} _shader_stage;

	typedef enum {
		TEST_ITERATION_FIRST,

		TEST_ITERATION_INPUT_FS_VARIABLE = TEST_ITERATION_FIRST,
		TEST_ITERATION_INPUT_FS_VARIABLE_IN_INPUT_BLOCK,
		TEST_ITERATION_INPUT_FS_VARIABLE_PASSED_TO_INOUT_FUNCTION_PARAMETER,
		TEST_ITERATION_INPUT_FS_VARIABLE_PASSED_TO_OUT_FUNCTION_PARAMETER,
		TEST_ITERATION_INPUT_GS_VARIABLE,
		TEST_ITERATION_INPUT_GS_VARIABLE_IN_INPUT_BLOCK,
		TEST_ITERATION_INPUT_GS_VARIABLE_PASSED_TO_INOUT_FUNCTION_PARAMETER,
		TEST_ITERATION_INPUT_GS_VARIABLE_PASSED_TO_OUT_FUNCTION_PARAMETER,
		TEST_ITERATION_INPUT_TC_VARIABLE,
		TEST_ITERATION_INPUT_TC_VARIABLE_IN_INPUT_BLOCK,
		TEST_ITERATION_INPUT_TC_VARIABLE_PASSED_TO_INOUT_FUNCTION_PARAMETER,
		TEST_ITERATION_INPUT_TC_VARIABLE_PASSED_TO_OUT_FUNCTION_PARAMETER,
		TEST_ITERATION_INPUT_TE_PATCH_VARIABLE,
		TEST_ITERATION_INPUT_TE_VARIABLE,
		TEST_ITERATION_INPUT_TE_VARIABLE_IN_INPUT_BLOCK,
		TEST_ITERATION_INPUT_TE_VARIABLE_PASSED_TO_INOUT_FUNCTION_PARAMETER,
		TEST_ITERATION_INPUT_TE_VARIABLE_PASSED_TO_OUT_FUNCTION_PARAMETER,
		TEST_ITERATION_INPUT_VS_VARIABLE,
		TEST_ITERATION_INPUT_VS_VARIABLE_PASSED_TO_INOUT_FUNCTION_PARAMETER,
		TEST_ITERATION_INPUT_VS_VARIABLE_PASSED_TO_OUT_FUNCTION_PARAMETER,

		TEST_ITERATION_COUNT
	} _test_iteration;

	/* Private functions */
	void getIterationData(_test_iteration iteration, glu::ApiType* out_required_min_context_type_ptr,
						  _shader_stage* out_target_shader_stage_ptr, std::string* out_body_ptr) const;
	std::string getIterationName(_test_iteration iteration) const;
	std::string getShaderStageName(_shader_stage stage) const;

	/* Private members */
	glw::GLuint m_fs_id;
	glw::GLuint m_gs_id;
	glw::GLuint m_tc_id;
	glw::GLuint m_te_id;
	glw::GLuint m_vs_id;
};

/* Conformance test which verifies that:
 *
 * - !     operator does not accept bvec2, bvec3 and bvec4 arguments.
 * - all() function does not accept a bool argument.
 * - not() function does not accept a bool argument.
 */
class InvalidUseCasesForAllNotFuncsAndExclMarkOpTest : public deqp::TestCase
{
public:
	/* Public methods */
	InvalidUseCasesForAllNotFuncsAndExclMarkOpTest(deqp::Context& context);

	void						 deinit();
	void						 init();
	tcu::TestNode::IterateResult iterate();

private:
	/* Private type declarations */
	typedef enum {
		TEST_ITERATION_FIRST,

		TEST_ITERATION_EXCL_MARK_OP_MUST_NOT_ACCEPT_BVEC2 = TEST_ITERATION_FIRST,
		TEST_ITERATION_EXCL_MARK_OP_MUST_NOT_ACCEPT_BVEC3,
		TEST_ITERATION_EXCL_MARK_OP_MUST_NOT_ACCEPT_BVEC4,

		TEST_ITERATION_ALL_FUNC_MUST_NOT_ACCEPT_BOOL,
		TEST_ITERATION_NOT_FUNC_MUST_NOT_ACCEPT_BOOL,

		TEST_ITERATION_COUNT
	} _test_iteration;

	/* Private functions */
	std::string getIterationName(_test_iteration iteration) const;
	std::string getShaderBody(_test_iteration iteration) const;

	/* Private members */
	glw::GLuint m_vs_id;
};

/* Conformance test which verifies that all reserved names are rejected by the GL SL compiler
 * at compilation time, if used as:
 *
 * - Block names (input blocks, output blocks, SSBOs, UBOs)
 * - Function names
 * - Shader inputs
 * - Shader outputs
 * - Structure member name
 * - Structure names
 * - Subroutine names
 * - Uniform names
 * - Variable names
 *
 * in all shader stages supported for GL contexts, starting from GL 3.1.
 *
 * Support for all contexts (core profile where applicable) from GL 3.1
 * up to GL 4.5 is implemented.
 * */
class ReservedNamesTest : public deqp::TestCase
{
public:
	/* Public methods */
	ReservedNamesTest(deqp::Context& context);
	void						 deinit();
	void						 init();
	tcu::TestNode::IterateResult iterate();

private:
	/* Private type declarations */
	typedef enum {
		LANGUAGE_FEATURE_ATOMIC_COUNTER, /* tests usage of incorrectly named atomic counters */
		LANGUAGE_FEATURE_ATTRIBUTE,
		LANGUAGE_FEATURE_CONSTANT,
		LANGUAGE_FEATURE_FUNCTION_ARGUMENT_NAME, /* tests usage of incorrectly named function argument name */
		LANGUAGE_FEATURE_FUNCTION_NAME,			 /* tests usage of incorrectly named function name          */
		LANGUAGE_FEATURE_INPUT,
		LANGUAGE_FEATURE_INPUT_BLOCK_INSTANCE_NAME, /* tests usage of incorrectly named input block instance                   */
		LANGUAGE_FEATURE_INPUT_BLOCK_MEMBER_NAME, /* tests usage of an input block with an incorrectly named member variable */
		LANGUAGE_FEATURE_INPUT_BLOCK_NAME, /* tests usage of incorrectly named input block name                       */
		LANGUAGE_FEATURE_OUTPUT,
		LANGUAGE_FEATURE_OUTPUT_BLOCK_INSTANCE_NAME, /* tests usage of incorrectly named output block instance                          */
		LANGUAGE_FEATURE_OUTPUT_BLOCK_MEMBER_NAME, /* tests usage of an output block with an incorrectly named member variable        */
		LANGUAGE_FEATURE_OUTPUT_BLOCK_NAME, /* tests usage of incorrectly named output block name                              */
		LANGUAGE_FEATURE_SHADER_STORAGE_BLOCK_INSTANCE_NAME, /* tests usage of incorrectly named shader storage block instance                  */
		LANGUAGE_FEATURE_SHADER_STORAGE_BLOCK_MEMBER_NAME, /* tests usage of a shader storage block with an incorrectly named member variable */
		LANGUAGE_FEATURE_SHADER_STORAGE_BLOCK_NAME, /* tests usage of incorrectly named shader storage block name                      */
		LANGUAGE_FEATURE_SHARED_VARIABLE,
		LANGUAGE_FEATURE_STRUCTURE_MEMBER, /* tests usage of a structure whose member variable is incorrectly named           */
		LANGUAGE_FEATURE_STRUCTURE_INSTANCE_NAME, /* tests usage of a structure whose instance is incorrectly named                  */
		LANGUAGE_FEATURE_STRUCTURE_NAME, /* tests usage of a structure whose name is incorrect                              */
		LANGUAGE_FEATURE_SUBROUTINE_FUNCTION_NAME, /* tests usage of incorrectly named subroutine functions                           */
		LANGUAGE_FEATURE_SUBROUTINE_TYPE, /* tests usage of incorrectly named subroutine types                               */
		LANGUAGE_FEATURE_SUBROUTINE_UNIFORM, /* tests usage of incorrectly named subroutine uniforms                            */
		LANGUAGE_FEATURE_UNIFORM, /* tests usage of incorrectly named sampler2D uniforms                             */
		LANGUAGE_FEATURE_UNIFORM_BLOCK_INSTANCE_NAME, /* tests usage of incorrectly named shader storage block instance                  */
		LANGUAGE_FEATURE_UNIFORM_BLOCK_MEMBER_NAME, /* tests usage of a shader storage block with an incorrectly named member variable */
		LANGUAGE_FEATURE_UNIFORM_BLOCK_NAME, /* tests usage of incorrectly named shader storage block name                      */
		LANGUAGE_FEATURE_VARIABLE,
		LANGUAGE_FEATURE_VARYING,

		LANGUAGE_FEATURE_COUNT
	} _language_feature;

	typedef enum {
		SHADER_TYPE_COMPUTE,
		SHADER_TYPE_FRAGMENT,
		SHADER_TYPE_GEOMETRY,
		SHADER_TYPE_TESS_CONTROL,
		SHADER_TYPE_TESS_EVALUATION,
		SHADER_TYPE_VERTEX,

		SHADER_TYPE_COUNT
	} _shader_type;

	/* Private functions */
	std::string getLanguageFeatureName(_language_feature language_feature) const;
	std::vector<std::string> getReservedNames() const;
	std::string getShaderBody(_shader_type shader_type, _language_feature language_feature,
							  const char* invalid_name) const;
	std::string getShaderTypeName(_shader_type shader_type) const;
	std::vector<_language_feature> getSupportedLanguageFeatures(_shader_type shader_type) const;
	std::vector<_shader_type> getSupportedShaderTypes() const;
	bool isStructAllowed(_shader_type shader_type, _language_feature language_feature) const;

	/* Private members */
	glw::GLint  m_max_fs_ssbos;
	glw::GLint  m_max_gs_acs;
	glw::GLint  m_max_gs_ssbos;
	glw::GLint  m_max_tc_acs;
	glw::GLint  m_max_tc_ssbos;
	glw::GLint  m_max_te_acs;
	glw::GLint  m_max_te_ssbos;
	glw::GLint  m_max_vs_acs;
	glw::GLint  m_max_vs_ssbos;
	glw::GLuint m_so_ids[SHADER_TYPE_COUNT];
};

/* Conformance test which verifies that the following types, used to declare a vertex
 * shader input variable, result in a compilation-time error:
 *
 * - bool, bvec2, bvec3, bvec4
 * - opaque type
 * - structure
 *
 * The test also verifies that it is illegal to use any of the following qualifiers
 * for an otherwise valid vertex shader input variable declaration:
 *
 * - centroid
 * - patch
 * - sample
 */
class InvalidVSInputsTest : public deqp::TestCase
{
public:
	/* Public methods */
	InvalidVSInputsTest(deqp::Context& context);

	void						 deinit();
	void						 init();
	tcu::TestNode::IterateResult iterate();

private:
	/* Private type declarations */
	typedef enum {
		TEST_ITERATION_FIRST,

		TEST_ITERATION_INVALID_BOOL_INPUT = TEST_ITERATION_FIRST,
		TEST_ITERATION_INVALID_BVEC2_INPUT,
		TEST_ITERATION_INVALID_BVEC3_INPUT,
		TEST_ITERATION_INVALID_BVEC4_INPUT,
		TEST_ITERATION_INVALID_CENTROID_QUALIFIED_INPUT,
		TEST_ITERATION_INVALID_PATCH_QUALIFIED_INPUT,
		TEST_ITERATION_INVALID_OPAQUE_TYPE_INPUT,
		TEST_ITERATION_INVALID_STRUCTURE_INPUT,
		TEST_ITERATION_INVALID_SAMPLE_QUALIFIED_INPUT,

		TEST_ITERATION_COUNT
	} _test_iteration;

	/* Private functions */
	std::string getIterationName(_test_iteration iteration) const;
	std::string getShaderBody(_test_iteration iteration) const;

	/* Private members */
	glw::GLuint m_vs_id;
};

/* Conformance test which verifies that parenthesis are not accepted in compute shaders, prior to GL4.4,
 * unless GL_ARB_enhanced_layouts is supported.
 */
class ParenthesisInLayoutQualifierIntegerValuesTest : public deqp::TestCase
{
public:
	/* Public methods */
	ParenthesisInLayoutQualifierIntegerValuesTest(deqp::Context& context);

	void						 deinit();
	void						 init();
	tcu::TestNode::IterateResult iterate();

private:
	/* Private members */
	glw::GLuint m_cs_id;
	glw::GLuint m_po_id;
};

/* Conformance test which verifies that gl_PerVertex block re-declarations are required by
 * the OpenGL implementation if separate shader object functionality is used.
 *
 * Additionally, the test also checks that the following test cases result in an error:
 *
 * - Usage of any of the input/output built-in variables in any of the five shader stages, with the
 *   variable not being defined in the re-declared gl_PerVertex block.
 * - gl_PerVertex block re-declarations defined in a different manner for each of the used shader stages.
 *
 * Each test iteration is run in two "modes":
 *
 * 1. A pipeline object is created and shader programs are attached to it. It is expected that validation
 *    should fail.
 * 2. A single separate shader program, to which all shader stages used by the test are attached, is linked.
 *    It is expected the linking process should fail.
 *
 */
class PerVertexValidationTest : public deqp::TestCase
{
public:
	/* Public methods */
	PerVertexValidationTest(deqp::Context& context);

	void						 deinit();
	void						 init();
	tcu::TestNode::IterateResult iterate();

private:
	/* Private type definitions */
	typedef enum {
		SHADER_STAGE_FRAGMENT				 = 1 << 0,
		SHADER_STAGE_GEOMETRY				 = 1 << 1,
		SHADER_STAGE_TESSELLATION_CONTROL	= 1 << 2,
		SHADER_STAGE_TESSELLATION_EVALUATION = 1 << 3,
		SHADER_STAGE_VERTEX					 = 1 << 4
	} _shader_stage;

	typedef enum {
		TEST_ITERATION_FIRST,

		TEST_ITERATION_UNDECLARED_IN_PERVERTEX_GS_GL_CLIPDISTANCE_USAGE = TEST_ITERATION_FIRST,
		TEST_ITERATION_UNDECLARED_IN_PERVERTEX_GS_GL_CULLDISTANCE_USAGE,
		TEST_ITERATION_UNDECLARED_IN_PERVERTEX_GS_GL_POINTSIZE_USAGE,
		TEST_ITERATION_UNDECLARED_IN_PERVERTEX_GS_GL_POSITION_USAGE,
		TEST_ITERATION_UNDECLARED_IN_PERVERTEX_TC_GL_CLIPDISTANCE_USAGE,
		TEST_ITERATION_UNDECLARED_IN_PERVERTEX_TC_GL_CULLDISTANCE_USAGE,
		TEST_ITERATION_UNDECLARED_IN_PERVERTEX_TC_GL_POINTSIZE_USAGE,
		TEST_ITERATION_UNDECLARED_IN_PERVERTEX_TC_GL_POSITION_USAGE,
		TEST_ITERATION_UNDECLARED_IN_PERVERTEX_TE_GL_CLIPDISTANCE_USAGE,
		TEST_ITERATION_UNDECLARED_IN_PERVERTEX_TE_GL_CULLDISTANCE_USAGE,
		TEST_ITERATION_UNDECLARED_IN_PERVERTEX_TE_GL_POINTSIZE_USAGE,
		TEST_ITERATION_UNDECLARED_IN_PERVERTEX_TE_GL_POSITION_USAGE,

		TEST_ITERATION_UNDECLARED_OUT_PERVERTEX_GS_GL_CLIPDISTANCE_USAGE,
		TEST_ITERATION_UNDECLARED_OUT_PERVERTEX_GS_GL_CULLDISTANCE_USAGE,
		TEST_ITERATION_UNDECLARED_OUT_PERVERTEX_GS_GL_POINTSIZE_USAGE,
		TEST_ITERATION_UNDECLARED_OUT_PERVERTEX_GS_GL_POSITION_USAGE,
		TEST_ITERATION_UNDECLARED_OUT_PERVERTEX_TC_GL_CLIPDISTANCE_USAGE,
		TEST_ITERATION_UNDECLARED_OUT_PERVERTEX_TC_GL_CULLDISTANCE_USAGE,
		TEST_ITERATION_UNDECLARED_OUT_PERVERTEX_TC_GL_POINTSIZE_USAGE,
		TEST_ITERATION_UNDECLARED_OUT_PERVERTEX_TC_GL_POSITION_USAGE,
		TEST_ITERATION_UNDECLARED_OUT_PERVERTEX_TE_GL_CLIPDISTANCE_USAGE,
		TEST_ITERATION_UNDECLARED_OUT_PERVERTEX_TE_GL_CULLDISTANCE_USAGE,
		TEST_ITERATION_UNDECLARED_OUT_PERVERTEX_TE_GL_POINTSIZE_USAGE,
		TEST_ITERATION_UNDECLARED_OUT_PERVERTEX_TE_GL_POSITION_USAGE,
		TEST_ITERATION_UNDECLARED_OUT_PERVERTEX_VS_GL_CLIPDISTANCE_USAGE,
		TEST_ITERATION_UNDECLARED_OUT_PERVERTEX_VS_GL_CULLDISTANCE_USAGE,
		TEST_ITERATION_UNDECLARED_OUT_PERVERTEX_VS_GL_POINTSIZE_USAGE,
		TEST_ITERATION_UNDECLARED_OUT_PERVERTEX_VS_GL_POSITION_USAGE,

		TEST_ITERATION_PERVERTEX_DECLARATION_MISMATCH_GS_VS,
		TEST_ITERATION_PERVERTEX_DECLARATION_MISMATCH_GS_TC_TE_VS,
		TEST_ITERATION_PERVERTEX_DECLARATION_MISMATCH_TC_TE_VS,

		TEST_ITERATION_PERVERTEX_BLOCK_UNDEFINED,

		/* Always last */
		TEST_ITERATION_COUNT
	} _test_iteration;

	typedef enum {
		VALIDATION_RESULT_FALSE,
		VALIDATION_RESULT_TRUE,
		VALIDATION_RESULT_UNDEFINED,
	} _validation;

	/* Private functions */
	void		destroyPOsAndSOs();
	_validation getProgramPipelineValidationExpectedResult(void) const;
	std::string getShaderStageName(_shader_stage shader_stage) const;
	std::string getTestIterationName(_test_iteration iteration) const;

	void getTestIterationProperties(glu::ContextType context_type, _test_iteration iteration,
									glu::ContextType* out_min_context_type_ptr,
									_shader_stage* out_used_shader_stages_ptr, std::string* out_gs_body_ptr,
									std::string* out_tc_body_ptr, std::string* out_te_body_ptr,
									std::string* out_vs_body_ptr) const;

	std::string getVertexShaderBody(glu::ContextType context_type, _test_iteration iteration,
									std::string main_body = std::string("gl_Position = vec4(1.0);")) const;

	bool isShaderProgramLinkingFailureExpected(_test_iteration iteration) const;
	bool runPipelineObjectValidationTestMode(_test_iteration iteration);
	bool runSeparateShaderTestMode(_test_iteration iteration);

	/* Private members */
	glw::GLuint m_fs_id;
	glw::GLuint m_fs_po_id;
	glw::GLuint m_gs_id;
	glw::GLuint m_gs_po_id;
	glw::GLuint m_pipeline_id;
	glw::GLuint m_tc_id;
	glw::GLuint m_tc_po_id;
	glw::GLuint m_te_id;
	glw::GLuint m_te_po_id;
	glw::GLuint m_vs_id;
	glw::GLuint m_vs_po_id;
};

/* Conformance test which verifies that glCopyBufferSubData() and glBufferSubData() calls, executed
 * within a single page boundary, work correctly. The test is ran for a number of consecutive pages,
 * a predefined number of times, where each pass is separated by a front/back buffer swap operation.
 */
class SparseBuffersWithCopyOpsTest : public deqp::TestCase
{
public:
	/* Public methods */
	SparseBuffersWithCopyOpsTest(deqp::Context& context);

	void						 deinit();
	void						 init();
	tcu::TestNode::IterateResult iterate();

private:
	/* Private methods */
	bool initTest();

	/* Private members */
	glw::GLuint	m_bo_id;
	glw::GLuint	m_bo_read_id;
	unsigned char* m_clear_buffer;
	glw::GLint	 m_page_size;
	unsigned char  m_reference_data[16];
	unsigned int   m_result_data_storage_size;

	const unsigned int m_n_iterations_to_run;
	const unsigned int m_n_pages_to_test;
	const unsigned int m_virtual_bo_size;
};

/** Test group which encapsulates all "common bugs" conformance tests */
class CommonBugsTests : public deqp::TestCaseGroup
{
public:
	/* Public methods */
	CommonBugsTests(deqp::Context& context);

	void init();

private:
	CommonBugsTests(const CommonBugsTests& other);
	CommonBugsTests& operator=(const CommonBugsTests& other);
};
} /* glcts namespace */

#endif // _GL3CCOMMONBUGSTESTS_HPP
