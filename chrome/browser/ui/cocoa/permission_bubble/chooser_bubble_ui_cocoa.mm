// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/permission_bubble/chooser_bubble_ui_cocoa.h"

#include <stddef.h>

#include <algorithm>
#include <cmath>

#include "base/mac/scoped_nsobject.h"
#include "base/strings/sys_string_conversions.h"
#include "chrome/browser/chooser_controller/chooser_controller.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window.h"
#import "chrome/browser/ui/cocoa/base_bubble_controller.h"
#import "chrome/browser/ui/cocoa/browser_window_controller.h"
#import "chrome/browser/ui/cocoa/browser_window_utils.h"
#import "chrome/browser/ui/cocoa/bubble_anchor_helper.h"
#import "chrome/browser/ui/cocoa/device_chooser_content_view_cocoa.h"
#import "chrome/browser/ui/cocoa/info_bubble_view.h"
#import "chrome/browser/ui/cocoa/info_bubble_window.h"
#import "chrome/browser/ui/cocoa/location_bar/location_bar_view_mac.h"
#include "components/bubble/bubble_controller.h"
#include "content/public/browser/native_web_keyboard_event.h"
#include "ui/base/cocoa/cocoa_base_utils.h"
#include "ui/base/cocoa/window_size_constants.h"

@interface ChooserBubbleUiController
    : BaseBubbleController<NSTableViewDataSource, NSTableViewDelegate> {
 @private
  // Bridge to the C++ class that created this object.
  ChooserBubbleUiCocoa* bridge_;  // Weak.
  bool buttonPressed_;

  base::scoped_nsobject<DeviceChooserContentViewCocoa>
      deviceChooserContentView_;
  NSTableView* tableView_;   // Weak.
  NSButton* connectButton_;  // Weak.
  NSButton* cancelButton_;   // Weak.

  Browser* browser_;  // Weak.
}

// Designated initializer.  |browser| and |bridge| must both be non-nil.
- (id)initWithBrowser:(Browser*)browser
    chooserController:(std::unique_ptr<ChooserController>)chooserController
               bridge:(ChooserBubbleUiCocoa*)bridge;

// Makes the bubble visible.
- (void)show;

// Will reposition the bubble based in case the anchor or parent should change.
- (void)updateAnchorPosition;

// Will calculate the expected anchor point for this bubble.
// Should only be used outside this class for tests.
- (NSPoint)getExpectedAnchorPoint;

// Update |tableView_| when chooser options changed.
- (void)updateTableView;

// Determines if the bubble has an anchor in a corner or no anchor at all.
- (info_bubble::BubbleArrowLocation)getExpectedArrowLocation;

// Returns the expected parent for this bubble.
- (NSWindow*)getExpectedParentWindow;

// Called when the "Connect" button is pressed.
- (void)onConnect:(id)sender;

// Called when the "Cancel" button is pressed.
- (void)onCancel:(id)sender;

@end

@implementation ChooserBubbleUiController

