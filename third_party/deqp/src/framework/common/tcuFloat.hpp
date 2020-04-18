#ifndef _TCUFLOAT_HPP
#define _TCUFLOAT_HPP
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
 * \brief Reconfigurable floating-point value template.
 *//*--------------------------------------------------------------------*/

#include "tcuDefs.hpp"

// For memcpy().
#include <string.h>

namespace tcu
{

enum FloatFlags
{
	FLOAT_HAS_SIGN			= (1<<0),
	FLOAT_SUPPORT_DENORM	= (1<<1)
};

/*--------------------------------------------------------------------*//*!
 * \brief Floating-point format template
 *
 * This template implements arbitrary floating-point handling. Template
 * can be used for conversion between different formats and checking
 * various properties of floating-point values.
 *//*--------------------------------------------------------------------*/
template <typename StorageType_, int ExponentBits, int MantissaBits, int ExponentBias, deUint32 Flags>
class Float
{
public:
	typedef StorageType_ StorageType;

	enum
	{
		EXPONENT_BITS	= ExponentBits,
		MANTISSA_BITS	= MantissaBits,
		EXPONENT_BIAS	= ExponentBias,
		FLAGS			= Flags,
	};

							Float			(void);
	explicit				Float			(StorageType value);
	explicit				Float			(float v);
	explicit				Float			(double v);

	template <typename OtherStorageType, int OtherExponentBits, int OtherMantissaBits, int OtherExponentBias, deUint32 OtherFlags>
	static Float			convert			(const Float<OtherStorageType, OtherExponentBits, OtherMantissaBits, OtherExponentBias, OtherFlags>& src);

	static inline Float		convert			(const Float<StorageType, ExponentBits, MantissaBits, ExponentBias, Flags>& src) { return src; }

	/*--------------------------------------------------------------------*//*!
	 * \brief Construct floating point value
	 * \param sign		Sign. Must be +1/-1
	 * \param exponent	Exponent in range [1-ExponentBias, ExponentBias+1]
	 * \param mantissa	Mantissa bits with implicit leading bit explicitly set
	 * \return The specified float
	 *
	 * This function constructs a floating point value from its inputs.
	 * The normally implicit leading bit of the mantissa must be explicitly set.
	 * The exponent normally used for zero/subnormals is an invalid input. Such
	 * values are specified with the leading mantissa bit of zero and the lowest
	 * normal exponent (1-ExponentBias). Additionally having both exponent and
	 * mantissa set to zero is a shorthand notation for the correctly signed
	 * floating point zero. Inf and NaN must be specified directly with an
	 * exponent of ExponentBias+1 and the appropriate mantissa (with leading
	 * bit set)
	 *//*--------------------------------------------------------------------*/
	static inline Float		construct		(int sign, int exponent, StorageType mantissa);

	/*--------------------------------------------------------------------*//*!
	 * \brief Construct floating point value. Explicit version
	 * \param sign		Sign. Must be +1/-1
	 * \param exponent	Exponent in range [-ExponentBias, ExponentBias+1]
	 * \param mantissa	Mantissa bits
	 * \return The specified float
	 *
	 * This function constructs a floating point value from its inputs with
	 * minimal intervention.
	 * The sign is turned into a sign bit and the exponent bias is added.
	 * See IEEE-754 for additional information on the inputs and
	 * the encoding of special values.
	 *//*--------------------------------------------------------------------*/
	static Float			constructBits	(int sign, int exponent, StorageType mantissaBits);

	StorageType				bits			(void) const	{ return m_value;															}
	float					asFloat			(void) const;
	double					asDouble		(void) const;

	inline int				signBit			(void) const	{ return (int)(m_value >> (ExponentBits+MantissaBits)) & 1;					}
	inline StorageType		exponentBits	(void) const	{ return (m_value >> MantissaBits) & ((StorageType(1)<<ExponentBits)-1);	}
	inline StorageType		mantissaBits	(void) const	{ return m_value & ((StorageType(1)<<MantissaBits)-1);						}

	inline int				sign			(void) const	{ return signBit() ? -1 : 1;																			}
	inline int				exponent		(void) const	{ return isDenorm() ? 1	- ExponentBias : (int)exponentBits() - ExponentBias;							}
	inline StorageType		mantissa		(void) const	{ return isZero() || isDenorm() ? mantissaBits() : (mantissaBits() | (StorageType(1)<<MantissaBits));	}

	inline bool				isInf			(void) const	{ return exponentBits() == ((1<<ExponentBits)-1)	&& mantissaBits() == 0;	}
	inline bool				isNaN			(void) const	{ return exponentBits() == ((1<<ExponentBits)-1)	&& mantissaBits() != 0;	}
	inline bool				isZero			(void) const	{ return exponentBits() == 0						&& mantissaBits() == 0;	}
	inline bool				isDenorm		(void) const	{ return exponentBits() == 0						&& mantissaBits() != 0;	}

