// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/signin/dice_tab_helper.h"

#include "base/test/histogram_tester.h"
#include "base/test/user_action_tester.h"
#include "chrome/test/base/testing_profile.h"
#include "components/signin/core/browser/signin_metrics.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "content/public/test/test_web_contents_factory.h"
#include "google_apis/gaia/gaia_urls.h"
#include "testing/gtest/include/gtest/gtest.h"

// Tests DiceTabHelper intialization.
TEST(DiceTabHelperTest, Initialization) {
  content::TestBrowserThreadBundle thread_bundle;
  TestingProfile profile;
  content::TestWebContentsFactory factory;
  content::WebContents* web_contents = factory.CreateWebContents(&profile);
  DiceTabHelper::CreateForWebContents(web_contents);
  DiceTabHelper* dice_tab_helper = DiceTabHelper::FromWebContents(web_contents);

  // Check default state.
  EXPECT_EQ(signin_metrics::AccessPoint::ACCESS_POINT_UNKNOWN,
            dice_tab_helper->signin_access_point());
  EXPECT_EQ(signin_metrics::Reason::REASON_UNKNOWN_REASON,
            dice_tab_helper->signin_reason());

  // Initialize the signin flow.
  signin_metrics::AccessPoint access_point =
      signin_metrics::AccessPoint::ACCESS_POINT_BOOKMARK_BUBBLE;
  signin_metrics::Reason reason =
      signin_metrics::Reason::REASON_SIGNIN_PRIMARY_ACCOUNT;
  dice_tab_helper->InitializeSigninFlow(
      access_point, reason,
      signin_metrics::PromoAction::PROMO_ACTION_NO_SIGNIN_PROMO);
  EXPECT_EQ(access_point, dice_tab_helper->signin_access_point());
  EXPECT_EQ(reason, dice_tab_helper->signin_reason());
}

// Tests DiceTabHelper metrics.
TEST(DiceTabHelperTest, Metrics) {
  base::UserActionTester ua_tester;
  base::HistogramTester h_tester;
  content::TestBrowserThreadBundle thread_bundle;
  TestingProfile profile;
  content::TestWebContentsFactory factory;
  content::WebContents* web_contents = factory.CreateWebContents(&profile);
  DiceTabHelper::CreateForWebContents(web_contents);
  DiceTabHelper* dice_tab_helper = DiceTabHelper::FromWebContents(web_contents);

  // No metrics are logged when the Dice tab helper is created.
  EXPECT_EQ(0, ua_tester.GetActionCount("Signin_Signin_FromStartPage"));
  EXPECT_EQ(0, ua_tester.GetActionCount("Signin_SigninPage_Loading"));
  EXPECT_EQ(0, ua_tester.GetActionCount("Signin_SigninPage_Shown"));

  // Check metrics logged when the Dice tab helper is initialized.
  dice_tab_helper->InitializeSigninFlow(
      signin_metrics::AccessPoint::ACCESS_POINT_SETTINGS,
      signin_metrics::Reason::REASON_SIGNIN_PRIMARY_ACCOUNT,
      signin_metrics::PromoAction::PROMO_ACTION_NEW_ACCOUNT);
  EXPECT_EQ(1, ua_tester.GetActionCount("Signin_Signin_FromSettings"));
  EXPECT_EQ(1, ua_tester.GetActionCount("Signin_SigninPage_Loading"));
  EXPECT_EQ(0, ua_tester.GetActionCount("Signin_SigninPage_Shown"));
  h_tester.ExpectUniqueSample(
      "Signin.SigninStartedAccessPoint",
      signin_metrics::AccessPoint::ACCESS_POINT_SETTINGS, 1);
  h_tester.ExpectUniqueSample(
      "Signin.SigninStartedAccessPoint.NewAccount",
      signin_metrics::AccessPoint::ACCESS_POINT_SETTINGS, 1);

  // First call to did finish load does logs any Signin_SigninPage_Shown user
  // action.
  GURL signin_page_url = GaiaUrls::GetInstance()->gaia_url();
  dice_tab_helper->DidFinishLoad(nullptr, signin_page_url);
  EXPECT_EQ(1, ua_tester.GetActionCount("Signin_SigninPage_Loading"));
  EXPECT_EQ(1, ua_tester.GetActionCount("Signin_SigninPage_Shown"));

  // Second call to did finish load does not log any metrics.
  dice_tab_helper->DidFinishLoad(nullptr, signin_page_url);
  EXPECT_EQ(1, ua_tester.GetActionCount("Signin_SigninPage_Loading"));
  EXPECT_EQ(1, ua_tester.GetActionCount("Signin_SigninPage_Shown"));

  // Check metrics are logged again when the Dice tab helper is re-initialized.
  dice_tab_helper->InitializeSigninFlow(
      signin_metrics::AccessPoint::ACCESS_POINT_SETTINGS,
      signin_metrics::Reason::REASON_SIGNIN_PRIMARY_ACCOUNT,
      signin_metrics::PromoAction::PROMO_ACTION_WITH_DEFAULT);
  EXPECT_EQ(2, ua_tester.GetActionCount("Signin_Signin_FromSettings"));
  EXPECT_EQ(2, ua_tester.GetActionCount("Signin_SigninPage_Loading"));
  h_tester.ExpectUniqueSample(
      "Signin.SigninStartedAccessPoint",
      signin_metrics::AccessPoint::ACCESS_POINT_SETTINGS, 2);
  h_tester.ExpectUniqueSample(
      "Signin.SigninStartedAccessPoint.WithDefault",
      signin_metrics::AccessPoint::ACCESS_POINT_SETTINGS, 1);
}
