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
 * \brief Choose config tests.
 *//*--------------------------------------------------------------------*/

#include "teglChooseConfigTests.hpp"
#include "teglChooseConfigReference.hpp"
#include "tcuTestLog.hpp"
#include "egluStrUtil.hpp"
#include "egluUtil.hpp"
#include "eglwLibrary.hpp"
#include "eglwEnums.hpp"
#include "deRandom.hpp"
#include "deStringUtil.hpp"
#include "deUniquePtr.hpp"
#include "deSTLUtil.hpp"

#include <vector>
#include <algorithm>
#include <string>
#include <set>
#include <map>

namespace deqp
{
namespace egl
{

using std::set;
using std::vector;
using std::pair;
using std::string;
using tcu::TestLog;
using eglu::ConfigInfo;
using namespace eglw;

namespace
{

string configListToString (const Library& egl, const EGLDisplay& display, const vector<EGLConfig>& configs)
{
	string str = "";
	for (vector<EGLConfig>::const_iterator cfgIter = configs.begin(); cfgIter != configs.end(); cfgIter++)
	{
		EGLConfig	config		= *cfgIter;
		EGLint		configId	= eglu::getConfigID(egl, display, config);

		if (str.length() != 0)
			str += " ";

		str += de::toString(configId);
	}
	return str;
}

void logConfigAttrib (TestLog& log, EGLenum attrib, EGLint value)
{
	const std::string	attribStr	= eglu::getConfigAttribName(attrib);

	if (value == EGL_DONT_CARE)
	{
		log << TestLog::Message << "  " << attribStr << ": EGL_DONT_CARE" << TestLog::EndMessage;
		return;
	}

	log << TestLog::Message << "  " << attribStr << ": " << eglu::getConfigAttribValueStr(attrib, value) << TestLog::EndMessage;
}

bool configListEqual (const Library& egl, const EGLDisplay& display, const vector<EGLConfig>& as, const vector<EGLConfig>& bs)
{
	if (as.size() != bs.size())
		return false;

	for (int configNdx = 0; configNdx < (int)as.size(); configNdx++)
	{
		if (as[configNdx] != bs[configNdx])
		{
			// Allow lists to differ if both configs are non-conformant
			const EGLint aCaveat = eglu::getConfigAttribInt(egl, display, as[configNdx], EGL_CONFIG_CAVEAT);
			const EGLint bCaveat = eglu::getConfigAttribInt(egl, display, bs[configNdx], EGL_CONFIG_CAVEAT);

			if (aCaveat != EGL_NON_CONFORMANT_CONFIG || bCaveat != EGL_NON_CONFORMANT_CONFIG)
				return false;
		}
	}

	return true;
}

} // anonymous

class ChooseConfigCase : public TestCase
{
public:
	ChooseConfigCase (EglTestContext& eglTestCtx, const char* name, const char* description, bool checkOrder, const EGLint* attributes)
		: TestCase		(eglTestCtx, name, description)
		, m_checkOrder	(checkOrder)
		, m_display		(EGL_NO_DISPLAY)
	{
		// Parse attributes
		while (attributes[0] != EGL_NONE)
		{
			m_attributes.push_back(std::make_pair((EGLenum)attributes[0], (EGLint)attributes[1]));
			attributes += 2;
		}
	}

	ChooseConfigCase (EglTestContext& eglTestCtx, const char* name, const char* description, bool checkOrder, const std::vector<std::pair<EGLenum, EGLint> >& attributes)
		: TestCase		(eglTestCtx, name, description)
		, m_checkOrder	(checkOrder)
		, m_attributes	(attributes)
		, m_display		(EGL_NO_DISPLAY)
	{
	}

	void init (void)
	{
		DE_ASSERT(m_display == EGL_NO_DISPLAY);
		m_display	= eglu::getAndInitDisplay(m_eglTestCtx.getNativeDisplay());
	}

