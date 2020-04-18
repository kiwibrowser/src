// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/browser_list/browser_list_impl.h"

#include <memory>

#include "base/macros.h"
#include "base/scoped_observer.h"
#include "base/test/scoped_task_environment.h"
#include "ios/chrome/browser/browser_state/test_chrome_browser_state.h"
#import "ios/chrome/browser/ui/browser_list/browser_list_observer.h"
#import "ios/chrome/browser/web_state_list/fake_web_state_list_delegate.h"
#import "ios/chrome/browser/web_state_list/web_state_list_delegate.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/platform_test.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

// Observer for the test BrowserList that record which of the
// BrowserListObserver methods have been called.
class BrowserListImplTestObserver : public BrowserListObserver {
 public:
  BrowserListImplTestObserver() = default;

  // Reset statistics whether events has been called.
  void ResetStatistics() {
    browser_created_called_ = false;
    browser_removed_called_ = false;
  }

  // Returns whether OnBrowserCreated() was invoked.
  bool browser_created_called() const { return browser_created_called_; }

  // Returns whether OnBrowserRemoved() was invoked.
  bool browser_removed_called() const { return browser_removed_called_; }

  // BrowserListObserver implementation.
  void OnBrowserCreated(BrowserList* browser_list, Browser* browser) override {
    browser_created_called_ = true;
  }

  void OnBrowserRemoved(BrowserList* browser_list, Browser* browser) override {
    browser_removed_called_ = true;
  }

 private:
  bool browser_created_called_ = false;
  bool browser_removed_called_ = false;
};

}  // namespace

class BrowserListImplTest : public PlatformTest {
 public:
  BrowserListImplTest() {
    browser_state_ = TestChromeBrowserState::Builder().Build();
    browser_list_ = std::make_unique<BrowserListImpl>(
        browser_state_.get(), std::make_unique<FakeWebStateListDelegate>());
  }

  BrowserListImpl* browser_list() { return browser_list_.get(); }

 private:
  base::test::ScopedTaskEnvironment task_environment_;
  std::unique_ptr<ios::ChromeBrowserState> browser_state_;
  std::unique_ptr<BrowserListImpl> browser_list_;

  DISALLOW_COPY_AND_ASSIGN(BrowserListImplTest);
};

// Tests the creation of a new Browser.
TEST_F(BrowserListImplTest, CreateBrowser) {
  EXPECT_EQ(0, browser_list()->GetCount());

  Browser* browser = browser_list()->CreateNewBrowser();
  ASSERT_EQ(1, browser_list()->GetCount());
  EXPECT_EQ(browser, browser_list()->GetBrowserAtIndex(0));
}

// Tests the destruction of a Browser.
TEST_F(BrowserListImplTest, CloseBrowser) {
  browser_list()->CreateNewBrowser();
  EXPECT_EQ(1, browser_list()->GetCount());

  browser_list()->CloseBrowserAtIndex(0);
  EXPECT_EQ(0, browser_list()->GetCount());
}

// Tests that ContainsIndex returns the correct Browser and supports invalid
// indexes.
TEST_F(BrowserListImplTest, ContainsIndex) {
  EXPECT_EQ(0, browser_list()->GetCount());
  EXPECT_FALSE(browser_list()->ContainsIndex(-1));
  EXPECT_FALSE(browser_list()->ContainsIndex(0));
  EXPECT_FALSE(browser_list()->ContainsIndex(1));

  browser_list()->CreateNewBrowser();
  EXPECT_EQ(1, browser_list()->GetCount());
  EXPECT_FALSE(browser_list()->ContainsIndex(-1));
  EXPECT_TRUE(browser_list()->ContainsIndex(0));
  EXPECT_FALSE(browser_list()->ContainsIndex(1));

  browser_list()->CreateNewBrowser();
  EXPECT_EQ(2, browser_list()->GetCount());
  EXPECT_FALSE(browser_list()->ContainsIndex(-1));
  EXPECT_TRUE(browser_list()->ContainsIndex(0));
  EXPECT_TRUE(browser_list()->ContainsIndex(1));
}

// Tests that GetIndexOfBrowser returns the correct index for a Browser,
// including when passed an unknown Browser.
TEST_F(BrowserListImplTest, GetIndexOfBrowser) {
  EXPECT_EQ(BrowserList::kInvalidIndex,
            browser_list()->GetIndexOfBrowser(nullptr));

  Browser* browser_0 = browser_list()->CreateNewBrowser();
  EXPECT_EQ(0, browser_list()->GetIndexOfBrowser(browser_0));
  EXPECT_EQ(browser_0, browser_list()->GetBrowserAtIndex(0));

  Browser* browser_1 = browser_list()->CreateNewBrowser();
  EXPECT_EQ(1, browser_list()->GetIndexOfBrowser(browser_1));
  EXPECT_EQ(browser_1, browser_list()->GetBrowserAtIndex(1));

  // browser_0 is now invalid.
  browser_list()->CloseBrowserAtIndex(0);
  EXPECT_EQ(BrowserList::kInvalidIndex,
            browser_list()->GetIndexOfBrowser(browser_0));
  EXPECT_EQ(0, browser_list()->GetIndexOfBrowser(browser_1));
  EXPECT_EQ(browser_1, browser_list()->GetBrowserAtIndex(0));
}

// Tests that the BrowserListObserver methods are correctly called.
TEST_F(BrowserListImplTest, Observer) {
  BrowserListImplTestObserver observer;
  ScopedObserver<BrowserList, BrowserListObserver> scoped_observer(&observer);
  scoped_observer.Add(browser_list());

  observer.ResetStatistics();
  ASSERT_FALSE(observer.browser_created_called());
  ASSERT_FALSE(observer.browser_removed_called());

  browser_list()->CreateNewBrowser();
  EXPECT_TRUE(observer.browser_created_called());
  EXPECT_FALSE(observer.browser_removed_called());

  observer.ResetStatistics();
  ASSERT_FALSE(observer.browser_created_called());
  ASSERT_FALSE(observer.browser_removed_called());

  browser_list()->CloseBrowserAtIndex(0);
  EXPECT_FALSE(observer.browser_created_called());
  EXPECT_TRUE(observer.browser_removed_called());
}
