#ifndef _DEATOMIC_H
#define _DEATOMIC_H
/*-------------------------------------------------------------------------
 * drawElements Thread Library
 * ---------------------------
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
 * \brief Atomic operations.
 *//*--------------------------------------------------------------------*/

#include "deDefs.h"

#if (DE_COMPILER == DE_COMPILER_MSC)
#	include <intrin.h>
#endif

DE_BEGIN_EXTERN_C

/*--------------------------------------------------------------------*//*!
 * \brief Atomic increment and fetch 32-bit signed integer.
 * \param dstAddr	Destination address.
 * \return Incremented value.
 *//*--------------------------------------------------------------------*/
DE_INLINE deInt32 deAtomicIncrementInt32 (volatile deInt32* dstAddr)
{
#if (DE_COMPILER == DE_COMPILER_MSC)
	return _InterlockedIncrement((long volatile*)dstAddr);
#elif (DE_COMPILER == DE_COMPILER_GCC) || (DE_COMPILER == DE_COMPILER_CLANG)
	return __sync_add_and_fetch(dstAddr, 1);
#else
#	error "Implement deAtomicIncrementInt32()"
#endif
}

/*--------------------------------------------------------------------*//*!
 * \brief Atomic increment and fetch 32-bit unsigned integer.
 * \param dstAddr	Destination address.
 * \return Incremented value.
 *//*--------------------------------------------------------------------*/
DE_INLINE deUint32 deAtomicIncrementUint32 (volatile deUint32* dstAddr)
{
	return deAtomicIncrementInt32((deInt32 volatile*)dstAddr);
}

/*--------------------------------------------------------------------*//*!
 * \brief Atomic decrement and fetch 32-bit signed integer.
 * \param dstAddr	Destination address.
 * \return Decremented value.
 *//*--------------------------------------------------------------------*/
DE_INLINE deInt32 deAtomicDecrementInt32 (volatile deInt32* dstAddr)
{
#if (DE_COMPILER == DE_COMPILER_MSC)
	return _InterlockedDecrement((volatile long*)dstAddr);
#elif (DE_COMPILER == DE_COMPILER_GCC) || (DE_COMPILER == DE_COMPILER_CLANG)
	return __sync_sub_and_fetch(dstAddr, 1);
#else
#	error "Implement deAtomicDecrementInt32()"
#endif
}

/*--------------------------------------------------------------------*//*!
 * \brief Atomic decrement and fetch 32-bit unsigned integer.
 * \param dstAddr	Destination address.
 * \return Decremented value.
 *//*--------------------------------------------------------------------*/
DE_INLINE deUint32 deAtomicDecrementUint32 (volatile deUint32* dstAddr)
{
	return deAtomicDecrementInt32((volatile deInt32*)dstAddr);
}

/*--------------------------------------------------------------------*//*!
 * \brief Atomic compare and exchange (CAS) 32-bit value.
 * \param dstAddr	Destination address.
 * \param compare	Old value.
 * \param exchange	New value.
 * \return			compare value if CAS passes, *dstAddr value otherwise
 *
 * Performs standard Compare-And-Swap with 32b data. Dst value is compared
 * to compare value and if that comparison passes, value is replaced with
 * exchange value.
 *
 * If CAS succeeds, compare value is returned. Otherwise value stored in
 * dstAddr is returned.
 *//*--------------------------------------------------------------------*/
DE_INLINE deUint32 deAtomicCompareExchangeUint32 (volatile deUint32* dstAddr, deUint32 compare, deUint32 exchange)
{
#if (DE_COMPILER == DE_COMPILER_MSC)
	return _InterlockedCompareExchange((volatile long*)dstAddr, exchange, compare);
#elif (DE_COMPILER == DE_COMPILER_GCC) || (DE_COMPILER == DE_COMPILER_CLANG)
	return __sync_val_compare_and_swap(dstAddr, compare, exchange);
#else
#	error "Implement deAtomicCompareExchange32()"
#endif
}

/* Deprecated names */
#define deAtomicIncrement32			deAtomicIncrementInt32
#define deAtomicDecrement32			deAtomicDecrementInt32
#define deAtomicCompareExchange32	deAtomicCompareExchangeUint32

#if (DE_PTR_SIZE == 8)

/*--------------------------------------------------------------------*//*!
 * \brief Atomic increment and fetch 64-bit signed integer.
 * \param dstAddr	Destination address.
 * \return Incremented value.
 *//*--------------------------------------------------------------------*/
DE_INLINE deInt64 deAtomicIncrementInt64 (volatile deInt64* dstAddr)
{
#if (DE_COMPILER == DE_COMPILER_MSC)
	return _InterlockedIncrement64((volatile long long*)dstAddr);
#elif (DE_COMPILER == DE_COMPILER_GCC) || (DE_COMPILER == DE_COMPILER_CLANG)
	return __sync_add_and_fetch(dstAddr, 1);
#else
#	error "Implement deAtomicIncrementInt64()"
#endif
}

/*--------------------------------------------------------------------*//*!
 * \brief Atomic increment and fetch 64-bit unsigned integer.
 * \param dstAddr	Destination address.
 * \return Incremented value.
 *//*--------------------------------------------------------------------*/
DE_INLINE deUint64 deAtomicIncrementUint64 (volatile deUint64* dstAddr)
{
	return deAtomicIncrementInt64((volatile deInt64*)dstAddr);
}