	void deinit (void)
	{
		m_eglTestCtx.getLibrary().terminate(m_display);
		m_display = EGL_NO_DISPLAY;
	}

	IterateResult iterate (void)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
		executeTest(m_attributes, m_checkOrder);
		return STOP;
	}

protected:
	ChooseConfigCase (EglTestContext& eglTestCtx, const char* name, const char* description, bool checkOrder)
		: TestCase		(eglTestCtx, name, description)
		, m_checkOrder	(checkOrder)
		, m_display		(EGL_NO_DISPLAY)
	{
	}

	void executeTest (const std::vector<std::pair<EGLenum, EGLint> >& attributes, bool checkOrder)
	{
		const Library&	egl	= m_eglTestCtx.getLibrary();
		TestLog&		log	= m_testCtx.getLog();

		// Build attributes for EGL
		vector<EGLint> attribList;
		for (vector<pair<EGLenum, EGLint> >::const_iterator i = attributes.begin(); i != attributes.end(); i++)
		{
			attribList.push_back(i->first);
			attribList.push_back(i->second);
		}
		attribList.push_back(EGL_NONE);

		// Print attribList to log
		log << TestLog::Message << "Attributes:" << TestLog::EndMessage;
		for (vector<pair<EGLenum, EGLint> >::const_iterator i = attributes.begin(); i != attributes.end(); i++)
			logConfigAttrib(log, i->first, i->second);

		std::vector<EGLConfig>	resultConfigs;
		std::vector<EGLConfig>	referenceConfigs;

		// Query from EGL implementation
		{
			EGLint numConfigs = 0;
			EGLU_CHECK_CALL(egl, chooseConfig(m_display, &attribList[0], DE_NULL, 0, &numConfigs));
			resultConfigs.resize(numConfigs);

			if (numConfigs > 0)
				EGLU_CHECK_CALL(egl, chooseConfig(m_display, &attribList[0], &resultConfigs[0], (EGLint)resultConfigs.size(), &numConfigs));
		}

		// Build reference
		chooseConfigReference(egl, m_display, referenceConfigs, attributes);

		log << TestLog::Message << "Expected:\n  " << configListToString(egl, m_display, referenceConfigs) << TestLog::EndMessage;
		log << TestLog::Message << "Got:\n  " << configListToString(egl, m_display, resultConfigs) << TestLog::EndMessage;

		bool isSetMatch		= (set<EGLConfig>(resultConfigs.begin(), resultConfigs.end()) == set<EGLConfig>(referenceConfigs.begin(), referenceConfigs.end()));
		bool isExactMatch	= configListEqual(egl, m_display, resultConfigs, referenceConfigs);
		bool isMatch		= isSetMatch && (checkOrder ? isExactMatch : true);

		if (isMatch)
			log << TestLog::Message << "Pass" << TestLog::EndMessage;
		else if (!isSetMatch)
			log << TestLog::Message << "Fail, configs don't match" << TestLog::EndMessage;
		else if (!isExactMatch)
			log << TestLog::Message << "Fail, got correct configs but in invalid order" << TestLog::EndMessage;

		if (!isMatch)
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
	}

	void fillDontCare (std::vector<std::pair<EGLenum, EGLint> >& attributes)
	{
		static const EGLenum dontCareAttributes[] =
		{
			EGL_TRANSPARENT_TYPE,
			EGL_COLOR_BUFFER_TYPE,
			EGL_RENDERABLE_TYPE,
			EGL_SURFACE_TYPE
		};

		// Fill appropriate unused attributes with EGL_DONT_CARE
		for (int ndx = 0; ndx < DE_LENGTH_OF_ARRAY(dontCareAttributes); ndx++)
		{
			bool found = false;
			for (size_t findNdx = 0; findNdx < attributes.size(); findNdx++)
				if (attributes[findNdx].first == dontCareAttributes[ndx]) found = true;

			if (!found) attributes.push_back(std::make_pair(dontCareAttributes[ndx], EGL_DONT_CARE));
		}
	}

