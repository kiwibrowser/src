// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_BUBBLE_COMBOBOX_H_
#define CHROME_BROWSER_UI_COCOA_BUBBLE_COMBOBOX_H_

#import <Cocoa/Cocoa.h>

namespace ui {
class ComboboxModel;
}  // namespace ui

// An NSPopUpButton that auto-populates from a ui::ComboboxModel.
// By default it comes with a border, small font size, and small control size.
@interface BubbleCombobox : NSPopUpButton
// Does not take ownership nor store a pointer to |model|; it is used only for
// population of the combobox.
- (id)initWithFrame:(NSRect)frame
          pullsDown:(BOOL)pullsDown
              model:(ui::ComboboxModel*)model;
@end

#endif  // CHROME_BROWSER_UI_COCOA_BUBBLE_COMBOBOX_H_
