// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/supervised_user/supervised_user_creation_policy_handler.h"

#include "base/values.h"
#include "chrome/common/pref_names.h"
#include "components/policy/core/common/policy_map.h"
#include "components/policy/policy_constants.h"
#include "components/prefs/pref_value_map.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace policy {

class SupervisedUserCreationPolicyHandlerTest : public ::testing::Test {
 protected:
  void SetUpPolicyAndApply(const char* policy_name, bool value) {
    policies_.Set(policy_name, POLICY_LEVEL_MANDATORY, POLICY_SCOPE_MACHINE,
                  POLICY_SOURCE_PLATFORM, std::make_unique<base::Value>(value),
                  nullptr);
    ApplyPolicySettings();
  }

  void ApplyPolicySettings() {
    handler_.ApplyPolicySettings(policies_, &prefs_);
  }

  PrefValueMap* prefs() { return &prefs_; }

 private:
  PolicyMap policies_;
  PrefValueMap prefs_;
  SupervisedUserCreationPolicyHandler handler_;
};

TEST_F(SupervisedUserCreationPolicyHandlerTest, ForceSigninNotSet) {
  ApplyPolicySettings();
  EXPECT_FALSE(
      prefs()->GetValue(prefs::kSupervisedUserCreationAllowed, nullptr));

  bool value;
  SetUpPolicyAndApply(key::kSupervisedUserCreationEnabled, true);
  EXPECT_TRUE(
      prefs()->GetBoolean(prefs::kSupervisedUserCreationAllowed, &value));
  EXPECT_TRUE(value);

  SetUpPolicyAndApply(key::kSupervisedUserCreationEnabled, false);
  EXPECT_TRUE(
      prefs()->GetBoolean(prefs::kSupervisedUserCreationAllowed, &value));
  EXPECT_FALSE(value);
}

TEST_F(SupervisedUserCreationPolicyHandlerTest, ForceSigninDisabled) {
  SetUpPolicyAndApply(key::kForceBrowserSignin, false);
  EXPECT_FALSE(
      prefs()->GetValue(prefs::kSupervisedUserCreationAllowed, nullptr));

  bool value;
  SetUpPolicyAndApply(key::kSupervisedUserCreationEnabled, true);
  EXPECT_TRUE(
      prefs()->GetBoolean(prefs::kSupervisedUserCreationAllowed, &value));
  EXPECT_TRUE(value);

  SetUpPolicyAndApply(key::kSupervisedUserCreationEnabled, false);
  EXPECT_TRUE(
      prefs()->GetBoolean(prefs::kSupervisedUserCreationAllowed, &value));
  EXPECT_FALSE(value);
}

TEST_F(SupervisedUserCreationPolicyHandlerTest, ForceSigninEnabled) {
  bool value;
  SetUpPolicyAndApply(key::kForceBrowserSignin, true);
  EXPECT_TRUE(
      prefs()->GetBoolean(prefs::kSupervisedUserCreationAllowed, &value));
  EXPECT_FALSE(value);

  // When force sign in is enabled, the supervised user creation will be
  // disabled even if the policy is set to true.
  SetUpPolicyAndApply(key::kSupervisedUserCreationEnabled, true);
  EXPECT_TRUE(
      prefs()->GetBoolean(prefs::kSupervisedUserCreationAllowed, &value));
  EXPECT_FALSE(value);

  SetUpPolicyAndApply(key::kSupervisedUserCreationEnabled, false);
  EXPECT_TRUE(
      prefs()->GetBoolean(prefs::kSupervisedUserCreationAllowed, &value));
  EXPECT_FALSE(value);
}

}  // namespace policy
