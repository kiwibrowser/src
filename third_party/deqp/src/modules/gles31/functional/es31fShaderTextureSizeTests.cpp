/*-------------------------------------------------------------------------
 * drawElements Quality Program OpenGL ES 3.1 Module
 * -------------------------------------------------
 *
 * Copyright 2014 The Android Open Source Project
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
 *//*!
 * \file
 * \brief Multisample texture size tests
 *//*--------------------------------------------------------------------*/

#include "es31fShaderTextureSizeTests.hpp"
#include "gluRenderContext.hpp"
#include "gluShaderProgram.hpp"
#include "gluPixelTransfer.hpp"
#include "gluContextInfo.hpp"
#include "glwEnums.hpp"
#include "glwFunctions.hpp"
#include "tcuTestLog.hpp"
#include "tcuStringTemplate.hpp"
#include "tcuSurface.hpp"
#include "tcuRenderTarget.hpp"
#include "deStringUtil.hpp"

using namespace glw;

namespace deqp
{
namespace gles31
{
namespace Functional
{
namespace
{

static const char* const s_positionVertexShaderSource =	"${GLSL_VERSION_DECL}\n"
														"in highp vec4 a_position;\n"
														"void main (void)\n"
														"{\n"
														"	gl_Position = a_position;\n"
														"}\n";

static std::string specializeShader(Context& context, const char* code)
{
	glu::GLSLVersion glslVersion = glu::getContextTypeGLSLVersion(context.getRenderContext().getType());
	std::map<std::string, std::string> specializationMap;

	specializationMap["GLSL_VERSION_DECL"] = glu::getGLSLVersionDeclaration(glslVersion);

	return tcu::StringTemplate(code).specialize(specializationMap);
}

class TextureSizeCase : public TestCase
{
public:
	enum TextureType
	{
		TEXTURE_FLOAT_2D = 0,
		TEXTURE_FLOAT_2D_ARRAY,
		TEXTURE_INT_2D,
		TEXTURE_INT_2D_ARRAY,
		TEXTURE_UINT_2D,
		TEXTURE_UINT_2D_ARRAY,

		TEXTURE_LAST
	};

							TextureSizeCase		(Context& context, const char* name, const char* desc, TextureType type, int samples);
							~TextureSizeCase	(void);

private:
	void					init				(void);
	void					deinit				(void);
	IterateResult			iterate				(void);

	std::string				genFragmentSource	(void);
	glw::GLenum				getTextureGLTarget	(void);
	glw::GLenum				getTextureGLInternalFormat (void);

	void					createTexture		(const tcu::IVec3& size);
	void					deleteTexture		(void);
	void					runShader			(tcu::Surface& dst, const tcu::IVec3& size);
	bool					verifyImage			(const tcu::Surface& dst);

	const TextureType		m_type;
	const int				m_numSamples;
	const bool				m_isArrayType;

	glw::GLuint				m_texture;
	glw::GLuint				m_vbo;
	glu::ShaderProgram*		m_shader;
	std::vector<tcu::IVec3>	m_iterations;
	int						m_iteration;

	bool					m_allIterationsPassed;
	bool					m_allCasesSkipped;
};

TextureSizeCase::TextureSizeCase (Context& context, const char* name, const char* desc, TextureType type, int samples)
	: TestCase				(context, name, desc)
	, m_type				(type)
	, m_numSamples			(samples)
	, m_isArrayType			(m_type == TEXTURE_FLOAT_2D_ARRAY || m_type == TEXTURE_INT_2D_ARRAY || m_type == TEXTURE_UINT_2D_ARRAY)
	, m_texture				(0)
	, m_vbo					(0)
	, m_shader				(DE_NULL)
	, m_iteration			(0)
	, m_allIterationsPassed	(true)
	, m_allCasesSkipped		(true)
{
	DE_ASSERT(type < TEXTURE_LAST);
}

TextureSizeCase::~TextureSizeCase (void)
{
	deinit();
}

void TextureSizeCase::init (void)
{
	static const tcu::IVec2 testSizes2D[] =
	{
		tcu::IVec2(1,	1),
		tcu::IVec2(1,	4),
		tcu::IVec2(4,	8),
		tcu::IVec2(21,	11),
		tcu::IVec2(107,	254),
		tcu::IVec2(-1,	3),
		tcu::IVec2(3,	-1),
	};
	static const tcu::IVec3 testSizes3D[] =
	{
		tcu::IVec3(1,	1,		1),
		tcu::IVec3(1,	4,		7),
		tcu::IVec3(4,	8,		12),
		tcu::IVec3(21,	11,		9),
		tcu::IVec3(107,	254,	2),
		tcu::IVec3(-1,	3,		3),
		tcu::IVec3(3,	-1,		3),
		tcu::IVec3(4,	4,		-1),
	};
	static const tcu::Vec4 fullscreenQuad[] =
	{
		tcu::Vec4(-1.0f,  1.0f, 0.0f, 1.0f),
		tcu::Vec4(-1.0f, -1.0f, 0.0f, 1.0f),
		tcu::Vec4( 1.0f,  1.0f, 0.0f, 1.0f),
		tcu::Vec4( 1.0f, -1.0f, 0.0f, 1.0f)
	};

	const glw::Functions&	gl					= m_context.getRenderContext().getFunctions();
	const bool				supportsES32		= glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::es(3, 2));

