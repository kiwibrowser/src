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

#include "dawn_native/RenderEncoderBase.h"

#include "common/Constants.h"
#include "dawn_native/Buffer.h"
#include "dawn_native/CommandEncoder.h"
#include "dawn_native/Commands.h"
#include "dawn_native/Device.h"
#include "dawn_native/RenderPipeline.h"

#include <math.h>
#include <cstring>

namespace dawn_native {

    RenderEncoderBase::RenderEncoderBase(DeviceBase* device, EncodingContext* encodingContext)
        : ProgrammablePassEncoder(device, encodingContext, PassType::Render),
          mDisableBaseVertex(device->IsToggleEnabled(Toggle::DisableBaseVertex)),
          mDisableBaseInstance(device->IsToggleEnabled(Toggle::DisableBaseInstance)) {
    }

    RenderEncoderBase::RenderEncoderBase(DeviceBase* device,
                                         EncodingContext* encodingContext,
                                         ErrorTag errorTag)
        : ProgrammablePassEncoder(device, encodingContext, errorTag, PassType::Render),
          mDisableBaseVertex(device->IsToggleEnabled(Toggle::DisableBaseVertex)),
          mDisableBaseInstance(device->IsToggleEnabled(Toggle::DisableBaseInstance)) {
    }

    void RenderEncoderBase::Draw(uint32_t vertexCount,
                                 uint32_t instanceCount,
                                 uint32_t firstVertex,
                                 uint32_t firstInstance) {
        mEncodingContext->TryEncode(this, [&](CommandAllocator* allocator) -> MaybeError {
            if (mDisableBaseInstance && firstInstance != 0) {
                return DAWN_VALIDATION_ERROR("Non-zero first instance not supported");
            }

            DrawCmd* draw = allocator->Allocate<DrawCmd>(Command::Draw);
            draw->vertexCount = vertexCount;
            draw->instanceCount = instanceCount;
            draw->firstVertex = firstVertex;
            draw->firstInstance = firstInstance;

            return {};
        });
    }

    void RenderEncoderBase::DrawIndexed(uint32_t indexCount,
                                        uint32_t instanceCount,
                                        uint32_t firstIndex,
                                        int32_t baseVertex,
                                        uint32_t firstInstance) {
        mEncodingContext->TryEncode(this, [&](CommandAllocator* allocator) -> MaybeError {
            if (mDisableBaseInstance && firstInstance != 0) {
                return DAWN_VALIDATION_ERROR("Non-zero first instance not supported");
            }
            if (mDisableBaseInstance && baseVertex != 0) {
                return DAWN_VALIDATION_ERROR("Non-zero base vertex not supported");
            }

            DrawIndexedCmd* draw = allocator->Allocate<DrawIndexedCmd>(Command::DrawIndexed);
            draw->indexCount = indexCount;
            draw->instanceCount = instanceCount;
            draw->firstIndex = firstIndex;
            draw->baseVertex = baseVertex;
            draw->firstInstance = firstInstance;

            return {};
        });
    }

    void RenderEncoderBase::DrawIndirect(BufferBase* indirectBuffer, uint64_t indirectOffset) {
        mEncodingContext->TryEncode(this, [&](CommandAllocator* allocator) -> MaybeError {
            DAWN_TRY(GetDevice()->ValidateObject(indirectBuffer));

            if (indirectOffset >= indirectBuffer->GetSize() ||
                indirectOffset + kDrawIndirectSize > indirectBuffer->GetSize()) {
                return DAWN_VALIDATION_ERROR("Indirect offset out of bounds");
            }

            DrawIndirectCmd* cmd = allocator->Allocate<DrawIndirectCmd>(Command::DrawIndirect);
            cmd->indirectBuffer = indirectBuffer;
            cmd->indirectOffset = indirectOffset;

            mUsageTracker.BufferUsedAs(indirectBuffer, wgpu::BufferUsage::Indirect);

            return {};
        });
    }

