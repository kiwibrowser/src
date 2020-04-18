// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/public/cpp/config.h"
#include "ash/shell.h"
#include "ash/test/ash_test_base.h"
#include "ash/test/ash_test_helper.h"
#include "ui/aura/test/mus/test_window_manager_client.h"
#include "ui/aura/test/mus/test_window_tree.h"
#include "ui/aura/test/mus/test_window_tree_client_setup.h"
#include "ui/aura/window.h"

namespace ash {

using WindowManagerCommonTest = AshTestBase;

// TODO(jamescook): Move into one of the existing WindowManager test suites.
TEST_F(WindowManagerCommonTest, Focus) {
  if (Shell::GetAshConfig() == Config::CLASSIC)
    return;

  // Ensure that activation parents have been added, an ancestor of |window|
  // must support activation for the focus attempt below to succeed.
  aura::TestWindowManagerClient* test_window_manager_client =
      ash_test_helper()
          ->window_tree_client_setup()
          ->test_window_manager_client();
  EXPECT_LT(0u,
            test_window_manager_client->GetChangeCountForType(
                aura::WindowManagerClientChangeType::ADD_ACTIVATION_PARENT));

  // Ensure a call to Window::Focus() makes it way to the WindowTree.
  std::unique_ptr<aura::Window> window = CreateTestWindow();
  aura::TestWindowTree* test_window_tree =
      ash_test_helper()->window_tree_client_setup()->window_tree();
  const size_t initial_focus_count = test_window_tree->GetChangeCountForType(
      aura::WindowTreeChangeType::FOCUS);
  window->Focus();
  EXPECT_TRUE(window->HasFocus());
  EXPECT_EQ(initial_focus_count + 1, test_window_tree->GetChangeCountForType(
                                         aura::WindowTreeChangeType::FOCUS));
}

}  // namespace ash
