// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/paint/skia_paint_image_generator.h"

#include "cc/paint/paint_image_generator.h"

namespace cc {

SkiaPaintImageGenerator::SkiaPaintImageGenerator(
    sk_sp<PaintImageGenerator> paint_image_generator,
    size_t frame_index)
    : SkImageGenerator(paint_image_generator->GetSkImageInfo()),
      paint_image_generator_(std::move(paint_image_generator)),
      frame_index_(frame_index) {}

SkiaPaintImageGenerator::~SkiaPaintImageGenerator() = default;

sk_sp<SkData> SkiaPaintImageGenerator::onRefEncodedData() {
  return paint_image_generator_->GetEncodedData();
}

bool SkiaPaintImageGenerator::onGetPixels(const SkImageInfo& info,
                                          void* pixels,
                                          size_t row_bytes,
                                          const Options& options) {
  return paint_image_generator_->GetPixels(info, pixels, row_bytes,
                                           frame_index_, uniqueID());
}

bool SkiaPaintImageGenerator::onQueryYUV8(SkYUVSizeInfo* size_info,
                                          SkYUVColorSpace* color_space) const {
  return paint_image_generator_->QueryYUV8(size_info, color_space);
}

bool SkiaPaintImageGenerator::onGetYUV8Planes(const SkYUVSizeInfo& size_info,
                                              void* planes[3]) {
  return paint_image_generator_->GetYUV8Planes(size_info, planes, frame_index_,
                                               uniqueID());
}

}  // namespace cc
