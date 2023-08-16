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

#ifndef DAWNNATIVE_COMMANDVALIDATION_H_
#define DAWNNATIVE_COMMANDVALIDATION_H_

#include "dawn_native/CommandAllocator.h"
#include "dawn_native/Error.h"
#include "dawn_native/Texture.h"

#include <vector>

namespace dawn_native {

    class AttachmentState;
    class QuerySetBase;
    struct BeginRenderPassCmd;
    struct PassResourceUsage;

    MaybeError ValidateCanPopDebugGroup(uint64_t debugGroupStackSize);
    MaybeError ValidateFinalDebugGroupStackSize(uint64_t debugGroupStackSize);

    MaybeError ValidateRenderBundle(CommandIterator* commands,
                                    const AttachmentState* attachmentState);
    MaybeError ValidateRenderPass(CommandIterator* commands, const BeginRenderPassCmd* renderPass);
    MaybeError ValidateComputePass(CommandIterator* commands);

    MaybeError ValidatePassResourceUsage(const PassResourceUsage& usage);

    MaybeError ValidateTimestampQuery(QuerySetBase* querySet, uint32_t queryIndex);

    uint32_t ComputeRequiredBytesInCopy(const Format& textureFormat,
                                        const Extent3D& copySize,
                                        uint32_t bytesPerRow,
                                        uint32_t rowsPerImage);

    MaybeError ValidateLinearTextureData(const TextureDataLayout& layout,
                                         uint64_t byteSize,
                                         const Format& format,
                                         const Extent3D& copyExtent);
    MaybeError ValidateTextureCopyRange(const TextureCopyView& textureCopyView,
                                        const Extent3D& copySize);

    MaybeError ValidateBufferCopyView(DeviceBase const* device,
                                      const BufferCopyView& bufferCopyView);
    MaybeError ValidateTextureCopyView(DeviceBase const* device,
                                       const TextureCopyView& textureCopyView);

    MaybeError ValidateRowsPerImage(const Format& format,
                                    uint32_t rowsPerImage,
                                    uint32_t copyHeight);
    MaybeError ValidateBytesPerRow(const Format& format,
                                   const Extent3D& copySize,
                                   uint32_t bytesPerRow);
    MaybeError ValidateCopySizeFitsInBuffer(const Ref<BufferBase>& buffer,
                                            uint64_t offset,
                                            uint64_t size);

    bool IsRangeOverlapped(uint32_t startA, uint32_t startB, uint32_t length);

}  // namespace dawn_native

#endif  // DAWNNATIVE_COMMANDVALIDATION_H_
