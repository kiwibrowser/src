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

#include "dawn_native/SwapChain.h"

#include "common/Constants.h"
#include "dawn_native/Adapter.h"
#include "dawn_native/Device.h"
#include "dawn_native/Surface.h"
#include "dawn_native/Texture.h"
#include "dawn_native/ValidationUtils_autogen.h"

namespace dawn_native {

    namespace {

        class ErrorSwapChain final : public SwapChainBase {
          public:
            ErrorSwapChain(DeviceBase* device) : SwapChainBase(device, ObjectBase::kError) {
            }

          private:
            void Configure(wgpu::TextureFormat format,
                           wgpu::TextureUsage allowedUsage,
                           uint32_t width,
                           uint32_t height) override {
                GetDevice()->ConsumedError(DAWN_VALIDATION_ERROR("error swapchain"));
            }

            TextureViewBase* GetCurrentTextureView() override {
                GetDevice()->ConsumedError(DAWN_VALIDATION_ERROR("error swapchain"));
                return TextureViewBase::MakeError(GetDevice());
            }

            void Present() override {
                GetDevice()->ConsumedError(DAWN_VALIDATION_ERROR("error swapchain"));
            }
        };

    }  // anonymous namespace

    MaybeError ValidateSwapChainDescriptor(const DeviceBase* device,
                                           const Surface* surface,
                                           const SwapChainDescriptor* descriptor) {
        if (descriptor->implementation != 0) {
            if (surface != nullptr) {
                return DAWN_VALIDATION_ERROR(
                    "Exactly one of surface or implementation must be set");
            }

            DawnSwapChainImplementation* impl =
                reinterpret_cast<DawnSwapChainImplementation*>(descriptor->implementation);

            if (!impl->Init || !impl->Destroy || !impl->Configure || !impl->GetNextTexture ||
                !impl->Present) {
                return DAWN_VALIDATION_ERROR("Implementation is incomplete");
            }

        } else {
            if (surface == nullptr) {
                return DAWN_VALIDATION_ERROR(
                    "At least one of surface or implementation must be set");
            }

            DAWN_TRY(ValidatePresentMode(descriptor->presentMode));

            // TODO(cwallez@chromium.org): Lift this restriction once
            // wgpu::Instance::GetPreferredSurfaceFormat is implemented.
            if (descriptor->format != wgpu::TextureFormat::BGRA8Unorm) {
                return DAWN_VALIDATION_ERROR("Format must (currently) be BGRA8Unorm");
            }

            if (descriptor->usage != wgpu::TextureUsage::OutputAttachment) {
                return DAWN_VALIDATION_ERROR("Usage must (currently) be OutputAttachment");
            }

            if (descriptor->width == 0 || descriptor->height == 0) {
                return DAWN_VALIDATION_ERROR("Swapchain size can't be empty");
            }

            if (descriptor->width > kMaxTextureSize || descriptor->height > kMaxTextureSize) {
                return DAWN_VALIDATION_ERROR("Swapchain size too big");
            }
        }

        return {};
    }

    TextureDescriptor GetSwapChainBaseTextureDescriptor(NewSwapChainBase* swapChain) {
        TextureDescriptor desc;
        desc.usage = swapChain->GetUsage();
        desc.dimension = wgpu::TextureDimension::e2D;
        desc.size = {swapChain->GetWidth(), swapChain->GetHeight(), 1};
        desc.format = swapChain->GetFormat();
        desc.mipLevelCount = 1;
        desc.sampleCount = 1;

        return desc;
    }

    // SwapChainBase

    SwapChainBase::SwapChainBase(DeviceBase* device) : ObjectBase(device) {
    }

    SwapChainBase::SwapChainBase(DeviceBase* device, ObjectBase::ErrorTag tag)
        : ObjectBase(device, tag) {
    }

    SwapChainBase::~SwapChainBase() {
    }

    // static
    SwapChainBase* SwapChainBase::MakeError(DeviceBase* device) {
        return new ErrorSwapChain(device);
    }

    // OldSwapChainBase

    OldSwapChainBase::OldSwapChainBase(DeviceBase* device, const SwapChainDescriptor* descriptor)
        : SwapChainBase(device),
          mImplementation(
              *reinterpret_cast<DawnSwapChainImplementation*>(descriptor->implementation)) {
    }

    OldSwapChainBase::~OldSwapChainBase() {
        if (!IsError()) {
            const auto& im = GetImplementation();
            im.Destroy(im.userData);
        }
    }

