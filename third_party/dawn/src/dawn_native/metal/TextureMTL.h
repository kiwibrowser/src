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

#ifndef DAWNNATIVE_METAL_TEXTUREMTL_H_
#define DAWNNATIVE_METAL_TEXTUREMTL_H_

#include "dawn_native/Texture.h"

#include <IOSurface/IOSurfaceRef.h>
#import <Metal/Metal.h>
#include "dawn_native/DawnNative.h"

namespace dawn_native { namespace metal {

    class Device;

    MTLPixelFormat MetalPixelFormat(wgpu::TextureFormat format);
    MaybeError ValidateIOSurfaceCanBeWrapped(const DeviceBase* device,
                                             const TextureDescriptor* descriptor,
                                             IOSurfaceRef ioSurface,
                                             uint32_t plane);

    class Texture final : public TextureBase {
      public:
        Texture(Device* device, const TextureDescriptor* descriptor);
        Texture(Device* device, const TextureDescriptor* descriptor, id<MTLTexture> mtlTexture);
        Texture(Device* device,
                const ExternalImageDescriptor* descriptor,
                IOSurfaceRef ioSurface,
                uint32_t plane);

        id<MTLTexture> GetMTLTexture();

        void EnsureSubresourceContentInitialized(const SubresourceRange& range);

      private:
        ~Texture() override;

        void DestroyImpl() override;

        MaybeError ClearTexture(const SubresourceRange& range, TextureBase::ClearValue clearValue);

        id<MTLTexture> mMtlTexture = nil;
    };

    class TextureView final : public TextureViewBase {
      public:
        TextureView(TextureBase* texture, const TextureViewDescriptor* descriptor);

        id<MTLTexture> GetMTLTexture();

      private:
        ~TextureView() override;

        id<MTLTexture> mMtlTextureView = nil;
    };

}}  // namespace dawn_native::metal

#endif  // DAWNNATIVE_METAL_TEXTUREMTL_H_
