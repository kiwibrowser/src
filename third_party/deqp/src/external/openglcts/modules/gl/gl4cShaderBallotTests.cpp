/*-------------------------------------------------------------------------
* OpenGL Conformance Test Suite
* -----------------------------
*
* Copyright (c) 2014-2017 The Khronos Group Inc.
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
* \file  gl4cShaderBallotTests.cpp
* \brief Conformance tests for the ARB_shader_ballot functionality.
*/ /*-------------------------------------------------------------------*/

#include "gl4cShaderBallotTests.hpp"

#include "glcContext.hpp"
#include "gluContextInfo.hpp"
#include "gluDefs.hpp"
#include "gluDrawUtil.hpp"
#include "gluObjectWrapper.hpp"
#include "gluProgramInterfaceQuery.hpp"
#include "gluShaderProgram.hpp"
#include "glwEnums.hpp"
#include "glwFunctions.hpp"
#include "tcuRenderTarget.hpp"

namespace gl4cts
{

ShaderBallotBaseTestCase::ShaderPipeline::ShaderPipeline(glu::ShaderType	testedShader,
														 const std::string& contentSnippet,
														 std::map<std::string, std::string> specMap)
	: m_programRender(NULL), m_programCompute(NULL), m_testedShader(testedShader), m_specializationMap(specMap)
{
	std::string testedHeadPart = "#extension GL_ARB_shader_ballot : enable\n"
								 "#extension GL_ARB_gpu_shader_int64 : enable\n";

	std::string testedContentPart = contentSnippet;

	// vertex shader parts

	m_shaders[glu::SHADERTYPE_VERTEX].push_back("#version 450 core\n");
	m_shaders[glu::SHADERTYPE_VERTEX].push_back(m_testedShader == glu::SHADERTYPE_VERTEX ? testedHeadPart : "");
	m_shaders[glu::SHADERTYPE_VERTEX].push_back("in highp vec2 inPosition;\n"
												"in highp vec4 inColor;\n"
												"out highp vec3 vsPosition;\n"
												"out highp vec4 vsColor;\n"
												"void main()\n"
												"{\n"
												"	gl_Position = vec4(inPosition, 0.0, 1.0);\n"
												"	vsPosition = vec3(inPosition, 0.0);\n"
												"	vec4 outColor = vec4(0.0); \n");
	m_shaders[glu::SHADERTYPE_VERTEX].push_back(m_testedShader == glu::SHADERTYPE_VERTEX ? testedContentPart :
																						   "	outColor = inColor;\n");
	m_shaders[glu::SHADERTYPE_VERTEX].push_back("	vsColor = outColor;\n"
												"}\n");

	// fragment shader parts

	m_shaders[glu::SHADERTYPE_FRAGMENT].push_back("#version 450 core\n");
	m_shaders[glu::SHADERTYPE_FRAGMENT].push_back(m_testedShader == glu::SHADERTYPE_FRAGMENT ? testedHeadPart : "");
	m_shaders[glu::SHADERTYPE_FRAGMENT].push_back("in highp vec4 gsColor;\n"
												  "out highp vec4 fsColor;\n"
												  "void main()\n"
												  "{\n"
												  "	vec4 outColor = vec4(0.0); \n");
	m_shaders[glu::SHADERTYPE_FRAGMENT].push_back(
		m_testedShader == glu::SHADERTYPE_FRAGMENT ? testedContentPart : "	outColor = gsColor;\n");
	m_shaders[glu::SHADERTYPE_FRAGMENT].push_back("	fsColor = outColor;\n"
												  "}\n");

	// tessellation control shader parts

	m_shaders[glu::SHADERTYPE_TESSELLATION_CONTROL].push_back("#version 450 core\n");
	m_shaders[glu::SHADERTYPE_TESSELLATION_CONTROL].push_back(
		m_testedShader == glu::SHADERTYPE_TESSELLATION_CONTROL ? testedHeadPart : "");
	m_shaders[glu::SHADERTYPE_TESSELLATION_CONTROL].push_back(
		"layout(vertices = 3) out;\n"
		"in highp vec4 vsColor[];\n"
		"in highp vec3 vsPosition[];\n"
		"out highp vec3 tcsPosition[];\n"
		"out highp vec4 tcsColor[];\n"
		"void main()\n"
		"{\n"
		"	tcsPosition[gl_InvocationID] = vsPosition[gl_InvocationID];\n"
		"	vec4 outColor = vec4(0.0);\n");
	m_shaders[glu::SHADERTYPE_TESSELLATION_CONTROL].push_back(m_testedShader == glu::SHADERTYPE_TESSELLATION_CONTROL ?
																  testedContentPart :
																  "	outColor = vsColor[gl_InvocationID];\n");
	m_shaders[glu::SHADERTYPE_TESSELLATION_CONTROL].push_back("	tcsColor[gl_InvocationID] = outColor;\n"
															  "	gl_TessLevelInner[0] = 3;\n"
															  "	gl_TessLevelOuter[0] = 3;\n"
															  "	gl_TessLevelOuter[1] = 3;\n"
															  "	gl_TessLevelOuter[2] = 3;\n"
															  "}\n");

	// tessellation evaluation shader parts

	m_shaders[glu::SHADERTYPE_TESSELLATION_EVALUATION].push_back("#version 450 core\n");
	m_shaders[glu::SHADERTYPE_TESSELLATION_EVALUATION].push_back(
		m_testedShader == glu::SHADERTYPE_TESSELLATION_EVALUATION ? testedHeadPart : "");
	m_shaders[glu::SHADERTYPE_TESSELLATION_EVALUATION].push_back("layout(triangles, equal_spacing, cw) in;\n"
																 "in highp vec3 tcsPosition[];\n"
																 "in highp vec4 tcsColor[];\n"
																 "out highp vec4 tesColor;\n"
																 "void main()\n"
																 "{\n"
																 "	vec3 p0 = gl_TessCoord.x * tcsPosition[0];\n"
																 "	vec3 p1 = gl_TessCoord.y * tcsPosition[1];\n"
																 "	vec3 p2 = gl_TessCoord.z * tcsPosition[2];\n"
																 "	vec4 outColor = vec4(0.0);\n");
	m_shaders[glu::SHADERTYPE_TESSELLATION_EVALUATION].push_back(
		m_testedShader == glu::SHADERTYPE_TESSELLATION_EVALUATION ? testedContentPart : "	outColor = tcsColor[0];\n");
	m_shaders[glu::SHADERTYPE_TESSELLATION_EVALUATION].push_back("	tesColor = outColor;\n"
																 "	gl_Position = vec4(normalize(p0 + p1 + p2), 1.0);\n"
																 "}\n");

	// geometry shader parts

	m_shaders[glu::SHADERTYPE_GEOMETRY].push_back("#version 450 core\n");
	m_shaders[glu::SHADERTYPE_GEOMETRY].push_back(m_testedShader == glu::SHADERTYPE_GEOMETRY ? testedHeadPart : "");
	m_shaders[glu::SHADERTYPE_GEOMETRY].push_back("layout(triangles) in;\n"
												  "layout(triangle_strip, max_vertices = 3) out;\n"
												  "in highp vec4 tesColor[];\n"
												  "out highp vec4 gsColor;\n"
												  "void main()\n"
												  "{\n"
												  "	for (int i = 0; i<3; i++)\n"
												  "	{\n"
												  "		gl_Position = gl_in[i].gl_Position;\n"
												  "		vec4 outColor = vec4(0.0);\n");
	m_shaders[glu::SHADERTYPE_GEOMETRY].push_back(
		m_testedShader == glu::SHADERTYPE_GEOMETRY ? testedContentPart : "		outColor = tesColor[i];\n");
	m_shaders[glu::SHADERTYPE_GEOMETRY].push_back("		gsColor = outColor;\n"
												  "		EmitVertex();\n"
												  "	}\n"
												  "	EndPrimitive();\n"
												  "}\n");

	// compute shader parts

	m_shaders[glu::SHADERTYPE_COMPUTE].push_back("#version 450 core\n");
	m_shaders[glu::SHADERTYPE_COMPUTE].push_back(m_testedShader == glu::SHADERTYPE_COMPUTE ? testedHeadPart : "");
	m_shaders[glu::SHADERTYPE_COMPUTE].push_back(
		"layout(rgba32f, binding = 1) writeonly uniform highp image2D destImage;\n"
		"layout (local_size_x = 16, local_size_y = 16) in;\n"
		"void main (void)\n"
		"{\n"
		"vec4 outColor = vec4(0.0);\n");
	m_shaders[glu::SHADERTYPE_COMPUTE].push_back(m_testedShader == glu::SHADERTYPE_COMPUTE ? testedContentPart : "");
	m_shaders[glu::SHADERTYPE_COMPUTE].push_back("imageStore(destImage, ivec2(gl_GlobalInvocationID.xy), outColor);\n"
												 "}\n");

	// create shader chunks

	for (unsigned int shaderType = 0; shaderType < glu::SHADERTYPE_LAST; ++shaderType)
	{
		m_shaderChunks[shaderType] = new char*[m_shaders[shaderType].size()];
		for (unsigned int i = 0; i < m_shaders[i].size(); ++i)
		{
			m_shaderChunks[shaderType][i] = (char*)m_shaders[shaderType][i].data();
		}
	}
}

ShaderBallotBaseTestCase::ShaderPipeline::~ShaderPipeline()
{
	if (m_programRender)
	{
		delete m_programRender;
	}

	if (m_programCompute)
	{
		delete m_programCompute;
	}

	for (unsigned int shaderType = 0; shaderType < glu::SHADERTYPE_LAST; ++shaderType)
	{
		delete[] m_shaderChunks[shaderType];
	}
}

const char* const* ShaderBallotBaseTestCase::ShaderPipeline::getShaderParts(glu::ShaderType shaderType) const
{
	return m_shaderChunks[shaderType];
}

unsigned int ShaderBallotBaseTestCase::ShaderPipeline::getShaderPartsCount(glu::ShaderType shaderType) const
{
	return m_shaders[shaderType].size();
}

void ShaderBallotBaseTestCase::ShaderPipeline::renderQuad(deqp::Context& context)
{
	const glw::Functions& gl = context.getRenderContext().getFunctions();

	deUint16 const quadIndices[] = { 0, 1, 2, 2, 1, 3 };

	float const position[] = { -1.0f, -1.0f, -1.0f, 1.0f, 1.0f, -1.0f, 1.0f, 1.0f };

	glu::VertexArrayBinding vertexArrays[] = { glu::va::Float("inPosition", 2, 4, 0, position) };

	this->use(context);

	glu::PrimitiveList primitiveList = glu::pr::Patches(DE_LENGTH_OF_ARRAY(quadIndices), quadIndices);

	glu::draw(context.getRenderContext(), m_programRender->getProgram(), DE_LENGTH_OF_ARRAY(vertexArrays), vertexArrays,
			  primitiveList);

	GLU_EXPECT_NO_ERROR(gl.getError(), "glu::draw error");
}

void ShaderBallotBaseTestCase::ShaderPipeline::executeComputeShader(deqp::Context& context)
{
	const glw::Functions& gl = context.getRenderContext().getFunctions();

	const glu::Texture outputTexture(context.getRenderContext());

	gl.useProgram(m_programCompute->getProgram());

	// output image
	gl.bindTexture(GL_TEXTURE_2D, *outputTexture);
	gl.texStorage2D(GL_TEXTURE_2D, 1, GL_RGBA32UI, 16, 16);
	gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Uploading image data failed");

	// bind image
	gl.bindImageTexture(1, *outputTexture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32UI);
	GLU_EXPECT_NO_ERROR(gl.getError(), "Image setup failed");

	// dispatch compute
	gl.dispatchCompute(1, 1, 1);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glDispatchCompute()");

	gl.memoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glMemoryBarrier()");

	// render output texture

	std::string vs = "#version 450 core\n"
					 "in highp vec2 position;\n"
					 "in vec2 inTexcoord;\n"
					 "out vec2 texcoord;\n"
					 "void main()\n"
					 "{\n"
					 "	texcoord = inTexcoord;\n"
					 "	gl_Position = vec4(position, 0.0, 1.0);\n"
					 "}\n";

	std::string fs = "#version 450 core\n"
					 "uniform sampler2D sampler;\n"
					 "in vec2 texcoord;\n"
					 "out vec4 color;\n"
					 "void main()\n"
					 "{\n"
					 "	color = texture(sampler, texcoord);\n"
					 "}\n";

	glu::ProgramSources sources;
	sources.sources[glu::SHADERTYPE_VERTEX].push_back(vs);
	sources.sources[glu::SHADERTYPE_FRAGMENT].push_back(fs);
	glu::ShaderProgram renderShader(context.getRenderContext(), sources);

	if (!m_programRender->isOk())
	{
		TCU_FAIL("Shader compilation failed");
	}

	gl.bindTexture(GL_TEXTURE_2D, *outputTexture);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture() call failed.");

	gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	gl.useProgram(renderShader.getProgram());

	gl.uniform1i(gl.getUniformLocation(renderShader.getProgram(), "sampler"), 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUniform1i failed");

	deUint16 const quadIndices[] = { 0, 1, 2, 2, 1, 3 };

	float const position[] = { -1.0f, -1.0f, -1.0f, 1.0f, 1.0f, -1.0f, 1.0f, 1.0f };

	float const texCoord[] = { 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f };

	glu::VertexArrayBinding vertexArrays[] = { glu::va::Float("position", 2, 4, 0, position),
											   glu::va::Float("inTexcoord", 2, 4, 0, texCoord) };

	glu::draw(context.getRenderContext(), renderShader.getProgram(), DE_LENGTH_OF_ARRAY(vertexArrays), vertexArrays,
			  glu::pr::TriangleStrip(DE_LENGTH_OF_ARRAY(quadIndices), quadIndices));

	GLU_EXPECT_NO_ERROR(gl.getError(), "glu::draw error");
}

void ShaderBallotBaseTestCase::ShaderPipeline::use(deqp::Context& context)
{
	const glw::Functions& gl = context.getRenderContext().getFunctions();
	gl.useProgram(m_programRender->getProgram());
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram failed");
}

void ShaderBallotBaseTestCase::ShaderPipeline::test(deqp::Context& context)
{
	if (m_testedShader == glu::SHADERTYPE_COMPUTE)
	{
		executeComputeShader(context);
	}
	else
	{
		renderQuad(context);
	}
}

void ShaderBallotBaseTestCase::createShaderPrograms(ShaderPipeline& pipeline)
{
	glu::ProgramSources sourcesRender;

	for (unsigned int i = 0; i < glu::SHADERTYPE_COMPUTE; ++i)
	{
		glu::ShaderType shaderType = (glu::ShaderType)i;

		std::map<std::string, std::string>::const_iterator mapIter;
		for (mapIter = pipeline.getSpecializationMap().begin(); mapIter != pipeline.getSpecializationMap().end();
			 mapIter++)
			m_specializationMap[mapIter->first] = mapIter->second;

		std::string shader =
			specializeShader(pipeline.getShaderPartsCount(shaderType), pipeline.getShaderParts(shaderType));
		sourcesRender.sources[i].push_back(shader);
	}

	glu::ShaderProgram* programRender = new glu::ShaderProgram(m_context.getRenderContext(), sourcesRender);

	if (!programRender->isOk())
	{
		TCU_FAIL("Shader compilation failed");
	}

	glu::ProgramSources sourcesCompute;

	m_specializationMap.insert(pipeline.getSpecializationMap().begin(), pipeline.getSpecializationMap().end());
	std::string shaderCompute = specializeShader(pipeline.getShaderPartsCount(glu::SHADERTYPE_COMPUTE),
												 pipeline.getShaderParts(glu::SHADERTYPE_COMPUTE));
	sourcesCompute.sources[glu::SHADERTYPE_COMPUTE].push_back(shaderCompute);

	glu::ShaderProgram* programCompute = new glu::ShaderProgram(m_context.getRenderContext(), sourcesCompute);

	if (!programCompute->isOk())
	{
		TCU_FAIL("Shader compilation failed");
	}

	pipeline.setShaderPrograms(programRender, programCompute);
}

ShaderBallotBaseTestCase::~ShaderBallotBaseTestCase()
{
	for (ShaderPipelineIter iter = m_shaderPipelines.begin(); iter != m_shaderPipelines.end(); ++iter)
	{
		delete *iter;
	}
}

bool ShaderBallotBaseTestCase::validateScreenPixels(deqp::Context& context, tcu::Vec4 desiredColor,
													tcu::Vec4 ignoredColor)
{
	const glw::Functions&   gl			 = context.getRenderContext().getFunctions();
	const tcu::RenderTarget renderTarget = context.getRenderContext().getRenderTarget();
	tcu::IVec2				size(renderTarget.getWidth(), renderTarget.getHeight());

	glw::GLfloat* pixels = new glw::GLfloat[size.x() * size.y() * 4];

	// clear buffer
	for (int x = 0; x < size.x(); ++x)
	{
		for (int y = 0; y < size.y(); ++y)
		{
			int mappedPixelPosition = y * size.x() + x;

			pixels[mappedPixelPosition * 4 + 0] = -1.0f;
			pixels[mappedPixelPosition * 4 + 1] = -1.0f;
			pixels[mappedPixelPosition * 4 + 2] = -1.0f;
			pixels[mappedPixelPosition * 4 + 3] = -1.0f;
		}
	}

	// read pixels
	gl.readPixels(0, 0, size.x(), size.y(), GL_RGBA, GL_FLOAT, pixels);

	// validate pixels
	bool rendered = false;
	for (int x = 0; x < size.x(); ++x)
	{
		for (int y = 0; y < size.y(); ++y)
		{
			int mappedPixelPosition = y * size.x() + x;

			tcu::Vec4 color(pixels[mappedPixelPosition * 4 + 0], pixels[mappedPixelPosition * 4 + 1],
							pixels[mappedPixelPosition * 4 + 2], pixels[mappedPixelPosition * 4 + 3]);

			if (!ShaderBallotBaseTestCase::validateColor(color, ignoredColor))
			{
				rendered = true;
				if (!ShaderBallotBaseTestCase::validateColor(color, desiredColor))
				{
					return false;
				}
			}
		}
	}

	delete[] pixels;

	return rendered;
}

bool ShaderBallotBaseTestCase::validateScreenPixelsSameColor(deqp::Context& context, tcu::Vec4 ignoredColor)
{
	const glw::Functions&   gl			 = context.getRenderContext().getFunctions();
	const tcu::RenderTarget renderTarget = context.getRenderContext().getRenderTarget();
	tcu::IVec2				size(renderTarget.getWidth(), renderTarget.getHeight());

	glw::GLfloat* centerPixel = new glw::GLfloat[4];
	centerPixel[0]			  = -1.0f;
	centerPixel[1]			  = -1.0f;
	centerPixel[2]			  = -1.0f;
	centerPixel[3]			  = -1.0f;

	// read pixel
	gl.readPixels(size.x() / 2, size.y() / 2, 1, 1, GL_RGBA, GL_FLOAT, centerPixel);

	tcu::Vec4 desiredColor(centerPixel[0], centerPixel[1], centerPixel[2], centerPixel[3]);

	delete[] centerPixel;

	// validation
	return ShaderBallotBaseTestCase::validateScreenPixels(context, desiredColor, ignoredColor);
}

bool ShaderBallotBaseTestCase::validateColor(tcu::Vec4 testedColor, tcu::Vec4 desiredColor)
{
	const float epsilon = 0.008f;
	return de::abs(testedColor.x() - desiredColor.x()) < epsilon &&
		   de::abs(testedColor.y() - desiredColor.y()) < epsilon &&
		   de::abs(testedColor.z() - desiredColor.z()) < epsilon &&
		   de::abs(testedColor.w() - desiredColor.w()) < epsilon;
}

/** Constructor.
*
*  @param context Rendering context
*/
ShaderBallotAvailabilityTestCase::ShaderBallotAvailabilityTestCase(deqp::Context& context)
	: ShaderBallotBaseTestCase(context, "ShaderBallotAvailability",
							   "Implements verification of availability for new build-in features")
{
	std::string colorShaderSnippet =
		"	float red = gl_SubGroupSizeARB / 64.0f;\n"
		"	float green = 1.0f - (gl_SubGroupInvocationARB / float(gl_SubGroupSizeARB));\n"
		"	float blue = float(ballotARB(true) % 256) / 256.0f;\n"
		"	outColor = readInvocationARB(vec4(red, green, blue, 1.0f), gl_SubGroupInvocationARB);\n";

	for (unsigned int i = 0; i < glu::SHADERTYPE_LAST; ++i)
	{
		m_shaderPipelines.push_back(new ShaderPipeline((glu::ShaderType)i, colorShaderSnippet));
	}
}

/** Initializes the test
*/
void ShaderBallotAvailabilityTestCase::init()
{
}

/** Executes test iteration.
*
*  @return Returns STOP when test has finished executing, CONTINUE if more iterations are needed.
*/
tcu::TestNode::IterateResult ShaderBallotAvailabilityTestCase::iterate()
{
	if (!m_context.getContextInfo().isExtensionSupported("GL_ARB_shader_ballot") ||
		!m_context.getContextInfo().isExtensionSupported("GL_ARB_gpu_shader_int64"))
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "Not supported");
		return STOP;
	}

