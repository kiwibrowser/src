// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/android/disambiguation_popup_helper.h"

#include <stddef.h>

#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/platform/web_rect.h"
#include "third_party/blink/public/platform/web_vector.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/size.h"
#include "ui/gfx/geometry/size_conversions.h"

// these constants are copied from the implementation class
namespace {
const float kDisambiguationPopupMaxScale = 5.0;
const float kDisambiguationPopupMinScale = 2.0;
}  // unnamed namespace

namespace content {

class DisambiguationPopupHelperUnittest : public testing::Test {
 public:
  DisambiguationPopupHelperUnittest()
      : kScreenSize_(640, 480)
      , kVisibleContentSize_(640, 480)
      , kImplScale_(1) { }
 protected:
  const gfx::Size kScreenSize_;
  const gfx::Size kVisibleContentSize_;
  const float kImplScale_;
};

TEST_F(DisambiguationPopupHelperUnittest, ClipByViewport) {
  gfx::Rect tap_rect(1000, 1000, 10, 10);
  blink::WebVector<blink::WebRect> target_rects(static_cast<size_t>(1));
  target_rects[0] = gfx::Rect(-20, -20, 10, 10);

  gfx::Rect zoom_rect;
  float scale = DisambiguationPopupHelper::ComputeZoomAreaAndScaleFactor(
      tap_rect, target_rects, kScreenSize_, kVisibleContentSize_, kImplScale_,
      &zoom_rect);

  EXPECT_TRUE(gfx::Rect(kVisibleContentSize_).Contains(zoom_rect));
  EXPECT_LE(kDisambiguationPopupMinScale, scale);

  gfx::Size scaled_size = gfx::ScaleToCeiledSize(zoom_rect.size(), scale);
  EXPECT_TRUE(gfx::Rect(kScreenSize_).Contains(gfx::Rect(scaled_size)));
}

TEST_F(DisambiguationPopupHelperUnittest, MiniTarget) {
  gfx::Rect tap_rect(-5, -5, 20, 20);
  blink::WebVector<blink::WebRect> target_rects(static_cast<size_t>(1));
  target_rects[0] = gfx::Rect(10, 10, 1, 1);

  gfx::Rect zoom_rect;
  float scale = DisambiguationPopupHelper::ComputeZoomAreaAndScaleFactor(
      tap_rect, target_rects, kScreenSize_, kVisibleContentSize_, kImplScale_,
      &zoom_rect);

  EXPECT_TRUE(gfx::Rect(kVisibleContentSize_).Contains(zoom_rect));
  EXPECT_EQ(kDisambiguationPopupMaxScale, scale);
  EXPECT_TRUE(zoom_rect.Contains(target_rects[0]));

  gfx::Size scaled_size = gfx::ScaleToCeiledSize(zoom_rect.size(), scale);
  EXPECT_TRUE(gfx::Rect(kScreenSize_).Contains(gfx::Rect(scaled_size)));
}

TEST_F(DisambiguationPopupHelperUnittest, LongLinks) {
  gfx::Rect tap_rect(10, 10, 20, 20);
  blink::WebVector<blink::WebRect> target_rects(static_cast<size_t>(2));
  target_rects[0] = gfx::Rect(15, 15, 1000, 5);
  target_rects[1] = gfx::Rect(15, 25, 1000, 5);

  gfx::Rect zoom_rect;
  float scale = DisambiguationPopupHelper::ComputeZoomAreaAndScaleFactor(
      tap_rect, target_rects, kScreenSize_, kVisibleContentSize_, kImplScale_,
      &zoom_rect);

  EXPECT_TRUE(gfx::Rect(kVisibleContentSize_).Contains(zoom_rect));
  EXPECT_EQ(kDisambiguationPopupMaxScale, scale);
  EXPECT_TRUE(zoom_rect.Contains(tap_rect));

  gfx::Size scaled_size = gfx::ScaleToCeiledSize(zoom_rect.size(), scale);
  EXPECT_TRUE(gfx::Rect(kScreenSize_).Contains(gfx::Rect(scaled_size)));
}

}  // namespace content
