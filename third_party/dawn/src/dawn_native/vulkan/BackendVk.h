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

#ifndef DAWNNATIVE_VULKAN_BACKENDVK_H_
#define DAWNNATIVE_VULKAN_BACKENDVK_H_

#include "dawn_native/BackendConnection.h"

#include "common/DynamicLib.h"
#include "dawn_native/vulkan/VulkanFunctions.h"
#include "dawn_native/vulkan/VulkanInfo.h"

namespace dawn_native { namespace vulkan {

    class Backend : public BackendConnection {
      public:
        Backend(InstanceBase* instance);
        ~Backend() override;

        const VulkanFunctions& GetFunctions() const;
        VkInstance GetVkInstance() const;
        const VulkanGlobalInfo& GetGlobalInfo() const;

        MaybeError Initialize(bool useSwiftshader);

        std::vector<std::unique_ptr<AdapterBase>> DiscoverDefaultAdapters() override;

      private:
        MaybeError LoadVulkan(bool useSwiftshader);
        ResultOrError<VulkanGlobalKnobs> CreateInstance();

        MaybeError RegisterDebugReport();
        static VKAPI_ATTR VkBool32 VKAPI_CALL
        OnDebugReportCallback(VkDebugReportFlagsEXT flags,
                              VkDebugReportObjectTypeEXT objectType,
                              uint64_t object,
                              size_t location,
                              int32_t messageCode,
                              const char* pLayerPrefix,
                              const char* pMessage,
                              void* pUserdata);

        DynamicLib mVulkanLib;
        VulkanGlobalInfo mGlobalInfo = {};
        VkInstance mInstance = VK_NULL_HANDLE;
        VulkanFunctions mFunctions;

        VkDebugReportCallbackEXT mDebugReportCallback = VK_NULL_HANDLE;

        std::vector<VkPhysicalDevice> mPhysicalDevices;
    };

}}  // namespace dawn_native::vulkan

#endif  // DAWNNATIVE_VULKAN_BACKENDVK_H_
