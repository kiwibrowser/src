/*-------------------------------------------------------------------------
 * OpenGL Conformance Test Suite
 * -----------------------------
 *
 * Copyright (c) 2016 The Khronos Group Inc.
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
 * \file  gl4cShaderDrawParametersTests.cpp
 * \brief Conformance tests for the GL_ARB_shader_draw_parameters functionality.
 */ /*-------------------------------------------------------------------*/

#include "gl4cShaderDrawParametersTests.hpp"
#include "gluContextInfo.hpp"
#include "gluDefs.hpp"
#include "gluDrawUtil.hpp"
#include "gluShaderProgram.hpp"
#include "glwEnums.hpp"
#include "glwFunctions.hpp"
#include "tcuRenderTarget.hpp"
#include "tcuStringTemplate.hpp"
#include "tcuTestLog.hpp"

using namespace glw;
using namespace glu;

namespace gl4cts
{

const char* sdp_compute_extensionCheck = "#version 450 core\n"
										 "\n"
										 "#extension GL_ARB_shader_draw_parameters : require\n"
										 "\n"
										 "#ifndef GL_ARB_shader_draw_parameters\n"
										 "  #error GL_ARB_shader_draw_parameters not defined\n"
										 "#else\n"
										 "  #if (GL_ARB_shader_draw_parameters != 1)\n"
										 "    #error GL_ARB_shader_draw_parameters wrong value\n"
										 "  #endif\n"
										 "#endif // GL_ARB_shader_draw_parameters\n"
										 "\n"
										 "layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;\n"
										 "\n"
										 "void main()\n"
										 "{\n"
										 "}\n";

static const char* sdp_vertShader = "${VERSION}\n"
									"\n"
									"${DRAW_PARAMETERS_EXTENSION}\n"
									"\n"
									"in vec3 vertex;\n"
									"out vec3 color;\n"
									"\n"
									"void main()\n"
									"{\n"
									"    float hOffset = float(${GL_BASE_VERTEX}) / 5.0;\n"
									"    float vOffset = float(${GL_BASE_INSTANCE}) / 5.0;\n"
									"    color = vec3(0.0);\n"
									"    if (${GL_DRAW_ID} % 3 == 0) color.r = 1;\n"
									"    else if (${GL_DRAW_ID} % 3 == 1) color.g = 1;\n"
									"    else if (${GL_DRAW_ID} % 3 == 2) color.b = 1;\n"
									"    gl_Position = vec4(vertex + vec3(hOffset, vOffset, 0), 1);\n"
									"}\n";

static const char* sdp_fragShader = "${VERSION}\n"
									"\n"
									"${DRAW_PARAMETERS_EXTENSION}\n"
									"\n"
									"in vec3 color;\n"
									"out vec4 fragColor;\n"
									"\n"
									"void main()\n"
									"{\n"
									"    fragColor = vec4(color, 1.0);\n"
									"}\n";

/** Constructor.
 *
 *  @param context     Rendering context
 */
ShaderDrawParametersExtensionTestCase::ShaderDrawParametersExtensionTestCase(deqp::Context& context)
	: TestCase(context, "ShaderDrawParametersExtension",
			   "Verifies if GL_ARB_shader_draw_parameters extension is available for GLSL")
{
	/* Left blank intentionally */
}

/** Executes test iteration.
 *
 *  @return Returns STOP when test has finished executing, CONTINUE if more iterations are needed.
 */
tcu::TestNode::IterateResult ShaderDrawParametersExtensionTestCase::iterate()
{
	if (!m_context.getContextInfo().isExtensionSupported("GL_ARB_shader_draw_parameters"))
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "Not Supported");
		return STOP;
	}

	const Functions& gl = m_context.getRenderContext().getFunctions();

	std::string shader = sdp_compute_extensionCheck;

	ProgramSources sources;
	sources << ComputeSource(shader);
	ShaderProgram program(gl, sources);

	if (!program.isOk())
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
		m_testCtx.getLog() << tcu::TestLog::Message << "Checking shader preprocessor directives failed. Source:\n"
						   << shader.c_str() << "InfoLog:\n"
						   << program.getShaderInfo(SHADERTYPE_COMPUTE).infoLog << "\n"
						   << tcu::TestLog::EndMessage;
		return STOP;
	}

	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	return STOP;
}

/** Constructor.
 *
 *  @param context      Rendering context
 *  @param name         Test case name
 *  @param description  Test case description
 */
ShaderDrawParametersTestBase::ShaderDrawParametersTestBase(deqp::Context& context, const char* name,
														   const char* description)
	: TestCase(context, name, description)
	, m_vao(0)
	, m_arrayBuffer(0)
	, m_elementBuffer(0)
	, m_drawIndirectBuffer(0)
	, m_parameterBuffer(0)
{
	/* Left blank intentionally */
}

