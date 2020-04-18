// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/constrained_window/constrained_window_alert.h"

#import "base/logging.h"
#import "chrome/browser/ui/cocoa/chrome_style.h"
#import "chrome/browser/ui/cocoa/constrained_window/constrained_window_button.h"
#import "chrome/browser/ui/cocoa/constrained_window/constrained_window_control_utils.h"
#import "chrome/browser/ui/cocoa/constrained_window/constrained_window_custom_window.h"
#import "chrome/browser/ui/cocoa/hover_close_button.h"
#include "skia/ext/skia_utils_mac.h"
#include "third_party/google_toolbox_for_mac/src/AppKit/GTMUILocalizerAndLayoutTweaker.h"
#include "ui/base/cocoa/controls/hyperlink_button_cell.h"
#include "ui/base/cocoa/window_size_constants.h"

namespace {

const CGFloat kWindowMinWidth = 500;
const CGFloat kButtonGap = 6;

}  // namespace

@interface ConstrainedWindowAlert ()
// Position the alert buttons within the given window width.
- (void)layoutButtonsWithWindowWidth:(CGFloat)windowWidth;
// Resize the given text field to fit within the window and position it starting
// at |yPos|. Returns the new value of yPos.
- (CGFloat)layoutTextField:(NSTextField*)textField
                      yPos:(CGFloat)yPos
               windowWidth:(CGFloat)windowWidth;
// If a link has been set, resizes the link to fit within the window and
// position it starting at |yPos|. Returns the new value of yPos.
- (CGFloat)layoutLinkAtYPos:(CGFloat)yPos
                windowWidth:(CGFloat)windowWidth;
// Positions the accessory view starting at yPos. Returns the new value of yPos.
- (CGFloat)layoutAccessoryViewAtYPos:(CGFloat)yPos;
// Update the position of the close button.
- (void)layoutCloseButtonWithWindowWidth:(CGFloat)windowWidth
                            windowHeight:(CGFloat)windowHeight;
@end

@implementation ConstrainedWindowAlert

- (id)init {
  if ((self = [super init])) {
    window_.reset([[ConstrainedWindowCustomWindow alloc]
        initWithContentRect:ui::kWindowSizeDeterminedLater]);
    NSView* contentView = [window_ contentView];

    ui::ResourceBundle& rb = ui::ResourceBundle::GetSharedInstance();

    informativeTextField_.reset([constrained_window::CreateLabel() retain]);
    [informativeTextField_ setSelectable:YES];
    [informativeTextField_ setAlignment:NSNaturalTextAlignment];
    if (@available(macOS 10.10, *)) {
      [informativeTextField_ setLineBreakMode:NSLineBreakByWordWrapping];
    } else {
      [[informativeTextField_ cell] setLineBreakMode:NSLineBreakByWordWrapping];
    }
    [informativeTextField_
        setFont:rb.GetFont(chrome_style::kTextFontStyle).GetNativeFont()];
    [contentView addSubview:informativeTextField_];

    messageTextField_.reset([constrained_window::CreateLabel() retain]);
    [messageTextField_ setSelectable:YES];
    [messageTextField_ setAlignment:NSNaturalTextAlignment];
    if (@available(macOS 10.10, *)) {
      [messageTextField_ setLineBreakMode:NSLineBreakByWordWrapping];
    } else {
      [[messageTextField_ cell] setLineBreakMode:NSLineBreakByWordWrapping];
    }
    [messageTextField_
        setFont:rb.GetFont(chrome_style::kTitleFontStyle).GetNativeFont()];
    [contentView addSubview:messageTextField_];

    closeButton_.reset(
        [[WebUIHoverCloseButton alloc] initWithFrame:NSZeroRect]);
    [contentView addSubview:closeButton_];
  }
  return self;
}

- (NSString*)informativeText {
  return [informativeTextField_ stringValue];
}

- (void)setInformativeText:(NSString*)string {
  [informativeTextField_ setStringValue:string];
}

- (NSString*)messageText {
  return [messageTextField_ stringValue];
}

- (void)setMessageText:(NSString*)string {
  [messageTextField_ setStringValue:string];
}

- (void)setLinkText:(NSString*)text target:(id)target action:(SEL)action {
  if (![text length]) {
    [linkView_ removeFromSuperview];
    linkView_.reset(nil);
    return;
  }

  if (!linkView_.get()) {
    linkView_.reset(
        [[HyperlinkButtonCell buttonWithString:[NSString string]] retain]);
    [[window_ contentView] addSubview:linkView_];
  }

  [linkView_ setTitle:text];
  [linkView_ setTarget:target];
  [linkView_ setAction:action];
}

- (NSView*)accessoryView {
  return accessoryView_;
}

- (void)setAccessoryView:(NSView*)accessoryView {
  [accessoryView_ removeFromSuperview];
  accessoryView_.reset([accessoryView retain]);
  [[window_ contentView] addSubview:accessoryView_];
}

- (NSArray*)buttons {
  return buttons_;
}

- (NSButton*)closeButton {
  return closeButton_;
}

- (NSWindow*)window {
  return window_;
}

- (void)addButtonWithTitle:(NSString*)title
             keyEquivalent:(NSString*)keyEquivalent
                    target:(id)target
                    action:(SEL)action {
  if (!buttons_.get())
    buttons_.reset([[NSMutableArray alloc] init]);
  base::scoped_nsobject<NSButton> button(
      [[ConstrainedWindowButton alloc] initWithFrame:NSZeroRect]);
  [button setTitle:title];
  [button setKeyEquivalent:keyEquivalent];
  [button setTarget:target];
  [button setAction:action];
  [buttons_ addObject:button];
  [[window_ contentView] addSubview:button];
}

