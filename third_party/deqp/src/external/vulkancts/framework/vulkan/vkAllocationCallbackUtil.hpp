#ifndef _VKALLOCATIONCALLBACKUTIL_HPP
#define _VKALLOCATIONCALLBACKUTIL_HPP
/*-------------------------------------------------------------------------
 * Vulkan CTS Framework
 * --------------------
 *
 * Copyright (c) 2015 Google Inc.
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
 * \brief Memory allocation callback utilities.
 *//*--------------------------------------------------------------------*/

#include "vkDefs.hpp"
#include "deAppendList.hpp"

#include <vector>
#include <ostream>

namespace tcu
{
class TestLog;
}

namespace vk
{

class AllocationCallbacks
{
public:
									AllocationCallbacks		(void);
	virtual							~AllocationCallbacks	(void);

	virtual void*					allocate				(size_t size, size_t alignment, VkSystemAllocationScope allocationScope) = 0;
	virtual void*					reallocate				(void* original, size_t size, size_t alignment, VkSystemAllocationScope allocationScope) = 0;
	virtual void					free					(void* mem) = 0;

	virtual void					notifyInternalAllocation(size_t size, VkInternalAllocationType allocationType, VkSystemAllocationScope allocationScope) = 0;
	virtual void					notifyInternalFree		(size_t size, VkInternalAllocationType allocationType, VkSystemAllocationScope allocationScope) = 0;

	const VkAllocationCallbacks*	getCallbacks			(void) const { return &m_callbacks;	}

private:
	const VkAllocationCallbacks		m_callbacks;
};

struct AllocationCallbackRecord
{
	enum Type
	{
		TYPE_ALLOCATION		= 0,		//! Call to pfnAllocation
		TYPE_REALLOCATION,				//! Call to pfnReallocation
		TYPE_FREE,						//! Call to pfnFree
		TYPE_INTERNAL_ALLOCATION,		//! Call to pfnInternalAllocation
		TYPE_INTERNAL_FREE,				//! Call to pfnInternalFree

		TYPE_LAST
	};

	Type	type;

	union
	{
		struct
		{
			size_t						size;
			size_t						alignment;
			VkSystemAllocationScope		scope;
			void*						returnedPtr;
		} allocation;

		struct
		{
			void*						original;
			size_t						size;
			size_t						alignment;
			VkSystemAllocationScope		scope;
			void*						returnedPtr;
		} reallocation;

		struct
		{
			void*						mem;
		} free;

		// \note Used for both INTERNAL_ALLOCATION and INTERNAL_FREE
		struct
		{
			size_t						size;
			VkInternalAllocationType	type;
			VkSystemAllocationScope		scope;
		} internalAllocation;
	} data;

									AllocationCallbackRecord	(void) : type(TYPE_LAST) {}

	static AllocationCallbackRecord	allocation					(size_t size, size_t alignment, VkSystemAllocationScope scope, void* returnedPtr);
	static AllocationCallbackRecord	reallocation				(void* original, size_t size, size_t alignment, VkSystemAllocationScope scope, void* returnedPtr);
	static AllocationCallbackRecord	free						(void* mem);
	static AllocationCallbackRecord	internalAllocation			(size_t size, VkInternalAllocationType type, VkSystemAllocationScope scope);
	static AllocationCallbackRecord	internalFree				(size_t size, VkInternalAllocationType type, VkSystemAllocationScope scope);
};

class ChainedAllocator : public AllocationCallbacks
{
public:
									ChainedAllocator		(const VkAllocationCallbacks* nextAllocator);
									~ChainedAllocator		(void);

	void*							allocate				(size_t size, size_t alignment, VkSystemAllocationScope allocationScope);
	void*							reallocate				(void* original, size_t size, size_t alignment, VkSystemAllocationScope allocationScope);
	void							free					(void* mem);

	void							notifyInternalAllocation(size_t size, VkInternalAllocationType allocationType, VkSystemAllocationScope allocationScope);
	void							notifyInternalFree		(size_t size, VkInternalAllocationType allocationType, VkSystemAllocationScope allocationScope);

private:
	const VkAllocationCallbacks*	m_nextAllocator;
};

class AllocationCallbackRecorder : public ChainedAllocator
{
public:
							AllocationCallbackRecorder	(const VkAllocationCallbacks* allocator, deUint32 callCountHint = 1024);
							~AllocationCallbackRecorder	(void);

