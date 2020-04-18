// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/suggestions/image_encoder.h"

#include "ui/gfx/codec/jpeg_codec.h"
#include "ui/gfx/image/image_skia.h"

namespace suggestions {

std::unique_ptr<SkBitmap> DecodeJPEGToSkBitmap(const void* encoded_data,
                                               size_t size) {
  return gfx::JPEGCodec::Decode(static_cast<const unsigned char*>(encoded_data),
                                size);
}

bool EncodeSkBitmapToJPEG(const SkBitmap& bitmap,
                          std::vector<unsigned char>* dest) {
  if (!bitmap.readyToDraw() || bitmap.isNull()) {
    return false;
  }

  return gfx::JPEGCodec::Encode(bitmap, 100, dest);
}

}  // namespace suggestions