- (void)layout {
  // Button width.
  CGFloat buttonWidth = 0;
  for (NSButton* button in buttons_.get()) {
    [button sizeToFit];
    NSSize size = [button frame].size;
    buttonWidth += size.width;
  }
  if ([buttons_ count])
    buttonWidth += ([buttons_ count] - 1) * kButtonGap;

  // Window width.
  CGFloat windowWidth = buttonWidth;
  if (accessoryView_.get())
    windowWidth = std::max(windowWidth, NSWidth([accessoryView_ frame]));
  windowWidth += chrome_style::kHorizontalPadding * 2;
  windowWidth = std::max(windowWidth, kWindowMinWidth);

  // Layout controls.
  [self layoutButtonsWithWindowWidth:windowWidth];
  CGFloat curY = [buttons_ count] ? NSMaxY([[buttons_ lastObject] frame])
      : chrome_style::kClientBottomPadding;
  CGFloat availableMessageWidth =
      windowWidth - chrome_style::GetCloseButtonSize() - kButtonGap;
  curY = [self layoutLinkAtYPos:curY
                    windowWidth:availableMessageWidth];
  curY = [self layoutAccessoryViewAtYPos:curY];
  curY = [self layoutTextField:informativeTextField_
                          yPos:curY
                   windowWidth:windowWidth];
  curY = [self layoutTextField:messageTextField_
                          yPos:curY
                   windowWidth:availableMessageWidth];

  CGFloat windowHeight = curY + chrome_style::kTitleTopPadding;
  [self layoutCloseButtonWithWindowWidth:windowWidth
                            windowHeight:windowHeight];

  // Update window frame.
  NSRect windowFrame = NSMakeRect(0, 0, windowWidth, windowHeight);
  windowFrame = [window_ frameRectForContentRect:windowFrame];
  [window_ setFrame:windowFrame display:NO];
}

- (void)layoutButtonsWithWindowWidth:(CGFloat)windowWidth {
  // Layout first 2 button right to left.
  CGFloat curX = windowWidth - chrome_style::kHorizontalPadding;
  const int buttonCount = [buttons_ count];
  for (int i = 0; i < std::min(2, buttonCount); ++i) {
    NSButton* button = [buttons_ objectAtIndex:i];
    NSRect rect = [button frame];
    rect.origin.x = curX - NSWidth(rect);
    rect.origin.y = chrome_style::kClientBottomPadding;
    [button setFrameOrigin:rect.origin];
    curX = NSMinX(rect) - kButtonGap;
  }

  // Layout remaining buttons left to right.
  curX = chrome_style::kHorizontalPadding;
  for (int i = buttonCount - 1; i >= 2; --i) {
    NSButton* button = [buttons_ objectAtIndex:i];
    [button setFrameOrigin:
        NSMakePoint(curX, chrome_style::kClientBottomPadding)];
    curX += NSMaxX([button frame]) + kButtonGap;
  }
}

- (CGFloat)layoutTextField:(NSTextField*)textField
                      yPos:(CGFloat)yPos
               windowWidth:(CGFloat)windowWidth {
  if (![[textField stringValue] length]) {
    [textField setHidden:YES];
    return yPos;
  }

  [textField setHidden:NO];
  NSRect rect;
  rect.origin.y = yPos + chrome_style::kRowPadding;
  rect.origin.x = chrome_style::kHorizontalPadding;
  rect.size.width = windowWidth -
      chrome_style::kHorizontalPadding * 2;
  rect.size.height = 1;
  [textField setFrame:rect];
  [GTMUILocalizerAndLayoutTweaker sizeToFitFixedWidthTextField:textField];
  return NSMaxY([textField frame]);
}

- (CGFloat)layoutLinkAtYPos:(CGFloat)yPos
                windowWidth:(CGFloat)windowWidth {
  if (!linkView_.get())
    return yPos;

  NSRect availableBounds = NSMakeRect(
      0,
      0,
      windowWidth - chrome_style::kHorizontalPadding * 2,
      CGFLOAT_MAX);
  NSSize size = [[linkView_ cell] cellSizeForBounds:availableBounds];

  NSRect rect;
  rect.origin.y = yPos + chrome_style::kRowPadding;
  rect.origin.x = chrome_style::kHorizontalPadding;
  rect.size = size;
  [linkView_ setFrame:rect];
  return NSMaxY([linkView_ frame]);
}

- (CGFloat)layoutAccessoryViewAtYPos:(CGFloat)yPos {
  if (!accessoryView_.get())
    return yPos;
  NSRect frame = [accessoryView_ frame];
  frame.origin.y = yPos + chrome_style::kRowPadding;
  frame.origin.x = chrome_style::kHorizontalPadding;
  [accessoryView_ setFrameOrigin:frame.origin];
  return NSMaxY(frame);
}

- (void)layoutCloseButtonWithWindowWidth:(CGFloat)windowWidth
                            windowHeight:(CGFloat)windowHeight {
  NSRect frame;
  frame.size.width = chrome_style::GetCloseButtonSize();
  frame.size.height = chrome_style::GetCloseButtonSize();
  frame.origin.x = windowWidth -
      chrome_style::kCloseButtonPadding - NSWidth(frame);
  frame.origin.y = windowHeight -
      chrome_style::kCloseButtonPadding - NSHeight(frame);
  [closeButton_ setFrame:frame];
}

- (NSButton*)linkView {
  return linkView_;
}

@end
