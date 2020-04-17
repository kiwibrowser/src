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

#include "dawn_native/vulkan/PipelineLayoutVk.h"

#include "dawn_native/vulkan/BindGroupLayoutVk.h"
#include "dawn_native/vulkan/DeviceVk.h"
#include "dawn_native/vulkan/FencedDeleter.h"

#include "common/BitSetIterator.h"

namespace dawn_native { namespace vulkan {

    PipelineLayout::PipelineLayout(Device* device, const PipelineLayoutDescriptor* descriptor)
        : PipelineLayoutBase(device, descriptor) {
        // Compute the array of VkDescriptorSetLayouts that will be chained in the create info.
        // TODO(cwallez@chromium.org) Vulkan doesn't allow holes in this array, should we expose
        // this constraints at the Dawn level?
        uint32_t numSetLayouts = 0;
        std::array<VkDescriptorSetLayout, kMaxBindGroups> setLayouts;
        for (uint32_t setIndex : IterateBitSet(GetBindGroupLayoutsMask())) {
            setLayouts[numSetLayouts] = ToBackend(GetBindGroupLayout(setIndex))->GetHandle();
            numSetLayouts++;
        }

        VkPipelineLayoutCreateInfo createInfo;
        createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        createInfo.pNext = nullptr;
        createInfo.flags = 0;
        createInfo.setLayoutCount = numSetLayouts;
        createInfo.pSetLayouts = setLayouts.data();
        createInfo.pushConstantRangeCount = 0;
        createInfo.pPushConstantRanges = nullptr;

        if (device->fn.CreatePipelineLayout(device->GetVkDevice(), &createInfo, nullptr,
                                            &mHandle) != VK_SUCCESS) {
            ASSERT(false);
        }
    }

    PipelineLayout::~PipelineLayout() {
        if (mHandle != VK_NULL_HANDLE) {
            ToBackend(GetDevice())->GetFencedDeleter()->DeleteWhenUnused(mHandle);
            mHandle = VK_NULL_HANDLE;
        }
    }

    VkPipelineLayout PipelineLayout::GetHandle() const {
        return mHandle;
    }

}}  // namespace dawn_native::vulkan
