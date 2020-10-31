// Copyright 2017 The Dawn Authors
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

#include "dawn_native/d3d12/BufferD3D12.h"

#include "common/Assert.h"
#include "common/Constants.h"
#include "common/Math.h"
#include "dawn_native/CommandBuffer.h"
#include "dawn_native/DynamicUploader.h"
#include "dawn_native/d3d12/CommandRecordingContext.h"
#include "dawn_native/d3d12/D3D12Error.h"
#include "dawn_native/d3d12/DeviceD3D12.h"
#include "dawn_native/d3d12/HeapD3D12.h"
#include "dawn_native/d3d12/ResidencyManagerD3D12.h"

namespace dawn_native { namespace d3d12 {

    namespace {
        D3D12_RESOURCE_FLAGS D3D12ResourceFlags(wgpu::BufferUsage usage) {
            D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE;

            if (usage & wgpu::BufferUsage::Storage) {
                flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
            }

            return flags;
        }

        D3D12_RESOURCE_STATES D3D12BufferUsage(wgpu::BufferUsage usage) {
            D3D12_RESOURCE_STATES resourceState = D3D12_RESOURCE_STATE_COMMON;

            if (usage & wgpu::BufferUsage::CopySrc) {
                resourceState |= D3D12_RESOURCE_STATE_COPY_SOURCE;
            }
            if (usage & wgpu::BufferUsage::CopyDst) {
                resourceState |= D3D12_RESOURCE_STATE_COPY_DEST;
            }
            if (usage & (wgpu::BufferUsage::Vertex | wgpu::BufferUsage::Uniform)) {
                resourceState |= D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
            }
            if (usage & wgpu::BufferUsage::Index) {
                resourceState |= D3D12_RESOURCE_STATE_INDEX_BUFFER;
            }
            if (usage & wgpu::BufferUsage::Storage) {
                resourceState |= D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
            }
            if (usage & kReadOnlyStorageBuffer) {
                resourceState |= (D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE |
                                  D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
            }
            if (usage & wgpu::BufferUsage::Indirect) {
                resourceState |= D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT;
            }

            return resourceState;
        }

        D3D12_HEAP_TYPE D3D12HeapType(wgpu::BufferUsage allowedUsage) {
            if (allowedUsage & wgpu::BufferUsage::MapRead) {
                return D3D12_HEAP_TYPE_READBACK;
            } else if (allowedUsage & wgpu::BufferUsage::MapWrite) {
                return D3D12_HEAP_TYPE_UPLOAD;
            } else {
                return D3D12_HEAP_TYPE_DEFAULT;
            }
        }
    }  // namespace

    Buffer::Buffer(Device* device, const BufferDescriptor* descriptor)
        : BufferBase(device, descriptor) {
    }

    MaybeError Buffer::Initialize() {
        D3D12_RESOURCE_DESC resourceDescriptor;
        resourceDescriptor.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        resourceDescriptor.Alignment = 0;
        // TODO(cwallez@chromium.org): Have a global "zero" buffer that can do everything instead
        // of creating a new 4-byte buffer?
        resourceDescriptor.Width = std::max(GetSize(), uint64_t(4u));
        resourceDescriptor.Height = 1;
        resourceDescriptor.DepthOrArraySize = 1;
        resourceDescriptor.MipLevels = 1;
        resourceDescriptor.Format = DXGI_FORMAT_UNKNOWN;
        resourceDescriptor.SampleDesc.Count = 1;
        resourceDescriptor.SampleDesc.Quality = 0;
        resourceDescriptor.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        // Add CopyDst for non-mappable buffer initialization in CreateBufferMapped
        // and robust resource initialization.
        resourceDescriptor.Flags = D3D12ResourceFlags(GetUsage() | wgpu::BufferUsage::CopyDst);

        auto heapType = D3D12HeapType(GetUsage());
        auto bufferUsage = D3D12_RESOURCE_STATE_COMMON;

        // D3D12 requires buffers on the READBACK heap to have the D3D12_RESOURCE_STATE_COPY_DEST
        // state
        if (heapType == D3D12_HEAP_TYPE_READBACK) {
            bufferUsage |= D3D12_RESOURCE_STATE_COPY_DEST;
            mFixedResourceState = true;
            mLastUsage = wgpu::BufferUsage::CopyDst;
        }

        // D3D12 requires buffers on the UPLOAD heap to have the D3D12_RESOURCE_STATE_GENERIC_READ
        // state
        if (heapType == D3D12_HEAP_TYPE_UPLOAD) {
            bufferUsage |= D3D12_RESOURCE_STATE_GENERIC_READ;
            mFixedResourceState = true;
            mLastUsage = wgpu::BufferUsage::CopySrc;
        }

        DAWN_TRY_ASSIGN(
            mResourceAllocation,
            ToBackend(GetDevice())->AllocateMemory(heapType, resourceDescriptor, bufferUsage));

        if (GetDevice()->IsToggleEnabled(Toggle::NonzeroClearResourcesOnCreationForTesting)) {
            CommandRecordingContext* commandRecordingContext;
            DAWN_TRY_ASSIGN(commandRecordingContext,
                            ToBackend(GetDevice())->GetPendingCommandContext());

            DAWN_TRY(ClearBuffer(commandRecordingContext, uint8_t(1u)));
        }

        return {};
    }

