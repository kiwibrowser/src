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
 * \brief Simple Context construction test.
 *//*--------------------------------------------------------------------*/

#include "teglSimpleConfigCase.hpp"
#include "tcuTestLog.hpp"
#include "tcuFormatUtil.hpp"
#include "egluUtil.hpp"
#include "eglwLibrary.hpp"
#include "eglwEnums.hpp"
#include "deStringUtil.hpp"

namespace deqp
{
namespace egl
{

using std::vector;
using std::string;
using tcu::TestLog;
using eglu::ConfigInfo;
using namespace eglw;
using namespace eglu;

SimpleConfigCase::SimpleConfigCase (EglTestContext& eglTestCtx, const char* name, const char* description, const FilterList& filters)
	: TestCase	(eglTestCtx, name, description)
	, m_filters	(filters)
	, m_display	(EGL_NO_DISPLAY)
{
}

SimpleConfigCase::~SimpleConfigCase (void)
{
}

void SimpleConfigCase::init (void)
{
	const Library&		egl		= m_eglTestCtx.getLibrary();

	DE_ASSERT(m_display == EGL_NO_DISPLAY && m_configs.empty());

	m_display	= getAndInitDisplay(m_eglTestCtx.getNativeDisplay());
	m_configs	= chooseConfigs(egl, m_display, m_filters);

	// Log matching configs.
	{
		vector<EGLint> configIds(m_configs.size());

		for (size_t ndx = 0; ndx < m_configs.size(); ndx++)
			configIds[ndx] = getConfigID(egl, m_display, m_configs[ndx]);

		m_testCtx.getLog() << TestLog::Message << "Compatible configs: " << tcu::formatArray(configIds.begin(), configIds.end()) << TestLog::EndMessage;
	}

	if (m_configs.empty())
	{
		egl.terminate(m_display);
		m_display = EGL_NO_DISPLAY;
		TCU_THROW(NotSupportedError, "No compatible configs found");
	}

	// Init config iter
	m_configIter = m_configs.begin();

	// Init test case result to Pass
	m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
}

void SimpleConfigCase::deinit (void)
{
	if (m_display != EGL_NO_DISPLAY)
	{
		m_eglTestCtx.getLibrary().terminate(m_display);
		m_display = EGL_NO_DISPLAY;
	}
	m_configs.clear();
}

SimpleConfigCase::IterateResult SimpleConfigCase::iterate (void)
{
	DE_ASSERT(m_configIter != m_configs.end());

	EGLConfig	config	= *m_configIter++;

	try
	{
		executeForConfig(m_display, config);
	}
	catch (const tcu::TestError& e)
	{
		m_testCtx.getLog() << e;
		m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Fail");
	}
	// \note Other errors are handled by framework (resource / internal errors).

	return (m_configIter != m_configs.end()) ? CONTINUE : STOP;
}

template <int Red, int Green, int Blue, int Alpha>
static bool colorBits (const eglu::CandidateConfig& c)
{
	return c.redSize()		== Red		&&
		   c.greenSize()	== Green	&&
		   c.blueSize()		== Blue		&&
		   c.alphaSize()	== Alpha;
}

template <int Red, int Green, int Blue, int Alpha>
static bool notColorBits (const eglu::CandidateConfig& c)
{
	return c.redSize()		!= Red		||
		   c.greenSize()	!= Green	||
		   c.blueSize()		!= Blue		||
		   c.alphaSize()	!= Alpha;
}

static bool	hasDepth	(const eglu::CandidateConfig& c)	{ return c.depthSize() > 0;		}
static bool	noDepth		(const eglu::CandidateConfig& c)	{ return c.depthSize() == 0;	}
static bool	hasStencil	(const eglu::CandidateConfig& c)	{ return c.stencilSize() > 0;	}
static bool	noStencil	(const eglu::CandidateConfig& c)	{ return c.stencilSize() == 0;	}

static bool isConformant (const eglu::CandidateConfig& c)
{
	return c.get(EGL_CONFIG_CAVEAT) != EGL_NON_CONFORMANT_CONFIG;
}

static bool notFloat (const eglu::CandidateConfig& c)
{
	return c.colorComponentType() != EGL_COLOR_COMPONENT_TYPE_FLOAT_EXT;
}

void getDefaultFilterLists (vector<NamedFilterList>& lists, const FilterList& baseFilters)
{
	static const struct
	{
		const char*			name;
		eglu::ConfigFilter	filter;
	} s_colorRules[] =
	{
		{ "rgb565",		colorBits<5, 6, 5, 0> },
		{ "rgb888",		colorBits<8, 8, 8, 0> },
		{ "rgba4444",	colorBits<4, 4, 4, 4> },
		{ "rgba5551",	colorBits<5, 5, 5, 1> },
		{ "rgba8888",	colorBits<8, 8, 8, 8> }
	};

	static const struct
	{
		const char*			name;
		eglu::ConfigFilter	filter;
	} s_depthRules[] =
	{
		{ "no_depth",	noDepth		},
		{ "depth",		hasDepth	},
	};

	static const struct
	{
		const char*			name;
		eglu::ConfigFilter	filter;
	} s_stencilRules[] =
	{
		{ "no_stencil",	noStencil	},
		{ "stencil",	hasStencil	},
	};

	for (int colorRuleNdx = 0; colorRuleNdx < DE_LENGTH_OF_ARRAY(s_colorRules); colorRuleNdx++)
	{
		for (int depthRuleNdx = 0; depthRuleNdx < DE_LENGTH_OF_ARRAY(s_depthRules); depthRuleNdx++)
		{
			for (int stencilRuleNdx = 0; stencilRuleNdx < DE_LENGTH_OF_ARRAY(s_stencilRules); stencilRuleNdx++)
			{
				const string		name		= string(s_colorRules[colorRuleNdx].name) + "_" + s_depthRules[depthRuleNdx].name + "_" + s_stencilRules[stencilRuleNdx].name;
				NamedFilterList		filters		(name.c_str(), "");

				filters << baseFilters
						<< s_colorRules[colorRuleNdx].filter
						<< s_depthRules[depthRuleNdx].filter
						<< s_stencilRules[stencilRuleNdx].filter
						<< isConformant;

				lists.push_back(filters);
			}
		}
	}

	// Build "other" set - not configs that don't match any of known color rules
	{
		NamedFilterList		filters		("other", "All other configs");

		// \todo [2014-12-18 pyry] Optimize rules
		filters << baseFilters
				<< notColorBits<5, 6, 5, 0>
				<< notColorBits<8, 8, 8, 0>
				<< notColorBits<4, 4, 4, 4>
				<< notColorBits<5, 5, 5, 1>
				<< notColorBits<8, 8, 8, 8>
				<< isConformant
				<< notFloat;

		lists.push_back(filters);
	}
}

} // egl
} // deqp
