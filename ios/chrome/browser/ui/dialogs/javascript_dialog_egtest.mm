// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <EarlGrey/EarlGrey.h>
#import <UIKit/UIKit.h>
#import <XCTest/XCTest.h>

#include "base/ios/ios_util.h"
#import "base/strings/sys_string_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "components/strings/grit/components_strings.h"
#import "ios/chrome/browser/ui/dialogs/dialog_presenter.h"
#include "ios/chrome/browser/ui/tools_menu/public/tools_menu_constants.h"
#include "ios/chrome/browser/ui/ui_util.h"
#include "ios/chrome/grit/ios_strings.h"
#include "ios/chrome/test/app/chrome_test_util.h"
#import "ios/chrome/test/earl_grey/chrome_actions.h"
#import "ios/chrome/test/earl_grey/chrome_earl_grey.h"
#import "ios/chrome/test/earl_grey/chrome_earl_grey_ui.h"
#import "ios/chrome/test/earl_grey/chrome_matchers.h"
#import "ios/chrome/test/earl_grey/chrome_test_case.h"
#import "ios/testing/earl_grey/matchers.h"
#import "ios/testing/wait_util.h"
#import "ios/web/public/test/earl_grey/web_view_matchers.h"
#import "ios/web/public/test/http_server/http_server.h"
#include "ios/web/public/test/http_server/http_server_util.h"
#include "ios/web/public/test/url_test_util.h"
#include "ios/web/public/web_state/web_state.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/l10n/l10n_util_mac.h"
#import "url/gurl.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

using chrome_test_util::ButtonWithAccessibilityLabel;
using chrome_test_util::OKButton;
using chrome_test_util::SettingsDoneButton;
using web::test::HttpServer;

