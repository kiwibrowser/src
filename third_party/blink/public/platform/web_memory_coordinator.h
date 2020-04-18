// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_PUBLIC_PLATFORM_WEB_MEMORY_COORDINATOR_H_
#define THIRD_PARTY_BLINK_PUBLIC_PLATFORM_WEB_MEMORY_COORDINATOR_H_

#include "third_party/blink/public/platform/web_common.h"
#include "third_party/blink/public/platform/web_memory_pressure_level.h"
#include "third_party/blink/public/platform/web_memory_state.h"

namespace blink {

class WebMemoryCoordinator {
 public:
  // Called when a memory pressure notification is received.
  // TODO(bashi): Deprecating. Remove this when MemoryPressureListener is
  // gone.
  BLINK_PLATFORM_EXPORT static void OnMemoryPressure(WebMemoryPressureLevel);

  BLINK_PLATFORM_EXPORT static void OnMemoryStateChange(MemoryState);

  BLINK_PLATFORM_EXPORT static void OnPurgeMemory();
};

}  // namespace blink

#endif
