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
 * \brief Internal format query tests
 *//*--------------------------------------------------------------------*/

#include "es31fInternalFormatQueryTests.hpp"
#include "tcuTestLog.hpp"
#include "gluRenderContext.hpp"
#include "gluStrUtil.hpp"
#include "gluContextInfo.hpp"
#include "glwFunctions.hpp"
#include "glwEnums.hpp"

namespace deqp
{
namespace gles31
{
namespace Functional
{
namespace
{

class FormatSamplesCase : public TestCase
{
public:
	enum FormatType
	{
		FORMAT_COLOR,
		FORMAT_INT,
		FORMAT_DEPTH_STENCIL
	};

						FormatSamplesCase	(Context& ctx, const char* name, const char* desc, glw::GLenum texTarget, glw::GLenum internalFormat, FormatType type);
private:
	void				init				(void);
	IterateResult		iterate				(void);

	const glw::GLenum	m_target;
	const glw::GLenum	m_internalFormat;
	const FormatType	m_type;
};

FormatSamplesCase::FormatSamplesCase (Context& ctx, const char* name, const char* desc, glw::GLenum target, glw::GLenum internalFormat, FormatType type)
	: TestCase			(ctx, name, desc)
	, m_target			(target)
	, m_internalFormat	(internalFormat)
	, m_type			(type)
{
	DE_ASSERT(m_target == GL_TEXTURE_2D_MULTISAMPLE			||
			  m_target == GL_TEXTURE_2D_MULTISAMPLE_ARRAY	||
			  m_target == GL_RENDERBUFFER);
}

void FormatSamplesCase::init (void)
{
	const bool isTextureTarget	=	(m_target == GL_TEXTURE_2D_MULTISAMPLE) ||
									(m_target == GL_TEXTURE_2D_MULTISAMPLE_ARRAY);
	const bool supportsES32		=	contextSupports(m_context.getRenderContext().getType(), glu::ApiType::es(3, 2));

	if (!supportsES32 && m_target == GL_TEXTURE_2D_MULTISAMPLE_ARRAY && !m_context.getContextInfo().isExtensionSupported("GL_OES_texture_storage_multisample_2d_array"))
		TCU_THROW(NotSupportedError, "Test requires OES_texture_storage_multisample_2d_array extension or a context version equal or higher than 3.2");

	// stencil8 textures are not supported without GL_OES_texture_stencil8 extension
	if (!supportsES32 && isTextureTarget && m_internalFormat == GL_STENCIL_INDEX8 && !m_context.getContextInfo().isExtensionSupported("GL_OES_texture_stencil8"))
		TCU_THROW(NotSupportedError, "Test requires GL_OES_texture_stencil8 extension or a context version equal or higher than 3.2");
}

FormatSamplesCase::IterateResult FormatSamplesCase::iterate (void)
{
	const glw::Functions&	gl				= m_context.getRenderContext().getFunctions();
	bool					isFloatFormat	= false;
	bool					error			= false;
	glw::GLint				maxSamples		= 0;
	glw::GLint				numSampleCounts	= 0;
	const bool				supportsES32			= contextSupports(m_context.getRenderContext().getType(), glu::ApiType::es(3, 2));

	if (!supportsES32)
	{
		if (m_internalFormat == GL_RGBA16F || m_internalFormat == GL_R32F || m_internalFormat == GL_RG32F || m_internalFormat == GL_RGBA32F || m_internalFormat == GL_R16F || m_internalFormat == GL_RG16F || m_internalFormat == GL_R11F_G11F_B10F)
		{
			TCU_THROW(NotSupportedError, "The internal format is not supported in a context lower than 3.2");
		}
	}
	else if (m_internalFormat == GL_RGBA16F || m_internalFormat == GL_R32F || m_internalFormat == GL_RG32F || m_internalFormat == GL_RGBA32F)
	{
		isFloatFormat = true;
	}

	// Lowest limit
	{
		const glw::GLenum samplesEnum = (m_type == FORMAT_COLOR) ? (GL_MAX_COLOR_TEXTURE_SAMPLES) : (m_type == FORMAT_INT) ? (GL_MAX_INTEGER_SAMPLES) : (GL_MAX_DEPTH_TEXTURE_SAMPLES);
		m_testCtx.getLog() << tcu::TestLog::Message << "Format must support sample count of " << glu::getGettableStateStr(samplesEnum) << tcu::TestLog::EndMessage;

		gl.getIntegerv(samplesEnum, &maxSamples);
		GLU_EXPECT_NO_ERROR(gl.getError(), "get MAX_*_SAMPLES");

		m_testCtx.getLog() << tcu::TestLog::Message << glu::getGettableStateStr(samplesEnum) << " = " << maxSamples << tcu::TestLog::EndMessage;

		if (maxSamples < 1)
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "ERROR: minimum value of "  << glu::getGettableStateStr(samplesEnum) << " is 1" << tcu::TestLog::EndMessage;
			error = true;
		}
	}

