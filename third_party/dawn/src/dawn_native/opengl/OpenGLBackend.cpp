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

// OpenGLBackend.cpp: contains the definition of symbols exported by OpenGLBackend.h so that they
// can be compiled twice: once export (shared library), once not exported (static library)

#include "dawn_native/OpenGLBackend.h"

#include "common/SwapChainUtils.h"
#include "dawn_native/opengl/DeviceGL.h"
#include "dawn_native/opengl/NativeSwapChainImplGL.h"

namespace dawn_native { namespace opengl {

    AdapterDiscoveryOptions::AdapterDiscoveryOptions()
        : AdapterDiscoveryOptionsBase(WGPUBackendType_OpenGL) {
    }

    DawnSwapChainImplementation CreateNativeSwapChainImpl(WGPUDevice device,
                                                          PresentCallback present,
                                                          void* presentUserdata) {
        Device* backendDevice = reinterpret_cast<Device*>(device);

        DawnSwapChainImplementation impl;
        impl = CreateSwapChainImplementation(
            new NativeSwapChainImpl(backendDevice, present, presentUserdata));
        impl.textureUsage = WGPUTextureUsage_Present;

        return impl;
    }

    WGPUTextureFormat GetNativeSwapChainPreferredFormat(
        const DawnSwapChainImplementation* swapChain) {
        NativeSwapChainImpl* impl = reinterpret_cast<NativeSwapChainImpl*>(swapChain->userData);
        return static_cast<WGPUTextureFormat>(impl->GetPreferredFormat());
    }

}}  // namespace dawn_native::opengl
