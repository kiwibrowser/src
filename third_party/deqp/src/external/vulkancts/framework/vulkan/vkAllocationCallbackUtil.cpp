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

#include "vkAllocationCallbackUtil.hpp"
#include "tcuFormatUtil.hpp"
#include "tcuTestLog.hpp"
#include "deSTLUtil.hpp"
#include "deMemory.h"

#include <map>

namespace vk
{

// System default allocator

static VKAPI_ATTR void* VKAPI_CALL systemAllocate (void*, size_t size, size_t alignment, VkSystemAllocationScope)
{
	if (size > 0)
		return deAlignedMalloc(size, (deUint32)alignment);
	else
		return DE_NULL;
}

static VKAPI_ATTR void VKAPI_CALL systemFree (void*, void* pMem)
{
	deAlignedFree(pMem);
}

static VKAPI_ATTR void* VKAPI_CALL systemReallocate (void*, void* pOriginal, size_t size, size_t alignment, VkSystemAllocationScope)
{
	return deAlignedRealloc(pOriginal, size, alignment);
}

static VKAPI_ATTR void VKAPI_CALL systemInternalAllocationNotification (void*, size_t, VkInternalAllocationType, VkSystemAllocationScope)
{
}

static VKAPI_ATTR void VKAPI_CALL systemInternalFreeNotification (void*, size_t, VkInternalAllocationType, VkSystemAllocationScope)
{
}

static const VkAllocationCallbacks s_systemAllocator =
{
	DE_NULL,		// pUserData
	systemAllocate,
	systemReallocate,
	systemFree,
	systemInternalAllocationNotification,
	systemInternalFreeNotification,
};

const VkAllocationCallbacks* getSystemAllocator (void)
{
	return &s_systemAllocator;
}

// AllocationCallbacks

static VKAPI_ATTR void* VKAPI_CALL allocationCallback (void* pUserData, size_t size, size_t alignment, VkSystemAllocationScope allocationScope)
{
	return reinterpret_cast<AllocationCallbacks*>(pUserData)->allocate(size, alignment, allocationScope);
}

static VKAPI_ATTR void* VKAPI_CALL reallocationCallback (void* pUserData, void* pOriginal, size_t size, size_t alignment, VkSystemAllocationScope allocationScope)
{
	return reinterpret_cast<AllocationCallbacks*>(pUserData)->reallocate(pOriginal, size, alignment, allocationScope);
}

static VKAPI_ATTR void VKAPI_CALL freeCallback (void* pUserData, void* pMem)
{
	reinterpret_cast<AllocationCallbacks*>(pUserData)->free(pMem);
}

static VKAPI_ATTR void VKAPI_CALL internalAllocationNotificationCallback (void* pUserData, size_t size, VkInternalAllocationType allocationType, VkSystemAllocationScope allocationScope)
{
	reinterpret_cast<AllocationCallbacks*>(pUserData)->notifyInternalAllocation(size, allocationType, allocationScope);
}

static VKAPI_ATTR void VKAPI_CALL internalFreeNotificationCallback (void* pUserData, size_t size, VkInternalAllocationType allocationType, VkSystemAllocationScope allocationScope)
{
	reinterpret_cast<AllocationCallbacks*>(pUserData)->notifyInternalFree(size, allocationType, allocationScope);
}

static VkAllocationCallbacks makeCallbacks (AllocationCallbacks* object)
{
	const VkAllocationCallbacks callbacks =
	{
		reinterpret_cast<void*>(object),
		allocationCallback,
		reallocationCallback,
		freeCallback,
		internalAllocationNotificationCallback,
		internalFreeNotificationCallback
	};
	return callbacks;
}

AllocationCallbacks::AllocationCallbacks (void)
	: m_callbacks(makeCallbacks(this))
{
}

AllocationCallbacks::~AllocationCallbacks (void)
{
}

// AllocationCallbackRecord

AllocationCallbackRecord AllocationCallbackRecord::allocation (size_t size, size_t alignment, VkSystemAllocationScope scope, void* returnedPtr)
{
	AllocationCallbackRecord record;

	record.type							= TYPE_ALLOCATION;
	record.data.allocation.size			= size;
	record.data.allocation.alignment	= alignment;
	record.data.allocation.scope		= scope;
	record.data.allocation.returnedPtr	= returnedPtr;

	return record;
}

AllocationCallbackRecord AllocationCallbackRecord::reallocation (void* original, size_t size, size_t alignment, VkSystemAllocationScope scope, void* returnedPtr)
{
	AllocationCallbackRecord record;

	record.type								= TYPE_REALLOCATION;
	record.data.reallocation.original		= original;
	record.data.reallocation.size			= size;
	record.data.reallocation.alignment		= alignment;
	record.data.reallocation.scope			= scope;
	record.data.reallocation.returnedPtr	= returnedPtr;

	return record;
}

AllocationCallbackRecord AllocationCallbackRecord::free (void* mem)
{
	AllocationCallbackRecord record;

	record.type				= TYPE_FREE;
	record.data.free.mem	= mem;

	return record;
}

AllocationCallbackRecord AllocationCallbackRecord::internalAllocation (size_t size, VkInternalAllocationType type, VkSystemAllocationScope scope)
{
	AllocationCallbackRecord record;

	record.type								= TYPE_INTERNAL_ALLOCATION;
	record.data.internalAllocation.size		= size;
	record.data.internalAllocation.type		= type;
	record.data.internalAllocation.scope	= scope;

	return record;
}

AllocationCallbackRecord AllocationCallbackRecord::internalFree (size_t size, VkInternalAllocationType type, VkSystemAllocationScope scope)
{
	AllocationCallbackRecord record;

	record.type								= TYPE_INTERNAL_FREE;
	record.data.internalAllocation.size		= size;
	record.data.internalAllocation.type		= type;
	record.data.internalAllocation.scope	= scope;

	return record;
}

// ChainedAllocator

ChainedAllocator::ChainedAllocator (const VkAllocationCallbacks* nextAllocator)
	: m_nextAllocator(nextAllocator)
{
}

ChainedAllocator::~ChainedAllocator (void)
{
}

void* ChainedAllocator::allocate (size_t size, size_t alignment, VkSystemAllocationScope allocationScope)
{
	return m_nextAllocator->pfnAllocation(m_nextAllocator->pUserData, size, alignment, allocationScope);
}

void* ChainedAllocator::reallocate (void* original, size_t size, size_t alignment, VkSystemAllocationScope allocationScope)
{
	return m_nextAllocator->pfnReallocation(m_nextAllocator->pUserData, original, size, alignment, allocationScope);
}

void ChainedAllocator::free (void* mem)
{
	m_nextAllocator->pfnFree(m_nextAllocator->pUserData, mem);
}

void ChainedAllocator::notifyInternalAllocation (size_t size, VkInternalAllocationType allocationType, VkSystemAllocationScope allocationScope)
{
	m_nextAllocator->pfnInternalAllocation(m_nextAllocator->pUserData, size, allocationType, allocationScope);
}

void ChainedAllocator::notifyInternalFree (size_t size, VkInternalAllocationType allocationType, VkSystemAllocationScope allocationScope)
{
	m_nextAllocator->pfnInternalFree(m_nextAllocator->pUserData, size, allocationType, allocationScope);
}

// AllocationCallbackRecorder

AllocationCallbackRecorder::AllocationCallbackRecorder (const VkAllocationCallbacks* allocator, deUint32 callCountHint)
	: ChainedAllocator	(allocator)
	, m_records			(callCountHint)
{
}

AllocationCallbackRecorder::~AllocationCallbackRecorder (void)
{
}

void* AllocationCallbackRecorder::allocate (size_t size, size_t alignment, VkSystemAllocationScope allocationScope)
{
	void* const	ptr	= ChainedAllocator::allocate(size, alignment, allocationScope);

	m_records.append(AllocationCallbackRecord::allocation(size, alignment, allocationScope, ptr));

	return ptr;
}

void* AllocationCallbackRecorder::reallocate (void* original, size_t size, size_t alignment, VkSystemAllocationScope allocationScope)
{
	void* const	ptr	= ChainedAllocator::reallocate(original, size, alignment, allocationScope);

	m_records.append(AllocationCallbackRecord::reallocation(original, size, alignment, allocationScope, ptr));

	return ptr;
}

void AllocationCallbackRecorder::free (void* mem)
{
	ChainedAllocator::free(mem);

	m_records.append(AllocationCallbackRecord::free(mem));
}

void AllocationCallbackRecorder::notifyInternalAllocation (size_t size, VkInternalAllocationType allocationType, VkSystemAllocationScope allocationScope)
{
	ChainedAllocator::notifyInternalAllocation(size, allocationType, allocationScope);

	m_records.append(AllocationCallbackRecord::internalAllocation(size, allocationType, allocationScope));
}

void AllocationCallbackRecorder::notifyInternalFree (size_t size, VkInternalAllocationType allocationType, VkSystemAllocationScope allocationScope)
{
	ChainedAllocator::notifyInternalFree(size, allocationType, allocationScope);

	m_records.append(AllocationCallbackRecord::internalFree(size, allocationType, allocationScope));
}

// DeterministicFailAllocator

DeterministicFailAllocator::DeterministicFailAllocator (const VkAllocationCallbacks* allocator, Mode mode, deUint32 numPassingAllocs)
	: ChainedAllocator	(allocator)
{
	reset(mode, numPassingAllocs);
}

DeterministicFailAllocator::~DeterministicFailAllocator (void)
{
}

void DeterministicFailAllocator::reset (Mode mode, deUint32 numPassingAllocs)
{
	m_mode				= mode;
	m_numPassingAllocs	= numPassingAllocs;
	m_allocationNdx		= 0;
}

void* DeterministicFailAllocator::allocate (size_t size, size_t alignment, VkSystemAllocationScope allocationScope)
{
	if ((m_mode == MODE_DO_NOT_COUNT) ||
		(deAtomicIncrementUint32(&m_allocationNdx) <= m_numPassingAllocs))
		return ChainedAllocator::allocate(size, alignment, allocationScope);
	else
		return DE_NULL;
}

void* DeterministicFailAllocator::reallocate (void* original, size_t size, size_t alignment, VkSystemAllocationScope allocationScope)
{
	if ((m_mode == MODE_DO_NOT_COUNT) ||
		(deAtomicIncrementUint32(&m_allocationNdx) <= m_numPassingAllocs))
		return ChainedAllocator::reallocate(original, size, alignment, allocationScope);
	else
		return DE_NULL;
}

// Utils

AllocationCallbackValidationResults::AllocationCallbackValidationResults (void)
{
	deMemset(internalAllocationTotal, 0, sizeof(internalAllocationTotal));
}

void AllocationCallbackValidationResults::clear (void)
{
	liveAllocations.clear();
	violations.clear();
	deMemset(internalAllocationTotal, 0, sizeof(internalAllocationTotal));
}

namespace
{

struct AllocationSlot
{
	AllocationCallbackRecord	record;
	bool						isLive;

