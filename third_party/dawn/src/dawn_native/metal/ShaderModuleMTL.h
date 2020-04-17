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

namespace spirv_cross {
    class CompilerMSL;
}

namespace dawn_native { namespace metal {

    class Device;
    class PipelineLayout;

    class ShaderModule : public ShaderModuleBase {
      public:
        ShaderModule(Device* device, const ShaderModuleDescriptor* descriptor);

        struct MetalFunctionData {
            id<MTLFunction> function;
            MTLSize localWorkgroupSize;
            ~MetalFunctionData() {
                [function release];
            }
        };
        MetalFunctionData GetFunction(const char* functionName,
                                      dawn::ShaderStage functionStage,
                                      const PipelineLayout* layout) const;

      private:
        // Calling compile on CompilerMSL somehow changes internal state that makes subsequent
        // compiles return invalid MSL. We keep the spirv around and recreate the compiler everytime
        // we need to use it.
        std::vector<uint32_t> mSpirv;
    };

}}  // namespace dawn_native::metal

#endif  // DAWNNATIVE_METAL_SHADERMODULEMTL_H_
