// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/test/content/test_shell_content_state.h"

#include "content/public/test/test_browser_context.h"

namespace ash {

TestShellContentState::TestShellContentState() = default;
TestShellContentState::~TestShellContentState() = default;

content::BrowserContext* TestShellContentState::GetActiveBrowserContext() {
  active_browser_context_.reset(new content::TestBrowserContext());
  return active_browser_context_.get();
}

content::BrowserContext* TestShellContentState::GetBrowserContextByIndex(
    UserIndex index) {
  return nullptr;
}

content::BrowserContext* TestShellContentState::GetBrowserContextForWindow(
    aura::Window* window) {
  return nullptr;
}

content::BrowserContext*
TestShellContentState::GetUserPresentingBrowserContextForWindow(
    aura::Window* window) {
  return nullptr;
}

}  // namespace ash
