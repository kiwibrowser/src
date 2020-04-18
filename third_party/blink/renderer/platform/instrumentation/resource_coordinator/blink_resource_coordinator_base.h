// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_INSTRUMENTATION_RESOURCE_COORDINATOR_BLINK_RESOURCE_COORDINATOR_BASE_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_INSTRUMENTATION_RESOURCE_COORDINATOR_BLINK_RESOURCE_COORDINATOR_BASE_H_

#include "services/resource_coordinator/public/cpp/resource_coordinator_features.h"
#include "third_party/blink/renderer/platform/platform_export.h"
#include "third_party/blink/renderer/platform/wtf/noncopyable.h"

namespace blink {

// Base class for Resource Coordinators in Blink.
class PLATFORM_EXPORT BlinkResourceCoordinatorBase {
  WTF_MAKE_NONCOPYABLE(BlinkResourceCoordinatorBase);

 public:
  static bool IsEnabled() {
    return resource_coordinator::IsResourceCoordinatorEnabled();
  }

  BlinkResourceCoordinatorBase() = default;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_INSTRUMENTATION_RESOURCE_COORDINATOR_BLINK_RESOURCE_COORDINATOR_BASE_H_
