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
* \file  glcPolygonOffsetClampTests.cpp
* \brief Conformance tests for the EXT_polygon_offset_clamp functionality.
*/ /*-------------------------------------------------------------------*/

#include "glcPolygonOffsetClampTests.hpp"
#include "gluContextInfo.hpp"
#include "gluShaderProgram.hpp"
#include "glwEnums.hpp"
#include "tcuRenderTarget.hpp"
#include "tcuTestLog.hpp"

#include <stdio.h>

using namespace glw;
using namespace glu;

namespace glcts
{

const char* poc_shader_version_450core = "#version 450 core\n\n";
const char* poc_shader_version_310es   = "#version 310 es\n\n";

const char* poc_vertexColor = "in highp vec3 vertex;\n"
							  "\n"
							  "void main()\n"
							  "{\n"
							  "    gl_Position = vec4(vertex, 1);\n"
							  "}\n";

const char* poc_fragmentColor = "out highp vec4 fragColor;\n"
								"\n"
								"void main()\n"
								"{\n"
								"    fragColor = vec4(1, 1, 1, 1);\n"
								"}\n";

const char* poc_vertexTexture = "in highp vec3 vertex;\n"
								"in highp vec2 texCoord;\n"
								"out highp vec2 varyingtexCoord;\n"
								"\n"
								"void main()\n"
								"{\n"
								"    gl_Position = vec4(vertex, 1);\n"
								"    varyingtexCoord = texCoord;\n"
								"}\n";

const char* poc_fragmentTexture = "in highp vec2 varyingtexCoord;\n"
								  "out highp vec4 fragColor;\n"
								  "\n"
								  "layout (location = 0) uniform highp sampler2D tex;\n"
								  "\n"
								  "void main()\n"
								  "{\n"
								  "    highp vec4 v = texture(tex, varyingtexCoord);\n"
								  "    int r = int(v.r * 65536.0) % 256;\n"
								  "    int g = int(v.r * 65536.0) / 256;\n"
								  "    fragColor = vec4(float(r) / 255.0, float(g) / 255.0, 0.0, 1.0);\n"
								  "}\n";

/** Constructor.
*
*  @param context Rendering context
*  @param name Test name
*  @param description Test description
*/
PolygonOffsetClampTestCaseBase::PolygonOffsetClampTestCaseBase(deqp::Context& context, const char* name,
															   const char* description)
	: TestCase(context, name, description)
{
	glu::ContextType contextType = m_context.getRenderContext().getType();
	m_extensionSupported		 = glu::contextSupports(contextType, glu::ApiType::core(4, 6));
	m_extensionSupported |= context.getContextInfo().isExtensionSupported("GL_EXT_polygon_offset_clamp");
	m_extensionSupported |= context.getContextInfo().isExtensionSupported("GL_ARB_polygon_offset_clamp");
}

tcu::TestNode::IterateResult PolygonOffsetClampTestCaseBase::iterate()
{
	if (!m_extensionSupported)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "Not supported");
		return STOP;
	}

	test(m_context.getRenderContext().getFunctions());

	return STOP;
}

/** Constructor.
*
*  @param context Rendering context
*/
PolygonOffsetClampAvailabilityTestCase::PolygonOffsetClampAvailabilityTestCase(deqp::Context& context)
	: PolygonOffsetClampTestCaseBase(context, "PolygonOffsetClampAvailability",
									 "Verifies if queries for GL_EXT_polygon_offset_clamp extension works properly")
{
}

void PolygonOffsetClampAvailabilityTestCase::test(const glw::Functions& gl)
{
	{
		glw::GLboolean data;
		gl.getBooleanv(GL_POLYGON_OFFSET_CLAMP_EXT, &data);
		GLU_EXPECT_NO_ERROR(gl.getError(), "getBooleanv error occurred");
	}
	{
		glw::GLint data;
		gl.getIntegerv(GL_POLYGON_OFFSET_CLAMP_EXT, &data);
		GLU_EXPECT_NO_ERROR(gl.getError(), "getBooleanv error occurred");
	}
	{
		glw::GLint64 data;
		gl.getInteger64v(GL_POLYGON_OFFSET_CLAMP_EXT, &data);
		GLU_EXPECT_NO_ERROR(gl.getError(), "getBooleanv error occurred");
	}
	{
		glw::GLfloat data;
		gl.getFloatv(GL_POLYGON_OFFSET_CLAMP_EXT, &data);
		GLU_EXPECT_NO_ERROR(gl.getError(), "getBooleanv error occurred");
	}

	// OpenGL ES does not support getDoublev query
	if (glu::isContextTypeGLCore(m_context.getRenderContext().getType()))
	{
		glw::GLdouble data;
		gl.getDoublev(GL_POLYGON_OFFSET_CLAMP_EXT, &data);
		GLU_EXPECT_NO_ERROR(gl.getError(), "getBooleanv error occurred");
	}

	gl.polygonOffsetClamp(1.0f, 1.0f, 0.5f);
	GLU_EXPECT_NO_ERROR(gl.getError(), "polygonOffsetClamp error occurred");

	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
}

