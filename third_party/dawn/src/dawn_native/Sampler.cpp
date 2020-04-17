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

#include "dawn_native/Sampler.h"

#include "common/HashUtils.h"
#include "dawn_native/Device.h"
#include "dawn_native/ValidationUtils_autogen.h"

#include <cmath>

namespace dawn_native {

    MaybeError ValidateSamplerDescriptor(DeviceBase*, const SamplerDescriptor* descriptor) {
        if (descriptor->nextInChain != nullptr) {
            return DAWN_VALIDATION_ERROR("nextInChain must be nullptr");
        }

        if (!std::isfinite(descriptor->lodMinClamp) || !std::isfinite(descriptor->lodMaxClamp)) {
            return DAWN_VALIDATION_ERROR("LOD must be finite");
        }

        if (descriptor->lodMinClamp < 0 || descriptor->lodMaxClamp < 0) {
            return DAWN_VALIDATION_ERROR("LOD must be positive");
        }

        if (descriptor->lodMinClamp > descriptor->lodMaxClamp) {
            return DAWN_VALIDATION_ERROR(
                "Min lod clamp value cannot greater than max lod clamp value");
        }

        DAWN_TRY(ValidateFilterMode(descriptor->minFilter));
        DAWN_TRY(ValidateFilterMode(descriptor->magFilter));
        DAWN_TRY(ValidateFilterMode(descriptor->mipmapFilter));
        DAWN_TRY(ValidateAddressMode(descriptor->addressModeU));
        DAWN_TRY(ValidateAddressMode(descriptor->addressModeV));
        DAWN_TRY(ValidateAddressMode(descriptor->addressModeW));
        DAWN_TRY(ValidateCompareFunction(descriptor->compareFunction));
        return {};
    }

    // SamplerBase

    SamplerBase::SamplerBase(DeviceBase* device,
                             const SamplerDescriptor* descriptor,
                             bool blueprint)
        : ObjectBase(device),
          mAddressModeU(descriptor->addressModeU),
          mAddressModeV(descriptor->addressModeV),
          mAddressModeW(descriptor->addressModeW),
          mMagFilter(descriptor->magFilter),
          mMinFilter(descriptor->minFilter),
          mMipmapFilter(descriptor->mipmapFilter),
          mLodMinClamp(descriptor->lodMinClamp),
          mLodMaxClamp(descriptor->lodMaxClamp),
          mCompareFunction(descriptor->compareFunction),
          mIsBlueprint(blueprint) {
    }

    SamplerBase::SamplerBase(DeviceBase* device, ObjectBase::ErrorTag tag)
        : ObjectBase(device, tag) {
    }

    SamplerBase::~SamplerBase() {
        // Do not uncache the actual cached object if we are a blueprint
        if (!mIsBlueprint && !IsError()) {
            GetDevice()->UncacheSampler(this);
        }
    }

    // static
    SamplerBase* SamplerBase::MakeError(DeviceBase* device) {
        return new SamplerBase(device, ObjectBase::kError);
    }

    size_t SamplerBase::HashFunc::operator()(const SamplerBase* module) const {
        size_t hash = 0;

        HashCombine(&hash, module->mAddressModeU);
        HashCombine(&hash, module->mAddressModeV);
        HashCombine(&hash, module->mAddressModeW);
        HashCombine(&hash, module->mMagFilter);
        HashCombine(&hash, module->mMinFilter);
        HashCombine(&hash, module->mMipmapFilter);
        HashCombine(&hash, module->mLodMinClamp);
        HashCombine(&hash, module->mLodMaxClamp);
        HashCombine(&hash, module->mCompareFunction);

        return hash;
    }

    bool SamplerBase::EqualityFunc::operator()(const SamplerBase* a, const SamplerBase* b) const {
        if (a == b) {
            return true;
        }

        ASSERT(std::isfinite(a->mLodMinClamp));
        ASSERT(std::isfinite(b->mLodMinClamp));
        ASSERT(std::isfinite(a->mLodMaxClamp));
        ASSERT(std::isfinite(b->mLodMaxClamp));

        return a->mAddressModeU == b->mAddressModeU && a->mAddressModeV == b->mAddressModeV &&
               a->mAddressModeW == b->mAddressModeW && a->mMagFilter == b->mMagFilter &&
               a->mMinFilter == b->mMinFilter && a->mMipmapFilter == b->mMipmapFilter &&
               a->mLodMinClamp == b->mLodMinClamp && a->mLodMaxClamp == b->mLodMaxClamp &&
               a->mCompareFunction == b->mCompareFunction;
    }

}  // namespace dawn_native
