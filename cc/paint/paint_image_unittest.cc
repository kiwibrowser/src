// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/paint/paint_image.h"

#include "base/test/gtest_util.h"
#include "cc/paint/paint_image_builder.h"
#include "cc/test/fake_paint_image_generator.h"
#include "cc/test/skia_common.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace cc {

TEST(PaintImageTest, Subsetting) {
  PaintImage image = CreateDiscardablePaintImage(gfx::Size(100, 100));
  EXPECT_EQ(image.width(), 100);
  EXPECT_EQ(image.height(), 100);

  PaintImage subset_rect_1 = PaintImageBuilder::WithCopy(image)
                                 .make_subset(gfx::Rect(25, 25, 50, 50))
                                 .TakePaintImage();
  EXPECT_EQ(subset_rect_1.width(), 50);
  EXPECT_EQ(subset_rect_1.height(), 50);
  EXPECT_EQ(subset_rect_1.subset_rect_, gfx::Rect(25, 25, 50, 50));

  PaintImage subset_rect_2 = PaintImageBuilder::WithCopy(subset_rect_1)
                                 .make_subset(gfx::Rect(25, 25, 25, 25))
                                 .TakePaintImage();
  EXPECT_EQ(subset_rect_2.width(), 25);
  EXPECT_EQ(subset_rect_2.height(), 25);
  EXPECT_EQ(subset_rect_2.subset_rect_, gfx::Rect(50, 50, 25, 25));

  EXPECT_EQ(image, PaintImageBuilder::WithCopy(image)
                       .make_subset(gfx::Rect(100, 100))
                       .TakePaintImage());
  EXPECT_DCHECK_DEATH(PaintImageBuilder::WithCopy(subset_rect_2)
                          .make_subset(gfx::Rect(10, 10, 25, 25))
                          .TakePaintImage());
  EXPECT_DCHECK_DEATH(PaintImageBuilder::WithCopy(image)
                          .make_subset(gfx::Rect())
                          .TakePaintImage());
}

TEST(PaintImageTest, DecodesCorrectFrames) {
  std::vector<FrameMetadata> frames = {
      FrameMetadata(true, base::TimeDelta::FromMilliseconds(2)),
      FrameMetadata(true, base::TimeDelta::FromMilliseconds(3))};
  sk_sp<FakePaintImageGenerator> generator =
      sk_make_sp<FakePaintImageGenerator>(SkImageInfo::MakeN32Premul(10, 10),
                                          frames);
  PaintImage image = PaintImageBuilder::WithDefault()
                         .set_id(PaintImage::GetNextId())
                         .set_paint_image_generator(generator)
                         .set_frame_index(0u)
                         .TakePaintImage();

  // The recorded index is 0u but ask for 1u frame.
  SkImageInfo info = SkImageInfo::MakeN32Premul(10, 10);
  std::vector<size_t> memory(info.computeMinByteSize());
  image.Decode(memory.data(), &info, nullptr, 1u);
  ASSERT_EQ(generator->frames_decoded().size(), 1u);
  EXPECT_EQ(generator->frames_decoded().count(1u), 1u);
  generator->reset_frames_decoded();

  // Subsetted.
  PaintImage subset_image = PaintImageBuilder::WithCopy(image)
                                .make_subset(gfx::Rect(0, 0, 5, 5))
                                .TakePaintImage();
  SkImageInfo subset_info = info.makeWH(5, 5);
  subset_image.Decode(memory.data(), &subset_info, nullptr, 1u);
  ASSERT_EQ(generator->frames_decoded().size(), 1u);
  EXPECT_EQ(generator->frames_decoded().count(1u), 1u);
  generator->reset_frames_decoded();

  // Not N32 color type.
  info.makeColorType(kRGB_565_SkColorType);
  memory = std::vector<size_t>(info.computeMinByteSize());
  image.Decode(memory.data(), &info, nullptr, 1u);
  ASSERT_EQ(generator->frames_decoded().size(), 1u);
  EXPECT_EQ(generator->frames_decoded().count(1u), 1u);
  generator->reset_frames_decoded();
}

}  // namespace cc
