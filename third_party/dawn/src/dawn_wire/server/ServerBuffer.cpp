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

#include "common/Assert.h"
#include "dawn_wire/server/Server.h"

#include <memory>

namespace dawn_wire { namespace server {

    bool Server::PreHandleBufferUnmap(const BufferUnmapCmd& cmd) {
        auto* buffer = BufferObjects().Get(cmd.selfId);
        DAWN_ASSERT(buffer != nullptr);

        buffer->mappedData = nullptr;
        buffer->mapWriteState = BufferMapWriteState::Unmapped;

        return true;
    }

    bool Server::DoBufferMapAsync(ObjectId bufferId,
                                  uint32_t requestSerial,
                                  bool isWrite) {
        // These requests are just forwarded to the buffer, with userdata containing what the
        // client will require in the return command.

        // The null object isn't valid as `self`
        if (bufferId == 0) {
            return false;
        }

        auto* buffer = BufferObjects().Get(bufferId);
        if (buffer == nullptr) {
            return false;
        }

        MapUserdata* userdata = new MapUserdata;
        userdata->server = this;
        userdata->buffer = ObjectHandle{bufferId, buffer->serial};
        userdata->requestSerial = requestSerial;
        userdata->isWrite = isWrite;

        if (isWrite) {
            mProcs.bufferMapWriteAsync(buffer->handle, ForwardBufferMapWriteAsync, userdata);
        } else {
            mProcs.bufferMapReadAsync(buffer->handle, ForwardBufferMapReadAsync, userdata);
        }

        return true;
    }

    bool Server::DoDeviceCreateBufferMapped(DawnDevice device,
                                            const DawnBufferDescriptor* descriptor,
                                            ObjectHandle bufferResult) {
        auto* resultData = BufferObjects().Allocate(bufferResult.id);
        if (resultData == nullptr) {
            return false;
        }
        resultData->serial = bufferResult.serial;

        DawnCreateBufferMappedResult result = mProcs.deviceCreateBufferMapped(device, descriptor);
        ASSERT(result.buffer != nullptr);
        if (result.data == nullptr && result.dataLength != 0) {
            // Non-zero dataLength but null data is used to indicate an allocation error.
            resultData->mapWriteState = BufferMapWriteState::MapError;
        } else {
            // The buffer is mapped and has a valid mappedData pointer.
            // The buffer may still be an error with fake staging data.
            resultData->mapWriteState = BufferMapWriteState::Mapped;
        }
        resultData->handle = result.buffer;
        resultData->mappedData = result.data;
        resultData->mappedDataSize = result.dataLength;

        return true;
    }

    bool Server::DoBufferSetSubDataInternal(ObjectId bufferId,
                                            uint64_t start,
                                            uint64_t offset,
                                            const uint8_t* data) {
        // The null object isn't valid as `self`
        if (bufferId == 0) {
            return false;
        }

        auto* buffer = BufferObjects().Get(bufferId);
        if (buffer == nullptr) {
            return false;
        }

        mProcs.bufferSetSubData(buffer->handle, start, offset, data);
        return true;
    }

    bool Server::DoBufferUpdateMappedData(ObjectId bufferId, uint32_t count, const uint8_t* data) {
        // The null object isn't valid as `self`
        if (bufferId == 0) {
            return false;
        }

        auto* buffer = BufferObjects().Get(bufferId);
        if (buffer == nullptr || buffer->mappedDataSize != count) {
            return false;
        }
        switch (buffer->mapWriteState) {
            case BufferMapWriteState::Unmapped:
                return false;
            case BufferMapWriteState::MapError:
                // The buffer is mapped but there was an error allocating mapped data.
                // Do not perform the memcpy.
                return true;
            case BufferMapWriteState::Mapped:
                break;
        }

        ASSERT(data != nullptr);
        ASSERT(buffer->mappedData != nullptr);
        memcpy(buffer->mappedData, data, count);

        return true;
    }

    void Server::ForwardBufferMapReadAsync(DawnBufferMapAsyncStatus status,
                                           const void* ptr,
                                           uint64_t dataLength,
                                           void* userdata) {
        auto data = static_cast<MapUserdata*>(userdata);
        data->server->OnBufferMapReadAsyncCallback(status, ptr, dataLength, data);
    }

    void Server::ForwardBufferMapWriteAsync(DawnBufferMapAsyncStatus status,
                                            void* ptr,
                                            uint64_t dataLength,
                                            void* userdata) {
        auto data = static_cast<MapUserdata*>(userdata);
        data->server->OnBufferMapWriteAsyncCallback(status, ptr, dataLength, data);
    }

    void Server::OnBufferMapReadAsyncCallback(DawnBufferMapAsyncStatus status,
                                              const void* ptr,
                                              uint64_t dataLength,
                                              MapUserdata* userdata) {
        std::unique_ptr<MapUserdata> data(userdata);

        // Skip sending the callback if the buffer has already been destroyed.
        auto* bufferData = BufferObjects().Get(data->buffer.id);
        if (bufferData == nullptr || bufferData->serial != data->buffer.serial) {
            return;
        }

        ReturnBufferMapReadAsyncCallbackCmd cmd;
        cmd.buffer = data->buffer;
        cmd.requestSerial = data->requestSerial;
        cmd.status = status;
        cmd.dataLength = 0;
        cmd.data = static_cast<const uint8_t*>(ptr);

        if (status == DAWN_BUFFER_MAP_ASYNC_STATUS_SUCCESS) {
            cmd.dataLength = dataLength;
        }

        size_t requiredSize = cmd.GetRequiredSize();
        char* allocatedBuffer = static_cast<char*>(GetCmdSpace(requiredSize));
        cmd.Serialize(allocatedBuffer);
    }

    void Server::OnBufferMapWriteAsyncCallback(DawnBufferMapAsyncStatus status,
                                               void* ptr,
                                               uint64_t dataLength,
                                               MapUserdata* userdata) {
        std::unique_ptr<MapUserdata> data(userdata);

        // Skip sending the callback if the buffer has already been destroyed.
        auto* bufferData = BufferObjects().Get(data->buffer.id);
        if (bufferData == nullptr || bufferData->serial != data->buffer.serial) {
            return;
        }

        ReturnBufferMapWriteAsyncCallbackCmd cmd;
        cmd.buffer = data->buffer;
        cmd.requestSerial = data->requestSerial;
        cmd.status = status;
        cmd.dataLength = dataLength;

        size_t requiredSize = cmd.GetRequiredSize();
        char* allocatedBuffer = static_cast<char*>(GetCmdSpace(requiredSize));
        cmd.Serialize(allocatedBuffer);

        if (status == DAWN_BUFFER_MAP_ASYNC_STATUS_SUCCESS) {
            bufferData->mapWriteState = BufferMapWriteState::Mapped;
            bufferData->mappedData = ptr;
            bufferData->mappedDataSize = dataLength;
        }
    }

}}  // namespace dawn_wire::server
