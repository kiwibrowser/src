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

#include "dawn_native/vulkan/FencedDeleter.h"

#include "dawn_native/vulkan/DeviceVk.h"

namespace dawn_native { namespace vulkan {

    FencedDeleter::FencedDeleter(Device* device) : mDevice(device) {
    }

    FencedDeleter::~FencedDeleter() {
        ASSERT(mBuffersToDelete.Empty());
        ASSERT(mDescriptorPoolsToDelete.Empty());
        ASSERT(mFramebuffersToDelete.Empty());
        ASSERT(mImagesToDelete.Empty());
        ASSERT(mImageViewsToDelete.Empty());
        ASSERT(mMemoriesToDelete.Empty());
        ASSERT(mPipelinesToDelete.Empty());
        ASSERT(mPipelineLayoutsToDelete.Empty());
        ASSERT(mRenderPassesToDelete.Empty());
        ASSERT(mSamplersToDelete.Empty());
        ASSERT(mSemaphoresToDelete.Empty());
        ASSERT(mShaderModulesToDelete.Empty());
        ASSERT(mSurfacesToDelete.Empty());
        ASSERT(mSwapChainsToDelete.Empty());
    }

    void FencedDeleter::DeleteWhenUnused(VkBuffer buffer) {
        mBuffersToDelete.Enqueue(buffer, mDevice->GetPendingCommandSerial());
    }

    void FencedDeleter::DeleteWhenUnused(VkDescriptorPool pool) {
        mDescriptorPoolsToDelete.Enqueue(pool, mDevice->GetPendingCommandSerial());
    }

    void FencedDeleter::DeleteWhenUnused(VkDeviceMemory memory) {
        mMemoriesToDelete.Enqueue(memory, mDevice->GetPendingCommandSerial());
    }

    void FencedDeleter::DeleteWhenUnused(VkFramebuffer framebuffer) {
        mFramebuffersToDelete.Enqueue(framebuffer, mDevice->GetPendingCommandSerial());
    }

    void FencedDeleter::DeleteWhenUnused(VkImage image) {
        mImagesToDelete.Enqueue(image, mDevice->GetPendingCommandSerial());
    }

    void FencedDeleter::DeleteWhenUnused(VkImageView view) {
        mImageViewsToDelete.Enqueue(view, mDevice->GetPendingCommandSerial());
    }

    void FencedDeleter::DeleteWhenUnused(VkPipeline pipeline) {
        mPipelinesToDelete.Enqueue(pipeline, mDevice->GetPendingCommandSerial());
    }

    void FencedDeleter::DeleteWhenUnused(VkPipelineLayout layout) {
        mPipelineLayoutsToDelete.Enqueue(layout, mDevice->GetPendingCommandSerial());
    }

    void FencedDeleter::DeleteWhenUnused(VkRenderPass renderPass) {
        mRenderPassesToDelete.Enqueue(renderPass, mDevice->GetPendingCommandSerial());
    }

    void FencedDeleter::DeleteWhenUnused(VkSampler sampler) {
        mSamplersToDelete.Enqueue(sampler, mDevice->GetPendingCommandSerial());
    }

    void FencedDeleter::DeleteWhenUnused(VkSemaphore semaphore) {
        mSemaphoresToDelete.Enqueue(semaphore, mDevice->GetPendingCommandSerial());
    }

    void FencedDeleter::DeleteWhenUnused(VkShaderModule module) {
        mShaderModulesToDelete.Enqueue(module, mDevice->GetPendingCommandSerial());
    }

    void FencedDeleter::DeleteWhenUnused(VkSurfaceKHR surface) {
        mSurfacesToDelete.Enqueue(surface, mDevice->GetPendingCommandSerial());
    }

    void FencedDeleter::DeleteWhenUnused(VkSwapchainKHR swapChain) {
        mSwapChainsToDelete.Enqueue(swapChain, mDevice->GetPendingCommandSerial());
    }

