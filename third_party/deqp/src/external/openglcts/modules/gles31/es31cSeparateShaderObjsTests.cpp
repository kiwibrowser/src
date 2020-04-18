/*-------------------------------------------------------------------------
 * OpenGL Conformance Test Suite
 * -----------------------------
 *
 * Copyright (c) 2014-2016 The Khronos Group Inc.
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

#include "es31cSeparateShaderObjsTests.hpp"
#include "deMath.h"
#include "deRandom.hpp"
#include "deString.h"
#include "deStringUtil.hpp"
#include "gluDrawUtil.hpp"
#include "gluPixelTransfer.hpp"
#include "gluShaderProgram.hpp"
#include "glw.h"
#include "glwEnums.hpp"
#include "glwFunctions.hpp"
#include "tcuImageCompare.hpp"
#include "tcuRenderTarget.hpp"
#include "tcuStringTemplate.hpp"
#include "tcuSurface.hpp"
#include "tcuTestLog.hpp"
#include "tcuVector.hpp"

#include <map>

/*************************************************************************/
/* Test Plan for shared_shader_objects:
 * Overview
 *
 *    This is a conformance test for XXX_separate_shader_objects extension. The
 *    results of the tests are verified by checking the expected GL error,
 *    by checking the expected state with state query functions and by rendering
 *    test images and comparing specified pixels with expected values. Not all of
 *    the methods are applicable for all tests. Additional details such as
 *    expected error codes are included in test descriptions.
 *
 *New Tests
 *
 *--  Tests for invidual API functions
 *
 *
 *    CreateShaderProgramv tests
 *
 *    Positive tests:
 *
 *    * Test with valid parameters and verify that program and GL state are set
 *      accordingly to the code sequence defined in the extension spec.
 *    * Test with vertex and fragment shader types.
 *    * Test with few different count/strings parameters (count >= 1)
 *
 *    Negative tests:
 *
 *    * Test with invalid type. Expect INVALID_ENUM and zero return value.
 *    * Test with uncompilable / unlinkable source. Expect no errors. Program
 *      should be returned Program info log may contain information about link /
 *      compile failure.
 *    * Test with count < 0. Expect INVALID_VALUE and zero return value.
 *
 *
 *    UseProgramStages tests
 *
 *    Positive tests:
 *
 *    * Test with a program containing
 *      - vertex stage only
 *      - fragment stage only
 *      - both stages
 *    * Test with null program to reset a stage or stages. Expect no errors.
 *    * Test with a program that doesn't contain code for a stage defined in the
 *      stages bitfield. Expect no errors.
 *    * Test with a new program pipeline object that has not yet been used/bound.
 *
 *    Negative tests:
 *
 *    * Test with invalid stages bitfield (with unused bits). Expect INVALID_VALUE GL error.
 *    * Test with deleted/nonexistent pipeline. Expect INVALID_OPERATION GL error.
 *    * Test with program that isn't separable. Expect INVALID_OPERATION GL error.
 *    * Test with program that isn't linked succesfully. Expect INVALID_OPERATION
 *      GL error.
 *    * Test with deleted/nonexistent program. Expect INVALID_OPERATION error.
 *
 *
 *    ActiveShaderProgram tests
 *
 *    Positive tests:
 *
 *    * Test with a new program pipeline object that has not yet been used/bound.
 *
 *    Negative tests:
 *
 *    * Test with deleted/nonexistent program pipeline object. Expect INVALID_OPERATION and no
 *      changes to active program.
 *    * Test with nonexistent/deleted/unsuccesfully linked program. Expect
 *      INVALID_OPERATION GL error and no changes to active program.
 *
 *
 *    GenProgramPipelines tests
 *
 *    Positive tests:
 *
 *    * Test creating different amounts of program pipeline object names.
 *      Verify with IsProgramPipeline.
 *
 *
 *    BindProgramPipeline tests
 *
 *    Positive tests:
 *
 *    * Test binding existing program pipeline object. Verify with
 *      PROGRAM_PIPELINE_BINDING
 *    * Test binding zero program pipeline object. Verify
 *      PROGRAM_PIPELINE_BINDING is reset to 0
 *
 *    Negative tests:
 *
 *    * Test binding deleted/nonexistent program pipeline object. Expect
 *      INVALID_OPERATION GL error and no changes to bound pipeline.
 *
 *
 *    DeleteProgramPipelines tests
 *
 *    Positive tests:
 *
 *    * Test deleting zero and unused names. Expect no errors (should be no-op)
 *    * Test deleting different amounts of existing pipeline names. Verify
 *      deletion with IsProgramPipeline.
 *    * Test deleting bound names. Expect pipeline binding revert to zero, verify
 *      with PROGRAM_PIPELINE_BINDING.
 *
 *
 *    IsProgramPipeline
 *
 *    Positive tests:
 *
 *    * Test with deleted/nonexistent program pipeline names.
 *    * Test with existing program pipeline names.
 *
 *
 *    ProgramParameteri / PROGRAM_SEPARABLE tests
 *
 *    Positive tests:
 *
 *    * Test setting TRUE and FALSE values for existing, unlinked programs.
 *      Verify with GetProgramParameteri
 *
 *    Negative tests:
 *
 *    * Test with nonexistent/deleted program. Expect INVALID_OPERATION GL error
 *    * Test with invalid value. Expect INVALID_VALUE GL error
 *
 *
 *    GetProgramPipelineiv tests
 *
 *    Positive tests:
 *
 *    * Test with new program pipeline object that has not yet been used/bound
 *    * Test ACTIVE_PROGRAM
 *    * Test VERTEX_SHADER
 *    * Test FRAGMENT_SHADER
 *    * Test VALIDATE_STATUS
 *    * Test INFO_LOG_LENGTH
 *
 *    Negative tests:
 *
 *    * Test with deleted/nonexistent pipeline. Expect INVALID_OPERATION GL error
 *
 *
 *    ValidateProgramPipeline tests:
 *
 *    Positive tests:
 *
 *    * Test with valid program pipeline. Expect VALIDATE_STATUS = TRUE
 *    * Test with invalid program pipeline Expect VALIDATE_STATUS = FALSE
 *    * Test with empty program pipeline (uninitialized, but bound). Expect VALIDATE_STATUS = FALSE.
 *    * Test that initial (unvalidated) VALIDATE_STATUS is FALSE
 *    * Test with a new program pipeline object that has not been used/bound yet
 *
 *    Negative tests:
 *
 *    * Test with deleted/nonexistent program pipeline object. Expect
 *      INVALID_OPERATION
 *
 *
 *    ProgramUniform* tests
 *
 *    Positive tests:
 *
 *    * Test all ProgramUniform* methods with few different parameters combinations
 *    * Setup pipeline with programs A and B. Update uniforms for A and verify
 *      that only A is affected.
 *    * Test with a program with all combinations of
 *      - program is/isn't part of a bound pipeline
 *      - program is/isn't made current with UseProgram
 *      - program is/isn't made active with ActiveShaderProgram
 *      in all cases, only the uniforms of the specified program should be
 *      updated.
 *
 *    Negative tests:
 *
 *    * Test with deleted/nonexistent program. Expect INVALID_VALUE GL error.
 *    * Test with unsuccesfully linked program. Expect INVALID_OPERATION GL error.
 *
 *
 *    GetProgramPipelineInfoLog tests
 *
 *    Run ValidateProgramPipeline for valid / invalid program pipeline object
 *    before running the tests. NOTE: The spec doesn't require that the driver
 *    updates the pipeline info log. It may or may not contain information about
 *    validation.
 *
 *    Positive tests
 *
 *    * Test with NULL length.
 *    * Test with zero bufSize. Expect no errors
 *    * Test with varying bufSizes (where 0 < bufSize <= INFO_LOG_LENGTH). Except
 *    * length = (bufSize - 1) and zero-terminated string with matching length in
 *      infoLog.
 *
 *    Negative tests
 *
 *    * Test with deleted/nonexistent program pipeline object. Expect
 *      GL_INVALID_VALUE error (the error is still missing from the spec)
 *
 *
 *--  Other tests
 *
 *
 *    UseProgram vs. BindProgramPipeline tests
 *
 *    Positive tests:
 *
 *    * Test that a program made active with UseProgram has precedence over
 *      program pipeline object bound with BindProgramPipeline.
 *    * Test that program(s) in bound program pipeline object will be used if
 *      there is no active program set with UseProgram
 *    * Test that a state without active program or without bound pipeline object
 *      generates no errors.
 *
 *
 *    Pipeline setup tests
 *
 *    Positive tests:
 *
 *    * Test that missing pipeline stages produces no errors:
 *      - no program set with UseProgramStages for vertex or frargment stages
 *      - no vertex or fragment code in a program set for the stage
 *
 *    Negative tests:
 *
 *    * Test that program with both vertex and fragment shaders cannot be attached
 *      just to vertex or fragment stage. Expect DrawArrays/Elements to generate
 *      INVALID_OPERATION and pipeline VALIDATE_STATUS set to FALSE.
 *      - Run with and without validating the pipeline with ValidateProgramPipeline
 *
 *
 *    Shader/program management tests
 *
 *    Positive tests:
 *
 *    * Test creating separable shader objects both by
 *      - Using the core functions combined with PROGRAM_SEPARABLE flag
 *      - CreateShaderProgram
 *    * Test that separable program can contain and links properly if there are
 *      - vertex stage
 *      - fragment stage
 *      - both stages
 *    * Test that active program isn't deleted immediately (deletion doesn't
 *      affect rendering state)
 *    * Test that program in current pipeline isn't deleted immediately
 *    * Test that attaching/detaching/recompiling a shader in active program or
 *      program in current pipeline doesn't affect the program link status or
 *      rendering state.
 *    * Test that succesfully re-linking active program or program in current
 *      pipeline affects the rendering state.
 *
 *    Negative tests:
 *
 *      aren't present.
 *    * Test that unsuccesfully re-linking active program or program in current
 *      pipeline sets LINK_STATUS=FALSE but doesn't affect the rendering state.
 *    * Test that unsuccesfully linked program cannot be made part of a program
 *      pipeline object.
 *
 *
 *    Uniform update tests
 *
 *    Positive cases:
 *
 *      with UseProgram.
 *    * Test that Uniform* functions update the uniforms of a program made active with
 *      ActiveShader program if no program has been made active with UseProgram.
 *    * Test that ProgramUniform* functions update the uniforms of a specified
 *      program regardless of active program (probably already covered with
 *      "ProgramUniform* tests")
 *
 *    Negative cases:
 *
 *    * Test that Uniform* functions set INVALID_OPERATION if there is no active
 *      program set with UseProgram nor ActiveShaderProgram
 *
 *
 *    Shader interface matching tests
 *
 *    Positive tests:
 *
 *    * Test that partially or completely mismatching shaders do not generate
 *      validation nor other GL errors (just undefined inputs)
 *    * Test that exactly matching shaders work.
 *    * Test that variables with matching layout qualifiers match and are defined
 *      even if the shaders don't match exactly.
 *      - Test with int, uint and float component types
 *      - Test with different vector sizes, where output vector size >= input
 *        vector size
 *
 *
 * End Test Plan */
/*************************************************************************/

