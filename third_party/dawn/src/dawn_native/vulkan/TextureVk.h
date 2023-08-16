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

#ifndef DAWNNATIVE_VULKAN_TEXTUREVK_H_
#define DAWNNATIVE_VULKAN_TEXTUREVK_H_

#include "dawn_native/Texture.h"

#include "common/vulkan_platform.h"
#include "dawn_native/PassResourceUsage.h"
#include "dawn_native/ResourceMemoryAllocation.h"
#include "dawn_native/vulkan/ExternalHandle.h"
#include "dawn_native/vulkan/external_memory/MemoryService.h"

namespace dawn_native { namespace vulkan {

    struct CommandRecordingContext;
    class Device;

    VkFormat VulkanImageFormat(const Device* device, wgpu::TextureFormat format);
    VkImageUsageFlags VulkanImageUsage(wgpu::TextureUsage usage, const Format& format);
    VkSampleCountFlagBits VulkanSampleCount(uint32_t sampleCount);

    MaybeError ValidateVulkanImageCanBeWrapped(const DeviceBase* device,
                                               const TextureDescriptor* descriptor);

    bool IsSampleCountSupported(const dawn_native::vulkan::Device* device,
                                const VkImageCreateInfo& imageCreateInfo);

    class Texture final : public TextureBase {
      public:
        // Used to create a regular texture from a descriptor.
        static ResultOrError<Ref<TextureBase>> Create(Device* device,
                                                      const TextureDescriptor* descriptor);

        // Creates a texture and initializes it with a VkImage that references an external memory
        // object. Before the texture can be used, the VkDeviceMemory associated with the external
        // image must be bound via Texture::BindExternalMemory.
        static ResultOrError<Texture*> CreateFromExternal(
            Device* device,
            const ExternalImageDescriptor* descriptor,
            const TextureDescriptor* textureDescriptor,
            external_memory::Service* externalMemoryService);

        // Creates a texture that wraps a swapchain-allocated VkImage.
        static Ref<Texture> CreateForSwapChain(Device* device,
                                               const TextureDescriptor* descriptor,
                                               VkImage nativeImage);

        VkImage GetHandle() const;
        VkImageAspectFlags GetVkAspectMask() const;

        // Transitions the texture to be used as `usage`, recording any necessary barrier in
        // `commands`.
        // TODO(cwallez@chromium.org): coalesce barriers and do them early when possible.
        void TransitionFullUsage(CommandRecordingContext* recordingContext,
                                 wgpu::TextureUsage usage);

        void TransitionUsageNow(CommandRecordingContext* recordingContext,
                                wgpu::TextureUsage usage,
                                const SubresourceRange& range);
        void TransitionUsageForPass(CommandRecordingContext* recordingContext,
                                    const PassTextureUsage& textureUsages,
                                    std::vector<VkImageMemoryBarrier>* imageBarriers,
                                    VkPipelineStageFlags* srcStages,
                                    VkPipelineStageFlags* dstStages);

        void EnsureSubresourceContentInitialized(CommandRecordingContext* recordingContext,
                                                 const SubresourceRange& range);

        MaybeError SignalAndDestroy(VkSemaphore* outSignalSemaphore);
        // Binds externally allocated memory to the VkImage and on success, takes ownership of
        // semaphores.
        MaybeError BindExternalMemory(const ExternalImageDescriptor* descriptor,
                                      VkSemaphore signalSemaphore,
                                      VkDeviceMemory externalMemoryAllocation,
                                      std::vector<VkSemaphore> waitSemaphores);

      private:
        ~Texture() override;
        using TextureBase::TextureBase;

        MaybeError InitializeAsInternalTexture();
        MaybeError InitializeFromExternal(const ExternalImageDescriptor* descriptor,
                                          external_memory::Service* externalMemoryService);
        void InitializeForSwapChain(VkImage nativeImage);

        void DestroyImpl() override;
        MaybeError ClearTexture(CommandRecordingContext* recordingContext,
                                const SubresourceRange& range,
                                TextureBase::ClearValue);

        void TweakTransitionForExternalUsage(CommandRecordingContext* recordingContext,
                                             std::vector<VkImageMemoryBarrier>* barriers,
                                             size_t transitionBarrierStart);
        bool CanReuseWithoutBarrier(wgpu::TextureUsage lastUsage, wgpu::TextureUsage usage);

        VkImage mHandle = VK_NULL_HANDLE;
        ResourceMemoryAllocation mMemoryAllocation;
        VkDeviceMemory mExternalAllocation = VK_NULL_HANDLE;

        enum class ExternalState {
            InternalOnly,
            PendingAcquire,
            Acquired,
            PendingRelease,
            Released
        };
        ExternalState mExternalState = ExternalState::InternalOnly;
        ExternalState mLastExternalState = ExternalState::InternalOnly;

        VkSemaphore mSignalSemaphore = VK_NULL_HANDLE;
        std::vector<VkSemaphore> mWaitRequirements;

        bool mSameLastUsagesAcrossSubresources = true;

        // A usage of none will make sure the texture is transitioned before its first use as
        // required by the Vulkan spec.
        std::vector<wgpu::TextureUsage> mSubresourceLastUsages =
            std::vector<wgpu::TextureUsage>(GetSubresourceCount(), wgpu::TextureUsage::None);
    };

    class TextureView final : public TextureViewBase {
      public:
        static ResultOrError<TextureView*> Create(TextureBase* texture,
                                                  const TextureViewDescriptor* descriptor);
        VkImageView GetHandle() const;

      private:
        ~TextureView() override;
        using TextureViewBase::TextureViewBase;
        MaybeError Initialize(const TextureViewDescriptor* descriptor);

        VkImageView mHandle = VK_NULL_HANDLE;
    };

}}  // namespace dawn_native::vulkan

#endif  // DAWNNATIVE_VULKAN_TEXTUREVK_H_
