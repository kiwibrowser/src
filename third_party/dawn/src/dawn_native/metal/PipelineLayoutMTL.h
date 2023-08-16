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

#ifndef DAWNNATIVE_METAL_PIPELINELAYOUTMTL_H_
#define DAWNNATIVE_METAL_PIPELINELAYOUTMTL_H_

#include "common/ityp_stack_vec.h"
#include "dawn_native/BindingInfo.h"
#include "dawn_native/PipelineLayout.h"

#include "dawn_native/PerStage.h"

#import <Metal/Metal.h>

namespace spirv_cross {
    class CompilerMSL;
}

namespace dawn_native { namespace metal {

    class Device;

    // The number of Metal buffers usable by applications in general
    static constexpr size_t kMetalBufferTableSize = 31;
    // The Metal buffer slot that Dawn reserves for its own use to pass more data to shaders
    static constexpr size_t kBufferLengthBufferSlot = kMetalBufferTableSize - 1;
    // The number of Metal buffers Dawn can use in a generic way (i.e. that aren't reserved)
    static constexpr size_t kGenericMetalBufferSlots = kMetalBufferTableSize - 1;

    class PipelineLayout final : public PipelineLayoutBase {
      public:
        PipelineLayout(Device* device, const PipelineLayoutDescriptor* descriptor);

        using BindingIndexInfo =
            ityp::array<BindGroupIndex,
                        ityp::stack_vec<BindingIndex, uint32_t, kMaxOptimalBindingsPerGroup>,
                        kMaxBindGroups>;
        const BindingIndexInfo& GetBindingIndexInfo(SingleShaderStage stage) const;

        // The number of Metal vertex stage buffers used for the whole pipeline layout.
        uint32_t GetBufferBindingCount(SingleShaderStage stage);

      private:
        ~PipelineLayout() override = default;
        PerStage<BindingIndexInfo> mIndexInfo;
        PerStage<uint32_t> mBufferBindingCount;
    };

}}  // namespace dawn_native::metal

#endif  // DAWNNATIVE_METAL_PIPELINELAYOUTMTL_H_
