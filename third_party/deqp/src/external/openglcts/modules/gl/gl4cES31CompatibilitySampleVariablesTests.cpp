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

#include "deMath.h"
#include "deRandom.hpp"
#include "deStringUtil.hpp"
#include "gl4cES31CompatibilityTests.hpp"
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

namespace gl4cts
{
namespace es31compatibility
{

using tcu::TestLog;
using std::string;
using std::vector;
using deqp::Context;

static std::string specializeVersion(std::string const& source, glu::GLSLVersion version,
									 std::string const& sampler = "", std::string const& outType = "")
{
	DE_ASSERT(version == glu::GLSL_VERSION_310_ES || version >= glu::GLSL_VERSION_400);
	std::map<std::string, std::string> args;
	args["VERSION_DECL"] = glu::getGLSLVersionDeclaration(version);
	args["SAMPLER"]		 = sampler;
	args["OUT_TYPE"]	 = outType;
	if (version == glu::GLSL_VERSION_310_ES)
	{
		args["OES_SV_RQ"] = "#extension GL_OES_sample_variables : require\n";
		args["OES_SV_EN"] = "#extension GL_OES_sample_variables : enable\n";
	}
	else
	{
		args["OES_SV_RQ"] = "";
		args["OES_SV_EN"] = "";
	}
	return tcu::StringTemplate(source.c_str()).specialize(args);
}

class SampleShadingExtensionCase : public deqp::TestCase
{
public:
	SampleShadingExtensionCase(Context& context, const char* name, const char* description,
							   glu::GLSLVersion glslVersion);
	~SampleShadingExtensionCase();

	IterateResult iterate();

protected:
	glu::GLSLVersion m_glslVersion;
};

SampleShadingExtensionCase::SampleShadingExtensionCase(Context& context, const char* name, const char* description,
													   glu::GLSLVersion glslVersion)
	: TestCase(context, name, description), m_glslVersion(glslVersion)
{
	DE_ASSERT(glslVersion == glu::GLSL_VERSION_310_ES);
}

SampleShadingExtensionCase::~SampleShadingExtensionCase()
{
}

SampleShadingExtensionCase::IterateResult SampleShadingExtensionCase::iterate()
{
	TestLog& log = m_testCtx.getLog();

	/* OpenGL support query. */
	bool is_at_least_gl_45 = (glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::core(4, 5)));
	bool is_arb_es31_compatibility = m_context.getContextInfo().isExtensionSupported("GL_ARB_ES3_1_compatibility");

	if (!(is_at_least_gl_45 || is_arb_es31_compatibility))
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "GL_ARB_ES3_1_compatibility");
		return STOP;
	}

	static char const* vss = "${VERSION_DECL}\n"
							 "in highp vec4 a_position;\n"
							 "void main()\n"
							 "{\n"
							 "    gl_Position = a_position;\n"
							 "}\n";

	{
		static char const* fss = "${VERSION_DECL}\n"
								 "${OES_SV_RQ}"
								 "out highp vec4 o_color;\n"
								 "void main()\n"
								 "{\n"
								 "    for (int i = 0; i < (gl_MaxSamples + 31) / 32; ++i) {\n"
								 "        gl_SampleMask[i] = gl_SampleMaskIn[i];\n"
								 "    }\n"
								 "    o_color = vec4(gl_SampleID, gl_SamplePosition.x, gl_SamplePosition.y, 1);\n"
								 "}\n";

		glu::ShaderProgram programRequire(m_context.getRenderContext(),
										  glu::makeVtxFragSources(specializeVersion(vss, m_glslVersion).c_str(),
																  specializeVersion(fss, m_glslVersion).c_str()));
		log << programRequire;
		if (!programRequire.isOk())
		{
			TCU_FAIL("Compile failed");
		}
	}

	{
		static char const* fss = "${VERSION_DECL}\n"
								 "${OES_SV_EN}"
								 "out highp vec4 o_color;\n"
								 "void main()\n"
								 "{\n"
								 "#if !GL_OES_sample_variables\n"
								 "    this is broken\n"
								 "#endif\n"
								 "    for (int i = 0; i < (gl_MaxSamples + 31) / 32; ++i) {\n"
								 "        gl_SampleMask[i] = gl_SampleMaskIn[i];\n"
								 "    }\n"
								 "    o_color = vec4(gl_SampleID, gl_SamplePosition.x, gl_SamplePosition.y, 1);\n"
								 "}\n";

		glu::ShaderProgram programEnable(m_context.getRenderContext(),
										 glu::makeVtxFragSources(specializeVersion(vss, m_glslVersion).c_str(),
																 specializeVersion(fss, m_glslVersion).c_str()));
		log << programEnable;
		if (!programEnable.isOk())
		{
			TCU_FAIL("Compile failed");
		}
	}

	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	return STOP;
}

