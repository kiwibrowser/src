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

#ifndef DAWNNATIVE_D3D12_BUFFERD3D12_H_
#define DAWNNATIVE_D3D12_BUFFERD3D12_H_

#include "common/SerialQueue.h"
#include "dawn_native/Buffer.h"

#include "dawn_native/d3d12/ResourceHeapAllocationD3D12.h"
#include "dawn_native/d3d12/d3d12_platform.h"

namespace dawn_native { namespace d3d12 {

    class CommandRecordingContext;
    class Device;

    class Buffer final : public BufferBase {
      public:
        Buffer(Device* device, const BufferDescriptor* descriptor);

        MaybeError Initialize();

        ID3D12Resource* GetD3D12Resource() const;
        D3D12_GPU_VIRTUAL_ADDRESS GetVA() const;

        bool TrackUsageAndGetResourceBarrier(CommandRecordingContext* commandContext,
                                             D3D12_RESOURCE_BARRIER* barrier,
                                             wgpu::BufferUsage newUsage);
        void TrackUsageAndTransitionNow(CommandRecordingContext* commandContext,
                                        wgpu::BufferUsage newUsage);

        bool CheckAllocationMethodForTesting(AllocationMethod allocationMethod) const;
        bool CheckIsResidentForTesting() const;

        MaybeError EnsureDataInitialized(CommandRecordingContext* commandContext);
        MaybeError EnsureDataInitializedAsDestination(CommandRecordingContext* commandContext,
                                                      uint64_t offset,
                                                      uint64_t size);
        MaybeError EnsureDataInitializedAsDestination(CommandRecordingContext* commandContext,
                                                      const CopyTextureToBufferCmd* copy);

      private:
        ~Buffer() override;
        // Dawn API
        MaybeError MapReadAsyncImpl() override;
        MaybeError MapWriteAsyncImpl() override;
        MaybeError MapAsyncImpl(wgpu::MapMode mode, size_t offset, size_t size) override;
        void UnmapImpl() override;
        void DestroyImpl() override;

        bool IsMappableAtCreation() const override;
        virtual MaybeError MapAtCreationImpl() override;
        void* GetMappedPointerImpl() override;
        MaybeError MapInternal(bool isWrite, size_t start, size_t end, const char* contextInfo);

        bool TransitionUsageAndGetResourceBarrier(CommandRecordingContext* commandContext,
                                                  D3D12_RESOURCE_BARRIER* barrier,
                                                  wgpu::BufferUsage newUsage);

        MaybeError InitializeToZero(CommandRecordingContext* commandContext);
        MaybeError ClearBuffer(CommandRecordingContext* commandContext, uint8_t clearValue);

        ResourceHeapAllocation mResourceAllocation;
        bool mFixedResourceState = false;
        wgpu::BufferUsage mLastUsage = wgpu::BufferUsage::None;
        Serial mLastUsedSerial = UINT64_MAX;

        D3D12_RANGE mWrittenMappedRange = {0, 0};
        void* mMappedData = nullptr;
    };

}}  // namespace dawn_native::d3d12

#endif  // DAWNNATIVE_D3D12_BUFFERD3D12_H_
