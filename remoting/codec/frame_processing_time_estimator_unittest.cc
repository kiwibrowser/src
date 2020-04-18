// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/base/constants.h"
#include "remoting/codec/frame_processing_time_estimator.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace remoting {

namespace {

class TestFrameProcessingTimeEstimator : public FrameProcessingTimeEstimator {
 public:
  TestFrameProcessingTimeEstimator() = default;
  ~TestFrameProcessingTimeEstimator() override = default;

  void TimeElapseMs(int delta) {
    now_ += base::TimeDelta::FromMilliseconds(delta);
  }

 private:
  base::TimeTicks Now() const override { return now_; }

  base::TimeTicks now_ = base::TimeTicks::Now();
};

WebrtcVideoEncoder::EncodedFrame CreateEncodedFrame(bool key_frame, int size) {
  WebrtcVideoEncoder::EncodedFrame result;
  result.key_frame = key_frame;
  result.data.assign(static_cast<size_t>(size), 'A');
  return result;
}

}  // namespace

TEST(FrameProcessingTimeEstimatorTest, EstimateDeltaAndKeyFrame) {
  TestFrameProcessingTimeEstimator estimator;
  for (int i = 0; i < 10; i++) {
    estimator.StartFrame();
    if (i % 5 == 0) {
      // Fake a key-frame.
      estimator.TimeElapseMs(10);
      estimator.FinishFrame(CreateEncodedFrame(true, 100));
    } else {
      // Fake a delta-frame.
      estimator.TimeElapseMs(1);
      estimator.FinishFrame(CreateEncodedFrame(false, 50));
    }
  }

  estimator.SetBandwidthKbps(50);
  estimator.SetBandwidthKbps(150);

  EXPECT_EQ(100, estimator.AverageBandwidthKbps());

  EXPECT_EQ(base::TimeDelta::FromMilliseconds(10),
            estimator.EstimatedProcessingTime(true));
  EXPECT_EQ(base::TimeDelta::FromMilliseconds(1),
            estimator.EstimatedProcessingTime(false));
  EXPECT_EQ(base::TimeDelta::FromMilliseconds(8),
            estimator.EstimatedTransitTime(true));
  EXPECT_EQ(base::TimeDelta::FromMilliseconds(4),
            estimator.EstimatedTransitTime(false));

  EXPECT_EQ(60, estimator.EstimatedFrameSize());

  EXPECT_EQ(base::TimeDelta::FromMicroseconds(2800),
            estimator.EstimatedProcessingTime());
  EXPECT_EQ(base::TimeDelta::FromMicroseconds(4800),
            estimator.EstimatedTransitTime());

  EXPECT_EQ(kTargetFrameRate, estimator.RecentFrameRate());
  EXPECT_EQ(kTargetFrameRate, estimator.PredictedFrameRate());
  EXPECT_EQ(kTargetFrameRate, estimator.EstimatedFrameRate());
}

TEST(FrameProcessingTimeEstimatorTest, NegativeBandwidthShouldBeDropped) {
  TestFrameProcessingTimeEstimator estimator;
  estimator.StartFrame();
  estimator.TimeElapseMs(10);
  estimator.FinishFrame(CreateEncodedFrame(true, 100));
  estimator.SetBandwidthKbps(100);
  estimator.SetBandwidthKbps(-100);
  EXPECT_EQ(base::TimeDelta::FromMilliseconds(8),
            estimator.EstimatedTransitTime(true));
}

TEST(FrameProcessingTimeEstimatorTest, ShouldNotReturn0WithOnlyKeyFrames) {
  TestFrameProcessingTimeEstimator estimator;
  estimator.StartFrame();
  estimator.TimeElapseMs(10);
  estimator.FinishFrame(CreateEncodedFrame(true, 100));
  estimator.SetBandwidthKbps(100);
  EXPECT_EQ(base::TimeDelta::FromMilliseconds(10),
            estimator.EstimatedProcessingTime(true));
  EXPECT_EQ(base::TimeDelta::FromMilliseconds(8),
            estimator.EstimatedTransitTime(true));
  EXPECT_EQ(base::TimeDelta::FromMilliseconds(10),
            estimator.EstimatedProcessingTime(false));
  EXPECT_EQ(base::TimeDelta::FromMilliseconds(8),
            estimator.EstimatedTransitTime(false));

  EXPECT_EQ(base::TimeDelta::FromMilliseconds(10),
            estimator.EstimatedProcessingTime());
  EXPECT_EQ(base::TimeDelta::FromMilliseconds(8),
            estimator.EstimatedTransitTime());
  EXPECT_EQ(100, estimator.EstimatedFrameSize());
  EXPECT_EQ(kTargetFrameRate, estimator.RecentFrameRate());
  EXPECT_EQ(kTargetFrameRate, estimator.PredictedFrameRate());
  EXPECT_EQ(kTargetFrameRate, estimator.EstimatedFrameRate());
}

