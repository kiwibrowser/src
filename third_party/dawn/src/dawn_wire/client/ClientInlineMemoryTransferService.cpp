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
#include "dawn_wire/WireClient.h"
#include "dawn_wire/client/Client.h"

#include <cstring>

namespace dawn_wire { namespace client {

    class InlineMemoryTransferService : public MemoryTransferService {
        class ReadHandleImpl : public ReadHandle {
          public:
            explicit ReadHandleImpl(size_t size) : mSize(size) {
            }

            ~ReadHandleImpl() override = default;

            size_t SerializeCreateSize() override {
                return 0;
            }

            void SerializeCreate(void*) override {
            }

            bool DeserializeInitialData(const void* deserializePointer,
                                        size_t deserializeSize,
                                        const void** data,
                                        size_t* dataLength) override {
                if (deserializeSize != mSize || deserializePointer == nullptr) {
                    return false;
                }

                mStagingData = std::unique_ptr<uint8_t[]>(new (std::nothrow) uint8_t[mSize]);
                if (!mStagingData) {
                    return false;
                }
                memcpy(mStagingData.get(), deserializePointer, mSize);

                ASSERT(data != nullptr);
                ASSERT(dataLength != nullptr);
                *data = mStagingData.get();
                *dataLength = mSize;

                return true;
            }

          private:
            size_t mSize;
            std::unique_ptr<uint8_t[]> mStagingData;
        };

        class WriteHandleImpl : public WriteHandle {
          public:
            explicit WriteHandleImpl(size_t size) : mSize(size) {
            }

            ~WriteHandleImpl() override = default;

            size_t SerializeCreateSize() override {
                return 0;
            }

            void SerializeCreate(void*) override {
            }

            std::pair<void*, size_t> Open() override {
                mStagingData = std::unique_ptr<uint8_t[]>(new (std::nothrow) uint8_t[mSize]);
                if (!mStagingData) {
                    return std::make_pair(nullptr, 0);
                }
                memset(mStagingData.get(), 0, mSize);
                return std::make_pair(mStagingData.get(), mSize);
            }

            size_t SerializeFlushSize() override {
                return mSize;
            }

            void SerializeFlush(void* serializePointer) override {
                ASSERT(mStagingData != nullptr);
                ASSERT(serializePointer != nullptr);
                memcpy(serializePointer, mStagingData.get(), mSize);
            }

          private:
            size_t mSize;
            std::unique_ptr<uint8_t[]> mStagingData;
        };

      public:
        InlineMemoryTransferService() {
        }
        ~InlineMemoryTransferService() override = default;

        ReadHandle* CreateReadHandle(size_t size) override {
            return new ReadHandleImpl(size);
        }

        WriteHandle* CreateWriteHandle(size_t size) override {
            return new WriteHandleImpl(size);
        }
    };

    std::unique_ptr<MemoryTransferService> CreateInlineMemoryTransferService() {
        return std::make_unique<InlineMemoryTransferService>();
    }

}}  //  namespace dawn_wire::client
