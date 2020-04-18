// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/app/application_delegate/app_state.h"

#import <QuartzCore/QuartzCore.h>

#include <memory>

#include "base/ios/block_types.h"
#include "base/mac/scoped_block.h"
#include "base/synchronization/lock.h"
#import "ios/chrome/app/application_delegate/app_navigation.h"
#import "ios/chrome/app/application_delegate/app_state_testing.h"
#import "ios/chrome/app/application_delegate/browser_launcher.h"
#import "ios/chrome/app/application_delegate/fake_startup_information.h"
#import "ios/chrome/app/application_delegate/memory_warning_helper.h"
#import "ios/chrome/app/application_delegate/metrics_mediator.h"
#import "ios/chrome/app/application_delegate/mock_tab_opener.h"
#import "ios/chrome/app/application_delegate/startup_information.h"
#import "ios/chrome/app/application_delegate/tab_switching.h"
#import "ios/chrome/app/application_delegate/user_activity_handler.h"
#import "ios/chrome/app/main_application_delegate.h"
#import "ios/chrome/app/startup/content_suggestions_scheduler_notifications.h"
#import "ios/chrome/browser/app_startup_parameters.h"
#include "ios/chrome/browser/browser_state/test_chrome_browser_state.h"
#include "ios/chrome/browser/chrome_url_constants.h"
#import "ios/chrome/browser/device_sharing/device_sharing_manager.h"
#include "ios/chrome/browser/experimental_flags.h"
#import "ios/chrome/browser/geolocation/omnibox_geolocation_config.h"
#include "ios/chrome/browser/ntp_snippets/ios_chrome_content_suggestions_service_factory.h"
#import "ios/chrome/browser/tabs/tab_model.h"
#import "ios/chrome/browser/ui/browser_view_controller.h"
#import "ios/chrome/browser/ui/commands/browser_commands.h"
#import "ios/chrome/browser/ui/main/browser_view_information.h"
#import "ios/chrome/browser/ui/safe_mode/safe_mode_coordinator.h"
#import "ios/chrome/browser/ui/settings/settings_navigation_controller.h"
#import "ios/chrome/test/base/scoped_block_swizzler.h"
#include "ios/chrome/test/block_cleanup_test.h"
#include "ios/chrome/test/ios_chrome_scoped_testing_chrome_browser_provider.h"
#include "ios/public/provider/chrome/browser/distribution/app_distribution_provider.h"
#include "ios/public/provider/chrome/browser/test_chrome_browser_provider.h"
#include "ios/public/provider/chrome/browser/user_feedback/test_user_feedback_provider.h"
#import "ios/testing/ocmock_complex_type_helper.h"
#include "ios/web/net/request_tracker_impl.h"
#include "ios/web/public/test/test_web_thread_bundle.h"
#import "third_party/ocmock/OCMock/OCMock.h"
#include "third_party/ocmock/gtest_support.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

#pragma mark - Class definition.

namespace {

// A block that takes self as argument and return a BOOL.
typedef BOOL (^DecisionBlock)(id self);
// A block that takes the arguments of UserActivityHandler's
// +handleStartupParametersWithTabOpener.
typedef void (^HandleStartupParam)(
    id self,
    id<TabOpening> tabOpener,
    id<StartupInformation> startupInformation,
    id<BrowserViewInformation> browserViewInformation);

class FakeAppDistributionProvider : public AppDistributionProvider {
 public:
  FakeAppDistributionProvider() : cancel_called_(false) {}
  ~FakeAppDistributionProvider() override {}

  void CancelDistributionNotifications() override { cancel_called_ = true; }
  bool cancel_called() { return cancel_called_; }

 private:
  bool cancel_called_;
  DISALLOW_COPY_AND_ASSIGN(FakeAppDistributionProvider);
};

class FakeUserFeedbackProvider : public TestUserFeedbackProvider {
 public:
  FakeUserFeedbackProvider() : synchronize_called_(false) {}
  ~FakeUserFeedbackProvider() override {}
  void Synchronize() override { synchronize_called_ = true; }
  bool synchronize_called() { return synchronize_called_; }

 private:
  bool synchronize_called_;
  DISALLOW_COPY_AND_ASSIGN(FakeUserFeedbackProvider);
};

class FakeChromeBrowserProvider : public ios::TestChromeBrowserProvider {
 public:
  FakeChromeBrowserProvider()
      : app_distribution_provider_(
            std::make_unique<FakeAppDistributionProvider>()),
        user_feedback_provider_(std::make_unique<FakeUserFeedbackProvider>()) {}
  ~FakeChromeBrowserProvider() override {}

  AppDistributionProvider* GetAppDistributionProvider() const override {
    return app_distribution_provider_.get();
  }

  UserFeedbackProvider* GetUserFeedbackProvider() const override {
    return user_feedback_provider_.get();
  }

 private:
  std::unique_ptr<FakeAppDistributionProvider> app_distribution_provider_;
  std::unique_ptr<FakeUserFeedbackProvider> user_feedback_provider_;
  DISALLOW_COPY_AND_ASSIGN(FakeChromeBrowserProvider);
};

}  // namespace

class AppStateTest : public BlockCleanupTest {
 protected:
  AppStateTest() {
    browser_launcher_mock_ =
        [OCMockObject mockForProtocol:@protocol(BrowserLauncher)];
    startup_information_mock_ =
        [OCMockObject mockForProtocol:@protocol(StartupInformation)];
    main_application_delegate_ =
        [OCMockObject mockForClass:[MainApplicationDelegate class]];
    window_ = [OCMockObject mockForClass:[UIWindow class]];
    browser_view_information_ =
        [OCMockObject mockForProtocol:@protocol(BrowserViewInformation)];
  }

