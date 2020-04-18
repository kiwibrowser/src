// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/web_state_list/web_state_list.h"

#include "base/macros.h"
#include "base/supports_user_data.h"
#import "ios/chrome/browser/web_state_list/fake_web_state_list_delegate.h"
#import "ios/chrome/browser/web_state_list/web_state_list_observer.h"
#import "ios/chrome/browser/web_state_list/web_state_opener.h"
#import "ios/web/public/test/fakes/test_navigation_manager.h"
#import "ios/web/public/test/fakes/test_web_state.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/platform_test.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {
const char kURL0[] = "https://chromium.org/0";
const char kURL1[] = "https://chromium.org/1";
const char kURL2[] = "https://chromium.org/2";

// WebStateList observer that records which events have been called by the
// WebStateList.
class WebStateListTestObserver : public WebStateListObserver {
 public:
  WebStateListTestObserver() = default;

  // Reset statistics whether events have been called.
  void ResetStatistics() {
    web_state_inserted_called_ = false;
    web_state_moved_called_ = false;
    web_state_replaced_called_ = false;
    web_state_detached_called_ = false;
    web_state_activated_called_ = false;
  }

  // Returns whether WebStateInsertedAt was invoked.
  bool web_state_inserted_called() const { return web_state_inserted_called_; }

  // Returns whether WebStateMoved was invoked.
  bool web_state_moved_called() const { return web_state_moved_called_; }

  // Returns whether WebStateReplacedAt was invoked.
  bool web_state_replaced_called() const { return web_state_replaced_called_; }

  // Returns whether WebStateDetachedAt was invoked.
  bool web_state_detached_called() const { return web_state_detached_called_; }

  // Returns whether WebStateActivatedAt was invoked.
  bool web_state_activated_called() const {
    return web_state_activated_called_;
  }

  // WebStateListObserver implementation.
  void WebStateInsertedAt(WebStateList* web_state_list,
                          web::WebState* web_state,
                          int index,
                          bool activating) override {
    web_state_inserted_called_ = true;
  }

  void WebStateMoved(WebStateList* web_state_list,
                     web::WebState* web_state,
                     int from_index,
                     int to_index) override {
    web_state_moved_called_ = true;
  }

  void WebStateReplacedAt(WebStateList* web_state_list,
                          web::WebState* old_web_state,
                          web::WebState* new_web_state,
                          int index) override {
    web_state_replaced_called_ = true;
  }

  void WebStateDetachedAt(WebStateList* web_state_list,
                          web::WebState* web_state,
                          int index) override {
    web_state_detached_called_ = true;
  }

  void WebStateActivatedAt(WebStateList* web_state_list,
                           web::WebState* old_web_state,
                           web::WebState* new_web_state,
                           int active_index,
                           int reason) override {
    web_state_activated_called_ = true;
  }

 private:
  bool web_state_inserted_called_ = false;
  bool web_state_moved_called_ = false;
  bool web_state_replaced_called_ = false;
  bool web_state_detached_called_ = false;
  bool web_state_activated_called_ = false;

  DISALLOW_COPY_AND_ASSIGN(WebStateListTestObserver);
};

// A fake NavigationManager used to test opener-opened relationship in the
// WebStateList.
class FakeNavigationManager : public web::TestNavigationManager {
 public:
  FakeNavigationManager() = default;

  // web::NavigationManager implementation.
  int GetLastCommittedItemIndex() const override {
    return last_committed_item_index;
  }

  bool CanGoBack() const override { return last_committed_item_index > 0; }

  bool CanGoForward() const override {
    return last_committed_item_index < INT_MAX;
  }

  void GoBack() override {
    DCHECK(CanGoBack());
    --last_committed_item_index;
  }

  void GoForward() override {
    DCHECK(CanGoForward());
    ++last_committed_item_index;
  }

  void GoToIndex(int index) override { last_committed_item_index = index; }

  int last_committed_item_index = 0;

  DISALLOW_COPY_AND_ASSIGN(FakeNavigationManager);
};

}  // namespace

class WebStateListTest : public PlatformTest {
 public:
  WebStateListTest() : web_state_list_(&web_state_list_delegate_) {
    web_state_list_.AddObserver(&observer_);
  }

