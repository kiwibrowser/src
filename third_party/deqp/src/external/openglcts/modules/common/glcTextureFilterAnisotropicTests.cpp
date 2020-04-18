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
 * \file  glcTextureFilterAnisotropicTests.cpp
 * \brief Conformance tests for the GL_EXT_texture_filter_anisotropic functionality.
 */ /*-------------------------------------------------------------------*/

#include "deMath.h"

#include "glcTextureFilterAnisotropicTests.hpp"
#include "gluContextInfo.hpp"
#include "gluDefs.hpp"
#include "gluShaderProgram.hpp"
#include "gluStrUtil.hpp"
#include "gluTextureUtil.hpp"
#include "glwEnums.hpp"
#include "glwFunctions.hpp"
#include "tcuRGBA.hpp"
#include "tcuRenderTarget.hpp"
#include "tcuTestLog.hpp"
#include "tcuTexture.hpp"

using namespace glw;
using namespace glu;

namespace glcts
{

namespace TextureFilterAnisotropicUtils
{

/** Replace all occurrence of <token> with <text> in <str>
 *
 * @param token           Token string
 * @param text            String that will be used as replacement for <token>
 * @param string          String to work on
 **/
void replaceToken(const GLchar* token, const GLchar* text, std::string& str)
{
	const size_t text_length  = strlen(text);
	const size_t token_length = strlen(token);

	size_t token_position;
	while ((token_position = str.find(token, 0)) != std::string::npos)
	{
		str.replace(token_position, token_length, text, text_length);
	}
}

/** Allocate storage for texture
 *
 * @param target           Texture target
 * @param refTexCoordType  GLSL texture coord type
 * @param refSamplerType   GLSL texture sampler type
 **/
void generateTokens(GLenum target, std::string& refTexCoordType, std::string& refSamplerType)
{
	switch (target)
	{
	case GL_TEXTURE_2D:
		refTexCoordType = "vec2";
		refSamplerType  = "sampler2D";
		break;
	case GL_TEXTURE_2D_ARRAY:
		refTexCoordType = "vec3";
		refSamplerType  = "sampler2DArray";
		break;
	default:
		refTexCoordType = "vec2";
		refSamplerType  = "sampler2D";
		break;
	}
}

/** Set contents of texture
 *
 * @param gl              GL functions
 * @param target          Texture target
 * @param level           Mipmap level
 * @param internal_format Format of data
 * @param width           Width of texture
 * @param height          Height of texture
 * @param depth           Depth of texture
 * @param format          Format of data
 * @param type            Type of data
 * @param data            Buffer with image data
 **/
void texImage(const Functions& gl, GLenum target, GLint level, GLenum internal_format, GLuint width, GLuint height,
			  GLuint depth, GLenum format, GLenum type, const GLvoid* data)
{
	switch (target)
	{
	case GL_TEXTURE_2D:
		gl.texImage2D(target, level, internal_format, width, height, 0 /* border */, format, type, data);
		GLU_EXPECT_NO_ERROR(gl.getError(), "texImage");
		break;
	case GL_TEXTURE_2D_ARRAY:
		gl.texImage3D(target, level, internal_format, width, height, depth, 0 /* border */, format, type, data);
		GLU_EXPECT_NO_ERROR(gl.getError(), "texImage");
		break;
	default:
		TCU_FAIL("Invalid enum");
		break;
	}
}

/** Set contents of texture
 *
 * @param gl              GL functions
 * @param target          Texture target
 * @param level           Mipmap level
 * @param x               X offset
 * @param y               Y offset
 * @param z               Z offset
 * @param width           Width of texture
 * @param height          Height of texture
 * @param depth           Depth of texture
 * @param format          Format of data
 * @param type            Type of data
 * @param pixels          Buffer with image data
 **/
void subImage(const Functions& gl, GLenum target, GLint level, GLint x, GLint y, GLint z, GLsizei width, GLsizei height,
			  GLsizei depth, GLenum format, GLenum type, const GLvoid* pixels)
{
	switch (target)
	{
	case GL_TEXTURE_2D:
		gl.texSubImage2D(target, level, x, y, width, height, format, type, pixels);
		GLU_EXPECT_NO_ERROR(gl.getError(), "TexSubImage2D");
		break;
	case GL_TEXTURE_2D_ARRAY:
		gl.texSubImage3D(target, level, x, y, z, width, height, depth, format, type, pixels);
		GLU_EXPECT_NO_ERROR(gl.getError(), "TexSubImage3D");
		break;
	default:
		TCU_FAIL("Invalid enum");
		break;
	}
}

} // TextureFilterAnisotropicUtils namespace

/** Constructor.
 *
 *  @param context     Rendering context
 */
TextureFilterAnisotropicQueriesTestCase::TextureFilterAnisotropicQueriesTestCase(deqp::Context& context)
	: TestCase(context, "queries", "Verifies if queries for GL_EXT_texture_filter_anisotropic tokens works as expected")
{
	/* Left blank intentionally */
}

/** Stub deinit method. */
void TextureFilterAnisotropicQueriesTestCase::deinit()
{
}

/** Stub init method */
void TextureFilterAnisotropicQueriesTestCase::init()
{
	glu::ContextType contextType = m_context.getRenderContext().getType();
	if (!glu::contextSupports(contextType, glu::ApiType::core(4, 6)) &&
		!m_context.getContextInfo().isExtensionSupported("GL_EXT_texture_filter_anisotropic") &&
		!m_context.getContextInfo().isExtensionSupported("GL_ARB_texture_filter_anisotropic"))
	{
		TCU_THROW(NotSupportedError, "texture filter anisotropic functionality not supported");
	}
}

/** Executes test iteration.
 *
 *  @return Returns STOP when test has finished executing, CONTINUE if more iterations are needed.
 */
tcu::TestNode::IterateResult TextureFilterAnisotropicQueriesTestCase::iterate()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	GLuint texture;
	gl.genTextures(1, &texture);
	GLU_EXPECT_NO_ERROR(gl.getError(), "genTextures");
	gl.bindTexture(GL_TEXTURE_2D, texture);
	GLU_EXPECT_NO_ERROR(gl.getError(), "bindTexture");
	TextureFilterAnisotropicUtils::texImage(gl, GL_TEXTURE_2D, 0, GL_RGBA8, 16, 16, 1, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

	if (verifyTexParameters(gl) && verifyGet(gl))
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	else
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");

	gl.deleteTextures(1, &texture);
	GLU_EXPECT_NO_ERROR(gl.getError(), "deleteTextures");

	return STOP;
}

/** Verify if texParameter*, getTexParameter* queries for GL_TEXTURE_MAX_ANISOTROPY_EXT pname works as expected.
 *
 *  @param gl   OpenGL functions wrapper
 *
 *  @return Returns true if queries test passed, false otherwise.
 */
bool TextureFilterAnisotropicQueriesTestCase::verifyTexParameters(const glw::Functions& gl)
{
	GLint iValue;
	gl.getTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, &iValue);
	GLU_EXPECT_NO_ERROR(gl.getError(), "getTexParameteriv");

