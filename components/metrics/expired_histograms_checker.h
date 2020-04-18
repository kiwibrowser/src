// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_METRICS_EXPIRED_HISTOGRAMS_CHECKER_H_
#define COMPONENTS_METRICS_EXPIRED_HISTOGRAMS_CHECKER_H_

#include <stdint.h>
#include <set>

#include "base/macros.h"
#include "base/metrics/record_histogram_checker.h"
#include "base/strings/string_piece.h"

namespace metrics {

// ExpiredHistogramsChecker implements RecordHistogramChecker interface
// to avoid recording expired metrics.
class ExpiredHistogramsChecker final : public base::RecordHistogramChecker {
 public:
  // Takes sorted in nondecreasing order array of histogram hashes, its size and
  // list of whitelisted histogram names concatenated as a comma-separated
  // string.
  ExpiredHistogramsChecker(const uint64_t* array,
                           size_t size,
                           const std::string& whitelist_str);
  ~ExpiredHistogramsChecker() override;

  // Checks if the given |histogram_hash| corresponds to an expired histogram.
  bool ShouldRecord(uint64_t histogram_hash) const override;

 private:
  // Initializes the |whitelist_| array of histogram hashes that should be
  // recorded regardless of their expiration.
  void InitWhitelist(const std::string& whitelist_str);

  // Array of expired histogram hashes.
  const uint64_t* const array_;

  // Size of the |array_|.
  const size_t size_;

  // List of expired histogram hashes that should be recorded.
  std::set<uint64_t> whitelist_;

  DISALLOW_COPY_AND_ASSIGN(ExpiredHistogramsChecker);
};

}  // namespace metrics

#endif  // COMPONENTS_METRICS_EXPIRED_HISTOGRAMS_CHECKER_H_
