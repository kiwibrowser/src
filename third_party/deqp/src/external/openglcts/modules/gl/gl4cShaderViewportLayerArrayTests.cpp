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
 * \file
 * \brief
 */ /*-------------------------------------------------------------------*/

/**
 */ /*!
 * \file  gl4cShaderViewportLayerArrayTests.cpp
 * \brief Conformance tests for the ARB_shader_viewport_layer_array functionality.
 */ /*-------------------------------------------------------------------*/

#include "gl4cShaderViewportLayerArrayTests.hpp"
#include "gluContextInfo.hpp"
#include "gluDefs.hpp"
#include "gluDrawUtil.hpp"
#include "gluObjectWrapper.hpp"
#include "gluShaderProgram.hpp"
#include "glwEnums.hpp"
#include "glwFunctions.hpp"
#include "tcuRenderTarget.hpp"

#include <sstream>
#include <string>
#include <vector>

using namespace glw;

namespace gl4cts
{

ShaderViewportLayerArrayUtils::ShaderPipeline::ShaderPipeline(bool tessellationShader, bool geometryShader,
															  int maxViewportsLayers, const std::string& varName)
	: m_program(NULL)
	, m_hasTessellationShader(tessellationShader)
	, m_hasGeometryShader(geometryShader)
	, m_viewportLayerOffset(m_hasGeometryShader ? OFFSET_GEOMETRY :
												  m_hasTessellationShader ? OFFSET_TESSELLATION : OFFSET_VERTEX)
	, m_varName(varName)
{
	m_vs = "#version 450 core\n"
		   "#extension GL_ARB_shader_viewport_layer_array: require\n"
		   "in highp vec2 inPosition;\n"
		   "in int in<var_name>;\n"
		   "in highp vec4 inColor;\n"
		   "out int vs<var_name>;\n"
		   "out highp vec3 vsPosition;\n"
		   "out highp vec4 vsColor;\n"
		   "void main()\n"
		   "{\n"
		   "	gl_Position = vec4(inPosition, 0.0, 1.0);\n"
		   "	gl_<var_name> = (in<var_name> + <viewport_layer_offset>) % <viewport_layer_max>;\n"
		   "	vs<var_name> = in<var_name>;\n"
		   "	vsPosition = vec3(inPosition, 0.0);\n"
		   "	vsColor = inColor;\n"
		   "}\n";

	m_tcs = "#version 450 core\n"
			"layout(vertices = 3) out;\n"
			"in highp vec3 vsPosition[];\n"
			"in highp vec4 vsColor[];\n"
			"in int vs<var_name>[];\n"
			"out highp vec3 tcsPosition[];\n"
			"out highp vec4 tcsColor[];\n"
			"out int tcs<var_name>[];\n"
			"void main()\n"
			"{\n"
			"	tcsPosition[gl_InvocationID] = vsPosition[gl_InvocationID];\n"
			"	tcsColor[gl_InvocationID] = vsColor[gl_InvocationID];\n"
			"	tcs<var_name>[gl_InvocationID] = vs<var_name>[gl_InvocationID];\n"
			"	gl_TessLevelInner[0] = 3;\n"
			"	gl_TessLevelOuter[0] = 3;\n"
			"	gl_TessLevelOuter[1] = 3;\n"
			"	gl_TessLevelOuter[2] = 3;\n"
			"}\n";

	m_tes = "#version 450 core\n"
			"#extension GL_ARB_shader_viewport_layer_array: require\n"
			"layout(triangles, equal_spacing, cw) in;\n"
			"in highp vec3 tcsPosition[];\n"
			"in highp vec4 tcsColor[];\n"
			"in int tcs<var_name>[];\n"
			"out highp vec4 tesColor;\n"
			"out highp int tes<var_name>;\n"
			"void main()\n"
			"{\n"
			"	vec3 p0 = gl_TessCoord.x * tcsPosition[0];\n"
			"	vec3 p1 = gl_TessCoord.y * tcsPosition[1];\n"
			"	vec3 p2 = gl_TessCoord.z * tcsPosition[2];\n"
			"	tesColor = tcsColor[0];\n"
			"	tes<var_name> = tcs<var_name>[0];\n"
			"	gl_<var_name> = (tcs<var_name>[0] + <viewport_layer_offset>) % <viewport_layer_max>;\n"
			"	gl_Position = vec4(normalize(p0 + p1 + p2), 1.0);\n"
			"}\n";

	m_gs = "#version 450 core\n"
		   "#extension GL_ARB_shader_viewport_layer_array: require\n"
		   "layout(triangles) in;\n"
		   "layout(triangle_strip, max_vertices = 3) out;\n"
		   "in highp vec4 tesColor[];\n"
		   "in int tes<var_name>[];\n"
		   "out highp vec4 gsColor;\n"
		   "void main()\n"
		   "{\n"
		   "	for (int i = 0; i<3; i++)\n"
		   "	{\n"
		   "		gl_Position = gl_in[i].gl_Position;\n"
		   "		gl_<var_name> = (tes<var_name>[i] + <viewport_layer_offset>) % <viewport_layer_max>;\n"
		   "		gsColor = tesColor[i];\n"
		   "		EmitVertex();\n"
		   "	}\n"
		   "	EndPrimitive();\n"
		   "}\n";

	m_fs = "#version 450 core\n"
		   "in highp vec4 <input_color>;\n"
		   "out vec4 finalOutColor;\n"
		   "void main()\n"
		   "{\n"
		   "	finalOutColor = <input_color>;\n"
		   "}\n";

	this->adaptShaderToPipeline(m_vs, "<var_name>", varName);
	this->adaptShaderToPipeline(m_vs, "<viewport_layer_offset>", OFFSET_VERTEX);
	this->adaptShaderToPipeline(m_vs, "<viewport_layer_max>", maxViewportsLayers);

	this->adaptShaderToPipeline(m_tes, "<var_name>", varName);
	this->adaptShaderToPipeline(m_tes, "<viewport_layer_offset>", OFFSET_TESSELLATION);
	this->adaptShaderToPipeline(m_tes, "<viewport_layer_max>", maxViewportsLayers);

	this->adaptShaderToPipeline(m_tcs, "<var_name>", varName);

	this->adaptShaderToPipeline(m_gs, "<var_name>", varName);
	this->adaptShaderToPipeline(m_gs, "<viewport_layer_offset>", OFFSET_GEOMETRY);
	this->adaptShaderToPipeline(m_gs, "<viewport_layer_max>", maxViewportsLayers);

	this->adaptShaderToPipeline(m_fs, "<input_color>", "vsColor", "tesColor", "gsColor");
}

void ShaderViewportLayerArrayUtils::ShaderPipeline::adaptShaderToPipeline(std::string&		 shader,
																		  const std::string& varKey,
																		  const std::string& vsVersion,
																		  const std::string& tesVersion,
																		  const std::string& gsVersion)
{
	std::string varName = m_hasGeometryShader ? gsVersion : m_hasTessellationShader ? tesVersion : vsVersion;

	size_t start = 0;
	while ((start = shader.find(varKey, start)) != std::string::npos)
	{
		shader.replace(start, varKey.length(), varName);
		start += varName.length();
	}
}

void ShaderViewportLayerArrayUtils::ShaderPipeline::adaptShaderToPipeline(std::string&		 shader,
																		  const std::string& varKey,
																		  const std::string& value)
{
	this->adaptShaderToPipeline(shader, varKey, value, value, value);
}

void ShaderViewportLayerArrayUtils::ShaderPipeline::adaptShaderToPipeline(std::string&		 shader,
																		  const std::string& varKey, int value)
{
	std::ostringstream valueStr;
	valueStr << value;

	this->adaptShaderToPipeline(shader, varKey, valueStr.str(), valueStr.str(), valueStr.str());
}

ShaderViewportLayerArrayUtils::ShaderPipeline::~ShaderPipeline()
{
	if (m_program)
	{
		delete m_program;
	}
}

void ShaderViewportLayerArrayUtils::ShaderPipeline::create(const glu::RenderContext& context)
{
	glu::ProgramSources sources;
	sources.sources[glu::SHADERTYPE_VERTEX].push_back(m_vs);
	if (m_hasTessellationShader)
	{
		sources.sources[glu::SHADERTYPE_TESSELLATION_CONTROL].push_back(m_tcs);
		sources.sources[glu::SHADERTYPE_TESSELLATION_EVALUATION].push_back(m_tes);
	}
	if (m_hasGeometryShader)
	{
		sources.sources[glu::SHADERTYPE_GEOMETRY].push_back(m_gs);
	}
	sources.sources[glu::SHADERTYPE_FRAGMENT].push_back(m_fs);

	m_program = new glu::ShaderProgram(context, sources);
	if (!m_program->isOk())
	{
		TCU_FAIL("Shader compilation failed");
	}
}

void ShaderViewportLayerArrayUtils::ShaderPipeline::use(const glu::RenderContext& context)
{
	const glw::Functions& gl = context.getFunctions();
	gl.useProgram(m_program->getProgram());
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram failed");
}

void ShaderViewportLayerArrayUtils::renderQuad(const glu::RenderContext& context, ShaderPipeline& shaderPipeline,
											   int viewportLayerIndex, tcu::Vec4 color)
{
	const glw::Functions& gl = context.getFunctions();

	deUint16 const quadIndices[] = { 0, 1, 2, 2, 1, 3 };

	float const position[] = { -1.0f, -1.0f, -1.0f, 1.0f, 1.0f, -1.0f, 1.0f, 1.0f };

	int const viewportLayerIndices[] = { viewportLayerIndex, viewportLayerIndex, viewportLayerIndex,
										 viewportLayerIndex };

	float const colors[] = { color.x(), color.y(), color.z(), color.w(), color.x(), color.y(), color.z(), color.w(),
							 color.x(), color.y(), color.z(), color.w(), color.x(), color.y(), color.z(), color.w() };

	std::string varName = "in";
	varName += shaderPipeline.getVarName();

	glu::VertexArrayBinding vertexArrays[] = { glu::va::Float("inPosition", 2, 4, 0, position),
											   glu::va::Int32(varName, 1, 4, 0, viewportLayerIndices),
											   glu::va::Float("inColor", 4, 4, 0, colors) };

	shaderPipeline.use(context);

	glu::PrimitiveList primitiveList = shaderPipeline.hasTessellationShader() ?
										   glu::pr::Patches(DE_LENGTH_OF_ARRAY(quadIndices), quadIndices) :
										   glu::pr::TriangleStrip(DE_LENGTH_OF_ARRAY(quadIndices), quadIndices);

	glu::draw(context, shaderPipeline.getShaderProgram()->getProgram(), DE_LENGTH_OF_ARRAY(vertexArrays), vertexArrays,
			  primitiveList, (glu::DrawUtilCallback*)DE_NULL);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glu::draw error");
}

bool ShaderViewportLayerArrayUtils::validateColor(tcu::Vec4 renderedColor, tcu::Vec4 referenceColor)
{
	const float epsilon = 0.008f;
	return de::abs(renderedColor.x() - referenceColor.x()) < epsilon &&
		   de::abs(renderedColor.y() - referenceColor.y()) < epsilon &&
		   de::abs(renderedColor.z() - referenceColor.z()) < epsilon &&
		   de::abs(renderedColor.w() - referenceColor.w()) < epsilon;
}

glw::GLint ShaderViewportIndexTestCase::createMaxViewports()
{
	const Functions&		gl			 = m_context.getRenderContext().getFunctions();
	const tcu::RenderTarget renderTarget = m_context.getRenderContext().getRenderTarget();
	const GLfloat			targetWidth  = (GLfloat)renderTarget.getWidth();

	GLint maxViewports = 0;
	gl.getIntegerv(GL_MAX_VIEWPORTS, &maxViewports);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GetIntegerv error");

	const int				  viewportDataSize = 4; // x + y + w + h
	std::vector<glw::GLfloat> data(maxViewports * viewportDataSize);

	GLfloat viewportWidth  = 16.0f;
	GLfloat viewportHeight = 16.0f;

	int currentX = 0;
	int currentY = 0;
	for (GLint i = 0; i < maxViewports; ++i)
	{
		GLfloat x = (GLfloat)currentX * viewportWidth;
		if (x > (targetWidth - viewportWidth))
		{
			x		 = 0.0f;
			currentX = 0;
			currentY++;
		}
		GLfloat y = (GLfloat)currentY * viewportHeight;

		data[i * viewportDataSize + 0] = x;
		data[i * viewportDataSize + 1] = y;
		data[i * viewportDataSize + 2] = viewportWidth;
		data[i * viewportDataSize + 3] = viewportHeight;

		m_viewportData.push_back(tcu::Vec4(x, y, viewportWidth, viewportHeight));

		currentX++;
	}

	gl.viewportArrayv(0, maxViewports, data.data());
	GLU_EXPECT_NO_ERROR(gl.getError(), "ViewportArrayv");

	return maxViewports;
}

/** Constructor.
*
*  @param context Rendering context
*/
ShaderViewportIndexTestCase::ShaderViewportIndexTestCase(deqp::Context& context)
	: TestCase(context, "ShaderViewportIndexTestCase",
			   "Implements gl_ViewportIndex tests described in CTS_ARB_shader_viewport_layer_array")
	, m_maxViewports(0)
	, m_currentViewport(0)
{
	m_isExtensionSupported = context.getContextInfo().isExtensionSupported("GL_ARB_shader_viewport_layer_array");
}

void ShaderViewportIndexTestCase::init()
{
	if (!m_isExtensionSupported)
		return;

	m_maxViewports = this->createMaxViewports();

	m_shaderPipelines.push_back(
		ShaderViewportLayerArrayUtils::ShaderPipeline(false, false, m_maxViewports, "ViewportIndex"));
	m_shaderPipelines.push_back(
		ShaderViewportLayerArrayUtils::ShaderPipeline(true, false, m_maxViewports, "ViewportIndex"));
	m_shaderPipelines.push_back(
		ShaderViewportLayerArrayUtils::ShaderPipeline(true, true, m_maxViewports, "ViewportIndex"));

	for (ShaderPipelineIter iter = m_shaderPipelines.begin(); iter != m_shaderPipelines.end(); ++iter)
	{
		iter->create(m_context.getRenderContext());
	}
}

void ShaderViewportIndexTestCase::deinit()
{
	const Functions&		gl			 = m_context.getRenderContext().getFunctions();
	const tcu::RenderTarget renderTarget = m_context.getRenderContext().getRenderTarget();

	gl.viewport(0, 0, renderTarget.getWidth(), renderTarget.getHeight());
	GLU_EXPECT_NO_ERROR(gl.getError(), "Viewport");
}

/** Executes test iteration.
 *
 *  @return Returns STOP when test has finished executing, CONTINUE if more iterations are needed.
 */
tcu::TestNode::IterateResult ShaderViewportIndexTestCase::iterate()
{
	if (!m_isExtensionSupported)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "Not supported");
		return STOP;
	}

	const glw::Functions&	 gl			= m_context.getRenderContext().getFunctions();
	const glu::RenderContext& renderContext = m_context.getRenderContext();

	tcu::Vec4 renderColor((m_currentViewport + 1) / (float)m_maxViewports, 0.0f, 0.0f, 1.0f);
	tcu::Vec4 backgroundColor(0.0f, 0.0f, 0.0f, 1.0f);

	for (ShaderPipelineIter pipelineIter = m_shaderPipelines.begin(); pipelineIter != m_shaderPipelines.end();
		 ++pipelineIter)
	{
		// rendering

		gl.clearColor(0.0f, 0.0f, 0.0f, 1.0f);
		GLU_EXPECT_NO_ERROR(gl.getError(), "ClearColor");
		gl.clear(GL_COLOR_BUFFER_BIT);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Clear");
		ShaderViewportLayerArrayUtils::renderQuad(renderContext, *pipelineIter, m_currentViewport, renderColor);
		gl.flush();
		GLU_EXPECT_NO_ERROR(gl.getError(), "Flush");

		// verification

		std::vector<std::pair<tcu::Vec2, tcu::Vec4> > expectedPixels;

		for (size_t i = 0; i < m_viewportData.size(); ++i)
		{
			tcu::Vec4 viewportData = m_viewportData[i];

			int currentViewportWithOffset =
				(m_currentViewport + pipelineIter->getViewportLayerOffset()) % m_maxViewports;

			tcu::Vec2 center(viewportData.x() + viewportData.z() * 0.5f, viewportData.y() + viewportData.w() * 0.5f);

			if (i == (unsigned int)currentViewportWithOffset)
			{
				expectedPixels.push_back(std::make_pair(center, renderColor));
			}
			else
			{
				expectedPixels.push_back(std::make_pair(center, backgroundColor));
			}
		}

		for (size_t i = 0; i < expectedPixels.size(); ++i)
		{
			glw::GLfloat rgba[4] = { -1.f, -1.f, -1.f, -1.f };
			gl.readPixels((glw::GLint)expectedPixels[i].first.x(), (glw::GLint)expectedPixels[i].first.y(), 1, 1,
						  GL_RGBA, GL_FLOAT, rgba);
			GLU_EXPECT_NO_ERROR(gl.getError(), "ReadPixels");
			bool validationResult = ShaderViewportLayerArrayUtils::validateColor(
				tcu::Vec4(rgba[0], rgba[1], rgba[2], rgba[3]), expectedPixels[i].second);
			TCU_CHECK_MSG(validationResult, "Expected pixel color did not match rendered one.");
		}
	}

	if (m_currentViewport < (m_maxViewports - 1))
	{
		m_currentViewport++;
		return CONTINUE;
	}

	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	return STOP;
}

