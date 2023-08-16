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

#ifndef DAWNNATIVE_VULKAN_ADAPTERVK_H_
#define DAWNNATIVE_VULKAN_ADAPTERVK_H_

#include "dawn_native/Adapter.h"

#include "common/vulkan_platform.h"
#include "dawn_native/vulkan/VulkanInfo.h"

namespace dawn_native { namespace vulkan {

    class Backend;

    class Adapter : public AdapterBase {
      public:
        Adapter(Backend* backend, VkPhysicalDevice physicalDevice);
        ~Adapter() override = default;

        const VulkanDeviceInfo& GetDeviceInfo() const;
        VkPhysicalDevice GetPhysicalDevice() const;
        Backend* GetBackend() const;

        MaybeError Initialize();

      private:
        ResultOrError<DeviceBase*> CreateDeviceImpl(const DeviceDescriptor* descriptor) override;
        void InitializeSupportedExtensions();

        VkPhysicalDevice mPhysicalDevice;
        Backend* mBackend;
        VulkanDeviceInfo mDeviceInfo = {};
    };

}}  // namespace dawn_native::vulkan

#endif  // DAWNNATIVE_VULKAN_ADAPTERVK_H_
