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

#include "dawn_native/ResourceMemoryAllocation.h"
#include "common/Assert.h"

namespace dawn_native {

    ResourceMemoryAllocation::ResourceMemoryAllocation()
        : mOffset(0), mResourceHeap(nullptr), mMappedPointer(nullptr) {
    }

    ResourceMemoryAllocation::ResourceMemoryAllocation(const AllocationInfo& info,
                                                       uint64_t offset,
                                                       ResourceHeapBase* resourceHeap,
                                                       uint8_t* mappedPointer)
        : mInfo(info), mOffset(offset), mResourceHeap(resourceHeap), mMappedPointer(mappedPointer) {
    }

    ResourceHeapBase* ResourceMemoryAllocation::GetResourceHeap() const {
        ASSERT(mInfo.mMethod != AllocationMethod::kInvalid);
        return mResourceHeap;
    }

    uint64_t ResourceMemoryAllocation::GetOffset() const {
        ASSERT(mInfo.mMethod != AllocationMethod::kInvalid);
        return mOffset;
    }

    AllocationInfo ResourceMemoryAllocation::GetInfo() const {
        return mInfo;
    }

    uint8_t* ResourceMemoryAllocation::GetMappedPointer() const {
        return mMappedPointer;
    }

    void ResourceMemoryAllocation::Invalidate() {
        mResourceHeap = nullptr;
        mInfo = {};
    }
}  // namespace dawn_native
