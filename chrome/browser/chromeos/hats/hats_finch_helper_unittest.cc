// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/hats/hats_finch_helper.h"

#include <map>
#include <set>
#include <string>

#include "base/strings/string_number_conversions.h"
#include "base/test/scoped_feature_list.h"
#include "chrome/common/chrome_features.h"
#include "chrome/common/pref_names.h"
#include "chrome/test/base/testing_profile.h"
#include "components/prefs/pref_service.h"
#include "components/variations/variations_associated_data.h"
#include "components/variations/variations_params_manager.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace chromeos {

namespace {

const std::string kFeatureAndTrialName(features::kHappinessTrackingSystem.name);

}  // namespace

class HatsFinchHelperTest : public testing::Test {
 public:
  using ParamMap = std::map<std::string, std::string>;

  HatsFinchHelperTest() {}

  void SetFinchSeedParams(ParamMap params) {
    params_manager_.SetVariationParamsWithFeatureAssociations(
        kFeatureAndTrialName /* trial name */, params,
        std::set<std::string>{kFeatureAndTrialName} /*features to switch on */);
  }

  ParamMap CreateParamMap(std::string prob,
                          std::string cycle_length,
                          std::string start_date,
                          std::string reset_survey,
                          std::string reset) {
    ParamMap params;
    params[HatsFinchHelper::kProbabilityParam] = prob;
    params[HatsFinchHelper::kSurveyCycleLengthParam] = cycle_length;
    params[HatsFinchHelper::kSurveyStartDateMsParam] = start_date;
    params[HatsFinchHelper::kResetSurveyCycleParam] = reset_survey;
    params[HatsFinchHelper::kResetAllParam] = reset;
    return params;
  }

 private:
  // Must outlive |profile_|.
  content::TestBrowserThreadBundle thread_bundle_;

 protected:
  TestingProfile profile_;

 private:
  variations::testing::VariationParamsManager params_manager_;

  DISALLOW_COPY_AND_ASSIGN(HatsFinchHelperTest);
};

TEST_F(HatsFinchHelperTest, InitFinchSeed_ValidValues) {
  ParamMap params =
      CreateParamMap("1.0", "7", "1475613895337", "false", "false");
  SetFinchSeedParams(params);

  HatsFinchHelper hats_finch_helper(&profile_);

  EXPECT_EQ(hats_finch_helper.probability_of_pick_, 1.0);
  EXPECT_EQ(hats_finch_helper.survey_cycle_length_, 7);
  EXPECT_EQ(hats_finch_helper.first_survey_start_date_,
            base::Time().FromJsTime(1475613895337LL));
  EXPECT_FALSE(hats_finch_helper.reset_survey_cycle_);
  EXPECT_FALSE(hats_finch_helper.reset_hats_);
}

TEST_F(HatsFinchHelperTest, InitFinchSeed_Invalidalues) {
  ParamMap params = CreateParamMap("-0.1", "-1", "-1000", "false", "false");
  SetFinchSeedParams(params);

  base::Time current_time = base::Time::Now();
  HatsFinchHelper hats_finch_helper(&profile_);

  EXPECT_EQ(hats_finch_helper.probability_of_pick_, 0.0);
  EXPECT_EQ(hats_finch_helper.survey_cycle_length_, INT_MAX);
  EXPECT_GE(hats_finch_helper.first_survey_start_date_.ToJsTime(),
            2 * current_time.ToJsTime());
}

TEST_F(HatsFinchHelperTest, TestComputeNextDate) {
  ParamMap params = CreateParamMap("0",
                                   "7",  // 7 Days survey cycle length
                                   "0", "false", "false");

  SetFinchSeedParams(params);

  base::Time current_time = base::Time::Now();

  HatsFinchHelper hats_finch_helper(&profile_);

  // Case 1
  base::Time start_date = current_time - base::TimeDelta::FromDays(10);
  hats_finch_helper.first_survey_start_date_ = start_date;
  base::Time expected_date =
      start_date +
      base::TimeDelta::FromDays(2 * hats_finch_helper.survey_cycle_length_);
  EXPECT_EQ(expected_date.ToJsTime(),
            hats_finch_helper.ComputeNextEndDate().ToJsTime());

  // Case 2
  base::Time future_time = current_time + base::TimeDelta::FromDays(10);
  hats_finch_helper.first_survey_start_date_ = future_time;
  expected_date = future_time + base::TimeDelta::FromDays(
                                    hats_finch_helper.survey_cycle_length_);
  EXPECT_EQ(expected_date.ToJsTime(),
            hats_finch_helper.ComputeNextEndDate().ToJsTime());
}

TEST_F(HatsFinchHelperTest, ResetSurveyCycle) {
  ParamMap params = CreateParamMap("0.5", "7", "1475613895337", "TruE", "0");
  SetFinchSeedParams(params);

  int64_t initial_timestamp = base::Time::Now().ToInternalValue();
  PrefService* pref_service = profile_.GetPrefs();
  pref_service->SetBoolean(prefs::kHatsDeviceIsSelected, true);
  pref_service->SetInt64(prefs::kHatsSurveyCycleEndTimestamp,
                         initial_timestamp);

  base::Time current_time = base::Time::Now();
  HatsFinchHelper hats_finch_helper(&profile_);

  EXPECT_EQ(hats_finch_helper.probability_of_pick_, 0);
  EXPECT_EQ(hats_finch_helper.survey_cycle_length_, INT_MAX);
  EXPECT_GE(hats_finch_helper.first_survey_start_date_.ToJsTime(),
            2 * current_time.ToJsTime());

  EXPECT_FALSE(pref_service->GetBoolean(prefs::kHatsDeviceIsSelected));
  EXPECT_NE(pref_service->GetInt64(prefs::kHatsSurveyCycleEndTimestamp),
            initial_timestamp);
}

TEST_F(HatsFinchHelperTest, ResetHats) {
  ParamMap params = CreateParamMap("0.5", "7", "1475613895337", "0", "TrUe");
  SetFinchSeedParams(params);

  int64_t initial_timestamp = base::Time::Now().ToInternalValue();
  PrefService* pref_service = profile_.GetPrefs();
  pref_service->SetBoolean(prefs::kHatsDeviceIsSelected, true);
  pref_service->SetInt64(prefs::kHatsSurveyCycleEndTimestamp,
                         initial_timestamp);
  pref_service->SetInt64(prefs::kHatsLastInteractionTimestamp,
                         initial_timestamp);

  base::Time current_time = base::Time::Now();
  HatsFinchHelper hats_finch_helper(&profile_);

  EXPECT_EQ(hats_finch_helper.probability_of_pick_, 0);
  EXPECT_EQ(hats_finch_helper.survey_cycle_length_, INT_MAX);
  EXPECT_GE(hats_finch_helper.first_survey_start_date_.ToJsTime(),
            2 * current_time.ToJsTime());

  EXPECT_FALSE(pref_service->GetBoolean(prefs::kHatsDeviceIsSelected));
  EXPECT_NE(pref_service->GetInt64(prefs::kHatsSurveyCycleEndTimestamp),
            initial_timestamp);
  EXPECT_NE(pref_service->GetInt64(prefs::kHatsLastInteractionTimestamp),
            initial_timestamp);
}

}  // namespace chromeos
