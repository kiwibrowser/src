// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_BASE_COCOA_CONTROLS_HOVER_IMAGE_MENU_BUTTON_H_
#define UI_BASE_COCOA_CONTROLS_HOVER_IMAGE_MENU_BUTTON_H_

#import <Cocoa/Cocoa.h>

#import "ui/base/cocoa/tracking_area.h"
#include "ui/base/ui_base_export.h"

@class HoverImageMenuButtonCell;

// An NSPopUpButton that additionally tracks mouseover state and calls
// [[self cell] setHovered:flag] when the hover state changes. Uses
// HoverImageMenuButtonCell as the default cellClass. Note that the menu item at
// index 0 is ignored and client code should populate it with a dummy item.
UI_BASE_EXPORT
@interface HoverImageMenuButton : NSPopUpButton {
 @private
  ui::ScopedCrTrackingArea trackingArea_;
}

- (HoverImageMenuButtonCell*)hoverImageMenuButtonCell;

@end

#endif  // UI_BASE_COCOA_CONTROLS_HOVER_IMAGE_MENU_BUTTON_H_
