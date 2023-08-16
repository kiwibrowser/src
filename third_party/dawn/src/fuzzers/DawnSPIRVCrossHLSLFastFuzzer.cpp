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

#include <cstdint>
#include <string>
#include <vector>

#include "DawnSPIRVCrossFuzzer.h"

namespace {

    int FuzzTask(const std::vector<uint32_t>& input) {
        shaderc_spvc::Context context;
        if (!context.IsValid()) {
            return 0;
        }

        DawnSPIRVCrossFuzzer::ExecuteWithSignalTrap([&context, &input]() {
            shaderc_spvc::CompilationResult result;
            shaderc_spvc::CompileOptions options;
            options.SetSourceEnvironment(shaderc_target_env_webgpu, shaderc_env_version_webgpu);
            options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_1);

            // Using the options that are used by Dawn, they appear in ShaderModuleD3D12.cpp
            options.SetForceZeroInitializedVariables(true);
            options.SetHLSLShaderModel(51);
            // TODO (hao.x.li@intel.com): The HLSLPointCoordCompat and HLSLPointSizeCompat are
            // required temporarily for https://bugs.chromium.org/p/dawn/issues/detail?id=146,
            // but should be removed once WebGPU requires there is no gl_PointSize builtin.
            // See https://github.com/gpuweb/gpuweb/issues/332
            options.SetHLSLPointCoordCompat(true);
            options.SetHLSLPointSizeCompat(true);
            if (context.InitializeForHlsl(input.data(), input.size(), options) ==
                shaderc_spvc_status_success) {
                context.CompileShader(&result);
            }
        });

        return 0;
    }

}  // namespace

extern "C" int LLVMFuzzerInitialize(int* argc, char*** argv) {
    return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    return DawnSPIRVCrossFuzzer::Run(data, size, FuzzTask);
}