namespace glcts
{

using tcu::TestLog;
using std::string;
using std::vector;

// A fragment shader to allow testing various scalar and vector
// uniforms as well as array [2] varieties.  To keep the uniforms
// active they are compared against constants.
static const char* s_unifFragShaderSrc =
	"precision highp float;\n"
	"uniform ${SCALAR_TYPE}  uVal0;\n"
	"uniform ${VECTOR_TYPE}2 uVal1;\n"
	"uniform ${VECTOR_TYPE}3 uVal2;\n"
	"uniform ${VECTOR_TYPE}4 uVal3;\n"
	"\n"
	"uniform ${SCALAR_TYPE}  uVal4[2];\n"
	"uniform ${VECTOR_TYPE}2 uVal5[2];\n"
	"uniform ${VECTOR_TYPE}3 uVal6[2];\n"
	"uniform ${VECTOR_TYPE}4 uVal7[2];\n"
	"\n"
	"const ${SCALAR_TYPE}  kVal0= 1${SFX};\n"
	"const ${VECTOR_TYPE}2 kVal1 = ${VECTOR_TYPE}2(2${SFX}, 3${SFX});\n"
	"const ${VECTOR_TYPE}3 kVal2 = ${VECTOR_TYPE}3(4${SFX}, 5${SFX}, 6${SFX});\n"
	"const ${VECTOR_TYPE}4 kVal3 = ${VECTOR_TYPE}4(7${SFX}, 8${SFX}, 9${SFX}, 10${SFX});\n"
	"\n"
	"const ${SCALAR_TYPE}  kArr4_0 = 11${SFX};\n"
	"const ${SCALAR_TYPE}  kArr4_1 = 12${SFX};\n"
	"const ${VECTOR_TYPE}2 kArr5_0 = ${VECTOR_TYPE}2(13${SFX}, 14${SFX});\n"
	"const ${VECTOR_TYPE}2 kArr5_1 = ${VECTOR_TYPE}2(15${SFX}, 16${SFX});\n"
	"const ${VECTOR_TYPE}3 kArr6_0 = ${VECTOR_TYPE}3(17${SFX}, 18${SFX}, 19${SFX});\n"
	"const ${VECTOR_TYPE}3 kArr6_1 = ${VECTOR_TYPE}3(20${SFX}, 21${SFX}, 22${SFX});\n"
	"const ${VECTOR_TYPE}4 kArr7_0 = ${VECTOR_TYPE}4(23${SFX}, 24${SFX}, 25${SFX}, 26${SFX});\n"
	"const ${VECTOR_TYPE}4 kArr7_1 = ${VECTOR_TYPE}4(27${SFX}, 28${SFX}, 29${SFX}, 30${SFX});\n"
	"\n"
	"layout(location = 0) out mediump vec4 o_color;\n"
	"\n"
	"void main() {\n"
	"    if ((uVal0 != kVal0) ||\n"
	"        (uVal1 != kVal1) ||\n"
	"        (uVal2 != kVal2) ||\n"
	"        (uVal3 != kVal3) ||\n"
	"        (uVal4[0] != kArr4_0) || (uVal4[1] != kArr4_1) ||\n"
	"        (uVal5[0] != kArr5_0) || (uVal5[1] != kArr5_1) ||\n"
	"        (uVal6[0] != kArr6_0) || (uVal6[1] != kArr6_1) ||\n"
	"        (uVal7[0] != kArr7_0) || (uVal7[1] != kArr7_1)) {\n"
	"        o_color = vec4(1.0, 0.0, 0.0, 1.0);\n"
	"    } else {\n"
	"        o_color = vec4(0.0, 1.0, 0.0, 1.0);\n"
	"    }\n"
	"}\n";

// A fragment shader to test uniforms of square matrices
static const char* s_unifFragSquareMatShaderSrc = "precision highp float;\n"
												  "uniform mat2 uValM2[2];\n"
												  "uniform mat3 uValM3[2];\n"
												  "uniform mat4 uValM4[2];\n"
												  "\n"
												  "const mat2 kMat2_0 = mat2(91.0, 92.0, 93.0, 94.0);\n"
												  "const mat2 kMat2_1 = mat2(95.0, 96.0, 97.0, 98.0);\n"
												  "const mat3 kMat3_0 = mat3(vec3( 99.0, 100.0, 101.0),\n"
												  "                          vec3(102.0, 103.0, 104.0),\n"
												  "                          vec3(105.0, 106.0, 107.0));\n"
												  "const mat3 kMat3_1 = mat3(vec3(108.0, 109.0, 110.0),\n"
												  "                          vec3(111.0, 112.0, 113.0),\n"
												  "                          vec3(114.0, 115.0, 116.0));\n"
												  "const mat4 kMat4_0 = mat4(vec4(117.0, 118.0, 119.0, 120.0),\n"
												  "                          vec4(121.0, 122.0, 123.0, 124.0),\n"
												  "                          vec4(125.0, 126.0, 127.0, 128.0),\n"
												  "                          vec4(129.0, 130.0, 131.0, 132.0));\n"
												  "const mat4 kMat4_1 = mat4(vec4(133.0, 134.0, 135.0, 136.0),\n"
												  "                          vec4(137.0, 138.0, 139.0, 140.0),\n"
												  "                          vec4(141.0, 142.0, 143.0, 144.0),\n"
												  "                          vec4(145.0, 146.0, 147.0, 148.0));\n"
												  "\n"
												  "layout(location = 0) out mediump vec4 o_color;\n"
												  "\n"
												  "void main() {\n"
												  "    if ((uValM2[0] != kMat2_0) || (uValM2[1] != kMat2_1) ||\n"
												  "        (uValM3[0] != kMat3_0) || (uValM3[1] != kMat3_1) ||\n"
												  "        (uValM4[0] != kMat4_0) || (uValM4[1] != kMat4_1)) {\n"
												  "        o_color = vec4(1.0, 0.0, 0.0, 1.0);\n"
												  "    } else {\n"
												  "        o_color = vec4(0.0, 1.0, 0.0, 1.0);\n"
												  "    }\n"
												  "}\n";

// A fragment shader to test uniforms of square matrices
static const char* s_unifFragNonSquareMatShaderSrc =
	"precision highp float;\n"
	"uniform mat2x3 uValM2x3[2];\n"
	"uniform mat3x2 uValM3x2[2];\n"
	"uniform mat2x4 uValM2x4[2];\n"
	"uniform mat4x2 uValM4x2[2];\n"
	"uniform mat3x4 uValM3x4[2];\n"
	"uniform mat4x3 uValM4x3[2];\n"
	"\n"
	"const mat2x3 kMat2x3_0 = mat2x3(vec2(149.0, 150.0),\n"
	"                                vec2(151.0, 152.0),\n"
	"                                vec2(153.0, 154.0));\n"
	"const mat2x3 kMat2x3_1 = mat2x3(vec2(155.0, 156.0),\n"
	"                                vec2(157.0, 158.0),\n"
	"                                vec2(159.0, 160.0));\n"
	"const mat3x2 kMat3x2_0 = mat3x2(vec3(161.0, 162.0, 163.0),\n"
	"                                vec3(164.0, 165.0, 166.0));\n"
	"const mat3x2 kMat3x2_1 = mat3x2(vec3(167.0, 168.0, 169.0),\n"
	"                                vec3(170.0, 171.0, 172.0));\n"
	"const mat2x4 kMat2x4_0 = mat2x4(vec2(173.0, 174.0),\n"
	"                                vec2(175.0, 176.0),\n"
	"                                vec2(177.0, 178.0),\n"
	"                                vec2(179.0, 180.0));\n"
	"const mat2x4 kMat2x4_1 = mat2x4(vec2(181.0, 182.0),\n"
	"                                vec2(183.0, 184.0),\n"
	"                                vec2(185.0, 186.0),\n"
	"                                vec2(187.0, 188.0));\n"
	"const mat4x2 kMat4x2_0 = mat4x2(vec4(189.0, 190.0, 191.0, 192.0),\n"
	"                                vec4(193.0, 194.0, 195.0, 196.0));\n"
	"const mat4x2 kMat4x2_1 = mat4x2(vec4(197.0, 198.0, 199.0, 200.0),\n"
	"                                vec4(201.0, 202.0, 203.0, 204.0));\n"
	"const mat3x4 kMat3x4_0 = mat3x4(vec3(205.0, 206.0, 207.0),\n"
	"                                vec3(208.0, 209.0, 210.0),\n"
	"                                vec3(211.0, 212.0, 213.0),\n"
	"                                vec3(214.0, 215.0, 216.0));\n"
	"const mat3x4 kMat3x4_1 = mat3x4(vec3(217.0, 218.0, 219.0),\n"
	"                                vec3(220.0, 221.0, 222.0),\n"
	"                                vec3(223.0, 224.0, 225.0),\n"
	"                                vec3(226.0, 227.0, 228.0));\n"
	"const mat4x3 kMat4x3_0 = mat4x3(vec4(229.0, 230.0, 231.0, 232.0),\n"
	"                                vec4(233.0, 234.0, 235.0, 236.0),\n"
	"                                vec4(237.0, 238.0, 239.0, 240.0));\n"
	"const mat4x3 kMat4x3_1 = mat4x3(vec4(241.0, 242.0, 243.0, 244.0),\n"
	"                                vec4(245.0, 246.0, 247.0, 248.0),\n"
	"                                vec4(249.0, 250.0, 251.0, 252.0));\n"
	"\n"
	"layout(location = 0) out mediump vec4 o_color;\n"
	"\n"
	"void main() {\n"
	"    if ((uValM2x3[0] != kMat2x3_0) || (uValM2x3[1] != kMat2x3_1) ||\n"
	"        (uValM3x2[0] != kMat3x2_0) || (uValM3x2[1] != kMat3x2_1) ||\n"
	"        (uValM2x4[0] != kMat2x4_0) || (uValM2x4[1] != kMat2x4_1) ||\n"
	"        (uValM4x2[0] != kMat4x2_0) || (uValM4x2[1] != kMat4x2_1) ||\n"
	"        (uValM3x4[0] != kMat3x4_0) || (uValM3x4[1] != kMat3x4_1) ||\n"
	"        (uValM4x3[0] != kMat4x3_0) || (uValM4x3[1] != kMat4x3_1)) {\n"
	"        o_color = vec4(1.0, 0.0, 0.0, 1.0);\n"
	"    } else {\n"
	"        o_color = vec4(0.0, 1.0, 0.0, 1.0);\n"
	"    }\n"
	"}\n";

static std::string generateBasicVertexSrc(glu::GLSLVersion glslVersion)
{
	std::stringstream str;

	str << glu::getGLSLVersionDeclaration(glslVersion) << "\n";
	str << "in highp vec4 a_position;\n";
	if (glslVersion >= glu::GLSL_VERSION_410)
	{
		str << "out gl_PerVertex {\n"
			   "  vec4 gl_Position;\n"
			   "};\n";
	}
	str << "void main (void)\n"
		   "{\n"
		   "   gl_Position = a_position;\n"
		   "}\n";

	return str.str();
}

static std::string generateBasicFragmentSrc(glu::GLSLVersion glslVersion)
{
	std::stringstream str;

	str << glu::getGLSLVersionDeclaration(glslVersion) << "\n";
	str << "uniform highp vec4 u_color;\n"
		   "layout(location = 0) out mediump vec4 o_color;\n"
		   "void main (void)\n"
		   "{\n"
		   "   o_color = u_color;\n"
		   "}\n";

	return str.str();
}

// Testcase for glCreateShaderProgramv
class CreateShadProgCase : public TestCase
{
public:
	CreateShadProgCase(Context& context, const char* name, const char* description, glu::GLSLVersion glslVersion)
		: TestCase(context, name, description), m_glslVersion(glslVersion)
	{
	}

	~CreateShadProgCase(void)
	{
	}

	// Check program validity created with CreateShaderProgram
	bool checkCSProg(const glw::Functions& gl, GLuint program, int expectedSep = GL_TRUE, int expectedLink = GL_TRUE)
	{
		int separable = GL_FALSE;
		int linked	= GL_FALSE;
		if (program != 0)
		{
			gl.getProgramiv(program, GL_PROGRAM_SEPARABLE, &separable);
			gl.getProgramiv(program, GL_LINK_STATUS, &linked);
		}

		return (program != 0) && (separable == expectedSep) && (linked == expectedLink);
	}

	IterateResult iterate(void)
	{
		TestLog&			  log = m_testCtx.getLog();
		const glw::Functions& gl  = m_context.getRenderContext().getFunctions();
		int					  i;
		const char*			  srcStrings[10];
		glw::GLuint			  program;
		glw::GLenum			  err;

		// CreateShaderProgramv verification
		log << TestLog::Message << "Begin:CreateShadProgCase iterate" << TestLog::EndMessage;

		// vertex shader
		i				= 0;
		srcStrings[i++] = glu::getGLSLVersionDeclaration(m_glslVersion);
		srcStrings[i++] = "\n";
		if (m_glslVersion >= glu::GLSL_VERSION_410)
		{
			srcStrings[i++] = "out gl_PerVertex {\n"
							  "  vec4 gl_Position;\n"
							  "};\n";
		}
		srcStrings[i++] = "in vec4 a_position;\n";
		srcStrings[i++] = "void main ()\n";
		srcStrings[i++] = "{\n";
		srcStrings[i++] = "    gl_Position = a_position;\n";
		srcStrings[i++] = "}\n";

		program = gl.createShaderProgramv(GL_VERTEX_SHADER, i, srcStrings);
		if (!checkCSProg(gl, program))
		{
			TCU_FAIL("CreateShaderProgramv failed for vertex shader");
		}

		gl.deleteProgram(program);

		// Half as many strings
		i				= 0;
		srcStrings[i++] = glu::getGLSLVersionDeclaration(m_glslVersion);
		srcStrings[i++] = "\n";
		if (m_glslVersion >= glu::GLSL_VERSION_410)
		{
			srcStrings[i++] = "out gl_PerVertex {\n"
							  "  vec4 gl_Position;\n"
							  "};\n";
		}
		srcStrings[i++] = "in vec4 a_position;\n"
						  "void main ()\n";
		srcStrings[i++] = "{\n"
						  "    gl_Position = a_position;\n";
		srcStrings[i++] = "}\n";

		program = gl.createShaderProgramv(GL_VERTEX_SHADER, i, srcStrings);
		if (!checkCSProg(gl, program))
		{
			TCU_FAIL("CreateShaderProgramv failed for vertex shader");
		}

		gl.deleteProgram(program);

		// Fragment shader
		i				= 0;
		srcStrings[i++] = glu::getGLSLVersionDeclaration(m_glslVersion);
		srcStrings[i++] = "\nin highp vec4 u_color;\n";
		srcStrings[i++] = "layout(location = 0) out mediump vec4 o_color;\n";
		srcStrings[i++] = "void main ()\n";
		srcStrings[i++] = "{\n";
		srcStrings[i++] = "    o_color = u_color;\n";
		srcStrings[i++] = "}\n";

		program = gl.createShaderProgramv(GL_FRAGMENT_SHADER, i, srcStrings);
		if (!checkCSProg(gl, program))
		{
			TCU_FAIL("CreateShaderProgramv failed for fragment shader");
		}

		gl.deleteProgram(program);

		GLU_EXPECT_NO_ERROR(gl.getError(), "CreateShaderProgramv failed");

		// Negative Cases

		// invalid type
		program = gl.createShaderProgramv(GL_MAX_FRAGMENT_UNIFORM_VECTORS, i, srcStrings);
		err		= gl.getError();
		if ((program != 0) || (err != GL_INVALID_ENUM))
		{
			TCU_FAIL("CreateShaderProgramv failed");
		}

		// Negative count
		program = gl.createShaderProgramv(GL_FRAGMENT_SHADER, -1, srcStrings);
		err		= gl.getError();
		if ((program != 0) || (err != GL_INVALID_VALUE))
		{
			TCU_FAIL("CreateShaderProgramv failed");
		}

		// source compile error
		i				= 0;
		srcStrings[i++] = glu::getGLSLVersionDeclaration(m_glslVersion);
		srcStrings[i++] = "\nin highp vec4 u_color;\n";
		srcStrings[i++] = "layout(location = 0) out mediump vec4 o_color;\n";
		srcStrings[i++] = "void main ()\n";
		srcStrings[i++] = "{\n";
		srcStrings[i++] = "    o_color = u_color;\n";

		program = gl.createShaderProgramv(GL_FRAGMENT_SHADER, i, srcStrings);
		// expect valid program and false for link status
		if (!checkCSProg(gl, program, GL_FALSE, GL_FALSE))
		{
			TCU_FAIL("CreateShaderProgramv failed for fragment shader");
		}
		GLU_EXPECT_NO_ERROR(gl.getError(), "CreateShaderProgramv failed");
		gl.deleteProgram(program);

		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
		return STOP;
	}

private:
	glu::GLSLVersion m_glslVersion;
};

// Testcase for glUseProgamStages
class UseProgStagesCase : public TestCase
{
public:
	UseProgStagesCase(Context& context, const char* name, const char* description, glu::GLSLVersion glslVersion)
		: TestCase(context, name, description), m_glslVersion(glslVersion)
	{
	}

	~UseProgStagesCase(void)
	{
	}

