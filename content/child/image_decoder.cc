// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/child/image_decoder.h"

#include "content/public/child/image_decoder_utils.h"
#include "third_party/blink/public/platform/web_data.h"
#include "third_party/blink/public/platform/web_image.h"
#include "third_party/blink/public/platform/web_size.h"
#include "third_party/skia/include/core/SkBitmap.h"

using blink::WebData;
using blink::WebImage;

namespace content {

SkBitmap DecodeImage(const unsigned char* data,
                     const gfx::Size& desired_image_size,
                     size_t size) {
  ImageDecoder decoder(desired_image_size);
  return decoder.Decode(data, size);
}

ImageDecoder::ImageDecoder() : desired_icon_size_(0, 0) {
}

ImageDecoder::ImageDecoder(const gfx::Size& desired_icon_size)
    : desired_icon_size_(desired_icon_size) {
}

ImageDecoder::~ImageDecoder() {
}

SkBitmap ImageDecoder::Decode(const unsigned char* data, size_t size) const {
  const WebImage& image = WebImage::FromData(
      WebData(reinterpret_cast<const char*>(data), size), desired_icon_size_);
  return image.GetSkBitmap();
}

// static
std::vector<SkBitmap> ImageDecoder::DecodeAll(
      const unsigned char* data, size_t size) {
  const blink::WebVector<WebImage>& images = WebImage::FramesFromData(
      WebData(reinterpret_cast<const char*>(data), size));
  std::vector<SkBitmap> result;
  for (size_t i = 0; i < images.size(); ++i)
    result.push_back(images[i].GetSkBitmap());
  return result;
}

}  // namespace content
