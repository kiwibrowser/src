// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_ELEMENTS_SELECTOR_VIEW_CONTROLLER_DELEGATE_H_
#define IOS_CHROME_BROWSER_UI_ELEMENTS_SELECTOR_VIEW_CONTROLLER_DELEGATE_H_

// Protocol for delegates of view controllers presented by SelectorCoordinator.
@protocol SelectorViewControllerDelegate<NSObject>
// Invoked when an option has been selected, or the user otherwise dismisses
// the UI.
- (void)selectorViewController:(UIViewController*)viewController
               didSelectOption:(NSString*)option;
@end

#endif  // IOS_CHROME_BROWSER_UI_ELEMENTS_SELECTOR_VIEW_CONTROLLER_DELEGATE_H_
