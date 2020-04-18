// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/wm/core/easy_resize_window_targeter.h"

#include "ui/aura/mus/window_port_mus.h"
#include "ui/aura/test/aura_mus_test_base.h"
#include "ui/aura/test/mus/test_window_tree.h"
#include "ui/aura/window.h"
#include "ui/compositor/layer_type.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/geometry/rect.h"

namespace wm {

namespace {

class TestEasyResizeWindowTargeter : public EasyResizeWindowTargeter {
 public:
  explicit TestEasyResizeWindowTargeter(aura::Window* window)
      : EasyResizeWindowTargeter(window, gfx::Insets(), gfx::Insets()) {}
};

}  // namespace

using EasyResizeWindowTargeterTest = aura::test::AuraMusClientTestBase;

TEST_F(EasyResizeWindowTargeterTest, SetHitTestMask) {
  aura::Window window(nullptr);
  window.Init(ui::LAYER_NOT_DRAWN);
  TestEasyResizeWindowTargeter window_targeter(&window);
  const gfx::Rect bounds1 = gfx::Rect(10, 20, 200, 300);
  window.SetBounds(bounds1);
  const gfx::Insets insets1(1, 2, 3, 4);
  window_targeter.SetInsets(insets1, insets1);
  ASSERT_TRUE(window_tree()->last_hit_test_mask().has_value());
  EXPECT_EQ(gfx::Rect(insets1.left(), insets1.top(),
                      bounds1.width() - insets1.width(),
                      bounds1.height() - insets1.height()),
            *window_tree()->last_hit_test_mask());

  // Adjusting the bounds should trigger resetting the mask.
  const gfx::Rect bounds2 = gfx::Rect(10, 20, 300, 400);
  window.SetBounds(bounds2);
  ASSERT_TRUE(window_tree()->last_hit_test_mask().has_value());
  EXPECT_EQ(gfx::Rect(insets1.left(), insets1.top(),
                      bounds2.width() - insets1.width(),
                      bounds2.height() - insets1.height()),
            *window_tree()->last_hit_test_mask());

  // Empty insets should reset the mask.
  window_targeter.SetInsets(gfx::Insets(), gfx::Insets());
  EXPECT_FALSE(window_tree()->last_hit_test_mask().has_value());

  const gfx::Insets insets2(-1, 3, 4, 5);
  const gfx::Insets effective_insets2(0, 3, 4, 5);
  window_targeter.SetInsets(insets2, insets2);
  ASSERT_TRUE(window_tree()->last_hit_test_mask().has_value());
  EXPECT_EQ(gfx::Rect(effective_insets2.left(), effective_insets2.top(),
                      bounds2.width() - effective_insets2.width(),
                      bounds2.height() - effective_insets2.height()),
            *window_tree()->last_hit_test_mask());
}

}  // namespace wm
