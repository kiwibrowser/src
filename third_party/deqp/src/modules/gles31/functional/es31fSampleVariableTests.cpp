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
 * \brief Sample variable tests
 *//*--------------------------------------------------------------------*/

#include "es31fSampleVariableTests.hpp"
#include "es31fMultisampleShaderRenderCase.hpp"
#include "tcuSurface.hpp"
#include "tcuTestLog.hpp"
#include "tcuRenderTarget.hpp"
#include "tcuTextureUtil.hpp"
#include "tcuVectorUtil.hpp"
#include "tcuFormatUtil.hpp"
#include "tcuStringTemplate.hpp"
#include "gluContextInfo.hpp"
#include "gluShaderProgram.hpp"
#include "gluRenderContext.hpp"
#include "glwFunctions.hpp"
#include "glwEnums.hpp"
#include "deStringUtil.hpp"

namespace deqp
{

using std::map;
using std::string;

namespace gles31
{
namespace Functional
{
namespace
{

class Verifier
{
public:
	virtual bool	verify	(const tcu::RGBA& testColor, const tcu::IVec2& position) const = 0;
	virtual void	logInfo	(tcu::TestLog& log) const = 0;
};

class ColorVerifier : public Verifier
{
public:
	ColorVerifier (const tcu::Vec3& _color, int _threshold = 8)
		: m_color		(tcu::Vec4(_color.x(), _color.y(), _color.z(), 1.0f))
		, m_threshold	(tcu::IVec3(_threshold))
	{
	}

	ColorVerifier (const tcu::Vec3& _color, tcu::IVec3 _threshold)
		: m_color		(tcu::Vec4(_color.x(), _color.y(), _color.z(), 1.0f))
		, m_threshold	(_threshold)
	{
	}

	bool verify (const tcu::RGBA& testColor, const tcu::IVec2& position) const
	{
		DE_UNREF(position);
		return !tcu::boolAny(tcu::greaterThan(tcu::abs(m_color.toIVec().swizzle(0, 1, 2) - testColor.toIVec().swizzle(0, 1, 2)), tcu::IVec3(m_threshold)));
	}

	void logInfo (tcu::TestLog& log) const
	{
		// full threshold? print * for clarity
		log	<< tcu::TestLog::Message
			<< "Expecting unicolored image, color = RGB("
			<< ((m_threshold[0] >= 255) ? ("*") : (de::toString(m_color.getRed()))) << ", "
			<< ((m_threshold[1] >= 255) ? ("*") : (de::toString(m_color.getGreen()))) << ", "
			<< ((m_threshold[2] >= 255) ? ("*") : (de::toString(m_color.getBlue()))) << ")"
			<< tcu::TestLog::EndMessage;
	}

	const tcu::RGBA		m_color;
	const tcu::IVec3	m_threshold;
};

class FullBlueSomeGreenVerifier : public Verifier
{
public:
	FullBlueSomeGreenVerifier (void)
	{
	}

	bool verify (const tcu::RGBA& testColor, const tcu::IVec2& position) const
	{
		DE_UNREF(position);

		// Values from 0.0 and 1.0 are accurate

		if (testColor.getRed() != 0)
			return false;
		if (testColor.getGreen() == 0)
			return false;
		if (testColor.getBlue() != 255)
			return false;
		return true;
	}

	void logInfo (tcu::TestLog& log) const
	{
		log << tcu::TestLog::Message << "Expecting color c = (0.0, x, 1.0), x > 0.0" << tcu::TestLog::EndMessage;
	}
};

class NoRedVerifier : public Verifier
{
public:
	NoRedVerifier (void)
	{
	}

	bool verify (const tcu::RGBA& testColor, const tcu::IVec2& position) const
	{
		DE_UNREF(position);
		return testColor.getRed() == 0;
	}

	void logInfo (tcu::TestLog& log) const
	{
		log << tcu::TestLog::Message << "Expecting zero-valued red channel." << tcu::TestLog::EndMessage;
	}
};

class SampleAverageVerifier : public Verifier
{
public:
				SampleAverageVerifier	(int _numSamples);

	bool		verify					(const tcu::RGBA& testColor, const tcu::IVec2& position) const;
	void		logInfo					(tcu::TestLog& log) const;

	const int	m_numSamples;
	const bool	m_isStatisticallySignificant;
	float		m_distanceThreshold;
};

SampleAverageVerifier::SampleAverageVerifier (int _numSamples)
	: m_numSamples					(_numSamples)
	, m_isStatisticallySignificant	(_numSamples >= 4)
	, m_distanceThreshold			(0.0f)
{
	// approximate Bates distribution as normal
	const float variance			= (1.0f / (12.0f * (float)m_numSamples));
	const float standardDeviation	= deFloatSqrt(variance);

	// 95% of means of sample positions are within 2 standard deviations if
	// they were randomly assigned. Sample patterns are expected to be more
	// uniform than a random pattern.
	m_distanceThreshold = 2 * standardDeviation;
}

bool SampleAverageVerifier::verify (const tcu::RGBA& testColor, const tcu::IVec2& position) const
{
	DE_UNREF(position);
	DE_ASSERT(m_isStatisticallySignificant);

	const tcu::Vec2	avgPosition				((float)testColor.getGreen() / 255.0f, (float)testColor.getBlue() / 255.0f);
	const tcu::Vec2	distanceFromCenter		= tcu::abs(avgPosition - tcu::Vec2(0.5f, 0.5f));

	return distanceFromCenter.x() < m_distanceThreshold && distanceFromCenter.y() < m_distanceThreshold;
}

void SampleAverageVerifier::logInfo (tcu::TestLog& log) const
{
	log << tcu::TestLog::Message << "Expecting average sample position to be near the pixel center. Maximum per-axis distance " << m_distanceThreshold << tcu::TestLog::EndMessage;
}

class PartialDiscardVerifier : public Verifier
{
public:
	PartialDiscardVerifier (void)
	{
	}

	bool verify (const tcu::RGBA& testColor, const tcu::IVec2& position) const
	{
		DE_UNREF(position);

		return (testColor.getGreen() != 0) && (testColor.getGreen() != 255);
	}

	void logInfo (tcu::TestLog& log) const
	{
		log << tcu::TestLog::Message << "Expecting color non-zero and non-saturated green channel" << tcu::TestLog::EndMessage;
	}
};

static bool verifyImageWithVerifier (const tcu::Surface& resultImage, tcu::TestLog& log, const Verifier& verifier, bool logOnSuccess = true)
{
	tcu::Surface	errorMask	(resultImage.getWidth(), resultImage.getHeight());
	bool			error		= false;

	tcu::clear(errorMask.getAccess(), tcu::Vec4(0.0f, 1.0f, 0.0f, 1.0f));

	if (logOnSuccess)
	{
		log << tcu::TestLog::Message << "Verifying image." << tcu::TestLog::EndMessage;
		verifier.logInfo(log);
	}

	for (int y = 0; y < resultImage.getHeight(); ++y)
	for (int x = 0; x < resultImage.getWidth(); ++x)
	{
		const tcu::RGBA color		= resultImage.getPixel(x, y);

		// verify color value is valid for this pixel position
		if (!verifier.verify(color, tcu::IVec2(x,y)))
		{
			error = true;
			errorMask.setPixel(x, y, tcu::RGBA::red());
		}
	}

	if (error)
	{
		// describe the verification logic if we haven't already
		if (!logOnSuccess)
			verifier.logInfo(log);

		log	<< tcu::TestLog::Message << "Image verification failed." << tcu::TestLog::EndMessage
			<< tcu::TestLog::ImageSet("Verification", "Image Verification")
			<< tcu::TestLog::Image("Result", "Result image", resultImage.getAccess())
			<< tcu::TestLog::Image("ErrorMask", "Error Mask", errorMask.getAccess())
			<< tcu::TestLog::EndImageSet;
	}
	else if (logOnSuccess)
	{
		log << tcu::TestLog::Message << "Image verification passed." << tcu::TestLog::EndMessage
			<< tcu::TestLog::ImageSet("Verification", "Image Verification")
			<< tcu::TestLog::Image("Result", "Result image", resultImage.getAccess())
			<< tcu::TestLog::EndImageSet;
	}

	return !error;
}

class MultisampleRenderCase : public MultisampleShaderRenderUtil::MultisampleRenderCase
{
public:
						MultisampleRenderCase		(Context& context, const char* name, const char* desc, int numSamples, RenderTarget target, int renderSize, int flags = 0);
	virtual				~MultisampleRenderCase		(void);

	virtual void		init						(void);

};

MultisampleRenderCase::MultisampleRenderCase (Context& context, const char* name, const char* desc, int numSamples, RenderTarget target, int renderSize, int flags)
	: MultisampleShaderRenderUtil::MultisampleRenderCase(context, name, desc, numSamples, target, renderSize, flags)
{
	DE_ASSERT(target < TARGET_LAST);
}

MultisampleRenderCase::~MultisampleRenderCase (void)
{
	MultisampleRenderCase::deinit();
}

void MultisampleRenderCase::init (void)
{
	const bool	supportsES32 = glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::es(3, 2));
	if (!supportsES32 && !m_context.getContextInfo().isExtensionSupported("GL_OES_sample_variables"))
		TCU_THROW(NotSupportedError, "Test requires GL_OES_sample_variables extension or a context version 3.2 or higher.");

	MultisampleShaderRenderUtil::MultisampleRenderCase::init();
}

class NumSamplesCase : public MultisampleRenderCase
{
public:
					NumSamplesCase				(Context& context, const char* name, const char* desc, int sampleCount, RenderTarget target);
					~NumSamplesCase				(void);

	std::string		genFragmentSource			(int numTargetSamples) const;
	bool			verifyImage					(const tcu::Surface& resultImage);

private:
	enum
	{
		RENDER_SIZE = 64
	};
};

NumSamplesCase::NumSamplesCase (Context& context, const char* name, const char* desc, int sampleCount, RenderTarget target)
	: MultisampleRenderCase(context, name, desc, sampleCount, target, RENDER_SIZE)
{
}

NumSamplesCase::~NumSamplesCase (void)
{
}

std::string NumSamplesCase::genFragmentSource (int numTargetSamples) const
{
	std::ostringstream	buf;
	const bool			supportsES32	= glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::es(3, 2));
	map<string, string>	args;
	args["GLSL_VERSION_DECL"]	= supportsES32 ? getGLSLVersionDeclaration(glu::GLSL_VERSION_320_ES) : getGLSLVersionDeclaration(glu::GLSL_VERSION_310_ES);
	args["GLSL_EXTENSION"]		= supportsES32 ? "" : "#extension GL_OES_sample_variables : require";

	buf <<	"${GLSL_VERSION_DECL}\n"
			"${GLSL_EXTENSION}\n"
			"layout(location = 0) out mediump vec4 fragColor;\n"
			"void main (void)\n"
			"{\n"
			"	fragColor = vec4(1.0, 0.0, 0.0, 1.0);\n"
			"	if (gl_NumSamples == " << numTargetSamples << ")\n"
			"		fragColor = vec4(0.0, 1.0, 0.0, 1.0);\n"
			"}\n";

	return tcu::StringTemplate(buf.str()).specialize(args);
}

bool NumSamplesCase::verifyImage (const tcu::Surface& resultImage)
{
	return verifyImageWithVerifier(resultImage, m_testCtx.getLog(), NoRedVerifier());
}

class MaxSamplesCase : public MultisampleRenderCase
{
public:
					MaxSamplesCase				(Context& context, const char* name, const char* desc, int sampleCount, RenderTarget target);
					~MaxSamplesCase				(void);

private:
	void			preDraw						(void);
	std::string		genFragmentSource			(int numTargetSamples) const;
	bool			verifyImage					(const tcu::Surface& resultImage);

