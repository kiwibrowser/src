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

#ifndef DAWNNATIVE_D3D12_BACKENDD3D12_H_
#define DAWNNATIVE_D3D12_BACKENDD3D12_H_

#include "dawn_native/BackendConnection.h"

#include "dawn_native/d3d12/d3d12_platform.h"

namespace dawn_native { namespace d3d12 {

    class PlatformFunctions;

    class Backend : public BackendConnection {
      public:
        Backend(InstanceBase* instance);

        MaybeError Initialize();

        ComPtr<IDXGIFactory4> GetFactory() const;
        const PlatformFunctions* GetFunctions() const;

        std::vector<std::unique_ptr<AdapterBase>> DiscoverDefaultAdapters() override;

      private:
        // Keep mFunctions as the first member so that in the destructor it is freed last. Otherwise
        // the D3D12 DLLs are unloaded before we are done using them.
        std::unique_ptr<PlatformFunctions> mFunctions;
        ComPtr<IDXGIFactory4> mFactory;
    };

}}  // namespace dawn_native::d3d12

#endif  // DAWNNATIVE_D3D12_BACKENDD3D12_H_