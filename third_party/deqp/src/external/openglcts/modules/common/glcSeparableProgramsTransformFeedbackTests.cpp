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
 * \file glcSeparableProgramXFBTests.cpp
 * \brief
 */ /*-------------------------------------------------------------------*/

#include "glcSeparableProgramsTransformFeedbackTests.hpp"
#include "glcViewportArrayTests.hpp"
#include "gluContextInfo.hpp"
#include "gluDefs.hpp"
#include "glwEnums.hpp"
#include "glwFunctions.hpp"
#include "tcuCommandLine.hpp"
#include "tcuStringTemplate.hpp"
#include "tcuTestLog.hpp"

using namespace tcu;
using namespace glu;
using namespace glw;
using namespace glcts::ViewportArray;

namespace glcts
{

/**
 * @brief The StageIndex enum. Stages order coresponds to order
 * in which shader sources are specified in Utils::program::build.
 */
enum StageIndex
{
	FRAGMENT_STAGE_INDEX = 0,
	GEOMETRY_STAGE_INDEX,
	TESSELLATION_CONTROL_STAGE,
	TESSELLATION_EVALUATION_STAGE,
	VERTEX_STAGE,
	STAGES_COUNT
};

/**
 * @brief The StageTokens array. Stages order coresponds to order
 * in which shader sources are specified in Utils::program::build.
 */
static const GLenum StageTokens[STAGES_COUNT] = { GL_FRAGMENT_SHADER_BIT, GL_GEOMETRY_SHADER_BIT,
												  GL_TESS_CONTROL_SHADER_BIT, GL_TESS_EVALUATION_SHADER_BIT,
												  GL_VERTEX_SHADER_BIT };

/**
 * @brief The StageData structure.
 */
struct StageData
{
	const GLchar*		 source;
	const GLchar* const* tfVaryings;
	const GLuint		 tfVaryingsCount;
};

/**
 * @brief The PerStageData structure containimg shader data per all stages.
 */
struct PerStageData
{
	StageData stage[STAGES_COUNT];
};

static const GLchar* vs_code = "${VERSION}\n"
							   "flat out highp int o_vert;\n"
							   "${PERVERTEX_BLOCK}\n"
							   "void main()\n"
							   "{\n"
							   "    o_vert = 1;\n"
							   "    gl_Position = vec4(1, 0, 0, 1);\n"
							   "}\n";

static const GLchar* vs_tf_varyings[] = { "o_vert" };

static const GLchar* tcs_code = "${VERSION}\n"
								"layout(vertices = 1) out;\n"
								"flat in highp int o_vert[];\n"
								"${PERVERTEX_BLOCK}\n"
								"void main()\n"
								"{\n"
								"    gl_TessLevelInner[0] = 1.0;\n"
								"    gl_TessLevelInner[1] = 1.0;\n"
								"    gl_TessLevelOuter[0] = 1.0;\n"
								"    gl_TessLevelOuter[1] = 1.0;\n"
								"    gl_TessLevelOuter[2] = 1.0;\n"
								"    gl_TessLevelOuter[3] = 1.0;\n"
								"}\n";

static const GLchar* tes_code = "${VERSION}\n"
								"layout (triangles, point_mode) in;\n"
								"flat out highp int o_tess;\n"
								"${PERVERTEX_BLOCK}\n"
								"void main()\n"
								"{\n"
								"    o_tess = 2;\n"
								"    gl_Position = vec4(gl_TessCoord.xy*2.0 - 1.0, 0.0, 1.0);\n"
								"}\n";

static const GLchar* tes_tf_varyings[] = { "o_tess" };

static const GLchar* gs_code = "${VERSION}\n"
							   "layout (points) in;\n"
							   "layout (points, max_vertices = 3) out;\n"
							   "${PERVERTEX_BLOCK}\n"
							   "flat in highp int ${IN_VARYING_NAME}[];\n"
							   "flat out highp int o_geom;\n"
							   "void main()\n"
							   "{\n"
							   "    o_geom = 3;\n"
							   "    gl_Position  = vec4(-1, -1, 0, 1);\n"
							   "    EmitVertex();\n"
							   "    o_geom = 3;\n"
							   "    gl_Position  = vec4(-1, 1, 0, 1);\n"
							   "    EmitVertex();\n"
							   "    o_geom = 3;\n"
							   "    gl_Position  = vec4(1, -1, 0, 1);\n"
							   "    EmitVertex();\n"
							   "}\n";

static const GLchar* gs_tf_varyings[] = { "o_geom" };

static const GLchar* fs_code = "${VERSION}\n"
							   "flat in highp int ${IN_VARYING_NAME};"
							   "out highp vec4 o_color;\n"
							   "void main()\n"
							   "{\n"
							   "    o_color = vec4(1.0);\n"
							   "}\n";

class SeparableProgramTFTestCase : public deqp::TestCase
{
public:
	/* Public methods */
	SeparableProgramTFTestCase(deqp::Context& context, const char* name, PerStageData shaderData, GLint expectedValue);

