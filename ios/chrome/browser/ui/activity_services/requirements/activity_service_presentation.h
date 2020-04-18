// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_ACTIVITY_SERVICES_REQUIREMENTS_ACTIVITY_SERVICE_PRESENTATION_H_
#define IOS_CHROME_BROWSER_UI_ACTIVITY_SERVICES_REQUIREMENTS_ACTIVITY_SERVICE_PRESENTATION_H_

// ActivityServicePresentation contains methods that control how the activity
// services menu is presented and dismissed on screen.
@protocol ActivityServicePresentation

// Asks the implementor to present the given |controller|.
- (void)presentActivityServiceViewController:(UIViewController*)controller;

// Called after the activity services UI has been dismissed.  The UIKit-provided
// UIViewController dismisses itself automatically, so the UI does not need to
// be dismissed in this method.  Instead, it is provided to allow implementors
// to perform cleanup after the UI is gone.
- (void)activityServiceDidEndPresenting;

// Asks the implementor to show an error alert with the given |title| and
// |message|.
- (void)showActivityServiceErrorAlertWithStringTitle:(NSString*)title
                                             message:(NSString*)message;

@end

#endif  // IOS_CHROME_BROWSER_UI_ACTIVITY_SERVICES_REQUIREMENTS_ACTIVITY_SERVICE_PRESENTATION_H_
