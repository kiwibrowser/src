// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/cocoa/constrained_window/constrained_window_mac.h"

#include <memory>
#include <utility>

#include "base/logging.h"
#import "chrome/browser/ui/cocoa/constrained_window/constrained_window_sheet.h"
#import "chrome/browser/ui/cocoa/single_web_contents_dialog_manager_cocoa.h"
#include "components/guest_view/browser/guest_view_base.h"
#include "content/public/browser/browser_thread.h"

using web_modal::WebContentsModalDialogManager;

std::unique_ptr<ConstrainedWindowMac> CreateAndShowWebModalDialogMac(
    ConstrainedWindowMacDelegate* delegate,
    content::WebContents* web_contents,
    id<ConstrainedWindowSheet> sheet) {
  ConstrainedWindowMac* window =
      new ConstrainedWindowMac(delegate, web_contents, sheet);
  window->ShowWebContentsModalDialog();
  return std::unique_ptr<ConstrainedWindowMac>(window);
}

std::unique_ptr<ConstrainedWindowMac> CreateWebModalDialogMac(
    ConstrainedWindowMacDelegate* delegate,
    content::WebContents* web_contents,
    id<ConstrainedWindowSheet> sheet) {
  return std::unique_ptr<ConstrainedWindowMac>(
      new ConstrainedWindowMac(delegate, web_contents, sheet));
}

ConstrainedWindowMac::ConstrainedWindowMac(
    ConstrainedWindowMacDelegate* delegate,
    content::WebContents* web_contents,
    id<ConstrainedWindowSheet> sheet)
    : delegate_(delegate),
      sheet_([sheet retain]) {
  DCHECK(sheet);

  // |web_contents| may be embedded within a chain of nested GuestViews. If it
  // is, follow the chain of embedders to the outermost WebContents and use it.
  web_contents_ =
      guest_view::GuestViewBase::GetTopLevelWebContents(web_contents);

  native_manager_.reset(
      new SingleWebContentsDialogManagerCocoa(this, sheet_.get(),
                                              GetDialogManager()));
}

ConstrainedWindowMac::~ConstrainedWindowMac() {
  CHECK(content::BrowserThread::CurrentlyOn(content::BrowserThread::UI));
  native_manager_.reset();
  DCHECK(!manager_);
}

void ConstrainedWindowMac::ShowWebContentsModalDialog() {
  std::unique_ptr<SingleWebContentsDialogManagerCocoa> dialog_manager;
  dialog_manager = std::move(native_manager_);
  GetDialogManager()->ShowDialogWithManager(
      [sheet_.get() sheetWindow], std::move(dialog_manager));
}

void ConstrainedWindowMac::CloseWebContentsModalDialog() {
  if (manager_)
    manager_->Close();
}

void ConstrainedWindowMac::OnDialogClosing() {
  if (delegate_)
    delegate_->OnConstrainedWindowClosed(this);
}

bool ConstrainedWindowMac::DialogWasShown() {
  // If the dialog was shown, |native_manager_| would have been released.
  return !native_manager_;
}

WebContentsModalDialogManager* ConstrainedWindowMac::GetDialogManager() {
  DCHECK(web_contents_);
  WebContentsModalDialogManager* dialog_manager =
      WebContentsModalDialogManager::FromWebContents(web_contents_);
  // If WebContentsModalDialogManager::CreateForWebContents(web_contents_) was
  // never called, then the manager will be null. E.g., for browser tabs,
  // TabHelpers::AttachTabHelpers() calls CreateForWebContents(). It is invalid
  // to show a dialog on some kinds of WebContents. Crash cleanly in that case.
  CHECK(dialog_manager);
  return dialog_manager;
}
