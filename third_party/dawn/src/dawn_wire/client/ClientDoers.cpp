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
#include "dawn_wire/client/Client.h"
#include "dawn_wire/client/Device.h"

namespace dawn_wire { namespace client {

    bool Client::DoDeviceErrorCallback(const char* message) {
        DAWN_ASSERT(message != nullptr);
        mDevice->HandleError(message);
        return true;
    }

    bool Client::DoBufferMapReadAsyncCallback(Buffer* buffer,
                                              uint32_t requestSerial,
                                              uint32_t status,
                                              uint64_t dataLength,
                                              const uint8_t* data) {
        // The buffer might have been deleted or recreated so this isn't an error.
        if (buffer == nullptr) {
            return true;
        }

        // The requests can have been deleted via an Unmap so this isn't an error.
        auto requestIt = buffer->requests.find(requestSerial);
        if (requestIt == buffer->requests.end()) {
            return true;
        }

        // It is an error for the server to call the read callback when we asked for a map write
        if (requestIt->second.isWrite) {
            return false;
        }

        auto request = requestIt->second;
        // Delete the request before calling the callback otherwise the callback could be fired a
        // second time. If, for example, buffer.Unmap() is called inside the callback.
        buffer->requests.erase(requestIt);

        // On success, we copy the data locally because the IPC buffer isn't valid outside of this
        // function
        if (status == DAWN_BUFFER_MAP_ASYNC_STATUS_SUCCESS) {
            ASSERT(data != nullptr);

            if (buffer->mappedData != nullptr) {
                return false;
            }

            buffer->isWriteMapped = false;
            buffer->mappedDataSize = dataLength;
            buffer->mappedData = malloc(dataLength);
            memcpy(buffer->mappedData, data, dataLength);

            request.readCallback(static_cast<DawnBufferMapAsyncStatus>(status), buffer->mappedData,
                                 dataLength, request.userdata);
        } else {
            request.readCallback(static_cast<DawnBufferMapAsyncStatus>(status), nullptr, 0,
                                 request.userdata);
        }

        return true;
    }

    bool Client::DoBufferMapWriteAsyncCallback(Buffer* buffer,
                                               uint32_t requestSerial,
                                               uint32_t status,
                                               uint64_t dataLength) {
        // The buffer might have been deleted or recreated so this isn't an error.
        if (buffer == nullptr) {
            return true;
        }

        // The requests can have been deleted via an Unmap so this isn't an error.
        auto requestIt = buffer->requests.find(requestSerial);
        if (requestIt == buffer->requests.end()) {
            return true;
        }

        // It is an error for the server to call the write callback when we asked for a map read
        if (!requestIt->second.isWrite) {
            return false;
        }

        auto request = requestIt->second;
        // Delete the request before calling the callback otherwise the callback could be fired a
        // second time. If, for example, buffer.Unmap() is called inside the callback.
        buffer->requests.erase(requestIt);

        // On success, we copy the data locally because the IPC buffer isn't valid outside of this
        // function
        if (status == DAWN_BUFFER_MAP_ASYNC_STATUS_SUCCESS) {
            if (buffer->mappedData != nullptr) {
                return false;
            }

            buffer->isWriteMapped = true;
            buffer->mappedDataSize = dataLength;
            // |mappedData| is freed in Unmap or the Buffer destructor.
            // TODO(enga): Add dependency injection for buffer mapping so staging
            // memory can live in shared memory.
            buffer->mappedData = malloc(dataLength);
            memset(buffer->mappedData, 0, dataLength);

            request.writeCallback(static_cast<DawnBufferMapAsyncStatus>(status), buffer->mappedData,
                                  dataLength, request.userdata);
        } else {
            request.writeCallback(static_cast<DawnBufferMapAsyncStatus>(status), nullptr, 0,
                                  request.userdata);
        }

        return true;
    }

    bool Client::DoFenceUpdateCompletedValue(Fence* fence, uint64_t value) {
        // The fence might have been deleted or recreated so this isn't an error.
        if (fence == nullptr) {
            return true;
        }

        fence->completedValue = value;
        fence->CheckPassedFences();
        return true;
    }

}}  // namespace dawn_wire::client
