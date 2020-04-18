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
 * \brief Multisample interpolation tests
 *//*--------------------------------------------------------------------*/

#include "es31fShaderMultisampleInterpolationTests.hpp"
#include "es31fMultisampleShaderRenderCase.hpp"
#include "tcuTestLog.hpp"
#include "tcuRGBA.hpp"
#include "tcuSurface.hpp"
#include "tcuStringTemplate.hpp"
#include "tcuRenderTarget.hpp"
#include "gluContextInfo.hpp"
#include "gluShaderProgram.hpp"
#include "gluRenderContext.hpp"
#include "glwFunctions.hpp"
#include "glwEnums.hpp"
#include "deArrayUtil.hpp"
#include "deStringUtil.hpp"
#include "deMath.h"

#include <map>

namespace deqp
{
namespace gles31
{
namespace Functional
{
namespace
{

static std::string specializeShader(const std::string& shaderSource, const glu::ContextType& contextType)
{
	const bool supportsES32 = glu::contextSupports(contextType, glu::ApiType::es(3, 2));

	std::map<std::string, std::string> args;
	args["GLSL_VERSION_DECL"]							= glu::getGLSLVersionDeclaration(glu::getContextTypeGLSLVersion(contextType));
	args["GLSL_EXT_SHADER_MULTISAMPLE_INTERPOLATION"]	= supportsES32 ? "" : "#extension GL_OES_shader_multisample_interpolation : require\n";
	args["GLSL_EXT_SAMPLE_VARIABLES"]					= supportsES32 ? "" : "#extension GL_OES_sample_variables : require\n";

	return tcu::StringTemplate(shaderSource).specialize(args);
}

static bool verifyGreenImage (const tcu::Surface& image, tcu::TestLog& log)
{
	bool error = false;

	log << tcu::TestLog::Message << "Verifying result image, expecting green." << tcu::TestLog::EndMessage;

	// all pixels must be green

	for (int y = 0; y < image.getHeight(); ++y)
	for (int x = 0; x < image.getWidth(); ++x)
	{
		const tcu::RGBA color			= image.getPixel(x, y);
		const int		greenThreshold	= 8;

		if (color.getRed() > 0 || color.getGreen() < 255-greenThreshold || color.getBlue() > 0)
			error = true;
	}

	if (error)
		log	<< tcu::TestLog::Image("ResultImage", "Result Image", image.getAccess())
			<< tcu::TestLog::Message
			<< "Image verification failed."
			<< tcu::TestLog::EndMessage;
	else
		log	<< tcu::TestLog::Image("ResultImage", "Result Image", image.getAccess())
			<< tcu::TestLog::Message
			<< "Image verification passed."
			<< tcu::TestLog::EndMessage;

	return !error;
}

class MultisampleShadeCountRenderCase : public MultisampleShaderRenderUtil::MultisampleRenderCase
{
public:
						MultisampleShadeCountRenderCase		(Context& context, const char* name, const char* description, int numSamples, RenderTarget target);
	virtual				~MultisampleShadeCountRenderCase	(void);

	void				init								(void);

private:
	enum
	{
		RENDER_SIZE = 128
	};

	virtual std::string	getIterationDescription				(int iteration) const;
	bool				verifyImage							(const tcu::Surface& resultImage);
};

MultisampleShadeCountRenderCase::MultisampleShadeCountRenderCase (Context& context, const char* name, const char* description, int numSamples, RenderTarget target)
	: MultisampleShaderRenderUtil::MultisampleRenderCase(context, name, description, numSamples, target, RENDER_SIZE, MultisampleShaderRenderUtil::MultisampleRenderCase::FLAG_PER_ITERATION_SHADER)
{
	m_numIterations = -1; // must be set by deriving class
}

MultisampleShadeCountRenderCase::~MultisampleShadeCountRenderCase (void)
{
}

void MultisampleShadeCountRenderCase::init (void)
{
	// requirements
	if (!m_context.getContextInfo().isExtensionSupported("GL_OES_shader_multisample_interpolation") && !glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::es(3, 2)))
		TCU_THROW(NotSupportedError, "Test requires GL_OES_shader_multisample_interpolation extension");

