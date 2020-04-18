// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/paint/decode_stashing_image_provider.h"

namespace cc {
DecodeStashingImageProvider::DecodeStashingImageProvider(
    ImageProvider* source_provider)
    : source_provider_(source_provider) {
  DCHECK(source_provider_);
}
DecodeStashingImageProvider::~DecodeStashingImageProvider() = default;

ImageProvider::ScopedDecodedDrawImage
DecodeStashingImageProvider::GetDecodedDrawImage(const DrawImage& draw_image) {
  auto decode = source_provider_->GetDecodedDrawImage(draw_image);
  if (!decode.needs_unlock())
    return decode;

  // No need to add any destruction callback to the returned image. The images
  // decoded here match the lifetime of this provider.
  auto image_to_return = ScopedDecodedDrawImage(decode.decoded_image());
  decoded_images_->push_back(std::move(decode));
  return image_to_return;
}

void DecodeStashingImageProvider::Reset() {
  decoded_images_->clear();
}

}  // namespace cc
