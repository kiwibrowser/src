// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/test/earl_grey/chrome_test_case.h"

#import <objc/runtime.h>

#import <EarlGrey/EarlGrey.h>

#include <memory>

#include "base/command_line.h"
#include "base/mac/scoped_block.h"
#include "base/strings/sys_string_conversions.h"
#include "components/signin/core/browser/signin_switches.h"
#import "ios/chrome/test/app/chrome_test_util.h"
#include "ios/chrome/test/app/settings_test_util.h"
#include "ios/chrome/test/app/signin_test_util.h"
#import "ios/chrome/test/app/sync_test_util.h"
#import "ios/chrome/test/app/tab_test_util.h"
#import "ios/web/public/test/http_server/http_server.h"
#include "testing/coverage_util_ios.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

NSString* const kFlakyEarlGreyTestTargetSuffix = @"_flaky_egtests";

// Contains a list of test names that run in multitasking test suite.
NSArray* whiteListedMultitaskingTests = @[
  // Integration tests
  @"testContextMenuOpenInNewTab",        // ContextMenuTestCase
  @"testSwitchToMain",                   // CookiesTestCase
  @"testSwitchToIncognito",              // CookiesTestCase
  @"testFindDefaultFormAssistControls",  // FormInputTestCase
  @"testTabDeletion",                    // TabUsageRecorderTestCase
  @"testAutoTranslate",                  // TranslateTestCase

  // Settings tests
  @"testSignInPopUpAccountOnSyncSettings",   // AccountCollectionsTestCase
  @"testAutofillProfileEditing",             // AutofillSettingsTestCase
  @"testAccessibilityOfBlockPopupSettings",  // BlockPopupsTestCase
  @"testClearCookies",                       // SettingsTestCase
  @"testAccessibilityOfTranslateSettings",   // TranslateUITestCase

  // UI tests
  @"testActivityServiceControllerPrintAfterRedirectionToUnprintablePage",
  // ActivityServiceControllerTestCase
  @"testDismissOnDestroy",                      // AlertCoordinatorTestCase
  @"testAddRemoveBookmark",                     // BookmarksTestCase
  @"testJavaScriptInOmnibox",                   // BrowserViewControllerTestCase
  @"testChooseCastReceiverChooser",             // CastReceiverTestCase
  @"testErrorPage",                             // ErrorPageTestCase
  @"testFindInPage",                            // FindInPageTestCase
  @"testDismissFirstRun",                       // FirstRunTestCase
  @"testLongPDFScroll",                         // FullscreenTestCase
  @"testDeleteHistory",                         // HistoryUITestCase
  @"testInfobarsDismissOnNavigate",             // InfobarTestCase
  @"testShowJavaScriptAlert",                   // JavaScriptDialogTestCase
  @"testKeyboardCommands_RecentTabsPresented",  // KeyboardCommandsTestCase
  @"testAccessibilityOnMostVisited",            // NewTabPageTestCase
  @"testPrintNormalPage",                       // PrintControllerTestCase
  @"testQRScannerUIIsShown",                 // QRScannerViewControllerTestCase
  @"testMarkMixedEntriesRead",               // ReadingListTestCase
  @"testClosedTabAppearsInRecentTabsPanel",  // RecentTabsTableTestCase
  @"testSafeModeSendingCrashReport",         // SafeModeTestCase
  @"testSignInOneUser",          // SigninInteractionControllerTestCase
  @"testSwitchTabs",             // StackViewTestCase
  @"testTabStripSwitchTabs",     // TabStripTestCase
  @"testTabHistoryMenu",         // TabHistoryPopupControllerTestCase
  @"testEnteringTabSwitcher",    // TabSwitcherControllerTestCase
  @"testEnterURL",               // ToolbarTestCase
  @"testOpenAndCloseToolsMenu",  // ToolsPopupMenuTestCase
  @"testUserFeedbackPageOpenPrivacyPolicy",  // UserFeedbackTestCase
  @"testVersion",                            // WebUITestCase
];

const CFTimeInterval kDrainTimeout = 5;

}  // namespace

@interface ChromeTestCase () {
  // Block to be executed during object tearDown.
  ProceduralBlock _tearDownHandler;

  BOOL _isHTTPServerStopped;
  BOOL _isMockAuthenticationDisabled;
  std::unique_ptr<net::EmbeddedTestServer> _testServer;
}

// Cleans up mock authentication.
+ (void)disableMockAuthentication;

