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

#ifndef DAWNNATIVE_METAL_SHADERMODULEMTL_H_
#define DAWNNATIVE_METAL_SHADERMODULEMTL_H_

#include "dawn_native/ShaderModule.h"

#import <Metal/Metal.h>

#include "dawn_native/Error.h"

namespace spirv_cross {
    class CompilerMSL;
}

namespace dawn_native { namespace metal {

    class Device;
    class PipelineLayout;

    class ShaderModule final : public ShaderModuleBase {
      public:
        static ResultOrError<ShaderModule*> Create(Device* device,
                                                   const ShaderModuleDescriptor* descriptor);

        struct MetalFunctionData {
            id<MTLFunction> function = nil;
            MTLSize localWorkgroupSize;
            bool needsStorageBufferLength;
            ~MetalFunctionData() {
                [function release];
            }
        };
        MaybeError GetFunction(const char* functionName,
                               SingleShaderStage functionStage,
                               const PipelineLayout* layout,
                               MetalFunctionData* out);

      private:
        ShaderModule(Device* device, const ShaderModuleDescriptor* descriptor);
        ~ShaderModule() override = default;
        MaybeError Initialize();

        shaderc_spvc::CompileOptions GetMSLCompileOptions();
    };

}}  // namespace dawn_native::metal

#endif  // DAWNNATIVE_METAL_SHADERMODULEMTL_H_
