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
 * \brief Choose config reference implementation.
 *//*--------------------------------------------------------------------*/

#include "teglChooseConfigReference.hpp"

#include "egluUtil.hpp"
#include "egluConfigInfo.hpp"
#include "egluStrUtil.hpp"
#include "eglwLibrary.hpp"
#include "eglwEnums.hpp"

#include "deSTLUtil.hpp"

#include <algorithm>
#include <vector>
#include <map>

namespace deqp
{
namespace egl
{

using namespace eglw;
using eglu::ConfigInfo;

enum Criteria
{
	CRITERIA_AT_LEAST = 0,
	CRITERIA_EXACT,
	CRITERIA_MASK,
	CRITERIA_SPECIAL,

	CRITERIA_LAST
};

enum SortOrder
{
	SORTORDER_NONE	= 0,
	SORTORDER_SMALLER,
	SORTORDER_SPECIAL,

	SORTORDER_LAST
};

struct AttribRule
{
	EGLenum		name;
	EGLint		value;
	Criteria	criteria;
	SortOrder	sortOrder;

	AttribRule (void)
		: name			(EGL_NONE)
		, value			(EGL_NONE)
		, criteria		(CRITERIA_LAST)
		, sortOrder		(SORTORDER_LAST)
	{
	}

	AttribRule (EGLenum name_, EGLint value_, Criteria criteria_, SortOrder sortOrder_)
		: name			(name_)
		, value			(value_)
		, criteria		(criteria_)
		, sortOrder		(sortOrder_)
	{
	}
};

class SurfaceConfig
{
private:
	static int getCaveatRank (EGLenum caveat)
	{
		switch (caveat)
		{
			case EGL_NONE:					return 0;
			case EGL_SLOW_CONFIG:			return 1;
			case EGL_NON_CONFORMANT_CONFIG:	return 2;
			default:
				TCU_THROW(TestError, (std::string("Unknown config caveat: ") + eglu::getConfigCaveatStr(caveat).toString()).c_str());
		}
	}

	static int getColorBufferTypeRank (EGLenum type)
	{
		switch (type)
		{
			case EGL_RGB_BUFFER:			return 0;
			case EGL_LUMINANCE_BUFFER:		return 1;
			case EGL_YUV_BUFFER_EXT:		return 2;
			default:
				TCU_THROW(TestError, (std::string("Unknown color buffer type: ") + eglu::getColorBufferTypeStr(type).toString()).c_str());
		}
	}

	static int getYuvOrderRank (EGLenum order)
	{
		switch (order)
		{
			case EGL_NONE:					return 0;
			case EGL_YUV_ORDER_YUV_EXT:		return 1;
			case EGL_YUV_ORDER_YVU_EXT:		return 2;
			case EGL_YUV_ORDER_YUYV_EXT:	return 3;
			case EGL_YUV_ORDER_YVYU_EXT:	return 4;
			case EGL_YUV_ORDER_UYVY_EXT:	return 5;
			case EGL_YUV_ORDER_VYUY_EXT:	return 6;
			case EGL_YUV_ORDER_AYUV_EXT:	return 7;
			default:
				TCU_THROW(TestError, (std::string("Unknown YUV order: ") + eglu::getYuvOrderStr(order).toString()).c_str());
		}
	}

	static int getYuvPlaneBppValue (EGLenum bpp)
	{
		switch (bpp)
		{
			case EGL_YUV_PLANE_BPP_0_EXT:	return 0;
			case EGL_YUV_PLANE_BPP_8_EXT:	return 8;
			case EGL_YUV_PLANE_BPP_10_EXT:	return 10;
			default:
				TCU_THROW(TestError, (std::string("Unknown YUV plane BPP: ") + eglu::getYuvPlaneBppStr(bpp).toString()).c_str());
		}
	}

	static int getColorComponentTypeRank (EGLenum compType)
	{
		switch (compType)
		{
			case EGL_COLOR_COMPONENT_TYPE_FIXED_EXT:	return 0;
			case EGL_COLOR_COMPONENT_TYPE_FLOAT_EXT:	return 1;
			default:
				TCU_THROW(TestError, (std::string("Unknown color component type: ") + eglu::getColorComponentTypeStr(compType).toString()).c_str());
		}
	}

