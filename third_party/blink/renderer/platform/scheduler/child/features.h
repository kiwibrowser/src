// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_SCHEDULER_CHILD_FEATURES_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_SCHEDULER_CHILD_FEATURES_H_

#include "base/feature_list.h"

namespace blink {
namespace scheduler {

const base::Feature kHighPriorityInput{"BlinkSchedulerHighPriorityInput",
                                       base::FEATURE_DISABLED_BY_DEFAULT};

const base::Feature kDedicatedWorkerThrottling{
    "kBlinkSchedulerWorkerThrottling", base::FEATURE_DISABLED_BY_DEFAULT};

}  // namespace scheduler
}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_SCHEDULER_CHILD_FEATURES_H_
