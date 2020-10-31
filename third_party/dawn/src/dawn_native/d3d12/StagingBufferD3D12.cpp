// Copyright 2018 The Dawn Authors
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

#include "dawn_native/d3d12/StagingBufferD3D12.h"
#include "dawn_native/d3d12/D3D12Error.h"
#include "dawn_native/d3d12/DeviceD3D12.h"
#include "dawn_native/d3d12/HeapD3D12.h"
#include "dawn_native/d3d12/ResidencyManagerD3D12.h"

namespace dawn_native { namespace d3d12 {

    StagingBuffer::StagingBuffer(size_t size, Device* device)
        : StagingBufferBase(size), mDevice(device) {
    }

    MaybeError StagingBuffer::Initialize() {
        D3D12_RESOURCE_DESC resourceDescriptor;
        resourceDescriptor.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        resourceDescriptor.Alignment = 0;
        resourceDescriptor.Width = GetSize();
        resourceDescriptor.Height = 1;
        resourceDescriptor.DepthOrArraySize = 1;
        resourceDescriptor.MipLevels = 1;
        resourceDescriptor.Format = DXGI_FORMAT_UNKNOWN;
        resourceDescriptor.SampleDesc.Count = 1;
        resourceDescriptor.SampleDesc.Quality = 0;
        resourceDescriptor.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        resourceDescriptor.Flags = D3D12_RESOURCE_FLAG_NONE;

        DAWN_TRY_ASSIGN(mUploadHeap,
                        mDevice->AllocateMemory(D3D12_HEAP_TYPE_UPLOAD, resourceDescriptor,
                                                D3D12_RESOURCE_STATE_GENERIC_READ));

        // The mapped buffer can be accessed at any time, so it must be locked to ensure it is never
        // evicted. This buffer should already have been made resident when it was created.
        DAWN_TRY(mDevice->GetResidencyManager()->LockAllocation(
            ToBackend(mUploadHeap.GetResourceHeap())));

        return CheckHRESULT(GetResource()->Map(0, nullptr, &mMappedPointer), "ID3D12Resource::Map");
    }

    StagingBuffer::~StagingBuffer() {
        // Always check if the allocation is valid before Unmap.
        // The resource would not exist had it failed to allocate.
        if (mUploadHeap.GetInfo().mMethod == AllocationMethod::kInvalid) {
            return;
        }

        // The underlying heap was locked in residency upon creation. We must unlock it when this
        // buffer becomes unmapped.
        mDevice->GetResidencyManager()->UnlockAllocation(ToBackend(mUploadHeap.GetResourceHeap()));

        // Invalidate the CPU virtual address & flush cache (if needed).
        GetResource()->Unmap(0, nullptr);
        mMappedPointer = nullptr;

        mDevice->DeallocateMemory(mUploadHeap);
    }

    ID3D12Resource* StagingBuffer::GetResource() const {
        return mUploadHeap.GetD3D12Resource();
    }
}}  // namespace dawn_native::d3d12
