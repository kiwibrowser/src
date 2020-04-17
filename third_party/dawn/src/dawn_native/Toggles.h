// Copyright 2019 The Dawn Authors
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

#ifndef DAWNNATIVE_TOGGLES_H_
#define DAWNNATIVE_TOGGLES_H_

#include <bitset>

#include "dawn_native/DawnNative.h"

namespace dawn_native {

    enum class Toggle {
        EmulateStoreAndMSAAResolve,
        NonzeroClearResourcesOnCreationForTesting,
        AlwaysResolveIntoZeroLevelAndLayer,
        LazyClearResourceOnFirstUse,

        EnumCount,
        InvalidEnum = EnumCount,
    };

    // A wrapper of the bitset to store if a toggle is enabled or not. This wrapper provides the
    // convenience to convert the enums of enum class Toggle to the indices of a bitset.
    struct TogglesSet {
        std::bitset<static_cast<size_t>(Toggle::EnumCount)> toggleBitset;

        void SetToggle(Toggle toggle, bool enabled) {
            ASSERT(toggle != Toggle::InvalidEnum);
            const size_t toggleIndex = static_cast<size_t>(toggle);
            toggleBitset.set(toggleIndex, enabled);
        }

        bool IsEnabled(Toggle toggle) const {
            ASSERT(toggle != Toggle::InvalidEnum);
            const size_t toggleIndex = static_cast<size_t>(toggle);
            return toggleBitset.test(toggleIndex);
        }
    };

}  // namespace dawn_native

#endif  // DAWNNATIVE_TOGGLES_H_