    Buffer::~Buffer() {
        DestroyInternal();
    }

    ID3D12Resource* Buffer::GetD3D12Resource() const {
        return mResourceAllocation.GetD3D12Resource();
    }

    // When true is returned, a D3D12_RESOURCE_BARRIER has been created and must be used in a
    // ResourceBarrier call. Failing to do so will cause the tracked state to become invalid and can
    // cause subsequent errors.
    bool Buffer::TrackUsageAndGetResourceBarrier(CommandRecordingContext* commandContext,
                                                 D3D12_RESOURCE_BARRIER* barrier,
                                                 wgpu::BufferUsage newUsage) {
        // Track the underlying heap to ensure residency.
        Heap* heap = ToBackend(mResourceAllocation.GetResourceHeap());
        commandContext->TrackHeapUsage(heap, GetDevice()->GetPendingCommandSerial());

        // Return the resource barrier.
        return TransitionUsageAndGetResourceBarrier(commandContext, barrier, newUsage);
    }

    void Buffer::TrackUsageAndTransitionNow(CommandRecordingContext* commandContext,
                                            wgpu::BufferUsage newUsage) {
        D3D12_RESOURCE_BARRIER barrier;

        if (TrackUsageAndGetResourceBarrier(commandContext, &barrier, newUsage)) {
            commandContext->GetCommandList()->ResourceBarrier(1, &barrier);
        }
    }

    // When true is returned, a D3D12_RESOURCE_BARRIER has been created and must be used in a
    // ResourceBarrier call. Failing to do so will cause the tracked state to become invalid and can
    // cause subsequent errors.
    bool Buffer::TransitionUsageAndGetResourceBarrier(CommandRecordingContext* commandContext,
                                                      D3D12_RESOURCE_BARRIER* barrier,
                                                      wgpu::BufferUsage newUsage) {
        // Resources in upload and readback heaps must be kept in the COPY_SOURCE/DEST state
        if (mFixedResourceState) {
            ASSERT(mLastUsage == newUsage);
            return false;
        }

        D3D12_RESOURCE_STATES lastState = D3D12BufferUsage(mLastUsage);
        D3D12_RESOURCE_STATES newState = D3D12BufferUsage(newUsage);

        // If the transition is from-UAV-to-UAV, then a UAV barrier is needed.
        // If one of the usages isn't UAV, then other barriers are used.
        bool needsUAVBarrier = lastState == D3D12_RESOURCE_STATE_UNORDERED_ACCESS &&
                               newState == D3D12_RESOURCE_STATE_UNORDERED_ACCESS;

        if (needsUAVBarrier) {
            barrier->Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
            barrier->Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
            barrier->UAV.pResource = GetD3D12Resource();

            mLastUsage = newUsage;
            return true;
        }

        // We can skip transitions to already current usages.
        if ((mLastUsage & newUsage) == newUsage) {
            return false;
        }

        mLastUsage = newUsage;

        // The COMMON state represents a state where no write operations can be pending, which makes
        // it possible to transition to and from some states without synchronizaton (i.e. without an
        // explicit ResourceBarrier call). A buffer can be implicitly promoted to 1) a single write
        // state, or 2) multiple read states. A buffer that is accessed within a command list will
        // always implicitly decay to the COMMON state after the call to ExecuteCommandLists
        // completes - this is because all buffer writes are guaranteed to be completed before the
        // next ExecuteCommandLists call executes.
        // https://docs.microsoft.com/en-us/windows/desktop/direct3d12/using-resource-barriers-to-synchronize-resource-states-in-direct3d-12#implicit-state-transitions

        // To track implicit decays, we must record the pending serial on which a transition will
        // occur. When that buffer is used again, the previously recorded serial must be compared to
        // the last completed serial to determine if the buffer has implicity decayed to the common
        // state.
        const Serial pendingCommandSerial = ToBackend(GetDevice())->GetPendingCommandSerial();
        if (pendingCommandSerial > mLastUsedSerial) {
            lastState = D3D12_RESOURCE_STATE_COMMON;
            mLastUsedSerial = pendingCommandSerial;
        }

        // All possible buffer states used by Dawn are eligible for implicit promotion from COMMON.
        // These are: COPY_SOURCE, VERTEX_AND_COPY_BUFFER, INDEX_BUFFER, COPY_DEST,
        // UNORDERED_ACCESS, and INDIRECT_ARGUMENT. Note that for implicit promotion, the
        // destination state cannot be 1) more than one write state, or 2) both a read and write
        // state. This goes unchecked here because it should not be allowed through render/compute
        // pass validation.
        if (lastState == D3D12_RESOURCE_STATE_COMMON) {
            return false;
        }

        barrier->Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier->Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrier->Transition.pResource = GetD3D12Resource();
        barrier->Transition.StateBefore = lastState;
        barrier->Transition.StateAfter = newState;
        barrier->Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

        return true;
    }