	IterateResult iterate(void)
	{
		TestLog&			  log = m_testCtx.getLog();
		const glw::Functions& gl  = m_context.getRenderContext().getFunctions();
		glw::GLenum			  err;
		glw::GLuint			  pipeline;
		glw::GLuint			  progIdV, progIdF;
		glw::GLuint			  programVtx, programFrag;
		const char*			  shaderSrc[1];
		std::string			  vtx;
		std::string			  frag;
		glw::GLint			  linkStatus;

		vtx  = generateBasicVertexSrc(m_glslVersion);
		frag = generateBasicFragmentSrc(m_glslVersion);

		// UseProgramStages verification
		log << TestLog::Message << "Begin:UseProgStagesCase iterate" << TestLog::EndMessage;

		gl.genProgramPipelines(1, &pipeline);
		gl.bindProgramPipeline(pipeline);

		// Use Vertex Shader
		shaderSrc[0] = vtx.c_str();
		programVtx   = gl.createShaderProgramv(GL_VERTEX_SHADER, 1, shaderSrc);

		gl.useProgramStages(pipeline, GL_VERTEX_SHADER_BIT, programVtx);
		gl.getProgramPipelineiv(pipeline, GL_VERTEX_SHADER, (glw::GLint*)&progIdV);
		gl.getProgramPipelineiv(pipeline, GL_FRAGMENT_SHADER, (glw::GLint*)&progIdF);
		if ((programVtx == 0) || (progIdV != programVtx) || (progIdF != 0))
		{
			TCU_FAIL("UseProgramStages failed");
		}
		GLU_EXPECT_NO_ERROR(gl.getError(), "UseProgramStages failed");

		// Use Fragment Shader
		shaderSrc[0] = frag.c_str();
		programFrag  = gl.createShaderProgramv(GL_FRAGMENT_SHADER, 1, shaderSrc);

		gl.useProgramStages(pipeline, GL_FRAGMENT_SHADER_BIT, programFrag);
		gl.getProgramPipelineiv(pipeline, GL_FRAGMENT_SHADER, (glw::GLint*)&progIdF);
		if ((programFrag == 0) || (progIdF != programFrag) || (progIdF == progIdV))
		{
			TCU_FAIL("UseProgramStages failed");
		}

		// Reset stages
		gl.useProgramStages(pipeline, GL_VERTEX_SHADER_BIT | GL_FRAGMENT_SHADER_BIT, 0);
		gl.getProgramPipelineiv(pipeline, GL_VERTEX_SHADER, (glw::GLint*)&progIdV);
		gl.getProgramPipelineiv(pipeline, GL_FRAGMENT_SHADER, (glw::GLint*)&progIdF);
		if ((progIdV != 0) || (progIdF != 0))
		{
			TCU_FAIL("UseProgramStages failed");
		}

		// One program for both.
		glu::ShaderProgram progVF(m_context.getRenderContext(), glu::makeVtxFragSources(vtx.c_str(), frag.c_str()));

		// Make separable and relink
		gl.programParameteri(progVF.getProgram(), GL_PROGRAM_SEPARABLE, GL_TRUE);
		gl.linkProgram(progVF.getProgram());
		gl.getProgramiv(progVF.getProgram(), GL_LINK_STATUS, &linkStatus);
		if (linkStatus != 1)
		{
			TCU_FAIL("UseProgramStages failed");
		}
		GLU_EXPECT_NO_ERROR(gl.getError(), "UseProgramStages failed");

		gl.useProgramStages(pipeline, GL_VERTEX_SHADER_BIT | GL_FRAGMENT_SHADER_BIT, progVF.getProgram());
		gl.getProgramPipelineiv(pipeline, GL_VERTEX_SHADER, (glw::GLint*)&progIdV);
		gl.getProgramPipelineiv(pipeline, GL_FRAGMENT_SHADER, (glw::GLint*)&progIdF);
		if ((progIdV != progVF.getProgram()) || (progIdV != progIdF))
		{
			TCU_FAIL("UseProgramStages failed");
		}
		GLU_EXPECT_NO_ERROR(gl.getError(), "UseProgramStages failed");

		// Use a fragment program with vertex bit
		gl.useProgramStages(pipeline, GL_VERTEX_SHADER_BIT | GL_FRAGMENT_SHADER_BIT, 0);
		gl.useProgramStages(pipeline, GL_VERTEX_SHADER_BIT, programFrag);
		GLU_EXPECT_NO_ERROR(gl.getError(), "UseProgramStages failed");

		// Unbound pipeline
		gl.bindProgramPipeline(0);
		gl.deleteProgramPipelines(1, &pipeline);
		pipeline = 0;
		gl.genProgramPipelines(1, &pipeline);
		gl.useProgramStages(pipeline, GL_VERTEX_SHADER_BIT, programVtx);
		gl.useProgramStages(pipeline, GL_FRAGMENT_SHADER_BIT, programFrag);
		gl.getProgramPipelineiv(pipeline, GL_VERTEX_SHADER, (glw::GLint*)&progIdV);
		gl.getProgramPipelineiv(pipeline, GL_FRAGMENT_SHADER, (glw::GLint*)&progIdF);
		if ((progIdV != programVtx) || (progIdF != programFrag))
		{
			TCU_FAIL("UseProgramStages failed");
		}
		GLU_EXPECT_NO_ERROR(gl.getError(), "UseProgramStages failed");

		// Negative Cases

		// Invalid stages
		gl.useProgramStages(pipeline, GL_ALL_SHADER_BITS ^ (GL_VERTEX_SHADER_BIT | GL_FRAGMENT_SHADER_BIT), programVtx);
		err = gl.getError();
		if (err != GL_INVALID_VALUE)
		{
			TCU_FAIL("UseProgramStages failed");
		}

		// Program that is not separable
		gl.programParameteri(progVF.getProgram(), GL_PROGRAM_SEPARABLE, GL_FALSE);
		gl.linkProgram(progVF.getProgram());
		gl.getProgramiv(progVF.getProgram(), GL_LINK_STATUS, &linkStatus);
		if (linkStatus != 1)
		{
			TCU_FAIL("UseProgramStages failed");
		}
		GLU_EXPECT_NO_ERROR(gl.getError(), "UseProgramStages failed");
		gl.useProgramStages(pipeline, GL_VERTEX_SHADER_BIT | GL_FRAGMENT_SHADER_BIT, progVF.getProgram());
		err = gl.getError();
		if (err != GL_INVALID_OPERATION)
		{
			TCU_FAIL("UseProgramStages failed");
		}

		// Program that is not successfully linked
		// remove the main keyword
		std::string  fragNoMain = frag;
		unsigned int pos		= (unsigned int)fragNoMain.find("main");
		fragNoMain.replace(pos, 4, "niaM");
		glu::ShaderProgram progNoLink(m_context.getRenderContext(),
									  glu::makeVtxFragSources(vtx.c_str(), fragNoMain.c_str()));

		gl.programParameteri(progNoLink.getProgram(), GL_PROGRAM_SEPARABLE, GL_TRUE);
		gl.linkProgram(progNoLink.getProgram());
		gl.useProgramStages(pipeline, GL_VERTEX_SHADER_BIT | GL_FRAGMENT_SHADER_BIT, progNoLink.getProgram());
		err = gl.getError();
		if (err != GL_INVALID_OPERATION)
		{
			TCU_FAIL("UseProgramStages failed");
		}

		// Invalid pipeline
		gl.useProgramStages(pipeline + 1000, GL_VERTEX_SHADER_BIT, programVtx);
		err = gl.getError();
		if (err != GL_INVALID_OPERATION)
		{
			TCU_FAIL("UseProgramStages failed");
		}

		// Invalid pipeline
		gl.deleteProgramPipelines(1, &pipeline);
		gl.useProgramStages(pipeline, GL_VERTEX_SHADER_BIT, programVtx);
		err = gl.getError();
		if (err != GL_INVALID_OPERATION)
		{
			TCU_FAIL("UseProgramStages failed");
		}

		gl.deleteProgram(programVtx);
		gl.deleteProgram(programFrag);

		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
		return STOP;
	}

private:
	glu::GLSLVersion m_glslVersion;
};

// Testcase for pipeline api
class PipelineApiCase : public TestCase
{
public:
	PipelineApiCase(Context& context, const char* name, const char* description, glu::GLSLVersion glslVersion)
		: TestCase(context, name, description), m_glslVersion(glslVersion)
	{
	}

	~PipelineApiCase(void)
	{
	}

	// Validate glGetProgramPipelineInfoLog
	void checkProgInfoLog(const glw::Functions& gl, GLuint pipeline)
	{
		glw::GLint   value;
		glw::GLsizei bufSize;
		glw::GLsizei length;
		glw::GLenum  err;

		gl.getProgramPipelineiv(pipeline, GL_INFO_LOG_LENGTH, &value);
		std::vector<char> infoLogBuf(value + 1);

		bufSize = 0;
		gl.getProgramPipelineInfoLog(pipeline, bufSize, &length, &infoLogBuf[0]);
		GLU_EXPECT_NO_ERROR(gl.getError(), "GetProgramPipelineInfoLog failed");

		bufSize = value / 2; // read half the log
		gl.getProgramPipelineInfoLog(pipeline, bufSize, &length, &infoLogBuf[0]);
		if ((bufSize != 0) && (bufSize != length + 1))
		{
			TCU_FAIL("GetProgramPipelineInfoLog failed");
		}
		bufSize = value;
		gl.getProgramPipelineInfoLog(pipeline, bufSize, &length, &infoLogBuf[0]);
		if ((bufSize != 0) && (bufSize != length + 1))
		{
			TCU_FAIL("GetProgramPipelineInfoLog failed");
		}

		// Negative case for GetProgramPipelineInfoLog

		gl.getProgramPipelineInfoLog(pipeline + 101, bufSize, &length, &infoLogBuf[0]);
		err = gl.getError();
		if (err != GL_INVALID_VALUE)
		{
			TCU_FAIL("GetProgramPipelineInfoLog failed");
		}
	}

	IterateResult iterate(void)
	{
		TestLog&			  log = m_testCtx.getLog();
		const glw::Functions& gl  = m_context.getRenderContext().getFunctions();
		glw::GLenum			  err;
		const int			  maxpipelines = 10;
		glw::GLuint			  pipelines[maxpipelines];
		std::string			  vtx;
		std::string			  frag;
		glw::GLint			  linkStatus;
		glw::GLuint			  value;

		vtx  = generateBasicVertexSrc(m_glslVersion);
		frag = generateBasicFragmentSrc(m_glslVersion);

		// Pipeline API verification
		log << TestLog::Message << "Begin:PipelineApiCase iterate" << TestLog::EndMessage;

		glu::ShaderProgram progVF(m_context.getRenderContext(), glu::makeVtxFragSources(vtx.c_str(), frag.c_str()));

		// Make separable and relink
		gl.programParameteri(progVF.getProgram(), GL_PROGRAM_SEPARABLE, GL_TRUE);
		gl.linkProgram(progVF.getProgram());
		gl.getProgramiv(progVF.getProgram(), GL_LINK_STATUS, &linkStatus);
		if (linkStatus != 1)
		{
			TCU_FAIL("LinkProgram failed");
		}

		gl.genProgramPipelines(1, pipelines);

		// ActiveShaderProgram
		gl.activeShaderProgram(pipelines[0], progVF.getProgram());
		GLU_EXPECT_NO_ERROR(gl.getError(), "ActiveShaderProgram failed");

		// Negative cases for ActiveShaderProgram

		// Nonexistent program
		gl.activeShaderProgram(pipelines[0], progVF.getProgram() + 100);
		err = gl.getError();
		if (err != GL_INVALID_VALUE)
		{
			TCU_FAIL("ActiveShaderProgram failed");
		}
		gl.getProgramPipelineiv(pipelines[0], GL_ACTIVE_PROGRAM, (glw::GLint*)&value);
		if (value != progVF.getProgram())
		{
			TCU_FAIL("ActiveShaderProgram failed");
		}

		// Deleted pipeline
		gl.deleteProgramPipelines(1, pipelines);
		gl.activeShaderProgram(pipelines[0], progVF.getProgram());
		err = gl.getError();
		if (err != GL_INVALID_OPERATION)
		{
			TCU_FAIL("ActiveShaderProgram failed");
		}

		// GenProgramPipeline

		gl.genProgramPipelines(2, &pipelines[0]);
		gl.genProgramPipelines(3, &pipelines[2]);
		gl.genProgramPipelines(5, &pipelines[5]);

		for (int i = 0; i < maxpipelines; i++)
		{
			gl.bindProgramPipeline(pipelines[i]); // has to be bound to be recognized
			if (!gl.isProgramPipeline(pipelines[i]))
			{
				TCU_FAIL("GenProgramPipelines failed");
			}
		}
		gl.deleteProgramPipelines(maxpipelines, pipelines);

		// BindProgramPipeline

		gl.genProgramPipelines(2, pipelines);
		gl.bindProgramPipeline(pipelines[0]);
		gl.getIntegerv(GL_PROGRAM_PIPELINE_BINDING, (glw::GLint*)&value);
		if (value != pipelines[0])
		{
			TCU_FAIL("BindProgramPipeline failed");
		}
		gl.bindProgramPipeline(pipelines[1]);
		gl.getIntegerv(GL_PROGRAM_PIPELINE_BINDING, (glw::GLint*)&value);
		if (value != pipelines[1])
		{
			TCU_FAIL("BindProgramPipeline failed");
		}
		GLU_EXPECT_NO_ERROR(gl.getError(), "BindProgramPipeline failed");

		// Negative Case for BindProgramPipeline

		gl.bindProgramPipeline(pipelines[2]); // deleted pipeline
		gl.getIntegerv(GL_PROGRAM_PIPELINE_BINDING, (glw::GLint*)&value);
		err = gl.getError();
		if ((err != GL_INVALID_OPERATION) || (value != pipelines[1]))
		{
			TCU_FAIL("BindProgramPipeline failed");
		}

		// DeleteProgramPipelines

		gl.genProgramPipelines(8, &pipelines[2]); // back to 10 total
		gl.deleteProgramPipelines(2, &pipelines[8]);
		gl.deleteProgramPipelines(3, &pipelines[5]);
		pipelines[9] = 0;
		gl.deleteProgramPipelines(maxpipelines, pipelines); // 5 good, 4 deleted, 1 zero
		gl.deleteProgramPipelines(0, pipelines);
		GLU_EXPECT_NO_ERROR(gl.getError(), "DeleteProgramPipelines failed");
		for (int i = 0; i < maxpipelines; i++)
		{
			if (gl.isProgramPipeline(pipelines[i]))
			{
				TCU_FAIL("DeleteProgramPipelines failed");
			}
		}
		gl.getIntegerv(GL_PROGRAM_PIPELINE_BINDING, (glw::GLint*)&value);
		if (value != 0)
		{
			TCU_FAIL("DeleteProgramPipelines failed");
		}

		// IsProgramPipeline

		pipelines[1] = 0x1000;
		pipelines[2] += 100;
		for (int i = 0; i < 3; i++)
		{
			// 1 deleted and 2 bogus values
			if (gl.isProgramPipeline(pipelines[i]))
			{
				TCU_FAIL("IsProgramPipeline failed");
			}
		}
		gl.genProgramPipelines(1, pipelines);
		if (gl.isProgramPipeline(pipelines[0]))
		{
			TCU_FAIL("IsProgramPipeline failed");
		}
		gl.deleteProgramPipelines(1, pipelines);
		GLU_EXPECT_NO_ERROR(gl.getError(), "IsProgramPipeline failed");

		// ProgramParameteri PROGRAM_SEPARABLE
		// NOTE: The query for PROGRAM_SEPARABLE must query latched
		//       state. In other words, the state of the binary after
		//       it was linked. So in the tests below, the queries
		//       should return the default state GL_FALSE since the
		//       program has no linked binary.

		glw::GLuint programSep = gl.createProgram();
		int			separable;
		gl.programParameteri(programSep, GL_PROGRAM_SEPARABLE, GL_TRUE);
		gl.getProgramiv(programSep, GL_PROGRAM_SEPARABLE, &separable);
		if (separable != GL_FALSE)
		{
			TCU_FAIL("programParameteri PROGRAM_SEPARABLE failed");
		}
		gl.programParameteri(programSep, GL_PROGRAM_SEPARABLE, GL_FALSE);
		gl.getProgramiv(programSep, GL_PROGRAM_SEPARABLE, &separable);
		if (separable != 0)
		{
			TCU_FAIL("programParameteri PROGRAM_SEPARABLE failed");
		}

		// Negative Case for ProgramParameteri PROGRAM_SEPARABLE

		gl.deleteProgram(programSep);
		gl.programParameteri(programSep, GL_PROGRAM_SEPARABLE, GL_TRUE);
		err = gl.getError();
		if (err != GL_INVALID_VALUE)
		{
			TCU_FAIL("programParameteri PROGRAM_SEPARABLE failed");
		}
		gl.programParameteri(progVF.getProgram(), GL_PROGRAM_SEPARABLE, 501);
		err = gl.getError();
		if (err != GL_INVALID_VALUE)
		{
			TCU_FAIL("programParameteri PROGRAM_SEPARABLE failed");
		}

		// GetProgramPipelineiv

		gl.genProgramPipelines(1, pipelines);
		gl.getProgramPipelineiv(pipelines[0], GL_ACTIVE_PROGRAM, (glw::GLint*)&value);
		if (value != 0)
		{
			TCU_FAIL("GetProgramPipelineiv failed for ACTIVE_PROGRAM");
		}
		gl.getProgramPipelineiv(pipelines[0], GL_VERTEX_SHADER, (glw::GLint*)&value);
		if (value != 0)
		{
			TCU_FAIL("GetProgramPipelineiv failed for VERTEX_SHADER");
		}
		gl.getProgramPipelineiv(pipelines[0], GL_FRAGMENT_SHADER, (glw::GLint*)&value);
		if (value != 0)
		{
			TCU_FAIL("GetProgramPipelineiv failed for FRAGMENT_SHADER");
		}
		gl.getProgramPipelineiv(pipelines[0], GL_VALIDATE_STATUS, (glw::GLint*)&value);
		if (value != 0)
		{
			TCU_FAIL("GetProgramPipelineiv failed for VALIDATE_STATUS");
		}
		gl.getProgramPipelineiv(pipelines[0], GL_INFO_LOG_LENGTH, (glw::GLint*)&value);
		if (value != 0)
		{
			TCU_FAIL("GetProgramPipelineiv failed for INFO_LOG_LENGTH");
		}
		GLU_EXPECT_NO_ERROR(gl.getError(), "GetProgramPipelineiv failed");

		// Negative Case for GetProgramPipelineiv

		gl.deleteProgramPipelines(1, pipelines);
		gl.getProgramPipelineiv(pipelines[0], GL_ACTIVE_PROGRAM, (glw::GLint*)&value);
		err = gl.getError();
		if (err != GL_INVALID_OPERATION)
		{
			TCU_FAIL("GetProgramPipelineiv failed for ACTIVE_PROGRAM");
		}

		// ValidateProgramPipeline

		gl.genProgramPipelines(1, pipelines); // Unvalidated
		gl.getProgramPipelineiv(pipelines[0], GL_VALIDATE_STATUS, (glw::GLint*)&value);
		if (value != 0)
		{
			TCU_FAIL("ValidateProgramPipeline failed");
		}

		gl.validateProgramPipeline(pipelines[0]); // Not bound yet
		gl.getProgramPipelineiv(pipelines[0], GL_VALIDATE_STATUS, (glw::GLint*)&value);
		if (value != 0)
		{
			TCU_FAIL("ValidateProgramPipeline failed");
		}

		gl.bindProgramPipeline(pipelines[0]);

		gl.validateProgramPipeline(pipelines[0]); // Still empty program pipeline.
		gl.getProgramPipelineiv(pipelines[0], GL_VALIDATE_STATUS, (glw::GLint*)&value);
		if (value != 0)
		{
			TCU_FAIL("ValidateProgramPipeline failed with empty program pipeline");
		}

		gl.useProgramStages(pipelines[0], GL_VERTEX_SHADER_BIT | GL_FRAGMENT_SHADER_BIT, progVF.getProgram());
		gl.validateProgramPipeline(pipelines[0]);
		gl.getProgramPipelineiv(pipelines[0], GL_VALIDATE_STATUS, (glw::GLint*)&value);
		if (value != 1)
		{
			TCU_FAIL("ValidateProgramPipeline failed");
		}

		// GetProgramPipelineInfoLog
		checkProgInfoLog(gl, pipelines[0]);

		// ValidateProgramPipeline additional
		// Relink the bound separable program as not separable
		gl.programParameteri(progVF.getProgram(), GL_PROGRAM_SEPARABLE, GL_FALSE);
		gl.linkProgram(progVF.getProgram());
		err = gl.getError();
		gl.validateProgramPipeline(pipelines[0]);
		gl.getProgramPipelineiv(pipelines[0], GL_VALIDATE_STATUS, (glw::GLint*)&value);
		if (value != 0)
		{
			TCU_FAIL("ValidateProgramPipeline failed");
		}
		GLU_EXPECT_NO_ERROR(gl.getError(), "ValidateProgramPipeline failed");

		// GetProgramPipelineInfoLog
		checkProgInfoLog(gl, pipelines[0]);

		// Negative Case for ValidateProgramPipeline

		gl.deleteProgramPipelines(1, pipelines);
		gl.validateProgramPipeline(pipelines[0]);
		err = gl.getError();
		if (err != GL_INVALID_OPERATION)
		{
			TCU_FAIL("ValidateProgramPipeline failed");
		}

		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
		return STOP;
	}

private:
	glu::GLSLVersion m_glslVersion;
};

// Testcase for glProgramUniform
class ProgramUniformCase : public TestCase
{
public:
	ProgramUniformCase(Context& context, const char* name, const char* description, glu::GLSLVersion glslVersion)
		: TestCase(context, name, description), m_glslVersion(glslVersion)
	{
	}

