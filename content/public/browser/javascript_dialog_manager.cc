// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/browser/javascript_dialog_manager.h"

namespace content {

bool JavaScriptDialogManager::HandleJavaScriptDialog(
    WebContents* web_contents,
    bool accept,
    const base::string16* prompt_override) {
  return false;
}

}  // namespace content
