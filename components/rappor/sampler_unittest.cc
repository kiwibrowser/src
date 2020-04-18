// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/rappor/sampler.h"

#include <memory>
#include <utility>

#include "base/metrics/metrics_hashes.h"
#include "components/rappor/byte_vector_utils.h"
#include "components/rappor/proto/rappor_metric.pb.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace rappor {

const RapporParameters kTestRapporParameters = {
    1 /* Num cohorts */,
    1 /* Bloom filter size bytes */,
    4 /* Bloom filter hash count */,
    NORMAL_NOISE /* Noise level */,
    UMA_RAPPOR_GROUP /* Recording group (not used) */};

class TestSamplerFactory {
 public:
  static std::unique_ptr<Sample> CreateSample() {
    return std::unique_ptr<Sample>(new Sample(0, kTestRapporParameters));
  }
};

namespace internal {

// Test that exporting deletes samples.
TEST(RapporSamplerTest, TestExport) {
  Sampler sampler;

  std::unique_ptr<Sample> sample1 = TestSamplerFactory::CreateSample();
  sample1->SetStringField("Foo", "Junk");
  sampler.AddSample("Metric1", std::move(sample1));

  std::unique_ptr<Sample> sample2 = TestSamplerFactory::CreateSample();
  sample2->SetStringField("Foo", "Junk2");
  sampler.AddSample("Metric1", std::move(sample2));

  // Since the two samples were for one metric, we should randomly get one
  // of the two.
  RapporReports reports;
  std::string secret = HmacByteVectorGenerator::GenerateEntropyInput();
  sampler.ExportMetrics(secret, &reports);
  EXPECT_EQ(1, reports.report_size());
  EXPECT_EQ(1u, reports.report(0).bits().size());

  // First export should clear the metric.
  RapporReports reports2;
  sampler.ExportMetrics(secret, &reports2);
  EXPECT_EQ(0, reports2.report_size());
}

// Test exporting fields with NO_NOISE.
TEST(RapporSamplerTest, TestNoNoise) {
  Sampler sampler;

  std::unique_ptr<Sample> sample1 = TestSamplerFactory::CreateSample();
  sample1->SetFlagsField("Foo", 0xde, 8, NO_NOISE);
  sample1->SetUInt64Field("Bar", 0x0011223344aabbccdd, NO_NOISE);
  sampler.AddSample("Metric1", std::move(sample1));

  RapporReports reports;
  std::string secret = HmacByteVectorGenerator::GenerateEntropyInput();
  sampler.ExportMetrics(secret, &reports);
  EXPECT_EQ(2, reports.report_size());

  uint64_t hash1 = base::HashMetricName("Metric1.Foo");
  bool order = reports.report(0).name_hash() == hash1;
  const RapporReports::Report& report1 = reports.report(order ? 0 : 1);
  EXPECT_EQ("\xde", report1.bits());
  const RapporReports::Report& report2 = reports.report(order ? 1 : 0);
  EXPECT_EQ("\xdd\xcc\xbb\xaa\x44\x33\x22\x11\x00", report2.bits());
}

}  // namespace internal

}  // namespace rappor
