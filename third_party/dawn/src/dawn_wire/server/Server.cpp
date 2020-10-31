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

#include "dawn_wire/server/Server.h"
#include "dawn_wire/WireServer.h"

namespace dawn_wire { namespace server {

    Server::Server(WGPUDevice device,
                   const DawnProcTable& procs,
                   CommandSerializer* serializer,
                   MemoryTransferService* memoryTransferService)
        : mSerializer(serializer), mProcs(procs), mMemoryTransferService(memoryTransferService) {
        if (mMemoryTransferService == nullptr) {
            // If a MemoryTransferService is not provided, fallback to inline memory.
            mOwnedMemoryTransferService = CreateInlineMemoryTransferService();
            mMemoryTransferService = mOwnedMemoryTransferService.get();
        }
        // The client-server knowledge is bootstrapped with device 1.
        auto* deviceData = DeviceObjects().Allocate(1);
        deviceData->handle = device;

        mProcs.deviceSetUncapturedErrorCallback(device, ForwardUncapturedError, this);
        mProcs.deviceSetDeviceLostCallback(device, ForwardDeviceLost, this);
    }

    Server::~Server() {
        DestroyAllObjects(mProcs);
    }

    char* Server::GetCmdSpace(size_t size) {
        return static_cast<char*>(mSerializer->GetCmdSpace(size));
    }

    bool Server::InjectTexture(WGPUTexture texture, uint32_t id, uint32_t generation) {
        ObjectData<WGPUTexture>* data = TextureObjects().Allocate(id);
        if (data == nullptr) {
            return false;
        }

        data->handle = texture;
        data->generation = generation;
        data->allocated = true;

        // The texture is externally owned so it shouldn't be destroyed when we receive a destroy
        // message from the client. Add a reference to counterbalance the eventual release.
        mProcs.textureReference(texture);

        return true;
    }

}}  // namespace dawn_wire::server