	// Verify initial integer value which should be equal to 1
	if (iValue != 1)
	{
		m_testCtx.getLog() << tcu::TestLog::Message
						   << "GetTexParameteriv failed. Expected value: 1, Queried value: " << iValue
						   << tcu::TestLog::EndMessage;
		return false;
	}

	GLfloat fValue;
	gl.getTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, &fValue);
	GLU_EXPECT_NO_ERROR(gl.getError(), "getTexParameterfv");

	// Verify initial float value which should be equal to 1.0f
	if (fValue != 1.0f)
	{
		m_testCtx.getLog() << tcu::TestLog::Message
						   << "GetTexParameterfv failed. Expected value: 1.0, Queried value: " << iValue
						   << tcu::TestLog::EndMessage;
		return false;
	}

	// Set custom integer value and verify it
	iValue = 2;
	gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, iValue);
	GLU_EXPECT_NO_ERROR(gl.getError(), "texParameteri");

	gl.getTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, &iValue);
	GLU_EXPECT_NO_ERROR(gl.getError(), "getTexParameteriv");

	if (iValue != 2)
	{
		m_testCtx.getLog() << tcu::TestLog::Message
						   << "texParameteri failed. Expected value: 2, Queried value: " << iValue
						   << tcu::TestLog::EndMessage;
		return false;
	}

	// Set custom float value and verify it
	fValue = 1.5f;
	gl.texParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, fValue);
	GLU_EXPECT_NO_ERROR(gl.getError(), "texParameterf");

	gl.getTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, &fValue);
	GLU_EXPECT_NO_ERROR(gl.getError(), "getTexParameterfv");

	if (fValue != 1.5f)
	{
		m_testCtx.getLog() << tcu::TestLog::Message
						   << "texParameterf failed. Expected value: 1.5, Queried value: " << fValue
						   << tcu::TestLog::EndMessage;
		return false;
	}

	// Set custom integer value and verify it
	iValue = 1;
	gl.texParameteriv(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, &iValue);
	GLU_EXPECT_NO_ERROR(gl.getError(), "texParameteriv");

	gl.getTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, &iValue);
	GLU_EXPECT_NO_ERROR(gl.getError(), "getTexParameteriv");

	if (iValue != 1)
	{
		m_testCtx.getLog() << tcu::TestLog::Message
						   << "texParameteriv failed. Expected value: 1, Queried value: " << iValue
						   << tcu::TestLog::EndMessage;
		return false;
	}

	// Set custom float value and verify it
	fValue = 2.0f;
	gl.texParameterfv(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, &fValue);
	GLU_EXPECT_NO_ERROR(gl.getError(), "texParameterfv");

	gl.getTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, &fValue);
	GLU_EXPECT_NO_ERROR(gl.getError(), "getTexParameterfv");

	if (fValue != 2.0f)
	{
		m_testCtx.getLog() << tcu::TestLog::Message
						   << "texParameterfv failed. Expected value: 2.0, Queried value: " << fValue
						   << tcu::TestLog::EndMessage;
		return false;
	}

	// Set texture filter anisotropic to 0.9f and check if INVALID_VALUE error is generated
	fValue = 0.9f;
	gl.texParameterfv(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, &fValue);
	GLint error = gl.getError();
	if (error != GL_INVALID_VALUE)
	{
		m_testCtx.getLog()
			<< tcu::TestLog::Message
			<< "texParameterfv failed for values less then 1.0f. Expected INVALID_VALUE error. Generated error: "
			<< glu::getErrorName(error) << tcu::TestLog::EndMessage;
		return false;
	}

	return true;
}

