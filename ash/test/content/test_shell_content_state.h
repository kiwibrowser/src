// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_TEST_CONTENT_TEST_SHELL_CONTENT_STATE_H_
#define ASH_TEST_CONTENT_TEST_SHELL_CONTENT_STATE_H_

#include <memory>

#include "ash/content/shell_content_state.h"
#include "base/macros.h"

namespace content {
class BrowserContext;
}

namespace ash {

class TestShellContentState : public ShellContentState {
 public:
  TestShellContentState();

  content::ScreenOrientationDelegate* screen_orientation_delegate() {
    return &orientation_delegate_;
  }

 private:
  ~TestShellContentState() override;

  // Overridden from ShellContentState:
  content::BrowserContext* GetActiveBrowserContext() override;
  content::BrowserContext* GetBrowserContextByIndex(UserIndex index) override;
  content::BrowserContext* GetBrowserContextForWindow(
      aura::Window* window) override;
  content::BrowserContext* GetUserPresentingBrowserContextForWindow(
      aura::Window* window) override;

  std::unique_ptr<content::BrowserContext> active_browser_context_;

  DISALLOW_COPY_AND_ASSIGN(TestShellContentState);
};

}  // namespace ash

#endif  // ASH_TEST_CONTENT_TEST_SHELL_CONTENT_STATE_H_
