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

#include "dawn_native/RefCounted.h"

#include "common/Assert.h"

namespace dawn_native {

    RefCounted::RefCounted() {
    }

    RefCounted::~RefCounted() {
    }

    uint64_t RefCounted::GetRefCount() const {
        return mRefCount;
    }

    void RefCounted::Reference() {
        ASSERT(mRefCount != 0);
        mRefCount++;
    }

    void RefCounted::Release() {
        ASSERT(mRefCount != 0);

        mRefCount--;
        if (mRefCount == 0) {
            delete this;
        }
    }

}  // namespace dawn_native
