// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_INFOBARS_INFOBAR_UTILITIES_H_
#define CHROME_BROWSER_UI_COCOA_INFOBARS_INFOBAR_UTILITIES_H_

#import <Cocoa/Cocoa.h>

// InfoBarUtilities provide helper functions to construct infobars with
// similar appearance.
namespace InfoBarUtilities {

// Move the |toMove| view |spacing| pixels before/after the |anchor| view.
// |after| signifies the side of |anchor| on which to place |toMove|.
void MoveControl(NSView* anchor, NSView* toMove, int spacing, bool after);

// Creates a label control in the style we need for the infobar's labels
// within |bounds|.
NSTextField* CreateLabel(NSRect bounds);

// Adds an item with the specified properties to |menu|.
void AddMenuItem(NSMenu* menu,
                 id target,
                 SEL selector,
                 NSString* title,
                 int tag,
                 bool enabled,
                 bool checked,
                 NSString* representedObj);

}  // namespace InfoBarUtilities

#endif  // CHROME_BROWSER_UI_COCOA_INFOBARS_INFOBAR_UTILITIES_H_
