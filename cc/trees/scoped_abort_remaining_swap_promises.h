// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_TREES_SCOPED_ABORT_REMAINING_SWAP_PROMISES_H_
#define CC_TREES_SCOPED_ABORT_REMAINING_SWAP_PROMISES_H_

#include "base/macros.h"
#include "cc/trees/swap_promise.h"
#include "cc/trees/swap_promise_manager.h"

namespace cc {

class ScopedAbortRemainingSwapPromises {
 public:
  explicit ScopedAbortRemainingSwapPromises(
      SwapPromiseManager* swap_promise_manager)
      : swap_promise_manager_(swap_promise_manager) {}

  ~ScopedAbortRemainingSwapPromises() {
    swap_promise_manager_->BreakSwapPromises(SwapPromise::COMMIT_FAILS);
  }

 private:
  SwapPromiseManager* swap_promise_manager_;

  DISALLOW_COPY_AND_ASSIGN(ScopedAbortRemainingSwapPromises);
};

}  // namespace cc

#endif  // CC_TREES_SCOPED_ABORT_REMAINING_SWAP_PROMISES_H_
