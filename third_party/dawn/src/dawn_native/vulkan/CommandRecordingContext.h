// Copyright 2019 The Dawn Authors
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
#ifndef DAWNNATIVE_VULKAN_COMMANDRECORDINGCONTEXT_H_
#define DAWNNATIVE_VULKAN_COMMANDRECORDINGCONTEXT_H_

#include "common/vulkan_platform.h"

#include "dawn_native/vulkan/BufferVk.h"

#include <vector>

namespace dawn_native { namespace vulkan {
    class Buffer;

    // Used to track operations that are handled after recording.
    // Currently only tracks semaphores, but may be used to do barrier coalescing in the future.
    struct CommandRecordingContext {
        VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
        std::vector<VkSemaphore> waitSemaphores = {};
        std::vector<VkSemaphore> signalSemaphores = {};

        // The internal buffers used in the workaround of texture-to-texture copies with compressed
        // formats.
        std::vector<Ref<Buffer>> tempBuffers;

        // For Device state tracking only.
        VkCommandPool commandPool = VK_NULL_HANDLE;
        bool used = false;
    };

}}  // namespace dawn_native::vulkan

#endif  // DAWNNATIVE_VULKAN_COMMANDRECORDINGCONTEXT_H_
