// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/download/public/common/download_stats.h"

#include "base/test/metrics/histogram_tester.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace download {

namespace {

void VerfiyParallelizableAverageStats(int64_t bytes_downloaded,
                                      const base::TimeDelta& time_span) {
  base::HistogramTester histogram_tester;
  int64_t expected_bandwidth = bytes_downloaded / time_span.InSeconds();

  RecordParallelizableDownloadAverageStats(bytes_downloaded, time_span);
  histogram_tester.ExpectBucketCount("Download.ParallelizableDownloadBandwidth",
                                     expected_bandwidth, 1);
  histogram_tester.ExpectBucketCount("Download.Parallelizable.DownloadTime",
                                     time_span.InMilliseconds(), 1);
  histogram_tester.ExpectBucketCount("Download.Parallelizable.FileSize",
                                     bytes_downloaded / 1024, 1);
}

}  // namespace

TEST(DownloadStatsTest, ParallelizableAverageStats) {
  VerfiyParallelizableAverageStats(1, base::TimeDelta::FromSeconds(1));
  VerfiyParallelizableAverageStats(1024 * 1024 * 20,
                                   base::TimeDelta::FromSeconds(10));
  VerfiyParallelizableAverageStats(1024 * 1024 * 1024,
                                   base::TimeDelta::FromSeconds(1));
}

}  // namespace download
