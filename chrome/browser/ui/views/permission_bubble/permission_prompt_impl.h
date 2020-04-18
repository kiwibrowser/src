// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_PERMISSION_BUBBLE_PERMISSION_PROMPT_IMPL_H_
#define CHROME_BROWSER_UI_VIEWS_PERMISSION_BUBBLE_PERMISSION_PROMPT_IMPL_H_

#include "base/macros.h"
#include "chrome/browser/ui/permission_bubble/permission_prompt.h"

class Browser;
class PermissionsBubbleDialogDelegateView;

class PermissionPromptImpl : public PermissionPrompt {
 public:
  PermissionPromptImpl(Browser* browser, Delegate* delegate);
  ~PermissionPromptImpl() override;

  // PermissionPrompt:
  void UpdateAnchorPosition() override;
  gfx::NativeWindow GetNativeWindow() override;

  void Closing();
  void Accept();
  void Deny();

  Browser* browser() { return browser_; }

 private:
  void Show();

  Browser* const browser_;
  Delegate* const delegate_;
  PermissionsBubbleDialogDelegateView* bubble_delegate_;

  DISALLOW_COPY_AND_ASSIGN(PermissionPromptImpl);
};

#endif  // CHROME_BROWSER_UI_VIEWS_PERMISSION_BUBBLE_PERMISSION_PROMPT_IMPL_H_
