// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_TAB_GRID_TRANSITIONS_GRID_TRANSITION_ANIMATION_H_
#define IOS_CHROME_BROWSER_UI_TAB_GRID_TRANSITIONS_GRID_TRANSITION_ANIMATION_H_

#import <UIKit/UIKit.h>

@class GridTransitionLayout;

// The directions the animation can take.
typedef NS_ENUM(NSUInteger, GridAnimationDirection) {
  // Moving from the expanded grid down into the regular grid.
  GridAnimationDirectionContracting = 0,
  // Moving from the regular grid out to the expanded grid.
  GridAnimationDirectionExpanding = 1,
};

// Delegate for this animation, to be informed about animation events.
@protocol GridTransitionAnimationDelegate
// Tell the delegate thet the animation completed. If |finished| is YES, then
// the animation was able to run its full duration.
- (void)gridTransitionAnimationDidFinish:(BOOL)finished;
@end

// A view that encapsulates an animation used to transition into a grid.
// A transition animator should place this view at the appropriate place in the
// view hierarchy and then call -animateWithDuration: to trigger the animations.
// TODO(crbug.com/804539): Update this class to be an  Orchestrator object
// that the present and dismiss animations can both use.
@interface GridTransitionAnimation : UIView

// Designated initializer. |layout| is a GridTransitionLayout object defining
// the layout the animation should animate to. |delegate| is an object that will
// be informed about events in this object's animation.
// If |startsExpanded| is YES, the animation will start with the grid cells in
// the expanded position and zoom down to the regular grid position. Otherwise
// they will start in the grid position and zoom out to the expanded positions.
- (instancetype)initWithLayout:(GridTransitionLayout*)layout
                      delegate:(id<GridTransitionAnimationDelegate>)delegate
                     direction:(GridAnimationDirection)direction
    NS_DESIGNATED_INITIALIZER;

- (instancetype)initWithCoder:(NSCoder*)aDecoder NS_UNAVAILABLE;
- (instancetype)initWithFrame:(CGRect)frame NS_UNAVAILABLE;

// Runs the animation for this object with the passed duration.
// It's an error to call this more than once on any instance of this object.
- (void)animateWithDuration:(NSTimeInterval)duration;

@end

#endif  // IOS_CHROME_BROWSER_UI_TAB_GRID_TRANSITIONS_GRID_TRANSITION_ANIMATION_H_
