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

#ifndef DAWNNATIVE_VULKAN_BUFFERVK_H_
#define DAWNNATIVE_VULKAN_BUFFERVK_H_

#include "dawn_native/Buffer.h"

#include "common/SerialQueue.h"
#include "common/vulkan_platform.h"
#include "dawn_native/ResourceMemoryAllocation.h"

namespace dawn_native { namespace vulkan {

    struct CommandRecordingContext;
    class Device;

    class Buffer final : public BufferBase {
      public:
        static ResultOrError<Ref<Buffer>> Create(Device* device,
                                                 const BufferDescriptor* descriptor);

        VkBuffer GetHandle() const;

        // Transitions the buffer to be used as `usage`, recording any necessary barrier in
        // `commands`.
        // TODO(cwallez@chromium.org): coalesce barriers and do them early when possible.
        void TransitionUsageNow(CommandRecordingContext* recordingContext, wgpu::BufferUsage usage);
        void TransitionUsageNow(CommandRecordingContext* recordingContext,
                                wgpu::BufferUsage usage,
                                std::vector<VkBufferMemoryBarrier>* bufferBarriers,
                                VkPipelineStageFlags* srcStages,
                                VkPipelineStageFlags* dstStages);

        void EnsureDataInitialized(CommandRecordingContext* recordingContext);
        void EnsureDataInitializedAsDestination(CommandRecordingContext* recordingContext,
                                                uint64_t offset,
                                                uint64_t size);
        void EnsureDataInitializedAsDestination(CommandRecordingContext* recordingContext,
                                                const CopyTextureToBufferCmd* copy);

      private:
        ~Buffer() override;
        using BufferBase::BufferBase;
        MaybeError Initialize();
        void InitializeToZero(CommandRecordingContext* recordingContext);
        void ClearBuffer(CommandRecordingContext* recordingContext, uint32_t clearValue);

        // Dawn API
        MaybeError MapReadAsyncImpl() override;
        MaybeError MapWriteAsyncImpl() override;
        MaybeError MapAsyncImpl(wgpu::MapMode mode, size_t offset, size_t size) override;
        void UnmapImpl() override;
        void DestroyImpl() override;

        bool IsMappableAtCreation() const override;
        MaybeError MapAtCreationImpl() override;
        void* GetMappedPointerImpl() override;

        VkBuffer mHandle = VK_NULL_HANDLE;
        ResourceMemoryAllocation mMemoryAllocation;

        wgpu::BufferUsage mLastUsage = wgpu::BufferUsage::None;
    };

}}  // namespace dawn_native::vulkan

#endif  // DAWNNATIVE_VULKAN_BUFFERVK_H_