/*--------------------------------------------------------------------*//*!
 * \brief Atomic decrement and fetch.
 * \param dstAddr	Destination address.
 * \return Decremented value.
 *//*--------------------------------------------------------------------*/
DE_INLINE deInt64 deAtomicDecrementInt64 (volatile deInt64* dstAddr)
{
#if (DE_COMPILER == DE_COMPILER_MSC)
	return _InterlockedDecrement64((volatile long long*)dstAddr);
#elif (DE_COMPILER == DE_COMPILER_GCC) || (DE_COMPILER == DE_COMPILER_CLANG)
	return __sync_sub_and_fetch(dstAddr, 1);
#else
#	error "Implement deAtomicDecrementInt64()"
#endif
}

/*--------------------------------------------------------------------*//*!
 * \brief Atomic increment and fetch 64-bit unsigned integer.
 * \param dstAddr	Destination address.
 * \return Incremented value.
 *//*--------------------------------------------------------------------*/
DE_INLINE deUint64 deAtomicDecrementUint64 (volatile deUint64* dstAddr)
{
	return deAtomicDecrementInt64((volatile deInt64*)dstAddr);
}

/*--------------------------------------------------------------------*//*!
 * \brief Atomic compare and exchange (CAS) 64-bit value.
 * \param dstAddr	Destination address.
 * \param compare	Old value.
 * \param exchange	New value.
 * \return			compare value if CAS passes, *dstAddr value otherwise
 *
 * Performs standard Compare-And-Swap with 64b data. Dst value is compared
 * to compare value and if that comparison passes, value is replaced with
 * exchange value.
 *
 * If CAS succeeds, compare value is returned. Otherwise value stored in
 * dstAddr is returned.
 *//*--------------------------------------------------------------------*/
DE_INLINE deUint64 deAtomicCompareExchangeUint64 (volatile deUint64* dstAddr, deUint64 compare, deUint64 exchange)
{
#if (DE_COMPILER == DE_COMPILER_MSC)
	return _InterlockedCompareExchange64((volatile long long*)dstAddr, exchange, compare);
#elif (DE_COMPILER == DE_COMPILER_GCC) || (DE_COMPILER == DE_COMPILER_CLANG)
	return __sync_val_compare_and_swap(dstAddr, compare, exchange);
#else
#	error "Implement deAtomicCompareExchangeUint64()"
#endif
}

#endif /* (DE_PTR_SIZE == 8) */

/*--------------------------------------------------------------------*//*!
 * \brief Atomic increment and fetch size_t.
 * \param dstAddr	Destination address.
 * \return Incremented value.
 *//*--------------------------------------------------------------------*/
DE_INLINE size_t deAtomicIncrementUSize (volatile size_t* size)
{
#if (DE_PTR_SIZE == 8)
	return deAtomicIncrementUint64((volatile deUint64*)size);
#elif (DE_PTR_SIZE == 4)
	return deAtomicIncrementUint32((volatile deUint32*)size);
#else
#	error "Invalid DE_PTR_SIZE value"
#endif
}

/*--------------------------------------------------------------------*//*!
 * \brief Atomic increment and fetch size_t.
 * \param dstAddr	Destination address.
 * \return Incremented value.
 *//*--------------------------------------------------------------------*/
DE_INLINE size_t deAtomicDecrementUSize (volatile size_t* size)
{
#if (DE_PTR_SIZE == 8)
	return deAtomicDecrementUint64((volatile deUint64*)size);
#elif (DE_PTR_SIZE == 4)
	return deAtomicDecrementUint32((volatile deUint32*)size);
#else
#	error "Invalid DE_PTR_SIZE value"
#endif
}

/*--------------------------------------------------------------------*//*!
 * \brief Atomic compare and exchange (CAS) pointer.
 * \param dstAddr	Destination address.
 * \param compare	Old value.
 * \param exchange	New value.
 * \return			compare value if CAS passes, *dstAddr value otherwise
 *
 * Performs standard Compare-And-Swap with pointer value. Dst value is compared
 * to compare value and if that comparison passes, value is replaced with
 * exchange value.
 *
 * If CAS succeeds, compare value is returned. Otherwise value stored in
 * dstAddr is returned.
 *//*--------------------------------------------------------------------*/
DE_INLINE void* deAtomicCompareExchangePtr (void* volatile* dstAddr, void* compare, void* exchange)
{
#if (DE_PTR_SIZE == 8)
	return (void*)deAtomicCompareExchangeUint64((volatile deUint64*)dstAddr, (deUint64)compare, (deUint64)exchange);
#elif (DE_PTR_SIZE == 4)
	return (void*)deAtomicCompareExchangeUint32((volatile deUint32*)dstAddr, (deUint32)compare, (deUint32)exchange);
#else
#	error "Invalid DE_PTR_SIZE value"
#endif
}

/*--------------------------------------------------------------------*//*!
 * \brief Issue hardware memory read-write fence.
 *//*--------------------------------------------------------------------*/
#if (DE_COMPILER == DE_COMPILER_MSC)
void deMemoryReadWriteFence (void);
#elif (DE_COMPILER == DE_COMPILER_GCC) || (DE_COMPILER == DE_COMPILER_CLANG)
DE_INLINE void deMemoryReadWriteFence (void)
{
	__sync_synchronize();
}
#else
#	error "Implement deMemoryReadWriteFence()"
#endif

DE_END_EXTERN_C

#endif /* _DEATOMIC_H */
