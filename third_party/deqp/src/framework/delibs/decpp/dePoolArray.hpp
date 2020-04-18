#ifndef _DEPOOLARRAY_HPP
#define _DEPOOLARRAY_HPP
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
 * \brief Array template backed by memory pool.
 *//*--------------------------------------------------------------------*/

#include "deDefs.hpp"
#include "deMemPool.hpp"
#include "deInt32.h"

#include <iterator>

namespace de
{

//! Self-test for PoolArray
void PoolArray_selfTest (void);

template<typename T, deUint32 Alignment>
class PoolArrayConstIterator;

template<typename T, deUint32 Alignment>
class PoolArrayIterator;

/*--------------------------------------------------------------------*//*!
 * \brief Array template backed by memory pool
 *
 * \note Memory in PoolArray is not contiguous so pointer arithmetic
 *       to access next element(s) doesn't work.
 * \todo [2013-02-11 pyry] Make elements per page template argument.
 *//*--------------------------------------------------------------------*/
template<typename T, deUint32 Alignment = (sizeof(T) > sizeof(void*) ? (deUint32)sizeof(void*) : (deUint32)sizeof(T))>
class PoolArray
{
public:
	typedef PoolArrayIterator<T, Alignment>			Iterator;
	typedef PoolArrayConstIterator<T, Alignment>	ConstIterator;

	typedef Iterator								iterator;
	typedef ConstIterator							const_iterator;

	explicit		PoolArray			(MemPool* pool);
					PoolArray			(MemPool* pool, const PoolArray<T, Alignment>& other);
					~PoolArray			(void);

	void			clear				(void);

	void			reserve				(deUintptr capacity);
	void			resize				(deUintptr size);
	void			resize				(deUintptr size, const T& value);

	deUintptr		size				(void) const			{ return m_numElements;		}
	bool			empty				(void) const			{ return m_numElements == 0;}

	void			pushBack			(const T& value);
	T				popBack				(void);

	const T&		at					(deIntptr ndx) const	{ return *getPtr(ndx);		}
	T&				at					(deIntptr ndx)			{ return *getPtr(ndx);		}

	const T&		operator[]			(deIntptr ndx) const	{ return at(ndx);			}
	T&				operator[]			(deIntptr ndx)			{ return at(ndx);			}

	Iterator		begin				(void)					{ return Iterator(this, 0);								}
	Iterator		end					(void)					{ return Iterator(this, (deIntptr)m_numElements);		}

	ConstIterator	begin				(void) const			{ return ConstIterator(this, 0);						}
	ConstIterator	end					(void) const			{ return ConstIterator(this, (deIntptr)m_numElements);	}

	const T&		front				(void) const			{ return at(0); }
	T&				front				(void)					{ return at(0); }

	const T&		back				(void) const			{ return at(m_numElements-1); }
	T&				back				(void)					{ return at(m_numElements-1); }

private:
	enum
	{
		ELEMENTS_PER_PAGE_LOG2	= 4			//!< 16 elements per page.
	};

					PoolArray			(const PoolArray<T, Alignment>& other); // \note Default copy ctor is not allowed, use PoolArray(pool, copy) instead.

	T*				getPtr				(deIntptr ndx) const;

	MemPool*		m_pool;

	deUintptr		m_numElements;			//!< Number of elements in the array.
	deUintptr		m_capacity;				//!< Number of allocated elements in the array.

	deUintptr		m_pageTableCapacity;	//!< Size of the page table.
	void**			m_pageTable;			//!< Pointer to the page table.
};

template<typename T, deUint32 Alignment>
class PoolArrayIteratorBase
{
public:
						PoolArrayIteratorBase		(deUintptr ndx) : m_ndx(ndx) {}
						~PoolArrayIteratorBase		(void) {}

	deIntptr			getNdx						(void) const throw() { return m_ndx;	}

protected:
	deIntptr			m_ndx;
};

template<typename T, deUint32 Alignment>
class PoolArrayConstIterator : public PoolArrayIteratorBase<T, Alignment>
{
public:
											PoolArrayConstIterator		(void);
											PoolArrayConstIterator		(const PoolArray<T, Alignment>* array, deIntptr ndx);
											PoolArrayConstIterator		(const PoolArrayIterator<T, Alignment>& iterator);
											~PoolArrayConstIterator		(void);

	// \note Default assignment and copy-constructor are auto-generated.

	const PoolArray<T, Alignment>*			getArray	(void) const throw() { return m_array;	}

