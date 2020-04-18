// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_APP_MENU_APP_MENU_BUTTON_CELL_H_
#define CHROME_BROWSER_UI_COCOA_APP_MENU_APP_MENU_BUTTON_CELL_H_

#import <Cocoa/Cocoa.h>

// The AppMenuButtonCell overrides drawing the background gradient to use the
// same colors as NSSmallSquareBezelStyle but as a smooth gradient, rather than
// two blocks of colors.  This also uses the blue menu highlight color for the
// pressed state.
@interface AppMenuButtonCell : NSButtonCell {
}

@end

#endif  // CHROME_BROWSER_UI_COCOA_APP_MENU_APP_MENU_BUTTON_CELL_H_