	MultisampleShaderRenderUtil::MultisampleRenderCase::init();
}

std::string	MultisampleShadeCountRenderCase::getIterationDescription (int iteration) const
{
	// must be overriden
	DE_UNREF(iteration);
	DE_ASSERT(false);
	return "";
}

bool MultisampleShadeCountRenderCase::verifyImage (const tcu::Surface& resultImage)
{
	const bool				isSingleSampleTarget	= (m_renderTarget != TARGET_DEFAULT && m_numRequestedSamples == 0) || (m_renderTarget == TARGET_DEFAULT && m_context.getRenderTarget().getNumSamples() <= 1);
	const int				numShadesRequired		= (isSingleSampleTarget) ? (2) : (m_numTargetSamples + 1);
	const int				rareThreshold			= 100;
	int						rareCount				= 0;
	std::map<deUint32, int>	shadeFrequency;

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

class SampleQualifierRenderCase : public MultisampleShadeCountRenderCase
{
public:
				SampleQualifierRenderCase	(Context& context, const char* name, const char* description, int numSamples, RenderTarget target);
				~SampleQualifierRenderCase	(void);

	void		init						(void);

private:
	std::string	genVertexSource				(int numTargetSamples) const;
	std::string	genFragmentSource			(int numTargetSamples) const;
	std::string	getIterationDescription		(int iteration) const;
};

SampleQualifierRenderCase::SampleQualifierRenderCase (Context& context, const char* name, const char* description, int numSamples, RenderTarget target)
	: MultisampleShadeCountRenderCase(context, name, description, numSamples, target)
{
	m_numIterations = 6; // float, vec2, .3, .4, array, struct
}

SampleQualifierRenderCase::~SampleQualifierRenderCase (void)
{
}

void SampleQualifierRenderCase::init (void)
{
	const bool isSingleSampleTarget = (m_renderTarget != TARGET_DEFAULT && m_numRequestedSamples == 0) || (m_renderTarget == TARGET_DEFAULT && m_context.getRenderTarget().getNumSamples() <= 1);

	// test purpose and expectations
	if (isSingleSampleTarget)
	{
		m_testCtx.getLog()
			<< tcu::TestLog::Message
			<< "Verifying that a sample-qualified varying is given different values for different samples.\n"
			<< "	Render high-frequency function, map result to black/white.\n"
			<< "	=> Resulting image image should contain both black and white pixels.\n"
			<< tcu::TestLog::EndMessage;
	}
	else
	{
		m_testCtx.getLog()
			<< tcu::TestLog::Message
			<< "Verifying that a sample-qualified varying is given different values for different samples.\n"
			<< "	Render high-frequency function, map result to black/white.\n"
			<< "	=> Resulting image should contain n+1 shades of gray, n = sample count.\n"
			<< tcu::TestLog::EndMessage;
	}

	MultisampleShadeCountRenderCase::init();
}

std::string	SampleQualifierRenderCase::genVertexSource (int numTargetSamples) const
{
	DE_UNREF(numTargetSamples);

	std::ostringstream buf;

	buf <<	"${GLSL_VERSION_DECL}\n"
			"${GLSL_EXT_SHADER_MULTISAMPLE_INTERPOLATION}"
			"in highp vec4 a_position;\n";

	if (m_iteration == 0)
		buf << "sample out highp float v_input;\n";
	else if (m_iteration == 1)
		buf << "sample out highp vec2 v_input;\n";
	else if (m_iteration == 2)
		buf << "sample out highp vec3 v_input;\n";
	else if (m_iteration == 3)
		buf << "sample out highp vec4 v_input;\n";
	else if (m_iteration == 4)
		buf << "sample out highp float[2] v_input;\n";
	else if (m_iteration == 5)
		buf << "struct VaryingStruct { highp float a; highp float b; };\n"
			   "sample out VaryingStruct v_input;\n";
	else
		DE_ASSERT(false);

	buf <<	"void main (void)\n"
			"{\n"
			"	gl_Position = a_position;\n";

	if (m_iteration == 0)
		buf << "	v_input = a_position.x + exp(a_position.y) + step(0.9, a_position.x)*step(a_position.y, -0.9)*8.0;\n";
	else if (m_iteration == 1)
		buf << "	v_input = a_position.xy;\n";
	else if (m_iteration == 2)
		buf << "	v_input = vec3(a_position.xy, a_position.x * 2.0 - a_position.y);\n";
	else if (m_iteration == 3)
		buf << "	v_input = vec4(a_position.xy, a_position.x * 2.0 - a_position.y, a_position.x*a_position.y);\n";
	else if (m_iteration == 4)
		buf << "	v_input[0] = a_position.x;\n"
			   "	v_input[1] = a_position.y;\n";
	else if (m_iteration == 5)
		buf << "	v_input.a = a_position.x;\n"
			   "	v_input.b = a_position.y;\n";
	else
		DE_ASSERT(false);

	buf <<	"}";

	return specializeShader(buf.str(), m_context.getRenderContext().getType());
}

std::string	SampleQualifierRenderCase::genFragmentSource (int numTargetSamples) const
{
	DE_UNREF(numTargetSamples);

	std::ostringstream buf;

	buf <<	"${GLSL_VERSION_DECL}\n"
			"${GLSL_EXT_SHADER_MULTISAMPLE_INTERPOLATION}";

	if (m_iteration == 0)
		buf << "sample in highp float v_input;\n";
	else if (m_iteration == 1)
		buf << "sample in highp vec2 v_input;\n";
	else if (m_iteration == 2)
		buf << "sample in highp vec3 v_input;\n";
	else if (m_iteration == 3)
		buf << "sample in highp vec4 v_input;\n";
	else if (m_iteration == 4)
		buf << "sample in highp float[2] v_input;\n";
	else if (m_iteration == 5)
		buf << "struct VaryingStruct { highp float a; highp float b; };\n"
			   "sample in VaryingStruct v_input;\n";
	else
		DE_ASSERT(false);

	buf <<	"layout(location = 0) out mediump vec4 fragColor;\n"
			"void main (void)\n"
			"{\n";

	if (m_iteration == 0)
		buf << "	highp float field = exp(v_input) + v_input*v_input;\n";
	else if (m_iteration == 1)
		buf << "	highp float field = dot(v_input.xy, v_input.xy) + dot(21.0 * v_input.xx, sin(3.1 * v_input.xy));\n";
	else if (m_iteration == 2)
		buf << "	highp float field = dot(v_input.xy, v_input.xy) + dot(21.0 * v_input.zx, sin(3.1 * v_input.zy));\n";
	else if (m_iteration == 3)
		buf << "	highp float field = dot(v_input.xy, v_input.zw) + dot(21.0 * v_input.zy, sin(3.1 * v_input.zw));\n";
	else if (m_iteration == 4)
		buf << "	highp float field = dot(vec2(v_input[0], v_input[1]), vec2(v_input[0], v_input[1])) + dot(21.0 * vec2(v_input[0]), sin(3.1 * vec2(v_input[0], v_input[1])));\n";
	else if (m_iteration == 5)
		buf << "	highp float field = dot(vec2(v_input.a, v_input.b), vec2(v_input.a, v_input.b)) + dot(21.0 * vec2(v_input.a), sin(3.1 * vec2(v_input.a, v_input.b)));\n";
	else
		DE_ASSERT(false);

	buf <<	"	fragColor = vec4(1.0, 1.0, 1.0, 1.0);\n"
			"\n"
			"	if (fract(field) > 0.5)\n"
			"		fragColor = vec4(0.0, 0.0, 0.0, 1.0);\n"
			"}";

	return specializeShader(buf.str(), m_context.getRenderContext().getType());
}

std::string	SampleQualifierRenderCase::getIterationDescription (int iteration) const
{
	if (iteration == 0)
		return "Test with float varying";
	else if (iteration == 1)
		return "Test with vec2 varying";
	else if (iteration == 2)
		return "Test with vec3 varying";
	else if (iteration == 3)
		return "Test with vec4 varying";
	else if (iteration == 4)
		return "Test with array varying";
	else if (iteration == 5)
		return "Test with struct varying";

	DE_ASSERT(false);
	return "";
}

class InterpolateAtSampleRenderCase : public MultisampleShadeCountRenderCase
{
public:
	enum IndexingMode
	{
		INDEXING_STATIC,
		INDEXING_DYNAMIC,

		INDEXING_LAST
	};
						InterpolateAtSampleRenderCase	(Context& context, const char* name, const char* description, int numSamples, RenderTarget target, IndexingMode mode);
						~InterpolateAtSampleRenderCase	(void);

	void				init							(void);
	void				preDraw							(void);

private:
	std::string			genVertexSource					(int numTargetSamples) const;
	std::string			genFragmentSource				(int numTargetSamples) const;
	std::string			getIterationDescription			(int iteration) const;

	const IndexingMode	m_indexMode;
};

InterpolateAtSampleRenderCase::InterpolateAtSampleRenderCase (Context& context, const char* name, const char* description, int numSamples, RenderTarget target, IndexingMode mode)
	: MultisampleShadeCountRenderCase	(context, name, description, numSamples, target)
	, m_indexMode						(mode)
{
	DE_ASSERT(mode < INDEXING_LAST);

	m_numIterations = 5; // float, vec2, .3, .4, array
}

InterpolateAtSampleRenderCase::~InterpolateAtSampleRenderCase (void)
{
}

void InterpolateAtSampleRenderCase::init (void)
{
	const bool isSingleSampleTarget = (m_renderTarget != TARGET_DEFAULT && m_numRequestedSamples == 0) || (m_renderTarget == TARGET_DEFAULT && m_context.getRenderTarget().getNumSamples() <= 1);

	// test purpose and expectations
	if (isSingleSampleTarget)
	{
		m_testCtx.getLog()
			<< tcu::TestLog::Message
			<< "Verifying that a interpolateAtSample returns different values for different samples.\n"
			<< "	Render high-frequency function, map result to black/white.\n"
			<< "	=> Resulting image image should contain both black and white pixels.\n"
			<< tcu::TestLog::EndMessage;
	}
	else
	{
		m_testCtx.getLog()
			<< tcu::TestLog::Message
			<< "Verifying that a interpolateAtSample returns different values for different samples.\n"
			<< "	Render high-frequency function, map result to black/white.\n"
			<< "	=> Resulting image should contain n+1 shades of gray, n = sample count.\n"
			<< tcu::TestLog::EndMessage;
	}

	MultisampleShadeCountRenderCase::init();
}

void InterpolateAtSampleRenderCase::preDraw (void)
{
	if (m_indexMode == INDEXING_DYNAMIC)
	{
		const deInt32			range		= m_numTargetSamples;
		const deInt32			offset		= 1;
		const glw::Functions&	gl			= m_context.getRenderContext().getFunctions();
		const deInt32			offsetLoc	= gl.getUniformLocation(m_program->getProgram(), "u_offset");
		const deInt32			rangeLoc	= gl.getUniformLocation(m_program->getProgram(), "u_range");

		if (offsetLoc == -1)
			throw tcu::TestError("Location of u_offset was -1");
		if (rangeLoc == -1)
			throw tcu::TestError("Location of u_range was -1");

		gl.uniform1i(offsetLoc, 0);
		gl.uniform1i(rangeLoc, m_numTargetSamples);
		GLU_EXPECT_NO_ERROR(gl.getError(), "set uniforms");

		m_testCtx.getLog()
			<< tcu::TestLog::Message
			<< "Set u_offset = " << offset << "\n"
			<< "Set u_range = " << range
			<< tcu::TestLog::EndMessage;
	}
}

std::string InterpolateAtSampleRenderCase::genVertexSource (int numTargetSamples) const
{
	DE_UNREF(numTargetSamples);

	std::ostringstream buf;

	buf <<	"${GLSL_VERSION_DECL}\n"
			"in highp vec4 a_position;\n";

	if (m_iteration == 0)
		buf << "out highp float v_input;\n";
	else if (m_iteration == 1)
		buf << "out highp vec2 v_input;\n";
	else if (m_iteration == 2)
		buf << "out highp vec3 v_input;\n";
	else if (m_iteration == 3)
		buf << "out highp vec4 v_input;\n";
	else if (m_iteration == 4)
		buf << "out highp vec2[2] v_input;\n";
	else
		DE_ASSERT(false);

	buf <<	"void main (void)\n"
			"{\n"
			"	gl_Position = a_position;\n";

	if (m_iteration == 0)
		buf << "	v_input = a_position.x + exp(a_position.y) + step(0.9, a_position.x)*step(a_position.y, -0.9)*8.0;\n";
	else if (m_iteration == 1)
		buf << "	v_input = a_position.xy;\n";
	else if (m_iteration == 2)
		buf << "	v_input = vec3(a_position.xy, a_position.x * 2.0 - a_position.y);\n";
	else if (m_iteration == 3)
		buf << "	v_input = vec4(a_position.xy, a_position.x * 2.0 - a_position.y, a_position.x*a_position.y);\n";
	else if (m_iteration == 4)
		buf << "	v_input[0] = a_position.yx + vec2(0.5, 0.5);\n"
			   "	v_input[1] = a_position.xy;\n";
	else
		DE_ASSERT(false);

	buf <<	"}";

	return specializeShader(buf.str(), m_context.getRenderContext().getType());
}

std::string InterpolateAtSampleRenderCase::genFragmentSource (int numTargetSamples) const
{
	std::ostringstream buf;

	buf <<	"${GLSL_VERSION_DECL}\n"
			"${GLSL_EXT_SHADER_MULTISAMPLE_INTERPOLATION}";

	if (m_iteration == 0)
		buf << "in highp float v_input;\n";
	else if (m_iteration == 1)
		buf << "in highp vec2 v_input;\n";
	else if (m_iteration == 2)
		buf << "in highp vec3 v_input;\n";
	else if (m_iteration == 3)
		buf << "in highp vec4 v_input;\n";
	else if (m_iteration == 4)
		buf << "in highp vec2[2] v_input;\n";
	else
		DE_ASSERT(false);

	buf << "layout(location = 0) out mediump vec4 fragColor;\n";

	if (m_indexMode == INDEXING_DYNAMIC)
		buf <<	"uniform highp int u_offset;\n"
				"uniform highp int u_range;\n";

	buf <<	"void main (void)\n"
			"{\n"
			"	mediump int coverage = 0;\n"
			"\n";

	if (m_indexMode == INDEXING_STATIC)
	{
		for (int ndx = 0; ndx < numTargetSamples; ++ndx)
		{
			if (m_iteration == 0)
				buf <<	"	highp float sampleInput" << ndx << " = interpolateAtSample(v_input, " << ndx << ");\n";
			else if (m_iteration == 1)
				buf <<	"	highp vec2 sampleInput" << ndx << " = interpolateAtSample(v_input, " << ndx << ");\n";
			else if (m_iteration == 2)
				buf <<	"	highp vec3 sampleInput" << ndx << " = interpolateAtSample(v_input, " << ndx << ");\n";
			else if (m_iteration == 3)
				buf <<	"	highp vec4 sampleInput" << ndx << " = interpolateAtSample(v_input, " << ndx << ");\n";
			else if (m_iteration == 4)
				buf <<	"	highp vec2 sampleInput" << ndx << " = interpolateAtSample(v_input[1], " << ndx << ");\n";
			else
				DE_ASSERT(false);
		}
		buf <<	"\n";

		for (int ndx = 0; ndx < numTargetSamples; ++ndx)
		{
			if (m_iteration == 0)
				buf << "	highp float field" << ndx << " = exp(sampleInput" << ndx << ") + sampleInput" << ndx << "*sampleInput" << ndx << ";\n";
			else if (m_iteration == 1 || m_iteration == 4)
				buf << "	highp float field" << ndx << " = dot(sampleInput" << ndx << ", sampleInput" << ndx << ") + dot(21.0 * sampleInput" << ndx << ".xx, sin(3.1 * sampleInput" << ndx << "));\n";
			else if (m_iteration == 2)
				buf << "	highp float field" << ndx << " = dot(sampleInput" << ndx << ".xy, sampleInput" << ndx << ".xy) + dot(21.0 * sampleInput" << ndx << ".zx, sin(3.1 * sampleInput" << ndx << ".zy));\n";
			else if (m_iteration == 3)
				buf << "	highp float field" << ndx << " = dot(sampleInput" << ndx << ".xy, sampleInput" << ndx << ".zw) + dot(21.0 * sampleInput" << ndx << ".zy, sin(3.1 * sampleInput" << ndx << ".zw));\n";
			else
				DE_ASSERT(false);
		}
		buf <<	"\n";

		for (int ndx = 0; ndx < numTargetSamples; ++ndx)
			buf <<	"	if (fract(field" << ndx << ") <= 0.5)\n"
					"		++coverage;\n";
	}
	else if (m_indexMode == INDEXING_DYNAMIC)
	{
		buf <<	"	for (int ndx = 0; ndx < " << numTargetSamples << "; ++ndx)\n"
				"	{\n";

		if (m_iteration == 0)
			buf <<	"		highp float sampleInput = interpolateAtSample(v_input, (u_offset + ndx) % u_range);\n";
		else if (m_iteration == 1)
			buf <<	"		highp vec2 sampleInput = interpolateAtSample(v_input, (u_offset + ndx) % u_range);\n";
		else if (m_iteration == 2)
			buf <<	"		highp vec3 sampleInput = interpolateAtSample(v_input, (u_offset + ndx) % u_range);\n";
		else if (m_iteration == 3)
			buf <<	"		highp vec4 sampleInput = interpolateAtSample(v_input, (u_offset + ndx) % u_range);\n";
		else if (m_iteration == 4)
			buf <<	"		highp vec2 sampleInput = interpolateAtSample(v_input[1], (u_offset + ndx) % u_range);\n";
		else
			DE_ASSERT(false);

		if (m_iteration == 0)
			buf << "		highp float field = exp(sampleInput) + sampleInput*sampleInput;\n";
		else if (m_iteration == 1 || m_iteration == 4)
			buf << "		highp float field = dot(sampleInput, sampleInput) + dot(21.0 * sampleInput.xx, sin(3.1 * sampleInput));\n";
		else if (m_iteration == 2)
			buf << "		highp float field = dot(sampleInput.xy, sampleInput.xy) + dot(21.0 * sampleInput.zx, sin(3.1 * sampleInput.zy));\n";
		else if (m_iteration == 3)
			buf << "		highp float field = dot(sampleInput.xy, sampleInput.zw) + dot(21.0 * sampleInput.zy, sin(3.1 * sampleInput.zw));\n";
		else
			DE_ASSERT(false);

		buf <<	"		if (fract(field) <= 0.5)\n"
				"			++coverage;\n"
				"	}\n";
	}

	buf <<	"	fragColor = vec4(vec3(float(coverage) / float(" << numTargetSamples << ")), 1.0);\n"
			"}";

	return specializeShader(buf.str(), m_context.getRenderContext().getType());
}

std::string InterpolateAtSampleRenderCase::getIterationDescription (int iteration) const
{
	if (iteration == 0)
		return "Test with float varying";
	else if (iteration < 4)
		return "Test with vec" + de::toString(iteration+1) + " varying";
	else if (iteration == 4)
		return "Test with array varying";

	DE_ASSERT(false);
	return "";
}

class SingleSampleInterpolateAtSampleCase : public MultisampleShaderRenderUtil::MultisampleRenderCase
{
public:
	enum SampleCase
	{
		SAMPLE_0 = 0,
		SAMPLE_N,

		SAMPLE_LAST
	};

						SingleSampleInterpolateAtSampleCase		(Context& context, const char* name, const char* description, int numSamples, RenderTarget target, SampleCase sampleCase);
	virtual				~SingleSampleInterpolateAtSampleCase	(void);

	void				init									(void);

private:
	enum
	{
		RENDER_SIZE = 32
	};

	std::string			genVertexSource							(int numTargetSamples) const;
	std::string			genFragmentSource						(int numTargetSamples) const;
	bool				verifyImage								(const tcu::Surface& resultImage);

	const SampleCase	m_sampleCase;
};

SingleSampleInterpolateAtSampleCase::SingleSampleInterpolateAtSampleCase (Context& context, const char* name, const char* description, int numSamples, RenderTarget target, SampleCase sampleCase)
	: MultisampleShaderRenderUtil::MultisampleRenderCase	(context, name, description, numSamples, target, RENDER_SIZE)
	, m_sampleCase											(sampleCase)
{
	DE_ASSERT(numSamples == 0);
	DE_ASSERT(sampleCase < SAMPLE_LAST);
}

SingleSampleInterpolateAtSampleCase::~SingleSampleInterpolateAtSampleCase (void)
{
}

void SingleSampleInterpolateAtSampleCase::init (void)
{
	// requirements
	if (!m_context.getContextInfo().isExtensionSupported("GL_OES_shader_multisample_interpolation") && !glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::es(3, 2)))
		TCU_THROW(NotSupportedError, "Test requires GL_OES_shader_multisample_interpolation extension");
	if (m_renderTarget == TARGET_DEFAULT && m_context.getRenderTarget().getNumSamples() > 1)
		TCU_THROW(NotSupportedError, "Non-multisample framebuffer required");

	// test purpose and expectations
	m_testCtx.getLog()
		<< tcu::TestLog::Message
		<< "Verifying that using interpolateAtSample with multisample buffers not available returns sample evaluated at the center of the pixel.\n"
		<< "	Interpolate varying containing screen space location.\n"
		<< "	=> fract(screen space location) should be (about) (0.5, 0.5)\n"
		<< tcu::TestLog::EndMessage;

	MultisampleShaderRenderUtil::MultisampleRenderCase::init();
}

std::string SingleSampleInterpolateAtSampleCase::genVertexSource (int numTargetSamples) const
{
	DE_UNREF(numTargetSamples);

	std::ostringstream buf;

	buf <<	"${GLSL_VERSION_DECL}\n"
			"in highp vec4 a_position;\n"
			"out highp vec2 v_position;\n"
			"void main (void)\n"
			"{\n"
			"	gl_Position = a_position;\n"
			"	v_position = (a_position.xy + vec2(1.0, 1.0)) / 2.0 * vec2(" << (int)RENDER_SIZE << ".0, " << (int)RENDER_SIZE << ".0);\n"
			"}\n";

	return specializeShader(buf.str(), m_context.getRenderContext().getType());
}

std::string SingleSampleInterpolateAtSampleCase::genFragmentSource (int numTargetSamples) const
{
	DE_UNREF(numTargetSamples);

	std::ostringstream buf;

	buf <<	"${GLSL_VERSION_DECL}\n"
			"${GLSL_EXT_SHADER_MULTISAMPLE_INTERPOLATION}"
			"in highp vec2 v_position;\n"
			"layout(location = 0) out mediump vec4 fragColor;\n"
			"void main (void)\n"
			"{\n"
			"	const highp float threshold = 0.15625; // 4 subpixel bits. Assume 3 accurate bits + 0.03125 for other errors\n"; // 0.03125 = mediump epsilon when value = 32 (RENDER_SIZE)

	if (m_sampleCase == SAMPLE_0)
	{
		buf <<	"	highp vec2 samplePosition = interpolateAtSample(v_position, 0);\n"
				"	highp vec2 positionInsideAPixel = fract(samplePosition);\n"
				"\n"
				"	if (abs(positionInsideAPixel.x - 0.5) <= threshold && abs(positionInsideAPixel.y - 0.5) <= threshold)\n"
				"		fragColor = vec4(0.0, 1.0, 0.0, 1.0);\n"
				"	else\n"
				"		fragColor = vec4(1.0, 0.0, 0.0, 1.0);\n"
				"}\n";
	}
	else if (m_sampleCase == SAMPLE_N)
	{
		buf <<	"	bool allOk = true;\n"
				"	for (int sampleNdx = 159; sampleNdx < 163; ++sampleNdx)\n"
				"	{\n"
				"		highp vec2 samplePosition = interpolateAtSample(v_position, sampleNdx);\n"
				"		highp vec2 positionInsideAPixel = fract(samplePosition);\n"
				"		if (abs(positionInsideAPixel.x - 0.5) > threshold || abs(positionInsideAPixel.y - 0.5) > threshold)\n"
				"			allOk = false;\n"
				"	}\n"
				"\n"
				"	if (allOk)\n"
				"		fragColor = vec4(0.0, 1.0, 0.0, 1.0);\n"
				"	else\n"
				"		fragColor = vec4(1.0, 0.0, 0.0, 1.0);\n"
				"}\n";
	}
	else
		DE_ASSERT(false);

	return specializeShader(buf.str(), m_context.getRenderContext().getType());
}

bool SingleSampleInterpolateAtSampleCase::verifyImage (const tcu::Surface& resultImage)
{
	return verifyGreenImage(resultImage, m_testCtx.getLog());
}

class CentroidRenderCase : public MultisampleShaderRenderUtil::MultisampleRenderCase
{
public:
									CentroidRenderCase	(Context& context, const char* name, const char* description, int numSamples, RenderTarget target, int renderSize);
	virtual							~CentroidRenderCase	(void);

