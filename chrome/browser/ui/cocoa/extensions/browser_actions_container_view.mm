// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/extensions/browser_actions_container_view.h"

#include <algorithm>
#include <utility>

#import "chrome/browser/ui/cocoa/l10n_util.h"
#import "chrome/browser/ui/cocoa/view_id_util.h"
#include "ui/base/cocoa/appkit_utils.h"
#include "ui/events/keycodes/keyboard_code_conversion_mac.h"

NSString* const kBrowserActionGrippyDragStartedNotification =
    @"BrowserActionGrippyDragStartedNotification";
NSString* const kBrowserActionGrippyDraggingNotification =
    @"BrowserActionGrippyDraggingNotification";
NSString* const kBrowserActionGrippyDragFinishedNotification =
    @"BrowserActionGrippyDragFinishedNotification";
NSString* const kBrowserActionsContainerWillAnimate =
    @"BrowserActionsContainerWillAnimate";
NSString* const kBrowserActionsContainerAnimationEnded =
    @"BrowserActionsContainerAnimationEnded";
NSString* const kTranslationWithDelta =
    @"TranslationWithDelta";
NSString* const kBrowserActionsContainerReceivedKeyEvent =
    @"BrowserActionsContainerReceivedKeyEvent";
NSString* const kBrowserActionsContainerKeyEventKey =
    @"BrowserActionsContainerKeyEventKey";

namespace {
const CGFloat kAnimationDuration = 0.2;
const CGFloat kGrippyWidth = 3.0;
}  // namespace

@interface BrowserActionsContainerView(Private)
// Returns the cursor that should be shown when hovering over the grippy based
// on |canDragLeft_| and |canDragRight_|.
- (NSCursor*)appropriateCursorForGrippy;
@end

@implementation BrowserActionsContainerView

@synthesize minWidth = minWidth_;
@synthesize maxWidth = maxWidth_;
@synthesize grippyPinned = grippyPinned_;
@synthesize userIsResizing = userIsResizing_;

#pragma mark -
#pragma mark Overridden Class Functions

- (id)initWithFrame:(NSRect)frameRect {
  if ((self = [super initWithFrame:frameRect])) {
    grippyRect_ = NSMakeRect(0.0, 0.0, kGrippyWidth, NSHeight([self bounds]));
    if (cocoa_l10n_util::ShouldDoExperimentalRTLLayout())
      grippyRect_.origin.x = NSWidth(frameRect) - NSWidth(grippyRect_);

    resizable_ = YES;

    resizeAnimation_.reset([[NSViewAnimation alloc] init]);
    [resizeAnimation_ setDuration:kAnimationDuration];
    [resizeAnimation_ setAnimationBlockingMode:NSAnimationNonblocking];
    [resizeAnimation_ setDelegate:self];

    [self setHidden:YES];
  }
  return self;
}

- (void)drawRect:(NSRect)rect {
  [super drawRect:rect];
  if (highlight_) {
    ui::DrawNinePartImage(
        [self bounds], *highlight_, NSCompositeSourceOver, 1.0, true);
  }
}

- (void)viewDidMoveToWindow {
  if (isOverflow_) {
    // Yet another Cocoa oddity: Custom views in menu items in Cocoa, by
    // default, won't receive key events. However, if we make this the first
    // responder when it's moved to a window, it will, and it will behave
    // properly (i.e., will only receive key events if the menu item is
    // highlighted, not for any key event in the menu). More strangely,
    // setting this to be first responder at any other time (such as calling
    // [[containerView window] makeFirstResponder:containerView] when the menu
    // item is highlighted) does *not* work (it messes up the currently-
    // highlighted item).
    // Since this seems to have the right behavior, use it.
    [[self window] makeFirstResponder:self];
  }
}

