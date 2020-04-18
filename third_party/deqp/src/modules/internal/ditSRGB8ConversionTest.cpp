/*-------------------------------------------------------------------------
 * drawElements Internal Test Module
 * ---------------------------------
 *
 * Copyright 2015 The Android Open Source Project
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
 * \brief 8-bit sRGB conversion test.
 *//*--------------------------------------------------------------------*/

#include "ditSRGB8ConversionTest.hpp"

#include "tcuFloat.hpp"
#include "tcuTestLog.hpp"
#include "tcuTextureUtil.hpp"
#include "tcuVectorUtil.hpp"

namespace dit
{
namespace
{

deUint32 calculateDiscreteFloatDistance (float a, float b)
{
	const deUint32		au		= tcu::Float32(a).bits();
	const deUint32		bu		= tcu::Float32(b).bits();

	const bool			asign	= (au & (0x1u << 31u)) != 0u;
	const bool			bsign	= (bu & (0x1u << 31u)) != 0u;

	const deUint32		avalue	= (au & ((0x1u << 31u) - 1u));
	const deUint32		bvalue	= (bu & ((0x1u << 31u) - 1u));

	if (asign != bsign)
		return avalue + bvalue + 1u;
	else if (avalue < bvalue)
		return bvalue - avalue;
	else
		return avalue - bvalue;
}

const tcu::UVec4 calculateDiscreteFloatDistance (const tcu::Vec4& ref, const tcu::Vec4& res)
{
	return tcu::UVec4(calculateDiscreteFloatDistance(ref[0], res[0]), calculateDiscreteFloatDistance(ref[1], res[1]), calculateDiscreteFloatDistance(ref[2], res[2]), calculateDiscreteFloatDistance(ref[3], res[3]));
}

class SRGB8ConversionTest : public tcu::TestCase
{
public:
	SRGB8ConversionTest (tcu::TestContext& context)
		: tcu::TestCase	(context, "srgb8", "SRGB8 conversion test")
	{
	}

	IterateResult iterate (void)
	{
		bool			isOk	= true;
		tcu::TestLog&	log		= m_testCtx.getLog();

		for (int i = 0; i < 256; i++)
		{
			const tcu::UVec4	src					(i);
			const tcu::Vec4		res					(tcu::sRGBA8ToLinear(src));
			const tcu::Vec4		ref					(tcu::sRGBToLinear(src.cast<float>() / tcu::Vec4(255.0f)));
			const tcu::Vec4		diff				(res - ref);
			const tcu::UVec4	discreteFloatDiff	(calculateDiscreteFloatDistance(ref, res));

			if (tcu::anyNotEqual(res, ref))
				log << tcu::TestLog::Message << i << ", Res: " << res << ", Ref: " << ref << ", Diff: " << diff << ", Discrete float diff: " << discreteFloatDiff << tcu::TestLog::EndMessage;

			if (tcu::boolAny(tcu::greaterThan(discreteFloatDiff, tcu::UVec4(1u))))
				isOk = false;
		}

		if (isOk)
			m_testCtx.setTestResult(QP_TEST_RESULT_PASS, "Pass");
		else
			m_testCtx.setTestResult(QP_TEST_RESULT_FAIL, "Got ulp diffs greater than one.");

		return STOP;
	}
};

} // anonymous

tcu::TestCase* createSRGB8ConversionTest (tcu::TestContext& context)
{
	return new SRGB8ConversionTest(context);
}

} // dit
