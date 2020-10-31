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

        // The buffer was unmapped. Clear the Read/WriteHandle.
        buffer->readHandle = nullptr;
        buffer->writeHandle = nullptr;
        buffer->mapWriteState = BufferMapWriteState::Unmapped;

        return true;
    }

    bool Server::PreHandleBufferDestroy(const BufferDestroyCmd& cmd) {
        // Destroying a buffer does an implicit unmapping.
        auto* buffer = BufferObjects().Get(cmd.selfId);
        DAWN_ASSERT(buffer != nullptr);

        // The buffer was destroyed. Clear the Read/WriteHandle.
        buffer->readHandle = nullptr;
        buffer->writeHandle = nullptr;
        buffer->mapWriteState = BufferMapWriteState::Unmapped;

        return true;
    }

    bool Server::DoBufferMapAsync(ObjectId bufferId,
                                  uint32_t requestSerial,
                                  WGPUMapModeFlags mode,
                                  size_t offset,
                                  size_t size,
                                  uint64_t handleCreateInfoLength,
                                  const uint8_t* handleCreateInfo) {
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

        // The server only knows how to deal with write XOR read. Validate that.
        bool isReadMode = mode & WGPUMapMode_Read;
        bool isWriteMode = mode & WGPUMapMode_Write;
        if (!(isReadMode ^ isWriteMode)) {
            return false;
        }

        if (handleCreateInfoLength > std::numeric_limits<size_t>::max()) {
            // This is the size of data deserialized from the command stream, which must be
            // CPU-addressable.
            return false;
        }

        std::unique_ptr<MapUserdata> userdata = std::make_unique<MapUserdata>();
        userdata->server = this;
        userdata->buffer = ObjectHandle{bufferId, buffer->generation};
        userdata->bufferObj = buffer->handle;
        userdata->requestSerial = requestSerial;
        userdata->offset = offset;
        userdata->size = size;
        userdata->mode = mode;

        // The handle will point to the mapped memory or staging memory for the mapping.
        // Store it on the map request.
        if (isWriteMode) {
            // Deserialize metadata produced from the client to create a companion server handle.
            MemoryTransferService::WriteHandle* writeHandle = nullptr;
            if (!mMemoryTransferService->DeserializeWriteHandle(
                    handleCreateInfo, static_cast<size_t>(handleCreateInfoLength), &writeHandle)) {
                return false;
            }
            ASSERT(writeHandle != nullptr);

            userdata->writeHandle =
                std::unique_ptr<MemoryTransferService::WriteHandle>(writeHandle);
        } else {
            ASSERT(isReadMode);
            // Deserialize metadata produced from the client to create a companion server handle.
            MemoryTransferService::ReadHandle* readHandle = nullptr;
            if (!mMemoryTransferService->DeserializeReadHandle(
                    handleCreateInfo, static_cast<size_t>(handleCreateInfoLength), &readHandle)) {
                return false;
            }
            ASSERT(readHandle != nullptr);

            userdata->readHandle = std::unique_ptr<MemoryTransferService::ReadHandle>(readHandle);
        }

        mProcs.bufferMapAsync(buffer->handle, mode, offset, size, ForwardBufferMapAsync,
                              userdata.release());

        return true;
    }

    bool Server::DoDeviceCreateBufferMapped(WGPUDevice device,
                                            const WGPUBufferDescriptor* descriptor,
                                            ObjectHandle bufferResult,
                                            uint64_t handleCreateInfoLength,
                                            const uint8_t* handleCreateInfo) {
        if (handleCreateInfoLength > std::numeric_limits<size_t>::max()) {
            // This is the size of data deserialized from the command stream, which must be
            // CPU-addressable.
            return false;
        }

        auto* resultData = BufferObjects().Allocate(bufferResult.id);
        if (resultData == nullptr) {
            return false;
        }
        resultData->generation = bufferResult.generation;

        WGPUCreateBufferMappedResult result = mProcs.deviceCreateBufferMapped(device, descriptor);
        ASSERT(result.buffer != nullptr);
        if (result.data == nullptr && result.dataLength != 0) {
            // Non-zero dataLength but null data is used to indicate an allocation error.
            // Don't return false because this is not fatal. result.buffer is an ErrorBuffer
            // and subsequent operations will be errors.
            // This should only happen when fuzzing with the Null backend.
            resultData->mapWriteState = BufferMapWriteState::MapError;
        } else {
            // Deserialize metadata produced from the client to create a companion server handle.
            MemoryTransferService::WriteHandle* writeHandle = nullptr;
            if (!mMemoryTransferService->DeserializeWriteHandle(
                    handleCreateInfo, static_cast<size_t>(handleCreateInfoLength), &writeHandle)) {
                return false;
            }
            ASSERT(writeHandle != nullptr);

            // Set the target of the WriteHandle to the mapped GPU memory.
            writeHandle->SetTarget(result.data, result.dataLength);

            // The buffer is mapped and has a valid mappedData pointer.
            // The buffer may still be an error with fake staging data.
            resultData->mapWriteState = BufferMapWriteState::Mapped;
            resultData->writeHandle =
                std::unique_ptr<MemoryTransferService::WriteHandle>(writeHandle);
        }
        resultData->handle = result.buffer;

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

    bool Server::DoBufferUpdateMappedData(ObjectId bufferId,
                                          uint64_t writeFlushInfoLength,
                                          const uint8_t* writeFlushInfo) {
        // The null object isn't valid as `self`
        if (bufferId == 0) {
            return false;
        }

        if (writeFlushInfoLength > std::numeric_limits<size_t>::max()) {
            return false;
        }

        auto* buffer = BufferObjects().Get(bufferId);
        if (buffer == nullptr) {
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
        if (!buffer->writeHandle) {
            // This check is performed after the check for the MapError state. It is permissible
            // to Unmap and attempt to update mapped data of an error buffer.
            return false;
        }

        // Deserialize the flush info and flush updated data from the handle into the target
        // of the handle. The target is set via WriteHandle::SetTarget.
        return buffer->writeHandle->DeserializeFlush(writeFlushInfo,
                                                     static_cast<size_t>(writeFlushInfoLength));
    }

    void Server::ForwardBufferMapAsync(WGPUBufferMapAsyncStatus status, void* userdata) {
        auto data = static_cast<MapUserdata*>(userdata);
        data->server->OnBufferMapAsyncCallback(status, data);
    }

    void Server::OnBufferMapAsyncCallback(WGPUBufferMapAsyncStatus status, MapUserdata* userdata) {
        std::unique_ptr<MapUserdata> data(userdata);

        // Skip sending the callback if the buffer has already been destroyed.
        auto* bufferData = BufferObjects().Get(data->buffer.id);
        if (bufferData == nullptr || bufferData->generation != data->buffer.generation) {
            return;
        }

        bool isRead = data->mode & WGPUMapMode_Read;
        bool isSuccess = status == WGPUBufferMapAsyncStatus_Success;

        ReturnBufferMapAsyncCallbackCmd cmd;
        cmd.buffer = data->buffer;
        cmd.requestSerial = data->requestSerial;
        cmd.status = status;
        cmd.readInitialDataInfoLength = 0;
        cmd.readInitialDataInfo = nullptr;

        const void* readData = nullptr;
        if (isSuccess && isRead) {
            // Get the serialization size of the message to initialize ReadHandle data.
            readData = mProcs.bufferGetConstMappedRange(data->bufferObj, data->offset, data->size);
            cmd.readInitialDataInfoLength =
                data->readHandle->SerializeInitialDataSize(readData, data->size);
        }

        char* readHandleSpace = SerializeCommand(cmd, cmd.readInitialDataInfoLength);

        if (isSuccess) {
            if (isRead) {
                // Serialize the initialization message into the space after the command.
                data->readHandle->SerializeInitialData(readData, data->size, readHandleSpace);
                // The in-flight map request returned successfully.
                // Move the ReadHandle so it is owned by the buffer.
                bufferData->readHandle = std::move(data->readHandle);
            } else {
                // The in-flight map request returned successfully.
                // Move the WriteHandle so it is owned by the buffer.
                bufferData->writeHandle = std::move(data->writeHandle);
                bufferData->mapWriteState = BufferMapWriteState::Mapped;
                // Set the target of the WriteHandle to the mapped buffer data.
                bufferData->writeHandle->SetTarget(
                    mProcs.bufferGetMappedRange(data->bufferObj, data->offset, data->size),
                    data->size);
            }
        }
    }

}}  // namespace dawn_wire::server