  void SetUp() override {
    BlockCleanupTest::SetUp();
    TestChromeBrowserState::Builder test_cbs_builder;
    test_cbs_builder.AddTestingFactory(
        IOSChromeContentSuggestionsServiceFactory::GetInstance(),
        IOSChromeContentSuggestionsServiceFactory::GetDefaultFactory());
    browser_state_ = test_cbs_builder.Build();
  }

  void initializeIncognitoBlocker(UIWindow* window) {
    id application = [OCMockObject niceMockForClass:[UIApplication class]];
    id memoryHelper = [OCMockObject mockForClass:[MemoryWarningHelper class]];
    id browserViewInformation =
        [OCMockObject mockForProtocol:@protocol(BrowserViewInformation)];
    id tabModel = [OCMockObject mockForClass:[TabModel class]];

    [[startup_information_mock_ stub] expireFirstUserActionRecorder];
    [[[memoryHelper stub] andReturnValue:@0] foregroundMemoryWarningCount];
    [[[tabModel stub] andReturnValue:@NO] isEmpty];
    [[[browser_view_information_ stub] andReturn:tabModel] otrTabModel];
    stubNullCurrentBrowserState(browserViewInformation);
    [[[browser_view_information_ stub] andReturn:nil] currentBVC];

    swizzleMetricsMediatorDisableReporting();

    [app_state_ applicationDidEnterBackground:application
                                 memoryHelper:memoryHelper
                          tabSwitcherIsActive:YES];

    metrics_mediator_called_ = NO;
  }

  void stubNullCurrentBrowserState(id browserViewInformation) {
    [[[browserViewInformation stub] andDo:^(NSInvocation* invocation) {
      ios::ChromeBrowserState* browserState = nullptr;
      [invocation setReturnValue:&browserState];
    }] currentBrowserState];
  }

  void stubBrowserState(id BVC) {
    [[[BVC stub] andDo:^(NSInvocation* invocation) {
      ios::ChromeBrowserState* browserState = getBrowserState();
      [invocation setReturnValue:&browserState];
    }] browserState];
  }

  void swizzleSafeModeShouldStart(BOOL shouldStart) {
    safe_mode_swizzle_block_ = ^BOOL(id self) {
      return shouldStart;
    };
    safe_mode_swizzler_.reset(new ScopedBlockSwizzler(
        [SafeModeCoordinator class], @selector(shouldStart),
        safe_mode_swizzle_block_));
  }

  void swizzleMetricsMediatorDisableReporting() {
    metrics_mediator_called_ = NO;

    metrics_mediator_swizzle_block_ = ^{
      metrics_mediator_called_ = YES;
    };

    metrics_mediator_swizzler_.reset(new ScopedBlockSwizzler(
        [MetricsMediator class], @selector(disableReporting),
        metrics_mediator_swizzle_block_));
  }

  void swizzleHandleStartupParameters(
      id<TabOpening> expectedTabOpener,
      id<BrowserViewInformation> expectedBrowserViewInformation) {
    handle_startup_swizzle_block_ =
        ^(id self, id<TabOpening> tabOpener,
          id<StartupInformation> startupInformation,
          id<BrowserViewInformation> browserViewInformation) {
          ASSERT_EQ(startup_information_mock_, startupInformation);
          ASSERT_EQ(expectedTabOpener, tabOpener);
          ASSERT_EQ(expectedBrowserViewInformation, browserViewInformation);
        };

    handle_startup_swizzler_.reset(new ScopedBlockSwizzler(
        [UserActivityHandler class],
        @selector(handleStartupParametersWithTabOpener:
                                    startupInformation:
                                browserViewInformation:),
        handle_startup_swizzle_block_));
  }

  AppState* getAppStateWithOpenNTPAndIncognitoBlock(BOOL shouldOpenNTP,
                                                    UIWindow* window) {
    AppState* appState = getAppStateWithRealWindow(window);

    id application = [OCMockObject mockForClass:[UIApplication class]];
    id metricsMediator = [OCMockObject mockForClass:[MetricsMediator class]];
    id memoryHelper = [OCMockObject mockForClass:[MemoryWarningHelper class]];
    id tabOpener = [OCMockObject mockForProtocol:@protocol(TabOpening)];
    id appNavigation = [OCMockObject mockForProtocol:@protocol(AppNavigation)];
    id tabModel = [OCMockObject mockForClass:[TabModel class]];

    [[[browser_view_information_ stub] andReturn:tabModel] currentTabModel];
    [[metricsMediator stub] updateMetricsStateBasedOnPrefsUserTriggered:NO];
    [[memoryHelper stub] resetForegroundMemoryWarningCount];
    [[[memoryHelper stub] andReturnValue:@0] foregroundMemoryWarningCount];
    [[[tabOpener stub] andReturnValue:@(shouldOpenNTP)]
        shouldOpenNTPTabOnActivationOfTabModel:tabModel];

    stubNullCurrentBrowserState(browser_view_information_);

    void (^swizzleBlock)() = ^{
    };

    ScopedBlockSwizzler swizzler(
        [MetricsMediator class],
        @selector(logLaunchMetricsWithStartupInformation:
                                  browserViewInformation:),
        swizzleBlock);

    [appState applicationWillEnterForeground:application
                             metricsMediator:metricsMediator
                                memoryHelper:memoryHelper
                                   tabOpener:tabOpener
                               appNavigation:appNavigation];

    initializeIncognitoBlocker(window);

    return appState;
  }

