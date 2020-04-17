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

#include "dawn_native/Commands.h"

#include "dawn_native/BindGroup.h"
#include "dawn_native/Buffer.h"
#include "dawn_native/CommandAllocator.h"
#include "dawn_native/ComputePipeline.h"
#include "dawn_native/RenderPipeline.h"
#include "dawn_native/Texture.h"

namespace dawn_native {

    void FreeCommands(CommandIterator* commands) {
        commands->Reset();

        Command type;
        while (commands->NextCommandId(&type)) {
            switch (type) {
                case Command::BeginComputePass: {
                    BeginComputePassCmd* begin = commands->NextCommand<BeginComputePassCmd>();
                    begin->~BeginComputePassCmd();
                } break;
                case Command::BeginRenderPass: {
                    BeginRenderPassCmd* begin = commands->NextCommand<BeginRenderPassCmd>();
                    begin->~BeginRenderPassCmd();
                } break;
                case Command::CopyBufferToBuffer: {
                    CopyBufferToBufferCmd* copy = commands->NextCommand<CopyBufferToBufferCmd>();
                    copy->~CopyBufferToBufferCmd();
                } break;
                case Command::CopyBufferToTexture: {
                    CopyBufferToTextureCmd* copy = commands->NextCommand<CopyBufferToTextureCmd>();
                    copy->~CopyBufferToTextureCmd();
                } break;
                case Command::CopyTextureToBuffer: {
                    CopyTextureToBufferCmd* copy = commands->NextCommand<CopyTextureToBufferCmd>();
                    copy->~CopyTextureToBufferCmd();
                } break;
                case Command::CopyTextureToTexture: {
                    CopyTextureToTextureCmd* copy =
                        commands->NextCommand<CopyTextureToTextureCmd>();
                    copy->~CopyTextureToTextureCmd();
                } break;
                case Command::Dispatch: {
                    DispatchCmd* dispatch = commands->NextCommand<DispatchCmd>();
                    dispatch->~DispatchCmd();
                } break;
                case Command::DispatchIndirect: {
                    DispatchIndirectCmd* dispatch = commands->NextCommand<DispatchIndirectCmd>();
                    dispatch->~DispatchIndirectCmd();
                } break;
                case Command::Draw: {
                    DrawCmd* draw = commands->NextCommand<DrawCmd>();
                    draw->~DrawCmd();
                } break;
                case Command::DrawIndexed: {
                    DrawIndexedCmd* draw = commands->NextCommand<DrawIndexedCmd>();
                    draw->~DrawIndexedCmd();
                } break;
                case Command::DrawIndirect: {
                    DrawIndirectCmd* draw = commands->NextCommand<DrawIndirectCmd>();
                    draw->~DrawIndirectCmd();
                } break;
                case Command::DrawIndexedIndirect: {
                    DrawIndexedIndirectCmd* draw = commands->NextCommand<DrawIndexedIndirectCmd>();
                    draw->~DrawIndexedIndirectCmd();
                } break;
                case Command::EndComputePass: {
                    EndComputePassCmd* cmd = commands->NextCommand<EndComputePassCmd>();
                    cmd->~EndComputePassCmd();
                } break;
                case Command::EndRenderPass: {
                    EndRenderPassCmd* cmd = commands->NextCommand<EndRenderPassCmd>();
                    cmd->~EndRenderPassCmd();
                } break;
                case Command::InsertDebugMarker: {
                    InsertDebugMarkerCmd* cmd = commands->NextCommand<InsertDebugMarkerCmd>();
                    commands->NextData<char>(cmd->length + 1);
                    cmd->~InsertDebugMarkerCmd();
                } break;
                case Command::PopDebugGroup: {
                    PopDebugGroupCmd* cmd = commands->NextCommand<PopDebugGroupCmd>();
                    cmd->~PopDebugGroupCmd();
                } break;
                case Command::PushDebugGroup: {
                    PushDebugGroupCmd* cmd = commands->NextCommand<PushDebugGroupCmd>();
                    commands->NextData<char>(cmd->length + 1);
                    cmd->~PushDebugGroupCmd();
                } break;
                case Command::SetComputePipeline: {
                    SetComputePipelineCmd* cmd = commands->NextCommand<SetComputePipelineCmd>();
                    cmd->~SetComputePipelineCmd();
                } break;
                case Command::SetRenderPipeline: {
                    SetRenderPipelineCmd* cmd = commands->NextCommand<SetRenderPipelineCmd>();
                    cmd->~SetRenderPipelineCmd();
                } break;
                case Command::SetStencilReference: {
                    SetStencilReferenceCmd* cmd = commands->NextCommand<SetStencilReferenceCmd>();
                    cmd->~SetStencilReferenceCmd();
                } break;
                case Command::SetScissorRect: {
                    SetScissorRectCmd* cmd = commands->NextCommand<SetScissorRectCmd>();
                    cmd->~SetScissorRectCmd();
                } break;
                case Command::SetBlendColor: {
                    SetBlendColorCmd* cmd = commands->NextCommand<SetBlendColorCmd>();
                    cmd->~SetBlendColorCmd();
                } break;
                case Command::SetBindGroup: {
                    SetBindGroupCmd* cmd = commands->NextCommand<SetBindGroupCmd>();
                    if (cmd->dynamicOffsetCount > 0) {
                        commands->NextData<uint64_t>(cmd->dynamicOffsetCount);
                    }
                    cmd->~SetBindGroupCmd();
                } break;
                case Command::SetIndexBuffer: {
                    SetIndexBufferCmd* cmd = commands->NextCommand<SetIndexBufferCmd>();
                    cmd->~SetIndexBufferCmd();
                } break;
                case Command::SetVertexBuffers: {
                    SetVertexBuffersCmd* cmd = commands->NextCommand<SetVertexBuffersCmd>();
                    auto buffers = commands->NextData<Ref<BufferBase>>(cmd->count);
                    for (size_t i = 0; i < cmd->count; ++i) {
                        (&buffers[i])->~Ref<BufferBase>();
                    }
                    commands->NextData<uint64_t>(cmd->count);
                    cmd->~SetVertexBuffersCmd();
                } break;
            }
        }
        commands->DataWasDestroyed();
    }

