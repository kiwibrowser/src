// Copyright 2020 The Dawn Authors
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
#ifndef DAWNNATIVE_METAL_COMMANDRECORDINGCONTEXT_H_
#define DAWNNATIVE_METAL_COMMANDRECORDINGCONTEXT_H_

#import <Metal/Metal.h>

namespace dawn_native { namespace metal {

    // This class wraps a MTLCommandBuffer and tracks which Metal encoder is open.
    // Only one encoder may be open at a time.
    class CommandRecordingContext {
      public:
        CommandRecordingContext();
        CommandRecordingContext(id<MTLCommandBuffer> commands);

        CommandRecordingContext(const CommandRecordingContext& rhs) = delete;
        CommandRecordingContext& operator=(const CommandRecordingContext& rhs) = delete;

        CommandRecordingContext(CommandRecordingContext&& rhs);
        CommandRecordingContext& operator=(CommandRecordingContext&& rhs);

        ~CommandRecordingContext();

        id<MTLCommandBuffer> GetCommands();

        id<MTLCommandBuffer> AcquireCommands();

        id<MTLBlitCommandEncoder> EnsureBlit();
        void EndBlit();

        id<MTLComputeCommandEncoder> BeginCompute();
        void EndCompute();

        id<MTLRenderCommandEncoder> BeginRender(MTLRenderPassDescriptor* descriptor);
        void EndRender();

      private:
        id<MTLCommandBuffer> mCommands = nil;
        id<MTLBlitCommandEncoder> mBlit = nil;
        id<MTLComputeCommandEncoder> mCompute = nil;
        id<MTLRenderCommandEncoder> mRender = nil;
        bool mInEncoder = false;
    };

}}  // namespace dawn_native::metal

#endif  // DAWNNATIVE_METAL_COMMANDRECORDINGCONTEXT_H_
