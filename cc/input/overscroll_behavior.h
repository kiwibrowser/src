// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_INPUT_OVERSCROLL_BEHAVIOR_H_
#define CC_INPUT_OVERSCROLL_BEHAVIOR_H_

#include "cc/cc_export.h"

namespace cc {

struct CC_EXPORT OverscrollBehavior {
  enum OverscrollBehaviorType {
    // Same as contain but also hint that no overscroll affordance should be
    // triggered.
    kOverscrollBehaviorTypeNone,
    // Allows the default behavior for the user agent.
    kOverscrollBehaviorTypeAuto,
    // Hint to disable scroll chaining. The user agent may show an appropriate
    // overscroll affordance.
    kOverscrollBehaviorTypeContain,
    kOverscrollBehaviorTypeMax = kOverscrollBehaviorTypeContain
  };

  OverscrollBehavior()
      : x(kOverscrollBehaviorTypeAuto), y(kOverscrollBehaviorTypeAuto) {}
  explicit OverscrollBehavior(OverscrollBehaviorType type) : x(type), y(type) {}
  OverscrollBehavior(OverscrollBehaviorType x_type,
                     OverscrollBehaviorType y_type)
      : x(x_type), y(y_type) {}

  OverscrollBehaviorType x;
  OverscrollBehaviorType y;

  bool operator==(const OverscrollBehavior& a) const {
    return (a.x == x) && (a.y == y);
  }
  bool operator!=(const OverscrollBehavior& a) const { return !(*this == a); }
};

}  // namespace cc

#endif  // CC_INPUT_OVERSCROLL_BEHAVIOR_H_
