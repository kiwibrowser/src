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

#include "dawn_native/vulkan/VulkanError.h"

#include <string>

namespace dawn_native { namespace vulkan {

    const char* VkResultAsString(::VkResult result) {
        // Convert to a int32_t to silence and MSVC warning that the fake errors don't appear in
        // the original VkResult enum.
        int32_t code = static_cast<int32_t>(result);

        switch (code) {
            case VK_SUCCESS:
                return "VK_SUCCESS";
            case VK_NOT_READY:
                return "VK_NOT_READY";
            case VK_TIMEOUT:
                return "VK_TIMEOUT";
            case VK_EVENT_SET:
                return "VK_EVENT_SET";
            case VK_EVENT_RESET:
                return "VK_EVENT_RESET";
            case VK_INCOMPLETE:
                return "VK_INCOMPLETE";
            case VK_ERROR_OUT_OF_HOST_MEMORY:
                return "VK_ERROR_OUT_OF_HOST_MEMORY";
            case VK_ERROR_OUT_OF_DEVICE_MEMORY:
                return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
            case VK_ERROR_INITIALIZATION_FAILED:
                return "VK_ERROR_INITIALIZATION_FAILED";
            case VK_ERROR_DEVICE_LOST:
                return "VK_ERROR_DEVICE_LOST";
            case VK_ERROR_MEMORY_MAP_FAILED:
                return "VK_ERROR_MEMORY_MAP_FAILED";
            case VK_ERROR_LAYER_NOT_PRESENT:
                return "VK_ERROR_LAYER_NOT_PRESENT";
            case VK_ERROR_EXTENSION_NOT_PRESENT:
                return "VK_ERROR_EXTENSION_NOT_PRESENT";
            case VK_ERROR_FEATURE_NOT_PRESENT:
                return "VK_ERROR_FEATURE_NOT_PRESENT";
            case VK_ERROR_INCOMPATIBLE_DRIVER:
                return "VK_ERROR_INCOMPATIBLE_DRIVER";
            case VK_ERROR_TOO_MANY_OBJECTS:
                return "VK_ERROR_TOO_MANY_OBJECTS";
            case VK_ERROR_FORMAT_NOT_SUPPORTED:
                return "VK_ERROR_FORMAT_NOT_SUPPORTED";
            case VK_ERROR_FRAGMENTED_POOL:
                return "VK_ERROR_FRAGMENTED_POOL";
            case VK_FAKE_DEVICE_OOM_FOR_TESTING:
                return "VK_FAKE_DEVICE_OOM_FOR_TESTING";
            case VK_FAKE_ERROR_FOR_TESTING:
                return "VK_FAKE_ERROR_FOR_TESTING";
            default:
                return "<Unknown VkResult>";
        }
    }

    MaybeError CheckVkSuccessImpl(VkResult result, const char* context) {
        if (DAWN_LIKELY(result == VK_SUCCESS)) {
            return {};
        }

        std::string message = std::string(context) + " failed with " + VkResultAsString(result);

        if (result == VK_ERROR_DEVICE_LOST) {
            return DAWN_DEVICE_LOST_ERROR(message);
        } else {
            return DAWN_INTERNAL_ERROR(message);
        }
    }

    MaybeError CheckVkOOMThenSuccessImpl(VkResult result, const char* context) {
        if (DAWN_LIKELY(result == VK_SUCCESS)) {
            return {};
        }

        std::string message = std::string(context) + " failed with " + VkResultAsString(result);

        if (result == VK_ERROR_OUT_OF_DEVICE_MEMORY || result == VK_ERROR_OUT_OF_HOST_MEMORY ||
            result == VK_FAKE_DEVICE_OOM_FOR_TESTING) {
            return DAWN_OUT_OF_MEMORY_ERROR(message);
        } else if (result == VK_ERROR_DEVICE_LOST) {
            return DAWN_DEVICE_LOST_ERROR(message);
        } else {
            return DAWN_INTERNAL_ERROR(message);
        }
    }

}}  // namespace dawn_native::vulkan
