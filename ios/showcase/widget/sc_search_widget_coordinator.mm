// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/showcase/widget/sc_search_widget_coordinator.h"

#import "ios/chrome/search_widget_extension/search_widget_view_controller.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@implementation SCSearchWidgetCoordinator

@synthesize baseViewController;

- (void)start {
  SearchWidgetViewController* searchWidget =
      [[SearchWidgetViewController alloc] init];
  searchWidget.title = @"Search Widget";
  searchWidget.view.backgroundColor = [UIColor whiteColor];
  [self.baseViewController pushViewController:searchWidget animated:YES];
}

@end