/** Stub init method */
void ShaderDrawParametersTestBase::init()
{
	glu::ContextType contextType = m_context.getRenderContext().getType();
	if (!glu::contextSupports(contextType, glu::ApiType::core(4, 6)) &&
		!m_context.getContextInfo().isExtensionSupported("GL_ARB_shader_draw_parameters"))
	{
		TCU_THROW(NotSupportedError, "shader_draw_parameters functionality not supported");
	}

	initChild();

	const Functions& gl = m_context.getRenderContext().getFunctions();

	const GLfloat vertices[] = {
		-1.0f, -1.0f, 0.0f, -1.0f, -0.8f, 0.0f, -0.9f, -1.0f, 0.0f,
		-0.9f, -0.8f, 0.0f, -0.8f, -1.0f, 0.0f, -0.8f, -0.8f, 0.0f,
	};

	const GLushort elements[] = { 0, 1, 2, 3, 4, 5 };

	// Generate vertex array object
	gl.genVertexArrays(1, &m_vao);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenVertexArrays");

	gl.bindVertexArray(m_vao);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindVertexArray");

	// Setup vertex array buffer
	gl.genBuffers(1, &m_arrayBuffer);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenBuffers");

	gl.bindBuffer(GL_ARRAY_BUFFER, m_arrayBuffer);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer");

	gl.bufferData(GL_ARRAY_BUFFER, 24 * sizeof(GLfloat), vertices, GL_STATIC_DRAW);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferData");

	// Setup element array buffer
	gl.genBuffers(1, &m_elementBuffer);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenBuffers");

	gl.bindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_elementBuffer);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer");

	gl.bufferData(GL_ELEMENT_ARRAY_BUFFER, 6 * sizeof(GLushort), elements, GL_STATIC_DRAW);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferData");
}

/** Stub deinit method */
void ShaderDrawParametersTestBase::deinit()
{
	const Functions& gl = m_context.getRenderContext().getFunctions();

	if (m_vao)
		gl.deleteVertexArrays(1, &m_vao);
	if (m_arrayBuffer)
		gl.deleteBuffers(1, &m_arrayBuffer);
	if (m_elementBuffer)
		gl.deleteBuffers(1, &m_elementBuffer);

	deinitChild();
}

/** Executes test iteration.
 *
 *  @return Returns STOP when test has finished executing, CONTINUE if more iterations are needed.
 */
tcu::TestNode::IterateResult ShaderDrawParametersTestBase::iterate()
{
	if (draw() && verify())
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	else
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");

	return STOP;
}

/** Draws scene using specific case parameters.
 *
 *  @return Returns true if no error occurred, false otherwise.
 */
bool ShaderDrawParametersTestBase::draw()
{
	const Functions& gl = m_context.getRenderContext().getFunctions();
	glu::ContextType contextType = m_context.getRenderContext().getType();
	std::map<std::string, std::string> specializationMap;

	if (glu::contextSupports(contextType, glu::ApiType::core(4, 6)))
	{
		specializationMap["VERSION"]				   = "#version 460";
		specializationMap["DRAW_PARAMETERS_EXTENSION"] = "";
		specializationMap["GL_BASE_VERTEX"]			   = "gl_BaseVertex";
		specializationMap["GL_BASE_INSTANCE"]		   = "gl_BaseInstance";
		specializationMap["GL_DRAW_ID"]				   = "gl_DrawID";
	}
	else
	{
		specializationMap["VERSION"]				   = "#version 450";
		specializationMap["DRAW_PARAMETERS_EXTENSION"] = "#extension GL_ARB_shader_draw_parameters : enable";
		specializationMap["GL_BASE_VERTEX"]			   = "gl_BaseVertexARB";
		specializationMap["GL_BASE_INSTANCE"]		   = "gl_BaseInstanceARB";
		specializationMap["GL_DRAW_ID"]				   = "gl_DrawIDARB";
	}

	std::string vs = tcu::StringTemplate(sdp_vertShader).specialize(specializationMap);
	std::string fs = tcu::StringTemplate(sdp_fragShader).specialize(specializationMap);

	ProgramSources sources = makeVtxFragSources(vs, fs);
	ShaderProgram  program(gl, sources);

	if (!program.isOk())
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Shader build failed.\n"
						   << "Vertex: " << program.getShaderInfo(SHADERTYPE_VERTEX).infoLog << "\n"
						   << "Fragment: " << program.getShaderInfo(SHADERTYPE_FRAGMENT).infoLog << "\n"
						   << "Program: " << program.getProgramInfo().infoLog << tcu::TestLog::EndMessage;
		return false;
	}

	gl.useProgram(program.getProgram());
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram");

	gl.clearColor(0.0f, 0.0f, 0.0f, 1.0f);
	gl.clear(GL_COLOR_BUFFER_BIT);

	gl.enable(GL_BLEND);
	gl.blendFunc(GL_ONE, GL_ONE);

	gl.enableVertexAttribArray(0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glEnableVertexAttribArray");
	gl.vertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glVertexAttribPointer");

	drawCommand();

	gl.disableVertexAttribArray(0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glDisableVertexAttribArray");

	gl.disable(GL_BLEND);

	return true;
}

/** Verifies if drawing result is as expected.
 *
 *  @return Returns true if verifying process has been successfully completed, false otherwise.
 */
bool ShaderDrawParametersTestBase::verify()
{
	const Functions&		 gl = m_context.getRenderContext().getFunctions();
	const tcu::RenderTarget& rt = m_context.getRenderContext().getRenderTarget();

	const int width  = rt.getWidth();
	const int height = rt.getHeight();

	std::vector<GLubyte> pixels;
	pixels.resize(width * height * 3);

	gl.pixelStorei(GL_PACK_ALIGNMENT, 1);
	gl.readPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());
	gl.pixelStorei(GL_PACK_ALIGNMENT, 4);

	std::vector<ResultPoint>::iterator it;
	for (it = m_resultPoints.begin(); it != m_resultPoints.end(); ++it)
	{
		int x	 = (int)(((it->x + 1.0f) / 2) * width);
		int y	 = (int)(((it->y + 1.0f) / 2) * height);
		int red   = (int)(it->red * 255);
		int green = (int)(it->green * 255);
		int blue  = (int)(it->blue * 255);

		if (pixels[(x + y * width) * 3 + 0] != red || pixels[(x + y * width) * 3 + 1] != green ||
			pixels[(x + y * width) * 3 + 2] != blue)
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "Verification failed (" << x << "/" << y << ")\n"
							   << "Expected point: (" << red << "," << green << "," << blue << ")\n"
							   << "Result point: (" << (int)pixels[(x + y * width) * 3 + 0] << ","
							   << (int)pixels[(x + y * width) * 3 + 1] << "," << (int)pixels[(x + y * width) * 3 + 2]
							   << ")\n"
							   << tcu::TestLog::EndMessage;
			return false;
		}
	}

	return true;
}

