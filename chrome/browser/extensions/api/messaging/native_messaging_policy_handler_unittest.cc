// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/api/messaging/native_messaging_policy_handler.h"

#include "chrome/browser/extensions/policy_handlers.h"
#include "components/policy/core/browser/policy_error_map.h"
#include "components/policy/core/common/policy_map.h"
#include "components/policy/core/common/policy_types.h"
#include "components/policy/policy_constants.h"
#include "components/prefs/pref_value_map.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace extensions {

const char kTestPref[] = "unit_test.test_pref";

TEST(NativeMessagingHostListPolicyHandlerTest, CheckPolicySettings) {
  base::ListValue list;
  policy::PolicyMap policy_map;
  NativeMessagingHostListPolicyHandler handler(
      policy::key::kNativeMessagingBlacklist, kTestPref, true);

  policy_map.Set(policy::key::kNativeMessagingBlacklist,
                 policy::POLICY_LEVEL_MANDATORY, policy::POLICY_SCOPE_USER,
                 policy::POLICY_SOURCE_CLOUD, list.CreateDeepCopy(), nullptr);
  {
    policy::PolicyErrorMap errors;
    EXPECT_TRUE(handler.CheckPolicySettings(policy_map, &errors));
    EXPECT_TRUE(errors.empty());
  }

  list.AppendString("test.a.b");
  policy_map.Set(policy::key::kNativeMessagingBlacklist,
                 policy::POLICY_LEVEL_MANDATORY, policy::POLICY_SCOPE_USER,
                 policy::POLICY_SOURCE_CLOUD, list.CreateDeepCopy(), nullptr);
  {
    policy::PolicyErrorMap errors;
    EXPECT_TRUE(handler.CheckPolicySettings(policy_map, &errors));
    EXPECT_TRUE(errors.empty());
  }

  list.AppendString("*");
  policy_map.Set(policy::key::kNativeMessagingBlacklist,
                 policy::POLICY_LEVEL_MANDATORY, policy::POLICY_SCOPE_USER,
                 policy::POLICY_SOURCE_CLOUD, list.CreateDeepCopy(), nullptr);
  {
    policy::PolicyErrorMap errors;
    EXPECT_TRUE(handler.CheckPolicySettings(policy_map, &errors));
    EXPECT_TRUE(errors.empty());
  }

  list.AppendString("invalid Name");
  policy_map.Set(policy::key::kNativeMessagingBlacklist,
                 policy::POLICY_LEVEL_MANDATORY, policy::POLICY_SCOPE_USER,
                 policy::POLICY_SOURCE_CLOUD, list.CreateDeepCopy(), nullptr);
  {
    policy::PolicyErrorMap errors;
    EXPECT_TRUE(handler.CheckPolicySettings(policy_map, &errors));
    EXPECT_FALSE(errors.empty());
    EXPECT_FALSE(
        errors.GetErrors(policy::key::kNativeMessagingBlacklist).empty());
  }
}

TEST(NativeMessagingHostListPolicyHandlerTest, ApplyPolicySettings) {
  base::ListValue policy;
  base::ListValue expected;
  policy::PolicyMap policy_map;
  PrefValueMap prefs;
  base::Value* value = NULL;
  NativeMessagingHostListPolicyHandler handler(
      policy::key::kNativeMessagingBlacklist, kTestPref, true);

  policy.AppendString("com.example.test");
  expected.AppendString("com.example.test");

  policy_map.Set(policy::key::kNativeMessagingBlacklist,
                 policy::POLICY_LEVEL_MANDATORY, policy::POLICY_SCOPE_USER,
                 policy::POLICY_SOURCE_CLOUD, policy.CreateDeepCopy(), nullptr);
  handler.ApplyPolicySettings(policy_map, &prefs);
  EXPECT_TRUE(prefs.GetValue(kTestPref, &value));
  EXPECT_EQ(expected, *value);

  policy.AppendString("*");
  expected.AppendString("*");

  policy_map.Set(policy::key::kNativeMessagingBlacklist,
                 policy::POLICY_LEVEL_MANDATORY, policy::POLICY_SCOPE_USER,
                 policy::POLICY_SOURCE_CLOUD, policy.CreateDeepCopy(), nullptr);
  handler.ApplyPolicySettings(policy_map, &prefs);
  EXPECT_TRUE(prefs.GetValue(kTestPref, &value));
  EXPECT_EQ(expected, *value);

  policy.AppendString("invalid Name");
  policy_map.Set(policy::key::kNativeMessagingBlacklist,
                 policy::POLICY_LEVEL_MANDATORY, policy::POLICY_SCOPE_USER,
                 policy::POLICY_SOURCE_CLOUD, policy.CreateDeepCopy(), nullptr);
  handler.ApplyPolicySettings(policy_map, &prefs);
  EXPECT_TRUE(prefs.GetValue(kTestPref, &value));
  EXPECT_EQ(expected, *value);
}

}  // namespace extensions
