#ifndef _TCURGBA_HPP
#define _TCURGBA_HPP
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

#include "tcuDefs.hpp"
#include "deInt32.h"
#include "tcuVectorType.hpp"

#include <sstream>

namespace tcu
{

/*--------------------------------------------------------------------*//*!
 * \brief RGBA8888 color struct
 *//*--------------------------------------------------------------------*/
class RGBA
{
public:
	enum
	{
		RED_SHIFT	= 0,
		GREEN_SHIFT	= 8,
		BLUE_SHIFT	= 16,
		ALPHA_SHIFT	= 24
	};

	enum
	{
		RED_MASK	= (1<<0),
		GREEN_MASK	= (1<<1),
		BLUE_MASK	= (1<<2),
		ALPHA_MASK	= (1<<3)
	};

	RGBA (void) { m_value = 0; }

	RGBA (int r, int g, int b, int a)
	{
		DE_ASSERT(deInRange32(r, 0, 255));
		DE_ASSERT(deInRange32(g, 0, 255));
		DE_ASSERT(deInRange32(b, 0, 255));
		DE_ASSERT(deInRange32(a, 0, 255));
		m_value = ((deUint32)a << ALPHA_SHIFT) | ((deUint32)r << RED_SHIFT) | ((deUint32)g << GREEN_SHIFT) | ((deUint32)b << BLUE_SHIFT);
	}

	explicit RGBA (deUint32 val)
	{
		m_value = val;
	}

	explicit	RGBA					(const Vec4& v);

	void		setRed					(int v) { DE_ASSERT(deInRange32(v, 0, 255)); m_value = (m_value & ~((deUint32)0xFFu << RED_SHIFT))   | ((deUint32)v << RED_SHIFT);   }
	void		setGreen				(int v) { DE_ASSERT(deInRange32(v, 0, 255)); m_value = (m_value & ~((deUint32)0xFFu << GREEN_SHIFT)) | ((deUint32)v << GREEN_SHIFT); }
	void		setBlue					(int v) { DE_ASSERT(deInRange32(v, 0, 255)); m_value = (m_value & ~((deUint32)0xFFu << BLUE_SHIFT))  | ((deUint32)v << BLUE_SHIFT);  }
	void		setAlpha				(int v) { DE_ASSERT(deInRange32(v, 0, 255)); m_value = (m_value & ~((deUint32)0xFFu << ALPHA_SHIFT)) | ((deUint32)v << ALPHA_SHIFT); }
	int			getRed					(void) const { return (int)((m_value >> (deUint32)RED_SHIFT)   & 0xFFu); }
	int			getGreen				(void) const { return (int)((m_value >> (deUint32)GREEN_SHIFT) & 0xFFu); }
	int			getBlue					(void) const { return (int)((m_value >> (deUint32)BLUE_SHIFT)  & 0xFFu); }
	int			getAlpha				(void) const { return (int)((m_value >> (deUint32)ALPHA_SHIFT) & 0xFFu); }
	deUint32	getPacked				(void) const { return m_value; }

	bool		isBelowThreshold		(RGBA thr) const	{ return (getRed() <= thr.getRed()) && (getGreen() <= thr.getGreen()) && (getBlue() <= thr.getBlue()) && (getAlpha() <= thr.getAlpha()); }

	static RGBA	fromBytes				(const deUint8* bytes)	{ return RGBA(bytes[0], bytes[1], bytes[2], bytes[3]); }
	void		toBytes					(deUint8* bytes) const	{ bytes[0] = (deUint8)getRed(); bytes[1] = (deUint8)getGreen(); bytes[2] = (deUint8)getBlue(); bytes[3] = (deUint8)getAlpha(); }
	Vec4		toVec					(void) const;
	IVec4		toIVec					(void) const;

	bool		operator==				(const RGBA& v) const { return (m_value == v.m_value); }
	bool		operator!=				(const RGBA& v) const { return (m_value != v.m_value); }

