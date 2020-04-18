// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/confirm_bubble_views.h"

#include <utility>

#include "chrome/browser/ui/browser_dialogs.h"
#include "chrome/browser/ui/confirm_bubble.h"
#include "chrome/browser/ui/confirm_bubble_model.h"
#include "chrome/browser/ui/views/harmony/chrome_layout_provider.h"
#include "components/constrained_window/constrained_window_views.h"
#include "components/strings/grit/components_strings.h"
#include "components/vector_icons/vector_icons.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/ui_features.h"
#include "ui/views/controls/button/image_button.h"
#include "ui/views/controls/button/image_button_factory.h"
#include "ui/views/controls/label.h"
#include "ui/views/layout/grid_layout.h"
#include "ui/views/widget/widget.h"

ConfirmBubbleViews::ConfirmBubbleViews(
    std::unique_ptr<ConfirmBubbleModel> model)
    : model_(std::move(model)), help_button_(nullptr) {
  set_margins(ChromeLayoutProvider::Get()->GetDialogInsetsForContentType(
      views::TEXT, views::TEXT));
  views::GridLayout* layout =
      SetLayoutManager(std::make_unique<views::GridLayout>(this));

  // Use a fixed maximum message width, so longer messages will wrap.
  const int kMaxMessageWidth = 400;
  views::ColumnSet* cs = layout->AddColumnSet(0);
  cs->AddColumn(views::GridLayout::LEADING, views::GridLayout::CENTER, 0,
                views::GridLayout::FIXED, kMaxMessageWidth, false);

  // Add the message label.
  views::Label* label = new views::Label(model_->GetMessageText());
  DCHECK(!label->text().empty());
  label->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  label->SetMultiLine(true);
  label->SizeToFit(kMaxMessageWidth);
  layout->StartRow(0, 0);
  layout->AddView(label);

  // Initialize the help button.
  help_button_ = CreateVectorImageButton(this);
  help_button_->SetFocusForPlatform();
  help_button_->SetTooltipText(l10n_util::GetStringUTF16(IDS_LEARN_MORE));
  SetImageFromVectorIcon(help_button_, vector_icons::kHelpOutlineIcon);

  chrome::RecordDialogCreation(chrome::DialogIdentifier::CONFIRM_BUBBLE);
}

ConfirmBubbleViews::~ConfirmBubbleViews() {
}

base::string16 ConfirmBubbleViews::GetDialogButtonLabel(
    ui::DialogButton button) const {
  switch (button) {
    case ui::DIALOG_BUTTON_OK:
      return model_->GetButtonLabel(ConfirmBubbleModel::BUTTON_OK);
    case ui::DIALOG_BUTTON_CANCEL:
      return model_->GetButtonLabel(ConfirmBubbleModel::BUTTON_CANCEL);
    default:
      NOTREACHED();
      return DialogDelegateView::GetDialogButtonLabel(button);
  }
}

bool ConfirmBubbleViews::IsDialogButtonEnabled(ui::DialogButton button) const {
  switch (button) {
    case ui::DIALOG_BUTTON_OK:
      return !!(model_->GetButtons() & ConfirmBubbleModel::BUTTON_OK);
    case ui::DIALOG_BUTTON_CANCEL:
      return !!(model_->GetButtons() & ConfirmBubbleModel::BUTTON_CANCEL);
    default:
      NOTREACHED();
      return false;
  }
}

views::View* ConfirmBubbleViews::CreateExtraView() {
  return help_button_;
}

bool ConfirmBubbleViews::Cancel() {
  model_->Cancel();
  return true;
}

bool ConfirmBubbleViews::Accept() {
  model_->Accept();
  return true;
}

ui::ModalType ConfirmBubbleViews::GetModalType() const {
  return ui::MODAL_TYPE_WINDOW;
}

base::string16 ConfirmBubbleViews::GetWindowTitle() const {
  return model_->GetTitle();
}

bool ConfirmBubbleViews::ShouldShowCloseButton() const {
  return false;
}

void ConfirmBubbleViews::ButtonPressed(views::Button* sender,
                                       const ui::Event& event) {
  if (sender == help_button_) {
    model_->OpenHelpPage();
    GetWidget()->Close();
  }
}

namespace chrome {

void ShowConfirmBubble(gfx::NativeWindow window,
                       gfx::NativeView anchor_view,
                       const gfx::Point& origin,
                       std::unique_ptr<ConfirmBubbleModel> model) {
  constrained_window::CreateBrowserModalDialogViews(
      new ConfirmBubbleViews(std::move(model)), window)
      ->Show();
}

}  // namespace chrome
