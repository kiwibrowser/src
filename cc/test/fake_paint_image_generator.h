// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_TEST_FAKE_PAINT_IMAGE_GENERATOR_H_
#define CC_TEST_FAKE_PAINT_IMAGE_GENERATOR_H_

#include "base/containers/flat_set.h"
#include "cc/paint/paint_image_generator.h"

namespace cc {

class FakePaintImageGenerator : public PaintImageGenerator {
 public:
  explicit FakePaintImageGenerator(
      const SkImageInfo& info,
      std::vector<FrameMetadata> frames = {FrameMetadata()},
      bool allocate_discardable_memory = true);
  ~FakePaintImageGenerator() override;

  sk_sp<SkData> GetEncodedData() const override;
  bool GetPixels(const SkImageInfo& info,
                 void* pixels,
                 size_t row_bytes,
                 size_t frame_index,
                 uint32_t lazy_pixel_ref) override;
  bool QueryYUV8(SkYUVSizeInfo* info,
                 SkYUVColorSpace* color_space) const override;
  bool GetYUV8Planes(const SkYUVSizeInfo& info,
                     void* planes[3],
                     size_t frame_index,
                     uint32_t lazy_pixel_ref) override;

  const base::flat_set<size_t>& frames_decoded() const {
    return frames_decoded_;
  }
  void reset_frames_decoded() { frames_decoded_.clear(); }

 private:
  std::vector<uint8_t> image_backing_memory_;
  SkPixmap image_pixmap_;
  base::flat_set<size_t> frames_decoded_;
};

}  // namespace cc

#endif  // CC_TEST_FAKE_PAINT_IMAGE_GENERATOR_H_
