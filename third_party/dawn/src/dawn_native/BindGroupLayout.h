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

#ifndef DAWNNATIVE_BINDGROUPLAYOUT_H_
#define DAWNNATIVE_BINDGROUPLAYOUT_H_

#include "common/Constants.h"
#include "dawn_native/Error.h"
#include "dawn_native/Forward.h"
#include "dawn_native/ObjectBase.h"

#include "dawn_native/dawn_platform.h"

#include <array>
#include <bitset>

namespace dawn_native {

    MaybeError ValidateBindGroupLayoutDescriptor(DeviceBase*,
                                                 const BindGroupLayoutDescriptor* descriptor);

    class BindGroupLayoutBase : public ObjectBase {
      public:
        BindGroupLayoutBase(DeviceBase* device,
                            const BindGroupLayoutDescriptor* descriptor,
                            bool blueprint = false);
        ~BindGroupLayoutBase() override;

        static BindGroupLayoutBase* MakeError(DeviceBase* device);

        struct LayoutBindingInfo {
            std::array<dawn::ShaderStageBit, kMaxBindingsPerGroup> visibilities;
            std::array<dawn::BindingType, kMaxBindingsPerGroup> types;
            std::bitset<kMaxBindingsPerGroup> mask;
        };
        const LayoutBindingInfo& GetBindingInfo() const;

        // Functors necessary for the unordered_set<BGLBase*>-based cache.
        struct HashFunc {
            size_t operator()(const BindGroupLayoutBase* bgl) const;
        };
        struct EqualityFunc {
            bool operator()(const BindGroupLayoutBase* a, const BindGroupLayoutBase* b) const;
        };

        uint32_t GetDynamicBufferCount() const;

      private:
        BindGroupLayoutBase(DeviceBase* device, ObjectBase::ErrorTag tag);

        LayoutBindingInfo mBindingInfo;
        bool mIsBlueprint = false;
        uint32_t mDynamicBufferCount = 0;
    };

}  // namespace dawn_native

#endif  // DAWNNATIVE_BINDGROUPLAYOUT_H_
