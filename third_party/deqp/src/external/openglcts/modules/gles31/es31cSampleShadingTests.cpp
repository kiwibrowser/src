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

#include "es31cSampleShadingTests.hpp"
#include "deRandom.hpp"
#include "deStringUtil.hpp"
#include "gluContextInfo.hpp"
#include "gluDrawUtil.hpp"
#include "gluPixelTransfer.hpp"
#include "gluShaderProgram.hpp"
#include "glw.h"
#include "glwFunctions.hpp"
#include "tcuCommandLine.hpp"
#include "tcuStringTemplate.hpp"
#include "tcuSurface.hpp"
#include "tcuTestLog.hpp"

namespace tcu
{
static bool operator<(tcu::Vec4 const& k1, tcu::Vec4 const& k2)
{
	if (k1.y() < k2.y())
	{
		return true;
	}
	else if (k1.y() == k2.y())
	{
		return k1.x() < k2.x();
	}
	else
	{
		return false;
	}
}
}

namespace glcts
{

using tcu::TestLog;
using std::string;
using std::vector;
using glcts::Context;

static std::string specializeVersion(std::string const& source, glu::GLSLVersion version, std::string const& sampler,
									 std::string const& outType)
{
	DE_ASSERT(version == glu::GLSL_VERSION_310_ES || version >= glu::GLSL_VERSION_400);
	std::map<std::string, std::string> args;
	args["VERSION_DECL"] = glu::getGLSLVersionDeclaration(version);
	args["SAMPLER"]		 = sampler;
	args["OUT_TYPE"]	 = outType;
	return tcu::StringTemplate(source.c_str()).specialize(args);
}

class SampleShadingApiCaseGroup : public TestCaseGroup
{
public:
	SampleShadingApiCaseGroup(Context& context, glu::GLSLVersion glslVersion)
		: TestCaseGroup(context, "api", "Basic API verification"), m_glslVersion(glslVersion)
	{
	}

	void init(void)
	{
		addChild(new SampleShadingApiCase(m_context, m_glslVersion));
	}

private:
	class SampleShadingApiCase : public deqp::TestCase
	{
	public:
		SampleShadingApiCase(Context& context, glu::GLSLVersion glslVersion);
		~SampleShadingApiCase();

		IterateResult iterate();

	protected:
		glu::GLSLVersion			m_glslVersion;
		glw::glMinSampleShadingFunc m_pGLMinSampleShading;
	};

	glu::GLSLVersion m_glslVersion;
};

SampleShadingApiCaseGroup::SampleShadingApiCase::SampleShadingApiCase(Context& context, glu::GLSLVersion glslVersion)
	: TestCase(context, "verify", ""), m_glslVersion(glslVersion)
{
	DE_ASSERT(glslVersion == glu::GLSL_VERSION_310_ES || glslVersion >= glu::GLSL_VERSION_400);
}

SampleShadingApiCaseGroup::SampleShadingApiCase::~SampleShadingApiCase()
{
}

SampleShadingApiCaseGroup::SampleShadingApiCase::IterateResult SampleShadingApiCaseGroup::SampleShadingApiCase::
	iterate()
{
	const glw::Functions& gl   = m_context.getRenderContext().getFunctions();
	bool				  isOk = true;

	if (m_glslVersion == glu::GLSL_VERSION_310_ES &&
		!m_context.getContextInfo().isExtensionSupported("GL_OES_sample_shading"))
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "GL_OES_sample_shading");
		return STOP;
	}

	m_pGLMinSampleShading = gl.minSampleShading;

	struct Test
	{
		GLfloat input;
		GLfloat result;
	} tests[] = {
		{ 0.0f, 0.0f }, { 0.5f, 0.5f }, { -1.0f, 0.0f }, { 2.0f, 1.0f },
	};
	for (int i = 0; i < DE_LENGTH_OF_ARRAY(tests); ++i)
	{
		m_pGLMinSampleShading(tests[i].input);
		GLfloat result = -1.0f;
		gl.getFloatv(GL_MIN_SAMPLE_SHADING_VALUE_OES, &result);
		if (result != tests[i].result)
		{
			isOk = false;
		}
	}

	gl.enable(GL_SAMPLE_SHADING_OES);
	if (!gl.isEnabled(GL_SAMPLE_SHADING_OES))
	{
		isOk = false;
	}
	gl.disable(GL_SAMPLE_SHADING_OES);
	if (gl.isEnabled(GL_SAMPLE_SHADING_OES))
	{
		isOk = false;
	}

	m_pGLMinSampleShading(0.0f);

	m_testCtx.setTestResult(isOk ? QP_TEST_RESULT_PASS : QP_TEST_RESULT_FAIL, isOk ? "Pass" : "Fail");
	return STOP;
}