namespace {

// Enum specifying different types of JavaScript alerts:
//   - JavaScriptAlertType::ALERT - Dialog with only one OK button.
//   - JavaScriptAlertType::CONFIRMATION - Dialog with OK and Cancel button.
//   - JavaScriptAlertType::PROMPT - Dialog with OK button, cancel button, and
//     a text field.
enum class JavaScriptAlertType : NSUInteger { ALERT, CONFIRMATION, PROMPT };

// Script to inject that will show an alert.  The document's body will be reset
// to |kAlertResultBody| after the dialog is dismissed.
const char kAlertMessage[] = "This is a JavaScript alert.";
const char kAlertResultBody[] = "JAVASCRIPT ALERT WAS DISMISSED";
const char kJavaScriptAlertTestScriptFormat[] =
    "(function(){ "
    "  alert(\"%@\");"
    "  document.body.innerHTML = \"%@\";"
    "})();";
NSString* GetJavaScriptAlertTestScript() {
  return [NSString stringWithFormat:@(kJavaScriptAlertTestScriptFormat),
                                    @(kAlertMessage), @(kAlertResultBody)];
}

// Script to inject that will show a confirmation dialog.  The document's body
// will be reset to |kConfirmationResultBodyOK| or
// |kConfirmationResultBodyCancelled| depending on whether the OK or Cancel
// button was tapped.
const char kConfirmationMessage[] = "This is a JavaScript confirmation.";
const char kConfirmationResultBodyOK[] = "Okay";
const char kConfirmationResultBodyCancelled[] = "Cancelled";
const char kJavaScriptConfirmationScriptFormat[] =
    "(function(){ "
    "  if (confirm(\"%@\") == true) {"
    "    document.body.innerHTML = \"%@\";"
    "  } else {"
    "    document.body.innerHTML = \"%@\";"
    "  }"
    "})();";
NSString* GetJavaScriptConfirmationTestScript() {
  return [NSString stringWithFormat:@(kJavaScriptConfirmationScriptFormat),
                                    @(kConfirmationMessage),
                                    @(kConfirmationResultBodyOK),
                                    @(kConfirmationResultBodyCancelled)];
}

// Script to inject that will show a prompt dialog.  The document's body will be
// reset to |kPromptResultBodyCancelled| or |kPromptTestUserInput| depending on
// whether the OK or Cancel button was tapped.
const char kPromptMessage[] = "This is a JavaScript prompt.";
const char kPromptResultBodyCancelled[] = "Cancelled";
const char kPromptTestUserInput[] = "test";
const char kJavaScriptPromptTestScriptFormat[] =
    "(function(){ "
    "  var input = prompt(\"%@\");"
    "  if (input != null) {"
    "    document.body.innerHTML = input;"
    "  } else {"
    "    document.body.innerHTML = \"%@\";"
    "  }"
    "})();";
NSString* GetJavaScriptPromptTestScript() {
  return [NSString stringWithFormat:@(kJavaScriptPromptTestScriptFormat),
                                    @(kPromptMessage),
                                    @(kPromptResultBodyCancelled)];
}

// Script to inject that will show a JavaScript alert in a loop 20 times, then
// reset the document's HTML to |kAlertLoopFinishedText|.
const char kAlertLoopFinishedText[] = "Loop Finished";
const char kJavaScriptAlertLoopScriptFormat[] =
    "(function(){ "
    "  for (i = 0; i < 20; ++i) {"
    "    alert(\"ALERT TEXT\");"
    "  }"
    "  document.body.innerHTML = \"%@\";"
    "})();";
NSString* GetJavaScriptAlertLoopScript() {
  return [NSString stringWithFormat:@(kJavaScriptAlertLoopScriptFormat),
                                    @(kAlertLoopFinishedText)];
}

// Returns the message for a JavaScript alert with |type|.
NSString* GetMessageForAlertWithType(JavaScriptAlertType type) {
  switch (type) {
    case JavaScriptAlertType::ALERT:
      return @(kAlertMessage);
    case JavaScriptAlertType::CONFIRMATION:
      return @(kConfirmationMessage);
    case JavaScriptAlertType::PROMPT:
      return @(kPromptMessage);
  }
  GREYFail(@"JavascriptAlertType not recognized.");
  return nil;
}

// Returns the script to show a JavaScript alert with |type|.
NSString* GetScriptForAlertWithType(JavaScriptAlertType type) {
  switch (type) {
    case JavaScriptAlertType::ALERT:
      return GetJavaScriptAlertTestScript();
    case JavaScriptAlertType::CONFIRMATION:
      return GetJavaScriptConfirmationTestScript();
    case JavaScriptAlertType::PROMPT:
      return GetJavaScriptPromptTestScript();
  }
  GREYFail(@"JavascriptAlertType not recognized.");
  return nil;
}

// HTTP server constants.

// URL and response for a blank document.
const char kJavaScriptTestURL[] = "http://jsalerts";
const char kJavaScriptTestResponse[] =
    "<!DOCTYPE html><html><body></body></html>";

// URL and response for a page with an onload alert.
const char kOnLoadAlertURL[] = "http://onloadalert";
const char kOnLoadAlertResponse[] =
    "<!DOCTYPE html><html><body onload=\"alert('alert')\"></body></html>";

// URL and response for a page with a link to |kOnLoadAlertURL|.
const char kPageWithLinkURL[] = "http://pagewithlink";
const char kPageWithLinkResponseFormat[] =
    "<!DOCTYPE html><html><body><a id=\"%s\" href=\"%s\">%s</a></body></html>";
const char kPageWithLinkText[] = "LINK TO ONLOAD ALERT PAGE";
const char kLinkID[] = "link-id";
std::string GetPageWithLinkResponse() {
  return base::SysNSStringToUTF8([NSString
      stringWithFormat:@(kPageWithLinkResponseFormat), kLinkID,
                       HttpServer::MakeUrl(kOnLoadAlertURL).spec().c_str(),
                       kPageWithLinkText]);
}


// Display the javascript alert.
void DisplayJavaScriptAlert(JavaScriptAlertType type) {
  // Get the WebController.
  web::WebState* webState = chrome_test_util::GetCurrentWebState();

  // Evaluate JavaScript.
  NSString* script = GetScriptForAlertWithType(type);
  webState->ExecuteJavaScript(base::SysNSStringToUTF16(script));
}

// Assert that the javascript alert has been presented.
void WaitForAlertToBeShown(NSString* alert_label) {
  // Wait for the alert to be shown by trying to get the alert title.
  ConditionBlock condition = ^{
    NSError* error = nil;
    id<GREYMatcher> titleLabel =
        chrome_test_util::StaticTextWithAccessibilityLabel(alert_label);
    [[EarlGrey selectElementWithMatcher:titleLabel]
        assertWithMatcher:grey_notNil()
                    error:&error];
    return !error;
  };
  GREYAssert(testing::WaitUntilConditionOrTimeout(
                 testing::kWaitForUIElementTimeout, condition),
             @"Alert with title was not present: %@", alert_label);
}

void WaitForJavaScriptDialogToBeShown() {
  GURL javaScriptURL = HttpServer::MakeUrl(kJavaScriptTestURL);

  NSString* hostname = base::SysUTF8ToNSString(javaScriptURL.host());
  NSString* expectedTitle = l10n_util::GetNSStringF(
      IDS_JAVASCRIPT_MESSAGEBOX_TITLE, base::SysNSStringToUTF16(hostname));

  WaitForAlertToBeShown(expectedTitle);
}

// Injects JavaScript to show a dialog with |type|, verifying that it was
// properly displayed.
void ShowJavaScriptDialog(JavaScriptAlertType type) {
  DisplayJavaScriptAlert(type);

  WaitForJavaScriptDialogToBeShown();

  // Check the message of the alert.
  id<GREYMatcher> messageLabel =
      chrome_test_util::StaticTextWithAccessibilityLabel(
          GetMessageForAlertWithType(type));
  [[EarlGrey selectElementWithMatcher:messageLabel]
      assertWithMatcher:grey_notNil()];
}

// Assert no javascript alert is visible.
void AssertJavaScriptAlertNotPresent() {
  ConditionBlock condition = ^{
    NSError* error = nil;
    GURL javaScriptURL = HttpServer::MakeUrl(kJavaScriptTestURL);
    NSString* hostname = base::SysUTF8ToNSString(javaScriptURL.host());
    NSString* expectedTitle = l10n_util::GetNSStringF(
        IDS_JAVASCRIPT_MESSAGEBOX_TITLE, base::SysNSStringToUTF16(hostname));

    id<GREYMatcher> titleLabel =
        chrome_test_util::StaticTextWithAccessibilityLabel(expectedTitle);
    [[EarlGrey selectElementWithMatcher:titleLabel] assertWithMatcher:grey_nil()
                                                                error:&error];
    return !error;
  };

  GREYAssert(testing::WaitUntilConditionOrTimeout(
                 testing::kWaitForJSCompletionTimeout, condition),
             @"Javascript alert title was still present");
}

// Types |input| in the prompt.
void TypeInPrompt(NSString* input) {
  [[[EarlGrey selectElementWithMatcher:
                  grey_accessibilityID(
                      kJavaScriptDialogTextFieldAccessibiltyIdentifier)]
      assertWithMatcher:grey_sufficientlyVisible()] performAction:grey_tap()];
  [[EarlGrey selectElementWithMatcher:
                 grey_accessibilityID(
                     kJavaScriptDialogTextFieldAccessibiltyIdentifier)]
      performAction:grey_typeText(input)];
}

void TapCancel() {
  [[EarlGrey selectElementWithMatcher:chrome_test_util::CancelButton()]
      performAction:grey_tap()];
}

void TapSuppressDialogsButton() {
  id<GREYMatcher> suppress_dialogs_button =
      chrome_test_util::ButtonWithAccessibilityLabelId(
          IDS_IOS_JAVA_SCRIPT_DIALOG_BLOCKING_BUTTON_TEXT);
  [[EarlGrey selectElementWithMatcher:suppress_dialogs_button]
      performAction:grey_tap()];
}

}  // namespace

