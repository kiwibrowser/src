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

#include "common/GPUInfo.h"
#include "dawn_native/d3d12/AdapterD3D12.h"
#include "dawn_native/d3d12/BackendD3D12.h"
#include "dawn_native/d3d12/D3D12Error.h"
#include "dawn_native/d3d12/PlatformFunctions.h"

namespace dawn_native { namespace d3d12 {

    ResultOrError<D3D12DeviceInfo> GatherDeviceInfo(const Adapter& adapter) {
        D3D12DeviceInfo info = {};

        // Newer builds replace D3D_FEATURE_DATA_ARCHITECTURE with
        // D3D_FEATURE_DATA_ARCHITECTURE1. However, D3D_FEATURE_DATA_ARCHITECTURE can be used
        // for backwards compat.
        // https://docs.microsoft.com/en-us/windows/desktop/api/d3d12/ne-d3d12-d3d12_feature
        D3D12_FEATURE_DATA_ARCHITECTURE arch = {};
        DAWN_TRY(CheckHRESULT(adapter.GetDevice()->CheckFeatureSupport(D3D12_FEATURE_ARCHITECTURE,
                                                                       &arch, sizeof(arch)),
                              "ID3D12Device::CheckFeatureSupport"));

        info.isUMA = arch.UMA;

        D3D12_FEATURE_DATA_D3D12_OPTIONS options = {};
        DAWN_TRY(CheckHRESULT(adapter.GetDevice()->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS,
                                                                       &options, sizeof(options)),
                              "ID3D12Device::CheckFeatureSupport"));

        info.resourceHeapTier = options.ResourceHeapTier;

        // Windows builds 1809 and above can use the D3D12 render pass API. If we query
        // CheckFeatureSupport for D3D12_FEATURE_D3D12_OPTIONS5 successfully, then we can use
        // the render pass API.
        info.supportsRenderPass = false;
        D3D12_FEATURE_DATA_D3D12_OPTIONS5 featureOptions5 = {};
        if (SUCCEEDED(adapter.GetDevice()->CheckFeatureSupport(
                D3D12_FEATURE_D3D12_OPTIONS5, &featureOptions5, sizeof(featureOptions5)))) {
            // Performance regressions been observed when using a render pass on Intel graphics with
            // RENDER_PASS_TIER_1 available, so fall back to a software emulated render pass on
            // these platforms.
            if (featureOptions5.RenderPassesTier < D3D12_RENDER_PASS_TIER_1 ||
                !gpu_info::IsIntel(adapter.GetPCIInfo().vendorId)) {
                info.supportsRenderPass = true;
            }
        }

        D3D12_FEATURE_DATA_SHADER_MODEL knownShaderModels[] = {{D3D_SHADER_MODEL_6_2},
                                                               {D3D_SHADER_MODEL_6_1},
                                                               {D3D_SHADER_MODEL_6_0},
                                                               {D3D_SHADER_MODEL_5_1}};
        for (D3D12_FEATURE_DATA_SHADER_MODEL shaderModel : knownShaderModels) {
            if (SUCCEEDED(adapter.GetDevice()->CheckFeatureSupport(
                    D3D12_FEATURE_SHADER_MODEL, &shaderModel, sizeof(shaderModel)))) {
                if (shaderModel.HighestShaderModel < D3D_SHADER_MODEL_5_1) {
                    return DAWN_INTERNAL_ERROR(
                        "Driver could not support Shader Model 5.1 or higher");
                }

                switch (shaderModel.HighestShaderModel) {
                    case D3D_SHADER_MODEL_6_2: {
                        info.shaderModel = 62;
                        info.shaderProfiles[SingleShaderStage::Vertex] = L"vs_6_2";
                        info.shaderProfiles[SingleShaderStage::Fragment] = L"ps_6_2";
                        info.shaderProfiles[SingleShaderStage::Compute] = L"cs_6_2";

                        D3D12_FEATURE_DATA_D3D12_OPTIONS4 featureData4 = {};
                        if (SUCCEEDED(adapter.GetDevice()->CheckFeatureSupport(
                                D3D12_FEATURE_D3D12_OPTIONS4, &featureData4,
                                sizeof(featureData4)))) {
                            info.supportsShaderFloat16 =
                                shaderModel.HighestShaderModel >= D3D_SHADER_MODEL_6_2 &&
                                featureData4.Native16BitShaderOpsSupported;
                        }
                        break;
                    }
                    case D3D_SHADER_MODEL_6_1: {
                        info.shaderModel = 61;
                        info.shaderProfiles[SingleShaderStage::Vertex] = L"vs_6_1";
                        info.shaderProfiles[SingleShaderStage::Fragment] = L"ps_6_1";
                        info.shaderProfiles[SingleShaderStage::Compute] = L"cs_6_1";
                        break;
                    }
                    case D3D_SHADER_MODEL_6_0: {
                        info.shaderModel = 60;
                        info.shaderProfiles[SingleShaderStage::Vertex] = L"vs_6_0";
                        info.shaderProfiles[SingleShaderStage::Fragment] = L"ps_6_0";
                        info.shaderProfiles[SingleShaderStage::Compute] = L"cs_6_0";
                        break;
                    }
                    default: {
                        info.shaderModel = 51;
                        info.shaderProfiles[SingleShaderStage::Vertex] = L"vs_5_1";
                        info.shaderProfiles[SingleShaderStage::Fragment] = L"ps_5_1";
                        info.shaderProfiles[SingleShaderStage::Compute] = L"cs_5_1";
                        break;
                    }
                }

                // Successfully find the maximum supported shader model.
                break;
            }
        }

        return std::move(info);
    }
}}  // namespace dawn_native::d3d12