	void							init				(void);

private:
	void							setupRenderData		(void);
};

CentroidRenderCase::CentroidRenderCase (Context& context, const char* name, const char* description, int numSamples, RenderTarget target, int renderSize)
	: MultisampleShaderRenderUtil::MultisampleRenderCase(context, name, description, numSamples, target, renderSize)
{
}

CentroidRenderCase::~CentroidRenderCase (void)
{
}

void CentroidRenderCase::init (void)
{
	// requirements
	if (!m_context.getContextInfo().isExtensionSupported("GL_OES_shader_multisample_interpolation") && !glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::es(3, 2)))
		TCU_THROW(NotSupportedError, "Test requires GL_OES_shader_multisample_interpolation extension");

	MultisampleShaderRenderUtil::MultisampleRenderCase::init();
}

void CentroidRenderCase::setupRenderData (void)
{
	const int				numTriangles	= 200;
	const glw::Functions&	gl				= m_context.getRenderContext().getFunctions();
	std::vector<tcu::Vec4>	data			(numTriangles * 3 * 3);

	m_renderMode = GL_TRIANGLES;
	m_renderCount = numTriangles * 3;
	m_renderSceneDescription = "triangle fan of narrow triangles";

	m_renderAttribs["a_position"].offset = 0;
	m_renderAttribs["a_position"].stride = (int)sizeof(float[4]) * 3;
	m_renderAttribs["a_barycentricsA"].offset = (int)sizeof(float[4]);
	m_renderAttribs["a_barycentricsA"].stride = (int)sizeof(float[4]) * 3;
	m_renderAttribs["a_barycentricsB"].offset = (int)sizeof(float[4]) * 2;
	m_renderAttribs["a_barycentricsB"].stride = (int)sizeof(float[4]) * 3;

	for (int triangleNdx = 0; triangleNdx < numTriangles; ++triangleNdx)
	{
		const float angle		= ((float)triangleNdx) / (float)numTriangles * 2.0f * DE_PI;
		const float nextAngle	= ((float)triangleNdx + 1.0f) / (float)numTriangles * 2.0f * DE_PI;

		data[(triangleNdx * 3 + 0) * 3 + 0] = tcu::Vec4(0.2f, -0.3f, 0.0f, 1.0f);
		data[(triangleNdx * 3 + 0) * 3 + 1] = tcu::Vec4(1.0f,  0.0f, 0.0f, 0.0f);
		data[(triangleNdx * 3 + 0) * 3 + 2] = tcu::Vec4(1.0f,  0.0f, 0.0f, 0.0f);

		data[(triangleNdx * 3 + 1) * 3 + 0] = tcu::Vec4(2.0f * deFloatCos(angle), 2.0f * deFloatSin(angle), 0.0f, 1.0f);
		data[(triangleNdx * 3 + 1) * 3 + 1] = tcu::Vec4(0.0f,  1.0f, 0.0f, 0.0f);
		data[(triangleNdx * 3 + 1) * 3 + 2] = tcu::Vec4(0.0f,  1.0f, 0.0f, 0.0f);

		data[(triangleNdx * 3 + 2) * 3 + 0] = tcu::Vec4(2.0f * deFloatCos(nextAngle), 2.0f * deFloatSin(nextAngle), 0.0f, 1.0f);
		data[(triangleNdx * 3 + 2) * 3 + 1] = tcu::Vec4(0.0f,  0.0f, 1.0f, 0.0f);
		data[(triangleNdx * 3 + 2) * 3 + 2] = tcu::Vec4(0.0f,  0.0f, 1.0f, 0.0f);
	}

	gl.bindBuffer(GL_ARRAY_BUFFER, m_buffer);
	gl.bufferData(GL_ARRAY_BUFFER, (glw::GLsizeiptr)(data.size() * sizeof(tcu::Vec4)), data[0].getPtr(), GL_STATIC_DRAW);
}

class CentroidQualifierAtSampleCase : public CentroidRenderCase
{
public:
									CentroidQualifierAtSampleCase	(Context& context, const char* name, const char* description, int numSamples, RenderTarget target);
	virtual							~CentroidQualifierAtSampleCase	(void);

