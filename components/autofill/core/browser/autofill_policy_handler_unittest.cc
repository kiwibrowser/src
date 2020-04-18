// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>

#include "components/autofill/core/browser/autofill_policy_handler.h"
#include "base/values.h"
#include "components/autofill/core/common/autofill_pref_names.h"
#include "components/policy/core/common/policy_map.h"
#include "components/policy/core/common/policy_types.h"
#include "components/policy/policy_constants.h"
#include "components/prefs/pref_value_map.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace autofill {

// Test cases for the Autofill policy setting.
class AutofillPolicyHandlerTest : public testing::Test {};

TEST_F(AutofillPolicyHandlerTest, Default) {
  policy::PolicyMap policy;
  PrefValueMap prefs;
  AutofillPolicyHandler handler;
  handler.ApplyPolicySettings(policy, &prefs);
  EXPECT_FALSE(prefs.GetValue(autofill::prefs::kAutofillEnabled, nullptr));
}

TEST_F(AutofillPolicyHandlerTest, Enabled) {
  policy::PolicyMap policy;
  policy.Set(policy::key::kAutoFillEnabled, policy::POLICY_LEVEL_MANDATORY,
             policy::POLICY_SCOPE_USER, policy::POLICY_SOURCE_CLOUD,
             std::make_unique<base::Value>(true), nullptr);
  PrefValueMap prefs;
  AutofillPolicyHandler handler;
  handler.ApplyPolicySettings(policy, &prefs);

  // Enabling Autofill should not set the pref.
  EXPECT_FALSE(prefs.GetValue(autofill::prefs::kAutofillEnabled, nullptr));
}

TEST_F(AutofillPolicyHandlerTest, Disabled) {
  policy::PolicyMap policy;
  policy.Set(policy::key::kAutoFillEnabled, policy::POLICY_LEVEL_MANDATORY,
             policy::POLICY_SCOPE_USER, policy::POLICY_SOURCE_CLOUD,
             std::make_unique<base::Value>(false), nullptr);
  PrefValueMap prefs;
  AutofillPolicyHandler handler;
  handler.ApplyPolicySettings(policy, &prefs);

  // Disabling Autofill should switch the pref to managed.
  const base::Value* value = nullptr;
  EXPECT_TRUE(prefs.GetValue(autofill::prefs::kAutofillEnabled, &value));
  ASSERT_TRUE(value);
  bool autofill_enabled = true;
  bool result = value->GetAsBoolean(&autofill_enabled);
  ASSERT_TRUE(result);
  EXPECT_FALSE(autofill_enabled);
}

}  // namespace autofill
