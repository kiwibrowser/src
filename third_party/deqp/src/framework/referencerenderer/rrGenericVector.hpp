#ifndef _RRGENERICVECTOR_HPP
#define _RRGENERICVECTOR_HPP
/*-------------------------------------------------------------------------
 * drawElements Quality Program Reference Renderer
 * -----------------------------------------------
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
 * \brief Generic vetor
 *//*--------------------------------------------------------------------*/

#include "rrDefs.hpp"
#include "tcuVector.hpp"

namespace rr
{

enum GenericVecType
{
	GENERICVECTYPE_FLOAT = 0,
	GENERICVECTYPE_UINT32,
	GENERICVECTYPE_INT32,

	GENERICVECTYPE_LAST
};

/*--------------------------------------------------------------------*//*!
 * \brief Generic vertex attrib
 *
 * Generic vertex attributes hold 4 32-bit scalar values that can be accessed
 * as floating-point or integer values.
 *
 * Aliasing rules must be adhered when accessing data (ie. writing as float
 * and reading as int has undefined result).
 *//*--------------------------------------------------------------------*/
class GenericVec4
{
private:
	union
	{
		deUint32 uData[4];
		deInt32  iData[4];
		float    fData[4];
	} v;

public:
	inline GenericVec4 (void)
	{
		v.iData[0] = 0;
		v.iData[1] = 0;
		v.iData[2] = 0;
		v.iData[3] = 0;
	}

	template<typename ScalarType>
	explicit GenericVec4 (const tcu::Vector<ScalarType, 4>& value)
	{
		*this = value;
	}

	GenericVec4& operator= (const GenericVec4& value)
	{
		v.iData[0] = value.v.iData[0];
		v.iData[1] = value.v.iData[1];
		v.iData[2] = value.v.iData[2];
		v.iData[3] = value.v.iData[3];
		return *this;
	}

	template<typename ScalarType>
	GenericVec4& operator= (const tcu::Vector<ScalarType, 4>& value)
	{
		getAccess<ScalarType>()[0] = value[0];
		getAccess<ScalarType>()[1] = value[1];
		getAccess<ScalarType>()[2] = value[2];
		getAccess<ScalarType>()[3] = value[3];
		return *this;
	}

	template<typename ScalarType>
	inline tcu::Vector<ScalarType, 4> get (void) const
	{
		return tcu::Vector<ScalarType, 4>(
			getAccess<ScalarType>()[0],
			getAccess<ScalarType>()[1],
			getAccess<ScalarType>()[2],
			getAccess<ScalarType>()[3]);
	}

	template<typename ScalarType>
	inline ScalarType* getAccess ();

	template<typename ScalarType>
	inline const ScalarType* getAccess () const;
} DE_WARN_UNUSED_TYPE;

template<>
inline float* GenericVec4::getAccess<float> ()
{
	return v.fData;
}

template<>
inline const float* GenericVec4::getAccess<float> () const
{
	return v.fData;
}

template<>
inline deUint32* GenericVec4::getAccess<deUint32> ()
{
	return v.uData;
}

template<>
inline const deUint32* GenericVec4::getAccess<deUint32> () const
{
	return v.uData;
}

template<>
inline deInt32* GenericVec4::getAccess<deInt32> ()
{
	return v.iData;
}

template<>
inline const deInt32* GenericVec4::getAccess<deInt32> () const
{
	return v.iData;
}

} // rr

#endif // _RRGENERICVECTOR_HPP
