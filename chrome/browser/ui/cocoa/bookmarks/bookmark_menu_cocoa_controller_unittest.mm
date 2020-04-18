// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "base/mac/scoped_nsobject.h"
#include "base/strings/string16.h"
#include "chrome/browser/bookmarks/bookmark_model_factory.h"
#include "chrome/browser/ui/browser.h"
#import "chrome/browser/ui/cocoa/bookmarks/bookmark_menu_cocoa_controller.h"
#include "chrome/browser/ui/cocoa/test/cocoa_profile_test.h"
#include "chrome/test/base/testing_profile.h"
#include "components/bookmarks/browser/bookmark_model.h"
#include "components/bookmarks/browser/bookmark_utils.h"
#include "testing/gtest/include/gtest/gtest.h"

using bookmarks::BookmarkModel;
using bookmarks::BookmarkNode;

@interface FakeBookmarkMenuController : BookmarkMenuCocoaController {
 @public
  const BookmarkNode* nodes_[2];
  BOOL opened_[2];
}
- (id)initWithProfile:(Profile*)profile;
@end

@implementation FakeBookmarkMenuController

- (id)initWithProfile:(Profile*)profile {
  if ((self = [super init])) {
    base::string16 empty;
    BookmarkModel* model = BookmarkModelFactory::GetForBrowserContext(profile);
    const BookmarkNode* bookmark_bar = model->bookmark_bar_node();
    nodes_[0] = model->AddURL(bookmark_bar, 0, empty, GURL("http://0.com"));
    nodes_[1] = model->AddURL(bookmark_bar, 1, empty, GURL("http://1.com"));
  }
  return self;
}

- (const BookmarkNode*)nodeForIdentifier:(int)identifier {
  if ((identifier < 0) || (identifier >= 2))
    return NULL;
  return nodes_[identifier];
}

- (void)openURLForNode:(const BookmarkNode*)node {
  std::string url = node->url().possibly_invalid_spec();
  if (url.find("http://0.com") != std::string::npos)
    opened_[0] = YES;
  if (url.find("http://1.com") != std::string::npos)
    opened_[1] = YES;
}

@end  // FakeBookmarkMenuController

class BookmarkMenuCocoaControllerTest : public CocoaProfileTest {
 public:
  void SetUp() override {
    CocoaProfileTest::SetUp();
    ASSERT_TRUE(profile());

    controller_.reset(
        [[FakeBookmarkMenuController alloc] initWithProfile:profile()]);
  }

  FakeBookmarkMenuController* controller() { return controller_.get(); }

 private:
  base::scoped_nsobject<FakeBookmarkMenuController> controller_;
};

TEST_F(BookmarkMenuCocoaControllerTest, TestOpenItem) {
  FakeBookmarkMenuController* c = controller();
  NSMenuItem *item = [[[NSMenuItem alloc] init] autorelease];
  for (int i = 0; i < 2; i++) {
    [item setTag:i];
    ASSERT_EQ(c->opened_[i], NO);
    [c openBookmarkMenuItem:item];
    ASSERT_NE(c->opened_[i], NO);
  }
}
