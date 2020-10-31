// Copyright 2020 The Dawn Authors
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

#ifndef DAWNNATIVE_VULKAN_DESCRIPTORSETALLOCATOR_H_
#define DAWNNATIVE_VULKAN_DESCRIPTORSETALLOCATOR_H_

#include "common/SerialQueue.h"
#include "common/vulkan_platform.h"
#include "dawn_native/Error.h"
#include "dawn_native/vulkan/DescriptorSetAllocation.h"

#include <map>
#include <vector>

namespace dawn_native { namespace vulkan {

    class BindGroupLayout;

    class DescriptorSetAllocator {
        using PoolIndex = uint32_t;
        using SetIndex = uint16_t;

      public:
        DescriptorSetAllocator(BindGroupLayout* layout,
                               std::map<VkDescriptorType, uint32_t> descriptorCountPerType);
        ~DescriptorSetAllocator();

        ResultOrError<DescriptorSetAllocation> Allocate();
        void Deallocate(DescriptorSetAllocation* allocationInfo);
        void FinishDeallocation(Serial completedSerial);

      private:
        MaybeError AllocateDescriptorPool();

        BindGroupLayout* mLayout;

        std::vector<VkDescriptorPoolSize> mPoolSizes;
        SetIndex mMaxSets;

        struct DescriptorPool {
            VkDescriptorPool vkPool;
            std::vector<VkDescriptorSet> sets;
            std::vector<SetIndex> freeSetIndices;
        };

        std::vector<PoolIndex> mAvailableDescriptorPoolIndices;
        std::vector<DescriptorPool> mDescriptorPools;

        struct Deallocation {
            PoolIndex poolIndex;
            SetIndex setIndex;
        };
        SerialQueue<Deallocation> mPendingDeallocations;
        Serial mLastDeallocationSerial = 0;
    };

}}  // namespace dawn_native::vulkan

#endif  // DAWNNATIVE_VULKAN_DESCRIPTORSETALLOCATOR_H_
