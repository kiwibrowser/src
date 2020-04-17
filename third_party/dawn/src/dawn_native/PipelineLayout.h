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

#ifndef DAWNNATIVE_PIPELINELAYOUT_H_
#define DAWNNATIVE_PIPELINELAYOUT_H_

#include "common/Constants.h"
#include "dawn_native/Error.h"
#include "dawn_native/Forward.h"
#include "dawn_native/ObjectBase.h"

#include "dawn_native/dawn_platform.h"

#include <array>
#include <bitset>

namespace dawn_native {

    MaybeError ValidatePipelineLayoutDescriptor(DeviceBase*,
                                                const PipelineLayoutDescriptor* descriptor);

    using BindGroupLayoutArray = std::array<Ref<BindGroupLayoutBase>, kMaxBindGroups>;

    class PipelineLayoutBase : public ObjectBase {
      public:
        PipelineLayoutBase(DeviceBase* device,
                           const PipelineLayoutDescriptor* descriptor,
                           bool blueprint = false);
        ~PipelineLayoutBase() override;

        static PipelineLayoutBase* MakeError(DeviceBase* device);

        const BindGroupLayoutBase* GetBindGroupLayout(size_t group) const;
        const std::bitset<kMaxBindGroups> GetBindGroupLayoutsMask() const;

        // Utility functions to compute inherited bind groups.
        // Returns the inherited bind groups as a mask.
        std::bitset<kMaxBindGroups> InheritedGroupsMask(const PipelineLayoutBase* other) const;

        // Returns the index of the first incompatible bind group in the range
        // [1, kMaxBindGroups + 1]
        uint32_t GroupsInheritUpTo(const PipelineLayoutBase* other) const;

        // Functors necessary for the unordered_set<PipelineLayoutBase*>-based cache.
        struct HashFunc {
            size_t operator()(const PipelineLayoutBase* pl) const;
        };
        struct EqualityFunc {
            bool operator()(const PipelineLayoutBase* a, const PipelineLayoutBase* b) const;
        };

      protected:
        PipelineLayoutBase(DeviceBase* device, ObjectBase::ErrorTag tag);

        BindGroupLayoutArray mBindGroupLayouts;
        std::bitset<kMaxBindGroups> mMask;
        bool mIsBlueprint = false;
    };

}  // namespace dawn_native

#endif  // DAWNNATIVE_PIPELINELAYOUT_H_
