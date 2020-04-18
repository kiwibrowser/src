// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/background_sync/background_sync_controller_impl.h"

#include <stdint.h>

#include "base/macros.h"
#include "chrome/test/base/testing_profile.h"
#include "components/rappor/test_rappor_service.h"
#include "components/variations/variations_associated_data.h"
#include "content/public/browser/background_sync_parameters.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

#if defined(OS_ANDROID)
#include "chrome/browser/android/background_sync_launcher_android.h"
#endif

namespace {

using content::BackgroundSyncController;

const char kFieldTrialGroup[] = "GroupA";

class TestBackgroundSyncControllerImpl : public BackgroundSyncControllerImpl {
 public:
  TestBackgroundSyncControllerImpl(
      Profile* profile,
      rappor::TestRapporServiceImpl* rappor_service)
      : BackgroundSyncControllerImpl(profile),
        rappor_service_(rappor_service) {}

 protected:
  rappor::RapporServiceImpl* GetRapporServiceImpl() override {
    return rappor_service_;
  }

 private:
  rappor::TestRapporServiceImpl* rappor_service_;

  DISALLOW_COPY_AND_ASSIGN(TestBackgroundSyncControllerImpl);
};

class BackgroundSyncControllerImplTest : public testing::Test {
 protected:
  BackgroundSyncControllerImplTest()
      : thread_bundle_(content::TestBrowserThreadBundle::IO_MAINLOOP),
        controller_(
            new TestBackgroundSyncControllerImpl(&profile_, &rappor_service_)) {
    ResetFieldTrialList();
#if defined(OS_ANDROID)
    BackgroundSyncLauncherAndroid::SetPlayServicesVersionCheckDisabledForTests(
        true);
#endif
  }

  void ResetFieldTrialList() {
    field_trial_list_.reset(
        new base::FieldTrialList(nullptr /* entropy provider */));
    variations::testing::ClearAllVariationParams();
    base::FieldTrialList::CreateFieldTrial(
        BackgroundSyncControllerImpl::kFieldTrialName, kFieldTrialGroup);
  }

  content::TestBrowserThreadBundle thread_bundle_;
  TestingProfile profile_;
  rappor::TestRapporServiceImpl rappor_service_;
  std::unique_ptr<TestBackgroundSyncControllerImpl> controller_;
  std::unique_ptr<base::FieldTrialList> field_trial_list_;

  DISALLOW_COPY_AND_ASSIGN(BackgroundSyncControllerImplTest);
};

TEST_F(BackgroundSyncControllerImplTest, RapporTest) {
  GURL url("http://www.example.com/foo/");
  EXPECT_EQ(0, rappor_service_.GetReportsCount());
  controller_->NotifyBackgroundSyncRegistered(url.GetOrigin());
  EXPECT_EQ(1, rappor_service_.GetReportsCount());

  std::string sample;
  rappor::RapporType type;
  LOG(ERROR) << url.GetOrigin().GetOrigin();
  EXPECT_TRUE(rappor_service_.GetRecordedSampleForMetric(
      "BackgroundSync.Register.Origin", &sample, &type));
  EXPECT_EQ("example.com", sample);
  EXPECT_EQ(rappor::ETLD_PLUS_ONE_RAPPOR_TYPE, type);
}

TEST_F(BackgroundSyncControllerImplTest, NoRapporWhenOffTheRecord) {
  GURL url("http://www.example.com/foo/");
  controller_.reset(new TestBackgroundSyncControllerImpl(
      profile_.GetOffTheRecordProfile(), &rappor_service_));

  controller_->NotifyBackgroundSyncRegistered(url.GetOrigin());
  EXPECT_EQ(0, rappor_service_.GetReportsCount());
}

TEST_F(BackgroundSyncControllerImplTest, NoFieldTrial) {
  content::BackgroundSyncParameters original;
  content::BackgroundSyncParameters overrides;
  controller_->GetParameterOverrides(&overrides);
  EXPECT_EQ(original, overrides);
}

TEST_F(BackgroundSyncControllerImplTest, SomeParamsSet) {
  std::map<std::string, std::string> field_parameters;
  field_parameters[BackgroundSyncControllerImpl::kDisabledParameterName] =
      "TrUe";
  field_parameters[BackgroundSyncControllerImpl::kInitialRetryParameterName] =
      "100";
  ASSERT_TRUE(variations::AssociateVariationParams(
      BackgroundSyncControllerImpl::kFieldTrialName, kFieldTrialGroup,
      field_parameters));

  content::BackgroundSyncParameters original;
  content::BackgroundSyncParameters sync_parameters;
  controller_->GetParameterOverrides(&sync_parameters);
  EXPECT_TRUE(sync_parameters.disable);
  EXPECT_EQ(base::TimeDelta::FromSeconds(100),
            sync_parameters.initial_retry_delay);

  EXPECT_EQ(original.max_sync_attempts, sync_parameters.max_sync_attempts);
  EXPECT_EQ(original.retry_delay_factor, sync_parameters.retry_delay_factor);
  EXPECT_EQ(original.min_sync_recovery_time,
            sync_parameters.min_sync_recovery_time);
  EXPECT_EQ(original.max_sync_event_duration,
            sync_parameters.max_sync_event_duration);
}

TEST_F(BackgroundSyncControllerImplTest, AllParamsSet) {
  std::map<std::string, std::string> field_parameters;
  field_parameters[BackgroundSyncControllerImpl::kDisabledParameterName] =
      "FALSE";
  field_parameters[BackgroundSyncControllerImpl::kInitialRetryParameterName] =
      "100";
  field_parameters[BackgroundSyncControllerImpl::kMaxAttemptsParameterName] =
      "200";
  field_parameters
      [BackgroundSyncControllerImpl::kRetryDelayFactorParameterName] = "300";
  field_parameters[BackgroundSyncControllerImpl::kMinSyncRecoveryTimeName] =
      "400";
  field_parameters[BackgroundSyncControllerImpl::kMaxSyncEventDurationName] =
      "500";
  ASSERT_TRUE(variations::AssociateVariationParams(
      BackgroundSyncControllerImpl::kFieldTrialName, kFieldTrialGroup,
      field_parameters));

  content::BackgroundSyncParameters sync_parameters;
  controller_->GetParameterOverrides(&sync_parameters);

  EXPECT_FALSE(sync_parameters.disable);
  EXPECT_EQ(base::TimeDelta::FromSeconds(100),
            sync_parameters.initial_retry_delay);
  EXPECT_EQ(200, sync_parameters.max_sync_attempts);
  EXPECT_EQ(300, sync_parameters.retry_delay_factor);
  EXPECT_EQ(base::TimeDelta::FromSeconds(400),
            sync_parameters.min_sync_recovery_time);
  EXPECT_EQ(base::TimeDelta::FromSeconds(500),
            sync_parameters.max_sync_event_duration);
}

}  // namespace
