// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/base_bubble_controller.h"

#include "base/logging.h"
#import "base/mac/bind_objc_block.h"
#include "base/mac/bundle_locations.h"
#include "base/mac/foundation_util.h"
#include "base/mac/mac_util.h"
#include "base/mac/scoped_nsobject.h"
#include "base/mac/sdk_forward_declarations.h"
#include "base/strings/string_util.h"
#import "chrome/browser/ui/cocoa/browser_window_controller.h"
#import "chrome/browser/ui/cocoa/info_bubble_view.h"
#import "chrome/browser/ui/cocoa/info_bubble_window.h"
#include "chrome/browser/ui/cocoa/l10n_util.h"
#import "chrome/browser/ui/cocoa/tabs/tab_strip_model_observer_bridge.h"
#include "components/bubble/bubble_controller.h"
#include "ui/base/cocoa/cocoa_base_utils.h"

@interface BaseBubbleController (Private)
- (void)registerForNotifications;
- (void)updateOriginFromAnchor;
- (void)activateTabWithContents:(content::WebContents*)newContents
               previousContents:(content::WebContents*)oldContents
                        atIndex:(NSInteger)index
                         reason:(int)reason;
- (void)recordAnchorOffset;
- (void)parentWindowDidResize:(NSNotification*)notification;
- (void)parentWindowWillClose:(NSNotification*)notification;
- (void)parentWindowWillToggleFullScreen:(NSNotification*)notification;
- (void)closeCleanup;

// Temporary methods to decide how to close the bubble controller.
// TODO(hcarmona): remove these methods when all bubbles use the BubbleManager.
// Notify BubbleManager to close a bubble.
- (void)closeBubbleWithReason:(BubbleCloseReason)reason;
// Will be a no-op in bubble API because this is handled by the BubbleManager.
- (void)closeBubble;
@end

@implementation BaseBubbleController

@synthesize anchorPoint = anchor_;
@synthesize bubble = bubble_;
@synthesize shouldOpenAsKeyWindow = shouldOpenAsKeyWindow_;
@synthesize shouldActivateOnOpen = shouldActivateOnOpen_;
@synthesize shouldCloseOnResignKey = shouldCloseOnResignKey_;
@synthesize bubbleReference = bubbleReference_;

- (id)initWithWindowNibPath:(NSString*)nibPath
               parentWindow:(NSWindow*)parentWindow
                 anchoredAt:(NSPoint)anchoredAt {
  nibPath = [base::mac::FrameworkBundle() pathForResource:nibPath
                                                   ofType:@"nib"];
  if ((self = [super initWithWindowNibPath:nibPath owner:self])) {
    [self setParentWindow:parentWindow];
    anchor_ = anchoredAt;
    shouldOpenAsKeyWindow_ = YES;
    shouldActivateOnOpen_ = YES;
    shouldCloseOnResignKey_ = YES;
  }
  return self;
}

- (id)initWithWindowNibPath:(NSString*)nibPath
             relativeToView:(NSView*)view
                     offset:(NSPoint)offset {
  DCHECK([view window]);
  NSWindow* window = [view window];
  NSRect bounds = [view convertRect:[view bounds] toView:nil];
  NSPoint anchor = NSMakePoint(NSMinX(bounds) + offset.x,
                               NSMinY(bounds) + offset.y);
  anchor = ui::ConvertPointFromWindowToScreen(window, anchor);
  return [self initWithWindowNibPath:nibPath
                        parentWindow:window
                          anchoredAt:anchor];
}

- (id)initWithWindow:(NSWindow*)theWindow
        parentWindow:(NSWindow*)parentWindow
          anchoredAt:(NSPoint)anchoredAt {
  DCHECK(theWindow);
  if ((self = [super initWithWindow:theWindow])) {
    [self setParentWindow:parentWindow];
    shouldOpenAsKeyWindow_ = YES;
    shouldActivateOnOpen_ = YES;
    shouldCloseOnResignKey_ = YES;

    DCHECK(![[self window] delegate]);
    [theWindow setDelegate:self];

    base::scoped_nsobject<InfoBubbleView> contentView(
        [[InfoBubbleView alloc] initWithFrame:NSZeroRect]);
    [theWindow setContentView:contentView.get()];
    bubble_ = contentView.get();

    [self awakeFromNib];
    [self setAnchorPoint:anchoredAt];
  }
  return self;
}

