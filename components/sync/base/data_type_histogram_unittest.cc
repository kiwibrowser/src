// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync/base/data_type_histogram.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace syncer {
namespace {

class DataTypeHistogramTest : public testing::Test {};

// Create a histogram of type LOCAL_HISTOGRAM_COUNTS for each model type.
// Nothing should break.
TEST(DataTypeHistogramTest, BasicCount) {
  for (int i = FIRST_REAL_MODEL_TYPE; i <= LAST_REAL_MODEL_TYPE; ++i) {
    ModelType type = ModelTypeFromInt(i);
#define PER_DATA_TYPE_MACRO(type_str) \
  LOCAL_HISTOGRAM_COUNTS("BasicCountPrefix" type_str "Suffix", 1);
    SYNC_DATA_TYPE_HISTOGRAM(type);
#undef PER_DATA_TYPE_MACRO
  }
}

// Create a histogram of type UMA_HISTOGRAM_ENUMERATION for each model type.
// Nothing should break.
TEST(DataTypeHistogramTest, BasicEnum) {
  enum HistTypes {
    TYPE_1,
    TYPE_2,
    TYPE_COUNT,
  };
  for (int i = FIRST_REAL_MODEL_TYPE; i <= LAST_REAL_MODEL_TYPE; ++i) {
    ModelType type = ModelTypeFromInt(i);
#define PER_DATA_TYPE_MACRO(type_str)                            \
  UMA_HISTOGRAM_ENUMERATION("BasicEnumPrefix" type_str "Suffix", \
                            (i % 2 ? TYPE_1 : TYPE_2), TYPE_COUNT);
    SYNC_DATA_TYPE_HISTOGRAM(type);
#undef PER_DATA_TYPE_MACRO
  }
}

}  // namespace
}  // namespace syncer
