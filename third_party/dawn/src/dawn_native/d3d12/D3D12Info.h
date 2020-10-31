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

#ifndef DAWNNATIVE_D3D12_D3D12INFO_H_
#define DAWNNATIVE_D3D12_D3D12INFO_H_

#include "dawn_native/Error.h"
#include "dawn_native/PerStage.h"
#include "dawn_native/d3d12/d3d12_platform.h"

namespace dawn_native { namespace d3d12 {

    class Adapter;

    struct D3D12DeviceInfo {
        bool isUMA;
        uint32_t resourceHeapTier;
        bool supportsRenderPass;
        bool supportsShaderFloat16;
        // shaderModel indicates the maximum supported shader model, for example, the value 62
        // indicates that current driver supports the maximum shader model is D3D_SHADER_MODEL_6_2.
        uint32_t shaderModel;
        PerStage<std::wstring> shaderProfiles;
    };

    ResultOrError<D3D12DeviceInfo> GatherDeviceInfo(const Adapter& adapter);
}}  // namespace dawn_native::d3d12

#endif  // DAWNNATIVE_D3D12_D3D12INFO_H_
