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

#ifndef DAWNNATIVE_BACKENDCONNECTION_H_
#define DAWNNATIVE_BACKENDCONNECTION_H_

#include "dawn_native/Adapter.h"
#include "dawn_native/DawnNative.h"

#include <memory>

namespace dawn_native {

    // An common interface for all backends. Mostly used to create adapters for a particular
    // backend.
    class BackendConnection {
      public:
        BackendConnection(InstanceBase* instance, wgpu::BackendType type);
        virtual ~BackendConnection() = default;

        wgpu::BackendType GetType() const;
        InstanceBase* GetInstance() const;

        // Returns all the adapters for the system that can be created by the backend, without extra
        // options (such as debug adapters, custom driver libraries, etc.)
        virtual std::vector<std::unique_ptr<AdapterBase>> DiscoverDefaultAdapters() = 0;

        // Returns new adapters created with the backend-specific options.
        virtual ResultOrError<std::vector<std::unique_ptr<AdapterBase>>> DiscoverAdapters(
            const AdapterDiscoveryOptionsBase* options);

      private:
        InstanceBase* mInstance = nullptr;
        wgpu::BackendType mType;
    };

}  // namespace dawn_native

#endif  // DAWNNATIVE_BACKENDCONNECTION_H_
