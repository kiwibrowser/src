// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_WEB_CONTENTS_MODAL_DIALOG_MANAGER_VIEWS_MAC_H_
#define CHROME_BROWSER_UI_COCOA_WEB_CONTENTS_MODAL_DIALOG_MANAGER_VIEWS_MAC_H_

#import "base/mac/scoped_nsobject.h"
#include "base/macros.h"
#include "components/web_modal/single_web_contents_dialog_manager.h"
#include "ui/views/widget/widget_observer.h"

@class NSWindow;
@class WrappedConstrainedWindowSheet;

// WebContents dialog manager for a toolkit-views dialog parented off a Cocoa
// browser window. Most of the modality behavior is still performed by the Cocoa
// based ConstrainedWindowSheetController. This class bridges the expectations
// of a toolkit-views dialog to the Cocoa ConstrainedWindowSheetController.
// Note that this is not a web_modal::ModalDialogHostObserver. This is because
// tabs in a Cocoa browser can't be dragged off their window if they have a tab-
// modal dialog open, so the ModalDialogHost is only used by the views plumbing
// when creating the dialog.
class SingleWebContentsDialogManagerViewsMac
    : public web_modal::SingleWebContentsDialogManager,
      public views::WidgetObserver {
 public:
  SingleWebContentsDialogManagerViewsMac(
      NSWindow* dialog,
      web_modal::SingleWebContentsDialogManagerDelegate* delegate);

  ~SingleWebContentsDialogManagerViewsMac() override;

  // SingleWebContentsDialogManager:
  void Show() override;
  void Hide() override;
  void Close() override;
  void Focus() override;
  void Pulse() override;
  void HostChanged(web_modal::WebContentsModalDialogHost* new_host) override;
  gfx::NativeWindow dialog() override;

  // views::WidgetObserver:
  void OnWidgetClosing(views::Widget* widget) override;
  void OnWidgetDestroying(views::Widget* widget) override;

 private:
  base::scoped_nsobject<WrappedConstrainedWindowSheet> sheet_;
  // Weak. Owns this.
  web_modal::SingleWebContentsDialogManagerDelegate* delegate_;
  // Weak. Owned by parent window.
  web_modal::WebContentsModalDialogHost* host_;

  views::Widget* widget_;  // Weak. Deletes |this| when closing.

  bool was_shown_ = false;

  DISALLOW_COPY_AND_ASSIGN(SingleWebContentsDialogManagerViewsMac);
};

#endif  // CHROME_BROWSER_UI_COCOA_WEB_CONTENTS_MODAL_DIALOG_MANAGER_VIEWS_MAC_H_
