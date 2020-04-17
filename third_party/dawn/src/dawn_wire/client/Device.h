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

#ifndef DAWNWIRE_CLIENT_DEVICE_H_
#define DAWNWIRE_CLIENT_DEVICE_H_

#include <dawn/dawn.h>

#include "dawn_wire/client/ObjectBase.h"

namespace dawn_wire { namespace client {

    class Client;

    class Device : public ObjectBase {
      public:
        Device(Client* client, uint32_t refcount, uint32_t id);

        Client* GetClient();
        void HandleError(const char* message);
        void SetErrorCallback(DawnDeviceErrorCallback errorCallback, void* errorUserdata);

      private:
        Client* mClient = nullptr;
        DawnDeviceErrorCallback mErrorCallback = nullptr;
        void* mErrorUserdata;
    };

}}  // namespace dawn_wire::client

#endif  // DAWNWIRE_CLIENT_DEVICE_H_
