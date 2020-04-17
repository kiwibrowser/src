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

#ifndef DAWNWIRE_SERVER_SERVER_H_
#define DAWNWIRE_SERVER_SERVER_H_

#include "dawn_wire/server/ServerBase_autogen.h"

namespace dawn_wire { namespace server {

    class Server;

    struct MapUserdata {
        Server* server;
        ObjectHandle buffer;
        uint32_t requestSerial;
        uint64_t size;
        bool isWrite;
    };

    struct FenceCompletionUserdata {
        Server* server;
        ObjectHandle fence;
        uint64_t value;
    };

    class Server : public ServerBase {
      public:
        Server(DawnDevice device, const DawnProcTable& procs, CommandSerializer* serializer);
        ~Server();

        const char* HandleCommands(const char* commands, size_t size);

        bool InjectTexture(DawnTexture texture, uint32_t id, uint32_t generation);

      private:
        void* GetCmdSpace(size_t size);

        // Forwarding callbacks
        static void ForwardDeviceError(const char* message, void* userdata);
        static void ForwardBufferMapReadAsync(DawnBufferMapAsyncStatus status,
                                              const void* ptr,
                                              uint64_t dataLength,
                                              void* userdata);
        static void ForwardBufferMapWriteAsync(DawnBufferMapAsyncStatus status,
                                               void* ptr,
                                               uint64_t dataLength,
                                               void* userdata);
        static void ForwardFenceCompletedValue(DawnFenceCompletionStatus status, void* userdata);

        // Error callbacks
        void OnDeviceError(const char* message);
        void OnBufferMapReadAsyncCallback(DawnBufferMapAsyncStatus status,
                                          const void* ptr,
                                          uint64_t dataLength,
                                          MapUserdata* userdata);
        void OnBufferMapWriteAsyncCallback(DawnBufferMapAsyncStatus status,
                                           void* ptr,
                                           uint64_t dataLength,
                                           MapUserdata* userdata);
        void OnFenceCompletedValueUpdated(DawnFenceCompletionStatus status,
                                          FenceCompletionUserdata* userdata);

#include "dawn_wire/server/ServerPrototypes_autogen.inc"

        CommandSerializer* mSerializer = nullptr;
        WireDeserializeAllocator mAllocator;
        DawnProcTable mProcs;
    };

}}  // namespace dawn_wire::server

#endif  // DAWNWIRE_SERVER_SERVER_H_
