// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/ui/request_pin_view.h"

#include <stddef.h>

#include "base/bind.h"
#include "base/callback_helpers.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/chromeos/options/passphrase_textfield.h"
#include "chrome/browser/ui/browser_dialogs.h"
#include "chrome/browser/ui/views/harmony/chrome_layout_provider.h"
#include "chrome/grit/generated_resources.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/events/event.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/textfield/textfield.h"
#include "ui/views/layout/grid_layout.h"
#include "ui/views/widget/widget.h"

namespace chromeos {

namespace {

// Default width of the dialog.
constexpr int kDefaultWidth = 448;

// Default width of the text field.
constexpr int kDefaultTextWidth = 200;

}  // namespace

RequestPinView::RequestPinView(const std::string& extension_name,
                               RequestPinView::RequestPinCodeType code_type,
                               int attempts_left,
                               const RequestPinCallback& callback,
                               Delegate* delegate)
    : callback_(callback), delegate_(delegate), weak_ptr_factory_(this) {
  DCHECK(code_type != RequestPinCodeType::UNCHANGED);
  DCHECK(delegate);
  Init();
  SetExtensionName(extension_name);
  const bool accept_input = (attempts_left != 0);
  SetDialogParameters(code_type, RequestPinErrorType::NONE, attempts_left,
                      accept_input);
  chrome::RecordDialogCreation(chrome::DialogIdentifier::REQUEST_PIN);
}

// When the parent window is closed while the dialog is active, this object is
// destroyed without triggering Accept or Cancel. If the callback_ wasn't called
// it needs to send the response.
RequestPinView::~RequestPinView() {
  if (!callback_.is_null()) {
    base::ResetAndReturn(&callback_).Run(base::string16());
  }

  delegate_->OnPinDialogClosed();
}

void RequestPinView::ContentsChanged(views::Textfield* sender,
                                     const base::string16& new_contents) {
  DialogModelChanged();
}

bool RequestPinView::Cancel() {
  // Destructor will be called after this which notifies the delegate.
  return true;
}

bool RequestPinView::Accept() {
  DCHECK(!callback_.is_null());

  if (!textfield_->enabled()) {
    return true;
  }
  DCHECK(!textfield_->text().empty());

  error_label_->SetVisible(true);
  error_label_->SetText(
      l10n_util::GetStringUTF16(IDS_REQUEST_PIN_DIALOG_PROCESSING));
  error_label_->SetTooltipText(error_label_->text());
  error_label_->SetEnabledColor(SK_ColorGRAY);
  error_label_->SizeToPreferredSize();
  // The |textfield_| and OK button become disabled, but the user still can
  // close the dialog.
  SetAcceptInput(false);
  base::ResetAndReturn(&callback_).Run(textfield_->text());
  DialogModelChanged();
  delegate_->OnPinDialogInput();

  return false;
}

base::string16 RequestPinView::GetWindowTitle() const {
  return window_title_;
}

views::View* RequestPinView::GetInitiallyFocusedView() {
  return textfield_;
}

bool RequestPinView::IsDialogButtonEnabled(ui::DialogButton button) const {
  switch (button) {
    case ui::DialogButton::DIALOG_BUTTON_CANCEL:
      return true;
    case ui::DialogButton::DIALOG_BUTTON_OK:
      if (callback_.is_null()) {
        return false;
      }
      // Not locked but the |textfield_| is not enabled. It's just a
      // notification to the user and [OK] button can be used to close the
      // dialog.
      if (!textfield_->enabled()) {
        return true;
      }
      return textfield_->text().size() > 0;
    case ui::DialogButton::DIALOG_BUTTON_NONE:
      return true;
  }

  NOTREACHED();
  return true;
}

gfx::Size RequestPinView::CalculatePreferredSize() const {
  return gfx::Size(
      kDefaultWidth,
      GetLayoutManager()->GetPreferredHeightForWidth(this, kDefaultWidth));
}

bool RequestPinView::IsLocked() {
  return callback_.is_null();
}

void RequestPinView::SetCallback(const RequestPinCallback& callback) {
  DCHECK(callback_.is_null());
  callback_ = callback;
}

void RequestPinView::SetDialogParameters(
    RequestPinView::RequestPinCodeType code_type,
    RequestPinView::RequestPinErrorType error_type,
    int attempts_left,
    bool accept_input) {
  SetErrorMessage(error_type, attempts_left);
  SetAcceptInput(accept_input);

  switch (code_type) {
    case RequestPinCodeType::PIN:
      code_type_ = l10n_util::GetStringUTF16(IDS_REQUEST_PIN_DIALOG_PIN);
      break;
    case RequestPinCodeType::PUK:
      code_type_ = l10n_util::GetStringUTF16(IDS_REQUEST_PIN_DIALOG_PUK);
      break;
    case RequestPinCodeType::UNCHANGED:
      break;
  }

  UpdateHeaderText();
}

void RequestPinView::SetExtensionName(const std::string& extension_name) {
  window_title_ = base::ASCIIToUTF16(extension_name);
  UpdateHeaderText();
}

void RequestPinView::UpdateHeaderText() {
  int label_text_id = IDS_REQUEST_PIN_DIALOG_HEADER;
  base::string16 label_text =
      l10n_util::GetStringFUTF16(label_text_id, window_title_, code_type_);
  header_label_->SetText(label_text);
  header_label_->SizeToPreferredSize();
}

void RequestPinView::Init() {
  set_margins(ChromeLayoutProvider::Get()->GetDialogInsetsForContentType(
      views::TEXT, views::TEXT));

  views::GridLayout* layout =
      SetLayoutManager(std::make_unique<views::GridLayout>(this));

  int column_view_set_id = 0;
  views::ColumnSet* column_set = layout->AddColumnSet(column_view_set_id);

  column_set->AddColumn(views::GridLayout::LEADING, views::GridLayout::FILL, 1,
                        views::GridLayout::USE_PREF, 0, 0);
  layout->StartRow(0, column_view_set_id);

  // Infomation label.
  int label_text_id = IDS_REQUEST_PIN_DIALOG_HEADER;
  base::string16 label_text = l10n_util::GetStringUTF16(label_text_id);
  header_label_ = new views::Label(label_text);
  header_label_->SetEnabled(true);
  layout->AddView(header_label_);

  const int related_vertical_spacing =
      ChromeLayoutProvider::Get()->GetDistanceMetric(
          views::DISTANCE_RELATED_CONTROL_VERTICAL);
  layout->AddPaddingRow(0, related_vertical_spacing);

  column_view_set_id++;
  column_set = layout->AddColumnSet(column_view_set_id);
  column_set->AddColumn(views::GridLayout::FILL, views::GridLayout::FILL, 100,
                        views::GridLayout::USE_PREF, 0, 0);

  // Textfield to enter the PIN/PUK.
  layout->StartRow(0, column_view_set_id);
  textfield_ = new PassphraseTextfield();
  textfield_->set_controller(this);
  textfield_->SetEnabled(true);
  textfield_->SetAssociatedLabel(header_label_);
  layout->AddView(textfield_, 1, 1, views::GridLayout::LEADING,
                  views::GridLayout::FILL, kDefaultTextWidth, 0);

  layout->AddPaddingRow(0, related_vertical_spacing);

  column_view_set_id++;
  column_set = layout->AddColumnSet(column_view_set_id);
  column_set->AddColumn(views::GridLayout::LEADING, views::GridLayout::FILL, 1,
                        views::GridLayout::USE_PREF, 0, 0);

  // Error label.
  layout->StartRow(0, column_view_set_id);
  error_label_ = new views::Label();
  error_label_->SetVisible(false);
  error_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  layout->AddView(error_label_);
}

void RequestPinView::SetAcceptInput(bool accept_input) {
  if (accept_input) {
    textfield_->SetEnabled(true);
    textfield_->SetBackgroundColor(SK_ColorWHITE);
    textfield_->RequestFocus();
  } else {
    textfield_->SetEnabled(false);
    textfield_->SetBackgroundColor(SK_ColorGRAY);
  }
}

void RequestPinView::SetErrorMessage(RequestPinErrorType error_type,
                                     int attempts_left) {
  base::string16 error_message;
  switch (error_type) {
    case RequestPinErrorType::INVALID_PIN:
      error_message =
          l10n_util::GetStringUTF16(IDS_REQUEST_PIN_DIALOG_INVALID_PIN_ERROR);
      break;
    case RequestPinErrorType::INVALID_PUK:
      error_message =
          l10n_util::GetStringUTF16(IDS_REQUEST_PIN_DIALOG_INVALID_PUK_ERROR);
      break;
    case RequestPinErrorType::MAX_ATTEMPTS_EXCEEDED:
      error_message = l10n_util::GetStringUTF16(
          IDS_REQUEST_PIN_DIALOG_MAX_ATTEMPTS_EXCEEDED_ERROR);
      break;
    case RequestPinErrorType::UNKNOWN_ERROR:
      error_message =
          l10n_util::GetStringUTF16(IDS_REQUEST_PIN_DIALOG_UNKNOWN_ERROR);
      break;
    case RequestPinErrorType::NONE:
      if (attempts_left < 0) {
        error_label_->SetVisible(false);
        textfield_->SetInvalid(false);
        return;
      }
      break;
  }

  if (attempts_left >= 0) {
    if (!error_message.empty())
      error_message.append(base::ASCIIToUTF16(" "));
    error_message.append(l10n_util::GetStringFUTF16(
        IDS_REQUEST_PIN_DIALOG_ATTEMPTS_LEFT,
        base::ASCIIToUTF16(std::to_string(attempts_left))));
  }

  error_label_->SetVisible(true);
  error_label_->SetText(error_message);
  error_label_->SetTooltipText(error_message);
  error_label_->SetEnabledColor(SK_ColorRED);
  error_label_->SizeToPreferredSize();
  textfield_->SetInvalid(true);
}

}  // namespace chromeos
