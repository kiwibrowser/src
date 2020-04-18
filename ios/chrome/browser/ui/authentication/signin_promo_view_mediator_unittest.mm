// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/authentication/signin_promo_view_mediator.h"

#include "base/run_loop.h"
#import "base/test/ios/wait_util.h"
#import "ios/chrome/browser/ui/authentication/signin_promo_view.h"
#import "ios/chrome/browser/ui/authentication/signin_promo_view_configurator.h"
#import "ios/chrome/browser/ui/authentication/signin_promo_view_consumer.h"
#import "ios/public/provider/chrome/browser/signin/fake_chrome_identity.h"
#import "ios/public/provider/chrome/browser/signin/fake_chrome_identity_service.h"
#import "ios/third_party/material_components_ios/src/components/Buttons/src/MaterialButtons.h"
#import "testing/platform_test.h"
#import "third_party/ocmock/OCMock/OCMock.h"
#include "third_party/ocmock/gtest_support.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

class SigninPromoViewMediatorTest : public PlatformTest {
 protected:
  void TearDown() override {
    [mediator_ signinPromoViewRemoved];
    EXPECT_EQ(ios::SigninPromoViewState::Invalid,
              mediator_.signinPromoViewState);
    mediator_ = nil;
    EXPECT_OCMOCK_VERIFY((id)consumer_);
    EXPECT_OCMOCK_VERIFY((id)signin_promo_view_);
    EXPECT_OCMOCK_VERIFY((id)primary_button_);
    EXPECT_OCMOCK_VERIFY((id)secondary_button_);
    EXPECT_OCMOCK_VERIFY((id)close_button_);
  }

  void CreateMediator(signin_metrics::AccessPoint accessPoint) {
    consumer_ = OCMStrictProtocolMock(@protocol(SigninPromoViewConsumer));
    mediator_ =
        [[SigninPromoViewMediator alloc] initWithBrowserState:nil
                                                  accessPoint:accessPoint
                                                    presenter:nil];
    mediator_.consumer = consumer_;

    signin_promo_view_ = OCMStrictClassMock([SigninPromoView class]);
    primary_button_ = OCMStrictClassMock([MDCFlatButton class]);
    OCMStub([signin_promo_view_ primaryButton]).andReturn(primary_button_);
    secondary_button_ = OCMStrictClassMock([MDCFlatButton class]);
    OCMStub([signin_promo_view_ secondaryButton]).andReturn(secondary_button_);
    close_button_ = OCMStrictClassMock([UIButton class]);
    OCMStub([signin_promo_view_ closeButton]).andReturn(close_button_);
  }

  void TestColdState() {
    EXPECT_EQ(nil, mediator_.defaultIdentity);
    CheckColdStateConfigurator([mediator_ createConfigurator]);
  }

  void TestWarmState() {
    expected_default_dentity_ =
        [FakeChromeIdentity identityWithEmail:@"johndoe@example.com"
                                       gaiaID:@"1"
                                         name:@"John Doe"];
    ios::FakeChromeIdentityService::GetInstanceFromChromeProvider()
        ->AddIdentity(expected_default_dentity_);
    CheckWarmStateConfigurator([mediator_ createConfigurator]);
  }

  void ExpectConfiguratorNotification(BOOL identity_changed) {
    configurator_ = nil;
    SigninPromoViewConfigurator* configurator_arg =
        [OCMArg checkWithBlock:^BOOL(id value) {
          configurator_ = value;
          return YES;
        }];
    OCMExpect([consumer_
        configureSigninPromoWithConfigurator:configurator_arg
                             identityChanged:identity_changed]);
  }

  void ExpectColdStateConfiguration() {
    OCMExpect([signin_promo_view_ setMode:SigninPromoViewModeColdState]);
    image_view_profile_image_ = nil;
    primary_button_title_ = nil;
    secondary_button_title_ = nil;
  }

