// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_MENU_BUTTON_H_
#define CHROME_BROWSER_UI_COCOA_MENU_BUTTON_H_

#import <Cocoa/Cocoa.h>

#include "base/mac/scoped_nsobject.h"
#import "chrome/browser/ui/cocoa/toolbar/toolbar_button_cocoa.h"

// This a button which displays a user-provided menu "attached" below it upon
// being clicked or dragged (or clicked and held). It expects a
// |ClickHoldButtonCell| as cell.
//
// There are two different behaviors of this button depending on the value of
// the |openMenuOnClick| property. If YES, the target-action mechanism will be
// handled internally to always show the menu when clicked. This behavior is
// used for the App menu, for example. When the property is NO, the button can
// have a separate target-action but will open the menu when clicked and held.
// This is used for the toolbar back/forward buttons, which have a primary
// action and the menu as a secondary click-hold action. The default value is NO
// so that custom actions can be hooked up in Interface Builder.
// The click-hold behavior can be disabled entirely through the
// |openMenuOnClickHold| property.
@interface MenuButton : ToolbarButton {
 @private
  base::scoped_nsobject<NSMenu> attachedMenu_;
  BOOL openMenuOnClick_;
  BOOL openMenuOnRightClick_;
  BOOL openMenuOnClickHold_;
  base::scoped_nsobject<NSPopUpButtonCell> popUpCell_;
}

// The menu to display. Note that it should have no (i.e., a blank) title and
// that the 0-th entry should be blank (and won't be displayed). (This is
// because we use a pulldown list, for which Cocoa uses the 0-th item as "title"
// in the button. This might change if we ever switch to a pop-up. Our direct
// use of the given NSMenu object means that the one can set and use NSMenu's
// delegate as usual.)
@property(retain, nonatomic) IBOutlet NSMenu* attachedMenu;

// Whether or not to open the menu when the button is clicked. Otherwise, the
// menu will only be opened when clicked and held, if that behavior has not
// been disabled.
@property(assign, nonatomic) BOOL openMenuOnClick;

// Whether or not to open the menu when the right button is clicked.
@property(assign, nonatomic) BOOL openMenuOnRightClick;

// Whether the menu should open if a user clicks and holds on the button. The
// default is YES. This does require that the user drag the mouse a small
// distance "pull open" the menu.
@property(assign, nonatomic) BOOL openMenuOnClickHold;

// Returns the rectangle that menus are anchored at. Can be overridden by
// subclasses, returns -bounds by default.
- (NSRect)menuRect;

@end  // @interface MenuButton

// Available for subclasses.
@interface MenuButton (Protected)
- (void)configureCell;
@end

#endif  // CHROME_BROWSER_UI_COCOA_MENU_BUTTON_H_