  ~WebStateListTest() override { web_state_list_.RemoveObserver(&observer_); }

 protected:
  FakeWebStateListDelegate web_state_list_delegate_;
  WebStateList web_state_list_;
  WebStateListTestObserver observer_;

  std::unique_ptr<web::TestWebState> CreateWebState(const char* url) {
    auto test_web_state = std::make_unique<web::TestWebState>();
    test_web_state->SetCurrentURL(GURL(url));
    test_web_state->SetNavigationManager(
        std::make_unique<FakeNavigationManager>());
    return test_web_state;
  }

  void AppendNewWebState(const char* url) {
    AppendNewWebState(url, WebStateOpener());
  }

  void AppendNewWebState(const char* url, WebStateOpener opener) {
    web_state_list_.InsertWebState(WebStateList::kInvalidIndex,
                                   CreateWebState(url),
                                   WebStateList::INSERT_NO_FLAGS, opener);
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(WebStateListTest);
};

TEST_F(WebStateListTest, IsEmpty) {
  EXPECT_EQ(0, web_state_list_.count());
  EXPECT_TRUE(web_state_list_.empty());

  AppendNewWebState(kURL0);

  EXPECT_TRUE(observer_.web_state_inserted_called());
  EXPECT_EQ(1, web_state_list_.count());
  EXPECT_FALSE(web_state_list_.empty());
}

TEST_F(WebStateListTest, InsertUrlSingle) {
  AppendNewWebState(kURL0);

  EXPECT_TRUE(observer_.web_state_inserted_called());
  ASSERT_EQ(1, web_state_list_.count());
  EXPECT_EQ(kURL0, web_state_list_.GetWebStateAt(0)->GetVisibleURL().spec());
}

TEST_F(WebStateListTest, InsertUrlMultiple) {
  web_state_list_.InsertWebState(0, CreateWebState(kURL0),
                                 WebStateList::INSERT_FORCE_INDEX,
                                 WebStateOpener());
  web_state_list_.InsertWebState(0, CreateWebState(kURL1),
                                 WebStateList::INSERT_FORCE_INDEX,
                                 WebStateOpener());
  web_state_list_.InsertWebState(1, CreateWebState(kURL2),
                                 WebStateList::INSERT_FORCE_INDEX,
                                 WebStateOpener());

  EXPECT_TRUE(observer_.web_state_inserted_called());
  ASSERT_EQ(3, web_state_list_.count());
  EXPECT_EQ(kURL1, web_state_list_.GetWebStateAt(0)->GetVisibleURL().spec());
  EXPECT_EQ(kURL2, web_state_list_.GetWebStateAt(1)->GetVisibleURL().spec());
  EXPECT_EQ(kURL0, web_state_list_.GetWebStateAt(2)->GetVisibleURL().spec());
}

TEST_F(WebStateListTest, ActivateWebState) {
  AppendNewWebState(kURL0);
  EXPECT_EQ(nullptr, web_state_list_.GetActiveWebState());

  web_state_list_.ActivateWebStateAt(0);

  EXPECT_TRUE(observer_.web_state_activated_called());
  ASSERT_EQ(1, web_state_list_.count());
  EXPECT_EQ(web_state_list_.GetWebStateAt(0),
            web_state_list_.GetActiveWebState());
}

TEST_F(WebStateListTest, InsertActivate) {
  web_state_list_.InsertWebState(
      0, CreateWebState(kURL0),
      WebStateList::INSERT_FORCE_INDEX | WebStateList::INSERT_ACTIVATE,
      WebStateOpener());

  EXPECT_TRUE(observer_.web_state_activated_called());
  ASSERT_EQ(1, web_state_list_.count());
  EXPECT_EQ(web_state_list_.GetWebStateAt(0),
            web_state_list_.GetActiveWebState());
}

TEST_F(WebStateListTest, InsertInheritOpener) {
  AppendNewWebState(kURL0);
  web_state_list_.ActivateWebStateAt(0);
  EXPECT_TRUE(observer_.web_state_activated_called());
  ASSERT_EQ(1, web_state_list_.count());
  ASSERT_EQ(web_state_list_.GetWebStateAt(0),
            web_state_list_.GetActiveWebState());

  web_state_list_.InsertWebState(
      WebStateList::kInvalidIndex, CreateWebState(kURL1),
      WebStateList::INSERT_INHERIT_OPENER, WebStateOpener());

  ASSERT_EQ(2, web_state_list_.count());
  ASSERT_EQ(web_state_list_.GetActiveWebState(),
            web_state_list_.GetOpenerOfWebStateAt(1).opener);
}

TEST_F(WebStateListTest, MoveWebStateAtRightByOne) {
  AppendNewWebState(kURL0);
  AppendNewWebState(kURL1);
  AppendNewWebState(kURL2);

  // Sanity check before closing WebState.
  EXPECT_EQ(3, web_state_list_.count());
  EXPECT_EQ(kURL0, web_state_list_.GetWebStateAt(0)->GetVisibleURL().spec());
  EXPECT_EQ(kURL1, web_state_list_.GetWebStateAt(1)->GetVisibleURL().spec());
  EXPECT_EQ(kURL2, web_state_list_.GetWebStateAt(2)->GetVisibleURL().spec());

  observer_.ResetStatistics();
  web_state_list_.MoveWebStateAt(0, 1);

  EXPECT_TRUE(observer_.web_state_moved_called());
  EXPECT_EQ(3, web_state_list_.count());
  EXPECT_EQ(kURL1, web_state_list_.GetWebStateAt(0)->GetVisibleURL().spec());
  EXPECT_EQ(kURL0, web_state_list_.GetWebStateAt(1)->GetVisibleURL().spec());
  EXPECT_EQ(kURL2, web_state_list_.GetWebStateAt(2)->GetVisibleURL().spec());
}

TEST_F(WebStateListTest, MoveWebStateAtRightByMoreThanOne) {
  AppendNewWebState(kURL0);
  AppendNewWebState(kURL1);
  AppendNewWebState(kURL2);

  // Sanity check before closing WebState.
  EXPECT_EQ(3, web_state_list_.count());
  EXPECT_EQ(kURL0, web_state_list_.GetWebStateAt(0)->GetVisibleURL().spec());
  EXPECT_EQ(kURL1, web_state_list_.GetWebStateAt(1)->GetVisibleURL().spec());
  EXPECT_EQ(kURL2, web_state_list_.GetWebStateAt(2)->GetVisibleURL().spec());

  observer_.ResetStatistics();
  web_state_list_.MoveWebStateAt(0, 2);

  EXPECT_TRUE(observer_.web_state_moved_called());
  EXPECT_EQ(3, web_state_list_.count());
  EXPECT_EQ(kURL1, web_state_list_.GetWebStateAt(0)->GetVisibleURL().spec());
  EXPECT_EQ(kURL2, web_state_list_.GetWebStateAt(1)->GetVisibleURL().spec());
  EXPECT_EQ(kURL0, web_state_list_.GetWebStateAt(2)->GetVisibleURL().spec());
}

TEST_F(WebStateListTest, MoveWebStateAtLeftByOne) {
  AppendNewWebState(kURL0);
  AppendNewWebState(kURL1);
  AppendNewWebState(kURL2);

  // Sanity check before closing WebState.
  EXPECT_EQ(3, web_state_list_.count());
  EXPECT_EQ(kURL0, web_state_list_.GetWebStateAt(0)->GetVisibleURL().spec());
  EXPECT_EQ(kURL1, web_state_list_.GetWebStateAt(1)->GetVisibleURL().spec());
  EXPECT_EQ(kURL2, web_state_list_.GetWebStateAt(2)->GetVisibleURL().spec());

  observer_.ResetStatistics();
  web_state_list_.MoveWebStateAt(2, 1);

  EXPECT_TRUE(observer_.web_state_moved_called());
  EXPECT_EQ(3, web_state_list_.count());
  EXPECT_EQ(kURL0, web_state_list_.GetWebStateAt(0)->GetVisibleURL().spec());
  EXPECT_EQ(kURL2, web_state_list_.GetWebStateAt(1)->GetVisibleURL().spec());
  EXPECT_EQ(kURL1, web_state_list_.GetWebStateAt(2)->GetVisibleURL().spec());
}

TEST_F(WebStateListTest, MoveWebStateAtLeftByMoreThanOne) {
  AppendNewWebState(kURL0);
  AppendNewWebState(kURL1);
  AppendNewWebState(kURL2);

  // Sanity check before closing WebState.
  EXPECT_EQ(3, web_state_list_.count());
  EXPECT_EQ(kURL0, web_state_list_.GetWebStateAt(0)->GetVisibleURL().spec());
  EXPECT_EQ(kURL1, web_state_list_.GetWebStateAt(1)->GetVisibleURL().spec());
  EXPECT_EQ(kURL2, web_state_list_.GetWebStateAt(2)->GetVisibleURL().spec());

  observer_.ResetStatistics();
  web_state_list_.MoveWebStateAt(2, 0);

  EXPECT_TRUE(observer_.web_state_moved_called());
  EXPECT_EQ(3, web_state_list_.count());
  EXPECT_EQ(kURL2, web_state_list_.GetWebStateAt(0)->GetVisibleURL().spec());
  EXPECT_EQ(kURL0, web_state_list_.GetWebStateAt(1)->GetVisibleURL().spec());
  EXPECT_EQ(kURL1, web_state_list_.GetWebStateAt(2)->GetVisibleURL().spec());
}

TEST_F(WebStateListTest, MoveWebStateAtSameIndex) {
  AppendNewWebState(kURL0);
  AppendNewWebState(kURL1);
  AppendNewWebState(kURL2);

  // Sanity check before closing WebState.
  EXPECT_EQ(3, web_state_list_.count());
  EXPECT_EQ(kURL0, web_state_list_.GetWebStateAt(0)->GetVisibleURL().spec());
  EXPECT_EQ(kURL1, web_state_list_.GetWebStateAt(1)->GetVisibleURL().spec());
  EXPECT_EQ(kURL2, web_state_list_.GetWebStateAt(2)->GetVisibleURL().spec());

  observer_.ResetStatistics();
  web_state_list_.MoveWebStateAt(2, 2);

  EXPECT_FALSE(observer_.web_state_moved_called());
  EXPECT_EQ(3, web_state_list_.count());
  EXPECT_EQ(kURL0, web_state_list_.GetWebStateAt(0)->GetVisibleURL().spec());
  EXPECT_EQ(kURL1, web_state_list_.GetWebStateAt(1)->GetVisibleURL().spec());
  EXPECT_EQ(kURL2, web_state_list_.GetWebStateAt(2)->GetVisibleURL().spec());
}

TEST_F(WebStateListTest, ReplaceWebStateAt) {
  AppendNewWebState(kURL0);
  AppendNewWebState(kURL1);

  // Sanity check before replacing WebState.
  EXPECT_EQ(2, web_state_list_.count());
  EXPECT_EQ(kURL0, web_state_list_.GetWebStateAt(0)->GetVisibleURL().spec());
  EXPECT_EQ(kURL1, web_state_list_.GetWebStateAt(1)->GetVisibleURL().spec());

  observer_.ResetStatistics();
  std::unique_ptr<web::WebState> old_web_state(
      web_state_list_.ReplaceWebStateAt(1, CreateWebState(kURL2)));

  EXPECT_TRUE(observer_.web_state_replaced_called());
  EXPECT_TRUE(observer_.web_state_activated_called());
  EXPECT_EQ(2, web_state_list_.count());
  EXPECT_EQ(kURL0, web_state_list_.GetWebStateAt(0)->GetVisibleURL().spec());
  EXPECT_EQ(kURL2, web_state_list_.GetWebStateAt(1)->GetVisibleURL().spec());
  EXPECT_EQ(kURL1, old_web_state->GetVisibleURL().spec());
}

TEST_F(WebStateListTest, DetachWebStateAtIndexBegining) {
  AppendNewWebState(kURL0);
  AppendNewWebState(kURL1);
  AppendNewWebState(kURL2);

  // Sanity check before closing WebState.
  EXPECT_EQ(3, web_state_list_.count());
  EXPECT_EQ(kURL0, web_state_list_.GetWebStateAt(0)->GetVisibleURL().spec());
  EXPECT_EQ(kURL1, web_state_list_.GetWebStateAt(1)->GetVisibleURL().spec());
  EXPECT_EQ(kURL2, web_state_list_.GetWebStateAt(2)->GetVisibleURL().spec());

  observer_.ResetStatistics();
  web_state_list_.DetachWebStateAt(0);

  EXPECT_TRUE(observer_.web_state_detached_called());
  EXPECT_EQ(2, web_state_list_.count());
  EXPECT_EQ(kURL1, web_state_list_.GetWebStateAt(0)->GetVisibleURL().spec());
  EXPECT_EQ(kURL2, web_state_list_.GetWebStateAt(1)->GetVisibleURL().spec());
}

TEST_F(WebStateListTest, DetachWebStateAtIndexMiddle) {
  AppendNewWebState(kURL0);
  AppendNewWebState(kURL1);
  AppendNewWebState(kURL2);

  // Sanity check before closing WebState.
  EXPECT_EQ(3, web_state_list_.count());
  EXPECT_EQ(kURL0, web_state_list_.GetWebStateAt(0)->GetVisibleURL().spec());
  EXPECT_EQ(kURL1, web_state_list_.GetWebStateAt(1)->GetVisibleURL().spec());
  EXPECT_EQ(kURL2, web_state_list_.GetWebStateAt(2)->GetVisibleURL().spec());

  observer_.ResetStatistics();
  web_state_list_.DetachWebStateAt(1);

  EXPECT_TRUE(observer_.web_state_detached_called());
  EXPECT_EQ(2, web_state_list_.count());
  EXPECT_EQ(kURL0, web_state_list_.GetWebStateAt(0)->GetVisibleURL().spec());
  EXPECT_EQ(kURL2, web_state_list_.GetWebStateAt(1)->GetVisibleURL().spec());
}

TEST_F(WebStateListTest, DetachWebStateAtIndexLast) {
  AppendNewWebState(kURL0);
  AppendNewWebState(kURL1);
  AppendNewWebState(kURL2);

  // Sanity check before closing WebState.
  EXPECT_EQ(3, web_state_list_.count());
  EXPECT_EQ(kURL0, web_state_list_.GetWebStateAt(0)->GetVisibleURL().spec());
  EXPECT_EQ(kURL1, web_state_list_.GetWebStateAt(1)->GetVisibleURL().spec());
  EXPECT_EQ(kURL2, web_state_list_.GetWebStateAt(2)->GetVisibleURL().spec());

  observer_.ResetStatistics();
  web_state_list_.DetachWebStateAt(2);

  EXPECT_TRUE(observer_.web_state_detached_called());
  EXPECT_EQ(2, web_state_list_.count());
  EXPECT_EQ(kURL0, web_state_list_.GetWebStateAt(0)->GetVisibleURL().spec());
  EXPECT_EQ(kURL1, web_state_list_.GetWebStateAt(1)->GetVisibleURL().spec());
}

TEST_F(WebStateListTest, OpenersEmptyList) {
  EXPECT_TRUE(web_state_list_.empty());

  EXPECT_EQ(WebStateList::kInvalidIndex,
            web_state_list_.GetIndexOfNextWebStateOpenedBy(
                nullptr, WebStateList::kInvalidIndex, false));
  EXPECT_EQ(WebStateList::kInvalidIndex,
            web_state_list_.GetIndexOfLastWebStateOpenedBy(
                nullptr, WebStateList::kInvalidIndex, false));

  EXPECT_EQ(WebStateList::kInvalidIndex,
            web_state_list_.GetIndexOfNextWebStateOpenedBy(
                nullptr, WebStateList::kInvalidIndex, true));
  EXPECT_EQ(WebStateList::kInvalidIndex,
            web_state_list_.GetIndexOfLastWebStateOpenedBy(
                nullptr, WebStateList::kInvalidIndex, true));
}

TEST_F(WebStateListTest, OpenersNothingOpened) {
  AppendNewWebState(kURL0);
  AppendNewWebState(kURL1);
  AppendNewWebState(kURL2);

  for (int index = 0; index < web_state_list_.count(); ++index) {
    web::WebState* opener = web_state_list_.GetWebStateAt(index);
    EXPECT_EQ(
        WebStateList::kInvalidIndex,
        web_state_list_.GetIndexOfNextWebStateOpenedBy(opener, index, false));
    EXPECT_EQ(
        WebStateList::kInvalidIndex,
        web_state_list_.GetIndexOfLastWebStateOpenedBy(opener, index, false));

    EXPECT_EQ(
        WebStateList::kInvalidIndex,
        web_state_list_.GetIndexOfNextWebStateOpenedBy(opener, index, true));
    EXPECT_EQ(
        WebStateList::kInvalidIndex,
        web_state_list_.GetIndexOfLastWebStateOpenedBy(opener, index, true));
  }
}

TEST_F(WebStateListTest, OpenersChildsAfterOpener) {
  AppendNewWebState(kURL0);
  web::WebState* opener = web_state_list_.GetWebStateAt(0);

  AppendNewWebState(kURL1, WebStateOpener(opener));
  AppendNewWebState(kURL2, WebStateOpener(opener));

  const int start_index = web_state_list_.GetIndexOfWebState(opener);
  EXPECT_EQ(1, web_state_list_.GetIndexOfNextWebStateOpenedBy(
                   opener, start_index, false));
  EXPECT_EQ(2, web_state_list_.GetIndexOfLastWebStateOpenedBy(
                   opener, start_index, false));

  EXPECT_EQ(1, web_state_list_.GetIndexOfNextWebStateOpenedBy(
                   opener, start_index, true));
  EXPECT_EQ(2, web_state_list_.GetIndexOfLastWebStateOpenedBy(
                   opener, start_index, true));

  // Simulate a navigation on the opener, results should not change if not
  // using groups, but should now be kInvalidIndex otherwise.
  opener->GetNavigationManager()->GoForward();

  EXPECT_EQ(1, web_state_list_.GetIndexOfNextWebStateOpenedBy(
                   opener, start_index, false));
  EXPECT_EQ(2, web_state_list_.GetIndexOfLastWebStateOpenedBy(
                   opener, start_index, false));

  EXPECT_EQ(WebStateList::kInvalidIndex,
            web_state_list_.GetIndexOfNextWebStateOpenedBy(opener, start_index,
                                                           true));
  EXPECT_EQ(WebStateList::kInvalidIndex,
            web_state_list_.GetIndexOfLastWebStateOpenedBy(opener, start_index,
                                                           true));

  // Add a new WebState with the same opener. It should be considered the next
  // WebState if groups are considered and the last independently on whether
  // groups are used or not.
  web_state_list_.InsertWebState(
      3, CreateWebState(kURL2), WebStateList::INSERT_FORCE_INDEX,
      WebStateOpener(web_state_list_.GetWebStateAt(0)));

  EXPECT_EQ(1, web_state_list_.GetIndexOfNextWebStateOpenedBy(
                   opener, start_index, false));
  EXPECT_EQ(3, web_state_list_.GetIndexOfLastWebStateOpenedBy(
                   opener, start_index, false));

  EXPECT_EQ(3, web_state_list_.GetIndexOfNextWebStateOpenedBy(
                   opener, start_index, true));
  EXPECT_EQ(3, web_state_list_.GetIndexOfLastWebStateOpenedBy(
                   opener, start_index, true));
}

TEST_F(WebStateListTest, OpenersChildsBeforeOpener) {
  AppendNewWebState(kURL0);
  web::WebState* opener = web_state_list_.GetWebStateAt(0);

  AppendNewWebState(kURL1, WebStateOpener(opener));
  AppendNewWebState(kURL2, WebStateOpener(opener));
  web_state_list_.MoveWebStateAt(0, 2);

  const int start_index = web_state_list_.GetIndexOfWebState(opener);
  EXPECT_EQ(WebStateList::kInvalidIndex,
            web_state_list_.GetIndexOfNextWebStateOpenedBy(opener, start_index,
                                                           false));
  EXPECT_EQ(WebStateList::kInvalidIndex,
            web_state_list_.GetIndexOfLastWebStateOpenedBy(opener, start_index,
                                                           false));

  EXPECT_EQ(WebStateList::kInvalidIndex,
            web_state_list_.GetIndexOfNextWebStateOpenedBy(opener, start_index,
                                                           true));
  EXPECT_EQ(WebStateList::kInvalidIndex,
            web_state_list_.GetIndexOfLastWebStateOpenedBy(opener, start_index,
                                                           true));
}
