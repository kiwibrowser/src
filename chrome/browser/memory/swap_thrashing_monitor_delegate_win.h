// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// The Windows implementation of the delegate used by the swap thrashing
// monitor.

#ifndef CHROME_BROWSER_MEMORY_SWAP_THRASHING_MONITOR_DELEGATE_WIN_H_
#define CHROME_BROWSER_MEMORY_SWAP_THRASHING_MONITOR_DELEGATE_WIN_H_

#include <list>

#include "base/containers/circular_deque.h"
#include "base/macros.h"
#include "base/optional.h"
#include "base/sequence_checker.h"
#include "chrome/browser/memory/swap_thrashing_monitor_delegate.h"

namespace memory {

class SwapThrashingMonitorDelegateWin : public SwapThrashingMonitorDelegate {
 public:
  SwapThrashingMonitorDelegateWin();
  ~SwapThrashingMonitorDelegateWin() override;

  SwapThrashingLevel SampleAndCalculateSwapThrashingLevel() override;

 protected:
  // Used to compute how many of the recent samples are above a given threshold.
  class HardFaultDeltasWindow {
   public:
    HardFaultDeltasWindow();

    // Virtual for unittesting.
    virtual ~HardFaultDeltasWindow();

    // Should be called when a new hard-page fault observation is made. The
    // delta with the previous observation will be added to the list of deltas.
    void OnObservation(uint64_t hard_fault_count);

    size_t observation_above_threshold_count() {
      return observation_above_threshold_count_;
    }

   protected:
    static const size_t kHardFaultDeltasWindowSize;

    base::Optional<uint64_t> latest_hard_fault_count_;

    // Delta between each observation.
    base::circular_deque<size_t> observation_deltas_;

    // Number of observations that are above the high swapping threshold.
    size_t observation_above_threshold_count_;

   private:
    SEQUENCE_CHECKER(sequence_checker_);

    DISALLOW_COPY_AND_ASSIGN(HardFaultDeltasWindow);
  };

  // Record the sum of the hard page-fault count for all the Chrome processes.
  //
  // Virtual for unittesting.
  virtual bool RecordHardFaultCountForChromeProcesses();

  std::unique_ptr<HardFaultDeltasWindow> hard_fault_deltas_window_;

 private:
  SEQUENCE_CHECKER(sequence_checker_);

  DISALLOW_COPY_AND_ASSIGN(SwapThrashingMonitorDelegateWin);
};

}  // namespace memory

#endif  // CHROME_BROWSER_MEMORY_SWAP_THRASHING_MONITOR_DELEGATE_WIN_H_