- (id)initWithBrowser:(Browser*)browser
    chooserController:(std::unique_ptr<ChooserController>)chooserController
               bridge:(ChooserBubbleUiCocoa*)bridge {
  DCHECK(browser);
  DCHECK(chooserController);
  DCHECK(bridge);

  browser_ = browser;

  base::scoped_nsobject<InfoBubbleWindow> window([[InfoBubbleWindow alloc]
      initWithContentRect:ui::kWindowSizeDeterminedLater
                styleMask:NSBorderlessWindowMask
                  backing:NSBackingStoreBuffered
                    defer:NO]);
  [window setAllowedAnimations:info_bubble::kAnimateNone];
  [window setReleasedWhenClosed:NO];
  if ((self = [super initWithWindow:window
                       parentWindow:[self getExpectedParentWindow]
                         anchoredAt:NSZeroPoint])) {
    [self setShouldCloseOnResignKey:YES];
    [self setShouldOpenAsKeyWindow:YES];
    [[self bubble] setArrowLocation:[self getExpectedArrowLocation]];
    bridge_ = bridge;
    NSNotificationCenter* center = [NSNotificationCenter defaultCenter];
    [center addObserver:self
               selector:@selector(parentWindowDidMove:)
                   name:NSWindowDidMoveNotification
                 object:[self getExpectedParentWindow]];

    base::string16 chooserTitle = chooserController->GetTitle();
    deviceChooserContentView_.reset([[DeviceChooserContentViewCocoa alloc]
        initWithChooserTitle:base::SysUTF16ToNSString(chooserTitle)
           chooserController:std::move(chooserController)]);

    tableView_ = [deviceChooserContentView_ tableView];
    connectButton_ = [deviceChooserContentView_ connectButton];
    cancelButton_ = [deviceChooserContentView_ cancelButton];

    [connectButton_ setTarget:self];
    [connectButton_ setAction:@selector(onConnect:)];
    [cancelButton_ setTarget:self];
    [cancelButton_ setAction:@selector(onCancel:)];
    [tableView_ setDelegate:self];
    [tableView_ setDataSource:self];

    [[[self window] contentView] addSubview:deviceChooserContentView_.get()];
  }

  return self;
}

- (void)windowWillClose:(NSNotification*)notification {
  [[NSNotificationCenter defaultCenter]
      removeObserver:self
                name:NSWindowDidMoveNotification
              object:nil];
  if (!buttonPressed_)
    [deviceChooserContentView_ close];
  bridge_->OnBubbleClosing();
  [super windowWillClose:notification];
}

- (void)parentWindowWillToggleFullScreen:(NSNotification*)notification {
  // Override the base class implementation, which would have closed the bubble.
}

- (void)parentWindowDidResize:(NSNotification*)notification {
  [self setAnchorPoint:[self getExpectedAnchorPoint]];
}

- (void)parentWindowDidMove:(NSNotification*)notification {
  DCHECK(bridge_);
  [self setAnchorPoint:[self getExpectedAnchorPoint]];
}

- (void)show {
  NSRect bubbleFrame =
      [[self window] frameRectForContentRect:[deviceChooserContentView_ frame]];
  if ([[self window] isVisible]) {
    // Unfortunately, calling -setFrame followed by -setFrameOrigin  (called
    // within -setAnchorPoint) causes flickering.  Avoid the flickering by
    // manually adjusting the new frame's origin so that the top left stays the
    // same, and only calling -setFrame.
    NSRect currentWindowFrame = [[self window] frame];
    bubbleFrame.origin = currentWindowFrame.origin;
    bubbleFrame.origin.y = bubbleFrame.origin.y +
                           currentWindowFrame.size.height -
                           bubbleFrame.size.height;
    [[self window] setFrame:bubbleFrame display:YES];
  } else {
    [[self window] setFrame:bubbleFrame display:NO];
    [self setAnchorPoint:[self getExpectedAnchorPoint]];
    [self showWindow:nil];
    [[self window] makeFirstResponder:nil];
    [[self window] setInitialFirstResponder:tableView_];
  }
}

- (NSInteger)numberOfRowsInTableView:(NSTableView*)tableView {
  return [deviceChooserContentView_ numberOfOptions];
}

- (NSView*)tableView:(NSTableView*)tableView
    viewForTableColumn:(NSTableColumn*)tableColumn
                   row:(NSInteger)row {
  return [deviceChooserContentView_ createTableRowView:row].autorelease();
}

- (BOOL)tableView:(NSTableView*)aTableView
    shouldEditTableColumn:(NSTableColumn*)aTableColumn
                      row:(NSInteger)rowIndex {
  return NO;
}

- (CGFloat)tableView:(NSTableView*)tableView heightOfRow:(NSInteger)row {
  return [deviceChooserContentView_ tableRowViewHeight:row];
}

- (void)updateTableView {
  [deviceChooserContentView_ updateTableView];
}