- (void)keyDown:(NSEvent*)theEvent {
  // If this is the overflow container, we handle three key events: left, right,
  // and space. Left and right navigate the actions within the container, and
  // space activates the current one. We have to handle this ourselves, because
  // Cocoa doesn't treat custom views with subviews in menu items differently
  // than any other menu item, so it would otherwise be impossible to navigate
  // to a particular action from the keyboard.
  ui::KeyboardCode key = ui::KeyboardCodeFromNSEvent(theEvent);
  BOOL shouldProcess = isOverflow_ &&
      (key == ui::VKEY_RIGHT || key == ui::VKEY_LEFT || key == ui::VKEY_SPACE);

  // If this isn't the overflow container, or isn't one of the keys we process,
  // forward the event on.
  if (!shouldProcess) {
    [super keyDown:theEvent];
    return;
  }

  // TODO(devlin): The keyboard navigation should be adjusted for RTL, but right
  // now we only ever display the extension items in the same way (LTR) on Mac.
  BrowserActionsContainerKeyAction action = BROWSER_ACTIONS_INVALID_KEY_ACTION;
  switch (key) {
    case ui::VKEY_RIGHT:
      action = BROWSER_ACTIONS_INCREMENT_FOCUS;
      break;
    case ui::VKEY_LEFT:
      action = BROWSER_ACTIONS_DECREMENT_FOCUS;
      break;
    case ui::VKEY_SPACE:
      action = BROWSER_ACTIONS_EXECUTE_CURRENT;
      break;
    default:
      NOTREACHED();  // Should have weeded this case out above.
  }

  DCHECK_NE(BROWSER_ACTIONS_INVALID_KEY_ACTION, action);
  NSDictionary* userInfo = @{ kBrowserActionsContainerKeyEventKey : @(action) };
  [[NSNotificationCenter defaultCenter]
      postNotificationName:kBrowserActionsContainerReceivedKeyEvent
                    object:self
                    userInfo:userInfo];
  [super keyDown:theEvent];
}

- (void)setHighlight:(std::unique_ptr<ui::NinePartImageIds>)highlight {
  if (highlight || highlight_) {
    highlight_ = std::move(highlight);
    // We don't allow resizing when the container is highlighting.
    resizable_ = highlight_.get() == nullptr;
    [self setNeedsDisplay:YES];
  }
}

- (BOOL)isHighlighting {
  return highlight_.get() != nullptr;
}

- (void)setIsOverflow:(BOOL)isOverflow {
  if (isOverflow_ != isOverflow) {
    isOverflow_ = isOverflow;
    resizable_ = !isOverflow_;
    [self setNeedsDisplay:YES];
  }
}

- (void)resetCursorRects {
  if (cocoa_l10n_util::ShouldDoExperimentalRTLLayout())
    grippyRect_.origin.x = NSWidth([self frame]) - NSWidth(grippyRect_);
  [self addCursorRect:grippyRect_ cursor:[self appropriateCursorForGrippy]];
}

- (BOOL)acceptsFirstResponder {
  // The overflow container needs to receive key events to handle in-item
  // navigation. The top-level container should not become first responder,
  // allowing focus travel to proceed to the first action.
  return isOverflow_;
}

- (void)mouseDown:(NSEvent*)theEvent {
  NSPoint location =
      [self convertPoint:[theEvent locationInWindow] fromView:nil];
  if (!resizable_ || !NSMouseInRect(location, grippyRect_, [self isFlipped]))
    return;

  dragOffset_ = location.x - (cocoa_l10n_util::ShouldDoExperimentalRTLLayout()
                                  ? NSWidth(self.frame)
                                  : 0);

  userIsResizing_ = YES;

  [[self appropriateCursorForGrippy] push];
  // Disable cursor rects so that the Omnibox and other UI elements don't push
  // cursors while the user is dragging. The cursor should be grippy until
  // the |-mouseUp:| message is received.
  [[self window] disableCursorRects];

  [[NSNotificationCenter defaultCenter]
      postNotificationName:kBrowserActionGrippyDragStartedNotification
                    object:self];
}

- (void)mouseUp:(NSEvent*)theEvent {
  if (!userIsResizing_)
    return;

  [NSCursor pop];
  [[self window] enableCursorRects];

  userIsResizing_ = NO;
  [[NSNotificationCenter defaultCenter]
      postNotificationName:kBrowserActionGrippyDragFinishedNotification
                    object:self];
}

