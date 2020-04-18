// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/constrained_window/constrained_window_sheet_controller.h"

#include <map>

#include "base/logging.h"
#include "base/mac/sdk_forward_declarations.h"
#import "chrome/browser/ui/cocoa/constrained_window/constrained_window_sheet.h"
#include "chrome/browser/ui/cocoa/constrained_window/constrained_window_sheet_info.h"
#import "chrome/browser/ui/cocoa/web_contents_modal_dialog_host_cocoa.h"

namespace {

// Maps parent windows to sheet controllers.
NSMutableDictionary* g_sheetControllers;

// Get a value for the given window that can be used as a key in a dictionary.
NSValue* GetKeyForParentWindow(NSWindow* parent_window) {
  return [NSValue valueWithNonretainedObject:parent_window];
}

// Returns the bounds to use when showing a sheet for a given parent view. This
// returns a rect in window coordinates.
NSRect GetSheetParentBoundsForParentView(NSView* view) {
  // If the devtools view is open, it shrinks the size of the WebContents, so go
  // up the hierarchy to the devtools container view to avoid that. Note that
  // the devtools view is always in the hierarchy even if it is not open or it
  // is detached.
  NSView* devtools_view = [[[view superview] superview] superview];
  if (devtools_view)
    view = devtools_view;
  return [view convertRect:[view bounds] toView:nil];
}

}  // namespace

// An invisible overlay window placed on top of the sheet's parent view.
// This window blocks interaction with the underlying view.
@interface CWSheetOverlayWindow : NSWindow {
  base::scoped_nsobject<ConstrainedWindowSheetController> controller_;
}
@end

@interface ConstrainedWindowSheetController ()
- (id)initWithParentWindow:(NSWindow*)parentWindow;
- (ConstrainedWindowSheetInfo*)findSheetInfoForParentView:(NSView*)parentView;
- (ConstrainedWindowSheetInfo*)
    findSheetInfoForSheet:(id<ConstrainedWindowSheet>)sheet;
- (void)onParentWindowWillClose:(NSNotification*)note;
- (void)onParentWindowSizeDidChange:(NSNotification*)note;
- (void)updateSheetPosition:(NSView*)parentView;
- (NSRect)overlayWindowFrameForParentView:(NSView*)parentView;
- (NSPoint)originForSheetSize:(NSSize)sheetSize
              inContainerRect:(NSRect)containerRect;
- (void)onOverlayWindowMouseDown:(CWSheetOverlayWindow*)overlayWindow;
- (void)closeSheet:(ConstrainedWindowSheetInfo*)info
     withAnimation:(BOOL)withAnimation;
@end

@implementation CWSheetOverlayWindow

- (id)initWithContentRect:(NSRect)rect
               controller:(ConstrainedWindowSheetController*)controller {
  if ((self = [super initWithContentRect:rect
                               styleMask:NSBorderlessWindowMask
                                 backing:NSBackingStoreBuffered
                                   defer:NO])) {
    [self setOpaque:NO];
    [self setBackgroundColor:[NSColor clearColor]];
    [self setIgnoresMouseEvents:NO];
    [self setReleasedWhenClosed:NO];
    controller_.reset([controller retain]);
  }
  return self;
}

- (void)mouseDown:(NSEvent*)event {
  [controller_ onOverlayWindowMouseDown:self];
}

@end

@implementation ConstrainedWindowSheetController

+ (ConstrainedWindowSheetController*)
    controllerForParentWindow:(NSWindow*)parentWindow {
  DCHECK(parentWindow);
  ConstrainedWindowSheetController* controller =
      [g_sheetControllers objectForKey:GetKeyForParentWindow(parentWindow)];
  if (controller)
    return controller;

  base::scoped_nsobject<ConstrainedWindowSheetController> new_controller(
      [[ConstrainedWindowSheetController alloc]
          initWithParentWindow:parentWindow]);
  if (!g_sheetControllers)
    g_sheetControllers = [[NSMutableDictionary alloc] init];
  [g_sheetControllers setObject:new_controller
                         forKey:GetKeyForParentWindow(parentWindow)];
  return new_controller;
}

+ (ConstrainedWindowSheetController*)
    controllerForSheet:(id<ConstrainedWindowSheet>)sheet {
  for (ConstrainedWindowSheetController* controller in
       [g_sheetControllers objectEnumerator]) {
    if ([controller findSheetInfoForSheet:sheet])
      return controller;
  }
  return nil;
}