	typedef bool (*CompareFunc) (const SurfaceConfig& a, const SurfaceConfig& b);

	static bool compareCaveat (const SurfaceConfig& a, const SurfaceConfig& b)
	{
		return getCaveatRank((EGLenum)a.m_info.configCaveat) < getCaveatRank((EGLenum)b.m_info.configCaveat);
	}

	static bool compareColorBufferType (const SurfaceConfig& a, const SurfaceConfig& b)
	{
		return getColorBufferTypeRank((EGLenum)a.m_info.colorBufferType) < getColorBufferTypeRank((EGLenum)b.m_info.colorBufferType);
	}

	static bool compareYuvOrder (const SurfaceConfig& a, const SurfaceConfig& b)
	{
		return getYuvOrderRank((EGLenum)a.m_info.yuvOrder) < getYuvOrderRank((EGLenum)b.m_info.yuvOrder);
	}

	static bool compareColorComponentType (const SurfaceConfig& a, const SurfaceConfig& b)
	{
		return getColorComponentTypeRank((EGLenum)a.m_info.colorComponentType) < getColorComponentTypeRank((EGLenum)b.m_info.colorComponentType);
	}

	static bool compareColorBufferBits (const SurfaceConfig& a, const SurfaceConfig& b, const tcu::BVec4& specifiedRGBColors, const tcu::BVec2& specifiedLuminanceColors, bool yuvPlaneBppSpecified)
	{
		DE_ASSERT(a.m_info.colorBufferType == b.m_info.colorBufferType);
		switch (a.m_info.colorBufferType)
		{
			case EGL_RGB_BUFFER:
			{
				const tcu::IVec4	mask	(specifiedRGBColors.cast<deInt32>());

				return (a.m_info.redSize * mask[0] + a.m_info.greenSize * mask[1] + a.m_info.blueSize * mask[2] + a.m_info.alphaSize * mask[3])
						> (b.m_info.redSize * mask[0] + b.m_info.greenSize * mask[1] + b.m_info.blueSize * mask[2] + b.m_info.alphaSize * mask[3]);
			}

			case EGL_LUMINANCE_BUFFER:
			{
				const tcu::IVec2	mask	(specifiedLuminanceColors.cast<deInt32>());

				return (a.m_info.luminanceSize * mask[0] + a.m_info.alphaSize * mask[1]) > (b.m_info.luminanceSize * mask[0] + b.m_info.alphaSize * mask[1]);
			}

			case EGL_YUV_BUFFER_EXT:
				return yuvPlaneBppSpecified ? (a.m_info.yuvPlaneBpp > b.m_info.yuvPlaneBpp) : false;

			default:
				DE_ASSERT(DE_FALSE);
				return true;
		}
	}

	template <EGLenum Attribute>
	static bool compareAttributeSmaller (const SurfaceConfig& a, const SurfaceConfig& b)
	{
		return a.getAttribute(Attribute) < b.getAttribute(Attribute);
	}
public:
	SurfaceConfig (EGLConfig config, ConfigInfo &info)
		: m_config(config)
		, m_info(info)
	{
	}

	EGLConfig getEglConfig (void) const
	{
		return m_config;
	}

	EGLint getAttribute (const EGLenum attribute) const
	{
		return m_info.getAttribute(attribute);
	}

	friend bool operator== (const SurfaceConfig& a, const SurfaceConfig& b)
	{
		const std::map<EGLenum, AttribRule> defaultRules = getDefaultRules();

		for (std::map<EGLenum, AttribRule>::const_iterator iter = defaultRules.begin(); iter != defaultRules.end(); iter++)
		{
			const EGLenum attribute = iter->first;

			if (a.getAttribute(attribute) != b.getAttribute(attribute)) return false;
		}
		return true;
	}

