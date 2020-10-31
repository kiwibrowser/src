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

#include "dawn_wire/client/Buffer.h"

#include "dawn_wire/client/Client.h"
#include "dawn_wire/client/Device.h"

namespace dawn_wire { namespace client {

    namespace {
        template <typename Handle>
        void SerializeBufferMapAsync(const Buffer* buffer,
                                     uint32_t serial,
                                     Handle* handle,
                                     size_t size) {
            // TODO(enga): Remove the template when Read/Write handles are combined in a tagged
            // pointer.
            constexpr bool isWrite =
                std::is_same<Handle, MemoryTransferService::WriteHandle>::value;

            // Get the serialization size of the handle.
            size_t handleCreateInfoLength = handle->SerializeCreateSize();

            BufferMapAsyncCmd cmd;
            cmd.bufferId = buffer->id;
            cmd.requestSerial = serial;
            cmd.mode = isWrite ? WGPUMapMode_Write : WGPUMapMode_Read;
            cmd.handleCreateInfoLength = handleCreateInfoLength;
            cmd.handleCreateInfo = nullptr;
            cmd.offset = 0;
            cmd.size = size;

            char* writeHandleSpace =
                buffer->device->GetClient()->SerializeCommand(cmd, handleCreateInfoLength);

            // Serialize the handle into the space after the command.
            handle->SerializeCreate(writeHandleSpace);
        }
    }  // namespace

    // static
    WGPUBuffer Buffer::Create(Device* device_, const WGPUBufferDescriptor* descriptor) {
        Client* wireClient = device_->GetClient();

        if ((descriptor->usage & (WGPUBufferUsage_MapRead | WGPUBufferUsage_MapWrite)) != 0 &&
            descriptor->size > std::numeric_limits<size_t>::max()) {
            device_->InjectError(WGPUErrorType_OutOfMemory, "Buffer is too large for map usage");
            return device_->CreateErrorBuffer();
        }

        auto* bufferObjectAndSerial = wireClient->BufferAllocator().New(device_);
        Buffer* buffer = bufferObjectAndSerial->object.get();
        // Store the size of the buffer so that mapping operations can allocate a
        // MemoryTransfer handle of the proper size.
        buffer->mSize = descriptor->size;

        DeviceCreateBufferCmd cmd;
        cmd.self = ToAPI(device_);
        cmd.descriptor = descriptor;
        cmd.result = ObjectHandle{buffer->id, bufferObjectAndSerial->generation};

        wireClient->SerializeCommand(cmd);

        return ToAPI(buffer);
    }

