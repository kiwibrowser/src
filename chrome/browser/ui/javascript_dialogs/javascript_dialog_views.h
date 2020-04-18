// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_JAVASCRIPT_DIALOGS_JAVASCRIPT_DIALOG_VIEWS_H_
#define CHROME_BROWSER_UI_JAVASCRIPT_DIALOGS_JAVASCRIPT_DIALOG_VIEWS_H_

#include <memory>

#include "base/macros.h"
#include "chrome/browser/ui/javascript_dialogs/javascript_dialog.h"
#include "content/public/browser/javascript_dialog_manager.h"
#include "ui/views/window/dialog_delegate.h"

namespace views {
class MessageBoxView;
}

// A Views version of a JavaScript dialog that automatically dismisses itself
// when the user switches away to a different tab, used for WebContentses that
// are browser tabs.
class JavaScriptDialogViews : public JavaScriptDialog,
                              public views::DialogDelegate {
 public:
  ~JavaScriptDialogViews() override;

  static base::WeakPtr<JavaScriptDialogViews> Create(
      content::WebContents* parent_web_contents,
      content::WebContents* alerting_web_contents,
      const base::string16& title,
      content::JavaScriptDialogType dialog_type,
      const base::string16& message_text,
      const base::string16& default_prompt_text,
      content::JavaScriptDialogManager::DialogClosedCallback dialog_callback);

  // JavaScriptDialog:
  void CloseDialogWithoutCallback() override;
  base::string16 GetUserInput() override;

  // views::DialogDelegate:
  int GetDefaultDialogButton() const override;
  int GetDialogButtons() const override;
  base::string16 GetWindowTitle() const override;
  bool Cancel() override;
  bool Accept() override;
  bool Close() override;
  void DeleteDelegate() override;

  // views::WidgetDelegate:
  bool ShouldShowCloseButton() const override;
  views::View* GetContentsView() override;
  views::View* GetInitiallyFocusedView() override;
  views::Widget* GetWidget() override;
  const views::Widget* GetWidget() const override;
  ui::ModalType GetModalType() const override;

 private:
  JavaScriptDialogViews(
      content::WebContents* parent_web_contents,
      content::WebContents* alerting_web_contents,
      const base::string16& title,
      content::JavaScriptDialogType dialog_type,
      const base::string16& message_text,
      const base::string16& default_prompt_text,
      content::JavaScriptDialogManager::DialogClosedCallback dialog_callback);

  base::string16 title_;
  content::JavaScriptDialogType dialog_type_;
  base::string16 message_text_;
  base::string16 default_prompt_text_;
  content::JavaScriptDialogManager::DialogClosedCallback dialog_callback_;

  // The message box view whose commands we handle.
  views::MessageBoxView* message_box_view_;

  base::WeakPtrFactory<JavaScriptDialogViews> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(JavaScriptDialogViews);
};

#endif  // CHROME_BROWSER_UI_JAVASCRIPT_DIALOGS_JAVASCRIPT_DIALOG_VIEWS_H_
