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

#ifndef DAWNNATIVE_ADAPTER_H_
#define DAWNNATIVE_ADAPTER_H_

#include "dawn_native/DawnNative.h"

#include "dawn_native/Error.h"
#include "dawn_native/Extensions.h"
#include "dawn_native/dawn_platform.h"

#include <string>

namespace dawn_native {

    class DeviceBase;

    class AdapterBase {
      public:
        AdapterBase(InstanceBase* instance, wgpu::BackendType backend);
        virtual ~AdapterBase() = default;

        wgpu::BackendType GetBackendType() const;
        wgpu::AdapterType GetAdapterType() const;
        const PCIInfo& GetPCIInfo() const;
        InstanceBase* GetInstance() const;

        DeviceBase* CreateDevice(const DeviceDescriptor* descriptor = nullptr);

        ExtensionsSet GetSupportedExtensions() const;
        bool SupportsAllRequestedExtensions(
            const std::vector<const char*>& requestedExtensions) const;
        WGPUDeviceProperties GetAdapterProperties() const;

      protected:
        PCIInfo mPCIInfo = {};
        wgpu::AdapterType mAdapterType = wgpu::AdapterType::Unknown;
        ExtensionsSet mSupportedExtensions;

      private:
        virtual ResultOrError<DeviceBase*> CreateDeviceImpl(const DeviceDescriptor* descriptor) = 0;

        MaybeError CreateDeviceInternal(DeviceBase** result, const DeviceDescriptor* descriptor);

        InstanceBase* mInstance = nullptr;
        wgpu::BackendType mBackend;
    };

}  // namespace dawn_native

#endif  // DAWNNATIVE_ADAPTER_H_