	void							init						(void);

private:
	enum
	{
		RENDER_SIZE = 128
	};

	std::string						genVertexSource				(int numTargetSamples) const;
	std::string						genFragmentSource			(int numTargetSamples) const;
	bool							verifyImage					(const tcu::Surface& resultImage);
};

CentroidQualifierAtSampleCase::CentroidQualifierAtSampleCase (Context& context, const char* name, const char* description, int numSamples, RenderTarget target)
	: CentroidRenderCase(context, name, description, numSamples, target, RENDER_SIZE)
{
}

CentroidQualifierAtSampleCase::~CentroidQualifierAtSampleCase (void)
{
}

void CentroidQualifierAtSampleCase::init (void)
{
	// test purpose and expectations
	m_testCtx.getLog()
		<< tcu::TestLog::Message
		<< "Verifying that interpolateAtSample ignores the centroid-qualifier.\n"
		<< "	Draw a fan of narrow triangles (large number of pixels on the edges).\n"
		<< "	Set varyings 'barycentricsA' and 'barycentricsB' to contain barycentric coordinates.\n"
		<< "	Add centroid-qualifier for barycentricsB.\n"
		<< "	=> interpolateAtSample(barycentricsB, N) ~= interpolateAtSample(barycentricsA, N)\n"
		<< tcu::TestLog::EndMessage;

	CentroidRenderCase::init();
}

std::string CentroidQualifierAtSampleCase::genVertexSource (int numTargetSamples) const
{
	DE_UNREF(numTargetSamples);

	std::ostringstream buf;

	buf <<	"${GLSL_VERSION_DECL}\n"
			"in highp vec4 a_position;\n"
			"in highp vec4 a_barycentricsA;\n"
			"in highp vec4 a_barycentricsB;\n"
			"out highp vec3 v_barycentricsA;\n"
			"centroid out highp vec3 v_barycentricsB;\n"
			"void main (void)\n"
			"{\n"
			"	gl_Position = a_position;\n"
			"	v_barycentricsA = a_barycentricsA.xyz;\n"
			"	v_barycentricsB = a_barycentricsB.xyz;\n"
			"}\n";

	return specializeShader(buf.str(), m_context.getRenderContext().getType());
}

std::string CentroidQualifierAtSampleCase::genFragmentSource (int numTargetSamples) const
{
	DE_UNREF(numTargetSamples);

	std::ostringstream buf;

	buf <<	"${GLSL_VERSION_DECL}\n"
			"${GLSL_EXT_SHADER_MULTISAMPLE_INTERPOLATION}"
			"in highp vec3 v_barycentricsA;\n"
			"centroid in highp vec3 v_barycentricsB;\n"
			"layout(location = 0) out mediump vec4 fragColor;\n"
			"void main (void)\n"
			"{\n"
			"	const highp float threshold = 0.0005;\n"
			"	bool allOk = true;\n"
			"\n"
			"	for (int sampleNdx = 0; sampleNdx < " << numTargetSamples << "; ++sampleNdx)\n"
			"	{\n"
			"		highp vec3 sampleA = interpolateAtSample(v_barycentricsA, sampleNdx);\n"
			"		highp vec3 sampleB = interpolateAtSample(v_barycentricsB, sampleNdx);\n"
			"		bool valuesEqual = all(lessThan(abs(sampleA - sampleB), vec3(threshold)));\n"
			"		if (!valuesEqual)\n"
			"			allOk = false;\n"
			"	}\n"
			"\n"
			"	if (allOk)\n"
			"		fragColor = vec4(0.0, 1.0, 0.0, 1.0);\n"
			"	else\n"
			"		fragColor = vec4(1.0, 0.0, 0.0, 1.0);\n"
			"}\n";

	return specializeShader(buf.str(), m_context.getRenderContext().getType());
}

bool CentroidQualifierAtSampleCase::verifyImage (const tcu::Surface& resultImage)
{
	return verifyGreenImage(resultImage, m_testCtx.getLog());
}

class InterpolateAtSampleIDCase : public MultisampleShaderRenderUtil::MultisampleRenderCase
{
public:
						InterpolateAtSampleIDCase	(Context& context, const char* name, const char* description, int numSamples, RenderTarget target);
	virtual				~InterpolateAtSampleIDCase	(void);

	void				init						(void);
private:
	enum
	{
		RENDER_SIZE = 32
	};

	std::string			genVertexSource				(int numTargetSamples) const;
	std::string			genFragmentSource			(int numTargetSamples) const;
	bool				verifyImage					(const tcu::Surface& resultImage);
};

InterpolateAtSampleIDCase::InterpolateAtSampleIDCase (Context& context, const char* name, const char* description, int numSamples, RenderTarget target)
	: MultisampleShaderRenderUtil::MultisampleRenderCase(context, name, description, numSamples, target, RENDER_SIZE)
{
}

InterpolateAtSampleIDCase::~InterpolateAtSampleIDCase (void)
{
}

void InterpolateAtSampleIDCase::init (void)
{
	// requirements
	if (!m_context.getContextInfo().isExtensionSupported("GL_OES_shader_multisample_interpolation") && !glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::es(3, 2)))
		TCU_THROW(NotSupportedError, "Test requires GL_OES_shader_multisample_interpolation extension");
	if (!m_context.getContextInfo().isExtensionSupported("GL_OES_sample_variables") && !glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::es(3, 2)))
		TCU_THROW(NotSupportedError, "Test requires GL_OES_sample_variables extension");

	// test purpose and expectations
	m_testCtx.getLog()
		<< tcu::TestLog::Message
		<< "Verifying that interpolateAtSample with the sample set to the current sampleID returns consistent values.\n"
		<< "	Interpolate varying containing screen space location.\n"
		<< "	=> interpolateAtSample(varying, sampleID) = varying"
		<< tcu::TestLog::EndMessage;

	MultisampleShaderRenderUtil::MultisampleRenderCase::init();
}

std::string InterpolateAtSampleIDCase::genVertexSource (int numTargetSamples) const
{
	DE_UNREF(numTargetSamples);

	std::ostringstream buf;

	buf <<	"${GLSL_VERSION_DECL}\n"
			"${GLSL_EXT_SHADER_MULTISAMPLE_INTERPOLATION}"
			"in highp vec4 a_position;\n"
			"sample out highp vec2 v_screenPosition;\n"
			"void main (void)\n"
			"{\n"
			"	gl_Position = a_position;\n"
			"	v_screenPosition = (a_position.xy + vec2(1.0, 1.0)) / 2.0 * vec2(" << (int)RENDER_SIZE << ".0, " << (int)RENDER_SIZE << ".0);\n"
			"}\n";

	return specializeShader(buf.str(), m_context.getRenderContext().getType());
}

