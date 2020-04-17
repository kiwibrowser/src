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

#include "common/Assert.h"
#include "dawn_wire/server/Server.h"

namespace dawn_wire { namespace server {

    bool Server::DoQueueSignal(DawnQueue cSelf, DawnFence cFence, uint64_t signalValue) {
        if (cFence == nullptr) {
            return false;
        }

        mProcs.queueSignal(cSelf, cFence, signalValue);

        ObjectId fenceId = FenceObjectIdTable().Get(cFence);
        ASSERT(fenceId != 0);
        auto* fence = FenceObjects().Get(fenceId);
        ASSERT(fence != nullptr);

        FenceCompletionUserdata* userdata = new FenceCompletionUserdata;
        userdata->server = this;
        userdata->fence = ObjectHandle{fenceId, fence->serial};
        userdata->value = signalValue;

        mProcs.fenceOnCompletion(cFence, signalValue, ForwardFenceCompletedValue, userdata);
        return true;
    }

}}  // namespace dawn_wire::server
