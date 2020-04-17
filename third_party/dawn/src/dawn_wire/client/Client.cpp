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

#include "dawn_wire/client/Client.h"
#include "dawn_wire/client/Device.h"

namespace dawn_wire { namespace client {

    Client::Client(CommandSerializer* serializer)
        : ClientBase(),
          mDevice(DeviceAllocator().New(this)->object.get()),
          mSerializer(serializer) {
    }

    Client::~Client() {
        DeviceAllocator().Free(mDevice);
    }

    ReservedTexture Client::ReserveTexture(DawnDevice cDevice) {
        Device* device = reinterpret_cast<Device*>(cDevice);
        ObjectAllocator<Texture>::ObjectAndSerial* allocation = TextureAllocator().New(device);

        ReservedTexture result;
        result.texture = reinterpret_cast<DawnTexture>(allocation->object.get());
        result.id = allocation->object->id;
        result.generation = allocation->serial;
        return result;
    }

}}  // namespace dawn_wire::client
