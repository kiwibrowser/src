/*-------------------------------------------------------------------------
 * drawElements Quality Program EGL Module
 * ---------------------------------------
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
 * \brief Config query tests.
 *//*--------------------------------------------------------------------*/

#include "teglQueryConfigTests.hpp"
#include "teglSimpleConfigCase.hpp"
#include "tcuTestLog.hpp"
#include "tcuTestContext.hpp"
#include "tcuCommandLine.hpp"
#include "egluCallLogWrapper.hpp"
#include "egluStrUtil.hpp"
#include "egluUtil.hpp"
#include "eglwLibrary.hpp"
#include "eglwEnums.hpp"
#include "deRandom.hpp"

#include <string>
#include <vector>

namespace deqp
{
namespace egl
{

using eglu::ConfigInfo;
using tcu::TestLog;
using namespace eglw;

static void logConfigAttribute (TestLog& log, EGLenum attrib, EGLint value)
{
	log << TestLog::Message << "  " << eglu::getConfigAttribName(attrib) << ": " << eglu::getConfigAttribValueStr(attrib, value) << TestLog::EndMessage;
}

static bool isAttributePresent (const eglu::Version& version, EGLenum attribute)
{
	switch (attribute)
	{
		case EGL_CONFORMANT:
			if (version < eglu::Version(1, 3)) return false;
			break;
		case EGL_LUMINANCE_SIZE:
		case EGL_ALPHA_MASK_SIZE:
		case EGL_COLOR_BUFFER_TYPE:
		case EGL_MATCH_NATIVE_PIXMAP:
			if (version < eglu::Version(1, 2)) return false;
			break;
		case EGL_BIND_TO_TEXTURE_RGB:
		case EGL_BIND_TO_TEXTURE_RGBA:
		case EGL_MAX_SWAP_INTERVAL:
		case EGL_MIN_SWAP_INTERVAL:
		case EGL_RENDERABLE_TYPE:
			if (version < eglu::Version(1, 1)) return false;
			break;
		default:
			break;
	}

	return true;
}

class GetConfigsBoundsCase : public TestCase, protected eglu::CallLogWrapper
{
public:
	GetConfigsBoundsCase (EglTestContext& eglTestCtx, const char* name, const char* description)
		: TestCase		(eglTestCtx, name, description)
		, CallLogWrapper(eglTestCtx.getLibrary(), eglTestCtx.getTestContext().getLog())
		, m_display		(EGL_NO_DISPLAY)
	{
	}

	void init (void)
	{
		DE_ASSERT(m_display == EGL_NO_DISPLAY);
		m_display = eglu::getAndInitDisplay(m_eglTestCtx.getNativeDisplay());
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}

	void deinit (void)
	{
		m_eglTestCtx.getLibrary().terminate(m_display);
		m_display = EGL_NO_DISPLAY;
	}

	void checkGetConfigsBounds (de::Random& rnd, const int numConfigAll, const int numConfigRequested)
	{
		tcu::TestLog&			log				= m_testCtx.getLog();
		std::vector<EGLConfig>	buffer			(numConfigAll + 10);

		std::vector<deUint32>	magicBuffer		((buffer.size() * sizeof(EGLConfig)) / sizeof(deUint32) + 1);
		const EGLConfig*		magicConfigs	= reinterpret_cast<EGLConfig*>(&magicBuffer[0]);

		int						numConfigReturned;

		// Fill buffers with magic
		for (size_t ndx = 0; ndx < magicBuffer.size(); ndx++)	magicBuffer[ndx]	= rnd.getUint32();
		for (size_t ndx = 0; ndx < buffer.size(); ndx++)		buffer[ndx]			= magicConfigs[ndx];

		eglGetConfigs(m_display, &buffer[0], numConfigRequested, &numConfigReturned);
		eglu::checkError(eglGetError(), DE_NULL, __FILE__, __LINE__);

		log << TestLog::Message << numConfigReturned << " configs returned" << TestLog::EndMessage;

		// Compare results with stored magic
		{
			int	numOverwritten	= 0;

			for (size_t ndx = 0; ndx < buffer.size(); ndx++)
			{
				if (buffer[ndx] == magicConfigs[ndx])
				{
					numOverwritten = (int)ndx;
					break;
				}
			}

			log << TestLog::Message << numOverwritten << " values actually written" << TestLog::EndMessage;

			if (numConfigReturned > deMax32(numConfigRequested, 0))
			{
				log << TestLog::Message << "Fail, more configs returned than requested." << TestLog::EndMessage;
				m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Too many configs returned");
			}

			if (numOverwritten > deMax32(numConfigReturned, 0))
			{
				log << TestLog::Message << "Fail, buffer overflow detected." << TestLog::EndMessage;
				m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Buffer overflow");
			}
			else if (numOverwritten != numConfigReturned)
			{
				log << TestLog::Message << "Fail, reported number of returned configs differs from number of values written." << TestLog::EndMessage;
				m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Incorrect size");
			}
		}
	}