	~ProgramUniformCase(void)
	{
	}

	bool isDataTypeSquareMatrix(glu::DataType dtyp)
	{
		return (dtyp == glu::TYPE_FLOAT_MAT2) || (dtyp == glu::TYPE_FLOAT_MAT3) || (dtyp == glu::TYPE_FLOAT_MAT4);
	}

	// outFragSrc will hold a fragment program that is DataType specific
	void generateUniformFragSrc(std::string& outFragSrc, glu::GLSLVersion glslVersion, glu::DataType dType)
	{
		std::ostringstream fragSrc;

		fragSrc << glu::getGLSLVersionDeclaration(glslVersion) << "\n";
		if (isDataTypeMatrix(dType) && isDataTypeSquareMatrix(dType))
		{
			fragSrc << s_unifFragSquareMatShaderSrc;
		}
		else if (isDataTypeMatrix(dType) && !isDataTypeSquareMatrix(dType))
		{
			fragSrc << s_unifFragNonSquareMatShaderSrc;
		}
		else
		{
			fragSrc << s_unifFragShaderSrc;
		}

		std::map<std::string, std::string> params;

		if (dType == glu::TYPE_INT)
		{
			params.insert(std::pair<std::string, std::string>("SCALAR_TYPE", "int"));
			params.insert(std::pair<std::string, std::string>("VECTOR_TYPE", "ivec"));
			params.insert(std::pair<std::string, std::string>("SFX", ""));
		}
		else if (dType == glu::TYPE_UINT)
		{
			params.insert(std::pair<std::string, std::string>("SCALAR_TYPE", "uint"));
			params.insert(std::pair<std::string, std::string>("VECTOR_TYPE", "uvec"));
			params.insert(std::pair<std::string, std::string>("SFX", "u"));
		}
		else if (dType == glu::TYPE_FLOAT)
		{
			params.insert(std::pair<std::string, std::string>("SCALAR_TYPE", "float"));
			params.insert(std::pair<std::string, std::string>("VECTOR_TYPE", "vec"));
			params.insert(std::pair<std::string, std::string>("SFX", ".0"));
		}

		tcu::StringTemplate fragTmpl(fragSrc.str().c_str());
		outFragSrc = fragTmpl.specialize(params);
	}

	// Set the integer programUniforms
	void progUniformi(const glw::Functions& gl, glw::GLuint prog, int arraySize, int* location, int* value)
	{
		gl.programUniform1i(prog, location[0], value[0]);
		value += 1;
		gl.programUniform2i(prog, location[1], value[0], value[1]);
		value += 2;
		gl.programUniform3i(prog, location[2], value[0], value[1], value[2]);
		value += 3;
		gl.programUniform4i(prog, location[3], value[0], value[1], value[2], value[3]);
		value += 4;

		gl.programUniform1iv(prog, location[4], arraySize, value);
		value += 1 * arraySize;
		gl.programUniform2iv(prog, location[6], arraySize, value);
		value += 2 * arraySize;
		gl.programUniform3iv(prog, location[8], arraySize, value);
		value += 3 * arraySize;
		gl.programUniform4iv(prog, location[10], arraySize, value);
	}

	// Set the unsigned integer programUniforms
	void progUniformui(const glw::Functions& gl, glw::GLuint prog, int arraySize, int* location, unsigned int* value)
	{
		gl.programUniform1ui(prog, location[0], value[0]);
		value += 1;
		gl.programUniform2ui(prog, location[1], value[0], value[1]);
		value += 2;
		gl.programUniform3ui(prog, location[2], value[0], value[1], value[2]);
		value += 3;
		gl.programUniform4ui(prog, location[3], value[0], value[1], value[2], value[3]);
		value += 4;

		gl.programUniform1uiv(prog, location[4], arraySize, value);
		value += 1 * arraySize;
		gl.programUniform2uiv(prog, location[6], arraySize, value);
		value += 2 * arraySize;
		gl.programUniform3uiv(prog, location[8], arraySize, value);
		value += 3 * arraySize;
		gl.programUniform4uiv(prog, location[10], arraySize, value);
	}

	// Set the float programUniforms
	void progUniformf(const glw::Functions& gl, glw::GLuint prog, int arraySize, int* location, float* value)
	{
		gl.programUniform1f(prog, location[0], value[0]);
		value += 1;
		gl.programUniform2f(prog, location[1], value[0], value[1]);
		value += 2;
		gl.programUniform3f(prog, location[2], value[0], value[1], value[2]);
		value += 3;
		gl.programUniform4f(prog, location[3], value[0], value[1], value[2], value[3]);
		value += 4;

		gl.programUniform1fv(prog, location[4], arraySize, value);
		value += 1 * arraySize;
		gl.programUniform2fv(prog, location[6], arraySize, value);
		value += 2 * arraySize;
		gl.programUniform3fv(prog, location[8], arraySize, value);
		value += 3 * arraySize;
		gl.programUniform4fv(prog, location[10], arraySize, value);
	}

	// Set the integer uniforms with conventional glUniformi
	void activeUniformi(const glw::Functions& gl, int arraySize, int* location, int* value)
	{
		gl.uniform1i(location[0], value[0]);
		value += 1;
		gl.uniform2i(location[1], value[0], value[1]);
		value += 2;
		gl.uniform3i(location[2], value[0], value[1], value[2]);
		value += 3;
		gl.uniform4i(location[3], value[0], value[1], value[2], value[3]);
		value += 4;

		gl.uniform1iv(location[4], arraySize, value);
		value += 1 * arraySize;
		gl.uniform2iv(location[6], arraySize, value);
		value += 2 * arraySize;
		gl.uniform3iv(location[8], arraySize, value);
		value += 3 * arraySize;
		gl.uniform4iv(location[10], arraySize, value);
	}

	// Set the unsigned integer uniforms with conventional glUniformui
	void activeUniformui(const glw::Functions& gl, int arraySize, int* location, unsigned int* value)
	{
		gl.uniform1ui(location[0], value[0]);
		value += 1;
		gl.uniform2ui(location[1], value[0], value[1]);
		value += 2;
		gl.uniform3ui(location[2], value[0], value[1], value[2]);
		value += 3;
		gl.uniform4ui(location[3], value[0], value[1], value[2], value[3]);
		value += 4;

		gl.uniform1uiv(location[4], arraySize, value);
		value += 1 * arraySize;
		gl.uniform2uiv(location[6], arraySize, value);
		value += 2 * arraySize;
		gl.uniform3uiv(location[8], arraySize, value);
		value += 3 * arraySize;
		gl.uniform4uiv(location[10], arraySize, value);
	}

	// Set the float uniforms with conventional glUniformui
	void activeUniformf(const glw::Functions& gl, int arraySize, int* location, float* value)
	{
		gl.uniform1f(location[0], value[0]);
		value += 1;
		gl.uniform2f(location[1], value[0], value[1]);
		value += 2;
		gl.uniform3f(location[2], value[0], value[1], value[2]);
		value += 3;
		gl.uniform4f(location[3], value[0], value[1], value[2], value[3]);
		value += 4;

		gl.uniform1fv(location[4], arraySize, value);
		value += 1 * arraySize;
		gl.uniform2fv(location[6], arraySize, value);
		value += 2 * arraySize;
		gl.uniform3fv(location[8], arraySize, value);
		value += 3 * arraySize;
		gl.uniform4fv(location[10], arraySize, value);
	}

	// Call programUniform and verify for non-Matrix uniforms
	// Two programs are verified independently and against each other
	bool setAndCompareUniforms(glw::GLuint pipeline, glw::GLuint programA, glw::GLuint programB, glu::DataType dType,
							   int seed)
	{
		TestLog&			  log = m_testCtx.getLog();
		const glw::Functions& gl  = m_context.getRenderContext().getFunctions();
		// The fragment shader has defined uniforms of type:
		// scalar, vec2, vec3, vec4, and then length 2 arrays of
		// scalar, vec2, vec3, and vec4.
		// 4 uniforms in array form and 4 not in arrays.
		// We query a total of 12 uniform locations
		const int nonarrayUnifCount = 4;
		const int arrayUnifCount	= 4;
		const int arraySize			= 2;
		const int locationCount		= nonarrayUnifCount + arraySize * arrayUnifCount;
		// dwordCount represents the number of dwords to compare for each uniform location
		// scalar, vec2, vec3, vec4, scalar[0], scalar[1], vec2[0], vec2[1], etc.
		const int  dwordCount[locationCount] = { 1, 2, 3, 4, 1, 1, 2, 2, 3, 3, 4, 4 };
		glw::GLint locationA[locationCount];
		glw::GLint locationB[locationCount];
		// The total amount of data the uniforms take up: 1+2+3+4 + 2*(1+2+3+4)
		const int	udataCount = 30;
		unsigned int udata[udataCount]; //
		int*		 data  = (int*)&udata[0];
		float*		 fdata = (float*)&udata[0];
		int			 i, j, k;
		std::string  uniformBaseName("uVal");

		// ProgramUniform API verification
		log << TestLog::Message << "Begin:ProgramUniformCase iterate" << TestLog::EndMessage;

		// get uniform locations
		// scalar and vec uniforms
		for (i = 0; i < nonarrayUnifCount; i++)
		{
			string name  = uniformBaseName + de::toString(i);
			locationA[i] = gl.getUniformLocation(programA, name.c_str());
			locationB[i] = gl.getUniformLocation(programB, name.c_str());
		}
		// uniform arrays
		for (j = 0; j < arrayUnifCount; j++)
		{
			for (k = 0; k < arraySize; k++)
			{
				string name  = uniformBaseName + de::toString(nonarrayUnifCount + j) + "[" + de::toString(k) + "]";
				locationA[i] = gl.getUniformLocation(programA, name.c_str());
				locationB[i] = gl.getUniformLocation(programB, name.c_str());
				i++;
			}
		}

		// seed data buffer with unique values
		if (dType == glu::TYPE_FLOAT)
		{
			for (i = 0; i < udataCount; i++)
			{
				fdata[i] = (float)(seed + i);
			}
		}
		else
		{
			for (i = 0; i < udataCount; i++)
			{
				data[i] = seed + i;
			}
		}

		// set uniforms in program A
		if (dType == glu::TYPE_INT)
		{
			progUniformi(gl, programA, arraySize, locationA, data);
		}
		else if (dType == glu::TYPE_UINT)
		{
			progUniformui(gl, programA, arraySize, locationA, udata);
		}
		else if (dType == glu::TYPE_FLOAT)
		{
			progUniformf(gl, programA, arraySize, locationA, fdata);
		}

		// get and compare uniforms
		unsigned int* uValue = &udata[0];
		for (i = 0; i < nonarrayUnifCount + arraySize * arrayUnifCount; i++)
		{
			unsigned int retValA[4], retValB[4];

			if (dType == glu::TYPE_INT)
			{
				gl.getUniformiv(programA, locationA[i], (int*)&retValA[0]);
				gl.getUniformiv(programB, locationB[i], (int*)&retValB[0]);
			}
			else if (dType == glu::TYPE_UINT)
			{
				gl.getUniformuiv(programA, locationA[i], &retValA[0]);
				gl.getUniformuiv(programB, locationB[i], &retValB[0]);
			}
			else if (dType == glu::TYPE_FLOAT)
			{
				gl.getUniformfv(programA, locationA[i], (float*)&retValA[0]);
				gl.getUniformfv(programB, locationB[i], (float*)&retValB[0]);
			}

			for (j = 0; j < dwordCount[i]; j++)
			{
				// Compare programA uniform to expected value and
				// test to see if programB picked up the value.
				if ((retValA[j] != *uValue++) || (retValA[j] == retValB[j]))
				{
					TCU_FAIL("ProgramUniformi failed");
				}
			}
		}

		// reseed data buffer, continuing to increment
		if (dType == glu::TYPE_FLOAT)
		{
			fdata[0] = fdata[udataCount - 1] + 1.0f;
			for (i = 1; i < udataCount; i++)
			{
				fdata[i] = fdata[i - 1] + 1.0f;
			}
		}
		else
		{
			data[0] = data[udataCount - 1] + 1;
			for (i = 1; i < udataCount; i++)
			{
				data[i] = data[i - 1] + 1;
			}
		}

		// set uniforms in program B

		if (dType == glu::TYPE_INT)
		{
			progUniformi(gl, programB, arraySize, locationB, data);
		}
		else if (dType == glu::TYPE_UINT)
		{
			progUniformui(gl, programB, arraySize, locationB, udata);
		}
		else if (dType == glu::TYPE_FLOAT)
		{
			progUniformf(gl, programB, arraySize, locationB, fdata);
		}

		// get and compare uniforms
		uValue = &udata[0];
		for (i = 0; i < nonarrayUnifCount + arraySize * arrayUnifCount; i++)
		{
			unsigned int retValA[4], retValB[4];

			if (dType == glu::TYPE_INT)
			{
				gl.getUniformiv(programA, locationA[i], (int*)&retValA[0]);
				gl.getUniformiv(programB, locationB[i], (int*)&retValB[0]);
			}
			else if (dType == glu::TYPE_UINT)
			{
				gl.getUniformuiv(programA, locationA[i], &retValA[0]);
				gl.getUniformuiv(programB, locationB[i], &retValB[0]);
			}
			else if (dType == glu::TYPE_FLOAT)
			{
				gl.getUniformfv(programA, locationA[i], (float*)&retValA[0]);
				gl.getUniformfv(programB, locationB[i], (float*)&retValB[0]);
			}

			for (j = 0; j < dwordCount[i]; j++)
			{
				// Compare programB uniform to expected value and
				// test to see if programA picked up the value.
				if ((retValB[j] != *uValue++) || (retValA[j] == retValB[j]))
				{
					TCU_FAIL("ProgramUniformi failed");
				}
			}
		}

		// Test the conventional uniform interfaces on an ACTIVE_PROGRAM
		glw::GLuint activeProgram = 0;
		if (pipeline != 0)
		{
			gl.getProgramPipelineiv(pipeline, GL_ACTIVE_PROGRAM, (int*)&activeProgram);
		}
		if ((activeProgram != 0) && ((activeProgram == programA) || (activeProgram == programB)))
		{
			glw::GLint* location;

			location = (activeProgram == programA) ? locationA : locationB;

			// reseed data buffer, continuing to increment
			if (dType == glu::TYPE_FLOAT)
			{
				fdata[0] = fdata[udataCount - 1] + 1.0f;
				for (i = 1; i < udataCount; i++)
				{
					fdata[i] = fdata[i - 1] + 1.0f;
				}
			}
			else
			{
				data[0] = data[udataCount - 1] + 1;
				for (i = 1; i < udataCount; i++)
				{
					data[i] = data[i - 1] + 1;
				}
			}

			// set uniforms using original glUniform*

			if (dType == glu::TYPE_INT)
			{
				activeUniformi(gl, arraySize, location, data);
			}
			else if (dType == glu::TYPE_UINT)
			{
				activeUniformui(gl, arraySize, location, udata);
			}
			else if (dType == glu::TYPE_FLOAT)
			{
				activeUniformf(gl, arraySize, location, fdata);
			}

			// get and compare uniforms
			uValue = &udata[0];
			for (i = 0; i < nonarrayUnifCount + arraySize * arrayUnifCount; i++)
			{
				unsigned int retVal[4];

				if (dType == glu::TYPE_INT)
				{
					gl.getUniformiv(activeProgram, location[i], (int*)&retVal[0]);
				}
				else if (dType == glu::TYPE_UINT)
				{
					gl.getUniformuiv(activeProgram, location[i], &retVal[0]);
				}
				else if (dType == glu::TYPE_FLOAT)
				{
					gl.getUniformfv(activeProgram, location[i], (float*)&retVal[0]);
				}

				for (j = 0; j < dwordCount[i]; j++)
				{
					// Compare activeProgram uniform to expected value
					if ((retVal[j] != *uValue++))
					{
						TCU_FAIL("ActiveShaderProgram failed");
					}
				}
			}
		}

		return true;
	}