    // static
    WGPUCreateBufferMappedResult Buffer::CreateMapped(Device* device_,
                                                      const WGPUBufferDescriptor* descriptor) {
        Client* wireClient = device_->GetClient();

        WGPUCreateBufferMappedResult result;
        result.data = nullptr;
        result.dataLength = 0;

        // This buffer is too large to be mapped and to make a WriteHandle for.
        if (descriptor->size > std::numeric_limits<size_t>::max()) {
            device_->InjectError(WGPUErrorType_OutOfMemory, "Buffer is too large for mapping");
            result.buffer = device_->CreateErrorBuffer();
            return result;
        }

        // Create a WriteHandle for the map request. This is the client's intent to write GPU
        // memory.
        std::unique_ptr<MemoryTransferService::WriteHandle> writeHandle =
            std::unique_ptr<MemoryTransferService::WriteHandle>(
                wireClient->GetMemoryTransferService()->CreateWriteHandle(descriptor->size));

        if (writeHandle == nullptr) {
            device_->InjectError(WGPUErrorType_OutOfMemory, "Buffer mapping allocation failed");
            result.buffer = device_->CreateErrorBuffer();
            return result;
        }

        // CreateBufferMapped is synchronous and the staging buffer for upload should be immediately
        // available.
        // Open the WriteHandle. This returns a pointer and size of mapped memory.
        // |result.data| may be null on error.
        std::tie(result.data, result.dataLength) = writeHandle->Open();
        if (result.data == nullptr) {
            device_->InjectError(WGPUErrorType_OutOfMemory, "Buffer mapping allocation failed");
            result.buffer = device_->CreateErrorBuffer();
            return result;
        }

        auto* bufferObjectAndSerial = wireClient->BufferAllocator().New(device_);
        Buffer* buffer = bufferObjectAndSerial->object.get();
        buffer->mSize = descriptor->size;
        // Successfully created staging memory. The buffer now owns the WriteHandle.
        buffer->mWriteHandle = std::move(writeHandle);
        buffer->mMappedData = result.data;
        buffer->mMapOffset = 0;
        buffer->mMapSize = descriptor->size;

        result.buffer = ToAPI(buffer);

        // Get the serialization size of the WriteHandle.
        size_t handleCreateInfoLength = buffer->mWriteHandle->SerializeCreateSize();

        DeviceCreateBufferMappedCmd cmd;
        cmd.device = ToAPI(device_);
        cmd.descriptor = descriptor;
        cmd.result = ObjectHandle{buffer->id, bufferObjectAndSerial->generation};
        cmd.handleCreateInfoLength = handleCreateInfoLength;
        cmd.handleCreateInfo = nullptr;

        char* writeHandleSpace =
            buffer->device->GetClient()->SerializeCommand(cmd, handleCreateInfoLength);

        // Serialize the WriteHandle into the space after the command.
        buffer->mWriteHandle->SerializeCreate(writeHandleSpace);

        return result;
    }

    // static
    WGPUBuffer Buffer::CreateError(Device* device_) {
        auto* allocation = device_->GetClient()->BufferAllocator().New(device_);

        DeviceCreateErrorBufferCmd cmd;
        cmd.self = ToAPI(device_);
        cmd.result = ObjectHandle{allocation->object->id, allocation->generation};
        device_->GetClient()->SerializeCommand(cmd);

        return ToAPI(allocation->object.get());
    }

    Buffer::~Buffer() {
        // Callbacks need to be fired in all cases, as they can handle freeing resources
        // so we call them with "Unknown" status.
        ClearMapRequests(WGPUBufferMapAsyncStatus_Unknown);
    }

    void Buffer::ClearMapRequests(WGPUBufferMapAsyncStatus status) {
        for (auto& it : mRequests) {
            if (it.second.writeHandle) {
                it.second.writeCallback(status, nullptr, 0, it.second.userdata);
            } else {
                it.second.readCallback(status, nullptr, 0, it.second.userdata);
            }
        }
        mRequests.clear();
    }

    void Buffer::MapReadAsync(WGPUBufferMapReadCallback callback, void* userdata) {
        uint32_t serial = mRequestSerial++;
        ASSERT(mRequests.find(serial) == mRequests.end());

        if (mSize > std::numeric_limits<size_t>::max()) {
            // On buffer creation, we check that mappable buffers do not exceed this size.
            // So this buffer must not have mappable usage. Inject a validation error.
            device->InjectError(WGPUErrorType_Validation, "Buffer needs the correct map usage bit");
            callback(WGPUBufferMapAsyncStatus_Error, nullptr, 0, userdata);
            return;
        }

        // Create a ReadHandle for the map request. This is the client's intent to read GPU
        // memory.
        MemoryTransferService::ReadHandle* readHandle =
            device->GetClient()->GetMemoryTransferService()->CreateReadHandle(
                static_cast<size_t>(mSize));
        if (readHandle == nullptr) {
            device->InjectError(WGPUErrorType_OutOfMemory, "Failed to create buffer mapping");
            callback(WGPUBufferMapAsyncStatus_Error, nullptr, 0, userdata);
            return;
        }

        Buffer::MapRequestData request = {};
        request.readCallback = callback;
        request.userdata = userdata;
        // The handle is owned by the MapRequest until the callback returns.
        request.readHandle = std::unique_ptr<MemoryTransferService::ReadHandle>(readHandle);

        // Store a mapping from serial -> MapRequest. The client can map/unmap before the map
        // operations are returned by the server so multiple requests may be in flight.
        mRequests[serial] = std::move(request);

        SerializeBufferMapAsync(this, serial, readHandle, mSize);
    }