class SampleShadingRenderCase : public TestCase
{
public:
	SampleShadingRenderCase(Context& context, const char* name, const char* description, glu::GLSLVersion glslVersion,
							GLenum internalFormat, tcu::TextureFormat const& texFormat, const char* m_sampler,
							const char* m_outType, GLfloat min, GLfloat max, const char* m_extension,
							GLfloat sampleShading);
	~SampleShadingRenderCase();

	IterateResult iterate();

protected:
	glu::GLSLVersion			m_glslVersion;
	glw::glMinSampleShadingFunc m_pGLMinSampleShading;
	GLenum						m_internalFormat;
	tcu::TextureFormat			m_texFormat;
	std::string					m_sampler;
	std::string					m_outType;
	GLfloat						m_min;
	GLfloat						m_max;
	std::string					m_extension;
	GLfloat						m_sampleShading;

	enum
	{
		WIDTH		= 16,
		HEIGHT		= 16,
		MAX_SAMPLES = 4,
	};

	int countUniquePixels(tcu::ConstPixelBufferAccess const& pixels);
	int countUniquePixels(const std::vector<tcu::Vec4>& pixels);
};

SampleShadingRenderCase::SampleShadingRenderCase(Context& context, const char* name, const char* description,
												 glu::GLSLVersion glslVersion, GLenum internalFormat,
												 tcu::TextureFormat const& texFormat, const char* sampler,
												 const char* outType, GLfloat min, GLfloat max, const char* extension,
												 GLfloat sampleShading)
	: TestCase(context, name, description)
	, m_glslVersion(glslVersion)
	, m_internalFormat(internalFormat)
	, m_texFormat(texFormat)
	, m_sampler(sampler)
	, m_outType(outType)
	, m_min(min)
	, m_max(max)
	, m_extension(extension)
	, m_sampleShading(sampleShading)
{
	DE_ASSERT(glslVersion == glu::GLSL_VERSION_310_ES || glslVersion >= glu::GLSL_VERSION_400);
}

SampleShadingRenderCase::~SampleShadingRenderCase()
{
}

