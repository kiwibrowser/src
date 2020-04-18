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
 * \file
 * \brief
 */ /*-------------------------------------------------------------------*/

/**
 */ /*!
 * \file  gl4cTextureFilterMinmaxTests.cpp
 * \brief Conformance tests for the ARB_texture_filter_minmax functionality.
 */ /*-------------------------------------------------------------------*/

#include "gl4cTextureFilterMinmaxTests.hpp"
#include "gluContextInfo.hpp"
#include "gluDefs.hpp"
#include "gluDrawUtil.hpp"
#include "gluObjectWrapper.hpp"
#include "gluShaderProgram.hpp"
#include "gluTexture.hpp"
#include "glwEnums.hpp"
#include "glwFunctions.hpp"
#include "tcuImageIO.hpp"
#include "tcuRenderTarget.hpp"
#include "tcuTestLog.hpp"
#include "tcuTextureUtil.hpp"

#include "deUniquePtr.hpp"

#include <map>
#include <vector>

namespace gl4cts
{

static const int   TEXTURE_FILTER_MINMAX_SIZE			  = 32;
static const float TEXTURE_FILTER_MINMAX_DOWNSCALE_FACTOR = 0.57f;
static const float TEXTURE_FILTER_MINMAX_UPSCALE_FACTOR   = 1.63f;

TextureFilterMinmaxUtils::TextureFilterMinmaxUtils()
{
	m_reductionModeParams.push_back(ReductionModeParam(GL_MIN, false));
	m_reductionModeParams.push_back(ReductionModeParam(GL_WEIGHTED_AVERAGE_ARB, false));
	m_reductionModeParams.push_back(ReductionModeParam(GL_MAX, false));

	m_supportedTextureTypes.push_back(new Texture1D());
	m_supportedTextureTypes.push_back(new Texture1DArray());
	m_supportedTextureTypes.push_back(new Texture2D());
	m_supportedTextureTypes.push_back(new Texture2DArray());
	m_supportedTextureTypes.push_back(new Texture3D());
	m_supportedTextureTypes.push_back(new TextureCube());

	m_supportedTextureDataTypes.push_back(SupportedTextureDataType(GL_RED, GL_FLOAT, MINMAX));
	m_supportedTextureDataTypes.push_back(SupportedTextureDataType(GL_RED, GL_UNSIGNED_BYTE, MINMAX));
	m_supportedTextureDataTypes.push_back(
		SupportedTextureDataType(GL_DEPTH_COMPONENT, GL_FLOAT, MINMAX | EXCLUDE_3D | EXCLUDE_CUBE));
	m_supportedTextureDataTypes.push_back(
		SupportedTextureDataType(GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, MINMAX | EXCLUDE_3D | EXCLUDE_CUBE));
}

TextureFilterMinmaxUtils::~TextureFilterMinmaxUtils()
{
	for (SupportedTextureTypeIter iter = m_supportedTextureTypes.begin(); iter != m_supportedTextureTypes.end(); ++iter)
	{
		SupportedTextureType* textureType = *iter;
		delete textureType;
	}
}

// SupportedTextureType

void TextureFilterMinmaxUtils::SupportedTextureType::replaceAll(std::string& str, const std::string& from,
																const std::string& to)
{
	size_t start = 0;
	while ((start = str.find(from, start)) != std::string::npos)
	{
		str.replace(start, from.length(), to);
		start += to.length();
	}
}

TextureFilterMinmaxUtils::SupportedTextureType::SupportedTextureType(glw::GLenum		type,
																	 const std::string& shaderTexcoordType,
																	 const std::string& shaderSamplerType)
	: m_type(type)
{
	m_vertexShader = "#version 450 core\n"
					 "in highp vec2 position;\n"
					 "in <texcoord_type> inTexcoord;\n"
					 "out <texcoord_type> texcoord;\n"
					 "void main()\n"
					 "{\n"
					 "	texcoord = inTexcoord;\n"
					 "	gl_Position = vec4(position, 0.0, 1.0);\n"
					 "}\n";

	m_fragmentShader = "#version 450 core\n"
					   "uniform <sampler_type> sampler;\n"
					   "in <texcoord_type> texcoord;\n"
					   "out vec4 color;\n"
					   "void main()\n"
					   "{\n"
					   "	color = texture(sampler, texcoord);\n"
					   "}\n";

	replaceAll(m_vertexShader, "<texcoord_type>", shaderTexcoordType);
	replaceAll(m_fragmentShader, "<texcoord_type>", shaderTexcoordType);
	replaceAll(m_fragmentShader, "<sampler_type>", shaderSamplerType);
}

void TextureFilterMinmaxUtils::SupportedTextureType::renderToFBO(const glu::RenderContext& context,
																 glw::GLuint resultTextureId, tcu::IVec2 size,
																 glw::GLint reductionMode)
{
	const glw::Functions& gl = context.getFunctions();

	gl.bindTexture(GL_TEXTURE_2D, resultTextureId);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture() call failed.");

	gl.texStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, size.x(), size.y());
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTexStorage2D() call failed.");