	for (ShaderPipelineIter iter = m_shaderPipelines.begin(); iter != m_shaderPipelines.end(); ++iter)
	{
		createShaderPrograms(**iter);
	}

	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	for (ShaderPipelineIter pipelineIter = m_shaderPipelines.begin(); pipelineIter != m_shaderPipelines.end();
		 ++pipelineIter)
	{
		gl.clearColor(0.0f, 0.0f, 0.0f, 1.0f);
		gl.clear(GL_COLOR_BUFFER_BIT);

		(*pipelineIter)->test(m_context);

		gl.flush();
	}

	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	return STOP;
}

/** Constructor.
*
*  @param context Rendering context
*/
ShaderBallotBitmasksTestCase::ShaderBallotBitmasksTestCase(deqp::Context& context)
	: ShaderBallotBaseTestCase(context, "ShaderBallotBitmasks",
							   "Implements verification of values of gl_SubGroup*MaskARB variables")
{
	m_maskVars["gl_SubGroupEqMaskARB"] = "==";
	m_maskVars["gl_SubGroupGeMaskARB"] = ">=";
	m_maskVars["gl_SubGroupGtMaskARB"] = ">";
	m_maskVars["gl_SubGroupLeMaskARB"] = "<=";
	m_maskVars["gl_SubGroupLtMaskARB"] = "<";

	std::string colorShaderSnippet = "	uint64_t mask = 0;\n"
									 "	for(uint i = 0; i < gl_SubGroupSizeARB; ++i)\n"
									 "	{\n"
									 "		if(i ${MASK_OPERATOR} gl_SubGroupInvocationARB)\n"
									 "			mask = mask | (1ul << i);\n"
									 "	}\n"
									 "	float color = (${MASK_VAR} ^ mask) == 0ul ? 1.0 : 0.0;\n"
									 "	outColor = vec4(color, color, color, 1.0);\n";

	for (MaskVarIter maskIter = m_maskVars.begin(); maskIter != m_maskVars.end(); maskIter++)
	{
		for (unsigned int i = 0; i < glu::SHADERTYPE_LAST; ++i)
		{
			std::map<std::string, std::string> specMap;
			specMap["MASK_VAR"]		 = maskIter->first;
			specMap["MASK_OPERATOR"] = maskIter->second;
			m_shaderPipelines.push_back(new ShaderPipeline((glu::ShaderType)i, colorShaderSnippet, specMap));
		}
	}
}

