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

#include "dawn_wire/client/Fence.h"

namespace dawn_wire { namespace client {

    Fence::~Fence() {
        // Callbacks need to be fired in all cases, as they can handle freeing resources
        // so we call them with "Unknown" status.
        for (auto& request : requests.IterateAll()) {
            request.completionCallback(DAWN_FENCE_COMPLETION_STATUS_UNKNOWN, request.userdata);
        }
        requests.Clear();
    }

    void Fence::CheckPassedFences() {
        for (auto& request : requests.IterateUpTo(completedValue)) {
            request.completionCallback(DAWN_FENCE_COMPLETION_STATUS_SUCCESS, request.userdata);
        }
        requests.ClearUpTo(completedValue);
    }

}}  // namespace dawn_wire::client
