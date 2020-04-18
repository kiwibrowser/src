// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/tools_menu/tools_menu_view_controller.h"

#include "ios/chrome/browser/experimental_flags.h"
#import "ios/chrome/browser/ui/tools_menu/public/tools_menu_constants.h"
#import "ios/chrome/browser/ui/tools_menu/tools_menu_configuration.h"
#import "ios/chrome/browser/ui/tools_menu/tools_menu_view_item.h"
#include "ios/web/public/user_agent.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/platform_test.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

class ToolsMenuViewControllerTest : public PlatformTest {
 protected:
  void SetUp() override {
    PlatformTest::SetUp();
    configuration_ = [[ToolsMenuConfiguration alloc] initWithDisplayView:nil
                                                      baseViewController:nil];
    controller_ = [[ToolsMenuViewController alloc] init];
  }

  // Returns tools menu view item by IDC value, null if not exit.
  ToolsMenuViewItem* GetToolsMenuViewItemWithTag(int tag) {
    for (ToolsMenuViewItem* item in [controller_ menuItems]) {
      if ([item tag] == tag)
        return item;
    }

    return nullptr;
  }

  ToolsMenuConfiguration* configuration_;
  ToolsMenuViewController* controller_;
};

// Tests that "Request Desktop Site" is visible and enabled, and
// "Request Mobile Site" is invisible when the current page is a native page,
// whose user agent type is NONE.
TEST_F(ToolsMenuViewControllerTest, TestUserAgentTypeNONE) {
  [configuration_ setUserAgentType:web::UserAgentType::NONE];
  [controller_ initializeMenuWithConfiguration:configuration_];

  ToolsMenuViewItem* desktop_item =
      GetToolsMenuViewItemWithTag(TOOLS_REQUEST_DESKTOP_SITE);
  ASSERT_TRUE(desktop_item);
  EXPECT_FALSE(desktop_item.active);

  ToolsMenuViewItem* mobile_item =
      GetToolsMenuViewItemWithTag(TOOLS_REQUEST_MOBILE_SITE);
  EXPECT_FALSE(mobile_item);
}

// Tests that "Request Desktop Site" is visible and not enabled, and
// "Request Mobile Site" is invisible when the current page is a web page and
// uses MOBILE user agent.
TEST_F(ToolsMenuViewControllerTest, TestUserAgentTypeMOBILE) {
  [configuration_ setUserAgentType:web::UserAgentType::MOBILE];
  [controller_ initializeMenuWithConfiguration:configuration_];

  ToolsMenuViewItem* desktop_item =
      GetToolsMenuViewItemWithTag(TOOLS_REQUEST_DESKTOP_SITE);
  ASSERT_TRUE(desktop_item);
  EXPECT_TRUE(desktop_item.active);

  ToolsMenuViewItem* mobile_item =
      GetToolsMenuViewItemWithTag(TOOLS_REQUEST_MOBILE_SITE);
  EXPECT_FALSE(mobile_item);
}

// Tests that when the current page is a web page and uses DESKTOP user
// agent, if request mobile site experiment is turned on, "Request Desktop Site"
// is invisible, and "Request Mobile Site" is visible and enabled; otherwise,
// "Request Desktop Site" is visible and not enabled, and "Request Mobile Site"
// is invisible.
TEST_F(ToolsMenuViewControllerTest, TestUserAgentTypeDESKTOP) {
  [configuration_ setUserAgentType:web::UserAgentType::DESKTOP];
  [controller_ initializeMenuWithConfiguration:configuration_];

  ToolsMenuViewItem* desktop_item =
      GetToolsMenuViewItemWithTag(TOOLS_REQUEST_DESKTOP_SITE);
  ToolsMenuViewItem* mobile_item =
      GetToolsMenuViewItemWithTag(TOOLS_REQUEST_MOBILE_SITE);

  EXPECT_FALSE(desktop_item);
  ASSERT_TRUE(mobile_item);
  EXPECT_TRUE(mobile_item.active);
}
