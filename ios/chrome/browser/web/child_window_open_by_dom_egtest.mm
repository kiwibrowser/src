// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <EarlGrey/EarlGrey.h>

#include "components/content_settings/core/common/content_settings.h"
#include "ios/chrome/test/app/settings_test_util.h"
#import "ios/chrome/test/app/tab_test_util.h"
#import "ios/chrome/test/app/web_view_interaction_test_util.h"
#import "ios/chrome/test/earl_grey/chrome_earl_grey.h"
#import "ios/chrome/test/earl_grey/chrome_matchers.h"
#import "ios/chrome/test/earl_grey/chrome_test_case.h"
#import "ios/web/public/test/http_server/http_server.h"
#include "ios/web/public/test/http_server/http_server_util.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

using chrome_test_util::TapWebViewElementWithId;
using web::test::HttpServer;

namespace {
// Test link text and ids.
const char kNamedWindowLink[] = "openWindowWithName";
const char kUnnamedWindowLink[] = "openWindowNoName";

// Web view text that indicates window's closed state.
const char kWindow2NeverOpen[] = "window2.closed: never opened";
const char kWindow1Open[] = "window1.closed: false";
const char kWindow2Open[] = "window2.closed: false";
const char kWindow1Closed[] = "window1.closed: true";
const char kWindow2Closed[] = "window2.closed: true";

}  // namespace

// Test case for child windows opened by DOM.
@interface ChildWindowOpenByDOMTestCase : ChromeTestCase
@end

@implementation ChildWindowOpenByDOMTestCase

+ (void)setUp {
  [super setUp];
  chrome_test_util::SetContentSettingsBlockPopups(CONTENT_SETTING_ALLOW);
  web::test::SetUpFileBasedHttpServer();
}

+ (void)tearDown {
  chrome_test_util::SetContentSettingsBlockPopups(CONTENT_SETTING_DEFAULT);
  [super tearDown];
}

- (void)setUp {
  [super setUp];
  // Open the test page. There should only be one tab open.
  const char kChildWindowTestURL[] =
      "http://ios/testing/data/http_server_files/window_proxy.html";
  [ChromeEarlGrey loadURL:HttpServer::MakeUrl(kChildWindowTestURL)];
  [ChromeEarlGrey waitForWebViewContainingText:kNamedWindowLink];
  [ChromeEarlGrey waitForMainTabCount:1];
}

// Tests that multiple calls to window.open() with the same window name returns
// the same window object.
- (void)test2ChildWindowsWithName {
  // Open two windows with the same name.
  GREYAssert(TapWebViewElementWithId(kNamedWindowLink), @"Failed to tap %s",
             kNamedWindowLink);
  [ChromeEarlGrey waitForMainTabCount:2];
  chrome_test_util::SelectTabAtIndexInCurrentMode(0);

  GREYAssert(TapWebViewElementWithId(kNamedWindowLink), @"Failed to tap %s",
             kNamedWindowLink);
  [ChromeEarlGrey waitForMainTabCount:2];
  chrome_test_util::SelectTabAtIndexInCurrentMode(0);

  // Check that they're the same window.
  GREYAssert(TapWebViewElementWithId("compareNamedWindows"),
             @"Failed to tap \"compareNamedWindows\"");
  const char kWindowsEqualText[] = "named windows equal: true";
  [ChromeEarlGrey waitForWebViewContainingText:kWindowsEqualText];
}

// Tests that multiple calls to window.open() with no window name passed in
// returns a unique window object each time.
- (void)test2ChildWindowsWithoutName {
  // Open two unnamed windows.
  GREYAssert(TapWebViewElementWithId(kUnnamedWindowLink), @"Failed to tap %s",
             kUnnamedWindowLink);
  [ChromeEarlGrey waitForMainTabCount:2];
  chrome_test_util::SelectTabAtIndexInCurrentMode(0);

  GREYAssert(TapWebViewElementWithId(kUnnamedWindowLink), @"Failed to tap %s",
             kUnnamedWindowLink);
  [ChromeEarlGrey waitForMainTabCount:3];
  chrome_test_util::SelectTabAtIndexInCurrentMode(0);

  // Check that they aren't the same window object.
  GREYAssert(TapWebViewElementWithId("compareUnnamedWindows"),
             @"Failed to tap \"compareUnnamedWindows\"");
  const char kWindowsEqualText[] = "unnamed windows equal: false";
  [ChromeEarlGrey waitForWebViewContainingText:kWindowsEqualText];
}

// Tests that calling window.open() with a name returns a different window
// object than a subsequent call to window.open() without a name.
- (void)testChildWindowsWithAndWithoutName {
  // Open a named window.
  GREYAssert(TapWebViewElementWithId(kNamedWindowLink), @"Failed to tap %s",
             kNamedWindowLink);
  [ChromeEarlGrey waitForMainTabCount:2];
  chrome_test_util::SelectTabAtIndexInCurrentMode(0);

  // Open an unnamed window.
  GREYAssert(TapWebViewElementWithId(kUnnamedWindowLink), @"Failed to tap %s",
             kUnnamedWindowLink);
  [ChromeEarlGrey waitForMainTabCount:3];
  chrome_test_util::SelectTabAtIndexInCurrentMode(0);

  // Check that they aren't the same window object.
  GREYAssert(TapWebViewElementWithId("compareNamedAndUnnamedWindows"),
             @"Failed to tap \"compareNamedAndUnnamedWindows\"");
  const char kWindowsEqualText[] = "named and unnamed equal: false";
  [ChromeEarlGrey waitForWebViewContainingText:kWindowsEqualText];
}

