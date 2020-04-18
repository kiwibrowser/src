// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_STACK_VIEW_PAGE_ANIMATION_UTIL_H_
#define IOS_CHROME_BROWSER_UI_STACK_VIEW_PAGE_ANIMATION_UTIL_H_

#import <CoreGraphics/CoreGraphics.h>
#import <QuartzCore/QuartzCore.h>

@class CardView;
@class UIView;

// Utilities for handling the animations of a page opening or closing.
// The expected use is to create a view with the same size and position as the
// page content area, add it to the view hierarchy, then call the appropriate
// animation method.
namespace page_animation_util {

// The standard margin between a card and the edge of the content view, in
// pixels.
extern const CGFloat kCardMargin;

// Animation start positions.
enum TabStartPosition {
  // From offscreen on the right.
  START_RIGHT,
  // From offscreen on the left.
  START_LEFT
};

// Applies a transformation that moves |view| to the new tab animation start
// position. The new tab starts translated down; if |start| is RIGHT, it is
// rotated clockwise and translated to the right; if LEFT, it is rotated
// counter-clockwise and translated to the left.
void SetTabAnimationStartPositionForView(UIView* view, TabStartPosition start);

// Adjusts the position of |view| to where a card (pre-MD) animate-in effect
// should start, then animates the view back to its original position (calling
// |extraAnimation| in the same animation context), and finally calls the given
// completion block when finished.  For MD, this is still used when a card is
// added in the background while in the stack view.
void AnimateInCardWithAnimationAndCompletion(UIView* view,
                                             void (^extraAnimation)(void),
                                             void (^completion)(void));

// Adjusts the position of |view| to |origin|, then animates with a MD
// animate-in effect, animating the view back to its original position (calling
// |extraAnimation| in the same animation context), and finally calls the given
// completion block when finished. |paperOffset| and |contentOffset| describe
// the vertical offset for the paper animation and the |view| animation
// respectively.
void AnimateInPaperWithAnimationAndCompletion(UIView* view,
                                              CGFloat paperOffset,
                                              CGFloat contentOffset,
                                              CGPoint origin,
                                              BOOL isOffTheRecord,
                                              void (^extraAnimation)(void),
                                              void (^completion)(void));

// Sets |currentPageCard| to the size of |displayFrame|, animates it into a
// standard inset CardView (just as in the appearance of a single-stack
// StackViewController), and out again. Creates and moves a paper card into the
// center of the screen, and then slides it offscreen. |completion| is
// called at the end of the sequence. |displayFrame| gives the frame within
// which the animation will take place. |imageFrame| gives the size of the
// snapshot image from which the animation starts.
void AnimateNewBackgroundPageWithCompletion(CardView* currentPageCard,
                                            CGRect displayFrame,
                                            CGRect imageFrame,
                                            BOOL isPortrait,
                                            void (^completion)(void));

// Sets |currentPageCard| to the size of |displayFrame|, animates it into a
// standard inset CardView (just as in the appearance of a single-stack
// StackViewController), and out again. Moves |newCard| offscreen, then rotates
// it into its given position, and finally slides it offscreen. |completion| is
// called at the end of the sequence. |displayFrame| gives the frame within
// which the animation will take place.
void AnimateNewBackgroundTabWithCompletion(CardView* currentPageCard,
                                           CardView* newCard,
                                           CGRect displayFrame,
                                           BOOL isPortrait,
                                           void (^completion)(void));

// Animates |view| to its final position following |delay| seconds, then calls
// the given completion block when finished.
void AnimateOutWithCompletion(UIView* view,
                              NSTimeInterval delay,
                              BOOL clockwise,
                              BOOL isPortrait,
                              void (^completion)(void));

// Returns a transform to rotate and translate the view in the proper direction
// and |fraction| of |kAnimateInInitialHorizontalOffset|,
// |kAnimateInInitialVerticalOffset| and |kCardRotationAnimationStart|.
CGAffineTransform AnimateOutTransform(CGFloat fraction,
                                      BOOL clockwise,
                                      BOOL isPortrait);

// Returns the breadth of the animation-out transform.
CGFloat AnimateOutTransformBreadth();

}  // namespace page_animation_util

#endif  // IOS_CHROME_BROWSER_UI_STACK_VIEW_PAGE_ANIMATION_UTIL_H_