/** Verify if get* queries for GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT pname works as expected.
 *
 *  @param gl   OpenGL functions wrapper
 *
 *  @return Returns true if queries test passed, false otherwise.
 */
bool TextureFilterAnisotropicQueriesTestCase::verifyGet(const glw::Functions& gl)
{
	GLboolean bValue;
	GLint	 iValue;
	GLfloat   fValue;
	GLdouble  dValue;

	gl.getBooleanv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &bValue);
	GLU_EXPECT_NO_ERROR(gl.getError(), "getBooleanv");

	gl.getIntegerv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &iValue);
	GLU_EXPECT_NO_ERROR(gl.getError(), "getIntegerv");

	gl.getFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &fValue);
	GLU_EXPECT_NO_ERROR(gl.getError(), "getFloatv");

	if (glu::isContextTypeGLCore(m_context.getRenderContext().getType()))
	{
		gl.getDoublev(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &dValue);
		GLU_EXPECT_NO_ERROR(gl.getError(), "getDoublev");
	}

	return true;
}

/** Constructor.
 *
 *  @param context     Rendering context
 */
TextureFilterAnisotropicDrawingTestCase::TextureFilterAnisotropicDrawingTestCase(deqp::Context& context)
	: TestCase(context, "drawing", "Verifies if drawing texture with anisotropic filtering is performed as expected")
	, m_vertex(DE_NULL)
	, m_fragment(DE_NULL)
	, m_texture(0)
{
	/* Left blank intentionally */
}

/** Stub deinit method. */
void TextureFilterAnisotropicDrawingTestCase::deinit()
{
	/* Left blank intentionally */
}

