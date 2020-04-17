// Copyright 2017 The Dawn Authors
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

#include "dawn_native/d3d12/ShaderModuleD3D12.h"

#include "common/Assert.h"
#include "common/BitSetIterator.h"
#include "dawn_native/d3d12/BindGroupLayoutD3D12.h"
#include "dawn_native/d3d12/DeviceD3D12.h"
#include "dawn_native/d3d12/PipelineLayoutD3D12.h"

#include <spirv-cross/spirv_hlsl.hpp>

namespace dawn_native { namespace d3d12 {

    ShaderModule::ShaderModule(Device* device, const ShaderModuleDescriptor* descriptor)
        : ShaderModuleBase(device, descriptor) {
        mSpirv.assign(descriptor->code, descriptor->code + descriptor->codeSize);
        spirv_cross::CompilerHLSL compiler(mSpirv);
        ExtractSpirvInfo(compiler);
    }

    const std::string ShaderModule::GetHLSLSource(PipelineLayout* layout) const {
        spirv_cross::CompilerHLSL compiler(mSpirv);

        // If these options are changed, the values in DawnSPIRVCrossHLSLFastFuzzer.cpp need to be
        // updated.
        spirv_cross::CompilerGLSL::Options options_glsl;
        options_glsl.vertex.flip_vert_y = true;
        compiler.set_common_options(options_glsl);

        spirv_cross::CompilerHLSL::Options options_hlsl;
        options_hlsl.shader_model = 51;
        compiler.set_hlsl_options(options_hlsl);

        const ModuleBindingInfo& moduleBindingInfo = GetBindingInfo();
        for (uint32_t group : IterateBitSet(layout->GetBindGroupLayoutsMask())) {
            const auto& bindingOffsets =
                ToBackend(layout->GetBindGroupLayout(group))->GetBindingOffsets();
            const auto& groupBindingInfo = moduleBindingInfo[group];
            for (uint32_t binding = 0; binding < groupBindingInfo.size(); ++binding) {
                const BindingInfo& bindingInfo = groupBindingInfo[binding];
                if (bindingInfo.used) {
                    uint32_t bindingOffset = bindingOffsets[binding];
                    compiler.set_decoration(bindingInfo.id, spv::DecorationBinding, bindingOffset);
                }
            }
        }
        return compiler.compile();
    }

}}  // namespace dawn_native::d3d12
