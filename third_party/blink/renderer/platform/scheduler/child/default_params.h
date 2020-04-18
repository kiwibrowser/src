// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_SCHEDULER_CHILD_DEFAULT_PARAMS_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_SCHEDULER_CHILD_DEFAULT_PARAMS_H_

#include "third_party/blink/renderer/platform/scheduler/child/page_visibility_state.h"

namespace blink {
namespace scheduler {

constexpr PageVisibilityState kDefaultPageVisibility =
    PageVisibilityState::kVisible;

}  // namespace scheduler
}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_SCHEDULER_CHILD_DEFAULT_PARAMS_H_