	// Number of sample counts
	{
		gl.getInternalformativ(m_target, m_internalFormat, GL_NUM_SAMPLE_COUNTS, 1, &numSampleCounts);
		GLU_EXPECT_NO_ERROR(gl.getError(), "get GL_NUM_SAMPLE_COUNTS");

		m_testCtx.getLog() << tcu::TestLog::Message << "GL_NUM_SAMPLE_COUNTS = " << numSampleCounts << tcu::TestLog::EndMessage;

		if (!isFloatFormat)
		{
			if (numSampleCounts < 1)
			{
				m_testCtx.getLog() << tcu::TestLog::Message << "ERROR: Format MUST support some multisample configuration, got GL_NUM_SAMPLE_COUNTS = " << numSampleCounts << tcu::TestLog::EndMessage;
				error = true;
			}
		}
	}

	// Sample counts
	{
		tcu::MessageBuilder		samplesMsg	(&m_testCtx.getLog());
		std::vector<glw::GLint>	samples		(numSampleCounts > 0 ? numSampleCounts : 1);

		if (numSampleCounts > 0 || isFloatFormat)
		{
			gl.getInternalformativ(m_target, m_internalFormat, GL_SAMPLES, numSampleCounts, &samples[0]);
			GLU_EXPECT_NO_ERROR(gl.getError(), "get GL_SAMPLES");
		}
		else
			TCU_FAIL("glGetInternalFormativ() reported 0 supported sample counts");

		// make a pretty log

		samplesMsg << "GL_SAMPLES = [";
		for (size_t ndx = 0; ndx < samples.size(); ++ndx)
		{
			if (ndx)
				samplesMsg << ", ";
			samplesMsg << samples[ndx];
		}
		samplesMsg << "]" << tcu::TestLog::EndMessage;

		// Samples are in order
		for (size_t ndx = 1; ndx < samples.size(); ++ndx)
		{
			if (samples[ndx-1] <= samples[ndx])
			{
				m_testCtx.getLog() << tcu::TestLog::Message << "ERROR: Samples must be ordered descending." << tcu::TestLog::EndMessage;
				error = true;
				break;
			}
		}

		// samples are positive
		for (size_t ndx = 1; ndx < samples.size(); ++ndx)
		{
			if (samples[ndx-1] <= 0)
			{
				m_testCtx.getLog() << tcu::TestLog::Message << "ERROR: Only positive SAMPLES allowed." << tcu::TestLog::EndMessage;
				error = true;
				break;
			}
		}

		// maxSamples must be supported
		if (!isFloatFormat)
		{
			if (samples[0] < maxSamples)
			{
				m_testCtx.getLog() << tcu::TestLog::Message << "ERROR: MAX_*_SAMPLES must be supported." << tcu::TestLog::EndMessage;
				error = true;
			}
		}
	}

	// Result
	if (!error)
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	else
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Invalid value");

	return STOP;
}

class NumSampleCountsBufferCase : public TestCase
{
public:
					NumSampleCountsBufferCase	(Context& ctx, const char* name, const char* desc);

private:
	IterateResult	iterate						(void);
};

NumSampleCountsBufferCase::NumSampleCountsBufferCase (Context& ctx, const char* name, const char* desc)
	: TestCase(ctx, name, desc)
{
}

NumSampleCountsBufferCase::IterateResult NumSampleCountsBufferCase::iterate (void)
{
	const glw::GLint		defaultValue	= -123; // queries always return positive values
	const glw::Functions&	gl				= m_context.getRenderContext().getFunctions();
	bool					error			= false;

	// Query to larger buffer
	{
		glw::GLint buffer[2] = { defaultValue, defaultValue };

		m_testCtx.getLog() << tcu::TestLog::Message << "Querying GL_NUM_SAMPLE_COUNTS to larger-than-needed buffer." << tcu::TestLog::EndMessage;
		gl.getInternalformativ(GL_TEXTURE_2D_MULTISAMPLE, GL_RGBA8, GL_NUM_SAMPLE_COUNTS, 2, buffer);
		GLU_EXPECT_NO_ERROR(gl.getError(), "get GL_NUM_SAMPLE_COUNTS");

		if (buffer[1] != defaultValue)
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "ERROR: trailing values were modified." << tcu::TestLog::EndMessage;
			error = true;
		}
	}