	void*					allocate					(size_t size, size_t alignment, VkSystemAllocationScope allocationScope);
	void*					reallocate					(void* original, size_t size, size_t alignment, VkSystemAllocationScope allocationScope);
	void					free						(void* mem);

	void					notifyInternalAllocation	(size_t size, VkInternalAllocationType allocationType, VkSystemAllocationScope allocationScope);
	void					notifyInternalFree			(size_t size, VkInternalAllocationType allocationType, VkSystemAllocationScope allocationScope);

	typedef de::AppendList<AllocationCallbackRecord>::const_iterator	RecordIterator;

	RecordIterator			getRecordsBegin				(void) const { return m_records.begin();	}
	RecordIterator			getRecordsEnd				(void) const { return m_records.end();		}
	std::size_t				getNumRecords				(void) const { return m_records.size();		}

private:
	typedef de::AppendList<AllocationCallbackRecord> Records;

	Records					m_records;
};

//! Allocator that starts returning null after N allocs
class DeterministicFailAllocator : public ChainedAllocator
{
public:
	enum Mode
	{
		MODE_DO_NOT_COUNT = 0,	//!< Do not count allocations, all allocs will succeed
		MODE_COUNT_AND_FAIL,	//!< Count allocations, fail when reaching alloc N

		MODE_LAST
	};

							DeterministicFailAllocator	(const VkAllocationCallbacks* allocator, Mode mode, deUint32 numPassingAllocs);
							~DeterministicFailAllocator	(void);

	void					reset						(Mode mode, deUint32 numPassingAllocs);

	void*					allocate					(size_t size, size_t alignment, VkSystemAllocationScope allocationScope);
	void*					reallocate					(void* original, size_t size, size_t alignment, VkSystemAllocationScope allocationScope);

private:
	Mode					m_mode;
	deUint32				m_numPassingAllocs;
	volatile deUint32		m_allocationNdx;
};

struct AllocationCallbackViolation
{
	enum Reason
	{
		REASON_DOUBLE_FREE = 0,
		REASON_FREE_NOT_ALLOCATED_PTR,
		REASON_REALLOC_NOT_ALLOCATED_PTR,
		REASON_REALLOC_FREED_PTR,
		REASON_NEGATIVE_INTERNAL_ALLOCATION_TOTAL,
		REASON_INVALID_ALLOCATION_SCOPE,
		REASON_INVALID_INTERNAL_ALLOCATION_TYPE,
		REASON_INVALID_ALIGNMENT,
		REASON_REALLOC_DIFFERENT_ALIGNMENT,

		REASON_LAST
	};

	AllocationCallbackRecord	record;
	Reason						reason;

	AllocationCallbackViolation (void)
		: reason(REASON_LAST)
	{}

	AllocationCallbackViolation (const AllocationCallbackRecord& record_, Reason reason_)
		: record(record_)
		, reason(reason_)
	{}
};

struct AllocationCallbackValidationResults
{
	std::vector<AllocationCallbackRecord>		liveAllocations;
	size_t										internalAllocationTotal[VK_INTERNAL_ALLOCATION_TYPE_LAST][VK_SYSTEM_ALLOCATION_SCOPE_LAST];
	std::vector<AllocationCallbackViolation>	violations;

												AllocationCallbackValidationResults	(void);

	void										clear								(void);
};

void							validateAllocationCallbacks		(const AllocationCallbackRecorder& recorder, AllocationCallbackValidationResults* results);
bool							checkAndLog						(tcu::TestLog& log, const AllocationCallbackValidationResults& results, deUint32 allowedLiveAllocScopeBits);
bool							validateAndLog					(tcu::TestLog& log, const AllocationCallbackRecorder& recorder, deUint32 allowedLiveAllocScopeBits);

size_t							getLiveSystemAllocationTotal	(const AllocationCallbackValidationResults& validationResults);

std::ostream&					operator<<						(std::ostream& str, const AllocationCallbackRecord& record);
std::ostream&					operator<<						(std::ostream& str, const AllocationCallbackViolation& violation);

const VkAllocationCallbacks*	getSystemAllocator				(void);

} // vk

#endif // _VKALLOCATIONCALLBACKUTIL_HPP
