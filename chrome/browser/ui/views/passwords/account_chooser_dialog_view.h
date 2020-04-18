// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_PASSWORDS_ACCOUNT_CHOOSER_DIALOG_VIEW_H_
#define CHROME_BROWSER_UI_VIEWS_PASSWORDS_ACCOUNT_CHOOSER_DIALOG_VIEW_H_

#include "base/macros.h"
#include "chrome/browser/ui/passwords/password_dialog_prompts.h"
#include "ui/views/bubble/bubble_dialog_delegate.h"
#include "ui/views/controls/button/button.h"
#include "ui/views/controls/styled_label_listener.h"

namespace content {
class WebContents;
}

class PasswordDialogController;

class AccountChooserDialogView : public views::BubbleDialogDelegateView,
                                 public views::StyledLabelListener,
                                 public views::ButtonListener,
                                 public AccountChooserPrompt {
 public:
  AccountChooserDialogView(PasswordDialogController* controller,
                           content::WebContents* web_contents);
  ~AccountChooserDialogView() override;

  // AccountChooserPrompt:
  void ShowAccountChooser() override;
  void ControllerGone() override;

 private:
  // WidgetDelegate:
  ui::ModalType GetModalType() const override;
  base::string16 GetWindowTitle() const override;
  bool ShouldShowCloseButton() const override;
  void AddedToWidget() override;
  void WindowClosing() override;

  // DialogDelegate:
  bool Accept() override;
  int GetDialogButtons() const override;
  base::string16 GetDialogButtonLabel(ui::DialogButton button) const override;

  // StyledLabelListener:
  void StyledLabelLinkClicked(views::StyledLabel* label,
                              const gfx::Range& range,
                              int event_flags) override;

  // ButtonListener:
  void ButtonPressed(views::Button* sender, const ui::Event& event) override;

  // Sets up the child views.
  void InitWindow();

  // A weak pointer to the controller.
  PasswordDialogController* controller_;
  content::WebContents* web_contents_;
  // The "Sign in" button is shown for one credential only. The variable is
  // cached because the framework can call GetDialogButtons() after the
  // controller is gone.
  bool show_signin_button_;

  DISALLOW_COPY_AND_ASSIGN(AccountChooserDialogView);
};

#endif  // CHROME_BROWSER_UI_VIEWS_PASSWORDS_ACCOUNT_CHOOSER_DIALOG_VIEW_H_
