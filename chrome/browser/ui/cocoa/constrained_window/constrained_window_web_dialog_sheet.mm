// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/constrained_window/constrained_window_web_dialog_sheet.h"

#import "ui/base/cocoa/window_size_constants.h"
#include "ui/gfx/geometry/size.h"
#include "ui/web_dialogs/web_dialog_delegate.h"

@implementation WebDialogConstrainedWindowSheet

- (id)initWithCustomWindow:(NSWindow*)customWindow
         webDialogDelegate:(ui::WebDialogDelegate*)delegate {
  if (self = [super initWithCustomWindow:customWindow]) {
    current_size_ = ui::kWindowSizeDeterminedLater.size;
    web_dialog_delegate_ = delegate;
  }

  return self;
}

- (void)updateSheetPosition {
  if (web_dialog_delegate_) {
    gfx::Size size;
    web_dialog_delegate_->GetDialogSize(&size);

    // If the dialog has autoresizing enabled, |size| will be empty. Use the
    // last known dialog size.
    NSSize content_size = size.IsEmpty() ? current_size_ :
        NSMakeSize(size.width(), size.height());
    [customWindow_ setContentSize:content_size];
  }
  [super updateSheetPosition];
}

- (void)resizeWithNewSize:(NSSize)size {
  DCHECK(size.height > 0 && size.width > 0);
  current_size_ = size;
  [customWindow_ setContentSize:current_size_];

  // self's updateSheetPosition() sets |customWindow_|'s contentSize to a
  // fixed dialog size. Here, we want to resize to |size| instead. Use
  // super rather than self to bypass the setContentSize() call for the fixed
  // size.
  [super updateSheetPosition];
}

@end