    void FencedDeleter::Tick(Serial completedSerial) {
        VkDevice vkDevice = mDevice->GetVkDevice();
        VkInstance instance = mDevice->GetVkInstance();

        // Buffers and images must be deleted before memories because it is invalid to free memory
        // that still have resources bound to it.
        for (VkBuffer buffer : mBuffersToDelete.IterateUpTo(completedSerial)) {
            mDevice->fn.DestroyBuffer(vkDevice, buffer, nullptr);
        }
        mBuffersToDelete.ClearUpTo(completedSerial);
        for (VkImage image : mImagesToDelete.IterateUpTo(completedSerial)) {
            mDevice->fn.DestroyImage(vkDevice, image, nullptr);
        }
        mImagesToDelete.ClearUpTo(completedSerial);

        for (VkDeviceMemory memory : mMemoriesToDelete.IterateUpTo(completedSerial)) {
            mDevice->fn.FreeMemory(vkDevice, memory, nullptr);
        }
        mMemoriesToDelete.ClearUpTo(completedSerial);

        for (VkPipelineLayout layout : mPipelineLayoutsToDelete.IterateUpTo(completedSerial)) {
            mDevice->fn.DestroyPipelineLayout(vkDevice, layout, nullptr);
        }
        mPipelineLayoutsToDelete.ClearUpTo(completedSerial);

        for (VkRenderPass renderPass : mRenderPassesToDelete.IterateUpTo(completedSerial)) {
            mDevice->fn.DestroyRenderPass(vkDevice, renderPass, nullptr);
        }
        mRenderPassesToDelete.ClearUpTo(completedSerial);

        for (VkFramebuffer framebuffer : mFramebuffersToDelete.IterateUpTo(completedSerial)) {
            mDevice->fn.DestroyFramebuffer(vkDevice, framebuffer, nullptr);
        }
        mFramebuffersToDelete.ClearUpTo(completedSerial);

        for (VkImageView view : mImageViewsToDelete.IterateUpTo(completedSerial)) {
            mDevice->fn.DestroyImageView(vkDevice, view, nullptr);
        }
        mImageViewsToDelete.ClearUpTo(completedSerial);

        for (VkShaderModule module : mShaderModulesToDelete.IterateUpTo(completedSerial)) {
            mDevice->fn.DestroyShaderModule(vkDevice, module, nullptr);
        }
        mShaderModulesToDelete.ClearUpTo(completedSerial);

        for (VkPipeline pipeline : mPipelinesToDelete.IterateUpTo(completedSerial)) {
            mDevice->fn.DestroyPipeline(vkDevice, pipeline, nullptr);
        }
        mPipelinesToDelete.ClearUpTo(completedSerial);

        // Vulkan swapchains must be destroyed before their corresponding VkSurface
        for (VkSwapchainKHR swapChain : mSwapChainsToDelete.IterateUpTo(completedSerial)) {
            mDevice->fn.DestroySwapchainKHR(vkDevice, swapChain, nullptr);
        }
        mSwapChainsToDelete.ClearUpTo(completedSerial);
        for (VkSurfaceKHR surface : mSurfacesToDelete.IterateUpTo(completedSerial)) {
            mDevice->fn.DestroySurfaceKHR(instance, surface, nullptr);
        }
        mSurfacesToDelete.ClearUpTo(completedSerial);

        for (VkSemaphore semaphore : mSemaphoresToDelete.IterateUpTo(completedSerial)) {
            mDevice->fn.DestroySemaphore(vkDevice, semaphore, nullptr);
        }
        mSemaphoresToDelete.ClearUpTo(completedSerial);

        for (VkDescriptorPool pool : mDescriptorPoolsToDelete.IterateUpTo(completedSerial)) {
            mDevice->fn.DestroyDescriptorPool(vkDevice, pool, nullptr);
        }
        mDescriptorPoolsToDelete.ClearUpTo(completedSerial);

        for (VkSampler sampler : mSamplersToDelete.IterateUpTo(completedSerial)) {
            mDevice->fn.DestroySampler(vkDevice, sampler, nullptr);
        }
        mSamplersToDelete.ClearUpTo(completedSerial);
    }

}}  // namespace dawn_native::vulkan
