// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_DEVICE_CHOOSER_CONTENT_VIEW_COCOA_H_
#define CHROME_BROWSER_UI_COCOA_DEVICE_CHOOSER_CONTENT_VIEW_COCOA_H_

#import <Cocoa/Cocoa.h>

#include <memory>

#include "base/mac/scoped_nsobject.h"

class ChooserContentViewController;
class ChooserController;
@class SpinnerView;

// A chooser content view class that user can select an option.
@interface DeviceChooserContentViewCocoa : NSView {
 @private
  base::scoped_nsobject<NSTextField> titleView_;
  base::scoped_nsobject<NSButton> adapterOffHelpButton_;
  base::scoped_nsobject<NSTextField> adapterOffMessage_;
  base::scoped_nsobject<NSScrollView> scrollView_;
  base::scoped_nsobject<NSTableColumn> tableColumn_;
  base::scoped_nsobject<NSTableView> tableView_;
  base::scoped_nsobject<SpinnerView> spinner_;
  base::scoped_nsobject<NSButton> connectButton_;
  base::scoped_nsobject<NSButton> cancelButton_;
  base::scoped_nsobject<NSBox> separator_;
  base::scoped_nsobject<NSButton> helpButton_;
  base::scoped_nsobject<NSTextField> scanningMessage_;
  base::scoped_nsobject<NSTextField> wordConnector_;
  base::scoped_nsobject<NSButton> rescanButton_;
  std::unique_ptr<ChooserController> chooserController_;
  std::unique_ptr<ChooserContentViewController> chooserContentViewController_;
  CGFloat separatorOriginY_;
}

// Designated initializer.
- (instancetype)initWithChooserTitle:(NSString*)chooserTitle
                   chooserController:
                       (std::unique_ptr<ChooserController>)chooserController;

// Creates the title for the chooser.
- (base::scoped_nsobject<NSTextField>)createChooserTitle:(NSString*)title;

// Creates a table row view for the chooser.
- (base::scoped_nsobject<NSView>)createTableRowView:(NSInteger)row;

// The height of a table row view.
- (CGFloat)tableRowViewHeight:(NSInteger)row;

// Creates a button with |title|.
- (base::scoped_nsobject<NSButton>)createButtonWithTitle:(NSString*)title;

// Creates the "Connect" button.
- (base::scoped_nsobject<NSButton>)createConnectButton;

// Creates the "Cancel" button.
- (base::scoped_nsobject<NSButton>)createCancelButton;

// Creates the separator.
- (base::scoped_nsobject<NSBox>)createSeparator;

// Creates a hyperlink button with |text|.
- (base::scoped_nsobject<NSButton>)createHyperlinkButtonWithText:
    (NSString*)text;

// Gets the adapter off help button.
- (NSButton*)adapterOffHelpButton;

// Gets the table view for the chooser.
- (NSTableView*)tableView;

// Gets the spinner.
- (SpinnerView*)spinner;

// Gets the "Connect" button.
- (NSButton*)connectButton;

// Gets the "Cancel" button.
- (NSButton*)cancelButton;

// Gets the "Get help" button.
- (NSButton*)helpButton;

// Gets the scanning message.
- (NSTextField*)scanningMessage;

// Gets the word connector.
- (NSTextField*)wordConnector;

// Gets the "Re-scan" button.
- (NSButton*)rescanButton;

// The number of options in the |tableView_|.
- (NSInteger)numberOfOptions;

// The |index|th option string which is listed in the chooser.
- (NSString*)optionAtIndex:(NSInteger)index;

// Update |tableView_| when chooser options changed.
- (void)updateTableView;

// Called when the "Connect" button is pressed.
- (void)accept;

// Called when the "Cancel" button is pressed.
- (void)cancel;

// Called when the chooser is closed.
- (void)close;

// Called when the adapter off help button is pressed.
- (void)onAdapterOffHelp:(id)sender;

// Called when "Re-scan" button is pressed.
- (void)onRescan:(id)sender;

// Called when the "Get help" button is pressed.
- (void)onHelpPressed:(id)sender;

// Update the color of the image and text in the table row view.
- (void)updateContentRowColor;

// Gets the image from table row view. For testing and internal use only.
- (NSImageView*)tableRowViewImage:(NSInteger)row;

// Gets the text from table row view. For testing and internal use only.
- (NSTextField*)tableRowViewText:(NSInteger)row;

// Gets the paired status from table row view. For testing and internal use
// only.
- (NSTextField*)tableRowViewPairedStatus:(NSInteger)row;

@end

#endif  // CHROME_BROWSER_UI_COCOA_DEVICE_CHOOSER_CONTENT_VIEW_COCOA_H_