/** Initializes the test
*/
void ShaderBallotBitmasksTestCase::init()
{
}

/** Executes test iteration.
*
*  @return Returns STOP when test has finished executing, CONTINUE if more iterations are needed.
*/
tcu::TestNode::IterateResult ShaderBallotBitmasksTestCase::iterate()
{
	if (!m_context.getContextInfo().isExtensionSupported("GL_ARB_shader_ballot") ||
		!m_context.getContextInfo().isExtensionSupported("GL_ARB_gpu_shader_int64"))
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "Not supported");
		return STOP;
	}

	for (ShaderPipelineIter iter = m_shaderPipelines.begin(); iter != m_shaderPipelines.end(); ++iter)
	{
		createShaderPrograms(**iter);
	}

	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	for (ShaderPipelineIter pipelineIter = m_shaderPipelines.begin(); pipelineIter != m_shaderPipelines.end();
		 ++pipelineIter)
	{
		gl.clearColor(1.0f, 0.0f, 0.0f, 1.0f);
		gl.clear(GL_COLOR_BUFFER_BIT);

		(*pipelineIter)->test(m_context);

		gl.flush();

		bool validationResult = ShaderBallotBaseTestCase::validateScreenPixels(
			m_context, tcu::Vec4(1.0f, 1.0f, 1.0f, 1.0f), tcu::Vec4(1.0f, 0.0f, 0.0f, 1.0f));
		TCU_CHECK_MSG(validationResult, "Bitmask value is not correct");
	}

	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	return STOP;
}