	tcu::TestNode::IterateResult iterate(void);

protected:
	/* Protected attributes */
	PerStageData m_shaderData;
	GLint		 m_expectedValue;
};

/** Constructor.
	 *
	 *  @param context     Rendering context
	 *  @param name        Test name
	 *  @param description Test description
	 */
SeparableProgramTFTestCase::SeparableProgramTFTestCase(deqp::Context& context, const char* name,
													   PerStageData shaderData, GLint expectedValue)
	: deqp::TestCase(context, name, ""), m_shaderData(shaderData), m_expectedValue(expectedValue)
{
}

tcu::TestNode::IterateResult SeparableProgramTFTestCase::iterate(void)
{
	const Functions& gl			 = m_context.getRenderContext().getFunctions();
	ContextType		 contextType = m_context.getRenderContext().getType();
	GLSLVersion		 glslVersion = getContextTypeGLSLVersion(contextType);

	/* For core GL gl_PerVertex interface block is combined from two parts.
	 * First part contains definition and the second part name, which is
	 * only specified for tess control stage (arrays are used here to avoid
	 * three branches in a loop). For ES both parts are empty string */
	const char*  blockName[STAGES_COUNT]	  = { "", ";\n", " gl_out[];\n", ";\n", ";\n" };
	const char*  blockEmptyName[STAGES_COUNT] = { "", "", "", "", "" };
	std::string  vertexBlock("");
	const char** vertexBlockPostfix = blockEmptyName;
	if (isContextTypeGLCore(contextType))
	{
		vertexBlock = "out gl_PerVertex"
					  "{\n"
					  "    vec4 gl_Position;\n"
					  "}";
		vertexBlockPostfix = blockName;
	}

	/* Construct specialization map - some specializations differ per stage */
	std::map<std::string, std::string> specializationMap;
	specializationMap["VERSION"] = glu::getGLSLVersionDeclaration(glslVersion);

	/* Create separate programs - start from vertex stage to catch varying names */
	std::vector<Utils::program> programs(STAGES_COUNT, Utils::program(m_context));
	const char*					code[STAGES_COUNT] = { 0, 0, 0, 0, 0 };
	for (int stageIndex = VERTEX_STAGE; stageIndex > -1; --stageIndex)
	{
		StageData*  stageData = m_shaderData.stage + stageIndex;
		std::string source	= stageData->source;
		if (source.empty())
			continue;
		specializationMap["PERVERTEX_BLOCK"] = vertexBlock + vertexBlockPostfix[stageIndex];
		std::string specializedShader		 = StringTemplate(source).specialize(specializationMap);

		code[stageIndex] = specializedShader.c_str();
		programs[stageIndex].build(0, code[0], code[1], code[2], code[3], code[4], stageData->tfVaryings,
								   stageData->tfVaryingsCount, true);
		code[stageIndex] = 0;

		/* Use varying name from current stage to specialize next stage */
		if (stageData->tfVaryings)
			specializationMap["IN_VARYING_NAME"] = stageData->tfVaryings[0];
	}

	/* Create program pipeline */
	GLuint pipelineId;
	gl.genProgramPipelines(1, &pipelineId);
	gl.bindProgramPipeline(pipelineId);
	for (int stageIndex = 0; stageIndex < STAGES_COUNT; ++stageIndex)
	{
		if (!programs[stageIndex].m_program_object_id)
			continue;
		gl.useProgramStages(pipelineId, StageTokens[stageIndex], programs[stageIndex].m_program_object_id);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgramStages() call failed.");
	}

	/* Validate the pipeline */
	GLint validateStatus = GL_FALSE;
	gl.validateProgramPipeline(pipelineId);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glValidateProgramPipeline() call failed.");
	gl.getProgramPipelineiv(pipelineId, GL_VALIDATE_STATUS, &validateStatus);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetProgramPipelineiv() call failed.");
	if (validateStatus != GL_TRUE)
	{
		GLint logLength;
		gl.getProgramPipelineiv(pipelineId, GL_INFO_LOG_LENGTH, &logLength);
		if (logLength)
		{
			std::vector<GLchar> logBuffer(logLength + 1);
			gl.getProgramPipelineInfoLog(pipelineId, logLength + 1, NULL, &logBuffer[0]);
			m_context.getTestContext().getLog() << tcu::TestLog::Message << &logBuffer[0] << tcu::TestLog::EndMessage;
		}
		TCU_FAIL("Program pipeline has not been validated successfully.");
	}

	/* Generate buffer object to hold result XFB data */
	Utils::buffer tfb(m_context);
	GLsizeiptr	tfbSize = 100;
	tfb.generate(GL_TRANSFORM_FEEDBACK_BUFFER);
	tfb.update(tfbSize, 0 /* data */, GL_DYNAMIC_COPY);
	tfb.bindRange(0, 0, tfbSize);

	/* Generate VAO to use for the draw calls */
	Utils::vertexArray vao(m_context);
	vao.generate();
	vao.bind();

	/* Generate query object */
	GLuint queryId;
	gl.genQueries(1, &queryId);

	/* Check if tessellation stage is active */
	GLenum drawMode = GL_POINTS;
	if (strlen(m_shaderData.stage[TESSELLATION_CONTROL_STAGE].source) > 0)
		drawMode = GL_PATCHES;

	/* Draw and capture data */
	gl.beginQuery(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN, queryId);
	gl.beginTransformFeedback(GL_POINTS);
	gl.patchParameteri(GL_PATCH_VERTICES, 1);
	gl.drawArrays(drawMode, 0 /* first */, 1 /* count */);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawArrays() call failed.");
	gl.endTransformFeedback();
	gl.endQuery(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN);

	/* Get TF results */
	GLuint writtenPrimitives = 0;
	gl.getQueryObjectuiv(queryId, GL_QUERY_RESULT, &writtenPrimitives);
	GLint* feedbackData = (GLint*)gl.mapBufferRange(GL_TRANSFORM_FEEDBACK_BUFFER, 0, tfbSize, GL_MAP_READ_BIT);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glMapBuffer");

	/* Verify if only values from upstream shader were captured */
	m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
	if (writtenPrimitives != 0)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
		for (GLuint dataIndex = 0; dataIndex < writtenPrimitives; ++dataIndex)
		{
			if (feedbackData[dataIndex] == m_expectedValue)
				continue;
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
			break;
		}
	}

