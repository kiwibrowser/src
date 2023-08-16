// Copyright 2018 The Dawn Authors
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

#ifndef DAWNNATIVE_DAWNNATIVE_H_
#define DAWNNATIVE_DAWNNATIVE_H_

#include <dawn/dawn_proc_table.h>
#include <dawn/webgpu.h>
#include <dawn_native/dawn_native_export.h>

#include <string>
#include <vector>

namespace dawn_platform {
    class Platform;
}  // namespace dawn_platform

namespace wgpu {
    struct AdapterProperties;
}

namespace dawn_native {

    // DEPRECATED: use WGPUAdapterProperties instead.
    struct PCIInfo {
        uint32_t deviceId = 0;
        uint32_t vendorId = 0;
        std::string name;
    };

    // DEPRECATED: use WGPUBackendType instead.
    enum class BackendType {
        D3D12,
        Metal,
        Null,
        OpenGL,
        Vulkan,
    };

    // DEPRECATED: use WGPUAdapterType instead.
    enum class DeviceType {
        DiscreteGPU,
        IntegratedGPU,
        CPU,
        Unknown,
    };

    class InstanceBase;
    class AdapterBase;

    // An optional parameter of Adapter::CreateDevice() to send additional information when creating
    // a Device. For example, we can use it to enable a workaround, optimization or feature.
    struct DAWN_NATIVE_EXPORT DeviceDescriptor {
        std::vector<const char*> requiredExtensions;
        std::vector<const char*> forceEnabledToggles;
        std::vector<const char*> forceDisabledToggles;
    };

    // A struct to record the information of a toggle. A toggle is a code path in Dawn device that
    // can be manually configured to run or not outside Dawn, including workarounds, special
    // features and optimizations.
    struct ToggleInfo {
        const char* name;
        const char* description;
        const char* url;
    };

    // A struct to record the information of an extension. An extension is a GPU feature that is not
    // required to be supported by all Dawn backends and can only be used when it is enabled on the
    // creation of device.
    using ExtensionInfo = ToggleInfo;

    // An adapter is an object that represent on possibility of creating devices in the system.
    // Most of the time it will represent a combination of a physical GPU and an API. Not that the
    // same GPU can be represented by multiple adapters but on different APIs.
    //
    // The underlying Dawn adapter is owned by the Dawn instance so this class is not RAII but just
    // a reference to an underlying adapter.
    class DAWN_NATIVE_EXPORT Adapter {
      public:
        Adapter();
        Adapter(AdapterBase* impl);
        ~Adapter();

        // DEPRECATED: use GetProperties instead.
        BackendType GetBackendType() const;
        DeviceType GetDeviceType() const;
        const PCIInfo& GetPCIInfo() const;

        // Essentially webgpu.h's wgpuAdapterGetProperties while we don't have WGPUAdapter in
        // dawn.json
        void GetProperties(wgpu::AdapterProperties* properties) const;

        std::vector<const char*> GetSupportedExtensions() const;
        WGPUDeviceProperties GetAdapterProperties() const;

        explicit operator bool() const;

        // Create a device on this adapter, note that the interface will change to include at least
        // a device descriptor and a pointer to backend specific options.
        // On an error, nullptr is returned.
        WGPUDevice CreateDevice(const DeviceDescriptor* deviceDescriptor = nullptr);

      private:
        AdapterBase* mImpl = nullptr;
    };

    // Base class for options passed to Instance::DiscoverAdapters.
    struct DAWN_NATIVE_EXPORT AdapterDiscoveryOptionsBase {
      public:
        const WGPUBackendType backendType;

      protected:
        AdapterDiscoveryOptionsBase(WGPUBackendType type);
    };

    // Represents a connection to dawn_native and is used for dependency injection, discovering
    // system adapters and injecting custom adapters (like a Swiftshader Vulkan adapter).
    //
    // This is an RAII class for Dawn instances and also controls the lifetime of all adapters
    // for this instance.
    class DAWN_NATIVE_EXPORT Instance {
      public:
        Instance();
        ~Instance();

        Instance(const Instance& other) = delete;
        Instance& operator=(const Instance& other) = delete;

        // Gather all adapters in the system that can be accessed with no special options. These
        // adapters will later be returned by GetAdapters.
        void DiscoverDefaultAdapters();

        // Adds adapters that can be discovered with the options provided (like a getProcAddress).
        // The backend is chosen based on the type of the options used. Returns true on success.
        bool DiscoverAdapters(const AdapterDiscoveryOptionsBase* options);

        // Returns all the adapters that the instance knows about.
        std::vector<Adapter> GetAdapters() const;

        const ToggleInfo* GetToggleInfo(const char* toggleName);

        // Enable backend's validation layers if it has.
        void EnableBackendValidation(bool enableBackendValidation);

        // Enable debug capture on Dawn startup
        void EnableBeginCaptureOnStartup(bool beginCaptureOnStartup);

        void SetPlatform(dawn_platform::Platform* platform);

        // Returns the underlying WGPUInstance object.
        WGPUInstance Get() const;

      private:
        InstanceBase* mImpl = nullptr;
    };

    // Backend-agnostic API for dawn_native
    DAWN_NATIVE_EXPORT DawnProcTable GetProcs();

    // Query the names of all the toggles that are enabled in device
    DAWN_NATIVE_EXPORT std::vector<const char*> GetTogglesUsed(WGPUDevice device);

    // Backdoor to get the number of lazy clears for testing
    DAWN_NATIVE_EXPORT size_t GetLazyClearCountForTesting(WGPUDevice device);

    // Backdoor to get the number of deprecation warnings for testing
    DAWN_NATIVE_EXPORT size_t GetDeprecationWarningCountForTesting(WGPUDevice device);

    //  Query if texture has been initialized
    DAWN_NATIVE_EXPORT bool IsTextureSubresourceInitialized(WGPUTexture texture,
                                                            uint32_t baseMipLevel,
                                                            uint32_t levelCount,
                                                            uint32_t baseArrayLayer,
                                                            uint32_t layerCount);

    // Backdoor to get the order of the ProcMap for testing
    DAWN_NATIVE_EXPORT std::vector<const char*> GetProcMapNamesForTesting();

    // ErrorInjector functions used for testing only. Defined in dawn_native/ErrorInjector.cpp
    DAWN_NATIVE_EXPORT void EnableErrorInjector();
    DAWN_NATIVE_EXPORT void DisableErrorInjector();
    DAWN_NATIVE_EXPORT void ClearErrorInjector();
    DAWN_NATIVE_EXPORT uint64_t AcquireErrorInjectorCallCount();
    DAWN_NATIVE_EXPORT void InjectErrorAt(uint64_t index);

    // The different types of ExternalImageDescriptors
    enum ExternalImageDescriptorType {
        OpaqueFD,
        DmaBuf,
        IOSurface,
        DXGISharedHandle,
    };

    // Common properties of external images
    struct DAWN_NATIVE_EXPORT ExternalImageDescriptor {
      public:
        const ExternalImageDescriptorType type;
        const WGPUTextureDescriptor* cTextureDescriptor;  // Must match image creation params
        bool isCleared;  // Sets whether the texture will be cleared before use

      protected:
        ExternalImageDescriptor(ExternalImageDescriptorType type);
    };
}  // namespace dawn_native

#endif  // DAWNNATIVE_DAWNNATIVE_H_
