// Copyright 2018 The Dawn Authors
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

#include "dawn_native/RenderPassEncoder.h"

#include "common/Constants.h"
#include "dawn_native/Buffer.h"
#include "dawn_native/CommandEncoder.h"
#include "dawn_native/Commands.h"
#include "dawn_native/Device.h"
#include "dawn_native/RenderPipeline.h"

#include <string.h>

namespace dawn_native {

    RenderPassEncoderBase::RenderPassEncoderBase(DeviceBase* device,
                                                 CommandEncoderBase* topLevelEncoder,
                                                 CommandAllocator* allocator)
        : ProgrammablePassEncoder(device, topLevelEncoder, allocator) {
    }

    RenderPassEncoderBase::RenderPassEncoderBase(DeviceBase* device,
                                                 CommandEncoderBase* topLevelEncoder,
                                                 ErrorTag errorTag)
        : ProgrammablePassEncoder(device, topLevelEncoder, errorTag) {
    }

    RenderPassEncoderBase* RenderPassEncoderBase::MakeError(DeviceBase* device,
                                                            CommandEncoderBase* topLevelEncoder) {
        return new RenderPassEncoderBase(device, topLevelEncoder, ObjectBase::kError);
    }

    void RenderPassEncoderBase::Draw(uint32_t vertexCount,
                                     uint32_t instanceCount,
                                     uint32_t firstVertex,
                                     uint32_t firstInstance) {
        if (mTopLevelEncoder->ConsumedError(ValidateCanRecordCommands())) {
            return;
        }

        DrawCmd* draw = mAllocator->Allocate<DrawCmd>(Command::Draw);
        draw->vertexCount = vertexCount;
        draw->instanceCount = instanceCount;
        draw->firstVertex = firstVertex;
        draw->firstInstance = firstInstance;
    }

    void RenderPassEncoderBase::DrawIndexed(uint32_t indexCount,
                                            uint32_t instanceCount,
                                            uint32_t firstIndex,
                                            int32_t baseVertex,
                                            uint32_t firstInstance) {
        if (mTopLevelEncoder->ConsumedError(ValidateCanRecordCommands())) {
            return;
        }

        DrawIndexedCmd* draw = mAllocator->Allocate<DrawIndexedCmd>(Command::DrawIndexed);
        draw->indexCount = indexCount;
        draw->instanceCount = instanceCount;
        draw->firstIndex = firstIndex;
        draw->baseVertex = baseVertex;
        draw->firstInstance = firstInstance;
    }

    void RenderPassEncoderBase::DrawIndirect(BufferBase* indirectBuffer, uint64_t indirectOffset) {
        if (mTopLevelEncoder->ConsumedError(ValidateCanRecordCommands()) ||
            mTopLevelEncoder->ConsumedError(GetDevice()->ValidateObject(indirectBuffer))) {
            return;
        }

        if (indirectOffset >= indirectBuffer->GetSize() ||
            indirectOffset + kDrawIndirectSize > indirectBuffer->GetSize()) {
            mTopLevelEncoder->HandleError("Indirect offset out of bounds");
            return;
        }

        DrawIndirectCmd* cmd = mAllocator->Allocate<DrawIndirectCmd>(Command::DrawIndirect);
        cmd->indirectBuffer = indirectBuffer;
        cmd->indirectOffset = indirectOffset;
    }

    void RenderPassEncoderBase::DrawIndexedIndirect(BufferBase* indirectBuffer,
                                                    uint64_t indirectOffset) {
        if (mTopLevelEncoder->ConsumedError(ValidateCanRecordCommands()) ||
            mTopLevelEncoder->ConsumedError(GetDevice()->ValidateObject(indirectBuffer))) {
            return;
        }

        if (indirectOffset >= indirectBuffer->GetSize() ||
            indirectOffset + kDrawIndexedIndirectSize > indirectBuffer->GetSize()) {
            mTopLevelEncoder->HandleError("Indirect offset out of bounds");
            return;
        }

        DrawIndexedIndirectCmd* cmd =
            mAllocator->Allocate<DrawIndexedIndirectCmd>(Command::DrawIndexedIndirect);
        cmd->indirectBuffer = indirectBuffer;
        cmd->indirectOffset = indirectOffset;
    }