ShaderLayerFramebufferTestCaseBase::ShaderLayerFramebufferTestCaseBase(deqp::Context& context, const char* name,
																	   const char* description, bool layered)
	: TestCase(context, name, description)
	, m_layersNum(layered ? 4 : 1)
	, m_fboSize(512)
	, m_texture(0)
	, m_mainFbo(0)
	, m_currentLayer(0)
{
	m_isExtensionSupported = context.getContextInfo().isExtensionSupported("GL_ARB_shader_viewport_layer_array");
}

void ShaderLayerFramebufferTestCaseBase::init()
{
	if (!m_isExtensionSupported)
		return;

	const glw::Functions&	 gl			= m_context.getRenderContext().getFunctions();

	this->createFBO();

	m_shaderPipelines.push_back(ShaderViewportLayerArrayUtils::ShaderPipeline(false, false, m_layersNum, "Layer"));
	m_shaderPipelines.push_back(ShaderViewportLayerArrayUtils::ShaderPipeline(true, false, m_layersNum, "Layer"));
	m_shaderPipelines.push_back(ShaderViewportLayerArrayUtils::ShaderPipeline(true, true, m_layersNum, "Layer"));

	for (ShaderPipelineIter iter = m_shaderPipelines.begin(); iter != m_shaderPipelines.end(); ++iter)
	{
		iter->create(m_context.getRenderContext());
	}

	gl.viewport(0, 0, m_fboSize, m_fboSize);
}