	// requirements
	if (m_isArrayType && !supportsES32 && !m_context.getContextInfo().isExtensionSupported("GL_OES_texture_storage_multisample_2d_array"))
		TCU_THROW(NotSupportedError, "Test requires OES_texture_storage_multisample_2d_array extension");

	if (m_context.getRenderTarget().getWidth() < 1 || m_context.getRenderTarget().getHeight() < 1)
		TCU_THROW(NotSupportedError, "rendertarget size must be at least 1x1");

	glw::GLint				maxTextureSize		= 0;
	glw::GLint				maxTextureLayers	= 0;
	glw::GLint				maxSamples			= 0;

	gl.getIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);
	gl.getIntegerv(GL_MAX_ARRAY_TEXTURE_LAYERS, &maxTextureLayers);
	gl.getInternalformativ(getTextureGLTarget(), getTextureGLInternalFormat(), GL_SAMPLES, 1, &maxSamples);

	if (m_numSamples > maxSamples)
		TCU_THROW(NotSupportedError, "sample count is not supported");

	// gen shade

	m_shader = new glu::ShaderProgram(m_context.getRenderContext(), glu::ProgramSources() << glu::VertexSource(specializeShader(m_context, s_positionVertexShaderSource)) << glu::FragmentSource(genFragmentSource()));
	m_testCtx.getLog() << *m_shader;
	if (!m_shader->isOk())
		throw tcu::TestError("shader build failed");

	// gen buffer

	gl.genBuffers(1, &m_vbo);
	gl.bindBuffer(GL_ARRAY_BUFFER, m_vbo);
	gl.bufferData(GL_ARRAY_BUFFER, sizeof(fullscreenQuad), fullscreenQuad, GL_STATIC_DRAW);

	// gen iterations

	m_testCtx.getLog()
		<< tcu::TestLog::Message
		<< "GL_MAX_TEXTURE_SIZE = " << maxTextureSize << "\n"
		<< "GL_MAX_ARRAY_TEXTURE_LAYERS = " << maxTextureLayers
		<< tcu::TestLog::EndMessage;

	if (!m_isArrayType)
	{
		for (int ndx = 0; ndx < DE_LENGTH_OF_ARRAY(testSizes2D); ++ndx)
		{
			if (testSizes2D[ndx].x() <= maxTextureSize && testSizes2D[ndx].y() <= maxTextureSize)
			{
				const int w = (testSizes2D[ndx].x() < 0) ? (maxTextureSize) : (testSizes2D[ndx].x());
				const int h = (testSizes2D[ndx].y() < 0) ? (maxTextureSize) : (testSizes2D[ndx].y());

				m_iterations.push_back(tcu::IVec3(w, h, 0));
			}
		}
	}
	else
	{
		for (int ndx = 0; ndx < DE_LENGTH_OF_ARRAY(testSizes3D); ++ndx)
		{
			if (testSizes3D[ndx].x() <= maxTextureSize && testSizes3D[ndx].y() <= maxTextureSize && testSizes3D[ndx].z() <= maxTextureLayers)
			{
				const int w = (testSizes3D[ndx].x() < 0) ? (maxTextureSize)		: (testSizes3D[ndx].x());
				const int h = (testSizes3D[ndx].y() < 0) ? (maxTextureSize)		: (testSizes3D[ndx].y());
				const int d = (testSizes3D[ndx].z() < 0) ? (maxTextureLayers)	: (testSizes3D[ndx].z());

				m_iterations.push_back(tcu::IVec3(w, h, d));
			}
		}
	}
}

void TextureSizeCase::deinit (void)
{
	if (m_texture)
	{
		m_context.getRenderContext().getFunctions().deleteTextures(1, &m_texture);
		m_texture = 0;
	}

	if (m_vbo)
	{
		m_context.getRenderContext().getFunctions().deleteBuffers(1, &m_vbo);
		m_vbo = 0;
	}

	if (m_shader)
	{
		delete m_shader;
		m_shader = DE_NULL;
	}
}

TextureSizeCase::IterateResult TextureSizeCase::iterate (void)
{
	tcu::Surface	result		(1, 1);
	bool			skipTest	= false;

	m_testCtx.getLog() << tcu::TestLog::Message << "\nIteration " << (m_iteration+1) << " / " << (int)m_iterations.size() << tcu::TestLog::EndMessage;

	try
	{
		// set texture size

		createTexture(m_iterations[m_iteration]);

		// query texture size

		runShader(result, m_iterations[m_iteration]);
	}
	catch (glu::OutOfMemoryError&)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Got GL_OUT_OF_MEMORY, skipping this size" << tcu::TestLog::EndMessage;

		skipTest = true;
	}