/** Child case initialization method.
 */
void ShaderDrawParametersTestBase::initChild()
{
	/* Do nothing */
}

/** Child case deinitialization method.
  */
void ShaderDrawParametersTestBase::deinitChild()
{
	/* Do nothing */
}

/** Child case drawing command invocation.
 */
void ShaderDrawParametersTestBase::drawCommand()
{
	/* Do nothing */
}

/* ShaderDrawArraysParametersTestCase */

void ShaderDrawArraysParametersTestCase::initChild()
{
	// Set expected result vector [x, y, red, green, blue]
	m_resultPoints.push_back(ResultPoint(-1.0f + 0.05f, -1.0f + 0.05f, 0.0f, 0.0f, 0.0f));
	m_resultPoints.push_back(ResultPoint(-1.0f + 0.05f, -1.0f + 0.15f, 1.0f, 0.0f, 0.0f));
	m_resultPoints.push_back(ResultPoint(-1.0f + 0.15f, -1.0f + 0.05f, 1.0f, 0.0f, 0.0f));
	m_resultPoints.push_back(ResultPoint(-1.0f + 0.15f, -1.0f + 0.15f, 0.0f, 0.0f, 0.0f));
}

void ShaderDrawArraysParametersTestCase::deinitChild()
{
	/* Do nothing */
}

void ShaderDrawArraysParametersTestCase::drawCommand()
{
	const Functions& gl = m_context.getRenderContext().getFunctions();

	gl.drawArrays(GL_TRIANGLE_STRIP, 1, 4);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawArrays");
}

/* ---------------------------------- */

/* ShaderDrawElementsParametersTestCase */

void ShaderDrawElementsParametersTestCase::initChild()
{
	// Set expected result vector [x, y, red, green, blue]
	m_resultPoints.push_back(ResultPoint(-1.0f + 0.05f, -1.0f + 0.05f, 0.0f, 0.0f, 0.0f));
	m_resultPoints.push_back(ResultPoint(-1.0f + 0.05f, -1.0f + 0.15f, 0.0f, 0.0f, 0.0f));
	m_resultPoints.push_back(ResultPoint(-1.0f + 0.15f, -1.0f + 0.05f, 1.0f, 0.0f, 0.0f));
	m_resultPoints.push_back(ResultPoint(-1.0f + 0.15f, -1.0f + 0.15f, 1.0f, 0.0f, 0.0f));
}

void ShaderDrawElementsParametersTestCase::deinitChild()
{
	/* Do nothing */
}

void ShaderDrawElementsParametersTestCase::drawCommand()
{
	const Functions& gl = m_context.getRenderContext().getFunctions();

	gl.drawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_SHORT, (GLvoid*)(2 * sizeof(GLushort)));
	GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawElements");
}

/* ---------------------------------- */

/* ShaderDrawArraysIndirectParametersTestCase */