/** Constructor.
*
*  @param context Rendering context
*/
PolygonOffsetClampValueTestCaseBase::PolygonOffsetClampValueTestCaseBase(deqp::Context& context, const char* name,
																		 const char* description)
	: PolygonOffsetClampTestCaseBase(context, name, description)
	, m_fbo(0)
	, m_depthBuf(0)
	, m_colorBuf(0)
	, m_fboReadback(0)
	, m_colorBufReadback(0)
{
}

/** Initialization method that creates framebuffer with depth attachment
 */
void PolygonOffsetClampValueTestCaseBase::init()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.genTextures(1, &m_depthBuf);
	GLU_EXPECT_NO_ERROR(gl.getError(), "genTextures");
	gl.bindTexture(GL_TEXTURE_2D, m_depthBuf);
	GLU_EXPECT_NO_ERROR(gl.getError(), "bindTexture");
	gl.texStorage2D(GL_TEXTURE_2D, 1, GL_DEPTH_COMPONENT16, 64, 64);
	GLU_EXPECT_NO_ERROR(gl.getError(), "texStorage2D");
	gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	GLU_EXPECT_NO_ERROR(gl.getError(), "texParameteri");
	gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	GLU_EXPECT_NO_ERROR(gl.getError(), "texParameteri");

	gl.genTextures(1, &m_colorBuf);
	GLU_EXPECT_NO_ERROR(gl.getError(), "genTextures");
	gl.bindTexture(GL_TEXTURE_2D, m_colorBuf);
	GLU_EXPECT_NO_ERROR(gl.getError(), "bindTexture");
	gl.texStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, 64, 64);
	GLU_EXPECT_NO_ERROR(gl.getError(), "texStorage2D");
	gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	GLU_EXPECT_NO_ERROR(gl.getError(), "texParameteri");
	gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	GLU_EXPECT_NO_ERROR(gl.getError(), "texParameteri");

	gl.genFramebuffers(1, &m_fbo);
	GLU_EXPECT_NO_ERROR(gl.getError(), "genFramebuffers");
	gl.bindFramebuffer(GL_FRAMEBUFFER, m_fbo);
	GLU_EXPECT_NO_ERROR(gl.getError(), "bindFramebuffer");
	gl.framebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_depthBuf, 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "framebufferTexture2D");
	gl.framebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_colorBuf, 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "framebufferTexture2D");

	if (!glu::isContextTypeGLCore(m_context.getRenderContext().getType()))
	{
		gl.genTextures(1, &m_colorBufReadback);
		GLU_EXPECT_NO_ERROR(gl.getError(), "genTextures");
		gl.bindTexture(GL_TEXTURE_2D, m_colorBufReadback);
		GLU_EXPECT_NO_ERROR(gl.getError(), "bindTexture");
		gl.texStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, 64, 64);
		GLU_EXPECT_NO_ERROR(gl.getError(), "texStorage2D");
		gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		GLU_EXPECT_NO_ERROR(gl.getError(), "texParameteri");
		gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		GLU_EXPECT_NO_ERROR(gl.getError(), "texParameteri");

		gl.genFramebuffers(1, &m_fboReadback);
		GLU_EXPECT_NO_ERROR(gl.getError(), "genFramebuffers");
		gl.bindFramebuffer(GL_FRAMEBUFFER, m_fboReadback);
		GLU_EXPECT_NO_ERROR(gl.getError(), "bindFramebuffer");
		gl.framebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_colorBufReadback, 0);
		GLU_EXPECT_NO_ERROR(gl.getError(), "framebufferTexture2D");
	}

	gl.viewport(0, 0, 64, 64);
}

/** De-Initialization method that releases
 */
