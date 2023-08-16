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

#include "dawn_native/AttachmentState.h"
#include "dawn_native/BindingInfo.h"
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
        ExecuteBundles,
        InsertDebugMarker,
        PopDebugGroup,
        PushDebugGroup,
        ResolveQuerySet,
        SetComputePipeline,
        SetRenderPipeline,
        SetStencilReference,
        SetViewport,
        SetScissorRect,
        SetBlendColor,
        SetBindGroup,
        SetIndexBuffer,
        SetVertexBuffer,
        WriteTimestamp,
    };

    struct BeginComputePassCmd {};

    struct RenderPassColorAttachmentInfo {
        Ref<TextureViewBase> view;
        Ref<TextureViewBase> resolveTarget;
        wgpu::LoadOp loadOp;
        wgpu::StoreOp storeOp;
        dawn_native::Color clearColor;
    };

    struct RenderPassDepthStencilAttachmentInfo {
        Ref<TextureViewBase> view;
        wgpu::LoadOp depthLoadOp;
        wgpu::StoreOp depthStoreOp;
        wgpu::LoadOp stencilLoadOp;
        wgpu::StoreOp stencilStoreOp;
        float clearDepth;
        uint32_t clearStencil;
    };

    struct BeginRenderPassCmd {
        Ref<AttachmentState> attachmentState;
        RenderPassColorAttachmentInfo colorAttachments[kMaxColorAttachments];
        RenderPassDepthStencilAttachmentInfo depthStencilAttachment;

        // Cache the width and height of all attachments for convenience
        uint32_t width;
        uint32_t height;
    };

    struct BufferCopy {
        Ref<BufferBase> buffer;
        uint64_t offset;
        uint32_t bytesPerRow;
        uint32_t rowsPerImage;
    };

    struct TextureCopy {
        Ref<TextureBase> texture;
        uint32_t mipLevel;
        Origin3D origin;  // Texels / array layer
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

    struct ExecuteBundlesCmd {
        uint32_t count;
    };

    struct InsertDebugMarkerCmd {
        uint32_t length;
    };

    struct PopDebugGroupCmd {};

    struct PushDebugGroupCmd {
        uint32_t length;
    };

    struct ResolveQuerySetCmd {
        Ref<QuerySetBase> querySet;
        uint32_t firstQuery;
        uint32_t queryCount;
        Ref<BufferBase> destination;
        uint64_t destinationOffset;
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

    struct SetViewportCmd {
        float x, y, width, height, minDepth, maxDepth;
    };

    struct SetScissorRectCmd {
        uint32_t x, y, width, height;
    };

    struct SetBlendColorCmd {
        Color color;
    };

    struct SetBindGroupCmd {
        BindGroupIndex index;
        Ref<BindGroupBase> group;
        uint32_t dynamicOffsetCount;
    };

    struct SetIndexBufferCmd {
        Ref<BufferBase> buffer;
        uint64_t offset;
        uint64_t size;
    };

    struct SetVertexBufferCmd {
        uint32_t slot;
        Ref<BufferBase> buffer;
        uint64_t offset;
        uint64_t size;
    };

    struct WriteTimestampCmd {
        Ref<QuerySetBase> querySet;
        uint32_t queryIndex;
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
