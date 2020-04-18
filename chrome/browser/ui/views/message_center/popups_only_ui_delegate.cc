// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/message_center/popups_only_ui_delegate.h"

#include "ui/display/screen.h"
#include "ui/message_center/message_center.h"
#include "ui/message_center/ui_controller.h"
#include "ui/message_center/views/desktop_popup_alignment_delegate.h"
#include "ui/message_center/views/message_popup_collection.h"

message_center::UiDelegate* CreateUiDelegate() {
  return new PopupsOnlyUiDelegate();
}

PopupsOnlyUiDelegate::PopupsOnlyUiDelegate() {
  ui_controller_.reset(new message_center::UiController(this));
  alignment_delegate_.reset(new message_center::DesktopPopupAlignmentDelegate);
  popup_collection_.reset(new message_center::MessagePopupCollection(
      message_center(), ui_controller_.get(), alignment_delegate_.get()));
  message_center()->SetHasMessageCenterView(false);
}

PopupsOnlyUiDelegate::~PopupsOnlyUiDelegate() {
  // Reset this early so that delegated events during destruction don't cause
  // problems.
  popup_collection_.reset();
  ui_controller_.reset();
}

message_center::MessageCenter* PopupsOnlyUiDelegate::message_center() {
  return ui_controller_->message_center();
}

bool PopupsOnlyUiDelegate::ShowPopups() {
  alignment_delegate_->StartObserving(display::Screen::GetScreen());
  popup_collection_->DoUpdate();
  return true;
}

void PopupsOnlyUiDelegate::HidePopups() {
  DCHECK(popup_collection_.get());
  popup_collection_->MarkAllPopupsShown();
}

bool PopupsOnlyUiDelegate::ShowMessageCenter(bool show_by_click) {
  // Message center not available on Windows/Linux.
  return false;
}

void PopupsOnlyUiDelegate::HideMessageCenter() {}

bool PopupsOnlyUiDelegate::ShowNotifierSettings() {
  // Message center settings not available on Windows/Linux.
  return false;
}

void PopupsOnlyUiDelegate::OnMessageCenterContentsChanged() {}

message_center::UiController*
PopupsOnlyUiDelegate::GetUiControllerForTesting() {
  return ui_controller_.get();
}
