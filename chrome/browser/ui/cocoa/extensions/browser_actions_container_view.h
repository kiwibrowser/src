// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_EXTENSIONS_BROWSER_ACTIONS_CONTAINER_VIEW_H_
#define CHROME_BROWSER_UI_COCOA_EXTENSIONS_BROWSER_ACTIONS_CONTAINER_VIEW_H_

#import <Cocoa/Cocoa.h>

#include <memory>

#include "base/mac/scoped_nsobject.h"

namespace ui {
struct NinePartImageIds;
}

// Sent when a user-initiated drag to resize the container is initiated.
extern NSString* const kBrowserActionGrippyDragStartedNotification;

// Sent when a user-initiated drag is resizing the container.
extern NSString* const kBrowserActionGrippyDraggingNotification;

// Sent when a user-initiated drag to resize the container has finished.
extern NSString* const kBrowserActionGrippyDragFinishedNotification;

// Sent when the Browser Actions container view is about to animate.
extern NSString* const kBrowserActionsContainerWillAnimate;

// Sent when a running animation has ended.
extern NSString* const kBrowserActionsContainerAnimationEnded;

// Key which is used to notify the translation with delta.
extern NSString* const kTranslationWithDelta;

// Sent when the container receives a key event that should be processed.
// The userInfo contains a single entry with the key event.
extern NSString* const kBrowserActionsContainerReceivedKeyEvent;

// The key into the userInfo dictionary to retrieve the key event (stored as an
// integer).
extern NSString* const kBrowserActionsContainerKeyEventKey;

// The possible key actions to process.
enum BrowserActionsContainerKeyAction {
  BROWSER_ACTIONS_INCREMENT_FOCUS = 0,
  BROWSER_ACTIONS_DECREMENT_FOCUS = 1,
  BROWSER_ACTIONS_EXECUTE_CURRENT = 2,
  BROWSER_ACTIONS_INVALID_KEY_ACTION = 3,
};

// The view that encompasses the Browser Action buttons in the toolbar and
// provides mechanisms for resizing.
@interface BrowserActionsContainerView : NSView<NSAnimationDelegate> {
 @private
  // The frame encompasing the grippy used for resizing the container.
  NSRect grippyRect_;

  // Remember where in the grippy the drag began.
  CGFloat dragOffset_;

  // Whether the container is currently being resized by the user.
  BOOL userIsResizing_;

  // Whether the user can resize the container; this is disabled for overflow
  // (where it would make no sense) and during highlighting, since this is a
  // temporary and entirely browser-driven sequence in order to warn the user
  // about potentially dangerous items.
  BOOL resizable_;

  // Whether or not the container is the overflow container, and is shown in the
  // app menu.
  BOOL isOverflow_;

  // When the left grippy is pinned, resizing the window has no effect on its
  // position. This prevents it from overlapping with other elements as well
  // as letting the container expand when the window is going from super small
  // to large.
  BOOL grippyPinned_;

  // The nine-grid of the highlight to paint, if any.
  std::unique_ptr<ui::NinePartImageIds> highlight_;

  base::scoped_nsobject<NSViewAnimation> resizeAnimation_;
}

// Sets whether or not the container is the overflow container.
- (void)setIsOverflow:(BOOL)isOverflow;

// Sets whether or not the container is highlighting.
- (void)setHighlight:(std::unique_ptr<ui::NinePartImageIds>)highlight;

// Reeturns true if the container is currently highlighting.
- (BOOL)isHighlighting;

// Resizes the container to the given ideal width, optionally animating.
- (void)resizeToWidth:(CGFloat)width animate:(BOOL)animate;

// Returns the frame of the container after the running animation has finished.
// If no animation is running, returns the container's current frame.
- (NSRect)animationEndFrame;

// Returns true if the view is animating.
- (BOOL)isAnimating;

// Stops any animation in progress.
- (void)stopAnimation;

// Returns true if this view is currently resizable.
- (BOOL)canBeResized;

@property(nonatomic) CGFloat minWidth;
@property(nonatomic) CGFloat maxWidth;
@property(nonatomic) BOOL grippyPinned;
@property(readonly, nonatomic) BOOL userIsResizing;

@end

#endif  // CHROME_BROWSER_UI_COCOA_EXTENSIONS_BROWSER_ACTIONS_CONTAINER_VIEW_H_
