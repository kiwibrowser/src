// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/filters/video_cadence_estimator.h"

#include <stddef.h>

#include <memory>

#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "base/strings/stringprintf.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace media {

// See VideoCadenceEstimator header for more details.
const int kMinimumAcceptableTimeBetweenGlitchesSecs = 8;

// Slows down the given |fps| according to NTSC field reduction standards; see
// http://en.wikipedia.org/wiki/Frame_rate#Digital_video_and_television
static double NTSC(double fps) {
  return fps / 1.001;
}

static base::TimeDelta Interval(double hertz) {
  return base::TimeDelta::FromSecondsD(1.0 / hertz);
}

std::vector<int> CreateCadenceFromString(const std::string& cadence) {
  CHECK_EQ('[', cadence.front());
  CHECK_EQ(']', cadence.back());

  std::vector<int> result;
  for (const std::string& token :
       base::SplitString(cadence.substr(1, cadence.length() - 2),
                         ":", base::TRIM_WHITESPACE, base::SPLIT_WANT_ALL)) {
    int cadence_value = 0;
    CHECK(base::StringToInt(token, &cadence_value)) << token;
    result.push_back(cadence_value);
  }

  return result;
}

static void VerifyCadenceVectorWithCustomDeviationAndDrift(
    VideoCadenceEstimator* estimator,
    double frame_hertz,
    double render_hertz,
    base::TimeDelta deviation,
    base::TimeDelta acceptable_drift,
    const std::string& expected_cadence) {
  SCOPED_TRACE(base::StringPrintf("Checking %.03f fps into %0.03f", frame_hertz,
                                  render_hertz));

  const std::vector<int> expected_cadence_vector =
      CreateCadenceFromString(expected_cadence);

  estimator->Reset();
  const bool cadence_changed = estimator->UpdateCadenceEstimate(
      Interval(render_hertz), Interval(frame_hertz), deviation,
      acceptable_drift);
  EXPECT_EQ(cadence_changed, estimator->has_cadence());
  EXPECT_EQ(expected_cadence_vector.empty(), !estimator->has_cadence());

  // Nothing further to test.
  if (expected_cadence_vector.empty() || !estimator->has_cadence())
    return;

  EXPECT_EQ(expected_cadence_vector.size(),
            estimator->cadence_size_for_testing());

  // Spot two cycles of the cadence.
  for (size_t i = 0; i < expected_cadence_vector.size() * 2; ++i) {
    ASSERT_EQ(expected_cadence_vector[i % expected_cadence_vector.size()],
              estimator->GetCadenceForFrame(i));
  }
}

static void VerifyCadenceVectorWithCustomDrift(
    VideoCadenceEstimator* estimator,
    double frame_hertz,
    double render_hertz,
    base::TimeDelta acceptable_drift,
    const std::string& expected_cadence) {
  VerifyCadenceVectorWithCustomDeviationAndDrift(
      estimator, frame_hertz, render_hertz, base::TimeDelta(), acceptable_drift,
      expected_cadence);
}

static void VerifyCadenceVectorWithCustomDeviation(
    VideoCadenceEstimator* estimator,
    double frame_hertz,
    double render_hertz,
    base::TimeDelta deviation,
    const std::string& expected_cadence) {
  const base::TimeDelta acceptable_drift =
      std::max(Interval(frame_hertz) / 2, Interval(render_hertz));
  VerifyCadenceVectorWithCustomDeviationAndDrift(
      estimator, frame_hertz, render_hertz, deviation, acceptable_drift,
      expected_cadence);
}

static void VerifyCadenceVector(VideoCadenceEstimator* estimator,
                                double frame_hertz,
                                double render_hertz,
                                const std::string& expected_cadence) {
  const base::TimeDelta acceptable_drift =
      std::max(Interval(frame_hertz) / 2, Interval(render_hertz));
  VerifyCadenceVectorWithCustomDeviationAndDrift(
      estimator, frame_hertz, render_hertz, base::TimeDelta(), acceptable_drift,
      expected_cadence);
}