SampleShadingRenderCase::IterateResult SampleShadingRenderCase::iterate()
{
	TestLog&			  log  = m_testCtx.getLog();
	const glw::Functions& gl   = m_context.getRenderContext().getFunctions();
	bool				  isOk = true;

	if (m_glslVersion == glu::GLSL_VERSION_310_ES &&
		!m_context.getContextInfo().isExtensionSupported("GL_OES_sample_shading"))
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "GL_OES_sample_shading");
		return STOP;
	}
	if (!m_extension.empty() && !m_context.getContextInfo().isExtensionSupported(m_extension.c_str()))
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, m_extension.c_str());
		return STOP;
	}

	m_pGLMinSampleShading = gl.minSampleShading;

	GLint maxSamples = 0;
	if (((m_texFormat.type == tcu::TextureFormat::FLOAT) && (m_texFormat.order == tcu::TextureFormat::RGBA)) ||
		((m_texFormat.type == tcu::TextureFormat::FLOAT) && (m_texFormat.order == tcu::TextureFormat::RG)) ||
		((m_texFormat.type == tcu::TextureFormat::FLOAT) && (m_texFormat.order == tcu::TextureFormat::R)) ||
		((m_texFormat.type == tcu::TextureFormat::HALF_FLOAT) && (m_texFormat.order == tcu::TextureFormat::RGBA)))
	{
		gl.getInternalformativ(GL_TEXTURE_2D_MULTISAMPLE, m_internalFormat, GL_SAMPLES, 1, &maxSamples);
		if (maxSamples == 0)
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "Multisample is not supported on this format");
			return STOP;
		}
	}
	else if (m_texFormat.type == tcu::TextureFormat::SIGNED_INT8 ||
			 m_texFormat.type == tcu::TextureFormat::UNSIGNED_INT8)
	{
		gl.getIntegerv(GL_MAX_INTEGER_SAMPLES, &maxSamples);
	}
	else
	{
		gl.getIntegerv(GL_MAX_SAMPLES, &maxSamples);
	}
	GLint samples = de::min<GLint>(maxSamples, MAX_SAMPLES);

	GLuint tex;
	gl.genTextures(1, &tex);
	gl.bindTexture(GL_TEXTURE_2D_MULTISAMPLE, tex);
	gl.texStorage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, m_internalFormat, WIDTH, HEIGHT, GL_FALSE);

	GLuint fboMs;
	gl.genFramebuffers(1, &fboMs);
	gl.bindFramebuffer(GL_FRAMEBUFFER, fboMs);
	gl.framebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, tex, 0);
	gl.viewport(0, 0, WIDTH, HEIGHT);

	m_pGLMinSampleShading(m_sampleShading);
	gl.enable(GL_SAMPLE_SHADING_OES);

	static const deUint16 quadIndices[] = { 0, 1, 2, 2, 1, 3 };

	{
		static char const* vss = "${VERSION_DECL}\n"
								 "in highp vec2 a_position;\n"
								 "in highp vec4 a_color;\n"
								 "out highp vec4 v_color;\n"
								 "void main (void)\n"
								 "{\n"
								 "   gl_Position = vec4(a_position, 0.0, 1.0);\n"
								 "   v_color = a_color;\n"
								 "}\n";

		static char const* fss = "${VERSION_DECL}\n"
								 "in highp vec4 v_color;\n"
								 "layout(location = 0) out highp ${OUT_TYPE} o_color;\n"
								 "void main (void)\n"
								 "{\n"
								 "   o_color = ${OUT_TYPE}(v_color.x, v_color.y, 0.0, 0.0);\n"
								 "}\n";

		glu::ShaderProgram program(
			m_context.getRenderContext(),
			glu::makeVtxFragSources(specializeVersion(vss, m_glslVersion, m_sampler, m_outType).c_str(),
									specializeVersion(fss, m_glslVersion, m_sampler, m_outType).c_str()));
		log << program;
		if (!program.isOk())
		{
			TCU_FAIL("Compile failed");
		}

		const float position[] = {
			-1.0f, -1.0f, -1.0f, +1.0f, +1.0f, -1.0f, +1.0f, +1.0f,
		};
		const float color[] = {
			m_min, m_min, 0.0f, 1.0f, m_min, m_max, 0.0f, 1.0f, m_max, m_min, 0.0f, 1.0f, m_max, m_max, 0.0f, 1.0f,
		};

		gl.useProgram(program.getProgram());

		glu::VertexArrayBinding vertexArrays[] = {
			glu::va::Float("a_position", 2, 4, 0, &position[0]), glu::va::Float("a_color", 4, 4, 0, &color[0]),
		};
		glu::draw(m_context.getRenderContext(), program.getProgram(), DE_LENGTH_OF_ARRAY(vertexArrays),
				  &vertexArrays[0], glu::pr::TriangleStrip(DE_LENGTH_OF_ARRAY(quadIndices), &quadIndices[0]));

		GLU_EXPECT_NO_ERROR(gl.getError(), "Draw quad");
	}
	m_pGLMinSampleShading(0.0f);
	gl.disable(GL_SAMPLE_SHADING_OES);
	gl.bindFramebuffer(GL_FRAMEBUFFER, m_context.getRenderContext().getDefaultFramebuffer());
	gl.deleteFramebuffers(1, &fboMs);

	GLsizei width = WIDTH * samples;

	GLuint rbo;
	gl.genRenderbuffers(1, &rbo);
	gl.bindRenderbuffer(GL_RENDERBUFFER, rbo);
	gl.renderbufferStorage(GL_RENDERBUFFER, m_internalFormat, width, HEIGHT);

	GLuint fbo;
	gl.genFramebuffers(1, &fbo);
	gl.bindFramebuffer(GL_FRAMEBUFFER, fbo);
	gl.framebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, rbo);
	gl.viewport(0, 0, width, HEIGHT);

	{
		static char const* vss = "${VERSION_DECL}\n"
								 "in highp vec2 a_position;\n"
								 "void main(void)\n"
								 "{\n"
								 "   gl_Position = vec4(a_position, 0.0, 1.0);\n"
								 "}\n";

		static char const* fss = "${VERSION_DECL}\n"
								 "uniform highp ${SAMPLER} u_tex;\n"
								 "uniform int u_samples;\n"
								 "layout(location = 0) out highp ${OUT_TYPE} o_color;\n"
								 "void main(void)\n"
								 "{\n"
								 "   ivec2 coord = ivec2(int(gl_FragCoord.x) / u_samples, gl_FragCoord.y);\n"
								 "   int sampleId = int(gl_FragCoord.x) % u_samples;\n"
								 "   o_color = texelFetch(u_tex, coord, sampleId);\n"
								 "}\n";

		glu::ShaderProgram program(
			m_context.getRenderContext(),
			glu::makeVtxFragSources(specializeVersion(vss, m_glslVersion, m_sampler, m_outType).c_str(),
									specializeVersion(fss, m_glslVersion, m_sampler, m_outType).c_str()));
		log << program;
		if (!program.isOk())
		{
			TCU_FAIL("Compile failed");
		}

		float const position[] = {
			-1.0f, -1.0f, -1.0f, +1.0f, +1.0f, -1.0f, +1.0f, +1.0f,
		};

		gl.useProgram(program.getProgram());
		gl.uniform1i(gl.getUniformLocation(program.getProgram(), "u_samples"), samples);
		gl.uniform1i(gl.getUniformLocation(program.getProgram(), "u_tex"), 0);

		glu::VertexArrayBinding vertexArrays[] = {
			glu::va::Float("a_position", 2, 4, 0, &position[0]),
		};
		glu::draw(m_context.getRenderContext(), program.getProgram(), DE_LENGTH_OF_ARRAY(vertexArrays),
				  &vertexArrays[0], glu::pr::TriangleStrip(DE_LENGTH_OF_ARRAY(quadIndices), &quadIndices[0]));

		GLU_EXPECT_NO_ERROR(gl.getError(), "Draw quad");
	}

	tcu::TextureLevel	  results(m_texFormat, width, HEIGHT);
	tcu::PixelBufferAccess pixels = results.getAccess();
	std::vector<tcu::Vec4> result(pixels.getHeight() * pixels.getWidth());
	int					   uniquePixels;

	if (pixels.getFormat().type == tcu::TextureFormat::SIGNED_INT8)
	{
		std::vector<GLint> data(pixels.getHeight() * pixels.getWidth() * 4);
		gl.readPixels(0, 0, pixels.getWidth(), pixels.getHeight(), GL_RGBA_INTEGER, GL_INT, &data[0]);
		for (unsigned int i = 0; i < data.size(); i += 4)
		{
			result[i / 4] =
				tcu::Vec4((GLfloat)data[i], (GLfloat)data[i + 1], (GLfloat)data[i + 2], (GLfloat)data[i + 3]);
		}
		uniquePixels = countUniquePixels(result);
	}
	else if (pixels.getFormat().type == tcu::TextureFormat::UNSIGNED_INT8)
	{
		std::vector<GLuint> data(pixels.getHeight() * pixels.getWidth() * 4);
		gl.readPixels(0, 0, pixels.getWidth(), pixels.getHeight(), GL_RGBA_INTEGER, GL_UNSIGNED_INT, &data[0]);
		for (unsigned int i = 0; i < data.size(); i += 4)
		{
			result[i / 4] =
				tcu::Vec4((GLfloat)data[i], (GLfloat)data[i + 1], (GLfloat)data[i + 2], (GLfloat)data[i + 3]);
		}
		uniquePixels = countUniquePixels(result);
	}
	else
	{
		glu::readPixels(m_context.getRenderContext(), 0, 0, pixels);
		uniquePixels = countUniquePixels(pixels);
	}
	int expectedUnique = WIDTH * HEIGHT * (de::clamp(int(float(samples) * m_sampleShading), 1, samples));
	if (uniquePixels < expectedUnique)
	{
		isOk = false;
	}

	gl.bindFramebuffer(GL_FRAMEBUFFER, m_context.getRenderContext().getDefaultFramebuffer());
	gl.deleteFramebuffers(1, &fbo);

	gl.bindRenderbuffer(GL_RENDERBUFFER, 0);
	gl.deleteRenderbuffers(1, &rbo);

	gl.bindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);
	gl.deleteTextures(1, &tex);

	m_testCtx.setTestResult(isOk ? QP_TEST_RESULT_PASS : QP_TEST_RESULT_FAIL, isOk ? "Pass" : "Fail");
	return STOP;
}

