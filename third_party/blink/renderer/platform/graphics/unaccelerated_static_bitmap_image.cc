// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/graphics/unaccelerated_static_bitmap_image.h"

#include "components/viz/common/gpu/context_provider.h"
#include "third_party/blink/public/platform/web_graphics_context_3d_provider.h"
#include "third_party/blink/renderer/platform/graphics/accelerated_static_bitmap_image.h"
#include "third_party/blink/renderer/platform/graphics/web_graphics_context_3d_provider_wrapper.h"
#include "third_party/skia/include/core/SkImage.h"

namespace blink {

scoped_refptr<UnacceleratedStaticBitmapImage>
UnacceleratedStaticBitmapImage::Create(sk_sp<SkImage> image) {
  DCHECK(!image->isTextureBacked());
  return base::AdoptRef(new UnacceleratedStaticBitmapImage(std::move(image)));
}

UnacceleratedStaticBitmapImage::UnacceleratedStaticBitmapImage(
    sk_sp<SkImage> image) {
  CHECK(image);
  DCHECK(!image->isLazyGenerated());

  paint_image_ =
      CreatePaintImageBuilder()
          .set_image(std::move(image), cc::PaintImage::GetNextContentId())
          .TakePaintImage();
}

scoped_refptr<UnacceleratedStaticBitmapImage>
UnacceleratedStaticBitmapImage::Create(PaintImage image) {
  return base::AdoptRef(new UnacceleratedStaticBitmapImage(std::move(image)));
}

UnacceleratedStaticBitmapImage::UnacceleratedStaticBitmapImage(PaintImage image)
    : paint_image_(std::move(image)) {
  CHECK(paint_image_.GetSkImage());
}

UnacceleratedStaticBitmapImage::~UnacceleratedStaticBitmapImage() = default;

IntSize UnacceleratedStaticBitmapImage::Size() const {
  return IntSize(paint_image_.width(), paint_image_.height());
}

bool UnacceleratedStaticBitmapImage::IsPremultiplied() const {
  return paint_image_.GetSkImage()->alphaType() ==
         SkAlphaType::kPremul_SkAlphaType;
}

scoped_refptr<StaticBitmapImage>
UnacceleratedStaticBitmapImage::MakeAccelerated(
    base::WeakPtr<WebGraphicsContext3DProviderWrapper> context_wrapper) {
  if (!context_wrapper)
    return nullptr;  // Can happen if the context is lost.

  GrContext* grcontext = context_wrapper->ContextProvider()->GetGrContext();
  if (!grcontext)
    return nullptr;  // Can happen if the context is lost.

  sk_sp<SkImage> sk_image = paint_image_.GetSkImage();
  sk_sp<SkImage> gpu_skimage =
      sk_image->makeTextureImage(grcontext, sk_image->colorSpace());
  if (!gpu_skimage)
    return nullptr;

  return AcceleratedStaticBitmapImage::CreateFromSkImage(
      std::move(gpu_skimage), std::move(context_wrapper));
}

bool UnacceleratedStaticBitmapImage::CurrentFrameKnownToBeOpaque() {
  return paint_image_.GetSkImage()->isOpaque();
}

void UnacceleratedStaticBitmapImage::Draw(PaintCanvas* canvas,
                                          const PaintFlags& flags,
                                          const FloatRect& dst_rect,
                                          const FloatRect& src_rect,
                                          RespectImageOrientationEnum,
                                          ImageClampingMode clamp_mode,
                                          ImageDecodingMode) {
  StaticBitmapImage::DrawHelper(canvas, flags, dst_rect, src_rect, clamp_mode,
                                PaintImageForCurrentFrame());
}

PaintImage UnacceleratedStaticBitmapImage::PaintImageForCurrentFrame() {
  return paint_image_;
}

}  // namespace blink