@interface JavaScriptDialogTestCase : ChromeTestCase

// Loads the blank test page at kJavaScriptTestURL.
- (void)loadBlankTestPage;

// Loads a page with a link to kOnLoadAlertURL.
- (void)loadPageWithLink;

@end

@implementation JavaScriptDialogTestCase

- (void)setUp {
  [super setUp];
  std::map<GURL, std::string> responses;
  responses[HttpServer::MakeUrl(kJavaScriptTestURL)] = kJavaScriptTestResponse;
  responses[HttpServer::MakeUrl(kPageWithLinkURL)] = GetPageWithLinkResponse();
  responses[HttpServer::MakeUrl(kOnLoadAlertURL)] = kOnLoadAlertResponse;
  web::test::SetUpSimpleHttpServer(responses);
}

- (void)tearDown {
  NSError* errorOK = nil;
  NSError* errorCancel = nil;

  // Dismiss JavaScript alert by tapping Cancel.
  [[EarlGrey selectElementWithMatcher:chrome_test_util::CancelButton()]
      performAction:grey_tap()
              error:&errorCancel];
  // Dismiss JavaScript alert by tapping OK.
  id<GREYMatcher> OKButton =
      chrome_test_util::ButtonWithAccessibilityLabelId(IDS_OK);
  [[EarlGrey selectElementWithMatcher:OKButton] performAction:grey_tap()
                                                        error:&errorOK];
  // Reenable synchronization in case it was disabled by a test.  See comments
  // in testShowJavaScriptAfterNewTabAnimation for details.
  [[GREYConfiguration sharedInstance]
          setValue:@(YES)
      forConfigKey:kGREYConfigKeySynchronizationEnabled];

  if (!errorOK || !errorCancel) {
    GREYFail(@"There are still alerts");
  }
  [super tearDown];
}