	// De-reference operators.
	const T*								operator->	(void) const throw()			{ return &(*m_array)[this->m_ndx];		}
	const T&								operator*	(void) const throw()			{ return (*m_array)[this->m_ndx];		}
	const T&								operator[]	(deUintptr offs) const throw()	{ return (*m_array)[this->m_ndx+offs];	}

	// Pre-increment and decrement.
	PoolArrayConstIterator<T, Alignment>&	operator++	(void)	{ this->m_ndx += 1; return *this;	}
	PoolArrayConstIterator<T, Alignment>&	operator--	(void)	{ this->m_ndx -= 1; return *this;	}

	// Post-increment and decrement.
	PoolArrayConstIterator<T, Alignment>	operator++	(int)	{ PoolArrayConstIterator<T, Alignment> copy(*this); this->m_ndx +=1; return copy; }
	PoolArrayConstIterator<T, Alignment>	operator--	(int)	{ PoolArrayConstIterator<T, Alignment> copy(*this); this->m_ndx -=1; return copy; }

	// Compound assignment.
	PoolArrayConstIterator<T, Alignment>&	operator+=	(deIntptr offs)	{ this->m_ndx += offs; return *this; }
	PoolArrayConstIterator<T, Alignment>&	operator-=	(deIntptr offs)	{ this->m_ndx -= offs; return *this; }

	// Assignment from non-const.
	PoolArrayConstIterator<T, Alignment>&	operator=	(const PoolArrayIterator<T, Alignment>& iter);

private:
	const PoolArray<T, Alignment>*			m_array;
};

template<typename T, deUint32 Alignment>
class PoolArrayIterator : public PoolArrayIteratorBase<T, Alignment>
{
public:
										PoolArrayIterator	(void);
										PoolArrayIterator	(PoolArray<T, Alignment>* array, deIntptr ndx);
										~PoolArrayIterator	(void);

	// \note Default assignment and copy-constructor are auto-generated.

	PoolArray<T, Alignment>*			getArray	(void) const throw() { return m_array;	}

	// De-reference operators.
	T*									operator->	(void) const throw()			{ return &(*m_array)[this->m_ndx];		}
	T&									operator*	(void) const throw()			{ return (*m_array)[this->m_ndx];		}
	T&									operator[]	(deUintptr offs) const throw()	{ return (*m_array)[this->m_ndx+offs];	}

	// Pre-increment and decrement.
	PoolArrayIterator<T, Alignment>&	operator++	(void)	{ this->m_ndx += 1; return *this;	}
	PoolArrayIterator<T, Alignment>&	operator--	(void)	{ this->m_ndx -= 1; return *this;	}

	// Post-increment and decrement.
	PoolArrayIterator<T, Alignment>		operator++	(int)	{ PoolArrayIterator<T, Alignment> copy(*this); this->m_ndx +=1; return copy; }
	PoolArrayIterator<T, Alignment>		operator--	(int)	{ PoolArrayIterator<T, Alignment> copy(*this); this->m_ndx -=1; return copy; }