void PolygonOffsetClampValueTestCaseBase::deinit()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.bindFramebuffer(GL_FRAMEBUFFER, 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "bindFramebuffer");

	if (m_fbo)
		gl.deleteFramebuffers(1, &m_fbo);
	if (m_depthBuf)
		gl.deleteTextures(1, &m_depthBuf);
	if (m_colorBuf)
		gl.deleteTextures(1, &m_colorBuf);

	if (!glu::isContextTypeGLCore(m_context.getRenderContext().getType()))
	{
		if (m_colorBufReadback)
			gl.deleteTextures(1, &m_colorBufReadback);
		if (m_fboReadback)
			gl.deleteFramebuffers(1, &m_fboReadback);
	}
}

/** Testing method that verifies if depth values generated after polygon offset clamp are as expected.
 *
 *  @param gl   Function bindings
 */
void PolygonOffsetClampValueTestCaseBase::test(const glw::Functions& gl)
{
	const GLfloat vertices[] = { -1.0f, -1.0f, 0.5f, -1.0f, 1.0f, 0.5f, 1.0f, -1.0f, 0.5f, 1.0f, 1.0f, 0.5f };

	// Prepare shader program
	std::string vertexColor;
	std::string fragmentColor;
	if (glu::isContextTypeGLCore(m_context.getRenderContext().getType()))
		vertexColor = std::string(poc_shader_version_450core);
	else
		vertexColor = std::string(poc_shader_version_310es);
	fragmentColor   = vertexColor;

	vertexColor   = vertexColor + poc_vertexColor;
	fragmentColor = fragmentColor + poc_fragmentColor;

	ProgramSources testSources = makeVtxFragSources(vertexColor, fragmentColor);
	ShaderProgram  testProgram(gl, testSources);

	if (!testProgram.isOk())
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "TestProgram build failed.\n"
						   << "Vertex: " << testProgram.getShaderInfo(SHADERTYPE_VERTEX).infoLog << "\n"
						   << "Fragment: " << testProgram.getShaderInfo(SHADERTYPE_FRAGMENT).infoLog << "\n"
						   << "Program: " << testProgram.getProgramInfo().infoLog << tcu::TestLog::EndMessage;

		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
		return;
	}

	ShaderProgram* readDepthProgram   = DE_NULL;
	GLuint		   readDepthProgramId = 0;

	// Prepare shader program for reading depth buffer indirectly
	if (!glu::isContextTypeGLCore(m_context.getRenderContext().getType()))
	{
		std::string vertexTexture   = std::string(poc_shader_version_310es) + poc_vertexTexture;
		std::string fragmentTexture = std::string(poc_shader_version_310es) + poc_fragmentTexture;

		ProgramSources readDepthSources = makeVtxFragSources(vertexTexture, fragmentTexture);

		readDepthProgram = new ShaderProgram(gl, readDepthSources);

		if (!readDepthProgram->isOk())
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "ReadDepthProgram build failed.\n"
							   << "Vertex: " << readDepthProgram->getShaderInfo(SHADERTYPE_VERTEX).infoLog << "\n"
							   << "Fragment: " << readDepthProgram->getShaderInfo(SHADERTYPE_FRAGMENT).infoLog << "\n"
							   << "Program: " << readDepthProgram->getProgramInfo().infoLog << tcu::TestLog::EndMessage;

			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
			return;
		}

		readDepthProgramId = readDepthProgram->getProgram();
	}

	gl.useProgram(testProgram.getProgram());
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram");

	GLuint vao;
	GLuint arrayBuffer;

	// Setup depth testing
	gl.enable(GL_DEPTH_TEST);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glEnable");

	gl.depthFunc(GL_ALWAYS);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glDepthFunc");

	// Generate vertex array object
	gl.genVertexArrays(1, &vao);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenVertexArrays");

	gl.bindVertexArray(vao);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindVertexArray");

	// Setup vertex array buffer
	gl.genBuffers(1, &arrayBuffer);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenBuffers");

	gl.bindBuffer(GL_ARRAY_BUFFER, arrayBuffer);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer");

	gl.bufferData(GL_ARRAY_BUFFER, 12 * sizeof(GLfloat), vertices, GL_STATIC_DRAW);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferData");

	// Setup vertex attrib pointer
	gl.enableVertexAttribArray(0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glEnableVertexAttribArray");

	gl.vertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glVertexAttribPointer");

	// Bind framebuffer for drawing
	gl.bindFramebuffer(GL_FRAMEBUFFER, m_fbo);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffer");

	bool result = true;
	for (GLuint i = 0; i < m_testValues.size(); ++i)
	{
		// Prepare verification variables
		GLfloat depthValue			  = 0.0f;
		GLfloat depthValueOffset	  = 0.0f;
		GLfloat depthValueOffsetClamp = 0.0f;

		// Draw reference polygon
		gl.disable(GL_POLYGON_OFFSET_FILL);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glDisable");

		gl.clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glClear");

		gl.drawArrays(GL_TRIANGLE_STRIP, 0, 4);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawArrays");

		// Get reference depth value
		depthValue = readDepthValue(gl, readDepthProgramId);

		// Draw polygon with depth offset
		gl.enable(GL_POLYGON_OFFSET_FILL);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glEnable");

		gl.bindFramebuffer(GL_FRAMEBUFFER, m_fbo);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffer");

		gl.polygonOffset(m_testValues[i].factor, m_testValues[i].units);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glPolygonOffset");

		gl.drawArrays(GL_TRIANGLE_STRIP, 0, 4);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawArrays");

		depthValueOffset = readDepthValue(gl, readDepthProgramId);

		// Draw reference polygon
		gl.disable(GL_POLYGON_OFFSET_FILL);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glDisable");

		gl.bindFramebuffer(GL_FRAMEBUFFER, m_fbo);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffer");

		gl.clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glClear");

		gl.drawArrays(GL_TRIANGLE_STRIP, 0, 4);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawArrays");

		// Draw polygon with depth offset
		gl.enable(GL_POLYGON_OFFSET_FILL);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glEnable");

		gl.polygonOffsetClamp(m_testValues[i].factor, m_testValues[i].units, m_testValues[i].clamp);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glPolygonOffsetClamp");

		gl.drawArrays(GL_TRIANGLE_STRIP, 0, 4);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawArrays");

		depthValueOffsetClamp = readDepthValue(gl, readDepthProgramId);

		// Verify results
		result = result && verify(i, depthValue, depthValueOffset, depthValueOffsetClamp);
	}

	// Cleanup
	gl.disableVertexAttribArray(0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glDisableVertexAttribArray");

	gl.deleteVertexArrays(1, &arrayBuffer);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glDeleteVertexArrays");

	gl.deleteVertexArrays(1, &vao);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glDeleteVertexArrays");

	gl.disable(GL_POLYGON_OFFSET_FILL);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glDisable");

	if (readDepthProgram)
		delete readDepthProgram;

	if (result)
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	else
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
}

/** Method .
 *
 *  @param gl   Function bindings
 */
float PolygonOffsetClampValueTestCaseBase::readDepthValue(const glw::Functions& gl, const GLuint readDepthProgramId)
{
	GLfloat depthValue = 0.0f;

	if (glu::isContextTypeGLCore(m_context.getRenderContext().getType()))
	{
		gl.readPixels(0, 0, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &depthValue);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glReadPixels");
	}
	// OpenGL ES does not support reading pixels directly from depth buffer
	else
	{
		// Bind framebuffer for readback
		gl.bindFramebuffer(GL_FRAMEBUFFER, m_fboReadback);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffer");

		gl.disable(GL_DEPTH_TEST);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glDisable");

		gl.useProgram(readDepthProgramId);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram");

		gl.activeTexture(GL_TEXTURE0);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glActiveTexture");
		gl.bindTexture(GL_TEXTURE_2D, m_depthBuf);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture");
		gl.uniform1i(0, 0);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glUniform1i");

		gl.drawArrays(GL_TRIANGLE_STRIP, 0, 4);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawArrays");

		GLubyte pixels[4];
		gl.readPixels(0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glReadPixels");

		gl.enable(GL_DEPTH_TEST);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glEnable");

		// Convert read depth value to GLfloat normalized
		depthValue = (GLfloat)(pixels[0] + pixels[1] * 256) / 0xFFFF;

		// Bind framebuffer for drawing
		gl.bindFramebuffer(GL_FRAMEBUFFER, m_fbo);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glBindFramebuffer");
	}

	return depthValue;
}

/** Constructor.
*
*  @param context Rendering context
*/
PolygonOffsetClampMinMaxTestCase::PolygonOffsetClampMinMaxTestCase(deqp::Context& context)
	: PolygonOffsetClampValueTestCaseBase(
		  context, "PolygonOffsetClampMinMax",
		  "Verifies if polygon offset clamp works as expected for non-zero, finite clamp values")
{
}

/** Initialization method that fills polygonOffset* testing values
 */
void PolygonOffsetClampMinMaxTestCase::init()
{
	PolygonOffsetClampValueTestCaseBase::init();

	m_testValues.clear();
	m_testValues.push_back(PolygonOffsetClampValues(0.0f, -1000.0f, -0.0001f)); // Min offset case
	m_testValues.push_back(PolygonOffsetClampValues(0.0f, 1000.0f, 0.0001f));   // Max offset case
}

/** Verification method that determines if depth values are as expected
 *
 *  @param caseNo           Case iteration number
 *  @param depth            Reference depth value
 *  @param offsetDepth      Case iteration number
 *  @param offsetClampDepth Case iteration number
 */
bool PolygonOffsetClampMinMaxTestCase::verify(GLuint caseNo, GLfloat depth, GLfloat offsetDepth,
											  GLfloat offsetClampDepth)
{
	// Min offset case
	if (caseNo == 0)
	{
		if (depth <= offsetDepth || depth <= offsetClampDepth || offsetDepth >= offsetClampDepth)
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "PolygonOffsetClampEXT failed at MIN offset test.\n"
							   << "Expected result: "
							   << "refDepth[" << depth << "] > "
							   << "offsetClampDepth[" << offsetClampDepth << "] > "
							   << "offsetDepth[" << offsetDepth << "]" << tcu::TestLog::EndMessage;

			return false;
		}
	}
	// Max offset case
	else if (caseNo == 1)
	{
		if (depth >= offsetDepth || depth >= offsetClampDepth || offsetDepth <= offsetClampDepth)
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "PolygonOffsetClampEXT failed at MAX offset test.\n"
							   << "Expected result: "
							   << "refDepth[" << depth << "] < "
							   << "offsetClampDepth[" << offsetClampDepth << "] < "
							   << "offsetDepth[" << offsetDepth << "]" << tcu::TestLog::EndMessage;

			return false;
		}
	}
	// Undefined case
	else
		return false;

	return true;
}

