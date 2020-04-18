// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/memory/swap_metrics_delegate_uma.h"

#include "base/metrics/histogram_macros.h"

namespace content {

SwapMetricsDelegateUma::SwapMetricsDelegateUma() = default;

SwapMetricsDelegateUma::~SwapMetricsDelegateUma() = default;

void SwapMetricsDelegateUma::OnSwapInCount(uint64_t count,
                                           base::TimeDelta interval) {
  UMA_HISTOGRAM_COUNTS_10000(
      "Memory.Experimental.SwapInPerSecond",
      static_cast<double>(count) / interval.InSecondsF());
}

void SwapMetricsDelegateUma::OnSwapOutCount(uint64_t count,
                                            base::TimeDelta interval) {
  UMA_HISTOGRAM_COUNTS_10000(
      "Memory.Experimental.SwapOutPerSecond",
      static_cast<double>(count) / interval.InSecondsF());
}

void SwapMetricsDelegateUma::OnDecompressedPageCount(uint64_t count,
                                                     base::TimeDelta interval) {
  UMA_HISTOGRAM_COUNTS_10000(
      "Memory.Experimental.DecompressedPagesPerSecond",
      static_cast<double>(count) / interval.InSecondsF());
}

void SwapMetricsDelegateUma::OnCompressedPageCount(uint64_t count,
                                                   base::TimeDelta interval) {
  UMA_HISTOGRAM_COUNTS_10000(
      "Memory.Experimental.CompressedPagesPerSecond",
      static_cast<double>(count) / interval.InSecondsF());
}

}  // namespace content
