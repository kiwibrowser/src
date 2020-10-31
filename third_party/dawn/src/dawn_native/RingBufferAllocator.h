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

#ifndef DAWNNATIVE_RINGBUFFERALLOCATOR_H_
#define DAWNNATIVE_RINGBUFFERALLOCATOR_H_

#include "common/SerialQueue.h"

#include <limits>
#include <memory>

// RingBufferAllocator is the front-end implementation used to manage a ring buffer in GPU memory.
namespace dawn_native {

    class RingBufferAllocator {
      public:
        RingBufferAllocator() = default;
        RingBufferAllocator(uint64_t maxSize);
        ~RingBufferAllocator() = default;

        uint64_t Allocate(uint64_t allocationSize, Serial serial);
        void Deallocate(Serial lastCompletedSerial);

        uint64_t GetSize() const;
        bool Empty() const;
        uint64_t GetUsedSize() const;

        static constexpr uint64_t kInvalidOffset = std::numeric_limits<uint64_t>::max();

      private:
        struct Request {
            uint64_t endOffset;
            uint64_t size;
        };

        SerialQueue<Request> mInflightRequests;  // Queue of the recorded sub-alloc requests (e.g.
                                                 // frame of resources).

        uint64_t mUsedEndOffset = 0;    // Tail of used sub-alloc requests (in bytes).
        uint64_t mUsedStartOffset = 0;  // Head of used sub-alloc requests (in bytes).
        uint64_t mMaxBlockSize = 0;     // Max size of the ring buffer (in bytes).
        uint64_t mUsedSize = 0;  // Size of the sub-alloc requests (in bytes) of the ring buffer.
        uint64_t mCurrentRequestSize =
            0;  // Size of the sub-alloc requests (in bytes) of the current serial.
    };
}  // namespace dawn_native

#endif  // DAWNNATIVE_RINGBUFFERALLOCATOR_H_
