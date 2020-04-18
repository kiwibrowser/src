// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/alert_coordinator/input_alert_coordinator.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@implementation InputAlertCoordinator

- (NSArray<UITextField*>*)textFields {
  return [self.alertController textFields];
}

- (void)addTextFieldWithConfigurationHandler:
    (void (^)(UITextField* textField))configurationHandler {
  [self.alertController
      addTextFieldWithConfigurationHandler:configurationHandler];
}

@end
