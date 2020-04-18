// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_EXTENSIONS_CHOOSER_DIALOG_COCOA_CONTROLLER_H_
#define CHROME_BROWSER_UI_COCOA_EXTENSIONS_CHOOSER_DIALOG_COCOA_CONTROLLER_H_

#import <Cocoa/Cocoa.h>

#include <memory>

#include "base/mac/scoped_nsobject.h"

class ChooserController;
class ChooserDialogCocoa;
@class DeviceChooserContentViewCocoa;

// Displays a chooser dialog, and notifies the ChooserController
// of the selected option.
@interface ChooserDialogCocoaController
    : NSViewController<NSTableViewDataSource, NSTableViewDelegate> {
  base::scoped_nsobject<DeviceChooserContentViewCocoa>
      deviceChooserContentView_;
  NSTableView* tableView_;   // Weak.
  NSButton* connectButton_;  // Weak.
  NSButton* cancelButton_;   // Weak.

  ChooserDialogCocoa* chooserDialogCocoa_;  // Weak.
}

// Designated initializer. |chooserDialogCocoa| and |chooserController|
// must both be non-nil.
- (instancetype)
initWithChooserDialogCocoa:(ChooserDialogCocoa*)chooserDialogCocoa
         chooserController:
             (std::unique_ptr<ChooserController>)chooserController;

// Called when the "Connect" button is pressed.
- (void)onConnect:(id)sender;

// Called when the "Cancel" button is pressed.
- (void)onCancel:(id)sender;

// Gets the |deviceChooserContentView_|. For testing only.
- (DeviceChooserContentViewCocoa*)deviceChooserContentView;

@end

#endif  // CHROME_BROWSER_UI_COCOA_EXTENSIONS_CHOOSER_DIALOG_COCOA_CONTROLLER_H_
