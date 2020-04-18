// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/shell/browser/shell.h"

#include "content/public/browser/web_contents.h"
#include "content/shell/browser/shell_platform_data_aura.h"
#include "ui/aura/env.h"
#include "ui/aura/test/test_screen.h"
#include "ui/aura/window.h"
#include "ui/aura/window_event_dispatcher.h"

namespace content {

// static
void Shell::PlatformInitialize(const gfx::Size& default_window_size) {
  CHECK(!platform_);
  aura::TestScreen* screen = aura::TestScreen::Create(gfx::Size());
  display::Screen::SetScreenInstance(screen);
  platform_ = new ShellPlatformDataAura(default_window_size);
}

void Shell::PlatformExit() {
  delete platform_;
  platform_ = nullptr;
}

void Shell::PlatformCleanUp() {
}

void Shell::PlatformEnableUIControl(UIControl control, bool is_enabled) {
}

void Shell::PlatformSetAddressBarURL(const GURL& url) {
}

void Shell::PlatformSetIsLoading(bool loading) {
}

void Shell::PlatformCreateWindow(int width, int height) {
  CHECK(platform_);
  if (!headless_)
    platform_->ShowWindow();
  platform_->ResizeWindow(gfx::Size(width, height));
}

void Shell::PlatformSetContents() {
  CHECK(platform_);
  aura::Window* content = web_contents_->GetNativeView();
  aura::Window* parent = platform_->host()->window();
  if (!parent->Contains(content))
    parent->AddChild(content);

  content->Show();
}

void Shell::PlatformResizeSubViews() {
}

void Shell::Close() {
  delete this;
}

void Shell::PlatformSetTitle(const base::string16& title) {
}

}  // namespace content
