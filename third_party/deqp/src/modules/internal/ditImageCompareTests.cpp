/*-------------------------------------------------------------------------
 * drawElements Internal Test Module
 * ---------------------------------
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
 * \brief Image comparison tests.
 *//*--------------------------------------------------------------------*/

#include "ditImageCompareTests.hpp"
#include "tcuResource.hpp"
#include "tcuImageCompare.hpp"
#include "tcuFuzzyImageCompare.hpp"
#include "tcuImageIO.hpp"
#include "tcuTexture.hpp"
#include "tcuTestLog.hpp"
#include "tcuTextureUtil.hpp"
#include "tcuRGBA.hpp"
#include "deFilePath.hpp"
#include "deClock.h"

namespace dit
{

using tcu::TestLog;

static const char*	BASE_DIR			= "internal/data/imagecompare";

static void loadImageRGBA8 (tcu::TextureLevel& dst, const tcu::Archive& archive, const char* path)
{
	tcu::TextureLevel tmp;
	tcu::ImageIO::loadImage(tmp, archive, path);

	dst.setStorage(tcu::TextureFormat(tcu::TextureFormat::RGBA, tcu::TextureFormat::UNORM_INT8), tmp.getWidth(), tmp.getHeight());
	tcu::copy(dst, tmp);
}

class FuzzyComparisonMetricCase : public tcu::TestCase
{
public:
	FuzzyComparisonMetricCase (tcu::TestContext& testCtx, const char* name, const char* refImg, const char* cmpImg, const float minBound, const float maxBound)
		: tcu::TestCase	(testCtx, name, "")
		, m_refImg		(refImg)
		, m_cmpImg		(cmpImg)
		, m_minBound	(minBound)
		, m_maxBound	(maxBound)
	{
	}

	IterateResult iterate (void)
	{
		tcu::TextureLevel		refImg;
		tcu::TextureLevel		cmpImg;
		tcu::TextureLevel		errorMask;
		tcu::FuzzyCompareParams	params;
		float					result		= 0.0f;
		deUint64				compareTime	= 0;

		params.maxSampleSkip = 0;

		tcu::ImageIO::loadImage(refImg, m_testCtx.getArchive(), de::FilePath::join(BASE_DIR, m_refImg).getPath());
		tcu::ImageIO::loadImage(cmpImg, m_testCtx.getArchive(), de::FilePath::join(BASE_DIR, m_cmpImg).getPath());

		errorMask.setStorage(refImg.getFormat(), refImg.getWidth(), refImg.getHeight(), refImg.getDepth());

		{
			const deUint64 startTime = deGetMicroseconds();
			result = tcu::fuzzyCompare(params, refImg, cmpImg, errorMask);
			compareTime = deGetMicroseconds()-startTime;
		}

		m_testCtx.getLog() << TestLog::Image("RefImage",	"Reference Image",	refImg)
						   << TestLog::Image("CmpImage",	"Compare Image",	cmpImg)
						   << TestLog::Image("ErrorMask",	"Error Mask",		errorMask);

		m_testCtx.getLog() << TestLog::Float("Result", "Result metric", "", QP_KEY_TAG_NONE, result)
						   << TestLog::Float("MinBound", "Minimum bound", "", QP_KEY_TAG_NONE, m_minBound)
						   << TestLog::Float("MaxBound", "Maximum bound", "", QP_KEY_TAG_NONE, m_maxBound)
						   << TestLog::Integer("CompareTime", "Comparison time", "us", QP_KEY_TAG_TIME, compareTime);

		{
			const bool isOk = de::inRange(result, m_minBound, m_maxBound);
			m_testCtx.setTestResult(isOk ? QP_TEST_RESULT_PASS	: QP_TEST_RESULT_FAIL,
									isOk ? "Pass"				: "Metric out of bounds");
		}

		return STOP;
	}

private:
	const std::string	m_refImg;
	const std::string	m_cmpImg;
	const float			m_minBound;
	const float			m_maxBound;
};

class BilinearCompareCase : public tcu::TestCase
{
public:
	BilinearCompareCase (tcu::TestContext& testCtx, const char* name, const char* refImg, const char* cmpImg, const tcu::RGBA& threshold, bool expectedResult)
		: tcu::TestCase		(testCtx, name, "")
		, m_refImg			(refImg)
		, m_cmpImg			(cmpImg)
		, m_threshold		(threshold)
		, m_expectedResult	(expectedResult)
	{
	}

