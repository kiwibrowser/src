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

namespace dawn_wire { namespace server {

    Server::Server(DawnDevice device, const DawnProcTable& procs, CommandSerializer* serializer)
        : mSerializer(serializer), mProcs(procs) {
        // The client-server knowledge is bootstrapped with device 1.
        auto* deviceData = DeviceObjects().Allocate(1);
        deviceData->handle = device;

        mProcs.deviceSetErrorCallback(device, ForwardDeviceError, this);
    }

    Server::~Server() {
        DestroyAllObjects(mProcs);
    }

    void* Server::GetCmdSpace(size_t size) {
        return mSerializer->GetCmdSpace(size);
    }

    bool Server::InjectTexture(DawnTexture texture, uint32_t id, uint32_t generation) {
        ObjectData<DawnTexture>* data = TextureObjects().Allocate(id);
        if (data == nullptr) {
            return false;
        }

        data->handle = texture;
        data->serial = generation;
        data->allocated = true;

        // The texture is externally owned so it shouldn't be destroyed when we receive a destroy
        // message from the client. Add a reference to counterbalance the eventual release.
        mProcs.textureReference(texture);

        return true;
    }

}}  // namespace dawn_wire::server