	// Call programUniform for Matrix uniforms
	// Two programs are verified independently and against each other
	bool setAndCompareMatrixUniforms(glw::GLuint pipeline, glw::GLuint programA, glw::GLuint programB,
									 glu::DataType dType, int seed)
	{
		TestLog&			  log		  = m_testCtx.getLog();
		const glw::Functions& gl		  = m_context.getRenderContext().getFunctions();
		bool				  isSquareMat = isDataTypeSquareMatrix(dType);
		// The matrix versions of the fragment shader have two element arrays
		// of each uniform.
		// There are 3 * 2 uniforms for the square matrix shader and
		// 6 * 2 uniforms in the non-square matrix shader.
		const int  maxUniforms = 12;
		int		   numUniforms;
		const int  arraySize = 2;
		glw::GLint locationA[maxUniforms];
		glw::GLint locationB[maxUniforms];
		// These arrays represent the number of floats for each uniform location
		// 2x2[0], 2x2[1], 3x3[0], 3x3[1], 4x4[0], 4x4[1]
		const int floatCountSqu[maxUniforms] = { 4, 4, 9, 9, 16, 16, 0, 0, 0, 0, 0, 0 };
		// 2x3[0], 2x3[1], 2x4[0], 2x4[1], 3x2[0], 3x2[1], 3x4[0], 3x4[1], 4x2[0]...
		const int  floatCountNonSqu[maxUniforms] = { 6, 6, 8, 8, 6, 6, 12, 12, 8, 8, 12, 12 };
		const int* floatCount;
		// Max data for the uniforms = 2*(2*3 + 3*2 + 2*4 + 4*2 + 3*4 + 4*3)
		const int   maxDataCount = 104;
		float		data[maxDataCount];
		int			i, j, k;
		std::string uniformBaseName("uValM");

		// ProgramUniform API verification
		log << TestLog::Message << "Begin:ProgramUniformCase for Matrix iterate" << TestLog::EndMessage;

		numUniforms = 0;
		// get uniform locations
		for (i = 2; i <= 4; i++) // matrix dimension m
		{
			for (j = 2; j <= 4; j++) // matrix dimension n
			{
				for (k = 0; k < arraySize; k++)
				{
					if ((i == j) && isSquareMat)
					{
						string name			   = uniformBaseName + de::toString(i) + "[" + de::toString(k) + "]";
						locationA[numUniforms] = gl.getUniformLocation(programA, name.c_str());
						locationB[numUniforms] = gl.getUniformLocation(programB, name.c_str());
						numUniforms++;
					}
					else if ((i != j) && !isSquareMat)
					{
						string name =
							uniformBaseName + de::toString(i) + "x" + de::toString(j) + "[" + de::toString(k) + "]";
						locationA[numUniforms] = gl.getUniformLocation(programA, name.c_str());
						locationB[numUniforms] = gl.getUniformLocation(programB, name.c_str());
						numUniforms++;
					}
				}
			}
		}
		DE_ASSERT((numUniforms == 6) || (numUniforms == 12));

		// init the float data array
		for (i = 0; i < maxDataCount; i++)
		{
			data[i] = (float)(seed + i);
		}

		// Set the uniforms in programA
		float* value = &data[0];
		if (isSquareMat)
		{
			floatCount = floatCountSqu;
			gl.programUniformMatrix2fv(programA, locationA[0], arraySize, GL_FALSE, value);
			value += 2 * 2 * arraySize;
			gl.programUniformMatrix3fv(programA, locationA[2], arraySize, GL_FALSE, value);
			value += 3 * 3 * arraySize;
			gl.programUniformMatrix4fv(programA, locationA[4], arraySize, GL_FALSE, value);
		}
		else
		{
			floatCount = floatCountNonSqu;
			gl.programUniformMatrix2x3fv(programA, locationA[0], arraySize, GL_FALSE, value);
			value += 2 * 3 * arraySize;
			gl.programUniformMatrix2x4fv(programA, locationA[2], arraySize, GL_FALSE, value);
			value += 2 * 4 * arraySize;
			gl.programUniformMatrix3x2fv(programA, locationA[4], arraySize, GL_FALSE, value);
			value += 3 * 2 * arraySize;
			gl.programUniformMatrix3x4fv(programA, locationA[6], arraySize, GL_FALSE, value);
			value += 3 * 4 * arraySize;
			gl.programUniformMatrix4x2fv(programA, locationA[8], arraySize, GL_FALSE, value);
			value += 4 * 2 * arraySize;
			gl.programUniformMatrix4x3fv(programA, locationA[10], arraySize, GL_FALSE, value);
		}

		// get and compare the uniform data
		value = &data[0];
		for (i = 0; i < numUniforms; i++)
		{
			float retValA[16], retValB[16];

			gl.getUniformfv(programA, locationA[i], retValA);
			gl.getUniformfv(programB, locationB[i], retValB);

			for (j = 0; j < floatCount[i]; j++)
			{
				// Compare programA uniform to expected value and
				// test to see if programB picked up the value.
				if ((retValA[j] != *value++) || (retValA[j] == retValB[j]))
				{
					TCU_FAIL("ProgramUniformi failed");
				}
			}
		}

		// reseed the float buffer
		data[0] = data[maxDataCount - 1];
		for (i = 1; i < maxDataCount; i++)
		{
			data[i] = data[i - 1] + 1.0f;
		}

		// set uniforms in program B
		value = &data[0];
		if (isSquareMat)
		{
			floatCount = floatCountSqu;
			gl.programUniformMatrix2fv(programB, locationB[0], arraySize, GL_FALSE, value);
			value += 2 * 2 * arraySize;
			gl.programUniformMatrix3fv(programB, locationB[2], arraySize, GL_FALSE, value);
			value += 3 * 3 * arraySize;
			gl.programUniformMatrix4fv(programB, locationB[4], arraySize, GL_FALSE, value);
		}
		else
		{
			floatCount = floatCountNonSqu;
			gl.programUniformMatrix2x3fv(programB, locationB[0], arraySize, GL_FALSE, value);
			value += 2 * 3 * arraySize;
			gl.programUniformMatrix2x4fv(programB, locationB[2], arraySize, GL_FALSE, value);
			value += 2 * 4 * arraySize;
			gl.programUniformMatrix3x2fv(programB, locationB[4], arraySize, GL_FALSE, value);
			value += 3 * 2 * arraySize;
			gl.programUniformMatrix3x4fv(programB, locationB[6], arraySize, GL_FALSE, value);
			value += 3 * 4 * arraySize;
			gl.programUniformMatrix4x2fv(programB, locationB[8], arraySize, GL_FALSE, value);
			value += 4 * 2 * arraySize;
			gl.programUniformMatrix4x3fv(programB, locationB[10], arraySize, GL_FALSE, value);
		}

		// get and compare the uniform data
		value = &data[0];
		for (i = 0; i < numUniforms; i++)
		{
			float retValA[16], retValB[16];

			gl.getUniformfv(programA, locationA[i], retValA);
			gl.getUniformfv(programB, locationB[i], retValB);

			for (j = 0; j < floatCount[i]; j++)
			{
				// Compare programB uniform to expected value and
				// test to see if programA picked up the value.
				if ((retValB[j] != *value++) || (retValA[j] == retValB[j]))
				{
					TCU_FAIL("ProgramUniformi failed");
				}
			}
		}

		// Use the conventional uniform interfaces on an ACTIVE_PROGRAM
		glw::GLuint activeProgram = 0;
		if (pipeline != 0)
		{
			gl.getProgramPipelineiv(pipeline, GL_ACTIVE_PROGRAM, (int*)&activeProgram);
		}
		if ((activeProgram != 0) && ((activeProgram == programA) || (activeProgram == programB)))
		{
			glw::GLint* location;

			location = (activeProgram == programA) ? locationA : locationB;

			// reseed the float buffer
			data[0] = data[maxDataCount - 1];
			for (i = 1; i < maxDataCount; i++)
			{
				data[i] = data[i - 1] + 1.0f;
			}

			// set uniforms with conventional uniform calls
			value = &data[0];
			if (isSquareMat)
			{
				floatCount = floatCountSqu;
				gl.uniformMatrix2fv(location[0], arraySize, GL_FALSE, value);
				value += 2 * 2 * arraySize;
				gl.uniformMatrix3fv(location[2], arraySize, GL_FALSE, value);
				value += 3 * 3 * arraySize;
				gl.uniformMatrix4fv(location[4], arraySize, GL_FALSE, value);
			}
			else
			{
				floatCount = floatCountNonSqu;
				gl.uniformMatrix2x3fv(location[0], arraySize, GL_FALSE, value);
				value += 2 * 3 * arraySize;
				gl.uniformMatrix2x4fv(location[2], arraySize, GL_FALSE, value);
				value += 2 * 4 * arraySize;
				gl.uniformMatrix3x2fv(location[4], arraySize, GL_FALSE, value);
				value += 3 * 2 * arraySize;
				gl.uniformMatrix3x4fv(location[6], arraySize, GL_FALSE, value);
				value += 3 * 4 * arraySize;
				gl.uniformMatrix4x2fv(location[8], arraySize, GL_FALSE, value);
				value += 4 * 2 * arraySize;
				gl.uniformMatrix4x3fv(location[10], arraySize, GL_FALSE, value);
			}

			// get and compare the uniform data
			value = &data[0];
			for (i = 0; i < numUniforms; i++)
			{
				float retVal[16];

				gl.getUniformfv(activeProgram, location[i], retVal);

				for (j = 0; j < floatCount[i]; j++)
				{
					// Compare activeshaderprogram uniform to expected value
					if (retVal[j] != *value++)
					{
						TCU_FAIL("ActiveShaderProgram with glUniform failed");
					}
				}
			}
		}

		return true;
	}

	IterateResult iterate(void)
	{
		const glw::Functions& gl	   = m_context.getRenderContext().getFunctions();
		glu::DataType		  dType[5] = { glu::TYPE_INT, glu::TYPE_UINT, glu::TYPE_FLOAT, glu::TYPE_FLOAT_MAT2,
								   glu::TYPE_FLOAT_MAT2X3 };

		// Loop over the various data types, generate fragment programs, and test uniforms
		// (MAT2 means stands for all square matrices, MAT2x3 stands for all non-square matrices)
		for (int i = 0; i < 5; i++)
		{
			glw::GLuint programA, programB;
			glw::GLuint pipeline = 0;
			const char* shaderSrc[1];
			std::string fragSrc;
			int			seed = 1000 + (1000 * i);

			generateUniformFragSrc(fragSrc, m_glslVersion, dType[i]);

			size_t			  length = fragSrc.size();
			std::vector<char> shaderbuf(length + 1);
			fragSrc.copy(&shaderbuf[0], length);
			shaderbuf[length] = '\0';
			shaderSrc[0]	  = &shaderbuf[0];
			programA		  = gl.createShaderProgramv(GL_FRAGMENT_SHADER, 1, shaderSrc);
			programB		  = gl.createShaderProgramv(GL_FRAGMENT_SHADER, 1, shaderSrc);

			if (isDataTypeMatrix(dType[i]))
			{
				// programs are unbound
				setAndCompareMatrixUniforms(pipeline, programA, programB, dType[i], seed);

				// bind one program with useProgramStages
				gl.genProgramPipelines(1, &pipeline);
				gl.bindProgramPipeline(pipeline);
				gl.useProgramStages(pipeline, GL_FRAGMENT_SHADER_BIT, programA);
				seed += 100;
				setAndCompareMatrixUniforms(pipeline, programA, programB, dType[i], seed);

				// make an active program with activeShaderProgram
				gl.activeShaderProgram(pipeline, programB);
				seed += 100;
				setAndCompareMatrixUniforms(pipeline, programA, programB, dType[i], seed);
			}
			else
			{
				// programs are unbound
				setAndCompareUniforms(pipeline, programA, programB, dType[i], seed);

				// bind one program with useProgramStages
				gl.genProgramPipelines(1, &pipeline);
				gl.bindProgramPipeline(pipeline);
				gl.useProgramStages(pipeline, GL_FRAGMENT_SHADER_BIT, programA);
				seed += 100;
				setAndCompareUniforms(pipeline, programA, programB, dType[i], seed);

				// make an active program with activeShaderProgram
				gl.activeShaderProgram(pipeline, programB);
				seed += 100;
				setAndCompareUniforms(pipeline, programA, programB, dType[i], seed);
			}

			gl.deleteProgram(programA);
			gl.deleteProgram(programB);
			gl.deleteProgramPipelines(1, &pipeline);
		}

		// Negative Cases

		// Program that is not successfully linked
		glw::GLenum err;
		std::string vtx;
		std::string frag;

		vtx  = generateBasicVertexSrc(m_glslVersion);
		frag = generateBasicFragmentSrc(m_glslVersion);

		// remove the main keyword so it doesn't link
		std::string  fragNoMain = frag;
		unsigned int pos		= (unsigned int)fragNoMain.find("main");
		fragNoMain.replace(pos, 4, "niaM");
		glu::ShaderProgram progNoLink(m_context.getRenderContext(),
									  glu::makeVtxFragSources(vtx.c_str(), fragNoMain.c_str()));
		gl.programParameteri(progNoLink.getProgram(), GL_PROGRAM_SEPARABLE, GL_TRUE);
		gl.linkProgram(progNoLink.getProgram());
		int unifLocation = gl.getUniformLocation(progNoLink.getProgram(), "u_color");
		gl.programUniform4f(progNoLink.getProgram(), unifLocation, 1.0, 1.0, 1.0, 1.0);
		err = gl.getError();
		if (err != GL_INVALID_OPERATION)
		{
			TCU_FAIL("ProgramUniformi failed");
		}

		// deleted program
		gl.deleteProgram(progNoLink.getProgram());
		gl.programUniform4f(progNoLink.getProgram(), unifLocation, 1.0, 1.0, 1.0, 1.0);
		err = gl.getError();
		if (err != GL_INVALID_VALUE)
		{
			TCU_FAIL("ProgramUniformi failed");
		}

		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
		return STOP;
	}

private:
	glu::GLSLVersion m_glslVersion;
};

// Testcase for state interactions
class StateInteractionCase : public TestCase
{
public:
	StateInteractionCase(Context& context, const char* name, const char* description, glu::GLSLVersion glslVersion)
		: TestCase(context, name, description), m_glslVersion(glslVersion)
	{
	}

