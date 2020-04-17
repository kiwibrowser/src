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

#ifndef DAWNNATIVE_D3D12_COMMANDBUFFERD3D12_H_
#define DAWNNATIVE_D3D12_COMMANDBUFFERD3D12_H_

#include "common/Constants.h"
#include "dawn_native/CommandAllocator.h"
#include "dawn_native/CommandBuffer.h"

#include "dawn_native/d3d12/Forward.h"
#include "dawn_native/d3d12/d3d12_platform.h"

#include <array>

namespace dawn_native {
    struct BeginRenderPassCmd;
}  // namespace dawn_native

namespace dawn_native { namespace d3d12 {

    class BindGroupStateTracker;
    class Device;
    class RenderPassDescriptorHeapTracker;
    class RenderPipeline;

    struct VertexBuffersInfo {
        // startSlot and endSlot indicate the range of dirty vertex buffers.
        // If there are multiple calls to SetVertexBuffers, the start and end
        // represent the union of the dirty ranges (the union may have non-dirty
        // data in the middle of the range).
        const RenderPipeline* lastRenderPipeline = nullptr;
        uint32_t startSlot = kMaxVertexBuffers;
        uint32_t endSlot = 0;
        std::array<D3D12_VERTEX_BUFFER_VIEW, kMaxVertexBuffers> d3d12BufferViews = {};
    };

    class CommandBuffer : public CommandBufferBase {
      public:
        CommandBuffer(Device* device, CommandEncoderBase* encoder);
        ~CommandBuffer();

        void RecordCommands(ComPtr<ID3D12GraphicsCommandList> commandList, uint32_t indexInSubmit);

      private:
        void FlushSetVertexBuffers(ComPtr<ID3D12GraphicsCommandList> commandList,
                                   VertexBuffersInfo* vertexBuffersInfo,
                                   const RenderPipeline* lastRenderPipeline);
        void RecordComputePass(ComPtr<ID3D12GraphicsCommandList> commandList,
                               BindGroupStateTracker* bindingTracker);
        void RecordRenderPass(ComPtr<ID3D12GraphicsCommandList> commandList,
                              BindGroupStateTracker* bindingTracker,
                              RenderPassDescriptorHeapTracker* renderPassTracker,
                              BeginRenderPassCmd* renderPass);

        CommandIterator mCommands;
    };

}}  // namespace dawn_native::d3d12

#endif  // DAWNNATIVE_D3D12_COMMANDBUFFERD3D12_H_
