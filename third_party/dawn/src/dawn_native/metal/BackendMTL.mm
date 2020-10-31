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

#include "dawn_native/metal/BackendMTL.h"

#include "common/GPUInfo.h"
#include "common/Platform.h"
#include "dawn_native/Instance.h"
#include "dawn_native/MetalBackend.h"
#include "dawn_native/metal/DeviceMTL.h"

#if defined(DAWN_PLATFORM_MACOS)
#    import <IOKit/IOKitLib.h>
#endif

namespace dawn_native { namespace metal {

    namespace {

        struct PCIIDs {
            uint32_t vendorId;
            uint32_t deviceId;
        };

        struct Vendor {
            const char* trademark;
            uint32_t vendorId;
        };

#if defined(DAWN_PLATFORM_MACOS)
        const Vendor kVendors[] = {{"AMD", gpu_info::kVendorID_AMD},
                                   {"Radeon", gpu_info::kVendorID_AMD},
                                   {"Intel", gpu_info::kVendorID_Intel},
                                   {"Geforce", gpu_info::kVendorID_Nvidia},
                                   {"Quadro", gpu_info::kVendorID_Nvidia}};

        // Find vendor ID from MTLDevice name.
        MaybeError GetVendorIdFromVendors(id<MTLDevice> device, PCIIDs* ids) {
            uint32_t vendorId = 0;
            const char* deviceName = [device.name UTF8String];
            for (const auto& it : kVendors) {
                if (strstr(deviceName, it.trademark) != nullptr) {
                    vendorId = it.vendorId;
                    break;
                }
            }

            if (vendorId == 0) {
                return DAWN_INTERNAL_ERROR("Failed to find vendor id with the device");
            }

            // Set vendor id with 0
            *ids = PCIIDs{vendorId, 0};
            return {};
        }

        // Extracts an integer property from a registry entry.
        uint32_t GetEntryProperty(io_registry_entry_t entry, CFStringRef name) {
            uint32_t value = 0;

            // Recursively search registry entry and its parents for property name
            // The data should release with CFRelease
            CFDataRef data = static_cast<CFDataRef>(IORegistryEntrySearchCFProperty(
                entry, kIOServicePlane, name, kCFAllocatorDefault,
                kIORegistryIterateRecursively | kIORegistryIterateParents));

            if (data == nullptr) {
                return value;
            }

            // CFDataGetBytePtr() is guaranteed to return a read-only pointer
            value = *reinterpret_cast<const uint32_t*>(CFDataGetBytePtr(data));

            CFRelease(data);
            return value;
        }

        // Queries the IO Registry to find the PCI device and vendor IDs of the MTLDevice.
        // The registry entry correponding to [device registryID] doesn't contain the exact PCI ids
        // because it corresponds to a driver. However its parent entry corresponds to the device
        // itself and has uint32_t "device-id" and "registry-id" keys. For example on a dual-GPU
        // MacBook Pro 2017 the IORegistry explorer shows the following tree (simplified here):
        //
        //  - PCI0@0
        //  | - AppleACPIPCI
        //  | | - IGPU@2 (type IOPCIDevice)
        //  | | | - IntelAccelerator (type IOGraphicsAccelerator2)
        //  | | - PEG0@1
        //  | | | - IOPP
        //  | | | | - GFX0@0 (type IOPCIDevice)
        //  | | | | | - AMDRadeonX4000_AMDBaffinGraphicsAccelerator (type IOGraphicsAccelerator2)
        //
        // [device registryID] is the ID for one of the IOGraphicsAccelerator2 and we can see that
        // their parent always is an IOPCIDevice that has properties for the device and vendor IDs.
        MaybeError API_AVAILABLE(macos(10.13))
            GetDeviceIORegistryPCIInfo(id<MTLDevice> device, PCIIDs* ids) {
            // Get a matching dictionary for the IOGraphicsAccelerator2
            CFMutableDictionaryRef matchingDict = IORegistryEntryIDMatching([device registryID]);
            if (matchingDict == nullptr) {
                return DAWN_INTERNAL_ERROR("Failed to create the matching dict for the device");
            }

            // IOServiceGetMatchingService will consume the reference on the matching dictionary,
            // so we don't need to release the dictionary.
            io_registry_entry_t acceleratorEntry =
                IOServiceGetMatchingService(kIOMasterPortDefault, matchingDict);
            if (acceleratorEntry == IO_OBJECT_NULL) {
                return DAWN_INTERNAL_ERROR(
                    "Failed to get the IO registry entry for the accelerator");
            }

            // Get the parent entry that will be the IOPCIDevice
            io_registry_entry_t deviceEntry = IO_OBJECT_NULL;
            if (IORegistryEntryGetParentEntry(acceleratorEntry, kIOServicePlane, &deviceEntry) !=
                kIOReturnSuccess) {
                IOObjectRelease(acceleratorEntry);
                return DAWN_INTERNAL_ERROR("Failed to get the IO registry entry for the device");
            }

            ASSERT(deviceEntry != IO_OBJECT_NULL);
            IOObjectRelease(acceleratorEntry);

            uint32_t vendorId = GetEntryProperty(deviceEntry, CFSTR("vendor-id"));
            uint32_t deviceId = GetEntryProperty(deviceEntry, CFSTR("device-id"));

            *ids = PCIIDs{vendorId, deviceId};

            IOObjectRelease(deviceEntry);

            return {};
        }

