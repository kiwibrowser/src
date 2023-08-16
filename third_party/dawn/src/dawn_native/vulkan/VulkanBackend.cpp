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

// VulkanBackend.cpp: contains the definition of symbols exported by VulkanBackend.h so that they
// can be compiled twice: once export (shared library), once not exported (static library)

// Include vulkan_platform.h before VulkanBackend.h includes vulkan.h so that we use our version
// of the non-dispatchable handles.
#include "common/vulkan_platform.h"

#include "dawn_native/VulkanBackend.h"

#include "common/SwapChainUtils.h"
#include "dawn_native/vulkan/DeviceVk.h"
#include "dawn_native/vulkan/NativeSwapChainImplVk.h"
#include "dawn_native/vulkan/TextureVk.h"

namespace dawn_native { namespace vulkan {

    VkInstance GetInstance(WGPUDevice device) {
        Device* backendDevice = reinterpret_cast<Device*>(device);
        return backendDevice->GetVkInstance();
    }

    DAWN_NATIVE_EXPORT PFN_vkVoidFunction GetInstanceProcAddr(WGPUDevice device,
                                                              const char* pName) {
        Device* backendDevice = reinterpret_cast<Device*>(device);
        return (*backendDevice->fn.GetInstanceProcAddr)(backendDevice->GetVkInstance(), pName);
    }

    // Explicitly export this function because it uses the "native" type for surfaces while the
    // header as seen in this file uses the wrapped type.
    DAWN_NATIVE_EXPORT DawnSwapChainImplementation
    CreateNativeSwapChainImpl(WGPUDevice device, ::VkSurfaceKHR surfaceNative) {
        Device* backendDevice = reinterpret_cast<Device*>(device);
        VkSurfaceKHR surface = VkSurfaceKHR::CreateFromHandle(surfaceNative);

        DawnSwapChainImplementation impl;
        impl = CreateSwapChainImplementation(new NativeSwapChainImpl(backendDevice, surface));
        impl.textureUsage = WGPUTextureUsage_Present;

        return impl;
    }

    WGPUTextureFormat GetNativeSwapChainPreferredFormat(
        const DawnSwapChainImplementation* swapChain) {
        NativeSwapChainImpl* impl = reinterpret_cast<NativeSwapChainImpl*>(swapChain->userData);
        return static_cast<WGPUTextureFormat>(impl->GetPreferredFormat());
    }

#ifdef DAWN_PLATFORM_LINUX
    ExternalImageDescriptorFD::ExternalImageDescriptorFD(ExternalImageDescriptorType descType)
        : ExternalImageDescriptor(descType) {
    }

    ExternalImageDescriptorOpaqueFD::ExternalImageDescriptorOpaqueFD()
        : ExternalImageDescriptorFD(ExternalImageDescriptorType::OpaqueFD) {
    }

    ExternalImageDescriptorDmaBuf::ExternalImageDescriptorDmaBuf()
        : ExternalImageDescriptorFD(ExternalImageDescriptorType::DmaBuf) {
    }

    int ExportSignalSemaphoreOpaqueFD(WGPUDevice cDevice, WGPUTexture cTexture) {
        Device* device = reinterpret_cast<Device*>(cDevice);
        Texture* texture = reinterpret_cast<Texture*>(cTexture);

        if (!texture) {
            return -1;
        }

        ExternalSemaphoreHandle outHandle;
        if (device->ConsumedError(device->SignalAndExportExternalTexture(texture, &outHandle))) {
            return -1;
        }

        return outHandle;
    }

    WGPUTexture WrapVulkanImage(WGPUDevice cDevice, const ExternalImageDescriptor* descriptor) {
        Device* device = reinterpret_cast<Device*>(cDevice);

        switch (descriptor->type) {
            case ExternalImageDescriptorType::OpaqueFD:
            case ExternalImageDescriptorType::DmaBuf: {
                const ExternalImageDescriptorFD* fdDescriptor =
                    static_cast<const ExternalImageDescriptorFD*>(descriptor);
                TextureBase* texture = device->CreateTextureWrappingVulkanImage(
                    descriptor, fdDescriptor->memoryFD, fdDescriptor->waitFDs);
                return reinterpret_cast<WGPUTexture>(texture);
            }
            default:
                return nullptr;
        }
    }
#endif

}}  // namespace dawn_native::vulkan
