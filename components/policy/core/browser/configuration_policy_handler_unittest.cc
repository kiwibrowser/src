// Copyright (c) 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/policy/core/browser/configuration_policy_handler.h"

#include <memory>

#include "base/bind.h"
#include "base/callback.h"
#include "base/json/json_reader.h"
#include "base/values.h"
#include "components/policy/core/browser/policy_error_map.h"
#include "components/policy/core/common/policy_map.h"
#include "components/policy/core/common/policy_types.h"
#include "components/policy/core/common/schema.h"
#include "components/prefs/pref_value_map.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace policy {

namespace {

void GetIntegerTypeMap(
    std::vector<std::unique_ptr<StringMappingListPolicyHandler::MappingEntry>>*
        result) {
  result->push_back(
      std::make_unique<StringMappingListPolicyHandler::MappingEntry>(
          "one", std::unique_ptr<base::Value>(new base::Value(1))));
  result->push_back(
      std::make_unique<StringMappingListPolicyHandler::MappingEntry>(
          "two", std::unique_ptr<base::Value>(new base::Value(2))));
}

const char kTestPolicy[] = "unit_test.test_policy";
const char kTestPref[] = "unit_test.test_pref";

class TestSchemaValidatingPolicyHandler : public SchemaValidatingPolicyHandler {
 public:
  TestSchemaValidatingPolicyHandler(const Schema& schema,
                                    SchemaOnErrorStrategy strategy)
      : SchemaValidatingPolicyHandler("PolicyForTesting", schema, strategy) {}
  ~TestSchemaValidatingPolicyHandler() override {}

  void ApplyPolicySettings(const policy::PolicyMap&, PrefValueMap*) override {}

  bool CheckAndGetValueForTest(const PolicyMap& policies,
                               std::unique_ptr<base::Value>* value) {
    return SchemaValidatingPolicyHandler::CheckAndGetValue(policies, nullptr,
                                                           value);
  }
};

// Simple implementation of ListPolicyHandler that assumes a string list and
// sets the kTestPref pref to the filtered list.
class StringListPolicyHandler : public ListPolicyHandler {
 public:
  StringListPolicyHandler(const char* policy_name, const char* pref_path)
      : ListPolicyHandler(policy_name, base::Value::Type::STRING) {}

 protected:
  void ApplyList(std::unique_ptr<base::ListValue> filtered_list,
                 PrefValueMap* prefs) override {
    prefs->SetValue(kTestPref, std::move(filtered_list));
  }
};

}  // namespace

TEST(ListPolicyHandlerTest, CheckPolicySettings) {
  base::ListValue list;
  base::DictionaryValue dict;
  policy::PolicyMap policy_map;
  policy::PolicyErrorMap errors;
  StringListPolicyHandler handler(kTestPolicy, kTestPref);

  // No policy set is OK.
  EXPECT_TRUE(handler.CheckPolicySettings(policy_map, &errors));
  EXPECT_TRUE(errors.empty());
  errors.Clear();

  // Not a list is not OK.
  policy_map.Set(kTestPolicy, policy::POLICY_LEVEL_MANDATORY,
                 policy::POLICY_SCOPE_USER, policy::POLICY_SOURCE_CLOUD,
                 dict.CreateDeepCopy(), nullptr);
  EXPECT_FALSE(handler.CheckPolicySettings(policy_map, &errors));
  EXPECT_FALSE(errors.empty());
  errors.Clear();

  // Empty list is OK.
  policy_map.Set(kTestPolicy, policy::POLICY_LEVEL_MANDATORY,
                 policy::POLICY_SCOPE_USER, policy::POLICY_SOURCE_CLOUD,
                 list.CreateDeepCopy(), nullptr);
  EXPECT_TRUE(handler.CheckPolicySettings(policy_map, &errors));
  EXPECT_TRUE(errors.empty());
  errors.Clear();

  // List with an int is OK, but error is added.
  list.AppendInteger(175);  // hex af, 255's sake.
  policy_map.Set(kTestPolicy, policy::POLICY_LEVEL_MANDATORY,
                 policy::POLICY_SCOPE_USER, policy::POLICY_SOURCE_CLOUD,
                 list.CreateDeepCopy(), nullptr);
  EXPECT_TRUE(handler.CheckPolicySettings(policy_map, &errors));
  EXPECT_FALSE(errors.empty());
  list.Clear();
  errors.Clear();

  // List with a string is OK.
  list.AppendString("any_string");
  policy_map.Set(kTestPolicy, policy::POLICY_LEVEL_MANDATORY,
                 policy::POLICY_SCOPE_USER, policy::POLICY_SOURCE_CLOUD,
                 list.CreateDeepCopy(), nullptr);
  EXPECT_TRUE(handler.CheckPolicySettings(policy_map, &errors));
  EXPECT_TRUE(errors.empty());
  errors.Clear();
}