	// free resources

	deleteTexture();

	// queried value was correct?

	if (!skipTest)
	{
		m_allCasesSkipped = false;

		if (!verifyImage(result))
			m_allIterationsPassed = false;
	}

	// final result

	if (++m_iteration < (int)m_iterations.size())
		return CONTINUE;

	if (!m_allIterationsPassed)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "One or more test sizes failed." << tcu::TestLog::EndMessage;
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Got invalid texture size");
	}
	else if (m_allCasesSkipped)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Could not test any texture size, texture creation failed." << tcu::TestLog::EndMessage;
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "All test texture creations failed");
	}
	else
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "All texture sizes passed." << tcu::TestLog::EndMessage;
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}

	return STOP;
}

std::string TextureSizeCase::genFragmentSource (void)
{
	static const char* const templateSource =	"${GLSL_VERSION_DECL}\n"
												"${EXTENSION_STATEMENT}"
												"layout(location = 0) out highp vec4 fragColor;\n"
												"uniform highp ${SAMPLERTYPE} u_sampler;\n"
												"uniform highp ${SIZETYPE} u_size;\n"
												"void main (void)\n"
												"{\n"
												"	const highp vec4 okColor = vec4(0.0, 1.0, 0.0, 1.0);\n"
												"	const highp vec4 failColor = vec4(1.0, 0.0, 0.0, 1.0);\n"
												"	fragColor = (textureSize(u_sampler) == u_size) ? (okColor) : (failColor);\n"
												"}\n";

	std::map<std::string, std::string> args;

	switch (m_type)
	{
		case TEXTURE_FLOAT_2D:			args["SAMPLERTYPE"] = "sampler2DMS";		break;
		case TEXTURE_FLOAT_2D_ARRAY:	args["SAMPLERTYPE"] = "sampler2DMSArray";	break;
		case TEXTURE_INT_2D:			args["SAMPLERTYPE"] = "isampler2DMS";		break;
		case TEXTURE_INT_2D_ARRAY:		args["SAMPLERTYPE"] = "isampler2DMSArray";	break;
		case TEXTURE_UINT_2D:			args["SAMPLERTYPE"] = "usampler2DMS";		break;
		case TEXTURE_UINT_2D_ARRAY:		args["SAMPLERTYPE"] = "usampler2DMSArray";	break;
		default:
			DE_ASSERT(DE_FALSE);
	}

	if (!m_isArrayType)
		args["SIZETYPE"] = "ivec2";
	else
		args["SIZETYPE"] = "ivec3";

	const glu::ContextType	contextType	= m_context.getRenderContext().getType();
	const bool				supportsES32	= glu::contextSupports(contextType, glu::ApiType::es(3, 2));

	if (m_isArrayType && !supportsES32)
		args["EXTENSION_STATEMENT"] = "#extension GL_OES_texture_storage_multisample_2d_array : require\n";
	else
		args["EXTENSION_STATEMENT"] = "";

	args["GLSL_VERSION_DECL"] = glu::getGLSLVersionDeclaration(glu::getContextTypeGLSLVersion(contextType));

	return tcu::StringTemplate(templateSource).specialize(args);
}

