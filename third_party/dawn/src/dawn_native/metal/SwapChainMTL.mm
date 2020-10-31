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

#include "dawn_native/Surface.h"
#include "dawn_native/metal/DeviceMTL.h"
#include "dawn_native/metal/TextureMTL.h"

#include <dawn/dawn_wsi.h>

#import <QuartzCore/CAMetalLayer.h>

namespace dawn_native { namespace metal {

    // OldSwapChain

    OldSwapChain::OldSwapChain(Device* device, const SwapChainDescriptor* descriptor)
        : OldSwapChainBase(device, descriptor) {
        const auto& im = GetImplementation();
        DawnWSIContextMetal wsiContext = {};
        wsiContext.device = ToBackend(GetDevice())->GetMTLDevice();
        wsiContext.queue = ToBackend(GetDevice())->GetMTLQueue();
        im.Init(im.userData, &wsiContext);
    }

    OldSwapChain::~OldSwapChain() {
    }

    TextureBase* OldSwapChain::GetNextTextureImpl(const TextureDescriptor* descriptor) {
        const auto& im = GetImplementation();
        DawnSwapChainNextTexture next = {};
        DawnSwapChainError error = im.GetNextTexture(im.userData, &next);
        if (error) {
            GetDevice()->HandleError(InternalErrorType::Internal, error);
            return nullptr;
        }

        id<MTLTexture> nativeTexture = reinterpret_cast<id<MTLTexture>>(next.texture.ptr);
        return new Texture(ToBackend(GetDevice()), descriptor, nativeTexture);
    }

    MaybeError OldSwapChain::OnBeforePresent(TextureViewBase*) {
        return {};
    }

    // SwapChain

    SwapChain::SwapChain(Device* device,
                         Surface* surface,
                         NewSwapChainBase* previousSwapChain,
                         const SwapChainDescriptor* descriptor)
        : NewSwapChainBase(device, surface, descriptor) {
        ASSERT(surface->GetType() == Surface::Type::MetalLayer);

        if (previousSwapChain != nullptr) {
            // TODO(cwallez@chromium.org): figure out what should happen when surfaces are used by
            // multiple backends one after the other. It probably needs to block until the backend
            // and GPU are completely finished with the previous swapchain.
            ASSERT(previousSwapChain->GetBackendType() == wgpu::BackendType::Metal);
            previousSwapChain->DetachFromSurface();
        }

        mLayer = static_cast<CAMetalLayer*>(surface->GetMetalLayer());
        ASSERT(mLayer != nullptr);

        CGSize size = {};
        size.width = GetWidth();
        size.height = GetHeight();
        [mLayer setDrawableSize:size];

        [mLayer setFramebufferOnly:(GetUsage() == wgpu::TextureUsage::OutputAttachment)];
        [mLayer setDevice:ToBackend(GetDevice())->GetMTLDevice()];
        [mLayer setPixelFormat:MetalPixelFormat(GetFormat())];

#if defined(DAWN_PLATFORM_MACOS)
        if (@available(macos 10.13, *)) {
            [mLayer setDisplaySyncEnabled:(GetPresentMode() != wgpu::PresentMode::Immediate)];
        }
#endif  // defined(DAWN_PLATFORM_MACOS)

        // There is no way to control Fifo vs. Mailbox in Metal.
    }

    SwapChain::~SwapChain() {
        DetachFromSurface();
    }

    MaybeError SwapChain::PresentImpl() {
        ASSERT(mCurrentDrawable != nil);
        [mCurrentDrawable present];

        mTexture->Destroy();
        mTexture = nullptr;

        [mCurrentDrawable release];
        mCurrentDrawable = nil;

        return {};
    }

    ResultOrError<TextureViewBase*> SwapChain::GetCurrentTextureViewImpl() {
        ASSERT(mCurrentDrawable == nil);
        mCurrentDrawable = [mLayer nextDrawable];
        [mCurrentDrawable retain];

        TextureDescriptor textureDesc = GetSwapChainBaseTextureDescriptor(this);

        // mTexture will add a reference to mCurrentDrawable.texture to keep it alive.
        mTexture =
            AcquireRef(new Texture(ToBackend(GetDevice()), &textureDesc, mCurrentDrawable.texture));
        return mTexture->CreateView(nullptr);
    }

    void SwapChain::DetachFromSurfaceImpl() {
        ASSERT((mTexture.Get() == nullptr) == (mCurrentDrawable == nil));

        if (mTexture.Get() != nullptr) {
            mTexture->Destroy();
            mTexture = nullptr;

            [mCurrentDrawable release];
            mCurrentDrawable = nil;
        }
    }

}}  // namespace dawn_native::metal
