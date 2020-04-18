// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/memory/swap_thrashing_monitor_delegate.h"

#include "base/logging.h"

namespace memory {

SwapThrashingLevel
SwapThrashingMonitorDelegate::SampleAndCalculateSwapThrashingLevel() {
  NOTIMPLEMENTED();
  return SwapThrashingLevel::SWAP_THRASHING_LEVEL_NONE;
}

}  // namespace memory
