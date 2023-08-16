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

#ifndef DAWNNATIVE_VULKAN_NATIVESWAPCHAINIMPLVK_H_
#define DAWNNATIVE_VULKAN_NATIVESWAPCHAINIMPLVK_H_

#include "dawn_native/vulkan/VulkanInfo.h"

#include "dawn/dawn_wsi.h"
#include "dawn_native/dawn_platform.h"

namespace dawn_native { namespace vulkan {

    class Device;

    class NativeSwapChainImpl {
      public:
        using WSIContext = DawnWSIContextVulkan;

        NativeSwapChainImpl(Device* device, VkSurfaceKHR surface);
        ~NativeSwapChainImpl();

        void Init(DawnWSIContextVulkan* context);
        DawnSwapChainError Configure(WGPUTextureFormat format,
                                     WGPUTextureUsage,
                                     uint32_t width,
                                     uint32_t height);
        DawnSwapChainError GetNextTexture(DawnSwapChainNextTexture* nextTexture);
        DawnSwapChainError Present();

        wgpu::TextureFormat GetPreferredFormat() const;

        struct ChosenConfig {
            VkFormat nativeFormat;
            wgpu::TextureFormat format;
            VkColorSpaceKHR colorSpace;
            VkSurfaceTransformFlagBitsKHR preTransform;
            uint32_t minImageCount;
            VkPresentModeKHR presentMode;
            VkCompositeAlphaFlagBitsKHR compositeAlpha;
        };

      private:
        void UpdateSurfaceConfig();

        VkSurfaceKHR mSurface = VK_NULL_HANDLE;
        VkSwapchainKHR mSwapChain = VK_NULL_HANDLE;
        std::vector<VkImage> mSwapChainImages;
        uint32_t mLastImageIndex = 0;

        VulkanSurfaceInfo mInfo;

        ChosenConfig mConfig;

        Device* mDevice = nullptr;
    };

}}  // namespace dawn_native::vulkan

#endif  // DAWNNATIVE_VULKAN_NATIVESWAPCHAINIMPLVK_H_
