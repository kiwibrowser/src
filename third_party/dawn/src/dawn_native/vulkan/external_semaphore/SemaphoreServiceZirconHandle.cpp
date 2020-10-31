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

#include "dawn_native/vulkan/AdapterVk.h"
#include "dawn_native/vulkan/BackendVk.h"
#include "dawn_native/vulkan/DeviceVk.h"
#include "dawn_native/vulkan/VulkanError.h"
#include "dawn_native/vulkan/external_semaphore/SemaphoreService.h"

namespace dawn_native { namespace vulkan { namespace external_semaphore {

    Service::Service(Device* device) : mDevice(device) {
        mSupported = device->GetDeviceInfo().hasExt(DeviceExt::ExternalSemaphoreZirconHandle);

        // Early out before we try using extension functions
        if (!mSupported) {
            return;
        }

        VkPhysicalDeviceExternalSemaphoreInfoKHR semaphoreInfo;
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_SEMAPHORE_INFO_KHR;
        semaphoreInfo.pNext = nullptr;
        semaphoreInfo.handleType = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_TEMP_ZIRCON_EVENT_BIT_FUCHSIA;

        VkExternalSemaphorePropertiesKHR semaphoreProperties;
        semaphoreProperties.sType = VK_STRUCTURE_TYPE_EXTERNAL_SEMAPHORE_PROPERTIES_KHR;
        semaphoreProperties.pNext = nullptr;

        mDevice->fn.GetPhysicalDeviceExternalSemaphoreProperties(
            ToBackend(mDevice->GetAdapter())->GetPhysicalDevice(), &semaphoreInfo,
            &semaphoreProperties);

        VkFlags requiredFlags = VK_EXTERNAL_SEMAPHORE_FEATURE_EXPORTABLE_BIT_KHR |
                                VK_EXTERNAL_SEMAPHORE_FEATURE_IMPORTABLE_BIT_KHR;
        mSupported =
            mSupported &&
            ((semaphoreProperties.externalSemaphoreFeatures & requiredFlags) == requiredFlags);
    }

    Service::~Service() = default;

    bool Service::Supported() {
        return mSupported;
    }

    ResultOrError<VkSemaphore> Service::ImportSemaphore(ExternalSemaphoreHandle handle) {
        if (handle == ZX_HANDLE_INVALID) {
            return DAWN_VALIDATION_ERROR("Trying to import semaphore with invalid handle");
        }

        VkSemaphore semaphore = VK_NULL_HANDLE;
        VkSemaphoreCreateInfo info;
        info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        info.pNext = nullptr;
        info.flags = 0;

        DAWN_TRY(CheckVkSuccess(
            mDevice->fn.CreateSemaphore(mDevice->GetVkDevice(), &info, nullptr, &*semaphore),
            "vkCreateSemaphore"));

        VkImportSemaphoreZirconHandleInfoFUCHSIA importSempahoreHandleInfo;
        importSempahoreHandleInfo.sType =
            VK_STRUCTURE_TYPE_TEMP_IMPORT_SEMAPHORE_ZIRCON_HANDLE_INFO_FUCHSIA;
        importSempahoreHandleInfo.pNext = nullptr;
        importSempahoreHandleInfo.semaphore = semaphore;
        importSempahoreHandleInfo.flags = 0;
        importSempahoreHandleInfo.handleType =
            VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_TEMP_ZIRCON_EVENT_BIT_FUCHSIA;
        importSempahoreHandleInfo.handle = handle;

        MaybeError status = CheckVkSuccess(mDevice->fn.ImportSemaphoreZirconHandleFUCHSIA(
                                               mDevice->GetVkDevice(), &importSempahoreHandleInfo),
                                           "vkImportSemaphoreZirconHandleFUCHSIA");

        if (status.IsError()) {
            mDevice->fn.DestroySemaphore(mDevice->GetVkDevice(), semaphore, nullptr);
            DAWN_TRY(std::move(status));
        }

        return semaphore;
    }

    ResultOrError<VkSemaphore> Service::CreateExportableSemaphore() {
        VkExportSemaphoreCreateInfoKHR exportSemaphoreInfo;
        exportSemaphoreInfo.sType = VK_STRUCTURE_TYPE_EXPORT_SEMAPHORE_CREATE_INFO_KHR;
        exportSemaphoreInfo.pNext = nullptr;
        exportSemaphoreInfo.handleTypes =
            VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_TEMP_ZIRCON_EVENT_BIT_FUCHSIA;

        VkSemaphoreCreateInfo semaphoreCreateInfo;
        semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        semaphoreCreateInfo.pNext = &exportSemaphoreInfo;
        semaphoreCreateInfo.flags = 0;

        VkSemaphore signalSemaphore;
        DAWN_TRY(
            CheckVkSuccess(mDevice->fn.CreateSemaphore(mDevice->GetVkDevice(), &semaphoreCreateInfo,
                                                       nullptr, &*signalSemaphore),
                           "vkCreateSemaphore"));
        return signalSemaphore;
    }

    ResultOrError<ExternalSemaphoreHandle> Service::ExportSemaphore(VkSemaphore semaphore) {
        VkSemaphoreGetZirconHandleInfoFUCHSIA semaphoreGetHandleInfo;
        semaphoreGetHandleInfo.sType =
            VK_STRUCTURE_TYPE_TEMP_SEMAPHORE_GET_ZIRCON_HANDLE_INFO_FUCHSIA;
        semaphoreGetHandleInfo.pNext = nullptr;
        semaphoreGetHandleInfo.semaphore = semaphore;
        semaphoreGetHandleInfo.handleType =
            VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_TEMP_ZIRCON_EVENT_BIT_FUCHSIA;

        zx_handle_t handle = ZX_HANDLE_INVALID;
        DAWN_TRY(CheckVkSuccess(mDevice->fn.GetSemaphoreZirconHandleFUCHSIA(
                                    mDevice->GetVkDevice(), &semaphoreGetHandleInfo, &handle),
                                "VkSemaphoreGetZirconHandleInfoFUCHSIA"));

        ASSERT(handle != ZX_HANDLE_INVALID);
        return handle;
    }

}}}  // namespace dawn_native::vulkan::external_semaphore