	const bool						m_checkOrder;
	vector<pair<EGLenum, EGLint> >	m_attributes;

	EGLDisplay						m_display;
};

class ChooseConfigSimpleCase : public ChooseConfigCase
{
protected:
	EGLint getValue (EGLenum name)
	{
		static const struct
		{
			EGLenum		name;
			EGLint		value;
		} attributes[] =
		{
			{ EGL_BUFFER_SIZE,				0					},
			{ EGL_RED_SIZE,					0					},
			{ EGL_GREEN_SIZE,				0					},
			{ EGL_BLUE_SIZE,				0					},
			{ EGL_LUMINANCE_SIZE,			0					},
			{ EGL_ALPHA_SIZE,				0					},
			{ EGL_ALPHA_MASK_SIZE,			0					},
			{ EGL_BIND_TO_TEXTURE_RGB,		EGL_DONT_CARE		},
			{ EGL_BIND_TO_TEXTURE_RGBA,		EGL_DONT_CARE		},
			{ EGL_COLOR_BUFFER_TYPE,		EGL_DONT_CARE		},
			{ EGL_CONFIG_CAVEAT,			EGL_DONT_CARE		},
			//{ EGL_CONFIG_ID,				EGL_DONT_CARE		},
			{ EGL_DEPTH_SIZE,				0					},
			{ EGL_LEVEL,					0					},
			{ EGL_MAX_SWAP_INTERVAL,		EGL_DONT_CARE		},
			{ EGL_MIN_SWAP_INTERVAL,		EGL_DONT_CARE		},
			{ EGL_NATIVE_RENDERABLE,		EGL_DONT_CARE		},
			{ EGL_NATIVE_VISUAL_TYPE,		EGL_DONT_CARE		},
			{ EGL_SAMPLE_BUFFERS,			0					},
			{ EGL_SAMPLES,					0					},
			{ EGL_STENCIL_SIZE,				0					},
			{ EGL_TRANSPARENT_TYPE,			EGL_TRANSPARENT_RGB	},
			{ EGL_TRANSPARENT_RED_VALUE,	0					},
			{ EGL_TRANSPARENT_GREEN_VALUE,	0					},
			{ EGL_TRANSPARENT_BLUE_VALUE,	0					},
			{ EGL_CONFORMANT,				EGL_OPENGL_ES_BIT	},
			{ EGL_RENDERABLE_TYPE,			EGL_OPENGL_ES_BIT	},
			{ EGL_SURFACE_TYPE,				EGL_WINDOW_BIT		}
			//{ EGL_CONFORMANT,				EGL_OPENGL_BIT | EGL_OPENGL_ES_BIT | EGL_OPENGL_ES2_BIT | EGL_OPENVG_BIT	},
			//{ EGL_RENDERABLE_TYPE,			EGL_OPENGL_BIT | EGL_OPENGL_ES_BIT | EGL_OPENGL_ES2_BIT | EGL_OPENVG_BIT	},
			//{ EGL_SURFACE_TYPE,				EGL_WINDOW_BIT
			//								| EGL_PIXMAP_BIT
			//								| EGL_PBUFFER_BIT
			//								| EGL_MULTISAMPLE_RESOLVE_BOX_BIT
			//								| EGL_VG_ALPHA_FORMAT_PRE_BIT
			//								| EGL_SWAP_BEHAVIOR_PRESERVED_BIT
			//								| EGL_VG_COLORSPACE_LINEAR_BIT
			//								}
		};

		if (name == EGL_CONFIG_ID)
		{
			de::Random rnd(0);
			vector<EGLConfig> configs = eglu::getConfigs(m_eglTestCtx.getLibrary(), m_display);
			return eglu::getConfigID(m_eglTestCtx.getLibrary(), m_display, configs[rnd.getInt(0, (int)configs.size()-1)]);
		}
		else
		{
			for (int ndx = 0; ndx < DE_LENGTH_OF_ARRAY(attributes); ndx++)
			{
				if (attributes[ndx].name == name)
					return attributes[ndx].value;
			}
		}

		DE_ASSERT(DE_FALSE);
		return EGL_NONE;
	}
public:
	ChooseConfigSimpleCase (EglTestContext& eglTestCtx, const char* name, const char* description, EGLenum attribute, bool checkOrder)
		: ChooseConfigCase(eglTestCtx, name, description, checkOrder)
		, m_attribute(attribute)
	{
	}

