// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_RESOURCE_COORDINATOR_LIFECYCLE_STATE_H_
#define CHROME_BROWSER_RESOURCE_COORDINATOR_LIFECYCLE_STATE_H_

namespace resource_coordinator {

// Note that these states are emitted via UKMs, so the integer values should
// remain consistent.
enum class LifecycleState {
  // The LifecycleUnit is alive and active with no CPU throttling.
  ACTIVE = 0,
  // The LifecycleUnit is currently being CPU throttled.
  THROTTLED = 1,
  // The LifecycleUnit is currently transitioning to a FROZEN state.
  PENDING_FREEZE = 2,
  // The LifecycleUnit is frozen.
  FROZEN = 3,
  // The LifecycleUnit is currently transitioning to a DISCARDED state.
  PENDING_DISCARD = 4,
  // The LifecycleUnit is discarded, and is consuming no system resources.
  DISCARDED = 5,
};

// An enumeration of reasons why a lifecycle state change was applied. These are
// also emitted via UKM so need to remain stable.
enum class LifecycleStateChangeReason {
  // Policy in the browser decided to initiate the state change.
  BROWSER_INITIATED = 0,
  // Policy in the renderer decided to initiate the state change.
  RENDERER_INITIATED = 1,
  // A system wide memory pressure condition initiated the state change.
  SYSTEM_MEMORY_PRESSURE = 2,
};

}  // namespace resource_coordinator

#endif  // CHROME_BROWSER_RESOURCE_COORDINATOR_LIFECYCLE_STATE_H_