TEST(StringListPolicyHandlerTest, ApplyPolicySettings) {
  base::ListValue list;
  base::ListValue expected;
  PolicyMap policy_map;
  PrefValueMap prefs;
  base::Value* value;
  StringListPolicyHandler handler(kTestPolicy, kTestPref);

  // Empty list applies as empty list.
  policy_map.Set(kTestPolicy, POLICY_LEVEL_MANDATORY, POLICY_SCOPE_USER,
                 POLICY_SOURCE_CLOUD, list.CreateDeepCopy(), nullptr);
  handler.ApplyPolicySettings(policy_map, &prefs);
  EXPECT_TRUE(prefs.GetValue(kTestPref, &value));
  EXPECT_EQ(expected, *value);

  // List with any string applies that string.
  list.AppendString("any_string");
  expected.AppendString("any_string");
  policy_map.Set(kTestPolicy, POLICY_LEVEL_MANDATORY, POLICY_SCOPE_USER,
                 POLICY_SOURCE_CLOUD, list.CreateDeepCopy(), nullptr);
  handler.ApplyPolicySettings(policy_map, &prefs);
  EXPECT_TRUE(prefs.GetValue(kTestPref, &value));
  EXPECT_EQ(expected, *value);
  list.Clear();
  expected.Clear();

  // List with a string and an integer filters out the integer.
  list.AppendString("any_string");
  list.AppendInteger(42);
  expected.AppendString("any_string");
  policy_map.Set(kTestPolicy, POLICY_LEVEL_MANDATORY, POLICY_SCOPE_USER,
                 POLICY_SOURCE_CLOUD, list.CreateDeepCopy(), nullptr);
  handler.ApplyPolicySettings(policy_map, &prefs);
  EXPECT_TRUE(prefs.GetValue(kTestPref, &value));
  EXPECT_EQ(expected, *value);
  list.Clear();
  expected.Clear();
}

TEST(StringToIntEnumListPolicyHandlerTest, CheckPolicySettings) {
  base::ListValue list;
  PolicyMap policy_map;
  PolicyErrorMap errors;
  StringMappingListPolicyHandler handler(
      kTestPolicy,
      kTestPref,
      base::Bind(GetIntegerTypeMap));

  policy_map.Set(kTestPolicy, POLICY_LEVEL_MANDATORY, POLICY_SCOPE_USER,
                 POLICY_SOURCE_CLOUD, list.CreateDeepCopy(), nullptr);
  errors.Clear();
  EXPECT_TRUE(handler.CheckPolicySettings(policy_map, &errors));
  EXPECT_TRUE(errors.empty());

  list.AppendString("one");
  policy_map.Set(kTestPolicy, POLICY_LEVEL_MANDATORY, POLICY_SCOPE_USER,
                 POLICY_SOURCE_CLOUD, list.CreateDeepCopy(), nullptr);
  errors.Clear();
  EXPECT_TRUE(handler.CheckPolicySettings(policy_map, &errors));
  EXPECT_TRUE(errors.empty());

  list.AppendString("invalid");
  policy_map.Set(kTestPolicy, POLICY_LEVEL_MANDATORY, POLICY_SCOPE_USER,
                 POLICY_SOURCE_CLOUD, list.CreateDeepCopy(), nullptr);
  errors.Clear();
  EXPECT_TRUE(handler.CheckPolicySettings(policy_map, &errors));
  EXPECT_FALSE(errors.empty());
  EXPECT_FALSE(errors.GetErrors(kTestPolicy).empty());

  policy_map.Set(kTestPolicy, POLICY_LEVEL_MANDATORY, POLICY_SCOPE_USER,
                 POLICY_SOURCE_CLOUD, std::make_unique<base::Value>("no list"),
                 nullptr);
  errors.Clear();
  EXPECT_FALSE(handler.CheckPolicySettings(policy_map, &errors));
  EXPECT_FALSE(errors.empty());
  EXPECT_FALSE(errors.GetErrors(kTestPolicy).empty());
}

TEST(StringMappingListPolicyHandlerTest, ApplyPolicySettings) {
  base::ListValue list;
  base::ListValue expected;
  PolicyMap policy_map;
  PrefValueMap prefs;
  base::Value* value;
  StringMappingListPolicyHandler handler(
      kTestPolicy,
      kTestPref,
      base::Bind(GetIntegerTypeMap));

  policy_map.Set(kTestPolicy, POLICY_LEVEL_MANDATORY, POLICY_SCOPE_USER,
                 POLICY_SOURCE_CLOUD, list.CreateDeepCopy(), nullptr);
  handler.ApplyPolicySettings(policy_map, &prefs);
  EXPECT_TRUE(prefs.GetValue(kTestPref, &value));
  EXPECT_EQ(expected, *value);

  list.AppendString("two");
  expected.AppendInteger(2);
  policy_map.Set(kTestPolicy, POLICY_LEVEL_MANDATORY, POLICY_SCOPE_USER,
                 POLICY_SOURCE_CLOUD, list.CreateDeepCopy(), nullptr);
  handler.ApplyPolicySettings(policy_map, &prefs);
  EXPECT_TRUE(prefs.GetValue(kTestPref, &value));
  EXPECT_EQ(expected, *value);

  list.AppendString("invalid");
  policy_map.Set(kTestPolicy, POLICY_LEVEL_MANDATORY, POLICY_SCOPE_USER,
                 POLICY_SOURCE_CLOUD, list.CreateDeepCopy(), nullptr);
  handler.ApplyPolicySettings(policy_map, &prefs);
  EXPECT_TRUE(prefs.GetValue(kTestPref, &value));
  EXPECT_EQ(expected, *value);
}