void ShaderLayerFramebufferTestCaseBase::deinit()
{
	const Functions&		gl			 = m_context.getRenderContext().getFunctions();
	const tcu::RenderTarget renderTarget = m_context.getRenderContext().getRenderTarget();

	gl.viewport(0, 0, renderTarget.getWidth(), renderTarget.getHeight());
	GLU_EXPECT_NO_ERROR(gl.getError(), "Viewport");
}


tcu::TestNode::IterateResult ShaderLayerFramebufferTestCaseBase::iterate()
{
	if (!m_isExtensionSupported)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "Not supported");
		return STOP;
	}

	const glw::Functions&	 gl			= m_context.getRenderContext().getFunctions();
	const glu::RenderContext& renderContext = m_context.getRenderContext();

	tcu::Vec4 renderColor((m_currentLayer + 1) / (float)m_layersNum, 0.0f, 0.0f, 1.0f);

	for (ShaderPipelineIter pipelineIter = m_shaderPipelines.begin(); pipelineIter != m_shaderPipelines.end();
		 ++pipelineIter)
	{
		// bind main framebuffer (layered)
		gl.bindFramebuffer(GL_DRAW_FRAMEBUFFER, m_mainFbo);
		GLU_EXPECT_NO_ERROR(gl.getError(), "BindFramebuffer");

		// render
		gl.clearColor(0.0f, 0.0f, 0.0f, 1.0f);
		GLU_EXPECT_NO_ERROR(gl.getError(), "ClearColor");
		gl.clear(GL_COLOR_BUFFER_BIT);
		GLU_EXPECT_NO_ERROR(gl.getError(), "Clear");
		ShaderViewportLayerArrayUtils::renderQuad(renderContext, *pipelineIter, m_currentLayer, renderColor);
		gl.flush();
		GLU_EXPECT_NO_ERROR(gl.getError(), "Flush");

		// calculate layer offset (same value as gl_Layer in shader)
		int currentLayerWithOffset = (m_currentLayer + pipelineIter->getViewportLayerOffset()) % m_layersNum;

		// bind framebuffer of this layer
		gl.bindFramebuffer(GL_READ_FRAMEBUFFER, m_fbos[currentLayerWithOffset]);
		GLU_EXPECT_NO_ERROR(gl.getError(), "bindFramebuffer() call failed.");

		// verification
		glw::GLfloat rgba[4] = { -1.f, -1.f, -1.f, -1.f };
		gl.readPixels(m_fboSize / 2, m_fboSize / 2, 1, 1, GL_RGBA, GL_FLOAT, rgba);
		GLU_EXPECT_NO_ERROR(gl.getError(), "ReadPixels");
		bool validationResult =
			ShaderViewportLayerArrayUtils::validateColor(tcu::Vec4(rgba[0], rgba[1], rgba[2], rgba[3]), renderColor);

		TCU_CHECK_MSG(validationResult, "Expected pixel color did not match rendered one.");
	}

	if (m_currentLayer < (m_layersNum - 1))
	{
		m_currentLayer++;
		return CONTINUE;
	}

	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	return STOP;
}

