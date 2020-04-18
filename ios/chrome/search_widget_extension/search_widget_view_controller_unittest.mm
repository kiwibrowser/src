// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/search_widget_extension/search_widget_view_controller.h"

#import "ios/chrome/search_widget_extension/search_widget_view_controller.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/platform_test.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

using SearchWidgetViewControllerTest = PlatformTest;

TEST_F(SearchWidgetViewControllerTest, Alloc) {
  SearchWidgetViewController* controller =
      [[SearchWidgetViewController alloc] init];
  ASSERT_NE(controller, nil);
}