std::string InterpolateAtSampleIDCase::genFragmentSource (int numTargetSamples) const
{
	DE_UNREF(numTargetSamples);

	std::ostringstream buf;

	buf <<	"${GLSL_VERSION_DECL}\n"
			"${GLSL_EXT_SAMPLE_VARIABLES}"
			"${GLSL_EXT_SHADER_MULTISAMPLE_INTERPOLATION}"
			"sample in highp vec2 v_screenPosition;\n"
			"layout(location = 0) out mediump vec4 fragColor;\n"
			"void main (void)\n"
			"{\n"
			"	const highp float threshold = 0.15625; // 4 subpixel bits. Assume 3 accurate bits + 0.03125 for other errors\n" // 0.03125 = mediump epsilon when value = 32 (RENDER_SIZE)
			"\n"
			"	highp vec2 offsetValue = interpolateAtSample(v_screenPosition, gl_SampleID);\n"
			"	highp vec2 refValue = v_screenPosition;\n"
			"\n"
			"	bool valuesEqual = all(lessThan(abs(offsetValue - refValue), vec2(threshold)));\n"
			"	if (valuesEqual)\n"
			"		fragColor = vec4(0.0, 1.0, 0.0, 1.0);\n"
			"	else\n"
			"		fragColor = vec4(1.0, 0.0, 0.0, 1.0);\n"
			"}\n";

	return specializeShader(buf.str(), m_context.getRenderContext().getType());
}

bool InterpolateAtSampleIDCase::verifyImage (const tcu::Surface& resultImage)
{
	return verifyGreenImage(resultImage, m_testCtx.getLog());
}

class InterpolateAtCentroidCase : public CentroidRenderCase
{
public:
	enum TestType
	{
		TEST_CONSISTENCY = 0,
		TEST_ARRAY_ELEMENT,

		TEST_LAST
	};

									InterpolateAtCentroidCase	(Context& context, const char* name, const char* description, int numSamples, RenderTarget target, TestType type);
	virtual							~InterpolateAtCentroidCase	(void);

	void							init						(void);

private:
	enum
	{
		RENDER_SIZE = 128
	};

	std::string						genVertexSource				(int numTargetSamples) const;
	std::string						genFragmentSource			(int numTargetSamples) const;
	bool							verifyImage					(const tcu::Surface& resultImage);

	const TestType					m_type;
};

InterpolateAtCentroidCase::InterpolateAtCentroidCase (Context& context, const char* name, const char* description, int numSamples, RenderTarget target, TestType type)
	: CentroidRenderCase	(context, name, description, numSamples, target, RENDER_SIZE)
	, m_type				(type)
{
}

InterpolateAtCentroidCase::~InterpolateAtCentroidCase (void)
{
}

void InterpolateAtCentroidCase::init (void)
{
	// test purpose and expectations
	if (m_type == TEST_CONSISTENCY)
	{
		m_testCtx.getLog()
			<< tcu::TestLog::Message
			<< "Verifying that interpolateAtCentroid does not return different values than a corresponding centroid-qualified varying.\n"
			<< "	Draw a fan of narrow triangles (large number of pixels on the edges).\n"
			<< "	Set varyings 'barycentricsA' and 'barycentricsB' to contain barycentric coordinates.\n"
			<< "	Add centroid-qualifier for barycentricsB.\n"
			<< "	=> interpolateAtCentroid(barycentricsA) ~= barycentricsB\n"
			<< tcu::TestLog::EndMessage;
	}
	else if (m_type == TEST_ARRAY_ELEMENT)
	{
		m_testCtx.getLog()
			<< tcu::TestLog::Message
			<< "Testing interpolateAtCentroid with element of array as an argument."
			<< tcu::TestLog::EndMessage;
	}
	else
		DE_ASSERT(false);

	CentroidRenderCase::init();
}

std::string InterpolateAtCentroidCase::genVertexSource (int numTargetSamples) const
{
	DE_UNREF(numTargetSamples);

	std::ostringstream buf;

	if (m_type == TEST_CONSISTENCY)
		buf <<	"${GLSL_VERSION_DECL}\n"
				"in highp vec4 a_position;\n"
				"in highp vec4 a_barycentricsA;\n"
				"in highp vec4 a_barycentricsB;\n"
				"out highp vec3 v_barycentricsA;\n"
				"centroid out highp vec3 v_barycentricsB;\n"
				"void main (void)\n"
				"{\n"
				"	gl_Position = a_position;\n"
				"	v_barycentricsA = a_barycentricsA.xyz;\n"
				"	v_barycentricsB = a_barycentricsB.xyz;\n"
				"}\n";
	else if (m_type == TEST_ARRAY_ELEMENT)
		buf <<	"${GLSL_VERSION_DECL}\n"
				"in highp vec4 a_position;\n"
				"in highp vec4 a_barycentricsA;\n"
				"in highp vec4 a_barycentricsB;\n"
				"out highp vec3[2] v_barycentrics;\n"
				"void main (void)\n"
				"{\n"
				"	gl_Position = a_position;\n"
				"	v_barycentrics[0] = a_barycentricsA.xyz;\n"
				"	v_barycentrics[1] = a_barycentricsB.xyz;\n"
				"}\n";
	else
		DE_ASSERT(false);

	return specializeShader(buf.str(), m_context.getRenderContext().getType());
}

std::string InterpolateAtCentroidCase::genFragmentSource (int numTargetSamples) const
{
	DE_UNREF(numTargetSamples);

	std::ostringstream buf;

	if (m_type == TEST_CONSISTENCY)
		buf <<	"${GLSL_VERSION_DECL}\n"
				"${GLSL_EXT_SHADER_MULTISAMPLE_INTERPOLATION}"
				"in highp vec3 v_barycentricsA;\n"
				"centroid in highp vec3 v_barycentricsB;\n"
				"layout(location = 0) out highp vec4 fragColor;\n"
				"void main (void)\n"
				"{\n"
				"	const highp float threshold = 0.0005;\n"
				"\n"
				"	highp vec3 centroidASampled = interpolateAtCentroid(v_barycentricsA);\n"
				"	bool valuesEqual = all(lessThan(abs(centroidASampled - v_barycentricsB), vec3(threshold)));\n"
				"	bool centroidAIsInvalid = any(greaterThan(centroidASampled, vec3(1.0))) ||\n"
				"	                          any(lessThan(centroidASampled, vec3(0.0)));\n"
				"	bool centroidBIsInvalid = any(greaterThan(v_barycentricsB, vec3(1.0))) ||\n"
				"	                          any(lessThan(v_barycentricsB, vec3(0.0)));\n"
				"\n"
				"	if (valuesEqual && !centroidAIsInvalid && !centroidBIsInvalid)\n"
				"		fragColor = vec4(0.0, 1.0, 0.0, 1.0);\n"
				"	else if (centroidAIsInvalid || centroidBIsInvalid)\n"
				"		fragColor = vec4(1.0, 0.0, 0.0, 1.0);\n"
				"	else\n"
				"		fragColor = vec4(1.0, 1.0, 0.0, 1.0);\n"
				"}\n";
	else if (m_type == TEST_ARRAY_ELEMENT)
		buf <<	"${GLSL_VERSION_DECL}\n"
				"${GLSL_EXT_SHADER_MULTISAMPLE_INTERPOLATION}"
				"in highp vec3[2] v_barycentrics;\n"
				"layout(location = 0) out mediump vec4 fragColor;\n"
				"void main (void)\n"
				"{\n"
				"	const highp float threshold = 0.0005;\n"
				"\n"
				"	highp vec3 centroidInterpolated = interpolateAtCentroid(v_barycentrics[1]);\n"
				"	bool centroidIsInvalid = any(greaterThan(centroidInterpolated, vec3(1.0))) ||\n"
				"	                         any(lessThan(centroidInterpolated, vec3(0.0)));\n"
				"\n"
				"	if (!centroidIsInvalid)\n"
				"		fragColor = vec4(0.0, 1.0, 0.0, 1.0);\n"
				"	else\n"
				"		fragColor = vec4(1.0, 0.0, 0.0, 1.0);\n"
				"}\n";
	else
		DE_ASSERT(false);

	return specializeShader(buf.str(), m_context.getRenderContext().getType());
}

bool InterpolateAtCentroidCase::verifyImage (const tcu::Surface& resultImage)
{
	return verifyGreenImage(resultImage, m_testCtx.getLog());
}

class InterpolateAtOffsetCase : public MultisampleShaderRenderUtil::MultisampleRenderCase
{
public:
	enum TestType
	{
		TEST_QUALIFIER_NONE = 0,
		TEST_QUALIFIER_CENTROID,
		TEST_QUALIFIER_SAMPLE,
		TEST_ARRAY_ELEMENT,

		TEST_LAST
	};
						InterpolateAtOffsetCase		(Context& context, const char* name, const char* description, int numSamples, RenderTarget target, TestType testType);
	virtual				~InterpolateAtOffsetCase	(void);

	void				init						(void);
private:
	enum
	{
		RENDER_SIZE = 32
	};

	std::string			genVertexSource				(int numTargetSamples) const;
	std::string			genFragmentSource			(int numTargetSamples) const;
	bool				verifyImage					(const tcu::Surface& resultImage);

	const TestType		m_testType;
};

InterpolateAtOffsetCase::InterpolateAtOffsetCase (Context& context, const char* name, const char* description, int numSamples, RenderTarget target, TestType testType)
	: MultisampleShaderRenderUtil::MultisampleRenderCase	(context, name, description, numSamples, target, RENDER_SIZE)
	, m_testType											(testType)
{
	DE_ASSERT(testType < TEST_LAST);
}

InterpolateAtOffsetCase::~InterpolateAtOffsetCase (void)
{
}

void InterpolateAtOffsetCase::init (void)
{
	// requirements
	if (!m_context.getContextInfo().isExtensionSupported("GL_OES_shader_multisample_interpolation") && !glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::es(3, 2)))
		TCU_THROW(NotSupportedError, "Test requires GL_OES_shader_multisample_interpolation extension");

	// test purpose and expectations
	m_testCtx.getLog()
		<< tcu::TestLog::Message
		<< "Verifying that interpolateAtOffset returns correct values.\n"
		<< "	Interpolate varying containing screen space location.\n"
		<< "	=> interpolateAtOffset(varying, offset) should be \"varying value at the pixel center\" + offset"
		<< tcu::TestLog::EndMessage;

	MultisampleShaderRenderUtil::MultisampleRenderCase::init();
}

