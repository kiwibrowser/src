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

#ifndef DAWNWIRE_WIRESERVER_H_
#define DAWNWIRE_WIRESERVER_H_

#include <memory>

#include "dawn_wire/Wire.h"

struct DawnProcTable;

namespace dawn_wire {

    namespace server {
        class Server;
        class MemoryTransferService;
    }  // namespace server

    struct DAWN_WIRE_EXPORT WireServerDescriptor {
        WGPUDevice device;
        const DawnProcTable* procs;
        CommandSerializer* serializer;
        server::MemoryTransferService* memoryTransferService = nullptr;
    };

    class DAWN_WIRE_EXPORT WireServer : public CommandHandler {
      public:
        WireServer(const WireServerDescriptor& descriptor);
        ~WireServer() override;

        const volatile char* HandleCommands(const volatile char* commands,
                                            size_t size) override final;

        bool InjectTexture(WGPUTexture texture, uint32_t id, uint32_t generation);

      private:
        std::unique_ptr<server::Server> mImpl;
    };

    namespace server {
        class DAWN_WIRE_EXPORT MemoryTransferService {
          public:
            class ReadHandle;
            class WriteHandle;

            virtual ~MemoryTransferService();

            // Deserialize data to create Read/Write handles. These handles are for the client
            // to Read/Write data.
            virtual bool DeserializeReadHandle(const void* deserializePointer,
                                               size_t deserializeSize,
                                               ReadHandle** readHandle) = 0;
            virtual bool DeserializeWriteHandle(const void* deserializePointer,
                                                size_t deserializeSize,
                                                WriteHandle** writeHandle) = 0;

            class DAWN_WIRE_EXPORT ReadHandle {
              public:
                // Get the required serialization size for SerializeInitialData
                virtual size_t SerializeInitialDataSize(const void* data, size_t dataLength) = 0;

                // Initialize the handle data.
                // Serialize into |serializePointer| so the client can update handle data.
                virtual void SerializeInitialData(const void* data,
                                                  size_t dataLength,
                                                  void* serializePointer) = 0;
                virtual ~ReadHandle();
            };

            class DAWN_WIRE_EXPORT WriteHandle {
              public:
                // Set the target for writes from the client. DeserializeFlush should copy data
                // into the target.
                void SetTarget(void* data, size_t dataLength);

                // This function takes in the serialized result of
                // client::MemoryTransferService::WriteHandle::SerializeFlush.
                virtual bool DeserializeFlush(const void* deserializePointer,
                                              size_t deserializeSize) = 0;
                virtual ~WriteHandle();

              protected:
                void* mTargetData = nullptr;
                size_t mDataLength = 0;
            };
        };
    }  // namespace server

}  // namespace dawn_wire

#endif  // DAWNWIRE_WIRESERVER_H_
