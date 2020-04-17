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

#ifndef DAWNNATIVE_VULKAN_FENCEDDELETER_H_
#define DAWNNATIVE_VULKAN_FENCEDDELETER_H_

#include "common/SerialQueue.h"
#include "common/vulkan_platform.h"

namespace dawn_native { namespace vulkan {

    class Device;

    class FencedDeleter {
      public:
        FencedDeleter(Device* device);
        ~FencedDeleter();

        void DeleteWhenUnused(VkBuffer buffer);
        void DeleteWhenUnused(VkDescriptorPool pool);
        void DeleteWhenUnused(VkDeviceMemory memory);
        void DeleteWhenUnused(VkFramebuffer framebuffer);
        void DeleteWhenUnused(VkImage image);
        void DeleteWhenUnused(VkImageView view);
        void DeleteWhenUnused(VkPipelineLayout layout);
        void DeleteWhenUnused(VkRenderPass renderPass);
        void DeleteWhenUnused(VkPipeline pipeline);
        void DeleteWhenUnused(VkSampler sampler);
        void DeleteWhenUnused(VkSemaphore semaphore);
        void DeleteWhenUnused(VkShaderModule module);
        void DeleteWhenUnused(VkSurfaceKHR surface);
        void DeleteWhenUnused(VkSwapchainKHR swapChain);

        void Tick(Serial completedSerial);

      private:
        Device* mDevice = nullptr;
        SerialQueue<VkBuffer> mBuffersToDelete;
        SerialQueue<VkDescriptorPool> mDescriptorPoolsToDelete;
        SerialQueue<VkDeviceMemory> mMemoriesToDelete;
        SerialQueue<VkFramebuffer> mFramebuffersToDelete;
        SerialQueue<VkImage> mImagesToDelete;
        SerialQueue<VkImageView> mImageViewsToDelete;
        SerialQueue<VkPipeline> mPipelinesToDelete;
        SerialQueue<VkPipelineLayout> mPipelineLayoutsToDelete;
        SerialQueue<VkRenderPass> mRenderPassesToDelete;
        SerialQueue<VkSampler> mSamplersToDelete;
        SerialQueue<VkSemaphore> mSemaphoresToDelete;
        SerialQueue<VkShaderModule> mShaderModulesToDelete;
        SerialQueue<VkSurfaceKHR> mSurfacesToDelete;
        SerialQueue<VkSwapchainKHR> mSwapChainsToDelete;
    };

}}  // namespace dawn_native::vulkan

#endif  // DAWNNATIVE_VULKAN_FENCEDDELETER_H_
