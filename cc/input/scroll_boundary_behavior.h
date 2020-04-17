// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_INPUT_SCROLL_BOUNDARY_BEHAVIOR_H_
#define CC_INPUT_SCROLL_BOUNDARY_BEHAVIOR_H_

#include "cc/cc_export.h"

namespace cc {

struct CC_EXPORT ScrollBoundaryBehavior {
  enum ScrollBoundaryBehaviorType {
    // Same as contain but also hint that no overscroll affordance should be
    // triggered.
    kScrollBoundaryBehaviorTypeNone,
    // Allows the default behavior for the user agent.
    kScrollBoundaryBehaviorTypeAuto,
    // Hint to disable scroll chaining. The user agent may show an appropriate
    // overscroll affordance.
    kScrollBoundaryBehaviorTypeContain,
    kScrollBoundaryBehaviorTypeMax = kScrollBoundaryBehaviorTypeContain
  };

  ScrollBoundaryBehavior()
      : x(kScrollBoundaryBehaviorTypeAuto),
        y(kScrollBoundaryBehaviorTypeAuto) {}
  explicit ScrollBoundaryBehavior(ScrollBoundaryBehaviorType type)
      : x(type), y(type) {}
  ScrollBoundaryBehavior(ScrollBoundaryBehaviorType x_type,
                         ScrollBoundaryBehaviorType y_type)
      : x(x_type), y(y_type) {}

  ScrollBoundaryBehaviorType x;
  ScrollBoundaryBehaviorType y;

  bool operator==(const ScrollBoundaryBehavior& a) const {
    return (a.x == x) && (a.y == y);
  }
  bool operator!=(const ScrollBoundaryBehavior& a) const {
    return !(*this == a);
  }
};

}  // namespace cc

#endif  // CC_INPUT_SCROLL_BOUNDARY_BEHAVIOR_H_
