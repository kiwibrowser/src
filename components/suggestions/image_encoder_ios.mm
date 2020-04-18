// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/suggestions/image_encoder.h"

#include <stddef.h>
#import <UIKit/UIKit.h>

#include <memory>

#include "base/mac/scoped_cftyperef.h"
#include "skia/ext/skia_utils_ios.h"

namespace suggestions {

std::unique_ptr<SkBitmap> DecodeJPEGToSkBitmap(const void* encoded_data,
                                               size_t size) {
  NSData* data = [NSData dataWithBytes:encoded_data length:size];
  UIImage* image = [UIImage imageWithData:data scale:1.0];
  return std::make_unique<SkBitmap>(
      skia::CGImageToSkBitmap(image.CGImage, [image size], YES));
}

bool EncodeSkBitmapToJPEG(const SkBitmap& bitmap,
                          std::vector<unsigned char>* dest) {
  base::ScopedCFTypeRef<CGColorSpaceRef> color_space(
      CGColorSpaceCreateDeviceRGB());
  UIImage* image =
      skia::SkBitmapToUIImageWithColorSpace(bitmap, 1 /* scale */, color_space);
  NSData* data = UIImageJPEGRepresentation(image, 1.0);
  const char* bytes = reinterpret_cast<const char*>([data bytes]);
  dest->assign(bytes, bytes + [data length]);
  return true;
}

}  // namespace suggestions