- (void)tableViewSelectionDidChange:(NSNotification*)aNotification {
  [deviceChooserContentView_ updateContentRowColor];
  [connectButton_ setEnabled:[tableView_ numberOfSelectedRows] > 0];
}

// Selection changes (while the mouse button is still down).
- (void)tableViewSelectionIsChanging:(NSNotification*)aNotification {
  [deviceChooserContentView_ updateContentRowColor];
  [connectButton_ setEnabled:[tableView_ numberOfSelectedRows] > 0];
}

- (void)updateAnchorPosition {
  [self setParentWindow:[self getExpectedParentWindow]];
  [self setAnchorPoint:[self getExpectedAnchorPoint]];
}

- (NSPoint)getExpectedAnchorPoint {
  return GetPageInfoAnchorPointForBrowser(browser_);
}

- (info_bubble::BubbleArrowLocation)getExpectedArrowLocation {
  return info_bubble::kTopLeading;
}

- (NSWindow*)getExpectedParentWindow {
  DCHECK(browser_->window());
  return browser_->window()->GetNativeWindow();
}

+ (CGFloat)matchWidthsOf:(NSView*)viewA andOf:(NSView*)viewB {
  NSRect frameA = [viewA frame];
  NSRect frameB = [viewB frame];
  CGFloat width = std::max(NSWidth(frameA), NSWidth(frameB));
  [viewA setFrameSize:NSMakeSize(width, NSHeight(frameA))];
  [viewB setFrameSize:NSMakeSize(width, NSHeight(frameB))];
  return width;
}

+ (void)alignCenterOf:(NSView*)viewA verticallyToCenterOf:(NSView*)viewB {
  NSRect frameA = [viewA frame];
  NSRect frameB = [viewB frame];
  frameA.origin.y =
      NSMinY(frameB) + std::floor((NSHeight(frameB) - NSHeight(frameA)) / 2);
  [viewA setFrameOrigin:frameA.origin];
}

- (void)onConnect:(id)sender {
  buttonPressed_ = true;
  [deviceChooserContentView_ accept];
  if (self.bubbleReference)
    self.bubbleReference->CloseBubble(BUBBLE_CLOSE_ACCEPTED);
  [self close];
}

- (void)onCancel:(id)sender {
  buttonPressed_ = true;
  [deviceChooserContentView_ cancel];
  if (self.bubbleReference)
    self.bubbleReference->CloseBubble(BUBBLE_CLOSE_CANCELED);
  [self close];
}

@end

ChooserBubbleUiCocoa::ChooserBubbleUiCocoa(
    Browser* browser,
    std::unique_ptr<ChooserController> chooser_controller)
    : browser_(browser),
      chooser_bubble_ui_controller_(nil) {
  DCHECK(browser_);
  DCHECK(chooser_controller);
  chooser_bubble_ui_controller_ = [[ChooserBubbleUiController alloc]
        initWithBrowser:browser_
      chooserController:std::move(chooser_controller)
                 bridge:this];
}

ChooserBubbleUiCocoa::~ChooserBubbleUiCocoa() {
  if (chooser_bubble_ui_controller_) {
    [chooser_bubble_ui_controller_ close];
    chooser_bubble_ui_controller_ = nil;
  }
}

void ChooserBubbleUiCocoa::Show(BubbleReference bubble_reference) {
  [chooser_bubble_ui_controller_ setBubbleReference:bubble_reference];
  [chooser_bubble_ui_controller_ show];
  [chooser_bubble_ui_controller_ updateTableView];
}

void ChooserBubbleUiCocoa::Close() {
  if (chooser_bubble_ui_controller_) {
    [chooser_bubble_ui_controller_ close];
    chooser_bubble_ui_controller_ = nil;
  }
}

void ChooserBubbleUiCocoa::UpdateAnchorPosition() {
  if (chooser_bubble_ui_controller_)
    [chooser_bubble_ui_controller_ updateAnchorPosition];
}

void ChooserBubbleUiCocoa::OnBubbleClosing() {
  chooser_bubble_ui_controller_ = nil;
}