/** Constructor.
*
*  @param context Rendering context
*/
ShaderBallotFunctionBallotTestCase::ShaderBallotFunctionBallotTestCase(deqp::Context& context)
	: ShaderBallotBaseTestCase(context, "ShaderBallotFunctionBallot",
							   "Implements verification of ballotARB calls and returned results")
{
	std::string ballotFalseSnippet = "	uint64_t result = ballotARB(false);\n"
									 "	float color = result == 0ul ? 1.0 : 0.0;\n"
									 "	outColor = vec4(color, color, color, 1.0);\n";

	std::string ballotTrueSnippet = "	uint64_t result = ballotARB(true);\n"
									"	float color = result != 0ul ? 1.0 : 0.0;\n"
									"	uint64_t invocationBit = 1ul << gl_SubGroupInvocationARB;\n"
									"	color *= float(invocationBit & result);\n"
									"	outColor = vec4(color, color, color, 1.0);\n";

	std::string ballotMixedSnippet = "	bool param = (gl_SubGroupInvocationARB % 2) == 0ul;\n"
									 "	uint64_t result = ballotARB(param);\n"
									 "	float color = (param && result != 0ul) || !param ? 1.0 : 0.0;\n"
									 "	outColor = vec4(color, color, color, 1.0);\n";

	for (unsigned int i = 0; i < glu::SHADERTYPE_LAST; ++i)
	{
		m_shaderPipelines.push_back(new ShaderPipeline((glu::ShaderType)i, ballotFalseSnippet));
		m_shaderPipelines.push_back(new ShaderPipeline((glu::ShaderType)i, ballotTrueSnippet));
		m_shaderPipelines.push_back(new ShaderPipeline((glu::ShaderType)i, ballotMixedSnippet));
	}
}