	IterateResult iterate (void)
	{
		tcu::TestLog&	log		= m_testCtx.getLog();
		EGLint			numConfigAll;

		enableLogging(true);

		eglGetConfigs(m_display, 0, 0, &numConfigAll);

		log << TestLog::Message << numConfigAll << " configs available" << TestLog::EndMessage;
		log << TestLog::Message << TestLog::EndMessage;

		if (numConfigAll > 0)
		{
			de::Random		rnd					(123);

			for (int i = 0; i < 5; i++)
			{
				checkGetConfigsBounds(rnd, numConfigAll, rnd.getInt(0, numConfigAll));
				log << TestLog::Message << TestLog::EndMessage;
			}

			checkGetConfigsBounds(rnd, numConfigAll, -1);
		}
		else
		{
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "No configs");
		}

		enableLogging(false);

		return STOP;
	}

protected:
	EGLDisplay	m_display;
};

class GetConfigAttribCase : public TestCase, protected eglu::CallLogWrapper
{
public:
	GetConfigAttribCase (EglTestContext& eglTestCtx, const char* name, const char* description);

	void			init		(void);
	void			deinit		(void);
	IterateResult	iterate		(void);

	EGLint			getValue	(EGLConfig config, EGLenum attrib, bool logValue=true);

	virtual void	executeTest	(EGLConfig config) = 0;

protected:
	EGLDisplay								m_display;

private:
	std::vector<EGLConfig>					m_configs;
	std::vector<EGLConfig>::const_iterator	m_configsIter;
};

GetConfigAttribCase::GetConfigAttribCase (EglTestContext& eglTestCtx, const char* name, const char* description)
	: TestCase			(eglTestCtx, name, description)
	, CallLogWrapper	(eglTestCtx.getLibrary(), eglTestCtx.getTestContext().getLog())
	, m_display			(EGL_NO_DISPLAY)
{
}

void GetConfigAttribCase::init (void)
{
	DE_ASSERT(m_display == EGL_NO_DISPLAY);
	m_display		= eglu::getAndInitDisplay(m_eglTestCtx.getNativeDisplay());
	m_configs		= eglu::getConfigs(m_eglTestCtx.getLibrary(), m_display);
	m_configsIter	= m_configs.begin();

	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
}

void GetConfigAttribCase::deinit (void)
{
	m_eglTestCtx.getLibrary().terminate(m_display);
	m_display = EGL_NO_DISPLAY;
}

tcu::TestNode::IterateResult GetConfigAttribCase::iterate (void)
{
	tcu::TestLog&	log		= m_testCtx.getLog();

	if (m_configsIter == m_configs.end())
	{
		log << TestLog::Message << "No configs available." << TestLog::EndMessage;
		return STOP;
	}

	{
		const EGLConfig	config	= *m_configsIter;
		EGLint			id;

		eglGetConfigAttrib(m_display, config, EGL_CONFIG_ID, &id);
		eglu::checkError(eglGetError(), DE_NULL, __FILE__, __LINE__);
		log << TestLog::Message << "Config ID " << id << TestLog::EndMessage;

		executeTest(config);
	}

	log << TestLog::Message << TestLog::EndMessage;

	m_configsIter++;

	if (m_configsIter == m_configs.end())
		return STOP;
	else
		return CONTINUE;
}

EGLint GetConfigAttribCase::getValue (EGLConfig config, EGLenum attrib, bool logValue)
{
	TestLog&	log		= m_testCtx.getLog();
	EGLint		value;

	eglGetConfigAttrib(m_display, config, attrib, &value);
	eglu::checkError(eglGetError(), DE_NULL, __FILE__, __LINE__);

	if (logValue)
		logConfigAttribute(log, attrib, value);

	return value;
}

class GetConfigAttribSimpleCase : public GetConfigAttribCase
{
public:
	GetConfigAttribSimpleCase (EglTestContext& eglTestCtx, const char* name, const char* description, EGLenum attribute)
		: GetConfigAttribCase(eglTestCtx, name, description)
		, m_attrib(attribute)
	{
	}

