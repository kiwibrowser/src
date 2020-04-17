// Copyright 2017 The Dawn Authors
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

#ifndef DAWNNATIVE_BUFFER_H_
#define DAWNNATIVE_BUFFER_H_

#include "dawn_native/Error.h"
#include "dawn_native/Forward.h"
#include "dawn_native/ObjectBase.h"

#include "dawn_native/dawn_platform.h"

namespace dawn_native {

    MaybeError ValidateBufferDescriptor(DeviceBase* device, const BufferDescriptor* descriptor);

    static constexpr dawn::BufferUsageBit kReadOnlyBufferUsages =
        dawn::BufferUsageBit::MapRead | dawn::BufferUsageBit::TransferSrc |
        dawn::BufferUsageBit::Index | dawn::BufferUsageBit::Vertex | dawn::BufferUsageBit::Uniform;

    static constexpr dawn::BufferUsageBit kWritableBufferUsages =
        dawn::BufferUsageBit::MapWrite | dawn::BufferUsageBit::TransferDst |
        dawn::BufferUsageBit::Storage;

    class BufferBase : public ObjectBase {
        enum class BufferState {
            Unmapped,
            Mapped,
            Destroyed,
        };

      public:
        BufferBase(DeviceBase* device, const BufferDescriptor* descriptor);
        ~BufferBase();

        static BufferBase* MakeError(DeviceBase* device);
        static BufferBase* MakeErrorMapped(DeviceBase* device,
                                           uint64_t size,
                                           uint8_t** mappedPointer);

        uint64_t GetSize() const;
        dawn::BufferUsageBit GetUsage() const;

        MaybeError MapAtCreation(uint8_t** mappedPointer);

        MaybeError ValidateCanUseInSubmitNow() const;

        // Dawn API
        void SetSubData(uint32_t start, uint32_t count, const void* data);
        void MapReadAsync(DawnBufferMapReadCallback callback, void* userdata);
        void MapWriteAsync(DawnBufferMapWriteCallback callback, void* userdata);
        void Unmap();
        void Destroy();

      protected:
        BufferBase(DeviceBase* device, ObjectBase::ErrorTag tag);

        void CallMapReadCallback(uint32_t serial,
                                 DawnBufferMapAsyncStatus status,
                                 const void* pointer,
                                 uint32_t dataLength);
        void CallMapWriteCallback(uint32_t serial,
                                  DawnBufferMapAsyncStatus status,
                                  void* pointer,
                                  uint32_t dataLength);

        void DestroyInternal();

      private:
        virtual MaybeError MapAtCreationImpl(uint8_t** mappedPointer) = 0;
        virtual MaybeError SetSubDataImpl(uint32_t start, uint32_t count, const void* data);
        virtual void MapReadAsyncImpl(uint32_t serial) = 0;
        virtual void MapWriteAsyncImpl(uint32_t serial) = 0;
        virtual void UnmapImpl() = 0;
        virtual void DestroyImpl() = 0;

        virtual bool IsMapWritable() const = 0;
        MaybeError CopyFromStagingBuffer();

        MaybeError ValidateSetSubData(uint32_t start, uint32_t count) const;
        MaybeError ValidateMap(dawn::BufferUsageBit requiredUsage) const;
        MaybeError ValidateUnmap() const;
        MaybeError ValidateDestroy() const;

        uint64_t mSize = 0;
        dawn::BufferUsageBit mUsage = dawn::BufferUsageBit::None;

        DawnBufferMapReadCallback mMapReadCallback = nullptr;
        DawnBufferMapWriteCallback mMapWriteCallback = nullptr;
        void* mMapUserdata = 0;
        uint32_t mMapSerial = 0;

        std::unique_ptr<StagingBufferBase> mStagingBuffer;

        BufferState mState;
    };

}  // namespace dawn_native

#endif  // DAWNNATIVE_BUFFER_H_
