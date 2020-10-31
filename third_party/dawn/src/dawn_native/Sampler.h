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

#ifndef DAWNNATIVE_SAMPLER_H_
#define DAWNNATIVE_SAMPLER_H_

#include "dawn_native/CachedObject.h"
#include "dawn_native/Error.h"

#include "dawn_native/dawn_platform.h"

namespace dawn_native {

    class DeviceBase;

    MaybeError ValidateSamplerDescriptor(DeviceBase* device, const SamplerDescriptor* descriptor);

    class SamplerBase : public CachedObject {
      public:
        SamplerBase(DeviceBase* device, const SamplerDescriptor* descriptor);
        ~SamplerBase() override;

        static SamplerBase* MakeError(DeviceBase* device);

        bool HasCompareFunction() const;

        // Functors necessary for the unordered_set<SamplerBase*>-based cache.
        struct HashFunc {
            size_t operator()(const SamplerBase* module) const;
        };
        struct EqualityFunc {
            bool operator()(const SamplerBase* a, const SamplerBase* b) const;
        };

      private:
        SamplerBase(DeviceBase* device, ObjectBase::ErrorTag tag);

        // TODO(cwallez@chromium.org): Store a crypto hash of the items instead?
        wgpu::AddressMode mAddressModeU;
        wgpu::AddressMode mAddressModeV;
        wgpu::AddressMode mAddressModeW;
        wgpu::FilterMode mMagFilter;
        wgpu::FilterMode mMinFilter;
        wgpu::FilterMode mMipmapFilter;
        float mLodMinClamp;
        float mLodMaxClamp;
        wgpu::CompareFunction mCompareFunction;
    };

}  // namespace dawn_native

#endif  // DAWNNATIVE_SAMPLER_H_
