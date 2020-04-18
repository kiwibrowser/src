// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/aura/test/mus/test_window_tree_client_setup.h"

#include "ui/aura/test/mus/test_window_manager_client.h"
#include "ui/aura/test/mus/test_window_tree.h"
#include "ui/aura/test/mus/window_tree_client_private.h"
#include "ui/display/display.h"

namespace aura {

TestWindowTreeClientSetup::TestWindowTreeClientSetup() {}

TestWindowTreeClientSetup::~TestWindowTreeClientSetup() {}

void TestWindowTreeClientSetup::Init(
    WindowTreeClientDelegate* window_tree_delegate) {
  CommonInit(window_tree_delegate, nullptr, WindowTreeClient::Config::kMash);
  WindowTreeClientPrivate(window_tree_client_.get())
      .OnEmbed(window_tree_.get());
}

void TestWindowTreeClientSetup::InitForWindowManager(
    WindowTreeClientDelegate* window_tree_delegate,
    WindowManagerDelegate* window_manager_delegate) {
  test_window_manager_client_ = std::make_unique<TestWindowManagerClient>();
  CommonInit(window_tree_delegate, window_manager_delegate,
             WindowTreeClient::Config::kMash);
  WindowTreeClientPrivate window_tree_client_private(window_tree_client_.get());
  window_tree_client_private.SetTree(window_tree_.get());
  window_tree_->set_window_manager(window_tree_client_.get());
  window_tree_client_private.SetWindowManagerClient(
      test_window_manager_client_.get());
}

void TestWindowTreeClientSetup::InitWithoutEmbed(
    WindowTreeClientDelegate* window_tree_delegate,
    WindowTreeClient::Config config) {
  CommonInit(window_tree_delegate, nullptr, config);
  WindowTreeClientPrivate(window_tree_client_.get())
      .SetTree(window_tree_.get());
}

void TestWindowTreeClientSetup::NotifyClientAboutAcceleratedWidgets(
    display::DisplayManager* display_manager) {
  window_tree_->NotifyClientAboutAcceleratedWidgets(display_manager);
}

std::unique_ptr<WindowTreeClient>
TestWindowTreeClientSetup::OwnWindowTreeClient() {
  DCHECK(window_tree_client_);
  return std::move(window_tree_client_);
}

WindowTreeClient* TestWindowTreeClientSetup::window_tree_client() {
  return window_tree_client_.get();
}

void TestWindowTreeClientSetup::CommonInit(
    WindowTreeClientDelegate* window_tree_delegate,
    WindowManagerDelegate* window_manager_delegate,
    WindowTreeClient::Config config) {
  window_tree_ = std::make_unique<TestWindowTree>();
  window_tree_client_ = WindowTreeClientPrivate::CreateWindowTreeClient(
      window_tree_delegate, window_manager_delegate, config);
  window_tree_->set_client(window_tree_client_.get());
}

}  // namespace aura
