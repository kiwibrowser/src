// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/metrics/field_trials_provider.h"

#include "components/variations/active_field_trials.h"
#include "components/variations/synthetic_trial_registry.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/metrics_proto/system_profile.pb.h"

namespace variations {

namespace {

const ActiveGroupId kFieldTrialIds[] = {{37, 43}, {13, 47}, {23, 17}};
const ActiveGroupId kSyntheticTrials[] = {{55, 15}, {66, 16}};

class TestProvider : public FieldTrialsProvider {
 public:
  TestProvider(SyntheticTrialRegistry* registry, base::StringPiece suffix)
      : FieldTrialsProvider(registry, suffix) {}
  ~TestProvider() override {}

  void GetFieldTrialIds(
      std::vector<ActiveGroupId>* field_trial_ids) const override {
    ASSERT_TRUE(field_trial_ids->empty());
    for (const ActiveGroupId& id : kFieldTrialIds) {
      field_trial_ids->push_back(id);
    }
  }
};

// Check that the values in |system_values| correspond to the test data
// defined at the top of this file.
void CheckSystemProfile(const metrics::SystemProfileProto& system_profile) {
  ASSERT_EQ(arraysize(kFieldTrialIds) + arraysize(kSyntheticTrials),
            static_cast<size_t>(system_profile.field_trial_size()));
  for (size_t i = 0; i < arraysize(kFieldTrialIds); ++i) {
    const metrics::SystemProfileProto::FieldTrial& field_trial =
        system_profile.field_trial(i);
    EXPECT_EQ(kFieldTrialIds[i].name, field_trial.name_id());
    EXPECT_EQ(kFieldTrialIds[i].group, field_trial.group_id());
  }
  // Verify the right data is present for the synthetic trials.
  for (size_t i = 0; i < arraysize(kSyntheticTrials); ++i) {
    const metrics::SystemProfileProto::FieldTrial& field_trial =
        system_profile.field_trial(i + arraysize(kFieldTrialIds));
    EXPECT_EQ(kSyntheticTrials[i].name, field_trial.name_id());
    EXPECT_EQ(kSyntheticTrials[i].group, field_trial.group_id());
  }
}

}  // namespace

class FieldTrialsProviderTest : public ::testing::Test {
 public:
  FieldTrialsProviderTest() {}
  ~FieldTrialsProviderTest() override {}

 protected:
  // Register trials which should get recorded.
  void RegisterExpectedSyntheticTrials() {
    for (const ActiveGroupId& id : kSyntheticTrials) {
      registry_.RegisterSyntheticFieldTrial(
          SyntheticTrialGroup(id.name, id.group));
    }
  }
  // Register trial which shouldn't get recorded.
  void RegisterExtraSyntheticTrial() {
    registry_.RegisterSyntheticFieldTrial(SyntheticTrialGroup(100, 1000));
  }

  // Waits until base::TimeTicks::Now() no longer equals |value|. This should
  // take between 1-15ms per the documented resolution of base::TimeTicks.
  void WaitUntilTimeChanges(const base::TimeTicks& value) {
    while (base::TimeTicks::Now() == value) {
      base::PlatformThread::Sleep(base::TimeDelta::FromMilliseconds(1));
    }
  }

  SyntheticTrialRegistry registry_;
};

TEST_F(FieldTrialsProviderTest, ProvideSyntheticTrials) {
  TestProvider provider(&registry_, base::StringPiece());

  RegisterExpectedSyntheticTrials();
  // Make sure these trials are older than the log.
  WaitUntilTimeChanges(base::TimeTicks::Now());

  provider.OnDidCreateMetricsLog();
  // Make sure that the log is older than the trials that should be excluded.
  WaitUntilTimeChanges(base::TimeTicks::Now());

  RegisterExtraSyntheticTrial();

  metrics::SystemProfileProto proto;
  provider.ProvideSystemProfileMetrics(&proto);
  CheckSystemProfile(proto);
}

}  // namespace variations
