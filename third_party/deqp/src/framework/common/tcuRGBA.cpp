/*-------------------------------------------------------------------------
 * drawElements Quality Program Tester Core
 * ----------------------------------------
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
 * \brief RGBA8888 color type.
 *//*--------------------------------------------------------------------*/

#include "tcuRGBA.hpp"
#include "tcuVector.hpp"
#include "tcuTextureUtil.hpp"

namespace tcu
{

RGBA::RGBA (const Vec4& v)
{
	const deUint32 r = (deUint32)floatToU8(v.x());
	const deUint32 g = (deUint32)floatToU8(v.y());
	const deUint32 b = (deUint32)floatToU8(v.z());
	const deUint32 a = (deUint32)floatToU8(v.w());
	m_value = (a << ALPHA_SHIFT) | (r << RED_SHIFT) | (g << GREEN_SHIFT) | (b << BLUE_SHIFT);
}

Vec4 RGBA::toVec (void) const
{
	return Vec4(float(getRed())		/ 255.0f,
				float(getGreen())	/ 255.0f,
				float(getBlue())	/ 255.0f,
				float(getAlpha())	/ 255.0f);
}

IVec4 RGBA::toIVec (void) const
{
	return IVec4(getRed(), getGreen(), getBlue(), getAlpha());
}

RGBA computeAbsDiffMasked (RGBA a, RGBA b, deUint32 cmpMask)
{
	deUint32	aPacked = a.getPacked();
	deUint32	bPacked = b.getPacked();
	deUint8		rDiff	= 0;
	deUint8		gDiff	= 0;
	deUint8		bDiff	= 0;
	deUint8		aDiff	= 0;

	if (cmpMask & RGBA::RED_MASK)
	{
		int ra = (aPacked >> RGBA::RED_SHIFT) & 0xFF;
		int rb = (bPacked >> RGBA::RED_SHIFT) & 0xFF;

		rDiff = (deUint8)deAbs32(ra - rb);
	}

	if (cmpMask & RGBA::GREEN_MASK)
	{
		int ga = (aPacked >> RGBA::GREEN_SHIFT) & 0xFF;
		int gb = (bPacked >> RGBA::GREEN_SHIFT) & 0xFF;

		gDiff = (deUint8)deAbs32(ga - gb);
	}

	if (cmpMask & RGBA::BLUE_MASK)
	{
		int ba = (aPacked >> RGBA::BLUE_SHIFT) & 0xFF;
		int bb = (bPacked >> RGBA::BLUE_SHIFT) & 0xFF;

		bDiff = (deUint8)deAbs32(ba - bb);
	}

	if (cmpMask & RGBA::ALPHA_MASK)
	{
		int aa = (aPacked >> RGBA::ALPHA_SHIFT) & 0xFF;
		int ab = (bPacked >> RGBA::ALPHA_SHIFT) & 0xFF;

		aDiff = (deUint8)deAbs32(aa - ab);
	}

	return RGBA(rDiff,gDiff,bDiff,aDiff);
}

bool compareThresholdMasked	(RGBA a, RGBA b, RGBA threshold, deUint32 cmpMask)
{
	return computeAbsDiffMasked(a, b, cmpMask).isBelowThreshold(threshold);
}

} // tcu