- (void)awakeFromNib {
  // Check all connections have been made in Interface Builder.
  DCHECK([self window]);
  DCHECK(bubble_);
  DCHECK_EQ(self, [[self window] delegate]);

  BrowserWindowController* bwc =
      [BrowserWindowController browserWindowControllerForWindow:parentWindow_];
  if (bwc) {
    TabStripController* tabStripController = [bwc tabStripController];
    TabStripModel* tabStripModel = [tabStripController tabStripModel];
    tabStripObserverBridge_.reset(new TabStripModelObserverBridge(tabStripModel,
                                                                  self));
  }

  [bubble_ setArrowLocation:info_bubble::kTopTrailing];
}

- (void)dealloc {
  [self unregisterFromNotifications];
  [super dealloc];
}

- (void)registerForNotifications {
  // No window to register notifications for.
  if (!parentWindow_)
    return;

  NSNotificationCenter* center = [NSNotificationCenter defaultCenter];
  // Watch to see if the parent window closes, and if so, close this one.
  [center addObserver:self
             selector:@selector(parentWindowWillClose:)
                 name:NSWindowWillCloseNotification
               object:parentWindow_];
  // Watch for the full screen event, if so, close the bubble
  [center addObserver:self
             selector:@selector(parentWindowWillToggleFullScreen:)
                 name:NSWindowWillEnterFullScreenNotification
               object:parentWindow_];
  // Watch for the full screen exit event, if so, close the bubble
  [center addObserver:self
             selector:@selector(parentWindowWillToggleFullScreen:)
                 name:NSWindowWillExitFullScreenNotification
               object:parentWindow_];
  // Watch for parent window's resizing, to ensure this one is always
  // anchored correctly.
  [center addObserver:self
             selector:@selector(parentWindowDidResize:)
                 name:NSWindowDidResizeNotification
               object:parentWindow_];
}

- (void)unregisterFromNotifications {
  // No window to unregister notifications.
  if (!parentWindow_)
    return;

  NSNotificationCenter* center = [NSNotificationCenter defaultCenter];
  [center removeObserver:self
                    name:NSWindowWillCloseNotification
                  object:parentWindow_];
  [center removeObserver:self
                    name:NSWindowWillEnterFullScreenNotification
                  object:parentWindow_];
  [center removeObserver:self
                    name:NSWindowWillExitFullScreenNotification
                  object:parentWindow_];
  [center removeObserver:self
                    name:NSWindowDidResizeNotification
                  object:parentWindow_];
}

- (NSWindow*)parentWindow {
  return parentWindow_;
}

- (void)setParentWindow:(NSWindow*)parentWindow {
  if (parentWindow_ == parentWindow) {
    return;
  }

  [self unregisterFromNotifications];

  if (parentWindow_ && [[self window] isVisible]) {
    [parentWindow_ removeChildWindow:[self window]];
    parentWindow_ = parentWindow;
    [parentWindow_ addChildWindow:[self window] ordered:NSWindowAbove];
  } else {
    parentWindow_ = parentWindow;
  }

  [self registerForNotifications];
}

- (void)setAnchorPoint:(NSPoint)anchor {
  anchor_ = anchor;
  [self updateOriginFromAnchor];
}

- (void)recordAnchorOffset {
  // The offset of the anchor from the parent's upper-left-hand corner is kept
  // to ensure the bubble stays anchored correctly if the parent is resized.
  anchorOffset_ = NSMakePoint(NSMinX([parentWindow_ frame]),
                              NSMaxY([parentWindow_ frame]));
  anchorOffset_.x -= anchor_.x;
  anchorOffset_.y -= anchor_.y;
}

- (NSBox*)horizontalSeparatorWithFrame:(NSRect)frame {
  frame.size.height = 1.0;
  base::scoped_nsobject<NSBox> spacer([[NSBox alloc] initWithFrame:frame]);
  [spacer setBoxType:NSBoxSeparator];
  [spacer setBorderType:NSLineBorder];
  [spacer setAlphaValue:0.75];
  return [spacer.release() autorelease];
}

