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

#ifndef DAWNNATIVE_EXTENSIONS_H_
#define DAWNNATIVE_EXTENSIONS_H_

#include <bitset>
#include <unordered_map>
#include <vector>

#include "dawn_native/DawnNative.h"

namespace dawn_native {

    enum class Extension {
        TextureCompressionBC,
        ShaderFloat16,
        PipelineStatisticsQuery,
        TimestampQuery,

        EnumCount,
        InvalidEnum = EnumCount,
        ExtensionMin = TextureCompressionBC,
    };

    // A wrapper of the bitset to store if an extension is enabled or not. This wrapper provides the
    // convenience to convert the enums of enum class Extension to the indices of a bitset.
    struct ExtensionsSet {
        std::bitset<static_cast<size_t>(Extension::EnumCount)> extensionsBitSet;

        void EnableExtension(Extension extension);
        bool IsEnabled(Extension extension) const;
        std::vector<const char*> GetEnabledExtensionNames() const;
        void InitializeDeviceProperties(WGPUDeviceProperties* properties) const;
    };

    const char* ExtensionEnumToName(Extension extension);

    class ExtensionsInfo {
      public:
        ExtensionsInfo();

        // Used to query the details of an extension. Return nullptr if extensionName is not a valid
        // name of an extension supported in Dawn
        const ExtensionInfo* GetExtensionInfo(const char* extensionName) const;
        Extension ExtensionNameToEnum(const char* extensionName) const;
        ExtensionsSet ExtensionNamesToExtensionsSet(
            const std::vector<const char*>& requiredExtensions) const;

      private:
        std::unordered_map<std::string, Extension> mExtensionNameToEnumMap;
    };

}  // namespace dawn_native

#endif  // DAWNNATIVE_EXTENSIONS_H_
