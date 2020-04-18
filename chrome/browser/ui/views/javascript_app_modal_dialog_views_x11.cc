// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/javascript_app_modal_dialog_views_x11.h"

#include "chrome/browser/ui/blocked_content/popunder_preventer.h"
#include "chrome/browser/ui/browser_dialogs.h"
#include "chrome/browser/ui/views/javascript_app_modal_event_blocker_x11.h"
#include "components/app_modal/javascript_app_modal_dialog.h"
#include "ui/views/widget/widget.h"

JavaScriptAppModalDialogViewsX11::JavaScriptAppModalDialogViewsX11(
    app_modal::JavaScriptAppModalDialog* parent)
    : app_modal::JavaScriptAppModalDialogViews(parent),
      popunder_preventer_(new PopunderPreventer(parent->web_contents())) {
  chrome::RecordDialogCreation(
      chrome::DialogIdentifier::JAVA_SCRIPT_APP_MODAL_X11);
}

JavaScriptAppModalDialogViewsX11::~JavaScriptAppModalDialogViewsX11() {
}

void JavaScriptAppModalDialogViewsX11::ShowAppModalDialog() {
  // BrowserView::CanActivate() ensures that other browser windows cannot be
  // activated for long while the dialog is visible. Block events to other
  // browser windows so that the user cannot interact with other browser windows
  // in the short time that the other browser windows are active. This hack is
  // unnecessary on Windows and Chrome OS.
  // TODO(pkotwicz): Find a better way of doing this and remove this hack.
  if (!event_blocker_x11_.get()) {
    event_blocker_x11_.reset(
        new JavascriptAppModalEventBlockerX11(GetWidget()->GetNativeView()));
  }
  GetWidget()->Show();
}

void JavaScriptAppModalDialogViewsX11::WindowClosing() {
  event_blocker_x11_.reset();
}
