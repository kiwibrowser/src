// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/passwords/credential_manager.h"

#import <EarlGrey/EarlGrey.h>
#import <UIKit/UIKit.h>
#import <XCTest/XCTest.h>

#include <memory>

#include "base/strings/utf_string_conversions.h"
#include "base/test/scoped_feature_list.h"
#include "components/keyed_service/core/service_access_type.h"
#include "components/password_manager/core/common/password_manager_pref_names.h"
#include "ios/chrome/browser/passwords/credential_manager_features.h"
#include "ios/chrome/browser/passwords/ios_chrome_password_store_factory.h"
#import "ios/chrome/test/app/chrome_test_util.h"
#import "ios/chrome/test/app/tab_test_util.h"
#import "ios/chrome/test/earl_grey/chrome_earl_grey.h"
#import "ios/chrome/test/earl_grey/chrome_earl_grey_ui.h"
#import "ios/chrome/test/earl_grey/chrome_matchers.h"
#import "ios/chrome/test/earl_grey/chrome_test_case.h"
#import "ios/testing/earl_grey/disabled_test_macros.h"
#import "ios/testing/wait_util.h"
#import "ios/web/public/test/http_server/http_server.h"
#include "ios/web/public/test/http_server/http_server_util.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

// Notification should be displayed for 3 seconds. 4 should be safe to check.
constexpr CFTimeInterval kDisappearanceTimeout = 4;
// Provides basic response for example page.
std::unique_ptr<net::test_server::HttpResponse> StandardResponse(
    const net::test_server::HttpRequest& request) {
  std::unique_ptr<net::test_server::BasicHttpResponse> http_response =
      std::make_unique<net::test_server::BasicHttpResponse>();
  http_response->set_code(net::HTTP_OK);
  http_response->set_content(
      "<head><title>Example website</title></head>"
      "<body>You are here.</body>");
  return std::move(http_response);
}

}  // namespace

// This class tests UI behavior for Credential Manager.
// TODO(crbug.com/435048): Add EG test for save/update password prompt.
// TODO(crbug.com/435048): When account chooser and first run experience dialog
// are implemented, test them too.
@interface CredentialManagerEGTest : ChromeTestCase

@end

@implementation CredentialManagerEGTest {
  base::test::ScopedFeatureList _featureList;
}

- (void)setUp {
  _featureList.InitAndEnableFeature(features::kCredentialManager);
  [super setUp];

  // Set up server.
  self.testServer->RegisterRequestHandler(base::Bind(&StandardResponse));
  GREYAssertTrue(self.testServer->Start(), @"Test server failed to start.");
}

- (void)tearDown {
  scoped_refptr<password_manager::PasswordStore> passwordStore =
      IOSChromePasswordStoreFactory::GetForBrowserState(
          chrome_test_util::GetOriginalBrowserState(),
          ServiceAccessType::IMPLICIT_ACCESS)
          .get();
  // Remove Credentials stored during executing the test.
  passwordStore->RemoveLoginsCreatedBetween(base::Time(), base::Time::Now(),
                                            base::Closure());
  [super tearDown];
}

#pragma mark - Utils

// Sets preferences required for autosign-in to true.
- (void)setAutosigninPreferences {
  chrome_test_util::SetBooleanUserPref(
      chrome_test_util::GetOriginalBrowserState(),
      password_manager::prefs::kWasAutoSignInFirstRunExperienceShown, true);
  chrome_test_util::SetBooleanUserPref(
      chrome_test_util::GetOriginalBrowserState(),
      password_manager::prefs::kCredentialsEnableAutosignin, true);
}

// Loads simple page on localhost and stores an example PasswordCredential.
- (void)loadSimplePageAndStoreACredential {
  // Loads simple page. It is on localhost so it is considered a secure context.
  const GURL URL = self.testServer->GetURL("/example");
  [ChromeEarlGrey loadURL:URL];
  [ChromeEarlGrey waitForWebViewContainingText:"You are here."];

  // Obtain a PasswordStore.
  scoped_refptr<password_manager::PasswordStore> passwordStore =
      IOSChromePasswordStoreFactory::GetForBrowserState(
          chrome_test_util::GetOriginalBrowserState(),
          ServiceAccessType::IMPLICIT_ACCESS)
          .get();
  GREYAssertTrue(passwordStore != nullptr,
                 @"PasswordStore is unexpectedly null for BrowserState");

  // Store a PasswordForm representing a PasswordCredential.
  autofill::PasswordForm passwordCredentialForm;
  passwordCredentialForm.username_value =
      base::ASCIIToUTF16("johndoe@example.com");
  passwordCredentialForm.password_value = base::ASCIIToUTF16("ilovejanedoe123");
  passwordCredentialForm.origin =
      chrome_test_util::GetCurrentWebState()->GetLastCommittedURL().GetOrigin();
  passwordCredentialForm.signon_realm = passwordCredentialForm.origin.spec();
  passwordCredentialForm.scheme = autofill::PasswordForm::SCHEME_HTML;
  passwordStore->AddLogin(passwordCredentialForm);
}

