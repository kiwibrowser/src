// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_ONE_CLICK_SIGNIN_DIALOG_CONTROLLER_H_
#define CHROME_BROWSER_UI_COCOA_ONE_CLICK_SIGNIN_DIALOG_CONTROLLER_H_

#import <Cocoa/Cocoa.h>

#include <memory>

#include "base/mac/scoped_nsobject.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/cocoa/constrained_window/constrained_window_mac.h"

namespace content {
class WebContents;
}
@class OneClickSigninViewController;

// After the user signs into chrome this class is used to display a tab modal
// signin confirmation dialog.
class OneClickSigninDialogController : public ConstrainedWindowMacDelegate {
 public:
  // Creates an OneClickSigninDialogController. |web_contents| is used to
  // open links, |email| is the user's email address that is used for sync,
  // and |sync_callback| is called to start sync.
  OneClickSigninDialogController(
      content::WebContents* web_contents,
      const BrowserWindow::StartSyncCallback& sync_callback,
      const base::string16& email);
  virtual ~OneClickSigninDialogController();

  // ConstrainedWindowMacDelegate implementation.
  void OnConstrainedWindowClosed(ConstrainedWindowMac* window) override;

  ConstrainedWindowMac* constrained_window() const {
    return constrained_window_.get();
  }

  OneClickSigninViewController* view_controller() { return view_controller_; }

 private:
  void PerformClose();

  std::unique_ptr<ConstrainedWindowMac> constrained_window_;
  base::scoped_nsobject<OneClickSigninViewController> view_controller_;
};

#endif  // CHROME_BROWSER_UI_COCOA_ONE_CLICK_SIGNIN_DIALOG_CONTROLLER_H_
