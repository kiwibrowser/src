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

#include "dawn_native/d3d12/AdapterD3D12.h"

#include "common/Constants.h"
#include "dawn_native/d3d12/BackendD3D12.h"
#include "dawn_native/d3d12/DeviceD3D12.h"
#include "dawn_native/d3d12/PlatformFunctions.h"

#include <locale>

namespace dawn_native { namespace d3d12 {

    // utility wrapper to adapt locale-bound facets for wstring/wbuffer convert
    template <class Facet>
    struct DeletableFacet : Facet {
        template <class... Args>
        DeletableFacet(Args&&... args) : Facet(std::forward<Args>(args)...) {
        }

        ~DeletableFacet() {
        }
    };

    Adapter::Adapter(Backend* backend, ComPtr<IDXGIAdapter1> hardwareAdapter)
        : AdapterBase(backend->GetInstance(), BackendType::D3D12),
          mHardwareAdapter(hardwareAdapter),
          mBackend(backend) {
    }

    const D3D12DeviceInfo& Adapter::GetDeviceInfo() const {
        return mDeviceInfo;
    }

    IDXGIAdapter1* Adapter::GetHardwareAdapter() const {
        return mHardwareAdapter.Get();
    }

    Backend* Adapter::GetBackend() const {
        return mBackend;
    }

    ComPtr<ID3D12Device> Adapter::GetDevice() const {
        return mD3d12Device;
    }

    MaybeError Adapter::Initialize() {
        // D3D12 cannot check for feature support without a device.
        // Create the device to populate the adapter properties then reuse it when needed for actual
        // rendering.
        const PlatformFunctions* functions = GetBackend()->GetFunctions();
        if (FAILED(functions->d3d12CreateDevice(GetHardwareAdapter(), D3D_FEATURE_LEVEL_11_0,
                                                _uuidof(ID3D12Device), &mD3d12Device))) {
            return DAWN_CONTEXT_LOST_ERROR("D3D12CreateDevice failed");
        }

        DAWN_TRY_ASSIGN(mDeviceInfo, GatherDeviceInfo(*this));

        DXGI_ADAPTER_DESC1 adapterDesc;
        mHardwareAdapter->GetDesc1(&adapterDesc);

        mPCIInfo.deviceId = adapterDesc.DeviceId;
        mPCIInfo.vendorId = adapterDesc.VendorId;

        if (adapterDesc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
            mDeviceType = DeviceType::CPU;
        } else {
            mDeviceType = (mDeviceInfo.isUMA) ? DeviceType::IntegratedGPU : DeviceType::DiscreteGPU;
        }

        std::wstring_convert<DeletableFacet<std::codecvt<wchar_t, char, std::mbstate_t>>> converter(
            "Error converting");
        mPCIInfo.name = converter.to_bytes(adapterDesc.Description);

        return {};
    }

    ResultOrError<DeviceBase*> Adapter::CreateDeviceImpl(const DeviceDescriptor* descriptor) {
        std::unique_ptr<Device> device = std::make_unique<Device>(this, descriptor);
        DAWN_TRY(device->Initialize());
        return device.release();
    }

}}  // namespace dawn_native::d3d12
