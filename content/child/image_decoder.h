// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_CHILD_IMAGE_DECODER_H_
#define CONTENT_CHILD_IMAGE_DECODER_H_

#include <stddef.h>

#include <vector>

#include "base/macros.h"
#include "ui/gfx/geometry/size.h"

class SkBitmap;

namespace content {

// Provides an interface to WebKit's image decoders.
//
// Note to future: This class should be deleted. We should have our own nice
// image decoders in base/gfx, and our port should use those. Currently, it's
// the other way around.
class ImageDecoder {
 public:
  // Use the constructor with desired_size when you think you may have an .ico
  // format and care about which size you get back. Otherwise, use the 0-arg
  // constructor.
  ImageDecoder();
  ImageDecoder(const gfx::Size& desired_icon_size);
  ~ImageDecoder();

  // Call this function to decode the image. If successful, the decoded image
  // will be returned. Otherwise, an empty bitmap will be returned.
  SkBitmap Decode(const unsigned char* data, size_t size) const;

  // Returns all frames found in the image represented by data. If there are
  // multiple frames at the same size, only the first one is returned.
  static std::vector<SkBitmap> DecodeAll(
      const unsigned char* data, size_t size);

 private:
  // Size will be empty to get the largest possible size.
  gfx::Size desired_icon_size_;

  DISALLOW_COPY_AND_ASSIGN(ImageDecoder);
};

}  // namespace content

#endif  // CONTENT_CHILD_IMAGE_DECODER_H_