+ (id<ConstrainedWindowSheet>)sheetForOverlayWindow:(NSWindow*)overlayWindow {
  for (ConstrainedWindowSheetController* controller in
          [g_sheetControllers objectEnumerator]) {
    for (ConstrainedWindowSheetInfo* info in controller->sheets_.get()) {
      if ([overlayWindow isEqual:[info overlayWindow]])
        return [info sheet];
    }
  }
  return nil;
}

- (id)initWithParentWindow:(NSWindow*)parentWindow {
  if ((self = [super init])) {
    parentWindow_.reset([parentWindow retain]);
    sheets_.reset([[NSMutableArray alloc] init]);

    [[NSNotificationCenter defaultCenter]
        addObserver:self
           selector:@selector(onParentWindowWillClose:)
               name:NSWindowWillCloseNotification
             object:parentWindow_];
  }
  return self;
}

- (web_modal::WebContentsModalDialogHost*)dialogHost {
  if (!dialogHost_)
    dialogHost_.reset(new WebContentsModalDialogHostCocoa(self));
  return dialogHost_.get();
}

- (NSWindow*)parentWindow {
  return parentWindow_.get();
}

- (void)showSheet:(id<ConstrainedWindowSheet>)sheet
    forParentView:(NSView*)parentView {
  DCHECK(sheet);
  DCHECK(parentView);

  // At maximum one active view is allowed.
  DCHECK(!activeView_.get() || [activeView_ isEqual:parentView]);
  if (!activeView_.get())
    activeView_.reset([parentView retain]);

  // This function can be called multiple times for the same
  // |parentView|, so sheet info could be created already.
  ConstrainedWindowSheetInfo* existingInfo =
      [self findSheetInfoForParentView:activeView_];
  if (existingInfo) {
    DCHECK([[existingInfo sheet] isEqual:sheet]);
    [self updateSheetPosition:activeView_];
    [existingInfo showSheet];
    return;
  }

  // Observe the parent window's size.
  [[NSNotificationCenter defaultCenter]
      addObserver:self
         selector:@selector(onParentWindowSizeDidChange:)
             name:NSWindowDidResizeNotification
           object:parentWindow_];

  // Create an invisible overlay window.
  NSRect rect = [self overlayWindowFrameForParentView:parentView];
  base::scoped_nsobject<NSWindow> overlayWindow(
      [[CWSheetOverlayWindow alloc] initWithContentRect:rect controller:self]);
  [parentWindow_ addChildWindow:overlayWindow
                        ordered:NSWindowAbove];

  // Add an entry for the sheet.
  base::scoped_nsobject<ConstrainedWindowSheetInfo> info(
      [[ConstrainedWindowSheetInfo alloc] initWithSheet:sheet
                                             parentView:parentView
                                          overlayWindow:overlayWindow]);
  [sheets_ addObject:info];

  // Show the sheet.
  [info showSheet];
}

- (void)hideSheet:(id<ConstrainedWindowSheet>)sheet {
  ConstrainedWindowSheetInfo* info = [self findSheetInfoForSheet:sheet];
  // Method can be called for already hidden sheet. http://crbug.com/589074.
  if ([[info parentView] isEqual:activeView_]) {
    [info hideSheet];
    activeView_.reset();
  }
}

- (void)hideSheetForFullscreenTransition {
  if (ConstrainedWindowSheetInfo* sheetInfo =
          [self findSheetInfoForParentView:activeView_]) {
    [sheetInfo hideSheet];
    isSheetHiddenForFullscreen_ = YES;
  }
}

- (void)unhideSheetForFullscreenTransition {
  isSheetHiddenForFullscreen_ = NO;
  if (ConstrainedWindowSheetInfo* sheetInfo =
          [self findSheetInfoForParentView:activeView_]) {
    [self showSheet:[sheetInfo sheet] forParentView:activeView_];
  }
}

- (NSPoint)originForSheet:(id<ConstrainedWindowSheet>)sheet
           withWindowSize:(NSSize)size {
  ConstrainedWindowSheetInfo* info = [self findSheetInfoForSheet:sheet];
  DCHECK(info);
  NSRect containerRect =
      [self overlayWindowFrameForParentView:[info parentView]];
  return [self originForSheetSize:size inContainerRect:containerRect];
}

- (void)closeSheet:(id<ConstrainedWindowSheet>)sheet {
  ConstrainedWindowSheetInfo* info = [self findSheetInfoForSheet:sheet];
  DCHECK(info);
  [self closeSheet:info withAnimation:YES];
}

- (void)pulseSheet:(id<ConstrainedWindowSheet>)sheet {
  ConstrainedWindowSheetInfo* info = [self findSheetInfoForSheet:sheet];
  DCHECK(info);
  if ([activeView_ isEqual:[info parentView]])
    [[info sheet] pulseSheet];
}