// Sets up mock authentication.
+ (void)enableMockAuthentication;

// Stops the HTTP server. Should only be called when the server is running.
+ (void)stopHTTPServer;

// Starts the HTTP server. Should only be called when the server is stopped.
+ (void)startHTTPServer;

// Returns a NSArray of test names in this class that contain the prefix
// "FLAKY_".
+ (NSArray*)flakyTestNames;

// Returns a NSArray of test names in this class for multitasking test suite.
+ (NSArray*)multitaskingTestNames;
@end

@implementation ChromeTestCase

// Overrides testInvocations so the set of tests run can be modified, as
// necessary.
+ (NSArray*)testInvocations {
  NSError* error = nil;
  [[EarlGrey selectElementWithMatcher:grey_systemAlertViewShown()]
      assertWithMatcher:grey_nil()
                  error:&error];
  if (error != nil) {
    NSLog(@"System alert view is present, so skipping all tests!");
#if TARGET_IPHONE_SIMULATOR
    return @[];
#else
    // Invoke XCTFail via call to stubbed out test.
    NSMethodSignature* signature =
        [ChromeTestCase instanceMethodSignatureForSelector:@selector
                        (failAllTestsDueToSystemAlertVisible)];
    NSInvocation* systemAlertTest =
        [NSInvocation invocationWithMethodSignature:signature];
    systemAlertTest.selector = @selector(failAllTestsDueToSystemAlertVisible);
    return @[ systemAlertTest ];
#endif
  }

  // Return specific list of tests based on the target.
  NSString* targetName = [NSBundle mainBundle].infoDictionary[@"CFBundleName"];
  if ([targetName hasSuffix:kFlakyEarlGreyTestTargetSuffix]) {
    // Only run FLAKY_ tests for flaky test suites.
    return [self flakyTestNames];
  } else if ([targetName isEqualToString:@"ios_chrome_multitasking_egtests"]) {
    // Only run white listed tests for the multitasking test suite.
    return [self multitaskingTestNames];
  } else {
    return [super testInvocations];
  }
}

// Set up called once for the class, to dismiss anything displayed on startup
// and revert browser settings to default. It also starts the HTTP server and
// enables mock authentication.
+ (void)setUp {
  [super setUp];
  [[self class] startHTTPServer];
  [[self class] enableMockAuthentication];

  // Sometimes on start up there can be infobars (e.g. restore session), so
  // ensure the UI is in a clean state.
  [self removeAnyOpenMenusAndInfoBars];
  [self closeAllTabs];
  chrome_test_util::SetContentSettingsBlockPopups(CONTENT_SETTING_DEFAULT);

  coverage_util::ConfigureCoverageReportPath();
}

// Tear down called once for the class, to shutdown mock authentication and
// the HTTP server.
+ (void)tearDown {
  [[self class] disableMockAuthentication];
  [[self class] stopHTTPServer];
  [super tearDown];
}

- (net::EmbeddedTestServer*)testServer {
  if (!_testServer) {
    _testServer = std::make_unique<net::EmbeddedTestServer>();
    _testServer->AddDefaultHandlers(base::FilePath(
        FILE_PATH_LITERAL("ios/testing/data/http_server_files/")));
  }
  return _testServer.get();
}

// Set up called once per test, to open a new tab.
- (void)setUp {
  [super setUp];
  _isHTTPServerStopped = NO;
  _isMockAuthenticationDisabled = NO;
  _tearDownHandler = nil;

  chrome_test_util::ResetSigninPromoPreferences();
  chrome_test_util::ResetMockAuthentication();
  chrome_test_util::OpenNewTab();
  [[GREYUIThreadExecutor sharedInstance] drainUntilIdle];
}

// Tear down called once per test, to close all tabs and menus, and clear the
// tracked tests accounts. It also makes sure mock authentication and the HTTP
// server are running.
- (void)tearDown {
  if (_tearDownHandler) {
    _tearDownHandler();
  }

  // Clear any remaining test accounts and signed in users.
  chrome_test_util::SignOutAndClearAccounts();

  // Re-start anything that was disabled this test, so it is running when the
  // next test starts.
  if (_isHTTPServerStopped) {
    [[self class] startHTTPServer];
    _isHTTPServerStopped = NO;
  }
  if (_isMockAuthenticationDisabled) {
    [[self class] enableMockAuthentication];
    _isMockAuthenticationDisabled = NO;
  }

  // Clean up any UI that may remain open so the next test starts in a clean
  // state.
  [[self class] removeAnyOpenMenusAndInfoBars];
  [[self class] closeAllTabs];
  [super tearDown];
}

