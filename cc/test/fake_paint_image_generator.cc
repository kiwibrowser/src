// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <cc/test/fake_paint_image_generator.h>

namespace cc {

FakePaintImageGenerator::FakePaintImageGenerator(
    const SkImageInfo& info,
    std::vector<FrameMetadata> frames,
    bool allocate_discardable_memory)
    : PaintImageGenerator(info, std::move(frames)),
      image_backing_memory_(
          allocate_discardable_memory ? info.computeMinByteSize() : 0,
          0),
      image_pixmap_(info, image_backing_memory_.data(), info.minRowBytes()) {}

FakePaintImageGenerator::~FakePaintImageGenerator() = default;

sk_sp<SkData> FakePaintImageGenerator::GetEncodedData() const {
  return nullptr;
}

bool FakePaintImageGenerator::GetPixels(const SkImageInfo& info,
                                        void* pixels,
                                        size_t row_bytes,
                                        size_t frame_index,
                                        uint32_t lazy_pixel_ref) {
  if (image_backing_memory_.empty())
    return false;
  frames_decoded_.insert(frame_index);
  CHECK(image_pixmap_.readPixels(info, pixels, row_bytes, 0, 0));
  return true;
}

bool FakePaintImageGenerator::QueryYUV8(SkYUVSizeInfo* info,
                                        SkYUVColorSpace* color_space) const {
  return false;
}

bool FakePaintImageGenerator::GetYUV8Planes(const SkYUVSizeInfo& info,
                                            void* planes[3],
                                            size_t frame_index,
                                            uint32_t lazy_pixel_ref) {
  NOTREACHED();
  return false;
}

}  // namespace cc
