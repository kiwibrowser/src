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

#include "dawn_wire/server/ServerMemoryTransferService_mock.h"

#include "common/Assert.h"

namespace dawn_wire { namespace server {

    MockMemoryTransferService::MockReadHandle::MockReadHandle(MockMemoryTransferService* service)
        : ReadHandle(), mService(service) {
    }

    MockMemoryTransferService::MockReadHandle::~MockReadHandle() {
        mService->OnReadHandleDestroy(this);
    }

    size_t MockMemoryTransferService::MockReadHandle::SerializeInitialDataSize(const void* data,
                                                                               size_t dataLength) {
        return mService->OnReadHandleSerializeInitialDataSize(this, data, dataLength);
    }

    void MockMemoryTransferService::MockReadHandle::SerializeInitialData(const void* data,
                                                                         size_t dataLength,
                                                                         void* serializePointer) {
        mService->OnReadHandleSerializeInitialData(this, data, dataLength, serializePointer);
    }

    MockMemoryTransferService::MockWriteHandle::MockWriteHandle(MockMemoryTransferService* service)
        : WriteHandle(), mService(service) {
    }

    MockMemoryTransferService::MockWriteHandle::~MockWriteHandle() {
        mService->OnWriteHandleDestroy(this);
    }

    bool MockMemoryTransferService::MockWriteHandle::DeserializeFlush(
        const void* deserializePointer,
        size_t deserializeSize) {
        ASSERT(deserializeSize % sizeof(uint32_t) == 0);
        return mService->OnWriteHandleDeserializeFlush(
            this, reinterpret_cast<const uint32_t*>(deserializePointer), deserializeSize);
    }

    const uint32_t* MockMemoryTransferService::MockWriteHandle::GetData() const {
        return reinterpret_cast<const uint32_t*>(mTargetData);
    }

    MockMemoryTransferService::MockMemoryTransferService() = default;
    MockMemoryTransferService::~MockMemoryTransferService() = default;

    bool MockMemoryTransferService::DeserializeReadHandle(const void* deserializePointer,
                                                          size_t deserializeSize,
                                                          ReadHandle** readHandle) {
        ASSERT(deserializeSize % sizeof(uint32_t) == 0);
        return OnDeserializeReadHandle(reinterpret_cast<const uint32_t*>(deserializePointer),
                                       deserializeSize, readHandle);
    }

    bool MockMemoryTransferService::DeserializeWriteHandle(const void* deserializePointer,
                                                           size_t deserializeSize,
                                                           WriteHandle** writeHandle) {
        ASSERT(deserializeSize % sizeof(uint32_t) == 0);
        return OnDeserializeWriteHandle(reinterpret_cast<const uint32_t*>(deserializePointer),
                                        deserializeSize, writeHandle);
    }

    MockMemoryTransferService::MockReadHandle* MockMemoryTransferService::NewReadHandle() {
        return new MockReadHandle(this);
    }

    MockMemoryTransferService::MockWriteHandle* MockMemoryTransferService::NewWriteHandle() {
        return new MockWriteHandle(this);
    }

}}  //  namespace dawn_wire::server
