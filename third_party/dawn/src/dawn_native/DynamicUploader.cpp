// Copyright 2018 The Dawn Authors
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

#include "dawn_native/DynamicUploader.h"
#include "common/Math.h"
#include "dawn_native/Device.h"

namespace dawn_native {

    DynamicUploader::DynamicUploader(DeviceBase* device) : mDevice(device) {
        mRingBuffers.emplace_back(std::unique_ptr<RingBuffer>(
            new RingBuffer{nullptr, RingBufferAllocator(kRingBufferSize)}));
    }

    void DynamicUploader::ReleaseStagingBuffer(std::unique_ptr<StagingBufferBase> stagingBuffer) {
        mReleasedStagingBuffers.Enqueue(std::move(stagingBuffer),
                                        mDevice->GetPendingCommandSerial());
    }

    ResultOrError<UploadHandle> DynamicUploader::Allocate(uint64_t allocationSize, Serial serial) {
        // Disable further sub-allocation should the request be too large.
        if (allocationSize > kRingBufferSize) {
            std::unique_ptr<StagingBufferBase> stagingBuffer;
            DAWN_TRY_ASSIGN(stagingBuffer, mDevice->CreateStagingBuffer(allocationSize));

            UploadHandle uploadHandle;
            uploadHandle.mappedBuffer = static_cast<uint8_t*>(stagingBuffer->GetMappedPointer());
            uploadHandle.stagingBuffer = stagingBuffer.get();

            ReleaseStagingBuffer(std::move(stagingBuffer));
            return uploadHandle;
        }

        // Note: Validation ensures size is already aligned.
        // First-fit: find next smallest buffer large enough to satisfy the allocation request.
        RingBuffer* targetRingBuffer = mRingBuffers.back().get();
        for (auto& ringBuffer : mRingBuffers) {
            const RingBufferAllocator& ringBufferAllocator = ringBuffer->mAllocator;
            // Prevent overflow.
            ASSERT(ringBufferAllocator.GetSize() >= ringBufferAllocator.GetUsedSize());
            const uint64_t remainingSize =
                ringBufferAllocator.GetSize() - ringBufferAllocator.GetUsedSize();
            if (allocationSize <= remainingSize) {
                targetRingBuffer = ringBuffer.get();
                break;
            }
        }

        uint64_t startOffset = RingBufferAllocator::kInvalidOffset;
        if (targetRingBuffer != nullptr) {
            startOffset = targetRingBuffer->mAllocator.Allocate(allocationSize, serial);
        }

        // Upon failure, append a newly created ring buffer to fulfill the
        // request.
        if (startOffset == RingBufferAllocator::kInvalidOffset) {
            mRingBuffers.emplace_back(std::unique_ptr<RingBuffer>(
                new RingBuffer{nullptr, RingBufferAllocator(kRingBufferSize)}));

            targetRingBuffer = mRingBuffers.back().get();
            startOffset = targetRingBuffer->mAllocator.Allocate(allocationSize, serial);
        }

        ASSERT(startOffset != RingBufferAllocator::kInvalidOffset);

        // Allocate the staging buffer backing the ringbuffer.
        // Note: the first ringbuffer will be lazily created.
        if (targetRingBuffer->mStagingBuffer == nullptr) {
            std::unique_ptr<StagingBufferBase> stagingBuffer;
            DAWN_TRY_ASSIGN(stagingBuffer,
                            mDevice->CreateStagingBuffer(targetRingBuffer->mAllocator.GetSize()));
            targetRingBuffer->mStagingBuffer = std::move(stagingBuffer);
        }

        ASSERT(targetRingBuffer->mStagingBuffer != nullptr);

        UploadHandle uploadHandle;
        uploadHandle.stagingBuffer = targetRingBuffer->mStagingBuffer.get();
        uploadHandle.mappedBuffer =
            static_cast<uint8_t*>(uploadHandle.stagingBuffer->GetMappedPointer()) + startOffset;
        uploadHandle.startOffset = startOffset;

        return uploadHandle;
    }

    void DynamicUploader::Deallocate(Serial lastCompletedSerial) {
        // Reclaim memory within the ring buffers by ticking (or removing requests no longer
        // in-flight).
        for (size_t i = 0; i < mRingBuffers.size(); ++i) {
            mRingBuffers[i]->mAllocator.Deallocate(lastCompletedSerial);

            // Never erase the last buffer as to prevent re-creating smaller buffers
            // again. The last buffer is the largest.
            if (mRingBuffers[i]->mAllocator.Empty() && i < mRingBuffers.size() - 1) {
                mRingBuffers.erase(mRingBuffers.begin() + i);
            }
        }
        mReleasedStagingBuffers.ClearUpTo(lastCompletedSerial);
    }
}  // namespace dawn_native
