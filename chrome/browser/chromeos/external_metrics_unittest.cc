// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/external_metrics.h"

#include <memory>

#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/metrics/statistics_recorder.h"
#include "base/test/histogram_tester.h"
#include "components/metrics/serialization/metric_sample.h"
#include "components/metrics/serialization/serialization_utils.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace chromeos {  // Need this because of the FRIEND_TEST

class ExternalMetricsTest : public testing::Test {
 public:
  void SetUp() override {
    ASSERT_TRUE(dir_.CreateUniqueTempDir());
    external_metrics_ = ExternalMetrics::CreateForTesting(
        dir_.GetPath().Append("testfile").value());
  }

  base::ScopedTempDir dir_;
  scoped_refptr<ExternalMetrics> external_metrics_;
  content::TestBrowserThreadBundle thread_bundle_;
};

TEST_F(ExternalMetricsTest, HandleMissingFile) {
  ASSERT_TRUE(base::DeleteFile(
      base::FilePath(external_metrics_->uma_events_file_), false));

  EXPECT_EQ(0, external_metrics_->CollectEvents());
}

TEST_F(ExternalMetricsTest, CanReceiveHistogram) {
  base::HistogramTester histogram_tester;

  std::unique_ptr<metrics::MetricSample> hist =
      metrics::MetricSample::HistogramSample("foo", 2, 1, 100, 10);

  EXPECT_TRUE(metrics::SerializationUtils::WriteMetricToFile(
      *hist.get(), external_metrics_->uma_events_file_));

  EXPECT_EQ(1, external_metrics_->CollectEvents());

  histogram_tester.ExpectTotalCount("foo", 1);
}

TEST_F(ExternalMetricsTest, IncorrectHistogramsAreDiscarded) {
  base::HistogramTester histogram_tester;

  // Malformed histogram (min > max).
  std::unique_ptr<metrics::MetricSample> hist =
      metrics::MetricSample::HistogramSample("bar", 30, 200, 20, 10);

  EXPECT_TRUE(metrics::SerializationUtils::WriteMetricToFile(
      *hist.get(), external_metrics_->uma_events_file_));

  external_metrics_->CollectEvents();

  histogram_tester.ExpectTotalCount("bar", 0);
}

}  // namespace chromeos
