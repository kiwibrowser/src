// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/resources/ui_resource_bitmap.h"

#include <stdint.h>

#include <memory>

#include "base/logging.h"
#include "third_party/skia/include/core/SkImageInfo.h"
#include "third_party/skia/include/core/SkMallocPixelRef.h"
#include "third_party/skia/include/core/SkPixelRef.h"

namespace cc {
namespace {

UIResourceBitmap::UIResourceFormat SkColorTypeToUIResourceFormat(
    SkColorType sk_type) {
  UIResourceBitmap::UIResourceFormat format = UIResourceBitmap::RGBA8;
  switch (sk_type) {
    case kN32_SkColorType:
      format = UIResourceBitmap::RGBA8;
      break;
    case kAlpha_8_SkColorType:
      format = UIResourceBitmap::ALPHA_8;
      break;
    default:
      NOTREACHED() << "Invalid SkColorType for UIResourceBitmap: " << sk_type;
      break;
  }
  return format;
}

}  // namespace

void UIResourceBitmap::Create(sk_sp<SkPixelRef> pixel_ref,
                              const SkImageInfo& info,
                              UIResourceFormat format) {
  DCHECK(info.width());
  DCHECK(info.height());
  DCHECK(pixel_ref);
  DCHECK(pixel_ref->isImmutable());
  format_ = format;
  info_ = info;
  pixel_ref_ = std::move(pixel_ref);
}

void UIResourceBitmap::DrawToCanvas(SkCanvas* canvas, SkPaint* paint) {
  SkBitmap bitmap;
  bitmap.setInfo(info_, pixel_ref_.get()->rowBytes());
  bitmap.setPixelRef(pixel_ref_, 0, 0);
  canvas->drawBitmap(bitmap, 0, 0, paint);
  canvas->flush();
}

UIResourceBitmap::UIResourceBitmap(const SkBitmap& skbitmap) {
  DCHECK_EQ(skbitmap.width(), skbitmap.rowBytesAsPixels());
  DCHECK(skbitmap.isImmutable());

  sk_sp<SkPixelRef> pixel_ref = sk_ref_sp(skbitmap.pixelRef());
  Create(std::move(pixel_ref), skbitmap.info(),
         SkColorTypeToUIResourceFormat(skbitmap.colorType()));
}

UIResourceBitmap::UIResourceBitmap(const gfx::Size& size, bool is_opaque) {
  SkAlphaType alphaType = is_opaque ? kOpaque_SkAlphaType : kPremul_SkAlphaType;
  SkImageInfo info =
      SkImageInfo::MakeN32(size.width(), size.height(), alphaType);
  sk_sp<SkPixelRef> pixel_ref(
      SkMallocPixelRef::MakeAllocate(info, info.minRowBytes()));
  pixel_ref->setImmutable();
  Create(std::move(pixel_ref), info, UIResourceBitmap::RGBA8);
}

UIResourceBitmap::UIResourceBitmap(sk_sp<SkPixelRef> pixel_ref,
                                   const gfx::Size& size) {
  SkImageInfo info =
      SkImageInfo::MakeN32(size.width(), size.height(), kOpaque_SkAlphaType);
  Create(std::move(pixel_ref), info, UIResourceBitmap::ETC1);
}

UIResourceBitmap::UIResourceBitmap(const UIResourceBitmap& other) = default;

UIResourceBitmap::~UIResourceBitmap() = default;
}  // namespace cc
