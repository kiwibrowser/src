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

    AdapterBase::AdapterBase(InstanceBase* instance, BackendType backend)
        : mInstance(instance), mBackend(backend) {
    }

    BackendType AdapterBase::GetBackendType() const {
        return mBackend;
    }

    DeviceType AdapterBase::GetDeviceType() const {
        return mDeviceType;
    }

    const PCIInfo& AdapterBase::GetPCIInfo() const {
        return mPCIInfo;
    }

    InstanceBase* AdapterBase::GetInstance() const {
        return mInstance;
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
        // TODO(cwallez@chromium.org): This will eventually have validation that the device
        // descriptor is valid and is a subset what's allowed on this adapter.
        DAWN_TRY_ASSIGN(*result, CreateDeviceImpl(descriptor));
        return {};
    }

}  // namespace dawn_native