TEST(IntRangePolicyHandler, CheckPolicySettingsClamp) {
  PolicyMap policy_map;
  PolicyErrorMap errors;

  // This tests needs to modify an int policy. The exact policy used and its
  // semantics outside the test are irrelevant.
  IntRangePolicyHandler handler(kTestPolicy, kTestPref, 0, 10, true);

  // Check that values lying in the accepted range are not rejected.
  policy_map.Set(kTestPolicy, POLICY_LEVEL_MANDATORY, POLICY_SCOPE_USER,
                 POLICY_SOURCE_CLOUD, std::make_unique<base::Value>(0),
                 nullptr);
  errors.Clear();
  EXPECT_TRUE(handler.CheckPolicySettings(policy_map, &errors));
  EXPECT_TRUE(errors.empty());

  policy_map.Set(kTestPolicy, POLICY_LEVEL_MANDATORY, POLICY_SCOPE_USER,
                 POLICY_SOURCE_CLOUD, std::make_unique<base::Value>(5),
                 nullptr);
  errors.Clear();
  EXPECT_TRUE(handler.CheckPolicySettings(policy_map, &errors));
  EXPECT_TRUE(errors.empty());

  policy_map.Set(kTestPolicy, POLICY_LEVEL_MANDATORY, POLICY_SCOPE_USER,
                 POLICY_SOURCE_CLOUD, std::make_unique<base::Value>(10),
                 nullptr);
  errors.Clear();
  EXPECT_TRUE(handler.CheckPolicySettings(policy_map, &errors));
  EXPECT_TRUE(errors.empty());

  // Check that values lying outside the accepted range are not rejected
  // (because clamping is enabled) but do yield a warning message.
  policy_map.Set(kTestPolicy, POLICY_LEVEL_MANDATORY, POLICY_SCOPE_USER,
                 POLICY_SOURCE_CLOUD, std::make_unique<base::Value>(-5),
                 nullptr);
  errors.Clear();
  EXPECT_TRUE(handler.CheckPolicySettings(policy_map, &errors));
  EXPECT_FALSE(errors.empty());

  policy_map.Set(kTestPolicy, POLICY_LEVEL_MANDATORY, POLICY_SCOPE_USER,
                 POLICY_SOURCE_CLOUD, std::make_unique<base::Value>(15),
                 nullptr);
  errors.Clear();
  EXPECT_TRUE(handler.CheckPolicySettings(policy_map, &errors));
  EXPECT_FALSE(errors.empty());

  // Check that an entirely invalid value is rejected and yields an error
  // message.
  policy_map.Set(kTestPolicy, POLICY_LEVEL_MANDATORY, POLICY_SCOPE_USER,
                 POLICY_SOURCE_CLOUD, std::make_unique<base::Value>("invalid"),
                 nullptr);
  errors.Clear();
  EXPECT_FALSE(handler.CheckPolicySettings(policy_map, &errors));
  EXPECT_FALSE(errors.empty());
}

TEST(IntRangePolicyHandler, CheckPolicySettingsDontClamp) {
  PolicyMap policy_map;
  PolicyErrorMap errors;

  // This tests needs to modify an int policy. The exact policy used and its
  // semantics outside the test are irrelevant.
  IntRangePolicyHandler handler(kTestPolicy, kTestPref, 0, 10, false);

  // Check that values lying in the accepted range are not rejected.
  policy_map.Set(kTestPolicy, POLICY_LEVEL_MANDATORY, POLICY_SCOPE_USER,
                 POLICY_SOURCE_CLOUD, std::make_unique<base::Value>(0),
                 nullptr);
  errors.Clear();
  EXPECT_TRUE(handler.CheckPolicySettings(policy_map, &errors));
  EXPECT_TRUE(errors.empty());

  policy_map.Set(kTestPolicy, POLICY_LEVEL_MANDATORY, POLICY_SCOPE_USER,
                 POLICY_SOURCE_CLOUD, std::make_unique<base::Value>(5),
                 nullptr);
  errors.Clear();
  EXPECT_TRUE(handler.CheckPolicySettings(policy_map, &errors));
  EXPECT_TRUE(errors.empty());

  policy_map.Set(kTestPolicy, POLICY_LEVEL_MANDATORY, POLICY_SCOPE_USER,
                 POLICY_SOURCE_CLOUD, std::make_unique<base::Value>(10),
                 nullptr);
  errors.Clear();
  EXPECT_TRUE(handler.CheckPolicySettings(policy_map, &errors));
  EXPECT_TRUE(errors.empty());

  // Check that values lying outside the accepted range are rejected and yield
  // an error message.
  policy_map.Set(kTestPolicy, POLICY_LEVEL_MANDATORY, POLICY_SCOPE_USER,
                 POLICY_SOURCE_CLOUD, std::make_unique<base::Value>(-5),
                 nullptr);
  errors.Clear();
  EXPECT_FALSE(handler.CheckPolicySettings(policy_map, &errors));
  EXPECT_FALSE(errors.empty());

  policy_map.Set(kTestPolicy, POLICY_LEVEL_MANDATORY, POLICY_SCOPE_USER,
                 POLICY_SOURCE_CLOUD, std::make_unique<base::Value>(15),
                 nullptr);
  errors.Clear();
  EXPECT_FALSE(handler.CheckPolicySettings(policy_map, &errors));
  EXPECT_FALSE(errors.empty());

  // Check that an entirely invalid value is rejected and yields an error
  // message.
  policy_map.Set(kTestPolicy, POLICY_LEVEL_MANDATORY, POLICY_SCOPE_USER,
                 POLICY_SOURCE_CLOUD, std::make_unique<base::Value>("invalid"),
                 nullptr);
  errors.Clear();
  EXPECT_FALSE(handler.CheckPolicySettings(policy_map, &errors));
  EXPECT_FALSE(errors.empty());
}

