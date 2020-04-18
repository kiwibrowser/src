// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/shell/browser/layout_test/layout_test_javascript_dialog_manager.h"

#include <utility>

#include "base/command_line.h"
#include "base/logging.h"
#include "base/strings/utf_string_conversions.h"
#include "content/public/browser/web_contents.h"
#include "content/shell/browser/layout_test/blink_test_controller.h"
#include "content/shell/browser/shell_javascript_dialog.h"
#include "content/shell/common/shell_switches.h"

namespace content {

LayoutTestJavaScriptDialogManager::LayoutTestJavaScriptDialogManager() {
}

LayoutTestJavaScriptDialogManager::~LayoutTestJavaScriptDialogManager() {
}

void LayoutTestJavaScriptDialogManager::RunJavaScriptDialog(
    WebContents* web_contents,
    RenderFrameHost* render_frame_host,
    JavaScriptDialogType dialog_type,
    const base::string16& message_text,
    const base::string16& default_prompt_text,
    DialogClosedCallback callback,
    bool* did_suppress_message) {
  std::move(callback).Run(true, base::string16());
}

void LayoutTestJavaScriptDialogManager::RunBeforeUnloadDialog(
    WebContents* web_contents,
    RenderFrameHost* render_frame_host,
    bool is_reload,
    DialogClosedCallback callback) {
  std::move(callback).Run(true, base::string16());
}

}  // namespace content
