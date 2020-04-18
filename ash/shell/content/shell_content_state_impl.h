// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SHELL_CONTENT_SHELL_CONTENT_STATE_IMPL_H_
#define ASH_SHELL_CONTENT_SHELL_CONTENT_STATE_IMPL_H_

#include "ash/content/shell_content_state.h"
#include "base/macros.h"

namespace ash {

class ShellContentStateImpl : public ShellContentState {
 public:
  explicit ShellContentStateImpl(content::BrowserContext* context);

 private:
  ~ShellContentStateImpl() override;

  // Overridden from ShellContentState:
  content::BrowserContext* GetActiveBrowserContext() override;
  content::BrowserContext* GetBrowserContextByIndex(UserIndex index) override;
  content::BrowserContext* GetBrowserContextForWindow(
      aura::Window* window) override;
  content::BrowserContext* GetUserPresentingBrowserContextForWindow(
      aura::Window* window) override;

  content::BrowserContext* browser_context_;

  DISALLOW_COPY_AND_ASSIGN(ShellContentStateImpl);
};

}  // namespace ash

#endif  // ASH_SHELL_CONTENT_SHELL_CONTENT_STATE_IMPL_H_