    D3D12_GPU_VIRTUAL_ADDRESS Buffer::GetVA() const {
        return mResourceAllocation.GetGPUPointer();
    }

    bool Buffer::IsMappableAtCreation() const {
        // TODO(enga): Handle CPU-visible memory on UMA
        return (GetUsage() & (wgpu::BufferUsage::MapRead | wgpu::BufferUsage::MapWrite)) != 0;
    }

    MaybeError Buffer::MapInternal(bool isWrite,
                                   size_t offset,
                                   size_t size,
                                   const char* contextInfo) {
        // The mapped buffer can be accessed at any time, so it must be locked to ensure it is never
        // evicted. This buffer should already have been made resident when it was created.
        Heap* heap = ToBackend(mResourceAllocation.GetResourceHeap());
        DAWN_TRY(ToBackend(GetDevice())->GetResidencyManager()->LockAllocation(heap));

        D3D12_RANGE range = {offset, offset + size};
        // mMappedData is the pointer to the start of the resource, irrespective of offset.
        // MSDN says (note the weird use of "never"):
        //
        //   When ppData is not NULL, the pointer returned is never offset by any values in
        //   pReadRange.
        //
        // https://docs.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-id3d12resource-map
        DAWN_TRY(CheckHRESULT(GetD3D12Resource()->Map(0, &range, &mMappedData), contextInfo));

        if (isWrite) {
            mWrittenMappedRange = range;
        }

        return {};
    }

    MaybeError Buffer::MapAtCreationImpl() {
        CommandRecordingContext* commandContext;
        DAWN_TRY_ASSIGN(commandContext, ToBackend(GetDevice())->GetPendingCommandContext());
        DAWN_TRY(EnsureDataInitialized(commandContext));

        // Setting isMapWrite to false on MapRead buffers to silence D3D12 debug layer warning.
        bool isMapWrite = (GetUsage() & wgpu::BufferUsage::MapWrite) != 0;
        DAWN_TRY(MapInternal(isMapWrite, 0, size_t(GetSize()), "D3D12 map at creation"));
        return {};
    }

    MaybeError Buffer::MapReadAsyncImpl() {
        return MapInternal(false, 0, size_t(GetSize()), "D3D12 map read async");
    }

    MaybeError Buffer::MapWriteAsyncImpl() {
        return MapInternal(true, 0, size_t(GetSize()), "D3D12 map write async");
    }

    MaybeError Buffer::MapAsyncImpl(wgpu::MapMode mode, size_t offset, size_t size) {
        CommandRecordingContext* commandContext;
        DAWN_TRY_ASSIGN(commandContext, ToBackend(GetDevice())->GetPendingCommandContext());
        DAWN_TRY(EnsureDataInitialized(commandContext));

        return MapInternal(mode & wgpu::MapMode::Write, offset, size, "D3D12 map async");
    }

    void Buffer::UnmapImpl() {
        GetD3D12Resource()->Unmap(0, &mWrittenMappedRange);
        mMappedData = nullptr;
        mWrittenMappedRange = {0, 0};

        // When buffers are mapped, they are locked to keep them in resident memory. We must unlock
        // them when they are unmapped.
        Heap* heap = ToBackend(mResourceAllocation.GetResourceHeap());
        ToBackend(GetDevice())->GetResidencyManager()->UnlockAllocation(heap);
    }

    void* Buffer::GetMappedPointerImpl() {
        // The frontend asks that the pointer returned is from the start of the resource
        // irrespective of the offset passed in MapAsyncImpl, which is what mMappedData is.
        return mMappedData;
    }

