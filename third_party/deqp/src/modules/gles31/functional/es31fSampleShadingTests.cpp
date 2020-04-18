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
 * \brief Sample shading tests
 *//*--------------------------------------------------------------------*/

#include "es31fSampleShadingTests.hpp"
#include "es31fMultisampleShaderRenderCase.hpp"
#include "tcuRenderTarget.hpp"
#include "tcuSurface.hpp"
#include "glsStateQueryUtil.hpp"
#include "gluCallLogWrapper.hpp"
#include "gluContextInfo.hpp"
#include "gluShaderProgram.hpp"
#include "gluRenderContext.hpp"
#include "gluPixelTransfer.hpp"
#include "glwFunctions.hpp"
#include "glwEnums.hpp"
#include "deStringUtil.hpp"
#include "deRandom.hpp"

#include <map>

namespace deqp
{
namespace gles31
{
namespace Functional
{
namespace
{

using namespace gls::StateQueryUtil;

class SampleShadingStateCase : public TestCase
{
public:
						SampleShadingStateCase	(Context& ctx, const char* name, const char* desc, QueryType);

	void				init					(void);
	IterateResult		iterate					(void);

private:
	const QueryType		m_verifier;
};

SampleShadingStateCase::SampleShadingStateCase (Context& ctx, const char* name, const char* desc, QueryType type)
	: TestCase		(ctx, name, desc)
	, m_verifier	(type)
{
}

void SampleShadingStateCase::init (void)
{
	if (!contextSupports(m_context.getRenderContext().getType(), glu::ApiType::es(3, 2)) && !m_context.getContextInfo().isExtensionSupported("GL_OES_sample_shading"))
		throw tcu::NotSupportedError("Test requires GL_OES_sample_shading extension or a context version 3.2 or higher.");
}

SampleShadingStateCase::IterateResult SampleShadingStateCase::iterate (void)
{
	glu::CallLogWrapper		gl		(m_context.getRenderContext().getFunctions(), m_testCtx.getLog());
	tcu::ResultCollector	result	(m_testCtx.getLog(), " // ERROR: ");
	gl.enableLogging(true);

	// initial
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Verifying initial value" << tcu::TestLog::EndMessage;
		verifyStateBoolean(result, gl, GL_SAMPLE_SHADING, false, m_verifier);
	}

	// true and false too
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Verifying random values" << tcu::TestLog::EndMessage;

		gl.glEnable(GL_SAMPLE_SHADING);
		verifyStateBoolean(result, gl, GL_SAMPLE_SHADING, true, m_verifier);

		gl.glDisable(GL_SAMPLE_SHADING);
		verifyStateBoolean(result, gl, GL_SAMPLE_SHADING, false, m_verifier);
	}

	result.setTestContextResult(m_testCtx);
	return STOP;
}

class MinSampleShadingValueCase : public TestCase
{
public:
						MinSampleShadingValueCase	(Context& ctx, const char* name, const char* desc, QueryType);

	void				init						(void);
	IterateResult		iterate						(void);

private:
	const QueryType		m_verifier;
};

MinSampleShadingValueCase::MinSampleShadingValueCase (Context& ctx, const char* name, const char* desc, QueryType type)
	: TestCase		(ctx, name, desc)
	, m_verifier	(type)
{
}

void MinSampleShadingValueCase::init (void)
{
	if (!contextSupports(m_context.getRenderContext().getType(), glu::ApiType::es(3, 2)) && !m_context.getContextInfo().isExtensionSupported("GL_OES_sample_shading"))
		throw tcu::NotSupportedError("Test requires GL_OES_sample_shading extension or a context version 3.2 or higher.");
}

MinSampleShadingValueCase::IterateResult MinSampleShadingValueCase::iterate (void)
{
	glu::CallLogWrapper		gl		(m_context.getRenderContext().getFunctions(), m_testCtx.getLog());
	tcu::ResultCollector	result	(m_testCtx.getLog(), " // ERROR: ");

	gl.enableLogging(true);

	// initial
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Verifying initial value" << tcu::TestLog::EndMessage;
		verifyStateFloat(result, gl, GL_MIN_SAMPLE_SHADING_VALUE, 0.0, m_verifier);
	}

