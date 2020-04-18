// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/common/api/declarative_net_request/test_utils.h"

#include "base/files/file_util.h"
#include "base/json/json_file_value_serializer.h"
#include "base/values.h"
#include "extensions/common/api/declarative_net_request/constants.h"
#include "extensions/common/constants.h"
#include "extensions/common/manifest_constants.h"
#include "extensions/common/value_builder.h"

namespace extensions {
namespace keys = manifest_keys;
namespace declarative_net_request {

namespace {

const base::FilePath::CharType kBackgroundScriptFilepath[] =
    FILE_PATH_LITERAL("background.js");

}  // namespace

TestRuleCondition::TestRuleCondition() = default;
TestRuleCondition::~TestRuleCondition() = default;
TestRuleCondition::TestRuleCondition(const TestRuleCondition&) = default;
TestRuleCondition& TestRuleCondition::operator=(const TestRuleCondition&) =
    default;

std::unique_ptr<base::DictionaryValue> TestRuleCondition::ToValue() const {
  auto value = std::make_unique<base::DictionaryValue>();
  if (url_filter)
    value->SetString(kUrlFilterKey, *url_filter);
  if (is_url_filter_case_sensitive) {
    value->SetBoolean(kIsUrlFilterCaseSensitiveKey,
                      *is_url_filter_case_sensitive);
  }
  if (domains)
    value->Set(kDomainsKey, ToListValue(*domains));
  if (excluded_domains)
    value->Set(kExcludedDomainsKey, ToListValue(*excluded_domains));
  if (resource_types)
    value->Set(kResourceTypesKey, ToListValue(*resource_types));
  if (excluded_resource_types)
    value->Set(kExcludedResourceTypesKey,
               ToListValue(*excluded_resource_types));
  if (domain_type)
    value->SetString(kDomainTypeKey, *domain_type);
  return value;
}

TestRuleAction::TestRuleAction() = default;
TestRuleAction::~TestRuleAction() = default;
TestRuleAction::TestRuleAction(const TestRuleAction&) = default;
TestRuleAction& TestRuleAction::operator=(const TestRuleAction&) = default;

std::unique_ptr<base::DictionaryValue> TestRuleAction::ToValue() const {
  auto value = std::make_unique<base::DictionaryValue>();
  if (type)
    value->SetString(kRuleActionTypeKey, *type);
  if (redirect_url)
    value->SetString(kRedirectUrlKey, *redirect_url);
  return value;
}

TestRule::TestRule() = default;
TestRule::~TestRule() = default;
TestRule::TestRule(const TestRule&) = default;
TestRule& TestRule::operator=(const TestRule&) = default;

std::unique_ptr<base::DictionaryValue> TestRule::ToValue() const {
  auto value = std::make_unique<base::DictionaryValue>();
  if (id)
    value->SetInteger(kIDKey, *id);
  if (priority)
    value->SetInteger(kPriorityKey, *priority);
  if (condition)
    value->Set(kRuleConditionKey, condition->ToValue());
  if (action)
    value->Set(kRuleActionKey, action->ToValue());
  return value;
}

TestRule CreateGenericRule() {
  TestRuleCondition condition;
  condition.url_filter = std::string("filter");
  TestRuleAction action;
  action.type = std::string("blacklist");
  TestRule rule;
  rule.id = kMinValidID;
  rule.action = action;
  rule.condition = condition;
  return rule;
}

std::unique_ptr<base::DictionaryValue> CreateManifest(
    const std::string& json_rules_filename,
    const std::vector<std::string>& hosts,
    bool has_background_script) {
  std::vector<std::string> permissions = hosts;
  permissions.push_back(kAPIPermission);

  std::vector<std::string> background_scripts;
  if (has_background_script)
    background_scripts.push_back("background.js");

  return DictionaryBuilder()
      .Set(keys::kName, "Test extension")
      .Set(keys::kDeclarativeNetRequestKey,
           DictionaryBuilder()
               .Set(keys::kDeclarativeRuleResourcesKey,
                    ToListValue({json_rules_filename}))
               .Build())
      .Set(keys::kPermissions, ToListValue(permissions))
      .Set(keys::kVersion, "1.0")
      .Set(keys::kManifestVersion, 2)
      .Set("background", DictionaryBuilder()
                             .Set("scripts", ToListValue(background_scripts))
                             .Build())
      .Build();
}

std::unique_ptr<base::ListValue> ToListValue(
    const std::vector<std::string>& vec) {
  ListBuilder builder;
  for (const std::string& str : vec)
    builder.Append(str);
  return builder.Build();
}

void WriteManifestAndRuleset(
    const base::FilePath& extension_dir,
    const base::FilePath::CharType* json_rules_filepath,
    const std::string& json_rules_filename,
    const std::vector<TestRule>& rules,
    const std::vector<std::string>& hosts,
    bool has_background_script) {
  ListBuilder builder;
  for (const auto& rule : rules)
    builder.Append(rule.ToValue());
  WriteManifestAndRuleset(extension_dir, json_rules_filepath,
                          json_rules_filename, *builder.Build(), hosts,
                          has_background_script);
}

void WriteManifestAndRuleset(
    const base::FilePath& extension_dir,
    const base::FilePath::CharType* json_rules_filepath,
    const std::string& json_rules_filename,
    const base::Value& rules,
    const std::vector<std::string>& hosts,
    bool has_background_script) {
  // Persist JSON rules file.
  JSONFileValueSerializer(extension_dir.Append(json_rules_filepath))
      .Serialize(rules);

  // Persists a background script if needed.
  if (has_background_script) {
    std::string content = "chrome.test.sendMessage('ready');";
    CHECK_EQ(static_cast<int>(content.length()),
             base::WriteFile(extension_dir.Append(kBackgroundScriptFilepath),
                             content.c_str(), content.length()));
  }

  // Persist manifest file.
  JSONFileValueSerializer(extension_dir.Append(kManifestFilename))
      .Serialize(
          *CreateManifest(json_rules_filename, hosts, has_background_script));
}

}  // namespace declarative_net_request
}  // namespace extensions