	enum
	{
		RENDER_SIZE = 64
	};
};

MaxSamplesCase::MaxSamplesCase (Context& context, const char* name, const char* desc, int sampleCount, RenderTarget target)
	: MultisampleRenderCase(context, name, desc, sampleCount, target, RENDER_SIZE)
{
}

MaxSamplesCase::~MaxSamplesCase (void)
{
}

void MaxSamplesCase::preDraw (void)
{
	const glw::Functions&	gl			= m_context.getRenderContext().getFunctions();
	deInt32					maxSamples	= -1;

	// query samples
	{
		gl.getIntegerv(GL_MAX_SAMPLES, &maxSamples);
		GLU_EXPECT_NO_ERROR(gl.getError(), "query GL_MAX_SAMPLES");

		m_testCtx.getLog() << tcu::TestLog::Message << "GL_MAX_SAMPLES = " << maxSamples << tcu::TestLog::EndMessage;
	}

	// set samples
	{
		const int maxSampleLoc = gl.getUniformLocation(m_program->getProgram(), "u_maxSamples");
		if (maxSampleLoc == -1)
			throw tcu::TestError("Location of u_maxSamples was -1");

		gl.uniform1i(maxSampleLoc, maxSamples);
		GLU_EXPECT_NO_ERROR(gl.getError(), "set u_maxSamples uniform");

		m_testCtx.getLog() << tcu::TestLog::Message << "Set u_maxSamples = " << maxSamples << tcu::TestLog::EndMessage;
	}
}

std::string MaxSamplesCase::genFragmentSource (int numTargetSamples) const
{
	DE_UNREF(numTargetSamples);

	std::ostringstream	buf;
	const bool			supportsES32	= glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::es(3, 2));
	map<string, string>	args;
	args["GLSL_VERSION_DECL"]	= supportsES32 ? getGLSLVersionDeclaration(glu::GLSL_VERSION_320_ES) : getGLSLVersionDeclaration(glu::GLSL_VERSION_310_ES);
	args["GLSL_EXTENSION"]		= supportsES32 ? "" : "#extension GL_OES_sample_variables : require";

	buf <<	"${GLSL_VERSION_DECL}\n"
			"${GLSL_EXTENSION}\n"
			"layout(location = 0) out mediump vec4 fragColor;\n"
			"uniform mediump int u_maxSamples;\n"
			"void main (void)\n"
			"{\n"
			"	fragColor = vec4(1.0, 0.0, 0.0, 1.0);\n"
			"	if (gl_MaxSamples == u_maxSamples)\n"
			"		fragColor = vec4(0.0, 1.0, 0.0, 1.0);\n"
			"}\n";

	return tcu::StringTemplate(buf.str()).specialize(args);
}

bool MaxSamplesCase::verifyImage (const tcu::Surface& resultImage)
{
	return verifyImageWithVerifier(resultImage, m_testCtx.getLog(), NoRedVerifier());
}

class SampleIDCase : public MultisampleRenderCase
{
public:
					SampleIDCase				(Context& context, const char* name, const char* desc, int sampleCount, RenderTarget target);
					~SampleIDCase				(void);

	void			init						(void);

private:
	std::string		genFragmentSource			(int numTargetSamples) const;
	bool			verifyImage					(const tcu::Surface& resultImage);
	bool			verifySampleBuffers			(const std::vector<tcu::Surface>& resultBuffers);

	enum
	{
		RENDER_SIZE = 64
	};
	enum VerificationMode
	{
		VERIFY_USING_SAMPLES,
		VERIFY_USING_SELECTION,
	};

	const VerificationMode	m_vericationMode;
};

SampleIDCase::SampleIDCase (Context& context, const char* name, const char* desc, int sampleCount, RenderTarget target)
	: MultisampleRenderCase	(context, name, desc, sampleCount, target, RENDER_SIZE, MultisampleShaderRenderUtil::MultisampleRenderCase::FLAG_VERIFY_MSAA_TEXTURE_SAMPLE_BUFFERS)
	, m_vericationMode		((target == TARGET_TEXTURE) ? (VERIFY_USING_SAMPLES) : (VERIFY_USING_SELECTION))
{
}

SampleIDCase::~SampleIDCase (void)
{
}

void SampleIDCase::init (void)
{
	// log the test method and expectations
	if (m_vericationMode == VERIFY_USING_SAMPLES)
		m_testCtx.getLog()
			<< tcu::TestLog::Message
			<< "Writing gl_SampleID to the green channel of the texture and verifying texture values, expecting:\n"
			<< "	1) 0 with non-multisample targets.\n"
			<< "	2) value N at sample index N of a multisample texture\n"
			<< tcu::TestLog::EndMessage;
	else if (m_vericationMode == VERIFY_USING_SELECTION)
		m_testCtx.getLog()
			<< tcu::TestLog::Message
			<< "Selecting a single sample id for each pixel and writing color only if gl_SampleID == selected.\n"
			<< "Expecting all output pixels to be partially (multisample) or fully (singlesample) colored.\n"
			<< tcu::TestLog::EndMessage;
	else
		DE_ASSERT(false);

	MultisampleRenderCase::init();
}

std::string SampleIDCase::genFragmentSource (int numTargetSamples) const
{
	DE_ASSERT(numTargetSamples != 0);

	std::ostringstream buf;
	const bool			supportsES32	= glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::es(3, 2));
	map<string, string>	args;
	args["GLSL_VERSION_DECL"]	= supportsES32 ? getGLSLVersionDeclaration(glu::GLSL_VERSION_320_ES) : getGLSLVersionDeclaration(glu::GLSL_VERSION_310_ES);
	args["GLSL_EXTENSION"]		= supportsES32 ? "" : "#extension GL_OES_sample_variables : require";

	if (m_vericationMode == VERIFY_USING_SAMPLES)
	{
		// encode the id to the output, and then verify it during sampling
		buf <<	"${GLSL_VERSION_DECL}\n"
				"${GLSL_EXTENSION}\n"
				"layout(location = 0) out mediump vec4 fragColor;\n"
				"void main (void)\n"
				"{\n"
				"	highp float normalizedSample = float(gl_SampleID) / float(" << numTargetSamples << ");\n"
				"	fragColor = vec4(0.0, normalizedSample, 1.0, 1.0);\n"
				"}\n";
	}
	else if (m_vericationMode == VERIFY_USING_SELECTION)
	{
		if (numTargetSamples == 1)
		{
			// single sample, just verify value is 0
			buf <<	"${GLSL_VERSION_DECL}\n"
					"${GLSL_EXTENSION}\n"
					"layout(location = 0) out mediump vec4 fragColor;\n"
					"void main (void)\n"
					"{\n"
					"	if (gl_SampleID == 0)\n"
					"		fragColor = vec4(0.0, 1.0, 1.0, 1.0);\n"
					"	else\n"
					"		fragColor = vec4(0.0, 0.0, 1.0, 1.0);\n"
					"}\n";
		}
		else
		{
			// select only one sample per PIXEL
			buf <<	"${GLSL_VERSION_DECL}\n"
					"${GLSL_EXTENSION}\n"
					"in highp vec4 v_position;\n"
					"layout(location = 0) out mediump vec4 fragColor;\n"
					"void main (void)\n"
					"{\n"
					"	highp vec2 relPosition = (v_position.xy + vec2(1.0, 1.0)) / 2.0;\n"
					"	highp ivec2 pixelPos = ivec2(floor(relPosition * " << (int)RENDER_SIZE << ".0));\n"
					"	highp int selectedID = abs(pixelPos.x + 17 * pixelPos.y) % " << numTargetSamples << ";\n"
					"\n"
					"	if (gl_SampleID == selectedID)\n"
					"		fragColor = vec4(0.0, 1.0, 1.0, 1.0);\n"
					"	else\n"
					"		fragColor = vec4(0.0, 0.0, 1.0, 1.0);\n"
					"}\n";
		}
	}
	else
		DE_ASSERT(false);

	return tcu::StringTemplate(buf.str()).specialize(args);
}

bool SampleIDCase::verifyImage (const tcu::Surface& resultImage)
{
	if (m_vericationMode == VERIFY_USING_SAMPLES)
	{
		// never happens
		DE_ASSERT(false);
		return false;
	}
	else if (m_vericationMode == VERIFY_USING_SELECTION)
	{
		// should result in full blue and some green everywhere
		return verifyImageWithVerifier(resultImage, m_testCtx.getLog(), FullBlueSomeGreenVerifier());
	}
	else
	{
		DE_ASSERT(false);
		return false;
	}
}

bool SampleIDCase::verifySampleBuffers (const std::vector<tcu::Surface>& resultBuffers)
{
	// Verify all sample buffers
	bool allOk = true;

	// Log layers
	{
		m_testCtx.getLog() << tcu::TestLog::ImageSet("SampleBuffers", "Image sample buffers");
		for (int sampleNdx = 0; sampleNdx < (int)resultBuffers.size(); ++sampleNdx)
			m_testCtx.getLog() << tcu::TestLog::Image("Buffer" + de::toString(sampleNdx), "Sample " + de::toString(sampleNdx), resultBuffers[sampleNdx].getAccess());
		m_testCtx.getLog() << tcu::TestLog::EndImageSet;
	}

	m_testCtx.getLog() << tcu::TestLog::Message << "Verifying sample buffers" << tcu::TestLog::EndMessage;
	for (int sampleNdx = 0; sampleNdx < (int)resultBuffers.size(); ++sampleNdx)
	{
		// sample id should be sample index
		const int threshold = 255 / 4 / m_numTargetSamples + 1;
		const float sampleIdColor = (float)sampleNdx / (float)m_numTargetSamples;

		m_testCtx.getLog() << tcu::TestLog::Message << "Verifying sample " << (sampleNdx+1) << "/" << (int)resultBuffers.size() << tcu::TestLog::EndMessage;
		allOk &= verifyImageWithVerifier(resultBuffers[sampleNdx], m_testCtx.getLog(), ColorVerifier(tcu::Vec3(0.0f, sampleIdColor, 1.0f), tcu::IVec3(1, threshold, 1)), false);
	}

	if (!allOk)
		m_testCtx.getLog() << tcu::TestLog::Message << "Sample buffer verification failed" << tcu::TestLog::EndMessage;

	return allOk;
}

class SamplePosDistributionCase : public MultisampleRenderCase
{
public:
					SamplePosDistributionCase	(Context& context, const char* name, const char* desc, int sampleCount, RenderTarget target);
					~SamplePosDistributionCase	(void);

	void			init						(void);
private:
	enum
	{
		RENDER_SIZE = 64
	};

	std::string		genFragmentSource			(int numTargetSamples) const;
	bool			verifyImage					(const tcu::Surface& resultImage);
	bool			verifySampleBuffers			(const std::vector<tcu::Surface>& resultBuffers);
};

SamplePosDistributionCase::SamplePosDistributionCase (Context& context, const char* name, const char* desc, int sampleCount, RenderTarget target)
	: MultisampleRenderCase(context, name, desc, sampleCount, target, RENDER_SIZE, MultisampleShaderRenderUtil::MultisampleRenderCase::FLAG_VERIFY_MSAA_TEXTURE_SAMPLE_BUFFERS)
{
}

SamplePosDistributionCase::~SamplePosDistributionCase (void)
{
}

void SamplePosDistributionCase::init (void)
{
	// log the test method and expectations
	if (m_renderTarget == TARGET_TEXTURE)
	{
		m_testCtx.getLog()
			<< tcu::TestLog::Message
			<< "Verifying gl_SamplePosition value:\n"
			<< "	1) With non-multisample targets: Expect the center of the pixel.\n"
			<< "	2) With multisample targets:\n"
			<< "		a) Expect legal sample position.\n"
			<< "		b) Sample position is unique within the set of all sample positions of a pixel.\n"
			<< "		c) Sample position distribution is uniform or almost uniform.\n"
			<< tcu::TestLog::EndMessage;
	}
	else
	{
		m_testCtx.getLog()
			<< tcu::TestLog::Message
			<< "Verifying gl_SamplePosition value:\n"
			<< "	1) With non-multisample targets: Expect the center of the pixel.\n"
			<< "	2) With multisample targets:\n"
			<< "		a) Expect legal sample position.\n"
			<< "		b) Sample position distribution is uniform or almost uniform.\n"
			<< tcu::TestLog::EndMessage;
	}

	MultisampleRenderCase::init();
}

