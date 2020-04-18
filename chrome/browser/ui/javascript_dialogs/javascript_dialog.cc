// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/javascript_dialogs/javascript_dialog.h"

#include <utility>

#include "chrome/browser/ui/blocked_content/popunder_preventer.h"
#include "chrome/browser/ui/javascript_dialogs/javascript_dialog_views.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_delegate.h"

JavaScriptDialog::JavaScriptDialog(content::WebContents* parent_web_contents) {
  popunder_preventer_.reset(new PopunderPreventer(parent_web_contents));
  parent_web_contents->GetDelegate()->ActivateContents(parent_web_contents);
}

JavaScriptDialog::~JavaScriptDialog() = default;

base::WeakPtr<JavaScriptDialog> JavaScriptDialog::Create(
    content::WebContents* parent_web_contents,
    content::WebContents* alerting_web_contents,
    const base::string16& title,
    content::JavaScriptDialogType dialog_type,
    const base::string16& message_text,
    const base::string16& default_prompt_text,
    content::JavaScriptDialogManager::DialogClosedCallback dialog_callback) {
  return JavaScriptDialogViews::Create(
      parent_web_contents, alerting_web_contents, title, dialog_type,
      message_text, default_prompt_text, std::move(dialog_callback));
}