  void CheckColdStateConfigurator(SigninPromoViewConfigurator* configurator) {
    EXPECT_NE(nil, configurator);
    ExpectColdStateConfiguration();
    OCMExpect([close_button_ setHidden:YES]);
    [configurator configureSigninPromoView:signin_promo_view_];
    EXPECT_EQ(nil, image_view_profile_image_);
    EXPECT_EQ(nil, primary_button_title_);
    EXPECT_EQ(nil, secondary_button_title_);
  }

  void ExpectWarmStateConfiguration() {
    EXPECT_EQ(expected_default_dentity_, mediator_.defaultIdentity);
    OCMExpect([signin_promo_view_ setMode:SigninPromoViewModeWarmState]);
    OCMExpect([signin_promo_view_
        setProfileImage:[OCMArg checkWithBlock:^BOOL(id value) {
          image_view_profile_image_ = value;
          return YES;
        }]]);
    primary_button_title_ = nil;
    OCMExpect([primary_button_ setTitle:[OCMArg checkWithBlock:^BOOL(id value) {
                                 primary_button_title_ = value;
                                 return YES;
                               }]
                               forState:UIControlStateNormal]);
    secondary_button_title_ = nil;
    OCMExpect([secondary_button_
        setTitle:[OCMArg checkWithBlock:^BOOL(id value) {
          secondary_button_title_ = value;
          return YES;
        }]
        forState:UIControlStateNormal]);
  }

  void CheckWarmStateConfigurator(SigninPromoViewConfigurator* configurator) {
    EXPECT_NE(nil, configurator);
    ExpectWarmStateConfiguration();
    OCMExpect([close_button_ setHidden:YES]);
    [configurator configureSigninPromoView:signin_promo_view_];
    EXPECT_NE(nil, image_view_profile_image_);
    NSString* userFullName = expected_default_dentity_.userFullName.length
                                 ? expected_default_dentity_.userFullName
                                 : expected_default_dentity_.userEmail;
    NSRange profileNameRange =
        [primary_button_title_ rangeOfString:userFullName];
    EXPECT_NE(profileNameRange.length, 0u);
    NSString* userEmail = expected_default_dentity_.userEmail;
    NSRange profileEmailRange =
        [secondary_button_title_ rangeOfString:userEmail];
    EXPECT_NE(profileEmailRange.length, 0u);
  }

  // Mediator used for the tests.
  SigninPromoViewMediator* mediator_;

  // Identity used for the warm state.
  FakeChromeIdentity* expected_default_dentity_;

  // Configurator received from the consumer.
  SigninPromoViewConfigurator* configurator_;

  // Mocks.
  id<SigninPromoViewConsumer> consumer_;
  SigninPromoView* signin_promo_view_;
  MDCFlatButton* primary_button_;
  MDCFlatButton* secondary_button_;
  UIButton* close_button_;

  // Value set by -[SigninPromoView setProfileImage:].
  UIImage* image_view_profile_image_;
  // Value set by -[primary_button_ setTitle: forState:UIControlStateNormal].
  NSString* primary_button_title_;
  // Value set by -[secondary_button_ setTitle: forState:UIControlStateNormal].
  NSString* secondary_button_title_;
  // Value set by -[close_button_ setHidden:].
  BOOL close_button_hidden_;
};

// Cold state with recent tabs.
TEST_F(SigninPromoViewMediatorTest, ColdStateConfigureSigninPromoView) {
  CreateMediator(signin_metrics::AccessPoint::ACCESS_POINT_RECENT_TABS);
  TestColdState();
}

// Cold state with tab switcher.
TEST_F(SigninPromoViewMediatorTest, ColdStateTabSwitcher) {
  CreateMediator(signin_metrics::AccessPoint::ACCESS_POINT_TAB_SWITCHER);
  TestColdState();
}

TEST_F(SigninPromoViewMediatorTest,
       WarmStateConfigureSigninPromoViewWithoutImage) {
  CreateMediator(signin_metrics::AccessPoint::ACCESS_POINT_RECENT_TABS);
  ExpectConfiguratorNotification(YES);
  expected_default_dentity_ =
      [FakeChromeIdentity identityWithEmail:@"johndoe@example.com"
                                     gaiaID:@"1"
                                       name:nil];
  ios::FakeChromeIdentityService::GetInstanceFromChromeProvider()->AddIdentity(
      expected_default_dentity_);
  CheckWarmStateConfigurator([mediator_ createConfigurator]);
  CheckWarmStateConfigurator(configurator_);
}