#pragma mark - Utility

- (void)loadBlankTestPage {
  [ChromeEarlGrey loadURL:HttpServer::MakeUrl(kJavaScriptTestURL)];
  [ChromeEarlGrey waitForWebViewContainingText:std::string()];
}

- (void)loadPageWithLink {
  [ChromeEarlGrey loadURL:HttpServer::MakeUrl(kPageWithLinkURL)];
  [ChromeEarlGrey waitForWebViewContainingText:kPageWithLinkText];
}

#pragma mark - Tests

// Tests that an alert is shown, and that the completion block is called.
- (void)testShowJavaScriptAlert {
  // Load the blank test page and show an alert.
  [self loadBlankTestPage];
  ShowJavaScriptDialog(JavaScriptAlertType::ALERT);

  [[EarlGrey selectElementWithMatcher:OKButton()] performAction:grey_tap()];

  // Wait for the html body to be reset to the correct value.
  [ChromeEarlGrey waitForWebViewContainingText:kAlertResultBody];
}

// Tests that a confirmation dialog is shown, and that the completion block is
// called with the correct value when the OK buton is tapped.
- (void)testShowJavaScriptConfirmationOK {
  // Load the blank test page and show a confirmation dialog.
  [self loadBlankTestPage];
  ShowJavaScriptDialog(JavaScriptAlertType::CONFIRMATION);

  [[EarlGrey selectElementWithMatcher:OKButton()] performAction:grey_tap()];

  // Wait for the html body to be reset to the correct value.
  [ChromeEarlGrey waitForWebViewContainingText:kConfirmationResultBodyOK];
}

// Tests that a confirmation dialog is shown, and that the completion block is
// called with the correct value when the Cancel buton is tapped.
- (void)testShowJavaScriptConfirmationCancelled {
  // Load the blank test page and show a confirmation dialog.
  [self loadBlankTestPage];
  ShowJavaScriptDialog(JavaScriptAlertType::CONFIRMATION);

  // Tap the Cancel button.
  TapCancel();

  // Wait for the html body to be reset to the correct value.
  [ChromeEarlGrey
      waitForWebViewContainingText:kConfirmationResultBodyCancelled];
}

