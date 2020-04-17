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

#ifndef DAWNNATIVE_COMMANDENCODER_H_
#define DAWNNATIVE_COMMANDENCODER_H_

#include "dawn_native/dawn_platform.h"

#include "dawn_native/CommandAllocator.h"
#include "dawn_native/Error.h"
#include "dawn_native/ObjectBase.h"
#include "dawn_native/PassResourceUsage.h"

#include <string>

namespace dawn_native {

    struct BeginRenderPassCmd;

    class CommandEncoderBase : public ObjectBase {
      public:
        CommandEncoderBase(DeviceBase* device);
        ~CommandEncoderBase();

        CommandIterator AcquireCommands();
        CommandBufferResourceUsage AcquireResourceUsages();

        // Dawn API
        ComputePassEncoderBase* BeginComputePass();
        RenderPassEncoderBase* BeginRenderPass(const RenderPassDescriptor* info);
        void CopyBufferToBuffer(BufferBase* source,
                                uint64_t sourceOffset,
                                BufferBase* destination,
                                uint64_t destinationOffset,
                                uint64_t size);
        void CopyBufferToTexture(const BufferCopyView* source,
                                 const TextureCopyView* destination,
                                 const Extent3D* copySize);
        void CopyTextureToBuffer(const TextureCopyView* source,
                                 const BufferCopyView* destination,
                                 const Extent3D* copySize);
        void CopyTextureToTexture(const TextureCopyView* source,
                                  const TextureCopyView* destination,
                                  const Extent3D* copySize);
        CommandBufferBase* Finish();

        // Functions to interact with the encoders
        void HandleError(const char* message);
        void ConsumeError(ErrorData* error);
        bool ConsumedError(MaybeError maybeError) {
            if (DAWN_UNLIKELY(maybeError.IsError())) {
                ConsumeError(maybeError.AcquireError());
                return true;
            }
            return false;
        }

        void PassEnded();

      private:
        MaybeError ValidateFinish();
        MaybeError ValidateComputePass();
        MaybeError ValidateRenderPass(BeginRenderPassCmd* renderPass);
        MaybeError ValidateCanRecordTopLevelCommands() const;

        enum class EncodingState : uint8_t;
        EncodingState mEncodingState;

        void MoveToIterator();
        CommandAllocator mAllocator;
        CommandIterator mIterator;
        bool mWasMovedToIterator = false;
        bool mWereCommandsAcquired = false;

        bool mWereResourceUsagesAcquired = false;
        CommandBufferResourceUsage mResourceUsages;

        unsigned int mDebugGroupStackSize = 0;

        bool mGotError = false;
        std::string mErrorMessage;
    };

}  // namespace dawn_native

#endif  // DAWNNATIVE_COMMANDENCODER_H_
