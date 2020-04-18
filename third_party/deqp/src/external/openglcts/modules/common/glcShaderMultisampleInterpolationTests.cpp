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

#include "glcShaderMultisampleInterpolationTests.hpp"
#include "deMath.h"
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

namespace deqp
{

using tcu::TestLog;
using std::string;
using std::vector;

static std::string specializeVersion(std::string const& source, glu::GLSLVersion version,
									 std::string const& sampler = "", std::string const& outType = "",
									 std::string const& qualifier = "", std::string const& assignment = "",
									 std::string const& condition = "")
{
	DE_ASSERT(version == glu::GLSL_VERSION_310_ES || version >= glu::GLSL_VERSION_440);
	std::map<std::string, std::string> args;
	args["VERSION_DECL"] = glu::getGLSLVersionDeclaration(version);
	args["SAMPLER"]		 = sampler;
	args["OUT_TYPE"]	 = outType;
	args["QUALIFIER"]	= qualifier;
	args["ASSIGNMENT"]   = assignment;
	args["CONDITION"]	= condition;
	if (version == glu::GLSL_VERSION_310_ES)
	{
		args["OES_SMI_EN"] = "#extension GL_OES_shader_multisample_interpolation : enable\n";
		args["OES_SMI_RQ"] = "#extension GL_OES_shader_multisample_interpolation : require\n";
		args["OES_SMI_CH"] = "#if !GL_OES_shader_multisample_interpolation\n"
							 "    this is broken\n"
							 "#endif\n";
		args["OES_SV_EN"] = "#extension GL_OES_sample_variables : enable\n";
	}
	else
	{
		args["OES_SMI_EN"] = "";
		args["OES_SMI_RQ"] = "";
		args["OES_SMI_CH"] = "";
		args["OES_SV_EN"]  = "";
	}
	return tcu::StringTemplate(source.c_str()).specialize(args);
}

class ShaderMultisampleInterpolationApiCase : public TestCase
{
public:
	ShaderMultisampleInterpolationApiCase(Context& context, const char* name, const char* description,
										  glu::GLSLVersion glslVersion);
	~ShaderMultisampleInterpolationApiCase();

