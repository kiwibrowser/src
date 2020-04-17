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

#include "dawn_native/Device.h"
#include "dawn_native/Texture.h"
#include "dawn_native/ValidationUtils_autogen.h"

namespace dawn_native {

    namespace {

        class ErrorSwapChain : public SwapChainBase {
          public:
            ErrorSwapChain(DeviceBase* device) : SwapChainBase(device, ObjectBase::kError) {
            }

          private:
            TextureBase* GetNextTextureImpl(const TextureDescriptor*) override {
                UNREACHABLE();
            }

            void OnBeforePresent(TextureBase* texture) override {
                UNREACHABLE();
            }
        };

    }  // anonymous namespace

    MaybeError ValidateSwapChainDescriptor(const DeviceBase* device,
                                           const SwapChainDescriptor* descriptor) {
        if (descriptor->implementation == 0) {
            return DAWN_VALIDATION_ERROR("Null implementation for the swapchain");
        }

        DawnSwapChainImplementation* impl =
            reinterpret_cast<DawnSwapChainImplementation*>(descriptor->implementation);

        if (!impl->Init || !impl->Destroy || !impl->Configure || !impl->GetNextTexture ||
            !impl->Present) {
            return DAWN_VALIDATION_ERROR("Implementation is incomplete");
        }

        return {};
    }

    // SwapChain

    SwapChainBase::SwapChainBase(DeviceBase* device, const SwapChainDescriptor* descriptor)
        : ObjectBase(device),
          mImplementation(
              *reinterpret_cast<DawnSwapChainImplementation*>(descriptor->implementation)) {
    }

    SwapChainBase::SwapChainBase(DeviceBase* device, ObjectBase::ErrorTag tag)
        : ObjectBase(device, tag) {
    }

    SwapChainBase::~SwapChainBase() {
        if (!IsError()) {
            const auto& im = GetImplementation();
            im.Destroy(im.userData);
        }
    }

    // static
    SwapChainBase* SwapChainBase::MakeError(DeviceBase* device) {
        return new ErrorSwapChain(device);
    }

    void SwapChainBase::Configure(dawn::TextureFormat format,
                                  dawn::TextureUsageBit allowedUsage,
                                  uint32_t width,
                                  uint32_t height) {
        if (GetDevice()->ConsumedError(ValidateConfigure(format, allowedUsage, width, height))) {
            return;
        }
        ASSERT(!IsError());

        allowedUsage |= dawn::TextureUsageBit::Present;

        mFormat = format;
        mAllowedUsage = allowedUsage;
        mWidth = width;
        mHeight = height;
        mImplementation.Configure(mImplementation.userData, static_cast<DawnTextureFormat>(format),
                                  static_cast<DawnTextureUsageBit>(allowedUsage), width, height);
    }

    TextureBase* SwapChainBase::GetNextTexture() {
        if (GetDevice()->ConsumedError(ValidateGetNextTexture())) {
            return TextureBase::MakeError(GetDevice());
        }
        ASSERT(!IsError());

        TextureDescriptor descriptor;
        descriptor.dimension = dawn::TextureDimension::e2D;
        descriptor.size.width = mWidth;
        descriptor.size.height = mHeight;
        descriptor.size.depth = 1;
        descriptor.arrayLayerCount = 1;
        descriptor.sampleCount = 1;
        descriptor.format = mFormat;
        descriptor.mipLevelCount = 1;
        descriptor.usage = mAllowedUsage;

        auto* texture = GetNextTextureImpl(&descriptor);
        mLastNextTexture = texture;
        return texture;
    }

    void SwapChainBase::Present(TextureBase* texture) {
        if (GetDevice()->ConsumedError(ValidatePresent(texture))) {
            return;
        }
        ASSERT(!IsError());

        OnBeforePresent(texture);

        mImplementation.Present(mImplementation.userData);
    }

    const DawnSwapChainImplementation& SwapChainBase::GetImplementation() {
        ASSERT(!IsError());
        return mImplementation;
    }

    MaybeError SwapChainBase::ValidateConfigure(dawn::TextureFormat format,
                                                dawn::TextureUsageBit allowedUsage,
                                                uint32_t width,
                                                uint32_t height) const {
        DAWN_TRY(GetDevice()->ValidateObject(this));

        DAWN_TRY(ValidateTextureUsageBit(allowedUsage));
        DAWN_TRY(ValidateTextureFormat(format));

        if (width == 0 || height == 0) {
            return DAWN_VALIDATION_ERROR("Swap chain cannot be configured to zero size");
        }

        return {};
    }

    MaybeError SwapChainBase::ValidateGetNextTexture() const {
        DAWN_TRY(GetDevice()->ValidateObject(this));

        if (mWidth == 0) {
            // If width is 0, it implies swap chain has never been configured
            return DAWN_VALIDATION_ERROR("Swap chain needs to be configured before GetNextTexture");
        }

        return {};
    }

    MaybeError SwapChainBase::ValidatePresent(TextureBase* texture) const {
        DAWN_TRY(GetDevice()->ValidateObject(this));
        DAWN_TRY(GetDevice()->ValidateObject(texture));

        // This also checks that the texture is valid since mLastNextTexture is always valid.
        if (texture != mLastNextTexture) {
            return DAWN_VALIDATION_ERROR(
                "Tried to present something other than the last NextTexture");
        }

        return {};
    }

}  // namespace dawn_native