    void Buffer::DestroyImpl() {
        if (mMappedData != nullptr) {
            // If the buffer is currently mapped, unmap without flushing the writes to the GPU
            // since the buffer cannot be used anymore. UnmapImpl checks mWrittenRange to know
            // which parts to flush, so we set it to an empty range to prevent flushes.
            mWrittenMappedRange = {0, 0};
            UnmapImpl();
        }

        ToBackend(GetDevice())->DeallocateMemory(mResourceAllocation);
    }

    bool Buffer::CheckIsResidentForTesting() const {
        Heap* heap = ToBackend(mResourceAllocation.GetResourceHeap());
        return heap->IsInList() || heap->IsResidencyLocked();
    }

    bool Buffer::CheckAllocationMethodForTesting(AllocationMethod allocationMethod) const {
        return mResourceAllocation.GetInfo().mMethod == allocationMethod;
    }

    MaybeError Buffer::EnsureDataInitialized(CommandRecordingContext* commandContext) {
        // TODO(jiawei.shao@intel.com): check Toggle::LazyClearResourceOnFirstUse
        // instead when buffer lazy initialization is completely supported.
        if (IsDataInitialized() ||
            !GetDevice()->IsToggleEnabled(Toggle::LazyClearBufferOnFirstUse)) {
            return {};
        }

        DAWN_TRY(InitializeToZero(commandContext));

        return {};
    }

    MaybeError Buffer::EnsureDataInitializedAsDestination(CommandRecordingContext* commandContext,
                                                          uint64_t offset,
                                                          uint64_t size) {
        // TODO(jiawei.shao@intel.com): check Toggle::LazyClearResourceOnFirstUse
        // instead when buffer lazy initialization is completely supported.
        if (IsDataInitialized() ||
            !GetDevice()->IsToggleEnabled(Toggle::LazyClearBufferOnFirstUse)) {
            return {};
        }

        if (IsFullBufferRange(offset, size)) {
            SetIsDataInitialized();
        } else {
            DAWN_TRY(InitializeToZero(commandContext));
        }

        return {};
    }

    MaybeError Buffer::EnsureDataInitializedAsDestination(CommandRecordingContext* commandContext,
                                                          const CopyTextureToBufferCmd* copy) {
        // TODO(jiawei.shao@intel.com): check Toggle::LazyClearResourceOnFirstUse
        // instead when buffer lazy initialization is completely supported.
        if (IsDataInitialized() ||
            !GetDevice()->IsToggleEnabled(Toggle::LazyClearBufferOnFirstUse)) {
            return {};
        }

        if (IsFullBufferOverwrittenInTextureToBufferCopy(copy)) {
            SetIsDataInitialized();
        } else {
            DAWN_TRY(InitializeToZero(commandContext));
        }

        return {};
    }

    MaybeError Buffer::InitializeToZero(CommandRecordingContext* commandContext) {
        ASSERT(GetDevice()->IsToggleEnabled(Toggle::LazyClearBufferOnFirstUse));
        ASSERT(!IsDataInitialized());

        // TODO(jiawei.shao@intel.com): skip initializing the buffer when it is created on a heap
        // that has already been zero initialized.
        DAWN_TRY(ClearBuffer(commandContext, uint8_t(0u)));
        SetIsDataInitialized();
        GetDevice()->IncrementLazyClearCountForTesting();

        return {};
    }

    MaybeError Buffer::ClearBuffer(CommandRecordingContext* commandContext, uint8_t clearValue) {
        Device* device = ToBackend(GetDevice());

        // The state of the buffers on UPLOAD heap must always be GENERIC_READ and cannot be
        // changed away, so we can only clear such buffer with buffer mapping.
        if (D3D12HeapType(GetUsage()) == D3D12_HEAP_TYPE_UPLOAD) {
            DAWN_TRY(MapInternal(true, 0, size_t(GetSize()), "D3D12 map at clear buffer"));
            memset(mMappedData, clearValue, GetSize());
            UnmapImpl();
        } else {
            // TODO(jiawei.shao@intel.com): use ClearUnorderedAccessView*() when the buffer usage
            // includes STORAGE.
            DynamicUploader* uploader = device->GetDynamicUploader();
            UploadHandle uploadHandle;
            DAWN_TRY_ASSIGN(uploadHandle,
                            uploader->Allocate(GetSize(), device->GetPendingCommandSerial()));

            memset(uploadHandle.mappedBuffer, clearValue, GetSize());

            device->CopyFromStagingToBufferImpl(commandContext, uploadHandle.stagingBuffer,
                                                uploadHandle.startOffset, this, 0, GetSize());
        }

        return {};
    }
}}  // namespace dawn_native::d3d12
