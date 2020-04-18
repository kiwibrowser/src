// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_RESOURCE_COORDINATOR_PUBLIC_CPP_MEMORY_INSTRUMENTATION_COORDINATOR_H_
#define SERVICES_RESOURCE_COORDINATOR_PUBLIC_CPP_MEMORY_INSTRUMENTATION_COORDINATOR_H_

#include "services/resource_coordinator/public/mojom/memory_instrumentation/memory_instrumentation.mojom.h"

namespace service_manager {
struct BindSourceInfo;
}

namespace memory_instrumentation {

class Coordinator {
 public:
  // Binds a CoordinatorRequest to this Coordinator instance.
  virtual void BindCoordinatorRequest(
      mojom::CoordinatorRequest,
      const service_manager::BindSourceInfo& source_info) = 0;
};

}  // namespace memory_instrumentation

#endif  // SERVICES_RESOURCE_COORDINATOR_PUBLIC_CPP_MEMORY_INSTRUMENTATION_COORDINATOR_H_
