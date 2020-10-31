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

#include "dawn_native/metal/SamplerMTL.h"

#include "dawn_native/metal/DeviceMTL.h"
#include "dawn_native/metal/UtilsMetal.h"

namespace dawn_native { namespace metal {

    namespace {
        MTLSamplerMinMagFilter FilterModeToMinMagFilter(wgpu::FilterMode mode) {
            switch (mode) {
                case wgpu::FilterMode::Nearest:
                    return MTLSamplerMinMagFilterNearest;
                case wgpu::FilterMode::Linear:
                    return MTLSamplerMinMagFilterLinear;
            }
        }

        MTLSamplerMipFilter FilterModeToMipFilter(wgpu::FilterMode mode) {
            switch (mode) {
                case wgpu::FilterMode::Nearest:
                    return MTLSamplerMipFilterNearest;
                case wgpu::FilterMode::Linear:
                    return MTLSamplerMipFilterLinear;
            }
        }

        MTLSamplerAddressMode AddressMode(wgpu::AddressMode mode) {
            switch (mode) {
                case wgpu::AddressMode::Repeat:
                    return MTLSamplerAddressModeRepeat;
                case wgpu::AddressMode::MirrorRepeat:
                    return MTLSamplerAddressModeMirrorRepeat;
                case wgpu::AddressMode::ClampToEdge:
                    return MTLSamplerAddressModeClampToEdge;
            }
        }
    }

    // static
    ResultOrError<Sampler*> Sampler::Create(Device* device, const SamplerDescriptor* descriptor) {
        if (descriptor->compare != wgpu::CompareFunction::Undefined &&
            device->IsToggleEnabled(Toggle::MetalDisableSamplerCompare)) {
            return DAWN_VALIDATION_ERROR("Sampler compare function not supported.");
        }

        return new Sampler(device, descriptor);
    }

    Sampler::Sampler(Device* device, const SamplerDescriptor* descriptor)
        : SamplerBase(device, descriptor) {
        MTLSamplerDescriptor* mtlDesc = [MTLSamplerDescriptor new];

        mtlDesc.minFilter = FilterModeToMinMagFilter(descriptor->minFilter);
        mtlDesc.magFilter = FilterModeToMinMagFilter(descriptor->magFilter);
        mtlDesc.mipFilter = FilterModeToMipFilter(descriptor->mipmapFilter);

        mtlDesc.sAddressMode = AddressMode(descriptor->addressModeU);
        mtlDesc.tAddressMode = AddressMode(descriptor->addressModeV);
        mtlDesc.rAddressMode = AddressMode(descriptor->addressModeW);

        mtlDesc.lodMinClamp = descriptor->lodMinClamp;
        mtlDesc.lodMaxClamp = descriptor->lodMaxClamp;

        if (descriptor->compare != wgpu::CompareFunction::Undefined) {
            // Sampler compare is unsupported before A9, which we validate in
            // Sampler::Create.
            mtlDesc.compareFunction = ToMetalCompareFunction(descriptor->compare);
            // The value is default-initialized in the else-case, and we don't set it or the
            // Metal debug device errors.
        }

        mMtlSamplerState = [device->GetMTLDevice() newSamplerStateWithDescriptor:mtlDesc];

        [mtlDesc release];
    }

    Sampler::~Sampler() {
        [mMtlSamplerState release];
    }

    id<MTLSamplerState> Sampler::GetMTLSamplerState() {
        return mMtlSamplerState;
    }

}}  // namespace dawn_native::metal
