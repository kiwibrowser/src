// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/memory/memory_coordinator_default_policy.h"

namespace content {

MemoryCoordinatorDefaultPolicy::MemoryCoordinatorDefaultPolicy(
    MemoryCoordinatorImpl* coordinator)
    : coordinator_(coordinator) {
  DCHECK(coordinator_);
}

MemoryCoordinatorDefaultPolicy::~MemoryCoordinatorDefaultPolicy() {}

void MemoryCoordinatorDefaultPolicy::OnCriticalCondition() {
  // Just trigger tab discarding for now.
  coordinator_->DiscardTab(true);
}

void MemoryCoordinatorDefaultPolicy::OnConditionChanged(MemoryCondition prev,
                                                        MemoryCondition next) {}

void MemoryCoordinatorDefaultPolicy::OnChildVisibilityChanged(
    int render_process_id,
    bool is_visible) {}

}  // namespace content
