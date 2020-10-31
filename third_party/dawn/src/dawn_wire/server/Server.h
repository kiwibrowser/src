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
    class MemoryTransferService;

    struct MapUserdata {
        Server* server;
        ObjectHandle buffer;
        WGPUBuffer bufferObj;
        uint32_t requestSerial;
        uint64_t offset;
        uint64_t size;
        WGPUMapModeFlags mode;
        // TODO(enga): Use a tagged pointer to save space.
        std::unique_ptr<MemoryTransferService::ReadHandle> readHandle = nullptr;
        std::unique_ptr<MemoryTransferService::WriteHandle> writeHandle = nullptr;
    };

    struct ErrorScopeUserdata {
        Server* server;
        // TODO(enga): ObjectHandle device;
        // when the wire supports multiple devices.
        uint64_t requestSerial;
    };

    struct FenceCompletionUserdata {
        Server* server;
        ObjectHandle fence;
        uint64_t value;
    };

    class Server : public ServerBase {
      public:
        Server(WGPUDevice device,
               const DawnProcTable& procs,
               CommandSerializer* serializer,
               MemoryTransferService* memoryTransferService);
        ~Server();

        const volatile char* HandleCommands(const volatile char* commands, size_t size);

        bool InjectTexture(WGPUTexture texture, uint32_t id, uint32_t generation);

      private:
        template <typename Cmd>
        char* SerializeCommand(const Cmd& cmd, size_t extraSize = 0) {
            size_t requiredSize = cmd.GetRequiredSize();
            // TODO(cwallez@chromium.org): Check for overflows and allocation success?
            char* allocatedBuffer = GetCmdSpace(requiredSize + extraSize);
            cmd.Serialize(allocatedBuffer);
            return allocatedBuffer + requiredSize;
        }
        char* GetCmdSpace(size_t size);

        // Forwarding callbacks
        static void ForwardUncapturedError(WGPUErrorType type, const char* message, void* userdata);
        static void ForwardDeviceLost(const char* message, void* userdata);
        static void ForwardPopErrorScope(WGPUErrorType type, const char* message, void* userdata);
        static void ForwardBufferMapAsync(WGPUBufferMapAsyncStatus status, void* userdata);
        static void ForwardFenceCompletedValue(WGPUFenceCompletionStatus status, void* userdata);

        // Error callbacks
        void OnUncapturedError(WGPUErrorType type, const char* message);
        void OnDeviceLost(const char* message);
        void OnDevicePopErrorScope(WGPUErrorType type,
                                   const char* message,
                                   ErrorScopeUserdata* userdata);
        void OnBufferMapAsyncCallback(WGPUBufferMapAsyncStatus status, MapUserdata* userdata);
        void OnFenceCompletedValueUpdated(WGPUFenceCompletionStatus status,
                                          FenceCompletionUserdata* userdata);

#include "dawn_wire/server/ServerPrototypes_autogen.inc"

        CommandSerializer* mSerializer = nullptr;
        WireDeserializeAllocator mAllocator;
        DawnProcTable mProcs;
        std::unique_ptr<MemoryTransferService> mOwnedMemoryTransferService = nullptr;
        MemoryTransferService* mMemoryTransferService = nullptr;
    };

    std::unique_ptr<MemoryTransferService> CreateInlineMemoryTransferService();

}}  // namespace dawn_wire::server

#endif  // DAWNWIRE_SERVER_SERVER_H_