	~StateInteractionCase(void)
	{
	}

	// Log the program info log
	void logProgramInfoLog(const glw::Functions& gl, glw::GLuint program)
	{
		TestLog&	 log	 = m_testCtx.getLog();
		glw::GLint   value   = 0;
		glw::GLsizei bufSize = 0;
		glw::GLsizei length  = 0;

		gl.getProgramiv(program, GL_INFO_LOG_LENGTH, &value);
		std::vector<char> infoLogBuf(value + 1);

		gl.getProgramInfoLog(program, bufSize, &length, &infoLogBuf[0]);
		GLU_EXPECT_NO_ERROR(gl.getError(), "GetProgramInfoLog failed");

		log << TestLog::Message << "Program Log:\n" << &infoLogBuf[0] << TestLog::EndMessage;
	}

	// Check program validity created with CreateShaderProgram
	bool checkCSProg(const glw::Functions& gl, GLuint program, int expectedLink = GL_TRUE)
	{
		int linked = GL_FALSE;
		if (program != 0)
		{
			gl.getProgramiv(program, GL_LINK_STATUS, &linked);

			if (expectedLink && !linked)
			{
				logProgramInfoLog(gl, program);
			}
		}

		return (program != 0) && (linked == expectedLink);
	}

	// Generate a vertex shader for variable input/output testing
	void generateVarLinkVertexShaderSrc(std::string& outVtxSrc, glu::GLSLVersion glslVersion, int numOutputs)
	{
		std::ostringstream vtxSrc;

		vtxSrc << glu::getGLSLVersionDeclaration(glslVersion) << "\n";
		if (glslVersion >= glu::GLSL_VERSION_410)
		{
			vtxSrc << "out gl_PerVertex {\n"
					  "  vec4 gl_Position;\n"
					  "};\n";
		}
		vtxSrc << "in highp vec4 a_position;\n";
		vtxSrc << "uniform highp vec4 u_color;\n";

		switch (numOutputs)
		{
		// Note all these cases fall through
		case 5:
			vtxSrc << "layout(location = 3) out vec4 o_val5;\n";
		case 4:
			vtxSrc << "flat out uvec4 val4;\n";
		case 3:
			vtxSrc << "flat out ivec2 val3;\n";
		case 2:
			vtxSrc << "out vec3 val2[2];\n";
		case 1:
			vtxSrc << "out vec4 val1;\n";
		default:
			vtxSrc << "out float val0;\n";
		}

		vtxSrc << "void main (void)\n";
		vtxSrc << "{\n";
		vtxSrc << "   gl_Position = a_position;\n";

		// The color uniform is passed in the last declared output variable
		switch (numOutputs)
		{
		case 5:
			vtxSrc << "    o_val5 = u_color;\n";
			break;
		case 4:
			vtxSrc << "    val4 = uvec4(u_color);\n";
			break;
		case 3:
			vtxSrc << "    val3 = ivec2(u_color);\n";
			break;
		case 2:
			vtxSrc << "    val2[0] = vec3(u_color);\n";
			break;
		case 1:
			vtxSrc << "    val1 = u_color;\n";
			break;
		default:
			vtxSrc << "    val0 = u_color.x;\n";
			break;
		}
		vtxSrc << "}\n";

		outVtxSrc = vtxSrc.str();
	}

	// Generate a fragment shader for variable input/output testing
	void generateVarLinkFragmentShaderSrc(std::string& outFragSrc, glu::GLSLVersion glslVersion, int numInputs)
	{
		std::ostringstream fragSrc;

		fragSrc << glu::getGLSLVersionDeclaration(glslVersion) << "\n";
		fragSrc << "precision highp float;\n";
		fragSrc << "precision highp int;\n";

		switch (numInputs)
		{
		// Note all these cases fall through
		case 5:
			fragSrc << "layout(location = 3) in vec4 i_val5;\n";
		case 4:
			fragSrc << "flat in uvec4 val4;\n";
		case 3:
			fragSrc << "flat in ivec2 val3;\n";
		case 2:
			fragSrc << "in vec3 val2[2];\n";
		case 1:
			fragSrc << "in vec4 val1;\n";
		default:
			fragSrc << "in float val0;\n";
		}

		fragSrc << "layout(location = 0) out mediump vec4 o_color;\n";
		fragSrc << "void main (void)\n";
		fragSrc << "{\n";

		switch (numInputs)
		{
		case 5:
			fragSrc << "    o_color = i_val5;\n";
			break;
		case 4:
			fragSrc << "    o_color = vec4(val4);\n";
			break;
		case 3:
			fragSrc << "    o_color = vec4(val3, 1.0, 1.0);\n";
			break;
		case 2:
			fragSrc << "    o_color = vec4(val2[0], 1.0);\n";
			break;
		case 1:
			fragSrc << "    o_color = vec4(val1);\n";
			break;
		default:
			fragSrc << "    o_color = vec4(val0, val0, val0, 1.0);\n";
			break;
		}

		fragSrc << "}\n";

		outFragSrc = fragSrc.str();
	}

	// Verify the surface is filled with the expected color
	bool checkSurface(tcu::Surface surface, tcu::RGBA expectedColor)
	{
		int numFailedPixels = 0;
		for (int y = 0; y < surface.getHeight(); y++)
		{
			for (int x = 0; x < surface.getWidth(); x++)
			{
				if (surface.getPixel(x, y) != expectedColor)
					numFailedPixels += 1;
			}
		}

		return (numFailedPixels == 0);
	}

	IterateResult iterate(void)
	{
		TestLog&				 log		  = m_testCtx.getLog();
		const glw::Functions&	gl			  = m_context.getRenderContext().getFunctions();
		const tcu::RenderTarget& renderTarget = m_context.getRenderContext().getRenderTarget();
		int						 viewportW	= de::min(16, renderTarget.getWidth());
		int						 viewportH	= de::min(16, renderTarget.getHeight());
		tcu::Surface			 renderedFrame(viewportW, viewportH);

		glw::GLuint programA, programB;
		glw::GLuint vao, vertexBuf, indexBuf;
		std::string vtx;
		std::string frag, frag2;
		glw::GLuint pipeline;
		const char* srcStrings[1];
		glw::GLenum err;

		log << TestLog::Message << "Begin:StateInteractionCase iterate" << TestLog::EndMessage;

		gl.viewport(0, 0, viewportW, viewportH);
		gl.clearColor(0.0f, 0.0f, 0.0f, 0.0f);
		gl.clear(GL_COLOR_BUFFER_BIT);

		// Check the precedence of glUseProgram over glBindProgramPipeline
		// The program bound with glUseProgram will draw green, the programs
		// bound with glBindProgramPipeline will render blue.
		vtx  = generateBasicVertexSrc(m_glslVersion);
		frag = generateBasicFragmentSrc(m_glslVersion);

		glu::ShaderProgram progVF(m_context.getRenderContext(), glu::makeVtxFragSources(vtx.c_str(), frag.c_str()));

		gl.useProgram(progVF.getProgram());
		// Ouput green in the fragment shader
		gl.uniform4f(gl.getUniformLocation(progVF.getProgram(), "u_color"), 0.0f, 1.0f, 0.0f, 1.0f);

		// Create and bind a pipeline with a different fragment shader
		gl.genProgramPipelines(1, &pipeline);
		// Use a different uniform name in another fragment shader
		frag2	  = frag;
		size_t pos = 0;
		while ((pos = frag2.find("u_color", pos)) != std::string::npos)
		{
			frag2.replace(pos, 7, "u_clrPB");
			pos += 7;
		}

		srcStrings[0] = vtx.c_str();
		programA	  = gl.createShaderProgramv(GL_VERTEX_SHADER, 1, srcStrings);
		if (!checkCSProg(gl, programA))
		{
			TCU_FAIL("CreateShaderProgramv failed for vertex shader");
		}
		srcStrings[0] = frag2.c_str();
		programB	  = gl.createShaderProgramv(GL_FRAGMENT_SHADER, 1, srcStrings);
		if (!checkCSProg(gl, programB))
		{
			TCU_FAIL("CreateShaderProgramv failed for fragment shader");
		}
		// Program B outputs blue.
		gl.programUniform4f(programB, gl.getUniformLocation(programB, "u_clrPB"), 0.0f, 0.0f, 1.0f, 1.0f);
		gl.useProgramStages(pipeline, GL_VERTEX_SHADER_BIT, programA);
		gl.useProgramStages(pipeline, GL_FRAGMENT_SHADER_BIT, programB);
		gl.bindProgramPipeline(pipeline);

		static const deUint16 quadIndices[] = { 0, 1, 2, 2, 1, 3 };
		const float			  position[]	= { -1.0f, -1.0f, +1.0f, 1.0f, -1.0f, +1.0f, 0.0f,  1.0f,
								   +1.0f, -1.0f, 0.0f,  1.0f, +1.0f, +1.0f, -1.0f, 1.0f };

		// Draw a quad with glu::draw
		glu::VertexArrayBinding posArray = glu::va::Float("a_position", 4, 4, 0, &position[0]);
		glu::draw(m_context.getRenderContext(), progVF.getProgram(), 1, &posArray,
				  glu::pr::Triangles(DE_LENGTH_OF_ARRAY(quadIndices), &quadIndices[0]));
		GLU_EXPECT_NO_ERROR(gl.getError(), "StateInteraction glu::draw failure");

		glu::readPixels(m_context.getRenderContext(), 0, 0, renderedFrame.getAccess());

		// useProgram takes precedence and the buffer should be green
		if (!checkSurface(renderedFrame, tcu::RGBA::green()))
		{
			TCU_FAIL("StateInteraction failed; surface should be green");
		}

		// The position attribute locations may be different.
		int posLoc = gl.getAttribLocation(progVF.getProgram(), "a_position");

		if (glu::isContextTypeES(m_context.getRenderContext().getType()))
			gl.disableVertexAttribArray(posLoc);

		/* Set up a vertex array object */
		gl.genVertexArrays(1, &vao);
		gl.bindVertexArray(vao);

		gl.genBuffers(1, &indexBuf);
		gl.bindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuf);
		gl.bufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(quadIndices), quadIndices, GL_STATIC_DRAW);

		gl.genBuffers(1, &vertexBuf);
		gl.bindBuffer(GL_ARRAY_BUFFER, vertexBuf);
		gl.bufferData(GL_ARRAY_BUFFER, sizeof(position), position, GL_STATIC_DRAW);

		posLoc = gl.getAttribLocation(programA, "a_position");
		gl.vertexAttribPointer(posLoc, 4, GL_FLOAT, GL_FALSE, 0, 0);
		gl.enableVertexAttribArray(posLoc);
		gl.bindBuffer(GL_ARRAY_BUFFER, 0);

		GLU_EXPECT_NO_ERROR(gl.getError(), "VAO setup failure");

		// bindProgramPipeline without a program installed by useProgram
		// Rerender the quad.  Don't use glu::draw because it takes the
		// program as a parameter and sets state.
		gl.useProgram(0);
		gl.bindProgramPipeline(pipeline);

		gl.drawElements(GL_TRIANGLES, DE_LENGTH_OF_ARRAY(quadIndices), GL_UNSIGNED_SHORT, NULL);
		GLU_EXPECT_NO_ERROR(gl.getError(), "DrawElements failure");

		glu::readPixels(m_context.getRenderContext(), 0, 0, renderedFrame.getAccess());

		// bindProgramPipeline will render blue
		if (!checkSurface(renderedFrame, tcu::RGBA::blue()))
		{
			TCU_FAIL("StateInteraction failed; surface should be blue");
		}

		// Test rendering with no program bound.  Rendering is undefined
		// but shouldn't produce an error.
		gl.useProgram(0);
		gl.bindProgramPipeline(0);

		gl.drawElements(GL_TRIANGLES, DE_LENGTH_OF_ARRAY(quadIndices), GL_UNSIGNED_SHORT, NULL);
		GLU_EXPECT_NO_ERROR(gl.getError(), "DrawElements failure");

		// Render call with missing pipeline stages should not generate an error
		gl.useProgramStages(pipeline, GL_VERTEX_SHADER_BIT, 0);
		gl.useProgramStages(pipeline, GL_FRAGMENT_SHADER_BIT, programB);
		gl.bindProgramPipeline(pipeline);

		gl.drawElements(GL_TRIANGLES, DE_LENGTH_OF_ARRAY(quadIndices), GL_UNSIGNED_SHORT, NULL);
		GLU_EXPECT_NO_ERROR(gl.getError(), "DrawElements failure");

		gl.useProgramStages(pipeline, GL_VERTEX_SHADER_BIT, programA);
		gl.useProgramStages(pipeline, GL_FRAGMENT_SHADER_BIT, 0);

		gl.drawElements(GL_TRIANGLES, DE_LENGTH_OF_ARRAY(quadIndices), GL_UNSIGNED_SHORT, NULL);
		GLU_EXPECT_NO_ERROR(gl.getError(), "DrawElements failure");

		// Missing program for fragment shader
		gl.useProgramStages(pipeline, GL_VERTEX_SHADER_BIT | GL_FRAGMENT_SHADER_BIT, programA);

		gl.drawElements(GL_TRIANGLES, DE_LENGTH_OF_ARRAY(quadIndices), GL_UNSIGNED_SHORT, NULL);
		GLU_EXPECT_NO_ERROR(gl.getError(), "DrawElements failure");

		// Separable program with both vertex and fragment shaders attached to only one stage

		gl.programParameteri(progVF.getProgram(), GL_PROGRAM_SEPARABLE, GL_TRUE);
		gl.linkProgram(progVF.getProgram());
		gl.useProgramStages(pipeline, GL_VERTEX_SHADER_BIT, progVF.getProgram());

