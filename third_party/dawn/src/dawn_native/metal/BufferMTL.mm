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

#include "dawn_native/metal/BufferMTL.h"

#include "common/Math.h"
#include "dawn_native/CommandBuffer.h"
#include "dawn_native/metal/CommandRecordingContext.h"
#include "dawn_native/metal/DeviceMTL.h"

#include <limits>

namespace dawn_native { namespace metal {
    // The size of uniform buffer and storage buffer need to be aligned to 16 bytes which is the
    // largest alignment of supported data types
    static constexpr uint32_t kMinUniformOrStorageBufferAlignment = 16u;

    // The maximum buffer size if querying the maximum buffer size or recommended working set size
    // is not available. This is a somewhat arbitrary limit of 1 GiB.
    static constexpr uint32_t kMaxBufferSizeFallback = 1024u * 1024u * 1024u;

    // static
    ResultOrError<Ref<Buffer>> Buffer::Create(Device* device, const BufferDescriptor* descriptor) {
        Ref<Buffer> buffer = AcquireRef(new Buffer(device, descriptor));
        DAWN_TRY(buffer->Initialize());
        return std::move(buffer);
    }

    MaybeError Buffer::Initialize() {
        MTLResourceOptions storageMode;
        if (GetUsage() & (wgpu::BufferUsage::MapRead | wgpu::BufferUsage::MapWrite)) {
            storageMode = MTLResourceStorageModeShared;
        } else {
            storageMode = MTLResourceStorageModePrivate;
        }

        // TODO(cwallez@chromium.org): Have a global "zero" buffer that can do everything instead
        // of creating a new 4-byte buffer?
        if (GetSize() > std::numeric_limits<NSUInteger>::max()) {
            return DAWN_OUT_OF_MEMORY_ERROR("Buffer allocation is too large");
        }
        NSUInteger currentSize = static_cast<NSUInteger>(std::max(GetSize(), uint64_t(4u)));

        // Metal validation layer requires the size of uniform buffer and storage buffer to be no
        // less than the size of the buffer block defined in shader, and the overall size of the
        // buffer must be aligned to the largest alignment of its members.
        if (GetUsage() & (wgpu::BufferUsage::Uniform | wgpu::BufferUsage::Storage)) {
            if (currentSize >
                std::numeric_limits<NSUInteger>::max() - kMinUniformOrStorageBufferAlignment) {
                // Alignment would overlow.
                return DAWN_OUT_OF_MEMORY_ERROR("Buffer allocation is too large");
            }
            currentSize = Align(currentSize, kMinUniformOrStorageBufferAlignment);
        }

        if (@available(iOS 12, macOS 10.14, *)) {
            NSUInteger maxBufferSize = [ToBackend(GetDevice())->GetMTLDevice() maxBufferLength];
            if (currentSize > maxBufferSize) {
                return DAWN_OUT_OF_MEMORY_ERROR("Buffer allocation is too large");
            }
#if defined(DAWN_PLATFORM_MACOS)
        } else if (@available(macOS 10.12, *)) {
            // |maxBufferLength| isn't always available on older systems. If available, use
            // |recommendedMaxWorkingSetSize| instead. We can probably allocate more than this,
            // but don't have a way to discover a better limit. MoltenVK also uses this heuristic.
            uint64_t maxWorkingSetSize =
                [ToBackend(GetDevice())->GetMTLDevice() recommendedMaxWorkingSetSize];
            if (currentSize > maxWorkingSetSize) {
                return DAWN_OUT_OF_MEMORY_ERROR("Buffer allocation is too large");
            }
#endif
        } else if (currentSize > kMaxBufferSizeFallback) {
            return DAWN_OUT_OF_MEMORY_ERROR("Buffer allocation is too large");
        }

        mMtlBuffer = [ToBackend(GetDevice())->GetMTLDevice() newBufferWithLength:currentSize
                                                                         options:storageMode];
        if (mMtlBuffer == nil) {
            return DAWN_OUT_OF_MEMORY_ERROR("Buffer allocation failed");
        }

        if (GetDevice()->IsToggleEnabled(Toggle::NonzeroClearResourcesOnCreationForTesting)) {
            CommandRecordingContext* commandContext =
                ToBackend(GetDevice())->GetPendingCommandContext();
            ClearBuffer(commandContext, uint8_t(1u));
        }

        return {};
    }