	// special values
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Verifying special values" << tcu::TestLog::EndMessage;

		gl.glMinSampleShading(0.0f);
		verifyStateFloat(result, gl, GL_MIN_SAMPLE_SHADING_VALUE, 0.0, m_verifier);

		gl.glMinSampleShading(1.0f);
		verifyStateFloat(result, gl, GL_MIN_SAMPLE_SHADING_VALUE, 1.0, m_verifier);

		gl.glMinSampleShading(0.5f);
		verifyStateFloat(result, gl, GL_MIN_SAMPLE_SHADING_VALUE, 0.5, m_verifier);
	}

	// random values
	{
		const int	numRandomTests	= 10;
		de::Random	rnd				(0xde123);

		m_testCtx.getLog() << tcu::TestLog::Message << "Verifying random values" << tcu::TestLog::EndMessage;

		for (int randNdx = 0; randNdx < numRandomTests; ++randNdx)
		{
			const float value = rnd.getFloat();

			gl.glMinSampleShading(value);
			verifyStateFloat(result, gl, GL_MIN_SAMPLE_SHADING_VALUE, value, m_verifier);
		}
	}

	result.setTestContextResult(m_testCtx);
	return STOP;
}

class MinSampleShadingValueClampingCase : public TestCase
{
public:
						MinSampleShadingValueClampingCase	(Context& ctx, const char* name, const char* desc);

	void				init								(void);
	IterateResult		iterate								(void);
};

MinSampleShadingValueClampingCase::MinSampleShadingValueClampingCase (Context& ctx, const char* name, const char* desc)
	: TestCase(ctx, name, desc)
{
}

void MinSampleShadingValueClampingCase::init (void)
{
	if (!contextSupports(m_context.getRenderContext().getType(), glu::ApiType::es(3, 2)) && !m_context.getContextInfo().isExtensionSupported("GL_OES_sample_shading"))
		throw tcu::NotSupportedError("Test requires GL_OES_sample_shading extension or a context version 3.2 or higher.");
}

MinSampleShadingValueClampingCase::IterateResult MinSampleShadingValueClampingCase::iterate (void)
{
	glu::CallLogWrapper		gl		(m_context.getRenderContext().getFunctions(), m_testCtx.getLog());
	tcu::ResultCollector	result	(m_testCtx.getLog(), " // ERROR: ");
	gl.enableLogging(true);

	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

	// special values
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Verifying clamped values. Value is clamped when specified." << tcu::TestLog::EndMessage;

		gl.glMinSampleShading(-0.5f);
		verifyStateFloat(result, gl, GL_MIN_SAMPLE_SHADING_VALUE, 0.0, QUERY_FLOAT);

		gl.glMinSampleShading(-1.0f);
		verifyStateFloat(result, gl, GL_MIN_SAMPLE_SHADING_VALUE, 0.0, QUERY_FLOAT);

		gl.glMinSampleShading(-1.5f);
		verifyStateFloat(result, gl, GL_MIN_SAMPLE_SHADING_VALUE, 0.0, QUERY_FLOAT);

		gl.glMinSampleShading(1.5f);
		verifyStateFloat(result, gl, GL_MIN_SAMPLE_SHADING_VALUE, 1.0, QUERY_FLOAT);

		gl.glMinSampleShading(2.0f);
		verifyStateFloat(result, gl, GL_MIN_SAMPLE_SHADING_VALUE, 1.0, QUERY_FLOAT);

		gl.glMinSampleShading(2.5f);
		verifyStateFloat(result, gl, GL_MIN_SAMPLE_SHADING_VALUE, 1.0, QUERY_FLOAT);
	}

	result.setTestContextResult(m_testCtx);
	return STOP;
}

class SampleShadingRenderingCase : public MultisampleShaderRenderUtil::MultisampleRenderCase
{
public:
	enum TestType
	{
		TEST_DISCARD = 0,
		TEST_COLOR,

		TEST_LAST
	};
						SampleShadingRenderingCase	(Context& ctx, const char* name, const char* desc, RenderTarget target, int numSamples, TestType type);
						~SampleShadingRenderingCase	(void);