	void checkColorBufferType (EGLint value)
	{
		const bool isRGBBuffer			= value == EGL_RGB_BUFFER;
		const bool isLuminanceBuffer	= value == EGL_LUMINANCE_BUFFER;
		const bool isYuvBuffer			= value == EGL_YUV_BUFFER_EXT;
		const bool hasYuvSupport		= eglu::hasExtension(m_eglTestCtx.getLibrary(), m_display, "EGL_EXT_yuv_surface");

		if (!(isRGBBuffer || isLuminanceBuffer || (isYuvBuffer && hasYuvSupport)))
		{
			TestLog&	log	= m_testCtx.getLog();

			log << TestLog::Message << "Fail, invalid EGL_COLOR_BUFFER_TYPE value" << TestLog::EndMessage;
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Invalid value");
		}
	}

	void checkCaveat (EGLint value)
	{
		if (!(value == EGL_NONE || value == EGL_SLOW_CONFIG || value == EGL_NON_CONFORMANT_CONFIG))
		{
			TestLog&	log	= m_testCtx.getLog();

			log << TestLog::Message << "Fail, invalid EGL_CONFIG_CAVEAT value" << TestLog::EndMessage;
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Invalid value");
		}
	}

	void checkTransparentType (EGLint value)
	{
		if (!(value == EGL_NONE || value == EGL_TRANSPARENT_RGB))
		{
			TestLog&	log	= m_testCtx.getLog();

			log << TestLog::Message << "Fail, invalid EGL_TRANSPARENT_TYPE value" << TestLog::EndMessage;
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Invalid value");
		}
	}

	void checkBoolean (EGLenum attrib, EGLint value)
	{
		if (!(value == EGL_FALSE || value == EGL_TRUE))
		{
			TestLog&	log	= m_testCtx.getLog();

			log << TestLog::Message << "Fail, " << eglu::getConfigAttribStr(attrib) << " should be a boolean value." << TestLog::EndMessage;
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Invalid value");
		}
	}

	void checkInteger (EGLenum attrib, EGLint value)
	{
		if (attrib == EGL_NATIVE_VISUAL_ID || attrib == EGL_NATIVE_VISUAL_TYPE) // Implementation-defined
			return;

		if (attrib == EGL_CONFIG_ID && value < 1)
		{
			TestLog&	log	= m_testCtx.getLog();

			log << TestLog::Message << "Fail, config IDs should be positive integer values beginning from 1." << TestLog::EndMessage;
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Invalid value");
		}
	}

	void checkSurfaceTypeMask (EGLint value)
	{
		const EGLint	wantedBits	= EGL_WINDOW_BIT | EGL_PIXMAP_BIT | EGL_PBUFFER_BIT;

		if ((value & wantedBits) == 0)
		{
			TestLog&	log	= m_testCtx.getLog();

			log << TestLog::Message << "Fail, config does not actually support creation of any surface type?" << TestLog::EndMessage;
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Invalid value");
		}
	}

	void checkAttribute (EGLenum attrib, EGLint value)
	{
		switch (attrib)
		{
			case EGL_COLOR_BUFFER_TYPE:
				checkColorBufferType(value);
				break;
			case EGL_CONFIG_CAVEAT:
				checkCaveat(value);
				break;
			case EGL_TRANSPARENT_TYPE:
				checkTransparentType(value);
				break;
			case EGL_CONFORMANT:
			case EGL_RENDERABLE_TYPE:
				// Just print what we know
				break;
			case EGL_SURFACE_TYPE:
				checkSurfaceTypeMask(value);
				break;
			case EGL_BIND_TO_TEXTURE_RGB:
			case EGL_BIND_TO_TEXTURE_RGBA:
			case EGL_NATIVE_RENDERABLE:
				checkBoolean(attrib, value);
				break;
			default:
				checkInteger(attrib, value);
		}
	}

