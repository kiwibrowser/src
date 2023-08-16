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

#ifndef DAWNNATIVE_METAL_SWAPCHAINMTL_H_
#define DAWNNATIVE_METAL_SWAPCHAINMTL_H_

#include "dawn_native/SwapChain.h"

@class CAMetalLayer;
@protocol CAMetalDrawable;

namespace dawn_native { namespace metal {

    class Device;
    class Texture;

    class OldSwapChain final : public OldSwapChainBase {
      public:
        OldSwapChain(Device* device, const SwapChainDescriptor* descriptor);

      protected:
        ~OldSwapChain() override;
        TextureBase* GetNextTextureImpl(const TextureDescriptor* descriptor) override;
        MaybeError OnBeforePresent(TextureViewBase* view) override;
    };

    class SwapChain final : public NewSwapChainBase {
      public:
        SwapChain(Device* device,
                  Surface* surface,
                  NewSwapChainBase* previousSwapChain,
                  const SwapChainDescriptor* descriptor);

      private:
        ~SwapChain() override;

        CAMetalLayer* mLayer = nullptr;

        id<CAMetalDrawable> mCurrentDrawable = nil;
        Ref<Texture> mTexture;

        MaybeError PresentImpl() override;
        ResultOrError<TextureViewBase*> GetCurrentTextureViewImpl() override;
        void DetachFromSurfaceImpl() override;
    };

}}  // namespace dawn_native::metal

#endif  // DAWNNATIVE_METAL_SWAPCHAINMTL_H_