glw::GLenum TextureSizeCase::getTextureGLTarget (void)
{
	switch (m_type)
	{
		case TEXTURE_FLOAT_2D:			return GL_TEXTURE_2D_MULTISAMPLE;
		case TEXTURE_FLOAT_2D_ARRAY:	return GL_TEXTURE_2D_MULTISAMPLE_ARRAY;
		case TEXTURE_INT_2D:			return GL_TEXTURE_2D_MULTISAMPLE;
		case TEXTURE_INT_2D_ARRAY:		return GL_TEXTURE_2D_MULTISAMPLE_ARRAY;
		case TEXTURE_UINT_2D:			return GL_TEXTURE_2D_MULTISAMPLE;
		case TEXTURE_UINT_2D_ARRAY:		return GL_TEXTURE_2D_MULTISAMPLE_ARRAY;
		default:
			DE_ASSERT(DE_FALSE);
			return 0;
	}
}

glw::GLenum TextureSizeCase::getTextureGLInternalFormat (void)
{
	switch (m_type)
	{
		case TEXTURE_FLOAT_2D:			return GL_RGBA8;
		case TEXTURE_FLOAT_2D_ARRAY:	return GL_RGBA8;
		case TEXTURE_INT_2D:			return GL_R8I;
		case TEXTURE_INT_2D_ARRAY:		return GL_R8I;
		case TEXTURE_UINT_2D:			return GL_R8UI;
		case TEXTURE_UINT_2D_ARRAY:		return GL_R8UI;
		default:
			DE_ASSERT(DE_FALSE);
			return 0;
	}
}

void TextureSizeCase::createTexture (const tcu::IVec3& size)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (!m_isArrayType)
		m_testCtx.getLog() << tcu::TestLog::Message << "Creating texture with size " << size.x() << "x" << size.y() << tcu::TestLog::EndMessage;
	else
		m_testCtx.getLog() << tcu::TestLog::Message << "Creating texture with size " << size.x() << "x" << size.y() << "x" << size.z() << tcu::TestLog::EndMessage;

	gl.genTextures(1, &m_texture);
	gl.bindTexture(getTextureGLTarget(), m_texture);
	GLU_EXPECT_NO_ERROR(gl.getError(), "texture gen");

	if (!m_isArrayType)
		gl.texStorage2DMultisample(getTextureGLTarget(), m_numSamples, getTextureGLInternalFormat(), size.x(), size.y(), GL_FALSE);
	else
		gl.texStorage3DMultisample(getTextureGLTarget(), m_numSamples, getTextureGLInternalFormat(), size.x(), size.y(), size.z(), GL_FALSE);
	GLU_EXPECT_NO_ERROR(gl.getError(), "texStorage");
}

void TextureSizeCase::deleteTexture (void)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (m_texture)
	{
		gl.deleteTextures(1, &m_texture);
		m_texture = 0;

		GLU_EXPECT_NO_ERROR(gl.getError(), "texture delete");
	}
}

void TextureSizeCase::runShader (tcu::Surface& dst, const tcu::IVec3& size)
{
	const glw::Functions&	gl					= m_context.getRenderContext().getFunctions();
	const int				positionLoc			= gl.getAttribLocation(m_shader->getProgram(), "a_position");
	const int				shaderSamplerLoc	= gl.getUniformLocation(m_shader->getProgram(), "u_sampler");
	const int				shaderSizeLoc		= gl.getUniformLocation(m_shader->getProgram(), "u_size");

	m_testCtx.getLog() << tcu::TestLog::Message << "Running the verification shader." << tcu::TestLog::EndMessage;

	GLU_EXPECT_NO_ERROR(gl.getError(), "preclear");
	gl.viewport(0, 0, 1, 1);
	gl.clearColor(0.0f, 0.0f, 0.0f, 1.0f);
	gl.clear(GL_COLOR_BUFFER_BIT);
	GLU_EXPECT_NO_ERROR(gl.getError(), "clear");

	gl.bindBuffer(GL_ARRAY_BUFFER, m_vbo);
	gl.vertexAttribPointer(positionLoc, 4, GL_FLOAT, GL_FALSE, 0, DE_NULL);
	gl.enableVertexAttribArray(positionLoc);
	GLU_EXPECT_NO_ERROR(gl.getError(), "vertexAttrib");

	gl.useProgram(m_shader->getProgram());
	gl.uniform1i(shaderSamplerLoc, 0);
	if (m_isArrayType)
		gl.uniform3iv(shaderSizeLoc, 1, size.getPtr());
	else
		gl.uniform2iv(shaderSizeLoc, 1, size.getPtr());
	GLU_EXPECT_NO_ERROR(gl.getError(), "setup program");

	gl.bindTexture(getTextureGLTarget(), m_texture);
	GLU_EXPECT_NO_ERROR(gl.getError(), "bindtex");

	gl.drawArrays(GL_TRIANGLE_STRIP, 0, 4);
	GLU_EXPECT_NO_ERROR(gl.getError(), "drawArrays");

	gl.disableVertexAttribArray(positionLoc);
	gl.useProgram(0);
	GLU_EXPECT_NO_ERROR(gl.getError(), "cleanup");

	gl.finish();
	glu::readPixels(m_context.getRenderContext(), 0, 0, dst.getAccess());
	GLU_EXPECT_NO_ERROR(gl.getError(), "readPixels");
}

