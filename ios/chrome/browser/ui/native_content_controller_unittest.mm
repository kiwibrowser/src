// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/native_content_controller.h"
#include "base/logging.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/platform_test.h"
#include "url/gurl.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

using NativeContentControllerTest = PlatformTest;

TEST_F(NativeContentControllerTest, TestInitWithURL) {
  GURL url("http://foo.bar.com");
  NativeContentController* controller =
      [[NativeContentController alloc] initWithURL:url];
  EXPECT_EQ(url, controller.url);

  // There is no default view without a nib.
  EXPECT_EQ(nil, controller.view);
}

TEST_F(NativeContentControllerTest, TestInitWithEmptyNibNameAndURL) {
  GURL url("http://foo.bar.com");
  NativeContentController* controller =
      [[NativeContentController alloc] initWithNibName:nil url:url];
  EXPECT_EQ(url, controller.url);

  // There is no default view without a nib.
  EXPECT_EQ(nil, controller.view);
}

TEST_F(NativeContentControllerTest, TestInitWithNibAndURL) {
  GURL url("http://foo.bar.com");
  NSString* nibName = @"native_content_controller_test";
  NativeContentController* controller =
      [[NativeContentController alloc] initWithNibName:nibName url:url];
  EXPECT_EQ(url, controller.url);

  // Check that view is loaded from nib.
  EXPECT_NE(nil, controller.view);
  UIView* view = controller.view;
  EXPECT_EQ(1u, view.subviews.count);
  EXPECT_NE(nil, [view.subviews firstObject]);
  UILabel* label = [view.subviews firstObject];
  EXPECT_TRUE([label isKindOfClass:[UILabel class]]);
  EXPECT_TRUE([label.text isEqualToString:@"test pass"]);
}

}  // namespace