std::string InterpolateAtOffsetCase::genVertexSource (int numTargetSamples) const
{
	DE_UNREF(numTargetSamples);

	std::ostringstream buf;
	buf << "${GLSL_VERSION_DECL}\n"
		<< "${GLSL_EXT_SHADER_MULTISAMPLE_INTERPOLATION}"
		<< "in highp vec4 a_position;\n";

	if (m_testType == TEST_QUALIFIER_NONE || m_testType == TEST_QUALIFIER_CENTROID || m_testType == TEST_QUALIFIER_SAMPLE)
	{
		const char* const qualifier = (m_testType == TEST_QUALIFIER_CENTROID) ? ("centroid ") : (m_testType == TEST_QUALIFIER_SAMPLE) ? ("sample ") : ("");
		buf << qualifier << "out highp vec2 v_screenPosition;\n"
			<< qualifier << "out highp vec2 v_offset;\n";
	}
	else if (m_testType == TEST_ARRAY_ELEMENT)
	{
		buf << "out highp vec2[2] v_screenPosition;\n"
			<< "out highp vec2 v_offset;\n";
	}
	else
		DE_ASSERT(false);

	buf	<< "void main (void)\n"
		<< "{\n"
		<< "	gl_Position = a_position;\n";

	if (m_testType != TEST_ARRAY_ELEMENT)
		buf	<< "	v_screenPosition = (a_position.xy + vec2(1.0, 1.0)) / 2.0 * vec2(" << (int)RENDER_SIZE << ".0, " << (int)RENDER_SIZE << ".0);\n";
	else
		buf	<< "	v_screenPosition[0] = a_position.xy; // not used\n"
				"	v_screenPosition[1] = (a_position.xy + vec2(1.0, 1.0)) / 2.0 * vec2(" << (int)RENDER_SIZE << ".0, " << (int)RENDER_SIZE << ".0);\n";

	buf	<< "	v_offset = a_position.xy * 0.5f;\n"
		<< "}\n";

	return specializeShader(buf.str(), m_context.getRenderContext().getType());
}

std::string InterpolateAtOffsetCase::genFragmentSource (int numTargetSamples) const
{
	DE_UNREF(numTargetSamples);

	const char* const	arrayIndexing = (m_testType == TEST_ARRAY_ELEMENT) ? ("[1]") : ("");
	std::ostringstream	buf;

	buf <<	"${GLSL_VERSION_DECL}\n"
			"${GLSL_EXT_SHADER_MULTISAMPLE_INTERPOLATION}";

	if (m_testType == TEST_QUALIFIER_NONE || m_testType == TEST_QUALIFIER_CENTROID || m_testType == TEST_QUALIFIER_SAMPLE)
	{
		const char* const qualifier = (m_testType == TEST_QUALIFIER_CENTROID) ? ("centroid ") : (m_testType == TEST_QUALIFIER_SAMPLE) ? ("sample ") : ("");
		buf	<< qualifier << "in highp vec2 v_screenPosition;\n"
			<< qualifier << "in highp vec2 v_offset;\n";
	}
	else if (m_testType == TEST_ARRAY_ELEMENT)
	{
		buf << "in highp vec2[2] v_screenPosition;\n"
			<< "in highp vec2 v_offset;\n";
	}
	else
		DE_ASSERT(false);

	buf	<<	"layout(location = 0) out mediump vec4 fragColor;\n"
			"void main (void)\n"
			"{\n"
			"	const highp float threshold = 0.15625; // 4 subpixel bits. Assume 3 accurate bits + 0.03125 for other errors\n" // 0.03125 = mediump epsilon when value = 32 (RENDER_SIZE)
			"\n"
			"	highp vec2 pixelCenter = floor(v_screenPosition" << arrayIndexing << ") + vec2(0.5, 0.5);\n"
			"	highp vec2 offsetValue = interpolateAtOffset(v_screenPosition" << arrayIndexing << ", v_offset);\n"
			"	highp vec2 refValue = pixelCenter + v_offset;\n"
			"\n"
			"	bool valuesEqual = all(lessThan(abs(offsetValue - refValue), vec2(threshold)));\n"
			"	if (valuesEqual)\n"
			"		fragColor = vec4(0.0, 1.0, 0.0, 1.0);\n"
			"	else\n"
			"		fragColor = vec4(1.0, 0.0, 0.0, 1.0);\n"
			"}\n";

	return specializeShader(buf.str(), m_context.getRenderContext().getType());
}

bool InterpolateAtOffsetCase::verifyImage (const tcu::Surface& resultImage)
{
	return verifyGreenImage(resultImage, m_testCtx.getLog());
}

class InterpolateAtSamplePositionCase : public MultisampleShaderRenderUtil::MultisampleRenderCase
{
public:
						InterpolateAtSamplePositionCase		(Context& context, const char* name, const char* description, int numSamples, RenderTarget target);
	virtual				~InterpolateAtSamplePositionCase	(void);

	void				init								(void);
private:
	enum
	{
		RENDER_SIZE = 32
	};

	std::string			genVertexSource						(int numTargetSamples) const;
	std::string			genFragmentSource					(int numTargetSamples) const;
	bool				verifyImage							(const tcu::Surface& resultImage);
};

InterpolateAtSamplePositionCase::InterpolateAtSamplePositionCase (Context& context, const char* name, const char* description, int numSamples, RenderTarget target)
	: MultisampleShaderRenderUtil::MultisampleRenderCase(context, name, description, numSamples, target, RENDER_SIZE)
{
}

InterpolateAtSamplePositionCase::~InterpolateAtSamplePositionCase (void)
{
}

void InterpolateAtSamplePositionCase::init (void)
{
	// requirements
	if (!m_context.getContextInfo().isExtensionSupported("GL_OES_shader_multisample_interpolation") && !glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::es(3, 2)))
		TCU_THROW(NotSupportedError, "Test requires GL_OES_shader_multisample_interpolation extension");
	if (!m_context.getContextInfo().isExtensionSupported("GL_OES_sample_variables") && !glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::es(3, 2)))
		TCU_THROW(NotSupportedError, "Test requires GL_OES_sample_variables extension");

	// test purpose and expectations
	m_testCtx.getLog()
		<< tcu::TestLog::Message
		<< "Verifying that interpolateAtOffset with the offset of current sample position returns consistent values.\n"
		<< "	Interpolate varying containing screen space location.\n"
		<< "	=> interpolateAtOffset(varying, currentOffset) = varying"
		<< tcu::TestLog::EndMessage;

	MultisampleShaderRenderUtil::MultisampleRenderCase::init();
}

std::string InterpolateAtSamplePositionCase::genVertexSource (int numTargetSamples) const
{
	DE_UNREF(numTargetSamples);

	std::ostringstream buf;

	buf <<	"${GLSL_VERSION_DECL}\n"
			"${GLSL_EXT_SHADER_MULTISAMPLE_INTERPOLATION}"
			"in highp vec4 a_position;\n"
			"sample out highp vec2 v_screenPosition;\n"
			"void main (void)\n"
			"{\n"
			"	gl_Position = a_position;\n"
			"	v_screenPosition = (a_position.xy + vec2(1.0, 1.0)) / 2.0 * vec2(" << (int)RENDER_SIZE << ".0, " << (int)RENDER_SIZE << ".0);\n"
			"}\n";

	return specializeShader(buf.str(), m_context.getRenderContext().getType());
}

std::string InterpolateAtSamplePositionCase::genFragmentSource (int numTargetSamples) const
{
	DE_UNREF(numTargetSamples);

	std::ostringstream buf;

	buf <<	"${GLSL_VERSION_DECL}\n"
			"${GLSL_EXT_SAMPLE_VARIABLES}"
			"${GLSL_EXT_SHADER_MULTISAMPLE_INTERPOLATION}"
			"sample in highp vec2 v_screenPosition;\n"
			"layout(location = 0) out mediump vec4 fragColor;\n"
			"void main (void)\n"
			"{\n"
			"	const highp float threshold = 0.15625; // 4 subpixel bits. Assume 3 accurate bits + 0.03125 for other errors\n" // 0.03125 = mediump epsilon when value = 32 (RENDER_SIZE)
			"\n"
			"	highp vec2 offset = gl_SamplePosition - vec2(0.5, 0.5);\n"
			"	highp vec2 offsetValue = interpolateAtOffset(v_screenPosition, offset);\n"
			"	highp vec2 refValue = v_screenPosition;\n"
			"\n"
			"	bool valuesEqual = all(lessThan(abs(offsetValue - refValue), vec2(threshold)));\n"
			"	if (valuesEqual)\n"
			"		fragColor = vec4(0.0, 1.0, 0.0, 1.0);\n"
			"	else\n"
			"		fragColor = vec4(1.0, 0.0, 0.0, 1.0);\n"
			"}\n";

	return specializeShader(buf.str(), m_context.getRenderContext().getType());
}

bool InterpolateAtSamplePositionCase::verifyImage (const tcu::Surface& resultImage)
{
	return verifyGreenImage(resultImage, m_testCtx.getLog());
}

class NegativeCompileInterpolationCase : public TestCase
{
public:
	enum CaseType
	{
		CASE_VEC4_IDENTITY_SWIZZLE = 0,
		CASE_VEC4_CROP_SWIZZLE,
		CASE_VEC4_MIXED_SWIZZLE,
		CASE_INTERPOLATE_IVEC4,
		CASE_INTERPOLATE_UVEC4,
		CASE_INTERPOLATE_ARRAY,
		CASE_INTERPOLATE_STRUCT,
		CASE_INTERPOLATE_STRUCT_MEMBER,
		CASE_INTERPOLATE_LOCAL,
		CASE_INTERPOLATE_GLOBAL,
		CASE_INTERPOLATE_CONSTANT,

		CASE_LAST
	};
	enum InterpolatorType
	{
		INTERPOLATE_AT_SAMPLE = 0,
		INTERPOLATE_AT_CENTROID,
		INTERPOLATE_AT_OFFSET,

