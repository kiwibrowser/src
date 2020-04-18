// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_ALERT_COORDINATOR_INPUT_ALERT_COORDINATOR_H_
#define IOS_CHROME_BROWSER_UI_ALERT_COORDINATOR_INPUT_ALERT_COORDINATOR_H_

#import "ios/chrome/browser/ui/alert_coordinator/alert_coordinator.h"

// Coordinator for displaying Modal Alert with text inputs.
@interface InputAlertCoordinator : AlertCoordinator

// Text fields displayed by the alert.
@property(strong, nonatomic, readonly) NSArray<UITextField*>* textFields;

// Add a text field to the alert.
- (void)addTextFieldWithConfigurationHandler:
    (void (^)(UITextField* textField))configurationHandler;

@end

#endif  // IOS_CHROME_BROWSER_UI_ALERT_COORDINATOR_INPUT_ALERT_COORDINATOR_H_