TEST(IntRangePolicyHandler, ApplyPolicySettingsClamp) {
  PolicyMap policy_map;
  PrefValueMap prefs;
  std::unique_ptr<base::Value> expected;
  const base::Value* value;

  // This tests needs to modify an int policy. The exact policy used and its
  // semantics outside the test are irrelevant.
  IntRangePolicyHandler handler(kTestPolicy, kTestPref, 0, 10, true);

  // Check that values lying in the accepted range are written to the pref.
  policy_map.Set(kTestPolicy, POLICY_LEVEL_MANDATORY, POLICY_SCOPE_USER,
                 POLICY_SOURCE_CLOUD, std::make_unique<base::Value>(0),
                 nullptr);
  prefs.Clear();
  handler.ApplyPolicySettings(policy_map, &prefs);
  expected.reset(new base::Value(0));
  EXPECT_TRUE(prefs.GetValue(kTestPref, &value));
  EXPECT_EQ(*expected, *value);

  policy_map.Set(kTestPolicy, POLICY_LEVEL_MANDATORY, POLICY_SCOPE_USER,
                 POLICY_SOURCE_CLOUD, std::make_unique<base::Value>(5),
                 nullptr);
  prefs.Clear();
  handler.ApplyPolicySettings(policy_map, &prefs);
  expected.reset(new base::Value(5));
  EXPECT_TRUE(prefs.GetValue(kTestPref, &value));
  EXPECT_EQ(*expected, *value);

  policy_map.Set(kTestPolicy, POLICY_LEVEL_MANDATORY, POLICY_SCOPE_USER,
                 POLICY_SOURCE_CLOUD, std::make_unique<base::Value>(10),
                 nullptr);
  prefs.Clear();
  handler.ApplyPolicySettings(policy_map, &prefs);
  expected.reset(new base::Value(10));
  EXPECT_TRUE(prefs.GetValue(kTestPref, &value));
  EXPECT_EQ(*expected, *value);

  // Check that values lying outside the accepted range are clamped and written
  // to the pref.
  policy_map.Set(kTestPolicy, POLICY_LEVEL_MANDATORY, POLICY_SCOPE_USER,
                 POLICY_SOURCE_CLOUD, std::make_unique<base::Value>(-5),
                 nullptr);
  prefs.Clear();
  handler.ApplyPolicySettings(policy_map, &prefs);
  expected.reset(new base::Value(0));
  EXPECT_TRUE(prefs.GetValue(kTestPref, &value));
  EXPECT_EQ(*expected, *value);

  policy_map.Set(kTestPolicy, POLICY_LEVEL_MANDATORY, POLICY_SCOPE_USER,
                 POLICY_SOURCE_CLOUD, std::make_unique<base::Value>(15),
                 nullptr);
  prefs.Clear();
  handler.ApplyPolicySettings(policy_map, &prefs);
  expected.reset(new base::Value(10));
  EXPECT_TRUE(prefs.GetValue(kTestPref, &value));
  EXPECT_EQ(*expected, *value);
}

TEST(IntRangePolicyHandler, ApplyPolicySettingsDontClamp) {
  PolicyMap policy_map;
  PrefValueMap prefs;
  std::unique_ptr<base::Value> expected;
  const base::Value* value;

  // This tests needs to modify an int policy. The exact policy used and its
  // semantics outside the test are irrelevant.
  IntRangePolicyHandler handler(kTestPolicy, kTestPref, 0, 10, true);

  // Check that values lying in the accepted range are written to the pref.
  policy_map.Set(kTestPolicy, POLICY_LEVEL_MANDATORY, POLICY_SCOPE_USER,
                 POLICY_SOURCE_CLOUD, std::make_unique<base::Value>(0),
                 nullptr);
  prefs.Clear();
  handler.ApplyPolicySettings(policy_map, &prefs);
  expected.reset(new base::Value(0));
  EXPECT_TRUE(prefs.GetValue(kTestPref, &value));
  EXPECT_EQ(*expected, *value);

  policy_map.Set(kTestPolicy, POLICY_LEVEL_MANDATORY, POLICY_SCOPE_USER,
                 POLICY_SOURCE_CLOUD, std::make_unique<base::Value>(5),
                 nullptr);
  prefs.Clear();
  handler.ApplyPolicySettings(policy_map, &prefs);
  expected.reset(new base::Value(5));
  EXPECT_TRUE(prefs.GetValue(kTestPref, &value));
  EXPECT_EQ(*expected, *value);

  policy_map.Set(kTestPolicy, POLICY_LEVEL_MANDATORY, POLICY_SCOPE_USER,
                 POLICY_SOURCE_CLOUD, std::make_unique<base::Value>(10),
                 nullptr);
  prefs.Clear();
  handler.ApplyPolicySettings(policy_map, &prefs);
  expected.reset(new base::Value(10));
  EXPECT_TRUE(prefs.GetValue(kTestPref, &value));
  EXPECT_EQ(*expected, *value);
}

