// Copyright 2020 The Dawn Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "common/Math.h"

#include "dawn_native/d3d12/D3D12Error.h"
#include "dawn_native/d3d12/DeviceD3D12.h"
#include "dawn_native/d3d12/StagingDescriptorAllocatorD3D12.h"

namespace dawn_native { namespace d3d12 {

    StagingDescriptorAllocator::StagingDescriptorAllocator(Device* device,
                                                           uint32_t descriptorCount,
                                                           uint32_t heapSize,
                                                           D3D12_DESCRIPTOR_HEAP_TYPE heapType)
        : mDevice(device),
          mSizeIncrement(device->GetD3D12Device()->GetDescriptorHandleIncrementSize(heapType)),
          mBlockSize(descriptorCount * mSizeIncrement),
          mHeapSize(RoundUp(heapSize, descriptorCount)),
          mHeapType(heapType) {
        ASSERT(descriptorCount <= heapSize);
    }

    StagingDescriptorAllocator::~StagingDescriptorAllocator() {
        const Index freeBlockIndicesSize = GetFreeBlockIndicesSize();
        for (auto& buffer : mPool) {
            ASSERT(buffer.freeBlockIndices.size() == freeBlockIndicesSize);
        }
        ASSERT(mAvailableHeaps.size() == mPool.size());
    }

    ResultOrError<CPUDescriptorHeapAllocation>
    StagingDescriptorAllocator::AllocateCPUDescriptors() {
        if (mAvailableHeaps.empty()) {
            DAWN_TRY(AllocateCPUHeap());
        }

        ASSERT(!mAvailableHeaps.empty());

        const uint32_t heapIndex = mAvailableHeaps.back();
        NonShaderVisibleBuffer& buffer = mPool[heapIndex];

        ASSERT(!buffer.freeBlockIndices.empty());

        const Index blockIndex = buffer.freeBlockIndices.back();

        buffer.freeBlockIndices.pop_back();

        if (buffer.freeBlockIndices.empty()) {
            mAvailableHeaps.pop_back();
        }

        const D3D12_CPU_DESCRIPTOR_HANDLE baseCPUDescriptor = {
            buffer.heap->GetCPUDescriptorHandleForHeapStart().ptr + (blockIndex * mBlockSize)};

        return CPUDescriptorHeapAllocation{baseCPUDescriptor, heapIndex};
    }

    MaybeError StagingDescriptorAllocator::AllocateCPUHeap() {
        D3D12_DESCRIPTOR_HEAP_DESC heapDescriptor;
        heapDescriptor.Type = mHeapType;
        heapDescriptor.NumDescriptors = mHeapSize;
        heapDescriptor.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        heapDescriptor.NodeMask = 0;

        ComPtr<ID3D12DescriptorHeap> heap;
        DAWN_TRY(CheckHRESULT(
            mDevice->GetD3D12Device()->CreateDescriptorHeap(&heapDescriptor, IID_PPV_ARGS(&heap)),
            "ID3D12Device::CreateDescriptorHeap"));

        NonShaderVisibleBuffer newBuffer;
        newBuffer.heap = std::move(heap);

        const Index freeBlockIndicesSize = GetFreeBlockIndicesSize();
        newBuffer.freeBlockIndices.reserve(freeBlockIndicesSize);

        for (Index blockIndex = 0; blockIndex < freeBlockIndicesSize; blockIndex++) {
            newBuffer.freeBlockIndices.push_back(blockIndex);
        }

        mAvailableHeaps.push_back(mPool.size());
        mPool.emplace_back(std::move(newBuffer));

        return {};
    }

    void StagingDescriptorAllocator::Deallocate(CPUDescriptorHeapAllocation* allocation) {
        ASSERT(allocation->IsValid());

        const uint32_t heapIndex = allocation->GetHeapIndex();

        ASSERT(heapIndex < mPool.size());

        // Insert the deallocated block back into the free-list. Order does not matter. However,
        // having blocks be non-contigious could slow down future allocations due to poor cache
        // locality.
        // TODO(dawn:155): Consider more optimization.
        std::vector<Index>& freeBlockIndices = mPool[heapIndex].freeBlockIndices;
        if (freeBlockIndices.empty()) {
            mAvailableHeaps.emplace_back(heapIndex);
        }

        const D3D12_CPU_DESCRIPTOR_HANDLE heapStart =
            mPool[heapIndex].heap->GetCPUDescriptorHandleForHeapStart();

        const D3D12_CPU_DESCRIPTOR_HANDLE baseDescriptor = allocation->OffsetFrom(0, 0);

        const Index blockIndex = (baseDescriptor.ptr - heapStart.ptr) / mBlockSize;

        freeBlockIndices.emplace_back(blockIndex);

        // Invalidate the handle in case the developer accidentally uses it again.
        allocation->Invalidate();
    }

    uint32_t StagingDescriptorAllocator::GetSizeIncrement() const {
        return mSizeIncrement;
    }

    StagingDescriptorAllocator::Index StagingDescriptorAllocator::GetFreeBlockIndicesSize() const {
        return ((mHeapSize * mSizeIncrement) / mBlockSize);
    }

    ResultOrError<CPUDescriptorHeapAllocation>
    StagingDescriptorAllocator::AllocateTransientCPUDescriptors() {
        CPUDescriptorHeapAllocation allocation;
        DAWN_TRY_ASSIGN(allocation, AllocateCPUDescriptors());
        mAllocationsToDelete.Enqueue(allocation, mDevice->GetPendingCommandSerial());
        return allocation;
    }

    void StagingDescriptorAllocator::Tick(Serial completedSerial) {
        for (CPUDescriptorHeapAllocation& allocation :
             mAllocationsToDelete.IterateUpTo(completedSerial)) {
            Deallocate(&allocation);
        }

        mAllocationsToDelete.ClearUpTo(completedSerial);
    }

}}  // namespace dawn_native::d3d12