std::string SamplePosDistributionCase::genFragmentSource (int numTargetSamples) const
{
	DE_ASSERT(numTargetSamples != 0);
	DE_UNREF(numTargetSamples);

	const bool			multisampleTarget	= (m_numRequestedSamples > 0) || (m_renderTarget == TARGET_DEFAULT && m_context.getRenderTarget().getNumSamples() > 1);
	std::ostringstream	buf;
	const bool			supportsES32		= glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::es(3, 2));
	map<string, string>	args;
	args["GLSL_VERSION_DECL"]				= supportsES32 ? getGLSLVersionDeclaration(glu::GLSL_VERSION_320_ES) : getGLSLVersionDeclaration(glu::GLSL_VERSION_310_ES);
	args["GLSL_EXTENSION"]					= supportsES32 ? "\n" : "#extension GL_OES_sample_variables : require\n";

	if (multisampleTarget)
	{
		// encode the position to the output, use red channel as error channel
		buf <<	"${GLSL_VERSION_DECL}\n"
				"${GLSL_EXTENSION}\n"
				"layout(location = 0) out mediump vec4 fragColor;\n"
				"void main (void)\n"
				"{\n"
				"	if (gl_SamplePosition.x < 0.0 || gl_SamplePosition.x > 1.0 || gl_SamplePosition.y < 0.0 || gl_SamplePosition.y > 1.0)\n"
				"		fragColor = vec4(1.0, 0.0, 0.0, 1.0);\n"
				"	else\n"
				"		fragColor = vec4(0.0, gl_SamplePosition.x, gl_SamplePosition.y, 1.0);\n"
				"}\n";
	}
	else
	{
		// verify value is ok
		buf <<	"${GLSL_VERSION_DECL}\n"
				"${GLSL_EXTENSION}\n"
				"layout(location = 0) out mediump vec4 fragColor;\n"
				"void main (void)\n"
				"{\n"
				"	if (gl_SamplePosition.x != 0.5 || gl_SamplePosition.y != 0.5)\n"
				"		fragColor = vec4(1.0, 0.0, 0.0, 1.0);\n"
				"	else\n"
				"		fragColor = vec4(0.0, gl_SamplePosition.x, gl_SamplePosition.y, 1.0);\n"
				"}\n";
	}

	return tcu::StringTemplate(buf.str()).specialize(args);
}

bool SamplePosDistributionCase::verifyImage (const tcu::Surface& resultImage)
{
	const int				sampleCount	= (m_renderTarget == TARGET_DEFAULT) ? (m_context.getRenderTarget().getNumSamples()) : (m_numRequestedSamples);
	SampleAverageVerifier	verifier	(sampleCount);

	// check there is nothing in the error channel
	if (!verifyImageWithVerifier(resultImage, m_testCtx.getLog(), NoRedVerifier()))
		return false;

	// position average should be around 0.5, 0.5
	if (verifier.m_isStatisticallySignificant && !verifyImageWithVerifier(resultImage, m_testCtx.getLog(), verifier))
		throw MultisampleShaderRenderUtil::QualityWarning("Bias detected, sample positions are not uniformly distributed within the pixel");

	return true;
}

bool SamplePosDistributionCase::verifySampleBuffers (const std::vector<tcu::Surface>& resultBuffers)
{
	const int	width				= resultBuffers[0].getWidth();
	const int	height				= resultBuffers[0].getHeight();
	bool		allOk				= true;
	bool		distibutionError	= false;

	// Check sample range, uniqueness, and distribution, log layers
	{
		m_testCtx.getLog() << tcu::TestLog::ImageSet("SampleBuffers", "Image sample buffers");
		for (int sampleNdx = 0; sampleNdx < (int)resultBuffers.size(); ++sampleNdx)
			m_testCtx.getLog() << tcu::TestLog::Image("Buffer" + de::toString(sampleNdx), "Sample " + de::toString(sampleNdx), resultBuffers[sampleNdx].getAccess());
		m_testCtx.getLog() << tcu::TestLog::EndImageSet;
	}

	// verify range
	{
		bool rangeOk = true;

		m_testCtx.getLog() << tcu::TestLog::Message << "Verifying sample position range" << tcu::TestLog::EndMessage;
		for (int sampleNdx = 0; sampleNdx < (int)resultBuffers.size(); ++sampleNdx)
		{
			// shader does the check, just check the shader error output (red)
			m_testCtx.getLog() << tcu::TestLog::Message << "Verifying sample " << (sampleNdx+1) << "/" << (int)resultBuffers.size() << tcu::TestLog::EndMessage;
			rangeOk &= verifyImageWithVerifier(resultBuffers[sampleNdx], m_testCtx.getLog(), NoRedVerifier(), false);
		}

		if (!rangeOk)
		{
			allOk = false;

			m_testCtx.getLog() << tcu::TestLog::Message << "Sample position verification failed." << tcu::TestLog::EndMessage;
		}
	}

	// Verify uniqueness
	{
		bool					uniquenessOk	= true;
		tcu::Surface			errorMask		(width, height);
		std::vector<tcu::Vec2>	samplePositions	(resultBuffers.size());
		int						printCount		= 0;
		const int				printFloodLimit	= 5;

		tcu::clear(errorMask.getAccess(), tcu::Vec4(0.0f, 1.0f, 0.0f, 1.0f));

		m_testCtx.getLog() << tcu::TestLog::Message << "Verifying sample position uniqueness." << tcu::TestLog::EndMessage;

		for (int y = 0; y < height; ++y)
		for (int x = 0; x < width; ++x)
		{
			bool samplePosNotUnique = false;

			for (int sampleNdx = 0; sampleNdx < (int)resultBuffers.size(); ++sampleNdx)
			{
				const tcu::RGBA color = resultBuffers[sampleNdx].getPixel(x, y);
				samplePositions[sampleNdx] = tcu::Vec2((float)color.getGreen() / 255.0f, (float)color.getBlue() / 255.0f);
			}

			// Just check there are no two samples with same positions
			for (int sampleNdxA = 0;            sampleNdxA < (int)resultBuffers.size() && (!samplePosNotUnique || printCount < printFloodLimit); ++sampleNdxA)
			for (int sampleNdxB = sampleNdxA+1; sampleNdxB < (int)resultBuffers.size() && (!samplePosNotUnique || printCount < printFloodLimit); ++sampleNdxB)
			{
				if (samplePositions[sampleNdxA] == samplePositions[sampleNdxB])
				{
					if (++printCount <= printFloodLimit)
					{
						m_testCtx.getLog()
							<< tcu::TestLog::Message
							<< "Pixel (" << x << ", " << y << "): Samples " << sampleNdxA << " and " << sampleNdxB << " have the same position."
							<< tcu::TestLog::EndMessage;
					}

					samplePosNotUnique = true;
					uniquenessOk = false;
					errorMask.setPixel(x, y, tcu::RGBA::red());
				}
			}
		}

		// end result
		if (!uniquenessOk)
		{
			if (printCount > printFloodLimit)
				m_testCtx.getLog()
					<< tcu::TestLog::Message
					<< "...\n"
					<< "Omitted " << (printCount-printFloodLimit) << " error descriptions."
					<< tcu::TestLog::EndMessage;

			m_testCtx.getLog()
				<< tcu::TestLog::Message << "Image verification failed." << tcu::TestLog::EndMessage
				<< tcu::TestLog::ImageSet("Verification", "Image Verification")
				<< tcu::TestLog::Image("ErrorMask", "Error Mask", errorMask.getAccess())
				<< tcu::TestLog::EndImageSet;

			allOk = false;
		}
	}

	// check distribution
	{
		const SampleAverageVerifier verifier		(m_numTargetSamples);
		tcu::Surface				errorMask		(width, height);
		int							printCount		= 0;
		const int					printFloodLimit	= 5;

		tcu::clear(errorMask.getAccess(), tcu::Vec4(0.0f, 1.0f, 0.0f, 1.0f));

		// don't bother with small sample counts
		if (verifier.m_isStatisticallySignificant)
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "Verifying sample position distribution is (nearly) unbiased." << tcu::TestLog::EndMessage;
			verifier.logInfo(m_testCtx.getLog());

			for (int y = 0; y < height; ++y)
			for (int x = 0; x < width; ++x)
			{
				tcu::IVec3 colorSum(0, 0, 0);

				// color average

				for (int sampleNdx = 0; sampleNdx < (int)resultBuffers.size(); ++sampleNdx)
				{
					const tcu::RGBA color = resultBuffers[sampleNdx].getPixel(x, y);
					colorSum.x() += color.getRed();
					colorSum.y() += color.getBlue();
					colorSum.z() += color.getGreen();
				}

				colorSum.x() /= m_numTargetSamples;
				colorSum.y() /= m_numTargetSamples;
				colorSum.z() /= m_numTargetSamples;

				// verify average sample position

				if (!verifier.verify(tcu::RGBA(colorSum.x(), colorSum.y(), colorSum.z(), 0), tcu::IVec2(x, y)))
				{
					if (++printCount <= printFloodLimit)
					{
						m_testCtx.getLog()
							<< tcu::TestLog::Message
							<< "Pixel (" << x << ", " << y << "): Sample distribution is biased."
							<< tcu::TestLog::EndMessage;
					}

					distibutionError = true;
					errorMask.setPixel(x, y, tcu::RGBA::red());
				}
			}

			// sub-verification result
			if (distibutionError)
			{
				if (printCount > printFloodLimit)
					m_testCtx.getLog()
						<< tcu::TestLog::Message
						<< "...\n"
						<< "Omitted " << (printCount-printFloodLimit) << " error descriptions."
						<< tcu::TestLog::EndMessage;

				m_testCtx.getLog()
					<< tcu::TestLog::Message << "Image verification failed." << tcu::TestLog::EndMessage
					<< tcu::TestLog::ImageSet("Verification", "Image Verification")
					<< tcu::TestLog::Image("ErrorMask", "Error Mask", errorMask.getAccess())
					<< tcu::TestLog::EndImageSet;
			}
		}
	}

	// results
	if (!allOk)
		return false;
	else if (distibutionError)
		throw MultisampleShaderRenderUtil::QualityWarning("Bias detected, sample positions are not uniformly distributed within the pixel");
	else
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Verification ok." << tcu::TestLog::EndMessage;
		return true;
	}
}

class SamplePosCorrectnessCase : public MultisampleRenderCase
{
public:
					SamplePosCorrectnessCase	(Context& context, const char* name, const char* desc, int sampleCount, RenderTarget target);
					~SamplePosCorrectnessCase	(void);

	void			init						(void);
private:
	enum
	{
		RENDER_SIZE = 32
	};

	void			preDraw						(void);
	void			postDraw					(void);

	std::string		genVertexSource				(int numTargetSamples) const;
	std::string		genFragmentSource			(int numTargetSamples) const;
	bool			verifyImage					(const tcu::Surface& resultImage);

	bool			m_useSampleQualifier;
};

SamplePosCorrectnessCase::SamplePosCorrectnessCase (Context& context, const char* name, const char* desc, int sampleCount, RenderTarget target)
	: MultisampleRenderCase	(context, name, desc, sampleCount, target, RENDER_SIZE)
	, m_useSampleQualifier	(false)
{
}

SamplePosCorrectnessCase::~SamplePosCorrectnessCase (void)
{
}

void SamplePosCorrectnessCase::init (void)
{
	const bool supportsES32 = glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::es(3, 2));
	// requirements: per-invocation interpolation required
	if (!supportsES32 && !m_context.getContextInfo().isExtensionSupported("GL_OES_shader_multisample_interpolation") &&
		!m_context.getContextInfo().isExtensionSupported("GL_OES_sample_shading"))
		TCU_THROW(NotSupportedError, "Test requires GL_OES_shader_multisample_interpolation or GL_OES_sample_shading extension or a context version 3.2 or higher.");

	// prefer to use the sample qualifier path
	m_useSampleQualifier = m_context.getContextInfo().isExtensionSupported("GL_OES_shader_multisample_interpolation");

	// log the test method and expectations
	m_testCtx.getLog()
		<< tcu::TestLog::Message
		<< "Verifying gl_SamplePosition correctness:\n"
		<< "	1) Varying values should be sampled at the sample position.\n"
		<< "		=> fract(screenSpacePosition) == gl_SamplePosition\n"
		<< tcu::TestLog::EndMessage;

	MultisampleRenderCase::init();
}

void SamplePosCorrectnessCase::preDraw (void)
{
	if (!m_useSampleQualifier)
	{
		const glw::Functions& gl = m_context.getRenderContext().getFunctions();

		// use GL_OES_sample_shading to set per fragment sample invocation interpolation
		gl.enable(GL_SAMPLE_SHADING);
		gl.minSampleShading(1.0f);
		GLU_EXPECT_NO_ERROR(gl.getError(), "set ratio");

		m_testCtx.getLog() << tcu::TestLog::Message << "Enabling per-sample interpolation with GL_SAMPLE_SHADING." << tcu::TestLog::EndMessage;
	}
}

void SamplePosCorrectnessCase::postDraw (void)
{
	if (!m_useSampleQualifier)
	{
		const glw::Functions& gl = m_context.getRenderContext().getFunctions();

		gl.disable(GL_SAMPLE_SHADING);
		gl.minSampleShading(1.0f);
		GLU_EXPECT_NO_ERROR(gl.getError(), "set ratio");
	}
}