  AppState* getAppStateWithMock() {
    if (!app_state_) {
      app_state_ =
          [[AppState alloc] initWithBrowserLauncher:browser_launcher_mock_
                                 startupInformation:startup_information_mock_
                                applicationDelegate:main_application_delegate_];
      [app_state_ setWindow:window_];
    }
    return app_state_;
  }

  AppState* getAppStateWithRealWindow(UIWindow* window) {
    if (!app_state_) {
      app_state_ =
          [[AppState alloc] initWithBrowserLauncher:browser_launcher_mock_
                                 startupInformation:startup_information_mock_
                                applicationDelegate:main_application_delegate_];
      [app_state_ setWindow:window];
    }
    return app_state_;
  }

  id getBrowserLauncherMock() { return browser_launcher_mock_; }
  id getStartupInformationMock() { return startup_information_mock_; }
  id getApplicationDelegateMock() { return main_application_delegate_; }
  id getWindowMock() { return window_; }
  id getBrowserViewInformationMock() { return browser_view_information_; }
  ios::ChromeBrowserState* getBrowserState() { return browser_state_.get(); }

  BOOL metricsMediatorHasBeenCalled() { return metrics_mediator_called_; }

 private:
  web::TestWebThreadBundle thread_bundle_;
  AppState* app_state_;
  id browser_launcher_mock_;
  id startup_information_mock_;
  id main_application_delegate_;
  id window_;
  id browser_view_information_;
  DecisionBlock safe_mode_swizzle_block_;
  HandleStartupParam handle_startup_swizzle_block_;
  ProceduralBlock metrics_mediator_swizzle_block_;
  std::unique_ptr<ScopedBlockSwizzler> safe_mode_swizzler_;
  std::unique_ptr<ScopedBlockSwizzler> handle_startup_swizzler_;
  std::unique_ptr<ScopedBlockSwizzler> metrics_mediator_swizzler_;
  __block BOOL metrics_mediator_called_;
  std::unique_ptr<TestChromeBrowserState> browser_state_;
};

// TODO(crbug.com/585700): remove this.
// Creates a requestTracker, needed for teardown.
void createTracker(BOOL* created, base::Lock* lock) {
  web::RequestTrackerImpl::GetTrackerForRequestGroupID(@"test");
  base::AutoLock scoped_lock(*lock);
  *created = YES;
}

// Used to have a thread handling the closing of the IO threads.
class AppStateWithThreadTest : public PlatformTest {
 protected:
  AppStateWithThreadTest()
      : thread_bundle_(web::TestWebThreadBundle::REAL_IO_THREAD) {
    BOOL created = NO;
    base::Lock* lock = new base::Lock;

    web::WebThread::PostTask(web::WebThread::IO, FROM_HERE,
                             base::Bind(&createTracker, &created, lock));

    CFTimeInterval start = CACurrentMediaTime();

    // Poll for at most 1s, waiting for the Tracker creation.
    while (1) {
      base::AutoLock scoped_lock(*lock);
      if (created)
        return;
      if (CACurrentMediaTime() - start > 1.0) {
        trackerCreationFailed();
        return;
      }
      // Ensure that other threads have a chance to run even on a single-core
      // devices.
      pthread_yield_np();
    }
  }

  void trackerCreationFailed() {
    FAIL() << "Tracker creation took too much time.";
  }

 private:
  web::TestWebThreadBundle thread_bundle_;
};

#pragma mark - Tests.

// Tests -isInSafeMode returns true if there is a SafeModeController.
TEST_F(AppStateTest, isInSafeModeTest) {
  // Setup.
  id safeModeContollerMock =
      [OCMockObject mockForClass:[SafeModeCoordinator class]];

  AppState* appState = getAppStateWithMock();

  appState.safeModeCoordinator = nil;
  ASSERT_FALSE([appState isInSafeMode]);
  [appState setSafeModeCoordinator:safeModeContollerMock];

  // Action.
  BOOL result = [appState isInSafeMode];

  // Test.
  EXPECT_TRUE(result);
}

// Tests that if the application is in background
// -requiresHandlingAfterLaunchWithOptions saves the launchOptions and returns
// YES (to handle the launch options later).
TEST_F(AppStateTest, requiresHandlingAfterLaunchWithOptionsBackground) {
  // Setup.
  NSString* sourceApplication = @"com.apple.mobilesafari";
  NSDictionary* launchOptions =
      @{UIApplicationLaunchOptionsSourceApplicationKey : sourceApplication};

  AppState* appState = getAppStateWithMock();

  id browserLauncherMock = getBrowserLauncherMock();
  BrowserInitializationStageType stageBasic = INITIALIZATION_STAGE_BASIC;
  [[browserLauncherMock expect] startUpBrowserToStage:stageBasic];
  [[browserLauncherMock expect] setLaunchOptions:launchOptions];

  // Action.
  BOOL result = [appState requiresHandlingAfterLaunchWithOptions:launchOptions
                                                 stateBackground:YES];

  // Test.
  EXPECT_TRUE(result);
  EXPECT_OCMOCK_VERIFY(browserLauncherMock);
}

