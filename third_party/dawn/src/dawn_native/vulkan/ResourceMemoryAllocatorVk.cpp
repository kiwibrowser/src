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

#include "dawn_native/vulkan/ResourceMemoryAllocatorVk.h"

#include "common/Math.h"
#include "dawn_native/BuddyMemoryAllocator.h"
#include "dawn_native/ResourceHeapAllocator.h"
#include "dawn_native/vulkan/DeviceVk.h"
#include "dawn_native/vulkan/FencedDeleter.h"
#include "dawn_native/vulkan/ResourceHeapVk.h"
#include "dawn_native/vulkan/VulkanError.h"

namespace dawn_native { namespace vulkan {

    namespace {

        // TODO(cwallez@chromium.org): This is a hardcoded heurstic to choose when to
        // suballocate but it should ideally depend on the size of the memory heaps and other
        // factors.
        constexpr uint64_t kMaxSizeForSubAllocation = 4ull * 1024ull * 1024ull;  // 4MiB

        // Have each bucket of the buddy system allocate at least some resource of the maximum
        // size
        constexpr uint64_t kBuddyHeapsSize = 2 * kMaxSizeForSubAllocation;

    }  // anonymous namespace

    // SingleTypeAllocator is a combination of a BuddyMemoryAllocator and its client and can
    // service suballocation requests, but for a single Vulkan memory type.

    class ResourceMemoryAllocator::SingleTypeAllocator : public ResourceHeapAllocator {
      public:
        SingleTypeAllocator(Device* device, size_t memoryTypeIndex, VkDeviceSize memoryHeapSize)
            : mDevice(device),
              mMemoryTypeIndex(memoryTypeIndex),
              mMemoryHeapSize(memoryHeapSize),
              mBuddySystem(
                  // Round down to a power of 2 that's <= mMemoryHeapSize. This will always
                  // be a multiple of kBuddyHeapsSize because kBuddyHeapsSize is a power of 2.
                  uint64_t(1) << Log2(mMemoryHeapSize),
                  // Take the min in the very unlikely case the memory heap is tiny.
                  std::min(uint64_t(1) << Log2(mMemoryHeapSize), kBuddyHeapsSize),
                  this) {
            ASSERT(IsPowerOfTwo(kBuddyHeapsSize));
        }
        ~SingleTypeAllocator() override = default;

        ResultOrError<ResourceMemoryAllocation> AllocateMemory(
            const VkMemoryRequirements& requirements) {
            return mBuddySystem.Allocate(requirements.size, requirements.alignment);
        }

        void DeallocateMemory(const ResourceMemoryAllocation& allocation) {
            mBuddySystem.Deallocate(allocation);
        }

        // Implementation of the MemoryAllocator interface to be a client of BuddyMemoryAllocator

        ResultOrError<std::unique_ptr<ResourceHeapBase>> AllocateResourceHeap(
            uint64_t size) override {
            if (size > mMemoryHeapSize) {
                return DAWN_OUT_OF_MEMORY_ERROR("Allocation size too large");
            }

            VkMemoryAllocateInfo allocateInfo;
            allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            allocateInfo.pNext = nullptr;
            allocateInfo.allocationSize = size;
            allocateInfo.memoryTypeIndex = mMemoryTypeIndex;

            VkDeviceMemory allocatedMemory = VK_NULL_HANDLE;

            // First check OOM that we want to surface to the application.
            DAWN_TRY(CheckVkOOMThenSuccess(
                mDevice->fn.AllocateMemory(mDevice->GetVkDevice(), &allocateInfo, nullptr,
                                           &*allocatedMemory),
                "vkAllocateMemory"));

            ASSERT(allocatedMemory != VK_NULL_HANDLE);
            return {std::make_unique<ResourceHeap>(allocatedMemory, mMemoryTypeIndex)};
        }

        void DeallocateResourceHeap(std::unique_ptr<ResourceHeapBase> allocation) override {
            mDevice->GetFencedDeleter()->DeleteWhenUnused(ToBackend(allocation.get())->GetMemory());
        }

      private:
        Device* mDevice;
        size_t mMemoryTypeIndex;
        VkDeviceSize mMemoryHeapSize;
        BuddyMemoryAllocator mBuddySystem;
    };

    // Implementation of ResourceMemoryAllocator

    ResourceMemoryAllocator::ResourceMemoryAllocator(Device* device) : mDevice(device) {
        const VulkanDeviceInfo& info = mDevice->GetDeviceInfo();
        mAllocatorsPerType.reserve(info.memoryTypes.size());

        for (size_t i = 0; i < info.memoryTypes.size(); i++) {
            mAllocatorsPerType.emplace_back(std::make_unique<SingleTypeAllocator>(
                mDevice, i, info.memoryHeaps[info.memoryTypes[i].heapIndex].size));
        }
    }

    ResourceMemoryAllocator::~ResourceMemoryAllocator() = default;

