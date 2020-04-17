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

#include "dawn_native/ComputePassEncoder.h"

#include "dawn_native/Buffer.h"
#include "dawn_native/CommandEncoder.h"
#include "dawn_native/Commands.h"
#include "dawn_native/ComputePipeline.h"
#include "dawn_native/Device.h"

namespace dawn_native {

    ComputePassEncoderBase::ComputePassEncoderBase(DeviceBase* device,
                                                   CommandEncoderBase* topLevelEncoder,
                                                   CommandAllocator* allocator)
        : ProgrammablePassEncoder(device, topLevelEncoder, allocator) {
    }

    ComputePassEncoderBase::ComputePassEncoderBase(DeviceBase* device,
                                                   CommandEncoderBase* topLevelEncoder,
                                                   ErrorTag errorTag)
        : ProgrammablePassEncoder(device, topLevelEncoder, errorTag) {
    }

    ComputePassEncoderBase* ComputePassEncoderBase::MakeError(DeviceBase* device,
                                                              CommandEncoderBase* topLevelEncoder) {
        return new ComputePassEncoderBase(device, topLevelEncoder, ObjectBase::kError);
    }

    void ComputePassEncoderBase::Dispatch(uint32_t x, uint32_t y, uint32_t z) {
        if (mTopLevelEncoder->ConsumedError(ValidateCanRecordCommands())) {
            return;
        }

        DispatchCmd* dispatch = mAllocator->Allocate<DispatchCmd>(Command::Dispatch);
        dispatch->x = x;
        dispatch->y = y;
        dispatch->z = z;
    }

    void ComputePassEncoderBase::DispatchIndirect(BufferBase* indirectBuffer,
                                                  uint64_t indirectOffset) {
        if (mTopLevelEncoder->ConsumedError(ValidateCanRecordCommands()) ||
            mTopLevelEncoder->ConsumedError(GetDevice()->ValidateObject(indirectBuffer))) {
            return;
        }

        if (indirectOffset >= indirectBuffer->GetSize() ||
            indirectOffset + kDispatchIndirectSize > indirectBuffer->GetSize()) {
            mTopLevelEncoder->HandleError("Indirect offset out of bounds");
            return;
        }

        DispatchIndirectCmd* dispatch =
            mAllocator->Allocate<DispatchIndirectCmd>(Command::DispatchIndirect);
        dispatch->indirectBuffer = indirectBuffer;
        dispatch->indirectOffset = indirectOffset;
    }

    void ComputePassEncoderBase::SetPipeline(ComputePipelineBase* pipeline) {
        if (mTopLevelEncoder->ConsumedError(ValidateCanRecordCommands()) ||
            mTopLevelEncoder->ConsumedError(GetDevice()->ValidateObject(pipeline))) {
            return;
        }

        SetComputePipelineCmd* cmd =
            mAllocator->Allocate<SetComputePipelineCmd>(Command::SetComputePipeline);
        cmd->pipeline = pipeline;
    }

}  // namespace dawn_native