// Tests that if the application is active and Safe Mode should be activated
// -requiresHandlingAfterLaunchWithOptions save the launch options and activate
// the Safe Mode.
TEST_F(AppStateTest, requiresHandlingAfterLaunchWithOptionsForegroundSafeMode) {
  // Setup.
  NSString* sourceApplication = @"com.apple.mobilesafari";
  NSDictionary* launchOptions =
      @{UIApplicationLaunchOptionsSourceApplicationKey : sourceApplication};

  id windowMock = getWindowMock();
  [[[windowMock stub] andReturn:nil] rootViewController];
  [[windowMock expect] setRootViewController:[OCMArg any]];
  [[windowMock expect] makeKeyAndVisible];

  AppState* appState = getAppStateWithMock();

  id browserLauncherMock = getBrowserLauncherMock();
  BrowserInitializationStageType stageBasic = INITIALIZATION_STAGE_BASIC;
  [[browserLauncherMock expect] startUpBrowserToStage:stageBasic];
  [[browserLauncherMock expect] setLaunchOptions:launchOptions];

  swizzleSafeModeShouldStart(YES);

  ASSERT_FALSE([appState isInSafeMode]);

  // Action.
  BOOL result = [appState requiresHandlingAfterLaunchWithOptions:launchOptions
                                                 stateBackground:NO];

  // Test.
  EXPECT_TRUE(result);
  EXPECT_TRUE([appState isInSafeMode]);
  EXPECT_OCMOCK_VERIFY(browserLauncherMock);
  EXPECT_OCMOCK_VERIFY(windowMock);
}

// Tests that if the application is active
// -requiresHandlingAfterLaunchWithOptions saves the launchOptions and start the
// application in foreground.
TEST_F(AppStateTest, requiresHandlingAfterLaunchWithOptionsForeground) {
  // Setup.
  NSString* sourceApplication = @"com.apple.mobilesafari";
  NSDictionary* launchOptions =
      @{UIApplicationLaunchOptionsSourceApplicationKey : sourceApplication};

  [[[getStartupInformationMock() stub] andReturnValue:@YES] isColdStart];

  [[[getWindowMock() stub] andReturn:nil] rootViewController];

  AppState* appState = getAppStateWithMock();

  id browserLauncherMock = getBrowserLauncherMock();
  BrowserInitializationStageType stageBasic = INITIALIZATION_STAGE_BASIC;
  [[browserLauncherMock expect] startUpBrowserToStage:stageBasic];
  BrowserInitializationStageType stageForeground =
      INITIALIZATION_STAGE_FOREGROUND;
  [[browserLauncherMock expect] startUpBrowserToStage:stageForeground];
  [[browserLauncherMock expect] setLaunchOptions:launchOptions];

  swizzleSafeModeShouldStart(NO);

  // Action.
  BOOL result = [appState requiresHandlingAfterLaunchWithOptions:launchOptions
                                                 stateBackground:NO];

  // Test.
  EXPECT_TRUE(result);
  EXPECT_OCMOCK_VERIFY(browserLauncherMock);
}

using AppStateNoFixtureTest = PlatformTest;

// Test that -willResignActive set cold start to NO and launch record.
TEST_F(AppStateNoFixtureTest, willResignActive) {
  // Setup.
  id tabModel = [OCMockObject mockForClass:[TabModel class]];
  [[tabModel expect] recordSessionMetrics];

  id browserViewInformation =
      [OCMockObject mockForProtocol:@protocol(BrowserViewInformation)];

  [[[browserViewInformation stub] andReturn:tabModel] mainTabModel];

  id browserLauncher =
      [OCMockObject mockForProtocol:@protocol(BrowserLauncher)];
  [[[browserLauncher stub] andReturnValue:@(INITIALIZATION_STAGE_FOREGROUND)]
      browserInitializationStage];
  [[[browserLauncher stub] andReturn:browserViewInformation]
      browserViewInformation];

  id applicationDelegate =
      [OCMockObject mockForClass:[MainApplicationDelegate class]];
  id window = [OCMockObject mockForClass:[UIWindow class]];

  FakeStartupInformation* startupInformation =
      [[FakeStartupInformation alloc] init];
  [startupInformation setIsColdStart:YES];

  AppState* appState =
      [[AppState alloc] initWithBrowserLauncher:browserLauncher
                             startupInformation:startupInformation
                            applicationDelegate:applicationDelegate];
  [appState setWindow:window];

  ASSERT_TRUE([startupInformation isColdStart]);

  // Action.
  [appState willResignActiveTabModel];

  // Test.
  EXPECT_FALSE([startupInformation isColdStart]);
  EXPECT_OCMOCK_VERIFY(tabModel);
}