        MaybeError GetDevicePCIInfo(id<MTLDevice> device, PCIIDs* ids) {
            // [device registryID] is introduced on macOS 10.13+, otherwise workaround to get vendor
            // id by vendor name on old macOS
            if (@available(macos 10.13, *)) {
                return GetDeviceIORegistryPCIInfo(device, ids);
            } else {
                return GetVendorIdFromVendors(device, ids);
            }
        }

        bool IsMetalSupported() {
            // Metal was first introduced in macOS 10.11
            NSOperatingSystemVersion macOS10_11 = {10, 11, 0};
            return [NSProcessInfo.processInfo isOperatingSystemAtLeastVersion:macOS10_11];
        }
#elif defined(DAWN_PLATFORM_IOS)
        MaybeError GetDevicePCIInfo(id<MTLDevice> device, PCIIDs* ids) {
            DAWN_UNUSED(device);
            *ids = PCIIDs{0, 0};
            return {};
        }

        bool IsMetalSupported() {
            return true;
        }
#else
#    error "Unsupported Apple platform."
#endif
    }  // anonymous namespace

    // The Metal backend's Adapter.

    class Adapter : public AdapterBase {
      public:
        Adapter(InstanceBase* instance, id<MTLDevice> device)
            : AdapterBase(instance, wgpu::BackendType::Metal), mDevice([device retain]) {
            mPCIInfo.name = std::string([mDevice.name UTF8String]);

            PCIIDs ids;
            if (!instance->ConsumedError(GetDevicePCIInfo(device, &ids))) {
                mPCIInfo.vendorId = ids.vendorId;
                mPCIInfo.deviceId = ids.deviceId;
            }

#if defined(DAWN_PLATFORM_IOS)
            mAdapterType = wgpu::AdapterType::IntegratedGPU;
#elif defined(DAWN_PLATFORM_MACOS)
            if ([device isLowPower]) {
                mAdapterType = wgpu::AdapterType::IntegratedGPU;
            } else {
                mAdapterType = wgpu::AdapterType::DiscreteGPU;
            }
#else
#    error "Unsupported Apple platform."
#endif

            InitializeSupportedExtensions();
        }

        ~Adapter() override {
            [mDevice release];
        }

      private:
        ResultOrError<DeviceBase*> CreateDeviceImpl(const DeviceDescriptor* descriptor) override {
            return Device::Create(this, mDevice, descriptor);
        }

        void InitializeSupportedExtensions() {
#if defined(DAWN_PLATFORM_MACOS)
            if ([mDevice supportsFeatureSet:MTLFeatureSet_macOS_GPUFamily1_v1]) {
                mSupportedExtensions.EnableExtension(Extension::TextureCompressionBC);
            }

            if (@available(macOS 10.15, *)) {
                mSupportedExtensions.EnableExtension(Extension::PipelineStatisticsQuery);
                mSupportedExtensions.EnableExtension(Extension::TimestampQuery);
            }
#endif

            mSupportedExtensions.EnableExtension(Extension::ShaderFloat16);
        }

        id<MTLDevice> mDevice = nil;
    };

    // Implementation of the Metal backend's BackendConnection

    Backend::Backend(InstanceBase* instance)
        : BackendConnection(instance, wgpu::BackendType::Metal) {
        if (GetInstance()->IsBackendValidationEnabled()) {
            setenv("METAL_DEVICE_WRAPPER_TYPE", "1", 1);
        }
    }

    std::vector<std::unique_ptr<AdapterBase>> Backend::DiscoverDefaultAdapters() {
        std::vector<std::unique_ptr<AdapterBase>> adapters;
        BOOL supportedVersion = NO;
#if defined(DAWN_PLATFORM_MACOS)
        if (@available(macOS 10.11, *)) {
            supportedVersion = YES;
            NSArray<id<MTLDevice>>* devices = MTLCopyAllDevices();

            for (id<MTLDevice> device in devices) {
                adapters.push_back(std::make_unique<Adapter>(GetInstance(), device));
            }

            [devices release];
        }
#endif

#if defined(DAWN_PLATFORM_IOS)
        if (@available(iOS 8.0, *)) {
            supportedVersion = YES;
            // iOS only has a single device so MTLCopyAllDevices doesn't exist there.
            adapters.push_back(
                std::make_unique<Adapter>(GetInstance(), MTLCreateSystemDefaultDevice()));
        }
#endif
        if (!supportedVersion) {
            UNREACHABLE();
        }
        return adapters;
    }

    BackendConnection* Connect(InstanceBase* instance) {
        if (!IsMetalSupported()) {
            return nullptr;
        }
        return new Backend(instance);
    }

}}  // namespace dawn_native::metal
