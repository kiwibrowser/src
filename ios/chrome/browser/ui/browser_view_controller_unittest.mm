// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <Foundation/Foundation.h>
#import <PassKit/PassKit.h>

#include <memory>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/path_service.h"
#include "base/strings/sys_string_conversions.h"
#include "base/test/scoped_feature_list.h"
#include "components/bookmarks/test/bookmark_test_helpers.h"
#include "components/payments/core/features.h"
#include "components/prefs/testing_pref_service.h"
#include "components/search_engines/template_url_service.h"
#include "components/sessions/core/tab_restore_service.h"
#include "components/toolbar/test_toolbar_model.h"
#include "ios/chrome/browser/bookmarks/bookmark_model_factory.h"
#import "ios/chrome/browser/browser_state/test_chrome_browser_state.h"
#include "ios/chrome/browser/chrome_paths.h"
#include "ios/chrome/browser/chrome_url_constants.h"
#include "ios/chrome/browser/favicon/ios_chrome_large_icon_service_factory.h"
#include "ios/chrome/browser/search_engines/template_url_service_factory.h"
#include "ios/chrome/browser/sessions/ios_chrome_tab_restore_service_factory.h"
#import "ios/chrome/browser/snapshots/snapshot_tab_helper.h"
#import "ios/chrome/browser/tabs/tab.h"
#import "ios/chrome/browser/tabs/tab_model.h"
#import "ios/chrome/browser/tabs/tab_model_observer.h"
#import "ios/chrome/browser/tabs/tab_private.h"
#import "ios/chrome/browser/ui/activity_services/share_protocol.h"
#import "ios/chrome/browser/ui/activity_services/share_to_data.h"
#import "ios/chrome/browser/ui/alert_coordinator/alert_coordinator.h"
#import "ios/chrome/browser/ui/browser_view_controller.h"
#import "ios/chrome/browser/ui/browser_view_controller_dependency_factory.h"
#import "ios/chrome/browser/ui/browser_view_controller_helper.h"
#import "ios/chrome/browser/ui/commands/browser_commands.h"
#import "ios/chrome/browser/ui/ntp/new_tab_page_controller.h"
#import "ios/chrome/browser/ui/page_not_available_controller.h"
#import "ios/chrome/browser/ui/toolbar/public/omnibox_focuser.h"
#include "ios/chrome/browser/ui/ui_util.h"
#import "ios/chrome/browser/web/error_page_content.h"
#include "ios/chrome/browser/web_state_list/fake_web_state_list_delegate.h"
#include "ios/chrome/browser/web_state_list/web_state_list.h"
#include "ios/chrome/grit/ios_strings.h"
#include "ios/chrome/test/block_cleanup_test.h"
#include "ios/chrome/test/ios_chrome_scoped_testing_local_state.h"
#include "ios/chrome/test/testing_application_context.h"
#import "ios/net/protocol_handler_util.h"
#import "ios/testing/ocmock_complex_type_helper.h"
#include "ios/web/public/referrer.h"
#include "ios/web/public/test/test_web_thread_bundle.h"
#import "ios/web/public/web_state/ui/crw_native_content_provider.h"
#import "ios/web/web_state/ui/crw_web_controller.h"
#import "ios/web/web_state/web_state_impl.h"
#import "net/base/mac/url_conversions.h"
#include "net/url_request/url_request_test_util.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/gtest_mac.h"
#import "third_party/ocmock/OCMock/OCMock.h"
#include "third_party/ocmock/gtest_support.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/l10n/l10n_util_mac.h"
#include "ui/base/test/ios/ui_image_test_utils.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

using web::NavigationManagerImpl;
using web::WebStateImpl;

@class ToolbarButtonUpdater;

// Private methods in BrowserViewController to test.
@interface BrowserViewController (
    Testing)<CRWNativeContentProvider, TabModelObserver>