// Test that -applicationWillTerminate clears everything.
TEST_F(AppStateWithThreadTest, willTerminate) {
  // Setup.
  IOSChromeScopedTestingChromeBrowserProvider provider_(
      std::make_unique<FakeChromeBrowserProvider>());

  id browserViewController = OCMClassMock([BrowserViewController class]);
  OCMExpect([browserViewController setActive:NO]);
  id browserLauncher =
      [OCMockObject mockForProtocol:@protocol(BrowserLauncher)];
  id applicationDelegate =
      [OCMockObject mockForClass:[MainApplicationDelegate class]];
  id window = [OCMockObject mockForClass:[UIWindow class]];
  id browserViewInformation =
      [OCMockObject mockForProtocol:@protocol(BrowserViewInformation)];
  id mainTabModel = [OCMockObject mockForClass:[TabModel class]];
  id OTRTabModel = [OCMockObject mockForClass:[TabModel class]];
  [[[browserLauncher stub] andReturnValue:@(INITIALIZATION_STAGE_FOREGROUND)]
      browserInitializationStage];
  [[[browserLauncher stub] andReturn:browserViewInformation]
      browserViewInformation];
  [[[browserViewInformation stub] andReturn:mainTabModel] mainTabModel];
  [[[browserViewInformation stub] andReturn:OTRTabModel] otrTabModel];
  [[[browserViewInformation stub] andReturn:browserViewController] currentBVC];

  id settingsNavigationController =
      [OCMockObject mockForClass:[SettingsNavigationController class]];

  id appNavigation = [OCMockObject mockForProtocol:@protocol(AppNavigation)];
  [[[appNavigation stub] andReturn:settingsNavigationController]
      settingsNavigationController];
  [[appNavigation expect] closeSettingsAnimated:NO completion:nil];

  [[browserViewInformation expect] cleanDeviceSharingManager];

  [[mainTabModel expect] haltAllTabs];

  [[OTRTabModel expect] closeAllTabs];
  [[OTRTabModel expect] saveSessionImmediately:YES];

  id startupInformation =
      [OCMockObject mockForProtocol:@protocol(StartupInformation)];
  [[startupInformation expect] stopChromeMain];

  AppState* appState =
      [[AppState alloc] initWithBrowserLauncher:browserLauncher
                             startupInformation:startupInformation
                            applicationDelegate:applicationDelegate];
  [appState setWindow:window];

  id application = [OCMockObject mockForClass:[UIApplication class]];

  // Action.
  [appState applicationWillTerminate:application
               applicationNavigation:appNavigation];

  // Test.
  EXPECT_OCMOCK_VERIFY(OTRTabModel);
  EXPECT_OCMOCK_VERIFY(mainTabModel);
  EXPECT_OCMOCK_VERIFY(browserViewController);
  EXPECT_OCMOCK_VERIFY(startupInformation);
  EXPECT_OCMOCK_VERIFY(appNavigation);
  EXPECT_OCMOCK_VERIFY(browserViewInformation);
  EXPECT_OCMOCK_VERIFY(application);
  FakeAppDistributionProvider* provider =
      static_cast<FakeAppDistributionProvider*>(
          ios::GetChromeBrowserProvider()->GetAppDistributionProvider());
  EXPECT_TRUE(provider->cancel_called());
}

// Test that -resumeSessionWithTabOpener removes incognito blocker,
// restart metrics and launchs from StartupParameters if they exist.
TEST_F(AppStateTest, resumeSessionWithStartupParameters) {
  // Setup.

  // BrowserLauncher.
  id browserViewInformation = getBrowserViewInformationMock();
  [[[getBrowserLauncherMock() stub]
      andReturnValue:@(INITIALIZATION_STAGE_FOREGROUND)]
      browserInitializationStage];
  [[[getBrowserLauncherMock() stub] andReturn:browserViewInformation]
      browserViewInformation];

  // StartupInformation.
  id appStartupParameters =
      [OCMockObject mockForClass:[AppStartupParameters class]];
  [[[getStartupInformationMock() stub] andReturn:appStartupParameters]
      startupParameters];
  [[[getStartupInformationMock() stub] andReturnValue:@NO] isColdStart];

  // TabOpening.
  id tabOpener = [OCMockObject mockForProtocol:@protocol(TabOpening)];
  // TabSwitcher.
  id tabSwitcher = [OCMockObject mockForProtocol:@protocol(TabSwitching)];

  // BrowserViewInformation.
  id mainTabModel = [OCMockObject mockForClass:[TabModel class]];
  [[mainTabModel expect] resetSessionMetrics];
  [[[browserViewInformation stub] andReturn:mainTabModel] mainTabModel];
  id mainBVC = [OCMockObject mockForClass:[BrowserViewController class]];
  stubBrowserState(mainBVC);
  [[[browserViewInformation stub] andReturn:mainBVC] mainBVC];

  // Swizzle Startup Parameters.
  swizzleHandleStartupParameters(tabOpener, browserViewInformation);

  UIWindow* window = [[UIWindow alloc] init];
  AppState* appState = getAppStateWithOpenNTPAndIncognitoBlock(NO, window);

  ASSERT_EQ(NSUInteger(1), [window subviews].count);

  // Action.
  [appState resumeSessionWithTabOpener:tabOpener tabSwitcher:tabSwitcher];

  // Test.
  EXPECT_EQ(NSUInteger(0), [window subviews].count);
  EXPECT_OCMOCK_VERIFY(mainTabModel);
}

