// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/rappor/rappor_metric.h"

#include <stdlib.h>

#include "base/rand_util.h"
#include "base/strings/stringprintf.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace rappor {

const RapporParameters kTestRapporParameters = {
    1 /* Num cohorts */,
    16 /* Bloom filter size bytes */,
    4 /* Bloom filter hash count */,
    NORMAL_NOISE /* Noise level */,
    UMA_RAPPOR_GROUP /* Recording group (not used) */,
};

// Check for basic syntax and use.
TEST(RapporMetricTest, BasicMetric) {
  RapporMetric testMetric("MyRappor", kTestRapporParameters, 0);
  testMetric.AddSample("Bar");
  EXPECT_EQ(0x80, testMetric.bytes()[1]);
}

TEST(RapporMetricTest, GetReport) {
  RapporMetric metric("MyRappor", kTestRapporParameters, 0);

  const ByteVector report = metric.GetReport(
      HmacByteVectorGenerator::GenerateEntropyInput());
  EXPECT_EQ(16u, report.size());
}

}  // namespace rappor
