// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_CONSTRAINED_WINDOW_CONSTRAINED_WINDOW_MAC_H_
#define CHROME_BROWSER_UI_COCOA_CONSTRAINED_WINDOW_CONSTRAINED_WINDOW_MAC_H_

#import <Cocoa/Cocoa.h>

#include "base/mac/scoped_nsobject.h"
#include "components/web_modal/web_contents_modal_dialog_manager.h"

namespace content {
class WebContents;
}
class ConstrainedWindowMac;
class SingleWebContentsDialogManagerCocoa;
@protocol ConstrainedWindowSheet;

// A delegate for a constrained window. The delegate is notified when the
// window closes.
class ConstrainedWindowMacDelegate {
 public:
  virtual void OnConstrainedWindowClosed(ConstrainedWindowMac* window) = 0;
};

// Creates a ConstrainedWindowMac, shows the dialog, and returns it.
std::unique_ptr<ConstrainedWindowMac> CreateAndShowWebModalDialogMac(
    ConstrainedWindowMacDelegate* delegate,
    content::WebContents* web_contents,
    id<ConstrainedWindowSheet> sheet);

// Creates a ConstrainedWindowMac and returns it.
std::unique_ptr<ConstrainedWindowMac> CreateWebModalDialogMac(
    ConstrainedWindowMacDelegate* delegate,
    content::WebContents* web_contents,
    id<ConstrainedWindowSheet> sheet);

// Constrained window implementation for Mac.
// Normally an instance of this class is owned by the delegate. The delegate
// should delete the instance when the window is closed.
class ConstrainedWindowMac {
 public:
  ConstrainedWindowMac(ConstrainedWindowMacDelegate* delegate,
                       content::WebContents* web_contents,
                       id<ConstrainedWindowSheet> sheet);
  ~ConstrainedWindowMac();

  // Shows the constrained window.
  void ShowWebContentsModalDialog();

  // Closes the constrained window.
  void CloseWebContentsModalDialog();

  SingleWebContentsDialogManagerCocoa* manager() const { return manager_; }
  void set_manager(SingleWebContentsDialogManagerCocoa* manager) {
    manager_ = manager;
  }
  id<ConstrainedWindowSheet> sheet() const { return sheet_.get(); }

  // Called by |manager_| when the dialog is closing.
  void OnDialogClosing();

  // Whether or not the dialog was shown. If the dialog is auto-resizable, it
  // is hidden until its WebContents initially loads.
  bool DialogWasShown();

 private:
  // Gets the dialog manager for |web_contents_|.
  web_modal::WebContentsModalDialogManager* GetDialogManager();

  ConstrainedWindowMacDelegate* delegate_;  // weak, owns us.
  SingleWebContentsDialogManagerCocoa* manager_;  // weak, owned by WCMDM.
  content::WebContents* web_contents_;  // weak, owned by dialog initiator.
  base::scoped_nsprotocol<id<ConstrainedWindowSheet>> sheet_;
  std::unique_ptr<SingleWebContentsDialogManagerCocoa> native_manager_;
};

#endif  // CHROME_BROWSER_UI_COCOA_CONSTRAINED_WINDOW_CONSTRAINED_WINDOW_MAC_H_