	TestCase::IterateResult iterate (void)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

		std::vector<std::pair<EGLenum, EGLint> > attributes;
		attributes.push_back(std::pair<EGLenum, EGLint>(m_attribute, getValue(m_attribute)));

		fillDontCare(attributes);
		executeTest(attributes, m_checkOrder);

		return STOP;
	}
private:
	EGLenum	m_attribute;
};

class ChooseConfigRandomCase : public ChooseConfigCase
{
public:
	ChooseConfigRandomCase (EglTestContext& eglTestCtx, const char* name, const char* description, const set<EGLenum>& attribSet)
		: ChooseConfigCase	(eglTestCtx, name, description, true)
		, m_attribSet		(attribSet)
		, m_numIters		(10)
		, m_iterNdx			(0)
	{
	}

	void init (void)
	{
		ChooseConfigCase::init();
		m_iterNdx = 0;
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
	}

	TestCase::IterateResult iterate (void)
	{
		m_testCtx.getLog() << TestLog::Message << "Iteration :" << m_iterNdx << TestLog::EndMessage;
		m_iterNdx += 1;

		// Build random list of attributes
		de::Random									rnd(m_iterNdx);
		const int									numAttribs	= rnd.getInt(0, (int)m_attribSet.size()*2);

		std::vector<std::pair<EGLenum, EGLint> >	attributes	= genRandomAttributes(m_attribSet, numAttribs, rnd);

		fillDontCare(attributes);
		executeTest(attributes, m_checkOrder);

		return m_iterNdx < m_numIters ? CONTINUE : STOP;
	}

	template <int MinVal, int MaxVal> static EGLint getInt (de::Random& rnd)
	{
		return rnd.getInt(MinVal, MaxVal);
	}

	static EGLint getBool (de::Random& rnd)
	{
		return rnd.getBool() ? EGL_TRUE : EGL_FALSE;
	}

	static EGLint getBufferType (de::Random& rnd)
	{
		static const EGLint types[] = { EGL_RGB_BUFFER, EGL_LUMINANCE_BUFFER };
		return rnd.choose<EGLint>(types, types+DE_LENGTH_OF_ARRAY(types));
	}

	static EGLint getConfigCaveat (de::Random& rnd)
	{
		static const EGLint caveats[] = { EGL_SLOW_CONFIG, EGL_NON_CONFORMANT_CONFIG };
		return rnd.choose<EGLint>(caveats, caveats+DE_LENGTH_OF_ARRAY(caveats));
	}

	static EGLint getApiBits (de::Random& rnd)
	{
		EGLint api = 0;
		api |= rnd.getBool() ? EGL_OPENGL_BIT		: 0;
		api |= rnd.getBool() ? EGL_OPENGL_ES_BIT	: 0;
		api |= rnd.getBool() ? EGL_OPENGL_ES2_BIT	: 0;
		api |= rnd.getBool() ? EGL_OPENVG_BIT		: 0;
		return api;
	}

	static EGLint getSurfaceType (de::Random& rnd)
	{
		EGLint bits = 0;
		bits |= rnd.getBool() ? EGL_WINDOW_BIT	: 0;
		bits |= rnd.getBool() ? EGL_PIXMAP_BIT	: 0;
		bits |= rnd.getBool() ? EGL_PBUFFER_BIT	: 0;
		return bits;
	}

	struct AttribSpec
	{
		EGLenum			attribute;
		EGLint			(*getValue)(de::Random& rnd);
	};

