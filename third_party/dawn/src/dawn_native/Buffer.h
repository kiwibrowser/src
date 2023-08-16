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

#include <memory>

namespace dawn_native {

    struct CopyTextureToBufferCmd;

    enum class MapType : uint32_t;

    MaybeError ValidateBufferDescriptor(DeviceBase* device, const BufferDescriptor* descriptor);

    static constexpr wgpu::BufferUsage kReadOnlyBufferUsages =
        wgpu::BufferUsage::MapRead | wgpu::BufferUsage::CopySrc | wgpu::BufferUsage::Index |
        wgpu::BufferUsage::Vertex | wgpu::BufferUsage::Uniform | kReadOnlyStorageBuffer;

    class BufferBase : public ObjectBase {
        enum class BufferState {
            Unmapped,
            Mapped,
            MappedAtCreation,
            Destroyed,
        };

      public:
        BufferBase(DeviceBase* device, const BufferDescriptor* descriptor);

        static BufferBase* MakeError(DeviceBase* device, const BufferDescriptor* descriptor);

        uint64_t GetSize() const;
        wgpu::BufferUsage GetUsage() const;

        MaybeError MapAtCreation();
        void OnMapCommandSerialFinished(uint32_t mapSerial, MapType type);

        MaybeError ValidateCanUseOnQueueNow() const;

        bool IsFullBufferRange(uint64_t offset, uint64_t size) const;
        bool IsDataInitialized() const;
        void SetIsDataInitialized();

        // Dawn API
        void SetSubData(uint64_t start, uint64_t count, const void* data);
        void MapReadAsync(WGPUBufferMapReadCallback callback, void* userdata);
        void MapWriteAsync(WGPUBufferMapWriteCallback callback, void* userdata);
        void MapAsync(wgpu::MapMode mode,
                      size_t offset,
                      size_t size,
                      WGPUBufferMapCallback callback,
                      void* userdata);
        void* GetMappedRange(size_t offset, size_t size);
        const void* GetConstMappedRange(size_t offset, size_t size);
        void Unmap();
        void Destroy();

      protected:
        BufferBase(DeviceBase* device,
                   const BufferDescriptor* descriptor,
                   ObjectBase::ErrorTag tag);
        ~BufferBase() override;

        void DestroyInternal();

        bool IsMapped() const;

      private:
        virtual MaybeError MapAtCreationImpl() = 0;
        virtual MaybeError MapReadAsyncImpl() = 0;
        virtual MaybeError MapWriteAsyncImpl() = 0;
        virtual MaybeError MapAsyncImpl(wgpu::MapMode mode, size_t offset, size_t size) = 0;
        virtual void UnmapImpl() = 0;
        virtual void DestroyImpl() = 0;
        virtual void* GetMappedPointerImpl() = 0;

        virtual bool IsMappableAtCreation() const = 0;
        MaybeError CopyFromStagingBuffer();
        void* GetMappedRangeInternal(bool writable, size_t offset, size_t size);
        void CallMapReadCallback(uint32_t serial,
                                 WGPUBufferMapAsyncStatus status,
                                 const void* pointer,
                                 uint64_t dataLength);
        void CallMapWriteCallback(uint32_t serial,
                                  WGPUBufferMapAsyncStatus status,
                                  void* pointer,
                                  uint64_t dataLength);
        void CallMapCallback(uint32_t serial, WGPUBufferMapAsyncStatus status);

        MaybeError ValidateMap(wgpu::BufferUsage requiredUsage,
                               WGPUBufferMapAsyncStatus* status) const;
        MaybeError ValidateMapAsync(wgpu::MapMode mode,
                                    size_t offset,
                                    size_t size,
                                    WGPUBufferMapAsyncStatus* status) const;
        MaybeError ValidateUnmap() const;
        MaybeError ValidateDestroy() const;
        bool CanGetMappedRange(bool writable, size_t offset, size_t size) const;

        uint64_t mSize = 0;
        wgpu::BufferUsage mUsage = wgpu::BufferUsage::None;
        BufferState mState;
        bool mIsDataInitialized = false;

        std::unique_ptr<StagingBufferBase> mStagingBuffer;

        WGPUBufferMapReadCallback mMapReadCallback = nullptr;
        WGPUBufferMapWriteCallback mMapWriteCallback = nullptr;
        WGPUBufferMapCallback mMapCallback = nullptr;
        void* mMapUserdata = 0;
        uint32_t mMapSerial = 0;
        wgpu::MapMode mMapMode = wgpu::MapMode::None;
        size_t mMapOffset = 0;
        size_t mMapSize = 0;
    };

}  // namespace dawn_native

#endif  // DAWNNATIVE_BUFFER_H_