- (void)pageLoadStarted:(NSNotification*)notification;
- (void)pageLoadComplete:(NSNotification*)notification;
- (void)tabSelected:(Tab*)tab notifyToolbar:(BOOL)notifyToolbar;
- (void)tabDeselected:(NSNotification*)notification;
- (void)tabCountChanged:(NSNotification*)notification;
@end

@interface BVCTestTabMock : OCMockComplexTypeHelper {
  WebStateImpl* _webState;
}

@property(nonatomic, assign) WebStateImpl* webState;

- (web::NavigationManager*)navigationManager;
- (web::NavigationManagerImpl*)navigationManagerImpl;

@end

@implementation BVCTestTabMock
- (WebStateImpl*)webState {
  return _webState;
}
- (void)setWebState:(WebStateImpl*)webState {
  _webState = webState;
}
- (web::NavigationManager*)navigationManager {
  return &(_webState->GetNavigationManagerImpl());
}
- (web::NavigationManagerImpl*)navigationManagerImpl {
  return &(_webState->GetNavigationManagerImpl());
}
@end

@interface BVCTestTabModel : OCMockComplexTypeHelper
- (instancetype)init NS_DESIGNATED_INITIALIZER;
@property(nonatomic, assign) ios::ChromeBrowserState* browserState;
@end

@implementation BVCTestTabModel {
  FakeWebStateListDelegate _webStateListDelegate;
  std::unique_ptr<WebStateList> _webStateList;
}
@synthesize browserState = _browserState;

- (instancetype)init {
  if ((self = [super
           initWithRepresentedObject:[OCMockObject
                                         niceMockForClass:[TabModel class]]])) {
    _webStateList = std::make_unique<WebStateList>(&_webStateListDelegate);
  }
  return self;
}

- (WebStateList*)webStateList {
  return _webStateList.get();
}
@end

#pragma mark -

namespace {
class BrowserViewControllerTest : public BlockCleanupTest {
 public:
  BrowserViewControllerTest() {}

 protected:
  void SetUp() override {
    BlockCleanupTest::SetUp();
    // Set up a TestChromeBrowserState instance.
    TestChromeBrowserState::Builder test_cbs_builder;
    test_cbs_builder.AddTestingFactory(
        IOSChromeTabRestoreServiceFactory::GetInstance(),
        IOSChromeTabRestoreServiceFactory::GetDefaultFactory());
    test_cbs_builder.AddTestingFactory(
        ios::TemplateURLServiceFactory::GetInstance(),
        ios::TemplateURLServiceFactory::GetDefaultFactory());
    test_cbs_builder.AddTestingFactory(
        IOSChromeLargeIconServiceFactory::GetInstance(),
        IOSChromeLargeIconServiceFactory::GetDefaultFactory());
    chrome_browser_state_ = test_cbs_builder.Build();
    chrome_browser_state_->CreateBookmarkModel(false);
    bookmarks::BookmarkModel* bookmark_model =
        ios::BookmarkModelFactory::GetForBrowserState(
            chrome_browser_state_.get());
    bookmarks::test::WaitForBookmarkModelToLoad(bookmark_model);

    // Set up mock TabModel, Tab, and CRWWebController.
    id tabModel = [[BVCTestTabModel alloc] init];
    [tabModel setBrowserState:chrome_browser_state_.get()];
    id currentTab = [[BVCTestTabMock alloc]
        initWithRepresentedObject:[OCMockObject niceMockForClass:[Tab class]]];

    // Stub methods for TabModel.
    NSUInteger tabCount = 1;
    [[[tabModel stub] andReturnValue:OCMOCK_VALUE(tabCount)] count];
    BOOL enabled = YES;
    [[[tabModel stub] andReturnValue:OCMOCK_VALUE(enabled)] webUsageEnabled];
    [[[tabModel stub] andReturn:currentTab] currentTab];
    [[[tabModel stub] andReturn:currentTab] tabAtIndex:0];
    [[tabModel stub] addObserver:[OCMArg any]];
    [[tabModel stub] removeObserver:[OCMArg any]];
    [[tabModel stub] saveSessionImmediately:NO];
    [[tabModel stub] setCurrentTab:[OCMArg any]];
    [[tabModel stub] closeAllTabs];

    // Stub methods for Tab.
    UIView* dummyView = [[UIView alloc] initWithFrame:CGRectZero];
    [[[currentTab stub] andReturn:dummyView] view];

    web::WebState::CreateParams params(chrome_browser_state_.get());
    std::unique_ptr<web::WebState> webState = web::WebState::Create(params);
    webStateImpl_.reset(static_cast<web::WebStateImpl*>(webState.release()));
    [currentTab setWebState:webStateImpl_.get()];

    SnapshotTabHelper::CreateForWebState(webStateImpl_.get(),
                                         [[NSUUID UUID] UUIDString]);

    id passKitController =
        [OCMockObject niceMockForClass:[PKAddPassesViewController class]];
    passKitViewController_ = passKitController;

    bvcHelper_ = [[BrowserViewControllerHelper alloc] init];

    // Set up a stub dependency factory.
    id factory = [OCMockObject
        mockForClass:[BrowserViewControllerDependencyFactory class]];
    [[[factory stub] andReturn:bvcHelper_] newBrowserViewControllerHelper];

    tabModel_ = tabModel;
    tab_ = currentTab;
    dependencyFactory_ = factory;
    bvc_ = [[BrowserViewController alloc]
                  initWithTabModel:tabModel_
                      browserState:chrome_browser_state_.get()
                 dependencyFactory:factory
        applicationCommandEndpoint:nil];

    // Load TemplateURLService.
    TemplateURLService* template_url_service =
        ios::TemplateURLServiceFactory::GetForBrowserState(
            chrome_browser_state_.get());
    template_url_service->Load();

    // Force the view to load.
    UIWindow* window = [[UIWindow alloc] initWithFrame:CGRectZero];
    [window addSubview:[bvc_ view]];
    window_ = window;
  }

