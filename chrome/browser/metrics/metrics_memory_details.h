// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_METRICS_METRICS_MEMORY_DETAILS_H_
#define CHROME_BROWSER_METRICS_METRICS_MEMORY_DETAILS_H_

#include <map>

#include "base/callback.h"
#include "base/macros.h"
#include "build/build_config.h"
#include "chrome/browser/memory_details.h"

// Handles asynchronous fetching of memory details and logging histograms about
// memory use of various processes.
// Will run the provided callback when finished.
class MetricsMemoryDetails : public MemoryDetails {
 public:
  explicit MetricsMemoryDetails(const base::Closure& callback);

  void set_generate_histograms(bool generate_histograms) {
    generate_histograms_ = generate_histograms;
  }

 protected:
  ~MetricsMemoryDetails() override;

  // MemoryDetails:
  void OnDetailsAvailable() override;

 private:
  // Updates the global histograms for tracking memory usage.
  void UpdateHistograms();

  base::Closure callback_;

  // A flag indicating if histogram data should be generated. True on default.
  // If false, then only MemoryGrowthTracker gets notified about memory usage.
  bool generate_histograms_;

  DISALLOW_COPY_AND_ASSIGN(MetricsMemoryDetails);
};

#endif  // CHROME_BROWSER_METRICS_METRICS_MEMORY_DETAILS_H_