    void SkipCommand(CommandIterator* commands, Command type) {
        switch (type) {
            case Command::BeginComputePass:
                commands->NextCommand<BeginComputePassCmd>();
                break;

            case Command::BeginRenderPass:
                commands->NextCommand<BeginRenderPassCmd>();
                break;

            case Command::CopyBufferToBuffer:
                commands->NextCommand<CopyBufferToBufferCmd>();
                break;

            case Command::CopyBufferToTexture:
                commands->NextCommand<CopyBufferToTextureCmd>();
                break;

            case Command::CopyTextureToBuffer:
                commands->NextCommand<CopyTextureToBufferCmd>();
                break;

            case Command::CopyTextureToTexture:
                commands->NextCommand<CopyTextureToTextureCmd>();
                break;

            case Command::Dispatch:
                commands->NextCommand<DispatchCmd>();
                break;

            case Command::DispatchIndirect:
                commands->NextCommand<DispatchIndirectCmd>();
                break;

            case Command::Draw:
                commands->NextCommand<DrawCmd>();
                break;

            case Command::DrawIndexed:
                commands->NextCommand<DrawIndexedCmd>();
                break;

            case Command::DrawIndirect:
                commands->NextCommand<DrawIndirectCmd>();
                break;

            case Command::DrawIndexedIndirect:
                commands->NextCommand<DrawIndexedIndirectCmd>();
                break;

            case Command::EndComputePass:
                commands->NextCommand<EndComputePassCmd>();
                break;

            case Command::EndRenderPass:
                commands->NextCommand<EndRenderPassCmd>();
                break;

            case Command::InsertDebugMarker: {
                InsertDebugMarkerCmd* cmd = commands->NextCommand<InsertDebugMarkerCmd>();
                commands->NextData<char>(cmd->length + 1);
            } break;

            case Command::PopDebugGroup:
                commands->NextCommand<PopDebugGroupCmd>();
                break;

            case Command::PushDebugGroup: {
                PushDebugGroupCmd* cmd = commands->NextCommand<PushDebugGroupCmd>();
                commands->NextData<char>(cmd->length + 1);
            } break;

            case Command::SetComputePipeline:
                commands->NextCommand<SetComputePipelineCmd>();
                break;

            case Command::SetRenderPipeline:
                commands->NextCommand<SetRenderPipelineCmd>();
                break;

            case Command::SetStencilReference:
                commands->NextCommand<SetStencilReferenceCmd>();
                break;

            case Command::SetScissorRect:
                commands->NextCommand<SetScissorRectCmd>();
                break;

            case Command::SetBlendColor:
                commands->NextCommand<SetBlendColorCmd>();
                break;

            case Command::SetBindGroup:
                commands->NextCommand<SetBindGroupCmd>();
                break;

            case Command::SetIndexBuffer:
                commands->NextCommand<SetIndexBufferCmd>();
                break;

            case Command::SetVertexBuffers: {
                auto* cmd = commands->NextCommand<SetVertexBuffersCmd>();
                commands->NextData<Ref<BufferBase>>(cmd->count);
                commands->NextData<uint64_t>(cmd->count);
            } break;
        }
    }

}  // namespace dawn_native