	bool compareTo (const SurfaceConfig& b, const tcu::BVec4& specifiedRGBColors, const tcu::BVec2& specifiedLuminanceColors, bool yuvPlaneBppSpecified) const
	{
		static const SurfaceConfig::CompareFunc compareFuncs[] =
		{
			SurfaceConfig::compareCaveat,
			SurfaceConfig::compareColorBufferType,
			SurfaceConfig::compareColorComponentType,
			DE_NULL, // SurfaceConfig::compareColorBufferBits,
			SurfaceConfig::compareAttributeSmaller<EGL_BUFFER_SIZE>,
			SurfaceConfig::compareAttributeSmaller<EGL_SAMPLE_BUFFERS>,
			SurfaceConfig::compareAttributeSmaller<EGL_SAMPLES>,
			SurfaceConfig::compareAttributeSmaller<EGL_DEPTH_SIZE>,
			SurfaceConfig::compareAttributeSmaller<EGL_STENCIL_SIZE>,
			SurfaceConfig::compareAttributeSmaller<EGL_ALPHA_MASK_SIZE>,
			SurfaceConfig::compareYuvOrder,
			SurfaceConfig::compareAttributeSmaller<EGL_CONFIG_ID>
		};

		if (*this == b)
			return false; // std::sort() can compare object to itself.

		for (int ndx = 0; ndx < (int)DE_LENGTH_OF_ARRAY(compareFuncs); ndx++)
		{
			if (!compareFuncs[ndx])
			{
				if (compareColorBufferBits(*this, b, specifiedRGBColors, specifiedLuminanceColors, yuvPlaneBppSpecified))
					return true;
				else if (compareColorBufferBits(b, *this, specifiedRGBColors, specifiedLuminanceColors, yuvPlaneBppSpecified))
					return false;

				continue;
			}

			if (compareFuncs[ndx](*this, b))
				return true;
			else if (compareFuncs[ndx](b, *this))
				return false;
		}

		TCU_FAIL("Unable to compare configs - duplicate ID?");
	}