/** Constructor.
*
*  @param context Rendering context
*/
PolygonOffsetClampZeroInfinityTestCase::PolygonOffsetClampZeroInfinityTestCase(deqp::Context& context)
	: PolygonOffsetClampValueTestCaseBase(
		  context, "PolygonOffsetClampZeroInfinity",
		  "Verifies if polygon offset clamp works as expected for zero and infinite clamp values")
{
}

/** Initialization method that fills polygonOffset* testing values
 */
void PolygonOffsetClampZeroInfinityTestCase::init()
{
	PolygonOffsetClampValueTestCaseBase::init();

	m_testValues.clear();
	m_testValues.push_back(PolygonOffsetClampValues(0.0f, -1000.0f, 0.0f));		 // Min offset, zero clamp case
	m_testValues.push_back(PolygonOffsetClampValues(0.0f, -1000.0f, -INFINITY)); // Min Offset, infinity clamp case
	m_testValues.push_back(PolygonOffsetClampValues(0.0f, 1000.0f, 0.0f));		 // Max offset, zero clamp case
	m_testValues.push_back(PolygonOffsetClampValues(0.0f, 1000.0f, INFINITY));   // Max Offset, infinity clamp case
}

bool PolygonOffsetClampZeroInfinityTestCase::verify(GLuint caseNo, GLfloat depth, GLfloat offsetDepth,
													GLfloat offsetClampDepth)
{
	DE_UNREF(caseNo);

	if (depth == offsetDepth || depth == offsetClampDepth || offsetDepth != offsetClampDepth)
	{
		m_testCtx.getLog() << tcu::TestLog::Message
						   << "PolygonOffsetClampEXT failed at Zero/Infinity offset clamp test.\n"
						   << "Expected result: "
						   << "refDepth[" << depth << "] != "
						   << "(offsetClampDepth[" << offsetClampDepth << "] == "
						   << "offsetDepth[" << offsetDepth << "])" << tcu::TestLog::EndMessage;

		return false;
	}

	return true;
}

/** Constructor.
*
*  @param context Rendering context.
*/
PolygonOffsetClamp::PolygonOffsetClamp(deqp::Context& context)
	: TestCaseGroup(context, "polygon_offset_clamp",
					"Verify conformance of CTS_EXT_polygon_offset_clamp implementation")
{
}

/** Initializes the test group contents. */
void PolygonOffsetClamp::init()
{
	addChild(new PolygonOffsetClampAvailabilityTestCase(m_context));
	addChild(new PolygonOffsetClampMinMaxTestCase(m_context));
	addChild(new PolygonOffsetClampZeroInfinityTestCase(m_context));
}
} /* glcts namespace */