// Spot check common display and frame rate pairs for correctness.
TEST(VideoCadenceEstimatorTest, CadenceCalculations) {
  VideoCadenceEstimator estimator(
      base::TimeDelta::FromSeconds(kMinimumAcceptableTimeBetweenGlitchesSecs));
  estimator.set_cadence_hysteresis_threshold_for_testing(base::TimeDelta());

  const std::string kEmptyCadence = "[]";
  VerifyCadenceVector(&estimator, 1, NTSC(60), "[60]");

  VerifyCadenceVector(&estimator, 24, 60, "[3:2]");
  VerifyCadenceVector(&estimator, NTSC(24), 60, "[3:2]");
  VerifyCadenceVector(&estimator, 24, NTSC(60), "[3:2]");

  VerifyCadenceVector(&estimator, 25, 60, "[2:3:2:3:2]");
  VerifyCadenceVector(&estimator, NTSC(25), 60, "[2:3:2:3:2]");
  VerifyCadenceVector(&estimator, 25, NTSC(60), "[2:3:2:3:2]");

  VerifyCadenceVector(&estimator, 30, 60, "[2]");
  VerifyCadenceVector(&estimator, NTSC(30), 60, "[2]");
  VerifyCadenceVector(&estimator, 29.5, 60, kEmptyCadence);

  VerifyCadenceVector(&estimator, 50, 60, "[1:1:2:1:1]");
  VerifyCadenceVector(&estimator, NTSC(50), 60, "[1:1:2:1:1]");
  VerifyCadenceVector(&estimator, 50, NTSC(60), "[1:1:2:1:1]");

  VerifyCadenceVector(&estimator, NTSC(60), 60, "[1]");
  VerifyCadenceVector(&estimator, 60, NTSC(60), "[1]");

  VerifyCadenceVector(&estimator, 120, 60, "[1:0]");
  VerifyCadenceVector(&estimator, NTSC(120), 60, "[1:0]");
  VerifyCadenceVector(&estimator, 120, NTSC(60), "[1:0]");

  // Test cases for cadence below 1.
  VerifyCadenceVector(&estimator, 120, 24, "[1:0:0:0:0]");
  VerifyCadenceVector(&estimator, 120, 48, "[1:0:0:1:0]");
  VerifyCadenceVector(&estimator, 120, 72, "[1:0:1:0:1]");
  VerifyCadenceVector(&estimator, 90, 60, "[1:0:1]");

  // 50Hz is common in the EU.
  VerifyCadenceVector(&estimator, NTSC(24), 50, kEmptyCadence);
  VerifyCadenceVector(&estimator, 24, 50, kEmptyCadence);

  VerifyCadenceVector(&estimator, NTSC(25), 50, "[2]");
  VerifyCadenceVector(&estimator, 25, 50, "[2]");

  VerifyCadenceVector(&estimator, NTSC(30), 50, "[2:1:2]");
  VerifyCadenceVector(&estimator, 30, 50, "[2:1:2]");

  VerifyCadenceVector(&estimator, NTSC(60), 50, kEmptyCadence);
  VerifyCadenceVector(&estimator, 60, 50, kEmptyCadence);

}

// Check the extreme case that max_acceptable_drift is larger than
// minimum_time_until_max_drift.
TEST(VideoCadenceEstimatorTest, CadenceCalculationWithLargeDrift) {
  VideoCadenceEstimator estimator(
      base::TimeDelta::FromSeconds(kMinimumAcceptableTimeBetweenGlitchesSecs));
  estimator.set_cadence_hysteresis_threshold_for_testing(base::TimeDelta());

  base::TimeDelta drift = base::TimeDelta::FromHours(1);
  VerifyCadenceVectorWithCustomDrift(&estimator, 1, NTSC(60), drift, "[60]");

  VerifyCadenceVectorWithCustomDrift(&estimator, 30, 60, drift, "[2]");
  VerifyCadenceVectorWithCustomDrift(&estimator, NTSC(30), 60, drift, "[2]");
  VerifyCadenceVectorWithCustomDrift(&estimator, 30, NTSC(60), drift, "[2]");

  VerifyCadenceVectorWithCustomDrift(&estimator, 25, 60, drift, "[2]");
  VerifyCadenceVectorWithCustomDrift(&estimator, NTSC(25), 60, drift, "[2]");
  VerifyCadenceVectorWithCustomDrift(&estimator, 25, NTSC(60), drift, "[2]");

  // Test cases for cadence below 1.
  VerifyCadenceVectorWithCustomDrift(&estimator, 120, 24, drift, "[1]");
  VerifyCadenceVectorWithCustomDrift(&estimator, 120, 48, drift, "[1]");
  VerifyCadenceVectorWithCustomDrift(&estimator, 120, 72, drift, "[1]");
  VerifyCadenceVectorWithCustomDrift(&estimator, 90, 60, drift, "[1]");
}

// Check the case that the estimator excludes variable FPS case from Cadence.
TEST(VideoCadenceEstimatorTest, CadenceCalculationWithLargeDeviation) {
  VideoCadenceEstimator estimator(
      base::TimeDelta::FromSeconds(kMinimumAcceptableTimeBetweenGlitchesSecs));
  estimator.set_cadence_hysteresis_threshold_for_testing(base::TimeDelta());

  const base::TimeDelta deviation = base::TimeDelta::FromMilliseconds(30);
  VerifyCadenceVectorWithCustomDeviation(&estimator, 1, 60, deviation, "[]");
  VerifyCadenceVectorWithCustomDeviation(&estimator, 30, 60, deviation, "[]");
  VerifyCadenceVectorWithCustomDeviation(&estimator, 25, 60, deviation, "[]");

  // Test cases for cadence with low refresh rate.
  VerifyCadenceVectorWithCustomDeviation(&estimator, 60, 12, deviation,
                                         "[1:0:0:0:0]");
}

