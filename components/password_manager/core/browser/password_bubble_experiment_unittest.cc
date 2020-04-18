// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/password_manager/core/browser/password_bubble_experiment.h"

#include <ostream>

#include "base/strings/string_number_conversions.h"
#include "components/password_manager/core/common/password_manager_pref_names.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/testing_pref_service.h"
#include "components/sync/base/model_type.h"
#include "components/sync/driver/fake_sync_service.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace password_bubble_experiment {

namespace {

enum class CustomPassphraseState { NONE, SET };

class TestSyncService : public syncer::FakeSyncService {
 public:
  // FakeSyncService overrides.
  bool IsSyncAllowed() const override { return is_sync_allowed_; }

  bool IsFirstSetupComplete() const override {
    return is_first_setup_complete_;
  }

  bool IsSyncActive() const override {
    return is_sync_allowed_ && is_first_setup_complete_;
  }

  syncer::ModelTypeSet GetActiveDataTypes() const override { return type_set_; }

  syncer::ModelTypeSet GetPreferredDataTypes() const override {
    return type_set_;
  }

  bool IsUsingSecondaryPassphrase() const override {
    return is_using_secondary_passphrase_;
  }

  void set_is_using_secondary_passphrase(bool is_using_secondary_passphrase) {
    is_using_secondary_passphrase_ = is_using_secondary_passphrase;
  }

  void set_active_data_types(syncer::ModelTypeSet type_set) {
    type_set_ = type_set;
  }

  void set_sync_allowed(bool sync_allowed) { is_sync_allowed_ = sync_allowed; }

  void set_first_setup_complete(bool setup_complete) {
    is_first_setup_complete_ = setup_complete;
  }

  void ClearActiveDataTypes() { type_set_.Clear(); }

  bool CanSyncStart() const override { return true; }

 private:
  syncer::ModelTypeSet type_set_;
  bool is_using_secondary_passphrase_ = false;
  bool is_sync_allowed_ = true;
  bool is_first_setup_complete_ = true;
};

}  // namespace

class PasswordManagerPasswordBubbleExperimentTest : public testing::Test {
 public:
  PasswordManagerPasswordBubbleExperimentTest() {
    RegisterPrefs(pref_service_.registry());
  }

  PrefService* prefs() { return &pref_service_; }

  TestSyncService* sync_service() { return &fake_sync_service_; }

 protected:
  void SetupFakeSyncServiceForTestCase(syncer::ModelType type,
                                       CustomPassphraseState passphrase_state) {
    syncer::ModelTypeSet active_types;
    active_types.Put(type);
    sync_service()->ClearActiveDataTypes();
    sync_service()->set_active_data_types(active_types);
    sync_service()->set_is_using_secondary_passphrase(
        passphrase_state == CustomPassphraseState::SET);
  }

 private:
  TestSyncService fake_sync_service_;
  TestingPrefServiceSimple pref_service_;
};

TEST_F(PasswordManagerPasswordBubbleExperimentTest,
       ShouldShowChromeSignInPasswordPromo) {
  // By default the promo is off.
  EXPECT_FALSE(ShouldShowChromeSignInPasswordPromo(prefs(), nullptr));
  constexpr struct {
    bool was_already_clicked;
    bool is_sync_allowed;
    bool is_first_setup_complete;
    int current_shown_count;
    bool result;
  } kTestData[] = {
      {false, true, false, 0, true},   {false, true, false, 5, false},
      {true, true, false, 0, false},   {true, true, false, 10, false},
      {false, false, false, 0, false}, {false, true, true, 0, false},
  };
  for (const auto& test_case : kTestData) {
    SCOPED_TRACE(testing::Message("#test_case = ") << (&test_case - kTestData));
    prefs()->SetBoolean(password_manager::prefs::kWasSignInPasswordPromoClicked,
                        test_case.was_already_clicked);
    prefs()->SetInteger(
        password_manager::prefs::kNumberSignInPasswordPromoShown,
        test_case.current_shown_count);
    sync_service()->set_sync_allowed(test_case.is_sync_allowed);
    sync_service()->set_first_setup_complete(test_case.is_first_setup_complete);

    EXPECT_EQ(test_case.result,
              ShouldShowChromeSignInPasswordPromo(prefs(), sync_service()));
  }
}

TEST_F(PasswordManagerPasswordBubbleExperimentTest, IsSmartLockUser) {
  constexpr struct {
    syncer::ModelType type;
    CustomPassphraseState passphrase_state;
    bool expected_smart_lock_user;
  } kTestData[] = {
      {syncer::ModelType::BOOKMARKS, CustomPassphraseState::NONE, false},
      {syncer::ModelType::BOOKMARKS, CustomPassphraseState::SET, false},
      {syncer::ModelType::PASSWORDS, CustomPassphraseState::NONE, true},
      {syncer::ModelType::PASSWORDS, CustomPassphraseState::SET, false},
  };
  for (const auto& test_case : kTestData) {
    SCOPED_TRACE(testing::Message("#test_case = ") << (&test_case - kTestData));
    SetupFakeSyncServiceForTestCase(test_case.type, test_case.passphrase_state);

    EXPECT_EQ(test_case.expected_smart_lock_user,
              IsSmartLockUser(sync_service()));
  }
}

}  // namespace password_bubble_experiment
