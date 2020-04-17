// Copyright 2018 The Dawn Authors
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

#include "dawn_native/vulkan/SamplerVk.h"

#include "dawn_native/vulkan/DeviceVk.h"
#include "dawn_native/vulkan/FencedDeleter.h"
#include "dawn_native/vulkan/UtilsVulkan.h"

namespace dawn_native { namespace vulkan {

    namespace {
        VkSamplerAddressMode VulkanSamplerAddressMode(dawn::AddressMode mode) {
            switch (mode) {
                case dawn::AddressMode::Repeat:
                    return VK_SAMPLER_ADDRESS_MODE_REPEAT;
                case dawn::AddressMode::MirroredRepeat:
                    return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
                case dawn::AddressMode::ClampToEdge:
                    return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
                default:
                    UNREACHABLE();
            }
        }

        VkFilter VulkanSamplerFilter(dawn::FilterMode filter) {
            switch (filter) {
                case dawn::FilterMode::Linear:
                    return VK_FILTER_LINEAR;
                case dawn::FilterMode::Nearest:
                    return VK_FILTER_NEAREST;
                default:
                    UNREACHABLE();
            }
        }

        VkSamplerMipmapMode VulkanMipMapMode(dawn::FilterMode filter) {
            switch (filter) {
                case dawn::FilterMode::Linear:
                    return VK_SAMPLER_MIPMAP_MODE_LINEAR;
                case dawn::FilterMode::Nearest:
                    return VK_SAMPLER_MIPMAP_MODE_NEAREST;
                default:
                    UNREACHABLE();
            }
        }
    }  // anonymous namespace

    Sampler::Sampler(Device* device, const SamplerDescriptor* descriptor)
        : SamplerBase(device, descriptor), mDevice(device) {
        VkSamplerCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        createInfo.pNext = nullptr;
        createInfo.flags = 0;
        createInfo.magFilter = VulkanSamplerFilter(descriptor->magFilter);
        createInfo.minFilter = VulkanSamplerFilter(descriptor->minFilter);
        createInfo.mipmapMode = VulkanMipMapMode(descriptor->mipmapFilter);
        createInfo.addressModeU = VulkanSamplerAddressMode(descriptor->addressModeU);
        createInfo.addressModeV = VulkanSamplerAddressMode(descriptor->addressModeV);
        createInfo.addressModeW = VulkanSamplerAddressMode(descriptor->addressModeW);
        createInfo.mipLodBias = 0.0f;
        createInfo.anisotropyEnable = VK_FALSE;
        createInfo.maxAnisotropy = 1.0f;
        createInfo.compareOp = ToVulkanCompareOp(descriptor->compareFunction);
        createInfo.compareEnable = createInfo.compareOp == VK_COMPARE_OP_NEVER ? VK_FALSE : VK_TRUE;
        createInfo.minLod = descriptor->lodMinClamp;
        createInfo.maxLod = descriptor->lodMaxClamp;
        createInfo.unnormalizedCoordinates = VK_FALSE;

        if (device->fn.CreateSampler(device->GetVkDevice(), &createInfo, nullptr, &mHandle) !=
            VK_SUCCESS) {
            ASSERT(false);
        }
    }

    Sampler::~Sampler() {
        if (mHandle != VK_NULL_HANDLE) {
            mDevice->GetFencedDeleter()->DeleteWhenUnused(mHandle);
            mHandle = VK_NULL_HANDLE;
        }
    }

    VkSampler Sampler::GetHandle() const {
        return mHandle;
    }

}}  // namespace dawn_native::vulkan
