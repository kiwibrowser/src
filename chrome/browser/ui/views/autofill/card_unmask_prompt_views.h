// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_AUTOFILL_CARD_UNMASK_PROMPT_VIEWS_H_
#define CHROME_BROWSER_UI_VIEWS_AUTOFILL_CARD_UNMASK_PROMPT_VIEWS_H_

#include <stdint.h>

#include "base/macros.h"
#include "chrome/browser/ui/autofill/autofill_dialog_models.h"
#include "components/autofill/core/browser/ui/card_unmask_prompt_view.h"
#include "ui/views/controls/combobox/combobox_listener.h"
#include "ui/views/controls/link_listener.h"
#include "ui/views/controls/textfield/textfield_controller.h"
#include "ui/views/window/dialog_delegate.h"

namespace content {
class WebContents;
}

namespace views {
class Checkbox;
class ImageView;
class Label;
class Link;
class Textfield;
class Throbber;
}

namespace autofill {

class CardUnmaskPromptController;

class CardUnmaskPromptViews : public CardUnmaskPromptView,
                              public views::ComboboxListener,
                              public views::DialogDelegateView,
                              public views::TextfieldController,
                              public views::LinkListener {
 public:
  CardUnmaskPromptViews(CardUnmaskPromptController* controller,
                        content::WebContents* web_contents);
  ~CardUnmaskPromptViews() override;

  // CardUnmaskPromptView
  void Show() override;
  void ControllerGone() override;
  void DisableAndWaitForVerification() override;
  void GotVerificationResult(const base::string16& error_message,
                             bool allow_retry) override;

  // views::DialogDelegateView
  View* GetContentsView() override;
  View* CreateFootnoteView() override;

  // views::View
  gfx::Size CalculatePreferredSize() const override;
  void OnNativeThemeChanged(const ui::NativeTheme* theme) override;
  ui::ModalType GetModalType() const override;
  base::string16 GetWindowTitle() const override;
  void DeleteDelegate() override;
  base::string16 GetDialogButtonLabel(ui::DialogButton button) const override;
  bool IsDialogButtonEnabled(ui::DialogButton button) const override;
  View* GetInitiallyFocusedView() override;
  bool ShouldShowCloseButton() const override;
  bool Cancel() override;
  bool Accept() override;

  // views::TextfieldController
  void ContentsChanged(views::Textfield* sender,
                       const base::string16& new_contents) override;

  // views::ComboboxListener
  void OnPerformAction(views::Combobox* combobox) override;

  // views::LinkListener
  void LinkClicked(views::Link* source, int event_flags) override;

 private:
  friend class CardUnmaskPromptViewTesterViews;

  void InitIfNecessary();
  void SetRetriableErrorMessage(const base::string16& message);
  bool ExpirationDateIsValid() const;
  void SetInputsEnabled(bool enabled);
  void ShowNewCardLink();
  void ClosePrompt();

  CardUnmaskPromptController* controller_;
  content::WebContents* web_contents_;

  // Expository language at the top of the dialog.
  views::Label* instructions_ = nullptr;

  // Holds the cvc and expiration inputs.
  View* input_row_ = nullptr;
  views::Textfield* cvc_input_ = nullptr;
  views::Combobox* month_input_ = nullptr;
  views::Combobox* year_input_ = nullptr;

  MonthComboboxModel month_combobox_model_;
  YearComboboxModel year_combobox_model_;

  views::Link* new_card_link_ = nullptr;

  // The error icon and label for most errors, which live beneath the inputs.
  views::ImageView* error_icon_ = nullptr;
  views::Label* error_label_ = nullptr;
  // The error label for permanent errors (where the user can't retry).
  views::Label* permanent_error_label_ = nullptr;

  views::View* controls_container_ = nullptr;
  views::View* storage_row_ = nullptr;
  views::Checkbox* storage_checkbox_ = nullptr;

  // Elements related to progress when the request is being made.
  views::View* progress_overlay_ = nullptr;
  views::Throbber* progress_throbber_ = nullptr;
  views::Label* progress_label_ = nullptr;

  base::WeakPtrFactory<CardUnmaskPromptViews> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(CardUnmaskPromptViews);
};

}  // namespace autofill

#endif  // CHROME_BROWSER_UI_VIEWS_AUTOFILL_CARD_UNMASK_PROMPT_VIEWS_H_