	glw::GLuint fbo = 0;
	gl.genFramebuffers(1, &fbo);
	GLU_EXPECT_NO_ERROR(gl.getError(), "genFramebuffers error occurred");

	gl.bindFramebuffer(GL_FRAMEBUFFER, fbo);
	GLU_EXPECT_NO_ERROR(gl.getError(), "bindFramebuffer error occurred");

	gl.framebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, resultTextureId, 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "framebufferTexture2D error occurred");

	if (gl.checkFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		throw 0;
	}

	gl.viewport(0, 0, size.x(), size.y());
	renderQuad(context, reductionMode);

	gl.deleteFramebuffers(1, &fbo);
}

void TextureFilterMinmaxUtils::SupportedTextureType::renderQuad(const glu::RenderContext& context,
																glw::GLint				  reductionMode)
{
	const glw::Functions& gl = context.getFunctions();

	deUint16 const quadIndices[] = { 0, 1, 2, 2, 1, 3 };

	float const position[] = { -1.0f, -1.0f, -1.0f, 1.0f, 1.0f, -1.0f, 1.0f, 1.0f };

	std::vector<float> texCoords = getTexCoords();

	glu::VertexArrayBinding vertexArrays[] = { glu::va::Float("position", 2, 4, 0, position),
											   glu::va::Float("inTexcoord", 1, 4, 0, texCoords.data()) };

	gl.bindTexture(m_type, this->getTextureGL());
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture() call failed.");

	gl.texParameteri(m_type, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	gl.texParameteri(m_type, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	gl.texParameteri(m_type, GL_TEXTURE_REDUCTION_MODE_ARB, reductionMode);

	glu::ShaderProgram program(context, glu::makeVtxFragSources(m_vertexShader, m_fragmentShader));
	if (!program.isOk())
	{
		TCU_FAIL("Shader compilation failed");
	}

	gl.useProgram(program.getProgram());
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram failed");

	gl.uniform1i(gl.getUniformLocation(program.getProgram(), "sampler"), 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUniform1i failed");

	gl.clear(GL_COLOR_BUFFER_BIT);

	glu::draw(context, program.getProgram(), DE_LENGTH_OF_ARRAY(vertexArrays), vertexArrays,
			  glu::pr::TriangleStrip(DE_LENGTH_OF_ARRAY(quadIndices), quadIndices));
}

// Texture1D

TextureFilterMinmaxUtils::Texture1D::Texture1D() : SupportedTextureType(GL_TEXTURE_1D, "float", "sampler1D")
{
}

glw::GLuint TextureFilterMinmaxUtils::Texture1D::getTextureGL()
{
	return m_texture->getGLTexture();
}

std::vector<float> TextureFilterMinmaxUtils::Texture1D::getTexCoords()
{
	float const texCoord[] = { 0.0f, 0.0f, 1.0f, 1.0f };
	return std::vector<float>(texCoord, texCoord + sizeof(texCoord) / sizeof(float));
}

void TextureFilterMinmaxUtils::Texture1D::generate(const glu::RenderContext& context, tcu::IVec3 size,
												   glw::GLenum format, glw::GLenum type, bool generateMipmaps)
{
	const glw::Functions& gl = context.getFunctions();

	m_texture = de::MovePtr<glu::Texture1D>(new glu::Texture1D(context, format, type, size.x()));

	m_texture->getRefTexture().allocLevel(0);
	if (generateMipmaps)
	{
		m_texture->getRefTexture().allocLevel(1);
	}
	GLU_EXPECT_NO_ERROR(gl.getError(), "glu::Texture1D error occurred");

	tcu::fillWithGrid(m_texture->getRefTexture().getLevel(0), 4, tcu::Vec4(1.0f, 1.0f, 1.0f, 1.0f),
					  tcu::Vec4(0.0f, 0.0f, 0.0f, 1.0f));
	GLU_EXPECT_NO_ERROR(gl.getError(), "glu::Texture1D error occurred");

	m_texture->upload();
	GLU_EXPECT_NO_ERROR(gl.getError(), "glu::Texture1D error occurred");

	if (generateMipmaps)
	{
		gl.generateTextureMipmap(m_texture->getGLTexture());
	}

	gl.activeTexture(GL_TEXTURE0);
}

// Texture1DArray

TextureFilterMinmaxUtils::Texture1DArray::Texture1DArray()
	: SupportedTextureType(GL_TEXTURE_1D_ARRAY, "vec2", "sampler1DArray")
{
}

glw::GLuint TextureFilterMinmaxUtils::Texture1DArray::getTextureGL()
{
	return m_texture->getGLTexture();
}

std::vector<float> TextureFilterMinmaxUtils::Texture1DArray::getTexCoords()
{
	float const texCoord[] = { 0.0f, 0.5f, 0.0f, 0.5f, 1.0f, 0.5f, 1.0f, 0.5f };
	return std::vector<float>(texCoord, texCoord + sizeof(texCoord) / sizeof(float));
}

void TextureFilterMinmaxUtils::Texture1DArray::generate(const glu::RenderContext& context, tcu::IVec3 size,
														glw::GLenum format, glw::GLenum type, bool generateMipmaps)
{
	DE_UNREF(generateMipmaps);

	const glw::Functions& gl = context.getFunctions();

	m_texture = de::MovePtr<glu::Texture1DArray>(new glu::Texture1DArray(context, format, type, size.x(), 2));

	m_texture->getRefTexture().allocLevel(0);
	m_texture->getRefTexture().allocLevel(1);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glu::Texture1DArray error occurred");

	tcu::fillWithGrid(m_texture->getRefTexture().getLevel(0), 4, tcu::Vec4(1.0f, 1.0f, 1.0f, 1.0f),
					  tcu::Vec4(0.0f, 0.0f, 0.0f, 1.0f));
	GLU_EXPECT_NO_ERROR(gl.getError(), "glu::Texture1DArray error occurred");

	tcu::fillWithGrid(m_texture->getRefTexture().getLevel(1), 4, tcu::Vec4(0.0f, 1.0f, 1.0f, 1.0f),
					  tcu::Vec4(1.0f, 0.0f, 0.0f, 1.0f));
	GLU_EXPECT_NO_ERROR(gl.getError(), "glu::Texture1DArray error occurred");

	m_texture->upload();
	GLU_EXPECT_NO_ERROR(gl.getError(), "glu::Texture1DArray error occurred");

	gl.activeTexture(GL_TEXTURE0);
}

// Texture2D

TextureFilterMinmaxUtils::Texture2D::Texture2D() : SupportedTextureType(GL_TEXTURE_2D, "vec2", "sampler2D")
{
}

glw::GLuint TextureFilterMinmaxUtils::Texture2D::getTextureGL()
{
	return m_texture->getGLTexture();
}

std::vector<float> TextureFilterMinmaxUtils::Texture2D::getTexCoords()
{
	float const texCoord[] = { 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f };
	return std::vector<float>(texCoord, texCoord + sizeof(texCoord) / sizeof(float));
}

void TextureFilterMinmaxUtils::Texture2D::generate(const glu::RenderContext& context, tcu::IVec3 size,
												   glw::GLenum format, glw::GLenum type, bool generateMipmaps)
{
	const glw::Functions& gl = context.getFunctions();

	m_texture = de::MovePtr<glu::Texture2D>(new glu::Texture2D(context, format, type, size.x(), size.y()));

	m_texture->getRefTexture().allocLevel(0);
	if (generateMipmaps)
	{
		m_texture->getRefTexture().allocLevel(1);
	}
	GLU_EXPECT_NO_ERROR(gl.getError(), "glu::Texture2D error occurred");

	tcu::fillWithGrid(m_texture->getRefTexture().getLevel(0), 4, tcu::Vec4(1.0f, 1.0f, 1.0f, 1.0f),
					  tcu::Vec4(0.0f, 0.0f, 0.0f, 1.0f));
	GLU_EXPECT_NO_ERROR(gl.getError(), "glu::Texture2D error occurred");

	m_texture->upload();
	GLU_EXPECT_NO_ERROR(gl.getError(), "glu::Texture2D error occurred");

	if (generateMipmaps)
	{
		gl.generateTextureMipmap(m_texture->getGLTexture());
	}

	gl.activeTexture(GL_TEXTURE0);
}

// Texture2DArray

TextureFilterMinmaxUtils::Texture2DArray::Texture2DArray()
	: SupportedTextureType(GL_TEXTURE_2D_ARRAY, "vec3", "sampler2DArray")
{
}

glw::GLuint TextureFilterMinmaxUtils::Texture2DArray::getTextureGL()
{
	return m_texture->getGLTexture();
}

std::vector<float> TextureFilterMinmaxUtils::Texture2DArray::getTexCoords()
{
	float const texCoord[] = { 0.0f, 0.0f, 0.5f, 0.0f, 1.0f, 0.5f, 1.0f, 0.0f, 0.5f, 1.0f, 1.0f, 0.5f };
	return std::vector<float>(texCoord, texCoord + sizeof(texCoord) / sizeof(float));
}

void TextureFilterMinmaxUtils::Texture2DArray::generate(const glu::RenderContext& context, tcu::IVec3 size,
														glw::GLenum format, glw::GLenum type, bool generateMipmaps)
{
	DE_UNREF(generateMipmaps);

	const glw::Functions& gl = context.getFunctions();

	m_texture = de::MovePtr<glu::Texture2DArray>(new glu::Texture2DArray(context, format, type, size.x(), size.y(), 2));

	m_texture->getRefTexture().allocLevel(0);
	m_texture->getRefTexture().allocLevel(1);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glu::Texture2DArray error occurred");

	tcu::fillWithGrid(m_texture->getRefTexture().getLevel(0), 4, tcu::Vec4(1.0f, 1.0f, 1.0f, 1.0f),
					  tcu::Vec4(0.0f, 0.0f, 0.0f, 1.0f));
	GLU_EXPECT_NO_ERROR(gl.getError(), "glu::Texture2DArray error occurred");

	tcu::fillWithGrid(m_texture->getRefTexture().getLevel(1), 4, tcu::Vec4(0.0f, 1.0f, 1.0f, 1.0f),
					  tcu::Vec4(1.0f, 0.0f, 0.0f, 1.0f));
	GLU_EXPECT_NO_ERROR(gl.getError(), "glu::Texture2DArray error occurred");

	m_texture->upload();
	GLU_EXPECT_NO_ERROR(gl.getError(), "glu::Texture2DArray error occurred");

	gl.activeTexture(GL_TEXTURE0);
}

// Texture3D

TextureFilterMinmaxUtils::Texture3D::Texture3D() : SupportedTextureType(GL_TEXTURE_3D, "vec3", "sampler3D")
{
}

glw::GLuint TextureFilterMinmaxUtils::Texture3D::getTextureGL()
{
	return m_texture->getGLTexture();
}

std::vector<float> TextureFilterMinmaxUtils::Texture3D::getTexCoords()
{
	float const texCoord[] = { 0.0f, 0.0f, 0.5f, 0.0f, 1.0f, 0.5f, 1.0f, 0.0f, 0.5f, 1.0f, 1.0f, 0.5f };
	return std::vector<float>(texCoord, texCoord + sizeof(texCoord) / sizeof(float));
}

void TextureFilterMinmaxUtils::Texture3D::generate(const glu::RenderContext& context, tcu::IVec3 size,
												   glw::GLenum format, glw::GLenum type, bool generateMipmaps)
{
	DE_UNREF(generateMipmaps);

	const glw::Functions& gl = context.getFunctions();

	m_texture = de::MovePtr<glu::Texture3D>(new glu::Texture3D(context, format, type, size.x(), size.y(), size.z()));

	m_texture->getRefTexture().allocLevel(0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glu::Texture3D error occurred");

	tcu::fillWithGrid(m_texture->getRefTexture().getLevel(0), 4, tcu::Vec4(1.0f, 1.0f, 1.0f, 1.0f),
					  tcu::Vec4(0.0f, 0.0f, 0.0f, 1.0f));
	GLU_EXPECT_NO_ERROR(gl.getError(), "glu::Texture3D error occurred");

	m_texture->upload();
	GLU_EXPECT_NO_ERROR(gl.getError(), "glu::Texture3D error occurred");

	gl.activeTexture(GL_TEXTURE0);
}

// TextureCube

TextureFilterMinmaxUtils::TextureCube::TextureCube() : SupportedTextureType(GL_TEXTURE_CUBE_MAP, "vec3", "samplerCube")
{
}

glw::GLuint TextureFilterMinmaxUtils::TextureCube::getTextureGL()
{
	return m_texture->getGLTexture();
}

std::vector<float> TextureFilterMinmaxUtils::TextureCube::getTexCoords()
{
	float const texCoord[] = { 1.0f, 1.0f, 0.0f, -1.0f, 1.0f, 0.0f, -1.0f, -1.0f, 0.0f, 1.0f, -1.0f, 0.0f };
	return std::vector<float>(texCoord, texCoord + sizeof(texCoord) / sizeof(float));
}

void TextureFilterMinmaxUtils::TextureCube::generate(const glu::RenderContext& context, tcu::IVec3 size,
													 glw::GLenum format, glw::GLenum type, bool generateMipmaps)
{
	DE_UNREF(generateMipmaps);

	const glw::Functions& gl = context.getFunctions();

	m_texture = de::MovePtr<glu::TextureCube>(new glu::TextureCube(context, format, type, size.x()));

	for (int faceIndex = 0; faceIndex < tcu::CUBEFACE_LAST; ++faceIndex)
	{
		m_texture->getRefTexture().allocLevel((tcu::CubeFace)faceIndex, 0);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glu::TextureCube error occurred");

		tcu::fillWithGrid(m_texture->getRefTexture().getLevelFace(0, (tcu::CubeFace)faceIndex), 4,
						  tcu::Vec4(1.0f, 1.0f, 1.0f, 1.0f), tcu::Vec4(0.0f, 0.0f, 0.0f, 1.0f));
		GLU_EXPECT_NO_ERROR(gl.getError(), "glu::TextureCube error occurred");
	}

	m_texture->upload();
	GLU_EXPECT_NO_ERROR(gl.getError(), "glu::TextureCube error occurred");

	gl.activeTexture(GL_TEXTURE0);
}

// TextureFilterMinmaxUtils methods

std::vector<glw::GLuint> TextureFilterMinmaxUtils::getDataFromTexture(const glu::RenderContext& context,
																	  glw::GLuint textureId, tcu::IVec2 textureSize,
																	  glw::GLenum format, glw::GLenum type)
{
	const glw::Functions& gl = context.getFunctions();

	glw::GLsizei imageLength = textureSize.x() * textureSize.y();

	std::vector<glw::GLuint> data;
	data.resize(imageLength);

	gl.bindTexture(GL_TEXTURE_2D, textureId);
	GLU_EXPECT_NO_ERROR(gl.getError(), "bindTexture error occurred");

	gl.getTexImage(GL_TEXTURE_2D, 0, format, type, &data[0]);
	GLU_EXPECT_NO_ERROR(gl.getError(), "getTexImage error occurred");

	return data;
}

glw::GLuint TextureFilterMinmaxUtils::calcPixelSumValue(const glu::RenderContext& context, glw::GLuint textureId,
														tcu::IVec2 textureSize, glw::GLenum format, glw::GLenum type)
{
	std::vector<glw::GLuint> textureData = getDataFromTexture(context, textureId, textureSize, format, type);

	glw::GLuint sum = 0;

	for (size_t texel = 0; texel < textureData.size(); ++texel)
	{
		tcu::RGBA rgba(textureData[texel]);
		sum += rgba.getRed();
		sum += rgba.getGreen();
		sum += rgba.getBlue();
		sum += rgba.getAlpha();
	}

	return sum;
}

/** Constructor.
*
*  @param context Rendering context
*/
TextureFilterMinmaxParameterQueriesTestCase::TextureFilterMinmaxParameterQueriesTestCase(deqp::Context& context)
	: TestCase(context, "TextureFilterMinmaxParameterQueries",
			   "Implements all parameter queries tests described in CTS_ARB_texture_filter_minmax")
{
	/* Left blank intentionally */
}

/** Executes test iteration.
 *
 *  @return Returns STOP when test has finished executing, CONTINUE if more iterations are needed.
 */
tcu::TestNode::IterateResult TextureFilterMinmaxParameterQueriesTestCase::iterate()
{
	if (!m_context.getContextInfo().isExtensionSupported("GL_ARB_texture_filter_minmax"))
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "Not Supported");
		return STOP;
	}

	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	testReductionModeQueriesDefaultValues(gl);

	for (TextureFilterMinmaxUtils::ReductionModeParamIter iter = m_utils.getReductionModeParams().begin();
		 iter != m_utils.getReductionModeParams().end(); ++iter)
	{
		testReductionModeQueries(gl, iter->m_reductionMode);
	}

	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

	return STOP;
}

void TextureFilterMinmaxParameterQueriesTestCase::testReductionModeQueriesDefaultValues(const glw::Functions& gl)
{
	const glu::RenderContext& renderContext = m_context.getRenderContext();
	de::MovePtr<glu::Sampler> sampler		= de::MovePtr<glu::Sampler>(new glu::Sampler(renderContext));
	GLU_EXPECT_NO_ERROR(gl.getError(), "glu::Sampler error occurred");

	glw::GLint params;

	gl.getSamplerParameteriv(**sampler, GL_TEXTURE_REDUCTION_MODE_ARB, &params);
	GLU_EXPECT_NO_ERROR(gl.getError(), "getSamplerParameteriv error occurred");
	TCU_CHECK_MSG(params == GL_WEIGHTED_AVERAGE_ARB, "getSamplerParameteriv value mismatch with expected default");

	for (TextureFilterMinmaxUtils::SupportedTextureTypeIter iter = m_utils.getSupportedTextureTypes().begin();
		 iter != m_utils.getSupportedTextureTypes().end(); ++iter)
	{
		TextureFilterMinmaxUtils::SupportedTextureType* textureType = *iter;

		gl.getTexParameteriv(textureType->getType(), GL_TEXTURE_REDUCTION_MODE_ARB, &params);
		GLU_EXPECT_NO_ERROR(gl.getError(), "getTexParameteriv error occurred");
		TCU_CHECK_MSG(params == GL_WEIGHTED_AVERAGE_ARB, "getTexParameteriv value mismatch with expected default");

		gl.getTexParameterIiv(textureType->getType(), GL_TEXTURE_REDUCTION_MODE_ARB, &params);
		GLU_EXPECT_NO_ERROR(gl.getError(), "getTexParameterIiv error occurred");
		TCU_CHECK_MSG(params == GL_WEIGHTED_AVERAGE_ARB, "getTexParameterIiv value mismatch with expected default");
	}

	for (TextureFilterMinmaxUtils::SupportedTextureDataTypeIter iter = m_utils.getSupportedTextureDataTypes().begin();
		 iter != m_utils.getSupportedTextureDataTypes().end(); ++iter)
	{
		de::MovePtr<glu::Texture2D> texture = de::MovePtr<glu::Texture2D>(new glu::Texture2D(
			renderContext, iter->m_format, iter->m_type, TEXTURE_FILTER_MINMAX_SIZE, TEXTURE_FILTER_MINMAX_SIZE));
		texture->upload();
		GLU_EXPECT_NO_ERROR(gl.getError(), "glu::Texture2D error occurred");

		gl.getTextureParameteriv(texture->getGLTexture(), GL_TEXTURE_REDUCTION_MODE_ARB, &params);
		GLU_EXPECT_NO_ERROR(gl.getError(), "getTextureParameteriv error occurred");
		TCU_CHECK_MSG(params == GL_WEIGHTED_AVERAGE_ARB, "getTextureParameteriv value mismatch with expected default");

		gl.getTextureParameterIiv(texture->getGLTexture(), GL_TEXTURE_REDUCTION_MODE_ARB, &params);
		GLU_EXPECT_NO_ERROR(gl.getError(), "getTextureParameterIiv error occurred");
		TCU_CHECK_MSG(params == GL_WEIGHTED_AVERAGE_ARB, "getTextureParameterIiv value mismatch with expected default");
	}
}

void TextureFilterMinmaxParameterQueriesTestCase::testReductionModeQueries(const glw::Functions& gl, glw::GLint pname)
{
	const glu::RenderContext& renderContext = m_context.getRenderContext();
	de::MovePtr<glu::Sampler> sampler		= de::MovePtr<glu::Sampler>(new glu::Sampler(renderContext));
	GLU_EXPECT_NO_ERROR(gl.getError(), "glu::Sampler error occurred");

	gl.samplerParameteri(**sampler, GL_TEXTURE_REDUCTION_MODE_ARB, pname);
	GLU_EXPECT_NO_ERROR(gl.getError(), "samplerParameteri error occurred");

	for (TextureFilterMinmaxUtils::SupportedTextureTypeIter iter = m_utils.getSupportedTextureTypes().begin();
		 iter != m_utils.getSupportedTextureTypes().end(); ++iter)
	{
		TextureFilterMinmaxUtils::SupportedTextureType* textureType = *iter;

		gl.texParameteriv(textureType->getType(), GL_TEXTURE_REDUCTION_MODE_ARB, &pname);
		GLU_EXPECT_NO_ERROR(gl.getError(), "texParameteriv error occurred");

		gl.texParameterIiv(textureType->getType(), GL_TEXTURE_REDUCTION_MODE_ARB, &pname);
		GLU_EXPECT_NO_ERROR(gl.getError(), "texParameterIiv error occurred");
	}

	for (TextureFilterMinmaxUtils::SupportedTextureDataTypeIter iter = m_utils.getSupportedTextureDataTypes().begin();
		 iter != m_utils.getSupportedTextureDataTypes().end(); ++iter)
	{
		de::MovePtr<glu::Texture2D> texture = de::MovePtr<glu::Texture2D>(new glu::Texture2D(
			renderContext, iter->m_format, iter->m_type, TEXTURE_FILTER_MINMAX_SIZE, TEXTURE_FILTER_MINMAX_SIZE));
		texture->upload();
		GLU_EXPECT_NO_ERROR(gl.getError(), "glu::Texture2D error occurred");

		gl.textureParameteriv(texture->getGLTexture(), GL_TEXTURE_REDUCTION_MODE_ARB, &pname);
		GLU_EXPECT_NO_ERROR(gl.getError(), "textureParameteriv error occurred");

		gl.textureParameterIiv(texture->getGLTexture(), GL_TEXTURE_REDUCTION_MODE_ARB, &pname);
		GLU_EXPECT_NO_ERROR(gl.getError(), "textureParameterIiv error occurred");
	}
}

/** Base class for minmax render tests. Constructor.
*
*  @param context Rendering context
*  @param name TestCase name
*  @param description TestCase description
*  @param renderScale Scale of rendered texture
*  @param mipmapping Is mipmapping enabled
*/
TextureFilterMinmaxFilteringTestCaseBase::TextureFilterMinmaxFilteringTestCaseBase(deqp::Context& context,
																				   const char*	name,
																				   const char*	description,
																				   float renderScale, bool mipmapping)
	: deqp::TestCase(context, name, description), m_renderScale(renderScale), m_mipmapping(mipmapping)
{
}

/** Executes test iteration.
*
*  @return Returns STOP when test has finished executing, CONTINUE if more iterations are needed.
*/
tcu::TestNode::IterateResult TextureFilterMinmaxFilteringTestCaseBase::iterate()
{
	if (!m_context.getContextInfo().isExtensionSupported("GL_ARB_texture_filter_minmax"))
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "Not Supported");
		return STOP;
	}

	const glu::RenderContext& renderContext = m_context.getRenderContext();
	const glw::Functions&	 gl			= renderContext.getFunctions();

	for (TextureFilterMinmaxUtils::SupportedTextureTypeIter textureTypeIter =
			 m_utils.getSupportedTextureTypes().begin();
		 textureTypeIter != m_utils.getSupportedTextureTypes().end(); ++textureTypeIter)
	{
		TextureFilterMinmaxUtils::SupportedTextureType* textureType = *textureTypeIter;

		for (TextureFilterMinmaxUtils::SupportedTextureDataTypeIter dataTypeIter =
				 m_utils.getSupportedTextureDataTypes().begin();
			 dataTypeIter != m_utils.getSupportedTextureDataTypes().end(); ++dataTypeIter)
		{
			if (!dataTypeIter->hasFlag(TextureFilterMinmaxUtils::MINMAX))
				continue;

			if (dataTypeIter->hasFlag(TextureFilterMinmaxUtils::EXCLUDE_3D) && textureType->getType() == GL_TEXTURE_3D)
				continue;

			if (dataTypeIter->hasFlag(TextureFilterMinmaxUtils::EXCLUDE_CUBE) &&
				textureType->getType() == GL_TEXTURE_CUBE_MAP)
				continue;

			std::map<glw::GLint, glw::GLuint> reductionModeTexelSumMap;

			for (TextureFilterMinmaxUtils::ReductionModeParamIter paramIter = m_utils.getReductionModeParams().begin();
				 paramIter != m_utils.getReductionModeParams().end(); ++paramIter)
			{
				if (paramIter->m_queryTestOnly)
					continue;

				tcu::IVec2 scaledTextureSize(int(float(TEXTURE_FILTER_MINMAX_SIZE) * m_renderScale));

				textureType->generate(renderContext, tcu::IVec3(TEXTURE_FILTER_MINMAX_SIZE), dataTypeIter->m_format,
									  dataTypeIter->m_type, m_mipmapping);

				glw::GLuint resultTextureId = 0;
				gl.genTextures(1, &resultTextureId);
				GLU_EXPECT_NO_ERROR(gl.getError(), "glGenTextures() call failed.");

				textureType->renderToFBO(renderContext, resultTextureId, scaledTextureSize, paramIter->m_reductionMode);

				reductionModeTexelSumMap[paramIter->m_reductionMode] = m_utils.calcPixelSumValue(
					renderContext, resultTextureId, scaledTextureSize, GL_RED, GL_UNSIGNED_BYTE);

				gl.deleteTextures(1, &resultTextureId);
			}

			TCU_CHECK_MSG(reductionModeTexelSumMap[GL_MIN] < reductionModeTexelSumMap[GL_WEIGHTED_AVERAGE_ARB],
						  "Sum of texels for GL_MIN should be smaller than for GL_WEIGHTED_AVERAGE_ARB");

			TCU_CHECK_MSG(reductionModeTexelSumMap[GL_MAX] > reductionModeTexelSumMap[GL_WEIGHTED_AVERAGE_ARB],
						  "Sum of texels for GL_MAX should be greater than for GL_WEIGHTED_AVERAGE_ARB");
		}
	}

	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

	return STOP;
}

