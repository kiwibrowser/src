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

#ifndef DAWNNATIVE_VULKAN_UTILSVULKAN_H_
#define DAWNNATIVE_VULKAN_UTILSVULKAN_H_

#include "common/vulkan_platform.h"
#include "dawn_native/Commands.h"
#include "dawn_native/dawn_platform.h"

namespace dawn_native { namespace vulkan {

    // A Helper type used to build a pNext chain of extension structs.
    // Usage is:
    //   1) Create instance, passing the address of the first struct in the
    //      chain. This will parse the existing |pNext| chain in it to find
    //      its tail.
    //
    //   2) Call Add(&vk_struct) every time a new struct needs to be appended
    //      to the chain.
    //
    //   3) Alternatively, call Add(&vk_struct, VK_STRUCTURE_TYPE_XXX) to
    //      initialize the struct with a given VkStructureType value while
    //      appending it to the chain.
    //
    // Examples:
    //     VkPhysicalFeatures2 features2 = {
    //       .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
    //       .pNext = nullptr,
    //     };
    //
    //     PNextChainBuilder featuresChain(&features2);
    //
    //     featuresChain.Add(&featuresExtensions.subgroupSizeControl,
    //                       VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_SIZE_CONTROL_FEATURES_EXT);
    //
    struct PNextChainBuilder {
        // Constructor takes the address of a Vulkan structure instance, and
        // walks its pNext chain to record the current location of its tail.
        //
        // NOTE: Some VK_STRUCT_TYPEs define their pNext field as a const void*
        // which is why the VkBaseOutStructure* casts below are necessary.
        template <typename VK_STRUCT_TYPE>
        explicit PNextChainBuilder(VK_STRUCT_TYPE* head)
            : mCurrent(reinterpret_cast<VkBaseOutStructure*>(head)) {
            // Find the end of the current chain.
            while (mCurrent->pNext != nullptr) {
                mCurrent = mCurrent->pNext;
            }
        }

        // Add one item to the chain. |vk_struct| must be a Vulkan structure
        // that is already initialized.
        template <typename VK_STRUCT_TYPE>
        void Add(VK_STRUCT_TYPE* vkStruct) {
            // Sanity checks to ensure proper type safety.
            static_assert(
                offsetof(VK_STRUCT_TYPE, sType) == offsetof(VkBaseOutStructure, sType) &&
                    offsetof(VK_STRUCT_TYPE, pNext) == offsetof(VkBaseOutStructure, pNext),
                "Argument type is not a proper Vulkan structure type");
            vkStruct->pNext = nullptr;

            mCurrent->pNext = reinterpret_cast<VkBaseOutStructure*>(vkStruct);
            mCurrent = mCurrent->pNext;
        }

        // A variant of Add() above that also initializes the |sType| field in |vk_struct|.
        template <typename VK_STRUCT_TYPE>
        void Add(VK_STRUCT_TYPE* vkStruct, VkStructureType sType) {
            vkStruct->sType = sType;
            Add(vkStruct);
        }

      private:
        VkBaseOutStructure* mCurrent;
    };

    VkCompareOp ToVulkanCompareOp(wgpu::CompareFunction op);

    Extent3D ComputeTextureCopyExtent(const TextureCopy& textureCopy, const Extent3D& copySize);

    VkBufferImageCopy ComputeBufferImageCopyRegion(const BufferCopy& bufferCopy,
                                                   const TextureCopy& textureCopy,
                                                   const Extent3D& copySize);
    VkBufferImageCopy ComputeBufferImageCopyRegion(const TextureDataLayout& dataLayout,
                                                   const TextureCopy& textureCopy,
                                                   const Extent3D& copySize);

}}  // namespace dawn_native::vulkan

#endif  // DAWNNATIVE_VULKAN_UTILSVULKAN_H_