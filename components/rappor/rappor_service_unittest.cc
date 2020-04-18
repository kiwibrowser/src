// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/rappor/rappor_service_impl.h"

#include <stddef.h>
#include <stdint.h>

#include <memory>
#include <utility>

#include "base/base64.h"
#include "base/metrics/metrics_hashes.h"
#include "components/prefs/testing_pref_service.h"
#include "components/rappor/byte_vector_utils.h"
#include "components/rappor/proto/rappor_metric.pb.h"
#include "components/rappor/public/rappor_parameters.h"
#include "components/rappor/rappor_pref_names.h"
#include "components/rappor/test_log_uploader.h"
#include "components/rappor/test_rappor_service.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace rappor {

TEST(RapporServiceImplTest, Update) {
  // Test rappor service initially has uploading and reporting enabled.
  TestRapporServiceImpl rappor_service;
  EXPECT_LT(base::TimeDelta(), rappor_service.next_rotation());
  EXPECT_TRUE(rappor_service.test_uploader()->is_running());

  // Disabling both should stop both uploads and reports.
  rappor_service.Update(false, false);
  EXPECT_EQ(base::TimeDelta(), rappor_service.next_rotation());
  EXPECT_FALSE(rappor_service.test_uploader()->is_running());

  // Recording, but no reporting.
  rappor_service.Update(true, false);
  // Reports generation should still be scheduled.
  EXPECT_LT(base::TimeDelta(), rappor_service.next_rotation());
  EXPECT_FALSE(rappor_service.test_uploader()->is_running());

  // Recording and reporting enabled.
  rappor_service.Update(true, true);
  EXPECT_LT(base::TimeDelta(), rappor_service.next_rotation());
  EXPECT_TRUE(rappor_service.test_uploader()->is_running());
}

// Check that samples can be recorded and exported.
TEST(RapporServiceImplTest, RecordAndExportMetrics) {
  TestRapporServiceImpl rappor_service;

  // Multiple samples for the same metric should only generate one report.
  rappor_service.RecordSampleString("MyMetric", ETLD_PLUS_ONE_RAPPOR_TYPE,
                                    "foo");
  rappor_service.RecordSampleString("MyMetric", ETLD_PLUS_ONE_RAPPOR_TYPE,
                                    "bar");

  RapporReports reports;
  rappor_service.GetReports(&reports);
  EXPECT_EQ(1, reports.report_size());

  const RapporReports::Report& report = reports.report(0);
  EXPECT_TRUE(report.name_hash());
  // ETLD_PLUS_ONE_RAPPOR_TYPE has 128 bits
  EXPECT_EQ(16u, report.bits().size());
}

// Check that reporting_enabled_ is respected.
TEST(RapporServiceImplTest, UmaRecordingGroup) {
  TestRapporServiceImpl rappor_service;
  rappor_service.Update(false, false);

  // Reporting disabled.
  rappor_service.RecordSampleString("UmaMetric", UMA_RAPPOR_TYPE, "foo");

  RapporReports reports;
  rappor_service.GetReports(&reports);
  EXPECT_EQ(0, reports.report_size());
}

// Check that GetRecordedSampleForMetric works as expected.
TEST(RapporServiceImplTest, GetRecordedSampleForMetric) {
  TestRapporServiceImpl rappor_service;

  // Multiple samples for the same metric; only the latest is remembered.
  rappor_service.RecordSampleString("MyMetric", ETLD_PLUS_ONE_RAPPOR_TYPE,
                                    "foo");
  rappor_service.RecordSampleString("MyMetric", ETLD_PLUS_ONE_RAPPOR_TYPE,
                                    "bar");

  std::string sample;
  RapporType type;
  EXPECT_FALSE(
      rappor_service.GetRecordedSampleForMetric("WrongMetric", &sample, &type));
  EXPECT_TRUE(
      rappor_service.GetRecordedSampleForMetric("MyMetric", &sample, &type));
  EXPECT_EQ("bar", sample);
  EXPECT_EQ(ETLD_PLUS_ONE_RAPPOR_TYPE, type);
}

// Check that the incognito is respected.
TEST(RapporServiceImplTest, Incognito) {
  TestRapporServiceImpl rappor_service;
  rappor_service.set_is_incognito(true);

  rappor_service.RecordSampleString("MyMetric", UMA_RAPPOR_TYPE, "foo");

  RapporReports reports;
  rappor_service.GetReports(&reports);
  EXPECT_EQ(0, reports.report_size());
}

// Check that Sample objects record correctly.
TEST(RapporServiceImplTest, RecordSample) {
  TestRapporServiceImpl rappor_service;
  std::unique_ptr<Sample> sample = rappor_service.CreateSample(UMA_RAPPOR_TYPE);
  sample->SetStringField("Url", "example.com");
  sample->SetFlagsField("Flags1", 0xbcd, 12);
  rappor_service.RecordSample("ObjMetric", std::move(sample));
  uint64_t url_hash = base::HashMetricName("ObjMetric.Url");
  uint64_t flags_hash = base::HashMetricName("ObjMetric.Flags1");
  RapporReports reports;
  rappor_service.GetReports(&reports);
  EXPECT_EQ(2, reports.report_size());
  size_t url_index = reports.report(0).name_hash() == url_hash ? 0 : 1;
  size_t flags_index = url_index == 0 ? 1 : 0;
  EXPECT_EQ(url_hash, reports.report(url_index).name_hash());
  EXPECT_EQ(4u, reports.report(url_index).bits().size());
  EXPECT_EQ(flags_hash, reports.report(flags_index).name_hash());
  EXPECT_EQ(2u, reports.report(flags_index).bits().size());
}

}  // namespace rappor