TEST(IntPercentageToDoublePolicyHandler, CheckPolicySettingsClamp) {
  PolicyMap policy_map;
  PolicyErrorMap errors;

  // This tests needs to modify an int policy. The exact policy used and its
  // semantics outside the test are irrelevant.
  IntPercentageToDoublePolicyHandler handler(
      kTestPolicy, kTestPref, 0, 10, true);

  // Check that values lying in the accepted range are not rejected.
  policy_map.Set(kTestPolicy, POLICY_LEVEL_MANDATORY, POLICY_SCOPE_USER,
                 POLICY_SOURCE_CLOUD, std::make_unique<base::Value>(0),
                 nullptr);
  errors.Clear();
  EXPECT_TRUE(handler.CheckPolicySettings(policy_map, &errors));
  EXPECT_TRUE(errors.empty());

  policy_map.Set(kTestPolicy, POLICY_LEVEL_MANDATORY, POLICY_SCOPE_USER,
                 POLICY_SOURCE_CLOUD, std::make_unique<base::Value>(5),
                 nullptr);
  errors.Clear();
  EXPECT_TRUE(handler.CheckPolicySettings(policy_map, &errors));
  EXPECT_TRUE(errors.empty());

  policy_map.Set(kTestPolicy, POLICY_LEVEL_MANDATORY, POLICY_SCOPE_USER,
                 POLICY_SOURCE_CLOUD, std::make_unique<base::Value>(10),
                 nullptr);
  errors.Clear();
  EXPECT_TRUE(handler.CheckPolicySettings(policy_map, &errors));
  EXPECT_TRUE(errors.empty());

  // Check that values lying outside the accepted range are not rejected
  // (because clamping is enabled) but do yield a warning message.
  policy_map.Set(kTestPolicy, POLICY_LEVEL_MANDATORY, POLICY_SCOPE_USER,
                 POLICY_SOURCE_CLOUD, std::make_unique<base::Value>(-5),
                 nullptr);
  errors.Clear();
  EXPECT_TRUE(handler.CheckPolicySettings(policy_map, &errors));
  EXPECT_FALSE(errors.empty());

  policy_map.Set(kTestPolicy, POLICY_LEVEL_MANDATORY, POLICY_SCOPE_USER,
                 POLICY_SOURCE_CLOUD, std::make_unique<base::Value>(15),
                 nullptr);
  errors.Clear();
  EXPECT_TRUE(handler.CheckPolicySettings(policy_map, &errors));
  EXPECT_FALSE(errors.empty());

  // Check that an entirely invalid value is rejected and yields an error
  // message.
  policy_map.Set(kTestPolicy, POLICY_LEVEL_MANDATORY, POLICY_SCOPE_USER,
                 POLICY_SOURCE_CLOUD, std::make_unique<base::Value>("invalid"),
                 nullptr);
  errors.Clear();
  EXPECT_FALSE(handler.CheckPolicySettings(policy_map, &errors));
  EXPECT_FALSE(errors.empty());
}

