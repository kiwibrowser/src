// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromecast/graphics/cast_window_manager.h"

#include <memory>

#include "ui/aura/client/focus_client.h"
#include "ui/aura/test/aura_test_base.h"
#include "ui/aura/test/test_window_delegate.h"
#include "ui/aura/window.h"
#include "ui/events/test/event_generator.h"

namespace chromecast {
namespace test {

using CastWindowManagerAuraTest = aura::test::AuraTestBase;

class CastTestWindowDelegate : public aura::test::TestWindowDelegate {
 public:
  CastTestWindowDelegate() : key_code_(ui::VKEY_UNKNOWN) {}
  ~CastTestWindowDelegate() override {}

  // Overridden from TestWindowDelegate:
  void OnKeyEvent(ui::KeyEvent* event) override {
    key_code_ = event->key_code();
  }

  ui::KeyboardCode key_code() { return key_code_; }

 private:
  ui::KeyboardCode key_code_;

  DISALLOW_COPY_AND_ASSIGN(CastTestWindowDelegate);
};

TEST_F(CastWindowManagerAuraTest, InitialWindowId) {
  CastTestWindowDelegate window_delegate;
  aura::Window window(&window_delegate);
  window.Init(ui::LAYER_NOT_DRAWN);

  // We have chosen WindowId::BOTTOM to match the initial window ID of an Aura
  // window so that z-ordering works correctly.
  EXPECT_EQ(window.id(), CastWindowManager::WindowId::BOTTOM);
}

TEST_F(CastWindowManagerAuraTest, WindowInput) {
  std::unique_ptr<CastWindowManager> window_manager(
      CastWindowManager::Create(true /* enable input */));

  CastTestWindowDelegate window_delegate;
  aura::Window window(&window_delegate);
  window.Init(ui::LAYER_NOT_DRAWN);
  window.SetName("event window");
  window_manager->AddWindow(&window);
  window.SetBounds(gfx::Rect(0, 0, 1280, 720));
  window.Show();
  EXPECT_FALSE(window.IsRootWindow());
  EXPECT_TRUE(window.GetHost());

  // Confirm that the Aura focus client tracks window focus correctly.
  aura::client::FocusClient* focus_client =
      aura::client::GetFocusClient(&window);
  EXPECT_TRUE(focus_client);
  EXPECT_FALSE(focus_client->GetFocusedWindow());
  window.Focus();
  EXPECT_EQ(&window, focus_client->GetFocusedWindow());

  // Confirm that a keyboard event is delivered to the window.
  ui::test::EventGenerator event_generator(&window);
  event_generator.PressKey(ui::VKEY_0, ui::EF_NONE);
  EXPECT_EQ(ui::VKEY_0, window_delegate.key_code());
}

TEST_F(CastWindowManagerAuraTest, WindowInputDisabled) {
  std::unique_ptr<CastWindowManager> window_manager(
      CastWindowManager::Create(false /* enable input */));

  CastTestWindowDelegate window_delegate;
  aura::Window window(&window_delegate);
  window.Init(ui::LAYER_NOT_DRAWN);
  window.SetName("event window");
  window_manager->AddWindow(&window);
  window.SetBounds(gfx::Rect(0, 0, 1280, 720));
  window.Show();
  EXPECT_FALSE(window.IsRootWindow());
  EXPECT_TRUE(window.GetHost());

  // Confirm that the Aura focus client tracks window focus correctly.
  aura::client::FocusClient* focus_client =
      aura::client::GetFocusClient(&window);
  EXPECT_TRUE(focus_client);
  EXPECT_FALSE(focus_client->GetFocusedWindow());
  window.Focus();
  EXPECT_EQ(&window, focus_client->GetFocusedWindow());

  // Confirm that a key event is *not* delivered to the window when input is
  // disabled.
  ui::test::EventGenerator event_generator(&window);
  event_generator.PressKey(ui::VKEY_0, ui::EF_NONE);
  EXPECT_EQ(ui::VKEY_UNKNOWN, window_delegate.key_code());
}

}  // namespace test
}  // namespace chromecast
