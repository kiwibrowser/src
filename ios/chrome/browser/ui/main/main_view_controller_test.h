// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_MAIN_MAIN_VIEW_CONTROLLER_TEST_H_
#define IOS_CHROME_BROWSER_UI_MAIN_MAIN_VIEW_CONTROLLER_TEST_H_

#import <UIKit/UIKit.h>

#include "base/macros.h"
#import "ios/chrome/test/block_cleanup_test.h"

@protocol TabSwitcher;

class MainViewControllerTest : public BlockCleanupTest {
 public:
  MainViewControllerTest() = default;
  ~MainViewControllerTest() override = default;

 protected:
  // Creates and returns an object that conforms to the TabSwitcher protocol.
  id<TabSwitcher> CreateTestTabSwitcher();

  // Sets the current key window's rootViewController and saves a pointer to
  // the original VC to allow restoring it at the end of the test.
  void SetRootViewController(UIViewController* new_root_view_controller);

  // testing::Test implementation.
  void TearDown() override;

 private:
  // The key window's original root view controller, which must be restored at
  // the end of the test.
  UIViewController* original_root_view_controller_;

  DISALLOW_COPY_AND_ASSIGN(MainViewControllerTest);
};

#endif  // IOS_CHROME_BROWSER_UI_MAIN_MAIN_VIEW_CONTROLLER_TEST_H_
