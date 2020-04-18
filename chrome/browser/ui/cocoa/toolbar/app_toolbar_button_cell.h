// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_TOOLBAR_APP_TOOLBAR_BUTTON_CELL_H_
#define CHROME_BROWSER_UI_COCOA_TOOLBAR_APP_TOOLBAR_BUTTON_CELL_H_

#import <Cocoa/Cocoa.h>

#include <memory>

#import "chrome/browser/ui/cocoa/clickhold_button_cell.h"


// Cell for the app toolbar button. This is used to draw the app menu icon
// and paint severity levels.
@interface AppToolbarButtonCell : ClickHoldButtonCell {
}

@end

#endif  // CHROME_BROWSER_UI_COCOA_TOOLBAR_APP_TOOLBAR_BUTTON_CELL_H_
