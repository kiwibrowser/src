// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_VIEWS_COCOA_BRIDGED_CONTENT_VIEW_H_
#define UI_VIEWS_COCOA_BRIDGED_CONTENT_VIEW_H_

#import <Cocoa/Cocoa.h>

#include "base/strings/string16.h"
#import "ui/base/cocoa/tool_tip_base_view.h"
#import "ui/base/cocoa/tracking_area.h"

namespace ui {
class TextInputClient;
}

namespace views {
class View;
}

// The NSView that sits as the root contentView of the NSWindow, whilst it has
// a views::RootView present. Bridges requests from Cocoa to the hosted
// views::View.
@interface BridgedContentView : ToolTipBaseView<NSTextInputClient,
                                                NSUserInterfaceValidations,
                                                NSDraggingSource> {
 @private
  // Weak. The hosted RootView, owned by hostedView_->GetWidget().
  views::View* hostedView_;

  // Weak. If non-null the TextInputClient of the currently focused View in the
  // hierarchy rooted at |hostedView_|. Owned by the focused View.
  ui::TextInputClient* textInputClient_;

  // A tracking area installed to enable mouseMoved events.
  ui::ScopedCrTrackingArea cursorTrackingArea_;

  // The keyDown event currently being handled, nil otherwise.
  NSEvent* keyDownEvent_;

  // Whether there's an active key down event which is not handled yet.
  BOOL hasUnhandledKeyDownEvent_;

  // The last tooltip text, used to limit updates.
  base::string16 lastTooltipText_;
}

@property(readonly, nonatomic) views::View* hostedView;
@property(assign, nonatomic) ui::TextInputClient* textInputClient;
@property(assign, nonatomic) BOOL drawMenuBackgroundForBlur;

// Initialize the NSView -> views::View bridge. |viewToHost| must be non-NULL.
- (id)initWithView:(views::View*)viewToHost;

// Clear the hosted view. For example, if it is about to be destroyed.
- (void)clearView;

// Process a mouse event captured while the widget had global mouse capture.
- (void)processCapturedMouseEvent:(NSEvent*)theEvent;

// Mac's version of views::corewm::TooltipController::UpdateIfRequired().
// Updates the tooltip on the ToolTipBaseView if the text needs to change.
// |locationInContent| is the position from the top left of the window's
// contentRect (also this NSView's frame), as given by a ui::LocatedEvent.
- (void)updateTooltipIfRequiredAt:(const gfx::Point&)locationInContent;

// Notifies the associated FocusManager whether full keyboard access is enabled
// or not.
- (void)updateFullKeyboardAccess;

@end

#endif  // UI_VIEWS_COCOA_BRIDGED_CONTENT_VIEW_H_
