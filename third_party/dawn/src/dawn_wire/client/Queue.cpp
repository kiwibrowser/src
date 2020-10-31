// Copyright 2020 The Dawn Authors
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

#include "dawn_wire/client/Queue.h"

#include "dawn_wire/client/Client.h"
#include "dawn_wire/client/Device.h"

namespace dawn_wire { namespace client {

    WGPUFence Queue::CreateFence(WGPUFenceDescriptor const* descriptor) {
        auto* allocation = device->GetClient()->FenceAllocator().New(device);

        QueueCreateFenceCmd cmd;
        cmd.self = ToAPI(this);
        cmd.result = ObjectHandle{allocation->object->id, allocation->generation};
        cmd.descriptor = descriptor;
        device->GetClient()->SerializeCommand(cmd);

        Fence* fence = allocation->object.get();
        fence->Initialize(this, descriptor);
        return ToAPI(fence);
    }

    void Queue::Signal(WGPUFence cFence, uint64_t signalValue) {
        Fence* fence = FromAPI(cFence);
        if (fence->GetQueue() != this) {
            device->InjectError(WGPUErrorType_Validation,
                                "Fence must be signaled on the queue on which it was created.");
            return;
        }
        if (signalValue <= fence->GetSignaledValue()) {
            device->InjectError(WGPUErrorType_Validation,
                                "Fence value less than or equal to signaled value");
            return;
        }

        fence->SetSignaledValue(signalValue);

        QueueSignalCmd cmd;
        cmd.self = ToAPI(this);
        cmd.fence = cFence;
        cmd.signalValue = signalValue;

        device->GetClient()->SerializeCommand(cmd);
    }

    void Queue::WriteBuffer(WGPUBuffer cBuffer,
                            uint64_t bufferOffset,
                            const void* data,
                            size_t size) {
        Buffer* buffer = FromAPI(cBuffer);

        QueueWriteBufferInternalCmd cmd;
        cmd.queueId = id;
        cmd.bufferId = buffer->id;
        cmd.bufferOffset = bufferOffset;
        cmd.data = static_cast<const uint8_t*>(data);
        cmd.size = size;

        device->GetClient()->SerializeCommand(cmd);
    }

    void Queue::WriteTexture(const WGPUTextureCopyView* destination,
                             const void* data,
                             size_t dataSize,
                             const WGPUTextureDataLayout* dataLayout,
                             const WGPUExtent3D* writeSize) {
        QueueWriteTextureInternalCmd cmd;
        cmd.queueId = id;
        cmd.destination = destination;
        cmd.data = static_cast<const uint8_t*>(data);
        cmd.dataSize = dataSize;
        cmd.dataLayout = dataLayout;
        cmd.writeSize = writeSize;

        device->GetClient()->SerializeCommand(cmd);
    }

}}  // namespace dawn_wire::client
