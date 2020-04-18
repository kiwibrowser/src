// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/autofill/autofill_popup_base_view_cocoa.h"

#import "base/mac/scoped_nsobject.h"
#include "chrome/browser/ui/autofill/autofill_popup_view_delegate.h"
#include "chrome/browser/ui/autofill/popup_constants.h"
#include "ui/base/cocoa/window_size_constants.h"
#include "ui/gfx/mac/coordinate_conversion.h"

@implementation AutofillPopupBaseViewCocoa

#pragma mark -
#pragma mark Colors

- (NSColor*)backgroundColor {
  return [NSColor whiteColor];
}

- (NSColor*)borderColor {
  return [NSColor colorForControlTint:[NSColor currentControlTint]];
}

- (NSColor*)highlightColor {
  return [NSColor selectedControlColor];
}

- (NSColor*)nameColor {
  return [NSColor blackColor];
}

- (NSColor*)separatorColor {
  return [NSColor colorWithCalibratedWhite:220 / 255.0 alpha:1];
}

- (NSColor*)subtextColor {
  // Represents #646464.
  return [NSColor colorWithCalibratedRed:100.0 / 255.0
                                   green:100.0 / 255.0
                                    blue:100.0 / 255.0
                                   alpha:1.0];
}

#pragma mark -
#pragma mark Public methods

- (id)initWithDelegate:(autofill::AutofillPopupViewDelegate*)delegate
                 frame:(NSRect)frame {
  self = [super initWithFrame:frame];
  if (self) {
    popup_delegate_ = delegate;
    popup_delegate_->SetTypesetter(gfx::Typesetter::BROWSER);
  }

  return self;
}

- (void)delegateDestroyed {
  popup_delegate_ = NULL;
}

- (void)drawSeparatorWithBounds:(NSRect)bounds {
  [[self separatorColor] set];
  [NSBezierPath fillRect:bounds];
}

// A slight optimization for drawing:
// https://developer.apple.com/library/mac/#documentation/Cocoa/Conceptual/CocoaViewsGuide/Optimizing/Optimizing.html
- (BOOL)isOpaque {
  return YES;
}

- (BOOL)isFlipped {
  // Flipped so that it's easier to share controller logic with other OSes.
  return YES;
}

- (void)drawBackgroundAndBorder {
  // The inset is needed since the border is centered on the |path|.
  // TODO(isherman): We should consider using asset-based drawing for the
  // border, creating simple bitmaps for the view's border and background, and
  // drawing them using NSDrawNinePartImage().
  CGFloat inset = autofill::kPopupBorderThickness / 2.0;
  NSRect borderRect = NSInsetRect([self bounds], inset, inset);
  NSBezierPath* path = [NSBezierPath bezierPathWithRect:borderRect];
  [[self backgroundColor] setFill];
  [path fill];
  [path setLineWidth:autofill::kPopupBorderThickness];
  [[self borderColor] setStroke];
  [path stroke];
}

- (void)mouseUp:(NSEvent*)theEvent {
  // If the view is in the process of being destroyed, abort.
  if (!popup_delegate_)
    return;

  // Only accept single-click.
  if ([theEvent clickCount] > 1)
    return;

  NSPoint location = [self convertPoint:[theEvent locationInWindow]
                               fromView:nil];

  if (NSPointInRect(location, [self bounds])) {
    popup_delegate_->SetSelectionAtPoint(
        gfx::Point(NSPointToCGPoint(location)));
    popup_delegate_->AcceptSelectedLine();
  }
}

- (void)mouseMoved:(NSEvent*)theEvent {
  // If the view is in the process of being destroyed, abort.
  if (!popup_delegate_)
    return;

  NSPoint location = [self convertPoint:[theEvent locationInWindow]
                               fromView:nil];

  popup_delegate_->SetSelectionAtPoint(gfx::Point(NSPointToCGPoint(location)));
}

- (void)mouseDragged:(NSEvent*)theEvent {
  [self mouseMoved:theEvent];
}

- (void)mouseExited:(NSEvent*)theEvent {
  // If the view is in the process of being destroyed, abort.
  if (!popup_delegate_)
    return;

  popup_delegate_->SelectionCleared();
}