class SampleShadingMaskCase : public deqp::TestCase
{
public:
	SampleShadingMaskCase(Context& context, const char* name, const char* description, glu::GLSLVersion glslVersion,
						  GLenum internalFormat, tcu::TextureFormat const& texFormat, const char* sampler,
						  const char* outType, GLint samples, GLint sampleMask);
	~SampleShadingMaskCase();

	IterateResult iterate();

protected:
	glu::GLSLVersion   m_glslVersion;
	GLenum			   m_internalFormat;
	tcu::TextureFormat m_texFormat;
	std::string		   m_sampler;
	std::string		   m_outType;
	GLint			   m_samples;
	GLint			   m_sampleMask;

	enum
	{
		WIDTH		= 16,
		HEIGHT		= 16,
		MAX_SAMPLES = 4,
	};
};

SampleShadingMaskCase::SampleShadingMaskCase(Context& context, const char* name, const char* description,
											 glu::GLSLVersion glslVersion, GLenum internalFormat,
											 tcu::TextureFormat const& texFormat, const char* sampler,
											 const char* outType, GLint samples, GLint sampleMask)
	: TestCase(context, name, description)
	, m_glslVersion(glslVersion)
	, m_internalFormat(internalFormat)
	, m_texFormat(texFormat)
	, m_sampler(sampler)
	, m_outType(outType)
	, m_samples(samples)
	, m_sampleMask(sampleMask)
{
	DE_ASSERT(glslVersion == glu::GLSL_VERSION_310_ES || glslVersion >= glu::GLSL_VERSION_400);
}

SampleShadingMaskCase::~SampleShadingMaskCase()
{
}