std::string SamplePosCorrectnessCase::genVertexSource (int numTargetSamples) const
{
	DE_UNREF(numTargetSamples);

	std::ostringstream	buf;
	const bool			supportsES32	= glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::es(3, 2));
	map<string, string>	args;
	args["GLSL_VERSION_DECL"]	= supportsES32 ? getGLSLVersionDeclaration(glu::GLSL_VERSION_320_ES) : getGLSLVersionDeclaration(glu::GLSL_VERSION_310_ES);
	args["GLSL_EXTENSION"]		= supportsES32 ? "" : m_useSampleQualifier ? "#extension GL_OES_shader_multisample_interpolation : require" : "";

	buf <<	"${GLSL_VERSION_DECL}\n"
			"${GLSL_EXTENSION}\n"
		<<	"in highp vec4 a_position;\n"
		<<	((m_useSampleQualifier) ? ("sample ") : ("")) << "out highp vec4 v_position;\n"
			"void main (void)\n"
			"{\n"
			"	gl_Position = a_position;\n"
			"	v_position = a_position;\n"
			"}\n";

	return tcu::StringTemplate(buf.str()).specialize(args);
}

std::string SamplePosCorrectnessCase::genFragmentSource (int numTargetSamples) const
{
	DE_UNREF(numTargetSamples);

	std::ostringstream	buf;
	const bool			supportsES32	= glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::es(3, 2));
	map<string, string>	args;
	args["GLSL_VERSION_DECL"]			= supportsES32 ? getGLSLVersionDeclaration(glu::GLSL_VERSION_320_ES) : getGLSLVersionDeclaration(glu::GLSL_VERSION_310_ES);
	args["GLSL_SAMPLE_EXTENSION"]		= supportsES32 ? "" : "#extension GL_OES_sample_variables : require";
	args["GLSL_MULTISAMPLE_EXTENSION"]	= supportsES32 ? "" : m_useSampleQualifier ? "#extension GL_OES_shader_multisample_interpolation : require" : "";

	// encode the position to the output, use red channel as error channel
	buf <<	"${GLSL_VERSION_DECL}\n"
			"${GLSL_SAMPLE_EXTENSION}\n"
			"${GLSL_MULTISAMPLE_EXTENSION}\n"
		<<	((m_useSampleQualifier) ? ("sample ") : ("")) << "in highp vec4 v_position;\n"
			"layout(location = 0) out mediump vec4 fragColor;\n"
			"void main (void)\n"
			"{\n"
			"	const highp float maxDistance = 0.15625; // 4 subpixel bits. Assume 3 accurate bits + 0.03125 for other errors\n" // 0.03125 = mediump epsilon when value = 32 (RENDER_SIZE)
			"\n"
			"	highp vec2 screenSpacePosition = (v_position.xy + vec2(1.0, 1.0)) / 2.0 * " << (int)RENDER_SIZE << ".0;\n"
			"	highp ivec2 nearbyPixel = ivec2(floor(screenSpacePosition));\n"
			"	bool allOk = false;\n"
			"\n"
			"	// sample at edge + inaccuaries may cause us to round to any neighboring pixel\n"
			"	// check all neighbors for any match\n"
			"	for (highp int dy = -1; dy <= 1; ++dy)\n"
			"	for (highp int dx = -1; dx <= 1; ++dx)\n"
			"	{\n"
			"		highp ivec2 currentPixel = nearbyPixel + ivec2(dx, dy);\n"
			"		highp vec2 candidateSamplingPos = vec2(currentPixel) + gl_SamplePosition.xy;\n"
			"		highp vec2 positionDiff = abs(candidateSamplingPos - screenSpacePosition);\n"
			"		if (positionDiff.x < maxDistance && positionDiff.y < maxDistance)\n"
			"			allOk = true;\n"
			"	}\n"
			"\n"
			"	if (allOk)\n"
			"		fragColor = vec4(0.0, 1.0, 0.0, 1.0);\n"
			"	else\n"
			"		fragColor = vec4(1.0, 0.0, 0.0, 1.0);\n"
			"}\n";

	return tcu::StringTemplate(buf.str()).specialize(args);
}

bool SamplePosCorrectnessCase::verifyImage (const tcu::Surface& resultImage)
{
	return verifyImageWithVerifier(resultImage, m_testCtx.getLog(), NoRedVerifier());
}

class SampleMaskBaseCase : public MultisampleRenderCase
{
public:
	enum ShaderRunMode
	{
		RUN_PER_PIXEL = 0,
		RUN_PER_SAMPLE,
		RUN_PER_TWO_SAMPLES,

		RUN_LAST
	};

						SampleMaskBaseCase			(Context& context, const char* name, const char* desc, int sampleCount, RenderTarget target, int renderSize, ShaderRunMode runMode, int flags = 0);
	virtual				~SampleMaskBaseCase			(void);

protected:
	virtual void		init						(void);
	virtual void		preDraw						(void);
	virtual void		postDraw					(void);
	virtual bool		verifyImage					(const tcu::Surface& resultImage);

	const ShaderRunMode	m_runMode;
};

SampleMaskBaseCase::SampleMaskBaseCase (Context& context, const char* name, const char* desc, int sampleCount, RenderTarget target, int renderSize, ShaderRunMode runMode, int flags)
	: MultisampleRenderCase	(context, name, desc, sampleCount, target, renderSize, flags)
	, m_runMode				(runMode)
{
	DE_ASSERT(runMode < RUN_LAST);
}

SampleMaskBaseCase::~SampleMaskBaseCase (void)
{
}

void SampleMaskBaseCase::init (void)
{
	const bool supportsES32 = glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::es(3, 2));
	// required extra extension
	if (m_runMode == RUN_PER_TWO_SAMPLES && !supportsES32 && !m_context.getContextInfo().isExtensionSupported("GL_OES_sample_shading"))
			TCU_THROW(NotSupportedError, "Test requires GL_OES_sample_shading extension or a context version 3.2 or higher.");

	MultisampleRenderCase::init();
}

void SampleMaskBaseCase::preDraw (void)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (m_runMode == RUN_PER_TWO_SAMPLES)
	{
		gl.enable(GL_SAMPLE_SHADING);
		gl.minSampleShading(0.5f);
		GLU_EXPECT_NO_ERROR(gl.getError(), "enable sample shading");

		m_testCtx.getLog() << tcu::TestLog::Message << "Enabled GL_SAMPLE_SHADING, value = 0.5" << tcu::TestLog::EndMessage;
	}
}

void SampleMaskBaseCase::postDraw (void)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (m_runMode == RUN_PER_TWO_SAMPLES)
	{
		gl.disable(GL_SAMPLE_SHADING);
		gl.minSampleShading(1.0f);
		GLU_EXPECT_NO_ERROR(gl.getError(), "disable sample shading");
	}
}

bool SampleMaskBaseCase::verifyImage (const tcu::Surface& resultImage)
{
	// shader does the verification
	return verifyImageWithVerifier(resultImage, m_testCtx.getLog(), NoRedVerifier());
}

class SampleMaskCase : public SampleMaskBaseCase
{
public:
						SampleMaskCase				(Context& context, const char* name, const char* desc, int sampleCount, RenderTarget target);
						~SampleMaskCase				(void);

	void				init						(void);
	void				preDraw						(void);
	void				postDraw					(void);

private:
	enum
	{
		RENDER_SIZE = 64
	};

	std::string			genFragmentSource			(int numTargetSamples) const;
};

SampleMaskCase::SampleMaskCase (Context& context, const char* name, const char* desc, int sampleCount, RenderTarget target)
	: SampleMaskBaseCase(context, name, desc, sampleCount, target, RENDER_SIZE, RUN_PER_PIXEL)
{
}

SampleMaskCase::~SampleMaskCase (void)
{
}

void SampleMaskCase::init (void)
{
	// log the test method and expectations
	m_testCtx.getLog()
		<< tcu::TestLog::Message
		<< "Verifying gl_SampleMaskIn value with SAMPLE_MASK state. gl_SampleMaskIn does not contain any bits set that are have been killed by SAMPLE_MASK state. Expecting:\n"
		<< "	1) With multisample targets: gl_SampleMaskIn AND ~(SAMPLE_MASK) should be zero.\n"
		<< "	2) With non-multisample targets: SAMPLE_MASK state is only ANDed as a multisample operation. gl_SampleMaskIn should only have its last bit set regardless of SAMPLE_MASK state.\n"
		<< tcu::TestLog::EndMessage;

	SampleMaskBaseCase::init();
}

void SampleMaskCase::preDraw (void)
{
	const glw::Functions&	gl					= m_context.getRenderContext().getFunctions();
	const bool				multisampleTarget	= (m_numRequestedSamples > 0) || (m_renderTarget == TARGET_DEFAULT && m_context.getRenderTarget().getNumSamples() > 1);
	const deUint32			fullMask			= (deUint32)0xAAAAAAAAUL;
	const deUint32			maskMask			= (1U << m_numTargetSamples) - 1;
	const deUint32			effectiveMask		=  fullMask & maskMask;

	// set test mask
	gl.enable(GL_SAMPLE_MASK);
	gl.sampleMaski(0, effectiveMask);
	GLU_EXPECT_NO_ERROR(gl.getError(), "set mask");

	m_testCtx.getLog() << tcu::TestLog::Message << "Setting sample mask " << tcu::Format::Hex<4>(effectiveMask) << tcu::TestLog::EndMessage;

	// set multisample case uniforms
	if (multisampleTarget)
	{
		const int maskLoc = gl.getUniformLocation(m_program->getProgram(), "u_sampleMask");
		if (maskLoc == -1)
			throw tcu::TestError("Location of u_mask was -1");

		gl.uniform1ui(maskLoc, effectiveMask);
		GLU_EXPECT_NO_ERROR(gl.getError(), "set mask uniform");
	}

	// base class logic
	SampleMaskBaseCase::preDraw();
}

void SampleMaskCase::postDraw (void)
{
	const glw::Functions&	gl			= m_context.getRenderContext().getFunctions();
	const deUint32			fullMask	= (1U << m_numTargetSamples) - 1;

	gl.disable(GL_SAMPLE_MASK);
	gl.sampleMaski(0, fullMask);
	GLU_EXPECT_NO_ERROR(gl.getError(), "set mask");

	// base class logic
	SampleMaskBaseCase::postDraw();
}

std::string SampleMaskCase::genFragmentSource (int numTargetSamples) const
{
	DE_ASSERT(numTargetSamples != 0);

	const bool			multisampleTarget	= (m_numRequestedSamples > 0) || (m_renderTarget == TARGET_DEFAULT && m_context.getRenderTarget().getNumSamples() > 1);
	std::ostringstream	buf;
	const bool			supportsES32		= glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::es(3, 2));
	map<string, string>	args;
	args["GLSL_VERSION_DECL"]				= supportsES32 ? getGLSLVersionDeclaration(glu::GLSL_VERSION_320_ES) : getGLSLVersionDeclaration(glu::GLSL_VERSION_310_ES);
	args["GLSL_EXTENSION"]					= supportsES32 ? "" : "#extension GL_OES_sample_variables : require";

	// test supports only one sample mask word
	if (numTargetSamples > 32)
		TCU_THROW(NotSupportedError, "Sample count larger than 32 is not supported.");

	if (multisampleTarget)
	{
		buf <<	"${GLSL_VERSION_DECL}\n"
				"${GLSL_EXTENSION}\n"
				"layout(location = 0) out mediump vec4 fragColor;\n"
				"uniform highp uint u_sampleMask;\n"
				"void main (void)\n"
				"{\n"
				"	if ((uint(gl_SampleMaskIn[0]) & (~u_sampleMask)) != 0u)\n"
				"		fragColor = vec4(1.0, 0.0, 0.0, 1.0);\n"
				"	else\n"
				"		fragColor = vec4(0.0, 1.0, 0.0, 1.0);\n"
				"}\n";
	}
	else
	{
		// non-multisample targets don't get multisample operations like ANDing with mask

		buf <<	"${GLSL_VERSION_DECL}\n"
				"${GLSL_EXTENSION}\n"
				"layout(location = 0) out mediump vec4 fragColor;\n"
				"uniform highp uint u_sampleMask;\n"
				"void main (void)\n"
				"{\n"
				"	if (gl_SampleMaskIn[0] != 1)\n"
				"		fragColor = vec4(1.0, 0.0, 0.0, 1.0);\n"
				"	else\n"
				"		fragColor = vec4(0.0, 1.0, 0.0, 1.0);\n"
				"}\n";
	}

	return tcu::StringTemplate(buf.str()).specialize(args);
}

class SampleMaskCountCase : public SampleMaskBaseCase
{
public:
						SampleMaskCountCase			(Context& context, const char* name, const char* desc, int sampleCount, RenderTarget target, ShaderRunMode runMode);
						~SampleMaskCountCase		(void);

	void				init						(void);
	void				preDraw						(void);
	void				postDraw					(void);

private:
	enum
	{
		RENDER_SIZE = 64
	};

	std::string			genFragmentSource			(int numTargetSamples) const;
};

