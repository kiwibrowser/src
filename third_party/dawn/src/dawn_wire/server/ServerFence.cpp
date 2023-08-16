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

#include "dawn_wire/server/Server.h"

#include <memory>

namespace dawn_wire { namespace server {

    void Server::ForwardFenceCompletedValue(WGPUFenceCompletionStatus status, void* userdata) {
        auto data = static_cast<FenceCompletionUserdata*>(userdata);
        data->server->OnFenceCompletedValueUpdated(status, data);
    }

    void Server::OnFenceCompletedValueUpdated(WGPUFenceCompletionStatus status,
                                              FenceCompletionUserdata* userdata) {
        std::unique_ptr<FenceCompletionUserdata> data(userdata);

        if (status != WGPUFenceCompletionStatus_Success) {
            return;
        }

        ReturnFenceUpdateCompletedValueCmd cmd;
        cmd.fence = data->fence;
        cmd.value = data->value;

        SerializeCommand(cmd);
    }

}}  // namespace dawn_wire::server
