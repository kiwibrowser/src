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
#include "dawn_native/d3d12/BindGroupD3D12.h"
#include "dawn_native/d3d12/DeviceD3D12.h"
#include "dawn_native/d3d12/SamplerHeapCacheD3D12.h"
#include "dawn_native/d3d12/StagingDescriptorAllocatorD3D12.h"

namespace dawn_native { namespace d3d12 {
    namespace {
        BindGroupLayout::DescriptorType WGPUBindingTypeToDescriptorType(
            wgpu::BindingType bindingType) {
            switch (bindingType) {
                case wgpu::BindingType::UniformBuffer:
                    return BindGroupLayout::DescriptorType::CBV;
                case wgpu::BindingType::StorageBuffer:
                case wgpu::BindingType::WriteonlyStorageTexture:
                    return BindGroupLayout::DescriptorType::UAV;
                case wgpu::BindingType::SampledTexture:
                case wgpu::BindingType::ReadonlyStorageBuffer:
                case wgpu::BindingType::ReadonlyStorageTexture:
                    return BindGroupLayout::DescriptorType::SRV;
                case wgpu::BindingType::Sampler:
                case wgpu::BindingType::ComparisonSampler:
                    return BindGroupLayout::DescriptorType::Sampler;
                case wgpu::BindingType::StorageTexture:
                    UNREACHABLE();
                    return BindGroupLayout::DescriptorType::UAV;
            }
        }
    }  // anonymous namespace

    BindGroupLayout::BindGroupLayout(Device* device, const BindGroupLayoutDescriptor* descriptor)
        : BindGroupLayoutBase(device, descriptor),
          mBindingOffsets(GetBindingCount()),
          mDescriptorCounts{},
          mBindGroupAllocator(MakeFrontendBindGroupAllocator<BindGroup>(4096)) {
        for (BindingIndex bindingIndex = GetDynamicBufferCount(); bindingIndex < GetBindingCount();
             ++bindingIndex) {
            const BindingInfo& bindingInfo = GetBindingInfo(bindingIndex);

            // For dynamic resources, Dawn uses root descriptor in D3D12 backend.
            // So there is no need to allocate the descriptor from descriptor heap.
            // This loop starts after the dynamic buffer indices to skip counting
            // dynamic resources in calculating the size of the descriptor heap.
            ASSERT(!bindingInfo.hasDynamicOffset);

            DescriptorType descriptorType = WGPUBindingTypeToDescriptorType(bindingInfo.type);
            mBindingOffsets[bindingIndex] = mDescriptorCounts[descriptorType]++;
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

        for (BindingIndex bindingIndex{0}; bindingIndex < GetBindingCount(); ++bindingIndex) {
            const BindingInfo& bindingInfo = GetBindingInfo(bindingIndex);

            if (bindingInfo.hasDynamicOffset) {
                // Dawn is using values in mBindingOffsets to decide register number in HLSL.
                // Root descriptor needs to set this value to set correct register number in
                // generated HLSL shader.
                switch (bindingInfo.type) {
                    case wgpu::BindingType::UniformBuffer:
                    case wgpu::BindingType::StorageBuffer:
                    case wgpu::BindingType::ReadonlyStorageBuffer:
                        mBindingOffsets[bindingIndex] = baseRegister++;
                        break;
                    case wgpu::BindingType::SampledTexture:
                    case wgpu::BindingType::Sampler:
                    case wgpu::BindingType::ComparisonSampler:
                    case wgpu::BindingType::StorageTexture:
                    case wgpu::BindingType::ReadonlyStorageTexture:
                    case wgpu::BindingType::WriteonlyStorageTexture:
                        UNREACHABLE();
                        break;
                }
                continue;
            }

            // TODO(shaobo.yan@intel.com): Implement dynamic buffer offset.
            DescriptorType descriptorType = WGPUBindingTypeToDescriptorType(bindingInfo.type);
            mBindingOffsets[bindingIndex] += descriptorOffsets[descriptorType];
        }

        mViewAllocator = device->GetViewStagingDescriptorAllocator(GetCbvUavSrvDescriptorCount());
        mSamplerAllocator =
            device->GetSamplerStagingDescriptorAllocator(GetSamplerDescriptorCount());
    }

    ResultOrError<BindGroup*> BindGroupLayout::AllocateBindGroup(
        Device* device,
        const BindGroupDescriptor* descriptor) {
        uint32_t viewSizeIncrement = 0;
        CPUDescriptorHeapAllocation viewAllocation;
        if (GetCbvUavSrvDescriptorCount() > 0) {
            DAWN_TRY_ASSIGN(viewAllocation, mViewAllocator->AllocateCPUDescriptors());
            viewSizeIncrement = mViewAllocator->GetSizeIncrement();
        }

        Ref<BindGroup> bindGroup = AcquireRef<BindGroup>(
            mBindGroupAllocator.Allocate(device, descriptor, viewSizeIncrement, viewAllocation));

        if (GetSamplerDescriptorCount() > 0) {
            Ref<SamplerHeapCacheEntry> samplerHeapCacheEntry;
            DAWN_TRY_ASSIGN(samplerHeapCacheEntry, device->GetSamplerHeapCache()->GetOrCreate(
                                                       bindGroup.Get(), mSamplerAllocator));
            bindGroup->SetSamplerAllocationEntry(std::move(samplerHeapCacheEntry));
        }

        return bindGroup.Detach();
    }

    void BindGroupLayout::DeallocateBindGroup(BindGroup* bindGroup,
                                              CPUDescriptorHeapAllocation* viewAllocation) {
        if (viewAllocation->IsValid()) {
            mViewAllocator->Deallocate(viewAllocation);
        }

        mBindGroupAllocator.Deallocate(bindGroup);
    }

    ityp::span<BindingIndex, const uint32_t> BindGroupLayout::GetBindingOffsets() const {
        return {mBindingOffsets.data(), mBindingOffsets.size()};
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