	// Compound assignment.
	PoolArrayIterator<T, Alignment>&	operator+=	(deIntptr offs)	{ this->m_ndx += offs; return *this; }
	PoolArrayIterator<T, Alignment>&	operator-=	(deIntptr offs)	{ this->m_ndx -= offs; return *this; }

private:
	PoolArray<T, Alignment>*			m_array;
};

// Initializer helper for array.
template<typename T>
struct PoolArrayElement
{
	static void constructDefault	(void* ptr)					{ new (ptr) T();	}	//!< Called for non-initialized memory.
	static void	constructCopy		(void* ptr, const T& val)	{ new (ptr) T(val);	}	//!< Called for non-initialized memory when initial value is provided.
	static void destruct			(T* ptr)					{ ptr->~T();		}	//!< Called when element is destructed.
};

// Specialization for basic types.
#define DE_SPECIALIZE_POOL_ARRAY_ELEMENT_BASIC_TYPE(TYPE)							\
template<> struct PoolArrayElement<TYPE> {											\
	static void constructDefault	(void*)					{}						\
	static void constructCopy		(void* ptr, TYPE val)	{ *(TYPE*)ptr = val; }	\
	static void destruct			(TYPE*)					{}						\
}

DE_SPECIALIZE_POOL_ARRAY_ELEMENT_BASIC_TYPE(deUint8);
DE_SPECIALIZE_POOL_ARRAY_ELEMENT_BASIC_TYPE(deUint16);
DE_SPECIALIZE_POOL_ARRAY_ELEMENT_BASIC_TYPE(deUint32);
DE_SPECIALIZE_POOL_ARRAY_ELEMENT_BASIC_TYPE(deUint64);
DE_SPECIALIZE_POOL_ARRAY_ELEMENT_BASIC_TYPE(deInt8);
DE_SPECIALIZE_POOL_ARRAY_ELEMENT_BASIC_TYPE(deInt16);
DE_SPECIALIZE_POOL_ARRAY_ELEMENT_BASIC_TYPE(deInt32);
DE_SPECIALIZE_POOL_ARRAY_ELEMENT_BASIC_TYPE(deInt64);

// PoolArray<T> implementation.

template<typename T, deUint32 Alignment>
PoolArray<T, Alignment>::PoolArray (MemPool* pool)
	: m_pool				(pool)
	, m_numElements			(0)
	, m_capacity			(0)
	, m_pageTableCapacity	(0)
	, m_pageTable			(0)
{
	DE_ASSERT(deIsPowerOfTwo32(Alignment));
}

template<typename T, deUint32 Alignment>
PoolArray<T, Alignment>::~PoolArray (void)
{
	// Clear resets values to T()
	clear();
}

template<typename T, deUint32 Alignment>
inline void PoolArray<T, Alignment>::clear (void)
{
	resize(0);
}

template<typename T, deUint32 Alignment>
inline void PoolArray<T, Alignment>::resize (deUintptr newSize)
{
	if (newSize < m_numElements)
	{
		// Destruct elements that are no longer active.
		for (deUintptr ndx = newSize; ndx < m_numElements; ndx++)
			PoolArrayElement<T>::destruct(getPtr(ndx));

		m_numElements = newSize;
	}
	else if (newSize > m_numElements)
	{
		deUintptr prevSize = m_numElements;

		reserve(newSize);
		m_numElements = newSize;

		// Fill new elements with default values
		for (deUintptr ndx = prevSize; ndx < m_numElements; ndx++)
			PoolArrayElement<T>::constructDefault(getPtr(ndx));
	}
}

template<typename T, deUint32 Alignment>
inline void PoolArray<T, Alignment>::resize (deUintptr newSize, const T& value)
{
	if (newSize < m_numElements)
		resize(newSize); // value is not used
	else if (newSize > m_numElements)
	{
		deUintptr prevSize = m_numElements;

		reserve(newSize);
		m_numElements = newSize;

		// Fill new elements with copies of value
		for (deUintptr ndx = prevSize; ndx < m_numElements; ndx++)
			PoolArrayElement<T>::constructCopy(getPtr(ndx), value);
	}
}

template<typename T, deUint32 Alignment>
inline void PoolArray<T, Alignment>::reserve (deUintptr capacity)
{
	if (capacity >= m_capacity)
	{
		void*		oldPageTable			= DE_NULL;
		deUintptr	oldPageTableSize		= 0;

		deUintptr	newCapacity				= (deUintptr)deAlignPtr((void*)capacity, 1 << ELEMENTS_PER_PAGE_LOG2);
		deUintptr	reqPageTableCapacity	= newCapacity >> ELEMENTS_PER_PAGE_LOG2;

		if (m_pageTableCapacity < reqPageTableCapacity)
		{
			deUintptr		newPageTableCapacity	= max(2*m_pageTableCapacity, reqPageTableCapacity);
			void**			newPageTable			= (void**)m_pool->alloc(newPageTableCapacity * sizeof(void*));
			deUintptr		i;

			for (i = 0; i < m_pageTableCapacity; i++)
				newPageTable[i] = m_pageTable[i];

			for (; i < newPageTableCapacity; i++)
				newPageTable[i] = DE_NULL;

			// Grab information about old page table for recycling purposes.
			oldPageTable		= m_pageTable;
			oldPageTableSize	= m_pageTableCapacity * sizeof(T*);

			m_pageTable			= newPageTable;
			m_pageTableCapacity	= newPageTableCapacity;
		}

		// Allocate new pages.
		{
			deUintptr	elementSize		= (deUintptr)deAlignPtr((void*)(deUintptr)sizeof(T), Alignment);
			deUintptr	pageAllocSize	= elementSize << ELEMENTS_PER_PAGE_LOG2;
			deUintptr	pageTableNdx	= m_capacity >> ELEMENTS_PER_PAGE_LOG2;

			// Allocate new pages from recycled old page table.
			for (;;)
			{
				void*		newPage			= deAlignPtr(oldPageTable, Alignment);
				deUintptr	alignPadding	= (deUintptr)newPage - (deUintptr)oldPageTable;

				if (oldPageTableSize < pageAllocSize+alignPadding)
					break; // No free space for alloc + alignment.

				DE_ASSERT(m_pageTableCapacity > pageTableNdx);
				DE_ASSERT(!m_pageTable[pageTableNdx]);
				m_pageTable[pageTableNdx++] = newPage;

				oldPageTable		 = (void*)((deUint8*)newPage + pageAllocSize);
				oldPageTableSize	-= pageAllocSize+alignPadding;
			}

			// Allocate the rest of the needed pages from the pool.
			for (; pageTableNdx < reqPageTableCapacity; pageTableNdx++)
			{
				DE_ASSERT(!m_pageTable[pageTableNdx]);
				m_pageTable[pageTableNdx] = m_pool->alignedAlloc(pageAllocSize, Alignment);
			}

			m_capacity = pageTableNdx << ELEMENTS_PER_PAGE_LOG2;
			DE_ASSERT(m_capacity >= newCapacity);
		}
	}
}

template<typename T, deUint32 Alignment>
inline void PoolArray<T, Alignment>::pushBack (const T& value)
{
	resize(size()+1);
	at(size()-1) = value;
}

template<typename T, deUint32 Alignment>
inline T PoolArray<T, Alignment>::popBack (void)
{
	T val = at(size()-1);
	resize(size()-1);
	return val;
}

template<typename T, deUint32 Alignment>
inline T* PoolArray<T, Alignment>::getPtr (deIntptr ndx) const
{
	DE_ASSERT(inBounds<deIntptr>(ndx, 0, (deIntptr)m_numElements));
	deUintptr	pageNdx		= ((deUintptr)ndx >> ELEMENTS_PER_PAGE_LOG2);
	deUintptr	subNdx		= (deUintptr)ndx & ((1 << ELEMENTS_PER_PAGE_LOG2) - 1);
	deUintptr	elemSize	= (deUintptr)deAlignPtr((void*)(deUintptr)sizeof(T), Alignment);
	T*			ptr			= (T*)((deUint8*)m_pageTable[pageNdx] + (subNdx*elemSize));
	DE_ASSERT(deIsAlignedPtr(ptr, Alignment));
	return ptr;
}

// PoolArrayIteratorBase implementation

template<typename T, deUint32 Alignment>
inline bool operator== (const PoolArrayIteratorBase<T, Alignment>& a, const PoolArrayIteratorBase<T, Alignment>& b)
{
	// \todo [2013-02-08 pyry] Compare array ptr.
	return a.getNdx() == b.getNdx();
}

template<typename T, deUint32 Alignment>
inline bool operator!= (const PoolArrayIteratorBase<T, Alignment>& a, const PoolArrayIteratorBase<T, Alignment>& b)
{
	// \todo [2013-02-08 pyry] Compare array ptr.
	return a.getNdx() != b.getNdx();
}

template<typename T, deUint32 Alignment>
inline bool operator< (const PoolArrayIteratorBase<T, Alignment>& a, const PoolArrayIteratorBase<T, Alignment>& b)
{
	return a.getNdx() < b.getNdx();
}

template<typename T, deUint32 Alignment>
inline bool operator> (const PoolArrayIteratorBase<T, Alignment>& a, const PoolArrayIteratorBase<T, Alignment>& b)
{
	return a.getNdx() > b.getNdx();
}

template<typename T, deUint32 Alignment>
inline bool operator<= (const PoolArrayIteratorBase<T, Alignment>& a, const PoolArrayIteratorBase<T, Alignment>& b)
{
	return a.getNdx() <= b.getNdx();
}

template<typename T, deUint32 Alignment>
inline bool operator>= (const PoolArrayIteratorBase<T, Alignment>& a, const PoolArrayIteratorBase<T, Alignment>& b)
{
	return a.getNdx() >= b.getNdx();
}

// PoolArrayConstIterator<T> implementation

template<typename T, deUint32 Alignment>
inline PoolArrayConstIterator<T, Alignment>::PoolArrayConstIterator (void)
	: PoolArrayIteratorBase<T, Alignment>	(0)
	, m_array								(DE_NULL)
{
}

template<typename T, deUint32 Alignment>
inline PoolArrayConstIterator<T, Alignment>::PoolArrayConstIterator (const PoolArray<T, Alignment>* array, deIntptr ndx)
	: PoolArrayIteratorBase<T, Alignment>	(ndx)
	, m_array								(array)
{
}

template<typename T, deUint32 Alignment>
inline PoolArrayConstIterator<T, Alignment>::PoolArrayConstIterator (const PoolArrayIterator<T, Alignment>& iter)
	: PoolArrayIteratorBase<T, Alignment>	(iter)
	, m_array								(iter.getArray())
{
}

template<typename T, deUint32 Alignment>
inline PoolArrayConstIterator<T, Alignment>::~PoolArrayConstIterator (void)
{
}

// Arithmetic operators.

template<typename T, deUint32 Alignment>
inline PoolArrayConstIterator<T, Alignment> operator+ (const PoolArrayConstIterator<T, Alignment>& iter, deIntptr offs)
{
	return PoolArrayConstIterator<T, Alignment>(iter->getArray(), iter->getNdx()+offs);
}

template<typename T, deUint32 Alignment>
inline PoolArrayConstIterator<T, Alignment> operator+ (deUintptr offs, const PoolArrayConstIterator<T, Alignment>& iter)
{
	return PoolArrayConstIterator<T, Alignment>(iter->getArray(), iter->getNdx()+offs);
}

template<typename T, deUint32 Alignment>
PoolArrayConstIterator<T, Alignment> operator- (const PoolArrayConstIterator<T, Alignment>& iter, deIntptr offs)
{
	return PoolArrayConstIterator<T, Alignment>(iter.getArray(), iter.getNdx()-offs);
}

template<typename T, deUint32 Alignment>
deIntptr operator- (const PoolArrayConstIterator<T, Alignment>& iter, const PoolArrayConstIterator<T, Alignment>& other)
{
	return iter.getNdx()-other.getNdx();
}

// PoolArrayIterator<T> implementation.

template<typename T, deUint32 Alignment>
inline PoolArrayIterator<T, Alignment>::PoolArrayIterator (void)
	: PoolArrayIteratorBase<T, Alignment>	(0)
	, m_array								(DE_NULL)
{
}

template<typename T, deUint32 Alignment>
inline PoolArrayIterator<T, Alignment>::PoolArrayIterator (PoolArray<T, Alignment>* array, deIntptr ndx)
	: PoolArrayIteratorBase<T, Alignment>	(ndx)
	, m_array								(array)
{
}

template<typename T, deUint32 Alignment>
inline PoolArrayIterator<T, Alignment>::~PoolArrayIterator (void)
{
}

// Arithmetic operators.

template<typename T, deUint32 Alignment>
inline PoolArrayIterator<T, Alignment> operator+ (const PoolArrayIterator<T, Alignment>& iter, deIntptr offs)
{
	return PoolArrayIterator<T, Alignment>(iter.getArray(), iter.getNdx()+offs);
}

template<typename T, deUint32 Alignment>
inline PoolArrayIterator<T, Alignment> operator+ (deUintptr offs, const PoolArrayIterator<T, Alignment>& iter)
{
	return PoolArrayIterator<T, Alignment>(iter.getArray(), iter.getNdx()+offs);
}

template<typename T, deUint32 Alignment>
PoolArrayIterator<T, Alignment> operator- (const PoolArrayIterator<T, Alignment>& iter, deIntptr offs)
{
	return PoolArrayIterator<T, Alignment>(iter.getArray(), iter.getNdx()-offs);
}

template<typename T, deUint32 Alignment>
deIntptr operator- (const PoolArrayIterator<T, Alignment>& iter, const PoolArrayIterator<T, Alignment>& other)
{
	return iter.getNdx()-other.getNdx();
}

} // de

// std::iterator_traits specializations
namespace std
{

template<typename T, deUint32 Alignment>
struct iterator_traits<de::PoolArrayConstIterator<T, Alignment> >
{
	typedef deIntptr					difference_type;
	typedef T							value_type;
	typedef const T*					pointer;
	typedef const T&					reference;
	typedef random_access_iterator_tag	iterator_category;
};

template<typename T, deUint32 Alignment>
struct iterator_traits<de::PoolArrayIterator<T, Alignment> >
{
	typedef deIntptr					difference_type;
	typedef T							value_type;
	typedef T*							pointer;
	typedef T&							reference;
	typedef random_access_iterator_tag	iterator_category;
};

} // std

#endif // _DEPOOLARRAY_HPP
