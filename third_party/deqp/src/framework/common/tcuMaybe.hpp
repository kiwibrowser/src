#ifndef _TCUMAYBE_HPP
#define _TCUMAYBE_HPP
/*-------------------------------------------------------------------------
 * drawElements Quality Program Tester Core
 * ----------------------------------------
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
 * \brief Template for values that may not exist.
 *//*--------------------------------------------------------------------*/

#include "tcuDefs.hpp"

namespace tcu
{

// \note Type T is always aligned to same alignment as deUint64.
// \note This type always uses at least sizeof(T*) + sizeof(deUint64) of memory.
template<typename T>
class Maybe
{
public:
				Maybe			(void);
				~Maybe			(void);

				Maybe			(const T& val);
	Maybe<T>&	operator=		(const T& val);

				Maybe			(const Maybe<T>& other);
	Maybe<T>&	operator=		(const Maybe<T>& other);

	const T&	get				(void) const;
	const T&	operator*		(void) const { return get(); }

	const T*	operator->		(void) const;
				operator bool	(void) const { return !!m_ptr; }

private:
	T*				m_ptr;

	union
	{
		deUint8		m_data[sizeof(T)];
		deUint64	m_align;
	};
} DE_WARN_UNUSED_TYPE;

template<typename T>
Maybe<T> nothing (void)
{
	return Maybe<T>();
}

template<typename T>
Maybe<T> just (const T& value)
{
	return Maybe<T>(value);
}

template<typename T>
Maybe<T>::Maybe (void)
	: m_ptr (DE_NULL)
{
}

template<typename T>
Maybe<T>::~Maybe (void)
{
	if (m_ptr)
		m_ptr->~T();
}

template<typename T>
Maybe<T>::Maybe (const T& val)
	: m_ptr (DE_NULL)
{
	m_ptr = new(m_data)T(val);
}

template<typename T>
Maybe<T>& Maybe<T>::operator= (const T& val)
{
	if (m_ptr)
		m_ptr->~T();

	m_ptr = new(m_data)T(val);

	return *this;
}

template<typename T>
Maybe<T>::Maybe (const Maybe<T>& other)
	: m_ptr (DE_NULL)
{
	if (other.m_ptr)
		m_ptr = new(m_data)T(*other.m_ptr);
}

template<typename T>
Maybe<T>& Maybe<T>::operator= (const Maybe<T>& other)
{
	if (this == &other)
		return *this;

	if (m_ptr)
		m_ptr->~T();

	if (other.m_ptr)
		m_ptr = new(m_data)T(*other.m_ptr);
	else
		m_ptr = DE_NULL;

	return *this;
}

template<typename T>
const T* Maybe<T>::operator-> (void) const
{
	DE_ASSERT(m_ptr);
	return m_ptr;
}

template<typename T>
const T& Maybe<T>::get (void) const
{
	DE_ASSERT(m_ptr);
	return *m_ptr;
}

} // tcu

#endif // _TCUMAYBE_HPP
