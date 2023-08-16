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

#include "dawn_native/Adapter.h"

#include "dawn_native/Instance.h"

namespace dawn_native {

    AdapterBase::AdapterBase(InstanceBase* instance, wgpu::BackendType backend)
        : mInstance(instance), mBackend(backend) {
    }

    wgpu::BackendType AdapterBase::GetBackendType() const {
        return mBackend;
    }

    wgpu::AdapterType AdapterBase::GetAdapterType() const {
        return mAdapterType;
    }

    const PCIInfo& AdapterBase::GetPCIInfo() const {
        return mPCIInfo;
    }

    InstanceBase* AdapterBase::GetInstance() const {
        return mInstance;
    }

    ExtensionsSet AdapterBase::GetSupportedExtensions() const {
        return mSupportedExtensions;
    }

    bool AdapterBase::SupportsAllRequestedExtensions(
        const std::vector<const char*>& requestedExtensions) const {
        for (const char* extensionStr : requestedExtensions) {
            Extension extensionEnum = mInstance->ExtensionNameToEnum(extensionStr);
            if (extensionEnum == Extension::InvalidEnum) {
                return false;
            }
            if (!mSupportedExtensions.IsEnabled(extensionEnum)) {
                return false;
            }
        }
        return true;
    }

    WGPUDeviceProperties AdapterBase::GetAdapterProperties() const {
        WGPUDeviceProperties adapterProperties = {};

        mSupportedExtensions.InitializeDeviceProperties(&adapterProperties);
        return adapterProperties;
    }

    DeviceBase* AdapterBase::CreateDevice(const DeviceDescriptor* descriptor) {
        DeviceBase* result = nullptr;

        if (mInstance->ConsumedError(CreateDeviceInternal(&result, descriptor))) {
            return nullptr;
        }

        return result;
    }

    MaybeError AdapterBase::CreateDeviceInternal(DeviceBase** result,
                                                 const DeviceDescriptor* descriptor) {
        if (descriptor != nullptr) {
            if (!SupportsAllRequestedExtensions(descriptor->requiredExtensions)) {
                return DAWN_VALIDATION_ERROR("One or more requested extensions are not supported");
            }
        }

        // TODO(cwallez@chromium.org): This will eventually have validation that the device
        // descriptor is valid and is a subset what's allowed on this adapter.
        DAWN_TRY_ASSIGN(*result, CreateDeviceImpl(descriptor));
        return {};
    }

}  // namespace dawn_native
