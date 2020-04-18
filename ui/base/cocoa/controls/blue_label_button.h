// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_BASE_COCOA_CONTROLS_BLUE_LABEL_BUTTON_H_
#define UI_BASE_COCOA_CONTROLS_BLUE_LABEL_BUTTON_H_

#import <Cocoa/Cocoa.h>

#import "ui/base/cocoa/hover_button.h"
#include "ui/base/ui_base_export.h"

// A rectangular blue NSButton that reacts to hover, focus and lit states. It
// can contain an arbitrary single-line text label, and will be sized to fit the
// font height and label width.
UI_BASE_EXPORT
@interface BlueLabelButton : HoverButton
@end

#endif  // UI_BASE_COCOA_CONTROLS_BLUE_LABEL_BUTTON_H_
