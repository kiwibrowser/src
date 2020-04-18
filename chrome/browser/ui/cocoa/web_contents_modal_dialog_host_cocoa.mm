// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/cocoa/web_contents_modal_dialog_host_cocoa.h"

#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/tab_dialogs.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#import "chrome/browser/ui/cocoa/constrained_window/constrained_window_sheet_controller.h"
#include "ui/gfx/geometry/point.h"
#include "ui/gfx/geometry/size.h"

WebContentsModalDialogHostCocoa::WebContentsModalDialogHostCocoa(
    ConstrainedWindowSheetController* sheet_controller)
    : sheet_controller_(sheet_controller) {
}

WebContentsModalDialogHostCocoa::~WebContentsModalDialogHostCocoa() {
  // Toolkit-Views calls OnHostDestroying on observers here, but the Cocoa host
  // doesn't need to be observed.
}

gfx::NativeView WebContentsModalDialogHostCocoa::GetHostView() const {
  // To avoid the constrained window controller having to know about the browser
  // view layout, use the active tab in the parent window.
  NSWindow* parent_window = [sheet_controller_ parentWindow];
  Browser* browser = chrome::FindBrowserWithWindow(parent_window);
  // This could be null for packaged app windows, but this dialog host is
  // currently only used for browsers.
  DCHECK(browser);
  content::WebContents* web_contents =
      browser->tab_strip_model()->GetActiveWebContents();
  DCHECK(web_contents);
  TabDialogs* tab_dialogs = TabDialogs::FromWebContents(web_contents);
  DCHECK(tab_dialogs);

  // Note this returns the WebContents' superview, so it doesn't really matter
  // which WebContents inside the browser we actually chose above.
  return tab_dialogs->GetDialogParentView();
}

gfx::Point WebContentsModalDialogHostCocoa::GetDialogPosition(
    const gfx::Size& size) {
  // Dialogs are always re-positioned by the constrained window sheet controller
  // so nothing interesting to return yet.
  return gfx::Point();
}

bool WebContentsModalDialogHostCocoa::ShouldActivateDialog() const {
  return [[sheet_controller_ parentWindow] isMainWindow];
}

void WebContentsModalDialogHostCocoa::AddObserver(
    web_modal::ModalDialogHostObserver* observer) {
  NOTREACHED();
}
void WebContentsModalDialogHostCocoa::RemoveObserver(
    web_modal::ModalDialogHostObserver* observer) {
  NOTREACHED();
}

gfx::Size WebContentsModalDialogHostCocoa::GetMaximumDialogSize() {
  // The dialog should try to fit within the overlay for the web contents.
  // Note that, for things like print preview, this is just a suggested maximum.
  return gfx::Size(
      [sheet_controller_ overlayWindowSizeForParentView:GetHostView()]);
}
