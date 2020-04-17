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

#include "dawn_native/vulkan/StagingBufferVk.h"
#include "dawn_native/vulkan/DeviceVk.h"
#include "dawn_native/vulkan/FencedDeleter.h"
#include "dawn_native/vulkan/MemoryAllocator.h"

namespace dawn_native { namespace vulkan {

    StagingBuffer::StagingBuffer(size_t size, Device* device)
        : StagingBufferBase(size), mDevice(device) {
    }

    MaybeError StagingBuffer::Initialize() {
        VkBufferCreateInfo createInfo;
        createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        createInfo.pNext = nullptr;
        createInfo.flags = 0;
        createInfo.size = GetSize();
        createInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0;
        createInfo.pQueueFamilyIndices = 0;

        if (mDevice->fn.CreateBuffer(mDevice->GetVkDevice(), &createInfo, nullptr, &mBuffer) !=
            VK_SUCCESS) {
            return DAWN_CONTEXT_LOST_ERROR("Unable to create staging buffer.");
        }

        VkMemoryRequirements requirements;
        mDevice->fn.GetBufferMemoryRequirements(mDevice->GetVkDevice(), mBuffer, &requirements);

        if (!mDevice->GetMemoryAllocator()->Allocate(requirements, true, &mAllocation)) {
            return DAWN_CONTEXT_LOST_ERROR("Unable to allocate memory for staging buffer.");
        }

        if (mDevice->fn.BindBufferMemory(mDevice->GetVkDevice(), mBuffer, mAllocation.GetMemory(),
                                         mAllocation.GetMemoryOffset()) != VK_SUCCESS) {
            return DAWN_CONTEXT_LOST_ERROR("Unable to attach memory to the staging buffer.");
        }

        mMappedPointer = mAllocation.GetMappedPointer();
        if (mMappedPointer == nullptr) {
            return DAWN_CONTEXT_LOST_ERROR("Unable to map staging buffer.");
        }

        return {};
    }

    StagingBuffer::~StagingBuffer() {
        mMappedPointer = nullptr;
        mDevice->GetFencedDeleter()->DeleteWhenUnused(mBuffer);
        mDevice->GetMemoryAllocator()->Free(&mAllocation);
    }

    VkBuffer StagingBuffer::GetBufferHandle() const {
        return mBuffer;
    }

}}  // namespace dawn_native::vulkan