    void Buffer::MapWriteAsync(WGPUBufferMapWriteCallback callback, void* userdata) {
        uint32_t serial = mRequestSerial++;
        ASSERT(mRequests.find(serial) == mRequests.end());

        if (mSize > std::numeric_limits<size_t>::max()) {
            // On buffer creation, we check that mappable buffers do not exceed this size.
            // So this buffer must not have mappable usage. Inject a validation error.
            device->InjectError(WGPUErrorType_Validation, "Buffer needs the correct map usage bit");
            callback(WGPUBufferMapAsyncStatus_Error, nullptr, 0, userdata);
            return;
        }

        // Create a WriteHandle for the map request. This is the client's intent to write GPU
        // memory.
        MemoryTransferService::WriteHandle* writeHandle =
            device->GetClient()->GetMemoryTransferService()->CreateWriteHandle(
                static_cast<size_t>(mSize));
        if (writeHandle == nullptr) {
            device->InjectError(WGPUErrorType_OutOfMemory, "Failed to create buffer mapping");
            callback(WGPUBufferMapAsyncStatus_Error, nullptr, 0, userdata);
            return;
        }

        Buffer::MapRequestData request = {};
        request.writeCallback = callback;
        request.userdata = userdata;
        // The handle is owned by the MapRequest until the callback returns.
        request.writeHandle = std::unique_ptr<MemoryTransferService::WriteHandle>(writeHandle);

        // Store a mapping from serial -> MapRequest. The client can map/unmap before the map
        // operations are returned by the server so multiple requests may be in flight.
        mRequests[serial] = std::move(request);

        SerializeBufferMapAsync(this, serial, writeHandle, mSize);
    }

    void Buffer::MapAsync(WGPUMapModeFlags mode,
                          size_t offset,
                          size_t size,
                          WGPUBufferMapCallback callback,
                          void* userdata) {
        // Do early validation for mode because it needs to be correct for the proxying to
        // MapReadAsync or MapWriteAsync to work.
        bool isReadMode = mode & WGPUMapMode_Read;
        bool isWriteMode = mode & WGPUMapMode_Write;
        bool modeOk = isReadMode ^ isWriteMode;
        // Do early validation of offset and size because it isn't checked by MapReadAsync /
        // MapWriteAsync.
        bool offsetOk = (uint64_t(offset) <= mSize) && offset % 4 == 0;
        bool sizeOk = (uint64_t(size) <= mSize - uint64_t(offset)) && size % 4 == 0;

        if (!(modeOk && offsetOk && sizeOk)) {
            device->InjectError(WGPUErrorType_Validation, "MapAsync error (you figure out :P)");
            if (callback != nullptr) {
                callback(WGPUBufferMapAsyncStatus_Error, userdata);
            }
            return;
        }

        // The structure to keep arguments so we can forward the MapReadAsync and MapWriteAsync to
        // `callback`
        struct ProxyData {
            WGPUBufferMapCallback callback;
            void* userdata;
            size_t mapOffset;
            size_t mapSize;
            Buffer* self;
        };
        ProxyData* proxy = new ProxyData;
        proxy->callback = callback;
        proxy->userdata = userdata;
        proxy->mapOffset = offset;
        proxy->mapSize = size;
        proxy->self = this;
        // Note technically we should keep the buffer alive until the callback is fired but the
        // client doesn't have good facilities to do that yet.

        // Call MapReadAsync or MapWriteAsync and forward the callback.
        if (isReadMode) {
            MapReadAsync(
                [](WGPUBufferMapAsyncStatus status, const void*, uint64_t, void* userdata) {
                    ProxyData* proxy = static_cast<ProxyData*>(userdata);
                    proxy->self->mMapOffset = proxy->mapOffset;
                    proxy->self->mMapSize = proxy->mapSize;
                    if (proxy->callback) {
                        proxy->callback(status, proxy->userdata);
                    }
                    delete proxy;
                },
                proxy);
        } else {
            ASSERT(isWriteMode);
            MapWriteAsync(
                [](WGPUBufferMapAsyncStatus status, void*, uint64_t, void* userdata) {
                    ProxyData* proxy = static_cast<ProxyData*>(userdata);
                    proxy->self->mMapOffset = proxy->mapOffset;
                    proxy->self->mMapSize = proxy->mapSize;
                    if (proxy->callback) {
                        proxy->callback(status, proxy->userdata);
                    }
                    delete proxy;
                },
                proxy);
        }
    }