	IterateResult iterate();

protected:
	glu::GLSLVersion m_glslVersion;
};

ShaderMultisampleInterpolationApiCase::ShaderMultisampleInterpolationApiCase(Context& context, const char* name,
																			 const char*	  description,
																			 glu::GLSLVersion glslVersion)
	: TestCase(context, name, description), m_glslVersion(glslVersion)
{
	DE_ASSERT(glslVersion == glu::GLSL_VERSION_310_ES || glslVersion >= glu::GLSL_VERSION_440);
}

ShaderMultisampleInterpolationApiCase::~ShaderMultisampleInterpolationApiCase()
{
}

ShaderMultisampleInterpolationApiCase::IterateResult ShaderMultisampleInterpolationApiCase::iterate()
{
	TestLog&			  log  = m_testCtx.getLog();
	const glw::Functions& gl   = m_context.getRenderContext().getFunctions();
	bool				  isOk = true;

	if (m_glslVersion == glu::GLSL_VERSION_310_ES &&
		!m_context.getContextInfo().isExtensionSupported("GL_OES_shader_multisample_interpolation"))
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "GL_OES_shader_multisample_interpolation");
		return STOP;
	}

	static char const* vss = "${VERSION_DECL}\n"
							 "${OES_SMI_RQ}"
							 "in highp vec4 a_position;\n"
							 "in highp vec4 a_color;\n"
							 "sample out highp vec4 v_color;\n"
							 "void main()\n"
							 "{\n"
							 "    gl_Position = a_position;\n"
							 "}\n";

	{
		static char const* fss = "${VERSION_DECL}\n"
								 "${OES_SMI_RQ}"
								 "sample in highp vec4 v_color;\n"
								 "out highp vec4 o_color;\n"
								 "void main()\n"
								 "{\n"
								 "    o_color = v_color;\n"
								 "}\n";

		glu::ShaderProgram program(m_context.getRenderContext(),
								   glu::makeVtxFragSources(specializeVersion(vss, m_glslVersion).c_str(),
														   specializeVersion(fss, m_glslVersion).c_str()));
		log << program;
		if (!program.isOk())
		{
			TCU_FAIL("Compile failed");
		}
	}

	{
		static char const* fss = "${VERSION_DECL}\n"
								 "${OES_SMI_EN}"
								 "sample in highp vec4 v_color;\n"
								 "out highp vec4 o_color;\n"
								 "void main()\n"
								 "{\n"
								 "${OES_SMI_CH}"
								 "    o_color = v_color;\n"
								 "}\n";

		glu::ShaderProgram program(m_context.getRenderContext(),
								   glu::makeVtxFragSources(specializeVersion(vss, m_glslVersion).c_str(),
														   specializeVersion(fss, m_glslVersion).c_str()));
		log << program;
		if (!program.isOk())
		{
			TCU_FAIL("Compile failed");
		}
	}

	GLfloat minFragmentInterpolationOffset = 0.0f;
	gl.getFloatv(GL_MIN_FRAGMENT_INTERPOLATION_OFFSET, &minFragmentInterpolationOffset);
	if (minFragmentInterpolationOffset > -0.5f)
	{
		isOk = false;
	}

	GLfloat maxFragmentInterpolationOffset = 0.0f;
	gl.getFloatv(GL_MAX_FRAGMENT_INTERPOLATION_OFFSET, &maxFragmentInterpolationOffset);
	if (maxFragmentInterpolationOffset < 0.5f)
	{
		isOk = false;
	}

	GLint fragmentInterpolationOffsetBits = 0;
	gl.getIntegerv(GL_FRAGMENT_INTERPOLATION_OFFSET_BITS, &fragmentInterpolationOffsetBits);
	if (fragmentInterpolationOffsetBits < 4)
	{
		isOk = false;
	}

	m_testCtx.setTestResult(isOk ? QP_TEST_RESULT_PASS : QP_TEST_RESULT_FAIL, isOk ? "Pass" : "Fail");
	return STOP;
}

class ShaderMultisampleInterpolationBaseCase : public TestCase
{
public:
	ShaderMultisampleInterpolationBaseCase(Context& context, const char* name, const char* description,
										   glu::GLSLVersion glslVersion, char const* qualifier, char const* assignment,
										   char const* condition, bool unique, GLenum internalFormat,
										   tcu::TextureFormat const& texFormat, const char* m_sampler,
										   const char* m_outType, GLfloat min, GLfloat max, GLint samples);
	~ShaderMultisampleInterpolationBaseCase();

	IterateResult iterate();

protected:
	glu::GLSLVersion   m_glslVersion;
	std::string		   m_qualifier;
	std::string		   m_assignment;
	std::string		   m_condition;
	bool			   m_unique;
	GLenum			   m_internalFormat;
	tcu::TextureFormat m_texFormat;
	std::string		   m_sampler;
	std::string		   m_outType;
	GLfloat			   m_min;
	GLfloat			   m_max;
	GLint			   m_samples;

	enum
	{
		WIDTH  = 8,
		HEIGHT = 8,
	};

	int countUniquePixels(tcu::ConstPixelBufferAccess const& pixels);
	int countUniquePixels(const std::vector<tcu::Vec4>& pixels);
};

ShaderMultisampleInterpolationBaseCase::ShaderMultisampleInterpolationBaseCase(
	Context& context, const char* name, const char* description, glu::GLSLVersion glslVersion, char const* qualifier,
	char const* assignment, char const* condition, bool unique, GLenum internalFormat,
	tcu::TextureFormat const& texFormat, const char* sampler, const char* outType, GLfloat min, GLfloat max,
	GLint samples)
	: TestCase(context, name, description)
	, m_glslVersion(glslVersion)
	, m_qualifier(qualifier)
	, m_assignment(assignment)
	, m_condition(condition)
	, m_unique(unique)
	, m_internalFormat(internalFormat)
	, m_texFormat(texFormat)
	, m_sampler(sampler)
	, m_outType(outType)
	, m_min(min)
	, m_max(max)
	, m_samples(samples)
{
	DE_ASSERT(glslVersion == glu::GLSL_VERSION_310_ES || glslVersion >= glu::GLSL_VERSION_440);
}

