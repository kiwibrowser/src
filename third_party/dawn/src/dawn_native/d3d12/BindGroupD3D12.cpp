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

#include "dawn_native/d3d12/BindGroupD3D12.h"
#include "common/BitSetIterator.h"
#include "dawn_native/d3d12/BindGroupLayoutD3D12.h"
#include "dawn_native/d3d12/BufferD3D12.h"
#include "dawn_native/d3d12/SamplerD3D12.h"
#include "dawn_native/d3d12/TextureD3D12.h"

#include "dawn_native/d3d12/DeviceD3D12.h"

namespace dawn_native { namespace d3d12 {

    BindGroup::BindGroup(Device* device, const BindGroupDescriptor* descriptor)
        : BindGroupBase(device, descriptor) {
    }

    void BindGroup::AllocateDescriptors(const DescriptorHeapHandle& cbvUavSrvHeapStart,
                                        uint32_t* cbvUavSrvHeapOffset,
                                        const DescriptorHeapHandle& samplerHeapStart,
                                        uint32_t* samplerHeapOffset) {
        const auto* bgl = ToBackend(GetLayout());
        const auto& layout = bgl->GetBindingInfo();

        // Save the offset to the start of the descriptor table in the heap
        mCbvUavSrvHeapOffset = *cbvUavSrvHeapOffset;
        mSamplerHeapOffset = *samplerHeapOffset;

        const auto& bindingOffsets = bgl->GetBindingOffsets();

        auto d3d12Device = ToBackend(GetDevice())->GetD3D12Device();
        for (uint32_t bindingIndex : IterateBitSet(layout.mask)) {
            switch (layout.types[bindingIndex]) {
                case dawn::BindingType::UniformBuffer: {
                    BufferBinding binding = GetBindingAsBufferBinding(bindingIndex);

                    D3D12_CONSTANT_BUFFER_VIEW_DESC desc;
                    // TODO(enga@google.com): investigate if this needs to be a constraint at the
                    // API level
                    desc.SizeInBytes = Align(binding.size, 256);
                    desc.BufferLocation = ToBackend(binding.buffer)->GetVA() + binding.offset;

                    d3d12Device->CreateConstantBufferView(
                        &desc, cbvUavSrvHeapStart.GetCPUHandle(*cbvUavSrvHeapOffset +
                                                               bindingOffsets[bindingIndex]));
                } break;
                case dawn::BindingType::StorageBuffer: {
                    BufferBinding binding = GetBindingAsBufferBinding(bindingIndex);

                    D3D12_UNORDERED_ACCESS_VIEW_DESC desc;
                    // TODO(enga@google.com): investigate if this needs to be a constraint at the
                    // API level
                    desc.Buffer.NumElements = Align(binding.size, 256);
                    desc.Format = DXGI_FORMAT_UNKNOWN;
                    desc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
                    desc.Buffer.FirstElement = binding.offset;
                    desc.Buffer.StructureByteStride = 1;
                    desc.Buffer.CounterOffsetInBytes = 0;
                    desc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;

                    d3d12Device->CreateUnorderedAccessView(
                        ToBackend(binding.buffer)->GetD3D12Resource().Get(), nullptr, &desc,
                        cbvUavSrvHeapStart.GetCPUHandle(*cbvUavSrvHeapOffset +
                                                        bindingOffsets[bindingIndex]));
                } break;
                case dawn::BindingType::SampledTexture: {
                    auto* view = ToBackend(GetBindingAsTextureView(bindingIndex));
                    auto& srv = view->GetSRVDescriptor();
                    d3d12Device->CreateShaderResourceView(
                        ToBackend(view->GetTexture())->GetD3D12Resource(), &srv,
                        cbvUavSrvHeapStart.GetCPUHandle(*cbvUavSrvHeapOffset +
                                                        bindingOffsets[bindingIndex]));
                } break;
                case dawn::BindingType::Sampler: {
                    auto* sampler = ToBackend(GetBindingAsSampler(bindingIndex));
                    auto& samplerDesc = sampler->GetSamplerDescriptor();
                    d3d12Device->CreateSampler(
                        &samplerDesc, samplerHeapStart.GetCPUHandle(*samplerHeapOffset +
                                                                    bindingOffsets[bindingIndex]));
                } break;

                // TODO(shaobo.yan@intel.com): Implement dynamic buffer offset.
                case dawn::BindingType::DynamicUniformBuffer:
                case dawn::BindingType::DynamicStorageBuffer:
                    UNREACHABLE();
                    break;
            }
        }

        // Offset by the number of descriptors created
        *cbvUavSrvHeapOffset += bgl->GetCbvUavSrvDescriptorCount();
        *samplerHeapOffset += bgl->GetSamplerDescriptorCount();
    }

    uint32_t BindGroup::GetCbvUavSrvHeapOffset() const {
        return mCbvUavSrvHeapOffset;
    }

    uint32_t BindGroup::GetSamplerHeapOffset() const {
        return mSamplerHeapOffset;
    }

    bool BindGroup::TestAndSetCounted(uint64_t heapSerial, uint32_t indexInSubmit) {
        bool isCounted = (mHeapSerial == heapSerial && mIndexInSubmit == indexInSubmit);
        mHeapSerial = heapSerial;
        mIndexInSubmit = indexInSubmit;
        return isCounted;
    }

}}  // namespace dawn_native::d3d12