// Tests that window.closed is correctly set to true when the corresponding tab
// is closed. Verifies that calling window.open() multiple times with the same
// name returns the same window object, and thus closing the corresponding tab
// results in window.closed being set to true for all references to the window
// object for that tab.
- (void)testWindowClosedWithName {
  GREYAssert(TapWebViewElementWithId("openWindowWithName"),
             @"Failed to tap \"openWindowWithName\"");
  [ChromeEarlGrey waitForMainTabCount:2];
  chrome_test_util::SelectTabAtIndexInCurrentMode(0);

  // Check that named window 1 is opened and named window 2 isn't.
  const char kCheckWindow1Link[] = "checkNamedWindow1Closed";
  [ChromeEarlGrey waitForWebViewContainingText:kCheckWindow1Link];
  GREYAssert(TapWebViewElementWithId(kCheckWindow1Link), @"Failed to tap %s",
             kCheckWindow1Link);
  [ChromeEarlGrey waitForWebViewContainingText:kWindow1Open];
  const char kCheckWindow2Link[] = "checkNamedWindow2Closed";
  GREYAssert(TapWebViewElementWithId(kCheckWindow2Link), @"Failed to tap %s",
             kCheckWindow2Link);
  [ChromeEarlGrey waitForWebViewContainingText:kWindow2NeverOpen];

  // Open another window with the same name. Check that named window 2 is now
  // opened.
  GREYAssert(TapWebViewElementWithId("openWindowWithName"),
             @"Failed to tap \"openWindowWithName\"");
  [ChromeEarlGrey waitForMainTabCount:2];
  chrome_test_util::SelectTabAtIndexInCurrentMode(0);
  GREYAssert(TapWebViewElementWithId(kCheckWindow2Link), @"Failed to tap %s",
             kCheckWindow2Link);
  [ChromeEarlGrey waitForWebViewContainingText:kWindow2Open];

  // Close the opened window. Check that named window 1 and 2 are both closed.
  chrome_test_util::CloseTabAtIndex(1);
  [ChromeEarlGrey waitForMainTabCount:1];
  GREYAssert(TapWebViewElementWithId(kCheckWindow1Link), @"Failed to tap %s",
             kCheckWindow1Link);
  [ChromeEarlGrey waitForWebViewContainingText:kWindow1Closed];
  GREYAssert(TapWebViewElementWithId(kCheckWindow2Link), @"Failed to tap %s",
             kCheckWindow2Link);
  [ChromeEarlGrey waitForWebViewContainingText:kWindow2Closed];
}

// Tests that closing a tab will set window.closed to true for only
// corresponding window object and not for any other window objects.
- (void)testWindowClosedWithoutName {
  GREYAssert(TapWebViewElementWithId("openWindowNoName"),
             @"Failed to tap \"openWindowNoName\"");
  [ChromeEarlGrey waitForMainTabCount:2];
  chrome_test_util::SelectTabAtIndexInCurrentMode(0);

  // Check that unnamed window 1 is opened and unnamed window 2 isn't.
  const char kCheckWindow1Link[] = "checkUnnamedWindow1Closed";
  [ChromeEarlGrey waitForWebViewContainingText:kCheckWindow1Link];
  GREYAssert(TapWebViewElementWithId(kCheckWindow1Link), @"Failed to tap %s",
             kCheckWindow1Link);
  [ChromeEarlGrey waitForWebViewContainingText:kWindow1Open];
  const char kCheckWindow2Link[] = "checkUnnamedWindow2Closed";
  GREYAssert(TapWebViewElementWithId(kCheckWindow2Link), @"Failed to tap %s",
             kCheckWindow2Link);
  [ChromeEarlGrey waitForWebViewContainingText:kWindow2NeverOpen];

  // Open another unnamed window. Check that unnamed window 2 is now opened.
  GREYAssert(TapWebViewElementWithId("openWindowNoName"),
             @"Failed to tap \"openWindowNoName\"");
  [ChromeEarlGrey waitForMainTabCount:3];
  chrome_test_util::SelectTabAtIndexInCurrentMode(0);
  GREYAssert(TapWebViewElementWithId(kCheckWindow2Link), @"Failed to tap %s",
             kCheckWindow2Link);
  [ChromeEarlGrey waitForWebViewContainingText:kWindow2Open];

  // Close the first opened window. Check that unnamed window 1 is closed and
  // unnamed window 2 is still open.
  chrome_test_util::CloseTabAtIndex(1);
  GREYAssert(TapWebViewElementWithId(kCheckWindow1Link), @"Failed to tap %s",
             kCheckWindow1Link);
  [ChromeEarlGrey waitForWebViewContainingText:kWindow1Closed];
  GREYAssert(TapWebViewElementWithId(kCheckWindow2Link), @"Failed to tap %s",
             kCheckWindow2Link);
  [ChromeEarlGrey waitForWebViewContainingText:kWindow2Open];

  // Close the second opened window. Check that unnamed window 2 is closed.
  chrome_test_util::CloseTabAtIndex(1);
  GREYAssert(TapWebViewElementWithId(kCheckWindow2Link), @"Failed to tap %s",
             kCheckWindow2Link);
  [ChromeEarlGrey waitForWebViewContainingText:kWindow2Closed];
}

@end
