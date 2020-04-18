// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_ONE_CLICK_SIGNIN_VIEW_CONTROLLER_H_
#define CHROME_BROWSER_UI_COCOA_ONE_CLICK_SIGNIN_VIEW_CONTROLLER_H_

#import <Cocoa/Cocoa.h>

#include "base/callback.h"
#include "base/mac/scoped_nsobject.h"
#include "chrome/browser/ui/browser_window.h"

@class BrowserWindowController;
namespace content {
class WebContents;
}
@class HyperlinkTextView;

// View controller for the one-click signin confirmation UI.
@interface OneClickSigninViewController : NSViewController<NSTextViewDelegate> {
 @private
  IBOutlet NSTextField* messageTextField_;
  IBOutlet NSTextField* titleTextField_;
  IBOutlet NSTextField* informativePlaceholderTextField_;
  IBOutlet NSButton* advancedLink_;
  IBOutlet NSButton* closeButton_;

  // This is YES if this is the modal sync confirmation dialog.
  BOOL isSyncDialog_;

  // This is YES if the user clicked the Learn More link before another action.
  BOOL clickedLearnMore_;

  // The user's email address to be used for sync.
  base::string16 email_;

  // Alternate error message to be displayed.
  base::scoped_nsobject<NSString> errorMessage_;

  // Text fields don't work as well with embedded links as text views, but
  // text views cannot conveniently be created in IB. The xib file contains
  // a text field |informativePlaceholderTextField_| that's replaced by this
  // text view |promo_| in -awakeFromNib.
  base::scoped_nsobject<HyperlinkTextView> informativeTextView_;
  BrowserWindow::StartSyncCallback startSyncCallback_;
  base::Closure closeCallback_;
  content::WebContents* webContents_;
}

// Initializes the controller from a nib file, with an alternate |errorMessage|
// that can be displayed in the case of an authentication error.
// |syncCallback| is called to start sync for the given |email|, if
// |isSyncDialog| is YES. |webContents| is used to open the Learn More and
// Advanced links and |callback| is called when the view is closing.
- (id)initWithNibName:(NSString*)nibName
          webContents:(content::WebContents*)webContents
         syncCallback:(const BrowserWindow::StartSyncCallback&)syncCallback
        closeCallback:(const base::Closure&)callback
         isSyncDialog:(BOOL)isSyncDialog
                email:(const base::string16&)email
         errorMessage:(NSString*)errorMessage;

// Called before the view is closed.
- (void)viewWillClose;

// Starts sync and closes the bubble.
- (IBAction)ok:(id)sender;

// Starts sync and closes the bubble.
- (IBAction)onClickClose:(id)sender;

// Does not start sync and closes the bubble.
- (IBAction)onClickUndo:(id)sender;

// Calls |advancedCallback_|.
- (IBAction)onClickAdvancedLink:(id)sender;

@end

@interface OneClickSigninViewController (TestingAPI)
- (NSTextView*)linkViewForTesting;
@end

#endif  // CHROME_BROWSER_UI_COCOA_ONE_CLICK_SIGNIN_VIEW_CONTROLLER_H_
