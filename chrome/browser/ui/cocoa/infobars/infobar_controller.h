// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_INFOBARS_INFOBAR_CONTROLLER_H_
#define CHROME_BROWSER_UI_COCOA_INFOBARS_INFOBAR_CONTROLLER_H_

#import <Cocoa/Cocoa.h>

#include "base/mac/scoped_nsobject.h"
#include "base/memory/weak_ptr.h"

class InfoBarCocoa;
@class InfoBarBackgroundView;

namespace infobars {
class InfoBarDelegate;
}

// A controller for an infobar in the browser window.  There is one
// controller per infobar view.  The base InfoBarController is able to
// draw an icon, a text message, and a close button.  Subclasses can
// override addAdditionalControls to customize the UI.
@interface InfoBarController : NSViewController<NSTextViewDelegate> {
 @private
  base::WeakPtr<InfoBarCocoa> infobar_;

 @protected
  IBOutlet InfoBarBackgroundView* infoBarView_;
  IBOutlet NSImageView* image_;
  IBOutlet NSTextField* labelPlaceholder_;
  IBOutlet NSButton* okButton_;
  IBOutlet NSButton* cancelButton_;
  IBOutlet NSButton* closeButton_;

  // Text fields don't work as well with embedded links as text views, but
  // text views cannot conveniently be created in IB. The xib file contains
  // a text field |labelPlaceholder_| that's replaced by this text view |label_|
  // in -awakeFromNib.
  base::scoped_nsobject<NSTextView> label_;
}

@property(nonatomic, readonly) infobars::InfoBarDelegate* delegate;
@property(nonatomic, readonly) InfoBarCocoa* infobar;

// Initializes a new InfoBarController and takes a WeakPtr to |infobar|.
- (id)initWithInfoBar:(InfoBarCocoa*)infobar;

// Returns YES if the infobar is owned.  If this is NO, it is not safe to call
// any delegate functions, since they might attempt to access the owner.  Code
// should generally just do nothing at all in this case (once we're closing, all
// controls can safely just go dead).
- (BOOL)isOwned;

// Called when someone clicks on the OK or Cancel buttons.  Subclasses
// must override if they do not hide the buttons.
- (void)ok:(id)sender;
- (void)cancel:(id)sender;

// Called when someone clicks on the close button.  Dismisses the infobar
// without taking any action.
// NOTE: Subclasses should not call this to close the infobar as it will lead to
// errors in stat counting.  Call -removeSelf instead.
- (IBAction)dismiss:(id)sender;

// Asks the container controller to remove the infobar for this delegate.  This
// call will trigger a notification that starts the infobar animating closed.
- (void)removeSelf;

// Subclasses can override this method to add additional controls to
// the infobar view.  This method is called by awakeFromNib.  The
// default implementation does nothing.
- (void)addAdditionalControls;

// Subclasses must override this method to perform cleanup just before the
// infobar hides.
- (void)infobarWillHide;

// Subclasses must override this method to perform cleanup just before the
// infobar closes.
- (void)infobarWillClose;

// Removes the OK and Cancel buttons and resizes the textfield to use the
// space.
- (void)removeButtons;

@end

@interface InfoBarController (Protected)
// Disables the provided menu.  Subclasses should call this for each popup menu
// in -infobarWillClose.
- (void)disablePopUpMenu:(NSMenu*)menu;
@end

/////////////////////////////////////////////////////////////////////////
// InfoBarController subclasses, one for each InfoBarDelegate
// subclass.  Each of these subclasses overrides addAdditionalControls to
// configure its view as necessary.

#endif  // CHROME_BROWSER_UI_COCOA_INFOBARS_INFOBAR_CONTROLLER_H_
