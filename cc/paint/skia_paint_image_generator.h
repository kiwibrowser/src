// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_PAINT_SKIA_PAINT_IMAGE_GENERATOR_H_
#define CC_PAINT_SKIA_PAINT_IMAGE_GENERATOR_H_

#include "base/macros.h"
#include "cc/paint/paint_export.h"
#include "third_party/skia/include/core/SkImageGenerator.h"

namespace cc {
class PaintImageGenerator;

class CC_PAINT_EXPORT SkiaPaintImageGenerator final : public SkImageGenerator {
 public:
  SkiaPaintImageGenerator(sk_sp<PaintImageGenerator> paint_image_generator,
                          size_t frame_index);
  ~SkiaPaintImageGenerator() override;

  sk_sp<SkData> onRefEncodedData() override;
  bool onGetPixels(const SkImageInfo&,
                   void* pixels,
                   size_t row_bytes,
                   const Options& options) override;
  bool onQueryYUV8(SkYUVSizeInfo* size_info,
                   SkYUVColorSpace* color_space) const override;
  bool onGetYUV8Planes(const SkYUVSizeInfo& size_info,
                       void* planes[3]) override;

 private:
  sk_sp<PaintImageGenerator> paint_image_generator_;
  const size_t frame_index_;

  DISALLOW_COPY_AND_ASSIGN(SkiaPaintImageGenerator);
};

}  // namespace cc

#endif  // CC_PAINT_SKIA_PAINT_IMAGE_GENERATOR_H_
