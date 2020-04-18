// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_ELEMENTS_SELECTOR_COORDINATOR_H_
#define IOS_CHROME_BROWSER_UI_ELEMENTS_SELECTOR_COORDINATOR_H_

#import "ios/chrome/browser/ui/coordinators/chrome_coordinator.h"

@class SelectorCoordinator;

// Delegate protocol for SelectorCoordinator
@protocol SelectorCoordinatorDelegate<NSObject>
// Called when selection UI has completed.
- (void)selectorCoordinator:(nonnull SelectorCoordinator*)coordinator
    didCompleteWithSelection:(nonnull NSString*)selection;
@end

// Coordinator for displaying UI to allow the user to pick among options.
@interface SelectorCoordinator : ChromeCoordinator

// Options to present to the user.
@property(nonatomic, nullable, copy) NSOrderedSet<NSString*>* options;

// The default option. Starts out selected, and is set as the selected option
// if the user performs a cancel action.
@property(nonatomic, nullable, copy) NSString* defaultOption;

@property(nonatomic, nullable, weak) id<SelectorCoordinatorDelegate> delegate;

@end

#endif  // IOS_CHROME_BROWSER_UI_ELEMENTS_SELECTOR_COORDINATOR_H_
