// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/feature_engagement/internal/never_availability_model.h"

#include "base/bind.h"
#include "base/feature_list.h"
#include "base/message_loop/message_loop.h"
#include "base/optional.h"
#include "base/run_loop.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace feature_engagement {

namespace {

const base::Feature kTestFeatureFoo{"test_foo",
                                    base::FEATURE_DISABLED_BY_DEFAULT};
const base::Feature kTestFeatureBar{"test_bar",
                                    base::FEATURE_DISABLED_BY_DEFAULT};

class NeverAvailabilityModelTest : public ::testing::Test {
 public:
  NeverAvailabilityModelTest() = default;

  void OnInitializedCallback(bool success) { success_ = success; }

 protected:
  NeverAvailabilityModel availability_model_;
  base::Optional<bool> success_;

 private:
  base::MessageLoop message_loop_;

  DISALLOW_COPY_AND_ASSIGN(NeverAvailabilityModelTest);
};

}  // namespace

TEST_F(NeverAvailabilityModelTest, ShouldNeverHaveData) {
  EXPECT_EQ(base::nullopt,
            availability_model_.GetAvailability(kTestFeatureFoo));
  EXPECT_EQ(base::nullopt,
            availability_model_.GetAvailability(kTestFeatureBar));

  availability_model_.Initialize(
      base::BindOnce(&NeverAvailabilityModelTest::OnInitializedCallback,
                     base::Unretained(this)),
      14u);
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(base::nullopt,
            availability_model_.GetAvailability(kTestFeatureFoo));
  EXPECT_EQ(base::nullopt,
            availability_model_.GetAvailability(kTestFeatureBar));
}

TEST_F(NeverAvailabilityModelTest, ShouldBeReadyAfterInitialization) {
  EXPECT_FALSE(availability_model_.IsReady());
  availability_model_.Initialize(
      base::BindOnce(&NeverAvailabilityModelTest::OnInitializedCallback,
                     base::Unretained(this)),
      14u);
  EXPECT_FALSE(availability_model_.IsReady());
  EXPECT_FALSE(success_.has_value());
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(availability_model_.IsReady());
  ASSERT_TRUE(success_.has_value());
  EXPECT_TRUE(success_.value());
}

}  // namespace feature_engagement