		gl.drawElements(GL_TRIANGLES, DE_LENGTH_OF_ARRAY(quadIndices), GL_UNSIGNED_SHORT, NULL);
		err = gl.getError();
		if (err != GL_INVALID_OPERATION)
		{
			TCU_FAIL("DrawElements failed");
		}

		gl.validateProgramPipeline(pipeline);
		glw::GLint value;
		gl.getProgramPipelineiv(pipeline, GL_VALIDATE_STATUS, (glw::GLint*)&value);
		if (value != 0)
		{
			TCU_FAIL("Program pipeline validation failed");
		}

		// attached to just the fragment shader
		// Call validateProgramPipeline before rendering this time
		gl.useProgramStages(pipeline, GL_VERTEX_SHADER_BIT, 0);
		gl.useProgramStages(pipeline, GL_FRAGMENT_SHADER_BIT, progVF.getProgram());

		gl.validateProgramPipeline(pipeline);
		gl.getProgramPipelineiv(pipeline, GL_VALIDATE_STATUS, (glw::GLint*)&value);
		if (value != 0)
		{
			TCU_FAIL("Program pipeline validation failed");
		}

		gl.drawElements(GL_TRIANGLES, DE_LENGTH_OF_ARRAY(quadIndices), GL_UNSIGNED_SHORT, NULL);
		err = gl.getError();
		if (err != GL_INVALID_OPERATION)
		{
			TCU_FAIL("DrawElements failed");
		}

		// Program deletion
		gl.useProgramStages(pipeline, GL_VERTEX_SHADER_BIT, programA);
		gl.useProgramStages(pipeline, GL_FRAGMENT_SHADER_BIT, programB);

		// Program B renders red this time
		gl.programUniform4f(programB, gl.getUniformLocation(programB, "u_clrPB"), 1.0f, 0.0f, 0.0f, 1.0f);

		gl.deleteProgram(programB);

		gl.drawElements(GL_TRIANGLES, DE_LENGTH_OF_ARRAY(quadIndices), GL_UNSIGNED_SHORT, NULL);
		glu::readPixels(m_context.getRenderContext(), 0, 0, renderedFrame.getAccess());

		// expect red
		if (!checkSurface(renderedFrame, tcu::RGBA::red()))
		{
			TCU_FAIL("StateInteraction failed; surface should be red");
		}

		// Attach new shader
		srcStrings[0] = frag2.c_str();
		programB	  = gl.createShaderProgramv(GL_FRAGMENT_SHADER, 1, srcStrings);
		if (!checkCSProg(gl, programB))
		{
			TCU_FAIL("CreateShaderProgramv failed for fragment shader");
		}
		// Render green
		gl.programUniform4f(programB, gl.getUniformLocation(programB, "u_clrPB"), 0.0f, 1.0f, 0.0f, 1.0f);
		gl.useProgramStages(pipeline, GL_FRAGMENT_SHADER_BIT, programB);

		// new shader
		glw::GLuint vshader = gl.createShader(GL_FRAGMENT_SHADER);
		srcStrings[0]		= frag.c_str(); // First frag shader with u_color uniform
		gl.shaderSource(vshader, 1, srcStrings, NULL);
		gl.compileShader(vshader);
		gl.getShaderiv(vshader, GL_COMPILE_STATUS, &value);
		DE_ASSERT(value == GL_TRUE);
		gl.attachShader(programB, vshader);

		// changing shader shouldn't affect link_status
		gl.getProgramiv(programB, GL_LINK_STATUS, &value);
		if (value != 1)
		{
			TCU_FAIL("Shader attachment shouldn't affect link status");
		}

		gl.drawElements(GL_TRIANGLES, DE_LENGTH_OF_ARRAY(quadIndices), GL_UNSIGNED_SHORT, NULL);
		GLU_EXPECT_NO_ERROR(gl.getError(), "DrawElements failure");

		glu::readPixels(m_context.getRenderContext(), 0, 0, renderedFrame.getAccess());

		// expect green
		if (!checkSurface(renderedFrame, tcu::RGBA::green()))
		{
			TCU_FAIL("StateInteraction failed; surface should be green");
		}

		// Negative Case: Unsuccessfully linked program should not affect current program

		// Render white
		gl.programUniform4f(programB, gl.getUniformLocation(programB, "u_clrPB"), 1.0f, 1.0f, 1.0f, 1.0f);
		std::string noMain = frag;
		pos				   = noMain.find("main", 0);
		noMain.replace(pos, 4, "niaM");

		srcStrings[0] = noMain.c_str();
		gl.shaderSource(vshader, 1, srcStrings, NULL);
		gl.compileShader(vshader);
		gl.getShaderiv(vshader, GL_COMPILE_STATUS, &value);
		gl.attachShader(programB, vshader);
		gl.linkProgram(programB);
		err = gl.getError();

		// link_status should be false
		gl.getProgramiv(programB, GL_LINK_STATUS, &value);
		if (value != 0)
		{
			TCU_FAIL("StateInteraction failed; link failure");
		}

		gl.drawElements(GL_TRIANGLES, DE_LENGTH_OF_ARRAY(quadIndices), GL_UNSIGNED_SHORT, NULL);

		glu::readPixels(m_context.getRenderContext(), 0, 0, renderedFrame.getAccess());

		// expect white
		if (!checkSurface(renderedFrame, tcu::RGBA::white()))
		{
			TCU_FAIL("StateInteraction failed; surface should be white");
		}

		gl.deleteProgram(programA);
		gl.deleteProgram(programB);

		// Shader interface matching inputs/outputs

		int maxVars = 6; // generate code supports 6 variables
		for (int numInputs = 0; numInputs < maxVars; numInputs++)
		{
			for (int numOutputs = 0; numOutputs < maxVars; numOutputs++)
			{

				generateVarLinkVertexShaderSrc(vtx, m_glslVersion, numOutputs);
				generateVarLinkFragmentShaderSrc(frag, m_glslVersion, numInputs);

				srcStrings[0] = vtx.c_str();
				programA	  = gl.createShaderProgramv(GL_VERTEX_SHADER, 1, srcStrings);
				if (!checkCSProg(gl, programA))
				{
					TCU_FAIL("CreateShaderProgramv failed for vertex shader");
				}

				srcStrings[0] = frag.c_str();
				programB	  = gl.createShaderProgramv(GL_FRAGMENT_SHADER, 1, srcStrings);
				if (!checkCSProg(gl, programB))
				{
					TCU_FAIL("CreateShaderProgramv failed for fragment shader");
				}

				gl.useProgramStages(pipeline, GL_VERTEX_SHADER_BIT, programA);
				gl.useProgramStages(pipeline, GL_FRAGMENT_SHADER_BIT, programB);
				GLU_EXPECT_NO_ERROR(gl.getError(), "UseProgramStages failure");

				gl.validateProgramPipeline(pipeline);
				gl.getProgramPipelineiv(pipeline, GL_VALIDATE_STATUS, (glw::GLint*)&value);

				// Matched input and output variables should render
				if (numInputs == numOutputs)
				{
					if (value != 1)
					{
						log << TestLog::Message << "Matched input and output variables should validate successfully.\n"
							<< "Vertex Shader:\n"
							<< vtx << "Fragment Shader:\n"
							<< frag << TestLog::EndMessage;
						TCU_FAIL("StateInteraction failed");
					}
					gl.clear(GL_COLOR_BUFFER_BIT);
					// white
					gl.programUniform4f(programA, gl.getUniformLocation(programA, "u_color"), 1.0f, 1.0f, 1.0f, 1.0f);
					gl.drawElements(GL_TRIANGLES, DE_LENGTH_OF_ARRAY(quadIndices), GL_UNSIGNED_SHORT, NULL);
					GLU_EXPECT_NO_ERROR(gl.getError(), "DrawElements failure");

					glu::readPixels(m_context.getRenderContext(), 0, 0, renderedFrame.getAccess());

					// expect white
					if (!checkSurface(renderedFrame, tcu::RGBA::white()))
					{
						TCU_FAIL("StateInteraction failed; surface should be white");
					}
				}
				else
				{
					// Mismatched input and output variables
					// For OpenGL ES contexts, this should cause a validation failure
					// For OpenGL contexts, validation should succeed.
					if (glu::isContextTypeES(m_context.getRenderContext().getType()) != (value == 0))
					{
						log << TestLog::Message << "Mismatched input and output variables; validation should "
							<< (glu::isContextTypeES(m_context.getRenderContext().getType()) ? "fail.\n" : "succeed.\n")
							<< "Vertex Shader:\n"
							<< vtx << "Fragment Shader:\n"
							<< frag << TestLog::EndMessage;
						TCU_FAIL("StateInteraction failed");
					}
				}

				gl.deleteProgram(programA);
				gl.deleteProgram(programB);
			}
		}

		gl.bindProgramPipeline(0);
		gl.bindVertexArray(0);
		gl.deleteProgramPipelines(1, &pipeline);
		gl.deleteShader(vshader);
		gl.deleteVertexArrays(1, &vao);
		gl.deleteBuffers(1, &indexBuf);
		gl.deleteBuffers(1, &vertexBuf);

		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
		return STOP;
	}

private:
	glu::GLSLVersion m_glslVersion;
};

// Testcase for interface qualifiers matching
class InterfaceMatchingCase : public TestCase
{
public:
	enum TestType
	{
		DEFAULT_PRECISION,
		SET_DEFAULT_PRECISION,
		SET_PRECISION
	};

	std::string getTestTypeName(TestType testType)
	{
		switch (testType)
		{
		case DEFAULT_PRECISION:
			return "use predeclared precision";
		case SET_DEFAULT_PRECISION:
			return "set default precision";
		case SET_PRECISION:
			return "explicit precision";
		}
		return "";
	}

	InterfaceMatchingCase(Context& context, const char* name, glu::GLSLVersion glslVersion)
		: TestCase(context, name, "matching precision qualifiers between stages"), m_glslVersion(glslVersion)
	{
	}

	~InterfaceMatchingCase(void)
	{
	}

	string getDefaultFragmentPrecision()
	{
		return "";
	}

	// Generate a vertex shader for variable input/output precision testing
	virtual void generateVarLinkVertexShaderSrc(std::string& outVtxSrc, glu::GLSLVersion glslVersion,
												const string& precision, TestType testMode) = 0;

	// Generate a fragment shader for variable input/output precision testing
	virtual void generateVarLinkFragmentShaderSrc(std::string& outFragSrc, glu::GLSLVersion glslVersion,
												  const string& precision, TestType testMode) = 0;

	// Verify the surface is filled with the expected color
	bool checkSurface(tcu::Surface surface, tcu::RGBA expectedColor)
	{
		int numFailedPixels = 0;
		for (int y = 0; y < surface.getHeight(); y++)
		{
			for (int x = 0; x < surface.getWidth(); x++)
			{
				if (surface.getPixel(x, y) != expectedColor)
					numFailedPixels += 1;
			}
		}
		return (numFailedPixels == 0);
	}

	// Log the program info log
	void logProgramInfoLog(const glw::Functions& gl, glw::GLuint program)
	{
		TestLog&	 log	 = m_testCtx.getLog();
		glw::GLint   value   = 0;
		glw::GLsizei bufSize = 0;
		glw::GLsizei length  = 0;

		gl.getProgramiv(program, GL_INFO_LOG_LENGTH, &value);
		std::vector<char> infoLogBuf(value + 1);

		gl.getProgramInfoLog(program, bufSize, &length, &infoLogBuf[0]);
		GLU_EXPECT_NO_ERROR(gl.getError(), "GetProgramInfoLog failed");

		log << TestLog::Message << "Program Log:\n" << &infoLogBuf[0] << TestLog::EndMessage;
	}

	// Check program validity created with CreateShaderProgram
	bool checkCSProg(const glw::Functions& gl, GLuint program, int expectedLink = GL_TRUE)
	{
		int linked = GL_FALSE;
		if (program != 0)
		{
			gl.getProgramiv(program, GL_LINK_STATUS, &linked);

			if (expectedLink && !linked)
			{
				logProgramInfoLog(gl, program);
			}
		}

		return (program != 0) && (linked == expectedLink);
	}