TEST(IntPercentageToDoublePolicyHandler, CheckPolicySettingsDontClamp) {
  PolicyMap policy_map;
  PolicyErrorMap errors;

  // This tests needs to modify an int policy. The exact policy used and its
  // semantics outside the test are irrelevant.
  IntPercentageToDoublePolicyHandler handler(
      kTestPolicy, kTestPref, 0, 10, false);

  // Check that values lying in the accepted range are not rejected.
  policy_map.Set(kTestPolicy, POLICY_LEVEL_MANDATORY, POLICY_SCOPE_USER,
                 POLICY_SOURCE_CLOUD, std::make_unique<base::Value>(0),
                 nullptr);
  errors.Clear();
  EXPECT_TRUE(handler.CheckPolicySettings(policy_map, &errors));
  EXPECT_TRUE(errors.empty());

  policy_map.Set(kTestPolicy, POLICY_LEVEL_MANDATORY, POLICY_SCOPE_USER,
                 POLICY_SOURCE_CLOUD, std::make_unique<base::Value>(5),
                 nullptr);
  errors.Clear();
  EXPECT_TRUE(handler.CheckPolicySettings(policy_map, &errors));
  EXPECT_TRUE(errors.empty());

  policy_map.Set(kTestPolicy, POLICY_LEVEL_MANDATORY, POLICY_SCOPE_USER,
                 POLICY_SOURCE_CLOUD, std::make_unique<base::Value>(10),
                 nullptr);
  errors.Clear();
  EXPECT_TRUE(handler.CheckPolicySettings(policy_map, &errors));
  EXPECT_TRUE(errors.empty());

  // Check that values lying outside the accepted range are rejected and yield
  // an error message.
  policy_map.Set(kTestPolicy, POLICY_LEVEL_MANDATORY, POLICY_SCOPE_USER,
                 POLICY_SOURCE_CLOUD, std::make_unique<base::Value>(-5),
                 nullptr);
  errors.Clear();
  EXPECT_FALSE(handler.CheckPolicySettings(policy_map, &errors));
  EXPECT_FALSE(errors.empty());

  policy_map.Set(kTestPolicy, POLICY_LEVEL_MANDATORY, POLICY_SCOPE_USER,
                 POLICY_SOURCE_CLOUD, std::make_unique<base::Value>(15),
                 nullptr);
  errors.Clear();
  EXPECT_FALSE(handler.CheckPolicySettings(policy_map, &errors));
  EXPECT_FALSE(errors.empty());

  // Check that an entirely invalid value is rejected and yields an error
  // message.
  policy_map.Set(kTestPolicy, POLICY_LEVEL_MANDATORY, POLICY_SCOPE_USER,
                 POLICY_SOURCE_CLOUD, std::make_unique<base::Value>("invalid"),
                 nullptr);
  errors.Clear();
  EXPECT_FALSE(handler.CheckPolicySettings(policy_map, &errors));
  EXPECT_FALSE(errors.empty());
}

TEST(IntPercentageToDoublePolicyHandler, ApplyPolicySettingsClamp) {
  PolicyMap policy_map;
  PrefValueMap prefs;
  std::unique_ptr<base::Value> expected;
  const base::Value* value;

  // This tests needs to modify an int policy. The exact policy used and its
  // semantics outside the test are irrelevant.
  IntPercentageToDoublePolicyHandler handler(
      kTestPolicy, kTestPref, 0, 10, true);

  // Check that values lying in the accepted range are written to the pref.
  policy_map.Set(kTestPolicy, POLICY_LEVEL_MANDATORY, POLICY_SCOPE_USER,
                 POLICY_SOURCE_CLOUD, std::make_unique<base::Value>(0),
                 nullptr);
  prefs.Clear();
  handler.ApplyPolicySettings(policy_map, &prefs);
  expected.reset(new base::Value(0.0));
  EXPECT_TRUE(prefs.GetValue(kTestPref, &value));
  EXPECT_EQ(*expected, *value);

  policy_map.Set(kTestPolicy, POLICY_LEVEL_MANDATORY, POLICY_SCOPE_USER,
                 POLICY_SOURCE_CLOUD, std::make_unique<base::Value>(5),
                 nullptr);
  prefs.Clear();
  handler.ApplyPolicySettings(policy_map, &prefs);
  expected.reset(new base::Value(0.05));
  EXPECT_TRUE(prefs.GetValue(kTestPref, &value));
  EXPECT_EQ(*expected, *value);

  policy_map.Set(kTestPolicy, POLICY_LEVEL_MANDATORY, POLICY_SCOPE_USER,
                 POLICY_SOURCE_CLOUD, std::make_unique<base::Value>(10),
                 nullptr);
  prefs.Clear();
  handler.ApplyPolicySettings(policy_map, &prefs);
  expected.reset(new base::Value(0.1));
  EXPECT_TRUE(prefs.GetValue(kTestPref, &value));
  EXPECT_EQ(*expected, *value);

  // Check that values lying outside the accepted range are clamped and written
  // to the pref.
  policy_map.Set(kTestPolicy, POLICY_LEVEL_MANDATORY, POLICY_SCOPE_USER,
                 POLICY_SOURCE_CLOUD, std::make_unique<base::Value>(-5),
                 nullptr);
  prefs.Clear();
  handler.ApplyPolicySettings(policy_map, &prefs);
  expected.reset(new base::Value(0.0));
  EXPECT_TRUE(prefs.GetValue(kTestPref, &value));
  EXPECT_EQ(*expected, *value);

  policy_map.Set(kTestPolicy, POLICY_LEVEL_MANDATORY, POLICY_SCOPE_USER,
                 POLICY_SOURCE_CLOUD, std::make_unique<base::Value>(15),
                 nullptr);
  prefs.Clear();
  handler.ApplyPolicySettings(policy_map, &prefs);
  expected.reset(new base::Value(0.1));
  EXPECT_TRUE(prefs.GetValue(kTestPref, &value));
  EXPECT_EQ(*expected, *value);
}

