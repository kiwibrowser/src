// Copyright 2017 The Dawn Authors
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

#include "dawn_native/d3d12/BindGroupLayoutD3D12.h"

#include "common/BitSetIterator.h"
#include "dawn_native/d3d12/DeviceD3D12.h"

namespace dawn_native { namespace d3d12 {

    BindGroupLayout::BindGroupLayout(Device* device, const BindGroupLayoutDescriptor* descriptor)
        : BindGroupLayoutBase(device, descriptor), mDescriptorCounts{} {
        const auto& groupInfo = GetBindingInfo();

        for (uint32_t binding : IterateBitSet(groupInfo.mask)) {
            switch (groupInfo.types[binding]) {
                case dawn::BindingType::UniformBuffer:
                    mBindingOffsets[binding] = mDescriptorCounts[CBV]++;
                    break;
                case dawn::BindingType::StorageBuffer:
                    mBindingOffsets[binding] = mDescriptorCounts[UAV]++;
                    break;
                case dawn::BindingType::SampledTexture:
                    mBindingOffsets[binding] = mDescriptorCounts[SRV]++;
                    break;
                case dawn::BindingType::Sampler:
                    mBindingOffsets[binding] = mDescriptorCounts[Sampler]++;
                    break;
                // TODO(shaobo.yan@intel.com): Implement dynamic buffer offset.
                case dawn::BindingType::DynamicUniformBuffer:
                case dawn::BindingType::DynamicStorageBuffer:
                    UNREACHABLE();
                    break;
            }
        }

        auto SetDescriptorRange = [&](uint32_t index, uint32_t count, uint32_t* baseRegister,
                                      D3D12_DESCRIPTOR_RANGE_TYPE type) -> bool {
            if (count == 0) {
                return false;
            }

            auto& range = mRanges[index];
            range.RangeType = type;
            range.NumDescriptors = count;
            range.RegisterSpace = 0;
            range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
            range.BaseShaderRegister = *baseRegister;
            *baseRegister += count;
            // These ranges will be copied and range.BaseShaderRegister will be set in
            // d3d12::PipelineLayout to account for bind group register offsets
            return true;
        };

        uint32_t rangeIndex = 0;
        uint32_t baseRegister = 0;

        std::array<uint32_t, DescriptorType::Count> descriptorOffsets;
        // Ranges 0-2 contain the CBV, UAV, and SRV ranges, if they exist, tightly packed
        // Range 3 contains the Sampler range, if there is one
        if (SetDescriptorRange(rangeIndex, mDescriptorCounts[CBV], &baseRegister,
                               D3D12_DESCRIPTOR_RANGE_TYPE_CBV)) {
            descriptorOffsets[CBV] = mRanges[rangeIndex++].BaseShaderRegister;
        }
        if (SetDescriptorRange(rangeIndex, mDescriptorCounts[UAV], &baseRegister,
                               D3D12_DESCRIPTOR_RANGE_TYPE_UAV)) {
            descriptorOffsets[UAV] = mRanges[rangeIndex++].BaseShaderRegister;
        }
        if (SetDescriptorRange(rangeIndex, mDescriptorCounts[SRV], &baseRegister,
                               D3D12_DESCRIPTOR_RANGE_TYPE_SRV)) {
            descriptorOffsets[SRV] = mRanges[rangeIndex++].BaseShaderRegister;
        }
        uint32_t zero = 0;
        SetDescriptorRange(Sampler, mDescriptorCounts[Sampler], &zero,
                           D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER);
        descriptorOffsets[Sampler] = 0;

        for (uint32_t binding : IterateBitSet(groupInfo.mask)) {
            switch (groupInfo.types[binding]) {
                case dawn::BindingType::UniformBuffer:
                    mBindingOffsets[binding] += descriptorOffsets[CBV];
                    break;
                case dawn::BindingType::StorageBuffer:
                    mBindingOffsets[binding] += descriptorOffsets[UAV];
                    break;
                case dawn::BindingType::SampledTexture:
                    mBindingOffsets[binding] += descriptorOffsets[SRV];
                    break;
                case dawn::BindingType::Sampler:
                    mBindingOffsets[binding] += descriptorOffsets[Sampler];
                    break;
                // TODO(shaobo.yan@intel.com): Implement dynamic buffer offset.
                case dawn::BindingType::DynamicUniformBuffer:
                case dawn::BindingType::DynamicStorageBuffer:
                    UNREACHABLE();
                    break;
            }
        }
    }

    const std::array<uint32_t, kMaxBindingsPerGroup>& BindGroupLayout::GetBindingOffsets() const {
        return mBindingOffsets;
    }

    uint32_t BindGroupLayout::GetCbvUavSrvDescriptorTableSize() const {
        return (static_cast<uint32_t>(mDescriptorCounts[CBV] > 0) +
                static_cast<uint32_t>(mDescriptorCounts[UAV] > 0) +
                static_cast<uint32_t>(mDescriptorCounts[SRV] > 0));
    }

    uint32_t BindGroupLayout::GetSamplerDescriptorTableSize() const {
        return mDescriptorCounts[Sampler] > 0;
    }

    uint32_t BindGroupLayout::GetCbvUavSrvDescriptorCount() const {
        return mDescriptorCounts[CBV] + mDescriptorCounts[UAV] + mDescriptorCounts[SRV];
    }

    uint32_t BindGroupLayout::GetSamplerDescriptorCount() const {
        return mDescriptorCounts[Sampler];
    }

    const D3D12_DESCRIPTOR_RANGE* BindGroupLayout::GetCbvUavSrvDescriptorRanges() const {
        return mRanges;
    }

    const D3D12_DESCRIPTOR_RANGE* BindGroupLayout::GetSamplerDescriptorRanges() const {
        return &mRanges[Sampler];
    }

}}  // namespace dawn_native::d3d12