/** Initializes the test
*/
void ShaderBallotFunctionBallotTestCase::init()
{
}

/** Executes test iteration.
*
*  @return Returns STOP when test has finished executing, CONTINUE if more iterations are needed.
*/
tcu::TestNode::IterateResult ShaderBallotFunctionBallotTestCase::iterate()
{
	if (!m_context.getContextInfo().isExtensionSupported("GL_ARB_shader_ballot") ||
		!m_context.getContextInfo().isExtensionSupported("GL_ARB_gpu_shader_int64"))
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "Not supported");
		return STOP;
	}

	for (ShaderPipelineIter iter = m_shaderPipelines.begin(); iter != m_shaderPipelines.end(); ++iter)
	{
		createShaderPrograms(**iter);
	}

	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	for (ShaderPipelineIter pipelineIter = m_shaderPipelines.begin(); pipelineIter != m_shaderPipelines.end();
		 ++pipelineIter)
	{
		gl.clearColor(1.0f, 0.0f, 0.0f, 1.0f);
		gl.clear(GL_COLOR_BUFFER_BIT);

		(*pipelineIter)->test(m_context);

		gl.flush();

		bool validationResult = ShaderBallotBaseTestCase::validateScreenPixels(
			m_context, tcu::Vec4(1.0f, 1.0f, 1.0f, 1.0f), tcu::Vec4(1.0f, 0.0f, 0.0f, 1.0f));
		TCU_CHECK_MSG(validationResult, "Value returned from ballotARB function is not correct");
	}

	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	return STOP;
}

