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

#ifndef DAWNNATIVE_VULKAN_DEVICEVK_H_
#define DAWNNATIVE_VULKAN_DEVICEVK_H_

#include "dawn_native/dawn_platform.h"

#include "common/Serial.h"
#include "common/SerialQueue.h"
#include "dawn_native/Device.h"
#include "dawn_native/vulkan/Forward.h"
#include "dawn_native/vulkan/VulkanFunctions.h"
#include "dawn_native/vulkan/VulkanInfo.h"

#include <memory>
#include <queue>

namespace dawn_native { namespace vulkan {

    class Adapter;
    class BufferUploader;
    class FencedDeleter;
    class MapRequestTracker;
    class MemoryAllocator;
    class RenderPassCache;

    class Device : public DeviceBase {
      public:
        Device(Adapter* adapter, const DeviceDescriptor* descriptor);
        ~Device();

        MaybeError Initialize();

        // Contains all the Vulkan entry points, vkDoFoo is called via device->fn.DoFoo.
        const VulkanFunctions fn;

        VkInstance GetVkInstance() const;
        const VulkanDeviceInfo& GetDeviceInfo() const;
        VkDevice GetVkDevice() const;
        uint32_t GetGraphicsQueueFamily() const;
        VkQueue GetQueue() const;

        BufferUploader* GetBufferUploader() const;
        FencedDeleter* GetFencedDeleter() const;
        MapRequestTracker* GetMapRequestTracker() const;
        MemoryAllocator* GetMemoryAllocator() const;
        RenderPassCache* GetRenderPassCache() const;

        VkCommandBuffer GetPendingCommandBuffer();
        Serial GetPendingCommandSerial() const override;
        void SubmitPendingCommands();
        void AddWaitSemaphore(VkSemaphore semaphore);

        // Dawn API
        CommandBufferBase* CreateCommandBuffer(CommandEncoderBase* encoder) override;

        Serial GetCompletedCommandSerial() const final override;
        Serial GetLastSubmittedCommandSerial() const final override;
        void TickImpl() override;

        ResultOrError<std::unique_ptr<StagingBufferBase>> CreateStagingBuffer(size_t size) override;
        MaybeError CopyFromStagingToBuffer(StagingBufferBase* source,
                                           uint64_t sourceOffset,
                                           BufferBase* destination,
                                           uint64_t destinationOffset,
                                           uint64_t size) override;

      private:
        ResultOrError<BindGroupBase*> CreateBindGroupImpl(
            const BindGroupDescriptor* descriptor) override;
        ResultOrError<BindGroupLayoutBase*> CreateBindGroupLayoutImpl(
            const BindGroupLayoutDescriptor* descriptor) override;
        ResultOrError<BufferBase*> CreateBufferImpl(const BufferDescriptor* descriptor) override;
        ResultOrError<ComputePipelineBase*> CreateComputePipelineImpl(
            const ComputePipelineDescriptor* descriptor) override;
        ResultOrError<PipelineLayoutBase*> CreatePipelineLayoutImpl(
            const PipelineLayoutDescriptor* descriptor) override;
        ResultOrError<QueueBase*> CreateQueueImpl() override;
        ResultOrError<RenderPipelineBase*> CreateRenderPipelineImpl(
            const RenderPipelineDescriptor* descriptor) override;
        ResultOrError<SamplerBase*> CreateSamplerImpl(const SamplerDescriptor* descriptor) override;
        ResultOrError<ShaderModuleBase*> CreateShaderModuleImpl(
            const ShaderModuleDescriptor* descriptor) override;
        ResultOrError<SwapChainBase*> CreateSwapChainImpl(
            const SwapChainDescriptor* descriptor) override;
        ResultOrError<TextureBase*> CreateTextureImpl(const TextureDescriptor* descriptor) override;
        ResultOrError<TextureViewBase*> CreateTextureViewImpl(
            TextureBase* texture,
            const TextureViewDescriptor* descriptor) override;

        ResultOrError<VulkanDeviceKnobs> CreateDevice(VkPhysicalDevice physicalDevice);
        void GatherQueueFromDevice();

        // To make it easier to use fn it is a public const member. However
        // the Device is allowed to mutate them through these private methods.
        VulkanFunctions* GetMutableFunctions();

        VulkanDeviceInfo mDeviceInfo = {};
        VkDevice mVkDevice = VK_NULL_HANDLE;
        uint32_t mQueueFamily = 0;
        VkQueue mQueue = VK_NULL_HANDLE;

        std::unique_ptr<FencedDeleter> mDeleter;
        std::unique_ptr<MapRequestTracker> mMapRequestTracker;
        std::unique_ptr<MemoryAllocator> mMemoryAllocator;
        std::unique_ptr<RenderPassCache> mRenderPassCache;

        VkFence GetUnusedFence();
        void CheckPassedFences();

        // We track which operations are in flight on the GPU with an increasing serial.
        // This works only because we have a single queue. Each submit to a queue is associated
        // to a serial and a fence, such that when the fence is "ready" we know the operations
        // have finished.
        std::queue<std::pair<VkFence, Serial>> mFencesInFlight;
        std::vector<VkFence> mUnusedFences;
        Serial mCompletedSerial = 0;
        Serial mLastSubmittedSerial = 0;

        struct CommandPoolAndBuffer {
            VkCommandPool pool = VK_NULL_HANDLE;
            VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
        };

        CommandPoolAndBuffer GetUnusedCommands();
        void RecycleCompletedCommands();
        void FreeCommands(CommandPoolAndBuffer* commands);

        SerialQueue<CommandPoolAndBuffer> mCommandsInFlight;
        std::vector<CommandPoolAndBuffer> mUnusedCommands;
        CommandPoolAndBuffer mPendingCommands;
        std::vector<VkSemaphore> mWaitSemaphores;
    };

}}  // namespace dawn_native::vulkan

#endif  // DAWNNATIVE_VULKAN_DEVICEVK_H_
