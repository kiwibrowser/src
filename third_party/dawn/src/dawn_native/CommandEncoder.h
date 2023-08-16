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

#include "dawn_native/EncodingContext.h"
#include "dawn_native/Error.h"
#include "dawn_native/ObjectBase.h"
#include "dawn_native/PassResourceUsage.h"

#include <string>

namespace dawn_native {

    struct BeginRenderPassCmd;

    class CommandEncoder final : public ObjectBase {
      public:
        CommandEncoder(DeviceBase* device, const CommandEncoderDescriptor* descriptor);

        CommandIterator AcquireCommands();
        CommandBufferResourceUsage AcquireResourceUsages();

        void TrackUsedQuerySet(QuerySetBase* querySet);

        // Dawn API
        ComputePassEncoder* BeginComputePass(const ComputePassDescriptor* descriptor);
        RenderPassEncoder* BeginRenderPass(const RenderPassDescriptor* descriptor);

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

        void InsertDebugMarker(const char* groupLabel);
        void PopDebugGroup();
        void PushDebugGroup(const char* groupLabel);

        void ResolveQuerySet(QuerySetBase* querySet,
                             uint32_t firstQuery,
                             uint32_t queryCount,
                             BufferBase* destination,
                             uint64_t destinationOffset);
        void WriteTimestamp(QuerySetBase* querySet, uint32_t queryIndex);

        CommandBufferBase* Finish(const CommandBufferDescriptor* descriptor);

      private:
        MaybeError ValidateFinish(CommandIterator* commands,
                                  const PerPassUsages& perPassUsages) const;

        EncodingContext mEncodingContext;
        std::set<BufferBase*> mTopLevelBuffers;
        std::set<TextureBase*> mTopLevelTextures;
        std::set<QuerySetBase*> mUsedQuerySets;
    };

}  // namespace dawn_native

#endif  // DAWNNATIVE_COMMANDENCODER_H_