/** Constructor.
*
*  @param context Rendering context
*/
TextureFilterMinmaxMinificationFilteringTestCase::TextureFilterMinmaxMinificationFilteringTestCase(
	deqp::Context& context)
	: TextureFilterMinmaxFilteringTestCaseBase(
		  context, "TextureFilterMinmaxMinificationFiltering",
		  "Implements minification filtering tests described in CTS_ARB_texture_filter_minmax",
		  TEXTURE_FILTER_MINMAX_DOWNSCALE_FACTOR, false)
{
	/* Left blank intentionally */
}

/** Constructor.
*
*  @param context Rendering context
*/
TextureFilterMinmaxMagnificationFilteringTestCase::TextureFilterMinmaxMagnificationFilteringTestCase(
	deqp::Context& context)
	: TextureFilterMinmaxFilteringTestCaseBase(
		  context, "TextureFilterMinmaxMagnificationFiltering",
		  "Implements magnification filtering tests described in CTS_ARB_texture_filter_minmax",
		  TEXTURE_FILTER_MINMAX_UPSCALE_FACTOR, false)
{
	/* Left blank intentionally */
}

/** Constructor.
*
*  @param context Rendering context
*/
TextureFilterMinmaxMipmapMinificationFilteringTestCase::TextureFilterMinmaxMipmapMinificationFilteringTestCase(
	deqp::Context& context)
	: TextureFilterMinmaxFilteringTestCaseBase(
		  context, "TextureFilterMinmaxMipmapMinificationFiltering",
		  "Implements mipmap minification filtering tests described in CTS_ARB_texture_filter_minmax",
		  TEXTURE_FILTER_MINMAX_DOWNSCALE_FACTOR, true)
{
	/* Left blank intentionally */
}

