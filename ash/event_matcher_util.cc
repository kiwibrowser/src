// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/event_matcher_util.h"

namespace ash {

ui::mojom::EventMatcherPtr BuildKeyReleaseMatcher() {
  ui::mojom::EventMatcherPtr matcher(ui::mojom::EventMatcher::New());
  matcher->type_matcher = ui::mojom::EventTypeMatcher::New();
  matcher->type_matcher->type = ui::mojom::EventType::KEY_RELEASED;
  return matcher;
}

ui::mojom::EventMatcherPtr BuildAltMatcher() {
  ui::mojom::EventMatcherPtr matcher(ui::mojom::EventMatcher::New());
  matcher->flags_matcher = ui::mojom::EventFlagsMatcher::New();
  matcher->flags_matcher->flags = ui::mojom::kEventFlagAltDown;
  matcher->ignore_flags_matcher = ui::mojom::EventFlagsMatcher::New();
  matcher->ignore_flags_matcher->flags =
      ui::mojom::kEventFlagCapsLockOn | ui::mojom::kEventFlagScrollLockOn |
      ui::mojom::kEventFlagNumLockOn | ui::mojom::kEventFlagControlDown;
  return matcher;
}

ui::mojom::EventMatcherPtr BuildControlMatcher() {
  ui::mojom::EventMatcherPtr matcher(ui::mojom::EventMatcher::New());
  matcher->flags_matcher = ui::mojom::EventFlagsMatcher::New();
  matcher->flags_matcher->flags = ui::mojom::kEventFlagControlDown;
  matcher->ignore_flags_matcher = ui::mojom::EventFlagsMatcher::New();
  matcher->ignore_flags_matcher->flags =
      ui::mojom::kEventFlagCapsLockOn | ui::mojom::kEventFlagScrollLockOn |
      ui::mojom::kEventFlagNumLockOn | ui::mojom::kEventFlagAltDown;
  return matcher;
}

ui::mojom::EventMatcherPtr BuildKeyMatcher(ui::mojom::KeyboardCode code) {
  ui::mojom::EventMatcherPtr matcher(ui::mojom::EventMatcher::New());
  matcher->key_matcher = ui::mojom::KeyEventMatcher::New();
  matcher->key_matcher->keyboard_code = code;
  return matcher;
}

void BuildKeyMatcherRange(ui::mojom::KeyboardCode start,
                          ui::mojom::KeyboardCode end,
                          std::vector<::ui::mojom::EventMatcherPtr>* matchers) {
  for (int i = static_cast<int>(start); i <= static_cast<int>(end); ++i) {
    matchers->push_back(
        BuildKeyMatcher(static_cast<ui::mojom::KeyboardCode>(i)));
  }
}

void BuildKeyMatcherList(std::vector<ui::mojom::KeyboardCode> codes,
                         std::vector<::ui::mojom::EventMatcherPtr>* matchers) {
  for (auto& code : codes)
    matchers->push_back(BuildKeyMatcher(code));
}

}  // namespace ash
