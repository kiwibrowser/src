// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/settings/clear_browsing_data_coordinator.h"

#include "base/mac/foundation_util.h"
#import "ios/chrome/browser/ui/settings/clear_browsing_data_table_view_controller.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@implementation ClearBrowsingDataCoordinator

- (void)start {
  ClearBrowsingDataTableViewController* clearBrowsingDataTVC =
      [[ClearBrowsingDataTableViewController alloc] init];
  clearBrowsingDataTVC.extendedLayoutIncludesOpaqueBars = YES;
  // We currently know for sure that baseViewController is a
  // Navigation Controller.
  // Todo: there should be a way to stop coordinators once they've been pushed
  // out of the Navigation VC.
  UINavigationController* tableViewNavigationController =
      base::mac::ObjCCastStrict<UINavigationController>(
          self.baseViewController);
  [tableViewNavigationController pushViewController:clearBrowsingDataTVC
                                           animated:YES];
}

@end