SampleShadingMaskCase::IterateResult SampleShadingMaskCase::iterate()
{
	TestLog&			  log  = m_testCtx.getLog();
	const glw::Functions& gl   = m_context.getRenderContext().getFunctions();
	bool				  isOk = true;

	/* OpenGL support query. */
	bool is_at_least_gl_45 = (glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::core(4, 5)));
	bool is_arb_es31_compatibility = m_context.getContextInfo().isExtensionSupported("GL_ARB_ES3_1_compatibility");

	if (!(is_at_least_gl_45 || is_arb_es31_compatibility))
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "GL_ARB_ES3_1_compatibility");
		return STOP;
	}

	GLint maxSamples;
	if (((m_texFormat.type == tcu::TextureFormat::FLOAT) && (m_texFormat.order == tcu::TextureFormat::RGBA)) ||
		((m_texFormat.type == tcu::TextureFormat::FLOAT) && (m_texFormat.order == tcu::TextureFormat::RG)) ||
		((m_texFormat.type == tcu::TextureFormat::FLOAT) && (m_texFormat.order == tcu::TextureFormat::R)) ||
		((m_texFormat.type == tcu::TextureFormat::HALF_FLOAT) && (m_texFormat.order == tcu::TextureFormat::RGBA)))
	{
		gl.getInternalformativ(GL_TEXTURE_2D_MULTISAMPLE, m_internalFormat, GL_SAMPLES, 1, &maxSamples);
		if (m_samples > maxSamples)
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED,
									"Test sample count greater than samples that the format supports");
			return STOP;
		}
	}
	else if (m_texFormat.type == tcu::TextureFormat::SIGNED_INT8 ||
			 m_texFormat.type == tcu::TextureFormat::UNSIGNED_INT8)
	{
		gl.getIntegerv(GL_MAX_INTEGER_SAMPLES, &maxSamples);
		if (m_samples > maxSamples)
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "Test sample count greater than MAX_INTEGER_SAMPLES");
			return STOP;
		}
	}
	else
	{
		gl.getIntegerv(GL_MAX_SAMPLES, &maxSamples);
		if (m_samples > maxSamples)
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "Test sample count greater than MAX_SAMPLES");
			return STOP;
		}
	}

	// Create a multisample texture, or a regular texture if samples is zero.
	GLuint tex;
	gl.genTextures(1, &tex);
	GLenum target;
	if (m_samples)
	{
		target = GL_TEXTURE_2D_MULTISAMPLE;
		gl.bindTexture(target, tex);
		gl.texStorage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, m_samples, m_internalFormat, WIDTH, HEIGHT, GL_FALSE);
	}
	else
	{
		target = GL_TEXTURE_2D;
		gl.bindTexture(target, tex);
		gl.texStorage2D(GL_TEXTURE_2D, 1, m_internalFormat, WIDTH, HEIGHT);
		if (m_texFormat.type == tcu::TextureFormat::SIGNED_INT8 ||
			m_texFormat.type == tcu::TextureFormat::UNSIGNED_INT8 || m_texFormat.type == tcu::TextureFormat::FLOAT)
		{
			gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			gl.texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		}
	}

	// Create a framebuffer with the texture attached and clear to "green".
	GLuint fboMs;
	gl.genFramebuffers(1, &fboMs);
	gl.bindFramebuffer(GL_FRAMEBUFFER, fboMs);
	gl.framebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, target, tex, 0);
	gl.viewport(0, 0, WIDTH, HEIGHT);
	if (m_texFormat.type == tcu::TextureFormat::SIGNED_INT8)
	{
		GLint color[4] = { 0, 1, 0, 1 };
		gl.clearBufferiv(GL_COLOR, 0, color);
	}
	else if (m_texFormat.type == tcu::TextureFormat::UNSIGNED_INT8)
	{
		GLuint color[4] = { 0, 1, 0, 1 };
		gl.clearBufferuiv(GL_COLOR, 0, color);
	}
	else
	{
		GLfloat color[4] = { 0.0f, 1.0f, 0.0f, 1.0f };
		gl.clearBufferfv(GL_COLOR, 0, color);
	}

	static deUint16 const quadIndices[] = { 0, 1, 2, 2, 1, 3 };

	{
		// Draw a quad setting all samples to "red". We only expect "red"
		// to be written if the sample mask bit for that sample is 1.

		static char const* vss = "${VERSION_DECL}\n"
								 "in highp vec2 a_position;\n"
								 "void main()\n"
								 "{\n"
								 "   gl_Position = vec4(a_position, 0.0, 1.0);\n"
								 "}\n";

		static char const* fss = "${VERSION_DECL}\n"
								 "${OES_SV_RQ}"
								 "layout(location = 0) out highp ${OUT_TYPE} o_color;\n"
								 "uniform int u_sampleMask;\n"
								 "void main()\n"
								 "{\n"
								 "    for (int i = 0; i < (gl_NumSamples + 31) / 32; ++i) {\n"
								 "        gl_SampleMask[i] = u_sampleMask & gl_SampleMaskIn[i];\n"
								 "    }\n"
								 "    o_color = ${OUT_TYPE}(1, 0, 0, 1);\n"
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

		static float const position[] = {
			-1.0f, -1.0f, -1.0f, +1.0f, +1.0f, -1.0f, +1.0f, +1.0f,
		};

		gl.useProgram(program.getProgram());
		gl.uniform1i(gl.getUniformLocation(program.getProgram(), "u_sampleMask"), m_sampleMask);

		glu::VertexArrayBinding vertexArrays[] = {
			glu::va::Float("a_position", 2, 4, 0, &position[0]),
		};
		glu::draw(m_context.getRenderContext(), program.getProgram(), DE_LENGTH_OF_ARRAY(vertexArrays),
				  &vertexArrays[0], glu::pr::TriangleStrip(DE_LENGTH_OF_ARRAY(quadIndices), &quadIndices[0]));

		GLU_EXPECT_NO_ERROR(gl.getError(), "Draw quad");
	}

	gl.bindFramebuffer(GL_FRAMEBUFFER, m_context.getRenderContext().getDefaultFramebuffer());
	gl.deleteFramebuffers(1, &fboMs);

	GLsizei width = WIDTH * ((m_samples) ? m_samples : 1);

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
		// Resolve the multi-sample texture into a render-buffer sized such that
		// the width can hold all samples of a pixel.
		static char const* vss = "${VERSION_DECL}\n"
								 "in highp vec2 a_position;\n"
								 "void main(void)\n"
								 "{\n"
								 "   gl_Position = vec4(a_position, 0.0, 1.0);\n"
								 "}\n";

		static char const* fss = "${VERSION_DECL}\n"
								 "uniform highp ${SAMPLER} u_tex;\n"
								 "uniform highp ${SAMPLER}MS u_texMS;\n"
								 "uniform int u_samples;\n"
								 "layout(location = 0) out highp ${OUT_TYPE} o_color;\n"
								 "void main(void)\n"
								 "{\n"
								 "    if (u_samples > 0) {\n"
								 "        ivec2 coord = ivec2(int(gl_FragCoord.x) / u_samples, gl_FragCoord.y);\n"
								 "        int sampleId = int(gl_FragCoord.x) % u_samples;\n"
								 "        o_color = texelFetch(u_texMS, coord, sampleId);\n"
								 "    } else {\n"
								 "        ivec2 coord = ivec2(gl_FragCoord.x, gl_FragCoord.y);\n"
								 "       o_color = texelFetch(u_tex, coord, 0);\n"
								 "    }\n"
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

		static float const position[] = {
			-1.0f, -1.0f, -1.0f, +1.0f, +1.0f, -1.0f, +1.0f, +1.0f,
		};

		gl.useProgram(program.getProgram());
		gl.uniform1i(gl.getUniformLocation(program.getProgram(), "u_samples"), m_samples);
		if (m_samples > 0)
		{
			// only MS sampler needed, TU 1 is not used
			gl.uniform1i(gl.getUniformLocation(program.getProgram(), "u_tex"), 1);
			gl.uniform1i(gl.getUniformLocation(program.getProgram(), "u_texMS"), 0);
		}
		else
		{
			// only non-MS sampler needed, TU 1 is not used
			gl.uniform1i(gl.getUniformLocation(program.getProgram(), "u_tex"), 0);
			gl.uniform1i(gl.getUniformLocation(program.getProgram(), "u_texMS"), 1);
		}

		glu::VertexArrayBinding vertexArrays[] = {
			glu::va::Float("a_position", 2, 4, 0, &position[0]),
		};
		glu::draw(m_context.getRenderContext(), program.getProgram(), DE_LENGTH_OF_ARRAY(vertexArrays),
				  &vertexArrays[0], glu::pr::TriangleStrip(DE_LENGTH_OF_ARRAY(quadIndices), &quadIndices[0]));

		GLU_EXPECT_NO_ERROR(gl.getError(), "Draw quad");
	}

	tcu::TextureLevel	  textureLevel(m_texFormat, width, HEIGHT);
	tcu::PixelBufferAccess pixels = textureLevel.getAccess();
	std::vector<tcu::Vec4> result(pixels.getHeight() * pixels.getWidth());

	if (pixels.getFormat().type == tcu::TextureFormat::SIGNED_INT8)
	{
		std::vector<GLint> data(pixels.getHeight() * pixels.getWidth() * 4);
		gl.readPixels(0, 0, pixels.getWidth(), pixels.getHeight(), GL_RGBA_INTEGER, GL_INT, &data[0]);
		for (unsigned int i = 0; i < data.size(); i += 4)
		{
			result[i / 4] =
				tcu::Vec4((GLfloat)data[i], (GLfloat)data[i + 1], (GLfloat)data[i + 2], (GLfloat)data[i + 3]);
		}
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
	}
	else
	{
		glu::readPixels(m_context.getRenderContext(), 0, 0, pixels);
	}

	for (int y = 0; y < HEIGHT; ++y)
	{
		for (int x = 0; x < WIDTH; ++x)
		{
			GLint samples = (m_samples) ? m_samples : 1;
			for (int sample = 0; sample < samples; ++sample)
			{
				tcu::Vec4 pixel;
				if (pixels.getFormat().type == tcu::TextureFormat::SIGNED_INT8 ||
					pixels.getFormat().type == tcu::TextureFormat::UNSIGNED_INT8)
				{
					pixel = result[y * WIDTH + x * samples + sample];
				}
				else
				{
					pixel = pixels.getPixel(x * samples + sample, y);
				}

				// Make sure only those samples where the sample mask bit is
				// non-zero have the "red" pixel values.
				if (!m_samples || (m_sampleMask & (1 << sample)))
				{
					if (pixel != tcu::Vec4(1.0f, 0.0f, 0.0f, 1.0f))
					{
						isOk = false;
					}
				}
				else
				{
					if (pixel != tcu::Vec4(0.0f, 1.0f, 0.0f, 1.0f))
					{
						isOk = false;
					}
				}
			}
		}
	}

	gl.bindFramebuffer(GL_FRAMEBUFFER, m_context.getRenderContext().getDefaultFramebuffer());
	gl.deleteFramebuffers(1, &fbo);

	gl.bindRenderbuffer(GL_RENDERBUFFER, 0);
	gl.deleteRenderbuffers(1, &rbo);

	gl.bindTexture(target, 0);
	gl.deleteTextures(1, &tex);

	m_testCtx.setTestResult(isOk ? QP_TEST_RESULT_PASS : QP_TEST_RESULT_FAIL, isOk ? "Pass" : "Fail");
	return STOP;
}