	void executeTest (EGLConfig config)
	{
		TestLog&			log		= m_testCtx.getLog();
		eglu::Version		version	= eglu::getVersion(m_eglTestCtx.getLibrary(), m_display);

		if (!isAttributePresent(version, m_attrib))
		{
			log << TestLog::Message << eglu::getConfigAttribStr(m_attrib) << " not supported by this EGL version";
		}
		else
		{
			EGLint			value;

			enableLogging(true);

			eglGetConfigAttrib(m_display, config, m_attrib, &value);
			eglu::checkError(eglGetError(), DE_NULL, __FILE__, __LINE__);

			logConfigAttribute(log, m_attrib, value);
			checkAttribute(m_attrib, value);

			enableLogging(false);
		}
	}

private:
	EGLenum	m_attrib;
};

class GetConfigAttribBufferSizeCase : public GetConfigAttribCase
{
public:
	GetConfigAttribBufferSizeCase (EglTestContext& eglTestCtx, const char* name, const char* description)
		: GetConfigAttribCase(eglTestCtx, name, description)
	{
	}

	void executeTest (EGLConfig config)
	{
		TestLog&		log				= m_testCtx.getLog();

		const EGLint	colorBufferType	= getValue(config, EGL_COLOR_BUFFER_TYPE);

		const EGLint	bufferSize		= getValue(config, EGL_BUFFER_SIZE);
		const EGLint	redSize			= getValue(config, EGL_RED_SIZE);
		const EGLint	greenSize		= getValue(config, EGL_GREEN_SIZE);
		const EGLint	blueSize		= getValue(config, EGL_BLUE_SIZE);
		const EGLint	luminanceSize	= getValue(config, EGL_LUMINANCE_SIZE);
		const EGLint	alphaSize		= getValue(config, EGL_ALPHA_SIZE);

		if (alphaSize < 0)
		{
			log << TestLog::Message << "Fail, alpha size must be zero or positive." << TestLog::EndMessage;
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Invalid alpha size");
		}

		if (colorBufferType == EGL_RGB_BUFFER)
		{
			if (luminanceSize != 0)
			{
				log << TestLog::Message << "Fail, luminance size must be zero for an RGB buffer." << TestLog::EndMessage;
				m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Invalid luminance size");
			}

			if (redSize <= 0 || greenSize <= 0  || blueSize <= 0)
			{
				log << TestLog::Message << "Fail, RGB component sizes must be positive for an RGB buffer." << TestLog::EndMessage;
				m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Invalid color component size");
			}

			if (bufferSize != (redSize + greenSize + blueSize + alphaSize))
			{
				log << TestLog::Message << "Fail, buffer size must be equal to the sum of RGB component sizes and alpha size." << TestLog::EndMessage;
				m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Invalid buffer size");
			}
		}
		else if (colorBufferType == EGL_LUMINANCE_BUFFER)
		{
			if (luminanceSize <= 0)
			{
				log << TestLog::Message << "Fail, luminance size must be positive for a luminance buffer." << TestLog::EndMessage;
				m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Invalid luminance size");
			}

			if (redSize != 0 || greenSize != 0  || blueSize != 0)
			{
				log << TestLog::Message << "Fail, RGB component sizes must be zero for a luminance buffer." << TestLog::EndMessage;
				m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Invalid color component size");
			}

			if (bufferSize != (luminanceSize + alphaSize))
			{
				log << TestLog::Message << "Fail, buffer size must be equal to the sum of luminance size and alpha size." << TestLog::EndMessage;
				m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Invalid buffer size");
			}
		}
	}
};

class GetConfigAttribTransparentValueCase : public GetConfigAttribCase
{
public:
	GetConfigAttribTransparentValueCase (EglTestContext& eglTestCtx, const char* name, const char* description)
		: GetConfigAttribCase(eglTestCtx, name, description)
	{
	}

