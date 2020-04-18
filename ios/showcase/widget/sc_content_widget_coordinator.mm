// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/showcase/widget/sc_content_widget_coordinator.h"

#import "ios/chrome/content_widget_extension/content_widget_view_controller.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@implementation SCContentWidgetCoordinator

@synthesize baseViewController;

- (void)start {
  ContentWidgetViewController* contentWidget =
      [[ContentWidgetViewController alloc] init];
  contentWidget.title = @"Content Widget";
  contentWidget.view.backgroundColor = [UIColor whiteColor];
  [self.baseViewController pushViewController:contentWidget animated:YES];
}

@end
