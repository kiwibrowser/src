// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/thumbnails/thumbnail_utils.h"

#include "chrome/browser/thumbnails/thumbnailing_context.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/notification_types.h"
#include "content/public/test/mock_render_process_host.h"
#include "content/public/test/test_renderer_host.h"
#include "skia/ext/platform_canvas.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/skia/include/core/SkColorPriv.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/geometry/size_conversions.h"
#include "ui/surface/transport_dib.h"

using content::WebContents;

typedef testing::Test SimpleThumbnailCropTest;

TEST_F(SimpleThumbnailCropTest, GetCanvasCopyInfo) {
  gfx::Size thumbnail_size(200, 120);
  gfx::Size expected_2x_size = gfx::ScaleToFlooredSize(thumbnail_size, 2.0);
  float desired_aspect =
      static_cast<float>(thumbnail_size.width()) / thumbnail_size.height();
  gfx::Rect clipping_rect_result;
  gfx::Size target_size_result;

  thumbnails::ClipResult clip_result = thumbnails::GetCanvasCopyInfo(
      gfx::Size(400, 210), ui::SCALE_FACTOR_200P, thumbnail_size,
      &clipping_rect_result, &target_size_result);
  gfx::Size clipping_size = clipping_rect_result.size();
  float clip_aspect =
      static_cast<float>(clipping_size.width()) / clipping_size.height();
  EXPECT_EQ(thumbnails::CLIP_RESULT_WIDER_THAN_TALL, clip_result);
  EXPECT_EQ(expected_2x_size, target_size_result);
  EXPECT_NEAR(desired_aspect, clip_aspect, 0.01);

  clip_result = thumbnails::GetCanvasCopyInfo(
      gfx::Size(600, 200), ui::SCALE_FACTOR_200P, thumbnail_size,
      &clipping_rect_result, &target_size_result);
  clipping_size = clipping_rect_result.size();
  clip_aspect =
      static_cast<float>(clipping_size.width()) / clipping_size.height();
  EXPECT_EQ(thumbnails::CLIP_RESULT_MUCH_WIDER_THAN_TALL, clip_result);
  EXPECT_EQ(expected_2x_size, target_size_result);
  EXPECT_NEAR(desired_aspect, clip_aspect, 0.01);

  clip_result = thumbnails::GetCanvasCopyInfo(
      gfx::Size(300, 600), ui::SCALE_FACTOR_200P, thumbnail_size,
      &clipping_rect_result, &target_size_result);
  clipping_size = clipping_rect_result.size();
  clip_aspect =
      static_cast<float>(clipping_size.width()) / clipping_size.height();
  EXPECT_EQ(thumbnails::CLIP_RESULT_TALLER_THAN_WIDE, clip_result);
  EXPECT_EQ(expected_2x_size, target_size_result);
  EXPECT_NEAR(desired_aspect, clip_aspect, 0.01);

  clip_result = thumbnails::GetCanvasCopyInfo(
      gfx::Size(200, 100), ui::SCALE_FACTOR_200P, thumbnail_size,
      &clipping_rect_result, &target_size_result);
  EXPECT_EQ(thumbnails::CLIP_RESULT_SOURCE_IS_SMALLER, clip_result);
  EXPECT_EQ(expected_2x_size, target_size_result);
}

TEST_F(SimpleThumbnailCropTest, GetCanvasCopyInfoDifferentScales) {
  gfx::Size thumbnail_size(200, 120);

  gfx::Rect clipping_rect_result;
  gfx::Size target_size_result;

  gfx::Size expected_2x_size = gfx::ScaleToFlooredSize(thumbnail_size, 2.0);

  // Test at 1x scale. Expect a 2x thumbnail (we do this for quality).
  thumbnails::GetCanvasCopyInfo(gfx::Size(400, 210), ui::SCALE_FACTOR_100P,
                                thumbnail_size, &clipping_rect_result,
                                &target_size_result);
  EXPECT_EQ(expected_2x_size, target_size_result);

  // Test at 1.5x scale.
  gfx::Size expected_15x_size = gfx::ScaleToFlooredSize(thumbnail_size, 1.5);
  thumbnails::GetCanvasCopyInfo(gfx::Size(400, 210), ui::SCALE_FACTOR_150P,
                                thumbnail_size, &clipping_rect_result,
                                &target_size_result);
  EXPECT_EQ(expected_15x_size, target_size_result);

  // Test at 2x scale.
  thumbnails::GetCanvasCopyInfo(gfx::Size(400, 210), ui::SCALE_FACTOR_200P,
                                thumbnail_size, &clipping_rect_result,
                                &target_size_result);
  EXPECT_EQ(expected_2x_size, target_size_result);

  // Test at 3x scale. Expect a 2x (!) thumbnail (see crbug.com/670488).
  thumbnails::GetCanvasCopyInfo(gfx::Size(400, 210), ui::SCALE_FACTOR_300P,
                                thumbnail_size, &clipping_rect_result,
                                &target_size_result);
  EXPECT_EQ(expected_2x_size, target_size_result);
}

TEST_F(SimpleThumbnailCropTest, GetClippingRect) {
  const gfx::Size desired_size(300, 200);
  thumbnails::ClipResult clip_result;
  // Try out 'microsource'.
  gfx::Rect clip_rect = thumbnails::GetClippingRect(gfx::Size(300, 199),
                                                    desired_size, &clip_result);
  EXPECT_EQ(thumbnails::CLIP_RESULT_SOURCE_IS_SMALLER, clip_result);
  EXPECT_EQ(gfx::Point(0, 0).ToString(), clip_rect.origin().ToString());
  EXPECT_EQ(desired_size.ToString(), clip_rect.size().ToString());

  // Portrait source.
  clip_rect = thumbnails::GetClippingRect(gfx::Size(500, 1200), desired_size,
                                          &clip_result);
  EXPECT_EQ(thumbnails::CLIP_RESULT_TALLER_THAN_WIDE, clip_result);
  EXPECT_EQ(gfx::Point(0, 0).ToString(), clip_rect.origin().ToString());
  EXPECT_EQ(500, clip_rect.width());
  EXPECT_GE(1200, clip_rect.height());

  clip_rect = thumbnails::GetClippingRect(gfx::Size(2000, 800), desired_size,
                                          &clip_result);
  EXPECT_TRUE(clip_result == thumbnails::CLIP_RESULT_WIDER_THAN_TALL ||
              clip_result == thumbnails::CLIP_RESULT_MUCH_WIDER_THAN_TALL);
  EXPECT_EQ(0, clip_rect.y());
  EXPECT_LT(0, clip_rect.x());
  EXPECT_GE(2000, clip_rect.width());
  EXPECT_EQ(800, clip_rect.height());

  clip_rect = thumbnails::GetClippingRect(gfx::Size(900, 600), desired_size,
                                          &clip_result);
  EXPECT_EQ(thumbnails::CLIP_RESULT_NOT_CLIPPED, clip_result);
  EXPECT_EQ(gfx::Point(0, 0).ToString(), clip_rect.origin().ToString());
  EXPECT_EQ(gfx::Size(900, 600).ToString(), clip_rect.size().ToString());
}
