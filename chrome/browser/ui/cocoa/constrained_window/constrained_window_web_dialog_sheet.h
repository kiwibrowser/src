// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_CONSTRAINED_WINDOW_CONSTRAINED_WINDOW_WEB_DIALOG_SHEET_H_
#define CHROME_BROWSER_UI_COCOA_CONSTRAINED_WINDOW_CONSTRAINED_WINDOW_WEB_DIALOG_SHEET_H_

#import "chrome/browser/ui/cocoa/constrained_window/constrained_window_custom_sheet.h"

namespace ui {
class WebDialogDelegate;
}

// Represents a custom sheet for web dialog.
@interface WebDialogConstrainedWindowSheet : CustomConstrainedWindowSheet {
 @private
  NSSize current_size_;
  ui::WebDialogDelegate* web_dialog_delegate_;  // Weak.
}

- (id)initWithCustomWindow:(NSWindow*)customWindow
         webDialogDelegate:(ui::WebDialogDelegate*)delegate;

@end

#endif  // CHROME_BROWSER_UI_COCOA_CONSTRAINED_WINDOW_CONSTRAINED_WINDOW_WEB_DIALOG_SHEET_H_
