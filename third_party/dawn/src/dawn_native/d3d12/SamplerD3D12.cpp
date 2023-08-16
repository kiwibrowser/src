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

#include "dawn_native/d3d12/SamplerD3D12.h"

#include "dawn_native/d3d12/DeviceD3D12.h"
#include "dawn_native/d3d12/UtilsD3D12.h"

namespace dawn_native { namespace d3d12 {

    namespace {
        D3D12_TEXTURE_ADDRESS_MODE AddressMode(wgpu::AddressMode mode) {
            switch (mode) {
                case wgpu::AddressMode::Repeat:
                    return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
                case wgpu::AddressMode::MirrorRepeat:
                    return D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
                case wgpu::AddressMode::ClampToEdge:
                    return D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
                default:
                    UNREACHABLE();
            }
        }
    }  // namespace

    Sampler::Sampler(Device* device, const SamplerDescriptor* descriptor)
        : SamplerBase(device, descriptor) {
        D3D12_FILTER_TYPE minFilter;
        switch (descriptor->minFilter) {
            case wgpu::FilterMode::Nearest:
                minFilter = D3D12_FILTER_TYPE_POINT;
                break;
            case wgpu::FilterMode::Linear:
                minFilter = D3D12_FILTER_TYPE_LINEAR;
                break;
            default:
                UNREACHABLE();
                break;
        }

        D3D12_FILTER_TYPE magFilter;
        switch (descriptor->magFilter) {
            case wgpu::FilterMode::Nearest:
                magFilter = D3D12_FILTER_TYPE_POINT;
                break;
            case wgpu::FilterMode::Linear:
                magFilter = D3D12_FILTER_TYPE_LINEAR;
                break;
            default:
                UNREACHABLE();
                break;
        }

        D3D12_FILTER_TYPE mipmapFilter;
        switch (descriptor->mipmapFilter) {
            case wgpu::FilterMode::Nearest:
                mipmapFilter = D3D12_FILTER_TYPE_POINT;
                break;
            case wgpu::FilterMode::Linear:
                mipmapFilter = D3D12_FILTER_TYPE_LINEAR;
                break;
            default:
                UNREACHABLE();
                break;
        }

        D3D12_FILTER_REDUCTION_TYPE reduction =
            descriptor->compare == wgpu::CompareFunction::Undefined
                ? D3D12_FILTER_REDUCTION_TYPE_STANDARD
                : D3D12_FILTER_REDUCTION_TYPE_COMPARISON;

        mSamplerDesc.Filter =
            D3D12_ENCODE_BASIC_FILTER(minFilter, magFilter, mipmapFilter, reduction);
        mSamplerDesc.AddressU = AddressMode(descriptor->addressModeU);
        mSamplerDesc.AddressV = AddressMode(descriptor->addressModeV);
        mSamplerDesc.AddressW = AddressMode(descriptor->addressModeW);
        mSamplerDesc.MipLODBias = 0.f;
        mSamplerDesc.MaxAnisotropy = 1;
        if (descriptor->compare != wgpu::CompareFunction::Undefined) {
            mSamplerDesc.ComparisonFunc = ToD3D12ComparisonFunc(descriptor->compare);
        } else {
            // Still set the function so it's not garbage.
            mSamplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
        }
        mSamplerDesc.MinLOD = descriptor->lodMinClamp;
        mSamplerDesc.MaxLOD = descriptor->lodMaxClamp;
    }

    const D3D12_SAMPLER_DESC& Sampler::GetSamplerDescriptor() const {
        return mSamplerDesc;
    }

}}  // namespace dawn_native::d3d12
