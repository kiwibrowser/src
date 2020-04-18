// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_TRANSLATE_LANGUAGE_SELECTION_COORDINATOR_H_
#define IOS_CHROME_BROWSER_UI_TRANSLATE_LANGUAGE_SELECTION_COORDINATOR_H_

#import <UIKit/UIKit.h>

#import "ios/chrome/browser/translate/language_selection_handler.h"

@protocol ContainedPresenter;

// A coordinator for a language selection UI. This is intended for display as
// a contained, not presented, view controller.
// The methods defined in the LanguageSelectionHandler protocol will be called
// to start the coordinator.
@interface LanguageSelectionCoordinator : NSObject<LanguageSelectionHandler>

// Creates a coordinator that will use |viewController| as the base view
// controller for presenting its UI.
- (instancetype)initWithBaseViewController:(UIViewController*)viewController
    NS_DESIGNATED_INITIALIZER;

- (instancetype)init NS_UNAVAILABLE;

// The base view controller the receiver was initialized with.
@property(weak, nonatomic, readonly) UIViewController* baseViewController;

// Presenter to use to for presenting the receiver's view controller.
@property(nonatomic) id<ContainedPresenter> presenter;

@end

#endif  // IOS_CHROME_BROWSER_UI_TRANSLATE_LANGUAGE_SELECTION_COORDINATOR_H_