TEST(FrameProcessingTimeEstimatorTest, ShouldNotReturn0WithOnlyDeltaFrames) {
  TestFrameProcessingTimeEstimator estimator;
  estimator.StartFrame();
  estimator.TimeElapseMs(1);
  estimator.FinishFrame(CreateEncodedFrame(false, 50));
  estimator.SetBandwidthKbps(100);
  EXPECT_EQ(base::TimeDelta::FromMilliseconds(1),
            estimator.EstimatedProcessingTime(false));
  EXPECT_EQ(base::TimeDelta::FromMilliseconds(4),
            estimator.EstimatedTransitTime(false));
  EXPECT_EQ(base::TimeDelta::FromMilliseconds(1),
            estimator.EstimatedProcessingTime(true));
  EXPECT_EQ(base::TimeDelta::FromMilliseconds(4),
            estimator.EstimatedTransitTime(true));

  EXPECT_EQ(base::TimeDelta::FromMilliseconds(1),
            estimator.EstimatedProcessingTime());
  EXPECT_EQ(base::TimeDelta::FromMilliseconds(4),
            estimator.EstimatedTransitTime());
  EXPECT_EQ(50, estimator.EstimatedFrameSize());
  EXPECT_EQ(kTargetFrameRate, estimator.RecentFrameRate());
  EXPECT_EQ(kTargetFrameRate, estimator.PredictedFrameRate());
  EXPECT_EQ(kTargetFrameRate, estimator.EstimatedFrameRate());
}

TEST(FrameProcessingTimeEstimatorTest, ShouldReturnDefaultValueWithoutRecords) {
  TestFrameProcessingTimeEstimator estimator;
  EXPECT_EQ(base::TimeDelta(), estimator.EstimatedProcessingTime(true));
  EXPECT_EQ(base::TimeDelta(), estimator.EstimatedProcessingTime(false));
  EXPECT_EQ(base::TimeDelta(), estimator.EstimatedProcessingTime());
  EXPECT_EQ(0, estimator.EstimatedFrameSize());
  EXPECT_EQ(kTargetFrameRate, estimator.RecentFrameRate());
  EXPECT_EQ(1, estimator.PredictedFrameRate());
  EXPECT_EQ(1, estimator.EstimatedFrameRate());

  for (int i = 0; i < 10; i++) {
    estimator.StartFrame();
    if (i % 5 == 0) {
      // Fake a key-frame.
      estimator.TimeElapseMs(100);
      estimator.FinishFrame(CreateEncodedFrame(true, 100));
    } else {
      // Fake a delta-frame.
      estimator.TimeElapseMs(50);
      estimator.FinishFrame(CreateEncodedFrame(false, 50));
    }
  }
  EXPECT_EQ(base::TimeDelta::FromMillisecondsD(static_cast<double>(500) / 9),
            estimator.RecentAverageFrameInterval());
  EXPECT_EQ(19, estimator.RecentFrameRate());
  EXPECT_EQ(1, estimator.PredictedFrameRate());
  EXPECT_EQ(1, estimator.EstimatedFrameRate());
}

TEST(FrameProcessingTimeEstimatorTest,
     ShouldEstimateFrameRateFromProcessingTime) {
  // Processing times are much longer than transit times, so the estimation of
  // the frame rate should depend on the processing time.
  TestFrameProcessingTimeEstimator estimator;
  for (int i = 0; i < 10; i++) {
    estimator.StartFrame();
    if (i % 5 == 0) {
      // Fake a key-frame.
      estimator.TimeElapseMs(100);
      estimator.FinishFrame(CreateEncodedFrame(true, 100));
    } else {
      // Fake a delta-frame.
      estimator.TimeElapseMs(50);
      estimator.FinishFrame(CreateEncodedFrame(false, 50));
    }
  }
  estimator.SetBandwidthKbps(100);

  EXPECT_EQ(base::TimeDelta::FromMilliseconds(60),
            estimator.EstimatedProcessingTime());
  EXPECT_EQ(base::TimeDelta::FromMillisecondsD(static_cast<double>(500) / 9),
            estimator.RecentAverageFrameInterval());
  EXPECT_EQ(19, estimator.RecentFrameRate());
  EXPECT_EQ(17, estimator.PredictedFrameRate());
  EXPECT_EQ(17, estimator.EstimatedFrameRate());
}