/** Stub init method */
void TextureFilterAnisotropicDrawingTestCase::init()
{
	glu::ContextType contextType = m_context.getRenderContext().getType();
	if (!glu::contextSupports(contextType, glu::ApiType::core(4, 6)) &&
		!m_context.getContextInfo().isExtensionSupported("GL_EXT_texture_filter_anisotropic") &&
		!m_context.getContextInfo().isExtensionSupported("GL_ARB_texture_filter_anisotropic"))
	{
		TCU_THROW(NotSupportedError, "texture filter anisotropic functionality not supported");
	}

	const tcu::RenderTarget& rt = m_context.getRenderTarget();

	GLint width  = rt.getWidth();
	GLint height = rt.getHeight();

	if (width < 32 || height < 32)
		TCU_THROW(NotSupportedError, "Config not supported - render buffer size should be at least 32x32");

	m_vertex = "#version <VERSION>\n"
			   "\n"
			   "in highp vec3 vertex;\n"
			   "in highp <TEXCOORD_TYPE> inTexCoord;\n"
			   "out highp <TEXCOORD_TYPE> commonTexCoord;\n"
			   "\n"
			   "uniform highp mat4 projectionMatrix;\n"
			   "\n"
			   "void main()\n"
			   "{\n"
			   "    commonTexCoord = inTexCoord;\n"
			   "    gl_Position = vec4(vertex, 1.0) * projectionMatrix;\n"
			   "}\n";

	m_fragment = "#version <VERSION>\n"
				 "\n"
				 "in highp <TEXCOORD_TYPE> commonTexCoord;\n"
				 "out highp vec4 fragColor;\n"
				 "\n"
				 "uniform highp <SAMPLER_TYPE> tex;\n"
				 "\n"
				 "void main()\n"
				 "{\n"
				 "    fragColor = texture(tex, commonTexCoord);\n"
				 "}\n"
				 "\n";

	m_supportedTargets.clear();
	m_supportedTargets.push_back(GL_TEXTURE_2D);
	m_supportedTargets.push_back(GL_TEXTURE_2D_ARRAY);

	m_supportedInternalFormats.clear();
	m_supportedInternalFormats.push_back(GL_R8);
	m_supportedInternalFormats.push_back(GL_R8_SNORM);
	m_supportedInternalFormats.push_back(GL_RG8);
	m_supportedInternalFormats.push_back(GL_RG8_SNORM);
	m_supportedInternalFormats.push_back(GL_RGB8);
	m_supportedInternalFormats.push_back(GL_RGB8_SNORM);
	m_supportedInternalFormats.push_back(GL_RGB565);
	m_supportedInternalFormats.push_back(GL_RGBA4);
	m_supportedInternalFormats.push_back(GL_RGB5_A1);
	m_supportedInternalFormats.push_back(GL_RGBA8);
	m_supportedInternalFormats.push_back(GL_RGBA8_SNORM);
	m_supportedInternalFormats.push_back(GL_RGB10_A2);
	m_supportedInternalFormats.push_back(GL_SRGB8);
	m_supportedInternalFormats.push_back(GL_SRGB8_ALPHA8);
	m_supportedInternalFormats.push_back(GL_R16F);
	m_supportedInternalFormats.push_back(GL_RG16F);
	m_supportedInternalFormats.push_back(GL_RGB16F);
	m_supportedInternalFormats.push_back(GL_RGBA16F);
	m_supportedInternalFormats.push_back(GL_R11F_G11F_B10F);
	m_supportedInternalFormats.push_back(GL_RGB9_E5);
}

/** Executes test iteration.
 *
 *  @return Returns STOP when test has finished executing, CONTINUE if more iterations are needed.
 */
