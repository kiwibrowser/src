// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/ui/ws/event_matcher.h"

#include "base/time/time.h"
#include "services/ui/public/interfaces/event_matcher.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/events/event.h"
#include "ui/events/mojo/event_constants.mojom.h"
#include "ui/gfx/geometry/point.h"

namespace ui {
namespace ws {

// NOTE: Most of the matching functionality is exercised by tests of Accelerator
// handling in the EventDispatcher and WindowTree tests. These are just basic
// sanity checks.

using EventTesterTest = testing::Test;

TEST_F(EventTesterTest, MatchesEventByType) {
  mojom::EventMatcherPtr matcher = mojom::EventMatcher::New();
  matcher->type_matcher = mojom::EventTypeMatcher::New();
  matcher->type_matcher->type = ui::mojom::EventType::POINTER_DOWN;
  EventMatcher pointer_down_matcher(*matcher);

  ui::PointerEvent pointer_down(ui::TouchEvent(
      ui::ET_TOUCH_PRESSED, gfx::Point(), base::TimeTicks(),
      ui::PointerDetails(ui::EventPointerType::POINTER_TYPE_TOUCH, 1)));
  EXPECT_TRUE(pointer_down_matcher.MatchesEvent(pointer_down));

  ui::PointerEvent pointer_up(ui::TouchEvent(
      ui::ET_TOUCH_RELEASED, gfx::Point(), base::TimeTicks(),
      ui::PointerDetails(ui::EventPointerType::POINTER_TYPE_TOUCH, 1)));
  EXPECT_FALSE(pointer_down_matcher.MatchesEvent(pointer_up));
}

TEST_F(EventTesterTest, MatchesEventByKeyCode) {
  mojom::EventMatcherPtr matcher(mojom::EventMatcher::New());
  matcher->type_matcher = mojom::EventTypeMatcher::New();
  matcher->type_matcher->type = ui::mojom::EventType::KEY_PRESSED;
  matcher->key_matcher = mojom::KeyEventMatcher::New();
  matcher->key_matcher->keyboard_code = ui::mojom::KeyboardCode::Z;
  EventMatcher z_matcher(*matcher);

  ui::KeyEvent z_key(ui::ET_KEY_PRESSED, ui::VKEY_Z, ui::EF_NONE);
  EXPECT_TRUE(z_matcher.MatchesEvent(z_key));

  ui::KeyEvent y_key(ui::ET_KEY_PRESSED, ui::VKEY_Y, ui::EF_NONE);
  EXPECT_FALSE(z_matcher.MatchesEvent(y_key));
}

TEST_F(EventTesterTest, MatchesEventByKeyFlags) {
  mojom::EventMatcherPtr matcher(mojom::EventMatcher::New());
  matcher->type_matcher = mojom::EventTypeMatcher::New();
  matcher->type_matcher->type = ui::mojom::EventType::KEY_PRESSED;
  matcher->flags_matcher = mojom::EventFlagsMatcher::New();
  matcher->flags_matcher->flags = ui::mojom::kEventFlagControlDown;
  matcher->key_matcher = mojom::KeyEventMatcher::New();
  matcher->key_matcher->keyboard_code = ui::mojom::KeyboardCode::N;
  EventMatcher control_n_matcher(*matcher);

  ui::KeyEvent control_n(ui::ET_KEY_PRESSED, ui::VKEY_N, ui::EF_CONTROL_DOWN);
  EXPECT_TRUE(control_n_matcher.MatchesEvent(control_n));

  ui::KeyEvent shift_n(ui::ET_KEY_PRESSED, ui::VKEY_N, ui::EF_SHIFT_DOWN);
  EXPECT_FALSE(control_n_matcher.MatchesEvent(shift_n));
}

}  // namespace ws
}  // namespace ui
