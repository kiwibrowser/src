// Copyright 2020 The Dawn Authors
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

#include "dawn_native/d3d12/PageableD3D12.h"

namespace dawn_native { namespace d3d12 {
    Pageable::Pageable(ComPtr<ID3D12Pageable> d3d12Pageable,
                       MemorySegment memorySegment,
                       uint64_t size)
        : mD3d12Pageable(std::move(d3d12Pageable)), mMemorySegment(memorySegment), mSize(size) {
    }

    // When a pageable is destroyed, it no longer resides in resident memory, so we must evict
    // it from the LRU cache. If this heap is not manually removed from the LRU-cache, the
    // ResidencyManager will attempt to use it after it has been deallocated.
    Pageable::~Pageable() {
        if (IsInResidencyLRUCache()) {
            RemoveFromList();
        }
    }

    ID3D12Pageable* Pageable::GetD3D12Pageable() const {
        return mD3d12Pageable.Get();
    }

    Serial Pageable::GetLastUsage() const {
        return mLastUsage;
    }

    void Pageable::SetLastUsage(Serial serial) {
        mLastUsage = serial;
    }

    uint64_t Pageable::GetLastSubmission() const {
        return mLastSubmission;
    }

    void Pageable::SetLastSubmission(Serial serial) {
        mLastSubmission = serial;
    }

    MemorySegment Pageable::GetMemorySegment() const {
        return mMemorySegment;
    }

    uint64_t Pageable::GetSize() const {
        return mSize;
    }

    bool Pageable::IsInResidencyLRUCache() const {
        return IsInList();
    }

    void Pageable::IncrementResidencyLock() {
        mResidencyLockRefCount++;
    }

    void Pageable::DecrementResidencyLock() {
        mResidencyLockRefCount--;
    }

    bool Pageable::IsResidencyLocked() const {
        return mResidencyLockRefCount != 0;
    }
}}  // namespace dawn_native::d3d12
