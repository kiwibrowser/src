// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/web/public/web_state/page_display_state.h"

#include "testing/gtest/include/gtest/gtest.h"

#define EXPECT_NAN(value) EXPECT_NE(value, value)
#include "testing/platform_test.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

using PageDisplayStateTest = PlatformTest;

// Tests that the empty constructor creates an invalid PageDisplayState with all
// NAN values.
TEST_F(PageDisplayStateTest, EmptyConstructor) {
  web::PageDisplayState state;
  EXPECT_NAN(state.scroll_state().offset_x());
  EXPECT_NAN(state.scroll_state().offset_y());
  EXPECT_NAN(state.zoom_state().minimum_zoom_scale());
  EXPECT_NAN(state.zoom_state().maximum_zoom_scale());
  EXPECT_NAN(state.zoom_state().zoom_scale());
  EXPECT_FALSE(state.IsValid());
}

// Tests that the constructor with input states correctly populates the display
// state.
TEST_F(PageDisplayStateTest, StatesConstructor) {
  web::PageScrollState scroll_state(0.0, 1.0);
  EXPECT_EQ(0.0, scroll_state.offset_x());
  EXPECT_EQ(1.0, scroll_state.offset_y());
  EXPECT_TRUE(scroll_state.IsValid());
  web::PageZoomState zoom_state(1.0, 5.0, 1.0);
  EXPECT_EQ(1.0, zoom_state.minimum_zoom_scale());
  EXPECT_EQ(5.0, zoom_state.maximum_zoom_scale());
  EXPECT_EQ(1.0, zoom_state.zoom_scale());
  EXPECT_TRUE(zoom_state.IsValid());
  web::PageDisplayState state(scroll_state, zoom_state);
  EXPECT_EQ(scroll_state.offset_x(), state.scroll_state().offset_x());
  EXPECT_EQ(scroll_state.offset_y(), state.scroll_state().offset_y());
  EXPECT_EQ(zoom_state.minimum_zoom_scale(),
            state.zoom_state().minimum_zoom_scale());
  EXPECT_EQ(zoom_state.maximum_zoom_scale(),
            state.zoom_state().maximum_zoom_scale());
  EXPECT_EQ(zoom_state.zoom_scale(), state.zoom_state().zoom_scale());
  EXPECT_TRUE(state.IsValid());
}

// Tests the constructor with value inputs.
TEST_F(PageDisplayStateTest, ValuesConstructor) {
  web::PageDisplayState state(0.0, 1.0, 1.0, 5.0, 1.0);
  EXPECT_EQ(0.0, state.scroll_state().offset_x());
  EXPECT_EQ(1.0, state.scroll_state().offset_y());
  EXPECT_EQ(1.0, state.zoom_state().minimum_zoom_scale());
  EXPECT_EQ(5.0, state.zoom_state().maximum_zoom_scale());
  EXPECT_EQ(1.0, state.zoom_state().zoom_scale());
  EXPECT_TRUE(state.IsValid());
}

// Tests converting between a PageDisplayState, its serialization, and back.
TEST_F(PageDisplayStateTest, Serialization) {
  web::PageDisplayState state(0.0, 1.0, 1.0, 5.0, 1.0);
  web::PageDisplayState new_state(state.GetSerialization());
  EXPECT_EQ(state, new_state);
}
