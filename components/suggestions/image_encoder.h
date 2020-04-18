// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SUGGESTIONS_IMAGE_ENCODER_H_
#define COMPONENTS_SUGGESTIONS_IMAGE_ENCODER_H_

#include <stddef.h>

#include <memory>
#include <vector>

class SkBitmap;

namespace suggestions {

// From encoded bytes to SkBitmap.
std::unique_ptr<SkBitmap> DecodeJPEGToSkBitmap(const void* encoded_data,
                                               size_t size);

inline std::unique_ptr<SkBitmap> DecodeJPEGToSkBitmap(
    const std::vector<unsigned char>& encoded_data) {
  return DecodeJPEGToSkBitmap(&encoded_data[0], encoded_data.size());
}

// From SkBitmap to a vector of JPEG-encoded bytes, |dst|.
bool EncodeSkBitmapToJPEG(const SkBitmap& bitmap,
                          std::vector<unsigned char>* dest);

}  // namespace suggestions

#endif  // COMPONENTS_SUGGESTIONS_IMAGE_ENCODER_H_