TEST(FrameProcessingTimeEstimatorTest,
     ShouldEstimateFrameRateFromTransitTime) {
  // Transit times are much longer than processing times, so the estimation of
  // the frame rate should depend on the transit time.
  TestFrameProcessingTimeEstimator estimator;
  for (int i = 0; i < 10; i++) {
    estimator.StartFrame();
    if (i % 5 == 0) {
      // Fake a key-frame.
      estimator.TimeElapseMs(10);
      estimator.FinishFrame(CreateEncodedFrame(true, 100));
    } else {
      // Fake a delta-frame.
      estimator.TimeElapseMs(1);
      estimator.FinishFrame(CreateEncodedFrame(false, 50));
    }
  }
  estimator.SetBandwidthKbps(10);

  EXPECT_EQ(base::TimeDelta::FromMilliseconds(48),
            estimator.EstimatedTransitTime());
  EXPECT_EQ(kTargetFrameRate, estimator.RecentFrameRate());
  EXPECT_EQ(21, estimator.PredictedFrameRate());
  EXPECT_EQ(21, estimator.EstimatedFrameRate());
}

TEST(FrameProcessingTimeEstimatorTest,
     ShouldNotReturnNegativeEstimatedFrameRate) {
  // Both processing times and transit times are extremely long, estimator
  // should return 1.
  TestFrameProcessingTimeEstimator estimator;
  for (int i = 0; i < 10; i++) {
    estimator.StartFrame();
    if (i % 5 == 0) {
      // Fake a key-frame.
      estimator.TimeElapseMs(10000);
      estimator.FinishFrame(CreateEncodedFrame(true, 1000));
    } else {
      // Fake a delta-frame.
      estimator.TimeElapseMs(5000);
      estimator.FinishFrame(CreateEncodedFrame(false, 500));
    }
  }
  estimator.SetBandwidthKbps(1);

  EXPECT_EQ(base::TimeDelta::FromMilliseconds(6000),
            estimator.EstimatedProcessingTime());
  EXPECT_EQ(base::TimeDelta::FromMilliseconds(4800),
            estimator.EstimatedTransitTime());
  EXPECT_EQ(1, estimator.RecentFrameRate());
  EXPECT_EQ(1, estimator.PredictedFrameRate());
  EXPECT_EQ(1, estimator.EstimatedFrameRate());
}

TEST(FrameProcessingTimeEstimatorTest,
     RecentAverageFrameIntervalShouldConsiderDelay) {
  TestFrameProcessingTimeEstimator estimator;
  for (int i = 0; i < 10; i++) {
    estimator.TimeElapseMs(50);
    estimator.StartFrame();
    if (i % 5 == 0) {
      // Fake a key-frame.
      estimator.TimeElapseMs(10);
      estimator.FinishFrame(CreateEncodedFrame(true, 1000));
    } else {
      // Fake a delta-frame.
      estimator.TimeElapseMs(5);
      estimator.FinishFrame(CreateEncodedFrame(false, 500));
    }
  }
  estimator.SetBandwidthKbps(1000);
  EXPECT_EQ(base::TimeDelta::FromMilliseconds(6),
            estimator.EstimatedProcessingTime());
  EXPECT_EQ(base::TimeDelta::FromMillisecondsD(
                static_cast<double>(50 * 9 + 10 + 5 * 8) / 9),
            estimator.RecentAverageFrameInterval());
  EXPECT_EQ(19, estimator.RecentFrameRate());
  // Processing time & transit time are both pretty low, we should be able to
  // reach 30 FPS if capturing delay has been reduced.
  EXPECT_EQ(kTargetFrameRate, estimator.PredictedFrameRate());
  EXPECT_EQ(19, estimator.EstimatedFrameRate());
}

}  // namespace remoting
