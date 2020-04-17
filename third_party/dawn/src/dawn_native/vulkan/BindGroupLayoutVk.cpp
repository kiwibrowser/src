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

#include "dawn_native/vulkan/BindGroupLayoutVk.h"

#include "common/BitSetIterator.h"
#include "dawn_native/vulkan/DeviceVk.h"

namespace dawn_native { namespace vulkan {

    namespace {

        VkShaderStageFlags VulkanShaderStageFlags(dawn::ShaderStageBit stages) {
            VkShaderStageFlags flags = 0;

            if (stages & dawn::ShaderStageBit::Vertex) {
                flags |= VK_SHADER_STAGE_VERTEX_BIT;
            }
            if (stages & dawn::ShaderStageBit::Fragment) {
                flags |= VK_SHADER_STAGE_FRAGMENT_BIT;
            }
            if (stages & dawn::ShaderStageBit::Compute) {
                flags |= VK_SHADER_STAGE_COMPUTE_BIT;
            }

            return flags;
        }

    }  // anonymous namespace

    VkDescriptorType VulkanDescriptorType(dawn::BindingType type) {
        switch (type) {
            case dawn::BindingType::UniformBuffer:
                return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            case dawn::BindingType::Sampler:
                return VK_DESCRIPTOR_TYPE_SAMPLER;
            case dawn::BindingType::SampledTexture:
                return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
            case dawn::BindingType::StorageBuffer:
                return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            case dawn::BindingType::DynamicUniformBuffer:
                return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
            case dawn::BindingType::DynamicStorageBuffer:
                return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
            default:
                UNREACHABLE();
        }
    }

    BindGroupLayout::BindGroupLayout(Device* device, const BindGroupLayoutDescriptor* descriptor)
        : BindGroupLayoutBase(device, descriptor) {
        const auto& info = GetBindingInfo();

        // Compute the bindings that will be chained in the DescriptorSetLayout create info. We add
        // one entry per binding set. This might be optimized by computing continuous ranges of
        // bindings of the same type.
        uint32_t numBindings = 0;
        std::array<VkDescriptorSetLayoutBinding, kMaxBindingsPerGroup> bindings;
        for (uint32_t bindingIndex : IterateBitSet(info.mask)) {
            auto& binding = bindings[numBindings];
            binding.binding = bindingIndex;
            binding.descriptorType = VulkanDescriptorType(info.types[bindingIndex]);
            binding.descriptorCount = 1;
            binding.stageFlags = VulkanShaderStageFlags(info.visibilities[bindingIndex]);
            binding.pImmutableSamplers = nullptr;

            numBindings++;
        }

        VkDescriptorSetLayoutCreateInfo createInfo;
        createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        createInfo.pNext = nullptr;
        createInfo.flags = 0;
        createInfo.bindingCount = numBindings;
        createInfo.pBindings = bindings.data();

        if (device->fn.CreateDescriptorSetLayout(device->GetVkDevice(), &createInfo, nullptr,
                                                 &mHandle) != VK_SUCCESS) {
            ASSERT(false);
        }
    }

    BindGroupLayout::~BindGroupLayout() {
        // DescriptorSetLayout aren't used by execution on the GPU and can be deleted at any time,
        // so we destroy mHandle immediately instead of using the FencedDeleter
        if (mHandle != VK_NULL_HANDLE) {
            Device* device = ToBackend(GetDevice());
            device->fn.DestroyDescriptorSetLayout(device->GetVkDevice(), mHandle, nullptr);
            mHandle = VK_NULL_HANDLE;
        }
    }

    VkDescriptorSetLayout BindGroupLayout::GetHandle() const {
        return mHandle;
    }

    BindGroupLayout::PoolSizeSpec BindGroupLayout::ComputePoolSizes(uint32_t* numPoolSizes) const {
        uint32_t numSizes = 0;
        PoolSizeSpec result{};

        // Defines an array and indices into it that will contain for each sampler type at which
        // position it is in the PoolSizeSpec, or -1 if it isn't present yet.
        enum DescriptorType {
            UNIFORM_BUFFER,
            SAMPLER,
            SAMPLED_IMAGE,
            STORAGE_BUFFER,
            MAX_TYPE,
        };
        static_assert(MAX_TYPE == kMaxPoolSizesNeeded, "");
        auto ToDescriptorType = [](dawn::BindingType type) -> DescriptorType {
            switch (type) {
                case dawn::BindingType::UniformBuffer:
                case dawn::BindingType::DynamicUniformBuffer:
                    return UNIFORM_BUFFER;
                case dawn::BindingType::Sampler:
                    return SAMPLER;
                case dawn::BindingType::SampledTexture:
                    return SAMPLED_IMAGE;
                case dawn::BindingType::StorageBuffer:
                case dawn::BindingType::DynamicStorageBuffer:
                    return STORAGE_BUFFER;
                default:
                    UNREACHABLE();
            }
        };

        std::array<int, MAX_TYPE> descriptorTypeIndex;
        descriptorTypeIndex.fill(-1);

        const auto& info = GetBindingInfo();
        for (uint32_t bindingIndex : IterateBitSet(info.mask)) {
            DescriptorType type = ToDescriptorType(info.types[bindingIndex]);

            if (descriptorTypeIndex[type] == -1) {
                descriptorTypeIndex[type] = numSizes;
                result[numSizes].type = VulkanDescriptorType(info.types[bindingIndex]);
                result[numSizes].descriptorCount = 1;
                numSizes++;
            } else {
                result[descriptorTypeIndex[type]].descriptorCount++;
            }
        }

        *numPoolSizes = numSizes;
        return result;
    }
}}  // namespace dawn_native::vulkan