// Tests that a prompt dialog is shown, and that the completion block is called
// with the correct value when the OK buton is tapped.
- (void)testShowJavaScriptPromptOK {
  // TODO(crbug.com/753098): Re-enable this test on iOS 11 iPad once
  // grey_typeText works on iOS 11.
  if (base::ios::IsRunningOnIOS11OrLater() && IsIPadIdiom()) {
    EARL_GREY_TEST_DISABLED(@"Test disabled on iOS 11.");
  }

  // Load the blank test page and show a prompt dialog.
  [self loadBlankTestPage];
  ShowJavaScriptDialog(JavaScriptAlertType::PROMPT);

  // Enter text into text field.
  TypeInPrompt(@(kPromptTestUserInput));

  [[EarlGrey selectElementWithMatcher:OKButton()] performAction:grey_tap()];

  // Wait for the html body to be reset to the input text.
  [ChromeEarlGrey waitForWebViewContainingText:kPromptTestUserInput];
}

// Tests that a prompt dialog is shown, and that the completion block is called
// with the correct value when the Cancel buton is tapped.
- (void)testShowJavaScriptPromptCancelled {
  // TODO(crbug.com/753098): Re-enable this test on iOS 11 iPad once
  // grey_typeText works on iOS 11.
  if (base::ios::IsRunningOnIOS11OrLater() && IsIPadIdiom()) {
    EARL_GREY_TEST_DISABLED(@"Test disabled on iOS 11.");
  }

  // Load the blank test page and show a prompt dialog.
  [self loadBlankTestPage];
  ShowJavaScriptDialog(JavaScriptAlertType::PROMPT);

  // Enter text into text field.
  TypeInPrompt(@(kPromptTestUserInput));

  // Tap the Cancel button.
  TapCancel();

  // Wait for the html body to be reset to the cancel text.
  [ChromeEarlGrey waitForWebViewContainingText:kPromptResultBodyCancelled];
}

// Tests that JavaScript alerts that are shown in a loop can be suppressed.
- (void)testShowJavaScriptAlertLoop {
  // Load the blank test page and show alerts in a loop.
  [self loadBlankTestPage];
  web::WebState* webState = chrome_test_util::GetCurrentWebState();
  NSString* script = GetJavaScriptAlertLoopScript();
  webState->ExecuteJavaScript(base::SysNSStringToUTF16(script));
  WaitForJavaScriptDialogToBeShown();

  [[EarlGrey selectElementWithMatcher:OKButton()] performAction:grey_tap()];
  WaitForJavaScriptDialogToBeShown();

  // Tap the suppress dialogs button.
  TapSuppressDialogsButton();

  // Wait for confirmation action sheet to be shown.
  NSString* alertLabel =
      l10n_util::GetNSString(IDS_JAVASCRIPT_MESSAGEBOX_SUPPRESS_OPTION);
  WaitForAlertToBeShown(alertLabel);

  // Tap the suppress dialogs confirmation button.
  TapSuppressDialogsButton();

  // Wait for the html body to be reset to the loop finished text.
  [ChromeEarlGrey waitForWebViewContainingText:kAlertLoopFinishedText];
}

// Tests to ensure crbug.com/658260 does not regress.
// Tests that if an alert should be called when settings are displays, the alert
// waits for the dismiss of the settings.
- (void)testShowJavaScriptBehindSettings {
  // Load the blank test page.
  [self loadBlankTestPage];

  // Show settings.
  [ChromeEarlGreyUI openSettingsMenu];
  [[EarlGrey selectElementWithMatcher:chrome_test_util::
                                          StaticTextWithAccessibilityLabelId(
                                              IDS_IOS_SETTINGS_TITLE)]
      assertWithMatcher:grey_sufficientlyVisible()];

  // Show an alert.
  DisplayJavaScriptAlert(JavaScriptAlertType::ALERT);

  // Make sure the alert is not present.
  AssertJavaScriptAlertNotPresent();

  // Close the settings.
  [[EarlGrey selectElementWithMatcher:SettingsDoneButton()]
      performAction:grey_tap()];

  // Make sure the alert is present.
  WaitForJavaScriptDialogToBeShown();

  [[EarlGrey selectElementWithMatcher:OKButton()] performAction:grey_tap()];

  // Wait for the html body to be reset to the correct value.
  [ChromeEarlGrey waitForWebViewContainingText:kAlertResultBody];
}