	// Query to empty buffer
	{
		glw::GLint buffer[1] = { defaultValue };

		m_testCtx.getLog() << tcu::TestLog::Message << "Querying GL_NUM_SAMPLE_COUNTS to zero-sized buffer." << tcu::TestLog::EndMessage;
		gl.getInternalformativ(GL_TEXTURE_2D_MULTISAMPLE, GL_RGBA8, GL_NUM_SAMPLE_COUNTS, 0, buffer);
		GLU_EXPECT_NO_ERROR(gl.getError(), "get GL_NUM_SAMPLE_COUNTS");

		if (buffer[0] != defaultValue)
		{
			m_testCtx.getLog() << tcu::TestLog::Message << "ERROR: Query wrote over buffer bounds." << tcu::TestLog::EndMessage;
			error = true;
		}
	}

	// Result
	if (!error)
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	else
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Unexpected buffer modification");

	return STOP;
}

class SamplesBufferCase : public TestCase
{
public:
					SamplesBufferCase	(Context& ctx, const char* name, const char* desc);

private:
	IterateResult	iterate				(void);
};

SamplesBufferCase::SamplesBufferCase (Context& ctx, const char* name, const char* desc)
	: TestCase(ctx, name, desc)
{
}

SamplesBufferCase::IterateResult SamplesBufferCase::iterate (void)
{
	const glw::GLint		defaultValue	= -123; // queries always return positive values
	const glw::Functions&	gl				= m_context.getRenderContext().getFunctions();
	bool					error			= false;

	glw::GLint				numSampleCounts	= 0;

	// Number of sample counts
	{
		gl.getInternalformativ(GL_TEXTURE_2D_MULTISAMPLE, GL_RGBA8, GL_NUM_SAMPLE_COUNTS, 1, &numSampleCounts);
		GLU_EXPECT_NO_ERROR(gl.getError(), "get GL_NUM_SAMPLE_COUNTS");

		m_testCtx.getLog() << tcu::TestLog::Message << "GL_NUM_SAMPLE_COUNTS = " << numSampleCounts << tcu::TestLog::EndMessage;
	}

	if (numSampleCounts < 1)
	{
		m_testCtx.getLog() << tcu::TestLog::Message << "ERROR: Format MUST support some multisample configuration, got GL_NUM_SAMPLE_COUNTS = " << numSampleCounts << tcu::TestLog::EndMessage;
		error = true;
	}
	else
	{
		// Query to larger buffer
		{
			std::vector<glw::GLint> buffer(numSampleCounts + 1, defaultValue);

			m_testCtx.getLog() << tcu::TestLog::Message << "Querying GL_SAMPLES to larger-than-needed buffer." << tcu::TestLog::EndMessage;
			gl.getInternalformativ(GL_TEXTURE_2D_MULTISAMPLE, GL_RGBA8, GL_SAMPLES, (glw::GLsizei)buffer.size(), &buffer[0]);
			GLU_EXPECT_NO_ERROR(gl.getError(), "get GL_SAMPLES");

			if (buffer.back() != defaultValue)
			{
				m_testCtx.getLog() << tcu::TestLog::Message << "ERROR: trailing value was modified." << tcu::TestLog::EndMessage;
				error = true;
			}
		}

		// Query to smaller buffer
		if (numSampleCounts > 2)
		{
			glw::GLint buffer[3] = { defaultValue, defaultValue, defaultValue };

			m_testCtx.getLog() << tcu::TestLog::Message << "Querying GL_SAMPLES to buffer with bufSize=2." << tcu::TestLog::EndMessage;
			gl.getInternalformativ(GL_TEXTURE_2D_MULTISAMPLE, GL_RGBA8, GL_SAMPLES, 2, buffer);
			GLU_EXPECT_NO_ERROR(gl.getError(), "get GL_SAMPLES");

			if (buffer[2] != defaultValue)
			{
				m_testCtx.getLog() << tcu::TestLog::Message << "ERROR: Query wrote over buffer bounds." << tcu::TestLog::EndMessage;
				error = true;
			}
		}

		// Query to empty buffer
		{
			glw::GLint buffer[1] = { defaultValue };

			m_testCtx.getLog() << tcu::TestLog::Message << "Querying GL_SAMPLES to zero-sized buffer." << tcu::TestLog::EndMessage;
			gl.getInternalformativ(GL_TEXTURE_2D_MULTISAMPLE, GL_RGBA8, GL_SAMPLES, 0, buffer);
			GLU_EXPECT_NO_ERROR(gl.getError(), "get GL_SAMPLES");

			if (buffer[0] != defaultValue)
			{
				m_testCtx.getLog() << tcu::TestLog::Message << "ERROR: Query wrote over buffer bounds." << tcu::TestLog::EndMessage;
				error = true;
			}
		}
	}

	// Result
	if (!error)
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	else
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Unexpected buffer modification");

	return STOP;
}

} // anonymous

InternalFormatQueryTests::InternalFormatQueryTests (Context& context)
	: TestCaseGroup(context, "internal_format", "Internal format queries")
{
}

InternalFormatQueryTests::~InternalFormatQueryTests (void)
{
}

void InternalFormatQueryTests::init (void)
{
	static const struct InternalFormat
	{
		const char*						name;
		glw::GLenum						format;
		FormatSamplesCase::FormatType	type;
	} internalFormats[] =
	{
		// color renderable
		{ "r8",						GL_R8,					FormatSamplesCase::FORMAT_COLOR			},
		{ "rg8",					GL_RG8,					FormatSamplesCase::FORMAT_COLOR			},
		{ "rgb8",					GL_RGB8,				FormatSamplesCase::FORMAT_COLOR			},
		{ "rgb565",					GL_RGB565,				FormatSamplesCase::FORMAT_COLOR			},
		{ "rgba4",					GL_RGBA4,				FormatSamplesCase::FORMAT_COLOR			},
		{ "rgb5_a1",				GL_RGB5_A1,				FormatSamplesCase::FORMAT_COLOR			},
		{ "rgba8",					GL_RGBA8,				FormatSamplesCase::FORMAT_COLOR			},
		{ "rgb10_a2",				GL_RGB10_A2,			FormatSamplesCase::FORMAT_COLOR			},
		{ "rgb10_a2ui",				GL_RGB10_A2UI,			FormatSamplesCase::FORMAT_INT			},
		{ "srgb8_alpha8",			GL_SRGB8_ALPHA8,		FormatSamplesCase::FORMAT_COLOR			},
		{ "r8i",					GL_R8I,					FormatSamplesCase::FORMAT_INT			},
		{ "r8ui",					GL_R8UI,				FormatSamplesCase::FORMAT_INT			},
		{ "r16i",					GL_R16I,				FormatSamplesCase::FORMAT_INT			},
		{ "r16ui",					GL_R16UI,				FormatSamplesCase::FORMAT_INT			},
		{ "r32i",					GL_R32I,				FormatSamplesCase::FORMAT_INT			},
		{ "r32ui",					GL_R32UI,				FormatSamplesCase::FORMAT_INT			},
		{ "rg8i",					GL_RG8I,				FormatSamplesCase::FORMAT_INT			},
		{ "rg8ui",					GL_RG8UI,				FormatSamplesCase::FORMAT_INT			},
		{ "rg16i",					GL_RG16I,				FormatSamplesCase::FORMAT_INT			},
		{ "rg16ui",					GL_RG16UI,				FormatSamplesCase::FORMAT_INT			},
		{ "rg32i",					GL_RG32I,				FormatSamplesCase::FORMAT_INT			},
		{ "rg32ui",					GL_RG32UI,				FormatSamplesCase::FORMAT_INT			},
		{ "rgba8i",					GL_RGBA8I,				FormatSamplesCase::FORMAT_INT			},
		{ "rgba8ui",				GL_RGBA8UI,				FormatSamplesCase::FORMAT_INT			},
		{ "rgba16i",				GL_RGBA16I,				FormatSamplesCase::FORMAT_INT			},
		{ "rgba16ui",				GL_RGBA16UI,			FormatSamplesCase::FORMAT_INT			},
		{ "rgba32i",				GL_RGBA32I,				FormatSamplesCase::FORMAT_INT			},
		{ "rgba32ui",				GL_RGBA32UI,			FormatSamplesCase::FORMAT_INT			},

		// float formats
		{ "r16f",					GL_R16F,				FormatSamplesCase::FORMAT_COLOR			},
		{ "rg16f",					GL_RG16F,				FormatSamplesCase::FORMAT_COLOR			},
		{ "rgba16f",				GL_RGBA16F,				FormatSamplesCase::FORMAT_COLOR			},
		{ "r32f",					GL_R32F,				FormatSamplesCase::FORMAT_INT			},
		{ "rg32f",					GL_RG32F,				FormatSamplesCase::FORMAT_INT			},
		{ "rgba32f",				GL_RGBA32F,				FormatSamplesCase::FORMAT_INT			},
		{ "r11f_g11f_b10f",			GL_R11F_G11F_B10F,		FormatSamplesCase::FORMAT_COLOR			},

		// depth renderable
		{ "depth_component16",		GL_DEPTH_COMPONENT16,	FormatSamplesCase::FORMAT_DEPTH_STENCIL	},
		{ "depth_component24",		GL_DEPTH_COMPONENT24,	FormatSamplesCase::FORMAT_DEPTH_STENCIL	},
		{ "depth_component32f",		GL_DEPTH_COMPONENT32F,	FormatSamplesCase::FORMAT_DEPTH_STENCIL	},
		{ "depth24_stencil8",		GL_DEPTH24_STENCIL8,	FormatSamplesCase::FORMAT_DEPTH_STENCIL	},
		{ "depth32f_stencil8",		GL_DEPTH32F_STENCIL8,	FormatSamplesCase::FORMAT_DEPTH_STENCIL	},

		// stencil renderable
		{ "stencil_index8",			GL_STENCIL_INDEX8,		FormatSamplesCase::FORMAT_DEPTH_STENCIL	}
		// DEPTH24_STENCIL8,  duplicate
		// DEPTH32F_STENCIL8  duplicate
	};

	static const struct
	{
		const char*	name;
		deUint32	target;
	} textureTargets[] =
	{
		{ "renderbuffer",					GL_RENDERBUFFER					},
		{ "texture_2d_multisample",			GL_TEXTURE_2D_MULTISAMPLE		},
		{ "texture_2d_multisample_array",	GL_TEXTURE_2D_MULTISAMPLE_ARRAY	},
	};

	for (int groupNdx = 0; groupNdx < DE_LENGTH_OF_ARRAY(textureTargets); ++groupNdx)
	{
		tcu::TestCaseGroup* const	group		= new tcu::TestCaseGroup(m_testCtx, textureTargets[groupNdx].name, glu::getInternalFormatTargetName(textureTargets[groupNdx].target));
		const glw::GLenum			texTarget	= textureTargets[groupNdx].target;

		addChild(group);

		for (int caseNdx = 0; caseNdx < DE_LENGTH_OF_ARRAY(internalFormats); ++caseNdx)
		{
			const std::string name = std::string(internalFormats[caseNdx].name) + "_samples";
			const std::string desc = std::string("Verify GL_SAMPLES of ") + internalFormats[caseNdx].name;

			group->addChild(new FormatSamplesCase(m_context, name.c_str(), desc.c_str(), texTarget, internalFormats[caseNdx].format, internalFormats[caseNdx].type));
		}
	}

	// Check buffer sizes are honored
	{
		tcu::TestCaseGroup* const group = new tcu::TestCaseGroup(m_testCtx, "partial_query", "Query data to too short a buffer");

		addChild(group);

		group->addChild(new NumSampleCountsBufferCase	(m_context, "num_sample_counts",	"Query GL_NUM_SAMPLE_COUNTS to too short a buffer"));
		group->addChild(new SamplesBufferCase			(m_context, "samples",				"Query GL_SAMPLES to too short a buffer"));
	}
}

} // Functional
} // gles31
} // deqp