	void				init						(void);
private:
	void				setShadingValue				(int sampleCount);

	void				preDraw						(void);
	void				postDraw					(void);
	std::string			getIterationDescription		(int iteration) const;

	bool				verifyImage					(const tcu::Surface& resultImage);

	std::string			genFragmentSource			(int numSamples) const;

	enum
	{
		RENDER_SIZE = 128
	};

	const TestType		m_type;
};

SampleShadingRenderingCase::SampleShadingRenderingCase (Context& ctx, const char* name, const char* desc, RenderTarget target, int numSamples, TestType type)
	: MultisampleShaderRenderUtil::MultisampleRenderCase	(ctx, name, desc, numSamples, target, RENDER_SIZE)
	, m_type												(type)
{
	DE_ASSERT(type < TEST_LAST);
}

SampleShadingRenderingCase::~SampleShadingRenderingCase (void)
{
	deinit();
}

void SampleShadingRenderingCase::init (void)
{
	// requirements

	if (!contextSupports(m_context.getRenderContext().getType(), glu::ApiType::es(3, 2)) && !m_context.getContextInfo().isExtensionSupported("GL_OES_sample_shading"))
		throw tcu::NotSupportedError("Test requires GL_OES_sample_shading extension or a context version 3.2 or higher.");
	if (m_renderTarget == TARGET_DEFAULT && m_context.getRenderTarget().getNumSamples() <= 1)
		throw tcu::NotSupportedError("Multisampled default framebuffer required");

	// test purpose and expectations
	m_testCtx.getLog()
		<< tcu::TestLog::Message
		<< "Verifying that a varying is given at least N different values for different samples within a single pixel.\n"
		<< "	Render high-frequency function, map result to black/white. Modify N with glMinSampleShading().\n"
		<< "	=> Resulting image should contain N+1 shades of gray.\n"
		<< tcu::TestLog::EndMessage;

	// setup resources

	MultisampleShaderRenderUtil::MultisampleRenderCase::init();

	// set iterations

	m_numIterations = m_numTargetSamples + 1;
}

void SampleShadingRenderingCase::setShadingValue (int sampleCount)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	if (sampleCount == 0)
	{
		gl.disable(GL_SAMPLE_SHADING);
		gl.minSampleShading(1.0f);
		GLU_EXPECT_NO_ERROR(gl.getError(), "set ratio");
	}
	else
	{
		// Minimum number of samples is max(ceil(<mss> * <samples>),1). Decrease mss with epsilon to prevent
		// ceiling to a too large sample count.
		const float epsilon	= 0.25f / (float)m_numTargetSamples;
		const float ratio	= ((float)sampleCount / (float)m_numTargetSamples) - epsilon;

		gl.enable(GL_SAMPLE_SHADING);
		gl.minSampleShading(ratio);
		GLU_EXPECT_NO_ERROR(gl.getError(), "set ratio");

		m_testCtx.getLog()
			<< tcu::TestLog::Message
			<< "Setting MIN_SAMPLE_SHADING_VALUE = " << ratio << "\n"
			<< "Requested sample count: shadingValue * numSamples = " << ratio << " * " << m_numTargetSamples << " = " << (ratio * (float)m_numTargetSamples) << "\n"
			<< "Minimum sample count: ceil(shadingValue * numSamples) = ceil(" << (ratio * (float)m_numTargetSamples) << ") = " << sampleCount
			<< tcu::TestLog::EndMessage;

		// can't fail with reasonable values of numSamples
		DE_ASSERT(deFloatCeil(ratio * (float)m_numTargetSamples) == float(sampleCount));
	}
}

void SampleShadingRenderingCase::preDraw (void)
{
	setShadingValue(m_iteration);
}

void SampleShadingRenderingCase::postDraw (void)
{
	const glw::Functions& gl = m_context.getRenderContext().getFunctions();

	gl.disable(GL_SAMPLE_SHADING);
	gl.minSampleShading(1.0f);
}

std::string	SampleShadingRenderingCase::getIterationDescription (int iteration) const
{
	if (iteration == 0)
		return "Disabled SAMPLE_SHADING";
	else
		return "Samples per pixel: " + de::toString(iteration);
}