#pragma mark -
#pragma mark Private methods:

// Returns the full frame needed by the content, which may exceed the available
// vertical space (see clippedPopupFrame).
- (NSRect)fullPopupFrame {
  // Flip the y-origin back into Cocoa-land. The controller's platform-neutral
  // coordinate space places the origin at the top-left of the first screen
  // (e.g. 300 from the top), whereas Cocoa's coordinate space expects the
  // origin to be at the bottom-left of this same screen (e.g. 1200 from the
  // bottom, when including the height of the popup).
  return gfx::ScreenRectToNSRect(popup_delegate_->popup_bounds());
}

// Returns the frame of the popup that should be displayed, which is basically
// the bounds of the popup, clipped if there is not enough available vertical
// space.
- (NSRect)clippedPopupFrame {
  NSRect clippedPopupFrame = [self fullPopupFrame];

  // The y-origin of the popup may be outside the application window. If this
  // happens, it is corrected to be at the application window's bottom edge, and
  // the popup height is adjusted.
  NSWindow* appWindow = [popup_delegate_->container_view() window];
  CGFloat appWindowBottomEdge = NSMinY([appWindow frame]);
  if (clippedPopupFrame.origin.y < appWindowBottomEdge) {
    clippedPopupFrame.origin.y = appWindowBottomEdge;

    // Both the popup frame and [appWindow frame] are in screen coordinates.
    CGFloat dY = NSMaxY([appWindow frame]) - NSMaxY([self fullPopupFrame]);
    clippedPopupFrame.size.height = NSHeight([appWindow frame]) - dY;
  }
  return clippedPopupFrame;
}

#pragma mark -
#pragma mark Messages from AutofillPopupViewBridge:

- (void)updateBoundsAndRedrawPopup {
  // Update the full popup view and the scrollview, as the contents of the popup
  // may have changed and affected the height.
  [self setFrame:[self fullPopupFrame]];
  [(NSScrollView*)[self superview] setDocumentView:self];
  [[[self superview] window] setFrame:[self clippedPopupFrame] display:YES];

  [self setNeedsDisplay:YES];
}

- (void)showPopup {
  NSRect clippedPopupFrame = [self clippedPopupFrame];
  // The window contains a scroll view, and both are the same size, which is
  // may be clipped at the application window's bottom edge (see
  // clippedPopupFrame).
  NSWindow* window =
      [[NSWindow alloc] initWithContentRect:clippedPopupFrame
                                  styleMask:NSBorderlessWindowMask
                                    backing:NSBackingStoreBuffered
                                      defer:NO];
  base::scoped_nsobject<NSScrollView> scrollView(
      [[NSScrollView alloc] initWithFrame:clippedPopupFrame]);
  // Configure the scroller to have no visible border.
  [scrollView setBorderType:NSNoBorder];

  // Configure scrolling behavior and appearance of the scrollbar, which will
  // not show if scrolling is not possible, and only show during scrolling as an
  // overlay scrollbar.
  [scrollView setHasVerticalScroller:YES];
  [scrollView setAutohidesScrollers:YES];
  [scrollView setVerticalScrollElasticity:NSScrollElasticityNone];
  [scrollView setHorizontalScrollElasticity:NSScrollElasticityNone];

  // The |window| contains the |scrollView|, which contains |self|, the full
  // popup view (which is not clipped and may be longer than |scrollView|).
  [self setFrame:[self fullPopupFrame]];
  [scrollView setDocumentView:self];
  [window setContentView:scrollView];

  // Telling Cocoa that the window is opaque enables some drawing optimizations.
  [window setOpaque:YES];

  [self updateBoundsAndRedrawPopup];
  [[popup_delegate_->container_view() window] addChildWindow:window
                                                     ordered:NSWindowAbove];

  // This will momentarily show the vertical scrollbar to indicate that it is
  // possible to scroll. Will do the right thing and not show if scrolling is
  // not possible.
  [scrollView flashScrollers];
}

- (void)hidePopup {
  // Remove the child window before closing, otherwise it can mess up
  // display ordering.
  NSWindow* window = [self window];
  [[window parentWindow] removeChildWindow:window];
  [window close];
}

@end