	/* Cleanup */
	gl.unmapBuffer(GL_TRANSFORM_FEEDBACK_BUFFER);
	gl.deleteQueries(1, &queryId);
	gl.bindProgramPipeline(0);
	gl.deleteProgramPipelines(1, &pipelineId);

	return STOP;
}

/** Constructor.
	 *
	 *  @param context Rendering context.
	 */
SeparableProgramsTransformFeedbackTests::SeparableProgramsTransformFeedbackTests(deqp::Context& context)
	: deqp::TestCaseGroup(context, "separable_programs_tf", "")
{
}

/** Initializes the test group contents. */
void SeparableProgramsTransformFeedbackTests::init(void)
{
	PerStageData tessellation_active = { {
		{ fs_code, NULL, 0 },			  // fragment stage
		{ "", NULL, 0 },				  // geometry stage
		{ tcs_code, NULL, 0 },			  // tesselation control stage
		{ tes_code, tes_tf_varyings, 1 }, // tesselation evaluation stage
		{ vs_code, vs_tf_varyings, 1 }	// vertex_stage
	} };
	PerStageData geometry_active = { {
		{ fs_code, NULL, 0 },			  // fragment stage
		{ gs_code, gs_tf_varyings, 1 },   // geometry stage
		{ tcs_code, NULL, 0 },			  // tesselation control stage
		{ tes_code, tes_tf_varyings, 1 }, // tesselation evaluation stage
		{ vs_code, vs_tf_varyings, 1 }	// vertex_stage
	} };

	addChild(new SeparableProgramTFTestCase(m_context, "tessellation_active", tessellation_active, 2));
	addChild(new SeparableProgramTFTestCase(m_context, "geometry_active", geometry_active, 3));
}

} /* glcts namespace */
