// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/scheduler/util/task_duration_metric_reporter.h"

#include "base/metrics/histogram_base.h"
#include "base/metrics/histogram_samples.h"
#include "base/values.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace blink {
namespace scheduler {

namespace {

using testing::_;
using testing::Mock;

class FakeHistogram : public base::HistogramBase {
 public:
  FakeHistogram() : base::HistogramBase("fake") {}
  ~FakeHistogram() override = default;

  MOCK_METHOD2(AddCount, void(base::HistogramBase::Sample, int));
  MOCK_CONST_METHOD0(name_hash, uint64_t());
  MOCK_CONST_METHOD0(GetHistogramType, base::HistogramType());
  MOCK_CONST_METHOD3(HasConstructionArguments,
                     bool(base::HistogramBase::Sample,
                          base::HistogramBase::Sample,
                          uint32_t));
  MOCK_METHOD1(Add, void(base::HistogramBase::Sample));
  MOCK_METHOD1(AddSamples, void(const base::HistogramSamples&));
  MOCK_CONST_METHOD0(SnapshotSamples,
                     std::unique_ptr<base::HistogramSamples>());
  MOCK_METHOD0(SnapshotDelta, std::unique_ptr<base::HistogramSamples>());
  MOCK_CONST_METHOD0(SnapshotFinalDelta,
                     std::unique_ptr<base::HistogramSamples>());
  MOCK_METHOD1(AddSamplesFromPickle, bool(base::PickleIterator*));
  MOCK_CONST_METHOD1(WriteHTMLGraph, void(std::string*));
  MOCK_CONST_METHOD1(WriteAscii, void(std::string*));
  MOCK_CONST_METHOD1(SerializeInfoImpl, void(base::Pickle*));
  MOCK_CONST_METHOD1(GetParameters, void(base::DictionaryValue*));
  MOCK_CONST_METHOD3(GetCountAndBucketData,
                     void(base::HistogramBase::Count*,
                          int64_t*,
                          base::ListValue*));
};

enum class FakeTaskQueueType {
  kFakeType0 = 0,
  kFakeType1 = 1,
  kFakeType2 = 2,
  kCount = 3
};

}  // namespace

TEST(TaskDurationMetricReporterTest, Test) {
  FakeHistogram histogram;

  TaskDurationMetricReporter<FakeTaskQueueType> metric_reporter(&histogram);

  EXPECT_CALL(histogram, AddCount(2, 3));
  metric_reporter.RecordTask(static_cast<FakeTaskQueueType>(2),
                             base::TimeDelta::FromMicroseconds(3400));
  Mock::VerifyAndClearExpectations(&histogram);

  EXPECT_CALL(histogram, AddCount(_, _)).Times(0);
  metric_reporter.RecordTask(static_cast<FakeTaskQueueType>(2),
                             base::TimeDelta::FromMicroseconds(300));
  Mock::VerifyAndClearExpectations(&histogram);

  EXPECT_CALL(histogram, AddCount(2, 1));
  metric_reporter.RecordTask(static_cast<FakeTaskQueueType>(2),
                             base::TimeDelta::FromMicroseconds(800));
  Mock::VerifyAndClearExpectations(&histogram);

  EXPECT_CALL(histogram, AddCount(2, 16));
  metric_reporter.RecordTask(static_cast<FakeTaskQueueType>(2),
                             base::TimeDelta::FromMicroseconds(15600));
  Mock::VerifyAndClearExpectations(&histogram);
}

}  // namespace scheduler
}  // namespace blink
