// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_WEB_CONTENTS_MODAL_DIALOG_HOST_COCOA_H_
#define CHROME_BROWSER_UI_COCOA_WEB_CONTENTS_MODAL_DIALOG_HOST_COCOA_H_

#include "base/macros.h"
#include "components/web_modal/web_contents_modal_dialog_host.h"

@class ConstrainedWindowSheetController;

// In a Cocoa browser, the modal dialog host is a simple bridge to the
// ConstrainedWindowSheetController to get the dialog size and parent window.
class WebContentsModalDialogHostCocoa
    : public web_modal::WebContentsModalDialogHost {
 public:
  explicit WebContentsModalDialogHostCocoa(
      ConstrainedWindowSheetController* sheet_controller);
  ~WebContentsModalDialogHostCocoa() override;

  // web_modal::ModalDialogHost:
  gfx::NativeView GetHostView() const override;
  gfx::Point GetDialogPosition(const gfx::Size& size) override;
  bool ShouldActivateDialog() const override;
  void AddObserver(web_modal::ModalDialogHostObserver* observer) override;
  void RemoveObserver(web_modal::ModalDialogHostObserver* observer) override;

  // web_modal::WebContentsModalDialogHost:
  gfx::Size GetMaximumDialogSize() override;

 private:
  ConstrainedWindowSheetController* sheet_controller_;  // Weak. Owns |this|.

  DISALLOW_COPY_AND_ASSIGN(WebContentsModalDialogHostCocoa);
};

#endif  // CHROME_BROWSER_UI_COCOA_WEB_CONTENTS_MODAL_DIALOG_HOST_COCOA_H_