/** Constructor.
*
*  @param context Rendering context
*/
ShaderLayerFramebufferLayeredTestCase::ShaderLayerFramebufferLayeredTestCase(deqp::Context& context)
	: ShaderLayerFramebufferTestCaseBase(
		  context, "ShaderLayerFramebufferLayeredTestCase",
		  "Implements gl_Layer tests for layered framebuffer described in CTS_ARB_shader_viewport_layer_array", true)
{
}

void ShaderLayerFramebufferLayeredTestCase::createFBO()
{
	const Functions& gl = m_context.getRenderContext().getFunctions();

	gl.genTextures(1, &m_texture);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GenTextures");

	gl.bindTexture(GL_TEXTURE_3D, m_texture);
	GLU_EXPECT_NO_ERROR(gl.getError(), "bindTexture() call failed.");

	gl.texImage3D(GL_TEXTURE_3D, 0, GL_RGBA8, m_fboSize, m_fboSize, m_layersNum, 0, GL_RGBA, GL_FLOAT, 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "texImage3D() call failed.");

	// create main FBO

	gl.genFramebuffers(1, &m_mainFbo);
	GLU_EXPECT_NO_ERROR(gl.getError(), "genFramebuffers() call failed.");
	gl.bindFramebuffer(GL_FRAMEBUFFER, m_mainFbo);
	GLU_EXPECT_NO_ERROR(gl.getError(), "bindFramebuffer() call failed.");
	gl.framebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, m_texture, 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "framebufferTexture() call failed.");

	// create FBO for each layer

	for (int i = 0; i < m_layersNum; ++i)
	{
		deUint32 layerFbo;

		gl.genFramebuffers(1, &layerFbo);
		GLU_EXPECT_NO_ERROR(gl.getError(), "genFramebuffers() call failed.");
		gl.bindFramebuffer(GL_FRAMEBUFFER, layerFbo);
		GLU_EXPECT_NO_ERROR(gl.getError(), "bindFramebuffer() call failed.");
		gl.framebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, m_texture, 0, i);
		GLU_EXPECT_NO_ERROR(gl.getError(), "framebufferTextureLayer() call failed.");

		m_fbos.push_back(layerFbo);
	}
}

