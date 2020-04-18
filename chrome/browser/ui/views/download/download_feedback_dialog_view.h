// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_DOWNLOAD_DOWNLOAD_FEEDBACK_DIALOG_VIEW_H_
#define CHROME_BROWSER_UI_VIEWS_DOWNLOAD_DOWNLOAD_FEEDBACK_DIALOG_VIEW_H_

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "ui/views/controls/link_listener.h"
#include "ui/views/window/dialog_delegate.h"

namespace views {
class MessageBoxView;
}

namespace content {
class PageNavigator;
}

class Profile;

// Asks the user whether they want to participate in the Safe Browsing
// download feedback program. Shown only for downloads marked DANGEROUS_HOST
// or UNCOMMON_DOWNLOAD. The user should only see this dialog once.
class DownloadFeedbackDialogView : public views::DialogDelegate,
                                   public views::LinkListener {
 public:
  // Callback with the user's decision. |accepted| is true if the user clicked
  // Accept(). Otherwise, assume the user cancelled.
  typedef base::Callback<void(bool accepted)> UserDecisionCallback;

  static void Show(
      gfx::NativeWindow parent_window,
      Profile* profile,
      content::PageNavigator* navigator,
      const UserDecisionCallback& callback);

 private:
  DownloadFeedbackDialogView(
      Profile* profile,
      content::PageNavigator* navigator,
      const UserDecisionCallback& callback);
  ~DownloadFeedbackDialogView() override;

  // Handles the user's decision.
  bool OnButtonClicked(bool accepted);

  // views::DialogDelegate:
  ui::ModalType GetModalType() const override;
  base::string16 GetWindowTitle() const override;
  void DeleteDelegate() override;
  views::Widget* GetWidget() override;
  const views::Widget* GetWidget() const override;
  views::View* GetContentsView() override;
  int GetDefaultDialogButton() const override;
  base::string16 GetDialogButtonLabel(ui::DialogButton button) const override;
  bool Cancel() override;
  bool Accept() override;
  views::View* CreateExtraView() override;

  // views::LinkListener:
  void LinkClicked(views::Link* source, int event_flags) override;

  Profile* profile_;
  content::PageNavigator* navigator_;
  const UserDecisionCallback callback_;
  views::MessageBoxView* explanation_box_view_;
  views::Link* link_view_;
  base::string16 title_text_;
  base::string16 ok_button_text_;
  base::string16 cancel_button_text_;

  DISALLOW_COPY_AND_ASSIGN(DownloadFeedbackDialogView);
};

#endif  // CHROME_BROWSER_UI_VIEWS_DOWNLOAD_DOWNLOAD_FEEDBACK_DIALOG_VIEW_H_