// Tests that an alert is presented after displaying the share menu.
- (void)testShowJavaScriptAfterShareMenu {
  // TODO(crbug.com/747622): re-enable this test on iOS 11 once earl grey can
  // interact with the share menu.
  if (base::ios::IsRunningOnIOS11OrLater()) {
    EARL_GREY_TEST_DISABLED(@"Disabled on iOS 11.");
  }

  // Load the blank test page.
  [self loadBlankTestPage];

  [ChromeEarlGreyUI openShareMenu];

  // Copy URL, dismissing the share menu.
  id<GREYMatcher> printButton =
      grey_allOf(grey_accessibilityLabel(@"Copy"),
                 grey_accessibilityTrait(UIAccessibilityTraitButton), nil);
  [[EarlGrey selectElementWithMatcher:printButton] performAction:grey_tap()];

  // Show an alert and assert it is present.
  ShowJavaScriptDialog(JavaScriptAlertType::ALERT);

  [[EarlGrey selectElementWithMatcher:OKButton()] performAction:grey_tap()];

  // Wait for the html body to be reset to the correct value.
  [ChromeEarlGrey waitForWebViewContainingText:kAlertResultBody];
}

// Tests that an alert is presented after a new tab animation is finished.
- (void)testShowJavaScriptAfterNewTabAnimation {
  // Load the test page with a link to kOnLoadAlertURL and long tap on the link.
  [self loadPageWithLink];

  // TODO(crbug.com/712358): Use method LongPressElementAndTapOnButton once
  // it is moved out of context_menu_egtests.mm and into a shared location.
  [ChromeEarlGrey waitForWebViewContainingText:kPageWithLinkText];
  id<GREYMatcher> webViewMatcher =
      web::WebViewInWebState(chrome_test_util::GetCurrentWebState());

  [[EarlGrey selectElementWithMatcher:webViewMatcher]
      performAction:chrome_test_util::LongPressElementForContextMenu(
                        kLinkID, true /* menu should appear */)];

  // Tap on the "Open In New Tab" button.
  id<GREYMatcher> newTabMatcher = ButtonWithAccessibilityLabel(
      l10n_util::GetNSStringWithFixup(IDS_IOS_CONTENT_CONTEXT_OPENLINKNEWTAB));
  [[EarlGrey selectElementWithMatcher:newTabMatcher] performAction:grey_tap()];

  // This test case requires that a dialog is presented in the onload event so
  // that the DialogPresenter attempts to display during a new tab animation.
  // Because presenting a dialog halts the JavaScript execution on the page,
  // this prevents the page loaded event from being received until the alert is
  // closed.  On iPad, this means that there is a loading indicator that
  // continues to animate until the dialog is closed.  Disabling EarlGrey
  // synchronization code for iPad allows the test to detect and dismiss the
  // dialog while this animation is occurring.
  if (IsIPadIdiom()) {
    [[GREYConfiguration sharedInstance]
            setValue:@(NO)
        forConfigKey:kGREYConfigKeySynchronizationEnabled];
  }

  // Wait for the alert to be shown.
  GURL javaScriptURL = HttpServer::MakeUrl(kJavaScriptTestURL);
  NSString* hostname = base::SysUTF8ToNSString(javaScriptURL.host());
  NSString* expectedTitle = l10n_util::GetNSStringF(
      IDS_JAVASCRIPT_MESSAGEBOX_TITLE, base::SysNSStringToUTF16(hostname));

  WaitForAlertToBeShown(expectedTitle);

  // Verify that the omnibox shows the correct URL when the dialog is visible.
  GURL onloadURL = HttpServer::MakeUrl(kOnLoadAlertURL);
  std::string title = base::UTF16ToUTF8(web::GetDisplayTitleForUrl(onloadURL));
  [[EarlGrey selectElementWithMatcher:chrome_test_util::OmniboxText(title)]
      assertWithMatcher:grey_notNil()];

  [[EarlGrey selectElementWithMatcher:OKButton()] performAction:grey_tap()];

  // Reenable synchronization on iPads now that the dialog has been dismissed.
  if (IsIPadIdiom()) {
    [[GREYConfiguration sharedInstance]
            setValue:@(YES)
        forConfigKey:kGREYConfigKeySynchronizationEnabled];
  }
}

@end
