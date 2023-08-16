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

#ifndef DAWNNATIVE_RESOURCEHEAPALLOCATOR_H_
#define DAWNNATIVE_RESOURCEHEAPALLOCATOR_H_

#include "dawn_native/Error.h"
#include "dawn_native/ResourceHeap.h"

#include <memory>

namespace dawn_native {

    // Interface for backend allocators that create memory heaps resoruces can be suballocated in.
    class ResourceHeapAllocator {
      public:
        virtual ~ResourceHeapAllocator() = default;

        virtual ResultOrError<std::unique_ptr<ResourceHeapBase>> AllocateResourceHeap(
            uint64_t size) = 0;
        virtual void DeallocateResourceHeap(std::unique_ptr<ResourceHeapBase> allocation) = 0;
    };

}  // namespace dawn_native

#endif  // DAWNNATIVE_RESOURCEHEAPALLOCATOR_H_