	IterateResult iterate(void)
	{
		TestLog&				 log		  = m_testCtx.getLog();
		const glw::Functions&	gl			  = m_context.getRenderContext().getFunctions();
		const tcu::RenderTarget& renderTarget = m_context.getRenderContext().getRenderTarget();
		int						 viewportW	= de::min(16, renderTarget.getWidth());
		int						 viewportH	= de::min(16, renderTarget.getHeight());
		tcu::Surface			 renderedFrame(viewportW, viewportH);

		glw::GLuint programA, programB;
		glw::GLuint vao, vertexBuf, indexBuf;
		std::string vtx;
		std::string frag, frag2;
		glw::GLuint pipeline;
		const char* srcStrings[1];
		glw::GLuint value;

		gl.viewport(0, 0, viewportW, viewportH);
		gl.clearColor(0.0f, 0.0f, 0.0f, 0.0f);
		gl.clear(GL_COLOR_BUFFER_BIT);

		static const deUint16 quadIndices[] = { 0, 1, 2, 2, 1, 3 };
		const float			  position[]	= { -1.0f, -1.0f, +1.0f, 1.0f, -1.0f, +1.0f, 0.0f,  1.0f,
								   +1.0f, -1.0f, 0.0f,  1.0f, +1.0f, +1.0f, -1.0f, 1.0f };

		/* Set up a vertex array object */
		gl.genVertexArrays(1, &vao);
		gl.bindVertexArray(vao);

		gl.genBuffers(1, &indexBuf);
		gl.bindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuf);
		gl.bufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(quadIndices), quadIndices, GL_STATIC_DRAW);

		gl.genBuffers(1, &vertexBuf);
		gl.bindBuffer(GL_ARRAY_BUFFER, vertexBuf);
		gl.bufferData(GL_ARRAY_BUFFER, sizeof(position), position, GL_STATIC_DRAW);

		/* Set up shader pipeline */
		gl.genProgramPipelines(1, &pipeline);
		gl.bindProgramPipeline(pipeline);

		struct PrecisionTests
		{
			TestType	testType;
			std::string precision;
		};

		PrecisionTests vertexPrecisionTests[] = {
			{ DEFAULT_PRECISION, "highp" },	{ SET_DEFAULT_PRECISION, "highp" }, { SET_DEFAULT_PRECISION, "mediump" },
			{ SET_DEFAULT_PRECISION, "lowp" }, { SET_PRECISION, "highp" },		   { SET_PRECISION, "mediump" },
			{ SET_PRECISION, "lowp" }
		};

		PrecisionTests fragmentPrecisionTests[] = { { DEFAULT_PRECISION, getDefaultFragmentPrecision() },
													{ SET_DEFAULT_PRECISION, "highp" },
													{ SET_DEFAULT_PRECISION, "mediump" },
													{ SET_DEFAULT_PRECISION, "lowp" },
													{ SET_PRECISION, "highp" },
													{ SET_PRECISION, "mediump" },
													{ SET_PRECISION, "lowp" } };

		// Shader interface matching inputs/outputs precision
		int maxTests = 7;
		for (int vertexTestIteration = 0; vertexTestIteration < maxTests; vertexTestIteration++)
		{
			std::string vertexPrecision = vertexPrecisionTests[vertexTestIteration].precision;
			TestType	vertexTestType  = vertexPrecisionTests[vertexTestIteration].testType;
			for (int fragmentTestIteration = 0; fragmentTestIteration < maxTests; fragmentTestIteration++)
			{
				std::string fragmentPrecision = fragmentPrecisionTests[fragmentTestIteration].precision;
				TestType	fragmentTestType  = fragmentPrecisionTests[fragmentTestIteration].testType;
				if (fragmentPrecision.empty())
					continue;

				log << TestLog::Message << "vertex shader precision: " << vertexPrecision
					<< ", shader test mode: " << getTestTypeName(vertexTestType) << TestLog::EndMessage;

				log << TestLog::Message << "fragment shader precision: " << fragmentPrecision
					<< ", shader test mode: " << getTestTypeName(fragmentTestType) << TestLog::EndMessage;

				generateVarLinkVertexShaderSrc(vtx, m_glslVersion, vertexPrecision, vertexTestType);
				generateVarLinkFragmentShaderSrc(frag, m_glslVersion, fragmentPrecision, fragmentTestType);

				srcStrings[0] = vtx.c_str();
				programA	  = gl.createShaderProgramv(GL_VERTEX_SHADER, 1, srcStrings);
				if (!checkCSProg(gl, programA))
				{
					TCU_FAIL("CreateShaderProgramv failed for vertex shader");
				}
				srcStrings[0] = frag.c_str();
				programB	  = gl.createShaderProgramv(GL_FRAGMENT_SHADER, 1, srcStrings);
				if (!checkCSProg(gl, programB))
				{
					TCU_FAIL("CreateShaderProgramv failed for fragment shader");
				}

				gl.useProgramStages(pipeline, GL_VERTEX_SHADER_BIT, programA);
				gl.useProgramStages(pipeline, GL_FRAGMENT_SHADER_BIT, programB);
				GLU_EXPECT_NO_ERROR(gl.getError(), "InterfaceMatching failure");

				// Mismatched input and output qualifiers
				// For OpenGL ES contexts, this should result in a validation failure.
				// For OpenGL contexts, validation should succeed.
				gl.validateProgramPipeline(pipeline);
				gl.getProgramPipelineiv(pipeline, GL_VALIDATE_STATUS, (glw::GLint*)&value);
				int precisionCompareResult = fragmentPrecision.compare(vertexPrecision);
				if (glu::isContextTypeES(m_context.getRenderContext().getType()) && (precisionCompareResult != 0))
				{
					// precision mismatch
					if (value != GL_FALSE)
					{
						log.startShaderProgram(
							false, "Precision mismatch, pipeline validation status GL_TRUE expected GL_FALSE");
						log.writeShader(QP_SHADER_TYPE_VERTEX, vtx.c_str(), true, "");
						log.writeShader(QP_SHADER_TYPE_FRAGMENT, frag.c_str(), true, "");
						log.endShaderProgram();
						TCU_FAIL("InterfaceMatchingCase failed");
					}
					else
					{
						log << TestLog::Message << "Precision mismatch, Pipeline validation status GL_FALSE -> OK"
							<< TestLog::EndMessage;
					}
				}
				else
				{
					if (value != GL_TRUE)
					{
						std::stringstream str;
						str << "Precision " << (precisionCompareResult ? "mismatch" : "matches")
							<< ", pipeline validation status GL_FALSE expected GL_TRUE";

						log.startShaderProgram(false, str.str().c_str());
						log.writeShader(QP_SHADER_TYPE_VERTEX, vtx.c_str(), true, "");
						log.writeShader(QP_SHADER_TYPE_FRAGMENT, frag.c_str(), true, "");
						log.endShaderProgram();
						TCU_FAIL("InterfaceMatchingCase failed");
					}
					else
					{
						log << TestLog::Message << "Precision " << (precisionCompareResult ? "mismatch" : "matches")
							<< ", pipeline validation status GL_TRUE -> OK" << TestLog::EndMessage;
						// precision matches
						gl.clear(GL_COLOR_BUFFER_BIT);
						// white
						int posLoc = gl.getAttribLocation(programA, "a_position");
						gl.vertexAttribPointer(posLoc, 4, GL_FLOAT, GL_FALSE, 0, 0);
						gl.enableVertexAttribArray(posLoc);
						gl.programUniform4f(programA, gl.getUniformLocation(programA, "u_color"), 1.0f, 1.0f, 1.0f,
											1.0f);
						GLU_EXPECT_NO_ERROR(gl.getError(), "StateInteraction failure, set uniform value");
						gl.drawElements(GL_TRIANGLES, DE_LENGTH_OF_ARRAY(quadIndices), GL_UNSIGNED_SHORT, NULL);
						GLU_EXPECT_NO_ERROR(gl.getError(), "DrawElements failure");
						gl.disableVertexAttribArray(posLoc);

						glu::readPixels(m_context.getRenderContext(), 0, 0, renderedFrame.getAccess());

						// expect white
						if (!checkSurface(renderedFrame, tcu::RGBA::white()))
						{
							TCU_FAIL("InterfaceMatchingCase failed; surface should be white");
						}
					}
				}

				// validate non separable program

				glu::ShaderProgram progVF(m_context.getRenderContext(),
										  glu::makeVtxFragSources(vtx.c_str(), frag.c_str()));

				gl.useProgram(progVF.getProgram());
				gl.uniform4f(gl.getUniformLocation(progVF.getProgram(), "u_color"), 1.0f, 1.0f, 1.0f, 1.0f);
				if (!progVF.getProgramInfo().linkOk)
				{
					log << progVF;
					log << TestLog::Message << "Non separable program link status GL_FALSE expected GL_TRUE"
						<< TestLog::EndMessage;
					TCU_FAIL("InterfaceMatchingCase failed, non separable program should link");
				}

				int posLoc = gl.getAttribLocation(progVF.getProgram(), "a_position");
				gl.vertexAttribPointer(posLoc, 4, GL_FLOAT, GL_FALSE, 0, 0);
				gl.enableVertexAttribArray(posLoc);
				gl.drawElements(GL_TRIANGLES, DE_LENGTH_OF_ARRAY(quadIndices), GL_UNSIGNED_SHORT, NULL);
				gl.disableVertexAttribArray(posLoc);

				GLU_EXPECT_NO_ERROR(gl.getError(), "StateInteraction failure, non separable program draw call");
				glu::readPixels(m_context.getRenderContext(), 0, 0, renderedFrame.getAccess());
				// expect white
				if (!checkSurface(renderedFrame, tcu::RGBA::white()))
				{
					TCU_FAIL("InterfaceMatchingCase failed, non separable program, unexpected color found");
				}

				gl.deleteProgram(programA);
				gl.deleteProgram(programB);
				gl.useProgram(0);
			}
		}
		gl.bindVertexArray(0);
		gl.deleteVertexArrays(1, &vao);
		gl.deleteBuffers(1, &indexBuf);
		gl.deleteBuffers(1, &vertexBuf);
		gl.bindProgramPipeline(0);
		gl.deleteProgramPipelines(1, &pipeline);

		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
		return STOP;
	}

protected:
	glu::GLSLVersion m_glslVersion;
};

class InterfaceMatchingCaseFloat : public InterfaceMatchingCase
{
public:
	InterfaceMatchingCaseFloat(Context& context, const char* name, glu::GLSLVersion glslVersion)
		: InterfaceMatchingCase(context, name, glslVersion)
	{
	}

	void generateVarLinkVertexShaderSrc(std::string& outVtxSrc, glu::GLSLVersion glslVersion, const string& precision,
										TestType testMode)
	{
		std::ostringstream vtxSrc;
		vtxSrc << glu::getGLSLVersionDeclaration(glslVersion) << "\n";
		if (glslVersion >= glu::GLSL_VERSION_410)
		{
			vtxSrc << "out gl_PerVertex {\n"
					  "  vec4 gl_Position;\n"
					  "};\n";
		}
		vtxSrc << "in highp vec4 a_position;\n";
		vtxSrc << "uniform highp vec4 u_color;\n";
		switch (testMode)
		{
		case SET_DEFAULT_PRECISION:
			vtxSrc << "precision " << precision << " float;\n";
		case DEFAULT_PRECISION:
			vtxSrc << "out float var;\n";
			break;
		case SET_PRECISION:
			vtxSrc << "out " << precision << " float var;\n";
			break;
		}
		vtxSrc << "void main (void)\n";
		vtxSrc << "{\n";
		vtxSrc << "   gl_Position = a_position;\n";
		vtxSrc << "   var = u_color.r;\n";
		vtxSrc << "}\n";
		outVtxSrc = vtxSrc.str();
	}

	void generateVarLinkFragmentShaderSrc(std::string& outFragSrc, glu::GLSLVersion glslVersion,
										  const string& precision, TestType testMode)
	{
		std::ostringstream fragSrc;
		fragSrc << glu::getGLSLVersionDeclaration(glslVersion) << "\n";
		switch (testMode)
		{
		case SET_DEFAULT_PRECISION:
			fragSrc << "precision " << precision << " float;\n";
		case DEFAULT_PRECISION:
			fragSrc << "in float var;\n";
			break;
		case SET_PRECISION:
			fragSrc << "in " << precision << " float var;\n";
			break;
		}
		fragSrc << "layout(location = 0) out mediump vec4 o_color;\n";
		fragSrc << "void main (void)\n";
		fragSrc << "{\n";
		fragSrc << "    o_color = vec4(var);\n";
		fragSrc << "}\n";
		outFragSrc = fragSrc.str();
	}
};

class InterfaceMatchingCaseInt : public InterfaceMatchingCase
{
public:
	InterfaceMatchingCaseInt(Context& context, const char* name, glu::GLSLVersion glslVersion)
		: InterfaceMatchingCase(context, name, glslVersion)
	{
	}

	std::string getDefaultFragmentPrecision()
	{
		return "mediump";
	}

	void generateVarLinkVertexShaderSrc(std::string& outVtxSrc, glu::GLSLVersion glslVersion, const string& precision,
										TestType testMode)
	{
		std::ostringstream vtxSrc;
		vtxSrc << glu::getGLSLVersionDeclaration(glslVersion) << "\n";
		if (glslVersion >= glu::GLSL_VERSION_410)
		{
			vtxSrc << "out gl_PerVertex {\n"
					  "  vec4 gl_Position;\n"
					  "};\n";
		}
		vtxSrc << "in highp vec4 a_position;\n";
		vtxSrc << "uniform highp vec4 u_color;\n";
		switch (testMode)
		{
		case SET_DEFAULT_PRECISION:
			vtxSrc << "precision " << precision << " int;\n";
		case DEFAULT_PRECISION:
			vtxSrc << "flat out int var;\n";
			break;
		case SET_PRECISION:
			vtxSrc << "flat out " << precision << " int var;\n";
			break;
		}
		vtxSrc << "void main (void)\n";
		vtxSrc << "{\n";
		vtxSrc << "   gl_Position = a_position;\n";
		vtxSrc << "   var = int(u_color.r);\n";
		vtxSrc << "}\n";
		outVtxSrc = vtxSrc.str();
	}

	void generateVarLinkFragmentShaderSrc(std::string& outFragSrc, glu::GLSLVersion glslVersion,
										  const string& precision, TestType testMode)
	{
		std::ostringstream fragSrc;
		fragSrc << glu::getGLSLVersionDeclaration(glslVersion) << "\n";
		switch (testMode)
		{
		case SET_DEFAULT_PRECISION:
			fragSrc << "precision " << precision << " int;\n";
		case DEFAULT_PRECISION:
			fragSrc << "flat in int var;\n";
			break;
		case SET_PRECISION:
			fragSrc << "flat in " << precision << " int var;\n";
			break;
		}
		fragSrc << "layout(location = 0) out mediump vec4 o_color;\n";
		fragSrc << "void main (void)\n";
		fragSrc << "{\n";
		fragSrc << "    o_color = vec4(var);\n";
		fragSrc << "}\n";
		outFragSrc = fragSrc.str();
	}
};

class InterfaceMatchingCaseUInt : public InterfaceMatchingCase
{
public:
	InterfaceMatchingCaseUInt(Context& context, const char* name, glu::GLSLVersion glslVersion)
		: InterfaceMatchingCase(context, name, glslVersion)
	{
	}

	std::string getDefaultFragmentPrecision()
	{
		return "mediump";
	}

	void generateVarLinkVertexShaderSrc(std::string& outVtxSrc, glu::GLSLVersion glslVersion, const string& precision,
										TestType testMode)
	{
		std::ostringstream vtxSrc;
		vtxSrc << glu::getGLSLVersionDeclaration(glslVersion) << "\n";
		if (glslVersion >= glu::GLSL_VERSION_410)
		{
			vtxSrc << "out gl_PerVertex {\n"
					  "  vec4 gl_Position;\n"
					  "};\n";
		}
		vtxSrc << "in highp vec4 a_position;\n";
		vtxSrc << "uniform highp vec4 u_color;\n";
		switch (testMode)
		{
		case SET_DEFAULT_PRECISION:
			vtxSrc << "precision " << precision << " int;\n";
		case DEFAULT_PRECISION:
			vtxSrc << "flat out uint var;\n";
			break;
		case SET_PRECISION:
			vtxSrc << "flat out " << precision << " uint var;\n";
			break;
		}
		vtxSrc << "void main (void)\n";
		vtxSrc << "{\n";
		vtxSrc << "   gl_Position = a_position;\n";
		vtxSrc << "   var = uint(u_color.r);\n";
		vtxSrc << "}\n";
		outVtxSrc = vtxSrc.str();
	}

	void generateVarLinkFragmentShaderSrc(std::string& outFragSrc, glu::GLSLVersion glslVersion,
										  const string& precision, TestType testMode)
	{
		std::ostringstream fragSrc;
		fragSrc << glu::getGLSLVersionDeclaration(glslVersion) << "\n";
		switch (testMode)
		{
		case SET_DEFAULT_PRECISION:
			fragSrc << "precision " << precision << " int;\n";
		case DEFAULT_PRECISION:
			fragSrc << "flat in uint var;\n";
			break;
		case SET_PRECISION:
			fragSrc << "flat in " << precision << " uint var;\n";
			break;
		}
		fragSrc << "layout(location = 0) out mediump vec4 o_color;\n";
		fragSrc << "void main (void)\n";
		fragSrc << "{\n";
		fragSrc << "    o_color = vec4(var);\n";
		fragSrc << "}\n";
		outFragSrc = fragSrc.str();
	}
};

SeparateShaderObjsTests::SeparateShaderObjsTests(Context& context, glu::GLSLVersion glslVersion)
	: TestCaseGroup(context, "sepshaderobjs", "separate_shader_object tests"), m_glslVersion(glslVersion)
{
	DE_ASSERT(glslVersion == glu::GLSL_VERSION_310_ES || glslVersion >= glu::GLSL_VERSION_440);
}

SeparateShaderObjsTests::~SeparateShaderObjsTests(void)
{
}

void SeparateShaderObjsTests::init(void)
{

	// API validation for CreateShaderProgram
	addChild(new CreateShadProgCase(m_context, "CreateShadProgApi", "createShaderProgram API", m_glslVersion));
	// API validation for UseProgramStages
	addChild(new UseProgStagesCase(m_context, "UseProgStagesApi", "useProgramStages API", m_glslVersion));
	// API validation for pipeline related functions
	addChild(new PipelineApiCase(m_context, "PipelineApi", "Pipeline API", m_glslVersion));
	// API validation for variations of ProgramUniform
	addChild(new ProgramUniformCase(m_context, "ProgUniformAPI", "ProgramUniform API", m_glslVersion));
	// State interactions
	addChild(new StateInteractionCase(m_context, "StateInteraction", "SSO State Interactions", m_glslVersion));
	// input / output precision matching
	addChild(new InterfaceMatchingCaseFloat(m_context, "InterfacePrecisionMatchingFloat", m_glslVersion));
	addChild(new InterfaceMatchingCaseInt(m_context, "InterfacePrecisionMatchingInt", m_glslVersion));
	addChild(new InterfaceMatchingCaseUInt(m_context, "InterfacePrecisionMatchingUInt", m_glslVersion));
}

} // glcts
