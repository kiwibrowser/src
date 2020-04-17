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

#include "dawn_native/Instance.h"
#include "dawn_native/MetalBackend.h"
#include "dawn_native/metal/DeviceMTL.h"

#include <IOKit/graphics/IOGraphicsLib.h>

namespace dawn_native { namespace metal {

    namespace {
        // Since CGDisplayIOServicePort was deprecated in macOS 10.9, we need create
        // an alternative function for getting I/O service port from current display.
        io_service_t GetDisplayIOServicePort() {
            // The matching service port (or 0 if none can be found)
            io_service_t servicePort = 0;

            // Create matching dictionary for display service
            CFMutableDictionaryRef matchingDict = IOServiceMatching("IODisplayConnect");
            if (matchingDict == nullptr) {
                return 0;
            }

            io_iterator_t iter;
            // IOServiceGetMatchingServices look up the default master ports that match a
            // matching dictionary, and will consume the reference on the matching dictionary,
            // so we don't need to release the dictionary, but the iterator handle should
            // be released when its iteration is finished.
            if (IOServiceGetMatchingServices(kIOMasterPortDefault, matchingDict, &iter) !=
                kIOReturnSuccess) {
                return 0;
            }

            // Vendor number and product number of current main display
            const uint32_t displayVendorNumber = CGDisplayVendorNumber(kCGDirectMainDisplay);
            const uint32_t displayProductNumber = CGDisplayModelNumber(kCGDirectMainDisplay);

            io_service_t serv;
            while ((serv = IOIteratorNext(iter)) != IO_OBJECT_NULL) {
                CFDictionaryRef displayInfo =
                    IODisplayCreateInfoDictionary(serv, kIODisplayOnlyPreferredName);

                CFNumberRef vendorIDRef, productIDRef;
                Boolean success;
                // The ownership of CF object follows the 'Get Rule', we don't need to
                // release these values
                success = CFDictionaryGetValueIfPresent(displayInfo, CFSTR(kDisplayVendorID),
                                                        (const void**)&vendorIDRef);
                success &= CFDictionaryGetValueIfPresent(displayInfo, CFSTR(kDisplayProductID),
                                                         (const void**)&productIDRef);
                if (success) {
                    CFIndex vendorID = 0, productID = 0;
                    CFNumberGetValue(vendorIDRef, kCFNumberSInt32Type, &vendorID);
                    CFNumberGetValue(productIDRef, kCFNumberSInt32Type, &productID);

                    if (vendorID == displayVendorNumber && productID == displayProductNumber) {
                        // Check if vendor id and product id match with current display's
                        // If it does, we find the desired service port
                        servicePort = serv;
                        CFRelease(displayInfo);
                        break;
                    }
                }

                CFRelease(displayInfo);
                IOObjectRelease(serv);
            }
            IOObjectRelease(iter);
            return servicePort;
        }

        // Get integer property from registry entry.
        uint32_t GetEntryProperty(io_registry_entry_t entry, CFStringRef name) {
            uint32_t value = 0;

            // Recursively search registry entry and its parents for property name
            // The data should release with CFRelease
            CFDataRef data = static_cast<CFDataRef>(IORegistryEntrySearchCFProperty(
                entry, kIOServicePlane, name, kCFAllocatorDefault,
                kIORegistryIterateRecursively | kIORegistryIterateParents));

            if (data != nullptr) {
                const uint32_t* valuePtr =
                    reinterpret_cast<const uint32_t*>(CFDataGetBytePtr(data));
                if (valuePtr) {
                    value = *valuePtr;
                }

                CFRelease(data);
            }

            return value;
        }

        bool IsMetalSupported() {
            // Metal was first introduced in macOS 10.11
            NSOperatingSystemVersion macOS10_11 = {10, 11, 0};
            return [NSProcessInfo.processInfo isOperatingSystemAtLeastVersion:macOS10_11];
        }
    }  // anonymous namespace

    // The Metal backend's Adapter.

    class Adapter : public AdapterBase {
      public:
        Adapter(InstanceBase* instance, id<MTLDevice> device)
            : AdapterBase(instance, BackendType::Metal), mDevice([device retain]) {
            mPCIInfo.name = std::string([mDevice.name UTF8String]);
            // Gather the PCI device and vendor IDs based on which device is rendering to the
            // main display. This is obviously wrong for systems with multiple devices.
            // TODO(cwallez@chromium.org): Once Chromium has the macOS 10.13 SDK rolled, we
            // should use MTLDevice.registryID to gather the information.
            io_registry_entry_t entry = GetDisplayIOServicePort();
            if (entry != IO_OBJECT_NULL) {
                mPCIInfo.vendorId = GetEntryProperty(entry, CFSTR("vendor-id"));
                mPCIInfo.deviceId = GetEntryProperty(entry, CFSTR("device-id"));
                IOObjectRelease(entry);
            }

            if ([device isLowPower]) {
                mDeviceType = DeviceType::IntegratedGPU;
            } else {
                mDeviceType = DeviceType::DiscreteGPU;
            }
        }

        ~Adapter() override {
            [mDevice release];
        }

      private:
        ResultOrError<DeviceBase*> CreateDeviceImpl(const DeviceDescriptor* descriptor) override {
            return {new Device(this, mDevice, descriptor)};
        }

        id<MTLDevice> mDevice = nil;
    };

    // Implementation of the Metal backend's BackendConnection

    Backend::Backend(InstanceBase* instance) : BackendConnection(instance, BackendType::Metal) {
        if (GetInstance()->IsBackendValidationEnabled()) {
            setenv("METAL_DEVICE_WRAPPER_TYPE", "1", 1);
        }
    }

    std::vector<std::unique_ptr<AdapterBase>> Backend::DiscoverDefaultAdapters() {
        NSArray<id<MTLDevice>>* devices = MTLCopyAllDevices();

        std::vector<std::unique_ptr<AdapterBase>> adapters;
        for (id<MTLDevice> device in devices) {
            adapters.push_back(std::make_unique<Adapter>(GetInstance(), device));
        }

        [devices release];
        return adapters;
    }

    BackendConnection* Connect(InstanceBase* instance) {
        if (!IsMetalSupported()) {
            return nullptr;
        }
        return new Backend(instance);
    }

}}  // namespace dawn_native::metal