int SampleShadingRenderCase::countUniquePixels(tcu::ConstPixelBufferAccess const& pixels)
{
	std::set<tcu::Vec4> uniquePixels;

	for (int y = 0; y < pixels.getHeight(); ++y)
	{
		for (int x = 0; x < pixels.getWidth(); ++x)
		{
			uniquePixels.insert(pixels.getPixel(x, y));
		}
	}

	return (int)uniquePixels.size();
}

int SampleShadingRenderCase::countUniquePixels(const std::vector<tcu::Vec4>& pixels)
{
	std::set<tcu::Vec4> uniquePixels;

	for (unsigned int i = 0; i < pixels.size(); ++i)
	{
		uniquePixels.insert(pixels[i]);
	}

	return (int)uniquePixels.size();
}

class SampleShadingRenderFormatTests : public glcts::TestCaseGroup
{
public:
	SampleShadingRenderFormatTests(glcts::Context& context, glu::GLSLVersion glslVersion, GLenum internalFormat,
								   const char* format, tcu::TextureFormat const& texFormat, const char* sampler,
								   const char* outType, GLfloat min, GLfloat max, const char* extension = "");
	~SampleShadingRenderFormatTests(void);

	void init(void);

private:
	SampleShadingRenderFormatTests(const SampleShadingTests& other);
	SampleShadingRenderFormatTests& operator=(const SampleShadingTests& other);

