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

#include "dawn_native/Queue.h"

#include "dawn_native/Buffer.h"
#include "dawn_native/CommandBuffer.h"
#include "dawn_native/CommandValidation.h"
#include "dawn_native/Device.h"
#include "dawn_native/DynamicUploader.h"
#include "dawn_native/ErrorScope.h"
#include "dawn_native/ErrorScopeTracker.h"
#include "dawn_native/Fence.h"
#include "dawn_native/FenceSignalTracker.h"
#include "dawn_native/QuerySet.h"
#include "dawn_native/Texture.h"
#include "dawn_platform/DawnPlatform.h"
#include "dawn_platform/tracing/TraceEvent.h"

#include <cstring>

namespace dawn_native {

    // QueueBase

    QueueBase::QueueBase(DeviceBase* device) : ObjectBase(device) {
    }

    QueueBase::QueueBase(DeviceBase* device, ObjectBase::ErrorTag tag) : ObjectBase(device, tag) {
    }

    // static
    QueueBase* QueueBase::MakeError(DeviceBase* device) {
        return new QueueBase(device, ObjectBase::kError);
    }

    MaybeError QueueBase::SubmitImpl(uint32_t commandCount, CommandBufferBase* const* commands) {
        UNREACHABLE();
        return {};
    }

    void QueueBase::Submit(uint32_t commandCount, CommandBufferBase* const* commands) {
        DeviceBase* device = GetDevice();
        if (device->ConsumedError(device->ValidateIsAlive())) {
            // If device is lost, don't let any commands be submitted
            return;
        }

        TRACE_EVENT0(device->GetPlatform(), General, "Queue::Submit");
        if (device->IsValidationEnabled() &&
            device->ConsumedError(ValidateSubmit(commandCount, commands))) {
            return;
        }
        ASSERT(!IsError());

        if (device->ConsumedError(SubmitImpl(commandCount, commands))) {
            return;
        }
        device->GetErrorScopeTracker()->TrackUntilLastSubmitComplete(
            device->GetCurrentErrorScope());
    }

    void QueueBase::Signal(Fence* fence, uint64_t signalValue) {
        DeviceBase* device = GetDevice();
        if (device->ConsumedError(ValidateSignal(fence, signalValue))) {
            return;
        }
        ASSERT(!IsError());

        fence->SetSignaledValue(signalValue);
        device->GetFenceSignalTracker()->UpdateFenceOnComplete(fence, signalValue);
        device->GetErrorScopeTracker()->TrackUntilLastSubmitComplete(
            device->GetCurrentErrorScope());
    }

    Fence* QueueBase::CreateFence(const FenceDescriptor* descriptor) {
        if (GetDevice()->ConsumedError(ValidateCreateFence(descriptor))) {
            return Fence::MakeError(GetDevice());
        }

        if (descriptor == nullptr) {
            FenceDescriptor defaultDescriptor = {};
            return new Fence(this, &defaultDescriptor);
        }
        return new Fence(this, descriptor);
    }

    void QueueBase::WriteBuffer(BufferBase* buffer,
                                uint64_t bufferOffset,
                                const void* data,
                                size_t size) {
        GetDevice()->ConsumedError(WriteBufferInternal(buffer, bufferOffset, data, size));
    }

    MaybeError QueueBase::WriteBufferInternal(BufferBase* buffer,
                                              uint64_t bufferOffset,
                                              const void* data,
                                              size_t size) {
        DAWN_TRY(ValidateWriteBuffer(buffer, bufferOffset, size));
        return WriteBufferImpl(buffer, bufferOffset, data, size);
    }

    MaybeError QueueBase::WriteBufferImpl(BufferBase* buffer,
                                          uint64_t bufferOffset,
                                          const void* data,
                                          size_t size) {
        if (size == 0) {
            return {};
        }

        DeviceBase* device = GetDevice();

        UploadHandle uploadHandle;
        DAWN_TRY_ASSIGN(uploadHandle, device->GetDynamicUploader()->Allocate(
                                          size, device->GetPendingCommandSerial()));
        ASSERT(uploadHandle.mappedBuffer != nullptr);

        memcpy(uploadHandle.mappedBuffer, data, size);

        return device->CopyFromStagingToBuffer(uploadHandle.stagingBuffer, uploadHandle.startOffset,
                                               buffer, bufferOffset, size);
    }

    void QueueBase::WriteTexture(const TextureCopyView* destination,
                                 const void* data,
                                 size_t dataSize,
                                 const TextureDataLayout* dataLayout,
                                 const Extent3D* writeSize) {
        GetDevice()->ConsumedError(
            WriteTextureInternal(destination, data, dataSize, dataLayout, writeSize));
    }

    MaybeError QueueBase::WriteTextureInternal(const TextureCopyView* destination,
                                               const void* data,
                                               size_t dataSize,
                                               const TextureDataLayout* dataLayout,
                                               const Extent3D* writeSize) {
        DAWN_TRY(ValidateWriteTexture(destination, dataSize, dataLayout, writeSize));

        if (writeSize->width == 0 || writeSize->height == 0 || writeSize->depth == 0) {
            return {};
        }

        return WriteTextureImpl(destination, data, dataSize, dataLayout, writeSize);
    }