- (NSBox*)verticalSeparatorWithFrame:(NSRect)frame {
  frame.size.width = 1.0;
  base::scoped_nsobject<NSBox> spacer([[NSBox alloc] initWithFrame:frame]);
  [spacer setBoxType:NSBoxSeparator];
  [spacer setBorderType:NSLineBorder];
  [spacer setAlphaValue:0.75];
  return [spacer.release() autorelease];
}

- (void)parentWindowDidResize:(NSNotification*)notification {
  if (!parentWindow_)
    return;

  DCHECK_EQ(parentWindow_, [notification object]);
  NSPoint newOrigin = NSMakePoint(NSMinX([parentWindow_ frame]),
                                  NSMaxY([parentWindow_ frame]));
  newOrigin.x -= anchorOffset_.x;
  newOrigin.y -= anchorOffset_.y;
  [self setAnchorPoint:newOrigin];
}

- (void)parentWindowWillClose:(NSNotification*)notification {
  [self setParentWindow:nil];
  [self closeBubble];
}

- (void)parentWindowWillToggleFullScreen:(NSNotification*)notification {
  [self setParentWindow:nil];
  [self closeBubble];
}

- (void)closeCleanup {
  bubbleCloser_ = nullptr;
  if (resignationObserver_) {
    [[NSNotificationCenter defaultCenter]
        removeObserver:resignationObserver_
                  name:NSWindowDidResignKeyNotification
                object:nil];
    resignationObserver_ = nil;
  }

  tabStripObserverBridge_.reset();
}

- (void)closeBubbleWithReason:(BubbleCloseReason)reason {
  if ([self bubbleReference])
    [self bubbleReference]->CloseBubble(reason);
  else
    [self close];
}

- (void)closeBubble {
  if (![self bubbleReference])
    [self close];
}

- (void)windowWillClose:(NSNotification*)notification {
  [self closeCleanup];
  [[NSNotificationCenter defaultCenter] removeObserver:self];
  [self autorelease];
}

// We want this to be a child of a browser window.  addChildWindow:
// (called from this function) will bring the window on-screen;
// unfortunately, [NSWindowController showWindow:] will also bring it
// on-screen (but will cause unexpected changes to the window's
// position).  We cannot have an addChildWindow: and a subsequent
// showWindow:. Thus, we have our own version.
- (void)showWindow:(id)sender {
  NSWindow* window = [self window];  // Completes nib load.
  [self updateOriginFromAnchor];
  [parentWindow_ addChildWindow:window ordered:NSWindowAbove];
  if (parentWindow_ == [NSApp mainWindow] || shouldActivateOnOpen_) {
    if (shouldOpenAsKeyWindow_) {
      [window makeKeyAndOrderFront:self];
    } else {
      [window orderFront:nil];
    }
  } else {
    [window orderWindow:NSWindowAbove relativeTo:[parentWindow_ windowNumber]];
  }
  [self registerKeyStateEventTap];
  [self recordAnchorOffset];
}

- (void)close {
  [self closeCleanup];
  [super close];
}

// The controller is the delegate of the window so it receives did resign key
// notifications. When key is resigned mirror Windows behavior and close the
// window.
- (void)windowDidResignKey:(NSNotification*)notification {
  NSWindow* window = [self window];
  DCHECK_EQ([notification object], window);

  // If the window isn't visible, it is already closed, and this notification
  // has been sent as part of the closing operation, so no need to close.
  if (![window isVisible])
    return;

  // Don't close when explicily disabled, or if there's an attached sheet (e.g.
  // Open File dialog).
  if ([self shouldCloseOnResignKey] && ![window attachedSheet]) {
    [self closeBubbleWithReason:BUBBLE_CLOSE_FOCUS_LOST];
    return;
  }

  // The bubble should not receive key events when it is no longer key window,
  // so disable sharing parent key state. Share parent key state is only used
  // to enable the close/minimize/maximize buttons of the parent window when
  // the bubble has key state, so disabling it here is safe.
  InfoBubbleWindow* bubbleWindow =
      base::mac::ObjCCastStrict<InfoBubbleWindow>([self window]);
  [bubbleWindow setAllowShareParentKeyState:NO];
}

- (void)windowDidBecomeKey:(NSNotification*)notification {
  // Re-enable share parent key state to make sure the close/minimize/maximize
  // buttons of the parent window are active.
  InfoBubbleWindow* bubbleWindow =
      base::mac::ObjCCastStrict<InfoBubbleWindow>([self window]);
  [bubbleWindow setAllowShareParentKeyState:YES];
}

