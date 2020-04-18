// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/feature_engagement/internal/never_condition_validator.h"

#include <string>

#include "base/feature_list.h"
#include "components/feature_engagement/internal/configuration.h"
#include "components/feature_engagement/internal/event_model.h"
#include "components/feature_engagement/internal/never_availability_model.h"
#include "components/feature_engagement/internal/noop_display_lock_controller.h"
#include "components/feature_engagement/internal/proto/event.pb.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace feature_engagement {

namespace {

const base::Feature kTestFeatureFoo{"test_foo",
                                    base::FEATURE_DISABLED_BY_DEFAULT};
const base::Feature kTestFeatureBar{"test_bar",
                                    base::FEATURE_DISABLED_BY_DEFAULT};

// A EventModel that is always postive to show in-product help.
class TestEventModel : public EventModel {
 public:
  TestEventModel() = default;

  void Initialize(const OnModelInitializationFinished& callback,
                  uint32_t current_day) override {}

  bool IsReady() const override { return true; }

  const Event* GetEvent(const std::string& event_name) const override {
    return nullptr;
  }

  void IncrementEvent(const std::string& event_name, uint32_t day) override {}

 private:
  DISALLOW_COPY_AND_ASSIGN(TestEventModel);
};

class NeverConditionValidatorTest : public ::testing::Test {
 public:
  NeverConditionValidatorTest() = default;

 protected:
  TestEventModel event_model_;
  NeverAvailabilityModel availability_model_;
  NoopDisplayLockController display_lock_controller_;
  NeverConditionValidator validator_;

 private:
  DISALLOW_COPY_AND_ASSIGN(NeverConditionValidatorTest);
};

}  // namespace

TEST_F(NeverConditionValidatorTest, ShouldNeverMeetConditions) {
  EXPECT_FALSE(validator_
                   .MeetsConditions(kTestFeatureFoo, FeatureConfig(),
                                    event_model_, availability_model_,
                                    display_lock_controller_, 0u)
                   .NoErrors());
  EXPECT_FALSE(validator_
                   .MeetsConditions(kTestFeatureBar, FeatureConfig(),
                                    event_model_, availability_model_,
                                    display_lock_controller_, 0u)
                   .NoErrors());
}

}  // namespace feature_engagement