    MaybeError QueueBase::WriteTextureImpl(const TextureCopyView* destination,
                                           const void* data,
                                           size_t dataSize,
                                           const TextureDataLayout* dataLayout,
                                           const Extent3D* writeSize) {
        // TODO(tommek@google.com): This should be implemented.
        return {};
    }

    MaybeError QueueBase::ValidateSubmit(uint32_t commandCount,
                                         CommandBufferBase* const* commands) const {
        TRACE_EVENT0(GetDevice()->GetPlatform(), Validation, "Queue::ValidateSubmit");
        DAWN_TRY(GetDevice()->ValidateObject(this));

        for (uint32_t i = 0; i < commandCount; ++i) {
            DAWN_TRY(GetDevice()->ValidateObject(commands[i]));

            const CommandBufferResourceUsage& usages = commands[i]->GetResourceUsages();

            for (const PassResourceUsage& passUsages : usages.perPass) {
                for (const BufferBase* buffer : passUsages.buffers) {
                    DAWN_TRY(buffer->ValidateCanUseOnQueueNow());
                }
                for (const TextureBase* texture : passUsages.textures) {
                    DAWN_TRY(texture->ValidateCanUseInSubmitNow());
                }
            }

            for (const BufferBase* buffer : usages.topLevelBuffers) {
                DAWN_TRY(buffer->ValidateCanUseOnQueueNow());
            }
            for (const TextureBase* texture : usages.topLevelTextures) {
                DAWN_TRY(texture->ValidateCanUseInSubmitNow());
            }
            for (const QuerySetBase* querySet : usages.usedQuerySets) {
                DAWN_TRY(querySet->ValidateCanUseInSubmitNow());
            }
        }

        return {};
    }

    MaybeError QueueBase::ValidateSignal(const Fence* fence, uint64_t signalValue) const {
        DAWN_TRY(GetDevice()->ValidateIsAlive());
        DAWN_TRY(GetDevice()->ValidateObject(this));
        DAWN_TRY(GetDevice()->ValidateObject(fence));

        if (fence->GetQueue() != this) {
            return DAWN_VALIDATION_ERROR(
                "Fence must be signaled on the queue on which it was created.");
        }
        if (signalValue <= fence->GetSignaledValue()) {
            return DAWN_VALIDATION_ERROR("Signal value less than or equal to fence signaled value");
        }
        return {};
    }

    MaybeError QueueBase::ValidateCreateFence(const FenceDescriptor* descriptor) const {
        DAWN_TRY(GetDevice()->ValidateIsAlive());
        DAWN_TRY(GetDevice()->ValidateObject(this));
        if (descriptor != nullptr) {
            DAWN_TRY(ValidateFenceDescriptor(descriptor));
        }

        return {};
    }

    MaybeError QueueBase::ValidateWriteBuffer(const BufferBase* buffer,
                                              uint64_t bufferOffset,
                                              size_t size) const {
        DAWN_TRY(GetDevice()->ValidateIsAlive());
        DAWN_TRY(GetDevice()->ValidateObject(this));
        DAWN_TRY(GetDevice()->ValidateObject(buffer));

        if (bufferOffset % 4 != 0) {
            return DAWN_VALIDATION_ERROR("Queue::WriteBuffer bufferOffset must be a multiple of 4");
        }
        if (size % 4 != 0) {
            return DAWN_VALIDATION_ERROR("Queue::WriteBuffer size must be a multiple of 4");
        }

        uint64_t bufferSize = buffer->GetSize();
        if (bufferOffset > bufferSize || size > (bufferSize - bufferOffset)) {
            return DAWN_VALIDATION_ERROR("Queue::WriteBuffer out of range");
        }

        if (!(buffer->GetUsage() & wgpu::BufferUsage::CopyDst)) {
            return DAWN_VALIDATION_ERROR("Buffer needs the CopyDst usage bit");
        }

        return buffer->ValidateCanUseOnQueueNow();
    }

    MaybeError QueueBase::ValidateWriteTexture(const TextureCopyView* destination,
                                               size_t dataSize,
                                               const TextureDataLayout* dataLayout,
                                               const Extent3D* writeSize) const {
        DAWN_TRY(GetDevice()->ValidateIsAlive());
        DAWN_TRY(GetDevice()->ValidateObject(this));
        DAWN_TRY(GetDevice()->ValidateObject(destination->texture));

        DAWN_TRY(ValidateTextureCopyView(GetDevice(), *destination));

        if (dataLayout->offset > dataSize) {
            return DAWN_VALIDATION_ERROR("Queue::WriteTexture out of range");
        }

        if (!(destination->texture->GetUsage() & wgpu::TextureUsage::CopyDst)) {
            return DAWN_VALIDATION_ERROR("Texture needs the CopyDst usage bit");
        }

        if (destination->texture->GetSampleCount() > 1) {
            return DAWN_VALIDATION_ERROR("The sample count of textures must be 1");
        }

        // We validate texture copy range before validating linear texture data,
        // because in the latter we divide copyExtent.width by blockWidth and
        // copyExtent.height by blockHeight while the divisibility conditions are
        // checked in validating texture copy range.
        DAWN_TRY(ValidateTextureCopyRange(*destination, *writeSize));
        DAWN_TRY(ValidateLinearTextureData(*dataLayout, dataSize, destination->texture->GetFormat(),
                                           *writeSize));

        return {};
    }

}  // namespace dawn_native