	static std::map<EGLenum, AttribRule> getDefaultRules (void)
	{
		// \todo [2011-03-24 pyry] From EGL 1.4 spec - check that this is valid for other versions as well
		std::map<EGLenum, AttribRule> rules;

		//									Attribute									Default				Selection Criteria	Sort Order			Sort Priority
		rules[EGL_BUFFER_SIZE]				= AttribRule(EGL_BUFFER_SIZE,				0,					CRITERIA_AT_LEAST,	SORTORDER_SMALLER);	//	4
		rules[EGL_RED_SIZE]					= AttribRule(EGL_RED_SIZE,					0,					CRITERIA_AT_LEAST,	SORTORDER_SPECIAL);	//	3
		rules[EGL_GREEN_SIZE]				= AttribRule(EGL_GREEN_SIZE,				0,					CRITERIA_AT_LEAST,	SORTORDER_SPECIAL);	//	3
		rules[EGL_BLUE_SIZE]				= AttribRule(EGL_BLUE_SIZE,					0,					CRITERIA_AT_LEAST,	SORTORDER_SPECIAL);	//	3
		rules[EGL_LUMINANCE_SIZE]			= AttribRule(EGL_LUMINANCE_SIZE,			0,					CRITERIA_AT_LEAST,	SORTORDER_SPECIAL);	//	3
		rules[EGL_ALPHA_SIZE]				= AttribRule(EGL_ALPHA_SIZE,				0,					CRITERIA_AT_LEAST,	SORTORDER_SPECIAL);	//	3
		rules[EGL_ALPHA_MASK_SIZE]			= AttribRule(EGL_ALPHA_MASK_SIZE,			0,					CRITERIA_AT_LEAST,	SORTORDER_SMALLER);	//	9
		rules[EGL_BIND_TO_TEXTURE_RGB]		= AttribRule(EGL_BIND_TO_TEXTURE_RGB,		EGL_DONT_CARE,		CRITERIA_EXACT,		SORTORDER_NONE);
		rules[EGL_BIND_TO_TEXTURE_RGBA]		= AttribRule(EGL_BIND_TO_TEXTURE_RGBA,		EGL_DONT_CARE,		CRITERIA_EXACT,		SORTORDER_NONE);
		rules[EGL_COLOR_BUFFER_TYPE]		= AttribRule(EGL_COLOR_BUFFER_TYPE,			EGL_RGB_BUFFER,		CRITERIA_EXACT,		SORTORDER_NONE);	//	2
		rules[EGL_CONFIG_CAVEAT]			= AttribRule(EGL_CONFIG_CAVEAT,				EGL_DONT_CARE,		CRITERIA_EXACT,		SORTORDER_SPECIAL);	//	1
		rules[EGL_CONFIG_ID]				= AttribRule(EGL_CONFIG_ID,					EGL_DONT_CARE,		CRITERIA_EXACT,		SORTORDER_SMALLER);	//	11
		rules[EGL_CONFORMANT]				= AttribRule(EGL_CONFORMANT,				0,					CRITERIA_MASK,		SORTORDER_NONE);
		rules[EGL_DEPTH_SIZE]				= AttribRule(EGL_DEPTH_SIZE,				0,					CRITERIA_AT_LEAST,	SORTORDER_SMALLER);	//	7
		rules[EGL_LEVEL]					= AttribRule(EGL_LEVEL,						0,					CRITERIA_EXACT,		SORTORDER_NONE);
		rules[EGL_MAX_SWAP_INTERVAL]		= AttribRule(EGL_MAX_SWAP_INTERVAL,			EGL_DONT_CARE,		CRITERIA_EXACT,		SORTORDER_NONE);
		rules[EGL_MIN_SWAP_INTERVAL]		= AttribRule(EGL_MIN_SWAP_INTERVAL,			EGL_DONT_CARE,		CRITERIA_EXACT,		SORTORDER_NONE);
		rules[EGL_NATIVE_RENDERABLE]		= AttribRule(EGL_NATIVE_RENDERABLE,			EGL_DONT_CARE,		CRITERIA_EXACT,		SORTORDER_NONE);
		rules[EGL_NATIVE_VISUAL_TYPE]		= AttribRule(EGL_NATIVE_VISUAL_TYPE,		EGL_DONT_CARE,		CRITERIA_EXACT,		SORTORDER_SPECIAL);	//	10
		rules[EGL_RENDERABLE_TYPE]			= AttribRule(EGL_RENDERABLE_TYPE,			EGL_OPENGL_ES_BIT,	CRITERIA_MASK,		SORTORDER_NONE);
		rules[EGL_SAMPLE_BUFFERS]			= AttribRule(EGL_SAMPLE_BUFFERS,			0,					CRITERIA_AT_LEAST,	SORTORDER_SMALLER);	//	5
		rules[EGL_SAMPLES]					= AttribRule(EGL_SAMPLES,					0,					CRITERIA_AT_LEAST,	SORTORDER_SMALLER);	//	6
		rules[EGL_STENCIL_SIZE]				= AttribRule(EGL_STENCIL_SIZE,				0,					CRITERIA_AT_LEAST,	SORTORDER_SMALLER);	//	8
		rules[EGL_SURFACE_TYPE]				= AttribRule(EGL_SURFACE_TYPE,				EGL_WINDOW_BIT,		CRITERIA_MASK,		SORTORDER_NONE);
		rules[EGL_TRANSPARENT_TYPE]			= AttribRule(EGL_TRANSPARENT_TYPE,			EGL_NONE,			CRITERIA_EXACT,		SORTORDER_NONE);
		rules[EGL_TRANSPARENT_RED_VALUE]	= AttribRule(EGL_TRANSPARENT_RED_VALUE,		EGL_DONT_CARE,		CRITERIA_EXACT,		SORTORDER_NONE);
		rules[EGL_TRANSPARENT_GREEN_VALUE]	= AttribRule(EGL_TRANSPARENT_GREEN_VALUE,	EGL_DONT_CARE,		CRITERIA_EXACT,		SORTORDER_NONE);
		rules[EGL_TRANSPARENT_BLUE_VALUE]	= AttribRule(EGL_TRANSPARENT_BLUE_VALUE,	EGL_DONT_CARE,		CRITERIA_EXACT,		SORTORDER_NONE);

		// EGL_EXT_yuv_surface
		rules[EGL_YUV_ORDER_EXT]			= AttribRule(EGL_YUV_ORDER_EXT,				EGL_DONT_CARE,		CRITERIA_EXACT,		SORTORDER_SPECIAL);
		rules[EGL_YUV_NUMBER_OF_PLANES_EXT]	= AttribRule(EGL_YUV_NUMBER_OF_PLANES_EXT,	EGL_DONT_CARE,		CRITERIA_EXACT,		SORTORDER_NONE);
		rules[EGL_YUV_SUBSAMPLE_EXT]		= AttribRule(EGL_YUV_SUBSAMPLE_EXT,			EGL_DONT_CARE,		CRITERIA_EXACT,		SORTORDER_NONE);
		rules[EGL_YUV_DEPTH_RANGE_EXT]		= AttribRule(EGL_YUV_DEPTH_RANGE_EXT,		EGL_DONT_CARE,		CRITERIA_EXACT,		SORTORDER_NONE);
		rules[EGL_YUV_CSC_STANDARD_EXT]		= AttribRule(EGL_YUV_CSC_STANDARD_EXT,		EGL_DONT_CARE,		CRITERIA_EXACT,		SORTORDER_NONE);
		rules[EGL_YUV_PLANE_BPP_EXT]		= AttribRule(EGL_YUV_PLANE_BPP_EXT,			EGL_DONT_CARE,		CRITERIA_AT_LEAST,	SORTORDER_SPECIAL);	//	3

		// EGL_EXT_pixel_format_float
		rules[EGL_COLOR_COMPONENT_TYPE_EXT]	= AttribRule(EGL_COLOR_COMPONENT_TYPE_EXT,	EGL_COLOR_COMPONENT_TYPE_FIXED_EXT,		CRITERIA_EXACT,		SORTORDER_SPECIAL);	//	2

		return rules;
	}
private:
	EGLConfig m_config;
	ConfigInfo m_info;
};

class CompareConfigs
{
public:
	CompareConfigs (const tcu::BVec4& specifiedRGBColors, const tcu::BVec2& specifiedLuminanceColors, bool yuvPlaneBppSpecified)
		: m_specifiedRGBColors			(specifiedRGBColors)
		, m_specifiedLuminanceColors	(specifiedLuminanceColors)
		, m_yuvPlaneBppSpecified		(yuvPlaneBppSpecified)
	{
	}

