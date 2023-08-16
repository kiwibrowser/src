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

#ifndef DAWNNATIVE_VULKAN_RESOURCEHEAPVK_H_
#define DAWNNATIVE_VULKAN_RESOURCEHEAPVK_H_

#include "common/vulkan_platform.h"
#include "dawn_native/ResourceHeap.h"

namespace dawn_native { namespace vulkan {

    // Wrapper for physical memory used with or without a resource object.
    class ResourceHeap : public ResourceHeapBase {
      public:
        ResourceHeap(VkDeviceMemory memory, size_t memoryType);
        ~ResourceHeap() = default;

        VkDeviceMemory GetMemory() const;
        size_t GetMemoryType() const;

      private:
        VkDeviceMemory mMemory = VK_NULL_HANDLE;
        size_t mMemoryType = 0;
    };

}}  // namespace dawn_native::vulkan

#endif  // DAWNNATIVE_VULKAN_RESOURCEHEAPVK_H_
