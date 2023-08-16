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

#include "dawn_wire/client/ClientMemoryTransferService_mock.h"

#include <cstdio>
#include "common/Assert.h"

namespace dawn_wire { namespace client {

    MockMemoryTransferService::MockReadHandle::MockReadHandle(MockMemoryTransferService* service)
        : ReadHandle(), mService(service) {
    }

    MockMemoryTransferService::MockReadHandle::~MockReadHandle() {
        mService->OnReadHandleDestroy(this);
    }

    size_t MockMemoryTransferService::MockReadHandle::SerializeCreateSize() {
        return mService->OnReadHandleSerializeCreateSize(this);
    }

    void MockMemoryTransferService::MockReadHandle::SerializeCreate(void* serializePointer) {
        mService->OnReadHandleSerializeCreate(this, serializePointer);
    }

    bool MockMemoryTransferService::MockReadHandle::DeserializeInitialData(
        const void* deserializePointer,
        size_t deserializeSize,
        const void** data,
        size_t* dataLength) {
        ASSERT(deserializeSize % sizeof(uint32_t) == 0);
        return mService->OnReadHandleDeserializeInitialData(
            this, reinterpret_cast<const uint32_t*>(deserializePointer), deserializeSize, data,
            dataLength);
    }

    MockMemoryTransferService::MockWriteHandle::MockWriteHandle(MockMemoryTransferService* service)
        : WriteHandle(), mService(service) {
    }

    MockMemoryTransferService::MockWriteHandle::~MockWriteHandle() {
        mService->OnWriteHandleDestroy(this);
    }

    size_t MockMemoryTransferService::MockWriteHandle::SerializeCreateSize() {
        return mService->OnWriteHandleSerializeCreateSize(this);
    }

    void MockMemoryTransferService::MockWriteHandle::SerializeCreate(void* serializePointer) {
        mService->OnWriteHandleSerializeCreate(this, serializePointer);
    }

    std::pair<void*, size_t> MockMemoryTransferService::MockWriteHandle::Open() {
        return mService->OnWriteHandleOpen(this);
    }

    size_t MockMemoryTransferService::MockWriteHandle::SerializeFlushSize() {
        return mService->OnWriteHandleSerializeFlushSize(this);
    }

    void MockMemoryTransferService::MockWriteHandle::SerializeFlush(void* serializePointer) {
        mService->OnWriteHandleSerializeFlush(this, serializePointer);
    }

    MockMemoryTransferService::MockMemoryTransferService() = default;
    MockMemoryTransferService::~MockMemoryTransferService() = default;

    MockMemoryTransferService::ReadHandle* MockMemoryTransferService::CreateReadHandle(
        size_t size) {
        return OnCreateReadHandle(size);
    }

    MockMemoryTransferService::WriteHandle* MockMemoryTransferService::CreateWriteHandle(
        size_t size) {
        return OnCreateWriteHandle(size);
    }

    MockMemoryTransferService::MockReadHandle* MockMemoryTransferService::NewReadHandle() {
        return new MockReadHandle(this);
    }

    MockMemoryTransferService::MockWriteHandle* MockMemoryTransferService::NewWriteHandle() {
        return new MockWriteHandle(this);
    }

}}  //  namespace dawn_wire::client