	AllocationSlot (void)
		: isLive	(false)
	{}

	AllocationSlot (const AllocationCallbackRecord& record_, bool isLive_)
		: record	(record_)
		, isLive	(isLive_)
	{}
};

size_t getAlignment (const AllocationCallbackRecord& record)
{
	if (record.type == AllocationCallbackRecord::TYPE_ALLOCATION)
		return record.data.allocation.alignment;
	else if (record.type == AllocationCallbackRecord::TYPE_REALLOCATION)
		return record.data.reallocation.alignment;
	else
	{
		DE_ASSERT(false);
		return 0;
	}
}

} // anonymous

void validateAllocationCallbacks (const AllocationCallbackRecorder& recorder, AllocationCallbackValidationResults* results)
{
	std::vector<AllocationSlot>		allocations;
	std::map<void*, size_t>			ptrToSlotIndex;

	DE_ASSERT(results->liveAllocations.empty() && results->violations.empty());

	for (AllocationCallbackRecorder::RecordIterator callbackIter = recorder.getRecordsBegin();
		 callbackIter != recorder.getRecordsEnd();
		 ++callbackIter)
	{
		const AllocationCallbackRecord&		record	= *callbackIter;

		// Validate scope
		{
			const VkSystemAllocationScope* const	scopePtr	= record.type == AllocationCallbackRecord::TYPE_ALLOCATION			? &record.data.allocation.scope
																: record.type == AllocationCallbackRecord::TYPE_REALLOCATION		? &record.data.reallocation.scope
																: record.type == AllocationCallbackRecord::TYPE_INTERNAL_ALLOCATION	? &record.data.internalAllocation.scope
																: record.type == AllocationCallbackRecord::TYPE_INTERNAL_FREE		? &record.data.internalAllocation.scope
																: DE_NULL;

			if (scopePtr && !de::inBounds(*scopePtr, (VkSystemAllocationScope)0, VK_SYSTEM_ALLOCATION_SCOPE_LAST))
				results->violations.push_back(AllocationCallbackViolation(record, AllocationCallbackViolation::REASON_INVALID_ALLOCATION_SCOPE));
		}

		// Validate alignment
		if (record.type == AllocationCallbackRecord::TYPE_ALLOCATION ||
			record.type == AllocationCallbackRecord::TYPE_REALLOCATION)
		{
			if (!deIsPowerOfTwoSize(getAlignment(record)))
				results->violations.push_back(AllocationCallbackViolation(record, AllocationCallbackViolation::REASON_INVALID_ALIGNMENT));
		}

		// Validate actual allocation behavior
		switch (record.type)
		{
			case AllocationCallbackRecord::TYPE_ALLOCATION:
			{
				if (record.data.allocation.returnedPtr)
				{
					if (!de::contains(ptrToSlotIndex, record.data.allocation.returnedPtr))
					{
						ptrToSlotIndex[record.data.allocation.returnedPtr] = allocations.size();
						allocations.push_back(AllocationSlot(record, true));
					}
					else
					{
						const size_t		slotNdx		= ptrToSlotIndex[record.data.allocation.returnedPtr];
						if (!allocations[slotNdx].isLive)
						{
							allocations[slotNdx].isLive = true;
							allocations[slotNdx].record = record;
						}
						else
						{
							// we should not have multiple live allocations with the same pointer
							DE_ASSERT(false);
						}
					}
				}

				break;
			}

			case AllocationCallbackRecord::TYPE_REALLOCATION:
			{
				if (de::contains(ptrToSlotIndex, record.data.reallocation.original))
				{
					const size_t		origSlotNdx		= ptrToSlotIndex[record.data.reallocation.original];
					AllocationSlot&		origSlot		= allocations[origSlotNdx];

					DE_ASSERT(record.data.reallocation.original != DE_NULL);

					if (record.data.reallocation.size > 0)
					{
						if (getAlignment(origSlot.record) != record.data.reallocation.alignment)
							results->violations.push_back(AllocationCallbackViolation(record, AllocationCallbackViolation::REASON_REALLOC_DIFFERENT_ALIGNMENT));

						if (record.data.reallocation.original == record.data.reallocation.returnedPtr)
						{
							if (!origSlot.isLive)
							{
								results->violations.push_back(AllocationCallbackViolation(record, AllocationCallbackViolation::REASON_REALLOC_FREED_PTR));
								origSlot.isLive	= true; // Mark live to suppress further errors
							}

							// Just update slot record
							allocations[origSlotNdx].record = record;
						}
						else
						{
							if (record.data.reallocation.returnedPtr)
							{
								allocations[origSlotNdx].isLive = false;
								if (!de::contains(ptrToSlotIndex, record.data.reallocation.returnedPtr))
								{
									ptrToSlotIndex[record.data.reallocation.returnedPtr] = allocations.size();
									allocations.push_back(AllocationSlot(record, true));
								}
								else
								{
									const size_t slotNdx = ptrToSlotIndex[record.data.reallocation.returnedPtr];
									if (!allocations[slotNdx].isLive)
									{
										allocations[slotNdx].isLive = true;
										allocations[slotNdx].record = record;
									}
									else
									{
										// we should not have multiple live allocations with the same pointer
										DE_ASSERT(false);
									}
								}
							}
							// else original ptr remains valid and live
						}
					}
					else
					{
						DE_ASSERT(!record.data.reallocation.returnedPtr);

						origSlot.isLive = false;
					}
				}
				else
				{
					if (record.data.reallocation.original)
						results->violations.push_back(AllocationCallbackViolation(record, AllocationCallbackViolation::REASON_REALLOC_NOT_ALLOCATED_PTR));

					if (record.data.reallocation.returnedPtr)
					{
						if (!de::contains(ptrToSlotIndex, record.data.reallocation.returnedPtr))
						{
							ptrToSlotIndex[record.data.reallocation.returnedPtr] = allocations.size();
							allocations.push_back(AllocationSlot(record, true));
						}
						else
						{
							const size_t slotNdx = ptrToSlotIndex[record.data.reallocation.returnedPtr];
							DE_ASSERT(!allocations[slotNdx].isLive);
							allocations[slotNdx].isLive = true;
							allocations[slotNdx].record = record;
						}
					}
				}

				break;
			}

			case AllocationCallbackRecord::TYPE_FREE:
			{
				if (record.data.free.mem != DE_NULL) // Freeing null pointer is valid and ignored
				{
					if (de::contains(ptrToSlotIndex, record.data.free.mem))
					{
						const size_t	slotNdx		= ptrToSlotIndex[record.data.free.mem];

						if (allocations[slotNdx].isLive)
							allocations[slotNdx].isLive = false;
						else
							results->violations.push_back(AllocationCallbackViolation(record, AllocationCallbackViolation::REASON_DOUBLE_FREE));
					}
					else
						results->violations.push_back(AllocationCallbackViolation(record, AllocationCallbackViolation::REASON_FREE_NOT_ALLOCATED_PTR));
				}

				break;
			}

			case AllocationCallbackRecord::TYPE_INTERNAL_ALLOCATION:
			case AllocationCallbackRecord::TYPE_INTERNAL_FREE:
			{
				if (de::inBounds(record.data.internalAllocation.type, (VkInternalAllocationType)0, VK_INTERNAL_ALLOCATION_TYPE_LAST))
				{
					size_t* const		totalAllocSizePtr	= &results->internalAllocationTotal[record.data.internalAllocation.type][record.data.internalAllocation.scope];
					const size_t		size				= record.data.internalAllocation.size;

					if (record.type == AllocationCallbackRecord::TYPE_INTERNAL_FREE)
					{
						if (*totalAllocSizePtr < size)
						{
							results->violations.push_back(AllocationCallbackViolation(record, AllocationCallbackViolation::REASON_NEGATIVE_INTERNAL_ALLOCATION_TOTAL));
							*totalAllocSizePtr = 0; // Reset to 0 to suppress compound errors
						}
						else
							*totalAllocSizePtr -= size;
					}
					else
						*totalAllocSizePtr += size;
				}
				else
					results->violations.push_back(AllocationCallbackViolation(record, AllocationCallbackViolation::REASON_INVALID_INTERNAL_ALLOCATION_TYPE));

				break;
			}

			default:
				DE_ASSERT(false);
		}
	}

	DE_ASSERT(!de::contains(ptrToSlotIndex, DE_NULL));

	// Collect live allocations
	for (std::vector<AllocationSlot>::const_iterator slotIter = allocations.begin();
		 slotIter != allocations.end();
		 ++slotIter)
	{
		if (slotIter->isLive)
			results->liveAllocations.push_back(slotIter->record);
	}
}

bool checkAndLog (tcu::TestLog& log, const AllocationCallbackValidationResults& results, deUint32 allowedLiveAllocScopeBits)
{
	using tcu::TestLog;

	size_t	numLeaks	= 0;

	if (!results.violations.empty())
	{
		for (size_t violationNdx = 0; violationNdx < results.violations.size(); ++violationNdx)
		{
			log << TestLog::Message << "VIOLATION " << (violationNdx+1)
													<< ": " << results.violations[violationNdx]
													<< " (" << results.violations[violationNdx].record << ")"
				<< TestLog::EndMessage;
		}

		log << TestLog::Message << "ERROR: Found " << results.violations.size() << " invalid allocation callbacks!" << TestLog::EndMessage;
	}

	// Verify live allocations
	for (size_t liveNdx = 0; liveNdx < results.liveAllocations.size(); ++liveNdx)
	{
		const AllocationCallbackRecord&		record	= results.liveAllocations[liveNdx];
		const VkSystemAllocationScope		scope	= record.type == AllocationCallbackRecord::TYPE_ALLOCATION		? record.data.allocation.scope
													: record.type == AllocationCallbackRecord::TYPE_REALLOCATION	? record.data.reallocation.scope
													: VK_SYSTEM_ALLOCATION_SCOPE_LAST;

		DE_ASSERT(de::inBounds(scope, (VkSystemAllocationScope)0, VK_SYSTEM_ALLOCATION_SCOPE_LAST));

		if ((allowedLiveAllocScopeBits & (1u << scope)) == 0)
		{
			log << TestLog::Message << "LEAK " << (numLeaks+1) << ": " << record << TestLog::EndMessage;
			numLeaks += 1;
		}
	}

	// Verify internal allocations
	for (int internalAllocTypeNdx = 0; internalAllocTypeNdx < VK_INTERNAL_ALLOCATION_TYPE_LAST; ++internalAllocTypeNdx)
	{
		for (int scopeNdx = 0; scopeNdx < VK_SYSTEM_ALLOCATION_SCOPE_LAST; ++scopeNdx)
		{
			const VkInternalAllocationType	type			= (VkInternalAllocationType)internalAllocTypeNdx;
			const VkSystemAllocationScope	scope			= (VkSystemAllocationScope)scopeNdx;
			const size_t					totalAllocated	= results.internalAllocationTotal[type][scope];

			if ((allowedLiveAllocScopeBits & (1u << scopeNdx)) == 0 &&
				totalAllocated > 0)
			{
				log << TestLog::Message << "LEAK " << (numLeaks+1) << ": " << totalAllocated
										<< " bytes of (" << type << ", " << scope << ") internal memory is still allocated"
					<< TestLog::EndMessage;
				numLeaks += 1;
			}
		}
	}

	if (numLeaks > 0)
		log << TestLog::Message << "ERROR: Found " << numLeaks << " memory leaks!" << TestLog::EndMessage;

	return results.violations.empty() && numLeaks == 0;
}

bool validateAndLog (tcu::TestLog& log, const AllocationCallbackRecorder& recorder, deUint32 allowedLiveAllocScopeBits)
{
	AllocationCallbackValidationResults	validationResults;

	validateAllocationCallbacks(recorder, &validationResults);

	return checkAndLog(log, validationResults, allowedLiveAllocScopeBits);
}

size_t getLiveSystemAllocationTotal (const AllocationCallbackValidationResults& validationResults)
{
	size_t	allocationTotal	= 0;

	DE_ASSERT(validationResults.violations.empty());

	for (std::vector<AllocationCallbackRecord>::const_iterator alloc = validationResults.liveAllocations.begin();
		 alloc != validationResults.liveAllocations.end();
		 ++alloc)
	{
		DE_ASSERT(alloc->type == AllocationCallbackRecord::TYPE_ALLOCATION ||
				  alloc->type == AllocationCallbackRecord::TYPE_REALLOCATION);

		const size_t	size		= (alloc->type == AllocationCallbackRecord::TYPE_ALLOCATION ? alloc->data.allocation.size : alloc->data.reallocation.size);
		const size_t	alignment	= (alloc->type == AllocationCallbackRecord::TYPE_ALLOCATION ? alloc->data.allocation.alignment : alloc->data.reallocation.alignment);

		allocationTotal += size + alignment - (alignment > 0 ? 1 : 0);
	}

	for (int internalAllocationTypeNdx = 0; internalAllocationTypeNdx < VK_INTERNAL_ALLOCATION_TYPE_LAST; ++internalAllocationTypeNdx)
	{
		for (int internalAllocationScopeNdx = 0; internalAllocationScopeNdx < VK_SYSTEM_ALLOCATION_SCOPE_LAST; ++internalAllocationScopeNdx)
			allocationTotal += validationResults.internalAllocationTotal[internalAllocationTypeNdx][internalAllocationScopeNdx];
	}

	return allocationTotal;
}

std::ostream& operator<< (std::ostream& str, const AllocationCallbackRecord& record)
{
	switch (record.type)
	{
		case AllocationCallbackRecord::TYPE_ALLOCATION:
			str << "ALLOCATION: size=" << record.data.allocation.size
				<< ", alignment=" << record.data.allocation.alignment
				<< ", scope=" << record.data.allocation.scope
				<< ", returnedPtr=" << tcu::toHex(record.data.allocation.returnedPtr);
			break;

		case AllocationCallbackRecord::TYPE_REALLOCATION:
			str << "REALLOCATION: original=" << tcu::toHex(record.data.reallocation.original)
				<< ", size=" << record.data.reallocation.size
				<< ", alignment=" << record.data.reallocation.alignment
				<< ", scope=" << record.data.reallocation.scope
				<< ", returnedPtr=" << tcu::toHex(record.data.reallocation.returnedPtr);
			break;

		case AllocationCallbackRecord::TYPE_FREE:
			str << "FREE: mem=" << tcu::toHex(record.data.free.mem);
			break;

		case AllocationCallbackRecord::TYPE_INTERNAL_ALLOCATION:
		case AllocationCallbackRecord::TYPE_INTERNAL_FREE:
			str << "INTERNAL_" << (record.type == AllocationCallbackRecord::TYPE_INTERNAL_ALLOCATION ? "ALLOCATION" : "FREE")
				<< ": size=" << record.data.internalAllocation.size
				<< ", type=" << record.data.internalAllocation.type
				<< ", scope=" << record.data.internalAllocation.scope;
			break;

		default:
			DE_ASSERT(false);
	}

	return str;
}

std::ostream& operator<< (std::ostream& str, const AllocationCallbackViolation& violation)
{
	switch (violation.reason)
	{
		case AllocationCallbackViolation::REASON_DOUBLE_FREE:
		{
			DE_ASSERT(violation.record.type == AllocationCallbackRecord::TYPE_FREE);
			str << "Double free of " << tcu::toHex(violation.record.data.free.mem);
			break;
		}

		case AllocationCallbackViolation::REASON_FREE_NOT_ALLOCATED_PTR:
		{
			DE_ASSERT(violation.record.type == AllocationCallbackRecord::TYPE_FREE);
			str << "Attempt to free " << tcu::toHex(violation.record.data.free.mem) << " which has not been allocated";
			break;
		}

		case AllocationCallbackViolation::REASON_REALLOC_NOT_ALLOCATED_PTR:
		{
			DE_ASSERT(violation.record.type == AllocationCallbackRecord::TYPE_REALLOCATION);
			str << "Attempt to reallocate " << tcu::toHex(violation.record.data.reallocation.original) << " which has not been allocated";
			break;
		}

		case AllocationCallbackViolation::REASON_REALLOC_FREED_PTR:
		{
			DE_ASSERT(violation.record.type == AllocationCallbackRecord::TYPE_REALLOCATION);
			str << "Attempt to reallocate " << tcu::toHex(violation.record.data.reallocation.original) << " which has been freed";
			break;
		}

		case AllocationCallbackViolation::REASON_NEGATIVE_INTERNAL_ALLOCATION_TOTAL:
		{
			DE_ASSERT(violation.record.type == AllocationCallbackRecord::TYPE_INTERNAL_FREE);
			str << "Internal allocation total for (" << violation.record.data.internalAllocation.type << ", " << violation.record.data.internalAllocation.scope << ") is negative";
			break;
		}

		case AllocationCallbackViolation::REASON_INVALID_INTERNAL_ALLOCATION_TYPE:
		{
			DE_ASSERT(violation.record.type == AllocationCallbackRecord::TYPE_INTERNAL_ALLOCATION ||
					  violation.record.type == AllocationCallbackRecord::TYPE_INTERNAL_FREE);
			str << "Invalid internal allocation type " << tcu::toHex(violation.record.data.internalAllocation.type);
			break;
		}

		case AllocationCallbackViolation::REASON_INVALID_ALLOCATION_SCOPE:
		{
			str << "Invalid allocation scope";
			break;
		}

		case AllocationCallbackViolation::REASON_INVALID_ALIGNMENT:
		{
			str << "Invalid alignment";
			break;
		}

		case AllocationCallbackViolation::REASON_REALLOC_DIFFERENT_ALIGNMENT:
		{
			str << "Reallocation with different alignment";
			break;
		}

		default:
			DE_ASSERT(false);
	}

	return str;
}

} // vk
