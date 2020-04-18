// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_EVENT_MATCHER_UTIL_H_
#define ASH_EVENT_MATCHER_UTIL_H_

#include "services/ui/public/interfaces/event_matcher.mojom.h"

namespace ash {

// Builds a matcher which matches all key releases.
ui::mojom::EventMatcherPtr BuildKeyReleaseMatcher();

// Builds a matcher which matches any use of these modifier keys.
ui::mojom::EventMatcherPtr BuildAltMatcher();
ui::mojom::EventMatcherPtr BuildControlMatcher();

// Builds a matcher which matches a specific keycode.
ui::mojom::EventMatcherPtr BuildKeyMatcher(ui::mojom::KeyboardCode code);

// Adds key matchers for the range |start| to |end| inclusive.
void BuildKeyMatcherRange(ui::mojom::KeyboardCode start,
                          ui::mojom::KeyboardCode end,
                          std::vector<::ui::mojom::EventMatcherPtr>* matchers);

// Adds matchers for |codes| to |matchers|.
void BuildKeyMatcherList(std::vector<ui::mojom::KeyboardCode> codes,
                         std::vector<::ui::mojom::EventMatcherPtr>* matchers);

}  // namespace ash

#endif  // ASH_EVENT_MATCHER_UTIL_H_