void ShaderLayerFramebufferLayeredTestCase::deleteFBO()
{
	const Functions& gl = m_context.getRenderContext().getFunctions();

	gl.deleteFramebuffers(1, &m_mainFbo);
	GLU_EXPECT_NO_ERROR(gl.getError(), "DeleteFramebuffers");
	for (int i = 0; i < m_layersNum; ++i)
	{
		gl.deleteFramebuffers(1, &(m_fbos[i]));
		GLU_EXPECT_NO_ERROR(gl.getError(), "DeleteFramebuffers");
	}
	gl.deleteTextures(1, &m_texture);
	GLU_EXPECT_NO_ERROR(gl.getError(), "DeleteTextures");
}

/** Constructor.
*
*  @param context Rendering context
*/
ShaderLayerFramebufferNonLayeredTestCase::ShaderLayerFramebufferNonLayeredTestCase(deqp::Context& context)
	: ShaderLayerFramebufferTestCaseBase(
		  context, "ShaderLayerFramebufferNonLayeredTestCase",
		  "Implements gl_Layer tests for non-layered framebuffer described in CTS_ARB_shader_viewport_layer_array",
		  false)
{
}

void ShaderLayerFramebufferNonLayeredTestCase::createFBO()
{
	const Functions& gl = m_context.getRenderContext().getFunctions();
	deUint32		 tex;

	gl.genTextures(1, &tex);
	GLU_EXPECT_NO_ERROR(gl.getError(), "GenTextures");

	gl.bindTexture(GL_TEXTURE_2D, tex);
	GLU_EXPECT_NO_ERROR(gl.getError(), "bindTexture() call failed.");

	gl.texImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, m_fboSize, m_fboSize, 0, GL_RGBA, GL_FLOAT, 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "texImage2D() call failed.");

	// create main FBO

	gl.genFramebuffers(1, &m_mainFbo);
	GLU_EXPECT_NO_ERROR(gl.getError(), "genFramebuffers() call failed.");
	gl.bindFramebuffer(GL_FRAMEBUFFER, m_mainFbo);
	GLU_EXPECT_NO_ERROR(gl.getError(), "bindFramebuffer() call failed.");
	gl.framebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, tex, 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "framebufferTexture() call failed.");

	// main FBO is only layer

	m_fbos.push_back(m_mainFbo);
}

void ShaderLayerFramebufferNonLayeredTestCase::deleteFBO()
{
	const Functions& gl = m_context.getRenderContext().getFunctions();

	gl.deleteFramebuffers(1, &m_mainFbo);
	GLU_EXPECT_NO_ERROR(gl.getError(), "DeleteFramebuffers");
	gl.deleteTextures(1, &m_texture);
	GLU_EXPECT_NO_ERROR(gl.getError(), "DeleteTextures");
}

/** Constructor.
 *
 *  @param context Rendering context.
 */
ShaderViewportLayerArray::ShaderViewportLayerArray(deqp::Context& context)
	: TestCaseGroup(context, "shader_viewport_layer_array",
					"Verify conformance of CTS_ARB_shader_viewport_layer_array implementation")
{
}

/** Initializes the test group contents. */
void ShaderViewportLayerArray::init()
{
	addChild(new ShaderViewportIndexTestCase(m_context));
	addChild(new ShaderLayerFramebufferLayeredTestCase(m_context));
	addChild(new ShaderLayerFramebufferNonLayeredTestCase(m_context));
}
} /* gl4cts namespace */