	// Color constants.  Designed as methods to avoid static-initialization-order fiasco.
	static inline const RGBA red	(void) { return RGBA(0xFF, 0x0,  0x0,  0xFF); }
	static inline const RGBA green	(void) { return RGBA(0x0,  0xFF, 0x0,  0xFF); }
	static inline const RGBA blue	(void) { return RGBA(0x0,  0x0,  0xFF, 0xFF); }
	static inline const RGBA gray	(void) { return RGBA(0x80, 0x80, 0x80, 0xFF); }
	static inline const RGBA white	(void) { return RGBA(0xFF, 0xFF, 0xFF, 0xFF); }
	static inline const RGBA black	(void) { return RGBA(0x0,  0x0,  0x0,  0xFF); }

private:
	deUint32	m_value;
} DE_WARN_UNUSED_TYPE;

inline bool compareEqualMasked (RGBA a, RGBA b, deUint32 cmpMask)
{
	RGBA		mask((cmpMask&RGBA::RED_MASK)?0xFF:0, (cmpMask&RGBA::GREEN_MASK)?0xFF:0, (cmpMask&RGBA::BLUE_MASK)?0xFF:0, (cmpMask&RGBA::ALPHA_MASK)?0xFF:0);
	deUint32	aPacked		= a.getPacked();
	deUint32	bPacked		= b.getPacked();
	deUint32	maskPacked	= mask.getPacked();
	return (aPacked & maskPacked) == (bPacked & maskPacked);
}

inline RGBA computeAbsDiff (RGBA a, RGBA b)
{
	return RGBA(
		deAbs32(a.getRed()   - b.getRed()),
		deAbs32(a.getGreen() - b.getGreen()),
		deAbs32(a.getBlue()  - b.getBlue()),
		deAbs32(a.getAlpha() - b.getAlpha()));
}

inline RGBA blend (RGBA a, RGBA b, float t)
{
	DE_ASSERT(t >= 0.0f && t <= 1.0f);
	float it = 1.0f - t;
	// \todo [petri] Handling of alpha!
	return RGBA(
		(int)(it*(float)a.getRed() + t*(float)b.getRed() + 0.5f),
		(int)(it*(float)a.getGreen() + t*(float)b.getGreen() + 0.5f),
		(int)(it*(float)a.getBlue() + t*(float)b.getBlue() + 0.5f),
		(int)(it*(float)a.getAlpha() + t*(float)b.getAlpha() + 0.5f));
}

inline bool compareThreshold (RGBA a, RGBA b, RGBA threshold)
{
	if (a == b) return true;	// Quick-accept
	return computeAbsDiff(a, b).isBelowThreshold(threshold);
}

inline RGBA max (RGBA a, RGBA b)
{
	return RGBA(deMax32(a.getRed(),		b.getRed()),
				deMax32(a.getGreen(),	b.getGreen()),
				deMax32(a.getBlue(),	b.getBlue()),
				deMax32(a.getAlpha(),	b.getAlpha()));
}

RGBA computeAbsDiffMasked	(RGBA a, RGBA b, deUint32 cmpMask);
bool compareThresholdMasked	(RGBA a, RGBA b, RGBA threshold, deUint32 cmpMask);

// Arithmetic operators (saturating if not stated otherwise).

inline RGBA operator+ (const RGBA& a, const RGBA& b)
{
	return RGBA(deClamp32(a.getRed()	+ b.getRed(),	0, 255),
				deClamp32(a.getGreen()	+ b.getGreen(),	0, 255),
				deClamp32(a.getBlue()	+ b.getBlue(),	0, 255),
				deClamp32(a.getAlpha()	+ b.getAlpha(),	0, 255));
}

inline RGBA operator- (const RGBA& a, const RGBA& b)
{
	return RGBA(deClamp32(a.getRed()	- b.getRed(),	0, 255),
				deClamp32(a.getGreen()	- b.getGreen(),	0, 255),
				deClamp32(a.getBlue()	- b.getBlue(),	0, 255),
				deClamp32(a.getAlpha()	- b.getAlpha(),	0, 255));
}

inline RGBA operator* (const RGBA& a, const int b)
{
	return RGBA(deClamp32(a.getRed()	* b,	0, 255),
				deClamp32(a.getGreen()	* b,	0, 255),
				deClamp32(a.getBlue()	* b,	0, 255),
				deClamp32(a.getAlpha()	* b,	0, 255));
}

inline std::ostream& operator<< (std::ostream& stream, RGBA c)
{
	return stream << "RGBA(" << c.getRed() << ", " << c.getGreen() << ", " << c.getBlue() << ", " << c.getAlpha() << ")";
}

} // tcu

#endif // _TCURGBA_HPP
