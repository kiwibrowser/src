// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/sync/one_click_signin_dialog_view.h"

#include <utility>

#include "base/callback_helpers.h"
#include "base/logging.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_dialogs.h"
#include "chrome/browser/ui/views/harmony/chrome_layout_provider.h"
#include "chrome/common/url_constants.h"
#include "chrome/grit/chromium_strings.h"
#include "chrome/grit/generated_resources.h"
#include "components/constrained_window/constrained_window_views.h"
#include "components/google/core/browser/google_util.h"
#include "components/strings/grit/components_strings.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/events/keycodes/keyboard_codes.h"
#include "ui/views/controls/button/image_button.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/controls/image_view.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/link.h"
#include "ui/views/layout/grid_layout.h"
#include "ui/views/widget/widget.h"

namespace {

// Minimum width for the multi-line label.
const int kMinimumDialogLabelWidth = 400;

}  // namespace

// static
OneClickSigninDialogView* OneClickSigninDialogView::dialog_view_ = NULL;

// static
void OneClickSigninDialogView::ShowDialog(
    const base::string16& email,
    std::unique_ptr<OneClickSigninLinksDelegate> delegate,
    gfx::NativeWindow window,
    const BrowserWindow::StartSyncCallback& start_sync) {
  if (IsShowing())
    return;

  dialog_view_ =
      new OneClickSigninDialogView(email, std::move(delegate), start_sync);
  dialog_view_->Init();
  constrained_window::CreateBrowserModalDialogViews(dialog_view_, window)
      ->Show();
}

// static
bool OneClickSigninDialogView::IsShowing() {
  return dialog_view_ != nullptr;
}

// static
void OneClickSigninDialogView::Hide() {
  if (IsShowing())
    dialog_view_->GetWidget()->Close();
}

OneClickSigninDialogView::OneClickSigninDialogView(
    const base::string16& email,
    std::unique_ptr<OneClickSigninLinksDelegate> delegate,
    const BrowserWindow::StartSyncCallback& start_sync_callback)
    : delegate_(std::move(delegate)),
      email_(email),
      start_sync_callback_(start_sync_callback),
      advanced_link_(nullptr),
      learn_more_link_(nullptr) {
  DCHECK(!start_sync_callback_.is_null());
  set_margins(ChromeLayoutProvider::Get()->GetDialogInsetsForContentType(
      views::TEXT, views::TEXT));
  chrome::RecordDialogCreation(chrome::DialogIdentifier::ONE_CLICK_SIGNIN);
}

OneClickSigninDialogView::~OneClickSigninDialogView() {
  if (!start_sync_callback_.is_null()) {
    base::ResetAndReturn(&start_sync_callback_)
        .Run(OneClickSigninSyncStarter::UNDO_SYNC);
  }
}

void OneClickSigninDialogView::Init() {
  views::GridLayout* layout =
      SetLayoutManager(std::make_unique<views::GridLayout>(this));

  // Column set for descriptive text and link.
  views::ColumnSet* cs = layout->AddColumnSet(0);
  cs->AddColumn(views::GridLayout::FILL, views::GridLayout::CENTER, 1,
                views::GridLayout::USE_PREF, 0, 0);

  layout->StartRow(0, 0);

  views::Label* label = new views::Label(l10n_util::GetStringFUTF16(
      IDS_ONE_CLICK_SIGNIN_DIALOG_MESSAGE_NEW, email_));
  label->SetMultiLine(true);
  label->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  label->SizeToFit(kMinimumDialogLabelWidth);
  layout->AddView(label);

  layout->StartRow(0, 0);

  learn_more_link_ = new views::Link(l10n_util::GetStringUTF16(IDS_LEARN_MORE));
  learn_more_link_->set_listener(this);
  learn_more_link_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  layout->AddView(learn_more_link_, 1, 1, views::GridLayout::TRAILING,
                  views::GridLayout::CENTER);
}

base::string16 OneClickSigninDialogView::GetWindowTitle() const {
  return l10n_util::GetStringUTF16(IDS_ONE_CLICK_SIGNIN_DIALOG_TITLE_NEW);
}

ui::ModalType OneClickSigninDialogView::GetModalType() const {
  return ui::MODAL_TYPE_WINDOW;
}

void OneClickSigninDialogView::WindowClosing() {
  // We have to reset |dialog_view_| here, not in our destructor, because
  // we'll be destroyed asynchronously and the shown state will be checked
  // before then.
  DCHECK_EQ(dialog_view_, this);
  dialog_view_ = NULL;
}

views::View* OneClickSigninDialogView::CreateExtraView() {
  advanced_link_ = new views::Link(
      l10n_util::GetStringUTF16(IDS_ONE_CLICK_SIGNIN_DIALOG_ADVANCED));

  advanced_link_->set_listener(this);
  advanced_link_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  return advanced_link_;
}

base::string16 OneClickSigninDialogView::GetDialogButtonLabel(
    ui::DialogButton button) const {
  return l10n_util::GetStringUTF16(
      button == ui::DIALOG_BUTTON_OK ? IDS_ONE_CLICK_SIGNIN_DIALOG_OK_BUTTON
                                     : IDS_ONE_CLICK_SIGNIN_DIALOG_UNDO_BUTTON);
}

void OneClickSigninDialogView::LinkClicked(views::Link* source,
                                           int event_flags) {
  if (source == learn_more_link_) {
    delegate_->OnLearnMoreLinkClicked(true);
  } else if (source == advanced_link_) {
    base::ResetAndReturn(&start_sync_callback_)
        .Run(OneClickSigninSyncStarter::CONFIGURE_SYNC_FIRST);
    GetWidget()->Close();
  }
}

bool OneClickSigninDialogView::Accept() {
  base::ResetAndReturn(&start_sync_callback_)
      .Run(OneClickSigninSyncStarter::SYNC_WITH_DEFAULT_SETTINGS);
  return true;
}
