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

#ifndef DAWNNATIVE_SWAPCHAIN_H_
#define DAWNNATIVE_SWAPCHAIN_H_

#include "dawn_native/Error.h"
#include "dawn_native/Forward.h"
#include "dawn_native/ObjectBase.h"

#include "dawn/dawn_wsi.h"
#include "dawn_native/dawn_platform.h"

namespace dawn_native {

    MaybeError ValidateSwapChainDescriptor(const DeviceBase* device,
                                           const Surface* surface,
                                           const SwapChainDescriptor* descriptor);

    TextureDescriptor GetSwapChainBaseTextureDescriptor(NewSwapChainBase* swapChain);

    class SwapChainBase : public ObjectBase {
      public:
        SwapChainBase(DeviceBase* device);

        static SwapChainBase* MakeError(DeviceBase* device);

        // Dawn API
        virtual void Configure(wgpu::TextureFormat format,
                               wgpu::TextureUsage allowedUsage,
                               uint32_t width,
                               uint32_t height) = 0;
        virtual TextureViewBase* GetCurrentTextureView() = 0;
        virtual void Present() = 0;

      protected:
        SwapChainBase(DeviceBase* device, ObjectBase::ErrorTag tag);
        ~SwapChainBase() override;
    };

    // The base class for implementation-based SwapChains that are deprecated.
    class OldSwapChainBase : public SwapChainBase {
      public:
        OldSwapChainBase(DeviceBase* device, const SwapChainDescriptor* descriptor);

        static SwapChainBase* MakeError(DeviceBase* device);

        // Dawn API
        void Configure(wgpu::TextureFormat format,
                       wgpu::TextureUsage allowedUsage,
                       uint32_t width,
                       uint32_t height) override;
        TextureViewBase* GetCurrentTextureView() override;
        void Present() override;

      protected:
        ~OldSwapChainBase() override;
        const DawnSwapChainImplementation& GetImplementation();
        virtual TextureBase* GetNextTextureImpl(const TextureDescriptor*) = 0;
        virtual MaybeError OnBeforePresent(TextureViewBase* view) = 0;

      private:
        MaybeError ValidateConfigure(wgpu::TextureFormat format,
                                     wgpu::TextureUsage allowedUsage,
                                     uint32_t width,
                                     uint32_t height) const;
        MaybeError ValidateGetCurrentTextureView() const;
        MaybeError ValidatePresent() const;

        DawnSwapChainImplementation mImplementation = {};
        wgpu::TextureFormat mFormat = {};
        wgpu::TextureUsage mAllowedUsage;
        uint32_t mWidth = 0;
        uint32_t mHeight = 0;
        Ref<TextureBase> mCurrentTexture;
        Ref<TextureViewBase> mCurrentTextureView;
    };

    // The base class for surface-based SwapChains that aren't ready yet.
    class NewSwapChainBase : public SwapChainBase {
      public:
        NewSwapChainBase(DeviceBase* device,
                         Surface* surface,
                         const SwapChainDescriptor* descriptor);

        // This is called when the swapchain is detached for any reason:
        //
        //  - The swapchain is being destroyed.
        //  - The surface it is attached to is being destroyed.
        //  - The swapchain is being replaced by another one on the surface.
        //
        // The call for the old swapchain being replaced should be called inside the backend
        // implementation of SwapChains. This is to allow them to acquire any resources before
        // calling detach to make a seamless transition from the previous swapchain.
        //
        // Likewise the call for the swapchain being destroyed must be done in the backend's
        // swapchain's destructor since C++ says it is UB to call virtual methods in the base class
        // destructor.
        void DetachFromSurface();

        // Dawn API
        void Configure(wgpu::TextureFormat format,
                       wgpu::TextureUsage allowedUsage,
                       uint32_t width,
                       uint32_t height) override;
        TextureViewBase* GetCurrentTextureView() override;
        void Present() override;

        uint32_t GetWidth() const;
        uint32_t GetHeight() const;
        wgpu::TextureFormat GetFormat() const;
        wgpu::TextureUsage GetUsage() const;
        wgpu::PresentMode GetPresentMode() const;
        Surface* GetSurface() const;
        bool IsAttached() const;
        wgpu::BackendType GetBackendType() const;

      protected:
        ~NewSwapChainBase() override;

      private:
        bool mAttached;
        uint32_t mWidth;
        uint32_t mHeight;
        wgpu::TextureFormat mFormat;
        wgpu::TextureUsage mUsage;
        wgpu::PresentMode mPresentMode;

        // This is a weak reference to the surface. If the surface is destroyed it will call
        // DetachFromSurface and mSurface will be updated to nullptr.
        Surface* mSurface = nullptr;
        Ref<TextureViewBase> mCurrentTextureView;

        MaybeError ValidatePresent() const;
        MaybeError ValidateGetCurrentTextureView() const;

        // GetCurrentTextureViewImpl and PresentImpl are guaranteed to be called in an interleaved
        // manner, starting with GetCurrentTextureViewImpl.

        // The returned texture view must match the swapchain descriptor exactly.
        virtual ResultOrError<TextureViewBase*> GetCurrentTextureViewImpl() = 0;
        // The call to present must destroy the current view's texture so further access to it are
        // invalid.
        virtual MaybeError PresentImpl() = 0;

        // Guaranteed to be called exactly once during the lifetime of the SwapChain. After it is
        // called no other virtual method can be called.
        virtual void DetachFromSurfaceImpl() = 0;
    };

}  // namespace dawn_native

#endif  // DAWNNATIVE_SWAPCHAIN_H_
