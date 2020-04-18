#ifndef _DEPOOLHASH_H
#define _DEPOOLHASH_H
/*-------------------------------------------------------------------------
 * drawElements Memory Pool Library
 * --------------------------------
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
 * \brief Memory pool hash class.
 *//*--------------------------------------------------------------------*/

#include "deDefs.h"
#include "deMemPool.h"
#include "dePoolArray.h"
#include "deInt32.h"

#include <string.h> /* memset() */

enum
{
	DE_HASH_ELEMENTS_PER_SLOT	= 4
};

DE_BEGIN_EXTERN_C

void	dePoolHash_selfTest		(void);

DE_END_EXTERN_C

/*--------------------------------------------------------------------*//*!
 * \brief Declare a template pool hash class interface.
 * \param TYPENAME	Type name of the declared hash.
 * \param KEYTYPE	Type of the key.
 * \param VALUETYPE	Type of the value.
 *
 * This macro declares the interface for a hash. For the implementation of
 * the hash, see DE_IMPLEMENT_POOL_HASH. Usually this macro is put into the
 * header file and the implementation macro is put in some .c file.
 *
 * \todo [petri] Detailed description.
 *
 * The functions for operating the hash are:
 * \todo [petri] Figure out how to comment these in Doxygen-style.
 *
 * \code
 * Hash*    Hash_create            (deMemPool* pool);
 * int      Hash_getNumElements    (const Hash* hash);
 * Value*   Hash_find              (Hash* hash, Key key);
 * deBool   Hash_insert            (Hash* hash, Key key, Value value);
 * void     Hash_delete            (Hash* hash, Key key);
 * \endcode
*//*--------------------------------------------------------------------*/
#define DE_DECLARE_POOL_HASH(TYPENAME, KEYTYPE, VALUETYPE)		\
\
typedef struct TYPENAME##Slot_s TYPENAME##Slot;    \
\
struct TYPENAME##Slot_s \
{    \
	int				numUsed; \
	TYPENAME##Slot*	nextSlot; \
	KEYTYPE			keys[DE_HASH_ELEMENTS_PER_SLOT]; \
	VALUETYPE		values[DE_HASH_ELEMENTS_PER_SLOT]; \
}; \
\
typedef struct TYPENAME##_s    \
{    \
	deMemPool*			pool;				\
	int					numElements;		\
\
	int					slotTableSize;		\
	TYPENAME##Slot**	slotTable;			\
	TYPENAME##Slot*		slotFreeList;		\
} TYPENAME; /* NOLINT(TYPENAME) */			\
\
typedef struct TYPENAME##Iter_s \
{	\
	const TYPENAME*			hash;			\
	int						curSlotIndex;	\
	const TYPENAME##Slot*	curSlot;		\
	int						curElemIndex;	\
} TYPENAME##Iter;	\
\
TYPENAME*	TYPENAME##_create	(deMemPool* pool);											\
void		TYPENAME##_reset	(DE_PTR_TYPE(TYPENAME) hash);								\
deBool		TYPENAME##_reserve	(DE_PTR_TYPE(TYPENAME) hash, int capacity);					\
VALUETYPE*	TYPENAME##_find		(const TYPENAME* hash, KEYTYPE key);						\
deBool		TYPENAME##_insert	(DE_PTR_TYPE(TYPENAME) hash, KEYTYPE key, VALUETYPE value);	\
void		TYPENAME##_delete	(DE_PTR_TYPE(TYPENAME) hash, KEYTYPE key);					\
\
DE_INLINE int		TYPENAME##_getNumElements	(const TYPENAME* hash)							DE_UNUSED_FUNCTION;	\
DE_INLINE void		TYPENAME##Iter_init			(const TYPENAME* hash, TYPENAME##Iter* iter)	DE_UNUSED_FUNCTION;	\
DE_INLINE deBool	TYPENAME##Iter_hasItem		(const TYPENAME##Iter* iter)					DE_UNUSED_FUNCTION;	\
DE_INLINE void		TYPENAME##Iter_next			(TYPENAME##Iter* iter)							DE_UNUSED_FUNCTION;	\
DE_INLINE KEYTYPE	TYPENAME##Iter_getKey		(const TYPENAME##Iter* iter)					DE_UNUSED_FUNCTION;	\
DE_INLINE VALUETYPE	TYPENAME##Iter_getValue		(const TYPENAME##Iter* iter)					DE_UNUSED_FUNCTION;	\
\
DE_INLINE int TYPENAME##_getNumElements (const TYPENAME* hash)    \
{    \
	return hash->numElements;    \
}    \
\
DE_INLINE void TYPENAME##Iter_init (const TYPENAME* hash, TYPENAME##Iter* iter)    \
{	\
	iter->hash			= hash;		\
	iter->curSlotIndex	= 0;		\
	iter->curSlot		= DE_NULL;	\
	iter->curElemIndex	= 0;		\
	if (TYPENAME##_getNumElements(hash) > 0)		\
	{												\
		int slotTableSize	= hash->slotTableSize;	\
		int slotNdx			= 0;					\
		while (slotNdx < slotTableSize)				\
		{											\
			if (hash->slotTable[slotNdx])			\
				break;								\
			slotNdx++;								\
		}											\
		DE_ASSERT(slotNdx < slotTableSize);			\
		iter->curSlotIndex = slotNdx;				\
		iter->curSlot = hash->slotTable[slotNdx];	\
		DE_ASSERT(iter->curSlot);					\
	}	\
}	\
\
DE_INLINE deBool TYPENAME##Iter_hasItem (const TYPENAME##Iter* iter)    \
{	\
	return (iter->curSlot != DE_NULL); \
}	\
\
DE_INLINE void TYPENAME##Iter_next (TYPENAME##Iter* iter)    \
{	\
	DE_ASSERT(TYPENAME##Iter_hasItem(iter));	\
	if (++iter->curElemIndex == iter->curSlot->numUsed)	\
	{													\
		iter->curElemIndex = 0;							\
		if (iter->curSlot->nextSlot)					\
		{												\
			iter->curSlot = iter->curSlot->nextSlot;	\
		}												\
		else											\
		{												\
			const TYPENAME*	hash			= iter->hash;			\
			int				curSlotIndex	= iter->curSlotIndex;	\
			int				slotTableSize	= hash->slotTableSize;	\
			while (++curSlotIndex < slotTableSize)		\
			{											\
				if (hash->slotTable[curSlotIndex])		\
					break;								\
			}											\
			iter->curSlotIndex = curSlotIndex;			\
			if (curSlotIndex < slotTableSize)					\
				iter->curSlot = hash->slotTable[curSlotIndex];	\
			else												\
				iter->curSlot = DE_NULL;						\
		}	\
	}	\
}	\
\
DE_INLINE KEYTYPE TYPENAME##Iter_getKey	(const TYPENAME##Iter* iter)    \
{	\
	DE_ASSERT(TYPENAME##Iter_hasItem(iter));	\
	return iter->curSlot->keys[iter->curElemIndex];	\
}	\
\
DE_INLINE VALUETYPE	TYPENAME##Iter_getValue	(const TYPENAME##Iter* iter)    \
{	\
	DE_ASSERT(TYPENAME##Iter_hasItem(iter));	\
	return iter->curSlot->values[iter->curElemIndex];	\
}	\
\
struct TYPENAME##Dummy_s { int dummy; }

/*--------------------------------------------------------------------*//*!
 * \brief Implement a template pool hash class.
 * \param TYPENAME	Type name of the declared hash.
 * \param KEYTYPE	Type of the key.
 * \param VALUETYPE	Type of the value.
 * \param HASHFUNC	Function used for hashing the key.
 * \param CMPFUNC	Function used for exact matching of the keys.
 *
 * This macro has implements the hash declared with DE_DECLARE_POOL_HASH.
 * Usually this macro should be used from a .c file, since the macro expands
 * into multiple functions. The TYPENAME, KEYTYPE, and VALUETYPE parameters
 * must match those of the declare macro.
*//*--------------------------------------------------------------------*/
#define DE_IMPLEMENT_POOL_HASH(TYPENAME, KEYTYPE, VALUETYPE, HASHFUNC, CMPFUNC)		\
\
TYPENAME* TYPENAME##_create (deMemPool* pool)    \
{   \
	/* Alloc struct. */ \
	DE_PTR_TYPE(TYPENAME) hash = DE_POOL_NEW(pool, TYPENAME); \
	if (!hash) \
		return DE_NULL; \
\
	memset(hash, 0, sizeof(TYPENAME)); \
	hash->pool = pool; \
\
	return hash; \
} \
\
void TYPENAME##_reset (DE_PTR_TYPE(TYPENAME) hash)    \
{   \
	int slotNdx; \
	for (slotNdx = 0; slotNdx < hash->slotTableSize; slotNdx++)	\
	{	\
		TYPENAME##Slot* slot = hash->slotTable[slotNdx]; \
		while (slot) \
		{ \
			TYPENAME##Slot*	nextSlot = slot->nextSlot;	\
			slot->nextSlot = hash->slotFreeList;		\
			hash->slotFreeList = slot;	\
			slot->numUsed = 0;			\
			slot = nextSlot;			\
		}	\
		hash->slotTable[slotNdx] = DE_NULL; \
	}	\
	hash->numElements = 0; \
}	\
\
TYPENAME##Slot* TYPENAME##_allocSlot (DE_PTR_TYPE(TYPENAME) hash)    \
{   \
	TYPENAME##Slot* slot; \
	if (hash->slotFreeList) \
	{ \
		slot = hash->slotFreeList; \
		hash->slotFreeList = hash->slotFreeList->nextSlot; \
	} \
	else \
		slot = (TYPENAME##Slot*)deMemPool_alloc(hash->pool, sizeof(TYPENAME##Slot) * DE_HASH_ELEMENTS_PER_SLOT); \
\
	if (slot) \
	{ \
		slot->nextSlot = DE_NULL; \
		slot->numUsed = 0; \
	} \
\
	return slot; \
} \
\
deBool TYPENAME##_rehash (DE_PTR_TYPE(TYPENAME) hash, int newSlotTableSize)    \
{    \
	DE_ASSERT(deIsPowerOfTwo32(newSlotTableSize) && newSlotTableSize > 0); \
	if (newSlotTableSize > hash->slotTableSize)    \
	{ \
		TYPENAME##Slot**	oldSlotTable = hash->slotTable; \
		TYPENAME##Slot**	newSlotTable = (TYPENAME##Slot**)deMemPool_alloc(hash->pool, sizeof(TYPENAME##Slot*) * (size_t)newSlotTableSize); \
		int					oldSlotTableSize = hash->slotTableSize; \
		int					slotNdx; \
\
		if (!newSlotTable) \
			return DE_FALSE; \
\
		for (slotNdx = 0; slotNdx < oldSlotTableSize; slotNdx++) \
			newSlotTable[slotNdx] = oldSlotTable[slotNdx]; \
\
		for (slotNdx = oldSlotTableSize; slotNdx < newSlotTableSize; slotNdx++) \
			newSlotTable[slotNdx] = DE_NULL; \
\
		hash->slotTableSize		= newSlotTableSize; \
		hash->slotTable			= newSlotTable; \
\
		for (slotNdx = 0; slotNdx < oldSlotTableSize; slotNdx++) \
		{ \
			TYPENAME##Slot* slot = oldSlotTable[slotNdx]; \
			newSlotTable[slotNdx] = DE_NULL; \
			while (slot) \
			{ \
				int elemNdx; \
				for (elemNdx = 0; elemNdx < slot->numUsed; elemNdx++) \
				{ \
					hash->numElements--; \
					if (!TYPENAME##_insert(hash, slot->keys[elemNdx], slot->values[elemNdx])) \
						return DE_FALSE; \
				} \
				slot = slot->nextSlot; \
			} \
		} \
	} \
\
	return DE_TRUE;    \
}    \
\
VALUETYPE* TYPENAME##_find (const TYPENAME* hash, KEYTYPE key)    \
{    \
	if (hash->numElements > 0) \
	{	\
		int				slotNdx	= (int)(HASHFUNC(key) & (deUint32)(hash->slotTableSize - 1)); \
		TYPENAME##Slot*	slot	= hash->slotTable[slotNdx]; \
		DE_ASSERT(deInBounds32(slotNdx, 0, hash->slotTableSize)); \
	\
		while (slot) \
		{ \
			int elemNdx; \
			for (elemNdx = 0; elemNdx < slot->numUsed; elemNdx++) \
			{ \
				if (CMPFUNC(slot->keys[elemNdx], key)) \
					return &slot->values[elemNdx]; \
			} \
			slot = slot->nextSlot; \
		} \
	} \
\
	return DE_NULL; \
}    \
\
deBool TYPENAME##_insert (DE_PTR_TYPE(TYPENAME) hash, KEYTYPE key, VALUETYPE value)    \
{    \
	int				slotNdx; \
	TYPENAME##Slot*	slot; \
\
	DE_ASSERT(!TYPENAME##_find(hash, key));	\
\
	if ((hash->numElements + 1) >= hash->slotTableSize * DE_HASH_ELEMENTS_PER_SLOT) \
		if (!TYPENAME##_rehash(hash, deMax32(4, 2*hash->slotTableSize))) \
			return DE_FALSE; \
\
	slotNdx	= (int)(HASHFUNC(key) & (deUint32)(hash->slotTableSize - 1)); \
	DE_ASSERT(slotNdx >= 0 && slotNdx < hash->slotTableSize); \
	slot	= hash->slotTable[slotNdx]; \
\
	if (!slot) \
	{ \
		slot = TYPENAME##_allocSlot(hash); \
		if (!slot) return DE_FALSE; \
		hash->slotTable[slotNdx] = slot; \
	} \
\
	for (;;) \
	{ \
		if (slot->numUsed == DE_HASH_ELEMENTS_PER_SLOT) \
		{ \
			if (slot->nextSlot) \
				slot = slot->nextSlot; \
			else \
			{ \
				TYPENAME##Slot* nextSlot = TYPENAME##_allocSlot(hash); \
				if (!nextSlot) return DE_FALSE; \
				slot->nextSlot = nextSlot; \
				slot = nextSlot; \
			} \
		} \
		else \
		{ \
			slot->keys[slot->numUsed]	= key; \
			slot->values[slot->numUsed]	= value; \
			slot->numUsed++; \
			hash->numElements++; \
			return DE_TRUE; \
		} \
	} \
} \
\
void TYPENAME##_delete (DE_PTR_TYPE(TYPENAME) hash, KEYTYPE key)    \
{    \
	int				slotNdx; \
	TYPENAME##Slot*	slot; \
	TYPENAME##Slot*	prevSlot = DE_NULL; \
\
	DE_ASSERT(hash->numElements > 0); \
	slotNdx	= (int)(HASHFUNC(key) & (deUint32)(hash->slotTableSize - 1)); \
	DE_ASSERT(slotNdx >= 0 && slotNdx < hash->slotTableSize); \
	slot	= hash->slotTable[slotNdx]; \
	DE_ASSERT(slot); \
\
	for (;;) \
	{ \
		int elemNdx; \
		DE_ASSERT(slot->numUsed > 0); \
		for (elemNdx = 0; elemNdx < slot->numUsed; elemNdx++) \
		{ \
			if (CMPFUNC(slot->keys[elemNdx], key)) \
			{ \
				TYPENAME##Slot*	lastSlot = slot; \
				while (lastSlot->nextSlot) \
				{ \
					prevSlot = lastSlot; \
					lastSlot = lastSlot->nextSlot; \
				} \
\
				slot->keys[elemNdx]		= lastSlot->keys[lastSlot->numUsed-1]; \
				slot->values[elemNdx]	= lastSlot->values[lastSlot->numUsed-1]; \
				lastSlot->numUsed--; \
\
				if (lastSlot->numUsed == 0) \
				{ \
					if (prevSlot) \
						prevSlot->nextSlot = DE_NULL; \
					else \
						hash->slotTable[slotNdx] = DE_NULL; \
\
					lastSlot->nextSlot = hash->slotFreeList; \
					hash->slotFreeList = lastSlot; \
				} \
\
				hash->numElements--; \
				return; \
			} \
		} \
\
		prevSlot = slot; \
		slot = slot->nextSlot; \
		DE_ASSERT(slot); \
	} \
}    \
struct TYPENAME##Dummy2_s { int dummy; }

/* Copy-to-array templates. */

#define DE_DECLARE_POOL_HASH_TO_ARRAY(HASHTYPENAME, KEYARRAYTYPENAME, VALUEARRAYTYPENAME)		\
	deBool HASHTYPENAME##_copyToArray(const HASHTYPENAME* set, DE_PTR_TYPE(KEYARRAYTYPENAME) keyArray, DE_PTR_TYPE(VALUEARRAYTYPENAME) valueArray);	\
	struct HASHTYPENAME##_##KEYARRAYTYPENAME##_##VALUEARRAYTYPENAME##_declare_dummy { int dummy; }

#define DE_IMPLEMENT_POOL_HASH_TO_ARRAY(HASHTYPENAME, KEYARRAYTYPENAME, VALUEARRAYTYPENAME)		\
deBool HASHTYPENAME##_copyToArray(const HASHTYPENAME* hash, DE_PTR_TYPE(KEYARRAYTYPENAME) keyArray, DE_PTR_TYPE(VALUEARRAYTYPENAME) valueArray)	\
{	\
	int numElements	= hash->numElements;	\
	int arrayNdx	= 0;	\
	int slotNdx;	\
	\
	if ((keyArray && !KEYARRAYTYPENAME##_setSize(keyArray, numElements)) ||			\
		(valueArray && !VALUEARRAYTYPENAME##_setSize(valueArray, numElements)))		\
		return DE_FALSE;	\
	\
	for (slotNdx = 0; slotNdx < hash->slotTableSize; slotNdx++) \
	{ \
		const HASHTYPENAME##Slot* slot = hash->slotTable[slotNdx]; \
		while (slot) \
		{ \
			int elemNdx; \
			for (elemNdx = 0; elemNdx < slot->numUsed; elemNdx++) \
			{	\
				if (keyArray)	\
					KEYARRAYTYPENAME##_set(keyArray, arrayNdx, slot->keys[elemNdx]); \
				if (valueArray)	\
					VALUEARRAYTYPENAME##_set(valueArray, arrayNdx, slot->values[elemNdx]);	\
				arrayNdx++;	\
			} \
			slot = slot->nextSlot; \
		} \
	}	\
	DE_ASSERT(arrayNdx == numElements);	\
	return DE_TRUE;	\
}	\
struct HASHTYPENAME##_##KEYARRAYTYPENAME##_##VALUEARRAYTYPENAME##_implement_dummy { int dummy; }

#endif /* _DEPOOLHASH_H */
