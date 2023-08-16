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
#include "dawn_native/Instance.h"
#include "dawn_native/d3d12/BackendD3D12.h"
#include "dawn_native/d3d12/D3D12Error.h"
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

    Adapter::Adapter(Backend* backend, ComPtr<IDXGIAdapter3> hardwareAdapter)
        : AdapterBase(backend->GetInstance(), wgpu::BackendType::D3D12),
          mHardwareAdapter(hardwareAdapter),
          mBackend(backend) {
    }

    Adapter::~Adapter() {
        CleanUpDebugLayerFilters();
    }

    const D3D12DeviceInfo& Adapter::GetDeviceInfo() const {
        return mDeviceInfo;
    }

    IDXGIAdapter3* Adapter::GetHardwareAdapter() const {
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
            return DAWN_INTERNAL_ERROR("D3D12CreateDevice failed");
        }

        DAWN_TRY(InitializeDebugLayerFilters());

        DXGI_ADAPTER_DESC1 adapterDesc;
        mHardwareAdapter->GetDesc1(&adapterDesc);

        mPCIInfo.deviceId = adapterDesc.DeviceId;
        mPCIInfo.vendorId = adapterDesc.VendorId;

        DAWN_TRY_ASSIGN(mDeviceInfo, GatherDeviceInfo(*this));

        if (adapterDesc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
            mAdapterType = wgpu::AdapterType::CPU;
        } else {
            mAdapterType = (mDeviceInfo.isUMA) ? wgpu::AdapterType::IntegratedGPU
                                               : wgpu::AdapterType::DiscreteGPU;
        }

        std::wstring_convert<DeletableFacet<std::codecvt<wchar_t, char, std::mbstate_t>>> converter(
            "Error converting");
        mPCIInfo.name = converter.to_bytes(adapterDesc.Description);

        InitializeSupportedExtensions();

        return {};
    }

    void Adapter::InitializeSupportedExtensions() {
        mSupportedExtensions.EnableExtension(Extension::TextureCompressionBC);
        mSupportedExtensions.EnableExtension(Extension::PipelineStatisticsQuery);
        mSupportedExtensions.EnableExtension(Extension::TimestampQuery);
        if (mDeviceInfo.supportsShaderFloat16 && GetBackend()->GetFunctions()->IsDXCAvailable()) {
            mSupportedExtensions.EnableExtension(Extension::ShaderFloat16);
        }
    }

    MaybeError Adapter::InitializeDebugLayerFilters() {
        if (!GetInstance()->IsBackendValidationEnabled()) {
            return {};
        }

        D3D12_MESSAGE_ID denyIds[] = {

            //
            // Permanent IDs: list of warnings that are not applicable
            //

            // Resource sub-allocation partially maps pre-allocated heaps. This means the
            // entire physical addresses space may have no resources or have many resources
            // assigned the same heap.
            D3D12_MESSAGE_ID_HEAP_ADDRESS_RANGE_HAS_NO_RESOURCE,
            D3D12_MESSAGE_ID_HEAP_ADDRESS_RANGE_INTERSECTS_MULTIPLE_BUFFERS,

            // The debug layer validates pipeline objects when they are created. Dawn validates
            // them when them when they are set. Therefore, since the issue is caught at a later
            // time, we can silence this warnings.
            D3D12_MESSAGE_ID_CREATEGRAPHICSPIPELINESTATE_RENDERTARGETVIEW_NOT_SET,

            // Adding a clear color during resource creation would require heuristics or delayed
            // creation.
            // https://crbug.com/dawn/418
            D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,
            D3D12_MESSAGE_ID_CLEARDEPTHSTENCILVIEW_MISMATCHINGCLEARVALUE,

            // Dawn enforces proper Unmaps at a later time.
            // https://crbug.com/dawn/422
            D3D12_MESSAGE_ID_EXECUTECOMMANDLISTS_GPU_WRITTEN_READBACK_RESOURCE_MAPPED,

            //
            // Temporary IDs: list of warnings that should be fixed or promoted
            //

            // Remove after warning have been addressed
            // https://crbug.com/dawn/421
            D3D12_MESSAGE_ID_GPU_BASED_VALIDATION_INCOMPATIBLE_RESOURCE_STATE,
        };

        // Create a retrieval filter with a deny list to suppress messages.
        // Any messages remaining will be converted to Dawn errors.
        D3D12_INFO_QUEUE_FILTER filter{};
        // Filter out info/message and only create errors from warnings or worse.
        D3D12_MESSAGE_SEVERITY severities[] = {
            D3D12_MESSAGE_SEVERITY_INFO,
            D3D12_MESSAGE_SEVERITY_MESSAGE,
        };
        filter.DenyList.NumSeverities = ARRAYSIZE(severities);
        filter.DenyList.pSeverityList = severities;
        filter.DenyList.NumIDs = ARRAYSIZE(denyIds);
        filter.DenyList.pIDList = denyIds;

        ComPtr<ID3D12InfoQueue> infoQueue;
        ASSERT_SUCCESS(mD3d12Device.As(&infoQueue));

        DAWN_TRY(CheckHRESULT(infoQueue->PushRetrievalFilter(&filter),
                              "ID3D12InfoQueue::PushRetrievalFilter"));

        return {};
    }

    void Adapter::CleanUpDebugLayerFilters() {
        if (!GetInstance()->IsBackendValidationEnabled()) {
            return;
        }
        ComPtr<ID3D12InfoQueue> infoQueue;
        ASSERT_SUCCESS(mD3d12Device.As(&infoQueue));
        infoQueue->PopRetrievalFilter();
    }

    ResultOrError<DeviceBase*> Adapter::CreateDeviceImpl(const DeviceDescriptor* descriptor) {
        return Device::Create(this, descriptor);
    }

}}  // namespace dawn_native::d3d12
