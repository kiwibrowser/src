// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/javascript_dialogs/javascript_dialog_views.h"

#include "chrome/browser/ui/browser_dialogs.h"
#include "components/constrained_window/constrained_window_views.h"
#include "ui/views/controls/message_box_view.h"
#include "ui/views/controls/textfield/textfield.h"

JavaScriptDialogViews::~JavaScriptDialogViews() = default;

// static
base::WeakPtr<JavaScriptDialogViews> JavaScriptDialogViews::Create(
    content::WebContents* parent_web_contents,
    content::WebContents* alerting_web_contents,
    const base::string16& title,
    content::JavaScriptDialogType dialog_type,
    const base::string16& message_text,
    const base::string16& default_prompt_text,
    content::JavaScriptDialogManager::DialogClosedCallback dialog_callback) {
  return (new JavaScriptDialogViews(
              parent_web_contents, alerting_web_contents, title, dialog_type,
              message_text, default_prompt_text, std::move(dialog_callback)))
      ->weak_factory_.GetWeakPtr();
}

void JavaScriptDialogViews::CloseDialogWithoutCallback() {
  dialog_callback_.Reset();
  GetWidget()->Close();
}

base::string16 JavaScriptDialogViews::GetUserInput() {
  return message_box_view_->GetInputText();
}

int JavaScriptDialogViews::GetDefaultDialogButton() const {
  return ui::DIALOG_BUTTON_OK;
}

int JavaScriptDialogViews::GetDialogButtons() const {
  const bool is_alert = dialog_type_ == content::JAVASCRIPT_DIALOG_TYPE_ALERT;
  return ui::DIALOG_BUTTON_OK | (is_alert ? 0 : ui::DIALOG_BUTTON_CANCEL);
}

base::string16 JavaScriptDialogViews::GetWindowTitle() const {
  return title_;
}

bool JavaScriptDialogViews::Cancel() {
  if (dialog_callback_)
    std::move(dialog_callback_).Run(false, base::string16());
  return true;
}

bool JavaScriptDialogViews::Accept() {
  if (dialog_callback_)
    std::move(dialog_callback_).Run(true, message_box_view_->GetInputText());
  return true;
}

bool JavaScriptDialogViews::Close() {
  if (dialog_callback_)
    std::move(dialog_callback_).Run(false, base::string16());
  return true;
}

void JavaScriptDialogViews::DeleteDelegate() {
  delete this;
}

bool JavaScriptDialogViews::ShouldShowCloseButton() const {
  return false;
}

views::View* JavaScriptDialogViews::GetContentsView() {
  return message_box_view_;
}

views::View* JavaScriptDialogViews::GetInitiallyFocusedView() {
  auto* text_box = message_box_view_->text_box();
  return text_box ? text_box : views::DialogDelegate::GetInitiallyFocusedView();
}

views::Widget* JavaScriptDialogViews::GetWidget() {
  return message_box_view_->GetWidget();
}

const views::Widget* JavaScriptDialogViews::GetWidget() const {
  return message_box_view_->GetWidget();
}

ui::ModalType JavaScriptDialogViews::GetModalType() const {
  return ui::MODAL_TYPE_CHILD;
}

JavaScriptDialogViews::JavaScriptDialogViews(
    content::WebContents* parent_web_contents,
    content::WebContents* alerting_web_contents,
    const base::string16& title,
    content::JavaScriptDialogType dialog_type,
    const base::string16& message_text,
    const base::string16& default_prompt_text,
    content::JavaScriptDialogManager::DialogClosedCallback dialog_callback)
    : JavaScriptDialog(parent_web_contents),
      title_(title),
      dialog_type_(dialog_type),
      message_text_(message_text),
      default_prompt_text_(default_prompt_text),
      dialog_callback_(std::move(dialog_callback)),
      weak_factory_(this) {
  int options = views::MessageBoxView::DETECT_DIRECTIONALITY;
  if (dialog_type == content::JAVASCRIPT_DIALOG_TYPE_PROMPT)
    options |= views::MessageBoxView::HAS_PROMPT_FIELD;

  views::MessageBoxView::InitParams params(message_text);
  params.options = options;
  params.default_prompt = default_prompt_text;
  message_box_view_ = new views::MessageBoxView(params);
  DCHECK(message_box_view_);

  constrained_window::ShowWebModalDialogViews(this, parent_web_contents);
  chrome::RecordDialogCreation(chrome::DialogIdentifier::JAVA_SCRIPT);
}
