// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_JAVASCRIPT_DIALOGS_JAVASCRIPT_DIALOG_COCOA_H_
#define CHROME_BROWSER_UI_JAVASCRIPT_DIALOGS_JAVASCRIPT_DIALOG_COCOA_H_

#include <memory>

#include "base/macros.h"
#include "chrome/browser/ui/javascript_dialogs/javascript_dialog.h"
#include "content/public/browser/javascript_dialog_manager.h"

// A Cocoa version of a JavaScript dialog that automatically dismisses itself
// when the user switches away to a different tab, used for WebContentses that
// are browser tabs.
class JavaScriptDialogCocoa : public JavaScriptDialog {
 public:
  class JavaScriptDialogCocoaImpl;

  ~JavaScriptDialogCocoa() override;

  static base::WeakPtr<JavaScriptDialogCocoa> Create(
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

 private:
  JavaScriptDialogCocoa(
      content::WebContents* parent_web_contents,
      content::WebContents* alerting_web_contents,
      const base::string16& title,
      content::JavaScriptDialogType dialog_type,
      const base::string16& message_text,
      const base::string16& default_prompt_text,
      content::JavaScriptDialogManager::DialogClosedCallback dialog_callback);

  std::unique_ptr<JavaScriptDialogCocoaImpl> impl_;

  base::WeakPtrFactory<JavaScriptDialogCocoa> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(JavaScriptDialogCocoa);
};

#endif  // CHROME_BROWSER_UI_JAVASCRIPT_DIALOGS_JAVASCRIPT_DIALOG_COCOA_H_
