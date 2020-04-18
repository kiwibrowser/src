#ifndef _TCURANDOMVALUEITERATOR_HPP
#define _TCURANDOMVALUEITERATOR_HPP
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
 * \brief Random value iterator.
 *//*--------------------------------------------------------------------*/

#include "tcuDefs.hpp"
#include "deRandom.hpp"

namespace tcu
{

template <typename T>
T getRandomValue (de::Random& rnd)
{
	// \note memcpy() is the only valid way to do cast from uint32 to float for instnance.
	deUint8 data[sizeof(T) + sizeof(T)%4];
	DE_STATIC_ASSERT(sizeof(data)%4 == 0);
	for (int vecNdx = 0; vecNdx < DE_LENGTH_OF_ARRAY(data)/4; vecNdx++)
	{
		deUint32 rval = rnd.getUint32();
		for (int compNdx = 0; compNdx < 4; compNdx++)
			data[vecNdx*4+compNdx] = ((const deUint8*)&rval)[compNdx];
	}
	return *(const T*)&data[0];
}

// Faster implementations for int types.
template <> inline deUint8	getRandomValue<deUint8>		(de::Random& rnd) { return (deUint8)rnd.getUint32();	}
template <> inline deUint16	getRandomValue<deUint16>	(de::Random& rnd) { return (deUint16)rnd.getUint32();	}
template <> inline deUint32	getRandomValue<deUint32>	(de::Random& rnd) { return rnd.getUint32();				}
template <> inline deUint64	getRandomValue<deUint64>	(de::Random& rnd) { return rnd.getUint64();				}
template <> inline deInt8	getRandomValue<deInt8>		(de::Random& rnd) { return (deInt8)rnd.getUint32();		}
template <> inline deInt16	getRandomValue<deInt16>		(de::Random& rnd) { return (deInt16)rnd.getUint32();	}
template <> inline deInt32	getRandomValue<deInt32>		(de::Random& rnd) { return (deInt32)rnd.getUint32();	}
template <> inline deInt64	getRandomValue<deInt64>		(de::Random& rnd) { return (deInt64)rnd.getUint64();	}

template <typename T>
class RandomValueIterator : public std::iterator<std::forward_iterator_tag, T>
{
public:
	static RandomValueIterator	begin					(deUint32 seed, int numValues)	{ return RandomValueIterator<T>(seed, numValues);	}
	static RandomValueIterator	end						(void)							{ return RandomValueIterator<T>(0, 0);				}

	RandomValueIterator&		operator++				(void);
	RandomValueIterator			operator++				(int);

	const T&					operator*				(void) const { return m_curVal; }

	bool						operator==				(const RandomValueIterator<T>& other) const;
	bool						operator!=				(const RandomValueIterator<T>& other) const;

private:
								RandomValueIterator		(deUint32 seed, int numLeft);

	de::Random					m_rnd;
	int							m_numLeft;
	T							m_curVal;
};

template <typename T>
RandomValueIterator<T>::RandomValueIterator (deUint32 seed, int numLeft)
	: m_rnd		(seed)
	, m_numLeft	(numLeft)
	, m_curVal	(numLeft > 0 ? getRandomValue<T>(m_rnd) : T())
{
}

template <typename T>
RandomValueIterator<T>& RandomValueIterator<T>::operator++ (void)
{
	DE_ASSERT(m_numLeft > 0);

	m_numLeft	-= 1;
	m_curVal	 = getRandomValue<T>(m_rnd);

	return *this;
}

template <typename T>
RandomValueIterator<T> RandomValueIterator<T>::operator++ (int)
{
	RandomValueIterator copy(*this);
	++(*this);
	return copy;
}

template <typename T>
bool RandomValueIterator<T>::operator== (const RandomValueIterator<T>& other) const
{
	return (m_numLeft == 0 && other.m_numLeft == 0) || (m_numLeft == other.m_numLeft && m_rnd == other.m_rnd);
}

template <typename T>
bool RandomValueIterator<T>::operator!= (const RandomValueIterator<T>& other) const
{
	return !(m_numLeft == 0 && other.m_numLeft == 0) && (m_numLeft != other.m_numLeft || m_rnd != other.m_rnd);
}

} // tcu

#endif // _TCURANDOMVALUEITERATOR_HPP
