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

#include <array>

#include "common/Assert.h"
#include "common/BitSetIterator.h"
#include "dawn_native/Extensions.h"

namespace dawn_native {
    namespace {

        struct ExtensionEnumAndInfo {
            Extension extension;
            ExtensionInfo info;
            bool WGPUDeviceProperties::*memberInWGPUDeviceProperties;
        };

        using ExtensionEnumAndInfoList =
            std::array<ExtensionEnumAndInfo, static_cast<size_t>(Extension::EnumCount)>;

        static constexpr ExtensionEnumAndInfoList kExtensionNameAndInfoList = {
            {{Extension::TextureCompressionBC,
              {"texture_compression_bc", "Support Block Compressed (BC) texture formats",
               "https://bugs.chromium.org/p/dawn/issues/detail?id=42"},
              &WGPUDeviceProperties::textureCompressionBC},
             {Extension::ShaderFloat16,
              {"shader_float16",
               "Support 16bit float arithmetic and declarations in uniform and storage buffers",
               "https://bugs.chromium.org/p/dawn/issues/detail?id=426"},
              &WGPUDeviceProperties::shaderFloat16},
             {Extension::PipelineStatisticsQuery,
              {"pipeline_statistics_query", "Support Pipeline Statistics Query",
               "https://bugs.chromium.org/p/dawn/issues/detail?id=434"},
              &WGPUDeviceProperties::pipelineStatisticsQuery},
             {Extension::TimestampQuery,
              {"timestamp_query", "Support Timestamp Query",
               "https://bugs.chromium.org/p/dawn/issues/detail?id=434"},
              &WGPUDeviceProperties::timestampQuery}}};

    }  // anonymous namespace

    void ExtensionsSet::EnableExtension(Extension extension) {
        ASSERT(extension != Extension::InvalidEnum);
        const size_t extensionIndex = static_cast<size_t>(extension);
        extensionsBitSet.set(extensionIndex);
    }

    bool ExtensionsSet::IsEnabled(Extension extension) const {
        ASSERT(extension != Extension::InvalidEnum);
        const size_t extensionIndex = static_cast<size_t>(extension);
        return extensionsBitSet[extensionIndex];
    }

    std::vector<const char*> ExtensionsSet::GetEnabledExtensionNames() const {
        std::vector<const char*> enabledExtensionNames(extensionsBitSet.count());

        uint32_t index = 0;
        for (uint32_t i : IterateBitSet(extensionsBitSet)) {
            const char* extensionName = ExtensionEnumToName(static_cast<Extension>(i));
            enabledExtensionNames[index] = extensionName;
            ++index;
        }
        return enabledExtensionNames;
    }

    void ExtensionsSet::InitializeDeviceProperties(WGPUDeviceProperties* properties) const {
        ASSERT(properties != nullptr);

        for (uint32_t i : IterateBitSet(extensionsBitSet)) {
            properties->*(kExtensionNameAndInfoList[i].memberInWGPUDeviceProperties) = true;
        }
    }

    const char* ExtensionEnumToName(Extension extension) {
        ASSERT(extension != Extension::InvalidEnum);

        const ExtensionEnumAndInfo& extensionNameAndInfo =
            kExtensionNameAndInfoList[static_cast<size_t>(extension)];
        ASSERT(extensionNameAndInfo.extension == extension);
        return extensionNameAndInfo.info.name;
    }

    ExtensionsInfo::ExtensionsInfo() {
        for (size_t index = 0; index < kExtensionNameAndInfoList.size(); ++index) {
            const ExtensionEnumAndInfo& extensionNameAndInfo = kExtensionNameAndInfoList[index];
            ASSERT(index == static_cast<size_t>(extensionNameAndInfo.extension));
            mExtensionNameToEnumMap[extensionNameAndInfo.info.name] =
                extensionNameAndInfo.extension;
        }
    }

    const ExtensionInfo* ExtensionsInfo::GetExtensionInfo(const char* extensionName) const {
        ASSERT(extensionName);

        const auto& iter = mExtensionNameToEnumMap.find(extensionName);
        if (iter != mExtensionNameToEnumMap.cend()) {
            return &kExtensionNameAndInfoList[static_cast<size_t>(iter->second)].info;
        }
        return nullptr;
    }

    Extension ExtensionsInfo::ExtensionNameToEnum(const char* extensionName) const {
        ASSERT(extensionName);

        const auto& iter = mExtensionNameToEnumMap.find(extensionName);
        if (iter != mExtensionNameToEnumMap.cend()) {
            return kExtensionNameAndInfoList[static_cast<size_t>(iter->second)].extension;
        }
        return Extension::InvalidEnum;
    }

    ExtensionsSet ExtensionsInfo::ExtensionNamesToExtensionsSet(
        const std::vector<const char*>& requiredExtensions) const {
        ExtensionsSet extensionsSet;

        for (const char* extensionName : requiredExtensions) {
            Extension extensionEnum = ExtensionNameToEnum(extensionName);
            ASSERT(extensionEnum != Extension::InvalidEnum);
            extensionsSet.EnableExtension(extensionEnum);
        }
        return extensionsSet;
    }

}  // namespace dawn_native