  void TearDown() override {
    [[bvc_ view] removeFromSuperview];
    [bvc_ browserStateDestroyed];
    [bvc_ shutdown];

    BlockCleanupTest::TearDown();
  }

  MOCK_METHOD0(OnCompletionCalled, void());

  web::TestWebThreadBundle thread_bundle_;
  IOSChromeScopedTestingLocalState local_state_;
  std::unique_ptr<TestChromeBrowserState> chrome_browser_state_;
  std::unique_ptr<WebStateImpl> webStateImpl_;
  Tab* tab_;
  TabModel* tabModel_;
  BrowserViewControllerHelper* bvcHelper_;
  PKAddPassesViewController* passKitViewController_;
  OCMockObject* dependencyFactory_;
  BrowserViewController* bvc_;
  UIWindow* window_;
};

TEST_F(BrowserViewControllerTest, TestTabSelected) {
  [bvc_ tabSelected:tab_ notifyToolbar:YES];
  EXPECT_EQ([[tab_ view] superview], [bvc_ contentArea]);
  EXPECT_TRUE(webStateImpl_->IsVisible());
}

TEST_F(BrowserViewControllerTest, TestTabSelectedIsNewTab) {
  id block = [^{
    return GURL(kChromeUINewTabURL);
  } copy];
  id tabMock = (id)tab_;
  [tabMock onSelector:@selector(url) callBlockExpectation:block];
  [bvc_ tabSelected:tab_ notifyToolbar:YES];
  EXPECT_EQ([[tab_ view] superview], [bvc_ contentArea]);
  EXPECT_TRUE(webStateImpl_->IsVisible());
}

TEST_F(BrowserViewControllerTest, TestTabDeselected) {
  [bvc_ tabModel:nil didDeselectTab:tab_];
  EXPECT_FALSE(webStateImpl_->IsVisible());
}

TEST_F(BrowserViewControllerTest, TestNativeContentController) {
  id<CRWNativeContent> controller =
      [bvc_ controllerForURL:GURL(kChromeUINewTabURL)
                    webState:webStateImpl_.get()];
  EXPECT_TRUE(controller != nil);
  EXPECT_TRUE([controller isMemberOfClass:[NewTabPageController class]]);

  controller = [bvc_ controllerForURL:GURL(kChromeUISettingsURL)
                             webState:webStateImpl_.get()];
  EXPECT_TRUE(controller != nil);
  EXPECT_TRUE([controller isMemberOfClass:[PageNotAvailableController class]]);
}

TEST_F(BrowserViewControllerTest, TestErrorController) {
  const GURL badUrl("http://floofywhizbangzzz.com");
  NSString* badURLString = base::SysUTF8ToNSString(badUrl.spec());
  NSDictionary* userInfoDic = [NSDictionary
      dictionaryWithObjectsAndKeys:badURLString,
                                   NSURLErrorFailingURLStringErrorKey,
                                   [NSError errorWithDomain:net::kNSErrorDomain
                                                       code:-104
                                                   userInfo:nil],
                                   NSUnderlyingErrorKey, nil];
  NSError* testError =
      [NSError errorWithDomain:@"testdomain" code:-1 userInfo:userInfoDic];
  id<CRWNativeContent> controller =
      [bvc_ controllerForURL:badUrl withError:testError isPost:NO];
  EXPECT_TRUE(controller != nil);
  EXPECT_TRUE([controller isMemberOfClass:[ErrorPageContent class]]);
}

// TODO(altse): Needs a testing |Profile| that implements AutocompleteClassifier
//             before enabling again.
TEST_F(BrowserViewControllerTest, DISABLED_TestShieldWasTapped) {
  [bvc_.dispatcher focusOmnibox];
  EXPECT_TRUE([[bvc_ typingShield] superview] != nil);
  EXPECT_FALSE([[bvc_ typingShield] isHidden]);
  [bvc_ shieldWasTapped:nil];
  EXPECT_TRUE([[bvc_ typingShield] superview] == nil);
  EXPECT_TRUE([[bvc_ typingShield] isHidden]);
}

// Verifies that editing the omnimbox while the page is loading will stop the
// load on a handset, but not stop the load on a tablet.
TEST_F(BrowserViewControllerTest,
       TestLocationBarBeganEdit_whenPageLoadIsInProgress) {
  OCMockObject* tabMock = static_cast<OCMockObject*>(tab_);

  // Have the TestToolbarModel indicate that a page load is in progress.
  id partialMock = OCMPartialMock(bvcHelper_);
  OCMExpect([partialMock isToolbarLoading:static_cast<web::WebState*>(
                                              [OCMArg anyPointer])])
      .andReturn(YES);

  // The tab should stop loading on iPhones.
  [bvc_ locationBarBeganEdit];
  if (!IsIPadIdiom())
    EXPECT_FALSE(webStateImpl_->IsLoading());

  EXPECT_OCMOCK_VERIFY(tabMock);
}

TEST_F(BrowserViewControllerTest, TestClearPresentedState) {
  EXPECT_CALL(*this, OnCompletionCalled());
  [bvc_ clearPresentedStateWithCompletion:^{
    this->OnCompletionCalled();
  }
                           dismissOmnibox:YES];
}

// Tests for the browser view controller when Payment Request is enabled.
class PaymentRequestBrowserViewControllerTest
    : public BrowserViewControllerTest {
 public:
  PaymentRequestBrowserViewControllerTest() {}

 protected:
  void SetUp() override {
    feature_list_.InitAndEnableFeature(payments::features::kWebPayments);
    BrowserViewControllerTest::SetUp();
  }

  base::test::ScopedFeatureList feature_list_;
};

// Verifies that the controller starts up and shuts down cleanly with Payment
// Request enabled.
TEST_F(PaymentRequestBrowserViewControllerTest, TestStartupAndShutdown) {
  // The body of this test is deliberately left empty.
}

}  // namespace
