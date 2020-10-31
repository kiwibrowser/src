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
#include <unordered_map>
#include <vector>

#include "dawn_native/DawnNative.h"

namespace dawn_native {

    enum class Toggle {
        EmulateStoreAndMSAAResolve,
        NonzeroClearResourcesOnCreationForTesting,
        AlwaysResolveIntoZeroLevelAndLayer,
        LazyClearResourceOnFirstUse,
        TurnOffVsync,
        UseTemporaryBufferInCompressedTextureToTextureCopy,
        UseD3D12ResourceHeapTier2,
        UseD3D12RenderPass,
        UseD3D12ResidencyManagement,
        SkipValidation,
        UseSpvc,
        UseSpvcParser,
        VulkanUseD32S8,
        MetalDisableSamplerCompare,
        DisableBaseVertex,
        DisableBaseInstance,
        UseD3D12SmallShaderVisibleHeapForTesting,
        UseDXC,
        DisableRobustness,
        LazyClearBufferOnFirstUse,

        EnumCount,
        InvalidEnum = EnumCount,
    };

    // A wrapper of the bitset to store if a toggle is present or not. This wrapper provides the
    // convenience to convert the enums of enum class Toggle to the indices of a bitset.
    struct TogglesSet {
        std::bitset<static_cast<size_t>(Toggle::EnumCount)> toggleBitset;

        void Set(Toggle toggle, bool enabled);
        bool Has(Toggle toggle) const;
        std::vector<const char*> GetContainedToggleNames() const;
    };

    const char* ToggleEnumToName(Toggle toggle);

    class TogglesInfo {
      public:
        // Used to query the details of a toggle. Return nullptr if toggleName is not a valid name
        // of a toggle supported in Dawn.
        const ToggleInfo* GetToggleInfo(const char* toggleName);
        Toggle ToggleNameToEnum(const char* toggleName);

      private:
        void EnsureToggleNameToEnumMapInitialized();

        bool mToggleNameToEnumMapInitialized = false;
        std::unordered_map<std::string, Toggle> mToggleNameToEnumMap;
    };

}  // namespace dawn_native

#endif  // DAWNNATIVE_TOGGLES_H_
