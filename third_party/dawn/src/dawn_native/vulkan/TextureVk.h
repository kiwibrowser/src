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
#include "dawn_native/vulkan/MemoryAllocator.h"

namespace dawn_native { namespace vulkan {

    VkFormat VulkanImageFormat(dawn::TextureFormat format);
    VkImageUsageFlags VulkanImageUsage(dawn::TextureUsageBit usage, dawn::TextureFormat format);
    VkSampleCountFlagBits VulkanSampleCount(uint32_t sampleCount);

    class Texture : public TextureBase {
      public:
        Texture(Device* device, const TextureDescriptor* descriptor);
        Texture(Device* device, const TextureDescriptor* descriptor, VkImage nativeImage);
        ~Texture();

        VkImage GetHandle() const;
        VkImageAspectFlags GetVkAspectMask() const;

        // Transitions the texture to be used as `usage`, recording any necessary barrier in
        // `commands`.
        // TODO(cwallez@chromium.org): coalesce barriers and do them early when possible.
        void TransitionUsageNow(VkCommandBuffer commands, dawn::TextureUsageBit usage);
        void EnsureSubresourceContentInitialized(VkCommandBuffer commands,
                                                 uint32_t baseMipLevel,
                                                 uint32_t levelCount,
                                                 uint32_t baseArrayLayer,
                                                 uint32_t layerCount);

      private:
        void DestroyImpl() override;
        void ClearTexture(VkCommandBuffer commands,
                          uint32_t baseMipLevel,
                          uint32_t levelCount,
                          uint32_t baseArrayLayer,
                          uint32_t layerCount);

        VkImage mHandle = VK_NULL_HANDLE;
        DeviceMemoryAllocation mMemoryAllocation;

        // A usage of none will make sure the texture is transitioned before its first use as
        // required by the spec.
        dawn::TextureUsageBit mLastUsage = dawn::TextureUsageBit::None;
    };

    class TextureView : public TextureViewBase {
      public:
        TextureView(TextureBase* texture, const TextureViewDescriptor* descriptor);
        ~TextureView();

        VkImageView GetHandle() const;

      private:
        VkImageView mHandle = VK_NULL_HANDLE;
    };

}}  // namespace dawn_native::vulkan

#endif  // DAWNNATIVE_VULKAN_TEXTUREVK_H_