/** Constructor.
*
*  @param context Rendering context
*/
TextureFilterMinmaxSupportTestCase::TextureFilterMinmaxSupportTestCase(deqp::Context& context)
	: TestCase(context, "TextureFilterMinmaxSupport", "Implements calling GetInternalFormat* and validates with "
													  "expected result described in CTS_ARB_texture_filter_minmax")
{
}

/** Executes test iteration.
*
*  @return Returns STOP when test has finished executing, CONTINUE if more iterations are needed.
*/
tcu::TestNode::IterateResult TextureFilterMinmaxSupportTestCase::iterate()
{
	if (!m_context.getContextInfo().isExtensionSupported("GL_ARB_texture_filter_minmax"))
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "Not Supported");
		return STOP;
	}

	const glu::RenderContext& renderContext = m_context.getRenderContext();
	const glw::Functions&	 gl			= renderContext.getFunctions();

	for (TextureFilterMinmaxUtils::SupportedTextureTypeIter textureTypeIter =
			 m_utils.getSupportedTextureTypes().begin();
		 textureTypeIter != m_utils.getSupportedTextureTypes().end(); ++textureTypeIter)
	{
		TextureFilterMinmaxUtils::SupportedTextureType* textureType = *textureTypeIter;

		for (TextureFilterMinmaxUtils::SupportedTextureDataTypeIter dataTypeIter =
				 m_utils.getSupportedTextureDataTypes().begin();
			 dataTypeIter != m_utils.getSupportedTextureDataTypes().end(); ++dataTypeIter)
		{
			if (!dataTypeIter->hasFlag(TextureFilterMinmaxUtils::MINMAX))
				continue;

			if (dataTypeIter->hasFlag(TextureFilterMinmaxUtils::EXCLUDE_3D) && textureType->getType() == GL_TEXTURE_3D)
				continue;

			if (dataTypeIter->hasFlag(TextureFilterMinmaxUtils::EXCLUDE_CUBE) &&
				textureType->getType() == GL_TEXTURE_CUBE_MAP)
				continue;

			glw::GLint params = 0;
			gl.getInternalformativ(textureType->getType(), dataTypeIter->m_format, GL_TEXTURE_REDUCTION_MODE_ARB,
								   sizeof(glw::GLint), &params);
			GLU_EXPECT_NO_ERROR(gl.getError(), "getInternalformativ() call failed.");
			TCU_CHECK_MSG(params, "GetInternalformativ incorrect value");
		}
	}

	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

	return STOP;
}

/** Constructor.
 *
 *  @param context Rendering context.
 */
TextureFilterMinmax::TextureFilterMinmax(deqp::Context& context)
	: TestCaseGroup(context, "texture_filter_minmax_tests",
					"Verify conformance of CTS_ARB_texture_filter_minmax implementation")
{
}

/** Initializes the test group contents. */
void TextureFilterMinmax::init()
{
	addChild(new TextureFilterMinmaxParameterQueriesTestCase(m_context));
	addChild(new TextureFilterMinmaxMinificationFilteringTestCase(m_context));
	addChild(new TextureFilterMinmaxMagnificationFilteringTestCase(m_context));
	addChild(new TextureFilterMinmaxMipmapMinificationFilteringTestCase(m_context));
	addChild(new TextureFilterMinmaxSupportTestCase(m_context));
}
} /* gl4cts namespace */
