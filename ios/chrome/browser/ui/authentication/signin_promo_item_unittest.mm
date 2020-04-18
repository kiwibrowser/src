// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/authentication/signin_promo_item.h"

#import "ios/chrome/browser/ui/authentication/signin_promo_view.h"
#import "ios/chrome/browser/ui/authentication/signin_promo_view_configurator.h"
#import "testing/gtest_mac.h"
#include "testing/platform_test.h"
#import "third_party/ocmock/OCMock/OCMock.h"
#include "third_party/ocmock/gtest_support.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

using SigninPromoItemTest = PlatformTest;

// Tests that SigninPromoItem creates and configures correctly SigninPromoCell.
TEST_F(SigninPromoItemTest, ConfigureCell) {
  SigninPromoViewConfigurator* configurator =
      OCMStrictClassMock([SigninPromoViewConfigurator class]);
  SigninPromoItem* item = [[SigninPromoItem alloc] initWithType:0];
  item.configurator = configurator;

  id cell = [[[item cellClass] alloc] init];
  ASSERT_TRUE([cell isMemberOfClass:[SigninPromoCell class]]);

  SigninPromoCell* signInCell = cell;
  EXPECT_NE(nil, signInCell.signinPromoView);
  EXPECT_TRUE(
      [signInCell.signinPromoView isMemberOfClass:[SigninPromoView class]]);
  SigninPromoView* signinPromoView = signInCell.signinPromoView;
  OCMExpect([configurator configureSigninPromoView:signinPromoView]);
  [item configureCell:signInCell];
  EXPECT_NE(nil, signinPromoView.textLabel.text);
  EXPECT_OCMOCK_VERIFY((id)configurator);
}
