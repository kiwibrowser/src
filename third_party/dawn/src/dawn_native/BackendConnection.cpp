// Copyright 2018 The Dawn Authors
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

#include "dawn_native/BackendConnection.h"

namespace dawn_native {

    BackendConnection::BackendConnection(InstanceBase* instance, wgpu::BackendType type)
        : mInstance(instance), mType(type) {
    }

    wgpu::BackendType BackendConnection::GetType() const {
        return mType;
    }

    InstanceBase* BackendConnection::GetInstance() const {
        return mInstance;
    }

    ResultOrError<std::vector<std::unique_ptr<AdapterBase>>> BackendConnection::DiscoverAdapters(
        const AdapterDiscoveryOptionsBase* options) {
        return DAWN_VALIDATION_ERROR("DiscoverAdapters not implemented for this backend.");
    }

}  // namespace dawn_native