		INTERPOLATE_LAST
	};

							NegativeCompileInterpolationCase	(Context& context, const char* name, const char* description, CaseType caseType, InterpolatorType interpolator);

private:
	void					init								(void);
	IterateResult			iterate								(void);

	std::string				genShaderSource						(void) const;

	const CaseType			m_caseType;
	const InterpolatorType	m_interpolation;
};

NegativeCompileInterpolationCase::NegativeCompileInterpolationCase (Context& context, const char* name, const char* description, CaseType caseType, InterpolatorType interpolator)
	: TestCase			(context, name, description)
	, m_caseType		(caseType)
	, m_interpolation	(interpolator)
{
	DE_ASSERT(m_caseType < CASE_LAST);
	DE_ASSERT(m_interpolation < INTERPOLATE_LAST);
}

void NegativeCompileInterpolationCase::init (void)
{
	if (!m_context.getContextInfo().isExtensionSupported("GL_OES_shader_multisample_interpolation") && !glu::contextSupports(m_context.getRenderContext().getType(), glu::ApiType::es(3, 2)))
		TCU_THROW(NotSupportedError, "Test requires GL_OES_shader_multisample_interpolation extension");

	m_testCtx.getLog() << tcu::TestLog::Message << "Trying to compile illegal shader, expecting compile to fail." << tcu::TestLog::EndMessage;
}

NegativeCompileInterpolationCase::IterateResult NegativeCompileInterpolationCase::iterate (void)
{
	const std::string	source			= genShaderSource();
	glu::Shader			shader			(m_context.getRenderContext(), glu::SHADERTYPE_FRAGMENT);
	const char* const	sourceStrPtr	= source.c_str();

	m_testCtx.getLog()	<< tcu::TestLog::Message
						<< "Fragment shader source:"
						<< tcu::TestLog::EndMessage
						<< tcu::TestLog::KernelSource(source);

	shader.setSources(1, &sourceStrPtr, DE_NULL);
	shader.compile();

	m_testCtx.getLog()	<< tcu::TestLog::Message
						<< "Info log:"
						<< tcu::TestLog::EndMessage
						<< tcu::TestLog::KernelSource(shader.getInfoLog());

	if (shader.getCompileStatus())
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "ERROR: Illegal shader compiled successfully." << tcu::TestLog::EndMessage;
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Unexpected compile status");
	}
	else
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "Compile failed as expected." << tcu::TestLog::EndMessage;
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}
	return STOP;
}

std::string NegativeCompileInterpolationCase::genShaderSource (void) const
{
	std::ostringstream	buf;
	std::string			interpolation;
	const char*			interpolationTemplate;
	const char*			description;
	const char*			globalDeclarations		= "";
	const char*			localDeclarations		= "";
	const char*			interpolationTarget		= "";
	const char*			postSelector			= "";

	switch (m_caseType)
	{
		case CASE_VEC4_IDENTITY_SWIZZLE:
			globalDeclarations	= "in highp vec4 v_var;\n";
			interpolationTarget	= "v_var.xyzw";
			description			= "component selection is illegal";
			break;

		case CASE_VEC4_CROP_SWIZZLE:
			globalDeclarations	= "in highp vec4 v_var;\n";
			interpolationTarget	= "v_var.xy";
			postSelector		= ".x";
			description			= "component selection is illegal";
			break;

		case CASE_VEC4_MIXED_SWIZZLE:
			globalDeclarations	= "in highp vec4 v_var;\n";
			interpolationTarget	= "v_var.yzxw";
			description			= "component selection is illegal";
			break;

		case CASE_INTERPOLATE_IVEC4:
			globalDeclarations	= "flat in highp ivec4 v_var;\n";
			interpolationTarget	= "v_var";
			description			= "no overload for ivec";
			break;

		case CASE_INTERPOLATE_UVEC4:
			globalDeclarations	= "flat in highp uvec4 v_var;\n";
			interpolationTarget	= "v_var";
			description			= "no overload for uvec";
			break;

		case CASE_INTERPOLATE_ARRAY:
			globalDeclarations	= "in highp float v_var[2];\n";
			interpolationTarget	= "v_var";
			postSelector		= "[1]";
			description			= "no overload for arrays";
			break;

		case CASE_INTERPOLATE_STRUCT:
		case CASE_INTERPOLATE_STRUCT_MEMBER:
			globalDeclarations	=	"struct S\n"
									"{\n"
									"	highp float a;\n"
									"	highp float b;\n"
									"};\n"
									"in S v_var;\n";

			interpolationTarget	= (m_caseType == CASE_INTERPOLATE_STRUCT) ? ("v_var")						: ("v_var.a");
			postSelector		= (m_caseType == CASE_INTERPOLATE_STRUCT) ? (".a")							: ("");
			description			= (m_caseType == CASE_INTERPOLATE_STRUCT) ? ("no overload for this type")	: ("<interpolant> is not an input variable (just a member of)");
			break;

		case CASE_INTERPOLATE_LOCAL:
			localDeclarations	= "	highp vec4 local_var = gl_FragCoord;\n";
			interpolationTarget	= "local_var";
			description			= "<interpolant> is not an input variable";
			break;

		case CASE_INTERPOLATE_GLOBAL:
			globalDeclarations	= "highp vec4 global_var;\n";
			localDeclarations	= "	global_var = gl_FragCoord;\n";
			interpolationTarget	= "global_var";
			description			= "<interpolant> is not an input variable";
			break;

		case CASE_INTERPOLATE_CONSTANT:
			globalDeclarations	= "const highp vec4 const_var = vec4(0.2);\n";
			interpolationTarget	= "const_var";
			description			= "<interpolant> is not an input variable";
			break;

		default:
			DE_ASSERT(false);
			return "";
	}

	switch (m_interpolation)
	{
		case INTERPOLATE_AT_SAMPLE:
			interpolationTemplate = "interpolateAtSample(${TARGET}, 0)${POST_SELECTOR}";
			break;

		case INTERPOLATE_AT_CENTROID:
			interpolationTemplate = "interpolateAtCentroid(${TARGET})${POST_SELECTOR}";
			break;

		case INTERPOLATE_AT_OFFSET:
			interpolationTemplate = "interpolateAtOffset(${TARGET}, vec2(0.2, 0.2))${POST_SELECTOR}";
			break;

		default:
			DE_ASSERT(false);
			return "";
	}

	{
		std::map<std::string, std::string> args;
		args["TARGET"] = interpolationTarget;
		args["POST_SELECTOR"] = postSelector;

		interpolation = tcu::StringTemplate(interpolationTemplate).specialize(args);
	}

	buf <<	glu::getGLSLVersionDeclaration(glu::getContextTypeGLSLVersion(m_context.getRenderContext().getType())) << "\n"
		<< "${GLSL_EXT_SHADER_MULTISAMPLE_INTERPOLATION}"
		<<	globalDeclarations
		<<	"layout(location = 0) out mediump vec4 fragColor;\n"
			"void main (void)\n"
			"{\n"
		<<	localDeclarations
		<<	"	fragColor = vec4(" << interpolation << "); // " << description << "\n"
			"}\n";

	return specializeShader(buf.str(), m_context.getRenderContext().getType());
}

} // anonymous

ShaderMultisampleInterpolationTests::ShaderMultisampleInterpolationTests (Context& context)
	: TestCaseGroup(context, "multisample_interpolation", "Test multisample interpolation")
{
}

ShaderMultisampleInterpolationTests::~ShaderMultisampleInterpolationTests (void)
{
}