#pragma mark - Public methods

- (void)setTearDownHandler:(ProceduralBlock)tearDownHandler {
  // Enforce that only one |_tearDownHandler| is set per test.
  DCHECK(!_tearDownHandler);
  _tearDownHandler = [tearDownHandler copy];
}

+ (void)removeAnyOpenMenusAndInfoBars {
  chrome_test_util::RemoveAllInfoBars();
  chrome_test_util::ClearPresentedState();
  // After programatically removing UI elements, allow Earl Grey's
  // UI synchronization to become idle, so subsequent steps won't start before
  // the UI is in a good state.
  [[GREYUIThreadExecutor sharedInstance]
      drainUntilIdleWithTimeout:kDrainTimeout];
}

+ (void)closeAllTabs {
  chrome_test_util::CloseAllTabs();
  [[GREYUIThreadExecutor sharedInstance]
      drainUntilIdleWithTimeout:kDrainTimeout];
}

- (void)disableMockAuthentication {
  // Enforce that disableMockAuthentication can only be called once.
  DCHECK(!_isMockAuthenticationDisabled);
  [[self class] disableMockAuthentication];
  _isMockAuthenticationDisabled = YES;
}

- (void)stopHTTPServer {
  // Enforce that the HTTP server can only be stopped once per test. It should
  // not be stopped if it is not running.
  DCHECK(!_isHTTPServerStopped);
  [[self class] stopHTTPServer];
  _isHTTPServerStopped = YES;
}

#pragma mark - Private methods

+ (void)disableMockAuthentication {
  // Make sure local data is cleared, before disabling mock authentication,
  // where data may be sent to real servers.
  chrome_test_util::SignOutAndClearAccounts();
  chrome_test_util::TearDownFakeSyncServer();
  chrome_test_util::TearDownMockAccountReconcilor();
  chrome_test_util::TearDownMockAuthentication();
}

+ (void)enableMockAuthentication {
  chrome_test_util::SetUpMockAuthentication();
  chrome_test_util::SetUpMockAccountReconcilor();
  chrome_test_util::SetUpFakeSyncServer();
}

+ (void)stopHTTPServer {
  web::test::HttpServer& server = web::test::HttpServer::GetSharedInstance();
  DCHECK(server.IsRunning());
  server.Stop();
}

+ (void)startHTTPServer {
  web::test::HttpServer& server = web::test::HttpServer::GetSharedInstance();
  server.StartOrDie();
}

+ (NSArray*)flakyTestNames {
  const char kFlakyTestPrefix[] = "FLAKY";
  unsigned int count = 0;
  Method* methods = class_copyMethodList(self, &count);
  NSMutableArray* flakyTestNames = [NSMutableArray array];
  for (unsigned int i = 0; i < count; i++) {
    SEL selector = method_getName(methods[i]);
    if (std::string(sel_getName(selector)).find(kFlakyTestPrefix) == 0) {
      NSMethodSignature* methodSignature =
          [self instanceMethodSignatureForSelector:selector];
      NSInvocation* invocation =
          [NSInvocation invocationWithMethodSignature:methodSignature];
      invocation.selector = selector;
      [flakyTestNames addObject:invocation];
    }
  }
  free(methods);
  return flakyTestNames;
}

+ (NSArray*)multitaskingTestNames {
  unsigned int count = 0;
  Method* methods = class_copyMethodList(self, &count);
  NSMutableArray* multitaskingTestNames = [NSMutableArray array];
  for (unsigned int i = 0; i < count; i++) {
    SEL selector = method_getName(methods[i]);
    if ([whiteListedMultitaskingTests
            containsObject:base::SysUTF8ToNSString(sel_getName(selector))]) {
      NSMethodSignature* methodSignature =
          [self instanceMethodSignatureForSelector:selector];
      NSInvocation* invocation =
          [NSInvocation invocationWithMethodSignature:methodSignature];
      invocation.selector = selector;
      [multitaskingTestNames addObject:invocation];
    }
  }
  free(methods);
  return multitaskingTestNames;
}

#pragma mark - Handling system alerts

- (void)failAllTestsDueToSystemAlertVisible {
  XCTFail("System alerts are present on device. Skipping all tests.");
}

@end
