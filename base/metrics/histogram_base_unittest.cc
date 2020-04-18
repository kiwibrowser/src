// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <vector>

#include "base/metrics/histogram.h"
#include "base/metrics/histogram_base.h"
#include "base/metrics/sample_vector.h"
#include "base/metrics/sparse_histogram.h"
#include "base/metrics/statistics_recorder.h"
#include "base/pickle.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace base {

class HistogramBaseTest : public testing::Test {
 protected:
  HistogramBaseTest() {
    // Each test will have a clean state (no Histogram / BucketRanges
    // registered).
    ResetStatisticsRecorder();
  }

  ~HistogramBaseTest() override = default;

  void ResetStatisticsRecorder() {
    // It is necessary to fully destruct any existing StatisticsRecorder
    // before creating a new one.
    statistics_recorder_.reset();
    statistics_recorder_ = StatisticsRecorder::CreateTemporaryForTesting();
  }

 private:
  std::unique_ptr<StatisticsRecorder> statistics_recorder_;

  DISALLOW_COPY_AND_ASSIGN(HistogramBaseTest);
};

TEST_F(HistogramBaseTest, DeserializeHistogram) {
  HistogramBase* histogram = Histogram::FactoryGet(
      "TestHistogram", 1, 1000, 10,
      (HistogramBase::kUmaTargetedHistogramFlag |
      HistogramBase::kIPCSerializationSourceFlag));

  Pickle pickle;
  histogram->SerializeInfo(&pickle);

  PickleIterator iter(pickle);
  HistogramBase* deserialized = DeserializeHistogramInfo(&iter);
  EXPECT_EQ(histogram, deserialized);

  ResetStatisticsRecorder();

  PickleIterator iter2(pickle);
  deserialized = DeserializeHistogramInfo(&iter2);
  EXPECT_TRUE(deserialized);
  EXPECT_NE(histogram, deserialized);
  EXPECT_EQ("TestHistogram", StringPiece(deserialized->histogram_name()));
  EXPECT_TRUE(deserialized->HasConstructionArguments(1, 1000, 10));

  // kIPCSerializationSourceFlag will be cleared.
  EXPECT_EQ(HistogramBase::kUmaTargetedHistogramFlag, deserialized->flags());
}

TEST_F(HistogramBaseTest, DeserializeLinearHistogram) {
  HistogramBase* histogram = LinearHistogram::FactoryGet(
      "TestHistogram", 1, 1000, 10,
      HistogramBase::kIPCSerializationSourceFlag);

  Pickle pickle;
  histogram->SerializeInfo(&pickle);

  PickleIterator iter(pickle);
  HistogramBase* deserialized = DeserializeHistogramInfo(&iter);
  EXPECT_EQ(histogram, deserialized);

  ResetStatisticsRecorder();

  PickleIterator iter2(pickle);
  deserialized = DeserializeHistogramInfo(&iter2);
  EXPECT_TRUE(deserialized);
  EXPECT_NE(histogram, deserialized);
  EXPECT_EQ("TestHistogram", StringPiece(deserialized->histogram_name()));
  EXPECT_TRUE(deserialized->HasConstructionArguments(1, 1000, 10));
  EXPECT_EQ(0, deserialized->flags());
}

TEST_F(HistogramBaseTest, DeserializeBooleanHistogram) {
  HistogramBase* histogram = BooleanHistogram::FactoryGet(
      "TestHistogram", HistogramBase::kIPCSerializationSourceFlag);

  Pickle pickle;
  histogram->SerializeInfo(&pickle);

  PickleIterator iter(pickle);
  HistogramBase* deserialized = DeserializeHistogramInfo(&iter);
  EXPECT_EQ(histogram, deserialized);

  ResetStatisticsRecorder();

  PickleIterator iter2(pickle);
  deserialized = DeserializeHistogramInfo(&iter2);
  EXPECT_TRUE(deserialized);
  EXPECT_NE(histogram, deserialized);
  EXPECT_EQ("TestHistogram", StringPiece(deserialized->histogram_name()));
  EXPECT_TRUE(deserialized->HasConstructionArguments(1, 2, 3));
  EXPECT_EQ(0, deserialized->flags());
}

TEST_F(HistogramBaseTest, DeserializeCustomHistogram) {
  std::vector<HistogramBase::Sample> ranges;
  ranges.push_back(13);
  ranges.push_back(5);
  ranges.push_back(9);

  HistogramBase* histogram = CustomHistogram::FactoryGet(
      "TestHistogram", ranges, HistogramBase::kIPCSerializationSourceFlag);

  Pickle pickle;
  histogram->SerializeInfo(&pickle);

  PickleIterator iter(pickle);
  HistogramBase* deserialized = DeserializeHistogramInfo(&iter);
  EXPECT_EQ(histogram, deserialized);

  ResetStatisticsRecorder();

  PickleIterator iter2(pickle);
  deserialized = DeserializeHistogramInfo(&iter2);
  EXPECT_TRUE(deserialized);
  EXPECT_NE(histogram, deserialized);
  EXPECT_EQ("TestHistogram", StringPiece(deserialized->histogram_name()));
  EXPECT_TRUE(deserialized->HasConstructionArguments(5, 13, 4));
  EXPECT_EQ(0, deserialized->flags());
}

TEST_F(HistogramBaseTest, DeserializeSparseHistogram) {
  HistogramBase* histogram = SparseHistogram::FactoryGet(
      "TestHistogram", HistogramBase::kIPCSerializationSourceFlag);

  Pickle pickle;
  histogram->SerializeInfo(&pickle);

  PickleIterator iter(pickle);
  HistogramBase* deserialized = DeserializeHistogramInfo(&iter);
  EXPECT_EQ(histogram, deserialized);

  ResetStatisticsRecorder();

  PickleIterator iter2(pickle);
  deserialized = DeserializeHistogramInfo(&iter2);
  EXPECT_TRUE(deserialized);
  EXPECT_NE(histogram, deserialized);
  EXPECT_EQ("TestHistogram", StringPiece(deserialized->histogram_name()));
  EXPECT_EQ(0, deserialized->flags());
}

TEST_F(HistogramBaseTest, AddKilo) {
  HistogramBase* histogram =
      LinearHistogram::FactoryGet("TestAddKiloHistogram", 1, 1000, 100, 0);

  histogram->AddKilo(100, 1000);
  histogram->AddKilo(200, 2000);
  histogram->AddKilo(300, 1500);

  std::unique_ptr<HistogramSamples> samples = histogram->SnapshotSamples();
  EXPECT_EQ(1, samples->GetCount(100));
  EXPECT_EQ(2, samples->GetCount(200));
  EXPECT_LE(1, samples->GetCount(300));
  EXPECT_GE(2, samples->GetCount(300));
}

TEST_F(HistogramBaseTest, AddKiB) {
  HistogramBase* histogram =
      LinearHistogram::FactoryGet("TestAddKiBHistogram", 1, 1000, 100, 0);

  histogram->AddKiB(100, 1024);
  histogram->AddKiB(200, 2048);
  histogram->AddKiB(300, 1536);

  std::unique_ptr<HistogramSamples> samples = histogram->SnapshotSamples();
  EXPECT_EQ(1, samples->GetCount(100));
  EXPECT_EQ(2, samples->GetCount(200));
  EXPECT_LE(1, samples->GetCount(300));
  EXPECT_GE(2, samples->GetCount(300));
}

}  // namespace base
