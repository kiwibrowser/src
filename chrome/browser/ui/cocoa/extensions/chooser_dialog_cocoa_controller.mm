// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/extensions/chooser_dialog_cocoa_controller.h"

#include "base/strings/sys_string_conversions.h"
#include "chrome/browser/chooser_controller/chooser_controller.h"
#import "chrome/browser/ui/cocoa/device_chooser_content_view_cocoa.h"
#import "chrome/browser/ui/cocoa/extensions/chooser_dialog_cocoa.h"

@implementation ChooserDialogCocoaController

- (instancetype)
initWithChooserDialogCocoa:(ChooserDialogCocoa*)chooserDialogCocoa
         chooserController:
             (std::unique_ptr<ChooserController>)chooserController {
  DCHECK(chooserDialogCocoa);
  DCHECK(chooserController);
  if ((self = [super init]))
    chooserDialogCocoa_ = chooserDialogCocoa;

  base::string16 chooserTitle = chooserController->GetTitle();
  deviceChooserContentView_.reset([[DeviceChooserContentViewCocoa alloc]
      initWithChooserTitle:base::SysUTF16ToNSString(chooserTitle)
         chooserController:std::move(chooserController)]);

  tableView_ = [deviceChooserContentView_ tableView];
  connectButton_ = [deviceChooserContentView_ connectButton];
  cancelButton_ = [deviceChooserContentView_ cancelButton];

  [connectButton_ setTarget:self];
  [connectButton_ setAction:@selector(onConnect:)];
  [cancelButton_ setTarget:self];
  [cancelButton_ setAction:@selector(onCancel:)];
  [tableView_ setDelegate:self];
  [tableView_ setDataSource:self];
  self.view = deviceChooserContentView_;
  [deviceChooserContentView_ updateTableView];

  return self;
}

- (NSInteger)numberOfRowsInTableView:(NSTableView*)tableView {
  return [deviceChooserContentView_ numberOfOptions];
}

- (NSView*)tableView:(NSTableView*)tableView
    viewForTableColumn:(NSTableColumn*)tableColumn
                   row:(NSInteger)row {
  return [deviceChooserContentView_ createTableRowView:row].autorelease();
}

- (BOOL)tableView:(NSTableView*)aTableView
    shouldEditTableColumn:(NSTableColumn*)aTableColumn
                      row:(NSInteger)rowIndex {
  return NO;
}

- (CGFloat)tableView:(NSTableView*)tableView heightOfRow:(NSInteger)row {
  return [deviceChooserContentView_ tableRowViewHeight:row];
}

- (void)tableViewSelectionDidChange:(NSNotification*)aNotification {
  [deviceChooserContentView_ updateContentRowColor];
  [connectButton_ setEnabled:[tableView_ numberOfSelectedRows] > 0];
}

// Selection changes (while the mouse button is still down).
- (void)tableViewSelectionIsChanging:(NSNotification*)aNotification {
  [deviceChooserContentView_ updateContentRowColor];
  [connectButton_ setEnabled:[tableView_ numberOfSelectedRows] > 0];
}

- (void)onConnect:(id)sender {
  [deviceChooserContentView_ accept];
  chooserDialogCocoa_->Dismissed();
}

- (void)onCancel:(id)sender {
  [deviceChooserContentView_ cancel];
  chooserDialogCocoa_->Dismissed();
}

- (DeviceChooserContentViewCocoa*)deviceChooserContentView {
  return deviceChooserContentView_.get();
}

@end
