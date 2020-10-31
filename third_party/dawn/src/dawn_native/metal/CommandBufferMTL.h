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

#ifndef DAWNNATIVE_METAL_COMMANDBUFFERMTL_H_
#define DAWNNATIVE_METAL_COMMANDBUFFERMTL_H_

#include "dawn_native/CommandAllocator.h"
#include "dawn_native/CommandBuffer.h"
#include "dawn_native/Error.h"

#import <Metal/Metal.h>

namespace dawn_native {
    class CommandEncoder;
}

namespace dawn_native { namespace metal {

    class CommandRecordingContext;
    class Device;

    class CommandBuffer final : public CommandBufferBase {
      public:
        CommandBuffer(CommandEncoder* encoder, const CommandBufferDescriptor* descriptor);

        MaybeError FillCommands(CommandRecordingContext* commandContext);

      private:
        ~CommandBuffer() override;
        MaybeError EncodeComputePass(CommandRecordingContext* commandContext);
        MaybeError EncodeRenderPass(CommandRecordingContext* commandContext,
                                    MTLRenderPassDescriptor* mtlRenderPass,
                                    uint32_t width,
                                    uint32_t height);

        MaybeError EncodeRenderPassInternal(CommandRecordingContext* commandContext,
                                            MTLRenderPassDescriptor* mtlRenderPass,
                                            uint32_t width,
                                            uint32_t height);

        CommandIterator mCommands;
    };

}}  // namespace dawn_native::metal

#endif  // DAWNNATIVE_METAL_COMMANDBUFFERMTL_H_
