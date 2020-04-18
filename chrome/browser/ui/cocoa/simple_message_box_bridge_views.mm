// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/callback.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "chrome/browser/ui/cocoa/browser_dialogs_views_mac.h"
#include "chrome/browser/ui/cocoa/simple_message_box_cocoa.h"
#include "chrome/browser/ui/simple_message_box.h"
#include "chrome/browser/ui/views/simple_message_box_views.h"
#include "ui/base/resource/resource_bundle.h"

namespace {

chrome::MessageBoxResult ShowMessageBoxImpl(
    gfx::NativeWindow parent,
    const base::string16& title,
    const base::string16& message,
    chrome::MessageBoxType type,
    const base::string16& yes_text,
    const base::string16& no_text,
    const base::string16& checkbox_text) {
  // These functions may be called early in browser startup, in which case the
  // UI thread may not be ready to run or configured fully. In that case, fall
  // back to native Cocoa message boxes.
  if (base::MessageLoopForUI::IsCurrent() &&
      base::RunLoop::IsRunningOnCurrentThread() &&
      ui::ResourceBundle::HasSharedInstance() &&
      chrome::ShowAllDialogsWithViewsToolkit()) {
    return SimpleMessageBoxViews::Show(parent, title, message, type, yes_text,
                                       no_text, checkbox_text);
  }
  // ShowMessageBoxCocoa() and NSAlerts in general don't support most of the
  // above options at all.
  return chrome::ShowMessageBoxCocoa(message, type, checkbox_text);
}

}  // namespace

namespace chrome {

void ShowWarningMessageBox(gfx::NativeWindow parent,
                           const base::string16& title,
                           const base::string16& message) {
  ShowMessageBoxImpl(parent, title, message, MESSAGE_BOX_TYPE_WARNING,
                     base::string16(), base::string16(), base::string16());
}

void ShowWarningMessageBoxWithCheckbox(
    gfx::NativeWindow parent,
    const base::string16& title,
    const base::string16& message,
    const base::string16& checkbox_text,
    base::OnceCallback<void(bool checked)> callback) {
  MessageBoxResult result =
      ShowMessageBoxImpl(parent, title, message, MESSAGE_BOX_TYPE_WARNING,
                         base::string16(), base::string16(), checkbox_text);
  std::move(callback).Run(result == MESSAGE_BOX_RESULT_YES);
}

MessageBoxResult ShowQuestionMessageBox(gfx::NativeWindow parent,
                                        const base::string16& title,
                                        const base::string16& message) {
  return ShowMessageBoxImpl(parent, title, message, MESSAGE_BOX_TYPE_QUESTION,
                            base::string16(), base::string16(),
                            base::string16());
}

}  // namespace chrome