	bool operator() (const SurfaceConfig& a, const SurfaceConfig& b)
	{
		return a.compareTo(b, m_specifiedRGBColors, m_specifiedLuminanceColors, m_yuvPlaneBppSpecified);
	}

private:
	const tcu::BVec4	m_specifiedRGBColors;
	const tcu::BVec2	m_specifiedLuminanceColors;
	const bool			m_yuvPlaneBppSpecified;
};

class ConfigFilter
{
private:
	std::map<EGLenum, AttribRule> m_rules;
public:
	ConfigFilter ()
		: m_rules(SurfaceConfig::getDefaultRules())
	{
	}

	void setValue (EGLenum name, EGLint value)
	{
		DE_ASSERT(de::contains(m_rules, name));
		m_rules[name].value = value;
	}

	void setValues (std::vector<std::pair<EGLenum, EGLint> > values)
	{
		for (size_t ndx = 0; ndx < values.size(); ndx++)
		{
			const EGLenum	name	= values[ndx].first;
			const EGLint	value	= values[ndx].second;

			setValue(name, value);
		}
	}

	AttribRule getAttribute (EGLenum name) const
	{
		DE_ASSERT(de::contains(m_rules, name));
		return m_rules.find(name)->second;
	}

	bool isMatch (const SurfaceConfig& config) const
	{
		for (std::map<EGLenum, AttribRule>::const_iterator iter = m_rules.begin(); iter != m_rules.end(); iter++)
		{
			const AttribRule rule = iter->second;

			if (rule.value == EGL_DONT_CARE)
				continue;
			else if (rule.name == EGL_MATCH_NATIVE_PIXMAP)
				TCU_CHECK(rule.value == EGL_NONE); // Not supported
			else if (rule.name == EGL_TRANSPARENT_RED_VALUE || rule.name == EGL_TRANSPARENT_GREEN_VALUE || rule.name == EGL_TRANSPARENT_BLUE_VALUE)
				continue;
			else
			{
				const EGLint cfgValue = config.getAttribute(rule.name);

				switch (rule.criteria)
				{
					case CRITERIA_EXACT:
						if (rule.value != cfgValue)
							return false;
						break;

					case CRITERIA_AT_LEAST:
						if (rule.value > cfgValue)
							return false;
						break;

					case CRITERIA_MASK:
						if ((rule.value & cfgValue) != rule.value)
							return false;
						break;

					default:
						TCU_FAIL("Unknown criteria");
				}
			}
		}

		return true;
	}

