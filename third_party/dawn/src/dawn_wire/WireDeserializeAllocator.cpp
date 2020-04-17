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

#include "dawn_wire/WireDeserializeAllocator.h"

#include <algorithm>

namespace dawn_wire {
    WireDeserializeAllocator::WireDeserializeAllocator() {
        Reset();
    }

    WireDeserializeAllocator::~WireDeserializeAllocator() {
        Reset();
    }

    void* WireDeserializeAllocator::GetSpace(size_t size) {
        // Return space in the current buffer if possible first.
        if (mRemainingSize >= size) {
            char* buffer = mCurrentBuffer;
            mCurrentBuffer += size;
            mRemainingSize -= size;
            return buffer;
        }

        // Otherwise allocate a new buffer and try again.
        size_t allocationSize = std::max(size, size_t(2048));
        char* allocation = static_cast<char*>(malloc(allocationSize));
        if (allocation == nullptr) {
            return nullptr;
        }

        mAllocations.push_back(allocation);
        mCurrentBuffer = allocation;
        mRemainingSize = allocationSize;
        return GetSpace(size);
    }

    void WireDeserializeAllocator::Reset() {
        for (auto allocation : mAllocations) {
            free(allocation);
        }
        mAllocations.clear();

        // The initial buffer is the inline buffer so that some allocations can be skipped
        mCurrentBuffer = mStaticBuffer;
        mRemainingSize = sizeof(mStaticBuffer);
    }
}  // namespace dawn_wire