class SampleShadingPositionCase : public deqp::TestCase
{
public:
	SampleShadingPositionCase(Context& context, const char* name, const char* description, glu::GLSLVersion glslVersion,
							  GLint samples, GLboolean fixedSampleLocations);
	~SampleShadingPositionCase();

	IterateResult iterate();

protected:
	glu::GLSLVersion m_glslVersion;
	GLint			 m_samples;
	GLboolean		 m_fixedSampleLocations;

	enum
	{
		WIDTH		= 8,
		HEIGHT		= 8,
		MAX_SAMPLES = 8,
	};
};

SampleShadingPositionCase::SampleShadingPositionCase(Context& context, const char* name, const char* description,
													 glu::GLSLVersion glslVersion, GLint samples,
													 GLboolean fixedSampleLocations)
	: TestCase(context, name, description)
	, m_glslVersion(glslVersion)
	, m_samples(samples)
	, m_fixedSampleLocations(fixedSampleLocations)
{
	DE_ASSERT(glslVersion == glu::GLSL_VERSION_310_ES || glslVersion >= glu::GLSL_VERSION_400);
}

SampleShadingPositionCase::~SampleShadingPositionCase()
{
}

SampleShadingPositionCase::IterateResult SampleShadingPositionCase::iterate()
{
	TestLog&			  log  = m_testCtx.getLog();
	const glw::Functions& gl   = m_context.getRenderContext().getFunctions();
	bool				  isOk = true;

	/* OpenGL support query. */
	bool is_at_least_gl_45 = (glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::core(4, 5)));
	bool is_arb_es31_compatibility = m_context.getContextInfo().isExtensionSupported("GL_ARB_ES3_1_compatibility");

	if (!(is_at_least_gl_45 || is_arb_es31_compatibility))
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "GL_ARB_ES3_1_compatibility");
		return STOP;
	}

	GLint maxSamples;
	gl.getIntegerv(GL_MAX_SAMPLES, &maxSamples);
	if (m_samples > maxSamples)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "Test sample count great than MAX_SAMPLES");
		return STOP;
	}

	// Create a multisample texture, or a regular texture if samples is zero.
	GLuint tex;
	gl.genTextures(1, &tex);
	GLenum target;
	if (m_samples)
	{
		target = GL_TEXTURE_2D_MULTISAMPLE;
		gl.bindTexture(target, tex);
		gl.texStorage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, m_samples, GL_RGBA8, WIDTH, HEIGHT,
								   m_fixedSampleLocations);
	}
	else
	{
		target = GL_TEXTURE_2D;
		gl.bindTexture(target, tex);
		gl.texStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, WIDTH, HEIGHT);
	}

	// Attach the texture to the framebuffer to render to it.
	GLuint fboMs;
	gl.genFramebuffers(1, &fboMs);
	gl.bindFramebuffer(GL_FRAMEBUFFER, fboMs);
	gl.framebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, target, tex, 0);
	gl.viewport(0, 0, WIDTH, HEIGHT);

	// Save all the sample positions for this multisample framebuffer.
	std::vector<tcu::Vec4> samplePositions;
	if (m_samples)
	{
		samplePositions.resize(m_samples);
		for (int sample = 0; sample < m_samples; ++sample)
		{
			GLfloat position[2];
			gl.getMultisamplefv(GL_SAMPLE_POSITION, sample, position);
			samplePositions[sample] = tcu::Vec4(position[0], position[1], 0.0f, 1.0f);
		}
	}

	static deUint16 const quadIndices[] = { 0, 1, 2, 2, 1, 3 };

	{
		// Render all the sample positions to each pixel sample.

		static char const* vss = "${VERSION_DECL}\n"
								 "in highp vec2 a_position;\n"
								 "void main()\n"
								 "{\n"
								 "   gl_Position = vec4(a_position, 0.0, 1.0);\n"
								 "}\n";

		static char const* fss = "${VERSION_DECL}\n"
								 "${OES_SV_RQ}"
								 "layout(location = 0) out highp vec4 o_color;\n"
								 "void main()\n"
								 "{\n"
								 "    o_color = vec4(gl_SamplePosition, 0, 1);\n"
								 "}\n";

		glu::ShaderProgram program(m_context.getRenderContext(),
								   glu::makeVtxFragSources(specializeVersion(vss, m_glslVersion).c_str(),
														   specializeVersion(fss, m_glslVersion).c_str()));
		log << program;

		if (!program.isOk())
		{
			TCU_FAIL("Compile failed");
		}

		const float position[] = {
			-1.0f, -1.0f, -1.0f, +1.0f, +1.0f, -1.0f, +1.0f, +1.0f,
		};

		gl.useProgram(program.getProgram());

		glu::VertexArrayBinding vertexArrays[] = {
			glu::va::Float("a_position", 2, 4, 0, &position[0]),
		};
		glu::draw(m_context.getRenderContext(), program.getProgram(), DE_LENGTH_OF_ARRAY(vertexArrays),
				  &vertexArrays[0], glu::pr::TriangleStrip(DE_LENGTH_OF_ARRAY(quadIndices), &quadIndices[0]));

		GLU_EXPECT_NO_ERROR(gl.getError(), "Draw quad");
	}

	gl.bindFramebuffer(GL_FRAMEBUFFER, m_context.getRenderContext().getDefaultFramebuffer());
	gl.deleteFramebuffers(1, &fboMs);

	// Create a regular non-multisample render buffer to resolve to multisample texture into.
	// The width is increased to save all samples of the pixel.

	GLsizei width = WIDTH * ((m_samples) ? m_samples : 1);

	GLuint rbo;
	gl.genRenderbuffers(1, &rbo);
	gl.bindRenderbuffer(GL_RENDERBUFFER, rbo);
	gl.renderbufferStorage(GL_RENDERBUFFER, GL_RGBA8, width, HEIGHT);

	GLuint fbo;
	gl.genFramebuffers(1, &fbo);
	gl.bindFramebuffer(GL_FRAMEBUFFER, fbo);
	gl.framebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, rbo);
	gl.viewport(0, 0, width, HEIGHT);

	{
		// Resolve the multisample texture to the renderbuffer.

		static char const* vss = "${VERSION_DECL}\n"
								 "in highp vec2 a_position;\n"
								 "void main(void)\n"
								 "{\n"
								 "   gl_Position = vec4(a_position, 0.0, 1.0);\n"
								 "}\n";

		static char const* fss = "${VERSION_DECL}\n"
								 "uniform highp sampler2D u_tex;\n"
								 "uniform highp sampler2DMS u_texMS;\n"
								 "uniform int u_samples;\n"
								 "layout(location = 0) out highp vec4 o_color;\n"
								 "void main(void)\n"
								 "{\n"
								 "    if (u_samples > 0) {\n"
								 "        ivec2 coord = ivec2(int(gl_FragCoord.x) / u_samples, gl_FragCoord.y);\n"
								 "        int sampleId = int(gl_FragCoord.x) % u_samples;\n"
								 "        o_color = texelFetch(u_texMS, coord, sampleId);\n"
								 "    } else {\n"
								 "        ivec2 coord = ivec2(gl_FragCoord.x, gl_FragCoord.y);\n"
								 "        o_color = texelFetch(u_tex, coord, 0);\n"
								 "    }\n"
								 "}\n";

		glu::ShaderProgram program(m_context.getRenderContext(),
								   glu::makeVtxFragSources(specializeVersion(vss, m_glslVersion).c_str(),
														   specializeVersion(fss, m_glslVersion).c_str()));
		log << program;
		if (!program.isOk())
		{
			TCU_FAIL("Compile failed");
		}

		static float const position[] = {
			-1.0f, -1.0f, -1.0f, +1.0f, +1.0f, -1.0f, +1.0f, +1.0f,
		};

		gl.useProgram(program.getProgram());
		gl.uniform1i(gl.getUniformLocation(program.getProgram(), "u_samples"), m_samples);
		if (m_samples > 0)
		{
			// only MS sampler needed, TU 1 is not used
			gl.uniform1i(gl.getUniformLocation(program.getProgram(), "u_tex"), 1);
			gl.uniform1i(gl.getUniformLocation(program.getProgram(), "u_texMS"), 0);
		}
		else
		{
			// only non-MS sampler needed, TU 1 is not used
			gl.uniform1i(gl.getUniformLocation(program.getProgram(), "u_tex"), 0);
			gl.uniform1i(gl.getUniformLocation(program.getProgram(), "u_texMS"), 1);
		}

		glu::VertexArrayBinding vertexArrays[] = {
			glu::va::Float("a_position", 2, 4, 0, &position[0]),
		};
		glu::draw(m_context.getRenderContext(), program.getProgram(), DE_LENGTH_OF_ARRAY(vertexArrays),
				  &vertexArrays[0], glu::pr::TriangleStrip(DE_LENGTH_OF_ARRAY(quadIndices), &quadIndices[0]));

		GLU_EXPECT_NO_ERROR(gl.getError(), "Draw quad");
	}

	// Read the renderbuffer pixels and verify we get back what we're expecting.
	tcu::TextureLevel results(tcu::TextureFormat(tcu::TextureFormat::RGBA, tcu::TextureFormat::UNORM_INT8), width,
							  HEIGHT);
	tcu::PixelBufferAccess pixels = results.getAccess();
	glu::readPixels(m_context.getRenderContext(), 0, 0, pixels);
	if (m_samples)
	{
		// If m_fixedSampleLocations are used make sure the first pixel's samples
		// all match the SAMPLE_POSITION state saved earlier.
		std::set<tcu::Vec4> fixedSampleLocations;
		if (m_fixedSampleLocations)
		{
			for (int sample = 0; sample < m_samples; ++sample)
			{
				tcu::Vec4 pixel = pixels.getPixel(sample, 0);
				fixedSampleLocations.insert(pixel);
				if (deFloatAbs(pixel.x() - samplePositions[sample].x()) > 0.01 ||
					deFloatAbs(pixel.y() - samplePositions[sample].y()) > 0.01)
				{

					isOk = false;
				}
			}
		}

		// Verify all samples of every pixel to make sure each position is unique.
		for (int y = 0; y < HEIGHT; ++y)
		{
			for (int x = 0; x < WIDTH; ++x)
			{
				std::set<tcu::Vec4> uniquePixels;
				for (int sample = 0; sample < m_samples; ++sample)
				{
					uniquePixels.insert(pixels.getPixel(x * m_samples + sample, y));
				}
				if ((GLint)uniquePixels.size() != m_samples)
				{
					isOk = false;
				}
				// For the m_fixedSampleLocations case make sure each position
				// matches the sample positions of pixel(0, 0) saved earlier.
				if (m_fixedSampleLocations)
				{
					if (fixedSampleLocations != uniquePixels)
					{
						isOk = false;
					}
				}
			}
		}
	}
	else
	{
		// For the non-multisample case make sure all the positions are (0.5,0.5).
		for (int y = 0; y < pixels.getHeight(); ++y)
		{
			for (int x = 0; x < pixels.getWidth(); ++x)
			{
				tcu::Vec4 pixel = pixels.getPixel(x, y);
				if (deFloatAbs(pixel.x() - 0.5f) > 0.01 || deFloatAbs(pixel.y() - 0.5f) > 0.01 || pixel.z() != 0.0f ||
					pixel.w() != 1.0f)
				{
					isOk = false;
				}
			}
		}
	}

	gl.bindFramebuffer(GL_FRAMEBUFFER, m_context.getRenderContext().getDefaultFramebuffer());
	gl.deleteFramebuffers(1, &fbo);

	gl.bindRenderbuffer(GL_RENDERBUFFER, 0);
	gl.deleteRenderbuffers(1, &rbo);

	gl.bindTexture(target, 0);
	gl.deleteTextures(1, &tex);

	m_testCtx.setTestResult(isOk ? QP_TEST_RESULT_PASS : QP_TEST_RESULT_FAIL, isOk ? "Pass" : "Fail");
	return STOP;
}