	static Float			zero			(int sign);
	static Float			inf				(int sign);
	static Float			nan				(void);

private:
	StorageType				m_value;
} DE_WARN_UNUSED_TYPE;

// Common floating-point types.
typedef Float<deUint16,  5, 10,   15, FLOAT_HAS_SIGN|FLOAT_SUPPORT_DENORM>	Float16;	//!< IEEE 754-2008 16-bit floating-point value
typedef Float<deUint32,  8, 23,  127, FLOAT_HAS_SIGN|FLOAT_SUPPORT_DENORM>	Float32;	//!< IEEE 754 32-bit floating-point value
typedef Float<deUint64, 11, 52, 1023, FLOAT_HAS_SIGN|FLOAT_SUPPORT_DENORM>	Float64;	//!< IEEE 754 64-bit floating-point value

template <typename StorageType, int ExponentBits, int MantissaBits, int ExponentBias, deUint32 Flags>
inline Float<StorageType, ExponentBits, MantissaBits, ExponentBias, Flags>::Float (void)
	: m_value(0)
{
}

template <typename StorageType, int ExponentBits, int MantissaBits, int ExponentBias, deUint32 Flags>
inline Float<StorageType, ExponentBits, MantissaBits, ExponentBias, Flags>::Float (StorageType value)
	: m_value(value)
{
}

template <typename StorageType, int ExponentBits, int MantissaBits, int ExponentBias, deUint32 Flags>
inline Float<StorageType, ExponentBits, MantissaBits, ExponentBias, Flags>::Float (float value)
	: m_value(0)
{
	deUint32 u32;
	memcpy(&u32, &value, sizeof(deUint32));
	*this = convert(Float32(u32));
}

template <typename StorageType, int ExponentBits, int MantissaBits, int ExponentBias, deUint32 Flags>
inline Float<StorageType, ExponentBits, MantissaBits, ExponentBias, Flags>::Float (double value)
	: m_value(0)
{
	deUint64 u64;
	memcpy(&u64, &value, sizeof(deUint64));
	*this = convert(Float64(u64));
}

template <typename StorageType, int ExponentBits, int MantissaBits, int ExponentBias, deUint32 Flags>
inline float Float<StorageType, ExponentBits, MantissaBits, ExponentBias, Flags>::asFloat (void) const
{
	float		v;
	deUint32	u32		= Float32::convert(*this).bits();
	memcpy(&v, &u32, sizeof(deUint32));
	return v;
}

template <typename StorageType, int ExponentBits, int MantissaBits, int ExponentBias, deUint32 Flags>
inline double Float<StorageType, ExponentBits, MantissaBits, ExponentBias, Flags>::asDouble (void) const
{
	double		v;
	deUint64	u64		= Float64::convert(*this).bits();
	memcpy(&v, &u64, sizeof(deUint64));
	return v;
}

template <typename StorageType, int ExponentBits, int MantissaBits, int ExponentBias, deUint32 Flags>
inline Float<StorageType, ExponentBits, MantissaBits, ExponentBias, Flags> Float<StorageType, ExponentBits, MantissaBits, ExponentBias, Flags>::zero (int sign)
{
	DE_ASSERT(sign == 1 || ((Flags & FLOAT_HAS_SIGN) && sign == -1));
	return Float(StorageType((sign > 0 ? 0ull : 1ull) << (ExponentBits+MantissaBits)));
}

template <typename StorageType, int ExponentBits, int MantissaBits, int ExponentBias, deUint32 Flags>
inline Float<StorageType, ExponentBits, MantissaBits, ExponentBias, Flags> Float<StorageType, ExponentBits, MantissaBits, ExponentBias, Flags>::inf (int sign)
{
	DE_ASSERT(sign == 1 || ((Flags & FLOAT_HAS_SIGN) && sign == -1));
	return Float(StorageType(((sign > 0 ? 0ull : 1ull) << (ExponentBits+MantissaBits)) | (((1ull<<ExponentBits)-1) << MantissaBits)));
}

template <typename StorageType, int ExponentBits, int MantissaBits, int ExponentBias, deUint32 Flags>
inline Float<StorageType, ExponentBits, MantissaBits, ExponentBias, Flags> Float<StorageType, ExponentBits, MantissaBits, ExponentBias, Flags>::nan (void)
{
	return Float(StorageType((1ull<<(ExponentBits+MantissaBits))-1));
}

template <typename StorageType, int ExponentBits, int MantissaBits, int ExponentBias, deUint32 Flags>
Float<StorageType, ExponentBits, MantissaBits, ExponentBias, Flags>
Float<StorageType, ExponentBits, MantissaBits, ExponentBias, Flags>::construct
	(int sign, int exponent, StorageType mantissa)
{
	// Repurpose this otherwise invalid input as a shorthand notation for zero (no need for caller to care about internal representation)
	const bool			isShorthandZero	= exponent == 0 && mantissa == 0;

	// Handles the typical notation for zero (min exponent, mantissa 0). Note that the exponent usually used exponent (-ExponentBias) for zero/subnormals is not used.
	// Instead zero/subnormals have the (normally implicit) leading mantissa bit set to zero.
	const bool			isDenormOrZero	= (exponent == 1 - ExponentBias) && (mantissa >> MantissaBits == 0);
	const StorageType	s				= StorageType((StorageType(sign < 0 ? 1 : 0)) << (StorageType(ExponentBits+MantissaBits)));
	const StorageType	exp				= (isShorthandZero  || isDenormOrZero) ? StorageType(0) : StorageType(exponent + ExponentBias);

	DE_ASSERT(sign == +1 || sign == -1);
	DE_ASSERT(isShorthandZero || isDenormOrZero || mantissa >> MantissaBits == 1);
	DE_ASSERT(exp >> ExponentBits == 0);

	return Float(StorageType(s | (exp << MantissaBits) | (mantissa & ((StorageType(1)<<MantissaBits)-1))));
}

template <typename StorageType, int ExponentBits, int MantissaBits, int ExponentBias, deUint32 Flags>
Float<StorageType, ExponentBits, MantissaBits, ExponentBias, Flags>
Float<StorageType, ExponentBits, MantissaBits, ExponentBias, Flags>::constructBits
	(int sign, int exponent, StorageType mantissaBits)
{
	const StorageType signBit		= sign < 0 ? 1 : 0;
	const StorageType exponentBits	= exponent + ExponentBias;

	DE_ASSERT(sign == +1 || sign == -1 );
	DE_ASSERT(exponentBits >> ExponentBits == 0);
	DE_ASSERT(mantissaBits >> MantissaBits == 0);

	return Float(StorageType((signBit << (ExponentBits+MantissaBits)) | (exponentBits << MantissaBits) | (mantissaBits)));
}

template <typename StorageType, int ExponentBits, int MantissaBits, int ExponentBias, deUint32 Flags>
template <typename OtherStorageType, int OtherExponentBits, int OtherMantissaBits, int OtherExponentBias, deUint32 OtherFlags>
Float<StorageType, ExponentBits, MantissaBits, ExponentBias, Flags>
Float<StorageType, ExponentBits, MantissaBits, ExponentBias, Flags>::convert
	(const Float<OtherStorageType, OtherExponentBits, OtherMantissaBits, OtherExponentBias, OtherFlags>& other)
{
	if (!(Flags & FLOAT_HAS_SIGN) && other.sign() < 0)
	{
		// Negative number, truncate to zero.
		return zero(+1);
	}
	else if (other.isInf())
	{
		return inf(other.sign());
	}
	else if (other.isNaN())
	{
		return nan();
	}
	else if (other.isZero())
	{
		return zero(other.sign());
	}
	else
	{
		const int			eMin	= 1 - ExponentBias;
		const int			eMax	= ((1<<ExponentBits)-2) - ExponentBias;

		const StorageType	s		= StorageType((StorageType(other.signBit())) << (StorageType(ExponentBits+MantissaBits))); // \note Not sign, but sign bit.
		int					e		= other.exponent();
		deUint64			m		= other.mantissa();

		// Normalize denormalized values prior to conversion.
		while (!(m & (1ull<<OtherMantissaBits)))
		{
			m <<= 1;
			e  -= 1;
		}

		if (e < eMin)
		{
			// Underflow.
			if ((Flags & FLOAT_SUPPORT_DENORM) && (eMin-e-1 <= MantissaBits))
			{
				// Shift and round (RTE).
				int			bitDiff	= (OtherMantissaBits-MantissaBits) + (eMin-e);
				deUint64	half	= (1ull << (bitDiff - 1)) - 1;
				deUint64	bias	= (m >> bitDiff) & 1;

				return Float(StorageType(s | (m + half + bias) >> bitDiff));
			}
			else
				return zero(other.sign());
		}
		else
		{
			// Remove leading 1.
			m = m & ~(1ull<<OtherMantissaBits);

			if (MantissaBits < OtherMantissaBits)
			{
				// Round mantissa (round to nearest even).
				int			bitDiff	= OtherMantissaBits-MantissaBits;
				deUint64	half	= (1ull << (bitDiff - 1)) - 1;
				deUint64	bias	= (m >> bitDiff) & 1;

				m = (m + half + bias) >> bitDiff;

				if (m & (1ull<<MantissaBits))
				{
					// Overflow in mantissa.
					m  = 0;
					e += 1;
				}
			}
			else
			{
				int bitDiff = MantissaBits-OtherMantissaBits;
				m = m << bitDiff;
			}

			if (e > eMax)
			{
				// Overflow.
				return inf(other.sign());
			}
			else
			{
				DE_ASSERT(de::inRange(e, eMin, eMax));
				DE_ASSERT(((e + ExponentBias) & ~((1ull<<ExponentBits)-1)) == 0);
				DE_ASSERT((m & ~((1ull<<MantissaBits)-1)) == 0);

				return Float(StorageType(s | (StorageType(e + ExponentBias) << MantissaBits) | m));
			}
		}
	}
}

} // tcu

#endif // _TCUFLOAT_HPP