- (int)sheetCount {
  return [sheets_ count];
}

- (NSSize)overlayWindowSizeForParentView:(NSView*)parentView {
  return [self overlayWindowFrameForParentView:parentView].size;
}

- (ConstrainedWindowSheetInfo*)findSheetInfoForParentView:(NSView*)parentView {
  for (ConstrainedWindowSheetInfo* info in sheets_.get()) {
    if ([parentView isEqual:[info parentView]])
      return info;
  }
  return NULL;
}

- (ConstrainedWindowSheetInfo*)
    findSheetInfoForSheet:(id<ConstrainedWindowSheet>)sheet {
  for (ConstrainedWindowSheetInfo* info in sheets_.get()) {
    if ([sheet isEqual:[info sheet]])
      return info;
  }
  return NULL;
}

- (void)onParentWindowWillClose:(NSNotification*)note {
  [[NSNotificationCenter defaultCenter]
      removeObserver:self
                name:NSWindowWillCloseNotification
              object:parentWindow_];

  // Close all sheets.
  NSArray* sheets = [NSArray arrayWithArray:sheets_];
  for (ConstrainedWindowSheetInfo* info in sheets)
    [self closeSheet:info withAnimation:NO];

  dialogHost_.reset();

  // Delete this instance.
  [g_sheetControllers removeObjectForKey:GetKeyForParentWindow(parentWindow_)];
  if (![g_sheetControllers count]) {
    [g_sheetControllers release];
    g_sheetControllers = nil;
  }
}

- (void)onParentWindowSizeDidChange:(NSNotification*)note {
  if (isSheetHiddenForFullscreen_)
    return;

  [self updateSheetPosition:activeView_];
}

- (void)updateSheetPosition:(NSView*)parentView {
  ConstrainedWindowSheetInfo* info =
      [self findSheetInfoForParentView:parentView];
  if (!info)
    return;

  NSRect rect = [self overlayWindowFrameForParentView:parentView];
  [[info overlayWindow] setFrame:rect display:YES];
  [[info sheet] updateSheetPosition];
}

- (NSRect)overlayWindowFrameForParentView:(NSView*)parentView {
  NSRect viewFrame = GetSheetParentBoundsForParentView(parentView);

  id<NSWindowDelegate> delegate = [[parentView window] delegate];
  if ([delegate respondsToSelector:@selector(window:
                                  willPositionSheet:
                                          usingRect:)]) {
    NSRect sheetFrame = NSZeroRect;
    // This API needs Y to be the distance from the bottom of the overlay to
    // the top of the sheet. X, width, and height are ignored.
    sheetFrame.origin.y = NSMaxY(viewFrame);
    NSRect customSheetFrame = [delegate window:[parentView window]
                             willPositionSheet:nil
                                     usingRect:sheetFrame];
    viewFrame.size.height += NSMinY(customSheetFrame) - NSMinY(sheetFrame);
  }

  viewFrame = [[parentView window] convertRectToScreen:viewFrame];
  return viewFrame;
}

- (NSPoint)originForSheetSize:(NSSize)sheetSize
              inContainerRect:(NSRect)containerRect {
  NSPoint origin;
  origin.x = roundf(NSMinX(containerRect) +
                    (NSWidth(containerRect) - sheetSize.width) / 2.0);
  origin.y = NSMaxY(containerRect) + 5 - sheetSize.height;
  return origin;
}

- (void)onOverlayWindowMouseDown:(CWSheetOverlayWindow*)overlayWindow {
  for (ConstrainedWindowSheetInfo* curInfo in sheets_.get()) {
    if ([overlayWindow isEqual:[curInfo overlayWindow]]) {
      [self pulseSheet:[curInfo sheet]];
      [[curInfo sheet] makeSheetKeyAndOrderFront];
      break;
    }
  }
}

- (void)closeSheet:(ConstrainedWindowSheetInfo*)info
     withAnimation:(BOOL)withAnimation {
  if (![sheets_ containsObject:info])
    return;

  [[NSNotificationCenter defaultCenter]
      removeObserver:self
                name:NSWindowDidResizeNotification
              object:parentWindow_];

  if ([activeView_ isEqual:[info parentView]])
    activeView_.reset();

  [parentWindow_ removeChildWindow:[info overlayWindow]];
  [[info sheet] closeSheetWithAnimation:withAnimation];
  [[info overlayWindow] close];
  [sheets_ removeObject:info];
}

@end
