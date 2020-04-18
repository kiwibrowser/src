// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_STACK_VIEW_STACK_VIEW_CONTROLLER_H_
#define IOS_CHROME_BROWSER_UI_STACK_VIEW_STACK_VIEW_CONTROLLER_H_

#import <UIKit/UIKit.h>

#import "ios/chrome/browser/ui/tab_switcher/tab_switcher.h"
#include "ui/base/page_transition_types.h"

@protocol ApplicationCommands;
@class Tab;
@class TabModel;

// A protocol used by the StackViewController for test purposes.
@protocol StackViewControllerTestDelegate
// Informs the delegate that the show tab animation starts.
- (void)stackViewControllerShowWithSelectedTabAnimationDidStart;
// Informs the delegate that the show tab animation finished.
- (void)stackViewControllerShowWithSelectedTabAnimationDidEnd;
// Informs the delegate that the preload of card views finished.
- (void)stackViewControllerPreloadCardViewsDidEnd;
@end

// Controller for the tab-switching UI displayed as a stack of tabs.
@interface StackViewController : UIViewController<TabSwitcher>

@property(nonatomic, weak) id<StackViewControllerTestDelegate> testDelegate;

// Initializes with the given tab models, which must not be nil.
// |activeTabModel| is the model which starts active, and must be one of the
// other two models.
// |applicationCommandEndpoint| is the object that methods in the
// ApplicationCommands protocol should be dispatched to by any BVCs that are
// created.
// TODO(stuartmorgan): Replace the word 'active' in these methods and the
// corresponding delegate methods. crbug.com/513782
- (instancetype)initWithMainTabModel:(TabModel*)mainModel
                         otrTabModel:(TabModel*)otrModel
                      activeTabModel:(TabModel*)activeModel
          applicationCommandEndpoint:
              (id<ApplicationCommands>)applicationCommandEndpoint;

// Performs an animation to zoom the selected tab to the full size of the
// content area. When the animation completes, calls
// |-stackViewControllerDismissAnimationWillStartWithActiveModel:| and
// |-stackViewControllerDismissAnimationDidEnd| on the delegate.
- (void)dismissWithSelectedTabAnimation;

@end

#endif  // IOS_CHROME_BROWSER_UI_STACK_VIEW_STACK_VIEW_CONTROLLER_H_