	void executeTest (EGLConfig config)
	{
		TestLog&		log	= m_testCtx.getLog();

		const EGLint	transparentType	= getValue(config, EGL_TRANSPARENT_TYPE);
		const EGLint	redValue		= getValue(config, EGL_TRANSPARENT_RED_VALUE);
		const EGLint	greenValue		= getValue(config, EGL_TRANSPARENT_GREEN_VALUE);
		const EGLint	blueValue		= getValue(config, EGL_TRANSPARENT_BLUE_VALUE);

		const EGLint	redSize			= getValue(config, EGL_RED_SIZE);
		const EGLint	greenSize		= getValue(config, EGL_GREEN_SIZE);
		const EGLint	blueSize		= getValue(config, EGL_BLUE_SIZE);

		if (transparentType == EGL_TRANSPARENT_RGB)
		{
			if (   (redValue	< 0	|| redValue		>= (1 << redSize))
				|| (greenValue	< 0	|| greenValue	>= (1 << greenSize))
				|| (blueValue	< 0	|| blueValue	>= (1 << blueSize))	)
			{
				log << TestLog::Message << "Fail, transparent color values must lie between 0 and the maximum component value." << TestLog::EndMessage;
				m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Invalid transparent color value");
			}
		}
	}
};

QueryConfigTests::QueryConfigTests (EglTestContext& eglTestCtx)
	: TestCaseGroup(eglTestCtx, "query_config", "Surface config query tests")
{
}

QueryConfigTests::~QueryConfigTests (void)
{
}

void QueryConfigTests::init (void)
{
	// eglGetGonfigs
	{
		tcu::TestCaseGroup* getConfigsGroup = new tcu::TestCaseGroup(m_testCtx, "get_configs", "eglGetConfigs tests");
		addChild(getConfigsGroup);

		getConfigsGroup->addChild(new GetConfigsBoundsCase(m_eglTestCtx, "get_configs_bounds", "eglGetConfigs bounds checking test"));
	}

	// eglGetConfigAttrib
	{
		static const struct
		{
			EGLenum			attribute;
			const char*		testName;
		} attributes[] =
		{
			{ EGL_BUFFER_SIZE,				"buffer_size"				},
			{ EGL_RED_SIZE,					"red_size"					},
			{ EGL_GREEN_SIZE,				"green_size"				},
			{ EGL_BLUE_SIZE,				"blue_size"					},
			{ EGL_LUMINANCE_SIZE,			"luminance_size"			},
			{ EGL_ALPHA_SIZE,				"alpha_size"				},
			{ EGL_ALPHA_MASK_SIZE,			"alpha_mask_size"			},
			{ EGL_BIND_TO_TEXTURE_RGB,		"bind_to_texture_rgb"		},
			{ EGL_BIND_TO_TEXTURE_RGBA,		"bind_to_texture_rgba"		},
			{ EGL_COLOR_BUFFER_TYPE,		"color_buffer_type"			},
			{ EGL_CONFIG_CAVEAT,			"config_caveat"				},
			{ EGL_CONFIG_ID,				"config_id"					},
			{ EGL_CONFORMANT,				"conformant"				},
			{ EGL_DEPTH_SIZE,				"depth_size"				},
			{ EGL_LEVEL,					"level"						},
			{ EGL_MAX_SWAP_INTERVAL,		"max_swap_interval"			},
			{ EGL_MIN_SWAP_INTERVAL,		"min_swap_interval"			},
			{ EGL_NATIVE_RENDERABLE,		"native_renderable"			},
			{ EGL_NATIVE_VISUAL_TYPE,		"native_visual_type"		},
			{ EGL_RENDERABLE_TYPE,			"renderable_type"			},
			{ EGL_SAMPLE_BUFFERS,			"sample_buffers"			},
			{ EGL_SAMPLES,					"samples"					},
			{ EGL_STENCIL_SIZE,				"stencil_size"				},
			{ EGL_SURFACE_TYPE,				"surface_type"				},
			{ EGL_TRANSPARENT_TYPE,			"transparent_type"			},
			{ EGL_TRANSPARENT_RED_VALUE,	"transparent_red_value"		},
			{ EGL_TRANSPARENT_GREEN_VALUE,	"transparent_green_value"	},
			{ EGL_TRANSPARENT_BLUE_VALUE,	"transparent_blue_value"	}
		};

		tcu::TestCaseGroup* simpleGroup = new tcu::TestCaseGroup(m_testCtx, "get_config_attrib", "eglGetConfigAttrib() tests");
		addChild(simpleGroup);

		for (int ndx = 0; ndx < (int)DE_LENGTH_OF_ARRAY(attributes); ndx++)
		{
			simpleGroup->addChild(new GetConfigAttribSimpleCase(m_eglTestCtx, attributes[ndx].testName, "Simple attribute query case", attributes[ndx].attribute));
		}
	}

	// Attribute constraints
	{
		tcu::TestCaseGroup* constraintsGroup = new tcu::TestCaseGroup(m_testCtx, "constraints", "Attribute constraint tests");
		addChild(constraintsGroup);

		constraintsGroup->addChild(new GetConfigAttribBufferSizeCase(m_eglTestCtx,			"color_buffer_size",	"Color buffer component sizes"));
		constraintsGroup->addChild(new GetConfigAttribTransparentValueCase(m_eglTestCtx,	"transparent_value",	"Transparent color value"));
	}
}

} // egl
} // deqp