TEST(IntPercentageToDoublePolicyHandler, ApplyPolicySettingsDontClamp) {
  PolicyMap policy_map;
  PrefValueMap prefs;
  std::unique_ptr<base::Value> expected;
  const base::Value* value;

  // This tests needs to modify an int policy. The exact policy used and its
  // semantics outside the test are irrelevant.
  IntPercentageToDoublePolicyHandler handler(
      kTestPolicy, kTestPref, 0, 10, true);

  // Check that values lying in the accepted range are written to the pref.
  policy_map.Set(kTestPolicy, POLICY_LEVEL_MANDATORY, POLICY_SCOPE_USER,
                 POLICY_SOURCE_CLOUD, std::make_unique<base::Value>(0),
                 nullptr);
  prefs.Clear();
  handler.ApplyPolicySettings(policy_map, &prefs);
  expected.reset(new base::Value(0.0));
  EXPECT_TRUE(prefs.GetValue(kTestPref, &value));
  EXPECT_EQ(*expected, *value);

  policy_map.Set(kTestPolicy, POLICY_LEVEL_MANDATORY, POLICY_SCOPE_USER,
                 POLICY_SOURCE_CLOUD, std::make_unique<base::Value>(5),
                 nullptr);
  prefs.Clear();
  handler.ApplyPolicySettings(policy_map, &prefs);
  expected.reset(new base::Value(0.05));
  EXPECT_TRUE(prefs.GetValue(kTestPref, &value));
  EXPECT_EQ(*expected, *value);

  policy_map.Set(kTestPolicy, POLICY_LEVEL_MANDATORY, POLICY_SCOPE_USER,
                 POLICY_SOURCE_CLOUD, std::make_unique<base::Value>(10),
                 nullptr);
  prefs.Clear();
  handler.ApplyPolicySettings(policy_map, &prefs);
  expected.reset(new base::Value(0.1));
  EXPECT_TRUE(prefs.GetValue(kTestPref, &value));
  EXPECT_EQ(*expected, *value);
}

TEST(SchemaValidatingPolicyHandlerTest, CheckAndGetValue) {
  std::string error;
  static const char kSchemaJson[] =
      "{"
      "  \"type\": \"object\","
      "  \"properties\": {"
      "    \"OneToThree\": {"
      "      \"type\": \"integer\","
      "      \"minimum\": 1,"
      "      \"maximum\": 3"
      "    },"
      "    \"Colors\": {"
      "      \"type\": \"string\","
      "      \"enum\": [ \"Red\", \"Green\", \"Blue\" ]"
      "    }"
      "  }"
      "}";
  Schema schema = Schema::Parse(kSchemaJson, &error);
  ASSERT_TRUE(schema.valid()) << error;

  static const char kPolicyMapJson[] =
      "{"
      "  \"PolicyForTesting\": {"
      "    \"OneToThree\": 2,"
      "    \"Colors\": \"White\""
      "  }"
      "}";
  std::unique_ptr<base::Value> policy_map_value =
      base::JSONReader::ReadAndReturnError(kPolicyMapJson, base::JSON_PARSE_RFC,
                                           nullptr, &error);
  ASSERT_TRUE(policy_map_value) << error;

  const base::DictionaryValue* policy_map_dict = nullptr;
  ASSERT_TRUE(policy_map_value->GetAsDictionary(&policy_map_dict));

  PolicyMap policy_map;
  policy_map.LoadFrom(policy_map_dict, POLICY_LEVEL_RECOMMENDED,
                      POLICY_SCOPE_USER, POLICY_SOURCE_CLOUD);

  TestSchemaValidatingPolicyHandler handler(schema, SCHEMA_ALLOW_INVALID);
  std::unique_ptr<base::Value> output_value;
  ASSERT_TRUE(handler.CheckAndGetValueForTest(policy_map, &output_value));
  ASSERT_TRUE(output_value);

  base::DictionaryValue* dict = nullptr;
  ASSERT_TRUE(output_value->GetAsDictionary(&dict));

  // Test that CheckAndGetValue() actually dropped invalid properties.
  int int_value = -1;
  EXPECT_TRUE(dict->GetInteger("OneToThree", &int_value));
  EXPECT_EQ(2, int_value);
  EXPECT_FALSE(dict->HasKey("Colors"));
}