#pragma mark - Tests

// Tests that notification saying "Signing is as ..." appears on auto sign-in.
- (void)testNotificationAppearsOnAutoSignIn {
  // TODO(crbug.com/786960): re-enable when fixed.
  EARL_GREY_TEST_DISABLED(@"Fails on iOS 11.0.");

  [self setAutosigninPreferences];
  [self loadSimplePageAndStoreACredential];

  // Call get() from JavaScript.
  NSError* error = nil;
  NSString* result = chrome_test_util::ExecuteJavaScript(
      @"typeof navigator.credentials.get({password: true})", &error);
  GREYAssertTrue([result isEqual:@"object"],
                 @"Unexpected error occurred when executing JavaScript.");
  GREYAssertTrue(!error,
                 @"Unexpected error occurred when executing JavaScript.");

  // Matches the UILabel by its accessibilityLabel.
  id<GREYMatcher> matcher =
      grey_allOf(grey_accessibilityLabel(@"Signing in as johndoe@example.com"),
                 grey_accessibilityTrait(UIAccessibilityTraitStaticText), nil);
  // Wait for notification to appear.
  ConditionBlock waitForAppearance = ^{
    NSError* error = nil;
    [[EarlGrey selectElementWithMatcher:matcher] assertWithMatcher:grey_notNil()
                                                             error:&error];
    return error == nil;
  };
  // Gives some time for the notification to appear.
  GREYAssert(testing::WaitUntilConditionOrTimeout(
                 testing::kWaitForUIElementTimeout, waitForAppearance),
             @"Notification did not appear");
  // Wait for the notification to disappear.
  ConditionBlock waitForDisappearance = ^{
    NSError* error = nil;
    [[EarlGrey selectElementWithMatcher:matcher]
        assertWithMatcher:grey_sufficientlyVisible()
                    error:&error];
    return error == nil;
  };
  // Ensures that notification disappears after time limit.
  GREYAssert(testing::WaitUntilConditionOrTimeout(kDisappearanceTimeout,
                                                  waitForDisappearance),
             @"Notification did not disappear");
}

// Tests that when navigator.credentials.get() was called from inactive tab, the
// autosign-in notification appears once tab becomes active.
- (void)testNotificationAppearsWhenTabIsActive {
  // TODO(crbug.com/786960): re-enable when fixed.
  EARL_GREY_TEST_DISABLED(@"Fails on iOS 11.0.");

  [self setAutosigninPreferences];
  [self loadSimplePageAndStoreACredential];

  // Get WebState before switching the tab.
  web::WebState* webState = chrome_test_util::GetCurrentWebState();

  // Open new tab.
  [ChromeEarlGreyUI openNewTab];
  [ChromeEarlGrey waitForMainTabCount:2];

  // Execute JavaScript from inactive tab.
  webState->ExecuteJavaScript(
      base::UTF8ToUTF16("typeof navigator.credentials.get({password: true})"));

  // Matches the UILabel by its accessibilityLabel.
  id<GREYMatcher> matcher = chrome_test_util::StaticTextWithAccessibilityLabel(
      @"Signing in as johndoe@example.com");
  // Wait for notification to appear.
  ConditionBlock waitForAppearance = ^{
    NSError* error = nil;
    [[EarlGrey selectElementWithMatcher:matcher]
        assertWithMatcher:grey_sufficientlyVisible()
                    error:&error];
    return error == nil;
  };

  // Check that notification doesn't appear in current tab.
  GREYAssertFalse(testing::WaitUntilConditionOrTimeout(
                      testing::kWaitForUIElementTimeout, waitForAppearance),
                  @"Notification appeared in wrong tab");

  // Switch to previous tab.
  chrome_test_util::SelectTabAtIndexInCurrentMode(0);

  // Check that the notification has appeared.
  GREYAssert(testing::WaitUntilConditionOrTimeout(
                 testing::kWaitForUIElementTimeout, waitForAppearance),
             @"Notification did not appear");
}

@end