// Test that -resumeSessionWithTabOpener removes incognito blocker,
// restart metrics and creates a new tab from tab switcher if shouldOpenNTP is
// YES.
TEST_F(AppStateTest, resumeSessionShouldOpenNTPTabSwitcher) {
  // Setup.
  // BrowserLauncher.
  id browserViewInformation = getBrowserViewInformationMock();
  [[[getBrowserLauncherMock() stub]
      andReturnValue:@(INITIALIZATION_STAGE_FOREGROUND)]
      browserInitializationStage];
  [[[getBrowserLauncherMock() stub] andReturn:browserViewInformation]
      browserViewInformation];

  // StartupInformation.
  [[[getStartupInformationMock() stub] andReturn:nil] startupParameters];
  [[[getStartupInformationMock() stub] andReturnValue:@NO] isColdStart];

  // TabOpening.
  id tabOpener = [OCMockObject mockForProtocol:@protocol(TabOpening)];

  // BrowserViewInformation.
  id mainTabModel = [OCMockObject mockForClass:[TabModel class]];
  [[mainTabModel expect] resetSessionMetrics];
  [[[browserViewInformation stub] andReturn:mainTabModel] mainTabModel];

  // TabSwitcher.
  id tabSwitcher = [OCMockObject mockForProtocol:@protocol(TabSwitching)];
  [[[tabSwitcher stub] andReturnValue:@YES] openNewTabFromTabSwitcher];

  id mainBVC = [OCMockObject mockForClass:[BrowserViewController class]];
  stubBrowserState(mainBVC);
  [[[browserViewInformation stub] andReturn:mainBVC] mainBVC];

  UIWindow* window = [[UIWindow alloc] init];
  AppState* appState = getAppStateWithOpenNTPAndIncognitoBlock(YES, window);

  ASSERT_EQ(NSUInteger(1), [window subviews].count);

  // Action.
  [appState resumeSessionWithTabOpener:tabOpener tabSwitcher:tabSwitcher];

  // Test.
  EXPECT_EQ(NSUInteger(0), [window subviews].count);
  EXPECT_OCMOCK_VERIFY(mainTabModel);
}

// Test that -resumeSessionWithTabOpener removes incognito blocker,
// restart metrics and creates a new tab if shouldOpenNTP is YES.
TEST_F(AppStateTest, resumeSessionShouldOpenNTPNoTabSwitcher) {
  // Setup.
  // BrowserLauncher.
  id browserViewInformation = getBrowserViewInformationMock();
  [[[getBrowserLauncherMock() stub]
      andReturnValue:@(INITIALIZATION_STAGE_FOREGROUND)]
      browserInitializationStage];
  [[[getBrowserLauncherMock() stub] andReturn:browserViewInformation]
      browserViewInformation];

  // StartupInformation.
  [[[getStartupInformationMock() stub] andReturn:nil] startupParameters];
  [[[getStartupInformationMock() stub] andReturnValue:@NO] isColdStart];

  // TabOpening.
  id tabOpener = [OCMockObject mockForProtocol:@protocol(TabOpening)];

  // BrowserViewInformation.
  id mainTabModel = [OCMockObject mockForClass:[TabModel class]];
  [[mainTabModel expect] resetSessionMetrics];

  id dispatcher = [OCMockObject mockForProtocol:@protocol(BrowserCommands)];
  [[dispatcher expect] openNewTab:[OCMArg any]];

  id currentBVC = [OCMockObject mockForClass:[BrowserViewController class]];
  stubBrowserState(currentBVC);
  [[[currentBVC stub] andReturn:dispatcher] dispatcher];

  [[[browserViewInformation stub] andReturn:mainTabModel] mainTabModel];
  [[[browserViewInformation stub] andReturn:currentBVC] currentBVC];
  [[[browserViewInformation stub] andReturn:nil] otrBVC];

  // TabSwitcher.
  id tabSwitcher = [OCMockObject mockForProtocol:@protocol(TabSwitching)];
  [[[tabSwitcher stub] andReturnValue:@NO] openNewTabFromTabSwitcher];

  id mainBVC = [OCMockObject mockForClass:[BrowserViewController class]];
  stubBrowserState(mainBVC);
  [[[browserViewInformation stub] andReturn:mainBVC] mainBVC];

  UIWindow* window = [[UIWindow alloc] init];
  AppState* appState = getAppStateWithOpenNTPAndIncognitoBlock(YES, window);

  // incognitoBlocker.
  ASSERT_EQ(NSUInteger(1), [window subviews].count);

  // Action.
  [appState resumeSessionWithTabOpener:tabOpener tabSwitcher:tabSwitcher];

  // Test.
  EXPECT_EQ(NSUInteger(0), [window subviews].count);
  EXPECT_OCMOCK_VERIFY(mainTabModel);
  EXPECT_OCMOCK_VERIFY(currentBVC);
}

