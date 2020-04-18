// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/payments/cells/accessibility_util.h"

#include "testing/platform_test.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

class PaymentRequestAccessibilityLabelBuilderTest : public PlatformTest {
 protected:
  void SetUp() override { builder_ = [[AccessibilityLabelBuilder alloc] init]; }

  void Expect(NSString* result) {
    EXPECT_TRUE([[builder_ buildAccessibilityLabel] isEqualToString:result]);
  }

  AccessibilityLabelBuilder* builder_ = nil;
};

TEST_F(PaymentRequestAccessibilityLabelBuilderTest, NoItems) {
  Expect(@"");
}

TEST_F(PaymentRequestAccessibilityLabelBuilderTest, OneItem) {
  [builder_ appendItem:@"test"];
  Expect(@"test");
}

TEST_F(PaymentRequestAccessibilityLabelBuilderTest, MultipleItems) {
  [builder_ appendItem:@"test"];
  [builder_ appendItem:@"test2"];
  [builder_ appendItem:@"test3"];
  Expect(@"test, test2, test3");
}

TEST_F(PaymentRequestAccessibilityLabelBuilderTest, NilItems) {
  [builder_ appendItem:@"test"];
  [builder_ appendItem:nil];
  [builder_ appendItem:nil];
  [builder_ appendItem:@"test2"];
  Expect(@"test, test2");
}

TEST_F(PaymentRequestAccessibilityLabelBuilderTest, EndsWithNilItem) {
  [builder_ appendItem:@"test"];
  [builder_ appendItem:@"test2"];
  [builder_ appendItem:nil];
  Expect(@"test, test2");
}

TEST_F(PaymentRequestAccessibilityLabelBuilderTest, StartsWithNilItem) {
  [builder_ appendItem:nil];
  [builder_ appendItem:@"test"];
  [builder_ appendItem:@"test2"];
  Expect(@"test, test2");
}

}  // namespace
