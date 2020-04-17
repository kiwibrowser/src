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
    }

    ResultOrError<std::unique_ptr<StagingBufferBase>> DynamicUploader::CreateStagingBuffer(
        size_t size) {
        std::unique_ptr<StagingBufferBase> stagingBuffer;
        DAWN_TRY_ASSIGN(stagingBuffer, mDevice->CreateStagingBuffer(size));
        DAWN_TRY(stagingBuffer->Initialize());
        return stagingBuffer;
    }

    void DynamicUploader::ReleaseStagingBuffer(std::unique_ptr<StagingBufferBase> stagingBuffer) {
        mReleasedStagingBuffers.Enqueue(std::move(stagingBuffer),
                                        mDevice->GetPendingCommandSerial());
    }

    MaybeError DynamicUploader::CreateAndAppendBuffer(size_t size) {
        std::unique_ptr<RingBuffer> ringBuffer = std::make_unique<RingBuffer>(mDevice, size);
        DAWN_TRY(ringBuffer->Initialize());
        mRingBuffers.emplace_back(std::move(ringBuffer));
        return {};
    }

    ResultOrError<UploadHandle> DynamicUploader::Allocate(uint32_t size, uint32_t alignment) {
        ASSERT(IsPowerOfTwo(alignment));

        // Align the requested allocation size
        const size_t alignedSize = Align(size, alignment);

        RingBuffer* largestRingBuffer = GetLargestBuffer();
        UploadHandle uploadHandle = largestRingBuffer->SubAllocate(alignedSize);

        // Upon failure, append a newly created (and much larger) ring buffer to fulfill the
        // request.
        if (uploadHandle.mappedBuffer == nullptr) {
            // Compute the new max size (in powers of two to preserve alignment).
            size_t newMaxSize = largestRingBuffer->GetSize() * 2;
            while (newMaxSize < size) {
                newMaxSize *= 2;
            }

            // TODO(bryan.bernhart@intel.com): Fall-back to no sub-allocations should this fail.
            DAWN_TRY(CreateAndAppendBuffer(newMaxSize));
            largestRingBuffer = GetLargestBuffer();
            uploadHandle = largestRingBuffer->SubAllocate(alignedSize);
        }

        uploadHandle.stagingBuffer = largestRingBuffer->GetStagingBuffer();

        return uploadHandle;
    }

    void DynamicUploader::Tick(Serial lastCompletedSerial) {
        // Reclaim memory within the ring buffers by ticking (or removing requests no longer
        // in-flight).
        for (size_t i = 0; i < mRingBuffers.size(); ++i) {
            mRingBuffers[i]->Tick(lastCompletedSerial);

            // Never erase the last buffer as to prevent re-creating smaller buffers
            // again. The last buffer is the largest.
            if (mRingBuffers[i]->Empty() && i < mRingBuffers.size() - 1) {
                mRingBuffers.erase(mRingBuffers.begin() + i);
            }
        }
        mReleasedStagingBuffers.ClearUpTo(lastCompletedSerial);
    }

    RingBuffer* DynamicUploader::GetLargestBuffer() {
        ASSERT(!mRingBuffers.empty());
        return mRingBuffers.back().get();
    }

    bool DynamicUploader::IsEmpty() const {
        return mRingBuffers.empty();
    }
}  // namespace dawn_native