TEST_F(SigninPromoViewMediatorTest,
       WarmStateConfigureSigninPromoViewWithoutFullName) {
  CreateMediator(signin_metrics::AccessPoint::ACCESS_POINT_RECENT_TABS);
  ExpectConfiguratorNotification(YES);
  TestWarmState();
  CheckWarmStateConfigurator(configurator_);
}

TEST_F(SigninPromoViewMediatorTest,
       WarmStateConfigureSigninPromoViewWithImage) {
  CreateMediator(signin_metrics::AccessPoint::ACCESS_POINT_RECENT_TABS);
  ExpectConfiguratorNotification(YES);
  TestWarmState();
  ExpectConfiguratorNotification(NO);
  base::test::ios::SpinRunLoopWithMaxDelay(base::TimeDelta::FromSecondsD(0.1));
  CheckWarmStateConfigurator(configurator_);
}

// Cold state configuration and then warm state configuration.
TEST_F(SigninPromoViewMediatorTest, ConfigureSigninPromoViewWithColdAndWarm) {
  CreateMediator(signin_metrics::AccessPoint::ACCESS_POINT_RECENT_TABS);
  TestColdState();
  ExpectConfiguratorNotification(YES);
  TestWarmState();
  CheckWarmStateConfigurator(configurator_);
}

// Warm state configuration and then cold state configuration.
TEST_F(SigninPromoViewMediatorTest, ConfigureSigninPromoViewWithWarmAndCold) {
  CreateMediator(signin_metrics::AccessPoint::ACCESS_POINT_RECENT_TABS);
  ExpectConfiguratorNotification(YES);
  TestWarmState();
  CheckWarmStateConfigurator(configurator_);
  ExpectConfiguratorNotification(YES);
  ios::FakeChromeIdentityService::GetInstanceFromChromeProvider()
      ->RemoveIdentity(expected_default_dentity_);
  expected_default_dentity_ = nil;
  TestColdState();
  CheckColdStateConfigurator(configurator_);
}

TEST_F(SigninPromoViewMediatorTest, SigninPromoViewStateVisible) {
  CreateMediator(signin_metrics::AccessPoint::ACCESS_POINT_RECENT_TABS);
  EXPECT_EQ(ios::SigninPromoViewState::NeverVisible,
            mediator_.signinPromoViewState);
  [mediator_ signinPromoViewVisible];
  EXPECT_EQ(ios::SigninPromoViewState::Unused, mediator_.signinPromoViewState);
}

TEST_F(SigninPromoViewMediatorTest, SigninPromoViewStateSignedin) {
  CreateMediator(signin_metrics::AccessPoint::ACCESS_POINT_RECENT_TABS);
  [mediator_ signinPromoViewVisible];
  __block ShowSigninCommandCompletionCallback completion;
  ShowSigninCommandCompletionCallback completion_arg =
      [OCMArg checkWithBlock:^BOOL(ShowSigninCommandCompletionCallback value) {
        completion = value;
        return YES;
      }];
  OCMExpect([consumer_ signinPromoViewMediator:mediator_
                  shouldOpenSigninWithIdentity:nil
                                   promoAction:signin_metrics::PromoAction::
                                                   PROMO_ACTION_NEW_ACCOUNT
                                    completion:completion_arg]);
  [mediator_ signinPromoViewDidTapSigninWithNewAccount:signin_promo_view_];
  EXPECT_TRUE(mediator_.isSigninInProgress);
  EXPECT_EQ(ios::SigninPromoViewState::UsedAtLeastOnce,
            mediator_.signinPromoViewState);
  EXPECT_NE(nil, (id)completion);
  OCMExpect([consumer_ signinDidFinish]);
  completion(YES);
  EXPECT_FALSE(mediator_.isSigninInProgress);
  EXPECT_EQ(ios::SigninPromoViewState::UsedAtLeastOnce,
            mediator_.signinPromoViewState);
}

}  // namespace
