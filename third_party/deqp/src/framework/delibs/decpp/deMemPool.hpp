#ifndef _DEMEMPOOL_HPP
#define _DEMEMPOOL_HPP
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
 * \brief Memory pool (deMemPool wrapper).
 *//*--------------------------------------------------------------------*/

#include "deDefs.hpp"
#include "deMemPool.h"

#include <new>

namespace de
{

/*--------------------------------------------------------------------*//*!
 * \brief Memory pool
 *//*--------------------------------------------------------------------*/
class MemPool
{
public:
					MemPool					(const deMemPoolUtil* util = DE_NULL, deUint32 flags = 0u);
					MemPool					(MemPool* parent);
					~MemPool				(void);

	deMemPool*		getRawPool				(void)					{ return m_pool;																}

	int				getNumChildren			(void) const			{ return deMemPool_getNumChildren(m_pool);										}

	deUintptr		getNumAllocatedBytes	(bool recurse) const	{ return deMemPool_getNumAllocatedBytes(m_pool, recurse ? DE_TRUE : DE_FALSE);	}
	deUintptr		getCapacity				(bool recurse) const	{ return deMemPool_getCapacity(m_pool, recurse ? DE_TRUE : DE_FALSE);			}

	void*			alloc					(deUintptr numBytes);
	void*			alignedAlloc			(deUintptr numBytes, deUint32 alignBytes);

private:
					MemPool					(const MemPool& other); // Not allowed!
	MemPool&		operator=				(const MemPool& other); // Not allowed!

	deMemPool*		m_pool;
};

// MemPool utils.

char* copyToPool (de::MemPool* pool, const char* string);

// MemPool inline implementations.

inline MemPool::MemPool (const deMemPoolUtil* util, deUint32 flags)
{
	m_pool = deMemPool_createRoot(util, flags);
	if (!m_pool)
		throw std::bad_alloc();
}

inline MemPool::MemPool (MemPool* parent)
{
	m_pool = deMemPool_create(parent->m_pool);
	if (!m_pool)
		throw std::bad_alloc();
}

inline MemPool::~MemPool (void)
{
	deMemPool_destroy(m_pool);
}

inline void* MemPool::alloc (deUintptr numBytes)
{
	// \todo [2013-02-07 pyry] Use deUintptr in deMemPool.
	DE_ASSERT((deUintptr)(int)numBytes == numBytes);
	void* ptr = deMemPool_alloc(m_pool, (int)numBytes);
	if (!ptr)
		throw std::bad_alloc();
	return ptr;
}

inline void* MemPool::alignedAlloc (deUintptr numBytes, deUint32 alignBytes)
{
	// \todo [2013-02-07 pyry] Use deUintptr in deMemPool.
	DE_ASSERT((deUintptr)(int)numBytes == numBytes);
	void* ptr = deMemPool_alignedAlloc(m_pool, (int)numBytes, alignBytes);
	if (!ptr)
		throw std::bad_alloc();
	return ptr;
}

} // de

#endif // _DEMEMPOOL_HPP
