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

#include "dawn_wire/WireClient.h"
#include "dawn_wire/client/Client.h"

namespace dawn_wire {

    WireClient::WireClient(const WireClientDescriptor& descriptor)
        : mImpl(new client::Client(descriptor.serializer, descriptor.memoryTransferService)) {
    }

    WireClient::~WireClient() {
        mImpl.reset();
    }

    // static
    DawnProcTable WireClient::GetProcs() {
        return client::GetProcs();
    }

    WGPUDevice WireClient::GetDevice() const {
        return mImpl->GetDevice();
    }

    const volatile char* WireClient::HandleCommands(const volatile char* commands, size_t size) {
        return mImpl->HandleCommands(commands, size);
    }

    ReservedTexture WireClient::ReserveTexture(WGPUDevice device) {
        return mImpl->ReserveTexture(device);
    }

    void WireClient::Disconnect() {
        mImpl->Disconnect();
    }

    namespace client {
        MemoryTransferService::~MemoryTransferService() = default;

        MemoryTransferService::ReadHandle*
        MemoryTransferService::CreateReadHandle(WGPUBuffer buffer, uint64_t offset, size_t size) {
            return CreateReadHandle(size);
        }

        MemoryTransferService::WriteHandle*
        MemoryTransferService::CreateWriteHandle(WGPUBuffer buffer, uint64_t offset, size_t size) {
            return CreateWriteHandle(size);
        }

        MemoryTransferService::ReadHandle::~ReadHandle() = default;

        MemoryTransferService::WriteHandle::~WriteHandle() = default;
    }  // namespace client

}  // namespace dawn_wire