SampleMaskCountCase::SampleMaskCountCase (Context& context, const char* name, const char* desc, int sampleCount, RenderTarget target, ShaderRunMode runMode)
	: SampleMaskBaseCase(context, name, desc, sampleCount, target, RENDER_SIZE, runMode)
{
	DE_ASSERT(runMode < RUN_LAST);
}

SampleMaskCountCase::~SampleMaskCountCase (void)
{
}

void SampleMaskCountCase::init (void)
{
	// log the test method and expectations
	if (m_runMode == RUN_PER_PIXEL)
		m_testCtx.getLog()
			<< tcu::TestLog::Message
			<< "Verifying gl_SampleMaskIn.\n"
			<< "	Fragment shader may be invoked [1, numSamples] times.\n"
			<< "	=> gl_SampleMaskIn should have the number of bits set in range [1, numSamples]\n"
			<< tcu::TestLog::EndMessage;
	else if (m_runMode == RUN_PER_SAMPLE)
		m_testCtx.getLog()
			<< tcu::TestLog::Message
			<< "Verifying gl_SampleMaskIn.\n"
			<< "	Fragment will be invoked numSamples times.\n"
			<< "	=> gl_SampleMaskIn should have only one bit set.\n"
			<< tcu::TestLog::EndMessage;
	else if (m_runMode == RUN_PER_TWO_SAMPLES)
		m_testCtx.getLog()
			<< tcu::TestLog::Message
			<< "Verifying gl_SampleMaskIn.\n"
			<< "	Fragment shader may be invoked [ceil(numSamples/2), numSamples] times.\n"
			<< "	=> gl_SampleMaskIn should have the number of bits set in range [1, numSamples - ceil(numSamples/2) + 1]:\n"
			<< tcu::TestLog::EndMessage;
	else
		DE_ASSERT(false);

	SampleMaskBaseCase::init();
}

void SampleMaskCountCase::preDraw (void)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (m_runMode == RUN_PER_PIXEL)
	{
		const int maxLoc = gl.getUniformLocation(m_program->getProgram(), "u_maxBitCount");
		const int minLoc = gl.getUniformLocation(m_program->getProgram(), "u_minBitCount");
		const int minBitCount = 1;
		const int maxBitCount = m_numTargetSamples;

		if (maxLoc == -1)
			throw tcu::TestError("Location of u_maxBitCount was -1");
		if (minLoc == -1)
			throw tcu::TestError("Location of u_minBitCount was -1");

		gl.uniform1i(minLoc, minBitCount);
		gl.uniform1i(maxLoc, maxBitCount);
		GLU_EXPECT_NO_ERROR(gl.getError(), "set limits");

		m_testCtx.getLog() << tcu::TestLog::Message << "Setting minBitCount = " << minBitCount << ", maxBitCount = " << maxBitCount << tcu::TestLog::EndMessage;
	}
	else if (m_runMode == RUN_PER_TWO_SAMPLES)
	{
		const int maxLoc = gl.getUniformLocation(m_program->getProgram(), "u_maxBitCount");
		const int minLoc = gl.getUniformLocation(m_program->getProgram(), "u_minBitCount");

		// Worst case: all but one shader invocations get one sample, one shader invocation the rest of the samples
		const int minInvocationCount = ((m_numTargetSamples + 1) / 2);
		const int minBitCount = 1;
		const int maxBitCount = m_numTargetSamples - ((minInvocationCount-1) * minBitCount);

		if (maxLoc == -1)
			throw tcu::TestError("Location of u_maxBitCount was -1");
		if (minLoc == -1)
			throw tcu::TestError("Location of u_minBitCount was -1");

		gl.uniform1i(minLoc, minBitCount);
		gl.uniform1i(maxLoc, maxBitCount);
		GLU_EXPECT_NO_ERROR(gl.getError(), "set limits");

		m_testCtx.getLog() << tcu::TestLog::Message << "Setting minBitCount = " << minBitCount << ", maxBitCount = " << maxBitCount << tcu::TestLog::EndMessage;
	}

	SampleMaskBaseCase::preDraw();
}

void SampleMaskCountCase::postDraw (void)
{
	SampleMaskBaseCase::postDraw();
}

std::string SampleMaskCountCase::genFragmentSource (int numTargetSamples) const
{
	DE_ASSERT(numTargetSamples != 0);

	std::ostringstream	buf;
	const bool			supportsES32	= glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::es(3, 2));
	map<string, string>	args;
	args["GLSL_VERSION_DECL"]	= supportsES32 ? getGLSLVersionDeclaration(glu::GLSL_VERSION_320_ES) : getGLSLVersionDeclaration(glu::GLSL_VERSION_310_ES);
	args["GLSL_EXTENSION"]		= supportsES32 ? "" : "#extension GL_OES_sample_variables : require";

	// test supports only one sample mask word
	if (numTargetSamples > 32)
		TCU_THROW(NotSupportedError, "Sample count larger than 32 is not supported.");

	// count the number of the bits in gl_SampleMask

	buf <<	"${GLSL_VERSION_DECL}\n"
			"${GLSL_EXTENSION}\n"
			"layout(location = 0) out mediump vec4 fragColor;\n";

	if (m_runMode != RUN_PER_SAMPLE)
		buf <<	"uniform highp int u_minBitCount;\n"
				"uniform highp int u_maxBitCount;\n";

	buf <<	"void main (void)\n"
			"{\n"
			"	mediump int maskBitCount = 0;\n"
			"	for (int i = 0; i < 32; ++i)\n"
			"		if (((gl_SampleMaskIn[0] >> i) & 0x01) == 0x01)\n"
			"			++maskBitCount;\n"
			"\n";

	if (m_runMode == RUN_PER_SAMPLE)
	{
		// check the validity here
		buf <<	"	// force per-sample shading\n"
				"	highp float blue = float(gl_SampleID);\n"
				"\n"
				"	if (maskBitCount != 1)\n"
				"		fragColor = vec4(1.0, 0.0, blue, 1.0);\n"
				"	else\n"
				"		fragColor = vec4(0.0, 1.0, blue, 1.0);\n"
				"}\n";
	}
	else
	{
		// check the validity here
		buf <<	"	if (maskBitCount < u_minBitCount || maskBitCount > u_maxBitCount)\n"
				"		fragColor = vec4(1.0, 0.0, 0.0, 1.0);\n"
				"	else\n"
				"		fragColor = vec4(0.0, 1.0, 0.0, 1.0);\n"
				"}\n";
	}

	return tcu::StringTemplate(buf.str()).specialize(args);
}

class SampleMaskUniqueCase : public SampleMaskBaseCase
{
public:
						SampleMaskUniqueCase		(Context& context, const char* name, const char* desc, int sampleCount, RenderTarget target, ShaderRunMode runMode);
						~SampleMaskUniqueCase		(void);

	void				init						(void);

private:
	enum
	{
		RENDER_SIZE = 64
	};

	std::string			genFragmentSource			(int numTargetSamples) const;
	bool				verifySampleBuffers			(const std::vector<tcu::Surface>& resultBuffers);
};

SampleMaskUniqueCase::SampleMaskUniqueCase (Context& context, const char* name, const char* desc, int sampleCount, RenderTarget target, ShaderRunMode runMode)
	: SampleMaskBaseCase(context, name, desc, sampleCount, target, RENDER_SIZE, runMode, MultisampleShaderRenderUtil::MultisampleRenderCase::FLAG_VERIFY_MSAA_TEXTURE_SAMPLE_BUFFERS)
{
	DE_ASSERT(runMode == RUN_PER_SAMPLE);
	DE_ASSERT(target == TARGET_TEXTURE);
}

SampleMaskUniqueCase::~SampleMaskUniqueCase (void)
{
}

void SampleMaskUniqueCase::init (void)
{
	// log the test method and expectations
	m_testCtx.getLog()
		<< tcu::TestLog::Message
		<< "Verifying gl_SampleMaskIn.\n"
		<< "	Fragment will be invoked numSamples times.\n"
		<< "	=> gl_SampleMaskIn should have only one bit set\n"
		<< "	=> and that bit index should be unique within other fragment shader invocations of that pixel.\n"
		<< "	Writing sampleMask bit index to green channel in render shader. Verifying uniqueness in sampler shader.\n"
		<< tcu::TestLog::EndMessage;

	SampleMaskBaseCase::init();
}

std::string SampleMaskUniqueCase::genFragmentSource (int numTargetSamples) const
{
	DE_ASSERT(numTargetSamples != 0);

	std::ostringstream	buf;
	const bool			supportsES32	= glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::es(3, 2));
	map<string, string>	args;
	args["GLSL_VERSION_DECL"]	= supportsES32 ? getGLSLVersionDeclaration(glu::GLSL_VERSION_320_ES) : getGLSLVersionDeclaration(glu::GLSL_VERSION_310_ES);
	args["GLSL_EXTENSION"]		= supportsES32 ? "" : "#extension GL_OES_sample_variables : require";

	// test supports only one sample mask word
	if (numTargetSamples > 32)
		TCU_THROW(NotSupportedError, "Sample count larger than 32 is not supported.");

	// find our sampleID by searching for unique bit.
	buf <<	"${GLSL_VERSION_DECL}\n"
			"${GLSL_EXTENSION}\n"
			"layout(location = 0) out mediump vec4 fragColor;\n"
			"void main (void)\n"
			"{\n"
			"	mediump int firstIndex = -1;\n"
			"	for (int i = 0; i < 32; ++i)\n"
			"	{\n"
			"		if (((gl_SampleMaskIn[0] >> i) & 0x01) == 0x01)\n"
			"		{\n"
			"			firstIndex = i;\n"
			"			break;\n"
			"		}\n"
			"	}\n"
			"\n"
			"	bool notUniqueError = false;\n"
			"	for (int i = firstIndex + 1; i < 32; ++i)\n"
			"		if (((gl_SampleMaskIn[0] >> i) & 0x01) == 0x01)\n"
			"			notUniqueError = true;\n"
			"\n"
			"	highp float encodedSampleId = float(firstIndex) / " << numTargetSamples <<".0;\n"
			"\n"
			"	// force per-sample shading\n"
			"	highp float blue = float(gl_SampleID);\n"
			"\n"
			"	if (notUniqueError)\n"
			"		fragColor = vec4(1.0, 0.0, blue, 1.0);\n"
			"	else\n"
			"		fragColor = vec4(0.0, encodedSampleId, blue, 1.0);\n"
			"}\n";

	return tcu::StringTemplate(buf.str()).specialize(args);
}