    void OldSwapChainBase::Configure(wgpu::TextureFormat format,
                                     wgpu::TextureUsage allowedUsage,
                                     uint32_t width,
                                     uint32_t height) {
        if (GetDevice()->ConsumedError(ValidateConfigure(format, allowedUsage, width, height))) {
            return;
        }
        ASSERT(!IsError());

        allowedUsage |= wgpu::TextureUsage::Present;

        mFormat = format;
        mAllowedUsage = allowedUsage;
        mWidth = width;
        mHeight = height;
        mImplementation.Configure(mImplementation.userData, static_cast<WGPUTextureFormat>(format),
                                  static_cast<WGPUTextureUsage>(allowedUsage), width, height);
    }

    TextureViewBase* OldSwapChainBase::GetCurrentTextureView() {
        if (GetDevice()->ConsumedError(ValidateGetCurrentTextureView())) {
            return TextureViewBase::MakeError(GetDevice());
        }
        ASSERT(!IsError());

        // Return the same current texture view until Present is called.
        if (mCurrentTextureView.Get() != nullptr) {
            // Calling GetCurrentTextureView always returns a new reference so add it even when
            // reuse the existing texture view.
            mCurrentTextureView->Reference();
            return mCurrentTextureView.Get();
        }

        // Create the backing texture and the view.
        TextureDescriptor descriptor;
        descriptor.dimension = wgpu::TextureDimension::e2D;
        descriptor.size.width = mWidth;
        descriptor.size.height = mHeight;
        descriptor.size.depth = 1;
        descriptor.sampleCount = 1;
        descriptor.format = mFormat;
        descriptor.mipLevelCount = 1;
        descriptor.usage = mAllowedUsage;

        // Get the texture but remove the external refcount because it is never passed outside
        // of dawn_native
        mCurrentTexture = AcquireRef(GetNextTextureImpl(&descriptor));

        mCurrentTextureView = mCurrentTexture->CreateView(nullptr);
        return mCurrentTextureView.Get();
    }

    void OldSwapChainBase::Present() {
        if (GetDevice()->ConsumedError(ValidatePresent())) {
            return;
        }
        ASSERT(!IsError());

        if (GetDevice()->ConsumedError(OnBeforePresent(mCurrentTextureView.Get()))) {
            return;
        }

        mImplementation.Present(mImplementation.userData);

        mCurrentTexture = nullptr;
        mCurrentTextureView = nullptr;
    }

    const DawnSwapChainImplementation& OldSwapChainBase::GetImplementation() {
        ASSERT(!IsError());
        return mImplementation;
    }

    MaybeError OldSwapChainBase::ValidateConfigure(wgpu::TextureFormat format,
                                                   wgpu::TextureUsage allowedUsage,
                                                   uint32_t width,
                                                   uint32_t height) const {
        DAWN_TRY(GetDevice()->ValidateIsAlive());
        DAWN_TRY(GetDevice()->ValidateObject(this));

        DAWN_TRY(ValidateTextureUsage(allowedUsage));
        DAWN_TRY(ValidateTextureFormat(format));

        if (width == 0 || height == 0) {
            return DAWN_VALIDATION_ERROR("Swap chain cannot be configured to zero size");
        }

        return {};
    }

    MaybeError OldSwapChainBase::ValidateGetCurrentTextureView() const {
        DAWN_TRY(GetDevice()->ValidateIsAlive());
        DAWN_TRY(GetDevice()->ValidateObject(this));

        if (mWidth == 0) {
            // If width is 0, it implies swap chain has never been configured
            return DAWN_VALIDATION_ERROR("Swap chain needs to be configured before GetNextTexture");
        }

        return {};
    }

    MaybeError OldSwapChainBase::ValidatePresent() const {
        DAWN_TRY(GetDevice()->ValidateIsAlive());
        DAWN_TRY(GetDevice()->ValidateObject(this));

        if (mCurrentTextureView.Get() == nullptr) {
            return DAWN_VALIDATION_ERROR(
                "Cannot call present without a GetCurrentTextureView call for this frame");
        }

        return {};
    }

    // Implementation of NewSwapChainBase

    NewSwapChainBase::NewSwapChainBase(DeviceBase* device,
                                       Surface* surface,
                                       const SwapChainDescriptor* descriptor)
        : SwapChainBase(device),
          mAttached(true),
          mWidth(descriptor->width),
          mHeight(descriptor->height),
          mFormat(descriptor->format),
          mUsage(descriptor->usage),
          mPresentMode(descriptor->presentMode),
          mSurface(surface) {
    }

