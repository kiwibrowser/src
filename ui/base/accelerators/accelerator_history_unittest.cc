// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/base/accelerators/accelerator_history.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ui {

TEST(AcceleratorHistoryTest, SimulatePressAndHold) {
  AcceleratorHistory history;
  Accelerator alt_press(ui::VKEY_MENU, ui::EF_NONE,
                        ui::Accelerator::KeyState::PRESSED);
  history.StoreCurrentAccelerator(alt_press);
  EXPECT_EQ(alt_press, history.current_accelerator());

  // Repeats don't affect previous accelerators.
  history.StoreCurrentAccelerator(alt_press);
  EXPECT_EQ(alt_press, history.current_accelerator());
  EXPECT_NE(alt_press, history.previous_accelerator());

  Accelerator search_alt_press(ui::VKEY_LWIN, ui::EF_ALT_DOWN,
                               ui::Accelerator::KeyState::PRESSED);
  history.StoreCurrentAccelerator(search_alt_press);
  EXPECT_EQ(search_alt_press, history.current_accelerator());
  EXPECT_EQ(alt_press, history.previous_accelerator());
  history.StoreCurrentAccelerator(search_alt_press);
  EXPECT_EQ(search_alt_press, history.current_accelerator());
  EXPECT_EQ(alt_press, history.previous_accelerator());

  Accelerator alt_release_search_down(ui::VKEY_MENU, ui::EF_COMMAND_DOWN,
                                      ui::Accelerator::KeyState::RELEASED);
  history.StoreCurrentAccelerator(alt_release_search_down);
  EXPECT_EQ(alt_release_search_down, history.current_accelerator());
  EXPECT_EQ(search_alt_press, history.previous_accelerator());

  // Search is still down and search presses will keep being generated, but from
  // the perspective of the AcceleratorHistory, this is the same Search press
  // that hasn't been released yet.
  Accelerator search_press(ui::VKEY_LWIN, ui::EF_NONE,
                           ui::Accelerator::KeyState::PRESSED);
  history.StoreCurrentAccelerator(search_press);
  history.StoreCurrentAccelerator(search_press);
  history.StoreCurrentAccelerator(search_press);
  EXPECT_EQ(alt_release_search_down, history.current_accelerator());
  EXPECT_EQ(search_alt_press, history.previous_accelerator());

  Accelerator search_release(ui::VKEY_LWIN, ui::EF_NONE,
                             ui::Accelerator::KeyState::RELEASED);
  history.StoreCurrentAccelerator(search_release);
  EXPECT_EQ(search_release, history.current_accelerator());
  EXPECT_EQ(alt_release_search_down, history.previous_accelerator());
}

}  // namespace ui