    void RenderPassEncoderBase::SetPipeline(RenderPipelineBase* pipeline) {
        if (mTopLevelEncoder->ConsumedError(ValidateCanRecordCommands()) ||
            mTopLevelEncoder->ConsumedError(GetDevice()->ValidateObject(pipeline))) {
            return;
        }

        SetRenderPipelineCmd* cmd =
            mAllocator->Allocate<SetRenderPipelineCmd>(Command::SetRenderPipeline);
        cmd->pipeline = pipeline;
    }

    void RenderPassEncoderBase::SetStencilReference(uint32_t reference) {
        if (mTopLevelEncoder->ConsumedError(ValidateCanRecordCommands())) {
            return;
        }

        SetStencilReferenceCmd* cmd =
            mAllocator->Allocate<SetStencilReferenceCmd>(Command::SetStencilReference);
        cmd->reference = reference;
    }

    void RenderPassEncoderBase::SetBlendColor(const Color* color) {
        if (mTopLevelEncoder->ConsumedError(ValidateCanRecordCommands())) {
            return;
        }

        SetBlendColorCmd* cmd = mAllocator->Allocate<SetBlendColorCmd>(Command::SetBlendColor);
        cmd->color = *color;
    }

    void RenderPassEncoderBase::SetScissorRect(uint32_t x,
                                               uint32_t y,
                                               uint32_t width,
                                               uint32_t height) {
        if (mTopLevelEncoder->ConsumedError(ValidateCanRecordCommands())) {
            return;
        }
        if (width == 0 || height == 0) {
            mTopLevelEncoder->HandleError("Width and height must be greater than 0.");
            return;
        }

        SetScissorRectCmd* cmd = mAllocator->Allocate<SetScissorRectCmd>(Command::SetScissorRect);
        cmd->x = x;
        cmd->y = y;
        cmd->width = width;
        cmd->height = height;
    }

    void RenderPassEncoderBase::SetIndexBuffer(BufferBase* buffer, uint64_t offset) {
        if (mTopLevelEncoder->ConsumedError(ValidateCanRecordCommands()) ||
            mTopLevelEncoder->ConsumedError(GetDevice()->ValidateObject(buffer))) {
            return;
        }

        SetIndexBufferCmd* cmd = mAllocator->Allocate<SetIndexBufferCmd>(Command::SetIndexBuffer);
        cmd->buffer = buffer;
        cmd->offset = offset;
    }

    void RenderPassEncoderBase::SetVertexBuffers(uint32_t startSlot,
                                                 uint32_t count,
                                                 BufferBase* const* buffers,
                                                 uint64_t const* offsets) {
        if (mTopLevelEncoder->ConsumedError(ValidateCanRecordCommands())) {
            return;
        }

        for (size_t i = 0; i < count; ++i) {
            if (mTopLevelEncoder->ConsumedError(GetDevice()->ValidateObject(buffers[i]))) {
                return;
            }
        }

        SetVertexBuffersCmd* cmd =
            mAllocator->Allocate<SetVertexBuffersCmd>(Command::SetVertexBuffers);
        cmd->startSlot = startSlot;
        cmd->count = count;

        Ref<BufferBase>* cmdBuffers = mAllocator->AllocateData<Ref<BufferBase>>(count);
        for (size_t i = 0; i < count; ++i) {
            cmdBuffers[i] = buffers[i];
        }

        uint64_t* cmdOffsets = mAllocator->AllocateData<uint64_t>(count);
        memcpy(cmdOffsets, offsets, count * sizeof(uint64_t));
    }

}  // namespace dawn_native