    NewSwapChainBase::~NewSwapChainBase() {
        if (mCurrentTextureView.Get() != nullptr) {
            ASSERT(mCurrentTextureView->GetTexture()->GetTextureState() ==
                   TextureBase::TextureState::Destroyed);
        }

        ASSERT(!mAttached);
        ASSERT(mSurface == nullptr);
    }

    void NewSwapChainBase::DetachFromSurface() {
        if (mAttached) {
            DetachFromSurfaceImpl();
            GetSurface()->SetAttachedSwapChain(nullptr);
            mSurface = nullptr;
            mAttached = false;
        }
    }

    void NewSwapChainBase::Configure(wgpu::TextureFormat format,
                                     wgpu::TextureUsage allowedUsage,
                                     uint32_t width,
                                     uint32_t height) {
        GetDevice()->ConsumedError(
            DAWN_VALIDATION_ERROR("Configure is invalid for surface-based swapchains"));
    }

    TextureViewBase* NewSwapChainBase::GetCurrentTextureView() {
        if (GetDevice()->ConsumedError(ValidateGetCurrentTextureView())) {
            return TextureViewBase::MakeError(GetDevice());
        }

        if (mCurrentTextureView.Get() != nullptr) {
            // Calling GetCurrentTextureView always returns a new reference so add it even when
            // reusing the existing texture view.
            mCurrentTextureView->Reference();
            return mCurrentTextureView.Get();
        }

        TextureViewBase* view = nullptr;
        if (GetDevice()->ConsumedError(GetCurrentTextureViewImpl(), &view)) {
            return TextureViewBase::MakeError(GetDevice());
        }

        // Check that the return texture view matches exactly what was given for this descriptor.
        ASSERT(view->GetTexture()->GetFormat().format == mFormat);
        ASSERT((view->GetTexture()->GetUsage() & mUsage) == mUsage);
        ASSERT(view->GetLevelCount() == 1);
        ASSERT(view->GetLayerCount() == 1);
        ASSERT(view->GetDimension() == wgpu::TextureViewDimension::e2D);
        ASSERT(view->GetTexture()->GetMipLevelVirtualSize(view->GetBaseMipLevel()).width == mWidth);
        ASSERT(view->GetTexture()->GetMipLevelVirtualSize(view->GetBaseMipLevel()).height ==
               mHeight);

        mCurrentTextureView = view;
        return view;
    }

    void NewSwapChainBase::Present() {
        if (GetDevice()->ConsumedError(ValidatePresent())) {
            return;
        }

        if (GetDevice()->ConsumedError(PresentImpl())) {
            return;
        }

        ASSERT(mCurrentTextureView->GetTexture()->GetTextureState() ==
               TextureBase::TextureState::Destroyed);
        mCurrentTextureView = nullptr;
    }

    uint32_t NewSwapChainBase::GetWidth() const {
        return mWidth;
    }

    uint32_t NewSwapChainBase::GetHeight() const {
        return mHeight;
    }

    wgpu::TextureFormat NewSwapChainBase::GetFormat() const {
        return mFormat;
    }

    wgpu::TextureUsage NewSwapChainBase::GetUsage() const {
        return mUsage;
    }

    wgpu::PresentMode NewSwapChainBase::GetPresentMode() const {
        return mPresentMode;
    }

    Surface* NewSwapChainBase::GetSurface() const {
        return mSurface;
    }

    bool NewSwapChainBase::IsAttached() const {
        return mAttached;
    }

    wgpu::BackendType NewSwapChainBase::GetBackendType() const {
        return GetDevice()->GetAdapter()->GetBackendType();
    }

    MaybeError NewSwapChainBase::ValidatePresent() const {
        DAWN_TRY(GetDevice()->ValidateIsAlive());
        DAWN_TRY(GetDevice()->ValidateObject(this));

        if (!mAttached) {
            return DAWN_VALIDATION_ERROR("Presenting on detached swapchain");
        }

        if (mCurrentTextureView.Get() == nullptr) {
            return DAWN_VALIDATION_ERROR("Presenting without prior GetCurrentTextureView");
        }

        return {};
    }

    MaybeError NewSwapChainBase::ValidateGetCurrentTextureView() const {
        DAWN_TRY(GetDevice()->ValidateIsAlive());
        DAWN_TRY(GetDevice()->ValidateObject(this));

        if (!mAttached) {
            return DAWN_VALIDATION_ERROR("Getting view on detached swapchain");
        }

        return {};
    }

}  // namespace dawn_native
