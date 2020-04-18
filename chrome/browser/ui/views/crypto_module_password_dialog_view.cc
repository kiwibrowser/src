// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/crypto_module_password_dialog_view.h"

#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/ui/browser_dialogs.h"
#include "chrome/browser/ui/views/harmony/chrome_layout_provider.h"
#include "chrome/grit/generated_resources.h"
#include "components/strings/grit/components_strings.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/events/event.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/textfield/textfield.h"
#include "ui/views/layout/grid_layout.h"
#include "ui/views/widget/widget.h"

////////////////////////////////////////////////////////////////////////////////
// CryptoModulePasswordDialogView, public:

CryptoModulePasswordDialogView::CryptoModulePasswordDialogView(
    const std::string& slot_name,
    CryptoModulePasswordReason reason,
    const std::string& hostname,
    const CryptoModulePasswordCallback& callback)
    : callback_(callback) {
  set_margins(ChromeLayoutProvider::Get()->GetDialogInsetsForContentType(
      views::TEXT, views::CONTROL));
  Init(hostname, slot_name, reason);
  chrome::RecordDialogCreation(chrome::DialogIdentifier::CRYPTO_PASSWORD);
}

CryptoModulePasswordDialogView::~CryptoModulePasswordDialogView() {
}

////////////////////////////////////////////////////////////////////////////////
// CryptoModulePasswordDialogView, private:

views::View* CryptoModulePasswordDialogView::GetInitiallyFocusedView() {
  return password_entry_;
}

ui::ModalType CryptoModulePasswordDialogView::GetModalType() const {
  return ui::MODAL_TYPE_WINDOW;
}

base::string16 CryptoModulePasswordDialogView::GetWindowTitle() const {
  return l10n_util::GetStringUTF16(IDS_CRYPTO_MODULE_AUTH_DIALOG_TITLE);
}

base::string16 CryptoModulePasswordDialogView::GetDialogButtonLabel(
    ui::DialogButton button) const {
  return l10n_util::GetStringUTF16(button == ui::DIALOG_BUTTON_OK ?
      IDS_CRYPTO_MODULE_AUTH_DIALOG_OK_BUTTON_LABEL : IDS_CANCEL);
}

bool CryptoModulePasswordDialogView::Cancel() {
  callback_.Run(std::string());
  const base::string16 empty;
  password_entry_->SetText(empty);
  return true;
}

bool CryptoModulePasswordDialogView::Accept() {
  callback_.Run(base::UTF16ToUTF8(password_entry_->text()));
  const base::string16 empty;
  password_entry_->SetText(empty);
  return true;
}

void CryptoModulePasswordDialogView::ContentsChanged(
    views::Textfield* sender,
    const base::string16& new_contents) {
}

bool CryptoModulePasswordDialogView::HandleKeyEvent(
    views::Textfield* sender,
    const ui::KeyEvent& keystroke) {
  return false;
}

void CryptoModulePasswordDialogView::Init(const std::string& hostname,
                                          const std::string& slot_name,
                                          CryptoModulePasswordReason reason) {
  // Select an appropriate text for the reason.
  std::string text;
  const base::string16& hostname16 = base::UTF8ToUTF16(hostname);
  const base::string16& slot16 = base::UTF8ToUTF16(slot_name);
  switch (reason) {
    case kCryptoModulePasswordCertEnrollment:
      text = l10n_util::GetStringFUTF8(
          IDS_CRYPTO_MODULE_AUTH_DIALOG_TEXT_CERT_ENROLLMENT,
          slot16,
          hostname16);
      break;
    case kCryptoModulePasswordClientAuth:
      text = l10n_util::GetStringFUTF8(
          IDS_CRYPTO_MODULE_AUTH_DIALOG_TEXT_CLIENT_AUTH, slot16, hostname16);
      break;
    case kCryptoModulePasswordListCerts:
      text = l10n_util::GetStringFUTF8(
          IDS_CRYPTO_MODULE_AUTH_DIALOG_TEXT_LIST_CERTS, slot16);
      break;
    case kCryptoModulePasswordCertImport:
      text = l10n_util::GetStringFUTF8(
          IDS_CRYPTO_MODULE_AUTH_DIALOG_TEXT_CERT_IMPORT, slot16);
      break;
    case kCryptoModulePasswordCertExport:
      text = l10n_util::GetStringFUTF8(
          IDS_CRYPTO_MODULE_AUTH_DIALOG_TEXT_CERT_EXPORT, slot16);
      break;
    default:
      NOTREACHED();
  }
  reason_label_ = new views::Label(base::UTF8ToUTF16(text));
  reason_label_->SetMultiLine(true);

  password_label_ = new views::Label(l10n_util::GetStringUTF16(
      IDS_CRYPTO_MODULE_AUTH_DIALOG_PASSWORD_FIELD));

  password_entry_ = new views::Textfield();
  password_entry_->SetTextInputType(ui::TEXT_INPUT_TYPE_PASSWORD);
  password_entry_->set_controller(this);

  ChromeLayoutProvider* provider = ChromeLayoutProvider::Get();

  views::GridLayout* layout =
      SetLayoutManager(std::make_unique<views::GridLayout>(this));

  views::ColumnSet* reason_column_set = layout->AddColumnSet(0);
  reason_column_set->AddColumn(
      views::GridLayout::LEADING, views::GridLayout::LEADING, 1,
      views::GridLayout::USE_PREF, 0, 0);

  views::ColumnSet* column_set = layout->AddColumnSet(1);
  column_set->AddColumn(views::GridLayout::LEADING,
                        views::GridLayout::LEADING, 0,
                        views::GridLayout::USE_PREF, 0, 0);
  column_set->AddPaddingColumn(
      0,
      provider->GetDistanceMetric(DISTANCE_UNRELATED_CONTROL_HORIZONTAL_LARGE));
  column_set->AddColumn(views::GridLayout::FILL, views::GridLayout::FILL, 1,
                        views::GridLayout::USE_PREF, 0, 0);

  layout->StartRow(0, 0);
  layout->AddView(reason_label_);
  layout->AddPaddingRow(0, provider->GetDistanceMetric(
                               views::DISTANCE_UNRELATED_CONTROL_VERTICAL));

  layout->StartRow(0, 1);
  layout->AddView(password_label_);
  layout->AddView(password_entry_);
}

void ShowCryptoModulePasswordDialog(
    const std::string& slot_name,
    bool retry,
    CryptoModulePasswordReason reason,
    const std::string& hostname,
    gfx::NativeWindow parent,
    const CryptoModulePasswordCallback& callback) {
  CryptoModulePasswordDialogView* dialog =
      new CryptoModulePasswordDialogView(slot_name, reason, hostname, callback);
  views::DialogDelegate::CreateDialogWidget(dialog, NULL, parent)->Show();
}