TEST(SimpleSchemaValidatingPolicyHandlerTest, CheckAndGetValue) {
  const char policy_name[] = "PolicyForTesting";
  static const char kSchemaJson[] =
      "{"
      "  \"type\": \"object\","
      "  \"properties\": {"
      "    \"PolicyForTesting\": {"
      "      \"type\": \"object\","
      "      \"properties\": {"
      "        \"OneToThree\": {"
      "          \"type\": \"integer\","
      "          \"minimum\": 1,"
      "          \"maximum\": 3"
      "        },"
      "        \"Colors\": {"
      "          \"type\": \"string\","
      "          \"enum\": [ \"Red\", \"Green\", \"Blue\" ]"
      "        }"
      "      }"
      "    }"
      "  }"
      "}";
  std::string error;
  Schema schema = Schema::Parse(kSchemaJson, &error);
  ASSERT_TRUE(schema.valid()) << error;

  static const char kPolicyMapJson[] =
      "{"
      "  \"PolicyForTesting\": {"
      "    \"OneToThree\": 2,"
      "    \"Colors\": \"Green\""
      "  }"
      "}";
  std::unique_ptr<base::Value> policy_map_value =
      base::JSONReader::ReadAndReturnError(kPolicyMapJson, base::JSON_PARSE_RFC,
                                           nullptr, &error);
  ASSERT_TRUE(policy_map_value) << error;

  const base::DictionaryValue* policy_map_dict = nullptr;
  ASSERT_TRUE(policy_map_value->GetAsDictionary(&policy_map_dict));

  PolicyMap policy_map_recommended;
  policy_map_recommended.LoadFrom(
      policy_map_dict, POLICY_LEVEL_RECOMMENDED, POLICY_SCOPE_USER,
      POLICY_SOURCE_CLOUD);

  PolicyMap policy_map_mandatory;
  policy_map_mandatory.LoadFrom(
      policy_map_dict, POLICY_LEVEL_MANDATORY, POLICY_SCOPE_USER,
      POLICY_SOURCE_CLOUD);

  SimpleSchemaValidatingPolicyHandler handler_all(
      policy_name,
      kTestPref,
      schema,
      SCHEMA_STRICT,
      SimpleSchemaValidatingPolicyHandler::RECOMMENDED_ALLOWED,
      SimpleSchemaValidatingPolicyHandler::MANDATORY_ALLOWED);

  SimpleSchemaValidatingPolicyHandler handler_recommended(
      policy_name,
      kTestPref,
      schema,
      SCHEMA_STRICT,
      SimpleSchemaValidatingPolicyHandler::RECOMMENDED_ALLOWED,
      SimpleSchemaValidatingPolicyHandler::MANDATORY_PROHIBITED);

  SimpleSchemaValidatingPolicyHandler handler_mandatory(
      policy_name,
      kTestPref,
      schema,
      SCHEMA_STRICT,
      SimpleSchemaValidatingPolicyHandler::RECOMMENDED_PROHIBITED,
      SimpleSchemaValidatingPolicyHandler::MANDATORY_ALLOWED);

  SimpleSchemaValidatingPolicyHandler handler_none(
      policy_name,
      kTestPref,
      schema,
      SCHEMA_STRICT,
      SimpleSchemaValidatingPolicyHandler::RECOMMENDED_PROHIBITED,
      SimpleSchemaValidatingPolicyHandler::MANDATORY_PROHIBITED);

  const base::Value* value_expected_in_pref;
  policy_map_dict->Get(policy_name, &value_expected_in_pref);

  PolicyErrorMap errors;
  PrefValueMap prefs;
  base::Value* value_set_in_pref;

  EXPECT_TRUE(handler_all.CheckPolicySettings(policy_map_mandatory, &errors));
  EXPECT_TRUE(errors.empty());
  prefs.Clear();
  handler_all.ApplyPolicySettings(policy_map_mandatory, &prefs);
  EXPECT_TRUE(prefs.GetValue(kTestPref, &value_set_in_pref));
  EXPECT_TRUE(value_expected_in_pref->Equals(value_set_in_pref));

  EXPECT_FALSE(
      handler_recommended.CheckPolicySettings(policy_map_mandatory, &errors));
  EXPECT_FALSE(errors.empty());
  errors.Clear();

  EXPECT_TRUE(
      handler_mandatory.CheckPolicySettings(policy_map_mandatory, &errors));
  EXPECT_TRUE(errors.empty());
  prefs.Clear();
  handler_mandatory.ApplyPolicySettings(policy_map_mandatory, &prefs);
  EXPECT_TRUE(prefs.GetValue(kTestPref, &value_set_in_pref));
  EXPECT_TRUE(value_expected_in_pref->Equals(value_set_in_pref));

  EXPECT_FALSE(handler_none.CheckPolicySettings(policy_map_mandatory, &errors));
  EXPECT_FALSE(errors.empty());
  errors.Clear();

  EXPECT_TRUE(handler_all.CheckPolicySettings(policy_map_recommended, &errors));
  EXPECT_TRUE(errors.empty());
  prefs.Clear();
  handler_all.ApplyPolicySettings(policy_map_mandatory, &prefs);
  EXPECT_TRUE(prefs.GetValue(kTestPref, &value_set_in_pref));
  EXPECT_TRUE(value_expected_in_pref->Equals(value_set_in_pref));

  EXPECT_FALSE(
      handler_mandatory.CheckPolicySettings(policy_map_recommended, &errors));
  EXPECT_FALSE(errors.empty());
  errors.Clear();

  EXPECT_TRUE(
      handler_recommended.CheckPolicySettings(policy_map_recommended, &errors));
  EXPECT_TRUE(errors.empty());
  prefs.Clear();
  handler_recommended.ApplyPolicySettings(policy_map_mandatory, &prefs);
  EXPECT_TRUE(prefs.GetValue(kTestPref, &value_set_in_pref));
  EXPECT_TRUE(value_expected_in_pref->Equals(value_set_in_pref));

  EXPECT_FALSE(
      handler_none.CheckPolicySettings(policy_map_recommended, &errors));
  EXPECT_FALSE(errors.empty());
}

}  // namespace policy
