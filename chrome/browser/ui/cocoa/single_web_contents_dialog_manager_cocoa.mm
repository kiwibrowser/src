// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/single_web_contents_dialog_manager_cocoa.h"

#import "chrome/browser/ui/cocoa/constrained_window/constrained_window_custom_sheet.h"
#import "chrome/browser/ui/cocoa/constrained_window/constrained_window_mac.h"
#import "chrome/browser/ui/cocoa/constrained_window/constrained_window_sheet_controller.h"
#include "components/web_modal/web_contents_modal_dialog_host.h"
#include "components/web_modal/web_contents_modal_dialog_manager.h"
#include "content/public/browser/web_contents.h"

SingleWebContentsDialogManagerCocoa::SingleWebContentsDialogManagerCocoa(
    ConstrainedWindowMac* client,
    id<ConstrainedWindowSheet> sheet,
    web_modal::SingleWebContentsDialogManagerDelegate* delegate)
    : client_(client),
      sheet_([sheet retain]),
      delegate_(delegate),
      host_(nullptr) {
  DCHECK(client);
  client->set_manager(this);
}

SingleWebContentsDialogManagerCocoa::~SingleWebContentsDialogManagerCocoa() {
}

void SingleWebContentsDialogManagerCocoa::Show() {
  // If a dialog is initially shown on a hidden/background WebContents, the
  // |delegate_| will defer the Show() until the WebContents is shown. If the
  // defer happens during tab closure or tab dragging, a suspected data race or
  // ObserverList ordering may result in |host_| being null here. If the tab is
  // closing anyway, it doesn't matter. For tab dragging, avoid a crash, but the
  // user may have to switch tabs again to see the dialog. See
  // http://crbug.com/514826 for details.
  if (!host_)
    return;

  NSView* parent_view = host_->GetHostView();
  // Note that simply [parent_view window] for an inactive tab is nil. However,
  // the following should always be non-nil for all WebContents containers.
  NSWindow* parent_window =
      delegate_->GetWebContents()->GetTopLevelNativeWindow();

  [[ConstrainedWindowSheetController controllerForParentWindow:parent_window]
      showSheet:sheet_ forParentView:parent_view];
}

void SingleWebContentsDialogManagerCocoa::Hide() {
  NSWindow* parent_window =
      delegate_->GetWebContents()->GetTopLevelNativeWindow();
  [[ConstrainedWindowSheetController controllerForParentWindow:parent_window]
      hideSheet:sheet_];
}

void SingleWebContentsDialogManagerCocoa::Close() {
  [[ConstrainedWindowSheetController controllerForSheet:sheet_]
      closeSheet:sheet_];
  client_->set_manager(nullptr);
  bool dialog_was_open = client_->DialogWasShown();
  client_->OnDialogClosing();      // |client_| might delete itself here.
  if (dialog_was_open)
    delegate_->WillClose(dialog());  // Deletes |this|.
}

void SingleWebContentsDialogManagerCocoa::Focus() {
}

void SingleWebContentsDialogManagerCocoa::Pulse() {
  [[ConstrainedWindowSheetController controllerForSheet:sheet_]
      pulseSheet:sheet_];
}

void SingleWebContentsDialogManagerCocoa::HostChanged(
    web_modal::WebContentsModalDialogHost* new_host) {
  // No need to observe the host. For Cocoa, the constrained window controller
  // will reposition the dialog when necessary. The host can also never change.
  // Tabs showing a dialog can not be dragged off a Cocoa browser window.
  // However, closing a tab with a dialog open will set the host back to null.
  DCHECK_NE(!!host_, !!new_host);
  host_ = new_host;
}

gfx::NativeWindow SingleWebContentsDialogManagerCocoa::dialog() {
  return [sheet_ sheetWindow];
}