	IterateResult iterate (void)
	{
		tcu::TextureLevel		refImg;
		tcu::TextureLevel		cmpImg;
		bool					result;
		deUint64				compareTime	= 0;

		loadImageRGBA8(refImg, m_testCtx.getArchive(), de::FilePath::join(BASE_DIR, m_refImg).getPath());
		loadImageRGBA8(cmpImg, m_testCtx.getArchive(), de::FilePath::join(BASE_DIR, m_cmpImg).getPath());

		{
			const deUint64 startTime = deGetMicroseconds();
			result = tcu::bilinearCompare(m_testCtx.getLog(), "CompareResult", "Image comparison result", refImg, cmpImg, m_threshold, tcu::COMPARE_LOG_EVERYTHING);
			compareTime = deGetMicroseconds()-startTime;
		}

		m_testCtx.getLog() << TestLog::Integer("CompareTime", "Comparison time", "us", QP_KEY_TAG_TIME, compareTime);

		{
			const bool isOk = result == m_expectedResult;
			m_testCtx.setTestResult(isOk ? QP_TEST_RESULT_PASS	: QP_TEST_RESULT_FAIL,
									isOk ? "Pass"				: "Wrong comparison result");
		}

		return STOP;
	}

private:
	const std::string		m_refImg;
	const std::string		m_cmpImg;
	const tcu::RGBA			m_threshold;
	const bool				m_expectedResult;
};

class FuzzyComparisonMetricTests : public tcu::TestCaseGroup
{
public:
	FuzzyComparisonMetricTests (tcu::TestContext& testCtx)
		: tcu::TestCaseGroup(testCtx, "fuzzy_metric", "Fuzzy comparison metrics")
	{
	}

	void init (void)
	{
		addChild(new FuzzyComparisonMetricCase(m_testCtx, "identical",		"cube_ref.png",				"cube_ref.png",				0.0f,			0.000001f));
		addChild(new FuzzyComparisonMetricCase(m_testCtx, "cube",			"cube_ref.png",				"cube_cmp.png",				0.0029f,		0.0031f));
		addChild(new FuzzyComparisonMetricCase(m_testCtx, "cube_2",			"cube_2_ref.png",			"cube_2_cmp.png",			0.0134f,		0.0140f));
		addChild(new FuzzyComparisonMetricCase(m_testCtx, "cube_sphere",	"cube_sphere_ref.png",		"cube_sphere_cmp.png",		0.0730f,		0.0801f));
		addChild(new FuzzyComparisonMetricCase(m_testCtx, "cube_nmap",		"cube_nmap_ref.png",		"cube_nmap_cmp.png",		0.0022f,		0.0025f));
		addChild(new FuzzyComparisonMetricCase(m_testCtx, "cube_nmap_2",	"cube_nmap_2_ref.png",		"cube_nmap_2_cmp.png",		0.0172f,		0.0189f));
		addChild(new FuzzyComparisonMetricCase(m_testCtx, "earth_diffuse",	"earth_diffuse_ref.png",	"earth_diffuse_cmp.png",	0.0f,			0.00002f));
		addChild(new FuzzyComparisonMetricCase(m_testCtx, "eath_texture",	"earth_texture_ref.png",	"earth_texture_cmp.png",	0.0002f,		0.0003f));
		addChild(new FuzzyComparisonMetricCase(m_testCtx, "earth_spot",		"earth_spot_ref.png",		"earth_spot_cmp.png",		0.0015f,		0.0018f));
		addChild(new FuzzyComparisonMetricCase(m_testCtx, "earth_light",	"earth_light_ref.png",		"earth_light_cmp.png",		1.7050f,		1.7070f));
		addChild(new FuzzyComparisonMetricCase(m_testCtx, "lessThan0",		"lessThan0-reference.png",	"lessThan0-result.png",		0.0003f,		0.0004f));
		addChild(new FuzzyComparisonMetricCase(m_testCtx, "cube_sphere_2",	"cube_sphere_2_ref.png",	"cube_sphere_2_cmp.png",	0.0207f,		0.0230f));
		addChild(new FuzzyComparisonMetricCase(m_testCtx, "earth_to_empty",	"earth_spot_ref.png",		"empty_256x256.png",		54951.0f,		54955.0f));
	}
};

class BilinearCompareTests : public tcu::TestCaseGroup
{
public:
	BilinearCompareTests (tcu::TestContext& testCtx)
		: tcu::TestCaseGroup(testCtx, "bilinear_compare", "Bilinear Image Comparison Tests")
	{
	}

