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

#include "common/Result.h"

// Implementation details of the tagged pointer Results
namespace detail {

    intptr_t MakePayload(const void* pointer, PayloadType type) {
        intptr_t payload = reinterpret_cast<intptr_t>(pointer);
        ASSERT((payload & 3) == 0);
        return payload | type;
    }

    PayloadType GetPayloadType(intptr_t payload) {
        return static_cast<PayloadType>(payload & 3);
    }

}  // namespace detail