    ResultOrError<ResourceMemoryAllocation> ResourceMemoryAllocator::Allocate(
        const VkMemoryRequirements& requirements,
        bool mappable) {
        // The Vulkan spec guarantees at least on memory type is valid.
        int memoryType = FindBestTypeIndex(requirements, mappable);
        ASSERT(memoryType >= 0);

        VkDeviceSize size = requirements.size;

        // Sub-allocate non-mappable resources because at the moment the mapped pointer
        // is part of the resource and not the heap, which doesn't match the Vulkan model.
        // TODO(cwallez@chromium.org): allow sub-allocating mappable resources, maybe.
        if (requirements.size < kMaxSizeForSubAllocation && !mappable) {
            ResourceMemoryAllocation subAllocation;
            DAWN_TRY_ASSIGN(subAllocation,
                            mAllocatorsPerType[memoryType]->AllocateMemory(requirements));
            if (subAllocation.GetInfo().mMethod != AllocationMethod::kInvalid) {
                return std::move(subAllocation);
            }
        }

        // If sub-allocation failed, allocate memory just for it.
        std::unique_ptr<ResourceHeapBase> resourceHeap;
        DAWN_TRY_ASSIGN(resourceHeap, mAllocatorsPerType[memoryType]->AllocateResourceHeap(size));

        void* mappedPointer = nullptr;
        if (mappable) {
            DAWN_TRY(
                CheckVkSuccess(mDevice->fn.MapMemory(mDevice->GetVkDevice(),
                                                     ToBackend(resourceHeap.get())->GetMemory(), 0,
                                                     size, 0, &mappedPointer),
                               "vkMapMemory"));
        }

        AllocationInfo info;
        info.mMethod = AllocationMethod::kDirect;
        return ResourceMemoryAllocation(info, /*offset*/ 0, resourceHeap.release(),
                                        static_cast<uint8_t*>(mappedPointer));
    }

    void ResourceMemoryAllocator::Deallocate(ResourceMemoryAllocation* allocation) {
        switch (allocation->GetInfo().mMethod) {
            // Some memory allocation can never be initialized, for example when wrapping
            // swapchain VkImages with a Texture.
            case AllocationMethod::kInvalid:
                break;

            // For direct allocation we can put the memory for deletion immediately and the fence
            // deleter will make sure the resources are freed before the memory.
            case AllocationMethod::kDirect: {
                ResourceHeap* heap = ToBackend(allocation->GetResourceHeap());
                allocation->Invalidate();
                mDevice->GetFencedDeleter()->DeleteWhenUnused(heap->GetMemory());
                delete heap;
                break;
            }

            // Suballocations aren't freed immediately, otherwise another resource allocation could
            // happen just after that aliases the old one and would require a barrier.
            // TODO(cwallez@chromium.org): Maybe we can produce the correct barriers to reduce the
            // latency to reclaim memory.
            case AllocationMethod::kSubAllocated:
                mSubAllocationsToDelete.Enqueue(*allocation, mDevice->GetPendingCommandSerial());
                break;

            default:
                UNREACHABLE();
                break;
        }

        // Invalidate the underlying resource heap in case the client accidentally
        // calls DeallocateMemory again using the same allocation.
        allocation->Invalidate();
    }

    void ResourceMemoryAllocator::Tick(Serial completedSerial) {
        for (const ResourceMemoryAllocation& allocation :
             mSubAllocationsToDelete.IterateUpTo(completedSerial)) {
            ASSERT(allocation.GetInfo().mMethod == AllocationMethod::kSubAllocated);
            size_t memoryType = ToBackend(allocation.GetResourceHeap())->GetMemoryType();

            mAllocatorsPerType[memoryType]->DeallocateMemory(allocation);
        }

        mSubAllocationsToDelete.ClearUpTo(completedSerial);
    }

    int ResourceMemoryAllocator::FindBestTypeIndex(VkMemoryRequirements requirements,
                                                   bool mappable) {
        const VulkanDeviceInfo& info = mDevice->GetDeviceInfo();

        // Find a suitable memory type for this allocation
        int bestType = -1;
        for (size_t i = 0; i < info.memoryTypes.size(); ++i) {
            // Resource must support this memory type
            if ((requirements.memoryTypeBits & (1 << i)) == 0) {
                continue;
            }

            // Mappable resource must be host visible
            if (mappable &&
                (info.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) == 0) {
                continue;
            }

            // Mappable must also be host coherent.
            if (mappable &&
                (info.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0) {
                continue;
            }

            // Found the first candidate memory type
            if (bestType == -1) {
                bestType = static_cast<int>(i);
                continue;
            }

            // For non-mappable resources, favor device local memory.
            if (!mappable) {
                if ((info.memoryTypes[bestType].propertyFlags &
                     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) == 0 &&
                    (info.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) !=
                        0) {
                    bestType = static_cast<int>(i);
                    continue;
                }
            }

            // All things equal favor the memory in the biggest heap
            VkDeviceSize bestTypeHeapSize =
                info.memoryHeaps[info.memoryTypes[bestType].heapIndex].size;
            VkDeviceSize candidateHeapSize = info.memoryHeaps[info.memoryTypes[i].heapIndex].size;
            if (candidateHeapSize > bestTypeHeapSize) {
                bestType = static_cast<int>(i);
                continue;
            }
        }

        return bestType;
    }

}}  // namespace dawn_native::vulkan
