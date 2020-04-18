// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/tabs/tab_model_favicon_driver_observer.h"

#include "base/logging.h"
#include "base/scoped_observer.h"
#include "components/favicon/ios/web_favicon_driver.h"
#include "ios/chrome/browser/browser_state/test_chrome_browser_state.h"
#import "ios/chrome/browser/tabs/legacy_tab_helper.h"
#import "ios/chrome/browser/tabs/tab_model_observers.h"
#import "ios/chrome/browser/web/tab_id_tab_helper.h"
#import "ios/chrome/browser/web_state_list/fake_web_state_list_delegate.h"
#import "ios/chrome/browser/web_state_list/web_state_list.h"
#import "ios/chrome/browser/web_state_list/web_state_opener.h"
#include "ios/web/public/test/test_web_thread_bundle.h"
#import "ios/web/public/web_state/web_state.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/platform_test.h"
#include "ui/gfx/image/image.h"
#include "url/gurl.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface TabModelFaviconDriverObserverTestTabModelObserver
    : NSObject<TabModelObserver>

@property(nonatomic, readonly) BOOL tabModelDidChangeTabCalled;

@end

@implementation TabModelFaviconDriverObserverTestTabModelObserver

@synthesize tabModelDidChangeTabCalled = _tabModelDidChangeTabCalled;

- (void)tabModel:(TabModel*)model didChangeTab:(Tab*)tab {
  _tabModelDidChangeTabCalled = YES;
}

@end

class TabModelFaviconDriverObserverTest : public PlatformTest {
 public:
  TabModelFaviconDriverObserverTest();
  ~TabModelFaviconDriverObserverTest() override = default;

  void TearDown() override;

  favicon::FaviconDriver* CreateAndInsertTab();

  TabModelFaviconDriverObserver* tab_model_favicon_driver_observer() {
    return &tab_model_favicon_driver_observer_;
  }

  TabModelFaviconDriverObserverTestTabModelObserver* tab_model_observer() {
    return tab_model_observer_;
  }

 private:
  web::TestWebThreadBundle test_web_thread_bundle_;
  std::unique_ptr<ios::ChromeBrowserState> browser_state_;
  FakeWebStateListDelegate web_state_list_delegate_;
  WebStateList web_state_list_;

  TabModelObservers* tab_model_observers_;
  TabModelFaviconDriverObserver tab_model_favicon_driver_observer_;
  ScopedObserver<WebStateList, WebStateListObserver>
      scoped_web_state_list_observer_;

  TabModelFaviconDriverObserverTestTabModelObserver* tab_model_observer_;

  DISALLOW_COPY_AND_ASSIGN(TabModelFaviconDriverObserverTest);
};

TabModelFaviconDriverObserverTest::TabModelFaviconDriverObserverTest()
    : browser_state_(TestChromeBrowserState::Builder().Build()),
      web_state_list_(&web_state_list_delegate_),
      tab_model_observers_([TabModelObservers observers]),
      tab_model_favicon_driver_observer_(nil, tab_model_observers_),
      scoped_web_state_list_observer_(&tab_model_favicon_driver_observer_) {
  scoped_web_state_list_observer_.Add(&web_state_list_);
  tab_model_observer_ =
      [[TabModelFaviconDriverObserverTestTabModelObserver alloc] init];
  [tab_model_observers_ addObserver:tab_model_observer_];
}

void TabModelFaviconDriverObserverTest::TearDown() {
  // Clear pointer to Objective-C instances to ensure they are released
  // when the PlatformTest autoreleasepool is cleared.
  [tab_model_observers_ removeObserver:tab_model_observer_];
  tab_model_observer_ = nil;

  // It is safe to drop the last strong reference to TabModelObservers here
  // because TabModelFaviconDriverObserver only has a weak reference to it.
  tab_model_observers_ = nil;

  PlatformTest::TearDown();
}

favicon::FaviconDriver*
TabModelFaviconDriverObserverTest::CreateAndInsertTab() {
  std::unique_ptr<web::WebState> web_state =
      web::WebState::Create(web::WebState::CreateParams(browser_state_.get()));

  // TODO(crbug.com/783776): once the API has been changed to use WebState
  // instead of Tab, then we can remove the creation of the LegacyTabHelper
  // and TabIdTabHelper.
  TabIdTabHelper::CreateForWebState(web_state.get());
  LegacyTabHelper::CreateForWebState(web_state.get());
  favicon::WebFaviconDriver::CreateForWebState(web_state.get(),
                                               /*favicon_service=*/nullptr,
                                               /*history_service=*/nullptr);

  favicon::FaviconDriver* favicon_driver =
      favicon::WebFaviconDriver::FromWebState(web_state.get());

  web_state_list_.InsertWebState(0, std::move(web_state),
                                 WebStateList::INSERT_FORCE_INDEX,
                                 WebStateOpener());

  return favicon_driver;
}

TEST_F(TabModelFaviconDriverObserverTest, OnFaviconUpdated) {
  ASSERT_FALSE([tab_model_observer() tabModelDidChangeTabCalled]);

  favicon::FaviconDriver* favicon_driver = CreateAndInsertTab();
  EXPECT_FALSE([tab_model_observer() tabModelDidChangeTabCalled]);

  tab_model_favicon_driver_observer()->OnFaviconUpdated(
      favicon_driver, favicon::FaviconDriverObserver::TOUCH_LARGEST, GURL(),
      true, gfx::Image());
  EXPECT_TRUE([tab_model_observer() tabModelDidChangeTabCalled]);
}
