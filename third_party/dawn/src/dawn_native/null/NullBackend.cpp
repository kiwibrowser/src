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

// NullBackend.cpp: contains the definition of symbols exported by NullBackend.h so that they
// can be compiled twice: once export (shared library), once not exported (static library)

#include "dawn_native/NullBackend.h"

#include "common/SwapChainUtils.h"
#include "dawn_native/null/DeviceNull.h"

namespace dawn_native { namespace null {

    DawnSwapChainImplementation CreateNativeSwapChainImpl() {
        DawnSwapChainImplementation impl;
        impl = CreateSwapChainImplementation(new NativeSwapChainImpl());
        impl.textureUsage = WGPUTextureUsage_Present;
        return impl;
    }

}}  // namespace dawn_native::null
