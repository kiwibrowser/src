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

#include "dawn_native/d3d12/ComputePipelineD3D12.h"

#include "common/Assert.h"
#include "dawn_native/d3d12/DeviceD3D12.h"
#include "dawn_native/d3d12/PipelineLayoutD3D12.h"
#include "dawn_native/d3d12/PlatformFunctions.h"
#include "dawn_native/d3d12/ShaderModuleD3D12.h"

namespace dawn_native { namespace d3d12 {

    ComputePipeline::ComputePipeline(Device* device, const ComputePipelineDescriptor* descriptor)
        : ComputePipelineBase(device, descriptor) {
        uint32_t compileFlags = 0;
#if defined(_DEBUG)
        // Enable better shader debugging with the graphics debugging tools.
        compileFlags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif
        // SPRIV-cross does matrix multiplication expecting row major matrices
        compileFlags |= D3DCOMPILE_PACK_MATRIX_ROW_MAJOR;

        const ShaderModule* module = ToBackend(descriptor->computeStage->module);
        const std::string& hlslSource = module->GetHLSLSource(ToBackend(GetLayout()));

        ComPtr<ID3DBlob> compiledShader;
        ComPtr<ID3DBlob> errors;

        const PlatformFunctions* functions = device->GetFunctions();
        if (FAILED(functions->d3dCompile(hlslSource.c_str(), hlslSource.length(), nullptr, nullptr,
                                         nullptr, descriptor->computeStage->entryPoint, "cs_5_1",
                                         compileFlags, 0, &compiledShader, &errors))) {
            printf("%s\n", reinterpret_cast<char*>(errors->GetBufferPointer()));
            ASSERT(false);
        }

        D3D12_COMPUTE_PIPELINE_STATE_DESC d3dDesc = {};
        d3dDesc.pRootSignature = ToBackend(GetLayout())->GetRootSignature().Get();
        d3dDesc.CS.pShaderBytecode = compiledShader->GetBufferPointer();
        d3dDesc.CS.BytecodeLength = compiledShader->GetBufferSize();

        device->GetD3D12Device()->CreateComputePipelineState(&d3dDesc,
                                                             IID_PPV_ARGS(&mPipelineState));
    }

    ComputePipeline::~ComputePipeline() {
        ToBackend(GetDevice())->ReferenceUntilUnused(mPipelineState);
    }

    ComPtr<ID3D12PipelineState> ComputePipeline::GetPipelineState() {
        return mPipelineState;
    }

}}  // namespace dawn_native::d3d12
