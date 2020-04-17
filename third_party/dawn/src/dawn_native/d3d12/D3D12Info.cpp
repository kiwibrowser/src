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

#include "dawn_native/d3d12/D3D12Info.h"

#include "dawn_native/D3D12/AdapterD3D12.h"
#include "dawn_native/D3D12/BackendD3D12.h"

#include "dawn_native/d3d12/PlatformFunctions.h"

namespace dawn_native { namespace d3d12 {

    ResultOrError<D3D12DeviceInfo> GatherDeviceInfo(const Adapter& adapter) {
        D3D12DeviceInfo info = {};

        // Gather info about device memory
        {
            // Newer builds replace D3D_FEATURE_DATA_ARCHITECTURE with
            // D3D_FEATURE_DATA_ARCHITECTURE1. However, D3D_FEATURE_DATA_ARCHITECTURE can be used
            // for backwards compat.
            // https://docs.microsoft.com/en-us/windows/desktop/api/d3d12/ne-d3d12-d3d12_feature
            D3D12_FEATURE_DATA_ARCHITECTURE arch = {};
            if (FAILED(adapter.GetDevice()->CheckFeatureSupport(D3D12_FEATURE_ARCHITECTURE, &arch,
                                                                sizeof(arch)))) {
                return DAWN_CONTEXT_LOST_ERROR("CheckFeatureSupport failed");
            }

            info.isUMA = arch.UMA;
        }

        return info;
    }
}}  // namespace dawn_native::d3d12