bool SampleShadingRenderingCase::verifyImage (const tcu::Surface& resultImage)
{
	const int				numShadesRequired	= (m_iteration == 0) ? (2) : (m_iteration + 1);
	const int				rareThreshold		= 100;
	int						rareCount			= 0;
	std::map<deUint32, int>	shadeFrequency;

	// we should now have n+1 different shades of white, n = num samples

	m_testCtx.getLog()
		<< tcu::TestLog::Image("ResultImage", "Result Image", resultImage.getAccess())
		<< tcu::TestLog::Message
		<< "Verifying image has (at least) " << numShadesRequired << " different shades.\n"
		<< "Excluding pixels with no full coverage (pixels on the shared edge of the triangle pair)."
		<< tcu::TestLog::EndMessage;

	for (int y = 0; y < RENDER_SIZE; ++y)
	for (int x = 0; x < RENDER_SIZE; ++x)
	{
		const tcu::RGBA	color	= resultImage.getPixel(x, y);
		const deUint32	packed	= ((deUint32)color.getRed()) + ((deUint32)color.getGreen() << 8) + ((deUint32)color.getGreen() << 16);

		// on the triangle edge, skip
		if (x == y)
			continue;

		if (shadeFrequency.find(packed) == shadeFrequency.end())
			shadeFrequency[packed] = 1;
		else
			shadeFrequency[packed] = shadeFrequency[packed] + 1;
	}

	for (std::map<deUint32, int>::const_iterator it = shadeFrequency.begin(); it != shadeFrequency.end(); ++it)
		if (it->second < rareThreshold)
			rareCount++;

	m_testCtx.getLog()
		<< tcu::TestLog::Message
		<< "Found " << (int)shadeFrequency.size() << " different shades.\n"
		<< "\tRare (less than " << rareThreshold << " pixels): " << rareCount << "\n"
		<< "\tCommon: " << (int)shadeFrequency.size() - rareCount << "\n"
		<< tcu::TestLog::EndMessage;

	if ((int)shadeFrequency.size() < numShadesRequired)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Image verification failed." << tcu::TestLog::EndMessage;
		return false;
	}
	return true;
}

std::string SampleShadingRenderingCase::genFragmentSource (int numSamples) const
{
	DE_UNREF(numSamples);
	const glu::GLSLVersion	version	= contextSupports(m_context.getRenderContext().getType(), glu::ApiType::es(3, 2))
									? glu::GLSL_VERSION_320_ES
									: glu::GLSL_VERSION_310_ES;
	std::ostringstream		buf;

	buf <<	glu::getGLSLVersionDeclaration(version) << "\n"
			"in highp vec4 v_position;\n"
			"layout(location = 0) out mediump vec4 fragColor;\n"
			"void main (void)\n"
			"{\n"
			"	highp float field = dot(v_position.xy, v_position.xy) + dot(21.0 * v_position.xx, sin(3.1 * v_position.xy));\n"
			"	fragColor = vec4(1.0, 1.0, 1.0, 1.0);\n"
			"\n"
			"	if (fract(field) > 0.5)\n";

	if (m_type == TEST_DISCARD)
		buf <<	"		discard;\n";
	else if (m_type == TEST_COLOR)
		buf <<	"		fragColor = vec4(0.0, 0.0, 0.0, 1.0);\n";
	else
		DE_ASSERT(false);

	buf <<	"}";

	return buf.str();
}

} // anonymous

SampleShadingTests::SampleShadingTests (Context& context)
	: TestCaseGroup(context, "sample_shading", "Test sample shading")
{
}

SampleShadingTests::~SampleShadingTests (void)
{
}