bool SampleMaskUniqueCase::verifySampleBuffers (const std::vector<tcu::Surface>& resultBuffers)
{
	const int	width				= resultBuffers[0].getWidth();
	const int	height				= resultBuffers[0].getHeight();
	bool		allOk				= true;

	// Log samples
	{
		m_testCtx.getLog() << tcu::TestLog::ImageSet("SampleBuffers", "Image sample buffers");
		for (int sampleNdx = 0; sampleNdx < (int)resultBuffers.size(); ++sampleNdx)
			m_testCtx.getLog() << tcu::TestLog::Image("Buffer" + de::toString(sampleNdx), "Sample " + de::toString(sampleNdx), resultBuffers[sampleNdx].getAccess());
		m_testCtx.getLog() << tcu::TestLog::EndImageSet;
	}

	// check for earlier errors (in fragment shader)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Verifying fragment shader invocation found only one set sample mask bit." << tcu::TestLog::EndMessage;

		for (int sampleNdx = 0; sampleNdx < (int)resultBuffers.size(); ++sampleNdx)
		{
			// shader does the check, just check the shader error output (red)
			m_testCtx.getLog() << tcu::TestLog::Message << "Verifying sample " << (sampleNdx+1) << "/" << (int)resultBuffers.size() << tcu::TestLog::EndMessage;
			allOk &= verifyImageWithVerifier(resultBuffers[sampleNdx], m_testCtx.getLog(), NoRedVerifier(), false);
		}

		if (!allOk)
		{
			// can't check the uniqueness if the masks don't work at all
			m_testCtx.getLog() << tcu::TestLog::Message << "Could not get mask information from the rendered image, cannot continue verification." << tcu::TestLog::EndMessage;
			return false;
		}
	}

	// verify index / index ranges

	if (m_numRequestedSamples == 0)
	{
		// single sample target, expect index=0

		m_testCtx.getLog() << tcu::TestLog::Message << "Verifying sample mask bit index is 0." << tcu::TestLog::EndMessage;

		// only check the mask index
		allOk &= verifyImageWithVerifier(resultBuffers[0], m_testCtx.getLog(), ColorVerifier(tcu::Vec3(0.0f, 0.0f, 0.0f), tcu::IVec3(255, 8, 255)), false);
	}
	else
	{
		// check uniqueness

		tcu::Surface		errorMask		(width, height);
		bool				uniquenessOk	= true;
		int					printCount		= 0;
		const int			printFloodLimit	= 5;
		std::vector<int>	maskBitIndices	(resultBuffers.size());

		tcu::clear(errorMask.getAccess(), tcu::Vec4(0.0f, 1.0f, 0.0f, 1.0f));

		m_testCtx.getLog() << tcu::TestLog::Message << "Verifying per-invocation sample mask bit is unique." << tcu::TestLog::EndMessage;

		for (int y = 0; y < height; ++y)
		for (int x = 0; x < width; ++x)
		{
			bool maskNdxNotUnique = false;

			// decode index
			for (int sampleNdx = 0; sampleNdx < (int)resultBuffers.size(); ++sampleNdx)
			{
				const tcu::RGBA color = resultBuffers[sampleNdx].getPixel(x, y);
				maskBitIndices[sampleNdx] = (int)deFloatRound((float)color.getGreen() / 255.0f * (float)m_numTargetSamples);
			}

			// just check there are no two invocations with the same bit index
			for (int sampleNdxA = 0;            sampleNdxA < (int)resultBuffers.size() && (!maskNdxNotUnique || printCount < printFloodLimit); ++sampleNdxA)
			for (int sampleNdxB = sampleNdxA+1; sampleNdxB < (int)resultBuffers.size() && (!maskNdxNotUnique || printCount < printFloodLimit); ++sampleNdxB)
			{
				if (maskBitIndices[sampleNdxA] == maskBitIndices[sampleNdxB])
				{
					if (++printCount <= printFloodLimit)
					{
						m_testCtx.getLog()
							<< tcu::TestLog::Message
							<< "Pixel (" << x << ", " << y << "): Samples " << sampleNdxA << " and " << sampleNdxB << " have the same sample mask. (Single bit at index " << maskBitIndices[sampleNdxA] << ")"
							<< tcu::TestLog::EndMessage;
					}

					maskNdxNotUnique = true;
					uniquenessOk = false;
					errorMask.setPixel(x, y, tcu::RGBA::red());
				}
			}
		}

		// end result
		if (!uniquenessOk)
		{
			if (printCount > printFloodLimit)
				m_testCtx.getLog()
					<< tcu::TestLog::Message
					<< "...\n"
					<< "Omitted " << (printCount-printFloodLimit) << " error descriptions."
					<< tcu::TestLog::EndMessage;

			m_testCtx.getLog()
				<< tcu::TestLog::Message << "Image verification failed." << tcu::TestLog::EndMessage
				<< tcu::TestLog::ImageSet("Verification", "Image Verification")
				<< tcu::TestLog::Image("ErrorMask", "Error Mask", errorMask.getAccess())
				<< tcu::TestLog::EndImageSet;

			allOk = false;
		}
	}

	return allOk;
}

class SampleMaskUniqueSetCase : public SampleMaskBaseCase
{
public:
									SampleMaskUniqueSetCase		(Context& context, const char* name, const char* desc, int sampleCount, RenderTarget target, ShaderRunMode runMode);
									~SampleMaskUniqueSetCase	(void);

	void							init						(void);
	void							deinit						(void);

private:
	enum
	{
		RENDER_SIZE = 64
	};

	void							preDraw						(void);
	void							postDraw					(void);
	std::string						genFragmentSource			(int numTargetSamples) const;
	bool							verifySampleBuffers			(const std::vector<tcu::Surface>& resultBuffers);
	std::string						getIterationDescription		(int iteration) const;

	void							preTest						(void);
	void							postTest					(void);

	std::vector<tcu::Surface>		m_iterationSampleBuffers;
};

SampleMaskUniqueSetCase::SampleMaskUniqueSetCase (Context& context, const char* name, const char* desc, int sampleCount, RenderTarget target, ShaderRunMode runMode)
	: SampleMaskBaseCase(context, name, desc, sampleCount, target, RENDER_SIZE, runMode, MultisampleShaderRenderUtil::MultisampleRenderCase::FLAG_VERIFY_MSAA_TEXTURE_SAMPLE_BUFFERS)
{
	DE_ASSERT(runMode == RUN_PER_TWO_SAMPLES);
	DE_ASSERT(target == TARGET_TEXTURE);

	// high and low bits
	m_numIterations = 2;
}

SampleMaskUniqueSetCase::~SampleMaskUniqueSetCase (void)
{
}

void SampleMaskUniqueSetCase::init (void)
{
	// log the test method and expectations
	m_testCtx.getLog()
		<< tcu::TestLog::Message
		<< "Verifying gl_SampleMaskIn.\n"
		<< "	Fragment shader may be invoked [ceil(numSamples/2), numSamples] times.\n"
		<< "	=> Each invocation should have unique bit set\n"
		<< "	Writing highest and lowest bit index to color channels in render shader. Verifying:\n"
		<< "		1) no other invocation contains these bits in sampler shader.\n"
		<< "		2) number of invocations is at least ceil(numSamples/2).\n"
		<< tcu::TestLog::EndMessage;

	SampleMaskBaseCase::init();
}

void SampleMaskUniqueSetCase::deinit (void)
{
	m_iterationSampleBuffers.clear();
}

void SampleMaskUniqueSetCase::preDraw (void)
{
	const glw::Functions&	gl			= m_context.getRenderContext().getFunctions();
	const int				selectorLoc	= gl.getUniformLocation(m_program->getProgram(), "u_bitSelector");

	gl.uniform1ui(selectorLoc, (deUint32)m_iteration);
	GLU_EXPECT_NO_ERROR(gl.getError(), "set u_bitSelector");

	m_testCtx.getLog() << tcu::TestLog::Message << "Setting u_bitSelector = " << m_iteration << tcu::TestLog::EndMessage;

	SampleMaskBaseCase::preDraw();
}

void SampleMaskUniqueSetCase::postDraw (void)
{
	SampleMaskBaseCase::postDraw();
}

std::string SampleMaskUniqueSetCase::genFragmentSource (int numTargetSamples) const
{
	DE_ASSERT(numTargetSamples != 0);

	std::ostringstream	buf;
	const bool			supportsES32	= glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::es(3, 2));
	map<string, string>	args;
	args["GLSL_VERSION_DECL"]	= supportsES32 ? getGLSLVersionDeclaration(glu::GLSL_VERSION_320_ES) : getGLSLVersionDeclaration(glu::GLSL_VERSION_310_ES);
	args["GLSL_EXTENSION"]		= supportsES32 ? "" : "#extension GL_OES_sample_variables : require";

	// test supports only one sample mask word
	if (numTargetSamples > 32)
		TCU_THROW(NotSupportedError, "Sample count larger than 32 is not supported.");

	// output min and max sample id
	buf <<	"${GLSL_VERSION_DECL}\n"
			"${GLSL_EXTENSION}\n"
			"uniform highp uint u_bitSelector;\n"
			"layout(location = 0) out mediump vec4 fragColor;\n"
			"void main (void)\n"
			"{\n"
			"	highp int selectedBits;\n"
			"	if (u_bitSelector == 0u)\n"
			"		selectedBits = (gl_SampleMaskIn[0] & 0xFFFF);\n"
			"	else\n"
			"		selectedBits = ((gl_SampleMaskIn[0] >> 16) & 0xFFFF);\n"
			"\n"
			"	// encode bits to color\n"
			"	highp int redBits = selectedBits & 31;\n"
			"	highp int greenBits = (selectedBits >> 5) & 63;\n"
			"	highp int blueBits = (selectedBits >> 11) & 31;\n"
			"\n"
			"	fragColor = vec4(float(redBits) / float(31), float(greenBits) / float(63), float(blueBits) / float(31), 1.0);\n"
			"}\n";

	return tcu::StringTemplate(buf.str()).specialize(args);
}

bool SampleMaskUniqueSetCase::verifySampleBuffers (const std::vector<tcu::Surface>& resultBuffers)
{
	// we need results from all passes to do verification. Store results and verify later (at postTest).

	DE_ASSERT(m_numTargetSamples == (int)resultBuffers.size());
	for (int ndx = 0; ndx < m_numTargetSamples; ++ndx)
		m_iterationSampleBuffers[m_iteration * m_numTargetSamples + ndx] = resultBuffers[ndx];

	return true;
}

std::string SampleMaskUniqueSetCase::getIterationDescription (int iteration) const
{
	if (iteration == 0)
		return "Reading low bits";
	else if (iteration == 1)
		return "Reading high bits";
	else
		DE_ASSERT(false);
	return "";
}

void SampleMaskUniqueSetCase::preTest (void)
{
	m_iterationSampleBuffers.resize(m_numTargetSamples * 2);
}

