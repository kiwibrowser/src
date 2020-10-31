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

#include "dawn_native/vulkan/DescriptorSetAllocator.h"

#include "dawn_native/vulkan/BindGroupLayoutVk.h"
#include "dawn_native/vulkan/DeviceVk.h"
#include "dawn_native/vulkan/FencedDeleter.h"
#include "dawn_native/vulkan/VulkanError.h"

namespace dawn_native { namespace vulkan {

    // TODO(enga): Figure out this value.
    static constexpr uint32_t kMaxDescriptorsPerPool = 512;

    DescriptorSetAllocator::DescriptorSetAllocator(
        BindGroupLayout* layout,
        std::map<VkDescriptorType, uint32_t> descriptorCountPerType)
        : mLayout(layout) {
        ASSERT(layout != nullptr);

        // Compute the total number of descriptors for this layout.
        uint32_t totalDescriptorCount = 0;
        mPoolSizes.reserve(descriptorCountPerType.size());
        for (const auto& it : descriptorCountPerType) {
            ASSERT(it.second > 0);
            totalDescriptorCount += it.second;
            mPoolSizes.push_back(VkDescriptorPoolSize{it.first, it.second});
        }

        if (totalDescriptorCount == 0) {
            // Vulkan requires that valid usage of vkCreateDescriptorPool must have a non-zero
            // number of pools, each of which has non-zero descriptor counts.
            // Since the descriptor set layout is empty, we should be able to allocate
            // |kMaxDescriptorsPerPool| sets from this 1-sized descriptor pool.
            // The type of this descriptor pool doesn't matter because it is never used.
            mPoolSizes.push_back(VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1});
            mMaxSets = kMaxDescriptorsPerPool;
        } else {
            ASSERT(totalDescriptorCount <= kMaxBindingsPerPipelineLayout);
            static_assert(kMaxBindingsPerPipelineLayout <= kMaxDescriptorsPerPool, "");

            // Compute the total number of descriptors sets that fits given the max.
            mMaxSets = kMaxDescriptorsPerPool / totalDescriptorCount;
            ASSERT(mMaxSets > 0);

            // Grow the number of desciptors in the pool to fit the computed |mMaxSets|.
            for (auto& poolSize : mPoolSizes) {
                poolSize.descriptorCount *= mMaxSets;
            }
        }
    }

    DescriptorSetAllocator::~DescriptorSetAllocator() {
        for (auto& pool : mDescriptorPools) {
            ASSERT(pool.freeSetIndices.size() == mMaxSets);
            if (pool.vkPool != VK_NULL_HANDLE) {
                Device* device = ToBackend(mLayout->GetDevice());
                device->GetFencedDeleter()->DeleteWhenUnused(pool.vkPool);
            }
        }
    }

    ResultOrError<DescriptorSetAllocation> DescriptorSetAllocator::Allocate() {
        if (mAvailableDescriptorPoolIndices.empty()) {
            DAWN_TRY(AllocateDescriptorPool());
        }

        ASSERT(!mAvailableDescriptorPoolIndices.empty());

        const PoolIndex poolIndex = mAvailableDescriptorPoolIndices.back();
        DescriptorPool* pool = &mDescriptorPools[poolIndex];

        ASSERT(!pool->freeSetIndices.empty());

        SetIndex setIndex = pool->freeSetIndices.back();
        pool->freeSetIndices.pop_back();

        if (pool->freeSetIndices.empty()) {
            mAvailableDescriptorPoolIndices.pop_back();
        }

        return DescriptorSetAllocation{pool->sets[setIndex], poolIndex, setIndex};
    }

    void DescriptorSetAllocator::Deallocate(DescriptorSetAllocation* allocationInfo) {
        ASSERT(allocationInfo != nullptr);
        ASSERT(allocationInfo->set != VK_NULL_HANDLE);

        // We can't reuse the descriptor set right away because the Vulkan spec says in the
        // documentation for vkCmdBindDescriptorSets that the set may be consumed any time between
        // host execution of the command and the end of the draw/dispatch.
        Device* device = ToBackend(mLayout->GetDevice());
        const Serial serial = device->GetPendingCommandSerial();
        mPendingDeallocations.Enqueue({allocationInfo->poolIndex, allocationInfo->setIndex},
                                      serial);

        if (mLastDeallocationSerial != serial) {
            device->EnqueueDeferredDeallocation(mLayout);
            mLastDeallocationSerial = serial;
        }

        // Clear the content of allocation so that use after frees are more visible.
        *allocationInfo = {};
    }

    void DescriptorSetAllocator::FinishDeallocation(Serial completedSerial) {
        for (const Deallocation& dealloc : mPendingDeallocations.IterateUpTo(completedSerial)) {
            ASSERT(dealloc.poolIndex < mDescriptorPools.size());

            auto& freeSetIndices = mDescriptorPools[dealloc.poolIndex].freeSetIndices;
            if (freeSetIndices.empty()) {
                mAvailableDescriptorPoolIndices.emplace_back(dealloc.poolIndex);
            }
            freeSetIndices.emplace_back(dealloc.setIndex);
        }
        mPendingDeallocations.ClearUpTo(completedSerial);
    }

    MaybeError DescriptorSetAllocator::AllocateDescriptorPool() {
        VkDescriptorPoolCreateInfo createInfo;
        createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        createInfo.pNext = nullptr;
        createInfo.flags = 0;
        createInfo.maxSets = mMaxSets;
        createInfo.poolSizeCount = static_cast<PoolIndex>(mPoolSizes.size());
        createInfo.pPoolSizes = mPoolSizes.data();

        Device* device = ToBackend(mLayout->GetDevice());

        VkDescriptorPool descriptorPool;
        DAWN_TRY(CheckVkSuccess(device->fn.CreateDescriptorPool(device->GetVkDevice(), &createInfo,
                                                                nullptr, &*descriptorPool),
                                "CreateDescriptorPool"));

        std::vector<VkDescriptorSetLayout> layouts(mMaxSets, mLayout->GetHandle());

        VkDescriptorSetAllocateInfo allocateInfo;
        allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocateInfo.pNext = nullptr;
        allocateInfo.descriptorPool = descriptorPool;
        allocateInfo.descriptorSetCount = mMaxSets;
        allocateInfo.pSetLayouts = AsVkArray(layouts.data());

        std::vector<VkDescriptorSet> sets(mMaxSets);
        MaybeError result =
            CheckVkSuccess(device->fn.AllocateDescriptorSets(device->GetVkDevice(), &allocateInfo,
                                                             AsVkArray(sets.data())),
                           "AllocateDescriptorSets");
        if (result.IsError()) {
            // On an error we can destroy the pool immediately because no command references it.
            device->fn.DestroyDescriptorPool(device->GetVkDevice(), descriptorPool, nullptr);
            DAWN_TRY(std::move(result));
        }

        std::vector<SetIndex> freeSetIndices;
        freeSetIndices.reserve(mMaxSets);

        for (SetIndex i = 0; i < mMaxSets; ++i) {
            freeSetIndices.push_back(i);
        }

        mAvailableDescriptorPoolIndices.push_back(mDescriptorPools.size());
        mDescriptorPools.emplace_back(
            DescriptorPool{descriptorPool, std::move(sets), std::move(freeSetIndices)});

        return {};
    }

}}  // namespace dawn_native::vulkan
