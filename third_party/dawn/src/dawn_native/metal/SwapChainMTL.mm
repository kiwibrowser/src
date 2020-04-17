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

#include "dawn_native/metal/SwapChainMTL.h"

#include "dawn_native/metal/DeviceMTL.h"
#include "dawn_native/metal/TextureMTL.h"

#include <dawn/dawn_wsi.h>

namespace dawn_native { namespace metal {

    SwapChain::SwapChain(Device* device, const SwapChainDescriptor* descriptor)
        : SwapChainBase(device, descriptor) {
        const auto& im = GetImplementation();
        DawnWSIContextMetal wsiContext = {};
        wsiContext.device = ToBackend(GetDevice())->GetMTLDevice();
        im.Init(im.userData, &wsiContext);
    }

    SwapChain::~SwapChain() {
    }

    TextureBase* SwapChain::GetNextTextureImpl(const TextureDescriptor* descriptor) {
        const auto& im = GetImplementation();
        DawnSwapChainNextTexture next = {};
        DawnSwapChainError error = im.GetNextTexture(im.userData, &next);
        if (error) {
            GetDevice()->HandleError(error);
            return nullptr;
        }

        id<MTLTexture> nativeTexture = reinterpret_cast<id<MTLTexture>>(next.texture.ptr);
        return new Texture(ToBackend(GetDevice()), descriptor, nativeTexture);
    }

    void SwapChain::OnBeforePresent(TextureBase*) {
    }

}}  // namespace dawn_native::metal
