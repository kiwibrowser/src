// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ssl/ios_security_state_tab_helper.h"

#include "components/security_state/core/security_state.h"
#include "ios/chrome/browser/browser_state/test_chrome_browser_state.h"
#import "ios/chrome/browser/ssl/insecure_input_tab_helper.h"
#import "ios/chrome/browser/web/chrome_web_test.h"
#import "ios/web/public/navigation_item.h"
#import "ios/web/public/navigation_manager.h"
#include "ios/web/public/ssl_status.h"
#import "ios/web/public/test/web_test_with_web_state.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

// This test fixture enables Incognito mode and creates an
// IOSSecurityStateTabHelper for the WebState.
class IOSSecurityStateTabHelperIncognitoTest : public ChromeWebTest {
 protected:
  void SetUp() override {
    ChromeWebTest::SetUp();
    ASSERT_TRUE(web_state()->GetBrowserState()->IsOffTheRecord());
    IOSSecurityStateTabHelper::CreateForWebState(web_state());
  }

  // Returns the SecurityInfo for current WebState().
  void GetSecurityInfo(security_state::SecurityInfo* result) const {
    IOSSecurityStateTabHelper::FromWebState(web_state())
        ->GetSecurityInfo(result);
  }

  // WebTest
  web::BrowserState* GetBrowserState() override {
    return chrome_browser_state_->GetOffTheRecordChromeBrowserState();
  }
};

// Ensures that the security level of an HTTPS page loaded in Incognito mode
// is not downgraded.
TEST_F(IOSSecurityStateTabHelperIncognitoTest,
       SecurityInfoNotDowngradedForHTTPS) {
  LoadHtml(@"<html><body></body></html>", GURL("https://chromium.test"));
  security_state::SecurityInfo security_info;
  GetSecurityInfo(&security_info);
  EXPECT_FALSE(security_info.incognito_downgraded_security_level);
}

// Ensures that the security level of an HTTP page loaded in Incognito mode
// is downgraded.
TEST_F(IOSSecurityStateTabHelperIncognitoTest, SecurityInfoDowngradedForHTTP) {
  LoadHtml(@"<html><body></body></html>", GURL("http://chromium.test"));
  security_state::SecurityInfo security_info;
  GetSecurityInfo(&security_info);
  EXPECT_EQ(security_state::HTTP_SHOW_WARNING, security_info.security_level);
}

// This test fixture creates an IOSSecurityStateTabHelper and an
// InsecureInputTabHelper for the WebState, then loads a non-secure
// HTML document.
class IOSSecurityStateTabHelperTest : public web::WebTestWithWebState {
 protected:
  void SetUp() override {
    web::WebTestWithWebState::SetUp();
    InsecureInputTabHelper::CreateForWebState(web_state());
    insecure_input_ = InsecureInputTabHelper::FromWebState(web_state());
    ASSERT_TRUE(insecure_input_);

    IOSSecurityStateTabHelper::CreateForWebState(web_state());
    LoadHtml(@"<html><body></body></html>", GURL("http://chromium.test"));
  }

  // Returns the InsecureInputEventData for current WebState().
  security_state::InsecureInputEventData GetInsecureInputEventData() const {
    security_state::SecurityInfo info;
    IOSSecurityStateTabHelper::FromWebState(web_state())
        ->GetSecurityInfo(&info);
    return info.insecure_input_events;
  }

  InsecureInputTabHelper* insecure_input() { return insecure_input_; }

 private:
  InsecureInputTabHelper* insecure_input_;
};

// Ensures that an HTTP page in non-Incognito mode does not downgrade to
// non-secure.
TEST_F(IOSSecurityStateTabHelperTest, SecurityInfoNotDowngradedInNonIncognito) {
  LoadHtml(@"<html><body></body></html>", GURL("http://chromium.test"));
  security_state::SecurityInfo security_info;
  IOSSecurityStateTabHelper::FromWebState(web_state())
      ->GetSecurityInfo(&security_info);
  EXPECT_FALSE(security_info.incognito_downgraded_security_level);
}

// Ensures that |insecure_field_edited| is set only when an editing event has
// been reported.
TEST_F(IOSSecurityStateTabHelperTest, SecurityInfoAfterEditing) {
  // Verify |insecure_field_edited| is not set prematurely.
  security_state::InsecureInputEventData events = GetInsecureInputEventData();
  EXPECT_FALSE(events.insecure_field_edited);

  // Simulate an edit and verify |insecure_field_edited| is noted in the
  // insecure_input_events.
  insecure_input()->DidEditFieldInInsecureContext();
  events = GetInsecureInputEventData();
  EXPECT_TRUE(events.insecure_field_edited);
}

// Ensures that |password_field_shown| is set only when a password field has
// been reported.
TEST_F(IOSSecurityStateTabHelperTest, SecurityInfoWithInsecurePasswordField) {
  // Verify |password_field_shown| is not set prematurely.
  security_state::InsecureInputEventData events = GetInsecureInputEventData();
  EXPECT_FALSE(events.password_field_shown);

  // Simulate a password field display and verify |password_field_shown|
  // is noted in the insecure_input_events.
  insecure_input()->DidShowPasswordFieldInInsecureContext();
  events = GetInsecureInputEventData();
  EXPECT_TRUE(events.password_field_shown);
}

// Ensures that |credit_card_field_edited| is set only when a credit card field
// interaction has been reported.
TEST_F(IOSSecurityStateTabHelperTest, SecurityInfoWithInsecureCreditCardField) {
  // Verify |credit_card_field_edited| is not set prematurely.
  security_state::InsecureInputEventData events = GetInsecureInputEventData();
  EXPECT_FALSE(events.credit_card_field_edited);

  // Simulate a credit card field display and verify |credit_card_field_edited|
  // is noted in the insecure_input_events.
  insecure_input()->DidInteractWithNonsecureCreditCardInput();
  events = GetInsecureInputEventData();
  EXPECT_TRUE(events.credit_card_field_edited);
}
