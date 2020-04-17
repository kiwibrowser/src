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

#ifndef DAWNNATIVE_RINGBUFFER_H_
#define DAWNNATIVE_RINGBUFFER_H_

#include "common/SerialQueue.h"
#include "dawn_native/StagingBuffer.h"

#include <memory>

// RingBuffer is the front-end implementation used to manage a ring buffer in GPU memory.
namespace dawn_native {

    struct UploadHandle {
        uint8_t* mappedBuffer = nullptr;
        size_t startOffset = 0;
        StagingBufferBase* stagingBuffer = nullptr;
    };

    class DeviceBase;

    class RingBuffer {
      public:
        RingBuffer(DeviceBase* device, size_t size);
        ~RingBuffer() = default;

        MaybeError Initialize();

        UploadHandle SubAllocate(size_t requestedSize);

        void Tick(Serial lastCompletedSerial);
        size_t GetSize() const;
        bool Empty() const;
        size_t GetUsedSize() const;
        StagingBufferBase* GetStagingBuffer() const;

        // Seperated for testing.
        void Track();

      private:
        std::unique_ptr<StagingBufferBase> mStagingBuffer;

        struct Request {
            size_t endOffset;
            size_t size;
        };

        SerialQueue<Request> mInflightRequests;  // Queue of the recorded sub-alloc requests (e.g.
                                                 // frame of resources).

        size_t mUsedEndOffset = 0;    // Tail of used sub-alloc requests (in bytes).
        size_t mUsedStartOffset = 0;  // Head of used sub-alloc requests (in bytes).
        size_t mBufferSize = 0;       // Max size of the ring buffer (in bytes).
        size_t mUsedSize = 0;  // Size of the sub-alloc requests (in bytes) of the ring buffer.
        size_t mCurrentRequestSize =
            0;  // Size of the sub-alloc requests (in bytes) of the current serial.

        DeviceBase* mDevice;
    };
}  // namespace dawn_native

#endif  // DAWNNATIVE_RINGBUFFER_H_