// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_MEMORY_SWAP_METRICS_DELEGATE_UMA_H_
#define CONTENT_BROWSER_MEMORY_SWAP_METRICS_DELEGATE_UMA_H_

#include "content/public/browser/swap_metrics_driver.h"

#include "base/time/time.h"

namespace content {

// This class records metrics related to the system's swapping behavior.
// Metrics can be platform-specific.
class SwapMetricsDelegateUma : public SwapMetricsDriver::Delegate {
 public:
  SwapMetricsDelegateUma();
  ~SwapMetricsDelegateUma() override;

  void OnSwapInCount(uint64_t count, base::TimeDelta interval) override;
  void OnSwapOutCount(uint64_t count, base::TimeDelta interval) override;
  void OnDecompressedPageCount(uint64_t count,
                               base::TimeDelta interval) override;
  void OnCompressedPageCount(uint64_t count, base::TimeDelta interval) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(SwapMetricsDelegateUma);
};

}  // namespace content

#endif  // CONTENT_BROWSER_MEMORY_SWAP_METRICS_DELEGATE_UMA_H_