void SampleShadingTests::init (void)
{
	tcu::TestCaseGroup* const stateQueryGroup = new tcu::TestCaseGroup(m_testCtx, "state_query", "State query tests.");
	tcu::TestCaseGroup* const minSamplesGroup = new tcu::TestCaseGroup(m_testCtx, "min_sample_shading", "Min sample shading tests.");

	addChild(stateQueryGroup);
	addChild(minSamplesGroup);

	// .state query
	{
		stateQueryGroup->addChild(new SampleShadingStateCase			(m_context, "sample_shading_is_enabled",				"test SAMPLE_SHADING",						QUERY_ISENABLED));
		stateQueryGroup->addChild(new SampleShadingStateCase			(m_context, "sample_shading_get_boolean",				"test SAMPLE_SHADING",						QUERY_BOOLEAN));
		stateQueryGroup->addChild(new SampleShadingStateCase			(m_context, "sample_shading_get_integer",				"test SAMPLE_SHADING",						QUERY_INTEGER));
		stateQueryGroup->addChild(new SampleShadingStateCase			(m_context, "sample_shading_get_float",					"test SAMPLE_SHADING",						QUERY_FLOAT));
		stateQueryGroup->addChild(new SampleShadingStateCase			(m_context, "sample_shading_get_integer64",				"test SAMPLE_SHADING",						QUERY_INTEGER64));
		stateQueryGroup->addChild(new MinSampleShadingValueCase			(m_context, "min_sample_shading_value_get_boolean",		"test MIN_SAMPLE_SHADING_VALUE",			QUERY_BOOLEAN));
		stateQueryGroup->addChild(new MinSampleShadingValueCase			(m_context, "min_sample_shading_value_get_integer",		"test MIN_SAMPLE_SHADING_VALUE",			QUERY_INTEGER));
		stateQueryGroup->addChild(new MinSampleShadingValueCase			(m_context, "min_sample_shading_value_get_float",		"test MIN_SAMPLE_SHADING_VALUE",			QUERY_FLOAT));
		stateQueryGroup->addChild(new MinSampleShadingValueCase			(m_context, "min_sample_shading_value_get_integer64",	"test MIN_SAMPLE_SHADING_VALUE",			QUERY_INTEGER64));
		stateQueryGroup->addChild(new MinSampleShadingValueClampingCase	(m_context, "min_sample_shading_value_clamping",		"test MIN_SAMPLE_SHADING_VALUE clamping"));
	}

	// .min_sample_count
	{
		static const struct Target
		{
			SampleShadingRenderingCase::RenderTarget	target;
			int											numSamples;
			const char*									name;
		} targets[] =
		{
			{ SampleShadingRenderingCase::TARGET_DEFAULT,			0,	"default_framebuffer"					},
			{ SampleShadingRenderingCase::TARGET_TEXTURE,			2,	"multisample_texture_samples_2"			},
			{ SampleShadingRenderingCase::TARGET_TEXTURE,			4,	"multisample_texture_samples_4"			},
			{ SampleShadingRenderingCase::TARGET_TEXTURE,			8,	"multisample_texture_samples_8"			},
			{ SampleShadingRenderingCase::TARGET_TEXTURE,			16,	"multisample_texture_samples_16"		},
			{ SampleShadingRenderingCase::TARGET_RENDERBUFFER,		2,	"multisample_renderbuffer_samples_2"	},
			{ SampleShadingRenderingCase::TARGET_RENDERBUFFER,		4,	"multisample_renderbuffer_samples_4"	},
			{ SampleShadingRenderingCase::TARGET_RENDERBUFFER,		8,	"multisample_renderbuffer_samples_8"	},
			{ SampleShadingRenderingCase::TARGET_RENDERBUFFER,		16,	"multisample_renderbuffer_samples_16"	},
		};

		for (int ndx = 0; ndx < DE_LENGTH_OF_ARRAY(targets); ++ndx)
		{
			minSamplesGroup->addChild(new SampleShadingRenderingCase(m_context, (std::string(targets[ndx].name) + "_color").c_str(),	"Test multiple samples per pixel with color",	targets[ndx].target, targets[ndx].numSamples, SampleShadingRenderingCase::TEST_COLOR));
			minSamplesGroup->addChild(new SampleShadingRenderingCase(m_context, (std::string(targets[ndx].name) + "_discard").c_str(),	"Test multiple samples per pixel with",			targets[ndx].target, targets[ndx].numSamples, SampleShadingRenderingCase::TEST_DISCARD));
		}
	}
}

} // Functional
} // gles31
} // deqp
