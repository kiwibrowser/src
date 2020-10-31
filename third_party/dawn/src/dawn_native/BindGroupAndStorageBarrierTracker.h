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

#ifndef DAWNNATIVE_BINDGROUPANDSTORAGEBARRIERTRACKER_H_
#define DAWNNATIVE_BINDGROUPANDSTORAGEBARRIERTRACKER_H_

#include "common/ityp_bitset.h"
#include "common/ityp_stack_vec.h"
#include "dawn_native/BindGroup.h"
#include "dawn_native/BindGroupTracker.h"
#include "dawn_native/Buffer.h"
#include "dawn_native/Texture.h"

namespace dawn_native {

    // Extends BindGroupTrackerBase to also keep track of resources that need a usage transition.
    template <bool CanInheritBindGroups, typename DynamicOffset>
    class BindGroupAndStorageBarrierTrackerBase
        : public BindGroupTrackerBase<CanInheritBindGroups, DynamicOffset> {
        using Base = BindGroupTrackerBase<CanInheritBindGroups, DynamicOffset>;

      public:
        BindGroupAndStorageBarrierTrackerBase() = default;

        void OnSetBindGroup(BindGroupIndex index,
                            BindGroupBase* bindGroup,
                            uint32_t dynamicOffsetCount,
                            uint32_t* dynamicOffsets) {
            ASSERT(index < kMaxBindGroupsTyped);

            if (this->mBindGroups[index] != bindGroup) {
                const BindGroupLayoutBase* layout = bindGroup->GetLayout();

                mBindings[index].resize(layout->GetBindingCount());
                mBindingTypes[index].resize(layout->GetBindingCount());
                mBindingsNeedingBarrier[index] = {};

                for (BindingIndex bindingIndex{0}; bindingIndex < layout->GetBindingCount();
                     ++bindingIndex) {
                    const BindingInfo& bindingInfo = layout->GetBindingInfo(bindingIndex);

                    if ((bindingInfo.visibility & wgpu::ShaderStage::Compute) == 0) {
                        continue;
                    }

                    mBindingTypes[index][bindingIndex] = bindingInfo.type;
                    switch (bindingInfo.type) {
                        case wgpu::BindingType::UniformBuffer:
                        case wgpu::BindingType::ReadonlyStorageBuffer:
                        case wgpu::BindingType::Sampler:
                        case wgpu::BindingType::ComparisonSampler:
                        case wgpu::BindingType::SampledTexture:
                            // Don't require barriers.
                            break;

                        case wgpu::BindingType::StorageBuffer:
                            mBindingsNeedingBarrier[index].set(bindingIndex);
                            mBindings[index][bindingIndex] = static_cast<ObjectBase*>(
                                bindGroup->GetBindingAsBufferBinding(bindingIndex).buffer);
                            break;

                        // Read-only and write-only storage textures must use general layout
                        // because load and store operations on storage images can only be done on
                        // the images in VK_IMAGE_LAYOUT_GENERAL layout.
                        case wgpu::BindingType::ReadonlyStorageTexture:
                        case wgpu::BindingType::WriteonlyStorageTexture:
                            mBindingsNeedingBarrier[index].set(bindingIndex);
                            mBindings[index][bindingIndex] = static_cast<ObjectBase*>(
                                bindGroup->GetBindingAsTextureView(bindingIndex));
                            break;

                        case wgpu::BindingType::StorageTexture:
                            // Not implemented.
                        default:
                            UNREACHABLE();
                            break;
                    }
                }
            }

            Base::OnSetBindGroup(index, bindGroup, dynamicOffsetCount, dynamicOffsets);
        }

      protected:
        ityp::array<BindGroupIndex,
                    ityp::bitset<BindingIndex, kMaxBindingsPerPipelineLayout>,
                    kMaxBindGroups>
            mBindingsNeedingBarrier = {};
        ityp::array<BindGroupIndex,
                    ityp::stack_vec<BindingIndex, wgpu::BindingType, kMaxOptimalBindingsPerGroup>,
                    kMaxBindGroups>
            mBindingTypes = {};
        ityp::array<BindGroupIndex,
                    ityp::stack_vec<BindingIndex, ObjectBase*, kMaxOptimalBindingsPerGroup>,
                    kMaxBindGroups>
            mBindings = {};
    };

}  // namespace dawn_native

#endif  // DAWNNATIVE_BINDGROUPANDSTORAGEBARRIERTRACKER_H_