    Buffer::~Buffer() {
        DestroyInternal();
    }

    id<MTLBuffer> Buffer::GetMTLBuffer() const {
        return mMtlBuffer;
    }

    bool Buffer::IsMappableAtCreation() const {
        // TODO(enga): Handle CPU-visible memory on UMA
        return (GetUsage() & (wgpu::BufferUsage::MapRead | wgpu::BufferUsage::MapWrite)) != 0;
    }

    MaybeError Buffer::MapAtCreationImpl() {
        CommandRecordingContext* commandContext =
            ToBackend(GetDevice())->GetPendingCommandContext();
        EnsureDataInitialized(commandContext);

        return {};
    }

    MaybeError Buffer::MapReadAsyncImpl() {
        return {};
    }

    MaybeError Buffer::MapWriteAsyncImpl() {
        return {};
    }

    MaybeError Buffer::MapAsyncImpl(wgpu::MapMode mode, size_t offset, size_t size) {
        CommandRecordingContext* commandContext =
            ToBackend(GetDevice())->GetPendingCommandContext();
        EnsureDataInitialized(commandContext);

        return {};
    }

    void* Buffer::GetMappedPointerImpl() {
        return [mMtlBuffer contents];
    }

    void Buffer::UnmapImpl() {
        // Nothing to do, Metal StorageModeShared buffers are always mapped.
    }

    void Buffer::DestroyImpl() {
        [mMtlBuffer release];
        mMtlBuffer = nil;
    }

    void Buffer::EnsureDataInitialized(CommandRecordingContext* commandContext) {
        // TODO(jiawei.shao@intel.com): check Toggle::LazyClearResourceOnFirstUse
        // instead when buffer lazy initialization is completely supported.
        if (IsDataInitialized() ||
            !GetDevice()->IsToggleEnabled(Toggle::LazyClearBufferOnFirstUse)) {
            return;
        }

        InitializeToZero(commandContext);
    }

    void Buffer::EnsureDataInitializedAsDestination(CommandRecordingContext* commandContext,
                                                    uint64_t offset,
                                                    uint64_t size) {
        // TODO(jiawei.shao@intel.com): check Toggle::LazyClearResourceOnFirstUse
        // instead when buffer lazy initialization is completely supported.
        if (IsDataInitialized() ||
            !GetDevice()->IsToggleEnabled(Toggle::LazyClearBufferOnFirstUse)) {
            return;
        }

        if (IsFullBufferRange(offset, size)) {
            SetIsDataInitialized();
        } else {
            InitializeToZero(commandContext);
        }
    }

    void Buffer::EnsureDataInitializedAsDestination(CommandRecordingContext* commandContext,
                                                    const CopyTextureToBufferCmd* copy) {
        // TODO(jiawei.shao@intel.com): check Toggle::LazyClearResourceOnFirstUse
        // instead when buffer lazy initialization is completely supported.
        if (IsDataInitialized() ||
            !GetDevice()->IsToggleEnabled(Toggle::LazyClearBufferOnFirstUse)) {
            return;
        }

        if (IsFullBufferOverwrittenInTextureToBufferCopy(copy)) {
            SetIsDataInitialized();
        } else {
            InitializeToZero(commandContext);
        }
    }

    void Buffer::InitializeToZero(CommandRecordingContext* commandContext) {
        ASSERT(GetDevice()->IsToggleEnabled(Toggle::LazyClearBufferOnFirstUse));
        ASSERT(!IsDataInitialized());

        ClearBuffer(commandContext, uint8_t(0u));

        SetIsDataInitialized();
        GetDevice()->IncrementLazyClearCountForTesting();
    }

    void Buffer::ClearBuffer(CommandRecordingContext* commandContext, uint8_t clearValue) {
        ASSERT(commandContext != nullptr);
        [commandContext->EnsureBlit() fillBuffer:mMtlBuffer
                                           range:NSMakeRange(0, GetSize())
                                           value:clearValue];
    }

}}  // namespace dawn_native::metal