    bool Buffer::OnMapAsyncCallback(uint32_t requestSerial,
                                    uint32_t status,
                                    uint64_t readInitialDataInfoLength,
                                    const uint8_t* readInitialDataInfo) {
        // The requests can have been deleted via an Unmap so this isn't an error.
        auto requestIt = mRequests.find(requestSerial);
        if (requestIt == mRequests.end()) {
            return true;
        }

        auto request = std::move(requestIt->second);
        // Delete the request before calling the callback otherwise the callback could be fired a
        // second time. If, for example, buffer.Unmap() is called inside the callback.
        mRequests.erase(requestIt);

        auto FailRequest = [&request]() -> bool {
            if (request.readCallback != nullptr) {
                request.readCallback(WGPUBufferMapAsyncStatus_DeviceLost, nullptr, 0,
                                     request.userdata);
            }
            if (request.writeCallback != nullptr) {
                request.writeCallback(WGPUBufferMapAsyncStatus_DeviceLost, nullptr, 0,
                                      request.userdata);
            }
            return false;
        };

        bool isRead = request.readHandle != nullptr;
        bool isWrite = request.writeHandle != nullptr;
        ASSERT(isRead != isWrite);

        size_t mappedDataLength = 0;
        const void* mappedData = nullptr;
        if (status == WGPUBufferMapAsyncStatus_Success) {
            if (mReadHandle || mWriteHandle) {
                // Buffer is already mapped.
                return FailRequest();
            }

            if (isRead) {
                if (readInitialDataInfoLength > std::numeric_limits<size_t>::max()) {
                    // This is the size of data deserialized from the command stream, which must be
                    // CPU-addressable.
                    return FailRequest();
                }

                // The server serializes metadata to initialize the contents of the ReadHandle.
                // Deserialize the message and return a pointer and size of the mapped data for
                // reading.
                if (!request.readHandle->DeserializeInitialData(
                        readInitialDataInfo, static_cast<size_t>(readInitialDataInfoLength),
                        &mappedData, &mappedDataLength)) {
                    // Deserialization shouldn't fail. This is a fatal error.
                    return FailRequest();
                }
                ASSERT(mappedData != nullptr);

            } else {
                // Open the WriteHandle. This returns a pointer and size of mapped memory.
                // On failure, |mappedData| may be null.
                std::tie(mappedData, mappedDataLength) = request.writeHandle->Open();

                if (mappedData == nullptr) {
                    return FailRequest();
                }
            }

            // The MapAsync request was successful. The buffer now owns the Read/Write handles
            // until Unmap().
            mReadHandle = std::move(request.readHandle);
            mWriteHandle = std::move(request.writeHandle);
        }

        mMappedData = const_cast<void*>(mappedData);

        if (isRead) {
            request.readCallback(static_cast<WGPUBufferMapAsyncStatus>(status), mMappedData,
                                 static_cast<uint64_t>(mappedDataLength), request.userdata);
        } else {
            request.writeCallback(static_cast<WGPUBufferMapAsyncStatus>(status), mMappedData,
                                  static_cast<uint64_t>(mappedDataLength), request.userdata);
        }

        return true;
    }