	std::vector<std::pair<EGLenum, EGLint> > genRandomAttributes (const std::set<EGLenum>& attribSet, int numAttribs, de::Random& rnd)
	{
		static const struct AttribSpec attributes[] =
		{
			{ EGL_BUFFER_SIZE,				ChooseConfigRandomCase::getInt<0, 32>,		},
			{ EGL_RED_SIZE,					ChooseConfigRandomCase::getInt<0, 8>,		},
			{ EGL_GREEN_SIZE,				ChooseConfigRandomCase::getInt<0, 8>,		},
			{ EGL_BLUE_SIZE,				ChooseConfigRandomCase::getInt<0, 8>,		},
			{ EGL_LUMINANCE_SIZE,			ChooseConfigRandomCase::getInt<0, 1>,		},
			{ EGL_ALPHA_SIZE,				ChooseConfigRandomCase::getInt<0, 8>,		},
			{ EGL_ALPHA_MASK_SIZE,			ChooseConfigRandomCase::getInt<0, 1>,		},
			{ EGL_BIND_TO_TEXTURE_RGB,		ChooseConfigRandomCase::getBool,			},
			{ EGL_BIND_TO_TEXTURE_RGBA,		ChooseConfigRandomCase::getBool,			},
			{ EGL_COLOR_BUFFER_TYPE,		ChooseConfigRandomCase::getBufferType,		},
			{ EGL_CONFIG_CAVEAT,			ChooseConfigRandomCase::getConfigCaveat,	},
//			{ EGL_CONFIG_ID,				0/*special*/,		},
			{ EGL_CONFORMANT,				ChooseConfigRandomCase::getApiBits,			},
			{ EGL_DEPTH_SIZE,				ChooseConfigRandomCase::getInt<0, 32>,		},
			{ EGL_LEVEL,					ChooseConfigRandomCase::getInt<0, 1>,		},
//			{ EGL_MATCH_NATIVE_PIXMAP,		EGL_NONE,			},
			{ EGL_MAX_SWAP_INTERVAL,		ChooseConfigRandomCase::getInt<0, 2>,		},
			{ EGL_MIN_SWAP_INTERVAL,		ChooseConfigRandomCase::getInt<0, 1>,		},
			{ EGL_NATIVE_RENDERABLE,		ChooseConfigRandomCase::getBool,			},
//			{ EGL_NATIVE_VISUAL_TYPE,		EGL_DONT_CARE,		},
			{ EGL_RENDERABLE_TYPE,			ChooseConfigRandomCase::getApiBits,			},
			{ EGL_SAMPLE_BUFFERS,			ChooseConfigRandomCase::getInt<0, 1>,		},
			{ EGL_SAMPLES,					ChooseConfigRandomCase::getInt<0, 1>,		},
			{ EGL_STENCIL_SIZE,				ChooseConfigRandomCase::getInt<0, 1>,		},
			{ EGL_SURFACE_TYPE,				ChooseConfigRandomCase::getSurfaceType,		},
//			{ EGL_TRANSPARENT_TYPE,			EGL_TRANSPARENT_RGB,},
//			{ EGL_TRANSPARENT_RED_VALUE,	ChooseConfigRandomCase::getInt<0, 255>,		},
//			{ EGL_TRANSPARENT_GREEN_VALUE,	ChooseConfigRandomCase::getInt<0, 255>,		},
//			{ EGL_TRANSPARENT_BLUE_VALUE,	ChooseConfigRandomCase::getInt<0, 255>,		}
		};

		std::vector<std::pair<EGLenum, EGLint> > out;

		// Build list to select from
		std::vector<AttribSpec> candidates;
		for (int ndx = 0; ndx < (int)DE_LENGTH_OF_ARRAY(attributes); ndx++)
		{
			if (attribSet.find(attributes[ndx].attribute) != attribSet.end())
				candidates.push_back(attributes[ndx]);
		}

		for (int attribNdx = 0; attribNdx < numAttribs; attribNdx++)
		{
			AttribSpec spec = rnd.choose<AttribSpec>(candidates.begin(), candidates.end());
			out.push_back(std::make_pair(spec.attribute, spec.getValue(rnd)));
		}

		return out;
	}
private:
	std::set<EGLenum>	m_attribSet;
	int					m_numIters;
	int					m_iterNdx;
};

