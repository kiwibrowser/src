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
#include "dawn_native/Commands.h"
#include "dawn_native/Device.h"
#include "dawn_native/vulkan/CommandRecordingContext.h"
#include "dawn_native/vulkan/Forward.h"
#include "dawn_native/vulkan/VulkanFunctions.h"
#include "dawn_native/vulkan/VulkanInfo.h"

#include "dawn_native/vulkan/external_memory/MemoryService.h"
#include "dawn_native/vulkan/external_semaphore/SemaphoreService.h"

#include <memory>
#include <queue>

namespace dawn_native { namespace vulkan {

    class Adapter;
    class BindGroupLayout;
    class BufferUploader;
    class FencedDeleter;
    class RenderPassCache;
    class ResourceMemoryAllocator;

    class Device : public DeviceBase {
      public:
        static ResultOrError<Device*> Create(Adapter* adapter, const DeviceDescriptor* descriptor);
        ~Device() override;

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
        RenderPassCache* GetRenderPassCache() const;

        CommandRecordingContext* GetPendingRecordingContext();
        MaybeError SubmitPendingCommands();

        void EnqueueDeferredDeallocation(BindGroupLayout* bindGroupLayout);

        // Dawn Native API

        TextureBase* CreateTextureWrappingVulkanImage(
            const ExternalImageDescriptor* descriptor,
            ExternalMemoryHandle memoryHandle,
            const std::vector<ExternalSemaphoreHandle>& waitHandles);

        MaybeError SignalAndExportExternalTexture(Texture* texture,
                                                  ExternalSemaphoreHandle* outHandle);

        // Dawn API
        CommandBufferBase* CreateCommandBuffer(CommandEncoder* encoder,
                                               const CommandBufferDescriptor* descriptor) override;

        MaybeError TickImpl() override;

        ResultOrError<std::unique_ptr<StagingBufferBase>> CreateStagingBuffer(size_t size) override;
        MaybeError CopyFromStagingToBuffer(StagingBufferBase* source,
                                           uint64_t sourceOffset,
                                           BufferBase* destination,
                                           uint64_t destinationOffset,
                                           uint64_t size) override;
        MaybeError CopyFromStagingToTexture(StagingBufferBase* source,
                                            const TextureDataLayout& src,
                                            TextureCopy* dst,
                                            const Extent3D copySize);

        ResultOrError<ResourceMemoryAllocation> AllocateMemory(VkMemoryRequirements requirements,
                                                               bool mappable);
        void DeallocateMemory(ResourceMemoryAllocation* allocation);

        int FindBestMemoryTypeIndex(VkMemoryRequirements requirements, bool mappable);

        ResourceMemoryAllocator* GetResourceMemoryAllocatorForTesting() const;

        // Return the fixed subgroup size to use for compute shaders on this device or 0 if none
        // needs to be set.
        uint32_t GetComputeSubgroupSize() const;

      private:
        Device(Adapter* adapter, const DeviceDescriptor* descriptor);

        ResultOrError<BindGroupBase*> CreateBindGroupImpl(
            const BindGroupDescriptor* descriptor) override;
        ResultOrError<BindGroupLayoutBase*> CreateBindGroupLayoutImpl(
            const BindGroupLayoutDescriptor* descriptor) override;
        ResultOrError<Ref<BufferBase>> CreateBufferImpl(
            const BufferDescriptor* descriptor) override;
        ResultOrError<ComputePipelineBase*> CreateComputePipelineImpl(
            const ComputePipelineDescriptor* descriptor) override;
        ResultOrError<PipelineLayoutBase*> CreatePipelineLayoutImpl(
            const PipelineLayoutDescriptor* descriptor) override;
        ResultOrError<QuerySetBase*> CreateQuerySetImpl(
            const QuerySetDescriptor* descriptor) override;
        ResultOrError<RenderPipelineBase*> CreateRenderPipelineImpl(
            const RenderPipelineDescriptor* descriptor) override;
        ResultOrError<SamplerBase*> CreateSamplerImpl(const SamplerDescriptor* descriptor) override;
        ResultOrError<ShaderModuleBase*> CreateShaderModuleImpl(
            const ShaderModuleDescriptor* descriptor) override;
        ResultOrError<SwapChainBase*> CreateSwapChainImpl(
            const SwapChainDescriptor* descriptor) override;
        ResultOrError<NewSwapChainBase*> CreateSwapChainImpl(
            Surface* surface,
            NewSwapChainBase* previousSwapChain,
            const SwapChainDescriptor* descriptor) override;
        ResultOrError<Ref<TextureBase>> CreateTextureImpl(
            const TextureDescriptor* descriptor) override;
        ResultOrError<TextureViewBase*> CreateTextureViewImpl(
            TextureBase* texture,
            const TextureViewDescriptor* descriptor) override;

        ResultOrError<VulkanDeviceKnobs> CreateDevice(VkPhysicalDevice physicalDevice);
        void GatherQueueFromDevice();

        uint32_t FindComputeSubgroupSize() const;
        void InitTogglesFromDriver();
        void ApplyDepth24PlusS8Toggle();

        void ShutDownImpl() override;
        MaybeError WaitForIdleForDestruction() override;

        // To make it easier to use fn it is a public const member. However
        // the Device is allowed to mutate them through these private methods.
        VulkanFunctions* GetMutableFunctions();

        VulkanDeviceInfo mDeviceInfo = {};
        VkDevice mVkDevice = VK_NULL_HANDLE;
        uint32_t mQueueFamily = 0;
        VkQueue mQueue = VK_NULL_HANDLE;
        uint32_t mComputeSubgroupSize = 0;

        SerialQueue<Ref<BindGroupLayout>> mBindGroupLayoutsPendingDeallocation;
        std::unique_ptr<FencedDeleter> mDeleter;
        std::unique_ptr<ResourceMemoryAllocator> mResourceMemoryAllocator;
        std::unique_ptr<RenderPassCache> mRenderPassCache;

        std::unique_ptr<external_memory::Service> mExternalMemoryService;
        std::unique_ptr<external_semaphore::Service> mExternalSemaphoreService;

        ResultOrError<VkFence> GetUnusedFence();
        Serial CheckAndUpdateCompletedSerials() override;

        // We track which operations are in flight on the GPU with an increasing serial.
        // This works only because we have a single queue. Each submit to a queue is associated
        // to a serial and a fence, such that when the fence is "ready" we know the operations
        // have finished.
        std::queue<std::pair<VkFence, Serial>> mFencesInFlight;
        // Fences in the unused list aren't reset yet.
        std::vector<VkFence> mUnusedFences;

        MaybeError PrepareRecordingContext();
        void RecycleCompletedCommands();

        struct CommandPoolAndBuffer {
            VkCommandPool pool = VK_NULL_HANDLE;
            VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
        };
        SerialQueue<CommandPoolAndBuffer> mCommandsInFlight;
        // Command pools in the unused list haven't been reset yet.
        std::vector<CommandPoolAndBuffer> mUnusedCommands;
        // There is always a valid recording context stored in mRecordingContext
        CommandRecordingContext mRecordingContext;

        MaybeError ImportExternalImage(const ExternalImageDescriptor* descriptor,
                                       ExternalMemoryHandle memoryHandle,
                                       VkImage image,
                                       const std::vector<ExternalSemaphoreHandle>& waitHandles,
                                       VkSemaphore* outSignalSemaphore,
                                       VkDeviceMemory* outAllocation,
                                       std::vector<VkSemaphore>* outWaitSemaphores);
    };

}}  // namespace dawn_native::vulkan

#endif  // DAWNNATIVE_VULKAN_DEVICEVK_H_