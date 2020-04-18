// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_HISTORY_POPUP_TAB_HISTORY_LEGACY_COORDINATOR_H_
#define IOS_CHROME_BROWSER_UI_HISTORY_POPUP_TAB_HISTORY_LEGACY_COORDINATOR_H_

#import "ios/chrome/browser/ui/coordinators/chrome_coordinator.h"

@class CommandDispatcher;
@protocol PopupMenuDelegate;
@protocol TabHistoryPresentation;
@protocol TabHistoryUIUpdater;
@class TabModel;

// The coordinator in charge of displaying and dismissing the TabHistoryPopup.
// The TabHistoryPopup is presented when the user long presses the back or
// forward Toolbar button.
// TODO(crbug.com/800266): Remove this coordinator once Phase 1 is enabled.
@interface LegacyTabHistoryCoordinator : ChromeCoordinator

// The dispatcher for this Coordinator.
@property(nonatomic, weak) CommandDispatcher* dispatcher;
// |presentationProvider| runs tasks for before and after presenting the
// TabHistoryPopup.
@property(nonatomic, weak) id<TabHistoryPresentation> presentationProvider;
// |tabHistoryUIUpdater| updates the relevant UI before and after presenting
// the TabHistoryPopup.
@property(nonatomic, weak) id<TabHistoryUIUpdater> tabHistoryUIUpdater;
// The current TabModel being used by BVC.
@property(nonatomic, weak) TabModel* tabModel;

// Dissmisses the currently presented TabHistoryPopup, if none is being
// presented it will no-op.
- (void)dismissHistoryPopup;
// Stops listening for dispatcher calls.
- (void)disconnect;

@end

#endif  // IOS_CHROME_BROWSER_UI_HISTORY_POPUP_TAB_HISTORY_LEGACY_COORDINATOR_H_
