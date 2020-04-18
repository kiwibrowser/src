// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/web_dialogs/web_dialog_delegate.h"

#include "ui/base/accelerators/accelerator.h"

namespace ui {

std::string WebDialogDelegate::GetDialogName() const {
  return std::string();
}

void WebDialogDelegate::GetMinimumDialogSize(gfx::Size* size) const {
  GetDialogSize(size);
}

bool WebDialogDelegate::CanCloseDialog() const {
  return true;
}

bool WebDialogDelegate::CanResizeDialog() const {
  return true;
}

void WebDialogDelegate::OnDialogCloseFromWebUI(
    const std::string& json_retval) {
  OnDialogClosed(json_retval);
}

bool WebDialogDelegate::HandleContextMenu(
    const content::ContextMenuParams& params) {
  return false;
}

bool WebDialogDelegate::HandleOpenURLFromTab(
    content::WebContents* source,
    const content::OpenURLParams& params,
    content::WebContents** out_new_contents) {
  return false;
}

bool WebDialogDelegate::HandleShouldCreateWebContents() {
  return true;
}

std::vector<Accelerator> WebDialogDelegate::GetAccelerators() {
  return std::vector<Accelerator>();
}

bool WebDialogDelegate::AcceleratorPressed(const Accelerator& accelerator) {
  return false;
}

}  // namespace ui
