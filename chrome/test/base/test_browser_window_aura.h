// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_TEST_BASE_TEST_BROWSER_WINDOW_AURA_H_
#define CHROME_TEST_BASE_TEST_BROWSER_WINDOW_AURA_H_

#include "base/macros.h"
#include "chrome/test/base/test_browser_window.h"
#include "ui/aura/window.h"

// A browser window proxy with an associated Aura native window.
class TestBrowserWindowAura : public TestBrowserWindow {
 public:
  explicit TestBrowserWindowAura(std::unique_ptr<aura::Window> native_window);
  ~TestBrowserWindowAura() override;

  // TestBrowserWindow overrides:
  gfx::NativeWindow GetNativeWindow() const override;
  void Show() override;
  void Hide() override;
  bool IsVisible() const override;
  void Activate() override;
  bool IsActive() const override;
  gfx::Rect GetBounds() const override;

  std::unique_ptr<Browser> CreateBrowser(Browser::CreateParams* params);

 private:
  Browser* browser_;  // not owned
  std::unique_ptr<aura::Window> native_window_;

  DISALLOW_COPY_AND_ASSIGN(TestBrowserWindowAura);
};

namespace chrome {

// Helper that creates a browser with a native Aura |window|. If |window| is
// nullptr, it will create an Aura window to associate with the browser. It also
// handles the lifetime of TestBrowserWindowAura.
std::unique_ptr<Browser> CreateBrowserWithAuraTestWindowForParams(
    std::unique_ptr<aura::Window> window,
    Browser::CreateParams* params);

}  // namespace chrome

#endif  // CHROME_TEST_BASE_TEST_BROWSER_WINDOW_AURA_H_