	tcu::BVec4 getSpecifiedRGBColors (void) const
	{
		const EGLenum bitAttribs[] =
		{
			EGL_RED_SIZE,
			EGL_GREEN_SIZE,
			EGL_BLUE_SIZE,
			EGL_ALPHA_SIZE
		};

		tcu::BVec4 result;

		for (int ndx = 0; ndx < DE_LENGTH_OF_ARRAY(bitAttribs); ndx++)
		{
			const EGLenum	attrib	= bitAttribs[ndx];
			const EGLint	value	= getAttribute(attrib).value;

			if (value != 0 && value != EGL_DONT_CARE)
				result[ndx] = true;
			else
				result[ndx] = false;
		}

		return result;
	}

	tcu::BVec2 getSpecifiedLuminanceColors (void) const
	{
		const EGLenum bitAttribs[] =
		{
			EGL_LUMINANCE_SIZE,
			EGL_ALPHA_SIZE
		};

		tcu::BVec2 result;

		for (int ndx = 0; ndx < DE_LENGTH_OF_ARRAY(bitAttribs); ndx++)
		{
			const EGLenum	attrib	= bitAttribs[ndx];
			const EGLint	value	= getAttribute(attrib).value;

			if (value != 0 && value != EGL_DONT_CARE)
				result[ndx] = true;
			else
				result[ndx] = false;
		}

		return result;
	}

	bool isYuvPlaneBppSpecified (void) const
	{
		const EGLenum	attrib	= EGL_YUV_PLANE_BPP_EXT;
		const EGLint	value	= getAttribute(attrib).value;

		return (value != 0) && (value != EGL_DONT_CARE);
	}

	std::vector<SurfaceConfig> filter (const std::vector<SurfaceConfig>& configs) const
	{
		std::vector<SurfaceConfig> out;

		for (std::vector<SurfaceConfig>::const_iterator iter = configs.begin(); iter != configs.end(); iter++)
		{
			if (isMatch(*iter)) out.push_back(*iter);
		}

		return out;
	}
};

void chooseConfigReference (const Library& egl, EGLDisplay display, std::vector<EGLConfig>& dst, const std::vector<std::pair<EGLenum, EGLint> >& attributes)
{
	// Get all configs
	std::vector<EGLConfig> eglConfigs = eglu::getConfigs(egl, display);

	// Config infos - including extension attributes
	std::vector<ConfigInfo> configInfos;
	configInfos.resize(eglConfigs.size());

	for (size_t ndx = 0; ndx < eglConfigs.size(); ndx++)
	{
		eglu::queryCoreConfigInfo(egl, display, eglConfigs[ndx], &configInfos[ndx]);
		eglu::queryExtConfigInfo(egl, display, eglConfigs[ndx], &configInfos[ndx]);
	}

	// Pair configs with info
	std::vector<SurfaceConfig> configs;
	for (size_t ndx = 0; ndx < eglConfigs.size(); ndx++)
		configs.push_back(SurfaceConfig(eglConfigs[ndx], configInfos[ndx]));

	// Filter configs
	ConfigFilter configFilter;
	configFilter.setValues(attributes);

	std::vector<SurfaceConfig> filteredConfigs = configFilter.filter(configs);

	// Sort configs
	std::sort(filteredConfigs.begin(), filteredConfigs.end(), CompareConfigs(configFilter.getSpecifiedRGBColors(), configFilter.getSpecifiedLuminanceColors(), configFilter.isYuvPlaneBppSpecified()));

	// Write to dst list
	dst.resize(filteredConfigs.size());
	for (size_t ndx = 0; ndx < filteredConfigs.size(); ndx++)
		dst[ndx] = filteredConfigs[ndx].getEglConfig();
}

} // egl
} // deqp
