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

#include "dawn_native/BindGroupLayout.h"

#include "common/BitSetIterator.h"
#include "common/HashUtils.h"
#include "dawn_native/Device.h"
#include "dawn_native/ValidationUtils_autogen.h"

#include <functional>

namespace dawn_native {

    MaybeError ValidateBindGroupLayoutDescriptor(DeviceBase*,
                                                 const BindGroupLayoutDescriptor* descriptor) {
        if (descriptor->nextInChain != nullptr) {
            return DAWN_VALIDATION_ERROR("nextInChain must be nullptr");
        }

        std::bitset<kMaxBindingsPerGroup> bindingsSet;
        for (uint32_t i = 0; i < descriptor->bindingCount; ++i) {
            auto& binding = descriptor->bindings[i];
            DAWN_TRY(ValidateShaderStageBit(binding.visibility));
            DAWN_TRY(ValidateBindingType(binding.type));

            if (binding.binding >= kMaxBindingsPerGroup) {
                return DAWN_VALIDATION_ERROR("some binding index exceeds the maximum value");
            }
            if (bindingsSet[binding.binding]) {
                return DAWN_VALIDATION_ERROR("some binding index was specified more than once");
            }
            bindingsSet.set(binding.binding);
        }
        return {};
    }

    namespace {
        size_t HashBindingInfo(const BindGroupLayoutBase::LayoutBindingInfo& info) {
            size_t hash = Hash(info.mask);

            for (uint32_t binding : IterateBitSet(info.mask)) {
                HashCombine(&hash, info.visibilities[binding], info.types[binding]);
            }

            return hash;
        }

        bool operator==(const BindGroupLayoutBase::LayoutBindingInfo& a,
                        const BindGroupLayoutBase::LayoutBindingInfo& b) {
            if (a.mask != b.mask) {
                return false;
            }

            for (uint32_t binding : IterateBitSet(a.mask)) {
                if ((a.visibilities[binding] != b.visibilities[binding]) ||
                    (a.types[binding] != b.types[binding])) {
                    return false;
                }
            }

            return true;
        }
    }  // namespace

    // BindGroupLayoutBase

    BindGroupLayoutBase::BindGroupLayoutBase(DeviceBase* device,
                                             const BindGroupLayoutDescriptor* descriptor,
                                             bool blueprint)
        : ObjectBase(device), mIsBlueprint(blueprint) {
        for (uint32_t i = 0; i < descriptor->bindingCount; ++i) {
            auto& binding = descriptor->bindings[i];

            uint32_t index = binding.binding;
            mBindingInfo.visibilities[index] = binding.visibility;
            mBindingInfo.types[index] = binding.type;

            if (binding.type == dawn::BindingType::DynamicUniformBuffer ||
                binding.type == dawn::BindingType::DynamicStorageBuffer) {
                mDynamicBufferCount++;
            }

            ASSERT(!mBindingInfo.mask[index]);
            mBindingInfo.mask.set(index);
        }
    }

    BindGroupLayoutBase::BindGroupLayoutBase(DeviceBase* device, ObjectBase::ErrorTag tag)
        : ObjectBase(device, tag), mIsBlueprint(true) {
    }

    BindGroupLayoutBase::~BindGroupLayoutBase() {
        // Do not uncache the actual cached object if we are a blueprint
        if (!mIsBlueprint && !IsError()) {
            GetDevice()->UncacheBindGroupLayout(this);
        }
    }

    // static
    BindGroupLayoutBase* BindGroupLayoutBase::MakeError(DeviceBase* device) {
        return new BindGroupLayoutBase(device, ObjectBase::kError);
    }

    const BindGroupLayoutBase::LayoutBindingInfo& BindGroupLayoutBase::GetBindingInfo() const {
        ASSERT(!IsError());
        return mBindingInfo;
    }

    size_t BindGroupLayoutBase::HashFunc::operator()(const BindGroupLayoutBase* bgl) const {
        return HashBindingInfo(bgl->mBindingInfo);
    }

    bool BindGroupLayoutBase::EqualityFunc::operator()(const BindGroupLayoutBase* a,
                                                       const BindGroupLayoutBase* b) const {
        return a->mBindingInfo == b->mBindingInfo;
    }

    uint32_t BindGroupLayoutBase::GetDynamicBufferCount() const {
        return mDynamicBufferCount;
    }

}  // namespace dawn_native
