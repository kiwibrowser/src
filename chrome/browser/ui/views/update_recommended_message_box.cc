// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/update_recommended_message_box.h"

#include "build/build_config.h"
#include "chrome/browser/lifetime/application_lifetime.h"
#include "chrome/browser/ui/browser_dialogs.h"
#include "chrome/browser/ui/views/harmony/chrome_layout_provider.h"
#include "chrome/grit/chromium_strings.h"
#include "components/constrained_window/constrained_window_views.h"
#include "components/strings/grit/components_strings.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/views/controls/message_box_view.h"
#include "ui/views/widget/widget.h"

#if defined(OS_CHROMEOS)
#include "chromeos/dbus/dbus_thread_manager.h"
#include "chromeos/dbus/power_manager_client.h"
#endif

////////////////////////////////////////////////////////////////////////////////
// UpdateRecommendedMessageBox, public:

// static
void UpdateRecommendedMessageBox::Show(gfx::NativeWindow parent_window) {
  // When the window closes, it will delete itself.
  constrained_window::CreateBrowserModalDialogViews(
      new UpdateRecommendedMessageBox(), parent_window)->Show();
}

////////////////////////////////////////////////////////////////////////////////
// UpdateRecommendedMessageBox, private:

UpdateRecommendedMessageBox::UpdateRecommendedMessageBox() {
  views::MessageBoxView::InitParams params(
      l10n_util::GetStringUTF16(IDS_UPDATE_RECOMMENDED));
  params.message_width = ChromeLayoutProvider::Get()->GetDistanceMetric(
      ChromeDistanceMetric::DISTANCE_MODAL_DIALOG_PREFERRED_WIDTH);
  // Also deleted when the window closes.
  message_box_view_ = new views::MessageBoxView(params);
  chrome::RecordDialogCreation(chrome::DialogIdentifier::UPDATE_RECOMMENDED);
}

UpdateRecommendedMessageBox::~UpdateRecommendedMessageBox() {
}

bool UpdateRecommendedMessageBox::Accept() {
  chrome::AttemptRelaunch();
  return true;
}

base::string16 UpdateRecommendedMessageBox::GetDialogButtonLabel(
    ui::DialogButton button) const {
  return l10n_util::GetStringUTF16((button == ui::DIALOG_BUTTON_OK) ?
      IDS_RELAUNCH_AND_UPDATE : IDS_NOT_NOW);
}

bool UpdateRecommendedMessageBox::ShouldShowWindowTitle() const {
#if defined(OS_CHROMEOS)
  return false;
#else
  return true;
#endif
}

bool UpdateRecommendedMessageBox::ShouldShowCloseButton() const {
  return false;
}

base::string16 UpdateRecommendedMessageBox::GetWindowTitle() const {
#if defined(OS_CHROMEOS)
  return base::string16();
#else
  return l10n_util::GetStringUTF16(IDS_UPDATE_RECOMMENDED_DIALOG_TITLE);
#endif
}

void UpdateRecommendedMessageBox::DeleteDelegate() {
  delete this;
}

ui::ModalType UpdateRecommendedMessageBox::GetModalType() const {
  return ui::MODAL_TYPE_WINDOW;
}

views::View* UpdateRecommendedMessageBox::GetContentsView() {
  return message_box_view_;
}

views::Widget* UpdateRecommendedMessageBox::GetWidget() {
  return message_box_view_->GetWidget();
}

const views::Widget* UpdateRecommendedMessageBox::GetWidget() const {
  return message_box_view_->GetWidget();
}
