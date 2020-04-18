// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_RESOURCE_COORDINATOR_RESOURCE_COORDINATOR_CLOCK_H_
#define SERVICES_RESOURCE_COORDINATOR_RESOURCE_COORDINATOR_CLOCK_H_

#include <memory>

#include "base/macros.h"
#include "base/time/time.h"

namespace base {
class TickClock;
}

namespace resource_coordinator {

// ResourceCoordinatorClock provides timing methods that resource_coordinator/
// needs at a central place, and facilitates testing across components when
// clock manipulation is needed.
class ResourceCoordinatorClock {
 public:
  // Returns time from the testing TickClock if set; otherwise returns time from
  // TimeTicks::Now().
  static base::TimeTicks NowTicks();

  static const base::TickClock* GetClockForTesting();

  // Sets a TickClock for testing.
  static void SetClockForTesting(const base::TickClock* tick_clock);

  static void ResetClockForTesting();

 private:
  DISALLOW_COPY_AND_ASSIGN(ResourceCoordinatorClock);
};

}  // namespace resource_coordinator

#endif  // SERVICES_RESOURCE_COORDINATOR_RESOURCE_COORDINATOR_CLOCK_H_