bool TextureSizeCase::verifyImage (const tcu::Surface& dst)
{
	DE_ASSERT(dst.getWidth() == 1 && dst.getHeight() == 1);

	const int		colorThresholdRed	= 1 << (8 - m_context.getRenderTarget().getPixelFormat().redBits);
	const int		colorThresholdGreen	= 1 << (8 - m_context.getRenderTarget().getPixelFormat().greenBits);
	const int		colorThresholdBlue	= 1 << (8 - m_context.getRenderTarget().getPixelFormat().blueBits);
	const tcu::RGBA	color				= dst.getPixel(0,0);

	m_testCtx.getLog() << tcu::TestLog::Message << "Verifying image." << tcu::TestLog::EndMessage;

	// green
	if (color.getRed() < colorThresholdRed && color.getGreen() > 255 - colorThresholdGreen && color.getBlue() < colorThresholdBlue)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Result ok." << tcu::TestLog::EndMessage;
		return true;
	}
	// red
	else if (color.getRed() > 255 - colorThresholdRed && color.getGreen() < colorThresholdGreen && color.getBlue() < colorThresholdBlue)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Image size incorrect." << tcu::TestLog::EndMessage;
		return false;
	}

	m_testCtx.getLog() << tcu::TestLog::Message << "Expected either green or red pixel, got " << color << tcu::TestLog::EndMessage;
	return false;
}

} // anonymous

ShaderTextureSizeTests::ShaderTextureSizeTests (Context& context)
	: TestCaseGroup(context, "texture_size", "Texture size tests")
{
}

ShaderTextureSizeTests::~ShaderTextureSizeTests (void)
{
}

void ShaderTextureSizeTests::init (void)
{
	static const struct SamplerType
	{
		TextureSizeCase::TextureType	type;
		const char*						name;
	} samplerTypes[] =
	{
		{ TextureSizeCase::TEXTURE_FLOAT_2D,		"texture_2d"			},
		{ TextureSizeCase::TEXTURE_FLOAT_2D_ARRAY,	"texture_2d_array"		},
		{ TextureSizeCase::TEXTURE_INT_2D,			"texture_int_2d"		},
		{ TextureSizeCase::TEXTURE_INT_2D_ARRAY,	"texture_int_2d_array"	},
		{ TextureSizeCase::TEXTURE_UINT_2D,			"texture_uint_2d"		},
		{ TextureSizeCase::TEXTURE_UINT_2D_ARRAY,	"texture_uint_2d_array"	},
	};

	static const int sampleCounts[] = { 1, 4 };

	for (int samplerTypeNdx = 0; samplerTypeNdx < DE_LENGTH_OF_ARRAY(samplerTypes); ++samplerTypeNdx)
	{
		for (int sampleCountNdx = 0; sampleCountNdx < DE_LENGTH_OF_ARRAY(sampleCounts); ++sampleCountNdx)
		{
			const std::string name = std::string() + "samples_" + de::toString(sampleCounts[sampleCountNdx]) + "_" + samplerTypes[samplerTypeNdx].name;
			const std::string desc = std::string() + "samples count = " + de::toString(sampleCounts[sampleCountNdx]) + ", type = " + samplerTypes[samplerTypeNdx].name;

			addChild(new TextureSizeCase(m_context, name.c_str(), desc.c_str(), samplerTypes[samplerTypeNdx].type, sampleCounts[sampleCountNdx]));
		}
	}
}

} // Functional
} // gles31
} // deqp