void SampleMaskUniqueSetCase::postTest (void)
{
	DE_ASSERT((m_iterationSampleBuffers.size() % 2) == 0);
	DE_ASSERT((int)m_iterationSampleBuffers.size() / 2 == m_numTargetSamples);

	const int						width			= m_iterationSampleBuffers[0].getWidth();
	const int						height			= m_iterationSampleBuffers[0].getHeight();
	bool							allOk			= true;
	std::vector<tcu::TextureLevel>	sampleCoverage	(m_numTargetSamples);
	const tcu::ScopedLogSection		section			(m_testCtx.getLog(), "Verify", "Verify masks");

	// convert color layers to 32 bit coverage masks, 2 passes per coverage

	for (int sampleNdx = 0; sampleNdx < (int)sampleCoverage.size(); ++sampleNdx)
	{
		sampleCoverage[sampleNdx].setStorage(tcu::TextureFormat(tcu::TextureFormat::R, tcu::TextureFormat::UNSIGNED_INT32), width, height);

		for (int y = 0; y < height; ++y)
		for (int x = 0; x < width; ++x)
		{
			const tcu::RGBA		lowColor	= m_iterationSampleBuffers[sampleNdx].getPixel(x, y);
			const tcu::RGBA		highColor	= m_iterationSampleBuffers[sampleNdx + (int)sampleCoverage.size()].getPixel(x, y);
			deUint16			low;
			deUint16			high;

			{
				int redBits		= (int)deFloatRound((float)lowColor.getRed() / 255.0f * 31);
				int greenBits	= (int)deFloatRound((float)lowColor.getGreen() / 255.0f * 63);
				int blueBits	= (int)deFloatRound((float)lowColor.getBlue() / 255.0f * 31);

				low = (deUint16)(redBits | (greenBits << 5) | (blueBits << 11));
			}
			{
				int redBits		= (int)deFloatRound((float)highColor.getRed() / 255.0f * 31);
				int greenBits	= (int)deFloatRound((float)highColor.getGreen() / 255.0f * 63);
				int blueBits	= (int)deFloatRound((float)highColor.getBlue() / 255.0f * 31);

				high = (deUint16)(redBits | (greenBits << 5) | (blueBits << 11));
			}

			sampleCoverage[sampleNdx].getAccess().setPixel(tcu::UVec4((((deUint32)high) << 16) | low, 0, 0, 0), x, y);
		}
	}

	// verify masks

	if (m_numRequestedSamples == 0)
	{
		// single sample target, expect mask = 0x01
		const int	printFloodLimit	= 5;
		int			printCount		= 0;

		m_testCtx.getLog() << tcu::TestLog::Message << "Verifying sample mask is 0x00000001." << tcu::TestLog::EndMessage;

		for (int y = 0; y < height; ++y)
		for (int x = 0; x < width; ++x)
		{
			deUint32 mask = sampleCoverage[0].getAccess().getPixelUint(x, y).x();
			if (mask != 0x01)
			{
				allOk = false;

				if (++printCount <= printFloodLimit)
				{
					m_testCtx.getLog()
						<< tcu::TestLog::Message
						<< "Pixel (" << x << ", " << y << "): Invalid mask, got " << tcu::Format::Hex<8>(mask) << ", expected " << tcu::Format::Hex<8>(0x01) << "\n"
						<< tcu::TestLog::EndMessage;
				}
			}
		}

		if (!allOk && printCount > printFloodLimit)
		{
			m_testCtx.getLog()
				<< tcu::TestLog::Message
				<< "...\n"
				<< "Omitted " << (printCount-printFloodLimit) << " error descriptions."
				<< tcu::TestLog::EndMessage;
		}
	}
	else
	{
		// check uniqueness
		{
			bool		uniquenessOk	= true;
			int			printCount		= 0;
			const int	printFloodLimit	= 5;

			m_testCtx.getLog() << tcu::TestLog::Message << "Verifying invocation sample masks do not share bits." << tcu::TestLog::EndMessage;

			for (int y = 0; y < height; ++y)
			for (int x = 0; x < width; ++x)
			{
				bool maskBitsNotUnique = false;

				for (int sampleNdxA = 0;            sampleNdxA < m_numTargetSamples && (!maskBitsNotUnique || printCount < printFloodLimit); ++sampleNdxA)
				for (int sampleNdxB = sampleNdxA+1; sampleNdxB < m_numTargetSamples && (!maskBitsNotUnique || printCount < printFloodLimit); ++sampleNdxB)
				{
					const deUint32 maskA = sampleCoverage[sampleNdxA].getAccess().getPixelUint(x, y).x();
					const deUint32 maskB = sampleCoverage[sampleNdxB].getAccess().getPixelUint(x, y).x();

					// equal mask == emitted by the same invocation
					if (maskA != maskB)
					{
						// shares samples?
						if (maskA & maskB)
						{
							maskBitsNotUnique = true;
							uniquenessOk = false;

							if (++printCount <= printFloodLimit)
							{
								m_testCtx.getLog()
									<< tcu::TestLog::Message
									<< "Pixel (" << x << ", " << y << "):\n"
									<< "\tSamples " << sampleNdxA << " and " << sampleNdxB << " share mask bits\n"
									<< "\tMask" << sampleNdxA << " = " << tcu::Format::Hex<8>(maskA) << "\n"
									<< "\tMask" << sampleNdxB << " = " << tcu::Format::Hex<8>(maskB) << "\n"
									<< tcu::TestLog::EndMessage;
							}
						}
					}
				}
			}

			if (!uniquenessOk)
			{
				allOk = false;

				if (printCount > printFloodLimit)
					m_testCtx.getLog()
						<< tcu::TestLog::Message
						<< "...\n"
						<< "Omitted " << (printCount-printFloodLimit) << " error descriptions."
						<< tcu::TestLog::EndMessage;
			}
		}

		// check number of sample mask bit groups is valid ( == number of invocations )
		{
			const deUint32			minNumInvocations	= (deUint32)de::max(1, (m_numTargetSamples+1)/2);
			bool					countOk				= true;
			int						printCount			= 0;
			const int				printFloodLimit		= 5;

			m_testCtx.getLog() << tcu::TestLog::Message << "Verifying cardinality of separate sample mask bit sets. Expecting equal to the number of invocations, (greater or equal to " << minNumInvocations << ")" << tcu::TestLog::EndMessage;

			for (int y = 0; y < height; ++y)
			for (int x = 0; x < width; ++x)
			{
				std::set<deUint32> masks;

				for (int maskNdx = 0; maskNdx < m_numTargetSamples; ++maskNdx)
				{
					const deUint32 mask = sampleCoverage[maskNdx].getAccess().getPixelUint(x, y).x();
					masks.insert(mask);
				}

				if ((int)masks.size() < (int)minNumInvocations)
				{
					if (++printCount <= printFloodLimit)
					{
						m_testCtx.getLog()
							<< tcu::TestLog::Message
							<< "Pixel (" << x << ", " << y << "): Pixel invocations had only " << (int)masks.size() << " separate mask sets. Expected " << minNumInvocations << " or more. Found masks:"
							<< tcu::TestLog::EndMessage;

						for (std::set<deUint32>::iterator it = masks.begin(); it != masks.end(); ++it)
							m_testCtx.getLog()
							<< tcu::TestLog::Message
							<< "\tMask: " << tcu::Format::Hex<8>(*it) << "\n"
							<< tcu::TestLog::EndMessage;
					}

					countOk = false;
				}
			}

			if (!countOk)
			{
				allOk = false;

				if (printCount > printFloodLimit)
					m_testCtx.getLog()
						<< tcu::TestLog::Message
						<< "...\n"
						<< "Omitted " << (printCount-printFloodLimit) << " error descriptions."
						<< tcu::TestLog::EndMessage;
			}
		}
	}

	if (!allOk)
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Image verification failed");
}

class SampleMaskWriteCase : public SampleMaskBaseCase
{
public:
	enum TestMode
	{
		TEST_DISCARD = 0,
		TEST_INVERSE,

		TEST_LAST
	};
						SampleMaskWriteCase			(Context& context, const char* name, const char* desc, int sampleCount, RenderTarget target, ShaderRunMode runMode, TestMode testMode);
						~SampleMaskWriteCase		(void);

	void				init						(void);
	void				preDraw						(void);
	void				postDraw					(void);

private:
	enum
	{
		RENDER_SIZE = 64
	};

	std::string			genFragmentSource			(int numTargetSamples) const;
	bool				verifyImage					(const tcu::Surface& resultImage);

	const TestMode		m_testMode;
};

SampleMaskWriteCase::SampleMaskWriteCase (Context& context, const char* name, const char* desc, int sampleCount, RenderTarget target, ShaderRunMode runMode, TestMode testMode)
	: SampleMaskBaseCase	(context, name, desc, sampleCount, target, RENDER_SIZE, runMode)
	, m_testMode			(testMode)
{
	DE_ASSERT(testMode < TEST_LAST);
}

SampleMaskWriteCase::~SampleMaskWriteCase (void)
{
}

void SampleMaskWriteCase::init (void)
{
	// log the test method and expectations
	if (m_testMode == TEST_DISCARD)
		m_testCtx.getLog()
			<< tcu::TestLog::Message
			<< "Discarding half of the samples using gl_SampleMask, expecting:\n"
			<< "	1) half intensity on multisample targets (numSamples > 1)\n"
			<< "	2) full discard on multisample targets (numSamples == 1)\n"
			<< "	3) full intensity (no discard) on singlesample targets. (Mask is only applied as a multisample operation.)\n"
			<< tcu::TestLog::EndMessage;
	else if (m_testMode == TEST_INVERSE)
		m_testCtx.getLog()
			<< tcu::TestLog::Message
			<< "Discarding half of the samples using GL_SAMPLE_MASK, setting inverse mask in fragment shader using gl_SampleMask, expecting:\n"
			<< "	1) full discard on multisample targets (mask & modifiedCoverge == 0)\n"
			<< "	2) full intensity (no discard) on singlesample targets. (Mask and coverage is only applied as a multisample operation.)\n"
			<< tcu::TestLog::EndMessage;
	else
		DE_ASSERT(false);

	SampleMaskBaseCase::init();
}

void SampleMaskWriteCase::preDraw (void)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (m_testMode == TEST_INVERSE)
	{
		// set mask to 0xAAAA.., set inverse mask bit coverage in shader

		const int		maskLoc	= gl.getUniformLocation(m_program->getProgram(), "u_mask");
		const deUint32	mask	= (deUint32)0xAAAAAAAAUL;

		if (maskLoc == -1)
			throw tcu::TestError("Location of u_mask was -1");

		gl.enable(GL_SAMPLE_MASK);
		gl.sampleMaski(0, mask);
		GLU_EXPECT_NO_ERROR(gl.getError(), "set mask");

		gl.uniform1ui(maskLoc, mask);
		GLU_EXPECT_NO_ERROR(gl.getError(), "set mask uniform");

		m_testCtx.getLog() << tcu::TestLog::Message << "Setting sample mask " << tcu::Format::Hex<4>(mask) << tcu::TestLog::EndMessage;
	}

	SampleMaskBaseCase::preDraw();
}

void SampleMaskWriteCase::postDraw (void)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (m_testMode == TEST_INVERSE)
	{
		const deUint32 fullMask	= (1U << m_numTargetSamples) - 1;

		gl.disable(GL_SAMPLE_MASK);
		gl.sampleMaski(0, fullMask);
		GLU_EXPECT_NO_ERROR(gl.getError(), "set mask");
	}

	SampleMaskBaseCase::postDraw();
}

std::string SampleMaskWriteCase::genFragmentSource (int numTargetSamples) const
{
	DE_ASSERT(numTargetSamples != 0);
	DE_UNREF(numTargetSamples);

	std::ostringstream	buf;
	const bool			supportsES32	= glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::es(3, 2));
	map<string, string>	args;
	args["GLSL_VERSION_DECL"]	= supportsES32 ? getGLSLVersionDeclaration(glu::GLSL_VERSION_320_ES) : getGLSLVersionDeclaration(glu::GLSL_VERSION_310_ES);
	args["GLSL_EXTENSION"]		= supportsES32 ? "" : "#extension GL_OES_sample_variables : require";

	if (m_testMode == TEST_DISCARD)
	{
		// mask out every other coverage bit

		buf <<	"${GLSL_VERSION_DECL}\n"
				"${GLSL_EXTENSION}\n"
				"layout(location = 0) out mediump vec4 fragColor;\n"
				"void main (void)\n"
				"{\n"
				"	for (int i = 0; i < gl_SampleMask.length(); ++i)\n"
				"		gl_SampleMask[i] = int(0xAAAAAAAA);\n"
				"\n";

		if (m_runMode == RUN_PER_SAMPLE)
			buf <<	"	// force per-sample shading\n"
					"	highp float blue = float(gl_SampleID);\n"
					"\n"
					"	fragColor = vec4(0.0, 1.0, blue, 1.0);\n"
					"}\n";
		else
			buf <<	"	fragColor = vec4(0.0, 1.0, 0.0, 1.0);\n"
					"}\n";
	}
	else if (m_testMode == TEST_INVERSE)
	{
		// inverse every coverage bit

		buf <<	"${GLSL_VERSION_DECL}\n"
				"${GLSL_EXTENSION}\n"
				"layout(location = 0) out mediump vec4 fragColor;\n"
				"uniform highp uint u_mask;\n"
				"void main (void)\n"
				"{\n"
				"	gl_SampleMask[0] = int(~u_mask);\n"
				"\n";

		if (m_runMode == RUN_PER_SAMPLE)
			buf <<	"	// force per-sample shading\n"
					"	highp float blue = float(gl_SampleID);\n"
					"\n"
					"	fragColor = vec4(0.0, 1.0, blue, 1.0);\n"
					"}\n";
		else
			buf <<	"	fragColor = vec4(0.0, 1.0, 0.0, 1.0);\n"
					"}\n";
	}
	else
		DE_ASSERT(false);

	return tcu::StringTemplate(buf.str()).specialize(args);
}

bool SampleMaskWriteCase::verifyImage (const tcu::Surface& resultImage)
{
	const bool singleSampleTarget = m_numRequestedSamples == 0 && !(m_renderTarget == TARGET_DEFAULT && m_context.getRenderTarget().getNumSamples() > 1);

	if (m_testMode == TEST_DISCARD)
	{
		if (singleSampleTarget)
		{
			// single sample case => multisample operations are not effective => don't discard anything
			// expect green
			return verifyImageWithVerifier(resultImage, m_testCtx.getLog(), ColorVerifier(tcu::Vec3(0.0f, 1.0f, 0.0f)));
		}
		else if (m_numTargetSamples == 1)
		{
			// total discard, expect black
			return verifyImageWithVerifier(resultImage, m_testCtx.getLog(), ColorVerifier(tcu::Vec3(0.0f, 0.0f, 0.0f)));
		}
		else
		{
			// partial discard, expect something between black and green
			return verifyImageWithVerifier(resultImage, m_testCtx.getLog(), PartialDiscardVerifier());
		}
	}
	else if (m_testMode == TEST_INVERSE)
	{
		if (singleSampleTarget)
		{
			// single sample case => multisample operations are not effective => don't discard anything
			// expect green
			return verifyImageWithVerifier(resultImage, m_testCtx.getLog(), ColorVerifier(tcu::Vec3(0.0f, 1.0f, 0.0f)));
		}
		else
		{
			// total discard, expect black
			return verifyImageWithVerifier(resultImage, m_testCtx.getLog(), ColorVerifier(tcu::Vec3(0.0f, 0.0f, 0.0f)));
		}
	}
	else
	{
		DE_ASSERT(false);
		return false;
	}
}

} // anonymous

SampleVariableTests::SampleVariableTests (Context& context)
	: TestCaseGroup(context, "sample_variables", "Test sample variables")
{
}

SampleVariableTests::~SampleVariableTests (void)
{
}