void ShaderMultisampleInterpolationTests::init (void)
{
	using namespace MultisampleShaderRenderUtil;

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

	static const struct
	{
		const char*									name;
		const char*									description;
		NegativeCompileInterpolationCase::CaseType	caseType;
	} negativeCompileCases[] =
	{
		{ "vec4_identity_swizzle",		"use identity swizzle",				NegativeCompileInterpolationCase::CASE_VEC4_IDENTITY_SWIZZLE		},
		{ "vec4_crop_swizzle",			"use cropped identity swizzle",		NegativeCompileInterpolationCase::CASE_VEC4_CROP_SWIZZLE			},
		{ "vec4_mixed_swizzle",			"use swizzle",						NegativeCompileInterpolationCase::CASE_VEC4_MIXED_SWIZZLE			},
		{ "interpolate_ivec4",			"interpolate integer variable",		NegativeCompileInterpolationCase::CASE_INTERPOLATE_IVEC4			},
		{ "interpolate_uvec4",			"interpolate integer variable",		NegativeCompileInterpolationCase::CASE_INTERPOLATE_UVEC4			},
		{ "interpolate_array",			"interpolate whole array",			NegativeCompileInterpolationCase::CASE_INTERPOLATE_ARRAY			},
		{ "interpolate_struct",			"interpolate whole struct",			NegativeCompileInterpolationCase::CASE_INTERPOLATE_STRUCT			},
		{ "interpolate_struct_member",	"interpolate struct member",		NegativeCompileInterpolationCase::CASE_INTERPOLATE_STRUCT_MEMBER	},
		{ "interpolate_local",			"interpolate local variable",		NegativeCompileInterpolationCase::CASE_INTERPOLATE_LOCAL			},
		{ "interpolate_global",			"interpolate global variable",		NegativeCompileInterpolationCase::CASE_INTERPOLATE_GLOBAL			},
		{ "interpolate_constant",		"interpolate constant variable",	NegativeCompileInterpolationCase::CASE_INTERPOLATE_CONSTANT			},
	};

	// .sample_qualifier
	{
		tcu::TestCaseGroup* const sampleQualifierGroup = new tcu::TestCaseGroup(m_testCtx, "sample_qualifier", "Test sample qualifier");
		addChild(sampleQualifierGroup);

		for (int targetNdx = 0; targetNdx < DE_LENGTH_OF_ARRAY(targets); ++targetNdx)
			sampleQualifierGroup->addChild(new SampleQualifierRenderCase(m_context, targets[targetNdx].name, targets[targetNdx].desc, targets[targetNdx].numSamples, targets[targetNdx].target));
	}

	// .interpolate_at_sample
	{
		tcu::TestCaseGroup* const interpolateAtSampleGroup = new tcu::TestCaseGroup(m_testCtx, "interpolate_at_sample", "Test interpolateAtSample");
		addChild(interpolateAtSampleGroup);

		// .static_sample_number
		{
			tcu::TestCaseGroup* const group = new tcu::TestCaseGroup(m_testCtx, "static_sample_number", "Test interpolateAtSample sample number");
			interpolateAtSampleGroup->addChild(group);

			for (int targetNdx = 0; targetNdx < DE_LENGTH_OF_ARRAY(targets); ++targetNdx)
				group->addChild(new InterpolateAtSampleRenderCase(m_context, targets[targetNdx].name, targets[targetNdx].desc, targets[targetNdx].numSamples, targets[targetNdx].target, InterpolateAtSampleRenderCase::INDEXING_STATIC));
		}

		// .dynamic_sample_number
		{
			tcu::TestCaseGroup* const group = new tcu::TestCaseGroup(m_testCtx, "dynamic_sample_number", "Test interpolateAtSample sample number");
			interpolateAtSampleGroup->addChild(group);

			for (int targetNdx = 0; targetNdx < DE_LENGTH_OF_ARRAY(targets); ++targetNdx)
				group->addChild(new InterpolateAtSampleRenderCase(m_context, targets[targetNdx].name, targets[targetNdx].desc, targets[targetNdx].numSamples, targets[targetNdx].target, InterpolateAtSampleRenderCase::INDEXING_DYNAMIC));
		}

		// .non_multisample_buffer
		{
			tcu::TestCaseGroup* const group = new tcu::TestCaseGroup(m_testCtx, "non_multisample_buffer", "Test interpolateAtSample with non-multisample buffers");
			interpolateAtSampleGroup->addChild(group);

			for (int targetNdx = 0; targetNdx < DE_LENGTH_OF_ARRAY(targets); ++targetNdx)
				if (targets[targetNdx].numSamples == 0)
					group->addChild(new SingleSampleInterpolateAtSampleCase(m_context, std::string("sample_0_").append(targets[targetNdx].name).c_str(), targets[targetNdx].desc, targets[targetNdx].numSamples, targets[targetNdx].target, SingleSampleInterpolateAtSampleCase::SAMPLE_0));

			for (int targetNdx = 0; targetNdx < DE_LENGTH_OF_ARRAY(targets); ++targetNdx)
				if (targets[targetNdx].numSamples == 0)
					group->addChild(new SingleSampleInterpolateAtSampleCase(m_context, std::string("sample_n_").append(targets[targetNdx].name).c_str(), targets[targetNdx].desc, targets[targetNdx].numSamples, targets[targetNdx].target, SingleSampleInterpolateAtSampleCase::SAMPLE_N));
		}

		// .centroid_qualifier
		{
			tcu::TestCaseGroup* const group = new tcu::TestCaseGroup(m_testCtx, "centroid_qualified", "Test interpolateAtSample with centroid qualified varying");
			interpolateAtSampleGroup->addChild(group);

			for (int targetNdx = 0; targetNdx < DE_LENGTH_OF_ARRAY(targets); ++targetNdx)
				group->addChild(new CentroidQualifierAtSampleCase(m_context, targets[targetNdx].name, targets[targetNdx].desc, targets[targetNdx].numSamples, targets[targetNdx].target));
		}

		// .at_sample_id
		{
			tcu::TestCaseGroup* const group = new tcu::TestCaseGroup(m_testCtx, "at_sample_id", "Test interpolateAtSample at current sample id");
			interpolateAtSampleGroup->addChild(group);

			for (int targetNdx = 0; targetNdx < DE_LENGTH_OF_ARRAY(targets); ++targetNdx)
				group->addChild(new InterpolateAtSampleIDCase(m_context, targets[targetNdx].name, targets[targetNdx].desc, targets[targetNdx].numSamples, targets[targetNdx].target));
		}

		// .negative
		{
			tcu::TestCaseGroup* const group = new tcu::TestCaseGroup(m_testCtx, "negative", "interpolateAtSample negative tests");
			interpolateAtSampleGroup->addChild(group);

			for (int ndx = 0; ndx < DE_LENGTH_OF_ARRAY(negativeCompileCases); ++ndx)
				group->addChild(new NegativeCompileInterpolationCase(m_context,
																	 negativeCompileCases[ndx].name,
																	 negativeCompileCases[ndx].description,
																	 negativeCompileCases[ndx].caseType,
																	 NegativeCompileInterpolationCase::INTERPOLATE_AT_SAMPLE));
		}
	}

	// .interpolate_at_centroid
	{
		tcu::TestCaseGroup* const methodGroup = new tcu::TestCaseGroup(m_testCtx, "interpolate_at_centroid", "Test interpolateAtCentroid");
		addChild(methodGroup);

		// .consistency
		{
			tcu::TestCaseGroup* const group = new tcu::TestCaseGroup(m_testCtx, "consistency", "Test interpolateAtCentroid return value is consistent to centroid qualified value");
			methodGroup->addChild(group);

			for (int targetNdx = 0; targetNdx < DE_LENGTH_OF_ARRAY(targets); ++targetNdx)
				group->addChild(new InterpolateAtCentroidCase(m_context, targets[targetNdx].name, targets[targetNdx].desc, targets[targetNdx].numSamples, targets[targetNdx].target, InterpolateAtCentroidCase::TEST_CONSISTENCY));
		}

		// .array_element
		{
			tcu::TestCaseGroup* const group = new tcu::TestCaseGroup(m_testCtx, "array_element", "Test interpolateAtCentroid with array element");
			methodGroup->addChild(group);

			for (int targetNdx = 0; targetNdx < DE_LENGTH_OF_ARRAY(targets); ++targetNdx)
				group->addChild(new InterpolateAtCentroidCase(m_context, targets[targetNdx].name, targets[targetNdx].desc, targets[targetNdx].numSamples, targets[targetNdx].target, InterpolateAtCentroidCase::TEST_ARRAY_ELEMENT));
		}

		// .negative
		{
			tcu::TestCaseGroup* const group = new tcu::TestCaseGroup(m_testCtx, "negative", "interpolateAtCentroid negative tests");
			methodGroup->addChild(group);

			for (int ndx = 0; ndx < DE_LENGTH_OF_ARRAY(negativeCompileCases); ++ndx)
				group->addChild(new NegativeCompileInterpolationCase(m_context,
																	 negativeCompileCases[ndx].name,
																	 negativeCompileCases[ndx].description,
																	 negativeCompileCases[ndx].caseType,
																	 NegativeCompileInterpolationCase::INTERPOLATE_AT_CENTROID));
		}
	}

	// .interpolate_at_offset
	{
		static const struct TestConfig
		{
			const char*							name;
			InterpolateAtOffsetCase::TestType	type;
		} configs[] =
		{
			{ "no_qualifiers",		InterpolateAtOffsetCase::TEST_QUALIFIER_NONE		},
			{ "centroid_qualifier",	InterpolateAtOffsetCase::TEST_QUALIFIER_CENTROID	},
			{ "sample_qualifier",	InterpolateAtOffsetCase::TEST_QUALIFIER_SAMPLE		},
		};

		tcu::TestCaseGroup* const methodGroup = new tcu::TestCaseGroup(m_testCtx, "interpolate_at_offset", "Test interpolateAtOffset");
		addChild(methodGroup);

		// .no_qualifiers
		// .centroid_qualifier
		// .sample_qualifier
		for (int configNdx = 0; configNdx < DE_LENGTH_OF_ARRAY(configs); ++configNdx)
		{
			tcu::TestCaseGroup* const qualifierGroup = new tcu::TestCaseGroup(m_testCtx, configs[configNdx].name, "Test interpolateAtOffset with qualified/non-qualified varying");
			methodGroup->addChild(qualifierGroup);

			for (int targetNdx = 0; targetNdx < DE_LENGTH_OF_ARRAY(targets); ++targetNdx)
				qualifierGroup->addChild(new InterpolateAtOffsetCase(m_context, targets[targetNdx].name, targets[targetNdx].desc, targets[targetNdx].numSamples, targets[targetNdx].target, configs[configNdx].type));
		}

		// .at_sample_position
		{
			tcu::TestCaseGroup* const group = new tcu::TestCaseGroup(m_testCtx, "at_sample_position", "Test interpolateAtOffset at sample position");
			methodGroup->addChild(group);

			for (int targetNdx = 0; targetNdx < DE_LENGTH_OF_ARRAY(targets); ++targetNdx)
				group->addChild(new InterpolateAtSamplePositionCase(m_context, targets[targetNdx].name, targets[targetNdx].desc, targets[targetNdx].numSamples, targets[targetNdx].target));
		}

		// .array_element
		{
			tcu::TestCaseGroup* const group = new tcu::TestCaseGroup(m_testCtx, "array_element", "Test interpolateAtOffset with array element");
			methodGroup->addChild(group);

			for (int targetNdx = 0; targetNdx < DE_LENGTH_OF_ARRAY(targets); ++targetNdx)
				group->addChild(new InterpolateAtOffsetCase(m_context, targets[targetNdx].name, targets[targetNdx].desc, targets[targetNdx].numSamples, targets[targetNdx].target, InterpolateAtOffsetCase::TEST_ARRAY_ELEMENT));
		}

		// .negative
		{
			tcu::TestCaseGroup* const group = new tcu::TestCaseGroup(m_testCtx, "negative", "interpolateAtOffset negative tests");
			methodGroup->addChild(group);

			for (int ndx = 0; ndx < DE_LENGTH_OF_ARRAY(negativeCompileCases); ++ndx)
				group->addChild(new NegativeCompileInterpolationCase(m_context,
																	 negativeCompileCases[ndx].name,
																	 negativeCompileCases[ndx].description,
																	 negativeCompileCases[ndx].caseType,
																	 NegativeCompileInterpolationCase::INTERPOLATE_AT_OFFSET));
		}
	}
}

} // Functional
} // gles31
} // deqp