tcu::TestNode::IterateResult TextureFilterAnisotropicDrawingTestCase::iterate()
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	bool result = true;

	GLfloat maxAnisoDegree = 2.0;

	gl.getFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAnisoDegree);
	GLU_EXPECT_NO_ERROR(gl.getError(), "getFloatv");

	std::vector<GLfloat> anisoVec;
	anisoVec.push_back(1.0f);
	anisoVec.push_back(2.0f);

	for (deUint32 iTarget = 0; iTarget < m_supportedTargets.size(); ++iTarget)
	{
		GLenum target = m_supportedTargets[iTarget];

		for (deUint32 iFormat = 0; iFormat < m_supportedInternalFormats.size(); ++iFormat)
		{
			GLenum format = m_supportedInternalFormats[iFormat];

			// Generate texture
			generateTexture(gl, target);

			// Fill texture with strips pattern
			fillTexture(gl, target, format);

			// Draw scene
			GLuint lastPoints = 0xFFFFFFFF;
			for (deUint32 i = 0; i < anisoVec.size(); ++i)
			{
				GLfloat aniso = anisoVec[i];

				if (result)
					result = result && drawTexture(gl, target, aniso);

				// Verify result
				if (result)
				{
					GLuint currentPoints = verifyScene(gl);

					if (lastPoints <= currentPoints)
					{
						m_testCtx.getLog()
							<< tcu::TestLog::Message
							<< "Anisotropy verification failed (lastPoints <= currentPoints) for "
							<< "anisotropy: " << aniso << ", "
							<< "target: " << glu::getTextureTargetName(target) << ", "
							<< "internalFormat: " << glu::getUncompressedTextureFormatName(format) << ", "
							<< "lastPoints: " << lastPoints << ", "
							<< "currentPoints: " << currentPoints << tcu::TestLog::EndMessage;

						result = false;
						break;
					}

					lastPoints = currentPoints;
				}
			}

			// Release texture
			releaseTexture(gl);

			if (!result)
			{
				// Stop loops
				iTarget = m_supportedTargets.size();
				iFormat = m_supportedInternalFormats.size();
			}
		}
	}

	if (result)
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	else
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
	return STOP;
}

/** Generate texture and set filtering parameters.
 *
 *  @param gl              OpenGL functions wrapper
 *  @param target          Texture target
 *  @param internalFormat  Texture internal format
 */
