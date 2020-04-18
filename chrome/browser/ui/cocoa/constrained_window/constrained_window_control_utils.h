// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_CONSTRAINED_WINDOW_CONSTRAINED_WINDOW_CONTROL_UTILS_H_
#define CHROME_BROWSER_UI_COCOA_CONSTRAINED_WINDOW_CONSTRAINED_WINDOW_CONTROL_UTILS_H_

#import <Cocoa/Cocoa.h>

#include "ui/base/resource/resource_bundle.h"

namespace constrained_window {

// Creates a label control.
NSTextField* CreateLabel();

// Helper function to create constrained window label string with the given
// font and alignment.
NSAttributedString* GetAttributedLabelString(
    NSString* string,
    ui::ResourceBundle::FontStyle fontStyle,
    NSTextAlignment alignment,
    NSLineBreakMode lineBreakMode);

// Create a new NSTextField and add it to the specified parent.
NSTextField* AddTextField(NSView* parent,
                          const base::string16& message,
                          const ui::ResourceBundle::FontStyle& font_style);

// Add the given button to the specified parent and customize it
// according to the parameters.
void AddButton(NSView* parent,
               NSButton* button,
               int title_id,
               id target,
               SEL action,
               BOOL should_auto_size);

// Determine the frame required to fit the content of a string.  Uses the
// provided height and width as preferred dimensions, where a value of
// 0.0 indicates no preference.
NSRect ComputeFrame(NSAttributedString* text, CGFloat width, CGFloat height);

}  // namespace constrained_window

#endif  // CHROME_BROWSER_UI_COCOA_CONSTRAINED_WINDOW_CONSTRAINED_WINDOW_CONTROL_UTILS_H_
