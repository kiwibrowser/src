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

#include "dawn_native/vulkan/DeviceVk.h"
#include "dawn_native/vulkan/external_semaphore/SemaphoreService.h"

namespace dawn_native { namespace vulkan { namespace external_semaphore {

    Service::Service(Device* device) : mDevice(device) {
        DAWN_UNUSED(mDevice);
        DAWN_UNUSED(mSupported);
    }

    Service::~Service() = default;

    bool Service::Supported() {
        return false;
    }

    ResultOrError<VkSemaphore> Service::ImportSemaphore(ExternalSemaphoreHandle handle) {
        return DAWN_UNIMPLEMENTED_ERROR("Using null semaphore service to interop inside Vulkan");
    }

    ResultOrError<VkSemaphore> Service::CreateExportableSemaphore() {
        return DAWN_UNIMPLEMENTED_ERROR("Using null semaphore service to interop inside Vulkan");
    }

    ResultOrError<ExternalSemaphoreHandle> Service::ExportSemaphore(VkSemaphore semaphore) {
        return DAWN_UNIMPLEMENTED_ERROR("Using null semaphore service to interop inside Vulkan");
    }

}}}  // namespace dawn_native::vulkan::external_semaphore