// Since the bubble shares first responder with its parent window, set event
// handlers to dismiss the bubble when it would normally lose key state.
// Events on sheets are ignored: this assumes the sheet belongs to the bubble
// since, to affect a sheet on a different window, the bubble would also lose
// key status in -[NSWindowDelegate windowDidResignKey:]. This keeps the logic
// simple, since -[NSWindow attachedSheet] returns nil while the sheet is still
// closing.
- (void)registerKeyStateEventTap {
  NSWindow* window = self.window;
  NSNotification* note =
      [NSNotification notificationWithName:NSWindowDidResignKeyNotification
                                    object:window];

  bubbleCloser_ = std::make_unique<ui::BubbleCloser>(
      window, base::BindBlock(^{
        // Do it right now, because if this event is right mouse event, it may
        // pop up a menu. windowDidResignKey: will not run until the menu is
        // closed.
        if ([self respondsToSelector:@selector(windowDidResignKey:)])
          [self windowDidResignKey:note];
      }));

  // The resignationObserver_ watches for when a window resigns key state,
  // meaning the key window has changed and the bubble should be dismissed.
  NSNotificationCenter* center = [NSNotificationCenter defaultCenter];
  resignationObserver_ =
      [center addObserverForName:NSWindowDidResignKeyNotification
                          object:nil
                           queue:[NSOperationQueue mainQueue]
                      usingBlock:^(NSNotification* notif) {
                          if (![[notif object] isSheet] &&
                              [NSApp keyWindow] != [self window])
                            [self windowDidResignKey:note];
                      }];
}

// By implementing this, ESC causes the window to go away.
- (IBAction)cancel:(id)sender {
  // This is not a "real" cancel as potential changes to the radio group are not
  // undone. That's ok.
  [self closeBubbleWithReason:BUBBLE_CLOSE_CANCELED];
}

// Takes the |anchor_| point and adjusts the window's origin accordingly.
- (void)updateOriginFromAnchor {
  NSWindow* window = [self window];
  NSPoint origin = anchor_;

  BOOL isRTL = cocoa_l10n_util::ShouldDoExperimentalRTLLayout();
  switch ([bubble_ alignment]) {
    case info_bubble::kAlignArrowToAnchor: {
      NSSize offsets = NSMakeSize(info_bubble::kBubbleArrowXOffset +
                                  info_bubble::kBubbleArrowWidth / 2.0, 0);
      offsets = [[parentWindow_ contentView] convertSize:offsets toView:nil];
      switch ([bubble_ arrowLocation]) {
        case info_bubble::kTopTrailing:
          origin.x -=
              isRTL ? offsets.width : NSWidth([window frame]) - offsets.width;
          break;
        case info_bubble::kTopLeading:
          origin.x -=
              isRTL ? NSWidth([window frame]) - offsets.width : offsets.width;
          break;
        case info_bubble::kNoArrow:
        // FALLTHROUGH.
        case info_bubble::kTopCenter:
          origin.x -= NSWidth([window frame]) / 2.0;
          break;
      }
      break;
    }

    case info_bubble::kAlignEdgeToAnchorEdge:
      // If the arrow is to the right then move the origin so that the right
      // edge aligns with the anchor. If the arrow is to the left then there's
      // nothing to do because the left edge is already aligned with the left
      // edge of the anchor.
      if ([bubble_ arrowLocation] == info_bubble::kTopTrailing) {
        origin.x -= NSWidth([window frame]);
      }
      break;

    case info_bubble::kAlignTrailingEdgeToAnchorEdge:
      if (!isRTL)
        origin.x -= NSWidth([window frame]);
      break;

    case info_bubble::kAlignLeadingEdgeToAnchorEdge:
      if (isRTL)
        origin.x -= NSWidth([window frame]);
      break;

    default:
      NOTREACHED();
  }

  origin.y -= NSHeight([window frame]);
  [window setFrameOrigin:origin];
}

- (void)activateTabWithContents:(content::WebContents*)newContents
               previousContents:(content::WebContents*)oldContents
                        atIndex:(NSInteger)index
                         reason:(int)reason {
  // The user switched tabs; close.
  [self closeBubble];
}

@end  // BaseBubbleController
