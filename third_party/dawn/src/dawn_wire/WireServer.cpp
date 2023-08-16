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

#include "dawn_wire/WireServer.h"
#include "dawn_wire/server/Server.h"

namespace dawn_wire {

    WireServer::WireServer(const WireServerDescriptor& descriptor)
        : mImpl(new server::Server(descriptor.device,
                                   *descriptor.procs,
                                   descriptor.serializer,
                                   descriptor.memoryTransferService)) {
    }

    WireServer::~WireServer() {
        mImpl.reset();
    }

    const volatile char* WireServer::HandleCommands(const volatile char* commands, size_t size) {
        return mImpl->HandleCommands(commands, size);
    }

    bool WireServer::InjectTexture(WGPUTexture texture, uint32_t id, uint32_t generation) {
        return mImpl->InjectTexture(texture, id, generation);
    }

    namespace server {
        MemoryTransferService::~MemoryTransferService() = default;

        MemoryTransferService::ReadHandle::~ReadHandle() = default;

        MemoryTransferService::WriteHandle::~WriteHandle() = default;

        void MemoryTransferService::WriteHandle::SetTarget(void* data, size_t dataLength) {
            mTargetData = data;
            mDataLength = dataLength;
        }
    }  // namespace server

}  // namespace dawn_wire