void TextureFilterAnisotropicDrawingTestCase::generateTexture(const glw::Functions& gl, GLenum target)
{
	gl.genTextures(1, &m_texture);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenTextures");
	gl.bindTexture(target, m_texture);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture");

	gl.texParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTexParameteri");
	gl.texParameteri(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTexParameteri");
	gl.texParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTexParameteri");
	gl.texParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTexParameteri");
	gl.texParameteri(target, GL_TEXTURE_MAX_LEVEL, 1);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glTexParameteri");
}

/** Fill texture with strips pattern.
 *
 *  @param gl              OpenGL functions wrapper
 *  @param target          Texture target
 *  @param internalFormat  Texture internal format
 */
void TextureFilterAnisotropicDrawingTestCase::fillTexture(const glw::Functions& gl, GLenum target,
														  GLenum internalFormat)
{
	tcu::TextureFormat  texFormat   = glu::mapGLInternalFormat(internalFormat);
	glu::TransferFormat transFormat = glu::getTransferFormat(texFormat);

	for (int l = 0; l < 2; ++l)
	{
		GLuint texSize = 32 / (l + 1);

		std::vector<GLubyte> vecData;
		vecData.resize(texSize * texSize * texFormat.getPixelSize() * 2);

		tcu::PixelBufferAccess bufferAccess(texFormat, texSize, texSize, 1, vecData.data());

		for (GLuint x = 0; x < texSize; ++x)
		{
			for (GLuint y = 0; y < texSize; ++y)
			{
				int		  value = ((x * (l + 1)) % 8 < 4) ? 255 : 0;
				tcu::RGBA rgbaColor(value, value, value, 255);
				tcu::Vec4 color = rgbaColor.toVec();
				bufferAccess.setPixel(color, x, y);
			}
		}

		TextureFilterAnisotropicUtils::texImage(gl, target, l, internalFormat, texSize, texSize, 1, transFormat.format,
												transFormat.dataType, vecData.data());
	}
}

/** Render polygon with anisotropic filtering.
 *
 *  @param gl           OpenGL functions wrapper
 *  @param target       Texture target
 *  @param anisoDegree  Degree of anisotropy
 *
 *  @return Returns true if no error occured, false otherwise.
 */
bool TextureFilterAnisotropicDrawingTestCase::drawTexture(const glw::Functions& gl, GLenum target, GLfloat anisoDegree)
{
	const GLfloat vertices2[] = { -1.0f, 0.0f, -0.5f, 0.0f, 0.0f, -4.0f, 4.0f, -2.0f, 0.0f, 1.0f,
								  1.0f,  0.0f, -0.5f, 1.0f, 0.0f, -2.0f, 4.0f, -2.0f, 1.0f, 1.0f };
	const GLfloat vertices3[] = { -1.0f, 0.0f, -0.5f, 0.0f, 0.0f, 0.0f, -4.0f, 4.0f, -2.0f, 0.0f, 1.0f, 0.0f,
								  1.0f,  0.0f, -0.5f, 1.0f, 0.0f, 0.0f, -2.0f, 4.0f, -2.0f, 1.0f, 1.0f, 0.0f };

	// Projection values.
	const GLfloat projectionMatrix[] = { 0.5f, 0.0f, 0.0f,		   0.0f,		 0.0f, 0.5f, 0.0f,  0.0f,
										 0.0f, 0.0f, -2.5f / 1.5f, -2.0f / 1.5f, 0.0f, 0.0f, -1.0f, 0.0f };

	gl.viewport(0, 0, 32, 32);

	std::string vertexShader   = m_vertex;
	std::string fragmentShader = m_fragment;

	std::string texCoordType;
	std::string samplerType;
	TextureFilterAnisotropicUtils::generateTokens(target, texCoordType, samplerType);

	TextureFilterAnisotropicUtils::replaceToken("<TEXCOORD_TYPE>", texCoordType.c_str(), vertexShader);
	TextureFilterAnisotropicUtils::replaceToken("<TEXCOORD_TYPE>", texCoordType.c_str(), fragmentShader);
	TextureFilterAnisotropicUtils::replaceToken("<SAMPLER_TYPE>", samplerType.c_str(), vertexShader);
	TextureFilterAnisotropicUtils::replaceToken("<SAMPLER_TYPE>", samplerType.c_str(), fragmentShader);

	if (glu::isContextTypeGLCore(m_context.getRenderContext().getType()))
	{
		TextureFilterAnisotropicUtils::replaceToken("<VERSION>", "130", vertexShader);
		TextureFilterAnisotropicUtils::replaceToken("<VERSION>", "130", fragmentShader);
	}
	else
	{
		TextureFilterAnisotropicUtils::replaceToken("<VERSION>", "300 es", vertexShader);
		TextureFilterAnisotropicUtils::replaceToken("<VERSION>", "300 es", fragmentShader);
	}

	ProgramSources sources = makeVtxFragSources(vertexShader, fragmentShader);
	ShaderProgram  program(gl, sources);

	if (!program.isOk())
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Shader build failed.\n"
						   << "Vertex: " << program.getShaderInfo(SHADERTYPE_VERTEX).infoLog << "\n"
						   << vertexShader << "\n"
						   << "Fragment: " << program.getShaderInfo(SHADERTYPE_FRAGMENT).infoLog << "\n"
						   << fragmentShader << "\n"
						   << "Program: " << program.getProgramInfo().infoLog << tcu::TestLog::EndMessage;
		return false;
	}

	GLuint vao;
	gl.genVertexArrays(1, &vao);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenVertexArrays");
	gl.bindVertexArray(vao);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindVertexArray");

	GLuint vbo;
	gl.genBuffers(1, &vbo);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGenBuffers");
	gl.bindBuffer(GL_ARRAY_BUFFER, vbo);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindBuffer");

	std::vector<GLfloat> vboData;
	vboData.resize(24);

	GLuint texCoordDim;
	if (texCoordType == "vec2")
	{
		texCoordDim = 2;
		deMemcpy((void*)vboData.data(), (void*)vertices2, sizeof(vertices2));
	}
	else
	{
		texCoordDim = 3;
		deMemcpy((void*)vboData.data(), (void*)vertices3, sizeof(vertices3));
	}

	gl.bufferData(GL_ARRAY_BUFFER, vboData.size() * sizeof(GLfloat), (GLvoid*)vboData.data(), GL_DYNAMIC_DRAW);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBufferData");

	gl.useProgram(program.getProgram());
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUseProgram");

	GLuint matrixLocation = gl.getUniformLocation(program.getProgram(), "projectionMatrix");
	GLuint texLocation	= gl.getUniformLocation(program.getProgram(), "tex");

	gl.activeTexture(GL_TEXTURE0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glActiveTexture");
	gl.bindTexture(target, m_texture);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glBindTexture");
	gl.uniformMatrix4fv(matrixLocation, 1, GL_FALSE, projectionMatrix);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUniformMatrix4fv");
	gl.uniform1i(texLocation, 0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glUniform1i");

	gl.texParameterf(target, GL_TEXTURE_MAX_ANISOTROPY_EXT, anisoDegree);
	GLU_EXPECT_NO_ERROR(gl.getError(), "texParameterfv");

	gl.clearColor(0.0f, 0.0f, 0.0f, 1.0f);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glClearColor");
	gl.clear(GL_COLOR_BUFFER_BIT);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glClear");

	gl.enableVertexAttribArray(0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glEnableVertexAttribArray");
	gl.enableVertexAttribArray(1);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glEnableVertexAttribArray");

	GLint attrLocationVertex = gl.getAttribLocation(program.getProgram(), "vertex");
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetAttribLocation");
	GLint attrLocationInTexCoord = gl.getAttribLocation(program.getProgram(), "inTexCoord");
	GLU_EXPECT_NO_ERROR(gl.getError(), "glGetAttribLocation");

	GLuint strideSize = (3 + texCoordDim) * sizeof(GLfloat);
	gl.vertexAttribPointer(attrLocationVertex, 3, GL_FLOAT, GL_FALSE, strideSize, DE_NULL);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glVertexAttribPointer");
	gl.vertexAttribPointer(attrLocationInTexCoord, texCoordDim, GL_FLOAT, GL_FALSE, strideSize,
						   (GLvoid*)(3 * sizeof(GLfloat)));
	GLU_EXPECT_NO_ERROR(gl.getError(), "glVertexAttribPointer");

	gl.drawArrays(GL_TRIANGLE_STRIP, 0, 4);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glDrawArray");

	gl.disableVertexAttribArray(0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glDisableVertexAttribArray");
	gl.disableVertexAttribArray(1);
	GLU_EXPECT_NO_ERROR(gl.getError(), "glDisableVertexAttribArray");

	if (vbo)
	{
		gl.deleteBuffers(1, &vbo);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glDeleteBuffers");
	}

	if (vao)
	{
		gl.deleteVertexArrays(1, &vao);
		GLU_EXPECT_NO_ERROR(gl.getError(), "glDeleteVertexArrays");
	}

	return true;
}

/** Verify rendered polygon anisotropy.
 *
 *  @param gl  OpenGL functions wrapper
 *
 *  @return Returns points value. Less points means better anisotropy (smoother strips).
 */
GLuint TextureFilterAnisotropicDrawingTestCase::verifyScene(const glw::Functions& gl)
{
	std::vector<GLubyte> pixels;
	pixels.resize(32 * 8 * 4);

	gl.readPixels(0, 23, 32, 8, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
	GLU_EXPECT_NO_ERROR(gl.getError(), "glReadPixels");

	GLuint sum = 0;

	GLubyte last	= 0;
	GLubyte current = 0;
	for (int j = 0; j < 8; ++j)
	{
		for (int i = 0; i < 32; ++i)
		{
			current = pixels[(i + j * 32) * 4];

			if (i > 0)
				sum += deAbs32((int)current - (int)last);

			last = current;
		}
	}

	return sum;
}

/** Release texture.
 *
 *  @param gl  OpenGL functions wrapper
 */
void TextureFilterAnisotropicDrawingTestCase::releaseTexture(const glw::Functions& gl)
{
	if (m_texture)
		gl.deleteTextures(1, &m_texture);

	m_texture = 0;
}

/** Constructor.
 *
 *  @param context Rendering context.
 */
TextureFilterAnisotropicTests::TextureFilterAnisotropicTests(deqp::Context& context)
	: TestCaseGroup(context, "texture_filter_anisotropic",
					"Verify conformance of CTS_EXT_texture_filter_anisotropic implementation")
{
}

/** Initializes the test group contents. */
void TextureFilterAnisotropicTests::init()
{
	addChild(new TextureFilterAnisotropicQueriesTestCase(m_context));
	addChild(new TextureFilterAnisotropicDrawingTestCase(m_context));
}

} /* glcts namespace */