- (void)mouseDragged:(NSEvent*)theEvent {
  if (!userIsResizing_)
    return;

  const CGFloat translation =
      [self convertPoint:[theEvent locationInWindow] fromView:nil].x -
      dragOffset_;
  const CGFloat targetWidth = (cocoa_l10n_util::ShouldDoExperimentalRTLLayout()
                                   ? translation
                                   : NSWidth(self.frame) - translation);

  [self resizeToWidth:targetWidth animate:NO];

  [[NSNotificationCenter defaultCenter]
      postNotificationName:kBrowserActionGrippyDraggingNotification
                    object:self];
}

- (void)animationDidEnd:(NSAnimation*)animation {
  // We notify asynchronously so that the animation fully finishes before any
  // listeners do work.
  [self performSelector:@selector(notifyAnimationEnded)
              withObject:self
              afterDelay:0];
}

- (void)animationDidStop:(NSAnimation*)animation {
  // We notify asynchronously so that the animation fully finishes before any
  // listeners do work.
  [self performSelector:@selector(notifyAnimationEnded)
              withObject:self
              afterDelay:0];
}

- (void)notifyAnimationEnded {
  [[NSNotificationCenter defaultCenter]
      postNotificationName:kBrowserActionsContainerAnimationEnded
                    object:self];
}

- (ViewID)viewID {
  return VIEW_ID_BROWSER_ACTION_TOOLBAR;
}

#pragma mark -
#pragma mark Public Methods

- (void)resizeToWidth:(CGFloat)width animate:(BOOL)animate {
  width = std::min(std::max(width, minWidth_), maxWidth_);

  NSRect newFrame = [self frame];
  if (!cocoa_l10n_util::ShouldDoExperimentalRTLLayout())
    newFrame.origin.x += NSWidth(newFrame) - width;
  newFrame.size.width = width;

  grippyPinned_ = width == maxWidth_;

  [self stopAnimation];

  if (animate) {
    NSDictionary* animationDictionary = @{
      NSViewAnimationTargetKey : self,
      NSViewAnimationStartFrameKey : [NSValue valueWithRect:[self frame]],
      NSViewAnimationEndFrameKey : [NSValue valueWithRect:newFrame]
    };
    [resizeAnimation_ setViewAnimations:@[ animationDictionary ]];
    [resizeAnimation_ startAnimation];

    [[NSNotificationCenter defaultCenter]
        postNotificationName:kBrowserActionsContainerWillAnimate
                      object:self];
  } else {
    [self setFrame:newFrame];
    [self setNeedsDisplay:YES];
  }
}

- (NSRect)animationEndFrame {
  if ([resizeAnimation_ isAnimating]) {
    NSRect endFrame = [[[[resizeAnimation_ viewAnimations] objectAtIndex:0]
        valueForKey:NSViewAnimationEndFrameKey] rectValue];
    return endFrame;
  } else {
    return [self frame];
  }
}

- (BOOL)isAnimating {
  return [resizeAnimation_ isAnimating];
}

- (void)stopAnimation {
  if ([resizeAnimation_ isAnimating])
    [resizeAnimation_ stopAnimation];
}

- (BOOL)canBeResized {
  return resizable_;
}

#pragma mark -
#pragma mark Private Methods

// Returns the cursor to display over the grippy hover region depending on the
// current drag state.
- (NSCursor*)appropriateCursorForGrippy {
  if (resizable_) {
    const CGFloat width = NSWidth(self.frame);
    const BOOL isRTL = cocoa_l10n_util::ShouldDoExperimentalRTLLayout();
    const BOOL canDragLeft = width != (isRTL ? minWidth_ : maxWidth_);
    const BOOL canDragRight = width != (isRTL ? maxWidth_ : minWidth_);

    if (canDragLeft && canDragRight)
      return [NSCursor resizeLeftRightCursor];
    if (canDragLeft)
      return [NSCursor resizeLeftCursor];
    if (canDragRight)
      return [NSCursor resizeRightCursor];
  }
  return [NSCursor arrowCursor];
}

@end