void ShaderDrawArraysIndirectParametersTestCase::initChild()
{
	const Functions& gl = m_context.getRenderContext().getFunctions();

	// Set expected result vector [x, y, red, green, blue]
	m_resultPoints.push_back(ResultPoint(-1.0f + 0.05f, -1.0f + 0.05f, 1.0f, 0.0f, 0.0f));
	m_resultPoints.push_back(ResultPoint(-1.0f + 0.05f, -1.0f + 0.15f, 1.0f, 0.0f, 0.0f));
	m_resultPoints.push_back(ResultPoint(-1.0f + 0.15f, -1.0f + 0.05f, 1.0f, 0.0f, 0.0f));
	m_resultPoints.push_back(ResultPoint(-1.0f + 0.15f, -1.0f + 0.15f, 0.0f, 0.0f, 0.0f));

	const SDPDrawArraysIndirectCommand indirect[] = {
		{ 5, 1, 0, 0 },
	};

	// Setup indirect command buffer
	gl.genBuffers(1, &m_drawIndirectBuffer);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenBuffers");

	gl.bindBuffer(GL_DRAW_INDIRECT_BUFFER, m_drawIndirectBuffer);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer");

	gl.bufferData(GL_DRAW_INDIRECT_BUFFER, 1 * sizeof(SDPDrawArraysIndirectCommand), indirect, GL_STATIC_DRAW);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferData");
}

void ShaderDrawArraysIndirectParametersTestCase::deinitChild()
{
	const Functions& gl = m_context.getRenderContext().getFunctions();

	if (m_drawIndirectBuffer)
		gl.deleteBuffers(1, &m_drawIndirectBuffer);
}