	glu::GLSLVersion   m_glslVersion;
	GLenum			   m_internalFormat;
	tcu::TextureFormat m_texFormat;
	std::string		   m_sampler;
	std::string		   m_outType;
	GLfloat			   m_min;
	GLfloat			   m_max;
	std::string		   m_extension;
};

SampleShadingRenderFormatTests::SampleShadingRenderFormatTests(Context& context, glu::GLSLVersion glslVersion,
															   GLenum internalFormat, const char* format,
															   tcu::TextureFormat const& texFormat, const char* sampler,
															   const char* outType, GLfloat min, GLfloat max,
															   const char* extension)
	: TestCaseGroup(context, format, "")
	, m_glslVersion(glslVersion)
	, m_internalFormat(internalFormat)
	, m_texFormat(texFormat)
	, m_sampler(sampler)
	, m_outType(outType)
	, m_min(min)
	, m_max(max)
	, m_extension(extension)
{
}

SampleShadingRenderFormatTests::~SampleShadingRenderFormatTests(void)
{
}

void SampleShadingRenderFormatTests::init(void)
{
	// sample_shading.render.full
	addChild(new SampleShadingRenderCase(m_context, "full", "Sample shader functionality", m_glslVersion,
										 m_internalFormat, m_texFormat, m_sampler.c_str(), m_outType.c_str(), m_min,
										 m_max, m_extension.c_str(), 1.0));
	// sample_shading.render.half
	addChild(new SampleShadingRenderCase(m_context, "half", "Sample shader functionality", m_glslVersion,
										 m_internalFormat, m_texFormat, m_sampler.c_str(), m_outType.c_str(), m_min,
										 m_max, m_extension.c_str(), 0.5));
	// sample_shading.render.none
	addChild(new SampleShadingRenderCase(m_context, "none", "Sample shader functionality", m_glslVersion,
										 m_internalFormat, m_texFormat, m_sampler.c_str(), m_outType.c_str(), m_min,
										 m_max, m_extension.c_str(), 0.0));
}

