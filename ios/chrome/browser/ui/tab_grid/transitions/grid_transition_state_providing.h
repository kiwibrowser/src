// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_TAB_GRID_TRANSITIONS_GRID_TRANSITION_STATE_PROVIDING_H_
#define IOS_CHROME_BROWSER_UI_TAB_GRID_TRANSITIONS_GRID_TRANSITION_STATE_PROVIDING_H_

#import <Foundation/Foundation.h>

@class GridTransitionLayout;

// Objects conforming to this protocol can provide state information to
// transition delegates and animators for a grid.
@protocol GridTransitionStateProviding

// YES if the currently selected cell is visible in the grid.
@property(nonatomic, readonly, getter=isSelectedCellVisible)
    BOOL selectedCellVisible;

// Asks the provider for an aray of layout items that provide objects for use
// in building an animated transition.
- (GridTransitionLayout*)layoutForTransitionContext:
    (id<UIViewControllerContextTransitioning>)context;

// Asks the provider for the view to add proxy views to when building an
// animated transition.
- (UIView*)proxyContainerForTransitionContext:
    (id<UIViewControllerContextTransitioning>)context;

// Asks the provider for the view (if any) that proxy views should be added
// in front of when building an animated transition. It's an error if this
// view is not nil and isn't an immediate subview of the view returned by
// |-proxyContainerForTransitionContext:|
- (UIView*)proxyPositionForTransitionContext:
    (id<UIViewControllerContextTransitioning>)context;

@end

#endif  // IOS_CHROME_BROWSER_UI_TAB_GRID_TRANSITIONS_TAB_GRID_TRANSITION_STATE_PROVIDING_H_
