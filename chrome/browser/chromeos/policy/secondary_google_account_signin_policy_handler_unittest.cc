// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>
#include <string>
#include <utility>

#include "base/macros.h"
#include "chrome/browser/chromeos/policy/secondary_google_account_signin_policy_handler.h"
#include "components/policy/core/common/policy_map.h"
#include "components/policy/core/common/policy_pref_names.h"
#include "components/policy/policy_constants.h"
#include "components/prefs/pref_value_map.h"
#include "components/signin/core/browser/signin_pref_names.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace policy {

class SecondaryGoogleAccountSigninPolicyHandlerTest : public testing::Test {
 protected:
  SecondaryGoogleAccountSigninPolicyHandlerTest() = default;

  void SetPolicy(std::unique_ptr<base::Value> value) {
    policies_.Set(key::kSecondaryGoogleAccountSigninAllowed,
                  POLICY_LEVEL_MANDATORY, POLICY_SCOPE_USER,
                  POLICY_SOURCE_CLOUD, std::move(value), nullptr);
  }

  void ApplyPolicySettings(bool value) {
    SetPolicy(std::make_unique<base::Value>(value));
    handler_.ApplyPolicySettings(policies_, &prefs_);
  }

  bool GetAccountConsistencyPref(bool* pref) {
    return prefs_.GetBoolean(prefs::kAccountConsistencyMirrorRequired, pref);
  }

  void SetAccountConsistencyPref(bool pref) {
    prefs_.SetBoolean(prefs::kAccountConsistencyMirrorRequired, pref);
  }

 private:
  SecondaryGoogleAccountSigninPolicyHandler handler_;
  PolicyMap policies_;
  PrefValueMap prefs_;

  DISALLOW_COPY_AND_ASSIGN(SecondaryGoogleAccountSigninPolicyHandlerTest);
};

TEST_F(SecondaryGoogleAccountSigninPolicyHandlerTest,
       CheckSigninAllowedDoesNotChangeDefaultTruePreference) {
  SetAccountConsistencyPref(true);
  ApplyPolicySettings(true /* policy value */);

  bool preference = false;
  EXPECT_TRUE(GetAccountConsistencyPref(&preference));
  EXPECT_TRUE(preference);
}

TEST_F(SecondaryGoogleAccountSigninPolicyHandlerTest,
       CheckSigninAllowedDoesNotChangeDefaultFalsePreference) {
  SetAccountConsistencyPref(false);
  ApplyPolicySettings(true /* policy value */);

  bool preference = true;
  EXPECT_TRUE(GetAccountConsistencyPref(&preference));
  EXPECT_FALSE(preference);
}

TEST_F(SecondaryGoogleAccountSigninPolicyHandlerTest,
       CheckSigninDisallowedEnablesMirror) {
  ApplyPolicySettings(false /* policy value */);

  bool preference = false;
  EXPECT_TRUE(GetAccountConsistencyPref(&preference));
  EXPECT_TRUE(preference);
}

}  // namespace policy