class SampleShadingRenderTests : public glcts::TestCaseGroup
{
public:
	SampleShadingRenderTests(glcts::Context& context, glu::GLSLVersion glslVersion);
	~SampleShadingRenderTests(void);

	void init(void);

private:
	SampleShadingRenderTests(const SampleShadingTests& other);
	SampleShadingRenderTests& operator=(const SampleShadingTests& other);

	glu::GLSLVersion m_glslVersion;
};

SampleShadingRenderTests::SampleShadingRenderTests(Context& context, glu::GLSLVersion glslVersion)
	: TestCaseGroup(context, "render", "Sample Shading render tests"), m_glslVersion(glslVersion)
{
}

SampleShadingRenderTests::~SampleShadingRenderTests(void)
{
}

void SampleShadingRenderTests::init(void)
{
	// sample_shading.render.rgba8
	addChild(new SampleShadingRenderFormatTests(
		m_context, m_glslVersion, GL_RGBA8, "rgba8",
		tcu::TextureFormat(tcu::TextureFormat::RGBA, tcu::TextureFormat::UNORM_INT8), "sampler2DMS", "vec4", 0.0, 1.0));
	// sample_shading.render.rgba8i
	addChild(new SampleShadingRenderFormatTests(
		m_context, m_glslVersion, GL_RGBA8I, "rgba8i",
		tcu::TextureFormat(tcu::TextureFormat::RGBA, tcu::TextureFormat::SIGNED_INT8), "isampler2DMS", "ivec4", -128.0,
		127.0));
	// sample_shading.render.rgba8ui
	addChild(new SampleShadingRenderFormatTests(
		m_context, m_glslVersion, GL_RGBA8UI, "rgba8ui",
		tcu::TextureFormat(tcu::TextureFormat::RGBA, tcu::TextureFormat::UNSIGNED_INT8), "usampler2DMS", "uvec4", 0.0,
		255.0));
	// sample_shading.render.rgba32f
	const char* extension =
		(glu::isContextTypeES(m_context.getRenderContext().getType())) ? "GL_EXT_color_buffer_float" : "";
	addChild(new SampleShadingRenderFormatTests(m_context, m_glslVersion, GL_RGBA32F, "rgba32f",
												tcu::TextureFormat(tcu::TextureFormat::RGBA, tcu::TextureFormat::FLOAT),
												"sampler2DMS", "vec4", 0.0, 1.0, extension));
}

SampleShadingTests::SampleShadingTests(Context& context, glu::GLSLVersion glslVersion)
	: TestCaseGroup(context, "sample_shading", "Sample Shading tests"), m_glslVersion(glslVersion)
{
}

SampleShadingTests::~SampleShadingTests(void)
{
}

void SampleShadingTests::init(void)
{
	// sample_shading.api
	addChild(new SampleShadingApiCaseGroup(m_context, m_glslVersion));
	// sample_shading.render
	addChild(new SampleShadingRenderTests(m_context, m_glslVersion));
}

} // glcts