void SampleVariableTests::init (void)
{
	tcu::TestCaseGroup* const numSampleGroup	= new tcu::TestCaseGroup(m_testCtx,	"num_samples",		"Test NumSamples");
	tcu::TestCaseGroup* const maxSampleGroup	= new tcu::TestCaseGroup(m_testCtx,	"max_samples",		"Test MaxSamples");
	tcu::TestCaseGroup* const sampleIDGroup		= new tcu::TestCaseGroup(m_testCtx,	"sample_id",		"Test SampleID");
	tcu::TestCaseGroup* const samplePosGroup	= new tcu::TestCaseGroup(m_testCtx,	"sample_pos",		"Test SamplePosition");
	tcu::TestCaseGroup* const sampleMaskInGroup	= new tcu::TestCaseGroup(m_testCtx,	"sample_mask_in",	"Test SampleMaskIn");
	tcu::TestCaseGroup* const sampleMaskGroup	= new tcu::TestCaseGroup(m_testCtx,	"sample_mask",		"Test SampleMask");

	addChild(numSampleGroup);
	addChild(maxSampleGroup);
	addChild(sampleIDGroup);
	addChild(samplePosGroup);
	addChild(sampleMaskInGroup);
	addChild(sampleMaskGroup);

	static const struct RenderTarget
	{
		const char*							name;
		const char*							desc;
		int									numSamples;
		MultisampleRenderCase::RenderTarget	target;
	} targets[] =
	{
		{ "default_framebuffer",		"Test with default framebuffer",	0,	MultisampleRenderCase::TARGET_DEFAULT		},
		{ "singlesample_texture",		"Test with singlesample texture",	0,	MultisampleRenderCase::TARGET_TEXTURE		},
		{ "multisample_texture_1",		"Test with multisample texture",	1,	MultisampleRenderCase::TARGET_TEXTURE		},
		{ "multisample_texture_2",		"Test with multisample texture",	2,	MultisampleRenderCase::TARGET_TEXTURE		},
		{ "multisample_texture_4",		"Test with multisample texture",	4,	MultisampleRenderCase::TARGET_TEXTURE		},
		{ "multisample_texture_8",		"Test with multisample texture",	8,	MultisampleRenderCase::TARGET_TEXTURE		},
		{ "multisample_texture_16",		"Test with multisample texture",	16,	MultisampleRenderCase::TARGET_TEXTURE		},
		{ "singlesample_rbo",			"Test with singlesample rbo",		0,	MultisampleRenderCase::TARGET_RENDERBUFFER	},
		{ "multisample_rbo_1",			"Test with multisample rbo",		1,	MultisampleRenderCase::TARGET_RENDERBUFFER	},
		{ "multisample_rbo_2",			"Test with multisample rbo",		2,	MultisampleRenderCase::TARGET_RENDERBUFFER	},
		{ "multisample_rbo_4",			"Test with multisample rbo",		4,	MultisampleRenderCase::TARGET_RENDERBUFFER	},
		{ "multisample_rbo_8",			"Test with multisample rbo",		8,	MultisampleRenderCase::TARGET_RENDERBUFFER	},
		{ "multisample_rbo_16",			"Test with multisample rbo",		16,	MultisampleRenderCase::TARGET_RENDERBUFFER	},
	};

	// .num_samples
	{
		for (int targetNdx = 0; targetNdx < DE_LENGTH_OF_ARRAY(targets); ++targetNdx)
			numSampleGroup->addChild(new NumSamplesCase(m_context, targets[targetNdx].name, targets[targetNdx].desc, targets[targetNdx].numSamples, targets[targetNdx].target));
	}

	// .max_samples
	{
		for (int targetNdx = 0; targetNdx < DE_LENGTH_OF_ARRAY(targets); ++targetNdx)
			maxSampleGroup->addChild(new MaxSamplesCase(m_context, targets[targetNdx].name, targets[targetNdx].desc, targets[targetNdx].numSamples, targets[targetNdx].target));
	}

	// .sample_ID
	{
		for (int targetNdx = 0; targetNdx < DE_LENGTH_OF_ARRAY(targets); ++targetNdx)
			sampleIDGroup->addChild(new SampleIDCase(m_context, targets[targetNdx].name, targets[targetNdx].desc, targets[targetNdx].numSamples, targets[targetNdx].target));
	}

	// .sample_pos
	{
		{
			tcu::TestCaseGroup* const group = new tcu::TestCaseGroup(m_testCtx,	"correctness", "Test SamplePos correctness");
			samplePosGroup->addChild(group);

			for (int targetNdx = 0; targetNdx < DE_LENGTH_OF_ARRAY(targets); ++targetNdx)
				group->addChild(new SamplePosCorrectnessCase(m_context, targets[targetNdx].name, targets[targetNdx].desc, targets[targetNdx].numSamples, targets[targetNdx].target));
		}

		{
			tcu::TestCaseGroup* const group = new tcu::TestCaseGroup(m_testCtx,	"distribution", "Test SamplePos distribution");
			samplePosGroup->addChild(group);

			for (int targetNdx = 0; targetNdx < DE_LENGTH_OF_ARRAY(targets); ++targetNdx)
				group->addChild(new SamplePosDistributionCase(m_context, targets[targetNdx].name, targets[targetNdx].desc, targets[targetNdx].numSamples, targets[targetNdx].target));
		}
	}

	// .sample_mask_in
	{
		// .sample_mask
		{
			tcu::TestCaseGroup* const group = new tcu::TestCaseGroup(m_testCtx,	"sample_mask", "Test with GL_SAMPLE_MASK");
			sampleMaskInGroup->addChild(group);

			for (int targetNdx = 0; targetNdx < DE_LENGTH_OF_ARRAY(targets); ++targetNdx)
				group->addChild(new SampleMaskCase(m_context, targets[targetNdx].name, targets[targetNdx].desc, targets[targetNdx].numSamples, targets[targetNdx].target));
		}
		// .bit_count_per_pixel
		{
			tcu::TestCaseGroup* const group = new tcu::TestCaseGroup(m_testCtx,	"bit_count_per_pixel", "Test number of coverage bits");
			sampleMaskInGroup->addChild(group);

			for (int targetNdx = 0; targetNdx < DE_LENGTH_OF_ARRAY(targets); ++targetNdx)
				group->addChild(new SampleMaskCountCase(m_context, targets[targetNdx].name, targets[targetNdx].desc, targets[targetNdx].numSamples, targets[targetNdx].target, SampleMaskCountCase::RUN_PER_PIXEL));
		}
		// .bit_count_per_sample
		{
			tcu::TestCaseGroup* const group = new tcu::TestCaseGroup(m_testCtx,	"bit_count_per_sample", "Test number of coverage bits");
			sampleMaskInGroup->addChild(group);

			for (int targetNdx = 0; targetNdx < DE_LENGTH_OF_ARRAY(targets); ++targetNdx)
				group->addChild(new SampleMaskCountCase(m_context, targets[targetNdx].name, targets[targetNdx].desc, targets[targetNdx].numSamples, targets[targetNdx].target, SampleMaskCountCase::RUN_PER_SAMPLE));
		}
		// .bit_count_per_two_samples
		{
			tcu::TestCaseGroup* const group = new tcu::TestCaseGroup(m_testCtx,	"bit_count_per_two_samples", "Test number of coverage bits");
			sampleMaskInGroup->addChild(group);

			for (int targetNdx = 0; targetNdx < DE_LENGTH_OF_ARRAY(targets); ++targetNdx)
				group->addChild(new SampleMaskCountCase(m_context, targets[targetNdx].name, targets[targetNdx].desc, targets[targetNdx].numSamples, targets[targetNdx].target, SampleMaskCountCase::RUN_PER_TWO_SAMPLES));
		}
		// .bits_unique_per_sample
		{
			tcu::TestCaseGroup* const group = new tcu::TestCaseGroup(m_testCtx,	"bits_unique_per_sample", "Test coverage bits");
			sampleMaskInGroup->addChild(group);

			for (int targetNdx = 0; targetNdx < DE_LENGTH_OF_ARRAY(targets); ++targetNdx)
				if (targets[targetNdx].target == MultisampleRenderCase::TARGET_TEXTURE)
					group->addChild(new SampleMaskUniqueCase(m_context, targets[targetNdx].name, targets[targetNdx].desc, targets[targetNdx].numSamples, targets[targetNdx].target, SampleMaskUniqueCase::RUN_PER_SAMPLE));
		}
		// .bits_unique_per_two_samples
		{
			tcu::TestCaseGroup* const group = new tcu::TestCaseGroup(m_testCtx,	"bits_unique_per_two_samples", "Test coverage bits");
			sampleMaskInGroup->addChild(group);

			for (int targetNdx = 0; targetNdx < DE_LENGTH_OF_ARRAY(targets); ++targetNdx)
				if (targets[targetNdx].target == MultisampleRenderCase::TARGET_TEXTURE)
					group->addChild(new SampleMaskUniqueSetCase(m_context, targets[targetNdx].name, targets[targetNdx].desc, targets[targetNdx].numSamples, targets[targetNdx].target, SampleMaskUniqueCase::RUN_PER_TWO_SAMPLES));
		}
	}

	// .sample_mask
	{
		// .discard_half_per_pixel
		{
			tcu::TestCaseGroup* const group = new tcu::TestCaseGroup(m_testCtx,	"discard_half_per_pixel", "Test coverage bits");
			sampleMaskGroup->addChild(group);

			for (int targetNdx = 0; targetNdx < DE_LENGTH_OF_ARRAY(targets); ++targetNdx)
				group->addChild(new SampleMaskWriteCase(m_context, targets[targetNdx].name, targets[targetNdx].desc, targets[targetNdx].numSamples, targets[targetNdx].target, SampleMaskWriteCase::RUN_PER_PIXEL, SampleMaskWriteCase::TEST_DISCARD));
		}
		// .discard_half_per_sample
		{
			tcu::TestCaseGroup* const group = new tcu::TestCaseGroup(m_testCtx,	"discard_half_per_sample", "Test coverage bits");
			sampleMaskGroup->addChild(group);

			for (int targetNdx = 0; targetNdx < DE_LENGTH_OF_ARRAY(targets); ++targetNdx)
				group->addChild(new SampleMaskWriteCase(m_context, targets[targetNdx].name, targets[targetNdx].desc, targets[targetNdx].numSamples, targets[targetNdx].target, SampleMaskWriteCase::RUN_PER_SAMPLE, SampleMaskWriteCase::TEST_DISCARD));
		}
		// .discard_half_per_two_samples
		{
			tcu::TestCaseGroup* const group = new tcu::TestCaseGroup(m_testCtx,	"discard_half_per_two_samples", "Test coverage bits");
			sampleMaskGroup->addChild(group);

			for (int targetNdx = 0; targetNdx < DE_LENGTH_OF_ARRAY(targets); ++targetNdx)
				group->addChild(new SampleMaskWriteCase(m_context, targets[targetNdx].name, targets[targetNdx].desc, targets[targetNdx].numSamples, targets[targetNdx].target, SampleMaskWriteCase::RUN_PER_TWO_SAMPLES, SampleMaskWriteCase::TEST_DISCARD));
		}

		// .discard_half_per_two_samples
		{
			tcu::TestCaseGroup* const group = new tcu::TestCaseGroup(m_testCtx,	"inverse_per_pixel", "Test coverage bits");
			sampleMaskGroup->addChild(group);

			for (int targetNdx = 0; targetNdx < DE_LENGTH_OF_ARRAY(targets); ++targetNdx)
				group->addChild(new SampleMaskWriteCase(m_context, targets[targetNdx].name, targets[targetNdx].desc, targets[targetNdx].numSamples, targets[targetNdx].target, SampleMaskWriteCase::RUN_PER_PIXEL, SampleMaskWriteCase::TEST_INVERSE));
		}
		// .inverse_per_sample
		{
			tcu::TestCaseGroup* const group = new tcu::TestCaseGroup(m_testCtx,	"inverse_per_sample", "Test coverage bits");
			sampleMaskGroup->addChild(group);

			for (int targetNdx = 0; targetNdx < DE_LENGTH_OF_ARRAY(targets); ++targetNdx)
				group->addChild(new SampleMaskWriteCase(m_context, targets[targetNdx].name, targets[targetNdx].desc, targets[targetNdx].numSamples, targets[targetNdx].target, SampleMaskWriteCase::RUN_PER_SAMPLE, SampleMaskWriteCase::TEST_INVERSE));
		}
		// .inverse_per_two_samples
		{
			tcu::TestCaseGroup* const group = new tcu::TestCaseGroup(m_testCtx,	"inverse_per_two_samples", "Test coverage bits");
			sampleMaskGroup->addChild(group);

			for (int targetNdx = 0; targetNdx < DE_LENGTH_OF_ARRAY(targets); ++targetNdx)
				group->addChild(new SampleMaskWriteCase(m_context, targets[targetNdx].name, targets[targetNdx].desc, targets[targetNdx].numSamples, targets[targetNdx].target, SampleMaskWriteCase::RUN_PER_TWO_SAMPLES, SampleMaskWriteCase::TEST_INVERSE));
		}
	}
}

} // Functional
} // gles31
} // deqp