class ColorComponentTypeCase : public ChooseConfigCase
{

public:
	ColorComponentTypeCase (EglTestContext& eglTestCtx, const char* name, EGLenum value)
		: ChooseConfigCase	(eglTestCtx, name, "", true /* sorting order is validated */)
		, m_value			(value)
	{
	}

	TestCase::IterateResult iterate (void)
	{
		m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");

		{
			const std::vector<std::string>	extensions	= eglu::getDisplayExtensions(m_eglTestCtx.getLibrary(), m_display);

			if (!de::contains(extensions.begin(), extensions.end(), "EGL_EXT_pixel_format_float"))
				TCU_THROW(NotSupportedError, "EGL_EXT_pixel_format_float is not supported");
		}

		{
			std::vector<std::pair<EGLenum, EGLint> > attributes;

			attributes.push_back(std::pair<EGLenum, EGLint>(EGL_COLOR_COMPONENT_TYPE_EXT, m_value));
			fillDontCare(attributes);

			executeTest(attributes, m_checkOrder);
		}

		return STOP;
	}
private:
	const EGLenum	m_value;
};

ChooseConfigTests::ChooseConfigTests (EglTestContext& eglTestCtx)
	: TestCaseGroup(eglTestCtx, "choose_config", "eglChooseConfig() tests")
{
}

ChooseConfigTests::~ChooseConfigTests (void)
{
}

namespace
{

template <typename T, size_t N>
std::set<T> toSet (const T (&arr)[N])
{
	std::set<T> set;
	for (size_t i = 0; i < N; i++)
		set.insert(arr[i]);
	return set;
}

} // anonymous

