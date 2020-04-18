// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/aura/mus/window_tree_host_mus.h"

#include "ui/aura/mus/window_port_mus.h"
#include "ui/aura/mus/window_tree_host_mus_init_params.h"
#include "ui/aura/test/aura_mus_test_base.h"
#include "ui/aura/test/mus/test_window_tree.h"

namespace aura {

using WindowTreeHostMusTest = aura::test::AuraMusClientTestBase;

TEST_F(WindowTreeHostMusTest, UpdateClientArea) {
  std::unique_ptr<WindowTreeHostMus> window_tree_host_mus =
      std::make_unique<WindowTreeHostMus>(
          aura::CreateInitParamsForTopLevel(window_tree_client_impl()));

  gfx::Insets new_insets(10, 11, 12, 13);
  window_tree_host_mus->SetClientArea(new_insets, std::vector<gfx::Rect>());
  EXPECT_EQ(new_insets, window_tree()->last_client_area());
}

TEST_F(WindowTreeHostMusTest, SetHitTestMask) {
  std::unique_ptr<WindowTreeHostMus> window_tree_host_mus =
      std::make_unique<WindowTreeHostMus>(
          CreateInitParamsForTopLevel(window_tree_client_impl()));

  EXPECT_FALSE(window_tree()->last_hit_test_mask().has_value());
  gfx::Rect mask(10, 10, 10, 10);
  WindowPortMus::Get(window_tree_host_mus->window())->SetHitTestMask(mask);
  ASSERT_TRUE(window_tree()->last_hit_test_mask().has_value());
  EXPECT_EQ(mask, window_tree()->last_hit_test_mask());

  WindowPortMus::Get(window_tree_host_mus->window())
      ->SetHitTestMask(base::nullopt);
  ASSERT_FALSE(window_tree()->last_hit_test_mask().has_value());
}

TEST_F(WindowTreeHostMusTest, PerformWmAction) {
  std::unique_ptr<WindowTreeHostMus> window_tree_host_mus =
      std::make_unique<WindowTreeHostMus>(
          CreateInitParamsForTopLevel(window_tree_client_impl()));

  const std::string test_action("test-action");
  window_tree_host_mus->PerformWmAction(test_action);
  EXPECT_EQ(test_action, window_tree()->last_wm_action());
}

}  // namespace aura
