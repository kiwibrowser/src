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

#ifndef COMMON_SWAPCHAINUTILS_H_
#define COMMON_SWAPCHAINUTILS_H_

#include "dawn/dawn_wsi.h"

template <typename T>
DawnSwapChainImplementation CreateSwapChainImplementation(T* swapChain) {
    DawnSwapChainImplementation impl = {};
    impl.userData = swapChain;
    impl.Init = [](void* userData, void* wsiContext) {
        auto* ctx = static_cast<typename T::WSIContext*>(wsiContext);
        reinterpret_cast<T*>(userData)->Init(ctx);
    };
    impl.Destroy = [](void* userData) { delete reinterpret_cast<T*>(userData); };
    impl.Configure = [](void* userData, WGPUTextureFormat format, WGPUTextureUsage allowedUsage,
                        uint32_t width, uint32_t height) {
        return static_cast<T*>(userData)->Configure(format, allowedUsage, width, height);
    };
    impl.GetNextTexture = [](void* userData, DawnSwapChainNextTexture* nextTexture) {
        return static_cast<T*>(userData)->GetNextTexture(nextTexture);
    };
    impl.Present = [](void* userData) { return static_cast<T*>(userData)->Present(); };
    return impl;
}

#endif  // COMMON_SWAPCHAINUTILS_H_