void ShaderDrawArraysIndirectParametersTestCase::drawCommand()
{
	const Functions& gl = m_context.getRenderContext().getFunctions();

	gl.drawArraysIndirect(GL_TRIANGLE_STRIP, (GLvoid*)0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawArraysIndirect");
}

/* ---------------------------------- */

/* ShaderDrawElementsIndirectParametersTestCase */

void ShaderDrawElementsIndirectParametersTestCase::initChild()
{
	const Functions& gl = m_context.getRenderContext().getFunctions();

	// Set expected result vector [x, y, red, green, blue]
	m_resultPoints.push_back(ResultPoint(-1.0f + 0.05f, -1.0f + 0.05f, 0.0f, 0.0f, 0.0f));
	m_resultPoints.push_back(ResultPoint(-1.0f + 0.05f, -1.0f + 0.15f, 1.0f, 0.0f, 0.0f));
	m_resultPoints.push_back(ResultPoint(-1.0f + 0.15f, -1.0f + 0.05f, 1.0f, 0.0f, 0.0f));
	m_resultPoints.push_back(ResultPoint(-1.0f + 0.15f, -1.0f + 0.15f, 1.0f, 0.0f, 0.0f));

	const SDPDrawElementsIndirectCommand indirect[] = {
		{ 5, 1, 1, 0, 0 },
	};

	// Setup indirect command buffer
	gl.genBuffers(1, &m_drawIndirectBuffer);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenBuffers");

	gl.bindBuffer(GL_DRAW_INDIRECT_BUFFER, m_drawIndirectBuffer);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer");

	gl.bufferData(GL_DRAW_INDIRECT_BUFFER, 1 * sizeof(SDPDrawElementsIndirectCommand), indirect, GL_STATIC_DRAW);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferData");
}

void ShaderDrawElementsIndirectParametersTestCase::deinitChild()
{
	const Functions& gl = m_context.getRenderContext().getFunctions();

	if (m_drawIndirectBuffer)
		gl.deleteBuffers(1, &m_drawIndirectBuffer);
}

void ShaderDrawElementsIndirectParametersTestCase::drawCommand()
{
	const Functions& gl = m_context.getRenderContext().getFunctions();

	gl.drawElementsIndirect(GL_TRIANGLE_STRIP, GL_UNSIGNED_SHORT, (GLvoid*)0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawElementsIndirect");
}

/* ---------------------------------- */

/* ShaderDrawArraysInstancedParametersTestCase */

void ShaderDrawArraysInstancedParametersTestCase::initChild()
{
	// Set expected result vector [x, y, red, green, blue]
	m_resultPoints.push_back(ResultPoint(-1.0f + 0.05f, -1.0f + 0.05f, 0.0f, 0.0f, 0.0f));
	m_resultPoints.push_back(ResultPoint(-1.0f + 0.05f, -1.0f + 0.15f, 0.0f, 0.0f, 0.0f));
	m_resultPoints.push_back(ResultPoint(-1.0f + 0.15f, -1.0f + 0.05f, 1.0f, 0.0f, 0.0f));
	m_resultPoints.push_back(ResultPoint(-1.0f + 0.15f, -1.0f + 0.15f, 1.0f, 0.0f, 0.0f));
}

void ShaderDrawArraysInstancedParametersTestCase::deinitChild()
{
	/* Do nothing */
}

void ShaderDrawArraysInstancedParametersTestCase::drawCommand()
{
	const Functions& gl = m_context.getRenderContext().getFunctions();

	gl.drawArraysInstanced(GL_TRIANGLE_STRIP, 2, 4, 2);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawArraysInstanced");
}

/* ---------------------------------- */

/* ShaderDrawElementsInstancedParametersTestCase */

void ShaderDrawElementsInstancedParametersTestCase::initChild()
{
	// Set expected result vector [x, y, red, green, blue]
	m_resultPoints.push_back(ResultPoint(-1.0f + 0.05f, -1.0f + 0.05f, 1.0f, 0.0f, 0.0f));
	m_resultPoints.push_back(ResultPoint(-1.0f + 0.05f, -1.0f + 0.15f, 1.0f, 0.0f, 0.0f));
	m_resultPoints.push_back(ResultPoint(-1.0f + 0.15f, -1.0f + 0.05f, 1.0f, 0.0f, 0.0f));
	m_resultPoints.push_back(ResultPoint(-1.0f + 0.15f, -1.0f + 0.15f, 1.0f, 0.0f, 0.0f));
}

void ShaderDrawElementsInstancedParametersTestCase::deinitChild()
{
	/* Do nothing */
}

void ShaderDrawElementsInstancedParametersTestCase::drawCommand()
{
	const Functions& gl = m_context.getRenderContext().getFunctions();

	gl.drawElementsInstanced(GL_TRIANGLE_STRIP, 6, GL_UNSIGNED_SHORT, (GLvoid*)0, 3);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawElementsInstanced");
}

/* ---------------------------------- */

/* ShaderMultiDrawArraysParametersTestCase */

void ShaderMultiDrawArraysParametersTestCase::initChild()
{
	// Set expected result vector [x, y, red, green, blue]
	m_resultPoints.push_back(ResultPoint(-1.0f + 0.05f, -1.0f + 0.05f, 1.0f, 0.0f, 0.0f));
	m_resultPoints.push_back(ResultPoint(-1.0f + 0.05f, -1.0f + 0.15f, 1.0f, 1.0f, 0.0f));
	m_resultPoints.push_back(ResultPoint(-1.0f + 0.15f, -1.0f + 0.05f, 0.0f, 1.0f, 0.0f));
	m_resultPoints.push_back(ResultPoint(-1.0f + 0.15f, -1.0f + 0.15f, 0.0f, 0.0f, 0.0f));
}

void ShaderMultiDrawArraysParametersTestCase::deinitChild()
{
	/* Do nothing */
}

void ShaderMultiDrawArraysParametersTestCase::drawCommand()
{
	const Functions& gl = m_context.getRenderContext().getFunctions();

	const GLint   dFirst[] = { 0, 1 };
	const GLsizei dCount[] = { 4, 4 };

	gl.multiDrawArrays(GL_TRIANGLE_STRIP, dFirst, dCount, 2);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glMultiDrawArrays");
}

/* ---------------------------------- */

/* ShaderMultiDrawElementsParametersTestCase */

void ShaderMultiDrawElementsParametersTestCase::initChild()
{
	// Set expected result vector [x, y, red, green, blue]
	m_resultPoints.push_back(ResultPoint(-1.0f + 0.05f, -1.0f + 0.05f, 0.0f, 0.0f, 0.0f));
	m_resultPoints.push_back(ResultPoint(-1.0f + 0.05f, -1.0f + 0.15f, 1.0f, 1.0f, 0.0f));
	m_resultPoints.push_back(ResultPoint(-1.0f + 0.15f, -1.0f + 0.05f, 1.0f, 1.0f, 0.0f));
	m_resultPoints.push_back(ResultPoint(-1.0f + 0.15f, -1.0f + 0.15f, 1.0f, 0.0f, 0.0f));
}

void ShaderMultiDrawElementsParametersTestCase::deinitChild()
{
	/* Do nothing */
}

void ShaderMultiDrawElementsParametersTestCase::drawCommand()
{
	const Functions& gl = m_context.getRenderContext().getFunctions();

	const GLsizei dCount[]   = { 5, 4 };
	const GLvoid* dIndices[] = { (GLvoid*)(1 * sizeof(GLushort)), (GLvoid*)(1 * sizeof(GLushort)) };

	gl.multiDrawElements(GL_TRIANGLE_STRIP, dCount, GL_UNSIGNED_SHORT, dIndices, 2);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glMultiDrawElements");
}

/* ---------------------------------- */

/* ShaderMultiDrawArraysIndirectParametersTestCase */

void ShaderMultiDrawArraysIndirectParametersTestCase::initChild()
{
	const Functions& gl = m_context.getRenderContext().getFunctions();

	// Set expected result vector [x, y, red, green, blue]
	m_resultPoints.push_back(ResultPoint(-1.0f + 0.05f, -1.0f + 0.05f, 0.0f, 1.0f, 0.0f));
	m_resultPoints.push_back(ResultPoint(-1.0f + 0.05f, -1.0f + 0.15f, 1.0f, 1.0f, 0.0f));
	m_resultPoints.push_back(ResultPoint(-1.0f + 0.15f, -1.0f + 0.05f, 1.0f, 0.0f, 0.0f));
	m_resultPoints.push_back(ResultPoint(-1.0f + 0.15f, -1.0f + 0.15f, 1.0f, 0.0f, 0.0f));
	m_resultPoints.push_back(ResultPoint(-1.0f + 0.05f, -0.8f + 0.05f, 0.0f, 0.0f, 1.0f));
	m_resultPoints.push_back(ResultPoint(-1.0f + 0.05f, -0.8f + 0.15f, 0.0f, 0.0f, 0.0f));
	m_resultPoints.push_back(ResultPoint(-1.0f + 0.15f, -0.8f + 0.05f, 0.0f, 0.0f, 0.0f));
	m_resultPoints.push_back(ResultPoint(-1.0f + 0.15f, -0.8f + 0.15f, 0.0f, 0.0f, 0.0f));

	const SDPDrawArraysIndirectCommand indirect[] = {
		{ 5, 1, 1, 0 }, { 4, 1, 0, 0 }, { 3, 1, 0, 1 },
	};

	// Setup indirect command buffer
	gl.genBuffers(1, &m_drawIndirectBuffer);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenBuffers");

	gl.bindBuffer(GL_DRAW_INDIRECT_BUFFER, m_drawIndirectBuffer);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer");

	gl.bufferData(GL_DRAW_INDIRECT_BUFFER, 3 * sizeof(SDPDrawArraysIndirectCommand), indirect, GL_STATIC_DRAW);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferData");
}

void ShaderMultiDrawArraysIndirectParametersTestCase::deinitChild()
{
	const Functions& gl = m_context.getRenderContext().getFunctions();

	if (m_drawIndirectBuffer)
		gl.deleteBuffers(1, &m_drawIndirectBuffer);
}

void ShaderMultiDrawArraysIndirectParametersTestCase::drawCommand()
{
	const Functions& gl = m_context.getRenderContext().getFunctions();

	gl.multiDrawArraysIndirect(GL_TRIANGLE_STRIP, (GLvoid*)0, 3, sizeof(SDPDrawArraysIndirectCommand));
	GLU_EXPECT_NO_ERROR(gl.getError(), "glMultiDrawArraysIndirect");
}

/* ---------------------------------- */

/* ShaderMultiDrawElementsIndirectParametersTestCase */

void ShaderMultiDrawElementsIndirectParametersTestCase::initChild()
{
	const Functions& gl = m_context.getRenderContext().getFunctions();

	// Set expected result vector [x, y, red, green, blue]
	m_resultPoints.push_back(ResultPoint(-1.0f + 0.05f, -1.0f + 0.05f, 1.0f, 0.0f, 0.0f));
	m_resultPoints.push_back(ResultPoint(-1.0f + 0.05f, -1.0f + 0.15f, 1.0f, 0.0f, 0.0f));
	m_resultPoints.push_back(ResultPoint(-1.0f + 0.15f, -1.0f + 0.05f, 0.0f, 0.0f, 0.0f));
	m_resultPoints.push_back(ResultPoint(-1.0f + 0.15f, -1.0f + 0.15f, 0.0f, 0.0f, 0.0f));
	m_resultPoints.push_back(ResultPoint(-1.0f + 0.05f, -0.8f + 0.05f, 0.0f, 0.0f, 0.0f));
	m_resultPoints.push_back(ResultPoint(-1.0f + 0.05f, -0.8f + 0.15f, 0.0f, 0.0f, 0.0f));
	m_resultPoints.push_back(ResultPoint(-1.0f + 0.15f, -0.8f + 0.05f, 0.0f, 0.0f, 0.0f));
	m_resultPoints.push_back(ResultPoint(-1.0f + 0.15f, -0.8f + 0.15f, 0.0f, 1.0f, 0.0f));
	m_resultPoints.push_back(ResultPoint(-0.8f + 0.05f, -1.0f + 0.05f, 0.0f, 0.0f, 0.0f));
	m_resultPoints.push_back(ResultPoint(-0.8f + 0.05f, -1.0f + 0.15f, 0.0f, 0.0f, 1.0f));
	m_resultPoints.push_back(ResultPoint(-0.8f + 0.15f, -1.0f + 0.05f, 0.0f, 0.0f, 0.0f));
	m_resultPoints.push_back(ResultPoint(-0.8f + 0.15f, -1.0f + 0.15f, 0.0f, 0.0f, 0.0f));

	const SDPDrawElementsIndirectCommand indirect[] = {
		{ 4, 1, 0, 0, 0 }, { 3, 1, 3, 0, 1 }, { 3, 1, 0, 1, 0 },
	};

	// Setup indirect command buffer
	gl.genBuffers(1, &m_drawIndirectBuffer);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenBuffers");

	gl.bindBuffer(GL_DRAW_INDIRECT_BUFFER, m_drawIndirectBuffer);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer");

	gl.bufferData(GL_DRAW_INDIRECT_BUFFER, 3 * sizeof(SDPDrawElementsIndirectCommand), indirect, GL_STATIC_DRAW);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferData");
}

void ShaderMultiDrawElementsIndirectParametersTestCase::deinitChild()
{
	const Functions& gl = m_context.getRenderContext().getFunctions();

	if (m_drawIndirectBuffer)
		gl.deleteBuffers(1, &m_drawIndirectBuffer);
}

void ShaderMultiDrawElementsIndirectParametersTestCase::drawCommand()
{
	const Functions& gl = m_context.getRenderContext().getFunctions();

	gl.multiDrawElementsIndirect(GL_TRIANGLE_STRIP, GL_UNSIGNED_SHORT, (GLvoid*)0, 3,
								 sizeof(SDPDrawElementsIndirectCommand));
	GLU_EXPECT_NO_ERROR(gl.getError(), "glMultiDrawElementsIndirect");
}

/* ---------------------------------- */

/* ShaderMultiDrawArraysIndirectCountParametersTestCase */

void ShaderMultiDrawArraysIndirectCountParametersTestCase::initChild()
{
	glu::ContextType contextType = m_context.getRenderContext().getType();
	if (!glu::contextSupports(contextType, glu::ApiType::core(4, 6)) &&
		!m_context.getContextInfo().isExtensionSupported("GL_ARB_indirect_parameters"))
	{
		TCU_THROW(NotSupportedError, "indirect_parameters functionality not supported");
	}

	const Functions& gl = m_context.getRenderContext().getFunctions();

	// Set expected result vector [x, y, red, green, blue]
	m_resultPoints.push_back(ResultPoint(-1.0f + 0.05f, -1.0f + 0.05f, 0.0f, 0.0f, 0.0f));
	m_resultPoints.push_back(ResultPoint(-1.0f + 0.05f, -1.0f + 0.15f, 1.0f, 0.0f, 0.0f));
	m_resultPoints.push_back(ResultPoint(-1.0f + 0.15f, -1.0f + 0.05f, 1.0f, 0.0f, 0.0f));
	m_resultPoints.push_back(ResultPoint(-1.0f + 0.15f, -1.0f + 0.15f, 1.0f, 0.0f, 0.0f));
	m_resultPoints.push_back(ResultPoint(-1.0f + 0.05f, -0.8f + 0.05f, 0.0f, 1.0f, 0.0f));
	m_resultPoints.push_back(ResultPoint(-1.0f + 0.05f, -0.8f + 0.15f, 0.0f, 1.0f, 0.0f));
	m_resultPoints.push_back(ResultPoint(-1.0f + 0.15f, -0.8f + 0.05f, 0.0f, 1.0f, 0.0f));
	m_resultPoints.push_back(ResultPoint(-1.0f + 0.15f, -0.8f + 0.15f, 0.0f, 1.0f, 0.0f));
	m_resultPoints.push_back(ResultPoint(-1.0f + 0.05f, -0.6f + 0.05f, 0.0f, 0.0f, 0.0f));
	m_resultPoints.push_back(ResultPoint(-1.0f + 0.05f, -0.6f + 0.15f, 0.0f, 0.0f, 0.0f));
	m_resultPoints.push_back(ResultPoint(-1.0f + 0.15f, -0.6f + 0.05f, 0.0f, 0.0f, 1.0f));
	m_resultPoints.push_back(ResultPoint(-1.0f + 0.15f, -0.6f + 0.15f, 0.0f, 0.0f, 1.0f));

	const SDPDrawArraysIndirectCommand indirect[] = {
		{ 5, 1, 1, 0 }, { 6, 1, 0, 1 }, { 4, 1, 2, 2 },
	};

	const GLushort parameters[] = { 1, 1, 1 };

	// Setup indirect command buffer
	gl.genBuffers(1, &m_drawIndirectBuffer);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenBuffers");

	gl.bindBuffer(GL_DRAW_INDIRECT_BUFFER, m_drawIndirectBuffer);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer");

	gl.bufferData(GL_DRAW_INDIRECT_BUFFER, 4 * sizeof(SDPDrawArraysIndirectCommand), indirect, GL_STATIC_DRAW);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferData");

	// Setup indirect command buffer
	gl.genBuffers(1, &m_parameterBuffer);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenBuffers");

	gl.bindBuffer(GL_PARAMETER_BUFFER_ARB, m_parameterBuffer);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer");

	gl.bufferData(GL_PARAMETER_BUFFER_ARB, 3 * sizeof(GLushort), parameters, GL_STATIC_DRAW);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferData");
}

void ShaderMultiDrawArraysIndirectCountParametersTestCase::deinitChild()
{
	const Functions& gl = m_context.getRenderContext().getFunctions();

	if (m_drawIndirectBuffer)
		gl.deleteBuffers(1, &m_drawIndirectBuffer);
	if (m_parameterBuffer)
		gl.deleteBuffers(1, &m_parameterBuffer);
}

void ShaderMultiDrawArraysIndirectCountParametersTestCase::drawCommand()
{
	const Functions& gl = m_context.getRenderContext().getFunctions();

	gl.multiDrawArraysIndirectCount(GL_TRIANGLE_STRIP, 0, 0, 3, sizeof(SDPDrawArraysIndirectCommand));
	GLU_EXPECT_NO_ERROR(gl.getError(), "glMultiDrawArraysIndirect");
}

/* ---------------------------------- */

/* ShaderMultiDrawElementsIndirectCountParametersTestCase */

void ShaderMultiDrawElementsIndirectCountParametersTestCase::initChild()
{
	glu::ContextType contextType = m_context.getRenderContext().getType();
	if (!glu::contextSupports(contextType, glu::ApiType::core(4, 6)) &&
		!m_context.getContextInfo().isExtensionSupported("GL_ARB_indirect_parameters"))
	{
		TCU_THROW(NotSupportedError, "indirect_parameters functionality not supported");
	}

	const Functions& gl = m_context.getRenderContext().getFunctions();

	// Set expected result vector [x, y, red, green, blue]
	m_resultPoints.push_back(ResultPoint(-1.0f + 0.05f, -1.0f + 0.05f, 1.0f, 0.0f, 0.0f));
	m_resultPoints.push_back(ResultPoint(-1.0f + 0.05f, -1.0f + 0.15f, 0.0f, 0.0f, 0.0f));
	m_resultPoints.push_back(ResultPoint(-1.0f + 0.15f, -1.0f + 0.05f, 0.0f, 0.0f, 0.0f));
	m_resultPoints.push_back(ResultPoint(-1.0f + 0.15f, -1.0f + 0.15f, 0.0f, 0.0f, 0.0f));
	m_resultPoints.push_back(ResultPoint(-0.8f + 0.05f, -0.8f + 0.05f, 0.0f, 0.0f, 0.0f));
	m_resultPoints.push_back(ResultPoint(-0.8f + 0.05f, -0.8f + 0.15f, 0.0f, 1.0f, 1.0f));
	m_resultPoints.push_back(ResultPoint(-0.8f + 0.15f, -0.8f + 0.05f, 0.0f, 0.0f, 1.0f));
	m_resultPoints.push_back(ResultPoint(-0.8f + 0.15f, -0.8f + 0.15f, 0.0f, 0.0f, 0.0f));

	const SDPDrawElementsIndirectCommand indirect[] = {
		{ 3, 1, 0, 0, 0 }, { 3, 1, 0, 1, 1 }, { 4, 1, 0, 1, 1 },
	};

	const GLushort parameters[] = { 1, 1, 1 };

	// Setup indirect command buffer
	gl.genBuffers(1, &m_drawIndirectBuffer);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenBuffers");

	gl.bindBuffer(GL_DRAW_INDIRECT_BUFFER, m_drawIndirectBuffer);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer");

	gl.bufferData(GL_DRAW_INDIRECT_BUFFER, 3 * sizeof(SDPDrawElementsIndirectCommand), indirect, GL_STATIC_DRAW);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferData");

	// Setup indirect command buffer
	gl.genBuffers(1, &m_parameterBuffer);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenBuffers");

	gl.bindBuffer(GL_PARAMETER_BUFFER_ARB, m_parameterBuffer);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer");

	gl.bufferData(GL_PARAMETER_BUFFER_ARB, 3 * sizeof(GLushort), parameters, GL_STATIC_DRAW);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferData");
}

void ShaderMultiDrawElementsIndirectCountParametersTestCase::deinitChild()
{
	const Functions& gl = m_context.getRenderContext().getFunctions();

	if (m_drawIndirectBuffer)
		gl.deleteBuffers(1, &m_drawIndirectBuffer);
	if (m_parameterBuffer)
		gl.deleteBuffers(1, &m_parameterBuffer);
}

void ShaderMultiDrawElementsIndirectCountParametersTestCase::drawCommand()
{
	const Functions& gl = m_context.getRenderContext().getFunctions();

	gl.multiDrawElementsIndirectCount(GL_TRIANGLE_STRIP, GL_UNSIGNED_SHORT, 0, 0, 3,
										 sizeof(SDPDrawElementsIndirectCommand));
	GLU_EXPECT_NO_ERROR(gl.getError(), "glMultiDrawElementsIndirect");
}

/* ---------------------------------- */

/** Constructor.
 *
 *  @param context Rendering context.
 */
ShaderDrawParametersTests::ShaderDrawParametersTests(deqp::Context& context)
	: TestCaseGroup(context, "shader_draw_parameters_tests",
					"Verify conformance of GL_ARB_shader_draw_parameters implementation")
{
}

/** Initializes the test group contents. */
void ShaderDrawParametersTests::init()
{
	addChild(new ShaderDrawParametersExtensionTestCase(m_context));
	addChild(new ShaderDrawArraysParametersTestCase(m_context));
	addChild(new ShaderDrawElementsParametersTestCase(m_context));
	addChild(new ShaderDrawArraysIndirectParametersTestCase(m_context));
	addChild(new ShaderDrawElementsIndirectParametersTestCase(m_context));
	addChild(new ShaderDrawArraysInstancedParametersTestCase(m_context));
	addChild(new ShaderDrawElementsInstancedParametersTestCase(m_context));
	addChild(new ShaderMultiDrawArraysParametersTestCase(m_context));
	addChild(new ShaderMultiDrawElementsParametersTestCase(m_context));
	addChild(new ShaderMultiDrawArraysIndirectParametersTestCase(m_context));
	addChild(new ShaderMultiDrawElementsIndirectParametersTestCase(m_context));
	addChild(new ShaderMultiDrawArraysIndirectCountParametersTestCase(m_context));
	addChild(new ShaderMultiDrawElementsIndirectCountParametersTestCase(m_context));
}

} /* gl4cts namespace */
