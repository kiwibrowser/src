// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/metrics/public/cpp/ukm_source_id.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace ukm {

TEST(UkmSourceIdTest, AssignSourceIds) {
  const size_t numIds = 5;
  SourceId ids[numIds] = {};

  for (size_t i = 0; i < numIds; i++) {
    ids[i] = AssignNewSourceId();
    EXPECT_NE(kInvalidSourceId, ids[i]);
    EXPECT_EQ(SourceIdType::UKM, GetSourceIdType(ids[i]));
    for (size_t j = 0; j < i; j++) {
      EXPECT_NE(ids[j], ids[i]);
    }
  }
}

TEST(UkmSourceIdTest, ConvertToSourceId) {
  const size_t numIds = 5;
  SourceId ids[numIds] = {};

  for (size_t i = 0; i < numIds; i++) {
    ids[i] = ConvertToSourceId(i, SourceIdType::NAVIGATION_ID);
    EXPECT_NE(kInvalidSourceId, ids[i]);
    EXPECT_EQ(SourceIdType::NAVIGATION_ID, GetSourceIdType(ids[i]));
    for (size_t j = 0; j < i; j++) {
      EXPECT_NE(ids[j], ids[i]);
    }
  }
}

}  // namespace ukm