/** Constructor.
*
*  @param context Rendering context
*/
ShaderBallotFunctionReadTestCase::ShaderBallotFunctionReadTestCase(deqp::Context& context)
	: ShaderBallotBaseTestCase(context, "ShaderBallotFunctionRead",
							   "Implements verification of readInvocationARB and readFirstInvocationARB function calls")
{
	std::string readFirstInvSnippet = "float color = 1.0f - (gl_SubGroupInvocationARB / float(gl_SubGroupSizeARB));\n"
									  "outColor = readFirstInvocationARB(vec4(color, color, color, 1.0f));\n";

	std::string readInvSnippet = "float color = 1.0 - (gl_SubGroupInvocationARB / float(gl_SubGroupSizeARB));\n"
								 "uvec2 parts = unpackUint2x32(ballotARB(true));\n"
								 "uint invocation;\n"
								 "if (parts.x != 0) {\n"
								 "    invocation = findLSB(parts.x);\n"
								 "} else {\n"
								 "    invocation = findLSB(parts.y) + 32;\n"
								 "}\n"
								 "outColor = readInvocationARB(vec4(color, color, color, 1.0f), invocation);\n";

	for (unsigned int i = 0; i < glu::SHADERTYPE_LAST; ++i)
	{
		m_shaderPipelines.push_back(new ShaderPipeline((glu::ShaderType)i, readFirstInvSnippet));
		m_shaderPipelines.push_back(new ShaderPipeline((glu::ShaderType)i, readInvSnippet));
	}
}

