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

#ifndef DAWNWIRE_CLIENT_BUFFER_H_
#define DAWNWIRE_CLIENT_BUFFER_H_

#include <dawn/webgpu.h>

#include "dawn_wire/WireClient.h"
#include "dawn_wire/client/ObjectBase.h"

#include <map>

namespace dawn_wire { namespace client {

    class Buffer : public ObjectBase {
      public:
        using ObjectBase::ObjectBase;

        static WGPUBuffer Create(Device* device, const WGPUBufferDescriptor* descriptor);
        static WGPUCreateBufferMappedResult CreateMapped(Device* device,
                                                         const WGPUBufferDescriptor* descriptor);
        static WGPUBuffer CreateError(Device* device);

        ~Buffer();
        void ClearMapRequests(WGPUBufferMapAsyncStatus status);

        void MapReadAsync(WGPUBufferMapReadCallback callback, void* userdata);
        void MapWriteAsync(WGPUBufferMapWriteCallback callback, void* userdata);
        bool OnMapAsyncCallback(uint32_t requestSerial,
                                uint32_t status,
                                uint64_t readInitialDataInfoLength,
                                const uint8_t* readInitialDataInfo);
        void MapAsync(WGPUMapModeFlags mode,
                      size_t offset,
                      size_t size,
                      WGPUBufferMapCallback callback,
                      void* userdata);
        void* GetMappedRange(size_t offset, size_t size);
        const void* GetConstMappedRange(size_t offset, size_t size);
        void Unmap();

        void Destroy();

        void SetSubData(uint64_t start, uint64_t count, const void* data);

      private:
        bool IsMappedForReading() const;
        bool IsMappedForWriting() const;
        bool CheckGetMappedRangeOffsetSize(size_t offset, size_t size) const;

        // We want to defer all the validation to the server, which means we could have multiple
        // map request in flight at a single time and need to track them separately.
        // On well-behaved applications, only one request should exist at a single time.
        struct MapRequestData {
            // TODO(enga): Use a tagged pointer to save space.
            WGPUBufferMapReadCallback readCallback = nullptr;
            WGPUBufferMapWriteCallback writeCallback = nullptr;
            void* userdata = nullptr;
            // TODO(enga): Use a tagged pointer to save space.
            std::unique_ptr<MemoryTransferService::ReadHandle> readHandle = nullptr;
            std::unique_ptr<MemoryTransferService::WriteHandle> writeHandle = nullptr;
        };
        std::map<uint32_t, MapRequestData> mRequests;
        uint32_t mRequestSerial = 0;
        uint64_t mSize = 0;

        // Only one mapped pointer can be active at a time because Unmap clears all the in-flight
        // requests.
        // TODO(enga): Use a tagged pointer to save space.
        std::unique_ptr<MemoryTransferService::ReadHandle> mReadHandle = nullptr;
        std::unique_ptr<MemoryTransferService::WriteHandle> mWriteHandle = nullptr;
        void* mMappedData = nullptr;
        size_t mMapOffset = 0;
        size_t mMapSize = 0;
    };

}}  // namespace dawn_wire::client

#endif  // DAWNWIRE_CLIENT_BUFFER_H_
