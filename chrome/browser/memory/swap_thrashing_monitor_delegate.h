// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_MEMORY_SWAP_THRASHING_MONITOR_DELEGATE_H_
#define CHROME_BROWSER_MEMORY_SWAP_THRASHING_MONITOR_DELEGATE_H_

#include "base/macros.h"
#include "build/build_config.h"

namespace memory {

enum class SwapThrashingLevel {
  // There's no thrashing happening on the system or the state is undetermined.
  //
  // There's no possible transition from this state to the confirmed one, the
  // system always go through the suspected state before confirming thrashing.
  SWAP_THRASHING_LEVEL_NONE,

  // Indicates that there's a suspicion of swap-thrashing but the swapping
  // activity is not sustained.
  SWAP_THRASHING_LEVEL_SUSPECTED,

  // Swap-thrashing is confirmed to affect the system.
  SWAP_THRASHING_LEVEL_CONFIRMED,
};

// Dummy definition of a SwapThrashingMonitorDelegate, the platforms interested
// in monitoring the swap thrashing state should implement this class. They
// should also ensure that the SampleAndCalculateSwapThrashingLevel function
// gets called periodically as it is responsible for querying the state of the
// system. It is recommended to use a frequency around 0.5 and 1Hz.
class SwapThrashingMonitorDelegate {
 public:
  SwapThrashingMonitorDelegate() {}
  virtual ~SwapThrashingMonitorDelegate() {}

  // Calculates the swap-thrashing level over the interval between now and the
  // last time this function was called. This function will always return
  // SWAP_THRASHING_LEVEL_NONE when it gets called for the first time.
  //
  // This function requires sequence-affinity, through use of ThreadChecker. It
  // is also blocking and should be run on a blocking sequenced task runner.
  virtual SwapThrashingLevel SampleAndCalculateSwapThrashingLevel();

 private:
  DISALLOW_COPY_AND_ASSIGN(SwapThrashingMonitorDelegate);
};

}  // namespace memory

#endif  // CHROME_BROWSER_MEMORY_SWAP_THRASHING_MONITOR_DELEGATE_H_