/** Initializes the test
*/
void ShaderBallotFunctionReadTestCase::init()
{
}

/** Executes test iteration.
*
*  @return Returns STOP when test has finished executing, CONTINUE if more iterations are needed.
*/
tcu::TestNode::IterateResult ShaderBallotFunctionReadTestCase::iterate()
{
	if (!m_context.getContextInfo().isExtensionSupported("GL_ARB_shader_ballot") ||
		!m_context.getContextInfo().isExtensionSupported("GL_ARB_gpu_shader_int64"))
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "Not supported");
		return STOP;
	}

	for (ShaderPipelineIter iter = m_shaderPipelines.begin(); iter != m_shaderPipelines.end(); ++iter)
	{
		createShaderPrograms(**iter);
	}

	const glw::Functions&   gl			 = m_context.getRenderContext().getFunctions();
	const tcu::RenderTarget renderTarget = m_context.getRenderContext().getRenderTarget();

	gl.clearColor(1.0f, 0.0f, 0.0f, 1.0f);
	gl.viewport(renderTarget.getWidth() / 2 - 1, renderTarget.getHeight() / 2 - 1, 2, 2);

	for (ShaderPipelineIter pipelineIter = m_shaderPipelines.begin(); pipelineIter != m_shaderPipelines.end();
		 ++pipelineIter)
	{
		gl.clear(GL_COLOR_BUFFER_BIT);

		(*pipelineIter)->test(m_context);

		gl.flush();

		bool validationResult =
			ShaderBallotBaseTestCase::validateScreenPixelsSameColor(m_context, tcu::Vec4(1.0f, 0.0f, 0.0f, 1.0f));
		TCU_CHECK_MSG(validationResult, "Read functions result is not correct");
	}

	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	return STOP;
}

/** Constructor.
*
*  @param context Rendering context.
**/
ShaderBallotTests::ShaderBallotTests(deqp::Context& context)
	: TestCaseGroup(context, "shader_ballot_tests", "Verify conformance of CTS_ARB_shader_ballot implementation")
{
}

/** Initializes the shader_ballot test group.
*
**/
void ShaderBallotTests::init(void)
{
	addChild(new ShaderBallotAvailabilityTestCase(m_context));
	addChild(new ShaderBallotBitmasksTestCase(m_context));
	addChild(new ShaderBallotFunctionBallotTestCase(m_context));
	addChild(new ShaderBallotFunctionReadTestCase(m_context));
}
} /* glcts namespace */
