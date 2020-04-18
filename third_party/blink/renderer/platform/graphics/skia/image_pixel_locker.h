// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_GRAPHICS_SKIA_IMAGE_PIXEL_LOCKER_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_GRAPHICS_SKIA_IMAGE_PIXEL_LOCKER_H_

#include "base/memory/scoped_refptr.h"
#include "third_party/blink/renderer/platform/heap/heap.h"
#include "third_party/blink/renderer/platform/wtf/allocator.h"
#include "third_party/blink/renderer/platform/wtf/noncopyable.h"
#include "third_party/skia/include/core/SkImageInfo.h"
#include "third_party/skia/include/core/SkRefCnt.h"

class SkImage;

namespace blink {

class ImagePixelLocker final {
  DISALLOW_NEW_EXCEPT_PLACEMENT_NEW();
  WTF_MAKE_NONCOPYABLE(ImagePixelLocker);

 public:
  ImagePixelLocker(sk_sp<const SkImage>, SkAlphaType, SkColorType);

  const void* Pixels() const { return pixels_; }

 private:
  const sk_sp<const SkImage> image_;
  const void* pixels_;
  Vector<char> pixel_storage_;
};

}  // namespace blink

#endif
