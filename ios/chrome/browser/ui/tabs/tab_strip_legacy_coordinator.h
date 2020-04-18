// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_TABS_TAB_STRIP_LEGACY_COORDINATOR_H_
#define IOS_CHROME_BROWSER_UI_TABS_TAB_STRIP_LEGACY_COORDINATOR_H_

#import <UIKit/UIKit.h>

#import "ios/chrome/browser/ui/bubble/bubble_view_anchor_point_provider.h"
#import "ios/chrome/browser/ui/coordinators/chrome_coordinator.h"
#import "ios/chrome/browser/ui/tabs/requirements/tab_strip_highlighting.h"

@protocol ApplicationCommands;
@protocol BrowserCommands;
@class TabModel;
@protocol TabStripFoldAnimation;
@protocol TabStripPresentation;

namespace ios {
class ChromeBrowserState;
}  // namespace

// A legacy coordinator that presents the public interface for the tablet tab
// strip feature.
@interface TabStripLegacyCoordinator
    : ChromeCoordinator<BubbleViewAnchorPointProvider, TabStripHighlighting>

// BrowserState for this coordinator.
@property(nonatomic, assign) ios::ChromeBrowserState* browserState;

// Dispatcher for sending commands.
@property(nonatomic, weak) id dispatcher;

// Model layer for the tab strip.
@property(nonatomic, weak) TabModel* tabModel;

// Provides methods for presenting the tab strip and checking the visibility
// of the tab strip in the containing object.
@property(nonatomic, assign) id<TabStripPresentation> presentationProvider;

// The duration to wait before starting tab strip animations. Used to
// synchronize animations.
@property(nonatomic, assign) NSTimeInterval animationWaitDuration;

// Used has a placeholder for the tab strip view during the tab switcher
// controller transition animations.
- (UIView<TabStripFoldAnimation>*)placeholderView;

@end

#endif  // IOS_CHROME_BROWSER_UI_TABS_TAB_STRIP_LEGACY_COORDINATOR_H_
