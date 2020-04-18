// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_GRAPHICS_SKIA_TEXTURE_HOLDER_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_GRAPHICS_SKIA_TEXTURE_HOLDER_H_

#include "base/memory/weak_ptr.h"
#include "third_party/blink/renderer/platform/graphics/texture_holder.h"
#include "third_party/blink/renderer/platform/platform_export.h"

namespace blink {

class WebGraphicsContext3DProviderWrapper;

class PLATFORM_EXPORT SkiaTextureHolder final : public TextureHolder {
 public:
  ~SkiaTextureHolder() override;

  // Methods overriding TextureHolder
  bool IsSkiaTextureHolder() final { return true; }
  bool IsMailboxTextureHolder() final { return false; }
  IntSize Size() const final {
    return IntSize(image_->width(), image_->height());
  }
  bool IsValid() const final;
  bool CurrentFrameKnownToBeOpaque() final { return image_->isOpaque(); }
  sk_sp<SkImage> GetSkImage() final { return image_; }
  void Abandon() final;

  // When creating a AcceleratedStaticBitmap from a texture-backed SkImage, this
  // function will be called to create a TextureHolder object.
  SkiaTextureHolder(sk_sp<SkImage>,
                    base::WeakPtr<WebGraphicsContext3DProviderWrapper>&&);
  // This function consumes the mailbox in the input parameter and turn it into
  // a texture-backed SkImage.
  SkiaTextureHolder(std::unique_ptr<TextureHolder>);

 private:
  //  void ReleaseImageThreadSafe();

  // The m_image should always be texture-backed
  sk_sp<SkImage> image_;
  THREAD_CHECKER(thread_checker_);
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_GRAPHICS_SKIA_TEXTURE_HOLDER_H_
