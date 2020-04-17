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

#include "dawn_wire/client/ApiObjects.h"
#include "dawn_wire/client/ApiProcs_autogen.h"
#include "dawn_wire/client/Client.h"

namespace dawn_wire { namespace client {

    void ClientBufferMapReadAsync(DawnBuffer cBuffer,
                                  DawnBufferMapReadCallback callback,
                                  void* userdata) {
        Buffer* buffer = reinterpret_cast<Buffer*>(cBuffer);

        uint32_t serial = buffer->requestSerial++;
        ASSERT(buffer->requests.find(serial) == buffer->requests.end());

        Buffer::MapRequestData request;
        request.readCallback = callback;
        request.userdata = userdata;
        request.isWrite = false;
        buffer->requests[serial] = request;

        BufferMapAsyncCmd cmd;
        cmd.bufferId = buffer->id;
        cmd.requestSerial = serial;
        cmd.isWrite = false;

        size_t requiredSize = cmd.GetRequiredSize();
        char* allocatedBuffer =
            static_cast<char*>(buffer->device->GetClient()->GetCmdSpace(requiredSize));
        cmd.Serialize(allocatedBuffer);
    }

    void ClientBufferMapWriteAsync(DawnBuffer cBuffer,
                                   DawnBufferMapWriteCallback callback,
                                   void* userdata) {
        Buffer* buffer = reinterpret_cast<Buffer*>(cBuffer);

        uint32_t serial = buffer->requestSerial++;
        ASSERT(buffer->requests.find(serial) == buffer->requests.end());

        Buffer::MapRequestData request;
        request.writeCallback = callback;
        request.userdata = userdata;
        request.isWrite = true;
        buffer->requests[serial] = request;

        BufferMapAsyncCmd cmd;
        cmd.bufferId = buffer->id;
        cmd.requestSerial = serial;
        cmd.isWrite = true;

        size_t requiredSize = cmd.GetRequiredSize();
        char* allocatedBuffer =
            static_cast<char*>(buffer->device->GetClient()->GetCmdSpace(requiredSize));
        cmd.Serialize(allocatedBuffer);
    }

    DawnCreateBufferMappedResult ClientDeviceCreateBufferMapped(
        DawnDevice cDevice,
        const DawnBufferDescriptor* descriptor) {
        Device* device = reinterpret_cast<Device*>(cDevice);
        Client* wireClient = device->GetClient();

        auto* bufferObjectAndSerial = wireClient->BufferAllocator().New(device);
        Buffer* buffer = bufferObjectAndSerial->object.get();
        buffer->isWriteMapped = true;
        // |mappedData| is freed in Unmap or the Buffer destructor.
        // TODO(enga): Add dependency injection for buffer mapping so staging
        // memory can live in shared memory.
        buffer->mappedData = malloc(descriptor->size);
        memset(buffer->mappedData, 0, descriptor->size);
        buffer->mappedDataSize = descriptor->size;

        DeviceCreateBufferMappedCmd cmd;
        cmd.device = cDevice;
        cmd.descriptor = descriptor;
        cmd.result = ObjectHandle{buffer->id, bufferObjectAndSerial->serial};

        size_t requiredSize = cmd.GetRequiredSize();
        char* allocatedBuffer = static_cast<char*>(wireClient->GetCmdSpace(requiredSize));
        cmd.Serialize(allocatedBuffer, *wireClient);

        DawnCreateBufferMappedResult result;
        result.buffer = reinterpret_cast<DawnBuffer>(buffer);
        result.data = reinterpret_cast<uint8_t*>(buffer->mappedData);
        result.dataLength = descriptor->size;

        return result;
    }

    uint64_t ClientFenceGetCompletedValue(DawnFence cSelf) {
        auto fence = reinterpret_cast<Fence*>(cSelf);
        return fence->completedValue;
    }

    void ClientFenceOnCompletion(DawnFence cFence,
                                 uint64_t value,
                                 DawnFenceOnCompletionCallback callback,
                                 void* userdata) {
        Fence* fence = reinterpret_cast<Fence*>(cFence);
        if (value > fence->signaledValue) {
            fence->device->HandleError("Value greater than fence signaled value");
            callback(DAWN_FENCE_COMPLETION_STATUS_ERROR, userdata);
            return;
        }

        if (value <= fence->completedValue) {
            callback(DAWN_FENCE_COMPLETION_STATUS_SUCCESS, userdata);
            return;
        }

        Fence::OnCompletionData request;
        request.completionCallback = callback;
        request.userdata = userdata;
        fence->requests.Enqueue(std::move(request), value);
    }

