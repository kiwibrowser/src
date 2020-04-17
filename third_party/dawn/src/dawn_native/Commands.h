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

#ifndef DAWNNATIVE_COMMANDS_H_
#define DAWNNATIVE_COMMANDS_H_

#include "common/Constants.h"

#include "dawn_native/Texture.h"

#include "dawn_native/dawn_platform.h"

#include <array>
#include <bitset>

namespace dawn_native {

    // Definition of the commands that are present in the CommandIterator given by the
    // CommandBufferBuilder. There are not defined in CommandBuffer.h to break some header
    // dependencies: Ref<Object> needs Object to be defined.

    enum class Command {
        BeginComputePass,
        BeginRenderPass,
        CopyBufferToBuffer,
        CopyBufferToTexture,
        CopyTextureToBuffer,
        CopyTextureToTexture,
        Dispatch,
        DispatchIndirect,
        Draw,
        DrawIndexed,
        DrawIndirect,
        DrawIndexedIndirect,
        EndComputePass,
        EndRenderPass,
        InsertDebugMarker,
        PopDebugGroup,
        PushDebugGroup,
        SetComputePipeline,
        SetRenderPipeline,
        SetStencilReference,
        SetScissorRect,
        SetBlendColor,
        SetBindGroup,
        SetIndexBuffer,
        SetVertexBuffers,
    };

    struct BeginComputePassCmd {};

    struct RenderPassColorAttachmentInfo {
        Ref<TextureViewBase> view;
        Ref<TextureViewBase> resolveTarget;
        dawn::LoadOp loadOp;
        dawn::StoreOp storeOp;
        dawn_native::Color clearColor;
    };

    struct RenderPassDepthStencilAttachmentInfo {
        Ref<TextureViewBase> view;
        dawn::LoadOp depthLoadOp;
        dawn::StoreOp depthStoreOp;
        dawn::LoadOp stencilLoadOp;
        dawn::StoreOp stencilStoreOp;
        float clearDepth;
        uint32_t clearStencil;
    };

    struct BeginRenderPassCmd {
        std::bitset<kMaxColorAttachments> colorAttachmentsSet;
        RenderPassColorAttachmentInfo colorAttachments[kMaxColorAttachments];
        bool hasDepthStencilAttachment;
        RenderPassDepthStencilAttachmentInfo depthStencilAttachment;

        // Cache the width, height and sample count of all attachments for convenience
        uint32_t width;
        uint32_t height;
        uint32_t sampleCount;
    };

    struct BufferCopy {
        Ref<BufferBase> buffer;
        uint64_t offset;       // Bytes
        uint32_t rowPitch;     // Bytes
        uint32_t imageHeight;  // Texels
    };

    struct TextureCopy {
        Ref<TextureBase> texture;
        uint32_t level;
        uint32_t slice;
        Origin3D origin;  // Texels
    };

    struct CopyBufferToBufferCmd {
        Ref<BufferBase> source;
        uint64_t sourceOffset;
        Ref<BufferBase> destination;
        uint64_t destinationOffset;
        uint64_t size;
    };

    struct CopyBufferToTextureCmd {
        BufferCopy source;
        TextureCopy destination;
        Extent3D copySize;  // Texels
    };

    struct CopyTextureToBufferCmd {
        TextureCopy source;
        BufferCopy destination;
        Extent3D copySize;  // Texels
    };

    struct CopyTextureToTextureCmd {
        TextureCopy source;
        TextureCopy destination;
        Extent3D copySize;  // Texels
    };

    struct DispatchCmd {
        uint32_t x;
        uint32_t y;
        uint32_t z;
    };

    struct DispatchIndirectCmd {
        Ref<BufferBase> indirectBuffer;
        uint64_t indirectOffset;
    };

    struct DrawCmd {
        uint32_t vertexCount;
        uint32_t instanceCount;
        uint32_t firstVertex;
        uint32_t firstInstance;
    };

    struct DrawIndexedCmd {
        uint32_t indexCount;
        uint32_t instanceCount;
        uint32_t firstIndex;
        int32_t baseVertex;
        uint32_t firstInstance;
    };

    struct DrawIndirectCmd {
        Ref<BufferBase> indirectBuffer;
        uint64_t indirectOffset;
    };

    struct DrawIndexedIndirectCmd {
        Ref<BufferBase> indirectBuffer;
        uint64_t indirectOffset;
    };

    struct EndComputePassCmd {};

    struct EndRenderPassCmd {};

    struct InsertDebugMarkerCmd {
        uint32_t length;
    };

    struct PopDebugGroupCmd {};

    struct PushDebugGroupCmd {
        uint32_t length;
    };

    struct SetComputePipelineCmd {
        Ref<ComputePipelineBase> pipeline;
    };

    struct SetRenderPipelineCmd {
        Ref<RenderPipelineBase> pipeline;
    };

    struct SetStencilReferenceCmd {
        uint32_t reference;
    };

    struct SetScissorRectCmd {
        uint32_t x, y, width, height;
    };

    struct SetBlendColorCmd {
        Color color;
    };

    struct SetBindGroupCmd {
        uint32_t index;
        Ref<BindGroupBase> group;
        uint32_t dynamicOffsetCount;
    };

    struct SetIndexBufferCmd {
        Ref<BufferBase> buffer;
        uint64_t offset;
    };

    struct SetVertexBuffersCmd {
        uint32_t startSlot;
        uint32_t count;
    };

    // This needs to be called before the CommandIterator is freed so that the Ref<> present in
    // the commands have a chance to run their destructor and remove internal references.
    class CommandIterator;
    void FreeCommands(CommandIterator* commands);

    // Helper function to allow skipping over a command when it is unimplemented, while still
    // consuming the correct amount of data from the command iterator.
    void SkipCommand(CommandIterator* commands, Command type);

}  // namespace dawn_native

#endif  // DAWNNATIVE_COMMANDS_H_