SampleVariablesTests::SampleVariablesTests(Context& context, glu::GLSLVersion glslVersion)
	: TestCaseGroup(context, "sample_variables", "Sample Variables tests"), m_glslVersion(glslVersion)
{
}

SampleVariablesTests::~SampleVariablesTests()
{
}

void SampleVariablesTests::init()
{
	de::Random rnd(m_context.getTestContext().getCommandLine().getBaseSeed());

	struct Sample
	{
		char const* name;
		GLint		samples;
	} samples[] = {
		{ "samples_0", 0 }, { "samples_1", 1 }, { "samples_2", 2 }, { "samples_4", 4 }, { "samples_8", 8 },
	};

	// sample_variables.extension
	if (m_glslVersion == glu::GLSL_VERSION_310_ES)
	{
		addChild(new SampleShadingExtensionCase(m_context, "extension", "#extension verification", m_glslVersion));
	}

	// sample_variables.mask
	tcu::TestCaseGroup* maskGroup = new tcu::TestCaseGroup(m_testCtx, "mask", "gl_SampleMask tests");
	addChild(maskGroup);
	struct Format
	{
		char const*		   name;
		GLenum			   internalFormat;
		tcu::TextureFormat textureFormat;
		char const*		   sampler;
		char const*		   outType;
	} formats[] = {
		{ "rgba8", GL_RGBA8, tcu::TextureFormat(tcu::TextureFormat::RGBA, tcu::TextureFormat::UNORM_INT8), "sampler2D",
		  "vec4" },
		{ "rgba8i", GL_RGBA8I, tcu::TextureFormat(tcu::TextureFormat::RGBA, tcu::TextureFormat::SIGNED_INT8),
		  "isampler2D", "ivec4" },
		{ "rgba8ui", GL_RGBA8UI, tcu::TextureFormat(tcu::TextureFormat::RGBA, tcu::TextureFormat::UNSIGNED_INT8),
		  "usampler2D", "uvec4" },
		{ "rgba32f", GL_RGBA32F, tcu::TextureFormat(tcu::TextureFormat::RGBA, tcu::TextureFormat::FLOAT), "sampler2D",
		  "vec4" },
	};
	for (int format = 0; format < DE_LENGTH_OF_ARRAY(formats); ++format)
	{
		tcu::TestCaseGroup* maskFormatGroup = new tcu::TestCaseGroup(m_testCtx, formats[format].name, "");
		maskGroup->addChild(maskFormatGroup);

		for (int sample = 0; sample < DE_LENGTH_OF_ARRAY(samples); ++sample)
		{
			tcu::TestCaseGroup* maskFormatSampleGroup = new tcu::TestCaseGroup(m_testCtx, samples[sample].name, "");
			maskFormatGroup->addChild(maskFormatSampleGroup);

			maskFormatSampleGroup->addChild(
				new SampleShadingMaskCase(m_context, "mask_zero", "", m_glslVersion, formats[format].internalFormat,
										  formats[format].textureFormat, formats[format].sampler,
										  formats[format].outType, samples[sample].samples, 0));

			for (int mask = 0; mask < SAMPLE_MASKS; ++mask)
			{
				std::stringstream ss;
				ss << "mask_" << mask;
				maskFormatSampleGroup->addChild(new SampleShadingMaskCase(
					m_context, ss.str().c_str(), "", m_glslVersion, formats[format].internalFormat,
					formats[format].textureFormat, formats[format].sampler, formats[format].outType,
					samples[sample].samples, rnd.getUint32()));
			}
		}
	}

	// sample_variables.position
	tcu::TestCaseGroup* positionGroup = new tcu::TestCaseGroup(m_testCtx, "position", "gl_SamplePosition tests");
	addChild(positionGroup);
	struct Fixed
	{
		char const* name;
		GLboolean   fixedSampleLocations;
	} fixed[] = {
		{ "non-fixed", GL_FALSE }, { "fixed", GL_TRUE },
	};
	for (int j = 0; j < DE_LENGTH_OF_ARRAY(fixed); ++j)
	{
		tcu::TestCaseGroup* positionFixedGroup = new tcu::TestCaseGroup(m_testCtx, fixed[j].name, "");
		positionGroup->addChild(positionFixedGroup);
		for (int sample = 0; sample < DE_LENGTH_OF_ARRAY(samples); ++sample)
		{
			positionFixedGroup->addChild(new SampleShadingPositionCase(m_context, samples[sample].name, "",
																	   m_glslVersion, samples[sample].samples,
																	   fixed[j].fixedSampleLocations));
		}
	}
}

} // es31compatibility
} // gl4cts