    void* Buffer::GetMappedRange(size_t offset, size_t size) {
        if (!IsMappedForWriting() || !CheckGetMappedRangeOffsetSize(offset, size)) {
            return nullptr;
        }
        return static_cast<uint8_t*>(mMappedData) + offset;
    }

    const void* Buffer::GetConstMappedRange(size_t offset, size_t size) {
        if (!(IsMappedForWriting() || IsMappedForReading()) ||
            !CheckGetMappedRangeOffsetSize(offset, size)) {
            return nullptr;
        }
        return static_cast<uint8_t*>(mMappedData) + offset;
    }

    void Buffer::Unmap() {
        // Invalidate the local pointer, and cancel all other in-flight requests that would
        // turn into errors anyway (you can't double map). This prevents race when the following
        // happens, where the application code would have unmapped a buffer but still receive a
        // callback:
        //   - Client -> Server: MapRequest1, Unmap, MapRequest2
        //   - Server -> Client: Result of MapRequest1
        //   - Unmap locally on the client
        //   - Server -> Client: Result of MapRequest2
        if (mWriteHandle) {
            // Writes need to be flushed before Unmap is sent. Unmap calls all associated
            // in-flight callbacks which may read the updated data.
            ASSERT(mReadHandle == nullptr);

            // Get the serialization size of metadata to flush writes.
            size_t writeFlushInfoLength = mWriteHandle->SerializeFlushSize();

            BufferUpdateMappedDataCmd cmd;
            cmd.bufferId = id;
            cmd.writeFlushInfoLength = writeFlushInfoLength;
            cmd.writeFlushInfo = nullptr;

            char* writeHandleSpace =
                device->GetClient()->SerializeCommand(cmd, writeFlushInfoLength);

            // Serialize flush metadata into the space after the command.
            // This closes the handle for writing.
            mWriteHandle->SerializeFlush(writeHandleSpace);
            mWriteHandle = nullptr;

        } else if (mReadHandle) {
            mReadHandle = nullptr;
        }

        mMappedData = nullptr;
        mMapOffset = 0;
        mMapSize = 0;
        ClearMapRequests(WGPUBufferMapAsyncStatus_Unknown);

        BufferUnmapCmd cmd;
        cmd.self = ToAPI(this);
        device->GetClient()->SerializeCommand(cmd);
    }

    void Buffer::Destroy() {
        // Cancel or remove all mappings
        mWriteHandle = nullptr;
        mReadHandle = nullptr;
        mMappedData = nullptr;
        ClearMapRequests(WGPUBufferMapAsyncStatus_Unknown);

        BufferDestroyCmd cmd;
        cmd.self = ToAPI(this);
        device->GetClient()->SerializeCommand(cmd);
    }

    void Buffer::SetSubData(uint64_t start, uint64_t count, const void* data) {
        BufferSetSubDataInternalCmd cmd;
        cmd.bufferId = id;
        cmd.start = start;
        cmd.count = count;
        cmd.data = static_cast<const uint8_t*>(data);

        device->GetClient()->SerializeCommand(cmd);
    }

    bool Buffer::IsMappedForReading() const {
        return mReadHandle != nullptr;
    }

    bool Buffer::IsMappedForWriting() const {
        return mWriteHandle != nullptr;
    }

    bool Buffer::CheckGetMappedRangeOffsetSize(size_t offset, size_t size) const {
        if (size > mMapSize || offset < mMapOffset) {
            return false;
        }

        size_t offsetInMappedRange = offset - mMapOffset;
        return offsetInMappedRange <= mMapSize - size;
    }
}}  // namespace dawn_wire::client