// Tests that -applicationWillEnterForeground resets components as needed.
TEST_F(AppStateTest, applicationWillEnterForeground) {
  // Setup.
  IOSChromeScopedTestingChromeBrowserProvider provider_(
      std::make_unique<FakeChromeBrowserProvider>());
  id application = [OCMockObject mockForClass:[UIApplication class]];
  id metricsMediator = [OCMockObject mockForClass:[MetricsMediator class]];
  id memoryHelper = [OCMockObject mockForClass:[MemoryWarningHelper class]];
  id browserViewInformation = getBrowserViewInformationMock();
  id tabOpener = [OCMockObject mockForProtocol:@protocol(TabOpening)];
  id appNavigation = [OCMockObject mockForProtocol:@protocol(AppNavigation)];
  id tabModel = [OCMockObject mockForClass:[TabModel class]];

  BrowserInitializationStageType stage = INITIALIZATION_STAGE_FOREGROUND;
  [[[getBrowserLauncherMock() stub] andReturnValue:@(stage)]
      browserInitializationStage];
  [[[getBrowserLauncherMock() stub] andReturn:browserViewInformation]
      browserViewInformation];
  [[[browserViewInformation stub] andReturn:tabModel] currentTabModel];
  id mainBVC = [OCMockObject mockForClass:[BrowserViewController class]];
  stubBrowserState(mainBVC);
  [[[browserViewInformation stub] andReturn:mainBVC] mainBVC];
  [[metricsMediator expect] updateMetricsStateBasedOnPrefsUserTriggered:NO];
  [[memoryHelper expect] resetForegroundMemoryWarningCount];
  [[[memoryHelper stub] andReturnValue:@0] foregroundMemoryWarningCount];
  [[[tabOpener stub] andReturnValue:@YES]
      shouldOpenNTPTabOnActivationOfTabModel:tabModel];

  id contentSuggestionsNotifier =
      OCMClassMock([ContentSuggestionsSchedulerNotifications class]);
  OCMExpect([contentSuggestionsNotifier notifyForeground:getBrowserState()]);

  stubNullCurrentBrowserState(browserViewInformation);

  void (^swizzleBlock)() = ^{
  };

  ScopedBlockSwizzler swizzler(
      [MetricsMediator class],
      @selector(logLaunchMetricsWithStartupInformation:browserViewInformation:),
      swizzleBlock);

  // Actions.
  [getAppStateWithMock() applicationWillEnterForeground:application
                                        metricsMediator:metricsMediator
                                           memoryHelper:memoryHelper
                                              tabOpener:tabOpener
                                          appNavigation:appNavigation];

  // Tests.
  EXPECT_OCMOCK_VERIFY(metricsMediator);
  EXPECT_OCMOCK_VERIFY(memoryHelper);
  FakeUserFeedbackProvider* user_feedback_provider =
      static_cast<FakeUserFeedbackProvider*>(
          ios::GetChromeBrowserProvider()->GetUserFeedbackProvider());
  EXPECT_TRUE(user_feedback_provider->synchronize_called());
  EXPECT_OCMOCK_VERIFY(contentSuggestionsNotifier);
}

// Tests that -applicationWillEnterForeground starts the browser if the
// application is in background.
TEST_F(AppStateTest, applicationWillEnterForegroundFromBackground) {
  // Setup.
  id application = [OCMockObject mockForClass:[UIApplication class]];
  id metricsMediator = [OCMockObject mockForClass:[MetricsMediator class]];
  id memoryHelper = [OCMockObject mockForClass:[MemoryWarningHelper class]];
  id tabOpener = [OCMockObject mockForProtocol:@protocol(TabOpening)];
  id appNavigation = [OCMockObject mockForProtocol:@protocol(AppNavigation)];

  BrowserInitializationStageType stage = INITIALIZATION_STAGE_BACKGROUND;
  [[[getBrowserLauncherMock() stub] andReturnValue:@(stage)]
      browserInitializationStage];

  [[[getWindowMock() stub] andReturn:nil] rootViewController];
  swizzleSafeModeShouldStart(NO);

  [[[getStartupInformationMock() stub] andReturnValue:@YES] isColdStart];
  [[getBrowserLauncherMock() expect]
      startUpBrowserToStage:INITIALIZATION_STAGE_FOREGROUND];

  // Actions.
  [getAppStateWithMock() applicationWillEnterForeground:application
                                        metricsMediator:metricsMediator
                                           memoryHelper:memoryHelper
                                              tabOpener:tabOpener
                                          appNavigation:appNavigation];

  // Tests.
  EXPECT_OCMOCK_VERIFY(getBrowserLauncherMock());
}

// Tests that -applicationWillEnterForeground starts the safe mode if the
// application is in background.
TEST_F(AppStateTest,
       applicationWillEnterForegroundFromBackgroundShouldStartSafeMode) {
  // Setup.
  id application = [OCMockObject mockForClass:[UIApplication class]];
  id metricsMediator = [OCMockObject mockForClass:[MetricsMediator class]];
  id memoryHelper = [OCMockObject mockForClass:[MemoryWarningHelper class]];
  id tabOpener = [OCMockObject mockForProtocol:@protocol(TabOpening)];
  id appNavigation = [OCMockObject mockForProtocol:@protocol(AppNavigation)];

  id window = getWindowMock();

  BrowserInitializationStageType stage = INITIALIZATION_STAGE_BACKGROUND;
  [[[getBrowserLauncherMock() stub] andReturnValue:@(stage)]
      browserInitializationStage];

  [[[window stub] andReturn:nil] rootViewController];
  [[window expect] makeKeyAndVisible];
  [[window stub] setRootViewController:[OCMArg any]];
  swizzleSafeModeShouldStart(YES);

  // Actions.
  [getAppStateWithMock() applicationWillEnterForeground:application
                                        metricsMediator:metricsMediator
                                           memoryHelper:memoryHelper
                                              tabOpener:tabOpener
                                          appNavigation:appNavigation];

  // Tests.
  EXPECT_OCMOCK_VERIFY(window);
  EXPECT_TRUE([getAppStateWithMock() isInSafeMode]);
}

// Tests that -applicationWillEnterForeground returns directly if the
// application is in safe mode and in foreground
TEST_F(AppStateTest, applicationWillEnterForegroundFromForegroundSafeMode) {
  // Setup.
  id application = [OCMockObject mockForClass:[UIApplication class]];
  id metricsMediator = [OCMockObject mockForClass:[MetricsMediator class]];
  id memoryHelper = [OCMockObject mockForClass:[MemoryWarningHelper class]];
  id tabOpener = [OCMockObject mockForProtocol:@protocol(TabOpening)];
  id appNavigation = [OCMockObject mockForProtocol:@protocol(AppNavigation)];

  BrowserInitializationStageType stage = INITIALIZATION_STAGE_FOREGROUND;
  [[[getBrowserLauncherMock() stub] andReturnValue:@(stage)]
      browserInitializationStage];

  AppState* appState = getAppStateWithMock();

  UIWindow* window = [[UIWindow alloc] init];
  appState.safeModeCoordinator =
      [[SafeModeCoordinator alloc] initWithWindow:window];

  ASSERT_TRUE([appState isInSafeMode]);

  // Actions.
  [appState applicationWillEnterForeground:application
                           metricsMediator:metricsMediator
                              memoryHelper:memoryHelper
                                 tabOpener:tabOpener
                             appNavigation:appNavigation];
}