TEST(VideoCadenceEstimatorTest, CadenceVariesWithAcceptableDrift) {
  VideoCadenceEstimator estimator(
      base::TimeDelta::FromSeconds(kMinimumAcceptableTimeBetweenGlitchesSecs));
  estimator.set_cadence_hysteresis_threshold_for_testing(base::TimeDelta());

  const base::TimeDelta render_interval = Interval(NTSC(60));
  const base::TimeDelta frame_interval = Interval(120);

  base::TimeDelta acceptable_drift = frame_interval / 2;
  EXPECT_FALSE(estimator.UpdateCadenceEstimate(
      render_interval, frame_interval, base::TimeDelta(), acceptable_drift));
  EXPECT_FALSE(estimator.has_cadence());

  // Increasing the acceptable drift should be result in more permissive
  // detection of cadence.
  acceptable_drift = render_interval;
  EXPECT_TRUE(estimator.UpdateCadenceEstimate(
      render_interval, frame_interval, base::TimeDelta(), acceptable_drift));
  EXPECT_TRUE(estimator.has_cadence());
  EXPECT_EQ("[1:0]", estimator.GetCadenceForTesting());
}

TEST(VideoCadenceEstimatorTest, CadenceVariesWithAcceptableGlitchTime) {
  std::unique_ptr<VideoCadenceEstimator> estimator(new VideoCadenceEstimator(
      base::TimeDelta::FromSeconds(kMinimumAcceptableTimeBetweenGlitchesSecs)));
  estimator->set_cadence_hysteresis_threshold_for_testing(base::TimeDelta());

  const base::TimeDelta render_interval = Interval(NTSC(60));
  const base::TimeDelta frame_interval = Interval(120);
  const base::TimeDelta acceptable_drift = frame_interval / 2;

  EXPECT_FALSE(estimator->UpdateCadenceEstimate(
      render_interval, frame_interval, base::TimeDelta(), acceptable_drift));
  EXPECT_FALSE(estimator->has_cadence());

  // Decreasing the acceptable glitch time should be result in more permissive
  // detection of cadence.
  estimator.reset(new VideoCadenceEstimator(base::TimeDelta::FromSeconds(
      kMinimumAcceptableTimeBetweenGlitchesSecs / 2)));
  estimator->set_cadence_hysteresis_threshold_for_testing(base::TimeDelta());
  EXPECT_TRUE(estimator->UpdateCadenceEstimate(
      render_interval, frame_interval, base::TimeDelta(), acceptable_drift));
  EXPECT_TRUE(estimator->has_cadence());
  EXPECT_EQ("[1:0]", estimator->GetCadenceForTesting());
}

TEST(VideoCadenceEstimatorTest, CadenceHystersisPreventsOscillation) {
  std::unique_ptr<VideoCadenceEstimator> estimator(new VideoCadenceEstimator(
      base::TimeDelta::FromSeconds(kMinimumAcceptableTimeBetweenGlitchesSecs)));

  const base::TimeDelta render_interval = Interval(30);
  const base::TimeDelta frame_interval = Interval(60);
  const base::TimeDelta acceptable_drift = frame_interval / 2;
  estimator->set_cadence_hysteresis_threshold_for_testing(render_interval * 2);

  // Cadence hysteresis should prevent the cadence from taking effect yet.
  EXPECT_FALSE(estimator->UpdateCadenceEstimate(
      render_interval, frame_interval, base::TimeDelta(), acceptable_drift));
  EXPECT_FALSE(estimator->has_cadence());

  // A second call should exceed cadence hysteresis and take into effect.
  EXPECT_TRUE(estimator->UpdateCadenceEstimate(
      render_interval, frame_interval, base::TimeDelta(), acceptable_drift));
  EXPECT_TRUE(estimator->has_cadence());

  // One bad interval shouldn't cause cadence to drop
  EXPECT_FALSE(
      estimator->UpdateCadenceEstimate(render_interval, frame_interval * 0.75,
                                       base::TimeDelta(), acceptable_drift));
  EXPECT_TRUE(estimator->has_cadence());

  // Resumption of cadence should clear bad interval count.
  EXPECT_FALSE(estimator->UpdateCadenceEstimate(
      render_interval, frame_interval, base::TimeDelta(), acceptable_drift));
  EXPECT_TRUE(estimator->has_cadence());

  // So one more bad interval shouldn't cause cadence to drop
  EXPECT_FALSE(
      estimator->UpdateCadenceEstimate(render_interval, frame_interval * 0.75,
                                       base::TimeDelta(), acceptable_drift));
  EXPECT_TRUE(estimator->has_cadence());

  // Two bad intervals should.
  EXPECT_TRUE(
      estimator->UpdateCadenceEstimate(render_interval, frame_interval * 0.75,
                                       base::TimeDelta(), acceptable_drift));
  EXPECT_FALSE(estimator->has_cadence());
}

}  // namespace media
