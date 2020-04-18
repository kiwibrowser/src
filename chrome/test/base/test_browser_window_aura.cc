// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/test/base/test_browser_window_aura.h"

#include <utility>

#include "base/memory/ptr_util.h"
#include "ui/wm/public/activation_client.h"

namespace chrome {

std::unique_ptr<Browser> CreateBrowserWithAuraTestWindowForParams(
    std::unique_ptr<aura::Window> window,
    Browser::CreateParams* params) {
  if (window.get() == nullptr) {
    window.reset(new aura::Window(nullptr));
    window->set_id(0);
    window->SetType(aura::client::WINDOW_TYPE_NORMAL);
    window->Init(ui::LAYER_TEXTURED);
    window->Show();
  }

  TestBrowserWindowAura* browser_window =
      new TestBrowserWindowAura(std::move(window));
  new TestBrowserWindowOwner(browser_window);
  return browser_window->CreateBrowser(params);
}

}  // namespace chrome

TestBrowserWindowAura::TestBrowserWindowAura(
    std::unique_ptr<aura::Window> native_window)
    : native_window_(std::move(native_window)) {}

TestBrowserWindowAura::~TestBrowserWindowAura() {}

gfx::NativeWindow TestBrowserWindowAura::GetNativeWindow() const {
  return native_window_.get();
}

void TestBrowserWindowAura::Show() {
  native_window_->Show();
}

void TestBrowserWindowAura::Hide() {
  native_window_->Hide();
}

bool TestBrowserWindowAura::IsVisible() const {
  return native_window_->IsVisible();
}

void TestBrowserWindowAura::Activate() {
  CHECK(native_window_->GetRootWindow())
      << "A TestBrowserWindowAura must have a root window to be activated.";
  ::wm::GetActivationClient(native_window_->GetRootWindow())
      ->ActivateWindow(native_window_.get());
}

bool TestBrowserWindowAura::IsActive() const {
  // A test window might not be parented.
  if (!native_window_->GetRootWindow())
    return false;
  return ::wm::GetActivationClient(native_window_->GetRootWindow())
             ->GetActiveWindow() == native_window_.get();
}

gfx::Rect TestBrowserWindowAura::GetBounds() const {
  return native_window_->bounds();
}

std::unique_ptr<Browser> TestBrowserWindowAura::CreateBrowser(
    Browser::CreateParams* params) {
  params->window = this;
  browser_ = new Browser(*params);
  return base::WrapUnique(browser_);
}