    void ClientBufferSetSubData(DawnBuffer cBuffer,
                                uint64_t start,
                                uint64_t count,
                                const void* data) {
        Buffer* buffer = reinterpret_cast<Buffer*>(cBuffer);

        BufferSetSubDataInternalCmd cmd;
        cmd.bufferId = buffer->id;
        cmd.start = start;
        cmd.count = count;
        cmd.data = static_cast<const uint8_t*>(data);

        Client* wireClient = buffer->device->GetClient();
        size_t requiredSize = cmd.GetRequiredSize();
        char* allocatedBuffer = static_cast<char*>(wireClient->GetCmdSpace(requiredSize));
        cmd.Serialize(allocatedBuffer);
    }

    void ClientBufferUnmap(DawnBuffer cBuffer) {
        Buffer* buffer = reinterpret_cast<Buffer*>(cBuffer);

        // Invalidate the local pointer, and cancel all other in-flight requests that would
        // turn into errors anyway (you can't double map). This prevents race when the following
        // happens, where the application code would have unmapped a buffer but still receive a
        // callback:
        //   - Client -> Server: MapRequest1, Unmap, MapRequest2
        //   - Server -> Client: Result of MapRequest1
        //   - Unmap locally on the client
        //   - Server -> Client: Result of MapRequest2
        if (buffer->mappedData) {
            // If the buffer was mapped for writing, send the update to the data to the server
            if (buffer->isWriteMapped) {
                BufferUpdateMappedDataCmd cmd;
                cmd.bufferId = buffer->id;
                cmd.dataLength = static_cast<uint32_t>(buffer->mappedDataSize);
                cmd.data = static_cast<const uint8_t*>(buffer->mappedData);

                size_t requiredSize = cmd.GetRequiredSize();
                char* allocatedBuffer =
                    static_cast<char*>(buffer->device->GetClient()->GetCmdSpace(requiredSize));
                cmd.Serialize(allocatedBuffer);
            }

            free(buffer->mappedData);
            buffer->mappedData = nullptr;
        }
        buffer->ClearMapRequests(DAWN_BUFFER_MAP_ASYNC_STATUS_UNKNOWN);

        BufferUnmapCmd cmd;
        cmd.self = cBuffer;
        size_t requiredSize = cmd.GetRequiredSize();
        char* allocatedBuffer =
            static_cast<char*>(buffer->device->GetClient()->GetCmdSpace(requiredSize));
        cmd.Serialize(allocatedBuffer, *buffer->device->GetClient());
    }

    DawnFence ClientQueueCreateFence(DawnQueue cSelf, DawnFenceDescriptor const* descriptor) {
        Queue* queue = reinterpret_cast<Queue*>(cSelf);
        Device* device = queue->device;

        QueueCreateFenceCmd cmd;
        cmd.self = cSelf;
        auto* allocation = device->GetClient()->FenceAllocator().New(device);
        cmd.result = ObjectHandle{allocation->object->id, allocation->serial};
        cmd.descriptor = descriptor;

        size_t requiredSize = cmd.GetRequiredSize();
        char* allocatedBuffer = static_cast<char*>(device->GetClient()->GetCmdSpace(requiredSize));
        cmd.Serialize(allocatedBuffer, *device->GetClient());

        DawnFence cFence = reinterpret_cast<DawnFence>(allocation->object.get());

        Fence* fence = reinterpret_cast<Fence*>(cFence);
        fence->queue = queue;
        fence->signaledValue = descriptor->initialValue;
        fence->completedValue = descriptor->initialValue;
        return cFence;
    }

    void ClientQueueSignal(DawnQueue cQueue, DawnFence cFence, uint64_t signalValue) {
        Fence* fence = reinterpret_cast<Fence*>(cFence);
        Queue* queue = reinterpret_cast<Queue*>(cQueue);
        if (fence->queue != queue) {
            fence->device->HandleError(
                "Fence must be signaled on the queue on which it was created.");
            return;
        }
        if (signalValue <= fence->signaledValue) {
            fence->device->HandleError("Fence value less than or equal to signaled value");
            return;
        }
        fence->signaledValue = signalValue;

        QueueSignalCmd cmd;
        cmd.self = cQueue;
        cmd.fence = cFence;
        cmd.signalValue = signalValue;

        size_t requiredSize = cmd.GetRequiredSize();
        char* allocatedBuffer =
            static_cast<char*>(fence->device->GetClient()->GetCmdSpace(requiredSize));
        cmd.Serialize(allocatedBuffer, *fence->device->GetClient());
    }

    void ClientDeviceReference(DawnDevice) {
    }

    void ClientDeviceRelease(DawnDevice) {
    }

    void ClientDeviceSetErrorCallback(DawnDevice cSelf,
                                      DawnDeviceErrorCallback callback,
                                      void* userdata) {
        Device* device = reinterpret_cast<Device*>(cSelf);
        device->SetErrorCallback(callback, userdata);
    }

}}  // namespace dawn_wire::client