    void RenderEncoderBase::DrawIndexedIndirect(BufferBase* indirectBuffer,
                                                uint64_t indirectOffset) {
        mEncodingContext->TryEncode(this, [&](CommandAllocator* allocator) -> MaybeError {
            DAWN_TRY(GetDevice()->ValidateObject(indirectBuffer));

            if ((indirectOffset >= indirectBuffer->GetSize() ||
                 indirectOffset + kDrawIndexedIndirectSize > indirectBuffer->GetSize())) {
                return DAWN_VALIDATION_ERROR("Indirect offset out of bounds");
            }

            DrawIndexedIndirectCmd* cmd =
                allocator->Allocate<DrawIndexedIndirectCmd>(Command::DrawIndexedIndirect);
            cmd->indirectBuffer = indirectBuffer;
            cmd->indirectOffset = indirectOffset;

            mUsageTracker.BufferUsedAs(indirectBuffer, wgpu::BufferUsage::Indirect);

            return {};
        });
    }

    void RenderEncoderBase::SetPipeline(RenderPipelineBase* pipeline) {
        mEncodingContext->TryEncode(this, [&](CommandAllocator* allocator) -> MaybeError {
            DAWN_TRY(GetDevice()->ValidateObject(pipeline));

            SetRenderPipelineCmd* cmd =
                allocator->Allocate<SetRenderPipelineCmd>(Command::SetRenderPipeline);
            cmd->pipeline = pipeline;

            return {};
        });
    }

    void RenderEncoderBase::SetIndexBuffer(BufferBase* buffer, uint64_t offset, uint64_t size) {
        mEncodingContext->TryEncode(this, [&](CommandAllocator* allocator) -> MaybeError {
            DAWN_TRY(GetDevice()->ValidateObject(buffer));

            uint64_t bufferSize = buffer->GetSize();
            if (offset > bufferSize) {
                return DAWN_VALIDATION_ERROR("Offset larger than the buffer size");
            }
            uint64_t remainingSize = bufferSize - offset;

            if (size == 0) {
                size = remainingSize;
            } else {
                if (size > remainingSize) {
                    return DAWN_VALIDATION_ERROR("Size + offset larger than the buffer size");
                }
            }

            SetIndexBufferCmd* cmd =
                allocator->Allocate<SetIndexBufferCmd>(Command::SetIndexBuffer);
            cmd->buffer = buffer;
            cmd->offset = offset;
            cmd->size = size;

            mUsageTracker.BufferUsedAs(buffer, wgpu::BufferUsage::Index);

            return {};
        });
    }

    void RenderEncoderBase::SetVertexBuffer(uint32_t slot,
                                            BufferBase* buffer,
                                            uint64_t offset,
                                            uint64_t size) {
        mEncodingContext->TryEncode(this, [&](CommandAllocator* allocator) -> MaybeError {
            DAWN_TRY(GetDevice()->ValidateObject(buffer));

            if (slot >= kMaxVertexBuffers) {
                return DAWN_VALIDATION_ERROR("Vertex buffer slot out of bounds");
            }

            uint64_t bufferSize = buffer->GetSize();
            if (offset > bufferSize) {
                return DAWN_VALIDATION_ERROR("Offset larger than the buffer size");
            }
            uint64_t remainingSize = bufferSize - offset;

            if (size == 0) {
                size = remainingSize;
            } else {
                if (size > remainingSize) {
                    return DAWN_VALIDATION_ERROR("Size + offset larger than the buffer size");
                }
            }

            SetVertexBufferCmd* cmd =
                allocator->Allocate<SetVertexBufferCmd>(Command::SetVertexBuffer);
            cmd->slot = slot;
            cmd->buffer = buffer;
            cmd->offset = offset;
            cmd->size = size;

            mUsageTracker.BufferUsedAs(buffer, wgpu::BufferUsage::Vertex);

            return {};
        });
    }

}  // namespace dawn_native
