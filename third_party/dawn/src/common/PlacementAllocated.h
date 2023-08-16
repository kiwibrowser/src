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

#ifndef COMMON_PLACEMENTALLOCATED_H_
#define COMMON_PLACEMENTALLOCATED_H_

#include <cstddef>

class PlacementAllocated {
  public:
    // Delete the default new operator so this can only be created with placement new.
    void* operator new(size_t) = delete;

    void* operator new(size_t size, void* ptr) {
        // Pass through the pointer of the allocation. This is essentially the default
        // placement-new implementation, but we must define it if we delete the default
        // new operator.
        return ptr;
    }

    void operator delete(void* ptr) {
        // Object is placement-allocated. Don't free the memory.
    }
};

#endif  // COMMON_PLACEMENTALLOCATED_H_