// Tests that -applicationDidEnterBackground creates an incognito blocker.
TEST_F(AppStateTest, applicationDidEnterBackgroundIncognito) {
  // Setup.
  UIWindow* window = [[UIWindow alloc] init];
  id application = [OCMockObject niceMockForClass:[UIApplication class]];
  id memoryHelper = [OCMockObject mockForClass:[MemoryWarningHelper class]];
  id browserViewInformation = getBrowserViewInformationMock();
  id tabModel = [OCMockObject mockForClass:[TabModel class]];
  id startupInformation = getStartupInformationMock();
  id browserLauncher = getBrowserLauncherMock();
  BrowserInitializationStageType stage = INITIALIZATION_STAGE_FOREGROUND;

  AppState* appState = getAppStateWithRealWindow(window);

  [[startupInformation expect] expireFirstUserActionRecorder];
  [[[memoryHelper stub] andReturnValue:@0] foregroundMemoryWarningCount];
  [[[tabModel stub] andReturnValue:@NO] isEmpty];
  [[[browserViewInformation stub] andReturn:tabModel] otrTabModel];
  [[[browserViewInformation stub] andReturn:nil] currentBVC];
  stubNullCurrentBrowserState(browserViewInformation);
  [[[browserLauncher stub] andReturnValue:@(stage)] browserInitializationStage];
  [[[browserLauncher stub] andReturn:browserViewInformation]
      browserViewInformation];

  swizzleMetricsMediatorDisableReporting();

  ASSERT_EQ(NSUInteger(0), [window subviews].count);

  // Action.
  [appState applicationDidEnterBackground:application
                             memoryHelper:memoryHelper
                      tabSwitcherIsActive:YES];

  // Tests.
  EXPECT_OCMOCK_VERIFY(startupInformation);
  EXPECT_TRUE(metricsMediatorHasBeenCalled());
  EXPECT_EQ(NSUInteger(1), [window subviews].count);
}

// Tests that -applicationDidEnterBackground do nothing if the application has
// never been in a Foreground stage.
TEST_F(AppStateTest, applicationDidEnterBackgroundStageBackground) {
  // Setup.
  UIWindow* window = [[UIWindow alloc] init];
  id application = [OCMockObject mockForClass:[UIApplication class]];
  id memoryHelper = [OCMockObject mockForClass:[MemoryWarningHelper class]];
  id browserLauncher = getBrowserLauncherMock();
  BrowserInitializationStageType stage = INITIALIZATION_STAGE_BACKGROUND;

  [[[browserLauncher stub] andReturnValue:@(stage)] browserInitializationStage];

  ASSERT_EQ(NSUInteger(0), [window subviews].count);

  // Action.
  [getAppStateWithRealWindow(window) applicationDidEnterBackground:application
                                                      memoryHelper:memoryHelper
                                               tabSwitcherIsActive:YES];

  // Tests.
  EXPECT_EQ(NSUInteger(0), [window subviews].count);
}

// Tests that -applicationDidEnterBackground does not create an incognito
// blocker if there is no incognito tab.
TEST_F(AppStateTest, applicationDidEnterBackgroundNoIncognitoBlocker) {
  // Setup.
  UIWindow* window = [[UIWindow alloc] init];
  id application = [OCMockObject niceMockForClass:[UIApplication class]];
  id memoryHelper = [OCMockObject mockForClass:[MemoryWarningHelper class]];
  id browserViewInformation = getBrowserViewInformationMock();
  id tabModel = [OCMockObject mockForClass:[TabModel class]];
  id startupInformation = getStartupInformationMock();
  id browserLauncher = getBrowserLauncherMock();
  BrowserInitializationStageType stage = INITIALIZATION_STAGE_FOREGROUND;

  AppState* appState = getAppStateWithRealWindow(window);

  [[startupInformation expect] expireFirstUserActionRecorder];
  [[[memoryHelper stub] andReturnValue:@0] foregroundMemoryWarningCount];
  [[[tabModel stub] andReturnValue:@YES] isEmpty];
  [[[browserViewInformation stub] andReturn:tabModel] otrTabModel];
  [[[browserViewInformation stub] andReturn:nil] currentBVC];
  stubNullCurrentBrowserState(browserViewInformation);
  [[[browserLauncher stub] andReturnValue:@(stage)] browserInitializationStage];
  [[[browserLauncher stub] andReturn:browserViewInformation]
      browserViewInformation];

  swizzleMetricsMediatorDisableReporting();

  ASSERT_EQ(NSUInteger(0), [window subviews].count);

  // Action.
  [appState applicationDidEnterBackground:application
                             memoryHelper:memoryHelper
                      tabSwitcherIsActive:YES];

  // Tests.
  EXPECT_OCMOCK_VERIFY(startupInformation);
  EXPECT_TRUE(metricsMediatorHasBeenCalled());
  EXPECT_EQ(NSUInteger(0), [window subviews].count);
}
