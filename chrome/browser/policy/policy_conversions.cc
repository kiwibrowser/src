// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/policy/policy_conversions.h"

#include "base/json/json_writer.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/policy/chrome_browser_policy_connector.h"
#include "chrome/browser/policy/profile_policy_connector.h"
#include "chrome/browser/policy/profile_policy_connector_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "components/policy/core/browser/policy_error_map.h"
#include "components/policy/core/common/policy_details.h"
#include "components/policy/core/common/policy_namespace.h"
#include "components/policy/core/common/policy_service.h"
#include "components/policy/core/common/policy_types.h"
#include "components/policy/policy_constants.h"
#include "components/strings/grit/components_strings.h"
#include "extensions/buildflags/buildflags.h"

#if BUILDFLAG(ENABLE_EXTENSIONS)
#include "extensions/browser/extension_registry.h"
#include "extensions/common/extension.h"
#include "extensions/common/manifest.h"
#include "extensions/common/manifest_constants.h"
#endif

namespace policy {

namespace {

struct PolicyStringMap {
  const char* key;
  int string_id;
};

const PolicyStringMap kPolicySources[policy::POLICY_SOURCE_COUNT] = {
    {"sourceEnterpriseDefault", IDS_POLICY_SOURCE_ENTERPRISE_DEFAULT},
    {"sourceCloud", IDS_POLICY_SOURCE_CLOUD},
    {"sourceActiveDirectory", IDS_POLICY_SOURCE_ACTIVE_DIRECTORY},
    {"sourcePublicSessionOverride", IDS_POLICY_SOURCE_PUBLIC_SESSION_OVERRIDE},
    {"sourcePlatform", IDS_POLICY_SOURCE_PLATFORM},
};

// Utility function that returns a JSON serialization of the given |dict|.
std::unique_ptr<base::Value> DictionaryToJSONString(
    const base::DictionaryValue& dict) {
  std::string json_string;
  base::JSONWriter::WriteWithOptions(
      dict, base::JSONWriter::OPTIONS_PRETTY_PRINT, &json_string);
  return std::make_unique<base::Value>(json_string);
}

// Returns a copy of |value|. If necessary (which is specified by
// |convert_values|), converts some values to a representation that
// i18n_template.js will display.
std::unique_ptr<base::Value> CopyAndMaybeConvert(const base::Value* value,
                                                 bool convert_values) {
  if (!convert_values)
    return value->CreateDeepCopy();
  const base::DictionaryValue* dict = NULL;
  if (value->GetAsDictionary(&dict))
    return DictionaryToJSONString(*dict);

  std::unique_ptr<base::Value> copy = value->CreateDeepCopy();
  base::ListValue* list = NULL;
  if (copy->GetAsList(&list)) {
    for (size_t i = 0; i < list->GetSize(); ++i) {
      if (list->GetDictionary(i, &dict))
        list->Set(i, DictionaryToJSONString(*dict));
    }
  }

  return copy;
}

PolicyService* GetPolicyService(content::BrowserContext* context) {
  return ProfilePolicyConnectorFactory::GetForBrowserContext(context)
      ->policy_service();
}

// Inserts a description of each policy in |policy_map| into |values|, using
// the optional errors in |errors| to determine the status of each policy. If
// |convert_values| is true, converts the values to show them in javascript.

void GetPolicyValues(const policy::PolicyMap& map,
                     policy::PolicyErrorMap* errors,
                     base::DictionaryValue* values,
                     bool with_user_policies,
                     bool convert_values) {
  for (const auto& entry : map) {
    if (entry.second.scope == policy::POLICY_SCOPE_USER && !with_user_policies)
      continue;
    std::unique_ptr<base::DictionaryValue> value(new base::DictionaryValue);
    value->Set("value",
               CopyAndMaybeConvert(entry.second.value.get(), convert_values));
    if (entry.second.scope == policy::POLICY_SCOPE_USER)
      value->SetString("scope", "user");
    else
      value->SetString("scope", "machine");
    if (entry.second.level == policy::POLICY_LEVEL_RECOMMENDED)
      value->SetString("level", "recommended");
    else
      value->SetString("level", "mandatory");
    value->SetString("source", kPolicySources[entry.second.source].key);
    base::string16 error = errors->GetErrors(entry.first);
    if (!error.empty())
      value->SetString("error", error);
    values->SetWithoutPathExpansion(entry.first, std::move(value));
  }
}

void GetChromePolicyValues(content::BrowserContext* context,
                           base::DictionaryValue* values,
                           bool keep_user_policies,
                           bool convert_values) {
  policy::PolicyService* policy_service = GetPolicyService(context);
  policy::PolicyMap map;

  // Make a copy that can be modified, since some policy values are modified
  // before being displayed.
  map.CopyFrom(policy_service->GetPolicies(
      policy::PolicyNamespace(policy::POLICY_DOMAIN_CHROME, std::string())));

  // Get a list of all the errors in the policy values.
  const policy::ConfigurationPolicyHandlerList* handler_list =
      g_browser_process->browser_policy_connector()->GetHandlerList();
  policy::PolicyErrorMap errors;
  handler_list->ApplyPolicySettings(map, NULL, &errors);

  // Convert dictionary values to strings for display.
  handler_list->PrepareForDisplaying(&map);

  GetPolicyValues(map, &errors, values, keep_user_policies, convert_values);
}

}  // namespace

std::unique_ptr<base::DictionaryValue> GetAllPolicyValuesAsDictionary(
    content::BrowserContext* context,
    bool with_user_policies,
    bool convert_values) {
  base::DictionaryValue all_policies;
  if (!context)
    return std::make_unique<base::DictionaryValue>(std::move(all_policies));

  // Add Chrome policy values.
  auto chrome_policies = std::make_unique<base::DictionaryValue>();
  GetChromePolicyValues(context, chrome_policies.get(), with_user_policies,
                        convert_values);
  all_policies.Set("chromePolicies", std::move(chrome_policies));

#if BUILDFLAG(ENABLE_EXTENSIONS)
  // Add extension policy values.
  extensions::ExtensionRegistry* registry =
      extensions::ExtensionRegistry::Get(Profile::FromBrowserContext(context));
  auto extension_values = std::make_unique<base::DictionaryValue>();

  for (const scoped_refptr<const extensions::Extension>& extension :
       registry->enabled_extensions()) {
    // Skip this extension if it's not an enterprise extension.
    if (!extension->manifest()->HasPath(
            extensions::manifest_keys::kStorageManagedSchema))
      continue;
    auto extension_policies = std::make_unique<base::DictionaryValue>();
    policy::PolicyNamespace policy_namespace = policy::PolicyNamespace(
        policy::POLICY_DOMAIN_EXTENSIONS, extension->id());
    policy::PolicyErrorMap empty_error_map;
    GetPolicyValues(GetPolicyService(context)->GetPolicies(policy_namespace),
                    &empty_error_map, extension_policies.get(),
                    with_user_policies, convert_values);
    extension_values->Set(extension->id(), std::move(extension_policies));
  }
  all_policies.Set("extensionPolicies", std::move(extension_values));
#endif
  return std::make_unique<base::DictionaryValue>(std::move(all_policies));
}

std::string GetAllPolicyValuesAsJSON(content::BrowserContext* context,
                                     bool with_user_policies) {
  std::unique_ptr<base::DictionaryValue> all_policies =
      policy::GetAllPolicyValuesAsDictionary(context, with_user_policies,
                                             false /* convert_values */);
  return DictionaryToJSONString(*all_policies)->GetString();
}

}  // namespace policy