ShaderMultisampleInterpolationBaseCase::~ShaderMultisampleInterpolationBaseCase()
{
}

ShaderMultisampleInterpolationBaseCase::IterateResult ShaderMultisampleInterpolationBaseCase::iterate()
{
	TestLog&			  log			  = m_testCtx.getLog();
	const glw::Functions& gl			  = m_context.getRenderContext().getFunctions();
	bool				  isOk			  = true;
	bool				  supportsRgba32f = false;

	if (m_glslVersion == glu::GLSL_VERSION_310_ES &&
		!m_context.getContextInfo().isExtensionSupported("GL_OES_shader_multisample_interpolation"))
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "GL_OES_shader_multisample_interpolation");
		return STOP;
	}

	supportsRgba32f = isContextTypeGLCore(m_context.getRenderContext().getType()) ?
						  true :
						  (m_context.getContextInfo().isExtensionSupported("GL_EXT_color_buffer_float") ||
						   m_context.getContextInfo().isExtensionSupported("GL_ARB_color_buffer_float"));

	if (m_internalFormat == GL_RGBA32F && !supportsRgba32f)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_NOT_SUPPORTED, "Internalformat rgba32f not supported");
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
	gl.bindTexture(GL_TEXTURE_2D_MULTISAMPLE, tex);
	gl.texStorage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, m_samples, m_internalFormat, WIDTH, HEIGHT, GL_FALSE);

	// Create a framebuffer with the texture attached.
	GLuint fboMs;
	gl.genFramebuffers(1, &fboMs);
	gl.bindFramebuffer(GL_FRAMEBUFFER, fboMs);
	gl.framebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, tex, 0);
	gl.viewport(0, 0, WIDTH, HEIGHT);

	static deUint16 const quadIndices[] = { 0, 1, 2, 2, 1, 3 };

	{
		// Draw with one of the fragment input qualifiers and with one of the
		// interpolate functions. Cross-check the result in the shader and
		// output the final interpolated value.

		static char const* vss = "${VERSION_DECL}\n"
								 "${OES_SMI_RQ}"
								 "layout(location = 0) in highp vec2 a_position;\n"
								 "layout(location = 1) in highp vec4 a_color;\n"
								 "out highp vec4 v_colorBase;\n"
								 "${QUALIFIER} out highp vec4 v_color;\n"
								 "void main()\n"
								 "{\n"
								 "    v_colorBase = a_color;\n"
								 "    v_color = a_color;\n"
								 "    gl_Position = vec4(a_position, 0.0, 1.0);\n"
								 "}\n";

		static char const* fss = "${VERSION_DECL}\n"
								 "${OES_SMI_RQ}"
								 "${OES_SV_EN}"
								 "in highp vec4 v_colorBase;\n"
								 "${QUALIFIER} in highp vec4 v_color;\n"
								 "layout(location = 0) out highp ${OUT_TYPE} o_color;\n"
								 "void main()\n"
								 "{\n"
								 "    highp vec4 temp = ${ASSIGNMENT};\n"
								 "    bool condition = ${CONDITION};\n"
								 "    o_color = ${OUT_TYPE}(temp.x, temp.y, condition, 1);\n"
								 "}\n";

		glu::ShaderProgram program(
			m_context.getRenderContext(),
			glu::makeVtxFragSources(
				specializeVersion(vss, m_glslVersion, m_sampler, m_outType, m_qualifier, m_assignment, m_condition)
					.c_str(),
				specializeVersion(fss, m_glslVersion, m_sampler, m_outType, m_qualifier, m_assignment, m_condition)
					.c_str()));
		log << program;
		if (!program.isOk())
		{
			TCU_FAIL("Compile failed");
		}

		static float const position[] = {
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

	gl.bindFramebuffer(GL_FRAMEBUFFER, m_context.getRenderContext().getDefaultFramebuffer());
	gl.deleteFramebuffers(1, &fboMs);

	GLsizei width = WIDTH * m_samples;

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
		// Resolve the mutli-sample texture into a render-buffer sized such that
		// the width can hold all samples of a pixel.

		static char const* vss = "${VERSION_DECL}\n"
								 "in highp vec2 a_position;\n"
								 "void main(void)\n"
								 "{\n"
								 "   gl_Position = vec4(a_position, 0.0, 1.0);\n"
								 "}\n";

		static char const* fss = "${VERSION_DECL}\n"
								 "uniform highp ${SAMPLER}MS u_texMS;\n"
								 "uniform int u_samples;\n"
								 "layout(location = 0) out highp ${OUT_TYPE} o_color;\n"
								 "void main(void)\n"
								 "{\n"
								 "    ivec2 coord = ivec2(int(gl_FragCoord.x) / u_samples, gl_FragCoord.y);\n"
								 "    int sampleId = int(gl_FragCoord.x) % u_samples;\n"
								 "    o_color = texelFetch(u_texMS, coord, sampleId);\n"
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
		gl.uniform1i(gl.getUniformLocation(program.getProgram(), "u_texMS"), 0);

		glu::VertexArrayBinding vertexArrays[] = {
			glu::va::Float("a_position", 2, 4, 0, &position[0]),
		};
		glu::draw(m_context.getRenderContext(), program.getProgram(), DE_LENGTH_OF_ARRAY(vertexArrays),
				  &vertexArrays[0], glu::pr::TriangleStrip(DE_LENGTH_OF_ARRAY(quadIndices), &quadIndices[0]));

		GLU_EXPECT_NO_ERROR(gl.getError(), "Draw quad");
	}

	// Verify the results.
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

	int expectedUnique = WIDTH * HEIGHT * ((m_unique) ? m_samples : 1);
	if (uniquePixels < expectedUnique)
	{
		// There are duplicate pixel values meaning interpolation didn't work as expected.
		isOk = false;
	}
	for (int y = 0; y < pixels.getHeight(); ++y)
	{
		for (int x = 0; x < pixels.getWidth(); ++x)
		{
			tcu::Vec4 pixel;
			if (pixels.getFormat().type == tcu::TextureFormat::SIGNED_INT8 ||
				pixels.getFormat().type == tcu::TextureFormat::UNSIGNED_INT8)
			{
				pixel = result[y * WIDTH + x];
			}
			else
			{
				pixel = pixels.getPixel(x, y);
			}
			if (pixel.z() != 1)
			{
				// The ${CONDITION} check in the shader failed.
				isOk = false;
			}
		}
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

int ShaderMultisampleInterpolationBaseCase::countUniquePixels(tcu::ConstPixelBufferAccess const& pixels)
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

int ShaderMultisampleInterpolationBaseCase::countUniquePixels(const std::vector<tcu::Vec4>& pixels)
{
	std::set<tcu::Vec4> uniquePixels;

	for (unsigned int i = 0; i < pixels.size(); ++i)
	{
		uniquePixels.insert(pixels[i]);
	}

	return (int)uniquePixels.size();
}

ShaderMultisampleInterpolationTests::ShaderMultisampleInterpolationTests(Context& context, glu::GLSLVersion glslVersion)
	: TestCaseGroup(context, "shader_multisample_interpolation", "Shader Multisample Interpolation tests")
	, m_glslVersion(glslVersion)
{
}

ShaderMultisampleInterpolationTests::~ShaderMultisampleInterpolationTests()
{
}

void ShaderMultisampleInterpolationTests::init()
{
	struct Sample
	{
		char const* name;
		GLint		samples;
	} samples[] = {
		{ "samples_1", 1 }, { "samples_2", 2 }, { "samples_4", 4 },
	};

	// shader_multisample_interpolation.api
	tcu::TestCaseGroup* apiGroup = new tcu::TestCaseGroup(m_testCtx, "api", "API verification");
	apiGroup->addChild(new ShaderMultisampleInterpolationApiCase(m_context, "api", "API verification", m_glslVersion));
	addChild(apiGroup);

	struct Case
	{
		char const* name;
		char const* qualifier;
		char const* assignment;
		char const* condition;
		bool		unique;
	} cases[] = {
		{ "base", "", "v_color", "true", false },
		{ "sample", "sample", "v_color", "true", true },
		{ "centroid", "centroid", "v_color", "true", false },
		{ "interpolate_at_sample", "", "interpolateAtSample(v_colorBase, gl_SampleID)", "true", true },
		{ "interpolate_at_sample_check", "sample", "interpolateAtSample(v_colorBase, gl_SampleID)", "temp == v_color",
		  true },
		{ "interpolate_at_centroid", "", "interpolateAtCentroid(v_colorBase)", "true", false },
		{ "interpolate_at_centroid_check", "centroid", "interpolateAtCentroid(v_colorBase)", "temp == v_color", false },
		{ "interpolate_at_offset", "", "interpolateAtOffset(v_colorBase, gl_SamplePosition - 0.5)", "true", true },
		{ "interpolate_at_offset_check", "sample", "interpolateAtOffset(v_colorBase, gl_SamplePosition - 0.5)",
		  "temp == v_color", true },
	};

	// shader_multisample_interpolation.render
	tcu::TestCaseGroup* renderGroup = new tcu::TestCaseGroup(m_testCtx, "render", "Rendering tests");
	addChild(renderGroup);
	for (int caseId = 0; caseId < DE_LENGTH_OF_ARRAY(cases); ++caseId)
	{
		tcu::TestCaseGroup* group = new tcu::TestCaseGroup(m_testCtx, cases[caseId].name, "");
		renderGroup->addChild(group);
		struct Format
		{
			char const*		   name;
			GLenum			   internalFormat;
			tcu::TextureFormat textureFormat;
			char const*		   sampler;
			char const*		   outType;
			GLfloat			   min;
			GLfloat			   max;
		} formats[] = {
			{ "rgba8", GL_RGBA8, tcu::TextureFormat(tcu::TextureFormat::RGBA, tcu::TextureFormat::UNORM_INT8),
			  "sampler2D", "vec4", 0.0f, 1.0f },
			{ "rgba8i", GL_RGBA8I, tcu::TextureFormat(tcu::TextureFormat::RGBA, tcu::TextureFormat::SIGNED_INT8),
			  "isampler2D", "ivec4", -128.0f, 127.0f },
			{ "rgba8ui", GL_RGBA8UI, tcu::TextureFormat(tcu::TextureFormat::RGBA, tcu::TextureFormat::UNSIGNED_INT8),
			  "usampler2D", "uvec4", 0.0f, 255.0f },
			{ "rgba32f", GL_RGBA32F, tcu::TextureFormat(tcu::TextureFormat::RGBA, tcu::TextureFormat::FLOAT),
			  "sampler2D", "vec4", 0.0f, 1.0f },
		};
		for (int format = 0; format < DE_LENGTH_OF_ARRAY(formats); ++format)
		{
			tcu::TestCaseGroup* formatGroup = new tcu::TestCaseGroup(m_testCtx, formats[format].name, "");
			group->addChild(formatGroup);

			for (int sample = 0; sample < DE_LENGTH_OF_ARRAY(samples); ++sample)
			{
				formatGroup->addChild(new ShaderMultisampleInterpolationBaseCase(
					m_context, samples[sample].name, "", m_glslVersion, cases[caseId].qualifier,
					cases[caseId].assignment, cases[caseId].condition, cases[caseId].unique,
					formats[format].internalFormat, formats[format].textureFormat, formats[format].sampler,
					formats[format].outType, formats[format].min, formats[format].max, samples[sample].samples));
			}
		}
	}
}

} // glcts
