// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync/user_events/trial_recorder.h"

#include <memory>
#include <set>
#include <string>

#include "base/message_loop/message_loop.h"
#include "base/metrics/field_trial.h"
#include "base/test/scoped_feature_list.h"
#include "components/sync/driver/sync_driver_switches.h"
#include "components/sync/protocol/sync.pb.h"
#include "components/sync/user_events/fake_user_event_service.h"
#include "components/variations/variations_associated_data.h"
#include "testing/gtest/include/gtest/gtest.h"

using sync_pb::UserEventSpecifics;

namespace syncer {

namespace {

const char kTrial1[] = "TrialNameOne";
const char kTrial2[] = "TrialNameTwo";
const char kGroup[] = "GroupName";
const variations::VariationID kVariation1 = 111;
const variations::VariationID kVariation2 = 222;

void VerifyEvent(std::set<variations::VariationID> expected_variations,
                 const UserEventSpecifics& actual) {
  ASSERT_EQ(
      expected_variations.size(),
      static_cast<size_t>(actual.field_trial_event().variation_ids_size()));
  for (int actaul_variation : actual.field_trial_event().variation_ids()) {
    auto iter = expected_variations.find(actaul_variation);
    if (iter == expected_variations.end()) {
      FAIL() << actaul_variation;
    } else {
      // Remove to make sure the event doesn't contain duplicates.
      expected_variations.erase(iter);
    }
  }
}

void SetupAndFinalizeTrial(const std::string& trial_name,
                           variations::VariationID id) {
  variations::AssociateGoogleVariationID(variations::CHROME_SYNC_EVENT_LOGGER,
                                         trial_name, kGroup, id);
  base::FieldTrialList::CreateFieldTrial(trial_name, kGroup);
  base::FieldTrialList::FindFullName(trial_name);
  base::RunLoop().RunUntilIdle();
}

class TrialRecorderTest : public testing::Test {
 public:
  TrialRecorderTest() : field_trial_list_(nullptr) {}

  ~TrialRecorderTest() override { variations::testing::ClearAllVariationIDs(); }

  FakeUserEventService* service() { return &service_; }
  TrialRecorder* recorder() { return recorder_.get(); }

  void VerifyLastEvent(std::set<variations::VariationID> expected_variations) {
    ASSERT_LE(1u, service()->GetRecordedUserEvents().size());
    VerifyEvent(expected_variations,
                *service()->GetRecordedUserEvents().rbegin());
  }

  void InitRecorder() {
    recorder_ = std::make_unique<TrialRecorder>(&service_);
  }

 private:
  base::MessageLoop message_loop_;
  base::FieldTrialList field_trial_list_;
  FakeUserEventService service_;
  std::unique_ptr<TrialRecorder> recorder_;
};

TEST_F(TrialRecorderTest, FinalizedBeforeInit) {
  SetupAndFinalizeTrial(kTrial1, kVariation1);
  SetupAndFinalizeTrial(kTrial2, kVariation2);
  // Should check on initialization to see if there are any trails to record.
  InitRecorder();
  VerifyLastEvent({kVariation1, kVariation2});
}

TEST_F(TrialRecorderTest, FinalizedAfterInit) {
  InitRecorder();
  SetupAndFinalizeTrial(kTrial1, kVariation1);
  SetupAndFinalizeTrial(kTrial2, kVariation2);
  VerifyLastEvent({kVariation1, kVariation2});
}

TEST_F(TrialRecorderTest, FinalizedMix) {
  SetupAndFinalizeTrial(kTrial1, kVariation1);
  InitRecorder();
  VerifyLastEvent({kVariation1});

  SetupAndFinalizeTrial(kTrial2, kVariation2);
  VerifyLastEvent({kVariation1, kVariation2});
}

TEST_F(TrialRecorderTest, WrongVariationKey) {
  InitRecorder();
  variations::AssociateGoogleVariationID(variations::GOOGLE_WEB_PROPERTIES,
                                         kTrial1, kGroup, kVariation1);
  base::FieldTrialList::CreateFieldTrial(kTrial1, kGroup);
  base::FieldTrialList::FindFullName(kTrial1);
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(0u, service()->GetRecordedUserEvents().size());
}

TEST_F(TrialRecorderTest, NoVariation) {
  InitRecorder();
  base::FieldTrialList::CreateFieldTrial(kTrial1, kGroup);
  base::FieldTrialList::FindFullName(kTrial1);
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(0u, service()->GetRecordedUserEvents().size());
}

TEST_F(TrialRecorderTest, FieldTrialFeatureDisabled) {
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(
      switches::kSyncUserFieldTrialEvents);
  InitRecorder();
  SetupAndFinalizeTrial(kTrial1, kVariation1);

  EXPECT_EQ(0u, service()->GetRecordedUserEvents().size());
}

TEST_F(TrialRecorderTest, FieldTrialTimer) {
  SetupAndFinalizeTrial(kTrial2, kVariation2);

  {
    // Start with 0 delay, which should mean we post immediately to record.
    // Need to be not call any methods that might invoke RunUntilIdle() while we
    // have a delay of 0, like SetupAndFinalizeTrial().
    base::test::ScopedFeatureList scoped_feature_list;
    scoped_feature_list.InitAndEnableFeatureWithParameters(
        switches::kSyncUserFieldTrialEvents,
        {{"field_trial_delay_seconds", "0"}});
    InitRecorder();
    EXPECT_EQ(1u, service()->GetRecordedUserEvents().size());
  }

  // Now that |scoped_feature_list| is gone, we should reset to default,
  // otherwise our RunUntilIdle() would infinitively loop with a delay of 0.
  base::RunLoop().RunUntilIdle();
  // Should have picked up exactly one more event.
  EXPECT_EQ(2u, service()->GetRecordedUserEvents().size());
}

}  // namespace

}  // namespace syncer
