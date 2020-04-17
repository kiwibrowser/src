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

#include "dawn_native/metal/ComputePipelineMTL.h"

#include "dawn_native/metal/DeviceMTL.h"
#include "dawn_native/metal/ShaderModuleMTL.h"

namespace dawn_native { namespace metal {

    ComputePipeline::ComputePipeline(Device* device, const ComputePipelineDescriptor* descriptor)
        : ComputePipelineBase(device, descriptor) {
        auto mtlDevice = ToBackend(GetDevice())->GetMTLDevice();

        const ShaderModule* computeModule = ToBackend(descriptor->computeStage->module);
        const char* computeEntryPoint = descriptor->computeStage->entryPoint;
        ShaderModule::MetalFunctionData computeData = computeModule->GetFunction(
            computeEntryPoint, dawn::ShaderStage::Compute, ToBackend(GetLayout()));

        NSError* error = nil;
        mMtlComputePipelineState =
            [mtlDevice newComputePipelineStateWithFunction:computeData.function error:&error];
        if (error != nil) {
            NSLog(@" error => %@", error);
            GetDevice()->HandleError("Error creating pipeline state");
            return;
        }

        // Copy over the local workgroup size as it is passed to dispatch explicitly in Metal
        mLocalWorkgroupSize = computeData.localWorkgroupSize;
    }

    ComputePipeline::~ComputePipeline() {
        [mMtlComputePipelineState release];
    }

    void ComputePipeline::Encode(id<MTLComputeCommandEncoder> encoder) {
        [encoder setComputePipelineState:mMtlComputePipelineState];
    }

    MTLSize ComputePipeline::GetLocalWorkGroupSize() const {
        return mLocalWorkgroupSize;
    }

}}  // namespace dawn_native::metal
