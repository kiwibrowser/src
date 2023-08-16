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

#include "common/Assert.h"
#include "dawn_wire/WireServer.h"
#include "dawn_wire/server/Server.h"

#include <cstring>

namespace dawn_wire { namespace server {

    class InlineMemoryTransferService : public MemoryTransferService {
      public:
        class ReadHandleImpl : public ReadHandle {
          public:
            ReadHandleImpl() {
            }
            ~ReadHandleImpl() override = default;

            size_t SerializeInitialDataSize(const void* data, size_t dataLength) override {
                return dataLength;
            }

            void SerializeInitialData(const void* data,
                                      size_t dataLength,
                                      void* serializePointer) override {
                if (dataLength > 0) {
                    ASSERT(data != nullptr);
                    ASSERT(serializePointer != nullptr);
                    memcpy(serializePointer, data, dataLength);
                }
            }
        };

        class WriteHandleImpl : public WriteHandle {
          public:
            WriteHandleImpl() {
            }
            ~WriteHandleImpl() override = default;

            bool DeserializeFlush(const void* deserializePointer, size_t deserializeSize) override {
                if (deserializeSize != mDataLength || mTargetData == nullptr ||
                    deserializePointer == nullptr) {
                    return false;
                }
                memcpy(mTargetData, deserializePointer, mDataLength);
                return true;
            }
        };

        InlineMemoryTransferService() {
        }
        ~InlineMemoryTransferService() override = default;

        bool DeserializeReadHandle(const void* deserializePointer,
                                   size_t deserializeSize,
                                   ReadHandle** readHandle) override {
            ASSERT(readHandle != nullptr);
            *readHandle = new ReadHandleImpl();
            return true;
        }

        bool DeserializeWriteHandle(const void* deserializePointer,
                                    size_t deserializeSize,
                                    WriteHandle** writeHandle) override {
            ASSERT(writeHandle != nullptr);
            *writeHandle = new WriteHandleImpl();
            return true;
        }
    };

    std::unique_ptr<MemoryTransferService> CreateInlineMemoryTransferService() {
        return std::make_unique<InlineMemoryTransferService>();
    }

}}  //  namespace dawn_wire::server
