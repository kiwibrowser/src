// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_ASH_CHROME_SHELL_CONTENT_STATE_H_
#define CHROME_BROWSER_UI_ASH_CHROME_SHELL_CONTENT_STATE_H_

#include "ash/content/shell_content_state.h"
#include "base/macros.h"

class ChromeShellContentState : public ash::ShellContentState {
 public:
  ChromeShellContentState();
  ~ChromeShellContentState() override;

  static ChromeShellContentState* GetInstance();

  // Overridden from ash::ShellContentState:
  content::BrowserContext* GetActiveBrowserContext() override;
  content::BrowserContext* GetBrowserContextByIndex(
      ash::UserIndex index) override;
  content::BrowserContext* GetBrowserContextForWindow(
      aura::Window* window) override;
  content::BrowserContext* GetUserPresentingBrowserContextForWindow(
      aura::Window* window) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(ChromeShellContentState);
};

#endif  // CHROME_BROWSER_UI_ASH_CHROME_SHELL_CONTENT_STATE_H_
