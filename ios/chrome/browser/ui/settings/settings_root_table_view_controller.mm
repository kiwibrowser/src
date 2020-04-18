// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/settings/settings_root_table_view_controller.h"

#import "base/mac/foundation_util.h"
#import "ios/chrome/browser/ui/material_components/utils.h"
#import "ios/chrome/browser/ui/settings/settings_navigation_controller.h"
#import "ios/third_party/material_components_ios/src/components/AppBar/src/MaterialAppBar.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@implementation SettingsRootTableViewController

- (void)viewWillAppear:(BOOL)animated {
  [super viewWillAppear:animated];

  // Set up the "Done" button in the upper right of the nav bar.
  SettingsNavigationController* navigationController =
      base::mac::ObjCCast<SettingsNavigationController>(
          self.navigationController);
  UIBarButtonItem* doneButton = [navigationController doneButton];
  self.navigationItem.rightBarButtonItem = doneButton;
}

@end
