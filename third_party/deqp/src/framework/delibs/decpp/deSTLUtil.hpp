#ifndef _DESTLUTIL_HPP
#define _DESTLUTIL_HPP
/*-------------------------------------------------------------------------
 * drawElements C++ Base Library
 * -----------------------------
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
 * \brief Utilities for STL containers.
 *//*--------------------------------------------------------------------*/

#include "deDefs.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>
#include <iterator>

namespace de
{

void STLUtil_selfTest (void);

//! Test whether `item` is a member of `container`. The type `C` must be an
//! AssociativeContainer.

template<typename C>
inline bool contains (const C& container, const typename C::key_type& item)
{
	const typename C::const_iterator it = container.find(item);
	return (it != container.end());
}

template<typename I, typename K>
inline bool contains (const I& begin, const I& end, const K& item)
{
	const I it = std::find(begin, end, item);
	return (it != end);
}

template<typename C>
C intersection (const C& s1, const C& s2)
{
	C ret;
	std::set_intersection(s1.begin(), s1.end(), s2.begin(), s2.end(),
						  std::insert_iterator<C>(ret, ret.begin()));
	return ret;
}

template<typename C>
C set_union (const C& s1, const C& s2)
{
	C ret;
	std::set_union(s1.begin(), s1.end(), s2.begin(), s2.end(),
				   std::insert_iterator<C>(ret, ret.begin()));
	return ret;
}

// Utilities for map-like container types

//! Return a pointer to the value mapped to `key`, or null if not found.
template <typename M>
const typename M::mapped_type* tryLookup (const M& map,
										  const typename M::key_type& key)
{
	typename M::const_iterator it = map.find(key);
	if (it == map.end())
		return DE_NULL;
	return &it->second;
}

//! Return a reference to the value mapped to `key`, or `fallback` if not found.
template<typename M>
const typename M::mapped_type& lookupDefault (const M&							map,
											  const typename M::key_type&		key,
											  const typename M::mapped_type&	fallback)
{
	const typename M::mapped_type* ptr = tryLookup(map, key);
	return ptr == DE_NULL ? fallback : *ptr;
}

//! Return a reference to the value mapped to `key`, or raise
//! `std::out_of_range` if not found.
template<typename M>
const typename M::mapped_type& lookup (const M& map, const typename M::key_type& key)
{
	const typename M::mapped_type* ptr = tryLookup(map, key);
	if (ptr == DE_NULL)
		throw std::out_of_range("key not found in map");
	return *ptr;
}

//! Map `key` to `value`. This differs from `map[key] = value` in that there
//! is no default construction and assignment involved.
template<typename M>
bool insert (M& map, const typename M::key_type& key, const typename M::mapped_type& value)
{
	typename M::value_type entry(key, value);
	std::pair<typename M::iterator,bool> ret = map.insert(entry);
	return ret.second;
}

} // de

#endif // _DESTLUTIL_HPP
