// Copyright 2020 The Dawn Authors
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

#ifndef DAWNNATIVE_VULKAN_VULKANEXTENSIONS_H_
#define DAWNNATIVE_VULKAN_VULKANEXTENSIONS_H_

#include <bitset>
#include <unordered_map>

namespace dawn_native { namespace vulkan {

    // The list of known instance extensions. They must be in dependency order (this is checked
    // inside EnsureDependencies)
    enum class InstanceExt {
        // Promoted to 1.1
        GetPhysicalDeviceProperties2,
        ExternalMemoryCapabilities,
        ExternalSemaphoreCapabilities,

        // Surface extensions
        Surface,
        FuchsiaImagePipeSurface,
        MetalSurface,
        WaylandSurface,
        Win32Surface,
        XcbSurface,
        XlibSurface,

        // Others
        DebugReport,

        EnumCount,
    };

    // A bitset wrapper that is indexed with InstanceExt.
    struct InstanceExtSet {
        std::bitset<static_cast<size_t>(InstanceExt::EnumCount)> extensionBitSet;
        void Set(InstanceExt extension, bool enabled);
        bool Has(InstanceExt extension) const;
    };

    // Information about a known instance extension.
    struct InstanceExtInfo {
        InstanceExt index;
        const char* name;
        // The version in which this extension was promoted as built with VK_MAKE_VERSION,
        // or NeverPromoted if it was never promoted.
        uint32_t versionPromoted;
    };

    // Returns the information about a known InstanceExt
    const InstanceExtInfo& GetInstanceExtInfo(InstanceExt ext);
    // Returns a map that maps a Vulkan extension name to its InstanceExt.
    std::unordered_map<std::string, InstanceExt> CreateInstanceExtNameMap();

    // Sets entries in `extensions` to true if that entry was promoted in Vulkan version `version`
    void MarkPromotedExtensions(InstanceExtSet* extensions, uint32_t version);
    // From a set of extensions advertised as supported by the instance (or promoted), remove all
    // extensions that don't have all their transitive dependencies in advertisedExts.
    InstanceExtSet EnsureDependencies(const InstanceExtSet& advertisedExts);

    // The list of known device extensions. They must be in dependency order (this is checked
    // inside EnsureDependencies)
    enum class DeviceExt {
        // Promoted to 1.1
        BindMemory2,
        Maintenance1,
        StorageBufferStorageClass,
        GetPhysicalDeviceProperties2,
        GetMemoryRequirements2,
        ExternalMemoryCapabilities,
        ExternalSemaphoreCapabilities,
        ExternalMemory,
        ExternalSemaphore,
        _16BitStorage,
        SamplerYCbCrConversion,

        // Promoted to 1.2
        ImageFormatList,
        ShaderFloat16Int8,

        // External* extensions
        ExternalMemoryFD,
        ExternalMemoryDmaBuf,
        ExternalMemoryZirconHandle,
        ExternalSemaphoreFD,
        ExternalSemaphoreZirconHandle,

        // Others
        DebugMarker,
        ImageDrmFormatModifier,
        Swapchain,
        SubgroupSizeControl,

        EnumCount,
    };

    // A bitset wrapper that is indexed with DeviceExt.
    struct DeviceExtSet {
        std::bitset<static_cast<size_t>(DeviceExt::EnumCount)> extensionBitSet;
        void Set(DeviceExt extension, bool enabled);
        bool Has(DeviceExt extension) const;
    };

    // A bitset wrapper that is indexed with DeviceExt.
    struct DeviceExtInfo {
        DeviceExt index;
        const char* name;
        // The version in which this extension was promoted as built with VK_MAKE_VERSION,
        // or NeverPromoted if it was never promoted.
        uint32_t versionPromoted;
    };

    // Returns the information about a known DeviceExt
    const DeviceExtInfo& GetDeviceExtInfo(DeviceExt ext);
    // Returns a map that maps a Vulkan extension name to its DeviceExt.
    std::unordered_map<std::string, DeviceExt> CreateDeviceExtNameMap();

    // Sets entries in `extensions` to true if that entry was promoted in Vulkan version `version`
    void MarkPromotedExtensions(DeviceExtSet* extensions, uint32_t version);
    // From a set of extensions advertised as supported by the device (or promoted), remove all
    // extensions that don't have all their transitive dependencies in advertisedExts or in
    // instanceExts.
    DeviceExtSet EnsureDependencies(const DeviceExtSet& advertisedExts,
                                    const InstanceExtSet& instanceExts,
                                    uint32_t icdVersion);

}}  // namespace dawn_native::vulkan

#endif  // DAWNNATIVE_VULKAN_VULKANEXTENSIONS_H_