	void init (void)
	{
		addChild(new BilinearCompareCase(m_testCtx, "identical",				"cube_ref.png",						"cube_ref.png",						tcu::RGBA(0,0,0,0),			true));
		addChild(new BilinearCompareCase(m_testCtx, "empty_to_white",			"empty_256x256.png",				"white_256x256.png",				tcu::RGBA(7,7,7,2),			false));
		addChild(new BilinearCompareCase(m_testCtx, "white_to_empty",			"white_256x256.png",				"empty_256x256.png",				tcu::RGBA(7,7,7,2),			false));
		addChild(new BilinearCompareCase(m_testCtx, "cube",						"cube_ref.png",						"cube_cmp.png",						tcu::RGBA(7,7,7,2),			false));
		addChild(new BilinearCompareCase(m_testCtx, "cube_2",					"cube_2_ref.png",					"cube_2_cmp.png",					tcu::RGBA(7,7,7,2),			false));
		addChild(new BilinearCompareCase(m_testCtx, "cube_sphere",				"cube_sphere_ref.png",				"cube_sphere_cmp.png",				tcu::RGBA(7,7,7,2),			false));
		addChild(new BilinearCompareCase(m_testCtx, "cube_nmap",				"cube_nmap_ref.png",				"cube_nmap_cmp.png",				tcu::RGBA(7,7,7,2),			false));
		addChild(new BilinearCompareCase(m_testCtx, "cube_nmap_2",				"cube_nmap_2_ref.png",				"cube_nmap_2_cmp.png",				tcu::RGBA(7,7,7,2),			false));
		addChild(new BilinearCompareCase(m_testCtx, "earth_diffuse",			"earth_diffuse_ref.png",			"earth_diffuse_cmp.png",			tcu::RGBA(20,20,20,2),		true));
		addChild(new BilinearCompareCase(m_testCtx, "eath_texture",				"earth_texture_ref.png",			"earth_texture_cmp.png",			tcu::RGBA(7,7,7,2),			false));
		addChild(new BilinearCompareCase(m_testCtx, "earth_spot",				"earth_spot_ref.png",				"earth_spot_cmp.png",				tcu::RGBA(7,7,7,2),			false));
		addChild(new BilinearCompareCase(m_testCtx, "earth_light",				"earth_light_ref.png",				"earth_light_cmp.png",				tcu::RGBA(7,7,7,2),			false));
		addChild(new BilinearCompareCase(m_testCtx, "lessThan0",				"lessThan0-reference.png",			"lessThan0-result.png",				tcu::RGBA(36,36,36,2),		true));
		addChild(new BilinearCompareCase(m_testCtx, "cube_sphere_2",			"cube_sphere_2_ref.png",			"cube_sphere_2_cmp.png",			tcu::RGBA(7,7,7,2),			false));
		addChild(new BilinearCompareCase(m_testCtx, "earth_to_empty",			"earth_spot_ref.png",				"empty_256x256.png",				tcu::RGBA(7,7,7,2),			false));
		addChild(new BilinearCompareCase(m_testCtx, "texfilter",				"texfilter_ref.png",				"texfilter_cmp.png",				tcu::RGBA(7,7,7,2),			true));
		addChild(new BilinearCompareCase(m_testCtx, "refract_vtx",				"refract_vtx_ref.png",				"refract_vtx_cmp.png",				tcu::RGBA(7,7,7,2),			true));
		addChild(new BilinearCompareCase(m_testCtx, "refract_frag",				"refract_frag_ref.png",				"refract_frag_cmp.png",				tcu::RGBA(7,7,7,2),			true));
		addChild(new BilinearCompareCase(m_testCtx, "lessthan_vtx",				"lessthan_vtx_ref.png",				"lessthan_vtx_cmp.png",				tcu::RGBA(7,7,7,2),			true));
		addChild(new BilinearCompareCase(m_testCtx, "2_units_2d",				"2_units_2d_ref.png",				"2_units_2d_cmp.png",				tcu::RGBA(7,7,7,2),			false));
		addChild(new BilinearCompareCase(m_testCtx, "4_units_cube_vtx",			"4_units_cube_ref.png",				"4_units_cube_cmp.png",				tcu::RGBA(7,7,7,2),			false));
		addChild(new BilinearCompareCase(m_testCtx, "texfilter_vtx_nearest",	"texfilter_vtx_nearest_ref.png",	"texfilter_vtx_nearest_cmp.png",	tcu::RGBA(7,7,7,2),			false));
		addChild(new BilinearCompareCase(m_testCtx, "texfilter_vtx_linear",		"texfilter_vtx_linear_ref.png",		"texfilter_vtx_linear_cmp.png",		tcu::RGBA(7,7,7,2),			false));
		addChild(new BilinearCompareCase(m_testCtx, "readpixels_msaa",			"readpixels_ref.png",				"readpixels_msaa.png",				tcu::RGBA(1,1,1,1),			true));
	}
};

ImageCompareTests::ImageCompareTests (tcu::TestContext& testCtx)
	: tcu::TestCaseGroup(testCtx, "image_compare", "Image comparison tests")
{
}

ImageCompareTests::~ImageCompareTests (void)
{
}

void ImageCompareTests::init (void)
{
	addChild(new FuzzyComparisonMetricTests	(m_testCtx));
	addChild(new BilinearCompareTests		(m_testCtx));
}

} // dit
