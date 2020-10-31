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

#ifndef DAWNWIRE_CLIENT_OBJECTBASE_H_
#define DAWNWIRE_CLIENT_OBJECTBASE_H_

#include <dawn/webgpu.h>

namespace dawn_wire { namespace client {

    class Device;

    // All non-Device objects of the client side have:
    //  - A pointer to the device to get where to serialize commands
    //  - The external reference count
    //  - An ID that is used to refer to this object when talking with the server side
    struct ObjectBase {
        ObjectBase(Device* device, uint32_t refcount, uint32_t id)
            : device(device), refcount(refcount), id(id) {
        }

        Device* device;
        uint32_t refcount;
        uint32_t id;
    };

}}  // namespace dawn_wire::client

#endif  // DAWNWIRE_CLIENT_OBJECTBASE_H_