void ChooseConfigTests::init (void)
{
	// Single attributes
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

		tcu::TestCaseGroup* simpleGroup = new tcu::TestCaseGroup(m_testCtx, "simple", "Simple tests");
		addChild(simpleGroup);

		tcu::TestCaseGroup* selectionGroup = new tcu::TestCaseGroup(m_testCtx, "selection_only", "Selection tests, order ignored");
		simpleGroup->addChild(selectionGroup);

		tcu::TestCaseGroup* sortGroup = new tcu::TestCaseGroup(m_testCtx, "selection_and_sort", "Selection and ordering tests");
		simpleGroup->addChild(sortGroup);

		for (int ndx = 0; ndx < (int)DE_LENGTH_OF_ARRAY(attributes); ndx++)
		{
			selectionGroup->addChild(new ChooseConfigSimpleCase(m_eglTestCtx, attributes[ndx].testName, "Simple config selection case", attributes[ndx].attribute, false));
			sortGroup->addChild(new ChooseConfigSimpleCase(m_eglTestCtx, attributes[ndx].testName, "Simple config selection and sort case", attributes[ndx].attribute, true));
		}
	}

	// Random
	{
		tcu::TestCaseGroup* randomGroup = new tcu::TestCaseGroup(m_testCtx, "random", "Random eglChooseConfig() usage");
		addChild(randomGroup);

		static const EGLenum rgbaSizes[] =
		{
			EGL_RED_SIZE,
			EGL_GREEN_SIZE,
			EGL_BLUE_SIZE,
			EGL_ALPHA_SIZE
		};
		randomGroup->addChild(new ChooseConfigRandomCase(m_eglTestCtx, "color_sizes", "Random color size rules", toSet(rgbaSizes)));

		static const EGLenum colorDepthStencilSizes[] =
		{
			EGL_RED_SIZE,
			EGL_GREEN_SIZE,
			EGL_BLUE_SIZE,
			EGL_ALPHA_SIZE,
			EGL_DEPTH_SIZE,
			EGL_STENCIL_SIZE
		};
		randomGroup->addChild(new ChooseConfigRandomCase(m_eglTestCtx, "color_depth_stencil_sizes", "Random color, depth and stencil size rules", toSet(colorDepthStencilSizes)));

		static const EGLenum bufferSizes[] =
		{
			EGL_BUFFER_SIZE,
			EGL_LUMINANCE_SIZE,
			EGL_ALPHA_MASK_SIZE,
			EGL_DEPTH_SIZE,
			EGL_STENCIL_SIZE
		};
		randomGroup->addChild(new ChooseConfigRandomCase(m_eglTestCtx, "buffer_sizes", "Various buffer size rules", toSet(bufferSizes)));

		static const EGLenum surfaceType[] =
		{
			EGL_NATIVE_RENDERABLE,
			EGL_SURFACE_TYPE
		};
		randomGroup->addChild(new ChooseConfigRandomCase(m_eglTestCtx, "surface_type", "Surface type rules", toSet(surfaceType)));

		static const EGLenum sampleBuffers[] =
		{
			EGL_SAMPLE_BUFFERS,
			EGL_SAMPLES
		};
		randomGroup->addChild(new ChooseConfigRandomCase(m_eglTestCtx, "sample_buffers", "Sample buffer rules", toSet(sampleBuffers)));

		// \note Not every attribute is supported at the moment
		static const EGLenum allAttribs[] =
		{
			EGL_BUFFER_SIZE,
			EGL_RED_SIZE,
			EGL_GREEN_SIZE,
			EGL_BLUE_SIZE,
			EGL_ALPHA_SIZE,
			EGL_ALPHA_MASK_SIZE,
			EGL_BIND_TO_TEXTURE_RGB,
			EGL_BIND_TO_TEXTURE_RGBA,
			EGL_COLOR_BUFFER_TYPE,
			EGL_CONFIG_CAVEAT,
			EGL_CONFIG_ID,
			EGL_CONFORMANT,
			EGL_DEPTH_SIZE,
			EGL_LEVEL,
//			EGL_MATCH_NATIVE_PIXMAP,
			EGL_MAX_SWAP_INTERVAL,
			EGL_MIN_SWAP_INTERVAL,
			EGL_NATIVE_RENDERABLE,
			EGL_NATIVE_VISUAL_TYPE,
			EGL_RENDERABLE_TYPE,
			EGL_SAMPLE_BUFFERS,
			EGL_SAMPLES,
			EGL_STENCIL_SIZE,
			EGL_SURFACE_TYPE,
			EGL_TRANSPARENT_TYPE,
//			EGL_TRANSPARENT_RED_VALUE,
//			EGL_TRANSPARENT_GREEN_VALUE,
//			EGL_TRANSPARENT_BLUE_VALUE
		};
		randomGroup->addChild(new ChooseConfigRandomCase(m_eglTestCtx, "all", "All attributes", toSet(allAttribs)));
	}

	// EGL_EXT_pixel_format_float
	{
		de::MovePtr<tcu::TestCaseGroup>	colorComponentTypeGroup	(new tcu::TestCaseGroup(m_testCtx, "color_component_type_ext", "EGL_EXT_pixel_format_float tests"));

		colorComponentTypeGroup->addChild(new ColorComponentTypeCase(m_eglTestCtx, "dont_care",	EGL_DONT_CARE));
		colorComponentTypeGroup->addChild(new ColorComponentTypeCase(m_eglTestCtx, "fixed",		EGL_COLOR_COMPONENT_TYPE_FIXED_EXT));
		colorComponentTypeGroup->addChild(new ColorComponentTypeCase(m_eglTestCtx, "float",		EGL_COLOR_COMPONENT_TYPE_FLOAT_EXT));

		addChild(colorComponentTypeGroup.release());
	}
}

} // egl
} // deqp
