// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/shell/content/shell_content_state_impl.h"

#include "content/public/browser/browser_context.h"

namespace ash {

ShellContentStateImpl::ShellContentStateImpl(content::BrowserContext* context)
    : browser_context_(context) {}
ShellContentStateImpl::~ShellContentStateImpl() = default;

content::BrowserContext* ShellContentStateImpl::GetActiveBrowserContext() {
  return browser_context_;
}

content::BrowserContext* ShellContentStateImpl::GetBrowserContextByIndex(
    UserIndex index) {
  return browser_context_;
}

content::BrowserContext* ShellContentStateImpl::GetBrowserContextForWindow(
    aura::Window* window) {
  return browser_context_;
}

content::BrowserContext*
ShellContentStateImpl::GetUserPresentingBrowserContextForWindow(
    aura::Window* window) {
  return browser_context_;
}

}  // namespace ash
